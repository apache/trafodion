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
#ifndef STMTDDLALTERTABLEPARTITION_H
#define STMTDDLALTERTABLEPARTITION_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTablePartition.h
 * Description:  class for Alter Table <table-name> Partition ...
 *               DDL statements
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.C.
 *
 *               
 * Created:      9/20/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "StmtDDLAlterTable.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAlterTablePartition;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAlterTablePartition
// -----------------------------------------------------------------------
class StmtDDLAlterTablePartition : public StmtDDLAlterTable
{

public:

  // constructor
  StmtDDLAlterTablePartition(ElemDDLNode * pPartitionAction)
  : StmtDDLAlterTable(DDL_ALTER_TABLE_PARTITION,
                    pPartitionAction)
  { }

  // virtual destructor
  virtual ~StmtDDLAlterTablePartition();

  // cast
  virtual StmtDDLAlterTablePartition * castToStmtDDLAlterTablePartition();

  // accessor
  inline ElemDDLNode * getPartitionAction() const;

  // method for tracing
  virtual const NAString getText() const;


private:

}; // class StmtDDLAlterTablePartition

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterTablePartition
// -----------------------------------------------------------------------


//
// accessor
//

inline
ElemDDLNode *
StmtDDLAlterTablePartition::getPartitionAction() const
{
  return getAlterTableAction();
}

#endif // STMTDDLALTERTABLEPARTITION_H
