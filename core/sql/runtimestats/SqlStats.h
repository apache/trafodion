/**********************************************************************
/ @@@ START COPYRIGHT @@@
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
#ifndef SQLSTATS_H
#define SQLSTATS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlStats.h
 * Description:  
 * Created:      2/28/06
 * Language:     C++
 *
 *****************************************************************************
 */
#include "seabed/fs.h"

#define STATS_INITAL_SEG_SIZE     4*1024*1024
#define STATS_MAX_SEG_SIZE        64*1024*1024

#define RMS_STORE_SQL_SOURCE_LEN    254

#define RMS_SHARED_MEMORY_ADDR    0x10000000 
// make the permission rw by owner only so we cannot attach to other 
// instances' segments
#define RMS_SHMFLAGS	          0600
#define RMS_SEM_NAME		  "/rms."
// outputs and returns semaphore name
extern char *gRmsSemName_;
extern void *gRmsSharedMemoryAddr_;
char *getRmsSemName(); 
void *getRmsSharedMemoryAddr();
// make the permission rw by owner only so we cannot use other 
// instances' seamphore
#define RMS_SEMFLAGS	          0600
class ExStatisticsArea;
class ProcessStats;
class HashQueue;
class SyncHashQueue;
class ExMasterStats;
class ExRMSStats;
class RecentSikey;
class StatsGlobals;
class ExOperStats;
class ExProcessStats;
class MemoryMonitor;

#include "rts_msg.h"
#include "ComTdb.h"
#include "SQLCLIdev.h"
#include "memorymonitor.h"

#define PID_MAX_DEFAULT     65536
#define PID_MAX_DEFAULT_MAX 131072
#define PID_MAX_DEFAULT_MIN 32768
#define PID_VIOLATION_MAX_COUNT 100

typedef struct GlobalStatsArray
{
  pid_t  processId_;
  SB_Verif_Type  phandleSeqNum_;
  ProcessStats  *processStats_;
} GlobalStatsArray;

class StmtStats
{
public:
  StmtStats(NAHeap *heap, pid_t pid, char *queryId, Lng32 queryIdLen, 
          void *backRef, Lng32 fragId,
          char *sourceStr = NULL, Lng32 sourceStrLen = 0, Lng32 sqlSrcStrLen = 0, 
          NABoolean isMaster = FALSE);
  ~StmtStats();
  void deleteMe();
  void setStatsArea(ExStatisticsArea *stats);
  NAHeap *getHeap() 
    { return heap_; }
  pid_t getPid() 
    { return pid_; }
  Lng32 getFragId()
    { return fragId_; }
  ExStatisticsArea *getStatsArea() 
    { return stats_; }
  Int64 getLastMergedTime() { return lastMergedTime_; }
  //unsigned short getLastMergedStatsType() { return lastMergedStatsType_; }
  ExStatisticsArea *getMergedStats() { return mergedStats_; }
  void setMergedStats(ExStatisticsArea *stats);
//  void setLastMergedStatsType(unsigned short type) { lastMergedStatsType_ = type; }
  NABoolean isMaster() 
  { 
    return (flags_ & IS_MASTER_) != 0; 
  }
  
  void setMaster(NABoolean v)      
  {
    (v ? flags_ |= IS_MASTER_ : flags_ &= ~IS_MASTER_); 
  }

  NABoolean canbeGCed() 
  {
    return (flags_ & CAN_BE_GCED_) != 0; 
  }
  
  void setTobeGCed(NABoolean v = TRUE)
  { 
     (v ? flags_ |= CAN_BE_GCED_ : flags_ &= ~CAN_BE_GCED_); 
  }

  void setMergeReqd(NABoolean v) 
  { 
    (v ? flags_ |= FORCE_MERGE_ : flags_ &= ~FORCE_MERGE_); 
  }

  NABoolean mergeReqd() 
  { 
    return (flags_ & FORCE_MERGE_) != 0; 
  }

  NABoolean deleteStats()
  {
    if (isMaster())
       return TRUE;
    else
       return FALSE;
  }
 
  NABoolean isStmtStatsUsed()
  {
    return (refCount_ > 0); 
  }

  void setStmtStatsUsed(NABoolean v)      
  {
    if (v)
      refCount_++;
    else
    {
      if (refCount_ > 0)
        refCount_--;
    }
  } 

  NABoolean isDeleteError()
  {
    return (flags_ & IS_DELETE_ERROR_) != 0; 
  }

