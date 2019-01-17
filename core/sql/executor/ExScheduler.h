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
#ifndef EX_SCHEDULER_H
#define EX_SCHEDULER_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExScheduler.h
 * Description:  Task and subtask scheduler.
 *               The ExScheduler class manages a set of subtasks. Each subtask
 *               is defined by a TCB and does some work related to that
 *		 TCB. A subtask consists of a function pointer to a work
 *		 procedure and a pointer to a TCB. Subtasks can be activated
 *		 via events, e.g. by queue operations or asynchronous I/O.
 * Created:      9/22/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ex_god.h"
#include "ComSqlcmpdbg.h"
#include "Platform.h"

#include "ComExeTrace.h"

// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------

class ExScheduler;
class ExSubtask;
class ExExceptionSubtaskEntry;
class ExExceptionSubtask;

// -----------------------------------------------------------------------
// Forward references
// -----------------------------------------------------------------------

class ex_tcb;
class ex_globals;
class ExFragRootOperStats;
class ExOperStats;
class ExTimeStats;


// -----------------------------------------------------------------------
// Typedef for return codes of subtask work procedures. In DP2, the return
// codes are partially generated and interpreted by the DP2 environment,
// therefore the error codes come from the Guardian error number space.
// The actual type is therefore an int, not an enum.
// -----------------------------------------------------------------------

enum WorkProcRetcodeEnum
{
  WORK_OK	    =  0, // OK, all work is done (same as FEOK)
  WORK_CALL_AGAIN   = -1, // reschedule work procedure, not done yet
  WORK_POOL_BLOCKED = -2, // wait for pool space to free up
  WORK_BAD_ERROR    = -3, // An internal error occurred during work
  WORK_RESCHEDULE_AND_RETURN = -4, // reschedule work procedure, but 
                                   // return control to the caller of
                                   // scheduler with this return code.
  WORK_NOBODY_WORKED         = -5, // Can be used for loop detection.
  WORK_STATS_PREEMPT         = -6, // If signaled for a statistics preemption
  WORK_HASHJ_PREEMPT         = -7, // HJ preempts to scheduler.

  // if a retcode is added to the enum, make sure that WORK_LAST_RETCODE
  // is changed accordingly. This value is used to set up and index an
  // array in a StatsEntry to keep track of the return codes.
  // We allocate an array of -WORK_LAST_RETCODE + 1 entries!
  WORK_LAST_RETCODE = WORK_HASHJ_PREEMPT
};

typedef short ExWorkProcRetcode;

const char *ExWorkProcRetcodeToString(ExWorkProcRetcode r);

// -----------------------------------------------------------------------
// Typedef for subtask work procedures triggered by an event
// This is a non-member function taking a TCB pointer as input
// and returning a value of type ExWorkProcRetcode. TCBs will have to
// define a static member function that looks like this:
//     static ExWorkProcRetcode myWorkProc(ex_tcb *);
// -----------------------------------------------------------------------

typedef ExWorkProcRetcode (*ExWorkProcPtr)(ex_tcb *);

// -----------------------------------------------------------------------
// Subtask tracing. Usually 8 are plenty.  To debug DP2 CPU Limits feature,
// it is helpful to have more.  Getting CPU times is expensive on DP2, so
// do not do it by default.
// -----------------------------------------------------------------------

const Int32 NumLastCalled = 8;

struct TraceEntry {
  ExSubtask         *lastCalledTask_;
  ExWorkProcRetcode  lastWorkRetcode_;
  #ifdef TRACE_DP2_CPU_LIMIT
    Int64              dp2TimeConsumed_;
    Int64              dp2QidCummulative_;
  #endif
  Int64              rmsTimeConsumed_;
};

// -----------------------------------------------------------------------
// class ExScheduler
// -----------------------------------------------------------------------

class ExScheduler : public ExGod
{

public:

  ExScheduler(ex_globals *glob);
  ~ExScheduler();

