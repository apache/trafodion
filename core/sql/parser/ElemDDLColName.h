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
#ifndef ELEMDDLCOLNAME_H
#define ELEMDDLCOLNAME_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLColName.h
 * Description:  class for Column Name (parse node) elements in DDL
 *               statements
 *
 *               
 * Created:      4/12/95
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
class ElemDDLColName;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Column Name (parse node) elements in DDL statements
// -----------------------------------------------------------------------
class ElemDDLColName : public ElemDDLNode
{

public:

  // constructor
  ElemDDLColName(const NAString &columnName)
  : ElemDDLNode(ELM_COL_NAME_ELEM),
  columnName_(columnName, PARSERHEAP())
  { }

  // virtual destructor
  virtual ~ElemDDLColName();

  // cast
  virtual ElemDDLColName * castToElemDDLColName();

  // accessor
  inline const NAString & getColumnName() const;

  // member functions for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;


private:

  NAString columnName_;

}; // class ElemDDLColName

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLColName
// -----------------------------------------------------------------------

inline const NAString &
ElemDDLColName::getColumnName() const
{
  return columnName_;
}

#endif // ELEMDDLCOLNAME_H