  void setDeleteError(NABoolean v)      
  {
    (v ? flags_ |= IS_DELETE_ERROR_ : flags_ &= ~IS_DELETE_ERROR_); 
  } 
  NABoolean aqrInProgress() { return (flags_ & AQR_IN_PROGRESS_) != 0; }

  void setAqrInProgress(NABoolean v)
  { (v ? flags_ |= AQR_IN_PROGRESS_: flags_ &= ~AQR_IN_PROGRESS_); }
  NABoolean isWMSMonitoredCliQuery()
  {
    return (flags_ & WMS_MONITORED_CLI_QUERY_) != 0; 
  }

  void setWMSMonitoredCliQuery(NABoolean v)      
  {
    (v ? flags_ |= WMS_MONITORED_CLI_QUERY_ : flags_ &= ~WMS_MONITORED_CLI_QUERY_);
  }
  
  NABoolean calledFromRemoveQuery()
  {
    return (flags_ & CALLED_FROM_REMOVE_QUERY) != 0; 
  }

  void setCalledFromRemoveQuery(NABoolean v)      
  {
    (v ? flags_ |= CALLED_FROM_REMOVE_QUERY : flags_ &= ~CALLED_FROM_REMOVE_QUERY);
  }

  char *getQueryId() { return queryId_; }
  Lng32 getQueryIdLen() { return queryIdLen_; }
  ExMasterStats *getMasterStats();
  void reuse(void *backRef);
  void setParentQid(char *parentQid, Lng32 parentQidLen, char *parentQidSystem,
                    Lng32 parentQidSystemLen, short myCpu, short myNodeId);
  inline NABoolean updateChildQid() { return updateChildQid_; }
  void setExplainFrag(void *explainFrag, Lng32 len, Lng32 topNodeOffset);
  RtsExplainFrag *getExplainInfo() { return explainInfo_; }
  void deleteExplainFrag();
  ULng32 getFlags() const { return flags_; }
  NABoolean reportError(pid_t pid);
private:
  enum Flags
  {
    IS_MASTER_     = 0x0001,
    CAN_BE_GCED_   = 0x0002,
    NOT_USED_1     = 0x0004,
    FORCE_MERGE_   = 0x0008,
    STMT_STATS_USED_ = 0x0010, // Unused Flag
    IS_DELETE_ERROR_ = 0x0020,
    AQR_IN_PROGRESS_ = 0x0040,
    WMS_MONITORED_CLI_QUERY_=0x0080, // Is this a CLI query monitored by WMS
    CALLED_FROM_REMOVE_QUERY = 0x0100
  };

  NAHeap *heap_;
  pid_t pid_;
  char *queryId_;
  Lng32 queryIdLen_;
  ExMasterStats *masterStats_;
  ExStatisticsArea *stats_;
  Int64 lastMergedTime_;
  ExStatisticsArea *mergedStats_;
  ULng32 flags_;
  void *backRef_;    // This member is for the debug purposes to analyze halts
                     // It should never be used 
                     // It points to Statement object for master and ExEspStmtGlobals
                     // for Esps
  short refCount_;
  Lng32 fragId_;
  RtsExplainFrag *explainInfo_;
  NABoolean updateChildQid_;
};

#define QUERYID_INFO_EYE_CATCHER "QIDI"

class QueryIdInfo
{
friend class StatsGlobals;
public:
   QueryIdInfo()
   {
     str_cpy_all(eye_catcher_, QUERYID_INFO_EYE_CATCHER, 4);
     ss_ = NULL;
     operStats_ = NULL;
   }
   
   QueryIdInfo(StmtStats *ss, ExOperStats *operStats)
   {
     str_cpy_all(eye_catcher_, QUERYID_INFO_EYE_CATCHER, 4);
     ss_ = ss;
     operStats_ = operStats;
   }
protected:
   char eye_catcher_[4];
   StmtStats *ss_;
   ExOperStats *operStats_;
};


class ProcessStats
{
public:
  ProcessStats(NAHeap *heap, short nid, pid_t pid);
  ~ProcessStats();
  NAHeap *getHeap() 
  {
     return heap_;
  }

  void setStatsArea(ExStatisticsArea *stats);
  ExStatisticsArea *getStatsArea()
  {
    return stats_;
  }

  void setQueryIdInfo(QueryIdInfo *queryIdInfo)
  { 
    if (queryIdInfo_ != NULL)
       NADELETE(queryIdInfo_, QueryIdInfo, heap_);
    queryIdInfo_ = queryIdInfo; 
  }

