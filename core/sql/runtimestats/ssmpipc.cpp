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
// File:         ssmpipc.cpp
// Description:  Class declaration for SSCP IPC infrastructure
//
// Created:      5/02/2006
**********************************************************************/

#include "Platform.h"
#include "ex_stdh.h"
#include "ssmpipc.h"
#include "ComCextdecs.h"
#include <semaphore.h>
#include "nsk/nskport.h"
#include "zsysc.h"
#include "NAStdlib.h"
#include "Ex_esp_msg.h"
#include "ComQueue.h"
#include "ComRtUtils.h"
#include "ComSqlId.h"
#include "Globals.h"
#include "SqlStats.h"
#include "ex_stdh.h"
#include "ExStats.h"
#include "ComDiags.h"
#include "PortProcessCalls.h"
#include "Statement.h"
#include "ComSqlId.h"

ExSsmpManager::ExSsmpManager(IpcEnvironment *env)
  : env_(env)
{

  ssmpServerClass_ = new(env->getHeap()) IpcServerClass(
       env_,
       IPC_SQLSSMP_SERVER,
       IPC_USE_PROCESS);
  ssmps_ = new (env->getHeap()) HashQueue(env->getHeap(), 16);
  deletedSsmps_ = new(env->getHeap()) NAList<IpcServer *>(env->getHeap());
}

ExSsmpManager::~ExSsmpManager()
{
  cleanupDeletedSsmpServers();
  NADELETE(deletedSsmps_, NAList, env_->getHeap());

  ssmps_->position();
  NADELETE(ssmps_, HashQueue, env_->getHeap());

  if (ssmpServerClass_)
  {
    NADELETE(ssmpServerClass_, IpcServerClass, env_->getHeap());
  }
}

IpcServer *ExSsmpManager::getSsmpServer(NAHeap *heap, char *nodeName, short cpuNum,
                                        ComDiagsArea *&diagsArea)
{
   char ssmpProcessName[50];
   IpcServer *ssmpServer = NULL;
   Int32 processNameLen = 0;

   char *tmpProcessName;

   tmpProcessName = ssmpServerClass_->getProcessName(cpuNum, ssmpProcessName);
   ex_assert(tmpProcessName != NULL, "ProcessName can't be null");

   processNameLen = str_len(tmpProcessName);

   // Check if we already have this SSMP server
   ssmps_->position(tmpProcessName, processNameLen);
   ssmpServer = (IpcServer *) ssmps_->getNext();

   while (ssmpServer != NULL)
   {
     if (str_cmp(ssmpServer->castToIpcGuardianServer()->getProcessName(),
            tmpProcessName, processNameLen) == 0)
     {
        GuaConnectionToServer *cbGCTS = ssmpServer->getControlConnection()->castToGuaConnectionToServer();

        // We need to keep 2 entries free - To send QueryFinishedMessage and to get the response for query started message
       if (cbGCTS->numReceiveCallbacksPending()+2 >= cbGCTS->getNowaitDepth())
       {
          if (diagsArea == NULL)
             diagsArea = ComDiagsArea::allocate(heap);
          *diagsArea << DgSqlCode(-2026)
            << DgString0(tmpProcessName)
            << DgInt0(GetCliGlobals()->myCpu())
            << DgInt1(GetCliGlobals()->myPin()); 
          return NULL;
       }
       return ssmpServer;
     }
     ssmpServer = (IpcServer *) ssmps_->getNext();
   }

   // We don't have this SSMP server, so we'll try to allocate one.
   ssmpServer = ssmpServerClass_->allocateServerProcess(&diagsArea,
            env_->getHeap(),
            nodeName,
            cpuNum,
            IPC_PRIORITY_DONT_CARE,
            FALSE);
   if (ssmpServer != NULL && ssmpServer->castToIpcGuardianServer()->isReady())
   {
     tmpProcessName = (char *)ssmpServer->castToIpcGuardianServer()->getProcessName();
     ssmps_->insert(tmpProcessName, str_len(tmpProcessName), ssmpServer);
   }
   else
   {
     if (ssmpServer != NULL)
     {
       ssmpServerClass_->freeServerProcess(ssmpServer);
       ssmpServer = NULL;
     }
   }
   
   
   return ssmpServer;
}

void ExSsmpManager::removeSsmpServer(char *nodeName, short cpuNum)
{
   char ssmpProcessName[50];
   IpcServer *ssmpServer = NULL;
   Int32 processNameLen = 0;

   char *tmpProcessName;
   tmpProcessName = ssmpServerClass_->getProcessName(cpuNum, ssmpProcessName);
   ex_assert(tmpProcessName != NULL, "ProcessName can't be null");

   processNameLen = str_len(tmpProcessName);

   ssmps_->position(tmpProcessName, processNameLen);
   ssmpServer = (IpcServer *)ssmps_->getNext();
   while (ssmpServer != NULL)
   {
     //Only remove the returned ssmpServer if its processName matches the processName
     //we passed in.
     if (str_cmp(ssmpServer->castToIpcGuardianServer()->getProcessName(),
                 tmpProcessName, processNameLen) == 0)
     {
       ssmps_->remove(ssmpServer);
       deletedSsmps_->insert(ssmpServer);
       break;
     }
     ssmpServer = (IpcServer *)ssmps_->getNext();
   }
}

void ExSsmpManager::cleanupDeletedSsmpServers()
{
  IpcServer *ssmp;
  while (deletedSsmps_->getFirst(ssmp))
    {
      ssmpServerClass_->freeServerProcess(ssmp);
    }
}

SsmpGlobals::SsmpGlobals(NAHeap *ssmpheap, IpcEnvironment *ipcEnv,  StatsGlobals *statsGlobals)
    : heap_(ssmpheap),
    statsGlobals_(statsGlobals),
    ipcEnv_(ipcEnv),
    recipients_(ipcEnv_->getAllConnections(),ipcEnv_->getHeap()),
    activeQueryMgr_(ipcEnv_, ssmpheap),
    pendingQueryMgr_(this, ssmpheap)
{
  sscpServerClass_ = NULL;
  sscps_ = new (heap_) HashQueue(heap_, 16);
  deletedSscps_ = new(heap_) NAList<IpcServer *>(heap_);
#ifdef _DEBUG_RTS
  statsCollectionInterval_ = 5 * 60; //in seconds
#else
  statsCollectionInterval_ = 30;// in seconds
#endif

  char defineName[24+1];
  int error;
  short mergeInterval, statsTimeout, sqlSrcLen;

  //Check to see if the user wants to use a different merge interval (default is 30 seocnds).
  //Set this define as follows: ADD DEFINE =_MX_RTS_MERGE_INTERVAL, CLASS DEFAULTS, VOLUME $A.Bnnnnn
  //where nnnnn is the interval in seconds.
  char *ln_attrValue = getenv("_MX_RTS_MERGE_INTERVAL");
  if (ln_attrValue)
  {
    mergeInterval = atoi(ln_attrValue);
    statsCollectionInterval_ = (Int64) MAXOF (mergeInterval, 30);
  }

  statsTimeout_ = 300; // in Centi-seconds
  //Set this define as follows: ADD DEFINE =_MX_RTS_MERGE_TIMEOUT, CLASS DEFAULTS, VOLUME $A.Bnnnnn
  //where nnnnn is the max number of queries.
  ln_attrValue = getenv("_MX_RTS_MERGE_TIMEOUT");
  if (ln_attrValue)
  {
    statsTimeout = atoi(ln_attrValue);
    if (statsTimeout > 1)
	statsTimeout_ = statsTimeout;
    else
        statsTimeout_ = 300;
  }

  storeSqlSrcLen_ = RMS_STORE_SQL_SOURCE_LEN;
  //Set this define as follows: ADD DEFINE =_MX_RTS_SQL_SOURCE_LEN, CLASS DEFAULTS, VOLUME $A.Bnnnnn
  //where nnnnn is the length of the source string
  ln_attrValue = getenv("_MX_RTS_SQL_SOURCE_LEN");
  if (ln_attrValue)
  {
    sqlSrcLen = atoi(ln_attrValue);
    if (sqlSrcLen < 0)
	storeSqlSrcLen_ = 0;
    else
        storeSqlSrcLen_ = sqlSrcLen;
  }

  CliGlobals *cliGlobals = GetCliGlobals();
  char programDir[100];
  short processType;
  char myNodeName[MAX_SEGMENT_NAME_LEN+1];
  Lng32 myNodeNumber;
  short myNodeNameLen = MAX_SEGMENT_NAME_LEN;
  Int64 myStartTime;
  short pri;
  char myProcessName[PROCESSNAME_STRING_LEN];

  error = statsGlobals_->openStatsSemaphore(semId_);
  ex_assert(error == 0, "BINSEM_OPEN returned an error");

  if (ComRtGetProgramInfo(programDir, 100, processType,
			myCpu_, myPin_,
			myNodeNumber, myNodeName, myNodeNameLen, myStartTime, myProcessName))
  {
    ex_assert(0,"Error in ComRtGetProgramInfo");
  }
  pri = 0;
  error = statsGlobals_->getStatsSemaphore(semId_, myPin_);
  NAProcessHandle phandle;

  (void)phandle.getmine(statsGlobals->getSsmpProcHandle());
  statsGlobals_->setSsmpPid(myPin_);
  statsGlobals_->setSsmpPriority(pri);
  statsGlobals_->setSsmpTimestamp(myStartTime);
  statsGlobals_->setStoreSqlSrcLen(storeSqlSrcLen_);
  statsGlobals_->setSsmpProcSemId(semId_);
  cliGlobals->setSemId(semId_);
  statsHeap_ = (NAHeap *)statsGlobals->getStatsHeap()->allocateHeapMemory(sizeof *statsHeap_);
  statsHeap_ = new (statsHeap_, statsGlobals->getStatsHeap())
          NAHeap("Process Stats Heap", statsGlobals->getStatsHeap(),
          8192,
          0);
  statsGlobals_->setSscpOpens(0);
  statsGlobals_->setSscpDeletedOpens(0);
  statsGlobals_->releaseStatsSemaphore(semId_, myPin_);
  deallocatedSscps_ = new (heap_) Queue(heap_);
  doingGC_ = FALSE;
  // Debug code to force merge if the =_MX_SSMP_FORCE_MERGE define
  // is specified..
  ln_attrValue = getenv("_MX_SSMP_FORCE_MERGE");
  if (ln_attrValue)
     forceMerge_ = TRUE;
  else
     forceMerge_ = FALSE;
  pendingSscpMessages_ = new (heap_) Queue(heap_);
}

SsmpGlobals::~SsmpGlobals()
{
  cleanupDeletedSscpServers();
  NADELETE(deletedSscps_, NAList, heap_);

  NADELETE(sscps_, HashQueue, heap_);

  if (sscpServerClass_ != NULL)
  {
    NADELETE(sscpServerClass_, IpcServerClass, heap_);
  }

  sem_close((sem_t *)semId_);
}

ULng32 SsmpGlobals::allocateServers()
{
  // Attempt connect to all SSCPs
  if (sscpServerClass_ == NULL)
  {
    Int32 noOfNodes;
    Int32 *cpuArray = NULL;

    noOfNodes = ComRtGetCPUArray(cpuArray, heap_);

    if (noOfNodes == 0)
      return 0;
    statsGlobals_->setNodesInCluster(noOfNodes);
    sscpServerClass_ = new(heap_) IpcServerClass(ipcEnv_, IPC_SQLSSCP_SERVER, IPC_USE_PROCESS);
    for (Int32 i = 0 ; i < noOfNodes ; i++)
    {
      allocateServer(NULL, 0, cpuArray[i]);
    }
    NADELETEBASIC(cpuArray, heap_);
  }
  else
  {
    ServerId *serverId;
    IpcServer *server;
    deallocatedSscps_->position();
    while ((serverId = (ServerId *)deallocatedSscps_->getNext()) != NULL)
    {
      server = allocateServer(serverId->nodeName_, (short)str_len(serverId->nodeName_),
              serverId->cpuNum_);
      if (server != NULL)
      {
        deallocatedSscps_->remove(NULL);
        statsGlobals_->setSscpDeletedOpens(getNumDeallocatedServers());
        NADELETEBASIC(serverId, heap_);
      }
    }
  }
  return sscps_->entries();
}

