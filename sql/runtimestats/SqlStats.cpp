/*********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlStats.cpp
 * Description:  
 * Created:      2/28/06
 * Language:     C++
 *
 *****************************************************************************
 */
#include "cli_stdh.h"
#include  "ex_stdh.h"
#include "ExStats.h"
#include "sql_id.h"
#include "ExCextdecs.h"
#include "Ipc.h"
#include "ComSqlId.h"
#include "PortProcessCalls.h"
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include "seabed/ms.h"
#include "seabed/fserr.h"
void *StatsGlobals::operator new (size_t size, void* loc)
{
  if (loc)
    return loc; 
  return ::operator new(size);
}

StatsGlobals::StatsGlobals(void *baseAddr, short envType, Lng32 maxSegSize)
    : statsHeap_("Stats Globals",
      getStatsSegmentId(),
      baseAddr,
      ((sizeof(StatsGlobals)+16-1)/16)*16,
      maxSegSize,
      maxSegSize,
      &segGlobals_,
      0), // Zero secondary segments
      recentSikeys_(NULL),
      newestRevokeTimestamp_(0), // all new compilers are current.
      statsArray_(NULL)
      , semPid_(-1)
      , semPidCreateTime_(0)
      , maxPid_(0)
      , pidToCheck_(0)
      , ssmpDumpedTimestamp_(0)
{
  statsHeap_.setSharedMemory();
  //Phandle wrapper in porting layer
  NAProcessHandle phandle;
  phandle.getmine();
  phandle.decompose();
  cpu_ = phandle.getCpu(); 

  statsSharedSegAddr_ = baseAddr; 
  version_ = CURRENT_SHARED_OBJECTS_VERSION_;
  isSscpInitialized_ = FALSE;
  // LCOV_EXCL_START 
  // On Seaquest, no need for a private env.
  if (envType == 0)
    rtsEnvType_ = RTS_PRIVATE_ENV;
  else
  // LCOV_EXCL_STOP
    rtsEnvType_ = RTS_GLOBAL_ENV;
  storeSqlSrcLen_ = RMS_STORE_SQL_SOURCE_LEN;
  abortedSemPid_ = -1;
  errorSemPid_ = -1;
  releasingSemPid_ = -1;
  seabedError_ = 0;
  seabedPidRecycle_ = false;
}

void StatsGlobals::init()
{
  Long semId;
  short error;
  short savedPriority, savedStopMode;
  char myNodeName[MAX_SEGMENT_NAME_LEN+1];

  error = openStatsSemaphore(semId);
  ex_assert(error == 0, "BINSEM_OPEN returned an error");

  error = getStatsSemaphore(semId, GetCliGlobals()->myPin(),  savedPriority,
                                savedStopMode,
                                FALSE /*shouldTimeout*/);
  ex_assert(error == 0, "getStatsSemaphore() returned an error");
  
  stmtStatsList_ = new (&statsHeap_) HashQueue(&statsHeap_, 512);
  rmsStats_ = new (&statsHeap_) ExRMSStats(&statsHeap_);
  recentSikeys_ = new (&statsHeap_) HashQueue(&statsHeap_, 512);
  rmsStats_->setCpu(cpu_);
  rmsStats_->setRmsVersion(version_);
  rmsStats_->setRmsEnvType(rtsEnvType_);
  rmsStats_->setStoreSqlSrcLen(storeSqlSrcLen_);
  int rc;
  nodeId_ = cpu_;
  MS_Mon_Node_Info_Type nodeInfo;

  rc = msg_mon_get_node_info_detail(nodeId_, &nodeInfo);
  if (rc == 0)
     strcpy(myNodeName, nodeInfo.node[0].node_name);
  else
    myNodeName[0] = '\0';
  rmsStats_->setNodeName(myNodeName);
  releaseStatsSemaphore(semId, GetCliGlobals()->myPin(), savedPriority,
            savedStopMode);
  sem_close((sem_t *)semId);
}

#ifndef __EID
Int64 StatsGlobals::getLastGCTime() { return rmsStats_->getLastGCTime(); }
pid_t StatsGlobals::getSsmpPid() { return rmsStats_->getSsmpPid(); } 
Int64 StatsGlobals::getSsmpTimestamp() { return rmsStats_->getSsmpTimestamp(); }
void StatsGlobals::setLastGCTime(Int64 gcTime) { rmsStats_->setLastGCTime(gcTime); }
void StatsGlobals::incStmtStatsGCed(short inc) { rmsStats_->incStmtStatsGCed(inc); }
void StatsGlobals::incSsmpReqMsg(Int64 msgBytes) { rmsStats_->incSsmpReqMsg(msgBytes); }
void StatsGlobals::incSsmpReplyMsg(Int64 msgBytes) { rmsStats_->incSsmpReplyMsg(msgBytes); }
void StatsGlobals::incSscpReqMsg(Int64 msgBytes) { rmsStats_->incSscpReqMsg(msgBytes); }
void StatsGlobals::incSscpReplyMsg(Int64 msgBytes) { rmsStats_->incSscpReplyMsg(msgBytes); }
void StatsGlobals::setSscpOpens(short numSscps) { rmsStats_->setSscpOpens(numSscps); }
void StatsGlobals::setSscpDeletedOpens(short numSscps) { rmsStats_->setSscpDeletedOpens(numSscps);  }
void StatsGlobals::setSscpPid(pid_t pid) { rmsStats_->setSscpPid(pid); }
void StatsGlobals::setSscpPriority(short pri) {rmsStats_->setSscpPriority(pri); }
void StatsGlobals::setSscpTimestamp(Int64 timestamp) { rmsStats_->setSscpTimestamp(timestamp);  }
void StatsGlobals::setSsmpPid(pid_t pid) { rmsStats_->setSsmpPid(pid); }
void StatsGlobals::setSsmpPriority(short pri) {rmsStats_->setSsmpPriority(pri); }
void StatsGlobals::setSsmpTimestamp(Int64 timestamp) { rmsStats_->setSsmpTimestamp(timestamp);  }
void StatsGlobals::setRMSStatsResetTimestamp(Int64 timestamp) { rmsStats_->setRMSStatsResetTimestamp(timestamp);  }
void StatsGlobals::incProcessRegd() { rmsStats_->incProcessRegd();  }
void StatsGlobals::decProcessRegd() { rmsStats_->decProcessRegd();  }
void StatsGlobals::incProcessStatsHeaps() { rmsStats_->incProcessStatsHeaps();  }
void StatsGlobals::decProcessStatsHeaps() { rmsStats_->decProcessStatsHeaps();  }
void StatsGlobals::setNodesInCluster(short numNodes) 
           { rmsStats_->setNodesInCluster(numNodes); }
#endif
// LCOV_EXCL_START
// Coverage cannot be collected here on Linux.  This method is called only 
// if the env is Private -- see ExExeUilGetStats.cpp 
const char *StatsGlobals::rmsEnvType(RTSEnvType envType) 
{
  switch (envType)
  {
  case RTS_GLOBAL_ENV: return "Global Environment";
  case RTS_PRIVATE_ENV: return "Private Environment";
  default: return "Unknown";
  }
}
// LCOV_EXCL_STOP

