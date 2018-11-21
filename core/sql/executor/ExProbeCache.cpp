// **********************************************************************
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
// **********************************************************************

#include "Platform.h"

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExProbeCache.h"
#include "ex_exe_stmt_globals.h"
#include "ex_expr.h"


ex_tcb * ExProbeCacheTdb::build(ex_globals * glob)
{
  ExExeStmtGlobals * exe_glob = glob->castToExExeStmtGlobals();
  
  ex_assert(exe_glob,"Probe Cache operator can't be in DP2");

  // first build the child
  ex_tcb *  child_tcb;
  child_tcb = tdbChild_->build(glob);

  ExProbeCacheTcb *pc_tcb = new(exe_glob->getSpace()) 
        ExProbeCacheTcb(
	 *this,
         *child_tcb,
	 exe_glob);

  ex_assert(pc_tcb, "Error building ExProbeCacheTcb.");

  // Add subtasks to the scheduler.
  pc_tcb->registerSubtasks();

  // This operator does use dynamic queue resizing.
  pc_tcb->registerResizeSubtasks();

  return (pc_tcb);
}


////////////////////////////////////////////////////////////////
// Constructor and initialization.
////////////////////////////////////////////////////////////////

ExProbeCacheTcb::ExProbeCacheTcb(const ExProbeCacheTdb &probeCacheTdb, 
				   const ex_tcb &child_tcb,
				   ex_globals * glob
				    ) : 
    ex_tcb( probeCacheTdb, 1, glob),
    childTcb_(NULL),
    workAtp_(NULL),
    probeBytes_(NULL),
    pool_(NULL),
    pcm_(NULL),
    workUpTask_(NULL)
{
  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);

  qparent_.up = qparent_.down = NULL;
  
  childTcb_ = &child_tcb;

  // Allocate the buffer pool
  pool_ = new(space) ExSimpleSQLBuffer( probeCacheTdb.numInnerTuples_,
                                        probeCacheTdb.recLen_,
                                        space);

  // get the child queue pair
  qchild_  = child_tcb.getParentQueue(); 

  // Allocate the queue to communicate with parent
  allocateParentQueues(qparent_);

  // Intialize nextRequest_ to the next request to process
  nextRequest_ = qparent_.down->getTailIndex();

  // Allocate buffer for probeBytes_.
  probeBytes_ = new(space) char[ probeCacheTdb.probeLen_ ];

  // allocate work atp and initialize the two tupps.
  workAtp_ = allocateAtp(probeCacheTdb.criDescUp_,getSpace());

  probeHashTupp_.init(sizeof(probeHashVal_),
		    NULL,
		    (char *) (&probeHashVal_));
  workAtp_->getTupp(probeCacheTdb.hashValIdx_) = &probeHashTupp_;

  hashProbeExpr()->fixup(0, getExpressionMode(), this, space, heap,
                glob->computeSpace(), glob);

  probeEncodeTupp_.init(probeCacheTdb.probeLen_,
		    NULL,
		    (char *) (probeBytes_));
  workAtp_->getTupp(probeCacheTdb.encodedProbeDataIdx_) = &probeEncodeTupp_;

  encodeProbeExpr()->fixup(0, getExpressionMode(), this, space, heap,
                glob->computeSpace(), glob);

  if (moveInnerExpr())
    moveInnerExpr()->fixup(0, getExpressionMode(), this, space, heap,
                glob->computeSpace(), glob);

  if (selectPred())
    selectPred()->fixup(0, getExpressionMode(), this, space, heap,
                glob->computeSpace(), glob);

  pcm_ = new(space) ExPCMgr(space, 
                probeCacheTdb.cacheSize_, probeCacheTdb.probeLen_, this);

}

NABoolean ExProbeCacheTcb::needStatsEntry()
{
  // stats are collected for ALL and OPERATOR options.
  if ((getGlobals()->getStatsArea()->getCollectStatsType() == ComTdb::ALL_STATS) ||
      (getGlobals()->getStatsArea()->getCollectStatsType() == ComTdb::OPERATOR_STATS))
    return TRUE;
  else
    return FALSE;
}

