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
 * File:         ExFirstN.cpp
 * Description:  class to return N rows.
 *               
 *               
 * Created:      5/2/2003
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

#include "ExFirstN.h"

//
// Build a firstN tcb
//
ex_tcb * ExFirstNTdb::build(ex_globals * glob)
{
  // first build the child
  ex_tcb * child_tcb = tdbChild_->build(glob);
  
  ExFirstNTcb * firstn_tcb = 
    new(glob->getSpace()) ExFirstNTcb(*this, *child_tcb, glob);
  
  // add this tcb to the schedule
  firstn_tcb->registerSubtasks();

  return (firstn_tcb);
}

//
// Constructor for firstn_tcb
//
ExFirstNTcb::ExFirstNTcb(const ExFirstNTdb & firstn_tdb, 
			 const ex_tcb & child_tcb,    // child queue pair
			 ex_globals *glob
			 ) : ex_tcb( firstn_tdb, 1, glob),
                             step_(INITIAL_)
{
  childTcb_ = &child_tcb;
  
  Space * space = glob->getSpace();
  CollHeap * heap = (glob ? glob->getDefaultHeap() : NULL);
  
  // Allocate the buffer pool
  pool_ = new(space) sql_buffer_pool(firstn_tdb.numBuffers_,
				     firstn_tdb.bufferSize_,
				     space);
  
  // get the queue that child use to communicate with me
  qchild_  = child_tcb.getParentQueue(); 
  
  // Allocate the queue to communicate with parent
  qparent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
				      firstn_tdb.queueSizeDown_,
				      firstn_tdb.criDescDown_,
				      space);
  
  // Allocate the private state in each entry of the down queue
  ExFirstNPrivateState p(this);
  qparent_.down->allocatePstate(&p, this);

  qparent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
				    firstn_tdb.queueSizeUp_,
				    firstn_tdb.criDescUp_,
				    space);

  workAtp_ = NULL;
  firstNParamVal_ = 0;
  effectiveFirstN_ = -1;
  returnedSoFar_ = 0;
  if (firstn_tdb.workCriDesc_)
    {
      workAtp_ = allocateAtp(firstn_tdb.workCriDesc_, space);
      pool_->get_free_tuple(workAtp_->getTupp(firstn_tdb.workCriDesc_->noTuples()-1), 0);
      workAtp_->getTupp(firstn_tdb.workCriDesc_->noTuples()-1).
        setDataPointer((char*)&firstNParamVal_);
    }

  if (firstn_tdb.firstNRowsExpr_)
    {
      firstn_tdb.firstNRowsExpr_->fixup(0, getExpressionMode(), this,  space, heap, 
                                        FALSE, glob);
     }
};

ExFirstNTcb::~ExFirstNTcb()
{
  freeResources();
};
  
////////////////////////////////////////////////////////////////////////
// Free Resources
//
void ExFirstNTcb::freeResources()
{
  delete qparent_.up;
  delete qparent_.down;
};

////////////////////////////////////////////////////////////////////////
// Register subtasks 
//
void ExFirstNTcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();
  ex_queue_pair pQueue = getParentQueue();

  // register events for parent queue
  ex_assert(pQueue.down && pQueue.up,"Parent down queue must exist");
  sched->registerInsertSubtask(ex_tcb::sWork, this, pQueue.down);
  sched->registerCancelSubtask(sCancel, this, pQueue.down);
  sched->registerUnblockSubtask(ex_tcb::sWork,this, pQueue.up);

  // register events for child queues
  const ex_queue_pair cQueue = getChild(0)->getParentQueue();

  sched->registerUnblockSubtask(ex_tcb::sWork,this, cQueue.down);
  sched->registerInsertSubtask(ex_tcb::sWork, this, cQueue.up);

}

short ExFirstNTcb::moveChildDataToParent()
{
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();

  ex_queue_entry * cUpEntry = qchild_.up->getHeadEntry();
  ex_queue_entry * pUpEntry = qparent_.up->getTailEntry();

  pUpEntry->copyAtp(cUpEntry);
  pUpEntry->upState.status      = cUpEntry->upState.status;
  pUpEntry->upState.downIndex   = qparent_.down->getHeadIndex();
  pUpEntry->upState.parentIndex = pentry_down->downState.parentIndex;
  pUpEntry->upState.setMatchNo(cUpEntry->upState.getMatchNo());
  
  // insert into parent up queue
  qparent_.up->insert();
  
  qchild_.up->removeHead();

  return 0;
}

