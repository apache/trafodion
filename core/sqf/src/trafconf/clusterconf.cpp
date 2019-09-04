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

using namespace std;

#include <errno.h>
#include <assert.h>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include "tclog.h"
#include "tctrace.h"
#include "trafconf/trafconfig.h"
#include "clusterconf.h"


///////////////////////////////////////////////////////////////////////////////
//  Cluster Configuration
///////////////////////////////////////////////////////////////////////////////

CClusterConfig::CClusterConfig( void )
              : CPNodeConfigContainer(TC_NODES_MAX)
              , CLNodeConfigContainer(TC_NODES_MAX)
              , configMaster_(-1)
              , clusterId_(-1)
              , instanceId_(-1)
              , isRealCluster_(true)
              , nodeReady_(false)
              , persistReady_(false)
              , newPNodeConfig_(true)
              , trafConfigInitialized_(false)
              , trafConfigStorageType_(TCDBSTOREUNDEFINED)
              , prevPNodeConfig_(NULL)
              , prevLNodeConfig_(NULL)
              , prevPersistConfig_(NULL)
{
    const char method_name[] = "CClusterConfig::CClusterConfig";
    TRACE_ENTRY;

    if ( getenv( "SQ_VIRTUAL_NODES" ) )
    {
        isRealCluster_ = false;
    }

    char *env;
    env = getenv("TRAF_CLUSTER_ID");
    if ( env && isdigit(*env) )
    {
        clusterId_ = atoi(env);
    }
    else
    {
        char la_buf[TC_LOG_BUF_SIZE];
        sprintf( la_buf
               , "[%s], Environment variable TRAF_CLUSTER_ID is undefined, exiting!\n"
               , method_name);
        TcLogWrite( MON_CLUSTERCONF_CLUSTERCONFIG_1, TC_LOG_CRIT, la_buf );
        exit(EXIT_FAILURE);
    }
    env = getenv("TRAF_INSTANCE_ID");
    if ( env && isdigit(*env) )
    {
        instanceId_ = atoi(env);
    }
    else
    {
        char la_buf[TC_LOG_BUF_SIZE];
        sprintf( la_buf
               , "[%s], Environment variable TRAF_INSTANCE_ID is undefined, exiting!\n"
               , method_name);
        TcLogWrite( MON_CLUSTERCONF_CLUSTERCONFIG_2, TC_LOG_CRIT, la_buf );
        exit(EXIT_FAILURE);
    }

    memset( &configMasterName_, 0, TC_PROCESSOR_NAME_MAX );

    TRACE_EXIT;
}

CClusterConfig::~CClusterConfig ( void )
{
    const char method_name[] = "CClusterConfig::~CClusterConfig";
    TRACE_ENTRY;

    TRACE_EXIT;
}

void CClusterConfig::Clear( void )
{
    const char method_name[] = "CClusterConfig::Clear";
    TRACE_ENTRY;

    // Delete the node configuration objects
    CPNodeConfigContainer::Clear();
    CLNodeConfigContainer::Clear();
    CPersistConfigContainer::Clear();

    nodeReady_ = false;
    persistReady_ = false;
    newPNodeConfig_ = true;
    prevPNodeConfig_ = NULL;

    if ( trafConfigInitialized_ )
    {
        int rc = tc_close();
        if ( rc )
        {
            char la_buf[TC_LOG_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s], Can't close configuration!\n"
                    , method_name );
            TcLogWrite( MON_CLUSTERCONF_CLEAR_1, TC_LOG_CRIT, la_buf );
        }
    
        trafConfigInitialized_ = false;
    }

    TRACE_EXIT;
}

void CClusterConfig::AddNodeConfiguration( pnodeConfigInfo_t &pnodeConfigInfo
                                         , lnodeConfigInfo_t &lnodeConfigInfo )
{
    const char method_name[] = "CClusterConfig::AddNodeConfiguration";
    TRACE_ENTRY;

    if ( TcTraceSettings & TC_TRACE_INIT )
    {
        trace_printf( "%s@%d nid=%d, pnid=%d, nodename=%s, domainname=%s\n"
                    , method_name, __LINE__
                    , lnodeConfigInfo.nid
                    , pnodeConfigInfo.pnid
                    , pnodeConfigInfo.nodename
                    , pnodeConfigInfo.domainname );
    }

    if ( newPNodeConfig_ )
    {
        prevPNodeConfig_ = CPNodeConfigContainer::AddPNodeConfig( pnodeConfigInfo );
        newPNodeConfig_ = false;
    }
    prevLNodeConfig_ = CLNodeConfigContainer::AddLNodeConfig( prevPNodeConfig_
                                                            , lnodeConfigInfo );

    TRACE_EXIT;
}

