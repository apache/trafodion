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

#ifndef PNODE_H_
#define PNODE_H_

#include <sched.h>
#include <list>
#include <vector>

#include "internal.h"
#include "clusterconf.h"
#include "lnode.h"
#include "monlogging.h"

typedef int * __attribute__((__may_alias__)) intBuffPtr_t;

typedef set<int>  pids_set_t;

class CNode;

typedef enum {
    Phase_Ready=0,                          // Node ready for use
    Phase_Activating,                       // Spare node going active
    Phase_SoftDown,                         // Node soft down
    Phase_SoftUp                            // Node soft up
} NodePhase;
typedef vector<int>     PNidVector;
typedef list<CNode *>   NodesList;
typedef vector<CNode *> NodeVector;

class CNodeContainer
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    int       NumberPNodes;  // # of physical node objects in array
    int       NumberLNodes;  // # of logical nodes objects in array
    CNode   **Node;          // array of physical node objects
    CLNode  **LNode;         // array of logical node objects


    CNodeContainer( void );
    ~CNodeContainer( void );

    void    AddNodes( void );
    void    AddToSpareNodesList( int pnid );
    CLNode *AssignLNode( CProcess *requester, PROCESSTYPE type, int nid, int not_nid );
    void    CancelDeathNotification( int nid
                                   , int pid
                                   , int verifier
                                   , _TM_Txid_External trans_id );
    inline CClusterConfig *GetClusterConfig( void ) { return ( clusterConfig_ ); }
    CLNode *GetLNode( int nid );
    CLNode *GetLNode( char *process_name, CProcess **process,
                      bool checkstate=true, bool backupOk=false );
    CNode  *GetNode( char *name );
    CNode  *GetNode( int pnid );
    CNode  *GetNode( char *process_name, CProcess **process,
                     bool checkstate=true );
    inline NodesList *GetSpareNodesList( void ) { return ( &spareNodesList_ ); }
    inline NodesList *GetSpareNodesConfigList( void ) { return ( &spareNodesConfigList_ ); }
    inline int GetNodesCount( void ) { return ( NumberPNodes ); }
    inline int GetLNodesCount( void ) { return ( NumberLNodes ); }
    inline int GetSNodesCount( void ) { return ( clusterConfig_->GetSNodesCount() ); }
    inline int GetAvailableSNodesCount( void ) { return ( spareNodesList_.size() ); }

    int     GetPNid( char *nodeName );
    CProcess *GetProcess( int nid, int pid, bool checknode=true );
    CProcess *GetProcess( int nid
                        , int pid
                        , Verifier_t verifier
                        , bool checknode=true
                        , bool checkprocess=true
                        , bool backupOk=false );
    CProcess *GetProcess( const char *name
                        , Verifier_t verifier
                        , bool checknode=true
                        , bool checkstate=true
                        , bool backupOk=false );
    CProcess *GetProcessByName( const char *name, bool checkstate=true );
    SyncState GetTmState( SyncState check_state );
    CNode  *GetZoneNode( int zid );

    struct internal_msg_def *InitSyncBuffer( struct sync_buffer_def *syncBuf
                                           , unsigned long long seqNum
                                           , upNodes_t upNodes );
    int GetSyncSize() { return  sizeof(cluster_state_def_t)
                              + sizeof(msgInfo_t)
                              + SyncBuffer->msgInfo.msg_offset; };
    inline int GetSyncHdrSize() { return  sizeof(cluster_state_def_t)
                                          + sizeof(msgInfo_t); };
    struct sync_buffer_def * GetSyncBuffer() { return SyncBuffer; };
    bool    IsShutdownActive( void );
    void    KillAll( CProcess *process );
    void    LoadConfig( void );
    void    MarkStaleOpens( int nid, int pid );
    int     ProcessCount( void );
    struct internal_msg_def *PopMsg( struct sync_buffer_def *recvBuf );
    bool    SpaceAvail ( int msgSize );
    void    AddMsg (struct internal_msg_def *&msg, int msgSize );
    void    SetupCluster( CNode ***pnode_list, CLNode ***lnode_list );
    void    RemoveFromSpareNodesList( CNode *node );

    int     PackNodeMappings( intBuffPtr_t &buffer );
    int     PackSpareNodesList( intBuffPtr_t &buffer );
    void    PackZids( intBuffPtr_t &buffer );
    void    UnpackNodeMappings( intBuffPtr_t &buffer, int nodeMapCount );
    void    UnpackSpareNodesList( intBuffPtr_t &buffer, int spareNodesCount );
    void    UnpackZids( intBuffPtr_t &buffer );

protected:

