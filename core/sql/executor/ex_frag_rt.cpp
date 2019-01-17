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
 * File:         ex_frag_rt.C
 * Description:  Run time fragment management in the master executor
 *
 * Created:      1/26/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

// for testing
// #define IPC_INTEGRITY_CHECKING 1

#include "Platform.h"

#include "ex_stdh.h"
#include "ex_exe_stmt_globals.h"
#include "ex_frag_rt.h"
#include "Ex_esp_msg.h"
#include "PartInputDataDesc.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include "ex_root.h"
#include "ex_transaction.h"
#include "ExStats.h"
#include "ComDistribution.h"
#include "sql_buffer_size.h"
#include "ExCextdecs.h"


#include "seabed/ms.h"
#include "PortProcessCalls.h"  // trace need phandle for ESP info
#include "ExSMGlobals.h"
#include "SMConnection.h"

class AssignEspArrays
{
public:
  ExRtFragInstance **instance_;
  Lng32 *creatingEspFragEntry_;
  IpcGuardianServer **creatingEspEntry_;
  NAHeap *heap_;
  AssignEspArrays(NAHeap *heap, Lng32 numEsps)
  {
    heap_ = heap;
    instance_ = (ExRtFragInstance **)heap_->allocateMemory(sizeof(ExRtFragInstance *) * numEsps);
    creatingEspFragEntry_ = (Lng32 *)heap_->allocateMemory(sizeof(Lng32) * numEsps);
    creatingEspEntry_ = (IpcGuardianServer **)heap_->allocateMemory(sizeof(IpcGuardianServer *) * numEsps);
    for (int i = 0; i < numEsps; i++)
    {
      instance_[i] = NULL;
      creatingEspEntry_[i] = NULL;
    }
  }
  ~AssignEspArrays()
  {
    heap_->deallocateMemory((void *)instance_);
    heap_->deallocateMemory((void *)creatingEspFragEntry_);
    heap_->deallocateMemory((void *)creatingEspEntry_);
  }
};

ULng32 hashFunc_EspCacheKey(const ExEspCacheKey& key)
{
  return ExEspCacheKey::hash(key);
}

// -----------------------------------------------------------------------
// Most data structures for a statement in the executor are allocated
// once in the life span of a statement. An object of class Space is
// used to allocate the space for these data structures. Unfortunately,
// the run-time fragment directory involves some classes that either
// have a life span that can be larger than that of a statement, or
// that may be allocated multiple times during execution of the
// statement. This are the classes:
// - ExFragInstance     (ESPs may be added/deleted dynamically)
// - ExMasterEspMessage (we may send many messages, e.g. for load balancing)
// - ExEspDbEntry       (ESPs may live longer than a statement)
//
// In all cases, the classes have an overloaded operator new, take an
// additional CollHeap * object in the constructor, and have a method
// deleteMe(). The classes must be allocated with the "placement" form
// of operator new, the used CollHeap must be passed to the constructor,
// and destruction of the classes must be done via the deleteMe() method.
// Using the standard operators new and delete of the class will abort
// the query.
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Methods for class ExRtFragTable
// -----------------------------------------------------------------------

#ifdef IPC_INTEGRITY_CHECKING

void checkExRtFragTableIntegrity(ExRtFragTable * ft)
  {
  ft->checkLocalIntegrity();
  }

#endif


ExRtFragTable::ExRtFragTable(ExMasterStmtGlobals *glob,
			     ExFragDir *fragDir,
			     char  *generatedObject) :
     fragmentEntries_(glob->getDefaultHeap(),fragDir->getNumEntries()),
     outstandingServiceRequests_(glob->getDefaultHeap()),
     displayInGui_(FALSE)
{
  glob_                     = glob;
  fragDir_                  = fragDir;
  generatedObject_          = generatedObject;
  state_                    = NOT_ASSIGNED;
  dynamicLoadBalancing_     = FALSE;
  numRootRequests_          = 0;
  quiesce_                  = FALSE;
  numLoadFixupMsgesOut_     = 0;
  numWorkMsgesOut_          = 0;
  numTransactionalMsgesOut_ = 0;
  numReleaseEspMsgesOut_    = 0;

  CollIndex numFrags = fragDir->getNumEntries();

  for (ExFragId i = 0; i < numFrags; i++)
    {
      // Add a new entry (id,generatedDir,numEsps) at index <id>, but don't
      // assign any ESPs yet; allocate table entry from space since it does
      // not change dynamically, but allocate frag instances from the heap
      ExRtFragTableEntry *entry =
	new(glob_->getSpace()) ExRtFragTableEntry(glob->getDefaultHeap());

      entry->id_                   = i;
      entry->numEsps_              = fragDir->getNumESPs(i);
      entry->dynamicLoadBalancing_ = FALSE;
      entry->partDesc_             = fragDir->getPartDesc(i);
      
      fragmentEntries_.insert(entry);
    }

  // build the unique identifier for the master's fragment instance
  // (used to easily create keys for the ESP fragments)
  ex_assert(fragDir->getType(0) == ExFragDir::MASTER,
	    "Master fragment must have fragment id 0");
  masterFragmentInstanceKey_ = ExFragKey(
       glob_->getIpcEnvironment()->getMyOwnProcessId(IPC_DOM_GUA_PHANDLE),
       (ExEspStatementHandle)(NA_JulianTimestamp() & 0xFFFFFFFF) /*64 bits to ulong*/,
       0);

#ifdef IPC_INTEGRITY_CHECKING  
  IpcEnvironment * ie = glob_->getIpcEnvironment(); 
  ie->setExRtFragTableIntegrityCheckPtr(checkExRtFragTableIntegrity);
  ie->checkIntegrity();
  ie->setCurrentExRtFragTable(this);
  ie->checkIntegrity();
#endif
}

ExRtFragTable::~ExRtFragTable()
{
#ifdef IPC_INTEGRITY_CHECKING
  cerr << "In destructor for ExRtFragTable " << (void *)this << "." << endl;
  IpcEnvironment * ie = glob_->getIpcEnvironment();
  ie->checkIntegrity();
#endif

  // since I am going away, remove all references to me
  CollIndex i = 0;
  for (i = 0; i < outstandingServiceRequests_.entries(); i++)
    outstandingServiceRequests_[i]->rtFragTable_ = NULL;

  // destroy and delete all the fragment entries
  for (i = 0; i < fragmentEntries_.entries(); i++)
    fragmentEntries_[i]->release();

#ifdef IPC_INTEGRITY_CHECKING
  ie->checkIntegrity();
  ie->removeCurrentExRtFragTable(this);
  ie->checkIntegrity();
#endif
}

NABoolean ExRtFragTable::setActiveState()
{
  // shortcut return, this method is called often w/o ESPs being used
  if (state_ == NO_ESPS_USED)
    return TRUE;

  // with dynamic load balancing, the protocol is not yet smart enough
  // to handle multiple partition assignments for different requests,
  // so force the root node to process its requests one at a time
  if (dynamicLoadBalancing_ AND numRootRequests_ > 0)
    return FALSE;

  // we now know that the data flow is started
  numRootRequests_++;

  // make sure we are no longer quiesced and send out work messages
  // if needed
  continueWithTransaction();

  // try to finish some requests, now that we're here
  workOnRequests();

  return TRUE;
}

void ExRtFragTable::setInactiveState()
{
  // root node indicates that data flow has stopped
  numRootRequests_--;
}

void ExRtFragTable::assignEsps(NABoolean /*checkResourceGovernor*/,
			       UInt32 &numOfTotalEspsUsed,
			       UInt32 &numOfEspsStarted
                               )
{
  Int16 esp_multi_fragment, esp_num_fragments;
  Int32 entryNumber, numEntries, launchesStarted, launchesCompleted;

  if (state_ == NO_ESPS_USED)
    return;

  numOfTotalEspsUsed = 0;
  numOfEspsStarted = 0;
  NABoolean espsUsed = FALSE;
  ex_assert(state_ != READY,"Invalid state for assigning ESPs");
  CollHeap *heap = glob_->getDefaultHeap();
  ComDiagsArea *diags = glob_->getDiagsArea();

  LIST(ExEspDbEntry *) alreadyAssignedEsps(heap);
  // Note: alreadyAssignedEsps is cleared after assigning ESPs for all the instances of a fragment

  // $$$$ check with the resource governor

  // Assign ESPs for all the fragments that are executed in an ESP

  ExMasterStmtGlobals *mstrGlob = glob_->castToExMasterStmtGlobals();
  ContextCli *currentContext = glob_->getContext();
  Statement *currentStatement = glob_->getStatement();
  NABoolean verifyCPU = FALSE ; // Check that each CPU is up

  IpcPriority espPriority = IPC_PRIORITY_DONT_CARE;

  ExEspManager *espManager = glob_->getEspManager();
  Lng32 idleTimeout = getEspIdleTimeout();
  Lng32 assignTimeWindow = currentContext->getSessionDefaults()->getEspAssignTimeWindow();

  esp_multi_fragment = fragDir_->espMultiFragments();
  esp_num_fragments = fragDir_->espNumFragments();

 // de-coupling ESP with database uid if set
  const char *esp_with_uid = getenv("ESP_WITH_USERID");

  for (CollIndex i = 0; i < fragmentEntries_.entries(); i++)
    {
      if (fragDir_->getType(i) == ExFragDir::ESP)
	{
	  ExRtFragTableEntry &fragEntry = *fragmentEntries_[i];
	  NABoolean needsPIV =
	    (fragEntry.partDesc_ AND
	     fragEntry.partDesc_->getPartInputDataLength() > 0);

	  espsUsed = TRUE;

	  // if there are more partitions than ESPs then use dynamic
	  // load balancing to assign partitions to ESPs
	  if (needsPIV AND
	      fragEntry.numEsps_ < fragEntry.partDesc_->getNumPartitions())
	    {
	      // this means that we will have to send partition input values
	      // each time when the root node processes a new query and
	      // that we will have to stay active to send more partition
	      // input values during query execution
	      fragEntry.dynamicLoadBalancing_ = TRUE;
	      dynamicLoadBalancing_ = TRUE; // set statement indicator, too
	      fragEntry.assignedPartInputValues_.clear();
	    }

	  // Get node map for this fragment.
	  ExEspNodeMap *nodeMap = fragDir_->getEspNodeMap(i);
	  numOfTotalEspsUsed += fragEntry.numEsps_;
	  AssignEspArrays assignEspArrays((NAHeap *)heap, fragEntry.numEsps_);
	  Lng32 espLevel = fragDir_->getEspLevel(i);
          NABoolean soloFragment = fragDir_->soloFrag(i);
          NABoolean containsBMOs = fragDir_->containsBMOs(i);

  	  entryNumber = launchesStarted = 0;
          for (Lng32 e = 0; e < fragEntry.numEsps_ && state_ != ERROR; e++)
	    {
	      // Decide on an ESP to use and link the instance to that
	      // ESP by adding a new entry. Don't download yet, so the
	      // fragment instance handle is not valid yet.
	      assignEspArrays.instance_[e] = new(heap) ExRtFragInstance(heap);
              IpcCpuNum cpuNum = nodeMap->getNodeNumber(e);

	      assignEspArrays.instance_[e]->clusterName_           = nodeMap->getClusterName(e);
	      assignEspArrays.instance_[e]->cpuNum_                = cpuNum;
	      assignEspArrays.instance_[e]->memoryQuota_           = 0;
	      assignEspArrays.instance_[e]->state_                 = ESP_ASSIGNED;
	      assignEspArrays.instance_[e]->fragmentHandle_        = NullFragInstanceHandle;
	      assignEspArrays.instance_[e]->numControlMessages_    = 0;
	      assignEspArrays.instance_[e]->partInputDataAssigned_ = NOT needsPIV;
	      assignEspArrays.instance_[e]->workMessageSent_       = FALSE;
	      assignEspArrays.instance_[e]->releasingWorkMsg_      = FALSE;
              assignEspArrays.instance_[e]->workMsg_               = NULL;

	      NABoolean startedANewEsp = FALSE;
             
              // Unassign Esps only when the CLI calls are not recursive
              if (currentContext->getNumOfCliCalls() == 1)
	        currentContext->unassignEsps();
	      
	      while (TRUE) { // "while" is used to try again when a CPU failed
		NABoolean origVerifyCPU = verifyCPU;
		assignEspArrays.creatingEspEntry_[entryNumber] = NULL;

		assignEspArrays.instance_[e]->usedEsp_ =
		  glob_->getEspManager()->shareEsp(
                    &diags,
		    alreadyAssignedEsps,
                    heap,
                    currentStatement,
                    assignEspArrays.instance_[e]->clusterName_,
		    startedANewEsp,
                    cpuNum,
		    assignEspArrays.instance_[e]->memoryQuota_,
                    (esp_with_uid != NULL)? *currentContext->getDatabaseUserID():
                        NA_UserIdDefault,
		    mstrGlob->verifyESP(),
		    &verifyCPU,
		    espPriority,
		    espLevel,
                    idleTimeout,
                    assignTimeWindow,
		    &assignEspArrays.creatingEspEntry_[entryNumber],
                    soloFragment, 
                    esp_multi_fragment,
                    esp_num_fragments 
		    );

                break; // Retry is done in IpcGuardianServer::launchNSKLiteProcess
	      }

              // Creating a new ESP in a nowaited manner?
	      if (assignEspArrays.creatingEspEntry_[entryNumber] != NULL)
	      {
		assignEspArrays.creatingEspFragEntry_[entryNumber] = e;
		if (assignEspArrays.creatingEspEntry_[entryNumber]->isCreatingProcess())
		  launchesStarted += 1;
		else
		  ex_assert(0, "Server must be in CREATING_PROCESS state");
		entryNumber += 1;
		continue;
	      }

	      if (startedANewEsp)
		numOfEspsStarted++;

              glob_->setGlobDiagsArea(diags);

	      if (assignEspArrays.instance_[e]->usedEsp_)
		{
		  fragEntry.assignedEsps_.insertAt(e,assignEspArrays.instance_[e]);
		  alreadyAssignedEsps.insert(assignEspArrays.instance_[e]->usedEsp_);

		  IpcServer *ipcs = assignEspArrays.instance_[e]->usedEsp_->getIpcServer();
		  if (NOT ipcs OR
		      NOT ipcs->getControlConnection() OR
		      ipcs->getControlConnection()->getState() ==
		      IpcConnection::ERROR_STATE)
		    {
                      bool canReportDiags = false;
                      if (ipcs && ipcs->getControlConnection())
                        canReportDiags = true;
                      if (diags && diags->getNumber(DgSqlCode::ERROR_) > 0)
                        canReportDiags = true;
                      ex_assert(canReportDiags, "Will not be able to report diags.");
		      assignEspArrays.instance_[e]->state_ = LOST_CONNECTION;
		      state_ = ERROR;
                      break;
		    }
		}
              else
                {
                  // failed to obtain an ESP
                  ex_assert(
                    (diags && diags->getNumber(DgSqlCode::ERROR_) > 0),
                    "Missing error condition.");
                  assignEspArrays.instance_[e]->deleteMe();
                  assignEspArrays.instance_[e] = NULL;
                  state_ = ERROR;
                  break;
                }
	    }
	  // for multi fragment esp - begin
	  if (esp_multi_fragment) {
	    alreadyAssignedEsps.clear();
	  }
	  // for multi fragment esp - end

	  numEntries = entryNumber;
	  launchesCompleted = 0;
	  for (entryNumber = 0; launchesCompleted < launchesStarted; entryNumber++)
	    {
	      if (entryNumber == numEntries)
                entryNumber = 0;
	      if (assignEspArrays.creatingEspEntry_[entryNumber]->isCreatingProcess() == FALSE)
		continue;
	      Lng32 e = assignEspArrays.creatingEspFragEntry_[entryNumber];
	      NABoolean startedANewEsp = FALSE;
	      assignEspArrays.instance_[e]->usedEsp_ =
		    glob_->getEspManager()->shareEsp(
                    &diags,
		    alreadyAssignedEsps,
		    heap,
		    currentStatement,
                    assignEspArrays.instance_[e]->clusterName_,
		    startedANewEsp,
                    assignEspArrays.instance_[e]->cpuNum_,
		    assignEspArrays.instance_[e]->memoryQuota_,
                    (esp_with_uid != NULL)? *currentContext->getDatabaseUserID():
                        NA_UserIdDefault, // de-coupling ESP with database uid
		    mstrGlob->verifyESP(),
		    &verifyCPU,
		    espPriority,
		    espLevel,
		    idleTimeout,
                    assignTimeWindow,
		    &assignEspArrays.creatingEspEntry_[entryNumber],
                    soloFragment,
                    esp_multi_fragment,
                    esp_num_fragments);
	      if (assignEspArrays.creatingEspEntry_[entryNumber] && assignEspArrays.creatingEspEntry_[entryNumber]->isReady())
              {
                if (glob_->getIpcEnvironment()->getNumOpensInProgress() >= FS_MAX_CONCUR_NOWAIT_OPENS)
                  glob_->getIpcEnvironment()->getAllConnections()->waitOnAll(IpcInfiniteTimeout);
		launchesCompleted += 1;
              }
	      else
              {
                if (!assignEspArrays.creatingEspEntry_[entryNumber])
                {
                  glob_->setGlobDiagsArea(diags);
                  launchesCompleted += 1; // Completed with error
                  ex_assert(
                    (diags && diags->getNumber(DgSqlCode::ERROR_) > 0),
                    "Missing error condition.");
                  assignEspArrays.instance_[e]->deleteMe();
                  assignEspArrays.instance_[e] = NULL;
                  state_ = ERROR;
                  continue;
                }
                else
                  ex_assert(0, "Server must be in READY state if it exists");
              }

	      if (startedANewEsp)
		numOfEspsStarted++;

              glob_->setGlobDiagsArea(diags);
	      if (assignEspArrays.instance_[e]->usedEsp_)
              {
		fragEntry.assignedEsps_.insertAt(e,assignEspArrays.instance_[e]);
		alreadyAssignedEsps.insert(assignEspArrays.instance_[e]->usedEsp_);

		IpcServer *ipcs = assignEspArrays.instance_[e]->usedEsp_->getIpcServer();
		if (NOT ipcs OR
		    NOT ipcs->getControlConnection() OR
		    ipcs->getControlConnection()->getState() ==
		    IpcConnection::ERROR_STATE)
		    {
                      bool canReportDiags = false;
                      if (ipcs && ipcs->getControlConnection())
                        canReportDiags = true;
                      if (diags && diags->getNumber(DgSqlCode::ERROR_) > 0)
                        canReportDiags = true;
                      ex_assert(canReportDiags, "Will not be able to report diags.");
		      assignEspArrays.instance_[e]->state_ = LOST_CONNECTION;
		      state_ = ERROR;
		      continue;
		    }
		}
		else
		{
		  // failed to obtain an ESP
                  ex_assert(
                    (diags && diags->getNumber(DgSqlCode::ERROR_) > 0),
                    "Missing error condition.");
		  assignEspArrays.instance_[e]->deleteMe();
		  assignEspArrays.instance_[e] = NULL;
		  state_ = ERROR;
                  continue;
		}
          }
          // Now complete any outstanding control connection parallel opens
          while (glob_->getIpcEnvironment()->getNumOpensInProgress() > 0)
            glob_->getIpcEnvironment()->getAllConnections()->waitOnAll(IpcInfiniteTimeout);
	  // multi fragment esp - begin
	  if (esp_multi_fragment)
          {
	    alreadyAssignedEsps.clear();
	  }
	  // multi fragment esp - end
          for (Lng32 e = 0; e < fragEntry.numEsps_; e++)
          {
            if (assignEspArrays.instance_[e] &&
                assignEspArrays.instance_[e]->usedEsp_)
            {
              IpcServer *ipcs = assignEspArrays.instance_[e]->usedEsp_->getIpcServer();
              if (NOT ipcs OR
                  NOT ipcs->getControlConnection() OR
                  ipcs->getControlConnection()->getState() ==
                  IpcConnection::ERROR_STATE)
              {
                if (ipcs && ipcs->getControlConnection())
                {
	          IpcAllocateDiagsArea(diags, heap);
                  ((GuaConnectionToServer*)ipcs->getControlConnection())->populateDiagsArea(diags, heap);
                  glob_->setGlobDiagsArea(diags);
                }
                ex_assert(
                  (diags && diags->getNumber(DgSqlCode::ERROR_) > 0),
                  "Missing error condition.");
                assignEspArrays.instance_[e]->state_ = LOST_CONNECTION;
                state_ = ERROR;
                break;
              }
            }
          }
          if (state_ == ERROR)
          {
            ex_assert(
              (diags && diags->getNumber(DgSqlCode::ERROR_) > 0),
              "Missing error condition.");
            if (diags != glob_->getDiagsArea())
              glob_->setGlobDiagsArea(diags);
            releaseAssignedEsps();
            return;
          }

	}
    }

  if (espsUsed)
  {
    currentContext->addToStatementWithEspsList(currentStatement);
    state_ = ASSIGNED; // this is done waited right now
  }
  else
    state_ = NO_ESPS_USED;
}

