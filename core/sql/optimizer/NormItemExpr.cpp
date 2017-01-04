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
* File:         NormItemExpr.C
* Description:  Item expressions (normalizer-related methods)
* Created:      12/02/94
* Language:     C++
*
*
*
*      "If you cannot convince them, then confuse them"
*                     Harry S. Truman
*
******************************************************************************
*/


#include "Debug.h"
#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "NormWA.h"
#include "AllItemExpr.h"
#include "mdam.h"
#include "ValueDesc.h"
#include "RelGrby.h"
#include "RelJoin.h"
#include "RelUpdate.h"
#include "Refresh.h"
#include "ItemSample.h"
#include "ItmFlowControlFunction.h"
#include "ItemFuncUDF.h"

#ifndef TRANSFORM_DEBUG_DECL      // artifact of NSK's OptAll.cpp ...
#define TRANSFORM_DEBUG_DECL
DBGDECLDBG( dbg; )
DBGDECL( static NAString unp; )
#endif


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// warning elimination (removed "inline")
static NABoolean canBeSQLUnknown(const ItemExpr *ie,
                                 NABoolean typeMustBeSQLBoolean = TRUE)
{
  const NAType &typ = ie->getValueId().getType();
  if (typ.getTypeQualifier() == NA_BOOLEAN_TYPE)
    return ((SQLBooleanRelat &)typ).canBeSQLUnknown();
  CMPASSERT(!typeMustBeSQLBoolean);
  return FALSE;
}

// -----------------------------------------------------------------------
// Ansi 8.12
// -----------------------------------------------------------------------
static void applyTruthTable(ItemExpr * ie, ExprValueId & locationOfPointerToMe)
{
  enum truthResultEnum { TRU_ = ITM_RETURN_TRUE,   // true
                         FAL_ = ITM_RETURN_FALSE,   // false
                         UNK_ = ITM_RETURN_NULL,   // unknown
                         SIB_ = ITM_CASE,      // sibling's truth value
                         NOP_ = ITM_NO_OP };      // continue|leave as is
  truthResultEnum t, f, u, result = NOP_;
  Int32 ucnt = 0;
  ItemExpr *ptrToMe = locationOfPointerToMe.getPtr();
  if (!ptrToMe) return;
  switch (ptrToMe->getOperatorType())
    {
    // Only the two BiLogic's use SIB_ and NOP_
    case ITM_AND:             t = SIB_; f = FAL_; u = NOP_; break;
    case ITM_OR:              t = TRU_; f = SIB_; u = NOP_; break;
    case ITM_NOT:             t = FAL_; f = TRU_; u = UNK_; break;
    case ITM_IS_TRUE:         t = TRU_; f = FAL_; u = FAL_; break;
    case ITM_IS_FALSE:        t = FAL_; f = TRU_; u = FAL_; break;
    case ITM_IS_UNKNOWN:      t = FAL_; f = FAL_; u = TRU_; break;
    case ITM_IS_NOT_UNKNOWN:  t = TRU_; f = TRU_; u = FAL_; break;
    default:                  return;
    }
  DBGSETDBG( "TRANSFORM_DEBUG" )
  DBGIF( cerr << "## " << ptrToMe->getText(); )
  Int32 i = ptrToMe->getArity();
  for (; result == NOP_ && i--; )
    {
      ItemExpr *child = ptrToMe->child(i);
      // If child was eliminated, it must have always evaluated to True
      if (!child)
        {
           result = t;
           DBGIF( cerr << " # elim/true"; )
        }
      else if (child->getArity() == 0)      // unconditional ITM_RETURN_xxx
        {
           DBGIF( cerr << " # " << child->getText(); )
           switch (child->getOperatorType())
           {
              case ITM_RETURN_TRUE:    result = t; break;
              case ITM_RETURN_FALSE:   result = f; break;
              case ITM_RETURN_NULL:    result = u; ucnt++; break;
              default:                 break;
           }
        }
      else DBGIF( cerr << " #- " << child->getText(); )
    }
  if (ucnt && result == NOP_) result = UNK_;    // BiLogic's only
  if (result != NOP_)
    {
      ItemExpr *replacement;
      if (result == SIB_)
        {
           if (i == 1) i = 0;
           else if (i == 0) i = 1;
           else CMPASSERT(FALSE);    // see for-loop test
           replacement = ptrToMe->child(i);
        }
      else
        replacement = new HEAP BoolVal(OperatorTypeEnum(result));

      DBGIF( cerr << " ==> " << replacement->getText(); )

      ie->getValueId().replaceItemExpr(replacement);
      replacement->synthTypeAndValueId(TRUE);
      locationOfPointerToMe = replacement;
    }
    DBGIF( cerr << endl; )
} // applyTruthTable()

// -----------------------------------------------------------------------
// This is a utility for building a tree of predicates that
// has a backbone of AND/OR nodes.
// -----------------------------------------------------------------------
static ItemExpr * buildPredTree(ItemExpr * rootPtr, ItemExpr * subtreePtr,
                         OperatorTypeEnum logicalConnectiveType = ITM_AND)
{
  ItemExpr * newrootPtr;
  if (rootPtr)
    {
      newrootPtr = new HEAP BiLogic(logicalConnectiveType, rootPtr, subtreePtr);
      newrootPtr->markAsTransformed();
    }
  else
    newrootPtr = subtreePtr;
  // Walk through this new tree, synthesizing the type and
  // allocating a ValueId for each operator that requires it.
  newrootPtr->synthTypeAndValueId();
  return newrootPtr;
} // buildPredTree()

// -----------------------------------------------------------------------
// This is a utility for building a tree of comparison  predicates.
// -----------------------------------------------------------------------
ItemExpr * buildComparisonPred(   ItemExpr * rootPtr,
                                  ItemExpr * leftSubtreePtr,
                                  ItemExpr * rightSubtreePtr,
                                  OperatorTypeEnum comparisonOpType = ITM_EQUAL,
                                  NABoolean specialNulls=FALSE  //++MV - Irena
                              )
{
  ItemExpr * eqPredPtr =
    new HEAP BiRelat  (   comparisonOpType,
                          leftSubtreePtr,
                          rightSubtreePtr,
                          specialNulls //++MV - Irena
                      );
  return buildPredTree(rootPtr, eqPredPtr, ITM_AND);
} // buildComparisonPred()

inline static Int32 isAColumnOrUserInput(const ItemExpr *ie)
{
  if (((ItemExpr *)ie)->isAColumnReference())     return 10;
  // VEG on ITM_UNIQUE_EXECUTE_ID is disabled, see triggers team for explanation
  if (ie->isAUserSuppliedInput() && ie->getOperatorType()!=ITM_UNIQUE_EXECUTE_ID) // -- Triggers
     return 1;
  return 0;
}


// --------------------------------------------------------------------
//
// If a recursive/cascaded triggers backbone has a connection to the
// triggering generic update the connection is using @old and @new
// columns from the triggering generic update.
// We found out that equality relation between each two levels of cascaded
// triggers might cause the creation of a huge VEGREF that the optimizer
// can't handle in a reasonable time.
// To prevent this problem we will block the creation of VEGs on equal predicates
// involving a @old/ @new column on high cascade levels.
// Cascade levels are counted by inBlockedUnionCount_, since every cascaded
// trigger backbone top node is a BlockedUnion.
//
// --------------------------------------------------------------------
inline static Int32  bothAreColumnsOrUserInput(   // AND AT LEAST ONE IS A COLUMN
                     NormWA & normWARef, const ItemExpr *i0, const ItemExpr *i1)
{
  Int32 ret = isAColumnOrUserInput(i0);

  if (ret)
    {
      ret += isAColumnOrUserInput(i1);

      // If both are columns, ret is 20, fine;
      // if one is a column and the other an input, ret is 11, fine;
      // if one is a column and the other is not col nor inp, ret is 10, reject;
      // if one or both are just inputs, ret is 1 or 2, reject.
      if (ret < 11) ret = 0;

    }
  return ret;
}

// ***********************************************************************
// $$$$ ItemExpr
// member functions for class ItemExpr
// ***********************************************************************

// -----------------------------------------------------------------------
// Each operator supports a (virtual) method for transforming a
// scalar expression to a canonical form
// -----------------------------------------------------------------------
void ItemExpr::transformNode(NormWA & normWARef,
                             ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
                             const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }
  Int32 arity = getArity();

  for (Int32 i = 0; i < arity; i++)
    child(i)->getReplacementExpr()->transformNode(normWARef, child(i),
                                        introduceSemiJoinHere, externalInputs);

  markAsTransformed();

  // Set return value
  locationOfPointerToMe = this;
} // ItemExpr::transformNode()

// -----------------------------------------------------------------------
// A method for checking whether a predicate eliminates null
// augmented rows produced by a left join.
// ----------------------------------------------------------------------
NABoolean ItemExpr::predicateEliminatesNullAugmentedRows
                       (NormWA & /* normWARef */,
                        ValueIdSet & /* outerReferences */)
{
  return FALSE;
} // ItemExpr::predicateEliminatesNullAugmentedRows()

// -----------------------------------------------------------------------
// A method for inverting (finding the inverse of) the operators
// in a subtree that is rooted in a NOT.
// ----------------------------------------------------------------------
ItemExpr * ItemExpr::transformSubtreeOfNot(NormWA & /* normWARef */,
                                           OperatorTypeEnum /* falseOrNot */)
{
  return this;
} // ItemExpr::transformSubtreeOfNot()

// -----------------------------------------------------------------------
// ItemExpr::initiateLeftToInnerJoinTransformation()
// ----------------------------------------------------------------------
ItemExpr * ItemExpr::initiateLeftToInnerJoinTransformation(NormWA & /*normWARef*/)
{
  return getReplacementExpr(); // return the replacement expression
} // ItemExpr::initiateLeftToInnerJoinTransformation()

// -----------------------------------------------------------------------
// Each operator supports a (virtual) method for transforming a query
// tree to a canonical form
// -----------------------------------------------------------------------
ItemExpr * ItemExpr::normalizeNode(NormWA & normWARef)
{
  // ---------------------------------------------------------------------
  // If the expression is column reference (such as a base column, an
  // index column, a rename column or an instantiate null) or a user
  // given value (such as a constant, host variable or a parameter)
  // check whether it is a member of a ValueId Equality Group (VEG).
  // If so, replace the expression with a wild card expression,
  // called a VEGReference, which is associated with its VEG.
  // ---------------------------------------------------------------------
  if (isAColumnOrUserInput(this))
    {
      // -----------------------------------------------------------------
      // Normalize the child.
      // -----------------------------------------------------------------
      Int32 arity = getArity();

      for (Int32 i = 0; i < arity; i++)
         child(i) = child(i)->getReplacementExpr()->normalizeNode(normWARef);

      // Now do non-recursive part
      return( normalizeNode2( normWARef ) );
    }
  // ---------------------------------------------------------------------
  // Normalize each child subtree.
  // ---------------------------------------------------------------------
  else
    {
      if (nodeIsNormalized())
         return getReplacementExpr();

      if ((NOT normWARef.isInJoinPredicate()) ||
          (CmpCommon::getDefault(COMP_BOOL_124) == DF_ON))
      {
         markAsNormalized();
         Int32 arity = getArity();
         for (Int32 i = 0; i < arity; i++)
           child(i) = child(i)->getReplacementExpr()->normalizeNode(normWARef);
      }
      else
      {
         Int32 arity = getArity();
         ItemExpr *newExpr = this->copyTopNode(NULL,CmpCommon::statementHeap());
         
         for (Int32 i = 0; i < arity; i++)
           newExpr->setChild(i,child(i)->getReplacementExpr()->normalizeNode(normWARef));
         newExpr->setReplacementExpr(newExpr);

         newExpr->synthTypeAndValueId();
         newExpr->markAsNormalized();

         return newExpr->getReplacementExpr();
      }
    }

  return getReplacementExpr();

} // ItemExpr::normalizeNode()


//
// normalizeNode2() - a helper routine for ItemExpr::normalizeNode()
//
// NOTE: The code in this routine came from the previous version of
//       ItemExpr::normalizeNode().   It has been pulled out into a separate
//       routine so that the C++ compiler will produce code that needs
//       signficantly less stack space for the recursive
//       ItemExpr::normalizeNode() routine.
//
ItemExpr * ItemExpr::normalizeNode2(NormWA & normWARef)
{
      // -----------------------------------------------------------------
      // If this expression is a member of a ValueId Equality Group
      // (VEG), then replace it with a reference to the VEG.
      // -----------------------------------------------------------------
      ItemExpr * vegrefPtr = normWARef.getVEGReference(getValueId());
      // we either got a VEGRef or an ITM_CONSTANT back
      if (vegrefPtr)
        // return without setting the replacement expression
        // the same valueid could map to different VEGRefs in different
        // regions
        return vegrefPtr;
      else
        return getReplacementExpr(); // returns this
}

//
//  This method is invoked from inside a ROWS SINCE operator.
//  Traverses tree looking for THIS functions.
//
void ItemExpr::transformNotTHISFunction()
{
 Int32 arity = getArity();

 for (Int32 i = 0; i < arity; i++)
 {
   if (child(i)->containsTHISFunction())
   {
     if (child(i)->getOperatorType() != ITM_THIS)
     {
      child(i)->transformNotTHISFunction();
     }
   }
   else
   {
     ItmSeqNotTHISFunction *newChild = new HEAP ItmSeqNotTHISFunction (child(i));
     newChild->synthTypeAndValueId(TRUE);
     child(i) = newChild;
     // child(i)->getValueId().replaceItemExpr(newChild);
     // setReplacementExpr(newChild);
   }
 }
} // ItemExpr::transformNotTHISFunction()

//
// Virtual function that allows the above NOT THIS transformation
// Redefined for ItmSequenceFunction classes

NABoolean ItemExpr::containsTHISFunction()
{
  Int32 arity = getArity();
  NABoolean result = FALSE;

  // --------------------------------------------------------
  // This check cannot exit early, because it also must
  // perform an illegal nesting check on all the children.
  // --------------------------------------------------------
  for (Int32 i = 0; i < arity; i++)
  {
   if (child(i)->containsTHISFunction())
     result = TRUE;
  }
  return result;
} //  ItemExpr::containsThis()


NABoolean ItemExpr::canTransformToSemiJoin(ItemExprList& valuesListIE, 
                                           TableDesc* tdesc, Lng32& numParams, 
                                         ValueId& colVid, CollHeap* h) const
{
  valuesListIE.clear();

  if (ActiveSchemaDB()->getDefaults().getAsLong(OR_PRED_TO_SEMIJOIN)== 0)
    return FALSE ;  // feature is turned OFF

  ItemExpr *predIE = (ItemExpr *) this;
  if(CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
    predIE = revertBackToOldTree(h, (ItemExpr *)this);

  if (predIE->getOperatorType() != ITM_OR) // If its not an OR there is nothing to transform
    return FALSE;  // may need to relax this for anti-semi-join NOT(OR ...)

  ValueIdList eqList;
  NABoolean status = predIE->convertToValueIdList(eqList,NULL, ITM_OR);

  if(status)
    return FALSE;

  ((BiLogic *) predIE)->setNumLeaves(eqList.entries());
  ValueIdSet noCharacteristicInputs, dummy1, dummy2, dummy3;  // empty sets
  GroupAttributes scanGA;
  NABoolean isCovered = FALSE;
  BiRelat * eqExpr = NULL;
  ItemExpr *  rightChild  = NULL;
  ItemExpr * colExpr = NULL;
  ValueId leftChildId;
  Convert * cnv = NULL;
  NABuiltInTypeEnum rcTypeEnum = NA_UNKNOWN_TYPE;

  if (eqList[0].getItemExpr()->getOperatorType() == ITM_EQUAL)
  {
    scanGA.addCharacteristicOutputs(tdesc->getColumnList());
    eqExpr = (BiRelat *) eqList[0].getItemExpr() ;
    colExpr = eqExpr->child(0).getPtr();
    rightChild = eqExpr->child(1).getPtr();

    isCovered = colExpr->isCovered(noCharacteristicInputs,
                                      scanGA,
                                      dummy1,
                                      dummy2,
                                      dummy3);

    // check if left side of '=' is covered by columns of TableDesc
    // 'a', 'a+1", 'a+b' are all allowed expressions.
    if (isCovered)
      leftChildId = colExpr->getValueId();
    else
      return FALSE;

    rcTypeEnum = (rightChild->getValueId()).getType().getTypeQualifier();
    if (rcTypeEnum == NA_UNKNOWN_TYPE)
      return FALSE;
  }

  numParams = 0;
  for (CollIndex i = 0; i < eqList.entries(); i++)
  {
    if (eqList[i].getItemExpr()->getOperatorType() != ITM_EQUAL)
      return FALSE;

    eqExpr = (BiRelat *) eqList[i].getItemExpr() ;
    if (eqExpr->child(0)->getValueId() != leftChildId)
      return FALSE;  // left side of '=' has different expressions, a = 1 OR b = 1
    rightChild = eqExpr->child(1).getPtr();
    if (rcTypeEnum != (rightChild->getValueId()).getType().getTypeQualifier())
      return FALSE; // right side of '=' has to be compatible

    if (!(rightChild->doesExprEvaluateToConstant(FALSE,TRUE)))
      return FALSE; // right side of '=' must be nonstrict constant (we also look inside VEG)
    if (!(rightChild->doesExprEvaluateToConstant(TRUE,TRUE))) // look for hostvars or params
      numParams++; // look for strict constants so that we can keep track of hostvars/params

    if (rightChild->getOperatorType() == ITM_VEG_REFERENCE)
    {
      // if we have a VEG on the right side, get the underlysing constant
      // we don't want to put a VEG in the tupleExprTree_ as this causes an 
      // abort when the tupleExprTree_ is copied in later phases. A VEGRef cannot
      // be currently copied with copyTopNode.
      const VEG * refVEG = ((VEGReference *)rightChild)->getVEG();
      rightChild = (refVEG->getAConstantHostVarOrParameter()).getItemExpr();
    }
    
    cnv = new(h) Convert(rightChild);
    cnv->synthTypeAndValueId();
    valuesListIE.insert(cnv);    
  }
  colVid = colExpr->getValueId();
  return TRUE;
}


// ***********************************************************************
// $$$$ Aggregate
// member functions for class Aggregate
// ***********************************************************************
void Aggregate::transformNode(NormWA & normWARef,
                              ExprValueId & locationOfPointerToMe,
                              ExprGroupId & introduceSemiJoinHere,
                              const ValueIdSet & externalInputs)
{
  // ---------------------------------------------------------------------
  // Transform the operands of the Aggregate
  // ---------------------------------------------------------------------
  normWARef.setNullFlag();
  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);
  normWARef.restoreNullFlag();

  // ---------------------------------------------------------------------
  // If this aggregate function is not sensitive to duplicate values
  // (like MIN, MAX) then a DISTINCT in this aggregate is unnecessary,
  // so transform, for example, MAX(DISTINCT x) into MAX(x)
  // ---------------------------------------------------------------------
  if (!isSensitiveToDuplicates())
    setDistinct(FALSE);

  // Set return value
  locationOfPointerToMe = this;
}  // Aggregate::transformNode()

ItemExpr * Aggregate::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return getReplacementExpr();
  markAsNormalized();

  // Set the null flag if we care about null values
  normWARef.setNullFlag();
  child(0) = child(0)->getReplacementExpr()->normalizeNode(normWARef);

  if (isDistinct())
    distinctId_ = distinctId_.getItemExpr()->getReplacementExpr()->
                                        normalizeNode(normWARef)->getValueId();

  normWARef.restoreNullFlag();

  return this;
} // Aggregate::normalizeNode()

// ***********************************************************************
// $$$$ Assign
//  member functions for class Assign
// ***********************************************************************
ItemExpr * Assign::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return getReplacementExpr();
  markAsNormalized();

  // Only the source needs to be normalized
  child(1) = child(1)->getReplacementExpr()->normalizeNode(normWARef);

  return this;
}





// ***************************************************************************
// $$$$ Between
// member functions for class Between
// ***************************************************************************

//----------------------------------------------------------------------------
//++ MV OZ
void
Between::setDirectionVector(const IntegerList & directionVector,
                            CollHeap* heap)
{
  CMPASSERT(0 != directionVector.entries() && NULL != heap);

  pDirectionVector_ = new (heap)IntegerList(heap);
  *pDirectionVector_ = directionVector;
}


//----------------------------------------------------------------------------
ItemExpr * Between::transformIntoTwoComparisons()
{
  ItemExpr *tfm = NULL;
  OperatorTypeEnum leftBoundryOp = ITM_GREATER;
  OperatorTypeEnum rightBoundryOp = ITM_LESS;
  ItemExpr *leftVal = child(0).getPtr();
  ItemExpr *startVal = child(1).getPtr();
  ItemExpr *endVal = child(2).getPtr();
  NABoolean optimizeForEquals = (leftBoundryIncluded_ AND
                                 rightBoundryIncluded_ AND
                                 pDirectionVector_ == NULL);
                                

  // THE DEFAULT BEHAVIOUR
  if (TRUE == leftBoundryIncluded_)
  {
    leftBoundryOp = ITM_GREATER_EQ;
  }

  if (TRUE == rightBoundryIncluded_)
  {
    rightBoundryOp = ITM_LESS_EQ;
  }

  // ---------------------------------------------------------------------
  // Try to optimize the case "a between x and x" and produce an
  // equals predicate instead, do this first for prefixes of multi-
  // valued between predicates like (a,b,c) between (1,1,2) and (1,1,4)
  // ---------------------------------------------------------------------
  if (optimizeForEquals AND
      leftVal->getOperatorType() == ITM_ITEM_LIST)
    {
      ItemExprList leftList(leftVal, HEAP);
      ItemExprList startList(startVal, HEAP);
      ItemExprList endList(endVal, HEAP);

      while (leftList.entries() > 0 AND
             startList[0] == endList[0])
        {
          ItemExpr *eqPred = 
            new HEAP BiRelat(ITM_EQUAL,
                             leftList[0],
                             startList[0]);
          if (tfm)
            tfm = new HEAP BiLogic(ITM_AND, tfm, eqPred);
          else
            tfm = eqPred;

          leftList.removeAt(0);
          startList.removeAt(0);
          endList.removeAt(0);
        }

      if (tfm)
        {
          // we took care of a prefix of the comparisons,
          // make a new list without those equal values
          if (leftList.entries() > 0)
            {
              leftVal = leftList.convertToItemExpr();
              startVal = startList.convertToItemExpr();
              endVal = endList.convertToItemExpr();
            }
          else
            leftVal = startVal = endVal = NULL;
        }
    }

  if (leftVal)
    {
      // ---------------------------------------------------------------------
      // Replace  col between val1 and val2 with
      //          col >= val1 and col <= val2
      // NOTE: This transformation assumes the ascii collating sequence.
      //       When other collations are supported, the transformation
      //       rule will have to be different.
      // ---------------------------------------------------------------------

      BiRelat * leftCondition =
        new HEAP BiRelat(leftBoundryOp, leftVal, startVal);

      BiRelat * rightCondition =
        new HEAP BiRelat(rightBoundryOp, leftVal, endVal);


      // NOTE: we copy the pointer as is. it should probably have been marked
      // as const but there was to much damn code to change so I gave up.
      //++ MV OZ
      leftCondition->setDirectionVector((IntegerList *)pDirectionVector_);
      rightCondition->setDirectionVector((IntegerList *)pDirectionVector_);


      ItemExpr *noneqPreds = new HEAP BiLogic(ITM_AND, leftCondition, rightCondition);

      if (tfm)
        tfm = new HEAP BiLogic(ITM_AND, tfm, noneqPreds);
      else
        tfm = noneqPreds;
    }

  CMPASSERT(tfm);
  return tfm;
}

ItemExpr * Between::transformMultiValuePredicate(
                             NABoolean flattenSubqueries,
                             ChildCondition condBiRelat)
{
  ItemExpr *tfm = transformIntoTwoComparisons()->
                   transformMultiValuePredicate(flattenSubqueries, condBiRelat);

  DBGSETDBG( "TRANSFORM_DEBUG" )
  DBGIF(
    if (tfm) {
    unp = "";
    tfm->unparse(unp);
    cerr << "Between MVP: " << unp << " " << (void*)this << endl;
    }
  )

  return tfm;
}

#pragma nowarn(1506)   // warning elimination
void Between::transformNode(NormWA & normWARef,
                            ExprValueId & locationOfPointerToMe,
                            ExprGroupId & introduceSemiJoinHere,
                            const ValueIdSet & externalInputs)
{
  DBGSETDBG( "TRANSFORM_DEBUG" )
  DBGIF(
    unp = "";
    unparse(unp);
    cerr << (Int32)getOperatorType() << " "
         << (Int32)getValueId() << " "
         << (void *)this << " "
         << unp << endl;
  )

  // ---------------------------------------------------------------------
  // Transform the operands of the Between
  // ---------------------------------------------------------------------
  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);

  ItemExpr *tfm = transformIntoTwoComparisons();
  getValueId().replaceItemExpr(tfm);
  tfm->synthTypeAndValueId(TRUE);
  locationOfPointerToMe = tfm;

  if(isSelectivitySetUsingHint())
  {
    double newSelFactor = sqrt(getSelectivityFactor());
    
    locationOfPointerToMe->child(0)->setSelectivitySetUsingHint();
    locationOfPointerToMe->child(0)->setSelectivityFactor(newSelFactor);

    locationOfPointerToMe->child(1)->setSelectivitySetUsingHint();
    locationOfPointerToMe->child(1)->setSelectivityFactor(newSelFactor);
  }

  // Make sure the new nodes are transformed
  locationOfPointerToMe->transformNode(normWARef, locationOfPointerToMe,
                                       introduceSemiJoinHere, externalInputs);

  // Set the replacement expression
  setReplacementExpr(locationOfPointerToMe);

} // Between::transformNode()
#pragma warn(1506)  // warning elimination

ItemExpr * Between::transformSubtreeOfNot(NormWA & normWARef,
                                          OperatorTypeEnum falseOrNot)
{
  ItemExpr *tfm = transformIntoTwoComparisons();

  // Do not synthTypeAndValueId() yet
  return tfm->transformSubtreeOfNot(normWARef,falseOrNot);
} // Between::transformSubtreeOfNot()

// ***********************************************************************
// $$$$ BuiltinFunction
// member functions for class BuiltinFunction
// ***********************************************************************
void BuiltinFunction::transformNode(NormWA & normWARef,
                                    ExprValueId & locationOfPointerToMe,
                                    ExprGroupId & introduceSemiJoinHere,
                                    const ValueIdSet & externalInputs)
{
  // Indicate that we are processing a complex scalar expression.
  normWARef.setComplexScalarExprFlag();
  // ---------------------------------------------------------------------
  // Transform the operands of the Builtinfunction.
  // ---------------------------------------------------------------------
  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);
  normWARef.restoreComplexScalarExprFlag();

} // BuiltinFunction::transformNode()

ItemExpr * BuiltinFunction::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return getReplacementExpr();

  // Indicate that we are processing a complex scalar expression.
  normWARef.setComplexScalarExprFlag();
  // ---------------------------------------------------------------------
  // Normalize the operands of the Builtinfunction.
  // ---------------------------------------------------------------------
  // Must call the base class version of normalizeNode. We cannot
  // call normalizeNode on the child directly. This is because
  // a builtin function node can be a "columnOrUserSuppliedInput". Only
  // the base class version has code for this. We could duplicate it
  // here, but then that would be duplicated code.
  ItemExpr * normalizedExpr = ItemExpr::normalizeNode(normWARef);
  normWARef.restoreComplexScalarExprFlag();

  return normalizedExpr;

} // BuiltinFunction::normalizeNode()


