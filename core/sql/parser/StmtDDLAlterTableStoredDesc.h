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
#ifndef STMTDDLALTERTABLESTOREDDESC_H
#define STMTDDLALTERTABLESTOREDDESC_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableStoredDesc.h
 * Description:  
 *   Alter Table <table-name> [generate|delete|enable|disable] stored descriptor
 *               DDL statements
 *
 *               
 * Created:      7/26/2016
 * Language:     C++
 *
 *****************************************************************************
 */


#include "StmtDDLAlterTable.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAlterTableStoredDesc;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableStoredDesc
// -----------------------------------------------------------------------
class StmtDDLAlterTableStoredDesc : public StmtDDLAlterTable
{

public:
  enum AlterStoredDescType
    {
      CHECK,
      GENERATE,
      DELETE,
      ENABLE,
      DISABLE
    };

  // constructor
  StmtDDLAlterTableStoredDesc(const AlterStoredDescType type)
  : StmtDDLAlterTable(DDL_ALTER_TABLE_STORED_DESC),
    type_(type)
  {}

  // virtual destructor
  virtual ~StmtDDLAlterTableStoredDesc()
  {}

  // cast
  virtual StmtDDLAlterTableStoredDesc * castToStmtDDLAlterTableStoredDesc()
  { return this; }

  // accessors
  AlterStoredDescType getType() { return type_; }

  // method for tracing
  virtual const NAString getText() const
  {
    return "StmtDDLAlterTableStoredDesc";
  }

private:

  AlterStoredDescType type_;

}; // class StmtDDLAlterTableStoredDesc

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterTableStoredDesc
// -----------------------------------------------------------------------

#endif // STMTDDLALTERTABLESTOREDDESC_H