  QueryIdInfo *getQueryIdInfo() 
  { return queryIdInfo_; }

  void updateMemStats(NAHeap *exeHeap, NAHeap *ipcHeap);
  inline size_t getExeMemHighWM();
  inline size_t getExeMemAlloc();
  inline size_t getExeMemUsed();
  inline size_t getIpcMemHighWM();
  inline size_t getIpcMemAlloc();
  inline size_t getIpcMemUsed();
  inline ExProcessStats *getExProcessStats() { return exProcessStats_; }
  StmtStats *addQuery(pid_t pid, char *queryId, Lng32 queryIdLen, void *backRef,
                      Lng32 fragId, char *sourceStr, Lng32 sourceStrLen, 
                      Lng32 sqlSourceLen, NABoolean isMaster);
private:
  NAHeap *heap_; 
  ExStatisticsArea *stats_ ; 
  QueryIdInfo *queryIdInfo_;
  ExProcessStats *exProcessStats_;
};

// The RecentSikey class is to support the CLI call which a compiler will
// use to see if its cache of queries and metadata needs to be updated
// because of a REVOKE.  These RecentSikey objects are organized in a 
// HashQueue in the StatsGlobals. When MXSSCP processes a 
// SecInvalidKeyRequest, it updates the structure, inserting new 
// RecentSikey values if needed, and setting the RecentSikey's 
// revokeTimestamp_.  The StatsGlobals semaphore protects the reading of 
// the table by the CLI as well as the updating by the MXSSCP.
// The MXSSCP performs maintenance on the RecentSikey HashQueue
// periodically, by removing entries that have have a revokeTimestamp_ 
// older than 24 hours.
class RecentSikey
{
public:
  SQL_QIKEY s_;
  Int64     revokeTimestamp_;
};

class StatsGlobals
{
public:
  StatsGlobals(void *baseAddr, short envType, Lng32 maxSegSize);
  static void* operator new (size_t size, void* loc = 0);
  void addProcess(pid_t pid, NAHeap *heap);
  void removeProcess(pid_t pid, NABoolean calledDuringAdd = FALSE);
  ProcessStats *checkProcess(pid_t pid);
  void setStatsArea(pid_t pid, ExStatisticsArea *stats)
  {
    if (statsArray_[pid].processStats_ != NULL)
      statsArray_[pid].processStats_->setStatsArea(stats);
  }
  
  ExStatisticsArea *getStatsArea(pid_t pid)
  {
    if (statsArray_[pid].processStats_ != NULL)
      return statsArray_[pid].processStats_->getStatsArea();
    else
      return NULL;
  }

  StmtStats *addQuery(pid_t pid, char *queryId, Lng32 queryIdLen, void *backRef,
                     Lng32 fragId, char *sourceStr = NULL, Lng32 sourceStrLen = 0,
                     NABoolean isMaster = FALSE);

  static StmtStats *addStmtStats(NAHeap * heap,
				 pid_t pid, char *queryId, Lng32 queryIdLen,
                                 char *sourceStr, Lng32 sourceStrLen);

  // returns 0 if query is deleted
  // returns 1 if query is not deleted because MergedStats is present

  short removeQuery(pid_t pid, StmtStats *stmtStats, 
                  NABoolean removeAlways = FALSE, 
                  NABoolean globalScan = FALSE,
                  NABoolean calledFromRemoveProcess = FALSE); 

  // global scan when the stmtList is positioned from begining and searched for pid
  int openStatsSemaphore(Long &semId);
  int getStatsSemaphore(Long &semId, pid_t pid);
  void releaseStatsSemaphore(Long &semId, pid_t pid);