IpcServer *SsmpGlobals::allocateServer(char *nodeName, short nodeNameLen, short cpuNum)
{
  short len;
  IpcServer *server;
  const char *processName;
 ComDiagsArea *diagsArea = NULL;

  ServerId serverId;

  // No first connection yet
  if (sscpServerClass_ == NULL)
    return NULL;
  serverId.nodeName_[0] = '\0';

  serverId.cpuNum_ = cpuNum;
  IpcAllocateDiagsArea(diagsArea, heap_);
  server = sscpServerClass_->allocateServerProcess(&diagsArea,
            heap_,
	    serverId.nodeName_,
            serverId.cpuNum_,
            IPC_PRIORITY_DONT_CARE,
            1, // espLevel
            FALSE);
  if (server != NULL && server->castToIpcGuardianServer()->isReady())
  {
    processName = server->castToIpcGuardianServer()->getProcessName();
    sscps_->insert(processName, str_len(processName), server);
    statsGlobals_->setSscpOpens(getNumAllocatedServers());
    recipients_ += server->getControlConnection()->getId();
  }
  else
  {
    if (server != NULL)
    {
      sscpServerClass_->freeServerProcess(server);
      server = NULL;
    }
    insertDeallocatedSscp(serverId.nodeName_, serverId.cpuNum_);
  }
  diagsArea->decrRefCount();
  return server;
}

void SsmpGlobals::insertDeallocatedSscp(char *nodeName, short cpuNum )
{
  ServerId *serverId;

  short len = strlen(nodeName);
  deallocatedSscps_->position();
  while ((serverId = (ServerId *)deallocatedSscps_->getNext()) != NULL)
  {
    // ServerId already exists in deallocatedSscps_ list and hence don't add again
    // But, do delete it if the cpu is Down
    if ((str_cmp(serverId->nodeName_, nodeName, len) == 0) && serverId->cpuNum_ == cpuNum)
    {
       if (!ComRtGetCpuStatus(nodeName, cpuNum))
       {
          deallocatedSscps_->remove(NULL);
          statsGlobals_->setSscpDeletedOpens(getNumDeallocatedServers());
          NADELETEBASIC(serverId, heap_);
       }
       return;
    }
  }
  // If  the CPU is DOWN, then we don’t
  // want to insert it into deallocateSscps_ until we
  // get a CPUUP message later.  Because there is no point
  // trying to reopen the process until its CPU is up.

  if (ComRtGetCpuStatus(nodeName, cpuNum))
  {
     ServerId *entry = new (heap_) ServerId;
     str_cpy_all(entry->nodeName_, nodeName, len);
     entry->nodeName_[len] = '\0';
     entry->cpuNum_ = cpuNum;
     deallocatedSscps_->insert(entry, sizeof(ServerId));
     statsGlobals_->setSscpDeletedOpens(getNumDeallocatedServers());
  }
}

ULng32 SsmpGlobals::deAllocateServer(char *nodeName, short nodeNameLen,  short cpuNum )
{
  char sscpProcessName[50];
  ServerId serverId;
  short len;
  IpcServer *sscpServer;

  if (sscpServerClass_ == NULL)
    return 0;
  serverId.nodeName_[0] = '\0';
  serverId.cpuNum_ = cpuNum;
  char *tmpProcessName;
  tmpProcessName = sscpServerClass_->getProcessName(cpuNum, sscpProcessName);
  ex_assert(tmpProcessName != NULL, "ProcessName can't be null");

  sscps_->position(tmpProcessName, str_len(tmpProcessName));

  while ((sscpServer = (IpcServer *)sscps_->getNext()) != NULL)
  {
    //Only remove the returned sscpServer if its processName matches the processName
    //we passed in.
    if (str_cmp(sscpServer->castToIpcGuardianServer()->getProcessName(),
                tmpProcessName, str_len(tmpProcessName)) == 0)
        break;
  }
  if (sscpServer != NULL)
  {
    // Remove the sscpServer if it is in the list, i.e., it has not already
    // been removed.
    sscps_->remove(sscpServer);
    deletedSscps_->insert(sscpServer);
    statsGlobals_->setSscpOpens(getNumAllocatedServers());
    insertDeallocatedSscp(serverId.nodeName_, serverId.cpuNum_);
  }
  return sscps_->entries();
}

void SsmpGlobals::cleanupDeletedSscpServers()
{
  NAList<IpcServer *> notReadyToCleanup(heap_);
  IpcServer *sscp;
  while (deletedSscps_->getFirst(sscp))
    {
      IpcConnection *conn = sscp->getControlConnection();
      if (conn->numQueuedSendMessages()    ||
          conn->numQueuedReceiveMessages() ||
          conn->numReceiveCallbacksPending() ||
          conn->hasActiveIOs())
        {
          notReadyToCleanup.insert(sscp);
        }
      else
        {
          sscpServerClass_->freeServerProcess(sscp);
        }
    }
  deletedSscps_->insert(notReadyToCleanup);
}

void SsmpGlobals::allocateServerOnNextRequest(char *nodeName,
                                              short nodeNameLen,
                                              short cpuNum)
{
  char sscpProcessName[50];
  ServerId serverId;
  short len;
  IpcServer *sscpServer;

  if (sscpServerClass_ == NULL)
    return;

  serverId.nodeName_[0] = '\0';
  serverId.cpuNum_ = cpuNum;

  // Next, the code will do an integrity check, to make sure that
  // this server isn't already allocated.  If we fail the test,
  // we will pretend that we got a NodeDown and NodeUp message and
  // issue an EMS event.
  char *tmpProcessName;
  tmpProcessName = sscpServerClass_->getProcessName(cpuNum, sscpProcessName);
  ex_assert(tmpProcessName != NULL, "ProcessName can't be null");

  sscps_->position(tmpProcessName, str_len(tmpProcessName));

  while ((sscpServer = (IpcServer *)sscps_->getNext()) != NULL)
  {
    if (str_cmp(sscpServer->castToIpcGuardianServer()->getProcessName(),
                tmpProcessName, str_len(tmpProcessName)) == 0)
        break;
  }

  if (sscpServer != NULL)
  {
    // We don't seem to have gotten a Node Down message.
    // Pretend we got one and do the Node Down message processing.
    // Note that deAllocateServer will execute insertDeallocatedSscp.
    deAllocateServer(nodeName, nodeNameLen, cpuNum);
    // Issue an EMS event to help track occurrences of this phenomenon
    char msg[100];
    str_sprintf(msg,"Node UP received before Node DOWN for nid: %d", cpuNum);
    SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg, 0);
  }
  else
  {
    // By placing the server into the deallocatedSscps_, we ensure that
    // the next time we need to send a request to the server for stats,
    // we will first allocate it.
    insertDeallocatedSscp(serverId.nodeName_, serverId.cpuNum_);
  }
  return;
}

static Int64 GcInterval = -1;
static Int64 SikGcInterval = -1;

void SsmpGlobals::work()
{

  getIpcEnv()->getAllConnections()->waitOnAll(getStatsMergeTimeout());
  finishPendingSscpMessages();

  // Cleanup IpcEnvironment
  cleanupDeletedSscpServers();
  getIpcEnv()->deleteCompletedMessages();

  //Perform cancel escalation, as needed.
  pendingQueryMgr_.killPendingCanceled();

  StatsGlobals *statsGlobals = getStatsGlobals();

  statsGlobals->cleanupDanglingSemaphore(TRUE);

  // Every time we get here either because we timed out or because a request came in, check whether
  // any GC needs to be done to deallocate space for the master executor for any final stats. If
  // it has been at least 20 minutes since the last time we did GC, and it has been at least 15
  // minutes since a collector has requested stats for a query, and the master stats for that
  // query has its canBeGCed flag set to true, we'll clean it up.
  Int64 currTime = NA_JulianTimestamp();
  Int64 temp2 = (Int64)(currTime - statsGlobals->getLastGCTime());
  if (GcInterval < 0)
    {
      // call getenv once per process
      char *sct = getenv("RMS_GC_INTERVAL_SECONDS");
      if (sct)
      {
        GcInterval = ((Int64) str_atoi(sct, str_len(sct))) * 1000 * 1000;
        if (GcInterval < 10*1000*1000)
          GcInterval = 10*1000*1000;
      }
      else
        GcInterval = 10 * 60 *  1000 * 1000; // 10 minutes
    }
  if (SikGcInterval < 0)
    {
      // Note: If you change this logic see also the logic to update
      // siKeyGCinterval_ in optimizer/opt.cpp.

      // call getenv once per process
      char *sct = getenv("RMS_SIK_GC_INTERVAL_SECONDS");
      if (sct)
        {
          SikGcInterval = ((Int64) str_atoi(sct, str_len(sct))) * 1000 * 1000;
          if (SikGcInterval < 10*1000*1000)
            SikGcInterval = 10*1000*1000;
        }
      else
        SikGcInterval = (Int64)24 * 60 * 60 * 1000 * 1000; // 24 hours
    }


  if  ((temp2 > GcInterval) && !doingGC())
  {
    // It's been more than 20 minutes since we did a full GC. Do it again now
    // and update the time of last GC.
    statsGlobals->checkForDeadProcesses(myPin_);
    setDoingGC(TRUE);
    int error = statsGlobals->getStatsSemaphore(semId_, myPin_);
    statsGlobals->doFullGC();
    statsGlobals->setLastGCTime(NA_JulianTimestamp());
    statsGlobals->cleanupOldSikeys(SikGcInterval);
    setDoingGC(FALSE);
    statsGlobals->releaseStatsSemaphore(semId_, myPin_);
  }
}

void SsmpGlobals::addRecipients(SscpClientMsgStream *msgStream)
{
  IpcServer *server;

  sscps_->position();
  while ((server = (IpcServer *)sscps_->getNext()) != NULL)
  {
    msgStream->addRecipient(server->getControlConnection());
    msgStream->incNumOfClientRequestsSent();
  }
}

void SsmpGlobals::finishPendingSscpMessages()
{
  SscpClientMsgStream *sscpClientMsgStream;
  if (pendingSscpMessages_->numEntries() == 0)
    return;
  pendingSscpMessages_->position();
  Int64 currTimestamp = NA_JulianTimestamp();
  while ((sscpClientMsgStream = (SscpClientMsgStream *)pendingSscpMessages_->getNext()) != NULL)
  {
    if ((currTimestamp - sscpClientMsgStream->getMergeStartTime()) >= (getStatsMergeTimeout() * 1000))
    {
      sscpClientMsgStream->sendMergedStats();
      pendingSscpMessages_->remove(NULL);
    }
  }
}

void SsmpGlobals::removePendingSscpMessage(SscpClientMsgStream *sscpClientMsgStream)
{
  SscpClientMsgStream *lcSscpClientMsgStream;

  pendingSscpMessages_->position();
  while ((lcSscpClientMsgStream = (SscpClientMsgStream *)pendingSscpMessages_->getNext()) != NULL)
  {
    if (lcSscpClientMsgStream == sscpClientMsgStream)
    {
      pendingSscpMessages_->remove(NULL);
      break;
    }
  }
}

bool SsmpGlobals::getQidFromPid( Int32 pid,         // IN
                                 Int32 minimumAge,  // IN
                                 char *queryId,     // OUT
                                 Lng32 &queryIdLen  // OUT
                               )
{
  bool foundQid = false;
  StatsGlobals *statsGlobals = getStatsGlobals();
  short error =
    statsGlobals->getStatsSemaphore(getSemId(), myPin());

  SyncHashQueue *ssList = statsGlobals->getStmtStatsList();
  ssList->position();
  StmtStats *ss = (StmtStats *)ssList->getNext();

  while (ss != NULL)
  {
    if (ss->getPid() == pid &&
        ss->getMasterStats() &&
        (ss->getMasterStats()->timeSinceBlocking(minimumAge) > 0))
    {
      bool finishedSearch = true;
      ExMasterStats *m = ss->getMasterStats();
      char *parentQid = m->getParentQid();
      Lng32 parentQidLen = m->getParentQidLen();
      if (parentQid != NULL)
      {
        // If this query has a parent in this cpu, will keep looking.
        Int64 parentCpu = -1;
        ComSqlId::getSqlQueryIdAttr(ComSqlId::SQLQUERYID_CPUNUM,
                  parentQid, parentQidLen, parentCpu, NULL);
        if (parentCpu == statsGlobals->getCpu())
        {
          finishedSearch = false;
        }
        else
          ; // Even tho this query has a parent, it is not
            // executing on the same node as this query's
            // process, so following the semantics of
            // cancel-by-pid, we will not attempt to
            // cancel the parent.
      }

      if (finishedSearch)
      {
        queryIdLen = ss->getQueryIdLen();
        str_cpy_all(queryId, ss->getQueryId(), queryIdLen);
        foundQid = true;
        break;
      }
    }
    ss = (StmtStats *)ssList->getNext();
  }
  statsGlobals->releaseStatsSemaphore(getSemId(), myPin());
  return foundQid;
}

