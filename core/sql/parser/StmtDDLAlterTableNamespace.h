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
#ifndef STMTDDLALTERTABLENAMESPACE_H
#define STMTDDLALTERTABLENAMESPACE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableNamespace.h
 * Description:  class for Alter Table <table-name> namespace 
 *               DDL statements
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.C.
 *
 *               
 * Created:      5/16/2007
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
class StmtDDLAlterTableNamespace;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableNamespace
// -----------------------------------------------------------------------
class StmtDDLAlterTableNamespace : public StmtDDLAlterTable
{

public:

  // constructor
  StmtDDLAlterTableNamespace(const QualifiedName & objectName)
  : StmtDDLAlterTable(DDL_ALTER_TABLE_NAMESPACE),
    objName_(objectName, PARSERHEAP())
  { }

  // virtual destructor
  virtual ~StmtDDLAlterTableNamespace();

  // cast
  virtual StmtDDLAlterTableNamespace * castToStmtDDLAlterTableNamespace();

  // accessors
  inline const QualifiedName & getObjName() const;

  // method for tracing
  virtual const NAString getText() const;


private:

  QualifiedName   objName_;

}; // class StmtDDLAlterTableNamespace

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterTableNamespace
// -----------------------------------------------------------------------

inline
const QualifiedName &
StmtDDLAlterTableNamespace::getObjName() const
{
  return objName_;
}

#endif // STMTDDLALTERTABLENAMESPACE_H
