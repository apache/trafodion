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
 * File:         ex_stored_proc.cpp 
 * Description:  
 *               
 *               
 * Created:      3/15/1997
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "cli_stdh.h"
#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_stored_proc.h"
#include "ex_exe_stmt_globals.h"
#include "ex_io_control.h"
#include "ex_expr.h"
#include "exp_tuple_desc.h"
#include "exp_attrs.h"
#include "exp_clause_derived.h"
#include "ExSqlComp.h"

#include "ErrorMessage.h"
#include "CmpStatement.h"
#include "CmpStoredProc.h"

ex_tcb * ExStoredProcTdb::build(ex_globals * glob)
{ 
  ExStoredProcTcb * spTcb = new(glob->getSpace())
    ExStoredProcTcb(*this, glob);
  
  // add the stored_proc_tcb to the schedule
  spTcb->registerSubtasks();
  
  return (spTcb);
}


ExStoredProcTcb::ExStoredProcTcb(const ExStoredProcTdb &  sp_tdb, 
				 ex_globals * glob
				 ) 
  : ex_tcb( sp_tdb, 1, glob)
{
  Space * space = (glob ? glob->getSpace() : 0);
  
  // Allocate the buffer pool
  pool_ = new(space) sql_buffer_pool(sp_tdb.numBuffers_,
				     sp_tdb.bufferSize_,
				     space);

  // Allocate the queue to communicate with parent
  qparent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
				      sp_tdb.queueSizeDown_,
				      sp_tdb.criDescDown_,
				      space);
  
  // Allocate the private state in each entry of the down queue
  ExStoredProcPrivateState *p = new(space) ExStoredProcPrivateState(this);
  qparent_.down->allocatePstate(p, this);
  delete p;


  qparent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
				   sp_tdb.queueSizeUp_,
				   sp_tdb.criDescUp_,
				   space);

  inputBuffer_ = pool_->get_free_buffer(0);
  returnedBuffer_ = NULL;
  
  step_ = BUILD_INPUT_BUFFER_;

  workAtp_ = allocateAtp(sp_tdb.workCriDesc_, space);
 
  // create the arkcmp that will be used to process this
  // SP request.
  ExMasterStmtGlobals *master_glob = getGlobals()->castToExExeStmtGlobals()
                                                 ->castToExMasterStmtGlobals();
  ex_assert((master_glob != NULL), "Invalid reference in ExStoredProcTcb.");

  // Get the arkcmp_ that contains the query cache
  if (spTdb().getUseExistingArkcmp()) {
    arkcmp_ = master_glob->getCliGlobals()->getArkcmp();
  }
  else {
    arkcmp_ = 
      new(glob->getDefaultHeap()) ExSqlComp(0,
 					    glob->getDefaultHeap(),
					    master_glob->getCliGlobals(),
					    this,
                                            COM_VERS_COMPILER_VERSION,
                                            NULL,
                                            master_glob->getStatement()->
                                              getContext()->getEnvironment());
  }

  ioSubtask_        = NULL;
  numInputRequests_ = 0;

  if (sp_tdb.inputExpr_)
    (void) sp_tdb.inputExpr_->fixup(0, getExpressionMode(), this, space, glob->getDefaultHeap(), FALSE, glob);

  if (sp_tdb.outputExpr_)
    (void) sp_tdb.outputExpr_->fixup(0, getExpressionMode(), this, space, glob->getDefaultHeap(), FALSE, glob);

  if (sp_tdb.predExpr_)
    (void) sp_tdb.predExpr_->fixup(0, getExpressionMode(), this, space, glob->getDefaultHeap(), FALSE, glob);

}

ExStoredProcTcb::~ExStoredProcTcb()
{
  delete qparent_.up;
  delete qparent_.down;

  if (!spTdb().getUseExistingArkcmp()) {
    delete arkcmp_;
  }

  freeResources();
}
  
void ExStoredProcTcb::freeResources()
{
  delete pool_;
  pool_ = 0;
}

void ExStoredProcTcb::registerSubtasks()
{
  ex_tcb::registerSubtasks();

  // register a non-queue event for the IPC with the send top node
  ioSubtask_ =
    getGlobals()->getScheduler()->registerNonQueueSubtask(sWork,this);
}

