/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef STMTDDLALTERTABLEALTERCOLUMNDEFAULTVALUE_H
#define STMTDDLALTERTABLEALTERCOLUMNDEFAULTVALUE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableAlterColumnDefaultValue.h
 * Description:  class for Alter Table <table-name> alter column default 
 *               DDL statements
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.C.
 *
 *
 * Created:      1/07/2008
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

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableAlterColumnDefaultValue
// -----------------------------------------------------------------------
class StmtDDLAlterTableAlterColumnDefaultValue : public StmtDDLAlterTable
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

  StmtDDLAlterTableAlterColumnDefaultValue();   // DO NOT USE
  StmtDDLAlterTableAlterColumnDefaultValue(const StmtDDLAlterTableAlterColumnDefaultValue &);   // DO NOT USE
  StmtDDLAlterTableAlterColumnDefaultValue & operator=(const StmtDDLAlterTableAlterColumnDefaultValue &);  // DO NOT USE


}; // class StmtDDLAlterTableAlterColumnDefaultValue


inline NAString 
StmtDDLAlterTableAlterColumnDefaultValue::getColumnName()
{
  return columnName_;
}
#endif //STMTDDLALTERTABLEALTERCOLUMNDEFAULTVALUE_H
