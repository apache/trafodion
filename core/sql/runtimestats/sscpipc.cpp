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
// File:         sscpipc.cpp
// Description:  Class declaration for SSCP IPC infrastructure
//
// Created:      5/02/2006
**********************************************************************/

#include "Platform.h"
#include "ex_stdh.h"
#include "sscpipc.h"
#include "logmxevent.h"
#include "ExCextdecs.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include "nsk/nskport.h"
#include "seabed/ms.h"
#include "seabed/fs.h"
#include "NAStdlib.h"
#include "zsysc.h"
#include "ExStats.h"
#include "rts_msg.h"
#include "PortProcessCalls.h"
#include "ComTdb.h"
#include "ComSqlId.h"
#include "ComDistribution.h"

SscpGlobals::SscpGlobals(NAHeap *sscpheap, StatsGlobals *statsGlobals)
  : heap_(sscpheap),
    statsGlobals_(statsGlobals)
  , doLogCancelKillServers_(false)
{
  int error;
  Int32 myCpu;
  char programDir[100];
  short processType;
  char myNodeName[MAX_SEGMENT_NAME_LEN+1];
  Lng32 myNodeNumber;
  short myNodeNameLen = MAX_SEGMENT_NAME_LEN;
  Int64 myStartTime;
  short pri;
  char myProcessNameString[PROCESSNAME_STRING_LEN];

  error = statsGlobals_->openStatsSemaphore(semId_);
  ex_assert(error == 0, "BINSEM_OPEN returned an error");


  error = ComRtGetProgramInfo(programDir, 100, processType,
    myCpu, myPin_,
    myNodeNumber, myNodeName, myNodeNameLen, myStartTime, myProcessNameString);
  ex_assert(error == 0,"Error in ComRtGetProgramInfo");

  pri = 0;
  error = statsGlobals_->getStatsSemaphore(semId_, myPin_);
  // ProcessHandle wrapper in porting layer library
  NAProcessHandle sscpPhandle;
  error = sscpPhandle.getmine(statsGlobals->getSscpProcHandle());

  statsGlobals_->setSscpPid(myPin_);
  statsGlobals_->setSscpPriority(pri);
  statsGlobals_->setSscpTimestamp(myStartTime);
  statsGlobals_->setSscpProcSemId(semId_);
  statsGlobals->setSscpInitialized(TRUE);
  NAHeap *statsHeap = (NAHeap *)statsGlobals_->getStatsHeap()->
        allocateHeapMemory(sizeof *statsHeap, FALSE);

  // The following assertion may be hit if the RTS shared memory
  // segment is full.  
  ex_assert(statsHeap, "allocateHeapMemory returned NULL.");

  // This next allocation, a placement "new" will not fail.
  statsHeap = new (statsHeap, statsGlobals_->getStatsHeap())
       NAHeap("Process Stats Heap", statsGlobals_->getStatsHeap(),
               8192,
               0);
  statsGlobals_->addProcess(myPin_, statsHeap);

  statsGlobals_->releaseStatsSemaphore(semId_, myPin_);
  CliGlobals *cliGlobals = GetCliGlobals();
  cliGlobals->setSemId(semId_);
  cliGlobals->setStatsHeap(statsHeap);
  char defineName[24+1];
  short zeroMeansNo;
  str_cpy_all (defineName, "=_MX_RTS_LOG_KILL_SERVER", 24);
  if (((error = getDefineNumericValue(defineName, &zeroMeansNo)) == 0) &&
      (zeroMeansNo != 0))
    doLogCancelKillServers_ = true;
}

SscpGlobals::~SscpGlobals()
{
  sem_close((sem_t *)semId_);
}