  int releaseAndGetStatsSemaphore(Long &semId, 
       pid_t pid, pid_t releasePid);
  void cleanupDanglingSemaphore(NABoolean checkForSemaphoreHolder);
  void checkForDeadProcesses(pid_t myPid);
  SyncHashQueue *getStmtStatsList() { return stmtStatsList_; }
  ExStatisticsArea *getStatsArea(char *queryId, Lng32 queryIdLen);
  StmtStats *getMasterStmtStats(const char *queryId, Lng32 queryIdLen, short activeQueryNum);
  StmtStats *getStmtStats(char *queryId, Lng32 queryIdLen);
  StmtStats *getStmtStats(pid_t pid, short activeQueryNum);
  StmtStats *getStmtStats(short activeQueryNum);
  StmtStats *getStmtStats(pid_t pid, char *queryId, Lng32 queryIdLen, 
           Lng32 fragId);
  StmtStats *getStmtStats(short cpu, pid_t pid, Int64 timeStamp, Lng32 queryNumber);
  ExRMSStats *getRMSStats() { return rmsStats_; }
  void doFullGC();
  Lng32 registerQuery(ComDiagsArea &diags, pid_t pid, SQLQUERY_ID *queryId, 
                 Lng32 fragId, Lng32 tdbId, Lng32 explainTdbId, 
                 short statsCollectionType,
                 Lng32 instNum, ComTdb::ex_node_type tdbType,
                 char *tdbName, Lng32 tdbNameLen);
  Lng32 deregisterQuery(ComDiagsArea &diags, pid_t pid, SQLQUERY_ID *queryId, 
                  Lng32 fragId);
  Lng32 updateStats(ComDiagsArea &diags, SQLQUERY_ID *query_id, 
             void *operatorStats,
             Lng32 operatorstatsLen);

  void *getStatsSharedSegAddr() 
  {
    return statsSharedSegAddr_;
  }

  Lng32 getVersion()
  { 
    return version_;
  } 
  void getMemOffender(ExStatisticsArea *statsArea,
                           size_t  memThreshold);
  ExProcessStats *getExProcessStats(pid_t pid);
  enum 
  {
    // CURRENT_SHARED_OBJECTS_VERSION_ is used to ensure that all of the
    // RTS executables use structures in shared memory in the same way.
    // This value should be incremented whenever classes within this file
    // or classes in NAMemory.h are modified. If a merge from another stream
    // occurs and the following value is marked as a conflict, then this
    // value should be incremented. For release 2.4 development, this value
    // starts at 2400. For R2.93, the value starts are 2930 and for R2.5, 
    // the value starts at 2500.
    CURRENT_SHARED_OBJECTS_VERSION_ = 2511
  };
  void setSscpInitialized(NABoolean toggle)
  {
    isSscpInitialized_ = toggle;
  }
  NABoolean IsSscpInitialized()
  {
    return (isSscpInitialized_ == TRUE);
  }
  enum RTSEnvType
  {
    RTS_GLOBAL_ENV = 1,
    RTS_PRIVATE_ENV = 0
  };
  NABoolean globalRtsEnv() { return (NABoolean) rtsEnvType_; };
  inline short getStoreSqlSrcLen() { return storeSqlSrcLen_; }
  inline void setStoreSqlSrcLen(short len) { storeSqlSrcLen_ = len; }
  inline NAHeap *getStatsHeap() { return &statsHeap_; }
  inline pid_t getSemPid() { return semPid_; }
  inline pid_t getSsmpPid();
  inline Int64 getSsmpTimestamp();
  inline pid_t getConfiguredPidMax() { return configuredPidMax_; }
  inline void setSsmpDumpTimestamp(Int64 dumpTime) 
          { ssmpDumpedTimestamp_ = dumpTime; }
  inline Int64 getSsmpDumpTimestamp() 
          { return ssmpDumpedTimestamp_; }
  Int64 getLastGCTime();
  void setLastGCTime(Int64 gcTime) ;
  void incStmtStatsGCed(short inc) ;
  void incSsmpReqMsg(Int64 msgBytes);
  void incSsmpReplyMsg(Int64 msgBytes);
  void incSscpReqMsg(Int64 msgBytes);
  void incSscpReplyMsg(Int64 msgBytes);
  void setSscpOpens(short numSscps);
  void setSscpDeletedOpens(short numSscps);
  static const char *rmsEnvType(RTSEnvType envType);
  void setSscpTimestamp(Int64 timestamp);
  void setSsmpTimestamp(Int64 timestamp);
  void setSscpPid(pid_t pid);
  void setSsmpPid(pid_t pid);
  void setSscpPriority(short pri);
  void setSsmpPriority(short pri);
  void setRMSStatsResetTimestamp(Int64 timestamp); 
  void setNodesInCluster(short numNodes);
  void incProcessRegd();
  void decProcessRegd();
  void incProcessStatsHeaps();
  void decProcessStatsHeaps();
  inline short getCpu() { return cpu_; }
  inline short getNodeId() { return nodeId_; }
  inline void setAbortedSemPid()
      { abortedSemPid_ = semPid_; }
  Int64 getNewestRevokeTimestamp() const { return newestRevokeTimestamp_; }
  void cleanupOldSikeys(Int64 gcInterval);
  Lng32 getSecInvalidKeys(
                          CliGlobals * cliGlobals,
                          Int64 lastCallTimestamp,
                          SQL_QIKEY [],
                          Int32 maxNumSiKeys,
                          Int32 *returnedNumSiKeys);
  Int32 checkLobLock(CliGlobals *cliGlobals,char *&lobLockId);

