#ifndef ELEMDDLPARTITIONSINGLE_H
#define ELEMDDLPARTITIONSINGLE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLPartitionSingle.h
 * Description:  class to contain information about a single partition
 *               element specified in a DDL statement.  As the name
 *               implies, only single partition is involved (the
 *               primary partition).
 *
 *
 * Created:      7/27/96
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */


#include "ElemDDLPartitionSystem.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLPartitionSingle;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of class ElemDDLPartitionSingle
// -----------------------------------------------------------------------
class ElemDDLPartitionSingle : public ElemDDLPartitionSystem
{

public:

  // constructors
  ElemDDLPartitionSingle();

  // virtual destructor
  virtual ~ElemDDLPartitionSingle();

  // cast
  virtual ElemDDLPartitionSingle * castToElemDDLPartitionSingle();

private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------
  
  ElemDDLPartitionSingle(const ElemDDLPartitionSingle &);

        // copy constructor not supported
  
  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------
  //
  // none
  //
  
}; // class ElemDDLPartitionSingle

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLPartitionSingle
// -----------------------------------------------------------------------
//
// none
//

#endif // ELEMDDLPARTITIONSINGLE_H
