#ifndef STMTDDLDROPCONSTRAINT_H
#define STMTDDLDROPCONSTRAINT_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLDropConstraint.h
 * Description:  class for parse node representing Drop Alias statements
 *
 *
 * Created:      04/23/96
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


#include "ComSmallDefs.h"
#include "StmtDDLAlterTable.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLDropConstraint;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Drop Constraint statement
// -----------------------------------------------------------------------
class StmtDDLDropConstraint : public StmtDDLAlterTable
{

public:

  // constructor
  StmtDDLDropConstraint(const QualifiedName & constraintQualifiedName,
                        ComDropBehavior dropBehavior);

  // virtual destructor
  virtual ~StmtDDLDropConstraint();

  // cast
  virtual StmtDDLDropConstraint * castToStmtDDLDropConstraint();

  // accessors

  const NAString getConstraintName() const;
  inline ComDropBehavior getDropBehavior() const ;

  inline const QualifiedName & getConstraintNameAsQualifiedName() const;
  inline       QualifiedName & getConstraintNameAsQualifiedName();

  // for tracing
  virtual const NAString displayLabel2() const;
  virtual const NAString displayLabel3() const;
  virtual const NAString getText() const;

  // method for binding
  virtual ExprNode * bindNode(BindWA * pBindWA);


private:

  QualifiedName constraintQualName_;
  ComDropBehavior dropBehavior_;

}; // class StmtDDLDropConstraint

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLDropConstraint
// -----------------------------------------------------------------------

//
// accessors
//
inline QualifiedName &
StmtDDLDropConstraint::getConstraintNameAsQualifiedName()
{
  return constraintQualName_;
}

inline const QualifiedName &
StmtDDLDropConstraint::getConstraintNameAsQualifiedName() const
{
  return constraintQualName_;
}

inline ComDropBehavior
StmtDDLDropConstraint::getDropBehavior() const
{
  return dropBehavior_;
}


#endif // STMTDDLDROPCONSTRAINT_H






