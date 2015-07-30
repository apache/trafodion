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
#ifndef STMTDDLALTERTABLEALTERCOLUMN_H
#define STMTDDLALTERTABLEALTERCOLUMN_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableAlterColumn.h
 * Description:  class for Alter Table <table-name> alter column  
 *               DDL statements (datatype and default clauses)
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.C.
 *
 *
 * Created:     
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
class StmtDDLAlterTableAlterColumnDefaultValue;
class StmtDDLAlterTableAlterColumnDatatype;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableAlterColumn
// -----------------------------------------------------------------------
class StmtDDLAlterTableAlterColumn : public StmtDDLAlterTable
{
public:

  // constructor
  StmtDDLAlterTableAlterColumn( OperatorTypeEnum operatorType
                                , const NAString &columnName
                                , ElemDDLNode *pColDefault
                                , CollHeap    *heap = PARSERHEAP());
  
    // virtual destructor
  virtual ~StmtDDLAlterTableAlterColumn();

  // accessors
  inline NAString getColumnName();

  // method for tracing
  virtual const NAString getText() const;

  
private: 
  // column name
  NAString columnName_;

  //
  // please do not use the following methods
  //

  StmtDDLAlterTableAlterColumn();   // DO NOT USE
  StmtDDLAlterTableAlterColumn(const StmtDDLAlterTableAlterColumn &);   // DO NOT USE
  StmtDDLAlterTableAlterColumn & operator=(const StmtDDLAlterTableAlterColumn &);  // DO NOT USE


}; // class StmtDDLAlterTableAlterColumn


inline NAString 
StmtDDLAlterTableAlterColumn::getColumnName()
{
  return columnName_;
}

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableAlterColumnDefaultValue
// -----------------------------------------------------------------------
class StmtDDLAlterTableAlterColumnDefaultValue : public StmtDDLAlterTableAlterColumn
{
public:

  // constructor
  StmtDDLAlterTableAlterColumnDefaultValue( const NAString &columnName
                                          , ElemDDLNode *pColDefault
                                          , CollHeap    *heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLAlterTableAlterColumnDefaultValue();

  // cast
  virtual StmtDDLAlterTableAlterColumnDefaultValue * castToStmtDDLAlterTableAlterColumnDefaultValue();

  // method for tracing
  virtual const NAString getText() const;
  
private: 

  //
  // please do not use the following methods
  //

  StmtDDLAlterTableAlterColumnDefaultValue();   // DO NOT USE
  StmtDDLAlterTableAlterColumnDefaultValue(const StmtDDLAlterTableAlterColumnDefaultValue &);   // DO NOT USE
  StmtDDLAlterTableAlterColumnDefaultValue & operator=(const StmtDDLAlterTableAlterColumnDefaultValue &);  // DO NOT USE


}; // class StmtDDLAlterTableAlterColumnDefaultValue


// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableAlterColumnDatatype
// -----------------------------------------------------------------------
class StmtDDLAlterTableAlterColumnDatatype : public StmtDDLAlterTableAlterColumn
{
public:

  // constructor
  StmtDDLAlterTableAlterColumnDatatype( const NAString &columnName
                                        , NAType * natype
                                          , CollHeap    *heap = PARSERHEAP());

    // virtual destructor
  virtual ~StmtDDLAlterTableAlterColumnDatatype();

  // cast
  virtual StmtDDLAlterTableAlterColumnDatatype * castToStmtDDLAlterTableAlterColumnDatatype();

  // method for tracing
  virtual const NAString getText() const;

  const NAType * getDatatype() const { return natype_; }
   NAType * getType() { return natype_; }

private: 

  NAType * natype_;
  //
  // please do not use the following methods
  //

  StmtDDLAlterTableAlterColumnDatatype();   // DO NOT USE
  StmtDDLAlterTableAlterColumnDatatype(const StmtDDLAlterTableAlterColumnDatatype &);   // DO NOT USE
  StmtDDLAlterTableAlterColumnDatatype & operator=(const StmtDDLAlterTableAlterColumnDatatype &);  // DO NOT USE


}; // class StmtDDLAlterTableAlterColumnDatatype


#endif //STMTDDLALTERTABLEALTERCOLUMN_H
