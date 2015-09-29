///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett Packard Enterprise Development LP
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LNODECONFIG_H_
#define LNODECONFIG_H_

class CLNodeConfig;
class CPNodeConfig;

class CLNodeConfigContainer
{
public:
    CLNodeConfigContainer( void );
    CLNodeConfigContainer( int lnodesSize );
    ~CLNodeConfigContainer( void );

    CLNodeConfig *AddLNodeConfig( CPNodeConfig *pnodeConfig
                                , int           nid
                                , cpu_set_t    &coreMask 
                                , int           firstCore
                                , int           lastCore
                                , int           processors
                                , ZoneType      zoneType
                                );
    CLNodeConfig *AddLNodeConfigP( CLNodeConfig *lnodeConfig );
    void          Clear( void );
    void          DeleteLNodeConfig( CLNodeConfig *lnodeConfig );
    void          RemoveLNodeConfigP( CLNodeConfig *lnodeConfig );
    inline CLNodeConfig *GetFirstLNodeConfig( void ) { return ( head_ ); }
    inline int    GetNextNid( void ) { return ( nextNid_ ); }
    CLNodeConfig *GetLNodeConfig( int nid );
    inline int    GetLNodesCount( void ) { return ( lnodesCount_ ); }

protected:
    int             lnodesCount_; // # of logical nodes 
    int             nextNid_;     // next logical node id available

private:
    int             lnodeConfigSize_; // size of array 
    CLNodeConfig  **lnodeConfig_;     // array of all logical nodes

    CLNodeConfig   *head_;  // head of logical nodes linked list
    CLNodeConfig   *tail_;  // tail of logical nodes linked list
};

class CLNodeConfig
{
    friend CLNodeConfig *CLNodeConfigContainer::AddLNodeConfig( CPNodeConfig *pnodeConfig
                                                              , int           nid
                                                              , cpu_set_t    &coreMask 
                                                              , int           firstCore
                                                              , int           lastCore
                                                              , int           processors
                                                              , ZoneType      zoneType
                                                              );
    friend CLNodeConfig *CLNodeConfigContainer::AddLNodeConfigP( CLNodeConfig *lnodeConfig );
    friend void CLNodeConfigContainer::DeleteLNodeConfig( CLNodeConfig *lnodeConfig );
    friend void CLNodeConfigContainer::RemoveLNodeConfigP( CLNodeConfig *lnodeConfig );
public:
    CLNodeConfig( CPNodeConfig *pnodeConfig
                , int           nid
                , cpu_set_t    &coreMask 
                , int           firstCore
                , int           lastCore
                , int           processors
                , ZoneType      zoneType
                );
    ~CLNodeConfig( void );

    inline cpu_set_t    &GetCoreMask( void ) { return( coreMask_ ); }
    inline int           GetFirstCore( void ) { return( firstCore_ ); }
    inline int           GetLastCore( void ) { return( lastCore_ ); }
    const char          *GetName( void );
    inline CLNodeConfig *GetNext( void ) { return( next_); }
    inline CLNodeConfig *GetNextP( void ) { return( nextP_); }
    inline int           GetNid( void ) { return( nid_ ); }
    int                  GetPNid( void );
    CPNodeConfig        *GetPNodeConfig( void ) { return(pnodeConfig_); }

    inline int           GetProcessors( void ) { return( processors_ ); }
    inline ZoneType      GetZoneType( void ) { return( zoneType_ ); }

//    inline void          SetPNid( CPNodeConfig *pnodeConfig ) { pnodeConfig_ = pnodeConfig; }

protected:
private:
    int           nid_;         // Logical Node Identifier
    cpu_set_t     coreMask_;    // mask of SMP processor cores used by logical node
    int           firstCore_;   // First SMP processor core used by logical node
    int           lastCore_;    // Last SMP processor core used by logical node
    int           processors_;  // # of logical processors in logical node
    ZoneType      zoneType_;    // type of zone
    CPNodeConfig *pnodeConfig_; // logical node's current physical node

    CLNodeConfig *next_;   // next LNodeConfig in CLNodeConfigContainer list
    CLNodeConfig *prev_;   // previous LNodeConfig in CLNodeConfigContainer list
    CLNodeConfig *nextP_;  // next LNodeConfig in pnodeConfig_ linked list
    CLNodeConfig *prevP_;  // prev LNodeConfig in pnodeConfig_ linked list
};

#endif /* LNODECONFIG_H_ */
