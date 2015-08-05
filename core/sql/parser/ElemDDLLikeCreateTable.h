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
#ifndef ELEMDDLLIKECREATETABLE_H
#define ELEMDDLLIKECREATETABLE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLLikeCreateTable.h
 * Description:  class for Like clause in DDL Create Table statements
 *
 *
 * Created:      6/5/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLLike.h"
#include "ParDDLLikeOptsCreateTable.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLLikeCreateTable;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Definition of class ElemDDLLikeCreateTable
// -----------------------------------------------------------------------
class ElemDDLLikeCreateTable : public ElemDDLLike
{

public:

  // constructor
  ElemDDLLikeCreateTable(const CorrName & sourceTableName
                         = CorrName("", PARSERHEAP()),
                         ElemDDLNode * pLikeOptions = NULL,
                         CollHeap * h=0);

  // copy ctor
  ElemDDLLikeCreateTable (const ElemDDLLikeCreateTable & orig,
                          CollHeap * h=0) ; // not written

  // virtual destructor
  virtual ~ElemDDLLikeCreateTable();

  // cast
  virtual ElemDDLLikeCreateTable * castToElemDDLLikeCreateTable();

  // accessors

  const ParDDLLikeOptsCreateTable &
  getLikeOptions() const
  {
    return likeOptions_;
  }

  ParDDLLikeOptsCreateTable &
  getLikeOptions()
  {
    return likeOptions_;
  }

  // method for tracing
  virtual const NAString getText() const;

  inline const QualifiedName & getDDLLikeCreateTableNameAsQualifiedName() const;
  inline       QualifiedName & getDDLLikeCreateTableNameAsQualifiedName();
  


private:

  ParDDLLikeOptsCreateTable likeOptions_;

}; // class ElemDDLLikeCreateTable

#endif /* ELEMDDLLIKECREATETABLE_H */
