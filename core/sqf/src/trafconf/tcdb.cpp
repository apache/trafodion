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

#include <stdlib.h>
#include <string.h>
#include <cctype>
#include <string>

using namespace std;

#include "tclog.h"
#include "tctrace.h"
//#include "tcdbmysql.h"
#include "tcdbsqlite.h"
#include "tcdb.h"

///////////////////////////////////////////////////////////////////////////////
//  Cluster Configuration
///////////////////////////////////////////////////////////////////////////////

CTcdb::CTcdb( void )
     : dbStorageType_(TCDBSTOREUNDEFINED)
     , tcdbStore_(NULL)
{
    const char method_name[] = "CTcdb::CTcdb";
    TRACE_ENTRY;

    memcpy(&eyecatcher_, "TCDB", 4);

    char   *env;
    string  tcDbType;
    size_t  found;

    env = getenv("TRAF_CONFIG_DBSTORE");
    if ( env )
    {
        char c;
        int i = 0;
        while ( env[i] )
        {
            c=env[i];
            env[i] = (char) toupper( c );
            i++;
        }
        tcDbType = env;
        if ( tcDbType.length() == 0 )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], Environment variable TRAF_CONFIG_DBSTORE value is not set, "
                      "defaulting to SQLite storage method!\n"
                    , method_name );
            TcLogWrite( TCDB_TCDB_1, TC_LOG_WARNING, buf );
            dbStorageType_ = TCDBSQLITE;
        }
        else
        {
            found = tcDbType.find( TC_STORE_SQLITE );
            if (found != std::string::npos)
            {
                dbStorageType_ = TCDBSQLITE;
            }
            else
            {
                found = tcDbType.find( TC_STORE_MYSQL );
                if (found != std::string::npos)
                {
                    dbStorageType_ = TCDBMYSQL;
                }
                else
                {
                    found = tcDbType.find( TC_STORE_POSTGRESQL );
                    if (found != std::string::npos)
                    {
                        dbStorageType_ = TCDBPOSTGRESQL;
                    }
                    else
                    {
                        if ( tcDbType.length() == 0 )
                        {
                            char buf[TC_LOG_BUF_SIZE];
                            snprintf( buf, sizeof(buf)
                                    , "[%s], Environment variable TRAF_CONFIG_DBSTORE value (%s) invalid!\n"
                                    , method_name, tcDbType.c_str() );
                            TcLogWrite( TCDB_TCDB_2, TC_LOG_CRIT, buf );
                            TRACE_EXIT;
                        }
                    }
                }
            }
        }
    }
    else
    {
        // Environment variable TRAF_CONFIG_DBSTORE is undefined
        // defaulting to SQLite storage method!
        dbStorageType_ = TCDBSQLITE;
    }
    
    TRACE_EXIT;
}

CTcdb::~CTcdb ( void )
{
    const char method_name[] = "CTcdb::~CTcdb";
    TRACE_ENTRY;
    memcpy(&eyecatcher_, "tcdb", 4);
    TRACE_EXIT;
}