// ***********************************************************************
// UDFunction
// member functions for class UDFunction
// ***********************************************************************
void UDFunction::transformNode(NormWA & normWARef,
                                    ExprValueId & locationOfPointerToMe,
                                    ExprGroupId & introduceSemiJoinHere,
                                    const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
    {
      // Return the address of the expression that was used for replacing
      // this subquery in an earlier call.
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  // Indicate that we are processing a complex scalar expression.
  normWARef.setComplexScalarExprFlag();


  // Want to transform our inputs before we transform ourselves.
  // This maintains a tree were the UDF is on the right side of a RoutineJoin.
  if (inputVars_.transformNode(normWARef, introduceSemiJoinHere, 
                                          externalInputs))
  {
    introduceSemiJoinHere->transformNode(normWARef, introduceSemiJoinHere);
    CMPASSERT( introduceSemiJoinHere->nodeIsTransformed());
  }


  // ---------------------------------------------------------------------
  // Transform the UDFunction ItemExpr into a RoutineJoin and an 
  // IsolatedScalarUDF
  // ---------------------------------------------------------------------
  transformToRelExpr(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);

  markAsTransformed();

  setReplacementExpr(locationOfPointerToMe);

  normWARef.restoreComplexScalarExprFlag();

} // UDFunction::transformNode()



// -----------------------------------------------------------------------
// UDFunction::transformToRelExpr()
// Perform the UDFunction -> RoutineJoin + IsolatedScalarUDF transformation
// -----------------------------------------------------------------------
void UDFunction::transformToRelExpr(NormWA & normWARef,
                                 ExprValueId & locationOfPointerToMe,
                                 ExprGroupId & introduceSemiJoinHere,
                                 const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
    return;
  markAsTransformed();

  const RoutineDesc *rdesc = getRoutineDesc();

  CMPASSERT(rdesc); // Make sure we actually have one..

  if (rdesc->getEffectiveNARoutine() != NULL && 
      (!rdesc->getEffectiveNARoutine()->isIsolate()))
    return; // Don't want to transform trusted Functions to RelExprs.

  CMPASSERT(getOperatorType() == ITM_USER_DEF_FUNCTION);

  // This is going to work very similar to what we do for subqueries.
  // We are going to create a RoutineJoin, and an appropriate right child
  // the join. For starters it will be an IsolatedScalarUDF RelExpr.


  RelExpr *newRightChild;
//  ValueIdSet inputValues;
  ValueIdSet leafInputValues;
  GroupAttributes emptyGA;
    
    // We use the dummyOutputs when we generate the UDF relexpr
    // The real outputs of the routine will be assigned below.
  ValueIdList dummyOutputs;

  switch (getRoutineDesc()->getEffectiveNARoutine()->getRoutineType())
  {
     case COM_SCALAR_UDF_TYPE:
     case COM_UNIVERSAL_UDF_TYPE:
     case COM_ACTION_UDF_TYPE:
     {
    
       ItemExprList *params  = new(normWARef.wHeap()) 
                                  ItemExprList(normWARef.wHeap());
     
       Int32 arity = getArity();

       IsolatedScalarUDF   *isUdf;

       for (Int32 i=0; i< arity; i++) 
       {
         params->insertAt(i, child(i)->castToItemExpr());
       }
       
       isUdf = new(normWARef.wHeap())
                     IsolatedScalarUDF(
                        getRoutineDesc()->getEffectiveNARoutine()->getSqlName(),
                        params, dummyOutputs, normWARef.wHeap());


       delete params; // We don't need this list any more

       // We want to make make sure we mark the node as bound as some of
       // the RelRoutine methods behave different if the node is bound or not.
       isUdf->markAsBound();

       // The routine desc points to both the NARoutine structs.
       isUdf->setRoutineDesc(udfDesc_);
    

       // Get the valueIds of the parameters
       isUdf->gatherParamValueIds(isUdf->getProcAllParamsTree(), 
                                  isUdf->getProcAllParamsVids());
     
       // Clear out the params Tree, we don't need it anymore.
       isUdf->setProcAllParamsTree(NULL);

       // For UDFs all parameters are inputs

       isUdf->getProcInputParamsVids() = isUdf->getProcAllParamsVids();


       // The inputVars_ do not have the casts that we create 
       // at bind time for compatible types. Use it for our actual inputs.
       leafInputValues = inputVars_;

      
       // The characteristic outputs of the IsolatedScalarUDF RelExpr will
       // be that of the return parameters of the UDFunction
       //
       
       newRightChild = isUdf;

       break;
     }
     case COM_TABLE_UDF_TYPE:
     case COM_UNKNOWN_ROUTINE_TYPE:
     default:
     {
       *CmpCommon::diags() << DgSqlCode(2997)
              << DgString1("Unsupported Function type");

       return ;
       break; 
     }

  }


  ItemExpr *replacementExpr = NULL;

  
  if (normWARef.inValueIdProxy())
  {
       // We have already split the output of the MVF earlier and 
       // represented them with ValueIdProxies. This happens when the UDF
       // is part of select lists or equivalent (set clause in update etc)
       // and we only need to associate the valueId for the UDF with the 
       // first of its output.
       replacementExpr = udfDesc_->getOutputColumnList()[0].getItemExpr();
  }
  else
  {
       replacementExpr = udfDesc_->getOutputColumnList().rebuildExprTree(
                                                     ITM_ITEM_LIST,FALSE,FALSE);
  }


  //  Pass along what the function needs to be replaced with.
  locationOfPointerToMe = replacementExpr;

  // Replace the itemExpr the valueId for UDFunction points to
  // to point to the output of the ScalarIsolatedUDF relexpr.

  ValueId  oldOutId = replacementExpr->getValueId();
  getValueId().replaceItemExpr(replacementExpr);

     // Update the valuedId we know the output column as in the RoutineDesc.
     // This will now be the new output value for the RelRoutine

  udfDesc_->getOutputColumnList().substituteValueIds(oldOutId, getValueId());

  Join *newJoin;

  newJoin = new(normWARef.wHeap())
                  Join(introduceSemiJoinHere, newRightChild, REL_ROUTINE_JOIN);
  newJoin->setGroupAttr(new(normWARef.wHeap()) GroupAttributes());


  ValueIdSet outputValues;
  ValueIdSet outerReferences;

  // -------------------------------------------------------------
  // Setup the characteristic inputs and outputs for the IsolatedScalarUDF node
  // -------------------------------------------------------------
  newRightChild->getPotentialOutputValues(outputValues);
  newRightChild->getGroupAttr()->addCharacteristicOutputs(outputValues);
  newRightChild->getGroupAttr()->addCharacteristicInputs(leafInputValues);

  // -------------------------------------------------------------
  // Now minimize the characteristic inputs for the IsolatedScalarUDF node
  // This will weed out any constants, etc that doesn't need to flow...
  // -------------------------------------------------------------

  newRightChild->getGroupAttr()->getCharacteristicInputs().
                                          getOuterReferences(outerReferences);
  newRightChild->getGroupAttr()->setCharacteristicInputs(outerReferences);

  // ---------------------------------------------------------------------
  // Initialize the dataflow for the tuple-substitution join that was
  // allocated above.
  // ---------------------------------------------------------------------
  newJoin->getPotentialOutputValues(outputValues);
  newJoin->getGroupAttr()->addCharacteristicOutputs(outputValues);

  // subtract from the external inputs to the joins values that are
  // produced by the children of the join.
  ValueIdSet realExternalInputs = externalInputs;
  realExternalInputs -= outputValues;
  newJoin->getGroupAttr()->addCharacteristicInputs(realExternalInputs);

  newJoin->child(1)->getGroupAttr()->addCharacteristicInputs(externalInputs);
  newJoin->child(1)->getGroupAttr()->addCharacteristicInputs
             (newJoin->child(0)->getGroupAttr()->getCharacteristicOutputs());


  introduceSemiJoinHere = newJoin;


} // UDFunction::transformToRelExpr()


ItemExpr * UDFunction::normalizeNode(NormWA & normWARef)
{
  // Should never get here as we transform all UDFunctions. However, if we do
  // that means that we missed one somewhere...
  CMPASSERT(0);

  return NULL;

} // UDFunction::normalizeNode()

// -----------------------------------------------------------------------
// member functions for class InstantiateNull
// -----------------------------------------------------------------------
void InstantiateNull::transformNode(NormWA & normWARef,
                                    ExprValueId & locationOfPointerToMe,
                                    ExprGroupId & introduceSemiJoinHere,
                                    const ValueIdSet & externalInputs)
{
  // Indicate that we are processing a complex scalar expression.
  normWARef.setComplexScalarExprFlag();
  // ---------------------------------------------------------------------
  // Transform the operands of the Instantiate Null.
  // ---------------------------------------------------------------------
  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);
  normWARef.restoreComplexScalarExprFlag();
} // InstantiateNull::transformNode()

// -----------------------------------------------------------------------
// InstantiateNull::initiateLeftToInnerJoinTransformation()
// For each node in the tree of the form
//        InstantiateNull(InstantiateNull(InstantiateNull(X)))
// 1) Mark the Zone where the value is first produced as "To Be Merged".
// 2) Recurse on InstantiateNulls
// 3) Return a pointer to X.
// -----------------------------------------------------------------------
ItemExpr * InstantiateNull::initiateLeftToInnerJoinTransformation(NormWA & normWARef)
{
  if (!NoCheckforLeftToInnerJoin)
  {
    // ---------------------------------------------------------------------
    // Locate the VEGRegion that provides the data that is null-instantiated
    // by this InstantiateNull and mark it as "To Be Merged".
    // ---------------------------------------------------------------------
    normWARef.locateVEGRegionAndMarkToBeMergedRecursively(getValueId());

    // ---------------------------------------------------------------------
    // Recursively visit the children of the InstantiateNull.
    // ---------------------------------------------------------------------
    setReplacementExpr(getExpr()
                      ->initiateLeftToInnerJoinTransformation(normWARef));

    //----------------------------------------------------------------------
    //10-040116-2480: Mark this InstantiateNull as in transition to go away
    // at the end of Transformation.
    //----------------------------------------------------------------------
    beginLOJTransform_ = TRUE;
  }


  //----------------------------------------------------------------------
  // 10-031023-0723: do not return the replacement expression yet. The
  // transformation gets completed in the normalization. This is causing
  // certain predicates not to be pulled up properly.
  //----------------------------------------------------------------------
  return this;

} // ItemExpr::initiateLeftToInnerJoinTransformation()

//******************************************************
// ItemExpr * InstantiateNull::getReplacementExpr()
// If this instantiate null is going away (Ouetr Join is
// becoming an inner join) then during the transformation
// phase, we want to return the Instantantiate Null operator
// But after transformation phase we want to return the
// actual replacement expression.
//
// Otherwise, certain operators may have
// references to the instantiate null (outer references)
// and they may not know anything
// about replacement expression until the transformation
// is complete.
//******************************************************
ItemExpr * InstantiateNull::getReplacementExpr() const
{
  InstantiateNull * iNull= (InstantiateNull * const) this;

  if (beginLOJTransform_)
    return  iNull;

  ItemExpr* ie=ItemExpr::getReplacementExpr();

  if ((ItemExpr *)iNull == ie)
    return ie;

  while (ie != ie->getReplacementExpr() &&
         ie->getOperatorType() == ITM_INSTANTIATE_NULL)
  {
    ie=ie->getReplacementExpr();
  }

  return ie;
} // InstantiateNull::getReplacementExpr()

//******************************************************
// Soln: 10-060105-3714 
// This is a new overloaded function introduced to 
// remove NESTED Null Instantiates. We Introduce multiple 
// InstantiatedNull Nodes from the binder via 
// ValueId::nullInstantiate(...). Removing this additional
// nodes when they are initially added in the binder 
// resulted in lots of side-effects, so instead of that we 
// had chosen to remove it in normalization, when we are 
// sure that we no longer need them. It is currently 
// (at the time the call was written) used in 
// VEGRegion::replaceInstantiateNullMembers() method. 
//******************************************************
ItemExpr * InstantiateNull::getReplacementExpr(NABoolean forceNavigate) const
{
  InstantiateNull * iNull= (InstantiateNull * const) this;

  if (NOT forceNavigate) 
    return  iNull->getReplacementExpr();

  ItemExpr* ie=ItemExpr::getReplacementExpr();
 
  if ((ItemExpr *)iNull == ie)
    return ie;

  if(ie->getOperatorType() == ITM_INSTANTIATE_NULL)
  {
     iNull = (InstantiateNull * const)ie;
     ie = iNull->getReplacementExpr(TRUE);
  }

  return ie;   
} // InstantiateNull::getReplacementExpr(NABoolean)

ItemExpr * InstantiateNull::normalizeNode(NormWA & normWARef)
{
  // ---------------------------------------------------------------------
  // During the transformation phase of normalization the normalizer
  // can decide to convert a Left Join to an Inner Join. When this
  // happens, each InstantiateNull operator that is produced by such
  // a Left Join must be replaced with the expression for which it will
  // instantiate a null. The normalizer makes the latter expression the
  // "replacement expression" for the InstantiateNull and uses it for
  // replacing the InstantiateNull. The replacementExpr is nominally
  // set to the "this" pointer for an operator.
  // ---------------------------------------------------------------------

#if 0
  // ---------------------------------------------------------------------
  // Try to rewrite null-inst into VEGRef. Unfortunately, there seems to
  // be a lot of code in histogram/costing which relies on null-inst
  // values being not rewritten into VEGRef. So, comment out until more
  // time is allowed to understand and resolve the impact of doing this.
  // ---------------------------------------------------------------------
  if (nodeIsNormalized())
  {
    ItemExpr * vegrefPtr = normWARef.getVEGReference(getValueId());

    // we either got a VEGRef or an ITM_CONSTANT back
    if (vegrefPtr)
      // return without setting the replacement expression
      // the same valueid could map to different VEGRefs in different
      // regions
      return vegrefPtr;
    else
      // return this
      return getReplacementExpr();
  }
#endif

  beginLOJTransform_=FALSE;
  if (nodeIsNormalized())
    return getReplacementExpr();
  markAsNormalized();

  // Indicate that we are processing a complex scalar expression.
  normWARef.setComplexScalarExprFlag();
  child(0) = child(0)->getReplacementExpr()->normalizeNode(normWARef);
  normWARef.restoreComplexScalarExprFlag();

  // Left Join eliminated.
  // Notes by PK, 24MAR00:
  // Calling setReplacementExpr should not be necessary, as the
  // method initiateLeftToInnerJoinTransformation should have
  // already set the replacement expression correctly.
  // This code probably just sets the replacement expr to what
  // it already was. This code should be removed.
  if (getReplacementExpr() != this)
    setReplacementExpr(child(0).getPtr());

#if 0
  // ---------------------------------------------------------------------
  // Try to rewrite null-inst into VEGRef. Unfortunately, there seems to
  // be a lot of code in histogram/costing which relies on null-inst
  // values being not rewritten into VEGRef. So, comment out until more
  // time is allowed to understand and resolve the impact of doing this.
  // ---------------------------------------------------------------------
  else
  {
    ItemExpr * vegrefPtr = normWARef.getVEGReference(getValueId());

    // we either got a VEGRef or an ITM_CONSTANT back
    if (vegrefPtr)
      // return without setting the replacement expression
      // the same valueid could map to different VEGRefs in different
      // regions
      return vegrefPtr;
    else
      // return this
      return getReplacementExpr();
  }
#endif

  return getReplacementExpr();
} // InstantiateNull::normalizeNode()

// ***********************************************************************
// $$$$ BoolVal
// member functions for class BoolVal
// ***********************************************************************

// -----------------------------------------------------------------------
// Perform an MDAM tree walk on BoolVal
// -----------------------------------------------------------------------
DisjunctArray * BoolVal::mdamTreeWalk()
{
  DisjunctArray * disjunctArray = new HEAP DisjunctArray
                                      (new HEAP ValueIdSet(getValueId()));
  return disjunctArray;
} // BoolVal::mdamTreeWalk()


// ***********************************************************************
// $$$$ BiLogic
// member functions for class BiLogic
// ***********************************************************************
ItemExpr * BiLogic::transformMultiValuePredicate(
                                     NABoolean flatten,
                                     ChildCondition condBiRelat)
{
  ItemExpr *t0 = child(0)->transformMultiValuePredicate(flatten, condBiRelat);
  ItemExpr *t1 = child(1)->transformMultiValuePredicate(flatten, condBiRelat);

  if(t0 && ( t0->getOperatorType() == ITM_AND))
  {
    t0->synthTypeAndValueId(TRUE);
    // check for the added prefix predicate
    ItemExpr * leftChildOfAND = (ItemExpr *)t0->child(0);

    if((leftChildOfAND->isARangePredicate()) &&
       ((BiRelat *)leftChildOfAND)->isDerivedFromMCRP())
    {
      BiRelat * addedComparison = (BiRelat *) leftChildOfAND;
      addedComparison->translateListOfComparisonsIntoValueIds();
    }
  }

  if(t1 && ( t1->getOperatorType() == ITM_AND))
  {
    t1->synthTypeAndValueId(TRUE);
    // check for the added prefix predicate
    ItemExpr * leftChildOfAND = (ItemExpr *)t1->child(0);

    if((leftChildOfAND->isARangePredicate()) &&
       ((BiRelat *)leftChildOfAND)->isDerivedFromMCRP())
    {
      BiRelat * addedComparison = (BiRelat *) leftChildOfAND;
      addedComparison->translateListOfComparisonsIntoValueIds();
    }
  }

  // Let caller do any retry necessary
  if (!t0 && !t1) return NULL;

  DBGSETDBG( "TRANSFORM_DEBUG" )
  DBGIF(
    unp = "";
    if (child(0)) child(0)->unparse(unp);
    cerr << "BiLogic c0: " << (Int32)condBiRelat << " " << unp << " " << endl;
    unp = "";
    if (child(1)) child(1)->unparse(unp);
    cerr << "BiLogic c1: " << (Int32)condBiRelat << " " << unp << " " << endl;
    unp = "";
    if (t0) t0->unparse(unp);
    cerr << "BiLogic t0: " << (Int32)condBiRelat << " " << unp << " " << endl;
    unp = "";
    if (t1) t1->unparse(unp);
    cerr << "BiLogic t1: " << (Int32)condBiRelat << " " << unp << " " << endl;
  )

  OperatorTypeEnum op = ITM_AND;

  if (condBiRelat == ANY_CHILD_RAW)
    {
      if (!t0) return t1;
      if (!t1) return t0;
      // If BOTH BiRelats returned a raw-transform of any of their children,
      // fall thru, returning tfm, the explicit AND of these two tfms.
    }
  else
    {
      // Retry any untransformed non-raw children in raw fashion;
      // if there is a raw tfm, then the child is (itself AND raw-tfm).
      if (!t0 && condBiRelat == ANY_CHILD)
        {
           t0 = child(0)->transformMultiValuePredicate(flatten, ANY_CHILD_RAW);
           if (t0) t0 = new HEAP BiLogic(ITM_AND, t0, child(0));
        }
      if (!t0) t0 = child(0);

      if (!t1 && condBiRelat == ANY_CHILD)
        {
           t1 = child(1)->transformMultiValuePredicate(flatten, ANY_CHILD_RAW);
           if (t1) t1 = new HEAP BiLogic(ITM_AND, t1, child(1));
        }
      if (!t1) t1 = child(1);

      op = getOperatorType();
    }

  ItemExpr *tfm = new HEAP BiLogic(op, t0, t1);
  CMPASSERT(tfm);
  return tfm;
}

#pragma nowarn(1506)   // warning elimination
void BiLogic::transformNode(NormWA & normWARef,
                            ExprValueId & locationOfPointerToMe,
                            ExprGroupId & introduceSemiJoinHere,
                            const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  DBGSETDBG( "TRANSFORM_DEBUG" )
  DBGIF(
    unp = "";
    unparse(unp);
    cerr << (Int32)getOperatorType() << " "
         << (Int32)getValueId() << " "
         << (void *)this << " "
         << unp << endl;
  )

  switch(getOperatorType())
    {
    case ITM_AND:
      {
         ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                                 introduceSemiJoinHere, externalInputs);
            // Eliminate the AND from the tree if either of its children
            // have been eliminated.
         if (!child(0).getPtr() && !child(1).getPtr())
           locationOfPointerToMe = NULL;
         else if (!child(0).getPtr())
           locationOfPointerToMe = child(1);
         else if (!child(1).getPtr())
           locationOfPointerToMe = child(0);
         break;
      }
    case ITM_OR:
      {
         normWARef.setOrFlag();
         ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                                 introduceSemiJoinHere, externalInputs);

         normWARef.restoreOrFlag();

         // for the topmost OR, try to find common subexpressions and
         // factor those out
         if (NOT normWARef.haveAnOrAncestor())
           {
             locationOfPointerToMe = applyInverseDistributivityLaw();
             getValueId().replaceItemExpr(locationOfPointerToMe);
             locationOfPointerToMe->synthTypeAndValueId();
             // Make sure the new nodes are transformed (note that
             // unfortunately this may attempt to do the inverse
             // distributivity law transformation a second time)
             locationOfPointerToMe->transformNode(normWARef,
                                                  locationOfPointerToMe,
                                                  introduceSemiJoinHere,
                                                  externalInputs);
           }

         break;
      }
    default:
      ABORT("Internal error in BiLogic::transformNode() - unknown operator");
      break;
    }

    applyTruthTable(this, locationOfPointerToMe);
    setReplacementExpr(locationOfPointerToMe);

} // BiLogic::transformNode()
#pragma warn(1506)  // warning elimination

// -----------------------------------------------------------------------
// Apply De-Morgan's Laws.  Distribute a NOT over each subtree.
// -----------------------------------------------------------------------
ItemExpr * BiLogic::transformSubtreeOfNot(NormWA & normWARef,
                                          OperatorTypeEnum falseOrNot)
{
  ItemExpr * rv = NULL;

  switch(getOperatorType())
    {
    case ITM_AND:
      // NOT(A AND B) ==> NOT A OR NOT B
      rv = new(normWARef.wHeap())
               BiLogic(ITM_OR,
                       new(normWARef.wHeap())
                           UnLogic(falseOrNot, child(0).getPtr()),
                       new(normWARef.wHeap())
                           UnLogic(falseOrNot, child(1).getPtr()));
      break;
    case ITM_OR:
      // NOT(A OR B) ==> NOT A AND NOT B
      rv = new(normWARef.wHeap())
               BiLogic(ITM_AND,
                       new(normWARef.wHeap())
                           UnLogic(falseOrNot, child(0).getPtr()),
                       new(normWARef.wHeap())
                           UnLogic(falseOrNot, child(1).getPtr()));
      break;
    default:
      ABORT("Internal error in BiLogic::InvertNode() unknown op type");
    }
  // Do not synthTypeAndValueId() yet
  return rv;
} // BiLogic::transformSubtreeOfNot()

// -----------------------------------------------------------------------
// predicateEliminatesNullAugmentedRows()
// The following method determines whether a predicate is capable of
// discarding null augmented rows produced by a left join.
// -----------------------------------------------------------------------
NABoolean BiLogic::predicateEliminatesNullAugmentedRows(NormWA & normWARef,
                                               ValueIdSet & outerReferences)
{
  NABoolean returnValue = FALSE;

  // Short circuit the non-null rejecting ones.
  switch(getOperatorType())
  {
    // we could process the OR if we could determine readily that
    // both sides of the operator would be null rejecting
    // at the moment that would mean to take a copy of the tree before
    // allowing the recursion to either child, since the recursion has 
    // sideeffects.
    //
    // Leaving it for another day...

    case ITM_OR:
      return returnValue;
  }

  // -----------------------------------------------------------------
  // An InstantiateNull is used by the LeftJoin for instantiating a
  // null value in place of a value that is produced as output by
  // its second child. An InstantiateNull that appears in a binary
  // comparison predicate eliminates null augmented rows. As a
  // consequence, the LeftJoin becomes an InnerJoin. For example,
  //                     having
  //                  GB ------> A.x < D.x AND A.x > D.y
  //                  |
  //                  LJ
  //                 /  \
  //                A    LJ
  //                    /  \
  //                   B    LJ
  //                       /  \
  //                      C    D
  // The HAVING clause predicate A.x < D.x AND A.x > D.y will cause all the
  // LeftJoin (LJ) operators to be transformed to InnerJoins.
  // -----------------------------------------------------------------
  if (!normWARef.walkingAnExprTree() && !normWARef.subqUnderExprTree())
    {                        // does NOT lie within an expression tree
      // -------------------------------------------------------------
      // Initiate the left to inner join transformation. Replace
      // the InstantiateNull with the original expression that
      // is the subject for null instantiation.
      // -------------------------------------------------------------

    // 
    // NOTE: Use heap in the following lines, not stack.
    //       We are in a recursive method, so we must keep our stack
    //       requirements to a minimum.
    // 
    GroupAttributes * emptyGA   = new (STMTHEAP) GroupAttributes ;
    emptyGA->setCharacteristicOutputs(outerReferences);
    ValueIdSet * emptySet       = new (STMTHEAP) ValueIdSet ;
    ValueIdSet * emptySet1      = new (STMTHEAP) ValueIdSet ;
    ValueIdSet * coveredExpr    = new (STMTHEAP) ValueIdSet ;
    ValueIdSet * coveredSubExpr = new (STMTHEAP) ValueIdSet ;
    ValueIdSet * instNullValue  = new (STMTHEAP) ValueIdSet ;
    NABoolean containsOuterReferencesInSelectList = FALSE;
    for (CollIndex i = 0; i < 2; i++)
    {
      if (child(i)->getOperatorType() == ITM_INSTANTIATE_NULL)
      {
        InstantiateNull *inst = (InstantiateNull *)child(i)->castToItemExpr();
        if ((normWARef.inSelectList())&&!(inst->NoCheckforLeftToInnerJoin))
        {
          instNullValue->insert(inst->getValueId());
          emptyGA->coverTest(
            *instNullValue,
            *emptySet,
            *coveredExpr,
            *emptySet1,
            coveredSubExpr);
          if(!coveredExpr->isEmpty())
          {
            containsOuterReferencesInSelectList = TRUE;
          }
        }
        if ((!inst->NoCheckforLeftToInnerJoin)&&!containsOuterReferencesInSelectList)
        {
          child(i) = child(i)->initiateLeftToInnerJoinTransformation
                                                         (normWARef);
          returnValue = TRUE;
        }
         // we have to dig deeper
      } else if (child(i)->predicateEliminatesNullAugmentedRows
                                         (normWARef, outerReferences) == TRUE)
      {
          returnValue = TRUE;
      }
      containsOuterReferencesInSelectList = FALSE;
      instNullValue->clear();
      coveredExpr->clear();
      coveredSubExpr->clear();
    }
    NADELETE( emptyGA ,   GroupAttributes, STMTHEAP );
    NADELETE( emptySet ,       ValueIdSet, STMTHEAP );
    NADELETE( emptySet1 ,      ValueIdSet, STMTHEAP );
    NADELETE( coveredExpr ,    ValueIdSet, STMTHEAP );
    NADELETE( coveredSubExpr , ValueIdSet, STMTHEAP );
    NADELETE( instNullValue ,  ValueIdSet, STMTHEAP );
  }

  return returnValue;
} // BiLogic::predicateEliminatesNullAugmentedRows()

ItemExpr * BiLogic::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return getReplacementExpr();

  ItemExpr *normalizedExpr = NULL;

  switch(getOperatorType())
    {
    case ITM_AND:
      normalizedExpr = ItemExpr::normalizeNode(normWARef);
      break;

    case ITM_OR:
      {
         normWARef.setOrFlag(); // set flag to indicate in OR subtree
         normalizedExpr = ItemExpr::normalizeNode(normWARef);
         normWARef.restoreOrFlag();
      }
      break;

    default:
      ABORT("Internal error in BiLogic::normalizeNode() - unknown operator");
    }

  return normalizedExpr;

} // BiLogic::normalizeNode()


// MDAMR
// -----------------------------------------------------------------------
// Perform an MDAM tree walk on BiLogic
// -----------------------------------------------------------------------
DisjunctArray * BiLogic::mdamTreeWalk()
{
  // perform the tree walk on the left child
  DisjunctArray *  leftDisjunctArray = child(0)->getReplacementExpr()->mdamTreeWalk();
  //10-040128-2749 -begin
  // If Any of the Disjuncts are NULL then we must have ABORTed disjunct
  // generation at some point of time so we must return. To save time.
  //
  // NOTE : there are currently 4 more mdamTreeWalk call for
  // Veggies,uniLogic,Boolean and BiRelate ItemExprs but these are trivial.
  // Only this one BiLogic is complex.
  if(!leftDisjunctArray)
     return NULL;
  //10-040128-2749 -end

  // perform the tree walk on the right child
  DisjunctArray * rightDisjunctArray = child(1)->getReplacementExpr()->mdamTreeWalk();
  //10-040128-2749 -begin
  if(!rightDisjunctArray)
     return NULL;
  //10-040128-2749 -end

  switch(getOperatorType())                          // what node are we on?
    {
    case ITM_AND:                                    // we are at an AND node
      {
      // -----------------------------------------------------------------
      // The left and right disjunct arrays are ANDed together.  The
      // resulting disjunct array, returned by mdamANDDisjunctArrays, is
      // sent back to the previous ItemExpr as this method recurses back
      // up the expression tree.
      // -----------------------------------------------------------------
        return mdamANDDisjunctArrays( leftDisjunctArray,
                                     rightDisjunctArray );
      }
    case ITM_OR:                                     // we are at an OR node
      {
      // -----------------------------------------------------------------
      // The left and right disjunct arrays are ORed together.  The
      // resulting disjunct array, returned by mdamORDisjunctArrays, is
      // sent back to the previous ItemExpr as this method recurses back
      // up the expression tree.
      // -----------------------------------------------------------------
        return mdamORDisjunctArrays( leftDisjunctArray,
                                    rightDisjunctArray );
      }
    default:
      ABORT("Internal error in BiLogic::mdamTreeWalk() - unknown operator");
      break;
    }

   return 0; // NT_PORT ( bd 1/14/96 ) Eliminate compiler warning
} // BiLogic::mdamTreeWalk()

