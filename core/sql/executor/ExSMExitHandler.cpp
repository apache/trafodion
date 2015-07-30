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

#include "sm.h"
#include "ExSMExitHandler.h"
#include "ExSMCommon.h"
#include "ExSMGlobals.h"
#include "ExSMEvent.h"
#include "ExSMShortMessage.h"

static uint32_t ExSM_ExitHandlerCount = 0;

void ExSM_ExitHandler(void)
{
  // SEAMONSTER PROJECT APR 2013
  // Given that this function is an exit handler and the process is
  // about to exit, there is no real need to call SM cancel or
  // finalize. SM cleanup will happen when the process goes away. We
  // decided to keep this function anyway because it might be a useful
  // building block in the future if SM semantics change or if the
  // process wants to stay alive after detaching from SM.
  
  int rc = 0;
  ExSMGlobals *smGlobals = ExSMGlobals::GetExSMGlobals();
  bool smInitialized = (smGlobals ? smGlobals->getSMInitialized() : false);

  EXSM_TRACE(EXSM_TRACE_EXIT,"BEGIN ExSM_ExitHandler count %d glob %p init %d",
             (int) ExSM_ExitHandlerCount, smGlobals, (int) smInitialized);

  // Do nothing if any of the following are true
  // * this function was already called
  // * SM globals is NULL
  // * SM was never successfully initialized
  if (ExSM_ExitHandlerCount > 0 || smGlobals == NULL || !smInitialized)
  {
    EXSM_TRACE(EXSM_TRACE_EXIT, "Called multiple times, nothing to do");
    EXSM_TRACE(EXSM_TRACE_EXIT, "END ExSM_ExitHandler");
    return;
  }

  ExSM_ExitHandlerCount++;

  // If this is not the reader thread, send a SHUTDOWN message to the
  // reader thread and wait for the reader thread to clean up
  if (!pthread_equal(pthread_self(), smGlobals->getReaderThreadTID()))
  {
    EXSM_TRACE(EXSM_TRACE_EXIT, "Sending SHUTDOWN msg to reader");

    sm_target_t target;
    memset(&target, 0, sizeof(target));
    target.node = smGlobals->getSQNodeNum();
    target.pid = smGlobals->getMainThreadPID();
    target.id = smGlobals->getExeInternalSMID();
    target.tag = 0;
  
    ExSMShortMessage m;
    m.setTarget(target);
    m.setNumValues(1);
    m.setValue(0, (int32_t) ExSMShortMessage::SHUTDOWN);
    m.send();

    // Wait for reader thread to react
    EXSM_TRACE(EXSM_TRACE_MAIN_THR|EXSM_TRACE_INIT, "BEGIN wait for reader");

    struct timespec waitTime;
    clock_gettime(CLOCK_REALTIME, &waitTime);
    waitTime.tv_sec += ExSMGlobals::READER_SHUTDOWN_WAIT;
    
    rc = pthread_mutex_lock(smGlobals->getReaderThreadStateLock());
    exsm_assert_rc(rc, "pthread_mutex_lock");

    // Next step is a timed wait on a condition variable until the
    // reader thread is done, or until the timer expires. The reader
    // thread has two states to indicate completion: DONE and
    // TERMINATED_DUE_TO_ERROR.
    
    rc = 0;
    ExSMGlobals::ThreadState rstate = smGlobals->getReaderThreadState();

    while (rstate != ExSMGlobals::DONE &&
           rstate != ExSMGlobals::TERMINATED_DUE_TO_ERROR &&
           rc == 0)
    {
      rc = pthread_cond_timedwait(smGlobals->getReaderThreadStateCond(),
                                  smGlobals->getReaderThreadStateLock(),
                                  &waitTime);

      rstate = smGlobals->getReaderThreadState();
      EXSM_TRACE(EXSM_TRACE_MAIN_THR|EXSM_TRACE_INIT,
                 "pthread_cond_timedwait returned %d reader state %s", rc,
                 smGlobals->getThreadStateString(rstate));
    }
    
    rc = pthread_mutex_unlock(smGlobals->getReaderThreadStateLock());
    exsm_assert_rc(rc, "pthread_mutex_unlock");
    
    EXSM_TRACE(EXSM_TRACE_MAIN_THR|EXSM_TRACE_INIT, "END wait for reader");

  } // if not reader thread

  // This call will cancel the special ID used for executor internal
  // communcation and then call SM_finalize
  ExSM_Finalize(smGlobals);
  
  EXSM_TRACE(EXSM_TRACE_EXIT, "END ExSM_ExitHandler");
}