void CClusterConfig::AddSNodeConfiguration( pnodeConfigInfo_t &pnodeConfigInfo )
{
    const char method_name[] = "CClusterConfig::AddSNodeConfiguration";
    TRACE_ENTRY;

    if ( TcTraceSettings & TC_TRACE_INIT )
    {
        trace_printf( "%s@%d pnid=%d, nodename=%s, domainname=%s\n"
                    , method_name, __LINE__
                    , pnodeConfigInfo.pnid
                    , pnodeConfigInfo.nodename
                    , pnodeConfigInfo.domainname );
    }

    if ( newPNodeConfig_ )
    {
        prevPNodeConfig_ = CPNodeConfigContainer::AddPNodeConfig( pnodeConfigInfo );
        prevPNodeConfig_->SetSpareList( pnodeConfigInfo.sparePNid
                                      , pnodeConfigInfo.spareCount );
        newPNodeConfig_ = false;
    }

    TRACE_EXIT;
}

void CClusterConfig::AddPersistConfiguration( persistConfigInfo_t &persistConfigInfo )
{
    const char method_name[] = "CClusterConfig::AddPersistConfiguration";
    TRACE_ENTRY;

    if ( TcTraceSettings & TC_TRACE_INIT )
    {
        trace_printf( "%s@%d persistkey=%s\n"
                    , method_name, __LINE__
                    , persistConfigInfo.persistPrefix );
    }

    prevPersistConfig_ = CPersistConfigContainer::AddPersistConfig( persistConfigInfo );

    TRACE_EXIT;
}

bool CClusterConfig::DeleteNodeConfig( int  pnid )
{
    const char method_name[] = "CClusterConfig::DeleteNodeConfig";
    TRACE_ENTRY;

    bool rs = true;
    int rc;

    if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d deleting (pnid=%d), pnodesCount=%d, lnodesCount=%d\n"
                     , method_name, __LINE__
                     , pnid
                     , GetPNodesCount()
                     , GetLNodesCount() );
    }

    // Delete logical and physical nodes from the configuration database
    
    rc = tc_delete_node( pnid, NULL );
    if ( rc == 0 )
    {
        // Delete logical and physical nodes from configuration objects
        CPNodeConfig *pnodeConfig = GetPNodeConfig( pnid );
        if (pnodeConfig)
        {

            CLNodeConfig *lnodeConfig = pnodeConfig->GetFirstLNodeConfig();
            while ( lnodeConfig )
            {
                // Delete logical nodes unique strings from the configuration database
                rc = tc_delete_unique_strings( lnodeConfig->GetNid() );
                if ( rc )
                {
                    rs = false;
                    break;
                }
                DeleteLNodeConfig( lnodeConfig );
                lnodeConfig = pnodeConfig->GetFirstLNodeConfig();
            }

            if (rs)
            {
                DeletePNodeConfig( pnodeConfig );

                if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
                {
                    trace_printf( "%s@%d deleted (pnid=%d), pnodesCount=%d, lnodesCount=%d\n"
                                 , method_name, __LINE__
                                 , pnid
                                 , GetPNodesCount()
                                 , GetLNodesCount() );
                }
            }
        }
    }
    else
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] Node delete failed, pnid=%d\n",
                  method_name,  pnid );
        TcLogWrite( MON_CLUSTERCONF_DELETENODE_1, TC_LOG_ERR, buf );
        rs = false;
    }

    TRACE_EXIT;
    return( rs );
}

