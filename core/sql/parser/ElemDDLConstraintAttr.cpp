/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         ElemDDLConstraint.C
 * Description:  methods for classes representing constraints.
 *
 * Created:      9/21/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "AllElemDDLConstraintAttr.h"
#include "BaseTypes.h"

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraintAttr
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLConstraintAttr::~ElemDDLConstraintAttr()
{
}

// cast
ElemDDLConstraintAttr *
ElemDDLConstraintAttr::castToElemDDLConstraintAttr()
{
  return this;
}

//
// method for tracing
//

const NAString
ElemDDLConstraintAttr::getText() const
{
  NAAbort("ElemDDLConstraintAttr.C", __LINE__, "internal logic error");
  return "ElemDDLConstraintAttr";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraintAttrDroppable
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLConstraintAttrDroppable::~ElemDDLConstraintAttrDroppable()
{
}

// cast
ElemDDLConstraintAttrDroppable *
ElemDDLConstraintAttrDroppable::castToElemDDLConstraintAttrDroppable()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLConstraintAttrDroppable::displayLabel1() const
{
  return NAString("is droppable? ") + YesNo(isDroppable());
}

const NAString
ElemDDLConstraintAttrDroppable::getText() const
{
  return "ElemDDLConstraintAttrDroppable";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraintAttrEnforced
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLConstraintAttrEnforced::~ElemDDLConstraintAttrEnforced()
{
}

// cast
ElemDDLConstraintAttrEnforced *
ElemDDLConstraintAttrEnforced::castToElemDDLConstraintAttrEnforced()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLConstraintAttrEnforced::displayLabel1() const
{
  return NAString("is enforced? ") + YesNo(isEnforced());
}

const NAString
ElemDDLConstraintAttrEnforced::getText() const
{
  return "ElemDDLConstraintAttrEnforced";
}

//
// End of File
//
