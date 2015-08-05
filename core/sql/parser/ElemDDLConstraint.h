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
#ifndef ELEMDDLCONSTRAINT_H
#define ELEMDDLCONSTRAINT_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLConstraint.h
 * Description:  base class for (generic) constraint definitions in
 *               DDL statements
 *
 *               
 * Created:      3/29/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"
#include "ObjectNames.h"
#ifndef SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif

#include "SqlParserGlobals.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLConstraint;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLConstraint
// -----------------------------------------------------------------------
class ElemDDLConstraint : public ElemDDLNode
{

public:

  enum constraintKindEnum { COLUMN_CONSTRAINT_DEF,
                            TABLE_CONSTRAINT_DEF };

  enum droppableClauseInfo { DROPPABLE_CLAUSE_NOT_SPECIFIED
                           , DROPPABLE_SPECIFIED_EXPLICITLY
                           , NOT_DROPPABLE_SPECIFIED_EXPLICITLY
                           };

  // default constructor
  ElemDDLConstraint(
       CollHeap * h=0,
       OperatorTypeEnum operType = ELM_ANY_CONSTRAINT_ELEM,
       const QualifiedName & constraintName = QualifiedName(PARSERHEAP()),
       constraintKindEnum constraintKind = COLUMN_CONSTRAINT_DEF,
       ElemDDLNode * pConstraintAttributes = NULL,
       NABoolean isDeferrable = FALSE,
       NABoolean isDroppable = TRUE,
       NABoolean isEnforced = TRUE)
       : ElemDDLNode(operType),
          constraintQualName_(constraintName, h),
          constraintKind_(constraintKind),
          isDeferrable_(isDeferrable),
          isDroppable_(isDroppable),
	  isEnforced_(isEnforced),
          droppableClauseInfo_(DROPPABLE_CLAUSE_NOT_SPECIFIED)
{
  setChild(INDEX_CONSTRAINT_ATTRIBUTES, pConstraintAttributes);
}

  // copy ctor
  ElemDDLConstraint (const ElemDDLConstraint & orig, CollHeap * h=0) ; // not written

  // virtual destructor
  virtual ~ElemDDLConstraint();

  // cast
  virtual ElemDDLConstraint * castToElemDDLConstraint();

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline constraintKindEnum getConstraintKind() const;

        // returns an enumerated constraint to tell whether
        // the constraint is a column or table constraint.

  inline       NAString getConstraintName() const;

  inline const QualifiedName & getConstraintNameAsQualifiedName() const;
  inline       QualifiedName & getConstraintNameAsQualifiedName();

  inline NABoolean isDeferrable() const;
  inline NABoolean isDroppable() const;
  inline NABoolean isEnforced() const;
  
  inline NABoolean isDroppableSpecifiedExplicitly() const;

        // returns TRUE if the user specified the DROPPABLE clause
        // explicitly; returns FALSE otherwise (including the case
        // when the user specified the NOT DROPPABLE clause explicitly).

  inline NABoolean isNotDroppableSpecifiedExplicitly() const;

        // returns TRUE if the user specified the NOT DROPPABLE clause
        // explicitly; returns FALSE otherwise (including the case
        // when the user specified the DROPPABLE clause explicitly).

  inline NABoolean anyDroppableClauseSpecifiedExplicitly() const;

  //
  // mutators
  //
  
  virtual void setChild(Lng32 index, ExprNode * pChildNode);
  void setConstraintAttributes(ElemDDLNode * pConstraintAttributes);
  inline void setConstraintKind(constraintKindEnum constraintKind);
  inline void setConstraintName(const QualifiedName & constraintName);
  void setDroppableFlag(const NABoolean setting);
  void setEnforcedFlag(const NABoolean setting);

  //
  // methods for tracing
  //
  
  virtual const NAString displayLabel1() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;


protected:

  //
  // enumerated constant relating to the child parse node
  // and the arity of this parse node
  //

  enum { INDEX_CONSTRAINT_ATTRIBUTES = 0,
         MAX_ELEM_DDL_CONSTRAINT_ARITY };


private:

  void setConstraintName(const NAString &);  // DO NOT USE

  constraintKindEnum constraintKind_;

  QualifiedName constraintQualName_;

  //
  // Constraint Attributes
  //

  NABoolean isDeferrable_;
  NABoolean isDroppable_;
  NABoolean isEnforced_;

  droppableClauseInfo droppableClauseInfo_;

  //
  // pointer to child parse node
  //
  
  ElemDDLNode * pConstraintAttributes_;

}; // class ElemDDLConstraint
        

//
// accessors
//

inline ElemDDLConstraint::constraintKindEnum
ElemDDLConstraint::getConstraintKind() const
{
  return constraintKind_;
}

inline NAString
ElemDDLConstraint::getConstraintName() const
{
  return getConstraintNameAsQualifiedName().getQualifiedNameAsAnsiString();
}

inline const QualifiedName &
ElemDDLConstraint::getConstraintNameAsQualifiedName() const
{
  return constraintQualName_;
}

inline QualifiedName &
ElemDDLConstraint::getConstraintNameAsQualifiedName()
{
  return constraintQualName_;
}

inline NABoolean
ElemDDLConstraint::isDeferrable() const
{
  return isDeferrable_;
}

inline NABoolean
ElemDDLConstraint::isDroppable() const
{
  return isDroppable_;
}

inline NABoolean
ElemDDLConstraint::isEnforced() const
{
  return isEnforced_;
}

inline NABoolean
ElemDDLConstraint::isDroppableSpecifiedExplicitly() const
{
  return droppableClauseInfo_ EQU DROPPABLE_SPECIFIED_EXPLICITLY;
}

inline NABoolean
ElemDDLConstraint::isNotDroppableSpecifiedExplicitly() const
{
  return droppableClauseInfo_ EQU NOT_DROPPABLE_SPECIFIED_EXPLICITLY;
}

inline NABoolean
ElemDDLConstraint::anyDroppableClauseSpecifiedExplicitly() const
{
  return droppableClauseInfo_ NEQ DROPPABLE_CLAUSE_NOT_SPECIFIED;
}

//
// mutators
//

inline void
ElemDDLConstraint::setConstraintKind(constraintKindEnum constraintKind)
{
  constraintKind_ = constraintKind;
}

inline void
ElemDDLConstraint::setConstraintName(const QualifiedName & constraintName)
{
  constraintQualName_ = constraintName;
}

#endif // ELEMDDLCONSTRAINT_H