// The following method maps the 'sqconfig' text file persist section's
// <persist-key>_PROCESS_TYPE string value to the internal
// TcProcessType_t enum value
TcProcessType_t CClusterConfig::GetProcessType( const char *processtype )
{
    if (strcmp( "DTM", processtype) == 0)
    {
        return(ProcessType_DTM);
    }
    else if (strcmp( "GENERIC", processtype) == 0)
    {
        return(ProcessType_Generic);
    }
    else if (strcmp( "WDG", processtype) == 0)
    {
        return(ProcessType_Watchdog);
    }
    else if (strcmp( "TNS", processtype) == 0)
    {
        return(ProcessType_NameServer);
    }
    else if (strcmp( "MXOSRVR", processtype) == 0)
    {
        return(ProcessType_MXOSRVR);
    }
    else if (strcmp( "SPX", processtype) == 0)
    {
        return(ProcessType_SPX);
    }
    else if (strcmp( "SSMP", processtype) == 0)
    {
        return(ProcessType_SSMP);
    }
    else if (strcmp( "PSD", processtype) == 0)
    {
        return(ProcessType_PSD);
    }
    else if (strcmp( "SMS", processtype) == 0)
    {
        return(ProcessType_SMS);
    }
    else if (strcmp( "TMID", processtype) == 0)
    {
        return(ProcessType_TMID);
    }
    else if (strcmp( "PERSIST", processtype) == 0)
    {
        return(ProcessType_PERSIST);
    }

    return(ProcessType_Undefined);
}

bool CClusterConfig::Initialize( void )
{
    return( Initialize( false, NULL ) );
}

bool CClusterConfig::Initialize( bool traceEnabled, const char *traceFile )
{
    const char method_name[] = "CClusterConfig::Initialize";
    TRACE_ENTRY;

    if ( trafConfigInitialized_ )
    {
        // Already initialized
        return( true );
    }

    int rc = tc_initialize( traceEnabled, traceFile );
    if ( rc )
    {
        char la_buf[TC_LOG_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s], Can't initialize configuration!\n"
                , method_name );
        TcLogWrite( MON_CLUSTERCONF_INIT_1, TC_LOG_CRIT, la_buf );
        return( false );
    }

    trafConfigInitialized_ = true;
    trafConfigStorageType_ = tc_get_storage_type();

    TRACE_EXIT;
    return( true );
}

void CClusterConfig::InitCoreMask( cpu_set_t &coreMask )
{
    CPU_ZERO( &coreMask );
}

bool CClusterConfig::LoadConfig( void )
{
    const char method_name[] = "CClusterConfig::LoadConfig";
    TRACE_ENTRY;

    if ( LoadNodeConfig() )
    {
        LoadPersistConfig();
    }

    TRACE_EXIT;
    return( nodeReady_ && persistReady_ );
}

bool CClusterConfig::LoadNodeConfig( void )
{
    const char method_name[] = "CClusterConfig::LoadNodeConfig";
    TRACE_ENTRY;

    int rc;
    int nodeCount = 0;
    int snodeCount = 0;
    TcNodeConfiguration_t           nodeConfigData[TC_NODES_MAX];
    TcPhysicalNodeConfiguration_t   spareNodeConfigData[TC_SPARE_NODES_MAX];
    pnodeConfigInfo_t               pnodeConfigInfo;
    lnodeConfigInfo_t               lnodeConfigInfo;

    rc = tc_get_nodes( &nodeCount
                     , TC_NODES_MAX
                     , nodeConfigData );
    if ( rc )
    {
        char la_buf[TC_LOG_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] Node configuration access failed!\n"
                , method_name );
        TcLogWrite(MON_CLUSTERCONF_LOADNODE_1, TC_LOG_CRIT, la_buf);
        return( false );
    }

    // Process logical nodes
    for (int i =0; i < nodeCount; i++ )
    {
        if ( TcTraceSettings & TC_TRACE_INIT )
        {
            trace_printf( "%s@%d node_name=%s, domain_name=%s\n"
                          , method_name, __LINE__
                          , nodeConfigData[i].node_name
                          , nodeConfigData[i].domain_name );
        }

        ProcessLNode( nodeConfigData[i], pnodeConfigInfo, lnodeConfigInfo );
        // We want to pick the first configured node so all monitors pick the same one
        // This only comes into play for a Trafodion start from scratch
        if (i == 0)
        {
            configMaster_ = pnodeConfigInfo.pnid;
            strncpy( configMasterName_ , pnodeConfigInfo.nodename, sizeof(configMasterName_) );
        }
        AddNodeConfiguration( pnodeConfigInfo, lnodeConfigInfo );
    }

    rc = tc_get_snodes( &snodeCount
                     , TC_NODES_MAX
                     , spareNodeConfigData );
    if ( rc )
    {
        char la_buf[TC_LOG_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] Node configuration access failed!\n"
                , method_name );
        TcLogWrite(MON_CLUSTERCONF_LOADNODE_2, TC_LOG_CRIT, la_buf);
        return( false );
    }

    // Process spare nodes
    for (int i =0; i < snodeCount; i++ )
    {
        ProcessSNode( spareNodeConfigData[i], pnodeConfigInfo );
        AddSNodeConfiguration( pnodeConfigInfo );
    }

    prevPNodeConfig_ = NULL;
    prevLNodeConfig_ = NULL;
    nodeReady_ = true;
    
    if ( TcTraceSettings & TC_TRACE_INIT )
    {
        if ( nodeReady_ )
            trace_printf("%s@%d - Successfully loaded node configuration\n", method_name, __LINE__);
        else
            trace_printf("%s@%d - Failed to load node configuration\n", method_name, __LINE__);
    }

    TRACE_EXIT;
    return( nodeReady_ );
}

