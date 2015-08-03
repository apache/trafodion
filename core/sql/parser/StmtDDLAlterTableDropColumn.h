#ifndef STMTDDLALTERTABLEDROPCOLUMN_H
#define STMTDDLALTERTABLEDROPCOLUMN_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableDropColumn.h
 * Description:  class for Alter Table <table-name> Drop Column <column-definition>
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

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAlterTableDropColumn;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableDropColumn
// -----------------------------------------------------------------------
class StmtDDLAlterTableDropColumn : public StmtDDLAlterTable
{
public:

  // constructor
  StmtDDLAlterTableDropColumn(NAString &colName,
			      CollHeap * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLAlterTableDropColumn();

  // cast
  virtual StmtDDLAlterTableDropColumn * castToStmtDDLAlterTableDropColumn();

  // accessors
  const NAString &getColName() const
  {
    return colName_;
  }

  const NABoolean dropIfExists() const { return dropIfExists_; }
  void setDropIfExists(NABoolean v) { dropIfExists_ = v; }

  //
  // please do not use the following methods
  //

  StmtDDLAlterTableDropColumn();   // DO NOT USE
  StmtDDLAlterTableDropColumn(const StmtDDLAlterTableDropColumn &);   // DO NOT USE
  StmtDDLAlterTableDropColumn & operator=(const StmtDDLAlterTableDropColumn &);  // DO NOT USE

private:

  NAString colName_;

  // drop only if column exists. Otherwise just return.
  NABoolean dropIfExists_;

}; // class StmtDDLAlterTableDropColumn

#endif // STMTDDLALTERTABLEDROPCOLUMN_H
