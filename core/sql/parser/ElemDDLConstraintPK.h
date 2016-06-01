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
#ifndef ELEMDDLCONSTRAINTPK_H
#define ELEMDDLCONSTRAINTPK_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLConstraintPK.h
 * Description:  class for Primary Key constraint definitions in DDL
 *               statements
 *    
 *           
 * Created:      4/14/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComSmallDefs.h"
#include "ElemDDLConstraintUnique.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLConstraintPK;
class ElemDDLConstraintPKColumn;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLConstraintPK
// -----------------------------------------------------------------------
class ElemDDLConstraintPK : public ElemDDLConstraintUnique
{

public:

  // constructors
  ElemDDLConstraintPK(ElemDDLNode * pColumnRefList = NULL)
  : ElemDDLConstraintUnique(ELM_CONSTRAINT_PRIMARY_KEY_ELEM,
                          pColumnRefList)
  { }
  ElemDDLConstraintPK(OperatorTypeEnum operatorType)
  : ElemDDLConstraintUnique(operatorType,
                          NULL /*column_reference_list*/)
  { }

  // virtual destructor
  virtual ~ElemDDLConstraintPK();

  // cast
  virtual ElemDDLConstraintPK * castToElemDDLConstraintPK();

  // methods for tracing
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;


private:

}; // class ElemDDLConstraintPK

// -----------------------------------------------------------------------
// definition of class ElemDDLConstraintPKColumn
// -----------------------------------------------------------------------
class ElemDDLConstraintPKColumn : public ElemDDLConstraintPK
{

public:

  // constructor
  ElemDDLConstraintPKColumn(ComColumnOrdering orderingSpec =
                                   COM_ASCENDING_ORDER)
  : ElemDDLConstraintPK(ELM_CONSTRAINT_PRIMARY_KEY_COLUMN_ELEM),
  columnOrdering_(orderingSpec)
  { }

  // virtual destructor
  virtual ~ElemDDLConstraintPKColumn();

  // cast
  virtual ElemDDLConstraintPKColumn * castToElemDDLConstraintPKColumn();

  // accessor
  inline ComColumnOrdering getColumnOrdering() const;

  // methods for tracing
  virtual const NAString getText() const;


private:

  ComColumnOrdering columnOrdering_;

}; // class ElemDDLConstraintPKColumn

// -----------------------------------------------------------------------
// definitions of inline methods of class ElemDDLConstraintPKColumn
// -----------------------------------------------------------------------
//
// accessor
//

inline ComColumnOrdering
ElemDDLConstraintPKColumn::getColumnOrdering() const
{
  return columnOrdering_;
}

#endif /* ELEMDDLCONSTRAINTPK_H */
