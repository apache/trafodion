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
*
******************************************************************************
*/

//
// preCodeGen and codeGen implementations for Sequence Function 
// item expressions.
//

// Includes
//
#include "Generator.h"
#include "GenExpGenerator.h"
#include "exp_function.h"
#include "exp_math_func.h"
#include "CharType.h"
#include "NumericType.h"
#include "ExpSequenceFunction.h"
#include "ItmFlowControlFunction.h"


// ItmSeqOffset::preCodeGen
//
// Casts the second child to SqlInt.
//
ItemExpr *ItmSeqOffset::preCodeGen(Generator *generator)
{
  if (nodeIsPreCodeGenned())
    return this;
  
  CollHeap *wHeap = generator->wHeap();

  // The following code is being disabled (0 && ...) since it will
  // sometimes incorrectly think that the output of a tuple list of
  // contants is a single constant.  For example:
  //   SELECT a, b, c, MOVINGSUM(c,b) as MSUM, MOVINGAVG(c,b) as MAVG
  //   FROM (values
  //             (1,1, 1),
  //             (2,0, 2),
  //             (3,2, NULL),
  //             (4,0, 6),
  //             (5,3, 7)) as T(a,b,c)
  //   SEQUENCE BY a;
  //
  // For this query it will think that the offset index (b) is a
  // constant and it will use the value 3 for the
  // offsetConstantValue_.
  //
  if (0 && getArity() > 1)
  {
    NABoolean negate;
    ConstValue *cv = child(1)->castToConstValue(negate);
    if (cv AND cv->canGetExactNumericValue())
      {
        Lng32 scale;
        Int64 value = cv->getExactNumericValue(scale);

        if(scale == 0 && value >= 0 && value < INT_MAX) 
          {
            value = (negate ? -value : value);
            offsetConstantValue_ = (Int32)value;
            child(1) = NULL;
          }
      }
  }
    
  if (getArity() > 1)
  {

    const NAType &cType = child(1)->getValueId().getType();

    // (must be) signed; nulls allowed (if allowed by child1)   
   ItemExpr *castExpr   = new (wHeap) Cast (child(1),
                                           new (wHeap)
                                           SQLInt(wHeap, TRUE, cType.supportsSQLnullLogical()));
   castExpr->synthTypeAndValueId(TRUE);
   child (1) = castExpr;
  }
  return ItemExpr::preCodeGen(generator);
}

ItemExpr *ItmLeadOlapFunction::preCodeGen(Generator *generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (getArity() > 1)
  {
    const NAType &cType = child(1)->getValueId().getType();

    // (must be) signed; nulls allowed (if allowed by child1)   
    CollHeap *wHeap = generator->wHeap();
    ItemExpr *castExpr   = new (wHeap) Cast (child(1),
                                           new (wHeap)
                                           SQLInt(wHeap, TRUE, cType.supportsSQLnullLogical()));
    castExpr->synthTypeAndValueId(TRUE);
    child (1) = castExpr;
  }

  return ItemExpr::preCodeGen(generator);
}