bool CClusterConfig::LoadPersistConfig( void )
{
    const char method_name[] = "CClusterConfig::LoadPersistConfig";
    TRACE_ENTRY;

    int  rc;

    // Get persistent process keys
    char persistProcessKeys[TC_PERSIST_KEYS_VALUE_MAX];
    rc = tc_get_persist_keys( persistProcessKeys );
    if ( rc )
    {
        char la_buf[TC_LOG_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] Persist keys configuration access failed!\n"
                , method_name );
        TcLogWrite(MON_CLUSTERCONF_LOADPERSIST_1, TC_LOG_CRIT, la_buf);
        return( false );
    }

    TcPersistConfiguration_t persistConfig;
    persistConfigInfo_t     persistConfigInfo;
    pkeysVector_t     pkeysVector;   // vector of persist prefix strings

    // Initialize vector of persistent keys
    CPersistConfigContainer::InitializePersistKeys( persistProcessKeys
                                                  , pkeysVector );
    if ( CPersistConfigContainer::GetPersistKeysCount() == 0 )
    {
        char la_buf[TC_LOG_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] Invalid PERSIST_PROCESS_KEYS value, %s\n"
                , method_name, persistProcessKeys );
        TcLogWrite(MON_CLUSTERCONF_LOADPERSIST_2, TC_LOG_CRIT, la_buf);
        return( false );
    }

    pkeysVector_t::iterator pkit;
    
    // Process each prefix in the vector
    for (pkit = pkeysVector.begin(); pkit < pkeysVector.end(); pkit++ )
    {
        memset( &persistConfig, 0, sizeof(TcPersistConfiguration_t) );
        memset( &persistConfigInfo, 0, sizeof(persistConfigInfo_t) );
        strncpy( persistConfig.persist_prefix
               , pkit->c_str()
               , sizeof(persistConfig.persist_prefix));
        rc = tc_get_persist_process( pkit->c_str(), &persistConfig );
        if ( rc )
        {
            char la_buf[TC_LOG_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] Persist process info for prefix key %s does not exist!\n"
                    , method_name, pkit->c_str() );
            TcLogWrite(MON_CLUSTERCONF_LOADPERSIST_3, TC_LOG_CRIT, la_buf);
            return( false );
        }
    
        ProcessPersistInfo( persistConfig, persistConfigInfo );
        AddPersistConfiguration( persistConfigInfo );
    }

    persistReady_ = true;

    if ( TcTraceSettings & TC_TRACE_INIT )
    {
        if ( persistReady_ )
            trace_printf("%s@%d - Successfully loaded persist configuration\n", method_name, __LINE__);
        else
            trace_printf("%s@%d - Failed to load persist configuration\n", method_name, __LINE__);
    }

    TRACE_EXIT;
    return( persistReady_ );
}