bool SsmpGlobals::cancelQueryTree(char *queryId, Lng32 queryIdLen,
                                  CancelQueryRequest *request,
                                  ComDiagsArea **diags)
{
  bool didCancel = false;
  bool hasChildQid = false;
  char childQid[ComSqlId::MAX_QUERY_ID_LEN + 1];
  Lng32 childQidLen = 0;
  StatsGlobals *statsGlobals = getStatsGlobals();
  int error =
    statsGlobals->getStatsSemaphore(getSemId(), myPin());

  StmtStats *cqStmtStats = statsGlobals->getMasterStmtStats(
    queryId, queryIdLen,
    RtsQueryId::ANY_QUERY_);

  ExMasterStats *m = cqStmtStats ? cqStmtStats->getMasterStats(): NULL;
  if (m && m->getChildQid())
  {
    hasChildQid = true;
    str_cpy_all(childQid, m->getChildQid(), m->getChildQidLen());
    childQid[m->getChildQidLen()] = '\0';
    childQidLen = m->getChildQidLen();
  }

  statsGlobals->releaseStatsSemaphore(getSemId(), myPin());

  if (cqStmtStats == NULL)
  {
    ; // race condition
    return false;
  }

  if (hasChildQid)
  {
    if (request->getCancelLogging())
    {
      char thisQid[ComSqlId::MAX_QUERY_ID_LEN + 1];
      str_cpy_all(thisQid, queryId, queryIdLen);
      thisQid[queryIdLen] = '\0';

      char msg[120 + // the constant text
             ComSqlId::MAX_QUERY_ID_LEN +
             ComSqlId::MAX_QUERY_ID_LEN ];

      str_sprintf(msg,
        "cancelQueryTree for %s , found a child qid %s",
       thisQid, childQid);

      SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg, 0);
    }

    didCancel = cancelQueryTree(childQid,childQidLen, request, diags);
  }

  if (cancelQuery(queryId, queryIdLen, request, diags))
    didCancel = true;

  return didCancel;
}

bool SsmpGlobals::cancelQuery(char *queryId, Lng32 queryIdLen,
                              CancelQueryRequest *request,
                              ComDiagsArea **diags)
{
  bool didAttemptCancel = false;
  Int64 cancelStartTime = request->getCancelStartTime ();
  Int32 ceFirstInterval   = request->getFirstEscalationInterval();
  Int32 ceSecondInterval  = request->getSecondEscalationInterval();
  NABoolean ceSaveabend = request->getCancelEscalationSaveabend();
  bool cancelLogging = request->getCancelLogging();

  short sqlErrorCode = 0;
  const char *sqlErrorDesc = NULL;
  StatsGlobals *statsGlobals = getStatsGlobals();
  int error;
  char tempQid[ComSqlId::MAX_QUERY_ID_LEN+1];

  static int stopProcessAfterInSecs = 
             (getenv("MIN_QUERY_ACTIVE_TIME_IN_SECS_BEFORE_CANCEL") != NULL ? atoi(getenv("MIN_QUERY_ACTIVE_TIME_IN_SECS_BEFORE_CANCEL")) : -1);
  ActiveQueryEntry * aq = (queryId ? getActiveQueryMgr().getActiveQuery(
                       queryId, queryIdLen) : NULL);
  ExMasterStats * cMasterStats = NULL;
  StmtStats *cqStmtStats = NULL;

  if (aq == NULL)
  {
     error = statsGlobals->getStatsSemaphore(getSemId(), myPin());
     cqStmtStats = statsGlobals->getMasterStmtStats(
                queryId, queryIdLen,
                RtsQueryId::ANY_QUERY_);
     if (cqStmtStats == NULL) {
        sqlErrorCode = -EXE_CANCEL_QID_NOT_FOUND;
        statsGlobals->releaseStatsSemaphore(getSemId(), myPin());
     } else {
        cMasterStats = cqStmtStats->getMasterStats();
        if (cMasterStats == NULL) {
            sqlErrorCode = -EXE_CANCEL_NOT_POSSIBLE;
            sqlErrorDesc = "The query is not registered with cancel broker";
            statsGlobals->releaseStatsSemaphore(getSemId(), myPin());
        } else {
           Statement::State stmtState = (Statement::State)cMasterStats->getState();
           if (stmtState != Statement::OPEN_ &&
                   stmtState  != Statement::FETCH_ &&
                   stmtState != Statement::STMT_EXECUTE_) {
              sqlErrorCode = -EXE_CANCEL_NOT_POSSIBLE;
              sqlErrorDesc = "The query is not in OPEN or FETCH or EXECUTE state";
              statsGlobals->releaseStatsSemaphore(getSemId(), myPin());
           } else {
              if ((stopProcessAfterInSecs <= 0) || (cMasterStats->getExeEndTime() != -1)) {
                 sqlErrorCode = -EXE_CANCEL_NOT_POSSIBLE;
                 sqlErrorDesc = "The query can't be canceled because it finished processing";
                 statsGlobals->releaseStatsSemaphore(getSemId(), myPin());
              } else {
                 Int64 exeStartTime = cMasterStats->getExeStartTime();
                 int exeElapsedTimeInSecs = 0;
                 if (exeStartTime != -1) {
                    Int64 exeElapsedTime = NA_JulianTimestamp() - cMasterStats->getExeStartTime();
                    exeElapsedTimeInSecs = exeElapsedTime / 1000000;
                 }
                 statsGlobals->releaseStatsSemaphore(getSemId(), myPin());
                 if (exeElapsedTimeInSecs > 0 && exeElapsedTimeInSecs > (stopProcessAfterInSecs)) {
                    sqlErrorCode = stopMasterProcess(queryId, queryIdLen); 
                    if (sqlErrorCode != 0) {
                       switch (sqlErrorCode) {
                          case -1:
                             sqlErrorDesc = "Unable to get node number";
                             break;
                          case -2:
                             sqlErrorDesc = "Unable to get pid";
                             break;
                          case -3:
                             sqlErrorDesc = "Unable to get process name";
                             break;
                          default:
                             sqlErrorDesc = "Unable to stop the process";
                             break; 
                       } // switch
                       sqlErrorCode = -EXE_CANCEL_NOT_POSSIBLE;
                    } else 
                      didAttemptCancel = true;
                 } else {
                     sqlErrorDesc = "The query can't be canceled because cancel was requested earlier than required minimum query active time";
                     sqlErrorCode = -EXE_CANCEL_NOT_POSSIBLE;
                 } // stopAfterNSecs
              } // ExeEndTime
          } //StmtState
       } // cMasterStats 
    } // cqStmtStats
  } // aq
  else
  if (aq && (aq->getQueryStartTime() <= cancelStartTime))
  {
    didAttemptCancel = true;

    // Make sure query is activated first.  If it is already activated,
    // some error conditions will be raised.  Ignore these.
    ComDiagsArea *unimportantDiags = NULL;

    activateFromQid(queryId, queryIdLen,
                    ACTIVATE, unimportantDiags, cancelLogging);
    if (unimportantDiags)
      unimportantDiags->decrRefCount();

    StatsGlobals *statsGlobals = getStatsGlobals();
    int error = statsGlobals->getStatsSemaphore(getSemId(), myPin());

    StmtStats *cqStmtStats = statsGlobals->getMasterStmtStats(
      queryId, queryIdLen,
      RtsQueryId::ANY_QUERY_);

    if (cqStmtStats == NULL)
    {
      ; // race condition - query is gone, but we haven't cleanup up
        // active query entry yet.
    }
    else
    {
      ExMasterStats * cMasterStats = cqStmtStats->getMasterStats();
      if (cMasterStats)
        {
        cMasterStats->setCanceledTime(JULIANTIMESTAMP());
        cMasterStats->setCancelComment(request->getComment());
        }
    }
    statsGlobals->releaseStatsSemaphore(getSemId(), myPin());

    // Set up for escalation later.
    if ((ceFirstInterval != 0) || (ceSecondInterval != 0))
    {
      getPendingQueryMgr().addPendingQuery(aq,
        ceFirstInterval, ceSecondInterval, ceSaveabend, cancelLogging);
    }

    // This call makes the reply to the Query Started message.
    getActiveQueryMgr().rmActiveQuery(
          queryId, queryIdLen, getHeap(),
          CB_CANCEL, cancelLogging);
  }
  else
  {
     sqlErrorCode = -EXE_CANCEL_NOT_POSSIBLE;
     sqlErrorDesc = "You tried to cancel the subsequent execution of the query";
  }

  ComDiagsArea *lcDiags = NULL;
  if (sqlErrorCode != 0)
  {
     str_cpy_all(tempQid, queryId, queryIdLen);
     tempQid[queryIdLen] = '\0';
     lcDiags = ComDiagsArea::allocate(getHeap());
     if (sqlErrorDesc != NULL)
     {
        *lcDiags << DgSqlCode(sqlErrorCode)
               << DgString0(tempQid)
               << DgString1(sqlErrorDesc);
     }
     else
        *lcDiags << DgSqlCode(sqlErrorCode)
                << DgString0(tempQid);
  }
  *diags = lcDiags;
  return didAttemptCancel;
}


void SsmpGlobals::suspendOrActivate(
                char *queryId, Lng32 qidLen, SuspendOrActivate sOrA,
                bool suspendLogging)
{
  allocateServers();

  SscpClientMsgStream *sscpMsgStream = new (getHeap())
      SscpClientMsgStream((NAHeap *)getHeap(), getIpcEnv(), this, NULL);

  sscpMsgStream->setUsedToSendCbMsgs();

  addRecipients(sscpMsgStream);

  sscpMsgStream->clearAllObjects();

  SuspendActivateServersRequest *requestForSscp = new (getHeap())
      SuspendActivateServersRequest((RtsHandle) sscpMsgStream,
                                    getHeap(),
                                    (sOrA == SUSPEND), suspendLogging
                                   );
  *sscpMsgStream << *requestForSscp;

  RtsQueryId *qidObjForSscp = new (getHeap())
       RtsQueryId( getHeap(), queryId, qidLen);

  *sscpMsgStream << *qidObjForSscp;

  // Send the Message to all
  // Do not do this? ssmpGlobals_->addPendingSscpMessage(sscpMsgStream);
  sscpMsgStream->send(FALSE);

  requestForSscp->decrRefCount();
  qidObjForSscp->decrRefCount();

}

bool SsmpGlobals::activateFromQid(
                               char *qid, Lng32 qidLen,
                               SuspendOrActivate /*
                                                   sOrAOrC */,
                               ComDiagsArea *&diags,
                               bool suspendLogging)
{
  bool doAttemptActivate = true;
  // Find the query.
  StatsGlobals *statsGlobals = getStatsGlobals();

  int error = statsGlobals->getStatsSemaphore(getSemId(), myPin());

  SyncHashQueue *stmtStatsList = statsGlobals->getStmtStatsList();
  stmtStatsList->position(qid, qidLen);

  StmtStats *kqStmtStats = NULL;
  ExMasterStats *masterStats = NULL;

  while (NULL != (kqStmtStats = (StmtStats *)stmtStatsList->getNext()))
  {
    if (str_cmp(kqStmtStats->getQueryId(), qid, qidLen) == 0)
    {
      // Can control only queries which are executing.
      masterStats = kqStmtStats->getMasterStats();
      if ( masterStats &&
          (masterStats->getExeStartTime() != -1) &&
          (masterStats->getExeEndTime() == -1) )
      break;
    }
  }

  if (!masterStats)
  {
    doAttemptActivate = false;
    if (diags == NULL)
      diags = ComDiagsArea::allocate(getHeap());
    *diags << DgSqlCode(-EXE_RTS_QID_NOT_FOUND)
           << DgString0(qid);
  }
  else if (!masterStats->isQuerySuspended())
    {
      doAttemptActivate = false;

      if (diags == NULL)
        diags = ComDiagsArea::allocate(getHeap());

      if (!masterStats->isReadyToSuspend())
        *diags << DgSqlCode(-EXE_SUSPEND_QID_NOT_ACTIVE);
      else
        *diags << DgSqlCode(-EXE_SUSPEND_NOT_SUSPENDED);
    }

  if (doAttemptActivate)
  {
    suspendOrActivate(qid, qidLen, ACTIVATE, suspendLogging);
    masterStats->setQuerySuspended(false);
  }

  statsGlobals->releaseStatsSemaphore(getSemId(), myPin());

  if (doAttemptActivate && suspendLogging)
  {
    char msg[80 + // the constant text
             ComSqlId::MAX_QUERY_ID_LEN];

    str_sprintf(msg,
           "MXSSMP has processed a request to reactivate query %s.",
            qid);

    SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg, 0);
  }

  return doAttemptActivate;
}