ItemExpr *ItmLagOlapFunction::preCodeGen(Generator *generator)
{
  if (nodeIsPreCodeGenned())
    return this;
  
  CollHeap *wHeap = generator->wHeap();
    
  if (getArity() > 1)
  {

    const NAType &cType = child(1)->getValueId().getType();
    ItemExpr *castExpr   = new (wHeap) Cast (child(1),
                                       new (wHeap)
                                       SQLInt(wHeap, TRUE, cType.supportsSQLnullLogical()));
    castExpr->synthTypeAndValueId(TRUE);
    child (1) = castExpr;
  }
  return ItemExpr::preCodeGen(generator);
}
// ItmSeqOffset::codeGen
//
short ItmSeqOffset::codeGen(Generator* generator)
{
  Attributes** attr;
  Space* space = generator->getSpace();

  if(generator->getExpGenerator()->genItemExpr
     (this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause* seqClause 
    = new(space) ExpSequenceFunction(ITM_OFFSET,
				     getArity() + 1,
				     getOffsetConstantValue(),
				     attr,
				     space);

  if(nullRowIsZero())
    ((ExpSequenceFunction *)seqClause)->setNullRowIsZero(TRUE);

  ((ExpSequenceFunction *)seqClause)->setIsLeading(isLeading());
  ((ExpSequenceFunction *)seqClause)->setWinSize(winSize());

  if(isOLAP())
    ((ExpSequenceFunction *)seqClause)->setIsOLAP(TRUE);

  generator->getExpGenerator()->linkClause(this, seqClause);
  return 0;
}

// ItmSeqOlapFunction::codeGen
//
short ItmLeadOlapFunction::codeGen(Generator* generator)
{
  Attributes** attr;
  Space* space = generator->getSpace();

  if(generator->getExpGenerator()->genItemExpr
     (this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause* seqClause 
    = new(space) ExpSequenceFunction(ITM_OFFSET, 
				     getArity() + 1,
				     getOffset(),
				     attr,
				     space);

  ((ExpSequenceFunction *)seqClause)->setNullRowIsZero(FALSE);

  ((ExpSequenceFunction *)seqClause)->setIsLeading(TRUE);

  // Set the window size to 0 to evalauate LEAD() to NULL
  // for last few rows in each group. Please refer to 
  // GetHistoryRowFollowingOLAP() for the case returning -3. 
  ((ExpSequenceFunction *)seqClause)->setWinSize(0);

  if(isOLAP())
    ((ExpSequenceFunction *)seqClause)->setIsOLAP(TRUE);

  generator->getExpGenerator()->linkClause(this, seqClause);
  return 0;
}

short ItmLagOlapFunction::codeGen(Generator* generator)
{
  Attributes** attr;
  Space* space = generator->getSpace();

  if(generator->getExpGenerator()->genItemExpr
     (this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause* seqClause 
    = new(space) ExpSequenceFunction(ITM_OFFSET,
				     getArity() + 1,
				     0,
				     attr,
				     space);

  ((ExpSequenceFunction *)seqClause)->setNullRowIsZero(FALSE);

  ((ExpSequenceFunction *)seqClause)->setIsLeading(TRUE);
  ((ExpSequenceFunction *)seqClause)->setWinSize(0);

  if(isOLAP())
    ((ExpSequenceFunction *)seqClause)->setIsOLAP(TRUE);

  generator->getExpGenerator()->linkClause(this, seqClause);
  return 0;
}

// ItmSeqRunningFunction::preCodeGen
//
// Transforms the running sequence functions into scalar expressions
// that use offset to reference the previous value of the function.
//
ItemExpr *ItmSeqRunningFunction::preCodeGen(Generator *generator)
{
  if (nodeIsPreCodeGenned())
    return this;
  markAsPreCodeGenned();

  // Get some local handles...
  //
  CollHeap *wHeap = generator->wHeap();
  ItemExpr *itmChild = child(0)->castToItemExpr();

  // Allocate a HostVar for referencing the result before it is computed.
  //
  ItemExpr *itmResult 
    = new(wHeap) HostVar("_sys_Result",
			 getValueId().getType().newCopy(wHeap),
			 TRUE);

  // Create an item expression to reference the previous
  // value of this running sequence function.
  //
  ItemExpr *offExpr = new(wHeap) ItmSeqOffset(itmResult, 1);
  ((ItmSeqOffset *)offExpr)->setIsOLAP(isOLAP());
  // Add the sequence function specific computation.
  //
  ItemExpr *itmNewSeqFunc = 0;
  switch(getOperatorType())
    {
    case ITM_RUNNING_COUNT:
      {
	// By this point ITM_RUNNING_COUNT is count(column). The count
	// is one more than the previous count if the current column is
	// not null, otherwise, it is the previous count.
	//

        // Create the increment value.  For non-nullable values, this
        // is always 1, essentially runningcount(*).
        //
        ItemExpr *incr;
        if(itmChild->getValueId().getType().supportsSQLnullLogical()) {
          incr = generator->getExpGenerator()->createExprTree
            ("CASE WHEN @A1 IS NULL THEN @A3 ELSE @A2 END", 
             0, 3, 
             itmChild,
             new(wHeap) ConstValue(1),
             new(wHeap) ConstValue(0));
        } else {
          incr = new(wHeap) ConstValue(1);
        }

        ((ItmSeqOffset *)offExpr)->setNullRowIsZero(TRUE);
        ItemExpr *src = offExpr;

        // Do the increment.
        //
        itmNewSeqFunc = new(wHeap)
              BiArith(ITM_PLUS, src, incr);
      }
    break;
    
    case ITM_RUNNING_SUM:
      {
	// SUM(sum from previous row, child)
	//
	itmNewSeqFunc = new(wHeap) BiArithSum(ITM_PLUS, offExpr, itmChild);
      }
    break;
    
    case ITM_RUNNING_MIN:
      {
	// MIN(min from previous rows, child)
	//
	itmNewSeqFunc 
	  = new(wHeap) ItmScalarMinMax(ITM_SCALAR_MIN,
				       offExpr,
				       itmChild);
      }
    break;
    
    case ITM_RUNNING_MAX:
      {
	// MAX(max from previous row, child)
	//
	itmNewSeqFunc 
	  = new(wHeap) ItmScalarMinMax(ITM_SCALAR_MAX,
				       offExpr,
				       itmChild);
      }
    break;
    
    case ITM_LAST_NOT_NULL:
      {
	// If the current value is null then use the previous value
	// of last not null.
	//
	itmNewSeqFunc = generator->getExpGenerator()->createExprTree
	  ("CASE WHEN @A2 IS NOT NULL THEN @A2 ELSE @A1 END", 
	   0, 2, offExpr, itmChild);
      }
    break;

    case ITM_RUNNING_CHANGE:
      {
        // The running change (or 'rows since changed') can have a
        // composite child (a list of values)
        // Convert the list of values to a list of offset of values.
        //
        ItemExpr *offChild = itmChild;
      
        if (itmChild->getOperatorType() == ITM_ITEM_LIST)
          {
            // child is a multi-valued expression, transform into multiple
            // 
            ExprValueId treePtr = itmChild;

            ItemExprTreeAsList changeValues(&treePtr,
                                            ITM_ITEM_LIST,
                                            RIGHT_LINEAR_TREE);

            offChild = new(wHeap) ItmSeqOffset( changeValues[0], 1);
	    ((ItmSeqOffset *)offChild)->setIsOLAP(isOLAP());
            // add Offset expressions for all the items of the list
            // 
            CollIndex nc = changeValues.entries();
            for (CollIndex i = 1; i < nc; i++)
              {
                ItemExpr *off = new(generator->wHeap()) ItmSeqOffset( changeValues[i], 1);
		((ItmSeqOffset *)off)->setIsOLAP(isOLAP());
                offChild = new(generator->wHeap()) ItemList(offChild, off);
              }
          } else {
            offChild = new(wHeap) ItmSeqOffset( offChild, 1);
	    ((ItmSeqOffset *)offChild)->setIsOLAP(isOLAP());
          }
        
 
        ((ItmSeqOffset *)offExpr)->setNullRowIsZero(TRUE);
        ItemExpr *prevValue = offExpr;

        // Compare the value(s) to the previous value(s).  Use special
        // NULLs flags to treat NULLs as values.  Two NULL values are
        // considered equal here.
        //
        ItemExpr *pred = new (wHeap) BiRelat(ITM_EQUAL,
                                             itmChild,
                                             offChild,
                                             TRUE); // Special NULLs
        // running change = 
        //      (value(s) == prev(value(s))) ? prev(running change)+1 : 1
        //
        // Compute base value.
        //
        itmNewSeqFunc = new (wHeap) 
                IfThenElse(pred, prevValue, new (wHeap) SystemLiteral(0));
        
        itmNewSeqFunc = new (wHeap) Case(NULL, itmNewSeqFunc);

        // Force the evaluation of the offset expression so that the
        // result can be reused by subsequent references.
        //
        itmNewSeqFunc = new(wHeap) ItmBlockFunction(offChild, itmNewSeqFunc);

        // Increment the base value.
        //
        itmNewSeqFunc = new (wHeap) BiArith(ITM_PLUS, 
                                            itmNewSeqFunc,
                                            new(wHeap) SystemLiteral(1));

      }
      break;
      
    }
  
  // Get value Ids and types for all of the items. Must do this typing before
  // replacing this value Id's item expression -- otherwise, the typing
  // will give a result different than the type already computed for this
  // sequence function.
  //
  GenAssert(itmNewSeqFunc, "ItmSeqRunningFunction::preCodeGen -- Unexpected Operator Type!");
  itmNewSeqFunc->synthTypeAndValueId(TRUE);

  // Replace the original value ID with the new expression.
  //
  getValueId().replaceItemExpr(itmNewSeqFunc);
  
  // Map the reference to the result to the actual result in the map table.
  //
  Attributes *attr =  generator->getMapInfo
    (itmNewSeqFunc->getValueId())->getAttr();
  MapInfo *mapInfo = generator->addMapInfo(itmResult->getValueId(), attr);
  itmResult->markAsPreCodeGenned();
  mapInfo->codeGenerated();

  // Return the preCodeGen of the new expression.
  //
  return itmNewSeqFunc->preCodeGen(generator);
}

ItemExpr *ItmSeqOlapFunction::preCodeGen(Generator *generator)
{
  if (getOperatorType() != ITM_OLAP_MIN && getOperatorType() != ITM_OLAP_MAX)
  {
    GenAssert(0, "ItmSeqOlapFunction::preCodeGen -- Should never get here!");
    return 0;
  }


  if (nodeIsPreCodeGenned())
    return this;
  markAsPreCodeGenned();

  // Get some local handles...
  //
  CollHeap *wHeap = generator->wHeap();
  ItemExpr *itmChild = child(0)->castToItemExpr();
  //ItemExpr *itmWindow = child(1)->castToItemExpr();

  // What scalar operation needs to be done.
  //
  OperatorTypeEnum operation;
  if(getOperatorType() == ITM_OLAP_MIN) operation = ITM_SCALAR_MIN;
  else operation = ITM_SCALAR_MAX;

  // Allocate a HostVar for local storage of the index.
  //
  ItemExpr *itmLocalCounter 
    = new(wHeap) HostVar("_sys_LocalCounter",
			 new(wHeap) SQLInt(wHeap, TRUE,FALSE),
			 TRUE);

  // Expression to initailize the iterator.
  //
  ItemExpr *itmLocalCounterInitialize
              = new(wHeap) Assign(itmLocalCounter, 
                                  new(wHeap) ConstValue(frameStart_),
			          FALSE);

  // Expression to increment the iterator.
  //
  ItemExpr *itmLocalCounterIncrement
    = new(wHeap) Assign(itmLocalCounter,
			new(wHeap) BiArith(ITM_PLUS,
					   itmLocalCounter,
					   new (wHeap) ConstValue(1)),
			FALSE);

  // Allocate a HostVar for referencing the result before it is computed.
  //
  ItemExpr *itmResult 
    = new(wHeap) HostVar("_sys_Result",
			 getValueId().getType().newCopy(wHeap),
			 TRUE);

  // Expression to initialize the result.
  //
  ItemExpr *itmResultInitialize
    = new(wHeap) Assign(itmResult,
			new(wHeap) ConstValue());
			
  // Expression to compute the min/max.
  //

  ItemExpr * invCouter= new(wHeap) BiArith(ITM_MINUS,
                                                new (wHeap) ConstValue(0),
					        itmLocalCounter);
					   
  ItemExpr *  itmOffsetExpr = new(wHeap) ItmSeqOffset( itmChild, invCouter);


  //ItemExpr * itmOffsetIsNotNull = new (wHeap) UnLogic(ITM_IS_NOT_NULL, itmOffsetExpr);

  ((ItmSeqOffset *)itmOffsetExpr)->setIsOLAP(isOLAP());
  ItemExpr *itmResultUpdate
    = new(wHeap) Assign(itmResult,
			new(wHeap) ItmScalarMinMax(operation, 
						   itmResult, 
						   itmOffsetExpr));

  // Construct code blocks for the initialization and body for the while-loop
  //
  ItemExpr *itmInit 
    = new(wHeap) ItmBlockFunction(itmLocalCounterInitialize,
				  itmResultInitialize);
  ItemExpr *itmBody
    = new(wHeap) ItmBlockFunction(itmResultUpdate,
				  itmLocalCounterIncrement);
  
  // Construct the While loop (i < window)
  //
  ItemExpr *itmLoopCondition = new(wHeap) BiRelat
    (ITM_LESS_EQ, itmLocalCounter, new(wHeap) ConstValue(frameEnd_));
  
  if (isFrameEndUnboundedFollowing()) //(frameEnd_ == INT_MAX)// not needed in other cases -- can cause issues fo the preceding part
  {
    ItemExpr *  itmOffset1 = new(wHeap) ItmSeqOffset( itmChild, invCouter,NULL,TRUE);
    ItemExpr * itmOffset1IsNotNull = new (wHeap) UnLogic(ITM_IS_NOT_NULL, itmOffset1);

    ((ItmSeqOffset *)itmOffset1)->setIsOLAP(isOLAP());

    itmLoopCondition = itmOffset1IsNotNull;
    //new (wHeap) BiLogic( ITM_AND,
                        //                  itmLoopCondition,
                       //                   itmOffset1IsNotNull);
  }
  ItemExpr *itmWhile 
    = new(wHeap) ItmWhileFunction(itmBody,
				    itmLoopCondition);
  
  
  // Construct the blocks to contain the initialization and looping.
  // The result is the final value of the min/max.
  //
  ItemExpr *itmBlock = new(wHeap) ItmBlockFunction
    (new(wHeap) ItmBlockFunction(itmInit, itmWhile),
     itmResult);
  
  // Replace the item for this value id with the new item expression.
  //
  getValueId().replaceItemExpr(itmBlock);

  // Run the new expression through type and value Id synthesis.
  //
  itmBlock->synthTypeAndValueId(TRUE);

  // Map the reference to the result to the actual result in the map table.
  //
  Attributes *attr =  generator->getMapInfo(itmBlock->getValueId())->getAttr();
  MapInfo *mapInfo = generator->addMapInfo(itmResult->getValueId(), attr);
  itmResult->markAsPreCodeGenned();
  mapInfo->codeGenerated();

  // Return the preCodeGen of the new expression.
  //
  return itmBlock->preCodeGen(generator);

}
// ItmSeqRunningFunction::codeGen
//
// ItmSeqRunningFunction is transformed away in preCodeGen -- see above.
//
short ItmSeqRunningFunction::codeGen(Generator* generator)
{
  GenAssert(0, "ItmSeqRunningFunction::codeGen -- Should never get here!");
  return 0;
}

// ItmSeqMovingFunction::preCodeGen
//
// All of the moving sequence functions have been transformed away at this
// point except min and max. Transform these operations to a while-loop which 
// iterates over the past rows testing the min/max condition for each row. 
// Use the ItmScalarMin/Max functions for computing the min/max.
//
ItemExpr *ItmSeqMovingFunction::preCodeGen(Generator *generator)
{
  if (nodeIsPreCodeGenned())
    return this;
  markAsPreCodeGenned();

  // Get some local handles...
  //
  CollHeap *wHeap = generator->wHeap();
  ItemExpr *itmChild = child(0)->castToItemExpr();
  ItemExpr *itmWindow = child(1)->castToItemExpr();

  // What scalar operation needs to be done.
  //
  OperatorTypeEnum operation;
  if(getOperatorType() == ITM_MOVING_MIN) operation = ITM_SCALAR_MIN;
  else operation = ITM_SCALAR_MAX;

  // Allocate a HostVar for local storage of the index.
  //
  ItemExpr *itmLocalCounter 
    = new(wHeap) HostVar("_sys_LocalCounter",
			 new(wHeap) SQLInt(wHeap, TRUE,FALSE),
			 TRUE);

  // Expression to initailize the iterator.
  //
  ItemExpr *itmLocalCounterInitialize
    = new(wHeap) Assign(itmLocalCounter, 
			new(wHeap) ConstValue(0),
			FALSE);

  // Expression to increment the iterator.
  //
  ItemExpr *itmLocalCounterIncrement
    = new(wHeap) Assign(itmLocalCounter,
			new(wHeap) BiArith(ITM_PLUS,
					   itmLocalCounter,
					   new (wHeap) ConstValue(1)),
			FALSE);

  // Allocate a HostVar for referencing the result before it is computed.
  //
  ItemExpr *itmResult 
    = new(wHeap) HostVar("_sys_Result",
			 getValueId().getType().newCopy(wHeap),
			 TRUE);

  // Expression to initialize the result.
  //
  ItemExpr *itmResultInitialize
    = new(wHeap) Assign(itmResult,
			new(wHeap) ConstValue());
			
  // Expression to compute the min/max.
  //
  ItemExpr *itmOffsetExpr = new(wHeap) ItmSeqOffset( itmChild, itmLocalCounter);
  ((ItmSeqOffset *)itmOffsetExpr)->setIsOLAP(isOLAP());
  ItemExpr *itmResultUpdate
    = new(wHeap) Assign(itmResult,
			new(wHeap) ItmScalarMinMax(operation, 
						   itmResult, 
						   itmOffsetExpr));

  // Construct code blocks for the initialization and body for the while-loop
  //
  ItemExpr *itmInit 
    = new(wHeap) ItmBlockFunction(itmLocalCounterInitialize,
				  itmResultInitialize);
  ItemExpr *itmBody
    = new(wHeap) ItmBlockFunction(itmResultUpdate,
				  itmLocalCounterIncrement);
  
  // Construct the While loop (i < window)
  //
  ItemExpr *itmLoopCondition = new(wHeap) BiRelat
    (ITM_LESS, itmLocalCounter, itmWindow);
  ItemExpr *itmWhile 
    = new(wHeap) ItmWhileFunction(itmBody,
				    itmLoopCondition);
  
  
  // Construct the blocks to contain the initialization and looping.
  // The result is the final value of the min/max.
  //
  ItemExpr *itmBlock = new(wHeap) ItmBlockFunction
    (new(wHeap) ItmBlockFunction(itmInit, itmWhile),
     itmResult);
  
  // Replace the item for this value id with the new item expression.
  //
  getValueId().replaceItemExpr(itmBlock);

  // Run the new expression through type and value Id synthesis.
  //
  itmBlock->synthTypeAndValueId(TRUE);

  // Map the reference to the result to the actual result in the map table.
  //
  Attributes *attr =  generator->getMapInfo(itmBlock->getValueId())->getAttr();
  MapInfo *mapInfo = generator->addMapInfo(itmResult->getValueId(), attr);
  itmResult->markAsPreCodeGenned();
  mapInfo->codeGenerated();

  // Return the preCodeGen of the new expression.
  //
  return itmBlock->preCodeGen(generator);
}

// ItmSeqMovingFunction::codeGen
//
// ItmSeqMovingFunction is transformed away in preCodeGen -- see above.
//
short ItmSeqMovingFunction::codeGen(Generator* generator)
{
  GenAssert(0, "ItmSeqMovingFunction::codeGen -- Should never get here!");
  return 0;
}

// ItmSeqRowsSince::preCodeGen
//
// Transform ItmSeqRowsSince to a do-while which iterates over the
// past rows testing the since condition for each row. The expression
// returns the relative index of the first row in which the since condition is
// TRUE or the number of rows in the history buffer + 1.
//
ItemExpr *ItmSeqRowsSince::preCodeGen(Generator *generator)
{
  if (nodeIsPreCodeGenned())
    return this;
  markAsPreCodeGenned();

  // Get some local handles...
  //
  CollHeap *wHeap = generator->wHeap();
  ItemExpr *itmChild = child(0)->castToItemExpr();
  ItemExpr *itmWindow = NULL;
  if(getArity() > 1) itmWindow = child(1)->castToItemExpr();

  // Allocate a counter for iterating through the past rows. The counter
  // also doubles as the result of the rows since. This requires the
  // counter to be nullable in case the condition is never true.
  //
  ItemExpr *itmLocalCounter 
    = new(wHeap) ItmExpressionVar(getValueId().getType().newCopy(wHeap));

  // If the ROWS SINCE is inclusive start the iterator at -1 to
  // include the current row, otherwise, start the iterator at 0 to
  // skip the current row. (The iterator is incremented before testing
  // the condition since a DoWhile loop is used -- thus, the iterator
  // is not starting at 0 and 1.)
  //
  ItemExpr *startExpr;
  if(this->includeCurrentRow()) startExpr = new(wHeap) ConstValue(-1);
  else startExpr = new(wHeap) ConstValue(0);
  
  // Expression to initialize the iterator. -- and put the variable in
  // the map table before it is used in an assignment.
  //
  ItemExpr *itmInitializeCounter = new(wHeap) ItmBlockFunction
    (itmLocalCounter,
     new(wHeap) Assign(itmLocalCounter, 
		       startExpr,
		       FALSE));
  
  // Expression to increment the iterator.
  //
  ItemExpr *itmIncrementCounter = new(wHeap) Assign
    (itmLocalCounter,
     new(wHeap) BiArith(ITM_PLUS,
			itmLocalCounter,
			new (wHeap) ConstValue(1)),
     FALSE);

  // Expression to make the counter NULL.
  //
  ItemExpr *itmNullCounter
    = new(wHeap) Assign(itmLocalCounter,
			new(wHeap) ConstValue(),
			FALSE);
  
  // Expression for DoWhile looping condition.
  //
  // AND(0) returns TRUE if the looping should continue, FALSE otherwise.
  // The looping should continue as long as the counter is within the
  // row history. the counter is less than the maximum search offset, and
  // the condition has not returned TRUE.
  //
  // The left side of AND(0) returns TRUE if the OFFSET of the current
  // counter value is within the row history and the counter is less
  // than the maximum search offset. If the left side of the AND(0)
  // returns FALSE, the counter is set to NULL because the condition
  // was not found to be true withing the search window -- and the
  // counter doubles as the result. In this case the right side of the
  // AND(0) is not evaluated because of short circuit evaluation.
  //
  // The right side of AND(0) returns TRUE if the condition is not TRUE
  // relative to the row indicated by counter.
  //
  //                 
  //                                    AND(0)
  //                               ____/      \___
  //                              /               \
  //                             /                 \
  //                       _ OR _                   IS_NOT_TRUE
  //                      /      \                      |    
  //                     /        BLOCK               OFFSET
  //               AND(2)        /     \             /      \
  //              /     \  Counter=NULL FALSE  ROWS SINCE  Counter     
  //    IS_NOT_NULL      \                       (cond)      
  //        /        LESS THAN               
  //       /         /       \               
  //     OFFSET   Counter  MaxWindow  
  //    /      \                  
  //ROWS SINCE  Counter           
  //  (cond)
  //        

  // Expression to test if counter is within history range. 
  //
  ItemExpr *itmRangeIndicator =  new (wHeap) ItmSeqOffset(
                                                          new (wHeap) UnLogic
                                                         (ITM_IS_TRUE,itmChild),
							  itmLocalCounter);
  ((ItmSeqOffset *)itmRangeIndicator)->setIsOLAP(isOLAP());
  // Expression to compute AND(2). If the window is not specified, then
  // return the left child of AND(2).
  //
  ItemExpr *itmAnd2;
  if(itmWindow)
    {
      itmAnd2 = new(wHeap) BiLogic
	(ITM_AND,
	 new(wHeap) UnLogic (ITM_IS_NOT_NULL, itmRangeIndicator),
	 new(wHeap) BiRelat (ITM_LESS_EQ, itmLocalCounter, itmWindow));
    }
  else
    {
      itmAnd2 = new(wHeap) UnLogic (ITM_IS_NOT_NULL, itmRangeIndicator);
    }

  // Expression to compute OR
  //
  ItemExpr *itmOr = new(wHeap) BiLogic
    (ITM_OR,
     itmAnd2,
     new(wHeap) ItmBlockFunction(itmNullCounter, new(wHeap) ConstValue(0)));

  // Expression to compute AND(0)
  //
  ItemExpr *itmLoopCondition = new(wHeap) BiLogic
    (ITM_AND, 
     itmOr, 
     new(wHeap) UnLogic(ITM_NOT, new (wHeap) UnLogic(ITM_IS_TRUE,itmChild)));

  // Construct the Do-While loop
  //
  ItemExpr *itmDoWhile 
    = new(wHeap) ItmDoWhileFunction(itmIncrementCounter,
				    itmLoopCondition);
  
  // Construct the counter initialization along with the looping.
  //
  ItemExpr *itmBlockPart
    = new(wHeap) ItmBlockFunction(itmInitializeCounter,
				  itmDoWhile);

  // Finally, construct a block to execute the operation and return
  // the FINAL value of the counter as the result. This is tricky, because
  // the do-while returns the value of the counter for the last time
  // the body (the increment expression) is executed which may be
  // different than the last value of the counter.
  //
  ItemExpr *itmBlock
    = new(wHeap) ItmBlockFunction(itmBlockPart, itmLocalCounter);
  
  // Replace the item for this value id with the new result item expression.
  //
  getValueId().replaceItemExpr(itmBlock);

  // Run the new expression through type and value Id synthesis.
  //
  itmBlock->synthTypeAndValueId(TRUE);

  // Save the previous rows since counter and set to the current one.
  //
  ItemExpr *savedRowsSinceCounter 
    = generator->getExpGenerator()->getRowsSinceCounter();

  generator->getExpGenerator()->setRowsSinceCounter (itmLocalCounter);

  // Save the preCodeGen of the new expression.
  //
  ItemExpr *tmpBlock = itmBlock->preCodeGen(generator);

  // Restore the saved ROWS SINCE counter expression.
  //
  generator->getExpGenerator()->setRowsSinceCounter (savedRowsSinceCounter);

  // return the saved expression
  //
  return tmpBlock;
}


// ItmSeqRowsSince::codeGen
//
// ItmSeqRowsSince is transformed away in preCodeGen -- see above.
//
short ItmSeqRowsSince::codeGen(Generator* generator)
{
  GenAssert(0, "ItmSeqRowsSince::codeGen -- Should never get here!");
  return 0;
}

// ItmSeqDiff1::codeGen
//
// ItmSeqDiff1 is transformed away in normalize.
//
short ItmSeqDiff1::codeGen(Generator* generator)
{
  GenAssert(0, "ItmSeqDiff1::codeGen -- Should never get here!");
  return 0;
}

// ItmSeqDiff2::codeGen
//
// ItmSeqDiff2 is transformed away in normalize.
//
short ItmSeqDiff2::codeGen(Generator* generator)
{
  GenAssert(0, "ItmSeqDiff2::codeGen -- Should never get here!");
  return 0;
}

// ItmSeqThis::preCodeGen
//
ItemExpr *ItmSeqThisFunction::preCodeGen(Generator* generator)
{
  return child(0)->preCodeGen(generator); 
}

// ItmSeqThis::CodeGen
//
short ItmSeqThisFunction::codeGen(Generator* generator)
{
  GenAssert(0, "ItmSeqThis::codeGen -- Should never get here!");
  return 0;
}
// ItmSeqNotTHISFunction::preCodeGen
//
// Transforms the NOT THIS sequence function into an offset of its child
// that uses the saved ROWS SINCE offset in the ExpGenerator. This allows
// the entire part of the expression which changes with each history row to be 
// calculated inside a single offset expression. All other parts of the 
// expression are below THIS, i.e., in the current row.
//
// Note: NOT THIS expressions occur only within a ROWS SINCE.
//
// EXAMPLE:
//   select runningsum(this(a)), 
//          rows since (this (b) > a * (c+5))  
//          from iTab2 sort by a;
//
//          rows since      ----->  becomes:     rows since
//                |                                    |
//                >                                    >
//               /  \                                 /  \
//           this   not this                      this      OFFSET                     
//             /          \                         /        /  \                      
//            b            *                       b        *   <not THIS Loop counter>
//                        / \                              / \                         
//                       a   +                            a   +                        
//                          / \                              / \    
//                         c   5                            c   5
//                                                  
//
ItemExpr *ItmSeqNotTHISFunction::preCodeGen(Generator *generator)
{
  if (nodeIsPreCodeGenned())
    return this;
  markAsPreCodeGenned();

  // Get some local handles...
  //
  CollHeap *wHeap = generator->wHeap();
  ItemExpr *itmChild = child(0)->castToItemExpr();
  ItemExpr *savedRowsSinceCounter =
                 generator->getExpGenerator()->getRowsSinceCounter();

  GenAssert(savedRowsSinceCounter, "ItmSeqNotTHIS::preCodeGen -- ROWS SINCE counter is NULL.");

  // Generate the new OFFSET expression
  //
  ItemExpr *offExpr = new(wHeap) ItmSeqOffset(itmChild, savedRowsSinceCounter);
  ((ItmSeqOffset *)offExpr)->setIsOLAP(isOLAP());
  // Get value Ids and types for all of the items. Must do this typing before
  // replacing this value Id's item expression -- otherwise, the typing
  // will give a result different than the type already computed for this
  // sequence function.
  //
  offExpr->synthTypeAndValueId(TRUE);

  // Replace the original value ID with the new expression.
  //
  getValueId().replaceItemExpr(offExpr);

  // Return the preCodeGen of the new OFFSET expression.
  //
  return offExpr->preCodeGen(generator);
}

// ItmSeqNotTHIS::CodeGen
//
short ItmSeqNotTHISFunction::codeGen(Generator* generator)
{
  GenAssert(0, "ItmSeqNotTHIS::codeGen -- Should never get here!");
  return 0;
}

// A transformation method for protecting sequence functions from not
// being evaluated due to short-circuit evaluation. This is the base
// class implementation which simply recurses on the children unless
// they have already been code-generated.
//
void ItemExpr::protectiveSequenceFunctionTransformation(Generator *generator)
{
  for(Int32 i=0; i<getArity(); i++)
    {
      MapInfo *mapInfo = generator->getMapInfoAsIs(child(i));
      if(!mapInfo || !mapInfo->isCodeGenerated())
	child(i)->protectiveSequenceFunctionTransformation(generator);
    }
}

// A transformation method for protecting sequence functions from not
// being evaluated due to short-circuit evaluation. 
//
void BiLogic::protectiveSequenceFunctionTransformation(Generator *generator)
{
  // Recurse on the children
  //
  ItemExpr::protectiveSequenceFunctionTransformation(generator);

  // Remove the original value id from the node being transformed and
  // assign it a new value id.
  //
  ValueId id = getValueId();
  setValueId(NULL_VALUE_ID);
  synthTypeAndValueId(TRUE);

  // Construct the new subtree.
  //
  // AND/OR -- force right child evaluation
  //
  // LOGIC(LEFT_CHILD, RIGHT_CHILD) ==>
  //   BLOCK(RIGHT_CHILD, LOGIC(LEFT_CHILD, RIGHT_CHILD))
  //
  ItemExpr *block = new(generator->wHeap()) ItmBlockFunction(child(1), this);

  // Replace the old expression with the new expression for the 
  // orginal value id
  //
  id.replaceItemExpr(block);

  // Run the new expression through type and value id synthesis
  //
  block->synthTypeAndValueId(TRUE);
}

// A transformation method for protecting sequence functions from not
// being evaluated due to short-circuit evaluation.
//
void Case::protectiveSequenceFunctionTransformation(Generator *generator)
{
  // Recurse on the children
  //
  ItemExpr::protectiveSequenceFunctionTransformation(generator);

  // Remove the original value id from the node being transformed and
  // assign it a new value id.
  //
  ValueId id = getValueId();
  setValueId(NULL_VALUE_ID);
  synthTypeAndValueId(TRUE);

  // Construct the new subtree.
  //
  // Case -- force evaluation of all the WHEN, THEN and ELSE parts
  //
  // CASE(IFE1(W1,T1,IFE2(W2,T2,IFE3(...)))) ==>
  //   BLOCK(BLOCK(BLOCK(W1,T1),BLOCK(W2,T2)), CASE(...))
  //
  // Decend the ITM_IF_THEN_ELSE tree pulling out each WHEN and THEN pair.
  // Mate each pair with a block and attach them to the protected block, 
  // which contains all of the WHEN/THEN pairs for the entire tree.
  // Also, pull out any CASE operands and attach them to the protected
  // block as well.
  //
  ItemExpr *block = NULL;
  ItemExpr *ife = child(0);
  for(; (ife != NULL) && (ife->getOperatorType() == ITM_IF_THEN_ELSE);
       ife = ife->child(2))
    {
      ItemExpr *sub = new(generator->wHeap())
	ItmBlockFunction(ife->child(0), ife->child(1));
      if(block)
	block = new(generator->wHeap()) ItmBlockFunction(sub, block);
      else
	block = sub;
    }      

  // Add the ELSE condition, if any to the protected block
  //
  if(ife)
    block = new(generator->wHeap()) ItmBlockFunction(ife, block);

  // Construct the top-level block function. The left child is the protected
  // block, which contains all of the expresssions that need to be 
  // pre-evaluated. This right child is the original case statement.
  //
  block = new(generator->wHeap()) ItmBlockFunction(block, this);

  // Replace the old expression with the new expression for the
  // original id
  //
  id.replaceItemExpr(block);

  // Run the new expression through type and value id synthesis
  //
  block->synthTypeAndValueId(TRUE);
}

// A transformation method for protecting sequence functions from not
// being evaluated due to short-circuit evaluation. 
//
void ItmScalarMinMax::protectiveSequenceFunctionTransformation
(Generator *generator)
{
  // Recurse on the children
  //
  ItemExpr::protectiveSequenceFunctionTransformation(generator);

  // Remove the original value id from the node being transformed.
  //
  ValueId id = getValueId();
  setValueId(NULL_VALUE_ID);
  synthTypeAndValueId(TRUE);

  // Construct the new subtree.
  //
  // SCALAR_MIN/MAX -- force evaluation of both children
  //
  // SCALAR(LEFT_CHILD, RIGHT_CHILD) ==>
  //   BLOCK(BLOCK(LEFT_CHILD, RIGHT_CHILD), 
  //         SCALAR(LEFT_CHILD, RIGHT_CHILD))
  // 
  ItemExpr *block = new(generator->wHeap()) ItmBlockFunction
    (new(generator->wHeap()) ItmBlockFunction(child(0), child(1)), this);
  
  // Replace the old expression with the new expression for the 
  // orginal value id
  //
  id.replaceItemExpr(block);

  // Run the new expression through type and value id synthesis
  //
  block->synthTypeAndValueId(TRUE);
}

