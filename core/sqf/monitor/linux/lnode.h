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

#ifndef LNODE_H_
#define LNODE_H_

#include "clusterconf.h"
#include "process.h"

class CLNodeContainer;

class CLNode : public CProcessContainer
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
#if 1
    int             Nid;           // Logical Node Identifier
    cpu_set_t       CoreMask;      // # of SMP processor cores used by logical node
    int             NumProcessors; // # of logical processors in logical node
    ZoneType        NodeZoneType;  // type of zone
    bool            ChangeState;   // True if need to up or down the node.
#endif
    CLNode( CLNodeContainer *lnodes
          , int              nid
          , cpu_set_t       &coreMask 
          , int              processors
          , ZoneType         zoneType
          );
    ~CLNode( void );

    void    Added( void );
    void    Changed( CLNodeConfig *lnodeConfig );
    void    Deleted( void );
    void    DeLink( CLNode **head, CLNode **tail );
    void    DeLinkP( CLNode **head, CLNode **tail );
    void    Down( void );
    inline CLNode *GetNext( void ) { return( next_); }
    inline CLNode *GetNextP( void ) { return( nextP_); }

    cpu_set_t       &GetCoreMask( void ) { return( CoreMask ); }
    inline CLNodeContainer *GetLNodeContainer( void ) { return( lnodes_ ); }
    inline int       GetNid( void ) { return( Nid ); }
    inline int       GetProcessors( void ) { return( NumProcessors ); }
    inline ZoneType  GetZoneType( void ) { return( NodeZoneType ); }
    inline int GetNumCores ( void ) { return numCores_; }
    inline int GetFirstCore ( void ) { return firstCore_; }
    inline long long GetCpuUser( void ) { return( cpuUser_ ); }
    inline long long GetCpuNice( void ) { return( cpuNice_ ); }
    inline long long GetCpuSystem( void ) { return( cpuSystem_ ); }
    inline long long GetCpuIdle( void ) { return( cpuIdle_ ); }
    inline long long GetCpuIowait( void ) { return( cpuIowait_ ); }
    inline long long GetCpuIrq( void ) { return( cpuIrq_ ); }
    inline long long GetCpuSoftIrq( void ) { return( cpuSoftIrq_ ); }
    CProcess *GetProcessL( int pid );
    CProcess *GetProcessL( char *name, bool checkstate=true );
    CProcess *GetProcessL( int pid
                         , Verifier_t verifier
                         , bool checkstate=true );
    CProcess *GetProcessL( const char *name
                         , Verifier_t verifier
                         , bool checkstate=true );
    CProcess *CompleteProcessStartup( char *process_name, 
                                      char *port, 
                                      int os_pid, 
                                      bool event_messages,
                                      bool system_messages,
                                      struct timespec *creation_time,
                                      int origPNidNs );
    inline void SetCpuUser( long long num ) { cpuUser_ = num; }
    inline void SetCpuNice( long long num ) { cpuNice_ = num; }
    inline void SetCpuSystem( long long num ) { cpuSystem_ = num; }
    inline void SetCpuIdle( long long num ) { cpuIdle_ = num; }
    inline void SetCpuIowait( long long num ) { cpuIowait_ = num; }
    inline void SetCpuIrq( long long num ) { cpuIrq_ = num; }
    inline void SetCpuSoftIrq( long long num ) { cpuSoftIrq_ = num; }

    CNode  *GetNode( void );
    STATE   GetState( void );
    int     GetZone( void );

    bool    IsKillingNode( void );

    CLNode  *LinkAfter( CLNode * &tail, CLNode * entry );
    CLNode  *LinkBefore( CLNode * &head, CLNode * entry );
    CLNode  *LinkAfterP( CLNode * &tail, CLNode * entry );
    CLNode  *LinkBeforeP( CLNode * &head, CLNode * entry );

    void    SetAffinity( pid_t pid, PROCESSTYPE type );
    void    SetAffinity( CProcess *process );

    inline void      SetLNodeContainer( CLNodeContainer *lnodes ) { lnodes_ = lnodes; }
    inline CProcess *GetSSMProc () { return SSMProc; }
    inline void      SetSSMProc ( CProcess * proc ) { SSMProc = proc; } 

    void    Up( void );