void ExRtFragTable::downloadAndFixup()
{
  static THREAD_P bool sv_exe_abandon_on_error_checked = false;
  static THREAD_P bool sv_exe_abandon_on_error = false;
  if (!sv_exe_abandon_on_error_checked)
  {
    char *lv_exe_abandon_on_error = getenv("EXE_STOP_ESPS_ON_ERROR");
    if ((lv_exe_abandon_on_error != NULL) &&  (*lv_exe_abandon_on_error == '1'))
	sv_exe_abandon_on_error = true;
    sv_exe_abandon_on_error_checked = true;
  }

  if (state_ == NO_ESPS_USED)
    return;

  ex_assert(state_ == ASSIGNED OR state_ == READY,
            "ESPs not assigned or not accepting service requests");

  // For SeaMonster queries we count the number of fixup requests sent
  // and wait for SM replies to those requests before returning from
  // this function. The replies are for synchronization purposes. By
  // waiting for SM replies we avoid problems related to the fact that
  // an ESP reply on the control connection is considered "out of
  // band" to SM.
  //
  // One SM reply is sent from ESP to master each time an ESP
  // successfully fixes up a fragment. There is no interesting content
  // in these messages. But they need to flow from ESP to master as an
  // SM "go message".
  //
  // Without these replies flowing from ESP to master, if the master
  // were to send a data request to the ESP, the receiving SM service
  // might report that ESP preposts are not yet available.
  Int64 smQueryID = glob_->getSMQueryID();
  UInt32 smFixupMsgsSent = 0;
  if (smQueryID > 0)
    ExSMGlobals::initFixupReplyCount();

  short rc, savedRc = 0;
  IpcPriority espFixupPriority = 0;
  IpcPriority espExecutePriority = 0;
  NABoolean altpriInEsp = FALSE;

  SessionDefaults *sd = glob_->getContext()->getSessionDefaults();

  altpriInEsp = sd->getAltpriEsp();
  
  IpcPriority myProcessPriority = glob_->getMyProcessPriority();

  if (sd->getEspPriority() > 0)
    espExecutePriority = sd->getEspPriority();
  else if (sd->getEspPriorityDelta() != 0)
    espExecutePriority = myProcessPriority + sd->getEspPriorityDelta();
  else
    espExecutePriority = myProcessPriority;

  NABoolean compressFrag;

  // At times during this download we take note of whether the IPC
  // heap has become full. Initially we consider it "not full" by
  // setting this flag to FALSE.
  glob_->getIpcEnvironment()->setHeapFullFlag(FALSE);

  // Allow testing of code that handles corrupt message headers.
  bool testCorruptMessage = glob_->getIpcEnvironment()->getCorruptDownloadMsg();

  if (smQueryID > 0)
    dumpSMRouteTable();

  // broadcast one message per ESP fragment
  bool abortFixup = false;
  NABoolean timedOut;
  for (CollIndex frag = 0; frag < fragmentEntries_.entries(); frag++)
    {
      if (fragDir_->getType(frag) != ExFragDir::ESP)
        continue;

      // We wait for half of our I/Os to complete if the IPC heap
      // became full during the previous iteration of this loop
      if (!abortFixup && glob_->getIpcEnvironment()->getHeapFullFlag())
        {
          Lng32 numOut = numLoadFixupMsgesOut_;
          Lng32 half = numOut / 2;
          while (numLoadFixupMsgesOut_ > half && getState() != ERROR)
            {
	      glob_->getIpcEnvironment()->getAllConnections()->waitOnAll();
              workOnRequests();
            }
          
          if (getState() == ERROR)
            abortFixup = true;
          else
            glob_->getIpcEnvironment()->setHeapFullFlag(FALSE);
        }

      // this fragment gets downloaded to an ESP, build a load message
      // for it and also attach all the DP2 fragments that need to
      // come along
      ExRtFragTableEntry *fragEntry = fragmentEntries_[frag];

      ExMasterEspMessage *mm = NULL;

      // add all ESPs that are supposed to get the message as recipients
      // and see how much those ESPs know already
      NABoolean objectIsAlreadyDownloaded = TRUE;
      for (CollIndex i = 0; i < (CollIndex)fragEntry->numEsps_; i++)
        {
          // abort fixup msg due to error
          if (abortFixup)
            break;

          if (fragEntry->assignedEsps_.used(i))
            {
              ExRtFragInstance *inst = fragEntry->assignedEsps_[i];
              switch (inst->state_)
                {
                case ESP_ASSIGNED:
                  // this ESP hasn't heard about our fragment
                  objectIsAlreadyDownloaded = FALSE;
                  // pump up the priority for fixup, bring it down when done
                  // Note that we don't change ESP priority on Linux,
                  // see above, so the if test will fail.
                  if (espFixupPriority > 0 && !altpriInEsp)
                    {
                      // This is the first interaction with this assigned ESP.
                      // if esp has died, changePriority() should return error.
                      rc = inst->usedEsp_->getIpcServer()
                        ->castToIpcGuardianServer()
                        ->changePriority(espFixupPriority, FALSE);
                      if (rc)
                        {
                          if (!savedRc)
                            // save first error
                            savedRc = rc;

                          if (rc == FENOTFOUND || rc == FEPATHDOWN )
                            {
                              // ESP died or that CPU is down
                              IpcConnection *controlConn = inst->usedEsp_
                                ->getIpcServer()->getControlConnection();

                              ComDiagsArea *da = glob_->getDiagsArea();
                              if (!da)
                                {
                                  da = ComDiagsArea::allocate(glob_->getDefaultHeap());
                                  glob_->setGlobDiagsArea(da);
                                  da->decrRefCount();
                                }
                              *da << DgSqlCode(-EXE_ESP_CHANGE_PRIORITY_FAILED)
                                  << DgInt0(rc);
                              controlConn->getOtherEnd().addProcIdToDiagsArea(*da, 0);

                              controlConn->setState(IpcConnection::ERROR_STATE);
                              abortFixup = true;

                              // continue to next esp
                              continue;
                            }

                          // for other errors return from changePriority(),
                          // ignore them and only add a warning.
                        } // if (rc)
                    } // if (espFixupPriority > 0 && !altpriInEsp)

                  // fall through into the next case

                case DOWNLOADED:
                case FIXED_UP:
                  // ESP already knows about this fragment, but send
                  // it a message anyway to force fixup again
                  if (!mm)
                    mm = new(glob_->getIpcEnvironment()->getHeap())
                      ExMasterEspMessage(glob_->getIpcEnvironment(), this);
                  mm->addRecipient(inst->usedEsp_->getIpcServer()->
                                   getControlConnection());
                  inst->numControlMessages_++;
                  inst->usedEsp_->clearIdleTimestamp();
                  break;

                case UNASSIGNED:
                case DOWNLOADING:
                case FIXING_UP:
                case LOST_CONNECTION:
                default:
                  ex_assert(0,"Invalid state for an ESP to download");
                }
            } // fragment instance is present
          else
            {
              // some day we might support allocating ESPs on demand, but
              // until then we ought to have them all there
              ex_assert(0,"No support for partial allocation of ESPs yet");
            }
        } // for each ESP assigned to this esp fragment

      if (abortFixup)
        {
          if (mm)
            NADELETEBASIC(mm, glob_->getIpcEnvironment()->getHeap());

          // when we decide to abort fixup due to error, we must change the
          // state of all remaining esp frag instances (in current fragment
          // and remaining fragments) to LOST_CONNECTION. this ensures that
          // we won't send any msgs to those esps in future, such as release
          // work msg. otherwise esp may receive a release work msg without
          // receiving the fixup msg and result in assert.
          //
          // note that changing esp frag instance state to LOST_CONNECTION
          // won't result in the esp to be killed unnecessarily. An esp is
          // killed only when its connection is in ERROR state - for more info
          // see ExEspManager::releaseEsp().
          for (CollIndex i = 0; i < (CollIndex)fragEntry->numEsps_; i++)
            {
              if (fragEntry->assignedEsps_.used(i))
                {
                  ExRtFragInstance *inst = fragEntry->assignedEsps_[i];
                  // mark this esp instance so we will never send msg to it
                  inst->state_ = LOST_CONNECTION;
                }
              else
                {
                  ex_assert(0, "ESP instance not found");
                }
            }

          // continue to next fragment
          continue;
        }

      // add the load request to the message, if needed
      if (NOT objectIsAlreadyDownloaded)
        {
          compressFrag = (fragDir_->isCompressFrag(frag))? TRUE: FALSE;
          addLoadRequestToMessage(mm,frag,TRUE,compressFrag);

          // pick up DP2 fragments as well
          for (CollIndex dp2Frag = 0;
               dp2Frag < fragmentEntries_.entries();
               dp2Frag++)
            {
              if (fragDir_->getType(dp2Frag) == ExFragDir::DP2 AND
                  fragDir_->getParentId(dp2Frag) == frag)
                addLoadRequestToMessage(mm,dp2Frag,FALSE,compressFrag);
            }
        }

      // add the fixup request to the message
      Lng32 maxPollingInterval =  sd->getMaxPollingInterval();
      Lng32 persistentOpens = sd->getPersistentOpens();
      NABoolean espCloseErrorLogging =  sd->getEspCloseErrorLogging();
      Lng32 espFreeMemTimeout = sd->getEspFreeMemTimeout();
      addFixupRequestToMessage(mm,frag, 
                               (altpriInEsp ? espFixupPriority : 0),
                               (altpriInEsp ? espExecutePriority : 0),
                               maxPollingInterval,
                               persistentOpens,
                               espCloseErrorLogging,
                               espFreeMemTimeout);

      // for each recipient add one to the number of outstanding fixup
      // messages
      Int32 numRecipients = (Int32) mm->getRecipients().entries();
      numLoadFixupMsgesOut_ += numRecipients;

      if (smQueryID > 0)
      {
        smFixupMsgsSent += numRecipients;
        EXSM_TRACE(EXSM_TRACE_MAIN_THR, "smFixupMsgsSent is now %d",
                   smFixupMsgsSent);
      }
      
      if (fragDir_->needsTransaction(frag))
        {
          numTransactionalMsgesOut_ += mm->getRecipients().entries();
          mm->markAsTransactionalRequest();
        }

      if (testCorruptMessage)
        mm->corruptMessage();

      // send the load/fixup message to all ESPs involved but don't
      // wait until it's completed.
      addRequestToBeSent(mm);

      // mm now lives in the list of active requests

      if (sv_exe_abandon_on_error)
      {
        while (numLoadFixupMsgesOut_ > 0)
          {
            if (getState() == ERROR)
              {
                abortFixup = true;
                abandonPendingIOs();
                break;
              }
              else
                // Bug 3290
                // Complete all the messages for the current fragment/stream
                // before sending messages for the next fragment/stream.
                glob_->getIpcEnvironment()->getAllConnections()->waitOnAll();
          }
      }
      else  // Don't kill working ESPS (CR 4914)
      {
        while (numLoadFixupMsgesOut_ > 0)
          {
                // Bug 3290
                // Complete all the messages for the current fragment/stream
                // before sending messages for the next fragment/stream.
                if (getState() == ERROR)
                  abortFixup = true;
                timedOut = FALSE;
                glob_->getIpcEnvironment()->getAllConnections()->waitOnAll(1000, FALSE, &timedOut); //wait for up to 10 seconds
                if (abortFixup && timedOut)
                {
                  abandonPendingIOs();
                  break;
                }
          }
      }
    } // for each fragment of type ESP

  if (abortFixup == true)
    {
      SQLMXLoggingArea::logExecRtInfo(NULL, 0, "Aborted Fixup due to error", 0);
    }

  // For SeaMonster queries, wait for all SeaMonster fixup replies. We
  // do not use normal IPC wait here, we just have a loop that
  // sleeps. This may need to change in the future. For now the sleep
  // seems acceptable because it's rare we have to wait long at all.
  if (smQueryID > 0 && 
      getState() != ERROR &&
      ((!glob_->getDiagsArea())  ||
        (glob_->getDiagsArea()->mainSQLCODE() >= 0)))
  {
    int timeoutInSeconds = 60;
    int secondsRemaining = timeoutInSeconds;
    int count = 0;

    ExSMGlobals *smGlobals = ExSMGlobals::GetExSMGlobals();

    while ((ExSMGlobals::getFixupReplyCount() < smFixupMsgsSent)  &&
           (smGlobals->getReaderThreadState() != ExSMGlobals::TERMINATED_DUE_TO_ERROR))
    {
      if (count == 0)
        EXSM_TRACE(EXSM_TRACE_MAIN_THR, "Fixup replies: %d",
                   ExSMGlobals::getFixupReplyCount());
      
      // Sleep for .01 seconds
      useconds_t usec = 10000;
      usleep(usec);
      
      // Decrement the timeout value every 100 iterations
      count++;
      if (count == 100)
      {
        count = 0;
        secondsRemaining--;
      }

      // Report an error if the timeout value reaches zero
      if (secondsRemaining == 0)
      {
        // Note: After this query fails if we quickly process a new
        // query, one potential problem is a late fixup reply
        // associated with the OLD query, causing us to bump the reply
        // counter during fixup of the NEW query.

        UInt32 replyCount = (UInt32) ExSMGlobals::getFixupReplyCount();
        CliGlobals *cliGlobals = glob_->getContext()->getCliGlobals();
        const char *processName = cliGlobals->myProcessNameString();

        EXSM_TRACE(EXSM_TRACE_MAIN_THR,
                   "Timeout waiting for fixup replies: "
                   "seconds %d expected %d actual %d",
                   (int) timeoutInSeconds, (int) smFixupMsgsSent,
                   (int) replyCount);

        ComDiagsArea *da = glob_->getDiagsArea();
        if (!da)
        {
          da = ComDiagsArea::allocate(glob_->getDefaultHeap());
          glob_->setGlobDiagsArea(da);
          da->decrRefCount();
        }
        
        *da << DgSqlCode(-EXE_SM_FIXUP_REPLY_TIMEOUT)
            << DgString0(processName)
            << DgInt0((Lng32) timeoutInSeconds)
            << DgInt1((Lng32) smFixupMsgsSent)
            << DgInt2((Lng32) replyCount);
        
        EXSM_TRACE(EXSM_TRACE_MAIN_THR, "Setting frag table state to ERROR");
        state_ = ERROR;
        break;

      } // if timeout == 0
    } // while actual < expected and reader thread not terminated

    EXSM_TRACE(EXSM_TRACE_MAIN_THR, "Fixup replies received: %d",
               ExSMGlobals::getFixupReplyCount());
    
    if (smGlobals->getReaderThreadState() == ExSMGlobals::TERMINATED_DUE_TO_ERROR)
    {
      EXSM_TRACE(EXSM_TRACE_MAIN_THR, "Reader thread state is TERMINATED");
      EXSM_TRACE(EXSM_TRACE_MAIN_THR, "Setting frag table state to ERROR");
      smGlobals->addReaderThreadError(glob_);
      state_ = ERROR;
    }
    
  } // if (smQueryID > 0 && getState() != ERROR)

  if (getState() != ERROR)
     workOnRequests();

  glob_->getIpcEnvironment()->releaseSafetyBuffer();

  // Note that we don't change ESP priority on Linux, see above,
  // so savedRc will remain 0
  if (savedRc)  // report change priority error as warning
    {
   }
}