  void mergeNewSikeys(Int32 numSikeys, 
                    SQL_QIKEY sikeys[]);
  MemoryMonitor *getMemoryMonitor() { return memMonitor_; }
  void createMemoryMonitor();

  void init();
  NABoolean isShmDirty() { return isBeingUpdated_; }
  void setShmDirty() { isBeingUpdated_ = TRUE; }
  void cleanup_SQL(pid_t pidToCleanup, pid_t myPid);
  void verifyAndCleanup(pid_t pidThatDied, SB_Int64_Type seqNum);

  void updateMemStats(pid_t pid, NAHeap *exeMem, NAHeap *ipcHeap);
  SB_Phandle_Type *getSsmpProcHandle() { return &ssmpProcHandle_; }
  SB_Phandle_Type *getSscpProcHandle() { return &sscpProcHandle_; }
  SyncHashQueue *getRecentSikeys() { return recentSikeys_; }
  SyncHashQueue *getLobLocks() { return lobLocks_;}
  void setSsmpProcSemId(Long semId) { ssmpProcSemId_ = semId; } 
  Long &getSsmpProcSemId() { return ssmpProcSemId_; } 
  void setSscpProcSemId(Long semId) { sscpProcSemId_ = semId; } 
  void setSeabedError(Int32 error) { seabedError_ = error; }
  NABoolean getInitError(pid_t pid, NABoolean &reportError );
private:
  void *statsSharedSegAddr_;
  Lng32 version_;             // A field used to prevent downrev compiler or other 
                             // incompatible programs to store objects in the
                             // shared memory
  Long sscpProcSemId_;
  SB_Phandle_Type  sscpProcHandle_;
  Long ssmpProcSemId_;
  SB_Phandle_Type ssmpProcHandle_;
  GlobalStatsArray *statsArray_;
  SyncHashQueue *stmtStatsList_;
  short cpu_;
  pid_t semPid_;    // Pid of the process that holds semaphore lock - This element is used for debugging purpose only
  Int64 semPidCreateTime_; // Creation timestamp - pid recycle workaround. 
  NAHeap statsHeap_;
  NABoolean isSscpInitialized_;
  short rtsEnvType_; // 1 - Global Environment
                     // 0 - Private environment
  short storeSqlSrcLen_;
  ExRMSStats *rmsStats_;
  short nodeId_;
  pid_t abortedSemPid_;
  pid_t errorSemPid_;
  pid_t releasingSemPid_;
  timespec releasingTimestamp_;
  timespec lockingTimestamp_;
  Int32 seabedError_;
  bool seabedPidRecycle_; //if true, then the most recent call to 
                          // cleanupDanglingSemaphore detected the semaphore
                          // holding process is gone, but its pid was 
                          // recycled.
  SyncHashQueue *recentSikeys_;
  Int64 newestRevokeTimestamp_;  // Allows CLI call w/o use if a semaphore.
  NABoolean isBeingUpdated_;
  pid_t pidToCheck_;
  pid_t maxPid_;
  Int64 ssmpDumpedTimestamp_;
  MemoryMonitor *memMonitor_;
  SyncHashQueue *lobLocks_;
  pid_t configuredPidMax_;
  Int64 pidViolationCount_;
};
StatsGlobals * shareStatsSegment(Int32 &shmid, NABoolean checkForSSMP = TRUE);
short getMasterCpu(char *uniqueStmtId, Lng32 uniqueStmtIdLen, char *nodeName, short maxLen, short &cpu);
short getStmtNameInQid(char *uniqueStmtId, Lng32 uniqueStmtIdLen, char *stmtName, short maxLen);
NABoolean filterStmtStats(ExMasterStats *masterStats, short activeQueryNum, short &queryNum);
short getRTSSemaphore();
void releaseRTSSemaphore();
SB_Phandle_Type *getMySsmpPhandle();
short getDefineNumericValue(char * defineName, short *numValue);
#endif
