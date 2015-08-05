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
#ifndef ELEMDDLCONSTRAINTNOTNULL_H
#define ELEMDDLCONSTRAINTNOTNULL_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLConstraintNotNull.h
 * Description:  class for Not Null constraint definitions in DDL statements
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


#include "ElemDDLConstraint.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLConstraintNotNull;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// defintion of class ElemDDLConstraintNotNull
// -----------------------------------------------------------------------
class ElemDDLConstraintNotNull : public ElemDDLConstraint
{

public:

  // constructors
 ElemDDLConstraintNotNull(NABoolean isNotNull = TRUE, CollHeap * h=PARSERHEAP())
   : ElemDDLConstraint(h, ELM_CONSTRAINT_NOT_NULL_ELEM),
    isNotNull_(isNotNull)
  { }

  inline ElemDDLConstraintNotNull(const NAString & constraintName,
				  NABoolean isNotNull = TRUE,
                                  CollHeap * h=PARSERHEAP());

  // copy ctor
  ElemDDLConstraintNotNull (const ElemDDLConstraintNotNull & orig,
                            CollHeap * h=0) ; // not written

  // virtual destructor
  virtual ~ElemDDLConstraintNotNull();

  // cast
  virtual ElemDDLConstraintNotNull * castToElemDDLConstraintNotNull();
  virtual NABoolean isConstraintNotNull() const 
    //  { return TRUE; }
  { return isNotNull_; }

  // methods for tracing
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;


private:
  NABoolean isNotNull_;
}; // class ElemDDLConstraintNotNull

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLConstraintNotNull
// -----------------------------------------------------------------------

//
// constructors
//

inline
ElemDDLConstraintNotNull::ElemDDLConstraintNotNull (
						    const NAString & constraintName,
						    const NABoolean isNotNull,
						    CollHeap * h)
			 : ElemDDLConstraint(h, ELM_CONSTRAINT_NOT_NULL_ELEM,
					     constraintName),
			 isNotNull_(isNotNull)
{
}

#endif // ELEMDDLCONSTRAINTNOTNULL_H