void SscpGuaReceiveControlConnection::actOnSystemMessage(
       short                  messageNum,
       IpcMessageBufferPtr    sysMsg,
       IpcMessageObjSize      sysMsgLen,
       short                  clientFileNumber,
       const GuaProcessHandle &clientPhandle,
       GuaConnectionToClient  *connection)
{
  CliGlobals *cliGlobals = GetCliGlobals();
  switch (messageNum)
    {
    case ZSYS_VAL_SMSG_OPEN:
      {
        SscpNewIncomingConnectionStream *newStream = new(getEnv()->getHeap())
          SscpNewIncomingConnectionStream((NAHeap *)getEnv()->getHeap(),
              getEnv(),getSscpGlobals());

        ex_assert(connection != NULL,"Must create connection for open sys msg");
        newStream->addRecipient(connection);
        newStream->receive(FALSE);
        initialized_ = TRUE;
      }
      break;
    case ZSYS_VAL_SMSG_CLOSE:
      break;
    case ZSYS_VAL_SMSG_CPUDOWN:
    case ZSYS_VAL_SMSG_REMOTECPUDOWN:
    case ZSYS_VAL_SMSG_NODEDOWN:
      // Somebody closed us or went down. Do a search thru all
      // downloaded fragment entries and check whether their
      // client is still using them. The IPC layer will wake
      // up the scheduler so the actual release can take place.
      sscpGlobals_->releaseOrphanEntries();
      break;
    case XZSYS_VAL_SMSG_SHUTDOWN:
      sem_unlink(getRmsSemName());
      // Mark the shared memory segment for desctruction
      shmctl(cliGlobals->getSharedMemId(), IPC_RMID, NULL);
      NAExit(0);
      break;
    default:
      // do nothing for all other kinds of system messages
      break;
    } // switch


}

SscpNewIncomingConnectionStream::SscpNewIncomingConnectionStream(NAHeap *heap, IpcEnvironment *env,
                                SscpGlobals *sscpGlobals) :
                IpcMessageStream(env,
                   IPC_MSG_SSCP_REPLY,
		   CurrSscpReplyMessageVersion,
#ifndef USE_SB_NEW_RI
		   RTS_STATS_MSG_BUF_SIZE,
#else
		   env->getGuaMaxMsgIOSize(),
#endif
		   TRUE)
{
  heap_ = heap;
  sscpGlobals_ = sscpGlobals;
  ipcEnv_ = env;
}


void SscpNewIncomingConnectionStream::actOnSend(IpcConnection *connection)
{
  if (connection->getErrorInfo() == 0)
  {
    sscpGlobals_->incSscpReplyMsg(connection->getLastSentMsg()->getMessageLength());
  }
}

void SscpNewIncomingConnectionStream::actOnSendAllComplete()
{
  clearAllObjects();
  receive(FALSE);
}

void SscpNewIncomingConnectionStream::actOnReceive(IpcConnection *connection)
{
  if (connection->getErrorInfo() != 0)
    return;

  sscpGlobals_->incSscpReqMsg(connection->getLastReceivedMsg()->getMessageLength());
  switch(getNextObjType())
  {
  case RTS_MSG_STATS_REQ:
    processStatsReq(connection);
    break;
  case RTS_MSG_CPU_STATS_REQ:
    processCpuStatsReq(connection);
    break;
  case CANCEL_QUERY_KILL_SERVERS_REQ:
    processKillServersReq();
    break;
  case SUSPEND_QUERY_REQ:
    suspendActivateSchedulers();
    break;
  case SECURITY_INVALID_KEY_REQ:
    processSecInvReq();
    break;
  case LOB_LOCK_REQ:
    processLobLockReq();
    break;
  default:
    ex_assert(FALSE,"Invalid request for first client message");
  }
}

