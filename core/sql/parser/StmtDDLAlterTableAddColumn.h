#ifndef STMTDDLALTERTABLEADDCOLUMN_H
#define STMTDDLALTERTABLEADDCOLUMN_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableAddColumn.h
 * Description:  class for Alter Table <table-name> Add Column <column-definition>
 *               DDL statements
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.C.
 *
 *               
 * Created:      1/27/98
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

// -----------------------------------------------------------------------
// Change history:
// 
// -----------------------------------------------------------------------

#include "StmtDDLAlterTable.h"
#include "ElemDDLColDefArray.h"
#include "StmtDDLAddConstraintArray.h"
#include "StmtDDLAddConstraintCheckArray.h"
#include "StmtDDLAddConstraintRIArray.h"
#include "StmtDDLAddConstraintUniqueArray.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAlterTableAddColumn;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableAddColumn
// -----------------------------------------------------------------------
class StmtDDLAlterTableAddColumn : public StmtDDLAlterTable
{
  //
  // The following public global function helps to improve
  // performance.  It is passed to the method traverseList
  // of the class ElemDDLList.  For information, please read
  // the descriptions of the method ElemDDLList::traverseList.
  //

  // Visit an element in a left linear tree list.  Each element
  // represents a column constraint definition.
  //
  // pNode contains a pointer pointing to an Alter Table Add Column
  //   parse node (this node).
  // index contains the index of the element in the list.  Each
  //   element is a leaf node in the left linear tree representing
  //   the list.
  // pElement contains a pointer pointing to the element in the
  //   list.
  //
  // All parameters are input parameters passed by the method
  // traverseList of class ElemDDLList (or a class derived from
  // class ElemDDLList).
  //
  // StmtDDLAlterTableAddColumn_visit is a global function
  // defined in StmtDDLAlter.cpp
  //
  friend void StmtDDLAlterTableAddColumn_visit(
       ElemDDLNode * pThisNode,
       CollIndex index,
       ElemDDLNode * pElement);

public:

  // constructor
  StmtDDLAlterTableAddColumn(ElemDDLNode * pColumnDefinition
                            ,CollHeap    * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLAlterTableAddColumn();

  // cast
  virtual StmtDDLAlterTableAddColumn * castToStmtDDLAlterTableAddColumn();

  // accessors
  inline ElemDDLNode * getColToAdd();
  inline ElemDDLColDefArray & getColDefArray();
  inline StmtDDLAddConstraintCheckArray & getAddConstraintCheckArray();
  inline StmtDDLAddConstraintRIArray & getAddConstraintRIArray();
  inline StmtDDLAddConstraintUniqueArray & getAddConstraintUniqueArray();
  inline StmtDDLAddConstraintPK * getAddConstraintPK() const;

  NABoolean addIfNotExists() { return addIfNotExists_; }
  void setAddIfNotExists(NABoolean v) { addIfNotExists_ = v; }

  // ---------------------------------------------------------------------
  // mutators
  // ---------------------------------------------------------------------
 
  // This method collects information in the parse sub-tree and copies it
  // to the current parse node.
  void synthesize();
  void setConstraint(ElemDDLNode * pConstraint);

  // method for tracing
  virtual const NAString getText() const;

  //
  // please do not use the following methods
  //

  StmtDDLAlterTableAddColumn();   // DO NOT USE
  StmtDDLAlterTableAddColumn(const StmtDDLAlterTableAddColumn &);   // DO NOT USE
  StmtDDLAlterTableAddColumn & operator=(const StmtDDLAlterTableAddColumn &);  // DO NOT USE

private:

  // column definition
  ElemDDLNode * pColumnToAdd_;

  // list of (only one) column definition
  ElemDDLColDefArray columnDefArray_;

  //
  // Each element of the following arrays is a pointer
  // pointing to a StmtDDLAddConstraint parse node in
  // the parse sub-tree containing the list of constraints.
  // Class StmtDDLAddConstraint represents
  // Alter Table <table-name> Add Constraint statements.
  //
  // In order to easily reuse the existing functions for 
  // adding constraints, this class mimics class
  // StmtDDLCreateTable in creating a kludge
  // parse node derived from class StmtDDLAddConstraint for
  // each column constraint definition specified for the
  // column being added.
  //
  StmtDDLAddConstraintCheckArray  addConstraintCheckArray_;
  StmtDDLAddConstraintRIArray     addConstraintRIArray_;
  StmtDDLAddConstraintUniqueArray addConstraintUniqueArray_;

  //
  // Since the kludge parge nodes (see above) that are 
  // used for the column constraints  are not part of the 
  // Alter Table Add Column parse tree, we need to save
  // pointers to them in the following array so that we
  // can delete them when we invoke the destructor to destroy
  // Alter Table Add Column parse node.  For more information, 
  // please read the content of the header file StmtDDLAlterTable.h.
  //
  StmtDDLAddConstraintArray addConstraintArray_;

  // PRIMARY KEY clause
  //
  StmtDDLAddConstraintPK * pAddConstraintPK_;

  // add only if column doesnt exist. Otherwise just return.
  NABoolean addIfNotExists_;

}; // class StmtDDLAlterTableAddColumn

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterTableAddColumn
// -----------------------------------------------------------------------

inline ElemDDLNode *
StmtDDLAlterTableAddColumn::getColToAdd()
{
  return pColumnToAdd_;
}

inline ElemDDLColDefArray &
StmtDDLAlterTableAddColumn::getColDefArray()
{
  return columnDefArray_;
}
inline StmtDDLAddConstraintCheckArray &
StmtDDLAlterTableAddColumn::getAddConstraintCheckArray()
{
  return addConstraintCheckArray_;
}

inline StmtDDLAddConstraintRIArray &
StmtDDLAlterTableAddColumn::getAddConstraintRIArray()
{
  return addConstraintRIArray_;
}

inline StmtDDLAddConstraintUniqueArray &
StmtDDLAlterTableAddColumn::getAddConstraintUniqueArray()
{
  return addConstraintUniqueArray_;
}

inline StmtDDLAddConstraintPK *
StmtDDLAlterTableAddColumn::getAddConstraintPK() const
{
  return pAddConstraintPK_;
}
#endif // STMTDDLALTERTABLEADDCOLUMN_H
