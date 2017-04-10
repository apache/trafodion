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

#include "lnodeconfig.h"

typedef list<CPNodeConfig *>   PNodesConfigList_t;

class CPNodeConfigContainer
{
public:
    CPNodeConfigContainer( void );
    ~CPNodeConfigContainer( void );

    static int hostnamecmp(const char *p_str1, const char *p_str2);
    CPNodeConfig *AddPNodeConfig( int pnid, char *name, bool spare );
    void          DeletePNodeConfig( CPNodeConfig *pnodeConfig );
    inline CPNodeConfig *GetFirstPNodeConfig( void ) { return ( head_ ); }
    int           GetPNid( char  *nodename );
    CPNodeConfig *GetPNodeConfig( int pnid );
    inline int    GetPNodesCount( void ) { return ( pnodesCount_ ); }
    inline int    GetSNodesCount( void ) { return ( snodesCount_ ); }
    inline PNodesConfigList_t *GetSpareNodesConfigList( void ) { return ( &spareNodesConfigList_ ); }
    void          GetSpareNodesConfigSet( const char *name, PNodesConfigList_t &spareSet );

protected:
    CPNodeConfig  **pnodeConfig_; // array of physical node configuration objects
    int             pnodesCount_; // # of physical nodes 
    int             snodesCount_; // # of spare nodes

private:
    PNodesConfigList_t  spareNodesConfigList_; // configured spare nodes list
    CPNodeConfig  *head_; // head of physical nodes linked list
    CPNodeConfig  *tail_; // tail of physical nodes linked list

};

class CPNodeConfig : public CLNodeConfigContainer
{
    friend CPNodeConfig *CPNodeConfigContainer::AddPNodeConfig( int pnid, char *name, bool spare );
    friend void CPNodeConfigContainer::DeletePNodeConfig( CPNodeConfig *entry );
public:

    CPNodeConfig( CPNodeConfigContainer *pnodesConfig
                , int                    pnid
                , const char            *hostname
                );
    ~CPNodeConfig( void );

    inline cpu_set_t    &GetExcludedCoreMask( void ) { return (excludedCoreMask_); }
    inline int           GetLNodesCount( void ) { return ( CLNodeConfigContainer::lnodesCount_ ); }
    inline const char   *GetName( void ) { return ( name_ ); }
    inline CPNodeConfig *GetNext( void ) { return ( next_ ); }
    inline int           GetPNid( void ) { return ( pnid_ ); }
    void                 SetName( char *newName ); 
    int                  GetSpareList( int sparePNid[] );
    inline int           GetSparesCount( void ) { return ( sparePNidsCount_ ); }

    inline bool          IsSpareNode( void ) { return ( spareNode_ ); }
    inline void          SetExcludedCoreMask( cpu_set_t &coreMask ) { excludedCoreMask_ = coreMask; }

    void        SetSpareList( int sparePNid[], int spareCount );
    void        ResetSpare( void );

protected:
private:
    CPNodeConfigContainer *pnodesConfig_; // physical nodes container
    char                   name_[MPI_MAX_PROCESSOR_NAME]; // hostname
    int                    pnid_;         // physical node identifier
    cpu_set_t              excludedCoreMask_; // mask of excluded SMP processor cores
    bool                   spareNode_;    // true when this is a spare physical node
    int                   *sparePNids_;   // array of pnids this physical node can spare
    int                    sparePNidsCount_; // # of entries in spare sparePNids_ array

    CPNodeConfig *next_; // next PNodeConfig in CPNodeConfigContainer list
    CPNodeConfig *prev_; // previous PNodeConfig in CPNodeConfigContainer list
};

#endif /* PNODECONFIG_H_ */
