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

#ifndef PNODECONFIG_H_
#define PNODECONFIG_H_

#include <list>

#include "trafconf/trafconfig.h"
#include "lnodeconfig.h"

class CLNodeConfig;
class CPNodeConfig;

typedef list<CPNodeConfig *>   PNodesConfigList_t;

typedef struct pnodeConfigInfo_s
{
    int        pnid;
    char       nodename[TC_PROCESSOR_NAME_MAX];
    char       domainname[TC_PROCESSOR_NAME_MAX];
    int        excludedFirstCore;
    int        excludedLastCore;
    cpu_set_t  excludedCoreMask;
    int        spareCount;
    int        sparePNid[TC_SPARE_NODES_MAX];
} pnodeConfigInfo_t;

class CPNodeConfigContainer
{
public:
    CPNodeConfigContainer( int pnodesConfigMax );
    ~CPNodeConfigContainer( void );

    CPNodeConfig *AddPNodeConfig( pnodeConfigInfo_t &pnodeConfigInfo );
    void          Clear( void );
    static int    hostnamecmp(const char *p_str1, const char *p_str2);
    void          DeletePNodeConfig( CPNodeConfig *pnodeConfig );
    inline CPNodeConfig *GetFirstPNodeConfig( void ) { return ( head_ ); }
    inline int    GetNextPNid( void ) { return ( nextPNid_ ); }
    CPNodeConfig *GetNextPNodeConfigByName( char * name );
    int           GetPNid( char  *nodename );
    CPNodeConfig *GetPNodeConfig( char *nodename );
    CPNodeConfig *GetPNodeConfig( int pnid );
    inline int    GetPNodesConfigMax( void ) { return ( pnodesConfigMax_ ); }
    inline int    GetPNodesCount( void ) { return ( pnodesCount_ ); }
    inline int    GetSNodesCount( void ) { return ( snodesCount_ ); }
    inline PNodesConfigList_t *GetSpareNodesConfigList( void ) { return ( &spareNodesConfigList_ ); }
    void          GetSpareNodesConfigSet( const char *name, PNodesConfigList_t &spareSet );

protected:
    CPNodeConfig  **pnodesConfig_;// array of physical node configuration objects
    int             pnodesCount_; // # of physical nodes 
    int             snodesCount_; // # of spare nodes
    int             nextPNid_;    // next physical node id available

private:
    static char  *NormalizeCase( char *token );

    int             pnodesConfigMax_; // maximum number of physical nodes
    PNodesConfigList_t  spareNodesConfigList_; // configured spare nodes list
    CPNodeConfig  *head_; // head of physical nodes linked list
    CPNodeConfig  *tail_; // tail of physical nodes linked list

};

class CPNodeConfig : public CLNodeConfigContainer
{
    friend CPNodeConfig *CPNodeConfigContainer::AddPNodeConfig( pnodeConfigInfo_t &pnodeConfigInfo );
    friend void CPNodeConfigContainer::DeletePNodeConfig( CPNodeConfig *pnodeConfig );
public:

    CPNodeConfig( CPNodeConfigContainer *pnodesConfig
                , pnodeConfigInfo_t &pnodeConfigInfo
                );
    ~CPNodeConfig( void );

    inline cpu_set_t    &GetExcludedCoreMask( void ) { return (excludedCoreMask_); }
    inline int           GetExcludedFirstCore( void ) { return ( excludedFirstCore_ ); }
    inline int           GetExcludedLastCore( void ) { return ( excludedLastCore_ ); }
    inline const char   *GetDomain( void ) { return ( domain_ ); }
    inline const char   *GetFqdn( void ) { return ( fqdn_ ); }
    inline const char   *GetName( void ) { return ( name_ ); }
    inline CPNodeConfig *GetNext( void ) { return ( next_ ); }
    inline int           GetPNid( void ) { return ( pnid_ ); }
    void                 SetDomain( const char *newDomain ); 
    void                 SetName( const char *newName ); 
    void                 SetExcludedFirstCore( int excludedFirstCore ); 
    void                 SetExcludedLastCore( int excludedLastCore ); 
    int                  GetSpareList( int sparePNid[] );
    inline int           GetSparesCount( void ) { return ( sparePNidsCount_ ); }

    inline bool          IsSpareNode( void ) { return ( spareNode_ ); }
    inline void          SetExcludedCoreMask( cpu_set_t &coreMask ) { excludedCoreMask_ = coreMask; }
    void                 SetSpareList( int sparePNid[], int spareCount );
    void                 ResetSpare( void );

protected:
private:
    CPNodeConfigContainer *pnodesConfig_; // physical nodes container
    char                   name_[TC_PROCESSOR_NAME_MAX]; // short hostname
    char                   domain_[TC_PROCESSOR_NAME_MAX]; // domain
    char                   fqdn_[TC_PROCESSOR_NAME_MAX]; // FQDN hostname
    int                    pnid_;         // physical node identifier
    cpu_set_t              excludedCoreMask_; // mask of excluded SMP processor cores
    int                    excludedFirstCore_;// First excluded SMP processor core used by logical node
    int                    excludedLastCore_; // Last excluded SMP processor core used by logical node
    bool                   spareNode_;    // true when this is a spare physical node
    int                   *sparePNids_;   // array of pnids this physical node can spare
    int                    sparePNidsCount_; // # of entries in spare sparePNids_ array

    CPNodeConfig *next_; // next PNodeConfig in CPNodeConfigContainer list
    CPNodeConfig *prev_; // previous PNodeConfig in CPNodeConfigContainer list
};

#endif /* PNODECONFIG_H_ */