void SscpNewIncomingConnectionStream::actOnReceiveAllComplete()
{
  if (getState() == ERROR_STATE)
    addToCompletedList();
}
void SscpNewIncomingConnectionStream::processStatsReq(IpcConnection *connection)
{
  IpcMessageObjVersion msgVer;
  msgVer = getNextObjVersion();

  if (msgVer > currRtsStatsReqVersionNumber)
    // Send Error
    ;
  RtsStatsReq *request = new (getHeap())
      RtsStatsReq(INVALID_RTS_HANDLE, getHeap());

  *this >> *request;

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

        char *qid = queryId->getQid();
        short reqType = queryId->getStatsReqType();
        RtsStatsReply *reply = new (getHeap())
          RtsStatsReply(request->getHandle(), getHeap());

        SscpGlobals *sscpGlobals = getSscpGlobals();
        StatsGlobals *statsGlobals = sscpGlobals->getStatsGlobals();
        clearAllObjects();
        setType(IPC_MSG_SSCP_REPLY);
        setVersion(CurrSscpReplyMessageVersion);
        int error = statsGlobals->getStatsSemaphore(sscpGlobals->getSemId(),
                sscpGlobals->myPin());
        SyncHashQueue *stmtStatsList = statsGlobals->getStmtStatsList();
        StmtStats *stmtStats = statsGlobals->getStmtStats(qid, str_len(qid));
        ExStatisticsArea *stats;
        ExStatisticsArea *mergedStats = NULL;
        // stats Vptr is not belonging to this process
        // So avoid calling any virtual functions of this class or any embedded classes.
        // We create a mergeStats instance and create a
        // Operator stats entry for each stats entry and then call merge
        // This avoid calling any virtual function in the merge function
        // Also, for some reason, packing any object to the stream is allowed only when the
        // reference count is 1. It looks like some of the StatisticsArea has reference count
        // more than 1.
        while (stmtStats != NULL)
        {
          stats = stmtStats->getStatsArea();
          ComTdb::CollectStatsType statsType;
          if (stats != NULL && ((statsType = stats->getCollectStatsType()) == ComTdb::ACCUMULATED_STATS
                  || statsType == ComTdb::PERTABLE_STATS
                  || statsType == ComTdb::OPERATOR_STATS))
          {
            if (mergedStats == NULL)
            {
              switch (queryId->getStatsMergeType())
              {
              case SQLCLI_ACCUMULATED_STATS:
                 mergedStats = new (getHeap())
                    ExStatisticsArea(getHeap(), 0, ComTdb::ACCUMULATED_STATS,
                        stats->getOrigCollectStatsType());
                break;
              case SQLCLI_PERTABLE_STATS:
                 mergedStats = new (getHeap())
                    ExStatisticsArea(getHeap(), 0, ComTdb::PERTABLE_STATS,
                        stats->getOrigCollectStatsType());
                break;
              case SQLCLI_PROGRESS_STATS:
                 mergedStats = new (getHeap())
                    ExStatisticsArea(getHeap(), 0, ComTdb::PROGRESS_STATS,
                        stats->getOrigCollectStatsType());
                break;
              default:
                 if (reqType == SQLCLI_STATS_REQ_QID_DETAIL)
                    mergedStats = new (getHeap())
                       ExStatisticsArea(getHeap(), 0, ComTdb::QID_DETAIL_STATS,
                        ComTdb::QID_DETAIL_STATS);
                 else
                    mergedStats = new (getHeap())
                       ExStatisticsArea(getHeap(), 0, stats->getCollectStatsType(),
                        stats->getOrigCollectStatsType());
              }
              mergedStats->setDetailLevel(queryId->getDetailLevel());
            }
            if (reqType == SQLCLI_STATS_REQ_QID_DETAIL)
            {
              mergedStats->appendCpuStats(stats, TRUE);
              if (stats->getMasterStats() != NULL)
              {
                ExMasterStats *masterStats = new (getHeap()) ExMasterStats((NAHeap *)getHeap());
                masterStats->copyContents(stats->getMasterStats());
                mergedStats->setMasterStats(masterStats);
              }
            }
            else
            {
              mergedStats->merge(stats, queryId->getStatsMergeType());
              reply->incNumSqlProcs();
            }
          }
          do
          {
            stmtStats = (StmtStats *)stmtStatsList->getNext();
          } while (stmtStats != NULL && str_cmp(qid, stmtStats->getQueryId(), stmtStats->getQueryIdLen()) != 0);
        }

        statsGlobals->releaseStatsSemaphore(sscpGlobals->getSemId(), sscpGlobals->myPin());
#ifdef _DEBUG_RTS
        cerr << "Merged Stats " << mergedStats << " \n";
#endif
        if (mergedStats != NULL)
          reply->incNumCpus();
        *this << *reply;
        if (mergedStats != NULL)
          *this << *mergedStats;
        send(FALSE);
#ifdef _DEBUG_RTS
        cerr << "After send \n";
#endif
	NADELETE(mergedStats, ExStatisticsArea, getHeap());
	reply->decrRefCount();
        queryId->decrRefCount();
        request->decrRefCount();
      }
      break;
    default:
      break;
    }
  }
}

