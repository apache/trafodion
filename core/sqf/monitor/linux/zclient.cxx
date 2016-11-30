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
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
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
#include "pnode.h"
#include "zclient.h"

// The following specify the default values for the timers if the
// zclient cluster monitoring timer related variables are not defined.
//
// NOTE: It is recommended to set the checkrate to -1 which essentially
//       make the zclient event driven. Meaning the watcher is called
//       only when watched a znode is changed or is deleted (expires)
//       Also, the session timeout must be kept at or below 60 seconds
//       as this is enforced by Zookeeper. Any, value above 60 seconds
//       is renegotiated by Zookeeper to 60 seconds.
#define ZCLIENT_MONITORING_CHECKRATE         -1 // seconds (disabled)
#define ZCLIENT_SESSION_TIMEOUT              60 // seconds (1 minute)

// The monitors register their znodes under the cluster znode
#define ZCLIENT_CLUSTER_ZNODE               "/cluster"

using namespace std;

extern char Node_name[MPI_MAX_PROCESSOR_NAME];
extern int MyPNID;
extern int MyNid;
extern int MyPid;

extern CNodeContainer *Nodes;
extern CReqQueue ReqQueue;
extern CZClient    *ZClient;
extern CMonLog     *MonLog;
extern CMonLog     *SnmpLog;

extern bool debugFlag;

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
        case CZClient::ZC_CLUSTER:
            return "ZC_CLUSTER";
        case CZClient::ZC_ZNODE:
            return "ZC_ZNODE";
        case CZClient::ZC_WATCH:
            return "ZC_WATCH";
        case CZClient::ZC_STOP:
            return "ZC_STOP";
        case CZClient::ZC_SHUTDOWN:
            return "ZC_SHUTDOWN";
        default:
            break;
    }
    return "ZClient State Invalid";
}

static const char *ZooConnectionTypeStr( int type )
{
    if ( type == ZOO_CREATED_EVENT )
        return "ZOO_CREATED_EVENT";
    if ( type == ZOO_DELETED_EVENT )
        return "ZOO_DELETED_EVENT";
    if ( type == ZOO_CHANGED_EVENT )
        return "ZOO_CHANGED_EVENT";
    if ( type == ZOO_CHILD_EVENT )
        return "ZOO_CHILD_EVENT";
    if ( type == ZOO_SESSION_EVENT )
        return "ZOO_SESSION_EVENT";
    if ( type == ZOO_NOTWATCHING_EVENT )
        return "ZOO_NOTWATCHING_EVENT";

    return "INVALID_TYPE";
}

static const char *ZooConnectionStateStr( int state )
{
    if ( state == 0 )
        return "CLOSED_STATE";
    if ( state == ZOO_EXPIRED_SESSION_STATE )
        return "EXPIRED_SESSION_STATE";
    if ( state == ZOO_AUTH_FAILED_STATE )
        return "AUTH_FAILED_STATE";
    if ( state == ZOO_CONNECTING_STATE )
        return "CONNECTING_STATE";
    if ( state == ZOO_ASSOCIATING_STATE )
        return "ASSOCIATING_STATE";
    if ( state == ZOO_CONNECTED_STATE )
        return "CONNECTED_STATE";

    return "INVALID_STATE";
}