void StatsGlobals::addProcess(pid_t pid, NAHeap *heap)
{
  if (statsArray_ == NULL)
  {
    statsArray_ = new (&statsHeap_) GlobalStatsArray[MAX_PID_ARRAY_SIZE];
    for (pid_t i = 0; i < MAX_PID_ARRAY_SIZE ; i++)
    {
      statsArray_[i].processId_ = 0;
      statsArray_[i].processFlags_ = 0;
      statsArray_[i].processStats_ = NULL;
      statsArray_[i].removedAtAdd_ = FALSE; 
      statsArray_[i].creationTime_ = 0;
    }
  }
  if (statsArray_[pid].processStats_ != NULL)
  {

    char msg[256];;
    str_sprintf(msg,
        "Pid %d,%d got recycled soon or SSMP didn't receive the death message ",
           cpu_, pid);
    SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg, 0);
    statsArray_[pid].removedAtAdd_ = TRUE;
    removeProcess(pid, TRUE);
  }   
  statsArray_[pid].processId_ = pid;
  statsArray_[pid].processFlags_ = 0;
  statsArray_[pid].creationTime_ = GetCliGlobals()->myStartTime();
  statsArray_[pid].processStats_ = new (heap) ProcessStats(heap, nodeId_, pid);
  incProcessRegd();
  incProcessStatsHeaps();
  if (pid > maxPid_)
     maxPid_ = pid;
}

void StatsGlobals::removeProcess(pid_t pid, NABoolean calledAtAdd)
{
  short retcode;
  NABoolean queryRemain = FALSE;
  NAHeap *prevHeap = NULL;
  if (statsArray_ == NULL)
     return;
  if (statsArray_[pid].processStats_ != NULL)
  {
    if (!calledAtAdd)
    {
      if (statsArray_[pid].removedAtAdd_ )
      {
        statsArray_[pid].removedAtAdd_ = FALSE;

        // check if process exists
        if (kill(pid, 0) == 0) 
        {
          //Possibily pid was recycled and death message came later
          // ignore it
          return;
        }
      }
      else
      {
        // check if process pid exists. If not, cleanup. But if it 
        // does exist, do not be fooled by pid recycle. To prevent
        // the pid recycle scenario from causing problems, check 
        // the creation timestamp of the current incarnation of the 
        // pid. If it differs from the creation timestamp recorded
        // during addProcess, or if the new incarnation is not even 
        // a known process (i.e., msg_mon_get_process_name fails)
        // then do the cleanup. Otherwise, cannot cleanup. 
        if (kill(pid, 0) == 0)
        {
          Int64 processCreateTime = -1;
          short errorDetail = 0;
          if ((ComRtGetProcessCreateTime(&cpu_, &pid, NULL, 
                                        processCreateTime, errorDetail) 
              == XZFIL_ERR_OK) &&
              (processCreateTime == statsArray_[pid].creationTime_))
            return;
        }
      }
    }
    stmtStatsList_->position();
    StmtStats *ss;
    prevHeap = statsArray_[pid].processStats_->getHeap();
    while ((ss = (StmtStats *)stmtStatsList_->getNext()) != NULL)
    {
      if (ss->getPid() == pid)
      {
         retcode = removeQuery(pid, ss, FALSE, TRUE, TRUE);
         if (retcode == 1)
         {
            // Now that query remains after the process is gone, 
            // you can delete the stats, when the query is removed
            queryRemain = TRUE;
         }
      }
    }
    NADELETE(statsArray_[pid].processStats_, ProcessStats, prevHeap);
    decProcessRegd();
    if (! (queryRemain))
    {
      // Don't call NADELETE, since NADELETE needs update to VFPTR table of NAHeap object
      prevHeap->destroy();
      statsHeap_.deallocateMemory(prevHeap); 
      decProcessStatsHeaps();
    }
  }
  statsArray_[pid].processId_ = 0;
  statsArray_[pid].processFlags_ = 0;
  statsArray_[pid].creationTime_ = 0;
  statsArray_[pid].processStats_ = NULL;
  if (pid == maxPid_)
  { 
     for (maxPid_-- ; maxPid_ > 0 ; maxPid_--)
     {
         if (statsArray_[maxPid_].processId_  != 0)
            break;
     }
  } 
}

static bool  DeadPollingInitialized = false;
static bool  MissingDeathMsgDebug = false;
static Int32 CheckDeadFreq  = 120;
static Int32 CheckDeadTs    = 0;

void StatsGlobals::checkForDeadProcesses(pid_t myPid)
{
  short error = 0;
  short savedPriority, savedStopMode;

  if (statsArray_ == NULL)
    return;

  if (!DeadPollingInitialized)
  {
    DeadPollingInitialized = true;  // make getenv calls once per process
    {
      char *cdf = getenv("MXSSMP_CHECK_DEAD_SECONDS");
      if (cdf)
        CheckDeadFreq = str_atoi(cdf, str_len(cdf)); // LCOV_EXCL_LINE

      char *mdmd = getenv("MXSSMP_DEATH_MSG_DEBUG");
      if (mdmd && (*mdmd == '1'))
        MissingDeathMsgDebug = true;  // LCOV_EXCL_LINE
    }
  }

  if (CheckDeadFreq <= 0)
    return;

  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  if (ts.tv_sec - CheckDeadTs < CheckDeadFreq)
  {
    if (CheckDeadTs == 0)
      CheckDeadTs = ts.tv_sec;
    // too soon to re-check.
    return;
  }

  CheckDeadTs = ts.tv_sec;

  error = getStatsSemaphore(ssmpProcSemId_,
                            myPid, savedPriority,
                            savedStopMode, FALSE);
  ex_assert(error == 0, "Error from getStatsSemaphore.");

  int pidRemainingToCheck = 20;
  pid_t firstPid = pidToCheck_;
  for (;;)
  {
    if (pidRemainingToCheck <= 0)
      break;

    pidToCheck_++;

    if (pidToCheck_ > maxPid_)
      pidToCheck_ = 0;

    if (pidToCheck_ == firstPid)
      break;

    if (statsArray_[pidToCheck_].processId_ != 0)
    {
      pidRemainingToCheck--;

      char processName[MS_MON_MAX_PROCESS_NAME];
      Int32 ln_error = msg_mon_get_process_name(
                       cpu_, statsArray_[pidToCheck_].processId_, 
                       processName);
      if (ln_error == XZFIL_ERR_NOSUCHDEV) 
        removeProcess(pidToCheck_);
    }
  }

  releaseStatsSemaphore(ssmpProcSemId_, myPid,
                      savedPriority, savedStopMode);
}

// We expect a death message to be delivered to MXSSMP by the monitor
// when a generic SQL process exits -- see code handling system messages.
// This method is to address a concern that the death message may come 
// more than 30 seconds after the process exits and the process could
// be unexpectedly holding the stats semaphore.  

static Int32 SemHeldTooManySeconds = -1;

