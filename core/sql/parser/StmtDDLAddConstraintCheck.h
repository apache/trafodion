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
#ifndef STMTDDLADDCONSTRAINTCHECK_H
#define STMTDDLADDCONSTRAINTCHECK_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAddConstraintCheck.h
 * Description:  class for Alter Table <table-name> Add Constraint
 *               statements containing Check clause
 *
 *               The proper name for this class should be
 *               StmtDDLAlterTableAddConstraintCheck because this class
 *               is derived from the base class StmtDDLAlterTable.  The
 *               name StmtDDLAlterTableAddConstraintCheck is too long
 *               however.  So the short name StmtDDLAddConstraintCheck
 *               is used even though it is less clear.
 *
 *               Also contains definitions of classes describing
 *               the column usages information for check constraint.
 *
 *               Non-inline methods of classes defined in this header
 *               file are defined in the source file StmtDDLAlter.C.
 *
 *               
 * Created:      6/15/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComASSERT.h"
#include "ElemDDLConstraintCheck.h"
#include "NAString.h"
#include "ParNameLocList.h"
#include "ParTableUsageList.h"
#include "StmtDDLAlterTable.h"
#include "StmtDDLAddConstraint.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ParCheckConstraintColUsage;
class ParCheckConstraintColUsageList;
class ParCheckConstraintUsages;
class StmtDDLAddConstraintCheck;

// -----------------------------------------------------------------------
// definition of class ParCheckConstraintColUsage
// -----------------------------------------------------------------------
class ParCheckConstraintColUsage : public NABasicObject
{
public:

  // default constructor
  ParCheckConstraintColUsage(CollHeap * h=PARSERHEAP());

  // initialize constructor
  ParCheckConstraintColUsage(const ColRefName &colName,
                             const NABoolean isInSelectList,
                             CollHeap * h=PARSERHEAP());

  // copy ctor
  ParCheckConstraintColUsage (const ParCheckConstraintColUsage &,
                              CollHeap * h=PARSERHEAP()) ; // not written

  // virtual destructor
  virtual ~ParCheckConstraintColUsage();

  //
  // operator
  //

  NABoolean operator==(const ParCheckConstraintColUsage &rhs) const;
  
  //
  // accessors
  //
  
  inline const NAString & getColumnName() const;

        // returns the referenced column name in internal format.

  inline const QualifiedName & getTableQualName() const;

        // returns the referenced table name in QualifedName format.

  inline NABoolean isInSelectList() const;

        // returns TRUE if the referenced column is in a SELECT list;
        // returns FALSE otherwise.

  //
  // mutators
  //

  inline void setColumnName(const NAString &colName);
  inline void setIsInSelectList(const NABoolean setting);
  inline void setTableName(const QualifiedName &tableQualName);

private:
  
  QualifiedName tableName_;
  NAString      columnName_;
  NABoolean     isInSelectList_;
  
}; // class ParCheckConstraintColUsage

// -----------------------------------------------------------------------
// definition of class ParCheckConstraintColUsageList
// -----------------------------------------------------------------------

class ParCheckConstraintColUsageList: private LIST(ParCheckConstraintColUsage *)
{
public:
 
  //
  // constructors
  //

  ParCheckConstraintColUsageList(CollHeap * heap = PARSERHEAP())
  : LIST(ParCheckConstraintColUsage *)(heap),
  heap_(heap)
  { }

  ParCheckConstraintColUsageList(const ParCheckConstraintColUsageList &rhs,
                                 CollHeap * heap = PARSERHEAP());

  //
  // virtual destructor
  //

  virtual ~ParCheckConstraintColUsageList();

  //
  // operators
  //

  ParCheckConstraintColUsageList & operator=
    (const ParCheckConstraintColUsageList &rhs);

  inline const ParCheckConstraintColUsage & operator[](CollIndex index) const;
  inline       ParCheckConstraintColUsage & operator[](CollIndex index);

  //
  // accessors
  //
  
  inline CollIndex entries() const;

  inline const ParCheckConstraintColUsage * const find(const ColRefName &
                                                       colName) const;
  ParCheckConstraintColUsage * const find(const ColRefName &colName);

        // returns the pointer pointing to the ParCheckConstraintColUsage
        // element in the list containing the specified colName;
        // returns the NULL pointer value if not found.

  //
  // mutators
  //
  
  void clear();

        // removes all objects pointed by elements in this list and then
        // removes all elements (pointers) in this list.

  void insert(const ColRefName &columnName, const NABoolean isInSelectList);

