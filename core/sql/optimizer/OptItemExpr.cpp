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
* File:         OptItemExpr.C
* Description:  Item expressions (optimizer-related methods)
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "AllItemExpr.h"
#include "Cost.h"         /* for lookups in the defaults table */


// -----------------------------------------------------------------------
//  Methods for ItemExpr
// -----------------------------------------------------------------------
NABoolean ItemExpr::isAnEquiJoinPredicate
                       (const GroupAttributes* const leftGroupAttr,
			const GroupAttributes* const rightGroupAttr,
			const GroupAttributes* const joinGroupAttr,
			ValueId & leftChildValueId,
			ValueId & rightChildValueId,
			NABoolean & isOrderPreserving) const
{
  // This expression MUST be a binary predicate expression.
  if (NOT isAPredicate())
    return FALSE;

  NABoolean isAnEquiJoin = FALSE;

  // Check for equality predicates
  if (getOperatorType() == ITM_EQUAL)
    {
      // a join predicate of the form col1 = col2 or similar
      ItemExpr * leftExpr  = child(0);
      ItemExpr * rightExpr = child(1);

      leftChildValueId  = leftExpr->getValueId();
      rightChildValueId = rightExpr->getValueId();

      // the two sides of the comparison operator have to be supplied
      // by the two different child  nodes
      if (CmpCommon::getDefault(COMP_BOOL_113) == DF_OFF)
        {
          // fix to soln 10-080604-3651 is to relax this test from 
          // "isCharacteristicOutput()" to "covers"
          const ValueIdSet &inputs = joinGroupAttr->getCharacteristicInputs();
          ValueIdSet refs;
          if (leftGroupAttr->covers(leftChildValueId, inputs, refs) AND
              rightGroupAttr->covers(rightChildValueId, inputs, refs))
            {
              isAnEquiJoin = TRUE;
            }
          else if (leftGroupAttr->covers(rightChildValueId, inputs, refs) AND
                   rightGroupAttr->covers(leftChildValueId, inputs, refs))
            {
              isAnEquiJoin = TRUE;
              leftChildValueId  = rightChildValueId;
              rightChildValueId = leftExpr->getValueId();
            }
        }
      else // they want the old behavior
        {
          if (leftGroupAttr->isCharacteristicOutput(leftChildValueId) AND
              rightGroupAttr->isCharacteristicOutput(rightChildValueId))
            {
              isAnEquiJoin = TRUE;
            }
          else if (leftGroupAttr->isCharacteristicOutput(rightChildValueId) AND
                   rightGroupAttr->isCharacteristicOutput(leftChildValueId))
            {
              isAnEquiJoin = TRUE;
              leftChildValueId  = rightChildValueId;
              rightChildValueId = leftExpr->getValueId();
            }
        }

      // Now, check whether this is a predicate that preserves ordering
      // EX:  The following predicates are order preserving
      //          T1.A = T2.X;  T1.A = T2.X+<constant>;
      //
      //      The following predicates are not order preserving
      //          T1.A = T2.X+T2.Y;  T1.A=1/T2.X;  T1.A= ABS(T2.X);
      if (leftExpr->isOrderPreserving() && rightExpr->isOrderPreserving())
	isOrderPreserving = TRUE;
      else
	isOrderPreserving = FALSE;
    } // endif is an "="
  else if (getOperatorType() == ITM_VEG_PREDICATE)
    {
      // Check whether this VEG Predicate is a true equi-join predicate.
      // If the VEG contains a constant, then we know this is not
      // a "true" join predicate.  Also, check to ensure whether
      // both children deliver the VEGReference that belong to this VEG.

      VEG * predVEG = ((VEGPredicate *)this)->getVEG();
      const ValueIdSet & VEGGroup = predVEG->getAllValues();

      NABoolean containsConstant = VEGGroup.referencesAConstExpr();

      if (containsConstant)
	return FALSE;

      ValueId vegRefId = predVEG->getVEGReference()->getValueId();

      // If this is a left join or a anti-semi the right may be
      // producing a a VEGRef that is contained in the VEGRef
      // leftChildValueId
      leftChildValueId = NULL_VALUE_ID;
      if (leftGroupAttr->isCharacteristicOutput(vegRefId))
        {
	  leftChildValueId = vegRefId;
        }
      else
        {
          ValueIdSet referencedOutputs;
          const NABoolean doNotLookInsideVegReferences = TRUE ;

          // Pass the parameter to accumulateReferencedValues to
          // indicate that we should not dig inside embedded vegrefs,
          // because in some cases a VegRef can contain itself, and
          // then we can get stuck in an infinite loop.
          //
          //should add additional comments here explaining
          // in more detail why we don't want to look inside embedded
          // veg ref's.  And we should all probably look for cases
          // where we *DO* look inside veg ref's when we shouldn't ...
          referencedOutputs.accumulateReferencedValues(
            leftGroupAttr->getCharacteristicOutputs(),   /* IN */
            VEGGroup,                                    /* IN */
            doNotLookInsideVegReferences);               /* IN */
          if ( referencedOutputs.entries() > 0 )
            referencedOutputs.getFirst(leftChildValueId);
        }

      rightChildValueId = NULL_VALUE_ID;
      if (rightGroupAttr->isCharacteristicOutput(vegRefId))
        {
          rightChildValueId = vegRefId;
        }
      else
        {
          ValueIdSet referencedOutputs;
          const NABoolean doNotLookInsideVegReferences = TRUE ;

          referencedOutputs.accumulateReferencedValues(
            rightGroupAttr->getCharacteristicOutputs(),  /* IN */
            VEGGroup,                                    /* IN */
            doNotLookInsideVegReferences);               /* IN */
          if ( referencedOutputs.entries() > 0 )
            referencedOutputs.getFirst(rightChildValueId);
        }

      if (leftChildValueId AND rightChildValueId)
	isAnEquiJoin = TRUE;

      isOrderPreserving = TRUE;

    } // endif is a VEGPredicate

  return isAnEquiJoin;

} // ItemExpr::isAnEquiJoinPredicate


// -----------------------------------------------------------------------
// ItemExpr::isANestedJoinPredicate
// ABSTRACT:
// Use this method to figure out whether a predicate in an
// operator under a nested join is a join. It returns
// TRUE is it is, FALSE otherwise.
//
// INPUTS:
//    const ValueIdSet& inputValues: the characteristic inputs
//      of the operator
//    const ValueIdSet& operatorValues: the values available
//      to the operator (i.e. if the operator is a DP2Scan,
//      they would contain the columns of the table or index
//      being scanned.
// -----------------------------------------------------------------------

NABoolean
ItemExpr::isANestedJoinPredicate (
     const ValueIdSet& inputValues,
     const ValueIdSet& operatorValues) const
{
  // This expression MUST be a binary predicate expression.
  CMPASSERT(isAPredicate());



  // Assume it is not a nested join pred:
  NABoolean isANestedJoinPred = FALSE;
  const Int32 arity = getArity(); // for debugging
  switch (arity)
    {
    case 2:
      {
        // a join predicate of the form col1 op col2 or similar
        const ItemExpr *leftExpr  = child(0);
        const ItemExpr *rightExpr = child(1);

        const ValueId &leftChildVid  = leftExpr->getValueId();
        const ValueId &rightChildVid = rightExpr->getValueId();


        // -------------------------------------------------------------------
        // one operand must be in the operator's inputs
        // and the other in the operator's values:
        // --------------------------------------------------------------------
        //
		//check if the item expr is a non-strict constant
		//a strict constant is somethine like cos(1)
		//where as cos(?p) can be considered a constant
		//in the non-strict definition since it remains
        //constant for a given execution of a query
        if ( !(leftExpr->doesExprEvaluateToConstant(FALSE)) AND
	         !(rightExpr->doesExprEvaluateToConstant(FALSE)))
	  {

            // We need to explode the values because they may
            // contain references and "referencesOneValueFrom"
            // does not explode references
            ValueIdSet iv,ov;
            iv.replaceVEGExpressionsAndCopy(inputValues);
            ov.replaceVEGExpressionsAndCopy(operatorValues);

            // Now check for containment
	    if ( (leftExpr->referencesOneValueFrom(iv)
                  AND
                  rightExpr->referencesOneValueFrom(ov))
                 OR
                 (rightExpr->referencesOneValueFrom(iv)
                  AND
                  leftExpr->referencesOneValueFrom(ov))
                 )
	      {
	        isANestedJoinPred = TRUE;
	      }
	  }
      }
    break;

    case 0:
      if (getOperatorType() == ITM_VEG_PREDICATE)
	{
	  // Check whether this VEG Predicate is a true equi-join predicate.
	  // If the VEG contains a constant, then we know this is not
	  // a "true" join predicate.  Also, check to ensure whether
	  // both children deliver the value that belongs to this VEG.

          const VEG * predVEG = ((VEGPredicate *)this)->getVEG();
          const ValueIdSet & VEGGroupForPred = predVEG->getAllValues();
          NABoolean containsConstant = VEGGroupForPred.referencesAConstExpr();
	  if (NOT containsConstant)
	    {
              // We need to explode the values because they may
              // contain references and "referencesOneValueFrom"
              // does not explode references
              ValueIdSet iv,ov;
              iv.replaceVEGExpressionsAndCopy(inputValues);
              ov.replaceVEGExpressionsAndCopy(operatorValues);
	      if (VEGGroupForPred.referencesOneValueFromTheSet(iv)
                  AND
                  VEGGroupForPred.referencesOneValueFromTheSet(ov)
                  )
		{
		  isANestedJoinPred = TRUE;
		}
	    } // endif is a constant
	}
      break ;

    default:
      isANestedJoinPred = FALSE ;
      break ;
    } // switch on getArity()

  return isANestedJoinPred;
} // ItemExpr::isANestedJoinPredicate (...)