Lng32 SsmpGlobals::stopMasterProcess(char *queryId, Lng32 queryIdLen)
{
   Lng32 retcode;
   Int64 node;
   Int64 pin;
   char processName[MS_MON_MAX_PROCESS_NAME+1];

   if ((retcode = ComSqlId::getSqlSessionIdAttr(ComSqlId::SQLQUERYID_CPUNUM, queryId, queryIdLen, node, NULL)) != 0)
      return -1;
   if ((retcode = ComSqlId::getSqlSessionIdAttr(ComSqlId::SQLQUERYID_PIN, queryId, queryIdLen, pin, NULL)) != 0)
      return -2;
   if ((retcode = msg_mon_get_process_name((int)node, (int)pin, processName)) != XZFIL_ERR_OK)
      return -3; 
   if ((retcode = msg_mon_stop_process_name(processName)) != XZFIL_ERR_OK)
      return retcode;   
   return 0;    
}

void SsmpGuaReceiveControlConnection::actOnSystemMessage(
       short                  messageNum,
       IpcMessageBufferPtr    sysMsg,
       IpcMessageObjSize      sysMsgLen,
       short                  clientFileNumber,
       const GuaProcessHandle &clientPhandle,
       GuaConnectionToClient  *connection)
{
  switch (messageNum)
    {
    case ZSYS_VAL_SMSG_OPEN:
      {
        SsmpNewIncomingConnectionStream *newStream = new(getEnv()->getHeap())
          SsmpNewIncomingConnectionStream((NAHeap *)getEnv()->getHeap(),
              getEnv(),getSsmpGlobals());

        ex_assert(connection != NULL,"Must create connection for open sys msg");
        newStream->addRecipient(connection);
        newStream->receive(FALSE);
      }
      initialized_ = TRUE;
      break;
    case ZSYS_VAL_SMSG_CLOSE:
      ssmpGlobals_->getActiveQueryMgr().clientIsGone(clientPhandle,
                                                      clientFileNumber);
      ssmpGlobals_->getPendingQueryMgr().clientIsGone(clientPhandle,
                                                       clientFileNumber);
      break;
    case ZSYS_VAL_SMSG_PROCDEATH:
      {
        zsys_ddl_smsg_procdeath_def *msg =
          (zsys_ddl_smsg_procdeath_def *) sysMsg;
        SB_Phandle_Type  *phandle = (SB_Phandle_Type *)&msg->z_phandle;
        Int32 cpu;
        pid_t pid;
        SB_Int64_Type seqNum = 0;
        if (XZFIL_ERR_OK == XPROCESSHANDLE_DECOMPOSE_(
              phandle, &cpu, &pid
              , NULL   // nodeNumber
              , NULL   // nodeName
              , 0      // nodeNameLen input
              , NULL   // nodeNameLen output
              , NULL   // processName
              , 0      // processNameLen input
              , NULL   // processNameLen output
              , &seqNum
          ))            
        {
          if (cpu == ssmpGlobals_->myCpu())
            ssmpGlobals_->getStatsGlobals()->verifyAndCleanup(pid, seqNum);
        }
      }
      break;
    case ZSYS_VAL_SMSG_CPUDOWN:
      {
	zsys_ddl_smsg_cpudown_def *msg =
	      (zsys_ddl_smsg_cpudown_def *) sysMsg;
#ifdef _DEBUG
        cout << "Cpu Down received for NULL " << msg->z_cpunumber << endl;
#endif
        ssmpGlobals_->deAllocateServer(NULL, 0, msg->z_cpunumber);
      }
      break;
    case ZSYS_VAL_SMSG_REMOTECPUDOWN:
      {
	zsys_ddl_smsg_remotecpudown_def *msg =
	      (zsys_ddl_smsg_remotecpudown_def *) sysMsg;
#ifdef _DEBUG
        char la_nodename[msg->z_nodename_len + 1];
        memcpy(la_nodename, msg->z_nodename, (size_t) msg->z_nodename_len);
        la_nodename[msg->z_nodename_len] = '\0';
        cout << "Remote CPU DOWN received for " << la_nodename << " " << msg->z_cpunumber << endl;
#endif
        ssmpGlobals_->deAllocateServer(msg->z_nodename, msg->z_nodename_len, msg->z_cpunumber);
      }
      break;
    case ZSYS_VAL_SMSG_CPUUP:
      {
	zsys_ddl_smsg_cpuup_def *msg =
	      (zsys_ddl_smsg_cpuup_def *) sysMsg;
#ifdef _DEBUG
        cout << "CPU UP received for NULL " << msg->z_cpunumber << endl;
#endif
        ssmpGlobals_->allocateServerOnNextRequest(NULL, 0, msg->z_cpunumber);
      }
      break;
    case ZSYS_VAL_SMSG_REMOTECPUUP:
      {
	zsys_ddl_smsg_remotecpuup_def *msg =
	      (zsys_ddl_smsg_remotecpuup_def *) sysMsg;
#ifdef _DEBUG
        char la_nodename[msg->z_nodename_len + 1];
        memcpy( la_nodename, msg->z_nodename, (size_t) msg->z_nodename_len );
        la_nodename[msg->z_nodename_len] = '\0';
        cout << "Remote CPU UP received for " << la_nodename << " " << msg->z_cpunumber;
#endif
        ssmpGlobals_->allocateServerOnNextRequest(msg->z_nodename,
                                       msg->z_nodename_len, msg->z_cpunumber);
      }
      break;
    case ZSYS_VAL_SMSG_NODEUP:
      break;
    case ZSYS_VAL_SMSG_NODEDOWN:
      ssmpGlobals_->releaseOrphanEntries();
      break;
    case XZSYS_VAL_SMSG_SHUTDOWN:
      NAExit(0);
      break;
    default:
      // do nothing for all other kinds of system messages
      break;
    } // switch
}

SsmpNewIncomingConnectionStream::~SsmpNewIncomingConnectionStream()
{
  if (sscpDiagsArea_)
  {
    sscpDiagsArea_->decrRefCount();
    sscpDiagsArea_ = NULL;
  }
}

void SsmpNewIncomingConnectionStream::actOnSend(IpcConnection *connection)
{
   // check for OS errors
  if (connection->getErrorInfo() == 0)
    ssmpGlobals_->incSsmpReplyMsg(connection->getLastSentMsg()->getMessageLength());
}

void SsmpNewIncomingConnectionStream::actOnSendAllComplete()
{
  // Wait for the next request for the same stream
  clearAllObjects();
  receive(FALSE);
}

void SsmpNewIncomingConnectionStream::actOnReceive(IpcConnection *connection)
{
  // check for OS errors
  if (connection->getErrorInfo() != 0)
    return;
  ssmpGlobals_->incSsmpReqMsg(connection->getLastReceivedMsg()->getMessageLength());
  // take a look at the type of the first object in the message
  switch(getNextObjType())
  {
  case RTS_MSG_STATS_REQ:
    actOnStatsReq(connection);
    break;
  case RTS_MSG_CPU_STATS_REQ:
    actOnCpuStatsReq(connection);
    break;
  case RTS_MSG_EXPLAIN_REQ:
    actOnExplainReq(connection);
    break;
  case CANCEL_QUERY_STARTED_REQ:
    actOnQueryStartedReq(connection);
    break;
  case CANCEL_QUERY_FINISHED_REQ:
    actOnQueryFinishedReq(connection);
    break;
  case CANCEL_QUERY_REQ:
    actOnCancelQueryReq(connection);
    break;
  case SUSPEND_QUERY_REQ:
    actOnSuspendQueryReq(connection);
    break;
  case ACTIVATE_QUERY_REQ:
    actOnActivateQueryReq(connection);
    break;
  case SECURITY_INVALID_KEY_REQ:
    actOnSecInvalidKeyReq(connection);
    break;
  case LOB_LOCK_REQ:
    actOnLobLockReq(connection);
    break;
  default:
    ex_assert(FALSE,"Invalid request from client");
  }

}

void SsmpNewIncomingConnectionStream::actOnReceiveAllComplete()
{
  if (getState() == ERROR_STATE)
    addToCompletedList();
}

void SsmpNewIncomingConnectionStream::actOnQueryStartedReq(IpcConnection *connection)
{
  IpcMessageObjVersion msgVer;

  msgVer = getNextObjVersion();

  ex_assert(msgVer <= currRtsStatsReqVersionNumber, "Up-rev message received.");

  QueryStarted *request = new (getHeap())
    QueryStarted(INVALID_RTS_HANDLE, getHeap());

  *this >> *request;
  setHandle(request->getHandle());

  if (moreObjects())
  {
    RtsMessageObjType objType =
      (RtsMessageObjType) getNextObjType();

    switch (objType)
    {
    case RTS_QUERY_ID:
      {
        RtsQueryId *queryId = new (getHeap())
          RtsQueryId(getHeap());
        // Get the query Id from IPC
        *this >> *queryId;
        clearAllObjects();

        ssmpGlobals_->getActiveQueryMgr().addActiveQuery(
                  queryId->getQid(), queryId->getQueryIdLen(),
                  request->getStartTime(), request->getMasterPhandle(),
                  request->getExecutionCount(), this, connection);

        request->decrRefCount();
        queryId->decrRefCount();

      }
      break;
    default:
      ex_assert(0, "something besides an RTS_QUERY_ID followed QueryStarted");
      break;
    }
  }
  else
    ex_assert(0, "expected an RTS_QUERY_ID following a QueryStarted");

  // start another receive operation for the next request
  receive(FALSE);

  return;
}

void SsmpNewIncomingConnectionStream::actOnQueryFinishedReq(
                                                     IpcConnection *connection)
{
  ex_assert(getNextObjVersion()
            <= currRtsStatsReqVersionNumber, "Up-rev message received.");

  QueryFinished *request = new (getHeap())
    QueryFinished(INVALID_RTS_HANDLE, getHeap());

  *this >> *request;
  setHandle(request->getHandle());
  request->decrRefCount();

  if (moreObjects())
  {
    RtsMessageObjType objType =
      (RtsMessageObjType) getNextObjType();

    switch (objType)
    {
    case RTS_QUERY_ID:
      {
        RtsQueryId *queryId = new (getHeap())
          RtsQueryId(getHeap());
        // Get the query Id from IPC
        *this >> *queryId;
        clearAllObjects();

        // This call makes the reply to the Query Started message.
        ssmpGlobals_->getActiveQueryMgr().rmActiveQuery(queryId->getQid(),
          queryId->getQueryIdLen(), getHeap(), CB_COMPLETE,
          false /*no cancel logging */);

        queryId->decrRefCount();

        // Now, make a reply to the Query Finished message.
        RmsGenericReply *qfReply = new (getHeap())
                                      RmsGenericReply(getHeap());

        *this << *qfReply;
        qfReply->decrRefCount();
        send(FALSE);
      }
      break;
    default:
      ex_assert(0, "something besides an RTS_QUERY_ID followed QueryFinished");
      break;
    }
  }
  else
    ex_assert(0, "expected an RTS_QUERY_ID following a QueryFinished");

}

