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
 * File:         ExScheduler.cpp
 * Description:  Task and subtask scheduler.
 *               
 *               
 * Created:      9/22/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include <dlfcn.h>  
#include "Platform.h"
#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "exp_expr.h"
#include "ExScheduler.h"
#include "ExStats.h"
#include "ex_error.h"

// wait method need more include files
#include "ex_exe_stmt_globals.h"
#include "Ipc.h"
#include "ex_frag_rt.h"
#include "ex_root.h"

#include "ExCextdecs.h"
#include "ComRtUtils.h"
#include "ComSqlcmpdbg.h"

const char *TraceDesc = "SubTask state trace in Scheduler";

// -----------------------------------------------------------------------
// Methods for class ExScheduler
// -----------------------------------------------------------------------

ExScheduler::ExScheduler(ex_globals *glob)
{
  glob_               = glob;
  subtasks_           = NULL;
  tail_               = NULL;
  exceptionSubtask_   = NULL;
  externalEventCompleted_ = FALSE;
  rootStats_          = NULL;
  msGui_              = FALSE;
  lastCalledIdx_      = -1;
  maxCpuTime_         = LLONG_MAX;
  suspended_          = false;
  subtaskLoopCnt_     = 32;
  maxSubtaskLoops_    = 32;
  localRootTcb_       = NULL;
  pExpFuncs_          = NULL;
  Int32 i;
  for (i = 0; i < NumLastCalled; i++)
  {
     subtaskTrace_[i].lastCalledTask_ = NULL;
     subtaskTrace_[i].lastWorkRetcode_ = 777;
     #ifdef TRACE_DP2_CPU_LIMIT
       subtaskTrace_[i].dp2TimeConsumed_ = -1;
       subtaskTrace_[i].dp2QidCummulative_ = 0;
     #endif
     subtaskTrace_[i].rmsTimeConsumed_ = -1;
  }
  exeTrace_ = 0;
}

ExScheduler::~ExScheduler()
{
  if (exeTrace_ && glob_->castToExExeStmtGlobals()->getCliGlobals())
  {
    ExeTraceInfo *ti = glob_->castToExExeStmtGlobals()->getCliGlobals()
                            ->getExeTraceInfo();
    ti->removeTrace(exeTrace_);
  }
}

Int32 ExScheduler::printALiner(Int32 lineno, char *buf)
{
  if (lineno >= NumLastCalled)
    return 0;
  Int32 rv = 0;
  ExSubtask *st = subtaskTrace_[lineno].lastCalledTask_;
  if (!st) // not used entry
    rv = sprintf(buf, "%.4d  -                        -  -\n", lineno);
  else
    rv = sprintf(buf, "%.4d  %.4s.%.2s(%8p) %6d  " PF64 "\n", lineno,
                 (st->getTcb()? st->getTcb()->getEyeCatcher(): "????"),
                 (st->getTaskName()? st->getTaskName(): "??"), st,
                 subtaskTrace_[lineno].lastWorkRetcode_,
                 subtaskTrace_[lineno].rmsTimeConsumed_);
  return rv;
}

