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
******************************************************************************
*
* File:         ExCompoundStmt.cpp 
* Description:  3GL compound statement (CS) operator.
*	
* Created:      4/1/98 
* Language:     C++
*
*
*
******************************************************************************
*/

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_expr.h"
#include "ex_error.h"
#include "str.h"
#include "ExCompoundStmt.h"
#include "ExStats.h"


//////////////////////////////////////////////////////////////////////////////
// CatpoundStmt TDB methods.
//////////////////////////////////////////////////////////////////////////////
ex_tcb * ExCatpoundStmtTdb::build(ex_globals *glob)
{
  ex_tcb *left = tdbLeft_->build(glob);
  ex_tcb *right = tdbRight_->build(glob);

  ExCatpoundStmtTcb *cs = 
    new(glob->getSpace()) ExCatpoundStmtTcb(*this, *left, *right, glob);
  
  cs->registerSubtasks();
  
  return cs;
} // ExCatpoundStmtTdb::build


//////////////////////////////////////////////////////////////////////////////
// CatpoundStmt TCB methods.
//////////////////////////////////////////////////////////////////////////////
ExCatpoundStmtTcb::ExCatpoundStmtTcb(const ExCatpoundStmtTdb &tdb, 
                                     const ex_tcb &left,   
                                     const ex_tcb &right, 
                                     ex_globals *glob) : 
  ex_tcb(tdb, 1, glob)
{
  CollHeap *space = glob->getSpace();

  // Init children TCB handles.
  tcbLeft_ = &left;
  tcbRight_ = &right;

  // Init queue pairs used to communicate with children.
  qleft_ = left.getParentQueue();
  qright_ = right.getParentQueue();

  // flowParent2Right's rentry->copyAtp(lentry) requires right down-queue
  // allocate its ATPs. Otherwise, rentry->copyAtp() will crash because
  // of a null qright_.down->queue.atp_.
  qright_.down->allocateAtps(glob->getSpace());

  ex_cri_desc *criDown = tdb.criDescDown_;  
  ex_cri_desc *criUp = tdb.criDescUp_;

  // Allocate queue pair used to communicate with parent.
  qparent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
                                      tdb.queueSizeDown_,
                                      criDown,
                                      space);
  qparent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
                                    tdb.queueSizeUp_,
                                    criUp,
                                    space);

  // Allocate private state in each entry of the down queue.
  ExCatpoundStmtPrivateState *p = new(space) ExCatpoundStmtPrivateState(this);
  qparent_.down->allocatePstate(p, this);
  delete p;

  // remember parent's down-queue index
  parent2leftx_ = qparent_.down->getHeadIndex();

  // remember left child's up-queue index
  leftupx_ = qleft_.up->getHeadIndex();
} // ExCatpoundStmtTcb::ExCatpoundStmtTcb


ExCatpoundStmtTcb::~ExCatpoundStmtTcb()
{
  delete qparent_.up;
  delete qparent_.down;
  freeResources();
} // ExCatpoundStmtTcb::~ExCatpoundStmtTcb


void ExCatpoundStmtTcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  // schedule sWorkDownLeft only if parent down-queue is non-empty
  // or if left child down-queue is non-full.
  sched->registerInsertSubtask(sWorkDownLeft, this, qparent_.down,"P1");
  sched->registerUnblockSubtask(sWorkDownLeft, this, qleft_.down);

  // schedule sWorkLeft2Right only if left child up-queue is non-empty
  // or if right child down-queue or parent up-queue is non-full
  sched->registerInsertSubtask(sWorkLeft2Right, this, qleft_.up,"P2");
  sched->registerUnblockSubtask(sWorkLeft2Right, this, qright_.down);
  sched->registerUnblockSubtask(sWorkLeft2Right, this, qparent_.up);
  
  // schedule sWorkUp only if right child up-queue is non-empty
  // or if parent up-queue is non-full.
  sched->registerInsertSubtask(sWorkUp, this, qright_.up,"P3");
  sched->registerUnblockSubtask(sWorkUp, this, qparent_.up);

  // schedule sWorkCancel if a cancel request is received 
  sched->registerCancelSubtask(sWorkCancel, this, qparent_.down,"CN");
} // ExCatpoundStmtTcb::registerSubtasks