// MDAMR

// ***********************************************************************
// $$$$ BiRelat
// member functions for class BiRelat
// ***********************************************************************

static NABoolean switchLeftAndRightChildren(ItemExpr* leftChild,
                                            ItemExpr* rightChild)
{
  // Fastpath for expression of the form
  //  <column> <op> <expression>, <expression> op <column>
  if (leftChild->getOperatorType() == ITM_BASECOLUMN ||
      leftChild->getOperatorType() == ITM_INDEXCOLUMN)
    return FALSE;
  else if (rightChild->getOperatorType() == ITM_BASECOLUMN ||
           rightChild->getOperatorType() == ITM_INDEXCOLUMN)
    return TRUE;

  ValueId exprId;
  ValueIdSet leftLeafValues, rightLeafValues;
  NABoolean leftColumnFlag = FALSE;
  NABoolean rightColumnFlag = FALSE; 

  // Gather the ValueIds of all the expressions of arity 0
  leftChild->getLeafValueIds(leftLeafValues);
  rightChild->getLeafValueIds(rightLeafValues);

  // Do the transformation when the left child contains constants only
  // and the right child contains at least one column
  for (exprId = leftLeafValues.init();
       leftLeafValues.next(exprId);
       leftLeafValues.advance(exprId))
    {
      if ((exprId.getItemExpr()->getOperatorType() == ITM_BASECOLUMN) ||
          (exprId.getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN) )
         {
            leftColumnFlag = TRUE;
            break;
         }
    }

  for (exprId = rightLeafValues.init();
       rightLeafValues.next(exprId);
       rightLeafValues.advance(exprId))
    {
      if ( (exprId.getItemExpr()->getOperatorType() == ITM_BASECOLUMN) ||
           (exprId.getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN) )
         {
            rightColumnFlag = TRUE;
            break;
         }
    }

  // if leftColumn contains no base column, and the right child
  // contains even one. Do the transformation
  if (rightColumnFlag && !leftColumnFlag)
    return TRUE;
  else
    return FALSE;

} // switchLeftAndRightChildren()

// -----------------------------------------------------------------------
// Get the operator type for performing the reverse comparison.
// -----------------------------------------------------------------------
OperatorTypeEnum BiRelat::getReverseOperatorType() const
{

  switch(getOperatorType())
    {
    case ITM_GREATER_EQ:
      return ITM_LESS_EQ;
    case ITM_GREATER:
      return ITM_LESS;
    case ITM_LESS:
      return ITM_GREATER;
    case ITM_LESS_EQ:
      return ITM_GREATER_EQ;
    default:
      return getOperatorType();
    }
} // BiRelat::getReverseOperatorType

// -----------------------------------------------------------------------
// Find all subqueries directly contained in the ItemList child(ren)
// of thisIE.  Leaving the subqueries where they are, construct a list of preds
// "((subq) IS NULL) IS NOT UNKNOWN [AND...]".
//
// These predicates will of course always evaluate either to TRUE or
// to CardinalityViolation; this is merely a handy way for us to return
// a valid predicate containing all subqueries up to our caller,
// ItemExpr::convertToValueIdSet().  These preds will then be easily separable
// by removeSubqueryPredicates() in RelExpr::transformSelectPred()
// and the ITM_ONE_ROW aggregation will thus be enforced.
//
// Finally, note that GroupbyAgg::normalizeNode() will end up eliminating
// the "IS NULL IS NOT UNKNOWN" pred from its HAVING clause,
// leaving the ITM_ONE_ROW aggregate as is.
// -----------------------------------------------------------------------
static ItemExpr * dissectOutSubqueries(ItemExpr *thisIE,
                                       NABoolean flattenSubqueries,
                                       NABoolean considerDirectChildSubqueries)
{
  if (CmpCommon::getDefault(COMP_BOOL_137) == DF_OFF)
    return NULL ;

  if (flattenSubqueries) return NULL;   // makes no sense, otherwise

  ItemExpr *newPred = NULL;

  Int32 i = thisIE->getArity();
  for (; i--; )
    if (thisIE->child(i)->getOperatorType() == ITM_ITEM_LIST)
      {
        ItemExprList list(thisIE->child(i).getPtr(), HEAP,ITM_ITEM_LIST,FALSE);
        for (CollIndex i = list.entries(); i--; )
           if (list[i]->isASubquery())
              {
                 ItemExpr *u = new HEAP UnLogic(ITM_IS_NOT_UNKNOWN,
                                       new HEAP UnLogic(ITM_IS_NULL, list[i]));
                 newPred = (!newPred) ? u : new HEAP BiLogic(ITM_AND, u, newPred);
              }
      }
    else if (considerDirectChildSubqueries && thisIE->child(i)->isASubquery())
       {
          ItemExpr *u = new HEAP UnLogic(ITM_IS_NOT_UNKNOWN,
                              new HEAP UnLogic(ITM_IS_NULL, thisIE->child(i)));
          newPred = (!newPred) ? u : new HEAP BiLogic(ITM_AND, u, newPred);
       }

  return newPred;
} // dissectOutSubqueries

// -----------------------------------------------------------------------
// BiRelat::transformMultiValuePredicate()
//
// If the comparison operator is = (equality),
// transform a predicate of the form (A,B) = (C,D)
// to A = C AND B = D.
//
// If the comparison operator is <> (inequality),
// transform a predicate of the form (A,B) <> (C,D)
// to A <> C OR B <> D.
//
// If the comparison operator is < or <= or > or >=,
// transform a predicate of the form (A,B,C) {original-op} (X,Y,Z)
// to:
//    (A {new-op} X) OR
//      (A = X) AND ((B {new-op} Y) OR
//                     (B = Y) AND (C {original-op} Z))
// where the <new-op> is:
//  if original-op is < or <=, new-op is <
//  if original-op is > or >=, new-op is >
//
// MV --
// The optional directionVector parameter is for comparing index columns
// that are not all directed the same. For example if the index is on
// columns (A ASC, B DESC, C ASC), we can use the special syntax (internal
// use only) of: (a, b, c) > (x, y, z) DIRECTEDBY ( ASC, DESC, ASC).
// The direction vector is a list of integers: 1 for ASC, and -1 for DESC.
// The result will be:
//    (A > X) OR
//      (A = X) AND ((B < Y) OR
//                     (B = Y) AND (C > Z))
// -----------------------------------------------------------------------
static ItemExpr * transformMultiValueComparison(BiRelat *thisCmp,
                                                NABoolean flattenSubq,
                                                NABoolean flattenUDF,
                                                IntegerList *directionVector)
{
  // Convert the child subtrees into lists.
  ItemExprList lhs(thisCmp->child(0).getPtr(), HEAP,ITM_ITEM_LIST, 
                   flattenSubq, flattenUDF);
  ItemExprList rhs(thisCmp->child(1).getPtr(), HEAP,ITM_ITEM_LIST, 
                   flattenSubq, flattenUDF);

  if (lhs.entries() != rhs.entries())
    {
      if (!flattenSubq) return NULL;
      CMPASSERT(lhs.entries() == rhs.entries());
    }

  if ( lhs.entries() == 0 && rhs.entries() == 0)
  {
    return NULL; // this is possible in certain kinds of aggregate transformations
                 // For exampl OneRow aggregate
  }

#pragma nowarn(1506)   // warning elimination
  Int32 i = lhs.entries() - 1;
#pragma warn(1506)  // warning elimination

  // As an extension to Ansi, we allow predicates like
  //   select x,y from xy where((select a,b from t),x) = (y,(select m,n from s))
  // which obviously cannot be transformed when not flattening subqueries
  // (i.e. when called from convertToValueIdSet).
  //
  if (!flattenSubq)
    {
      CollIndex ll, rr;
      while (i--)
      {
        ll = rr = 1;       // non-subq's are single items, no lists
        if (lhs[i]->isASubquery())
          ll = ((Subquery *)lhs[i])->getSubquery()->getDegree();
        if (rhs[i]->isASubquery())
          rr = ((Subquery *)rhs[i])->getSubquery()->getDegree();
        if (ll != rr) return NULL;
      }
#pragma nowarn(1506)   // warning elimination
      i = lhs.entries() - 1;      // reset for loops following
#pragma warn(1506)  // warning elimination
    }

  ItemExpr * tfm;
  NABoolean special = thisCmp->getSpecialNulls();

  if (thisCmp->getOperatorType() == ITM_EQUAL)
    {
      tfm = new HEAP BiRelat(ITM_EQUAL, lhs[i], rhs[i], special);

      while (i--)
         tfm = new HEAP BiLogic(ITM_AND,
                                new HEAP
                                   BiRelat(ITM_EQUAL, lhs[i], rhs[i], special),
                                tfm
                               );
    }
  else
    {
      // convert '<=' to '<' and '>=' to '>'; leave '<', '>', '<>' as is.
      OperatorTypeEnum origOp, newOp, origRevOp, newRevOp, thisOp;
      origOp = newOp = thisCmp->getOperatorType();
      origRevOp = newRevOp = thisCmp->getReverseOperatorType();
      if (newOp == ITM_LESS_EQ)
         newOp = ITM_LESS;
      else if (newOp == ITM_GREATER_EQ)
         newOp = ITM_GREATER;
      if (newRevOp == ITM_LESS_EQ)
         newRevOp = ITM_LESS;
      else if (newRevOp == ITM_GREATER_EQ)
         newRevOp = ITM_GREATER;

      // An ItemExprList to capture the comparison for each column
      // Consider predicate (a, b) > (1, 2)
      // The above predicate will be transformed to (a > 1) or (a = 1 and b > 2)
      // The list of comparisons in this case will be have
      // 1. '>' ItemExpr from a > 1
      // 2. '>' ItemExpr from b > 2
      //
      // Consider predicate (a, b) > (1, 2) DIRECTED BY (ASC, DESC)
      // The above predicate will be transformed to (a > 1) or (a = 1 and b < 2)
      // The list of comparisons in this case will be have
      // 1. '>' ItemExpr from a > 1
      // 2. '<' ItemExpr from b < 2
      //
      // This list of comparisons is later used in BiRelat::transformNode
      // to get:
      // 1. list of the left children of (a, b) > (1, 2),
      //    The list will have the ValueIds
      //    1. VEG containing column A
      //    2. VEG containing column B
      // 2. list of the right Children of (a, b) > (1, 2),
      //    The list will have the ValueIds
      //    1. VEG containing literal 1
      //    2. VEG containing literal 2
      //
      // In addition the list of comparisons is also used to match key column
      // clustering order (i.e. asc, desc) to the type of comparison (i.e. >, <).
      //
      // This information is used in the scan optimizer to figure out if
      // a multicolumn range predicate can be used to specify a begin/end key
      // for a given access path
      ItemExprList * listOfComparisons = new HEAP ItemExprList(HEAP);

      // If the direction is DESC flip the sign.
      thisOp = origOp;
      if (directionVector!=NULL && directionVector->at(i)==-1)
        thisOp = origRevOp;
      tfm = new HEAP BiRelat(thisOp, lhs[i], rhs[i], special);

      // add the first comparison
      listOfComparisons->insertAt(0,tfm);

      while (i--)         // we MUST build THESE preds BACKWARDS
      {
          // If the direction is DESC flip the sign.
        thisOp = newOp;
        if (directionVector!=NULL && directionVector->at(i)==-1)
          thisOp = newRevOp;
        BiRelat * b1 = new HEAP BiRelat(thisOp, lhs[i], rhs[i], special);

           // add comparison to list of comparisons
        listOfComparisons->insertAt(0,b1);

        if (newOp != ITM_NOT_EQUAL)
          tfm = new HEAP BiLogic(ITM_AND,
                                 new HEAP
                                    BiRelat(ITM_EQUAL, lhs[i], rhs[i], special),
                                 tfm);

        tfm = new HEAP BiLogic(ITM_OR, b1, tfm);
      }

  // add an extra single column range predicate, the predicate is implied by
  // the MCRP. Consider MCRP (a, b) > (1, 2), this predicate also implies
  // predicate (a >= 1), therefore the MCRP is transformed to
  // (a >= 1) and (a, b) > (1, 2).
  // Since (a, b) > (1, 2) has already been transformed to
  // (a > 1) or (a = 1 and b > 2), the transformed predicate looks like
  // ((a >= 1) and ((a > 1) or (a = 1 and b > 2))
  // the extra predicate helps the single subset scan optimizer choose
  // a begin/end key based on the MCRP (a, b) > (1, 2).
  // If this is not done the whole table, since the single subset scan optimizer
  // cannot deal with disjunct i.e. ORs and (a, b) > (1, 2) is transformed
  // into an OR.
  if ((tfm->getOperatorType() == ITM_OR) && (origOp != ITM_NOT_EQUAL))
  {
    BiRelat * prefixComparison = (BiRelat *) (*listOfComparisons)[0];
    OperatorTypeEnum prefixCompOper = prefixComparison->getOperatorType();

    if(prefixCompOper == ITM_GREATER)
      prefixCompOper = ITM_GREATER_EQ;

    if(prefixCompOper == ITM_LESS)
      prefixCompOper = ITM_LESS_EQ;

    ItemExpr * compLeftChild = (ItemExpr *) prefixComparison->child(0);
    ItemExpr * compRightChild = (ItemExpr *) prefixComparison->child(1);

    BiRelat * addedComparison = new HEAP BiRelat(prefixCompOper,
                                            compLeftChild,
                                            compRightChild,
                                            prefixComparison->getSpecialNulls());

  if (thisCmp->isPreferredForSubsetScanKey())
    addedComparison->setPreferForSubsetScanKey();

    addedComparison->setDerivedFromMCRP();
    addedComparison->setListOfComparisons(listOfComparisons);

    tfm = new HEAP BiLogic(ITM_AND,
                           addedComparison,
                           tfm);
  }

    }

  // Do not synthTypeAndValueId() yet
  CMPASSERT(tfm);
  return tfm;

} // transformMultiValueComparison()

ItemExpr * BiRelat::transformMultiValuePredicate(
                                NABoolean flattenSubqueries,  // default TRUE
                                ChildCondition tfmIf)         // ANY_CHILD

{
  NABoolean tfmNeeded;
  NABoolean flattenUDFs( flattenSubqueries ); // we use the same rules..

  // MV --
  // Handle the direction vector correctly.
  if (directionVector_ != NULL)
    tfmNeeded = TRUE;
  else if (tfmIf == ANY_CHILD)
    tfmNeeded = (child(0)->getOperatorType() == ITM_ITEM_LIST ||
                 child(1)->getOperatorType() == ITM_ITEM_LIST) ||
                (child(0)->getOperatorType() == ITM_ONE_ROW ||
                 child(1)->getOperatorType() == ITM_ONE_ROW);

  else if (tfmIf == ALL_CHILDREN)
    tfmNeeded = child(0)->getOperatorType() == ITM_ITEM_LIST &&
                child(1)->getOperatorType() == ITM_ITEM_LIST;

  else
    tfmNeeded = TRUE;

  if (!tfmNeeded) return NULL;          // no transform needed

  ItemExpr *tfm;
  if (tfmIf == ANY_CHILD_RAW)
    tfm = dissectOutSubqueries(this, flattenSubqueries, TRUE);
  else
    tfm = transformMultiValueComparison(this, flattenSubqueries, 
                                        flattenUDFs, directionVector_);

  return tfm;
} // BiRelat::transformMultiValuePredicate()

#pragma nowarn(1506)   // warning elimination
void BiRelat::transformNode(NormWA & normWARef,
                            ExprValueId & locationOfPointerToMe,
                            ExprGroupId & introduceSemiJoinHere,
                            const ValueIdSet & externalInputs)
{

  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  DBGSETDBG( "TRANSFORM_DEBUG" )
  DBGIF(
    unp = "";
    unparse(unp);
    cerr << (Int32)getOperatorType() << " "
         << (Int32)getValueId() << " "
         << (void *)this << " "
         << unp << endl;
  )

  // ---------------------------------------------------------------------
  // Transform multi-value equality predicates.
  // Special transformation (i.e., done only if both children)
  // is enabled only for QuantifiedComp-generated BiRelat's.
  // ---------------------------------------------------------------------
  ChildCondition tfmIf =
    specialMultiValuePredicateTransformation() ? ALL_CHILDREN : ANY_CHILD;
  ItemExpr * transformedMultiValue = transformMultiValuePredicate(FALSE, tfmIf);

  if (transformedMultiValue)
    {
      // replace the definition of this valueId
      getValueId().replaceItemExpr(transformedMultiValue);
      locationOfPointerToMe = transformedMultiValue;
      locationOfPointerToMe->synthTypeAndValueId(TRUE);

      if( transformedMultiValue->getOperatorType() == ITM_AND)
      {
        // check for the added prefix predicate
        ItemExpr * leftChildOfAND = (ItemExpr *)transformedMultiValue->child(0);

        if((leftChildOfAND->isARangePredicate()) &&
           ((BiRelat *)leftChildOfAND)->isDerivedFromMCRP())
        {
          BiRelat * addedComparison = (BiRelat *) leftChildOfAND;
          addedComparison->translateListOfComparisonsIntoValueIds();
        }
      }

      // -----------------------------------------------------------------
      // Transform the transformed tree.
      // -----------------------------------------------------------------
      locationOfPointerToMe->transformNode
                                  (normWARef, locationOfPointerToMe,
                                   introduceSemiJoinHere, externalInputs);
    }

  // ---------------------------------------------------------------------
  // Switch the left and the right subtrees if the latter contains
  // an expression that only contains BaseColumns at its leaves.
  // ---------------------------------------------------------------------
  else if (switchLeftAndRightChildren(child(0).getPtr(), child(1).getPtr()))
    {
      // -----------------------------------------------------------------
      // Replace me with my inverse comparison.
      // a) >  with <
      // b) >= with <=
      // c) <  with >
      // d) <= with =>
      // e) = and <> remain unchanged.
      // -----------------------------------------------------------------
      BiRelat * flipflop = new(normWARef.wHeap())
                               BiRelat(getReverseOperatorType(),
                                       child(1).getPtr(),
                                       child(0).getPtr());

      flipflop->specialMultiValuePredicateTransformation() =
                                  specialMultiValuePredicateTransformation();
      //++MV - Irena
      flipflop->setSpecialNulls(getSpecialNulls());
      //--MV - Irena

      // replace the definition of this valueId
      getValueId().replaceItemExpr(flipflop);
      locationOfPointerToMe->synthTypeAndValueId(TRUE);
      locationOfPointerToMe = flipflop;
      // -----------------------------------------------------------------
      // Transform the transformed tree.
      // -----------------------------------------------------------------
      locationOfPointerToMe->transformNode
                                  (normWARef, locationOfPointerToMe,
                                   introduceSemiJoinHere, externalInputs);
    }
  else
    {
      // -----------------------------------------------------------------
      // Transform the left and right children.
      // -----------------------------------------------------------------
      ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                              introduceSemiJoinHere, externalInputs);

      // -----------------------------------------------------------------
      // Allocate a VEG that contains my left and right subtrees
      // provided this equality predicate is not encountered
      // while performing a tree walk in a scalar expression,
      // such as in a predicate tree rooted in an OR or in a CASE statement.
      //
      // NOTE:  added  nov 27, 95
      // The support for the generation of a VEGs for an equality
      // predicate that appear in a subtree of an OR or an IS NULL/
      // IS UNKNOWN is already in place. It is also easy to enable below.
      // However, the coverage test for a VEGPredicate permits it to
      // be pushed down even when, in fact, only one member of the VEG is
      // available. This property gives rise to a few interesting
      // problems when this support is enabled. For example,
      // 1) The normalization of the predicate isnull(anyTrue(x=y)), which
      //    is introduced by a subquery transformation, causes it to
      //    be transformed to isnull(anyTrue(VEGPredicate(x,y))). The
      //    presence of the VEGPredicate causes an internal error to be
      //    issued by FileScan::computeRetrievedColumns(). (I haven't
      //    debugged the cause of this problem      nov 27, 95)
      // 2) The predicate for T1 IJ T2 on T1.x = 10 or T2.y = 20 is
      //    normalized to VEGPredicate(T1.x,10) OR VEGPredicate(T2.x,20).
      //    Predicate pushdown causes each of T1 and T2 to compute the
      //    subexpressions VEGPredicate(T1.x,10) OR VEGPredicate(T2.x,20)
      //    respectively and eliminates the OR predicate from the join.
      //    The OR predicate should not be eliminated from the join
      //    because that changes the semantics of such queries.
      //    However, I do not have a solution for this problem yet.
      //
      // In order to enable the creation of a VEG for predicates
      // underneath an OR, replace the code:
      //    !normWARef.walkingAnExprTree()
      // with
      //    (!normWARef.walkingAnExprTree() ||
      //     (normWARef.haveAnOrAncestor() &&
      //      !normWARef.inAComplexScalarExpr()))
      //
      // -----------------------------------------------------------------
      if (getOperatorType() == ITM_EQUAL &&
          !normWARef.walkingAnExprTree())
      {

        // Do constant folding here,
        // This will result in more VEGs being created
        // because expressions will be folded in constant
        // and they will be considered user input.
        // e.g. predicate x = 1 + 3 will create a VEG
        // between (x,4) because now expression 1 + 3
        // will be folded into ITM_CONSTANT 4 which is considered
        // a user input

        // Do not attempt folding if one of the operands
        // of the ITM_EQUAL is a ITM_ITEM_LIST since constant
        // folding has a problem with that. It crashes
        // in the generator.

        if((child(0)->getOperatorType() != ITM_ITEM_LIST) )
        {
          // Get ValueIdList to invoke constant folding
          ValueIdList child0;
          child0.insert(child(0)->getValueId());

          //attempt constant folding
          child0.constantFolding();

          if(!child0.isEmpty())
            //set child to be the folded expressions
            setChild(0, child0[0].getItemExpr());
        }

        if((child(1)->getOperatorType() != ITM_ITEM_LIST))
        {
          ValueIdList child1;
          child1.insert(child(1)->getValueId());

          //attempt constant folding
          child1.constantFolding();

          if(!child1.isEmpty())
            //set child to be the folded expressions
            setChild(1, child1[0].getItemExpr());
        }

   if (bothAreColumnsOrUserInput(normWARef, child(0), child(1)))
   {
     // (fixes Genesis Case: 10-980827-2993)
     // To fix the problem arising from a VEG with a single column in
     // it, from now on we unconditionally transform {T1.A=T1.A -->
     // T1.A IS NOT NULL}
     // --------------
     // MVs:
     // When specialNulls_ flag is set, nulls are observed as values
     // and so they are also in scope (null=null).
     // That is why we don't want ITM_IS_NOT_NULL to be turned on
     // in that case.
     // Irena
          if  (   child(0)->getValueId() == child(1)->getValueId()
                  && NOT getSpecialNulls()  // ++MV - Irena
               )
          {
            // the check for whether T1.A is nullable is done inside
            // UnLogic::transformIsNull(), which gets called by
            // UnLogic::transformNode()
            UnLogic * newNode =
              new (normWARef.wHeap()) UnLogic (ITM_IS_NOT_NULL, child(0)) ;

            getValueId().replaceItemExpr(newNode);
            locationOfPointerToMe->synthTypeAndValueId(TRUE);
            locationOfPointerToMe = newNode;
            // -----------------------------------------------------------------
            // Transform the transformed tree.
            // -----------------------------------------------------------------
            locationOfPointerToMe->transformNode
              (normWARef, locationOfPointerToMe,
               introduceSemiJoinHere, externalInputs);
          }
          else
          {
            // QSTUFF
            // We don't push predicates below a generic update root.
            // Vegpreds for equality predicates are "implicitly" pushed
            // down due the the veg logic.  By not allowing vegpreds in case
            // of a genericupdate root we can retain those predicates.
            // The characteristics of the ITM_EQUAL BiRelat that must be
            // retained are that either 1) both children are column refs to
            // the same table or index, or 2) either child is a user-input.
            NABoolean putInVeg = TRUE;
            if ( normWARef.isInEmbeddedUpdateOrDelete() ||
                 normWARef.isInEmbeddedInsert()) {
              if (child(0)->isAColumnReference() &&
                  child(1)->isAColumnReference()) {

                const NATable *table0 = NULL;
                if(child(0)->getOperatorType() == ITM_BASECOLUMN) {
                  table0=((BaseColumn*)(ItemExpr *)child(0))->getTableDesc()->getNATable();
                }
                else if(child(0)->getOperatorType() == ITM_INDEXCOLUMN) {
                  table0=((IndexColumn*)(ItemExpr *)child(0))->getNAColumn()->getNATable();
                }

                const NATable *table1 = NULL;
                if(child(1)->getOperatorType() == ITM_BASECOLUMN) {
                  table1=((BaseColumn*)(ItemExpr *)child(1))->getTableDesc()->getNATable();
                }
                else if(child(1)->getOperatorType() == ITM_INDEXCOLUMN) {
                  table1=((IndexColumn*)(ItemExpr *)child(1))->getNAColumn()->getNATable();
                }

                if( (table0 == table1) &&
                (table0 != NULL) ){
                  // It's not valid to say table0 is the same as table1, unless
                  // both have been set to non-null.  However, there is no
                  // reason to test nullness of both, since if table0 is
                  // not null, and table1 == table0, then table1 is not null
                  // either.  The following assertion is my guarantee.
                  CMPASSERT( table1 != NULL);
                  putInVeg = FALSE;
                  }
                }
                else if ( child(0)->isAUserSuppliedInput() ||
                          child(1)->isAUserSuppliedInput() ) {
                  putInVeg = FALSE;
                }
            }
            //case 10-060531-0086/10-060714-4662 Soln 10-060531-6865 Begin
            //VEG between varchar and char might give wrong result based
            //on the data they have when columns are substituted elsewhere say
            //in select list.
            //For ex:create table tab(c1 char(5), c2 varchar(5));
            //       insert into tab('aaa  ','aaa');
            //       select c2 || c1 from tab where c1 = c2;
            // incorrect result : aaa  aaa
            // Without VEG      : aaaaaa
            // Incorrect result occurs when c1 is chosen out of the VEG which is
            // fixed character variable and it is substituted in the select list.
            // Though we would have cast node to convert it to varchar, but it 
            // would have left trailing spaces.The ANSI semantics states that when 
            // a cast node is converting a fixed char to varchar it retains the maximum 
            // length of characters of varchar variable and the rest are truncated.
            // This is also seen between to varchar variables.
            
            const NAType &type1 = child(0)->getValueId().getType();
            const NAType &type2 = child(1)->getValueId().getType();

            if (type1.getTypeQualifier() == NA_CHARACTER_TYPE &&
                type2.getTypeQualifier() == NA_CHARACTER_TYPE)
               {
                  if((NOT DFS2REC::isAnyVarChar(type1.getFSDatatype())
                      &&  DFS2REC::isAnyVarChar(type2.getFSDatatype())) ||
                      (DFS2REC::isAnyVarChar(type1.getFSDatatype()) &&
                       NOT DFS2REC::isAnyVarChar(type2.getFSDatatype())) ||
                      (DFS2REC::isAnyVarChar(type1.getFSDatatype()) &&
                        DFS2REC::isAnyVarChar(type2.getFSDatatype()))) 
                  {
                     if (CmpCommon::getDefault(COMP_BOOL_158) == DF_OFF)
                        putInVeg = FALSE;
                  }
               }

            //case 10-060531-0086/10-060714-4662 Soln 10-060531-6865 End

            // Solution 10-080923-6039
            // If we have an expression like T0.I1=T0.I2 and both
            // T0.I1 and T0.I2 are inputs, do not establish a VEG as
            // that would have been done above us in the tree if it was
            // necessary.

            if (externalInputs.contains(child(0)->getValueId()) &&
                externalInputs.contains(child(1)->getValueId()))
            {
               putInVeg = FALSE;
            }

            // Part of fix to bugzilla cases 3405, 3408, 3409.
            // For an UPSERT of the form:
            //   merge into T on c1=c2
            //   when matched then update ... where c2=2
            //   when not matched then insert ...;
            // we cannot safely allow a VEG to form for "c2=2"
            // because it applies only to the "when matched" clause.
            // Otherwise, the VEG "c2=2=c1" would incorrectly try to
            // do "insert ..." when "c2=2=c1" is false. 
            // The correct behavior here is to do "insert ..." 
            // only when "c1=c2" is false.
            if (putInVeg && normWARef.inMergeUpdWhere()) 
            {
              putInVeg = FALSE;
            }

            if (putInVeg) 
	        {
              normWARef.addVEG(child(0)->getValueId(),child(1)->getValueId());

              // if the column is nullable and part of a clustering key then that
              // column VEG was set as special nulls so the equality predicate
              // of the base and index column returns NULL equal NULL as TRUE
              // during an index join.
              // However if that column is part of an user predicate then there is
              // no need to set this flag as we do not return NULL values
              // anyway so resetting the special nulls falg if the column is part
              // of a user predicate

              ItemExpr * vegrefPtr = normWARef.getVEGReference(child(0)->getValueId());
              if (vegrefPtr)
              {
                ((VEGReference *)vegrefPtr)->getVEG()->setSpecialNulls(FALSE);

                if(isSelectivitySetUsingHint())
                {
                  VEGPredicate *vegPred = ((VEGReference *)vegrefPtr)->getVEG()->getVEGPredicate();
                  vegPred->getPredsWithSelectivities().insert(getValueId());
                  vegPred->setSelectivitySetUsingHint();
                }
              }
            }
          }    // if child(0)->getValueId() == child(1)->getValueId() ... else ...
        }  // if bothAreColumnsOrUserInput
      } // if getOperatorType() == ITM_EQUAL && !normWARef.walkingAnExprTree())
    }  // if transformedMultiValue ... elseif switchLeftAndRightChildren ... else ....

    markAsTransformed();
    // Set the replacement expression
    setReplacementExpr(locationOfPointerToMe);

} // BiRelat::transformNode()
#pragma warn(1506)  // warning elimination