const char *ZooErrorStr( int error )
{
    if ( error == 0 )
        return "ZOK";
    if ( error == ZNONODE )
        return "ZNONODE";
    if ( error == ZNODEEXISTS )
        return "ZNODEEXISTS";
    if ( error == ZNOAUTH )
        return "ZNOAUTH";
    if ( error == ZNOCHILDRENFOREPHEMERALS )
        return "ZNOCHILDRENFOREPHEMERALS";
    if ( error == ZBADARGUMENTS )
        return "ZBADARGUMENTS";
    if ( error == ZINVALIDSTATE )
        return "ZINVALIDSTATE";
    if ( error == ZMARSHALLINGERROR )
        return "ZMARSHALLINGERROR";
    if ( error == ZCONNECTIONLOSS )
        return "ZCONNECTIONLOSS";
    if ( error == ZOPERATIONTIMEOUT )
        return "ZOPERATIONTIMEOUT";

    static char errorStr[20];
    sprintf( errorStr, "%d", error );
    return errorStr;
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
                    , "[%s], Error Zookeeper authentication failure. Node going down...\n"
                    ,  method_name );
            mon_log_write(MON_ZCLIENT_ZSESSIONWATCHER_1, SQ_LOG_CRIT, buf);

            HandleZSessionExpiration();

            zookeeper_close( zzh );
            ZHandle=0;
        }
        else if ( state == ZOO_EXPIRED_SESSION_STATE )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], Error Zookeeper session expired. Node going down...\n"
                    ,  method_name );
            mon_log_write(MON_ZCLIENT_ZSESSIONWATCHER_2, SQ_LOG_CRIT, buf);

            HandleZSessionExpiration();

            zookeeper_close( zzh );
            ZHandle=0;
        }
    }
    else if ( type == ZOO_CREATED_EVENT )
    {
        ZClient->TriggerCheck( type, path );
    }
    else if ( type == ZOO_DELETED_EVENT )
    {
        ZClient->TriggerCheck( type, path );
    }
    else if ( type == ZOO_CHANGED_EVENT )
    {
        ZClient->TriggerCheck( type, path );
    }
    else if ( type == ZOO_CHILD_EVENT )
    {
        ZClient->TriggerCheck( type, path );
    }
    else if ( type == ZOO_NOTWATCHING_EVENT )
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
         ,checkCluster_(false)
         ,zcMonitoringRate_(ZCLIENT_MONITORING_CHECKRATE) // seconds
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
    if ( (zcMonitoringRateValueC = getenv( "SQ_MON_ZCLIENT_MONITORING_CHECKRATE" )) )
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
        abort();
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
        abort();
    }
    
    int rc = InitializeZClient();
    if ( rc )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Failed ZClient initialization (%s)\n"
                , method_name, ZooErrorStr(rc) );
        mon_log_write(MON_ZCLIENT_ZCLIENT_3, SQ_LOG_ERR, buf);
        abort();
    }

    TRACE_EXIT;
}

CZClient::~CZClient( void )
{
    const char method_name[] = "CZClient::~CZClient";
    TRACE_ENTRY;

    memcpy(&eyecatcher_, "zclt", 4);

    if (ZHandle)
    {
        WatchNodeDelete( Node_name );
        zookeeper_close(ZHandle);
        ZHandle = 0;
    }

    TRACE_EXIT;
}

void CZClient::CheckCluster( void )
{
    const char method_name[] = "CZClient::CheckCluster";
    TRACE_ENTRY;

    int rc;
    struct String_vector nodes;

    if ( IsCheckCluster() )
    {
        rc = GetClusterZNodes( &nodes );
        if ( rc != ZOK )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], GetClusterZNodes() failed!\n"
                    , method_name );
            mon_log_write(MON_ZCLIENT_CHECKCLUSTER_1, SQ_LOG_ERR, buf);
            SetState( CZClient::ZC_STOP );
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
                newpath << zkRootNode_.c_str() 
                        << zkRootNodeInstance_.c_str()
                        << ZCLIENT_CLUSTER_ZNODE << "/"
                        << nodes.data[i];
                string monZnode = newpath.str( );
            
                rc = GetZNodeData( monZnode, nodeName, pnid );
                if ( rc != ZOK )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s], GetZNodeData() failed!\n"
                            , method_name );
                    mon_log_write(MON_ZCLIENT_CHECKCLUSTER_2, SQ_LOG_ERR, buf);
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
            trace_printf( "%s@%d CheckCluster is NOT set!\n"
                        , method_name, __LINE__ );
        }
    }
    
    TRACE_EXIT;
}