void ItemExpr::accumulateConstExprs(ValueIdSet & constExprs)
{
  ValueIdSet predSet;
  getLeafPredicates(predSet);

  for( ValueId tempId = predSet.init();
      predSet.next( tempId );
      predSet.advance( tempId ) )
  {
    ItemExpr *tempExpr = tempId.getItemExpr();
    for (Lng32 i = 0; i < (Lng32)tempExpr->getArity(); i++)
    {
      if(tempExpr->child(i)->doesExprEvaluateToConstant(FALSE,TRUE))
      {
	constExprs += tempExpr->getValueId();
	break;
      }
    }
  }
}

NABoolean ItemExpr::isOrderPreserving() const
{
  return TRUE;
}

// -----------------------------------------------------------------------
// ItemExpr::applyDefaultPred
// This method is a virtual method for ItemExpr class. This method is redefined
// for derived classes of ItemExpr class. This method is used to apply selectivity
// of default predicates on histograms.
//
// In this default version, the columns are checked for existence of statistics.
// If they are found, an assertion is thrown in debug mode as this is considered
// an unexpected condition. In release mode, alreadyApplied flag is set to TRUE
// and returned.
// -----------------------------------------------------------------------
NABoolean ItemExpr::applyDefaultPred(ColStatDescList & histograms,
			                         OperatorTypeEnum exprOpCode,
						 ValueId predValueId,
                                     NABoolean & globalPredicate,
                                     CostScalar *maxSelectivity)
{
  NABoolean alreadyApplied = FALSE;
  ValueIdSet leftLeafValues;
  CollIndex leftColIndex;

   if(maxSelectivity==NULL && (getArity() > 0) &&
      (child(0)->checkForStats(histograms, leftColIndex, leftLeafValues)))
   {
      DCMPASSERT( FALSE ); // unexpected condition!
      alreadyApplied = TRUE;
   }
   return alreadyApplied;
}

// -----------------------------------------------------------------------
// ItemExpr::applyUnSuppDefaultPred
// This method is used to apply selectivity of unsupported default predicates
// on histograms.
//
// -----------------------------------------------------------------------
NABoolean ItemExpr::applyUnSuppDefaultPred(ColStatDescList & histograms,
						 ValueId predValueId,
						 NABoolean & globalPredicate)
{
  NABoolean alreadyApplied = FALSE, statsExist = FALSE;
  ValueIdSet leftLeafValues;
  CollIndex leftColIndex;

  OperatorTypeEnum op = getOperatorType();
  CostScalar defaultSel     = csOne;

  if ( getArity() > 0 )
  {
    NABoolean isOpTypeNot = FALSE;  
    ItemExpr * tempPred;
    OperatorTypeEnum tempOp;
	  
    ItemExpr * leftChild = child(0);

    if(getOperatorType() == ITM_NOT)
    {
      isOpTypeNot = TRUE;
      tempOp = leftChild->getOperatorType();
      tempPred = leftChild;
      leftChild = leftChild->child(0);
    }
    else
    {
      tempOp = op;
      tempPred = this;
    }

    statsExist = leftChild->checkForStats(histograms, leftColIndex, leftLeafValues);

    // First Question: is the predicate of the form
    //   "<col> <op> <expression>"  or  "<expression> <op> <col>"?
    // Earlier logic places stand-alone columns on the left, so look
    // for a histogram associated with the left-hand valueId.
    //

    if(statsExist)
    {
      globalPredicate = FALSE;   // not a 'global' predicate
      ColStatDescSharedPtr statDesc = (histograms)[leftColIndex];

      if ( NOT ( statDesc->isPredicateApplied( predValueId ) ) )
      {
        CostScalar oldRowcount = statDesc->getColStats()->getRowcount();

        // first time for this histogram
        // if the predicate is a LIKE predicate with no wild cards
        // in the pattern. And for some oreason could not be transformed
        // into an equality predicate, then set its selectivity equal to
        // 1/UEC, else go the usual way
		  
        if ( (tempOp == ITM_LIKE) && 
           ((Like *)tempPred)->isPatternAStringLiteral())
        {
          ColStatsSharedPtr colStat = statDesc->getColStats();

          if (colStat->isFakeHistogram())
            defaultSel = 1.0/(CURRSTMT_OPTDEFAULTS->defNoStatsUec()) ;
          else
          {
            CostScalar tempUec = colStat->getTotalUec();

            //To guard against div-by-zero assertion
            if(tempUec == csZero)
              tempUec = 1;
				
            defaultSel = 1.0/tempUec.value();
            defaultSel.maxCsOne();
          }

          if(isOpTypeNot)
            defaultSel = 1 - defaultSel.value();
        }
        else
          defaultSel = this->defaultSel();

        statDesc->addToAppliedPreds( predValueId );
        statDesc->applySel( defaultSel );

        // If user specified selectivity for this predicate, we need to make
        // adjustment in reduction to reflect that.
        statDesc->applySelIfSpecifiedViaHint(this, oldRowcount);

      } // NOT isPredicateApplied
      else
      {
        alreadyApplied = TRUE;
      }
    } // column is leading prefix of histogram
  } // if ( pred->getArity() > 0 )
  else
  {
    defaultSel = this->defaultSel();
  }
  return alreadyApplied;
}

NABoolean ItemExpr::checkForStats(ColStatDescList & histograms, 
				  CollIndex & columnIndex, 
				  ValueIdSet & leafValues)
{
  ItemExpr * tempPred = this->getLeafValueIfUseStats();
  leafValues.clear();
  tempPred->findAll(ITM_BASECOLUMN, leafValues, TRUE, TRUE);
  return histograms.getColStatDescIndexForColumn(columnIndex, tempPred->getValueId());
}

// Default case
NABoolean ItemExpr::calculateMinMaxUecs(ColStatDescList & histograms,
					CostScalar & minUec,
					CostScalar & maxUec)
{
 switch(getOperatorType())
 {
   case ITM_HOSTVAR:
   case ITM_DYN_PARAM:
   case ITM_CACHE_PARAM:
     minUec = maxUec = 1;
     return TRUE;
   default:
     minUec = maxUec = csMinusOne;
     return FALSE;
 }
}

NABoolean ItemExpr::calculateUecs(ColStatDescList & histograms,
                                 CostScalar & minUec,
                                 CostScalar & maxUec)
{
  CostScalar uec = csMinusOne;
  CollIndex leftColIndex;
  ValueIdSet leftLeafValues;

  if(checkForStats(histograms, leftColIndex, leftLeafValues))
  {
    ColStatDescSharedPtr statDesc = (histograms)[leftColIndex];
    minUec = statDesc->getColStats()->getTotalUec();
    maxUec = statDesc->getColStats()->getRowcount();    
    return TRUE;
  }
  else
    return FALSE;
}

void ItemExpr::resetRealBigNumFlag(ItemExpr *node)
{
  for (Int32 i=0; i < node->getArity(); i++)
    {
      resetRealBigNumFlag(node->child(i));
    }
  if((node->getOperatorType() == ITM_CAST) &&
   (((Cast *)node)->getType()->getTypeQualifier() == NA_NUMERIC_TYPE))
  {
     NumericType *numType = (NumericType *)((Cast *)node)->getType();
     if(numType->isBigNum() &&
      ((SQLBigNum *)numType)->isARealBigNum())
     {
        ((SQLBigNum *)numType)->resetRealBigNum();
     }
  }
}

NABoolean VEGReference::calculateMinMaxUecs(ColStatDescList & histograms,
					    CostScalar & minUec,
					    CostScalar & maxUec)
{
  return calculateUecs(histograms, minUec, maxUec);
}

NABoolean BaseColumn::calculateMinMaxUecs(ColStatDescList & histograms,
					  CostScalar & minUec,
					  CostScalar & maxUec)
{
  return calculateUecs(histograms, minUec, maxUec);
}

