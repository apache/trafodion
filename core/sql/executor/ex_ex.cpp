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
* File:         ex_ex.c
* Description:  Methods for class ex_tcb and for assertions
*               
*               
* Created:      5/3/94
* Language:     C++
* Status:       Experimental
*
*
*
*
******************************************************************************
*/

#include "Platform.h"

#include        <sys/time.h>
#include        "str.h"
#include        "ex_stdh.h"
#include        "ComTdb.h"
#include        "ex_tcb.h"
#include        "ExStats.h"
#include        "NAError.h"
#include        "exp_expr.h"
#include        "ExCextdecs.h"
#include "logmxevent.h"

extern Int32  gettimeofday(struct timeval *, struct timezone *);

// -----------------------------------------------------------------------
// NT Port - GSH 03/18/97
// ----------------------
// Added code to include GUI related code if building with the GUI tool
// ie. NO_NA_DEBUG_GUI is not set which in turn means NA_DEBUG_GUI is 
// set. The code I added is : && defined(NA_DEBUG_GUI).
// -----------------------------------------------------------------------

#ifdef NA_DEBUG_GUI
  Int32 ex_tcb::objectCount = 0;
#endif

/////////////////////////////////////////////////////////////
// class ex_tcb
/////////////////////////////////////////////////////////////
// Base class constructor
ex_tcb::ex_tcb( const ComTdb & tdb, const short in_version,
               ex_globals * g) :
  tdb(tdb),
  nodeType_((ComTdb::ex_node_type)tdb.getNodeType()),
  version_(in_version),
  globals_(g),
  statsEntry_(NULL),
  pool_(NULL)
  // QSTUFF
  , holdable_(FALSE)
  // QSTUFF
{
  str_cpy_all( (char *) &eyeCatcher_, (char *) &(tdb.eyeCatcher_), 4);

#ifdef NA_DEBUG_GUI
  increaseObjectcount();
  setObjectId(); 
#endif

  // Cleanup
  g->registerTcb(this);
};


ex_tcb::~ex_tcb()
{
  ex_assert(!str_cmp((char *)&eyeCatcher_, (char *)(&tdb.eyeCatcher_), 4),
            "TCB points to wrong TDB");
  // Change the eye catcher
  str_cpy_all((char *) &eyeCatcher_, eye_FREE, 4);
};

void ex_tcb::registerSubtasks()
{
  // the default implementation registers the default static
  // work procedure for all events that can occur for this task
  ExScheduler *sched = globals_->getScheduler();
  ex_queue_pair pQueue = getParentQueue();

  // register events for parent queue
  ex_assert(pQueue.down && pQueue.up,"Parent down queue must exist");
  sched->registerInsertSubtask(ex_tcb::sWork,   this, pQueue.down,"WK");
  sched->registerCancelSubtask(ex_tcb::sCancel, this, pQueue.down,"CN");
  sched->registerUnblockSubtask(ex_tcb::sWork,  this, pQueue.up,"WK");
  // BertBert VV
  // subtask to handle GET_NEXT_N events downstream.
  sched->registerNextSubtask(ex_tcb::sWork, this, pQueue.down,"GN");
  // BertBert ^^

  // register events for child queues
  Int32 nc = numChildren();
  for (Int32 i = 0; i < nc; i++)
    {
      const ex_tcb * childTcb = getChild(i);

      if (childTcb)
        {
          ex_queue_pair cQueue = childTcb->getParentQueue();

          sched->registerUnblockSubtask(ex_tcb::sWork,this, cQueue.down);
          sched->registerInsertSubtask(ex_tcb::sWork, this, cQueue.up);
        }
    }

  if (getPool())
    getPool()->setStaticMode(FALSE);
}

ExWorkProcRetcode ex_tcb::workCancel() 
{
  // the default cancel work method calls the regular work method
  return work();
}

