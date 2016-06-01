#ifndef STMTDDLALTERTABLEENABLEIDX_H
#define STMTDDLALTERTABLEENABLEIDX_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableEnableIndex.h
 * Description:  class for Alter Table <table-name> 
 *                  ENABLE ALL INDEXES and
 *                  ENABLE INDEX <index>
 *               DDL statements
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.cpp
 *
 *               
 * Created:    
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

#include "StmtDDLAlterTable.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAlterTableEnableIndex;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableEnableIndex
// -----------------------------------------------------------------------
class StmtDDLAlterTableEnableIndex : public StmtDDLAlterTable
{

public:

  // constructor
  StmtDDLAlterTableEnableIndex(NAString & indexName, NABoolean allIndexes,
                               NABoolean allUniqueIndexes);

  // virtual destructor
  virtual ~StmtDDLAlterTableEnableIndex();

  // cast
  virtual StmtDDLAlterTableEnableIndex * castToStmtDDLAlterTableEnableIndex();

  // accessors
  inline const NAString &getIndexName() const;
  inline const NABoolean getAllIndexes() const;
  inline const NABoolean getAllUniqueIndexes() const;

  // ---------------------------------------------------------------------
  // mutators
  // ---------------------------------------------------------------------
 
  // This method collects information in the parse sub-tree and copies it
  // to the current parse node.
  void synthesize();
  inline void setIndexName(NAString & indexName);
  inline void setAllIndexes(NABoolean allIndexes);
  inline void setAllUniqueIndexes(NABoolean allIndexes);

  // methods for tracing
  const NAString displayLabel2() const;
  virtual const NAString getText() const;

  //
  // please do not use the following methods
  //

  StmtDDLAlterTableEnableIndex(); 
  StmtDDLAlterTableEnableIndex(const StmtDDLAlterTableEnableIndex &);
  StmtDDLAlterTableEnableIndex & operator=(const StmtDDLAlterTableEnableIndex &);

private:

  NAString  indexName_;
  NABoolean allIndexes_;
  NABoolean allUniqueIndexes_;

}; // class StmtDDLAlterTableEnableIndex

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterTableEnableIndex
// -----------------------------------------------------------------------

inline const NAString &
StmtDDLAlterTableEnableIndex::getIndexName() const
{
  return indexName_;
}

inline const NABoolean
StmtDDLAlterTableEnableIndex::getAllIndexes() const
{
  return allIndexes_;
}

inline const NABoolean
StmtDDLAlterTableEnableIndex::getAllUniqueIndexes() const
{
  return allUniqueIndexes_;
}

inline void 
StmtDDLAlterTableEnableIndex::setIndexName(NAString & indexName)
{
   indexName_ = indexName;
}

inline void 
StmtDDLAlterTableEnableIndex::setAllIndexes(NABoolean allIndexes)
{
  allIndexes_ = allIndexes;
}

inline void 
StmtDDLAlterTableEnableIndex::setAllUniqueIndexes(NABoolean allUniqueIndexes)
{
  allUniqueIndexes_ = allUniqueIndexes;
}

#endif // STMTDDLALTERTABLEENABLEIDX_H
