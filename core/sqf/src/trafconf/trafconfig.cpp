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

#include <string.h>
#include "tcdb.h"
#include "tctrace.h"
#include "trafconf/trafconfig.h"

bool TcTraceEnabled = false;
bool TcIsRealCluster = true;

CTrafConfigTrace    TrafConfigTrace;
CTcdb               TrafConfigDb;

///////////////////////////////////////////////////////////////////////////////
//  Trafodion Configuration
///////////////////////////////////////////////////////////////////////////////

TC_Export int tc_close( void )
{
    int rc = TrafConfigDb.Close();

    if (TcTraceEnabled)
    {
        TrafConfigTrace.TraceClose();
    }

    return( rc );
}

TC_Export const char *tc_errmsg( int err )
{
    switch (err)
    {
        case TCSUCCESS:
            return "Successful operation";
        case TCNOTIMPLEMENTED:
            return "Not implemented";
        case TCNOTINIT:
            return "Database not open";
        case TCALREADYINIT:
            return "Database already opened";
        case TCDBOPERROR:
            return "Database operation failed";
        case TCDBNOEXIST:
            return "Database operation yielded non-existent data";
        case TCDBTRUNCATE:
            return "Database operation returned less data than available";
        case TCDBCORRUPT:
            return "Internal processing error or database corruption";
        default:
            break;
    }
    return "Error undefined!";
}

TC_Export int tc_initialize( bool traceEnabled
                           , const char *traceFileName
                           , const char *instanceNode
                           , const char *rootNode )
{
    int rc;

    if ( TrafConfigDb.IsInitialized() )
    {
        return( TCALREADYINIT );
    }

    if ( getenv( "SQ_VIRTUAL_NODES" ) )
    {
        TcIsRealCluster = false;
    }

    TcTraceEnabled = traceEnabled;
    if (TcTraceEnabled)
    {
        TrafConfigTrace.TraceInit( TcTraceEnabled, "0", traceFileName );
    }

    switch (TrafConfigDb.GetStorageType())
    {
        case TCDBMYSQL:
            if (!instanceNode)
            {
                instanceNode = TC_INSTANCE_NODE;
            }
            if (!rootNode)
            {
                rootNode = TC_ROOT_NODE;
            }
            rc = TrafConfigDb.Initialize( rootNode
                                        , instanceNode );
            break;
        case TCDBSQLITE:
            rc = TrafConfigDb.Initialize();
            break;
        default:
            rc = TCNOTIMPLEMENTED;
    }

    return( rc );
}

TC_Export int tc_delete_nameserver( const char *node_name )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.DeleteNameServer( node_name );

    return( rc );
}

TC_Export int tc_get_nameserver( const char *node_name )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.GetNameServer( node_name );

    return( rc );
}

TC_Export int tc_get_nameservers( int   *count
                                , int    max
                                , char **nodeNames )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.GetNameServers( count, max, nodeNames );

    return( rc );
}

TC_Export int tc_put_nameserver( const char* node_name )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TrafConfigDb.AddNameServer( node_name );

    return( rc );
}

TC_Export int tc_delete_node( int nid
                            , const char *node_name )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;
    TcNodeConfiguration_t nodeConfig;

    if (node_name)
    {
        rc = TrafConfigDb.GetNode( node_name, nodeConfig );
        if ( rc != TCSUCCESS)
        {
            return( rc );
        }
    }
    else
    {
        rc = TrafConfigDb.GetNode( nid, nodeConfig );
        if ( rc != TCSUCCESS)
        {
            return( rc );
        }
    }

    if ( nodeConfig.nid != -1)
    {
        rc = TrafConfigDb.DeleteNodeData( nodeConfig.pnid );
    }

    return( rc );
}

TC_Export int tc_get_node( const char *node_name
                         , TcNodeConfiguration_t *node_config )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.GetNode( node_name, *node_config );

    return( rc );
}

TC_Export int tc_put_node( TcNodeConfiguration_t *node_config )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;
    char fqdn_name[TC_PROCESSOR_NAME_MAX];

    if (strlen(node_config->domain_name))
    {
        snprintf( fqdn_name, sizeof(fqdn_name), "%s.%s"
                , node_config->node_name
                , node_config->domain_name );
    }
    else
    {
        strncpy( fqdn_name, node_config->node_name, sizeof(fqdn_name) );
    }

    rc = TrafConfigDb.AddPNodeData( fqdn_name
                                  , node_config->pnid
                                  , node_config->excluded_first_core
                                  , node_config->excluded_last_core );
    if (rc == TCSUCCESS)
    {
        rc = TrafConfigDb.AddLNodeData( node_config->nid
                                      , node_config->pnid
                                      , node_config->first_core
                                      , node_config->last_core
                                      , node_config->processors
                                      , node_config->roles );
    }

    return( rc );
}

TC_Export int tc_get_pnode( const char *node_name
                          , TcPhysicalNodeConfiguration_t *pnode_config )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.GetPNode( node_name, *pnode_config );

    return( rc );
}

