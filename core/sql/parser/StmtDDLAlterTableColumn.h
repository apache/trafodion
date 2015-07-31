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
#ifndef STMTDDLALTERTABLECOLUMN_H
#define STMTDDLALTERTABLECOLUMN_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableColumn.h
 * Description:  class for Alter Table <table-name> Column <column-name>
 *               DDL statements
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.C.
 *
 *               
 * Created:      9/19/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "StmtDDLAlterTable.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAlterTableColumn;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableColumn
// -----------------------------------------------------------------------
class StmtDDLAlterTableColumn : public StmtDDLAlterTable
{

public:

  // constructor
  StmtDDLAlterTableColumn(const NAString & columnName,
                                 ElemDDLNode *pColumnHeadingNode,
                                 CollHeap * h=0)
  : StmtDDLAlterTable(DDL_ALTER_TABLE_COLUMN,
                    QualifiedName(h) /*no table name*/,
                    pColumnHeadingNode),
  columnName_(columnName, h)
  { }


  // copy ctor
  StmtDDLAlterTableColumn (const StmtDDLAlterTableColumn & orig,
                           CollHeap * h=0) ;

  // virtual destructor
  virtual ~StmtDDLAlterTableColumn();

  // cast
  virtual StmtDDLAlterTableColumn * castToStmtDDLAlterTableColumn();

  // accessors
  inline ElemDDLColHeading * getColumnHeading() const;
  inline const NAString & getColumnName() const;

  // method for tracing
  virtual const NAString getText() const;


private:

  NAString columnName_;

}; // class StmtDDLAlterTableColumn

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterTableColumn
// -----------------------------------------------------------------------

inline
ElemDDLColHeading *
StmtDDLAlterTableColumn::getColumnHeading() const
{
  return getAlterTableAction()->castToElemDDLColHeading();
}

inline
const NAString &
StmtDDLAlterTableColumn::getColumnName() const
{
  return columnName_;
}

#endif // STMTDDLALTERTABLECOLUMN_H