void CClusterConfig::ProcessLNode( TcNodeConfiguration_t &nodeConfigData
                                 , pnodeConfigInfo_t  &pnodeConfigInfo
                                 , lnodeConfigInfo_t  &lnodeConfigInfo )
{
    const char method_name[] = "CClusterConfig::ProcessLNode";
    TRACE_ENTRY;

    bool excludedCores = false;

    if ( TcTraceSettings & TC_TRACE_INIT )
    {
        trace_printf( "%s@%d nid=%d, pnid=%d, name=%s, domain=%s, "
                      "excluded cores=(%d:%d), cores=(%d:%d), "
                      "processors=%d, roles=%d\n"
                    , method_name, __LINE__
                    , nodeConfigData.nid
                    , nodeConfigData.pnid
                    , nodeConfigData.node_name
                    , nodeConfigData.domain_name
                    , nodeConfigData.excluded_first_core
                    , nodeConfigData.excluded_last_core
                    , nodeConfigData.first_core
                    , nodeConfigData.last_core
                    , nodeConfigData.processors
                    , nodeConfigData.roles );
    }

    newPNodeConfig_ = ((prevPNodeConfig_ == NULL) ||
                       (nodeConfigData.pnid != prevPNodeConfig_->GetPNid()))
                        ? true : false;
    if ( newPNodeConfig_ )
    {
        memset( &pnodeConfigInfo, 0, sizeof(pnodeConfigInfo) );
        pnodeConfigInfo.pnid = nodeConfigData.pnid;
        strncpy( pnodeConfigInfo.nodename
               , nodeConfigData.node_name
               , sizeof(pnodeConfigInfo.nodename) );
        strncpy( pnodeConfigInfo.domainname
               , nodeConfigData.domain_name
               , sizeof(pnodeConfigInfo.domainname) );
        pnodeConfigInfo.excludedFirstCore = nodeConfigData.excluded_first_core;
        pnodeConfigInfo.excludedLastCore  = nodeConfigData.excluded_last_core;
        excludedCores = (nodeConfigData.excluded_first_core != -1 || 
                         nodeConfigData.excluded_last_core != -1)
                       ? true : false;
        if ( excludedCores )
        {
            SetCoreMask( nodeConfigData.excluded_first_core
                       , nodeConfigData.excluded_last_core
                       , pnodeConfigInfo.excludedCoreMask );
        }
        else
        {
            InitCoreMask( pnodeConfigInfo.excludedCoreMask );
        }
    }

    lnodeConfigInfo.nid = nodeConfigData.nid;
    lnodeConfigInfo.pnid = nodeConfigData.pnid;
    strncpy( lnodeConfigInfo.nodename
           , nodeConfigData.node_name
           , sizeof(lnodeConfigInfo.nodename) );
    strncpy( lnodeConfigInfo.domainname
           , nodeConfigData.domain_name
           , sizeof(lnodeConfigInfo.domainname) );
    lnodeConfigInfo.firstCore = nodeConfigData.first_core;
    lnodeConfigInfo.lastCore  = nodeConfigData.last_core;
    SetCoreMask( nodeConfigData.first_core
               , nodeConfigData.last_core
               , lnodeConfigInfo.coreMask );
    lnodeConfigInfo.processor = nodeConfigData.processors;
    lnodeConfigInfo.zoneType  = static_cast<TcZoneType_t>(nodeConfigData.roles);

    TRACE_EXIT;
}

void CClusterConfig::ProcessSNode( TcPhysicalNodeConfiguration_t &pnodeConfig
                                 , pnodeConfigInfo_t             &pnodeConfigInfo )
{
    const char method_name[] = "CClusterConfig::ProcessSNode";
    TRACE_ENTRY;

    if ( TcTraceSettings & TC_TRACE_INIT )
    {
        trace_printf( "%s@%d pnid=%d, node_name=%s, "
                      "excluded cores=(%d:%d), spareCount=%d\n"
                    , method_name, __LINE__
                    , pnodeConfig.pnid
                    , pnodeConfig.node_name
                    , pnodeConfig.excluded_first_core
                    , pnodeConfig.excluded_last_core
                    , pnodeConfig.spare_count
                    );
    }

    newPNodeConfig_ = ((prevPNodeConfig_ == NULL) ||
                       (pnodeConfig.pnid != prevPNodeConfig_->GetPNid()))
                        ? true : false;
    if ( newPNodeConfig_ )
    {
        if ( TcTraceSettings & TC_TRACE_INIT )
        {
            trace_printf( "%s@%d node_name=%s, domain_name=%s\n"
                          , method_name, __LINE__
                          , pnodeConfigInfo.nodename
                          , pnodeConfigInfo.domainname );
        }

        bool excludedCores = (pnodeConfig.excluded_first_core != -1 || 
                              pnodeConfig.excluded_last_core != -1)
                                ? true : false;
        if ( excludedCores )
        {
            SetCoreMask( pnodeConfig.excluded_first_core
                       , pnodeConfig.excluded_last_core
                       , pnodeConfigInfo.excludedCoreMask );
        }

        memset( pnodeConfigInfo.sparePNid, 255, sizeof(pnodeConfigInfo.sparePNid) );

        pnodeConfigInfo.spareCount = pnodeConfig.spare_count;
        for (int i = 0; i < pnodeConfigInfo.spareCount ; i++ )
        {
            pnodeConfigInfo.sparePNid[i] = pnodeConfig.spare_pnid[i];
        }
    }

    TRACE_EXIT;
}