// -----------------------------------------------------------------------
//  Methods for class BiArith
//
// -----------------------------------------------------------------------
NABoolean BiArith::calculateMinMaxUecs(ColStatDescList & histograms,
					CostScalar & minUec,
					CostScalar & maxUec)
{
    CollIndex colIndex = 0;
    ValueIdSet leafValues;

  // Certain functions such as DAYOFYEAR, WEEK get converted 
  // to BiArtih objects. They handled here.

  if(origOpType()== ITM_DAYOFYEAR)
  {
    minUec = maxUec = 366;
    return TRUE;
  }
  else if(origOpType()== ITM_WEEK)
  {
    minUec = maxUec = 54;
    return TRUE;
  }
  else if(origOpType()== ITM_DATE_TRUNC_DAY)
  {
    if(child(0)->checkForStats(histograms, colIndex, leafValues))
    {
      ColStatDescSharedPtr statDesc = (histograms)[colIndex];
      ColStatsSharedPtr colStats = statDesc->getColStats();
      double timeEncompassedInHistogram = (colStats->getMaxValue().getDblValue() - colStats->getMinValue().getDblValue());
      
      // Reduce the UEC by equivalent of 12 hours worth of time. i.e. 86400 / 2. (86400 secs in a day)
      double timeEncompassedInHistogramAfterFunctionApplied = MAXOF((timeEncompassedInHistogram - 43200), 0);
      
      minUec = maxUec = (colStats->getTotalUec() * (timeEncompassedInHistogramAfterFunctionApplied/timeEncompassedInHistogram));
      return TRUE;
    }
  }
  else if(origOpType()== ITM_DATE_TRUNC_MONTH)
  {
    if(child(0)->checkForStats(histograms, colIndex, leafValues))
    {
      ColStatDescSharedPtr statDesc = (histograms)[colIndex];
      ColStatsSharedPtr colStats = statDesc->getColStats();
      double timeEncompassedInHistogram = (colStats->getMaxValue().getDblValue() - colStats->getMinValue().getDblValue());

      // Reduce the UEC by equivalent of 15 days worth of time. i.e. 86400 * 15. (86400 secs in a day)
      double timeEncompassedInHistogramAfterFunctionApplied = MAXOF((timeEncompassedInHistogram - 1296000), 0);

      minUec = maxUec = (colStats->getTotalUec() * (timeEncompassedInHistogramAfterFunctionApplied/timeEncompassedInHistogram));
      return TRUE;
    }
  }
  else if(origOpType()== ITM_DATE_TRUNC_YEAR)
  {
    if(child(0)->checkForStats(histograms, colIndex, leafValues))
    {
      ColStatDescSharedPtr statDesc = (histograms)[colIndex];
      ColStatsSharedPtr colStats = statDesc->getColStats();
      double timeEncompassedInHistogram = (colStats->getMaxValue().getDblValue() - colStats->getMinValue().getDblValue());

      // Reduce the UEC by equivalent of 6 months worth of time. i.e. 86400 * 30 * 6. (86400 secs in a day)
      double timeEncompassedInHistogramAfterFunctionApplied = MAXOF((timeEncompassedInHistogram - 15552000), 0);

      minUec = maxUec = (colStats->getTotalUec() * (timeEncompassedInHistogramAfterFunctionApplied/timeEncompassedInHistogram));
      return TRUE;
    }
  }
  else
  {
    // Calculate the UEC of both the children and return the MAX of UEC
    // of children as the UEC for the biarithmetic operator.
    CostScalar firstMinUec, secondMinUec, firstMaxUec, secondMaxUec;
    if(child(0)->calculateMinMaxUecs(histograms, firstMinUec, firstMaxUec) &&
       child(1)->calculateMinMaxUecs(histograms, secondMinUec, secondMaxUec))
    {
      minUec = MAXOF(firstMinUec, secondMinUec);
      maxUec = MAXOF(firstMaxUec, secondMaxUec);
      return TRUE;
    }
  }
  return FALSE;
}

NABoolean BiArith::isOrderPreserving() const
{
  // Future Work:  By analyzing the predicate further, can determine
  // that certain biArith predicates ARE order preserving.
  //
  // EX:   T1.A = T2.X + <constant>;    -> order preserving
  //       T1.A = T2.X + T2.Y;          -> not order preserving
  // For now, we just return false.

  return FALSE;
}

// -----------------------------------------------------------------------
//  Methods for class Function
//
// -----------------------------------------------------------------------
NABoolean Function::isOrderPreserving() const
{
  // Without further info, cannot determine whether a user-defined
  // function is order preserving
  return FALSE;
}

// -----------------------------------------------------------------------
//  Methods for class UnLogic
//
// ITM_NOT, ITM_IS_TRUE, ITM_IS_FALSE, ITM_IS_NULL, ITM_IS_NOT_NULL,
// ITM_IS_UNKNOWN, ITM_IS_NOT_UNKNOWN
// -----------------------------------------------------------------------

double UnLogic::defaultSel()
{
  switch (getOperatorType())
    {
    case ITM_IS_NULL:
    case ITM_IS_UNKNOWN: // was 0.01
      return (CostPrimitives::getBasicCostFactor(HIST_DEFAULT_SEL_FOR_IS_NULL)) ;

    case ITM_IS_NOT_NULL:
    case ITM_IS_NOT_UNKNOWN:  // was 0.99
      return (1.0 - CostPrimitives::getBasicCostFactor(HIST_DEFAULT_SEL_FOR_IS_NULL)) ;

    case ITM_NOT:
      // ITM_NOT should actually be returning selectivity equal to 
      // 1 - default childSelectivity in all cases. But in some cases 
      // including aggregate expressions, we use default defaultSel() 
      // method to compute selectivity. This is 1.0 in all cases except when
      // the expression is ITM_RETURN_FALSE. Because of this the selectivity
      // of NOT returned for most aggregate function is 0. This needs to be fixed.
      // We have created a case to follow this up: Sol: 10-050721-0038

      if (child(0)->getOperatorType() == ITM_LIKE)
	return (1 - child(0)->defaultSel());
      else
        return (CostPrimitives::getBasicCostFactor(HIST_DEFAULT_SEL_FOR_BOOLEAN)) ;

    default: // ITM_IS_TRUE, ITM_IS_FALSE  // was 0.3333
      return (CostPrimitives::getBasicCostFactor(HIST_DEFAULT_SEL_FOR_BOOLEAN)) ;
    }
}

NABoolean UnLogic::synthSupportedOp() const
{
  switch (getOperatorType())
    {
    case ITM_NOT:
      return FALSE;

    case ITM_IS_TRUE:
    case ITM_IS_FALSE:
      return FALSE;

    case ITM_IS_NULL:
    case ITM_IS_NOT_NULL:
    case ITM_IS_UNKNOWN:
    case ITM_IS_NOT_UNKNOWN:
      return TRUE;

    default: // ??
      return FALSE;
    }

  return TRUE;
}

NABoolean UnLogic::applyDefaultPred(ColStatDescList & histograms,
			                         OperatorTypeEnum exprOpCode,
						 ValueId predValueId,
                                    NABoolean & globalPredicate,
                                    CostScalar *maxSelectivity)
{
  // maxSelectivity computation is done
  if (maxSelectivity) return TRUE;

  CostScalar defaultSel     = csOne;
  NABoolean statsExist = FALSE;
  NABoolean alreadyApplied  = FALSE;
  
  // leftColIndex contains the position of the left histogram whose statistics 
  // will be used for computing selectivity. 
  // In case the left child contains more than one columns, 
  // it would be the position of histogram with max UEC amongst left child
  CollIndex leftColIndex;

  // The leaf values of the left child
  ValueIdSet leftLeafValues;

  OperatorTypeEnum op = getOperatorType();

  statsExist = child(0)->checkForStats(histograms, leftColIndex, leftLeafValues);

  if (statsExist)
  {
    if (	 op == ITM_IS_NULL
		  OR op == ITM_IS_NOT_NULL
		  OR op == ITM_IS_UNKNOWN
		  OR op == ITM_IS_NOT_UNKNOWN
	    )
    {
	  globalPredicate = FALSE;   // not a 'global' predicate
	  ColStatDescSharedPtr statDesc = (histograms)[leftColIndex];

	  if ( NOT ( statDesc->isPredicateApplied( predValueId ) ) )
	  { // first time for this histogram
	    defaultSel = ( statDesc->isSimilarPredicateApplied( op ) ?  csOne : this->defaultSel() );

	    CostScalar oldRowcount = statDesc->getColStats()->getRowcount();

	    statDesc->addToAppliedPreds( predValueId );
	    statDesc->applySel( defaultSel );

	    // If user specified selectivity for this predicate, we need to make
	    // adjustment in reduction to reflect that.
	    statDesc->applySelIfSpecifiedViaHint(this, oldRowcount);
	  }
	  else
	  {
	    alreadyApplied = TRUE;
	  }
    } // null, not null, unknown, not unknown
    else
    {
	  DCMPASSERT( FALSE ); // unexpected condition!
	  alreadyApplied = TRUE;
    }
  }
  return alreadyApplied;
}