void SsmpNewIncomingConnectionStream::actOnCancelQueryReq(
                                          IpcConnection *connection)
{
  ex_assert(getNextObjVersion() <= currRtsStatsReqVersionNumber,
            "Up-rev message received.");

  CancelQueryRequest *request = new (getHeap())
    CancelQueryRequest(INVALID_RTS_HANDLE, getHeap());

  *this >> *request;
  setHandle(request->getHandle());

  Int32 minimumAge = request->getMinimumAge();
  bool cancelByPid = request->getCancelByPid();
  char queryId[200];
  Lng32 queryIdLen = 0;
  bool didAttemptCancel = false;
  bool haveAQid = false;

  if (cancelByPid)
  {
    haveAQid = ssmpGlobals_->getQidFromPid(request->getCancelPid(),
      minimumAge, queryId, queryIdLen);
  }
  else
  {
    ex_assert(moreObjects(),
              "expected an RTS_QUERY_ID following a CancelQuery");

    RtsMessageObjType objType =
      (RtsMessageObjType) getNextObjType();

    ex_assert(RTS_QUERY_ID == objType,
              "something besides an RTS_QUERY_ID followed CancelQuery");

    RtsQueryId *msgQueryId = new (getHeap()) RtsQueryId(getHeap());

    // Get the query Id from IPC
    *this >> *msgQueryId;
    ex_assert(msgQueryId->getQueryIdLen() <= sizeof(queryId),
              "query id received is too long");
    queryIdLen = msgQueryId->getQueryIdLen();
    str_cpy_all(queryId, msgQueryId->getQid(), queryIdLen);
    msgQueryId->decrRefCount();
    haveAQid = true;
  }
  clearAllObjects();
  ComDiagsArea *diags = NULL;

  if (haveAQid)
    didAttemptCancel = getSsmpGlobals()->cancelQueryTree(
                                     queryId, queryIdLen, request,
                                     &diags);

  // Now, make a reply to the Cancel Query message.
  RtsHandle rtsHandle = (RtsHandle) this;
  ControlQueryReply *cqReply = new (getHeap())
                                ControlQueryReply(rtsHandle, getHeap(),
                                didAttemptCancel || cancelByPid);

  *this << *cqReply;

  if (!(didAttemptCancel ||cancelByPid))
  {
    if (diags == NULL)
    {
       diags = ComDiagsArea::allocate(getHeap());
       *diags << DgSqlCode(-EXE_CANCEL_QID_NOT_FOUND);
    }
    *this << *diags;
  }
  send(FALSE);

  cqReply->decrRefCount();
  if (diags)
    diags->decrRefCount();
  request->decrRefCount();
}

void SsmpNewIncomingConnectionStream::actOnSuspendQueryReq(
                                                     IpcConnection *connection)
{
  ex_assert(getNextObjVersion()
            <= CurrSuspendQueryReplyVersionNumber, "Up-rev message received.");

  SuspendQueryRequest *request = new (getHeap())
    SuspendQueryRequest(INVALID_RTS_HANDLE, getHeap());

  *this >> *request;
  setHandle(request->getHandle());

  if (moreObjects())
  {
    RtsMessageObjType objType =
      (RtsMessageObjType) getNextObjType();

    switch (objType)
    {
    case RTS_QUERY_ID:
      {
        bool doAttemptSuspend = true;
        ComDiagsArea *diags = NULL;

        RtsQueryId *queryId = new (getHeap())
          RtsQueryId(getHeap());
        // Get the query Id from IPC
        *this >> *queryId;
        clearAllObjects();
        char *qid = queryId->getQid();
        Lng32 qidLen = queryId->getQueryIdLen();

        // Find the query.
        StatsGlobals *statsGlobals = ssmpGlobals_->getStatsGlobals();

        int error = statsGlobals->getStatsSemaphore(
                    ssmpGlobals_->getSemId(), ssmpGlobals_->myPin());

        SyncHashQueue *stmtStatsList = statsGlobals->getStmtStatsList();
        stmtStatsList->position(qid, qidLen);

        StmtStats *kqStmtStats = NULL;

        while (NULL != (kqStmtStats = (StmtStats *)stmtStatsList->getNext()))
        {
          if (str_cmp(kqStmtStats->getQueryId(), qid, qidLen) == 0)
          {
            // Can control only queries which have an ExMasterStats.
            if (NULL != kqStmtStats->getMasterStats())
              break;
          }
        }

        ExMasterStats *masterStats = NULL;
        if (kqStmtStats)
          masterStats = kqStmtStats->getMasterStats();

        if (masterStats)
        {
          if(!masterStats->isReadyToSuspend())
          {
            doAttemptSuspend = false;
            diags = ComDiagsArea::allocate(getHeap());
            *diags << DgSqlCode(-EXE_SUSPEND_QID_NOT_ACTIVE);
          }
          else if (!request->getIsForced())
          {
            // See if safe to suspend.
            if (masterStats &&
                 masterStats->getSuspendMayHaveAuditPinned())
            {
              doAttemptSuspend = false;
              diags = ComDiagsArea::allocate(getHeap());
              *diags << DgSqlCode(-EXE_SUSPEND_AUDIT);
            }
            else if (masterStats &&
                      masterStats->getSuspendMayHoldLock())
            {
              doAttemptSuspend = false;
              diags = ComDiagsArea::allocate(getHeap());
              *diags << DgSqlCode(-EXE_SUSPEND_LOCKS);
            }
          }

          if (doAttemptSuspend && masterStats->isQuerySuspended())
          {
            doAttemptSuspend = false;
            diags = ComDiagsArea::allocate(getHeap());
            *diags << DgSqlCode(-EXE_SUSPEND_ALREADY_SUSPENDED);
          }
          if (doAttemptSuspend)
          {
            // sanity checking here - may be better for MXSSMP to fail now
            // than for MXSSCPs to fail later.
            ExStatisticsArea *statsArea = kqStmtStats->getStatsArea();
            ex_assert(statsArea,
                      "Eligible subject query has no ExStatisticsArea");
            ExOperStats *rootStats = statsArea->getRootStats();
            ex_assert(rootStats,
                      "Eligible subject query has no root ExOperStats");
            ex_assert((rootStats->statType() == ExOperStats::ROOT_OPER_STATS) ||
                      (rootStats->statType() == ExOperStats::MEAS_STATS),
                      "Eligible subject query does not have correct stats.");

            ssmpGlobals_->suspendOrActivate(qid, qidLen, SUSPEND,
                              request->getSuspendLogging());
            masterStats->setQuerySuspended(true);
          }
        }
        else
        {
          doAttemptSuspend = false;
          diags = ComDiagsArea::allocate(getHeap());
          *diags << DgSqlCode(-EXE_RTS_QID_NOT_FOUND)
                 << DgString0(queryId->getQid());
        }

        statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(),
                                            ssmpGlobals_->myPin());

        if (doAttemptSuspend && request->getSuspendLogging())
        {
          char msg[80 + // the constant text
                   ComSqlId::MAX_QUERY_ID_LEN];

          str_sprintf(msg,
               "MXSSMP has processed a request to suspend query %s.",
                qid);

          SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg, 0);
        }

        queryId->decrRefCount();

        // Now, make a reply to the Suspend Query message.
        RtsHandle rtsHandle = (RtsHandle) this;
        ControlQueryReply *cqReply = new (getHeap())
                                      ControlQueryReply(rtsHandle, getHeap(),
                                      doAttemptSuspend);

        *this << *cqReply;
        cqReply->decrRefCount();

        if (diags)
        {
          *this << *diags;
          diags->decrRefCount();
        }

        send(FALSE);
      }
      break;
    default:
      ex_assert(0,
        "something besides an RTS_QUERY_ID followed SuspendQueryRequest");
      break;
    }
  }
  else
    ex_assert(0,
      "expected an RTS_QUERY_ID following a SuspendQueryRequest");

  request->decrRefCount();
}

void SsmpNewIncomingConnectionStream::actOnActivateQueryReq(
                                                     IpcConnection *connection)
{
  ex_assert(getNextObjVersion()
            <= CurrSuspendQueryReplyVersionNumber, "Up-rev message received.");

  ActivateQueryRequest *request = new (getHeap())
    ActivateQueryRequest(INVALID_RTS_HANDLE, getHeap());

  *this >> *request;
  setHandle(request->getHandle());
  bool suspendLogging = request->getSuspendLogging();
  request->decrRefCount();

  if (moreObjects())
  {
    RtsMessageObjType objType =
      (RtsMessageObjType) getNextObjType();

    switch (objType)
    {
    case RTS_QUERY_ID:
      {
        ComDiagsArea *diags = NULL;

        RtsQueryId *queryId = new (getHeap())
          RtsQueryId(getHeap());
        // Get the query Id from IPC
        *this >> *queryId;
        clearAllObjects();
        char *qid = queryId->getQid();
        Lng32 qidLen = queryId->getQueryIdLen();

        bool didAttemptActivate =
          ssmpGlobals_->activateFromQid(qid, qidLen, ACTIVATE,
                                        diags, suspendLogging);

        queryId->decrRefCount();

        // Now, make a reply to the Activate Query message.
        RtsHandle rtsHandle = (RtsHandle) this;
        ControlQueryReply *cqReply = new (getHeap())
                                      ControlQueryReply(rtsHandle, getHeap(),
                                      didAttemptActivate);

        *this << *cqReply;
        cqReply->decrRefCount();

        if (diags)
        {
          *this << *diags;
          diags->decrRefCount();
        }

        send(FALSE);
      }
      break;
    default:
      ex_assert(0,
        "something besides an RTS_QUERY_ID followed SuspendQueryRequest");
      break;
    }
  }
  else
    ex_assert(0,
      "expected an RTS_QUERY_ID following a SuspendQueryRequest");

}
void SsmpNewIncomingConnectionStream::actOnLobLockReq(
                                               IpcConnection *connection)
{
  IpcMessageObjVersion msgVer = getNextObjVersion();
  StatsGlobals *statsGlobals;
  NABoolean releasingLock = FALSE;
  CliGlobals *cliGlobals = GetCliGlobals();
  ex_assert(msgVer <= CurrLobLockVersionNumber,
            "Up-rev message received.");
  LobLockRequest *llReq= new (getHeap()) LobLockRequest(getHeap());
  *this >> *llReq;
  setHandle(llReq->getHandle());
  ex_assert(!moreObjects(),"Unexpected objects following LobLockRequest");
  clearAllObjects();
  //check and set the lock in the local shared segment 
  statsGlobals = ssmpGlobals_->getStatsGlobals();
  char *inLobLockId = NULL;
  inLobLockId = llReq->getLobLockId();
  if (inLobLockId[0] == '-')   //we are releasing this lock. No need to check.
    inLobLockId = NULL;
  else
    {
      inLobLockId = &inLobLockId[1];
      statsGlobals->checkLobLock(cliGlobals, inLobLockId);
    }
    
  if (inLobLockId)
    {
      //It's already set, don't propagate
      if (sscpDiagsArea_== NULL)
        sscpDiagsArea_ = ComDiagsArea::allocate(ssmpGlobals_->getHeap());
      *sscpDiagsArea_<< DgSqlCode(-EXE_LOB_CONCURRENT_ACCESS_ERROR);
      RmsGenericReply *reply = new(getHeap())
        RmsGenericReply(getHeap());

      *this << *reply;
      *this << *sscpDiagsArea_;
      this->clearSscpDiagsArea();
      send(FALSE);
      reply->decrRefCount();
    }
  else
    {
                             
      // Forward request to all mxsscps.
      ssmpGlobals_->allocateServers();
      SscpClientMsgStream *sscpMsgStream = new (heap_)
        SscpClientMsgStream(heap_, getIpcEnv(), ssmpGlobals_, this);
      sscpMsgStream->setUsedToSendLLMsgs();
      ssmpGlobals_->addRecipients(sscpMsgStream);
      sscpMsgStream->clearAllObjects();
      *sscpMsgStream << *llReq;
      llReq->decrRefCount();
      sscpMsgStream->send(FALSE);
    }
  // Reply to client when the msgs to mxsscp have all completed.  The reply
  // is made from the sscpMsgStream's callback.

}
void SsmpNewIncomingConnectionStream::actOnSecInvalidKeyReq(
                                               IpcConnection *connection)
{
  IpcMessageObjVersion msgVer = getNextObjVersion();
  ex_assert(msgVer <= CurrSecurityInvalidKeyVersionNumber,
            "Up-rev message received.");

  SecInvalidKeyRequest *sikReq = new (getHeap())
                      SecInvalidKeyRequest(getHeap());
  *this >> *sikReq;
  setHandle(sikReq->getHandle());
  ex_assert( !moreObjects(),
            "Unexpected objects following SecInvalidKeyRequest");
  clearAllObjects();

  // Forward request to all mxsscps.
  ssmpGlobals_->allocateServers();
  SscpClientMsgStream *sscpMsgStream = new (heap_)
        SscpClientMsgStream(heap_, getIpcEnv(), ssmpGlobals_, this);
  sscpMsgStream->setUsedToSendSikMsgs();
  ssmpGlobals_->addRecipients(sscpMsgStream);
  sscpMsgStream->clearAllObjects();
  *sscpMsgStream << *sikReq;
  sikReq->decrRefCount();
  sscpMsgStream->send(FALSE);

  // Reply to client when the msgs to mxsscp have all completed.  The reply
  // is made from the sscpMsgStream's callback.
}

