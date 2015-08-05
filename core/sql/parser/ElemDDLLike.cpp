/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         ElemDDLLike.C
 * Description:  methods for class ElemDDLLike and any classes
 *               derived from class ElemDDLLike.
 *
 *               Please note that classes with prefix ElemDDLLikeOpt
 *               are not derived from class ElemDDLLike.  Therefore
 *               they are defined in files ElemDDLLikeOpt.C and
 *               ElemDDLLikeOptions.h instead.
 *
 * Created:      6/5/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "AllElemDDLLike.h"
#include "ComASSERT.h"
#include "ComOperators.h"

// -----------------------------------------------------------------------
// methods for class ElemDDLLike
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLike::~ElemDDLLike()
{
  // delete all children
  for (Int32 i = 0; i < MAX_ELEM_DDL_LIKE_ARITY; i++)
  {
    delete getChild(i);
  }
}

// cast
ElemDDLLike *
ElemDDLLike::castToElemDDLLike()
{
  return this;
}

// accessors

// get the degree of this node
Int32
ElemDDLLike::getArity() const
{
  return MAX_ELEM_DDL_LIKE_ARITY;
}

ExprNode *
ElemDDLLike::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < MAX_ELEM_DDL_LIKE_ARITY);
  return pLikeOptions_;
}

// mutators

void
ElemDDLLike::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index EQU INDEX_LIKE_OPT_LIST);
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    pLikeOptions_ = pChildNode->castToElemDDLNode();
  }
  else
    pLikeOptions_ = NULL;
}

// methods for tracing

const NAString
ElemDDLLike::getText() const
{
  return "ElemDDLLike";
}

const NAString
ElemDDLLike::displayLabel1() const
{
  if (getSourceTableName().length() NEQ 0)
    return NAString("Source table name: ") + getSourceTableName();
  else
    return NAString();
}

// -----------------------------------------------------------------------
// methods for class ElemDDLLikeCreateTable
// -----------------------------------------------------------------------

// constructor
ElemDDLLikeCreateTable::ElemDDLLikeCreateTable(
     const CorrName & sourceTableName,
     ElemDDLNode * pLikeOptions,
     CollHeap * h)
: ElemDDLLike(ELM_LIKE_CREATE_TABLE_ELEM,
              sourceTableName,
              pLikeOptions,
              h)
{
  if (pLikeOptions NEQ NULL)
  {
    for (CollIndex index = 0; index < pLikeOptions->entries(); index++)
    {
      likeOptions_.setLikeOption((*pLikeOptions)[index]
                                 ->castToElemDDLLikeOpt());
    }
  }
}

// virtual destructor
ElemDDLLikeCreateTable::~ElemDDLLikeCreateTable()
{
}

// cast
ElemDDLLikeCreateTable *
ElemDDLLikeCreateTable::castToElemDDLLikeCreateTable()
{
  return this;
}

// methods for tracing

const NAString
ElemDDLLikeCreateTable::getText() const
{
  return "ElemDDLLikeCreateTable";
}

//
// End of File
//