// -----------------------------------------------------------------------
//  Methods for class BiLogic
// -----------------------------------------------------------------------

NABoolean BiLogic::isOrderPreserving() const
{
  return FALSE;
}

NABoolean BiLogic::synthSupportedOp() const
{
  return TRUE;
}

NABoolean BiLogic::applyDefaultPred(ColStatDescList & histograms,
			                         OperatorTypeEnum exprOpCode,
						 ValueId predValueId,
                                    NABoolean & globalPredicate,
                                    CostScalar *maxSelectivity)
{
  // maxSelectivity computation is done
  if (maxSelectivity) return TRUE;

  CostScalar defaultSel     = csOne;
  NABoolean statsExist = FALSE;
  NABoolean alreadyApplied  = FALSE;

  // leftColIndex contains the position of the left histogram whose statistics 
  // will be used for computing selectivity. 
  // In case the left child contains more than one columns, 
  // it would be the position of histogram with max UEC amongst left child
  CollIndex leftColIndex;

  // The leaf values of the left child
  ValueIdSet leftLeafValues;

  OperatorTypeEnum op = getOperatorType();

  statsExist = child(0)->checkForStats(histograms, leftColIndex, leftLeafValues);

  if (statsExist)
  {
    if ( ( op == ITM_OR) || ( op == ITM_AND ) )
    {
	  // Don't do anything with this predicate right here, right now.
	  alreadyApplied = TRUE;
    }  // op is AND, or OR
    else
    {
	  DCMPASSERT( FALSE ); // unexpected condition!
	  alreadyApplied = TRUE;
    }
  }
  return alreadyApplied;
}

// -----------------------------------------------------------------------
//  Methods for class BiRelat
//   ITM_EQUAL, ITM_NOT_EQUAL, ITM_LESS, ITM_LESS_EQUAL,
//   ITM_GREATER, ITM_GREATER_EQ
// -----------------------------------------------------------------------
double BiRelat::defaultSel()
{

  switch (getOperatorType())
    {
      case ITM_EQUAL:
	{
//      return (CostPrimitives::getBasicCostFactor(HIST_DEFAULT_SEL_FOR_PRED_EQUAL)) ;
	  double selectivityForPredEqual = (1.0/(CURRSTMT_OPTDEFAULTS->defNoStatsUec()) );
	  return selectivityForPredEqual;
	}

      case ITM_NOT_EQUAL:
	{
//      return (1.0 - CostPrimitives::getBasicCostFactor(HIST_DEFAULT_SEL_FOR_PRED_EQUAL)) ;
	  double selectivityForPredEqual = (1.0/(CURRSTMT_OPTDEFAULTS->defNoStatsUec()) );
	  if (selectivityForPredEqual == 1.0)
	    return MIN_SELECTIVITY;
	  else
	    return (1.0 - selectivityForPredEqual);
	}

      default: // ITM_LESS, ITM_LESS_EQUAL, ITM_GREATER, ITM_GREATER_EQ
	return CURRSTMT_OPTDEFAULTS->defSelForRangePred() ;
    }
}

NABoolean BiRelat::synthSupportedOp() const
{
  return TRUE;
}

NABoolean BiRelat::applyDefaultPred(ColStatDescList & histograms,
			                         OperatorTypeEnum exprOpCode,
						 ValueId predValueId,
                                    NABoolean & globalPredicate,
                                    CostScalar *maxSelectivity)
{
  CostScalar defaultSel = csOne, leftUec = csMinusOne, rightUec = csMinusOne, oldRowcount = csOne;
  NABoolean statsExist = FALSE;
  NABoolean alreadyApplied  = FALSE;
  NABoolean rhsStatsExist = FALSE;

  // leftColIndex contains the position of the left histogram whose statistics 
  // will be used for computing selectivity. 
  // In case the left child contains more than one columns, 
  // it would be the position of histogram with max UEC amongst left child
  CollIndex leftColIndex;
  CollIndex rightColIndex;

  // following two sets contain the leaf values of the respective children
  ValueIdSet leftLeafValues;
  ValueIdSet rightLeafValues;

  // This boolean will be set to TRUE if it is an equality predicate with more
  // than one column involved and COMP_BOOL_40 is ON.
  // When counting the number of columns, it takes a combined set of the
  // columns from the left and the right children
  NABoolean equiJoinWithExpr = FALSE;

  OperatorTypeEnum op = getOperatorType();

  ItemExpr *lhs = child(0);
  ItemExpr *rhs = child(1);

  if(( op == ITM_EQUAL ) && (CmpCommon::getDefault(COMP_BOOL_40) == DF_ON))
  {
    lhs->findAll(ITM_VEG_REFERENCE, leftLeafValues, TRUE, TRUE);
    rhs->findAll(ITM_VEG_REFERENCE, rightLeafValues, TRUE, TRUE);

    if((rightLeafValues.entries() + leftLeafValues.entries() > 1) && (rightLeafValues != leftLeafValues))
    {
      statsExist = histograms.getColStatDescIndexForColWithMaxUec(leftColIndex, leftLeafValues);
      if(statsExist)
      {
	if((exprOpCode != REL_SCAN) || (CmpCommon::getDefault(COMP_BOOL_74) == DF_OFF) )
	  equiJoinWithExpr = TRUE;

	if (rightLeafValues.entries() > 0)
	  rhsStatsExist = histograms.getColStatDescIndexForColWithMaxUec(rightColIndex, rightLeafValues);
      }
    }
    else
    {
      // Here local predicates with complex expression on one side are handled.
      // If the complex expression is MOD, SUBSTRING or other functions that cannot
      // be handled, then global default selectivity of 0.5 is used.
      statsExist = lhs->checkForStats(histograms, leftColIndex, leftLeafValues);
      rhsStatsExist = rhs->checkForStats(histograms, rightColIndex, rightLeafValues);
    }
  }
  else
  {
    statsExist = lhs->checkForStats(histograms, leftColIndex, leftLeafValues);
    rhsStatsExist = rhs->checkForStats(histograms, rightColIndex, rightLeafValues);
  }

  if (statsExist)
  {
    ColStatDescSharedPtr leftStatDesc = (histograms)[leftColIndex];

    if (leftStatDesc->isPredicateApplied(predValueId))
    {
      globalPredicate = FALSE;
      return TRUE; //alreadyApplied
    }

    oldRowcount = leftStatDesc->getColStats()->getRowcount();

    if ( ( op == ITM_EQUAL ) || ( op == ITM_NOT_EQUAL ) )
    {
      // if rightside has any column, get the histograms for the right child
      ColStatDescSharedPtr rightStatDesc;
      if(rhsStatsExist)
	rightStatDesc = (histograms)[rightColIndex];

      if (equiJoinWithExpr)
      {
       if (maxSelectivity == NULL) {
	globalPredicate = FALSE;

	// if the left side represents a simple expression, use UEC from left side for calculating selectivity
	if(isSimpleComplexPredInvolved())
	{
	  leftUec = leftStatDesc->getColStats()->getTotalUec().getValue();
	  defaultSel = 1/leftUec.minCsOne().getValue();
	  if( op == ITM_NOT_EQUAL)
	    defaultSel = 1 - defaultSel.getValue();
	  leftStatDesc->applySel(defaultSel);
	}
	else
	  applyEquiJoinExpr(leftStatDesc, rightStatDesc, histograms);

	ValueIdSet columnWithPreds(leftLeafValues);
	columnWithPreds.insert(rightLeafValues);
	histograms.addToAppliedPredsOfAllCSDs( columnWithPreds, predValueId );
       } // maxSelectivity == NULL
      }
      else
      {
	// The following is for Gen Sol: 10-090316-0026: See genesis for 
	// details. 
	// If there are multiple columns on the left side then 
	// we need to make sure that the columns are part of a VEG.
	if (leftLeafValues.entries() > 1)
	{
	  statsExist = FALSE;
	  
	  // Make sure the columns in the VEG do not belong to the same table
	  SET(TableDesc *) * tableDescs = NULL;
	  tableDescs = leftLeafValues.getAllTables();
	  if(tableDescs->entries() > 1)
	  {
	    leftLeafValues.clear();
	    lhs->findAll(ITM_VEG_REFERENCE, leftLeafValues, TRUE, TRUE);
	    if (leftLeafValues.entries() == 1)
	    {
	      statsExist = histograms.getColStatDescIndexForColWithMaxUec(leftColIndex, leftLeafValues);
	      leftStatDesc = (histograms)[leftColIndex];
	      oldRowcount = leftStatDesc->getColStats()->getRowcount();
	    }
	  }
	}

	if(statsExist)
	{
	  globalPredicate = FALSE;
	  applyLocalPredExpr(leftStatDesc, rightStatDesc, exprOpCode, 
			      leftLeafValues, rightLeafValues, maxSelectivity);	
	  if (maxSelectivity == NULL)
	    leftStatDesc->addToAppliedPreds( predValueId );
	}
       } // 1b: equal, not equal
    } 
    // less, less_eq, greater, greater_eq
    else if (	 op == ITM_LESS
		  OR op == ITM_LESS_EQ
		  OR op == ITM_GREATER
		  OR op == ITM_GREATER_EQ
	    )
    {
     if (maxSelectivity == NULL) {
      globalPredicate = FALSE;
      applyRangePredExpr(leftStatDesc, leftLeafValues, rightLeafValues);
      leftStatDesc->addToAppliedPreds( predValueId );
     } // maxSelectivity == NULL
    } // less, less_eq, greater, greater_eq
    else
    {
	  DCMPASSERT( FALSE ); // unexpected condition!
	  alreadyApplied = TRUE;
    }

    // If user specified selectivity for this predicate, we need to make
    // adjustment in reduction to reflect that.
    leftStatDesc->applySelIfSpecifiedViaHint(this, oldRowcount);
  }
  return alreadyApplied;
}