int CTcdb::AddLNodeData( int         nid
                       , int         pnid
                       , int         firstCore
                       , int         lastCore
                       , int         processors
                       , int         roles )
{
    const char method_name[] = "CTcdb::AddLNodeData";
    TRACE_ENTRY;

    int rc = tcdbStore_->AddLNodeData( nid
                                      , pnid
                                      , firstCore
                                      , lastCore
                                      , processors
                                      , roles );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::AddNameServer( const char *nodeName )
{
    const char method_name[] = "CTcdb::AddNameServer";
    TRACE_ENTRY;

    int rc = tcdbStore_->AddNameServer( nodeName );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::AddPNodeData( const char *name
                        , int         pnid
                        , int         excludedFirstCore
                        , int         excludedLastCore )
{
    const char method_name[] = "CTcdb::AddPNodeData";
    TRACE_ENTRY;


    int rc = tcdbStore_->AddPNodeData( name
                                     , pnid
                                     , excludedFirstCore
                                     , excludedLastCore );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::AddRegistryKey( const char *key )
{
    const char method_name[] = "CTcdb::AddRegistryKey";
    TRACE_ENTRY;

    int rc = tcdbStore_->AddRegistryKey( key );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::AddRegistryProcess( const char *processName )
{
    const char method_name[] = "CTcdb::AddRegistryProcess";
    TRACE_ENTRY;

    int rc = tcdbStore_->AddRegistryProcess( processName );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::AddRegistryClusterData( const char *key
                                 , const char *dataValue )
{
    const char method_name[] = "CTcdb::AddRegistryClusterData";
    TRACE_ENTRY;

    int rc = tcdbStore_->AddRegistryClusterData( key, dataValue );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::AddRegistryProcessData( const char *processName
                                 , const char *key
                                 , const char *dataValue )
{
    const char method_name[] = "CTcdb::AddRegistryProcessData";
    TRACE_ENTRY;

    int rc = tcdbStore_->AddRegistryProcessData( processName, key, dataValue );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::AddUniqueString( int nid
                          , int id
                          , const char *uniqStr )
{
    const char method_name[] = "CTcdb::AddUniqueString";
    TRACE_ENTRY;

    int rc = tcdbStore_->AddUniqueString( nid, id, uniqStr );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::Close( void )
{
    const char method_name[] = "CTcdb::Close";
    TRACE_ENTRY;

    int rc = tcdbStore_->Close();

    TRACE_EXIT;
    return( rc );
}

int CTcdb::DeleteNameServer( const char *nodeName )
{
    const char method_name[] = "CTcdb::DeleteNameServer";
    TRACE_ENTRY;

    int rc = tcdbStore_->DeleteNameServer( nodeName );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::DeleteNodeData( int pnid )
{
    const char method_name[] = "CTcdb::DeleteNodeData";
    TRACE_ENTRY;

    int rc = tcdbStore_->DeleteNodeData( pnid );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::DeleteUniqueString( int nid )
{
    const char method_name[] = "CTcdb::DeleteUniqueString";
    TRACE_ENTRY;

    int rc = tcdbStore_->DeleteUniqueString( nid );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::Initialize( const char *rootNode
                     , const char *instanceNode )
{
    const char method_name[] = "CTcdb::Initialize";
    TRACE_ENTRY;

    if ( IsInitialized() )
    {
        return( TCALREADYINIT );
    }

    if (!tcdbStore_)
    {
        switch (dbStorageType_)
        {
            case TCDBMYSQL:
                rootNode     = rootNode;
                instanceNode = instanceNode;
//                tcdbStore_ = new CTcdbMySql( rootNode
//                                            , instanceNode );
                return( TCNOTIMPLEMENTED );
                break;
            case TCDBSQLITE:
                tcdbStore_ = new CTcdbSqlite;
                break;
            default:
                TRACE_EXIT;
                return( TCNOTIMPLEMENTED );
        }
    }

    int rc = tcdbStore_->Initialize();
    
    TRACE_EXIT;
    return( rc );
}

bool CTcdb::IsInitialized( void )
{
    const char method_name[] = "CTcdb::IsInitialized";
    TRACE_ENTRY;

    bool rs = false;
    if ( tcdbStore_ )
    {
        rs = tcdbStore_->IsInitialized();
    }

    TRACE_EXIT;
    return( rs );
}

int CTcdb::GetNameServer( const char* nodeName )
{
    const char method_name[] = "CTcdb::GetNameServer";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetNameServer( nodeName );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetNameServers( int *count, int max, char *nodeNames[] )
{
    const char method_name[] = "CTcdb::GetNameServers";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetNameServers( count, max, nodeNames );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetNode( int nid
                  , TcNodeConfiguration_t &nodeConfig )
{
    const char method_name[] = "CTcdb::GetNode";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetNode( nid, nodeConfig );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetNode( const char *name
                  , TcNodeConfiguration_t &nodeConfig )
{
    const char method_name[] = "CTcdb::GetNode";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetNode( name, nodeConfig );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetNodes( int &count
                   , int max
                   , TcNodeConfiguration_t nodeConfig[] )
{
    const char method_name[] = "CTcdb::GetNodes";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetNodes( count, max, nodeConfig );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetPNode( int pNid
                   , TcPhysicalNodeConfiguration_t &pnodeConfig )
{
    const char method_name[] = "CTcdb::GetPNode";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetPNode( pNid, pnodeConfig );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetPNode( const char *name
                   , TcPhysicalNodeConfiguration_t &pnodeConfig )
{
    const char method_name[] = "CTcdb::GetPNode";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetPNode( name, pnodeConfig );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetSNodes( int &count
                    , int max
                    , TcPhysicalNodeConfiguration_t spareNodeConfig[] )
{
    const char method_name[] = "CTcdb::GetSNodes";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetSNodes( count, max, spareNodeConfig );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetPersistProcess( const char *persistPrefix
                            , TcPersistConfiguration_t &persistConfig )
{
    const char method_name[] = "CTcdb::GetPersistProcess";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetPersistProcess( persistPrefix, persistConfig );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetPersistProcessKeys( const char *persistProcessKeys )
{
    const char method_name[] = "CTcdb::GetPersistProcessKeys";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetPersistProcessKeys( persistProcessKeys );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetRegistryClusterSet( int &count
                                , int max
                                , TcRegistryConfiguration_t registryConfig[] )
{
    const char method_name[] = "CTcdb::GetRegistryClusterSet";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetRegistryClusterSet( count, max, registryConfig );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetRegistryProcessSet( int &count
                                , int max
                                , TcRegistryConfiguration_t registryConfig[] )
{
    const char method_name[] = "CTcdb::GetRegistryProcessSet";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetRegistryProcessSet( count, max, registryConfig );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetUniqueString( int nid, int id, const char *uniqStr )
{
    const char method_name[] = "CTcdb::GetUniqueString";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetUniqueString( nid, id, uniqStr );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetUniqueStringId( int nid
                            , const char *uniqStr
                            , int &id )
{
    const char method_name[] = "CTcdb::GetUniqueStringId";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetUniqueStringId( nid, uniqStr, id );

    TRACE_EXIT;
    return( rc );
}

int CTcdb::GetUniqueStringIdMax( int nid, int &id )
{
    const char method_name[] = "CTcdb::GetUniqueStringIdMax";
    TRACE_ENTRY;

    int rc = tcdbStore_->GetUniqueStringIdMax( nid, id );

    TRACE_EXIT;
    return( rc );
}