short ExRtFragTable::restoreEspPriority()
{
  short rc, savedRc = 0;


  return savedRc;
}

void ExRtFragTable::assignPartRangesAndTA(NABoolean /*initial*/)
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  // don't do anything if we are in the wrong state or if we aren't
  // supposed to send any more transaction-bearing messages or
  // if the root node doesn't have a down queue request
  if (state_ != READY OR quiesce_ OR numRootRequests_ <= 0)
    return;

  CollHeap *ipcHeap = glob_->getIpcEnvironment()->getHeap();

  // send one message to each fragment instance that is downloaded to an
  // ESP and that isn't the only instance of its fragment
  for (ExFragId fragId = 0;
       fragId < (ExFragId) fragDir_->getNumEntries();
       fragId++)
    {
      if (fragDir_->getType(fragId) == ExFragDir::ESP)
	{
	  ExRtFragTableEntry *entry = fragmentEntries_[fragId];

	  ex_assert(entry != NULL,
		    "ESPs for fragment not assigned before part. assignment");

	  ExPartInputDataDesc *partDesc = entry->partDesc_;

	  // we need a transaction for instances now if the application
	  // has started to execute a query and if the instances do
	  // an operation that require a transaction
	  NABoolean needsTransaction = fragDir_->needsTransaction(fragId);

	  // make sure the partDesc is ready to generate partition input values
	  if (partDesc)
	    {
	      ComDiagsArea *da = glob_->getDiagsArea();
	      partDesc->evalExpressions(
		   glob_->getSpace(),
		   glob_->getDefaultHeap(),
		   &da);
	      glob_->setGlobDiagsArea(da);
              if (da AND da->mainSQLCODE() < 0)
                {
                  // failed to evaluate expressions
                  return;
                }
	    }

	  // send or resend work requests and partition input data
	  // to all instances
	  for (CollIndex espNum = 0;
	       espNum < entry->assignedEsps_.entries();
	       espNum++)
	    {
	      ExRtFragInstance *inst = entry->assignedEsps_[espNum];

	      // Do we need to send a work message so the ESP has
	      // an outstanding control message to reply with diagnostics
	      // and/or statistics?
	      NABoolean needToSendWorkMessage =	(NOT inst->workMessageSent_);

	      // Do we need to make a static or a dynamic assignment of
	      // partition input values?
	      NABoolean needToSendPIVs = (NOT inst->partInputDataAssigned_);

	      if (needToSendWorkMessage OR needToSendPIVs)
		{
		  // send a transaction if the ESP needs one and if there are
		  // no outstanding work messages
		  NABoolean needToSendTransaction =
		    (needsTransaction AND needToSendWorkMessage);
		  
		  NABoolean needToSendStaticPIVs =
		    (needToSendPIVs AND
		     NOT entry->dynamicLoadBalancing_);
		  
		  NABoolean needToSendDynamicPIVs =
		    (needToSendPIVs AND
		     entry->dynamicLoadBalancing_ AND
		     (Lng32)entry->assignedPartInputValues_.entries() <
		     partDesc->getNumPartitions());

                  ExMasterEspMessage *workMsg = NULL;
		  ExMasterEspMessage *pivMsg = NULL;

		  ex_assert(needToSendWorkMessage OR NOT needToSendStaticPIVs,
			    "Static PIVs need to be sent in first work msg");

		  // prepare a work message if necessary, it will
		  // also transport static PIVs if needed
		  if (needToSendWorkMessage)
		    {
		      workMsg = new(ipcHeap) ExMasterEspMessage(
			   glob_->getIpcEnvironment(),
			   this);
		      
		      workMsg->addRecipient(inst->usedEsp_->getIpcServer()->
					    getControlConnection());
		      pivMsg = workMsg; // pivs piggyback on work message
		    }

		  // dynamic PIVs always travel in a separate message
		  // (they could even be sent by another process)
		  if (needToSendDynamicPIVs)
		    {
		      // Use a separate message stream to send dynamic
		      // load balancing messages, this makes it easier
		      // to have another process take over this chore
		      // sometimes in the future
		      pivMsg = new(ipcHeap) ExMasterEspMessage(
			   glob_->getIpcEnvironment(),
			   this);
		      
		      pivMsg->addRecipient(inst->usedEsp_->getIpcServer()->
					   getControlConnection());
		    }

		  if (needToSendPIVs)
		    {
		      // prepare an input data request header
		      // (looks the same for all instances)
		      ExEspPartInputDataReqHeader *preq =
			new(ipcHeap) ExEspPartInputDataReqHeader(ipcHeap);
		      
		      preq->key_ = masterFragmentInstanceKey_;
		      preq->key_.setFragId(fragId); // change to actual key
		      preq->staticAssignment_ =
			NOT entry->dynamicLoadBalancing_;
		      preq->askForMoreWorkWhenDone_ =
			entry->dynamicLoadBalancing_;
		      
		      pivMsg->markAsAssignPartRequest();
		      
		      // add the request to the message
		      *pivMsg << *preq;
		      preq->decrRefCount();
		      preq = NULL;
	      
		      Lng32 dataLen = partDesc->getPartInputDataLength();
		  
		      // now add the actual data to the message
		      TupMsgBuffer *buf =
			new(ipcHeap) 
			TupMsgBuffer((Lng32) 
				     SqlBufferNeededSize(1,dataLen,
							 SqlBuffer::NORMAL_),
				     TupMsgBuffer::MSG_IN,
				     ipcHeap);
		      // the buffer contains a single tupp
		      tupp_descriptor *tuppd =
			buf->get_sql_buffer()->add_tuple_desc(dataLen);
		      // initialize the tupp with the actual part. input values
		      // $$$$ need to pick an index into the partition input
		      // values if we do dynamic load balancing
		      Lng32 loPart = -1;
		      Lng32 hiPart = -1;
		      if (needToSendDynamicPIVs)
			{
			  ex_assert(0,"Dynamic load balancing not implemented");
			  // NOTE: when implementing this, use a separate
			  // IpcMessageStream to send the dynamic request!!!
			}
		      else
			{
			  // without dyn. load balancing, take slot #espNum
			  // from the partition input values
			  // (no need to maintain the bit vector) and
			  // send the request along with the work request
			  loPart = hiPart = (Lng32) espNum;
			}
		      partDesc->copyPartInputValue(loPart,
						   hiPart,
						   tuppd->getTupleAddress(),
						   dataLen);
		      *pivMsg << *buf;
		      buf->decrRefCount();
		      buf = NULL;
		  
		      // remember that the fragment instance now has
		      // partition input values assigned to it
		      inst->partInputDataAssigned_ = TRUE;

		      if (pivMsg != workMsg)
			{
			  // TBD for dynamic load balancing: send pivs off $$$$
			  ex_assert(0,"TBD: send piv msg");
			  if (needsTransaction)
			    {
			      numTransactionalMsgesOut_++;
			      pivMsg->markAsTransactionalRequest();
			    }
			}
		  
		    } // needToSendPartInputValues
	      
		  if (needToSendWorkMessage)
		    {
		      // prepare a transaction work request header
		      // (looks the same for all instances)
		      ExEspWorkReqHeader *treq =
			new(ipcHeap) ExEspWorkReqHeader(ipcHeap);
		  
		      treq->key_ = masterFragmentInstanceKey_;
		      treq->key_.setFragId(fragId); // change to actual key

		      workMsg->markAsWorkTransRequest();
		      
		      // add the request to the message
		      *workMsg << *treq;
		      treq->decrRefCount();
		      treq = NULL;
		  
		      if (needToSendTransaction)
			{
			  ExMsgTransId *msgTransId =
			    new(ipcHeap) ExMsgTransId
                                   (ipcHeap,
                                    glob_->getTransid(),
                                    (short *)&(inst->usedEsp_->getIpcServer()->
                                        getServerId().getPhandle().phandle_));

			  *workMsg << *msgTransId;
			  
			  msgTransId->decrRefCount();
			  msgTransId = NULL;
			}

		      // finally, send the work message off
		      inst->numControlMessages_++;
		      if (needToSendWorkMessage)
                        {
			  inst->workMessageSent_ = TRUE;
			  inst->workMsg_ = workMsg;
                        }
		      numWorkMsgesOut_++;
		      if (needsTransaction)
			{
			  numTransactionalMsgesOut_++;
			  workMsg->markAsTransactionalRequest();
			}
		      addRequestToBeSent(workMsg);
		    } // need to send work message
		} // need to send some message
	    } // send a partition input data message to all instances
	} // fragment type is ESP
    } // for each fragment

#ifdef IPC_INTEGRITY_CHECKING
    checkIntegrity();
#endif
}

