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
 * File:         ex_esp_frag_dir.cpp
 * Description:  Fragment instance directory in the ESP
 *
 * Created:      1/22/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------


#include "Platform.h"
#include <stdlib.h>
#include <sys/syscall.h>
#include "ex_stdh.h"
#include "ex_exe_stmt_globals.h"
#include "ex_esp_frag_dir.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_split_bottom.h"
#include "ex_send_bottom.h"
#include "ComSpace.h"
#include "ComDiags.h"
#include "LateBindInfo.h"
#include "NAHeap.h"
#include "NAMemory.h"
#include "ExpError.h"
#include "Globals.h"
#include "SqlStats.h"
#include "ComRtUtils.h"
#include "PortProcessCalls.h"
#include "ExStats.h"
#include "ExSMTrace.h"
#include <errno.h>
#include "Context.h"
#include <semaphore.h>
#include "ExSMCommon.h"
#include "ExSMEvent.h"
#include "ExSMGlobals.h"
#include "ExSMShortMessage.h"
#include "ComExeTrace.h"
#include "Context.h"

// -----------------------------------------------------------------------
// Methods for class ExEspFragInstanceDir
// -----------------------------------------------------------------------

ExEspFragInstanceDir::ExEspFragInstanceDir(CliGlobals *cliGlobals,
					   NAHeap *heap,
                                           StatsGlobals *statsGlobals)
  : instances_(heap),
    cliGlobals_(cliGlobals),
    heap_(heap),
    statsGlobals_(statsGlobals),
    userIDEstablished_(FALSE),
    localStatsHeap_(NULL)
{
  // If this is not Linux, database user ID is the same as process ID
  // so our database identity is already established

  numActiveInstances_ = 0;
  highWaterMark_      = 0;
  numMasters_         = 1;
  int error;

  //Phandle wrapper in porting layer
  NAProcessHandle phandle;

  phandle.getmine();
  phandle.decompose();
  cpu_ = phandle.getCpu();
  pid_ = phandle.getPin();

  tid_ = syscall(SYS_gettid);
  NABoolean reportError = FALSE;
  char msg[256];;
  if ((statsGlobals_ == NULL)
     || ((statsGlobals_ != NULL) &&  (statsGlobals_->getInitError(pid_, reportError))))
  {
    if (reportError) {
         snprintf(msg, sizeof(msg), 
          "Version mismatch or Pid %d,%d is higher than the configured pid max %d",
           cpu_, pid_, statsGlobals_->getConfiguredPidMax()); 
         SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg, 0);
    }
    statsGlobals_ = NULL;

    statsHeap_ = new (heap_) 
        NAHeap("Process Stats Heap", (NAHeap *)heap_,
        8192,
        0);
    semId_ = -1;
  }
  else
  {
    error = statsGlobals_->openStatsSemaphore(semId_);
    // Behave like as if stats is not available
    if (error != 0)
    {
        statsGlobals_ = NULL;
        statsHeap_ = (NAHeap *)heap_;
    }
    else
    {
      cliGlobals_->setStatsGlobals(statsGlobals_);
      cliGlobals_->setSemId(semId_);
      error = statsGlobals_->getStatsSemaphore(semId_, pid_);
      statsHeap_ = (NAHeap *)statsGlobals->getStatsHeap()->allocateHeapMemory(sizeof *statsHeap_);
      statsHeap_ = new (statsHeap_, statsGlobals->getStatsHeap()) 
        NAHeap("Process Stats Heap", statsGlobals->getStatsHeap(),
        8192,
        0);
      // We need to set up the cliGlobals, since addProcess will call getRTSSemaphore
      // and it uses these members
      cliGlobals_->setStatsHeap(statsHeap_);
      statsGlobals_->addProcess(pid_, statsHeap_);
      ExProcessStats *processStats = 
           statsGlobals_->getExProcessStats(pid_);
      processStats->setStartTime(cliGlobals_->myStartTime());
      cliGlobals_->setExProcessStats(processStats);
      statsGlobals_->releaseStatsSemaphore(semId_, pid_);
    }
  }
  cliGlobals_->setStatsHeap(statsHeap_);
  cliGlobals_->setStatsGlobals(statsGlobals_);
  cliGlobals_->setSemId(semId_);

  // Fragment instance trace
  fiTidx_ = 0;
  initFiStateTrace();
  // add the instance trace to global trace repository
  traceRef_ = NULL;
  ExeTraceInfo *ti = cliGlobals_->getExeTraceInfo();
  if (ti)
    {
      Int32 lineWidth = 42; // temp
      void *regdTrace;
      char traceDesc[80];
      sprintf(traceDesc, "Trace plan fragment instance state changes");
      Int32 ret = ti->addTrace("FragmentInstance", this, NumFiTraceElements, 3,
                               this, getALine,
                               &fiTidx_,
                               lineWidth, traceDesc, &regdTrace);
      if (ret == 0)
      {
        // trace info added successfully, now add entry fields
        ti->addTraceField(regdTrace, "FragInst#", 0,
                          ExeTrace::TR_INT32); 
        ti->addTraceField(regdTrace, "State             ", 1, ExeTrace::TR_STRING);
        ti->addTraceField(regdTrace, "Line#", 2, ExeTrace::TR_INT32);
        traceRef_ = (ExeTrace*) regdTrace;
      }
    }
}

ExEspFragInstanceDir::~ExEspFragInstanceDir()
{
  // if we come here we are exiting or abending from the process,
  // no point in making error checks
  if (statsGlobals_ != NULL)
  {
    int error = statsGlobals_->getStatsSemaphore(semId_, pid_);
    statsGlobals_->removeProcess(pid_);
    statsGlobals_->releaseStatsSemaphore(semId_, pid_);
    sem_close((sem_t *)semId_);
  }
}

ExFragInstanceHandle ExEspFragInstanceDir::findHandle(
					    const ExFragKey &key) const
{
  for (CollIndex i = 0; i < highWaterMark_; i++)
    {
      if (instances_.used(i) && instances_[i]->key_ == key)
	return instances_[i]->handle_;
    }
  return NullFragInstanceHandle;
}

