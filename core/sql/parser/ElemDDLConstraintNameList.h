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
#ifndef ELEMDDLCONSTRAINTNAMELIST_H
#define ELEMDDLCONSTRAINTNAMELIST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLConstraintNameList.h
 * Description:  class for lists of constraint name elements in 
 *               DDL statements
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


#include "ElemDDLList.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLConstraintNameList;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// A list of key value elements in First Key clause in DDL statement.
// -----------------------------------------------------------------------
class ElemDDLConstraintNameList : public ElemDDLList
{

public:

  // constructor
  ElemDDLConstraintNameList(ElemDDLNode * commaExpr,
                                   ElemDDLNode * otherExpr)
  : ElemDDLList(ELM_CONSTRAINT_NAME_LIST, commaExpr, otherExpr)
  { }

  // virtual destructor
  virtual ~ElemDDLConstraintNameList();

  // cast
  virtual ElemDDLConstraintNameList * castToElemDDLConstraintNameList();

  // methods for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLConstraintNameList


#endif // ELEMDDLCONSTRAINTNAMELIST_H
