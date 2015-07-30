#ifndef STMTDDLCREATEROLE_H
#define STMTDDLCREATEROLE_H
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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLCreateRole.h
 * Description:  Class for parse nodes representing create and drop role
 *               statements.  Patterned after StmtDDLRegisterUser.h.
 *
 * Created:      April 2011     
 * Language:     C++
 *
 *****************************************************************************
 */

#include "ComLocationNames.h"
#include "ElemDDLLocation.h"
#include "ComSmallDefs.h"
#include "StmtDDLNode.h"


// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLCreateRole;
class StmtDDLCreateRoleArray;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create and drop role statements
// -----------------------------------------------------------------------
class StmtDDLCreateRole : public StmtDDLNode
{

public:

  // constructors
  // create role
  StmtDDLCreateRole(const NAString & roleName,
                    ElemDDLNode * pOwner,
                    CollHeap * heap);
  // drop role; the only drop behavior supported by ANSI is restrict
  StmtDDLCreateRole(const NAString & roleName,
                    CollHeap * heap);

  // virtual destructor
  virtual ~StmtDDLCreateRole();

  // cast
  virtual StmtDDLCreateRole * castToStmtDDLCreateRole();

  // for binding
  ExprNode * bindNode(BindWA *bindWAPtr);

  // accessors

  inline const NAString & getRoleName() const;
  // returns TRUE if "CREATE ROLE" command, returns FALSE if "DROP ROLE" command
  inline const NABoolean isCreateRole() const;
  // returns TRUE if WITH ADMIN CURRENT_USER or no WITH ADMIN option was specified
  inline const NABoolean isCurrentUserSpecified() const;
  // getOwner will be NULL if isCurrentUserSpecified is TRUE
  inline const ElemDDLGrantee *getOwner() const;

  // for tracing

  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;
  
private:

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  NAString roleName_;
  NABoolean isCreateRole_;
  NABoolean isCurrentUserSpecified_;
  ElemDDLGrantee *pOwner_;
 
}; // class StmtDDLCreateRole

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCreateRole
// -----------------------------------------------------------------------

//
// accessors
//

inline const NAString &
StmtDDLCreateRole::getRoleName() const
{
  return roleName_;
}

inline const NABoolean
StmtDDLCreateRole::isCreateRole() const
{
  return isCreateRole_;
}

inline const NABoolean
StmtDDLCreateRole::isCurrentUserSpecified() const
{
  return (pOwner_ == NULL);
}

inline const ElemDDLGrantee *
StmtDDLCreateRole::getOwner() const
{
  return pOwner_;
}

// -----------------------------------------------------------------------
// Definition of class StmtDDLCreateRoleArray
// -----------------------------------------------------------------------
class StmtDDLCreateRoleArray : public LIST(StmtDDLCreateRole *)
{

public:

  // constructor
  StmtDDLCreateRoleArray(CollHeap *heap)
   : LIST(StmtDDLCreateRole *)(heap)
  { }

  // virtual destructor
  virtual ~StmtDDLCreateRoleArray();

private:

}; // class StmtDDLCreateRoleArray

#endif // STMTDDLCREATEROLE_H
