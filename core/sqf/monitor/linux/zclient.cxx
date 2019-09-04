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
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <new>
#include <stdio.h>
#include <list>
#include <string>

#include "lock.h"
#include "msgdef.h"
#include "montrace.h"
#include "monlogging.h"
#include "reqqueue.h"
#include "type2str.h"
#include "zclient.h"
#include "pnode.h"

//
// The following specify the default values for the timers if the
// zclient cluster timer related environment variables are not defined.
//
// - ZCLIENT_MY_ZNODE_CHECKRATE is the rate the local monitor's znode is checked
#define ZCLIENT_MY_ZNODE_CHECKRATE            5 // seconds
#define ZCLIENT_SESSION_TIMEOUT              60 // seconds (1 minute)

// zookeeper connection retries
#define ZOOKEEPER_CHILD_RETRY_COUNT           5
#define ZOOKEEPER_RETRY_COUNT                 3
#define ZOOKEEPER_RETRY_WAIT                  1 // seconds

using namespace std;

extern char Node_name[MAX_PROCESSOR_NAME];
extern int MyPNID;
extern int MyNid;
extern int MyPid;                                               

extern CReqQueue ReqQueue;
extern CZClient    *ZClient;
extern CMonLog     *MonLog;
extern CNodeContainer *Nodes;
extern CNode *MyNode;
extern bool debugFlag;
extern bool IsAgentMode;
extern bool IsMaster;

static zhandle_t *ZHandle;
static clientid_t MyZooId;

void ZSessionWatcher( zhandle_t *zzh
                   , int type
                   , int state
                   , const char *path
                   , void *watcherCtx);

void FreeStringVector( struct String_vector *v )
{
    if ( v->data )
    {
        for ( int32_t i=0; i < v->count; i++ )
        {
            free( v->data[i] );
        }
        free( v->data );
        v->data = NULL;
        v->count = 0;
    }
}

static const char *ZClientStateStr( CZClient::ZClientState_t state )
{
    switch (state)
    {
        case CZClient::ZC_DISABLED:
            return "ZC_DISABLED";
        case CZClient::ZC_START:
            return "ZC_START";
        case CZClient::ZC_WATCH:
            return "ZC_WATCH";
        case CZClient::ZC_CLUSTER:
            return "ZC_CLUSTER";
        case CZClient::ZC_ZNODE_CREATED:
            return "ZC_ZNODE_CREATED";
        case CZClient::ZC_ZNODE_CHANGED:
            return "ZC_ZNODE_CHANGED";
        case CZClient::ZC_ZNODE_CHILD:
            return "ZC_ZNODE_CHILD";
        case CZClient::ZC_ZNODE_DELETED:
            return "ZC_ZNODE_DELETED";
        case CZClient::ZC_MYZNODE:
            return "ZC_MYZNODE";
        case CZClient::ZC_STOP:
            return "ZC_STOP";
        case CZClient::ZC_SHUTDOWN:
            return "ZC_SHUTDOWN";
        default:
            break;
    }
    return "ZClient State Invalid";
}

// ZClientThread main
static void *ZClientThread(void *arg)
{
    const char method_name[] = "ZClientThread";
    TRACE_ENTRY;

    // Parameter passed to the thread is an instance of the CommAccept object
    CZClient *zooClient = (CZClient *) arg;

    // Mask all allowed signals 
    sigset_t  mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGPROF); // allows profiling such as google profiler
    int rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], pthread_sigmask error=%d\n",
                 method_name, rc);
        mon_log_write(MON_ZCLIENT_ZCLIENTTHREAD_1, SQ_LOG_ERR, buf);
    }

    // Enter thread processing loop
    zooClient->MonitorCluster();

    TRACE_EXIT;
    return NULL;
}


void ZSessionWatcher( zhandle_t *zzh
                    , int type
                    , int state
                    , const char *path
                    , void *watcherCtx)
{
    const char method_name[] = "ZSessionWatcher";
    TRACE_ENTRY;

    watcherCtx = watcherCtx; // Make compiler happy!
    
    /*
     * Be careful using ZHandle here rather than zzh - as this may be mt code
     * the client lib may call the watcher before zookeeper_init returns 
     */
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        if ( path && strlen( path ) > 0 )
        {
            trace_printf( "%s@%d" " - Watcher %s state = %s for path %s\n"
                        , method_name, __LINE__
                        , ZooConnectionTypeStr( type )
                        , ZooConnectionStateStr( state )
                        , path );
        }
        else
        {
            trace_printf( "%s@%d" " - Watcher %s state = %s\n"
                        , method_name, __LINE__
                        , ZooConnectionTypeStr( type )
                        , ZooConnectionStateStr( state ) );
        }
    }

    if ( type == ZOO_SESSION_EVENT )
    {
        if ( state == ZOO_CONNECTED_STATE )
        {
            const clientid_t *id = zoo_client_id( zzh );
            if ( MyZooId.client_id == 0 || MyZooId.client_id != id->client_id )
            {
                MyZooId = *id;
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d" " - Got a new session id: 0x%llx\n"
                                , method_name, __LINE__
                                , static_cast<long long unsigned int>(MyZooId.client_id) );
                }
            }
        }
        else if ( state == ZOO_AUTH_FAILED_STATE )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], Error Zookeeper authentication failure. Node going down (terminating!) ...\n"
                    ,  method_name );
            mon_log_write(MON_ZCLIENT_ZSESSIONWATCHER_1, SQ_LOG_CRIT, buf);

            mon_failure_exit();
        }
        else if ( state == ZOO_EXPIRED_SESSION_STATE )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], Error Zookeeper session expired. Node going down (terminating!) ...\n"
                    ,  method_name );
            mon_log_write(MON_ZCLIENT_ZSESSIONWATCHER_2, SQ_LOG_CRIT, buf);

            mon_failure_exit();
        }
    }
    else
    {
        ZClient->TriggerCheck( type, path );
    }

    TRACE_EXIT;
}

CZClient::CZClient( const char *quorumHosts
                  , const char *rootNode
                  , const char *instanceNode )
         :threadId_(0)
         ,state_(ZC_DISABLED)
         ,enabled_(false)
         ,clusterWatchEnabled_(false)
         ,resetMyZNodeFailedTime_(true)
         ,shutdown_(false)
         ,zcMonitoringRate_(ZCLIENT_MY_ZNODE_CHECKRATE) // seconds
         ,zkQuorumHosts_(quorumHosts)
         ,zkRootNode_(rootNode)
         ,zkRootNodeInstance_(instanceNode)
         ,zkQuorumPort_("")
         ,zkSessionTimeout_(ZCLIENT_SESSION_TIMEOUT) // seconds
{
    const char method_name[] = "CZClient::CZClient";
    TRACE_ENTRY;

    memcpy(&eyecatcher_, "ZCLT", 4);
    
    char *zcMonitoringRateValueC;
    int zcMonitoringRateValue;
    if ( (zcMonitoringRateValueC = getenv( "SQ_MON_ZCLIENT_MY_ZNODE_CHECKRATE" )) )
    {
        // in seconds
        zcMonitoringRateValue = atoi( zcMonitoringRateValueC );
        zcMonitoringRate_ = zcMonitoringRateValue; // in seconds
    }
    
    char *zkSessionTimeoutC;
    int zkSessionTimeoutValue;
    if ( (zkSessionTimeoutC = getenv( "SQ_MON_ZCLIENT_SESSION_TIMEOUT" )) )
    {
        // in seconds
        zkSessionTimeoutValue = atoi( zkSessionTimeoutC );
        zkSessionTimeout_ = zkSessionTimeoutValue; // in seconds
    }
    
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d" " - ZClient monitoring rate in seconds=%ld\n"
                    , method_name, __LINE__, zcMonitoringRate_ );
        trace_printf( "%s@%d" " - ZClient session timeout in seconds =%d\n"
                    , method_name, __LINE__, zkSessionTimeout_ );
    }

    if ( zkQuorumHosts_.length() == 0 )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Zookeeper quorum port address not initialized\n"
                ,  method_name);
        mon_log_write(MON_ZCLIENT_ZCLIENT_1, SQ_LOG_ERR, buf);

        mon_failure_exit();
    }
    else
    {
        zkQuorumPort_ << zkQuorumHosts_.c_str();

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d zkQuorumPort is: %s\n"
                        , method_name, __LINE__
                        , zkQuorumPort_.str( ).c_str( ));
        }
    }

    // Initialize zookeeper
    zoo_deterministic_conn_order( 0 ); // non-deterministic order for client connections
    ZHandle = zookeeper_init( zkQuorumPort_.str( ).c_str( )
                       , ZSessionWatcher
                       , zkSessionTimeout_ * 1000
                       , &MyZooId
                       , 0
                       , 0 );
    if ( ZHandle == 0 )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zookeeper_init() failed for host:port %s\n"
                , method_name, zkQuorumPort_.str( ).c_str( ));
        mon_log_write(MON_ZCLIENT_ZCLIENT_2, SQ_LOG_ERR, buf);

        mon_failure_exit();
    }
    
    int rc = InitializeZClient();
    if ( rc )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Failed ZClient initialization (%s)\n"
                , method_name, zerror(rc) );
        mon_log_write(MON_ZCLIENT_ZCLIENT_3, SQ_LOG_ERR, buf);

        mon_failure_exit();
    }

    ConfiguredZNodesDelete();
    ErrorZNodesDelete();

    TRACE_EXIT;
}

CZClient::~CZClient( void )
{
    const char method_name[] = "CZClient::~CZClient";
    TRACE_ENTRY;

    memcpy(&eyecatcher_, "zclt", 4);

    if (ZHandle)
    {
        ConfiguredZNodesDelete();
        ErrorZNodesDelete();
        RunningZNodeDelete( Node_name );
        zookeeper_close(ZHandle);
        ZHandle = 0;
    }

    TRACE_EXIT;
}

void CZClient::ClusterMonitoringStart( void )
{
    const char method_name[] = "CZClient::ClusterMonitoringStart";
    TRACE_ENTRY;

    if ( !IsEnabled() )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Cluster monitoring started!\n\n", method_name, __LINE__ );
        }
        EnabledSet( true );
        StateSet( ZC_WATCH );
        CLock::wakeOne();
    }

    TRACE_EXIT;
}

void CZClient::ClusterMonitoringStop( void )
{
    const char method_name[] = "CZClient::ClusterMonitoringStop";
    TRACE_ENTRY;

    if ( IsEnabled() )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "\n%s@%d Cluster monitoring stopped!\n", method_name, __LINE__ );
        }
        ClusterWatchEnabledSet( false );
        EnabledSet( false );
        StateSet( ZC_DISABLED );
        CLock::wakeOne();
    }

    TRACE_EXIT;
}

int CZClient::ConfiguredZNodeCreate( const char *nodeName )
{
    const char method_name[] = "CZClient::ConfiguredZNodeCreate";
    TRACE_ENTRY;

    int rc;

    lock();

    stringstream newpath;
    newpath.str( "" );
    newpath << configuredZNodePath_.c_str() << "/"
            << nodeName;
    string configZnode = newpath.str( );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d ZNodeCreate(%s)\n"
                    , method_name, __LINE__
                    , configZnode.c_str() );
    }

    // Suppress error logging if error == ZNODEEXISTS
    rc = ZNodeCreate( configZnode.c_str(), NULL, 0, true );

    unlock();

    TRACE_EXIT;
    return(rc);
}

int CZClient::ConfiguredZNodeDelete( const char *nodeName )
{
    const char method_name[] = "CZClient::ConfiguredZNodeDelete";
    TRACE_ENTRY;

    int rc;

    lock();

    stringstream newpath;
    newpath.str( "" );
    newpath << configuredZNodePath_.c_str() << "/"
            << nodeName;
    string configZnode = newpath.str( );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d ZNodeCreate(%s)\n"
                    , method_name, __LINE__
                    , configZnode.c_str() );
    }

    rc = ZNodeDelete( configZnode );

    unlock();

    TRACE_EXIT;
    return(rc);
}