int CZClient::GetClusterZNodes( String_vector *nodes )
{
    const char method_name[] = "CZClient::GetClusterZNodes";
    TRACE_ENTRY;

    bool found = false;
    int rc = -1;
    int retries = 0;
    Stat stat;

    stringstream ss;
    ss.str( "" );
    ss << zkRootNode_.c_str() 
       << zkRootNodeInstance_.c_str() 
       << ZCLIENT_CLUSTER_ZNODE;
    string trafCluster( ss.str( ) );

    nodes->count = 0;
    nodes->data = NULL;

    while ( !found )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d trafCluster=%s\n"
                        , method_name, __LINE__, trafCluster.c_str() );
        }
        // Verify the existence of the parent ZCLIENT_CLUSTER_ZNODE
        rc = zoo_exists( ZHandle, trafCluster.c_str( ), 0, &stat );
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
                if (retries > 10)
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
                    ,  method_name, trafCluster.c_str( ), ZooErrorStr(rc));
            mon_log_write(MON_ZCLIENT_GETCLUSTERZNODES_2, SQ_LOG_ERR, buf);
            break;
        }
    }

    TRACE_EXIT;
    return( rc );
}

int CZClient::GetZNodeData( string &monZnode, string &nodeName, int &pnid )
{
    const char method_name[] = "CZClient::GetZNodeData";
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
    rc = zoo_exists( ZHandle, monZnode.c_str( ), 0, &stat );
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
                    ,  method_name, monZnode.c_str( ), ZooErrorStr(rc));
            mon_log_write(MON_ZCLIENT_GETZNODEDATA_2, SQ_LOG_ERR, buf);
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists() for %s failed with error %s\n"
                ,  method_name, monZnode.c_str( ), ZooErrorStr(rc));
        mon_log_write(MON_ZCLIENT_GETZNODEDATA_3, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
    return( rc );
}

void CZClient::HandleExpiredZNode( void )
{
    const char method_name[] = "CZClient::HandleExpiredZNode";
    TRACE_ENTRY;

    if ( IsCheckCluster() )
    {
        char  pathStr[MAX_PROCESSOR_NAME] = { 0 };
        char  nodeName[MAX_PROCESSOR_NAME] = { 0 };
        char *tkn = NULL;
        char *tknStart = pathStr;
        char *tknLast = NULL;
        string monZnode;
    
        monZnode.assign( znodeQueue_.front() );

        if (trace_settings)
        {
            trace_printf("%s@%d" " - znodePath=%s, znodeQueue_.size=%ld\n"
                        , method_name, __LINE__
                        , monZnode.c_str(), znodeQueue_.size() );
        }

        znodeQueue_.pop_front();
        
        trace_printf( "%s@%d" " - Checking znode=%s\n"
                    , method_name, __LINE__
                    , monZnode.c_str() );

        strcpy( pathStr, monZnode.c_str() );

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

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], %s was deleted, handling node (%s) as a down node!\n"
                ,  method_name, monZnode.c_str(), nodeName );
        mon_log_write(MON_ZCLIENT_CHECKZNODE_1, SQ_LOG_ERR, buf);

        HandleNodeExpiration( nodeName );
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d CheckCluster is NOT set!\n"
                        , method_name, __LINE__ );
        }
    }
    
    TRACE_EXIT;
}

int CZClient::InitializeZClient( void )
{
    const char method_name[] = "CZClient::MakeClusterZNodes";
    TRACE_ENTRY;

    int rc;

    rc = MakeClusterZNodes();
    if ( rc == ZOK )
    {
        rc = RegisterMyNodeZNode();
    }

    TRACE_EXIT;
    return( rc );
}