ExOperStats * ExProbeCacheTcb::doAllocateStatsEntry(CollHeap *heap,
                                                        ComTdb *tdb)
{
  ExOperStats * stat = NULL;
  ComTdb::CollectStatsType statsType = getGlobals()->getStatsArea()->getCollectStatsType();
  if (statsType == ComTdb::OPERATOR_STATS)
  {
    return ex_tcb::doAllocateStatsEntry(heap, tdb);
  }
  else
  {
    ExProbeCacheTdb * pcTdb = (ExProbeCacheTdb*) tdb;

    return new(heap) ExProbeCacheStats(heap,
                      this,
                      tdb,
                      pcTdb->bufferSize_,
                      pcTdb->cacheSize_);
  }
}

void ExProbeCacheTcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  sched->registerInsertSubtask(sWorkDown,   this, qparent_.down,"PD");
  sched->registerUnblockSubtask(sWorkDown,  this, qchild_.down, "PD");

  sched->registerInsertSubtask(sWorkUp,     this, qchild_.up,   "PU");
  sched->registerUnblockSubtask(sWorkUp,    this, qparent_.up,  "PU");

  sched->registerCancelSubtask(sCancel,     this, qparent_.down,"CN");

  // We need to schedule workUp from workDown if a call to workDown 
  // has changed any request to either CACHE_HIT or CANCELED_NOT_STARTED 
  // and if it has not changed any request to CACHE_MISS.
  workUpTask_ = sched->registerNonQueueSubtask(sWorkUp, this, "PU");

}

ex_tcb_private_state * ExProbeCacheTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExProbeCachePrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

////////////////////////////////////////////////////////////////
// Destructor and cleanup.
////////////////////////////////////////////////////////////////

ExProbeCacheTcb::~ExProbeCacheTcb()
{
  freeResources();
}
  
void ExProbeCacheTcb::freeResources()
{
  if (pool_) 
    {
      delete pool_;
      pool_ = NULL;
    }

  if (qparent_.up)
    {
      delete qparent_.up;
      qparent_.up = NULL;
    }
  
  if (qparent_.down)
    {
      delete qparent_.down;
      qparent_.down = NULL;
    }

  if (probeBytes_)  
    {
      NADELETEBASIC(probeBytes_, getSpace());
      probeBytes_ = NULL;
    }

  if (pcm_)
    {
      delete pcm_;
      pcm_ = NULL;
    }
}
  
///////////////////////////////////////////////////////////////////
// The various work subtask methods.
///////////////////////////////////////////////////////////////////
ExWorkProcRetcode ExProbeCacheTcb::work()
{
  ex_assert(0, "ExProbeCache has separate workUp and workDown subtasks.");
  return WORK_OK;
}

///////////////////////////////////////////////////////////////////