// -----------------------------------------------------------------------
// predicateEliminatesNullAugmentedRows()
// The following method determines whether a predicate is capable of
// discarding null augmented rows produced by a left join.
// -----------------------------------------------------------------------
NABoolean BiRelat::predicateEliminatesNullAugmentedRows(NormWA & normWARef,
                                              ValueIdSet & outerReferences)
{
  NABoolean returnValue = FALSE;
  // -----------------------------------------------------------------
  // An InstantiateNull is used by the LeftJoin for instantiating a
  // null value in place of a value that is produced as output by
  // its second child. An InstantiateNull that appears in a binary
  // comparison predicate eliminates null augmented rows. As a
  // consequence, the LeftJoin becomes an InnerJoin. For example,
  //                     having
  //                  GB ------> A.x < D.x
  //                  |
  //                  LJ
  //                 /  \
  //                A    LJ
  //                    /  \
  //                   B    LJ
  //                       /  \
  //                      C    D
  // The HAVING clause predicate A.x < D.x will cause all the
  // LeftJoin (LJ) operators to be transformed to InnerJoins.
  // -----------------------------------------------------------------
  if (!normWARef.walkingAnExprTree() && !normWARef.subqUnderExprTree())
    {                        // does NOT lie within an expression tree
      // -------------------------------------------------------------
      // Initiate the left to inner join transformation. Replace
      // the InstantiateNull with the original expression that
      // is the subject for null instantiation.
      // -------------------------------------------------------------
    GroupAttributes emptyGA;
    emptyGA.setCharacteristicOutputs(outerReferences);
    ValueIdSet emptySet, emptySet1, coveredExpr, coveredSubExpr;
    ValueIdSet instNullValue;
    NABoolean containsOuterReferencesInSelectList = FALSE;
    for (CollIndex i = 0; i < 2; i++)
    {
      if (child(i)->getOperatorType() == ITM_INSTANTIATE_NULL)
      {
        InstantiateNull *inst = (InstantiateNull *)child(i)->castToItemExpr();
        if ((normWARef.inSelectList())&&!(inst->NoCheckforLeftToInnerJoin))
        {
          instNullValue.insert(inst->getValueId());
          emptyGA.coverTest(
            instNullValue,
            emptySet,
            coveredExpr,
            emptySet1,
            &coveredSubExpr);
          if(!coveredExpr.isEmpty())
          {
            containsOuterReferencesInSelectList = TRUE;
          }
        }
        if ((!inst->NoCheckforLeftToInnerJoin)&&!containsOuterReferencesInSelectList)
        {
          child(i) = child(i)->initiateLeftToInnerJoinTransformation
                                                         (normWARef);
          returnValue = TRUE;
        }
      }
      containsOuterReferencesInSelectList = FALSE;
      instNullValue.clear();
      coveredExpr.clear();
      coveredSubExpr.clear();
    }
  }

  return returnValue;
} // BiRelat::predicateEliminatesNullAugmentedRows()

ItemExpr * BiRelat::normalizeNode(NormWA & normWARef)
{
  ItemExpr * normalizedExpr = 0;

  // ---------------------------------------------------------------------
  // NOTE:  added  nov 27, 95
  // The support for the generation of a VEGs for an equality
  // predicate that appear in a subtree of an OR or an IS NULL/
  // IS UNKNOWN is already in place. It is also easy to enable below.
  // However, the coverage test for a VEGPredicate permits it to
  // be pushed down even when, in fact, only one member of the VEG is
  // available. This property gives rise to a few interesting
  // problems when this support is enabled. For example,
  // 1) The normalization of the predicate isnull(anyTrue(x=y)), which
  //    is introduced by a subquery transformation, causes it to
  //    be transformed to isnull(anyTrue(VEGPredicate(x,y))). The
  //    presence of the VEGPredicate causes an internal error to be
  //    issued by FileScan::computeRetrievedColumns(). (I haven't
  //    debugged the cause of this problem      nov 27, 95)
  // 2) The predicate for T1 IJ T2 on T1.x = 10 or T2.y = 20 is
  //    normalized to VEGPredicate(T1.x,10) OR VEGPredicate(T2.x,20).
  //    Predicate pushdown causes each of T1 and T2 to compute the
  //    subexpressions VEGPredicate(T1.x,10) OR VEGPredicate(T2.x,20)
  //    respectively and eliminates the OR predicate from the join.
  //    The OR predicate should not be eliminated from the join
  //    because that changes the semantics of such queries.
  //    However, I do not have a solution for this problem yet.
  //
  // In order to enable the replacement of a VEG for predicates
  // underneath an OR, replace the code:
  //    !normWARef.walkingAnExprTree()
  // with
  //    (!normWARef.walkingAnExprTree() ||
  //     (normWARef.haveAnOrAncestor() &&
  //      !normWARef.inAComplexScalarExpr()))
  //
  // -----------------------------------------------------------------
  if (getOperatorType() == ITM_EQUAL &&
      !normWARef.walkingAnExprTree() &&
      bothAreColumnsOrUserInput(normWARef, child(0), child(1)))
    {

      if (!( child(0)->getValueId() == child(1)->getValueId() ))
         {

         ItemExpr * vegRef0Ptr = normWARef.getVEGReference
                                                 (child(0)->getValueId());
         ItemExpr * vegRef1Ptr = normWARef.getVEGReference
                                                 (child(1)->getValueId());
         if (vegRef0Ptr && vegRef0Ptr == vegRef1Ptr)
            {
               normalizedExpr = normWARef.performTC(child(0)->getValueId());
            }
         //--------------------------------------------------------------------
         // Case-10-040630-8369
         // If performTC returns null just call ItemExpr::normalizeNode(normWARef) and
         // continue.
         //--------------------------------------------------------------------
         if (normalizedExpr == 0)
            {
               normalizedExpr = ItemExpr::normalizeNode(normWARef);
            }
         }
      else
         {

         //--------------------------------------------------------------------
         // Genesis case: 10-001006-2811:
         // We would like to transform T1.A = T1.A to T1.A is not NULL in all
         // cases. This is normally done in transformation, but in some cases
         // constant folding is done after this ItemExpression is transformed
         // In those cases, we do that transformation in Normalizer
         //--------------------------------------------------------------------

           UnLogic * newNode =
                new (normWARef.wHeap()) UnLogic (ITM_IS_NOT_NULL, child(0)) ;

           getValueId().replaceItemExpr(newNode);
           newNode->synthTypeAndValueId(TRUE);

           return newNode->normalizeNode(normWARef);
         }
      }
  else
    {
      normalizedExpr = ItemExpr::normalizeNode(normWARef);
    }
  //++MV - Irena
  ((VEGPredicate*)normalizedExpr)->setSpecialNulls(getSpecialNulls());
  //--MV - Irena
  return normalizedExpr;

} // BiRelat::normalizeNode()

// -----------------------------------------------------------------------
// Invert the comparison type when the operator is the child of a NOT.
// -----------------------------------------------------------------------
ItemExpr * BiRelat::transformSubtreeOfNot(NormWA & normWARef,
                                          OperatorTypeEnum falseOrNot)
{
  if (falseOrNot != ITM_NOT)
    return this;

  OperatorTypeEnum otype = NO_OPERATOR_TYPE;

  switch(getOperatorType())
    {
    case ITM_GREATER:
      otype = ITM_LESS_EQ;
      break;
    case ITM_GREATER_EQ:
      otype = ITM_LESS;
      break;
    case ITM_EQUAL:
      otype = ITM_NOT_EQUAL;
      break;
    case ITM_NOT_EQUAL:
      otype = ITM_EQUAL;
      break;
    case ITM_LESS_EQ:
      otype = ITM_GREATER;
      break;
    case ITM_LESS:
      otype = ITM_GREATER_EQ;
      break;
    default:
      ABORT("Internal error in BiRelat::transformSubtreeOfNot() unknown comparison operator");
      break;
    }

  BiRelat * newComp = new(normWARef.wHeap())
                          BiRelat(otype, child(0).getPtr(), child(1).getPtr());

  newComp->setDirectionVector(directionVector_);

  newComp->specialMultiValuePredicateTransformation() =
               specialMultiValuePredicateTransformation();

  
  //++MV - Irena
  newComp->setSpecialNulls(getSpecialNulls());
  //--MV - Irena

  // If the original BiRelat expression was a derivative of LIKE pred
  // then copy the LkeParentId and the Selectivity assigned to the
  // original expression to the new BiRelat expression too

  if (derivativeOfLike() )
  {
    newComp->setLikeSelectivity(1 - getLikeSelectivity() );
    newComp->setOriginalLikeExprId(getValueId() );
  }

  // Do not synthTypeAndValueId() yet
  return newComp;
} // BiRelat::transformSubtreeOfNot()

// MDAMR
// -----------------------------------------------------------------------
// Perform an MDAM tree walk on BiRelat
// -----------------------------------------------------------------------
DisjunctArray * BiRelat::mdamTreeWalk()
{
  // ---------------------------------------------------------------------
  // First a DisjunctArray is allocated.  Then a ValueIdSet is allocated
  // with the value id of this predicate added to the set.  The pointer
  // to this ValueIdSet is then inserted as the first entry of the array.
  // The pointer to the DisjunctArray is returned to the previous BiLogic
  // node as this method recurses back up the expression tree.
  // ---------------------------------------------------------------------
  return new HEAP DisjunctArray(new HEAP ValueIdSet(getValueId()));

} // BiRelat::mdamTreeWalk()
//MDAMR


// -----------------------------------------------------------------------
// predicateEliminatesNullAugmentedRows()
// The following method determines whether a predicate is capable of
// discarding null augmented rows produced by a left join.
// -----------------------------------------------------------------------
NABoolean PatternMatchingFunction::predicateEliminatesNullAugmentedRows(NormWA & normWARef,
                                            ValueIdSet & outerReferences)
{
  NABoolean returnValue = FALSE;
  // -----------------------------------------------------------------
  // An InstantiateNull is used by the LeftJoin for instantiating a
  // null value in place of a value that is produced as output by
  // its second child. An InstantiateNull that appears in a binary
  // comparison predicate eliminates null augmented rows. As a
  // consequence, the LeftJoin becomes an InnerJoin. For example,
  //
  // select t1.x
  // from t1 left join t2 on t1.x = t2.x
  // where t2.x like '_B%';


  //                   SelPred:
  //                   ------> T2.x Like 'Hello%'
  //                  |
  //                  LJ JoinPred: t1.x = t2.x
  //                 /  \
  //                T1    T2
  // The Selection predicate T2.x LIKE 'Hello%' will cause all the
  // LeftJoin (LJ) operators to be transformed to InnerJoins.
  // -----------------------------------------------------------------
  if (!normWARef.walkingAnExprTree() && !normWARef.subqUnderExprTree())
    {                        // does NOT lie within an expression tree
      // -------------------------------------------------------------
      // Initiate the left to inner join transformation. Replace
      // the InstantiateNull with the original expression that
      // is the subject for null instantiation.
      // 
      // We only need to check child 0 of the Like function.
      // -------------------------------------------------------------

    GroupAttributes emptyGA;
    emptyGA.setCharacteristicOutputs(outerReferences);
    ValueIdSet emptySet, emptySet1, coveredExpr, coveredSubExpr;
    ValueIdSet instNullValue;
    NABoolean containsOuterReferencesInSelectList = FALSE;
    if (child(0)->getOperatorType() == ITM_INSTANTIATE_NULL)
     {
       InstantiateNull *inst = (InstantiateNull *)child(0)->castToItemExpr();
       if ((normWARef.inSelectList())&&!(inst->NoCheckforLeftToInnerJoin))
       {
         instNullValue.insert(inst->getValueId());
         emptyGA.coverTest(
           instNullValue,
           emptySet,
           coveredExpr,
           emptySet1,
           &coveredSubExpr);
         if(!coveredExpr.isEmpty())
         {
           containsOuterReferencesInSelectList = TRUE;
         }
       }
       if ((!inst->NoCheckforLeftToInnerJoin)&&!containsOuterReferencesInSelectList)
       {
         child(0) = child(0)->initiateLeftToInnerJoinTransformation
                             (normWARef);
         returnValue = TRUE;
        }
      }
    }

  return returnValue;
} // Like::predicateEliminatesNullAugmentedRows()

void NotIn::transformNode(NormWA & normWARef,
                                 ExprValueId & locationOfPointerToMe,
                                 ExprGroupId & introduceSemiJoinHere,
                                 const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
  {
    locationOfPointerToMe = getReplacementExpr();
    return;
  }

  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);



  if (CmpCommon::getDefault(NOT_IN_ANSI_NULL_SEMANTICS) == DF_OFF)
  {
      ItemExpr * tfm = NULL;
    // ignore ANSI NULL semantics
    // NOT IN produces same results as equivalent NOT EXISTS
    tfm = new (normWARef.wHeap())
                    BiRelat(ITM_EQUAL,
                            child(0),
                            child(1));
    if (tfm)
    {
      getValueId().replaceItemExpr(tfm);
      tfm->synthTypeAndValueId(TRUE);
      locationOfPointerToMe = tfm;

      // Make sure the new nodes are transformed
      locationOfPointerToMe->transformNode(normWARef, locationOfPointerToMe,
                                       introduceSemiJoinHere, externalInputs);

      // Set the replacement expression
      setReplacementExpr(locationOfPointerToMe);

    }
  }
  


} // NotIn::transformNode()

// NotIn::rewriteMultiColNotInPredicate( ValueId ninValId, ValueIdSet jPred)
// This method rewrites a multi-column NotIn (where the left cild is the 
// item list O1,O2,O3...On and the right child is the item list I1,I2,I3...In)
// as follows:
// - If all inner and outer columns are not nullable or are nullable 
//   but have no NULL values then the predicate is rewritten into:
//   ((O1 = I1) AND (O2 = I2) ... AND (On = In)) 
// - If all inner and outer columns are nullable and may have NULL values 
//   then the predicate is rewritten into:
//   NOT (( (O1 <> I1) OR (O2 <> I2) .... OR (On <> In)) IS TRUE)
// - If m ( where 0 < m < n) inner and outer columns are not nullable or are nullable 
//   but will have no NULL values and p (where p =n-m) columns are nullable and may 
//   have NULL values then the predicate is rewritten to:
//   ((O1 =  I1) AND (O2 = I2) ... AND (Om = Im)) AND 
//   NOT (( (Om+1 <> Im+1) OR (Om_2 <> Im+2) OR (On <> In)) IS TRUE)

ValueIdSet NotIn::rewriteMultiColNotInPredicate( 
                                  ValueId ninValId, 
                                  ValueIdSet jPred, 
                                  ValueIdSet selPred)
{
  ItemExpr * itmExpr = ninValId.getItemExpr();

  CMPASSERT ( itmExpr->getOperatorType() == ITM_NOT_IN &&
              itmExpr->child(0)->getOperatorType() == ITM_ITEM_LIST &&
              itmExpr->child(1)->getOperatorType() == ITM_ITEM_LIST);

  ItemExprList outer(itmExpr->child(0).getPtr(),HEAP, ITM_ITEM_LIST, FALSE);
  ItemExprList inner(itmExpr->child(1).getPtr(),HEAP, ITM_ITEM_LIST, FALSE);

  CMPASSERT(outer.entries() >0 && 
                  outer.entries() == inner.entries() );

  ValueIdSet predSet;

  ItemExpr * nonEquiPred = NULL;

  jPred-= ninValId;
  selPred-= ninValId;

  for (CollIndex i = 0 ; i < outer.entries() ; i++)
  {
    const NAType &innerType = inner[i]->getValueId().getType();
    const NAType &outerType = outer[i]->getValueId().getType();

    if (( !innerType.supportsSQLnull() || 
          jPred.isNotNullable(inner[i]->getValueId())) &&
        ( !outerType.supportsSQLnull() || 
          selPred.isNotNullable(outer[i]->getValueId())))
    {
      ItemExpr * newPred = new (CmpCommon::statementHeap()) 
                            BiRelat(ITM_EQUAL, 
                                    outer[i],
                                    inner[i]);
        
      newPred->synthTypeAndValueId(TRUE);
      predSet += newPred->getValueId();
          
    }
    else
    {
      ItemExpr * newPred = new (CmpCommon::statementHeap()) 
                            BiRelat(ITM_NOT_EQUAL, 
                                    outer[i],
                                    inner[i]);
            
      if (!nonEquiPred)
      {
        nonEquiPred = newPred;
      }
      else
      {
        nonEquiPred = new (CmpCommon::statementHeap()) 
                        BiLogic(ITM_OR, 
                                nonEquiPred,
                                newPred);
      }
    }
  }//for (CollIndex i = 0 ; i < outer.entries() ; i++)
        
  if (nonEquiPred)
  {
    nonEquiPred = new (CmpCommon::statementHeap()) 
            UnLogic(ITM_IS_TRUE, 
                            nonEquiPred);
    nonEquiPred = new (CmpCommon::statementHeap()) 
            UnLogic(ITM_NOT,
                            nonEquiPred);
    nonEquiPred->synthTypeAndValueId(TRUE);

    predSet += nonEquiPred->getValueId();

  }//if (nonEquiPred)

  return predSet;
}


// ***********************************************************************
// $$$$ Subquery
// ***********************************************************************

