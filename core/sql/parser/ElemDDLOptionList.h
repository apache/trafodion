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
#ifndef ELEMDDLOPTIONLIST_H
#define ELEMDDLOPTIONLIST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLOptionList.h
 * Description:  class for lists of options specified in DDL
 *               statements.  Note that class ElemDDLOptionList
 *               is derived from base class ElemDDLList;
 *               therefore, class ElemDDLOptionList also
 *               represents a left linear tree.
 *
 *               
 * Created:      9/18/95
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
class ElemDDLOptionList;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLOptionList
// -----------------------------------------------------------------------
class ElemDDLOptionList : public ElemDDLList
{

public:

  // constructor
  ElemDDLOptionList(ElemDDLNode * commaExpr,
                           ElemDDLNode * otherExpr)
  : ElemDDLList(ELM_OPTION_LIST, commaExpr, otherExpr)
  { }

  // virtual destructor
  virtual ~ElemDDLOptionList();

  // cast
  virtual ElemDDLOptionList * castToElemDDLOptionList();

  // method for tracing
  virtual const NAString getText() const;


  // method for building text
  virtual NAString getSyntax() const;


private:

}; // class ElemDDLOptionList


#endif // ELEMDDLOPTIONLIST_H
