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
#ifndef ELEMDDLCONSTRAINTATTRENFORCED_H
#define ELEMDDLCONSTRAINTATTRENFORCED_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLConstraintAttrEnforced.h
 * Description:  class for parse node representing the enforced or
 *               not enforced constraint attribute specified in
 *               constraint definitions
 *               
 * Created:      04/15/08
 * Language:     C++
 *
 *
 */


#include "ElemDDLConstraintAttr.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLConstraintAttrEnforced;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLConstraintAttrEnforced
// -----------------------------------------------------------------------
class ElemDDLConstraintAttrEnforced : public ElemDDLConstraintAttr
{

public:

  // default constructor
  ElemDDLConstraintAttrEnforced(NABoolean isEnforced = TRUE)
  : ElemDDLConstraintAttr(ELM_CONSTRAINT_ATTR_ENFORCED_ELEM),
  isEnforced_(isEnforced)
  { }

  // virtual destructor
  virtual ~ElemDDLConstraintAttrEnforced();

  // cast
  virtual ElemDDLConstraintAttrEnforced *
                castToElemDDLConstraintAttrEnforced();

  // accessor
  inline NABoolean isEnforced() const;

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;


private:

  NABoolean isEnforced_;

}; // class ElemDDLConstraintAttrEnforced

//
// accessor
//

inline NABoolean
ElemDDLConstraintAttrEnforced::isEnforced() const
{
  return isEnforced_;
}

#endif // ELEMDDLCONSTRAINTATTRENFORCED_H