private:
    CClusterConfig *clusterConfig_;  // 'cluster.conf' objects
    NodesList  spareNodesList_; // current spare physical nodes list
    NodesList  spareNodesConfigList_; // configured spare physical nodes list
    CNode  *head_;  // head of physical nodes linked list
    CNode  *tail_;  // tail of physical nodes linked list
    size_t  syncBufferFreeSpace_;

    struct sync_buffer_def *SyncBuffer;

    void    AddLNodes( CNode  *node );
    void    AddLNodes( CNode  *node1, CNode  *node2 );
    void    AvgNodeData( ZoneType type, int *avg_pcount, unsigned int *avg_memory );    
    CLNode *SelectLNode( CProcess *requester, ZoneType type, int nid, int not_zone, bool considerLoad );
    CLNode *NextPossibleLNode( CProcess *requester, ZoneType type, int nid, int not_zone, bool considerLoad );
};

class CNode : public CLNodeContainer, public CProcessContainer
{
private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    CNode( char *name, int pnid, int rank );
    CNode( char *name
         , int   pnid
         , int   rank
         , int   sparePNidCount
         , int   sparePNids[]
         , cpu_set_t &excludedCoreMask );
    ~CNode( void );

    CLNode *AssignLNode( void );
    int     AssignNid( void );
    void    CheckActivationPhase( void );
    void    CheckShutdownProcessing( void );
    void    DeLink( CNode **head, CNode **tail );
    struct internal_msg_def *FindObjectToReplicate( struct internal_msg_def *msg );

    cpu_set_t   &GetAffinityMask( void ) { return( affinityMask_ ); }
    inline int   GetChangeState( void ) { return ( changeState_ ); }
    inline int   GetNumCores( void ) { return ( numCores_ ); }

    CNode  *GetNext( void );
    void    GetCpuStat ( void );
    bool    GetSchedulingData( void );

    inline unsigned int GetFreeCache( void ) { return( freeCache_ ); }
    inline unsigned int GetFreeMemory( void )
                      { return( memInfoData_[memFree] ); }
    inline unsigned int GetFreeSwap( void )
                      { return( memInfoData_[memSwapFree] ); }
    inline unsigned int GetMemTotal( void )
                      { return( memInfoData_[memTotal] ); }
    inline unsigned int GetMemActive( void )
                      { return( memInfoData_[memActive] ); }
    inline unsigned int GetMemInactive( void )
                      { return( memInfoData_[memInactive] ); }
    inline unsigned int GetMemDirty( void )
                      { return( memInfoData_[memDirty] ); }
    inline unsigned int GetMemWriteback( void )
                      { return( memInfoData_[memWriteback] ); }
    inline unsigned int GetMemVMallocUsed( void )
                      { return( memInfoData_[memVmallocUsed] ); }
    inline unsigned int GetBTime( void ) { return( bTime_ ); }
    inline CLNodeContainer *GetLNodeContainer( void ) { return( dynamic_cast<CLNodeContainer*>(this) ); }
    inline const char *GetHostname( void ) { return( hostname_.c_str() ); }
    inline const char *GetName( void ) { return( name_ ); }
    inline int   GetPNid( void ) { return( pnid_ ); }
    inline NodePhase     GetPhase( void ) { return( phase_ ); }
    inline int   GetCreatorPid( void ) { return( creatorPid_ ); }
    inline int   GetCreatorVerifier( void ) { return( creatorVerifier_ ); }
    inline JOINING_PHASE GetJoiningPhase( void ) { return( joiningPhase_ ); }
    inline int   GetRank( void ) { return( rank_ ); }
    inline ShutdownLevel GetShutdownLevel( void) { return( shutdownLevel_ ); }
    inline const char *GetCommPort( void ) { return commPort_.c_str(); }
    inline const char *GetSyncPort( void ) { return syncPort_.c_str(); }
    inline int   GetCommSocketPort( void ) { return( commSocketPort_ ); }
    inline int   GetSyncSocketPort( void ) { return( syncSocketPort_ ); }
    inline PNidVector   &GetSparePNids( void ) { return( sparePNids_ ); }
    inline STATE GetState( void ) { return( state_ ); }

    // If candidate string has not been seen before assign a unique
    // id and store it in the config database.   In either case return
    // the unique id as the value of the method.
    strId_t GetStringId(char *candidate);

    inline int   GetTmSyncNid( void ) { return( tmSyncNid_ ); }
    inline SyncState GetTmSyncState( void ) { return( tmSyncState_ ); }
    inline int   GetZone( void ) { return( zid_ ); }
    inline int   GetWDTKeepAliveTimerValue( void ) { return( wdtKeepAliveTimerValue_ ); }
    inline bool  IsActivatingSpare( void ) { return( activatingSpare_ ); }
    inline bool  IsCreator( void ) { return( creator_ ); }
    inline bool  IsDTMAborted( void ) { return( dtmAborted_ ); }
    inline bool  IsSMSAborted( void ) { return( smsAborted_ ); }
    inline bool  IsKillingNode( void ) { return( killingNode_ ); }
    inline bool  IsRankFailure( void ) { return( rankFailure_ ); }
    inline bool  IsSpareNode( void ) { return( spareNode_ ); }
    inline bool  IsSoftNodeDown( void ) { return( softDown_ ); }