ExWorkProcRetcode ExProbeCacheTcb::workDown()
{
  NABoolean anyMiss = FALSE;
  NABoolean anyHitOrCanceled = FALSE;
  queue_index tlindex = qparent_.down->getTailIndex();

  while (nextRequest_ != tlindex && !qchild_.down->isFull()) 
    { 
      ex_queue_entry *pentry = qparent_.down->getQueueEntry(nextRequest_);
      ExProbeCachePrivateState & pstate
	= *((ExProbeCachePrivateState *) pentry->pstate);

      ex_assert(pstate.step_ == NOT_STARTED, "deja vu in ExProbeCacheTcb")
      
      switch (pentry->downState.request)
      {
        case ex_queue::GET_N:
          {
            // As long as probe cache restricts probes to no more than
            // one reply, nothing special is need for GET_N.  Just fall
            // thru to the GET_ALL case.
          }
        case ex_queue::GET_ALL:
          {
            // Evaluate the hash expression on the probe.
            if ((hashProbeExpr()->eval(pentry->getAtp(),workAtp_)) != 
                ex_expr::EXPR_OK)
              ex_assert(0, "Unexpected result from hashProbeExpr");

            // Evaluate the encode expression on the probe.
            if ((encodeProbeExpr()->eval(pentry->getAtp(),workAtp_)) != 
                ex_expr::EXPR_OK)
              ex_assert(0, "Unexpected result from hashEncodeExpr");

            // Look for a match in the cache.  If it is not found
            // this same method will add this probe.
            if (pcm_->addOrFindEntry(
                  probeHashVal_, probeBytes_, nextRequest_, pstate.pcEntry_
                                    ) == ExPCMgr::FOUND)
              {
                // Had a probe already in cache, so workDown is finished
                // with this request.  Change step_ and move on to next one.
                pstate.step_ = CACHE_HIT;
                anyHitOrCanceled = TRUE;
              }
            else
              {
                // No probe was found in in cache; the probe cache manager has 
                // created one, but workDown must pass this request to
                // the child so that the inner table reply will be ready when 
                // this request (and any subsequent request that match this
                // request's probe data) comes to the head of the parent down
                // queue and is processed by work up.
                pstate.step_ = CACHE_MISS;
                anyMiss = TRUE;
                ex_queue_entry * centry = qchild_.down->getTailEntry();
                // pass same request down.
                centry->downState.request = pentry->downState.request;
                centry->downState.requestValue = 
                          pentry->downState.requestValue;
                centry->downState.numGetNextsIssued = 
                          pentry->downState.numGetNextsIssued;
                // remember the parent q entry from whence this request came.
                centry->downState.parentIndex = nextRequest_; 
                // set the atp to the parent atp.
                centry->passAtp(pentry);
                qchild_.down->insert();
              }
            break;
          }
        case ex_queue::GET_NOMORE:
          {
	    pstate.step_ = CANCELED_NOT_STARTED;
            anyHitOrCanceled = TRUE;
	    break;
          }
        default:
          {
            ex_assert(0, "ExProbeCacheTcb cannot handle this request");
            break;
          }
      }  // end switch on pentry->downState.request
    nextRequest_++;
    }

  if (anyHitOrCanceled && !anyMiss)
    {
      // Must be sure that workUp will be dispatched and since we 
      // cannot rely on activity in the ex_queues given that this
      // call to workDown didn't put anything in the child down queue,
      // we must scheduler workUp here.
      workUpTask_->schedule();
    }

  return WORK_OK;
}

///////////////////////////////////////////////////////////////////