ExFragInstanceHandle ExEspFragInstanceDir::addEntry(ExMsgFragment *msgFragment,
						    IpcConnection *connection)
{
  ExFragInstanceHandle result = instances_.unusedIndex();
  ExEspFragInstance *inst = new(heap_) ExEspFragInstance;
  const ExFragKey &key = msgFragment->getKey();

  inst->key_           = key;
  inst->handle_        = result;
  inst->fragType_      = msgFragment->getFragType();
  inst->parentKey_     = ExFragKey(key.getProcessId(),
				   key.getStatementHandle(),
				   msgFragment->getParentId());
  inst->controlConn_   = connection;
  inst->topNodeOffset_ = msgFragment->getTopNodeOffset();
  inst->msgFragment_   = msgFragment;
  inst->localRootTdb_  = (ex_split_bottom_tdb *)
                           (msgFragment->getFragment() + inst->topNodeOffset_);
  inst->mxvOfOriginator_ = msgFragment->getMxvOfOriginator();
  inst->planVersion_   = msgFragment->getPlanVersion();
  inst->queryId_ = (char *) msgFragment->getQueryId();
  inst->queryIdLen_ = msgFragment->getQueryIdLen();
  inst->localRootTcb_  = NULL;


  if (msgFragment->getFragType() == ExFragDir::ESP)
    {
      NAHeap *fragHeap = new(heap_) NAHeap("ESP Fragment Heap",
					   (NAHeap *) heap_,32768);
      Space * fragSpace = new(fragHeap) Space(Space::EXECUTOR_SPACE);
      fragSpace->setParent(fragHeap);

      // allocate the globals in their own heap, like the master
      // executor does it
      inst->globals_ = new(fragHeap) ExEspStmtGlobals(
	   (short) msgFragment->getNumTemps(),
	   cliGlobals_,
	   msgFragment->getDisplayInGui(),
	   fragSpace,
	   fragHeap,
	   this,
	   inst->handle_,
	   msgFragment->getInjectErrorAtExpr(),
           inst->queryId_,
           inst->queryIdLen_);
    }
  else
    inst->globals_ = NULL; // no globals needed for DP2 fragments

  inst->numSendBottomRequests_ = 0;
  inst->numSendBottomCancels_  = 0;
  inst->numLateCancelRequests_ = 0;
  inst->displayInGui_  = msgFragment->getDisplayInGui();

  instances_.insertAt(result,inst);
  setFiState(result, DOWNLOADED, __LINE__);
  if (result >= highWaterMark_)
    highWaterMark_ = result + 1;
  return result;
}

void ExEspFragInstanceDir::fixupEntry(ExFragInstanceHandle handle,
				      Lng32 numOfParentInstances,
				      ComDiagsArea &da)
{
  ExEspFragInstance *entry = NULL;
  FragmentInstanceState entryState = UNUSED;

  if (handle != NullFragInstanceHandle AND instances_.used(handle))
    {
      entry = instances_[handle];
      entryState = entry->fiState_;
    }

  switch (entryState)
    {
    case DOWNLOADED:
      {
        if (checkPlanVersion(entry, da)) 
        {
           // ERROR: there was a plan versioning error
           break;    
        }
        ComTdb *rootTdb = entry->localRootTdb_;

        // Set up reallocation space for unpacking. Use the space managed
        // by the globals of the entry.
        //
        //rootTdb->setReallocator(entry->globals_->getSpace());
        void *base = (void *)entry->msgFragment_->getFragment();

        ComTdb dummyTdb;
        if ( (rootTdb = (ex_split_bottom_tdb *)
              rootTdb->driveUnpack(base,&dummyTdb,
				   entry->globals_->getSpace())) == NULL )
        {
          // ERROR during unpacking. Most likely case is verison-unsupported.
          //
          entryState = setFiState(handle, BROKEN, __LINE__);
          break;
        }
        else
        {
          // The root tdb might have been relocated after unpacking due to a
          // version upgrade.
          //
          entry->localRootTdb_ = (ex_split_bottom_tdb *)(rootTdb);
          entryState = setFiState(handle, UNPACKED, __LINE__);
 
          // continue to next case
        }
      }
    case UNPACKED:
      entry->localRootTcb_ =
	entry->localRootTdb_->buildESPTcbTree(entry->globals_,
					      this,
					      entry->key_,
					      entry->parentKey_,
					      handle,
					      numOfParentInstances);
      entry->globals_->takeGlobalDiagsArea(da);
      if (da.mainSQLCODE() >= 0)
	{
	  entryState = setFiState(handle, BUILT, __LINE__);
	}
      else
      // if there is any error, do not change the state to BROKEN but
      // break out so that the executor master will wait and receive
      // replies from all ESPs.   (CR # 10-020919-5026)
        break;

      // continue to next case
      
    case BUILT:
    case FIXED_UP:
    case READY_INACTIVE:
      entry->localRootTcb_->fixup();
      entry->globals_->takeGlobalDiagsArea(da);

      if (da.mainSQLCODE() < 0)
	// Fixup failed, let master executor redrive when it receives
	// the diags area
	entryState = setFiState(handle, BUILT, __LINE__);
      else 
        // Statement is now definitely fixed up, it stays ready
        // for work if it was ready before the fixup.
        if (entryState == BUILT)
          entryState = setFiState(handle, FIXED_UP, __LINE__);
      break; // leave the switch here, rest is error handling
      
    case ACTIVE:
    case BROKEN:
    case UNUSED:
    default:
      ex_assert(FALSE, "Wrong state for downloaded frag");
      // this is a serious error, don't use this statement any more
      if (entry)
	entryState = setFiState(handle, BROKEN, __LINE__);
      // $$$$ set Diagnostics area
    }
}

void ExEspFragInstanceDir::openedSendBottom(
     ExFragInstanceHandle handle)
{
  if (instances_[handle]->fiState_ == FIXED_UP)
    {
      // now that we are linked up (at least partially)
      // with our clients, we are ready to do work
      setFiState(handle, READY_INACTIVE, __LINE__);
    }
}

// The following "started" and "finshed" methods are partially 
// responsible for handling the normal ESP fragment instance 
// state transitions of 
//    READY_INACTIVE -> ACTIVE -> RELEASING_WORK -> READY_INACTIVE
// Specifically, these methods handle the first two transitions.
// See ExEspFragInstanceDir::work for the code handling the final
// transition: RELEASING_WORK -> READY_INACTIVE.  

void ExEspFragInstanceDir::startedSendBottomRequest(
     ExFragInstanceHandle handle)
{
  ex_assert(instances_.used(handle),
	    "handle: ExEspFragInstanceDir::startedSendBottomRequest");

  instances_[handle]->numSendBottomRequests_++;

  if (instances_[handle]->fiState_ == READY_INACTIVE)
    {
      ex_assert(instances_[handle]->numSendBottomRequests_ == 1,
		"count: ExEspFragInstanceDir::startedSendBottomRequest");
      setFiState(handle, ACTIVE, __LINE__);
      numActiveInstances_++;
    }
}

void ExEspFragInstanceDir::finishedSendBottomRequest(
     ExFragInstanceHandle handle)
{
  ex_assert(instances_.used(handle),
	    "handle: ExEspFragInstanceDir::finishedSendBottomRequest");
  ex_assert(instances_[handle]->numSendBottomRequests_ > 0,
	    "requests: ExEspFragInstanceDir::finishedSendBottomRequest");

  instances_[handle]->numSendBottomRequests_--;

  finishedRequest(handle, TRUE);
}

void ExEspFragInstanceDir::startedSendBottomCancel(
     ExFragInstanceHandle handle)
{
  ex_assert(instances_.used(handle),
	    "handle: ExEspFragInstanceDir::startedSendBottomCancel");

  instances_[handle]->numSendBottomCancels_++;

  if (instances_[handle]->fiState_ == READY_INACTIVE)
    {
      ex_assert(instances_[handle]->numSendBottomCancels_ == 1,
		"count: ExEspFragInstanceDir::startedSendBottomCancel");
      setFiState(handle, ACTIVE, __LINE__);
      numActiveInstances_++;
    }
}