  // ---------------------------------------------------------------------
  // The work procedure of a scheduler is called by the root node of
  // a fragment in the master executor or in DP2, and by the ESP main
  // program. The work procedure returns when there is nothing more to
  // do. The work procedure does not wait for external events.
  // ---------------------------------------------------------------------
  ExWorkProcRetcode work(Int64 prevWaitTime = 0);

  // ---------------------------------------------------------------------
  // At build time, TCBs register their work procedures with the
  // scheduler. Work (subtask) procedures can be registered for multiple
  // events. Each call to one of the methods below registers the work
  // procedure for one event. Possible events:
  //
  // - A new entry is inserted into a queue for which a TCB is the reader,
  // - A queue into which the TCB inserts changes from being full to not
  //   being full (unblock),
  // - An cancel operation occurs on a queue for which a TCB is a reader.
  // - An event defined by the caller by scheduling the subtask via a
  //   procedure call.
  //
  // Only the TCB mentioned in the event description may register its
  // subtask procedures for the event. Only one subtask procedure can
  // be registered for a given event.
  //
  // Insert, unblock, and cancel events are triggered via queue
  // operations, NonQueue events are triggered explicitly by calling
  // the schedule() procedure for the ExSubtask object.
  //
  // Work procedure return a status code to the scheduler. Depending on
  // the return code, the scheduler will do the following:
  // - WORK_OK:		  An event needs to be triggered before this work
  //			  procedure is called again. The scheduler assumes
  //			  that currently there is no more work to be done.
  // - WORK_CALL_AGAIN:	  The work procedure is scheduled again for
  //			  execution. The triggering event is not reset.
  // - WORK_POOL_BLOCKED: This task is blocked because it cannot allocate
  //                      space in its sql_buffer_pool. Keep on scheduling
  //                      the subtask until we have space. The work procedure
  //                      will continue to return WORK_POOL_BLOCKED until
  //                      space frees up. The work procedure must not produce
  //                      or consume queue entries until more space frees up.
  // - WORK_ERROR:        Not all work may be done. Work procedure is returning
  //                      due to an error. Treated the same as WORK_CALL_AGAIN.
  // - WORK_BAD_ERROR:	  This work procedure is not called again. The error
  //			  indicates an internal inconsistency in the TCB.
  // - all others:	  The scheduler returns with the given return code
  //			  to its caller
  // ---------------------------------------------------------------------
  void registerInsertSubtask(ExWorkProcPtr subtask,
					ex_tcb *tcb,
					ex_queue *queue,
					const char *taskName = NULL);
  void registerUnblockSubtask(ExWorkProcPtr subtask,
					 ex_tcb *tcb,
					 ex_queue *queue,
					 const char *taskName = NULL);
  void registerCancelSubtask(ExWorkProcPtr subtask,
					ex_tcb *tcb,
					ex_queue *queue,
					const char *taskName = NULL);
  // BertBert VV
  void registerNextSubtask(ExWorkProcPtr subtask,
					ex_tcb *tcb,
					ex_queue *queue,
					const char *taskName = NULL);
  // BertBert ^^
  void registerResizeSubtask(ExWorkProcPtr subtask,
					ex_tcb *tcb,
					ex_queue *queue,
					const char *taskName = NULL);
  ExSubtask * registerNonQueueSubtask(ExWorkProcPtr subtask,
						 ex_tcb *tcb,
						 const char *taskName = NULL);

  // ---------------------------------------------------------------------
  // Acessor methods
  // ---------------------------------------------------------------------

  Int32 hasActiveEvents(ex_tcb *tcb);

  inline NABoolean externalEventCompleted(void)
    { return externalEventCompleted_; };

  // ---------------------------------------------------------------------
  // The following method allows an ExSubtask to flag the scheduler as
  // having an external event completed (that may give the scheduler some
  // useful work to do). (It would be desireable to have this method
  // declared in such a way that only ExSubtask can call it, but 
  // unfortunately the only way to do that is to make ExSubtask a 
  // friend of this class, giving it access to everything, since the
  // definition of ExSubtask follows this class.)
  // ---------------------------------------------------------------------

  inline void setExternalEventCompleted(void)
    { externalEventCompleted_ = TRUE; };