        // 1. If columeName is not in the list, inserts it to the end of
        //    the list.
        // 2. If the columnName is already in the list; there are two cases:
        //    a.  If the (existing) field isInSelectList in the element
        //        already contains the TRUE value, does nothing.
        //    b.  Otherwise, update the (existing) field with the value
        //        in the specified parameter isInSelectList.

private:

  void copy(const ParCheckConstraintColUsageList &rhs);

  // heap in which the objects (pointed to by the pointers in this
  // list) are allocated.
  //
  CollHeap * heap_;

}; // class ParCheckConstraintColUsageList

// -----------------------------------------------------------------------
// definition of class ParCheckConstraintUsages
// -----------------------------------------------------------------------
class ParCheckConstraintUsages : public NABasicObject
{
public:

  ParCheckConstraintUsages(CollHeap * heap = PARSERHEAP())
  : tableUsages_(heap),
  columnUsages_(heap)
  { }

  // virtual destructor
  virtual ~ParCheckConstraintUsages();

  //
  // accessors
  //
  
  inline const ParCheckConstraintColUsageList & getColumnUsageList() const;
  inline       ParCheckConstraintColUsageList & getColumnUsageList();
  
        // returns a list of column usages appearing in the search
        // condition of the check constraint definition.

  inline const ParTableUsageList & getTableUsageList() const;
  inline       ParTableUsageList & getTableUsageList();

        // returns a list of table names appearing in the search
        // condition of the check constraint definition.

private:

  //
  // check constraint table usages information
  //

  ParTableUsageList tableUsages_;

  //
  // check constraint column usages information
  //
  
  ParCheckConstraintColUsageList columnUsages_;
  
}; // class ParCheckConstraintUsages

// -----------------------------------------------------------------------
// definition of class StmtDDLAddConstraintCheck
// -----------------------------------------------------------------------
class StmtDDLAddConstraintCheck : public StmtDDLAddConstraint
{

public:

  // constructors
  StmtDDLAddConstraintCheck(ElemDDLNode * pElemDDLConstraintCheck);
  StmtDDLAddConstraintCheck(const QualifiedName & tableQualName,
                            ElemDDLNode * pElemDDLConstraintCheck);

  // virtual destructor
  virtual ~StmtDDLAddConstraintCheck();

  // cast
  virtual StmtDDLAddConstraintCheck * castToStmtDDLAddConstraintCheck();

  //
  // accessors
  //

  ElemDDLConstraintCheck * getElemDDLConstraintCheck() const
  {
    ElemDDLConstraintCheck *elem =
      getAlterTableAction()->castToElemDDLConstraintCheck();
    ComASSERT(elem);
    return elem;
  }

  virtual NABoolean isConstraintNotNull() const
  { return getElemDDLConstraintCheck()->isConstraintNotNull(); }
  
  inline const ParCheckConstraintColUsageList & getColumnUsageList() const;
  inline       ParCheckConstraintColUsageList & getColumnUsageList();

        // returns a list of column usages appearing in the search
        // condition of the check constraint definition.
  
  inline const StringPos getEndPosition() const;

        // returns the ending position (the position of the
        // last character) of the search condition (within
        // the input string)
  
  inline const ParNameLocList & getNameLocList() const;
  inline       ParNameLocList & getNameLocList();

        // returns a list of locations of names appearing in
        // the statement input string.  The list helps with
        // the computing of the view text.

  ItemExpr * getSearchCondition() const;

        // returns the pointer pointing to the parse sub-tree
        // representing the search condition in the check
        // constraint definition.
  
  inline const StringPos getStartPosition() const;

        // returns the starting position (the position of the
        // first character) of the search condition (within the
        // input string)

  inline const ParTableUsageList & getTableUsageList() const;
  inline       ParTableUsageList & getTableUsageList();

        // returns a list of table names appearing in the search
        // condition of the check constraint definition.
  
  //
  // mutators
  //
  
  inline void setEndPosition(const StringPos endPos);

        // sets the ending position (the position of the
        // last character) of the search condition (within
        // the input string)
  
  inline void setStartPosition(const StringPos startPos);

        // sets the starting position (the position of the
        // first character) of the search condition (within
        // the input string)

  //
  // method for binding
  //
  
  virtual ExprNode * bindNode(BindWA * pBindWA);
  
  //
  // method for tracing
  //
  
  virtual const NAString getText() const;


private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  StmtDDLAddConstraintCheck();  // DO NOT USE
  StmtDDLAddConstraintCheck(const NAString & tableName,
                            ElemDDLNode *);                     // DO NOT USE
  StmtDDLAddConstraintCheck(const StmtDDLAddConstraintCheck &); // DO NOT USE
  StmtDDLAddConstraintCheck & operator =
        (const StmtDDLAddConstraintCheck &); //DO NOT USE