// -----------------------------------------------------------------------
// Subquery::transformNode()
// -----------------------------------------------------------------------
void Subquery::transformNode(NormWA & normWARef,
                             ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
                             const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
    {
      // Return the address of the expression that was used for replacing
      // this subquery in an earlier call.
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  // ---------------------------------------------------------------------
  // L <comparison op> ALL (R   ==>   L <inverse comparison op> ANY(R
  // ---------------------------------------------------------------------
  if (isAnAllSubquery() && normWARef.walkingAnExprTree())
    {
      ExprValueId newRoot = new(normWARef.wHeap())
                                UnLogic(ITM_NOT,
                                        new(normWARef.wHeap())
                                            QuantifiedComp(getComplementOfOperatorType(),
                                        child(0).getPtr(),
                                        getSubquery(),
                                        getAvoidHalloweenR2()));
      // replace the definition of this valueId
      getValueId().replaceItemExpr(newRoot);

      newRoot->synthTypeAndValueId(TRUE);

      ((QuantifiedComp *)newRoot->child(0)->castToItemExpr())->
                                                setCreatedFromALLpred(TRUE);
      // -----------------------------------------------------------------
      // Perform the subquery transformation.
      // -----------------------------------------------------------------
      newRoot->child(0)->transformNode(normWARef, newRoot->child(0),
                                       introduceSemiJoinHere, externalInputs);

      locationOfPointerToMe = newRoot;
    }
  else
    {
      // -----------------------------------------------------------------
      // Perform the Join-Aggregate or SemiJoin subquery transformation.
      // -----------------------------------------------------------------
      transformToRelExpr(normWARef, locationOfPointerToMe,
                        introduceSemiJoinHere, externalInputs);

    }

  // ---------------------------------------------------------------------
  // Needs to remember this subquery is under an expr, so that when we go
  // back to transform the new Join and its subtree introduced, we won't
  // incorrectly use the selection predicates in the subquery to convert
  // left join into inner join.
  // ---------------------------------------------------------------------
  if (normWARef.walkingAnExprTree())
    normWARef.setSubqUnderExprTreeFlag();

  markAsTransformed();
  // Set the replacement expression
  setReplacementExpr(locationOfPointerToMe);

} // Subquery::transformNode()

// -----------------------------------------------------------------------
// Subquery::evaluateCandidateForUnnesting()
// See if we can unnest this query, if so set appropriate flags
// -----------------------------------------------------------------------
void Subquery::evaluateCandidateForUnnesting(NormWA & normWARef, Join *newJoin, GroupByAgg *newGrby)
{
   NABoolean wasAnAllSubquery = FALSE ;
   NABoolean skipUnnesting = FALSE ;

   if (isAnAnySubquery())
      wasAnAllSubquery = ((QuantifiedComp*)this)->createdFromALLpred();


   if (NOT ((isAnExistsSubquery() && normWARef.isChildOfANot()) || // NOT EXISTS
           (wasAnAllSubquery || isAnAllSubquery()) || // [NOT] ALL, NOT IN
           newGrby->aggregateExpr().containsCount() ||  // COUNT(*), COUNT(a)
           normWARef.isChildOfAnIsNull() ||           // IS NULL, IS NOT NULL
           normWARef.inSelectList() || // in select list
           normWARef.inGenericUpdateAssign() ||
           normWARef.haveAnOrAncestor()))            // have an OR ancestor
   {
      newGrby->setContainsNullRejectingPredicates(TRUE);
   }

      // Skip Unnesting for the following reasons:
      // 1) comp_bool_167 = ON means every subquery gets a VEGRegion as 
      //    in R2.2
   if (CmpCommon::getDefault(COMP_BOOL_167) == DF_ON)
   {                                                          
      newGrby->setContainsNullRejectingPredicates(FALSE);
      skipUnnesting = TRUE;
   }

   // skip unnesting if we are in the context of a BEFORE trigger.
   // With BEFORE triggers, subqueries can only be present in the condition 
   // of the trigger. Unnesting in this case will distrupt the logic
   // of execution of the BEFORE trigger condition
   if (normWARef.isInBeforeTrigger())
   {
      skipUnnesting = TRUE;
   }

   //-------------------------------------------------------------------
   // If this is a scalar row subquery 
   // and subquery unnesting is enabled, mark this joins node as 
   // a candidate for unnesting during the SemanticQueryOptimize
   // subphase of the normalizer
   //--------------------------------------------------------------------
   if ((CmpCommon::getDefault(SUBQUERY_UNNESTING) != DF_OFF) &&
       (skipUnnesting == FALSE))
   {
       // Give a hint for QA that we are trying to unnest
    if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
      *CmpCommon::diags() << DgSqlCode(2997)
        << DgString1("Attempting to unnest Subquery");

    newJoin->setCandidateForSubqueryUnnest(TRUE);
    normWARef.incrementCorrelatedSubqCount(); 

    if (!newGrby->containsNullRejectingPredicates() )

          // If phase2 unnesting disabled 
          // do not unnest this subquery as it contains 
          // non-NullRejectingPreds
       if (CmpCommon::getDefault(SUBQUERY_UNNESTING_P2) == DF_OFF) 
        {
             // Disable non-NullRejecting predicate unnesting
           newJoin->setCandidateForSubqueryUnnest(FALSE);
           normWARef.decrementCorrelatedSubqCount(); 

             // Give a hint to QA that we skipped this subquery
           if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
           {
               *CmpCommon::diags() << DgSqlCode(2997)
                 << DgString1("Skipping unnesting of Subquery due to NonNullRejecting Predicates");
           }

        }
        else
        {
            // Try to unnest this subquery.
            newJoin->setCandidateForSubqueryLeftJoinConversion(TRUE);
        }
   }
} // Subquery::evaluateCandidateForUnnesting()

// -----------------------------------------------------------------------
// Subquery::transformToRelExpr()
// Perform the Join-Aggregate or SemiJoin subquery transformation.
// -----------------------------------------------------------------------
void Subquery::transformToRelExpr(NormWA & normWARef,
                                 ExprValueId & locationOfPointerToMe,
                                 ExprGroupId & introduceSemiJoinHere,
                                 const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
    return;
  markAsTransformed();

  CMPASSERT(getSubquery()->getOperatorType() == REL_ROOT);

  RelExpr *childOfRoot = getSubquery()->child(0);

  if (!isARowSubquery())
    {
      // -----------------------------------------------------------------
      // If it is a quantified subquery
      //   + if the subquery contains a DISTINCT, remove it
      //   + if it is an exist and a scalar aggregate with no having clause
      //     return true.
      // -----------------------------------------------------------------
      if (childOfRoot->getOperatorType() == REL_GROUPBY)
        {
          GroupByAgg *aggNode = (GroupByAgg *)childOfRoot;

          // If the group by has a group by list and no aggregate
          // functions we can eliminate it. Keep ROLLUP groupbys
          // for now. This could later be improved, if we have
          // "null-rejecting" predicates that exclude the extra
          // rows included by the rollup.
          if (!aggNode->groupExpr().isEmpty() &&
              aggNode->aggregateExpr().isEmpty() &&
              !aggNode->isRollup())
            {
              // Remove the aggNode and pass the selection predicates
              // and inputs to the new child of the Root
              childOfRoot = aggNode->child(0);
              if (childOfRoot->getOperator() == REL_ROOT)
                {
                  // Another RelRoot. Remove it
                  childOfRoot->child(0)->selectionPred() +=
                      childOfRoot->selectionPred();
                  childOfRoot->child(0)->getGroupAttr()->addCharacteristicInputs(
                      childOfRoot->getGroupAttr()->getCharacteristicInputs());
                  childOfRoot =  childOfRoot->child(0);
                }
              getSubquery()->child(0) = childOfRoot;
              childOfRoot->selectionPred() += aggNode->selectionPred();
              childOfRoot->getGroupAttr()->addCharacteristicInputs(
                        aggNode->getGroupAttr()->getCharacteristicInputs());

            }
        }
    }

  childOfRoot = getSubquery()->child(0);

  // ----------------------------------------------------------------------
  // If the subquery is a scalar aggregate there are certain optimizations
  // we can do now.
  // ----------------------------------------------------------------------
  GroupByAgg *aggNode = NULL;
  if (childOfRoot->getOperatorType() == REL_GROUPBY)
    {
      aggNode = (GroupByAgg *)childOfRoot;

      if (!aggNode->groupExpr().isEmpty())
         // Not a scalar Agg since we have a group expression.
        aggNode = NULL;
    }

  // An Exists on top of a scalar aggregate can be transformed to
  // a boolean constant if there are no having predicates.
  // Ansi 6.5 GR 1a+2a:  COUNT always returns a valid numeric value;
  //          GR 2b(i):  other aggs always return AT LEAST "the null value",
  //          which is also a single row.
  // Thus {EXISTS on top of any agg} == {EXISTS atop guaranteed-single-row}
  // == {always TRUE}.
  if (aggNode                           &&
     aggNode->selectionPred().isEmpty() &&
     isAnExistsSubquery()               )
    {
      ItemExpr *tf;

      if (isNotExists())
         tf = new(normWARef.wHeap()) BoolVal(ITM_RETURN_FALSE);
      else
         tf = new(normWARef.wHeap()) BoolVal(ITM_RETURN_TRUE);
      getValueId().replaceItemExpr(tf);
      tf->synthTypeAndValueId(TRUE);
      locationOfPointerToMe = tf;
      return;
    }

  Join * newJoin;
  ValueIdSet aggregateExpr; // for parameter passing
  ValueIdSet outputValues;

  // ---------------------------------------------------------------------
  // Allocate a new Join, create its group attributes and make it
  // the parent of the Groupby.
  // When a subquery contains an outer reference, only a tuple
  // substitution join (tsj) can be performed. At this stage of the
  // transformation since we do not know whether the subquery contains
  // an outer reference, we use a tsj. The normalizer will transform
  // it to a join later if it detects that a tsj is unnecessary.
  // ---------------------------------------------------------------------
  if (isARowSubquery() || normWARef.walkingAnExprTree())
    {
      GroupByAgg * newGrby;

      // -------------------------------------------------------------
      // If the child is a scalar aggregate we need a new groupby agg
      // only if are some having predicates. Also use the blackbox agg
      // for NOT EXISTS
      // -------------------------------------------------------------
     if (!aggNode ||
         !aggNode->selectionPred().isEmpty() ||
         isNotExists())
        {
          // -------------------------------------------------------------
          // Get aggregate functions and predicates that are required for
          // performing the subquery transformation.
          // Also, get the expression for replacing me (a subquery).
          // aggregateExpr contains the aggregate expressions.
          // -----------------------------------------------------------------
          locationOfPointerToMe = getBlackBoxExpr(normWARef, aggregateExpr);

          // -------------------------------------------------------------
          // Allocate a new Groupby. It has an empty Groupby list.
          // Its operand is the subquery tree.
          // aggregateExpr contains the aggregate expressions.
          // -------------------------------------------------------------
          newGrby = new(normWARef.wHeap())
                       GroupByAgg(getSubquery(), aggregateExpr);

          newGrby->setGroupAttr(new(normWARef.wHeap())
                                    GroupAttributes());

          // -------------------------------------------------------------
          // Add the inputs that the RelRoot under the Group By has to
          // those of the Group By.
          // -------------------------------------------------------------
          newGrby->getPotentialOutputValues(outputValues);
          newGrby->getGroupAttr()->addCharacteristicOutputs(outputValues);
          newGrby->getGroupAttr()->addCharacteristicInputs
                 (newGrby->child(0)->getGroupAttr()->getCharacteristicInputs());


        }
      else
        {
          newGrby = aggNode;
          if (isAnExistsSubquery())
            {
              // No predicate to apply
              locationOfPointerToMe = (ItemExpr *) NULL;
            }
          else if (isAnAnySubquery() || isAnAllSubquery())
            {
              // -------------------------------------------------------------
              // This codeblock cloned from QuantifiedComp::getBlackBoxExpr().
              // -------------------------------------------------------------
              // Replace the quantified predicate with a corresponding 
              // predicate that does not have a quantifier.
              // -------------------------------------------------------------
              OperatorTypeEnum otype;
              otype = ((QuantifiedComp*)this)->getSimpleOperatorType();

              BiRelat * pred = new(normWARef.wHeap())
                                  BiRelat(otype, child(0).getPtr(),
                                          getSubquery()->selectList());

              pred->specialMultiValuePredicateTransformation() = TRUE;

              // replace the definition of this valueId
              getValueId().replaceItemExpr(pred);

              pred->synthTypeAndValueId(TRUE);
              locationOfPointerToMe = pred;
            }
          else // must be row subquery
            {
              // Simply replace the row subquery with the selectlist
              // list of the subquery. We already have a guarantee
              // that at most one row will be returned.

              // Depending on if the subquery has already been flatten, 
              // we either have to return the first element or the whole 
              // of the select list.

              RelRoot *subqRoot = (RelRoot *) getSubquery();

              if (normWARef.inValueIdProxy())
              {
                 locationOfPointerToMe = subqRoot->compExpr()[0];
              } 
              else
              {
                 locationOfPointerToMe = getSubquery()->selectList();
              }
            }
        }

      // -----------------------------------------------------------------
      // Allocate a new tuple-substitution join.
      // It doesn't need to be a semi join since the group by has
      // no group by list and therefore is guaranteed to provide only
      // one value.
      // -----------------------------------------------------------------
      newJoin = new(normWARef.wHeap())
                    Join(introduceSemiJoinHere, newGrby, REL_TSJ);
      newJoin->setGroupAttr(new(normWARef.wHeap()) GroupAttributes());
      evaluateCandidateForUnnesting(normWARef, newJoin, newGrby);

    }
  else
    {
      // -------------------------------------------------------------
      // Get aggregate functions and predicates that are required for
      // performing the subquery transformation.
      // Also, get the expression for replacing me (a subquery).
      // aggregateExpr contains the aggregate expressions.
      // -----------------------------------------------------------------
      locationOfPointerToMe = getBlackBoxExpr(normWARef, aggregateExpr);

      if(isSelectivitySetUsingHint())
      {
        locationOfPointerToMe->setSelectivitySetUsingHint();
        locationOfPointerToMe->setSelectivityFactor(getSelectivityFactor());
      }

      DBGSETDBG( "TRANSFORM_DEBUG" )
      DBGIF(
         unp = "";
         aggregateExpr.unparse(unp);
         cerr << "QComp BlkBox: " << unp << " " << (void*)this << endl;
      )

      // -----------------------------------------------------------------
      // Allocate a new semijoin or anti semijoin
      // -----------------------------------------------------------------
      OperatorTypeEnum joinOp = REL_SEMITSJ;
      if (isAnAllSubquery() || isNotExists())
        joinOp = REL_ANTI_SEMITSJ;

      newJoin = new(normWARef.wHeap())
           Join(introduceSemiJoinHere, getSubquery(), joinOp);
    

      newJoin->setGroupAttr(new(normWARef.wHeap()) GroupAttributes());

      if (CmpCommon::getDefault
            (SEMIJOIN_TO_INNERJOIN_TRANSFORMATION) != DF_OFF)
      {
         if (joinOp == REL_SEMITSJ)
         {
	    newJoin->setCandidateForSemiJoinTransform(TRUE);
	    normWARef.setContainsSemiJoinsToBeTransformed(TRUE); 
         }
      }

      if ((joinOp == REL_SEMITSJ) && (aggNode != NULL))
      {
           // setup unnesting flags if applicable
        evaluateCandidateForUnnesting(normWARef, newJoin, aggNode);
      }

      // If this was an EXISTS the predicate in locationOfPointerToMe
      // should be null
      if (isAnExistsSubquery())
        {
          CMPASSERT(!locationOfPointerToMe.getPtr());
        }
      else
        {
          // Move the predicate to the ON clause of the semiJoin.
          // This is because we want to preserve one instance of the row
          // that satisfies the subquery predicate.
          newJoin->joinPred() += locationOfPointerToMe->getValueId();

          if (isAnAllSubquery() && 
              !normWARef.walkingAnExprTree() &&
              ( CmpCommon::getDefault(COMP_BOOL_139) == DF_ON))
            {
              ItemExpr *filt = new(normWARef.wHeap())
                UnLogic(ITM_IS_NOT_NULL, child(0).getPtr());
              filt->synthTypeAndValueId(TRUE);
              
              newJoin->selectionPred() += filt->getValueId();
            }

          locationOfPointerToMe = (ItemExpr *) NULL;
        }
    }

  // ---------------------------------------------------------------------
  // Initialize the dataflow for the tuple-substitution join that was
  // allocated above.
  // ---------------------------------------------------------------------
  newJoin->getPotentialOutputValues(outputValues);
  newJoin->getGroupAttr()->addCharacteristicOutputs(outputValues);

  // subtract from the external inputs to the joins values that are
  // produced by the children of the join.
  ValueIdSet realExternalInputs = externalInputs;
  realExternalInputs -= outputValues;
  newJoin->getGroupAttr()->addCharacteristicInputs(realExternalInputs);

  newJoin->child(1)->getGroupAttr()->addCharacteristicInputs(externalInputs);
  newJoin->child(1)->getGroupAttr()->addCharacteristicInputs
             (newJoin->child(0)->getGroupAttr()->getCharacteristicOutputs());

  // Transfer the avoidHalloweenR2 flag from the Subquery to the new
  // join.  If set in the join, this will cause the join to be
  // implemented as a hash join.  This causes the source to be blocked
  // which avoid the halloween problem.
  //
  if (newJoin->isSemiJoin() || newJoin->isAntiSemiJoin()) 
    {
      newJoin->setAvoidHalloweenR2(avoidHalloweenR2());
    }
  else if(getAvoidHalloweenR2())
    {
      getAvoidHalloweenR2()->resetAvoidHalloweenR2();
    }
  
//   CMPASSERT(!avoidHalloweenR2() || 
//             (newJoin->isSemiJoin() || newJoin->isAntiSemiJoin()));

  // ---------------------------------------------------------------------
  // Assign the SemiJoin to the original query tree.
  // A new SemiJoin is added in introduceSemiJoinHere. It has not been
  // transformed yet. It will eventually be transformed either by
  // it's parent or one of it's children if the SemiJoin becomes
  // became the ancestor of the child while the child was being
  // transformed.
  // ---------------------------------------------------------------------
  introduceSemiJoinHere = newJoin;

  // NB: The predicate just added to the joinPred may still contain
  //     some row subqueries.

  // ---------------------------------------------------------------------
  // Transform the item expression that replaces the subquery.
  // ---------------------------------------------------------------------
//  if (locationOfPointerToMe.getPtr())
//    locationOfPointerToMe
//         ->transformNode(normWARef, locationOfPointerToMe,
//                         introduceSemiJoinHere, externalInputs);

} // Subquery::transformToRelExpr()

// -----------------------------------------------------------------------
// Subquery::getBlackBoxExpr()
// -----------------------------------------------------------------------
ItemExpr * Subquery::getBlackBoxExpr(NormWA & /*normWARef*/,
                                     ValueIdSet & /*blackBoxExpr*/)
{
  ABORT("Internal Error: Implementation for Subquery::getBlackBoxExpr() is missing");
  return NULL;
} // Subquery::getBlackBoxExpr()

// -----------------------------------------------------------------------
// A method for inverting (finding the inverse of) the operators
// in a subtree that is rooted in a NOT.
// -----------------------------------------------------------------------
ItemExpr * Subquery::transformSubtreeOfNot(NormWA & normWARef,
                                           OperatorTypeEnum falseOrNot)
{
  return ItemExpr::transformSubtreeOfNot(normWARef, falseOrNot);
} // Subquery::transformSubtreeOfNot(normWARef);

// -----------------------------------------------------------------------
// Invert the quantified comparison type when this operator
// is the child of a NOT.
// -----------------------------------------------------------------------
OperatorTypeEnum Subquery::getComplementOfOperatorType()
{

  switch(getOperatorType())
    {
    case ITM_GREATER_ALL:
      return ITM_LESS_EQ_ANY;
    case ITM_GREATER_EQ_ALL:
      return ITM_LESS_ANY;
    case ITM_EQUAL_ALL:
      return ITM_NOT_EQUAL_ANY;
    case ITM_NOT_EQUAL_ALL:
      return ITM_EQUAL_ANY;
    case ITM_LESS_EQ_ALL:
      return ITM_GREATER_ANY;
    case ITM_LESS_ALL:
      return ITM_GREATER_EQ_ANY;

    case ITM_LESS_EQ_ANY:
      return ITM_GREATER_ALL;
    case ITM_LESS_ANY:
      return ITM_GREATER_EQ_ALL;
    case ITM_NOT_EQUAL_ANY:
      return ITM_EQUAL_ALL;
    case ITM_EQUAL_ANY:
      return ITM_NOT_EQUAL_ALL;
    case ITM_GREATER_ANY:
      return ITM_LESS_EQ_ALL;
    case ITM_GREATER_EQ_ANY:
      return ITM_LESS_ALL;

    default:
      return getOperatorType();
    }

} // Subquery::getComplementOfOperatorType

// ***********************************************************************
// $$$$ RowSubquery
// ***********************************************************************
ItemExpr * RowSubquery::getBlackBoxExpr(NormWA &  normWARef ,
                                        ValueIdSet & blackBoxExpr)
{
  // ---------------------------------------------------------------------
  // The oneRow aggregate function implements the row subquery semantics.
  // ---------------------------------------------------------------------
  ItemExpr * aggExpr = new(normWARef.wHeap())
                           Aggregate(ITM_ONE_ROW,
                           getSubquery()->selectList());
  aggExpr->synthTypeAndValueId(TRUE);
  aggExpr->convertToValueIdSet(blackBoxExpr);
  ((Aggregate *)aggExpr)->isOneRowTransformed_ = FALSE;
  aggExpr = aggExpr->transformOneRowAggregate( normWARef );

  // replace the definition of this valueId
  ItemExpr * aggChild = aggExpr->child(0)->castToItemExpr();
  if ((aggChild->getOperatorType() == ITM_CAST) ||
      (aggChild->getOperatorType() == ITM_INSTANTIATE_NULL))
  {
    // The valueId referes to the child of aggExpr, if the aggregate expression
    // contains a child that is not a list
    getValueId().replaceItemExpr(aggChild);
    getValueId().changeType(
        ( (Cast *) aggChild )->getType() );
    return aggChild;
  }
  else
  {
    getValueId().replaceItemExpr(aggExpr);
    return aggExpr;
  }
} // RowSubquery::getBlackBoxExpr()


// ***********************************************************************
// $$$$ EXISTS
// ***********************************************************************
// -----------------------------------------------------------------------
// A method for inverting (finding the inverse of) the operators
// in a subtree that is rooted in a NOT.
// -----------------------------------------------------------------------
ItemExpr * Exists::transformSubtreeOfNot(NormWA & normWARef,
                                         OperatorTypeEnum falseOrNot)
{
  ItemExpr *rv = this;

  // Genesis case 10-070717-7303
  // Turn an EXISTS subquery into the NOT_EXISTS flavor, which can be
  // transformed into an anti-semijoin.
  // This fix was added late and it can be turned off by default for safety.
  // This is the only place where we generate the ITM_NOT_EXISTS operator.
  // Generate NOT_EXISTS only if we will be making the semijoin/anti-semijoin
  // transformation. If we will be making the Join-Aggregate transformation
  // (as indicated by walkingAnExprTreeCount > 1) then do not generate NOT_EXISTS
  // Note that the caller UnLogic::transformSubtreeOfNot increments 
  // WalkingAnExprTreeCount_ by 1 and we are trying to account for that here.

  if ((CmpCommon::getDefault(COMP_BOOL_162) == DF_ON) &&
      (normWARef.getWalkingAnExprTreeCount() == 1))
    {
      rv = new(normWARef.wHeap()) Exists(getSubquery());

      // invert the operator type
      if (isNotExists())
         rv->setOperatorType(ITM_EXISTS);
      else
         rv->setOperatorType(ITM_NOT_EXISTS);
    }

  // the caller (UnLogic::transformNode()) expects that we do NOT
  // call synthTypeAndValueId() on the result

  return rv;
}

ItemExpr * Exists::getBlackBoxExpr(NormWA & normWARef, ValueIdSet & blackBoxExpr)
{
  ItemExpr * aggExpr;

 // ---------------------------------------------------------------------
  // The oneTrue aggregate function implements the exists subquery
  // semantics. The aggregate function is required only when the
  // exists subquery has an OR ancestor. Otherwise, a semijoin or
  // anti-semijoin (for NOT EXISTS) is performed between the operator
  // that contains the subquery and the subquery tree.
  // ---------------------------------------------------------------------
  if (normWARef.walkingAnExprTree())
    {
      ItemExpr *tf = new(normWARef.wHeap()) BoolVal(ITM_RETURN_TRUE);

      aggExpr = new(normWARef.wHeap()) Aggregate(ITM_ONE_TRUE, tf);

      CMPASSERT(NOT isNotExists());

      // replace the definition of this valueId
      getValueId().replaceItemExpr(aggExpr);

      aggExpr->synthTypeAndValueId(TRUE);
      aggExpr->convertToValueIdSet(blackBoxExpr);
    }
  else
    {
      blackBoxExpr.clear();
      aggExpr = NULL;
    }

  return aggExpr;

} // Exists::getBlackBoxExpr()

// -----------------------------------------------------------------------
// $$$$ QuantifiedComp
// -----------------------------------------------------------------------
ItemExpr * QuantifiedComp::transformMultiValuePredicate(
                                   NABoolean flattenSubqueries,
                                   ChildCondition tfmIf)
{
  if (tfmIf == ANY_CHILD_RAW)
    return dissectOutSubqueries(this, flattenSubqueries, FALSE);
  return NULL;
}

// -----------------------------------------------------------------------
// Invert the quantified comparison type when this operator
// is the child of a NOT.
// -----------------------------------------------------------------------
ItemExpr * QuantifiedComp::transformSubtreeOfNot(NormWA & normWARef,
                                          OperatorTypeEnum falseOrNot)
{

  if (falseOrNot != ITM_NOT)
    return this;

  OperatorTypeEnum otype = getComplementOfOperatorType();

  if (otype != getOperatorType()) // any ALL inverted to an ANY
    {
      ItemExpr * newComp = new(normWARef.wHeap())
                               QuantifiedComp(otype,
                                              child(0).getPtr(),
                                              getSubquery(),
                                              getAvoidHalloweenR2());
      return newComp;
    }
  else
    return this;
} // QuantifiedComp::transformSubtreeOfNot()

OperatorTypeEnum QuantifiedComp::getSimpleOperatorType() const
{
  OperatorTypeEnum otype = NO_OPERATOR_TYPE;
  // ---------------------------------------------------------------------
  // Deduce the comparison operator that is used together with the
  // quantifier.
  // ---------------------------------------------------------------------
  switch (getOperatorType())
    {
    case ITM_GREATER_ALL:
      otype = ITM_GREATER;
      break;
    case ITM_GREATER_EQ_ALL:
      otype = ITM_GREATER_EQ;
      break;
    case ITM_EQUAL_ALL:
      otype = ITM_EQUAL;
      break;
    case ITM_NOT_EQUAL_ALL:
      otype = ITM_NOT_EQUAL;
      break;
    case ITM_LESS_EQ_ALL:
      otype = ITM_LESS_EQ;
      break;
    case ITM_LESS_ALL:
      otype = ITM_LESS;
      break;

    case ITM_EQUAL_ANY:
      otype = ITM_EQUAL;
      break;
    case ITM_NOT_EQUAL_ANY:
      otype = ITM_NOT_EQUAL;
      break;
    case ITM_GREATER_ANY:
      otype = ITM_GREATER;
      break;
    case ITM_GREATER_EQ_ANY:
      otype = ITM_GREATER_EQ;
      break;
    case ITM_LESS_EQ_ANY:
      otype = ITM_LESS_EQ;
      break;
    case ITM_LESS_ANY:
      otype = ITM_LESS;
      break;

    default:
      ABORT("Internal error in QuantifiedComp::getSimpleOperatorType");
    }

  return otype;
}

ItemExpr * QuantifiedComp::getBlackBoxExpr(NormWA & normWARef, ValueIdSet & blackBoxExpr)
{
  ItemExpr * rv;
  OperatorTypeEnum otype = getSimpleOperatorType();

  // ---------------------------------------------------------------------
  // Replace the quantified predicate with a corresponding predicate
  // that does not have a quantifier.
  // ---------------------------------------------------------------------
  ItemExpr * pred = new(normWARef.wHeap())
                        BiRelat(otype, child(0).getPtr(),
                                getSubquery()->selectList());
  // ---------------------------------------------------------------------
  // Next is a hack to influence a later call to
  // BiRelat::transformNode/MultiValuePredicate on this pred.
  //
  // Probably would not be necessary if our caller Subquery::transformToRelExpr
  // extracted subquery-containing preds from the blackBoxExpr
  // (built by convertToValueIdSet below and returned by this method)
  // and processed them to introduce GroupByAggs.
  // But this special hack works for now.
  // ---------------------------------------------------------------------
  ((BiRelat *)pred)->specialMultiValuePredicateTransformation() = TRUE;

  // If this is ALL subquery and we are not walking an expression
  // tree we will generate an anti semijoin looking for a row where
  // the predicate is not true (either false or null)
  if (isAnAllSubquery() && !normWARef.walkingAnExprTree())
    {
      if (CmpCommon::getDefault(NOT_IN_OPTIMIZATION) == DF_ON && 
	  getOperatorType() == ITM_NOT_EQUAL_ALL)
      {

	//create a new NotIn with the child0 and child1 of birelat
	// 
	pred = new(normWARef.wHeap())
                        NotIn(pred->child(0),
                              pred->child(1));
      }
      else
      {
	pred = new(normWARef.wHeap())
                 UnLogic(ITM_IS_TRUE, pred);
	pred = new(normWARef.wHeap())
                 UnLogic(ITM_NOT,pred);

        // the code below is not required anymore after implementing the NOT IN optimization 
        // and may be removed in the future (maybe in R2.6)
	if ( (CmpCommon::getDefault(COMP_BOOL_139) == DF_ON)
           && (otype == ITM_NOT_EQUAL))
        {
          // Instead of the NOT(A <> B) create an (A = B) predicate
          // This allows the optimizer to take advantage of the
          // equi-predicate (Hash join will generate hash expression,
          // match partition joins will be considered ...)
          //

          // Only do this is all of the RHS values are not null.
          //
          ItemExpr *selList = getSubquery()->selectList();
                
          NABoolean allNotNull = TRUE;

          while(selList)
            {
              if(selList->getOperatorType() == ITM_ITEM_LIST)
                {
                  if(selList->child(1)->
                     getValueId().getType().supportsSQLnullLogical())
                    {
                      
                      allNotNull = FALSE;
                      break;
                    }
                  selList = selList->child(0);
                } 
              else
                {
                  if(selList->getValueId().getType().supportsSQLnullLogical())
                    {
                      allNotNull = FALSE;
                    }
                  selList = NULL;
                }
            }
                    
          if(allNotNull) 
            {
              pred = new(normWARef.wHeap())
                BiRelat(ITM_EQUAL, child(0).getPtr(),
                        getSubquery()->selectList());
              
              // Make sure to mark this new pred as was done above.
              //
              ((BiRelat *)pred)->
                specialMultiValuePredicateTransformation() = TRUE;
            }
        }
      }
    }

  // replace the definition of this valueId
  getValueId().replaceItemExpr(pred);

  pred->synthTypeAndValueId(TRUE);

  // ---------------------------------------------------------------------
  // If this ANY subquery does not have an OR, NOT, ISNULL or IS UNKNOWN
  // as an ancestor, then the magic aggregate function is not required.
  // ---------------------------------------------------------------------
  if (isAnAnySubquery() && normWARef.walkingAnExprTree())
    {
      // anyTrue(left operand = select list of subquery)
      ItemExpr * aggExpr = new(normWARef.wHeap())
                               Aggregate(ITM_ANY_TRUE, pred);
      aggExpr->synthTypeAndValueId(TRUE);
      aggExpr->convertToValueIdSet(blackBoxExpr);

      rv = aggExpr;
    }
  else
    {
      pred->convertToValueIdSet(blackBoxExpr);

      rv = pred;
   }

   // ---------------------------------------------------------------------
   // We cannot have an ALL subquery if we are walking an expression
   // tree. It would have been converted to a NOT ANY
   // assert it to make sure.
   // ---------------------------------------------------------------------
   CMPASSERT(!isAnAllSubquery() || !normWARef.walkingAnExprTree());

  return rv;

} // QuantifiedComp::getBlackBoxExpr()

// ***********************************************************************
// $$$$ UnLogic
// member functions for class UnLogic
// ***********************************************************************
ItemExpr * UnLogic::transformMultiValuePredicate(
                             NABoolean flattenSubqueries,
                             ChildCondition condBiRelat)
{
  // An MVP of the form "(an,item,list) IS [NOT] NULL"
  // is adequately handled in UnLogic::transformIsNull,
  // so no need to code for these two cases here.
  if (getOperatorType() == ITM_IS_NULL ||
      getOperatorType() == ITM_IS_NOT_NULL)
    return NULL;

  ItemExpr *tfm =
    child(0)->transformMultiValuePredicate(flattenSubqueries, condBiRelat);

  if (tfm && condBiRelat != ANY_CHILD_RAW)
    {
      tfm = new HEAP UnLogic(getOperatorType(), tfm);
      CMPASSERT(tfm);
    }

  return tfm;
}

// -----------------------------------------------------------------------
// If this item is of the form
//   NOT(IS_UNKNOWN(x))  or  IS_NOT_UNKNOWN(x)
// where x is
//   NOT*(truefalse(y))   (i.e. zero or more NOTs)
// where truefalse is any of the "IS ..." predicates (yield T or F, never UNK),
// then return item y; else return NULL.
//
// The caller can determine if item y has any side effects (e.g. aggregation);
// if none, then this entire item can be eliminated as it always evals as TRUE.
// -----------------------------------------------------------------------
ItemExpr * UnLogicMayBeAnEliminableTruthTest(ItemExpr *unlogic, NABoolean aggOK)
{
  ItemExpr *itm = unlogic;
  Int32 notVal = 0;
  for (; itm->getOperatorType() == ITM_NOT; notVal++)
    itm = itm->child(0);
  notVal %= 2;
  if ((notVal && itm->getOperatorType() == ITM_IS_UNKNOWN) ||
      (!notVal && itm->getOperatorType() == ITM_IS_NOT_UNKNOWN))
    {
      itm = itm->child(0);
      if (!canBeSQLUnknown(itm, FALSE))
      {
        // Drill down past any more UnLogics
        // ## Should make a virtual ItemExpr::isUnLogic() method ...
        while (itm->getOperatorType() == ITM_NOT           ||
               itm->getOperatorType() == ITM_IS_NULL       ||
               itm->getOperatorType() == ITM_IS_NOT_NULL   ||
               itm->getOperatorType() == ITM_IS_TRUE       ||
               itm->getOperatorType() == ITM_IS_FALSE      ||
               itm->getOperatorType() == ITM_IS_UNKNOWN    ||
               itm->getOperatorType() == ITM_IS_NOT_UNKNOWN)
          itm = itm->child(0);

        if (!itm->isASubquery() &&
            (aggOK || !itm->isAnAggregate()))
          {
            // Now we return itm, which will be one of:
            //   . an EXISTS or MATCH predicate
            //   . an IS [NOT] NULL operand (e.g. colref, item-list, row-subq)
            //   . an aggregate (e.g. ITM_ONE_ROW)

            DBGSETDBG( "TRANSFORM_DEBUG" )
            DBGIF(
              cerr << (Int32)itm->getOperatorType() << " " << (void *)itm
                   << " of "
                   << (Int32)unlogic->getOperatorType() << " " << (void*)unlogic
                   << " may be eliminable" << endl;
            )
            return itm;
          }
      }
    }
  return NULL;
} // UnLogicMayBeAnEliminableTruthTest()

#pragma nowarn(1506)   // warning elimination

//
// transformNode2() - a helper routine for UnLogic::transformNode()
//
// NOTE: The code in this routine came from the previous version of
//       UnLogic::transformNode().   It has been pulled out
//       into a separate routine so that the C++ compiler will produce
//       code that needs signficantly less stack space for the
//       UnLogic::transformNode() routine which get used in
//       recursive code.
//
void UnLogic::transformNode2(NormWA & normWARef,
                            ExprValueId & locationOfPointerToMe,
                            ExprGroupId & introduceSemiJoinHere,
                            const ValueIdSet & externalInputs)
{
  DBGSETDBG( "TRANSFORM_DEBUG" )
  DBGIF(
    unp = "";
    unparse(unp);
    cerr << (Int32)getOperatorType() << " "
         << (Int32)getValueId() << " "
         << (void *)this << " "
         << unp << endl;
  )

  // First convert an IS FALSE to a NOT if they are equivalent.
  // If we are transforming a constraint, we cannot use its
  // allowsUnknown property -- that would be putting the cart before the horse
  // -- that property is indeed what we wish to enforce in the contraint,
  // and rely on when not transforming a constraint.
  //
  if (getOperatorType() == ITM_IS_FALSE &&
      !normWARef.inConstraints() &&
      (!normWARef.walkingAnExprTree() ||
       !canBeSQLUnknown(child(0)))
     )
    {
      setOperatorType(ITM_NOT);
    }

  if (!normWARef.isChildOfANot())
    {
      // This short-circuits some of the logic in transformIsNull(),
      // doing some special cases a bit faster and I think more completely:
      //
      // We can eliminate ourself entirely if we will always eval as TRUE ...
      // (Too dangerous to do on an aggregate; let GroupByAgg handle that!)
      //
      NABoolean eliminate = TRUE;
      ItemExpr *descendant = UnLogicMayBeAnEliminableTruthTest(this);
      if (!descendant)
         {
            eliminate = FALSE;
            UnLogic notVal(ITM_NOT, this);
            descendant = UnLogicMayBeAnEliminableTruthTest(&notVal);
            notVal.setChild(0, NULL);   // so that "this" won't be destructed
         }
      if (descendant)
         {
             DBGSETDBG( "TRANSFORM_DEBUG" )
             DBGIF(
                cerr << (eliminate ? "Eliminating" : "Collapsing") << " pred "
                     << (Int32)getOperatorType() << " " << (void *)descendant
                     << " in this " << (void *)this << endl;
             )
             markAsTransformed();
             if (eliminate)
                locationOfPointerToMe = NULL;
             else
                {
                    locationOfPointerToMe = new(normWARef.wHeap())
                                                BoolVal(ITM_RETURN_FALSE);
                    locationOfPointerToMe->synthTypeAndValueId(TRUE);
                }
             setReplacementExpr(locationOfPointerToMe);
             return;
         }
    }

}
void UnLogic::transformNode(NormWA & normWARef,
                            ExprValueId & locationOfPointerToMe,
                            ExprGroupId & introduceSemiJoinHere,
                            const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  UnLogic::transformNode2( normWARef, locationOfPointerToMe,
                              introduceSemiJoinHere, externalInputs ) ;
  switch(getOperatorType())
    {
    case ITM_NOT:
    case ITM_IS_FALSE:
      {
         ItemExpr * rv = transformSubtreeOfNot(normWARef, getOperatorType());
           // If the transformation has eliminated this NOT, then
           // replace it with its substitute expression.
         if (rv != this)
           {
               // Replace the valueId of NOT with the negated rv
               getValueId().replaceItemExpr(rv);

               // All the virtual functions transfromSubtreeOfNot() should not
               // call synthTypeAndValueId() because we want to do it here.
               rv->synthTypeAndValueId(TRUE);

               locationOfPointerToMe = rv;
               rv->transformNode(normWARef, locationOfPointerToMe,
                                 introduceSemiJoinHere, externalInputs);
           }
         else
           {
              // this must be a NOT or IS FALSE
              markAsTransformed();
              normWARef.setNotFlag();
              child(0)->getReplacementExpr()->transformNode(normWARef,
                                                        child(0),
                                                        introduceSemiJoinHere,
                                                        externalInputs);
              normWARef.restoreNotFlag();
           }
      }
      break;
    case ITM_IS_NULL:
    case ITM_IS_NOT_NULL:
    case ITM_IS_UNKNOWN:
    case ITM_IS_NOT_UNKNOWN:
      transformIsNull(normWARef, locationOfPointerToMe,
                      introduceSemiJoinHere, externalInputs);
      break;
    case ITM_IS_TRUE:
      {
         // See comments in UnLogic::transformSubtreeOfNot()
         // about why testing !normWARef.inConstraints() is unnecessary here.

        if (!canBeSQLUnknown(child(0)) || !normWARef.walkingAnExprTree())
          {
            locationOfPointerToMe = child(0)->getReplacementExpr();
            child(0)->getReplacementExpr()->transformNode(normWARef,
                                                   locationOfPointerToMe,
                                                   introduceSemiJoinHere,
                                                   externalInputs);
          }
        else
          {
            normWARef.setNotFlag();
            ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                                    introduceSemiJoinHere, externalInputs);
            normWARef.restoreNotFlag();

            if (!child(0).getPtr())
               locationOfPointerToMe = NULL;
            else if (!canBeSQLUnknown(child(0)))
               locationOfPointerToMe = child(0);
          }
      }
      break;
    default:
      CMPASSERT(FALSE);
      break;
    }

  markAsTransformed();
  applyTruthTable(this, locationOfPointerToMe);
  setReplacementExpr(locationOfPointerToMe);

} // UnLogic::transformNode()
#pragma warn(1506)  // warning elimination