NABoolean BiRelat::isSimpleComplexPredInvolved()
{  
  ItemExpr *lhs = child(0)->getLeafValueIfUseStats();
  ItemExpr *rhs = child(1)->getLeafValueIfUseStats();

  OperatorTypeEnum leftOp = lhs->getOperatorType();
  OperatorTypeEnum rightOp = rhs->getOperatorType();

  if( (leftOp != ITM_VEG_REFERENCE) && (rightOp != ITM_VEG_REFERENCE) ||
      (leftOp == ITM_VEG_REFERENCE) && (rightOp == ITM_VEG_REFERENCE) )
    return FALSE;

  // Some operators have specific logic for calculating UEC and should not be
  // considered for simple-complex rule. Currently, CASE, SUBSTRING are in the
  // list. More will be added as more operators require special logic.

  if((rightOp == ITM_MOD) || (leftOp == ITM_MOD))
    return FALSE;

  if((rightOp == ITM_SUBSTR) && (rhs->child(0)->getOperatorType()== ITM_CAST) ||
     (leftOp == ITM_SUBSTR) && (lhs->child(0)->getOperatorType()== ITM_CAST))
    return FALSE;

  NABoolean caseStmtInvlvd = FALSE;
  ItemExpr *tempPred;

  if(leftOp == ITM_CASE || leftOp == ITM_IF_THEN_ELSE)    
  {
    tempPred = lhs;
    if(leftOp == ITM_CASE)
      tempPred = tempPred->child(0);
    caseStmtInvlvd = TRUE;
  }
  else if(rightOp == ITM_CASE || rightOp == ITM_IF_THEN_ELSE)
  {
    tempPred = rhs;
    if(rightOp == ITM_CASE)
      tempPred = tempPred->child(0);
    caseStmtInvlvd = TRUE;
  }

  if(caseStmtInvlvd)
  {
    NABoolean flag = TRUE;
    if((tempPred->child(1)->getOperatorType()== ITM_VEG_REFERENCE)||
      (tempPred->child(1)->getOperatorType()== ITM_CONSTANT))
      flag = FALSE;
    else
      flag  = TRUE;

    while(!flag && (tempPred->child(2)->getOperatorType() == ITM_IF_THEN_ELSE))
    {
      tempPred = tempPred->child(2);
      if((tempPred->child(1)->getOperatorType()== ITM_VEG_REFERENCE)||
        (tempPred->child(1)->getOperatorType()== ITM_CONSTANT))
	flag = FALSE;
      else
      {
	flag  = TRUE;
	break;
      }
    }    

    if(!flag && 
      ((tempPred->child(2)->getOperatorType()== ITM_CONSTANT)||
      (tempPred->child(2)->getOperatorType()== ITM_VEG_REFERENCE))) 
      return FALSE;
  }

  ValueIdSet leftBaseColSet, rightBaseColSet;
  lhs->findAll( ITM_BASECOLUMN, leftBaseColSet, TRUE, TRUE );
  rhs->findAll( ITM_BASECOLUMN, rightBaseColSet, TRUE, TRUE );

  SET(TableDesc *) * leftSideTables = leftBaseColSet.getAllTables();
  SET(TableDesc *) * rightSideTables = rightBaseColSet.getAllTables();

  if( (leftSideTables->entries() > 0) && (rightSideTables->entries() > 0)
    && (!rightSideTables->contains(leftSideTables->at(0))))
    return TRUE;
  else
    return FALSE;
}

void BiRelat::applyEquiJoinExpr(ColStatDescSharedPtr & leftStatDesc,
					  ColStatDescSharedPtr & rightStatDesc,
					  ColStatDescList & histograms)
{
  CostScalar defaultSel = csOne;
  CostScalar leftUec = csMinusOne, rightUec = csMinusOne, leftMaxUec = csMinusOne, rightMaxUec = csMinusOne;
  CostScalar fudgeFactorForAggFn ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_6));

  if(CmpCommon::getDefault(COMP_BOOL_108) == DF_ON)
  {
    NABoolean foundLeftUec = child(0)->calculateMinMaxUecs(histograms, leftUec, leftMaxUec);
    NABoolean foundRightUec = child(1)->calculateMinMaxUecs(histograms, rightUec, rightMaxUec);

    if(foundLeftUec || foundRightUec)
    {
      if (child(0)->containsAnAggregate())
	leftUec = (leftUec * fudgeFactorForAggFn).minCsOne();

      if (child(1)->containsAnAggregate())
	rightUec = (rightUec * fudgeFactorForAggFn).minCsOne();

      defaultSel = csOne/(MAXOF(leftUec, rightUec)).minCsOne();
    }
    else
      defaultSel = CostPrimitives::getBasicCostFactor( HIST_DEFAULT_SEL_FOR_JOIN_EQUAL );
  }
  else
  {
    // rightLeafValues > 0 and leaftLeafValues > 0 or <col1> = <col2>
    // col1 = Fn(col2, col3) OR Fn(col1, col2) = col3
    // selectivity is equal to 1/MAXUEC of the columns 
    // participating in the query
    // get histograms from right child. In case there are
    // more than one columns in the right child, get the histogram 
    // with max UEC
    ColStatsSharedPtr leftColStat = leftStatDesc->getColStats();
    leftUec = leftColStat->getTotalUec();
    NABoolean leftColStatReal = !leftColStat->isOrigFakeHist();

    // If we reached here, it is guaranteed that we have histograms for the
    // left child. But, we cannot say anything for sure for the right child.
    // Hence check if there exists any column in the right side of the equality
    // predicates.

    ColStatsSharedPtr rightColStat;
    NABoolean rightColStatReal = FALSE;
    CostScalar rightUec = csMinusOne;

    // if rightside has any column, get the histograms for the right child
    if(rightStatDesc)
    {
      // out of all these columns, pick the one with Max UEC. While doing
      // that it also makes sure that it is comparing the default UEC to
      // default UEC and actual UEC to actual one of both children
      rightColStat = rightStatDesc->getColStats();
      rightUec = rightColStat->getTotalUec();
      rightColStatReal = !rightColStat->isOrigFakeHist();
    } // if getColIndex for rightLeafValues

    // check for aggregate function for both children 
    // and adjust UECs accordingly
    // We would have ideally like to compute selectivity for
    // aggregate functions by considering UEC for only those 
    // columns which are a children of Aggregate functions.
    // Because of time constraint and not being able to find
    // an inexpensive way to handle that, we are now taking the 
    // cardinality of the column with MAX UEC from the child
    // and in case of an aggregate, multiply with a fudge factor
    // Hence there could be cases where for predicates like 
    // "a + max(b) = c", we might pickup col 'a' as that has the
    // highest UEC and multiple cardinality of that by the fudge 
    // factor. If columns 'a' and 'b' belog to the same table, 
    // or have already been joined, it would not matter which
    // column we pickup, but in some cases it might poor estimates

    if (child(0)->containsAnAggregate())
      leftUec = (leftUec * fudgeFactorForAggFn).minCsOne();

    if (rightColStat && child(1)->containsAnAggregate())
      rightUec = (rightUec * fudgeFactorForAggFn).minCsOne();

    // To get the maxUec for selectivity, we don't want to compare
    // real UEC with the fake one. Hence, also check for fakeness
    // before comparing

    if (leftColStatReal && !rightColStatReal)
      defaultSel = csOne/leftUec;
    else if (rightColStatReal && !leftColStatReal)
      defaultSel = csOne/rightUec;
    else
      defaultSel = csOne/MAXOF(leftUec, rightUec);
    }

  // Since the histograms have not been merged, we don't know which
  // one will be finally picked up later for parent node, based on
  // the characteristics output. Hence set the aggregate information
  // of all correctly. Rowcount and UEC for all will be done automatically
  // during synchronizeStats. appliedPreds is the only that needs to be
  // correctly set

  // This will modify the rowcount, which should be done for only
  // one histogram. Remaining histograms will be synchronized later
  leftStatDesc->applySel( defaultSel );
}