inline const ex_tcb* ExCatpoundStmtTcb::getChild(Int32 pos) const
{
  ex_assert((pos >= 0), "ExCatpoundStmtTcb::getChild"); 
  if (pos == 0)
    return tcbLeft_;
  else if (pos == 1)
    return tcbRight_;
  else
    return NULL;
} // ExCatpoundStmtTcb::getChild


ExWorkProcRetcode ExCatpoundStmtTcb::work()
{
  // The work methods for the CS operator:
  // (1) workDownLeft() - Process new request, giving it to the left child.
  // (2) workLeft2Right() - Flow tuple value and control from left to right.
  // (3) workUp() - Flow result from right up-queue to parent up-queue.

  // This default work method is never called.
  ex_assert(0, "ExCatpoundStmtTcb::work: Invalid state");
  return WORK_OK;
} // ExCatpoundStmtTcb::work


// Work procedure to send requests down to left child
ExWorkProcRetcode ExCatpoundStmtTcb::workDownLeft()
{
  // while we have unprocessed down requests and while left
  // child's down queue has room, start more child requests
  while (qparent_.down->entryExists(parent2leftx_) && 
         !qleft_.down->isFull()) {
    startLeftChild();
    parent2leftx_++;
  }
  return WORK_OK;
} // ExCatpoundStmtTcb::workDownLeft


void ExCatpoundStmtTcb::startLeftChild()
{
  // Process the new parent request.
  ex_queue_entry *pentry = qparent_.down->getQueueEntry(parent2leftx_);
  ex_queue_entry *lentry = qleft_.down->getTailEntry();

  ExCatpoundStmtPrivateState &pstate = 
    *((ExCatpoundStmtPrivateState*)pentry->pstate);

  // Init private state.
  pstate.init();

  // pass same request to left child
  const ex_queue::down_request request = pentry->downState.request;
  lentry->downState.request = request;
  lentry->downState.requestValue = pentry->downState.requestValue;
  lentry->downState.parentIndex = parent2leftx_; 
  lentry->passAtp(pentry);

  qleft_.down->insert();
  pstate.leftstate_ = CS_STARTED;

  if (request == ex_queue::GET_NOMORE) {
    // immediately cancel the request (requests are already in
    // cancelled state but the cancel callback isn't activated yet)
    qleft_.down->cancelRequestWithParentIndex(parent2leftx_);
    pstate.leftstate_ = CS_CANCELLED;
  }
} // ExCatpoundStmtTcb::startLeftChild