void SscpNewIncomingConnectionStream::processCpuStatsReq(IpcConnection *connection)
{
  Int64 currTimestamp;
  struct timespec currTimespec;
  size_t memThreshold;

  IpcMessageObjVersion msgVer;
  msgVer = getNextObjVersion();

  if (msgVer > currRtsStatsReqVersionNumber)
    // Send Error
    ;
  RtsCpuStatsReq *request = new (getHeap())
      RtsCpuStatsReq(INVALID_RTS_HANDLE, getHeap());

  *this >> *request;
  RtsStatsReply *reply = new (getHeap())
    RtsStatsReply(request->getHandle(), getHeap());

  SscpGlobals *sscpGlobals = getSscpGlobals();
  StatsGlobals *statsGlobals = sscpGlobals->getStatsGlobals();
  clearAllObjects();
  setType(IPC_MSG_SSCP_REPLY);
  setVersion(CurrSscpReplyMessageVersion);
  *this << *reply;
  ExStatisticsArea *stats;
  ExStatisticsArea *mergedStats = NULL;
  StmtStats *stmtStats;
  ExMasterStats *masterStats;
  short reqType = request->getReqType();
  short noOfQueries = request->getNoOfQueries();
  short subReqType = request->getSubReqType();
  Lng32 filter = request->getFilter();
  switch (reqType)
  {
  case SQLCLI_STATS_REQ_CPU_OFFENDER:
    mergedStats = new (getHeap())
      ExStatisticsArea(getHeap(), 0, ComTdb::CPU_OFFENDER_STATS, ComTdb::CPU_OFFENDER_STATS);
    break;
  case SQLCLI_STATS_REQ_SE_OFFENDER:
    mergedStats = new (getHeap())
      ExStatisticsArea(getHeap(), 0, ComTdb::SE_OFFENDER_STATS, ComTdb::SE_OFFENDER_STATS);
    mergedStats->setSubReqType(subReqType);
    break;
  case SQLCLI_STATS_REQ_ET_OFFENDER:
    mergedStats = new (getHeap())
      ExStatisticsArea(getHeap(), 0, ComTdb::ET_OFFENDER_STATS, ComTdb::ET_OFFENDER_STATS);
    break;
  case SQLCLI_STATS_REQ_RMS_INFO:
    mergedStats = new (getHeap())
      ExStatisticsArea(getHeap(), 0, ComTdb::RMS_INFO_STATS, ComTdb::RMS_INFO_STATS);
    mergedStats->setSubReqType(subReqType);
    break;
  case SQLCLI_STATS_REQ_MEM_OFFENDER:
    mergedStats = new (getHeap())
           ExStatisticsArea(getHeap(), 0, ComTdb::MEM_OFFENDER_STATS,
                     ComTdb::MEM_OFFENDER_STATS);
    mergedStats->setSubReqType(subReqType);
    break;
  default:
    ex_assert(0, "Unsupported Request Type");
  }
  mergedStats->setDetailLevel(request->getNoOfQueries());
  if (reqType != SQLCLI_STATS_REQ_RMS_INFO &&
          reqType != SQLCLI_STATS_REQ_MEM_OFFENDER)
  {
    int error = statsGlobals->getStatsSemaphore(sscpGlobals->getSemId(),
            sscpGlobals->myPin());
    SyncHashQueue *stmtStatsList = statsGlobals->getStmtStatsList();
    stmtStatsList->position();
    if (reqType == SQLCLI_STATS_REQ_ET_OFFENDER)
    {
       currTimestamp = NA_JulianTimestamp();
       while ((stmtStats = (StmtStats *)stmtStatsList->getNext()) != NULL)
       {
          stats = stmtStats->getStatsArea();
          masterStats = stmtStats->getMasterStats();
          if (masterStats != NULL)
             mergedStats->appendCpuStats(masterStats, FALSE,
                subReqType, filter, currTimestamp);
       }
    }
    else if (reqType == SQLCLI_STATS_REQ_SE_OFFENDER)
    {
       clock_gettime(CLOCK_MONOTONIC, &currTimespec);
       while ((stmtStats = (StmtStats *)stmtStatsList->getNext()) != NULL)
       {
          stats = stmtStats->getStatsArea();
          if (stats != NULL)
             mergedStats->appendCpuStats(stats, FALSE, filter, currTimespec); 
       }
    }
    else
    {
       currTimestamp = -1;
       while ((stmtStats = (StmtStats *)stmtStatsList->getNext()) != NULL)
       {
          stats = stmtStats->getStatsArea();
          if (stats != NULL)
             mergedStats->appendCpuStats(stats, FALSE);
       }
    }
    statsGlobals->releaseStatsSemaphore(sscpGlobals->getSemId(), sscpGlobals->myPin());
  }
  if (reqType == SQLCLI_STATS_REQ_RMS_INFO)
  {
    ExRMSStats *rmsStats = new (getHeap()) ExRMSStats(getHeap());
    rmsStats->copyContents(statsGlobals->getRMSStats());
    NAHeap *statsHeap = statsGlobals->getStatsHeap();
    rmsStats->setGlobalStatsHeapAlloc(statsHeap->getTotalSize());
    rmsStats->setGlobalStatsHeapUsed(statsHeap->getAllocSize());
    rmsStats->setStatsHeapWaterMark(statsHeap->getHighWaterMark());
    rmsStats->setNoOfStmtStats(statsGlobals->getStmtStatsList()->numEntries());
    rmsStats->setSemPid(statsGlobals->getSemPid());
    rmsStats->setNumQueryInvKeys(statsGlobals->getRecentSikeys()->entries());
    mergedStats->insert(rmsStats);
    if (request->getNoOfQueries() == RtsCpuStatsReq::INIT_RMS_STATS_)
        statsGlobals->getRMSStats()->reset();
  }
  if (reqType == SQLCLI_STATS_REQ_MEM_OFFENDER)
  {
    statsGlobals->getMemOffender(mergedStats, filter);
  }
  if (mergedStats->numEntries() > 0)
    *this << *mergedStats;
  send(FALSE);
  NADELETE(mergedStats, ExStatisticsArea, getHeap());
  reply->decrRefCount();
  request->decrRefCount();
}

