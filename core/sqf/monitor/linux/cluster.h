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

#ifndef CLUSTER_H_
#define CLUSTER_H_

#include "pnode.h"
#include "msgdef.h"
#include "internal.h"
#include "cmsh.h"


#define LOCKFILE(file)     struct flock lock; \
                           int ret; \
                           int lock_tries; \
                           lock.l_type = F_WRLCK; \
                           lock.l_whence = SEEK_SET; \
                           lock.l_start = 0; \
                           lock.l_len = 1; \
                           for (lock_tries=0; lock_tries<100; lock_tries++) \
                           { \
                               ret = fcntl(file, F_SETLK, &lock); \
                               if (ret != -1) break; \
                               usleep(100); \
                           }

#define UNLOCKFILE(file)   lock.l_type = F_UNLCK; \
                           ret = fcntl(file, F_SETLK, &lock)

#define ACCEPT_NEW_MONITOR_TIMEOUT "250"

enum MonXChngTags
{
    MON_XCHNG_HEADER = 100,
    MON_XCHNG_DATA
};

class CNode;
class CLNode;

class CCluster 
{
 protected:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
    int           *socks_;
    int           *sockPorts_;
    int            commSock_;
    int            syncSock_;
    int            epollFD_;

public:

    enum ReintegrateError
    {
        Reintegrate_Err1,   // Could not connect to creator monitor
        Reintegrate_Err2,   // Could not merge intercomm
        Reintegrate_Err3,   // Could not get cluster info from creator monitor
        Reintegrate_Err4,   // Failed to send name/port to other monitor
        Reintegrate_Err5,   // Failed to connect to other monitor
        Reintegrate_Err6,   // Could not merge intercomm
        Reintegrate_Err7,   // Could not disconnect intercomm
        Reintegrate_Err8,   // Could not send status to creator monitor
        Reintegrate_Err9,   // Could not send name/port to creator monitor
        Reintegrate_Err10,  // Could not write to port file
        Reintegrate_Err11,  // Could not open port file
        Reintegrate_Err12,  // Could not initialize local io
        Reintegrate_Err13,  // Could not initialize devices
        Reintegrate_Err14,  // [init error previously logged]
        Reintegrate_Err15   // Could not get connect acknowledgement
    };

    int        NumRanks;       // Current # of processes in the cluster
    int        NumNodes;       // Initial # of nodes in the cluster ( may include down nodes )

    CCluster( void );
    virtual ~CCluster( void );

    int  AcceptCommSock( void );
    int  AcceptSyncSock( void );
    int  Connect( const char *portName );
    void ConnectToSelf( void );
    int  SetKeepAliveSockOpt( int sock );
    int  MkCltSock( const char *portName );
#ifndef USE_BARRIER
    void ArmWakeUpSignal (void);
#endif
    void AssignTmLeader( int pnid );
    void stats();
    void CompleteSyncCycle()
        { syncCycle_.lock(); syncCycle_.wait(); syncCycle_.unlock(); }
    void EnterSyncCycle() { syncCycle_.lock(); }
    void ExitSyncCycle() { syncCycle_.unlock(); }

    void DoDeviceReq(char * ldevname);
    void ExpediteDown( void );
    inline int  GetTmLeader( void ) { return( TmLeaderNid); }
    inline void SetTmLeader( int tmLeaderNid ) { TmLeaderNid = tmLeaderNid; } 
    int  GetDownedNid( void );
    inline int GetTmSyncPNid( void ) { return( TmSyncPNid ); } // Physical Node ID of current TmSync operations master
    void InitClusterComm(int worldSize, int myRank, int *rankToPnid);
    void addNewComm(int nid, int otherRank, MPI_Comm comm);
    void addNewSock(int nid, int otherRank, int sockFd );