short ExStoredProcTcb::work()
{
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ExMasterStmtGlobals *master_glob = getGlobals()->castToExExeStmtGlobals()
                                                 ->castToExMasterStmtGlobals();

  // If there is no arkcmp, return
  if (!arkcmp_ && spTdb().getUseExistingArkcmp()) {

    // make sure we have space in the up queue
    if (qparent_.up->isFull()) {
      return WORK_OK;
    }

    queue_index pindex = qparent_.down->getHeadIndex();
    ex_queue_entry *pentry_down = qparent_.down->getQueueEntry(pindex);  	    
    ex_queue_entry *up_entry = qparent_.up->getTailEntry();
    up_entry->upState.status  = ex_queue::Q_NO_DATA;
    up_entry->upState.parentIndex = pentry_down->downState.parentIndex;
    ExStoredProcPrivateState & pstate 
              = *((ExStoredProcPrivateState *) pentry_down->pstate);
    up_entry->upState.setMatchNo(pstate.matchCount_);
	  
    // insert into parent
    qparent_.up->insert();

    pstate.matchCount_ = 0;
    qparent_.down->removeHead();
    numInputRequests_--;

    return WORK_OK;
  }

  while (1)
    {
      switch (step_)
	{
	case BUILD_INPUT_BUFFER_:
	  {
	    queue_index pindex = qparent_.down->getHeadIndex();
	    NABoolean done = FALSE;
	    
	    while (NOT done)
	      {
		if (pindex == qparent_.down->getTailIndex())
		  done = TRUE;
		else
		  {
		    ex_queue_entry *pentry_down = 
		      qparent_.down->getQueueEntry(pindex);

		    tupp_descriptor * dataDesc;
		    
		    if (inputBuffer_->
			moveInSendOrReplyData(TRUE, // send(input) data
					      FALSE, 
					      TRUE, // move data
					      &(pentry_down->downState),
					      sizeof(ControlInfo),
					      0,
					      spTdb().inputRowlen_,
					      &dataDesc,
					      0, NULL) == SqlBuffer::BUFFER_FULL)
		      {
			// no more space in input Buffer.
			done = TRUE;
		      }
		    else
		      {
			numInputRequests_++;

			// move input data
			if (spTdb().inputExpr_) 
			  {
			    workAtp_->getTupp(spTdb().workAtpIndex_) 
			      = dataDesc;
			    spTdb().inputExpr_->eval(pentry_down->getAtp(), workAtp_);
			    workAtp_->getTupp(spTdb().workAtpIndex_).release();
			  }
			
			// move to the next down entry
			pindex++;
			
		      } // move input data
		  } // more entries in down queue
	      } // while not done
              //CmpCommon::getDefault(PROCESS_ISP_LOCALLY) == DF_ON
              if (spTdb().isExecuteInLocalProcess())
                step_ = PROCESS_REQUEST_;
              else
	        // Ship the request to ARKCMP.
	        step_ = SEND_INPUT_BUFFER_;
	  }
	  break;
	case PROCESS_REQUEST_:
	  {
	    //construct request
	    ULng32 sz = inputBuffer_->get_buffer_size();
            inputBuffer_->drivePack();
            inputBuffer_->setBufferStatus(SqlBuffer::IN_USE);
            // The request is dynamically allocated here instead of using stack variable, 
            // because it is owned by a CmpStatementISP inside compileDirect,
            // and will be deleted in destructor of CmpStatementISP.
            // refer to ExCmpMessage::actOnReceive()
	    CmpMessageISPRequest* ispRequest = new (getHeap()) CmpMessageISPRequest((char *)spTdb().spName_,
                                            spTdb().extractInputExpr_, 
                                            spTdb().extractInputExpr_->getLength(), 
                                            spTdb().moveOutputExpr_,
                                            spTdb().moveOutputExpr_->getLength(),
                                            0, 0, 
                                            (void *)inputBuffer_, sz,
                                            spTdb().outputRowlen_, sz, 
                                            getHeap(),
                                            master_glob->getStatement()->getUniqueStmtId(), 
                                            master_glob->getStatement()->getUniqueStmtIdLen());
            //save requestId in case there may be getNext
            requestId_ = ispRequest->id();
            //allocate returnedBuffer
            returnedBuffer_ = pool_->get_free_buffer(0);
            if (returnedBuffer_ == NULL)
	       return WORK_POOL_BLOCKED;
            ULng32 returnedBuflen = returnedBuffer_->get_buffer_size();
            UInt32 dummyDatalen = 0; //not used here
            Int32 dummyCharSet = 0; //not used here
            Int32 cpStatus;
            ComDiagsArea *cpDiagsArea = ComDiagsArea::allocate(getHeap());
            //pass request and returnedBuffer_ to compileDirect
            cpStatus = CmpCommon::context()->compileDirect(
                                 (char*)ispRequest, dummyDatalen,
                                 master_glob->getStatement()->getContext()->exHeap(),
                                 dummyCharSet,
                                 CmpMessageObj::INTERNALSP_REQUEST,
                                 (char* &)returnedBuffer_, returnedBuflen,
                                 master_glob->getStatement()->getContext()->getSqlParserFlags(),

                                 NULL, 0, cpDiagsArea);
            if(cpStatus != 0)//FAILURE
            {
               //replied length exceeds the return buffer length, this will abort an dump the core
               if(returnedBuflen > returnedBuffer_->get_buffer_size())
                  ex_assert(FALSE, "Replied length exceeds receiving buffer length.");
                  
               if(cpDiagsArea->mainSQLCODE() != 0) {
                  ComDiagsArea *mainDiags = getGlobals()->castToExExeStmtGlobals()->getDiagsArea();
                  if (mainDiags == NULL)  {
                     mainDiags = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
                     getGlobals()->castToExExeStmtGlobals()->setGlobDiagsArea(mainDiags);
                     mainDiags->decrRefCount();
                  }
                  mainDiags->mergeAfter(*cpDiagsArea);
               }
               cpDiagsArea->decrRefCount();
               return WORK_BAD_ERROR;
            }
            cpDiagsArea->decrRefCount();
            returnedBuffer_->driveUnpack();
            //on success, returned rows are copied to returnedBuffer_ in compileDirect
	    step_ = RETURN_ROWS_;
	  }
	  break;
	case PROCESS_GETNEXT_:
	  {
	     //getNext request can be allocated in stack, because it's not own by CmpStatementISP
	     //refer to ExCmpMessage::actOnReceive()
             CmpMessageISPGetNext ispRequestGetNext(returnedBuffer_->get_buffer_size(), requestId_, 0, 0);
             returnedBuffer_ = pool_->get_free_buffer(0);
             if (returnedBuffer_ == NULL)
	          return WORK_POOL_BLOCKED;
             ULng32 returnedBuflen = returnedBuffer_->get_buffer_size();
             UInt32 dummyDatalen = 0; //not used here
             Int32 dummyCharSet = 0; //not used here
             Int32 cpStatus;
             ComDiagsArea *cpDiagsArea = ComDiagsArea::allocate(getHeap());
             cpStatus = CmpCommon::context()->compileDirect(
                                 (char*)&ispRequestGetNext, dummyDatalen,
                                 master_glob->getStatement()->getContext()->exHeap(),
                                 dummyCharSet,
                                 CmpMessageObj::INTERNALSP_GETNEXT,
                                 (char* &)returnedBuffer_, returnedBuflen,
                                 master_glob->getStatement()->getContext()->getSqlParserFlags(),
                                 NULL, 0, cpDiagsArea);
                                 
             if(cpStatus != 0)//FAILURE
             {
                 //replied length exceeds the return buffer length, this will abort an dump the core
                 if(returnedBuflen > returnedBuffer_->get_buffer_size())
                    ex_assert(FALSE, "Replied length exceeds receiving buffer length.");
                    
                 if(cpDiagsArea->mainSQLCODE() != 0) {
                    ComDiagsArea *mainDiags = getGlobals()->castToExExeStmtGlobals()->getDiagsArea();
                    if (mainDiags == NULL)  {
                       mainDiags = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
                       getGlobals()->castToExExeStmtGlobals()->setGlobDiagsArea(mainDiags);
                       mainDiags->decrRefCount();
                    }
                    mainDiags->mergeAfter(*cpDiagsArea);
                 }
                 cpDiagsArea->decrRefCount();
                 return WORK_BAD_ERROR;
             }
         
             cpDiagsArea->decrRefCount();
             returnedBuffer_->driveUnpack();
             //on success, returned rows are copied to returnedBuffer_ in compileDirect
	     step_ = RETURN_ROWS_;	    
	  }
	  break;
	  
	case SEND_INPUT_BUFFER_:
	  {
            ULng32 sz = inputBuffer_->get_buffer_size();
            inputBuffer_->drivePack();
	    inputBuffer_->setBufferStatus(SqlBuffer::IN_USE);
	    // Send request to arkcomp and wait for reply.
	    if (arkcmp_->sendRequest(
				     spTdb().spName_,
				     spTdb().extractInputExpr_, 
				     spTdb().extractInputExpr_->getLength(),
				     spTdb().moveOutputExpr_, 
				     spTdb().moveOutputExpr_->getLength(),
				     0, 0,
				     (void *)inputBuffer_,
				     sz,
				     spTdb().outputRowlen_, 
				     sz,
				     spTdb().getUseExistingArkcmp(), 
				             /* If true, then use a waited call. 
					       Use no-waited otherwise*/
				     &requestId_,
                                     master_glob->getStatement()->getUniqueStmtId(),
                                     master_glob->getStatement()->getUniqueStmtIdLen()) !=
		ExSqlComp::SUCCESS)
	      {

                if (arkcmp_->breakReceived())
                {
                   // Break signal was received. Return as quickly as possible
                   // to the scheduler, after setting a flag in the CLI globals
                   // that is used by mxci to determine whether a message
                   // should be returned to indicate that RECOVER needs to 
                   // be run.
                   master_glob->getCliGlobals()->setSPBreakReceived(TRUE);
                   return WORK_BAD_ERROR;
                }
                else
                   master_glob->getCliGlobals()->setSPBreakReceived(FALSE);

                if (arkcmp_->badConnection())
                {
                  // Error reporting in the GET_REPLY_BUFFER_ step
                  // will be used, since the error is in the IPC 
                  // connection.
                }
                else
                {
                  // Get error from the arkcmp_.
                  ex_assert(
                       arkcmp_->getDiags(), 
                       "ExSqlComp::getReply returns error, but no diagnostics");

                  ComDiagsArea *mainDiags = getGlobals()->
                      castToExExeStmtGlobals()->getDiagsArea();

                  if (mainDiags == NULL)
                  {
                    mainDiags = ComDiagsArea::allocate(
                            getGlobals()->getDefaultHeap());
                    getGlobals()->castToExExeStmtGlobals()->
                            setGlobDiagsArea(mainDiags);
                    mainDiags->decrRefCount();
                  }

                  mainDiags->mergeAfter(*arkcmp_->getDiags());

                  return WORK_BAD_ERROR;
                }

	      } // end if !SUCCESS

	    step_ = GET_REPLY_BUFFER_;
	  }
	  break;
	  
	case GET_REPLY_BUFFER_:
	  {
            if (arkcmp_->breakReceived())
            {
               // Break signal was received. Return as quickly as possible
               // to the scheduler, after setting a flag in the CLI globals
               // that is used by mxci to determine whether a message
               // should be returned to indicate that RECOVER needs to
               // be run.
               master_glob->getCliGlobals()->setSPBreakReceived(TRUE);
               return WORK_BAD_ERROR;
            }
            else
               master_glob->getCliGlobals()->setSPBreakReceived(FALSE);

	    if (arkcmp_->badConnection())
	      {
                ComDiagsArea* da = getGlobals()->
                  castToExExeStmtGlobals()->getDiagsArea();
                NABoolean alreadyHadGlobalDA = da ? TRUE : FALSE;
      
                arkcmp_->getServer()->getControlConnection()->
                    populateDiagsArea( da, getHeap());

                if (alreadyHadGlobalDA)
                  {
                    // Preceding populateDiagsArea call has
                    // added a ComCondition to existing DA, so
                    // there's nothing more to do.
                  }
                else
                  {
                    // Preceding populateDiagsArea call has 
                    // created a DiagsArea, added a ComCondition,
                    // and side-effected da so make this da
                    // the global DA.
                    getGlobals()->castToExExeStmtGlobals()->
                      setGlobDiagsArea(da);
                    da->decrRefCount();
                  }
                return WORK_BAD_ERROR;
	      }

	    if (arkcmp_->status() != ExSqlComp::FINISHED)
	      {
	        if (spTdb().getUseExistingArkcmp()) {
	          return WORK_CALL_AGAIN;
	        }
		return WORK_OK; // come back later.
	      }
            
	    // find an empty buffer where data will be returned.
	    returnedBuffer_ = pool_->get_free_buffer(0);
	    if (returnedBuffer_ == NULL)
	      return WORK_POOL_BLOCKED;

	    ULng32 returnedBuflen_ = returnedBuffer_->get_buffer_size();
	    char * rb = (char *)returnedBuffer_;
	    ExSqlComp::ReturnStatus status =
	      arkcmp_->getReply(rb, 
				returnedBuflen_, 
				returnedBuflen_,
				requestId_);
	    if (status == ExSqlComp::MOREDATA)
	      {
		// issue a getNext so arkcmp_ can prepare for
		// the next buffer
	        // waited if sharing arkcmp (last argument)
		status = arkcmp_->getNext(returnedBuflen_, requestId_, spTdb().getUseExistingArkcmp());
	      }

	    if (status == ExSqlComp::ERROR)
	      {
                ex_assert(
                     arkcmp_->getDiags(), 
                     "ExSqlComp::getReply returns error, but no diagnostics");

                ComDiagsArea *mainDiags = getGlobals()->
                    castToExExeStmtGlobals()->getDiagsArea();

                if (mainDiags == NULL)
                {
                  mainDiags = ComDiagsArea::allocate(
                          getGlobals()->getDefaultHeap());
                  getGlobals()->castToExExeStmtGlobals()->
                          setGlobDiagsArea(mainDiags);
                  mainDiags->decrRefCount();
                }

                mainDiags->mergeAfter(*arkcmp_->getDiags());

                return WORK_BAD_ERROR;
	      }

            returnedBuffer_->driveUnpack();
	    
	    step_ = RETURN_ROWS_;
	  }
	  break;
	  
	case RETURN_ROWS_:
	  {
	    // make sure we have space in the up queue
	    if(qparent_.up->isFull()) 
	      return WORK_OK;
 	    
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	
	    tupp p;
            ComDiagsArea* diags;
	    if (returnedBuffer_->moveOutSendOrReplyData(FALSE, // reply data
							&(up_entry->upState),
							p, NULL, &diags)
		== TRUE) // no more rows in this buffer
	      {
		// when we get a buffer from arkcmp, all tupp_descriptors in the
		// buffer have a refcount of 0. To make sure that the buffer is not
		// being reused it's status was IN_USE. Now we have put all
		// the tupps in the buffer into the up queue, i.e., they have a
		// refcount > 0.
		// Mark the buffer as being FULL.
		// It will be reused later after all tupp_descriptors have been
		// consumed and reference count of all have become 0.
		returnedBuffer_->bufferFull();
	
		if (numInputRequests_ > 0)
		  {
		    // still more input requests. Get the
		    // next buffer from arkcmp.
                  if (spTdb().isExecuteInLocalProcess())
                      step_ = PROCESS_GETNEXT_;
		    else
		        step_ = GET_REPLY_BUFFER_;
		  }
		else
		  {
		    // done with all the inputs. 
		    inputBuffer_->init(inputBuffer_->get_buffer_size());
		    step_ = BUILD_INPUT_BUFFER_;
		  }
		return WORK_CALL_AGAIN;
	      }

            ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
            ExStoredProcPrivateState & pstate 
              = *((ExStoredProcPrivateState *) pentry_down->pstate);

	    ex_queue::up_status status = up_entry->upState.status;
	    
	    switch(status) 
	      {		
	      case ex_queue::Q_OK_MMORE: 
		{
		  if (pstate.errorHappened_)
		  {
		    // just ignore this row -- we've already returned an error.
		  }
		  else
		  {
		    // Copy the pointers to the input data
		    up_entry->copyAtp(pentry_down);
		    
		    // set the new pointer
		    up_entry->getTupp(spTdb().criDescUp_->noTuples()-1) =p;

		    // Is there a selection predicate?
		    if (spTdb().predExpr_) {
		      ex_expr::exp_return_type retCode =
			  (spTdb().predExpr_->eval(up_entry->getAtp(), 0));
		      if (retCode == ex_expr::EXPR_FALSE) {
		        break;
		      } 
		      if (retCode == ex_expr::EXPR_ERROR) {
			up_entry->upState.status = ex_queue::Q_SQLERROR;
			up_entry->upState.parentIndex = pentry_down->downState.parentIndex;
			up_entry->upState.downIndex = qparent_.down->getHeadIndex();
			up_entry->upState.setMatchNo(pstate.matchCount_);
 			qparent_.up->insert();
			pstate.errorHappened_ = TRUE;
			break;
		      }
		    }
		    up_entry->upState.parentIndex = pentry_down->downState.parentIndex;
		    up_entry->upState.downIndex = qparent_.down->getHeadIndex();
		    pstate.matchCount_++;
		    up_entry->upState.setMatchNo(pstate.matchCount_);
		    qparent_.up->insert();
		  }
		}
	      break;

	      case ex_queue::Q_NO_DATA:
		{
		  if (diags)
		    {
		      ComDiagsArea * da =
			ComDiagsArea::allocate(
					       getGlobals()->getDefaultHeap());
		      da->unpackObj(diags->getType(),
				    diags->getVersion(),
				    TRUE,      // sameEndianness,
				    0, // dummy for now...
				    (IpcConstMessageBufferPtr) diags);
		      up_entry->setDiagsAreax(da);
		    }

		  // done with this input request.
		  up_entry->upState.status  = ex_queue::Q_NO_DATA;
		  up_entry->upState.parentIndex =
		    pentry_down->downState.parentIndex;
		  up_entry->upState.setMatchNo(pstate.matchCount_);

		  // insert into parent
		  qparent_.up->insert();

		  pstate.matchCount_ = 0;
		  pstate.errorHappened_ = FALSE;

		  qparent_.down->removeHead();

		  numInputRequests_--;
		}
		break;

	      case ex_queue::Q_SQLERROR:
		{
		  if (pstate.errorHappened_)
		  {
		    // just ignore this row -- we've already returned an error.
		  }
		  else
		  {
		    ComDiagsArea * da =
		      ComDiagsArea::allocate(
			   getGlobals()->getDefaultHeap());
		    da->unpackObj(diags->getType(),
				  diags->getVersion(),
				  TRUE,      // sameEndianness,
				  0, // dummy for now...
				  (IpcConstMessageBufferPtr) diags);
		    up_entry->setDiagsAreax(da);

		    // insert into parent
		    qparent_.up->insert();
		  }
		}
	      break;

	      case ex_queue::Q_INVALID:
		ex_assert(0,
			  "ExStoredProcTcb::work() Invalid state returned by arkcmp");
		break;
	      } // switch status
	  }
	  break;

	} // switch step_
    } // while

  return WORK_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for private state.
/////////////////////////////////////////////////////////////////////////////

ExStoredProcPrivateState::ExStoredProcPrivateState(const ExStoredProcTcb * 
						   /*tcb*/)
{
  matchCount_ = 0;
  errorHappened_ = FALSE;
}

ExStoredProcPrivateState::~ExStoredProcPrivateState()
{
};

ex_tcb_private_state * ExStoredProcPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) ExStoredProcPrivateState((ExStoredProcTcb *) tcb);
};