ExWorkProcRetcode ExProbeCacheTcb::workUp()
{

  ExProbeCacheStats *stats = getProbeCacheStats();
  // Work on requests from the head of parent down, until
  // either we reach nextRequest_ (which work down hasn't seen yet)
  // or until there is not room in the up queue.  Note that 
  // there are "return" statements coded in this loop.
  while ((qparent_.down->getHeadIndex() != nextRequest_) &&
         !qparent_.up->isFull())
    {
      ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
      ExProbeCachePrivateState & pstate
	= *((ExProbeCachePrivateState *) pentry_down->pstate);

      switch (pstate.step_)
        {
          case CACHE_MISS: 
            { 
              if (qchild_.up->isEmpty())
                return WORK_OK;
              ex_queue_entry *reply = qchild_.up->getHeadEntry();
              switch( reply->upState.status )
                {
                  case ex_queue::Q_OK_MMORE:
                    {
                      MoveStatus moveRetCode = 
                        moveReplyToCache(*reply, *pstate.pcEntry_);

                      if (moveRetCode == MOVE_BLOCKED)
                        {
                          return WORK_POOL_BLOCKED;
                        }
                      else if (moveRetCode == MOVE_OK)
                        {
                          pstate.matchCount_ = 1;

                          makeReplyToParentUp(pentry_down, pstate, 
                               ex_queue::Q_OK_MMORE);

                          // Cancel here, b/c semi-join and anti-semi-join
                          // will return more than one Q_OK_MMORE.  Tbd -
                          // perhaps the tdb should pass a flag for this.  

                          qchild_.down->cancelRequestWithParentIndex(
                                    pstate.pcEntry_->probeQueueIndex_);

                          break;
                        }
                      else
                        {
                          ex_assert(moveRetCode == MOVE_ERROR, 
                                    "bad retcode from moveReplyToCache");
                          // Don't break from this Q_OK_MMORE case, but
                          // instead flow down as if Q_SQLERROR.  The 
                          // diagsArea should have been init'd in the 
                          // moveInnerExpr()->eval. 
                        }
                    }
                  case ex_queue::Q_SQLERROR:
                    {
                      // Initialize ExPCE members
                      pstate.pcEntry_->upstateStatus_ = ex_queue::Q_SQLERROR;
  
                      ComDiagsArea *da = reply->getAtp()->getDiagsArea();
                      ex_assert(da, "Q_SQLERROR without a diags area");
                      pstate.pcEntry_->diagsArea_ =  da;
                      da->incrRefCount();

                      makeReplyToParentUp(pentry_down, pstate,
                                          ex_queue::Q_SQLERROR);

                      // No need to cancel, since we expect no more than
                      // one reply from our child.

                      break;
                    }
                  case ex_queue::Q_NO_DATA:
                    {
                      // Initialize ExPCE members
                      pstate.pcEntry_->upstateStatus_ = ex_queue::Q_NO_DATA;
  
                      ComDiagsArea *da = reply->getAtp()->getDiagsArea();
                      if (da)
                        {
                          pstate.pcEntry_->diagsArea_ =  da;
                          da->incrRefCount();
                        }
                      break;
                      // A Q_NO_DATA will be inserted into the parent up queue 
                      // in the DONE step_.  
                    }
                  default:
                    {
                      ex_assert(0, "Unknown upstate.status in child up queue");
                      break;
                    }
                }
              if (stats)
                stats->incMiss();
              pstate.pcEntry_->release(); //Request no longer references PCE.
              pstate.step_ = DONE_MISS;
              break;
            }
          case CACHE_HIT:
            {
              switch(pstate.pcEntry_->upstateStatus_)
                {
                  case ex_queue::Q_OK_MMORE:
                    {
                      pstate.matchCount_ = 1;

                      makeReplyToParentUp(pentry_down, pstate,
                            pstate.pcEntry_->upstateStatus_);

                      break;
                    }
                  case ex_queue::Q_SQLERROR:
                    {

                      makeReplyToParentUp(pentry_down, pstate,
                            pstate.pcEntry_->upstateStatus_);

                      // No need to cancel, since we expect no more than
                      // one reply from our child.

                      break;
                    }
                  case ex_queue::Q_NO_DATA:
                    {
                      // The DONE step will handle this.
                      break;
                    }
                  case ex_queue::Q_INVALID:
                    {
                      // Should not happen.
                      ex_assert(0, "CACHE_HIT saw Q_INVALID");
                    }
                  default:
                    {
                      // Should not happen.
                      ex_assert(0, "CACHE_HIT saw unknown upstateStatus");
                    }
                }
              if (stats)
                stats->incHit();
              pstate.step_ = DONE;
              pstate.pcEntry_->release(); //Request no longer references PCE.
              break;
            }
          case CANCELED_MISS:
            { 
              if (qchild_.up->isEmpty())
                return WORK_OK;
              if (pstate.pcEntry_->refCnt_ != 0)
                {
                  // There are other requests that are interested in this
                  // reply, so put it into the Probe Cache, according to
                  // its upState.status.  However, do not reply with Q_OK_MMORE
                  // or Q_SQLERROR to this request.
                  ex_queue_entry *reply = qchild_.up->getHeadEntry();
                  switch( reply->upState.status )
                    {
                      case ex_queue::Q_OK_MMORE:
                        {
                          MoveStatus moveRetCode2 = 
                            moveReplyToCache(*reply, *pstate.pcEntry_);
                          if (moveRetCode2 == MOVE_BLOCKED)
                            {
                              return WORK_POOL_BLOCKED;
                            }
                          else if (moveRetCode2 == MOVE_OK)
                            {
                              // Now that we have the reply, we can propagate 
                              // the cancel.  We do this b/c (anti-)semi-join 
                              // will return more than one Q_OK_MMORE.  Tbd -
                              // perhaps the tdb should pass a flag for this.  

                              qchild_.down->cancelRequestWithParentIndex(
                                        pstate.pcEntry_->probeQueueIndex_);
                              break;
                            }
                          else
                            {
                              ex_assert(moveRetCode2 == MOVE_ERROR, 
                                        "bad retcode from moveReplyToCache");
                              // Don't break from this Q_OK_MMORE case, but
                              // instead flow down as if Q_SQLERROR.  The 
                              // diagsArea should have been init'd in the 
                              // moveInnerExpr()->eval. 
                            }
                        }
                      case ex_queue::Q_SQLERROR:
                        {
                          // Initialize ExPCE members
                          pstate.pcEntry_->upstateStatus_ = ex_queue::Q_SQLERROR;
  
                          ComDiagsArea *da = reply->getAtp()->getDiagsArea();
                          ex_assert(da, "Q_SQLERROR without a diags area");
                          pstate.pcEntry_->diagsArea_ =  da;
                          da->incrRefCount();

                          break;
                        }
                      case ex_queue::Q_NO_DATA:
                        {
                          // Initialize ExPCE members
                          pstate.pcEntry_->upstateStatus_ = ex_queue::Q_NO_DATA;
  
                          ComDiagsArea *da = reply->getAtp()->getDiagsArea();
                          if (da)
                            {
                              pstate.pcEntry_->diagsArea_ =  da;
                              da->incrRefCount();
                            }
                          break;
                        }
                      default:
                        {
                          ex_assert(0, "Unknown upstate.status in child up queue");
                          break;
                        }
                    }
                }
              else
                {
                  // There are no uncanceled requests interested 
                  // in this  PCE.  
                }
              if (stats)
                stats->incCanceledMiss();
              pstate.step_ = DONE_MISS;
              break;
            }
          case CANCELED_HIT:
            {
              if (stats)
                stats->incCanceledHit();
              pstate.step_ = DONE;
              break;
            }
          case CANCELED_NOT_STARTED:
            {
              if (stats)
                stats->incCanceledNotStarted();
              pstate.step_ = DONE;
              break;
            }
          case DONE_MISS:
            {
              // In this step we discard the original 1st reply to the 
              // CACHE_MISS or CANCELED_MISS, as well as any other replies.
              // We can get multiple replies for a semi-join or
              // anti-semi-join.  
              // We also discard the Q_NO_DATA.
              NABoolean finishedDoneMiss = FALSE;
              while (!finishedDoneMiss)
                {
                  if (qchild_.up->isEmpty())
                    return WORK_OK;
                  ex_queue_entry *reply2 = qchild_.up->getHeadEntry();
                  if (reply2->upState.status == ex_queue::Q_NO_DATA)
                    finishedDoneMiss = TRUE;
                  qchild_.up->removeHead();
                }
              pstate.step_ = DONE;  
              break;
            }
          case DONE:
            {
              makeReplyToParentUp(pentry_down, pstate, ex_queue::Q_NO_DATA);
                 
              pstate.init();
              qparent_.down->removeHead();
              break;
            }
          case NOT_STARTED:
          default:  
            {
              ex_assert(0, "workUp saw unexpected pstate.step_");
              break;
            }
        }
    }
  return WORK_OK;
}

