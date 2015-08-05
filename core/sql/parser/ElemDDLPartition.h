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
#ifndef ELEMDDLPARTITION_H
#define ELEMDDLPARTITION_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLPartition.h
 * Description:  class to contain information about partition attributes
 *               associating with a DDL statement.  This class is a
 *               base class.  Classes ElemDDLPartitionRange and
 *               ElemDDLPartitionSystem are derived from this class.
 *
 *
 * Created:      4/6/95
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
class ElemDDLPartition;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of class ElemDDLPartition
// -----------------------------------------------------------------------
class ElemDDLPartition : public ElemDDLNode
{

public:

  enum optionEnum { ADD_OPTION, DROP_OPTION };

  // default constructor
  ElemDDLPartition(OperatorTypeEnum operType =
                          ELM_ANY_PARTITION_ELEM)
  : ElemDDLNode(operType)
  , isTheSpecialCase1aOr1b_(FALSE) // Fix for Bugzilla bug 1369

  { }

  // virtual destructor
  virtual ~ElemDDLPartition();

  // cast
  virtual ElemDDLPartition * castToElemDDLPartition();

  //
  // accessors
  //

  inline NABoolean isTheSpecialCase1aOr1b() const
  { return isTheSpecialCase1aOr1b_; } // Fix for Bugzilla bug 1369

  //
  // mutators
  //

  inline void setTheSpecialCase1aOr1bFlag(NABoolean setting)
  { isTheSpecialCase1aOr1b_= setting; }

private:

  NABoolean isTheSpecialCase1aOr1b_; // Fix for Bugzilla bug 1369 - See comments in StmtDDLCreate.cpp

}; // class ElemDDLPartition

#endif // ELEMDDLPARTITION_H

