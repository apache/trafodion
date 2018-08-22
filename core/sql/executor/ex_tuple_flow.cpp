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
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_tuple_flow.h"
#include "ex_exe_stmt_globals.h"
#include "ex_expr.h"
#include "str.h"
#include "ExStats.h"
#include "ExpError.h"
#include "cli_stdh.h"

/////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
/////////////////////////////////////////////////////////////////////////

ex_tcb * ExTupleFlowTdb::build(ex_globals * glob)
{
  // first build the children
  ex_tcb * src_tcb = tdbSrc_->build(glob);
  ex_tcb * tgt_tcb = tdbTgt_->build(glob);
  
  ExTupleFlowTcb * tuple_flow_tcb =
    new(glob->getSpace()) ExTupleFlowTcb(*this, 
  					 *src_tcb, *tgt_tcb, glob);
  
  // add the tuple_flow_tcb to the schedule
  tuple_flow_tcb->registerSubtasks();

  return (tuple_flow_tcb);
}

 
/////////////////////////////////////////////////////////////////////////
//
//  TCB procedures
//
/////////////////////////////////////////////////////////////////////////
ExTupleFlowTcb::ExTupleFlowTcb(const ExTupleFlowTdb &  tuple_flow_tdb, 
			       const ex_tcb & src_tcb,    // src queue pair
			       const ex_tcb & tgt_tcb,    // tgt queue pair
			       ex_globals *glob
                         ) : ex_tcb( tuple_flow_tdb, 1, glob)
{
  CollHeap * space = glob->getSpace();
  
  // Allocate the buffer pool, if tgtExpr_ is present
  if (tuple_flow_tdb.tgtExpr_)
    pool_ = new(glob->getSpace()) sql_buffer_pool(tuple_flow_tdb.numBuffers_,
						  tuple_flow_tdb.bufferSize_,
						  glob->getSpace());
  
  tcbSrc_ = &src_tcb;
  tcbTgt_ = &tgt_tcb;

  // get the queues that src and tgt use to communicate with me
  qSrc_  = src_tcb.getParentQueue();
  qTgt_  = tgt_tcb.getParentQueue();

  ex_cri_desc * from_parent_cri = tuple_flow_tdb.criDescDown_;  
  ex_cri_desc * to_parent_cri   = tuple_flow_tdb.criDescUp_;

  // Allocate the queue to communicate with parent
  qParent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
				      tuple_flow_tdb.queueSizeDown_,
				      from_parent_cri,
				      space);

  // source atps will be copied to target atps. Allocate
  // target atps.
  qTgt_.down->allocateAtps(space);
  
  // Allocate the private state in each entry of the down queue
  ExTupleFlowPrivateState *p =
    new(space) ExTupleFlowPrivateState(this);
  qParent_.down->allocatePstate(p, this);
  delete p;

  qParent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
				    tuple_flow_tdb.queueSizeUp_,
				    to_parent_cri,
				    space);

  // fixup expressions
  if (tflowTdb().tgtExpr_)
    (void) tflowTdb().tgtExpr_->fixup(0, getExpressionMode(), this,
  				      glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);
    
}

ExTupleFlowTcb::~ExTupleFlowTcb()
{
  delete qParent_.up;
  delete qParent_.down;
  freeResources();
}
 
void ExTupleFlowTcb::freeResources()
{
  delete pool_;
  pool_ = 0;
}

