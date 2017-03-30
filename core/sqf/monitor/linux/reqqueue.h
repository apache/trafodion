///////////////////////////////////////////////////////////////////////////////
//
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
//
///////////////////////////////////////////////////////////////////////////////

#ifndef REQQUEUE_H_
#define REQQUEUE_H_

#include <list>
using namespace std;

#include "lock.h"
#include "monitor.h"
#include "config.h"

#define MAX_REVIVE_QUEUE_SIZE 10000

class CReqResource
{
 protected:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    CReqResource();
    virtual ~CReqResource();

    typedef enum ResStat {UnAvailable, Acquired, Busy, NotUp} ResourceStatus_t;

    virtual ResourceStatus_t acquireResource( long requestId ) = 0;
    virtual void releaseResource() = 0;
};

class CReqResourceProc: public CReqResource
{
public:
    CReqResourceProc(int nid, int pid, const char *name, Verifier_t verifier );
    virtual ~CReqResourceProc();

    ResourceStatus_t acquireResource( long requestId );
    void releaseResource();
    CProcess* getProcess();

private:
    CReqResourceProc();
    int       nid_;
    int       pid_;
    Verifier_t verifier_;
    string     processName_;
};

class CReqResourceConfig: public CReqResource
{
public:
    CReqResourceConfig(CConfigGroup *config);
    virtual ~CReqResourceConfig();

    ResourceStatus_t acquireResource( long requestId );
    void releaseResource();

private:
    CReqResourceConfig();
// tbd: should be CConfigGroup or CConfigKey?
    CConfigGroup* config_;
};

class CRequest
{
 protected:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
                                     // (see comment block at end of file for 
                                     //  assignments).
 public:
    CRequest();
    virtual ~CRequest();

    virtual void performRequest() = 0;
    const char *requestString() { return( requestString_.c_str() ); }

    virtual bool isExclusive() { return concurrent_ == false; }
    virtual bool isSyncDependent() = 0;
    virtual bool isShutdown() = 0;
    virtual bool prepare() { return true; }
    virtual bool takeOwnership( bool & ownershipFailure ) = 0;
    virtual void giveupOwnership() = 0;
    virtual void errorReply( int rc ) = 0;
    void evalReqPerformance( void );
    struct timespec startTime () { return reqStart_; }

    void setConcurrent(bool reqType) { concurrent_ = reqType; }

    void addResource(CReqResource *);

    void setId(long id) { id_ = id; }
    long getId() { return id_; }

    static void lioreply(struct message_def *msg, int Pid, int *error = NULL);

    void timeDiff ( struct timespec t1, struct timespec t2,
                    struct timespec &tDiff );

    typedef enum ReqStatus {OkToExec, WaitToExec, Failed} ReqStatus_t;
    ReqStatus_t okToExecute();
    virtual void validateObj( void ) = 0;

    int getExecTimeMax() { return execTimeMax_; }

    typedef enum ReqPriority {Normal, High} ReqPriority_t;
    ReqPriority_t getPriority() { return priority_; }
    void setPriority(ReqPriority_t priority) { priority_ = priority; }

    virtual void populateRequestString( void ) = 0;

private:
    bool concurrent_;  // true if request can be executed concurrently
                       // with others.

    struct timespec reqArrival_; 
    struct timespec reqStart_; 

protected:
    long id_;
    int numResources_;
    static const int MAX_RESOURCES=5;
    CReqResource * resources_[MAX_RESOURCES];
    string requestString_;
    int execTimeMax_; // maximum execution time allowed 

    ReqPriority_t priority_;
};

class CExternalReq: public CRequest
{
public:
    typedef enum
    {
        AttachStartupMsg,
        StartupMsg,
        NonStartupMsg,
        ShutdownWork
    } reqQueueMsg_t;

    CExternalReq( reqQueueMsg_t msgType
                , int pid
                , struct message_def *msg)
        : msgType_(msgType)
        , pid_(pid)
        , verifier_(-1)
        , msg_(msg)
        , reqType_(msg?msg->u.request.type:ReqType_Invalid) {}

    virtual ~CExternalReq() {}

    virtual void performRequest() = 0;
    bool isSyncDependent() { return false; }
    bool isShutdown() { return msgType_ == ShutdownWork; }
    bool takeOwnership( bool & ownershipFailure );
    void giveupOwnership();
    bool setResourceNeeds();
    void errorReply( int rc );
    void validateObj( void );

private:
    void errorOpenReq(struct message_def * msg);

protected:
    reqQueueMsg_t msgType_;
    int nid_;
    int pid_;
    Verifier_t verifier_;
    string processName_;
    struct message_def * msg_;
    // Request save data
    REQTYPE reqType_;
};

