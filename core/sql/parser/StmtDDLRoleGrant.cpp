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
 * Description:  Methods for classes representing Grant/Revoke Role Statements.
 *               Patterned after StmtDDLRegisterUser.cpp.
 *
 * Created:      June 2011
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS        // must precede all #include's

#include <stdlib.h>
#ifndef NDEBUG
#include <iostream>
#endif
#include "AllElemDDLPartition.h"
#include "AllElemDDLParam.h"
#include "AllElemDDLUdr.h"
#include "StmtDDLRoleGrant.h"
#include "BaseTypes.h"
#include "ComDiags.h"
#include "ComOperators.h"

#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif

#include "SqlParserGlobals.h"   // must be last #include


// -----------------------------------------------------------------------
// member functions for class StmtDDLRoleGrant
// -----------------------------------------------------------------------
// constructor for GRANT /REVOKE ROLE
StmtDDLRoleGrant::StmtDDLRoleGrant(ElemDDLNode * pRolesList,
                                   ElemDDLNode * pGranteeList,
                                   NABoolean     withAdmin,
                                   ElemDDLNode * pOptionalGrantedBy,
                                   ComDropBehavior dropBehavior,
				   NABoolean     isGrantRole,
                                   CollHeap    * heap)

: StmtDDLNode(DDL_GRANT_ROLE),
  withAdmin_(withAdmin),
  isGrantRole_(isGrantRole),
  dropBehavior_(dropBehavior),
  grantedBy_(NULL)
{

  setChild(INDEX_ROLES_LIST, pRolesList);
  setChild(INDEX_GRANTEE_LIST, pGranteeList);
  setChild(INDEX_GRANTED_BY_OPTION, pOptionalGrantedBy);

  //
  // copies pointers to parse nodes representing grantee
  // to granteeArray_ so the user can access the information
  // easier.
  //

  ComASSERT(pGranteeList NEQ NULL);
  for (CollIndex i = 0; i < pGranteeList->entries(); i++)
  {
    granteeArray_.insert((*pGranteeList)[i]->castToElemDDLGrantee());
  }

  ComASSERT(pRolesList NEQ NULL);
  for (CollIndex i = 0; i < pRolesList->entries(); i++)
  {
    rolesArray_.insert((*pRolesList)[i]->castToElemDDLGrantee());
  }

  if ( pOptionalGrantedBy NEQ NULL )
  {
    grantedBy_ = pOptionalGrantedBy->castToElemDDLGrantee();
  }

} // StmtDDLRoleGrant::StmtDDLRoleGrant()

// virtual destructor
StmtDDLRoleGrant::~StmtDDLRoleGrant()
{
  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}
// cast
StmtDDLRoleGrant *
StmtDDLRoleGrant::castToStmtDDLRoleGrant()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLRoleGrant::getArity() const
{
  return MAX_STMT_DDL_GRANT_ARITY;
}

ExprNode *
StmtDDLRoleGrant::getChild(Lng32 index)
{
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

//
// mutators
//

void
StmtDDLRoleGrant::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    children_[index] = pChildNode->castToElemDDLNode();
  }
  else
  {
    children_[index] = NULL;
  }
}

// -----------------------------------------------------------------------
// methods for class StmtDDLRoleGrantArray
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLRoleGrantArray::~StmtDDLRoleGrantArray()
{
}
