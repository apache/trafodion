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

#ifndef TCDBSQLITE_H_
#define TCDBSQLITE_H_

#include <stdlib.h>
#include <sqlite3.h>
#include "trafconf/trafconfig.h"
#include "tcdbstore.h"

class CTcdbSqlite : public CTcdbStore
{
public:

    CTcdbSqlite( void );
    ~CTcdbSqlite( void );

    int         AddLNodeData( int nid
                            , int pnid
                            , int firstCore
                            , int lastCore
                            , int processors
                            , int roles );
    int         AddNameServer( const char *nodeName );
    int         AddPNodeData( const char *name
                            , int pnid
                            , int excludedFirstCore
                            , int excludedLastCore );
    int         AddRegistryKey( const char *key );
    int         AddRegistryProcess( const char *name );
    int         AddRegistryClusterData( const char *key, const char *dataValue );
    int         AddRegistryProcessData( const char *procName
                                      , const char *key
                                      , const char *dataValue );
    int         AddUniqueString( int nid, int id, const char *uniqStr );
    int         Close( void );
    int         DeleteNameServer( const char *nodeName );
    int         DeleteNodeData( int pnid );
    int         DeleteUniqueString( int nid );
    int         GetNameServer( const char *nodeName );
    int         GetNameServers( int *count, int max, char **nodeNames );
    int         GetNode( int nid
                       , TcNodeConfiguration_t &nodeConfig );
    int         GetNode( const char *name
                       , TcNodeConfiguration_t &nodeConfig );
    int         GetNodes( int &count
                        , int max
                        , TcNodeConfiguration_t nodeConfig[] );
    int         GetPNode( int pnid
                        , TcPhysicalNodeConfiguration_t &pnodeConfig );
    int         GetPNode( const char *name
                        , TcPhysicalNodeConfiguration_t &pnodeConfig );
    int         GetSNodes( int &count
                         , int max
                         , TcPhysicalNodeConfiguration_t pNodeConfig[] );
    int         GetPersistProcess( const char *persistPrefix
                                 , TcPersistConfiguration_t &persistConfig );
    int         GetPersistProcessKeys( const char *persistProcessKeys );
    int         GetRegistryClusterSet( int &count
                                     , int max
                                     , TcRegistryConfiguration_t registryConfig[] );
    int         GetRegistryProcessSet( int &count
                                     , int max
                                     , TcRegistryConfiguration_t registryConfig[] );
    int         GetUniqueString( int nid, int id, const char *uniqStr );
    int         GetUniqueStringId( int nid
                                 , const char *uniqStr
                                 , int &id );
    int         GetUniqueStringIdMax( int nid, int &id );
    int         Initialize( void );
    inline bool IsInitialized( void ) { return( (db_ != NULL) ); }
    int         UpdatePNodeConfig( int pnid
                                 , const char *name
                                 , int excludedFirstCore
                                 , int excludedLastCore );

protected:
private:
    int  GetSNodeData( int pnid
                     , const char *nodename
                     , int excfirstcore
                     , int exclastcore
                     , TcPhysicalNodeConfiguration_t &spareNodeConfig );
    void SetLNodeData( int nid
                     , int pnid
                     , const char *nodename
                     , int excfirstcore
                     , int exclastcore
                     , int firstcore
                     , int lastcore
                     , int processors
                     , int roles 
                     , TcNodeConfiguration_t &nodeConfig );
    void SetPNodeData( int pnid
                     , const char *nodename
                     , int excfirstcore
                     , int exclastcore
                     , TcPhysicalNodeConfiguration_t &pnodeConfig );
    int  SetPersistProcessData( const char *persistkey
                              , const char *persistvalue
                              , TcPersistConfiguration_t &persistConfig );
    int  UpdatePNodeData( int pnid
                        , const char *name
                        , int excludedFirstCore
                        , int excludedLastCore );

    sqlite3   *db_;
};


#endif /* TCDBSQLITE_H_ */
