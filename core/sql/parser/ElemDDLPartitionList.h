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
#ifndef ELEMDDLPARTITIONLIST_H
#define ELEMDDLPARTITIONLIST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLPartitionList.h
 * Description:  class for lists of Partition elements specified in DDL
 *               statements.  Each Partition element contains all legal
 *               partition attributes associating with the partition.
 *               For the partition attributes that are not specified
 *               (in DDL statements), the default values are used.
 *
 *               Note that class ElemDDLPartitionList is derived from
 *               the base class ElemDDLList; therefore, the class
 *               ElemDDLPartitionList also represents a left linear
 *               tree.
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
class ElemDDLPartitionList;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// A list of partition elements specified in DDL statement
// associating with PARTITION.
// -----------------------------------------------------------------------
class ElemDDLPartitionList : public ElemDDLList
{

public:

  // constructor
  ElemDDLPartitionList(ElemDDLNode * commaExpr,
                              ElemDDLNode * otherExpr)
  : ElemDDLList(ELM_PARTITION_LIST, commaExpr, otherExpr)
  { }

  // virtual destructor
  virtual ~ElemDDLPartitionList();

  // cast
  virtual ElemDDLPartitionList * castToElemDDLPartitionList();

  // method for tracing
  virtual const NAString getText() const;

  // method for building text
  virtual NAString getSyntax() const;



private:

}; // class ElemDDLPartitionList


#endif // ELEMDDLPARTITIONLIST_H
