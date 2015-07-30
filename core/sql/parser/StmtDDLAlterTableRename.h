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
#ifndef STMTDDLALTERTABLERENAME_H
#define STMTDDLALTERTABLERENAME_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableRename.h
 * Description:  class for Alter Table <table-name> Rename <new-name>
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
class StmtDDLAlterTableRename;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableRename
// -----------------------------------------------------------------------
class StmtDDLAlterTableRename : public StmtDDLAlterTable
{

public:

  // constructor
  StmtDDLAlterTableRename(const NAString & newName,
                          ComBoolean isCascade)
  : StmtDDLAlterTable(DDL_ALTER_TABLE_RENAME),
    newName_(newName, PARSERHEAP()),
    isCascade_(isCascade)
  { }

  // virtual destructor
  virtual ~StmtDDLAlterTableRename();

  // cast
  virtual StmtDDLAlterTableRename * castToStmtDDLAlterTableRename();

  // accessors
  inline const NAString & getNewName() const;
  inline ComBoolean isCascade() const;

  inline const NAString getNewNameAsAnsiString() const;

  // method for tracing
  virtual const NAString getText() const;


private:

  NAString   newName_;
  ComBoolean isCascade_;

}; // class StmtDDLAlterTableRename

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterTableRename
// -----------------------------------------------------------------------

inline
const NAString &
StmtDDLAlterTableRename::getNewName() const
{
  return newName_;
}

inline const NAString 
StmtDDLAlterTableRename::getNewNameAsAnsiString() const
{
  return QualifiedName(newName_).getQualifiedNameAsAnsiString();
}

inline
ComBoolean
StmtDDLAlterTableRename::isCascade() const
{
  return isCascade_;
}

#endif // STMTDDLALTERTABLERENAME_H
