#ifndef ELEMDDLCONSTRAINTCHECK_H
#define ELEMDDLCONSTRAINTCHECK_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLConstraintCheck.h
 * Description:  class for Check constraint definitions in DDL statements
 *
 *               
 * Created:      3/29/95
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


#include "ElemDDLConstraint.h"
#include "ItemExprList.h"
#include "ParNameLocList.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLConstraintCheck;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class ItemExpr;

// -----------------------------------------------------------------------
// definition of class ElemDDLConstraintCheck
// -----------------------------------------------------------------------
class ElemDDLConstraintCheck : public ElemDDLConstraint
{

public:

  // initialize constructor
  ElemDDLConstraintCheck(ItemExpr * pSearchCondition,
                         const ParNameLocList &nameLocList,
                         CollHeap * heap = PARSERHEAP());

  // virtual destructor
  virtual ~ElemDDLConstraintCheck();

  // cast
  virtual ElemDDLConstraintCheck * castToElemDDLConstraintCheck();
  virtual NABoolean isConstraintNotNull() const;
  NABoolean getColumnsNotNull(ItemExprList &);

  //
  // accessors
  //
  
  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline StringPos getEndPosition() const;

  inline const ParNameLocList & getNameLocList() const;
  inline       ParNameLocList & getNameLocList();

  inline ItemExpr * getSearchCondition() const;

  inline StringPos getStartPosition() const;

  //
  // mutators
  //
  
  virtual void setChild(Lng32 index, ExprNode * pChildNode);
  inline  void setEndPosition(const StringPos endPos);
  inline  void setStartPosition(const StringPos startPos);

  //
  // methods for tracing
  //
  
  // displayLabel1() is defined in the base class ElemDDLConstraint
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;


private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  ElemDDLConstraintCheck();  // DO NOT USE
  ElemDDLConstraintCheck(const ElemDDLConstraintCheck &); //DO NOT USE
  ElemDDLConstraintCheck & operator =
        (const ElemDDLConstraintCheck &); //DO NOT USE

  // ---------------------------------------------------------------------
  // private data member
  // ---------------------------------------------------------------------
  
  //
  // information about the position of the name within the input
  // string (to help with computing the check constraint search
  // condition text)
  //

  ParNameLocList nameLocList_;

  //
  // positions of the check constraint search condition within
  // the input string (to help with computing the text)
  // 
  
  StringPos startPos_;
  StringPos endPos_;

  //
  // pointer to child parse node
  //

  enum { INDEX_SEARCH_CONDITION = MAX_ELEM_DDL_CONSTRAINT_ARITY,
         MAX_ELEM_DDL_CONSTRAINT_CHECK_ARITY };

  ItemExpr * searchCondition_;
  
}; // class ElemDDLConstraintCheck

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLConstraintCheck
// -----------------------------------------------------------------------

//
// accessors
//

inline StringPos
ElemDDLConstraintCheck::getEndPosition() const
{
  return endPos_;
}

inline const ParNameLocList &
ElemDDLConstraintCheck::getNameLocList() const
{
  return nameLocList_;
}

inline ParNameLocList &
ElemDDLConstraintCheck::getNameLocList()
{
  return nameLocList_;
}

inline ItemExpr *
ElemDDLConstraintCheck::getSearchCondition() const
{
  return searchCondition_;
}

inline StringPos
ElemDDLConstraintCheck::getStartPosition() const
{
  return startPos_;
}

//
// mutators
//

inline void
ElemDDLConstraintCheck::setEndPosition(const StringPos endPos)
{
  endPos_ = endPos;
}

inline void
ElemDDLConstraintCheck::setStartPosition(const StringPos startPos)
{
  startPos_ = startPos;
}

#endif // ELEMDDLCONSTRAINTCHECK_H