int reportStops(int alreadyStoppedCnt, int stoppedCnt)
{
  return alreadyStoppedCnt + stoppedCnt;
}

void SscpNewIncomingConnectionStream::processKillServersReq()
{
  int alreadyStoppedCnt = 0;
  int stoppedCnt = 0;

  // On SQ, stop catcher does not run for processes stopped by another
  // process, so the original loop below will not advance beyond the first
  // ESP.  So we will keep a list of already stopped ESPs and skip these.
  // This is an N-squared algorithm, but it is mitigated in two ways:
  // 1.) N is number of ESPs local to this MXSSCP, problably never more than
  // ten or so;
  // 2.) After first process is stopped, the others will most likely not
  // need to be, so  the list of already stopped ESPs will be only one or two.

  HashQueue alreadyStopped( getHeap() );

  IpcMessageObjVersion msgVer = getNextObjVersion();

  ex_assert(msgVer <= currRtsStatsReqVersionNumber, "Up-rev message received.");

  CancelQueryKillServersRequest *request = new (getHeap())
    CancelQueryKillServersRequest(INVALID_RTS_HANDLE, getHeap());

  *this >> *request;

  ex_assert(moreObjects(), "CancelQueryKillServersRequest all by itself.");

  RtsMessageObjType objType = (RtsMessageObjType) getNextObjType();

  ex_assert(objType == RTS_QUERY_ID,
            "CancelQueryKillServersRequest came with unknown msg obj.");

  RtsQueryId *queryId = new (getHeap()) RtsQueryId(getHeap());

  *this >> *queryId;
  char *qid = queryId->getQueryId();
  Lng32 qidLen = queryId->getQueryIdLen();

  SscpGlobals *sscpGlobals = getSscpGlobals();
  StatsGlobals *statsGlobals = sscpGlobals->getStatsGlobals();
  clearAllObjects();
  setType(IPC_MSG_SSCP_REPLY);
  setVersion(CurrSscpReplyMessageVersion);

  int error = statsGlobals->getStatsSemaphore(sscpGlobals->getSemId(),
                  sscpGlobals->myPin());

  SyncHashQueue *stmtStatsList = statsGlobals->getStmtStatsList();
  stmtStatsList->position(qid, qidLen);

  StmtStats *kqStmtStats = NULL;

  while (NULL != (kqStmtStats = (StmtStats *)stmtStatsList->getNext()))
  {
    if (str_cmp(kqStmtStats->getQueryId(), qid, qidLen) != 0)
    {
      // This stmtStats is on the HashQueue collision chain, but
      // it is for a different query id.  Keep looking.
      continue;
    }

    ExOperStats *rootStats = NULL;
    if (kqStmtStats->getStatsArea())
      rootStats = kqStmtStats->getStatsArea()->getRootStats();

    ExFragRootOperStats *rootOperStats = NULL;
    if ( rootStats &&
        (rootStats->statType() == ExOperStats::ROOT_OPER_STATS))
      rootOperStats = (ExFragRootOperStats *) rootStats;

    ExMeasStats *measStats = NULL;
    if ( rootStats &&
        (rootStats->statType() == ExOperStats::MEAS_STATS))
      measStats = (ExMeasStats *) rootStats;

    if (!rootOperStats && !measStats)
    {
      // Could be operator stats or other?
      continue;
    }

    // Make sure the ESP is still working on the query.  ESPs update
    // their exectionCount_ when they *finish*, so this is a good
    // test.

    if ( rootOperStats  &&
        (request->getExecutionCount() != rootOperStats->getExecutionCount()))
      continue;

    if ( measStats  &&
        (request->getExecutionCount() != measStats->getExecutionCount()))
      continue;

      const SB_Phandle_Type *statsPhandle = rootOperStats ?
                                  rootOperStats->getPhandle() :
                                  measStats->getPhandle();

    GuaProcessHandle gph;
    mem_cpy_all(&gph.phandle_, statsPhandle, sizeof(gph.phandle_));

    // Don't stop the query's master executor here.
    if (request->getMasterPhandle() == gph)
      continue;

    //Phandle wrapper in porting layer
    NAProcessHandle phandle((SB_Phandle_Type *)&gph.phandle_);

    int guaRetcode = phandle.decompose();

    if (!guaRetcode)
    {
      char *phandleString = phandle.getPhandleString();
      short phandleStrLen =  phandle.getPhandleStringLen();

      alreadyStopped.position(phandleString, phandleStrLen);
      char *alreadyStoppedEsp = NULL;
      bool wasAlreadyStopped = false;
      while ( NULL != (alreadyStoppedEsp = (char *) alreadyStopped.getNext()))
      {
        if (0 == memcmp(alreadyStoppedEsp, phandleString, phandleStrLen))
        {
          wasAlreadyStopped = true;
          alreadyStoppedCnt++;
          break;
        }
      }

      if (wasAlreadyStopped)
        continue;
      else
        alreadyStopped.insert(phandleString, phandleStrLen, phandleString);
    }

    // Okay, here goes...
    stoppedCnt++;
    statsGlobals->releaseStatsSemaphore(sscpGlobals->getSemId(),
                                      sscpGlobals->myPin());
    gph.dumpAndStop(request->getMakeSaveabend(),
                    true);                    // doStop

    // wait 100 milliseconds.  This will increase the chances that the
    // internal cancel (and error handling for SQLCODE 2034) will clean up
    // the query and thereby minimize the # of ESPs that must be killed.
    DELAY(10);

    // Reacquire the sema4.  And reposition into the HashQueue.
    error = statsGlobals->getStatsSemaphore(sscpGlobals->getSemId(),
                    sscpGlobals->myPin());

    stmtStatsList->position(qid, qidLen);
  }

  statsGlobals->releaseStatsSemaphore(sscpGlobals->getSemId(),
                                    sscpGlobals->myPin());

  if (sscpGlobals->shouldLogCancelKillServers() ||
      request->getCancelLogging())
  {
    char msg[120 + // the constant text
             ComSqlId::MAX_QUERY_ID_LEN
            ];

    str_sprintf(msg,
      "Escalation of cancel of query %s caused %d ESP server "
      "process(es) to be stopped and %d to be dumped.",
       queryId->getQueryId(), stoppedCnt,
       (request->getMakeSaveabend() ? stoppedCnt : 0) );

    SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg, 0);
  }

  RtsHandle rtsHandle = (RtsHandle) this;
  CancelQueryKillServersReply *reply = new(getHeap())
        CancelQueryKillServersReply( rtsHandle , getHeap());
  *this << *reply;

  send(FALSE);

  queryId->decrRefCount();
  reply->decrRefCount();
  request->decrRefCount();
  reportStops(alreadyStoppedCnt, stoppedCnt);
}