  void setRootTcb(ex_tcb *tcb);

  // ---------------------------------------------------------------------
  // Query limits: next three methods support the ability to raise an error
  // if a query takes too much of a ESP or master's CPU time.  These are 
  // defined also for exe-in-dp2, though the feature is not used there.
  // ---------------------------------------------------------------------
  void setMaxCpuTime(Int64 m) { maxCpuTime_ = m * 1000L * 1000L; }

  void setCpuLimitCheckFreq(Int32 m) { 
    maxSubtaskLoops_ = m; 
    subtaskLoopCnt_ = maxSubtaskLoops_;    // A trick to ensure we check CPU 
                                           // time on the the first iteration.
  }

  NABoolean checkSuspendAndLimit();

  // ---------------------------------------------------------------------
  // Method to start the GUI
  // (this method can also be called from the debugger)!!!
  // ---------------------------------------------------------------------
  void startGui();
  void stopGui();
  void getProcInfoForGui(int &frag, int &inst, int &numInst,
                         int &nid, int &pid, char *procNameBuf,
                         int procNameBufLen);
  ExSubtask *getSubtasksForGui() { return subtasks_; }
  ex_tcb *getLocalRootTcbForGui() { return localRootTcb_; }
  int getFragInstIdForGui();

  // ---------------------------------------------------------------------
  // Method to aid in diagnosing looping problems
  // ---------------------------------------------------------------------

  inline ExSubtask * getLastCalledTask(void) const 
    { return (lastCalledIdx_ == -1 ? 0 : 
              subtaskTrace_[lastCalledIdx_].lastCalledTask_);}

  // ---------------------------------------------------------------------
  // Method to aid executor tracing
  // ---------------------------------------------------------------------
  Int32 printALiner(Int32 lineno, char *buf);
  static Int32 getALine(void * mine, Int32 lineno, char * buf)
               { return ((ExScheduler *) mine)->printALiner(lineno, buf); }

private:

  // remember where we get the space from
  ex_globals         *glob_;

  // list of events managed by this scheduler
  ExSubtask          *subtasks_;

  // a convenient pointer to the last event
  ExSubtask          *tail_;

  // helps with debugging loops
  TraceEntry          subtaskTrace_[NumLastCalled];
  Int32                 lastCalledIdx_;
  // helps with executor tracing
  ExeTrace            *exeTrace_;

  // list of rarely used subtasks (could keep multiple of these later)
  ExExceptionSubtask *exceptionSubtask_;

  // a flag for use by no-wait completion logic -- It is set to true
  // whenever an external event completes that might give this 
  // scheduler some useful work to do. It is reset whenever the
  // scheduler exits with no more work to do.
  NABoolean externalEventCompleted_;

  // pointer to the root operator's stats, if we need to collect
  // CPU and elapsed time measurements
  ExOperStats * rootStats_;

  // data members for executor GUI ONLY

  // should the GUI be displayed?
  NABoolean msGui_;
  // function pointers for GUI
  SqlcmpdbgExpFuncs *pExpFuncs_;
  // root TCB
  ex_tcb *localRootTcb_;

  // Query execution limits - CPU time for this fragment instance.
  // On DP2, this is max for all sessions of the query in this process.
  Int64 maxCpuTime_;

  // Flag to tell scheduler to exit and let caller wait for reactivation.
  bool suspended_;

  // Query execution limits - max times through the subtask list
  // before evaluating CPU limit.
  Int32 subtaskLoopCnt_;
  Int32 maxSubtaskLoops_;

  // private methods

  // add another subtask or share an already existing one
  ExSubtask * addOrFindSubtask(ExWorkProcPtr workProc,
					  ex_tcb        *tcb,
					  const char    *taskName);
  
  // add another exception subtask or share an already existing one
  ExExceptionSubtask * addOrFindExceptionSubtask(
       ExWorkProcPtr workProc,
       ex_tcb        *tcb,
       const char    *taskName,
       ex_globals    *glob);

  ExOperStats *getRootStats();
};

