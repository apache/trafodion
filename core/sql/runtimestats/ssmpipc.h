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
// File:         ssmpIpc.h
// Description:  Class declaration for SSCP IPC infrastructure
//
// Created:      5/08/2006
**********************************************************************/
#ifndef _SSMPIPC_H_
#define _SSMPIPC_H_

#include "Platform.h"
#include "Ipc.h"
#include "Collections.h"
#include "rts_msg.h"
#include "ComQueue.h"
#include "SqlStats.h"
#include "CancelBroker.h"

class StatsGlobals ;
class StmtStats ;
class HashQueue ;
class SscpClientMsgStream ;
class SsmpNewIncomingConnectionStream;
class ActiveQueryMgr;
class PendingQueryMgr;
class ExStatisticsArea;

typedef struct ServerId
{
  char  nodeName_[MAX_SEGMENT_NAME_LEN+1];
  short cpuNum_;
} ServerId;

/////////////////////////////////////////////////////////////
// class ExSsmpManager
/////////////////////////////////////////////////////////////
class ExSsmpManager
{
public:
  ExSsmpManager(IpcEnvironment *env);
  ~ExSsmpManager();
  IpcServer *getSsmpServer(NAHeap *heap, char *nodeName, short cpuNum, ComDiagsArea *&diagsArea);
  IpcEnvironment *getIpcEnvironment() { return env_; }
  void removeSsmpServer(char *nodeName, short cpuNum);
  void cleanupDeletedSsmpServers();
  IpcServerClass *getServerClass() { return ssmpServerClass_; }
private:
  IpcEnvironment       *env_;
  IpcServerClass       *ssmpServerClass_;
  HashQueue            *ssmps_;
  NAList<IpcServer *> *deletedSsmps_; // list of ssmp servers to be deleted
}; // ExSsmpManager

enum SuspendOrActivate {
    SUSPEND,
    ACTIVATE };

/////////////////////////////////////////////////////////////
// class SsmpGlobals
/////////////////////////////////////////////////////////////
class SsmpGlobals
{
public:

  SsmpGlobals(NAHeap *ssmpheap, IpcEnvironment *ipcEnv, 
              StatsGlobals *statsGlobals);
  ~SsmpGlobals();
  NAHeap *getHeap() { return heap_; }
  StatsGlobals *getStatsGlobals() { return statsGlobals_; }
  void releaseOrphanEntries() {}
  ULng32 allocateServers();
  IpcServer *allocateServer(char *nodeName, short nodeNameLen,  short cpuNum);
  void allocateServerOnNextRequest(char *nodeName, 
                                    short nodeNameLen,  
                                    short cpuNum);
  ULng32 deAllocateServer(char *nodeName, short nodeNameLen,  short cpuNum );
  void work();
  Int64 getStatsCollectionInterval() { return statsCollectionInterval_; }
  Int64 getStatsMergeTimeout() { return statsTimeout_; }
  Long &getSemId() { return semId_; }
  IpcEnvironment *getIpcEnv() { return ipcEnv_; }
  IpcSetOfConnections getRecipients() { return recipients_; }
  void addRecipients(SscpClientMsgStream *msgStream);
  NAHeap *getStatsHeap() { return statsHeap_; }
  Int32 myCpu() { return myCpu_; }
  pid_t myPin() { return myPin_; }
  NABoolean getForceMerge() { return forceMerge_; }
  Lng32 getNumDeallocatedServers() { return deallocatedSscps_->numEntries(); }
  inline NABoolean doingGC () { return doingGC_;}
  inline void setDoingGC(NABoolean value) { doingGC_ = value; }
  inline Lng32 getNumPendingSscpMessages() { return pendingSscpMessages_->numEntries(); }
  inline void finishPendingSscpMessages();
  inline void addPendingSscpMessage(SscpClientMsgStream *sscpClientMsgStream)
  {
    pendingSscpMessages_->insert(sscpClientMsgStream, sizeof(sscpClientMsgStream));
  }
  void removePendingSscpMessage(SscpClientMsgStream *sscpClientMsgStream);
  inline short getStoreSqlLen() { return storeSqlSrcLen_; }
  inline short getNumAllocatedServers() {return (short)sscps_->numEntries(); }
  inline void incSsmpReqMsg(Int64 msgBytes) { statsGlobals_->incSsmpReqMsg(msgBytes); }
  inline void incSsmpReplyMsg(Int64 msgBytes) { statsGlobals_->incSsmpReplyMsg(msgBytes); }
  void insertDeallocatedSscp(char *nodeName, short cpuNum);
  bool cancelQueryTree(char *queryId, Lng32 queryIdLen, 
                       CancelQueryRequest *request,
                       ComDiagsArea **diags);
  bool cancelQuery(char *queryId, Lng32 queryIdLen, 
                   CancelQueryRequest *request,
                   ComDiagsArea **diags);
  inline ActiveQueryMgr &getActiveQueryMgr() { return activeQueryMgr_; }
  inline PendingQueryMgr &getPendingQueryMgr() { return pendingQueryMgr_; }
  void cleanupDeletedSscpServers();
  bool getQidFromPid( Int32 pid,         // IN
                      Int32 minimumAge,  // IN
                      char *queryId,     // OUT
                      Lng32 &queryIdLen  // OUT
                    );
  bool activateFromQid(char *queryId, Lng32 qidLen, 
                       SuspendOrActivate sOrA,   // Param is placeholder.
                                                 // Someday may handle cancel.
                       ComDiagsArea *&diags,
                       bool suspendLogging);
  void suspendOrActivate(char *queryId, Lng32 qidLen, 
                         SuspendOrActivate sOrA, bool suspendLogging);
  Lng32 stopMasterProcess(char *queryId, Lng32 queryIdLen);

private:

  NAHeap *heap_;            // pointer to heap for process duration storage
  StatsGlobals *statsGlobals_;
  IpcEnvironment *ipcEnv_;
  IpcServerClass *sscpServerClass_;
  HashQueue *sscps_;
  NAList<IpcServer *> *deletedSscps_; // list of sscp servers to be deleted
  Int64 statsCollectionInterval_;
  Int64 statsTimeout_;
  IpcSetOfConnections recipients_;
  Long semId_;
  Int32 myCpu_;
  pid_t myPin_;
  NAHeap *statsHeap_; // Heap to store merged stats
  Queue *deallocatedSscps_;
  NABoolean forceMerge_;
  NABoolean doingGC_;
  Queue *pendingSscpMessages_;
  short storeSqlSrcLen_;
  ActiveQueryMgr activeQueryMgr_;
  PendingQueryMgr pendingQueryMgr_;
}; // SsmpGlobals

class SsmpGuaReceiveControlConnection : public GuaReceiveControlConnection
{
public:

   SsmpGuaReceiveControlConnection(
       IpcEnvironment *env,
       SsmpGlobals *ssmpGlobals,
       short receiveDepth = 256) :
       GuaReceiveControlConnection(env,
				   receiveDepth)
  { ssmpGlobals_ = ssmpGlobals; } 

  virtual void actOnSystemMessage(
       short                  messageNum,
       IpcMessageBufferPtr    sysMsg,
       IpcMessageObjSize      sysMsgLen,
       short                  clientFileNumber,
       const GuaProcessHandle &clientPhandle,
       GuaConnectionToClient  *connection);
  SsmpGlobals *getSsmpGlobals() { return ssmpGlobals_; }

private:

  SsmpGlobals *ssmpGlobals_;
}; // SsmpGuaReceiveControlConnection

// -----------------------------------------------------------------------
// An object that holds a new connection, created by a Guardian open
// system message, until the first application message comes in
// -----------------------------------------------------------------------

class SsmpNewIncomingConnectionStream : public IpcMessageStream
{
public:

  SsmpNewIncomingConnectionStream(NAHeap *heap, IpcEnvironment *ipcEnv,
    SsmpGlobals *ssmpGlobals)
   : IpcMessageStream(ipcEnv,
                   IPC_MSG_SSMP_REPLY,
		   CurrSsmpReplyMessageVersion,
#ifndef USE_SB_NEW_RI
		   RTS_STATS_MSG_BUF_SIZE,
#else
		   ipcEnv->getGuaMaxMsgIOSize(),
#endif
		   TRUE)
   , sscpDiagsArea_(NULL)
  {
    ipcEnv_ = ipcEnv;
    ssmpGlobals_ = ssmpGlobals;
    heap_ = heap;
    handle_ = INVALID_RTS_HANDLE;
    wmsProcess_ = FALSE;
  }

  ~SsmpNewIncomingConnectionStream();
  virtual void actOnSend(IpcConnection *connection);
  virtual void actOnSendAllComplete();
  virtual void actOnReceive(IpcConnection *connection);
  virtual void actOnReceiveAllComplete();
  SsmpGlobals *getSsmpGlobals() { return ssmpGlobals_; }
  NAHeap *getHeap() { return heap_; }
  IpcEnvironment *getIpcEnv() { return ipcEnv_; }