void ExEspFragInstanceDir::finishedSendBottomCancel(
     ExFragInstanceHandle handle)
{
  ex_assert(instances_.used(handle),
	    "handle: ExEspFragInstanceDir::finishedSendBottomCancel");
  ex_assert(instances_[handle]->numSendBottomCancels_ > 0,
	    "requests: ExEspFragInstanceDir::finishedSendBottomCancel");

  instances_[handle]->numSendBottomCancels_--;

  finishedRequest(handle);
}

void ExEspFragInstanceDir::startedLateCancelRequest(
     ExFragInstanceHandle handle)
{
  ex_assert(instances_.used(handle),
	    "handle: ExEspFragInstanceDir::startedLateCancelRequest");

  instances_[handle]->numLateCancelRequests_++;
}

void ExEspFragInstanceDir::finishedLateCancelRequest(
     ExFragInstanceHandle handle)
{
  ex_assert(instances_.used(handle),
	    "handle: ExEspFragInstanceDir::finishedLateCancelRequest");
  ex_assert(instances_[handle]->numLateCancelRequests_ > 0,
	    "requests: ExEspFragInstanceDir::finishedLateCancelRequest");

  instances_[handle]->numLateCancelRequests_--;

  finishedRequest(handle);
}

// This method handles the ACTIVE->RELEASING_WORK transition that 
// happens when the ESP runs out of work to do.  The RELEASING_WORK
// state might be only temporary, e.g., if this ESP is on the right
// side of a flow node and will get more probes later.
// The other way to transition ACTIVE->RELEASING_WORK is 
// for split_bottom to get a ESP_RELEASE_TRANSACTION_HDR message.
// Even in this case, the RELEASING_WORK state might be temporary,
// for example it might be to allow a temporary suspension of the 
// transaction.
void ExEspFragInstanceDir::finishedRequest(
     ExFragInstanceHandle handle,
     NABoolean testAllQueues)
{
  if (instances_[handle]->numSendBottomRequests_ <= 0 &&
      instances_[handle]->numLateCancelRequests_ <= 0 &&
      instances_[handle]->numSendBottomCancels_  <= 0 &&
      instances_[handle]->fiState_ == ACTIVE            &&
      instances_[handle]->globals_->anySendTopMsgesOut() == FALSE)
    {
      // if all requests from client are finished or canceled, and
      // and if there are no pending messages to servers (these can
      // be eager send top continue messages that haven't got the empty 
      // reply yet), and if this entry is still in the
      // active state, then signal to the ESP fragment directory to
      // deactivate this fragment and to reply to its work request.
      setFiState(handle, RELEASING_WORK, __LINE__);

      if (testAllQueues)
        instances_[handle]->localRootTcb_->testAllQueues();
    }
}

Lng32 ExEspFragInstanceDir::numLateCancelRequests(ExFragInstanceHandle handle)
{
  return instances_[handle]->numLateCancelRequests_;
}

void ExEspFragInstanceDir::releaseEntry(ExFragInstanceHandle handle)
{
  ExEspFragInstance *entry;

  if (handle != NullFragInstanceHandle AND instances_.used(handle))
    {
      entry = instances_[handle];
    }
  else
    {
      // error
      ex_assert(FALSE,"Fragment instance not found");
      return;
    }

  // loop again over all entries to remove the DP2 fragments that were
  // used by this one
  ExFragId fragId = entry->key_.getFragId();
  
  for (CollIndex i = 0; i < highWaterMark_; i++)
    {
      if (instances_.used(i) AND
	  instances_[i]->fragType_ == ExFragDir::DP2 AND
	  instances_[i]->parentKey_ == entry->key_)
	{
	  setFiState(i, RELEASING, __LINE__);
	}
    }
  
  // now remove the entry itself
  setFiState(handle, RELEASING, __LINE__);
}

void ExEspFragInstanceDir::releaseOrphanEntries()
{
  for (CollIndex i = 0; i < highWaterMark_; i++)
    {
      if (instances_.used(i) AND
	  NOT instances_[i]->controlConn_->isConnected())
	{
	  setFiState(i, RELEASING, __LINE__);
	}
    }
}

void ExEspFragInstanceDir::hasTransidReleaseRequest(
     ExFragInstanceHandle handle)
{
  ExEspFragInstance *entry;

  if (handle != NullFragInstanceHandle AND instances_.used(handle))
    {
      entry = instances_[handle];
    }
  else
    {
      // error
      ex_assert(FALSE,"Fragment instance not found");
      return;
    }

  // - at beginning of query execution, master sends a work request msg to
  // all the esps. The esp does not reply to the work msg but instead leave it
  // pending. Then at end of query master sends a release work msg to all the
  // esps. Upon receiving the release work msg, the esp will reply to release
  // work msg plus replying to the work msg saved earlier. However, during
  // query execution, it is possible that an esp may find all of its queues
  // empty and thus has no work left to do. in the past we allow esp to do an
  // early reply to the work msg even if it has not received the release work
  // msg from master. When esp does early reply to the work msg, depends on
  // timing on the master side, two things could happen:
  //
  // - master sends another work request msg to esp. esp will save it and
  // won't reply until it receives release work msg from master.
  //
  // - master receives EOD from all top level esps. Then master sends release
  // work msg to all esps EXCEPT the esp that has already replied to the work
  // msg. i.e., in this case the esp that does early reply to work msg will
  // not receive a release work msg from master.
  //
  // now we don't allow esp early reply to work msg any more. this means that
  // even if an esp has no work left to do, it will sit idle until it receives
  // the release work msg from master and then reply to both work msg and
  // release work msg. all esps are guaranteed to receive a release work msg
  // (except in error situations).
  //
  assert(entry->localRootTcb_->hasTransaction());
  entry->localRootTcb_->releaseWorkRequest();

  // set entry's state to RELEASING_WORK. note that we may still have some
  // work left with other esps (such as cancel request), but not with master
  // since we have already replied to master's work request above.
  if (entry->fiState_ == WAIT_TO_RELEASE)
    setFiState(handle, RELEASING_WORK, __LINE__);
  else
    // 1. the path where finishedRequest() is called before esp receives
    //    release work msg, or
    // 2. no consumer esp opened this esp as server.
    assert(entry->fiState_ == RELEASING_WORK ||
	   entry->fiState_ == READY_INACTIVE ||
	   entry->fiState_ == FIXED_UP);

  // if esp inactive timeout is turned on, then start the
  // inactive timestamp counter for this esp.
  //
  // note that since we don't allow esp sharing, each esp
  // can be used by only one statement. thus esp becomes
  // inactive if it receives the release work msg.
  //
  if (getEnvironment()->getInactiveTimeout() > 0)
    getEnvironment()->setInactiveTimestamp();
}

void ExEspFragInstanceDir::hasReleaseRequest(
     ExFragInstanceHandle handle)
{
  
  ex_assert((handle != NullFragInstanceHandle) && instances_.used(handle),
           "Fragment instance not found");
  
 ExEspFragInstance *entry = instances_[handle];

 setFiState(handle, WAIT_TO_RELEASE, __LINE__);
}