/////////////////////////////////////////////////
// class ExSPInputOutput
/////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// Moves data value from fieldNum'th position in inputRow to 'data.
// fieldNum is zero based, that is, the first field is number 0.
// Returns -1 in case of error. diagsArea contains the error number
// in this case (well, almost always).
////////////////////////////////////////////////////////////////////
short ExSPInputOutput::inputValue(ULng32 fieldNum, char * inputRow,
				  char * data, ULng32 datalen, NABoolean casting,
				  ComDiagsArea * diagsArea)
{
  Attributes * attr = tupleDesc_->getAttr((short)fieldNum);
  short varcharlen = 2;

  // diagsArea must be present. And fieldNum must be within range.
  if ((diagsArea == NULL) ||
      (fieldNum < 0) || 
      (fieldNum >= tupleDesc_->numAttrs()))
    {
      // return error
      return -1;
    }

  if (casting == FALSE)
    convDoIt(&inputRow[attr->getOffset()],
	     attr->getLength(&inputRow[attr->getVCLenIndOffset()]),
	     attr->getDatatype(),
	     attr->getPrecision(),
	     attr->getScale(),

	     // Pass in pointer to target data.
	     // if varchar, then data points to varchar length and
	     // 'data + varcharIndLen' points to the real data.
	     &data[((attr->getVCIndicatorLength() <= 0) ? 0 : 
		    attr->getVCIndicatorLength())],
	     attr->getLength(),
	     attr->getDatatype(),
	     attr->getPrecision(),
	     attr->getScale(),
             (attr->getVCIndicatorLength() > 0 ? data : NULL),
             attr->getVCIndicatorLength(),
	     NULL, /*CollHeap *heap,*/
	     &diagsArea,
	     CONV_UNKNOWN_LEFTPAD);
  else
    {
      //      TBD:
      //      if (datalen < (displayLength(attr) + 
      //               ((attr->getVCIndicatorLength() > 0) ?
      //                 attr->getVCIndicatorLength() : 0))
      //	; // return error.

      // target has to be formatted as a varchar. The first two bytes
      // of target will contain the actual target length, followed
      // by data bytes.
      convDoIt(&inputRow[attr->getOffset()],
	       attr->getLength(&inputRow[attr->getVCLenIndOffset()]),
	       attr->getDatatype(),
	       attr->getPrecision(),
	       attr->getScale(),
	       &data[varcharlen],
	       datalen - varcharlen,
	       REC_BYTE_V_ASCII,
	       0,
	       0,
	       data,
	       varcharlen,
	       NULL, /*CollHeap *heap,*/
	       &diagsArea,
	       CONV_UNKNOWN);
    }

  return 0;
}

