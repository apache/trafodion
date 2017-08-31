// **********************************************************************
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
// **********************************************************************

// ***********************************************************************
//
// File:         QRDescriptorExtentions.cpp
//
// Description:  QRDescriptor methods used only by MXCMP.
//               These methods are used when constructing an ItemExpr
//               expression tree from rewrite instructions in the 
//               result descriptor.
//
// Created:      08/24/2010
// ***********************************************************************

#include "QRDescriptor.h"
#include "QRLogger.h"
#include "ItemExpr.h"
#include "ItemFunc.h"
#include "ItemColRef.h"
#include "ItemFunc.h"
#include "ItemLog.h"
#include "ItemArith.h"
#include "NumericType.h"
#include "MVCandidates.h"

// ***************************************************************
// Use the ref attribute to find the ValueID the element was 
// constructed from, and from the ValueID get the ItemExpr
// object itself.
// ***************************************************************
ItemExpr* QRExplicitExpr::findItemExpr()
{
  CollIndex id = getRefNum();
  assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL, 
                    id!=0, QRLogicException, 
                    "findItemExpr() called with zero ValueID.");
  ValueId vid(id);
  return vid.getItemExpr();
}

// ***************************************************************
// This is a back-join or Outside column.
// ***************************************************************
ItemExpr* QRColumn::toItemExpr(const NAString& mvName, CollHeap* heap,
                               TableNameScanHash* scanhash)
{ 
  // Get the fully qualified table name by stripping the column name and last
  // dot from the fq column name, then create a CorrName to use in
  // constructing a ColRefName for the ColReference.
  NAString fqTblName(fqColumnName_);
  fqTblName.resize(fqColumnName_.length() - columnName_.length() - 1);
  QualifiedName tblQualName(fqTblName, 3, heap);
  NAString corrNameStr;
  // Add special correlation name if the column is from a backjoin table.
  if (scanhash->getFirstValue(&tableId_))
    (corrNameStr = MVCandidates::BACKJOIN_CORRNAME_PREFIX).append(tableId_);
  CorrName tblCorrName(tblQualName, heap, corrNameStr);
  return new(heap) ColReference(new(heap) 
                     ColRefName(columnName_, tblCorrName, heap)); 
}

// ***************************************************************
// This is a column of the MV.
// ***************************************************************
ItemExpr* QRMVColumn::toItemExpr(const NAString& mvName, CollHeap* heap,
                                 TableNameScanHash* scanhash)
{ 
  QualifiedName mvQualName(mvName, 3, heap);
  CorrName mvCorrName(mvQualName, heap);
  return new(heap) ColReference(new(heap) 
    ColRefName(getMVColName(), mvCorrName, heap));
}

// ***************************************************************
// This is a scalar value - create a new SystemLiteral for it.
// ***************************************************************
ItemExpr* QRScalarValue::toItemExpr(const NAString& mvName, CollHeap* heap,
                                    TableNameScanHash* scanhash)
{ 
  ConstValue* cv = NULL;
  ItemExpr* ie = findItemExpr();
  OperatorTypeEnum op = ie->getOperatorType();
  if (op == ITM_CACHE_PARAM)
    cv = ((ConstantParameter*)ie)->getConstVal();
  else
  {
    assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL, op == ITM_CONSTANT, 
                      QRLogicException, "Expecting a ConstValue object.");
    cv = (ConstValue*)ie;
  }
  return new(heap) SystemLiteral(cv->getType(), cv->getConstValue(), cv->getStorageSize()); 
}

// ***************************************************************
// This is a NULL value - much easier than the generic literal case.
// ***************************************************************
ItemExpr* QRNullVal::toItemExpr(const NAString& mvName, CollHeap* heap,
                                TableNameScanHash* scanhash)
{ 
  return new(heap) SystemLiteral(); 
}

// ***************************************************************
// A Binary operator - Dup the object and call recursively
// for the operands.
// ***************************************************************
ItemExpr* QRBinaryOper::toItemExpr(const NAString& mvName, CollHeap* heap,
                                   TableNameScanHash* scanhash)
{
  ItemExpr* oldObj = findItemExpr();
  ItemExpr* newObj = oldObj->copyTopNode(NULL, heap);
  newObj->child(0) = getFirstOperand()->toItemExpr(mvName, heap, scanhash);
  newObj->child(1) = getSecondOperand()->toItemExpr(mvName, heap, scanhash);
  return newObj;
}

// ***************************************************************
// A Unary operator - Dup the object and call recursively
// for the operand.
// ***************************************************************
ItemExpr* QRUnaryOper::toItemExpr(const NAString& mvName, CollHeap* heap,
                                  TableNameScanHash* scanhash)
{ 
  ItemExpr* oldObj = findItemExpr();
  ItemExpr* newObj = oldObj->copyTopNode(NULL, heap);
  newObj->child(0) = getOperand()->toItemExpr(mvName, heap, scanhash);
  return newObj;
}