///////////////////////////////////////////////////////////////////

ExWorkProcRetcode ExProbeCacheTcb::workCancel()
{
  queue_index qindex = qparent_.down->getHeadIndex();

  while (qindex != nextRequest_) 
    { 
      ex_queue_entry *pentry = qparent_.down->getQueueEntry(qindex);
      ExProbeCachePrivateState & pstate
	= *((ExProbeCachePrivateState *) pentry->pstate);
      if (pentry->downState.request == ex_queue::GET_NOMORE)
        {
          switch (pstate.step_)
            {
              case CACHE_MISS:
                {
                  cancelInterest(pstate.pcEntry_);
                  pstate.step_ = CANCELED_MISS;
                  break;
                }
              case CACHE_HIT:
                {
                  cancelInterest(pstate.pcEntry_);
                  pstate.step_ = CANCELED_HIT;
                  break;
                }
              case DONE:
              case DONE_MISS:
                {
                  // This can happen, but no further canceling is needed.
                  break;
                }
              case CANCELED_MISS:
              case CANCELED_HIT:
              case CANCELED_NOT_STARTED:
                {
                  // I think this can happen, but no further canceling needed.
                  break;
                }
              case NOT_STARTED:
                {
                  // The comparison of qindex to nextRequest_ should prevent.
                  ex_assert(0, 
                  "canceling & parent down queue processing out of sync");
                }
              default:
                {
                  ex_assert(0, 
                  "canceling found bad pstate.step_");
                }
            }
        }
      qindex++;
    }
  return WORK_OK;
}

