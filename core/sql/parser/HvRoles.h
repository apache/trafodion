/**********************************************************************
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
**********************************************************************/
#ifndef HVROLES_H
#define HVROLES_H

// This file holds the enum declaration and the Rogue Wave declaration
// necessary to create a vector whose elements describe the role
// of host variables in a SQL statement.  There is a vector element
// of many of the Statement Node (see StmtNode.h) derived classes.

#include "Collections.h"

// We like to assert our selves in here
#include <assert.h>

//
// WARNING: You MUST make sure that if you modify this list
// of enumeration definitions that you also modify the list
// in HvRoles.C.
//

enum HostVarRole {
  HV_UNASSIGNED,
  HV_IS_STRING,
  HV_IS_LITERAL, /* A misnomer since, it is not a host variable
                 that is a literal.  Used in routing host vars and literals. */
  HV_IS_ANY,
  HV_IS_INPUT,
  HV_IS_OUTPUT,
  HV_IS_INDICATOR,
  HV_IS_CURSOR_NAME,
  HV_IS_DESC_NAME,
  HV_IS_STMT_NAME,
  HV_IS_DESC_DEST,
  HV_IS_DESC_SOURCE,
  HV_IS_DYNPARAM,
  HV_IS_INPUT_OUTPUT  // A variable appearing in a SET statement of a Compound Statement
};


// default constructor makes this empty
// how about a method to clear the vector -- clear() already does this

class HostVarRole_vec : public  ARRAY(HostVarRole)
{
friend ostream& operator<<(ostream&, HostVarRole_vec &);
public:

   HostVarRole_vec(NAHeap* heap):ARRAY(HostVarRole)(heap) {};

   void setFirstUnassignedTo(HostVarRole theRole);
   void setLastUnassignedTo(HostVarRole theRole);
   void setLastNunassignedTo(UInt32  count, HostVarRole theRole);
   void setAllAssignedInputTo(HostVarRole theRole);
   void addUnassigned();

   void addARole(HostVarRole theRole);
   void add_indicator();
};

// These global declarations must be done after the above class definitions!
extern THREAD_P HostVarRole_vec *TheHostVarRoles;    // SqlParserGlobals.h

#endif // HVROLES_H
