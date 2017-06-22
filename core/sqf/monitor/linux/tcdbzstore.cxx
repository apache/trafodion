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

#include <string>
#include <stdlib.h>
#include <string.h>

using namespace std;

#include "type2str.h"
#include "tclog.h"
#include "tctrace.h"
#include "trafconfig.h"
#include "tcdbzstore.h"

CTcdbZstore::CTcdbZstore( void )
           : CTcdbStore( TCDBZOOKEEPER )
{
    const char method_name[] = "CTcdbZstore::CTcdbZstore";
    TRACE_ENTRY;

    memcpy(&eyecatcher_, "TCZS", 4);

    TRACE_EXIT;
}

CTcdbZstore::~CTcdbZstore ( void )
{
    const char method_name[] = "CTcdbZstore::~CTcdbZstore";
    TRACE_ENTRY;

    memcpy(&eyecatcher_, "tczs", 4);

    TRACE_EXIT;
}

int CTcdbZstore::AddRegistryKey( const char *key )
{
    const char method_name[] = "CTcdbZstore::AddRegistryKey";
    TRACE_ENTRY;

    key = key; // Make compiler happy!

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    // Do nothing, the key (znode) will be added along with the value (data)
    // on a subsequent method invocation of AddRegistryClusterData()
    // or AddRegistryProcessData()

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::AddRegistryProcess( const char *procName )
{
    const char method_name[] = "CTcdbZstore::AddRegistryProcess";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    //  /TRAFODION_REGISTRY_PROCESS_ZNODE
    //      /<process-name>
    stringstream ss;
    ss.str( "" );
    ss << regProcessZNodePath_ << "/"
       << procName;
    string znodePath(ss.str());
    
    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Adding registry process znode: %s\n"
                    , method_name, __LINE__, znodePath.c_str());
    }

    int rc = zConfig_.CreateConfigZNode( znodePath.c_str(), NULL, 0 );
    if ( rc != ZOK )
    {
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::AddRegistryClusterData( const char *key
                                       , const char *value )
{
    const char method_name[] = "CTcdbZstore::AddRegistryClusterData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    //  /TRAFODION_REGISTRY_CLUSTER_ZNODE
    //      /<key-name> (<value>)
    stringstream ss;
    ss.str( "" );
    ss << regClusterZNodePath_ << "/"
       << key;
    string znodePath(ss.str());
    
    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Adding registry cluster znode: %s (%s)\n"
                    , method_name, __LINE__, znodePath.c_str(), value);
    }

    int rc = zConfig_.CreateConfigZNode( znodePath.c_str(), value, 0 );
    if ( rc != ZOK )
    {
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::AddRegistryProcessData( const char *procName
                                       , const char *key
                                       , const char *value )
{
    const char method_name[] = "CTcdbZstore::AddRegistryProcessData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    //  /TRAFODION_REGISTRY_PROCESS_ZNODE
    //      /<process-name>
    //          /<key-name> (<value>)
    stringstream ss;
    ss.str( "" );
    ss << regProcessZNodePath_ << "/"
       << procName << "/"
       << key;
    string znodePath(ss.str());
    
    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Adding registry process znode: %s (%s)\n"
                    , method_name, __LINE__, znodePath.c_str(), value);
    }

    int rc = zConfig_.CreateConfigZNode( znodePath.c_str(), value, 0 );
    if ( rc != ZOK )
    {
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::AddUniqueString( int nid
                                , int id
                                , const char *uniqueStr )
{
    const char method_name[] = "CTcdbZstore::AddUniqueString";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    stringstream ss;

#if 1
    //  /TRAFODION_REGISTRY_USTRING_ZNODE
    //      /<nid-1>
    //          /<id-1> (<uniqueStr>)
    //           . . .
    //          /<id-n> (<uniqueStr>)
    ss.str( "" );
    ss << regUStringZNodePath_ << "/"
       << nid;
    string znodePath(ss.str());
    
    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Adding registry unique string znode: %s\n"
                    , method_name, __LINE__, znodePath.c_str());
    }

    rc = MakeConfigZNode( znodePath.c_str() );
    if ( rc && rc != ZNODEEXISTS )
    {
        TRACE_EXIT;
        return(rc);
    }

    ss.str( "" );
    ss << regUStringZNodePath_  << "/"
       << nid << "/"
       << id;
    znodePath = ss.str();

#else
    //  /TRAFODION_REGISTRY_USTRING_ZNODE
    //      /<id-1> (<uniqueStr>)
    ss.str( "" );
    ss << regUStringZNodePath_ << "/"
       << id;
    string znodePath(ss.str());
    
