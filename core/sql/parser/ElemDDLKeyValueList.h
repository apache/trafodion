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
#ifndef ELEMDDLKEYVALUELIST_H
#define ELEMDDLKEYVALUELIST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLKeyValueList.h
 * Description:  class for lists of Key Value elements in First Key
 *               clause in DDL statements
 *               
 *               
 * Created:      4/7/95
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
class ElemDDLKeyValueList;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// A list of key value elements in First Key clause in DDL statement.
// -----------------------------------------------------------------------
class ElemDDLKeyValueList : public ElemDDLList
{

public:

  // constructor
  ElemDDLKeyValueList(ElemDDLNode * commaExpr,
                             ElemDDLNode * otherExpr)
  : ElemDDLList(ELM_KEY_VALUE_LIST, commaExpr, otherExpr)
  { }

  // virtual destructor
  virtual ~ElemDDLKeyValueList();

  // cast
  virtual ElemDDLKeyValueList * castToElemDDLKeyValueList();

  // method for tracing
  virtual const NAString getText() const;

  // method for building text
  virtual NAString getSyntax() const;


private:

}; // class ElemDDLKeyValueList

#endif // ELEMDDLKEYVALUELIST_H