///////////////////////////////////////////////////////////////////
// Some helper methods called by the work methods.
///////////////////////////////////////////////////////////////////

void ExProbeCacheTcb::makeReplyToParentUp(ex_queue_entry *pentry_down, 
                                      ExProbeCachePrivateState &pstate, 
                                      ex_queue::up_status reply_status)
{
  ExOperStats *stats;
  ex_queue_entry * up_entry = qparent_.up->getTailEntry();
  Int32 rowQualifies = 1;
  
  // Copy the pointers to the input data from parent
  up_entry->copyAtp(pentry_down);

  if ((reply_status == ex_queue::Q_OK_MMORE) &&
      (pstate.pcEntry_->innerRowTupp_.isAllocated()))
    {
      // Use the pcEntry_ to set the returned tuple from 
      // the pool. 
      up_entry->getAtp()->getTupp(probeCacheTdb().tuppIndex_) = 
            pstate.pcEntry_->innerRowTupp_;

      if (selectPred())
        {
          ex_expr::exp_return_type evalRetCode =
            selectPred()->eval(up_entry->getAtp(), 0);

          if (evalRetCode == ex_expr::EXPR_FALSE)
            rowQualifies = 0;
          else if (evalRetCode != ex_expr::EXPR_TRUE)
            {
              ex_assert(evalRetCode == ex_expr::EXPR_ERROR,
                        "invalid return code from expr eval");
              // diags area should have been generated
              reply_status = ex_queue::Q_SQLERROR;
            }
        }
    }

  if (rowQualifies)
    {
      // Initialize the upState.
      up_entry->upState.parentIndex =
        pentry_down->downState.parentIndex;
      up_entry->upState.downIndex = qparent_.down->getHeadIndex();
      up_entry->upState.setMatchNo(pstate.matchCount_);
      up_entry->upState.status = reply_status;

      // Give the reply any diagsArea.  Test pcEntry_ before using it
      // because a request that went from CANCELED_NOT_STARTED to
      // DONE will never be hooked up with a pcEntry_.
      if (pstate.pcEntry_ &&
          pstate.pcEntry_->diagsArea_)
        {
          ComDiagsArea *accumulatedDiagsArea =
            up_entry->getDiagsArea();
          if (accumulatedDiagsArea)
            accumulatedDiagsArea->mergeAfter(*pstate.pcEntry_->diagsArea_);
          else
            {
              up_entry->setDiagsAreax(pstate.pcEntry_->diagsArea_);
              pstate.pcEntry_->diagsArea_->incrRefCount();
            }
        }

      // Insert the reply.
      qparent_.up->insert();
      if ((stats = getStatsEntry()) != NULL && reply_status == ex_queue::Q_OK_MMORE)
        stats->incActualRowsReturned();
    }
  return;
}

///////////////////////////////////////////////////////////////////

ExProbeCacheTcb::MoveStatus
ExProbeCacheTcb::moveReplyToCache(ex_queue_entry &reply,
                                             ExPCE &pcEntry)
{
  if (moveInnerExpr())
    {
      ex_assert(!pcEntry.innerRowTupp_.isAllocated(),
                "reusing an allocated innerRowTupp");

      if (pool_->getFreeTuple(pcEntry.innerRowTupp_))
        return MOVE_BLOCKED;

      workAtp_->getTupp(probeCacheTdb().innerRowDataIdx_) =
            pcEntry.innerRowTupp_;

      // Evaluate the move expression on the reply.
      ex_expr::exp_return_type innerMoveRtn = 
        moveInnerExpr()->eval(reply.getAtp(),workAtp_);
      
      if (innerMoveRtn == ex_expr::EXPR_ERROR)
        return MOVE_ERROR;
    }
  else
    {
      ex_assert(pcEntry.innerRowTupp_.isAllocated() == FALSE, 
               "Incorrectly initialized inneRowTupp_");
    }

  // Initialize ExPCE members
  pcEntry.upstateStatus_ = ex_queue::Q_OK_MMORE;
  
  ComDiagsArea *da = reply.getAtp()->getDiagsArea();
  if (da)
    {
      pcEntry.diagsArea_ =  da;
      da->incrRefCount();
    }

return MOVE_OK;
}