void StatsGlobals::cleanupDanglingSemaphore(NABoolean checkForSemaphoreHolders)
{
  CliGlobals *cliGlobals = GetCliGlobals();
  NABoolean cleanupSemaphore = FALSE;
  if (semPid_ == -1)
    return; // Nobody has the semaphore, nothing to do here.
    
  if (NOT (cliGlobals->myPin() == getSsmpPid()
          && cliGlobals->myStartTime() == getSsmpTimestamp()))
    return; // Only ssmp is allowed to cleanup after another process.

  // LCOV_EXCL_START
  // Coverage notes - it would be too difficult to automate a test
  // for this since usually a death message is used to clean up a
  // generic SQL process' exit.  But this code has been unit tested
  // using gdb sessions on the generic process and on this MXSSMP
  // process.
    if (checkForSemaphoreHolders)
    {
       if (SemHeldTooManySeconds < 0)
       {
          // call getenv once per process
          char *shtms = getenv("MXSSMP_SEM_RELEASE_SECONDS");
          if (shtms)
          {
             SemHeldTooManySeconds = str_atoi(shtms, str_len(shtms));
             if (SemHeldTooManySeconds < 1)
                SemHeldTooManySeconds = 10;
           }
           else
           SemHeldTooManySeconds = 10;
        }

        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        if ((ts.tv_sec - lockingTimestamp_.tv_sec) >= SemHeldTooManySeconds)
           cleanupSemaphore  = TRUE;
    }
    else
        cleanupSemaphore = TRUE;
    if (cleanupSemaphore)
    {
      NAProcessHandle myPhandle;
      myPhandle.getmine();
      myPhandle.decompose();
      char processName[MS_MON_MAX_PROCESS_NAME + 1];
      pid_t tempPid = semPid_;
      bool semHoldingProcessExists = true; 
      Int32 ln_error = msg_mon_get_process_name(myPhandle.getCpu(), 
                                          tempPid, processName);
      if (ln_error == XZFIL_ERR_NOSUCHDEV) 
      {
        semHoldingProcessExists = false;
        seabedError_ = ln_error;
        seabedPidRecycle_ = false;
      }
      else
      {
        // Is this the same incarnation of the process name?
        // Do not be fooled by pid recycle.
        MS_Mon_Process_Info_Type processInfo;
        ln_error = msg_mon_get_process_info_detail(
                      processName, &processInfo);
        if ((ln_error == XZFIL_ERR_OK) && 
            (ComRtGetJulianFromUTC(processInfo.creation_time) !=
            semPidCreateTime_))
        {
          seabedError_ = 0;
          seabedPidRecycle_ = true;
          semHoldingProcessExists = false;
        }
      }
      if (!semHoldingProcessExists)
      {
        cleanup_SQL(tempPid, myPhandle.getPin());
      }
    }
    // LCOV_EXCL_STOP
}

ProcessStats *StatsGlobals::checkProcess(pid_t pid)
{

  if (statsArray_ == NULL)
    return NULL;
  if (statsArray_[pid].processId_ == pid)
    return statsArray_[pid].processStats_;
  else
    return NULL;
}

StmtStats *StatsGlobals::addQuery(pid_t pid, char *queryId, Lng32 queryIdLen, 
                      void *backRef, Lng32 fragId,
                      char *sourceStr, Lng32 sourceStrLen,
                      NABoolean isMaster)
{
  StmtStats *ss;
  char *sqlSrc = NULL;
  Lng32 storeSqlSrcLen = 0;
  if (storeSqlSrcLen_ > 0)
  {
    sqlSrc = sourceStr;
    storeSqlSrcLen = ((sourceStrLen > storeSqlSrcLen_) ? storeSqlSrcLen_ : sourceStrLen);
  }
  if (statsArray_[pid].processStats_ != NULL && queryId != NULL)
  {
    ss = statsArray_[pid].processStats_->addQuery(pid, queryId, queryIdLen, 
                 backRef, fragId, sqlSrc, 
                 storeSqlSrcLen, sourceStrLen, isMaster);
    stmtStatsList_->insert(queryId, queryIdLen, ss);
    return ss;
  }
  else
    return NULL;
}

short StatsGlobals::getStatsSemaphore(Long &semId, pid_t pid, 
          short &savedPriority, short &savedStopMode, NABoolean shouldTimeout)
{
  short error = 0;
  timespec ts;
  Int32 ln_error;
  ln_error = sem_trywait((sem_t *)semId);
  NABoolean retrySemWait = FALSE;
  NABoolean resetClock = TRUE;
  if (ln_error != 0)
  {
    do
    { 
    retrySemWait = FALSE;
    if (resetClock)
    {
       if ((ln_error = clock_gettime(CLOCK_REALTIME, &ts)) < 0)
          return ln_error; 
       ts.tv_sec += 3;
    }
    resetClock = FALSE;
    ln_error = sem_timedwait((sem_t *)semId, &ts);
    if (ln_error != 0) 
    {
      switch (errno)
      {
        case EINTR:
           retrySemWait = TRUE;
           break;
        case EINVAL:
           error = openStatsSemaphore(semId);
           if (error == 0)
               retrySemWait = TRUE;
           break;
        case ETIMEDOUT:
           cleanupDanglingSemaphore(FALSE);
           retrySemWait = TRUE;
           resetClock = TRUE;
           break;
        default:
           error = errno;
           break;
       }
      }
    }
    while (retrySemWait);
  }
  if (error == 0)
  {
    if (isShmDirty())
    {
       genLinuxCorefile("Shared Segment might be corrupted");
       Int32 ndRetcode = msg_mon_node_down(getCpu());
       sleep(30);
       NAExit(0);    // already made a core.
    }
    semPid_ = pid;
    semPidCreateTime_ = GetCliGlobals()->myStartTime();
    clock_gettime(CLOCK_REALTIME, &lockingTimestamp_);
  }
  return error;
}

void StatsGlobals::releaseStatsSemaphore(Long &semId, pid_t pid, 
         short savedPriority, short savedStopMode, NABoolean canAssert)
{
  short error = 0;
  pid_t tempPid;
  NABoolean tempIsBeingUpdated;
  Int64 tempSPCT;
  ex_assert(semPid_ != -1 && semPid_ == pid, "SemPid_ is -1 or semPid_ != pid");
  ex_assert(semPidCreateTime_ == GetCliGlobals()->myStartTime(), 
            "semPidCreateTime_ unexpected.");
  tempPid = semPid_;
  tempIsBeingUpdated = isBeingUpdated_;
  tempSPCT = semPidCreateTime_;
  semPid_ = -1;
  semPidCreateTime_ = 0;
  isBeingUpdated_ = FALSE;
  Int32 ln_error = sem_post((sem_t *)semId);
  if (ln_error == -1)
     error = errno;
  if (error != 0)
  {
    semPid_ = tempPid;
    isBeingUpdated_ = tempIsBeingUpdated;
    semPidCreateTime_ = tempSPCT;
  }
  ex_assert(error == 0, "sem_post failed");
}

short StatsGlobals::releaseAndGetStatsSemaphore(Long &semId, pid_t pid, 
        pid_t releasePid,
        short &savedPriority, short &savedStopMode, NABoolean shouldTimeout)
{
   short error = 0;
   pid_t tempPid;
   Int32 ln_error;

   ex_assert(releasePid != -1, "release pid is -1");
   if (semPid_ == releasePid)
   {
      tempPid = semPid_;
      semPid_ = -1;
      errorSemPid_ = tempPid ;
      releasingSemPid_ = pid;
      clock_gettime(CLOCK_REALTIME, &releasingTimestamp_);
      ln_error = sem_post((sem_t *)semId);
      if (ln_error == -1)
      {
         semPid_ = tempPid;
         releasingSemPid_ = -1;
         error = errno;
         return error;
      }
   }
   error = getStatsSemaphore(semId, pid, savedPriority, savedStopMode,
                          shouldTimeout); 
   return error;
} 

StmtStats *StatsGlobals::addStmtStats(NAHeap * heap,
				      pid_t pid, 
				      char *queryId, Lng32 queryIdLen,
                               char *sourceStr, Lng32 sourceLength)
{
  StmtStats *ss;
  
  ss = new (heap) StmtStats(heap, pid,
			    queryId, queryIdLen, NULL, -1, sourceStr, 
                                   sourceLength, sourceLength, TRUE);
  return ss;
}