void ExEspFragInstanceDir::work(Int64 prevWaitTime)
{
  ULng32 startSeqNo;

  NABoolean transactionRestored; 

  // The following DO loop can be forced to execute again by setting
  // loopAgain to TRUE
  NABoolean loopAgain = FALSE;

  do
    {
      loopAgain = FALSE;

      // -----------------------------------------------------------------
      // remember the I/O sequence number when we start
      // -----------------------------------------------------------------
      startSeqNo =
	getEnvironment()->getAllConnections()->getCompletionSeqenceNo();

      // -----------------------------------------------------------------
      // Call the work() method once for each active instance.
      // NOTE: if the instances can wake each other up, then they must
      // artificially bump the external I/O completion count
      // (see, for example, the local send top/bottom nodes).
      // -----------------------------------------------------------------
      for (CollIndex currInst = 0; currInst < highWaterMark_; currInst++)
	{

	  if (instances_.used(currInst))
            {
	    switch (instances_[currInst]->fiState_)
	      {
	      case RELEASING_WORK:
		// - an esp instance can become RELEASING_WORK state from
		// two paths:
		//
		// 1. finishedRequest(): esp has no work left to do. all queues
		// are empty. but master has not sent the release work msg
		// yet. note that in the past esp will do an early reply to
		// master's work request. then depending on the timings
		// master may send another work request or master may not
		// do anything further with this esp (not even a release work
		// msg). but that is not the case any more. esp no longer
		// does early reply to the work request. instead, even if
		// esp has no work to do, it will just sit idle until master
		// sends release work msg. so esp will always receives an
		// release work msg from master.
		//
		// 2. hasTransidReleaseRequest(): esp receives release work
		// msg from master.
		//

		// dependent on whether the TCB tree has some requests from
		// send bottom nodes or not, set its state to ACTIVE or
		// READY_INACTIVE (restoreTransaction() will prevent
		// active fragment instances from working when they don't
		// have a work message)
		if ((instances_[currInst]->numSendBottomRequests_ > 0) ||
                    (instances_[currInst]->numSendBottomCancels_  > 0)  ||
                    (instances_[currInst]->numLateCancelRequests_ > 0)
                   )
		  {
		    setFiState(currInst, ACTIVE, __LINE__);
		  }
		else
		  {
		    setFiState(currInst, READY_INACTIVE, __LINE__);
		    // decrement the global number of active fragment
		    // instances per ESP
		    numActiveInstances_--;

		    // we shouldn't need to work on this fragment instance
		    // anymore
		    break;
		  }

		// fall through to the next case

	      case ACTIVE:

#ifdef NA_DEBUG_GUI
		if (instances_[currInst]->displayInGui_ == 1)
		  instances_[currInst]->globals_->getScheduler()->startGui();
#endif
		// To help debugging (dumps): Put current SB TCB in cli globals
		cliGlobals_->setRootTcb(instances_[currInst]->localRootTcb_);

		// -------------------------------------------------------
		// Call the scheduler work procedure for this fragment
		// instance, but not before restoring its transid.
		// -------------------------------------------------------

                transactionRestored = 
                       instances_[currInst]->globals_->restoreTransaction();
                if ( transactionRestored ||
                        // Allow working without a transaction, otherwise
                        // "continue" messages might never get their replies.
                     (instances_[currInst]->numSendBottomRequests_ > 0)   ||
                        // The test below covers the case where the work
                        // request finished and released the transaction
                        // before the cancel request was received.
                     (instances_[currInst]->numSendBottomCancels_  > 0)   ||
                     (instances_[currInst]->numLateCancelRequests_ > 0)
                   )
		  {
                    if (!transactionRestored)
                    {
                       // transactionRestored is FALSE if a transid is required
                       // for this request to work, but the transid is currently
                       // -1, i.e., no work request has been sent to this ESP
                       // yet. In this case, no new requests should be posted to
                       // DP2 (solution #10-060628-7424). 
                       instances_[currInst]->globals_->setNoNewRequest(TRUE);
                    }
 
		    // -----------------------------------------------------
		    // This is where we do the actual work
		    // -----------------------------------------------------
                    ExWorkProcRetcode retcode = 
		      instances_[currInst]->globals_->getScheduler()->work(prevWaitTime);

		    if (retcode == WORK_BAD_ERROR)
                    {
		      setFiState(currInst, GOING_FATAL, __LINE__);

                      // Force the loop to execute again so that error
                      // processing can continue
                      loopAgain = TRUE;
                    }
                  }
#ifdef NA_DEBUG_GUI
		if (instances_[currInst]->fiState_ != ACTIVE &&
                    instances_[currInst]->displayInGui_ == 1)
		  instances_[currInst]->globals_->getScheduler()->stopGui();
#endif
                break;

              case WAIT_TO_RELEASE:
                {
                  ExEspStmtGlobals *espGlobals = instances_[currInst]->globals_;
                  if (espGlobals->anyCancelMsgesOut() || 
                      espGlobals->anySendTopMsgesOut())
                    {
                      if (espGlobals->getSMQueryID() > 0)
                      {
                        EXSM_TRACE(EXSM_TRACE_IO_ALL,
                                   "WAIT_TO_RELEASE inst %d",
                                   (int) currInst);
                        EXSM_TRACE(EXSM_TRACE_IO_ALL, 
                                   " sndt msgs %d cancel msgs %d",
                                   (int) espGlobals->numSendTopMsgesOut(),
                                   (int) espGlobals->numCancelMsgesOut());
                      }

                      // Must stay in WAIT_TO_RELEASE state.  Must let other 
                      // frag instances (which might be hosted in this ESP 
                      // process) work, so wait for I/O completion outside 
                      // the scope of this frag instance (i.e., in the caller
                      // of this ExExpFragInstanceDir::work method.)
                    }
                  else
                    {
                      // Ready now to reply to release work request and 
                      // change state to RELEASING_WORK.
                      hasTransidReleaseRequest(currInst);
                    }
                  break; 
                }
              
              case RELEASING:
                {
		  // release any outstanding messages before destroying the
		  // instance (to avoid hangs in the master Executor)
		  ExEspFragInstance * fi = instances_[currInst];
		  if (fi->localRootTcb_)  // if root tcb is still around
		    fi->localRootTcb_->releaseWorkRequest();
		  destroyEntry(currInst);
		  break;
                }

              case GOING_FATAL:
                if (instances_[currInst]->localRootTcb_->reportErrorToMaster())
                  setFiState(currInst, BROKEN, __LINE__);
                break;

              case BROKEN:
		// waiting for message from master to release.
                break;

	      } // switch
            } // if instance is used and not suspended
	} // loop over fragment instances
    } while (loopAgain || getEnvironment()->getAllConnections()->
             getCompletionSeqenceNo() != startSeqNo);
    

  // clean up the completed MasterEspMessages
  getEnvironment()->deleteCompletedMessages();

  // When we exit here, we have called all active schedulers once,
  // they all have worked until they blocked, and no I/Os have
  // completed through the cycle. This must mean that we're stuck
  // until something external happens. Note that if fragment instances
  // send each other data they must bump the completion sequence number
  // or this method may cause a deadlock.
}

