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

#ifndef NODE_H_
#define NODE_H_

#include <map>
#include <string>
using namespace std;

class CPhysicalNode;


// Typedefs
typedef enum NodeState {
    StateDown=0,   // unavailable
    StateUp,       // available
    StateUnknown   // not yet known 
} NodeState_t;

typedef map<string, CPhysicalNode *> PhysicalNodeNameMap_t;

// Globals
//extern PhysicalNodeNameMap_t PhysicalNodeMap;

// Class declaration
class CPhysicalNode
{
public:
    CPhysicalNode( const char *name, NodeState_t state );
   ~CPhysicalNode( void );

    inline const char *GetName( void ) { return( name_.c_str() ); }
    inline NodeState_t GetState( void ) { return( state_ ); }
    inline void    SetState( NodeState_t state ) { state_ = state; }
    inline void SetName( const char * newName ) { if (newName) name_ = newName; } 
 

private:
    string          name_;
    NodeState_t     state_;
};

#endif /*NODE_H_*/