// flow tuple values and control from left to right child
ExWorkProcRetcode ExCatpoundStmtTcb::workLeft2Right()
{
  // We want to remove left up-queue entries as soon as they're passed to
  // the right child. But, we can't. We must defer removing some (ie, the 
  // 1st and its Q_NO_DATA) left up-queue entries to workUp because 
  // catpoundstmts like
  //   begin; select count(*) from t; delete from t; end;
  // may have a right child that produce no data result, ie, its first
  // up-queue entry is a Q_NO_DATA. Q_NO_DATA does not carry any data. 
  // We must flow any left data up to the parent even if the right child
  // is a non-data-producing statement.

  // while left child's up-queue is non-empty do
  for (; qleft_.up->entryExists(leftupx_); leftupx_++) {
    // process reply from left child
    ex_queue_entry *lentry = qleft_.up->getQueueEntry(leftupx_);
    ex_queue_entry *rentry = qright_.down->getTailEntry();
    ex_queue_entry *pentry = qparent_.down->getQueueEntry
      (lentry->upState.parentIndex);

    ExCatpoundStmtPrivateState &pstate = 
      *((ExCatpoundStmtPrivateState*)pentry->pstate);

    switch(lentry->upState.status) {
    case ex_queue::Q_NO_DATA:
      // if we did not get atleast one row from the left child and we
      // are expecting rows (because of SELECT) from the left child
      // then raise a warning and stop further execution of CS statements
      if (pstate.leftstate_ == CS_STARTED &&
          expectingLeftRows() ) {
        if (qparent_.up->isFull()) return WORK_OK; // try again later
        pstate.leftstate_ = CS_ERROR;
        if (afterUpdate()) {
          processEODErrorOrWarning(FALSE);
        }
        else {
          processEODErrorOrWarning(TRUE);
        }
        if (qparent_.up->isFull()) return WORK_OK; 
      }

      if (pstate.leftstate_ == CS_ERROR || 
          pstate.leftstate_ == CS_CANCELLED) { 
        if (qparent_.up->isFull())return WORK_OK; // try again later
        passChildReplyUp(lentry); // pass EOD up
        if (pstate.leftrows_ > 0) { 
          // some data is parked in up-queue; flush it.
          qleft_.up->removeHead(); // the 1st row
        }
        // we must remove left up-queue's EOD. 
        qleft_.up->removeHead(); // the EOD
        qparent_.down->removeHead(); // done with this request
      }
      else { // it's not an error nor a cancel
        if (qright_.down->isFull()) return WORK_OK; // try again later
        // for safety and correctness, we wait until
        // left is done before passing control to right
        if (pstate.leftstate_ == CS_STARTED) { // left has no data result
          // flow parent's down-queue entry to the right
          flowParent2Right(lentry->upState.parentIndex);
          qleft_.up->removeHead(); // it's now useless
        }
        qright_.down->insert();
        pstate.leftstate_ = CS_DONE;
        pstate.rightstate_ = CS_STARTED;
        if (pentry->downState.request == ex_queue::GET_NOMORE) {
          // immediately cancel the request
          qright_.down->cancelRequestWithParentIndex
            (lentry->upState.parentIndex);
          pstate.rightstate_ = CS_CANCELLED;
        }
      }
      break;
    case ex_queue::Q_OK_MMORE:
      if (pstate.leftstate_ == CS_ERROR || 
          pstate.leftstate_ == CS_CANCELLED) {
        qleft_.up->removeHead(); // toss it out; it's useless
        continue;
      }
      if (pstate.leftrows_ == 0) {
        // make sure right child's down-queue is not full before
        // changing its tail entry.
        if (qright_.down->isFull()) return WORK_OK; // try again later
        // catpound statement flows only 1st row from left to right
        flowLeft2Right(lentry);
        pstate.leftstate_ = CS_NOT_EMPTY;
      }
      else { // pstate.leftrows_ > 0
        pentry->downState.request = ex_queue::GET_NOMORE;
        if (qparent_.up->isFull()) return WORK_OK; // try again later
        pstate.leftstate_ = CS_ERROR;
        processCardinalityError(lentry);
        qleft_.up->removeHead(); // toss it out; it's useless
      }
      pstate.leftrows_++;
      break;
    case ex_queue::Q_SQLERROR:
      if (pstate.leftstate_ == CS_ERROR || 
          pstate.leftstate_ == CS_CANCELLED) {
        qleft_.up->removeHead(); // toss it out; it's useless
        continue;
      }
      pentry->downState.request = ex_queue::GET_NOMORE;
      if (qparent_.up->isFull()) return WORK_OK; // try again later
      pstate.leftstate_ = CS_ERROR;
      processError(lentry->getAtp(), NULL);
      qleft_.up->removeHead(); // toss it out; it's useless
      // pstate.leftrows_++; don't do this here because the Q_NO_DATA
      // case above checks for a positive leftrows_ and will try to 
      // remove the entry we've just removed from the left up-queue.
      
      break;
    case ex_queue::Q_INVALID:
    default:
      ex_assert(0,"ExCatpoundStmtTcb:workLeft2Right: Invalid state");
      break;
    }
  }
  return WORK_OK;
} // ExCatpoundStmtTcb::workLeft2Right


// flow tuple value from left to right
void ExCatpoundStmtTcb::flowLeft2Right(ex_queue_entry *lentry)
{
  ex_queue_entry *rentry = qright_.down->getTailEntry();
  ex_queue_entry *pentry = qparent_.down->getQueueEntry
    (lentry->upState.parentIndex);

  // pass same request from left to right child
  rentry->downState.request = pentry->downState.request;
  rentry->downState.requestValue = pentry->downState.requestValue;

  // associate right-down request with left-up entry's parent index
  rentry->downState.parentIndex = lentry->upState.parentIndex;

  // catpound statement flows any input data from left to right
  rentry->copyAtp(lentry);
  // passAtp may be OK here. But, we use copyAtp to be consistent with
  // ExCatpoundStmtTcb::flowParent2Right
} // ExCatpoundStmtTcb::flowLeft2Right