    bool exchangeNodeData ( );
    void exchangeTmSyncData ( struct sync_def *sync );
    int GetNumNodes() { return cfgPNodes_; }
    bool ImAlive( bool needed=false, struct sync_def *sync = NULL );
    int  MapRank( int current_rank );
    void HardNodeDown( int nid, bool communicate_state=false );
    void SoftNodeDown( int pnid );
    int  SoftNodeUpPrepare( int pnid );
    bool CheckSpareSet( int pnid );
    inline bool  IsIntegrating( void ) { return( (joinSock_ != -1 || joinComm_ != MPI_COMM_NULL) || integratingPNid_ != -1 ); }
    struct message_def *JoinMessage( const char *node_name, int pnid, JOINING_PHASE phase );
    struct message_def *SpareUpMessage( const char *node_name, int nid );
    struct message_def *ReIntegErrorMessage( const char *msgText );
    void SetIntegratingNid( int nid ) { integratingPNid_ = nid; };
    int HardNodeUp( int pnid, char *node_name );
    CNode* GetIntegratingNode() { return Node[integratingPNid_]; }
    static char *Timestamp( void );
    void WakeUp( void );
    bool responsive();
    void setMonInitComplete(bool flag) { monInitComplete_ = flag; }
    inline bool isMonInitComplete() { return monInitComplete_; } 
    int MPIAllgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm Comm);
    void ReIntegrate( int initProblem );
    void SetJoinComm(MPI_Comm joinComm) { joinComm_ = joinComm; }
    void SetJoinSock( int  joinSock ) { joinSock_ = joinSock; }
    unsigned long long getSeqNum() { return seqNum_; }
    MPI_Comm getJoinComm() { return joinComm_; }
    int getJoinSock() { return joinSock_; }
    void ActivateSpare( CNode *spareNode, CNode *downNode, bool checkHealth=false );
    void NodeTmReady( int nid );
    void NodeReady( CNode *spareNode );
    bool isMonSyncResponsive() { return monSyncResponsive_; }
    void SaveSchedData( struct internal_msg_def *recv_msg );
    bool IsNodeDownDeathNotices() { return nodeDownDeathNotices_; }

    int  ReceiveMPI(char *buf, int size, int source, MonXChngTags tag, MPI_Comm comm);
    int  ReceiveSock(char *buf, int size, int sockFd);
    int  SendMPI(char *buf, int size, int source, MonXChngTags tag, MPI_Comm comm);
    int  SendSock(char *buf, int size, int sockFd);

    int incrGetVerifierNum();

    enum { SYNC_MAX_RESPONSIVE = 1 }; // Max seconds before sync thread is "stuck"

    // cluster snapshot header for use in reintegration
    typedef struct snapShotHeader
    {
        long procCount_;
        long spareNodesCount_;
        long nodeMapCount_;
        long fullSize_;
        long compressedSize_;
        long tmLeader_;
        unsigned long long seqNum_; 
    } snapShotHeader_t; 

    enum { ACCEPT_NEW_MONITOR_RETRIES = 120 };  // Maximum retries by creator monitor

protected:
    CNode  **Node;           // array of nodes
    CLNode **LNode;          // array of logical nodes
    int      TmSyncPNid;     // Physical Node ID of current TmSync operations master


    void AddTmsyncMsg (struct sync_def *sync,
                                 struct internal_msg_def *msg);
    void AddReplData (struct internal_msg_def *msg);
    void AddMyNodeState ();
    void TraceTMSyncState(struct sync_buffer_def *recv_buffer,
                          size_t recvCount);
    void UpdateAllNodeState(struct sync_buffer_def *recv_buffer,
                            size_t recvCount,
                            bool &overflow);
    void HandleOtherNodeMsg (struct internal_msg_def *recv_msg, int pnid);
    void HandleMyNodeMsg (struct internal_msg_def *recv_msg, int pnid);

    CLock syncCycle_;