ExEspFragInstanceDir::ExEspFragInstance * ExEspFragInstanceDir::findEntry(
     const ExFragKey &key)
{
  CollIndex numEntries = instances_.entries();

  // compare keys
  for (CollIndex i = 0; i < numEntries; i++)
    if (instances_.used(i) AND instances_[i]->key_ == key)
      // found it
      return instances_[i];

  // this key doesn't exist in the fragment instance directory
  return NULL;
}

void ExEspFragInstanceDir::destroyEntry(ExFragInstanceHandle handle)
{
  ExEspFragInstance *entry = instances_[handle];

  // tcb tree will be deleted by glob->deleteMe().
  entry->localRootTcb_ = NULL;
  entry->queryId_ = NULL; // deleted by MsgFragment destructor

  if (entry->globals_)
    {
      Space    *sp = entry->globals_->getSpace();
      NAHeap *hp = (NAHeap *)entry->globals_->getDefaultHeap();

      entry->globals_->deleteMe(FALSE);
      entry->globals_ = NULL;
      NADELETE(sp, Space, hp);
      NADELETE(hp, NAHeap, heap_);
    }

  entry->msgFragment_->decrRefCount();
  entry->msgFragment_ = NULL;


  // delete the entry itself and remove it from the collection
  // (sorry, no destructors are called)
  heap_->deallocateMemory(entry);
  instances_.remove(handle);

  if (instances_.entries() == 0)
    {
      // this esp has been released by all statements and is now available
      getEnvironment()->setIdleTimestamp();
      this->traceIdleMemoryUsage();
    }

  // adjust the high water mark, if needed
  while (highWaterMark_ > 0 AND NOT instances_.used(highWaterMark_-1))
    highWaterMark_--;
}

void ExEspFragInstanceDir::traceIdleMemoryUsage()
{
  static bool first_time = true, mem_trace = false;
  static FILE *traceFile, *procStatusFile;
  static NAHeap *exHeap, *ipcHeap, *contextHeap;
  static UInt32 count = 0;
  static pid_t myPid;
  char fileName[32], buffer[1024], *currPtr;
  ULng32 memPeak, memSize; // VMSize in KB
  time_t timeInSecs;
  tm *localTime;
  Int32 success;
  size_t bytesRead;
  if (first_time)
  {
    char * env_var = getenv("ESP_IDLE_MEMORY_TRACE");
    if (env_var && *env_var == '1')
      mem_trace = true;
    first_time = false;
  }
  if (mem_trace)
  {
    count += 1;
    if (count == 1)
    {
      contextHeap = cliGlobals_->currContext()->exHeap();
      traceFile = fopen("espmemuse.log", "r+");
      if (traceFile == NULL)
      {
        traceFile = fopen("espmemuse.log", "a");
        if (traceFile == NULL)
        {
          fprintf(stderr, "ESP_IDLE_MEMORY_TRACE errno %d opening espmemuse.log by pid %d\n", errno, myPid);
          mem_trace = false;
          return;
        }
        else
          fprintf(traceFile, "TIME,PID,INST,EXAllSize,Cnt,TotSize,AllHW,IPCAllSz,Cnt,CONTEXTAllSize,VmSz,VmPk\n");
      }
      myPid = getpid();
      sprintf(fileName, "/proc/%d/status", myPid);
      procStatusFile = fopen(fileName, "r");
      if (procStatusFile == NULL)
      {
        fprintf(stderr, "ESP_IDLE_MEMORY_TRACE errno %d opening /proc/%d/status\n", errno, myPid);
        mem_trace = false;
        return;
      }
    }
    else
      success = fseek(procStatusFile, 0, SEEK_SET);
    bytesRead = fread(buffer, 1, 1024, procStatusFile);
    currPtr = strstr(buffer, "VmPeak");
    sscanf(currPtr, "%*s %u kB", &memPeak);
    currPtr = strstr(buffer, "VmSize");
    sscanf(currPtr, "%*s %u kB", &memSize);
    timeInSecs = time(0);
    localTime = localtime(&timeInSecs);
    success = fseek(traceFile, 0, SEEK_END); // append to end of file
    fprintf(traceFile, "%d:%02d:%02d,%d,%d,", localTime->tm_hour,
            localTime->tm_min, localTime->tm_sec, myPid, count);
    fprintf(traceFile, PFSZ "," PFSZ "," PFSZ "," PFSZ ",%ld,%ld\n",
            heap_->getAllocSize(),
            heap_->getAllocCnt(),
            heap_->getTotalSize(),
            heap_->getHighWaterMark(),
            (Long)memSize * 1024,
            (Long)memPeak * 1024);
    fflush(traceFile);
  }
}

Lng32 ExEspFragInstanceDir::checkPlanVersion(const ExEspFragInstance * entry, ComDiagsArea& da)
{
   VersionErrorCode versionError = VERSION_NO_ERROR;

   return versionError;
}

NAHeap *ExEspFragInstanceDir::getLocalStatsHeap()
{
  if (localStatsHeap_ == NULL)
    {
      // Need to initialize it first.  See if the 
      // other stats heap is local.  If so, just use it.
      if (statsGlobals_ == NULL)
        localStatsHeap_ = statsHeap_;
      else
          localStatsHeap_ = new (heap_) 
            NAHeap("Process Local Stats Heap", (NAHeap *)heap_,
            8192, 0);
    }
  return localStatsHeap_;
}

void ExEspFragInstanceDir::setDatabaseUserID(Int32 userID,
                                             const char *userName)
{
  ex_assert(cliGlobals_, "CliGlobals pointer should not be NULL");

  ContextCli *context = cliGlobals_->currContext();
  ex_assert(context, "ContextCli pointer should not be NULL");
  
  if (userIDEstablished_ == TRUE)
    {
      // close all (shared) opens if switching user by passing "true"
      context->setDatabaseUserInESP(userID, userName, true);
    }
  else
    {
      context->setDatabaseUserInESP(userID, userName, false);
      userIDEstablished_ = TRUE;
    }
}

void ExEspFragInstanceDir::initFiStateTrace()
{
  for (fiTidx_ = 0; fiTidx_ < NumFiTraceElements; fiTidx_++)
  {
    fiStateTrace_[fiTidx_].fragId_ = UNUSED_COLL_ENTRY;
    fiStateTrace_[fiTidx_].fiState_ = UNUSED;
    fiStateTrace_[fiTidx_].lineNum_ = __LINE__;
  }
}

enum ExEspFragInstanceDir::FragmentInstanceState 
ExEspFragInstanceDir::setFiState(ExFragInstanceHandle fragId, 
                   enum ExEspFragInstanceDir::FragmentInstanceState newState, 
                   Int32 linenum)
{
  if (newState != instances_[fragId]->fiState_)
  {
    if (++fiTidx_ >= NumFiTraceElements) 
      fiTidx_ = 0;
    instances_[fragId]->fiState_ = newState;
    fiStateTrace_[fiTidx_].fragId_ = fragId;
    fiStateTrace_[fiTidx_].fiState_ = newState;
    fiStateTrace_[fiTidx_].lineNum_ = linenum;
  }
  return newState;
}