bool CZClient::IsZNodeExpired( const char *nodeName, int &zerr )
{
    const char method_name[] = "CZClient::IsZNodeExpired";
    TRACE_ENTRY;

    bool  expired = false;
    int   rc = -1;
    Stat  stat;
    stringstream newpath;
    newpath.str( "" );
    newpath << zkRootNode_.c_str() 
            << zkRootNodeInstance_.c_str() 
            << ZCLIENT_CLUSTER_ZNODE << "/"
            << nodeName;
    string monZnode = newpath.str( );

    zerr = ZOK;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d monZnode=%s\n"
                    , method_name, __LINE__, monZnode.c_str() );
    }
    rc = zoo_exists( ZHandle, monZnode.c_str( ), 0, &stat );
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
                ,  method_name, monZnode.c_str( ), ZooErrorStr(rc));
        mon_log_write(MON_ZCLIENT_ISZNODEEXPIRED_1, SQ_LOG_ERR, buf);
    }
    else if ( rc == ZOK )
    {
        expired = false;
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d monZnode=%s exist\n"
                        , method_name, __LINE__, monZnode.c_str() );
        }
    }
    else
    {
        expired = true;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists() for %s failed with error %s\n"
                ,  method_name, monZnode.c_str( ), ZooErrorStr(rc));
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
            HandleZSessionExpiration();
            break;
        default:
            break;
        }
    }

    TRACE_EXIT;
    return( expired );
}

int CZClient::MakeClusterZNodes( void )
{
    const char method_name[] = "CZClient::MakeClusterZNodes";
    TRACE_ENTRY;

    int rc;
    Stat stat;

    stringstream ss;
    ss.str( "" );
    ss << zkRootNode_.c_str();
    string rootDir( ss.str( ) );

    rc = zoo_exists( ZHandle, rootDir.c_str(), 0, &stat );
    switch (rc)
    {
    case ZOK:
        break;
    case ZNONODE:
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d RegisterZNode(%s)\n"
                        , method_name, __LINE__ 
                        , rootDir.c_str() );
        }
        rc = RegisterZNode( rootDir.c_str(), NULL, 0 );
        if ( rc && rc != ZNODEEXISTS )
        {
            return(rc); // Return the error
        }
        rc = ZOK;
        break;
    default:
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists() failed with error %s\n"
                , method_name, ZooErrorStr(rc) );
        mon_log_write(MON_ZCLIENT_CHECKCLUSTERZNODES_1, SQ_LOG_ERR, buf);
        if (rc) return(rc); // Return the error
        break;
    }

    ss.str( "" );
    ss << zkRootNode_.c_str() 
       << zkRootNodeInstance_.c_str();
    string instanceDir( ss.str( ) );

    rc = zoo_exists( ZHandle, instanceDir.c_str( ), 0, &stat );
    switch (rc)
    {
    case ZOK:
        break;
    case ZNONODE:
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d RegisterZNode(%s)\n"
                        , method_name, __LINE__
                        , instanceDir.c_str() );
        }
        rc = RegisterZNode( instanceDir.c_str(), NULL, 0 );
        if ( rc && rc != ZNODEEXISTS )
        {
            return(rc); // Return the error
        }
        rc = ZOK;
        break;
    default:
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists() failed with error %s\n"
                , method_name, ZooErrorStr(rc) );
        mon_log_write(MON_ZCLIENT_CHECKCLUSTERZNODES_2, SQ_LOG_ERR, buf);
        break;
    }

    ss.str( "" );
    ss << zkRootNode_.c_str() 
       << zkRootNodeInstance_.c_str() 
       << ZCLIENT_CLUSTER_ZNODE;
    string clusterDir( ss.str( ) );

    rc = zoo_exists( ZHandle, clusterDir.c_str( ), 0, &stat );
    switch (rc)
    {
    case ZOK:
        break;
    case ZNONODE:
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d RegisterZNode(%s)\n"
                        , method_name, __LINE__
                        , clusterDir.c_str() );
        }
        rc = RegisterZNode( clusterDir.c_str(), NULL, 0 );
        if ( rc && rc != ZNODEEXISTS )
        {
            return(rc); // Return the error
        }
        rc = ZOK;
        break;
    default:
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists() failed with error %s\n"
                , method_name, ZooErrorStr(rc) );
        mon_log_write(MON_ZCLIENT_CHECKCLUSTERZNODES_3, SQ_LOG_ERR, buf);
        break;
    }

    TRACE_EXIT;
    return(rc);
}