  void actOnStatsReq(IpcConnection *connection);
  void actOnCpuStatsReq(IpcConnection *connection);
  void actOnExplainReq(IpcConnection *connection);
  void actOnQueryStartedReq(IpcConnection *connection);
  void actOnQueryFinishedReq(IpcConnection *connection);
  void actOnCancelQueryReq(IpcConnection *connection);
  void actOnSuspendQueryReq(IpcConnection *connection);
  void actOnActivateQueryReq(IpcConnection *connection);
  void actOnSecInvalidKeyReq(IpcConnection *connection);
  void actOnLobLockReq(IpcConnection *connection);
  void getProcessStats(short reqType,
                       short subReqType,
                       pid_t pid);

  void getMergedStats(RtsStatsReq *request,
                     RtsQueryId *queryId,
                     StmtStats *stmtStats,
                     short reqType,
                     UInt16 statsMergeType);
  void sendMergedStats(ExStatisticsArea *mergedStats, short numErrors, short reqType, 
            StmtStats *stmtStats, NABoolean updateMergeStats);
  void sscpIpcError(IpcConnection *conn);
  ComDiagsArea *getSscpDiagsArea() { return sscpDiagsArea_; }
  void clearSscpDiagsArea() { sscpDiagsArea_->decrRefCount(); 
                              sscpDiagsArea_ = NULL; }

  inline RtsHandle getHandle() { return handle_; }
  inline void setHandle(const RtsHandle h) { handle_ = h; }
  inline NABoolean isWmsProcess() { return wmsProcess_; }
  inline void setWmsProcess(NABoolean flag) { wmsProcess_ = flag; }
private:

  NAHeap *heap_;
  IpcEnvironment *ipcEnv_;
  SsmpGlobals *ssmpGlobals_;
  ComDiagsArea *sscpDiagsArea_;
  RtsHandle handle_;
  NABoolean wmsProcess_;
}; // SsmpNewIncomingConnectionStream

// -----------------------------------------------------------------------
// The message stream used by the send top node to exchange data with
// the send bottom node via an Ipc connection
// -----------------------------------------------------------------------

class SscpClientMsgStream : public IpcMessageStream
{
public:
  // constructor
  SscpClientMsgStream(NAHeap *heap, IpcEnvironment *ipcEnv, SsmpGlobals *ssmpGlobals,
          SsmpNewIncomingConnectionStream *ssmpStream)
      : IpcMessageStream(ipcEnv,
                     IPC_MSG_SSCP_REQUEST,
                     CurrSscpRequestMessageVersion,
#ifndef USE_SB_NEW_RI
                     RTS_STATS_MSG_BUF_SIZE, 
#else
                     ipcEnv->getGuaMaxMsgIOSize(),
#endif
                     TRUE), // Share the objects,
        heap_(heap)
  {
    ssmpGlobals_ = ssmpGlobals;
    mergedStats_ = NULL;
    mergeStartTime_ = 0;
    numOfClientRequestsSent_ = 0;
    numOfErrorRequests_ = 0;
    replySent_ = FALSE;
    ssmpStream_ = ssmpStream;
    numSqlProcs_ = 0;
    numCpus_ = 0;
    stmtStats_ = NULL;
    detailLevel_ = 0;
    completionProcessing_ = STATS;
    subReqType_ = -1;
  }
  ~SscpClientMsgStream();
  // method called upon send complete
  virtual void actOnSendAllComplete();

  // method called upon receive complete
  virtual void actOnReceive(IpcConnection* connection);
  virtual void actOnReceiveAllComplete();