short StatsGlobals::removeQuery(pid_t pid, StmtStats *stmtStats, 
      NABoolean removeAlways, NABoolean globalScan, NABoolean retainStats)
{
  short retcode = 1;
  NAHeap *heap = stmtStats->getHeap();
  ExMasterStats *masterStats;
  if (stmtStats->aqrInProgress() && !removeAlways)
    return 1;

/*
 MergedStats (ExStatisticsArea has virutal functions) 
 is allocated from SSMP and it should be deallocated from SSMP.

Hence, removeQuery called from MASTER and ESPs will not deallocate StmtStats
 by calling removeQuery with removeAlways set to FALSE. Once the mergedStats_ 
 is shipped to the collector SSMP will delete the StmtStats. This way the 
 collector process will have the chance to collect the completed stats
*/
  masterStats = stmtStats->getMasterStats();
  if ((retainStats && stmtStats->isMaster() &&
           masterStats != NULL && 
           masterStats->getCollectStatsType() != (UInt16)ComTdb::NO_STATS &&
           masterStats->getCollectStatsType() != (UInt16)ComTdb::ALL_STATS)
      || stmtStats->isStmtStatsUsed()
      || (!removeAlways && stmtStats->getMergedStats() != NULL)
      || (!removeAlways && stmtStats->isWMSMonitoredCliQuery()))
   {
      stmtStats->setTobeGCed();
      if (retainStats)
      {
        if (masterStats != NULL)
        {
          masterStats->setStmtState(Statement::PROCESS_ENDED_);
          masterStats->setEndTimes(TRUE);
        }
        stmtStats->setMergeReqd(TRUE);
      }
   }
   else
   {
     if (stmtStats->canbeGCed() || stmtStats->getMergedStats() == NULL)
     {
       if (!stmtStats->isDeleteError()) 
       {
         stmtStats->setDeleteError(TRUE);
         if (globalScan)
           stmtStatsList_->remove();
         else
           stmtStatsList_->remove(stmtStats->getQueryId(), stmtStats->getQueryIdLen(),
                      stmtStats);
         stmtStats->deleteMe();
         heap->deallocateMemory(stmtStats);
         retcode = 0;
       }
     }
     else 
     {
       stmtStats->setTobeGCed();
       if (retainStats)
       {
         if (masterStats != NULL)
         {
           masterStats->setStmtState(Statement::PROCESS_ENDED_);
           masterStats->setEndTimes(TRUE);
         }
         stmtStats->setMergeReqd(TRUE);
       }
     }
  }

  // Remove the heap if there is no memory allocated in case of removeAlways
  // is set to TRUE. RemoveAlways should be set to TRUE only from SSMP
  if (removeAlways && retcode == 0)
  { 
    Lng32 totalSize = (Lng32)heap->getTotalSize();
    if (totalSize == 0)
    {
      // Don't call NADELETE, since NADELETE needs update to VFPTR table of NAHeap object
      heap->destroy();
      statsHeap_.deallocateMemory(heap);
      decProcessStatsHeaps();
    }
  }
  return retcode;
}

short StatsGlobals::openStatsSemaphore(Long &semId)
{
  short error = 0;

  sem_t *ln_semId = sem_open((const char *)getRmsSemName(), 0);
  if (ln_semId == SEM_FAILED)
  {
    if (errno == ENOENT)
    {
      ln_semId = sem_open((const char *)getRmsSemName(), O_CREAT, RMS_SEMFLAGS, 1);
      if (ln_semId == SEM_FAILED)
        error = errno;
    }
    else
      error = errno;
  }
  if (error == 0)
    semId = (Long)ln_semId;
  return error; 
}

// LCOV_EXCL_START
// I removed this method and rebuilt successfully.  It seems to 
// be dead code.  It is only to be extra cautious that I am not removing it
// for M5.
ExStatisticsArea *StatsGlobals::getStatsArea(char *queryId, Lng32 queryIdLen)
{
  StmtStats *ss;

  stmtStatsList_->position(queryId, queryIdLen);
  ss = (StmtStats *)stmtStatsList_->getNext();
  while (ss != NULL)
  {
    if (str_cmp(ss->getQueryId(), queryId, queryIdLen) == 0)
      return ss->getStatsArea();
    ss = (StmtStats *)stmtStatsList_->getNext();
  }
  return NULL;
}
// LCOV_EXCL_STOP

StmtStats *StatsGlobals::getMasterStmtStats(char *queryId, Lng32 queryIdLen, short activeQueryNum)
{
  StmtStats *ss;
  ExMasterStats *masterStats;
  short queryNum = 0;
  stmtStatsList_->position(queryId, queryIdLen);
  ss = (StmtStats *)stmtStatsList_->getNext();

  while (ss != NULL)
  {
    if (str_cmp(ss->getQueryId(), queryId, queryIdLen) == 0)
    {
      masterStats = ss->getMasterStats();
      if (masterStats != NULL)
      { 
        if (filterStmtStats(masterStats, activeQueryNum, queryNum))
          break;
      }
    }
    ss = (StmtStats *)stmtStatsList_->getNext();
  }
  return ss;
}

StmtStats *StatsGlobals::getStmtStats(char *queryId, Lng32 queryIdLen)
{
  StmtStats *ss;

  stmtStatsList_->position(queryId, queryIdLen);
  ss = (StmtStats *)stmtStatsList_->getNext();
  while (ss != NULL)
  {
    if (str_cmp(ss->getQueryId(), queryId, queryIdLen) == 0)
      break;
    ss = (StmtStats *)stmtStatsList_->getNext();
  }
  return ss;
}

StmtStats *StatsGlobals::getStmtStats(pid_t pid, short activeQueryNum)
{
  StmtStats *ss;
  ExMasterStats *masterStats;
  short queryNum = 0;

  stmtStatsList_->position();
  // Active Query if the pid is a master 
  while ((ss = (StmtStats *)stmtStatsList_->getNext()) != NULL)
  {
    if (ss->getPid() == pid)
    {
      if (ss->getStatsArea() == NULL) 
          continue;
      masterStats = ss->getMasterStats();
      if (masterStats != NULL)
      {
        if (filterStmtStats(masterStats, activeQueryNum, queryNum))
         break;
      }
      else
      {
        queryNum++;
        if (queryNum == activeQueryNum)
          break;
      }
    }
  }
  return ss;
}

StmtStats *StatsGlobals::getStmtStats(short activeQueryNum)
{
  StmtStats *ss;
  ExMasterStats *masterStats;
  short queryNum = 0;

  stmtStatsList_->position();
  // Only the active Query whose master is that CPU is returned
  while ((ss = (StmtStats *)stmtStatsList_->getNext()) != NULL)
  {
    if (ss->getStatsArea() == NULL || 
        (ss->getStatsArea() != NULL && 
          ss->getStatsArea()->getCollectStatsType() != ComTdb::ACCUMULATED_STATS &&
            ss->getStatsArea()->getCollectStatsType() != ComTdb::PERTABLE_STATS))
        continue;
    masterStats = ss->getMasterStats();
    if (masterStats != NULL)
    {
      if (filterStmtStats(masterStats, activeQueryNum, queryNum))
        break;
    }
  }
  return ss;
}

StmtStats *StatsGlobals::getStmtStats(pid_t pid, char *queryId, Lng32 queryIdLen, Lng32 fragId)
{
  StmtStats *ss;
  stmtStatsList_->position(queryId, queryIdLen);
  ss = (StmtStats *)stmtStatsList_->getNext();

  while (ss != NULL)
  {
    if (ss->getPid() == pid && str_cmp(ss->getQueryId(), queryId, queryIdLen) == 0 &&
            ss->getFragId() == fragId)
    {
      break;
    }
    ss = (StmtStats *)stmtStatsList_->getNext();
  }
  return ss;
}

