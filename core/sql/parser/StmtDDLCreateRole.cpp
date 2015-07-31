/* -*-C++-*-
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
****************************************************************************
 *
 * File:         StmtDDLCreateRole.cpp
 * Description:  Methods for classes representing Create/Drop Role Statements.
 *               Patterned after StmtDDLRegisterUser.cpp.
 *
 * Created:      April 2011
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's

#include <stdlib.h>
#ifndef NDEBUG
#include <iostream>
#endif
#include "AllElemDDLPartition.h"
#include "AllElemDDLParam.h"
#include "AllElemDDLUdr.h"
#include "StmtDDLCreateRole.h"
#include "BaseTypes.h"
#include "ComDiags.h"
#include "ComOperators.h"

#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif

#include "SqlParserGlobals.h"	// must be last #include


// -----------------------------------------------------------------------
// methods for class StmtDDLCreateRole
// -----------------------------------------------------------------------

//
// constructor
//
// constructor used for CREATE ROLE
StmtDDLCreateRole::StmtDDLCreateRole(const NAString & roleName,
                                     ElemDDLNode * pOwner,
                                     CollHeap * heap)
  : StmtDDLNode(DDL_CREATE_ROLE),
    roleName_(roleName,heap),
    isCreateRole_(TRUE)
{
  if (pOwner)
  {

      pOwner_ = pOwner->castToElemDDLGrantee();
      ComASSERT(pOwner_ NEQ NULL);
      isCurrentUserSpecified_ = FALSE;
  }
  else
  {
      pOwner_ = NULL;
      isCurrentUserSpecified_ = TRUE;
  }
} 

// constructor used for DROP ROLE
StmtDDLCreateRole::StmtDDLCreateRole(const NAString & roleName,
                                     CollHeap * heap)
  : StmtDDLNode(DDL_CREATE_ROLE),
    roleName_(roleName,heap),
    isCreateRole_(FALSE),
    pOwner_(NULL),       // does not apply to drop but don't leave it uninitialized
    isCurrentUserSpecified_(FALSE)      // does not apply to drop but don't leave it uninitialized
{
} // StmtDDLCreateRole::StmtDDLCreateRole()

//
// virtual destructor
//
StmtDDLCreateRole::~StmtDDLCreateRole()
{
  // delete all children
  if (pOwner_)
    delete pOwner_;
}

//
// cast
//
StmtDDLCreateRole *
StmtDDLCreateRole::castToStmtDDLCreateRole()
{
  return this;
}


//
// methods for tracing
//

const NAString
StmtDDLCreateRole::displayLabel1() const
{
  return NAString("Role name: ") + getRoleName();
}

const NAString
StmtDDLCreateRole::getText() const
{
  return "StmtDDLCreateRole";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLCreateRoleArray
// -----------------------------------------------------------------------

// virtual destructor
// Do the list of user commands need to be removed?
StmtDDLCreateRoleArray::~StmtDDLCreateRoleArray()
{
}



//
// End of File
//