void ExRtFragTable::releaseTransaction(NABoolean allWorkRequests,
				       NABoolean alwaysSendReleaseMsg)
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  if (state_ == NO_ESPS_USED)
    return;

  CollHeap *ipcHeap = glob_->getIpcEnvironment()->getHeap();

  // mark the fragment dir as in the quiescing state, don't send any
  // new transaction work requests
  quiesce_ = TRUE;

  // Send a transaction release request after each outstanding transaction
  // message, indicating to the server to reply to both the release message
  // and the work request.

  Lng32 inactiveTimeout = getEspInactiveTimeout();

  // loop over fragments
  for (ExFragId fragId = 0;
       fragId < (ExFragId)fragDir_->getNumEntries();
       fragId++)
    {
      NABoolean needsTransaction = fragDir_->needsTransaction(fragId);
      if (fragDir_->getType(fragId) == ExFragDir::ESP AND
	  (allWorkRequests OR needsTransaction))
	{
	  ExRtFragTableEntry *entry = fragmentEntries_[fragId];

	  // loop over fragment instances
	  for (CollIndex espNum = 0;
	       espNum < entry->assignedEsps_.entries();
	       espNum++)
	    {
	      ExRtFragInstance *inst = entry->assignedEsps_[espNum];

	      if (((inst->numControlMessages_ > 0) ||
		   (alwaysSendReleaseMsg)) AND
		  NOT inst->releasingWorkMsg_ AND
		  inst->state_ != LOST_CONNECTION)
		{
                  // tom - in case of ipc error if all receive callbacks were
                  // invoked properly then all dead esps should have their
                  // corresponding inst->state_ set to LOST_CONNECTION and
                  // we shouldn't need to do following. but for now it's better
                  // to be safe. we may remove following code later.
                  IpcServer *ipcs = inst->usedEsp_->getIpcServer();
                  IpcConnection *conn = ipcs->getControlConnection();
                  if (conn->getState() == IpcConnection::ERROR_STATE)
                    {
                      inst->state_ =LOST_CONNECTION;
                      continue;
                    }
                  else if (glob_->verifyESP())
                    {
                      if (ipcs->castToIpcGuardianServer() &&
                          ipcs->castToIpcGuardianServer()->serverDied())
                        {
                          // this esp has died
                          conn->setState(IpcConnection::ERROR_STATE);
                          inst->state_ = LOST_CONNECTION;
                          continue;
                        }
                    }
                  // tom - end of checking dead esps

		  // send a release message to this instance
		  ExMasterEspMessage *msg =
		    new(ipcHeap) ExMasterEspMessage(
			 glob_->getIpcEnvironment(),
			 this);

		  msg->addRecipient(conn);
		  msg->markAsReleaseTransRequest();

		  ExEspReleaseWorkReqHeader *treq =
		    new(ipcHeap) ExEspReleaseWorkReqHeader(ipcHeap);

		  treq->key_ = masterFragmentInstanceKey_;
		  treq->key_.setFragId(fragId); // change to actual key
		  treq->allWorkRequests_ = allWorkRequests;
		  treq->inactiveTimeout_ = (Int32) inactiveTimeout;

		  *msg << *treq;
		  treq->decrRefCount();

		  inst->numControlMessages_++;
		  inst->releasingWorkMsg_ = TRUE;
		  numWorkMsgesOut_++;
		  if (needsTransaction)
		    {
		      numTransactionalMsgesOut_++;
		      msg->markAsTransactionalRequest();
		    }
		  addRequestToBeSent(msg);
		} // need to send release message
	    } // loop over instances
	} // fragment is ESP fragment and needs transaction
    } // loop over fragments

#ifdef IPC_INTEGRITY_CHECKING
    checkIntegrity();
#endif
}

void ExRtFragTable::continueWithTransaction()
{
  if (state_ == NO_ESPS_USED)
    return;

  // frag dir is no longer in quiescing state
  quiesce_ = FALSE;

  // now we can send some more messages out and get the work going again
  assignPartRangesAndTA(FALSE);
}

void ExRtFragTable::releaseEsps(NABoolean closeAllOpens)
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif
  CollHeap *ipcHeap = glob_->getIpcEnvironment()->getHeap();

  if (state_ == NOT_ASSIGNED OR state_ == NO_ESPS_USED)
    return;

  // First, release all outstanding work messages. Releasing the
  // ESPs while there are outstanding work messages complicates things
  // because TMF does not tolerate outstanding transactional messages.
  // The caller should wait for all messages to complete before
  // committing.
  releaseTransaction(TRUE, FALSE);

  ExMasterEspMessage *msg = NULL;
  SET(ExEspDbEntry *) releasedEsps(glob_->getDefaultHeap());
  
  Lng32 idleTimeout = getEspIdleTimeout();
  Lng32 stopIdleEspsTimeout = getStopIdleEspsTimeout();

  // broadcast one message to all ESPs that know about the statement
  for (CollIndex frag = 0; frag < fragmentEntries_.entries(); frag++)
    if (fragDir_->getType(frag) == ExFragDir::ESP)
      {
	// this fragment uses ESPs
	ExRtFragTableEntry *fragEntry = fragmentEntries_[frag];

	// add all ESPs that are supposed to get the message as recipients
	for (CollIndex inst = 0;
	     inst < (CollIndex)fragEntry->numEsps_;
	     inst++)
	  {
	    if (fragEntry->assignedEsps_.used(inst))
	      {
		ExRtFragInstance *fragInst = fragEntry->assignedEsps_[inst];

                // tom - in case of ipc error if all receive callbacks were
                // invoked properly then all dead esps should have their
                // corresponding inst->state_ set to LOST_CONNECTION and
                // we shouldn't need to do following. but for now it's better
                // to be safe. we may remove following code later.
                if (fragInst->state_ != LOST_CONNECTION)
                  {
                    IpcServer *ipcs = fragInst->usedEsp_->getIpcServer();
                    IpcConnection *conn = ipcs->getControlConnection();
                    if (conn->getState() == IpcConnection::ERROR_STATE)
                      {
                        fragInst->state_ =LOST_CONNECTION;
                      }
                    else if (glob_->verifyESP())
                      {
                        if (ipcs->castToIpcGuardianServer() &&
                            ipcs->castToIpcGuardianServer()->serverDied())
                          {
                            // this esp has died
                            conn->setState(IpcConnection::ERROR_STATE);
                            fragInst->state_ = LOST_CONNECTION;
                          }
                      }
                  }
                // tom - end of checking dead esps

		switch (fragInst->state_)
		  {
		  case ESP_ASSIGNED:
		  case DOWNLOADED:
		  case FIXED_UP:
		  case DOWNLOADING:
		  case FIXING_UP:
		    // ESP knows about this fragment
                    if (!msg)
                      msg = new(ipcHeap) ExMasterEspMessage(glob_->getIpcEnvironment(), this);
                    msg->addRecipient(fragInst->usedEsp_->getIpcServer()->getControlConnection());
		    break;

		  case UNASSIGNED:
		  case LOST_CONNECTION:
		    // do nothing, ESP doesn't know the fragment
		    break;

		  default:
		    ex_assert(0, "Invalid frag instance state");
		  }

		// release the ESP, if one is assigned
		// multi fragment esp - begin 
		if (fragInst->usedEsp_) {
		  if ((releasedEsps.insert(fragInst->usedEsp_) == FALSE) && 
		      fragDir_->espMultiFragments()) {
		    // decrement the usageCount_ of fragInst->usedEsp_
		    glob_->getEspManager( )->releaseEsp(fragInst->usedEsp_, glob_->verifyESP(), fragInst->usedEsp_->inUse());
		  }
		  if (idleTimeout > 0)
		    // esp idle timeout is turned on
		    fragInst->usedEsp_->setIdleTimestamp();
		}
		// multi fragment esp - end
		fragInst->deleteMe();
                fragEntry->assignedEsps_.remove(inst);
	      } // fragment instance is present
	  } // for each ESP assigned to this fragment of type ESP

      } // for each fragment of type ESP

  if (msg)
    {
      addReleaseRequestToMessage(msg, 0, idleTimeout, TRUE, closeAllOpens);

      // count the outgoing transactional messages (this is the
      // reason why we use a separate message for transactional
      // release messages)
      numReleaseEspMsgesOut_ += (Lng32) (msg->getRecipients().entries());

      // send the release message to all ESPs in nowait mode
      addRequestToBeSent(msg);
    }

#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  // finally, after sending the release messages, detach from the
  // ESPs
  CollIndex numEsps = releasedEsps.entries();

  for (CollIndex e = 0; e < numEsps; e++)
    glob_->getEspManager()->releaseEsp(releasedEsps[e], 
                glob_->verifyESP(), releasedEsps[e]->inUse());

#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif
}

void ExRtFragTable::releaseAssignedEsps()
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity( );
#endif
  CollHeap *ipcHeap = glob_->getIpcEnvironment( )->getHeap( );

  if ( state_ == NOT_ASSIGNED OR state_ == NO_ESPS_USED )
    return;

  SET( ExEspDbEntry * ) releasedEsps( glob_->getDefaultHeap( ) );

  // traverse each fragment
  for ( CollIndex frag = 0; frag < fragmentEntries_.entries( ); frag++ )
    if ( fragDir_->getType( frag ) == ExFragDir::ESP )
      {
        // this fragment uses ESPs
        ExRtFragTableEntry *fragEntry = fragmentEntries_[frag];

        // add all ESPs that are to be released
        for ( CollIndex inst = 0;
              inst < (CollIndex)fragEntry->numEsps_;
              inst++ )
        {
          if ( fragEntry->assignedEsps_.used( inst ) )
            {
              ExRtFragInstance *fragInst = fragEntry->assignedEsps_[inst];

              // release the ESP, if one is assigned
              if ( fragInst->usedEsp_ )
                releasedEsps.insert( fragInst->usedEsp_ );
              fragInst->deleteMe( );
              fragEntry->assignedEsps_.remove( inst );
            } // fragment instance is present
        } // for each ESP assigned to this fragment of type ESP
      } // for each fragment of type ESP

#ifdef IPC_INTEGRITY_CHECKING
    checkIntegrity( );
#endif

  // detach from the ESPs
  CollIndex numEsps = releasedEsps.entries( );

  for ( CollIndex e = 0; e < numEsps; e++ )
    glob_->getEspManager( )->releaseEsp(releasedEsps[e], glob_->verifyESP()
              , releasedEsps[e]->inUse());

#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity( );
#endif
}

const IpcProcessId & ExRtFragTable::getInstanceProcessId(
     ExFragId fragId,
     CollIndex espNum) const
{
  return fragmentEntries_[fragId]->assignedEsps_[espNum]->
    usedEsp_->getIpcServer()->getServerId();
}

Lng32 ExRtFragTable::getNumOfInstances(ExFragId fragId) const
{
  return fragmentEntries_[fragId]->numEsps_;
}

IpcConnection * ExRtFragTable::getControlConnection(ExFragId fragId,
						    CollIndex espNum) const
{
  return fragmentEntries_[fragId]->assignedEsps_[espNum]->
    usedEsp_->getIpcServer()->getControlConnection();
}

ExFragInstanceHandle ExRtFragTable::getFragmentHandle(
     ExFragId fragId,
     CollIndex espNum) const
{
  return fragmentEntries_[fragId]->assignedEsps_[espNum]->fragmentHandle_;
}

NABoolean ExRtFragTable::isLocal(const IpcProcessId & /*procId*/) const
{
  return FALSE; // $$$$ for now
}

void ExRtFragTable::addRequestToBeSent(ExMasterEspMessage *m)
{
  outstandingServiceRequests_.insert(m);

  // set the reply buffer length to 400 bytes for all control msgs (fixup,
  // work, release work, release esp) in order to reduce ipc memory usage.
  // in normal situations all control replies should be just over 200 bytes.
  // if the reply contains error info that exceeds 400 bytes, the reply will
  // come back in 2 chunks.
  m->setMaxReplyLength(CONTROL_MSG_REPLY_BUFFER_LENGTH);

  // call send with context transid.
  // Use TX's TCBREF in messages to ESP. If the ESP is on remote node,
  // TCBREF value will be massaged to get actual transaction id
  // by calling Fs2_transid_to_buffer by IPC code.
  m->send(FALSE, glob_->getContext()->getTransaction()->getExeXnId());
}

NABoolean ExRtFragTable::removeCompletedRequest(ExMasterEspMessage *m)
{
  return outstandingServiceRequests_.remove(m);
}

ExWorkProcRetcode ExRtFragTable::workOnRequests()
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif
  // check without waiting on all of the outstanding nowait requests,
  // let the callbacks handle the rest
  for(CollIndex i = 0; i < outstandingServiceRequests_.entries(); i++)
    outstandingServiceRequests_[i]->waitOnMsgStream(IpcImmediately);

  // Try to send more transaction work requests and partition input
  // data messages to fragment instances, this may go on while we are
  // executing the query.
  // Also try to modify the state of the fragment directory if necessary.
  assignPartRangesAndTA(FALSE);

#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  if (state_ == ERROR)
    return WORK_BAD_ERROR;
  else
    return WORK_OK;
}

void ExRtFragTable::addLoadRequestToMessage(ExMasterEspMessage *msg,
					    ExFragId fragId,
					    NABoolean addHeader,
					    NABoolean compressFrag)
{
  CollHeap *ipcHeap = glob_->getIpcEnvironment()->getHeap();

  if (addHeader)
    {
      ExEspLoadFragmentReqHeader *req =
	new(ipcHeap) ExEspLoadFragmentReqHeader(ipcHeap);
  
      // add the download request to the message
      *msg << *req;
      req->decrRefCount();
    }

  // Retrieve the user details from ContextCli. These values will become part
  // of the ExMsgFragment message object that gets sent to the ESP. 
  Int32 userID = 0;
  const char *userName = NULL;
  Lng32 userNameLen = 0;
  ContextCli *context = glob_->getContext();
  Int32 *pUserID = context->getDatabaseUserID();
  userID = *((Int32 *) pUserID);
  userName = context->getDatabaseUserName();
  userNameLen = (Int32) strlen(userName) + 1;
#ifdef _DEBUG
  if (fragDir_->getType(fragId) == ExFragDir::ESP)
  {
    NABoolean doDebug = (getenv("DBUSER_DEBUG") ? TRUE : FALSE);
    if (doDebug)
      printf("[DBUSER:%d] Sending ESP fragment %d, user ID %d, name [%s]\n",
             (int) getpid(), (int) fragId, (int) userID, userName);
  }
#endif

  // add the actual fragment to the message
  ExFragKey fkey(masterFragmentInstanceKey_);
  fkey.setFragId(fragId); // change to actual frag instance key
  ExMsgFragment *msgFrag = new(ipcHeap) ExMsgFragment(
       fkey,
       fragDir_->getType(fragId),
       fragDir_->getParentId(fragId),
       fragDir_->getTopNodeOffset(fragId),
       fragDir_->getFragmentLength(fragId),
       generatedObject_ + fragDir_->getGlobalOffset(fragId),
       glob_->getNumTemps(),
       (short) ComVersion_GetMXV(),
       (short) fragDir_->getPlanVersion(),
       (NABoolean) fragDir_->needsTransaction(fragId),
       glob_->getInjectErrorAtExpr(),
       ipcHeap,
       FALSE,
       isGuiDisplayActive(),
       glob_->getStatement()->getUniqueStmtId(),
       glob_->getStatement()->getUniqueStmtIdLen(),
       userID,
       userName,
       userNameLen,
       (compressFrag)? 0: 0xffffffff);
       // last parameter is compressed length. Overload it by setting
       // it to -1 when not compress it

  *msg << *msgFrag;
  msgFrag->decrRefCount();

  msg->markAsDownloadRequest();
}