void StatsGlobals::doFullGC()
{
  StmtStats *ss;

  stmtStatsList_->position();
  Int64 maxElapsedTime = 15 * 60 *  1000000; 
  Lng32 retcode;

  short stmtStatsGCed = 0;
  while ((ss = (StmtStats *)stmtStatsList_->getNext()) != NULL)
  {
    // If this is the master stats, and it's marked for GC, and it's been more than 15 minutes since
    // the stats were merged the last time, remove this master stats area.
    
    if ((ss->isMaster()) && 
        (ss->canbeGCed()) &&
        (NA_JulianTimestamp() - ss->getLastMergedTime() > maxElapsedTime))
    {
      retcode = removeQuery(ss->getPid(), ss, TRUE, TRUE);
      if (retcode == 0)
        stmtStatsGCed++;
    }
  }
  incStmtStatsGCed(stmtStatsGCed);
}

void StatsGlobals::cleanupOldSikeys(Int64 sikGcInterval)
{
  Int64 tooOld = NA_JulianTimestamp() - sikGcInterval;

  RecentSikey *recentSikey = NULL;
  recentSikeys_->position();

  while (NULL != (recentSikey = (RecentSikey *) recentSikeys_->getNext()))
  {
    if (recentSikey->revokeTimestamp_ < tooOld)
      recentSikeys_->remove();
  }

  return;
}


Lng32 StatsGlobals::registerQuery(ComDiagsArea &diags, pid_t pid, SQLQUERY_ID *query_id, Lng32 fragId,
                                 Lng32 tdbId, Lng32 explainTdbId, short statsCollectionType, Lng32 instNum, 
                                 ComTdb::ex_node_type tdbType,
                                 char *tdbName,
                                 Lng32 tdbNameLen)
{
  ProcessStats *processStats;
  NAHeap *heap;
  ExStatisticsArea *statsArea = NULL;
  ExOperStats *stat = NULL;
  StmtStats *ss;
  char *queryId;
  Lng32 queryIdLen;

  if (query_id == NULL  ||  query_id->name_mode != queryid_str
          || query_id->identifier == NULL  || query_id->identifier_len < 0)
  {
    diags << DgSqlCode(-CLI_INTERNAL_ERROR);
    return ERROR;
  }

  queryId = (char *)query_id->identifier;
  queryIdLen = query_id->identifier_len;

  // Check if process is registered
  if ((processStats = checkProcess(pid)) == NULL)
  {
    diags << DgSqlCode(-CLI_INTERNAL_ERROR);
    return ERROR;
  }
  heap = processStats->getHeap();
  // Check if the query is already registered
  if ((ss = getStmtStats(pid, queryId, queryIdLen, fragId)) != NULL)
  {
    diags << DgSqlCode(-CLI_INTERNAL_ERROR);
    return ERROR;
  }

  // Register the query
  if ((ss = addQuery(pid, queryId, queryIdLen, NULL, fragId)) == NULL)
  {
    diags << DgSqlCode(-CLI_INTERNAL_ERROR);
    return ERROR;
  }

  // allocate a FragRootOperStatsEntry
  if ((statsCollectionType == ComTdb::ALL_STATS) ||
      (statsCollectionType == ComTdb::OPERATOR_STATS) ||
      (statsCollectionType == ComTdb::PERTABLE_STATS))
  {
    statsArea = new(heap) ExStatisticsArea((NAMemory *)heap, 0, 
          (ComTdb::CollectStatsType)statsCollectionType);
    // Pass in tdbId as zero for ex_ROOT, this stats entry shouldn't
    // be shown when TDBID_DETAIL=<tdbId> is used.
    // However, this entry will be shown when DETAIL=-1 is used
    stat = new(heap) ExFragRootOperStats((NAMemory *)heap,
      (ComTdb::CollectStatsType)statsCollectionType, 
      (ExFragId)fragId, 0, explainTdbId, instNum, 
      ComTdb::ex_ROOT, (char *)"EX_ROOT", str_len("EX_ROOT"));
    ((ExFragRootOperStats *)stat)->setQueryId(ss->getQueryId(), ss->getQueryIdLen());
    statsArea->insert(stat);
    statsArea->setRootStats(stat);
    ss->setStatsArea(statsArea);
  }
  if (statsCollectionType == ComTdb::OPERATOR_STATS)
  {
    switch ((ComTdb::ex_node_type)tdbType)
    {
      default:
         stat = NULL;
         break;
     }
  }
  QueryIdInfo *queryIdInfo = new(heap) QueryIdInfo(ss, stat);
  processStats->setQueryIdInfo(queryIdInfo);
  // return the QueryIdInfo in the strucutre for easy access later
  query_id->handle = queryIdInfo;
  return SUCCESS; 
}

Lng32 StatsGlobals::deregisterQuery(ComDiagsArea &diags, pid_t pid, SQLQUERY_ID  *query_id, Lng32 fragId)
{
  ProcessStats *processStats;
  StmtStats *ss;

  char *queryId;
  Lng32 queryIdLen;

  if (query_id == NULL  ||  query_id->name_mode != queryid_str
          || query_id->identifier == NULL  || query_id->identifier_len < 0)
  {
    diags << DgSqlCode(-CLI_INTERNAL_ERROR);
    return ERROR;
  }

  queryId = (char *)query_id->identifier;
  queryIdLen = query_id->identifier_len;

  // Check if process is registered
  if ((processStats = checkProcess(pid)) == NULL)
  {
    diags << DgSqlCode(-CLI_INTERNAL_ERROR);
    return ERROR;
  }

  if (processStats->getQueryIdInfo() != query_id->handle)
  {
    diags << DgSqlCode(-CLI_INTERNAL_ERROR);
    return ERROR;
  }

  // Check if the query is already registered
  if ((ss = getStmtStats(pid, queryId, queryIdLen, fragId)) == NULL)
  {
    diags << DgSqlCode(-CLI_INTERNAL_ERROR);
    return ERROR;
  }
  ss->setDeleteStats(TRUE);
  if (removeQuery(pid, ss))
  {
    diags << DgSqlCode(-CLI_INTERNAL_ERROR);
    return ERROR;
  }
  processStats->setQueryIdInfo(NULL);
  query_id->handle = NULL;
  return SUCCESS;
}

Lng32 StatsGlobals::updateStats(ComDiagsArea &diags, SQLQUERY_ID *query_id, void *operatorStats,
                     Lng32 operatorStatsLen)
{
  Lng32 retcode = 0;
  char *queryId;
  Lng32 queryIdLen;
  QueryIdInfo *queryIdInfo;

  if (query_id == NULL  ||  query_id->name_mode != queryid_str
          || query_id->identifier == NULL  || query_id->identifier_len < 0
          || query_id->handle == 0)
  {
    diags << DgSqlCode(-CLI_INTERNAL_ERROR);
    return ERROR;
  }

  queryId = (char *)query_id->identifier;
  queryIdLen = query_id->identifier_len;
  queryIdInfo = (QueryIdInfo *)query_id->handle;   

  ExFragRootOperStats *rootStats = NULL;
  if (queryIdInfo->ss_->getStatsArea() != NULL)
     rootStats = (ExFragRootOperStats *)queryIdInfo->ss_->getStatsArea()->getRootStats();
  
  if (str_cmp(queryIdInfo->eye_catcher_, QUERYID_INFO_EYE_CATCHER, 4) != 0)
  {
    diags << DgSqlCode(-CLI_INTERNAL_ERROR);
    return ERROR;
  }  
  switch (queryIdInfo->operStats_->getTdbType())
  {
     default:
        retcode = -1;
        break;
  }
  if (retcode < 0)
     diags << DgSqlCode(-CLI_INTERNAL_ERROR);
  return retcode;
}