void CZClient::ConfiguredZNodesDelete( void )
{
    const char method_name[] = "CZClient::ConfiguredZNodesDelete";
    TRACE_ENTRY;

    int rc = -1;
    struct String_vector nodes;

    rc = ConfiguredZNodesGet( &nodes );
    if ( rc != ZOK && rc != ZNONODE )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ConfiguredZNodesGet() failed!\n"
                , method_name );
        mon_log_write(MON_ZCLIENT_CONFIGZNODESDELETE_1, SQ_LOG_ERR, buf);
        CLock::wakeOne();
        return;
    }

    stringstream newpath;
    string configznode;

    if ( nodes.count > 0 )
    {
        for (int i = 0; i < nodes.count ; i++ )
        {
            newpath.str( "" );
            newpath << configuredZNodePath_.c_str() << "/"
                    << nodes.data[i];
            configznode = newpath.str( );
        
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d Deleting configznode=%s\n"
                            , method_name, __LINE__
                            , configznode.c_str() );
            }

            ZNodeDelete( configznode );
        }
        FreeStringVector( &nodes );
    }

    TRACE_EXIT;
}

int CZClient::ConfiguredZNodesGet( String_vector *nodes )
{
    const char method_name[] = "CZClient::ConfiguredZNodesGet";
    TRACE_ENTRY;

    bool found = false;
    int rc = -1;
    int retries = 0;
    Stat stat;

    string configznodes( configuredZNodePath_.c_str() );

    nodes->count = 0;
    nodes->data = NULL;

    while ( !found )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d configznodes=%s\n"
                        , method_name, __LINE__, configznodes.c_str() );
        }
        // Verify the existence of the parent
        rc = ZooExistRetry( ZHandle, configznodes.c_str( ), 0, &stat );
        if ( rc == ZNONODE )
        {
            if (retries > 10)
                break;
            retries++;    
            continue;
        }
        else if ( rc == ZOK )
        {
            // Now get the list of available znodes in the cluster.
            //
            // This will return child znodes for each monitor process that has
            // registered, including this process.
            rc = zoo_get_children( ZHandle, configznodes.c_str( ), 0, nodes );
            if ( rc == ZOK )
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d nodes.count=%d\n"
                                , method_name, __LINE__
                                , nodes->count );
                }
                found = true;
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], zoo_get_children(%s) failed with error %s\n"
                        ,  method_name, configznodes.c_str( ), zerror(rc));
                mon_log_write(MON_ZCLIENT_CONFIGZNODESGET_1, SQ_LOG_ERR, buf);
                break;
            }
        }
        else  // error
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], zoo_exists(%s) failed with error %s\n"
                    ,  method_name, configznodes.c_str( ), zerror(rc));
            mon_log_write(MON_ZCLIENT_CONFIGZNODESGET_2, SQ_LOG_ERR, buf);
            break;
        }
    }

    TRACE_EXIT;
    return( rc );
}

void CZClient::ConfiguredZNodesWatchSet( void )
{
    const char method_name[] = "CZClient::ConfiguredZNodesWatchSet";
    TRACE_ENTRY;

    int rc;

    stringstream configpath;
    string confignode;

    configpath.str( "" );
    configpath << configuredZNodePath_.c_str();
    confignode = configpath.str( );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Setting watch set on confignode=%s\n"
                    , method_name, __LINE__
                    , confignode.c_str() );
    }

    rc = ZNodeWatchChildSet( confignode );
    if ( rc != ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ZNodeWatchChildSet(%s) failed!\n"
                , confignode.c_str()
                , method_name );
        mon_log_write(MON_ZCLIENT_CONFIGZNODESWATCHSET_1, SQ_LOG_ERR, buf);

        TRACE_EXIT;
        return;
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Watch set on confignode=%s\n"
                        , method_name, __LINE__
                        , confignode.c_str() );
        }
    }
    
    TRACE_EXIT;
}

int CZClient::ConfiguredZNodeWatchAdd( void )
{
    const char method_name[] = "CZClient::ConfiguredZNodeWatchAdd";
    TRACE_ENTRY;

    int rc;
    string configznode = configuredZNodePath_.c_str();

    lock();
    rc = ZNodeWatchSet( configznode );
    unlock();
    if ( rc != ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ZNodeWatchSet(%s) failed!\n"
                , method_name
                , configznode.c_str() );
        mon_log_write(MON_ZCLIENT_CONFZNODEWATCHADD_1, SQ_LOG_ERR, buf);
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Watch set on configznode=%s\n"
                        , method_name, __LINE__
                        , configznode.c_str() );
        }
    }

    TRACE_EXIT;
    return(rc);
}

int CZClient::ConfiguredZNodeWatchDelete( void )
{
    const char method_name[] = "CZClient::ConfiguredZNodeWatchDelete";
    TRACE_ENTRY;

    int rc = -1;

    string configznode = configuredZNodePath_.c_str();

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Deleting configznode(%s)\n"
                    , method_name, __LINE__
                    , configznode.c_str() );
    }
    rc = ZNodeWatchReset( configznode );
    if ( rc == ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], configznode (%s) deleted!\n"
                , method_name, configznode.c_str() );
        mon_log_write(MON_ZCLIENT_CONFZNODEWATCHDELETE_1, SQ_LOG_INFO, buf);
    }

    TRACE_EXIT;
    return( rc );
}

int CZClient::ErrorZNodeCreate( const char *errorNode )
{
    const char method_name[] = "CZClient::ErrorZNodeCreate";
    TRACE_ENTRY;

    int rc;
    int zerr;

    lock();
    if ( IsRunningZNodeExpired( errorNode, zerr ) )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Running znode %s already expired (%s)\n"
                        , method_name, __LINE__
                        , errorNode
                        , zerror(zerr) );
        }
        unlock();
        return(ZOK);
    }
    unlock();
    pthread_yield();
    lock();

    stringstream errorpath;
    errorpath.str( "" );
    errorpath << errorZNodePath_.c_str() << "/"
              << errorNode;
    string errorznode = errorpath.str( );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Error ZNodeCreate(%s)\n"
                    , method_name, __LINE__
                    , errorznode.c_str() );
    }

    // Suppress error logging if error == ZNODEEXISTS
    rc = ZNodeCreate( errorznode.c_str(), NULL, 0, true );

    errorpath.str( "" );
    errorpath << errorZNodePath_.c_str() << "/"
              << errorNode << "/"
              << Node_name;
    errorznode = errorpath.str( );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Error child ZNodeCreate(%s)\n"
                    , method_name, __LINE__
                    , errorznode.c_str() );
    }

    // Suppress error logging if error == ZNODEEXISTS
    rc = ZNodeCreate( errorznode.c_str(), NULL, 0, true );

    unlock();

    TRACE_EXIT;
    return(rc);
}

// The errorNode is the znode which contains more than one errorChildNodes
// and whose corresponding running znode is deleted to bring its node down
// (see CZClient::HandleErrorChildZNodes())
// The possibility exist that each errorChildNode is also an errorNode under
// errorZNodePath_ if the errorNode passed in could not communicate with
// one or more errorChildNodes.
// Therefore, the each errorChildNode that is also an errorNode and it child 
// znode must be also be deleted. 
// For example, if the error znode tree is as follows:
//   o node-b is the errorNode
//       /trafodion/1/cluster/error/node-a/node-b
//       /trafodion/1/cluster/error/node-b/node-a
//       /trafodion/1/cluster/error/node-b/node-c
//       /trafodion/1/cluster/error/node-c/node-b
//   o Therefore,
//       ErrorZNodeDelete( node-b, errorChildNodes-of-node-b )
//           Delete(/trafodion/1/cluster/error/node-a/node-b)
//           Delete(/trafodion/1/cluster/error/node-a)
//           Delete(/trafodion/1/cluster/error/node-c/node-b)
//           Delete(/trafodion/1/cluster/error/node-c)
//           Delete(/trafodion/1/cluster/error/node-b/node-a)
//           Delete(/trafodion/1/cluster/error/node-b/node-b)
//           Delete(/trafodion/1/cluster/error/node-b)
int CZClient::ErrorZNodeDelete( const char *errorNode, String_vector *errorChildNodes )
{
    const char method_name[] = "CZClient::ErrorZNodeDelete";
    TRACE_ENTRY;

    int rc = -1;
    struct String_vector childnodes;

    lock();

    stringstream errorpath;
    stringstream childpath;
    string errorznode;
    string childznode;

    errorpath.str( "" );
    errorpath << errorZNodePath_.c_str() << "/"
              << errorNode;
    errorznode = errorpath.str( );

retry:

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        for (int i = 0; i < errorChildNodes->count ; i++ )
        {
            trace_printf( "%s@%d errorNode=%s, errorChildNodes.count=%d, errorChildNode[%d]=%s\n"
                        , method_name, __LINE__
                        , errorNode
                        , errorChildNodes->count
                        , i
                        , errorChildNodes->data[i] );
        }
        trace_printf( "%s@%d Processing delete of errorznode=%s\n"
                    , method_name, __LINE__
                    , errorznode.c_str() );
    }

    if ( errorChildNodes->count > 0 )
    {
        for (int j = 0; j < errorChildNodes->count ; j++ )
        {
            rc = ErrorZNodesGetChild( errorChildNodes->data[j], &childnodes );
            if (rc == ZOK)
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d errorNode=%s, errorChildNode=%s, childnodes.count=%d\n"
                                , method_name, __LINE__
                                , errorNode
                                , errorChildNodes->data[j]
                                , childnodes.count );
                }

                if (strcmp( errorChildNodes->data[j], errorNode) == 0)
                {
                    FreeStringVector( &childnodes );
                    continue;
                }

                if (childnodes.count == 1 )
                {
                    ErrorChildZNodeDelete( errorNode, errorChildNodes->data[j], &childnodes );
                }
                FreeStringVector( &childnodes );
            }

            childpath.str( "" );
            childpath << errorZNodePath_ << "/"
                      << errorNode << "/"
                      << errorChildNodes->data[j];
            childznode = childpath.str( );

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d Deleting childznode=%s\n"
                            , method_name, __LINE__
                            , childznode.c_str() );
            }
            
            ZNodeDelete( childznode );
        }
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Deleting errorznode=%s\n"
                    , method_name, __LINE__
                    , errorznode.c_str() );
    }

    rc = ZNodeDelete( errorznode );
    if (rc == ZNOTEMPTY)
    {
        FreeStringVector( errorChildNodes );
        rc = ErrorZNodesGetChild( errorNode, errorChildNodes );
        if ( rc != ZOK && rc != ZNONODE)
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], ErrorZNodesGetChild() failed!\n"
                    , method_name );
            mon_log_write(MON_ZCLIENT_HNDLEERRORCHILDZNODES_1, SQ_LOG_ERR, buf);
            CLock::wakeOne();
            return(rc);
        }
    
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Retry deleting errorznode=%s\n"
                        , method_name, __LINE__
                        , errorznode.c_str() );
        }

        goto retry;
    }

    unlock();

    TRACE_EXIT;
    return(rc);
}

int CZClient::ErrorChildZNodeDelete( const char *errorNode
                                   , const char *errorChildNode
                                   , String_vector *errorChildNodes )
{
    const char method_name[] = "CZClient::ErrorChildZNodeDelete";
    TRACE_ENTRY;

    int rc1 = -1;
    int rc2 = -1;
    stringstream errorpath;
    stringstream childpath;
    string errorznode;
    string errorchildznode;
    string childznode;

    errorpath.str( "" );
    errorpath << errorZNodePath_.c_str() << "/"
              << errorNode << "/"
              << errorChildNode;
    errorchildznode = errorpath.str( );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        for (int i = 0; i < errorChildNodes->count ; i++ )
        {
            trace_printf( "%s@%d errorNode=%s, errorChildNode=%s, errorChildNodes.count=%d, errorChildNode[%d]=%s\n"
                        , method_name, __LINE__
                        , errorNode
                        , errorChildNode
                        , errorChildNodes->count
                        , i
                        , errorChildNodes->data[i] );
        }
        trace_printf( "%s@%d Processing delete of errorchildznode=%s\n"
                    , method_name, __LINE__
                    , errorchildznode.c_str() );
    }

    if ( errorChildNodes->count > 0 )
    {
        for (int j = 0; j < errorChildNodes->count ; j++ )
        {
            if (strcmp( errorChildNodes->data[j], errorNode) == 0)
            {
                childpath.str( "" );
                childpath << errorZNodePath_ << "/"
                          << errorChildNode << "/"
                          << errorNode;
                childznode = childpath.str( );

                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d Deleting childznode=%s\n"
                                , method_name, __LINE__
                                , childznode.c_str() );
                }

                rc1 = ZNodeDelete( childznode );

                childpath.str( "" );
                childpath << errorZNodePath_ << "/"
                          << errorChildNode;
                childznode = childpath.str( );
    
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d Deleting childznode=%s\n"
                                , method_name, __LINE__
                                , childznode.c_str() );
                }
                
                rc2 = ZNodeDelete( childznode );
            }
        }
    }

    TRACE_EXIT;
    return((rc1 != ZOK)?rc1:rc2);
}

