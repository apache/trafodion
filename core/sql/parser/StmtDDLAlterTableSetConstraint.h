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
#ifndef STMTDDLALTERTABLESETCONSTRAINT_H
#define STMTDDLALTERTABLESETCONSTRAINT_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableSetConstraint.h
 * Description:  class for Alter Table <table-name> Set Constraint ...
 *               DDL statements
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.C.
 *
 *               
 * Created:      9/20/95
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
class StmtDDLAlterTableSetConstraint;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableSetConstraint
// -----------------------------------------------------------------------
class StmtDDLAlterTableSetConstraint : public StmtDDLAlterTable
{

public:

  // constructor
  StmtDDLAlterTableSetConstraint(ElemDDLNode * constraintNameList,
                                        NABoolean setting)
  : StmtDDLAlterTable(DDL_ALTER_TABLE_SET_CONSTRAINT,
                    constraintNameList),
  setting_(setting)        
  { }

  // virtual destructor
  virtual ~StmtDDLAlterTableSetConstraint();

  // cast
  virtual StmtDDLAlterTableSetConstraint *
    castToStmtDDLAlterTableSetConstraint();

  // accessors
  inline ElemDDLNode * getConstraintNameList() const;
  inline NABoolean getConstraintSetting() const;

  // method for tracing
  virtual const NAString getText() const;


private:

  NABoolean setting_;

}; // class StmtDDLAlterTableSetConstraint

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterTableSetConstraint
// -----------------------------------------------------------------------


//
// accessor
//

inline ElemDDLNode *
StmtDDLAlterTableSetConstraint::getConstraintNameList() const
{
  return getAlterTableAction();
}

inline NABoolean
StmtDDLAlterTableSetConstraint::getConstraintSetting() const
{
  return setting_;
}

#endif // STMTDDLALTERTABLESETCONSTRAINT_H