Int32
ExEspFragInstanceDir::printALiner(Int32 lineno, char *buf)
{
  Int32 rv = 0;
  const char *stateName = "UNKNOWN";
  if (lineno >= NumFiTraceElements)
    return rv; 

  FiStateTrace *fi = &fiStateTrace_[lineno];
  if ((FiStateTrace *)NULL == fi)
     return rv;
  if ((UNUSED <= fi->fiState_) && (fi->fiState_ <= BROKEN))
     stateName = FragmentInstanceStateName[fi->fiState_];
  rv = sprintf(buf, "%.4d  %9d %.15s %6d\n", lineno, fi->fragId_,
               stateName,
               fi->lineNum_);
  return rv;
}


// -----------------------------------------------------------------------
// Methods for class ExEspControlMessage
// -----------------------------------------------------------------------

ExEspControlMessage::ExEspControlMessage(
     ExEspFragInstanceDir *fragInstanceDir,
     IpcEnvironment       *ipcEnvironment,
     CollHeap             *heap) : IpcMessageStream(
                                               ipcEnvironment,
					       IPC_MSG_SQLESP_CONTROL_REPLY,
					       CurrEspReplyMessageVersion,
					       0,
					       TRUE)
{
  fragInstanceDir_     = fragInstanceDir;
  heap_                = heap;
}

ExEspControlMessage::~ExEspControlMessage()
{
  // do nothing
}

void ExEspControlMessage::actOnSend(IpcConnection *connection)
{
  // do nothing
  if (getState() == ERROR_STATE)  
  {
    ex_assert(FALSE,"Error while replying to a control message from master");
  }
  else
  {
    if (connection && connection->getLastSentMsg())
      incReplyMsg(connection->getLastSentMsg()->getMessageLength());
    else
      incReplyMsg(0);
  }
}

void ExEspControlMessage::actOnSendAllComplete()
{
  // start another receive operation for the next request
  clearAllObjects();
  receive(FALSE);
}

void ExEspControlMessage::actOnReceive(IpcConnection *connection)
{
  if (getState() == ERROR_STATE)
    {
      ex_assert(FALSE,"Error while receiving a control message from master");
    }

  ComDiagsArea *da = ComDiagsArea::allocate(heap_);

  if (getType() == IPC_MSG_SQLESP_CONTROL_REQUEST)
    {
      ex_assert(getVersion() == CurrEspRequestMessageVersion,
		"Invalid request msg version");
      while (moreObjects() && da->mainSQLCODE() >= 0)
	{
	  switch (getNextObjType())
	    {
	    case ESP_LOAD_FRAGMENT_HDR:
	      actOnLoadFragmentReq(connection,*da);
	      break;

	    case ESP_FIXUP_FRAGMENT_HDR:
            {
	      actOnFixupFragmentReq(*da);

              // The fragment is fixed up. If the query uses
              // SeaMonster we send a SeaMonster reply back to the
              // master in addition to replying on the Guardian
              // control connection.
              //
              // One FIXUP_REPLY is sent from ESP to master each time
              // an ESP successfully fixes up a fragment. There is no
              // interesting content in these messages. But they need
              // to flow from ESP to master as an SM "go message".
              //
              // Without these replies flowing from ESP to master, if
              // the master were to send a data request to the ESP,
              // the receiving SM service might report that ESP
              // preposts are not yet available.

              // First see if the query uses SM
              bool queryUsesSM = false;
              Int64 smQueryID = 0;
              if (fragInstanceDir_ &&
                  fragInstanceDir_->instances_.used(currHandle_))
              {
                ExEspFragInstanceDir::ExEspFragInstance *inst =
                  fragInstanceDir_->instances_[currHandle_];
                ex_split_bottom_tdb *splitBottomTdb = inst->localRootTdb_;
                ex_assert(splitBottomTdb, "Invalid split bottom TDB pointer");

                if (splitBottomTdb->getQueryUsesSM() && fragInstanceDir_->getEnvironment()->smEnabled())
                {
                  queryUsesSM = true;
                  if (inst->globals_)
                    smQueryID = inst->globals_->getSMQueryID();
                }
              }

              if (queryUsesSM &&
                  da->mainSQLCODE() >= 0)
              {
                // Send the SM fixup reply
                ex_assert(environment_, "Invalid IpcEnvironment pointer");
                ex_assert(environment_->getControlConnection(),
                          "Invalid control connection pointer");

                IpcConnection *conn =
                  environment_->getControlConnection()->getConnection();
                ex_assert(conn, "Invalid IpcConnection pointer");

                const GuaProcessHandle &phandle =
                  conn->getOtherEnd().getPhandle();
                int otherCPU, otherPID, otherNode_unused;
                SB_Int64_Type seqNum = -1;
                phandle.decompose(otherCPU, otherPID, otherNode_unused
                                 , seqNum
                                 );
                
                sm_target_t target;
                memset(&target, 0, sizeof(target));
                
                // Note: Seaquest node number is the old CPU number
                target.node = ExSM_GetNodeID(otherCPU);
                target.pid = otherPID;
                target.verifier = seqNum;
                target.id = (smQueryID > 0 ? smQueryID :
                             ExSMGlobals::getExeInternalSMID());
                
                EXSM_TRACE(EXSM_TRACE_MAIN_THR,
                           "Sending FIXUP REPLY to node %d pid %d "
                           "seqNum %" PRId64 "id %" PRId64,
                           (int) target.node, (int) target.pid, 
                           seqNum, target.id);

                ExSMShortMessage m;
                m.setTarget(target);
                m.setNumValues(1);
                m.setValue(0, ExSMShortMessage::FIXUP_REPLY);
                int32_t rc = m.send();

                if (rc != 0)
                {
                  *da << DgSqlCode(-EXE_SM_FUNCTION_ERROR)
                  << DgString0("ExSM_SendShortMessage")
                  << DgInt0((Lng32) rc)
                  << DgInt1((Lng32) getpid())
                  << DgString1(fragInstanceDir_->getCliGlobals()->myProcessNameString())
                  << DgNskCode((Lng32) 10000 + abs(rc));
                }
              } // if (queryUsesSM)

            } // case ESP_FIXUP_FRAGMENT_HDR
            break;
            
	    case ESP_RELEASE_FRAGMENT_HDR:
	      actOnReleaseFragmentReq(*da);
	      break;

	    case ESP_PARTITION_INPUT_DATA_HDR:
	    case ESP_WORK_TRANSACTION_HDR:
	    case ESP_RELEASE_TRANSACTION_HDR:
	      da->decrRefCount();
	      actOnReqForSplitBottom(connection);
	      return; // don't reply, split bottom node will do this

	    default:
	      ex_assert(FALSE, "Invalid request for an ESP control connection");
	    }
	}

    }
  else
    {
      // is this an error or is this a system message?
      ex_assert(FALSE,"Internal error");
      // set diagnostics area and reply $$$$
    }

  // done with receiving, now build the reply
  clearAllObjects();

  setType(IPC_MSG_SQLESP_CONTROL_REPLY);
  setVersion(CurrEspReplyMessageVersion);

  ExEspReturnStatusReplyHeader *replyHeader =
    new(heap_) ExEspReturnStatusReplyHeader(heap_);

  replyHeader->key_    = currKey_;
  replyHeader->handle_ = currHandle_;

  if (currHandle_ == NullFragInstanceHandle)
    {
      replyHeader->instanceState_ =
	ExEspReturnStatusReplyHeader::INSTANCE_RELEASED;
    }
  else
    {
      switch(fragInstanceDir_->instances_[currHandle_]->fiState_)
	{
	case ExEspFragInstanceDir::UNUSED:
	  replyHeader->instanceState_ =
	    ExEspReturnStatusReplyHeader::INSTANCE_RELEASED;
	  break;

	case ExEspFragInstanceDir::DOWNLOADED:
	case ExEspFragInstanceDir::UNPACKED:
	case ExEspFragInstanceDir::BUILT:
	  replyHeader->instanceState_ =
	    ExEspReturnStatusReplyHeader::INSTANCE_DOWNLOADED;
	  break;

	case ExEspFragInstanceDir::FIXED_UP:
	case ExEspFragInstanceDir::READY_INACTIVE:
	  replyHeader->instanceState_ =
	    ExEspReturnStatusReplyHeader::INSTANCE_READY;
	  break;

	case ExEspFragInstanceDir::ACTIVE:
	  replyHeader->instanceState_ =
	    ExEspReturnStatusReplyHeader::INSTANCE_ACTIVE;
	  break;

	case ExEspFragInstanceDir::BROKEN:
	default:
	  replyHeader->instanceState_ =
	    ExEspReturnStatusReplyHeader::INSTANCE_BROKEN;
	  break;

	}
    }

  *this << *replyHeader;
  replyHeader->decrRefCount();

  // pack the SQL diagnostics area into the reply message
  if (da->mainSQLCODE())
    *this << *da;
  da->decrRefCount();
  
  // the individual cases have initialized a reply message,
  // now send the reply back to the client
  send(FALSE);
}

