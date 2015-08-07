#ifndef ELEMDDLDIVISIONCLAUSE_H
#define ELEMDDLDIVISIONCLAUSE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLDivisionClause.h
 * Description:  Classes representing Division clause specified in
 *               Create Table/Index/MV DDL statements
 *
 *               
 * Created:      7/12/2011
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

#include "ComSmallDefs.h"
#include "ElemDDLNode.h"
#include "ParNameLocList.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLDivisionClause;

// -----------------------------------------------------------------------
// class ElemDDLDivisionClause 
// -----------------------------------------------------------------------

class ElemDDLDivisionClause : public ElemDDLNode
{

public:

  enum divisionTypeEnum { UNKNOWN_DIVISION_TYPE,
                          DIVISION_BY_EXPR_LIST,
                          DIVISION_LIKE_TABLE };

  //
  // constructors
  //

  ElemDDLDivisionClause(ItemExpr * pDivisionExprTree);
  ElemDDLDivisionClause(divisionTypeEnum eDivisionType);

  // virtual destructor
  // The destructor does not destroy the parse tree pointed by pDivisionExprList_
  virtual ~ElemDDLDivisionClause();

  // casting
  virtual ElemDDLDivisionClause * castToElemDDLDivisionClause();

  //
  // accessors
  //

  inline divisionTypeEnum getDivisionType() const {return eDivisionType_; }

  inline ItemExprList * getDivisionExprList()
  {
    return pDivisionExprList_;
  }

  inline ElemDDLColRefArray & getDivisionColRefArray()
  {
    return columnRefArray_;
  }

  inline ItemExpr * getDivisionExprTree()
  {
    return pDivisionExprTree_;
  }

  inline ElemDDLNode * getColumnRefList()
  {
    return pColumnRefList_;
  }

  inline ParNameLocList & getNameLocList()
  {
    return nameLocList_;
  }

  inline StringPos getStartPosition() const
  {
    return startPos_;
  }

  inline StringPos getEndPosition() const
  {
    return endPos_;
  }

  //
  // mutators
  //

  inline void setNameLocList(ParNameLocList & rhs) { nameLocList_ = rhs; }
  inline void setStartPosition(StringPos beginPos) { startPos_ = beginPos; }
  inline void setEndPosition(StringPos endPos) { endPos_ = endPos; }
  inline void setColumnRefList(ElemDDLNode * pColRefTree) { pColumnRefList_ = pColRefTree; }

  //
  // methods for tracing and/or building text
  //

  virtual const NAString getText() const;
  virtual NAString getSyntax() const;

  //
  // helpers
  //

  // Gather the information from the child sub-tree and store it in this Parse node
  void synthesize(ElemDDLNode * pColRefTree);

  // Does the number of division expressions in the DIVISION BY ( ... ) clause match
  // the number of columns specified in the associating COLUMN NAME[S] ( ... ) clause
  NABoolean isNumOfDivExprsAndColsMatched() const;

private:

  //
  // Private methods
  //

  ElemDDLDivisionClause(const ElemDDLDivisionClause & rhs) ; // not defined - DO NOT USE
  ElemDDLDivisionClause & operator=(const ElemDDLDivisionClause & rhs) ; // not defined - DO NOT USE

  //
  // data members
  //

  divisionTypeEnum eDivisionType_;
  ItemExpr * pDivisionExprTree_;
  ItemExprList * pDivisionExprList_;
  ElemDDLNode * pColumnRefList_;
  ElemDDLColRefArray columnRefArray_;

  // start and end positions of the division by clause text.
  ParNameLocList nameLocList_;
  StringPos startPos_;
  StringPos endPos_;

}; // class ElemDDLDivisionClause

#endif // ELEMDDLDIVISIONCLAUSE_H
