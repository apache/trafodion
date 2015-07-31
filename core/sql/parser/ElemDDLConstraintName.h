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
#ifndef ELEMDDLCONSTRAINTNAME_H
#define ELEMDDLCONSTRAINTNAME_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLConstraintName.h
 * Description:  class for Constraint Name (parse node) elements in DDL
 *               statements
 *
 *               
 * Created:      9/21/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLConstraintName;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Constraint Name (parse node) elements in DDL statements
// -----------------------------------------------------------------------
class ElemDDLConstraintName : public ElemDDLNode
{

public:

  // constructor
  ElemDDLConstraintName(const QualifiedName &constraintName,
                               CollHeap * h=0)
  : ElemDDLNode(ELM_COL_NAME_ELEM),
  constraintQualName_(constraintName, h)
  { }

  // virtual destructor
  virtual ~ElemDDLConstraintName();

  // cast
  virtual ElemDDLConstraintName * castToElemDDLConstraintName();

  // accessor
  inline NAString getConstraintName() const;
  inline const QualifiedName & getConstraintQualName() const;

  // member functions for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;


private:

  ElemDDLConstraintName();  // DO NOT USE
  ElemDDLConstraintName(const NAString &); // DO NOT USE
  ElemDDLConstraintName(const ElemDDLConstraintName &);  // DO NOT USE

  QualifiedName constraintQualName_;  

}; // class ElemDDLConstraintName

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLConstraintName
// -----------------------------------------------------------------------

inline NAString 
ElemDDLConstraintName::getConstraintName() const
{
  return getConstraintQualName().getQualifiedNameAsAnsiString();
}

inline const QualifiedName &
ElemDDLConstraintName::getConstraintQualName() const
{
  return constraintQualName_;
}

#endif // ELEMDDLCONSTRAINTNAME_H