ExWorkProcRetcode ex_tcb::workResize() 
{
  queue_index newSize;
  queue_index maxSize;
  ex_queue *whichQueue;

  // go twice through this loop, once for the up queue and once
  // for the down queue
  for (Int32 i = 0; i < 2; i++)
    {
      // set some data that depends on the up/down queue
      if (i == 0)
	{
	  // point to the up queue
	  whichQueue = getParentQueue().up;
	  maxSize = getTdb()->getMaxQueueSizeUp();
	}
      else
	{
	  // point to the down queue
	  whichQueue = getParentQueue().down;
	  maxSize = getTdb()->getMaxQueueSizeDown();
	}

      // the following works the same for both up and down queue
      if (whichQueue->needsResize())
	{
	  // calculate the new size
	  newSize = whichQueue->getSize() * getTdb()->getQueueResizeFactor();
	  
	  // check whether the new size exceeds the max size
	  if (newSize > maxSize)
	    {
	      newSize = maxSize;
	    }

	  // is the new size larger than the current size?
	  if (newSize > whichQueue->getSize())
	    {
	      // yes, call the queue's resize method
	      whichQueue->resize(this, newSize);
	    }

	  if (whichQueue->getSize() >= maxSize ||
              newSize > whichQueue->getSize())
	    {
	      // Can't resize anymore, either because we exceeded
	      // the max size or because we're out of memory.
	      // Set the resize subtask to NULL so we don't repeatedly
	      // call this method.
	      whichQueue->setResizeSubtask(NULL);
	    }
	}
    }

  // this method never complains (but it may not always resize the queue)
  return WORK_OK;
}

Int32 ex_tcb::fixup()
{
  short error = 0;

  Int32 nc = numChildren();
  for (Int32 i=0; i<nc; i++)
  {
#pragma nowarn(1506)   // warning elimination 
     error = ((ex_tcb *)getChild(i))->fixup();
#pragma warn(1506)  // warning elimination 
     if (error)
        break;
  }

  return error;
}

// this method is called to rollback to a previously set savepoint.
Int32 ex_tcb::rollbackSavepoint()
{
  short error = 0;
  
  Int32 nc = numChildren();
  for (Int32 i=0; i<nc; i++)
    {
#pragma nowarn(1506)   // warning elimination 
      error = ((ex_tcb *)getChild(i))->rollbackSavepoint();
#pragma warn(1506)  // warning elimination 
      if (error)
        break;
    }
  
  return error;
}

// QSTUFF
void ex_tcb::propagateHoldable(NABoolean h)
{
  setHoldable(h);

  for (Int32 i=0; i<numChildren(); i++)
  {
    ((ex_tcb *)getChild(i))->propagateHoldable(h); 
  }
}
// QSTUFF

NABoolean ex_tcb::reOpenTables()
{
  return FALSE;
}

NABoolean ex_tcb::closeTables()
{
  return FALSE;
}

unsigned short ex_tcb::getExpressionMode() const
{ 
  if (globals_->getInjectErrorAtExpr())
#pragma nowarn(1506)   // warning elimination 
    return tdb.expressionMode_ | ex_expr::INJECT_ERROR; 
#pragma warn(1506)  // warning elimination 
  else
    return tdb.expressionMode_; 
};

void ex_tcb::allocateParentQueues(ex_queue_pair &parentQueues,
				  NABoolean allocatePstate)
{
  CollHeap     *space   = getGlobals()->getSpace();
  const ComTdb *tdb     = getTdb();
  queue_index  downSize = tdb->initialQueueSizeDown_;
  queue_index  upSize   = tdb->initialQueueSizeUp_;

  parentQueues.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
					  downSize,
					  tdb->criDescDown_,
					  space);

  if (allocatePstate)
    {
      // Allocate the private state in each entry of the down queue
      parentQueues.down->allocatePstate(this);
    }

  parentQueues.up = new(space) ex_queue(ex_queue::UP_QUEUE,
					upSize,
					tdb->criDescUp_,
					space);
}

