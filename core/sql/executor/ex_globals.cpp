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
 * File:         ex_globals.C
 * Description:  Base Class for executor statement globals
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
#include "ExScheduler.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExStats.h"
#include "ex_globals.h"
#include "Globals.h"
#include "SqlStats.h"
#include "ExpLOB.h"
#include "ExpLOBaccess.h"

ex_globals::ex_globals(short num_temps,
		       short create_gui_sched,
		       Space * space,
		       CollHeap * heap)
     : tcbList_(space),
       numTemps_(num_temps), 
       eventConsumedAddr_(NULL),
       tempList_(NULL),
       space_(space),
       heap_(heap),
       statsArea_(NULL),
       injectErrorAtExprFreq_(0),
       injectErrorAtQueueFreq_(0),
       flags_(0),
       planVersion_(0),
       sharedPool_(NULL),
       rowNum_(1),
       exLobGlobals_(NULL)
{
  // Small data items are allocated using space rather than heap so that
  // the allocation of memory for the heap can be avoided in simple queries.
  // This strategy has been applied to tcbList_, sch_ and tempList_.
  // tcbList_ really belongs on the heap because it can grow and shrink.
  // Placing it in space potentially wastes memory but is done because
  // it might be the only item in heap.
  sch_ = new(space_) ExScheduler(this);
  
  if (numTemps_ > 0)
    {
      tempList_ = (void **)(space_->allocateMemory(sizeof(void *) * num_temps));

      for (short i = 0; i < num_temps; i++)
	tempList_[i] = NULL;
    }

}

ExLobGlobals *&ex_globals::getExLobGlobal() 
{ 
  return exLobGlobals_;
}

void ex_globals::initLOBglobal(ContextCli *context, NABoolean useLibHdfs)
{
  exLobGlobals_ = ExpLOBoper::initLOBglobal((NAHeap *)heap_, context, useLibHdfs);
}

void ex_globals::reAllocate(short create_gui_sched)
{
  numTemps_ = 0;

  sch_ = new(space_) ExScheduler(this);

  tempList_ = NULL;
  
  tcbList_.allocate(0);
  exLobGlobals_ = NULL;
}

void ex_globals::deleteMe(NABoolean fatalError)
{
  delete sch_;
  sch_ = NULL;

  if (tempList_)
    space_->deallocateMemory(tempList_);
  tempList_ = NULL;

  if (statsArea_)
  {
    StatsGlobals *statsGlobals = getStatsGlobals();
    if (statsGlobals == NULL)
    {
      NADELETE(statsArea_, ExStatisticsArea, statsArea_->getHeap());
    }
    else
    {
      Long semId = getSemId();
      int error = statsGlobals->getStatsSemaphore(semId, getPid());
      NADELETE(statsArea_, ExStatisticsArea, statsArea_->getHeap());
      statsGlobals->releaseStatsSemaphore(semId, getPid());
    }
  }
  statsArea_ = NULL;
  cleanupTcbs();
  tcbList_.deallocate();
  if (exLobGlobals_ != NULL)
     ExpLOBoper::deleteLOBglobal(exLobGlobals_, (NAHeap *)heap_);
  exLobGlobals_ = NULL;
}

void ex_globals::deleteMemory(void *mem)
{
  //  ::operator delete(mem); // for now use global operator delete
}

void ex_globals::setNumOfTemps(Lng32 numTemps)
{
  numTemps_ = (short) numTemps;
  if (tempList_)
    space_->deallocateMemory(tempList_);
  if (numTemps <= 0)
    {
      tempList_ = NULL;
    }
  else
    {
      tempList_ = (void **) space_->allocateMemory(sizeof(void *) * numTemps_);
      for (short i = 0; i < numTemps_; i++)
	tempList_[i] = NULL;
    }
}

ExExeStmtGlobals * ex_globals::castToExExeStmtGlobals()
{
  return NULL;
}

ExEidStmtGlobals * ex_globals::castToExEidStmtGlobals()
{
  return NULL;
}

ExEspStmtGlobals * ex_globals::castToExEspStmtGlobals()
{
  return NULL;
}

/*
void * ex_globals::seqGen()
{
  if (castToExExeStmtGlobals())
    return castToExExeStmtGlobals()->seqGen();

  return NULL;
}
*/

void ex_globals::cleanupTcbs()
{
  const CollIndex numTcbs = tcbList_.entries();
  
  // Delete tcbs in FIFO order.  
  for (CollIndex i = 0; i < numTcbs; i++ )
    {
    const ex_tcb * aTcb = tcbList_[i];
    delete (ex_tcb *) aTcb;
    }
  tcbList_.clear();  
 }

void ex_globals::testAllQueues()
  {
  const CollIndex numTcbs = tcbList_.entries();

  for (CollIndex i = 0; i < numTcbs; i++ )
    {
    ex_queue_pair parentQ = tcbList_[i]->getParentQueue();
    if (parentQ.up &&
        !parentQ.up->isEmpty())
      {
      ex_assert(0, "Orphan entries in up queue!");
      }
    if (parentQ.down &&
        !parentQ.down->isEmpty())
      {
      ex_assert(0, "Orphan entries in down queue!");
      }
    }
  }

ExMeasStmtCntrs* ex_globals::getMeasStmtCntrs()
  { 
    return 0;
  }