void SscpNewIncomingConnectionStream::suspendActivateSchedulers()
{
  int espFragCnt = 0;
  IpcMessageObjVersion msgVer = getNextObjVersion();

  ex_assert(msgVer <= currRtsStatsReqVersionNumber, "Up-rev message received.");

  SuspendActivateServersRequest *request = new (getHeap())
    SuspendActivateServersRequest(INVALID_RTS_HANDLE, getHeap());

  *this >> *request;

  ex_assert(moreObjects(), "SuspendActivateServersRequest all by itself.");

  RtsMessageObjType objType = (RtsMessageObjType) getNextObjType();

  ex_assert(objType == RTS_QUERY_ID,
            "SuspendActivateServersRequest came with unknown msg obj.");

  RtsQueryId *queryId = new (getHeap()) RtsQueryId(getHeap());

  *this >> *queryId;
  char *qid = queryId->getQueryId();
  Lng32 qidLen = queryId->getQueryIdLen();

  SscpGlobals *sscpGlobals = getSscpGlobals();
  StatsGlobals *statsGlobals = sscpGlobals->getStatsGlobals();
  clearAllObjects();
  setType(IPC_MSG_SSCP_REPLY);
  setVersion(CurrSscpReplyMessageVersion);

  int error = statsGlobals->getStatsSemaphore(sscpGlobals->getSemId(),
                  sscpGlobals->myPin());

  SyncHashQueue *stmtStatsList = statsGlobals->getStmtStatsList();
  stmtStatsList->position(qid, qidLen);

  StmtStats *kqStmtStats = NULL;

  while (NULL != (kqStmtStats = (StmtStats *)stmtStatsList->getNext()))
  {
    if (str_cmp(kqStmtStats->getQueryId(), qid, qidLen) != 0)
    {
      // This stmtStats is on the HashQueue collision chain, but
      // it is for a different query id.  Keep looking.
      continue;
    }

    ExOperStats *rootStats = NULL;
    if (kqStmtStats->getStatsArea())
      rootStats = kqStmtStats->getStatsArea()->getRootStats();

    ExFragRootOperStats *rootOperStats = NULL;
    if ( rootStats &&
        (rootStats->statType() == ExOperStats::ROOT_OPER_STATS))
      rootOperStats = (ExFragRootOperStats *) rootStats;

    ExMeasStats *measStats = NULL;
    if ( rootStats &&
        (rootStats->statType() == ExOperStats::MEAS_STATS))
      measStats = (ExMeasStats *) rootStats;

    // Logic in ex_root_tcb::register query ensures that the
    // registered query has root_oper or meas stats.
    ex_assert(rootOperStats || measStats,
              "suspending/activating unregistered query.")

    if (rootOperStats)
      rootOperStats->setFragSuspended(request->isRequestToSuspend());
    else if (measStats)
      measStats->setFragSuspended(request->isRequestToSuspend());
   espFragCnt++;
  }

  statsGlobals->releaseStatsSemaphore(sscpGlobals->getSemId(),
                                    sscpGlobals->myPin());

  if (request->getSuspendLogging())
  {
    char msg[80 +
           ComSqlId::MAX_QUERY_ID_LEN];

    str_sprintf(msg,
       "MXSSCP has %s %d fragment(s) for query %s.",
       ( request->isRequestToSuspend() ? "suspended" : "reactivated" ),
        espFragCnt, qid);
    SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg, 0);
  }

  RtsHandle rtsHandle = (RtsHandle) this;
  CancelQueryKillServersReply *reply = new(getHeap())
        CancelQueryKillServersReply( rtsHandle , getHeap());
  *this << *reply;

  send(FALSE);

  queryId->decrRefCount();
  reply->decrRefCount();
  request->decrRefCount();
}