  void actOnStatsReply(IpcConnection *connection);
  void delinkConnection(IpcConnection *conn);
  NAHeap *getHeap() 
  { return heap_; } 
  inline Int64 getMergeStartTime() { return mergeStartTime_; }
  inline void setMergeStartTime(Int64 startTime) { mergeStartTime_ = startTime; }
  ExStatisticsArea *getMergedStats() { return mergedStats_; }
  void incNumOfClientRequestsSent() { numOfClientRequestsSent_++; }
  Lng32 getNumOfClientRequestsPending() { return numOfClientRequestsSent_; }
  Lng32 getNumOfErrorRequests() { return numOfErrorRequests_; }
  SsmpGlobals *getSsmpGlobals() { return ssmpGlobals_; }
  NABoolean isReplySent() { return replySent_; }
  SsmpNewIncomingConnectionStream *getSsmpStream() {return ssmpStream_; }
  void setReplySent() 
  {
    replySent_ = TRUE;
    mergedStats_ = NULL;
  }
  void sendMergedStats();
  inline short getReqType() { return reqType_; }
  inline void setReqType(short reqType) { reqType_ = reqType; }
  inline void incNumSqlProcs(short i=1) { numSqlProcs_+= i; }
  inline void incNumCpus(short i=1) { numCpus_ += i; }
  inline short getNumSqlProcs() { return numSqlProcs_; }
  inline short getNumCpus() { return numCpus_; }
  inline StmtStats *getStmtStats() { return stmtStats_; }
  inline void setStmtStats(StmtStats *stmtStats) 
    { stmtStats_ = stmtStats; }
  inline void setDetailLevel(short level)
    { detailLevel_ = level; }
  inline short getDetailLevel() { return detailLevel_; }
  inline void setUsedToSendCbMsgs() { completionProcessing_ = CB; }
  inline void setUsedToSendSikMsgs() { completionProcessing_ = SIK; }
  inline void setUsedToSendLLMsgs() { completionProcessing_ = LL; }
  void replySik();
  void replyLL();
  inline short getSubReqType() { return subReqType_; }
  inline void setSubReqType(short subReqType) { subReqType_ = subReqType; }
private:
  NAHeap *heap_;
  ExStatisticsArea *mergedStats_;
  Int64 mergeStartTime_;
  Lng32  numOfClientRequestsSent_;
  SsmpGlobals *ssmpGlobals_;
  Lng32  numOfErrorRequests_;
  NABoolean replySent_;
  SsmpNewIncomingConnectionStream *ssmpStream_;
  short reqType_;
  short numSqlProcs_;
  short numCpus_;
  StmtStats *stmtStats_;
  enum { STATS, CB, SIK,LL} completionProcessing_;
  short detailLevel_;
  short subReqType_;
};

// -----------------------------------------------------------------------
// The message stream used by the collector in ExStatsTcb or ExExplainTcb
// via an Ipc connection
// -----------------------------------------------------------------------

class SsmpClientMsgStream : public IpcMessageStream
{
public:

  // constructor
  SsmpClientMsgStream(NAHeap *heap, ExSsmpManager *ssmpManager,
                      ComDiagsArea *diagsForClient = NULL)

      : IpcMessageStream(ssmpManager->getIpcEnvironment(),
                     IPC_MSG_SSMP_REQUEST,
                     CurrSsmpRequestMessageVersion,
#ifndef USE_SB_NEW_RI
                     RTS_STATS_MSG_BUF_SIZE, 
#else
                      ssmpManager->getIpcEnvironment()->getGuaMaxMsgIOSize(),
#endif
                     TRUE),
      heap_(heap)
    , ssmpManager_(ssmpManager)
    , diagsForClient_(diagsForClient)
  {
    stats_ = NULL;
    replyRecvd_ = FALSE;
    rtsQueryId_ = NULL;
    numSscpReqFailed_ = 0;
    explainFrag_ = NULL;
  }
  
  // method called upon send complete
  virtual void actOnSend(IpcConnection* connection);
  virtual void actOnSendAllComplete();

  // method called upon receive complete
  virtual void actOnReceive(IpcConnection* connection);
  virtual void actOnReceiveAllComplete();

  virtual void delinkConnection(IpcConnection *);

  void actOnStatsReply(IpcConnection* connection);
  void actOnExplainReply(IpcConnection *connection);
  void actOnGenericReply();
  ExStatisticsArea *getStats() { return stats_; };
  NAHeap *getHeap() { return heap_; }
  NABoolean isReplyReceived() { return replyRecvd_; }
  RtsQueryId *getRtsQueryId() { return rtsQueryId_; }
  short getNumSscpReqFailed() { return numSscpReqFailed_; }
  RtsExplainFrag *getExplainFrag() { return explainFrag_; }
private:
  NAHeap *heap_;
  ExStatisticsArea *stats_;
  ExSsmpManager *ssmpManager_;
  ComDiagsArea *diagsForClient_;  // non-null if client wants ipc diags.
  NABoolean replyRecvd_;
  RtsQueryId *rtsQueryId_;
  short numSscpReqFailed_;
  RtsExplainFrag *explainFrag_;
};

#endif // _SSMPIPC_H_