int CZClient::ErrorZNodesGet( String_vector *nodes, bool doRetries )
{
    const char method_name[] = "CZClient::ErrorZNodesGet";
    TRACE_ENTRY;

    bool found = false;
    int rc = -1;
    int retries = 0;
    Stat stat;

    string errorznodes( errorZNodePath_.c_str() );

    nodes->count = 0;
    nodes->data = NULL;

    while ( !found )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d errorznode=%s\n"
                        , method_name, __LINE__, errorznodes.c_str() );
        }

        // Verify the existence of the parent
        rc = ZooExistRetry( ZHandle, errorznodes.c_str( ), 0, &stat );
        if ( rc == ZNONODE )
        {
            if (doRetries)
            {
                if (retries > ZOOKEEPER_RETRY_COUNT)
                    break;
                retries++;    
                continue;
            }
        }
        else if ( rc == ZOK )
        {
            // Now get the list of available znodes in the cluster.
            //
            // This will return child znodes for each monitor process that has
            // registered, including this process.
            rc = zoo_get_children( ZHandle, errorznodes.c_str( ), 0, nodes );
            if ( rc == ZOK )
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d errorznode=%s, errornodes.count=%d\n"
                                , method_name, __LINE__
                                , errorznodes.c_str()
                                , nodes->count );
                    for (int i = 0; i < nodes->count ; i++ )
                    {
                        trace_printf( "%s@%d errornodes[%d]=%s\n"
                                    , method_name, __LINE__
                                    , i
                                    , nodes->data[i] );
                    }
                }
                if (doRetries)
                {
                    if ( nodes->count && nodes->count < 2 )
                    {
                        unlock();
                        sleep(ZOOKEEPER_RETRY_WAIT);
                        lock();
                        if (retries < ZOOKEEPER_CHILD_RETRY_COUNT)
                        {
                            // Wait a bit to see if at least one other node is
                            // having communications problems with the same node
                            retries++;    
                            continue;
                        }
                        found = true;
                    }
                    else
                    {
                        unlock();
                        sleep(ZOOKEEPER_RETRY_WAIT);
                        lock();
                        if (retries > ZOOKEEPER_CHILD_RETRY_COUNT)
                            break;
                        retries++;    
                        continue;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], zoo_get_children(%s) failed with error %s\n"
                        ,  method_name, errorznodes.c_str( ), zerror(rc));
                mon_log_write(MON_ZCLIENT_ERRORZNODESGET_1, SQ_LOG_ERR, buf);
                break;
            }
        }
        else  // error
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], zoo_exists(%s) failed with error %s\n"
                    ,  method_name, errorznodes.c_str( ), zerror(rc));
            mon_log_write(MON_ZCLIENT_ERRORZNODESGET_2, SQ_LOG_ERR, buf);
            break;
        }
    }

    TRACE_EXIT;
    return( rc );
}

int CZClient::ErrorZNodesGetChild( const char *errorNode, String_vector *childnodes )
{
    const char method_name[] = "CZClient::ErrorZNodesGetChild";
    TRACE_ENTRY;

    bool found = false;
    int rc = -1;
    int retries = 0;
    Stat stat;

    stringstream ss;
    ss.str( "" );
    ss << errorZNodePath_.c_str() << "/"
       << errorNode;
    string errorchildznode( ss.str( ) );

    childnodes->count = 0;
    childnodes->data = NULL;

    while ( !found )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d errorchildznode=%s\n"
                        , method_name, __LINE__, errorchildznode.c_str() );
        }
        // Verify the existence of the parent
        rc = ZooExistRetry( ZHandle, errorchildznode.c_str( ), 0, &stat );
        if ( rc == ZNONODE )
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d errorchildznode=%s does not exist!\n"
                            , method_name, __LINE__
                            , errorchildznode.c_str( ) );
            }
            break;
        }
        else if ( rc == ZOK )
        {
            // Now get the list of available znodes in the cluster.
            //
            // This will return child znodes for each monitor process that has
            // registered, including this process.
            rc = zoo_get_children( ZHandle, errorchildznode.c_str( ), 0, childnodes );

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d errorNode=%s, childnodes.count=%d\n"
                            , method_name, __LINE__
                            , errorNode
                            , childnodes->count );
            }

            if ( childnodes->count > 0 )
            {
                found = true;
            }
            else
            {
                sleep(ZOOKEEPER_RETRY_WAIT);
                if (retries > ZOOKEEPER_CHILD_RETRY_COUNT)
                    break;
                retries++;    
                continue;
            }
        }
        else  // error
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], zoo_exists() for %s failed with error %s\n"
                    ,  method_name, errorchildznode.c_str( ), zerror(rc));
            mon_log_write(MON_ZCLIENT_ERRORCHILDZNODESGET_1, SQ_LOG_ERR, buf);
            break;
        }
    }

    TRACE_EXIT;
    return( rc );
}

void CZClient::ErrorZNodesDelete( void )
{
    const char method_name[] = "CZClient::ErrorZNodesDelete";
    TRACE_ENTRY;

    int rc = -1;
    struct String_vector errornodes;
    struct String_vector childnodes;

    lock();
    rc = ErrorZNodesGet( &errornodes );
    unlock();
    if ( rc != ZOK && rc != ZNONODE )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ErrorZNodesGet() failed!\n"
                , method_name );
        mon_log_write(MON_ZCLIENT_ERRORZNODESDELETE_1, SQ_LOG_ERR, buf);
        CLock::wakeOne();
        return;
    }

    stringstream errorpath;
    stringstream childpath;
    string errorznode;
    string childznode;

    if ( errornodes.count > 0 )
    {
        for (int i = 0; i < errornodes.count ; i++ )
        {
            errorpath.str( "" );
            errorpath << errorZNodePath_.c_str() << "/"
                      << errornodes.data[i];
            errorznode = errorpath.str( );
        
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d Deleting errorznode=%s\n"
                            , method_name, __LINE__
                            , errorznode.c_str() );
            }

            rc = ErrorZNodesGetChild( errornodes.data[i], &childnodes );
            if ( rc != ZOK && rc != ZNONODE )
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], ErrorZNodesGetChild() failed!\n"
                        , method_name );
                mon_log_write(MON_ZCLIENT_ERRORZNODESDELETE_2, SQ_LOG_ERR, buf);
                CLock::wakeOne();
                return;
            }

            if ( childnodes.count > 0 )
            {
                for (int j = 0; j < childnodes.count ; j++ )
                {
                    childpath.str( "" );
                    childpath << errorZNodePath_.c_str() << "/"
                              << errornodes.data[i] << "/"
                              << childnodes.data[j];
                    childznode = childpath.str( );
                
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    {
                        trace_printf( "%s@%d Deleting childznode=%s\n"
                                    , method_name, __LINE__
                                    , childznode.c_str() );
                    }
        
                    ZNodeDelete( childznode );
                }
            }

            FreeStringVector( &childnodes );
            ZNodeDelete( errorznode );
        }
        FreeStringVector( &errornodes );
    }

    TRACE_EXIT;
}

void CZClient::ErrorZNodesWatchSet( void )
{
    const char method_name[] = "CZClient::ErrorZNodesWatchSet";
    TRACE_ENTRY;

    int rc;

    stringstream errorpath;
    string errornode;

    errornode = errorZNodePath_.c_str();

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Setting watch set on errornode=%s\n"
                    , method_name, __LINE__
                    , errornode.c_str() );
    }

    rc = ZNodeWatchChildSet( errornode );
    if ( rc != ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ZNodeWatchChildSet(%s) failed!\n"
                , errornode.c_str()
                , method_name );
        mon_log_write(MON_ZCLIENT_ERRORZNODESWATCHSET_1, SQ_LOG_ERR, buf);

        TRACE_EXIT;
        return;
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Watch set on errornode=%s\n"
                        , method_name, __LINE__
                        , errornode.c_str() );
        }
    }
    
    TRACE_EXIT;
}

int CZClient::ErrorZNodeWatchAdd( void )
{
    const char method_name[] = "CZClient::ErrorZNodeWatchAdd";
    TRACE_ENTRY;

    int rc;
    string errorznode = errorZNodePath_.c_str();

    lock();
    rc = ZNodeWatchSet( errorznode );
    unlock();
    if ( rc != ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ZNodeWatchSet(%s) failed!\n"
                , method_name
                , errorznode.c_str() );
        mon_log_write(MON_ZCLIENT_ERRORZNODEWATCHADD_1, SQ_LOG_ERR, buf);
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Watch set on errorznode=%s\n"
                        , method_name, __LINE__
                        , errorznode.c_str() );
        }
    }

    TRACE_EXIT;
    return(rc);
}

int CZClient::ErrorZNodeWatchDelete( void )
{
    const char method_name[] = "CZClient::ErrorZNodeWatchDelete";
    TRACE_ENTRY;

    int rc = -1;

    string errorznode = errorZNodePath_.c_str();

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Deleting errorznode(%s)\n"
                    , method_name, __LINE__
                    , errorznode.c_str() );
    }
    rc = ZNodeWatchReset( errorznode );
    if ( rc == ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], errorznode (%s) deleted!\n"
                , method_name, errorznode.c_str() );
        mon_log_write(MON_ZCLIENT_ERRORZNODEWATCHDELETE_1, SQ_LOG_INFO, buf);
    }

    TRACE_EXIT;
    return( rc );
}

void CZClient::HandleChangedZNode( void )
{
    const char method_name[] = "CZClient::HandleChangedZNode";
    TRACE_ENTRY;

    if ( IsClusterWatchEnabled() )
    {
        char  pathStr[MAX_PROCESSOR_NAME] = { 0 };
        char  nodeName[MAX_PROCESSOR_NAME] = { 0 };
        string znode;
    

        while (znodeChangedQueue_.size() != 0)
        {
            znode.assign( znodeChangedQueue_.front() );
    
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d" " - znodePath=%s, znodeChangedQueue_.size=%ld\n"
                            , method_name, __LINE__
                            , znode.c_str(), znodeChangedQueue_.size() );
            }
    
            znodeChangedQueue_.pop_front();
            
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d nodeName=%s\n"
                            , method_name, __LINE__
                            , strlen(nodeName) ? nodeName : "" );
            }
    
            HandleNodeChange( nodeName );
        }
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d ClusterWatchEnabled is NOT set!\n"
                        , method_name, __LINE__ );
        }
    }
    
    TRACE_EXIT;
}

void CZClient::HandleChildZNode( void )
{
    const char method_name[] = "CZClient::HandleChildZNode";
    TRACE_ENTRY;

    if ( IsClusterWatchEnabled() )
    {
        char  pathStr[MAX_PROCESSOR_NAME] = { 0 };
        char  nodeName[MAX_PROCESSOR_NAME] = { 0 };
        char *tkn = NULL;
        char *tknStart = pathStr;
        char *tknLast = NULL;
        string znode;

        while (znodeChildQueue_.size() != 0)
        {
            znode.assign( znodeChildQueue_.front() );
    
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d" " - znodePath=%s, znodeChildQueue_.size=%ld\n"
                            , method_name, __LINE__
                            , znode.c_str(), znodeChildQueue_.size() );
            }
    
            znodeChildQueue_.pop_front();
            
            if (znode.compare( configuredZNodePath_ ) == 0)
            {
                // The configuredZNodePath_ contains child znodes of each
                // node in the static configuration.
                // As node are added or deleted from the static configuration
                // a correspoding child znode is added or deleted under the
                // configuredZNodePath_
                HandleConfiguredZNodes();
            } 
            else if (znode.compare( errorZNodePath_ ) == 0)
            {
                HandleErrorZNodes();
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], Don't know how to handle children of znode=%s\n"
                        , method_name
                        , znode.c_str() );
                mon_log_write(MON_ZCLIENT_HANDLECHILDZNODE_1, SQ_LOG_ERR, buf);
            }
        }
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d ClusterWatchEnabled is NOT set!\n"
                        , method_name, __LINE__ );
        }
    }
    
    TRACE_EXIT;
}