///////////////////////////////////////////////////////////////////

void ExProbeCacheTcb::cancelInterest(ExPCE *pcEntry)
{
  pcEntry->release();

  if ((pcEntry->refCnt_ == 0) &&
      (pcEntry->upstateStatus_ == ex_queue::Q_INVALID))
    {
      // No request is interested in this probe's result and 
      // our workup method has still not processed any reply
      // from the child, so cancel the probe.  (The reason for
      // the Q_INVALID test is that it lets use distinguish
      // a probe that does not have its result from a probe
      // that was able to get a result before being canceled.)
      qchild_.down->cancelRequestWithParentIndex(
                                        pcEntry->probeQueueIndex_);

      // This ExPCE will never get any reply except
      // Q_NO_DATA, and hence is no longer a valid 
      // probe. Mark it so no future request from our parent 
      // attempts to use this probe.
      pcEntry->flags_.canceledPending_ = 1;
    }
}

///////////////////////////////////////////////////////////////////
// Methods for the ExProbeCachePrivateState
///////////////////////////////////////////////////////////////////

ExProbeCachePrivateState::ExProbeCachePrivateState()
{
  init();
}

ExProbeCachePrivateState::~ExProbeCachePrivateState() {}

void ExProbeCachePrivateState::init()
{
  step_ = ExProbeCacheTcb::NOT_STARTED;
  pcEntry_ = NULL;
  matchCount_ = 0;
}

///////////////////////////////////////////////////////////////////
// Methods for the Probe Cache Manager
///////////////////////////////////////////////////////////////////
ExPCMgr::ExPCMgr(Space *space,
                 ULng32 numEntries, 
                 ULng32 probeLength,
                 ExProbeCacheTcb *tcb) :
    space_(space),
    numBuckets_(numEntries),
    probeLen_(probeLength),
    tcb_(tcb),
    buckets_(NULL),
    entries_(NULL),
    nextVictim_(0)
{
    buckets_ = new(space_) ExPCE *[numBuckets_];

    // Initialize all the buckets to "empty".    
    memset((char *)buckets_, 0, numBuckets_ * sizeof(ExPCE *));

    // Calculate the real size for each ExPCE -- the probeData_ 
    // array is one byte so subtract that from probeLength.
    sizeofExPCE_ = ROUND8(sizeof(ExPCE) + (probeLength - 1));

    // Get the size in bytes of the ExPCE array.
    const Int32 totalExPCEsizeInBytes = numEntries * sizeofExPCE_;

    entries_ = new(space_) char[totalExPCEsizeInBytes];

    memset(entries_, 0, totalExPCEsizeInBytes);
};

///////////////////////////////////////////////////////////////////
ExPCMgr::~ExPCMgr()
{
  if (buckets_ != NULL)
    NADELETEBASIC(buckets_, space_);
  buckets_ = NULL;

  if (entries_ != NULL)
    NADELETEBASIC(entries_, space_);
  entries_ = NULL;
}

///////////////////////////////////////////////////////////////////
ExPCMgr::AddedOrFound 
ExPCMgr::addOrFindEntry( ULng32 probeHashVal, 
                         char * probeBytes, 
                         queue_index qIdxForCancel, 
                         ExPCE * &pcEntry )
{
  AddedOrFound retcode;
  const Int32 bucketNum = probeHashVal % numBuckets_;
  UInt32 chainLength = 1;
  ExProbeCacheStats *stats = tcb_->getProbeCacheStats();

  pcEntry = buckets_[bucketNum];

  while (pcEntry)
    {
      if ((pcEntry->probeHashVal_ == probeHashVal) &&
          (memcmp((char *) pcEntry->probeData_, probeBytes, probeLen_) == 0))
        {
          if (pcEntry->flags_.canceledPending_)
            {
              // Found the correct entry for this probe but 
              // but we cannot use it because the corresponding child down 
              // queue request has been canceled and we are awaiting
              // the Q_NO_DATA reply.  Keep looking in case a duplicate
              // probe added an etry after this entry was canceled.
            }
          else
            {
              // This is a hit.
              break;
            }
        }
      pcEntry = pcEntry->nextHashVal_;
      chainLength++;
    }

  if (stats)
    stats->updateLongChain(chainLength);

  if (pcEntry == NULL)
    {
      // Not found (or it was found but a cancel was pending).  So add it.
      pcEntry = addEntry(bucketNum,   // add it to this collision chain.
                     probeHashVal, 
                     probeBytes, 
                     qIdxForCancel);
      retcode = ADDED;
    }
  else 
    {
      pcEntry->refCnt_++;
      pcEntry->flags_.useBit_ = 1;
      retcode = FOUND;
      if (stats)
        stats->updateUseCount(pcEntry->refCnt_);
    }

  return retcode;
  
}