short ExTupleFlowTcb::work()
{
  // This is some sort of a hack to fix the problems with the number of rows
  // inserted returned to the user for packed tables. For these tables, rows
  // are packed (by the Pack node which is the left child of this tuple flow)
  // before they are sent off to DP2. DP2 has no idea that it's actually
  // inserting multiple logical rows (as perceived by the user). However,
  // there is actually a hidden count of logical rows stored as the first 4
  // bytes of the packed row. This counter is supposed to keep track of a sum
  // of this number in each packed row it gets from the left. When all
  // insertions are done, this sum is used to override what's stored by the
  // PA node in the executor global area the number of rows inserted. This is
  // not a very good place to have this fix, but since this is a low-priority
  // problem at this time, here we are.
  //                                                       
  // 
  // NB: The code introduced for this fix 
  //      could be safely removed if desired. Also, all changes are within
  //     this file.
  // 

  if (qParent_.down->isEmpty())
    return WORK_OK;
  
  ex_queue_entry * pentry_down = qParent_.down->getHeadEntry();
  ExTupleFlowPrivateState &  pstate = 
    *((ExTupleFlowPrivateState*) pentry_down->pstate);

  if ((tflowTdb().userSidetreeInsert()) &&
      (pentry_down->downState.request == ex_queue::GET_EOD) &&
      (NOT pstate.parentEOD_))
    {
      pstate.step_ = MOVE_EOD_TO_TGT_;
    }
  else if ((pstate.step_ != DONE_) &&
     (pstate.step_ != CANCELLED_) &&
      (pentry_down->downState.request == ex_queue::GET_NOMORE))
    {
      if (pstate.step_ == EMPTY_)
        pstate.step_ = DONE_;
      else
        pstate.step_ = CANCELLED_;
    }

  while (1)
    {
      switch (pstate.step_)
	{
	case EMPTY_:
	  {
	    if (qSrc_.down->isFull())
	      return WORK_OK;

	    ex_queue_entry * src_entry = qSrc_.down->getTailEntry();

	    src_entry->downState.request = pentry_down->downState.request;
	    src_entry->downState.requestValue = 
	      pentry_down->downState.requestValue;

	    if ((tflowTdb().firstNRows() >= 0) &&
		(pentry_down->downState.request != ex_queue::GET_N))
	      {
		src_entry->downState.request = ex_queue::GET_N;
		src_entry->downState.requestValue = tflowTdb().firstNRows();
	      }

	    src_entry->downState.parentIndex = 
              qParent_.down->getHeadIndex();

	    src_entry->passAtp(pentry_down);
	    
	    qSrc_.down->insert();

	    // just checking to make sure we got a diags area from the CLI if we are 
	    // executing a non-tomic insert. This is done now so that we don't have to do it in multiple
	    // places later.
	    if (tflowTdb().isNonFatalErrorTolerated()) {
	      ComDiagsArea *cliDiagsArea = pentry_down->getDiagsArea();
	      ex_assert(cliDiagsArea, "In Tupleflow : Non-Atomic insert received no diags area from the CLI");
	    }

	    pstate.parentEOD_ = FALSE;
	    pstate.srcEOD_ = FALSE;
            pstate.matchCount_ = 0;
            pstate.tgtRequests_ = 0;
	    pstate.tgtRowsSent_ = FALSE;
	    pstate.noOfUnPackedRows_ = 0;
	    pstate.srcRequestCount_ = -1;
            pstate.nonFatalErrorSeen_ = FALSE;
	    // Set startRightIndex_ so that CancelReques doesn't do anything.
	    pstate.startRightIndex_ = pstate.srcRequestCount_;
	    pstate.step_ = MOVE_SRC_TO_TGT_;
	  }
	  break;

	case MOVE_SRC_TO_TGT_:
	  {
	    // if there are some rows in source up queue, move them to target.
	    while ((! qSrc_.up->isEmpty()) && (! qTgt_.down->isFull())
                     && (pstate.step_ != HANDLE_ERROR_))
	      {
		ex_queue_entry * src_entry = qSrc_.up->getHeadEntry();
		ex_queue_entry * tgt_entry = qTgt_.down->getTailEntry();

		switch (src_entry->upState.status)
		  {
		  case ex_queue::Q_OK_MMORE:
		    {
		      // move this source row to target.
                      
                      // BEGIN:  - Read note at beginning of work().
                      // 
                      if (tcbSrc_->getNodeType() == ComTdb::ex_PACKROWS)
                      {
                        char* packTuppPtr =
                          src_entry->getTupp(src_entry->numTuples()-1)
			  .getDataPointer();
                        Int32 noOfRows = *((Int32 *)packTuppPtr);
                        pstate.noOfUnPackedRows_ += (noOfRows - 1);
			
                      }
                      //
                      // END:- Read note at beginning of work().

		      pstate.srcRequestCount_++;
		      tgt_entry->downState.request = 
			pentry_down->downState.request;
		      tgt_entry->downState.requestValue = 
			pentry_down->downState.requestValue;
		      tgt_entry->downState.parentIndex = 
                       (Lng32) pstate.srcRequestCount_;
		      tgt_entry->copyAtp(src_entry);
		      qTgt_.down->insert();
                      pstate.tgtRequests_++;
	              pstate.tgtRowsSent_ = TRUE;
		      qSrc_.up->removeHead();
		    }
		    break;

		  case ex_queue::Q_NO_DATA:
		    {
		      if ((tflowTdb().vsbbInsertOn()) &&
	                  (pstate.tgtRowsSent_ == TRUE))
			{
			  if (tflowTdb().userSidetreeInsert())
			    {
			      tgt_entry->downState.request = 
				ex_queue::GET_EOD_NO_ST_COMMIT;
			    }
			  else
			    {
			      tgt_entry->downState.request = 
				ex_queue::GET_EOD;
			    }

			  tgt_entry->downState.requestValue = 
			    pentry_down->downState.requestValue;
			  tgt_entry->downState.parentIndex = 
                            (Lng32) pstate.srcRequestCount_;
			  tgt_entry->copyAtp(src_entry);
			  
			  qTgt_.down->insert();
                          pstate.tgtRequests_++;
			}

	              if ((pstate.tgtRowsSent_ == FALSE) &&
			  (src_entry->getDiagsArea()))
			{
			  // a warning is returned with EOD and
			  // nothing else was returned from source.
			  // Move warning to parent's up queue.
			  if (qParent_.up->isFull())
			    return WORK_OK;

			  ex_queue_entry * up_entry = 
			    qParent_.up->getTailEntry();

                          if (src_entry->getDiagsArea())
                            src_entry->getDiagsArea()->incrRefCount();

			  up_entry->setDiagsArea(src_entry->getDiagsArea());
			}

		      qSrc_.up->removeHead();
		      
		      pstate.srcEOD_ = TRUE;
                      
		      if (tflowTdb().sendEODtoTgt())
			pstate.step_ = MOVE_EOD_TO_TGT_;
		    }
		    break;
		    
		  case ex_queue::Q_SQLERROR:
                    {
	              if (qParent_.up->isFull())
	                return WORK_OK;
	    
	              ex_queue_entry * pentry = qParent_.up->getTailEntry();
		      ComDiagsArea * da = src_entry->getDiagsArea();
		      ex_assert(da, "We have a Q_SQLERROR in Tupleflow but no diags area");
		      
		      if (tflowTdb().isNonFatalErrorTolerated() &&
			 (da->getNextRowNumber(ComCondition::NONFATAL_ERROR) == 
			  ComCondition::NONFATAL_ERROR)) 
			{
			  pstate.nonFatalErrorSeen_ = TRUE;		
			}
		      else 
			{
			  pstate.step_ = HANDLE_ERROR_;
			  pstate.nonFatalErrorSeen_ = FALSE;
			}

		      pstate.srcRequestCount_++;
		      if(tflowTdb().isRowsetIterator())
	                da->setAllRowNumber((Lng32) pstate.srcRequestCount_); 

                      ComDiagsArea *accumulatedDiagsArea = pentry->getDiagsArea();
			if (accumulatedDiagsArea)
			  {
			    accumulatedDiagsArea->mergeAfter(*da);
			    if (!(accumulatedDiagsArea->canAcceptMoreErrors()) && 
				tflowTdb().isNonFatalErrorTolerated()) 
			      {
				pstate.nonFatalErrorSeen_ = FALSE;
				pstate.step_ = HANDLE_ERROR_; 
			      }
			  }
                        else
			  {
			    pentry->setDiagsArea(da);
			    da->incrRefCount();
			    accumulatedDiagsArea = da ;
			    if (tflowTdb().isNonFatalErrorTolerated()) 
			      {
				ComDiagsArea *cliDiagsArea = pentry_down->getDiagsArea();
				da->setLengthLimit(cliDiagsArea->getLengthLimit());
			      }
			  }

			// For Non-Fatal errors we will remove this Q_SQLERROR reply from the 
			// left child right below as we will continue to stay in this state (MOVE_SRC_TO_TGT_).
			// For fatal errors this Q_SQLERROR reply is removed in HANDLE_ERROR step to which
			// we will transition immediately.
			if (pstate.nonFatalErrorSeen_ == TRUE)
			  qSrc_.up->removeHead();	
                    }
                    break;

		  case ex_queue::Q_REC_SKIPPED:
                    {
		      pstate.srcRequestCount_++;
		      ComDiagsArea * da = src_entry->getDiagsArea();
		      if (da)
			pstate.nonFatalErrorSeen_ = TRUE;		  
		      qSrc_.up->removeHead();
                    }
                    break;

		  default:
		    {
		      ex_assert(0, "ExTupleFlowTcb::work() Error returned from src");
		    }
		    break;
		  } // switch

	      } // while
	    
            // if the child reply is not an Q_SQLERROR, then process target
            if ((pstate.step_ != HANDLE_ERROR_) &&
		(pstate.step_ != MOVE_EOD_TO_TGT_))
	      pstate.step_ = PROCESS_TGT_;
	  } // MOVE_SRC_TO_TGT
	  break;

	case MOVE_EOD_TO_TGT_:
	  {
	    pstate.parentEOD_ = TRUE;

	    if (qTgt_.down->isFull())
	      return WORK_OK;

	    ex_queue_entry * tgt_entry = qTgt_.down->getTailEntry();
	    tgt_entry->downState.request = ex_queue::GET_EOD;
	    
	    tgt_entry->downState.requestValue = 
	      pentry_down->downState.requestValue;
	    tgt_entry->downState.parentIndex = 
	      qParent_.down->getHeadIndex();
	    //tgt_entry->passAtp(pentry_down);
	    
	    qTgt_.down->insert();

	    pstate.tgtRequests_++;

	    if (tflowTdb().sendEODtoTgt())
	      pstate.srcEOD_ = TRUE;
	    else
	      pstate.srcEOD_ = FALSE;

	    pstate.step_ = PROCESS_TGT_;
	  }
	break;

	case PROCESS_TGT_:
	  {
	    while (! qTgt_.up->isEmpty()  &&  pstate.step_ != HANDLE_ERROR_)
	      {
		ex_queue_entry * tgt_entry = qTgt_.up->getHeadEntry();
		switch (tgt_entry->upState.status)
		  {
		  case ex_queue::Q_OK_MMORE:
		    {
		      if (!tflowTdb().isNonFatalErrorTolerated()) 
			{
			  // ex_assert(0, "ExTupleFlowTcb::work() OK_MMORE from tgt");
			  if (qParent_.up->isFull())
			    return WORK_OK;

			  ex_queue_entry * pentry = qParent_.up->getTailEntry();

			  pentry->upState.status = ex_queue::Q_OK_MMORE;
			  pentry->upState.downIndex = qParent_.down->getHeadIndex();
			  pentry->upState.parentIndex = pentry_down->downState.parentIndex;
			  pentry->upState.setMatchNo(pstate.matchCount_);
			  
			  // copy input tupps from parent request
			  pentry->copyAtp(pentry_down);

			  // copy child's atp to
			  // the output atp (to parent's up queue)
			  pentry->copyAtp(tgt_entry);

			  // insert into parent up queue
			  qParent_.up->insert();	  
			}
		      else 
			{
			  ComDiagsArea * da = tgt_entry->getDiagsArea();
			  ex_assert(da, "We have a Q_OK_MMORE in Tupleflow but no diags area");
			  if (da->mainSQLCODE() != 0) {
			    // Non-atomic Rowsets sends OK_MMORE with non-empty diags from child
			    // empty diags (mainsqlcode == 0) implies OK_MMORE sent by ignoreDupKey code
			    // when NAR is on, for -8102 error.  Just consume the OK_MMORE.
			    
			    if(tflowTdb().isRowsetIterator()) 
			      {
				da->setAllRowNumber(Lng32 (tgt_entry->upState.parentIndex));
			      }
			    
			    pstate.nonFatalErrorSeen_ = TRUE;
			    ex_queue_entry * pentry = qParent_.up->getTailEntry();
			    ComDiagsArea *accumulatedDiagsArea = pentry->getDiagsArea();
			    if (accumulatedDiagsArea)
			      {
				accumulatedDiagsArea->mergeAfter(*da);
				if (!(accumulatedDiagsArea->canAcceptMoreErrors()) && 
				    tflowTdb().isNonFatalErrorTolerated()) 
				  {
				    pstate.nonFatalErrorSeen_ = FALSE;
				    pstate.step_ = HANDLE_ERROR_; 
				  }
			      }
			    else
			      {
				pentry->setDiagsArea(da);
				da->incrRefCount();
				if (tflowTdb().isNonFatalErrorTolerated()) {
				  ComDiagsArea *cliDiagsArea = pentry_down->getDiagsArea();
				  da->setLengthLimit(cliDiagsArea->getLengthLimit());
				}
			      }
			  }
			}

		      qTgt_.up->removeHead();
		    }
		  break;
		  
		  case ex_queue::Q_NO_DATA:
		    {
                      ComDiagsArea * da = tgt_entry->getDiagsArea();
                      if (da)
                        {
	                  ex_queue_entry * pentry = qParent_.up->getTailEntry();
                          ComDiagsArea *accumulatedDiagsArea = pentry->getDiagsArea();
                          if (accumulatedDiagsArea) 
                            accumulatedDiagsArea->mergeAfter(*da);
                          else
                            {
	                      pentry->setDiagsArea(da);
                              da->incrRefCount();
			      if (tflowTdb().isNonFatalErrorTolerated()) {
				ComDiagsArea *cliDiagsArea = pentry_down->getDiagsArea();
				da->setLengthLimit(cliDiagsArea->getLengthLimit());
			      }
                            }
                        }
		      pstate.matchCount_ += tgt_entry->upState.getMatchNo();
		      qTgt_.up->removeHead();
		      pstate.tgtRequests_--;
		      pstate.startRightIndex_++;
		    }
		    break;
		    
		  case ex_queue::Q_SQLERROR:
                    {
		      if (qParent_.up->isFull())
			return WORK_OK;
	    
		      ex_queue_entry * pentry = qParent_.up->getTailEntry();
		      pentry->copyAtp(tgt_entry);  
		      pstate.nonFatalErrorSeen_ = FALSE;
		      pstate.step_ = HANDLE_ERROR_; 

		      if(tflowTdb().isRowsetIterator()) 
		      {
			ex_queue_entry * pentry = qParent_.up->getTailEntry();
	                ComDiagsArea *da = pentry->getDiagsArea();
			ex_assert(da, "To set RowNumber, an error condition must be present in the diags area");
	                da->setAllRowNumber(Lng32 (tgt_entry->upState.parentIndex));		      
		      }
	    
                    }
                    break;
                             
		  default:
		    {
		      ex_assert(0, "ExTupleFlowTcb::work() Error returned from tgt");
		    }
		    break;
		    
		  } // switch
		
	      } // while 

            if (pstate.step_ == HANDLE_ERROR_)
              break;

	    // if source has returned EOD,
	    // and there are no pending requests in target's down
	    // queue, then we are done with this parent request.
	    if (((pstate.srcEOD_ == TRUE) ||
		 (pstate.parentEOD_ == TRUE)) &&
		(qTgt_.down->isEmpty()))
	      pstate.step_ = DONE_;
	    else
	      {
		if (NOT pstate.parentEOD_)
		  pstate.step_ = MOVE_SRC_TO_TGT_;

	        if (qSrc_.up->isEmpty() || qTgt_.down->isFull())
	 	  return WORK_OK;
	        else
		  return WORK_CALL_AGAIN;
	      }
	  }
	  break;

        case HANDLE_ERROR_:
          {
	     ex_queue_entry * pentry = qParent_.up->getTailEntry();

 	     pentry->upState.status = ex_queue::Q_SQLERROR;
	     pentry->upState.downIndex = qParent_.down->getHeadIndex();
	     pentry->upState.parentIndex = pentry_down->downState.parentIndex;
	     pentry->upState.setMatchNo(pstate.matchCount_);

	     ComDiagsArea *da = pentry->getDiagsArea();
	     if (tflowTdb().isNonFatalErrorTolerated() &&
		  !(da->canAcceptMoreErrors())) {
	      ComDiagsArea *cliDiagsArea = pentry_down->getDiagsArea();
	      da->removeLastErrorCondition();
	      *da << DgSqlCode(-EXE_NONATOMIC_FAILURE_LIMIT_EXCEEDED) 
			      << DgInt0(cliDiagsArea->getLengthLimit());
	     }
	    
	     // insert into parent up queue
	     qParent_.up->insert();
   
             pstate.step_ = CANCELLED_;
          }
          break;

        case CANCELLED_:
          {
            qSrc_.down->cancelRequestWithParentIndex(qParent_.down->getHeadIndex());

            // Cancel all the outstanding requests that have been sent to the target.
            // Cancel all requests within given range (inclusive)
            qTgt_.down->cancelRequestWithParentIndexRange((queue_index)pstate.startRightIndex_+1,
                                                          (queue_index)pstate.srcRequestCount_);
            pstate.startRightIndex_ = pstate.srcRequestCount_;

            //ignore all rows from source child, till Q_NO_DATA is reached
            while ((pstate.srcEOD_ != TRUE) && (!qSrc_.up->isEmpty()))
              {
	        ex_queue_entry * src_entry = qSrc_.up->getHeadEntry();
                switch(src_entry->upState.status)
                  {
                    case ex_queue::Q_OK_MMORE:
                    case ex_queue::Q_SQLERROR:
		    case ex_queue::Q_REC_SKIPPED:
                      {
                        qSrc_.up->removeHead();
                      }
                      break;

                    case ex_queue::Q_NO_DATA:
                      {
                        pstate.srcEOD_ = TRUE;
                        qSrc_.up->removeHead();
                      }
                      break;
                        
                    default:
                      {
		        ex_assert(0, "ExTupleFlowTcb::work() Error returned from src");
		      }
		      break;
                  }
              }

            //ignore all rows from target child, till Q_NO_DATA is reached
            while (pstate.tgtRequests_ && !qTgt_.up->isEmpty())
              {
	        ex_queue_entry * tgt_entry = qTgt_.up->getHeadEntry();
                switch(tgt_entry->upState.status)
                  {
                    case ex_queue::Q_OK_MMORE:
                    case ex_queue::Q_SQLERROR:
                      {
                        qTgt_.up->removeHead();
                      }
                      break;

                    case ex_queue::Q_NO_DATA:
                      {
                        qTgt_.up->removeHead();
                        pstate.tgtRequests_--;
                      }
                      break;
                    
                    default:
                      {
                        ex_assert(0, "ExTupleFlowTcb::work() Error returned from tgt");
                      }
		      break;
                  }
              }

            // if both source and target returned all the rows,
            // insert Q_SQLERROR into the parent up queue
            if ((pstate.srcEOD_ == TRUE)  &&  !pstate.tgtRequests_)
              {
	        pstate.step_ = DONE_; 
              }
            else
              return WORK_OK;
          }
          break;
	  
	case DONE_:
	  {
	    if (qParent_.up->isFull())
	      return WORK_OK;
	    
	    ex_queue_entry * pentry = qParent_.up->getTailEntry();

	    if (pstate.nonFatalErrorSeen_) {
	      ComDiagsArea *da = pentry->getDiagsArea();
	      ComDiagsArea *cliDiagsArea = pentry_down->getDiagsArea();
	      ex_assert((da || cliDiagsArea), "We have non-fatal errors in Tupleflow but no diags area");
	      if (cliDiagsArea) {
		  if (da)
		    da->mergeAfter(*cliDiagsArea);
		  else
		    {
		      pentry->setDiagsArea(cliDiagsArea);
		      cliDiagsArea->incrRefCount();
		    }
	      } 

	      if (cliDiagsArea->canAcceptMoreErrors()) {
	      	  ComDiagsArea *mergedDiagsArea = pentry->getDiagsArea();
		  // used to make mainSQLCODE() return 30022 or 30035.
		  mergedDiagsArea->setNonFatalErrorSeen(TRUE);
		  NABoolean anyRowsAffected = FALSE;


		  // This tupleflow should be in the master for
		  // non-atomic rowsets.
		  ExMasterStmtGlobals *g = getGlobals()->
			castToExExeStmtGlobals()->castToExMasterStmtGlobals();
		  ex_assert(g, "Rowset insert has a flow node that is not in the master executor");
		  if (g->getRowsAffected() > 0)
		    anyRowsAffected = TRUE;
		  
		  if (anyRowsAffected)
		      *mergedDiagsArea << DgSqlCode(EXE_NONFATAL_ERROR_SEEN);
		  else 
		      *mergedDiagsArea << DgSqlCode(EXE_NONFATAL_ERROR_ON_ALL_ROWS);

	      } // we exceeded the Nonfatal error limit when merging with the CLI diags area
	      else {
		pstate.step_ = HANDLE_ERROR_;
		// will prevent us from merging the diags areas again
		pstate.nonFatalErrorSeen_ = FALSE ;
		break ;
	      }
	    }
	    
	    pentry->upState.status = ex_queue::Q_NO_DATA;
	    pentry->upState.downIndex = qParent_.down->getHeadIndex();
	    pentry->upState.parentIndex = pentry_down->downState.parentIndex;
	    pentry->upState.setMatchNo(pstate.matchCount_);

            // BEGIN:  Read note at beginning of work().
            //
            if(pstate.noOfUnPackedRows_ != 0)
            {
	      ComDiagsArea *da = pentry->getDiagsArea();
	      if (da == NULL)
		{
		  da = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
		  pentry->setDiagsArea(da);
		}
	      da->addRowCount(pstate.noOfUnPackedRows_);
              pstate.noOfUnPackedRows_ = 0;
            }
            //
            // END: - Read note at beginning of work().

	    // if stats are to be collected, collect them.
	    if (getStatsEntry())
	      {
		// nothing yet returned from right child or returned
		// to parent.
		getStatsEntry()->setActualRowsReturned(0);
	      }
	    
	    // insert into parent up queue
	    qParent_.up->insert();
	    
	    pstate.step_ = EMPTY_;
	    qParent_.down->removeHead();	

	    return WORK_CALL_AGAIN;  // check for more down requests

	  }
	  break;

	} // switch pstate.step_
    } // while
  
  return 0;
}


/////////////////////////////////////////////////////////////////////////
//
//  Private state procedures
//
/////////////////////////////////////////////////////////////////////////

ExTupleFlowPrivateState::ExTupleFlowPrivateState(const ExTupleFlowTcb * /*tcb*/) 
{
  init();
}

void ExTupleFlowPrivateState::init() 
{
  matchCount_      = 0;
  tgtRequests_      = 0;
  tgtRowsSent_      = FALSE;
  srcEOD_           = FALSE;
  parentEOD_        = FALSE;
  noOfUnPackedRows_ = 0;
  step_ = ExTupleFlowTcb::EMPTY_;
  srcRequestCount_ = -1;
  nonFatalErrorSeen_ = FALSE;
  // parent index for target Q entries are eqaul to value of srcRequestCount_.
  // therefore startRightIndex_ is initialized to the same value as srcRequestCount_
  startRightIndex_ = -1;
}

ExTupleFlowPrivateState::~ExTupleFlowPrivateState()
{
}

ex_tcb_private_state * ExTupleFlowPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) ExTupleFlowPrivateState((ExTupleFlowTcb *) tcb);
}















