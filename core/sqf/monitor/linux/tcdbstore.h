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

#ifndef TCDBSTORE_H_
#define TCDBSTORE_H_

#include "zookeeper/zookeeper.h"
#include "trafconfig.h"

using namespace std;


//
// Trafodion Configuration Database Adaptor (CTcdbStore class)
//
//  Implements common interface to storage classes used by the
//  Trafodion Configuration API (trafconfig.cxx/.h).
//

class CTcdbStore
{
protected:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:

    CTcdbStore( TC_STORAGE_TYPE dbStorageType );
    virtual ~CTcdbStore( void );

    virtual int         Close( void ) = 0;
    virtual int         AddRegistryKey( const char *key ) = 0;
    virtual int         AddRegistryProcess( const char *name ) = 0;
    virtual int         AddRegistryClusterData( const char *key
                                              , const char *dataValue ) = 0;
    virtual int         AddRegistryProcessData( const char *procName
                                              , const char *key
                                              , const char *dataValue ) = 0;
    virtual int         AddUniqueString( int nid
                                       , int id
                                       , const char *uniqStr ) = 0;
    virtual int         DeleteNodeData( int pnid ) = 0;
    virtual int         DeleteUniqueString( int nid ) = 0;
    virtual int         GetNode( int nid
                               , node_configuration_t &nodeConfig ) = 0;
    virtual int         GetNode( const char *name
                               , node_configuration_t &nodeConfig ) = 0;
    virtual int         GetNodes( int &count
                                , int max
                                , node_configuration_t nodeConfig[] ) = 0;
    virtual int         GetPNode( int pnid
                                , physical_node_configuration_t &pnodeConfig ) = 0;
    virtual int         GetPNode( const char *name
                                , physical_node_configuration_t &pnodeConfig ) = 0;
    virtual int         GetSNodes( int &count
                                 , int max
                                 , physical_node_configuration_t pNodeConfig[] ) = 0;
    virtual int         GetPersistProcess( const char *persistPrefix
                                         , persist_configuration_t &persistConfig ) = 0;
    virtual int         GetPersistProcessKeys( const char *persistProcessKeys ) = 0;
    virtual int         GetRegistryClusterSet( int &count
                                             , int max
                                             , registry_configuration_t registryConfig[] ) = 0;
    virtual int         GetRegistryProcessSet( int &count
                                             , int max
                                             , registry_configuration_t registryConfig[] ) = 0;
    virtual int         GetUniqueString( int nid, int id, const char *uniqStr ) = 0;
    virtual int         GetUniqueStringId( int nid
                                         , const char *uniqStr
                                         , int &id ) = 0;
    virtual int         GetUniqueStringIdMax( int nid, int &id ) = 0;
    inline TC_STORAGE_TYPE GetStorageType( void ) { return( dbStorageType_ ); }
    virtual int         Initialize( void ) = 0;
    virtual bool        IsInitialized( void ) = 0;
    virtual int         SaveLNodeData( int nid
                                     , int pnid
                                     , int firstCore
                                     , int lastCore
                                     , int processors
                                     , int roles ) = 0;
    virtual int         SavePNodeData( const char *name
                                     , int pnid
                                     , int excludedFirstCore
                                     , int excludedLastCore ) = 0;

protected:
    TC_STORAGE_TYPE     dbStorageType_;
private:
};


#endif /* TCDBSTORE_H_ */
