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
    bool            Initialize( void );
    void            InitCoreMask( cpu_set_t &coreMask );
    inline bool     IsConfigReady( void ) { return( nodeReady_ && persistReady_ ); }
    inline bool     IsNodeReady( void ) { return( nodeReady_ ); }
    inline bool     IsPersistReady( void ) { return( persistReady_ ); }
    bool            LoadConfig( void );
    bool            LoadNodeConfig( void );
    bool            LoadPersistConfig( void );
    bool            SaveNodeConfig( const char *name
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
                                     , int         excludedFirstCore
                                     , int         excludedLastCore );

protected:
private:

    bool            nodeReady_;    // true when node configuration loaded
    bool            persistReady_; // true when persist configuration loaded
    bool            newPNodeConfig_;
    bool            trafConfigInitialized_;
    CPNodeConfig   *prevPNodeConfig_;
    CLNodeConfig   *prevLNodeConfig_;
    CPersistConfig *prevPersistConfig_;
#if 0
    bool       excludedCores_;
    bool       newPNodeConfig_;
    bool       newLNodeConfig_;
    int        currNid_;
    int        currPNid_;
    int        currSPNid_;
    char       currNodename_[MPI_MAX_PROCESSOR_NAME];
    cpu_set_t  currExcludedCoreMask_;
    cpu_set_t  currCoreMask_;
    int        currExcludedFirstCore_;
    int        currExcludedLastCore_;
    int        currFirstCore_;
    int        currLastCore_;
    int        currProcessor_;
    ZoneType   currZoneType_;
    CPNodeConfig *currPNodeConfig_;
    int        prevNid_;
    int        prevPNid_;
    int        prevSPNid_;
    char       prevNodename_[MPI_MAX_PROCESSOR_NAME];
    cpu_set_t  prevExcludedCoreMask_;
    cpu_set_t  prevCoreMask_;
    int        prevExcludedFirstCore_;
    int        prevExcludedLastCore_;
    int        prevFirstCore_;
    int        prevLastCore_;
    int        prevProcessor_;
    ZoneType   prevZoneType_;
    int        sparePNid_[MAX_NODES];
    int        spareIndex_;
    CPNodeConfig *prevPNodeConfig_;
    CLNodeConfig *lnodeConfig_;
    char            persistPrefix_[PERSIST_KEY_MAX];
    char            processNamePrefix_[PERSIST_VALUE_MAX];
    char            processNameFormat_[PERSIST_VALUE_MAX];
    char            stdoutPrefix_[PERSIST_VALUE_MAX];
    char            stdoutFormat_[PERSIST_VALUE_MAX];
    char            programName_[PERSIST_VALUE_MAX];
    char            zoneFormat_[PERSIST_VALUE_MAX];
    PROCESSTYPE     processType_;
    bool            requiresDTM_;
    int             persistRetries_;
    int             persistWindow_;
    CPersistConfig *persistConfig_;
#endif

    void  AddNodeConfiguration( pnodeConfigInfo_t &pnodeConfigInfo
                              , lnodeConfigInfo_t &lnodeConfigInfo );
    void  AddSNodeConfiguration( pnodeConfigInfo_t &pnodeConfigInfo );
    void  AddPersistConfiguration( persistConfigInfo_t &persistConfigInfo );
    bool  DeleteDbNodeData( int  pnid );
    PROCESSTYPE GetProcessType( const char *processtype );
    void  ProcessLNode( node_configuration_t &nodeConfig
                      , pnodeConfigInfo_t    &pnodeConfigInfo
                      , lnodeConfigInfo_t    &lnodeConfigInfo );
    void  ProcessSNode( physical_node_configuration_t &pnodeConfig
                      , pnodeConfigInfo_t             &pnodeConfigInfo );
    void  ProcessPersistInfo( persist_configuration_t &persistConfigData
                            , persistConfigInfo_t     &persistConfigInfo );
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
                                  , int         excludedFirstCore
                                  , int         excludedLastCore );
};

#endif /* CLUSTERCONF_H_ */