ExWorkProcRetcode ExScheduler::work(Int64 prevWaitTime)
{
  ex_assert(glob_, "global pointer in scheduler is invalid");
   
  ExSubtask *subtask = subtasks_;
  Int32 listHadWork      = 0;
  Int32 noWorkDone       = TRUE;
  Int32 poolsBlocked     = 0;
  Int32 prevPoolsBlocked = 0;
  ExWorkProcRetcode rc = WORK_OK;
  ExOperStats *stats  = NULL;
  ExStatisticsArea *statsArea = glob_->getStatsArea();
  Int64 incCpuTime;
  NABoolean doNotUpdateCounter = FALSE;
  Space *space = glob_->getSpace();
  CollHeap *heap = glob_->getDefaultHeap();
  ExTimeStats *timer = NULL;
#ifdef NA_DEBUG_GUI
  ExSubtask *subtaskSetInGui = NULL;
#endif

  if (suspended_)
    return WORK_OK;

  if (exeTrace_ == 0 && glob_->castToExExeStmtGlobals()->getCliGlobals())
  {
    ExeTraceInfo *ti = glob_->castToExExeStmtGlobals()->getCliGlobals()
                            ->getExeTraceInfo();
    if (ti)
    { 
      Int32 lineWidth = 58; // temp
      void *regdTrace;
      Int32 ret = ti->addTrace("Scheduler", this, NumLastCalled, 3,
                               this, getALine,
//                               &subtaskTrace_[0],
                               &lastCalledIdx_,
                               lineWidth, TraceDesc, &regdTrace);
      if (ret == 0)
      {
        // trace info added successfully, now add entry fields
        ti->addTraceField(regdTrace, "TCB.NM(subtask ptr)", 0,
                          ExeTrace::TR_POINTER32);
        ti->addTraceField(regdTrace, "RetCode", 1, ExeTrace::TR_INT32);
        ti->addTraceField(regdTrace, "TimeConsumed", 2, ExeTrace::TR_INT64);
        exeTrace_ = (ExeTrace*) regdTrace;
      }
    }
  }

  ExOperStats *rootStats = NULL;
  ExFragRootOperStats *fragRootOperStats = NULL;
  ExMeasStats *measStats = NULL;
  
  rootStats = getRootStats();
  if (rootStats == NULL)
  {
    if (statsArea)
    {
      rootStats_ = statsArea->getRootStats();
      rootStats = rootStats_;
      fragRootOperStats = rootStats->castToExFragRootOperStats();
      measStats = rootStats->castToExMeasStats();
    }
  }
  else
  {
    fragRootOperStats = rootStats->castToExFragRootOperStats();
    measStats = rootStats->castToExMeasStats();
  }

  ExProcessStats *processStats;
  if (fragRootOperStats != NULL)
  {
     fragRootOperStats->incWaitTime(prevWaitTime);
     processStats = GetCliGlobals()->getExProcessStats();
     if (processStats != NULL)
         processStats->setRecentQid(fragRootOperStats->getQueryId());
  }     

  // ---------------------------------------------------------------------
  // Loop through all events that are managed by this scheduler
  // ---------------------------------------------------------------------
  while (subtask)
    {
      if (subtask->isScheduled())
      {
	  // -------------------------------------------------------------
	  // scheduled subtask, reset it and execute the work proc
	  // -------------------------------------------------------------

#ifdef NA_DEBUG_GUI

	  //------------------------------------------------------
	  // If using the GUI dll then use the 
	  // appropriate dll function to display the TCB tree.
	  //------------------------------------------------------
	  if (msGui_)
	    {
	      subtaskSetInGui = subtask;

	      pExpFuncs_->fpDisplayExecution(&subtaskSetInGui, this);

              if (subtaskSetInGui == subtask)
                // GUI did not alter the subtask
                subtaskSetInGui = NULL;
	    }
#endif

	  // reset the trigger BEFORE calling the work procedure, so that
	  // events don't get lost if they happen during execution of the
	  // work procedure
	  subtask->reset();
          // Prepare for the possibility of collecting detailed
	  // statistics. Record the start time and the current
	  // statistics entry for this work task.
	  //
          if (subtask->tcb_)
            stats = subtask->tcb_->getStatsEntry();
          else
            stats = NULL;
          if (stats && statsArea)
            timer = stats->getTimer();
          else
            timer = (rootStats ? rootStats->getTimer() : NULL);

          if (statsArea)
            doNotUpdateCounter = statsArea->donotUpdateCounters();
          if ((timer) && (NOT doNotUpdateCounter))
             timer->start();

          noWorkDone = FALSE;
          if (++lastCalledIdx_ >= NumLastCalled)
            lastCalledIdx_ = 0;
#ifdef TRACE_DP2_CPU_LIMIT
          QueryIDStats subtaskCpuTime;
          Int64 startQidCpuTime;
          if (maxCpuTime_ != LLONG_MAX)
            startQidCpuTime =
              subtaskCpuTime.getProcessBusyTime();
#endif

          if (stats)
            stats->incNumberCalls();
          else
          if (rootStats != NULL)
            rootStats->incNumberCalls();

          subtaskTrace_[lastCalledIdx_].lastCalledTask_ = subtask;

          // This next trace field is not valid until the subtask
          // returns, so initialize it here to a known invalid 
          // value so that it won't be misinterpreted in a dump
          // taken before the subtask has returned.
          subtaskTrace_[lastCalledIdx_].lastWorkRetcode_ = 666;

	  // Do the work
	  //
          rc = subtask->work();
          subtaskTrace_[lastCalledIdx_].lastWorkRetcode_ = rc;
        
          #ifdef TRACE_DP2_CPU_LIMIT
            if (maxCpuTime_ != LLONG_MAX)
            {
              subtaskTrace_[lastCalledIdx_].dp2QidCummulative_ = 
                subtaskCpuTime.getProcessBusyTime();
              subtaskTrace_[lastCalledIdx_].dp2TimeConsumed_ = 
                subtaskTrace_[lastCalledIdx_].dp2QidCummulative_ - 
                startQidCpuTime;
            }
          #endif
        
        if (timer)
          {
            doNotUpdateCounter = statsArea->donotUpdateCounters();
            if (NOT doNotUpdateCounter)
            {
              incCpuTime = timer->stop();
              subtaskTrace_[lastCalledIdx_].rmsTimeConsumed_ = incCpuTime;
              if (fragRootOperStats)
              {
                  fragRootOperStats->incCpuTime(incCpuTime);
                  fragRootOperStats->updateSpaceUsage(space, heap);
              }
              else
              if (measStats)
              {
                 measStats->incCpuTime(incCpuTime);
                 measStats->updateSpaceUsage(space, heap);
              }
            }
          }

          if (stats && (stats->getCollectStatsType() == ComTdb::ALL_STATS))
          {
            // If detailed statistics are being collected, record
	    // return code, queue utilizations and sizes for this TCB
	    //
	    stats->subTaskReturn(rc);

	    // collect stats about the queues
	    if (stats->getDownQueueSize() ||
	        stats->getUpQueueSize())
            {
              ex_queue_pair pq = subtask->tcb_->getParentQueue();
              if (pq.down != NULL)
              {
                stats->getDownQueueStats().addEntry(
                                           (double)pq.down->getLength());
                stats->setDownQueueSize(pq.down->getSize());
              }
              if (pq.up != NULL)
              {
                stats->getUpQueueStats().addEntry(
                                         (double)pq.up->getLength());
                 stats->setUpQueueSize(pq.up->getSize());
              }
            }
          } // ALL_STATS
          
          // Handle return code from work procedure
	  //
	  if (rc != WORK_OK)
	    {
              // return codes other than OK cause the work procedure
              // to be rescheduled.
              subtask->schedule();

	      if (rc == WORK_POOL_BLOCKED)
		{
		  // Work procedure indicates that it needs free pool
		  // space to continue. Some work may have been done,
		  // but the listHadWork flag is not set for this case.
		  // The scheduler will do one more round through all
		  // tasks to see whether some of them were scheduled.
		  // If all tasks through that round reply with
		  // WORK_POOL_BLOCKED then return from the scheduler
		  // just as if there were no active subtasks.
		  poolsBlocked++;
		}
              else if (rc == WORK_HASHJ_PREEMPT)
		{
                  // Hash join needs to check if CPU limit exceeded, or if
                  // Suspend or Cancel has been sent.
                  listHadWork = 1;
		}
	      else
		{
		  // Other return codes cause the scheduler to exit
                   if (rc != WORK_CALL_AGAIN)
		    {
		      // reset flag (may not have to but can't hurt)
		      externalEventCompleted_ = FALSE;


                      // Special case for cancel: master must check for I/O
                      // completion here.
                      ExMasterStmtGlobals *masterGlob = glob_->
                        castToExExeStmtGlobals()->castToExMasterStmtGlobals();

                      if ((masterGlob) && 
                          (masterGlob->getRtFragTable()->getState() == 
                           ExRtFragTable::NO_ESPS_USED) 
                        )
                      {
                        glob_->castToExExeStmtGlobals()->getIpcEnvironment()->
                              getAllConnections()->waitOnAll(IpcImmediately);

                      }

		      return rc;
		    }

		  // WORK_CALL_AGAIN means we have done work.
		  listHadWork = 1;
		}
	    }
	  else
	    {
	      // we have done some work
	      listHadWork = 1;
	    }
        }

      // advance to the next subtask in the list
      // (some day we may have a smarter way of searching for scheduled
      // subtasks)

      subtask = subtask->getNext();

#ifdef NA_DEBUG_GUI
      if (msGui_ && subtaskSetInGui && subtaskSetInGui->getNext() != subtask)
        {
          // if the user clicked on a task in the GUI, then
          // schedule and execute that task next
          subtask = subtaskSetInGui;
          subtask->schedule();
          subtaskSetInGui = NULL;
          // the GUI changed the subtask to be executed, set
          // an indicator that we have done work, otherwise
          // the scheduler might exit because it thinks that
          // it has finished an entire round through all subtasks
          listHadWork = 1;
        }
#endif
      
      // -----------------------------------------------------------------
      // Each time we reach the end of the list, check whether any of the
      // work procedures got invoked. If not, then we did not do any
      // work during the last round and therefore no subtasks could have
      // been scheduled. Return to the caller so it can work on other
      // schedulers and wait for external events.
      // -----------------------------------------------------------------
      if (subtask == NULL && !suspended_)
	{
	  if (listHadWork)
	    {
	      // reached end of linked events list, and did work during this
	      // round, therefore start over at the beginning
	      subtask = subtasks_;
	      listHadWork = 0;
	      poolsBlocked = 0;
	      prevPoolsBlocked = 0;

	      // poll each active connection once for each cycle through
	      // the scheduled subtasks
	      glob_->castToExExeStmtGlobals()->getIpcEnvironment()->
		getAllConnections()->waitOnAll(IpcImmediately);
	    }
	  else if (poolsBlocked)
	    {
	      // all work procedures in this round have returned
	      // WORK_POOL_BLOCKED, if this has happened more than
	      // once then return
	      if (poolsBlocked == prevPoolsBlocked)
		{
		  // Note that if any blocked procedures unblock they will
		  // return a code other then WORK_BLOCKED  or trigger
		  // another subtask and therefore set the listHadWork flag.
		  // This means that if we do two rounds and all work
		  // procedures return WORK_POOL_BLOCKED twice and the
		  // number of active subtasks does not change between the
		  // first and second round, then we know that nothing
		  // will happen until we have an external event.
		  // reset flag
		  externalEventCompleted_ = FALSE;
                  return WORK_POOL_BLOCKED;
		}

	      // The situation has not stabilized yet, this time
	      // it's not the same number of work procedures that
	      // have returned WORK_POOL_BLOCKED the last time
	      prevPoolsBlocked = poolsBlocked;
	      poolsBlocked = 0;
	      subtask = subtasks_;
          }  // if poolsBlocked

          if (rootStats)
          {
            if ((subtaskLoopCnt_ == maxSubtaskLoops_) ||
                (rc == WORK_HASHJ_PREEMPT))
            {
              checkSuspendAndLimit();
              subtaskLoopCnt_ = 1;
            }
            else
              subtaskLoopCnt_++;
          }
        }  // if subtask == NULL
  } // While Loop
  // reset flag
  externalEventCompleted_ = FALSE;

  return (noWorkDone ? WORK_NOBODY_WORKED : WORK_OK );
}

