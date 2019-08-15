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

#ifndef CLUSTERCONF_H_
#define CLUSTERCONF_H_

#include <stdlib.h>

#include "lnodeconfig.h"
#include "pnodeconfig.h"
#include "persistconfig.h"

class CClusterConfig  : public CPNodeConfigContainer
                      , public CLNodeConfigContainer
                      , public CPersistConfigContainer
{
public:

    CClusterConfig( void );
    ~CClusterConfig( void );

    void            Clear( void );
    bool            DeleteNodeConfig( int  pnid );
    inline int      GetClusterId( void ) { return clusterId_;} 
    inline int      GetConfigMaster( void ) { return configMaster_;} 
    inline char *   GetConfigMasterByName( void ) {return configMasterName_;} 
    inline int      GetInstanceId( void ) { return instanceId_;} 
    bool            Initialize( void );
    bool            Initialize( bool traceEnabled, const char *traceFile );
    void            InitCoreMask( cpu_set_t &coreMask );
    inline bool     IsConfigReady( void ) { return( nodeReady_ && persistReady_ ); }
    inline bool     IsNodeReady( void ) { return( nodeReady_ ); }
    inline bool     IsPersistReady( void ) { return( persistReady_ ); }
    inline TcStorageType_t GetStorageType( void ) { return(trafConfigStorageType_); }
    bool            LoadConfig( void );
    bool            LoadNodeConfig( void );
    bool            LoadPersistConfig( void );
    bool            SaveNodeConfig( const char *name
                                  , const char *domain
                                  , int         nid
                                  , int         pnid
                                  , int         firstCore
                                  , int         lastCore
                                  , int         processors
                                  , int         excludedFirstCore
                                  , int         excludedLastCore
                                  , int         roles );
    void            SetCoreMask( int        firstCore
                               , int        lastCore
                               , cpu_set_t &coreMask );
    bool            UpdatePNodeConfig( int         pnid
                                     , const char *name
                                     , const char *domain
                                     , int         excludedFirstCore
                                     , int         excludedLastCore );

protected:
private:

    int             configMaster_;
    int             clusterId_;
    int             instanceId_;
    char            configMasterName_[TC_PROCESSOR_NAME_MAX];
    bool            isRealCluster_;
    bool            nodeReady_;    // true when node configuration loaded
    bool            persistReady_; // true when persist configuration loaded
    bool            newPNodeConfig_;
    bool            trafConfigInitialized_;
    TcStorageType_t trafConfigStorageType_;
    CPNodeConfig   *prevPNodeConfig_;
    CLNodeConfig   *prevLNodeConfig_;
    CPersistConfig *prevPersistConfig_;

    void  AddNodeConfiguration( pnodeConfigInfo_t &pnodeConfigInfo
                              , lnodeConfigInfo_t &lnodeConfigInfo );
    void  AddSNodeConfiguration( pnodeConfigInfo_t &pnodeConfigInfo );
    void  AddPersistConfiguration( persistConfigInfo_t &persistConfigInfo );
    bool  DeleteDbNodeData( int  pnid );
    TcProcessType_t GetProcessType( const char *processtype );
    void  ProcessLNode( TcNodeConfiguration_t &nodeConfig
                      , pnodeConfigInfo_t     &pnodeConfigInfo
                      , lnodeConfigInfo_t     &lnodeConfigInfo );
    void  ProcessSNode( TcPhysicalNodeConfiguration_t &pnodeConfig
                      , pnodeConfigInfo_t             &pnodeConfigInfo );
    void  ProcessPersistInfo( TcPersistConfiguration_t &persistConfigData
                            , persistConfigInfo_t      &persistConfigInfo );
    bool  SaveDbLNodeData( int         nid
                         , int         pnid
                         , int         firstCore
                         , int         lastCore
                         , int         processors
                         , int         roles );
    bool  SaveDbPNodeData( const char *name
                         , int         pnid
                         , int         excludedFirstCore
                         , int         excludedLastCore );
    bool  UpdateDbPNodeData( int         pnid
                           , const char *name
                           , int         excludedFirstCore
                           , int         excludedLastCore );
    void  UpdatePNodeConfiguration( int         pnid
                                  , const char *name
                                  , const char *domain
                                  , int         excludedFirstCore
                                  , int         excludedLastCore );
};

#endif /* CLUSTERCONF_H_ */