void ExEspControlMessage::actOnLoadFragmentReq(
     IpcConnection *connection,
     ComDiagsArea & /*da*/)
{
  ExEspLoadFragmentReqHeader receivedRequest(heap_);
  ExFragInstanceHandle assignedHandle = NullFragInstanceHandle;

  *this >> receivedRequest;

  // set stopAfter=0 so esp will never timeout during query execution
  environment_->setStopAfter(0);
  environment_->setInactiveTimeout(0);
  environment_->clearIdleTimestamp();
  environment_->clearInactiveTimestamp();

  // make sure at least one fragment follows the header
  ex_assert(moreObjects() AND getNextObjType() == ESP_FRAGMENT,
	    "Load message without following fragment");

  // 1 or more fragments may follow
  while (moreObjects() AND getNextObjType() == ESP_FRAGMENT)
  {
    ExMsgFragment *msgFragment = new(heap_) ExMsgFragment(heap_);
    
    // get the fragment object out of the message and add it to the
    // global fragment directory
    *this >> *msgFragment;
    
    // On Linux if this is the first fragment seen by this ESP, set
    // the database user identity for this process
      Int32 userID = msgFragment->getDatabaseUserID();
      const char *userName = msgFragment->getDatabaseUserName();
    if (!fragInstanceDir_->getUserIDEstablished())
    {
    }
      fragInstanceDir_->setDatabaseUserID(userID, userName);
    
    ExFragInstanceHandle newHandle =
      fragInstanceDir_->addEntry(msgFragment,connection);
    
    if (msgFragment->getFragType() == ExFragDir::ESP)
    {
      // remember the key and handle of this download request
      // (ignore downloaded DP2 fragments, they always come as
      // dependents of ESP fragments)
      currHandle_ = newHandle;
      currKey_    = msgFragment->getKey();
    }
    
  } // while more objects
  
  // the reply is handled by the caller
}

void ExEspControlMessage::actOnFixupFragmentReq(ComDiagsArea &da)
{
  ExEspFixupFragmentReqHeader receivedRequest(fragInstanceDir_->heap_);
  ExProcessIdsOfFragList *poflist = NULL;
  ExResolvedNameObj *lnio = NULL;
  ExMsgResourceInfo *ri = NULL;
  ExMsgTimeoutData * td = NULL;  
  ExEspStmtGlobals  *glob = NULL;
  CollHeap          *instHeap;

  *this >> receivedRequest;

  Lng32 maxPollingInterval = receivedRequest.getMaxPollingInterval();
  Lng32 persistentOpens = receivedRequest.getPersistentOpens();
  environment_->setMaxPollingInterval(maxPollingInterval);
  environment_->setPersistentOpens(persistentOpens > 0);

  currKey_    = receivedRequest.key_;
  currHandle_ = fragInstanceDir_->findHandle(receivedRequest.key_);
  if (currHandle_ != NullFragInstanceHandle)
    {
      glob = fragInstanceDir_->getGlobals(currHandle_);
      instHeap = glob->getDefaultHeap();

      // are process ids of child fragments following the header?
      if (moreObjects() AND getNextObjType() == ESP_PROCESS_IDS_OF_FRAG)
	{
	  poflist = new(instHeap) ExProcessIdsOfFragList(instHeap);

	  // 1 or more list of input fragment process ids may follow
	  while (moreObjects() AND
		 getNextObjType() == ESP_PROCESS_IDS_OF_FRAG)
	    {
	      ExProcessIdsOfFrag *pof =
		new(instHeap) ExProcessIdsOfFrag(instHeap);
	      
	      *this >> *pof;
	      poflist->insert(pof);
	    }
	}

      if (moreObjects() AND getNextObjType() == ESP_RESOURCE_INFO)
	{
	  ri = new(instHeap) ExMsgResourceInfo(NULL,instHeap);

	  *this >> *ri;
	}

      

      if (moreObjects() AND getNextObjType() == ESP_TIMEOUT_DATA)
	{
	  td = new(instHeap) ExMsgTimeoutData( NULL, instHeap );

	  *this >> *td;
	}

      

      if (moreObjects() && getNextObjType() == ESP_SM_DOWNLOAD_INFO)
      {
        // Create a new object to hold SeaMonster properties for this
        // query and store a pointer to the object in statement
        // globals
        ExSMDownloadInfo *info = new (instHeap)
          ExSMDownloadInfo(instHeap);
        *this >> *info;
        glob->setSMDownloadInfo(info);
      }

      // remember the input fragment process ids in the globals for this
      // fragment entry
      glob->setPidFragList(poflist);

      glob->setResourceInfo(ri);
      if ( td ) * glob->getTimeoutData() = td->getTimeoutData() ;  

      glob->setStatsEnabled(receivedRequest.statsEnabled());

      // fixup the statement associated with the handle
      fragInstanceDir_->fixupEntry(currHandle_,
				   receivedRequest.numOfParentInstances_,
                                   da);
    }
  else
    {
      ex_assert(FALSE,"entry not found, set diags area and reply");
      // $$$$ entry not found set diagnostics area
      // $$$$ may have to remove extra message objects to be
      //      able to send a reply
    }

  // if fixup priority has been sent, save that so my priority could be restored to
  // this value.
  if (receivedRequest.getEspFixupPriority() > 0)
    {
      glob->setMyFixupPriority((IpcPriority)receivedRequest.getEspFixupPriority());
    }

  // if execute priority has been sent, set my priority to that value.
  if (receivedRequest.getEspExecutePriority() > 0)
    {
      Lng32 rc = 0;

      // get my current priority and save it in stmt globals.
      // If an error is returned, ignore and leave priorities as is.
      //      long p;
      //rc = ComRtGetProcessPriority(p);
      //      if (rc == 0) // no error
      //	{
      // set the execute priority
      rc = 
	ComRtSetProcessPriority(receivedRequest.getEspExecutePriority(), 
				FALSE);
      if (rc != 0)
	{
	  // don't do anything.
	}
      //	}
    }

  // the reply is handled by the caller
}