#ifndef __EID
Lng32 StatsGlobals::getSecInvalidKeys(
                          CliGlobals * cliGlobals,
                          Int64 lastCallTimestamp,
                          SQL_SIKEY siKeys[],
                          Int32 maxNumSiKeys,
                          Int32 *returnedNumSiKeys)
{
  Lng32 retcode = 0;
  short savedPriority;
  short savedStopMode;
  short error = getStatsSemaphore(cliGlobals->getSemId(), cliGlobals->myPin(), 
                           savedPriority, savedStopMode,
                           FALSE); 
  ex_assert(error == 0, "getStatsSemaphore() returned an error");
  Int32 numToReturn = 0;
  RecentSikey *recentSikey = NULL;
  recentSikeys_->position();
  while (NULL != (recentSikey = (RecentSikey *) recentSikeys_->getNext()))
  {
    if (recentSikey->revokeTimestamp_ > lastCallTimestamp)
    {
      numToReturn++;
      if (numToReturn <= maxNumSiKeys)
        siKeys[numToReturn-1] = recentSikey->s_;
    }
  }
  *returnedNumSiKeys = numToReturn;
  if (numToReturn > maxNumSiKeys)
    retcode = -CLI_INSUFFICIENT_SIKEY_BUFF ;

  releaseStatsSemaphore(cliGlobals->getSemId(), cliGlobals->myPin(),
                        savedPriority, savedStopMode);
  return retcode;
}

void StatsGlobals::mergeNewSikeys(Int32 numSikeys, SQL_SIKEY sikeys[])
{
  newestRevokeTimestamp_ = NA_JulianTimestamp();
  for (Int32 i=0; i < numSikeys; i++)
  {
    bool updatedExistingRecentKey = false;
    recentSikeys_->position((char *) &sikeys[i], sizeof(sikeys[i]));
    RecentSikey *existingRecentKey = NULL;
    while (NULL != 
           (existingRecentKey = (RecentSikey *) recentSikeys_->getNext()))
    {
      if ((existingRecentKey->s_.subject == sikeys[i].subject)           &&
          (existingRecentKey->s_.object  == sikeys[i].object )           &&
          (existingRecentKey->s_.operation[0] == sikeys[i].operation[0]) &&
          (existingRecentKey->s_.operation[1] == sikeys[i].operation[1]))
      {
        existingRecentKey->revokeTimestamp_ = newestRevokeTimestamp_;
        updatedExistingRecentKey = true;
        break;
      }
    }
    if (!updatedExistingRecentKey)
    {
      RecentSikey *newRecentKey = new(&statsHeap_) RecentSikey;
      newRecentKey->s_ = sikeys[i];
      newRecentKey->revokeTimestamp_ = newestRevokeTimestamp_;
      recentSikeys_->insert((char *) &newRecentKey->s_, 
                            sizeof(newRecentKey->s_), newRecentKey);
    }
  }
}

#endif

ProcessStats:: ProcessStats(NAHeap *heap, short nid, pid_t pid)
   : heap_(heap)
   , stats_(NULL)
   , queryIdInfo_(NULL)
{
   exProcessStats_ = new (heap_) ExProcessStats(heap_, nid, pid);
}

ProcessStats::~ProcessStats()
{
   if (queryIdInfo_ != NULL)
   {
      NADELETE(queryIdInfo_, QueryIdInfo, heap_);
      queryIdInfo_ = NULL;
   }
   if (exProcessStats_ != NULL)
   {
      NADELETE(exProcessStats_, ExProcessStats, heap_);
      exProcessStats_ = NULL;
   }
}

inline size_t ProcessStats::getExeMemHighWM() 
  { return exProcessStats_->getExeMemHighWM(); }
inline size_t ProcessStats::getExeMemAlloc()  
  { return exProcessStats_->getExeMemAlloc(); }
inline size_t ProcessStats::getExeMemUsed()   
  { return exProcessStats_->getExeMemUsed(); }
inline size_t ProcessStats::getIpcMemHighWM() 
  { return exProcessStats_->getIpcMemHighWM(); }
inline size_t ProcessStats::getIpcMemAlloc()  
  { return exProcessStats_->getIpcMemAlloc(); }
inline size_t ProcessStats::getIpcMemUsed() 
  { return exProcessStats_->getIpcMemUsed(); }


void ProcessStats::updateMemStats(NAHeap *exeHeap, NAHeap *ipcHeap)
{
    if (exProcessStats_ != NULL)
       exProcessStats_->updateMemStats(exeHeap, ipcHeap);
}

void ProcessStats::setStatsArea(ExStatisticsArea *stats)
{
  if (stats_ != NULL)
  {
    NADELETE(stats, ExStatisticsArea, heap_);
  }
  stats_ = stats;

}

StmtStats *ProcessStats::addQuery(pid_t pid, char *queryId, Lng32 queryIdLen,
                                    void *backRef, Lng32 fragId,
                                    char *sourceStr, Lng32 sourceLen, Lng32 sqlSourceLen, 
                                    NABoolean isMaster)
{
  StmtStats *ss;

  ss = new (heap_) StmtStats(heap_, pid,
          queryId, queryIdLen, backRef, fragId, sourceStr, sourceLen, sqlSourceLen, isMaster);
  return ss;
}

StmtStats::StmtStats(NAHeap *heap, pid_t pid, char *queryId, Lng32 queryIdLen, 
                     void *backRef, Lng32 fragId,
                     char *sourceStr, Lng32 sourceStrLen, Lng32 sqlStrLen,
                     NABoolean isMaster)
      :heap_(heap),
      pid_(pid),
      stats_(NULL),
      EspProcHandle_(NULL),
      refCount_(0),
      fragId_(fragId)
{

  queryId_ = new (heap_) char[queryIdLen+1];
  str_cpy_all(queryId_, queryId, queryIdLen);
  queryId_[queryIdLen] = '\0';
  queryIdLen_= queryIdLen;
  if (isMaster)
    masterStats_ = new (heap_) ExMasterStats(heap_, sourceStr, sourceStrLen, sqlStrLen, queryId_, queryIdLen_);
  else
    masterStats_ = NULL;
  lastMergedTime_ = 0;
  mergedStats_ = NULL;
  flags_ = 0;
  setMaster(isMaster);
  backRef_ = backRef;
  updateChildQid_ = FALSE;
}

StmtStats::~StmtStats()
{
  deleteMe();
}

void StmtStats::deleteMe()
{
// in case of Linux, create tempStats to do fixup
// since vptr table will vary from one instance to another
// of the same program (mxssmp)
  ExStatisticsArea tempStats(heap_);
  if (stats_ != NULL && deleteStats())
  {
    stats_->fixup(&tempStats); 
    NADELETE(stats_, ExStatisticsArea, stats_->getHeap());
    stats_ = NULL;
  }

  if (masterStats_ != NULL)
  {
    if (mergedStats_ != NULL) // Means that masterStats_ is being deleted 
                              // is not in the same process as the creator
    {
// in case of LINUX always create a temp ExMasterStats to fixup
      {
        ExMasterStats masterStats;
        masterStats_->fixup(&masterStats);
        NADELETE(masterStats_, ExMasterStats, masterStats_->getHeap());
        masterStats_ = NULL;
      }
    }
    else
    {
      NADELETE(masterStats_, ExMasterStats, masterStats_->getHeap());
      masterStats_ = NULL;
    }
  }
  if (mergedStats_ != NULL)
  {
    mergedStats_->fixup(&tempStats);  
    NADELETE(mergedStats_, ExStatisticsArea, mergedStats_->getHeap());
    mergedStats_ = NULL;
  }

  NADELETEBASIC(queryId_, heap_);
  return;
}

