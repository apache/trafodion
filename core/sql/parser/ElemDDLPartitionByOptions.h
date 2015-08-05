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
#ifndef ELEMDDLPARTITIONBYOPTIONS_H
#define ELEMDDLPARTITIONBYOPTIONS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLPartitionByOptions.h
 * Description:  classes representing Partition By options specified in
 *               Create Table DDL statements
 *               Modelled like ElemDDLStoreOptions.h
 *
 *               
 * Created:      07/10/97
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ElemDDLColRefArray.h"
#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLPartitionByOpt;
class ElemDDLPartitionByColumnList;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of base class ElemDDLPartitionByOpt
// -----------------------------------------------------------------------
class ElemDDLPartitionByOpt : public ElemDDLNode
{

public:

  // constructor
  ElemDDLPartitionByOpt(OperatorTypeEnum operatorType =
                          ELM_ANY_PARTITION_BY_ELEM);

  // virtual destructor
  virtual ~ElemDDLPartitionByOpt();

  // cast
  virtual ElemDDLPartitionByOpt * castToElemDDLPartitionByOpt();

  // methods for tracing
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPartitionByOpt

// -----------------------------------------------------------------------
// definition of class ElemDDLPartitionByColumnList
// -----------------------------------------------------------------------
class ElemDDLPartitionByColumnList : public ElemDDLPartitionByOpt
{

public:

  // constructor
  ElemDDLPartitionByColumnList(ElemDDLNode * partitionKeyColumnList,
                               CollHeap    * heap = PARSERHEAP());

  // virtual destructor
  virtual ~ElemDDLPartitionByColumnList();

  // casting
  virtual ElemDDLPartitionByColumnList * castToElemDDLPartitionByColumnList();

  //
  // accessors
  //

  virtual Int32 getArity() const;

  virtual ExprNode * getChild(Lng32 index);

  inline const ElemDDLColRefArray & getPartitionKeyColumnArray() const;
  inline       ElemDDLColRefArray & getPartitionKeyColumnArray();

  inline ElemDDLNode * getPartitionKeyColumnList() const;

  // mutator
  virtual void setChild(Lng32 index, ExprNode * pElemDDLNode);

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

  // method for building text
  virtual NAString getSyntax() const;

private:

  ElemDDLPartitionByColumnList
    (const ElemDDLPartitionByColumnList &);  // DO NOT USE
  ElemDDLPartitionByColumnList & operator=
    (const ElemDDLPartitionByColumnList &);  // DO NOT USE

  //
  // data members
  //

  ElemDDLColRefArray partitionKeyColumnArray_;

  // pointers to child parse node

  enum { INDEX_PARTITION_KEY_COLUMN_LIST = 0,
         MAX_ELEM_DDL_PARTITION_BY_COLUMN_LIST_ARITY };

  ElemDDLNode * children_[MAX_ELEM_DDL_PARTITION_BY_COLUMN_LIST_ARITY];

}; // class ElemDDLPartitionByColumnList

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLStoreOptKeyColumnList
// -----------------------------------------------------------------------

//
// accessor
//

inline ElemDDLColRefArray &
ElemDDLPartitionByColumnList::getPartitionKeyColumnArray()
{
  return partitionKeyColumnArray_;
}

inline const ElemDDLColRefArray &
ElemDDLPartitionByColumnList::getPartitionKeyColumnArray() const
{
  return partitionKeyColumnArray_;
}

inline ElemDDLNode *
ElemDDLPartitionByColumnList::getPartitionKeyColumnList() const
{
  return children_[INDEX_PARTITION_KEY_COLUMN_LIST];
}

#endif // ELEMDDLPARTITIONBYOPTIONS_H