class CExtAttachStartupReq: public CExternalReq
{
public:
    CExtAttachStartupReq (reqQueueMsg_t msgType, int pid,
                          struct message_def *msg );
    virtual ~CExtAttachStartupReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtDumpReq: public CExternalReq
{
public:
    CExtDumpReq (reqQueueMsg_t msgType, int pid,
                 struct message_def *msg );
    virtual ~CExtDumpReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtNodeNameReq: public CExternalReq
{
public:
    CExtNodeNameReq (reqQueueMsg_t msgType, int pid,
                 struct message_def *msg );
    virtual ~CExtNodeNameReq();

    void performRequest();

private:
    void populateRequestString( void );
};


class CExtEventReq: public CExternalReq
{
public:
    CExtEventReq (reqQueueMsg_t msgType, int pid,
                  struct message_def *msg );
    virtual ~CExtEventReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtExitReq: public CExternalReq
{
public:
    CExtExitReq (reqQueueMsg_t msgType, int pid,
                 struct message_def *msg );
    virtual ~CExtExitReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtGetReq: public CExternalReq
{
public:
    CExtGetReq (reqQueueMsg_t msgType, int pid,
                struct message_def *msg );
    virtual ~CExtGetReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtKillReq: public CExternalReq
{
public:
    CExtKillReq (reqQueueMsg_t msgType, int pid,
                 struct message_def *msg );
    virtual ~CExtKillReq();

    void performRequest();

private:
    void populateRequestString( void );
    void Kill( CProcess *process );
};

class CExtMonStatsReq: public CExternalReq
{
public:
    CExtMonStatsReq (reqQueueMsg_t msgType, int pid,
                     struct message_def *msg );
    virtual ~CExtMonStatsReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtMountReq: public CExternalReq
{
public:
    CExtMountReq (reqQueueMsg_t msgType, int pid,
                  struct message_def *msg );
    virtual ~CExtMountReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtNewProcReq: public CExternalReq
{
public:
    CExtNewProcReq (reqQueueMsg_t msgType, int pid,
                    struct message_def *msg );
    virtual ~CExtNewProcReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtNodeDownReq: public CExternalReq
{
public:
    CExtNodeDownReq (reqQueueMsg_t msgType, int pid,
                     struct message_def *msg );
    virtual ~CExtNodeDownReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtNodeInfoReq: public CExternalReq
{
public:
    CExtNodeInfoReq (reqQueueMsg_t msgType, int pid,
                     struct message_def *msg );
    virtual ~CExtNodeInfoReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtPNodeInfoReq: public CExternalReq
{
public:
    CExtPNodeInfoReq (reqQueueMsg_t msgType, int pid,
                      struct message_def *msg );
    virtual ~CExtPNodeInfoReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtNodeUpReq: public CExternalReq
{
public:
    CExtNodeUpReq (reqQueueMsg_t msgType, int pid,
                   struct message_def *msg );
    virtual ~CExtNodeUpReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtNotifyReq: public CExternalReq
{
public:
    CExtNotifyReq (reqQueueMsg_t msgType, int pid,
                   struct message_def *msg );
    virtual ~CExtNotifyReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtNullReq: public CExternalReq
{
public:
    CExtNullReq (reqQueueMsg_t msgType, int pid,
                 struct message_def *msg );
    virtual ~CExtNullReq();

    void performRequest() {}

private:
    void populateRequestString( void ){}
};

class CExtOpenReq: public CExternalReq
{
public:
    CExtOpenReq(reqQueueMsg_t msgType, int pid, struct message_def *msg);
    virtual ~CExtOpenReq();

    bool prepare();
    void performRequest();
    bool prepareRequest();
    void errorReply( int rc );

private:
    bool prepared_;
    CExtOpenReq();
    void populateRequestString( void );
};

class CExtProcInfoBase: public CExternalReq
{
 public:
    CExtProcInfoBase(reqQueueMsg_t msgType, int pid, struct message_def *msg)
        : CExternalReq(msgType, pid, msg) {}
    virtual ~CExtProcInfoBase() {}

 protected:
    void ProcessInfo_CopyPairData( CProcess *process
                                 , ProcessInfoState &procState );
    void ProcessInfo_CopyData(CProcess *process, ProcessInfoState &procState);

    CProcess * ProcessInfo_GetProcess (int &nid, bool getDataForAllNodes);
    int ProcessInfo_BuildReply(CProcess *process,
                               struct message_def * msg,
                               PROCESSTYPE type,
                               bool getDataForAllNodes);
};

class CExtProcInfoReq: public CExtProcInfoBase
{
public:
    CExtProcInfoReq (reqQueueMsg_t msgType, int pid,
                     struct message_def *msg );
    virtual ~CExtProcInfoReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtProcInfoContReq: public CExtProcInfoBase
{
public:
    CExtProcInfoContReq (reqQueueMsg_t msgType, int pid,
                         struct message_def *msg );
    virtual ~CExtProcInfoContReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtSetReq: public CExternalReq
{
public:
    CExtSetReq (reqQueueMsg_t msgType, int pid,
                struct message_def *msg );
    virtual ~CExtSetReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtShutdownReq: public CExternalReq
{
public:
    CExtShutdownReq (reqQueueMsg_t msgType, int pid,
                     struct message_def *msg );
    virtual ~CExtShutdownReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtStartupReq: public CExternalReq
{
public:
    CExtStartupReq (reqQueueMsg_t msgType, int pid,
                    struct message_def *msg );
    virtual ~CExtStartupReq();

    void performRequest();

private:
    void populateRequestString( void );
};


class CExtTmLeaderReq: public CExternalReq
{
public:
    CExtTmLeaderReq (reqQueueMsg_t msgType, int pid,
                     struct message_def *msg );
    virtual ~CExtTmLeaderReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtTmReadyReq: public CExternalReq
{
public:
    CExtTmReadyReq(reqQueueMsg_t msgType, int pid,
                   struct message_def *msg );
    virtual ~CExtTmReadyReq();

    void performRequest();

private:
    void populateRequestString( void );

    int nid_;
};

class CExtTmSyncReq: public CExternalReq
{
public:
    CExtTmSyncReq (reqQueueMsg_t msgType, int pid,
                   struct message_def *msg );
    virtual ~CExtTmSyncReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CExtZoneInfoReq: public CExternalReq
{
public:
    CExtZoneInfoReq (reqQueueMsg_t msgType, int pid,
                     struct message_def *msg );
    virtual ~CExtZoneInfoReq();

    void performRequest();

private:
    void populateRequestString( void );
};


class CInternalReq: public CRequest
{
public:
    CInternalReq();
    virtual ~CInternalReq();

    void performRequest();
    bool isSyncDependent() { return true; }
    bool isShutdown() { return false; }
    bool takeOwnership( bool &  ) { return true; }
    void giveupOwnership();
    void errorReply( int rc );
    void validateObj( void );

    void populateRequestString( void );

    void setSeqNum( unsigned long long seqNum ) { seqNum_ = seqNum; }
    unsigned long long getSeqNum() { return seqNum_; }
    int GetReviveFlag() { return reviveFlag_; }
    void SetReviveFlag(int value) { reviveFlag_ = value; }

private:

    unsigned long long seqNum_;
    int reviveFlag_; // set if this request is needed for revive operation
};


class CIntCloneProcReq: public CInternalReq
{
public:
    CIntCloneProcReq( bool backup, bool unhooked, bool eventMessages, bool systemMessages, int nid, PROCESSTYPE type, int priority, int parentNid, int parentPid, int parentVerifier, int osPid, int verifier, pid_t priorPid, int persistentRetries, int  argc, struct timespec creationTime, strId_t pathStrId, strId_t ldpathStrId, strId_t programStrId, int nameLen, int portLen, int infileLen, int outfileLen, int argvLen, const char * stringData);
    virtual ~CIntCloneProcReq();

    void performRequest();

private:
    void populateRequestString( void );

    bool backup_;
    bool unhooked_;
    bool eventMessages_;
    bool systemMessages_;
    int nid_;
    PROCESSTYPE type_;
    int priority_;
    int parentNid_;
    int parentPid_;
    int parentVerifier_;
    int osPid_;
    int verifier_; 
    pid_t priorPid_;
    int persistentRetries_;
    int  argc_;
    struct timespec creationTime_;
    strId_t pathStrId_;
    strId_t ldpathStrId_;
    strId_t programStrId_;
    int  nameLen_;
    int  portLen_;
    int  infileLen_;
    int  outfileLen_;
    int  argvLen_;
    char * stringData_;
};

class CIntDeviceReq: public CInternalReq
{
public:
    CIntDeviceReq( char *ldevName );
    virtual ~CIntDeviceReq();

    void performRequest();

private:
    void populateRequestString( void );

    char ldevName_[MAX_KEY_NAME];   // Logical device name
};

class CIntExitReq: public CInternalReq
{
public:
    CIntExitReq();
    virtual ~CIntExitReq();

    void prepRequest( struct exit_def *exitDef );
    void performRequest();

private:
    void populateRequestString( void );

    int nid_;
    int pid_;
    Verifier_t verifier_;
    bool abended_;
    char name_[MAX_PROCESS_NAME];
};

class CIntKillReq: public CInternalReq
{
public:
    CIntKillReq( struct kill_def *killDef );
    virtual ~CIntKillReq();

    void performRequest();

private:
    void populateRequestString( void );

    int nid_;
    int pid_;
    Verifier_t verifier_;
    bool abort_;
};

class CIntNewProcReq: public CInternalReq
{
public:
    CIntNewProcReq( int nid
                  , PROCESSTYPE type
                  , int priority
                  , int backup
                  , int parentNid
                  , int parentPid
                  , Verifier_t parentVerifier
                  , int pairParentNid
                  , int pairParentPid
                  , Verifier_t pairParentVerifier
                  , int argc
                  , bool unhooked
                  , void *reqTag
                  , strId_t pathStrId
                  , strId_t ldpathStrId
                  , strId_t programStrId
                  , int nameLen
                  , int infileLen
                  , int outfileLen
                  , int argvLen
                  , const char * stringData );

    virtual ~CIntNewProcReq ( );

    void performRequest();

private:
    void populateRequestString( void );

    int nid_;
    PROCESSTYPE type_;
    int priority_;
    int backup_;
    int parentNid_;
    int parentPid_;
    Verifier_t parentVerifier_;
    int pairParentNid_;
    int pairParentPid_;
    Verifier_t pairParentVerifier_;
    int  argc_;
    bool unhooked_;
    void *reqTag_;
    strId_t pathStrId_;
    strId_t ldpathStrId_;
    strId_t programStrId_;
    int  nameLen_;
    int  infileLen_;
    int  outfileLen_;
    int  argvLen_;
    char * stringData_;
};

class CIntOpenReq: public CInternalReq
{
public:
    CIntOpenReq ( struct open_def *openDef  );
    virtual ~CIntOpenReq ( );

    void performRequest();

private:
    void populateRequestString( void );

    int openerNid_;
    int openerPid_;
    Verifier_t openerVerifier_;
    int openedNid_;
    int openedPid_;
    Verifier_t openedVerifier_;
};


class CIntProcInitReq: public CInternalReq
{
public:
    CIntProcInitReq ( struct process_init_def *procInitDef  );
    virtual ~CIntProcInitReq ( );

    void performRequest();

private:
    void populateRequestString( void );

    int nid_;
    int pid_;
    Verifier_t verifier_;
    STATE state_;
    int result_;
    CProcess *process_;
    char name_[MAX_PROCESS_NAME];
};

class CIntSetReq: public CInternalReq
{
public:
    CIntSetReq ( ConfigType type, const char *group, const char *key,
                 const char *value  );
    virtual ~CIntSetReq ( );

    void performRequest();

private:
    void populateRequestString( void );

    ConfigType type_;
    char group_[MAX_KEY_NAME];
    char key_[MAX_KEY_NAME];
    char value_[MAX_VALUE_SIZE_INT];
};

class CIntUniqStrReq: public CInternalReq
{
public:
    CIntUniqStrReq ( int nid, int id, const char *value  );
    virtual ~CIntUniqStrReq ( );

    void performRequest();

private:
    void populateRequestString( void );

    int nid_;
    int id_;
    char value_[MAX_VALUE_SIZE_INT];
};


class CIntChildDeathReq: public CInternalReq
{
 public:
    CIntChildDeathReq ( pid_t pid );
    virtual ~CIntChildDeathReq ( );

    void performRequest();

 private:
    void populateRequestString( void );

    pid_t pid_;
    CProcess * process_;
};

class CIntAttachedDeathReq: public CInternalReq
{
 public:
    CIntAttachedDeathReq ( pid_t pid );
    virtual ~CIntAttachedDeathReq ( );

    void performRequest();

 private:
    void populateRequestString( void );

    pid_t pid_;
};

class CIntShutdownReq: public CInternalReq
{
public:
    CIntShutdownReq( int level );
    virtual ~CIntShutdownReq();

    void performRequest();

private:
    void populateRequestString( void );

    int level_;
};

class CIntNodeNameReq: public CInternalReq
{
public:
    CIntNodeNameReq( const char *current_name, const char *new_name );
    virtual ~CIntNodeNameReq();

    void performRequest();

private:
    void populateRequestString( void );

    string current_name_;
    string new_name_;
};

class CIntDownReq: public CInternalReq
{
public:
    CIntDownReq( int pnid );
    virtual ~CIntDownReq();

    void performRequest();

private:
    void populateRequestString( void );

    int pnid_;
};

class CIntSoftNodeDownReq: public CInternalReq
{
public:
    CIntSoftNodeDownReq( int pnid );
    virtual ~CIntSoftNodeDownReq();

    void performRequest();

private:
    void populateRequestString( void );

    int pnid_;
};

class CIntSoftNodeUpReq: public CInternalReq
{
public:
    CIntSoftNodeUpReq( int pnid );
    virtual ~CIntSoftNodeUpReq();

    void performRequest();

private:
    void populateRequestString( void );

    int pnid_;
};

class CIntUpReq: public CInternalReq
{
public:
    CIntUpReq( int pnid, char *node_name, int merge_lead );
    virtual ~CIntUpReq();

    void performRequest();

private:
    void populateRequestString( void );

    string nodeName_;
    int mergeLead_;
    int pnid_;
};

class CIntActivateSpareReq: public CInternalReq
{
public:
    CIntActivateSpareReq(CNode *spareNode, CNode *downNode, bool checkHealth);
    virtual ~CIntActivateSpareReq();

    void performRequest();

private:
    void populateRequestString( void );

    CNode *spareNode_;
    CNode *downNode_;
    bool   checkHealth_;
};

class CIntReviveReq: public CInternalReq
{
public:
    CIntReviveReq();
    virtual ~CIntReviveReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CIntSnapshotReq: public CInternalReq
{
public:
    CIntSnapshotReq(unsigned long long seqNum);
    virtual ~CIntSnapshotReq();

    void performRequest();

    void setSeqNum(unsigned long long seqNum) { seqNum_ = seqNum; }
    unsigned long long getSeqNum() { return seqNum_; }

private:
    void populateRequestString( void );

    unsigned long long seqNum_;
};

class CQuiesceReq: public CInternalReq
{
public:
    CQuiesceReq();
    virtual ~CQuiesceReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CPostQuiesceReq: public CInternalReq
{
public:
    CPostQuiesceReq();
    virtual ~CPostQuiesceReq();

    void performRequest();

private:
    void populateRequestString( void );
};

class CIntCreatePrimitiveReq: public CInternalReq
{
public:
    CIntCreatePrimitiveReq( int pnid );
    virtual ~CIntCreatePrimitiveReq();

    void performRequest();

private:
    void populateRequestString( void );

    int pnid_;
};

class CIntTmReadyReq: public CInternalReq
{
public:
    CIntTmReadyReq( int nid );
    virtual ~CIntTmReadyReq();

    void performRequest();

private:
    void populateRequestString( void );

    int nid_;
};

class CReqQueue
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
 public:
    CReqQueue();
    virtual ~CReqQueue();

    void enqueueReq(CExternalReq::reqQueueMsg_t msgType, int pid,
                    struct message_def *msg);
    void enqueueCloneReq( struct clone_def *cloneDef );
    void enqueueDeviceReq( char *ldevName );
    void enqueueExitReq( struct exit_def *exitDef );
    void enqueueKillReq( struct kill_def *killDef );
    void enqueueNewProcReq( struct process_def *procDef );
    void enqueueOpenReq( struct open_def *openDef );
    void enqueueProcInitReq( struct process_init_def *procInitDef );
    void enqueueSetReq( struct set_def *setDef );
    void enqueueUniqStrReq( struct uniqstr_def *uniqStrDef );
    void enqueueChildDeathReq ( pid_t pid );
    void enqueueAttachedDeathReq ( pid_t pid );
    void enqueueDownReq( int pnid );
    void enqueueNodeNameReq( char *current_name, char *new_name);
    void enqueueSoftNodeDownReq( int pnid );
    void enqueueSoftNodeUpReq( int pnid );
    void enqueueShutdownReq( int level );
    void enqueueActivateSpareReq( CNode *spareNode, CNode *downNode, bool checkHealth=false );
    void enqueueUpReq( int pnid, char *node_name, int merge_lead );
    void enqueueReviveReq();
    void enqueueSnapshotReq(unsigned long long seqnum);
    bool addToReqReviveQueue(CInternalReq *request);
    void processReviveRequests(unsigned long long seqNum);
    void enqueueCreatePrimitiveReq( int pnid );
    void enqueueQuiesceReq();
    void enqueuePostQuiesceReq();
    void enqueueTmReadyReq( int nid );
    CRequest *getRequest();
    void finishRequest(CRequest *request);
    void nudgeWorker();

    void stats();

    static void timeDiff ( struct timespec t1, struct timespec t2,
                           struct timespec &tDiff );
    bool responsive(struct timespec &curTime);

    int getExecTimeMax() { return execTimeMax_; }
    
    enum { REQ_MAX_PERFORM = 1,  // Max seconds expected for request execution
           REQ_MAX_TOTAL = 3,    // Max seconds expected for request lifetime
           REQ_MAX_DEFER = 10,   // Max seconds request can be deferred
           REQ_MAX_RESPONSIVE = 10 }; // Max seconds before request is "stuck"

    static const char *svcReqType[];
    static const char *intReqType[];

private:
    CExternalReq * prepExternalReq(CExternalReq::reqQueueMsg_t msgType,
                                   int pid,struct message_def *msg);
    void enqueueReq(CInternalReq *req, bool reviveOper = false);

    bool busyExclusive_;   // true if an exclusive request in progress,
                           //   false otherwise.
    int busyWorkers_;      // Count of worker threads currently 
                           //   working on requests.
    int syncDependentRequests_;

    // statistics
    int maxQueueSize_;
    int maxBusyWorkers_;
    long numRequests_;

    struct timespec mostRecentStart_;

    // locks
    CLock workerStatusLock_;

    CLock reqQueueLock_;

    typedef list<CRequest *> reqList_t;
    reqList_t reqQueue_;
    reqList_t reqDeferred_;

    CLock reqReviveQueueLock_;

    typedef list<CInternalReq *> reqListInt_t;
    reqListInt_t reqReviveQueue_;

    static const bool reqConcurrent[];
    int execTimeMax_; // maximum time allowed for the current request 

};

#endif

/* CRequest eyecatcher_ assignments:

   CInternalReq:

      RQIA   CIntAttachedDeathReq
      RQIB   CPostQuiesceReq
      RQIC   CIntChildDeathReq
      RQID   CIntDeviceReq
      RQIE   CIntExitReq
      RQIF   CIntUniqStrReq
      RQIG   CIntSnapshotReq
      RQIH   CIntShutdownReq
      RQII   CIntProcInitReq
      RQIJ   (unused)
      RQIK   CIntKillReq
      RQIL   CIntCloneProcReq
      RQIM   (unused)
      RQIN   CIntNewProcReq
      RQIO   CIntOpenReq
      RQIP   CIntDownReq
      RQIQ   CIntUpReq
      RQIR   CIntReviveReq
      RQIS   CIntSetReq
      RQIT   (unused)
      RQIU   CQuiesceReq
      RQIV   (unused)
      RQIW   CIntCreatePrimitiveReq
      RQIX   CIntSoftNodeDownReq
      RQIY   CIntSoftNodeUpReq
      RQIZ   (unused)

   CExternalReq:
      RQEA   CExtAttachStartupReq
      RQEB   CExtDumpReq
      RQEC   CExtEventReq
      RQED   CExtExitReq
      RQEE   CExtGetReq
      RQEF   CExtKillReq
      RQEG   CExtMonStatsReq
      RQEH   CExtMountReq
      RQEI   CExtNewProcReq
      RQEJ   CExtNodeDownReq
      RQEK   CExtNodeInfoReq
      RQEK   CExtPNodeInfoReq
      RQEL   CExtNodeUpReq
      RQEM   CExtNotifyReq
      RQEN   CExtOpenReq
      RQEO   CExtProcInfoReq
      RQEP   CExtProcInfoContReq
      RQEQ   CExtSetReq
      RQER   CExtShutdownReq
      RQES   CExtStartupReq
      RQET   CExtTmLeaderReq
      RQEV   CExtTmSyncReq
      RQEW   CExtZoneInfoReq
      RQEX   (unused)
      RQEY   (unused)
      RQEZ   CExtNodeNameReq
      RQE_   CExtNullReq

 */