void ExRtFragTable::addFixupRequestToMessage(ExMasterEspMessage *msg,
					     ExFragId fragId,
					     IpcPriority fixupPriority,
					     IpcPriority executePriority,
					     Lng32 maxPollingInterval,
					     Lng32 persistentOpens,
					     NABoolean espCloseErrorLogging,
					     Lng32 espFreeMemTimeout)
{ 
  CollHeap *ipcHeap = glob_->getIpcEnvironment()->getHeap();
  ExEspFixupFragmentReqHeader *req =
    new(ipcHeap) ExEspFixupFragmentReqHeader(ipcHeap);
  ExRtFragTableEntry *fragEntry = fragmentEntries_[fragId];
  
  req->key_ = masterFragmentInstanceKey_;
  req->key_.setFragId(fragId); // change to actual frag instance key
  // how many instances does the parent fragment have?
  ExFragId parentFragId = fragDir_->getParentId(fragId);
  if (fragDir_->getType(parentFragId) == ExFragDir::MASTER)
    {
      req->numOfParentInstances_ = 1; // the master himself
    }
  else
    {
      ex_assert(fragDir_->getType(parentFragId) == ExFragDir::ESP,
		"A downloaded fragment must be child of a master or ESP");
      req->numOfParentInstances_ =
	fragmentEntries_[fragDir_->getParentId(fragId)]->numEsps_;
    }

  req->setStatsEnabled(glob_->statsEnabled());

  req->setEspFixupPriority(fixupPriority);
  req->setEspExecutePriority(executePriority);
  req->setMaxPollingInterval(maxPollingInterval);
  req->setPersistentOpens(persistentOpens);
  req->setEspCloseErrorLogging(espCloseErrorLogging);
  req->setEspFreeMemTimeout(espFreeMemTimeout);

  // add the fixup request to the message
  *msg << *req;
  req->decrRefCount();
  req = NULL;

  // Add ExProcessIdsOfFrag objects to the message, if needed.  The
  // job of the loop below is to add process IDs to the message. IDs
  // to include are:
  // (a) All processes executing this fragment
  // (b) All processes executing a child ESP fragment
  // (c) For SeaMonster, all processes executing the parent fragment

  // Decide if we are interested in parent fragments
  bool includeParents = false;
  if (glob_->getSMQueryID() > 0)
    includeParents = true;
  
  // Walk the entire list of fragments looking for fragments of interest
  for (CollIndex i = 0; i < (CollIndex)fragDir_->getNumEntries(); i++)
  {
    if (fragId == i OR                                           // (a)
        (fragDir_->getParentId(i) == fragId AND
         fragDir_->getType(i) == ExFragDir::ESP) OR              // (b)
        (includeParents && fragDir_->getParentId(fragId) == i))  // (c)
    {
      // Fragment i is now one of the following:
      // * The fragment being fixed up (i == fragId)
      // * An ESP child of fragId
      // * The parent of fragId

      // Create a message object that will hold a collection of
      // process IDs for all processes executing fragment i
      ExProcessIdsOfFrag *pof = new(ipcHeap) ExProcessIdsOfFrag(ipcHeap,i);
      
      if (fragDir_->getType(i) == ExFragDir::MASTER)
      {
        // Fragment i is the master fragment. Add one process ID.
        const IpcProcessId &myProcessId =
          masterFragmentInstanceKey_.getProcessId();
        pof->addProcessId(myProcessId);
      }
      else
      {
        // Fragment i is an ESP fragment. Add all ESP process IDs.
        ExRtFragTableEntry *entry = fragmentEntries_[i];
        ex_assert(entry != NULL,
                  "ESPs for fragment not assigned before fixup");

        // Add all process IDs to the list of fragment instance IDs
        for (CollIndex espNum = 0;
             espNum < entry->assignedEsps_.entries();
             espNum++)
        {
          pof->addProcessId(entry->assignedEsps_[espNum]->
                            usedEsp_->getIpcServer()->getServerId());
        }
      }
      
      // poof, add the entire thing to the message
      *msg << *pof;

      // we now give up our interest in the created object
      pof->decrRefCount();
    }
  }
  
  // add late name info, if present
  if (getGlobals()->resolvedNameList())
    {
    }

  // add resource info if needed
  if (getGlobals()->getFragDir()->getScratchFileOptions())
    {
      ExMsgResourceInfo *ri =
	new(ipcHeap) ExMsgResourceInfo(
	     getGlobals()->getFragDir()->getScratchFileOptions(),ipcHeap);
      *msg << *ri;
      ri->decrRefCount();
    }

  // add timeout-data if needed
  if ( * getGlobals()->getTimeoutData() )
    {
      ExMsgTimeoutData * td = 
	new(ipcHeap) 
	    ExMsgTimeoutData( * getGlobals()->getTimeoutData(), ipcHeap);
      *msg << * td;
      td->decrRefCount();
    }

 

  // Add SeaMonster info if needed
  if (glob_->getSMQueryID() > 0)
  {
    Int64 smQueryID = glob_->getSMQueryID();
    Int32 traceLevel = glob_->getSMTraceLevel();
    const char *traceFilePrefix = glob_->getSMTraceFilePrefix();
    Int32 flags = 0;

    ExSMDownloadInfo *info = new (ipcHeap) 
      ExSMDownloadInfo(ipcHeap, smQueryID, traceLevel, traceFilePrefix, flags);

    *msg << *info;
    info->decrRefCount();
  }
  
  msg->markAsFixupRequest();
}

void ExRtFragTable::addReleaseRequestToMessage(ExMasterEspMessage *msg,
					       ExFragId fragId,
                                               Lng32 idleTimeout,
					       NABoolean releaseAll,
					       NABoolean closeAllOpens)
{
  CollHeap *ipcHeap = glob_->getIpcEnvironment()->getHeap();

  ExEspReleaseFragmentReqHeader *req =
    new(ipcHeap) ExEspReleaseFragmentReqHeader(ipcHeap);
  
  req->key_        = masterFragmentInstanceKey_;
  req->key_.setFragId(fragId); // change to actual frag instance key
  req->setDeleteStmt(releaseAll);
  req->setCloseAllOpens(closeAllOpens);
  req->idleTimeout_ = (Int32) idleTimeout;

  // add the release request to the message
  *msg << *req;
  req->decrRefCount();

  msg->markAsReleaseRequest();
}

void ExRtFragTable::abandonPendingIOs()
{
  ExMasterEspMessage *m;
  while (outstandingServiceRequests_.getFirst(m))
    // clean up all active IOs from all connections of the stream
    m->abandonPendingIOs();
}

ExRtFragInstance * ExRtFragTable::findInstance(
     ExFragId fragId,
     IpcConnection *connection) const
{
  ExRtFragTableEntry *entry = fragmentEntries_[fragId];
  for (CollIndex i = 0; i < (CollIndex)entry->numEsps_; i++)
    if (entry->assignedEsps_.used(i) AND
	entry->assignedEsps_[i]->usedEsp_->getIpcServer() AND 
        entry->assignedEsps_[i]->usedEsp_->
          getIpcServer()->getControlConnection() == connection)
      return entry->assignedEsps_[i];

  // not found
  return NULL;
}

Lng32 ExRtFragTable::getStopIdleEspsTimeout()
{
  return glob_->castToExMasterStmtGlobals()->getContext()->
    getSessionDefaults()->getEspStopIdleTimeout();
}

Lng32 ExRtFragTable::getEspIdleTimeout()
{
  Lng32 timeout = glob_->getContext()->getSessionDefaults()->getEspIdleTimeout();
  if (timeout <= 0)
    // idle esps never time out
    timeout = 0;

  return timeout;
}

Lng32 ExRtFragTable::getEspInactiveTimeout()
{
  Lng32 timeout = glob_->getContext()->getSessionDefaults()->getEspInactiveTimeout();
  if (timeout <= 0)
    // inactive esps never time out
    timeout = 0;
  else if (timeout <= 60)
    // inactive esps will wait at least 60 seconds before time out
    timeout = 60;

  return timeout;
}

short ExRtFragTable::countSQLNodes(short masterNode)
{
  NABitVector uniqueNodes (glob_->getDefaultHeap());
  uniqueNodes += masterNode;

  for (CollIndex i = 0; i < fragmentEntries_.entries(); i++)
  {
    if (fragDir_->getType(i) == ExFragDir::ESP)
    {
      ExRtFragTableEntry &fragEntry = *fragmentEntries_[i];
      for (Lng32 e = 0; e < fragEntry.numEsps_; e++)
      {
        ExRtFragInstance *inst = fragEntry.assignedEsps_[e];
        if (inst->cpuNum_ != -1)
          uniqueNodes += inst->cpuNum_;
      }
    }
  }
  ex_assert(uniqueNodes.entries() < SHRT_MAX,
            "Too many nodes for ExRtFragTable::countSQLNodes");
  return ((short) uniqueNodes.entries());
}

// Method to dump out the SeaMonster routing table
void ExRtFragTable::dumpSMRouteTable()
{
  EXSM_TRACE(EXSM_TRACE_TAG, "ROUTE TABLE %p", this);
  for (CollIndex frag = 0; frag < fragmentEntries_.entries(); frag++)
  {
    ExFragDir::ExFragEntryType fragType = fragDir_->getType(frag);
    if (fragType == ExFragDir::MASTER)
    {
      CliGlobals *cliGlob = GetCliGlobals();
      int pid = (int) cliGlob->myPin();
      int node = (int) cliGlob->myNodeNumber();
      EXSM_TRACE(EXSM_TRACE_TAG, "  frag[%d]", (int) frag);
      EXSM_TRACE(EXSM_TRACE_TAG, "    MASTER %d:%d", node, pid);

    } // MASTER fragment

    else if (fragType == ExFragDir::ESP)
    {
      Int32 parent = fragDir_->getParentId(frag);
      EXSM_TRACE(EXSM_TRACE_TAG, "  frag[%d] parent %d",
                 (int) frag, (int) parent);

      ExRtFragTableEntry &fragEntry = *fragmentEntries_[frag];
      for (CollIndex i = 0; i < fragEntry.numEsps_; i++)
      {
        int node = 0;
        int cpu = 0;
        int pin = 0;
        SB_Int64_Type seqNum = 0;
        
        ExRtFragInstance *inst = fragEntry.assignedEsps_[i];
        const IpcProcessId &processId =
          inst->usedEsp_->getIpcServer()->getServerId();

        processId.getPhandle().decompose(cpu, pin, node
                                        , seqNum
                                        );
        node = ExSM_GetNodeID(cpu);

        EXSM_TRACE(EXSM_TRACE_TAG, "    ESP %d:%d:%" PRId64, 
                   node, pin, seqNum);

      }

    } // ESP fragment
  } // for each fragment
}

void ExRtFragTable::print()
{
  printf("ExRtFragTable:\n");
  printf("--------------\n");
  printf("Global State: ");

  switch (state_)
    {
    case UNASSIGNED:
      printf("UNASSIGNED\n");
      break;
    case ESP_ASSIGNED:
      printf("ESP_ASSIGNED\n");
      break;
    case DOWNLOADING:
      printf("DOWNLOADING\n");
      break;
    case DOWNLOADED:
      printf("DOWNLOADED\n");
      break;
    case FIXING_UP:
      printf("FIXING_UP\n");
      break;
    case FIXED_UP:
      printf("FIXED_UP\n");
      break;
    case LOST_CONNECTION:
      printf("LOST_CONNECTION\n");
      break;
    default:
      printf("Invalid state!!\n");
      break;
    }

  printf("%d load/fixup, %d work, %d transactional and %d release messages outstanding\n\n",
         numLoadFixupMsgesOut_,
         numWorkMsgesOut_,
         numTransactionalMsgesOut_,
         numReleaseEspMsgesOut_);

  for (CollIndex i=0; i < fragmentEntries_.entries(); i++)
    {
      ExRtFragTableEntry *fragEntry = fragmentEntries_[i];
      Lng32 partInputDataLength = (fragEntry->partDesc_ ?
                                   fragEntry->partDesc_->getPartInputDataLength() :
                                   0);
      unsigned char *pivBuf = new unsigned char[partInputDataLength];
      const int pivMaxDisplayChars = 20;
      char hexPiv[2*pivMaxDisplayChars+1];
      ExFragDir::ExFragEntryType fragType = fragDir_->getType(i);

      printf("Fragment id  : %d\n", fragEntry->id_);
      printf("Fragment type: ");
      switch(fragType)
        {
        case ExFragDir::MASTER:
          printf("MASTER\n");
          break;
        case ExFragDir::EXPLAIN:
          printf("EXPLAIN\n");
          break;
        case ExFragDir::ESP:
          {
            printf("ESP\n");
          
            printf("Num ESPs     : %d\n", fragEntry->numEsps_);
            if (fragEntry->numEsps_ > 0)
              {
                printf("\n");
                printf("   ESP# Proc id  #c #w PIVs (in hex)\n");
                printf("   ---- -------- -- -- -------------------------------------------\n");
              }
            for (CollIndex e=0; e<fragEntry->numEsps_; e++)
              {
                ExRtFragInstance *fragInst = NULL;

                if (fragEntry->assignedEsps_.used(e))
                  fragInst = fragEntry->assignedEsps_[e];

                // ESP #
                printf("   %4d ", e);
                if (fragInst)
                  {
                    // Node (desired or actually assigned
                    if (fragInst->usedEsp_ != NULL)
                      {
                        // actual PID
                        const GuaProcessHandle &phandle =
                          fragEntry->assignedEsps_[e]->usedEsp_->
                          getIpcServer()->getServerId().getPhandle();
                        char pidBuf[9]; // includes trailing NUL
                        phandle.toAscii(pidBuf, sizeof(pidBuf));
                        pidBuf[8] = '\0';
                        printf("%8s ", pidBuf);
                      }
                    else if (fragEntry->assignedEsps_.used(e))
                      printf("CPU %4d ", fragEntry->assignedEsps_[e]->cpuNum_);

                    printf("%2d %2d ",
                           fragInst->numControlMessages_,
                           (fragInst->workMessageSent_ ? 1 : 0));
                  }
                else
                  printf("???            ");

                // print partition input data
                if (partInputDataLength > 0 &&
                    !fragEntry->dynamicLoadBalancing_ &&
                    e < fragEntry->partDesc_->getNumPartitions())
                  {
                    fragEntry->partDesc_->copyPartInputValue(
                         e, e, (char *) pivBuf, partInputDataLength);
                    int displayBytes = partInputDataLength;
                    NABoolean tooLong =
                      (displayBytes > pivMaxDisplayChars/2);

                    if (tooLong)
                      displayBytes = pivMaxDisplayChars/2;
                    for (int b=0; b<displayBytes; b++)
                      sprintf(&hexPiv[2*b],"%02x",pivBuf[b]);
                    hexPiv[2*displayBytes] = 0;
                    printf("%s%s\n", hexPiv, (tooLong ? "..." : ""));
                  }
              }
            delete pivBuf;
          }
          break;

        default:
          printf("Invalid fragment type: %d\n", (int) fragType);
        } // switch
      printf("\n");
    } // for
}

#ifdef IPC_INTEGRITY_CHECKING

void ExRtFragTable::checkIntegrity()
  {
  IpcEnvironment * ie = glob_->getIpcEnvironment(); 
  ie->checkIntegrity();
  }

void ExRtFragTable::checkLocalIntegrity()
  {
  // check the integrity of all outstanding service requests
  for (CollIndex i = 0; i < outstandingServiceRequests_.entries(); i++)
    {
    outstandingServiceRequests_[i]->checkLocalIntegrity();
    }
  }

#endif

// -----------------------------------------------------------------------
// Methods for class ExRtFragTableEntry
// -----------------------------------------------------------------------

ExRtFragTableEntry::ExRtFragTableEntry(CollHeap *heap) :
     assignedEsps_(heap),
     assignedPartInputValues_(heap)
{}

// -----------------------------------------------------------------------
// Free up any memory allocated.
// -----------------------------------------------------------------------
void ExRtFragTableEntry::release()
{
  for (Int32 i = 0; i < (Int32)assignedEsps_.getSize(); i++)
    {
      if (assignedEsps_.used(i) AND
          assignedEsps_[i] != NULL)
         assignedEsps_[i]->deleteMe();
    }
}

// -----------------------------------------------------------------------
// Methods for class ExRtFragInstance
// -----------------------------------------------------------------------