void ex_tcb::registerResizeSubtasks()
{
  // Register two resize tasks with the scheduler, one for the
  // down queue and one for the up queue. Do this only if there
  // is potential for resizing (if the initial size is less
  // than the max size for the queue). Set the resize limit.

  ExScheduler *sched      = globals_->getScheduler();
  Lng32        resizeLimit = (Lng32) getTdb()->getQueueResizeLimit();
  ex_queue    *whichQueue;

  whichQueue = getParentQueue().up;
  if (whichQueue->getSize() < getTdb()->getMaxQueueSizeUp())
    {
      whichQueue->setResizeLimit(resizeLimit);
      sched->registerResizeSubtask(ex_tcb::sResize,this,whichQueue,"RS");
    }

  whichQueue = getParentQueue().down;
  if (whichQueue->getSize() < getTdb()->getMaxQueueSizeDown())
    {
      whichQueue->setResizeLimit(resizeLimit);
      sched->registerResizeSubtask(ex_tcb::sResize,this,whichQueue,"RS");
    }
}

ex_tcb_private_state * ex_tcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  // This method can't be implemented in the base class, every
  // derived TCB class needs to allocate an array of its own
  // pstate class and return the pointer.

  // The easiest way to redefine this method is to use the template
  // provided in the header file:
  //
  //  PstateAllocator<my pstate type name goes here> pa;
  //  return pa.allocatePstates(this, numElems, pstateLength);

  // NOTE: This is a new method that isn't implemented for
  // all TCBs yet. The current policy is that this method needs
  // to be implemented for a TCB if it resizes its queues dynamically.

  numElems     = 0;
  pstateLength = sizeof(ex_tcb_private_state);
  return NULL;
}

// this method find the first set of children in the child tree
// that have a valid stats area and sets their parent id to the
// input tdb id.
void ex_tcb::propagateTdbIdForStats(Lng32 tdbId)
{
  Int32 nc = numChildren();
  ExOperStats *stat, *currentStat;
  currentStat = getStatsEntry();
  for (Int32 i=0; i < nc; i++)
  {
    if (getChild(i))
    {
      stat = ((ex_tcb*)getChild(i))->getStatsEntry();
      if (stat != NULL)
      {
      // If the parent tdb id is already set in the by the doAllocateStatsEntry at the time 
      // of building the tcb, donot set parentTdbId, leftChildId and rightChildId
      // tdb ::build methods need to set all these Ids correctly
      // If the rightChildTdb is set to -2 then this function just resets the 
      // right child to -1 even there is a right child (conceptually) like in ex_split_top
        if (stat->getParentTdbId() == -1)
        {
	  stat->setParentTdbId(tdbId);

        }
        if (currentStat && currentStat->getParentTdbId() == -1)
        {
          if (i == 0)
            currentStat->setLeftChildTdbId(getChild(i)->getTdb()->getTdbId());
          if (i == 1)
          {
            if (currentStat->getRightChildTdbId() == -2)
              currentStat->setRightChildTdbId(-1);
            else
              currentStat->setRightChildTdbId(getChild(i)->getTdb()->getTdbId());
          }
        }
      }
    }
  }
}