void SsmpNewIncomingConnectionStream::actOnStatsReq(IpcConnection *connection)
{
  IpcMessageObjVersion msgVer;
  StatsGlobals *statsGlobals;
  int error;
  char *qid;
  pid_t pid = 0;
  short cpu;
  Int64 timeStamp;
  Lng32 queryNumber;
  RtsQueryId *rtsQueryId = NULL;
  StmtStats *stmtStats = NULL;

  msgVer = getNextObjVersion();

  ex_assert(msgVer <= currRtsStatsReqVersionNumber, "Up-rev message received.");

  RtsStatsReq *request = new (getHeap())
      RtsStatsReq(INVALID_RTS_HANDLE, getHeap());

  *this >> *request;
  setHandle(request->getHandle());
  setWmsProcess(request->getWmsProcess());
  if (moreObjects())
  {
    RtsMessageObjType objType =
      (RtsMessageObjType) getNextObjType();

    switch (objType)
    {
    case RTS_QUERY_ID:
      {
        RtsQueryId *queryId = new (getHeap())
          RtsQueryId(getHeap());
        // Get the query Id from IPC
        *this >> *queryId;
        clearAllObjects();
        statsGlobals = ssmpGlobals_->getStatsGlobals();
        short reqType = queryId->getStatsReqType();
        short subReqType = queryId->getSubReqType();
        switch (reqType)
        {
          case SQLCLI_STATS_REQ_QID:
            qid = queryId->getQid();
            error = statsGlobals->getStatsSemaphore(ssmpGlobals_->getSemId(),
                    ssmpGlobals_->myPin());
            stmtStats =
              statsGlobals->getMasterStmtStats(qid, str_len(qid), queryId->getActiveQueryNum());
            if (stmtStats != NULL)
              stmtStats->setStmtStatsUsed(TRUE);
            statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(), ssmpGlobals_->myPin());
            if (stmtStats != NULL)
            {
              getMergedStats(request, queryId, stmtStats,
                       reqType, queryId->getStatsMergeType());
            }
            break;
          case SQLCLI_STATS_REQ_QID_DETAIL:
            qid = queryId->getQid();
            error = statsGlobals->getStatsSemaphore(ssmpGlobals_->getSemId(), 
                        ssmpGlobals_->myPin());
            stmtStats = 
              statsGlobals->getMasterStmtStats(qid, str_len(qid), 1);
            if (stmtStats != NULL)
              stmtStats->setStmtStatsUsed(TRUE);
            statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(), ssmpGlobals_->myPin());
            if (stmtStats != NULL)
            {
              getMergedStats(request, queryId, stmtStats,
                      reqType, queryId->getStatsMergeType());
            }
            break;
          case SQLCLI_STATS_REQ_PROCESS_INFO:
            pid = queryId->getPid();
            getProcessStats(reqType, subReqType, pid);
            break;
          case SQLCLI_STATS_REQ_PID:
            error = statsGlobals->getStatsSemaphore(ssmpGlobals_->getSemId(),
                    ssmpGlobals_->myPin());
            stmtStats = statsGlobals->getStmtStats(pid, queryId->getActiveQueryNum());
            if (stmtStats != NULL)
              stmtStats->setStmtStatsUsed(TRUE);
            statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(), ssmpGlobals_->myPin());
            if (stmtStats != NULL)
            {
              if (stmtStats->isMaster())
              {
                getMergedStats(request, NULL, stmtStats,
                    reqType, queryId->getStatsMergeType());
              }
              else
              {
                RtsStatsReply *reply = new (getHeap())
                RtsStatsReply(request->getHandle(), getHeap());
                clearAllObjects();
                setType(IPC_MSG_SSMP_REPLY);
                setVersion(CurrSsmpReplyMessageVersion);
                *this << *reply;
                rtsQueryId = new (getHeap()) RtsQueryId(getHeap(),
                    stmtStats->getQueryId(), stmtStats->getQueryIdLen(),
                    // We need to use ANY_QUERY_ otherwise you might get QUERY_ID not found
                    // if the query gets finished before stats is merged
                    (UInt16)SQLCLI_SAME_STATS, RtsQueryId::ANY_QUERY_);

                *this << *(rtsQueryId);
                send(FALSE);
                stmtStats->setStmtStatsUsed(FALSE);
                reply->decrRefCount();
                rtsQueryId->decrRefCount();
              }
            }
            break;
          case SQLCLI_STATS_REQ_QID_INTERNAL:
            cpu = queryId->getCpu();
            pid = queryId->getPid();
            timeStamp = queryId->getTimeStamp();
            queryNumber = queryId->getQueryNumber();
            error = statsGlobals->getStatsSemaphore(ssmpGlobals_->getSemId(),
                    ssmpGlobals_->myPin());
            stmtStats = statsGlobals->getStmtStats(cpu, pid, timeStamp, queryNumber);
            if (stmtStats != NULL)
              stmtStats->setStmtStatsUsed(TRUE);
            statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(), ssmpGlobals_->myPin());
            if (stmtStats != NULL)
            {
                ex_assert(stmtStats->isMaster() == TRUE, "Should be Master here");
                getMergedStats(request, NULL, stmtStats,
                    reqType, queryId->getStatsMergeType());
            }
            break;
          case SQLCLI_STATS_REQ_CPU:
            error = statsGlobals->getStatsSemaphore(ssmpGlobals_->getSemId(), ssmpGlobals_->myPin());
            stmtStats = statsGlobals->getStmtStats(queryId->getActiveQueryNum());
            if (stmtStats != NULL)
              stmtStats->setStmtStatsUsed(TRUE);
            statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(), ssmpGlobals_->myPin());
            if (stmtStats != NULL)
            {
              ex_assert(stmtStats->isMaster() == TRUE, "Should be Master here");
              getMergedStats(request, NULL, stmtStats,
                reqType, queryId->getStatsMergeType());
            }
            break;

          default:
            break;
        }
        request->decrRefCount();
        queryId->decrRefCount();
        // If there is no stmt stats, reply back with empty stats
        if (stmtStats == NULL && reqType != SQLCLI_STATS_REQ_PROCESS_INFO)
          sendMergedStats(NULL, 0, reqType, NULL, FALSE);
      }
      break;
    default:
      break;
    }
  }
}

void SsmpNewIncomingConnectionStream::getProcessStats(short reqType,
                                            short subReqType,
                                            pid_t pid)
{
  ExStatisticsArea *mergedStats=NULL;
  ExProcessStats *exProcessStats=NULL;
  ExProcessStats *tmpExProcessStats=NULL;
  StatsGlobals *statsGlobals = ssmpGlobals_->getStatsGlobals();

  exProcessStats = statsGlobals->getExProcessStats(pid);
  if (exProcessStats != NULL)
  {
     mergedStats = new (getHeap()) ExStatisticsArea(getHeap(),
                                          0,
                                          ComTdb::OPERATOR_STATS,
                                          ComTdb::OPERATOR_STATS);
     mergedStats->setStatsEnabled(TRUE);
     mergedStats->setSubReqType(subReqType);
     tmpExProcessStats = new (mergedStats->getHeap())
                                ExProcessStats(mergedStats->getHeap());
     tmpExProcessStats->copyContents(exProcessStats);
     mergedStats->insert(tmpExProcessStats);
  }
  sendMergedStats(mergedStats, 0, reqType, NULL, FALSE);
}



void SsmpNewIncomingConnectionStream::getMergedStats(RtsStatsReq *request,
                                                       RtsQueryId *queryId,
                                                       StmtStats *stmtStats,
                                                       short reqType,
                                                       UInt16 statsMergeType)
{
  Int64 currTime = NA_JulianTimestamp();
  RtsQueryId *rtsQueryId = NULL;
  ExStatisticsArea *mergedStats;
  ExStatisticsArea *srcMergedStats;

  // MergedStats_ in stmtStats can't be used directly in Ipc layer
  // since the stats->entries_ will be traversed at the time of send
  // This can cause corruption since hashQueue is not protected using semaphore
  // So, use local stats area instead when the statsArea is returned within the
  // RTS_MERGE_INTERVAL_TIMEOUT from SSMP itself. When the statsArea is merged from SSCPs
  // set the mergedStats in stmtStats_ after it has been shipped out, to avoid other processes
  // accessing the mergedStats_ while it is being traversed in Ipc layer
  if (reqType == SQLCLI_STATS_REQ_QID && (ssmpGlobals_->getForceMerge() == FALSE)
      && (ssmpGlobals_->getNumDeallocatedServers() == 0)
      && (stmtStats->mergeReqd() == FALSE)
      && ((currTime - stmtStats->getLastMergedTime()) <
        (ssmpGlobals_->getStatsCollectionInterval() * 1000000))
        && isWmsProcess())
  {
    if ((srcMergedStats = stmtStats->getMergedStats()) != NULL)
    {
      ComTdb::CollectStatsType tempStatsMergeType =
        (statsMergeType == SQLCLI_SAME_STATS ?
                                    srcMergedStats->getOrigCollectStatsType() :
                                    (ComTdb::CollectStatsType)statsMergeType);
      if (srcMergedStats->getCollectStatsType() == tempStatsMergeType)
      {
        mergedStats = new (getHeap()) ExStatisticsArea(getHeap(),
                                          0,
                                          tempStatsMergeType,
                                          srcMergedStats->getOrigCollectStatsType());
        mergedStats->setStatsEnabled(TRUE);
        StatsGlobals *statsGlobals = ssmpGlobals_->getStatsGlobals();
        int error = statsGlobals->getStatsSemaphore(ssmpGlobals_->getSemId(),
                    ssmpGlobals_->myPin());
        mergedStats->merge(srcMergedStats, statsMergeType);
        statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(),
                    ssmpGlobals_->myPin());
        sendMergedStats(mergedStats, 0, reqType, stmtStats, FALSE);
        return;
      }
    }
  }
  ssmpGlobals_->allocateServers();
  // Give the received message to SscpClientMsgStream
  SscpClientMsgStream *sscpMsgStream = new (heap_) SscpClientMsgStream(heap_, getIpcEnv(), ssmpGlobals_, this);
  ssmpGlobals_->addRecipients(sscpMsgStream);
  sscpMsgStream->clearAllObjects();
  *sscpMsgStream << *request;
  // If the incoming RtsQueryId (queryId) is null, construct the
  // RtsQueryId from stmtStats
  if (queryId == NULL)
  {
    rtsQueryId = new (getHeap()) RtsQueryId(getHeap(), stmtStats->getQueryId(),
        stmtStats->getQueryIdLen(), statsMergeType,
        RtsQueryId::ANY_QUERY_);
    *sscpMsgStream << *rtsQueryId;
    sscpMsgStream->setReqType(rtsQueryId->getStatsReqType());
    sscpMsgStream->setDetailLevel(rtsQueryId->getDetailLevel());
  }
  else
  {
    *sscpMsgStream << *queryId ;
    sscpMsgStream->setReqType(queryId->getStatsReqType());
    sscpMsgStream->setDetailLevel(queryId->getDetailLevel());
  }
  sscpMsgStream->setStmtStats(stmtStats);
  sscpMsgStream->setMergeStartTime(NA_JulianTimestamp());
  // Send the Message to all
  ssmpGlobals_->addPendingSscpMessage(sscpMsgStream);
  sscpMsgStream->send(FALSE);
  if (rtsQueryId != NULL)
    rtsQueryId->decrRefCount();
  return;
}

