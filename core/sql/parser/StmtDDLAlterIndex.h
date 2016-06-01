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
#ifndef STMTDDLALTERINDEX_H
#define STMTDDLALTERINDEX_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterIndex.h
 * Description:  base class for Alter Index statements
 *
 *
 * Created:      1/31/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAlterIndex;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create Catalog statement
// -----------------------------------------------------------------------
class StmtDDLAlterIndex : public StmtDDLNode
{

public:

  // constructors
  inline StmtDDLAlterIndex();
  inline StmtDDLAlterIndex(OperatorTypeEnum operatorType);
  StmtDDLAlterIndex(OperatorTypeEnum operatorType,
                           ElemDDLNode * pAlterIndexAction);
  inline StmtDDLAlterIndex(OperatorTypeEnum operatorType,
                           const QualifiedName & indexName,
                           ElemDDLNode * pAlterIndexAction);

  // virtual destructor
  virtual ~StmtDDLAlterIndex();

  // cast
  virtual StmtDDLAlterIndex * castToStmtDDLAlterIndex();

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline ElemDDLNode * getAlterIndexAction() const;
  inline const NAString & getIndexName() const;

  inline const QualifiedName & getIndexNameAsQualifiedName() const;
  inline       QualifiedName & getIndexNameAsQualifiedName();

  //
  // mutators
  //

  virtual void setChild(Lng32 index, ExprNode * pChildNode);
  virtual void setIndexName(const QualifiedName & indexName);

  // methods for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;

  // method for binding 
  ExprNode * bindNode(BindWA *bindWAPtr);

private:

  NAString indexName_;
  QualifiedName indexQualName_;
  // pointer to child parse node

  enum { INDEX_ALTER_INDEX_ACTION = 0,
         MAX_STMT_DDL_ALTER_INDEX_ARITY };

  ElemDDLNode * alterIndexAction_;

}; // class StmtDDLAlterIndex

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterIndex
// -----------------------------------------------------------------------

//
// constructors in StmtDDLAlter.cpp
//


//
// accessors
//

inline QualifiedName &
StmtDDLAlterIndex::getIndexNameAsQualifiedName()
{
  return indexQualName_;
}

inline const QualifiedName & 
StmtDDLAlterIndex::getIndexNameAsQualifiedName() const 
{
  return indexQualName_;
}

inline ElemDDLNode *
StmtDDLAlterIndex::getAlterIndexAction() const
{
  return alterIndexAction_;
}

inline const NAString &
StmtDDLAlterIndex::getIndexName() const
{
  return indexName_;
}

#endif // STMTDDLALTERINDEX_H