private:
    int     CurNodes;       // Current # of nodes in the cluster
    int     CurProcs;       // Current # if processes alive in MPI_COMM_WORLD
    int     cfgPNodes_;     // # of physical nodes configured
    int    *NodeMap;        // Mapping of Node ranks to COMM_WORLD ranks
    int     TmLeaderNid;    // Nid of currently assigned TM Leader node
    int     tmReadyCount_;  // # of DTM processes ready for transactions
    size_t  minRecvCount_;  // minimum size of receive buffer for allgather

    // Pointer to array of "sync_buffer_def" structures.  Used by
    // ShareWithPeers in "Allgather" operation.
    struct sync_buffer_def  *recvBuffer_;

    // Another pointer to array of "sync_buffer_def" structures. Used by 
    // ShareWithPeers when called again within "Allgather" operation.
    struct sync_buffer_def *recvBuffer2_; 

    // count number of recursive calls to ShareWithPeers.
    int     swpRecCount_;

    NodesList  deadNodeList_;     // list of dead nodes (CNode *), rank failures
    NodeVector spareNodeVector_;  // array of active (up) spare nodes (CNode *) 
 
    unsigned long long barrierCount_;
    unsigned long long allGatherCount_;
    unsigned long long commDupCount_;

    // saved off counters for responsive check
    unsigned long long barrierCountSaved_;
    unsigned long long allGatherCountSaved_;
    unsigned long long commDupCountSaved_;

    // used to compute responsiveness of sync thread
    bool inBarrier_;
    bool inAllGather_;
    bool inCommDup_; 
    bool monInitComplete_;
    bool monSyncResponsive_;

    int integratingPNid_;       // pnid of node when re-integration in progress
    MPI_Comm  joinComm_;        // new to creator communicator (1-to-1)
    int joinSock_;              // new to creator socket used at join phase
    unsigned long long seqNum_;
    int cumulativeDelaySec_;

    bool waitForWatchdogExit_;    // set when watchdog exit has already been issued

    typedef struct state_def
    {
        char node_state;
        char sync_needed;
        int change_nid;
        unsigned long long seq_num;
        size_t syncDataAvail;
    } state_def_t;

    bool checkSeqNum_;
    bool validateNodeDown_;
    bool enqueuedDown_;   // true if detected seq num mismatch

    int syncMinPerSec_;   // Min barrier calls per sec

    struct timespec agMaxElapsed_;
    struct timespec agMinElapsed_;

    int agElapsed_[10];
    static const struct timespec agBuckets_[10];
    static const int agBucketsSize_;

    bool agTimeStats(struct timespec & ts_begin,
                     struct timespec & ts_end);
    


    struct sync_buffer_def *tmSyncBuffer_;

    // Size of send and receive buffers used for communication between monitors
    enum { CommBufSize = MAX_SYNC_SIZE };

    // Array of communicators, one for each pair of monitors.
    // Each communicator has a group size of two.
    MPI_Comm * comms_;

    // Array corresponding to "comms_", value is:
    //  0: if the "other" monitor is rank 0 in the group
    //  1: if the "other" monitor is rank 1 in the group
    int *  otherMonRank_;

    // Set of nodes that are up, one bit per node
    upNodes_t upNodes_;  // Array of 64 bits

    // Container to store new communicators or socket from HardNodeUp
    typedef struct commInfo_s
    { int pnid; int otherRank; MPI_Comm comm; int socket; struct timespec ts;} commInfo_t;
    typedef list<commInfo_t> newComms_t;
    newComms_t newComms_;
    CLock newCommsLock_;

    // Container to store exited monitor info
    typedef struct monExited_s
    {   int exitedPnid;
        int detectingPnid;
        unsigned long long  seqNum; } monExited_t;
    typedef list<monExited_t> exitedMons_t;
    exitedMons_t exitedMons_;

    bool nodeDownDeathNotices_; // default true

    Verifier_t verifierNum_; // part of phandle that uniquely identifies a process 

    int Allgather(int nbytes, void *sbuf, char *rbuf, int tag, MPI_Status *stats);
    int AllgatherIB(int nbytes, void *sbuf, char *rbuf, int tag, MPI_Status *stats);
    int AllgatherSock(int nbytes, void *sbuf, char *rbuf, int tag, MPI_Status *stats);

    void ValidateClusterState( cluster_state_def_t nodestate[],
                               bool haveDivergence );
    bool ValidateSeqNum( cluster_state_def_t nodestate[] );
    void HandleDownNode( int nid );
    void HandleReintegrateError( int rc, int err,
                                 int nid, nodeId_t *nodeInfo,
                                 bool abort );
    void ReIntegrateMPI( int initProblem );
    void ReIntegrateSock( int initProblem );
    void SendReIntegrateStatus( STATE nodeState, int status );

    void UpdateClusterState( bool & shutdown,
                             struct sync_buffer_def * syncBuf,
                             MPI_Status *status,
                             int sentChangeNid);
    bool ProcessClusterData( struct sync_buffer_def * syncBuf,
                             struct sync_buffer_def * sendBuf,
                             bool deferredTmSync );

    bool checkIfDone ( void );
    void setNewComm(int nid);
    void setNewSock(int nid);
 

    unsigned long long EnsureAndGetSeqNum(cluster_state_def_t nodestate[]);

    void SetupCommWorld( void );
    bool ShareWithPeers ( struct sync_def *sync, size_t recvCount);
    void InitializeCluster( void );
    void InitializeConfigCluster( void );
    void InitializeVirtualConfigCluster( int rank );

    void InitClusterSocks( int worldSize, int myRank, char *nodeNames,int *rankToPnid );
    void InitServerSock( void );
    int AcceptSock( int sock );
    void EpollCtl( int efd, int op, int fd, struct epoll_event *event );
    int  MkSrvSock( int *pport );
    int  MkCltSock( unsigned char srcip[4], unsigned char dstip[4], int port );

};

extern bool Emulate_Down;

#endif /*CLUSTER_H_*/