void CClusterConfig::ProcessPersistInfo( TcPersistConfiguration_t &persistConfig
                                       , persistConfigInfo_t      &persistConfigInfo )
{
    const char method_name[] = "CClusterConfig::ProcessPersistInfo";
    TRACE_ENTRY;

    char workValue[TC_PERSIST_VALUE_MAX];
    char *token1;
    char *token2;
    static const char *delimPercent = "%";
    static int chPercent = '%';

    if ( TcTraceSettings & TC_TRACE_INIT )
    {
        trace_printf( "%s@%d Processing persist info for persistKey=%s\n"
                    , method_name, __LINE__
                    , persistConfig.persist_prefix );
    }
    
    strncpy( persistConfigInfo.persistPrefix
           , persistConfig.persist_prefix
           , sizeof(persistConfigInfo.persistPrefix) );

    strncpy( workValue, persistConfig.process_name, sizeof(workValue) );
    if (strlen(workValue))
    {
        // Extract name prefix
        token1 = strtok( workValue, delimPercent );
        if (token1)
        {
            strncpy( persistConfigInfo.processNamePrefix
                   , token1
                   , sizeof(persistConfigInfo.processNamePrefix) );
        }
        // Extract nid format
        strncpy( workValue, persistConfig.process_name, sizeof(workValue) );
        token2 = strchr( workValue, chPercent );
        if (token2)
        {
            strncpy( persistConfigInfo.processNameFormat
                   , token2
                   , sizeof(persistConfigInfo.processNameFormat) );
        }
    }

    persistConfigInfo.processType = GetProcessType( persistConfig.process_type );

    strncpy( persistConfigInfo.programName
           , persistConfig.program_name
           , sizeof(persistConfigInfo.programName) );

    strncpy( persistConfigInfo.programArgs
           , persistConfig.program_args
           , sizeof(persistConfigInfo.programArgs) );

    persistConfigInfo.requiresDTM = persistConfig.requires_DTM;

    strncpy( workValue, persistConfig.std_out, sizeof(workValue) );
    if (strlen(workValue))
    {
        // Extract name prefix
        token1 = strtok( workValue, delimPercent );
        if (token1)
        {
            strncpy( persistConfigInfo.stdoutPrefix
                   , token1
                   , sizeof(persistConfigInfo.stdoutPrefix) );
        }
        // Extract nid format
        strncpy( workValue, persistConfig.std_out, sizeof(workValue) );
        token2 = strchr( workValue, chPercent );
        if (token2)
        {
            strncpy( persistConfigInfo.stdoutFormat
                   , token2
                   , sizeof(persistConfigInfo.stdoutFormat) );
        }
    }
    
    persistConfigInfo.persistRetries = persistConfig.persist_retries;

    persistConfigInfo.persistWindow = persistConfig.persist_window;

    strncpy( persistConfigInfo.zoneFormat
           , persistConfig.persist_zones
           , sizeof(persistConfigInfo.zoneFormat) );

    TRACE_EXIT;
}