void CZClient::HandleConfiguredZNodes( void )
{
    const char method_name[] = "CZClient::HandleConfiguredZNodes";
    TRACE_ENTRY;

    int rc = -1;
    struct String_vector confignodes;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Handling Configured ZNodes!\n"
                    , method_name, __LINE__ );
    }

    rc = ConfiguredZNodesGet( &confignodes );
    if ( rc != ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ConfiguredZNodesGet() failed!\n"
                , method_name );
        mon_log_write(MON_ZCLIENT_HANDLEERRORZNODES_1, SQ_LOG_ERR, buf);
        CLock::wakeOne();
        return;
    }

    stringstream configpath;
    string configznode;

    if ( confignodes.count > 0 )
    {
        for (int i = 0; i < confignodes.count ; i++ )
        {
            configpath.str( "" );
            configpath << configuredZNodePath_.c_str() << "/"
                       << confignodes.data[i];
            configznode = configpath.str( );
        
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d Handling configznode=%s\n"
                            , method_name, __LINE__
                            , configznode.c_str() );
            }
        }
        HandleNodeConfigurationChange();
        FreeStringVector( &confignodes );
    }

    TRACE_EXIT;
}

void CZClient::HandleCreatedZNode( void )
{
    const char method_name[] = "CZClient::HandleCreatedZNode";
    TRACE_ENTRY;

    if ( IsClusterWatchEnabled() )
    {
        char  pathStr[MAX_PROCESSOR_NAME] = { 0 };
        char  nodeName[MAX_PROCESSOR_NAME] = { 0 };
        char *tkn = NULL;
        char *tknStart = pathStr;
        char *tknLast = NULL;
        string znode;
    

        while (znodeCreatedQueue_.size() != 0)
        {
            znode.assign( znodeCreatedQueue_.front() );
    
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d" " - znodePath=%s, znodeCreatedQueue_.size=%ld\n"
                            , method_name, __LINE__
                            , znode.c_str(), znodeCreatedQueue_.size() );
            }
    
            znodeCreatedQueue_.pop_front();
            
            strcpy( pathStr, znode.c_str() );
    
            tknStart++; // skip the first '/'
            tkn = strtok( tknStart, "/" );
            do
            {
                tknLast = tkn;
                tkn = strtok( NULL, "/" );
            }
            while( tkn != NULL );
    
            strcpy( nodeName, tknLast );
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d nodeName=%s\n"
                            , method_name, __LINE__
                            , strlen(nodeName) ? nodeName : "" );
            }
    
            HandleNodeCreated( nodeName );
        }
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d ClusterWatchEnabled is NOT set!\n"
                        , method_name, __LINE__ );
        }
    }
    
    TRACE_EXIT;
}

void CZClient::HandleDeletedZNode( void )
{
    const char method_name[] = "CZClient::HandleDeletedZNode";
    TRACE_ENTRY;

    if ( IsClusterWatchEnabled() )
    {
        char  pathStr[MAX_PROCESSOR_NAME] = { 0 };
        char  nodeName[MAX_PROCESSOR_NAME] = { 0 };
        char *tkn = NULL;
        char *tknStart = pathStr;
        char *tknLast = NULL;
        string znode;
    

        while (znodeDeletedQueue_.size() != 0)
        {
            znode.assign( znodeDeletedQueue_.front() );
    
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d" " - znodePath=%s, znodeDeletedQueue_.size=%ld\n"
                            , method_name, __LINE__
                            , znode.c_str(), znodeDeletedQueue_.size() );
            }
    
            znodeDeletedQueue_.pop_front();
            
            strcpy( pathStr, znode.c_str() );
    
            tknStart++; // skip the first '/'
            tkn = strtok( tknStart, "/" );
            do
            {
                tknLast = tkn;
                tkn = strtok( NULL, "/" );
            }
            while( tkn != NULL );
    
            strcpy( nodeName, tknLast );
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d nodeName=%s\n"
                            , method_name, __LINE__
                            , strlen(nodeName) ? nodeName : "" );
            }
    
            // Invoke the callback to handle the node expiration
            HandleNodeExpiration( nodeName );
        }
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d ClusterWatchEnabled is NOT set!\n"
                        , method_name, __LINE__ );
        }
    }
    
    TRACE_EXIT;
}

void CZClient::HandleErrorZNode( const char *errorNode, const char *childNode )
{
    const char method_name[] = "CZClient::HandleErrorZNode";
    TRACE_ENTRY;

    int rc = -1;
    bool deleteErrorznode = false;
    struct String_vector childnodes;
    stringstream errorpath;
    stringstream childpath;
    string errorznode;
    string childznode;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Handling errorNode=%s\n"
                    , method_name, __LINE__
                    , errorNode );
    }

    rc = ErrorZNodesGetChild( errorNode, &childnodes );
    if ( rc != ZOK && rc != ZNONODE)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ErrorZNodesGetChild() failed!\n"
                , method_name );
        mon_log_write(MON_ZCLIENT_HANDLEERRORZNODE_1, SQ_LOG_ERR, buf);
        return;
    }

    if ( childnodes.count > 0 )
    {
        for (int i = 0; i < childnodes.count ; i++ )
        {
            if (strcmp( childnodes.data[i], childNode ) == 0)
            {
                errorpath.str( "" );
                errorpath << errorZNodePath_.c_str() << "/"
                          << errorNode;
                errorznode = errorpath.str( );

                childpath.str( "" );
                childpath << errorpath.str( ) << "/"
                          << childNode;
                childznode = childpath.str( );

                // Delete the parent errorznode if it only had one childznode
                if (childnodes.count == 1)
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    {
                        trace_printf( "%s@%d Deleting childznode=%s\n"
                                    , method_name, __LINE__
                                    , childznode.c_str() );
                    }
                    ZNodeDelete( childznode );
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    {
                        trace_printf( "%s@%d Deleting errorznode=%s\n"
                                    , method_name, __LINE__
                                    , errorznode.c_str() );
                    }
                    ZNodeDelete( errorznode );
                }
                else if (childnodes.count > 1)
                {
                    HandleErrorChildZNodes( errorNode );
                }
            }
        }
    }

    FreeStringVector( &childnodes );

    TRACE_EXIT;
}

void CZClient::HandleErrorZNodes( void )
{
    const char method_name[] = "CZClient::HandleErrorZNodes";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Handling Error ZNodes!\n"
                    , method_name, __LINE__ );
    }

    int rc = -1;
    struct String_vector errornodes;

    rc = ErrorZNodesGet( &errornodes, false );
    if ( rc != ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ErrorZNodesGet() failed!\n"
                , method_name );
        mon_log_write(MON_ZCLIENT_HANDLEERRORZNODES_1, SQ_LOG_ERR, buf);
        return;
    }

    stringstream errorpath;
    string errorznode;

    if ( errornodes.count > 0 )
    {
        for (int i = 0; i < errornodes.count ; i++ )
        {
            errorpath.str( "" );
            errorpath << errorZNodePath_.c_str() << "/"
                      << errornodes.data[i];
            errorznode = errorpath.str( );
        
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d Handling errorznode=%s\n"
                            , method_name, __LINE__
                            , errorznode.c_str() );
            }

            HandleErrorChildZNodes( errornodes.data[i] );
        }
        FreeStringVector( &errornodes );
    }

    TRACE_EXIT;
}

void CZClient::HandleErrorChildZNodes( const char *errorNode )
{
    const char method_name[] = "CZClient::HandleErrorChildZNodes";
    TRACE_ENTRY;

    int rc = -1;
    bool deleteErrorznode = false;
    struct String_vector childnodes;
    stringstream errorpath;
    stringstream childpath;
    string errorznode;
    string childznode;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Handling errorNode=%s\n"
                    , method_name, __LINE__
                    , errorNode );
    }

    rc = ErrorZNodesGetChild( errorNode, &childnodes );
    if ( rc != ZOK && rc != ZNONODE)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ErrorZNodesGetChild() failed!\n"
                , method_name );
        mon_log_write(MON_ZCLIENT_HNDLEERRORCHILDZNODES_1, SQ_LOG_ERR, buf);
        return;
    }

    if ( childnodes.count > 1 )
    {
        ErrorZNodeDelete( errorNode, &childnodes );
        // Delete the corresponding running znode which will trigger node down
        RunningZNodeDelete( errorNode );
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Bypassing errorNode=%s, childnodes.count=%d\n"
                        , method_name, __LINE__
                        , errorNode
                        , childnodes.count );
        }
    }

    FreeStringVector( &childnodes );

    TRACE_EXIT;
}

void CZClient::HandleErrorChildZNodesForZNodeChild( const char *childNode, bool doRetries )
{
    const char method_name[] = "CZClient::HandleErrorChildZNodesForZNodeChild";
    TRACE_ENTRY;

    int rc = -1;
    bool deleteErrorznode = false;
    struct String_vector errornodes;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Handling childNode=%s\n"
                    , method_name, __LINE__
                    , childNode );
    }

    rc = ErrorZNodesGet( &errornodes, doRetries );
    if ( rc != ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ErrorZNodesGet() failed!\n"
                , method_name );
        mon_log_write(MON_ZCLIENT_HNDLERRCHLZNFORZNCHL_1, SQ_LOG_ERR, buf);
        return;
    }

    stringstream errorpath;
    string errorznode;

    if ( errornodes.count > 0 )
    {
        for (int i = 0; i < errornodes.count ; i++ )
        {
            errorpath.str( "" );
            errorpath << errorZNodePath_.c_str() << "/"
                      << errornodes.data[i];
            errorznode = errorpath.str( );
        
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d Handling errorznode=%s\n"
                            , method_name, __LINE__
                            , errorznode.c_str() );
            }

            HandleErrorZNode( errornodes.data[i], childNode );
        }
        FreeStringVector( &errornodes );
    }

    TRACE_EXIT;
}

int CZClient::InitializeZClient( void )
{
    const char method_name[] = "CZClient::InitializeZClient";
    TRACE_ENTRY;

    int rc;
    int retries = 0;

    rc = ZNodesTreeCreate();

    while ( rc != ZOK && retries < ZOOKEEPER_RETRY_COUNT)
    {
        sleep(ZOOKEEPER_RETRY_WAIT);
        retries++;
        rc = ZNodesTreeCreate();
    }

    if ( rc == ZOK )
    {
        rc = MyRunningZNodeCreate();
    }

    TRACE_EXIT;
    return( rc );
}

bool CZClient::IsRunningZNodeExpired( const char *nodeName, int &zerr )
{
    const char method_name[] = "CZClient::IsRunningZNodeExpired";
    TRACE_ENTRY;

    bool  expired = false;
    int   rc = -1;
    Stat  stat;
    stringstream newpath;
    newpath.str( "" );
    newpath << runningZNodePath_.c_str() << "/"
            << nodeName;
    string monZnode = newpath.str( );

    zerr = ZOK;

    rc = ZooExistRetry( ZHandle, monZnode.c_str( ), 0, &stat );
    if ( rc == ZNONODE )
    {
        expired = true;
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d monZnode=%s does not exist!\n"
                        , method_name, __LINE__, monZnode.c_str() );
        }
    }
    else if ( rc == ZCONNECTIONLOSS || rc == ZOPERATIONTIMEOUT )
    {
        // Treat this as not expired until communication resumes
        expired = false;
        zerr = rc;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists() for %s failed with error %s\n"
                ,  method_name, monZnode.c_str( ), zerror(rc));
        mon_log_write(MON_ZCLIENT_ISZNODEEXPIRED_1, SQ_LOG_ERR, buf);
    }
    else if ( rc == ZOK )
    {
        expired = false;
    }
    else
    {
        expired = true;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists() for %s failed with error %s\n"
                ,  method_name, monZnode.c_str( ), zerror(rc));
        mon_log_write(MON_ZCLIENT_ISZNODEEXPIRED_2, SQ_LOG_CRIT, buf);
        switch ( rc )
        {
        case ZSYSTEMERROR:
        case ZRUNTIMEINCONSISTENCY:
        case ZDATAINCONSISTENCY:
        case ZMARSHALLINGERROR:
        case ZUNIMPLEMENTED:
        case ZBADARGUMENTS:
        case ZINVALIDSTATE:
        case ZSESSIONEXPIRED:
        case ZCLOSING:
            // Treat these error like a session expiration, since
            // we can't communicate with quorum servers
            HandleMyNodeExpiration();
            break;
        default:
            break;
        }
    }

    TRACE_EXIT;
    return( expired );
}

bool CZClient::IsZNodeMaster( const char *nodeName )
{
    const char method_name[] = "CZClient::IsZNodeMaster";
    TRACE_ENTRY;

    bool isMaster = false;
    string masterZNode;

    masterZNode.assign(MasterWaitForAndReturn( true ));
    
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d masterZNode=%s, nodeName=%s\n"
                    , method_name, __LINE__
                    , masterZNode.c_str()
                    , nodeName );
    }

    isMaster = (masterZNode.compare( nodeName ) == 0) ? true : false;

    TRACE_EXIT;
    return( isMaster );
}