// flow tuple value from parent to right
void ExCatpoundStmtTcb::flowParent2Right(queue_index pindex)
{
  // We're the 2nd catpoundstmt node in a 3-statement block like this
  //   begin; select a from t; delete from u; select b from v; end;
  // The queues look like this
  //                  +--+
  //                  |CS|
  //                  +--+
  //   | |    +-+             | |      +-+
  //   | |    | |             |g|      | |
  //   +-+    | |             +-+      | |
  //      +--+                    +--+
  //      |S |                    |CS|
  //      +--+                    +--+
  //                    | |    +-+    | |    +-+
  //                    | |    |e|    | |    | |
  //                    +-+    | |    +-+    | |
  //                       +--+          +--+
  //                       |D |          |S |
  //                       +--+          +--+
  // The 1st catpoundstmt flowed the 1st select's result to our parent's
  // down-queue(g). Our left child (the delete) has a Q_NO_DATA in its up-
  // queue(e). Now, we must flow our parent's down-queue data (g) to our
  // right child's down-queue. If we don't do this (and instead flow our
  // left up-queue to the right down-queue), we'll lose the 1st select's
  // result because our left up-queue has nothing but a Q_NO_DATA.

  ex_queue_entry *rentry = qright_.down->getTailEntry();
  ex_queue_entry *pentry = qparent_.down->getQueueEntry(pindex);

  // pass same request from parent to right child
  rentry->downState.request = pentry->downState.request;
  rentry->downState.requestValue = pentry->downState.requestValue;

  // associate right-down request with parent index
  rentry->downState.parentIndex = pindex;

  // pass data from parent-to-right
  rentry->copyAtp(pentry);
  // passAtp would be incorrect here because catpoundstmts like this
  //   begin; set :h = select c from t; insert into s(d) values(:h); end;
  // require that :h be set to null if "select c from t" has no data.
  // In this case, copyAtp sets :h to null whereas passAtp leaves :h
  // uninitialized.
} // ExCatpoundStmtTcb::flowParent2Right