    CNode  *Link( CNode *entry );
    void    MoveLNodes( CNode *targetNode );
    inline void ResetSpareNode( void ) { spareNode_ = false; }
    void    ResetWatchdogTimer( void );
    inline void ResetSoftNodeDown( void ) { softDown_ = false; }
    inline void SetActivatingSpare( int activatingSpare ) { activatingSpare_ = activatingSpare; }
    void    SetAffinity( int nid, pid_t pid, PROCESSTYPE type );
    void    SetAffinity( CProcess *process );
    inline void SetFreeCache( unsigned int freeCache ) { freeCache_ = freeCache; }
    inline void SetFreeMemory( unsigned int freeMemory )
                      { memInfoData_[memFree] = freeMemory; }
    inline void SetFreeSwap( unsigned int freeSwap )
                      { memInfoData_[memSwapFree] = freeSwap; }
    inline void SetMemTotal( unsigned int totalMemory )
                      { memInfoData_[memTotal] = totalMemory; }
    inline void SetMemActive( unsigned int active )
                      { memInfoData_[memActive] = active; }
    inline void SetMemInactive( unsigned int inactive )
                      { memInfoData_[memInactive] = inactive; }
    inline void SetMemDirty( unsigned int dirty )
                      { memInfoData_[memDirty] = dirty; }
    inline void SetMemWriteback( unsigned int writeback )
                      { memInfoData_[memWriteback] = writeback; }
    inline void SetMemVMallocUsed( unsigned int vmallocUsed )
                      { memInfoData_[memVmallocUsed] = vmallocUsed; }
    inline void SetChangeState( int changeState ) { changeState_ = changeState; }
    inline void SetBTime( unsigned int btime ) { bTime_ = btime; }
    inline void SetCreator( bool creator, int pid, Verifier_t verifier ) { creator_ = creator; creatorPid_ = pid; creatorVerifier_ = verifier; }
    inline void SetJoiningPhase( JOINING_PHASE phase ) { joiningPhase_ = phase; }
    inline void SetDTMAborted( bool dtmAborted ) { dtmAborted_ = dtmAborted; }
    inline void SetSMSAborted( bool smsAborted ) { smsAborted_ = smsAborted; }
    inline void SetKillingNode( bool killingNode ) { killingNode_ = killingNode; }
    inline void SetNumCores( int numCores ) { numCores_ = numCores; }
    inline void SetPhase( NodePhase phase ) { phase_ = phase; }
    inline void SetSoftNodeDown( void ) { softDown_ = true; }
    inline void SetSparePNids( PNidVector &sparePNids ) { sparePNids_ = sparePNids; }
    inline void SetRank( int rank ) { rank_ = rank; }
    inline void SetRankFailure( bool failed ) { rankFailure_ = failed; 
                                                rank_ = rankFailure_ ? -1 : rank_; }
    //inline void SetPort( char * port) { port_ = port; }
    inline void SetCommPort( char *commPort) { commPort_ = commPort; }
    inline void SetSyncPort( char *syncPort) { syncPort_ = syncPort; }
    //inline void SetSockPort( int sockPort ) { sockPort_ = sockPort; }
    inline void SetCommSocketPort( int commSocketPort) { commSocketPort_ = commSocketPort; }
    inline void SetSyncSocketPort( int syncSocketPort) { syncSocketPort_ = syncSocketPort; }
    inline void SetSpareNode( void ) { spareNode_ = true; }
    inline void SetShutdownLevel( ShutdownLevel level ) { shutdownLevel_ = level; }
    void SetState( STATE state );
    inline void SetTmSyncNid( int nid ) { tmSyncNid_ = nid; }
    inline void SetTmSyncState( SyncState syncState ) { tmSyncState_ = syncState; }
    inline void SetZone( int zid ) { zid_ = zid; }
    inline void SetName( char *newName ) { if (newName) strcpy (name_, newName); }
    void StartPStartDProcess( void );
    void StartPStartDPersistent( void );
    void StartPStartDPersistentDTM( int nid );
    void StartSMServiceProcess( void );
    void StartWatchdogProcess( void );
    void StartWatchdogTimer( void );
    void StopWatchdogTimer( void );

