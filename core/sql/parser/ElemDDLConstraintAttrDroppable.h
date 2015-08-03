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
#ifndef ELEMDDLCONSTRAINTATTRDROPPABLE_H
#define ELEMDDLCONSTRAINTATTRDROPPABLE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLConstraintAttrDroppable.h
 * Description:  class for parse node representing the droppable or
 *               nondroppable constraint attribute specified in
 *               constraint definitions
 *               
 * Created:      11/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLConstraintAttr.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLConstraintAttrDroppable;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLConstraintAttrDroppable
// -----------------------------------------------------------------------
class ElemDDLConstraintAttrDroppable : public ElemDDLConstraintAttr
{

public:

  // default constructor
  ElemDDLConstraintAttrDroppable(NABoolean isDroppable = FALSE)
  : ElemDDLConstraintAttr(ELM_CONSTRAINT_ATTR_DROPPABLE_ELEM),
  isDroppable_(isDroppable)
  { }

  // virtual destructor
  virtual ~ElemDDLConstraintAttrDroppable();

  // cast
  virtual ElemDDLConstraintAttrDroppable *
                castToElemDDLConstraintAttrDroppable();

  // accessor
  inline NABoolean isDroppable() const;

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;


private:

  NABoolean isDroppable_;

}; // class ElemDDLConstraintAttrDroppable

//
// accessor
//

inline NABoolean
ElemDDLConstraintAttrDroppable::isDroppable() const
{
  return isDroppable_;
}

#endif // ELEMDDLCONSTRAINTATTRDROPPABLE_H
