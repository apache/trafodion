#ifndef STMTDDLROLEGRANT_H
#define STMTDDLROLEGRANT_H

/* -*-C++-*-
/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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

 *
 * File:         StmtDDLRoleGrant.h
 * Description:  class for parse nodes representing Grant Roles
 *               and Revoke Roles  DDL statements
 *
 * Created:      06/16/11
 * Language:     C++
 *
**********************************************************************/
#include "StmtDDLNode.h"
#include "ElemDDLGranteeArray.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLRoleGrant;
class StmtDDLRoleGrantArray;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of class StmtDDLRoleGrant
// -----------------------------------------------------------------------
class StmtDDLRoleGrant : public StmtDDLNode
{

public:

  // constructor
  StmtDDLRoleGrant(ElemDDLNode * pRolesList,  /* role list*/
                   ElemDDLNode * pGranteeList, /* grantee list */
                   NABoolean     withAdmin, /* with admin  option */
                   ElemDDLNode * pOptionalGrantedBy, /* optional granted by */
                   ComDropBehavior dropBehavior, /* optional drop behavior */
                   NABoolean     isGrantRole,   
                   CollHeap    * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLRoleGrant();

  // cast
  virtual StmtDDLRoleGrant * castToStmtDDLRoleGrant();

  //
  // accessors
  //

  inline const ComBoolean isGrantRole() { return isGrantRole_ ; } ;
  inline ComDropBehavior getDropBehavior() const;
  inline const ComBoolean isWithAdminOptionSpecified() const { return withAdmin_ ; } ;
  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  // for tracing

  //virtual const NAString displayLabel1() const;
  //virtual const NAString getText() const;

  // mutator
  virtual void setChild(Lng32 index, ExprNode * pChildNode);

  inline const ElemDDLGranteeArray & getGranteeArray() const;
  inline       ElemDDLGranteeArray & getGranteeArray();

  inline const ElemDDLGranteeArray & getRolesArray() const;
  inline       ElemDDLGranteeArray & getRolesArray();
  
  inline       ElemDDLGrantee * getGrantedBy() const { return grantedBy_; }; 

  // for processing
  ExprNode * bindNode(BindWA *bindWAPtr);

  // pointers to child parse nodes

  enum { INDEX_ROLES_LIST = 0,
         INDEX_GRANTEE_LIST,
         INDEX_GRANTED_BY_OPTION,
         MAX_STMT_DDL_GRANT_ARITY };

  ElemDDLNode * children_[MAX_STMT_DDL_GRANT_ARITY];

private: 
  // grantees
  ElemDDLGranteeArray granteeArray_;
  ElemDDLGranteeArray rolesArray_;
  NABoolean  withAdmin_;
  NABoolean   isGrantRole_;
  ElemDDLGrantee * grantedBy_;
  ComDropBehavior dropBehavior_;
};

inline ElemDDLGranteeArray &
StmtDDLRoleGrant::getGranteeArray()
{
  return granteeArray_;
}

inline const ElemDDLGranteeArray &
StmtDDLRoleGrant::getGranteeArray() const
{
  return granteeArray_;
}

inline ElemDDLGranteeArray &
StmtDDLRoleGrant::getRolesArray()
{
  return rolesArray_;
}

inline const ElemDDLGranteeArray &
StmtDDLRoleGrant::getRolesArray() const
{
  return rolesArray_;
}

inline ComDropBehavior
StmtDDLRoleGrant::getDropBehavior() const
{
  return dropBehavior_;
}

// -----------------------------------------------------------------------
// Definition of class StmtDDLRoleGrantArray
// -----------------------------------------------------------------------
class StmtDDLRoleGrantArray : public LIST(StmtDDLRoleGrant *)
{

public:

  // constructor
  StmtDDLRoleGrantArray(CollHeap *heap)
   : LIST(StmtDDLRoleGrant *)(heap)
  { }

  // virtual destructor
  virtual ~StmtDDLRoleGrantArray();

private:

}; // class StmtDDLRoleGrantArray
#endif // #define STMTDDLROLEGRANT_H