    void addToQuiesceSendPids( int pid, Verifier_t verifier );
    void addToQuiesceExitPids( int pid, Verifier_t verifier );
    void delFromQuiesceExitPids( int pid, Verifier_t verifier );
    inline bool isQuiesceExitPidsEmpty() { return quiesceExitPids_->empty(); }
    inline int getNumQuiesceExitPids() { return quiesceExitPids_->size(); }
    inline int getNumQuiesceSendPids() { return quiesceExitPids_->size(); }
    inline bool isInQuiesceState() { return (internalState_ == State_Quiesce); }
    inline void setQuiesceState() { internalState_ = State_Quiesce; }
    inline void clearQuiesceState() { internalState_ = State_Default; }
    inline IntNodeState getInternalState() { return internalState_; }
    inline void SetInternalState( IntNodeState state ) { internalState_ = state; }
    void EmptyQuiescingPids();
    void SendQuiescingNotices();
    
protected:
private:
    enum itemsProcmem {memTotal,        // total usable memory
                       memFree,         // amount of free memory in node
                       memBuffers,      // memory in buffer cache
                       memCached,       // memory in page cache minus swap cache
                       memSwapFree,     // amount of free swap in node
                       memActive,       // memory in active use
                       memInactive,     // memory available for reclamation
                       memDirty,        // memory waiting to be written to disk
                       memWriteback,    // memory being written to disk
                       memVmallocUsed,  // amount of used virtual memory
                       memFinalItem};   // [** final enum item **]

    int           pnid_;         // physical node identifier
    bool          changeState_;  // True if need to up or down the node.
    int           numCores_;     // # of SMP processors in physical node
    unsigned int  freeCache_;    // amount of free buffer/cache in node
    unsigned int  memInfoData_[memFinalItem];
    unsigned int  bTime_;        // node boot time
    char          name_[MPI_MAX_PROCESSOR_NAME]; // physical node name
    string        hostname_;     // physical node name without domain
    STATE         state_;        // Physical node's current operating state
    NodePhase     phase_;        // Physical node's current phase during spare node activation
    bool          softDown_;     // true when soft down node in process
    bool          killingNode_;  // true when down node in process
    bool          dtmAborted_;   // true when DTM process terminates abnormally
    bool          smsAborted_;   // true when SMS process terminates abnormally

    CLNode       *lastLNode_;    // last logical node selected for process attach
    ShutdownLevel lastSdLevel_;  // last shutdown level
    cpu_set_t     affinityMask_; // base SMP processor affinity settings
    cpu_set_t     excludedCoreMask_; // mask of excluded SMP processor cores
    bool          rankFailure_;  // true when this is has failed in CCluster::ReGroup()
    bool          creator_;      // true when this physical node where node up (re-integration) is initiated
    int           creatorPid_;   // pid of shell process that initated node up
    Verifier_t    creatorVerifier_; // verifier of shell process that initated node up
    JOINING_PHASE joiningPhase_; // node re-integration joining phase progress phase
    bool          activatingSpare_; // true when spare node activation in process
    bool          spareNode_;    // true when this is a spare physical node
    PNidVector    sparePNids_;   // array of pnids this node can spare
    CNode        *next_;
    CNode        *prev_;
    int           rank_;         // Node's Monitor rank in COMM_WORLD
    int           tmSyncNid_;    // Logical Node of TM that initiated sync
    SyncState     tmSyncState_;  // Sync operation state with TMs
    ShutdownLevel shutdownLevel_;
    int           wdtKeepAliveTimerValue_; // expiration time
    struct timeval todStart_;    // time of last watchdog reset

    SQ_LocalIOToClient::bcastPids_t   *quiesceSendPids_;   // list of pids on this node that needs quiescing.
    SQ_LocalIOToClient::bcastPids_t   *quiesceExitPids_;   // list of pids on this node that will exit on quiescing
    IntNodeState  internalState_;     // internal state of a node, not externalized to users 
    
    int           zid_;
    string        commPort_;          // monitor MPI or Integration port
    string        syncPort_;          // monitor socket allgather port
    int           commSocketPort_;          // re-integration socket port
    int           syncSocketPort_;          // algather socket port

    int uniqStrId_;
    
    FILE *procStatFile_;    // "/proc/stat" file pointer
    int  procMeminfoFile_;  // "/proc/meminfo" file descriptor

    typedef struct {
        char * buffer;
        char * bol;
        size_t bufsize;
        size_t remBytes;
    } bufInfo_t;

    bool NextMemInfoLine( bufInfo_t &inBuf, char * dataline );

    timespec      prevSchedData_;  // timestamp for when last acquired
                                   // scheduling data (see GetSchedulingData)
    static unsigned long int minSchedDataInterval_;

    static const char *memInfoString_[];     // strings for error messages
    static size_t memInfoStringLen_[memFinalItem];

};

#endif /*PNODE_H_*/