void ExScheduler::registerInsertSubtask(ExWorkProcPtr subtask,
					ex_tcb *tcb,
					ex_queue *queue,
					const char *taskName)
{
  ExSubtask *st = addOrFindSubtask(subtask,tcb,taskName);
  queue->setInsertSubtask(st);
}

void ExScheduler::registerUnblockSubtask(ExWorkProcPtr subtask,
					 ex_tcb *tcb,
					 ex_queue *queue,
					 const char *taskName)
{
  ExSubtask *st = addOrFindSubtask(subtask,tcb,taskName);
  queue->setUnblockSubtask(st);
}

void ExScheduler::registerCancelSubtask(ExWorkProcPtr subtask,
					ex_tcb *tcb,
					ex_queue *queue,
					const char *taskName)
{
  ExSubtask *st = addOrFindSubtask(subtask,tcb,taskName);
  queue->setCancelSubtask(st);
}

// BertBert VV
void ExScheduler::registerNextSubtask(ExWorkProcPtr subtask,
					ex_tcb *tcb,
					ex_queue *queue,
					const char *taskName)
{
  ExSubtask *st = addOrFindSubtask(subtask,tcb,taskName);
  queue->setNextSubtask(st);
}
// BertBert ^^


void ExScheduler::registerResizeSubtask(ExWorkProcPtr subtask,
					ex_tcb *tcb,
					ex_queue *queue,
					const char *taskName)
{
  ExSubtask *st = addOrFindExceptionSubtask(subtask,tcb,taskName,glob_);
  queue->setResizeSubtask(st);
}