// -----------------------------------------------------------------------
// A method for transforming a subtree rooted in a NOT.
// -----------------------------------------------------------------------
ItemExpr * UnLogic::transformSubtreeOfNot(NormWA & normWARef,
                                          OperatorTypeEnum falseOrNot)
{
  ItemExpr * newChild = NULL;

  // Only NOT or IS FALSE is processed here
  if (getOperatorType() != ITM_NOT &&
      getOperatorType() != ITM_IS_FALSE)
    return this;

  if (getOperatorType() == ITM_NOT && // we do not want to apply this transformation NOT IN (...)
      child(0)->getOperatorType() == ITM_OR) // type queries. The jump table implementation of 
  {
    ValueIdList eqList;
    NABoolean status = convertToValueIdList(eqList,NULL, ITM_OR);
    if (!status)
    {
      NABoolean leaveAsOrExpr = TRUE;
      BiRelat * eqExpr = NULL;
      ValueId leftChildId;
      for (CollIndex i = 0; (leaveAsOrExpr && (i < eqList.entries())); i++)
      {
        if (eqList[i].getItemExpr()->getOperatorType() != ITM_EQUAL)
          leaveAsOrExpr = FALSE;

        eqExpr = (BiRelat *) eqList[i].getItemExpr() ;
        if (i == 0)
          leftChildId = eqExpr->child(0)->getValueId() ;
        if (eqExpr->child(0)->getValueId() != leftChildId)
          leaveAsOrExpr = FALSE;  // left side of '=' has different expressions, a = 1 OR b = 1
      }
      if (leaveAsOrExpr)
         return this; // do not apply DeMorganRule here as OR will handle them efficiently.
    }
  }

  // If my child is a redundant IS TRUE eliminate it first
  while (child(0)->getOperatorType() == ITM_IS_TRUE)
    {
      // No need to also check !normWARef.inConstraints()
      // since if grandchild can't be Unknown, there must be a
      // physical or logical CHECK(col IS NOT NULL) constraint,
      // and that constraint will be violated at run-time
      // rather than this CHECK(pred IS TRUE) constraint
      // (and it doesn't matter which constraint is violated as long as one is).
      //
      if (!canBeSQLUnknown(child(0)->child(0)))
        child(0) = child(0)->child(0);
      else
        break;
    }

  // First convert a child from IS FALSE to a NOT if they are equivalent.
  if (child(0)->getOperatorType() == ITM_IS_FALSE &&
      !normWARef.inConstraints() &&
      !canBeSQLUnknown(child(0)))
    {
      child(0)->setOperatorType(ITM_NOT);
    }

  // --------------------------------------------------------------------
  // Eliminate redundant NOTs:  NOT(NOT(x)) ==> x
  // NOT(NOT(p))         ==> p
  // ISFALSE(NOT(p))     ==> ISTRUE(p)
  // --------------------------------------------------------------------
  if (child(0)->getOperatorType() == ITM_NOT)
    {
      // Eliminate the child NOT here.
      // Perform the transformation of the grandChild independently
      // in the caller. This is because when two NOTs are eliminated
      // it is sufficient to call transformNode; a call to transformSubtreeOfNot
      // may not be required on the child. For example,
      //   NOT ( NOT (a = 10) ) should be a = 10 and not a <> 10.
      newChild = child(0)->child(0)->castToItemExpr();

      if (falseOrNot == ITM_IS_FALSE)
        {
          newChild = new(normWARef.wHeap())
                         UnLogic(ITM_IS_TRUE, newChild);
        }
    }
  else if (child(0)->getOperatorType() == ITM_IS_NOT_NULL &&
           child(0)->child(0)->getOperatorType() != ITM_ITEM_LIST)
    {
      // NOT (x) IS NOT NULL  ==> x IS NULL provided x is not a list
      newChild = child(0);
      newChild->setOperatorType(ITM_IS_NULL);
      setChild(0, NULL);
    }
  else if (child(0)->getOperatorType() == ITM_IS_NULL &&
           child(0)->child(0)->getOperatorType() != ITM_ITEM_LIST)
    {
      // NOT (x) IS NULL  ==> x IS NOT NULL provided x is not a list
      newChild = child(0);
      newChild->setOperatorType(ITM_IS_NOT_NULL);
      setChild(0, NULL);
    }
  else // child is not a NOT
    {
      // Invert the subtree
      normWARef.setNotFlag();

      // Perform transformations in my child subtree
      newChild = child(0)->transformSubtreeOfNot(normWARef, falseOrNot);

      normWARef.restoreNotFlag();

    } // child is not a NOT

  // If the child of the NOT has been inverted, it already reflects the
  // effect of the NOT.  Otherwise, inversion was not possible,
  // so return the original expression.
  if (newChild == child(0))
    newChild = this;

  // Do not synthTypeAndValueId() yet
  return newChild;

} // UnLogic::transformSubtreeOfNot()

// -----------------------------------------------------------------------
// A method for transforming a subtree rooted in an ISNULL or IS NOT NULL.
// -----------------------------------------------------------------------
void UnLogic::transformIsNull(NormWA & normWARef,
                              ExprValueId & locationOfPointerToMe,
                              ExprGroupId & introduceSemiJoinHere,
                              const ValueIdSet & externalInputs)
{
  if (child(0)->isAnAggregate())
    {
      if (child(0)->getOperatorType() == ITM_ONE_ROW) return;
      // ## other aggs? e.g. ONE_TRUE, ANY_TRUE, ...?
    }

  // Eliminate redundant EXISTs subquery : ISNULL(EXISTS ... )    ==> FALSE
  //                                       ISNOTNULL(EXISTS ... ) ==> TRUE
  
  if (child(0)->getOperatorType() == ITM_EXISTS ||
      child(0)->getOperatorType() == ITM_NOT_EXISTS)  // i.e. isAnExistsSubquery()
    {
      // If the ISNULL is not within a complex scalar expression, i.e., it
      // is either a lone predicate factor or the direct descendant of an
      // AND, which, in turn, is not within a complex expression, then
      // eliminate the predicate tree rooted in the ISNULL.
      if (getOperatorType() == ITM_IS_NOT_UNKNOWN ||
          getOperatorType() == ITM_IS_UNKNOWN)
         {
          ItemExpr *tf = new(normWARef.wHeap())
                            BoolVal(getOperatorType() == ITM_IS_NOT_UNKNOWN ?
                                    ITM_RETURN_TRUE : ITM_RETURN_FALSE);

          getValueId().replaceItemExpr(tf);
          tf->synthTypeAndValueId(TRUE);
          locationOfPointerToMe = tf;
          return;
         }
    } // child is an EXISTS
  else
    if (child(0)->getOperatorType() == ITM_ITEM_LIST)
      {
         /////////////////////////////////////////////////////////////////
         // multivalue is null, is not null, is unknown, is not unknown.
         // (a,b) IS NULL      ==>  a IS NULL AND b IS NULL
         // (a,b) IS NOT NULL  ==>  a IS NOT NULL AND b IS NOT NULL
         ////////////////////////////////////////////////////////////////

         // Convert the child subtree into list WITHOUT FLATTENING SUBQUERIES,
         // so they will be transformed to ONE_ROW aggregates.
         ItemExprList childList(child(0).getPtr(), normWARef.wHeap(),
                                ITM_ITEM_LIST, FALSE/*don't flatten subquery*/);

         ItemExpr * rootPtr = NULL;
         for (CollIndex i = 0; i < childList.entries(); i++)
           {
             ItemExpr * unLogPtr = new HEAP
                                       UnLogic(getOperatorType(), childList[i]);
             if (!rootPtr)
               rootPtr = unLogPtr;
             else
               rootPtr = new HEAP BiLogic(ITM_AND, rootPtr, unLogPtr);
           }

        // replace the definition of this valueId
        getValueId().replaceItemExpr(rootPtr);

        // Synthesize its type
        rootPtr->synthTypeAndValueId(TRUE);

        locationOfPointerToMe = rootPtr;

        normWARef.setNullFlag(); // set flag for influencing subquery transformations
        locationOfPointerToMe->transformNode(normWARef, locationOfPointerToMe,
                                             introduceSemiJoinHere,
                                             externalInputs);
        normWARef.restoreNullFlag();
      }
  else
    {
      ExprValueId saveLocationOfPointerToMe = locationOfPointerToMe;

      normWARef.setNullFlag(); // set flag for influencing subquery transform
      ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                              introduceSemiJoinHere, externalInputs);
      normWARef.restoreNullFlag();

      // If (x) is not nullable, then
      // (x) IS NOT NULL  ==>  always True;   (x) IS NULL  ==>  always False
      // If (x) is nullable, then check for
      // NULL IS NULL  ==>  always True;    NULL IS NOT NULL  ==>  always False
      //
      OperatorTypeEnum op = getOperatorType();
      ItemExpr *operand = child(0).getPtr();
      const NAType &operandType = child(0)->getValueId().getType();
      if (saveLocationOfPointerToMe == locationOfPointerToMe &&
          (op == ITM_IS_NOT_NULL || op == ITM_IS_NULL) &&
          !normWARef.inConstraints() && !normWARef.subqUnderExprTree() &&
          !normWARef.isInBeforeTrigger())
        {
          ItemExpr *tf = NULL;
          if (!operandType.supportsSQLnullLogical())  // check for non-nullability
            tf = new(normWARef.wHeap()) BoolVal(op == ITM_IS_NOT_NULL 
                                                  ? ITM_RETURN_TRUE 
                                                  : ITM_RETURN_FALSE);
          else if (operand->getOperator() == ITM_CONSTANT &&    // check for
                   static_cast<ConstValue*>(operand)->isNull()) //   NULL constant
            tf = new(normWARef.wHeap()) BoolVal(op == ITM_IS_NOT_NULL 
                                                  ? ITM_RETURN_FALSE
                                                  : ITM_RETURN_TRUE);
          if (tf)
            {
              getValueId().replaceItemExpr(tf);
              tf->synthTypeAndValueId(TRUE);
              locationOfPointerToMe = tf;
            }
          }
    }
} // UnLogic::transformIsNull

// -----------------------------------------------------------------------
// predicateEliminatesNullAugmentedRows()
// The following method determines whether a predicate is capable of
// discarding null augmented rows produced by a left join.
// -----------------------------------------------------------------------
NABoolean UnLogic::predicateEliminatesNullAugmentedRows(NormWA & normWARef,
                                            ValueIdSet & outerReferences)
{
  NABoolean returnValue = FALSE;
  
  // Short circuit the non-null rejecting ones.
  switch(getOperatorType())
  {
    case ITM_IS_NULL:
    case ITM_IS_UNKNOWN:
      return returnValue;
  }


  // -----------------------------------------------------------------
  // An InstantiateNull is used by the LeftJoin for instantiating a
  // null value in place of a value that is produced as output by
  // its second child. An InstantiateNull that appears in a binary
  // comparison predicate eliminates null augmented rows. As a
  // consequence, the LeftJoin becomes an InnerJoin. For example,

  // select t1.x
  // from t1 left join t2 on t1.x = t2.x
  // where NOT t2.x like '_B%';
  //
  //                   SelPred:
  //                   ------> NOT T2.x Like 'Hello%'
  //                  |
  //                  LJ JoinPred: t1.x = t2.x
  //                 /  \
  //                T1    T2
  // The Selection predicate NOT T2.x LIKE 'Hello%' will cause all the
  // LeftJoin (LJ) operators to be transformed to InnerJoins.
  // -----------------------------------------------------------------
  if (!normWARef.walkingAnExprTree() && !normWARef.subqUnderExprTree())
    {                        // does NOT lie within an expression tree
      // -------------------------------------------------------------
      // Initiate the left to inner join transformation. Replace
      // the InstantiateNull with the original expression that
      // is the subject for null instantiation.
      // 
      // We only need to check child 0 of the Like function.
      // -------------------------------------------------------------

    GroupAttributes emptyGA;
    emptyGA.setCharacteristicOutputs(outerReferences);
    ValueIdSet emptySet, emptySet1, coveredExpr, coveredSubExpr;
    ValueIdSet instNullValue;
    NABoolean containsOuterReferencesInSelectList = FALSE;
    if (child(0)->getOperatorType() == ITM_INSTANTIATE_NULL)
     {
       InstantiateNull *inst = (InstantiateNull *)child(0)->castToItemExpr();
       if ((normWARef.inSelectList())&&!(inst->NoCheckforLeftToInnerJoin))
       {
         instNullValue.insert(inst->getValueId());
         emptyGA.coverTest(
           instNullValue,
           emptySet,
           coveredExpr,
           emptySet1,
           &coveredSubExpr);
         if(!coveredExpr.isEmpty())
         {
           containsOuterReferencesInSelectList = TRUE;
         }
       }
       if ((!inst->NoCheckforLeftToInnerJoin)&&!containsOuterReferencesInSelectList)
       {
         child(0) = child(0)->initiateLeftToInnerJoinTransformation
                             (normWARef);
         returnValue = TRUE;
        }
         // we have to dig deeper
     } else if (child(0)->predicateEliminatesNullAugmentedRows
                                         (normWARef, outerReferences) == TRUE)
     {
          returnValue = TRUE;
     }
    }

  return returnValue;
} // UnLogic::predicateEliminatesNullAugmentedRows()


// MDAMR


// -----------------------------------------------------------------------
// Perform an MDAM tree walk on UnLogic
// -----------------------------------------------------------------------
DisjunctArray * UnLogic::mdamTreeWalk()
{
  // ---------------------------------------------------------------------
  // First a DisjunctArray is allocated.  Then a ValueIdSet is allocated
  // with the value id of this predicate added to the set.  The pointer
  // to this ValueIdSet is then inserted as the first entry of the array.
  // The pointer to the DisjunctArray is returned to the previous BiLogic
  // node as this method recurses back up the expression tree.
  // ---------------------------------------------------------------------
  return new HEAP DisjunctArray(new HEAP ValueIdSet(getValueId()));

} // UnLogic::mdamTreeWalk()

// MDAMR


ItemExpr * UnLogic::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return getReplacementExpr();
  markAsNormalized();

  switch(getOperatorType())
  {
    case ITM_NOT:
    case ITM_IS_FALSE:
      normWARef.setNotFlag();
      child(0) = child(0)->getReplacementExpr()->normalizeNode(normWARef);
      normWARef.restoreNotFlag();
      break;

    case ITM_IS_NULL:
    case ITM_IS_NOT_NULL:
    case ITM_IS_UNKNOWN:
    case ITM_IS_NOT_UNKNOWN:
      normWARef.setNullFlag();
      child(0) = child(0)->getReplacementExpr()->normalizeNode(normWARef);
      normWARef.restoreNullFlag();
      break;

    case ITM_IS_TRUE:
      // "canBeSQLUnknown" should always be true, so you can only take
      // the if branch when not walking an expression tree.
      if (!canBeSQLUnknown(child(0)) || !normWARef.walkingAnExprTree())
      {
        child(0) = child(0)->getReplacementExpr()->normalizeNode(normWARef);
      }
      else
      {
        normWARef.setNotFlag();
        child(0) = child(0)->getReplacementExpr()->normalizeNode(normWARef);
        normWARef.restoreNotFlag();
      }
      break;

    default:
      CMPASSERT(FALSE);
      break;
  } // end switch

  return getReplacementExpr();

} // UnLogic::normalizeNode()


// ***********************************************************************
// $$$$ VEGReference
// member functions for class VEGReference
// ***********************************************************************
ItemExpr * VEGReference::normalizeNode(NormWA &  /*normWARef */)
{
  // ---------------------------------------------------------------------
  // If a reference to a VEG is encountered here, replace with with the
  // VEGReference of its VEG. By doing so, we account for any merges
  // of VEGs that have happened.
  //      original VEGReference -> original VEG
  // After a merge:
  //      original VEGReference -> new VEG -> new VEGReference
  // ---------------------------------------------------------------------
  return ((VEGReference *)this)->getVEG()->getVEGReference();
} // VEGReference::normalizeNode()

// ***********************************************************************
// $$$$ VEGPredicate
// member functions for class VEGPredicate
// ***********************************************************************
ItemExpr * VEGPredicate::normalizeNode(NormWA & /* normWARef */)
{
  // ---------------------------------------------------------------------
  // If a reference to a VEG is encountered here, replace with with the
  // VEGReference of its VEG. By doing so, we account for any merges
  // of VEGs that have happened.
  //      original VEGReference -> original VEG
  // After a merge:
  //      original VEGReference -> new VEG -> new VEGReference
  // ---------------------------------------------------------------------
  return ((VEGPredicate *)this)->getVEG()->getVEGPredicate();
} // VEGPredicate::normalizeNode()

// MDAMR
// -----------------------------------------------------------------------
// Perform an MDAM tree walk on VEGPredicate
// -----------------------------------------------------------------------
DisjunctArray * VEGPredicate::mdamTreeWalk()
{
  // ---------------------------------------------------------------------
  // First a DisjunctArray is allocated.  Then a ValueIdSet is allocated
  // with the value id of this predicate added to the set.  The pointer
  // to this ValueIdSet is then inserted as the first entry of the array.
  // The pointer to the DisjunctArray is returned to the previous BiLogic
  // node as this method recurses back up the expression tree.
  // ---------------------------------------------------------------------
  return new HEAP DisjunctArray(new HEAP ValueIdSet(getValueId()));

} // VEGPredicate::mdamTreeWalk()
// MDAMR

// ***********************************************************************
// $$$$ ValueIdUnion
// member functions for class ValueIdUnion
// ***********************************************************************
ItemExpr * ValueIdUnion::normalizeNode(NormWA & /* normWARef */)
{
  if (nodeIsNormalized())
    return this;
  markAsNormalized();
  // ---------------------------------------------------------------------
  // A ValueIdUnion is not replaced with a VEGReference, if at all such
  // a replacement is possible, during normalization. It is retained as-is.
  // During predicate pushdown the normalizer replaces the ValueIdUnion
  // with an expression that is appropriate in the context of the pushdown.
  // Otherwise, the operator represents the result of the union in the
  // dataflow tree for the query.
  // ---------------------------------------------------------------------
  return this;      // return unchanged do not replace with a VEGReference.
} // ValueIdUnion::normalizeNode()

ItemExpr * ValueIdUnion::normalizeSpecificChild(NormWA & normWARef, Lng32 childIndex)
{

  CMPASSERT(childIndex < (Lng32)entries());

  sources_[childIndex] =
    ((ItemExpr *)(sources_[childIndex].getItemExpr()->
                  normalizeNode(normWARef)))->getValueId();

  // If the result is different from this ValueIdUnion, normalize it too
  if (result_ != getValueId())
    result_ = ((ItemExpr *)(result_.getItemExpr()->normalizeNode(normWARef)))
                              ->getValueId();
  return this;
} // ValueIdUnion::normalizeSpecificChild()


// No transformation for this node.
void ZZZBinderFunction::transformNode(NormWA & normWARef,
                                      ExprValueId & locationOfPointerToMe,
                                      ExprGroupId & introduceSemiJoinHere,
                                      const ValueIdSet & externalInputs)
{
  ABORT("ZZZBinderFunction should never reach here.");

  return;
}
// ***********************************************************************
// $$$$ ItmSequenceFunction
// member functions for class ItmSequenceFunction
// ***********************************************************************

//
// Redefinition of virtual function to check for nested THIS function.
// A THIS function can occur within a ROWS SINCE, but not within
// any other sequence function.
//
NABoolean ItmSequenceFunction::containsTHISFunction()
{
  Int32 arity = getArity();
  NABoolean result = FALSE;

  for (Int32 i = 0; i < arity; i++)
  {
   if (child(i)->containsTHISFunction())
   {
     result = TRUE;
     if (getOperatorType() != ITM_ROWS_SINCE)
     {
       CMPASSERT("Invalid nested THIS function in Normalizer.");
     }
   }
   else     // child does not contain THIS
   {
    if (getOperatorType() == ITM_THIS)  // I am a THIS, return TRUE.
      result = TRUE;
   }
  }
  return result;
} //  ItmSequenceFunction::containsTHISFunction()

// ***********************************************************************
// $$$$ ItmSeqOffset
// member functions for class ItmSeqOffset
// ***********************************************************************
//
// Transform optional third argument into ScalarMin of second and third args.
//
void ItmSeqOffset::transformNode(NormWA & normWARef,
                                 ExprValueId & locationOfPointerToMe,
                                 ExprGroupId & introduceSemiJoinHere,
                                 const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  // ---------------------------------------------------------------------
  // Normalize the operands of the DIFF1
  // ---------------------------------------------------------------------
  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);

  if (getArity() == 3)
  {
   ItemExpr *newMin = new HEAP ItmScalarMinMax (ITM_SCALAR_MIN, child(1), child(2));
   newMin->synthTypeAndValueId(TRUE);
   child (1) = newMin;
   child (2) = NULL;
  }

} // ItmSeqOffset::transformNode()

void ItmLagOlapFunction::transformNode(NormWA & normWARef,
                                 ExprValueId & locationOfPointerToMe,
                                 ExprGroupId & introduceSemiJoinHere,
                                 const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);

} // ItmSeqOffset::transformNode()

void ItmLeadOlapFunction::transformNode(NormWA & normWARef,
                                 ExprValueId & locationOfPointerToMe,
                                 ExprGroupId & introduceSemiJoinHere,
                                 const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);

} // ItmLeadOlapFunction::transformNode()

// ***********************************************************************
// $$$$ ItmSeqDiff1
// member functions for class ItmSeqDiff1
// ***********************************************************************
//
// Transform Diff1(exp) into ((exp) - OFFSET (exp, 1));
// Transform Diff1 (x,y) into  (x-OFFSET(x,1)) / (y-OFFSET(y,1))
//
ItemExpr * ItmSeqDiff1::transformDiff1()
{
  // ---------------------------------------------------------------------
  // Replace  Diff1(exp) with exp - OFFSET(exp, 1)
  //                           ^
  //                           |
  //                        child1
  //
  //
  // Replace  Diff1 (x,y) with  (x-OFFSET(x,1)) / (y-OFFSET(y,1))
  // ---------------------------------------------------------------------

  ItemExpr *offsetExpr  = new HEAP ItmSeqOffset ( child(0), 1);   // new OFFSET expression
  ((ItmSeqOffset *)offsetExpr)->setIsOLAP(isOLAP());
  ItemExpr *tfm         = new HEAP BiArith(ITM_MINUS,
                                           child(0).getPtr(),         // child(0) is still child 0
                                           offsetExpr);

    //
    //  Generate DIFF1 (y) == y-OFFSEY(y,1)
    //
  if (getArity() == 2)
  {
    ItemExpr *tfm1         = tfm;
    ItemExpr *offsetExpr2  = new HEAP ItmSeqOffset (child(1), 1);         // new OFFSET expression
    ((ItmSeqOffset *)offsetExpr2)->setIsOLAP(isOLAP());
    ItemExpr *tfm2         = new HEAP BiArith(ITM_MINUS,                 // y - OFFSET (y, 1)
                                           child(1).getPtr(),            // child(1) is child (0)
                                           offsetExpr2) ;
    //
    // The following is a kludge to allow DIFFn (x,y) of dates and intervals; otherwise,
    // the divisor, that is 'y - OFFSET(y,1)', cannot be type INTERVAL.
    //
    NABuiltInTypeEnum  opType = child(1)->getValueId().getType().getTypeQualifier();

    if (opType == NA_INTERVAL_TYPE  || opType == NA_DATETIME_TYPE)
    {
     ItemExpr *castExpr   = new HEAP Cast (tfm2,
                                          new HEAP
                                          SQLLargeInt(TRUE, TRUE)); // (must be) signed; nulls allowed
     tfm2 = castExpr;
    }

    tfm                     = new HEAP BiArith(ITM_DIVIDE,
                                               tfm1,
                                               tfm2);
  }  // end getArity == 2
  CMPASSERT(tfm);
  return tfm;
}

void ItmSeqDiff1::transformNode(NormWA & normWARef,
                                ExprValueId & locationOfPointerToMe,
                                ExprGroupId & introduceSemiJoinHere,
                                const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  // ---------------------------------------------------------------------
  // Normalize the operands of the DIFF1
  // ---------------------------------------------------------------------
  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);

  ItemExpr *tfm = transformDiff1();
  getValueId().replaceItemExpr(tfm);
  tfm->synthTypeAndValueId(TRUE);
  locationOfPointerToMe = tfm;

  // Make sure the new nodes are transformed
  locationOfPointerToMe->transformNode(normWARef, locationOfPointerToMe,
                                       introduceSemiJoinHere, externalInputs);

  // Set the replacement expression
  setReplacementExpr(locationOfPointerToMe);

} // ItmSeqDiff1::transformNode()

// ***********************************************************************
// $$$$ ItmSeqDiff2
// member functions for class ItmSeqDiff2
// ***********************************************************************
//
// Transform Diff2(exp) into (DIFF1(exp) - OFFSET (DIFF1(exp), 1))
// Transform Diff2(x,y) into  DIFF2(x) / DIFF1(y)
//                      or   (DIFF1(x) - OFFSET (DIFF1(x), 1)) / DIFF1(y)
//
// Transform node will recursively transform the children.
//
ItemExpr * ItmSeqDiff2::transformDiff2()
{
  ItemExpr *newDiff1    = new HEAP ItmSeqDiff1 (child(0));                // new Diff1 expression
  ItemExpr *offsetExpr  = new HEAP ItmSeqOffset (newDiff1, 1);  // new OFFSET expression: OFFSET(DIFF1(x), 1)
  ((ItmSeqOffset *)offsetExpr)->setIsOLAP(isOLAP());

  ItemExpr *tfm         = new HEAP BiArith(ITM_MINUS,                     // new MINUS expression
                                           newDiff1,                      // newDiff1 is child 0
                                           offsetExpr);
  if (getArity() == 2)
  {
    ItemExpr *tfm1         = tfm;                                        // save DIFF2 expression
    ItemExpr *tfm2         =  new HEAP ItmSeqDiff1 (child(1));           // new Diff1 expression
    //
    // The following is a kludge to allow DIFF2(x, y) where y is dates or intervals; otherwise,
    // the divisor, that is 'DIFF1(Y)', cannot be type INTERVAL.
    //
    NABuiltInTypeEnum  opType = child(1)->getValueId().getType().getTypeQualifier();

    if (opType == NA_INTERVAL_TYPE  || opType == NA_DATETIME_TYPE)
    {
     ItemExpr *castExpr   = new HEAP Cast (tfm2,
                                          new HEAP
                                          SQLLargeInt(TRUE, TRUE)); // (must be) signed; nulls allowed
     tfm2 = castExpr;
    }

    tfm                     = new HEAP BiArith(ITM_DIVIDE,
                                               tfm1,
                                               tfm2);

  }  // end getArity == 2
  CMPASSERT(tfm);
  return tfm;
}