// ***************************************************************
// A Function - Dup the object and call recursively
// for the arguments.
// If a ValueId is not referenced, then its an aggregate
// function for rollup - construct it manually.
// ***************************************************************
ItemExpr* QRFunction::toItemExpr(const NAString& mvName, CollHeap* heap,
                                 TableNameScanHash* scanhash)
{
  ItemExpr* newObj = NULL;
  
  // Is a ValueId referenced?
  if (getRefNum() != 0)
  {
    // Yes, this is the normal case.
    ItemExpr* oldObj = findItemExpr();
    newObj = oldObj->copyTopNode(NULL, heap);
    const NAPtrList<QRExplicitExprPtr>& args = getArguments();

    // For StdDev and Variance, all args must be double precision. 
    // otherwise, ScalarVariance::preCodeGen will GenAssert.
    NABoolean isScalarVariance = (getFunctionName() == "Scalar Variance"  ||
                                getFunctionName() == "Scalar Stddev" );
    NAType *dbl = new(heap) SQLDoublePrecision(heap, FALSE);

    // Loop over the arguments and call recursively.
    for (CollIndex i=0; i<args.entries(); i++)
    {
      if (args[i] != NULL)
      {
        ItemExpr* argument = args[i]->toItemExpr(mvName, heap, scanhash);
        
        if (isScalarVariance)
          argument = new(heap) Cast(argument, dbl);
      	
        newObj->child(i) = argument;
      }
    }
  }
  else
  {
    // In a Rollup case, we generate a new aggregate function,
    // so it does not have a ref attribute.
    assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                      isAnAggregate(), QRLogicException, 
                      "Only Aggregate functions are allowed not to reference a ValueID.");
    OperatorTypeEnum ieEnum = NO_OPERATOR_TYPE;
    NABoolean isDistinct = FALSE;
    
    ItemExpr* argument = getArguments()[0]->toItemExpr(mvName, heap, scanhash);
    switch(getAggregateFunc())
    {
      case AFT_COUNTSTAR:
      	ieEnum = ITM_COUNT;
      	break;
      case AFT_COUNT:
      	ieEnum = ITM_COUNT_NONULL;
      	break;
      case AFT_SUM:
      	ieEnum = ITM_SUM;
      	break;
      case AFT_MIN:
      	ieEnum = ITM_MIN;
      	break;
      case AFT_MAX:
      	ieEnum = ITM_MAX;
      	break;
      case AFT_COUNT_DISTINCT:
      	ieEnum = ITM_COUNT_NONULL;
      	isDistinct = TRUE;
      	break;
      case AFT_SUM_DISTINCT:
      	ieEnum = ITM_SUM;
      	isDistinct = TRUE;
      	break;
      case AFT_ONE_ROW:
      	ieEnum = ITM_ONE_ROW;
      	break;
      case AFT_ONEROW:
      	ieEnum = ITM_ONEROW;
      	break;
      case AFT_ONE_TRUE:
      	ieEnum = ITM_ONE_TRUE;
      	break;
      case AFT_ANY_TRUE:
      	ieEnum = ITM_ANY_TRUE;
      	break;

      case AFT_COUNT_ON_GROUPING:
      case AFT_SUM_ON_GROUPING:
      {
        // These are the special cases of rewriting COUNT and SUM on an
        // MV grouping column. Instead of providing an explicit expression,
        // QMS provides these internal "functions", which are implemented here.
        // These special functions have 2 arguments, where the second is the 
        // MV's COUNT(*) column (referenced below as countStar).
      	QRExplicitExprPtr countStarRewrite = getArguments()[1];
      	ItemExpr* countStar = countStarRewrite->toItemExpr(mvName, heap, scanhash);
      	
      	if (getAggregateFunc() == AFT_COUNT_ON_GROUPING)
      	{
      	  // COUNT(g) => SUM( CASE WHEN g IS NULL THEN 0 ELSE countStar END )
      	  ieEnum = ITM_SUM;
      	  argument = new(heap) 
            Case(NULL, new(heap)
      	      IfThenElse(new(heap) UnLogic(ITM_IS_NULL, argument),
      	                 new(heap) SystemLiteral(0),
      	                 countStar));
      	}
      	else
      	{
      	  // SUM(g) => SUM(g*countStar)
      	  ieEnum = ITM_SUM;
      	  argument = new(heap) BiArith(ITM_TIMES, argument, countStar);
      	}
      }

    } // end switch()
    newObj = new(heap) Aggregate(ieEnum, argument, isDistinct);
  }
  
  return newObj;
}