const char* CZClient::MasterWaitForAndReturn( bool doWait )
{
    const char method_name[] = "CZClient::MasterWaitForAndReturn";
    TRACE_ENTRY;
    
    bool found = false;
    int rc = -1;
    int retries = 0;
    Stat stat;

    struct String_vector nodes = {0, NULL};
    string masterMonitor( masterZNodePath_.c_str() );

    // wait for ZCLIENT_MASTER_ZNODE_RETRY_COUNT minutes for giving up.  
    while ( (StateGet() != ZC_SHUTDOWN) && (!found) && (retries < ZCLIENT_MASTER_ZNODE_RETRY_COUNT)) 
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d masterMonitor path=%s\n"
                        , method_name, __LINE__, masterMonitor.c_str() );
        }

        if (MyNode && MyNode->IsPendingNodeDown())
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d MyNode IsPendingNodeDown=%s\n"
                            , method_name, __LINE__
                            , MyNode->IsPendingNodeDown()?"true":"false" );
            }
            break;
        }

        // Verify the existence of the parent ZCLIENT_MASTER_ZNODE
        rc = ZooExistRetry( ZHandle, masterMonitor.c_str( ), 0, &stat );
        
        if ( rc == ZNONODE )
        {
            if (doWait == false)
            {
                break;
            } 
            sleep(ZOOKEEPER_RETRY_WAIT);
            retries++;
            continue;
        }
        else if ( rc == ZOK )
        {
            // Now get the master znode that registered under the masterMonitor
            // znode.
            //
            // This will return one child znode for the monitor process that has
            // registered as the current master.
            rc = zoo_get_children( ZHandle, masterMonitor.c_str( ), 0, &nodes );
            if ( nodes.count > 0 )
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d nodes.count=%d\n"
                                , method_name, __LINE__
                                , nodes.count );
                }
                found = true;
            }
            else
            {
                if (doWait == false)
                {
                    break;
                }
                sleep(ZOOKEEPER_RETRY_WAIT);
                retries++;
                continue;
            }
        }
         
        else  // error
        { 
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d Error (MasterMonitor) MasterWaitForAndReturn() returned rc (%d), retries %d\n"
                        , method_name, __LINE__, rc, retries );
            }
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], ZooExistRetry() for %s failed with error %s\n"
                    ,  method_name, masterMonitor.c_str( ), zerror(rc));
            mon_log_write(MON_ZCLIENT_WAITFORRETURNMASTER_1, SQ_LOG_ERR, buf);
            break;
        }
    }
         
    //should we assert nodes.count == 1?
    if (found)
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d (MasterMonitor) Master Monitor found (%s/%s)\n"
                        , method_name, __LINE__, masterMonitor.c_str(), nodes.data[0] );
        }
        TRACE_EXIT;
        return nodes.data[0];
    }
    else
    {
      if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d (MasterMonitor) Master Monitor NOT found\n" , method_name, __LINE__);
        }
    }

    TRACE_EXIT;
    return NULL;
}

int CZClient::MasterZNodeCreate(  const char *nodeName )
{
    const char method_name[] = "CZClient::MasterZNodeCreate";
    TRACE_ENTRY;

    int rc;
    int retries = 0;

    stringstream masterpath;
    masterpath.str( "" );
    masterpath << masterZNodePath_.c_str() << "/"
               << nodeName;
            
    string monZnode = masterpath.str( );

    stringstream ss;
    ss.str( "" );
    ss <<nodeName << ":" << MyPNID;
    string monData = ss.str( ); 

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d ZNodeCreate(%s:%s)\n"
                    , method_name, __LINE__
                    , monZnode.c_str()
                    , monData.c_str() );
    }

    rc = ZNodeCreate( monZnode.c_str(), monData.c_str(), ZOO_EPHEMERAL );
    while ( ((rc == ZCONNECTIONLOSS) || (rc == ZOPERATIONTIMEOUT)) && retries < ZOOKEEPER_RETRY_COUNT)
    {
        sleep(ZOOKEEPER_RETRY_WAIT);
        retries++;
        rc = ZNodeCreate( monZnode.c_str(), monData.c_str(), ZOO_EPHEMERAL );
    }
    
    if (rc != ZOK)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ZNodeCreate(%s) failed with error %s\n"
                , method_name, monData.c_str(), zerror(rc) );
        mon_log_write(MON_ZCLIENT_MASTERZNODECREATE_1, SQ_LOG_ERR, buf);

        TRACE_EXIT;
        return(rc); // Return the error
    }
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d (MasterMonitor) Created master node for %s with rc = %d)\n"
                , method_name, __LINE__, monZnode.c_str( ), rc);
    }
    TRACE_EXIT;
    return(rc);
}

int CZClient::MasterZNodeDelete( const char *nodeName )
{
    const char method_name[] = "CZClient::MasterZNodeDelete";
    TRACE_ENTRY;
    
    int rc = -1;
    stringstream newpath;
    newpath.str( "" );
    newpath << masterZNodePath_.c_str() << "/"
            << nodeName;
           
    string znode = newpath.str( );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Deleting znode(%s)\n"
                    , method_name, __LINE__
                    , znode.c_str() );
    }
   
    rc = ZNodeDelete( znode );
    if ( rc == ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Master znode (%s) deleted!\n"
                , method_name, nodeName );
        mon_log_write(MON_ZCLIENT_MASTERZNODEDELETE_1, SQ_LOG_INFO, buf);
    }
    else if ( rc == ZNONODE )
    {
        // This is ok since we call it indiscriminately
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d (MasterMonitor) Master ZNode %s already deleted\n"
                        , method_name, __LINE__
                        , nodeName );
        }
    }
    
    TRACE_EXIT;
    return( rc );
}

// ZClient main processing loop
void CZClient::MonitorCluster()
{
    const char method_name[] = "CZClient::MonitorCluster";
    TRACE_ENTRY;

    int rc;
    struct timespec   timeout;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d thread %lx starting\n"
                    , method_name, __LINE__, threadId_);
    }

    if (zcMonitoringRate_ >= 0)
    {
        TimeToWakeUpSet( timeout );
    }

    while ( StateGet() != ZC_SHUTDOWN )
    {
        lock();
        if ( !IsEnabled() )
        {
            // Wait until enabled
            CLock::wait();
        }
        else
        {

            if (zcMonitoringRate_ < 0 || StateGet() == ZC_DISABLED)
            {
                // Wait until signaled
                CLock::wait();
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d" " - ZCluster signaled, state_=%s\n"
                                , method_name, __LINE__
                                , ZClientStateStr(StateGet()) );
                }
            }

            if (znodeDeletedQueue_.size() != 0)
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d - ZCluster signaling: "
                                  "ZC_ZNODE_DELETED, znodeDeletedQueue_=%ld\n"
                                , method_name, __LINE__
                                , znodeDeletedQueue_.size() );
                }
                StateSet( ZC_ZNODE_DELETED );
            }
            else if (znodeChildQueue_.size() != 0)
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d - ZCluster signaling: "
                                  "ZC_ZNODE_CHILD, znodeChildQueue_=%ld\n"
                                , method_name, __LINE__
                                , znodeChildQueue_.size() );
                }
                StateSet( ZC_ZNODE_CHILD );
            }
            else if (znodeCreatedQueue_.size() != 0)
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d - ZCluster signaling: "
                                  "ZC_ZNODE_CREATED, znodeCreatedQueue_=%ld\n"
                                , method_name, __LINE__
                                , znodeCreatedQueue_.size() );
                }
                StateSet( ZC_ZNODE_CREATED );
            }
            else if (znodeChangedQueue_.size() != 0)
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d - ZCluster signaling: "
                                  "ZC_ZNODE_CHANGED, znodeChangedQueue_=%ld\n"
                                , method_name, __LINE__
                                , znodeChangedQueue_.size() );
                }
                StateSet( ZC_ZNODE_CHANGED );
            }
            else
            {
                // Wait until signaled or timer expires
                rc = CLock::timedWait( &timeout );
                if ( rc != ETIMEDOUT  )
                {
                    if ( rc != 0 )
                    {
                        ClusterMonitoringStop();
                    }
                    else
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d" " - ZCluster signaled, state_=%s\n"
                                        , method_name, __LINE__
                                        , ZClientStateStr(StateGet()) );
                        }
                    }
                }
            }
        }

        switch ( StateGet() )
        {
            case ZC_START:
                ClusterMonitoringStart();
                break;
            case ZC_CLUSTER:
                if ( IsClusterWatchEnabled() )
                {
                    RunningZNodesCheck();
                    if (StateGet() != ZC_STOP)
                    {
                        StateSet( ZC_MYZNODE );
                    }
                }
                break;
            case ZC_WATCH:
                if ( !IsClusterWatchEnabled() )
                {
                    ConfiguredZNodesWatchSet();
                    ErrorZNodesWatchSet();
                    RunningZNodesWatchSet();
                    if (StateGet() != ZC_STOP)
                    {
                        ClusterWatchEnabledSet( true );
                        StateSet( ZC_MYZNODE );
                    }
                }
                break;
            case ZC_MYZNODE:
                if ( IsClusterWatchEnabled() )
                {
                    MyRunningZNodeCheck();
                }
                break;
            case ZC_ZNODE_CHANGED:
                if ( IsClusterWatchEnabled() )
                {
                    HandleChangedZNode();
                    StateSet( ZC_MYZNODE );
                }
                break;
            case ZC_ZNODE_CHILD:
                if ( IsClusterWatchEnabled() )
                {
                    HandleChildZNode();
                    StateSet( ZC_MYZNODE );
                }
                break;
            case ZC_ZNODE_CREATED:
                if ( IsClusterWatchEnabled() )
                {
                    HandleCreatedZNode();
                    StateSet( ZC_MYZNODE );
                }
                break;
            case ZC_ZNODE_DELETED:
                if ( IsClusterWatchEnabled() )
                {
                    HandleDeletedZNode();
                    StateSet( ZC_MYZNODE );
                }
                break;
            case ZC_STOP:
                ClusterMonitoringStop();
                break;
            default:
                break;
        }

        if (zcMonitoringRate_ >= 0)
        {
            TimeToWakeUpSet( timeout );
        }
        unlock();
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf("%s@%d thread %lx exiting\n"
                    , method_name,__LINE__, pthread_self());
    }

    TRACE_EXIT;
}

void CZClient::MyRunningZNodeCheck( void )
{
    const char method_name[] = "CZClient::MyRunningZNodeCheck";
    TRACE_ENTRY;

    int zerr;
    struct timespec currentTime;

    if ( IsClusterWatchEnabled() )
    {
        if (resetMyZNodeFailedTime_)
        {
            resetMyZNodeFailedTime_ = false;
            clock_gettime(CLOCK_REALTIME, &myZNodeFailedTime_);
            myZNodeFailedTime_.tv_sec += (SessionTimeoutGet() * 2);
#if 0
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d" " - Resetting MyZnode Fail Time %ld(secs)\n"
                            , method_name, __LINE__
                            , myZNodeFailedTime_.tv_sec );
            }
#endif
        }
        if (MyNode->IsPendingNodeDown())
        {
            return;
        }
        if ( ! IsRunningZNodeExpired( Node_name, zerr ) )
        {
            if ( zerr == ZCONNECTIONLOSS || zerr == ZOPERATIONTIMEOUT )
            {
                // Ignore transient errors with the quorum.
                // However, if longer than the session
                // timeout, handle it as a hard error.
                clock_gettime(CLOCK_REALTIME, &currentTime);
                if (currentTime.tv_sec > myZNodeFailedTime_.tv_sec)
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s], Zookeeper quorum comm error: %s - Handling my znode (%s) as expired! Node is going down.\n"
                            , method_name, zerror(zerr), Node_name );
                    mon_log_write(MON_ZCLIENT_MYRUNNINGZNODECHECK_1, SQ_LOG_ERR, buf);
                    HandleMyNodeExpiration();
                }
            }
            else
            {
                resetMyZNodeFailedTime_ = true;
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], My znode (%s) expired! Node is going down.\n"
                    , method_name, Node_name );
            mon_log_write(MON_ZCLIENT_MYRUNNINGZNODECHECK_2, SQ_LOG_ERR, buf);
            HandleMyNodeExpiration();
        }
    }
    
    TRACE_EXIT;
}

