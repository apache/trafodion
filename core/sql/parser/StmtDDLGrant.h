#ifndef STMTDDLGRANT_H
#define STMTDDLGRANT_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLGrant.h
 * Description:  class for parse nodes representing Grant DDL statements
 *
 *
 * Created:      10/16/95
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */


#include "ElemDDLGranteeArray.h"
#include "ElemDDLPrivActions.h"
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLGrant;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of class StmtDDLGrant
// -----------------------------------------------------------------------
class StmtDDLGrant : public StmtDDLNode
{

public:

  // constructor
  StmtDDLGrant(ElemDDLNode * pPrivileges,
               const QualifiedName & objectName,
               ElemDDLNode * pGranteeList,
               ElemDDLNode * pWithGrantOption,
               ElemDDLNode * pByGrantorOption,
               QualifiedName * actionName,
               CollHeap    * heap = PARSERHEAP());
  
  // virtual destructor
  virtual ~StmtDDLGrant();

  // cast
  virtual StmtDDLGrant * castToStmtDDLGrant();

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline const ElemDDLGranteeArray & getGranteeArray() const;
  inline       ElemDDLGranteeArray & getGranteeArray();

  inline const QualifiedName & getGrantNameAsQualifiedName() const;
  inline       QualifiedName & getGrantNameAsQualifiedName();   
        // returns the array of pointers pointing to parse
        // nodes representing grantees.  This array contains
        // at least one element.

  inline NAString getObjectName() const;
  inline NAString getOrigObjectName() const
  {return origObjectName_;}

  void setObjectQualName(QualifiedName qn)
  {
    objectQualName_ = qn;
    objectName_ = qn.getQualifiedNameAsAnsiString();
  }

  inline const QualifiedName * getActionNameAsQualifiedName() const;

        // returns the name of the object appearing in the
        // Grant statement

  inline const ElemDDLPrivActArray & getPrivilegeActionArray() const;
  inline       ElemDDLPrivActArray & getPrivilegeActionArray();

        // returns the array of pointer pointing to parse
        // nodes representing privilege actions.  If the All
        // Privileges phrase appears in the Grant statement,
        // the returned array is empty.  The method
        // isAllPrivileges() may be used to find out whether
        // the All Privileges phrase appears or not.
 
  inline NAString getTableName() const;

        // same as getObjectName() - returns the name of the
        // object appearing in the Grant statement

  inline NABoolean isAllPrivilegesSpecified() const;

        // returns TRUE if the All Privileges phrase appears
        // in the Grant statement; returns FALSE otherwise.
        //
        // Note that getPrivilegeActionArray() returns an
        // empty array when the the All Privileges phrase
        // appears in the Grant statement.

  inline NABoolean isWithGrantOptionSpecified() const;

        // returns TRUE if the With Grant Option phrase appears
        // in the Grant statement; returns FALSE otherwise.
  
  inline NABoolean isByGrantorOptionSpecified() const;

        // returns TRUE if the As <grantor) phrase appears
        // in the Revoke statement; returns FALSE otherwise.

  inline const ElemDDLGrantee * getByGrantor() const;
        // returns pointer to the optional "by grantor" 
        // (in the form of an ElemDDLGrantee).

  // mutator
  virtual void setChild(Lng32 index, ExprNode * pChildNode);

  // for processing
  ExprNode * bindNode(BindWA *bindWAPtr);

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;


private:

  NAString origObjectName_;
  NAString objectName_;
  QualifiedName objectQualName_;

  // privilege actions
  NABoolean isAllPrivileges_;
  ElemDDLPrivActArray privActArray_;

  // For action clause
  QualifiedName *actionQualName_;

  // grantees
  ElemDDLGranteeArray granteeArray_;

  // with grant option
  NABoolean isWithGrantOptionSpec_;

  // by grantor option specified?
  NABoolean isByGrantorOptionSpec_;

  // by grantor option value
  ElemDDLGrantee * byGrantor_;

  // pointers to child parse nodes

  enum { INDEX_PRIVILEGES = 0,
         INDEX_GRANTEE_LIST,
         INDEX_WITH_GRANT_OPTION,
         INDEX_BY_GRANTOR_OPTION,
         MAX_STMT_DDL_GRANT_ARITY };

  ElemDDLNode * children_[MAX_STMT_DDL_GRANT_ARITY];

}; // class StmtDDLGrant

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLGrant
// -----------------------------------------------------------------------

//
// accessors
//
inline QualifiedName &
StmtDDLGrant::getGrantNameAsQualifiedName()
{
  return objectQualName_;
}

inline const QualifiedName & 
StmtDDLGrant::getGrantNameAsQualifiedName() const 
{
  return objectQualName_;
}
inline NAString
StmtDDLGrant::getObjectName() const
{
  return objectName_;
}

inline ElemDDLGranteeArray &
StmtDDLGrant::getGranteeArray()
{
  return granteeArray_;
}

inline const ElemDDLGranteeArray &
StmtDDLGrant::getGranteeArray() const
{
  return granteeArray_;
}

inline ElemDDLPrivActArray &
StmtDDLGrant::getPrivilegeActionArray()
{
  return privActArray_;
}

inline const ElemDDLPrivActArray &
StmtDDLGrant::getPrivilegeActionArray() const
{
  return privActArray_;
}

inline NAString
StmtDDLGrant::getTableName() const
{
  return objectName_;
}

inline const QualifiedName *
StmtDDLGrant::getActionNameAsQualifiedName() const
{
  return actionQualName_;
}

inline NABoolean
StmtDDLGrant::isAllPrivilegesSpecified() const
{
  return isAllPrivileges_;
}

inline NABoolean
StmtDDLGrant::isWithGrantOptionSpecified() const
{
  return isWithGrantOptionSpec_;
}

inline NABoolean
StmtDDLGrant::isByGrantorOptionSpecified() const
{
  return isByGrantorOptionSpec_;
}

inline const ElemDDLGrantee *
StmtDDLGrant::getByGrantor() const
{
  return byGrantor_;
}

#endif // STMTDDLGRANT_H