protected:
private:
    int              tseCnt_;        // # of TSE primary processes in logical node 
    int              tseBackupCnt_;  // # of TSE backup processes in logical node 
    CLNodeContainer *lnodes_;        // logical nodes container of CNode (physical node)

#if 0
    int              nid_;           // Logical Node Identifier
    cpu_set_t        coreMask_;      // # of SMP processor cores used by logical node
    int              numProcessors_; // # of logical processors in logical node
    ZoneType         nodeZoneType_;  // type of zone
    bool             changeState_;   // True if need to up or down the node.
#endif
    long long cpuUser_;
    long long cpuNice_;
    long long cpuSystem_;
    long long cpuIdle_;
    long long cpuIowait_;
    long long cpuIrq_;
    long long cpuSoftIrq_;
    int numCores_;    // The number of cores assigned to this logical node
    int firstCore_;   // The first core assigned to this logical node
    int lastTseCoreAssigned_; // core # of last TSE primary process assigment
    int lastBackupTseCoreAssigned_; // core # of last TSE backup process assigment

    CLNode *next_;   // next CLNode in CLNodeContainer linked list (all CLNodes)
    CLNode *prev_;   // prev CLNode in CLNodeContainer linked list (all CLNodes)
    CLNode *nextP_;  // next CLNode in node_ linked list
    CLNode *prevP_;  // prev CLNode in node_ linked list
    CProcess * SSMProc; // The SQL Statistics Merge Process for the node
};

class CLNodeContainer
{
private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    CLNode  **LNode;      // array of logical node objects
    int     LastNid;      // Last node selected for process startup

    CLNodeContainer( CNode *node );
    ~CLNodeContainer( void );

    CLNode *AddLNode( CLNodeConfig *lnodeConfig, CNode *node );
    void    AddLNodeP( CLNode *lnode );
    void    CancelDeathNotification( int nid
                                   , int pid
                                   , int verifier
                                   , _TM_Txid_External trans_id );
    void    CheckForPendingCreates( CProcess *process=NULL );
    inline  CLNode *GetFirstLNode( void ) { return ( head_ ); }
    inline  CLNode *GetLastLNode( void ) { return ( tail_ ); }

    CLNode *GetLNode( int nid );
    CLNode *GetLNode( char *process_name, CProcess **process,
                      bool checkstate=true, bool backupOk=false );
    CLNode *GetLNodeByMap( int index );
    CLNode *GetLNodeNext( int nid, bool checkstate=true );

    inline  int    GetNidByMap( int index ) { return ( indexToNid_[index] ); }
    int     GetNidIndex( int nid );

    inline  CNode *GetNode( void ) { return ( node_ ); }
    inline  int    GetLNodesCount( void ) { return ( lnodesCount_ ); }
    bool    IsMyNode( int nid );
    void    MarkStaleOpens( int nid, int pid );
    void    RemoveLNodeP( CLNode *lnode );

protected:
    void    DeleteLNode( CLNode *lnode );
    inline  void SetFirstLNode( CLNode *lnode ) { head_ = lnode; }
    inline  void SetLastLNode( CLNode *lnode ) { tail_ = lnode; }
    inline  void SetLNodesCount( int lnodesCount ) { lnodesCount_ = lnodesCount; }

    int      lnodesCount_; // # of logical nodes in this container
    int     *indexToNid_;  // map of configuration entries to LNode[nid]

private:
    CNode   *node_;        // physical node of this container or 
                           // NULL when main logical node container
    CLNode  *head_;
    CLNode  *tail_;

};

#endif /* LNODE_H_ */