void SsmpNewIncomingConnectionStream::actOnCpuStatsReq(IpcConnection *connection)
{
  IpcMessageObjVersion msgVer;
  StmtStats *stmtStats;
  StatsGlobals *statsGlobals;
  ExStatisticsArea *stats;
  ExStatisticsArea *cpuStats = NULL;
  ExMasterStats *masterStats;

  short currQueryNum = 0;
  msgVer = getNextObjVersion();

  if (msgVer > currRtsCpuStatsReqVersionNumber)
    // Send Error
    ;

  RtsCpuStatsReq *request = new (getHeap())
      RtsCpuStatsReq(INVALID_RTS_HANDLE, getHeap());

  *this >> *request;
  setHandle(request->getHandle());
  short reqType = request->getReqType();
  short noOfQueries = request->getNoOfQueries();
  Lng32 filter = request->getFilter();
  short subReqType = request->getSubReqType();
  clearAllObjects();
  if (request->getCpu() != -1)
  {
    setType(IPC_MSG_SSMP_REPLY);
    setVersion(CurrSsmpReplyMessageVersion);
    RtsStatsReply *reply = new (getHeap())
                  RtsStatsReply(request->getHandle(), getHeap());
    *this << *reply;
    statsGlobals = ssmpGlobals_->getStatsGlobals();
    switch (reqType)
    {
    case SQLCLI_STATS_REQ_RMS_INFO:
     {
      cpuStats = new (getHeap())
        ExStatisticsArea(getHeap(), 0, ComTdb::RMS_INFO_STATS, ComTdb::RMS_INFO_STATS);
      cpuStats->setSubReqType(subReqType);
      cpuStats->setStatsEnabled(TRUE);
      ExRMSStats *rmsStats = new (getHeap()) ExRMSStats(getHeap());
      rmsStats->copyContents(statsGlobals->getRMSStats());
      NAHeap *statsHeap = statsGlobals->getStatsHeap();
      rmsStats->setGlobalStatsHeapAlloc(statsHeap->getTotalSize());
      rmsStats->setGlobalStatsHeapUsed(statsHeap->getAllocSize());
      rmsStats->setStatsHeapWaterMark(statsHeap->getHighWaterMark());
      rmsStats->setNoOfStmtStats(statsGlobals->getStmtStatsList()->numEntries());
      rmsStats->setSemPid(statsGlobals->getSemPid());
      rmsStats->setSscpOpens(ssmpGlobals_->getNumAllocatedServers());
      rmsStats->setSscpDeletedOpens(ssmpGlobals_->getNumDeallocatedServers());
      rmsStats->setNumQueryInvKeys(statsGlobals->getRecentSikeys()->entries());
      cpuStats->insert(rmsStats);
      if (request->getNoOfQueries() == RtsCpuStatsReq::INIT_RMS_STATS_)
        statsGlobals->getRMSStats()->reset();
      break;
     }
   case SQLCLI_STATS_REQ_ET_OFFENDER:
     {
       cpuStats = new (getHeap())
           ExStatisticsArea(getHeap(), 0, ComTdb::ET_OFFENDER_STATS,
                     ComTdb::ET_OFFENDER_STATS);
       cpuStats->setStatsEnabled(TRUE);
       int error = statsGlobals->getStatsSemaphore(ssmpGlobals_->getSemId(),
            ssmpGlobals_->myPin());
       SyncHashQueue *stmtStatsList = statsGlobals->getStmtStatsList();
       stmtStatsList->position();
       Int64 currTimestamp = NA_JulianTimestamp();
       while ((stmtStats = (StmtStats *)stmtStatsList->getNext()) != NULL)
       {
          masterStats = stmtStats->getMasterStats();
          if (masterStats != NULL)
             cpuStats->appendCpuStats(masterStats, FALSE,
                  subReqType, filter, currTimestamp);
       }
       statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(),
          ssmpGlobals_->myPin());
       break;

     }
    case SQLCLI_STATS_REQ_MEM_OFFENDER:
     {
       cpuStats = new (getHeap())
           ExStatisticsArea(getHeap(), 0, ComTdb::MEM_OFFENDER_STATS,
                     ComTdb::MEM_OFFENDER_STATS);
       cpuStats->setStatsEnabled(TRUE);
       cpuStats->setSubReqType(subReqType);
       statsGlobals->getMemOffender(cpuStats, filter);
     }
     break;
    case SQLCLI_STATS_REQ_CPU_OFFENDER:
     {
      short noOfQueries = request->getNoOfQueries();
      int error = statsGlobals->getStatsSemaphore(ssmpGlobals_->getSemId(),
                        ssmpGlobals_->myPin());
      SyncHashQueue * stmtStatsList = statsGlobals->getStmtStatsList();
      stmtStatsList->position();
     switch (noOfQueries)
      {
       case RtsCpuStatsReq::INIT_CPU_STATS_HISTORY_:
        while ((stmtStats = (StmtStats *)stmtStatsList->getNext()) != NULL)
        {
          stats = stmtStats->getStatsArea();
          if (stats != NULL)
            stats->setCpuStatsHistory();
        }
        break;
       default:
        if ( noOfQueries == RtsCpuStatsReq::ALL_ACTIVE_QUERIES_)
           noOfQueries = 32767;
        cpuStats = new (getHeap())
             ExStatisticsArea(getHeap(), 0, ComTdb::CPU_OFFENDER_STATS,
                    ComTdb::CPU_OFFENDER_STATS);
        cpuStats->setStatsEnabled(TRUE);
       while ((stmtStats = (StmtStats *)stmtStatsList->getNext()) != NULL
                 && currQueryNum <= noOfQueries)
        {
          stats = stmtStats->getStatsArea();
          if (stats != NULL)
          {
              if (cpuStats->appendCpuStats(stats))
                 currQueryNum++;
          }
        }
        break;
      }
      statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(),
          ssmpGlobals_->myPin());
      break;
     }
    default:
      break;
    }
    if (cpuStats != NULL && cpuStats->numEntries() > 0)
       *this << *(cpuStats);
    send(FALSE);
    reply->decrRefCount();
    if (cpuStats != NULL)
       NADELETE(cpuStats, ExStatisticsArea, cpuStats->getHeap());
  }
  else
  {
    ssmpGlobals_->allocateServers();
    // Give the received message to SscpClientMsgStream
    SscpClientMsgStream *sscpMsgStream = new (heap_) SscpClientMsgStream(heap_, getIpcEnv(), ssmpGlobals_, this);
    ssmpGlobals_->addRecipients(sscpMsgStream);
    sscpMsgStream->clearAllObjects();
    *sscpMsgStream << *request;
    sscpMsgStream->setMergeStartTime(NA_JulianTimestamp());
    sscpMsgStream->setReqType(request->getReqType());
    sscpMsgStream->setSubReqType(request->getSubReqType());
    ssmpGlobals_->addPendingSscpMessage(sscpMsgStream);
    sscpMsgStream->send(FALSE);
  }
  request->decrRefCount();
}

void SsmpNewIncomingConnectionStream::actOnExplainReq(IpcConnection *connection)
{
  IpcMessageObjVersion msgVer;
  StmtStats *stmtStats;
  StatsGlobals *statsGlobals;
  RtsExplainFrag *explainFrag = NULL;
  RtsExplainFrag *srcExplainFrag;

  short currQueryNum = 0;
  msgVer = getNextObjVersion();

  if (msgVer > currRtsExplainReqVersionNumber)
    // Send Error
    ;

  RtsExplainReq *request = new (getHeap())
      RtsExplainReq(INVALID_RTS_HANDLE, getHeap());

  *this >> *request;
  setHandle(request->getHandle());
  clearAllObjects();
  setType(RTS_MSG_EXPLAIN_REPLY);
  setVersion(currRtsExplainReplyVersionNumber);
  RtsExplainReply *reply = new (getHeap())
                RtsExplainReply(request->getHandle(), getHeap());
  *this << *reply;
  statsGlobals = ssmpGlobals_->getStatsGlobals();
  int error = statsGlobals->getStatsSemaphore(ssmpGlobals_->getSemId(),
                    ssmpGlobals_->myPin());
  stmtStats = statsGlobals->getMasterStmtStats(request->getQid(), request->getQidLen(),
              RtsQueryId::ANY_QUERY_);
  if (stmtStats != NULL)
  {
    srcExplainFrag = stmtStats->getExplainInfo();
    if (srcExplainFrag != NULL)
      explainFrag = new (getHeap()) RtsExplainFrag(getHeap(), srcExplainFrag);
  }
  statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(), ssmpGlobals_->myPin());
  if (explainFrag)
    *this << *(explainFrag);
  send(FALSE);
  reply->decrRefCount();
  request->decrRefCount();
  if (explainFrag)
    explainFrag->decrRefCount();
}

void SsmpNewIncomingConnectionStream::sscpIpcError(IpcConnection *conn)
{
  if (sscpDiagsArea_ == NULL)
    sscpDiagsArea_ = ComDiagsArea::allocate(ssmpGlobals_->getHeap());

  conn->populateDiagsArea(sscpDiagsArea_, ssmpGlobals_->getHeap());
}


void SscpClientMsgStream::actOnStatsReply(IpcConnection *connection)
{

  IpcMessageObjVersion msgVer;
  msgVer = getNextObjVersion();
  int error;

  if (msgVer > currRtsStatsReplyVersionNumber)
    // Send Error
    ;
  RtsStatsReply *reply = new (getHeap())
      RtsStatsReply(INVALID_RTS_HANDLE, getHeap());

  *this >> *reply;
  incNumSqlProcs(reply->getNumSqlProcs());
  incNumCpus(reply->getNumCpus());

  while (moreObjects())
  {
    RtsMessageObjType objType =
      (RtsMessageObjType) getNextObjType();
    short reqType = getReqType();
    switch (objType)
    {
    case IPC_SQL_STATS_AREA:
      {
        ExStatisticsArea *stats = new (getHeap())
          ExStatisticsArea(getHeap());

        // Get the query Id from IPC
        *this >> *stats;
        switch (reqType)
        {
          case SQLCLI_STATS_REQ_QID:
          {
            StatsGlobals *statsGlobals = ssmpGlobals_->getStatsGlobals();
            error = statsGlobals->getStatsSemaphore(ssmpGlobals_->getSemId(),
                ssmpGlobals_->myPin());
            if (mergedStats_ == NULL)
            {
              if (isReplySent())
                mergedStats_ = new (getHeap())
                  ExStatisticsArea(getHeap(), 0, stats->getCollectStatsType(),
                                  stats->getOrigCollectStatsType());
              else
                mergedStats_ = new (getSsmpGlobals()->getStatsHeap())
                  ExStatisticsArea(getSsmpGlobals()->getStatsHeap(), 0, stats->getCollectStatsType(),
                                  stats->getOrigCollectStatsType());
                // Can we always assume that the stats is enabled ?????
                // The stats should be enabled for the DISPLAY STATISTICS or SELECT ...TABLE(STATSISTICS...)
                // to display something
              mergedStats_->setStatsEnabled(TRUE);
            }
            mergedStats_->merge(stats);
            statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(), ssmpGlobals_->myPin());
            break;
          }
          case SQLCLI_STATS_REQ_QID_DETAIL:
          case SQLCLI_STATS_REQ_CPU_OFFENDER:
          case SQLCLI_STATS_REQ_SE_OFFENDER:
          case SQLCLI_STATS_REQ_ET_OFFENDER:
          case SQLCLI_STATS_REQ_MEM_OFFENDER:
          case SQLCLI_STATS_REQ_RMS_INFO:
          {
            if (mergedStats_ == NULL)
            {
              mergedStats_ = new (getHeap())
                  ExStatisticsArea(getHeap(), 0, stats->getCollectStatsType(),
                                  stats->getOrigCollectStatsType());
              mergedStats_->setStatsEnabled(TRUE);
              mergedStats_->setSubReqType(subReqType_);
            }
            mergedStats_->setDetailLevel(getDetailLevel());
            mergedStats_->appendCpuStats(stats, TRUE);

            if (reqType == SQLCLI_STATS_REQ_QID_DETAIL && stats->getMasterStats() != NULL)
            {
              ExMasterStats *masterStats = new (getHeap()) ExMasterStats((NAHeap *)getHeap());
              masterStats->copyContents(stats->getMasterStats());
              mergedStats_->setMasterStats(masterStats);
            } 
            break;
          }
          default:
            break;
        }
        NADELETE(stats, ExStatisticsArea, getHeap());
      }
      break;
    default:
      break;
    }
  }
  reply->decrRefCount();
#ifdef _DEBUG_RTS
  cerr << "Ssmp Merged Stats " << mergedStats_ << " \n";
#endif
}

SscpClientMsgStream::~SscpClientMsgStream()
{
  if (mergedStats_ != NULL)
  {
    NADELETE(mergedStats_, ExStatisticsArea, mergedStats_->getHeap());
  }
}

// method called upon send complete

void SscpClientMsgStream::actOnSendAllComplete()
{
  // once all sends have completed, initiate a nowait receive from all
  // SSCPs that we sent this message to
  clearAllObjects();
  receive(FALSE);
}