// ZClient main processing loop
void CZClient::MonitorZCluster()
{
    const char method_name[] = "CZClient::MonitorZCluster";
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
        SetTimeToWakeUp( timeout );
    }

    while ( GetState() != ZC_SHUTDOWN )
    {
        lock();
        if ( !IsEnabled() )
        {
            // Wait until timer started
            CLock::wait();
        }
        else
        {
            if (zcMonitoringRate_ < 0)
            {
                // Wait until signaled
                CLock::wait();
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d" " - ZCluster signaled, state_=%s\n"
                                , method_name, __LINE__
                                , ZClientStateStr(GetState()) );
                }
            }
            else
            {
                // Wait until signaled or timer expires
                rc = CLock::timedWait( &timeout );
                if ( rc != ETIMEDOUT  )
                {
                    if ( rc != 0 )
                    {
                        StopClusterMonitoring();
                    }
                    else
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d" " - ZCluster signaled, state_=%s\n"
                                        , method_name, __LINE__
                                        , ZClientStateStr(GetState()) );
                        }
                    }
                }
            }
        }

        switch ( GetState() )
        {
            case ZC_START:
                StartClusterMonitoring();
                break;
            case ZC_CLUSTER:
                if ( IsCheckCluster() )
                {
                    CheckCluster();
                }
                break;
            case ZC_WATCH:
                if ( !IsCheckCluster() )
                {
                    WatchCluster();
                }
                break;
            case ZC_ZNODE:
                if ( IsCheckCluster() )
                {
                    HandleExpiredZNode();
                }
                break;
            case ZC_STOP:
                StopClusterMonitoring();
                break;
            default:
                break;
        }
        if (zcMonitoringRate_ >= 0 )
        {
            SetTimeToWakeUp( timeout );
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

int CZClient::RegisterMyNodeZNode( void )
{
    const char method_name[] = "CZClient::RegisterMyNodeZNode";
    TRACE_ENTRY;

    int rc;
    char pnidStr[10];

    sprintf( pnidStr, "%d", MyPNID);

    stringstream newpath;
    newpath.str( "" );
    newpath << zkRootNode_.c_str() 
            << zkRootNodeInstance_.c_str() 
            << ZCLIENT_CLUSTER_ZNODE << "/"
            << Node_name;
    string monZnode = newpath.str( );

    stringstream ss;
    ss.str( "" );
    ss << Node_name << ":" << pnidStr;
    string monData = ss.str( );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d RegisterZNode(%s:%s)\n"
                    , method_name, __LINE__
                    , monZnode.c_str()
                    , monData.c_str() );
    }

    rc = RegisterZNode( monZnode.c_str(), monData.c_str(), ZOO_EPHEMERAL );

    TRACE_EXIT;

    return(rc);
}

int CZClient::RegisterZNode( const char *znodePath
                           , const char *znodeData
                           , int flags )
{
    const char method_name[] = "CZClient::RegisterZNode";
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
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_create(%s) failed with error %s\n"
                , method_name
                , zpath.c_str()
                , ZooErrorStr(rc) );
        mon_log_write(MON_ZCLIENT_REGISTERZNODE_1, SQ_LOG_ERR, buf);
    }
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf("%s@%d realpath=%s\n", method_name, __LINE__, realpath);
    }

    TRACE_EXIT;
    return( rc );
}

void CZClient::SetState( ZClientState_t state, const char *znodePath ) 
{
    CAutoLock lock(getLocker());
    state_ = state; 
    znodeQueue_.push_back( znodePath );
}

