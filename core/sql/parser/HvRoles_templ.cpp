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
// This file holds the implementation code supporting the
// HostVarRoles_vec class.

#include "Platform.h" 

// Always good to assert yourself.
#include <assert.h>

// We provide an output operator, so we need...
#include <iostream>


// Always good to include yourself so to speak.
#include "HvRoles.h"


// The //   HostVarRole   current_role_mode;
// entries in this array are defined based on the assumption
// that the corresponding enum definitions are 0,1,2...etc...
// If that changes, then these definitions must also change.

const char *host_var_name[] = {
  "HV_UNASSIGNED",
  "HV_IS_STRING",
  "HV_IS_LITERAL",
  "HV_IS_ANY",
  "HV_IS_INPUT",
  "HV_IS_OUTPUT",
  "HV_IS_INDICATOR",
  "HV_IS_CURSOR_NAME",
  "HV_IS_DESC_NAME",
  "HV_IS_STMT_NAME",
  "HV_IS_DESC_DEST",
  "HV_IS_DESC_SOURCE",
  "HV_IS_DYNPARAM",
  "HV_IS_INPUT_OUTPUT"  // A variable appearing in a SET statement of a Compound Statement

};

ostream& operator<<(ostream& o, HostVarRole_vec &v)
{
   UInt32   i;
   for (i=0;i!=v.entries();i++)
     o << host_var_name[v[i]] << "  " << flush ;
   //for
   return o;
}

// What this function should do is to search through the array looking
// for the first role which == HV_UNASSIGNED.
// It is an assertion fail if the vector is empty, or, if it contains
// no unassigned roles.


void HostVarRole_vec::setFirstUnassignedTo(HostVarRole theRole)
{
  assert(entries()!=0);
  UInt32 i;
  for (i=0;i!=entries() && (*this)[i] != HV_UNASSIGNED; i++) ;
  assert( i!=entries());
  (*this)[i]=theRole;
}

// Very similar to the previous method is this next method.

void HostVarRole_vec::setLastUnassignedTo(HostVarRole theRole)
{
  assert(entries()!=0);
  UInt32 i=entries();
  while (TRUE) 
    {
      assert(i!=0);
      i--;
      if ((*this)[i] == HV_UNASSIGNED)
	break;
      //if
    }
  (*this)[i]=theRole;  
}

// This function has a loop similar to the setLastUnassignedTo() method.
// However, in this loop, we are not setting just one, but some number, count,
// of the tail unassigned roles.  The loop skips over those which are NOT
// assigned.  We make it an assertion fail if those that ARE assigned are
// anything other than HV_IS_INDICATOR.  It is an assertion fail if we fall
// off the front of the list as well.
//
// count MAY be 0, although, we wonder why the client calls us in that case.

void HostVarRole_vec::setLastNunassignedTo(UInt32  count, HostVarRole theRole)
{
  assert(entries() >= count);
  UInt32 i=entries();
  while (i > 0) 
    {
      i--;
      if ((*this)[i] == HV_UNASSIGNED)
	{
	  (*this)[i] = theRole;
	  count--;
	  if (count==0) break;
	}
    }
}

void HostVarRole_vec::setAllAssignedInputTo(HostVarRole theRole)
{
  if (entries() == 0) return;
  UInt32 i=entries();
  while (i > 0) 
    {
      i--;
      if ( HV_IS_INPUT == (*this)[i] )
        (*this)[i] = theRole;
    }
}


void HostVarRole_vec::addUnassigned()
{
  insertAt(entries(), HV_UNASSIGNED); 
}


void HostVarRole_vec::add_indicator()
{
  insertAt(entries(), HV_IS_INDICATOR);
}

void HostVarRole_vec::addARole(HostVarRole theRole)
{
  insertAt(entries(), theRole);
}