void BiRelat::applyLocalPredExpr(ColStatDescSharedPtr & leftStatDesc,
                                 ColStatDescSharedPtr & rightStatDesc,
                                 OperatorTypeEnum exprOpCode,
                                 ValueIdSet leftLeafValues,
                                 ValueIdSet rightLeafValues,
                                 CostScalar *maxSelectivity)
{
  CostScalar defaultSel = csOne;
  OperatorTypeEnum op = getOperatorType();

  ColStatsSharedPtr leftColStat = leftStatDesc->getColStats();
  CostScalar leftUec = leftColStat->getTotalUec();;
  
  // If we reached here, it is guaranteed that we have histograms for the
  // left child. But, we cannot say anything for sure for the right child.
  // Hence check if there exists any column in the right side of the equality
  // predicates.

  ColStatsSharedPtr rightColStat;
  CostScalar rightUec = csMinusOne;

  // if rightside has any column, get the histograms for the right child
  if (rightStatDesc)
  {
    // out of all these columns, pick the one with Max UEC. While doing
    // that it also makes sure that it is comparing the default UEC to
    // default UEC and actual UEC to actual one of both children
    rightColStat = rightStatDesc->getColStats();
    rightUec = rightColStat->getTotalUec();
  } // if getColIndex for rightLeafValues

  // First Question: is the predicate of the form
  //   "<col> <op> <expression>"  or  "<expression> <op> <col>"?
  //
  // Earlier logic places stand-alone columns on the left, so look
  // for a histogram associated with the left-hand valueId.
  //
  // Of course, the trick here is that the following works even if
  // the left-hand ValueId isn't for a column.

  // There is only one column in the predicate or there is one 
  // column on the right hand side of the predicate and
  // this column is same as the left side column, use Uec
  // of the column instead of using default statistics

  if  (( rightLeafValues.isEmpty() )  ||  
    ( (rightLeafValues.entries() == 1) && (leftLeafValues == rightLeafValues ) ) )
  { 
    // <col> <op> <constant> or <col> <op> <col>, 
    // where the left and the right col are same
    // If this is a 'fake' histogram, the selectivity
    // assumed is the default selectivity associated
    // with the current predicate.  (But, don't apply
    // the same type of predicate multiple times.)
    if ( leftColStat->isFakeHistogram() )
    {
      defaultSel = (leftStatDesc->isSimilarPredicateApplied( op ) ?
				  csOne : this->defaultSel() );
    }
    else
    { 
      // not a 'fake histogram'
      // Determine the rowcount of the non-NULL value
      // with the greatest/least rowcount.
      HistogramSharedPtr histP    = leftColStat->getHistogram();
      const CostScalar & rowRedF = leftColStat->getRedFactor();

      const CostScalar & rowCountBeforePred = (leftColStat->getRowcount()).minCsOne();

      CostScalar maxRowCount = csOne;
      CostScalar minRowCount = rowCountBeforePred;
      CostScalar tmpRowCount;
      CostScalar uec;

      Interval iter = histP->getFirstInterval();

      while ( iter.isValid() && !iter.isNull() )
      {
	// uec must be at least 1 for these
	// calculations since we don't want to get
	// a huge blowup in rowcount
	if ( iter.getUec().isZero() )
	{
	  iter.next();
	  continue; // avoid divide-by-zero!
	}

	CostScalar iRows = rowRedF * iter.getRowcount();

	uec = (MINOF(iRows, iter.getUec())).minCsOne();

	tmpRowCount =
	  iRows / uec;

	if ( tmpRowCount > maxRowCount )
	  maxRowCount = tmpRowCount;

	if ( tmpRowCount < minRowCount )
	  minRowCount = tmpRowCount;

	iter.next();
      } // end while iter() is valid

      if ( op == ITM_EQUAL )
      {
	defaultSel = maxRowCount / rowCountBeforePred;
	if (maxSelectivity) 
	  {
	    // maxSelectivity(x=?) == max frequency / total rows
	    *maxSelectivity = 
	      MINOF(maxRowCount / rowCountBeforePred,
		    *maxSelectivity);
	  }
      }
      else
      {
	// With ITM_NOT_EQUAL, and no Histogram, avoid
	// setting defaultSel to zero:
	CostScalar numer;
	CostScalar denom;

	if ( minRowCount == rowCountBeforePred )
	{
	  numer = MAXOF( rowCountBeforePred, csTwo ) - csOne;
	  denom = MAXOF( rowCountBeforePred, csTwo );

	  defaultSel = numer / denom;
	}
	else
	{
	  numer = minRowCount;
	  denom = rowCountBeforePred;

	  defaultSel = csOne - ( numer / denom );
	  // maxSelectivity(x<>?) == 1.0
	  // which means do nothing here because 1.0 has
	  // already been set as the default maxSelectivity
	  // just before the estimateCardinality() call.
	}
      } // op == ITM_NOT_EQUAL
    }  // not a 'fake histogram'
  } // The Operand is a constant expression.
  else // i.e., NOT leafValues.isEmpty()
  {
    // <col1> <op> <col2>, or <col1 + col2> <op> <col3>
    // or <col1> <op> <col2 + col3>
    // The operand involves more than one column, which
    // makes this an equality join that we are not now
    // able to evaluate.
    // Note that the current predicate-based defaultSel
    // routine is not used in this case.....
    if(isSimpleComplexPredInvolved())
    {
      defaultSel = 1/leftUec.minCsOne().getValue();
      if( op == ITM_NOT_EQUAL)
	defaultSel = 1 - defaultSel.getValue();
    }
    else
    {
      if ( op == ITM_EQUAL )
      {
	if (leftStatDesc->isSimilarPredicateApplied( op ) )
	      defaultSel = csOne;
	else
	{
	  if ( (rightLeafValues.entries() == 1) &&
		(exprOpCode != REL_SCAN) )
	  {
	    // <col1> = <col2>
	    // we already know that left side has one column. This is the case
	    // col1 Join col2

	    if(rightStatDesc)
	    {
		CostScalar maxUec = (MAXOF(leftUec, rightUec)).minCsOne();
		defaultSel = csOne/maxUec;
	    } // colStat for column found
	    else
	    {
	      // histogram does not exist use default selectivity for Join equal
	      defaultSel = CostPrimitives::getBasicCostFactor( HIST_DEFAULT_SEL_FOR_JOIN_EQUAL );
	    }
	  } // rightLeafValueEntries = 1
	  else
	  {
	    // right side has more than one columns, or it is a scan. 
	    // Use default join equal selectivity for <col1> = <col2, col3>
	    // and hist_no_stats_uec for scan
	    if (exprOpCode == REL_SCAN)
	      defaultSel = (1.0/CURRSTMT_OPTDEFAULTS->defNoStatsUec());
	    else
	      defaultSel = CostPrimitives::getBasicCostFactor( HIST_DEFAULT_SEL_FOR_JOIN_EQUAL );
	  }
	}
      }
      else
      { // op == ITM_NOT_EQUAL. Apply default selectivity
	if (leftStatDesc->isSimilarPredicateApplied( op ) )
	  defaultSel = csOne;
	else
	{
	  if (exprOpCode == REL_SCAN)
	    defaultSel = 1 - (1.0/CURRSTMT_OPTDEFAULTS->defNoStatsUec() );
	  else
	      defaultSel = (1 - CostPrimitives::getBasicCostFactor( HIST_DEFAULT_SEL_FOR_JOIN_EQUAL ) );
	}
      }
    }
  } // NOT leafValue.isEmpty()
    leftStatDesc->applySel( defaultSel );
}

