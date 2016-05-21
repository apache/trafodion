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

#ifndef CMSH_H_
#define CMSH_H_

#include <list>
#include <string>
using namespace std;

#include "testpoint.h"
#include "phnode.h"
#include "system.h"

typedef list<string>    NodeStateList_t;

class CCmsh : public CSystem
{
public:
    CCmsh( const char *command );
   ~CCmsh( void );

    int  PopulateClusterState( void );
    int  GetClusterState( PhysicalNodeNameMap_t &physicalNodeMap );
    bool  IsInitialized( void ); 
    void ClearClusterState( void ) { nodeStateList_.clear(); }
    NodeState_t GetNodeState( char nodeName[] );
    
private:
    NodeStateList_t    nodeStateList_;
    
    void ParseNodeStatus( string &nodeStatus, string &nodeName, NodeState_t &state );
};

#endif /*CMSH_H_*/