int CZClient::MyRunningZNodeCreate( void )
{
    const char method_name[] = "CZClient::MyRunningZNodeCreate";
    TRACE_ENTRY;

    int rc;
    char pnidStr[10];

    sprintf( pnidStr, "%d", MyPNID);

    stringstream newpath;
    newpath.str( "" );
    newpath << runningZNodePath_.c_str() << "/"
            << Node_name;
    string monZnode = newpath.str( );

    stringstream ss;
    ss.str( "" );
    ss << Node_name << ":" << pnidStr;
    string monData = ss.str( );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d ZNodeCreate(%s:%s)\n"
                    , method_name, __LINE__
                    , monZnode.c_str()
                    , monData.c_str() );
    }

    lock();
    // Clean up previous error znodes
    HandleErrorChildZNodes( Node_name );
    unlock();

    rc = ZNodeCreate( monZnode.c_str(), monData.c_str(), ZOO_EPHEMERAL );

    TRACE_EXIT;
    return(rc);
}

int CZClient::RunningZNodeDelete( const char *nodeName )
{
    const char method_name[] = "CZClient::RunningZNodeDelete";
    TRACE_ENTRY;

    int rc = -1;

    stringstream newpath;
    newpath.str( "" );
    newpath << runningZNodePath_.c_str() << "/"
            << nodeName;
    string monZnode = newpath.str( );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Deleting znode(%s)\n"
                    , method_name, __LINE__
                    , monZnode.c_str() );
    }

    if (strcmp( Node_name, nodeName) == 0)
    {
        // Clean up my error znode and children
        HandleErrorChildZNodes( Node_name );
        // Clean up error znodes and where I am their 'only' child
        lock();
        HandleErrorChildZNodesForZNodeChild( Node_name, true );
        unlock();
    }

    rc = ZNodeDelete( monZnode );
    if ( rc == ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], znode (%s) deleted!\n"
                , method_name, nodeName );
        mon_log_write(MON_ZCLIENT_RUNZNODEWATCHDELETE_1, SQ_LOG_INFO, buf);
    }

    TRACE_EXIT;
    return( rc );
}

int CZClient::RunningZNodeWatchAdd( const char *nodeName )
{
    const char method_name[] = "CZClient::RunningZNodeWatchAdd";
    TRACE_ENTRY;

    int rc;
    stringstream newpath;
    newpath.str( "" );
    newpath << runningZNodePath_.c_str() << "/"
            << nodeName;
    string monZnode = newpath.str( );

    lock();
    rc = ZNodeWatchSet( monZnode );
    unlock();
    if ( rc != ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], ZNodeWatchSet(%s) failed!\n"
                , method_name
                , monZnode.c_str() );
        mon_log_write(MON_ZCLIENT_RUNZNODEWATCHADD_1, SQ_LOG_ERR, buf);
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Watch set on monZnode=%s\n"
                        , method_name, __LINE__
                        , monZnode.c_str() );
        }
    }

    TRACE_EXIT;
    return(rc);
}

int CZClient::RunningZNodeWatchDelete( const char *nodeName )
{
    const char method_name[] = "CZClient::RunningZNodeWatchDelete";
    TRACE_ENTRY;

    int rc = -1;

    stringstream newpath;
    newpath.str( "" );
    newpath << runningZNodePath_.c_str() << "/"
            << nodeName;
    string monZnode = newpath.str( );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Deleting znode(%s)\n"
                    , method_name, __LINE__
                    , monZnode.c_str() );
    }
    rc = ZNodeWatchReset( monZnode );
    if ( rc == ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], znode (%s) deleted!\n"
                , method_name, nodeName );
        mon_log_write(MON_ZCLIENT_RUNZNODEWATCHDELETE_1, SQ_LOG_INFO, buf);
    }

    TRACE_EXIT;
    return( rc );
}

void CZClient::RunningZNodesCheck( void )
{
    const char method_name[] = "CZClient::RunningZNodesCheck";
    TRACE_ENTRY;

    int rc;
    struct String_vector nodes;

    if ( IsClusterWatchEnabled() )
    {
        rc = RunningZNodesGet( &nodes );
        if ( rc != ZOK )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], RunningZNodesGet() failed!\n"
                    , method_name );
            mon_log_write(MON_ZCLIENT_RUNZNODESCHECK_1, SQ_LOG_ERR, buf);
            StateSet( CZClient::ZC_STOP );
            CLock::wakeOne();
            return;
        }

        stringstream newpath;
        string monZnode;
        string nodeName;
        int    pnid = -1;
    
        if ( nodes.count > 0 )
        {
            for (int i = 0; i < nodes.count ; i++ )
            {
                newpath.str( "" );
                newpath << runningZNodePath_.c_str() << "/"
                        << nodes.data[i];
                string monZnode = newpath.str( );
            
                rc = ZNodeDataGet( monZnode, nodeName, pnid );
                if ( rc != ZOK )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s], ZNodeDataGet(%s) failed!\n"
                            , method_name
                            , monZnode.c_str() );
                    mon_log_write(MON_ZCLIENT_RUNZNODESCHECK_2, SQ_LOG_ERR, buf);
                }
                else
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    {
                        trace_printf( "%s@%d monZnode=%s, nodeName=%s, pnid=%d)\n"
                                    , method_name, __LINE__
                                    , monZnode.c_str(), nodeName.c_str(), pnid );
                    }
                }
            }
            FreeStringVector( &nodes );
        }
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d ClusterWatch is NOT set!\n"
                        , method_name, __LINE__ );
        }
    }
    
    TRACE_EXIT;
}

void CZClient::RunningZNodesDelete( void )
{
    const char method_name[] = "CZClient::RunningZNodesDelete";
    TRACE_ENTRY;

    int rc;
    struct String_vector nodes;

    rc = RunningZNodesGet( &nodes );
    if ( rc != ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], RunningZNodesGet() failed!\n"
                , method_name );
        mon_log_write(MON_ZCLIENT_RUNZNODESDELETE_1, SQ_LOG_ERR, buf);
        CLock::wakeOne();
        return;
    }

    stringstream newpath;
    string monZnode;
    string nodeName;
    int    pnid = -1;

    if ( nodes.count > 0 )
    {
        for (int i = 0; i < nodes.count ; i++ )
        {
            newpath.str( "" );
            newpath << runningZNodePath_.c_str() << "/"
                    << nodes.data[i];
            string monZnode = newpath.str( );
        
            rc = ZNodeDataGet( monZnode, nodeName, pnid );
            if ( rc != ZOK )
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], ZNodeDataGet(%s) failed!\n"
                        , method_name
                        , monZnode.c_str() );
                mon_log_write(MON_ZCLIENT_RUNZNODESDELETE_2, SQ_LOG_ERR, buf);
            }
            else
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d monZnode=%s, nodeName=%s, pnid=%d)\n"
                                , method_name, __LINE__
                                , monZnode.c_str(), nodeName.c_str(), pnid );
                }
                ZClient->RunningZNodeDelete( nodeName.c_str() );
                ZClient->MasterZNodeDelete( nodeName.c_str() );
            }
        }
        FreeStringVector( &nodes );
    }

    TRACE_EXIT;
}

int CZClient::RunningZNodesGet( String_vector *nodes )
{
    const char method_name[] = "CZClient::RunningZNodesGet";
    TRACE_ENTRY;

    bool found = false;
    int rc = -1;
    int retries = 0;
    Stat stat;

    string trafCluster( runningZNodePath_.c_str() );

    nodes->count = 0;
    nodes->data = NULL;

    while ( !found )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d trafCluster=%s\n"
                        , method_name, __LINE__, trafCluster.c_str() );
        }
        // Verify the existence of the parent
        rc = ZooExistRetry( ZHandle, trafCluster.c_str( ), 0, &stat );
        if ( rc == ZNONODE )
        {
            if (retries > ZOOKEEPER_RETRY_COUNT)
                break;
            retries++;    
            continue;
        }
        else if ( rc == ZOK )
        {
            // Now get the list of available znodes in the cluster.
            //
            // This will return child znodes for each monitor process that has
            // registered, including this process.
            rc = zoo_get_children( ZHandle, trafCluster.c_str( ), 0, nodes );
            if ( nodes->count > 0 )
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d nodes.count=%d\n"
                                , method_name, __LINE__
                                , nodes->count );
                }
                found = true;
            }
            else
            {
                sleep(ZOOKEEPER_RETRY_WAIT);
                if (retries > ZOOKEEPER_CHILD_RETRY_COUNT)
                    break;
                retries++;    
                continue;
            }
        }
        else  // error
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], zoo_exists() for %s failed with error %s\n"
                    ,  method_name, trafCluster.c_str( ), zerror(rc));
            mon_log_write(MON_ZCLIENT_RUNZNODESGET_1, SQ_LOG_ERR, buf);
            break;
        }
    }

    TRACE_EXIT;
    return( rc );
}

void CZClient::RunningZNodesWatchSet( void )
{
    const char method_name[] = "CZClient::RunningZNodesWatchSet";
    TRACE_ENTRY;

    int rc;
    struct String_vector nodes;

    if ( !IsClusterWatchEnabled() )
    {
        rc = RunningZNodesGet( &nodes );
        if ( rc != ZOK )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], RunningZNodesGet() failed!\n"
                    , method_name );
            mon_log_write(MON_ZCLIENT_RUNZNODESWATCHSET_1, SQ_LOG_ERR, buf);
            CLock::wakeOne();
            return;
        }

        stringstream runningpath;
        string runningznode;
    
        if ( nodes.count > 0 )
        {
            for (int i = 0; i < nodes.count ; i++ )
            {
                runningpath.str( "" );
                runningpath << runningZNodePath_.c_str() << "/"
                            << nodes.data[i];
                string runningznode = runningpath.str( );
            
                rc = ZNodeWatchSet( runningznode );
                if ( rc != ZOK )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s], ZNodeWatchSet(%s) failed!\n"
                            , runningznode.c_str()
                            , method_name );
                    mon_log_write(MON_ZCLIENT_RUNZNODESWATCHSET_2, SQ_LOG_ERR, buf);

                    FreeStringVector( &nodes );
                    TRACE_EXIT;
                    return;
                }
                else
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    {
                        trace_printf( "%s@%d Watch set on monZnode=%s\n"
                                    , method_name, __LINE__
                                    , runningznode.c_str() );
                    }
                }
            }
            FreeStringVector( &nodes );
        }
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Cluster watch already enabled!\n"
                        , method_name, __LINE__ );
        }
    }
    
    TRACE_EXIT;
}

int CZClient::ShutdownWork(void)
{
    const char method_name[] = "CZClient::ShutdownWork";
    TRACE_ENTRY;

    // Set flag that tells the commAcceptor thread to exit
    StateSet( ZC_SHUTDOWN );
    CLock::wakeOne();

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d waiting for ZClient thread %lx to exit.\n"
                    ,  method_name, __LINE__, threadId_);
    }

    // Wait for commAcceptor thread to exit
    int rc = pthread_join( threadId_, NULL );
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        int err = rc;
        sprintf(buf, "[%s], Error= Can't join thread! - errno=%d (%s)\n", method_name, err, strerror(err));
        mon_log_write(MON_ZCLIENT_SHUTDOWNWORK_1, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
    return(rc);
}

// Create the ZClientThread
int CZClient::StartWork( void )
{
    const char method_name[] = "CZClient::StartWork";
    TRACE_ENTRY;

    int rc = pthread_create(&threadId_, NULL, ZClientThread, this);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], ZClientThread create error=%d\n",
                 method_name, rc);
        mon_log_write(MON_ZCLIENT_STARTWORK_1, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
    return(rc);
}

void CZClient::StartMonitoring( void )
{
    const char method_name[] = "CZClient::StartMonitoring";
    TRACE_ENTRY;
    if (ZHandle)
    {
        ZClient->StateSet( CZClient::ZC_START );
        ZClient->CLock::wakeOne();
    }
    TRACE_EXIT;
}

void CZClient::StateSet( ZClientState_t state )
{ 
    CAutoLock lock(getLocker());

    if ( StateGet() != ZC_SHUTDOWN )
    {
        if (state == ZC_SHUTDOWN)
        {
            shutdown_ = true;
        }
        state_ = state; 
    }
}