void BiRelat::applyRangePredExpr(ColStatDescSharedPtr & leftStatDesc,
					      ValueIdSet leftLeafValues,
					      ValueIdSet rightLeafValues)
{
  ItemExpr * lhs = child(0);
  lhs = lhs->getLeafValueIfUseStats();

  ItemExpr * rhs = child(1);
  rhs = rhs->getLeafValueIfUseStats();

  CostScalar defaultSel = csOne;
  OperatorTypeEnum op = getOperatorType();

  ColStatsSharedPtr leftColStat = leftStatDesc->getColStats();
  CostScalar leftUec = leftColStat->getTotalUec();

  // First Question: is the predicate of the form
  //   "<col> <op> <expression>"  or  "<expression> <op> <col>"?

    defaultSel = leftStatDesc->selForRelativeRange (op, lhs->getValueId(), rhs);
	    
	    // defaultSel is one for range predicates on char or varchar 
	    // column types.
	    // if a similar predicate has already been applied to this 
	    // histogram, then we don't want to reduce the rowcount and
	    // uec further. Therefore we return the selectivity equal to
	    // one. Following two preedicates are said to be similar:
	    // 1. < and <=
	    // 2. > and >=
	    // 3. If the two range predicates are derived from LIKE predicate

	    // There is only one column in the predicate or there is one 
	    // column on the right hand side of the predicate and
	    // this column is same as the left side column, use Uec
	    // of the column instead of using default statistics

	    if  (( rightLeafValues.isEmpty() )  ||  
	      ( (rightLeafValues.entries() == 1) && (leftLeafValues == rightLeafValues ) ) )
	    {
		  // pred <col> operatort <expression>
		  if ( defaultSel == csOne )
		  {
	    if (leftStatDesc->derivOfLikeAndSimilarPredApp(this) ||
		    leftStatDesc->isSimilarPredicateApplied( op ) )
			  defaultSel = csOne;
		    else
		    {
			  BiRelat *br = (BiRelat *) this;
			  if (br->derivativeOfLike())
			    defaultSel = br->getLikeSelectivity();
			  else
			    defaultSel = this->defaultSel();
	    } // else leftStatDesc->similarPredApplied
		  } // if defaultSel == csOne
	    } // if rightLeafValues.entries > 1 or leftLeafValues != rightLeafValues
	    else
	    {
		  // not a leaf value. Predicate is <col> operator <col>
		  defaultSel =
	    ( leftStatDesc->isSimilarPredicateApplied( op ) ?
		    csOne : CostPrimitives::getBasicCostFactor( HIST_DEFAULT_SEL_FOR_JOIN_RANGE ) );
	    } // end of join range

    leftStatDesc->applySel( defaultSel );
}


NABoolean Case::calculateMinMaxUecs(ColStatDescList & histograms,
				    CostScalar & minUec,
				    CostScalar & maxUec)
{
  // Some of the date/time functions get transformed into CASE
  // statements. In such cases, we can assign UECs without 
  // evaluating them.

  switch(origOpType())
  {
    case ITM_DAYNAME:
      minUec = maxUec = 7;
      return TRUE;
    case ITM_MONTHNAME:
      minUec = maxUec = 12;
      return TRUE;
    case ITM_QUARTER:
      minUec = maxUec = 4;
      return TRUE;
    case ITM_CASE:
      return child(0)->calculateMinMaxUecs(histograms, minUec, maxUec);
    default:
      minUec = maxUec = csMinusOne;
      return FALSE;
  }    
}

NABoolean IfThenElse::calculateMinMaxUecs(ColStatDescList & histograms,
					  CostScalar & minUec,
					  CostScalar & maxUec)
{
  CostScalar firstMinUec, secondMinUec, firstMaxUec, secondMaxUec;
  NABoolean firstOutcomeResult = child(1)->calculateMinMaxUecs(histograms, firstMinUec, firstMaxUec);
  NABoolean secondOutcomeResult = child(2)->calculateMinMaxUecs(histograms, secondMinUec, secondMaxUec);

  if(firstOutcomeResult && secondOutcomeResult)
  {
    if((child(1)->getOperatorType() == ITM_CONSTANT) || 
       (child(2)->getOperatorType() == ITM_CONSTANT))
      minUec = firstMinUec + secondMinUec;
    else
      minUec = MINOF(firstMinUec, secondMinUec);

    maxUec = firstMinUec + secondMinUec;
    return TRUE;
  }
  else
    return FALSE;
}

NABoolean Substring::calculateMinMaxUecs(ColStatDescList & histograms,
					 CostScalar & minUec,
					 CostScalar & maxUec)
{
  // Return if the function is not a SUBSTRING originally
  if(origOpType()!= ITM_SUBSTR)
    return FALSE;

  CostScalar rowCount = csMinusOne; 
  CostScalar uec = csMinusOne; 
  CollIndex columnIndex;
  ValueIdSet leafValues;

  this->findAll(ITM_VEG_REFERENCE, leafValues, TRUE, TRUE);

  if(leafValues.entries() != 1)
    return FALSE;

  ValueId vid;
  leafValues.getFirst(vid);

  NABoolean statsExist = histograms.getColStatDescIndexForColumn(columnIndex, vid);

  if(!statsExist)
    return FALSE;

  NABoolean negate;
  double origLength = 0, length = 0;
  const NAType *operand1 = &child(0)->getValueId().getType();
  const CharType *charOperand = (CharType *) operand1;
  origLength = charOperand->getStrCharLimit();

  if(getArity() == 3)
  {
    ConstValue *cv = child(2)->castToConstValue(negate);
    if(cv)
      length = convertInt64ToDouble(cv->getExactNumericValue());
    else
      return FALSE;
  }
  else
  {
    ConstValue *cv = child(1)->castToConstValue(negate);
    if(cv)
      length = convertInt64ToDouble(cv->getExactNumericValue());
    else
      return FALSE;
  }

  ColStatDescSharedPtr statDesc = (histograms)[columnIndex];
  rowCount = statDesc->getColStats()->getRowcount();
  uec = statDesc->getColStats()->getTotalUec();
  uec *= (length/origLength);
  minUec = MINOF(uec, pow(double(10), length));
  maxUec = MINOF(rowCount, pow(double(10), length));
  return TRUE;
}

// This will cover all of the math functions:
// Arithmetic functions - ABS, CEILING, FLOOR
// Geometric functions - DEGREES, RADIANS
// Trigonometric functions - ACOS, ASIN, ATAN, ATAN2, COS, COSH, SIN, SINH, TAN, TANH
// Other math functions - EXP, LOG, LOG10, POWER, SQRT
NABoolean MathFunc::calculateMinMaxUecs(ColStatDescList & histograms,
					 CostScalar & minUec,
					 CostScalar & maxUec)
{
  if(getArity() == 1)
    return child(0)->calculateMinMaxUecs(histograms, minUec, maxUec);
  else
    return FALSE;
}

NABoolean Modulus::calculateMinMaxUecs(ColStatDescList & histograms,
					 CostScalar & minUec,
					 CostScalar & maxUec)
{
  // In modulus function, the divisor stores the number of possible outcomes.
  // The divisor will be used as the UEC for modulus.
  NABoolean negate;
  double divisor = 0;
  ConstValue *cv = child(1)->castToConstValue(negate);
  if(cv)
  {
    divisor = convertInt64ToDouble(cv->getExactNumericValue());
    minUec = maxUec = divisor;
    return TRUE;
  }
  else
    return FALSE;
}

NABoolean DayOfWeek::calculateMinMaxUecs(ColStatDescList & histograms,
					 CostScalar & minUec,
					 CostScalar & maxUec)
{
  minUec = maxUec = 7;
  return TRUE;
}

NABoolean Extract::calculateMinMaxUecs(ColStatDescList & histograms,
					  CostScalar & minUec,
					  CostScalar & maxUec)
{
  switch(getExtractField())
  {
  case REC_DATE_YEAR:
  case REC_DATE_YEARQUARTER_EXTRACT:
  case REC_DATE_YEARMONTH_EXTRACT:
  case REC_DATE_YEARWEEK_EXTRACT:
    {
      // For YEAR function, the histogram data will be used to calculate
      // the number of years encompassed between boundaries of histogram data.
      CollIndex columnIndex;
      ValueIdSet leafValues;

      this->findAll(ITM_VEG_REFERENCE, leafValues, TRUE, TRUE);

      if(leafValues.entries() != 1)
	return FALSE;

      ValueId vid;
      leafValues.getFirst(vid);

      NABoolean statsExist = histograms.getColStatDescIndexForColumn(columnIndex, vid);

      if(!statsExist)
	return FALSE;

      ColStatDescSharedPtr statDesc = (histograms)[columnIndex];
      ColStatsSharedPtr colStats = statDesc->getColStats();

      if(colStats->isOrigFakeHist())
	return FALSE;

      NAString typeName = colStats->getStatColumns()[0]->getType()->getTypeName();
      if(typeName != "DATE" && typeName != "TIMESTAMP")
	return FALSE;

      EncodedValue minValue = colStats->getMinValue();
      EncodedValue maxValue = colStats->getMaxValue();

      // minUEC is number of days in the interval for now (1 day = 86400 sec)
      minUec = (maxValue.getDblValue() - minValue.getDblValue()) / 86400;

      switch(getExtractField())
        {
        case REC_DATE_YEAR:
          minUec /= 365;
          break;
        case REC_DATE_YEARQUARTER_EXTRACT:
          minUec /= 91;
          break;
        case REC_DATE_YEARMONTH_EXTRACT:
          minUec /= 30;
          break;
        case REC_DATE_YEARWEEK_EXTRACT:
          minUec /= 7;
          break;
        }

      minUec = MIN_ONE_CS(ceil(minUec.getValue()));
      maxUec = minUec + 1; // interval may wrap around a year, quarter, etc.
    }
    break;
  case REC_DATE_MONTH:
    minUec = maxUec = 12;
    break;
  case REC_DATE_DAY:
    minUec = maxUec = 31;
    break;
  case REC_DATE_HOUR:
    minUec = maxUec = 24;
    break;
  case REC_DATE_MINUTE:
    minUec = maxUec = 60;
    break;
  case REC_DATE_SECOND:
    minUec = maxUec = 60;
    break;
  default:
    minUec = maxUec = csMinusOne;
    return FALSE;
  }    
  return TRUE;
}

