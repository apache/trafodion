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

#ifndef NAMESERVERCONFIG_H_
#define NAMESERVERCONFIG_H_

#include "trafconf/trafconfig.h"

class CNameServerConfig;

class CNameServerConfigContainer
{
public:
    CNameServerConfigContainer( void );
    ~CNameServerConfigContainer( void );
    void                      AddConfig( const char *nodeName );
    void                      Clear( void );
    bool                      DeleteConfig( CNameServerConfig *config );
    bool                      LoadConfig( void );
    inline CNameServerConfig *GetFirstConfig( void ) { return ( head_ ); }
    CNameServerConfig        *GetConfig( const char *nodeName );
    inline int                GetConfigMax( void ) { return ( configMax_ ); }
    inline int                GetCount( void ) { return ( count_ ); }
    void                      RemoveConfig( CNameServerConfig *config );
    bool                      SaveConfig( const char *nodeName );

protected:
    int                  count_;     // # of name servers

private:
    int                  configMax_; // maximum number of name servers
    CNameServerConfig  **config_;    // array of all name servers

    CNameServerConfig   *head_;  // head of name server linked list
    CNameServerConfig   *tail_;  // tail of name server linked list
};

class CNameServerConfig
{
    friend void CNameServerConfigContainer::AddConfig( const char *nodeName );
    friend void CNameServerConfigContainer::RemoveConfig( CNameServerConfig *nameServerConfig );
public:
    CNameServerConfig( const char *nodeName );
    ~CNameServerConfig( void );

    const char               *GetName( void );
    inline CNameServerConfig *GetNext( void ) { return( next_); }

protected:
private:
    char *nodeName_;      // node name

    CNameServerConfig *next_;   // next NameServerConfig in CNameServerConfigContainer list
    CNameServerConfig *prev_;   // previous NameServerConfig in CNameServerConfigContainer list
};

#endif /* NAMESERVERCONFIG_H_ */
