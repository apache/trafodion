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
#ifndef ELEMDDLKEYVALUE_H
#define ELEMDDLKEYVALUE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLKeyValue.h
 * Description:  class for Key Value elements in DDL statements
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


#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLKeyValue;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class ItemExpr;
class ConstValue;

// -----------------------------------------------------------------------
// Key Value elements in DDL statements.
// -----------------------------------------------------------------------
class ElemDDLKeyValue : public ElemDDLNode
{

public:

  // constructor
  ElemDDLKeyValue(ItemExpr * pConstValue);

  // virtual destructor
  virtual ~ElemDDLKeyValue();

  // cast
  virtual ElemDDLKeyValue * castToElemDDLKeyValue();  

  // accessors

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline ConstValue * getKeyValue() const;

  // mutator
  virtual void setChild(Lng32 index, ExprNode * pChildNode);

  // methods for tracing
  virtual const NAString getText() const;

  // method for building text
  virtual NAString getSyntax() const;




private:

  // pointer to child parse node

  enum { INDEX_KEY_VALUE = 0,
         MAX_ELEM_DDL_KEY_VALUE_ARITY };

  ConstValue * keyValue_;
  
}; // class ElemDDLKeyValue

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLKeyValue
// -----------------------------------------------------------------------

inline ConstValue *
ElemDDLKeyValue::getKeyValue() const
{
  return keyValue_;
}

#endif // ELEMDDLKEYVALUE_H