void ExEspControlMessage::actOnReleaseFragmentReq(ComDiagsArea & /*da*/)
{
  // esp being released by master is regarded as idle but not inactive
  environment_->setInactiveTimeout(0);
  environment_->clearInactiveTimestamp();

  ExEspReleaseFragmentReqHeader receivedRequest(fragInstanceDir_->heap_);

  *this >> receivedRequest;

  currKey_    = receivedRequest.key_;
  currHandle_ = fragInstanceDir_->findHandle(receivedRequest.key_);

  ExEspStmtGlobals *glob = NULL;
  IpcPriority myFixupPriority = 0;

  if (receivedRequest.deleteStmt())
    {
      // delete all fragment instances that come from the same statement
      // as the given key
      ExFragKey wildCardKey = currKey_;
      for (CollIndex i = 0; i < fragInstanceDir_->highWaterMark_; i++)
	{
	  if (fragInstanceDir_->instances_.used(i))
	    {
	      if (glob == NULL)
		{
		  glob = fragInstanceDir_->getGlobals(i);
		  if (glob)
		    {
		      myFixupPriority = glob->getMyFixupPriority();
		      glob->setCloseAllOpens(receivedRequest.closeAllOpens());
		    }
		}

	      const ExFragKey &instKey =
		fragInstanceDir_->instances_[i]->key_;

	      // alter the wild card key such that it uses a matching
	      // fragment id, but keeps its old process id and
	      // statement handle
	      wildCardKey.setFragId(instKey.getFragId());
	      
	      // now, if the wild card key is equal to the key in this entry,
	      // it belongs to the process/statement to be released
	      if (wildCardKey == instKey)
		fragInstanceDir_->releaseEntry(i);
	    }
	}
    }
  else
    {
      // delete a particular fragment instance, indicated by the key and handle
      if (currHandle_ != NullFragInstanceHandle)
	{
	  glob = fragInstanceDir_->getGlobals(currHandle_);
	  if (glob)
	    {
	      myFixupPriority = glob->getMyFixupPriority();
	      glob->setCloseAllOpens(receivedRequest.closeAllOpens());
	    }

	  fragInstanceDir_->releaseEntry(currHandle_);
	}
      else
	{
	  // $$$$ entry not found, can probably ignore this??? assert for now
	  ex_assert(FALSE,"Couldn't find frag entry to release");
	}
    }

  if (receivedRequest.detachFromMaster_)
    {
      fragInstanceDir_->numMasters_--;
    }

  // change my priority back to my 'fixup' priority.
  if (myFixupPriority > 0)
    {
      Lng32 rc = 
	ComRtSetProcessPriority(myFixupPriority, FALSE);
      if (rc != 0)
	{
	  // ignore error.
	}
    }
  
  // how long to keep idle esp alive
  environment_->setStopAfter(receivedRequest.idleTimeout_);

  // the reply is handled by the caller
}

void ExEspControlMessage::actOnReqForSplitBottom(IpcConnection *connection)
{
  NABoolean changePri = FALSE;
  switch (getNextObjType())
    {
    case ESP_PARTITION_INPUT_DATA_HDR:
      {
	ExEspPartInputDataReqHeader receivedRequest(fragInstanceDir_->heap_);
	
	*this >> receivedRequest;
	currKey_ = receivedRequest.key_;
      }
      break;

    case ESP_WORK_TRANSACTION_HDR:
      {
	ExEspWorkReqHeader receivedRequest(fragInstanceDir_->heap_);

	*this >> receivedRequest;
	currKey_ = receivedRequest.key_;
      }
      break;

    case ESP_RELEASE_TRANSACTION_HDR:
      {
	ExEspReleaseWorkReqHeader receivedRequest(fragInstanceDir_->heap_);

	*this >> receivedRequest;
	currKey_ = receivedRequest.key_;

	changePri = TRUE;

	// how long to keep inactive esp alive
	environment_->setInactiveTimeout(receivedRequest.inactiveTimeout_);
      }
      break;

    default:
      ex_assert(0,"Invalid object type for split bottom node received");
    }

  currHandle_ = fragInstanceDir_->findHandle(currKey_);

  if (currHandle_ != NullFragInstanceHandle)
    {
      ex_split_bottom_tcb * splitBottom =
	fragInstanceDir_->getTopTcb(currHandle_);

      if (changePri)
	{
	  // change my priority back to my 'fixup' priority.
	  ExEspStmtGlobals *glob = fragInstanceDir_->getGlobals(currHandle_);
	  IpcPriority myFixupPriority = glob->getMyFixupPriority();
	  if (myFixupPriority > 0)
	    {
	      Lng32 rc = 
		ComRtSetProcessPriority(myFixupPriority, FALSE);
	      if (rc != 0)
		{
		  // ignore error.
		}

	      // reset priority indication in global
	      glob->setMyFixupPriority(0);
	      
	    }
	}

      if (splitBottom)
	{
	  giveMessageTo(*splitBottom->getMessageStream(),connection);

	  // start another receive operation for the next request
	  receive(FALSE);
	}
      else
	{
	  ex_assert(FALSE,"entry is in wrong state to receive part input");
	}
    }
  else
    {
      ex_assert(FALSE,"entry to receive part. input not found");
    }

  // the reply is handled by the split bottom node
}

void ExEspControlMessage::incReplyMsg(Int64 msgBytes)
{
  ExStatisticsArea *statsArea;

  if (currHandle_ != NullFragInstanceHandle)
  {
    if ((statsArea = fragInstanceDir_->getGlobals(currHandle_)->getStatsArea()) != NULL)
      statsArea->incReplyMsg(msgBytes);
  }
}

// search for the active tcb in the fragment whose security key matches the one provided.
ex_split_bottom_tcb *ExEspFragInstanceDir::getExtractTop(const char *securityKey)
{
  CollIndex numEntries = instances_.entries();
  for (CollIndex i = 0; i < numEntries; i++)
  {
    ex_split_bottom_tcb *top = instances_[i]->localRootTcb_;
    if (top && top->splitBottomTdb().getExtractProducerFlag()) 
    {
      if (!str_cmp_c(top->splitBottomTdb().getExtractSecurityKey(), securityKey)) 
        return top;
    }
  }
  return NULL;
}
