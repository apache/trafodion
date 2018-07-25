/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         /executor/ex_timeout.cpp
 * Description:  Implement methods of ExTimeoutTdb, ExTimeoutTcb
 *               
 * Created:      12/27/1999
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include  "Platform.h"
#include  "cli_stdh.h"
#include  "ex_stdh.h"

#include  "ComTdb.h"
#include  "ex_tcb.h"

#include  "ex_timeout.h"
#include  "ex_root.h"
#include  "ex_exe_stmt_globals.h"
#include  "exp_expr.h"
#include  "ex_error.h"

#include  "ExSqlComp.h"

#include "timeout_data.h"


/////////////////////////////////////////////////////////////////
// class ExTimeoutTdb, ExTimeoutTcb, ExTimeoutPrivateState
/////////////////////////////////////////////////////////////////

ex_tcb * ExTimeoutTdb::build(ex_globals * glob)
{
  ExTimeoutTcb * timeout_tcb = 
    new(glob->getSpace()) ExTimeoutTcb(*this, (ExMasterStmtGlobals *)glob);
  
  timeout_tcb->registerSubtasks();

  return (timeout_tcb);
}


////////////////////////////////////////////////////////////////
// Constructor for class ExTimeoutTcb
///////////////////////////////////////////////////////////////
ExTimeoutTcb::ExTimeoutTcb(const ExTimeoutTdb & timeout_tdb,
			   ExMasterStmtGlobals * glob)
  : ex_tcb( timeout_tdb, 1, glob)
{
  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);
  
  // Allocate the buffer pool
  pool_ = new(space) sql_buffer_pool(timeout_tdb.numBuffers_,
				     timeout_tdb.bufferSize_,
				     space);
  
  // Allocate the queue to communicate with parent
  qparent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
				      timeout_tdb.queueSizeDown_,
				      timeout_tdb.criDescDown_,
				      space);
  
  // Allocate the private state in each entry of the down queue
  ExTimeoutPrivateState *p = new(space) ExTimeoutPrivateState(this);
  qparent_.down->allocatePstate(p, this);
  delete p;
  
  qparent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
				    timeout_tdb.queueSizeUp_,
				    timeout_tdb.criDescUp_,
				    space);
  
  // Get the name of the table from the late-name-info in the root-tdb
  ex_root_tdb * rootTdb = (ex_root_tdb *) glob->getFragmentPtr(0) ;
  LateNameInfoList * lnil = rootTdb->getLateNameInfoList();
  ex_assert( lnil , "No late name info list in root TDB!");
  theTableName_ = lnil->getLateNameInfo(0).resolvedPhyName();
  ex_assert( theTableName_ , "No table name !");

  if (timeoutValueExpr())  {
    // allocate work atp to compute the timeout value
    workAtp_ = allocateAtp(timeout_tdb.workCriDesc_, space);
    
    // allocate tuple where the timeout value will be moved
    pool_->get_free_tuple(workAtp_->
			  getTupp(timeout_tdb.workCriDesc_->noTuples() - 1), 
			  sizeof(Lng32));
    
    (void) timeoutValueExpr()->fixup(0, getExpressionMode(), this,
				     space, heap, FALSE, glob);
  }
  else
    workAtp_ = 0;
}

// dtor
ExTimeoutTcb::~ExTimeoutTcb()
{
  delete qparent_.up;
  delete qparent_.down;
  delete pool_;
  pool_ = 0;
};