////////////////////////////////////////////////////////////////////////////
// This is where the action is.
////////////////////////////////////////////////////////////////////////////
short ExFirstNTcb::work()
{
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExFirstNPrivateState *pstate = (ExFirstNPrivateState*) pentry_down->pstate;
  const ex_queue::down_request & request = pentry_down->downState.request;

  while (1) // exit via return
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    if (qchild_.down->isFull())
	      return WORK_OK;

	    ex_queue_entry * centry = qchild_.down->getTailEntry();

	    // effectiveFirstN_ is set to a positive number
	    // if FIRST N rows are requested.
	    // It is set to 0 or a negative number, if last N rows are needed.
	    // 0 means process all but don't return any rows.
	    // -1 means get all rows. Should not reach this state.
	    // <-1 means return the last '-(N+2)' rows.
            //TRAFODION-2930
            //fix [first 600000000000] crash
	    //effectiveFirstN_ = firstnTdb().firstNRows();
            Int64 nFirstN = firstnTdb().firstNRows();
            Int64 nMaxInt = INT_MAX;
            effectiveFirstN_ = Lng32( min( nMaxInt,nFirstN ) );
            //TRAFODION-2930

            returnedSoFar_ = 0;

            if (firstnTdb().firstNRowsExpr_)
              {
                ex_expr::exp_return_type evalRetCode =
                  firstnTdb().firstNRowsExpr_->eval(pentry_down->getAtp(), workAtp_);
                if (evalRetCode == ex_expr::EXPR_ERROR)
                  {
                    step_ = CANCEL_;
                    
                    break;
                  }

                effectiveFirstN_ = firstNParamVal_;
              }
            
            if (effectiveFirstN_ >= 0)
              {
		centry->downState.request = ex_queue::GET_N;

		// if I got a GET_ALL request then send a GET_N request to
		// my child with the N value being effectiveFirstN_.
		// if I got a GET_N request, then send the minimum of the
		// GET_N request value and effectiveFirstN_ to my child.
		if ((pentry_down->downState.request != ex_queue::GET_N) ||
		    (pentry_down->downState.requestValue == effectiveFirstN_))
		  centry->downState.requestValue = effectiveFirstN_;
		else
		  {
		    centry->downState.requestValue = 
		      MINOF(pentry_down->downState.requestValue, effectiveFirstN_);
		  }

		step_ = PROCESS_FIRSTN_;
	      }
            else
	      {
		// last N processing, retrieve all rows.
		centry->downState.request = ex_queue::GET_ALL;
		centry->downState.requestValue = 11;

                requestedLastNRows_ = -(effectiveFirstN_ + 2);
                returnedLastNRows_ = 0;

		step_ = PROCESS_LASTN_;
	      }

	    centry->downState.parentIndex = qparent_.down->getHeadIndex();

	    centry->passAtp(pentry_down);
	    
	    qchild_.down->insert();
	  }
	  break;

	case PROCESS_FIRSTN_:
	  {
	    if ((qchild_.up->isEmpty()) ||
		(qparent_.up->isFull()))
	      return WORK_OK;

	    ex_queue_entry * cUpEntry = qchild_.up->getHeadEntry();
	    ex_queue_entry * pUpEntry = qparent_.up->getTailEntry();
	    switch (cUpEntry->upState.status)
	      {
	      case ex_queue::Q_OK_MMORE:
		{
                  if (returnedSoFar_ < effectiveFirstN_)
                    {
                      moveChildDataToParent();
                      returnedSoFar_++;
                    }
                  else
                    {
                      // looks like the child may not honor our
                      // GET_N request, so send a cancel, maybe
                      // that will work better
                      qchild_.down->cancelRequest();
                      step_ = CANCEL_;
                    }
		}
		break;

	      case ex_queue::Q_NO_DATA:
		{
		  moveChildDataToParent();

		  qparent_.down->removeHead();
		  
		  step_ = DONE_;
		}
		break;

	      case ex_queue::Q_SQLERROR:
		{
		  qchild_.down->cancelRequest();
		  moveChildDataToParent();
		  step_ = CANCEL_;
		}
		break;

	      case ex_queue::Q_INVALID:
		{
		  ex_assert(0, "ExFirstNTcb::work() Invalid state return by child.");
		}
		break;


	      } // switch cUpEntry status

	  }
	  break;

	case PROCESS_LASTN_:
	  {
	    if ((qchild_.up->isEmpty()) ||
		(qparent_.up->isFull()))
	      return WORK_OK;

	    ex_queue_entry * cUpEntry = qchild_.up->getHeadEntry();
	    ex_queue_entry * pUpEntry = qparent_.up->getTailEntry();
	    switch (cUpEntry->upState.status)
	      {
	      case ex_queue::Q_OK_MMORE:
		{
		  if (requestedLastNRows_ == 0) // last 0
		    {
		      // ignore any upcoming rows.
		      qchild_.up->removeHead();
		    }
		  else if (requestedLastNRows_ == 1) // last 1
		    {
		      // We know that current entry is Q_OK_MMORE.
		      // Need atleast 1 more entry than requested to process
		      // last N. Note that there is a small chance that this
                      // will lead to a buffer deadlock (child's buffer pool
                      // is full, child expects us to consume a row before it
                      // can produce another one).
		      if (qchild_.up->getLength() < 1 + 1)
			return WORK_OK;

		      queue_index headIndex = qchild_.up->getHeadIndex();
		      
		      ex_queue_entry * nextCupEntry = 
			qchild_.up->getQueueEntry(headIndex + 1);
		      switch (nextCupEntry->upState.status)
			{
			case ex_queue::Q_NO_DATA:
			  {
			    // if the next entry is Q_NO_DATA, return the
			    // current child head entry. Its the last row.

			    // This call will also remove the child head entry.
			    moveChildDataToParent();
			  }
			break;

			case ex_queue::Q_SQLERROR:
			case ex_queue::Q_OK_MMORE:
			  {
			    // just remove the current head entry. We don't
			    // need it.
			    qchild_.up->removeHead();
			  } 
			break;

			case ex_queue::Q_INVALID:
			  {
			    ex_assert(0, "ExFirstNTcb::work() Invalid state return by child.");
			  }
			break;
			
			} // switch
		    }
		  else
		    {
		      // not supported.
		      ex_assert(0, "ExFirstNTcb::work(): only last 0 and last 1 supported.");

		    }
		}
		break;

	      case ex_queue::Q_SQLERROR:
		{
		  qchild_.down->cancelRequest();
		  moveChildDataToParent();
		  step_ = CANCEL_;
		}
		break;

	      case ex_queue::Q_NO_DATA:
		{

		  moveChildDataToParent();

		  if (cUpEntry->upState.status == ex_queue::Q_NO_DATA)
		    {
		      qparent_.down->removeHead();

		      step_ = DONE_;
		    }
		}
		break;

	      case ex_queue::Q_INVALID:
		{
		  ex_assert(0, "ExFirstNTcb::work() Invalid state return by child.");
		}
		break;


	      } // switch cUpEntry status

	  }
	  break;

	case DONE_:
	  {
	    step_ = INITIAL_;

	    return WORK_CALL_AGAIN;
	  }
	  break;

	case CANCEL_:
	  {
            // ignore all up rows from child. wait for Q_NO_DATA.
	    if (qchild_.up->isEmpty())
	      return WORK_OK;

	    ex_queue_entry * cUpEntry = qchild_.up->getHeadEntry();
	    ex_queue_entry * pUpEntry = qparent_.up->getTailEntry();

	    switch(cUpEntry->upState.status)
	      {
	      case ex_queue::Q_OK_MMORE:
	      case ex_queue::Q_SQLERROR:
		{
		  qchild_.up->removeHead();
		}
	      break;
	      
	      case ex_queue::Q_NO_DATA:
		{
		  moveChildDataToParent();
		  qparent_.down->removeHead();
		  step_ = DONE_;
		}
	      break;
	      
	      case ex_queue::Q_INVALID: 
		{
		  ex_assert(0, "ExFirstNTcb::work() Invalid state returned by child");
		}; break;
	      }
	  }
	break;
	
	case ERROR_:
	  {
	  }
	  break;

	} // switch
    } // while

  return 0;
}

short ExFirstNTcb::cancel()
{
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExFirstNPrivateState *pstate = (ExFirstNPrivateState*) pentry_down->pstate;

  if (pentry_down->downState.request == ex_queue::GET_NOMORE)
    {
      step_ = CANCEL_;
      qchild_.down->cancelRequest();
    }
  
  return WORK_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for sort_private_state
///////////////////////////////////////////////////////////////////////////////
ExFirstNPrivateState::ExFirstNPrivateState(const ExFirstNTcb *  tcb)
{
}
ExFirstNPrivateState::~ExFirstNPrivateState()
{
};

ex_tcb_private_state * ExFirstNPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) ExFirstNPrivateState((ExFirstNTcb *) tcb);
};

