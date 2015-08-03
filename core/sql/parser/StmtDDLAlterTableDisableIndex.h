#ifndef STMTDDLALTERTABLEDISABLEIDX_H
#define STMTDDLALTERTABLEDISABLEIDX_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTableDisableIndex.h
 * Description:  class for Alter Table <table-name> 
 *                  DISABLE ALL INDEXES and
 *                  DISABLE INDEX <index>
 *               DDL statements
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.cpp
 *
 *               
 * Created:     03/09/07
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
class StmtDDLAlterTableDisableIndex;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTableDisableIndex
// -----------------------------------------------------------------------
class StmtDDLAlterTableDisableIndex : public StmtDDLAlterTable
{

public:

  // constructor
  StmtDDLAlterTableDisableIndex(NAString & indexName, NABoolean allIndexes,
                                NABoolean allUniqueIndexes);

  // virtual destructor
  virtual ~StmtDDLAlterTableDisableIndex();

  // cast
  virtual StmtDDLAlterTableDisableIndex * castToStmtDDLAlterTableDisableIndex();

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

  StmtDDLAlterTableDisableIndex(); 
  StmtDDLAlterTableDisableIndex(const StmtDDLAlterTableDisableIndex &);
  StmtDDLAlterTableDisableIndex & operator=(const StmtDDLAlterTableDisableIndex &);

private:

  NAString  indexName_;
  NABoolean allIndexes_;
  NABoolean allUniqueIndexes_;

}; // class StmtDDLAlterTableDisableIndex

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterTableDisableIndex
// -----------------------------------------------------------------------

inline const NAString &
StmtDDLAlterTableDisableIndex::getIndexName() const
{
  return indexName_;
}

inline const NABoolean
StmtDDLAlterTableDisableIndex::getAllIndexes() const
{
  return allIndexes_;
}

inline const NABoolean
StmtDDLAlterTableDisableIndex::getAllUniqueIndexes() const
{
  return allUniqueIndexes_;
}

inline void 
StmtDDLAlterTableDisableIndex::setIndexName(NAString & indexName)
{
   indexName_ = indexName;
}

inline void 
StmtDDLAlterTableDisableIndex::setAllIndexes(NABoolean allIndexes)
{
  allIndexes_ = allIndexes;
}

inline void 
StmtDDLAlterTableDisableIndex::setAllUniqueIndexes(NABoolean allUniqueIndexes)
{
  allUniqueIndexes_ = allUniqueIndexes;
}

#endif // STMTDDLALTERTABLEDISABLEIDX_H