//////////////////////////////////////////////////////
// work() for ExTimeoutTcb
//////////////////////////////////////////////////////
short ExTimeoutTcb::work()
{
  while (1) {
    // if no parent request, return
    if (qparent_.down->isEmpty())
      return WORK_OK;
      
    // if no room in up queue, won't be able to return data/status.
    // Come back later.
    if (qparent_.up->isFull()) return WORK_OK;
      
    ex_queue_entry * pentry_down = qparent_.down->getHeadEntry(); 
    ExTimeoutPrivateState & pstate =
      *((ExTimeoutPrivateState*) pentry_down->pstate);

    // Calculate the timeout actual value
    Lng32 timeoutValue = 0; 
    NABoolean goodTimeoutValue = TRUE ;
    if (timeoutValueExpr()) {
      if ( timeoutValueExpr()->eval(pentry_down->getAtp(), workAtp_) 
	   == ex_expr::EXPR_ERROR ) { // expression did not yield a valid value
	handleErrors(pentry_down, pentry_down->getAtp()->getDiagsArea()); 
	goodTimeoutValue = FALSE ;
      } else {
	tupp TP = workAtp_->getTupp(timeoutTdb().workCriDesc_->noTuples()-1);
	timeoutValue = *(Lng32 *)TP.getDataPointer(); // pointer is (char *)
      }
    }

    /****************************************************/
    /********   Do the actual SET TIMEOUT work   ********/
    /**                                                **/
    /** The scope of the work is only making changes   **/
    /** to the global timeout data kept at the context **/
    /****************************************************/

    // Get the global timeout-data object
    ContextCli * currContext = getGlobals()->castToExExeStmtGlobals()->
      castToExMasterStmtGlobals()->getStatement()->getContext();
    
    TimeoutData * GlobalTimeouts = currContext->getTimeouts();

#if _DEBUG    // For debugging only !!!!!
    if ( getenv("DEBUG_TIMEOUT") ) {
      ComDiagsArea* diagsArea = 
	ComDiagsArea::allocate (getGlobals()->getDefaultHeap());
      char errmsg[120];

      if ( timeoutTdb().isStream() ) {   // it was a SET STREAM TIMEOUT
	if ( GlobalTimeouts->isStreamTimeoutSet() ) {
	  sprintf(errmsg, "Stream timeout set to %d\n",
		  GlobalTimeouts->getStreamTimeout() );
	} else sprintf(errmsg, "Stream timeout was NOT SET ! \n");
      } // lock timeout -- not stream
      else {
	if ( theTableName_[0] == '*' ) { // For all tables
	  sprintf(errmsg, "Number of lock timeouts set: %d\n",
		  GlobalTimeouts->entries() );
	} else {
	  Lng32 timeoutValue;
	  NABoolean found = 
	    GlobalTimeouts->getLockTimeout(theTableName_, timeoutValue );
	  if ( ! found ) 
	    sprintf(errmsg, "Lock timeout for table %s was NOT SET ! \n",
		    theTableName_ );
	  else sprintf(errmsg, "Lock timeout for table %s is %d \n",
		       theTableName_ , timeoutValue );
	}
      }
      // emit message as an error ( msg 3066 has no text of its own )
      *diagsArea << DgSqlCode(-3066)
		<< DgString0(errmsg) ;
      ExHandleArkcmpErrors(qparent_, pentry_down, 0, getGlobals(), 
			   diagsArea, (ExeErrorCode) -3066 );
    }  // end of debugging section  
    else   
#endif    

    if ( goodTimeoutValue ) {

      // Update the globals as needed
      if ( timeoutTdb().isStream() ) {   // it was a SET STREAM TIMEOUT
	if ( timeoutTdb().isReset() )  // it was a RESET
	  GlobalTimeouts->resetStreamTimeout(); 
	else                           // it was a SET (with a value)
	  GlobalTimeouts->setStreamTimeout(timeoutValue);
      }
      else {                     // setting a LOCK TIMEOUT
	// TBD =============>>> Check if FORALL string includes CAT.SCH ......
	if ( theTableName_[0] == '*' ) { // For all tables
	  if ( timeoutTdb().isReset() )  // it was a RESET
	    GlobalTimeouts->resetAllLockTimeout();
	  else GlobalTimeouts->setAllLockTimeout(timeoutValue);
	}
	else {  // per specific table
	  if ( timeoutTdb().isReset() )  // it was a RESET
	    GlobalTimeouts->resetTableLockTimeout( theTableName_ );
	  else GlobalTimeouts->setTableLockTimeout(theTableName_,timeoutValue);
	}
      }

      // execution of every SET TIMEOUT stmt increments the change counter !!
      currContext->incrementTimeoutChangeCounter();

      // clear up (i.e. deallocate) the global timeout data, if possible
      if ( GlobalTimeouts->noLockTimeoutsSet() &&
	   ! GlobalTimeouts->isStreamTimeoutSet() )
	currContext->clearTimeoutData();

    } // end of if ( goodTimeoutValue )

    /**********  at this point the actual work is done  ******************/
      
    // all ok. Return EOF.
    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
      
    up_entry->upState.parentIndex = 
      pentry_down->downState.parentIndex;
      
    up_entry->upState.setMatchNo(0);
    up_entry->upState.status = ex_queue::Q_NO_DATA;
      
    // insert into parent
    qparent_.up->insert();
    
    qparent_.down->removeHead();
  }  
  return WORK_OK;
}

// if diagsArea is not NULL, then its error code is used.
// Otherwise, err is used to handle error.
void ExTimeoutTcb::handleErrors(ex_queue_entry *pentry_down, 
				ComDiagsArea *da,
				ExeErrorCode err)
{
  ExHandleArkcmpErrors(qparent_, pentry_down, 0, getGlobals(), da, err);
}

//////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for Lock_private_state
//////////////////////////////////////////////////////////////////////////////

ExTimeoutPrivateState::ExTimeoutPrivateState(const ExTimeoutTcb * /*tcb*/)
{
}

ExTimeoutPrivateState::~ExTimeoutPrivateState()
{
};

ex_tcb_private_state * ExTimeoutPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) 
    ExTimeoutPrivateState((ExTimeoutTcb *) tcb);
};


