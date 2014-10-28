/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExIar.cpp 
 * Description:  class to support INTERPRET_AS_ROW function.
 *               
 *               
 * Created:      6/8/2005
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include  "ex_stdh.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include "ExIar.h"

//
// Build an ExIar tcb
//
ex_tcb * ExIarTdb::build(ex_globals * glob)
{
  ExIarTcb * iarTcb = new(glob->getSpace()) ExIarTcb(*this, glob);

  // add the IarTcb to the schedule
  iarTcb->registerSubtasks();

  return (iarTcb);
}

//
// Constructor for ExIar tcb 
//
ExIarTcb::ExIarTcb (const ExIarTdb& iarTdb, 
		    ex_globals *glob
		   ) : ex_tcb( iarTdb, 1, glob)
{
   Space * space = (glob ? glob->getSpace() : 0);
   CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);

   // Allocate the buffer pool
#pragma nowarn(1506)   // warning elimination
   pool_ = new(space) sql_buffer_pool(iarTdb.numBuffers_,
                                      iarTdb.bufferSize_,
                                      space);
#pragma warn(1506)  // warning elimination

   // Allocate the queue to communicate with parent.
   qparent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
                                       iarTdb.queueSizeDown_,
                                       iarTdb.criDescDown_,
                                       space
                                      );

   // Allocate the private state in each entry of the down queue
   ExIarPrivateState *p = new(space) ExIarPrivateState(this);
   qparent_.down->allocatePstate(p, this);
   delete p;

   qparent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
                                     iarTdb.queueSizeUp_,
                                     iarTdb.criDescUp_,
                                     space);

   // Fixup the column extraction expression, if any.
   if (iarTdb.getExtractExpr())
      iarTdb.getExtractExpr()->fixup(0, getExpressionMode(), this, space, heap, FALSE, glob);

   // Fixup the scan expression, if any.
   if (iarTdb.getScanExpr())
      iarTdb.getScanExpr()->fixup(0, getExpressionMode(), this, space, heap, FALSE, glob);

   // Fixup the projection expression.
   if (iarTdb.getProjExpr())
      iarTdb.getProjExpr()->fixup(0, getExpressionMode(), this, space, heap, FALSE, glob);

   // Allocate the work ATP.
   workAtp_ = allocateAtp(iarTdb.workCriDesc_, space);
};

ExIarTcb::~ExIarTcb()
{
   delete qparent_.up;
   delete qparent_.down;

   freeResources();
};
  
void ExIarTcb::freeResources()
{
   delete pool_;
   pool_ = 0;
};

void ExIarTcb::registerSubtasks()
{
   ex_tcb::registerSubtasks();
}