void CZClient::SetTimeToWakeUp( struct timespec &ts )
{
    const char method_name[] = "CZClient::SetTimeToWakeUp";
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

int CZClient::SetZNodeWatch( string &monZnode )
{
    const char method_name[] = "CZClient::SetZNodeWatch";
    TRACE_ENTRY;

    char  zkData[MAX_PROCESSOR_NAME];
    int   rc = -1;
    int   zkDataLen = sizeof(zkData);
    Stat  stat;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d monZnode=%s\n"
                    , method_name, __LINE__, monZnode.c_str() );
    }
    rc = zoo_exists( ZHandle, monZnode.c_str( ), 0, &stat );
    if ( rc == ZNONODE ||
         rc == ZCONNECTIONLOSS || 
         rc == ZOPERATIONTIMEOUT )
    {
        // return the error
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d monZnode=%s does not exist or "
                          "cannot be accessed!\n"
                        , method_name, __LINE__, monZnode.c_str() );
        }
    }
    else if ( rc == ZOK )
    {
        // Get the pnid from the data part of znode
        rc = zoo_get( ZHandle, monZnode.c_str( ), true, zkData, &zkDataLen, &stat );
        if ( rc != ZOK )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], zoo_get() for %s failed with error %s\n"
                    ,  method_name, monZnode.c_str( ), ZooErrorStr(rc));
            mon_log_write(MON_ZCLIENT_SETZNODEWATCH_1, SQ_LOG_ERR, buf);
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_exists() for %s failed with error %s\n"
                ,  method_name, monZnode.c_str( ), ZooErrorStr(rc));
        mon_log_write(MON_ZCLIENT_SETZNODEWATCH_1, SQ_LOG_CRIT, buf);
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
            HandleZSessionExpiration();
            break;
        default:
            break;
        }
    }

    TRACE_EXIT;
    return( rc );
}

void CZClient::StartClusterMonitoring( void )
{
    const char method_name[] = "CZClient::StartClusterMonitoring";
    TRACE_ENTRY;

    if ( !IsEnabled() )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Cluster monitoring started!\n\n", method_name, __LINE__ );
        }
        SetEnabled( true );
        SetState( ZC_WATCH );
        CLock::wakeOne();
    }

    TRACE_EXIT;
}

void CZClient::StopClusterMonitoring( void )
{
    const char method_name[] = "CZClient::StopClusterMonitoring";
    TRACE_ENTRY;

    if ( IsEnabled() )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "\n%s@%d Cluster monitoring stopped!\n", method_name, __LINE__ );
        }
        SetCheckCluster( false );
        SetEnabled( false );
        SetState( ZC_DISABLED );
        CLock::wakeOne();
    }

    TRACE_EXIT;
}

int CZClient::ShutdownWork(void)
{
    const char method_name[] = "CZClient::ShutdownWork";
    TRACE_ENTRY;

    // Set flag that tells the commAcceptor thread to exit
    SetState( ZC_SHUTDOWN );
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
    zooClient->MonitorZCluster();

    TRACE_EXIT;
    return NULL;
}


// Create the ZClientThread
int CZClient::StartWork()
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
        ZClient->SetState( CZClient::ZC_START );
        ZClient->CLock::wakeOne();
    }
    TRACE_EXIT;
}

void CZClient::StopMonitoring( void )
{
    const char method_name[] = "CZClient::StopMonitoring";
    TRACE_ENTRY;
    ZClient->SetState( CZClient::ZC_STOP );
    ZClient->CLock::wakeOne();
    TRACE_EXIT;
}

void CZClient::TriggerCheck( int type, const char *znodePath )
{
    const char method_name[] = "CZClient::TriggerCheck";
    TRACE_ENTRY;
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d" " - state = %s\n"
                    , method_name, __LINE__
                    , ZooConnectionTypeStr( type ) );
    }

    if ( type == ZOO_CREATED_EVENT )
    {
        SetState( ZC_ZNODE, znodePath );
    }
    else if ( type == ZOO_DELETED_EVENT )
    {
        SetState( ZC_ZNODE, znodePath );
    }
    else if ( type == ZOO_CHANGED_EVENT )
    {
        SetState( ZC_ZNODE, znodePath );
    }
    else if ( type == ZOO_CHILD_EVENT )
    {
        SetState( ZC_CLUSTER, znodePath );
    }
    else if ( type == ZOO_NOTWATCHING_EVENT )
    {
        SetState( ZC_CLUSTER );
    }
    CLock::wakeOne();
    TRACE_EXIT;
}

