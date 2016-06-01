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
#ifndef ELEMDDLPARTITIONRANGE_H
#define ELEMDDLPARTITIONRANGE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLPartitionRange.h
 * Description:  class to contain information about a range partition
 *               element specified in a DDL statement
 *
 *
 * Created:      9/29/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "NAString.h"
#include "ElemDDLPartitionSystem.h"
#include "ItemConstValueArray.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLPartitionRange;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of class ElemDDLPartitionRange
// -----------------------------------------------------------------------
class ElemDDLPartitionRange : public ElemDDLPartitionSystem
{

public:

  // constructors
  ElemDDLPartitionRange(CollHeap    * heap = PARSERHEAP());
  ElemDDLPartitionRange(ElemDDLPartition::optionEnum,
                        ElemDDLNode * pKeyValueList,
                        ElemDDLNode * pLocation,
                        ElemDDLNode * pPartitionRangeAttrList,
                        CollHeap    * heap = PARSERHEAP());

  // virtual destructor
  virtual ~ElemDDLPartitionRange();

  // cast
  virtual ElemDDLPartitionRange * castToElemDDLPartitionRange();

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline ItemConstValueArray &       getKeyValueArray();
  inline const ItemConstValueArray & getKeyValueArray() const;

  // mutators
  virtual void setChild(Lng32 index, ExprNode * pChildNode);

  // functions for tracing
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;

  // method for building text
  virtual NAString getSyntax() const;



private:

  //
  // range partitioning information
  //

  // FIRST KEY clause
  ItemConstValueArray keyValueArray_;

  //
  // pointers to child parse nodes
  //
  //   Note that class ElemDDLPartitionRange is derived from class
  //   ElemDDLPartitionSystem.  Therefore the former inherits the array
  //   children_ from the latter.  Usually this array contains all
  //   pointers to the child parse nodes, but class ElemDDLPartitionRange,
  //   the array only some (but not all) of the pointers.  The data
  //   element pKeyValueList_ also points to a child parse node.  Hence,
  //   methods getChild() and setChild() contain additional logic to
  //   handle this special case.

  enum { INDEX_KEY_VALUE_LIST = MAX_ELEM_DDL_PARTITION_SYSTEM_ARITY,
         MAX_ELEM_DDL_PARTITION_RANGE_ARITY };

  ElemDDLNode * pKeyValueList_;

}; // class ElemDDLPartitionRange

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLPartitionRange
// -----------------------------------------------------------------------

//
// accessors
//

inline ItemConstValueArray &
ElemDDLPartitionRange::getKeyValueArray()
{
  return keyValueArray_;
}

inline const ItemConstValueArray &
ElemDDLPartitionRange::getKeyValueArray() const
{
  return keyValueArray_;
}

#endif // ELEMDDLPARTITIONRANGE_H