bool CClusterConfig::SaveNodeConfig( const char *name
                                   , const char *domain
                                   , int         nid
                                   , int         pnid
                                   , int         firstCore
                                   , int         lastCore
                                   , int         processors
                                   , int         excludedFirstCore
                                   , int         excludedLastCore
                                   , int         roles )
{
    const char method_name[] = "CClusterConfig::SaveNodeConfig";
    TRACE_ENTRY;

    bool rs = true;
    int  rc;
    TcNodeConfiguration_t       nodeConfig;
    pnodeConfigInfo_t           pnodeConfigInfo;
    lnodeConfigInfo_t           lnodeConfigInfo;

    if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Saving node config "
                      "(name=%s, domain=%s, processors=%d, "
                      "roles=%d, firstCore=%d, lastCore=%d "
                      "excludedFirstCore=%d, excludedLastCore=%d)\n"
                     , method_name, __LINE__
                     , name
                     , domain
                     , processors
                     , roles
                     , firstCore
                     , lastCore
                     , excludedFirstCore
                     , excludedLastCore );
    }
    
    nodeConfig.nid  = nid;
    nodeConfig.pnid = pnid;
    strncpy( nodeConfig.node_name, name, sizeof(nodeConfig.node_name) );
    strncpy( nodeConfig.domain_name, domain, sizeof(nodeConfig.domain_name) );
    nodeConfig.excluded_first_core = excludedFirstCore;
    nodeConfig.excluded_last_core  = excludedLastCore;
    nodeConfig.first_core = firstCore;
    nodeConfig.last_core  = lastCore;
    nodeConfig.processors = processors;
    nodeConfig.roles      = roles;
    
    // Insert data into pnode and lnode tables
    rc = tc_put_node( &nodeConfig );
    if ( rc == 0 )
    {
        ProcessLNode( nodeConfig, pnodeConfigInfo, lnodeConfigInfo );
        // Add new logical and physical nodes to configuration objects
        AddNodeConfiguration( pnodeConfigInfo, lnodeConfigInfo );
    }
    else
    {
        rs = false;
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] Node add failed, pnid=%d\n",
                  method_name,  pnid );
        TcLogWrite( MON_CLUSTERCONF_SAVENODE_1, TC_LOG_ERR, buf );
    }

    TRACE_EXIT;
    return( rs );
}

void CClusterConfig::SetCoreMask( int        firstCore
                                , int        lastCore
                                , cpu_set_t &coreMask )
{
    CPU_ZERO( &coreMask );
    for (int i = firstCore; i < (lastCore+1) ; i++ )
    {
        CPU_SET( i, &coreMask );
    }
}

bool CClusterConfig::UpdatePNodeConfig( int         pnid
                                      , const char *name
                                      , const char *domain
                                      , int         excludedFirstCore
                                      , int         excludedLastCore )
{
    const char method_name[] = "CClusterConfig::UpdatePNodeConfig";
    TRACE_ENTRY;

    bool rs = true;
    int  rc;
    TcPhysicalNodeConfiguration_t pnodeConfig;

    if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Updating pnode config "
                      "(pnid=%d, name=%s, domain=%s, "
                      "excludedFirstCore=%d, excludedLastCore=%d)\n"
                     , method_name, __LINE__
                     , pnid
                     , name
                     , domain
                     , excludedFirstCore
                     , excludedLastCore );
    }

    memset( &pnodeConfig, 0, sizeof(TcPhysicalNodeConfiguration_t) );
    pnodeConfig.pnid = pnid;
    if (strlen(domain))
    {
        snprintf( pnodeConfig.node_name, sizeof(pnodeConfig.node_name)
                , "%s.%s", name, domain );
    }
    else
    {
        strncpy( pnodeConfig.node_name, name, sizeof(pnodeConfig.node_name) );
    }
    pnodeConfig.excluded_first_core = excludedFirstCore;
    pnodeConfig.excluded_last_core  = excludedLastCore;
    
    // Update pnode table
    rc = tc_put_pnode( &pnodeConfig );
    if ( rc == 0 )
    {
        // Update physical node to configuration object
        UpdatePNodeConfiguration( pnid
                                , name
                                , domain
                                , excludedFirstCore
                                , excludedLastCore );
    }
    else
    {
        rs = false;
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] PNode update failed, pnid=%d, name=%s, domain=%s\n"
                , method_name,  pnid, name, domain );
        TcLogWrite( MON_CLUSTERCONF_UPDATEPNODECFG_1, TC_LOG_ERR, buf );
    }

    TRACE_EXIT;
    return( rs );
}

void CClusterConfig::UpdatePNodeConfiguration( int         pnid
                                             , const char *name
                                             , const char *domain
                                             , int         excludedFirstCore
                                             , int         excludedLastCore )
{
    const char method_name[] = "CClusterConfig::UpdatePNodeConfiguration";
    TRACE_ENTRY;

    if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d pnid=%d, name=%s, "
                       "excludedFirstCore=%d, excludedLastCore=%d\n"
                    , method_name, __LINE__
                    , pnid
                    , name
                    , excludedFirstCore
                    , excludedLastCore );
    }

    CPNodeConfig *pnodeConfig = GetPNodeConfig( pnid );
    if ( pnodeConfig )
    {
        pnodeConfig->SetName( name );
        pnodeConfig->SetDomain( domain );
        pnodeConfig->SetExcludedFirstCore( excludedFirstCore );
        pnodeConfig->SetExcludedLastCore( excludedLastCore );
    }

    TRACE_EXIT;
}