void ItmSeqDiff2::transformNode(NormWA & normWARef,
                                ExprValueId & locationOfPointerToMe,
                                ExprGroupId & introduceSemiJoinHere,
                                const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  // ---------------------------------------------------------------------
  // Normalize the operands of the DIFF2
  // ---------------------------------------------------------------------
  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);

  ItemExpr *tfm = transformDiff2();
  getValueId().replaceItemExpr(tfm);
  tfm->synthTypeAndValueId(TRUE);
  locationOfPointerToMe = tfm;

  // Make sure the new nodes are transformed
  locationOfPointerToMe->transformNode(normWARef, locationOfPointerToMe,
                                       introduceSemiJoinHere, externalInputs);

  // Set the replacement expression
  setReplacementExpr(locationOfPointerToMe);

} // ItmSeqDiff2::transformNode()

// ***********************************************************************
// $$$$ ItmSeqRunningFunction
// member functions for class ItmSeqRunningFunction
// ***********************************************************************
void ItmSeqRunningFunction::transformNode(NormWA & normWARef,
                                          ExprValueId & locationOfPointerToMe,
                                          ExprGroupId & introduceSemiJoinHere,
                                          const ValueIdSet & externalInputs)
{
  ValueId cacheEquivTransSeqId;
  ItemExpr * thisItem = locationOfPointerToMe.getPtr();
  if (CmpCommon::getDefault(COMP_BOOL_201) == DF_ON){
    if (normWARef.findEquivalentInSeqFunctionsCache( thisItem, cacheEquivTransSeqId)) {
      locationOfPointerToMe = cacheEquivTransSeqId;
      return;
    }
  }
  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  // ---------------------------------------------------------------------
  // Normalize the operands of the Running Function
  // ---------------------------------------------------------------------
  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);

  OperatorTypeEnum op = getOperatorType();

  if (op  == ITM_RUNNING_SDEV  ||
       op == ITM_RUNNING_VARIANCE  ||
       op == ITM_RUNNING_AVG ||
       op == ITM_RUNNING_RANK ||
       op == ITM_RUNNING_DRANK)
  {
   ItemExpr *tfm = NULL;

   switch (getOperatorType())
   {
     case ITM_RUNNING_VARIANCE:
     case ITM_RUNNING_SDEV:
       tfm = transformRunningVariance();
       break;
     case ITM_RUNNING_AVG:
       tfm = transformRunningAvg();
       break;
     case ITM_RUNNING_RANK:
       tfm = transformRunningRank();
       break;
     case ITM_RUNNING_DRANK:
       {
         ItemExpr *change
           = new HEAP ItmSeqRunningFunction(ITM_RUNNING_CHANGE, 
                                            child(0));
         ((ItmSeqRunningFunction *)change)->setIsOLAP(isOLAP());
         ItemExpr *pred = new HEAP BiRelat(ITM_EQUAL,
                                           change,
                                           new HEAP SystemLiteral(1));

         tfm = new HEAP 
           IfThenElse(pred, 
                      new HEAP SystemLiteral(1),
                      new HEAP SystemLiteral(0));                      
        
         tfm = new HEAP Case(NULL, tfm);

         tfm = new HEAP ItmSeqRunningFunction(ITM_RUNNING_SUM, tfm);
         ((ItmSeqRunningFunction *)tfm)->setIsOLAP(isOLAP());
         tfm = new HEAP Cast(tfm, &getValueId().getType());

         break;
      }
     default:
     break;
   }
   getValueId().replaceItemExpr(tfm);
   tfm->synthTypeAndValueId(TRUE);
   locationOfPointerToMe = tfm;

   // Make sure the new nodes are transformed
   locationOfPointerToMe->transformNode(normWARef, locationOfPointerToMe,
                                   introduceSemiJoinHere, externalInputs);

   // Set the replacement expression
   setReplacementExpr(locationOfPointerToMe);
   
   if (CmpCommon::getDefault(COMP_BOOL_201) == DF_ON){
    normWARef.insertIntoSeqFunctionsCache(thisItem, locationOfPointerToMe.getValueId());
   }

  }  // end SDEV, VARIANCE or AVG

} // ItmSeqRunningFunction::transformNode()


//------------------------------------------------------------------------------------
//  transformRunningVariance()
//
//  Transforms RUNNINGVARIANCE(x) or RUNNINGSDEV(x) into ScalarVariance:
//   RSUM ( (x - AVG(x))2) / (RUNNINGCOUNT(*) - 1)
//  == (RUNNING_SUM(Xi*Xi) - RUNNING_AVG(Xi)*RUNNING_SUM(Xi)) / (N-1)
//  == (RUNNING_SUM (x * x) - (RUNNING_AVG(x) * RUNNING_SUM(x)) / RUNNINGCOUNT(*) -1
//
//  Thus, terms needed for Scalar Variance are:
//   1. RUNNING_SUM (x * x) ==> "sumOfValSquared"
//   2. RUNNING_SUM(x)      ==> "sumOfVal"
//   3. RUNNING_COUNT(*)    == countofVal
//
//  All other terms can be derived from these.
//
// In ExFunctionSVariance::eval, the following calculations are performed (if VARIANCE):
//  avgOfVal = sumOfVal/countOfVal;
//
//  result = ( sumOfValSquared -  (2 * avgOfVal * sumOfVal) + (sumOfVal * avgOfVal)) / (countOfVal - 1);
//
// In ExFunctionSVariance::eval (if STDDEV)
//  result = sqrt ( sumOfValSquared -  (2 * avgOfVal * sumOfVal) + (sumOfVal * avgOfVal)) / (countOfVal - 1));
//--------------------------------------------------------------------------------------
ItemExpr * ItmSeqRunningFunction::transformRunningVariance()
{
  const NAType *desiredType    = new HEAP SQLDoublePrecision(TRUE);
  ItemExpr *childDouble        = new HEAP Cast(child(0), desiredType);
  ItemExpr *childDoubleSquared = new HEAP BiArith(ITM_TIMES,                       // x * x
                                                childDouble,
                                                childDouble);

  ItemExpr *sumOfValSquared    = new HEAP ItmSeqRunningFunction                    // RUNNINGSUM (x * x)
                                         (ITM_RUNNING_SUM, childDoubleSquared);
  ((ItmSeqRunningFunction *)sumOfValSquared)->setIsOLAP(isOLAP());
  ItemExpr *sumOfVal           = new HEAP ItmSeqRunningFunction                    // RUNNINGSUM (x)
                                         (ITM_RUNNING_SUM, childDouble);
  ((ItmSeqRunningFunction *)sumOfVal)->setIsOLAP(isOLAP());
  ItemExpr *const1             = new HEAP ConstValue(1);                           // constant value 1

  ItemExpr *countOfVal         = new HEAP ItmSeqRunningFunction                    // RUNNINGCOUNT (*)
                                          (ITM_RUNNING_SUM, const1);
  ((ItmSeqRunningFunction *)countOfVal)->setIsOLAP(isOLAP());
  ItemExpr *castDouble        = new HEAP Cast(countOfVal, desiredType);

  OperatorTypeEnum newOp;

  if (getOperatorType() == ITM_RUNNING_VARIANCE)
  {
   newOp = ITM_VARIANCE;
  }
  else
  {
   CMPASSERT (getOperatorType() == ITM_RUNNING_SDEV);
   newOp = ITM_STDDEV;
  }
  ItemExpr *result = new HEAP
                        ScalarVariance(newOp,
                                        sumOfValSquared,
                                        sumOfVal,
                                        castDouble) ;

  return result;
}
// transformRunningAvg
//
// transforms RUNNINGAVG (x) into
// RUNNINGSUM(x) / RUNNINGCOUNT(x)
//
ItemExpr * ItmSeqRunningFunction::transformRunningAvg()
{
  ItemExpr *newSum      = new HEAP ItmSeqRunningFunction (ITM_RUNNING_SUM, child(0)); // new RUNNINGSUM(x)
  ((ItmSeqRunningFunction *)newSum)->setIsOLAP(isOLAP());
  ItemExpr *newCount    = new HEAP ItmSeqRunningFunction (ITM_RUNNING_COUNT, child(0));// new RUNNINGCOUNT(x)
  ((ItmSeqRunningFunction *)newCount)->setIsOLAP(isOLAP());
  ItemExpr *tfm         = new HEAP BiArith(ITM_DIVIDE,                                          // RUNNINGSUM(x) / RUNNINGCOUNT(x)
                                           newSum   ,
                                           newCount);

  return tfm;
}

ItemExpr * ItmSeqRunningFunction::transformRunningRank()
{
  ItemExpr *tfm = NULL;

  NABoolean inv = FALSE;
  child(0) = child(0)->removeInverseFromExprTree(inv);

  if (inv && !(isTDFunction() || isOLAP()))
  {//Using ASC/DESC with sequence functions is not supported
    *CmpCommon::diags() << DgSqlCode(-4362);
    return this;
  }  

  ItemExpr *rcStar = new HEAP ItmSeqRunningFunction(ITM_RUNNING_COUNT, 
                                                    new HEAP SystemLiteral(1));
  ((ItmSeqRunningFunction *)rcStar)->setIsOLAP(isOLAP());

  ItemExpr *change = new HEAP ItmSeqRunningFunction(ITM_RUNNING_CHANGE, 
                                                    child(0));
  ((ItmSeqRunningFunction *)change)->setIsOLAP(isOLAP());

  tfm = new HEAP BiArith(ITM_MINUS, rcStar, change);

  tfm->synthTypeAndValueId(TRUE);
	    
  //set type to original type after if has been changed to BigNum above
  tfm->getValueId().changeType(&getValueId().getType());

  tfm = new HEAP BiArith(ITM_PLUS, tfm, new HEAP SystemLiteral(1));

  tfm->synthTypeAndValueId(TRUE);
	    
  //set type to original type after if has been changed to BigNum above
  tfm->getValueId().changeType(&getValueId().getType());
  tfm = new HEAP Cast(tfm, &getValueId().getType());
  
  if( !isOLAP() )
  {
    //applying NotCovered to rank only for now to allow predicates to be pushed down 
    //in the case of a semi join
    //Gen web Case ID : 10-080219-4401
    tfm = new HEAP NotCovered(tfm);
    tfm->synthTypeAndValueId(); 
  }

  return tfm;
}

void ItmSeqOlapFunction::transformNode(NormWA & normWARef,
                                          ExprValueId & locationOfPointerToMe,
                                          ExprGroupId & introduceSemiJoinHere,
                                          const ValueIdSet & externalInputs)
{
  ValueId cacheEquivTransSeqId;
  ItemExpr * thisItem = locationOfPointerToMe.getPtr();
  if (CmpCommon::getDefault(COMP_BOOL_201) == DF_ON){
    if (normWARef.findEquivalentInSeqFunctionsCache( thisItem, cacheEquivTransSeqId)) {
      locationOfPointerToMe = cacheEquivTransSeqId;
      return;
    }
  }
  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  // ---------------------------------------------------------------------
  // Normalize the operands of the Running Function
  // ---------------------------------------------------------------------
  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);

  OperatorTypeEnum op = getOperatorType();

  ItemExpr *tfm = NULL;

  switch (getOperatorType())
  {
    case ITM_OLAP_VARIANCE:
    case ITM_OLAP_SDEV:
      tfm = transformOlapVariance(normWARef.wHeap());
      break;
    case ITM_OLAP_AVG:
      tfm = transformOlapAvg(normWARef.wHeap());
      break;
    default:
      break;
  }

  if (tfm)
  {
    getValueId().replaceItemExpr(tfm);
    tfm->synthTypeAndValueId(TRUE);
    locationOfPointerToMe = tfm;

    // Make sure the new nodes are transformed
    locationOfPointerToMe->transformNode(normWARef, locationOfPointerToMe,
                                   introduceSemiJoinHere, externalInputs);

    // Set the replacement expression
    setReplacementExpr(locationOfPointerToMe);
   
     if (CmpCommon::getDefault(COMP_BOOL_201) == DF_ON)
    {
      normWARef.insertIntoSeqFunctionsCache(thisItem, locationOfPointerToMe.getValueId());
    }
}


} // ItmSeqOlapFunction::transformNode()



///----------------

ItemExpr * ItmSeqOlapFunction::transformOlapVariance(CollHeap *wHeap)
{
  const NAType *desiredType    = new (wHeap) SQLDoublePrecision(TRUE);
  ItemExpr *childDouble        = new (wHeap) Cast(child(0), desiredType);
  ItemExpr *childDoubleSquared = new (wHeap) BiArith(ITM_TIMES,                       // x * x
                                                childDouble,
                                                childDouble);

  ItemExpr *sumOfValSquared    = new (wHeap) ItmSeqOlapFunction                    
                                         (ITM_OLAP_SUM, childDoubleSquared);
  ((ItmSeqOlapFunction *)sumOfValSquared)->setIsOLAP(isOLAP());
  ((ItmSeqOlapFunction *)sumOfValSquared)->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
  ((ItmSeqOlapFunction *)sumOfValSquared)->setOlapWindowFrame(frameStart_, frameEnd_);

  ItemExpr *sumOfVal           = new (wHeap) ItmSeqOlapFunction                    // RUNNINGSUM (x)
                                         (ITM_OLAP_SUM, childDouble);
  ((ItmSeqOlapFunction *)sumOfVal)->setIsOLAP(isOLAP());
  ((ItmSeqOlapFunction *)sumOfVal)->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
  ((ItmSeqOlapFunction *)sumOfVal)->setOlapWindowFrame(frameStart_, frameEnd_);

  ItemExpr *const1             = new (wHeap) ConstValue(1);                           // constant value 1

  ItemExpr *countOfVal         = new (wHeap) ItmSeqOlapFunction                    // 
                                          (ITM_OLAP_SUM, const1);
  ((ItmSeqOlapFunction *)countOfVal)->setIsOLAP(isOLAP());
  ((ItmSeqOlapFunction *)countOfVal)->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
  ((ItmSeqOlapFunction *)countOfVal)->setOlapWindowFrame(frameStart_, frameEnd_);

  ItemExpr *castDouble        = new (wHeap) Cast(countOfVal, desiredType);

  OperatorTypeEnum newOp;

  if (getOperatorType() == ITM_OLAP_VARIANCE)
  {
   newOp = ITM_VARIANCE;
  }
  else
  {
   CMPASSERT (getOperatorType() == ITM_OLAP_SDEV);
   newOp = ITM_STDDEV;
  }
  ItemExpr *result = new (wHeap)
                        ScalarVariance(newOp,
                                        sumOfValSquared,
                                        sumOfVal,
                                        castDouble) ;

  return result;
}

ItemExpr * ItmSeqOlapFunction::transformOlapAvg(CollHeap *wHeap)
{
  ItemExpr *newSum      = new (wHeap) ItmSeqOlapFunction (ITM_OLAP_SUM, child(0)); 
  ((ItmSeqOlapFunction *)newSum)->setIsOLAP(isOLAP());/// may need to chnage this behavior
  ((ItmSeqOlapFunction *)newSum)->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
  ((ItmSeqOlapFunction *)newSum)->setOlapWindowFrame(frameStart_, frameEnd_);

  ItemExpr *newCount    = new (wHeap) ItmSeqOlapFunction (ITM_OLAP_COUNT, child(0));
  ((ItmSeqOlapFunction *)newCount)->setIsOLAP(isOLAP());
  ((ItmSeqOlapFunction *)newCount)->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
  ((ItmSeqOlapFunction *)newCount)->setOlapWindowFrame(frameStart_, frameEnd_);

  ItemExpr *tfm         = new (wHeap) BiArith(ITM_DIVIDE,
                                           newSum   ,
                                           newCount);
  tfm = new (wHeap) Cast(tfm, &getValueId().getType());

  return tfm;
}
// transform olap rank function --
// function called at precode gen time
ItemExpr * ItmSeqOlapFunction::transformOlapRank(CollHeap *wHeap)
{
  ItemExpr *tfm = NULL;

  ItemExpr *rcStar = new (wHeap) ItmSeqRunningFunction(ITM_RUNNING_COUNT, 
                                                    new (wHeap) SystemLiteral(1));
  ((ItmSeqRunningFunction *)rcStar)->setIsOLAP(isOLAP());

  ItemExpr *change = new (wHeap) ItmSeqRunningFunction(ITM_RUNNING_CHANGE, 
                                                    child(0));
  ((ItmSeqRunningFunction *)change)->setIsOLAP(isOLAP());

  tfm = new (wHeap) BiArith(ITM_MINUS, rcStar, change);

  tfm->synthTypeAndValueId(TRUE);
	    
  //set type to original type after if has been changed to BigNum above
  tfm->getValueId().changeType(&getValueId().getType());

  tfm = new (wHeap) BiArith(ITM_PLUS, tfm, new (wHeap) SystemLiteral(1));

  tfm->synthTypeAndValueId(TRUE);
	    
  //set type to original type after if has been changed to BigNum above
  tfm->getValueId().changeType(getValueId().getType().newCopy(wHeap));
  tfm = new (wHeap) Cast(tfm, getValueId().getType().newCopy(wHeap));

  return tfm;
}

ItemExpr * ItmSeqOlapFunction::transformOlapDRank(CollHeap *wHeap)
{
  ItemExpr *change
    = new (wHeap) ItmSeqRunningFunction(ITM_RUNNING_CHANGE, 
                                    child(0));
  ((ItmSeqRunningFunction *)change)->setIsOLAP(isOLAP());
  
  ItemExpr *pred = new (wHeap) BiRelat(ITM_EQUAL,
                                   change,
                                   new (wHeap) SystemLiteral(1));

  ItemExpr *tfm = new (wHeap) 
          IfThenElse( pred, 
                      new (wHeap) SystemLiteral(1),
                      new (wHeap) SystemLiteral(0));                      

  tfm = new (wHeap) Case(NULL, tfm);

  tfm = new (wHeap) ItmSeqRunningFunction(ITM_RUNNING_SUM, tfm);
  ((ItmSeqRunningFunction *)tfm)->setIsOLAP(isOLAP());
  tfm = new (wHeap) Cast(tfm, getValueId().getType().newCopy(wHeap));

  return tfm;
}


ItemExpr *ItmSeqOlapFunction::transformOlapFunction(CollHeap *heap)
{

  // KB -- later -order by and partition maybe need to remove them from itsSequenceFunction
  // and put them in ItmSeqOlapFunction??

  if (getOperatorType() == ITM_OLAP_RANK)
  {
    return transformOlapRank(heap);
  }
  else
  if (getOperatorType() == ITM_OLAP_DRANK)
  {
    return transformOlapDRank(heap);
  }


  ItemExpr  * tfm = NULL;

  //-- KB -- ALL WINDOW FRAMES 
  //-- ROWS BETWEEN  UNBOUNDED PRECEDING  AND  BOUNDED PRECEDING
  //-- ROWS BETWEEN  UNBOUNDED PRECEDING  AND  CURRENT ROW
  //-- ROWS BETWEEN  UNBOUNDED PRECEDING  AND  UNBOUNDED FOLLOWING
  //-- ROWS BETWEEN  UNBOUNDED PRECEDING  AND  BOUNDED FOLLOWING
  //-- ROWS BETWEEN  CURRENT ROW          AND  CURRENT ROW
  //-- ROWS BETWEEN  CURRENT ROW          AND  UNBOUNDED FOLLOWING
  //-- ROWS BETWEEN  CURRENT ROW          AND  BOUNDED FOLLOWING
  //-- ROWS BETWEEN  BOUNDED PRECEDING    AND  BOUNDED PRECEDING
  //-- ROWS BETWEEN  BOUNDED PRECEDING    AND  CURRENT ROW
  //-- ROWS BETWEEN  BOUNDED PRECEDING    AND  UNBOUNDED FOLLOWING
  //-- ROWS BETWEEN  BOUNDED PRECEDING    AND  BOUNDED FOLLOWING
  //-- ROWS BETWEEN  BOUNDED FOLLOWING    AND  UNBOUNDED FOLLOWING
  //-- ROWS BETWEEN  BOUNDED FOLLOWING    AND  BOUNDED FOLLOWING

  if (isFrameStartUnboundedPreceding() && //frameStart_ == -INT_MAX &&
      !isFrameEndUnboundedPreceding()  && //frameEnd_ != -INT_MAX && 
      !isFrameEndUnboundedFollowing())  //frameEnd_ != INT_MAX) 
  {
    // BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
    // BETWEEN UNBOUNDED PRECEDING AND BOUNDED PRECEDING
    // BETWEEN UNBOUNDED PRECEDING AND BOUNDED FOLLOWING
    //
    OperatorTypeEnum op = mapOperTypeToRunning();

    ItmSequenceFunction * seqFunc = new (heap)
                      ItmSeqRunningFunction(op, child(0));
    seqFunc->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());

    if (frameEnd_ !=0)
    {
      seqFunc = new (heap)
                ItmSeqOffset(seqFunc, -frameEnd_,FALSE,TRUE,INT_MAX);

      seqFunc->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
    }

   
    tfm =  seqFunc;

  }   
  else if (frameStart_ <= frameEnd_ &&
            !isFrameStartUnboundedPreceding() && //frameStart_ != -INT_MAX &&
            !isFrameEndUnboundedPreceding()) //frameEnd_ != -INT_MAX) 
  {
    //-- ROWS BETWEEN  CURRENT ROW          AND  CURRENT ROW
    //-- ROWS BETWEEN  CURRENT ROW          AND  BOUNDED FOLLOWING
    //-- ROWS BETWEEN  BOUNDED PRECEDING    AND  BOUNDED PRECEDING
    //-- ROWS BETWEEN  BOUNDED PRECEDING    AND  CURRENT ROW
    //-- ROWS BETWEEN  BOUNDED PRECEDING    AND  BOUNDED FOLLOWING
    //-- ROWS BETWEEN  BOUNDED FOLLOWING    AND  BOUNDED FOLLOWING
    //-- ROWS BETWEEN  CURRENT ROW          AND  UNBOUNDED FOLLOWING
    //-- ROWS BETWEEN  BOUNDED PRECEDING    AND  UNBOUNDED FOLLOWING
    //-- ROWS BETWEEN  BOUNDED FOLLOWING    AND  UNBOUNDED FOLLOWING
    // SUM(A) OVER(order by B ROWS BETWEEN UNBOUNDED PRECEDING
    //                                 AND UNBOUNDED FOLLOWING)
    // SUM(A) OVER(order by B ROWS BETWEEN 5 PRECEDING
    //                                 AND 10 FOLLOWING)

    //MIN/MAX handled in generator
    if (getOperatorType() == ITM_OLAP_MIN || 
        getOperatorType() == ITM_OLAP_MAX)
    {
      if (frameStart_ < 0 && frameEnd_>0)
      {
        OperatorTypeEnum op;
        if(getOperatorType() == ITM_OLAP_MIN) 
        {
          op = ITM_SCALAR_MIN;
        }
        else 
        {
          op = ITM_SCALAR_MAX;
        }

        ItmSeqOlapFunction *precFunc = new (heap)
                    ItmSeqOlapFunction(getOperatorType(), child(0));

        precFunc->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
        precFunc->setOlapWindowFrame(frameStart_, 0);

        ItmSeqOlapFunction *follFunc = new (heap)
                    ItmSeqOlapFunction(getOperatorType(),  child(0)); //off);

        follFunc->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
        follFunc->setOlapWindowFrame(0 , frameEnd_);

        ItemExpr * minmax = new(heap) 
                            ItmScalarMinMax(op, precFunc, follFunc);
        return minmax;
      }
      else
      {
        return this;
      }
    }

    OperatorTypeEnum op = mapOperTypeToRunning();

    Lng32 wSize= INT_MAX;

    if (!isFrameEndUnboundedFollowing()) //(frameEnd_ != INT_MAX)
    {
      wSize = frameEnd_ - frameStart_ + 1;
    }

    ItmSequenceFunction *newSeq = new (heap)
      ItmSeqRunningFunction (op, child(0));

    ItmSequenceFunction *newOffset1 = newSeq;
    
    if((-frameStart_ + 1) != 0) {
      newOffset1 = new (heap)
        ItmSeqOffset (newSeq, -frameStart_ + 1, FALSE, FALSE, wSize);
    }

    ItmSequenceFunction *newOffset2 = newSeq;

    if(-frameEnd_ != 0) {
      newOffset2 = new (heap)
        ItmSeqOffset (newSeq, -frameEnd_, FALSE, TRUE, wSize);
    }

    tfm = new (heap)
      BiArith(ITM_MINUS,
              newOffset2,
              newOffset1);

    newSeq->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
    newOffset1->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
    newOffset2->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());

    tfm->synthTypeAndValueId(TRUE);
    // change type to the sum to the original type of the olap function 
    // so that we don't promote the sum to a bignum type which may hurt
    // performance
    tfm->getValueId().changeType(&getValueId().getType()); 

  } else if (isFrameStartUnboundedPreceding()  && //frameStart_ == -INT_MAX &&
             isFrameEndUnboundedFollowing()) //frameEnd_ == INT_MAX) 
  {
    //-- ROWS BETWEEN  UNBOUNDED PRECEDING  AND  UNBOUNDED FOLLOWING
    // SUM(A) OVER(order by B ROWS BETWEEN UNBOUNDED PRECEDING
    //                                 AND UNBOUNDED FOLLOWING)

    OperatorTypeEnum op = mapOperTypeToRunning();

    ItmSequenceFunction *seqFunc = new (heap)
      ItmSeqRunningFunction(op, child(0));

    ItmSequenceFunction *seqOff = new (heap)
      ItmSeqOffset(seqFunc, -frameEnd_/*+1*/,FALSE,TRUE, INT_MAX);

    seqOff->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
    seqFunc->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
                                  
    tfm = seqOff;

  } 

  if (tfm)
  {
      
    if (getOperatorType()== ITM_OLAP_COUNT)
    {
      ItemExpr *constZero   = new (heap) ConstValue(0);
      ItemExpr *isnull   = new (heap) UnLogic(ITM_IS_NULL, tfm);
      ItemExpr *newITEExpr = new (heap) IfThenElse(isnull, constZero, tfm);
      tfm =  new (heap) Case(NULL, newITEExpr);

    }

    if (getOperatorType()== ITM_OLAP_SUM)
    {
      ItmSeqOlapFunction * olapCount = new (heap) ItmSeqOlapFunction(ITM_OLAP_COUNT, child(0));
      olapCount->setOLAPInfo(getOlapPartitionBy(), getOlapOrderBy());
      olapCount->setOlapWindowFrame(frameStart_, frameEnd_);
      
      olapCount->synthTypeAndValueId(TRUE); // valueId/type will be used later when we further transfornm this olap count function
      ItemExpr * itmExpr  = olapCount->transformOlapFunction(heap);

      ItemExpr *nullConst  = new (heap) ConstValue();
      ItemExpr *constZero   = new (heap) ConstValue(0);
      ItemExpr *newEqual   = new (heap) BiRelat(ITM_EQUAL, itmExpr, constZero);
      ItemExpr *newITEExpr = new (heap) IfThenElse(newEqual, nullConst, tfm);
      tfm =  new (heap) Case(NULL, newITEExpr);

    }

    return tfm;
  }

  return NULL;
}



