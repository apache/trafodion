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
#ifndef ELEMDDLCONSTRAINTATTR_H
#define ELEMDDLCONSTRAINTATTR_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLConstraintAttr.h
 * Description:  base class for (generic) parse nodes representing
 *               constraint attributes specified in constraint
 *               definitions
 *
 *               
 * Created:      11/10/95
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
class ElemDDLConstraintAttr;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLConstraintAttr
// -----------------------------------------------------------------------
class ElemDDLConstraintAttr : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLConstraintAttr(OperatorTypeEnum operType
                               = ELM_ANY_CONSTRAINT_ATTR_ELEM)
  : ElemDDLNode(operType)
  { }

  // virtual destructor
  virtual ~ElemDDLConstraintAttr();

  // cast
  virtual ElemDDLConstraintAttr * castToElemDDLConstraintAttr();

  // methods for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLConstraintAttr


#endif // ELEMDDLCONSTRAINTATTR_H