NABoolean BuiltinFunction::calculateMinMaxUecs(ColStatDescList & histograms,
					CostScalar & minUec,
					CostScalar & maxUec)
{
  switch(getOperatorType())
  {
  case ITM_ASCII:
    {
      minUec = maxUec = 256;

      // The UEC upper bound for ASCII is 256. If the UEC of the column
      // is less than 256, then the UEC will be equal to the UEC of the column.
      CostScalar colMinUec = csOne, colMaxUec = csOne;
      if((getArity() == 1) &&
	 (child(0)->calculateMinMaxUecs(histograms, colMinUec, colMaxUec)) &&
	 (colMinUec < 256))
	minUec = maxUec = colMinUec;
    }
    break;
  case ITM_CONCAT:
  case ITM_REPLACE:
    return child(0)->calculateMinMaxUecs(histograms, minUec, maxUec);
  case ITM_CHAR_LENGTH:
  case ITM_OCTET_LENGTH:
    {
      // For CHAR_LENGTH, OCTET_LENGTH, the first operand stores the maximum
      // length of the string. The UEC of these functions is the same.
      const NAType *operand1 = &child(0)->getValueId().getType();
      const CharType *charOperand = (CharType *) operand1;
      minUec = maxUec = charOperand->getStrCharLimit();
    }
    break;
  case ITM_POSITION:
    {
      // POSITION is similar to CHAR_LENGTH, OCTET_LENGTH except that the 
      // maximum string length is stored in the second operand.
      const NAType *operand2 = &child(1)->getValueId().getType();
      const CharType *mainStrCharType = (CharType *) operand2;
      minUec = maxUec = mainStrCharType->getStrCharLimit();
    }
    break;
  case ITM_CAST:
  case ITM_LOWER:
  case ITM_UPPER:
  case ITM_CONVERT:
    {
      if(getArity() == 1)
	return child(0)->calculateMinMaxUecs(histograms, minUec, maxUec);
      else
	return FALSE;
    }
    break;
  case ITM_TRIM:
    {
      if(getArity() == 2)
	return child(1)->calculateMinMaxUecs(histograms, minUec, maxUec);
      else
	return FALSE;
    }
    break;
  case ITM_CONVERTTIMESTAMP:
  case ITM_CURRENT_TIMESTAMP:
  case ITM_CURRENT_TIMESTAMP_RUNNING:
  case ITM_JULIANTIMESTAMP:
  case ITM_INTERNALTIMESTAMP:
    {
      minUec = maxUec = csOne;
    }
    break;
  default:
    minUec = maxUec = csMinusOne;
    return FALSE;
  }    
  return TRUE;
}

NABoolean ItmSequenceFunction::calculateMinMaxUecs(ColStatDescList & histograms,
					CostScalar & minUec,
					CostScalar & maxUec)
{
  minUec = maxUec = csOne;
  return TRUE;
}

NABoolean ConstValue::calculateMinMaxUecs(ColStatDescList & histograms,
					  CostScalar & minUec,
					  CostScalar & maxUec)
{
  minUec = maxUec = csOne;
  return TRUE;
}

// -----------------------------------------------------------------------
//  Methods for class VEGPredicate
// -----------------------------------------------------------------------

NABoolean VEGPredicate::synthSupportedOp() const
{
  return TRUE;
}

NABoolean VEGPredicate::applyDefaultPred(ColStatDescList & histograms,
			                         OperatorTypeEnum exprOpCode,
						 ValueId predValueId,
                                         NABoolean & globalPredicate,
                                         CostScalar *maxSelectivity)
{
  CostScalar defaultSel     = csOne;
  NABoolean statsExist = FALSE;
  NABoolean alreadyApplied  = FALSE;

  // leftColIndex contains the position of the left histogram whose statistics 
  // will be used for computing selectivity. 
  // In case the left child contains more than one columns, 
  // it would be the position of histogram with max UEC amongst left child
  CollIndex leftColIndex;

  // The leaf values of the left child
  ValueIdSet leftLeafValues;

  // could be a VEG predicate with no children
  CollIndexList statsToMerge(STMTHEAP);

  OperatorTypeEnum op = getOperatorType();

  // locate entries in this ColStatDescList that are associated
  // with the current VEG predicate.
  statsExist =
	histograms.identifyMergeCandidates( this, leftColIndex, statsToMerge );

  if (statsExist)
  {
    // Get the first column for which the histogram is found. 
    // multi-column histograms no longer are running around the system, but
    // there are still predicates that reach here
    //
    //   e.g., predicates involving aggregates (count(*) = 10)
    //   e.g., predicates on host variables (?p = 10)
    //
    // ... probably many more

    if ( op == ITM_VEG_PREDICATE ) // Transitive closure predicate
    {
	  // ----------------------------------------------------------
	  // This logic replicates a portion to the logic used in
	  // ColStatDescList::applyVEGPred to determine the parts of this
	  // VEGPred for which 'real' histogram manipulation was NOT done.
	  //
	  // Default selectivity should be applied *once* for *each* valid
	  // join that couldn't be executed because of its multi-column
	  // nature, but which would have otherwise been a valid join.
	  // (So long as an involved column hasn't already appeared in a
	  // default EQ-join.)
	  // ----------------------------------------------------------
	  // $$$ FCS_ONLY kludge -- this code should be reviewed and
	  // $$$ reconsidered post-FCS
	  //
	  // We reach this point when we're applying a predicate that looks
	  // like <col> = hostvar.
	  //
	  // We may reach this point in other instances, too.  For now, we're
	  // not going to worry about that.
	  //
	  // For this situation, we set the rowcount to be rc/uec, and set
	  // uec to be 1.  This reduction is exactly what you'd expect
	  // when applying an eqpred with a hostvar.
	  //
	  ColStatDescSharedPtr rootStatDesc = (histograms)[leftColIndex];
	  ColStatsSharedPtr rootColStats = rootStatDesc->getColStatsToModify();

          // compute maxSelectivity before histograms get modified
          if (maxSelectivity)
            {
              VEG* veg = ((VEGPredicate*)this)->getVEG();
              const ValueIdSet & values = veg->getAllValues();
              // we must distinguish between "X=?" and "X=Y".
              ItemExpr *cExpr = NULL;
              if (!values.referencesAConstExpr(&cExpr))
                { // veg is an "X=Y" predicate
                  // maxSelectivity("X=Y") == 1.0
                }
              else // veg is an "X=?" predicate
                {
                  // maxFreq = maxFrequency(X) for VEGPred "X=?"
                  // NB: "maxFreq = histograms.getMaxFreq(v);" may look
                  // attractive here. But, beware: it causes many maxCard
                  // tests to fail in compGeneral/test015 -- they get 0.
                  CostScalar maxFreq = csMinusOne;
                  for (ValueId v = values.init();    
                       values.next(v); 
                       values.advance(v))
                    {
                      if (v.getItemExpr()->getOperatorType() == 
                          ITM_BASECOLUMN)
                        {
                          maxFreq = histograms.getMaxFreq(v);
                          if (maxFreq > csMinusOne)
                            break;
                        }
                    }
                  // maxSelectivity("X=?") == max frequency(X) / total rows
                  if (maxFreq > csMinusOne && 
                      rootColStats->getRowcount() > csZero)
                    *maxSelectivity = 
                      MINOF(maxFreq / rootColStats->getRowcount(),
                            *maxSelectivity);
                } // veg is an "X=?" predicate
            } // if (maxSelectivity)
          else // maxSelectivity == NULL
          {
	  CostScalar oldUec  = rootColStats->getTotalUec();
	  CostScalar oldRows = rootColStats->getRowcount();

	  CostScalar newRows = oldRows / oldUec;

	  rootStatDesc->synchronizeStats(
	    oldRows,
	    newRows,
	    ColStatDesc::SET_UEC_TO_ONE
	    );

	  // If user specified selectivity for this predicate, we need to make
	  // adjustment in reduction to reflect that.
	  rootStatDesc->applySelIfSpecifiedViaHint(this, oldRows);

          } // else maxSelectivity == NULL
	  alreadyApplied = TRUE;
    } 
    else
    {
	  DCMPASSERT( FALSE ); // unexpected condition!
	  alreadyApplied = TRUE;
    }
  }
  return alreadyApplied;
}