static bool revokeTimerInitialized = false;
static bool revokeTimer = false;

void SscpNewIncomingConnectionStream::processSecInvReq()
{
  IpcMessageObjVersion msgVer = getNextObjVersion();

  ex_assert(msgVer <= currRtsStatsReqVersionNumber, "Up-rev message received.");

  SecInvalidKeyRequest *request = new(getHeap())
    SecInvalidKeyRequest(getHeap());

  *this >> *request;

  ex_assert( !moreObjects(), "unknown object follows SecInvalidKeyRequest.");

  Int32 numSiks = request->getNumSiks();
  if (numSiks)
  {
    if (!revokeTimerInitialized)
    {
      revokeTimerInitialized = true;
      char *r = getenv("RMS_REVOKE_TIMER");
      if (r && *r != '0')
        revokeTimer = true;
    }
    SscpGlobals *sscpGlobals = getSscpGlobals();
    StatsGlobals *statsGlobals = sscpGlobals->getStatsGlobals();
    int error = statsGlobals->getStatsSemaphore(sscpGlobals->getSemId(),
                  sscpGlobals->myPin());
    ExTimeStats timer;
    if (revokeTimer)
      timer.start();

    SyncHashQueue *stmtStatsList = statsGlobals->getStmtStatsList();
    StmtStats *kqStmtStats = NULL;
    stmtStatsList->position();
    // Look at each StmtStats
    while (NULL != (kqStmtStats = (StmtStats *)stmtStatsList->getNext()))
    {
      ExMasterStats *masterStats = kqStmtStats->getMasterStats();
      if (masterStats)
      {
        bool keysAreInvalid = false;
        // for each new invalidation key
        for (Int32 i = 0; i < numSiks && !keysAreInvalid; i++)
        {
          ComQIActionType siKeyType =
            ComQIActionTypeLiteralToEnum(request->getSik()[i].operation);
          if (siKeyType == COM_QI_OBJECT_REDEF)
          {
            // compare the new DDL invalidation key to each key in the
            // master stats.
            for (Int32 m = 0; m < masterStats->getNumObjUIDs()
                              && !keysAreInvalid; m++)            
            {
              if (masterStats->getObjUIDs()[m] ==
                  request->getSik()[i].ddlObjectUID)
              {
                keysAreInvalid = true;
                masterStats->setValidDDL(false);
              }
            }
          }

          // If a role is granted or revoked from a user do checks next time query is executed
          else if (siKeyType == COM_QI_USER_GRANT_ROLE)
          {
             keysAreInvalid = true;
             masterStats->setValidPrivs(false);
          }

          else if (siKeyType != COM_QI_STATS_UPDATED)
          {
            // compare the new REVOKE invalidation key to each key in the 
            // master stats.
            for (Int32 m = 0; m < masterStats->getNumSIKeys() 
                              && !keysAreInvalid; m++)
            {
              if (!memcmp(&masterStats->getSIKeys()[m], 
                          &request->getSik()[i], sizeof(SQL_QIKEY)))
              {
                keysAreInvalid = true;
                masterStats->setValidPrivs(false);
              } 
            }  // for each key in master stats.
          }  // revoke 
        }  // for each new invalidation key
      }  // if masterstats
    }
    statsGlobals->mergeNewSikeys(numSiks, request->getSik());

    statsGlobals->releaseStatsSemaphore(sscpGlobals->getSemId(),
                                    sscpGlobals->myPin());
    if (revokeTimer)
    {
      timer.stop();
      Int64 microSeconds = timer.getTime();
      char msg[256];
      str_sprintf(msg,
          "MXSSCP has processed %d security invalidation "
          "keys in %d milliseconds.",
          numSiks, (Int32)microSeconds / 1000);

      SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg, 0);
    }

  }

  clearAllObjects();
  setType(IPC_MSG_SSCP_REPLY);
  setVersion(CurrSscpReplyMessageVersion);

  RmsGenericReply *reply = new(getHeap())
    RmsGenericReply(getHeap());

  *this << *reply;

  send(FALSE);
  reply->decrRefCount();
  request->decrRefCount();
}

