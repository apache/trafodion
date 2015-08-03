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
 * File:         ElemDDLRefTrigActions.C
 * Description:  methods for class ElemDDLRefTrigAct and any classes
 *               derived from class ElemDDLRefTrigAct
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
#include "ElemDDLRefTrigActions.h"

// -----------------------------------------------------------------------
// methods for class ElemDDLRefTrigAct
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLRefTrigAct::~ElemDDLRefTrigAct()
{
}

// casting
ElemDDLRefTrigAct *
ElemDDLRefTrigAct::castToElemDDLRefTrigAct()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLRefTrigAct::getArity() const
{
  return MAX_ELEM_DDL_REF_TRIG_ACT_ARITY;
}

ExprNode *
ElemDDLRefTrigAct::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

//
// mutator
//

void
ElemDDLRefTrigAct::setChild(Lng32 index, ExprNode * pChildNode)
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

NATraceList
ElemDDLRefTrigAct::getDetailInfo() const
{
  NATraceList detailTextList;

  //
  // Note that the invoked displayLabel1() is a method of class
  // ElemDDLRefTrigActDeleteRule or ElemDDLRefTrigActUpdateRule
  //
  detailTextList.append(displayLabel1());

  return detailTextList;
}

const NAString
ElemDDLRefTrigAct::getText() const
{
  ABORT("internal logic error");
  return "ElemDDLRefTrigAct";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLRefTrigActDeleteRule
// -----------------------------------------------------------------------

//
// constructor
//

ElemDDLRefTrigActDeleteRule::ElemDDLRefTrigActDeleteRule(
     ElemDDLNode * pReferentialAction)
: ElemDDLRefTrigAct(ELM_REF_TRIG_ACT_DELETE_RULE_ELEM,
                    pReferentialAction)
{
  ComASSERT(pReferentialAction NEQ NULL);
  switch (pReferentialAction->getOperatorType())
  {
  case ELM_REF_ACT_CASCADE_ELEM :
    deleteRule_ = COM_CASCADE_DELETE_RULE;
    break;

  case ELM_REF_ACT_NO_ACTION_ELEM :
    deleteRule_ = COM_NO_ACTION_DELETE_RULE;
    break;

  case ELM_REF_ACT_RESTRICT_ELEM :
    deleteRule_ = COM_RESTRICT_DELETE_RULE;
    break;

  case ELM_REF_ACT_SET_DEFAULT_ELEM :
    deleteRule_ = COM_SET_DEFAULT_DELETE_RULE;
    break;

  case ELM_REF_ACT_SET_NULL_ELEM :
    deleteRule_ = COM_SET_NULL_DELETE_RULE;
    break;

  default :
    NAAbort("ElemDDLRefTrigActions.C", __LINE__, "internal logic error");
    break;
  }
} // ElemDDLRefTrigActDeleteRule::ElemDDLRefTrigActDeleteRule()

// virtual destructor
ElemDDLRefTrigActDeleteRule::~ElemDDLRefTrigActDeleteRule()
{
}

// casting
ElemDDLRefTrigActDeleteRule *
ElemDDLRefTrigActDeleteRule::castToElemDDLRefTrigActDeleteRule()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLRefTrigActDeleteRule::displayLabel1() const
{
  return NAString("Delete Rule referential triggered action");
}

const NAString
ElemDDLRefTrigActDeleteRule::getText() const
{
  return "ElemDDLRefTrigActDeleteRule";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLRefTrigActUpdateRule
// -----------------------------------------------------------------------

//
// constructor
//

ElemDDLRefTrigActUpdateRule::ElemDDLRefTrigActUpdateRule(
     ElemDDLNode * pReferentialAction)
: ElemDDLRefTrigAct(ELM_REF_TRIG_ACT_UPDATE_RULE_ELEM,
                    pReferentialAction)
{
  ComASSERT(pReferentialAction NEQ NULL);
  switch (pReferentialAction->getOperatorType())
  {
  case ELM_REF_ACT_CASCADE_ELEM :
    updateRule_ = COM_CASCADE_UPDATE_RULE;
    break;

  case ELM_REF_ACT_NO_ACTION_ELEM :
    updateRule_ = COM_NO_ACTION_UPDATE_RULE;
    break;

  case ELM_REF_ACT_RESTRICT_ELEM :
    updateRule_ = COM_RESTRICT_UPDATE_RULE;
    break;

  case ELM_REF_ACT_SET_DEFAULT_ELEM :
    updateRule_ = COM_SET_DEFAULT_UPDATE_RULE;
    break;

  case ELM_REF_ACT_SET_NULL_ELEM :
    updateRule_ = COM_SET_NULL_UPDATE_RULE;
    break;

  default :
    NAAbort("ElemDDLRefTrigActions.C", __LINE__, "internal logic error");
    break;
  }
} // ElemDDLRefTrigActUpdateRule::ElemDDLRefTrigActUpdateRule()

// virtual destructor
ElemDDLRefTrigActUpdateRule::~ElemDDLRefTrigActUpdateRule()
{
}

// casting
ElemDDLRefTrigActUpdateRule *
ElemDDLRefTrigActUpdateRule::castToElemDDLRefTrigActUpdateRule()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLRefTrigActUpdateRule::displayLabel1() const
{
  return NAString("Update Rule referential triggered action");
}

const NAString
ElemDDLRefTrigActUpdateRule::getText() const
{
  return "ElemDDLRefTrigActUpdateRule";
}

//
// End of File
//