  void init(ElemDDLNode * pElemDDLConstraintCheck);
  
        // initialize method invoked by the constructors
  
  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------
  
  //
  // information about the position of the name within the input
  // string (to help with computing the view text)
  //

  ParNameLocList nameLocList_;

  //
  // positions of the create view statement within the input
  // string (to help with computing the view text)
  // 
  
  StringPos startPos_;
  StringPos endPos_;

  //
  // column usages information
  //

  ParCheckConstraintUsages usages_;

}; // class StmtDDLAddConstraintCheck

// -----------------------------------------------------------------------
// definitions of inline methods for class ParCheckConstraintColUsage
// -----------------------------------------------------------------------

//
// accessors
//

inline const NAString &
ParCheckConstraintColUsage::getColumnName() const
{
  return columnName_;
}

inline const QualifiedName &
ParCheckConstraintColUsage::getTableQualName() const
{
  return tableName_;
}

inline NABoolean
ParCheckConstraintColUsage::isInSelectList() const
{
  return isInSelectList_;
}

//
// mutators
//

inline void
ParCheckConstraintColUsage::setColumnName(const NAString &colName)
{
  columnName_ = colName;
}

inline void
ParCheckConstraintColUsage::setIsInSelectList(const NABoolean setting)
{
  isInSelectList_ = setting;
}

inline void
ParCheckConstraintColUsage::setTableName(const QualifiedName &tableQualName)
{
  tableName_ = tableQualName;
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ParCheckConstraintColUsageList
// -----------------------------------------------------------------------
//
// operators
//

inline const ParCheckConstraintColUsage &
ParCheckConstraintColUsageList::operator[](CollIndex index) const
{
  return *(LIST(ParCheckConstraintColUsage *)::operator[](index));
}

inline ParCheckConstraintColUsage &
ParCheckConstraintColUsageList::operator[](CollIndex index)
{
  return *(LIST(ParCheckConstraintColUsage *)::operator[](index));
}

//
// accessors
//

inline CollIndex
ParCheckConstraintColUsageList::entries() const
{
  return LIST(ParCheckConstraintColUsage *)::entries();
}

inline const ParCheckConstraintColUsage * const
ParCheckConstraintColUsageList::find(const ColRefName &colName) const
{
  return ((ParCheckConstraintColUsageList *)this)->find(colName);
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ParCheckConstraintUsages
// -----------------------------------------------------------------------
//
// accessors
//

inline const ParCheckConstraintColUsageList &
ParCheckConstraintUsages::getColumnUsageList() const
{
  return columnUsages_;
}
     
inline ParCheckConstraintColUsageList &
ParCheckConstraintUsages::getColumnUsageList()
{
  return columnUsages_;
}
  
inline const ParTableUsageList &
ParCheckConstraintUsages::getTableUsageList() const
{
  return tableUsages_;
}

inline ParTableUsageList &
ParCheckConstraintUsages::getTableUsageList()
{
  return tableUsages_;
}

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAddConstraint
// -----------------------------------------------------------------------

//
// accessors
//

inline const ParCheckConstraintColUsageList &
StmtDDLAddConstraintCheck::getColumnUsageList() const
{
  return usages_.getColumnUsageList();
}

inline ParCheckConstraintColUsageList &
StmtDDLAddConstraintCheck::getColumnUsageList()
{
  return usages_.getColumnUsageList();
}

inline const StringPos
StmtDDLAddConstraintCheck::getEndPosition() const
{
  return endPos_;
}

inline const ParNameLocList &
StmtDDLAddConstraintCheck::getNameLocList() const
{
  return nameLocList_;
}

inline ParNameLocList &
StmtDDLAddConstraintCheck::getNameLocList()
{
  return nameLocList_;
}

inline const StringPos
StmtDDLAddConstraintCheck::getStartPosition() const
{
  return startPos_;
}

inline const ParTableUsageList &
StmtDDLAddConstraintCheck::getTableUsageList() const
{
  return usages_.getTableUsageList();
}

inline ParTableUsageList &
StmtDDLAddConstraintCheck::getTableUsageList()
{
  return usages_.getTableUsageList();
}

//
// mutators
//

inline void
StmtDDLAddConstraintCheck::setEndPosition(const StringPos endPos)
{
  endPos_ = endPos;
}

inline void
StmtDDLAddConstraintCheck::setStartPosition(const StringPos startPos)
{
  startPos_ = startPos;
}

#endif // STMTDDLADDCONSTRAINTCHECK_H