ExSubtask * ExScheduler::registerNonQueueSubtask(ExWorkProcPtr subtask,
						 ex_tcb *tcb,
						 const char *taskName)
{
  return addOrFindSubtask(subtask,tcb,taskName);
}

Int32 ExScheduler::hasActiveEvents(ex_tcb *tcb)
{
  ExSubtask *subtask = subtasks_;

  while (subtask)
    {
      if (subtask->tcb_ == tcb && subtask->scheduled_)
	// found an active event for this tcb
	return 1;

      subtask = subtask->getNext();
    }
  return 0; // tcb has no active events
}

///////////////////////////////////////////////////////////////////////////
// Method to set up the executor GUI. This method can also be
// called from the debugger.
///////////////////////////////////////////////////////////////////////////
void ExScheduler::startGui()
{
  msGui_ = TRUE;

  if (!pExpFuncs_ && getenv("DISPLAY"))
    {
      void* dlptr = dlopen("libSqlCompilerDebugger.so",RTLD_NOW);
      if(dlptr != NULL)
        {
          fpGetSqlcmpdbgExpFuncs GetExportedFunctions;

          GetExportedFunctions = (fpGetSqlcmpdbgExpFuncs) dlsym(
               dlptr, "GetSqlcmpdbgExpFuncs");
          if (GetExportedFunctions)
            pExpFuncs_ = GetExportedFunctions();
          if (pExpFuncs_ == NULL)
            dlclose(dlptr);
        }
      else // dlopen() failed 
        { 
          char *msg = dlerror(); 
        }
      msGui_ = (pExpFuncs_ != NULL);
    }
}