short ExIarTcb::work()
{
   // The work method does the actual extraction, selection, and projection
   // work. There are 4 possible states:
   // IAR_NOT_STARTED: This is the initial state before starting the IAR
   //                  operation. This is the state that gets set when the
   //                  TCB is built and parent queues allocated.
   // IAR_RETURNING_ROWS: In this state, the operator performs the actual
   //                  column extraction from the audit row image, and 
   //                  projects relevant columns.
   // IAR_DONE: In this state, Q_NO_DATA is returned in the upqueue to the
   //                  parent and state is reinitialized to IAR_NOT_STARTED.
   // IAR_ERROR: This is the state that is reached if an error occurs in any
   //            other state. The state is changed in this case to IAR_DONE.
  
   ex_queue_entry * pentry_down;
   ExIarPrivateState * pstate;
   ex_queue::down_request request;

   while (1) {
      if (qparent_.down->isEmpty())
         return WORK_OK;

      if (qparent_.up->isFull())
         return WORK_OK;

      pentry_down = qparent_.down->getHeadEntry();
      pstate = (ExIarPrivateState *)pentry_down->pstate;
      request = pentry_down->downState.request;

      switch (pstate->step_)
      {
         case IAR_NOT_STARTED:
         {
           if (request == ex_queue::GET_NOMORE) 
              pstate->step_ = IAR_DONE;
           else
              pstate->step_ = IAR_RETURNING_ROWS;
         }
         break;

         case IAR_RETURNING_ROWS:
         {
            if (qparent_.down->isEmpty())
               return WORK_OK; // no more requests - just return.

            if (qparent_.up->isFull())
               return WORK_OK; // parent queue is full. Just return.

            ex_queue_entry * up_entry = qparent_.up->getTailEntry();

            // Copy the input atp into the up queue atp.
            up_entry->copyAtp(pentry_down);

	    tupp returnedRow;
		
            // Allocate space to hold the projected output row to be 
            // returned.
#pragma nowarn(1506)
	    if (pool_->get_free_tuple(returnedRow, iarTdb().outputRowLen_))
#pragma warn(1506)
	      return WORK_POOL_BLOCKED; // couldn't allocate, try again later.
	    
	    up_entry->getTupp(iarTdb().criDescUp_->noTuples()-1) = returnedRow;
	    
	    tupp extractedRow;
	    
            // Allocate space to hold the extracted row.
#pragma nowarn(1506)
	    if (pool_->get_free_tuple(extractedRow, iarTdb().extractedRowLen_))
#pragma warn(1506)
	      return WORK_POOL_BLOCKED; // couldn't allocate, try again later.
	    
	    workAtp_->getTupp(2) = extractedRow;
	    
            ex_expr::exp_return_type retCode;
            if (iarTdb().getExtractExpr())
	      {
               retCode = iarTdb().getExtractExpr()->eval(pentry_down->getAtp(), 
                                                         workAtp_);
               if (retCode == ex_expr::EXPR_ERROR)
               {
                  // Set the diagnostics area in the up queue entry.
                  ComDiagsArea *upEntryDiags = up_entry->getDiagsArea();
                  ComDiagsArea *da = pentry_down->getAtp()->getDiagsArea();
                  if (!upEntryDiags)
                  {
                     upEntryDiags = 
                         ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
                     up_entry->setDiagsArea(upEntryDiags);
                  }
                  upEntryDiags->mergeAfter(*da);
                  pstate->step_ = IAR_ERROR;
                  break;
               }
            }

            // Evaluate the selection predicate, if any. 
            if (iarTdb().getScanExpr())
            {
               retCode = iarTdb().getScanExpr()->eval(pentry_down->getAtp(),
                                                      workAtp_);
               if (retCode == ex_expr::EXPR_FALSE)
               {
                  pstate->step_ = IAR_DONE;
                  break;
               }
               else
               {
                  if (retCode == ex_expr::EXPR_ERROR)
                  {
                     // Set the diagnostics area in the up queue entry.
                     ComDiagsArea *upEntryDiags = up_entry->getDiagsArea();
                     ComDiagsArea *da = pentry_down->getAtp()->getDiagsArea();
                     if (!upEntryDiags)
                     {
                        upEntryDiags =
                          ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
                        up_entry->setDiagsArea(upEntryDiags);
                     }
                     upEntryDiags->mergeAfter(*da);
                     pstate->step_ = IAR_ERROR;
                     break;
                  }
               }
            }

            // Evaluate the projection expression, if any.
            if (iarTdb().getProjExpr())
            { 
               retCode = iarTdb().getProjExpr()->eval(up_entry->getAtp(),
                                                      workAtp_);
               if (retCode == ex_expr::EXPR_ERROR)
               {
                  pstate->step_ = IAR_ERROR;
                  break;
               }
            }

            up_entry->upState.status = ex_queue::Q_OK_MMORE;
            up_entry->upState.parentIndex = 
                                         pentry_down->downState.parentIndex;
            up_entry->upState.downIndex = qparent_.down->getHeadIndex();
            pstate->matchCount_++;
            up_entry->upState.setMatchNo(pstate->matchCount_);
            qparent_.up->insert();
            pstate->step_ = IAR_DONE;
         }
         break;

         case IAR_DONE:
         {
            if (qparent_.up->isFull())
               return WORK_OK; // parent queue is full. Just return.

            ex_queue_entry * up_entry = qparent_.up->getTailEntry();
            up_entry->upState.parentIndex = pentry_down->downState.parentIndex;
            up_entry->upState.setMatchNo(pstate->matchCount_);
            up_entry->upState.status = ex_queue::Q_NO_DATA;
            qparent_.up->insert();

	    workAtp_->release();
	    
            qparent_.down->removeHead();
	    
            pstate->step_ = IAR_NOT_STARTED;
         }
         break;

         case IAR_ERROR:
         {
            if (qparent_.up->isFull())
               return WORK_OK;

            ex_queue_entry * up_entry = qparent_.up->getTailEntry();
            up_entry->upState.parentIndex = pentry_down->downState.parentIndex;
            up_entry->upState.setMatchNo(pstate->matchCount_);
            up_entry->upState.status = ex_queue::Q_SQLERROR;

            // insert into parent
            qparent_.up->insert();
            pstate->step_ = IAR_DONE;
         }
         break;
 
         default:
            ex_assert(FALSE, "Invalid step in TCB down queue");
            break;
      } 
   }

#pragma nowarn(203)
   return WORK_OK;
#pragma warn(203)
};

short ExIarTcb::cancel()
{
   return WORK_OK;
};

ExIarPrivateState::ExIarPrivateState(const ExIarTcb* tcb)
{
   init(tcb);
}

ExIarPrivateState::~ExIarPrivateState()
{
};

ex_tcb_private_state * ExIarPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) ExIarPrivateState((ExIarTcb *) tcb);
};

void ExIarPrivateState::init(const ExIarTcb *tcb)
{
   matchCount_ = 0;
   step_ = ExIarTcb::IAR_NOT_STARTED;
}
