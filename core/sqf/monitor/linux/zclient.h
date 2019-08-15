/*********************************************************************
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
//      CZClient::StartWork()
//              |
//          pthread_create(ZClientThread)
//              |
//          ZC_DISABLED
//              |
//          CZClient::MonitorCluster()
//      
//  CZClient::MonitorCluster() is the thread main, a state machine:
//      
//                       CZClient::StartMonitoring()
//                               |
//                           ZC_START
//                               |
//                       CZClient::ClusterMonitoringStart()
//                               |
//                           ZC_WATCH
//                               |
//                       CZClient::RunningZNodesWatchSet()
//                               |
//                           ZC_MYZNODE <------------------|
//                               |                         |
//                       CZClient::MyRunningZNodeCheck()   |
//                               |                         |
//                               |-------------------------|
//                                                         |
//  ZOO_NOTWATCHING_EVENT                                  |
//            |                                            |
//    CZClient::TriggerCheck()---|                         |
//                               |                         |
//                           ZC_CLUSTER                    |
//                               |                         |
//                       CZClient::CheckCluster()          |
//                               |                         |
//                               |-------------------------|
//                                                         |
//  ZOO_CHANGED_EVENT                                      |
//            |                                            |
//    CZClient::TriggerCheck()---|                         |
//                               |                         |
//                           ZC_ZNODE_CHANGED              |
//                               |                         |
//                       CZClient::HandleChangedZNode()    |
//                               |                         |
//                               |-------------------------|
//                                                         |
//  ZOO_CHILD_EVENT                                        |
//            |                                            |
//    CZClient::TriggerCheck()---|                         |
//                               |                         |
//                           ZC_ZNODE_CHILD                |
//                               |                         |
//                       CZClient::HandleChildZNode()      |
//                               |                         |
//                               |-------------------------|
//                                                         |
//  ZOO_CREATED_EVENT                                      |
//            |                                            |
//    CZClient::TriggerCheck()---|                         |
//                               |                         |
//                           ZC_ZNODE_CREATED              |
//                               |                         |
//                       CZClient::HandleCreatedZNode()    |
//                               |                         |
//                               |-------------------------|
//                                                         |
//  ZOO_DELETED_EVENT                                      |
//            |                                            |
//    CZClient::TriggerCheck()---|                         |
//                               |                         |
//                           ZC_ZNODE_DELETED              |
//                               |                         |
//                       CZClient::HandleDeletedZNode()    |
//                               |                         |
//                               |-------------------------|
//                 
//                       CZClient::StopMonitoring()
//                               |
//                           ZC_STOP
//                               |
//                       CZClient::ClusterMonitoringStop()
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

// The following is the znode directory hierarchy:
//      ZCLIENT_TRAFODION_ZNODE "/$TRAF_ROOT_ZNODE"
//      ZCLIENT_INSTANCE_ZNODE      "/$TRAF_INSTANCE_ID"
#define ZCLIENT_CLUSTER_ZNODE           "/cluster"
#define ZCLIENT_CONFIGURED_ZNODE            "/configured"
#define ZCLIENT_ERROR_ZNODE                 "/error"
#define ZCLIENT_RUNNING_ZNODE               "/running"
#define ZCLIENT_MONITOR_ZNODE           "/monitor"
#ifndef NAMESERVER_PROCESS
#define ZCLIENT_MASTER_ZNODE                "/master"
#else
#define ZCLIENT_MASTER_ZNODE                "/nsmaster"
#endif
// Usage:
//   ZCLIENT_ERROR_ZNODE - to determine when a non-communicative node should be declared down
//    /error
//       /<error-znode>      - node with communication problem if child count > 0
//          /<znode>         - node that has problem communicating with <error-znode>
//             o when more than one node has problem with <error-znode> 
//               the <error-znode>'s /running znode is deleted triggering node down
//               and the node down processing will delete the <error-znode> and child <znode>s
//   ZCLIENT_CONFIGURED_ZNODE - to determine node when a node is added/deleted from configuration
//    /configured
//       /<znode>      - node in static configuration
//   ZCLIENT_RUNNING_ZNODE - to determine operational status (node up/down)
//    /running
//       /<znode>      - node operational, i.e., monitor is running

#define ZCLIENT_MASTER_ZNODE_RETRY_COUNT 60

typedef list<string>    ZNodeList_t;

// The following functions must be implemented in the calling program.
// - HandleMyNodeExpiration() is invoked when the monitor's session expires, or
//                            the monitor's znode expires or
//                            quorum communication fails
// - HandleNodeExpiration(nodeName) is invoked when the znode associated with
//                                  the nodeName passed in expires.
extern void HandleMyNodeExpiration( void );
extern void HandleNodeChange( const char *nodeName );
extern void HandleNodeConfigurationChange( void );
extern void HandleNodeCreated( const char *nodeName );
extern void HandleNodeError( const char *nodeName );
extern void HandleNodeExpiration( const char *nodeName );

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
        ZC_CLUSTER,       // check all cluster znodes
        ZC_ZNODE_CHANGED, // check znode change
        ZC_ZNODE_CHILD,   // check znode child change
        ZC_ZNODE_CREATED, // check znode created
        ZC_ZNODE_DELETED, // check znode delete
        ZC_MYZNODE,       // check this monitor's znode
        ZC_STOP,          // stop monitoring
        ZC_SHUTDOWN       // thread exit 
    } ZClientState_t;

    CZClient( const char *quorumHosts
            , const char *rootZNode
            , const char *instanceZNode );
    ~CZClient( void );
    
    void    ClusterWatchEnabledSet( bool enabled ) { CAutoLock lock(getLocker()); clusterWatchEnabled_ = enabled; }
    int     ConfiguredZNodeCreate( const char *nodeName );
    int     ConfiguredZNodeDelete( const char *nodeName );
    int     ConfiguredZNodeWatchAdd( void );
    int     ConfiguredZNodeWatchDelete( void );
    void    ConfiguredZNodesDelete( void );
    int     ConfiguredZNodesGet( String_vector *children );
    int     ErrorZNodeCreate( const char *errorNode );
    int     ErrorZNodeWatchAdd( void );
    int     ErrorZNodeWatchDelete( void );
    void    ErrorZNodesDelete( void );
    int     ErrorZNodesGet( String_vector *children, bool doRetries=true );
    int     ErrorZNodesGetChild( const char *errorNode, String_vector *children );
    void    HandleErrorChildZNodesForZNodeChild( const char *childNode, bool doRetries=false );
    bool    IsRunningZNodeExpired( const char *nodeName, int &zerr );
    const char* MasterWaitForAndReturn( bool doWait );
    int     MasterZNodeCreate( const char *nodeName );
    int     MasterZNodeDelete( const char *nodeName );
    void    MonitorCluster( void );
    int     RunningZNodeDelete( const char *nodeName );
    int     RunningZNodeWatchAdd( const char *nodeName );
    void    RunningZNodesDelete( void );
    int     SessionTimeoutGet( void) { return( zkSessionTimeout_ ); }
    ZClientState_t StateGet( void ) { CAutoLock lock(getLocker()); return( shutdown_?ZC_SHUTDOWN:state_ ); }
    void    StateSet( ZClientState_t state );
    void    StateSet( int type, ZClientState_t state, const char *znodePath );
    int     ShutdownWork( void );
    void    StartMonitoring( void );
    int     StartWork( void );
    void    StopMonitoring( void );
    void    TriggerCheck( int type, const char *znodePath );
    
private:
    void    ClusterMonitoringStart( void );
    void    ClusterMonitoringStop( void );
    void    ConfiguredZNodesWatchSet( void );
    void    EnabledSet( bool enabled ) { CAutoLock lock(getLocker()); enabled_ = enabled; }
    int     ErrorZNodeDelete( const char *errorNode, String_vector *errorChildNodes );
    int     ErrorChildZNodeDelete( const char *errorNode
                                 , const char *errorChildNode
                                 , String_vector *errorChildNodes );
    void    ErrorZNodesWatchSet( void );
    void    HandleChangedZNode( void );
    void    HandleChildZNode( void );
    void    HandleConfiguredZNodes( void );
    void    HandleCreatedZNode( void );
    void    HandleDeletedZNode( void );
    void    HandleErrorZNode( const char *errorNode, const char *childNode );
    void    HandleErrorZNodes( void );
    void    HandleErrorChildZNodes( const char *errorNode );
    int     InitializeZClient( void );
    bool    IsClusterWatchEnabled( void ) { CAutoLock lock(getLocker()); return( clusterWatchEnabled_ ); }
    bool    IsEnabled( void ) { CAutoLock lock(getLocker()); return( enabled_ ); }
    bool    IsZNodeMaster( const char *nodeName );
    void    MyRunningZNodeCheck( void );
    int     MyRunningZNodeCreate( void );
    int     RunningZNodeWatchDelete( const char *nodeName );
    void    RunningZNodesCheck( void );
    int     RunningZNodesGet( String_vector *children );
    void    RunningZNodesWatchSet( void );
    char*   StrCpyLeafZNode( char* znode, const char* znodePath );
    void    TimeToWakeUpSet( struct timespec &ts);
    int     ZNodeCreate( const char *znodePath
                       , const char *znodeData
                       , int flags 
                       , bool existOk=false );
    int     ZNodeDataGet( string &monZnode, string &nodeName, int &pnid );
    int     ZNodeDelete( string &znode );
    int     ZNodeWatchReset( string &monZnode );
    int     ZNodeWatchSet( string &monZnode );
    int     ZNodeWatchChildSet( string &parentznode );
    int     ZNodesTreeCreate( void );
    int     ZooExistRetry(zhandle_t *zh, const char *path, int watch, struct Stat *stat);

    pthread_t       threadId_;

    ZClientState_t  state_;        // Physical node's current operating state
    bool            enabled_;      // true when cluster monitoring enabled
    bool            clusterWatchEnabled_; // true when cluster monitoring enabled
    bool            resetMyZNodeFailedTime_; // set to trigger fail time reset
    bool            shutdown_;     // set to terminate all process and exit thread
    long            zcMonitoringRate_; // in seconds

    string          zkQuorumHosts_;
    string          zkRootNode_;
    string          zkRootNodeInstance_;
    stringstream    zkQuorumPort_;
    int             zkSessionTimeout_;
    struct timespec myZNodeFailedTime_;
    
    string          clusterZNodePath_;
    string          configuredZNodePath_;
    string          errorZNodePath_;
    string          masterZNodePath_;
    string          monitorZNodePath_;
    string          runningZNodePath_;
    
    ZNodeList_t     znodeChangedQueue_;
    ZNodeList_t     znodeChildQueue_;
    ZNodeList_t     znodeCreatedQueue_;
    ZNodeList_t     znodeDeletedQueue_;
};

#endif // ZCLIENT_H_