///////////////////////////////////////////////////////////////////
ExPCE *ExPCMgr::addEntry(Int32 buckNum, 
                ULng32 probeHashVal, 
                char * probeBytes, 
                queue_index qIdxForCancel)
{
  ExProbeCacheStats *stats = tcb_->getProbeCacheStats();
  bool foundVictim = false;
  ExPCE *pce;

  while (foundVictim == false)
    {
      pce = getPossibleVictim();
      if (pce->refCnt_ == 0)
        if (pce->flags_.useBit_ == 1)
          pce->flags_.useBit_ = 0;   // give it a second chance.
        else
          foundVictim = true;
    }

  
  if (pce->flags_.everUsed_ == FALSE)
    {
      // This entry has never been added.
    }
  else
    {
      // Unlink from old collision chain
      const Int32 bucketNum = pce->probeHashVal_ % numBuckets_;
      ExPCE *pcEntryToUnlink = buckets_[bucketNum];
      ExPCE *prevPCE = NULL;
      UInt32 chainLength = 1;

      for (;;)
        {
          ex_assert(pcEntryToUnlink != NULL, "corrupt hash table.");

          // Use ptr values to make a match.
          if (pcEntryToUnlink == pce)
            {
              if (prevPCE == NULL)
                {
                  // This was the first entry in the chain.
                  buckets_[bucketNum] = pcEntryToUnlink->nextHashVal_;
                  
                  if ((stats) &&
                      (buckets_[bucketNum] == NULL))
                    stats->freeChain();
                }
              else
                {
                  prevPCE->nextHashVal_ = pcEntryToUnlink->nextHashVal_;
                }
              break;
            }
          prevPCE = pcEntryToUnlink;
          pcEntryToUnlink = pcEntryToUnlink->nextHashVal_;
          chainLength++;
        }
      if (stats)
        stats->updateLongChain(chainLength);
    }

  // Link this entry to head of its collision chain.
  if ((pce->nextHashVal_ = buckets_[buckNum]) == NULL)
    {
      // chain was empty, but now it is not.  Tell stats.
      if (stats)
          stats->newChain();
    }
  buckets_[buckNum] = pce;


  // Now initialize other members:
  pce->flags_.useBit_ = 1;  
  pce->flags_.canceledPending_ = 0;  
  pce->flags_.everUsed_ = 1;
  pce->probeHashVal_ = probeHashVal;
  pce->refCnt_ = 1;
  if (stats)
    stats->updateUseCount(1) ;
  pce->upstateStatus_ = ex_queue::Q_INVALID;
  pce->probeQueueIndex_ = qIdxForCancel;
  if (pce->diagsArea_)
    {
      pce->diagsArea_->decrRefCount();
      pce->diagsArea_ = NULL;
    }
  pce->innerRowTupp_.release();
  memcpy(pce->probeData_, probeBytes, probeLen_);

  return pce;
}

///////////////////////////////////////////////////////////////////
ExPCE *ExPCMgr::getPossibleVictim()
{
  // This method accesses the probe cache entries as an array,
  // to choose a possible victim.  The caller, addEntry, decides
  // whether the entry will be reused.  
  // Besides managing the nextVictim_ index, this method also encapsulates
  // access the array of entries, performing its own pointer arithmetic,
  // which is necessary because of the probeData_ array, the size of which
  // is not know when this C++ is compiled.
 
  // Note assumption that # buckets == # cache entries.
  if (nextVictim_ == numBuckets_)
    nextVictim_ = 0;

  ULng32 offsetToEntry = nextVictim_++ * sizeofExPCE_;
  
  return (ExPCE *) &entries_[offsetToEntry]; 
}
