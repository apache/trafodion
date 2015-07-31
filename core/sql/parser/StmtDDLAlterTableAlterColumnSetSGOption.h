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
#ifndef STMTDDLALTERTABLEALTERCOLUMNSETSGATTRIBUTE_H
#define STMTDDLALTERTABLEALTERCOLUMNSETSGATTRIBUTE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableAlterColumnSetSGOption.h
 * Description:  class for Alter Table <table-name> alter column <column-name>
 *               set SG attribute DDL statements
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.C.
 *
 *
 * Created:      11/04/2008
 * Language:     C++
 *
 *****************************************************************************
 */

#include "StmtDDLAlterTable.h"
#include "ElemDDLSGOptions.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAlterTableAlterColumnSetSGOption;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableAlterColumnSetSGOption
// -----------------------------------------------------------------------
class StmtDDLAlterTableAlterColumnSetSGOption : public StmtDDLAlterTable
{
public:

  // constructor
  StmtDDLAlterTableAlterColumnSetSGOption( const NAString &columnName
                                          , ElemDDLSGOptions *pSGOptions
                                          , CollHeap    *heap = PARSERHEAP());

    // virtual destructor
  virtual ~StmtDDLAlterTableAlterColumnSetSGOption();

  // cast
  virtual StmtDDLAlterTableAlterColumnSetSGOption * castToStmtDDLAlterTableAlterColumnSetSGOption();

  // accessors
  inline NAString getColumnName();

  inline const ElemDDLSGOptions* getSGOptions() const;
  inline       ElemDDLSGOptions* getSGOptions();
  inline       void setSGOptions(ElemDDLSGOptions* pSGOptions);

  // mutators
  void setIncrement(Int64 increment);
  void setMaxValue(Int64 maxValue);

  // method for tracing
  virtual const NAString getText() const;
  
private: 
  // column name
  NAString columnName_;
  ElemDDLSGOptions * pSGOptions_;

  //
  // please do not use the following methods
  //

  StmtDDLAlterTableAlterColumnSetSGOption();   // DO NOT USE
  StmtDDLAlterTableAlterColumnSetSGOption(const StmtDDLAlterTableAlterColumnSetSGOption &);   // DO NOT USE
  StmtDDLAlterTableAlterColumnSetSGOption & operator=(const StmtDDLAlterTableAlterColumnSetSGOption &);  // DO NOT USE


}; // class StmtDDLAlterTableAlterColumnSetSGOption


inline NAString 
StmtDDLAlterTableAlterColumnSetSGOption::getColumnName()
{
  return columnName_;
}

inline const ElemDDLSGOptions * StmtDDLAlterTableAlterColumnSetSGOption::getSGOptions() const 
{
  return pSGOptions_;
}

inline ElemDDLSGOptions * StmtDDLAlterTableAlterColumnSetSGOption::getSGOptions() 
{
  return pSGOptions_;
}

inline void StmtDDLAlterTableAlterColumnSetSGOption::setSGOptions(ElemDDLSGOptions *pSGOptions) 
{
  pSGOptions_ = pSGOptions;
}
#endif //STMTDDLALTERTABLEALTERCOLUMNSETSGATTRIBUTE_H