void ExScheduler::stopGui()
{
  msGui_ = FALSE;
  if (pExpFuncs_)
    pExpFuncs_->fpDisplayExecution(NULL, this);
}

void ExScheduler::getProcInfoForGui(int &frag,
                                    int &inst,
                                    int &numInst,
                                    int &nid,
                                    int &pid,
                                    char *procNameBuf,
                                    int procNameBufLen)
{
  ExExeStmtGlobals *glob = glob_->castToExExeStmtGlobals();
  MyGuaProcessHandle myh;
  Int32 myCpu, myPin, myNodeNum;
  SB_Int64_Type mySeqNum;

  frag = glob->getMyFragId();
  inst = glob->getMyInstanceNumber();
  numInst = glob->getNumOfInstances();
  myh.decompose(myCpu, myPin, myNodeNum, mySeqNum);
  nid = myCpu;
  pid = myPin;
  myh.toAscii(procNameBuf, procNameBufLen);
}

int ExScheduler::getFragInstIdForGui()
{
  ExExeStmtGlobals *stmtGlobals = glob_->castToExExeStmtGlobals();

  ex_assert(stmtGlobals, "GUI not in master or ESP");

  ExMasterStmtGlobals *masterGlobals = stmtGlobals->castToExMasterStmtGlobals();
  ExEspStmtGlobals *espGlobals = stmtGlobals->castToExEspStmtGlobals();

  if (masterGlobals)
    // assume there is only one active fragment instance in the master
    return 0;
  ex_assert(espGlobals, "stmt globals in GUI inconsistent");

  return espGlobals->getMyFragInstanceHandle();
}

ExSubtask * ExScheduler::addOrFindSubtask(
     ExWorkProcPtr workProc,
     ex_tcb        *tcb,
     const char    *taskName)
{
  // Loop through existing subtasks to see whether this one
  // has already been added
  ExSubtask *result = subtasks_;

  while (result &&
	 (! (result->matches(workProc,tcb))))
    result = result->getNext();

  if (!result)
    {
      // didn't find a duplicate, so add a new subtask
      result = new(glob_->getSpace()) ExSubtask(this,workProc,tcb,taskName);
      if (tail_)
	{
	  tail_->insertAfter(result);
	  tail_ = result;
	}
      else
	{
	  subtasks_ = result;
	  tail_     = subtasks_;
	}
    }

  return result;
}

ExExceptionSubtask * ExScheduler::addOrFindExceptionSubtask(
       ExWorkProcPtr workProc,
       ex_tcb        *tcb,
       const char    *taskName,
       ex_globals    *glob)
{
  if (!exceptionSubtask_)
    {
      // the first exception subtask to be added, create the
      // root subtask (which is the only one that gets scheduled)
      exceptionSubtask_ = new(glob->getSpace()) ExExceptionSubtask(this,"EX");
      if (tail_)
	{
	  tail_->insertAfter(exceptionSubtask_);
	  tail_ = exceptionSubtask_;
	}
      else
	{
	  subtasks_ = exceptionSubtask_;
	  tail_     = subtasks_;
	}
    }

  exceptionSubtask_->addOrFindSubtaskEntry(workProc,tcb,taskName,glob);

  // always return the root subtask
  return exceptionSubtask_;
}