TC_Export int tc_put_pnode( TcPhysicalNodeConfiguration_t *pnode_config )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.AddPNodeData( pnode_config->node_name
                                  , pnode_config->pnid
                                  , pnode_config->excluded_first_core
                                  , pnode_config->excluded_last_core );

    return( rc );
}

TC_Export int tc_get_nodes( int *count
                          , int  max
                          , TcNodeConfiguration_t *node_config )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    if ( node_config == NULL )
    {
        max = 0;
    }

    rc = TrafConfigDb.GetNodes( *count, max, node_config );

    return( rc );
}

TC_Export int tc_get_snodes( int *scount
                           , int  max
                           , TcPhysicalNodeConfiguration_t *pnode_config )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    if ( pnode_config == NULL )
    {
        max = 0;
    }

    rc = TrafConfigDb.GetSNodes( *scount, max, pnode_config );

    return( rc );
}

TC_Export int tc_delete_persist_keys( void )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCNOTIMPLEMENTED;

    //

    return( rc );
}

TC_Export int tc_get_persist_keys( const char *persist_keys )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.GetPersistProcessKeys( persist_keys );

    return( rc );
}

TC_Export int tc_put_persist_keys( const char *persist_keys )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCNOTIMPLEMENTED;

    persist_keys = persist_keys;
    //

    return( rc );
}

TC_Export int tc_delete_persist_process( const char *persist_key_prefix )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCNOTIMPLEMENTED;

    persist_key_prefix = persist_key_prefix;
    //

    return( rc );
}

TC_Export int tc_get_persist_process( const char *persist_key_prefix
                                    , TcPersistConfiguration_t *persist_config )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.GetPersistProcess( persist_key_prefix, *persist_config );

    return( rc );
}

TC_Export int tc_put_persist_process( const char *persist_key_prefix
                                    , TcPersistConfiguration_t *persist_config )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCNOTIMPLEMENTED;

    persist_key_prefix = persist_key_prefix;
    persist_config = persist_config;
    //

    return( rc );
}

TC_Export int tc_get_registry_cluster_set( int *count
                                         , int  max
                                         , TcRegistryConfiguration_t *registry_config )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.GetRegistryClusterSet( *count, max, registry_config );

    return( rc );
}

TC_Export int tc_get_registry_process_set( int *count
                                         , int  max
                                         , TcRegistryConfiguration_t *registry_config )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.GetRegistryProcessSet( *count, max, registry_config );

    return( rc );
}

TC_Export int tc_get_registry_key( const char *key )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCNOTIMPLEMENTED;

    key          = key;
    //rc = TrafConfigDb.GetRegistryKey( key );

    return( rc );
}

TC_Export int tc_put_registry_key( const char *key )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.AddRegistryKey( key );

    return( rc );
}

TC_Export int tc_get_registry_process( const char *process_name )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCNOTIMPLEMENTED;

    process_name = process_name;
    //rc = TrafConfigDb.GetRegistryProcess( process_name );

    return( rc );
}

TC_Export int tc_put_registry_process( const char *process_name )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.AddRegistryProcess( process_name );

    return( rc );
}

TC_Export int tc_get_registry_cluster_data( const char *key
                                          , const char *data )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCNOTIMPLEMENTED;

    key          = key;
    data         = data;
    //rc = TrafConfigDb.GetRegistryClusterData( key, data );

    return( rc );
}

TC_Export int tc_put_registry_cluster_data( const char *key
                                          , const char *data )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.AddRegistryClusterData( key, data );

    return( rc );
}

TC_Export int tc_get_registry_process_data( const char *process_name
                                          , const char *key
                                          , const char *data )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCNOTIMPLEMENTED;

    process_name = process_name;
    key          = key;
    data         = data;
    //rc = TrafConfigDb.GetRegistryProcessData( process_name, key, data );

    return( rc );
}

TC_Export int tc_put_registry_process_data( const char *process_name
                                          , const char *key
                                          , const char *data )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.AddRegistryProcessData( process_name, key, data );

    return( rc );
}

TC_Export TcStorageType_t tc_get_storage_type( void )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCDBSTOREUNDEFINED );
    }

    return( TrafConfigDb.GetStorageType() );
}

TC_Export int tc_delete_unique_strings( int nid )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.DeleteUniqueString( nid );

    return( rc );
}


TC_Export int tc_get_unique_string( int nid, int id, const char *unique_string )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.GetUniqueString( nid, id, unique_string );

    return( rc );
}

TC_Export int tc_put_unique_string( int nid, int id, const char *unique_string )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.AddUniqueString( nid, id, unique_string );

    return( rc );
}

TC_Export int tc_get_unique_string_id( int nid, const char *unique_string, int *id )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.GetUniqueStringId( nid, unique_string, *id );

    return( rc );
}

TC_Export int tc_get_unique_string_id_max( int nid, int *id )
{
    if ( ! TrafConfigDb.IsInitialized() )
    {
        return( TCNOTINIT );
    }

    int rc = TCDBOPERROR;

    rc = TrafConfigDb.GetUniqueStringIdMax( nid, *id );

    return( rc );
}

