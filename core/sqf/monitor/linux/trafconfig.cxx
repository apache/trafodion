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

#include "sqliteconfig.h"
#include "trafconfigtrace.h"
#include "trafconfig.h"

bool TcTraceEnabled = false;

CTrafConfigTrace    TrafConfigTrace;
CSqliteConfig       SqliteConfig;

///////////////////////////////////////////////////////////////////////////////
//  Trafodion Configuration
///////////////////////////////////////////////////////////////////////////////

TC_Export int tc_close( void )
{
    int rc = SqliteConfig.Close();

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

TC_Export int tc_initialize( bool traceEnabled )
{
    TcTraceEnabled = traceEnabled;
    if (TcTraceEnabled)
    {
        TrafConfigTrace.TraceInit( TcTraceEnabled, "0", NULL );
    }

    int rc = SqliteConfig.Initialize();

    return( rc );
}


TC_Export int tc_delete_node( int nid
                            , const char *node_name )
{
    int rc = TCDBOPERROR;
    node_configuration_t nodeConfig;

    if (node_name)
    {
        rc = SqliteConfig.GetNode( node_name, nodeConfig );
        if ( rc != TCSUCCESS)
        {
            return( rc );
        }
    }
    else
    {
        rc = SqliteConfig.GetNode( nid, nodeConfig );
        if ( rc != TCSUCCESS)
        {
            return( rc );
        }
    }

    if ( nodeConfig.nid != -1)
    {
        rc = SqliteConfig.DeleteNodeData( nodeConfig.pnid );
    }

    return( rc );
}

TC_Export int tc_get_node( const char *node_name
                         , node_configuration_t *node_config )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.GetNode( node_name, *node_config );

    return( rc );
}

TC_Export int tc_put_node( node_configuration_t *node_config )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.SaveLNodeData( node_config->nid
                                   , node_config->pnid
                                   , node_config->first_core
                                   , node_config->last_core
                                   , node_config->processors
                                   , node_config->roles );

    return( rc );
}

TC_Export int tc_get_pnode( const char *node_name
                          , physical_node_configuration_t *pnode_config )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.GetPNode( node_name, *pnode_config );

    return( rc );
}

TC_Export int tc_put_pnode( physical_node_configuration_t *pnode_config )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.SavePNodeData( pnode_config->node_name
                                   , pnode_config->pnid
                                   , pnode_config->excluded_first_core
                                   , pnode_config->excluded_last_core );

    return( rc );
}

TC_Export int tc_get_nodes( int *count
                          , int  max
                          , node_configuration_t *node_config )
{
    int rc = TCDBOPERROR;

    if ( node_config == NULL )
    {
        max = 0;
    }

    rc = SqliteConfig.GetNodes( *count, max, node_config );

    return( rc );
}

TC_Export int tc_get_snodes( int *scount
                           , int  max
                           , physical_node_configuration_t *pnode_config )
{
    int rc = TCDBOPERROR;

    if ( pnode_config == NULL )
    {
        max = 0;
    }

    rc = SqliteConfig.GetSNodes( *scount, max, pnode_config );

    return( rc );
}

TC_Export int tc_delete_persist_keys( void )
{
    int rc = TCNOTIMPLEMENTED;

    //

    return( rc );
}

TC_Export int tc_get_persist_keys( const char *persist_keys )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.GetPersistProcessKeys( persist_keys );

    return( rc );
}

TC_Export int tc_put_persist_keys( const char *persist_keys )
{
    int rc = TCNOTIMPLEMENTED;

    //

    return( rc );
}

TC_Export int tc_delete_persist_process( const char *persist_key_prefix )
{
    int rc = TCNOTIMPLEMENTED;

    //

    return( rc );
}

TC_Export int tc_get_persist_process( const char *persist_key_prefix
                                    , persist_configuration_t *persist_config )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.GetPersistProcess( persist_key_prefix, *persist_config );

    return( rc );
}

TC_Export int tc_put_persist_process( const char *persist_key_prefix
                                    , persist_configuration_t *persist_config )
{
    int rc = TCNOTIMPLEMENTED;

    //

    return( rc );
}

TC_Export int tc_get_registry_cluster_set( int *count
                                         , int  max
                                         , registry_configuration_t *registry_config )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.GetRegistryClusterSet( *count, max, registry_config );

    return( rc );
}

TC_Export int tc_get_registry_process_set( int *count
                                         , int  max
                                         , registry_configuration_t *registry_config )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.GetRegistryProcessSet( *count, max, registry_config );

    return( rc );
}

TC_Export int tc_get_registry_key( const char *key )
{
    int rc = TCNOTIMPLEMENTED;

    //rc = SqliteConfig.GetRegistryKey( key );

    return( rc );
}

TC_Export int tc_put_registry_key( const char *key )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.AddRegistryKey( key );

    return( rc );
}

TC_Export int tc_get_registry_process( const char *process_name )
{
    int rc = TCNOTIMPLEMENTED;

    //rc = SqliteConfig.GetRegistryProcess( process_name );

    return( rc );
}

TC_Export int tc_put_registry_process( const char *process_name )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.AddRegistryProcess( process_name );

    return( rc );
}

TC_Export int tc_get_registry_cluster_data( const char *key
                                          , const char *data )
{
    int rc = TCNOTIMPLEMENTED;

    //rc = SqliteConfig.GetRegistryClusterData( key, data );

    return( rc );
}

TC_Export int tc_put_registry_cluster_data( const char *key
                                          , const char *data )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.AddRegistryClusterData( key, data );

    return( rc );
}

TC_Export int tc_get_registry_process_data( const char *process_name
                                          , const char *key
                                          , const char *data )
{
    int rc = TCNOTIMPLEMENTED;

    //rc = SqliteConfig.GetRegistryProcessData( process_name, key, data );

    return( rc );
}

TC_Export int tc_put_registry_process_data( const char *process_name
                                          , const char *key
                                          , const char *data )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.AddRegistryProcessData( process_name, key, data );

    return( rc );
}

TC_Export int tc_delete_unique_strings( int nid )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.DeleteUniqueString( nid );

    return( rc );
}


TC_Export int tc_get_unique_string( int nid, int id, const char *unique_string )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.GetUniqueString( nid, id, unique_string );

    return( rc );
}

TC_Export int tc_put_unique_string( int nid, int id, const char *unique_string )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.AddUniqueString( nid, id, unique_string );

    return( rc );
}

TC_Export int tc_get_unique_string_id( int nid, const char *unique_string, int *id )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.GetUniqueStringId( nid, unique_string, *id );

    return( rc );
}

TC_Export int tc_get_unique_string_id_max( int nid, int *id )
{
    int rc = TCDBOPERROR;

    rc = SqliteConfig.GetUniqueStringIdMax( nid, *id );

    return( rc );
}