NABoolean ExScheduler::checkSuspendAndLimit()
{
  ExFragRootOperStats *fragRootStats = NULL;
  ExMeasStats *measRootStats = NULL;
  Int64 localCpuTime = 0;

  if (rootStats_) 
  {
    bool isFragSuspended = false;

    if ((fragRootStats = rootStats_->castToExFragRootOperStats()) != NULL)
      isFragSuspended = fragRootStats->isFragSuspended();
   
    if ((measRootStats = rootStats_->castToExMeasStats()) != NULL)
      isFragSuspended = measRootStats->isFragSuspended();

    while (isFragSuspended)
    {
          DELAY(100);
      if (fragRootStats)
        isFragSuspended = fragRootStats->isFragSuspended();
      else
        isFragSuspended = measRootStats->isFragSuspended();
    }
    
    // Query limits: 
    if (maxCpuTime_ == LLONG_MAX) 
    {
      ; // Don't evaluate CPU time, if there is no limit.
    }
    else
    {
      if (fragRootStats)
        localCpuTime = fragRootStats->getLocalCpuTime();
      if (measRootStats)
        localCpuTime = measRootStats->getLocalCpuTime();

      if (localCpuTime > maxCpuTime_) 
      {
        localRootTcb_->cpuLimitExceeded();
        return TRUE;
      }
    }
  }

  return FALSE;
}

ExWorkProcRetcode ExSubtask::work()
{
  // dereference the function pointer and pass it the TCB pointer as
  // an argument
  return workProc_(tcb_);
}

ExOperStats * ExScheduler::getRootStats()
{
  return (glob_->statsEnabled() ? rootStats_ : NULL);
}

void ExScheduler::setRootTcb(ex_tcb *tcb)
{ 
  localRootTcb_ = tcb; 
}

const char *ExWorkProcRetcodeToString(ExWorkProcRetcode r)
{
  switch (r)
  {
    case WORK_OK:                    return "WORK_OK";
    case WORK_CALL_AGAIN:            return "WORK_CALL_AGAIN";
    case WORK_POOL_BLOCKED:          return "WORK_POOL_BLOCKED";
    case WORK_BAD_ERROR:             return "WORK_BAD_ERROR";
    case WORK_RESCHEDULE_AND_RETURN: return "WORK_RESCHEDULE_AND_RETURN";
    case WORK_NOBODY_WORKED:         return "WORK_NOBODY_WORKED";
    case WORK_STATS_PREEMPT:         return "WORK_STATS_PREEMPT";
    case WORK_HASHJ_PREEMPT:         return "WORK_HASHJ_PREEMPT";
    default: return ComRtGetUnknownString((Int32) r);
  }
}

// -----------------------------------------------------------------------
// Methods for class ExExceptionSubtask
// -----------------------------------------------------------------------

void ExExceptionSubtask::addOrFindSubtaskEntry(ExWorkProcPtr workProc,
					 ex_tcb        *tcb,
					 const char    *taskName,
					 ex_globals    *glob)
{
  ExExceptionSubtaskEntry *result = entryList_;

  while (result &&
	 (! (result->matches(workProc,tcb))))
    {
      result = result->next_;
    }

  if (!result)
    {
      ExExceptionSubtaskEntry *e =
        new(glob->getSpace()) ExExceptionSubtaskEntry(
	     workProc, tcb, taskName);

      e->next_   = entryList_;
      entryList_ = e;
    }
}

ExWorkProcRetcode ExExceptionSubtask::work()
{
  ExWorkProcRetcode result = WORK_OK;

  ExExceptionSubtaskEntry *e = entryList_;

  // loop through the list of entries and call the work methods
  // associated with each entries (note that individual entries
  // can not be scheduled individually)
  while (e)
    {
      ExWorkProcRetcode r;

      r = e->workProc_(e->tcb_);

      if (r != WORK_OK)
	{
	  // Note that we are not done yet checking all of the tasks
	  // in the list. However, we know that the scheduler reschedules
	  // a task if it returns a code other than WORK_OK, so that we'll
	  // have another chance to visit the rest of the subtasks.
	  return r;
	}

      e = e->next_;
    }
  return result;
}