void SscpNewIncomingConnectionStream::processLobLockReq()
{
  IpcMessageObjVersion msgVer = getNextObjVersion();

  ex_assert(msgVer <= currRtsStatsReqVersionNumber, "Up-rev message received.");
  NAHeap *statsHeap = NULL;
  LobLockRequest *request = new(getHeap())
    LobLockRequest(getHeap());

  *this >> *request;
  ex_assert( !moreObjects(), "unknown object follows LobLockRequest.");
  SscpGlobals *sscpGlobals = getSscpGlobals();
  StatsGlobals *statsGlobals = sscpGlobals->getStatsGlobals();
  int error = statsGlobals->getStatsSemaphore(sscpGlobals->getSemId(),
                  sscpGlobals->myPin());
  statsHeap = statsGlobals->getStatsHeap();
  char *ll = new (statsHeap) char [LOB_LOCK_ID_SIZE];
  memcpy(ll,request->getLobLockId(),LOB_LOCK_ID_SIZE+1);
  SyncHashQueue *lobLockList = statsGlobals->getLobLocks();
  if (ll[0] == '+') // If it's a positive value, we are supposed to insert it.
    lobLockList->insert(&ll[1],LOB_LOCK_ID_SIZE,&ll[1]);
  else if (ll[0] =='-')
    {
      //negative value means we need to remove/release it from the list
      lobLockList->position((char *)&ll[1], LOB_LOCK_ID_SIZE);
      while (lobLockList->getCurr() && 
             memcmp(lobLockList->getCurr(), &ll[1],LOB_LOCK_ID_SIZE)!= 0)
        lobLockList->getNext();
 
      lobLockList->remove((char *)&ll[1], LOB_LOCK_ID_SIZE,lobLockList->getCurr());
    }
  else
    ex_assert(FALSE,"invalid lob lock id in LobLockRequest");
    
    
   statsGlobals->releaseStatsSemaphore(sscpGlobals->getSemId(),
                                    sscpGlobals->myPin());
   clearAllObjects();
   setType(IPC_MSG_SSCP_REPLY);
   setVersion(CurrSscpReplyMessageVersion);

   RmsGenericReply *reply = new(getHeap())
    RmsGenericReply(getHeap());

   *this << *reply;

   send(FALSE);
   reply->decrRefCount();
   request->decrRefCount();
}