void CZClient::WatchCluster( void )
{
    const char method_name[] = "CZClient::WatchCluster";
    TRACE_ENTRY;

    int rc;
    struct String_vector nodes;

    if ( !IsCheckCluster() )
    {
        rc = GetClusterZNodes( &nodes );
        if ( rc != ZOK )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], GetClusterZNodes() failed!\n"
                    , method_name );
            mon_log_write(MON_ZCLIENT_WATCHCLUSTER_1, SQ_LOG_ERR, buf);
            SetState( CZClient::ZC_STOP );
            CLock::wakeOne();
            return;
        }

        stringstream newpath;
        string monZnode;
    
        if ( nodes.count > 0 )
        {
            for (int i = 0; i < nodes.count ; i++ )
            {
                newpath.str( "" );
                newpath << zkRootNode_.c_str() 
                        << zkRootNodeInstance_.c_str() 
                        << ZCLIENT_CLUSTER_ZNODE << "/"
                        << nodes.data[i];
                string monZnode = newpath.str( );
            
                rc = SetZNodeWatch( monZnode );
                if ( rc != ZOK )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s], GetZNodeData() failed!\n"
                            , method_name );
                    mon_log_write(MON_ZCLIENT_WATCHCLUSTER_2, SQ_LOG_ERR, buf);

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
                                    , monZnode.c_str() );
                    }
                }
            }
            SetCheckCluster( true );
            SetState( ZC_CLUSTER );
            FreeStringVector( &nodes );
        }
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d CheckCluster is NOT set!\n"
                        , method_name, __LINE__ );
        }
    }
    
    TRACE_EXIT;
}

int CZClient::WatchNode( const char *nodeName )
{
    const char method_name[] = "CZClient::WatchNode";
    TRACE_ENTRY;

    int rc;
    stringstream newpath;
    newpath.str( "" );
    newpath << zkRootNode_.c_str() 
            << zkRootNodeInstance_.c_str() 
            << ZCLIENT_CLUSTER_ZNODE << "/"
            << nodeName;
    string monZnode = newpath.str( );

    lock();
    rc = SetZNodeWatch( monZnode );
    unlock();
    if ( rc != ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], SetZNodeWatch() failed!\n"
                , method_name );
        mon_log_write(MON_ZCLIENT_WATCHNODE_1, SQ_LOG_ERR, buf);
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

int CZClient::WatchNodeDelete( const char *nodeName )
{
    const char method_name[] = "CZClient::WatchNodeDelete";
    TRACE_ENTRY;

    int rc = -1;

    stringstream newpath;
    newpath.str( "" );
    newpath << zkRootNode_.c_str() 
            << zkRootNodeInstance_.c_str() 
            << ZCLIENT_CLUSTER_ZNODE << "/"
            << nodeName;
    string monZnode = newpath.str( );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d zoo_delete(%s)\n"
                    , method_name, __LINE__
                    , monZnode.c_str() );
    }
    rc = zoo_delete( ZHandle
                   , monZnode.c_str( )
                   , -1 );
    if ( rc == ZOK )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], znode (%s) deleted!\n"
                , method_name, nodeName );
        mon_log_write(MON_ZCLIENT_WATCHNODEDELETE_1, SQ_LOG_INFO, buf);
    }
    else if ( rc == ZNONODE ||
              rc == ZCONNECTIONLOSS || 
              rc == ZOPERATIONTIMEOUT )
    {
        rc = ZOK;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], znode (%s) already deleted or cannot be accessed!\n"
                , method_name, nodeName );
        mon_log_write(MON_ZCLIENT_WATCHNODEDELETE_2, SQ_LOG_INFO, buf);
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], zoo_delete(%s) failed with error %s\n"
                , method_name, nodeName, ZooErrorStr(rc) );
        mon_log_write(MON_ZCLIENT_WATCHNODEDELETE_3, SQ_LOG_CRIT, buf);
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
            HandleZSessionExpiration();
            break;
        default:
            break;
        }
    }

    TRACE_EXIT;
    return( rc );
}