void CZClient::StateSet( int type, ZClientState_t state, const char *znodePath ) 
{
    const char method_name[] = "CZClient::StateSet";

    CAutoLock lock(getLocker());
    if ( StateGet() != ZC_SHUTDOWN )
    {
        StateSet( state );
        if ( type == ZOO_CHANGED_EVENT )
        {
            znodeChangedQueue_.push_back( znodePath );
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - state_=%s, "
                              "znodeChangedQueue_=%ld\n"
                            , method_name, __LINE__
                            , ZClientStateStr(StateGet())
                            , znodeChangedQueue_.size() );
            }
        }
        else if ( type == ZOO_CHILD_EVENT )
        {
            znodeChildQueue_.push_back( znodePath );
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - state_=%s, "
                              "znodeChildQueue_=%ld\n"
                            , method_name, __LINE__
                            , ZClientStateStr(StateGet())
                            , znodeChildQueue_.size() );
            }
        }
        else if ( type == ZOO_CREATED_EVENT )
        {
            znodeCreatedQueue_.push_back( znodePath );
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - state_=%s, "
                              "znodeCreatedQueue_=%ld\n"
                            , method_name, __LINE__
                            , ZClientStateStr(StateGet())
                            , znodeCreatedQueue_.size() );
            }
        }
        else if ( type == ZOO_DELETED_EVENT )
        {
            znodeDeletedQueue_.push_back( znodePath );
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - state_=%s, "
                              "znodeDeletedQueue_=%ld\n"
                            , method_name, __LINE__
                            , ZClientStateStr(StateGet())
                            , znodeDeletedQueue_.size() );
            }
        }
        else
        {
            abort(); // Programmer bonehead!
        }
    }
}

void CZClient::StopMonitoring( void )
{
    const char method_name[] = "CZClient::StopMonitoring";
    TRACE_ENTRY;
    ZClient->StateSet( CZClient::ZC_STOP );
    ZClient->CLock::wakeOne();
    TRACE_EXIT;
}

char* CZClient::StrCpyLeafZNode( char* znode, const char* znodePath )
{
    char  pathStr[MAX_PROCESSOR_NAME] = { 0 };
    char *tkn = NULL;
    char *tknStart = pathStr;
    char *tknLast = NULL;

    strcpy( pathStr, znodePath );

    tknStart++; // skip the first '/'
    tkn = strtok( tknStart, "/" );
    do
    {
        tknLast = tkn;
        tkn = strtok( NULL, "/" );
    }
    while( tkn != NULL );

    strcpy( znode, tknLast );

    return( znode );
}

void CZClient::TimeToWakeUpSet( struct timespec &ts )
{
    const char method_name[] = "CZClient::TimeToWakeUpSet";
    TRACE_ENTRY;

    clock_gettime(CLOCK_REALTIME, &ts);
#if 0
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
            trace_printf("%s@%d" " - Clock   time %ld(secs):%ld(nsecs)(zcMonitoringRate_=%ld)\n"
                        , method_name, __LINE__
                        , ts.tv_sec, ts.tv_nsec, zcMonitoringRate_);
    }
#endif

    ts.tv_sec += zcMonitoringRate_;

#if 0
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
            trace_printf("%s@%d" " - Timeout time %ld(secs):%ld(nsecs)(zcMonitoringRate_=%ld)\n"
                        , method_name, __LINE__
                        , ts.tv_sec, ts.tv_nsec, zcMonitoringRate_);
    }
#endif
    TRACE_EXIT;
}

void CZClient::TriggerCheck( int type, const char *znodePath )
{
    const char method_name[] = "CZClient::TriggerCheck";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d" " - type=%s, path=%s\n"
                    , method_name, __LINE__
                    , ZooConnectionTypeStr( type )
                    , znodePath );
    }

    CAutoLock lock(getLocker());
    if ( StateGet() != ZC_SHUTDOWN )
    {
        if ( type == ZOO_CHANGED_EVENT )
        {
            StateSet( type, ZC_ZNODE_CHANGED, znodePath );
        }
        else if ( type == ZOO_CHILD_EVENT )
        {
            string znode;
            znode.assign( znodePath );
        
            if (configuredZNodePath_.compare( znode ) == 0)
            {
                // We are here due to a configured ZC_ZNODE_CHILD so reset the watch
                ConfiguredZNodesWatchSet();
            } 
            else if (errorZNodePath_.compare( znode ) == 0)
            {
                // We are here due to an error ZC_ZNODE_CHILD so reset the watch
                ErrorZNodesWatchSet();
            }
    
            StateSet( type, ZC_ZNODE_CHILD, znodePath );
        }
        else if ( type == ZOO_CREATED_EVENT )
        {
            StateSet( type, ZC_ZNODE_CREATED, znodePath );
        }
        else if ( type == ZOO_DELETED_EVENT )
        {
            StateSet( type, ZC_ZNODE_DELETED, znodePath );
        }
        else if ( type == ZOO_NOTWATCHING_EVENT )
        {
            StateSet( ZC_CLUSTER );
        }
    
        CLock::wakeOne();
    }
    TRACE_EXIT;
}

int CZClient::ZNodeCreate( const char *znodePath
                         , const char *znodeData
                         , int flags
                         , bool existOk )
{
    const char method_name[] = "CZClient::ZNodeCreate";
    TRACE_ENTRY;

    int rc = -1;
    char realpath[1024] = { 0 };

    stringstream ss;
    ss.str( "" );
    ss << znodePath;
    string zpath( ss.str( ) );
    
    ss.str( "" );
    ss << ((znodeData) ? znodeData : "");
    string zdata( ss.str( ) );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d zoo_create (%s : %s)\n"
                    , method_name, __LINE__
                    , zpath.c_str()
                    , zdata.c_str());
    }
    rc = zoo_create( ZHandle
                   , zpath.c_str( )
                   , zdata.length() ? zdata.c_str()  : NULL
                   , zdata.length() ? zdata.length() : -1
                   , &ZOO_OPEN_ACL_UNSAFE
                   , flags
                   , realpath
                   , sizeof(realpath)-1 );
    if ( rc != ZOK )
    {
        if ( rc != ZNODEEXISTS || 
            (rc == ZNODEEXISTS && !existOk) )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], zoo_create(%s) failed with error %s\n"
                    , method_name
                    , zpath.c_str()
                    , zerror(rc) );
            mon_log_write(MON_ZCLIENT_ZNODECREATE_1, SQ_LOG_ERR, buf);
        }
    }
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf("%s@%d realpath=%s\n", method_name, __LINE__, realpath);
    }

    TRACE_EXIT;
    return( rc );
}

int CZClient::ZNodeDataGet( string &monZnode, string &nodeName, int &pnid )
{
    const char method_name[] = "CZClient::ZNodeDataGet";
    TRACE_ENTRY;

    char  pnidStr[8] = { 0 };
    char *tkn = NULL;
    char  zkData[MAX_PROCESSOR_NAME];
    int   rc = -1;
    int   zkDataLen = sizeof(zkData);
    Stat  stat;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d monZnode=%s\n"
                    , method_name, __LINE__, monZnode.c_str() );
    }
    rc = ZooExistRetry( ZHandle, monZnode.c_str( ), 0, &stat );
    if ( rc == ZNONODE )
    {
        // return the error
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d monZnode=%s does not exist (ZNONODE)\n"
                        , method_name, __LINE__, monZnode.c_str() );
        }
    }
    else if ( rc == ZOK )
    {
        // Get the pnid from the data part of znode
        rc = zoo_get( ZHandle, monZnode.c_str( ), false, zkData, &zkDataLen, &stat );
        if ( rc == ZOK )
        {
            // The first token is the node name
            tkn = strtok( zkData, ":" );
            if ( tkn != NULL )
            {
                nodeName = tkn;
            }
            tkn = strtok( NULL, ":" );
            if ( tkn != NULL )
            {
                strcpy( pnidStr, tkn );
                pnid = atoi( pnidStr );
            }
            // TODO: Save monZnode path in corresponding physical node object
            //       to match with when ZC_NODE is triggered
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], zoo_get() for %s failed with error %s\n"
                    ,  method_name, monZnode.c_str( ), zerror(rc));
            mon_log_write(MON_ZCLIENT_ZNODEDATAGET_1, SQ_LOG_ERR, buf);
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists() for %s failed with error %s\n"
                ,  method_name, monZnode.c_str( ), zerror(rc));
        mon_log_write(MON_ZCLIENT_ZNODEDATAGET_2, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
    return( rc );
}

int CZClient::ZNodeDelete( string &znode )
{
    const char method_name[] = "CZClient::ZNodeDelete";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d Deleting znode=%s\n"
                    , method_name, __LINE__
                    , znode.c_str() );
    }

    int rc = -1;
    rc = zoo_delete( ZHandle
                   , znode.c_str()
                   , -1 );
    if ( rc == ZOK || rc == ZNONODE)
    {
        if ( rc == ZNONODE)
        {
            // This is ok since we call it indiscriminately
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d znode=%s already deleted!\n"
                            , method_name, __LINE__
                            , znode.c_str() );
            }
        }
        else
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d znode=%s deleted!\n"
                            , method_name, __LINE__
                            , znode.c_str() );
            }
        }
    }
    else if ( rc == ZCONNECTIONLOSS || 
              rc == ZOPERATIONTIMEOUT )
    {
        rc = ZOK;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], znode (%s) cannot be accessed!\n"
                , method_name, znode.c_str() );
        mon_log_write(MON_ZCLIENT_ZNODEDELETE_1, SQ_LOG_INFO, buf);
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_delete(%s) failed with error %s\n"
                , method_name, znode.c_str(), zerror(rc) );
        mon_log_write(MON_ZCLIENT_ZNODEDELETE_1, SQ_LOG_CRIT, buf);
        switch ( rc )
        {
        case ZSYSTEMERROR:
        case ZRUNTIMEINCONSISTENCY:
        case ZDATAINCONSISTENCY:
        case ZMARSHALLINGERROR:
        case ZUNIMPLEMENTED:
        case ZBADARGUMENTS:
        case ZINVALIDSTATE:
        case ZSESSIONEXPIRED:
        case ZCLOSING:
            // Treat these error like a session expiration, since
            // we can't communicate with quorum servers
            HandleMyNodeExpiration();
            break;
        default:
            break;
        }
    }

    TRACE_EXIT;
    return( rc );
}

int CZClient::ZNodeWatchReset( string &znode )
{
    const char method_name[] = "CZClient::ZNodeWatchReset";
    TRACE_ENTRY;

    char  zkData[MAX_PROCESSOR_NAME];
    int   rc = -1;
    int   zkDataLen = sizeof(zkData);
    Stat  stat;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d znode=%s\n"
                    , method_name, __LINE__, znode.c_str() );
    }
    rc = ZooExistRetry( ZHandle, znode.c_str( ), 0, &stat );
    if ( rc == ZNONODE ||
         rc == ZCONNECTIONLOSS || 
         rc == ZOPERATIONTIMEOUT )
    {
        // return the error
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d znode=%s does not exist or "
                          "cannot be accessed!\n"
                        , method_name, __LINE__, znode.c_str() );
        }
    }
    else if ( rc == ZOK )
    {
        // Reset a watch on monZode
        int watch = 0;
        rc = zoo_get( ZHandle, znode.c_str( ), watch, zkData, &zkDataLen, &stat );
        if ( rc != ZOK )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], zoo_get() for %s failed with error %s\n"
                    ,  method_name, znode.c_str( ), zerror(rc));
            mon_log_write(MON_ZCLIENT_ZNODEWATCHRESET_1, SQ_LOG_ERR, buf);
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists() for %s failed with error %s\n"
                ,  method_name, znode.c_str( ), zerror(rc));
        mon_log_write(MON_ZCLIENT_ZNODEWATCHRESET_2, SQ_LOG_CRIT, buf);
        switch ( rc )
        {
        case ZSYSTEMERROR:
        case ZRUNTIMEINCONSISTENCY:
        case ZDATAINCONSISTENCY:
        case ZMARSHALLINGERROR:
        case ZUNIMPLEMENTED:
        case ZBADARGUMENTS:
        case ZINVALIDSTATE:
        case ZSESSIONEXPIRED:
        case ZCLOSING:
            // Treat these error like a session expiration, since
            // we can't communicate with quorum servers
            HandleMyNodeExpiration();
            break;
        default:
            break;
        }
    }

    TRACE_EXIT;
    return( rc );
}

