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
/* -*-C++-*-
******************************************************************************
*
* File:         StmtDDLMisc.cpp
* Description:  Misc Stmt nodes
* Created:      
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "BaseTypes.h"
#include "ComASSERT.h"
#include "ComOperators.h"
#include "StmtDDLCleanupObjects.h"

StmtDDLCleanupObjects::StmtDDLCleanupObjects(ObjectType type,
                                             const NAString & param1,
                                             const NAString * param2,
                                             CollHeap    * heap)
  : StmtDDLNode(DDL_CLEANUP_OBJECTS),
    type_(type),
    param1_(param1),
    param2_(param2 ? *param2 : ""),
    tableQualName_(NULL),
    objectUID_(-1),
    stopOnError_(FALSE),
    getStatus_(FALSE),
    checkOnly_(FALSE),
    returnDetails_(FALSE)
{
}

StmtDDLCleanupObjects::~StmtDDLCleanupObjects()
{}

StmtDDLCleanupObjects *
StmtDDLCleanupObjects::castToStmtDDLCleanupObjects()
{
   return this;
}

Int32 StmtDDLCleanupObjects::getArity() const
{
  return 0;
}

ExprNode *
StmtDDLCleanupObjects::getChild(Lng32 index)
{
  return NULL;
}

const NAString
StmtDDLCleanupObjects::getText() const
{
  return "StmtDDLCleanupObjects";
}

ExprNode *
StmtDDLCleanupObjects::bindNode(BindWA * pBindWA)
{
  ComASSERT(pBindWA);

  //
  // expands table name
  // 

  if ((type_ == TABLE_) ||
      (type_ == INDEX_) ||
      (type_ == SEQUENCE_) ||
      (type_ == VIEW_) ||
      (type_ == SCHEMA_PRIVATE_) ||
      (type_ == SCHEMA_SHARED_) ||
      (type_ == HBASE_TABLE_) ||
      (type_ == UNKNOWN_))
    {
      ComObjectName tableName(param1_);
      tableQualName_ = new(PARSERHEAP())
        QualifiedName(
                      tableName.getObjectNamePart().getInternalName(),
                      tableName.getSchemaNamePart().getInternalName(),
                      tableName.getCatalogNamePart().getInternalName());

      // remember the original table name specified by user
      origTableQualName_ = *tableQualName_;

      if (applyDefaultsAndValidateObject(pBindWA, tableQualName_))
        {
          pBindWA->setErrStatus();
          return this;
        }

      if (NOT param2_.isNull())
        objectUID_ = atoInt64(param2_.data());
    }
  else if (type_ == OBJECT_UID_)
    {
      objectUID_ = atoInt64(param1_.data());
    }
  else
    {
      pBindWA->setErrStatus();
      return this;
    }

  //
  // sets a flag to let user know that the parse has
  // been bound.
  //
  markAsBound();

  return this;
}