ExWorkProcRetcode ExCatpoundStmtTcb::workUp()
{
  // while there is work that has been started do
  while (qparent_.down->getHeadIndex() != parent2leftx_) {
    // get head entry and pstate
    ex_queue_entry *pdentry = qparent_.down->getHeadEntry();
    ExCatpoundStmtPrivateState &pstate = 
      *((ExCatpoundStmtPrivateState*) pdentry->pstate);
    Int32 eod = 0;

    // while we have (up or cancel or error) work to do, ie,
    // while right child's up-queue is non-empty do
    for (; !qright_.up->isEmpty() && !eod; qright_.up->removeHead()) {
      // get right child's first up-queue entry
      ex_queue_entry *rentry = qright_.up->getHeadEntry();
      // get parent's last up-queue entry
      ex_queue_entry *puentry = qparent_.up->getTailEntry();
        
      switch (rentry->upState.status) {
      case ex_queue::Q_NO_DATA:
        // any room in the up-queue?
        if (qparent_.up->isFull()) return WORK_OK; // no, try again later

        // if we did not get atleast one row from the right child and we
        // are expecting rows (because of SELECT) from the right child
        // then raise a warning and stop further execution of CS statements
        if (pstate.rightstate_ == CS_STARTED &&
            expectingRightRows() ) {
          pstate.rightstate_ = CS_ERROR;
          if (afterUpdate()) {
            processEODErrorOrWarning(FALSE);
          }
          else {
            processEODErrorOrWarning(TRUE);
          }
          if (qparent_.up->isFull()) return WORK_OK;
        }

        if (pstate.rightstate_ == CS_ERROR || 
            pstate.rightstate_ == CS_CANCELLED) { 
          passChildReplyUp(rentry); // pass EOD up
        }
        else {
          if (pstate.rightstate_ == CS_STARTED && pstate.leftrows_ > 0) {
            // right has no data but left has some. pass left data up.
            passChildReplyUp(qleft_.up->getHeadEntry());
            pstate.rightrows_ = pstate.leftrows_;
            if (qparent_.up->isFull()) {
              pstate.rightstate_ = CS_DONE; // pass it up exactly once
              return WORK_OK; // retry passing EOD later
            }
          }
          pstate.rightstate_ = CS_DONE;
          passChildReplyUp(rentry); // pass EOD up
          if (getStatsEntry() != NULL) {
            getStatsEntry()->setActualRowsReturned(pstate.rightrows_);
          } 
        }
        eod = 1; // this parent request is done
        if (pstate.leftrows_ > 0) {
        // left had some data and that data has been catenated into right
        // child's atp, flush that left child data parked in it's up-queue.
          qleft_.up->removeHead(); // left data
          qleft_.up->removeHead(); // left EOD
        }
        break;
      case ex_queue::Q_OK_MMORE:
      case ex_queue::Q_SQLERROR:
        // if right is cancelled or an error, then throw it away
        if (pstate.rightstate_ == CS_ERROR ||
            pstate.rightstate_ == CS_CANCELLED) continue;

        if (qparent_.up->isFull()) return WORK_OK; // retry later

        if (rentry->upState.status == ex_queue::Q_SQLERROR) {
          pstate.rightstate_ = CS_ERROR;
          processError(rentry->getAtp(), NULL);
        }
        // catpound statement passes-up only 1st row from right
        else if (pstate.rightrows_ == 0) {
          passChildReplyUp(rentry);
          pstate.rightstate_ = CS_NOT_EMPTY;
        }
        else if (pstate.rightrows_ > 0) {
          pstate.rightstate_ = CS_ERROR;
          processCardinalityError(rentry);
        }
        pstate.rightrows_++;
        break;
      case ex_queue::Q_INVALID:
      default:
        ex_assert(0,"ExCatpoundStmtTcb::workUp: Invalid state");
        break;
      }
    }
    // right child's up-queue is empty or current request is done
    if (eod)
      qparent_.down->removeHead(); // this parent request is done
    else
      return WORK_OK;
  }
  return WORK_OK;
} // ExCatpoundStmtTcb::workUp


void ExCatpoundStmtTcb::passChildReplyUp(ex_queue_entry *centry)
{
  // get parent's down-queue head entry and its pstate
  ex_queue_entry *pdentry = qparent_.down->getHeadEntry();

  // get parent's last up-queue entry
  ex_queue_entry *puentry = qparent_.up->getTailEntry();

  // prepare to pass rentry up to parent's up-queue
  puentry->upState.status = centry->upState.status;
  puentry->upState.setMatchNo(centry->upState.getMatchNo());
  puentry->upState.parentIndex = pdentry->downState.parentIndex;
  puentry->upState.downIndex = qparent_.down->getHeadIndex();

  // pass the reply up to the parent's up-queue
  puentry->copyAtp(centry);
  qparent_.up->insert();
} // ExCatpoundStmtTcb::passChildReplyUp


ExWorkProcRetcode ExCatpoundStmtTcb::workCancel()
{
  // Check the down queue from the parent for cancellations. Propagate
  // cancel requests and remove all requests that are completely cancelled.
  // Loop over all requests that have been sent down.
  queue_index x;
  for (x = qparent_.down->getHeadIndex(); x != parent2leftx_; x++) {
    ex_queue_entry *pentry = qparent_.down->getQueueEntry(x);

    // check whether the current down request is cancelled
    if (pentry->downState.request == ex_queue::GET_NOMORE) {
      ExCatpoundStmtPrivateState &pstate = 
        *((ExCatpoundStmtPrivateState*)pentry->pstate);
	  if (pstate.leftstate_ == CS_STARTED ||
          pstate.leftstate_ == CS_NOT_EMPTY) {
        // cancel request to left child
        qleft_.down->cancelRequestWithParentIndex(x);
        pstate.leftstate_ = CS_CANCELLED;
      }
      if (pstate.rightstate_ == CS_STARTED ||
          pstate.rightstate_ == CS_NOT_EMPTY) {
        // cancel request to right child
        qright_.down->cancelRequestWithParentIndex(x);
        pstate.rightstate_ = CS_CANCELLED;
      }
    }
  }
  return WORK_OK;
} // ExCatpoundStmtTcb::workCancel


