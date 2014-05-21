/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2014 Hewlett-Packard Development Company, L.P.
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
#ifndef STMTDDLALTERTABLEALTERCOLUMNRECALIBRATESGATTRIBUTE_H
#define STMTDDLALTERTABLEALTERCOLUMNRECALIBRATESGATTRIBUTE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableAlterColumnRecalibrateSG.h
 * Description:  class for Alter Table <table-name> alter column <column-name>
 *               recalibrate DDL statements
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.C.
 *
 *
 * Created:      10/13/2009
 * Language:     C++
 *
 *****************************************************************************
 */

#include "StmtDDLAlterTable.h"
#include "NABoolean.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAlterTableAlterColumnRecalibrateSG;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableAlterColumnRecalibrateSG
// -----------------------------------------------------------------------
class StmtDDLAlterTableAlterColumnRecalibrateSG : public StmtDDLAlterTable
{
public:

  // constructor
  StmtDDLAlterTableAlterColumnRecalibrateSG( const NAString &columnName 
                                             , Int64 value 
					     , NABoolean userSpecified
					     , NABoolean performSelect
                                             , CollHeap    *heap = PARSERHEAP());

    // virtual destructor
  virtual ~StmtDDLAlterTableAlterColumnRecalibrateSG();

  // cast
  virtual StmtDDLAlterTableAlterColumnRecalibrateSG * castToStmtDDLAlterTableAlterColumnRecalibrateSG();

  // accessors
  inline NAString getColumnName();

  inline Int64 getValue();

  inline NABoolean userSpecified();

  inline NABoolean performSelect();

  // method for tracing
  virtual const NAString getText() const;
  
private: 
  // column name
  NAString columnName_;

  // value
  Int64 value_;

  // Is this a user specified value
  NABoolean userSpecified_;

  // Should the select against the maximum of
  // the IDENTITY column in the table be performed

  NABoolean performSelect_;
 
  //
  // please do not use the following methods
  //

  StmtDDLAlterTableAlterColumnRecalibrateSG();   // DO NOT USE
  StmtDDLAlterTableAlterColumnRecalibrateSG(const StmtDDLAlterTableAlterColumnRecalibrateSG &);   // DO NOT USE
  StmtDDLAlterTableAlterColumnRecalibrateSG & operator=(const StmtDDLAlterTableAlterColumnRecalibrateSG &);  // DO NOT USE


}; // class StmtDDLAlterTableAlterColumnRecalibrateSG


inline NAString 
StmtDDLAlterTableAlterColumnRecalibrateSG::getColumnName()
{
  return columnName_;
}

inline Int64 
StmtDDLAlterTableAlterColumnRecalibrateSG::getValue()
{
  return value_;
}

inline NABoolean 
StmtDDLAlterTableAlterColumnRecalibrateSG::userSpecified()
{
  return userSpecified_;
}

inline NABoolean 
StmtDDLAlterTableAlterColumnRecalibrateSG::performSelect()
{
  return performSelect_;
}



#endif //STMTDDLALTERTABLEALTERCOLUMNRECALIBRATESGATTRIBUTE_H
