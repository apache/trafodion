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

#define MAX_TOKEN   132

class CTokenizer
{
public:

    CTokenizer( void );
    ~CTokenizer( void );

protected:
    FILE *confFile_;
    char *cmdTail_;
    char buffer_[MPI_MAX_PROCESSOR_NAME+MAX_ROLEBUF_SIZE];
    char token_[MAX_TOKEN];
    int  line_;

    char *GetToken ( char *str, char *token, char *delimiter, int maxlen = MAX_TOKEN );
    virtual bool Initialize( void ){ abort(); } // virtual function must be defined
    bool  ReadLine( void );

private:
    char *FindDelimiter( char *str );
    char *FindEndOfToken( char *str, int  maxlen );
    char *NormalizeCase( char *token );
    char *RemoveWhiteSpace( char *str );
};

class CClusterConfig  : public CTokenizer
                      , public CPNodeConfigContainer
                      , public CLNodeConfigContainer
{
public:

    CClusterConfig( void );
    ~CClusterConfig( void );

    bool  Initialize( void );
    bool  LoadConfig( void );
    inline bool IsConfigReady( void ) { return ( configReady_ ); }

protected:
private:
    bool configReady_; // true when configuration loaded
    char delimiter_;
    char nodename_[MPI_MAX_PROCESSOR_NAME];
    bool excludedNid_;
    bool excludedProcessor_;
    bool excludedCores_;
    bool gatherSpares_;
    bool newPNodeConfig_;
    bool newLNodeConfig_;
    bool spareNode_;
    int  currProcessor_;
    int  prevProcessor_;
    int  currNid_;
    int  currPNid_;
    int  prevNid_;
    int  prevPNid_;
    int  sparePNid_[MAX_NODES];
    int  spareIndex_;
    cpu_set_t currCoreMask_;
    cpu_set_t prevCoreMask_;
    cpu_set_t excludedCoreMask_;
    cpu_set_t prevExcludedCoreMask_;
    ZoneType  currZoneType_;
    ZoneType  prevZoneType_;
    CPNodeConfig *currPNodeConfig_;
    CPNodeConfig *prevPNodeConfig_;
    CLNodeConfig *lnodeConfig_;
    
    bool  ParsePNid( void );
    bool  ParseNid( void );
    bool  ParseNodename( void );
    bool  ParseProcessor( void );
    bool  ParseCore( void );
    bool  ParseRoles( void );
};

#endif /* CLUSTERCONF_H_ */