// Process and propagate error up to parent's up-queue
void ExCatpoundStmtTcb::processError(atp_struct *atp, ComDiagsArea *da)
{
  ex_queue_entry *pdentry = qparent_.down->getHeadEntry();
  ex_queue_entry *puentry = qparent_.up->getTailEntry();
  ExCatpoundStmtPrivateState &pstate =
    *((ExCatpoundStmtPrivateState*)pdentry->pstate);

  // set up error entry for parent up-queue
  puentry->copyAtp(atp);
  puentry->upState.status = ex_queue::Q_SQLERROR;
  puentry->upState.parentIndex = pdentry->downState.parentIndex;
  puentry->upState.downIndex = qparent_.down->getHeadIndex();
  puentry->upState.setMatchNo(pstate.rightrows_);
  if (da) puentry->setDiagsAreax(da);

  // insert entry into parent's up queue
  qparent_.up->insert();

  // cancel this request and all its children
  qparent_.down->cancelRequest(qparent_.down->getHeadIndex());
  workCancel();
} // ExCatpoundStmtTcb::processError


// Process CS error due to a child returning > 1 row.
void ExCatpoundStmtTcb::processCardinalityError(ex_queue_entry *centry)
{
  // create error for diags
  ComDiagsArea *da = ExRaiseSqlError
    (getGlobals()->getDefaultHeap(), centry,
     (ExeErrorCode)-EXE_BLOCK_CARDINALITY_VIOLATION);
  processError(centry->getAtp(), da);
} // ExCatpoundStmtTcb::processCardinalityError


//This method is used to raise error -EXE_CS_EOD_ROLLBACK_ERROR or
// warning +EXE_CS_EOD. Raising this error or warning causes further processing of the
// CS to be stopped and if any updates were seen previously in this CS then the
// whole transaction is rolled back. Note that the warning EXE_CS_EOD is actually
//raised as an error here and and attached to a Q_SQLERROR entry. It is converted into a
//warning in the root::fetch or root::oltExecute method. This is done since it
//is crucial that this warning be posted and further processing on the offending CS be stopped.
//There is a possibility that warnings are not propogated correctly and we continue processing
//on the CS once the warning has been raised as an actual warning. 
void ExCatpoundStmtTcb::processEODErrorOrWarning(NABoolean isWarning)
{

  ex_queue_entry *pdentry = qparent_.down->getHeadEntry();
  ex_queue_entry *puentry = qparent_.up->getTailEntry();

  ComDiagsArea * da ;

  if (isWarning)
     da = ExRaiseSqlError(getGlobals()->getDefaultHeap(), puentry,
                         (ExeErrorCode)-EXE_CS_EOD);
  else
     da = ExRaiseSqlError(getGlobals()->getDefaultHeap(), puentry,
                         (ExeErrorCode)-EXE_CS_EOD_ROLLBACK_ERROR);

  puentry->setDiagsAreax(da);
  puentry->upState.status = ex_queue::Q_SQLERROR;
  puentry->upState.parentIndex = pdentry->downState.parentIndex;
  puentry->upState.downIndex = qparent_.down->getHeadIndex();
  puentry->upState.setMatchNo((Lng32)0);

  qparent_.up->insert();
}

//////////////////////////////////////////////////////////////////////////////
// CatpoundStmtPrivateState methods.
//////////////////////////////////////////////////////////////////////////////
void ExCatpoundStmtPrivateState::init() 
{
  leftstate_  = ExCatpoundStmtTcb::CS_EMPTY;
  rightstate_ = ExCatpoundStmtTcb::CS_EMPTY;
  leftrows_ = 0;
  rightrows_ = 0;
} // void ExCatpoundStmtPrivateState::init


ex_tcb_private_state*
ExCatpoundStmtPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) 
    ExCatpoundStmtPrivateState((ExCatpoundStmtTcb *) tcb);
} // ExCatpoundStmtPrivateState::allocate_new