void StmtStats::setStatsArea(ExStatisticsArea *stats)
{ 
  if (! isMaster())
  {
    stats_ = stats;
    return;
  }
  if (stats == NULL)
  {
    if (stats_ != NULL)
    {
      masterStats_ = stats_->getMasterStats();
      stats_->setMasterStats(NULL);
    }
  }
  else
  {
    if (stats_ == NULL)
    {
      masterStats_->setCollectStatsType(stats->getCollectStatsType());
      stats->setMasterStats(masterStats_);
      masterStats_ = NULL;
    }
    else
    {
      stats->setMasterStats(stats_->getMasterStats());
      stats_->setMasterStats(NULL);
      // delete stats_ if the flag is set. Otherwise, someone else is 
      // referring to this area, and will delete it later on
      if (deleteStats())
      {
        NADELETE(stats_, ExStatisticsArea, stats_->getHeap());
        setDeleteStats(FALSE);
      }    
    }
  }
  stats_ = stats;
}
  
void StmtStats::setMergedStats(ExStatisticsArea *stats)
{
  if (stats == mergedStats_)
    return;
  if (mergedStats_ != NULL)
  {
    mergedStats_->fixup(stats);
    NADELETE(mergedStats_, ExStatisticsArea, mergedStats_->getHeap());
  }
  mergedStats_ = stats;
  lastMergedTime_ = NA_JulianTimestamp();
  setMergeReqd(FALSE);
}


ExMasterStats *StmtStats::getMasterStats()
{ 
  if (masterStats_ == NULL)
  {
    if (stats_ != NULL)
      return stats_->getMasterStats();
    else
      return NULL;
  }
  else
    return masterStats_; 
}

void StmtStats::reuse(void *backRef)
{
  setTobeGCed(FALSE);
  setMergeReqd(TRUE);
  if (masterStats_ == NULL && stats_ != NULL)
  {
    masterStats_ = stats_->getMasterStats();
    stats_->setMasterStats(NULL);
  }
  if (masterStats_ != NULL)
    masterStats_->reuse();
  if (stats_ != NULL)
  {
    if (deleteStats())
    {
      NADELETE(stats_, ExStatisticsArea, stats_->getHeap());
      setDeleteStats(FALSE);
      stats_ = NULL;
    }
  }
  backRef_ = backRef;
}

void StmtStats::setParentQid(char *parentQid, Lng32 parentQidLen, 
           char *parentQidSystem, Lng32 parentQidSystemLen, short myCpu, short myNodeId)
{
  short parentQidCpu;
  short parentQidNodeId;
  Int64 value = 0;
  Lng32 retcode;
  getMasterStats()->setParentQid(parentQid, parentQidLen);
  getMasterStats()->setParentQidSystem(parentQidSystem, parentQidSystemLen);
  if (parentQid != NULL)
  {
    retcode = ComSqlId::getSqlQueryIdAttr(ComSqlId::SQLQUERYID_SEGMENTNUM,
			        parentQid,
			        parentQidLen,
			        value,
			        NULL);
    if (retcode == 0)
      parentQidNodeId = (short)value;
    else
      parentQidNodeId = -1;

    retcode = ComSqlId::getSqlQueryIdAttr(ComSqlId::SQLQUERYID_CPUNUM,
			        parentQid,
			        parentQidLen,
			        value,
			        NULL);
    if (retcode == 0)
      parentQidCpu = (short)value;
    else
      parentQidCpu = -1;
    if (parentQidCpu == myCpu && parentQidNodeId == myNodeId)
      updateChildQid_ = TRUE;
  }
}

void StatsGlobals::cleanup_SQL(
                 pid_t pidToCleanup,
                 pid_t myPid
                 )
{
  short savedPriority = 0;
  short savedStopMode = 0;
  if (statsArray_  && statsArray_[pidToCleanup].processId_ == 0)
  {
    // It is possible that process died before it registered
    // in the shared segment. 
    statsArray_[pidToCleanup].removedAtAdd_ = FALSE;
    return;
  }
  else if ((semPid_ == pidToCleanup) && 
            (statsArray_[pidToCleanup].removedAtAdd_))
  {
    // New process got here before this death message. It does the 
    // cleanup. The semPid_ is held by a new incarnation of the pidToCleanup.
    statsArray_[pidToCleanup].removedAtAdd_ = FALSE;
    return;
  }

  if (myPid != getSsmpPid())
     return;
  Long semId = ssmpProcSemId_;
  short error =
                releaseAndGetStatsSemaphore(
                  semId, myPid, pidToCleanup,
                  savedPriority, savedStopMode, FALSE /*shouldTimeout*/);
  ex_assert(error == 0, 
            "releaseAndGetStatsSemaphore() returned an error");

  removeProcess(pidToCleanup);
  releaseStatsSemaphore(
                  semId, myPid,
                  savedPriority, savedStopMode);
}
void StatsGlobals::updateMemStats(pid_t pid, 
          NAHeap *exeHeap, NAHeap *ipcHeap)
{
   ProcessStats *processStats = checkProcess(pid);
   if (processStats != NULL)
       processStats->updateMemStats(exeHeap, ipcHeap);  
}

ExProcessStats* StatsGlobals::getExProcessStats(pid_t pid)
{
   ProcessStats *processStats = checkProcess(pid);
   if (processStats != NULL)
      return processStats->getExProcessStats();
   else
      return NULL;
}


void StatsGlobals::getMemOffender(ExStatisticsArea *statsArea,
                           size_t filter)
{
    pid_t pid; 
    size_t memToCompare;
    ProcessStats *processStats;
    ExProcessStats *exProcessStats; 
    size_t memThreshold;

    for (pid = 0 ; pid < maxPid_ ; pid++)
    {
       if ((processStats = statsArray_[pid].processStats_) == NULL)
          continue;
       switch(statsArea->getSubReqType())
       {
          case SQLCLI_STATS_REQ_MEM_HIGH_WM:
             memThreshold  = filter << 20;
             memToCompare  = processStats->getExeMemHighWM()
                             + processStats->getIpcMemHighWM();
             break;
          case SQLCLI_STATS_REQ_MEM_ALLOC:
             memThreshold  = filter << 20;
             memToCompare = processStats->getExeMemAlloc() +
                         processStats->getIpcMemAlloc();
             break;
          case SQLCLI_STATS_REQ_PFS_USE:
             memToCompare = processStats->getExProcessStats()->getPfsCurUse();
             memThreshold = (size_t)((double) filter *
                               (double) processStats->getExProcessStats()->getPfsSize() 
                               / 100);
             break;
          default:
             continue;
       }
       if (memToCompare > 0 && memToCompare >= memThreshold)
       {
           exProcessStats = new (statsArea->getHeap())
                                ExProcessStats(statsArea->getHeap());
           exProcessStats->copyContents(processStats->getExProcessStats());
           statsArea->insert(exProcessStats);
       }
   } 
} 
       
StatsGlobals * shareStatsSegment(Int32 &shmid, NABoolean checkForSSMP)
{
  void *statsGlobalsAddr = NULL;
  StatsGlobals * statsGlobals;
  long enableHugePages;
  int shmFlag = RMS_SHMFLAGS;
  char *envShmHugePages = getenv("SQ_RMS_ENABLE_HUGEPAGES");
  if (envShmHugePages != NULL)
  {
     enableHugePages = (long) str_atoi(envShmHugePages,
                         str_len(envShmHugePages));
     if (enableHugePages > 0)
        shmFlag =  shmFlag | SHM_HUGETLB;
  }

  if ((shmid = shmget((key_t) getStatsSegmentId(),
			     0,  // size doesn't matter unless we are creating.
                             shmFlag )) == -1)
  {
     return NULL;
  }
  if ((statsGlobalsAddr = shmat(shmid, getRmsSharedMemoryAddr(), 0))
                == (void *)-1)
  {
     return NULL;
  }
  statsGlobals = (StatsGlobals *)statsGlobalsAddr;
  if (statsGlobals != NULL)
  {
    short i = 0;
    while (i < 3 && statsGlobals != (StatsGlobals *)statsGlobals->getStatsSharedSegAddr())
    {
      DELAY(100);
      i++;
    }
    if (statsGlobals->getStatsSharedSegAddr() != NULL)
    {
        ex_assert(statsGlobals == (StatsGlobals *)statsGlobals->getStatsSharedSegAddr(), 
        "Stats Shared segment can not be shared at the created addresss");
    }
    else 
       statsGlobals = NULL;
    if (statsGlobals != NULL && statsGlobals->IsSscpInitialized() != TRUE)
       statsGlobals = NULL;
  }
  return statsGlobals;
}

