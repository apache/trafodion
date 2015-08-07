#ifndef STMTDDLPUBLISH_H
#define STMTDDLPUBLISH_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLPublish.h
 * Description:  class for parse nodes representing Publish DDL statements
 *
 *
 * Created:      6/3/2009
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
class StmtDDLPublish;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of class StmtDDLPublish
// -----------------------------------------------------------------------
class StmtDDLPublish : public StmtDDLNode
{

public:

  // constructor
  StmtDDLPublish(ElemDDLNode * pPrivileges,
               const QualifiedName & objectName,
               const NAString & synonymName,
               NABoolean isRole,
               ElemDDLNode * pGranteeList,
               NABoolean isPublish,
               CollHeap    * heap = PARSERHEAP());
  
  // virtual destructor
  virtual ~StmtDDLPublish();

  // cast
  virtual StmtDDLPublish * castToStmtDDLPublish();

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline const ElemDDLGranteeArray & getGranteeArray() const;
  inline       ElemDDLGranteeArray & getGranteeArray();

        // returns the array of pointers pointing to parse
        // nodes representing grantees.  This array contains
        // at least one element.

  inline const QualifiedName & getGrantNameAsQualifiedName() const;
  inline       QualifiedName & getGrantNameAsQualifiedName();   

  inline NAString getObjectName() const;

        // returns the name of the object appearing in the
        // Publish/Unpublish statement

  inline NAString getSynonymName() const;

	// returns the name of the synonym appearing in the
        // publish statement'

  inline NABoolean isSynonymNameSpecified() const;

        // returns TRUE if the AS or FOR option specified a synonym
        // name in the Publish/Unpublish statement; returns FALSE otherwise.

  inline const ElemDDLPrivActArray & getPrivilegeActionArray() const;
  inline       ElemDDLPrivActArray & getPrivilegeActionArray();

        // returns the array of pointer pointing to parse
        // nodes representing privilege actions.  If the All
        // Privileges phrase appears in the Publish/Unpublish statement,
        // the returned array is empty.  The method
        // isAllPrivileges() may be used to find out whether
        // the All Privileges phrase appears or not.
 
  inline NAString getTableName() const;

        // same as getObjectName() - returns the name of the
        // object appearing in the Publish/Unpublish statement

  inline NABoolean isAllPrivilegesSpecified() const;

        // returns TRUE if the All Privileges phrase appears
        // in the Publish/Unpublish statement; returns FALSE otherwise.
        //
        // Note that getPrivilegeActionArray() returns an
        // empty array when the the All Privileges phrase
        // appears in the Publish/Unpublish statement.

  inline NABoolean isRoleListSpecified() const;

        // returns TRUE if publishing to roles
        // returns FALSE if publishing to users

  inline NABoolean isPublish() const;

  // mutator
  virtual void setChild(Lng32 index, ExprNode * pChildNode);

  // for processing
  ExprNode * bindNode(BindWA *bindWAPtr);

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;


private:

  NAString objectName_;
  QualifiedName objectQualName_;

  NAString synonymName_;

  // is synonym name specified?
  NABoolean isSynonymNameSpec_;

  // privilege actions
  NABoolean isAllPrivileges_;
  ElemDDLPrivActArray privActArray_;

  // grantees
  ElemDDLGranteeArray granteeArray_;

  // roles or users?
  NABoolean isRoleList_;

  // publish or unpublish?
  NABoolean isPublish_;

  // pointers to child parse nodes

  enum { PUBLISH_PRIVILEGES = 0,
         PUBLISH_GRANTEE_LIST,
         MAX_STMT_DDL_PUBLISH_ARITY };

  ElemDDLNode * children_[MAX_STMT_DDL_PUBLISH_ARITY];

}; // class StmtDDLPublish

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLPublish
// -----------------------------------------------------------------------

//
// accessors
//
inline QualifiedName &
StmtDDLPublish::getGrantNameAsQualifiedName()
{
  return objectQualName_;
}

inline const QualifiedName & 
StmtDDLPublish::getGrantNameAsQualifiedName() const 
{
  return objectQualName_;
}
inline NAString
StmtDDLPublish::getObjectName() const
{
  return objectName_;
}

inline NAString
StmtDDLPublish::getSynonymName() const
{
  return synonymName_;
}

inline NABoolean
StmtDDLPublish::isSynonymNameSpecified() const
{
  return isSynonymNameSpec_;
}

inline ElemDDLGranteeArray &
StmtDDLPublish::getGranteeArray()
{
  return granteeArray_;
}

inline const ElemDDLGranteeArray &
StmtDDLPublish::getGranteeArray() const
{
  return granteeArray_;
}

inline ElemDDLPrivActArray &
StmtDDLPublish::getPrivilegeActionArray()
{
  return privActArray_;
}

inline const ElemDDLPrivActArray &
StmtDDLPublish::getPrivilegeActionArray() const
{
  return privActArray_;
}

inline NAString
StmtDDLPublish::getTableName() const
{
  return objectName_;
}

inline NABoolean
StmtDDLPublish::isAllPrivilegesSpecified() const
{
  return isAllPrivileges_;
}

inline NABoolean
StmtDDLPublish::isRoleListSpecified() const
{
  return isRoleList_;
}

inline NABoolean
StmtDDLPublish::isPublish() const
{
  return isPublish_;
}

#endif // STMTDDLPUBLISH_H
