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
********************************************************************/
//
// Zookeeper Client (CZClient class)
//
//  Implements the Zookeeper client functionality in the monitor process
//  as the ZClient object which manages znode monitoring events through
//  the ZCLientThread.
//
//  CZClient::StartWork() and CZClient::ShutdownWork() manage ZCLientThread
//  creation and deletion.
//
//      CZClient::StartMonitoring()
//              |
//          pthread_create(ZClientThread)
//              |
//          ZC_DISABLED
//              |
//          CZClient::MonitorZCluster()
//      
//  CZClient::MonitorZCluster() is the thread main, a state machine:
//      
//                       CZClient::StartMonitoring()
//                               |
//                           ZC_START
//                               |
//                       CZClient::StartClusterMonitoring()
//                               |
//                           ZC_WATCH
//                               |
//                       CZClient::WatchCluster()
//                               |
//                           ZC_MYZNODE <------------------|
//                               |                         |
//                       CZClient::CheckMyZNode()          |
//                               |                         |
//                               |-------------------------|
//                                                         |
//  ZOO_CHILD_EVENT                                        |
//  ZOO_NOTWATCHING_EVENT                                  |
//            |                                            |
//    CZClient::TriggerCheck()---|                         |
//                               |                         |
//                           ZC_CLUSTER                    |
//                               |                         |
//                       CZClient::CheckCluster()          |
//                               |                         |
//                               |-------------------------|
//  ZOO_CREATED_EVENT                                      |
//  ZOO_DELETED_EVENT                                      |
//  ZOO_CHANGED_EVENT                                      |
//            |                                            |
//    CZClient::TriggerCheck()---|                         |
//                               |                         |
//                           ZC_ZNODE                      |
//                               |                         |
//                       CZClient::HandleExpiredZNode()    |
//                               |                         |
//                               |-------------------------|
//                 
//                       CZClient::StopMonitoring()
//                               |
//                           ZC_STOP
//                               |
//                       CZClient::StopClusterMonitoring()
//                               |
//                           ZC_DISABLED
//
//      CZClient::ShutdownWork()
//              |
//          ZC_SHUTDOWN
//              |
//          pthread_join()
//
#ifndef ZCLIENT_H_
#define ZCLIENT_H_

#include <list>
#include <string>

#include "zookeeper/zookeeper.h"

using namespace std;

#define ZCLIENT_TRAFODION_ZNODE     "/trafodion"
#define ZCLIENT_INSTANCE_ZNODE      "/instance"
#ifdef NAMESERVER_PROCESS
#define ZCLIENT_MASTER_ZNODE        "/nsmaster"
#else
#define ZCLIENT_MASTER_ZNODE        "/master"
#endif

typedef list<string>    ZNodeList_t;

// The following two functions must be implemented in the calling program.
// - HandleMyNodeExpiration() is invoked when the monitor's session expires, or
//   the monitor's znode expires or quorum communication fails
// - HandleNodeExpiration(nodeName) is invoked when the znode associated with
//   the nodeName passed in expires.
extern void HandleMyNodeExpiration( void );
extern void HandleNodeExpiration( const char *nodeName );
extern void HandleAssignMonitorLeader ( const char* failedMaster );

class CZClient : public CLock
{
private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    typedef enum {
        ZC_DISABLED=0,    // initial state
        ZC_START,         // start monitoring
        ZC_WATCH,         // set cluster watchers
        ZC_CLUSTER,       // check cluster
        ZC_ZNODE,         // check znode
        ZC_MYZNODE,       // check this monitor's znode
        ZC_STOP,          // stop monitoring
        ZC_SHUTDOWN       // thread exit 
    } ZClientState_t;

    CZClient( const char *quorumHosts
            , const char *rootZNode
            , const char *instanceZNode );
    ~CZClient( void );

    int     CreateMasterZNode(  const char *nodeName );
    int     GetSessionTimeout( void) { return( zkSessionTimeout_ ); }
    bool    IsZNodeExpired( const char *nodeName, int &zerr );
    void    MonitorZCluster( void );
    void    SetCheckCluster( bool checkCluster ) { CAutoLock lock(getLocker()); checkCluster_ = checkCluster; }
    void    SetState( ZClientState_t state ) { CAutoLock lock(getLocker()); state_ = state; }
    void    SetState( ZClientState_t state, const char *znodePath );
    int     ShutdownWork( void );
    void    StartMonitoring( void );
    int     StartWork( void );
    void    StopMonitoring( void );
    void    TriggerCheck( int type, const char *znodePath );
    const char* WaitForAndReturnMaster( bool doWait );
    int     WatchNode( const char *nodeName );
    int     WatchMasterNode( const char *nodeName );
    int     WatchNodeDelete( const char *nodeName );
    int     WatchNodeMasterDelete( const char *nodeName );

private:
    int     ZooExistRetry(zhandle_t *zh, const char *path, int watch, struct Stat *stat);
    void    CheckCluster( void );
    void    CheckMyZNode( void );
    int     GetClusterZNodes( String_vector *children );
    int     GetZNodeData( string &monZnode, string &nodeName, int &pnid );
    ZClientState_t GetState( void ) { CAutoLock lock(getLocker()); return( state_ ); }
    void    HandleExpiredZNode( void );
    void    HandleMasterZNode ( void );
    int     InitializeZClient( void );
    bool    IsEnabled( void ) { CAutoLock lock(getLocker()); return( enabled_ ); }
    bool    IsCheckCluster( void ) { CAutoLock lock(getLocker()); return( checkCluster_ ); }
    int     MakeClusterZNodes( void );
    int     RegisterMyNodeZNode( void );
    int     RegisterZNode( const char *znodePath
                         , const char *znodeData
                         , int flags );
    void    SetEnabled( bool enabled ) { CAutoLock lock(getLocker()); enabled_ = enabled; }
    void    SetTimeToWakeUp( struct timespec &ts);
    int     SetZNodeWatch( string &monZnode );
    void    StartClusterMonitoring( void );
    void    StopClusterMonitoring( void );
    void    WatchCluster( void );

    pthread_t       threadId_;

    ZClientState_t  state_;        // Physical node's current operating state
    bool            enabled_;      // true when cluster monitoring enabled
    bool            checkCluster_; // true when cluster monitoring enabled
    bool            resetMyZNodeFailedTime_; // set to trigger fail time reset
    long            zcMonitoringRate_; // in seconds

    string          zkQuorumHosts_;
    string          zkRootNode_;
    string          zkRootNodeInstance_;
    stringstream    zkQuorumPort_;
    int             zkSessionTimeout_;
    struct timespec myZNodeFailedTime_;
    
    ZNodeList_t     znodeQueue_;
};

#endif // ZCLIENT_H_