int CZClient::ZNodeWatchSet( string &znode )
{
    const char method_name[] = "CZClient::ZNodeWatchSet";
    TRACE_ENTRY;

    char  zkData[MAX_PROCESSOR_NAME];
    int   rc = -1;
    int   zkDataLen = sizeof(zkData);
    Stat  stat;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d znode=%s\n"
                    , method_name, __LINE__, znode.c_str() );
    }
    rc = ZooExistRetry( ZHandle, znode.c_str( ), 0, &stat );
    if ( rc == ZNONODE ||
         rc == ZCONNECTIONLOSS || 
         rc == ZOPERATIONTIMEOUT )
    {
        // return the error
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d znode=%s does not exist or "
                          "cannot be accessed!\n"
                        , method_name, __LINE__, znode.c_str() );
        }
    }
    else if ( rc == ZOK )
    {
        // Set a watch on monZode
        int watch = 1;
        rc = zoo_get( ZHandle, znode.c_str( ), watch, zkData, &zkDataLen, &stat );
        if ( rc != ZOK )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], zoo_get() for %s failed with error %s\n"
                    ,  method_name, znode.c_str( ), zerror(rc));
            mon_log_write(MON_ZCLIENT_ZNODEWATCHSET_1, SQ_LOG_ERR, buf);
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists() for %s failed with error %s\n"
                ,  method_name, znode.c_str( ), zerror(rc));
        mon_log_write(MON_ZCLIENT_ZNODEWATCHSET_2, SQ_LOG_CRIT, buf);
        switch ( rc )
        {
        case ZSYSTEMERROR:
        case ZRUNTIMEINCONSISTENCY:
        case ZDATAINCONSISTENCY:
        case ZMARSHALLINGERROR:
        case ZUNIMPLEMENTED:
        case ZBADARGUMENTS:
        case ZINVALIDSTATE:
        case ZSESSIONEXPIRED:
        case ZCLOSING:
            // Treat these error like a session expiration, since
            // we can't communicate with quorum servers
            HandleMyNodeExpiration();
            break;
        default:
            break;
        }
    }

    TRACE_EXIT;
    return( rc );
}

int CZClient::ZNodeWatchChildSet( string &parentznode )
{
    const char method_name[] = "CZClient::ZNodeWatchChildSet";
    TRACE_ENTRY;

    bool found = false;
    int rc = -1;
    int retries = 0;
    int watch = 1;
    Stat stat;
    struct String_vector nodes;

    nodes.count = 0;
    nodes.data = NULL;

    while ( !found )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d parentznode=%s\n"
                        , method_name, __LINE__, parentznode.c_str() );
        }
        // Verify the existence of the parent
        rc = ZooExistRetry( ZHandle, parentznode.c_str( ), 0, &stat );
        if ( rc == ZNONODE )
        {
            if (retries > 10)
                break;
            retries++;    
            continue;
        }
        else if ( rc == ZOK )
        {
            // Now get the list of available znodes in the cluster.
            //
            // This will return child znodes for each monitor process that has
            // registered, including this process.
            rc = zoo_get_children( ZHandle, parentznode.c_str( ), watch, &nodes );
            if ( rc == ZOK )
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d nodes.count=%d\n"
                                , method_name, __LINE__
                                , nodes.count );
                }
                FreeStringVector( &nodes );
                found = true;
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], zoo_get_children(%s) failed with error %s\n"
                        ,  method_name, parentznode.c_str( ), zerror(rc));
                mon_log_write(MON_ZCLIENT_ZNODEWATCHCHILDSET_1, SQ_LOG_ERR, buf);
                break;
            }
        }
        else  // error
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], zoo_exists() for %s failed with error %s\n"
                    ,  method_name, parentznode.c_str( ), zerror(rc));
            mon_log_write(MON_ZCLIENT_ZNODEWATCHCHILDSET_2, SQ_LOG_ERR, buf);
            break;
        }
    }

    TRACE_EXIT;
    return( rc );
}

int CZClient::ZNodesTreeCreate( void )
{
    const char method_name[] = "CZClient::ZNodesTreeCreate";
    TRACE_ENTRY;

    int rc;
    Stat stat;

    stringstream ss;
    ss.str( "" );
    ss << zkRootNode_.c_str();
    string rootDir( ss.str( ) );

    rc = ZooExistRetry( ZHandle, rootDir.c_str(), 0, &stat );
    switch (rc)
    {
    case ZOK:
        break;
    case ZNONODE:
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d ZNodeCreate(%s)\n"
                        , method_name, __LINE__ 
                        , rootDir.c_str() );
        }
        rc = ZNodeCreate( rootDir.c_str(), NULL, 0 );
        if ( rc && rc != ZNODEEXISTS )
        {
            return(rc); // Return the error
        }
        rc = ZOK;
        break;
    default:
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists(%s) failed with error %s\n"
                , method_name, rootDir.c_str(), zerror(rc) );
        mon_log_write(MON_ZCLIENT_ZNODESTREECREATE_1, SQ_LOG_ERR, buf);
        if (rc) return(rc); // Return the error
        break;
    }

    ss.str( "" );
    ss << zkRootNode_.c_str() 
       << zkRootNodeInstance_.c_str();
    string instanceDir( ss.str( ) );

    rc = ZooExistRetry( ZHandle, instanceDir.c_str( ), 0, &stat );
    switch (rc)
    {
    case ZOK:
        break;
    case ZNONODE:
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d ZNodeCreate(%s)\n"
                        , method_name, __LINE__
                        , instanceDir.c_str() );
        }
        rc = ZNodeCreate( instanceDir.c_str(), NULL, 0 );
        if ( rc && rc != ZNODEEXISTS )
        {
            return(rc); // Return the error
        }
        rc = ZOK;
        break;
    default:
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists(%s) failed with error %s\n"
                , method_name, instanceDir.c_str( ), zerror(rc) );
        mon_log_write(MON_ZCLIENT_ZNODESTREECREATE_2, SQ_LOG_ERR, buf);
        break;
    }

    ss.str( "" );
    ss << zkRootNode_.c_str() 
       << zkRootNodeInstance_.c_str() 
       << ZCLIENT_CLUSTER_ZNODE;
    clusterZNodePath_ = ss.str();

    rc = ZooExistRetry( ZHandle, clusterZNodePath_.c_str( ), 0, &stat );
    switch (rc)
    {
    case ZOK:
        break;
    case ZNONODE:
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d ZNodeCreate(%s)\n"
                        , method_name, __LINE__
                        , clusterZNodePath_.c_str() );
        }
        rc = ZNodeCreate( clusterZNodePath_.c_str(), NULL, 0 );
        if ( rc && rc != ZNODEEXISTS )
        {
            return(rc); // Return the error
        }
        rc = ZOK;
        break;
    default:
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists(%s) failed with error %s\n"
                , method_name, clusterZNodePath_.c_str(), zerror(rc) );
        mon_log_write(MON_ZCLIENT_ZNODESTREECREATE_3, SQ_LOG_ERR, buf);
        break;
    }

    ss.str( "" );
    ss << zkRootNode_.c_str() 
       << zkRootNodeInstance_.c_str() 
       << ZCLIENT_CLUSTER_ZNODE
       << ZCLIENT_CONFIGURED_ZNODE;
    configuredZNodePath_ = ss.str();

    rc = ZooExistRetry( ZHandle, configuredZNodePath_.c_str( ), 0, &stat );
    switch (rc)
    {
    case ZOK:
        break;
    case ZNONODE:
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d ZNodeCreate(%s)\n"
                        , method_name, __LINE__
                        , configuredZNodePath_.c_str() );
        }
        rc = ZNodeCreate( configuredZNodePath_.c_str(), NULL, 0 );
        if ( rc && rc != ZNODEEXISTS )
        {
            return(rc); // Return the error
        }
        rc = ZOK;
        break;
    default:
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists(%s) failed with error %s\n"
                , method_name, configuredZNodePath_.c_str(), zerror(rc) );
        mon_log_write(MON_ZCLIENT_ZNODESTREECREATE_4, SQ_LOG_ERR, buf);
        break;
    }

    ss.str( "" );
    ss << zkRootNode_.c_str() 
       << zkRootNodeInstance_.c_str() 
       << ZCLIENT_CLUSTER_ZNODE
       << ZCLIENT_ERROR_ZNODE;
    errorZNodePath_ = ss.str();

    rc = ZooExistRetry( ZHandle, errorZNodePath_.c_str( ), 0, &stat );
    switch (rc)
    {
    case ZOK:
        break;
    case ZNONODE:
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d ZNodeCreate(%s)\n"
                        , method_name, __LINE__
                        , errorZNodePath_.c_str() );
        }
        rc = ZNodeCreate( errorZNodePath_.c_str(), NULL, 0 );
        if ( rc && rc != ZNODEEXISTS )
        {
            return(rc); // Return the error
        }
        rc = ZOK;
        break;
    default:
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists(%s) failed with error %s\n"
                , method_name, errorZNodePath_.c_str(), zerror(rc) );
        mon_log_write(MON_ZCLIENT_ZNODESTREECREATE_6, SQ_LOG_ERR, buf);
        break;
    }

    ss.str( "" );
    ss << zkRootNode_.c_str() 
       << zkRootNodeInstance_.c_str() 
       << ZCLIENT_CLUSTER_ZNODE
       << ZCLIENT_RUNNING_ZNODE;
    runningZNodePath_ = ss.str();

    rc = ZooExistRetry( ZHandle, runningZNodePath_.c_str( ), 0, &stat );
    switch (rc)
    {
    case ZOK:
        break;
    case ZNONODE:
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d ZNodeCreate(%s)\n"
                        , method_name, __LINE__
                        , runningZNodePath_.c_str() );
        }
        rc = ZNodeCreate( runningZNodePath_.c_str(), NULL, 0 );
        if ( rc && rc != ZNODEEXISTS )
        {
            return(rc); // Return the error
        }
        rc = ZOK;
        break;
    default:
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists(%s) failed with error %s\n"
                , method_name, runningZNodePath_.c_str(), zerror(rc) );
        mon_log_write(MON_ZCLIENT_ZNODESTREECREATE_5, SQ_LOG_ERR, buf);
        break;
    }

    ss.str( "" );
    ss << zkRootNode_.c_str() 
       << zkRootNodeInstance_.c_str() 
       << ZCLIENT_MONITOR_ZNODE;
    monitorZNodePath_ = ss.str();

    rc = ZooExistRetry( ZHandle, monitorZNodePath_.c_str( ), 0, &stat );
    switch (rc)
    {
    case ZOK:
        break;
    case ZNONODE:
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d ZNodeCreate(%s)\n"
                        , method_name, __LINE__
                        , monitorZNodePath_.c_str() );
        }
        rc = ZNodeCreate( monitorZNodePath_.c_str(), NULL, 0 );
        if ( rc && rc != ZNODEEXISTS )
        {
            return(rc); // Return the error
        }
        rc = ZOK;
        break;
    default:
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists(%s) failed with error %s\n"
                , method_name, monitorZNodePath_.c_str(), zerror(rc) );
        mon_log_write(MON_ZCLIENT_ZNODESTREECREATE_7, SQ_LOG_ERR, buf);
        break;
    }

    ss.str( "" );
    ss << zkRootNode_.c_str() 
       << zkRootNodeInstance_.c_str() 
       << ZCLIENT_MONITOR_ZNODE
       << ZCLIENT_MASTER_ZNODE;
    string masterDir( ss.str( ) );
    masterZNodePath_ = ss.str();

    rc = ZooExistRetry( ZHandle, masterZNodePath_.c_str( ), 0, &stat );
    switch (rc)
    {
    case ZOK:
        break;
    case ZNONODE:
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Invoking ZNodeCreate(%s)\n"
                        , method_name, __LINE__
                        , masterZNodePath_.c_str() );
        }
        rc = ZNodeCreate( masterZNodePath_.c_str(), NULL, 0 );
        if ( rc && rc != ZNODEEXISTS )
        {
            return(rc); // Return the error
        }
        rc = ZOK;
        break;
    default:
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists(%s) failed with error %s\n"
                , method_name, masterZNodePath_.c_str(), zerror(rc) );
        mon_log_write(MON_ZCLIENT_ZNODESTREECREATE_8, SQ_LOG_ERR, buf);
        break;
    }
    
    TRACE_EXIT;
    return(rc);
}

int CZClient::ZooExistRetry(zhandle_t *zh, const char *path, int watch, struct Stat *stat)
{
    int retries = 0;
    int rc;
    rc = zoo_exists(zh, path, watch, stat);

    // retry when loss zconnection or timeout, this may be caused by one zookeeper server down
    while ( (rc == ZCONNECTIONLOSS
          || rc == ZOPERATIONTIMEOUT
          || rc == ZSESSIONMOVED)
         && retries < ZOOKEEPER_RETRY_COUNT)
    {
        sleep(ZOOKEEPER_RETRY_WAIT);
        retries++;
        rc = zoo_exists(zh, path, watch, stat);
    }
    return rc;
}