ExRtFragInstance::ExRtFragInstance(CollHeap *heap)
{
  state_          = ExRtFragTable::UNASSIGNED;
  usedEsp_        = NULL;
  fragmentHandle_ = NullFragInstanceHandle;
  whereIComeFrom_ = heap;
}

ExRtFragInstance::~ExRtFragInstance()
{
  release();
}

void ExRtFragInstance::release()
{
  // nothing to do
}

void * ExRtFragInstance::operator new(size_t)
{
  ex_assert(0,"Must use placement new");
  return (void *) NULL;
}

void * ExRtFragInstance::operator new(size_t size, CollHeap *heap)
{
  return heap->allocateMemory(size);
}

void ExRtFragInstance::operator delete(void *)
{
  ex_assert(0,"Should never call ExRtFragInstance::operator delete()");
}

void ExRtFragInstance::deleteMe()
{
  CollHeap *heap = whereIComeFrom_;
  release();
  heap->deallocateMemory(this);
}

// -----------------------------------------------------------------------
// Methods for class ExMasterEspMessage
// -----------------------------------------------------------------------

ExMasterEspMessage::ExMasterEspMessage(
     IpcEnvironment *env,
     ExRtFragTable  *rtFragTable) : 
        IpcMessageStream(
	  env,
	  IPC_MSG_SQLESP_CONTROL_REQUEST,
	  CurrEspRequestMessageVersion,
	  3000, // see note below
	  TRUE)
{
  rtFragTable_         = rtFragTable;
  downloadRequest_     = FALSE;
  fixupRequest_        = FALSE;
  assignPartRequest_   = FALSE;
  workTransRequest_    = FALSE;
  releaseTransRequest_ = FALSE;
  releaseRequest_      = FALSE;
  transactionalRequest_= FALSE;
  // A note: the fixed message buffer size given to the base class must
  // be large enough to avoid the "chunky" protocol for status replies
  // from the ESP to the master. Otherwise there are cases where
  // two fragments are downloaded through one control connection and
  // two "chunky" replies come back at the same time through the same
  // GuaConnctionToServer, which causes an assertion violation.
  // The status usually has about 200 bytes plus the space needed for
  // a diags area, so that violations of this should be restricted
  // to very rare error cases.
}

ExMasterEspMessage::~ExMasterEspMessage()
{
  // there is nothing left to do. When an ExMasterEspMessage completes,
  // the callback puts it on a list of completed messages (this list
  // resides in the IPCEnvironment. On a regular basis this list
  // is scanned and all entries are deleted. When this happens, the
  // statement which creates the ExMasterEspMessage might be deleted
  // already.
}


void ExMasterEspMessage::actOnSend(IpcConnection * connection)
{
  if (connection->getErrorInfo() == 0)
    incReqMsg(connection->getLastSentMsg()->getMessageLength());
}

void ExMasterEspMessage::actOnSendAllComplete()
{
  clearAllObjects();
  receive(FALSE);
}

void ExMasterEspMessage::actOnReceive(IpcConnection *connection)
{
  // do nothing if rtFragTable_ no longer exists. If rtFragTable_
  // is already gone, the statement issuing the message is also gone.
  if (!rtFragTable_)
    return;

  decrFragTableCounters();

  if ((connection->getErrorInfo() != 0) OR
      NOT (moreObjects() AND getNextObjType() == ESP_RETURN_STATUS_HDR))
    {
      // error receiving, set diagnostics area.
      // How to avoid redundant Conditions???
        
      ComDiagsArea *recvdDiagsArea = ComDiagsArea::allocate(
	   rtFragTable_->getGlobals()->getDefaultHeap());
	  
      connection->populateDiagsArea(
	   recvdDiagsArea, 
	   rtFragTable_->getGlobals()->getDefaultHeap());

      // merge the returned diagnostics area with the main one
      if (rtFragTable_->getGlobals()->getDiagsArea())
        rtFragTable_->getGlobals()->getDiagsArea()->mergeAfter(*recvdDiagsArea);
      else
        rtFragTable_->getGlobals()->setGlobDiagsArea(recvdDiagsArea);
      recvdDiagsArea->decrRefCount();

      // error code is in connection->getErrorInfo()

      // if we lost connection to the ESP, find the associated entries
      // and alter their state (if the frag table is still there)
      actOnErrorConnection(connection);

      rtFragTable_->state_ = ExRtFragTable::ERROR;

      // activate the scheduler even if we haven't received all replies
      // back, to avoid deadlocks when getting an error in a broadcast msg.
      rtFragTable_->schedulerEvent_->scheduleAndNoteCompletion();
    }

  // process the message, note that the fragment table may already have
  // been deallocated by now
  while (moreObjects())
    {
      if (connection->getErrorInfo() == 0)
        {
          if (getNextObjType() != ESP_RETURN_STATUS_HDR)
            connection->dumpAndStopOtherEnd(true, false);
          ex_assert(getNextObjType() == ESP_RETURN_STATUS_HDR,
                    "Expected return status header");
        }
      else if (getNextObjType() != ESP_RETURN_STATUS_HDR)
        // if error occurred during ipc receive, the message buffer may point
        // to the send buffer instead of the receive buffer. in that case
        // let's skip checking reply status.
        break;

      ExEspReturnStatusReplyHeader replyStatus(
	   rtFragTable_->getGlobals()->getDefaultHeap());

      *this >> replyStatus;

      // for a release request we update the state before we even send
      // it to the ESP, in all other cases we need to do that now
      if (NOT releaseRequest_)
	{
	  // find out who it is that replies
	  ExRtFragInstance *fragInstance =
	    rtFragTable_->findInstance(replyStatus.key_.getFragId(),
				       connection);
	  
          // It is possible that the reply to the frelease request
          // was processed before the reply to some other outstanding
          // request. In this case, fragInstance will be NULL. We just
          // ignore this header in this case.
          if (fragInstance)
            {

	      // remember the assigned fragment handle
	      fragInstance->fragmentHandle_ = replyStatus.handle_;

	      // do the bookkeeping / bean counting
	      fragInstance->numControlMessages_--;

	      // if we marked in the fragment instance that we
              // had a work or release message sent, then
              // reset that marker now that we got a reply
              
              if (releaseTransRequest_)
                fragInstance->releasingWorkMsg_ = FALSE;
              
              if (workTransRequest_)
                {
                  fragInstance->workMessageSent_ = FALSE;
                  fragInstance->workMsg_ = NULL;
                }

	      switch (replyStatus.instanceState_)
                {
	        case ExEspReturnStatusReplyHeader::INSTANCE_DOWNLOADED:

	          fragInstance->state_ = ExRtFragTable::DOWNLOADED;
	          break;

	        case ExEspReturnStatusReplyHeader::INSTANCE_READY:

		  // the state of the entire fragment table changes to
		  // READY once all load/fixup messages have been
		  // successfully completed
		  if (fixupRequest_ AND
		      rtFragTable_->numLoadFixupMsgesOut_ == 0 AND
		      rtFragTable_->state_ == ExRtFragTable::ASSIGNED AND
		      (rtFragTable_->getGlobals()->getDiagsArea() == NULL OR
		       rtFragTable_->getGlobals()->getDiagsArea()->
		                                      mainSQLCODE() >= 0))
		    rtFragTable_->state_ = ExRtFragTable::READY;

	          // if this wasn't a static assignment of partition input
	          // values, indicate that the entry has no current pivs
	          if (rtFragTable_->fragmentEntries_[
		   replyStatus.key_.getFragId()]->dynamicLoadBalancing_)
		    fragInstance->partInputDataAssigned_ = FALSE;
	          // fall through to next case

	        case ExEspReturnStatusReplyHeader::INSTANCE_ACTIVE:

	          fragInstance->state_ = ExRtFragTable::FIXED_UP;
	          break;

	        case ExEspReturnStatusReplyHeader::INSTANCE_RELEASED:

	          fragInstance->state_ = ExRtFragTable::UNASSIGNED;
	          break;

	        case ExEspReturnStatusReplyHeader::INSTANCE_BROKEN:
                  // ESP has declared itself broken due to runtime error.
	          // The ESP has (or should have) sent a diagnostics area,
                  // which we'll fetch below.
		  //
		  // the error esp replied to the work msg, changed its state
		  // to BROKEN and was waiting for the release esp msg (see
		  // ex_split_bottom_tcb::reportErrorToMaster()).
		  // meanwhile, the master received the error reply and set
		  // the fatal error flag in the root_tcb, which resulted in
		  // the master aborting outstanding work msgs to all other
		  // esps. master would end up killing those esps.
		  // however, the esp that replied error (in work msg) still
		  // had outstanding requests on its connections (GCTC) to
		  // the parent esps, and it can be difficult to cleanup those
		  // connections because they were on the server side. thus
		  // the safe thing to do is to also kill the error esp.
		  //

		  // fall through

	        default:
                  // error, invalid value coming back. since master does not
		  // understand the error, we may not be sure about the esp
		  // state and thus it's safer to just kill the esp.
		  //
		  // set connection to error state will kill the esp.
		  IpcConnection *conn = fragInstance->usedEsp_
		    ->getIpcServer()->getControlConnection();
		  conn->setState(IpcConnection::ERROR_STATE);		  

	          fragInstance->state_ = ExRtFragTable::LOST_CONNECTION;
                  rtFragTable_->state_ = ExRtFragTable::ERROR;
	          break;
                }
            }
	}

      if (moreObjects() AND getNextObjType() == ESP_DIAGNOSTICS_AREA)
	{
	  // allocate a diags area on the current heap and fill it with
	  // the contents of the diags area in the message
	  ComDiagsArea *recvdDiagsArea = ComDiagsArea::allocate(
	       rtFragTable_->getGlobals()->getDefaultHeap());
	  
	  *this >> *recvdDiagsArea;

	  // merge the returned diagnostics area with the main one
	  if (rtFragTable_->getGlobals()->getDiagsArea())
	    {
	      // Don't merge dup fixup errors
	      if (rtFragTable_->getGlobals()->getStatement()->getExecState() == Statement::FIXUP_)
		{
	      ComDiagsArea *existingDiags = 
		rtFragTable_->getGlobals()->getDiagsArea();
	      CollIndex recvdSqlCode = recvdDiagsArea->mainSQLCODE();
	      if ( !existingDiags->containsError(recvdSqlCode))
		  rtFragTable_->getGlobals()->getDiagsArea()->
		  mergeAfter(*recvdDiagsArea);
		}
	      else
		 rtFragTable_->getGlobals()->getDiagsArea()->
		  mergeAfter(*recvdDiagsArea);
	    }
	  else
            rtFragTable_->getGlobals()->setGlobDiagsArea(recvdDiagsArea);
          recvdDiagsArea->decrRefCount();
	}
    }
}

void ExMasterEspMessage::actOnReceiveAllComplete()
{
  if (rtFragTable_)
    {
      // remove the message from the list of outstanding requests
      rtFragTable_->removeCompletedRequest(this);
      // make the scheduler visit us again to redrive
      // dynamic load balancing and to send more transaction
      // requests down
      rtFragTable_->schedulerEvent_->scheduleAndNoteCompletion();
    }

  // and add it to the global list of completed requests.
  addToCompletedList();
}

// safe cast of IpcMessageStream hierarchy to ExMasterEspMessage
ExMasterEspMessage * ExMasterEspMessage::castToExMasterEspMessage(void)
{
  return this;
}

// Perform book keepings on rtFragTable_.
void ExMasterEspMessage::decrFragTableCounters()
{
  // decrement number of outstanding messages (independent of error status)
  if (downloadRequest_ || fixupRequest_)
    rtFragTable_->numLoadFixupMsgesOut_--;
  if (workTransRequest_ || releaseTransRequest_)
    rtFragTable_->numWorkMsgesOut_--;
  if (transactionalRequest_)
    rtFragTable_->numTransactionalMsgesOut_--;
  if (releaseRequest_)
    rtFragTable_->numReleaseEspMsgesOut_--;
}

// Clean up frag instance states upon connection error.
void ExMasterEspMessage::actOnErrorConnection(IpcConnection *connection)
{
  // find all frag instances associated with the error connection and
  // alter their states
  for (ExFragId fragId = 0;
       fragId < rtFragTable_->fragmentEntries_.entries();
       fragId++)
    {
      // find the instance of this fragment that uses the connection
      ExRtFragInstance *fragInstance = rtFragTable_->
                                         findInstance(fragId,connection);
      if (fragInstance)
        {
          // found an instance entry that is using the connection,
          // indicate that we lost it.
          fragInstance->state_ = ExRtFragTable::LOST_CONNECTION;

          if (releaseTransRequest_)
            {
              // if we get error when sending release transaction request
              // we would have shared the same connection for the work request
              // sent earlier. In this case, the server ESP would not reply
              // to that work request. Thus, we let the outstanding work
              // request stream to know the error and abort pending I/O
              if (fragInstance->getWorkMsg())
                fragInstance->getWorkMsg()->abandonPendingIOs();
            }
          break;  // since one ESP executes only one fragment instance
        }
    }

  // Propagate control connection errors to all seamonster connections
  // for this query
  if (connection)
  {
    GuaConnectionToServer *c = connection->castToGuaConnectionToServer();
    if (c)
    {
      const LIST(SMConnection *) &smConns =
        rtFragTable_->getGlobals()->allSMConnections();
      
      for (CollIndex i=0; i < smConns.entries(); i++)
      {
        SMConnection *smConn = smConns[i];
        if (smConn)
          smConn->reportControlConnectionError(c->getGuardianError());
      }
    }
  }
}

void ExMasterEspMessage::incReqMsg(Int64 msgBytes)
{
  ExStatisticsArea *statsArea;
  
  if (rtFragTable_)
  {
    if ((statsArea = rtFragTable_->getGlobals()->getStatsArea()) != NULL)
      statsArea->incReqMsg(msgBytes);
  }
}

// -----------------------------------------------------------------------
// Methods for class ExEspManager
// -----------------------------------------------------------------------

