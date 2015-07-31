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
 *****************************************************************************
 *
 * File:         RelDCL.C
 * Description:
 *
 *
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Sqlcomp.h"
#include "BindWA.h"
#include "RelScan.h"
#include "PhyProp.h"
#include "opt.h"
#include "RelDCL.h"


///////////////////////////////////////////////////////////////////
//
// methods for class RelLock
//
///////////////////////////////////////////////////////////////////
RelLock::RelLock(const CorrName& name,
		 LockMode lockMode,
		 NABoolean lockIndex,
		 OperatorTypeEnum otype,
		 NABoolean parallelExecution)
    : RelExpr(otype), userTableName_(name, CmpCommon::statementHeap()), 
      lockMode_(lockMode), lockIndex_(lockIndex),
      isView_(FALSE),
      parallelExecution_(parallelExecution), 
      baseTableNameList_(CmpCommon::statementHeap()),
      tabIds_(CmpCommon::statementHeap())
{ setNonCacheable(); }

RelLock::~RelLock()
{}

void RelLock::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear(); // for now, it produces no output values
}

RelExpr * RelLock::copyTopNode(RelExpr *derivedNode,CollHeap* outHeap)
{
  RelLock *result;

  if (derivedNode == NULL)
    result = new(outHeap) RelLock(userTableName_, lockMode_, lockIndex_,
				  getOperatorType(),
				  parallelExecution_);
  else
    result = (RelLock *) derivedNode;

  return RelExpr::copyTopNode(result, outHeap);
}

const NAString RelLock::getText() const
{
  NAString result(CmpCommon::statementHeap());

  if (getOperatorType() == REL_LOCK)
    result = "Lock ";
  else
    result = "Unlock ";

  return result + userTableName_.getText();
}

void RelLock::addTableNameToList(RelExpr * node)
{
  for (Int32 i=0; i < node->getArity(); i++)
    {
      addTableNameToList(node->child(i)->castToRelExpr());
    }

  if (node->getOperatorType() == REL_SCAN)
    baseTableNameList_.insert((CorrName *)(((Scan *)node)->getPtrToTableName()));
}

///////////////////////////////////////////////////////////////////
//
// methods for class RelTransaction
//
///////////////////////////////////////////////////////////////////
RelTransaction::RelTransaction(TransStmtType type, TransMode * mode,
			       ItemExpr * diagAreaSizeExpr)
    : RelExpr(REL_TRANSACTION),
      type_(type), mode_(mode),
      diagAreaSizeExpr_(diagAreaSizeExpr)
{
  setNonCacheable();
}

RelTransaction::~RelTransaction()
{
}

void RelTransaction::transformNode(NormWA & /*normWARef*/,
				   ExprGroupId & /*locationOfPointerToMe*/)
{
}

void RelTransaction::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear(); // for now, it produces no output values
}

RelExpr * RelTransaction::copyTopNode(RelExpr *derivedNode,CollHeap* outHeap)
{
  RelTransaction *result;

  if (derivedNode == NULL)
    result = new(outHeap)  RelTransaction(type_, mode_, diagAreaSizeExpr_);
  else
    result = (RelTransaction *) derivedNode;

  return RelExpr::copyTopNode(result, outHeap);
}

const NAString RelTransaction::getText() const
{
  NAString result(CmpCommon::statementHeap());

  switch (type_)
    {
    case BEGIN_:
      result = "Begin Work ";
      break;

    case COMMIT_:
      result = "Commit Work ";
      break;

    case ROLLBACK_:
      result = "Rollback Work ";
      break;

    case ROLLBACK_WAITED_:
      result = "Rollback Work Waited ";
      break;

    case SET_TRANSACTION_:
      result = "Set Transaction ";
      break;
    }

  return result;
}


///////////////////////////////////////////////////////////////////
//
//   methods for class RelSetTimeout
//
///////////////////////////////////////////////////////////////////
RelSetTimeout::RelSetTimeout(const CorrName & tableName,
			     ItemExpr * timeoutValueExpr,
			     NABoolean  isStreamTimeout, // default FALSE,
			     NABoolean  isReset,         // default FALSE
			     NABoolean  isForAllTables,  // default FALSE
			     char * physicalFileName ) // default NULL
  : RelExpr(REL_SET_TIMEOUT),
    userTableName_(tableName), 
    timeoutValueExpr_(timeoutValueExpr),
    isStream_(isStreamTimeout),
    isReset_(isReset),
    isForAllTables_(isForAllTables)
{
  setNonCacheable();
  CMPASSERT( ( NULL != timeoutValueExpr && ! isReset ) ||
	     ( NULL == timeoutValueExpr &&  isReset ) );
  if ( physicalFileName ) 
    setPhysicalFileName( physicalFileName );
}

void RelSetTimeout::transformNode(NormWA & /*normWARef*/,
				  ExprGroupId & /*locationOfPointerToMe*/)
{
  setNonCacheable();
}

void RelSetTimeout::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear(); // for now, it produces no output values
}

RelExpr * RelSetTimeout::copyTopNode(RelExpr *derivedNode,CollHeap* outHeap)
{
  RelSetTimeout *result;
  
  if (derivedNode == NULL)
    result = new(outHeap)  
      RelSetTimeout( userTableName_, timeoutValueExpr_, isStream_, isReset_,
		     isForAllTables_, physicalFileName_);
  else
    result = (RelSetTimeout *) derivedNode;

  return RelExpr::copyTopNode(result, outHeap);
}

const NAString RelSetTimeout::getText() const
{
  return "SET TIMEOUT";
}