// If statistics are to be collected and an entry for this
// node doesn't already exist in stats area passed in, 
// allocate one and add to it. Else, make pstate's stat pointer
// point to this entry.
void ex_tcb::allocateStatsEntry(CollHeap * heap) {
  
  // temporarily enable stats. Done so we can build stats related
  // structs, if asked for by the compiler.
  NABoolean statsEnabled = getGlobals()->statsEnabled();
  getGlobals()->setStatsEnabled(TRUE);
  
  if (getGlobals()->getStatsArea()) {
    
    // allocate a new stats entry
    ExOperStats* stat = NULL;
    if (needStatsEntry())
      stat = doAllocateStatsEntry(getGlobals()->getStatsArea()->getHeap(),
				  ((ComTdb *)getTdb()));
    if (stat)
      {
	// the queue sizes from the generator are just
	// estimates. Set the correct values here.
	if (getParentQueue().down) {
	  stat->setDownQueueSize(getParentQueue().down->getSize());
	  stat->setUpQueueSize(getParentQueue().up->getSize());
	}
	else {
	  stat->setDownQueueSize((queue_index) 0);
	  stat->setUpQueueSize((queue_index) 0); 
	};
	
	// add this entry to the stats area
	getGlobals()->getStatsArea()->insert(stat);
	
	setStatsEntry(stat);
      }

    // now allocate the stats entries of my children
    Int32 nc = numChildren();
    for (Int32 i = 0; i < nc; i++)
      {
	if (getChild(i) != NULL)
	  {
	    ex_tcb * childTcb = (ex_tcb*)getChild(i);
	    childTcb->allocateStatsEntry(getGlobals()->getStatsArea()->getHeap());
	  }
      }

    // assign my TDB id as parent ID to my children
    if (stat && nc)
      propagateTdbIdForStats(getTdb()->getTdbId());
  } // stats area present

  getGlobals()->setStatsEnabled(statsEnabled);
}

NABoolean ex_tcb::needStatsEntry()
{
  if ((getGlobals()->getStatsArea()->getCollectStatsType() == ComTdb::ALL_STATS) ||
      (getGlobals()->getStatsArea()->getCollectStatsType() == ComTdb::OPERATOR_STATS))
    return TRUE;
  else
    return FALSE;
}

ExOperStats * ex_tcb::doAllocateStatsEntry(CollHeap *heap, ComTdb *tdb)
{
  ComTdb::CollectStatsType statsType = getGlobals()->getStatsArea()->getCollectStatsType();
  if (statsType == ComTdb::OPERATOR_STATS || statsType == ComTdb::ALL_STATS)
    return new(heap) ExOperStats(heap,
			       ExOperStats::EX_OPER_STATS,
			       this,
			       tdb);
  else
  {
    ex_assert(FALSE, "doAllocateStatsEntry is not implemented by the TCB");
    return NULL;
  }
}

void ex_tcb::computeNeededPoolInfo(
     Int32 &numBuffs, 
     UInt32 &staticPoolSpaceSize, 
     UInt32 &dynPoolSpaceSize)
{
  numBuffs = getTdb()->numBuffers_;
  staticPoolSpaceSize = 0; 
  dynPoolSpaceSize = 0;
  if (getPool())
    {
      getPool()->getUsedMemorySize(staticPoolSpaceSize, dynPoolSpaceSize);
      
      dynPoolSpaceSize = numBuffs * getPool()->defaultBufferSize();
    }
}

void ex_tcb::mergeStats(ExStatisticsArea *otherStats)
{
  ex_globals * glob = getGlobals();
  StatsGlobals *statsGlobals = glob->getStatsGlobals();
  Long semId;
  if (statsGlobals != NULL)
  {
    semId = glob->getSemId();
    int error = statsGlobals->getStatsSemaphore(semId, glob->getPid());
    glob->getStatsArea()->merge(otherStats);
    statsGlobals->releaseStatsSemaphore(semId, glob->getPid());
  }
  else
    glob->getStatsArea()->merge(otherStats);
}

void ex_tcb::cpuLimitExceeded()
{
  ex_assert(0, "ex_tcb::cpuLimitExceeded not defined for this class.");
}

void ex_tcb::cleanup()
{
  Int32 nc = numChildren();
  for (Int32 i = 0; i < nc; i++)
    {
      ex_tcb * childTcb = (ex_tcb*)getChild(i);
      
      if (childTcb)
        {
          childTcb->cleanup();
        }
    }
}