ExEspManager::ExEspManager(IpcEnvironment *env, CliGlobals *cliGlobals)
  : env_(env), cliGlobals_(cliGlobals)
{
  espServerClass_ = new(env->getHeap()) IpcServerClass(
       env_,
       IPC_SQLESP_SERVER);

  // hash dictionary: default number of buckets is 256
  espCache_ = new(env->getHeap()) NAHashDictionary<ExEspCacheKey, NAList<ExEspDbEntry *> >(&hashFunc_EspCacheKey, 256, FALSE, env->getHeap());

  // esp tracing, other members are initialized in getEspFromCache()
  lastEspTraceIndex_ = MAX_NUM_ESP_STATE_TRACE_ENTRIES + 1;
  espTraceArea_ = NULL;

  lastAssignedCpu_ = 0;
  maxCpuNum_ = 0;

  /* Added with multi fragment esp support */
  Int32 nodeCount = 0;
  Int32 nodeMax = 0;
  MS_Mon_Node_Info_Entry_Type *nodeInfo = NULL;

  // Get the number of nodes to know how much info space to allocate
  Int32 lv_ret = msg_mon_get_node_info(&nodeCount, 0, NULL);
  if ((lv_ret == 0) && (nodeCount > 0))
  {
     // Allocate the space for node info entries
     nodeInfo = new(env->getHeap()) MS_Mon_Node_Info_Entry_Type[nodeCount];

     if (nodeInfo)
     {
        // Get the node info
        memset(nodeInfo, 0, sizeof(nodeInfo));
        nodeMax = nodeCount;
        lv_ret = msg_mon_get_node_info(&nodeCount, nodeMax, nodeInfo);
        if (lv_ret == 0)
        {
          // Find # of storage nodes by checking the storage bit.
          // The computed value is used as node Id where ESPs will be created.
          // ESPs should be running on the storage nodes only.
 
          // From Seabed ES:
          // MS_Mon_ZoneType_Any = ( MS_Mon_ZoneType_Edge |
          //                         MS_Mon_ZoneType_Aggregation |
          //                         MS_Mon_ZoneType_Storage ),
          // MS_Mon_ZoneType_Frontend = ( MS_Mon_ZoneType_Edge |
          //                              MS_Mon_ZoneType_Aggregation ),
          // MS_Mon_ZoneType_Backend = ( MS_Mon_ZoneType_Aggregation |
          //                             MS_Mon_ZoneType_Storage )

   
          for ( Int32 i=0; i<nodeCount; i++ ) {
            if ( ( nodeInfo[i].type & MS_Mon_ZoneType_Storage ) &&
                 ( !nodeInfo[i].spare_node) )
              maxCpuNum_++;
          }
        }
        NADELETEARRAY(nodeInfo,nodeCount,MS_Mon_Node_Info_Entry_Type,env->getHeap());
     }
  }



  Int32 lv_nid =0;
  Int32 lv_pid = 0;
  char lv_name[MS_MON_MAX_PROCESS_NAME+1];
  Int32 lv_name_len = MS_MON_MAX_PROCESS_NAME;
  Int32 lv_ptype = 0;
  Int32 lv_zid = 0;
  Int32 lv_os_pid = 0;
  ThreadId lv_os_tid = 0;
  memset(lv_name, 0, MS_MON_MAX_PROCESS_NAME+1);
  lv_ret = msg_mon_get_my_info(&lv_nid,
			       &lv_pid,
			       lv_name,
			       lv_name_len,
			       &lv_ptype,
			       &lv_zid,
			       &lv_os_pid,
			       &lv_os_tid);
  
  if (lv_ret == 0) {

#ifndef _DEBUG
    lastAssignedCpu_ = (Int32)lv_os_tid % maxCpuNum_;
#else
     lastAssignedCpu_ = lv_nid;
#endif

  }
  /* Added with multi fragment esp support end */

}

ExEspManager::~ExEspManager()
{
  if (espServerClass_)
    NADELETE(espServerClass_, IpcServerClass, env_->getHeap());
  
  if (espCache_)
    NADELETE(espCache_, NAHashDictionary, env_->getHeap());

  if (traceRef_)
    {
      ExeTraceInfo *ti = cliGlobals_->getExeTraceInfo();
      if (ti)  // unnecessary check but just in case.
        ti->removeTrace(traceRef_);
    }

  if (espTraceArea_)
    {
      NADELETEARRAY(espTraceArea_, maxEspTraceIndex_, EspDbEntryTrace,
                    env_->getHeap());
      espTraceArea_ = NULL;
    }
}

Int32 ExEspManager::getNumOfEsps()
{
  Int32 numEsps = 0;
  ExEspCacheKey *key = NULL;
  NAList<ExEspDbEntry *> *espList = NULL;
  NAHashDictionaryIterator<ExEspCacheKey, NAList<ExEspDbEntry *> > iter(*espCache_);
  for (CollIndex i = 0; i < iter.entries(); i++)
    {
      iter.getNext(key, espList);
      numEsps += espList->entries();
    }

  return numEsps;
}

ExEspDbEntry *ExEspManager::shareEsp(
     ComDiagsArea **diags,
     LIST(ExEspDbEntry *) &alreadyAssignedEsps, // multi fragment esp
     CollHeap *statementHeap,
     Statement *statement,
     const char * clusterName,
     NABoolean &startedANewEsp,
     IpcCpuNum cpuNum,
     short memoryQuota,
     Int32 user_id,
     NABoolean verifyESP,
     NABoolean * verifyCPUptr, // both input and output
     IpcPriority priority,
     Lng32 espLevel,
     Lng32 idleTimeout,
     Lng32 assignTimeWindow,
     IpcGuardianServer **creatingEsp,
     NABoolean soloFragment,
     Int16 esp_multi_fragment,
     Int16 esp_num_fragments)
{
  Int32 nowaitDepth;
  IpcServer *server;
  ExEspDbEntry *result = NULL;
  char *ptrToClusterName = (char *)clusterName;
  NAList<ExEspDbEntry *> *espList = NULL;
  ExProcessStats *processStats = GetCliGlobals()->getExProcessStats(); 
  ExMasterStats *masterStats = NULL;
  StmtStats *ss = statement->getStmtStats();
  if (ss != NULL)
     masterStats = ss->getMasterStats();
  if (espList == NULL)
  if (*creatingEsp == NULL) // Nowaited Creation of an ESP is not in progress
  {
    nowaitDepth = env_->getCCMaxWaitDepthLow();
    if ( cpuNum == IPC_CPU_DONT_CARE )
      cpuNum = getRoundRobinCPU();

    // look up the cache for esp to share
    NABoolean espServerError = FALSE;
    result = getEspFromCache(alreadyAssignedEsps, statementHeap, statement, clusterName, cpuNum, memoryQuota, user_id, verifyESP, espLevel, idleTimeout, assignTimeWindow, nowaitDepth, espServerError, soloFragment, esp_multi_fragment, esp_num_fragments);
    if (espServerError == TRUE)
      // found error from ESP already assigned to prev segment
      {
	if (diags) {
	  IpcAllocateDiagsArea(*diags, env_->getHeap());
	  char errMsg[100];
	  str_sprintf(errMsg, " ESP on %s, CPU %d has connection error.",
		clusterName, cpuNum);
	  (**diags) << DgSqlCode(-8586) << DgString0(errMsg);
	}
	return NULL;
      }

    if (result)
      return result;

    //
    // didn't find an ESP in cache to share, start a new one.
    //


   // if the given remote segment (from clusterName) is not available, we will
   // use local segment instead.
   if (ptrToClusterName == NULL)
    {
      // remote segment not available. look up cache for esp on local segment.
      ptrToClusterName = cliGlobals_->myNodeName();
      NABoolean espServerError = FALSE;
      result = getEspFromCache(alreadyAssignedEsps, statementHeap, statement, ptrToClusterName, cpuNum, memoryQuota, user_id, verifyESP, espLevel, idleTimeout, assignTimeWindow, nowaitDepth, espServerError, soloFragment, esp_multi_fragment, esp_num_fragments);
      if (espServerError == TRUE)
        // found error from ESP already assigned to prev segment
        {
	  if (diags) {
	    IpcAllocateDiagsArea(*diags, env_->getHeap());
	    char errMsg[100];
	    str_sprintf(errMsg, " ESP on %s, CPU %d has connection error.",
		  clusterName, cpuNum);
	    (**diags) << DgSqlCode(-8586) << DgString0(errMsg);
	  }
	  return NULL;
        }
      if (result)
        // found esp on local segment to share
        return result;
    }

    NABoolean waitedStartup;
    if (espServerClass_->nowaitedEspServer_.waitedStartupArg_ == '1')
      waitedStartup = TRUE;
    else
      waitedStartup = FALSE;
    server = espServerClass_->allocateServerProcess(
	 diags,
	 env_->getHeap(),
	 ptrToClusterName,
	 cpuNum,
	 priority,
	 espLevel,
	 TRUE, //usesTransactions
	 waitedStartup, //waited process creation
	 nowaitDepth,
	 NULL, // progFileName
	 NULL, //processName
	 espServerClass_->parallelOpens());

    if ( server == NULL )
      {
        if (processStats != NULL)
        {
           if (! waitedStartup)
              processStats->incStartedEsps();
           processStats->incBadEsps();
        }
	return NULL;   
      }


    if (((IpcGuardianServer *)server)->isCreatingProcess())
    {
      *creatingEsp = (IpcGuardianServer *)server;
      if (processStats != NULL)
         processStats->incStartedEsps();
      return NULL;
    }
  } // Nowaited creation of an ESP is not in progress
  else
  {
    // Nowaited creation of an ESP is in progress
    cpuNum = (*creatingEsp)->getCpuNum();
    ExEspCacheKey tempKey(clusterName, cpuNum, user_id);
    espList = espCache_->getFirstValue(&tempKey);
    nowaitDepth = (*creatingEsp)->getNowaitDepth();
    server = espServerClass_->allocateServerProcess(
	 diags,
	 env_->getHeap(),
	 ptrToClusterName,
	 cpuNum,
	 priority,
	 espLevel,
	 TRUE,
	 FALSE, //nowaited process creation (Must be FALSE on nowaited completion)
	 nowaitDepth,
	 NULL, // progFileName
	 NULL, //processName
	 espServerClass_->parallelOpens(),
	 creatingEsp);
    if ((*creatingEsp) && (*creatingEsp)->isCreatingProcess())
      return NULL; // Launch has not yet completed
    if ( server == NULL )
      {
        if (processStats != NULL)
           processStats->incBadEsps();
	return NULL;   
      }
  }
    
  result = new(env_->getHeap()) ExEspDbEntry(
      env_->getHeap(),
      server,
      ptrToClusterName,
      cpuNum,
      espLevel,
      user_id);
  
  result->inUse_ = true;
  result->usageCount_++;
  result->totalMemoryQuota_ += memoryQuota + 100;
  result->statement_ = statement;

  if (espList == NULL)
    espList = espCache_->getFirstValue(result->getKey());

  ex_assert(espList, "ESP list not found in cache for the given key");

  // insert the new esp entry in cache
  espList->insert(result);

  startedANewEsp = TRUE;

  if (processStats != NULL)
  {
     processStats->incStartupCompletedEsps();
     processStats->incNumESPsInUse(FALSE);
  }
  if (masterStats != NULL)
      masterStats->incNumEspsInUse();
  if (espTraceArea_ != NULL)  // ESP state tracing
    { // any esp picked up here must be created new
      addToTrace(result, CREATED_USE);
    }

  return result;
}

// look for an existing ESP in cache to share

const char *EspEntryTraceDesc = "ESP entry state trace in ESP manager.\n Can use env ESP_NUM_TRACE_ENTRIES to set more or less entries";

ExEspDbEntry *ExEspManager::getEspFromCache(LIST(ExEspDbEntry *) &alreadyAssignedEsps, // multi fragment esp
					    CollHeap *statementHeap,
					    Statement *statement,
                                            const char *clusterName,
                                            IpcCpuNum cpuNum,
					    short memoryQuota,
                                            Int32 user_id,
                                            NABoolean verifyESP,
					    Lng32 espLevel,
                                            Lng32 idleTimeout,
                                            Lng32 assignTimeWindow,
					    Int32 nowaitDepth,
                                            NABoolean &espServerError,
                                            NABoolean soloFragment,
                                            Int16 esp_multi_fragment,
                                            Int16 esp_num_fragments)
{
  ExEspDbEntry *result = NULL;
  LIST(ExEspDbEntry *) badEsps(statementHeap);
  
  ExEspCacheKey tempKey(clusterName, cpuNum, user_id);
  NAList<ExEspDbEntry *> *espList = espCache_->getFirstValue(&tempKey);
  ExProcessStats *processStats = GetCliGlobals()->getExProcessStats();
  ExMasterStats *masterStats = NULL;
  StmtStats *ss  = statement->getStmtStats();
  if (ss != NULL)
     masterStats = ss->getMasterStats();
  if (espList == NULL)
    {
      // no esp pool found in esp cache for the given segment-cpu-user.
      // create a new esp pool in cache.
      ExEspCacheKey *key = new(env_->getHeap()) ExEspCacheKey(clusterName, cpuNum, user_id, env_->getHeap());
      espList = new(env_->getHeap()) NAList<ExEspDbEntry *>(env_->getHeap());
      espCache_->insert(key, espList);

      // ESP state tracing >>
      // These constants are defined in the header file
      if (lastEspTraceIndex_ > MAX_NUM_ESP_STATE_TRACE_ENTRIES)
        {
          Int32 numTraceEntries = NUM_ESP_STATE_TRACE_ENTRIES;
          const char *envvar = getenv("ESP_NUM_TRACE_ENTRIES");
          if (envvar != NULL)
            {
              Int64 nums = str_atoi(envvar, str_len(envvar));
              if (nums >= 0 && nums < MAX_NUM_ESP_STATE_TRACE_ENTRIES)
                numTraceEntries = (Int32) nums;  //ignore any other value or invalid
            }

          if (numTraceEntries == 0)
            { // no tracing and don't check again (see few lines above)
              espTraceArea_ = NULL;
              lastEspTraceIndex_ = MAX_NUM_ESP_STATE_TRACE_ENTRIES;
            }
          else
            { // allocate and initialize the buffer
              espTraceArea_ = new(env_->getHeap()) EspDbEntryTrace[numTraceEntries];
              memset(espTraceArea_, 0, sizeof(EspDbEntryTrace) * numTraceEntries);
              lastEspTraceIndex_ = numTraceEntries;  // so 1st index will be 0
              // register trace to global trace info repository
              ExeTraceInfo *ti = cliGlobals_->getExeTraceInfo();
              if (ti)
                {
                  Int32 lineWidth = 75;  // print width per trace entry
                  void *regdTrace;
                  Int32 ret = ti->addTrace("EspManager", this, lastEspTraceIndex_,
                                       2, this, getALine, &lastEspTraceIndex_,
                                       lineWidth, EspEntryTraceDesc,
                                       &regdTrace);
                  if (ret == 0)
                    {
                      // trace info added successfully, now add entry fields
                      ti->addTraceField(regdTrace, "ESP Proc Name (nid,pid use count)     ", 0, ExeTrace::TR_POINTER32);
                      ti->addTraceField(regdTrace, "State", 1, ExeTrace::TR_INT32);
                      traceRef_ = (ExeTrace*) regdTrace;
                    }
                }
            }
          maxEspTraceIndex_ = numTraceEntries;
        }
  
      // ESP state tracing end <<
    }

  CollIndex i = 0;
  for (i = FIRST_COLL_INDEX; i < espList->getUsedLength(); i++)
    {
      if (espList->getUsage(i) == UNUSED_COLL_ENTRY)
        continue;

      ExEspDbEntry *e = espList->usedEntry(i);

      if ((e->inUse_) && (e->soloFragment_ || soloFragment || !(esp_multi_fragment) || e->statement_ != statement))
	continue;

      // don't reuse a broken ESP
      IpcServer *ipcs = e->getIpcServer();
      if (ipcs AND
          ipcs->getControlConnection() AND
          ipcs->getControlConnection()->getState() == IpcConnection::ERROR_STATE)
        {
	  if (e->usageCount_ < 1)
	    {
	      badEsps.insert(e);
	      continue;
	    }
	  // multi-fragment
	  else
	    {
	      // it's selected for earlier fragment, would have to restart
	      // the query. Return error now
	      espServerError = TRUE;
	      result = e;
	      break;
	    }
        }

      // multi-fragment
      if (alreadyAssignedEsps.contains(e))
	  // ESP is already assigned to this fragment, don't
	  // assign it a second time
	continue;
      
      if ( verifyESP )
        {
          IpcServer *ipcs = e->getIpcServer();
          if (ipcs AND ipcs->castToIpcGuardianServer()
              AND ipcs->castToIpcGuardianServer()->serverDied() ) {
            // this esp has died
            IpcConnection * controlConn = ipcs->getControlConnection();
            if ( controlConn )
              controlConn->setState(IpcConnection::ERROR_STATE);
            badEsps.insert(e);
            if (espTraceArea_ != NULL)  // ESP state tracing
              { // should be deleted due to error
                addToTrace(e, DELETED);
              }
            continue;
          }
        }

      if (idleTimeout > 0 && e->idleTimestamp_ > 0 && (e->usageCount_ == 0)) // multi-fragment
        {
          Int64 currentTimestamp = NA_JulianTimestamp();
          Int64 timeDiff = currentTimestamp - e->idleTimestamp_ - (Int64)idleTimeout * 1000000;
          if (timeDiff >= 0)
            {
              // this esp has been idle for the specified ESP_IDLE_TIMEOUT
              // limit or longer. let's release it.
              IpcConnection * controlConn = e->getIpcServer()->getControlConnection();
              if (controlConn)
                controlConn->setState(IpcConnection::ERROR_STATE);
              badEsps.insert(e);
              if (espTraceArea_ != NULL)  // ESP state tracing
                {
                  addToTrace(e, IDLE_TIMEDOUT);
                }
              // skip this esp
              continue;
            }
          else if (timeDiff > -(assignTimeWindow * 1000000))
            {
              // skip this esp because it will reach to its idle timeout limit
              // in less than the assignment time window, because it is
              // possible that the ESP may have died already or could
              // die before receiving the first fixup message
              continue;
            }
        }

      // Don't reassign an ESP if the ESP level doesn't match
      if (env_->getEspAssignByLevel() == '1' && e->espLevel_ != espLevel)
        continue;

      // we have found a free esp for reuse
      if ((2 * e->usageCount_ + 1 <= nowaitDepth) &&
          (e->usageCount_ < esp_num_fragments))
      {
        e->usageCount_++; // multi fragment esp
        e->statement_ = statement;
        e->totalMemoryQuota_ += 100 + memoryQuota;
        // If the ESP is already assigned to query
        // don't increment InUse counter again
        if (processStats &&  ! e->inUse_) {
           processStats->incNumESPsInUse(TRUE);
           if (masterStats != NULL)
               masterStats->incNumEspsInUse();
        }
        e->inUse_ = true;
        e->soloFragment_ = soloFragment;
        result = e;
        if (e->usageCount_ == 0 && espTraceArea_ != NULL)  // ESP state tracing
          { // any esp picked up here must be idling
            addToTrace(result, IDLE_REUSE);
          }
        break;
      }
      else
	continue;
      e->inUse_ = true;
      result = e;
      if (espTraceArea_ != NULL)  // ESP state tracing (non-Linux)
        { // any esp picked up here must be idling
          addToTrace(result, IDLE_REUSE);
        }
      break;
    } // for i
  NABoolean prevState;
  // release dead esps
  for (i = 0; i < badEsps.entries(); i++)
    {
      prevState =  badEsps[i]->inUse_; 
      // the ESP should not be in use and the usage count should be 0
      // must set esp to be in use
      badEsps[i]->inUse_ = true;
      // set the usage count to 1 so that releaseEsp() decrements it to zero
      badEsps[i]->usageCount_ = 1;
      // set verifyEsp to FALSE since all esps being released are known bad
      // set the badEsp - 3rd parameter to TRUE to ensure
      // that the ESP is not treated as it was in use
      // This ESP was free actually
      releaseEsp(badEsps[i], FALSE, prevState);
    }
  return result;
}