void SscpClientMsgStream::actOnReceive(IpcConnection* connection)
{
  IpcMessageObjType replyType;
  if (connection->getErrorInfo() != 0)
  {
    if (ssmpStream_)
      ssmpStream_->sscpIpcError(connection);
    delinkConnection(connection);
    return;
  }
  else
  {
    numOfClientRequestsSent_--;
    // check for protocol errors
    ex_assert(getType() == IPC_MSG_SSCP_REPLY AND
	      getVersion() == CurrSscpReplyMessageVersion AND
	      moreObjects(),
	      "Invalid  message from client");
    // take a look at the type of the first object in the message
    replyType = getNextObjType();
    switch(replyType)
    {
    case RTS_MSG_STATS_REPLY:
      actOnStatsReply(connection);
      break;
    case CANCEL_QUERY_KILL_SERVERS_REPLY:
      {
      CancelQueryKillServersReply *reply = new (getHeap())
        CancelQueryKillServersReply(INVALID_RTS_HANDLE, getHeap());

      *this >> *reply;
      reply->decrRefCount();
      break;
      }
    case IPC_MSG_RMS_REPLY:
      {
      RmsGenericReply *reply = new (getHeap())
              RmsGenericReply(getHeap());

      *this >> *reply;
      reply->decrRefCount();
      break;
      }
    default:
      ex_assert(FALSE,"Invalid reply from client");
    }
  }
}

void SscpClientMsgStream::actOnReceiveAllComplete()
{
  // If we have received responses to all requests we sent out, we mark this stream as having
  // completed its work. The IPCEnv will call the destructor at a time when it is safe to do so.
  if (! isReplySent())
    {
      switch (completionProcessing_)
      {
        case STATS:
        {
          sendMergedStats();
          break;
        }
        case CB:
        {
          // Control broker - cancel/suspend/activate.  Nothing to do.
          break;
        }
        case SIK:
        {
          replySik();
          break;
        }
        case LL:
        {
          replyLL();
          break;
        }
        default:
        {
          ex_assert(FALSE, "Unknown completionProcessing_ flag.");
          break;
        }
      }
      ssmpGlobals_->removePendingSscpMessage(this);
    }

  addToCompletedList();
}

void SscpClientMsgStream::replySik()
{
  RmsGenericReply *reply = new(getHeap())
    RmsGenericReply(getHeap());

  *ssmpStream_ << *reply;

  if (ssmpStream_->getSscpDiagsArea())
  {
    // Pass errors from communication w/ SSCPs back to the
    // client.
    *ssmpStream_ << *(ssmpStream_->getSscpDiagsArea());
    ssmpStream_->clearSscpDiagsArea();
  }

  ssmpStream_->send(FALSE);
  reply->decrRefCount();
}

void SscpClientMsgStream::replyLL()
{
  RmsGenericReply *reply = new(getHeap())
    RmsGenericReply(getHeap());

  *ssmpStream_ << *reply;

  if (ssmpStream_->getSscpDiagsArea())
  {
    // Pass errors from communication w/ SSCPs back to the
    // client.
    *ssmpStream_ << *(ssmpStream_->getSscpDiagsArea());
    ssmpStream_->clearSscpDiagsArea();
  }

  ssmpStream_->send(FALSE);
  reply->decrRefCount();
}

void SscpClientMsgStream::sendMergedStats()
{
  StmtStats *stmtStats;
  ExStatisticsArea *mergedStats;
  int error;

  stmtStats = getStmtStats();
  short reqType = getReqType();
  if (mergedStats_ == NULL && reqType == SQLCLI_STATS_REQ_QID &&
    stmtStats != NULL && stmtStats->getMasterStats() != NULL)
  {
    StatsGlobals *statsGlobals = ssmpGlobals_->getStatsGlobals();
    error = statsGlobals->getStatsSemaphore(ssmpGlobals_->getSemId(),
      ssmpGlobals_->myPin());
    mergedStats_ = new (ssmpGlobals_->getStatsHeap())
          ExStatisticsArea(ssmpGlobals_->getStatsHeap(), 0,
                stmtStats->getMasterStats()->getCollectStatsType());
    // Can we always assume that the stats is enabled ?????
    // The stats should be enabled for the DISPLAY STATISTICS or SELECT ...TABLE(STATSISTICS...)
    // to display something
    mergedStats_->setStatsEnabled(TRUE);
    ExMasterStats *masterStats = new (ssmpGlobals_->getStatsHeap())
            ExMasterStats(ssmpGlobals_->getStatsHeap());
    masterStats->copyContents(stmtStats->getMasterStats());
    mergedStats_->setMasterStats(masterStats);
    statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(), ssmpGlobals_->myPin());
  }
  ExMasterStats *masterStats;
  if (mergedStats_ != NULL)
  {
    masterStats = mergedStats_->getMasterStats();
    if (masterStats != NULL)
    {
      masterStats->setNumCpus(getNumCpus());
    }
  }
  // Get a reference to mergedStats_ before setReplySent() sets it zero
  mergedStats = mergedStats_;
  setReplySent();
  ssmpStream_->sendMergedStats(mergedStats, getNumOfErrorRequests()+getNumOfClientRequestsPending()+
            getSsmpGlobals()->getNumDeallocatedServers(), reqType, stmtStats,
            (reqType == SQLCLI_STATS_REQ_QID));
}

void SscpClientMsgStream::delinkConnection(IpcConnection *conn)
{
  char nodeName[MAX_SEGMENT_NAME_LEN+1];
  IpcCpuNum cpu;

  numOfErrorRequests_++;
  numOfClientRequestsSent_--;
  conn->getOtherEnd().getNodeName().getNodeNameAsString(nodeName);
  cpu = conn->getOtherEnd().getCpuNum();
#ifdef _DEBUG
  cout << "delinking " << nodeName << " in CPU " << cpu << endl;
#endif
  ssmpGlobals_->deAllocateServer(nodeName, (short)str_len(nodeName), (short)cpu);
}

void SsmpNewIncomingConnectionStream::sendMergedStats(ExStatisticsArea *mergedStats, short numErrors,
                                             short reqType, StmtStats *stmtStats, NABoolean updateMergeStats)
{
  StatsGlobals *statsGlobals;
  int error;

  RtsStatsReply *reply = new (getHeap())
      RtsStatsReply(getHandle(), getHeap());
  reply->setNumSscpErrors(numErrors);
  setType(IPC_MSG_SSMP_REPLY);
  setVersion(CurrSsmpReplyMessageVersion);
  *this << *reply;
  switch (reqType)
  {
    case SQLCLI_STATS_REQ_QID:
      if (mergedStats != NULL)
        *this << *(mergedStats);
      send (FALSE);
      statsGlobals = ssmpGlobals_->getStatsGlobals();
      error = statsGlobals->getStatsSemaphore(ssmpGlobals_->getSemId(),
            ssmpGlobals_->myPin());
      if (stmtStats != NULL)
      {
        if (updateMergeStats)
          stmtStats->setMergedStats(mergedStats);
        stmtStats->setStmtStatsUsed(FALSE);
        if (isWmsProcess() && stmtStats->canbeGCed())
          statsGlobals->removeQuery(stmtStats->getPid(), stmtStats, TRUE);
      }
      statsGlobals->releaseStatsSemaphore(ssmpGlobals_->getSemId(), ssmpGlobals_->myPin());
      break;
    case SQLCLI_STATS_REQ_QID_DETAIL:
      if (mergedStats != NULL)
        *this << *(mergedStats);
      send (FALSE);
      if (stmtStats != NULL)
        stmtStats->setStmtStatsUsed(FALSE);
      break;
    case SQLCLI_STATS_REQ_CPU_OFFENDER:
    case SQLCLI_STATS_REQ_SE_OFFENDER:
    case SQLCLI_STATS_REQ_ET_OFFENDER:
    case SQLCLI_STATS_REQ_RMS_INFO:
    case SQLCLI_STATS_REQ_MEM_OFFENDER:
    case SQLCLI_STATS_REQ_PROCESS_INFO:
      if (mergedStats != NULL)
        *this << *(mergedStats);
      send (FALSE);
      break;
    case SQLCLI_STATS_REQ_PID:
    case SQLCLI_STATS_REQ_CPU:
    case SQLCLI_STATS_REQ_QID_INTERNAL:
      if (mergedStats != NULL &&
             mergedStats->getMasterStats() != NULL)
           *this << *(mergedStats);
      send (FALSE);
      if (stmtStats != NULL)
        stmtStats->setStmtStatsUsed(FALSE);
      break;
  }
  reply->decrRefCount();
  if (!(updateMergeStats) && mergedStats != NULL)
  {
    NADELETE(mergedStats, ExStatisticsArea, mergedStats->getHeap());
  }
}

void SsmpClientMsgStream::actOnSend(IpcConnection* connection)
{
  if (connection->getErrorInfo() != 0)
    stats_ = NULL;
}

void SsmpClientMsgStream::actOnSendAllComplete()
{
  clearAllObjects();
  receive(FALSE);
}

void SsmpClientMsgStream::actOnReceive(IpcConnection* connection)
{
  replyRecvd_ = TRUE;

  if (connection->getErrorInfo() != 0)
  {
    stats_ = NULL;
    if (diagsForClient_)
       connection->populateDiagsArea(diagsForClient_, getHeap());

    delinkConnection(connection);
    return;
  }
  // take a look at the type of the first object in the message
  switch(getNextObjType())
  {
  case RTS_MSG_STATS_REPLY:
    actOnStatsReply(connection);
    break;
  case RTS_MSG_EXPLAIN_REPLY:
    actOnExplainReply(connection);
    break;
  case IPC_MSG_RMS_REPLY:
    actOnGenericReply();
    break;
  default:
    ex_assert(FALSE,"Invalid reply from first client message");
  }
}

void SsmpClientMsgStream::actOnReceiveAllComplete()
{
  // We mark this stream as having completed its work. The IPCEnv will
  // call the destructor at a time when it is safe to do so.
  addToCompletedList();
}

void SsmpClientMsgStream::actOnStatsReply(IpcConnection* connection)
{
  IpcMessageObjVersion msgVer;
  msgVer = getNextObjVersion();

  if (msgVer > currRtsStatsReplyVersionNumber)
    // Send Error
    ;
  RtsStatsReply *reply = new (getHeap())
      RtsStatsReply(INVALID_RTS_HANDLE, getHeap());

  *this >> *reply;
  numSscpReqFailed_ = reply->getNumSscpErrors();
  if (moreObjects())
  {
    RtsMessageObjType objType =
    (RtsMessageObjType) getNextObjType();

    switch (objType)
    {
    case IPC_SQL_STATS_AREA:
      {
        stats_ = new (getHeap())
          ExStatisticsArea(getHeap());
        *this >> *stats_;
      }
      break;
    case RTS_QUERY_ID:
      {
         rtsQueryId_ = new (getHeap())
          RtsQueryId(getHeap());
         *this >> *rtsQueryId_;
      }
      break;
    default:
      break;
    }
  }
  else
    stats_ = NULL;
  reply->decrRefCount();
  // we don't want to decrement the stats, since we want to pass it on
}

void SsmpClientMsgStream::actOnExplainReply(IpcConnection* connection)
{
  IpcMessageObjVersion msgVer;
  msgVer = getNextObjVersion();

  if (msgVer > currRtsExplainReplyVersionNumber)
    // Send Error
    ;
  RtsExplainReply *reply = new (getHeap())
      RtsExplainReply(INVALID_RTS_HANDLE, getHeap());

  *this >> *reply;
  numSscpReqFailed_ = 0;
  if (moreObjects())
  {
    RtsMessageObjType objType =
    (RtsMessageObjType) getNextObjType();

    switch (objType)
    {
    case RTS_EXPLAIN_FRAG:
      {
        explainFrag_ = new (getHeap())
          RtsExplainFrag(getHeap());
        *this >> *explainFrag_;
      }
      break;
    default:
      break;
    }
  }
  else
    explainFrag_ = NULL;
  reply->decrRefCount();
}

void SsmpClientMsgStream::delinkConnection(IpcConnection *conn)
{
  char nodeName[MAX_SEGMENT_NAME_LEN+1];
  IpcCpuNum cpu;

  conn->getOtherEnd().getNodeName().getNodeNameAsString(nodeName);
  cpu = conn->getOtherEnd().getCpuNum();
  ssmpManager_->removeSsmpServer(nodeName, (short)cpu);
}

void SsmpClientMsgStream::actOnGenericReply()
{
  RmsGenericReply *reply = new (getHeap()) RmsGenericReply(getHeap());
  *this >> *reply;

  while (moreObjects())
  {
    IpcMessageObjType objType = getNextObjType();
    ex_assert(objType == IPC_SQL_DIAG_AREA,
              "Unknown object returned from mxssmp.");
    if (diagsForClient_)
      *this >> *diagsForClient_;
  }

  reply->decrRefCount();
}