// -----------------------------------------------------------------------
// Subtasks are objects that are kept in the scheduler. They don't have
// public constructors in order to keep it that way. The scheduler
// searches through its subtasks for scheduled ones and calls their
// work procedures. Each TCB can register one or more subtasks with the
// scheduler and can link subtasks to queue and non-queue events.
// -----------------------------------------------------------------------

class ExSubtask
{
  friend class ExScheduler;

public:

  inline ex_tcb * getTcb() const         { return tcb_; }
  inline Int32 isScheduled() const         { return scheduled_; }
  inline void schedule()                    { scheduled_ = 1; }
  inline void scheduleAndNoteCompletion()           
    { scheduled_ = 1; scheduler_->setExternalEventCompleted(); }

  // for GUI
  //
  inline Int32 getBreakPoint() const      { return breakPoint_; }
  inline void setBreakPoint(Int32 val)     { breakPoint_ = val; }  
  inline const char * getTaskName() const   { return taskName_; }
  inline Int32 * getScheduledAddr()       { return &scheduled_; }
  inline ExSubtask *getNextForGUI()             { return next_; }

protected:

  ExSubtask( 
       ExScheduler * scheduler,
       ExWorkProcPtr workProc,
       ex_tcb *tcb,
       const char *taskName) {
    scheduler_ = scheduler;
    scheduled_ = 0; workProc_ = workProc; tcb_ = tcb;
    taskName_ = taskName; next_ = NULL; breakPoint_ = 0;
  }

  inline NABoolean matches(ExWorkProcPtr workProc, ex_tcb *tcb) const
                        { return (workProc_ == workProc && tcb_ == tcb); }

  inline void setTcb(ex_tcb *tcb)               { tcb_ = tcb; }  

private:

  // pointer to the ExScheduler that created (and dispatches) this subtask
  ExScheduler * scheduler_; 

  // an indicator whether the subtask is to be scheduled or not (boolean)
  Int32           scheduled_;

  // pointer to the work procedure that gets scheduled when the event is
  // triggered
  ExWorkProcPtr workProc_;

  // pointer to the TCB that registered the subtask
  ex_tcb        *tcb_;

  // events are kept in (intrusive) linked lists
  ExSubtask     *next_;

  // **	**  information for GUI  *** -------------
  Int32           breakPoint_;

  const char    *taskName_;

  // private methods

  virtual ExWorkProcRetcode work();
  inline void reset()                       { scheduled_ = 0; }
  inline ExSubtask *getNext()                 { return next_; }
  inline void insertAfter(ExSubtask *newNext)
                              { newNext->next_ = next_; next_ = newNext; }

};

class ExExceptionSubtask : public ExSubtask
{
public:
 
   ExExceptionSubtask(
       ExScheduler * scheduler,
       const char *taskName) : ExSubtask(scheduler, NULL, NULL, taskName),
	 entryList_(NULL) {}

  void addOrFindSubtaskEntry(ExWorkProcPtr workProc,
				  ex_tcb        *tcb,
				  const char    *taskName,
				  ex_globals    *glob);

  virtual ExWorkProcRetcode work();

private:

  ExExceptionSubtaskEntry * entryList_;
};

class ExExceptionSubtaskEntry
{
  friend class ExExceptionSubtask;

public:

  ExExceptionSubtaskEntry( 
       ExWorkProcPtr workProc,
       ex_tcb *tcb,
       const char *taskName) :
       workProc_(workProc), tcb_(tcb), next_(NULL), taskName_(taskName) {}

private:

  // pointer to the work procedure that gets scheduled when the event is
  // triggered
  ExWorkProcPtr workProc_;

  // pointer to the TCB that registered the subtask
  ex_tcb        *tcb_;

  // events are kept in (intrusive) linked lists
  ExExceptionSubtaskEntry * next_;

  // the name is mostly for debugging
  const char    *taskName_;
  
  inline Int32 matches(ExWorkProcPtr workProc, ex_tcb *tcb) const
                        { return (workProc_ == workProc && tcb_ == tcb); }
};



#endif /* EX_SCHEDULER_H */