///---------------
#pragma nowarn(1506)   // warning elimination
void ItmSeqRowsSince::transformNode(NormWA & normWARef,
                                 ExprValueId & locationOfPointerToMe,
                                 ExprGroupId & introduceSemiJoinHere,
                                 const ValueIdSet & externalInputs)
{
  DBGSETDBG( "TRANSFORM_DEBUG" );
  DBGIF(
     unp = "";
     unparse(unp);
     cerr << (Int32)getOperatorType() << " "
     << (Int32)getValueId() << " "
     << (void *)this << " "
     << unp << endl;
  );

  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  // --------------------------------------------------------------------
  //  Traverse the tree in order to locate all THIS and NOT THIS branches.
  // --------------------------------------------------------------------
  transformNotTHISFunction();

  // Replace the search condition with the search boolean
  //
  child(0) = new HEAP UnLogic(ITM_IS_TRUE, child(0));
  child(0)->synthTypeAndValueId(TRUE);

  // ---------------------------------------------------------------------
  // Normalize the operands of the Moving Function
  // ---------------------------------------------------------------------
  BuiltinFunction::transformNode(normWARef, locationOfPointerToMe,
                                 introduceSemiJoinHere, externalInputs);
} // ItmSeqRowsSince::transformNode()
#pragma warn(1506)  // warning elimination

// ***********************************************************************
// $$$$ ItmSeqMovingFunction
// member functions for class ItmSeqMovingFunction
// ***********************************************************************
void ItmSeqMovingFunction::transformNode(NormWA & normWARef,
                                         ExprValueId & locationOfPointerToMe,
                                         ExprGroupId & introduceSemiJoinHere,
                                         const ValueIdSet & externalInputs)
{
  ValueId cacheEquivTransSeqId;
  ItemExpr * thisItem = locationOfPointerToMe.getPtr();
  if (CmpCommon::getDefault(COMP_BOOL_201) == DF_ON){
    if (normWARef.findEquivalentInSeqFunctionsCache( thisItem, cacheEquivTransSeqId)) {
      locationOfPointerToMe = cacheEquivTransSeqId;
      return;
    }
  }

  if (nodeIsTransformed())
    {
      locationOfPointerToMe = getReplacementExpr();
      return;
    }

  // ---------------------------------------------------------------------
  // Normalize the operands of the Moving Function
  // ---------------------------------------------------------------------
  ItemExpr::transformNode(normWARef, locationOfPointerToMe,
                          introduceSemiJoinHere, externalInputs);

  //
  // For all moving functions: replace children 1 and 2 with SCALAR_MIN (child(1), child(2))
  //
  //  Start:      ItmSeqMovingFunction             End:        ItmSeqMovingFunction
  //              /        |          \                        /                \
  //             x         y          z                       x              SCALAR_MIN (y, z)
  //
  if (getArity() == 3)
  {
   ItemExpr *newMin = new HEAP ItmScalarMinMax (ITM_SCALAR_MIN, child(1), child(2));
   newMin->synthTypeAndValueId(TRUE);
   child (1) = newMin;
   child (2) = NULL;
  }

  OperatorTypeEnum op = getOperatorType();
  ItemExpr *tfm = 0;

  switch (op) {
    case ITM_MOVING_VARIANCE:
    case ITM_MOVING_SDEV:
      tfm = transformMovingVariance();
      break;
    case ITM_MOVING_AVG:
      tfm = transformMovingAvg();
      break;
    case ITM_MOVING_SUM:
      tfm = transformMovingSum();
      break;
    case ITM_MOVING_COUNT:
      tfm = transformMovingCount();
      break;
    case ITM_MOVING_MIN:
    case ITM_MOVING_MAX:
      if (getSkipMovingMinMaxTransformation() == FALSE)
      {
        tfm = transformMovingMinMax();
      }
      break;
    case ITM_MOVING_RANK:
      {
        tfm = transformMovingRank();
      }
      break;
    case ITM_MOVING_DRANK:
      {
        ItemExpr *constOne = new HEAP SystemLiteral(1);
        ItemExpr *constZero = new HEAP SystemLiteral(0);

        ItemExpr *change
          = new HEAP ItmSeqRunningFunction(ITM_RUNNING_CHANGE, 
                                           child(0));
        ((ItmSeqRunningFunction *)change)->setIsOLAP(isOLAP());
        ItemExpr *pred = new HEAP BiRelat(ITM_EQUAL,
                                          change,
                                          constOne);

        ItemExpr *rowChanged = new HEAP
          IfThenElse(pred, constOne, constZero);

        rowChanged = new HEAP Case(NULL, rowChanged);

        ItemExpr *rsum = new HEAP ItmSeqRunningFunction(ITM_RUNNING_SUM, 
                                                        rowChanged);
        ((ItmSeqRunningFunction *)rsum)->setIsOLAP(isOLAP());

        // Win is one less than the window size.  It is used to access
        // the first row in the window, not the last row in the
        // previous window as do many other moving funcions.
        //
        ItemExpr *win = new HEAP BiArith(ITM_MINUS,
                                         child(1),
                                         constOne);

        win->synthTypeAndValueId(TRUE);
        win->getValueId().changeType(&getValueId().getType());

        ItemExpr *offRsum  = new HEAP ItmSeqOffset(rsum, win, NULL, TRUE);
        ((ItmSeqOffset *)offRsum)->setIsOLAP(isOLAP());

        ItemExpr *diff = new HEAP BiArith(ITM_MINUS,
                                          rsum,
                                          offRsum);

        diff->synthTypeAndValueId(TRUE);
        diff->getValueId().changeType(&getValueId().getType());

        diff = new HEAP BiArith(ITM_PLUS,
                               diff,
                               constOne);

        diff->synthTypeAndValueId(TRUE);
        diff->getValueId().changeType(&getValueId().getType());

        // If Win is zero, then this is a special case of a window
        // size of one (see construction of win above) in which case
        // M_DRANK is one
        pred = new HEAP BiRelat(ITM_LESS,
                                win,
                                constOne);

        tfm = new HEAP IfThenElse(pred, constOne, diff);

        tfm= new HEAP Case(NULL, tfm);

        tfm = new HEAP Cast(tfm, &getValueId().getType());

        break;
      }
    default:
      CMPASSERT(FALSE);
      break;
   }

  if (tfm)
  {
   tfm->synthTypeAndValueId(TRUE);
                                           // revert to original synthesized type
   ItemExpr *newCast = new HEAP Cast(tfm, &getValueId().getType());
   if( !isOLAP() )
   {
     //applying NotCovered to rank only for now to allow predicates to be pushed down 
     //in the case of a semi join
     //Gen web Case ID : 10-080219-4401
     if (op == ITM_MOVING_RANK) 
     {
       newCast = new HEAP NotCovered(newCast);
       newCast->synthTypeAndValueId();
     }
   }

   getValueId().replaceItemExpr(newCast);
   newCast->synthTypeAndValueId(TRUE);

   locationOfPointerToMe = newCast;

   // Make sure the new nodes are transformed
   locationOfPointerToMe->transformNode(normWARef, locationOfPointerToMe,
                                   introduceSemiJoinHere, externalInputs);

   // Set the replacement expression
   setReplacementExpr(locationOfPointerToMe);

   if (CmpCommon::getDefault(COMP_BOOL_201) == DF_ON){
    normWARef.insertIntoSeqFunctionsCache(thisItem, locationOfPointerToMe.getValueId());
   }
  }

} // ItmSeqMovingFunction::transformNode()

//------------------------------------------------------------------------------
// transformMovingMinMax
//
// Transforms MOVINGMIN/MAX (x, y) into
// MOVINGMINMAX(x, CASE WHEN (y < 0 OR y IS NULL) THEN DEF_MAX_HISTORY_ROWS ELSE y END)
//
//------------------------------------------------------------------------------
ItemExpr * ItmSeqMovingFunction::transformMovingMinMax()

{
  OperatorTypeEnum op = getOperatorType();

  CMPASSERT (op == ITM_MOVING_MIN  || op == ITM_MOVING_MAX);

  OperatorTypeEnum runningOp =
                  ((op == ITM_MOVING_MIN) ? ITM_RUNNING_MIN : ITM_RUNNING_MAX);

                                                        // new RUNNINGMIN/MAX(x)
  ItemExpr *newRunning  = new HEAP ItmSeqRunningFunction (runningOp, child(0));
  ((ItmSeqRunningFunction *)newRunning)->setIsOLAP(isOLAP());
  ItemExpr *constZero   = new HEAP ConstValue(0);
                                                              // new (y is NULL)
  ItemExpr *newIsNull   = new HEAP UnLogic (ITM_IS_NULL, child(1));
                                                                 // new ( y < 0)
  ItemExpr *newLessThan = new HEAP BiRelat(ITM_LESS, child(1), constZero);
                                                     // new (y < 0 OR y IS NULL)
  ItemExpr *newOr       = new HEAP BiLogic(ITM_OR, newIsNull, newLessThan);

  ItmSeqMovingFunction *newMovingMinMax = new HEAP ItmSeqMovingFunction(op,  child(0), child(1));
  ((ItmSeqMovingFunction *)newMovingMinMax)->setIsOLAP(isOLAP());
  newMovingMinMax->setSkipMovingMinMaxTransformation();

// new if (y < 0 OR y IS NULL) THEN RUNNINGMIN/MAX(x) ELSE MOVINGMIN/MAX(x, y)
  ItemExpr *newITEExpr = new HEAP IfThenElse(newOr, newRunning, newMovingMinMax);

  ItemExpr *tfm =  new HEAP Case(NULL, newITEExpr);
  ItemExpr *off = new HEAP ItmSeqOffset(child(0), 1);

  tfm = new HEAP ItmBlockFunction(off, tfm);

  return tfm;
}

//------------------------------------------------------------------------------------
//  transformMovingVariance
//
//  Transforms MOVINGVARIANCE(x) or MOVINGSDEV(x) into ScalarVariance:
//  == (MOVING_SUM(Xi*Xi) - MOVING_AVG(Xi)*MOVING_SUM(Xi)) / MOVINGSUM(1)
//  == (MOVING_SUM (x * x) - (MOVING_AVG(x) * MOVING_SUM(x)) / MOVINGSUM(1)
//
//  Thus, terms needed for Scalar Variance are:
//   1. MOVING_SUM (x * x) ==> "sumOfValSquared"
//   2. MOVING_SUM(x)      ==> "sumOfVal"
//   3. MOVING_COUNT(*)    == countofVal
//
//  All other terms can be derived from these.
//
// In ExFunctionSVariance::eval, the following calculations are performed (if VARIANCE):
//  avgOfVal = sumOfVal/countOfVal;
//
//  result = ( sumOfValSquared -  (2 * avgOfVal * sumOfVal) + (sumOfVal * avgOfVal)) / (countOfVal - 1);
//
// In ExFunctionSVariance::eval (if STDDEV)
//  result = sqrt ( sumOfValSquared -  (2 * avgOfVal * sumOfVal) + (sumOfVal * avgOfVal)) / (countOfVal - 1));
//--------------------------------------------------------------------------------------
//
ItemExpr * ItmSeqMovingFunction::transformMovingVariance()
{
  const NAType *desiredType    = new HEAP SQLDoublePrecision(TRUE);
  ItemExpr *childDouble        = new HEAP Cast(child(0), desiredType);
  ItemExpr *childDoubleSquared = new HEAP BiArith(ITM_TIMES,                       // x * x
                                                childDouble,
                                                childDouble);

  ItemExpr *sumOfValSquared    = new HEAP ItmSeqMovingFunction                    // MOVINGSUM ((x * x), size)
                                         (ITM_MOVING_SUM, childDoubleSquared, child(1));
  ((ItmSeqMovingFunction *)sumOfValSquared)->setIsOLAP(isOLAP());

  ItemExpr *sumOfVal           = new HEAP ItmSeqMovingFunction                    // MOVINGSUM (x, size)
                                         (ITM_MOVING_SUM,  childDouble, child(1));
  ((ItmSeqMovingFunction *)sumOfVal)->setIsOLAP(isOLAP());
  ItemExpr *const1             = new HEAP ConstValue(1);                           // constant value 1
  ItemExpr *countOfVal         = new HEAP ItmSeqMovingFunction                    // MOVINGCOUNT (*, size)
                                          (ITM_MOVING_SUM, const1, child(1));
  ((ItmSeqMovingFunction *)countOfVal)->setIsOLAP(isOLAP());

  ItemExpr *castDouble        = new HEAP Cast(countOfVal, desiredType);

  OperatorTypeEnum newOp;

  if (getOperatorType() == ITM_MOVING_VARIANCE)
  {
   newOp = ITM_VARIANCE;
  }
  else
  {
   CMPASSERT (getOperatorType() == ITM_MOVING_SDEV);
   newOp = ITM_STDDEV;
  }
  ItemExpr *result = new HEAP
                        ScalarVariance(newOp,
                                        sumOfValSquared,
                                        sumOfVal,
                                        castDouble) ;
  return result;
}
//------------------------------------------------------------------------------------
// transformMovingAvg
//
// transforms MOVINGAVG (x, y) into
//  MOVINGSUM(x) / MOVINGCOUNT(x)
//------------------------------------------------------------------------------------
//
ItemExpr * ItmSeqMovingFunction::transformMovingAvg()
{
  ItemExpr *newSum    = new HEAP ItmSeqMovingFunction(ITM_MOVING_SUM, child(0), child(1));// new MOVINGSUM(x, size);
  ((ItmSeqMovingFunction *)newSum)->setIsOLAP(isOLAP());
  ItemExpr *newCount  = new HEAP ItmSeqMovingFunction(ITM_MOVING_COUNT, child(0), child(1));   // new MOVINGCOUNT(x, size);
  ((ItmSeqMovingFunction *)newCount)->setIsOLAP(isOLAP());
  ItemExpr *tfm       = new HEAP BiArith(ITM_DIVIDE,                                        // MOVINGSUM(x) / MOVINGCOUNT(x)
                                           newSum   ,
                                           newCount);

  return tfm;
}
//------------------------------------------------------------------------------------
// transformMovingSum
//
// Transforms MOVINGSUM (x, y) into
//  RUNNINGSUM(x) - OFFSET(RUNNINGSUM(x), y)
//
// Unlike MOVINGCOUNT, the MOVINGSUM should return NULL, not 0, if
// (1) all the rows it processed were NULL or
// (2) the logical window size was effectively 0 or
// (3) both (1) and (2)
// Therefore, the above expression (newMinus) must be further transformed into:
// CASE (MOVINGCOUNT(x,y)) WHEN 0 THEN NULL ELSE newMinus END
//------------------------------------------------------------------------------------
ItemExpr * ItmSeqMovingFunction::transformMovingSum()
{
  ItemExpr *newSum      = new HEAP ItmSeqRunningFunction (ITM_RUNNING_SUM, child(0));
  ((ItmSeqRunningFunction *)newSum)->setIsOLAP(isOLAP());
  ItemExpr *newOffset   = new HEAP ItmSeqOffset (newSum, child(1), NULL, TRUE);
  ((ItmSeqOffset *)newOffset)->setIsOLAP(isOLAP());
  ItemExpr *tfm         = new HEAP BiArith(ITM_MINUS,
                                           newSum,
                                           newOffset);

  tfm->synthTypeAndValueId(TRUE);
  tfm->getValueId().changeType(&getValueId().getType());

  ItemExpr *newMovingCount = new HEAP ItmSeqMovingFunction(ITM_MOVING_COUNT, child(0), child(1));
  ((ItmSeqMovingFunction *)newMovingCount)->setIsOLAP(isOLAP());
  ItemExpr *nullConst  = new HEAP ConstValue();
  ItemExpr *constZero   = new HEAP ConstValue(0);
  ItemExpr *newEqual   = new HEAP BiRelat(ITM_EQUAL, newMovingCount, constZero);
  ItemExpr *newITEExpr = new HEAP IfThenElse(newEqual, nullConst, tfm);
  tfm =  new HEAP Case(NULL, newITEExpr);

  return tfm;
}
//------------------------------------------------------------------------------------
// transformMovingCount()
//
// Transforms MOVINGCOUNT as follows:
//   MOVINGCOUNT(x, y) = RUNNINGCOUNT(x) - REPLACENULL(OFFSET(RUNNGINGCOUNT(x),y))
//------------------------------------------------------------------------------------
ItemExpr *ItmSeqMovingFunction::transformMovingCount()
{
  ItemExpr *tfm  ;
  ItemExpr *newCount    = new HEAP ItmSeqRunningFunction (ITM_RUNNING_COUNT,child(0));  // new RUNNINGCOUNT(x)
  ((ItmSeqRunningFunction *)newCount)->setIsOLAP(isOLAP());
  ItemExpr *newOffset   = new HEAP ItmSeqOffset (newCount, child(1), NULL, TRUE);        // new OFFSET(RUNNINGCOUNT(x), y)
  ((ItmSeqOffset *)newOffset)->setIsOLAP(isOLAP());
  tfm                   = new HEAP BiArith(ITM_MINUS ,                                  // RUNNINGCOUNT(x) - OFFSET(RUNNINGCOUNT(x), y)
                                           newCount   ,
                                           newOffset);
  tfm->synthTypeAndValueId(TRUE);
  tfm->getValueId().changeType(&getValueId().getType());

  tfm = new HEAP Cast(tfm, &getValueId().getType());

  return tfm;
} // ItmSeqMovingFunction::transformMovingCount()

ItemExpr *ItmSeqMovingFunction::transformMovingRank()
{
  ItemExpr *tfm = NULL;

  NABoolean inv = FALSE;
  child(0) = child(0)->removeInverseFromExprTree(inv);
  if (inv && !(isTDFunction() || isOLAP()))
  {
    *CmpCommon::diags() << DgSqlCode(-4362);
    return this;
  } 

  if( !isOLAP() )
  {          
    ItemExpr * partOrdExpr = NULL;
    ItemExpr * partExpr = NULL;

    ItemExprList partExprList( child(1)->child(0), HEAP);
    ItemExprList ordExprList( child(0) , HEAP );

    CollIndex nc =(CollIndex) partExprList.entries();

    partOrdExpr = partExprList.usedEntry(nc-1);

    for (CollIndex i = nc-1 ; i > 0 ;i--)
    {
      partOrdExpr =  new(HEAP) ItemList( partExprList.usedEntry(i-1 ),  partOrdExpr);
      partOrdExpr->synthTypeAndValueId(TRUE); 
    }

    partExpr = partOrdExpr->copyTree(HEAP);
    partExpr->synthTypeAndValueId(TRUE); 

    for (CollIndex i = 0; (i < (CollIndex) ordExprList.entries());i++)
    {
      partOrdExpr =  new(HEAP) ItemList(ordExprList.usedEntry(i), partOrdExpr);
      partOrdExpr->synthTypeAndValueId(TRUE); 
    }

    ItemExpr *partOrdChange = new HEAP ItmSeqRunningFunction(ITM_RUNNING_CHANGE, partOrdExpr);

    ItemExpr *partChange = new HEAP ItmSeqRunningFunction(ITM_RUNNING_CHANGE, partExpr);

    tfm = new HEAP BiArith(ITM_MINUS, partChange, partOrdChange);

    tfm->synthTypeAndValueId(TRUE);
    tfm->getValueId().changeType(&getValueId().getType());

    tfm = new HEAP BiArith(ITM_PLUS, tfm, new HEAP SystemLiteral(1));

    tfm->synthTypeAndValueId(TRUE);
    tfm->getValueId().changeType(&getValueId().getType());
  }
  else
  {
    //RANK() OVER  (PARTITION BY a ORDER BY b) is transformed into
    //ROWS SINCE CHANGED(b) - ROWS SINCE CHANGED(a,b)
    ItemExpr * partOrdExpr = child(0);

    ExprValueId treePtr = child(1)->child(0);

    ItemExprTreeAsList changeValues(&treePtr, ITM_ITEM_LIST, RIGHT_LINEAR_TREE);

    CollIndex nc = changeValues.entries();

    for (CollIndex i = nc; i > 0; i--)
    {
      partOrdExpr = new(HEAP) ItemList(changeValues[i-1], partOrdExpr);
      partOrdExpr->synthTypeAndValueId(TRUE); 
    }

    ItemExpr *partOrdChange
      = new HEAP ItmSeqRunningFunction(ITM_RUNNING_CHANGE, partOrdExpr);

    ((ItmSeqRunningFunction *)partOrdChange)->setIsOLAP(isOLAP());
    
    tfm = new HEAP BiArith(ITM_MINUS, child(1), partOrdChange);

    tfm->synthTypeAndValueId(TRUE);
    tfm->getValueId().changeType(&getValueId().getType());

    tfm = new HEAP BiArith(ITM_PLUS, tfm, new HEAP SystemLiteral(1));
    
    tfm->synthTypeAndValueId(TRUE);
    tfm->getValueId().changeType(&getValueId().getType());

    tfm = new HEAP Cast(tfm, &getValueId().getType());          
  }

  return tfm;
} // ItmSeqMovingFunction::transformMovingRank()




//****************************************************************************
// The next three routines deal with OneRow aggregate transformation.
// See comments at ItemExpr::transformOneRowAggregate()
//****************************************************************************
static ItemExpr * createCastNodesonLeaves3( ItemExpr *tfm, NormWA & normref )
{
  if (tfm->getOperatorType() != ITM_ITEM_LIST)
  {
    CMPASSERT(CmpCommon::getDefault(COMP_BOOL_137) == DF_ON) ;
    NAType *outType = tfm->getValueId().getType().newCopy(normref.wHeap());
    outType->setNullable(TRUE);
    InstantiateNull * inst =
      new(normref.wHeap()) InstantiateNull(tfm , outType);
    inst->NoCheckforLeftToInnerJoin = TRUE;
    inst->synthTypeAndValueId(TRUE);
    return inst;
  }
  else
  {
    for (Int32 i=0; i<tfm->getArity(); i++)
    {
      tfm->child(i) = createCastNodesonLeaves3(
                        tfm->child(i)->castToItemExpr(), normref );
    }
    return tfm;
  }
}

static ItemExpr *createCastNodesonLeaves2( ItemExpr *tfm, NormWA & normref )
{
  if (tfm->getOperatorType() == ITM_ONE_ROW)
  {
     CMPASSERT( tfm->getArity() == 1); // either a cast or a list operator
     CMPASSERT( tfm->nodeIsBound() ); // this is not bound??

     ValueId exprId = tfm->getValueId();
     Aggregate *agr = (Aggregate *)(tfm->getReplacementExpr());

     // if this has been transformed before do not redo it.

     if (! agr->isOneRowTransformed_ )
       agr->child(0) =
          createCastNodesonLeaves3(tfm->child(0)->castToItemExpr(), normref);

     agr->synthTypeAndValueId(TRUE);
     exprId.replaceItemExpr( agr);
     agr->isOneRowTransformed_ = TRUE;
     return agr;
  }
  /*else if ( tfm->getOperatorType() == ITM_BASECOLUMN ) // veg problem
  {
    Cast *castNode = new(normref.wHeap()) Cast(tfm, &(tfm->getValueId().getType()));
    castNode->synthTypeAndValueId(TRUE);
    return castNode;
  }*/
  else
  {
    for (Int32 i=0; i<tfm->getArity(); i++)
    {
      tfm->child(i) = createCastNodesonLeaves2(
                                   tfm->child(i)->castToItemExpr(), normref );
    }
    return tfm;
  }
} //  static createCastNodesonLeaves2()


// This routine handles OneRow Transformation. It goes through the ItemExpr tree and
// looks for OneRow AggregateNodes and inserts an InstantiateNull node on top of
// the column(s) the OneRow aggregate is outputting. It uses static functions shown
// above to achieve this purpose.
ItemExpr* ItemExpr::transformOneRowAggregate( NormWA & normref )
{
  ItemExpr *tfm = this;
  tfm = createCastNodesonLeaves2( tfm,  normref );
  return tfm;

} // ItemExpr::transformOneRowAggregate()

// This routine removes OneRow aggregate nodes from the Itemexpression tree.
// The following transformation takes place:
// For some nodes A, B: A->OneRowNode()->B  ===> A->B
ItemExpr* ItemExpr::removeOneRowAggregate( ItemExpr *tfm, NormWA & normref )
{
  if ( ( tfm->getOperatorType() == ITM_IS_NULL ||
       tfm->getOperatorType() == ITM_IS_NOT_NULL ) &&
      ( tfm->child(0)->getOperatorType() == ITM_ONE_ROW ))
  {
    //////////////////////////////////////////////////////////////////////////
    // currently this logic handles the case of IS[NOT]NULL of OneRow(A LIST):
    // it replaces that with a conjuct of leaf nodes of one row aggregate.
    // This is experimental; I borrowed the code from UnLogic::transformIsNull()
    // This would be rewritten so as to reduce duplication of code.
    // -           (11/25/98)
    //
    // (a,b) IS NULL      ==>  a IS NULL AND b IS NULL
    // (a,b) IS NOT NULL  ==>  a IS NOT NULL AND b IS NOT NULL
    //////////////////////////////////////////////////////////////////////////

    CMPASSERT( tfm->child(0)->child(0).getPtr() );
    // ITM_ONE_ROW has at least one child

    // Convert the child subtree into list WITHOUT FLATTENING SUBQUERIES,
    // so they will be transformed to ONE_ROW aggregates.
    ItemExprList childList(tfm->child(0)->castToItemExpr()->child(0).getPtr(),
                           normref.wHeap(),
                           ITM_ITEM_LIST,
                           FALSE);

    ItemExpr * rootPtr = NULL;
    for (CollIndex i = 0; i < childList.entries(); i++)
    {
      ItemExpr * unLogPtr = new HEAP
                           UnLogic(tfm->getOperatorType(), childList[i]);
      if (!rootPtr)
        rootPtr = unLogPtr;
      else
        rootPtr = new HEAP BiLogic(ITM_AND, rootPtr, unLogPtr);
    }

    // replace the definition of this valueId
    tfm->getValueId().replaceItemExpr(rootPtr);

    // Synthesize its type
    rootPtr->synthTypeAndValueId(TRUE);
    return rootPtr;
  }
  else
  if (tfm->getOperatorType() == ITM_ONE_ROW)
  {
   CMPASSERT( tfm->getArity() == 1); // as of 11/25/98
                                     // this can only handle only one child

   if (tfm->child(0)->getOperatorType() != ITM_ITEM_LIST)
     tfm = tfm->child(0)->castToItemExpr();

   return tfm;
  }
  else
  {
    for (Int32 i=0; i<tfm->getArity(); i++)
    {
      tfm->child(i) = removeOneRowAggregate(
                          tfm->child(i)->castToItemExpr(), normref );
    }
  return tfm;
  }

} // ItemExpr::removeOneRowAggregate()

void BiRelat::translateListOfComparisonsIntoValueIds()
{
  if(!listOfComparisonExprs_)
    return;

  for (CollIndex i = 0; i < listOfComparisonExprs_->entries(); i++)
  {
    CMPASSERT((*listOfComparisonExprs_)[i]->nodeIsBound());
    ValueId comparisonValueId = (*listOfComparisonExprs_)[i]->getValueId();
  listOfComparisonExprIds_.insert(comparisonValueId);
  }
}

// -----------------------------------------------------------------------
// Each operator supports a (virtual) method for transforming a
// scalar expression to a canonical form
// -----------------------------------------------------------------------
void ValueIdProxy::transformNode(NormWA & normWARef,
                             ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
                             const ValueIdSet & externalInputs)
{
  if (nodeIsTransformed())
  {
     // Return the address of the expression that was used for replacing
     // this subquery in an earlier call.
     locationOfPointerToMe = getReplacementExpr();
     return;
  }

  // If we are proxying for the initial source for this ValueIdProxy, typically
  // a MVF, or a subquery with degree > 1, make sure we transfrom it too
  if (transformDerivedFromValueId_ == TRUE)
  {
     ItemExpr *expr = derivedFrom_.getItemExpr();
 
     NABoolean origInValueIdProxyFlag = normWARef.inValueIdProxy();
     normWARef.setInValueIdProxy(TRUE);
     expr->transformNode(normWARef,
                        locationOfPointerToMe,
                        introduceSemiJoinHere,
                        externalInputs);

     normWARef.setInValueIdProxy(origInValueIdProxyFlag);
  }

  // Set return value, effectively getting rid of the ValueIdproxy node.
  // this replaces the ValueIdproxy with the output it represents.
  // Even though locationOfPointerToMe got set above in the transform case
  // it will be contain the outputValueId_.getItemExpr(). So we make the
  // two cases common.

  locationOfPointerToMe = outputValueId_.getItemExpr();

  // Set the replacement expression
  // if the transformDerivedFromValueId_ flag was set, the transform above 
  // initializes locationOfPointerToMe to be the itemExpr 
  // of the first output/element in the select list of the UDF/Subquery.
  // otherwise, we just replaces itself with the output it represents.

  // Note that not ALL ValueIdProxy in a query will go away after transformation
  // Some will be there until the Normalization phase is complete. This since
  // not all ItemExprs transforms its children. ValueIdUnion is one such 
  // ItemExpr.
  setReplacementExpr(locationOfPointerToMe);
  markAsTransformed();
  return;


} // ValueIdProxy::transformNode()
