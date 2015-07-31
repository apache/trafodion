#ifndef STMTDDLADDCONSTRAINTPK_H
#define STMTDDLADDCONSTRAINTPK_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAddConstraintPK.h
 * Description:  class for Alter Table <table-name> Add Constraint
 *               statements containing Primary Key clause
 *
 *               The proper name for this class should be
 *               StmtDDLAlterTableAddConstraintPK because this class
 *               is derived from the base class StmtDDLAlterTable.  The
 *               name StmtDDLAlterTableAddConstraintPK is too long
 *               however.  So the short name StmtDDLAddConstraintPK
 *               is used even though it is less clear.
 *
 *               Please note that class StmtDDLAddConstraintPK is
 *               derived directly from class StmtDDLAddConstraintUnique.
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.C.
 *
 *               
 * Created:      6/21/95
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


#include "StmtDDLAlterTable.h"
#include "StmtDDLAddConstraint.h"
#include "StmtDDLAddConstraintUnique.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAddConstraintPK;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAddConstraintPK
// -----------------------------------------------------------------------
class StmtDDLAddConstraintPK : public StmtDDLAddConstraintUnique
{

public:

  // constructors
  StmtDDLAddConstraintPK(ElemDDLNode         * pElemDDLConstraintPK,
                         const NABoolean       isAlwaysDroppable = FALSE);
  StmtDDLAddConstraintPK(const QualifiedName & tableQualName,
                         ElemDDLNode         * pElemDDLConstraintPK,
                         const NABoolean       isAlwaysDroppable = FALSE);

  // virtual destructor
  virtual ~StmtDDLAddConstraintPK();

  // cast
  virtual StmtDDLAddConstraintPK * castToStmtDDLAddConstraintPK();

  // accessor
  inline const NABoolean isAlwaysDroppable() const;

  // mutator
  inline void setIsAlwaysDroppable(const NABoolean flag);

  // method for tracing
  virtual const NAString getText() const;


private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  StmtDDLAddConstraintPK();                                 // DO NOT USE
  StmtDDLAddConstraintPK(const NAString & tableName,
                         ElemDDLNode *);                    // DO NOT USE
  StmtDDLAddConstraintPK(const StmtDDLAddConstraintPK &);   // DO NOT USE
  StmtDDLAddConstraintPK & operator=
        (const StmtDDLAddConstraintPK &);                   // DO NOT USE
  
  // ---------------------------------------------------------------------
  // private data member
  // ---------------------------------------------------------------------

  // Primary Key constraint created in an ALTER TABLE <table-name>
  // ADD CONSTRAINT statement (specified by the user) is always droppable
  // regardless of the value in the PRIMARY_KEY_CONSTRAINT_DROPPABLE_OPTION
  // in DEFAULTS table.
  NABoolean isAlwaysDroppable_;

}; // class StmtDDLAddConstraintPK

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAddConstraint
// -----------------------------------------------------------------------

// accessor
inline const NABoolean
StmtDDLAddConstraintPK::isAlwaysDroppable() const
{
  return isAlwaysDroppable_;
}

// mutator
inline void
StmtDDLAddConstraintPK::setIsAlwaysDroppable(const NABoolean flag)
{
  isAlwaysDroppable_ = flag;
}

#endif // STMTDDLADDCONSTRAINTPK_H