IpcCpuNum ExEspManager::getRoundRobinCPU()
{
  IpcCpuNum logCPUNum;
  CliGlobals *cliGlobals = GetCliGlobals();

  // lastAssignedCpu_ contains a "logical" CPU number (contiguous
  // values 0 ... n-1). We increment this number by 1 (mod n) for
  // every call.
  logCPUNum        =
  lastAssignedCpu_ = (lastAssignedCpu_ + 1) % cliGlobals->myNumCpus();

  // now map this logical number to an actual node id, in case we have
  // "holes" in the node ids (nodes were removed dynamically)
  return cliGlobals->mapLogicalToPhysicalCPUNum(logCPUNum);
}

void ExEspManager::releaseEsp(ExEspDbEntry *esp, NABoolean verifyEsp,
                 NABoolean prevState)
{
  if ((--esp->usageCount_) > 0) {
    return;
  }
  esp->totalMemoryQuota_ = 0;
  
  ExProcessStats *processStats = GetCliGlobals()->getExProcessStats();

  ex_assert(esp->inUse_, "ESP is already released");

  IpcServer *ipcs = esp->getIpcServer();
  IpcConnection *controlConn = ipcs->getControlConnection();

  // delete esp if the esp connection is in error state
  bool deleteEsp = (controlConn->getState() == IpcConnection::ERROR_STATE);
  // make sure esp is still alive if verifyEsp flag is set
  if (!deleteEsp && verifyEsp &&
      ipcs->castToIpcGuardianServer() &&
      ipcs->castToIpcGuardianServer()->serverDied())
    {
      controlConn->setState(IpcConnection::ERROR_STATE);
      deleteEsp = true;
      if (espTraceArea_ != NULL)  // ESP state tracing
        { // esp died
          addToTrace(esp, DELETED);
        }
    }

  // Reintegrate new node by deleting ESPs that were started on
  // alternate nodes.  
  IpcGuardianServer *ipcgs = ipcs->castToIpcGuardianServer();
  if (ipcgs && ipcgs->getRequestedCpuDown())
    {
      if (ComRtGetCpuStatus(NULL, ipcgs->getCpuNum()))
        deleteEsp = true;
    }
  if (processStats != NULL)
  {
     processStats->decNumESPs(prevState, deleteEsp);
  }
  if (deleteEsp)
    {
      NAList<ExEspDbEntry *> *espList = espCache_->getFirstValue(esp->getKey());
      ex_assert(espList, "No matching ESP found in cache");
      CollIndex i = espList->index(esp);
      ex_assert((i != NULL_COLL_INDEX), "ESP cache is corrupted");
      if (ipcs)
        ipcs->logEspRelease(__FILE__, __LINE__);
      delete esp;
      espList->removeAt(i);
  
    }
  else
    {
      // esp is now free since we don't allow esp sharing
      esp->inUse_ = false;
      esp->statement_ = NULL;
      esp->soloFragment_ = false;

      if (espTraceArea_ != NULL)  // ESP state tracing
        {
          addToTrace(esp, USED_IDLING);
        }
    }
}

// change the priority of all ESPs currently held by the ESP manager
// note that we may find that some ESPs are idle timed out when altering
// priority calls are returned. If ignoreNotFound is true, no error 11
// is returned, but the connections to those ESPs will still be set to
// error state
short ExEspManager::changePriorities(IpcPriority priority, NABoolean isDelta,
                                     bool ignoreNotFound)
{
  short rc, retRC = 0;
  if (!espServerClass_)
    return retRC;

  ExEspCacheKey *key = NULL;
  NAList<ExEspDbEntry *> *espList = NULL;
  NAHashDictionaryIterator<ExEspCacheKey, NAList<ExEspDbEntry *> > iter(*espCache_);
  for (CollIndex i = 0; i < iter.entries(); i++)
    {
      iter.getNext(key, espList);
      for (CollIndex j = FIRST_COLL_INDEX; j < espList->getUsedLength(); j++)
        {
          if (espList->getUsage(j) == UNUSED_COLL_ENTRY)
            continue;

          ExEspDbEntry *e = espList->usedEntry(j);

          IpcServer *ipcs = e->getIpcServer();
          IpcConnection *conn = ipcs->getControlConnection();
          if (!conn || conn->getState() == IpcConnection::ERROR_STATE)
            continue;

          rc = ipcs->castToIpcGuardianServer()->changePriority(priority, isDelta);
          if (rc)
            {
              // if return code is 11 the esp could have been stopped
              // due to idle timeout. Do not return any error if told.
              if (rc == 11 && ignoreNotFound == true)
                ; // no op
              else if (!retRC)
                retRC = rc;

              // change priority failed. set connection to error state.
              conn->setState(IpcConnection::ERROR_STATE);
            }
        } // for j
    } // for i

  return retRC;
}

Lng32 ExEspManager::endSession(ContextCli *context)
{
  // iterate thru all esps in cache and delete/kill free esps
  Lng32 numEspsStopped = 0;
  ExEspCacheKey *key = NULL;
  NAList<ExEspDbEntry *> *espList = NULL;
  NAHashDictionaryIterator<ExEspCacheKey, NAList<ExEspDbEntry *> > iter(*espCache_);
  NAArray<ExEspDbEntry *> savedEsps(env_->getHeap());
  ExEspDbEntry *entry;
  ExProcessStats *processStats = GetCliGlobals()->getExProcessStats();
  for (CollIndex i = 0; i < iter.entries(); i++)
    {
      iter.getNext(key, espList);
      Int32 pos = 0;
      while (espList->getFirst(entry))  // first esp entry removed from list
        {
          if (entry->inUse_)
            // esp still being used
            savedEsps.insertAt(pos++, entry);
          else
            // delete/kill free esp
            {
              if (espTraceArea_ != NULL)  // ESP state tracing
                {
                  addToTrace(entry, IDLE_DELETE);
                }
              if (entry->getIpcServer())
                entry->getIpcServer()->logEspRelease(__FILE__, __LINE__);
              delete entry;
              if (processStats != NULL)
                 processStats->decNumESPs(FALSE, TRUE); // inUse, DeletedEsps
              numEspsStopped++;
            }
        }

      // these esps are still in use. put them back in esp cache.
      for (CollIndex j = 0; j < savedEsps.entries(); j++)
        espList->insert(savedEsps[j]);

      savedEsps.clear();
    }
  return numEspsStopped;
}

void ExEspManager::stopIdleEsps(ContextCli *context)
{
  Lng32 stopIdleEspsTimeout = context->getSessionDefaults()->getEspStopIdleTimeout();
  if (stopIdleEspsTimeout <= 0)
    // do not kill idle esps
    return;

  // iterate thru all esps in cache and delete/kill those that have been idle
  // for longer than ESP_STOP_IDLE_TIMEOUT (default is 5 minutes).
  ExEspCacheKey *key = NULL;
  NAList<ExEspDbEntry *> *espList = NULL;
  ExEspDbEntry *entry;
  NAHashDictionaryIterator<ExEspCacheKey, NAList<ExEspDbEntry *> > iter(*espCache_);
  ExProcessStats *processStats = GetCliGlobals()->getExProcessStats();

  for (CollIndex i = 0; i < iter.entries(); i++)
    {
      iter.getNext(key, espList);
      for (CollIndex j = 0; j < espList->entries(); j++)
        {
	  entry = espList->at(j);

          if (entry->inUse_ || entry->idleTimestamp_ <= 0)
            // esp still being used
            continue;

          Int64 currentTimestamp = NA_JulianTimestamp();
          if (currentTimestamp - entry->idleTimestamp_ >
              (Int64)stopIdleEspsTimeout * 1000000)
            {
              // this esp has been idle longer than ESP_STOP_IDLE_TIMEOUT
              NABoolean removed = espList->removeAt(j--);
              ex_assert(removed, "Remove idle esp entry not found in ESP cache");
              if (espTraceArea_ != NULL)  // ESP state tracing
                {
                  addToTrace(entry, IDLE_TIMEDOUT);
                }
            if (entry->getIpcServer())
              entry->getIpcServer()->logEspRelease(__FILE__, __LINE__);
            if (processStats != NULL)
               processStats->decNumESPs(entry->inUse(), TRUE); // inUse,deletedESPs 
            delete entry;
            }
        } // for j
    } // for i
}

// print esp trace entry one per call
Int32 ExEspManager::printTrace(Int32 lineno, char *buf)
{
  if (lineno >= maxEspTraceIndex_)
    return 0;
  Int32 rv = 0;
  ExEspDbEntry *esp = espTraceArea_[lineno].espEntry_;
  if (!esp) // not used entry
    rv = sprintf(buf, "%.4d   -                                      -\n", lineno);
  else
    {
    IpcGuardianServer *igs = NULL;
    if (esp->getIpcServer())
      igs = esp->getIpcServer()->castToIpcGuardianServer();

    char procName[200];
    short procNameLen = 200;
    Int32 nid = -1;
    Int32 pid = -1;

    //Phandle wrapper in porting layer
    NAProcessHandle phandle((SB_Phandle_Type *)
                        &(igs->getServerId().getPhandle().phandle_));

    Int32 guaRetcode = phandle.decompose();

    if (!guaRetcode)
      {
      procNameLen = phandle.getPhandleStringLen();
      memcpy(procName, phandle.getPhandleString(), procNameLen + 1);
      procName[procNameLen] = 0; // null-terminate
      msg_mon_get_process_info (procName, &nid, &pid);
      }
    else
      procName[0] = 0;

    rv = sprintf(buf, "%.4d  %s (%.3d,%.8x use count: %.2d)  %s\n",
                 lineno, procName, nid, pid, esp->usageCount_,
                 EspStateEnumName[espTraceArea_[lineno].espState_]);
    }
  return rv;

}

// -----------------------------------------------------------------------
// Methods for class ExEspDbEntry
// -----------------------------------------------------------------------

ExEspDbEntry::ExEspDbEntry(CollHeap *heap,
                           IpcServer *server,
			   const char * clusterName,
			   IpcCpuNum cpuNum,
			   Lng32 espLevel,
                           Int32 userId)
{
  key_ = new(heap) ExEspCacheKey(clusterName, cpuNum, userId, heap);
  server_ = server;
  espLevel_          = espLevel;
  idleTimestamp_ = NA_JulianTimestamp();
  inUse_ = false;
  usageCount_ = 0;
  statement_ = NULL;
  totalMemoryQuota_ = 0;
  soloFragment_ = false;
}

ExEspDbEntry::~ExEspDbEntry()
{
  release();
}

void ExEspDbEntry::release()
{
  if (server_)
    {
      server_->release();
      server_ = NULL;
    }
  delete key_;
}

void ExEspDbEntry::setIdleTimestamp()
{
  idleTimestamp_ = NA_JulianTimestamp();    
}

// -----------------------------------------------------------------------
// Methods for class ExEspCacheKey
// -----------------------------------------------------------------------

ExEspCacheKey::ExEspCacheKey(const char *segment, IpcCpuNum cpu,
                             Int32 userId, CollHeap *heap)
  : cpu_(cpu), userId_(userId), heap_(heap)
{
  if (segment && *segment)
    {
      if (heap)
        {
          Int32 len = str_len(segment) + 1;
          segment_ = (char *) heap->allocateMemory(len);
          str_cpy_all(segment_, segment, len);
        }
      else
        {
          segment_ = (char *) segment;
        }
    }
  else
    {
      segment_= NULL;
    }
}

ExEspCacheKey::~ExEspCacheKey()
{
  if (heap_ && segment_)
    heap_->deallocateMemory(segment_);
}

ULng32 ExEspCacheKey::hash() const
{
  ULng32 result = 0;
  char *str = segment_;
  if (str)
    {
      // Bernstein's hash algorithm
      result = 5381;
      Int32 c;
      while (c = *str++)
        // result * 33 + c
        result = ((result << 5) + result) + c;
    }

  result = 31 * result + (ULng32) cpu_;

  // on Windoes userId_ is of type NTSEC_USER and cannot be converted to long.
  // thus do not use it in hash key.
  result = 31 * result + (ULng32) userId_;

  return result;
}

/* static */ ULng32
ExEspCacheKey::hash(const ExEspCacheKey& key)
{
  return key.hash();
}