////////////////////////////////////////////////////////////////////
// Moves data value from 'data' to fieldNum'th position in
// outputRow.
// fieldNum is zero based, that is, the first field is number 0.
// Returns -1 in case of error. diagsArea contains the error number
// in this case (well, almost always).
////////////////////////////////////////////////////////////////////
short ExSPInputOutput::outputValue(ULng32 fieldNum,
				   char * outputRow,
				   char * data,
				   ULng32 datalen,
				   NABoolean casting,
				   CollHeap * heap,
				   ComDiagsArea * diagsArea)
{
  Attributes * attr = tupleDesc_->getAttr((short)fieldNum);
  ex_expr::exp_return_type status = ex_expr::EXPR_OK;

  short nullIndicatorLen = attr->getNullIndicatorLength(),
    varcharIndLen    = attr->getVCIndicatorLength(),
    scale            = attr->getScale(),
    dataType         = attr->getDatatype();
  
  Lng32  nullIndOffset    = attr->getNullIndOffset(),
    varcharIndOffset = attr->getVCLenIndOffset(),
    precision        = attr->getPrecision();

  // diagsArea must be present. And fieldNum must be within range.
  if ((diagsArea == NULL) ||
      (fieldNum < 0) || 
      (fieldNum >= tupleDesc_->numAttrs()))
    {
      // return error
      ExRaiseSqlError(heap, &diagsArea, EXE_FIELD_NUM_OVERFLOW);
      return ex_expr::EXPR_ERROR;
    }

  if (data == NULL)
    {
      // a null value has been passed in. Move in a null value.

      if (attr->getNullFlag())
	{
	  // make the result null.
	  str_pad(&outputRow[nullIndOffset], 
		  nullIndicatorLen,
		  '\377');
	  return status;
	}
      else
	{
	  // return error
	  ExRaiseSqlError(heap, &diagsArea, EXE_ASSIGNING_NULL_TO_NOT_NULL);
	  return ex_expr::EXPR_ERROR;
	}
    } // null value

  if (casting == FALSE)
    status = convDoIt(
		      // Pass in pointer to real data.
		      // if varchar, then data points to varchar length and
		      // 'data + varcharIndLen' points to the real data.
		      &data[((varcharIndLen <= 0) ? 0 : varcharIndLen)],
		      attr->getLength(data),
		      dataType,
		      precision,
		      scale,
		      &outputRow[attr->getOffset()],
		      attr->getLength(),
		      dataType,
		      precision,
		      scale,
		      &outputRow[varcharIndOffset],
		      varcharIndLen,
		      heap,
		      &diagsArea,
		      CONV_UNKNOWN_LEFTPAD);
  else
    status = convDoIt(&data[0],
		      datalen,
		      REC_BYTE_F_ASCII,
		      precision,
		      scale,
		      &outputRow[attr->getOffset()],
		      attr->getLength(),
		      dataType,
		      precision,
		      scale,
		      (varcharIndLen > 0 ?
		       &outputRow[varcharIndOffset] : NULL),
		      varcharIndLen,
		      heap,
		      &diagsArea,
		      (getCaseIndexArray() ? getCaseIndexArray()[fieldNum]
		                           : CONV_UNKNOWN_LEFTPAD)); 

  if ((status == ex_expr::EXPR_OK) && attr->getNullFlag())
    {
      // make the result not null.
      str_pad(&outputRow[nullIndOffset], 
	      nullIndicatorLen,
	      '\0');
    }
  return status;
}

// Moved to cli/StoredProcInterface.cpp
 