short getMasterCpu(char *uniqueStmtId, Lng32 uniqueStmtIdLen, char *nodeName, short maxLen, short &cpu)
{
  Int32 nodeNumber = 0;
  cpu = 0;
  short rc;
  Lng32 retcode;

  Int64 value = 0;
  retcode = ComSqlId::getSqlQueryIdAttr(ComSqlId::SQLQUERYID_SEGMENTNUM,
			      uniqueStmtId,
			      uniqueStmtIdLen,
			      value,
			      NULL);
  if (retcode == 0)
    nodeNumber = (Int32)value;
  else
    return -1;

  retcode = ComSqlId::getSqlQueryIdAttr(ComSqlId::SQLQUERYID_CPUNUM,
			      uniqueStmtId,
			      uniqueStmtIdLen,
			      value,
			      NULL);
  if (retcode == 0)
    cpu = (short)value;
  else
    return -1;

  short len;
  rc = NODENUMBER_TO_NODENAME_(nodeNumber,
              nodeName,
              maxLen-1,
              &len);
  if (rc == 0)
  {
    nodeName[len] = '\0';
    return 0;
  }
  else
    return -1;
}   

short getStmtNameInQid(char *uniqueStmtId, Lng32 uniqueStmtIdLen, char *stmtName, short maxLen)
{
  Lng32 retcode;
  Int64 value = maxLen;

  retcode = ComSqlId::getSqlQueryIdAttr(ComSqlId::SQLQUERYID_STMTNAME,
			      uniqueStmtId,
			      uniqueStmtIdLen,
			      value,
			      stmtName);
  if (retcode == 0)
    stmtName[value] = '\0';
  return (short)retcode;
}   

NABoolean filterStmtStats(ExMasterStats *masterStats, short activeQueryNum, short &queryNum)
{
  NABoolean queryFound = FALSE;
  
  ex_assert(masterStats != NULL, "MasterStats can't be null");

  switch (activeQueryNum)
  {
  case RtsQueryId::ANY_QUERY_:
    queryFound = TRUE;
    break;
  case RtsQueryId::ALL_ACTIVE_QUERIES_:
    if ((masterStats->getExeEndTime() == -1 && masterStats->getExeStartTime() != -1)
      || (masterStats->getCompEndTime() == -1 && masterStats->getCompStartTime() != -1))
      queryFound = TRUE;
    break;
  default:
    if ((masterStats->getExeEndTime() == -1 && masterStats->getExeStartTime() != -1)
      || (masterStats->getCompEndTime() == -1 && masterStats->getCompStartTime() != -1))
      queryNum++;
    if (queryNum == activeQueryNum)
      queryFound = TRUE;
    break;
  }
  return queryFound;
}


SB_Phandle_Type *getMySsmpPhandle()
{
  CliGlobals *cliGlobals = GetCliGlobals();
  if (cliGlobals->getStatsGlobals())
     return &cliGlobals->getStatsGlobals()->ssmpProcHandle_;
  else
     return NULL;
}

short getRTSSemaphore()
{
  // 0 means NO stats globals or not locked
  // 1 means locked
  short retcode = 0;
  CliGlobals *cliGlobals = GetCliGlobals();
  StatsGlobals *statsGlobals = NULL;
  short error;
  short savedPriority, savedStopMode;
  if (cliGlobals) 
    statsGlobals = cliGlobals->getStatsGlobals();
  if (statsGlobals)
  {
     if (statsGlobals->semPid_ != cliGlobals->myPin())
     { 
       error = statsGlobals->getStatsSemaphore(cliGlobals->getSemId(),
            cliGlobals->myPin(), savedPriority, savedStopMode, FALSE); 
       ex_assert(error == 0, "getStatsSemaphore() returned an error");
       retcode = 1;
     }
  }
  return retcode;
}

#ifndef __EID
void updateMemStats()
{
   CliGlobals *cliGlobals = GetCliGlobals();
   StatsGlobals *statsGlobals = NULL;
   if (cliGlobals)
      statsGlobals = cliGlobals->getStatsGlobals();
   if (statsGlobals)
      statsGlobals->updateMemStats(cliGlobals->myPin(),
            cliGlobals->getExecutorMemory(), cliGlobals->getProcessIpcHeap());
}
#endif

void releaseRTSSemaphore()
{
  short savedStopMode = 0;
  CliGlobals *cliGlobals = GetCliGlobals();
  StatsGlobals *statsGlobals = NULL;
  if (cliGlobals) 
    statsGlobals = cliGlobals->getStatsGlobals();
  if (statsGlobals)
  {
    if (statsGlobals->semPid_ == cliGlobals->myPin())
    {
      // Though the semPid_ is saved in abortedSemPid_, you need to look at
      // the stack trace if the process is being aborted. releaseRTSSemaphore
      // will be called even when the process is not aborting
      statsGlobals->setAbortedSemPid();
      statsGlobals->releaseStatsSemaphore(cliGlobals->getSemId(), cliGlobals->myPin(),
            (short)cliGlobals->myPriority(), savedStopMode);
    }
  }
}

NABoolean checkIfRTSSemaphoreLocked()
{
  NABoolean retcode = FALSE;

  CliGlobals *cliGlobals = GetCliGlobals();
  StatsGlobals *statsGlobals = NULL;

  if (cliGlobals) 
    statsGlobals = cliGlobals->getStatsGlobals();
  if (statsGlobals)
  {
    if (statsGlobals->semPid_ == cliGlobals->myPin())
    {
      statsGlobals->setShmDirty();
      retcode = TRUE;
    } 
  }
  return retcode;
}

char *gRmsSemName_ = NULL;

char *getRmsSemName()
{
   if (gRmsSemName_ == NULL)
   {
      gRmsSemName_ = new char[100];
      sprintf(gRmsSemName_, "%s%d.%d", RMS_SEM_NAME, getuid(), 
              getStatsSegmentId());
   }
   return gRmsSemName_;
}

void *gRmsSharedMemoryAddr_ = NULL;

void *getRmsSharedMemoryAddr()
{
   const char *rmsAddrStr; 
   long rmsAddr;
   char *endPtr;
   if (gRmsSharedMemoryAddr_ == NULL)
   {
      if ((rmsAddrStr = getenv("RMS_SHARED_MEMORY_ADDR")) == NULL)
         gRmsSharedMemoryAddr_ = (void *)RMS_SHARED_MEMORY_ADDR;
      else
      {
        rmsAddr = strtol( rmsAddrStr, &endPtr, 16);
        if (*endPtr == '\0')
           gRmsSharedMemoryAddr_ = (void *)rmsAddr;
        else
           ex_assert(0, "Invalid RMS Shared Memory Address (RMS_SHARED_MEMORY_ADDR)");
      }
        
   }
   return gRmsSharedMemoryAddr_;

}


short getDefineNumericValue(char * defineName, short *numValue)
{
  short defineValueLen = 0;
  short error = 0;
  *numValue = 0;
  return error;
}