#endif

    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Adding registry unique string znode: %s (%s)\n"
                    , method_name, __LINE__, znodePath.c_str(), uniqueStr);
    }

    int rc = zConfig_.CreateConfigZNode( znodePath.c_str(), uniqueStr, 0 );
    if ( rc != ZOK )
    {
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf("%s@%d inserting unique string nid=%d id=%d into "
                     "monRegUniqueStrings\n", method_name, __LINE__,
                     nid, id);
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::Close( void )
{
    const char method_name[] = "CTcdbZstore::Close";
    TRACE_ENTRY;

    if ( ! IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    int rc = zConfig_.Close();

    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
    {
        trace_printf( "%s@%d Zookeeper store session ended! (%s)\n"
                    , method_name, __LINE__, tc_errmsg( rc ));
    }

    TRACE_EXIT;
    return( rc );
}

int CTcdbZstore::DeleteNodeData( int pnid )
{
    const char method_name[] = "CTcdbZstore::DeleteNodeData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    //
    // Get pnodesZNodePath_ children znodes 
    //      znode=<pnid>, data=(<hostname>:<excFirstCore>:<excLastCore>)
    //  Scan each znode for matching <pnid>
    //  Save matching pnode znode and <hostname>
    // Get clusterZNodePath_ children znodes
    //      znode=<hostname>, data=(<pnid>)
    //  Scan each znode's data for matching <pnid>
    //  Save matching cluster znode
    // Get lnodesZNodePath_ children znodes
    //      znode=<nid>, data=(<pnid>:<processors>:<roles>:<first-core>:<last-core>)
    //  Scan each znode's data for matching <pnid>
    //  Save matching lnode znode
    // Get snodesZNodePath_ children znodes
    //      znode=<hostname>, data=(<spare-hostname>)
    //  Scan each znode for matching <hostname>
    //  Save matching snode znode
    // 
    //  Delete saved cluster znode
    //  Delete saved pnode znode
    //  Delete saved lnode znode
    //  Delete saved snode znode
    // 

    stringstream ss;
    ss.str( "" );
    ss << lnodesZNodePath_ << "/"
       << nid;
    string znodePath(ss.str());
    
    if (TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Deleting logical node znode: %s\n" 
                     , method_name, __LINE__
                     , znodePath.c_str() );
    }

    int rc = zConfig_.DeleteConfigZNode( znodePath.c_str() );
    if ( rc != ZOK )
    {
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    if (TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d delete from lnode, pnode values (pNid=%d)\n"
                     , method_name, __LINE__
                     , pnid );
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::DeleteUniqueString( int nid )
{
    const char method_name[] = "CTcdbZstore::DeleteUniqueString";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d delete from monRegUniqueStrings values (nid=%d)\n"
                     , method_name, __LINE__
                     , nid );
    }

    //
    // Get regUStringZNodePath_ children znodes 
    //      znode=<usid>, data=(<ustring>)
    //  Loop on children
    //    Delete regUString znode
    //
    // NOTE: This will delete all ustrings on cluster, not just a give nid's
    //

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetNode( int nid
                        , node_configuration_t &nodeConfig )
{
    const char method_name[] = "CTcdbZstore::GetNode";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    //
    // List a node's configuration by <nid>
    //
    // Get lnodesZNodePath_ children znodes
    //      znode=<nid>, data=(<pnid>:<processors>:<roles>:<first-core>:<last-core>)
    //  Scan each lnodes znode for matching <nid>
    //      Save matching lnodes znode's data
    // Get pnodesZNodePath_ children znodes 
    //      znode=<pnid>, data=(<hostname>:<excFirstCore>:<excLastCore>)
    //  Scan each pnodes znode for matching <pnid>
    //      Save matching pnode znode's data
    // Return saved lnodes znode's and pnode znode's data
    //

    int  rc;
    int  firstcore = -1;
    int  lastcore = -1;
    int  excfirstcore = -1;
    int  exclastcore = -1;
    int  lnid = -1;
    int  pnid = -1;
    int  processors = 0;
    int  roles;

    SetLNodeData( lnid
                , pnid
                , nodename
                , excfirstcore
                , exclastcore
                , firstcore
                , lastcore
                , processors
                , roles
                , nodeConfig );

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetNode( const char *name
                        , node_configuration_t &nodeConfig )
{
    const char method_name[] = "CTcdbZstore::GetNode";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    //
    // List a node's configuration by <hostname>
    //
    // Get clusterZNodePath_ children znodes
    //      znode=<hostname>, data=(<pnid>)
    //  Scan each cluster znode for matching <hostname>
    //      Save matching cluster znode's data
    // Get pnodesZNodePath_ children znodes 
    //      znode=<pnid>, data=(<hostname>:<excFirstCore>:<excLastCore>)
    //  Scan each pnodes znode for matching <pnid>
    //      Save matching pnode znode's data
    // Get lnodesZNodePath_ children znodes
    //      znode=<nid>, data=(<pnid>:<processors>:<roles>:<first-core>:<last-core>)
    //  Scan each lnodes znode's data for matching <pnid>
    //      Save matching lnodes znode's data
    // Return saved lnodes znode's and pnode znode's data
    //

    int  rc;
    int  firstcore = -1;
    int  lastcore = -1;
    int  excfirstcore = -1;
    int  exclastcore = -1;
    int  nid = -1;
    int  pnid = -1;
    int  processors = 0;
    int  roles;

    pnid = sqlite3_column_int(prepStmt, 0);
    nid = sqlite3_column_int(prepStmt, 1);
    nodename = (const char *) sqlite3_column_text(prepStmt, 2);
    firstcore = sqlite3_column_int(prepStmt, 3);
    lastcore = sqlite3_column_int(prepStmt, 4);
    excfirstcore = sqlite3_column_int(prepStmt, 5);
    exclastcore = sqlite3_column_int(prepStmt, 6);
    processors = sqlite3_column_int(prepStmt, 7);
    roles = sqlite3_column_int(prepStmt, 8);
    SetLNodeData( nid
                , pnid
                , nodename
                , excfirstcore
                , exclastcore
                , firstcore
                , lastcore
                , processors
                , roles
                , nodeConfig );

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetNodes( int &count
                         , int max
                         , node_configuration_t nodeConfig[] )
{
    const char method_name[] = "CTcdbZstore::GetNodes";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    //
    // List all logical nodes, limited by max
    //

    sqlStmt = "select p.pNid, l.lNid, p.nodeName, l.firstCore, l.lastCore,"
                   " p.excFirstCore, p.excLastCore, l.processors, l.roles"
                   "  from pnode p, lnode l where p.pNid = l.pNid";

    //
    // Loop on lnodesZNodePath_ children
    //   Get lnodesZNodePath_ children znodes
    //      znode=<nid>, data=(<pnid>:<processors>:<roles>:<first-core>:<last-core>)
    //    Scan each lnodes znode for matching <nid>
    //      Save matching lnodes znode's data
    //   Get pnodesZNodePath_ children znodes 
    //      znode=<pnid>, data=(<hostname>:<excFirstCore>:<excLastCore>)
    //    Scan each pnodes znode for matching <pnid>
    //      Save matching pnode znode's data
    //   Save lnodes znode's and pnode znode's data in nodeConfig[nodeCount]
    //

    int  rc;
    int  firstcore = -1;
    int  lastcore = -1;
    int  excfirstcore = -1;
    int  exclastcore = -1;
    int  nid = -1;
    int  pnid = -1;
    int  processors = 0;
    int  roles;
    int  nodeCount = 0;

    while ( 1 )
    {
            if ( nodeCount < max )
            {
                pnid = sqlite3_column_int(prepStmt, 0);
                nid = sqlite3_column_int(prepStmt, 1);
                nodename = (const char *) sqlite3_column_text(prepStmt, 2);
                firstcore = sqlite3_column_int(prepStmt, 3);
                lastcore = sqlite3_column_int(prepStmt, 4);
                excfirstcore = sqlite3_column_int(prepStmt, 5);
                exclastcore = sqlite3_column_int(prepStmt, 6);
                processors = sqlite3_column_int(prepStmt, 7);
                roles = sqlite3_column_int(prepStmt, 8);
                SetLNodeData( nid
                            , pnid
                            , nodename
                            , excfirstcore
                            , exclastcore
                            , firstcore
                            , lastcore
                            , processors
                            , roles
                            , nodeConfig[nodeCount] );
                nodeCount++;
            }
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetPNode( int pNid
                         , physical_node_configuration_t &pnodeConfig )
{
    const char method_name[] = "CTcdbZstore::GetPNode";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int  rc;
    int  excfirstcore = -1;
    int  exclastcore = -1;
    int  pnid = -1;

    //
    // List a node's physical configuration by <pnid>
    //

    sqlStmt = "select p.pNid, p.nodeName, p.excFirstCore, p.excLastCore"
                   "  from pnode p where p.pNid = ?";

    //
    // Get pnodesZNodePath_/<pnid> znode
    //      znode=<pnid>, data=(<hostname>:<excFirstCore>:<excLastCore>)
    //      Save matching pnode znode's data
    // Return saved pnode znode's data
    //

    pnid = sqlite3_column_int(prepStmt, 0);
    nodename = (const char *) sqlite3_column_text(prepStmt, 1);
    excfirstcore = sqlite3_column_int(prepStmt, 2);
    exclastcore = sqlite3_column_int(prepStmt, 3);
    SetPNodeData( pnid
                , nodename
                , excfirstcore
                , exclastcore
                , pnodeConfig );

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetPNode( const char *name
                         , physical_node_configuration_t &pnodeConfig )
{
    const char method_name[] = "CTcdbZstore::GetPNode";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int  rc;
    int  excfirstcore = -1;
    int  exclastcore = -1;
    int  pnid = -1;

    //
    // List a node's physical configuration by <hostname>
    //

    sqlStmt = "select p.pNid, p.nodeName, p.excFirstCore, p.excLastCore"
                   "  from pnode p where p.nodeName = ?";

    //
    // Get clusterZNodePath_/<hostname> znode
    //      znode=<hostname>, data=(<pnid>)
    // Get pnodesZNodePath_/<pnid> znode
    //      znode=<pnid>, data=(<hostname>:<excFirstCore>:<excLastCore>)
    //      Save matching pnode znode's data
    // Return saved pnode znode's data
    //

    pnid = sqlite3_column_int(prepStmt, 0);
    nodename = (const char *) sqlite3_column_text(prepStmt, 1);
    excfirstcore = sqlite3_column_int(prepStmt, 2);
    exclastcore = sqlite3_column_int(prepStmt, 3);
    SetPNodeData( pnid
                , nodename
                , excfirstcore
                , exclastcore
                , pnodeConfig );

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetSNodes( int &count
                          , int max
                          , physical_node_configuration_t spareNodeConfig[] )
{
    const char method_name[] = "CTcdbZstore::GetSNodes";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    //
    // List all spare nodes, limited by max
    //

    sqlStmt = "select p.pNid, p.nodeName, p.excFirstCore, p.excLastCore,"
              " s.spNid "
              "  from pnode p, snode s where p.pNid = s.pNid";

    //
    // Loop on snodesZNodePath_ znodes <spare-pnid>
    //   Get pnodesZNodePath_/<spare-pnid> znode's data
    //     znode=<pnid>, data=(<hostname>:<excFirstCore>:<excLastCore>)
    //   Save <spare-pnid> pnode znode's data IN spareNodeConfig[snodeCount]
    //   Loop on each snodesZNodePath_/<spare-pnid> children <spared-pnid>
    //      Save <spared-pnid> in spareNodeConfig[snodeCount].spare_pnid[spareCount]
    //

    int  rc;
    int  pnid = -1;
    int  excfirstcore = -1;
    int  exclastcore = -1;
    int  snodeCount = 0;

    // Process spare nodes
    while ( 1 )
    {
            if ( snodeCount < max )
            {
                pnid = sqlite3_column_int(prepStmt, 0);
                nodename = (const char *) sqlite3_column_text(prepStmt, 1);
                excfirstcore = sqlite3_column_int(prepStmt, 2);
                exclastcore = sqlite3_column_int(prepStmt, 3);
                if ( ! GetSNodeData( pnid
                                   , nodename
                                   , excfirstcore
                                   , exclastcore
                                   , spareNodeConfig[snodeCount] ) )
                {
                    char buf[TC_LOG_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s], Error: Invalid node configuration\n"
                            , method_name);
                    TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
                    TRACE_EXIT;
                    return( TCDBOPERROR );
                }
                snodeCount++;
            }
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetSNodeData( int pnid
                             , const char *nodename
                             , int excfirstcore
                             , int exclastcore 
                             , physical_node_configuration_t &spareNodeConfig )
{
    const char method_name[] = "CTcdbZstore::GetSNodeData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int  rc;
    const char   *sqlStmt;
    sqlite3_stmt *prepStmt = NULL;

    if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
    {
        trace_printf( "%s@%d pnid=%d, name=%s, excluded cores=(%d:%d)\n"
                    , method_name, __LINE__
                    , pnid
                    , nodename
                    , excfirstcore
                    , exclastcore );
    }

    spareNodeConfig.pnid = pnid;
    strncpy( spareNodeConfig.node_name
           , nodename
           , sizeof(spareNodeConfig.node_name) );
    spareNodeConfig.excluded_first_core = excfirstcore;
    spareNodeConfig.excluded_last_core = exclastcore;

    // Select all spared nodes configured for this spare node
    sqlStmt = "select p.pNid, s.spNid"
              "  from pnode p, snode s"
              "    where p.pNid = s.pNid and p.pNid = ?";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmt
                           , strlen(sqlStmt)+1
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {   // Set pnid in prepared statement
        rc = sqlite3_bind_int(prepStmt, 1, pnid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(pnid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    int  spnid;
    int  sparedpnid;
    int  spareCount = 0;

    // Process spare nodes
    while ( 1 )
    {
        rc = sqlite3_step( prepStmt );
        if ( rc == SQLITE_ROW )
        {  // Process row
            int colCount = sqlite3_column_count(prepStmt);
            if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d sqlite3_column_count=%d\n",
                             method_name, __LINE__, colCount);
                for (int i=0; i<colCount; ++i)
                {
                    trace_printf("%s@%d column %d is %s\n",
                                 method_name, __LINE__, i,
                                 sqlite3_column_name(prepStmt, i));
                }
            }

            spnid = sqlite3_column_int(prepStmt, 0);
            sparedpnid = sqlite3_column_int(prepStmt, 1);
            spareNodeConfig.spare_pnid[spareCount] = sparedpnid;
            spareCount++;
        }
        else if ( rc == SQLITE_DONE )
        {
            spareNodeConfig.spare_count = spareCount;
            // Destroy prepared statement object
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }

            if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d Finished processing spared node set.\n",
                             method_name, __LINE__);
            }

            break;
        }
        else
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                    , method_name, sqlStmt, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetPersistProcess( const char *persistPrefix
                                  , persist_configuration_t &persistConfig )
{
    const char method_name[] = "CTcdbZstore::GetPersistProcess";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int  rc, rs;
    char param[TC_PERSIST_KEY_MAX];
    const char   *persistKey;
    const char   *persistValue;
    const char   *sqlStmtStmt;
    sqlite3_stmt *prepStmt = NULL;

    if ( TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST) )
    {
        trace_printf( "%s@%d processkey=%s\n"
                    , method_name, __LINE__
                    , persistPrefix );
    }
    
    strncpy( persistConfig.persist_prefix
           , persistPrefix
           , sizeof(persistConfig.persist_prefix) );

    snprintf( param, sizeof(param), "%s_%%", persistPrefix );

    // Prepare select persistent process for the key
    sqlStmtStmt = "select p.keyName, p.valueName"
                     " from monRegPersistData p"
                     "  where p.keyName like ?";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmtStmt
                           , strlen(sqlStmtStmt)+1
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmtStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {   // Set key in prepared statement
        rc = sqlite3_bind_text( prepStmt, 1, param, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(keyName) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    while ( 1 )
    {
        rc = sqlite3_step( prepStmt );
        if ( rc == SQLITE_ROW )
        {  // Process row
            int colCount = sqlite3_column_count(prepStmt);
            if ( TcTraceSettings & (TC_TRACE_PERSIST | TC_TRACE_REQUEST) )
            {
                trace_printf( "%s@%d sqlite3_column_count=%d\n"
                            , method_name, __LINE__, colCount);
                for (int i=0; i<colCount; ++i)
                {
                    trace_printf("%s@%d column %d is %s\n",
                                 method_name, __LINE__, i,
                                 sqlite3_column_name(prepStmt, i));
                }
            }

            persistKey = (const char *) sqlite3_column_text(prepStmt, 0);
            persistValue = (const char *) sqlite3_column_text(prepStmt, 1);

            if ( TcTraceSettings & (TC_TRACE_PERSIST | TC_TRACE_REQUEST) )
            {
                trace_printf( "%s@%d monRegPersistData key=%s, value=%s\n"
                            , method_name, __LINE__, persistKey, persistValue);
            }

            // Parse the value based on the key
            rs = SetPersistProcessData( persistKey
                                       , persistValue
                                       , persistConfig );
            if ( rs != TCSUCCESS )
            {
                char buf[TC_LOG_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], Error: Invalid persist key value in "
                          "configuration, key=%s, value=%s\n"
                        , method_name, persistKey, persistValue );
                TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
                TRACE_EXIT;
                return( TCDBOPERROR );
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            if ( TcTraceSettings & (TC_TRACE_PERSIST | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d Finished processing all rows.\n",
                             method_name, __LINE__);
            }
            break;
        }
        else
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                    , method_name, sqlStmtStmt, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetPersistProcessKeys( const char *persistProcessKeys )
{
    const char method_name[] = "CTcdbZstore::GetPersistProcessKeys";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    

    int  rc;

    const char   *sqlStmt;
    sqlite3_stmt *prepStmt = NULL;
    sqlStmt = "select p.valueName"
              " from monRegPersistData p"
              "  where p.keyName = 'PERSIST_PROCESS_KEYS'";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmt
                           , strlen(sqlStmt)+1
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    // Process persistent process keys
    rc = sqlite3_step( prepStmt );
    if ( rc == SQLITE_ROW )
    {  // Process row
        int colCount = sqlite3_column_count(prepStmt);

        if ( TcTraceSettings & (TC_TRACE_PERSIST | TC_TRACE_REQUEST) )
        {
            trace_printf("%s@%d sqlite3_column_count=%d\n",
                         method_name, __LINE__, colCount);
            for (int i=0; i<colCount; ++i)
            {
                trace_printf("%s@%d column %d is %s\n",
                             method_name, __LINE__, i,
                             sqlite3_column_name(prepStmt, i));
            }
        }

        const unsigned char *value;

        value = sqlite3_column_text(prepStmt, 0);
        strncpy( (char *)persistProcessKeys, (const char *)value, TC_PERSIST_KEYS_VALUE_MAX );
    
    }
    else if ( rc == SQLITE_DONE )
    {
        if ( prepStmt != NULL )
        {
            sqlite3_finalize( prepStmt );
        }
        return( TCDBNOEXIST );
    }
    else
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetRegistryClusterSet( int &count
                                      , int max
                                      , registry_configuration_t registryConfig[] )
{
    const char method_name[] = "CTcdbZstore::GetRegistryClusterSet";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Getting registry cluster set\n"
                    , method_name, __LINE__, znodePath.c_str());
    }

    char keyString[TC_REGISTRY_KEY_MAX];
    char *pch;
    char *token1;
    const char *group = "CLUSTER";
    const char *key;
    static const char *delimBSlash = "/";
    int entryNum = 0;
    struct String_vector regKeys; // Children znode paths

    count = 0;

    //  /TRAFODION_REGISTRY_CLUSTER_ZNODE
    //      /<key-name> (<value>)
    stringstream ss;
    ss.str( "" );
    ss << regClusterZNodePath_;
    string znodePath(ss.str());
    
    int rc = zConfig_.GetConfigZNodeChildren( znodePath, &regKeys );
    if ( rc != ZOK )
    {
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    if ( max == 0 )
    {
        // Just return the count
        entryNum = regKeys.count;
        FreeStringVector( &regKeys );
    }
    else if ( regKeys.count > 0 )
    {
        for (int i = 0; i < regKeys.count ; i++ )
        {
            if ( entryNum < max )
            {
                // Get the value from:
                // /trafodion/instance/registry/cluster/<key> (<value>)
                rc = zConfig_.GetConfigZNodeData( regKeys.data[i]
                                                , registryConfig[entryNum].value )
                if ( rc != ZOK )
                {
                    TRACE_EXIT;
                    return( TCDBOPERROR );
                }

                strncpy( keyString, (const char *)regKeys.data[i], sizeof(keyString) );
                pch = (char *) strstr( keyString, TRAFODION_REGISTRY_CLUSTER_ZNODE );
                if (pch != NULL)
                {
                    // Set key name
                    token1 = strtok( ++pch, delimBSlash );
                    if (token1)
                    {
                        key = token1;
                    }
                }

                if ( TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST) )
                {
                    trace_printf( "%s@%d entry %d: group=%s key=%s, value=%s\n"
                                , method_name, __LINE__
                                , entryNum, group, key, value);
                }

                strncpy( registryConfig[entryNum].scope, (const char *)group, TC_REGISTRY_KEY_MAX );
                strncpy( registryConfig[entryNum].key, (const char *)key, TC_REGISTRY_KEY_MAX );
                ++entryNum;
            }
            else
            {
                count = entryNum;
                return( TCDBTRUNCATE );
            }
        }

        FreeStringVector( &regKeys );
    }

    count = entryNum;
        
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetRegistryProcessSet( int &count
                                      , int max
                                      , registry_configuration_t registryConfig[] )
{
    const char method_name[] = "CTcdbZstore::GetRegistryProcessSet";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Getting registry process set\n"
                    , method_name, __LINE__, znodePath.c_str());
    }

    char processString[TC_REGISTRY_KEY_MAX];
    char keyString[TC_REGISTRY_KEY_MAX];
    char *pch;
    char *token1;
    const char *group;
    const char *key;
    static const char *delimBSlash = "/";
    int entryNum = 0;
    struct String_vector processNames; // Children znode paths
    struct String_vector regKeys; // Children znode paths

    count = 0;

    //      TRAFODION_REGISTRY_PROCESS_ZNODE
    //          /<process-name>
    //              /<key-name> (<value>)
    stringstream ss;
    ss.str( "" );
    ss << regClusterZNodePath_;
    string znodePath(ss.str());
    
    // /trafodion/instance/registry/process/<process-names>
    int rc = zConfig_.GetConfigZNodeChildren( znodePath, &processNames );
    if ( rc != ZOK )
    {
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    if ( processNames.count > 0 )
    {
        strncpy( processString, (const char *)processNames.data[i], sizeof(processString) );
        pch = (char *) strstr( processString, TRAFODION_REGISTRY_PROCESS_ZNODE );
        if (pch != NULL)
        {
            // Set process name group
            token1 = strtok( ++pch, delimBSlash );
            if (token1)
            {
                group = token1;
            }
        }

        // Get the keys for each process name
        for (int i = 0; i < processNames.count ; i++ )
        {
            // /trafodion/instance/registry/process/<process-name>/<keys>
            int rc = zConfig_.GetConfigZNodeChildren( processNames.data[i], &regKeys );
            if ( rc != ZOK )
            {
                TRACE_EXIT;
                return( TCDBOPERROR );
            }

            if ( max == 0 )
            {
                // Just return the count
                entryNum += regKeys.count;
                FreeStringVector( &regKeys );
            }
            else if ( regKeys.count > 0 )
            {
                for (int j = 0; i < regKeys.count ; j++ )
                {
                    if ( entryNum < max )
                    {
                        // Get the value from:
                        // /trafodion/instance/registry/process/<process-name>/<key> (<value>)
                        rc = zConfig_.GetConfigZNodeData( regKeys.data[j]
                                                        , registryConfig[entryNum].value )
                        if ( rc != ZOK )
                        {
                            TRACE_EXIT;
                            return( TCDBOPERROR );
                        }
        
                        strncpy( keyString, (const char *)regKeys.data[i], sizeof(keyString) );
                        pch = (char *) strstr( keyString, TRAFODION_REGISTRY_CLUSTER_ZNODE );
                        if (pch != NULL)
                        {
                            // Set key name
                            token1 = strtok( ++pch, delimBSlash );
                            if (token1)
                            {
                                key = token1;
                            }
                        }
        
                        if ( TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST) )
                        {
                            trace_printf( "%s@%d entry %d: group=%s key=%s, value=%s\n"
                                        , method_name, __LINE__
                                        , entryNum, group, key, value);
                        }
        
                        strncpy( registryConfig[entryNum].scope, (const char *)group, TC_REGISTRY_KEY_MAX );
                        strncpy( registryConfig[entryNum].key, (const char *)key, TC_REGISTRY_KEY_MAX );
                        ++entryNum;
                    }
                    else
                    {
                        count = entryNum;
                        return( TCDBTRUNCATE );
                    }
                }
        
                FreeStringVector( &regKeys );
            }
        }

        FreeStringVector( &processNames );
    }

    count = entryNum;

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetUniqueString( int nid, int id, const char *uniqStr )
{
    const char method_name[] = "CTcdbZstore::GetUniqueString";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    stringstream ss;
    ss.str( "" );

#if 1
    //  /TRAFODION_REGISTRY_USTRING_ZNODE
    //      /<nid>
    //          /<id> (<uniqueStr>)
    ss << regUStringZNodePath_ << "/"
       << nid  << "/"
       << id;
    string znodePath(ss.str());
    
    // Get the value from:
    // /trafodion/instance/registry/ustring/<nid>/<id> (<value>)
#else
    //  /TRAFODION_REGISTRY_USTRING_ZNODE
    //      /<id> (<uniqueStr>)
    ss << regUStringZNodePath_ << "/"
       << id;
    string znodePath(ss.str());
    
    // Get the value from:
    // /trafodion/instance/registry/ustring/<id> (<value>)
#endif

    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Getting unique string: znodePath=%d\n"
                    , method_name, __LINE__, znodePath.c_str());
    }

    int rc = zConfig_.GetConfigZNodeData( znodePath.c_str(), uniqStr )
    if ( rc != ZOK )
    {
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetUniqueStringId( int nid
                                  , const char *uniqStr
                                  , int &id )
{
    const char method_name[] = "CTcdbZstore::GetUniqueStringId";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Getting unique string id: nid=%d string=%s\n"
                    , method_name, __LINE__, nid, uniqStr);
    }

    char uStringValue[TC_UNIQUE_STRING_VALUE_MAX];
    char keyString[TC_REGISTRY_KEY_MAX];
    char *pch;
    char *token1;
    const char *key;
    int diff;
    static const char *delimBSlash = "/";
    struct String_vector regKeys; // Children znode paths

    stringstream ss;
    ss.str( "" );

#if 1
    //  /TRAFODION_REGISTRY_USTRING_ZNODE
    //      /<nid>
    //          /<id> (<uniqueStr>)
    ss << regUStringZNodePath_ << "/"
       << nid  << "/"
       << id;
    string znodePath(ss.str());
    
    // Get the value from:
    // /trafodion/instance/registry/ustring/<nid>/<id> (<uniqueStr>)
#else
    //  /TRAFODION_REGISTRY_USTRING_ZNODE
    //      /<id> (<uniqueStr>)
    ss << regUStringZNodePath_ << "/"
       << id;
    string znodePath(ss.str());
    
    // Get the value from:
    // /trafodion/instance/registry/ustring/<id> (<uniqueStr>)
#endif

    int rc = zConfig_.GetConfigZNodeChildren( znodePath, &regKeys );
    if ( rc != ZOK )
    {
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    if ( regKeys.count > 0 )
    {
        for (int i = 0; i < regKeys.count ; i++ )
        {
            // Get the value from:
            // /trafodion/instance/registry/ustring/<nid>/<id> (<uniqueStr>)
            //          or
            // /trafodion/instance/registry/ustring/<id> (<uniqueStr>)
            rc = zConfig_.GetConfigZNodeData( regKeys.data[i], uStringValue )
            if ( rc != ZOK )
            {
                TRACE_EXIT;
                return( TCDBOPERROR );
            }

            if ( TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST) )
            {
                trace_printf( "%s@%d znodePath=%s\nuniqStr=%s\nvalue=%s\n"
                            , method_name, __LINE__
                            , regKeys.data[i], uniqStr, uStringValue);
            }

            diff = strncpy( (const char *)uniqStr
                          , (const char *)uStringValue
                          , TC_UNIQUE_STRING_VALUE_MAX );
            if ( diff == 0 )
            {
                // /trafodion/instance/registry/ustring/[<nid>/]<id> (<uniqueStr>)
                strncpy( keyString, (const char *)regKeys.data[i], sizeof(keyString) );
                // /ustring[/<nid>]/<id> (<uniqueStr>)
                pch = (char *) strstr( keyString, TRAFODION_REGISTRY_USTRING_ZNODE );
                if (pch != NULL)
                {
#if 1
                    // /ustring/<nid>/<id> (<uniqueStr>)
                    pch = strtok( ++pch, delimBSlash );
                    if (pch != NULL)
                    {
                        // /<nid>/<id> (<uniqueStr>)
                        // Set the id
                        token1 = strtok( ++pch, delimBSlash );
                        if (token1)
                        {
                            key = token1;
                        }
                    }
#else
                    // /ustring/<id> (<uniqueStr>)
                    // Set the id
                    token1 = strtok( ++pch, delimBSlash );
                    if (token1)
                    {
                        key = token1;
                    }
#endif
                }
            }
        }

        FreeStringVector( &regKeys );
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::GetUniqueStringIdMax( int nid, int &id )
{
    const char method_name[] = "CTcdbZstore::GetUniqueStringIdMax";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int result = 0;
    int rc;
    const char *sqlStmt;
    sqlStmt = "select max(id) from monRegUniqueStrings where nid=?";

    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, strlen(sqlStmt)+1, &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_int( prepStmt, 1, nid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(nid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );

        if ( rc == SQLITE_ROW )
        {
            result = sqlite3_column_int(prepStmt, 0);
            id = result;
            if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
            {
                trace_printf("%s@%d found max(id)=%d for nid=%d in "
                             "monRegUniqueStrings\n", method_name, __LINE__,
                             result, nid);
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            return( TCDBNOEXIST );
        }
        else
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, nid=%d, error: %s\n"
                    , method_name, sqlStmt, nid, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }
        
    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::Initialize( void )
{
    const char method_name[] = "CTcdbZstore::Initialize";
    TRACE_ENTRY;

    if ( IsInitialized() )
    {
        return( TCALREADYINIT );
    }

    TRACE_EXIT;
    return( zConfig_.InitializeZConfig() );
}

bool CTcdbZstore::IsInitialized( void )
{
    return ( (ZHandle != 0) );
}

int CTcdbZstore::SaveLNodeData( int         nid
                              , int         pnid
                              , int         firstCore
                              , int         lastCore
                              , int         processors
                              , int         roles )
{
    const char method_name[] = "CTcdbZstore::SaveLNodeData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    stringstream ss;
    ss.str( "" );
    ss << lnodesZNodePath_ << "/"
       << nid;
    string znodePath(ss.str());
    ss.str( "" );
    ss << pnid << ":"
       << processors << ":"
       << roles << ":"
       << firstCore << ":"
       << lastCore;
    string znodeData(ss.str());
    
    if (TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Adding logical node znode: %s\n" 
                      "            (pnid=%d, processors=%d, roles=%d, firstCore=%d, lastCore=%d )\n"
                      "            (%s)\n"
                     , method_name, __LINE__
                     , znodePath.c_str()
                     , pnid
                     , processors
                     , roles
                     , firstCore
                     , lastCore
                     , znodeData.c_str() );
    }

    int rc = zConfig_.CreateConfigZNode( znodePath.c_str(), znodeData.c_str(), 0 );
    if ( rc != ZOK )
    {
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::SavePNodeData( const char *name
                              , int         pnid
                              , int         excludedFirstCore
                              , int         excludedLastCore )
{
    const char method_name[] = "CTcdbZstore::SavePNodeData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    stringstream ss;

    //  /TRAFODION_CLUSTER_ZNODE          [physical node]
    ss.str( "" );
    ss << clusterZNodePath_ << "/"
       << name;
    string znodePath(ss.str());

    //      /<hostname> (<pnid>)
    ss.str( "" );
    ss << pnid;
    string znodeData(ss.str());
    
    if (TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Adding cluster node znode: %s\n" 
                      "            (pNid=%d)\n"
                      "            (%s)\n"
                     , method_name, __LINE__
                     , znodePath.c_str()
                     , pnid
                     , znodeData.c_str() );
    }


    int rc = zConfig_.CreateConfigZNode( znodePath.c_str(), znodeData.c_str(), 0 );
    if ( rc != ZOK )
    {
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    //  /TRAFODION_PNODES_ZNODE         [physical node]
    ss.str( "" );
    ss << pnodesZNodePath_ << "/"
       << pnid;
    znodePath = ss.str();

    //      /<pnid> (<hostname>:<excFirstCore>:<excLastCore>)
    ss.str( "" );
    ss << name << ":"
       << excludedFirstCore << ":"
       << excludedLastCore;
    znodeData = ss.str();
    
    if (TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Adding physical node znode: %s\n" 
                      "            (hostname=%s, excFirstCore=%d, excLastCore=%d)\n"
                      "            (%s)\n"
                     , method_name, __LINE__
                     , znodePath.c_str()
                     , name
                     , excludedFirstCore
                     , excludedLastCore
                     , znodeData.c_str() );
    }


    int rc = zConfig_.CreateConfigZNode( znodePath.c_str(), znodeData.c_str(), 0 );
    if ( rc != ZOK )
    {
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

void CTcdbZstore::SetLNodeData( int nid
                              , int pnid
                              , const char *nodename
                              , int excfirstcore
                              , int exclastcore
                              , int firstcore
                              , int lastcore
                              , int processors
                              , int roles 
                              , node_configuration_t &nodeConfig )
{
    const char method_name[] = "CTcdbZstore::SetLNodeData";
    TRACE_ENTRY;

    if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
    {
        trace_printf( "%s@%d nid=%d, pnid=%d, name=%s, excluded cores=(%d:%d),"
                      " cores=(%d:%d), processors=%d, roles=%d\n"
                    , method_name, __LINE__
                    , nid
                    , pnid
                    , nodename
                    , excfirstcore
                    , exclastcore
                    , firstcore
                    , lastcore
                    , processors
                    , roles );
    }

    nodeConfig.nid  = nid;
    nodeConfig.pnid = pnid;
    strncpy( nodeConfig.node_name
           , nodename
           , sizeof(nodeConfig.node_name) );
    nodeConfig.excluded_first_core = excfirstcore;
    nodeConfig.excluded_last_core = exclastcore;
    nodeConfig.first_core = firstcore;
    nodeConfig.last_core = lastcore;
    nodeConfig.processors = processors;
    nodeConfig.roles  = roles;

    TRACE_EXIT;
}

void CTcdbZstore::SetPNodeData( int pnid
                              , const char *nodename
                              , int excfirstcore
                              , int exclastcore
                              , physical_node_configuration_t &pnodeConfig )
                                 
{
    const char method_name[] = "CTcdbZstore::SetLNodeData";
    TRACE_ENTRY;

    if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
    {
        trace_printf( "%s@%d pnid=%d, name=%s, excluded cores=(%d:%d)\n"
                    , method_name, __LINE__
                    , pnid
                    , nodename
                    , excfirstcore
                    , exclastcore );
    }

    pnodeConfig.pnid = pnid;
    strncpy( pnodeConfig.node_name
           , nodename
           , sizeof(pnodeConfig.node_name) );
    pnodeConfig.excluded_first_core = excfirstcore;
    pnodeConfig.excluded_last_core = exclastcore;

    TRACE_EXIT;
}

int CTcdbZstore::SetPersistProcessData( const char       *persistkey
                                      , const char       *persistvalue
                                      , persist_configuration_t &persistConfig )
{
    const char method_name[] = "CTcdbZstore::GetPersistProcessData";
    TRACE_ENTRY;

    char workValue[TC_PERSIST_KEY_MAX];
    char *pch;
    char *token1;
    char *token2;
    static const char *delimNone = "\0";
    static const char *delimComma = ",";

    if ( TcTraceSettings & (TC_TRACE_PERSIST | TC_TRACE_REQUEST) )
    {
        trace_printf( "%s@%d persistKey=%s, persistValue=%s\n"
                    , method_name, __LINE__
                    , persistkey, persistvalue );
    }
    
    strncpy( workValue, persistvalue, sizeof(workValue) );

    pch = (char *) strstr( persistkey, PERSIST_PROCESS_NAME_KEY );
    if (pch != NULL)
    {
        strncpy( persistConfig.process_name
               , workValue
               , sizeof(persistConfig.process_name) );
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_PROCESS_TYPE_KEY );
    if (pch != NULL)
    {
        strncpy( persistConfig.process_type
               , workValue
               , sizeof(persistConfig.process_type) );
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_PROGRAM_NAME_KEY );
    if (pch != NULL)
    {
        strncpy( persistConfig.program_name
               , workValue
               , sizeof(persistConfig.program_name) );
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_PROGRAM_ARGS_KEY );
    if (pch != NULL)
    {
        strncpy( persistConfig.program_args
               , workValue
               , sizeof(persistConfig.program_args) );
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_STDOUT_KEY );
    if (pch != NULL)
    {
        strncpy( persistConfig.std_out
               , workValue
               , sizeof(persistConfig.std_out) );
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_REQUIRES_DTM );
    if (pch != NULL)
    {
        persistConfig.requires_DTM = (strcasecmp(workValue,"Y") == 0) 
                                    ? true : false;
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_RETRIES_KEY );
    if (pch != NULL)
    {
        // Set retries
        token1 = strtok( workValue, delimComma );
        if (token1)
        {
            persistConfig.persist_retries = atoi(token1);
        }
        // Set time window
        token2 = strtok( NULL, delimNone );
        if (token2)
        {
            persistConfig.persist_window = atoi(token2);
        }
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_ZONES_KEY );
    if (pch != NULL)
    {
        strncpy( persistConfig.persist_zones
               , workValue
               , sizeof(persistConfig.persist_zones) );
        goto done;
    }
    else
    {
        TRACE_EXIT;
        return( TCDBCORRUPT );
    }

done:

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbZstore::UpdatePNodeData( int         pnid
                                , const char *name
                                , int         excludedFirstCore
                                , int         excludedLastCore )
{
    const char method_name[] = "CTcdbZstore::UpdatePNodeData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Zookeeper is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    if (TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d update pnode "
                      "nodeName=%s, excFirstCore=%d, excLastCore=%d\n"
                      "where pnid=%d)\n"
                     , method_name, __LINE__
                     , name
                     , excludedFirstCore
                     , excludedLastCore
                     , pnid );
    }

    int rc;
    const char *sqlStmt;
    sqlStmt = "update or replace pnode "
              " set nodeName = ?, excFirstCore = ?, excLastCore = ?"
              "   where pnode.pNid = :pNid";

    sqlite3_stmt *prepStmt = NULL;
    rc = sqlite3_prepare_v2( db_, sqlStmt, strlen(sqlStmt)+1, &prepStmt, NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_text( prepStmt, 
                                1, 
                                name, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(name) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt, 
                               2,
                               excludedFirstCore );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(excludedFirstCore) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt,
                               3, 
                               excludedLastCore );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(excludedLastCore) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt,
                               sqlite3_bind_parameter_index( prepStmt, ":pNid" ),
                               pnid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(:pNid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                      " (nodeName=%s, excFirstCore=%d, excLastCore=%d) "
                      " where pNid=%d " 
                    , method_name, sqlStmt, sqlite3_errmsg(db_)
                    , name, excludedFirstCore, excludedLastCore, pnid );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