short ex_tcb::moveRowToUpQueue(ex_queue_pair *qparent,
                               UInt16 tuppIndex,
                               const char * row, Lng32 len, 
                               short * rc, NABoolean isVarchar)
{
  if (qparent->up->isFull())
    {
      if (rc)
	*rc = WORK_CALL_AGAIN;
      return -1;
    }

  Lng32 length;
  if (len <= 0)
    length = strlen(row);
  else
    length = len;

  tupp p;
  if (pool_->get_free_tuple(p, (Lng32)
			    ((isVarchar ? SQL_VARCHAR_HDR_SIZE : 0)
			     + length)))
    {
      if (rc)
	*rc = WORK_POOL_BLOCKED;
      return -1;
    }
  
  char * dp = p.getDataPointer();
  if (isVarchar)
    {
      *(short*)dp = (short)length;
      str_cpy_all(&dp[SQL_VARCHAR_HDR_SIZE], row, length);
    }
  else
    {
      str_cpy_all(dp, row, length);
    }

  ex_queue_entry * pentry_down = qparent->down->getHeadEntry();
  ex_queue_entry * up_entry = qparent->up->getTailEntry();
  
  up_entry->copyAtp(pentry_down);
  up_entry->getAtp()->getTupp((Lng32)tuppIndex) = p;

  up_entry->upState.parentIndex = 
    pentry_down->downState.parentIndex;
  
  up_entry->upState.setMatchNo(0);
  up_entry->upState.status = ex_queue::Q_OK_MMORE;

  // insert into parent
  qparent->up->insert();

  return 0;
}

short ex_tcb::handleError(ex_queue_pair *qparent, ComDiagsArea *inDiagsArea)
{
  if (qparent->up->isFull())
    return 1;
  
  // Return EOF.
  ex_queue_entry * up_entry = qparent->up->getTailEntry();
  ex_queue_entry * pentry_down = qparent->down->getHeadEntry();
  
  up_entry->upState.parentIndex = 
    pentry_down->downState.parentIndex;
  
  up_entry->upState.setMatchNo(0);
  up_entry->upState.status = ex_queue::Q_SQLERROR;
  
  ComDiagsArea *diagsArea = up_entry->getDiagsArea();
  
  if (diagsArea == NULL)
    diagsArea = 
      ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
  else
    diagsArea->incrRefCount (); // the setDiagsArea below will decr the ref count
  
  if (inDiagsArea)
    diagsArea->mergeAfter(*inDiagsArea);
  
  up_entry->setDiagsArea (diagsArea);
  
  // insert into parent
  qparent->up->insert();
  
  return 0;
}

short ex_tcb::handleDone(ex_queue_pair *qparent, ComDiagsArea *inDiagsArea)
{
  if (qparent->up->isFull())
    return 1;
  
  // Return EOF.
  ex_queue_entry * up_entry = qparent->up->getTailEntry();
  ex_queue_entry * pentry_down = qparent->down->getHeadEntry();
  
  if (inDiagsArea && inDiagsArea->getNumber(DgSqlCode::WARNING_) > 0)
    {
      ComDiagsArea *diagsArea = up_entry->getDiagsArea();
      
      if (diagsArea == NULL)
	diagsArea = 
	  ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
      else
	diagsArea->incrRefCount (); // the setDiagsArea below will decr the ref count
      
      if (inDiagsArea)
	diagsArea->mergeAfter(*inDiagsArea);
      
      up_entry->setDiagsArea (diagsArea);
    }

  up_entry->upState.parentIndex = 
    pentry_down->downState.parentIndex;
  
  up_entry->upState.setMatchNo(0);
  up_entry->upState.status = ex_queue::Q_NO_DATA;
  
  // insert into parent
  qparent->up->insert();
  
  //	    pstate.matches_ = 0;
  qparent->down->removeHead();
  
  return 0;
}

__declspec(dllexport)
NABoolean ExExprComputeSpace(ex_tcb * tcb)

{
  return tcb->getGlobals()->computeSpace();
}

void ex_log_ems( const char *f, Int32 l, const char * m)
{
}
#pragma nowarn(770)   // warning elimination 
void assert_botch_longjmp( const char *f, Int32 l, const char * m)
{

}
#pragma warn(770)  // warning elimination 
