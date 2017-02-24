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
 * File:         ElemDDLStoreOptions.C
 * Description:  methods for class ElemDDLStoreOpt and any classes
 *               derived from class ElemDDLStoreOpt; also methods
 *               for class ElemDDLStoreOptArray
 *
 * Created:      10/16/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComASSERT.h"
#include "ComOperators.h"
#include "ElemDDLStoreOptions.h"

// -----------------------------------------------------------------------
// methods for class ElemDDLStoreOpt
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLStoreOpt::~ElemDDLStoreOpt()
{
}

// casting
ElemDDLStoreOpt *
ElemDDLStoreOpt::castToElemDDLStoreOpt()
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLStoreOpt::getDetailInfo() const
{
  NATraceList detailTextList;

  //
  // Note that the invoked displayLabel1() is a method of
  // a class derived from class ElemDDLStoreOpt
  //
  detailTextList.append(displayLabel1());

  return detailTextList;
}

const NAString
ElemDDLStoreOpt::getText() const
{
  ABORT("internal logic error");
  return "ElemDDLStoreOpt";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLStoreOptDefault
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLStoreOptDefault::~ElemDDLStoreOptDefault()
{
}

// casting
ElemDDLStoreOptDefault *
ElemDDLStoreOptDefault::castToElemDDLStoreOptDefault()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLStoreOptDefault::displayLabel1() const
{
  return NAString("Default store option");
}

const NAString
ElemDDLStoreOptDefault::getText() const
{
  return "ElemDDLStoreOptDefault";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLStoreOptEntryOrder
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLStoreOptEntryOrder::~ElemDDLStoreOptEntryOrder()
{
}

// casting
ElemDDLStoreOptEntryOrder *
ElemDDLStoreOptEntryOrder::castToElemDDLStoreOptEntryOrder()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLStoreOptEntryOrder::displayLabel1() const
{
  return NAString("Entry Order store option");
}

const NAString
ElemDDLStoreOptEntryOrder::getText() const
{
  return "ElemDDLStoreOptEntryOrder";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLStoreOptKeyColumnList
// -----------------------------------------------------------------------

//
// constructor
//

ElemDDLStoreOptKeyColumnList::ElemDDLStoreOptKeyColumnList(
     ElemDDLNode * pKeyColumnList,
     NABoolean uniqueStoreBy,
     NABoolean uniqueStoreByKeylist,
     NABoolean pkeyStoreByKeylist,
     CollHeap    * heap)
: ElemDDLStoreOpt(ELM_STORE_OPT_KEY_COLUMN_LIST_ELEM),
  keyColumnArray_(heap),
  uniqueStoreBy_(uniqueStoreBy),
  uniqueStoreByKeylist_(uniqueStoreByKeylist),
  pkeyStoreByKeylist_(pkeyStoreByKeylist),
  ser_(ComPkeySerialization::COM_SER_NOT_SPECIFIED)
{
  setChild(INDEX_KEY_COLUMN_LIST, pKeyColumnList);

  //
  // copies pointers to parse nodes representing
  // column names (appearing in a store option) to
  // keyColumnArray_ so the user can access this
  // information easier.
  //

  ComASSERT(pKeyColumnList NEQ NULL);
  for (CollIndex i = 0; i < pKeyColumnList->entries(); i++)
  {
    keyColumnArray_.insert((*pKeyColumnList)[i]->castToElemDDLColRef());
  }
}

// virtual destructor
ElemDDLStoreOptKeyColumnList::~ElemDDLStoreOptKeyColumnList()
{
}

// casting
ElemDDLStoreOptKeyColumnList *
ElemDDLStoreOptKeyColumnList::castToElemDDLStoreOptKeyColumnList()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLStoreOptKeyColumnList::getArity() const
{
  return MAX_ELEM_DDL_STORE_OPT_KEY_COLUMN_LIST_ARITY;
}

ExprNode *
ElemDDLStoreOptKeyColumnList::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

//
// mutator
//

void
ElemDDLStoreOptKeyColumnList::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    children_[index] = pChildNode->castToElemDDLNode();
  }
  else
  {
    children_[index] = NULL;
  }
}

//
// methods for tracing
//

const NAString
ElemDDLStoreOptKeyColumnList::displayLabel1() const
{
  return NAString("Key Column List store option");
}

NATraceList
ElemDDLStoreOptKeyColumnList::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;
  ElemDDLNode   * pKeyColumnList = getKeyColumnList();

  //
  // kind of store option
  //

  detailTextList.append(displayLabel1());

  //
  // column name list
  //

  if (pKeyColumnList EQU NULL)
  {
    detailTextList.append("No column name list.");
    return detailTextList;
  }

  detailText = "Column Name List [";
  detailText += LongToNAString((Lng32)pKeyColumnList->entries());
  detailText += " element(s)]:";
  detailTextList.append(detailText);

  for (CollIndex i = 0; i < pKeyColumnList->entries(); i++)
  {
    detailText = "[column ";
    detailText += LongToNAString((Lng32)i);
    detailText += "]";
    detailTextList.append(detailText);

    detailTextList.append("    ", (*pKeyColumnList)[i]->getDetailInfo());
  }
  return detailTextList;
}

const NAString
ElemDDLStoreOptKeyColumnList::getText() const
{
  return "ElemDDLStoreOptKeyColumnList";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLStoreOptNondroppablePK
// -----------------------------------------------------------------------


//
// constructor
//

ElemDDLStoreOptNondroppablePK::ElemDDLStoreOptNondroppablePK(
     NABoolean uniqueStoreByPrimaryKey)
: ElemDDLStoreOpt(ELM_STORE_OPT_NONDROPPABLE_PRIMARY_KEY_ELEM),
    uniqueStoreByPrimaryKey_(uniqueStoreByPrimaryKey)
{
}

// virtual destructor
ElemDDLStoreOptNondroppablePK::~ElemDDLStoreOptNondroppablePK()
{
}

// casting
ElemDDLStoreOptNondroppablePK *
ElemDDLStoreOptNondroppablePK::castToElemDDLStoreOptNondroppablePK()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLStoreOptNondroppablePK::displayLabel1() const
{
  return NAString("Nondroppable Primary Key store option");
}

const NAString
ElemDDLStoreOptNondroppablePK::getText() const
{
  return "ElemDDLStoreOptNondroppablePK";
}

//
// End of File
//
