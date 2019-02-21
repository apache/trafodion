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
* File:         ItemExpr.C
* Description:  Item expressions (both physical and logical operators)
* Created:      5/17/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

#define  SQLPARSERGLOBALS_NADEFAULTS
#include "SqlParserGlobals.h"

#include "Platform.h"
#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "AllItemExpr.h"
#include "PartFunc.h"
#include "wstr.h"
#include "NLSConversion.h"
#include "Cost.h"          /* for lookups in the defaults table */
#include "Stats.h"
#include "exp_function.h" // for calling ExHDPHash::hash(data, len)
#include "ItemFuncUDF.h"
#include "CmpStatement.h"
#include "exp_datetime.h"

#include "OptRange.h"

#   include <limits.h>

#include <string.h>			// memcmp


// A constant to be used for allocating a buffer for getText()
#define TEXT_DISPLAY_LENGTH 1001

static NABoolean isQuantifiedComp(const ItemExpr *ie);

#define _strcmpi strcasecmp


// Initialize static members.
//THREAD_P OperatorTypeEnum ItemExpr::origOpTypeBeingBound_ = NO_OPERATOR_TYPE;
//THREAD_P Int32 ItemExpr::origOpTypeCounter_ = 0;

void ItemExpr::cleanupPerStatement()
{ 
  if (CURRENTSTMT)
     CURRENTSTMT->setItemExprOrigOpTypeBeingBound(NO_OPERATOR_TYPE);
}

// -----------------------------------------------------------------------
// methods for class ExprValueId
// -----------------------------------------------------------------------


// Ane ExprValueId can hold either an ItemExpr * or a ValueId.
// If it holds ValueId then the ItemExpr * is NULL.

ExprValueId::ExprValueId()
{
  exprMode_ = STANDALONE;
  exprPtr_  = NULL;
  exprId_   = NULL_VALUE_ID;
}

ExprValueId::ExprValueId(const ExprValueId &other)
{
  exprMode_ = other.exprMode_;
  exprId_   = other.exprId_;
  exprPtr_  = other.exprPtr_;

  if (exprId_ != NULL_VALUE_ID)
    exprPtr_  = NULL;
}

ExprValueId::ExprValueId(ItemExpr * exprPtr)
{
  exprMode_ = STANDALONE;
  exprPtr_  = exprPtr;
  if (exprPtr_ != NULL)
    exprId_   = exprPtr->getValueId();
  else
    exprId_   = NULL_VALUE_ID;

  if (exprId_ != NULL_VALUE_ID)
    exprPtr_  = NULL;
}

ExprValueId::ExprValueId(const ValueId & exprId)
{
  exprMode_ = STANDALONE;
  exprPtr_  = NULL;
  exprId_   = exprId;
}

ExprValueId & ExprValueId::operator= (const ExprValueId & other)
{
  CMPASSERT(exprMode_ != MEMOIZED);
  exprMode_ = other.exprMode_;
  exprPtr_ = other.exprPtr_;
  exprId_ = other.exprId_;

  if (exprId_ != NULL_VALUE_ID)
    exprPtr_  = NULL;
  return *this;
}

ExprValueId & ExprValueId::operator= (ItemExpr * other)
{
  CMPASSERT(exprMode_ != MEMOIZED);
  exprPtr_ = other;
  // update the value id as well (may be set to a NULL value id, though)
  if (exprPtr_ != NULL)
    exprId_ = exprPtr_->getValueId();
  else
    exprId_ = NULL_VALUE_ID;

  if (exprId_ != NULL_VALUE_ID)
    exprPtr_  = NULL;
  return *this;
}

ExprValueId & ExprValueId::operator= (const ValueId & other)
{
  CMPASSERT(exprMode_ != MEMOIZED);
  exprId_ = other;
  exprPtr_ = NULL;
  return *this;
}

NABoolean ExprValueId::operator== (const ExprValueId &other) const
{
  return (getPtr() == other.getPtr());       // ptrs must match
}

NABoolean ExprValueId::operator== (const ItemExpr *other) const
{
  return getPtr() == other;
}
NABoolean ExprValueId::operator== (const ValueId &other) const
{
  return (getValueId() == other);
}

ValueId ExprValueId::getValueId() const
{
  // make sure we return the value id that belongs to exprPtr_
  // (we might have initialized it with an ItemExpr * and later
  // added the value id to the ItemExpr without updating this object)
  if (exprId_ == NULL_VALUE_ID AND exprPtr_ != NULL)
    return exprPtr_->getValueId();
  else
    return exprId_;
}
void ExprValueId::convertToMemoized()
{
  // set this mode to prevent updates to it: a MEMOIZED object cannot
  // be assigned a new value
  exprMode_ = MEMOIZED;
}

void ExprValueId::convertToStandalone()
{
  exprMode_ = STANDALONE;
}

ItemExpr * ExprValueId::getPtr() const
{
  ValueId temp = exprId_;		// NSK platform needs to save this...?
  exprPtr_->checkInvalidObject(this);
  if (temp == NULL_VALUE_ID)
  {
    if (exprPtr_ == NULL || exprPtr_->getValueId() == NULL_VALUE_ID)
      return exprPtr_;
    return exprPtr_->getValueId().getItemExpr();
  }
  return exprId_.getItemExpr();
}

// -----------------------------------------------------------------------
// common member functions
// -----------------------------------------------------------------------

THREAD_P ObjectCounter (*ItemExpr::counter_)(0);

ItemExpr::ItemExpr(OperatorTypeEnum otype,
		   ItemExpr *child0,
		   ItemExpr *child1)
  : ExprNode(otype),
//  origOpType_(origOpTypeBeingBound() != NO_OPERATOR_TYPE ?
//		origOpTypeBeingBound() : otype),
    currChildNo_(0),
    clause_(NULL),
    collateClause_(NULL),
    previousHostVar_(FALSE),
    resolveIncompleteType_(FALSE),
    preCodeGenNATypeChange_(FALSE),
    selectivityFactor_(-1),
    flags_(0)
{
  child(0) = child0;
  child(1) = child1;
  replacementExpr_ = this;
  (*counter_).incrementCounter();

  origOpType_ = otype;

  CmpStatement* currStmt = CmpCommon::statement();
  if ( currStmt ) {
    OperatorTypeEnum x = currStmt->getItemExprOrigOpTypeBeingBound();

    if (x != NO_OPERATOR_TYPE)
      origOpType_ = x;
  }
}

ItemExpr::ItemExpr(const ItemExpr& s)
  : ExprNode(s)
  , valId_(s.valId_)
  , currChildNo_(s.currChildNo_)
  , collateClause_(s.collateClause_)
  , replacementExpr_(s.replacementExpr_)
  , clause_(s.clause_)
  , origOpType_(s.origOpType_)
  , previousHostVar_(s.previousHostVar_)
  , resolveIncompleteType_(s.resolveIncompleteType_)
  , previousName_(s.previousName_)
  , preCodeGenNATypeChange_(s.preCodeGenNATypeChange_)
  , selectivityFactor_(s.selectivityFactor_)
  , flags_(s.flags_)
{
  inputs_[0] = s.inputs_[0];
  inputs_[1] = s.inputs_[1];
  inputs_[2] = s.inputs_[2];
}

ItemExpr::~ItemExpr()
{
  // recursively delete all the children
  for (Lng32 i = 0; i < MAX_ITM_ARITY; i++)
    delete inputs_[i].getPtr();
  (*counter_).decrementCounter();
}

void ItemExpr::transformToRelExpr(NormWA & normWARef,
                            ExprValueId & locationOfPointerToMe,
                            ExprGroupId & introduceSemiJoinHere,
                            const ValueIdSet & externalInputs)
{
      // Do nothing
   locationOfPointerToMe = this;

}

// operator[] is used to access the children of a tree
ExprValueId & ItemExpr::operator[] (Lng32 index)
{
  CMPASSERT(index >= 0 AND index < MAX_ITM_ARITY);
  return inputs_[index];
}

const ExprValueId & ItemExpr::operator[] (Lng32 index) const
{
  CMPASSERT(index >= 0 AND index < MAX_ITM_ARITY);
  return inputs_[index];
}

NABoolean ItemExpr::operator== (const ItemExpr& other) const	// virtual meth
{
  return (getValueId() == other.getValueId());
}
void ItemExpr::deleteInstance()
{
  Int32 nc = getArity();
  for (Lng32 i = 0; i < (Lng32)nc; i++)
    inputs_[i] = NULL;
  delete this;
} // ItemExpr::deleteInstance()


void ItemExpr::setChild(Lng32 index, ExprNode * newChild)
{
  if (newChild)
    {
      CMPASSERT(newChild->castToItemExpr());
      child(index) = newChild->castToItemExpr();
    }
  else
    child(index) = (ItemExpr *)NULL;
} // ItemExpr::setChild()

void ItemExpr::allocValueId()
{
  ValueDesc *vdesc = new (CmpCommon::statementHeap()) ValueDesc(this);
  setValueId(vdesc->getValueId());
} // ItemExpr::allocValueId()

// Check whether the given ValueId is referenced in this ItemExpr.
NABoolean ItemExpr::referencesTheGivenValue(const ValueId & vid,
                                    NABoolean doNotDigInsideVegRefs ,
                                    NABoolean doNotDigInsideInstNulls) const
{
  // In really peculiar cases, a VEGRef might contain another VEGRef within
  // its VEG which recursively self-references the VEGRef. In order to prevent
  // running into an infinite loop there, we remember those VEG we've already
  // already seen by setting a flag in those VEG. If we run into that again,
  // there is no need to check the members of the VEG into it anymore since
  // those have already been or will be checked.
  //
  NABoolean retVal = FALSE;

  if (getValueId() == vid)
    return TRUE;

  ValueId exprId;
  switch (getOperatorType())
  {
    case ITM_VEG_PREDICATE:
    {
      VEG *veg = ((VEGPredicate *)this)->getVEG();
      if (veg->getVEGReference()->getValueId() == vid)
        return TRUE;
      else
      {
        // Already seen. Return FALSE. In VEGRef_x(VEG{VEGRef_x(..),y,z}),
        // the call to the inner VEGRef_x returns FALSE. However, if the
        // vid is VEGRef_x itself, when the function is called on the outer
        // VEGRef_x, it has return TRUE already. The previous "if" clause
        // ensures that. If vid is one of the other values in the VEG such
        // as y or z, ValueIdSet::referencesTheGivenValue() is going to call
        // us again on other members of the VEG. We will return TRUE with
        // the matching member; and ValueIdSet::referencesTheGivenValue()
        // is also going to return TRUE when we return TRUE on any one of
        // the members of the VEG.
        //
        if (veg->seenBefore())
          return FALSE;
        else
        {
          // Mark this VEG as already seen before going over its members,
          // so that if one of its members is a VEGRef to this VEG, we
          // don't have to traverse the VEG anymore.
          //
          veg->markAsSeenBefore();
          retVal = veg->getAllValues().referencesTheGivenValue(vid,exprId);
          veg->markAsNotSeenBefore();
        }
      }
      break;
    }
    case ITM_VEG_REFERENCE:
    {
      if (doNotDigInsideVegRefs)
        return FALSE;

      VEG *veg = ((VEGReference *)this)->getVEG();

      // Already seen. Return FALSE. In VEGRef_x(VEG{VEGRef_x(..),y,z}),
      // the call to the inner VEGRef_x returns FALSE. However, if the
      // vid is VEGRef_x itself, when the function is called on the outer
      // VEGRef_x, it has return TRUE already. The previous "if" clause
      // ensures that. If vid is one of the other values in the VEG such
      // as y or z, ValueIdSet::referencesTheGivenValue() is going to call
      // us again on other members of the VEG. We will return TRUE with
      // the matching member; and ValueIdSet::referencesTheGivenValue()
      // is also going to return TRUE when we return TRUE on any one of
      // the members of the VEG.
      //
      if (veg->seenBefore())
      {
        return FALSE;
      }
      else
      {
        // Mark this VEG as already seen before going over its members,
        // so that if one of its members is a VEGRef to this VEG, we
        // don't have to traverse the VEG anymore.
        //
        veg->markAsSeenBefore();
        retVal = veg->getAllValues().referencesTheGivenValue(vid,exprId);
        veg->markAsNotSeenBefore();
      }
      break;
    }
    case ITM_INSTANTIATE_NULL:
    {
      ValueId  nvid = getValueId();
      const ItemExpr * nie = nvid.getItemExpr();
      InstantiateNull *inst = (InstantiateNull *)nie->castToItemExpr();

      if (doNotDigInsideInstNulls)
        return FALSE;
      else
      {
        // Need to dig underneath the instantiate null to check if
        // the value we are looking for is the immediate child of
        // the instantiate null or contained in a veg reference
        // that is the child of the instantiate null.
        const ValueId & childVid = this->child(0)->getValueId();
        const ItemExpr * childExpr = childVid.getItemExpr();
        retVal =  childExpr->referencesTheGivenValue(vid);
      }
      break;
    } // end case instantiate null
    case ITM_ROWSETARRAY_SCAN:
    {
      retVal = FALSE;
      break;
    }
    default:
    {
      Lng32 nc = getArity();
      for (Lng32 i = 0; i < nc; i++)
      {
        if (child(i).getPtr())
        {
          if (child(i)->referencesTheGivenValue(vid))
          {
            retVal = TRUE;
            break;
          }
        }
      } // endfor

      // if this is a subquery, check any outer references of the subquery as
      // well as the children.
      //
      if ((NOT retVal) AND isASubquery())
      {
        // check if the given value is referenced in the required inputs of
        // the subquery
        //
        const ValueIdSet inputs = ((Subquery *)this)->getSubquery()->
                                   getGroupAttr()->getCharacteristicInputs();

        // Must not call internal function here, since we are checking on a
        // different set (inputs), not a subexpr within this node.
        //
        retVal = inputs.referencesTheGivenValue(vid,exprId);
      }
      break;
    } // default
  } // switch

  return retVal;
}




// Check whether this ItemExpr references any one value
// belonging to the given ValueidSet
NABoolean ItemExpr::referencesOneValueFrom(const ValueIdSet &vs) const
{
  for (ValueId id = vs.init(); vs.next(id); vs.advance(id))
    {
	  //check if the item expr is a non-strict constant
	  //a strict constant is somethine like cos(1)
	  //where as cos(?p) can be considered a constant
	  //in the non-strict definition since it remains
	  //constant for a given execution of a query
      if (!id.getItemExpr()->doesExprEvaluateToConstant(FALSE) )
         if (referencesTheGivenValue(id))
	    return TRUE;
    }
  return FALSE;
} // ItemExpr::referencesOneValueFrom()

// Check whether the given ValueId is contained in a VEGRef
NABoolean ItemExpr::containsTheGivenValue(const ValueId & vid) const
{
  // In really peculiar cases, a VEGRef might contain another VEGRef within
  // its VEG which recursively self-references the VEGRef. In order to prevent
  // running into an infinite loop there, we remember those VEG we've already
  // already seen by setting a flag in those VEG. If we run into that again,
  // there is no need to check the members of the VEG into it anymore since
  // those have already been or will be checked.
  //
  NABoolean retVal = FALSE;

  if (getValueId() == vid)
    return TRUE;

  switch (getOperatorType())
  {
    case ITM_VEG_PREDICATE:
    {
      VEG *veg = ((VEGPredicate *)this)->getVEG();
      if (veg->getVEGReference()->getValueId() == vid)
        return TRUE;
      else
      {
        // Already seen. Return FALSE. In VEGRef_x(VEG{VEGRef_x(..),y,z}),
        // the call to the inner VEGRef_x returns FALSE. However, if the
        // vid is VEGRef_x itself, when the function is called on the outer
        // VEGRef_x, it has return TRUE already. The previous "if" clause
        // ensures that. If vid is one of the other values in the VEG such
        // as y or z, ValueIdSet::containsTheGivenValue() is going to call
        // us again on other members of the VEG. We will return TRUE with
        // the matching member; and ValueIdSet::containsTheGivenValue()
        // is also going to return TRUE when we return TRUE on any one of
        // the members of the VEG.
        //
        if (veg->seenBefore())
          return FALSE;
        else
        {
          // Mark this VEG as already seen before going over its members,
          // so that if one of its members is a VEGRef to this VEG, we
          // don't have to traverse the VEG anymore.
          //
          veg->markAsSeenBefore();
          retVal = veg->getAllValues().containsTheGivenValue(vid);
          veg->markAsNotSeenBefore();
        }
      }
      break;
    }
    case ITM_VEG_REFERENCE:
    {
      VEG *veg = ((VEGReference *)this)->getVEG();

      // Already seen. Return FALSE. In VEGRef_x(VEG{VEGRef_x(..),y,z}),
      // the call to the inner VEGRef_x returns FALSE. However, if the
      // vid is VEGRef_x itself, when the function is called on the outer
      // VEGRef_x, it has return TRUE already. The previous "if" clause
      // ensures that. If vid is one of the other values in the VEG such
      // as y or z, ValueIdSet::containsTheGivenValue() is going to call
      // us again on other members of the VEG. We will return TRUE with
      // the matching member; and ValueIdSet::containsTheGivenValue()
      // is also going to return TRUE when we return TRUE on any one of
      // the members of the VEG.
      //
      if (veg->seenBefore())
      {
        return FALSE;
      }
      else
      {
        // Mark this VEG as already seen before going over its members,
        // so that if one of its members is a VEGRef to this VEG, we
        // don't have to traverse the VEG anymore.
        //
        veg->markAsSeenBefore();
        retVal = veg->getAllValues().containsTheGivenValue(vid);
        veg->markAsNotSeenBefore();
      }
      break;
    }
    case ITM_INSTANTIATE_NULL:
    {
      ValueId  nvid = getValueId();
      const ItemExpr * nie = nvid.getItemExpr();
      InstantiateNull *inst = (InstantiateNull *)nie->castToItemExpr();
      // Special code to handle instantiate nulls for subqueries.
      // If the instantiate null was introduced for a subquery, then the
      // "NoCheckForLeftToInnerJoin" flag was set to TRUE. If set, this
      // flag indicates that we don't need to check to see if we need to
      // transform the left join to an inner join, and thus possibly
      // remove the instantiate null, because the instantiate_null isn't
      // for a left join. If the flag is FALSE, however, it means the
      // instantiate null is for left join. In this case, we don't need
      // to execute this special code, so just break out now.
      if (NOT inst->NoCheckforLeftToInnerJoin)
      {
        break;
      }
      else
      {
        // Need to dig underneath the instantiate null to check if
        // the value we are looking for is the immediate child of
        // the instantiate null or contained in a veg reference
        // that is the child of the instantiate null.
        const ValueId & childVid = this->child(0)->getValueId();
        const ItemExpr * childExpr = childVid.getItemExpr();
        return childExpr->containsTheGivenValue(vid);

      }
    } // end case instantiate null
    default:
    {
      break;
    }
  } // switch

  return retVal;

} // ItemExpr::containsTheGivenValue()


// return the folded expression (if can be done). Return NULL otherwise
ItemExpr* ItemExpr::constFold() 
{
   ItemExpr* expr = NULL;

   try  {
      expr = copyTree(STMTHEAP);
   }

   catch (...) {
      return NULL;
   }

   expr->synthTypeAndValueId(TRUE, TRUE);

   expr = expr->simplifyBeforeConstFolding();
   //expr = simplifyBeforeConstFolding();

   if ( expr == NULL )
     expr = this;

   ValueIdList dummy; ValueId dummyId;
   ItemExpr* outExpr = NULL;
   Int32 error = dummy.evaluateExpr(dummyId, expr->getValueId(), -1,
                                    FALSE, //don't simplify expr
                                    TRUE,  // eval all consts
                                    &outExpr);

   if ( error == TRUE && outExpr ) 
       return outExpr;
   else 
       return NULL; 
}

ItemExpr* ItemExpr::simplifyBeforeConstFolding()
{
  Lng32 nc = getArity();

  if ( nc == 0 ) {
    return getConstantInVEG();
  }

  for (Int32 i = 0; i < nc; i++)
   {

      ValueId id = child(i)->getValueId();

      ItemExpr* cExpr = id.getItemExpr()->simplifyBeforeConstFolding();

      if ( cExpr )
        setChild(i, cExpr);
   }

   return this;
}

ItemExpr* ItemExpr::getConstantInVEG()
{
  switch (getOperatorType())
  {
     case ITM_VEG_REFERENCE:
      {
          const VEG * exprVEG = ((VEGReference*)this)->getVEG();
          const ValueIdSet & VEGGroup = exprVEG->getAllValues();

          ItemExpr *c = NULL;
          Lng32 cnt = 0;

          for (ValueId id = VEGGroup.init(); VEGGroup.next(id); VEGGroup.advance(id))
            {
              const ItemExpr * expr = id.getItemExpr();
              if (expr->getOperatorType() == ITM_CONSTANT)
                {
                  c = (ConstValue*)(id.getItemExpr());
                  cnt ++;
                }

            }
            return (cnt == 1) ? c : NULL;
       }

       default:
        break;
   }

   return NULL;
}

ItemExpr * ItemExpr::createMirrorPred(ItemExpr *compColPtr, 
                                      ItemExpr * compColExprPtr, 
                                      const ValueIdSet &underlyingCols)
{
   CMPASSERT(compColPtr->getOperatorType() == ITM_BASECOLUMN);
   BaseColumn *bcol = static_cast<BaseColumn *>(compColPtr);
   // use the basecolumn Veg, using the basecolumn by itself can cause issues
   // during codegen downstream
   ValueId egVid = bcol->getTableDesc()->getColumnVEGList()[bcol->getColNumber()];
   ItemExpr *compColVEGRefPtr = egVid.getItemExpr();

   switch (getOperatorType())
     {
     case ITM_VEG_PREDICATE:
       {
          ValueId refColId ;
          // XXX this should be a for loop to collect the each sub
          // expression containing each referenced column, mirror those
          // sub expressions and then final assembly of the complete
          // complete expression. The colde below works for simple expressions
          // like A=2 and B=4 when the computed col expr is POWER(A+B,2)
          // because of the veges formed.
          underlyingCols.getFirst(refColId); 
          const VEG * predVEG = ((VEGPredicate *) this)->getVEG();
          const ValueIdSet & VEGGroup = predVEG->getAllValues();
          if (VEGGroup.containsTheGivenValue(refColId) &&
              underlyingCols.entries() == 1)
            {
              ItemExpr * keyColExpr = predVEG->getVEGReference();
              ItemExpr * keyColRef = refColId.getItemExpr();

              ValueIdMap compExprRewriteMap;
              ValueId newCompExpr;
              compExprRewriteMap.addMapEntry(keyColExpr->getValueId(), keyColRef->getValueId());
              newCompExpr = compColExprPtr->mapAndRewrite(compExprRewriteMap);

              // Now try to const folding the newCompExpr by evaluating 
              // the expression. If successful, we use the computed result 
              // instead of newCompExpr. The simplification is necessary for 
              // the scan optimizer to figure out the actual number of rows 
              // returned per probe. 

              ItemExpr* foldedExpr = newCompExpr.getItemExpr()->constFold();

              ItemExpr * compPred = new(CmpCommon::statementHeap())
                                      BiRelat(ITM_EQUAL,
                                              compColVEGRefPtr,
                                              (foldedExpr==NULL) ? newCompExpr.getItemExpr() : foldedExpr);
              compPred->synthTypeAndValueId(TRUE);

              return compPred;
            }
          
          return NULL; 
          break;
       }
     case ITM_EQUAL:
     case ITM_LESS:
     case ITM_LESS_EQ:
     case ITM_LESS_OR_LE:
     case ITM_GREATER:
     case ITM_GREATER_EQ:
     case ITM_GREATER_OR_GE:
       {
          ItemExpr *keyColRef;
          ItemExpr *keyColExpr;
          ValueId refColId;
          // XXX figure out what it means when we have more than one
          // say CC expr is POWER(T.A+T.B,2)
          // If the keyPred is: T.A =2 AND T.B=4
          // we need to make sure we substitue both T.A and T.B in the power
          // expression above. 
          // The current code does the correct thing for simple veg expressions
          // with multiple cols even though it 
          // only process the first of the underlying cols, due to the veges 
          // formed. Should be a for loop instead
          // Also, at the moment have restricted the key predicate candidates
          // to mirror only to be equipreds when we have multiple columns 
          // referenced in the computedColumn expression.
          underlyingCols.getFirst(refColId); 
          NABoolean keyColOnLeft = TRUE;
          if (child(0)->referencesTheGivenValue(refColId) &&
              underlyingCols.entries() == 1)
            {
              keyColRef = child(0);
              keyColExpr = child(1);
            }
          else if (child(1)->referencesTheGivenValue(refColId) &&
                   underlyingCols.entries() == 1)
            {
              keyColRef = child(1);
              keyColExpr = child(0);
              keyColOnLeft = FALSE;
            }
          else
            { // XXX print warning for now.
#ifdef DEBUG
              fprintf(stderr, "ItemExpr::createMirrorPred(): Didn't find any references to valueId: %d\n", tempV);
#endif
              return NULL; 
            }
          // now we want to replace the reference to the key column in the compExpr with the keyColExpr

          ValueIdMap compExprRewriteMap;
          ValueId newCompExpr;
          compExprRewriteMap.addMapEntry(keyColExpr->getValueId(), keyColRef->getValueId());
          newCompExpr = compColExprPtr->mapAndRewrite(compExprRewriteMap);


          ItemExpr* foldedExpr = newCompExpr.getItemExpr()->constFold();
          ItemExpr* mirroredExpr = 
               (foldedExpr==NULL) ? newCompExpr.getItemExpr() : foldedExpr;

          ItemExpr * compPred;
          if (keyColOnLeft == TRUE)
              compPred = new(CmpCommon::statementHeap())
                BiRelat(((BiRelat *) this)->getRelaxedComparisonOpType(),
                                   compColVEGRefPtr,
                                   mirroredExpr //newCompExpr.getItemExpr()
                        );
          else
              compPred = new(CmpCommon::statementHeap())
                           BiRelat(((BiRelat *) this)->getRelaxedComparisonOpType(),
                                   mirroredExpr, //newCompExpr.getItemExpr(),
                                   compColVEGRefPtr);
          compPred->synthTypeAndValueId(TRUE);
          return compPred;
          break;
       }
     default:
       return NULL;
     }
   return NULL;
}

//Does 'this' ItemExpr evaluate to a constant?
//e.g.
//Sin(Cos(1)+1) will return TRUE.
//Sin(?p) will return FALSE.
NABoolean ItemExpr::doesExprEvaluateToConstant(NABoolean strict,
                                               NABoolean considerVEG) const
{

  NABoolean result = TRUE;
  Lng32 nc = getArity();

  //check if I am a leaf node
  if(!nc){
	//I am a leaf node
	//therefore check if I can be considered
	//a constant
	switch(getOperatorType()){

		case ITM_CONSTANT:
		case ITM_DEFAULT_SPECIFICATION:
		case ITM_PI:
		  //I am a strict constant, so return TRUE
		  return TRUE;

		case ITM_HOSTVAR:
                 {
                     HostVar* hv = (HostVar*)this;
                     if ( hv->isSystemGeneratedOutputHV() )
                        return TRUE;
                 }

		case ITM_DYN_PARAM:
		case ITM_CACHE_PARAM:
		case ITM_CURRENT_USER:
		case ITM_SESSION_USER:
		case ITM_CURRENT_TIMESTAMP:
		case ITM_UNIX_TIMESTAMP:
		case ITM_GET_TRIGGERS_STATUS:
		case ITM_UNIQUE_EXECUTE_ID:
		case ITM_CURR_TRANSID:
		  if(strict)
			//I am a non-strict constant, so return FALSE
			return FALSE;
		  //non-strict definition of constant
		  //therefore return TRUE
		  return TRUE;
    case ITM_VEG_REFERENCE:
      if(considerVEG)
      {
        const VEG * refVEG = ((VEGReference *)this)->getVEG();
        ValueId literal = NULL_VALUE_ID;

        if (strict)
        {
          literal = refVEG->getAConstant();
        }
        else{
          literal = refVEG->getAConstantHostVarOrParameter();
        }

        if (literal == NULL_VALUE_ID)
          return FALSE;

        return TRUE;
      }
      return FALSE;
		default:
		  return FALSE;
    }
  }

  //I am not a leaf node

  //check if I the random method
  //I will return different values
  //for the same parameter, so
  //I will never return a constant
  //value
  if(getOperatorType() == ITM_RANDOMNUM)
    return FALSE;

  // An aggrgate, sequence function  and instantiateNull itemExprs require state information (i.e. other rows)
  // to determine their value. Therefore they cannot be a constant. Also a rowset array scan 
  // denotes an array of values and is therefore not a constant.
  if(isAnAggregate()|| isASequenceFunction () ||
      (getOperatorType() == ITM_INSTANTIATE_NULL) || 
      (getOperatorType() == ITM_ROWSETARRAY_SCAN) ) 
  {
    return FALSE;
  }

  //I am not constant if anyone of my children
  //is not constant
  //check if all my children can be considered constants
  for (Lng32 i = 0; i < nc; i++)
  {
    result = child(i)->doesExprEvaluateToConstant(strict,considerVEG);

    //check if this child is not a constant
    if(!result){
	  //this child is not a constant, so return FALSE
      return result;
    }

  }

  //none of my children is a non-constant, so return TRUE
  return result;

}//ItemExpr::doesExprEvaluateToConstant()

NABoolean ItemExpr::referencesAHostVar() const
{
  NABoolean result = FALSE;
  Lng32 nc = getArity();

  //check if I am a leaf node
  if(!nc)
  {
    //I am a leaf node
    //therefore check if I can be considered
    //a constant
    switch(getOperatorType())
    {
      case ITM_HOSTVAR:
      case ITM_DYN_PARAM:
      case ITM_CURRENT_USER:
      case ITM_SESSION_USER:
      case ITM_CURRENT_TIMESTAMP:
      case ITM_UNIX_TIMESTAMP:
      case ITM_GET_TRIGGERS_STATUS:
      case ITM_UNIQUE_EXECUTE_ID:
      case ITM_CURR_TRANSID:
	return TRUE;

      case ITM_VEG_PREDICATE:
      {
        VEG * predVEG = ((VEGPredicate *)this)->getVEG();
        // Now, get all members of the VEG group, only if this VEG has not
        // been seen before. This is to avoid infinite looping for cases
        // where a VEG references itself
        if (predVEG->seenBefore())
          return FALSE;

        // get all members of the VEG
        const ValueIdSet & VEGGroup = predVEG->getAllValues();
        for (ValueId id = VEGGroup.init(); VEGGroup.next(id); VEGGroup.advance(id))
        {
          predVEG->markAsSeenBefore();
          NABoolean isAHostVar = id.getItemExpr()->referencesAHostVar();
          predVEG->markAsNotSeenBefore();

          if (isAHostVar)
            return TRUE;
        }
        return FALSE;
      } // end Case ITM_VEG_PREDICATE
    } // end switch
  } // end nc == 0

  for (Lng32 i = 0; i < nc; i++)
  {
    if(this->child(i)->referencesAHostVar())
      return TRUE;
  }
  return FALSE;
}

// This method returns TRUE or FALSE depending whether the optimizer
// should use stats to compute selectivity or use default selectivity
// Return TRUE if the expression is a VEG reference, base column,
// index column, column with CAST / UPPER / LOWER / UCASE / 
// LCASE / UPSHIFT / TRIM / SUBSTR (sometimes)
//
// Note that in the case of functions such as CAST and UPPER,
// the stats returned are those of the argument. For a right TRIM
// where we are trimming blanks, this is correct but for the 
// others the resulting stats will be incorrect in some way.
// For example, UPPER maps characters to upper case, so the UECs
// of the result should often be lower than the original, and
// certain intervals (namely those encompassing values that begin
// with a lower case letter) should be empty. For another
// example, CAST often is a 1-to-1 transformation so the UECs
// are right, but the order might change (e.g. when casting 
// numerics to characters; 99 < 100, but '99' > '100'). Even
// so, the statistics of the argument are still a better reflection
// of the result of the function than the default distribution,
// particularly from the standpoint of skew. So, we use them.
NABoolean ItemExpr::useStatsForPred()
{
  OperatorTypeEnum myType = getOperatorType();

  if (myType == ITM_VEG_REFERENCE OR
	  myType == ITM_BASECOLUMN OR
	  myType == ITM_VEG_PREDICATE OR
	  myType == ITM_INDEXCOLUMN OR
	  myType == ITM_VALUEIDUNION OR
	  myType == ITM_CAST OR
	  myType == ITM_INSTANTIATE_NULL OR
	  myType == ITM_UNPACKCOL OR
	  myType == ITM_UPPER OR
	  myType == ITM_LOWER OR
	  myType == ITM_TRIM OR
	  myType == ITM_CONVERT)

	return TRUE;
  else if (myType == ITM_SUBSTR)
    {
      // if the substring is known to be a prefix of the string,
      // then use the stats
      ItemExpr * startPosition = child(1);
      if (startPosition->getOperatorType() == ITM_CONSTANT)
        {
          NABoolean negate = FALSE;
          ConstValue * c = startPosition->castToConstValue(negate);
          if (c->canGetExactNumericValue() &&
               (c->getExactNumericValue() == 1))
            return TRUE;
        }
      return FALSE;  
    }
  else
	return FALSE;
}

void ItemExpr::getLeafValueIds(ValueIdSet &lv) const
{
  Int32 nc = getArity();

  // if this is a leaf node, add its value id
  // UDFunction uses the getArity() function to return the number
  // of inputs. It is really a leaf value...
  if (nc == 0 || getOperatorType() == ITM_USER_DEF_FUNCTION)
    {
      lv += getValueId();
    }
  else
    {
      // else add the leaf value ids of all the children
      for (Lng32 i = 0; i < (Lng32)nc; i++)
	{
	  child(i)->getLeafValueIds(lv);
	}
    }
}

void ItemExpr::getLeafValueIds(ValueIdList &lv) const
{
  Int32 nc = getArity();

  // if this is a leaf node, add its value id
  // UDFunction uses the getArity() function to return the number
  // of inputs. It is really a leaf value...
  if (nc == 0 || getOperatorType() == ITM_USER_DEF_FUNCTION)
    {
      lv.insert(getValueId());
    }
  else
    {
      // else add the leaf value ids of all the children
      for (Lng32 i = 0; i < (Lng32)nc; i++)
	{
	  child(i)->getLeafValueIds(lv);
	}
    }
}

void ItemExpr::getLeafValueIdsForCaseExpr(ValueIdSet &lv) 
{
  Int32 nc = getArity();

  // if this is a leaf node, add its value id
  if (nc == 0)
    {
	  if (doesExprEvaluateToConstant(FALSE))
		  lv += getValueId();
	  else
	  {
        ValueIdSet baseColSet;
	    findAll(ITM_BASECOLUMN, baseColSet, TRUE, TRUE);
        lv.addSet(baseColSet);
	  }
    }
  else
    {
      // elseadd the leaf value ids of all the children
		if (getOperatorType() == ITM_IF_THEN_ELSE)
		{
		  // for if_then_else statement ignore the first parameter which is the condition
		  child(1)->getLeafValueIdsForCaseExpr(lv);

		  // else part of Case can be a NULL pointer if it is created by Generator
		  if (child(2))
			child(2)->getLeafValueIdsForCaseExpr(lv);
		}
		else // for Case get the child, which would be IF_THEN_ELSE
		{
		  if (getOperatorType() == ITM_CASE)
			child(0)->getLeafValueIdsForCaseExpr(lv);
		  else // for all other expressions, collect the base columns
		  {
			ValueIdSet baseColSet;
			findAll(ITM_BASECOLUMN, baseColSet, TRUE, TRUE);
			lv.addSet(baseColSet);
		  }
		}
	}
}

// get leaf expression if the cardinality for this expression can be
// estimated using histograms
ItemExpr * ItemExpr::getLeafValueIfUseStats(NABoolean digIntoInstantiateNull)
{
  ItemExpr * expr = this;

  if (useStatsForPred())
  {
    if (digIntoInstantiateNull == FALSE && expr->getOperatorType() == ITM_INSTANTIATE_NULL)
      return expr;

    if(expr->getArity() > 0)
    {
      if (expr->getOperatorType() != ITM_TRIM)
            expr = expr->child(0)->getLeafValueIfUseStats(digIntoInstantiateNull);
      else
            expr = expr->child(1)->getLeafValueIfUseStats(digIntoInstantiateNull);
    }
  }

  return expr;
}
ItemExpr * ItemExpr::castToItemExpr()
{
  return this;
} // ItemExpr::castToItemExpr()

const ItemExpr * ItemExpr::castToItemExpr() const
{
  return this;
} // ItemExpr::castToItemExpr()

ConstValue * ItemExpr::castToConstValue(NABoolean & negate_it)
{
  negate_it = FALSE;
  return NULL;
}

SimpleHashValue ItemExpr::hash()
{
  // this method is just defined to have a hash method in ExprNode
  // without referencing class HashValue (which is too complicated
  // for the common code directory)
  return treeHash().getValue();
}

HashValue ItemExpr::topHash()
{

  HashValue result = 0;

  // hash on item properties???

  return result;
}

// this method is not virtual, since combining the hash values of the
// top node and its inputs should be independent of the actual node
HashValue ItemExpr::treeHash()
{
  HashValue result = topHash();
  Int32 maxc = getArity();

  for (Lng32 i = 0; i < (Lng32)maxc; i++)
    {
      // call this method recursively for the inputs
      result ^= child(i)->treeHash();
    }

  return result;
}

NABoolean ItemExpr::duplicateMatch(const ItemExpr & other) const
{
  // duplicateMatch works a little different in ItemExpr objects than it
  // does in RelExpr. In classes derived from RelExpr we strictly require
  // the duplicateMatch() method to be defined correctly. In classes
  // derived from ItemExpr this has not been done. Therefore, the
  // default method returns FALSE to be conservative (except
  // when the two expressions are the same).
  // The generic processing of duplicateMatch() for every derived class
  // is done in a separate method, genericDuplicateMatch().
  return (this == &other);
}

NABoolean ItemExpr::genericDuplicateMatch(const ItemExpr & other) const
{
  DCMPASSERT(other.castToItemExpr());

  if ((getValueId() == other.getValueId()) &&
      (getValueId() != NULL_VALUE_ID))
    return TRUE;
  if (getOperatorType() != other.getOperatorType())
    return FALSE;
  Int32 arity = getArity();
  if (arity != other.getArity())
    return FALSE;

  for (Lng32 i=0; i < (Lng32) arity; i++)
    {
      if (NOT (child(i)->duplicateMatch(*(other.child(i).getPtr()))))
	return FALSE;
    }
  return TRUE;
}

ItemExpr * ItemExpr::copyTopNode(ItemExpr *derivedNode,
				 CollHeap* )
{
  ItemExpr *result = NULL;

  if (derivedNode == NULL)
    {
      ABORT("encountered an instantiation of an ItemExpr object");
    }
  else
    result = derivedNode;

  // copy this's (the source's) original setting to result (the target)
  result->origOpType_ = origOpType_;

  result->selectivityFactor_ = selectivityFactor_;

  #ifndef NDEBUG
    CMPASSERT(!collateClause());	// else, Binder fell down on the job
  #endif				// (wrong overload of synthType() used)

  // copy/synthesize other item properties???

  return result;
}

// this method is not virtual, since combining the copies of the
// top node and its inputs should be independent of the actual node
ItemExpr * ItemExpr::copyTree(CollHeap* outHeap)
{
  ItemExpr * result = copyTopNode(NULL, outHeap);
  Int32 arity = getArity();

  for (Int32 i = 0; i < arity; i++) {
    if (child(i)) {
      result->child(i) = child(i)->copyTree(outHeap);
    }
  }

  return result;
}

NABoolean ItemExpr::containsSubquery()     // virtual method
{
  Int32 arity = getArity();

  for (Int32 i = 0; i < arity; i++)
    if (child(i) && child(i)->containsSubquery())
      return TRUE;

  return FALSE;
}

ItemExpr *ItemExpr::containsUDF()     // virtual method
{
  Int32 arity = getArity();

  for (Int32 i = 0; i < arity; i++)
    if (child(i) && child(i)->containsUDF())
      return child(i)->containsUDF();

  return 0;
}
NABoolean ItemExpr::containsIsolatedUDFunction()    // virtual method
{
  Int32 arity = getArity();

  for (Int32 i = 0; i < arity; i++)
    if (child(i) && child(i)->containsIsolatedUDFunction())
      return TRUE;

  return FALSE;
}

NABoolean ItemExpr::containsValueIdProxySibling(const ValueIdSet &siblings)
    // virtual method
{
  Int32 arity = getArity();

  for (Int32 i = 0; i < arity; i++)
    if (child(i) && child(i)->containsValueIdProxySibling(siblings))
      return TRUE;

  return FALSE;
} 

NABoolean ItemExpr::isASubquery() const   { return FALSE; }  // virtual method

NABoolean ItemExpr::isAnAggregate() const { return FALSE; }  // virtual method

NABoolean ItemExpr::isAPredicate() const  { return FALSE; }  // virtual method

NABoolean ItemExpr::isASequenceFunction() const { return FALSE; }   // virtual method

NABoolean ItemExpr::isOlapFunction() const { return FALSE; }   // virtual method


NABoolean ItemExpr::isAUserSuppliedInput() const { return FALSE; }  // virtual

ItemExpr * ItemExpr::transformMultiValuePredicate(	     // virtual method
					NABoolean /*flattenSubqueries*/,
					ChildCondition /*tfmIf*/)
{ return NULL; }

NABoolean ItemExpr::containsAnAggregate() const
{
  for (Int32 i=0; i<getArity(); i++)
  {
    if (child(i)->containsAnAggregate())
      return TRUE;
  }
  return FALSE;
}

// ----------------------------------------------------------------------
// Walk through an ItemExpr tree and gather the ValueIds of those
// expressions that behave as if they are "leaves" for the sake of
// the coverage test, e.g., expressions that have no children, or
// aggregate functions, or instantiate null. These are usually values
// that are produced in one "scope" and referenced above that "scope"
// in the dataflow tree for the query.
// ----------------------------------------------------------------------
void ItemExpr::getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const
{
  Lng32 nc = getArity();

  if ((nc == 0) 
    || coveringGA.isCharacteristicOutput(getValueId())
    || newExternalInputs.contains(getValueId()))
    {
      leafValues += getValueId();
    }
  else
    {
      for (Lng32 i = 0; i < nc; i++) {
	if (coveringGA.isCharacteristicOutput(child(i)->getValueId()) ||
	    newExternalInputs.contains(child(i)->getValueId()))
          leafValues += child(i)->getValueId();
        else
	  child(i)->getLeafValuesForCoverTest(leafValues, coveringGA, newExternalInputs);
      }
    }
} // ItemExpr::getLeafValuesForCoverTest()

// This is a recursive
// function that returns the leaf predicates of the tree
// whose root is the ItemExpr into the ValueIdSet&.
void ItemExpr::getLeafPredicates(ValueIdSet& leafPredicates)
{
  if (getOperatorType() != ITM_AND &&
      getOperatorType() != ITM_OR)
    leafPredicates.insert(getValueId());
  else {
    for (Lng32 i=0; i < (Lng32)getArity(); i++)
      child(i)->getLeafPredicates(leafPredicates);
  }
}// getLeafPredicates()

// -----------------------------------------------------------------------
// temp. method to collect basecolumns, used in FileScanRule
// -----------------------------------------------------------------------

template <class Result>
void ItemExpr::findAllT(OperatorTypeEnum wantedType,
		       Result& result,
		       NABoolean visitVEGMembers,
		       NABoolean visitIndexColDefs)
{
  OperatorTypeEnum myType = getOperatorType();

  if (myType == wantedType)
    result.addMember(this);

  // for VEGReferences and VEGPredicates (both having arity 0),
  // look at all the VEG members as well, if requested by the caller
  if (visitVEGMembers AND
      (myType == ITM_VEG_REFERENCE OR myType == ITM_VEG_PREDICATE))
    {
      VEG *veg;
      ValueId vegM;

      if (myType == ITM_VEG_REFERENCE)
	veg = ((VEGReference *) this)->getVEG();
      else
	veg = ((VEGPredicate *) this)->getVEG();

      if(! veg->seenBefore())
      {
	      veg->markAsSeenBefore();
	      for (ValueId x = veg->getAllValues().init();
			 veg->getAllValues().next(x);
			 veg->getAllValues().advance(x))

	      x.getItemExpr()->findAllT(wantedType,
			       result,
			       visitVEGMembers,
			       visitIndexColDefs);

	      veg->markAsNotSeenBefore();
      }
    }

  // for indexcolumns, look at the corresponding base column
  // or expression from the base table, if requested
  if (visitIndexColDefs AND myType == ITM_INDEXCOLUMN)
    {
      ((IndexColumn *) this)->getDefinition().getItemExpr()->
	findAllT(wantedType,
		result,
		visitVEGMembers,
		visitIndexColDefs);
    }

  // recurse
  Int32 nc = getArity();

  for (Lng32 i = 0; i < (Lng32)nc; i++)
    child(i)->findAllT(wantedType,
		      result,
		      visitVEGMembers,
		      visitIndexColDefs);

  if ( nc == 0 )
    switch ( myType )
      {
      case ITM_VALUEIDUNION:
	{
          // NB: ValueIdUnion objects can have more than 2 sources!
	  ValueIdUnion * tempUnion = (ValueIdUnion*) this;
          for (Lng32 i = 0; i < (Lng32)tempUnion->entries(); i++)
            {
              // guard against loops in the references
              // (can happen with common subexpressions, for example)
              if (!tempUnion->getSource(i).getItemExpr()->
                     referencesTheGivenValue(getValueId()))
                tempUnion->getSource(i).getItemExpr()->
                  findAllT(wantedType,
                           result,
                           visitVEGMembers,
                           visitIndexColDefs);
            }
	  break;
	}

      default:
	break;
      }

} // ItemExpr::findAllT, Template version

void ItemExpr::findAll(OperatorTypeEnum wantedType,
                       ValueIdSet & result,
                       NABoolean visitVEGMembers,
                       NABoolean visitIndexColDefs)
{
  findAllT(wantedType, result, visitVEGMembers, visitIndexColDefs);
}

void ItemExpr::findAll(OperatorTypeEnum wantedType,
		       ItemExprList& result,
		       NABoolean visitVEGMembers,
		       NABoolean visitIndexColDefs)
{
  // Collect all ItemExprs into a list.
  findAllT(wantedType, result, visitVEGMembers, visitIndexColDefs);
} // ItemExpr::findAll


Lng32 ItemExpr::getTreeSize(Lng32& maxDepth, NABoolean giveUpThreshold)
{
  Lng32 currentSize=1;
  Int32 nc = getArity();

  for (Lng32 i = 0; i < (Lng32)nc; i++)
  {
    Lng32 thisDepth = 0;
    currentSize += child(i)->getTreeSize(thisDepth, giveUpThreshold);

    if (giveUpThreshold > 0 && currentSize >= giveUpThreshold)
      break;

    if (thisDepth > maxDepth)
      maxDepth = thisDepth;
  }

  maxDepth++;
  return currentSize;
}

// Find all eqaulity columns in an item expression tree.
void ItemExpr::findEqualityCols(ValueIdSet& result)
{
  OperatorTypeEnum myType = getOperatorType();
  ValueId  myVid = getValueId();

  // If it is a base column or an index column, add to the list
  if (myType == ITM_BASECOLUMN || myType == ITM_INDEXCOLUMN)
    result += myVid;

  // If it belongs to VEG, add all the memebers of that VEG
  if (myType == ITM_VEG_REFERENCE || myType == ITM_VEG_PREDICATE)
  {
    VEG *veg;

    if (myType == ITM_VEG_REFERENCE)
      veg = ((VEGReference *) myVid.getItemExpr())->getVEG();
    else 
      // myType == ITM_VEG_PREDICATE
      veg = ((VEGPredicate *) myVid.getItemExpr())->getVEG();

    for (ValueId vid = veg->getAllValues().init();
         veg->getAllValues().next(vid);
         veg->getAllValues().advance(vid))
    {    
      result += vid; 
    }    
  }

  // recurse
  if (myType == ITM_EQUAL)
  {
    child(0)->findEqualityCols(result);
    child(1)->findEqualityCols(result);
  }
}

ItemExpr * ItemExpr::treeWalk(ItemTreeWalkFunc f,
                              CollHeap *outHeap,
                              enum ItemTreeWalkSeq sequence,
                              void *context)
{
  ItemExpr *transformedChildrenArr[4];
  ItemExpr **transformedChildren = transformedChildrenArr;
  Int32 arity = -1;
  NABoolean needToCopy = FALSE;
  ItemExpr *result = this;

  if (sequence == ITM_PREFIX_WALK)
    result = f(result, outHeap, context);

  arity = result->getArity();
  if (arity > 4)
    // allocated from stmt heap since it is used locally only
    // and is not deallocated below
    transformedChildren = new(CmpCommon::statementHeap()) ItemExpr *[getArity()];

  for (int i=0; i<arity; i++)
    {
      transformedChildren[i] =
        result->child(i)->treeWalk(f,
                                   outHeap,
                                   sequence,
                                   context);
      if (transformedChildren[i] != child(i))
        needToCopy = TRUE;
    }

  if (needToCopy)
    {
      // one of the children changed, since we can't
      // change an ItemExpr once we assigned a ValueId to
      // it, make a copy with new children
      result = result->copyTopNode(NULL, outHeap);
      for (int j=0; j<arity; j++)
        result->setChild(j, transformedChildren[j]);
    }

  if (sequence == ITM_POSTFIX_WALK)
    result = f(result, outHeap, context);
  
  return result;
}

ValueId ItemExpr::mapAndRewrite(ValueIdMap &map,
				NABoolean mapDownwards)
{
  ValueId result;

  // Look in the map for a match
  if (mapDownwards)
    map.mapValueIdDown(getValueId(),result);
  else
    map.mapValueIdUp(result,getValueId());

  // if this expression can be mapped directly, then return the
  // mapped value id
  if (result != getValueId())
    return result;

  return mapAndRewriteCommon(map, mapDownwards);
} // ItemExpr::mapAndRewrite

ValueId Aggregate::mapAndRewrite(ValueIdMap &map,
                                NABoolean mapDownwards)
{

  // Need to make sure we map and rewrite the distinctId as well.
  if (isDistinct())
    distinctId_ = distinctId_.getItemExpr()->mapAndRewrite(map, mapDownwards);

  return ItemExpr::mapAndRewrite(map, mapDownwards);
} // Aggregate::mapAndRewrite

ValueId ItemExpr::mapAndRewriteWithIndx(ValueIdMap &map,
					CollIndex ind)
{
  ValueId result;

  map.mapValueIdUpWithIndex(result,getValueId(), ind);

  // if this expression can be mapped directly, then return the
  // mapped value id
  if (result != getValueId())
    return result;

  return mapAndRewriteCommon(map, FALSE);
} // ItemExpr::mapAndRewriteWithIndex

// This method has the code that is common to
// mapAndRewrite() and mapAndRewriteWithIndex().
ValueId ItemExpr::mapAndRewriteCommon(ValueIdMap &map, NABoolean mapDownwards)
{
  ValueId result;

  ItemExpr *copyOfMe = NULL;
  Lng32 nc = getArity();

  // if the children of this expression changed after mapping, then
  // return the value id of a duplicate of myself with the new
  // children attached to it
  for (Lng32 i = 0; i < nc; i++)
    {
      ValueId c = child(i)->mapAndRewrite(map,mapDownwards);

      if (c != child(i)->getValueId())
	{
          // This child was mapped to a different value id
	  if (copyOfMe == NULL)
	    {
              // This was the first child to map to a different value
	      // make a copy of me and initialize its children with
	      // the same pointers that I have
	      copyOfMe = copyTopNode(NULL, CmpCommon::statementHeap());
              // copy the previous children into the new expression
	      for (Lng32 j = 0; j < nc; j++)
		copyOfMe->child(j) = child(j).getPtr();
	    }

	  copyOfMe->child(i) = c.getItemExpr();
	}
    }
  // If the expr had not child then copyOfMe is NULL

  if (copyOfMe)
    {
      // A new expression was created as the mapped value
      copyOfMe->synthTypeAndValueId();
      result = copyOfMe->getValueId();

      // Add to the map table the newly formed expression
      if (mapDownwards)
        map.addMapEntry(getValueId(),result);
      else
        map.addMapEntry(result,getValueId());
    }
  else
    // otherwise just return my value id
    result = getValueId();

  return result;
} // ItemExpr::mapAndRewriteCommon

ItemExpr * ItemExpr::foldConstants(ComDiagsArea *diagsArea,
				   NABoolean newTypeSynthesis)
{
  ItemExpr *result = this;
  Lng32 nc = getArity();

  // if the children of this expression changed after constant folding, then
  // return a duplicate of myself with the new children attached to it
  for (Lng32 i = 0; i < nc; i++)
    {
      ItemExpr *c = child(i)->foldConstants(diagsArea,newTypeSynthesis);

      if (c != child(i))
	{
          // This child was transformed into a different ItemExpr
	  if (result == this)
	    {
              // This was the first child to be transformed;
	      // make a copy of me and initialize its children with
	      // the same pointers that I have
	      result = copyTopNode(NULL, CmpCommon::statementHeap());
              // copy the previous children into the new expression
	      for (Lng32 j = 0; j < nc; j++)
		result->child(j) = child(j).getPtr();
	    }

	  result->child(i) = c;
	}
    }

  // make sure we return a tree with a correct type
  if (result != this)
    result->synthTypeAndValueId(newTypeSynthesis);

  return result;
}

ItemExpr * ItemExpr::applyInverseDistributivityLaw(
     OperatorTypeEnum backboneType,
     OperatorTypeEnum innerType)
{
  // operators other than OR will require some more code, see CMPASSERTs
  DCMPASSERT(backboneType == ITM_OR);

  // we must have a backbone of the backboneType for this to work
  if (getOperatorType() != backboneType)
    return this;

  // a default can switch this code off if required (should be needed
  // only if bugs are found in the code)
  if (CmpCommon::getDefault(FIND_COMMON_SUBEXPRS_IN_OR) != DF_ON)
    return this;

  ItemExpr *result = NULL;
  // NOTE: we'll use the words "disjuncts" and "conjuncts" that make
  // sense with the default operator types, but may not be a great choice
  // if the caller uses different types (sorry, but this might help
  // understand the code better)
  ValueIdSet disjuncts;
  ValueIdSet commonSubexprs;
  CollIndex numCommonSubexprs;
  ValueId c;

  // get a ValueIdSet with all the "disjuncts".
  convertToValueIdSet(disjuncts,NULL,backboneType,FALSE);

  // prime the common subexpressions with all of the conjuncts
  // of the first disjunct
  disjuncts.getFirst(c);
  c.getItemExpr()->convertToValueIdSet(commonSubexprs,NULL,innerType,FALSE);
  disjuncts.advance(c);

  // in a first loop, find the common subexpressions among all of the disjuncts
  for (/* c already initialized */; disjuncts.next(c); disjuncts.advance(c))
    {
      ValueIdSet conjuncts;

      c.getItemExpr()->convertToValueIdSet(conjuncts,NULL,innerType,FALSE);

      // find common subexpressions between the
      // previous and the current disjunct
      commonSubexprs.findCommonSubexpressions(conjuncts);

      // stop if we have no common subexpressions
      if (commonSubexprs.isEmpty())
	break;
    }

  numCommonSubexprs = commonSubexprs.entries();

  if (numCommonSubexprs > 0)
    {
      // Now that we know the real common subexpressions do the same loop
      // again, but this time modify the conjuncts by removing the
      // common parts from them, forming new conjuncts, and connecting
      // them to form a new backbone of disjuncts.

      for (ValueId c2 = disjuncts.init();
	   disjuncts.next(c2);
	   disjuncts.advance(c2))
	{
	  ValueIdSet conjuncts;

	  // one child of the backbone, eliminate common subexpressions
	  // from it
	  c2.getItemExpr()->convertToValueIdSet(conjuncts,
						NULL,
						innerType,
						FALSE);
	  commonSubexprs.findCommonSubexpressions(conjuncts, TRUE);

	  // make sure the common subexpressions haven't changed
	  CMPASSERT(numCommonSubexprs == commonSubexprs.entries());

	  // are there any non-common conjuncts left?
	  if (conjuncts.entries() == 0)
	    {
	      // No, one of the disjuncts consists of only the common
	      // subexpressions. This requires logic specific to the
	      // operator type. For example, (a=10 and b=11) or (a=10)
	      // is (a=10) and (b=11 or TRUE) which is (a=10).
	      CMPASSERT(backboneType == ITM_OR);
	      result = NULL;
	      // leave the inner loop with a NULL result, meaning
	      // that only the common subexpressions should survive
	      break;
	    }

	  // rebuild the ItemExpr tree for the conjuncts
	  ValueIdList conjunctList(conjuncts);
	  ItemExpr *newConjuncts = conjunctList.rebuildExprTree(innerType);

	  // rebuild the backbone
	  if (result == NULL)
	    result = newConjuncts;
	  else
	    result = connect2(backboneType,result,newConjuncts);
	}

      // create a tree with the common subexpressions
      ValueIdList commonSubexprsList(commonSubexprs);
      ItemExpr *commonSubexprsAsItem =
	commonSubexprsList.rebuildExprTree(innerType);

      // hook up common part and the non-common expressions (if any)
      if (result)
	result = connect2(innerType,commonSubexprsAsItem,result);
      else
	result = commonSubexprsAsItem;

      result->synthTypeAndValueId();
    }
  else
    {
      // sorry, no common subexpressions found, return original expression
      result = this;
    }

  return result;
}

// little helper for the above method
ItemExpr * ItemExpr::connect2(OperatorTypeEnum op,
			      ItemExpr *op1,
			      ItemExpr *op2)
{
  switch (op)
    {
    case ITM_AND:
    case ITM_OR:
      return new(CmpCommon::statementHeap()) BiLogic(op, op1, op2);
    default:
      CMPASSERT("Operator type not supported by connect2" == 0);
      return NULL;
    }
}

// -----------------------------------------------------------------------
// ItemExpr::synthTypeAndValueId()
// -----------------------------------------------------------------------

void ItemExpr::synthTypeAndValueId(NABoolean redriveTypeSynthesisFlag, NABoolean redriveChildTypeSynthesis)
{
  // If the redriveTypeSynthesisFlag is set, it synthesises iself.
  // If the redriveChildTypeSynthesis is set, it synthesizes the
  // types for the entire subtree, from the leaves upto this operator,
  // once again.

  if (nodeIsBound() AND NOT redriveTypeSynthesisFlag) return;

  Int32 nc = getArity();

  // do it recursively on the children
  for (Lng32 i = 0; i < (Lng32)nc; i++)
    {
      if (child(i))
	{
	  // If it is an Item Expressions
	  child(i)->synthTypeAndValueId(redriveChildTypeSynthesis, redriveChildTypeSynthesis);
	  // refer to the child by its valueid instead of its pointer
	  child(i) = child(i)->getValueId();
	}
      // else leave the RelExpr alone
    }

  // Now do the non-recursive part
  synthTypeAndValueId2( redriveTypeSynthesisFlag, redriveChildTypeSynthesis );
  
} // ItemExpr::synthTypeAndValueId()

//
// ItemExpr::synthTypeAndValueId2() - a helper routine for synthTypeAndValueId()
// NOTE: The code for this routine came from the previous version of
//       ItemExpr::synthTypeAndValueId().  It was pulled out as a separate
//       routine so that the C++ compiler would generate code for
//       ItemExpr::synthTypeAndValueId() that used significantly less stack space.
//
void ItemExpr::synthTypeAndValueId2(NABoolean redriveTypeSynthesisFlag, NABoolean redriveChildTypeSynthesis)
{
  Int32 nc = getArity();
  ValueDesc *vdesc;
  DomainDesc *ddesc;

  // make sure the expression has a value id
  if (valId_ == NULL_VALUE_ID)
    {
      vdesc = new (CmpCommon::statementHeap()) ValueDesc(this);
      // remember the value id and the type in the node
      setValueId(vdesc->getValueId());
    }
  else
    {
      vdesc = valId_.getValueDesc();
    }

  // if the type hasn't been set or if type synthesis is to be
  // redriven, set the type to the synthesized value
  ddesc = vdesc->getDomainDesc();

  if (ddesc == NULL OR (nc > 0 AND redriveTypeSynthesisFlag))
    {
      const NAType *t = synthesizeType();
      // changed from CMPASSERT to CMPABORT, because it could be null
      // for several reasons. In most cases it is because of User error
      // where, the diagnostics would be filled with the error number.
      // In such cases it is wrong to raise an unnecessary alarm. If the
      // diagnostics buffer is not filled, then raise an exception
      if (!t)
	CMPABORT;

      // Fix for "BR0094.txt", here and in Case::bindNode().
      // Special handling is required for CASE because its operand
      // is not one of its children (because the operand can be a subquery,
      // which the normalizer needs to move).  And when that operand IS a subq,
      // then the CASE can return NULL if the subq produces zero rows.
      if (getOperatorType() == ITM_CASE &&
          ((Case *)this)->caseOperandWasNullable())
	t = t->synthesizeNullableType(CmpCommon::statementHeap());

      if (ddesc == NULL)
	{
	  ddesc = new (CmpCommon::statementHeap())
	    DomainDesc(ActiveSchemaDB(), *t);
	  vdesc->setDomainDesc(ddesc);
	}
      else
	{
	  getValueId().changeType(t);
	}
    }

  // mark me as bound
  markAsBound();
} // ItemExpr::synthTypeAndValueId2()

// -----------------------------------------------------------------------
// ItemExpr::isCovered()
// -----------------------------------------------------------------------
NABoolean ItemExpr::isCovered
                      (const ValueIdSet& newExternalInputs,
		       const GroupAttributes& coveringGA,
		       ValueIdSet& referencedInputs,
		       ValueIdSet& coveredSubExpr,
		       ValueIdSet& unCoveredExpr) const
{

  NABoolean exprIsCovered;

  switch (getArity())
    {
    case 0:

      exprIsCovered = FALSE;
      break;

    default:
      {
        exprIsCovered = TRUE;
	NABoolean coverFlag;
	Lng32 index, nc = getArity();

	for (index = 0; index < nc; index++)
	  {
	    coverFlag = coveringGA.covers(child(index)->getValueId(),
					  newExternalInputs,
					  referencedInputs,
					  &coveredSubExpr,
					  &unCoveredExpr);
	    if (coverFlag)
	      coveredSubExpr += ((ItemExpr *)child(index))->getValueId();
	    exprIsCovered &= coverFlag;
	  } // for i'th child of expr

	break;

      } // default

    } // switch(getArity())

  return exprIsCovered;

} // ItemExpr::isCovered()

OrderComparison ItemExpr::sameOrder(ItemExpr *other,
				    NABoolean askOther)
{
  // first, check whether the item expressions are identical
  if (this == other)
    return SAME_ORDER;

  // second, try to simplify both item expressions
  OrderComparison order1;
  OrderComparison order2;
  OrderComparison result;
  Int32 inversions = 0;

  ItemExpr *simpThis = simplifyOrderExpr(&order1);
  ItemExpr *simpOther = other->simplifyOrderExpr(&order2);

  // check whether the expressions actually got simplified, return
  // DIFFERENT order if that was not the case
  if (this == simpThis AND other == simpOther)
    return DIFFERENT_ORDER;

  // Recursively call sameOrder with the simplified expressions
  // If it is a VEG reference, need to do a cast of "simpOther" to type
  // VEGReference so that the implementation of sameOrder that accepts
  // a VEGReference "other" parameter will be called.
  if (simpOther->getOperatorType() == ITM_VEG_REFERENCE)
    result = simpThis->sameOrder((VEGReference *)simpOther,askOther);
  else
    result = simpThis->sameOrder(simpOther,askOther);

  if (result != DIFFERENT_ORDER)
    {
      // if the order is not different, add up all the inversions
      // we encountered, either from simplifying expressions or
      // by comparing them
      if (order1 == INVERSE_ORDER)
	inversions++;
      if (order2 == INVERSE_ORDER)
	inversions++;
      if (result == INVERSE_ORDER)
	inversions++;

      // if we inverted an even number of times then the two expressions
      // have the same order
      if (inversions == 0 OR inversions == 2)
	result = SAME_ORDER;
      else
	result = INVERSE_ORDER;
    }

  return result;
}

OrderComparison ItemExpr::sameOrder(VEGReference *other,
				    NABoolean askOther)
{
  // First, check whether the "this" item expression is identical
  // to the "other" VegReference
  if (this == other)
    return SAME_ORDER;

  // Second, go through the "other" VegReference members one at a
  // time, and call sameOrder recursively to see if one of the member
  // ItemExpr's is identical to "this".
  const ValueIdSet &equivValues = other->getVEG()->getAllValues();
  OrderComparison result = DIFFERENT_ORDER;

  for (ValueId x = equivValues.init();
       equivValues.next(x) AND result == DIFFERENT_ORDER;
       equivValues.advance(x))
    {
      result = sameOrder(x.getItemExpr(),askOther);
    }

  return result;
}


ItemExpr * ItemExpr::simplifyOrderExpr(OrderComparison *newOrder)
{
  // The default implementation assumes that it is not possible
  // to simplify an expression.
  if (newOrder)
    *newOrder = SAME_ORDER;
  return this;
}

ItemExpr * ItemExpr::removeInverseOrder()
{
  // The default implementation just returns a pointer to itself,
  // since obviously there is no inverse order operator to remove.
  return this;
}

OperatorTypeEnum ItemExpr::getInverseOpType() const
{
  switch (getOperatorType())
    {
    case ITM_EQUAL:
      return ITM_NOT_EQUAL;
    case ITM_NOT_EQUAL:
      return ITM_EQUAL;
    case ITM_LESS:
      return ITM_GREATER;
    case ITM_LESS_EQ:
      return ITM_GREATER_EQ;
    case ITM_GREATER:
      return ITM_LESS;
    case ITM_GREATER_EQ:
      return ITM_LESS_EQ;
    case ITM_IS_TRUE:
      return ITM_IS_FALSE;
    case ITM_IS_FALSE:
      return ITM_IS_TRUE;
    case ITM_IS_NULL:
      return ITM_IS_NOT_NULL;
    case ITM_IS_NOT_NULL:
      return ITM_IS_NULL;
    case ITM_IS_UNKNOWN:
      return ITM_IS_NOT_UNKNOWN;
    case ITM_IS_NOT_UNKNOWN:
      return ITM_IS_UNKNOWN;

    case ITM_RETURN_FALSE:
      return ITM_RETURN_TRUE;
    case ITM_RETURN_TRUE:
      return ITM_RETURN_FALSE;

    default:
      return (getOperatorType());
    }
}

NABoolean ItemExpr::isAColumnReference( )
  { return FALSE; }

// return true iff maxSelectivity(this) == selectivity(this)
NABoolean ItemExpr::maxSelectivitySameAsSelectivity() const
{ 
  switch (getOperatorType())
    {
    case ITM_EQUAL:
    case ITM_LESS:
    case ITM_LESS_EQ:
    case ITM_GREATER:
    case ITM_GREATER_EQ:
      if (getArity() > 1)
        {
          // maxSelectivity(any op constant) == selectivity(any op constant) 
          // maxSelectivity(constant op any) == selectivity(constant op any) 
          // when op is oneof: =, <, <=, >, >=
          NABoolean negate;
          ItemExpr * rhs = child(1);
          ItemExpr * lhs = child(0);
          if (rhs->getOperatorType() == ITM_CACHE_PARAM) 
             rhs = ((ConstantParameter *)rhs)->getConstVal();
          else
            rhs = rhs->castToConstValue( negate );

          if (lhs->getOperatorType() == ITM_CACHE_PARAM) 
             lhs = ((ConstantParameter *)lhs)->getConstVal();
          else
            lhs = lhs->castToConstValue( negate );
          return ((lhs != NULL) || (rhs != NULL));
        }
      break;
      // maxSelectivity(any <> constant) is 1.0
    case ITM_IS_NULL:
    case ITM_IS_UNKNOWN:
    case ITM_IS_NOT_NULL:
    case ITM_IS_NOT_UNKNOWN:
      // maxSelectivity(any is null)     == selectivity(any is null)
      // maxSelectivity(any is not null) == selectivity(any is not null)
      return TRUE;
    default:
      return FALSE;
    }
  return FALSE; 
}
void ItemExpr::print(FILE * f,
		     const char * prefix,
		     const char * suffix) const
{
#ifndef NDEBUG
  ExprNode::print(f,prefix,suffix);

#ifndef NDEBUG
  fprintf(f,"%sItem Expression (Value Id %d): ",prefix, (CollIndex) valId_);

  if ( valId_ != NULL_VALUE_ID && &(valId_.getType()) != NULL )
    fprintf( f, "%s", valId_.getType().getTypeSQLname().data() );

  fprintf( f, "\n" );
#else
  fprintf(f,"%sItem Expression (Value Id %d):\n",prefix, (CollIndex) valId_);
#endif

  // print children
  Int32 nc = getArity();
  for (Lng32 i = 0; i < (Lng32)nc; i++)
    {
      fprintf(f,"%sExpression input %d:\n",prefix,i);
      if (child(i).getPtr())
	child(i)->print(f,CONCAT(prefix,"    "));
      else
	fprintf(f,"%snonexistent child\n",prefix);
    }
#endif
}

void ItemExpr::display()
{
  NAString result;
  unparse(result, PARSER_PHASE, USER_FORMAT_DELUXE);
  fprintf(stdout, "%s\n", result.data());
}


//
// computeKwdAndFlags() - a helper routine for ItemExpr::unparse()
//
// NOTE: The code in this routine came from the previous version of
//       ItemExpr::unparse().   It has been pulled out into
//       a separate routine so that the C++ compiler will produce
//       code that needs signficantly less stack space for the
//       recursive ItemExpr::unparse() routine.
//
void ItemExpr::computeKwdAndFlags( NAString &kwd,
		                   NABoolean &prefixFns,
		                   NABoolean &specialPrefixFns,
		                   PhaseEnum phase,
		                   UnparseFormatEnum form,
		                   TableDesc * tabId ) const
{
  OperatorTypeEnum operatorType = getOperatorType();

  if ( operatorType == ITM_BASECOLUMN)
  {
    if (form == QUERY_FORMAT)
    {
      if (CmpCommon::getDefault(MVQR_LOG_QUERY_DESCRIPTORS) != DF_DUMP_MV)
        kwd = ((BaseColumn *)this)->getTextForQuery();
      else
        kwd = getText();
    }
    else if ((form == COMPUTED_COLUMN_FORMAT) ||
	     (form == HIVE_MD_FORMAT))
      kwd = ToAnsiIdentifier(((BaseColumn *)this)->getColName());
    else
      kwd = getText();
  }
  else if ( operatorType == ITM_RENAME_COL)
  {
    if (form == HIVE_MD_FORMAT)
      kwd = ((RenameCol *)this)->getNewColRefName()->getColName();
    else
      kwd = getText();
  }
  else if (operatorType == ITM_NAMED_TYPE_TO_ITEM)
  {
    kwd = ToAnsiIdentifier(((NamedTypeToItem *)this)->getText());
  }
  else if (( operatorType == ITM_CACHE_PARAM) &&
           (form == QUERY_FORMAT) )
    ((ConstantParameter *)this)->getConstVal()->unparse(kwd, phase, QUERY_FORMAT, tabId);
  else
    kwd = getText();

  if (form == USER_FORMAT_DELUXE)
    // Do not upcase if string literal or delimited ident.
    if (!strchr(kwd, '\"') && !strchr(kwd, '\'')) //"
    {
      TrimNAStringSpace(kwd);
      kwd.toUpper();
    }

  switch (operatorType)
  {
    case ITM_BITAND:
    case ITM_CAST:
    case ITM_CHAR_LENGTH:
    case ITM_EXTRACT:
    case ITM_LEFT:
    case ITM_MOD:    
    case ITM_MOVING_SDEV:
    case ITM_MOVING_VARIANCE:
    case ITM_OFFSET:
    case ITM_POSITION:
    case ITM_POWER:
    case ITM_STDDEV:
    case ITM_SUBSTR:
    case ITM_VARIANCE:
      prefixFns = TRUE;
      break;

    case ITM_DATEDIFF_YEAR:
    case ITM_DATEDIFF_QUARTER:
    case ITM_DATEDIFF_MONTH:
    case ITM_DATEDIFF_WEEK:
    case ITM_YEARWEEK:
    case ITM_YEARWEEKD:
      specialPrefixFns = TRUE;
      break;

    case ITM_PLUS:
    case ITM_MINUS:
      if (((BiArith *) this)->isDateMathFunction() &&
          (form == QUERY_FORMAT ||
           form == COMPUTED_COLUMN_FORMAT))
        {
          // this is not a regular addition or subtractions, it's
          // a datetime function with special handling of the last
          // day of the month
          specialPrefixFns = TRUE;
        }
  }
}
//
// computeKwdAndPostfix() - a helper routine for ItemExpr::unparse()
//
// NOTE: Either computes or augments kwd, depending on the item expr involved.
//
// NOTE: The code in this routine came from the previous version of
//       ItemExpr::unparse().   It has been pulled out into
//       a separate routine so that the C++ compiler will produce
//       code that needs signficantly less stack space for the
//       recursive ItemExpr::unparse() routine.
//
void ItemExpr::computeKwdAndPostfix( NAString &kwd,
                                     NAString &postfix,
                                     UnparseFormatEnum form ) const
{
        // print function syntax of the form <kwd> <children> <postfix>
        //     <fname>(<prefix-args,> <children> <,postfix-args>)
        //     <--------kwd--------->            <----postfix--->

        // handle special prefix argument cases by adding to 'kwd' and 'postfix'

        OperatorTypeEnum operatorType = getOperatorType();
        switch (operatorType)
          {
          case ITM_CAST:
            {
              if (form == QUERY_FORMAT || form == COMPUTED_COLUMN_FORMAT)
                {
                  kwd += "(";
                  postfix = " AS ";

                  // Get the Data type after the Cast
                  const NAType *naType = ((Cast *) this)->getType();

                  // ignore NULLs description in getTypeSQLName, since it
                  // does not return valid SQL syntax
                  postfix += naType->getTypeSQLname(TRUE);
                  if (!naType->supportsSQLnull())
                    postfix += " NOT NULL";
                  postfix += ")";
                }
              else
                kwd += "(";
            }
            break;

          case ITM_EXTRACT:
          case ITM_EXTRACT_ODBC:
            switch(((Extract*) this)->getExtractField())
              {
              case REC_DATE_YEAR:
                kwd = "YEAR(";
                break;
              case REC_DATE_MONTH:
                kwd = "MONTH(";
                break;
              case REC_DATE_DAY:
                kwd = "DAY(";
                break;
              case REC_DATE_HOUR:
                kwd = "HOUR(";
                break;
              case REC_DATE_MINUTE:
                kwd = "MINUTE(";
                break;
              case REC_DATE_SECOND:
                kwd = "SECOND(";
                break;
              case REC_DATE_YEARQUARTER_EXTRACT:
                kwd = "DATE_PART('YEARQUARTER',";
                break;
              case REC_DATE_YEARMONTH_EXTRACT:
                kwd = "DATE_PART('YEARMONTH',";
                break;
              case REC_DATE_YEARQUARTER_D_EXTRACT:
                kwd = "DATE_PART('YEARQUARTERD',";
                break;
              case REC_DATE_YEARMONTH_D_EXTRACT:
                kwd = "DATE_PART('YEARMONTHD',";
                break;
                // YEARWEEK gets transformed into ITM_YEARWEEK, see below
              default:
                kwd += "(";
                break;
              }
            break;

          case ITM_DATE_TRUNC_YEAR:
            kwd = "DATE_TRUNC('YEAR',";
            break;
          case ITM_DATE_TRUNC_MONTH:
            kwd = "DATE_TRUNC('MONTH',";
            break;
          case ITM_DATE_TRUNC_DAY:
            kwd = "DATE_TRUNC('DAY',";
            break;
          case ITM_DATE_TRUNC_HOUR:
            kwd = "DATE_TRUNC('HOUR',";
            break;
          case ITM_DATE_TRUNC_MINUTE:
            kwd = "DATE_TRUNC('MINUTE',";
            break;
          case ITM_DATE_TRUNC_SECOND:
            kwd = "DATE_TRUNC('SECOND',";
            break;
          case ITM_DATE_TRUNC_CENTURY:
            kwd = "DATE_TRUNC('CENTURY',";
            break;
          case ITM_DATE_TRUNC_DECADE:
            kwd = "DATE_TRUNC('DECADE',";
            break;
          case ITM_DATEDIFF_YEAR:
            kwd = "DATEDIFF(YEAR,";
            break;
          case ITM_DATEDIFF_QUARTER:
            kwd = "DATEDIFF(QUARTER,";
            break;
          case ITM_DATEDIFF_MONTH:
            kwd = "DATEDIFF(MONTH,";
            break;
          case ITM_DATEDIFF_WEEK:
            kwd = "DATEDIFF(WEEK,";
            break;
          case ITM_YEARWEEK:
            kwd = "DATE_PART('YEARWEEK',";
            break;
          case ITM_YEARWEEKD:
            kwd = "DATE_PART('YEARWEEKD',";
            break;

          case ITM_PLUS:
          case ITM_MINUS:
            // we come here for special datetime functions that
            // get represented by a + or - operator with
            // isStandardNormalization() set
            if (((BiArith *) this)->isStandardNormalization())
              {
                // Here are the original expressions and their transformations,
                // (+) and (-) indicate the +/- operators with the standard
                // normalization flag set:
                //
                // ADD_MONTHS(<datetime_expr>, <num_expr> [, 0])  ==>
                //                <datetime_expr> (+) CAST(<num_expr> AS INTERVAL MONTHS)
                // DATE_ADD(<datetime_expr>, <interval_expr>)  ==>
                //                <datetime_expr> (+) <interval_expr>
                // DATE_SUB(<datetime_expr>, <interval_expr>)  ==>
                //                <datetime_expr> (-) <interval_expr>
                // DATEADD(<keyword>, <num_expr>, <datetime_expr>)  ==>
                //                <datetime_expr> (+) CAST(<num_expr> AS ...)
                // TIMESTAMPADD(<keyword>, <num_expr>, <datetime_expr>)  ==>
                //                <datetime_expr> (+) CAST(<num_expr> AS ...)
                //
                // We unparse all those as the equivalent DATE_ADD and DATE_SUB
                // functions:
                if (operatorType == ITM_PLUS)
                  kwd = "DATE_ADD(";
                else
                  kwd = "DATE_SUB(";
              }
            else if (((BiArith *) this)->isKeepLastDay() && operatorType == ITM_PLUS)
              {
                // Here are the original expressions and their transformations,
                // (+) and (-) indicate the +/- operators with the "keep last day"
                // normalization flag set:
                //
                // ADD_MONTHS(<datetime_expr>, <num_expr>, 1)  ==>
                //                <datetime_expr> (+) CAST(<num_expr> AS INTERVAL MONTHS)
                CMPASSERT(child(1)->getValueId().getType().getTypeQualifier() == NA_INTERVAL_TYPE &&
                          child(1)->getValueId().getType().getFSDatatype() == REC_INT_MONTH);
                kwd = "ADD_MONTHS(";
                postfix = ", 1" + postfix;
              }
            else
              DCMPASSERT(FALSE); // the above should have covered all datetime math expressions
            break;

          default:
            kwd += "(";
            break;
          }
}

void ItemExpr::unparse(NAString &result,
		       PhaseEnum phase,
		       UnparseFormatEnum form,
		       TableDesc * tabId) const
{
  // Avoid creating large strings which will only be truncated later
  // by EXPLAIN.
  if (form == EXPLAIN_FORMAT && (result.length() > 4096)) 
  {
    return;
  }

  // Allocate 3 procedure local variables and initialize them here.
  // They may get changed by the call to ItemExpr::computeKwdAndFlags() 
  // which follows, but we initialize them here so source analysis
  // tools won't complain about unitialized variables.
  //
  NAString  kwd(CmpCommon::statementHeap());
  NABoolean prefixFns = FALSE;          // prefix function, some with infix, arity 2
  NABoolean specialPrefixFns = FALSE;   // require special prefix argument, arity 1 or 2

  computeKwdAndFlags( kwd, prefixFns, specialPrefixFns, phase, form, tabId );

  OperatorTypeEnum operatorType = getOperatorType();

  Int32 arity = getArity();

  if (operatorType != origOpType() &&
      form == QUERY_FORMAT ||
      form == COMPUTED_COLUMN_FORMAT)
    {
      // handle some cases where the original function was rewritten
      // in the binder, using a ZZZBinderFunction
      switch (origOpType())
        {
        case ITM_DATE_TRUNC_MINUTE:
        case ITM_DATE_TRUNC_SECOND:
        case ITM_DATE_TRUNC_MONTH:
        case ITM_DATE_TRUNC_HOUR:
        case ITM_DATE_TRUNC_CENTURY:
        case ITM_DATE_TRUNC_DECADE:
        case ITM_DATE_TRUNC_YEAR:
        case ITM_DATE_TRUNC_DAY:
        case ITM_DATEDIFF_YEAR:
        case ITM_DATEDIFF_QUARTER:
        case ITM_DATEDIFF_MONTH:
        case ITM_DATEDIFF_WEEK:
        case ITM_YEARWEEK:
        case ITM_YEARWEEKD:
          {
            ItemExpr *unboundExpr =
              ZZZBinderFunction::tryToUndoBindTransformation((ItemExpr *) this);
            if (unboundExpr)
              {
                // we were able to undo this transformation, unparse
                // the ZZZBinderFunction instead
                unboundExpr->unparse(result,
                                     phase,
                                     form,
                                     tabId);
                return;
              }
          }
          break;
          
        default:
          break;
        }
    }

  switch (arity)
  {
    case 0:
      // simply print the text out for a leaf operator
      result += kwd;
      break;

    case 2:
      if (form != FILE_FORMAT && !specialPrefixFns)
      {
        if( ((form == MV_SHOWDDL_FORMAT) || 
             (form == QUERY_FORMAT) ||
             (form == COMPUTED_COLUMN_FORMAT)) && 
             prefixFns )
        {
          result += kwd;
          result += "(";

          if (child(0))
            child(0)->unparse(result, phase,form, tabId);
          else
            result += "NULL";

          if (operatorType == ITM_POSITION)
            result += " IN ";
          else
            result += ", ";

          if (child(1))
            child(1)->unparse(result, phase, form, tabId);
          else
            result += "NULL";
          result += ")";
          break;
        } // STDDEV, MOD, VARIANCE
        else  
          if ( (operatorType == ITM_TRIM)  && 
               ((form == QUERY_FORMAT) ||
                (form == COMPUTED_COLUMN_FORMAT)))
        {
          result += kwd;
          result += "(";
          child(1)->unparse(result,phase,form,tabId);
          result += ")";
        } 
        else 
          if((operatorType == ITM_ITEM_LIST ||
          operatorType == ITM_AND) &&
          (form != MV_SHOWDDL_FORMAT) &&
          (form != QUERY_FORMAT) )
          {
            if (child(0))
              child(0)->unparse(result,phase,form, tabId);
            else
              result += "NULL";

            if (operatorType != ITM_ITEM_LIST)
              result += " ";
            result += kwd;
            result += " ";

            if (child(1))
              child(1)->unparse(result,phase,form, tabId);
            else
              result += "NULL";
          }
          else
          {
            // assume this is an infix operator (<child0>) op (<child1>)

            result += "(";

            NABoolean list0 = FALSE, list1 = FALSE;
            if (operatorType != ITM_ITEM_LIST)
            {
              list0 = child(0)->getOperatorType() == ITM_ITEM_LIST;
              list1 = child(1)->getOperatorType() == ITM_ITEM_LIST;
            }

            if (list0) result += "(";
            child(0)->unparse(result,phase,form, tabId);
            if (list0) result += ")";

            result += " ";
            result += kwd;
            result += " ";

            if (list1) result += "(";
            child(1)->unparse(result,phase,form, tabId);
            if (list1) result += ")";

            result += ")";
          } // infix operator
          break; // for now, break here
        } 
    // otherwise fall through to next case

    default:
    {
      // usually, that's arity 1, but can be >1 as well
      if (form == USER_FORMAT_DELUXE && isQuantifiedComp(this))
      {
        child(0)->unparse(result,phase,form, tabId);
        result += " ";
        result += kwd;
        result += " (_subquery_)";
      } // QuantifiedComp
      else if (arity == 1 && isAPredicate() && operatorType != ITM_NOT)
      {
        child(0)->unparse(result,phase,form,tabId);
        result += " ";
        result += kwd;
      } // predicate and != ITM_NOT
      else if (form == MV_SHOWDDL_FORMAT && operatorType == ITM_RENAME_COL)
      {
        child(0)->unparse(result, phase, form, tabId);
      }
      else if (form == MV_SHOWDDL_FORMAT && operatorType == ITM_COUNT_NONULL)
      {
        result += "count(";
        child(0)->unparse(result, phase, form, tabId);
        result += ")";
      }
      else if (form == MV_SHOWDDL_FORMAT && operatorType == ITM_COUNT)
      {
        result += "count(*)";
      }
      else
      {
        NAString postfix = ")";

        computeKwdAndPostfix( kwd, postfix, form );

        // function name, open parenthesis, initial arguments
        result += kwd;

        // unparse all the children as arguments
        for (Lng32 i = 0; i < (Lng32)arity; i++)
          {
            if (i > 0) result += ", ";
            child(i)->unparse(result,phase,form,tabId);
          }

        // add any postfix and closing parenthesis
        result += postfix;
      } // else (print function syntax)
    } // default of switch(arity)
  } // end switch (arity)
} // ItemExpr::unparse

// MDAMR
DisjunctArray * ItemExpr::mdamTreeWalk()
{

  DisjunctArray * rc = new STMTHEAP DisjunctArray(new STMTHEAP ValueIdSet(getValueId()));
  return rc;
}
//MDAMR

NABoolean ItemExpr::equatesToAConstant() const
{
  if (getOperatorType() != ITM_EQUAL)
    return FALSE;

  ItemExpr *lhs = child(0);
  lhs = lhs->getLeafValueIfUseStats();

  if(lhs->getOperatorType() != ITM_VEG_REFERENCE)
    return FALSE;

  ItemExpr *rhs = child(1);
  rhs = rhs->getLeafValueIfUseStats();

  if (rhs->getOperatorType() == ITM_CACHE_PARAM)
    return TRUE;

  if(!rhs->doesExprEvaluateToConstant(TRUE, TRUE))
     return FALSE;

  return TRUE;
}

// The check performed to determine if a column 
// exists works only till binder phase. For use 
// in post-Binder phase, the logic in the method 
// will have to be changed.
NABoolean ItemExpr::containsColumn()
{
  OperatorTypeEnum op = getOperatorType();

  // This is a base column.
  if (op == ITM_BASECOLUMN)
    return TRUE;

  // This is a leaf item other than a base column.
  if (op >= ITM_CONSTANT && op <= ITM_VEG_REFERENCE)
    return FALSE;

  // Iterate through this item's inputs looking for a base column.
  for (Int32 i = 0; i < getArity(); i++) {
    ItemExpr *expr = child(i);

    if (!expr)
      return FALSE;

    if (expr->containsColumn())
      return TRUE;
  }

  return FALSE;
}

//++MV
// -----------------------------------------------------------------------
// Return the default selectivity for this predicate
// -----------------------------------------------------------------------
double ItemExpr::defaultSel()
{
  if (selectivityFactor_ < 0)
    return getOperatorType() == ITM_RETURN_FALSE ? 0.0 : 1.0;
  else
    return selectivityFactor_;
}
//--MV

ItemExpr * ItemExpr::containsRightmost(const ItemExpr *ie)
{
  if (this == ie) return this;
  if (!this || !ie) return NULL;
  for (Int32 arity = getArity(); arity-- > 0; )
    if (child(arity))
      return (child(arity) == ie) ? this : child(arity)->containsRightmost(ie);
  return NULL;
}

NABoolean ItemExpr::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL )
    return FALSE;

  CMPASSERT(other->getValueId() != NULL_VALUE_ID && getValueId() != NULL_VALUE_ID);
  //defaults behavior is return false
  return FALSE;
}

NABoolean ItemExpr::hasBaseEquivalence(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;

  CMPASSERT(other->getValueId() != NULL_VALUE_ID && this->getValueId()!= NULL_VALUE_ID);
  
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  if (getValueId()== other->getValueId() ||
      (getOperatorType() == ITM_CONSTANT && ((ConstValue*)this)->isNull() && ((ConstValue*)other)->isNull()))
    return TRUE;
  else
    if (getArity()==0)
      return FALSE;

  for (Int32 i= 0 ; i < getArity(); i++)
  {
    ItemExpr * chld= child(i);
    if (!chld->hasBaseEquivalence(other->child(i)) || !chld->hasEquivalentProperties(other->child(i)) )
      return FALSE;
  }
  return TRUE;
}//ItemExpr::hasBaseEquivalence(ValueId otherId)


ItemExpr * ItemExpr::changeDefaultOrderToDesc()
{
  if (getOperatorType() == ITM_ITEM_LIST )
  {
    for(Int32 i=0; i<getArity(); i++)
    {
      child(i) = child(i)->changeDefaultOrderToDesc();
    }
    return this;
  }
  else
    if (getOperatorType()== ITM_INVERSE)
    {
      if (child(0)->getOperatorType() == ITM_INVERSE)
        return child(0)->child(0);
      else
        return this;
    }
    else
      return new (CmpCommon::statementHeap()) InverseOrder(this);
  }//ItemExpr::changeDefaultOrderToDesc()

ValueId ItemExpr::removeInverseFromExprTree( NABoolean & invExists)
{
  if (getOperatorType() == ITM_ITEM_LIST )
  {
    for(Int32 i=0; i<getArity(); i++)
      child(i) = child(i)->removeInverseFromExprTree(invExists);
    return getValueId();
  }
  else
      if (getOperatorType() == ITM_INVERSE)
        invExists = TRUE;
      return removeInverseOrder()->removeInverseOrder()->getValueId(); 

 }//ItemExpr::removeInverseFromExprTree()

///


void ItemExpr::transformOlapFunctions(CollHeap *wHeap)
{

  for (Int32 i= 0 ; i < getArity(); i++)
  {
    ItemExpr * mychild= child(i);
    if (mychild->isOlapFunction())
    {
      ItmSeqOlapFunction * olap = (ItmSeqOlapFunction*) mychild;
      ItemExpr * itmExpr = olap->transformOlapFunction(wHeap);
      itmExpr->synthTypeAndValueId(TRUE);
      child(i) = itmExpr;
    }
    //else  //olap function can t have another olap as descendant ????
   // {
    //  child(i)->transformOlapFunctions(wHeap);
    //}
    child(i)->transformOlapFunctions(wHeap);
  }

}//void ItemExpr::transformOlapFunctions(CollHeap *wHeap)


// -----------------------------------------------------------------------
// removeInverseFromExprTree
//
// overloading above method for special purpose use
// onlyDouble parameter is used when the Inverse should not be removed 
// unless two Inverse exprs are found in a row
// -----------------------------------------------------------------------
ItemExpr* 
ItemExpr::removeInverseFromExprTree( NABoolean & invExists,
                                     const NABoolean onlyDouble )
{
  if (getOperatorType() == ITM_ITEM_LIST )
  {
    for(Int32 i=0; i<getArity(); i++)
      child(i) = child(i)->removeInverseFromExprTree(invExists,
                                                     onlyDouble);
    return this;
  }
  else
  {
    if (getOperatorType() == ITM_INVERSE)
      invExists = TRUE;

    // if the onlyDouble flag is set to TRUE, then we
    // must find two Inverse exprs in a row to remove
    if( (!onlyDouble) ||
        getOperator().match(ITM_INVERSE) &&
        child(0)->getOperator().match(ITM_INVERSE) )
    {
      return removeInverseOrder()->removeInverseOrder();
    }
    else
    {
      return this;
    }
  }
 }//ItemExpr::removeInverseFromExprTree()

//


 void ItemExpr::removeNotCoveredFromExprTree(ItemExpr * &ie ,
                                             const ValueIdSet &seqColumns,
                                             NABoolean rootNode)
{
  if (rootNode)
  { 
    while (ie->getOperatorType() == ITM_NOTCOVERED && 
           seqColumns.contains(ie->getValueId()))
    {
      ie= ie->child(0);
    }
  }
 
  for(Int32 i=0; i<ie->getArity(); i++)
  {
    while (ie->child(i)->getOperatorType() == ITM_NOTCOVERED && 
      seqColumns.contains(ie->child(i)->getValueId()))
    {
      ie->child(i) = ie->child(i)->child(0);
    }
    ItemExpr * tmp = ie->child(i);
    removeNotCoveredFromExprTree(tmp,seqColumns, FALSE);
  }

}//ItemExpr::removeNotCoveredFromExprTree()

// -----------------------------------------------------------------------
// member functions for class ZZZBinderFunction
// -----------------------------------------------------------------------
ItemExpr *ZZZBinderFunction::copyTopNode(ItemExpr *derivedNode,
                                         CollHeap *outHeap)
{
  ZZZBinderFunction *result=0;

  if (derivedNode == NULL)
   {
    switch (getArity())
     {
       case 1:
               result = new (outHeap) ZZZBinderFunction(getOperatorType(),
                            child(0));
         break;
       case 2:
               result = new (outHeap) ZZZBinderFunction(getOperatorType(),
                            child(0), child(1));
         break;
       case 3:
               result = new (outHeap) ZZZBinderFunction(getOperatorType(),
                            child(0), child(1), child(2));
         break;
       case 4:
               result = new (outHeap) ZZZBinderFunction(getOperatorType(),
                            child(0), child(1), child(2), child(3));
         break;
       case 5:
               result = new (outHeap) ZZZBinderFunction(getOperatorType(),
                             child(0), child(1), child(2), child(3), child(4));
         break;
     } // switch
   }
  else
   {
     result = (ZZZBinderFunction*)derivedNode;
   }

  result->flags_ = flags_;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class BiArith
// -----------------------------------------------------------------------
Int32 BiArith::getArity() const { return 2;}

const NAString BiArith::getText() const
{
  switch (getOperatorType())
    {
    case ITM_PLUS:
      return "+";
    case ITM_MINUS:
      return "-";
    case ITM_TIMES:
      return "*";
    case ITM_DIVIDE:
      return "/";
    case ITM_EXPONENT:
      return "**";
    default:
      return "unknown BiArith";
    } // switch
} // BiArith::getText()

NABoolean BiArith::duplicateMatch(const ItemExpr& other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  const BiArith &o = (BiArith &) other;
  if (unaryNegate_ != o.unaryNegate_)
    return FALSE;

  if (normalizeFlags_ != o.normalizeFlags_ )
    return FALSE;

  if (intervalQualifier_ OR o.intervalQualifier_)
    {
      if (intervalQualifier_ == NULL OR
	  o.intervalQualifier_ == NULL OR
	  NOT (*intervalQualifier_ == *(o.intervalQualifier_)))
	return FALSE;
    }
  return TRUE;
}

ItemExpr * BiArith::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  BiArith *result;
  BiArith  *result2;

  if (derivedNode == NULL)
    result = new (outHeap) BiArith(getOperatorType());
  else
    result = (BiArith*)derivedNode;

  result->setRoundingMode(getRoundingMode());
  if ( ignoreSpecialRounding() ) result->setIgnoreSpecialRounding();
  result->setDivToDownscale(getDivToDownscale());

  result2 = (BiArith *)ItemExpr::copyTopNode(result, outHeap);
  result2->normalizeFlags_ = normalizeFlags_ ;
  return result2;
}

ItemExpr * BiArithSum::copyTopNode(ItemExpr *derivedNode, CollHeap *outHeap)
{
  ItemExpr *result;
  if (derivedNode == NULL)
    result = new (outHeap) BiArithSum(getOperatorType());
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

ConstValue * BiArith::castToConstValue (NABoolean & negate_it)
{
  OperatorTypeEnum op = getOperatorType();

  if ((op == ITM_MINUS) || (op == ITM_PLUS))
  {
    NABoolean neg0 = FALSE;
    NABoolean neg1 = FALSE;
    ConstValue * lhs = child(0)->castToConstValue(neg0);
    ConstValue * rhs = child(1)->castToConstValue(neg1);

    if (lhs && rhs)
    {
      Lng32 scale0 = 0;
      if (lhs->canGetExactNumericValue() &&
          lhs->getExactNumericValue(scale0) == 0)
        {
          if (op == ITM_MINUS)
            negate_it = (!neg1);
          else
            negate_it = neg1;
          return (rhs);
        }
      else
      {
        Lng32 scale1 = 0;
        if (rhs->canGetExactNumericValue() &&
            rhs->getExactNumericValue(scale1) == 0)
          {
            negate_it = neg0;
            return (lhs);
          }
      }
    }
  }
  return NULL;
}

/*--------------------------------------------------------------
* this routine is mainly called by complifyAndRemoveUncoveredSuffix(),
* sameOrder() and shortCutGroupBy::topMatch() to find out if a
* simplified expression still has the same order or if it has inverse
* order. Based on this we decide to do a inverse scan or a forward
* scan to satisfy order by or min/max aggregate.
*-----------------------------------------------------------------*/
ItemExpr * BiArith::simplifyOrderExpr(OrderComparison *newOrder)
{
  ItemExpr *result = this;
  OrderComparison myOrder = SAME_ORDER;
  OrderComparison childOrder = SAME_ORDER;


  // - the following expressions are simplified and
  //   has the same order as "a"
  //     a + const
  //     const + a
  //     a - const
  //     a * positive const
  //     positive const * a
  //     a / positive const
  // - the following expressions have the inverse order of "a"
  // and are siplified
  //     const - a
  //     a * negative const
  //     negative const * a
  //     a / negative const
  // - the following expressions are not simplified because
  //  the order is not consistent
  // positive const / a
  // negative const / a
  //
  // example:
  //
  //   values for a : -2 , -1, 1, 2, 3
  //
  //  if we say that 1/a is inverse order of a
  // then a order by of 1/a results in
  //
  //    (ordering of a    1/a          but the correct order
  //     if we simplify)                would be
  //	3	             .33             -1     -1
  //    2                .5				-.5    -2
  //	1                 1             .33    3
  //	-1                -1            .5     2
  //    -2                -.5            1     1
  //
  //Note: as long as 'a' is an unsigned integer we can simplify but
  //      it is to rare an occasion that a user does 1/a or -1/a so
  //      it is not supported.


  if (child(0)->getOperatorType() == ITM_CONSTANT)
    {
      switch (getOperatorType())
	{
	case ITM_PLUS:
	  result = child(1)->simplifyOrderExpr(&childOrder);
	  myOrder = SAME_ORDER;
	  break;

	case ITM_MINUS:
	  result = child(1)->simplifyOrderExpr(&childOrder);
	  myOrder = INVERSE_ORDER;
	  break;

	case ITM_TIMES:
	  result = child(1)->simplifyOrderExpr(&childOrder);
	  break;

	case ITM_DIVIDE:
      break;

	default:
	  break;
	}
    }
  else if (child(1)->getOperatorType() == ITM_CONSTANT)
    {
      switch (getOperatorType())
	{
	case ITM_PLUS:
	case ITM_MINUS:
	  result = child(0)->simplifyOrderExpr(&childOrder);
	  myOrder = SAME_ORDER;
	  break;

	case ITM_TIMES:
	  result = child(0)->simplifyOrderExpr(&childOrder);
	  break;

	case ITM_DIVIDE:
      // part of fix to genesis case 10-070416-0218, soln 10-070416-4141
      // mxcmp must consider ORDER BY "v/const" incompatible with GROUP BY "v"
      // when "v/const" can lose precision relative to "v"
      if (getRoundingMode() != 0 &&
          CmpCommon::getDefault(COMP_BOOL_176) == DF_OFF)
        {
          // do NOT simplify. leave expr as is.
          break;
        }
      else
        {
          result = child(0)->simplifyOrderExpr(&childOrder);
        }
	  break;

	default:
	  break;
	}
    }
  else if (child(0)->getOperatorType() == ITM_MINUS AND
	  child(0)->child(0)->getOperatorType() == ITM_CONSTANT
	  AND child(0)->child(1)->getOperatorType() == ITM_CONSTANT
	  AND *(short *)((ConstValue *)((ItemExpr*)(child(0)->child(0))))->getConstValue()==0
	  )

  {
	  ItemExpr * constExpr = child(0)->child(1);
	  switch(getOperatorType())
	  {
	  case ITM_TIMES:
		  result = child(1)->simplifyOrderExpr(&childOrder);
		  myOrder = INVERSE_ORDER;
		  break;
	  case ITM_DIVIDE:
		  break;
	  }
  }
  else if (child(1)->getOperatorType() == ITM_MINUS AND
	  child(1)->child(0)->getOperatorType() == ITM_CONSTANT
	  AND child(1)->child(1)->getOperatorType() == ITM_CONSTANT
	  AND *(short *)((ConstValue *)((ItemExpr*)(child(1)->child(0))))->getConstValue()==0
	  )

  {
	  switch(getOperatorType())
	  {
	  case ITM_TIMES:
		  result = child(0)->simplifyOrderExpr(&childOrder);
		  myOrder = INVERSE_ORDER;
		  break;
	  case ITM_DIVIDE:
        // part of fix to genesis case 10-070416-0218, soln 10-070416-4141
        // mxcmp must consider ORDER BY "v/-const" incompatible 
        // with GROUP BY "v"
        // when "v/-const" can lose precision relative to "v"
        if (getRoundingMode() != 0 &&
            CmpCommon::getDefault(COMP_BOOL_176) == DF_OFF)
          {
            // do NOT simplify. leave expr as is.
            break;
          }
        else
          {
            result = child(0)->simplifyOrderExpr(&childOrder);
            myOrder = INVERSE_ORDER;
          }
		  break;
	  }

  }

  // return results
  if (newOrder)
    {
      // if both this and the child inverse or leave the order the
      // net result is the same order, otherwise we inverse the order once
      if (myOrder == childOrder)
	*newOrder = SAME_ORDER;
      else
	*newOrder = INVERSE_ORDER;
    }
  return result;
}

ItemExpr * BiArith::foldConstants(ComDiagsArea *diagsArea,
				  NABoolean newTypeSynthesis)
{
  ItemExpr *result = this;

  // attempt to do constant arithmetic if both operands are constants
  // and if we are allowed to pick the result type
  if (child(0)->getOperatorType() == ITM_CONSTANT AND
      child(1)->getOperatorType() == ITM_CONSTANT AND
      newTypeSynthesis)
    {
      // get the two operands into two Int64 variables if possible
      Int64 ops[2] = {0,0};
      Lng32 scales[2] = {0,0};
      Int64 numResult = 0;
      NABoolean canDoIt = TRUE; // give up once it becomes too difficult
      scales[0] = 0;  scales[1] = 0;
      ops[0] = 0;  ops[1] = 0;

      for (Int32 i = 0; i < 2 AND canDoIt; i++)
	{
	  NABoolean negate;
	  ConstValue *cv = child(i)->castToConstValue(negate);
	  if (cv AND cv->canGetExactNumericValue())
	    {
	      CMPASSERT(NOT negate);
	      ops[i] = cv->getExactNumericValue(scales[i]);
	    }
	  else
	    {
	      // can't do it yet if this isn't represented as a signed
	      // or unsigned binary number without any additional fluff
	      canDoIt = FALSE;
	    }
	}

      // -----------------------------------------------------------------
      // At this point, ops contains two 64 bit numbers that represent the
      // exact numeric operands. scales contains the scales of the operands.
      // Now do the arithmetic, but try not to cause overflow traps.
      // -----------------------------------------------------------------

      // for now, give up on any values with a scale != 0
      if (scales[0] != 0 OR scales[1] != 0)
	canDoIt = FALSE;

      if (canDoIt)
	{
	  // adjust scales, if necessary
	  switch (getOperatorType())
	    {
	    case ITM_PLUS:
	    case ITM_MINUS:
	      if (scales[0] != scales[1])
		// sorry, keep it simple for now
		canDoIt = FALSE;
	      break;

	    default:
	      break;
	    }
	}

      NABoolean overflow = FALSE;
      NABoolean zeroDivide = FALSE;

      // check for overflow
      // for now, just do it for reasonably small constants
      if (ops[0] < -10000000 OR ops[0] > 10000000 OR
	  ops[1] < -10000000 OR ops[1] > 10000000)
	canDoIt = FALSE;

      // -----------------------------------------------------------------
      // ok, now it's time to do the real thing
      // -----------------------------------------------------------------

      if (canDoIt AND NOT overflow)
	{
	  switch (getOperatorType())
	    {
	    case ITM_PLUS:
	      numResult = ops[0] + ops[1];
	      break;

	    case ITM_MINUS:
	      numResult = ops[0] - ops[1];
	      break;

	    case ITM_TIMES:
	      numResult = ops[0] * ops[1];
	      break;

	    case ITM_DIVIDE:
	      if (ops[1] == 0)
		zeroDivide = TRUE;
	      else
		numResult = ops[0] / ops[1];
	      break;
	    }
	}

      // error handling
      if (canDoIt AND (overflow OR zeroDivide))
	{
          NAString up(CmpCommon::statementHeap());
	  unparse(up);
	  *diagsArea << DgSqlCode(zeroDivide ? -4075 : -4076) << DgString0(up);
	  canDoIt = FALSE;
	}

      if (canDoIt)
	{
	  // now we're ready to do the switch

          if (numResult >= INT_MIN AND numResult <= INT_MAX)
	    {
	      // result can fit into a long, use the ConstValue(long)
	      // constructor
	      Lng32 num;

	      num = (Lng32) numResult;
	      result = new(CmpCommon::statementHeap()) SystemLiteral(num);
	    }
	  else
	    {
	      // the result becomes an 8 byte integer, use the ConstValue
	      // constructor with a type, the binary image, and a text string

	      char numstr[TEXT_DISPLAY_LENGTH];
	      convertInt64ToAscii(numResult,numstr);
              NAString runningOutOfNames(numstr, CmpCommon::statementHeap());

	      result = new(CmpCommon::statementHeap()) SystemLiteral(
		   new(CmpCommon::statementHeap()) SQLLargeInt(CmpCommon::statementHeap(), TRUE,FALSE),
		   (void *) &numResult,
		   sizeof(numResult),
		   &runningOutOfNames);
	    }
	}
    }

  // perform the generic tasks (recursion, type synthesis) on the
  // result, regardless of whether we transformed it or not
  return result->ItemExpr::foldConstants(diagsArea,newTypeSynthesis);
}


NABoolean BiArith::isEquivalentForCodeGeneration(const ItemExpr * other)
  {
  NABoolean rc = FALSE;        // assume failure

  if (hasBaseEquivalenceForCodeGeneration(other))
    {
    // we know that other is a BiArith, its operator type is the same,
    // and that the children are equivalent
    BiArith * otherBiArith = (BiArith *)other;

    if ( (unaryNegate_ == otherBiArith->unaryNegate_) &&
         (normalizeFlags_ == otherBiArith->normalizeFlags_ ) )
      {
      // make sure both have the same interval qualifier (if any)
      NABoolean sameIntervalQualifier = FALSE;  // assume failure
      if (intervalQualifier_)
        {
        if (otherBiArith->intervalQualifier_)
          {
          const NAType & otherIntervalQualifier = *(otherBiArith->intervalQualifier_);
          if (intervalQualifier_->operator==(otherIntervalQualifier))
            sameIntervalQualifier = TRUE;
          }
        }
      else if (otherBiArith->intervalQualifier_ == 0)
        sameIntervalQualifier = TRUE;

      if (sameIntervalQualifier)
        {
        // the next test distinguishes between BiArith and its special-case
        // inheriting classes, BiArithSum and BiArithCount
	if (getText() == otherBiArith->getText())  // same class?
          rc = TRUE;  // the two values can be produced by the same generated expr
        }
      }
    }

  return rc;
  }

QR::ExprElement BiArith::getQRExprElem() const
{
  return QR::QRBinaryOperElem;
}

NABoolean BiArith::hasEquivalentProperties(ItemExpr * other)
{  
  if (other == NULL)    
    return FALSE;  
  if (getOperatorType() != other->getOperatorType() ||     
    getArity() != other->getArity())    
    return FALSE;    
  
  BiArith * otherBiArith = (BiArith *) other;    
  
  return     
    (this->unaryNegate_ == otherBiArith->unaryNegate_) &&     
    (this->normalizeFlags_ == otherBiArith->normalizeFlags_) &&    
    (this->intervalQualifier_ == otherBiArith->intervalQualifier_) &&    
    (this->divToDownscale_ == otherBiArith->divToDownscale_);
}

ItemExpr * UnArith::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  UnArith *result;

  if (derivedNode == NULL)
    result = new (outHeap) UnArith();
  else
    result = (UnArith*)derivedNode;

  return result;
}

// -----------------------------------------------------------------------
// member functions for class ColReference
// -----------------------------------------------------------------------
Int32 ColReference::getArity() const { return 0; }

const NAString ColReference::getText() const
{
  NAColumn *nacol = getValueId() != NULL_VALUE_ID ?
    getValueId().getNAColumn() :
    NULL;
  return nacol ?
    nacol->getFullColRefNameAsAnsiString() :
    colRefName_->getColRefAsAnsiString();
}

HashValue ColReference::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= colRefName_->getColRefAsString();

  return result;
}

NABoolean ColReference::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  ColReference &o = (ColReference &) other;

  // compare any local data members of the derived class
  // and return FALSE if they don't match
  if (NOT (*colRefName_ == *(o.colRefName_)))
    return FALSE;

  return TRUE;
}

ItemExpr * ColReference::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
  {
    ColRefName *colName;
    if (colRefName_->isStar())
    {
      colName = new (outHeap) ColRefName(1);
    }
    else
    {
      colName = new (outHeap)
	ColRefName(colRefName_->getColName(),
		   colRefName_->getCorrNameObj(),
		   outHeap);
    }
    ColReference *newColRef = new (outHeap) ColReference(colName);
    newColRef->setTargetColumnClass(getTargetColumnClass());
    result = newColRef;
  }
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class HostVar
// -----------------------------------------------------------------------

HostVar::HostVar(const HostVar &hv)
       : ItemExpr(hv.getOperatorType()),
	 varName_(hv.varName_, CmpCommon::statementHeap()),
	 indicatorName_(hv.indicatorName_, CmpCommon::statementHeap()),
	 prototypeValue_(hv.prototypeValue_, CmpCommon::statementHeap()),
	 prototypeType_(hv.prototypeType_),
	 isEnvVar_(hv.isEnvVar_), isDefine_(hv.isDefine_),
	 isSystemGenerated_(hv.isSystemGenerated_),
	 paramMode_ (hv.paramMode_),
	 ordinalPosition_ (hv.ordinalPosition_),
	 hvIndex_ (hv.hvIndex_),
	 rowsetInfo_(hv.rowsetInfo_),
	 heading_(hv.heading_, CmpCommon::statementHeap()),
	 tablename_(hv.tablename_,CmpCommon::statementHeap())
{
  if (hv.varType_ == NULL)
     varType_ = NULL;
  else
     varType_ = hv.varType_->newCopy(CmpCommon::statementHeap());
}

Int32 HostVar::getArity() const { return 0; }

NABoolean HostVar::isAUserSuppliedInput() const             { return TRUE; }

const NAString HostVar::getText() const
{
  NAString punc(CmpCommon::statementHeap());
  if (isSystemGenerated_) punc = "\\:";			// backslash, colon
  if (!indicatorName_.isNull())
    return punc + varName_ + " " + punc + indicatorName_;
  else
    return punc + varName_;
}

HashValue HostVar::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= (CollIndex) getValueId();

  return result;
}

NABoolean HostVar::duplicateMatch(const ItemExpr & other) const
{
  // rely on value id comparison since the binder should not
  // assign different value ids for the same hostvar
  return genericDuplicateMatch(other);
}

ItemExpr * HostVar::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) HostVar(*this);
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

NABoolean HostVar::isCharTypeMatchRulesRelaxable()
{
  if ( getType()->getTypeQualifier() == NA_CHARACTER_TYPE &&
       ((CharType*)getType())->getCharSet() == CharInfo::UNICODE
     )
    return TRUE;
  else
    return FALSE;
}

ItemExpr * DefaultSpecification::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) DefaultSpecification();
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}



NABoolean RowsetArrayScan::isCharTypeMatchRulesRelaxable()
{
  if ( getOperatorType() == ITM_ROWSETARRAY_SCAN &&
       getType()->getTypeQualifier() == NA_CHARACTER_TYPE &&
       ((CharType*)getType())->getCharSet() == CharInfo::UNICODE
     )
    return TRUE;
  else
    return FALSE;
}

// -----------------------------------------------------------------------
// member functions for class ConstantParameter
// -----------------------------------------------------------------------
ConstantParameter::ConstantParameter(const ConstValue& v, NAMemory *h,
                                     NABoolean quantizeLen, UInt32 p)
  : val_((ConstValue*)&v), type_(CONST_CAST(NAType*, v.getType()))
  , Parameter(ITM_CACHE_PARAM)
{
  if (type_->getTypeQualifier() != NA_CHARACTER_TYPE) {
    type_ = type_->newCopy(h);
  }
  else {
    // convert char type to equivalent varchar type
    type_ = ((CharType*)type_)->equivalentVarCharType(h, quantizeLen);
  }
  posns_ = new(STMTHEAP) ClusteredBitmap(STMTHEAP);
  *posns_ += p;
}

ConstantParameter::ConstantParameter(ConstValue* v, NAMemory *h,
                                     const NAType* typ, UInt32 p)
  : val_(v), type_(typ->newCopy(h)), Parameter(ITM_CACHE_PARAM)
{
  // non-null ConstantParameters are non-nullable
  type_->setNullable(v->isNull());

  if (v->getType()->getTypeQualifier() == NA_CHARACTER_TYPE &&
      type_->getTypeQualifier() == NA_CHARACTER_TYPE) {
    // this is part of a fix to genesis case: 10-010618-3484.
    // ConstantParameter's type must take v's (not typ's) upshift.
    CharType *vt = (CharType*)v->getType();
    CharType *t = (CharType*)type_;
    t->setUpshifted(vt->isUpshifted());
  }
  posns_ = new(STMTHEAP) ClusteredBitmap(STMTHEAP);
  *posns_ += p;
}

ConstantParameter::ConstantParameter(const ConstantParameter& cp, NAHeap *h)
  : val_(new(h) ConstValue(*cp.val_,h)), type_(cp.type_->newCopy(h))
  , posns_(new(h) ClusteredBitmap(*cp.posns_,h))
  , Parameter(cp)
{
}

ConstantParameter::~ConstantParameter()
{
  // we don't own type_; the statementHeap does.
  // so, we leave it up to statementHeap to free it.
}

ItemExpr *ConstantParameter::copyTopNode
(ItemExpr *derivedNode, CollHeap* outHeap)
{
  CMPASSERT(derivedNode == NULL);
  ItemExpr *result = new (outHeap) ConstantParameter(*this, (NAHeap*)outHeap);
  return result;
}

NABoolean ConstantParameter::duplicateMatch(const ItemExpr& other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  CMPASSERT(val_ != NULL);
  const ConstantParameter &o = (ConstantParameter &) other;
  return val_->duplicateMatch(*(o.val_));
  // no comparison of type_ since it should be the same if the values are
}

const NAString ConstantParameter::getText() const
{
  if (val_)
    return NAString("%(", CmpCommon::statementHeap()) + val_->getText() + ")";
  else
    return NAString("%", CmpCommon::statementHeap());
}

const NAType* ConstantParameter::synthesizeType()
{
  return getType();
}

HashValue ConstantParameter::topHash()
{
  return val_ ? val_->topHash() : ItemExpr::topHash();
}

// return true if val matches this ConstantParameter
NABoolean ConstantParameter::matches(ConstValue *val) const
{
  if (!val_ || !val) 
    return FALSE;
  else
    return *val_ == *val;
}

// -----------------------------------------------------------------------
// member functions for class DynamicParam
// -----------------------------------------------------------------------
const NAString DynamicParam::getText() const
{
  if (!indicatorName_.isNull())
    return NAString("?") + paramName_ +
           NAString(" ?") + indicatorName_ ;
  else
    return NAString("?") + paramName_  ;
}

HashValue DynamicParam::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= (CollIndex) getValueId();

  return result;
}

NABoolean DynamicParam::duplicateMatch(const ItemExpr & other) const
{
  // rely on value id comparison since the binder should not
  // assign different value ids for the same parameter
  return genericDuplicateMatch(other);
}

ItemExpr * Parameter::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result = NULL;
  if (derivedNode == NULL)
    ABORT("copyTopNode() can only be called for a derived class of Parameter");
  else
    result = derivedNode;
  return ItemExpr::copyTopNode(result, outHeap);
}

ItemExpr * DynamicParam::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL) {
    result = new (outHeap) DynamicParam(paramName_, indicatorName_, outHeap);
    ((DynamicParam *) result)->setRowsetSize(rowsetSize_);
    ((DynamicParam *) result)->setRowsetInfo(rowsetInfo_);
    ((DynamicParam *) result)->setParamHeading(heading_);
    ((DynamicParam *) result)->setParamTablename(tablename_);
    // we remember our original dynamic parameter because we
    // must use their valueid at dynamicparam::codegen time
    ((DynamicParam *) result)->setOriginal(this);
  }

  else
    result = derivedNode;

  return Parameter::copyTopNode(result, outHeap);
}

const NAType * DynamicParam::pushDownType(NAType& desiredType,
				  enum NABuiltInTypeEnum defaultQualifier)
{
   const NAType * currentType = &getValueId().getType();
   if ( desiredType.getTypeQualifier() == NA_CHARACTER_TYPE &&
        currentType-> getTypeQualifier() == NA_CHARACTER_TYPE
      )
   {
     CharType &desiredCT = (CharType&)desiredType;
     enum CharInfo::CharSet desiredCS = desiredCT.getCharSet();
     CharType* ct = (CharType*)currentType;

     if ( ct ->getCharSet() != desiredCS ) 
     {
       ct->setCharSet(desiredCS);
       ct->setCollation(desiredCT.getCollation());
       ct->setCoercibility(desiredCT.getCoercibility());
       ct->setCaseinsensitive(desiredCT.isCaseinsensitive());
     }
     return currentType;
   }
   return &desiredType;   
}


// -----------------------------------------------------------------------
// member functions for class RoutineParam
// -----------------------------------------------------------------------
const NAString RoutineParam::getText() const
{
    return NAString("?") + paramName_  ;
}

HashValue RoutineParam::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= paramName_;
  result ^= optionalParam_;
  result ^= paramMode_;
  result ^= ordinalPosition_;
  result ^= rdesc_;
  result ^= (CollIndex) getValueId();

  return result;
}

NABoolean RoutineParam::duplicateMatch(const ItemExpr & other) const
{
  // rely on value id comparison since the binder should not
  // assign different value ids for the same parameter
  return genericDuplicateMatch(other);
}


ItemExpr * RoutineParam::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  RoutineParam *result;

  if (derivedNode == NULL) {
      // Does not copy the rdesc_ 
    result = new (outHeap) RoutineParam(paramName_, paramType_, 
                              ordinalPosition_,paramMode_, NULL, outHeap);
  }

  else
    result = (RoutineParam *) derivedNode;

  if (derivedNode != NULL)
  {
    result->paramName_       = paramName_;
    if (paramType_ != NULL)
      result->paramType_     = paramType_->newCopy (outHeap);
    result->paramMode_       = paramMode_;
    result->ordinalPosition_ = ordinalPosition_;
    result->rdesc_           = NULL; // Do not want to copy the RoutineDesc
                                     // nor does it make any sense to just
                                     // point to the one from the clone since
                                     // multiple routineParams typically will
                                     // point to the same routineDesc. If we
                                     // just make a deep copy here we will end
                                     // up with N new copies of the same info..
                                     // 
  }
  result->optionalParam_   = optionalParam_;
  result->isCacheable_     = isCacheable_;
  memcpy(result->argumentType_, argumentType_, sizeof(argumentType_));

  return Parameter::copyTopNode((ItemExpr *)result, outHeap);
}
// -----------------------------------------------------------------------
// member functions for class BaseColumn
// -----------------------------------------------------------------------

BaseColumn::~BaseColumn()
{}

// copy constructor
BaseColumn::BaseColumn(const BaseColumn &column) :
  ItemExpr(column),
  tableDesc_(column.tableDesc_),
  colNumber_(column.colNumber_),
  equivalentIndexCols_(column.equivalentIndexCols_),
  computedColumnExpr_(column.computedColumnExpr_)
{}


NAColumn *BaseColumn::getNAColumn() const
{
  return getTableDesc()->getNATable()->getNAColumnArray()[colNumber_];
}

const NAString& BaseColumn::getColName() const
{
  return getNAColumn()->getColName();
}

const NAType& BaseColumn::getType() const
{
  return *getNAColumn()->getType();
}

Int32 BaseColumn::getArity() const { return 0; }

const NAString BaseColumn::getText() const
{
  // getenv() calls are costly. avoid especially in release code.
#ifndef NDEBUG
  if (getenv("SIMPLE_BASECOL_DISPLAY") || getenv("SIMPLE_DISPLAY"))
  {
    return
      NAString(getTableDesc()->getNATable()->getTableName().getObjectName()) +
      NAString(".") +
      NAString(getTableDesc()->getNATable()->getNAColumnArray()[colNumber_]->
               getColName());
  }
#endif

  if (CmpCommon::getDefault(MVQR_LOG_QUERY_DESCRIPTORS) == DF_DUMP_MV)
  {
    // If a correlation name exists, use it.
    // Otherwise, just use the base column name.
    const CorrName& corr = getTableDesc()->getCorrNameObj();
    NAString result;
    if (corr.getCorrNameAsString() != "")
    {
      result += corr.getCorrNameAsString();
      result += ".";
    }
    result += getTableDesc()->getNATable()->getNAColumnArray()[colNumber_]->getColName();
    return result;
  }

  // return the table in the format "table.col"
  ColRefName name(
    getTableDesc()->getNATable()->getNAColumnArray()[colNumber_]->getColName(),
    getTableDesc()->getCorrNameObj(), CmpCommon::statementHeap());
  return name.getColRefAsAnsiString(FALSE, TRUE);
}

const NAString BaseColumn::getTextForQuery() const {
  // return the table in the format "table.col" where
  // table is the physical table name and not the corr
  // table name as in getText() version

  return
    NAString(getTableDesc()->getNATable()->getTableName().getCatalogName()) +
    NAString(".") +
    NAString(getTableDesc()->getNATable()->getTableName().getSchemaName()) +
    NAString(".") +
    NAString(getTableDesc()->getNATable()->getTableName().getObjectName()) +
    NAString(".") +
    NAString(getTableDesc()->getNATable()->getNAColumnArray()[colNumber_]->
      getColName());
}

// return the set of KeyColumns referenced in the Computed Column Expression

void BaseColumn::getUnderlyingColumnsForCC(ValueIdSet &underlyingCols)
{
  if (getNAColumn()->isComputedColumn()) 
    {
      ValueIdSet keyCols = getTableDesc()->getClusteringIndex()->getIndexKey();
      keyCols = keyCols.convertToBaseIds();

      ValueIdSet compExprSet =  getComputedColumnExpr();
      underlyingCols.accumulateReferencedValues(keyCols, compExprSet);
    }
}

HashValue BaseColumn::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= (void *) getTableDesc();
  result ^= colNumber_;

  return result;
}

NABoolean BaseColumn::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  BaseColumn &o = (BaseColumn &) other;

  // compare any local data members of the derived class
  // and return FALSE if they don't match
  if (getTableDesc() != o.getTableDesc() OR
      colNumber_ != o.colNumber_)
    return FALSE;

  return TRUE;
}

ItemExpr * BaseColumn::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;
  ItemExpr *oldBC;

  if (derivedNode == NULL)
    oldBC = new (outHeap) BaseColumn(*this);
  else
    oldBC = derivedNode;

  result = ItemExpr::copyTopNode(oldBC, outHeap);
  if (derivedNode != NULL)
     ((BaseColumn *)result)->computedColumnExpr_ = 
                               ((BaseColumn *)derivedNode)->computedColumnExpr_;
  return result;
}

NABoolean BaseColumn::isCovered(const ValueIdSet& newExternalInputs,
				const GroupAttributes& newRelExprAnchorGA,
				ValueIdSet& referencedInputs,
				ValueIdSet& /*coveredSubExpr*/,
				ValueIdSet& /*unCoveredExpr*/) const
{
  // ---------------------------------------------------------------------
  // A base column is also covered if any equivalent index column is
  // available in the group attributes or in the new inputs. Note that
  // the inverse of this is not true: the ValueId of an IndexColumn item
  // expression is NOT covered by group attributes that contain the value
  // id of the corresponding BaseColumn.
  // ---------------------------------------------------------------------
  ValueId x = equivalentIndexCols_.init();
  for (;
       equivalentIndexCols_.next(x);
       equivalentIndexCols_.advance(x))
    {
      if (newRelExprAnchorGA.isCharacteristicOutput(x) OR
	  newRelExprAnchorGA.isCharacteristicInput(x))
	return TRUE;

      if (newExternalInputs.contains(x))
	{
	  referencedInputs += x;
	  return TRUE;
	}
    }

  return FALSE;
}

OrderComparison BaseColumn::sameOrder(ItemExpr *other,
				      NABoolean askOther)
{
  OrderComparison result =
    ItemExpr::sameOrder(other,askOther);

  if (result != DIFFERENT_ORDER)
    return result;

  // try all equivalent index columns, but don't call sameOrder recursively
  // for them, rather do a simple look-up
  // 10-031114-1317: replaced a for loop that traverses the ValueIdSet
  // with the contains method that is more efficient

  if (equivalentIndexCols_.contains(other->getValueId()))
  {
    result = SAME_ORDER;
  }

  return result;
}

ValueIdList *BaseColumn::getClusteringKeyCols() const
{
  return (ValueIdList*)
    &(tableDesc_->getClusteringIndex()->getClusteringKeyCols());
}

QR::ExprElement BaseColumn::getQRExprElem() const
{
  return QR::QRColumnElem;
}

// -----------------------------------------------------------------------
// member functions for class IndexColumn
// -----------------------------------------------------------------------
IndexColumn::IndexColumn(const NAFileSet *indexPtr,
			 Lng32 indexColumnNumber,
			 const ValueId &colDefinition)
  : ItemExpr(ITM_INDEXCOLUMN),index_(indexPtr),
    indexColNumber_(indexColumnNumber),indexColDefinition_(colDefinition)
{}

IndexColumn::~IndexColumn()
{}

NAColumn * IndexColumn::getNAColumn() const
{
  return index_->getAllColumns()[indexColNumber_];
}

const NAType& IndexColumn::getType() const
{
  return *getNAColumn()->getType();
}

Int32 IndexColumn::getArity() const { return 0; }

const NAString IndexColumn::getText() const
{
  // As an indexcol is not an Ansi construct, we need not make an effort to
  // unparse it following Ansi rules; this quick-and-dirty method is fine
  // (should only be seen in gui-display tool):

  return ColRefName::getColRefAsString(
			 getNAColumn()->getColName(),
			 index_->getFileSetName().getQualifiedNameAsString());
}

void IndexColumn::unparse(NAString &result,
			  PhaseEnum /*phase*/,
			  UnparseFormatEnum form,
			  TableDesc * tabId) const
{
  // don't print the NSK name in Explain
  if (form == EXPLAIN_FORMAT)
    {
      // result += "indexcol(";
      /*      if (index_->isVolatile())
	      {
	      // only output the object part of the external name
	      // and exclude cat/sch names as these are internal IDs.
	      ComObjectName con(index_->getExtFileSetName());
	      result += ColRefName::getColRefAsString(
	      getNAColumn()->getColName(),
	      con.getObjectNamePartAsAnsiString());
	      }
	      else
	      {
      */
      result += ColRefName::getColRefAsString(
	   getNAColumn()->getColName(),
	   index_->getExtFileSetName());
     // result += ")";
    }
  else
    {
      // in all other cases do print the NSK name?
      result += getText();
    }
}

HashValue IndexColumn::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any data members of the derived class
  // (the definition is functionally dependent on the index and
  // column number, so don't hash it)
  result ^= (void *) index_;
  result ^= indexColNumber_;

  return result;
}

NABoolean IndexColumn::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  IndexColumn &o = (IndexColumn &) other;

  // compare any local data members of the derived class
  // and return FALSE if they don't match
  if (index_ != o.index_ OR
      indexColNumber_ != o.indexColNumber_ OR
      NOT (indexColDefinition_ == o.indexColDefinition_))
    return FALSE;

  return TRUE;
}

ItemExpr * IndexColumn::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap)
      IndexColumn(index_,indexColNumber_,indexColDefinition_);
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

OrderComparison IndexColumn::sameOrder(ItemExpr *other,
				       NABoolean askOther)
{
  OrderComparison result =
    ItemExpr::sameOrder(other,askOther);

  if (result != DIFFERENT_ORDER)
    return result;

  // try it with the definition of the index column
  return indexColDefinition_.getItemExpr()->sameOrder(other,TRUE);
}

ValueId IndexColumn::mapAndRewrite(ValueIdMap &map,
				   NABoolean mapDownwards)
{
  // try the standard method first
  ValueId result = ItemExpr::mapAndRewrite(map,mapDownwards);

  // we're done if the map contained a mapping for this column
  if (result != getValueId())
    return result;

  // try to replace the index column with its base column and then map
  result = indexColDefinition_.getItemExpr()->mapAndRewrite(map,mapDownwards);

  // return the result if it could be mapped
  // (no need to cache the index col. mapping since no new expr is created)
  if (result != indexColDefinition_)
    return result;

  // if all failed, just return the value id unchanged
  return getValueId();
}

Lng32 IndexColumn::getOffset() const
{
  return index_->getIndexKeyColumns().getOffset((short) indexColNumber_);
}

// -----------------------------------------------------------------------
// member functions for class SelIndex
// -----------------------------------------------------------------------
ItemExpr * SelIndex::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  CMPASSERT(derivedNode == NULL);

  ItemExpr *result = new (outHeap)
    SelIndex(getSelIndex(), getExprInGrbyClause());
  ((SelIndex *)result)->setRenamedColNameInGrbyClause
    (renamedColNameInGrbyClause());

  return ItemExpr::copyTopNode(result, outHeap);
}

Int32 SelIndex::getArity() const { return 0; }

const NAString SelIndex::getText() const
{
  char cp[TEXT_DISPLAY_LENGTH];
  sprintf(cp, "%d", getSelIndex());
  return cp;
}

// -----------------------------------------------------------------------
// member functions for class ValueIdRef
// -----------------------------------------------------------------------
Int32 ValueIdRef::getArity() const { return 0; }

NABoolean ValueIdRef::isCovered(const ValueIdSet& newExternalInputs,
				const GroupAttributes& coveringGA,
				ValueIdSet& referencedInputs,
				ValueIdSet& coveredSubExpr,
				ValueIdSet& /*unCoveredExpr*/) const
{
  // ---------------------------------------------------------------------
  // The ValueIdRef isCovered if the expression that it derives from
  // is covered.
  // ---------------------------------------------------------------------
  return coveringGA.covers(isDerivedFrom(), newExternalInputs,
			   referencedInputs, &coveredSubExpr);
} // ValueIdRef::isCovered()

const NAString ValueIdRef::getText() const
{
  char cp[TEXT_DISPLAY_LENGTH];
  sprintf(cp,"ValueId (%d)",CollIndex(isDerivedFrom()));
  return cp;
} // ValueIdRef::getText()


HashValue ValueIdRef::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= derivedFrom_;

  return result;
}

NABoolean ValueIdRef::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  ValueIdRef &o = (ValueIdRef &) other;

  // compare any local data members of the derived class
  // and return FALSE if they don't match
  if (NOT (derivedFrom_ == o.derivedFrom_))
    return FALSE;

  return TRUE;
}

ItemExpr * ValueIdRef::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ValueIdRef(derivedFrom_);
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

OrderComparison ValueIdRef::sameOrder(ItemExpr *other,
				      NABoolean askOther)
{
  OrderComparison result =
    ItemExpr::sameOrder(other,askOther);

  if (result != DIFFERENT_ORDER)
    return result;

  // check with the original expression
  return isDerivedFrom().getItemExpr()->sameOrder(other,TRUE);
}

// -----------------------------------------------------------------------
// member functions for class ValueIdProxy
// -----------------------------------------------------------------------

NABoolean ValueIdProxy::isCovered(const ValueIdSet& newExternalInputs,
                                  const GroupAttributes& coveringGA,
                                  ValueIdSet& referencedInputs,
                                  ValueIdSet& coveredSubExpr,
                                  ValueIdSet& unCoveredExpr) const
{
  // ---------------------------------------------------------------------
  // The ValueIdProxy isCovered if the expression that it derives from
  // is covered.
  // ---------------------------------------------------------------------
  return coveringGA.covers(isDerivedFrom(), newExternalInputs,
                           referencedInputs, &coveredSubExpr, &unCoveredExpr);
} // ValueIdProxy::isCovered()

const NAString ValueIdProxy::getText() const
{
  char cp[TEXT_DISPLAY_LENGTH];
  sprintf(cp,"ValueIdProxy (%d:%d->%d)",CollIndex(isDerivedFrom()),
                                        CollIndex(getOutputNum()),
                                        CollIndex(getOutputId()));
  return cp;
} // ValueIdProxy::getText()


HashValue ValueIdProxy::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= derivedFrom_;
  result ^= outputOrdinalNumber_;
  result ^= outputValueId_;

  return result;
}

NABoolean ValueIdProxy::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  ValueIdProxy &o = (ValueIdProxy &) other;

  // compare any local data members of the derived class
  // and return FALSE if they don't match
  if ((NOT (derivedFrom_ == o.derivedFrom_)) AND 
      (NOT (outputValueId_ == o.outputValueId_)) AND
      (NOT (outputOrdinalNumber_ == o.outputOrdinalNumber_)))
    return FALSE;

  return TRUE;
}


ItemExpr * ValueIdProxy::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ValueIdProxy(derivedFrom_, 
                                        outputValueId_, 
                                        outputOrdinalNumber_);
  else
  {
    result = derivedNode;
  }

  ValueIdProxy * tmpNode = (ValueIdProxy *) result;
 
  tmpNode->derivedFrom_ = derivedFrom_;
  tmpNode->outputValueId_ = outputValueId_;
  tmpNode->outputOrdinalNumber_ = outputOrdinalNumber_;
  tmpNode->transformDerivedFromValueId_ = transformDerivedFromValueId_;

  return ItemExpr::copyTopNode(result, outHeap);
}

ItemExpr *ValueIdProxy::containsUDF()
{
    // The transfromChild_ means that this ValueIdProxy represents the
    // the subquery or UDF, thus return true..
  if ((transformDerivedFromValueId_ == TRUE) && 
      derivedFrom_.getItemExpr()->containsUDF())
    return this;
  else
    return 0;

} // ValueIdProxy::containsUDF

NABoolean ValueIdProxy::containsIsolatedUDFunction()
{
    // The transfromChild_ means that this ValueIdProxy represents the
    // the subquery or UDF, thus return true..
  if ((transformDerivedFromValueId_ == TRUE) && 
      derivedFrom_.getItemExpr()->containsUDF())
  {
     UDFunction *udf = (UDFunction *) derivedFrom_.getItemExpr()->containsUDF();

     const RoutineDesc *rdesc = udf->getRoutineDesc();
     if (rdesc == NULL || rdesc->getEffectiveNARoutine() == NULL ) return FALSE;  
     return ( rdesc->getEffectiveNARoutine()->isIsolate() ? TRUE : FALSE ) ;
  }
  else
    return FALSE;

} // ValueIdProxy::containsIsolatedUDFunction

NABoolean ValueIdProxy::containsSubquery()     // virtual method
{

    // The transfromChild_ means that this ValueIdProxy represents the
    // the subquery or UDF, thus return true..
  if ((transformDerivedFromValueId_ == TRUE) && 
      derivedFrom_.getItemExpr()->containsSubquery())
    return TRUE;
  else
    return FALSE;

} // ValueIdProxy::containsSubquery

NABoolean ValueIdProxy::containsValueIdProxySibling(const ValueIdSet &siblings)
    // virtual method
{
  // This method is used to make sure we extract all valueIds associated with
  // a subquery or UDF from a predicate or list.

  for (ValueId s = siblings.init(); siblings.next(s); siblings.advance(s))
  {
    if (s.getItemExpr()->getOperatorType() == ITM_VALUEID_PROXY)
    {
       ValueIdProxy *proxy = (ValueIdProxy *) (s.getItemExpr());
       if ( derivedFrom_ == proxy->isDerivedFrom() ) return TRUE;
    }
  }
  return FALSE;

} // ValueIdProxy::containsValueIdProxySibling

// -----------------------------------------------------------------------
// member functions for class ValueIdUnion
// -----------------------------------------------------------------------
void ValueIdUnion::setSource(Lng32 index, ValueId v)
{
  if((Lng32)sources_.entries() <= index) {
    sources_.insertAt(index, v);
  } else {
    sources_[index] = v;
  }
}

Int32 ValueIdUnion::getArity() const { return 0; }

NABoolean ValueIdUnion::isCovered(const ValueIdSet& newExternalInputs,
				  const GroupAttributes& coveringGA,
				  ValueIdSet& referencedInputs,
				  ValueIdSet& coveredSubExpr,
				  ValueIdSet& /*unCoveredExpr*/ ) const
{
  // ---------------------------------------------------------------------
  // Check which of all the operands of the ValueIdUnion isCovered().
  // Return its ValueId in coveredSubExpr.
  // ---------------------------------------------------------------------

  // $$$ Question: why not return all of the operands of the ValueIdUnion which
  // $$$ are covered -- why do we break after we find the first, in the loop below?

  ValueIdSet localSubExpr;

  for(CollIndex i = 0; i < entries(); i++)
  {
    localSubExpr.clear();
    if (coveringGA.covers(getSource(i), newExternalInputs,
			  referencedInputs, &localSubExpr) )
      {
	coveredSubExpr += getSource(i);
        break;
      }
  }

  // ---------------------------------------------------------------------
  // An aggregate function is coerced to fail the coverage test even when
  // its operand isCovered(). This is because the computation of the
  // aggregate function requires "raw" values produced by its operand to
  // be grouped. The aggregated values cannot be treated in the same
  // manner as the constitutent raw values.
  // ---------------------------------------------------------------------
  // Is something similar to the above, copied from
  // Aggregate::isCovered(), also true for ValueIdUnion?  If this is so
  // (which sort of makes sense to me), then that explains why this
  // function is hardcoded to always return FALSE.
  // ---------------------------------------------------------------------

  return FALSE; // the ValueIdUnion is not covered by its child's coveringGA

} // ValueIdUnion::isCovered()

void ValueIdUnion::getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                             const GroupAttributes& coveringGA,
                                             const ValueIdSet & newExternalInputs) const
{
  // for now, not sure if this method is really necessary -- default to
  // the base class method:

  ItemExpr::getLeafValuesForCoverTest (leafValues, coveringGA, newExternalInputs) ;

  //  leafValues += getValueId();  // this MIGHT be the right definition of this
  //                               // class -- hard to say at this point ...
} // ValueIdUnion::getLeafValuesForCoverTest()

const NAString ValueIdUnion::getText(UnparseFormatEnum form) const
{
  NAString result(CmpCommon::statementHeap());
  const char *delim;

  if (form == USER_FORMAT_DELUXE)
    delim = " UNION ";				// keyword format
  else
  {
    delim = ", ";				// standard list format
    result = "ValueIdUnion(";
  }

  for (CollIndex i = 0; i < entries(); i++)
  {
    if (i > 0)
      result += delim;

    // guard against loops in the references
    // (can happen with common subexpressions, for example)
    if (!getSource(i).getItemExpr()->referencesTheGivenValue(getValueId()))
      getSource(i).getItemExpr()->unparse(result);
    else
      result += "...";
  }

  if (form == USER_FORMAT_DELUXE)
    result.toUpper();
  else
    result += ")";

  return result;

} // ValueIdUnion::getText(), the NON-virtual method

const NAString ValueIdUnion::getText() const
{
  return getText(USER_FORMAT/*standard list format*/);

} // ValueIdUnion::getText(), the virtual method

HashValue ValueIdUnion::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  for(CollIndex i = 0; i < entries(); i++)
  {
    result ^= getSource(i);
  }

  result ^= result_;

  result ^= flags_;

  return result;
}

NABoolean ValueIdUnion::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  ValueIdUnion &o = (ValueIdUnion &) other;

  // compare any local data members of the derived class
  // and return FALSE if they don't match
  for(CollIndex i = 0; i < entries(); i++)
  {
    if (getSource(i) != o.getSource(i))
      return FALSE;
  }

  if (result_ != o.result_ || flags_ != o.flags_)
    return FALSE;

  return TRUE;
}

ItemExpr * ValueIdUnion::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ValueIdUnion *result;

  if (derivedNode == NULL)
    result = new (outHeap) ValueIdUnion(sources_, result_, flags_);
  else
    result = (ValueIdUnion*)derivedNode;

  result->otherFlags_ = otherFlags_;

  return ItemExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class VEG
// -----------------------------------------------------------------------
VEG::VEG()
  : ItemExpr(ITM_VEG), done_(FALSE), userInputs_(0), seenBefore_(FALSE),
   specialNulls_(FALSE)
{
  allocValueId();
  // Allocate a VegReference for representing any member of the VEG.
  vegRef_  = new (CmpCommon::statementHeap()) VEGReference(getValueId());
  // Allocate a VegPredicate for replacing the = predicate
  vegPred_ = new (CmpCommon::statementHeap()) VEGPredicate(getValueId());
} // VEG::VEG()

VEG::VEG(const ValueIdSet & vegSet)
 : ItemExpr(ITM_VEG), eqGroup_(vegSet), done_(FALSE), userInputs_(0),
   seenBefore_(FALSE), specialNulls_(FALSE)
{
  allocValueId();
  // Allocate a VegReference for representing any member of the VEG.
  vegRef_  = new (CmpCommon::statementHeap()) VEGReference(getValueId());
  // Allocate a VegPredicate for replacing the = predicate
  vegPred_ = new (CmpCommon::statementHeap()) VEGPredicate(getValueId());
} // VEG::VEG()

VEG::~VEG()
{
  delete vegRef_;
  delete vegPred_;
} // VEG::~VEG()

void VEG::insert(const ValueId & newValue)
{
  if (NOT eqGroup_.contains(newValue))
  {
    eqGroup_ += newValue;
    if (newValue.getItemExpr()->isAUserSuppliedInput())
      userInputs_++;
  }
} // VEG::insert()

void VEG::insert(const ValueIdSet & newValues)
{
  for (ValueId exprId = newValues.init();
       newValues.next(exprId);
       newValues.advance(exprId))
  {
    if (NOT eqGroup_.contains(exprId))
    {
      eqGroup_ += exprId;
      if (exprId.getItemExpr()->isAUserSuppliedInput())
        userInputs_++;
    }
  }
} // VEG::insert()

void VEG::merge(const VEG& other)
{
  // Add all members of the other VEG to my own
  eqGroup_ += other.eqGroup_;
  // Fixup the VEGReference and VEGPredicate of the
  // other VEG to reference me instead.
  other.getVEGReference()->replaceVEG(getValueId());
  other.getVEGPredicate()->replaceVEG(getValueId());
  userInputs_ += other.userInputs_;

  if (specialNulls_ && !other.getSpecialNulls())
    specialNulls_ = FALSE;
} // VEG::merge()

Int32 VEG::getArity() const { return 0; }

const NAString VEG::getText() const
{
  if (seenBefore())
  {
    // getenv() calls are costly. avoid especially in release code.
#ifndef NDEBUG
    if (getenv("SIMPLE_VEG_DISPLAY") || getenv("SIMPLE_DISPLAY"))
      return NAString("[...]");
#endif
    return NAString("...");
  }

  markAsSeenBefore();

  NAString result("(",CmpCommon::statementHeap());

  // getenv() calls are costly. avoid especially in release code.
#ifndef NDEBUG
  if (getenv("SIMPLE_VEG_DISPLAY") || getenv("SIMPLE_DISPLAY")) result = "[";
#endif
  NABoolean first = TRUE;

  //char cstrId[TEXT_DISPLAY_LENGTH];

  const ValueIdSet & eqGroup_ = getAllValues();

  for (ValueId x = eqGroup_.init(); eqGroup_.next(x); eqGroup_.advance(x))
  {
    // getenv() calls are costly. avoid especially in release code.
#ifndef NDEBUG
    if (getenv("NO_INDEXCOL_DISPLAY") || getenv("SIMPLE_DISPLAY"))
      if (x.getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN) continue;
#endif

      if (x.getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN) continue;
    if (NOT first)
    {
      result += " = ";
    }
    first = FALSE;
    x.getItemExpr()->unparse(result);
    // add valueId's after pred. name

    // getenv() calls are costly. avoid especially in release code.
#ifndef NDEBUG
    if ( (getenv("SIMPLE_VEG_DISPLAY") || getenv("SIMPLE_DISPLAY")) &&
          x.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE ) continue;
#endif



  }

  // getenv() calls are costly. avoid especially in release code.
#ifndef NDEBUG
  if (getenv("SIMPLE_VEG_DISPLAY") || getenv("SIMPLE_DISPLAY"))
    result += "]";
#endif
  result += ")";
  markAsNotSeenBefore();
  return result;
}

HashValue VEG::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= (ValueIdSet &) *this;

  return result;
}

NABoolean VEG::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  if (getVEGReference()->getVEG() !=
      ((VEG &)other).getVEGReference()->getVEG())
    return FALSE;

  return TRUE;

}

ItemExpr * VEG::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  ABORT("No methods for copying this object yet");

  if (derivedNode == NULL)
    result = new (outHeap) VEG();
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}


void
VEG::getAndExpandAllValues(ValueIdSet& expandedValues) const
{
  // Get all members of VEGRef:
  const ValueIdSet& vegGroup = getAllValues();
  for (ValueId vid = vegGroup.init();
       vegGroup.next(vid);
       vegGroup.advance(vid))
    {
      if (vid.getItemExpr()->getOperatorType() != ITM_VEG_REFERENCE)
         {
          expandedValues.insert(vid);
          continue;
         }

      if ( (vid.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE) &&
           (! expandedValues.contains(vid))
         )
        {

        // In some cases, a VEGRef might contain another VEGRef within
        // its VEG which recursively self-references the VEGRef.
        // In order to prevent running into an infinite loop there, we
        // insert the vid into the set and remove it after returning
        // from recursive call. (case 10-001201-9972)

          expandedValues.insert(vid);
         ((VEGReference *) (vid.getItemExpr())) -> getVEG() ->
           getAndExpandAllValues(expandedValues);
          expandedValues -=vid;
        }
    }
} // VEG::getAndExpandAllValues(ValueIdSet& expandedValues) const


// return a Constant, hostvar or parameter from this VEG
ValueId VEG::getAConstantHostVarOrParameter() const
{
  for (ValueId id = eqGroup_.init(); eqGroup_.next(id); eqGroup_.advance(id))
  {
    ItemExpr *ie = id.getItemExpr();

    OperatorTypeEnum oper = ie->getOperatorType();

    if ((oper == ITM_CONSTANT)  ||
        (oper == ITM_HOSTVAR)   ||
        (oper == ITM_DYN_PARAM) ||
        (oper == ITM_CACHE_PARAM))
      return id;
  }

  return NULL_VALUE_ID;
}

// return a Constant from this VEG. If includeCacheParam is TRUE, also accept
// the value of a constant parameter.
ValueId VEG::getAConstant(NABoolean includeCacheParam) const
{
  for (ValueId id = eqGroup_.init(); eqGroup_.next(id); eqGroup_.advance(id))
  {
    ItemExpr *ie = id.getItemExpr();

    OperatorTypeEnum oper = ie->getOperatorType();

    if (oper == ITM_CONSTANT)
      return id;
    else if (includeCacheParam && oper == ITM_CACHE_PARAM)
      return (static_cast<ConstantParameter*>(ie))->getConstVal()->getValueId();
  }

  return NULL_VALUE_ID;
}

// -----------------------------------------------------------------------
// member functions for class VEGPredicate
// -----------------------------------------------------------------------

// default selectivity for VEGPredicates (same as equality preds)
double VEGPredicate::defaultSel()
{
  return (1.0/(CURRSTMT_OPTDEFAULTS->defNoStatsUec()) );
}

VEGPredicate::VEGPredicate(const ValueId & ofVEG)
            : ItemExpr(ITM_VEG_PREDICATE)
	    , veg_(ofVEG)
	    , specialNulls_(FALSE) // ++MV - Irena
{
  CMPASSERT (ofVEG.getItemExpr()->getOperatorType() == ITM_VEG);
  synthTypeAndValueId();
}

VEGPredicate::VEGPredicate(const VEGPredicate& vp)
            : ItemExpr(ITM_VEG_PREDICATE)
	    , veg_(vp.veg_)
	    , specialNulls_(vp.specialNulls_)
	    , predsWithSelectivities_(vp.predsWithSelectivities_)
{
  synthTypeAndValueId();
}

VEGPredicate::~VEGPredicate() {}

void VEGPredicate::replaceVEG(const ValueId& vegId)
{
  CMPASSERT(vegId.getItemExpr()->getOperatorType() == ITM_VEG);
  veg_ = vegId;
} // VEGPredicate::replaceVEG()

Int32 VEGPredicate::getArity() const { return 0; }

NABoolean VEGPredicate::isAPredicate() const { return TRUE; }

// ----------------------------------------------------------------------
// Walk through an ItemExpr tree and gather the ValueIds of those
// expressions that behave as if they are "leaves" for the sake of
// the coverage test, e.g., expressions that have no children, or
// aggregate functions, or instantiate null. These are usually values
// that are produced in one "scope" and referenced above that "scope"
// in the dataflow tree for the query.
// ----------------------------------------------------------------------
void VEGPredicate::getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                             const GroupAttributes& coveringGA,
                                             const ValueIdSet & newExternalInputs) const
{
  leafValues += ((VEGPredicate *)this)->getVEG()->getVEGReference()->getValueId();
} // VEGPredicate::getLeafValuesForCoverTest()

NABoolean VEGPredicate::isCovered(const ValueIdSet& newExternalInputs,
				  const GroupAttributes& coveringGA,
				  ValueIdSet& referencedInputs,
				  ValueIdSet& /* coveredSubExpr*/,
				  ValueIdSet& /* unCoveredExpr*/ ) const
{
  ValueId VEGRefId = getVEG()->getVEGReference()->getValueId();
  ValueIdSet vegMembers = getVEG()->getAllValues();
  NABoolean covered = FALSE;

  // A VEGPredicate is covered if the child can supply the VEGRefId.
  // If the VEGRefId is only supplied by the inputs we want to return false.

  // The VEGRefId will be part of the child's required outputs since
  // the parent needed that to evaluate the VEGPredicate.
  // So simply check if the VEGRefId is part of the required outputs.
  // However, when FileScanRule::nextSubstitute is trying to determine
  // if a index is useful, the required output simply contains the
  // list of indexcol valueId's, and has not been remapped to the
  // VEGRefId's that correspond in that region. So check to see if
  // the indexcol intersects with the VEG members.

  // We cannot call coveringGA.covers() in this case because it will be fooled
  // by the presences of one of the members of the VEG in the required
  // inputs or newExternalInputs.

  if (coveringGA.isCharacteristicOutput(VEGRefId))
    {
      covered = TRUE;
    }
  else
    {
      // Check if a member of the VEG is in the characteristic outputs
      ValueIdSet outputs = coveringGA.getCharacteristicOutputs();
      outputs.intersectSet(vegMembers);
      if (NOT outputs.isEmpty())
        covered = TRUE;
    }

  // If the VEGPredicate is covered we want to include in the
  // characteristic inputs the VEGRefId for the VEG if
  // available
  if (covered)
    {
      // If the VEGPredicate is covered without supplying any
      // new external inputs, indicate which of the given
      // external inputs can help for evaluating this
      // VEGPredicate here.
      if (newExternalInputs.contains(VEGRefId))
        referencedInputs += VEGRefId;
      else
        {
          const NABoolean doNotLookInsideVegReferences = FALSE ;
          const NABoolean doNotLookInsideInstantiateNulls = FALSE ;
          //
          // It is likely that some member of the VEG appears in
          // the external inputs. For example, consider
          //       t1 left join t2 on t1.x = t2.y
          // The VEG will contain (t2.y, ixcol t2.y, VEGRef(t1.x ..))
          //
          // The nested loop join transformation will supply
          // VEGRef(t1.x ..), which belongs to an outer scope, as an
          // external input. Clearly, the latter new external input does
          // not contain the VEGReference for the former VEG. In fact,
          // the reverse situation is true. The case that is sketched
          // in these comments is detected by the code that appears below:
          referencedInputs.accumulateReferencedValues
            (newExternalInputs,
             vegMembers,
             doNotLookInsideVegReferences,
             doNotLookInsideInstantiateNulls);
        }
    } // if (covered)

  // The idea is to push the predicate down only when the tree can
  // produce a member of the VEG. However, we are having too many
  // problems with the above policy. We did not want to push a
  // VEGPredicate down the tree only because the VEG references a
  // constant that is needed in the tree. However, predicates of the
  // form :hv = 10 and other predicates that only reference correlated
  // values need to be pushed to some scan.

  // $$$$ Comments (Dec. 19, 1996)
  // Check whether the predicate should be pushed down by looking at
  // the number of user supplied inputs that are members of the
  // VEG. If the VEGPredicate is not covered because the tree cannot
  // produce a member of the VEG, push it down iff there are at least
  // two user supplied inputs that are members of the VEG. Indeed,
  // this is a kludge that is meant to permit the optimization
  // i.e., evaluating predicates such as :hv =
  // 10 on some scan. However, it has the advantage of eliminating the
  // really dumb behaviour of pushing a predicate down on the strength
  // of supplying a single constant value as an input.

  // The kludge added does not work. Predicates of the form
  // :hv = 10 are not stored in a VEGPred unless there also is a
  // predicate of the form col = :hv. So, all that needs to be handled
  // here are VEGPreds that reference correlated column values but
  // do not reference values supplied by the child's char outputs.
  // For example, 31 = T0.i1, and T0 is not a child.
  else
  {
     // Check if all value ids in the vegMembers set are either
     // user supplied inputs or correlated references (an embedded
     // vegRef must be a correlated reference or it's in an left join
     // ON clause).  If it's in an left join ON clause, this is ok,
     // it just means we may push down the pred to the right child
     // of the left join if the join is a TSJ and the pred only
     // refers to the left child and user inputs. For example,
     // select t0.a from t0 left join t1 on t0.a = 10. The pred
     // t0.a = 10 will get pushed to the right child (t1) if the
     // join is a TSJ. This would not happen without this fix but
     // this should not be a problem as it is legal to push an
     // ON clause pred to the right child of a left join.
     NABoolean allVEGMembersAreInputs = TRUE;
     NABoolean outerReferencesSeen = FALSE;
     NABoolean userSuppliedInputsSeen = FALSE;
     ValueIdSet outerReferences;
     ValueIdSet userSuppliedInputs;
     for (ValueId vid = vegMembers.init();
                        vegMembers.next(vid);
                        vegMembers.advance(vid))
     {
       // Check if the veg member is not a user supplied input or
       // a correlated reference. It is a correlated reference if it
       // is an embedded vegRef or if it is an instantiate null whose
       // child is an embedded vegRef. A instantiate null with an
       // embedded vegRef child represents a correlated reference to
       // the right child output of a left join. 
       
       if (NOT (vid.getItemExpr()->isAUserSuppliedInput() OR
                (vid.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE) OR
                ((vid.getItemExpr()->getOperatorType() ==
                    ITM_INSTANTIATE_NULL) AND
                 (vid.getItemExpr()->child(0)->getOperatorType() ==
                    ITM_VEG_REFERENCE))
               )
          )
       {
	 // If this column was a host var before binding, we consider it as such
         if (vid.getItemExpr()->previousHostVar())
         {
           allVEGMembersAreInputs = TRUE;
           break;
         }
         allVEGMembersAreInputs = FALSE;
       }

       // check for outer-references in form of embedded veg-references
       if (vid.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
       {
         outerReferencesSeen = TRUE;
         outerReferences +=vid;
       }
       else if (vid.getItemExpr()->isAUserSuppliedInput())
       {
         userSuppliedInputs +=vid;
         userSuppliedInputsSeen = TRUE;
       }
     } // for

     if (outerReferencesSeen &&
         NOT allVEGMembersAreInputs)
     {
       // This VEGPredicate has an outer reference in the VEG Members set.
       // In addition the set also has a NON-UserSuppliedInput such
       // as a base column or an index column (extra columns);
       // We already know that these extra columns are not part of the output
       // (A check is made at the beginning of this routine)
       //  Check if these are part of input and set covered to TRUE only when
       // they are NOT part of characteristic input set
       // CR 10-010314-1732

       if ( newExternalInputs.contains(VEGRefId) &&
            userSuppliedInputsSeen
          )
       {
         // VegMembers contain an user Supplied input and an outer Reference
         ValueIdSet inputset = vegMembers;
         inputset -= outerReferences;
         inputset -= userSuppliedInputs;
         inputset.intersectSet(newExternalInputs);

         if (inputset.isEmpty())
         {
           covered = TRUE;
           referencedInputs += VEGRefId;
         }
       }
     }

     // Allow the predicate to be pushed down if all vegMembers are
     // user supplied inputs or correlated references, and the VEGRef
     // or at least one of it's constiuent parts are covered by the
     // char. inputs.
     if (allVEGMembersAreInputs)
     {
       if (coveringGA.isCharacteristicInput(VEGRefId))
       {
         covered = TRUE;
       }
       if (newExternalInputs.contains(VEGRefId))
       {
         covered = TRUE;
         referencedInputs += VEGRefId;
       }
       if (NOT covered)
       {
         vegMembers.intersectSet(newExternalInputs);
         if (NOT vegMembers.isEmpty())
         {
           covered = TRUE;
           referencedInputs += vegMembers;
         }
       }
     } // end if all veg members are inputs
  } // end if not covered by outputs

  return covered;

} // VEGPredicate::isCovered()

const NAString VEGPredicate::getText() const
{
  char cp[TEXT_DISPLAY_LENGTH];

  // getenv() calls are costly. avoid especially in release code.
#ifndef NDEBUG
  if (getenv("SIMPLE_VEG_DISPLAY") || getenv("SIMPLE_DISPLAY"))
  {
    sprintf(cp,"VP%d",CollIndex(getValueId()));
    return NAString(cp) + veg_.getItemExpr()->getText();
  }
#endif

  //sprintf(cp,"VEGPred_%d(",CollIndex(getValueId()));
  cp[0] = 0;
  return NAString(cp) + veg_.getItemExpr()->getText(); // + ")";
} // VEGPredicate::getText()

// MVs -- 
void VEGPredicate::unparse(NAString &result,
			   PhaseEnum phase,
			   UnparseFormatEnum form,
			   TableDesc * tabId) const
{
  if ((form != MVINFO_FORMAT) &&
      (form != QUERY_FORMAT))
  {
    ItemExpr::unparse(result, phase, form, tabId);
    return;
  }

  const ValueIdList vegMembers(getVEG()->getAllValues());

  CMPASSERT(vegMembers.entries() >=2);
  ValueIdList copyList;  

  CollIndex startIndex;
  if (form == QUERY_FORMAT || form == MVINFO_FORMAT)
    startIndex = 0;
  else 
    startIndex = 1;

  ValueId  nextMemberId;
  for (CollIndex i=startIndex; i<vegMembers.entries(); i++)
  {
    nextMemberId = vegMembers[i];
    ItemExpr *nextExpr = nextMemberId.getItemExpr();
    if (nextExpr->getOperatorType() == ITM_INDEXCOLUMN)
      continue;
    if ((form == QUERY_FORMAT) &&
        (nextExpr->getOperatorType() == ITM_BASECOLUMN))
    {
      BaseColumn * bs = (BaseColumn *)nextExpr;
      if (bs->getTableDesc() != tabId && form != MVINFO_FORMAT)
        continue;
    }  
    copyList.insert(vegMembers[i]);
  }  

  if (copyList.entries() < 2) 
    return;

  ValueId  firstMemberId = copyList[0];
  NAString firstMemberText;
  firstMemberId.getItemExpr()->unparse(firstMemberText, phase, form, tabId);

  NAString nextMemberText("empty");
  for (CollIndex i=1; i<copyList.entries(); i++)
  {
    nextMemberId = copyList[i];

    // If this is not the first predicate, "AND" between predicates.
    if (nextMemberText != "empty")
      result += " AND ";

    nextMemberText = "";
    nextMemberId.getItemExpr()->unparse(nextMemberText, phase, form, tabId);

    result += firstMemberText;
    result += " = ";
    result += nextMemberText;
  }
}

HashValue VEGPredicate::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= getVEG()->topHash();

  return result;
} // VEGPredicate::topHash()

NABoolean VEGPredicate::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  // compare any local data members of the derived class
  // and return FALSE if they don't match
  if (veg_ != ((VEGPredicate &)other).veg_)
    return FALSE;

  return TRUE;
} // VEGPredicate::duplicateMatch()

ItemExpr * VEGPredicate::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result = NULL;

  if (derivedNode == NULL)
    //ABORT("No methods for copying this object yet");
    result = new VEGPredicate(*this);
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
} // VEGPredicate::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class VEGReference
// -----------------------------------------------------------------------
VEGReference::VEGReference(const ValueId & ofVEG)
            : ItemExpr(ITM_VEG_REFERENCE), veg_(ofVEG)
{
  CMPASSERT (ofVEG.getItemExpr()->getOperatorType() == ITM_VEG);
  allocValueId();
}

VEGReference::~VEGReference() {}

Int32 VEGReference::getArity() const { return 0; }

void VEGReference::replaceVEG(const ValueId& vegId)
{
  CMPASSERT(vegId.getItemExpr()->getOperatorType() == ITM_VEG);
  veg_ = vegId;
} // VEGReference::replaceVEG()

NABoolean VEGReference::isCovered(const ValueIdSet& newExternalInputs,
				  const GroupAttributes& coveringGA,
				  ValueIdSet& referencedInputs,
				  ValueIdSet& coveredSubExpr,
				  ValueIdSet& unCoveredExpr ) const
{
  // If we arrive here to check whether a VEGReference isCovered(),
  // it means that this VEGReference does not belong to the
  // Characteristic Inputs and Outputs. (Otherwise, the test in
  // covers() would have detected its presence.)
  // Walk through all the members of the set in order to return an
  // indication of which values are referenced inputs, which values
  // are covered sub expressions and which of them are uncovered expr.

  NABoolean retVal = FALSE;

  VEG *veg = getVEG();

  if (veg->seenBefore())
    return FALSE;

  // Remember valueId of the VEGReference that we've already seen.
  veg->markAsSeenBefore();

  for (ValueId x = veg->getAllValues().init();
                   veg->getAllValues().next(x);
                   veg->getAllValues().advance(x))
  {
    // I am covered if ANY ONE of my candidate values is covered
    if (coveringGA.covers(x, newExternalInputs, referencedInputs,
                          &coveredSubExpr, &unCoveredExpr))
    {
      retVal = TRUE;
      break;
    }
  }

  veg->markAsNotSeenBefore();
  return retVal;

} // VEGReference::isCovered()

OrderComparison VEGReference::sameOrder(ItemExpr * other,
					NABoolean /*askOther*/)
{
  // first, check whether the item expressions are identical
  if (this == other)
    return SAME_ORDER;

  // We can have circular VegRefs like :
  // VEGRef_285(TAB4.SSTUTKYOKI = VEGRef_310(TAB9.TKYOKI = VEGRef_285(..)))
  // In this case, sameOrder()will go into infinite loop (soln 10-110911-7326).
  // Not sure why we ended up constructing circular VegRefs in the first place,
  // but ItemExpr::referencesTheGivenValue() code indicates it's possible
  // and handles by marking Vegrefs as seen before and back out if VegRef is
  // being seen second time. Same logic is also used here as explained below:
  // 1. if VegRef has not seen before, mark it as seen before, also remember it.
  // 2. for every veg member who is of type vegreference and seen before,
  //    do not call sameOrder() again.
  // 3. if this method has called markAsSeenBefore(), then call
  //    markAsNotSeenBefore()
  NABoolean callNotSeen = FALSE;
  VEG *veg = getVEG();
  // step 1
  if (!veg->seenBefore())
  {
    veg->markAsSeenBefore();
    callNotSeen = TRUE;
  }

  // VEGReferences can be used for ordering comparison under the
  // assumption that all comparison predicates that can be applied
  // actually have been applied. For example, if we want "order by t1.x"
  // and t1.x is in a VEG with t2.y, then we can use an existing
  // order t2.y if we can assume that t1.x has already been compared
  // with t2.y. Note that we want order t1.x we should have t1.x
  // available at that time.
  const ValueIdSet &equivValues = getVEG()->getAllValues();
  OrderComparison result = DIFFERENT_ORDER;

  for (ValueId x = equivValues.init();
       equivValues.next(x) AND result == DIFFERENT_ORDER;
       equivValues.advance(x))
    {
      if((x.getItemExpr()->getOperatorType()) == ITM_VEG_REFERENCE)
      {
        // step 2
        VEG *innerVeg = ((VEGReference *)x.getItemExpr())->getVEG();
        if (!innerVeg->seenBefore())
          result = x.getItemExpr()->sameOrder(other,TRUE);
      }
      else
        result = x.getItemExpr()->sameOrder(other,TRUE);
    }

  // step 3
  if (callNotSeen)
    veg->markAsNotSeenBefore();

  return result;
}

// ----------------------------------------------------------------------
// Walk through members of VegReference and verify if any member of this
// VegReference, which is of TYPE ITM_VEG_REFERENCE contains passed
// argument. Also dig deep inside all ITM_VEG_REFERENCE objects to avoid
// circular vegRefences like :
// VEGRef_285((T4.SSTUTKYOKI = VEGRef_310((T9.TKYOKI = VEGRef_285(...))))
// This method gets called from VEGRegion::replaceVEGMember() and fixes
// bug QC_1348
// ----------------------------------------------------------------------
NABoolean VEGReference::referencesVegRefValue(ValueId& ofVegRef)
{
  // Get all members of VEGRef:
  const ValueIdSet &vegGroup = getVEG()->getAllValues();
  VEG *oldVEGPtr = ((VEGReference *)ofVegRef.getItemExpr())->getVEG();
  for (ValueId vid = vegGroup.init(); vegGroup.next(vid); vegGroup.advance(vid))
  {
    if (vid.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
    {
      VEG *newVEGPtr = ((VEGReference *) (vid.getItemExpr()))->getVEG();
      if (newVEGPtr == oldVEGPtr)
        return TRUE;
      else
        return ((VEGReference *) (vid.getItemExpr()))->referencesVegRefValue(ofVegRef);
    }
  }
  return FALSE;

}


const NAString VEGReference::getText() const
{
  char cp[TEXT_DISPLAY_LENGTH];
  sprintf(cp,"VEGRef_%d(",CollIndex(getValueId()));

  // getenv() calls are costly. avoid especially in release code.
#ifndef NDEBUG
  NABoolean simpleDisplay = FALSE;
  simpleDisplay = getenv("SIMPLE_VEG_DISPLAY") || getenv("SIMPLE_DISPLAY");
  if (simpleDisplay)
    sprintf(cp,"VR%d",CollIndex(getValueId()));
#endif
  NAString x(NAString(cp, CmpCommon::statementHeap())+
              veg_.getItemExpr()->getText(),
            CmpCommon::statementHeap());
  x += ")";
#ifndef NDEBUG
  if (simpleDisplay)
    x.remove(x.length()-1);
#endif
  return x;
} // VEGReference::getText()

void VEGReference::unparse(NAString &result,
			   PhaseEnum phase,
			   UnparseFormatEnum form,
			   TableDesc * tabId) const
{
  if ((form == EXPLAIN_FORMAT) || (form == MVINFO_FORMAT) ||
       (form == QUERY_FORMAT))
    {
      // End users won't know what a VegReference is, and
      // most items in EXPLAIN are already rewritten w/o VEGies.
      // Generate equiv(col) where col is the unparsed text
      // of some VEG member.

      /*if (form == EXPLAIN_FORMAT)
	result += "equiv(";
      */
      const ValueIdSet &vegMembers =
	((VEG *) veg_.getItemExpr())->getAllValues();

      for (ValueId someMemberId = vegMembers.init();
	   vegMembers.next(someMemberId);
	   vegMembers.advance(someMemberId))
        {
	  ItemExpr * someMemberExpr = someMemberId.getItemExpr();
	  if (form == QUERY_FORMAT)
	  {
	    // for QUERY_FORMAT, unparse the VEG member that belongs to the
	    // given tableDesc
	    if ((someMemberExpr->getOperatorType() == ITM_BASECOLUMN) &&
	        (((BaseColumn *)someMemberExpr)->getTableDesc() == tabId) ||
                 (CmpCommon::getDefault(MVQR_LOG_QUERY_DESCRIPTORS) == DF_DUMP_MV))
	    {
		someMemberExpr->unparse(result, phase, form, tabId);
		break;
	    }
	    else
	      continue;
	  }
	  else
	  {
	    // if not QUERY_FORMAT, unparse the first veg member
            someMemberExpr->unparse(result,phase,form);
	    break;
	  }
        }
      /*
      if (form == EXPLAIN_FORMAT)
        result += ") ";
	*/
    }
  else
    {
      result += getText();
    }
}

HashValue VEGReference::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= getVEG()->topHash();

  return result;
} // VEGReference::topHash()

NABoolean VEGReference::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  // compare any local data members of the derived class
  // and return FALSE if they don't match
  if (veg_ != ((VEGReference &)other).veg_)
    return FALSE;

  return TRUE;
} // VEGReference::duplicateMatch()

ItemExpr * VEGReference::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result = NULL;

  if (derivedNode == NULL)
    //ABORT("No methods for copying this object yet");
    result = new (outHeap) VEGReference(getVEG()->getValueId());
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
} // VEGReference::copyTopNode()

QR::ExprElement VEGReference::getQRExprElem() const
{
  return QR::QRColumnElem;
}

// -----------------------------------------------------------------------
// member functions for class CheckConstraint
// -----------------------------------------------------------------------
Int32 CheckConstraint::getArity() const { return 0;}

ItemExpr * CheckConstraint::copyTopNode(ItemExpr *derivedNode,
					CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) CheckConstraint(*this, outHeap);
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

const NAString CheckConstraint::getText() const
{
  return "CheckConstraint";
}

void CheckConstraint::unparse(NAString &result,
			     PhaseEnum /* phase */,
			     UnparseFormatEnum /* form */,
			     TableDesc * tabId) const
{
  result += getText();
  result += "(";
  result += getConstraintName().getQualifiedNameAsAnsiString();
  result += ")";
}

// -----------------------------------------------------------------------
// member functions for class OptConstraint
// -----------------------------------------------------------------------
ItemExpr * OptConstraint::copyTopNode(ItemExpr *derivedNode,
				      CollHeap* outHeap)
{
  return ItemExpr::copyTopNode(derivedNode, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class CardConstraint
// -----------------------------------------------------------------------
Int32 CardConstraint::getArity() const { return 0;}

ItemExpr * CardConstraint::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) CardConstraint(lowerBound_,upperBound_);
  else
    result = derivedNode;

  return OptConstraint::copyTopNode(result, outHeap);
}

const NAString CardConstraint::getText() const
{
  return "CardConstraint";
}

void CardConstraint::unparse(NAString &result,
			     PhaseEnum /* phase */,
			     UnparseFormatEnum /* form */,
			     TableDesc * tabId) const
{
  char ascii[TEXT_DISPLAY_LENGTH];

  sprintf(ascii,"CardConstraint(%g,%g)",lowerBound_,upperBound_);
  result += ascii;
}

// -----------------------------------------------------------------------
// member functions for class UniqueOptConstraint
// -----------------------------------------------------------------------
Int32 UniqueOptConstraint::getArity() const { return 0;}

ItemExpr * UniqueOptConstraint::copyTopNode(ItemExpr *derivedNode,
					    CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) UniqueOptConstraint(uniqueCols_);
  else
    result = derivedNode;

  return OptConstraint::copyTopNode(result, outHeap);
}

const NAString UniqueOptConstraint::getText() const
{
  return "UniqueOptConstraint";
}

void UniqueOptConstraint::unparse(NAString &result,
				  PhaseEnum phase,
				  UnparseFormatEnum form,
				  TableDesc * tabId) const
{
  result += "UniqueOptConstraint";
  uniqueCols_.unparse(result,phase,form);
}

// -----------------------------------------------------------------------
// member functions for class FuncDependencyConstraint
// -----------------------------------------------------------------------
Int32 FuncDependencyConstraint::getArity() const
{
  return 0;
}

ItemExpr * FuncDependencyConstraint::copyTopNode(ItemExpr *derivedNode,
						 CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) FuncDependencyConstraint(determiningCols_,
						    dependentCols_);
  else
    result = derivedNode;

  return OptConstraint::copyTopNode(result, outHeap);
}

void FuncDependencyConstraint::synthFunctionalDependenciesFromChild(
     GroupAttributes &ga,
     const RelExpr *child,
     NABoolean createNewDependencies)
{
  const ValueIdSet & childConstraints =
    child->getGroupAttr()->getConstraints();
  ValueIdSet vegColsFromChild(child->getGroupAttr()->
			      getCharacteristicOutputs());

  // For now we want to maintain only VEGReferences as the dependent
  // columns. This is mainly done to avoid unusual situations
  // (monkeys at the keyboard) that might lead to errors. Restricting
  // functional dependencies to VEGRefs should still cover the majority
  // of all interesting situations.
  if (createNewDependencies)
    {
      for (ValueId co = vegColsFromChild.init();
	   vegColsFromChild.next(co);
	   vegColsFromChild.advance(co))
	{
	  if (co.getItemExpr()->getOperatorType() != ITM_VEG_REFERENCE)
	    vegColsFromChild -= co;
	}
    }

  // walk through the child constraints
  for (ValueId cx = childConstraints.init();
       childConstraints.next(cx);
       childConstraints.advance(cx))
    {
      ItemExpr *c = cx.getItemExpr();
      OperatorTypeEnum optype = c->getOperatorType();

      if (optype == ITM_UNIQUE_OPT_CONSTRAINT AND
	  createNewDependencies AND
	  NOT vegColsFromChild.isEmpty())
	{
	  UniqueOptConstraint *uc = ((UniqueOptConstraint *)c);

	  // Try to find unique columns in the child that are no longer
	  // unique in the parent. For each such uniqueness constraint,
	  // add a functional dependency constraint if the unique columns
	  // are visible to the parent.

	  // NOTE: At some later point we should consider taking out
	  // the check on the characteristic outputs, since it may
	  // suppress some interesting constraints (we don't check
	  // uniqueness constraints for being covered by char. outputs either)
	  // Example: select a from t1 join t2 may get
	  // different uniqueness constraints (and cardinality estimates)
	  // than select * from t1 join t2.

	  if (NOT ga.isUnique(uc->uniqueCols()) &&
	      child->getGroupAttr()->getCharacteristicOutputs().contains(
		   uc->uniqueCols()))
	    {
	      // yes, found unique columns that are visible to the parent,
	      // now check whether all unique columns are VEGRefs
	      // (sorry, we only consider those for now)
	      NABoolean NonVEGFound = FALSE;

	      for (ValueId ux=uc->uniqueCols().init();
		   uc->uniqueCols().next(ux);
		   uc->uniqueCols().advance(ux))
		if (ux.getItemExpr()->getOperatorType() != ITM_VEG_REFERENCE)
		  NonVEGFound = TRUE;

	      if (NOT NonVEGFound)
		{
		  ValueIdSet myDependentCols(vegColsFromChild);

		  // remove the unique columns from the dependent columns
		  // (don't want functional dependencies like A --> A)
		  myDependentCols -= uc->uniqueCols();

		  if (NOT myDependentCols.isEmpty())
		    {
		      FuncDependencyConstraint *nc =
			new(CmpCommon::statementHeap())
			FuncDependencyConstraint(uc->uniqueCols(),
						 myDependentCols);
		      ga.addConstraint(nc);
		    }
		}
	    }
	}
      else if (optype == ITM_FUNC_DEPEND_CONSTRAINT)
	{
	  // Found a functional dependency in the child constraints.
	  // Copy it if the determining columns are visible in the
	  // parent node.
	  FuncDependencyConstraint *fd = (FuncDependencyConstraint *) c;

	  // NOTE: At some later point we should consider taking out
	  // the check on the characteristic outputs (same as above).

	  if (child->getGroupAttr()->getCharacteristicOutputs().contains(
		   fd->getDeterminingCols()))
	    {
	      ga.addConstraint(c);
	    }
	}
    }
}

void FuncDependencyConstraint::minimizeUniqueCols(ValueIdSet &uniqueCols)
{
  if (uniqueCols.contains(determiningCols_))
    {
      // We know now that the uniqueCols determine our "dependentCols_".

      // As an example, let's assume that uniqueCols is (a,b,c,d),
      // determiningCols_ is (a,b), and dependentCols_ is (c).

      // If columns (a,b) determine (c), then it is true that if
      // (a,b,c) is unique then (a,b) is unique as well. This is
      // because the definition of functional dependency says that
      // there will never be two rows with the same (a,b) values that
      // have different (c) values.

      // The extra column (d) in the example does not invalidate this
      // argument.

      // at this point we only use simple ValueId operations, no
      // cover test methods for checks like "does (a,b) determine (a+c+2)?"

      // Could assert here that the intersection of determiningCols_ and
      // dependentCols_ is empty (more precisely: that the dependent columns
      // don't cover any of the unique columns, but we don't have the
      // necessary information to check this here). See method
      // FuncDependencyConstraint::createFunctionalDependenciesFromChild()
      // above for where this constraint is enforced.

      uniqueCols.subtractSet(dependentCols_);
    }
}

const NAString FuncDependencyConstraint::getText() const
{
  return "FuncDependencyConstraint";
}

void FuncDependencyConstraint::unparse(NAString &result,
				       PhaseEnum phase,
				       UnparseFormatEnum form,
				       TableDesc * tabId) const
{
  // use EXPLAIN format since we only talk about VEGRefs and
  // one member should be sufficient to represent the VEG
  result += "FuncDependencyConstraint(";
  determiningCols_.unparse(result,phase,EXPLAIN_FORMAT);
  result += " ---> ";
  dependentCols_.unparse(result,phase,EXPLAIN_FORMAT);
  result += ")";
}


// -----------------------------------------------------------------------
// member functions for class CheckOptConstraint
// -----------------------------------------------------------------------
CheckOptConstraint::~CheckOptConstraint() {}

Int32 CheckOptConstraint::getArity() const { return 0; }

ItemExpr * CheckOptConstraint::copyTopNode(ItemExpr *derivedNode,
                                           CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) CheckOptConstraint(checkPreds_);
  else
    result = derivedNode;

  return OptConstraint::copyTopNode(result, outHeap);
}

const NAString CheckOptConstraint::getText() const
{
  return "CheckOptConstraint";
}

void CheckOptConstraint::unparse(NAString &result,
                                 PhaseEnum phase,
                                 UnparseFormatEnum form,
                                 TableDesc * tabId) const
{
  result += "CheckOptConstraint";
  checkPreds_.unparse(result,phase,form);
}

// -----------------------------------------------------------------------
// member functions for class RefOptConstraint
// -----------------------------------------------------------------------
Int32 RefOptConstraint::getArity() const { return 0;}

ItemExpr * RefOptConstraint::copyTopNode(ItemExpr *derivedNode,
					    CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL) 
  {
    result = new (outHeap) RefOptConstraint(fkCols_, uniqueConstraintName_);
    ((RefOptConstraint*)result)->isMatched_ = isMatched_ ;
  }
  else
    result = derivedNode;

  return OptConstraint::copyTopNode(result, outHeap);
}

const NAString RefOptConstraint::getText() const
{
  return "RefOptConstraint";
}

void RefOptConstraint::unparse(NAString &result,
				  PhaseEnum phase,
				  UnparseFormatEnum form,
				  TableDesc * tabId) const
{
  result += "RefOptConstraint";
  fkCols_.unparse(result,phase,form);
  result += "(";
  result += uniqueConstraintName().getQualifiedNameAsAnsiString();
  result += ")";
}

// -----------------------------------------------------------------------
// member functions for class ComplementaryRefOptConstraint
// -----------------------------------------------------------------------
Int32 ComplementaryRefOptConstraint::getArity() const { return 0;}

ItemExpr * ComplementaryRefOptConstraint::copyTopNode(ItemExpr *derivedNode,
					    CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ComplementaryRefOptConstraint(ucCols_, 
	    constraintName_, tabPtr_, tabDesc_, NULL, isMatchedForElimination_);
  else
    result = derivedNode;

  return OptConstraint::copyTopNode(result, outHeap);
}

const NAString ComplementaryRefOptConstraint::getText() const
{
  return "ComplementaryRefOptConstraint";
}

void ComplementaryRefOptConstraint::unparse(NAString &result,
				  PhaseEnum phase,
				  UnparseFormatEnum form,
				  TableDesc * tabId) const
{
  result += "ComplementaryRefOptConstraint";
  ucCols_.unparse(result,phase,form);
  result += "(";
  result += constraintName().getQualifiedNameAsAnsiString();
  result += ")";
}

// -----------------------------------------------------------------------
// member functions for class Aggregate
// -----------------------------------------------------------------------
Aggregate::~Aggregate() {}

NABoolean Aggregate::isAnAggregate() const { return TRUE; }

NABoolean Aggregate::containsAnAggregate() const { return TRUE; }

Int32 Aggregate::getArity() const
{
  // ##Should be same as Function::getNumChildren(), except added CMPASSERT here
  // (aggregates must have at least one child; stddev/variance can have more).

  if (!child(1)) return 1;	// most aggs have 1 child
  CMPASSERT(!child(2));		// no aggs have 3+ children
  return 2;			// STDDEV/VARIANCE may have 1 or 2
}

NABoolean Aggregate::operator== (const ItemExpr& other) const	// virtual meth
{
  if (ItemExpr::operator==(other)) return TRUE;

  if (getOperatorType() != other.getOperatorType()) return FALSE;

  const Aggregate &otherAgg = (const Aggregate &)other;

  if (getOperatorType() == ITM_MIN || getOperatorType() == ITM_MAX)
    {
      // Min/Max are order-sensitive
      ItemExprList arglist0(child(0), NULL);
      ItemExprList arglist1(otherAgg.child(0), NULL);
      if (arglist0.entries() != arglist1.entries()) return FALSE;
      for (CollIndex i = arglist0.entries(); i--; )
        if (arglist0[i] != arglist1[i]) return FALSE;
    }
  else
    {
      ValueIdSet argset0, argset1;
      child(0)->convertToValueIdSet(argset0, NULL, ITM_ITEM_LIST);
      otherAgg.child(0)->convertToValueIdSet(argset1, NULL, ITM_ITEM_LIST);
      if (argset0 != argset1) return FALSE;
    }

  return TRUE;
}

NABoolean Aggregate::duplicateMatch(const ItemExpr& other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  const Aggregate &ag = (Aggregate &)other;

  if (isDistinct_ != ag.isDistinct_)
    return FALSE;

  if (NOT isSensitiveToDuplicates())
    return FALSE;

  return TRUE;
}

NABoolean Aggregate::isCovered
                      (const ValueIdSet& newExternalInputs,
		       const GroupAttributes& coveringGA,
		       ValueIdSet& referencedInputs,
		       ValueIdSet& coveredSubExpr,
		       ValueIdSet& /*unCoveredExpr*/ ) const
{
  // ---------------------------------------------------------------------
  // If the operand of the aggregate isCovered(), then return its ValueId
  // in coveredSubExpr.
  // ---------------------------------------------------------------------
  ValueIdSet localSubExpr;
  for (Lng32 i = 0; i < (Lng32)getArity(); i++)
    {
      if ( coveringGA.covers(child(i)->getValueId(),
			     newExternalInputs,
			     referencedInputs,
			     &localSubExpr) )
	{
	  coveredSubExpr += child(i)->getValueId();
	}
    }

  // ---------------------------------------------------------------------
  // An aggregate function is coerced to fail the coverage test even
  // when its operand isCovered(). This is because the computation of
  // the aggregate function requires "raw" values produced by its
  // operand to be grouped. The aggregated values cannot be treated
  // in the same manner as the constituent raw values.
  // ---------------------------------------------------------------------
  return FALSE;  // sorry, i am a special case

} // Aggregate::isCovered()

// ----------------------------------------------------------------------
// Walk through an ItemExpr tree and gather the ValueIds of those
// expressions that behave as if they are "leaves" for the sake of
// the coverage test, e.g., expressions that have no children, or
// aggregate functions, or instantiate null. These are usually values
// that are produced in one "scope" and referenced above that "scope"
// in the dataflow tree for the query.
// ----------------------------------------------------------------------
void Aggregate::getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                          const GroupAttributes& coveringGA,
                                          const ValueIdSet & newExternalInputs) const
{
  leafValues += getValueId();
} // Aggregate::getLeafValuesForCoverTest()

const NAString Aggregate::getText() const
{
  NAString result(CmpCommon::statementHeap());
  NABoolean dumpMvMode = (CmpCommon::getDefault(MVQR_LOG_QUERY_DESCRIPTORS) == DF_DUMP_MV);

  switch (getOperatorType())
    {
    case ITM_AVG:
      result = "avg";
      break;
    case ITM_MAX:
      result = "max";
      break;
    case ITM_MIN:
      result = "min";
      break;
    case ITM_SUM:
      result = "sum";
      break;
    case ITM_COUNT:
      result = "count";
      break;
    case ITM_COUNT_NONULL:
      if (dumpMvMode)
        result = "count";
      else
        result = "count_nonull";
      break;
    case ITM_GROUPING:
      result = "grouping";
      break;
    case ITM_ONE_ROW:
      result = "one_Row";
      break;
    case ITM_ONE_TRUE:
      result = "oneTrue";
      break;
    case ITM_ANY_TRUE:
      result = "anytrue";
      break;
    case ITM_ANY_TRUE_MAX:
      result = "anytruemax";
      break;
    case ITM_STDDEV:
      result = "stddev";
      break;
    case ITM_VARIANCE:
      result = "variance";
      break;
    case ITM_ONEROW:
      result = "oneRow";
      break;
    case ITM_PIVOT_GROUP:
      result = "pivot_group";
      break;
    default:
      result = "unknown aggr";
      break;
    } // switch

  if (isDistinct_ && !dumpMvMode) {
    result += " distinct";
    if (distinctId_ != NULL_VALUE_ID) {
      char buf[13]; // CollIndex is UInt32 with a max value of 4,294,967,295
                    // We need 10 + 2 + 1=13 bytes to store all possible
                    // values.
      snprintf(buf, sizeof(buf), "(%u)", (CollIndex)distinctId_);
      result += buf;
      // ## Perhaps a better way of displaying this would instead be:
      //	result += "(";
      //	result += distinctId_.getItemExpr()->getText();
      //	result += ")";
    }
  }

  return result;
} // Aggregate::getText()

void Aggregate::unparse(NAString &result,
			PhaseEnum phase,
			UnparseFormatEnum form,
			TableDesc * tabId) const
{
  if (form == USER_FORMAT_DELUXE)
    {
      Aggregate *ncThis = (Aggregate *)this;		// cast away constness!

      OperatorTypeEnum saveType  = getOperatorType();	// save
      ItemExpr        *saveChild = child(0);
      ValueId	       saveVid   = getOriginalChild()->getValueId();

      getOriginalChild()->setValueId(NULL_VALUE_ID);	// BEFORE setChild()!
      ncThis->setOperatorType(origOpType());		// set to "orig"
      ncThis->setChild(0, getOriginalChild());

      ItemExpr::unparse(result,phase, form, tabId);		// invoke superclass

      getOriginalChild()->setValueId(saveVid);		// BEFORE setChild()!
      ncThis->setOperatorType(saveType);		// restore
      ncThis->setChild(0, saveChild);
    }
  else
  {
    if ((CmpCommon::getDefault(MVQR_LOG_QUERY_DESCRIPTORS) == DF_DUMP_MV) && isDistinct_)
    {
      // Fix DISTINCT to be parsable.
      result += getText();
      result += "( DISTINCT ";
      child(0)->unparse(result, phase, form, tabId);
      result += ")";
    }
    else
      ItemExpr::unparse(result, phase, form, tabId);	// invoke our superclass method
  }
} // Aggregate::unparse()

ItemExpr * Aggregate::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  Aggregate *result;

  if (derivedNode == NULL)
    result = new (outHeap) Aggregate(getOperatorType());
  else
    result = (Aggregate *) derivedNode;

  if (inScalarGroupBy())
    result->setInScalarGroupBy();

  if (topPartOfAggr())
    result->setTopPartOfAggr();

  result->origChild_ = origChild_;
  result->isDistinct_ = isDistinct_;
  result->inScalarGroupBy_ = inScalarGroupBy_;
  result->distinctId_ = distinctId_;

  // Copy OLAP Window Function information.
  //
  result->isOLAP_ = isOLAP_;

  result->olapPartitionBy_ = (olapPartitionBy_ ? 
                              olapPartitionBy_->copyTree(outHeap) :
                              NULL);

  result->olapOrderBy_ = (olapOrderBy_ ?
                          olapOrderBy_->copyTree(outHeap) : 
                          NULL);

  result->frameStart_ = frameStart_;
  result->frameEnd_ = frameEnd_;

  result->rollupGroupIndex_ = rollupGroupIndex_;

  return ItemExpr::copyTopNode(result, outHeap);
}

// One heck of an implementation for a virtual function!
NABoolean Aggregate::isSensitiveToDuplicates() const
{
  switch (getOperatorType())
    {
    case ITM_MAX:
    case ITM_MIN:
    case ITM_ANY_TRUE:
    case ITM_ONE_TRUE:
    case ITM_GROUPING:
      return FALSE;

    case ITM_SUM:
    case ITM_COUNT:
    case ITM_COUNT_NONULL:
    case ITM_ONE_ROW:
    case ITM_ONEROW:
    case ITM_AVG:
    default:
      return TRUE;
    } // switch
}

// Another hack of an implementation for a virtual function!!
NABoolean Aggregate::evaluationCanBeStaged() const
{
  switch (getOperatorType())
    {
    case ITM_ONE_ROW:
      // See Aggregate::rewriteForStagedEvaluation()
      return FALSE;

    default:
      return TRUE;
    } // switch
}

ItemExpr * Aggregate::rewriteForElimination()
{
  // rewrite the aggregate function to return the result it would have
  // returned if it were called on a group with exactly one member

  // NOTE: this method doesn't delete the existing aggregate node, but
  // it may reuse some of the children of the original expression.

  ItemExpr *result = NULL;

  switch (getOperatorType())
    {
    case ITM_AVG:
    case ITM_MAX:
    case ITM_MIN:
    case ITM_SUM:
    case ITM_ONEROW:
    case ITM_ANY_TRUE:
    case ITM_PIVOT_GROUP:
      // return the value of the aggregate function's argument
      result = child(0);
      break;

    case ITM_COUNT:
      // the count of a single value is always 1
      result = new (CmpCommon::statementHeap()) SystemLiteral(1);
      break;

    case ITM_COUNT_NONULL:
      if (child(0)->getValueId().getType().supportsSQLnullLogical())
      {
        // generate a CASE expression: CASE WHEN arg IS NOT NULL THEN 1 ELSE 0
        result = new (CmpCommon::statementHeap())
	  Case(NULL,
	       new (CmpCommon::statementHeap())
	       IfThenElse(new (CmpCommon::statementHeap())
			  UnLogic(ITM_IS_NOT_NULL,
				  child(0)),
			  new (CmpCommon::statementHeap()) SystemLiteral(1),
			  new (CmpCommon::statementHeap()) SystemLiteral(0)));
      }
      else
      {
        result = new (CmpCommon::statementHeap()) SystemLiteral(1);
      }
      break;

    case ITM_ONE_TRUE:
      // return TRUE only if the argument is TRUE, FALSE otherwise
      result = new (CmpCommon::statementHeap()) UnLogic(ITM_IS_TRUE,child(0));
      break;

    default:
      ABORT("unknown aggregate function encountered");
    }

  result->synthTypeAndValueId();
  return result;
}

ItemExpr * Aggregate::rewriteForStagedEvaluation(ValueIdList &initialAggrs,
						 ValueIdList &finalAggrs,
						 NABoolean sameFormat)
{
  // Split the aggregate function into three parts: the initial part is
  // executed in multiple groupby nodes whose results are sent to a
  // single, "final" groupby node that executes the final aggregate expression.
  // More than one aggregate function may be evaluated in the initial and/or
  // final nodes, therefore a set parameter is used. The return value is
  // an item expression equivalent to the original expression "this". The
  // return value is not necessarily an aggregate function, while
  // initialAggr and finalAggr contain only aggregate functions.

  // NOTE: this method doesn't delete the existing aggregate node, and it
  // produces new value ids for all three parts returned.

  ItemExpr *result = NULL;
  Aggregate *partial;
  Aggregate *sumOfCounts;
  Aggregate *sumOfSums;

  switch (getOperatorType())
    {
    case ITM_AVG:
      assert(NOT sameFormat);

      // divide the sum of sums by the sum of counts, but return NULL
      // if the sum of counts is zero
      // in pseudo-SQL, that sounds like transforming avg(x) into:
      // CASE WHEN SUM(COUNT(x)) > 0 THEN SUM(SUM(x)) / SUM(COUNT(x))
      //      ELSE NULL

      sumOfCounts = new (CmpCommon::statementHeap())
	Aggregate(ITM_SUM,
		  new (CmpCommon::statementHeap()) Aggregate(ITM_COUNT_NONULL,
							    child(0)));
      partial = new (CmpCommon::statementHeap())
                     Aggregate(ITM_SUM, child(0));

      sumOfSums = new (CmpCommon::statementHeap())
	               Aggregate(ITM_SUM, partial);

      if (inScalarGroupBy())
      {
        partial->setInScalarGroupBy();
        sumOfSums->setInScalarGroupBy();
        sumOfCounts->setInScalarGroupBy();
        ((Aggregate *)(sumOfCounts->child(0).getPtr()))->setInScalarGroupBy();
      }

      // The sum of the counts cannot be zero if the binder typed the
      // avg as non-nullable.
      // An AVG on a non scalar group by on a non-nullable
      // expression is always non-nullable, the sum(sum(x)) is always
      // non-nullable and the sum(count(x)) is always > 0

      if (getValueId().getType().supportsSQLnullLogical())
      {
        result = new (CmpCommon::statementHeap())
	  Case(NULL,
	       new (CmpCommon::statementHeap())
	       IfThenElse(
			  new (CmpCommon::statementHeap())
			  BiRelat(ITM_GREATER,
				  sumOfCounts,
				  new (CmpCommon::statementHeap()) SystemLiteral(0)),
			  new (CmpCommon::statementHeap())
			  BiArith(ITM_DIVIDE,sumOfSums,sumOfCounts),
			  new (CmpCommon::statementHeap()) SystemLiteral()));
      }
      else
      {
        result = new (CmpCommon::statementHeap())
                     BiArith(ITM_DIVIDE,sumOfSums,sumOfCounts);
      }

      result->synthTypeAndValueId();

      // execute the initial sum and count in the initial set
      initialAggrs.insert(sumOfCounts->child(0)->getValueId());
      initialAggrs.insert(sumOfSums->child(0)->getValueId());
      finalAggrs.insert(sumOfCounts->getValueId());
      finalAggrs.insert(sumOfSums->getValueId());
      break;

    case ITM_COUNT:
    case ITM_COUNT_NONULL:
      // compute the sum of the counts
      partial = new (CmpCommon::statementHeap())
                  Aggregate(getOperatorType(),child(0));

      sumOfCounts = new (CmpCommon::statementHeap())
                  Aggregate(ITM_SUM, partial);

      if (inScalarGroupBy())
      {
        partial->setInScalarGroupBy();
        sumOfCounts->setInScalarGroupBy();
      }
      sumOfCounts->setTreatAsACount();
      sumOfCounts->setTopPartOfAggr();
      result = sumOfCounts;

      result->synthTypeAndValueId();

      // If we have to force the same type for the initial and final
      // aggregate expressions, then force the type of the sum to
      // be the same as the type of the count.
      if (sameFormat)
	result->getValueId().changeType(&(result->child(0)->
					getValueId().getType()));

      // execute the nested aggregate function in the initial set
      initialAggrs.insert(result->child(0)->getValueId());
      finalAggrs.insert(result->getValueId());
      break;

    case ITM_MAX:
    case ITM_MIN:
    case ITM_SUM:
    case ITM_ANY_TRUE:
    case ITM_ONEROW:
    case ITM_GROUPING:
      // in these cases, just do the same aggregate function twice
      partial = new (CmpCommon::statementHeap())
                     Aggregate(getOperatorType(), child(0));

      result = new (CmpCommon::statementHeap())
                    Aggregate(getOperatorType(), partial);

      if (getOperatorType() == ITM_GROUPING)
        {
          ((Aggregate *)partial)->setRollupGroupIndex(getRollupGroupIndex());
          ((Aggregate *)result)->setRollupGroupIndex(getRollupGroupIndex());
        }

      if (inScalarGroupBy())
      {
        partial->setInScalarGroupBy();
        ((Aggregate *)result)->setInScalarGroupBy();
      }
      ((Aggregate *)result)->setTopPartOfAggr();

      // fix case 10-081203-5622, soln 10-081203-7701 by preserving this'
      // "treatAsACount_ & amTopPartOfAggr_" attribute settings.
      if (treatAsACount()) {
        partial->setTreatAsACount();
        ((Aggregate*)result)->setTreatAsACount();
      }
      if (topPartOfAggr()) {
        partial->setTopPartOfAggr();
      }
        
      result->synthTypeAndValueId();

      // If we have to force the same type for the initial and final
      // aggregate expressions, then force the type of the sum to
      // be the same as the type of the sum/min/max etc...
      if (sameFormat)
	result->getValueId().changeType(&(result->child(0)->
					  getValueId().getType()));

      // execute the nested aggregate function in the initial set
      initialAggrs.insert(result->child(0)->getValueId());
      finalAggrs.insert(result->getValueId());
      break;

    case ITM_ONE_TRUE:
      partial = new (CmpCommon::statementHeap())
                     Aggregate(getOperatorType(), child(0));

      result = new (CmpCommon::statementHeap())
                    Aggregate(ITM_ANY_TRUE, partial);

      if (inScalarGroupBy())
      {
        partial->setInScalarGroupBy();
        ((Aggregate *)result)->setInScalarGroupBy();
      }

      result->synthTypeAndValueId();

      // If we have to force the same type for the initial and final
      // aggregate expressions, then force the type of the sum to
      // be the same as the type of the sum/min/max etc...
      if (sameFormat)
	result->getValueId().changeType(&(result->child(0)->
					  getValueId().getType()));

      // execute the nested aggregate function in the initial set
      initialAggrs.insert(result->child(0)->getValueId());
      finalAggrs.insert(result->getValueId());
      break;

    case ITM_ONE_ROW:
      // need another aggregate function that handles the case where
      // some servers return NULL rows
      ABORT("sorry, one row aggregates cannot be implemented in a staged fashion");
      break;

    default:
      ABORT("unknown aggregate function encountered");
    }

  return result;
}

ItemExpr * PivotGroup::rewriteForStagedEvaluation(ValueIdList &initialAggrs,
                                                  ValueIdList &finalAggrs,
                                                  NABoolean sameFormat)
{
  // Split the aggregate function into three parts: the initial part is
  // executed in multiple groupby nodes whose results are sent to a
  // single, "final" groupby node that executes the final aggregate expression.
  // More than one aggregate function may be evaluated in the initial and/or
  // final nodes, therefore a set parameter is used. The return value is
  // an item expression equivalent to the original expression "this". The
  // return value is not necessarily an aggregate function, while
  // initialAggr and finalAggr contain only aggregate functions.

  // NOTE: this method doesn't delete the existing aggregate node, and it
  // produces new value ids for all three parts returned.

  ItemExpr *result = NULL;
  Aggregate *partial;

  partial = new (CmpCommon::statementHeap())
    PivotGroup(getOperatorType(), child(0), pivotOptionsList_, isDistinct());
  
  result = new (CmpCommon::statementHeap())
    PivotGroup(ITM_PIVOT_GROUP, partial, NULL);
  
  if (inScalarGroupBy())
    {
      partial->setInScalarGroupBy();
      ((Aggregate *)result)->setInScalarGroupBy();
    }
  
  result->synthTypeAndValueId();
  
  // If we have to force the same type for the initial and final
  // aggregate expressions, then force the type of the sum to
  // be the same as the type of the sum/min/max etc...
  if (sameFormat)
    result->getValueId().changeType(&(result->child(0)->
					  getValueId().getType()));
  
  // execute the nested aggregate function in the initial set
  initialAggrs.insert(result->child(0)->getValueId());
  finalAggrs.insert(result->getValueId());
  
  return result;
}

// Aggregate::isEquivalentForBinding Determine if these two
// ItemExprs are equivalent.  If so, one may be eliminated and
// replaced with the other.
// Inputs:
//   ItemExpr *other - the ItemExpr being compared to 'this'
//
// Outputs: return value: Returns TRUE if the 'other' ItemExpr is
// equivalent to 'this'.  In order to be equivalent, they must:
//     - Have equivalent children
//       (determined by 'hasBaseEquivalenceForCodeGeneration()
//     - Be the same aggregate. (same operatorType)
//       (determined by 'hasBaseEquivalenceForCodeGeneration()
//     - have the same distinct settings and if distinct, must both
//       be distinct with respect to the immediate child.  This could
//       prevent otherwise equivalent distinct STDDEV and VARIANCE aggregates
//       from being eliminated.
//
NABoolean Aggregate::isEquivalentForBinding(const ItemExpr * other)
{

  // Make sure that the children are equivalent and that this and
  // other are the same operator
  //
  if (hasBaseEquivalenceForCodeGeneration(other))
    {
      // we know that other is an Aggregate, its operator type is the same,
      // and that the children are equivalent
      Aggregate * otherAggregate = (Aggregate *)other;

      // Both must have the same distinct settings.
      //
      if(isDistinct() != otherAggregate->isDistinct()) {
        return FALSE;
      }

      // Both must have the same inScalarGroupBy settings.
      //
      if(inScalarGroupBy() != otherAggregate->inScalarGroupBy()) {
        return FALSE;
      }

      // If both are distinct (sufficient to check one since we know
      // both are the same).
      //
      if(isDistinct()) {

        // If they are distinct, both must be distinct on the
        // immediate child.  VARIANCE and STDDEV are transformed into
        // a more complicated expression and the distinct valueid will
        // not be the immediate child.  So this means that the
        // distinct versions of VARIANCE and STDDEV will not be
        // considered for elimination even though they may qualify.
        // Furthermore, the 'this' version of a distinct VARIANCE or
        // STDDEV will not have its distinctID set properly until
        // after this routine is called (but the 'other' one will).
        //
        if(getDistinctValueId() != child(0)->getValueId()) {
          return FALSE;
        }

        if(otherAggregate->getDistinctValueId() !=
           otherAggregate->child(0)->getValueId()) {
          return FALSE;
        }
      }

      // the two values are equivalent.
      return TRUE;
    }

  return FALSE;
}

// Return the equivalent running sequence function operator type
//
OperatorTypeEnum Aggregate::mapOperTypeToRunning() const
{
  switch(getOperatorType()) {
  case ITM_AVG:
    return ITM_RUNNING_AVG;
    break;
  case ITM_COUNT:
    return ITM_RUNNING_COUNT;
    break;
  case ITM_COUNT_NONULL:
    return ITM_RUNNING_COUNT;
    break;
  case ITM_MAX:
    return ITM_RUNNING_MAX;
    break;
  case ITM_MIN:
    return ITM_RUNNING_MIN;
    break;
  case ITM_STDDEV:
    return ITM_RUNNING_SDEV;
    break;
  case ITM_SUM:
    return ITM_RUNNING_SUM;
    break;
  case ITM_VARIANCE:
    return ITM_RUNNING_VARIANCE;
    break;
  case ITM_RUNNING_RANK:
    return ITM_RUNNING_RANK;
    break;
  case ITM_RUNNING_DRANK:
    return ITM_RUNNING_DRANK;
    break;
  default:
    return INVALID_OPERATOR_TYPE;
  }
}
OperatorTypeEnum Aggregate::mapOperTypeToOlap() const
{
  switch(getOperatorType()) {
  case ITM_AVG:
    return ITM_OLAP_AVG;
    break;
  case ITM_COUNT:
    return ITM_OLAP_COUNT;
    break;
  case ITM_COUNT_NONULL:
    return ITM_OLAP_COUNT;
    break;
  case ITM_MAX:
    return ITM_OLAP_MAX;
    break;
  case ITM_MIN:
    return ITM_OLAP_MIN;
    break;
  case ITM_STDDEV:
    return ITM_OLAP_SDEV;
    break;
  case ITM_SUM:
    return ITM_OLAP_SUM;
    break;
  case ITM_VARIANCE:
    return ITM_OLAP_VARIANCE;
    break;
  case ITM_RUNNING_RANK:
    return ITM_OLAP_RANK;
    break;
  case ITM_RUNNING_DRANK:
    return ITM_OLAP_DRANK;
    break;
  default:
    return INVALID_OPERATOR_TYPE;
  }
}


// Return the equivalent moving sequence function operator type
//
OperatorTypeEnum Aggregate::mapOperTypeToMoving() const
{
  switch(getOperatorType()) {
  case ITM_AVG:
    return ITM_MOVING_AVG;
    break;
  case ITM_COUNT:
    return ITM_MOVING_COUNT;
    break;
  case ITM_COUNT_NONULL:
    return ITM_MOVING_COUNT;
    break;
  case ITM_MAX:
    return ITM_MOVING_MAX;
    break;
  case ITM_MIN:
    return ITM_MOVING_MIN;
    break;
  case ITM_STDDEV:
    return ITM_MOVING_SDEV;
    break;
  case ITM_SUM:
    return ITM_MOVING_SUM;
    break;
  case ITM_VARIANCE:
    return ITM_MOVING_VARIANCE;
    break;
  case ITM_RUNNING_RANK:
    return ITM_MOVING_RANK;
    break;
  case ITM_RUNNING_DRANK:
    return ITM_MOVING_DRANK;
    break;
  default:
    return INVALID_OPERATOR_TYPE;
  }
}
OperatorTypeEnum ItmSeqOlapFunction::mapOperTypeToRunning() const
{
  switch(getOperatorType()) {
  case ITM_OLAP_AVG:
    return ITM_RUNNING_AVG;
    break;
  case ITM_OLAP_COUNT:
    return ITM_RUNNING_COUNT;
    break;
  case ITM_OLAP_MAX:
    return ITM_RUNNING_MAX;
    break;
  case ITM_OLAP_MIN:
    return ITM_RUNNING_MIN;
    break;
  case ITM_OLAP_SDEV:
    return ITM_RUNNING_SDEV;
    break;
  case ITM_OLAP_SUM:
    return ITM_RUNNING_SUM;
    break;
  case ITM_OLAP_VARIANCE:
    return ITM_RUNNING_VARIANCE;
    break;
  case ITM_OLAP_RANK:
    return ITM_RUNNING_RANK;
    break;
  case ITM_OLAP_DRANK:
    return ITM_RUNNING_DRANK;
    break;
  default:
    return INVALID_OPERATOR_TYPE;
  }
}



OperatorTypeEnum ItmSeqOlapFunction::mapOperTypeToMoving() const
{
  switch(getOperatorType()) {
  case ITM_OLAP_AVG:
    return ITM_MOVING_AVG;
    break;
  case ITM_OLAP_COUNT:
    return ITM_MOVING_COUNT;
    break;
  case ITM_OLAP_MAX:
    return ITM_MOVING_MAX;
    break;
  case ITM_OLAP_MIN:
    return ITM_MOVING_MIN;
    break;
  case ITM_OLAP_SDEV:
    return ITM_MOVING_SDEV;
    break;
  case ITM_OLAP_SUM:
    return ITM_MOVING_SUM;
    break;
  case ITM_OLAP_VARIANCE:
    return ITM_MOVING_VARIANCE;
    break;
  case ITM_OLAP_RANK:
    return ITM_MOVING_RANK;
    break;
  case ITM_OLAP_DRANK:
    return ITM_MOVING_DRANK;
    break;
  default:
    return INVALID_OPERATOR_TYPE;
  }
}


// transformOlapFunction.
// Transform OLAP Window aggregate functions into their equivalent 
// sequence functions.
//
// Also verifies that all OLAP Window function are using the same Window
// specification (partition by and order by).  If not, an error message
// is put into the diags area.
// 
// Inputs - this - the Aggregate with OLAP information 
//                 (partition by and order by)
//
//          bindWA - Used to get current scope for error handling

// Returns - transformed expression.
//         - NULL on error
//
ItemExpr *Aggregate::transformOlapFunction(BindWA *bindWA)
{
  if(NOT isOLAP_) {
    // Not an OLAP aggregate.
    return this;
  }

  // If this is an illegal Frame Specification, then issue an error.
  //
  if ((frameStart_ > frameEnd_) || 
      (isFrameStartUnboundedFollowing()) || // frameStart_ == INT_MAX) || 
      (isFrameEndUnboundedPreceding())) //frameEnd_ == -INT_MAX ))
    {
      *CmpCommon::diags() << DgSqlCode(-4342);
      bindWA->setErrStatus();
      return NULL;
    } 

  if (!olapOrderBy_ && ( getOperatorType() == ITM_RUNNING_RANK || getOperatorType() == ITM_RUNNING_DRANK))
    {//The use of RANK or DENSE_RANK window functions without a window ORDER BY clause is not supported.
      *CmpCommon::diags() << DgSqlCode(-4344);
      bindWA->setErrStatus();
      return NULL;     
    }
  // Distinct is not supported for Window Functions.
  //
  if (this->isDistinct_)
    {
      *CmpCommon::diags() << DgSqlCode(-4341);
      bindWA->setErrStatus();
      return NULL;
    }

  BindScope *currScope = bindWA->getCurrentScope();

  // OLAP Window functions can only be in the select list
  //
  if (! currScope->context()->inSelectList())
    {
      *CmpCommon::diags() << DgSqlCode(-4346);
      bindWA->setErrStatus();
      return NULL;     

    }
 
  if (currScope->context()->inAggregate())
    {
      *CmpCommon::diags() << DgSqlCode(-4375) << DgString0(getTextUpper());
      bindWA->setErrStatus();
      return NULL;     

    }

  ValueIdList  partition_vil, order_vil;
  ItemExpr * tmpItemExpr = NULL;

  // Verify that all Window Functions within this scope, use the
  // same Window Specification (partition by and order by)
  //

  // The Sequence functions are bound in the environment (RETDesc) of
  // the child of the OLAP Sequence if one exists
  //
  RelExpr *sequenceNode = currScope->getSequenceNode();
  RETDesc *currentRETDesc = currScope->getRETDesc();
   
  currScope->setRETDesc(sequenceNode->child(0)->getRETDesc());

  if (olapPartitionBy_)
    {
      currScope ->context()->inOrderBy() = TRUE;
      currScope ->context()->inOlapPartitionBy() = TRUE;
      olapPartitionBy_->convertToValueIdList(partition_vil, bindWA, ITM_ITEM_LIST);
      currScope ->context()->inOlapPartitionBy() = FALSE;
      currScope ->context()->inOrderBy() = FALSE;

      if (bindWA->errStatus())
        return NULL;
    }

  if (olapOrderBy_)
    {
      currScope->context()->inOtherSequenceFunction() = TRUE;
      currScope ->context()->inOrderBy() = TRUE;
      currScope ->context()->inOlapOrderBy() = TRUE;
      olapOrderBy_->convertToValueIdList(order_vil, bindWA, ITM_ITEM_LIST);
      currScope ->context()->inOlapOrderBy() = FALSE;
      currScope ->context()->inOrderBy() = FALSE;
      currScope->context()->inOtherSequenceFunction() = FALSE;

      if (bindWA->errStatus())
        return NULL;
    }
    
  currScope->setRETDesc(currentRETDesc);
    
  // If this is the first Window Function to be bound in this scope,
  // remember the partition and order by lists.
  //
  if ( currScope->getIsFirstOlapWindowSpec() )
    { 
      currScope->setOlapPartition( partition_vil );
      currScope->setOlapOrder( order_vil );
      currScope->setIsFirstOlapWindowSpec ( FALSE );
    } 
  else 
    {
      // Check to see if the partition by and order by for this Window
      // Function is the same as all the others we have bound in this
      // scope so far.
      //
      NABoolean olap = TRUE;
      if (partition_vil.entries() != currScope->getOlapPartition().entries() ||
	  order_vil.entries() != currScope->getOlapOrder().entries())
        {
          olap = FALSE;
        }
      for(CollIndex i = 0; olap && i < partition_vil.entries(); i++) 
        {
          ItemExpr *ie = currScope->getOlapPartition()[i].getItemExpr();
          ItemExpr *otherIe = partition_vil[i].getItemExpr();
          if(!ie->hasBaseEquivalence(otherIe)) 
            {
              olap = FALSE;
            }
        }

      for(CollIndex i = 0;  olap && i < order_vil.entries(); i++) 
        {
          ValueId vid1, vid2;
          vid1 = currScope->getOlapOrder()[i];
          vid2 = order_vil[i];
          if (order_vil[i].getItemExpr()->getOperatorType() == ITM_INVERSE &&
              currScope->getOlapOrder()[i].getItemExpr()->getOperatorType() == ITM_INVERSE)
            {
              vid1 = currScope->getOlapOrder()[i].getItemExpr()->child(0).getValueId();
              vid2 = order_vil[i].getItemExpr()->child(0).getValueId();
            }
	 
          ItemExpr *ie = vid1.getItemExpr();
          ItemExpr *otherIe = vid2.getItemExpr();
          if( !ie->hasBaseEquivalence(otherIe)) 
            {
              olap = FALSE;
            }
        }

      if (! olap)
        {
          // The Window Specification for this Window Function is not
          // the same as the others we have bound so far in this scope.
          //
          *CmpCommon::diags() << DgSqlCode(-4340);
          bindWA->setErrStatus();
          return NULL;
        }
    }
 
    CollHeap *heap = CmpCommon::statementHeap();

    OperatorTypeEnum op = mapOperTypeToOlap();

    ItmSeqOlapFunction *seqFunc = new (heap)
                ItmSeqOlapFunction(op, child(0));

    seqFunc->setOLAPInfo(olapPartitionBy_, olapOrderBy_);
    seqFunc->setOlapWindowFrame(frameStart_, frameEnd_);
                                        
    return seqFunc;


}


QR::ExprElement Aggregate::getQRExprElem() const
{
  return QR::QRFunctionElem;
}


// -----------------------------------------------------------------------
// member functions for class Variance
// -----------------------------------------------------------------------
Variance::~Variance() {}

ItemExpr * Variance::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  Variance *result;

  if (derivedNode == NULL)
    result = new (outHeap) Variance(getOperatorType(), NULL, NULL, isDistinct());
  else
    result = (Variance *) derivedNode;

  return Aggregate::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class PivotGroup
// -----------------------------------------------------------------------
PivotGroup::PivotGroup(OperatorTypeEnum otype,
                       ItemExpr *child0,
                       NAList<PivotOption*> * pivotOptionsList,
                       NABoolean isDistinct)
  : Aggregate(otype, child0, NULL, isDistinct),
    pivotOptionsList_(pivotOptionsList),
    maxLen_(DEFAULT_MAX_LEN),
    delim_(","),
    orderBy_(FALSE)
{
  if (pivotOptionsList)
    {
      for (CollIndex i = 0; i < pivotOptionsList->entries(); i++)
	{
	  PivotOption * po = (*pivotOptionsList)[i];
	  switch (po->option_)
	    {
	    case DELIMITER_:
	      {
		delim_ = *po->stringVal_;
              }
              break;

	    case MAX_LENGTH_:
	      {
		maxLen_ = po->numericVal_;
              }
              break;

            case ORDER_BY_:
              {
                orderBy_ = TRUE;
                // optionNode_ contains the ItemExpr 
                orgReqOrder_ = (ItemExpr *)po->optionNode_; 
              }
              break;
            }
        }
    }
}

PivotGroup::~PivotGroup() {}

ItemExpr * PivotGroup::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  PivotGroup *result;

  if (derivedNode == NULL)
    result = new (outHeap) PivotGroup(getOperatorType(), NULL, NULL, isDistinct());
  else
    result = (PivotGroup *) derivedNode;

  result->pivotOptionsList_ = pivotOptionsList_;
  result->delim_ = delim_;
  result->orderBy_ = orderBy_;
  result->reqdOrder_ = reqdOrder_;
  result->orgReqOrder_ = orgReqOrder_;
  result->maxLen_ = maxLen_;

  return Aggregate::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class Function
// -----------------------------------------------------------------------
Function::Function(OperatorTypeEnum otype,
                   NAMemory *h,
		   Lng32   argumentCount,
		   ItemExpr *child0,
		   ItemExpr *child1,
		   ItemExpr *child2,
		   ItemExpr *child3,
		   ItemExpr *child4,
		   ItemExpr *child5)
           : ItemExpr(otype),
	     children_(h,argumentCount),
	     allowsSQLnullArg_(TRUE)
{
  Lng32 lastInserted = -1;
  ItemExpr *childx;

  for (Lng32 i = 0; i < (Lng32)argumentCount; i++)
    {
      childx = IFX i==0
	THENX child0
	ELSEX IFX i==1
	THENX child1
	ELSEX IFX i==2
	THENX child2
	ELSEX IFX i==3
	THENX child3
	ELSEX IFX i==4
	THENX child4
	ELSEX IFX i==5
	THENX child5
	ELSEX (ItemExpr *) NULL;

      CMPASSERT((Lng32)children_.entries() == i);

      children_.insertAt(i, childx);
    } // end for
}

Function::Function(OperatorTypeEnum otype, const LIST(ItemExpr *) &children, 
                   CollHeap *h)
         : ItemExpr(otype),
	   children_(h)
{
  Lng32 ne = children.entries();

  for (Lng32 i = 0; i < ne; i++)
    {
      children_.insertAt(i,children[i]);
    }
}

Function::~Function() {}

Lng32 Function::getNumChildren() const
{
  Lng32 count = children_.entries();
  // $$$$ Skip all the NULL children at the tail end.
  // $$$$ Assumes children that are missing in the middle
  // $$$$ should figure in the count, e.g., F(a, NULL, b, NULL, NULL)
  while ( (count > 0) AND (children_[count-1].getPtr() == NULL) )
    count--;
  return count;
}

ItemExpr * Function::copyTopNode(ItemExpr * derivedNode, CollHeap* outHeap)
{
  Function *result = NULL;
  if (derivedNode == NULL)
    ABORT("copyTopNode() can only be called for a derived class of Function");
  else
    result = (Function *)derivedNode;

  result->allowsSQLnullArg() = allowsSQLnullArg();

  // Make sure we copy the kids as well.
  Lng32 ne = children_.entries();
  for (Lng32 i = 0; i < ne; i++)
    result->children_.insertAt(i, children_[i]);

  return ItemExpr::copyTopNode(result, outHeap);
}

ExprValueId & Function::operator[] (Lng32 index)
{
 CMPASSERT( (index >= 0) AND (index < (Lng32)children_.entries()) );
 return children_[index];
}

const ExprValueId & Function::operator[] (Lng32 index) const
{
  CMPASSERT( (index >= 0) AND (index < (Lng32)children_.entries()) );
  return children_[index];
}

// -----------------------------------------------------------------------
// member functions for class BuiltinFunction
// -----------------------------------------------------------------------
BuiltinFunction::BuiltinFunction(OperatorTypeEnum otype,
                                 NAMemory *h,
				 Lng32  argumentCount,
				 ItemExpr *child0,
				 ItemExpr *child1,
				 ItemExpr *child2,
				 ItemExpr *child3,
				 ItemExpr *child4,
				 ItemExpr *child5)
     : Function(otype,h,argumentCount,child0,child1,child2,child3,child4,child5)
{
  switch (getOperatorType())
    {
    case ITM_NULLIFZERO:
    case ITM_QUERYID_EXTRACT:
    case ITM_TOKENSTR:
    case ITM_REVERSE: {
	allowsSQLnullArg() = FALSE;
      }
    break;

    default:
      {
      }
    break;
    }
}

BuiltinFunction::~BuiltinFunction() {}

Int32 BuiltinFunction::getArity() const
{
  return getNumChildren();
}

// -----------------------------------------------------------------------
// BuiltinFunction::isCovered()
// -----------------------------------------------------------------------
NABoolean BuiltinFunction::isCovered
                             (const ValueIdSet& newExternalInputs,
		              const GroupAttributes& coveringGA,
		              ValueIdSet& referencedInputs,
		              ValueIdSet& coveredSubExpr,
		              ValueIdSet& unCoveredExpr) const
{

  // ITM_CURRENT_USER function should appear as an input characteristic
  // of the root for it to be evaluated by the executor. And hence it
  // needs to be treated differently from other BuiltinFunctions. For
  // it to be propogated to the root, it should not be covered.
  // Case 10-010419-2366

  if (isAUserSuppliedInput())  // for user(x), current_timestamp, extract (fix)
     return FALSE;

  // A BuiltinFunction with no arguments (children)
  // is like a constant. It is always covered.
  Lng32 nc = getNumChildren();
  if (nc == 0)
    return TRUE;

  // ---------------------------------------------------------------------
  // A BuiltinFunction can contain values that are produced by
  // different sources. The BuiltinFunction is covered if each
  // operand is covered.
  // The coverage test insists on a complete coverage of all the
  // children.
  // ---------------------------------------------------------------------
  ValueIdSet childValues;
  for (Int32 i = 0; i < nc; i++)
    childValues += child(i).getValueId();
  return childValues.isCovered(newExternalInputs,
			      coveringGA,
			      referencedInputs,
			      coveredSubExpr,
			      unCoveredExpr);
} // BuiltinFunction::isCovered()

NABoolean BuiltinFunction::isCacheableExpr(CacheWA& cwa)
{
  switch (getOperatorType())
    {
    case ITM_NULLIFZERO:
    case ITM_QUERYID_EXTRACT:
    case ITM_TOKENSTR:
      {
	return ItemExpr::isCacheableExpr(cwa);
      }
    break;

    case ITM_NVL:
    case ITM_REVERSE:
      {
	return FALSE;
      }
    break;
    case ITM_JSONOBJECTFIELDTEXT:
    {
	    return FALSE;
    }
    break;

    default:
      {
	return Function::isCacheableExpr(cwa);
      }
    break;
    }
}

const NAString BuiltinFunction::getText() const
{
  NABoolean dumpMvMode = (CmpCommon::getDefault(MVQR_LOG_QUERY_DESCRIPTORS) == DF_DUMP_MV);

  switch (getOperatorType())
    {
    case ITM_ABS:
      return "abs";
    case ITM_ASCII:
      return "ascii";
    case ITM_AUTHNAME:
      return "authname";
    case ITM_AUTHTYPE:
      return "authtype";
    case ITM_BETWEEN:
      return "between";
    case ITM_BLOCK:
      return "block";
    case ITM_BOOL_RESULT:
      return "bool_result";
    case ITM_CASE:
      return "case";
    case ITM_CAST:
      return "cast";
    case ITM_CAST_CONVERT:
      return "cast_convert";
    case ITM_CAST_TYPE:
      return "typecast";
    case ITM_CHAR:
      return "char";
    case ITM_CHAR_LENGTH:
      return "char_length";
    case ITM_COALESCE:
      return "coalesce";
    case ITM_COMP_ENCODE:
      return "comp_encode";
    case ITM_COMP_DECODE:
      return "comp_decode";
    case ITM_CONCAT:
      return "||";				// "concat";
    case ITM_CONVERTFROMHEX:
      return "CONVERTFROMHEX";
    case ITM_CONVERTTOHEX:
      return "CONVERTTOHEX";
    case ITM_CONVERTTOBITS:
      return "CONVERTTOBITS";
    case ITM_CONVERTTIMESTAMP:
      return "converttimestamp";
    case ITM_SLEEP:
      return "sleep";
    case ITM_UNIX_TIMESTAMP:
      return "unix_timestamp";
    case ITM_CURRENT_TIMESTAMP:
      return "current_timestamp";
    case ITM_CURRENT_TIMESTAMP_RUNNING:
      return "current_timestamp_running";
    case ITM_CURRENT_USER:
      return "current_user";
    case ITM_DATEFORMAT:
      return "dateformat";
    case ITM_DAYOFMONTH:
      return "dayofmonth";
    case ITM_DAYOFWEEK:
      return "dayofweek";
    case ITM_DO_WHILE:
      return "do while";
    case ITM_WHILE:
      return "while";
    case ITM_EXPLODE_VARCHAR:
      return "explodevarchar";
    case ITM_EXTRACT:
      return "extract";
    case ITM_EXTRACT_ODBC:
      return "extract_odbc";
    case ITM_GREATEST:
     return "greatest";
    case ITM_LEAST:
     return "least";
    case ITM_IN:
      return "in";
    case ITM_INSTANTIATE_NULL:
      // getenv() calls are costly. avoid especially in release code.
#ifndef NDEBUG
      if (getenv("SIMPLE_DISPLAY")) return "iNull";
#endif
      return "instantiate_null";
    case ITM_JULIANTIMESTAMP:
      return "juliantimestamp";
    case ITM_EXEC_COUNT:
      return "execution_count";
    case ITM_CURR_TRANSID:
      return "current_transid";
    case ITM_LIKE:
    case ITM_LIKE_DOUBLEBYTE:
      return "like";
    case ITM_REGEXP:
      return "regexp";
    case ITM_LOWER:
    case ITM_LOWER_UNICODE:
      return "lower";
    case ITM_NARROW:
      return "narrow";
    case ITM_NULLIFZERO:
      return "nullifzero";
    case ITM_NVL:
      return "nvl";
    case ITM_OVERLAY:
      return "overlay";
    case ITM_JSONOBJECTFIELDTEXT:
      return "json_object_field_text";
    case ITM_QUERYID_EXTRACT:
      return "queryid_extract";
    case ITM_UPPER:
    case ITM_UPPER_UNICODE:
      return "upper";
    case ITM_UNICODE_CHAR:
      return "unicode_char";
    case ITM_NO_OP:
      return "no_op";
    case ITM_POSITION:
      return "position";
    case ITM_REPEAT:
      return "repeat";
    case ITM_REPLACE:
      return "replace";
    case ITM_REPLACE_NULL:
      return "replace null";
    case ITM_RETURN_TRUE:
      if (dumpMvMode)
        return "1";
      else
        return "return_true";
    case ITM_RETURN_FALSE:
      if (dumpMvMode)
        return "0";
      else
        return "return_false";
    case ITM_RETURN_NULL:
      if (dumpMvMode)
        return "null";
      else
        return "return_unknown";
    case ITM_SESSION_USER:
      return "session_user";
    case ITM_OCTET_LENGTH:
      return "octet_length";
    case ITM_HASH:
      return "hash";
    case ITM_HASH2_DISTRIB:
      return "hash2_distrib";
    case ITM_MOD:
      return "mod";
    case ITM_INVERSE:
      return "inverse";
    case ITM_SUBSTR:
    case ITM_SUBSTR_DOUBLEBYTE:
      return "substring";
    case ITM_TRANSLATE:
      return "translate";
    case ITM_TRIM:
    case ITM_TRIM_DOUBLEBYTE:
      return "trim";
    case ITM_IF_THEN_ELSE:
      return "if_then_else";
    case ITM_LESS_OR_LE:
      return "< or <=";
    case ITM_GREATER_OR_GE:
      return "> or >=";
    case ITM_RANGE_LOOKUP:
      return "range_lookup";
    case ITM_RANDOMNUM:
      return "randomNum";
    case ITM_RAND_SELECTION:
      return "randomSelection";
    case ITM_ROUND_ROBIN:
      return "Round_Robin";
    case ITM_PACK_FUNC:
      return "pack";
    case ITM_SAMPLE_VALUE:
      return "sample_size";
    case ITM_UNIQUE_SHORT_ID:
      return "unique_short_id";
    case ITM_UNIQUE_ID:
      return "unique_id";
    case ITM_UNIQUE_ID_SYS_GUID:
      return "sys_guid";
    case ITM_HBASE_COLUMN_LOOKUP:
      return "hbase_column_lookup";
    case ITM_HBASE_COLUMNS_DISPLAY:
      return "hbase_columns_display";
    case ITM_HBASE_COLUMN_CREATE:
      return "hbase_column_create";
    case ITM_SEQUENCE_VALUE:
      return "seqnum";
    case ITM_ROWNUM:
      return "rownum";
    case ITM_USER:
      return "user";
    case ITM_UNIQUE_EXECUTE_ID:
      return "unique_execute_id";
    case ITM_GET_TRIGGERS_STATUS:
      return "get_triggers_status";
    case ITM_GET_BIT_VALUE_AT:
      return "get_bit_value_at";
    case ITM_IS_BITWISE_AND_TRUE:
      return "is_bitwise_and_true";
    case ITM_USERID:
      return "os_userid";
    case ITM_CURRENTEPOCH:
      return "current_epoch";
    case ITM_VSBBROWTYPE:
      return "vsbb_row_type";
    case ITM_VSBBROWCOUNT:
      return "vsbb_row_count";
    case ITM_INTERNALTIMESTAMP:
      return "internal_timestamp";
    case ITM_SCALAR_MIN:
      return "scalar_min";
    case ITM_SCALAR_MAX:
      return "scalar_max";
    case ITM_TOKENSTR:
      return "TOKENSTR";
    case ITM_REVERSE:
      return "REVERSE";

    // ZZZBinderFunction classes (for error messages only)
    case ITM_DATE_TRUNC_YEAR:
    case ITM_DATE_TRUNC_MONTH:
    case ITM_DATE_TRUNC_DAY:
    case ITM_DATE_TRUNC_HOUR:
    case ITM_DATE_TRUNC_MINUTE:
    case ITM_DATE_TRUNC_SECOND:
    case ITM_DATE_TRUNC_CENTURY:
    case ITM_DATE_TRUNC_DECADE:
      return "date_trunc";
    case ITM_DATEDIFF_YEAR:
    case ITM_DATEDIFF_MONTH:
    case ITM_DATEDIFF_DAY:
    case ITM_DATEDIFF_HOUR:
    case ITM_DATEDIFF_MINUTE:
    case ITM_DATEDIFF_SECOND:
    case ITM_DATEDIFF_QUARTER:
    case ITM_DATEDIFF_WEEK:
      return "datediff";
    case ITM_TSI_YEAR:
    case ITM_TSI_MONTH:
    case ITM_TSI_DAY:
    case ITM_TSI_HOUR:
    case ITM_TSI_MINUTE:
    case ITM_TSI_SECOND:
    case ITM_TSI_QUARTER:
    case ITM_TSI_WEEK:
      return "timestampdiff";
    case ITM_DAYNAME:
      return "dayname";
    case ITM_DAYOFYEAR:
      return "dayofyear";
    case ITM_DECODE:
      return "decode";
    case ITM_FIRSTDAYOFYEAR:
      return "firstdayofyear";
    case ITM_LAST_DAY:
      return "last_day";
    case ITM_NEXT_DAY:
      return "next_day";
    case ITM_INSERT_STR:
      return "insert";
    case ITM_LEFT:
      return "left";
    case ITM_LPAD:
      return "lpad";
    case ITM_MONTHNAME:
      return "monthname";
    case ITM_NULLIF:
      return "nullif";
    case ITM_ODBC_LENGTH:
      return "LENGTH";
    case ITM_QUARTER:
      return "quarter";
    case ITM_RIGHT:
      return "right";
    case ITM_RPAD:
      return "rpad";
    case ITM_SIGN:
      return "sign";
    case ITM_SPACE:
      return "space";
    case ITM_CODE_VALUE:
    case ITM_UNICODE_CODE_VALUE:
    case ITM_NCHAR_MP_CODE_VALUE:
      return "code_value";
    case ITM_WEEK:
      return "week";
    case ITM_ZEROIFNULL:
      return "zeroifnull";

      
    case ITM_LOBINSERT:
      return "lobinsert";

    case ITM_LOBSELECT:
      return "lobselect";

    case ITM_LOBDELETE:
      return "lobdelete";

    case ITM_LOBUPDATE:
      return "lobupdate";

    case ITM_LOBCONVERTHANDLE:
      return "lobconverthandle";

    case ITM_LOBCONVERT:
      return "lobconvert";

    case ITM_LOBLOAD:
      return "lobload";
    
    case ITM_AGGR_GROUPING_FUNC:
      return "aggr_grouping";

    case ITM_ENCODE_BASE64:
      return "encode_base64";

    case ITM_DECODE_BASE64:
      return "decode_base64";

    case ITM_TO_TIMESTAMP:
       return "to_timestamp";
 
    case ITM_SPLIT_PART:
      return "split_part";

    default:
      return "unknown func";
    } // switch
} // BuiltinFunction::getText()

//## Yuk -- embedded English text -- this is not per I18N standards!
const NAString BuiltinFunction::getTextForError() const
{
  switch (getOperatorType())
    {
    case ITM_CHAR_LENGTH:
      return "CHARACTER_LENGTH, CHAR_LENGTH, or LENGTH";

    case ITM_LOWER:
      return "LOWER or LCASE";

    case ITM_POSITION:
      return "POSITION or LOCATE";

    case ITM_TRIM:
    case ITM_TRIM_DOUBLEBYTE:
      return "TRIM, LTRIM, or RTRIM";

    case ITM_UPPER:
      return "UPPER, UPSHIFT, or UCASE";

    default: return getTextUpper();
    } // switch
} // BuiltinFunction::getTextForError()

ItemExpr * BuiltinFunction::copyTopNode(ItemExpr * derivedNode,
					CollHeap* outHeap)
{
  ItemExpr *result = NULL;
  if (derivedNode == NULL)
    {
      switch (getOperatorType())
	{
	case ITM_NULLIFZERO:
        case ITM_ISIPV4:
        case ITM_ISIPV6:
        case ITM_MD5:
        case ITM_CRC32:
	case ITM_SOUNDEX:
        case ITM_REVERSE:
	  {
	    result = new (outHeap) BuiltinFunction(getOperatorType(),
						   outHeap, 1, child(0));
	  }
	break;

	case ITM_NVL:
	case ITM_QUERYID_EXTRACT:
	case ITM_TOKENSTR:
	  {
	    result = new (outHeap) BuiltinFunction(getOperatorType(),
						   outHeap, 2, child(0), child(1));
	  }
	break;
    case ITM_JSONOBJECTFIELDTEXT:
	{
	    result = new (outHeap) BuiltinFunction(getOperatorType(),
						   outHeap, 2, child(0), child(1));
	}
	break;
	default:
	  {
	    ABORT("copyTopNode() can only be called for a derived class of BuiltinFunction");
	  }
	break;
	}
    }
  else
    result = derivedNode;

  return Function::copyTopNode(result, outHeap);
}

ItemExpr * HashCommon::copyTopNode(ItemExpr * derivedNode,
				   CollHeap* outHeap)
{
  ItemExpr *result = NULL;

  if (derivedNode == NULL)
    ABORT("copyTopNode() can only be called for a derived class of Function");
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

//++Triggers, 

// -----------------------------------------------------------------------
// member functions for class EvaluateOnceBuiltinFunction
// -----------------------------------------------------------------------


EvaluateOnceBuiltinFunction::~EvaluateOnceBuiltinFunction() {}

NABoolean EvaluateOnceBuiltinFunction::isAUserSuppliedInput() const    { return TRUE; }

NABoolean EvaluateOnceBuiltinFunction::isCovered(const ValueIdSet& newExternalInputs,
                               const GroupAttributes& newRelExprAnchorGA,
                               ValueIdSet& referencedInputs,
                               ValueIdSet& coveredSubExpr,
                               ValueIdSet& unCoveredExpr) const
{
	return FALSE;
}

//--Triggers, 

// -----------------------------------------------------------------------
// member functions for builtin functions
// -----------------------------------------------------------------------
Between::~Between()
{
  // NOTE: never destroy pDirectionVector_.
  // this is a pointer passed on to other objects as is.
  // if it is destroyed this may affect other items.

}

Concat::~Concat() {}

Case::~Case() {}

IfThenElse::~IfThenElse() {}

InstantiateNull::~InstantiateNull() {}

Hash::~Hash() {}

ReplaceNull::~ReplaceNull() {}

// -----------------------------------------------------------------------
// member functions for BoolVal
// -----------------------------------------------------------------------
BoolVal::~BoolVal() {}

NABoolean BoolVal::isCovered
                      (const ValueIdSet& newExternalInputs,
		       const GroupAttributes& coveringGA,
		       ValueIdSet& referencedInputs,
		       ValueIdSet& coveredSubExpr,
		       ValueIdSet& unCoveredExpr ) const
{
  // A BoolVal that returns a TRUE/FALSE/NULL unconditionally, i.e.,
  // its evaluation does not depend upon the boolean outcome of its
  // child subtree, is like a constant. It is always covered.
  if (getNumChildren() == 0)
    return TRUE;
  else
    // BoolVal is Covered if its operand is covered
    return coveringGA.covers(((ItemExpr *)child(0))->getValueId(),
			     newExternalInputs, referencedInputs,
			     &coveredSubExpr, &unCoveredExpr);
} // BoolVal::isCovered()

// -----------------------------------------------------------------------
// member functions for InverseOrder
// -----------------------------------------------------------------------
InverseOrder::~InverseOrder() {}

ItemExpr * InverseOrder::simplifyOrderExpr(OrderComparison *newOrder)
{
  OrderComparison resultOrder;
  ItemExpr *result = child(0)->simplifyOrderExpr(&resultOrder);

  // reverse the order of the simplified child
  if (resultOrder == SAME_ORDER)
    resultOrder = INVERSE_ORDER;
  else
    resultOrder = SAME_ORDER;

  if (newOrder)
    *newOrder = resultOrder;
  return result;
}

ItemExpr * InverseOrder::removeInverseOrder()
{
  return child(0);
}

// -----------------------------------------------------------------------
// member functions for PatternMatchingFunction.
// -----------------------------------------------------------------------
PatternMatchingFunction::~PatternMatchingFunction() {}

// -----------------------------------------------------------------------
// member functions for Like
// -----------------------------------------------------------------------
Like::~Like() {}

Regexp::~Regexp() {}


ItemExpr * Regexp::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result = NULL;

  if (derivedNode == NULL)
    {
      result = new (outHeap) Regexp(NULL, NULL,
	    numberOfNonWildcardChars_,
	    bytesInNonWildcardChars_,
	    patternAStringLiteral_,
	    oldDefaultSelForLikeWildCardUsed_,
	    beginEndKeysApplied_);
    }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Regexp::copyTopNode()

ItemExpr * Like::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result = NULL;

  if (derivedNode == NULL)
    {
      switch (getArity())
	{
	case 2:
	  result = new (outHeap) Like(child(0), child(1),
	    numberOfNonWildcardChars_,
	    bytesInNonWildcardChars_,
	    patternAStringLiteral_,
	    oldDefaultSelForLikeWildCardUsed_,
	    beginEndKeysApplied_);
	  break;
	case 3:
	  result = new (outHeap) Like(child(0), child(1), child(2),
	    numberOfNonWildcardChars_,
	    bytesInNonWildcardChars_,
	    patternAStringLiteral_,
	    oldDefaultSelForLikeWildCardUsed_,
	    beginEndKeysApplied_);
	  break;
	default:
	  CMPASSERT(0 == 1);
	  break;
	}
    }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Like::copyTopNode()

double PatternMatchingFunction::defaultSel()
{
  // if begin and end keys have been applied to this expression, then this means
  // that the original LIKE predicate was something like a%b. This was transformed
  // to >=a and < b and like %b. Here we would have already applied selectivity
  // to the first range predicate. We do not want to apply selectivity to the other
  // portions of the predicate. Hence return 1.0 for the last portion of the
  // predicate

  if ( beginEndKeysApplied( CmpCommon::statementHeap() ) )
     return 1.0;

  if (oldDefaultSelForLikeWildCardUsed_)
  {
    return ActiveSchemaDB()->getDefaults().getAsDouble(HIST_DEFAULT_SEL_FOR_LIKE_WILDCARD);
  }

  // For all other cases compute selectivity based on the number of non-wildcard
  // characters
  return computeSelForNonWildcardChars(); ;
}

void PatternMatchingFunction::setNumberOfNonWildcardChars(const LikePatternString &pattern)
{
  Int32 count = 0;
  Int32 byteCnt = 0;
  CharInfo::CharSet  cs = pattern.getPatternCharSet();

  LikePatternStringIterator i(pattern);

  while (i != LikePatternStringIterator::END_OF_PATTERN)
  {
    const char *currentChar = i.getCurrentChar();
    UInt16 number_bytes = Attributes::getFirstCharLength(currentChar, 8, cs);

    if (i == LikePatternStringIterator::NON_WILDCARD)
    {
        count++;
        byteCnt += number_bytes;
    }

    i += number_bytes + ((cs == CharInfo::UCS2) ? 1 : 0); // For UCS2, number_bytes is always 1
    i.determineCharType();
  }
  numberOfNonWildcardChars_ = count;
  bytesInNonWildcardChars_  = byteCnt ;
}

double PatternMatchingFunction::computeSelForNonWildcardChars()
{

  // get the default selectivity for like predicate
  double defaultSelectivity = CURRSTMT_OPTDEFAULTS->defSelForWildCard();

  // get the number of non_wildcard characters after the first wildcard char
  Int32 cnt = getNoOfNonWildcardChars();

  // Retuen default selectivity for the following cases:
  // 1. For some special cases, example when there is a dynamic parameter in the
  //    LIKE predicate, the LIKE expression is not optimized. In all such cases
  //    cnt is less than 0. For these, return the default selectivity
  // 2. if there are no non_wildcard characters in the like clause, then if the column
  //    is not nullable, we have already transformed the predicate to TRUE.
  //    For a nullable column, we apply default selectivity.
  // 3. If there is only one non_wildcard character, we go by the selectivity
  //    defined by HIST_DEFAULT_BASE_SEL_FOR_LIKE_WILDCARD,
  // 4. HIST_DEFAULT_SEL_FOR_LIKE_NO_WILDCARD is nearing 0 (e-16). We use default
  //    selectivity in that case.

  double reductionFactor = CURRSTMT_OPTDEFAULTS->defSelForNoWildCard();

  if ( (cnt <= 1) OR
       (reductionFactor <= MIN_SELECTIVITY) )
    return defaultSelectivity;

  // We compute selectivity as: if number of non-wild characters is 1, selectivity
  // is equal to default selectivity (which is 1/10 right now. If the number of
  // non-wild characters is 2, selectivity is 1/100. For number of non-wild characters
  // 3 or more, it is 1/1000. Multiplication factor each non-wildcard character is
  // defined by the CQD HIST_DEFAULT_SEL_FOR_LIKE_NO_WILDCARD

  Int32 nonWildCardChar = ((cnt > 4) ? 4 : cnt);

  defaultSelectivity = defaultSelectivity * pow(defaultSelectivity / reductionFactor, nonWildCardChar - 1);

  // Bind the selectivity between MIN_SELECTIVITY and 1.0

  if (defaultSelectivity < MIN_SELECTIVITY)
    return MIN_SELECTIVITY;
  if (defaultSelectivity > 1.0)
    return 1.0;
  return defaultSelectivity;
}

NABoolean PatternMatchingFunction::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  Like * tmp = (Like *) other;

  return 
      (this->numberOfNonWildcardChars_  == tmp->numberOfNonWildcardChars_) &&
      (this->bytesInNonWildcardChars_  == tmp->bytesInNonWildcardChars_) &&
      (this->patternAStringLiteral_ == tmp->patternAStringLiteral_ ) &&
      (this->oldDefaultSelForLikeWildCardUsed_ == tmp->oldDefaultSelForLikeWildCardUsed_) && 
      (this->beginEndKeysApplied_ == tmp->beginEndKeysApplied_ );

}

void PatternMatchingFunction::unparse(NAString &result,
		   PhaseEnum phase,
		   UnparseFormatEnum form,
		   TableDesc* tabId) const
{
  if (getArity() == 2)
    return CacheableBuiltinFunction::unparse(result, phase, form, tabId);
  else
  {
    result += "(";
    child(0)->unparse(result, phase, form, tabId);
    result += " like ";

    child(1)->unparse(result, phase, form, tabId);
    result += " escape ";

    child(2)->unparse(result, phase, form, tabId);
    result += ")";
  }                                               
}                                                 
                                                  

// -----------------------------------------------------------------------
// member functions for class ConvertHex
// -----------------------------------------------------------------------
ConvertHex::~ConvertHex() {}

ItemExpr * ConvertHex::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ConvertHex(getOperatorType(), child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // ConvertHex::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class CharLength
// -----------------------------------------------------------------------
CharLength::~CharLength() {}

ItemExpr * CharLength::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) CharLength(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // CharLength::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class OctetLength
// -----------------------------------------------------------------------
OctetLength::~OctetLength() {}

ItemExpr * OctetLength::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) OctetLength(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // OctetLength::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class PositionFunc
// -----------------------------------------------------------------------
PositionFunc::~PositionFunc() {}

ItemExpr * PositionFunc::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) PositionFunc(child(0), child(1), child(2), child(3));
  else
    result = derivedNode;

  ((PositionFunc*)result)->collation_ = collation_;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // PositionFunc::copyTopNode()

// -----------------------------------------------------------------------
// member functions for Substring
// -----------------------------------------------------------------------
Substring::~Substring() {}

ItemExpr * Substring::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result = NULL;

  if (derivedNode == NULL)
    {
      switch (getArity())
	{
	case 2:
	  result = new (outHeap) Substring(child(0), child(1));
	  break;
	case 3:
	  result = new (outHeap) Substring(child(0), child(1), child(2));
	  break;
	default:
	  CMPASSERT(0 == 1);
	  break;
	}
    }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Substring::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class ConvertTimestamp
// -----------------------------------------------------------------------
ConvertTimestamp::~ConvertTimestamp() {}

ItemExpr * ConvertTimestamp::copyTopNode(ItemExpr *derivedNode,
					 CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ConvertTimestamp(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);

} // ConvertTimestamp::copyTopNode()

SleepFunction::~SleepFunction() {}
ItemExpr * SleepFunction::copyTopNode(ItemExpr *derivedNode,
					 CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) SleepFunction(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);

} // SleepFunction::copyTopNode()
NABoolean SleepFunction::isAUserSuppliedInput() const    { return TRUE; }

UnixTimestamp::~UnixTimestamp() {}

ItemExpr * UnixTimestamp::copyTopNode(ItemExpr *derivedNode,
					 CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) UnixTimestamp();
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);

} // UnixTimestamp::copyTopNode()

NABoolean UnixTimestamp::isAUserSuppliedInput() const    { return TRUE; }

// -----------------------------------------------------------------------
// member functions for class CurrentTimestamp
// -----------------------------------------------------------------------
CurrentTimestamp::~CurrentTimestamp() {}

ItemExpr * CurrentTimestamp::copyTopNode(ItemExpr *derivedNode,
					 CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) CurrentTimestamp(dtCode_, fractPrec_);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);

} // CurrentTimestamp::copyTopNode()

NABoolean CurrentTimestamp::isAUserSuppliedInput() const    { return TRUE; }

ItemExpr * CurrentTimestamp::construct
(CollHeap * heap, 
 DatetimeType::Subtype dtCode ,
 Lng32 fractPrec)
{
  ItemExpr * ie = new(heap) CurrentTimestamp(dtCode, fractPrec);

  if ((fractPrec != SQLTimestamp::DEFAULT_FRACTION_PRECISION) ||
      (dtCode != DatetimeType::SUBTYPE_SQLTimestamp))
    {
      if (dtCode == DatetimeType::SUBTYPE_SQLDate)
        ie = new (heap)
          Cast(ie, new (heap) SQLDate(heap, FALSE));
      else if (dtCode == DatetimeType::SUBTYPE_SQLTime)
        ie = new (heap)
          Cast(ie, new (heap) SQLTime(heap, FALSE, fractPrec));
      else
        ie = new (heap)
          Cast(ie, new (heap) SQLTimestamp(heap, FALSE, fractPrec));
    }

  return ie;
}

// -----------------------------------------------------------------------
// member functions for class CurrentTimestampRunning
// -----------------------------------------------------------------------
CurrentTimestampRunning::~CurrentTimestampRunning() {}

ItemExpr * CurrentTimestampRunning::copyTopNode(ItemExpr *derivedNode,
						CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) CurrentTimestampRunning();
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);

} // CurrentTimestampRunning::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class DateFormat
// -----------------------------------------------------------------------
DateFormat::~DateFormat() {}

ItemExpr * DateFormat::copyTopNode(ItemExpr *derivedNode,
				   CollHeap* outHeap)
{
  DateFormat *result;

  if (derivedNode == NULL)
    result = new (outHeap) DateFormat(child(0), 
                                      formatStr_, formatType_, 
                                      wasDateformat_);
  else
    result = (DateFormat*)derivedNode;

  result->frmt_ = frmt_;
  result->dateFormat_ = dateFormat_;

  return BuiltinFunction::copyTopNode(result,outHeap);

} // DateFormat::copyTopNode()

NABoolean DateFormat::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  DateFormat * df = (DateFormat *) other;

  return 
      (this->dateFormat_ == df->dateFormat_);
}

void DateFormat::unparse(NAString &result,
		      PhaseEnum phase,
                      UnparseFormatEnum form,
		      TableDesc * tabId) const
{
  if (wasDateformat_)
    result += "DATEFORMAT(";
  else if (formatType_ == FORMAT_TO_DATE)
    result += "TO_DATE(";
  else if (formatType_ == FORMAT_TO_CHAR)
    result += "TO_CHAR(";
  else
    result += "unknown(";

  child(0)->unparse(result, phase, form, tabId);

  result += ", ";

  if (wasDateformat_)
    {
      if (frmt_ == ExpDatetime::DATETIME_FORMAT_DEFAULT)
        result += "DEFAULT";
      else if (frmt_ == ExpDatetime::DATETIME_FORMAT_USA)
        result += "USA";
      else if (frmt_ == ExpDatetime::DATETIME_FORMAT_EUROPEAN)
        result += "EUROPEAN";
      else
        result += "unknown";
    }
  else
    {
      result += "'";
      result += formatStr_;
      result += "'";
    }

  result += ")";  
}

// -----------------------------------------------------------------------
// member functions for class DayOfWeek
// -----------------------------------------------------------------------
DayOfWeek::~DayOfWeek() {}

ItemExpr * DayOfWeek::copyTopNode(ItemExpr *derivedNode,
				  CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) DayOfWeek(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);

} // DayOfWeek::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class ExplodeVarchar
// -----------------------------------------------------------------------
ExplodeVarchar::~ExplodeVarchar() {}

ItemExpr * ExplodeVarchar::copyTopNode(ItemExpr *derivedNode,
				       CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExplodeVarchar(child(0), type_, forInsert_);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);

} // ExplodeVarchar::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class Extract
// -----------------------------------------------------------------------
Extract::~Extract() {}

NABoolean Extract::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  Extract &o = (Extract &) other;

  // compare any local data members of the derived class
  // and return FALSE if they don't match
  if (extractField_ != o.extractField_)
    return FALSE;

  return TRUE;
}

ItemExpr * Extract::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Extract(getExtractField(), child(0), getFieldFunction());
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Extract::copyTopNode()

ItemExpr * ExtractOdbc::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExtractOdbc(getExtractField(), child(0), getFieldFunction());
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Extract::copyTopNode()

NABoolean Extract::isAUserSuppliedInput() const             
{ 
   ItemExpr * extractChild = NULL;
   if ( child(0) && (child(0)->getOperatorType() == ITM_CAST) && 
	child(0)->child(0) )
      extractChild = child(0)->child(0)->castToItemExpr();
   else if (child(0))
	extractChild = child(0)->castToItemExpr();

   if ((CmpCommon::getDefault(COMP_BOOL_185) == DF_ON) &&
       (extractChild && extractChild->isAUserSuppliedInput()) &&
       (NOT(extractChild->doesExprEvaluateToConstant(FALSE))))
      return TRUE; 
   else
      return FALSE;
} // Extract::isAUserSuppliedInput() 

// ----------------------------------------------------------------------
// Walk through an ItemExpr tree and gather the ValueIds of those
// expressions that behave as if they are "leaves" for the sake of
// the coverage test. In this case it is expressions of the type
// extract(cast(timestamp))
// ----------------------------------------------------------------------
void Extract::getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                        const GroupAttributes& coveringGA,
                                        const ValueIdSet & newExternalInputs) const
{ 
  if (isAUserSuppliedInput())
    leafValues += getValueId();
  else
    BuiltinFunction::getLeafValuesForCoverTest(leafValues, coveringGA, newExternalInputs);
}


NABoolean Extract::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  Extract * tmp = (Extract *) other;

  return 
    (this->extractField_ == tmp->extractField_ ) &&
      (this->fieldFunction_ == tmp->fieldFunction_);
}

QR::ExprElement Extract::getQRExprElem() const
{
  return QR::QRFunctionWithParameters;
}

// -----------------------------------------------------------------------
// member functions for class JulianTimestamp
// -----------------------------------------------------------------------
JulianTimestamp::~JulianTimestamp() {}

ItemExpr * JulianTimestamp::copyTopNode(ItemExpr *derivedNode,
					CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) JulianTimestamp(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);

} // JulianTimestamp::copyTopNode()

// Fix for cr 10-010718-3967
// We dont need this method, the reason this method was added was to return
// true, the ItemExpr method already returns false, so we don't need this
// method. So basically we want FALSE to be returned for JulianTimestamp all
// the time.
// NABoolean JulianTimestamp::isAUserSuppliedInput() const      { return TRUE; }

// -----------------------------------------------------------------------
// member functions for class StatementExecutionCount
// -----------------------------------------------------------------------

NABoolean StatementExecutionCount::isAUserSuppliedInput() const { return TRUE; }

ItemExpr * StatementExecutionCount::copyTopNode(ItemExpr *derivedNode,
						CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) StatementExecutionCount();
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);
}

// -----------------------------------------------------------------------
// member functions for class CurrentTransId
// -----------------------------------------------------------------------

NABoolean CurrentTransId::isAUserSuppliedInput() const { return TRUE; }

ItemExpr * CurrentTransId::copyTopNode(ItemExpr *derivedNode,
				       CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) CurrentTransId();
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);
}

// -----------------------------------------------------------------------
// member functions for class Upper
// -----------------------------------------------------------------------
Upper::~Upper() {}

ItemExpr * Upper::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Upper(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Upper::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class Lower
// -----------------------------------------------------------------------
Lower::~Lower() {}

ItemExpr * Lower::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Lower(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Lower::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class Trim
// -----------------------------------------------------------------------
Trim::~Trim() {}

ItemExpr * Trim::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Trim(getTrimMode(), child(0), child(1));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Trim::copyTopNode()

NABoolean Trim::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  Trim * t = (Trim *) other;

  return 
    (mode_ == t->mode_);
}
// -----------------------------------------------------------------------
// member functions for class Increment
// -----------------------------------------------------------------------
Increment::~Increment() {}

ItemExpr * Increment::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Increment(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Increment::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class Decrement
// -----------------------------------------------------------------------
Decrement::~Decrement() {}

ItemExpr * Decrement::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Decrement(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Decrement::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class TriRelational
// -----------------------------------------------------------------------

TriRelational::TriRelational(OperatorTypeEnum optype,
			     ItemExpr *val1Ptr,
			     ItemExpr *val2Ptr,
			     ItemExpr *val3Ptr)
     : BuiltinFunction(optype, CmpCommon::statementHeap(),
                       3, val1Ptr, val2Ptr, val3Ptr)
{
  CMPASSERT(optype == ITM_LESS_OR_LE OR
	 optype == ITM_GREATER_OR_GE);
}

TriRelational::~TriRelational() {}

ItemExpr * TriRelational::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) TriRelational(getOperatorType(),NULL,NULL,NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for ScalarVariance
// -----------------------------------------------------------------------
ScalarVariance::~ScalarVariance() {}

ItemExpr *
ScalarVariance::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    {
      CMPASSERT(getArity() == 3);
      result = new (outHeap)
	ScalarVariance(getOperatorType(), child(0), child(1), child(2));
    }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // ScalarVariance::copyTopNode()


// -----------------------------------------------------------------------
// member functions for UnPackCol
// -----------------------------------------------------------------------
//
UnPackCol::~UnPackCol() {}

ItemExpr *
UnPackCol::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    {
      CMPASSERT(getArity() == 2);
      result = new (outHeap)
	UnPackCol(child(0),
		  child(1),
		  width_,
		  base_,
		  nullsPresent_,
		  type_->newCopy(outHeap));
    }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // UnPackCol::copyTopNode()

NABoolean UnPackCol::isCovered(const ValueIdSet& newExternalInputs,
                               const GroupAttributes& newRelExprAnchorGA,
                               ValueIdSet& referencedInputs,
                               ValueIdSet& coveredSubExpr,
                               ValueIdSet& unCoveredExpr) const
{
  // ---------------------------------------------------------------------
  // If the operand is covered, then return its ValueId in coveredSubExpr.
  // ---------------------------------------------------------------------
  ValueIdSet localSubExpr;
  for(Lng32 i = 0; i < (Lng32)getArity(); i++)
  {
    if(newRelExprAnchorGA.covers(child(i)->getValueId(),
                                 newExternalInputs,
                                 referencedInputs,
                                 &localSubExpr))
    {
      coveredSubExpr += child(i)->getValueId();
    }
  }

  // ---------------------------------------------------------------------
  // The UnPackCol function is coerced to fail the coverage test even when
  // its operands isCovered(). This is because only the UnPackCol node can
  // evaluate the function. The function is associated with a UnPackCol
  // node at the very beginning and we don't allow it to be pushed down
  // even if the function's operands are covered at the node's child.
  // ---------------------------------------------------------------------
  return FALSE;
}

void UnPackCol::getLeafValuesForCoverTest(ValueIdSet& leafValues, 
                                          const GroupAttributes& coveringGA,
                                          const ValueIdSet & newExternalInputs) const
{
  // UnPackCol is considered a leaf operator for cover test.
  leafValues += getValueId();
}

// -----------------------------------------------------------------------
// member functions for RowsetArrayScan
// -----------------------------------------------------------------------
//
RowsetArrayScan::~RowsetArrayScan() {}

ItemExpr *
RowsetArrayScan::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL) {
    CMPASSERT(getArity() == 2);
    result = new (outHeap)
      RowsetArrayScan(child(0),
                      child(1),
                      maxNumElem_,
                      elemSize_,
                      elemNullInd_,
                      elemType_->newCopy(outHeap),
                      getOperatorType());
  }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

}

NABoolean RowsetArrayScan::isCovered(const ValueIdSet& newExternalInputs,
                                     const GroupAttributes& newRelExprAnchorGA,
                                     ValueIdSet& referencedInputs,
                                     ValueIdSet& coveredSubExpr,
                                     ValueIdSet& unCoveredExpr) const
{
  // ---------------------------------------------------------------------
  // If the operand is covered, then return its ValueId in coveredSubExpr.
  // ---------------------------------------------------------------------
  ValueIdSet localSubExpr;
  for (Lng32 i = 0; i < (Lng32)getArity(); i++) {
    if (newRelExprAnchorGA.covers(child(i)->getValueId(),
                                  newExternalInputs,
                                  referencedInputs,
                                  &localSubExpr)) {
      coveredSubExpr += child(i)->getValueId();
    }

    coveredSubExpr += localSubExpr;

  }

  // ---------------------------------------------------------------------
  // The RowsetArrayScan function is coerced to fail the coverage test even
  // when its operands isCovered(). This is because only the RowsetArrayScan
  // node can evaluate the function. The function is associated with a
  // RowsetArrayScan node at the very beginning and we don't allow it to be
  // pushed down even if the function's operands are covered at the node's
  // child.
  // ---------------------------------------------------------------------
  return FALSE;
}

void RowsetArrayScan::getLeafValuesForCoverTest(ValueIdSet& leafValues, 
                                                const GroupAttributes& coveringGA,
                                                const ValueIdSet & newExternalInputs) const
{
  // RowsetArrayScan is considered a leaf operator for cover test.
  leafValues += getValueId();
}

// -----------------------------------------------------------------------
// member functions for RowsetArrayInto
// -----------------------------------------------------------------------
//
RowsetArrayInto::~RowsetArrayInto() {}

ItemExpr *
RowsetArrayInto::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL) {
    CMPASSERT(getArity() == 2);
    result = new (outHeap)
      RowsetArrayInto(child(0),
                      child(1),
                      maxNumElem_,
                      elemSize_,
                      elemNullInd_,
                      hostVarType_->newCopy(outHeap),
                      getOperatorType());
  }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

NABoolean RowsetArrayInto::isCovered(const ValueIdSet& newExternalInputs,
                                     const GroupAttributes& newRelExprAnchorGA,
                                     ValueIdSet& referencedInputs,
                                     ValueIdSet& coveredSubExpr,
                                     ValueIdSet& unCoveredExpr) const
{
  // ---------------------------------------------------------------------
  // If the operand is covered, then return its ValueId in coveredSubExpr.
  // ---------------------------------------------------------------------
  ValueIdSet localSubExpr;
  for (Lng32 i = 0; i < (Lng32)getArity(); i++) {
    if (newRelExprAnchorGA.covers(child(i)->getValueId(),
                                  newExternalInputs,
                                  referencedInputs,
                                  &localSubExpr)) {
      coveredSubExpr += child(i)->getValueId();
    }
    coveredSubExpr += localSubExpr;
  }

  // ---------------------------------------------------------------------
  // The RowsetArrayInto function is coerced to fail the coverage test even
  // when its operands isCovered(). This is because only the RowsetArrayInto
  // node can evaluate the function. The function is associated with a
  // RowsetArrayInto node at the very beginning and we don't allow it to be
  // pushed down even if the function's operands are covered at the node's
  // child.
  // ---------------------------------------------------------------------
  return FALSE;
}

void RowsetArrayInto::getLeafValuesForCoverTest(ValueIdSet& leafValues, 
                                                const GroupAttributes& coveringGA,
                                                const ValueIdSet & newExternalInputs) const
{
  // RowsetArrayScan is considered a leaf operator for cover test.
  leafValues += getValueId();
}

// -----------------------------------------------------------------------
// member functions for class RangeLookup
// -----------------------------------------------------------------------

RangeLookup::RangeLookup(ItemExpr *val1Ptr,
			 const RangePartitioningFunction *partFunc) :
     BuiltinFunction(ITM_RANGE_LOOKUP, CmpCommon::statementHeap(), 1, val1Ptr),
     partFunc_(partFunc)
{
}

RangeLookup::~RangeLookup() {}

ItemExpr * RangeLookup::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) RangeLookup(NULL,partFunc_);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

Lng32 RangeLookup::splitKeysLen()
{
  // we assume a dense array of character strings, with #parts + 1 entries
  // (no NULL terminators, no fillers)
  return (partFunc_->getCountOfPartitions() + 1) *
    partFunc_->getRangePartitionBoundaries()->getEncodedBoundaryKeyLength();
}

void RangeLookup::copySplitKeys(char *tgt, Lng32 tgtLen)
{
  CMPASSERT(tgtLen = splitKeysLen());
  const RangePartitionBoundaries * b =
    partFunc_->getRangePartitionBoundaries();
  Lng32 numEntries = partFunc_->getCountOfPartitions() + 1;
  Lng32 entryLen   = b->getEncodedBoundaryKeyLength();
  Lng32 offset     = 0;

  for (Lng32 i = 0; i < numEntries; i++)
    {
      str_cpy_all(&tgt[offset],
		  b->getBinaryBoundaryValue(i),
		  entryLen);
      offset += entryLen;
    }
}

Lng32 RangeLookup::getNumOfPartitions()
{
  return partFunc_->getCountOfPartitions();
}

Lng32 RangeLookup::getEncodedBoundaryKeyLength()
{
  return partFunc_->getRangePartitionBoundaries()->
    getEncodedBoundaryKeyLength();
}

// -----------------------------------------------------------------------
// member functions for class PackFunc
// -----------------------------------------------------------------------

PackFunc::PackFunc(ItemExpr* val1Ptr,
                   ItemExpr* pf,
                   Lng32 base,
                   Lng32 width,
                   NABoolean nullsPresent)
 : BuiltinFunction(ITM_PACK_FUNC,
                   CmpCommon::statementHeap(),
                   2,val1Ptr,pf),
   isFormatInfoValid_(TRUE),
   base_(base),
   width_(width),
   nullsPresent_(nullsPresent),
   type_(NULL)
{
  deriveTypeFromFormatInfo();
}

PackFunc::PackFunc(ItemExpr* val1Ptr,
                  ItemExpr* pf,
                  const NAType* unpackType)
 : BuiltinFunction(ITM_PACK_FUNC,
                   CmpCommon::statementHeap(),
                   2,val1Ptr,pf),
   isFormatInfoValid_(FALSE)
{
  deriveFormatInfoFromUnpackType(unpackType);
}

void PackFunc::deriveTypeFromFormatInfo()
{
  CMPASSERT(isFormatInfoValid_);

  // The packing factor must be stored as a constant.
  NABoolean negateIt;
  ConstValue* pfconst = child(1)->castToConstValue(negateIt);
  CMPASSERT(pfconst AND negateIt == FALSE);

  // The packing factor must be stored as a long in the constant.
  CMPASSERT(pfconst->getStorageSize() == sizeof(Lng32));
  CMPASSERT(pfconst->getType()->getTypeQualifier() == NA_NUMERIC_TYPE);
  Lng32 pf;
  memcpy(&pf,pfconst->getConstValue(),sizeof(Lng32));

  Lng32 dataSizeInBytes = (width_ < 0 ? (-width_-1)/8+1 : width_);
  Lng32 packedRowSizeInBytes = (base_ + dataSizeInBytes);
  type_ = new(CmpCommon::statementHeap()) SQLChar(CmpCommon::statementHeap(), packedRowSizeInBytes,FALSE);
}

void PackFunc::deriveFormatInfoFromUnpackType(const NAType* unpackType)
{
  CMPASSERT(unpackType);

  // Packing of varying length column not (yet?) supported.
  // It should have been enforced when the packed table is created.
  CMPASSERT(NOT DFS2REC::isAnyVarChar(unpackType->getFSDatatype()));

  // Save four byte for the packing factor to be stored.
  Lng32 pfSizeInBytes = sizeof(Int32);

  // The packing factor must be stored as a constant.
  NABoolean negateIt;
  ConstValue* pfconst = child(1)->castToConstValue(negateIt);
  CMPASSERT(pfconst AND negateIt == FALSE);

  // The packing factor must be stored as a long in the constant.
  CMPASSERT(pfconst->getStorageSize() == sizeof(Lng32));
  CMPASSERT(pfconst->getType()->getTypeQualifier() == NA_NUMERIC_TYPE);
  Lng32 pf;
  memcpy(&pf,pfconst->getConstValue(),sizeof(Lng32));

  // No of bits reserved for null indicators.
  nullsPresent_ = unpackType->supportsSQLnullPhysical();
  Lng32 nullIndLenInBytes = (nullsPresent_ ? (pf-1)/8+1 : 0);
  base_ = pfSizeInBytes + nullIndLenInBytes;

  // Storage size for the actual packed column values in bytes.
  Lng32 dataSizeInBytes;

  // For bit precision integers, width needs to be in negative no of bits.
  if(unpackType->getFSDatatype() == REC_BPINT_UNSIGNED)
  {
    width_ = ((SQLBPInt*)unpackType)->getDeclaredLength();
    dataSizeInBytes = (width_*pf-1)/8+1;
    width_ = -width_;
  }
  else
  {
    width_ = unpackType->getNominalSize();
    dataSizeInBytes = width_ * pf;
  }

  // Now, the length of the contents of a packed record in bytes.
  Lng32 packedRowSizeInBytes = (base_ + dataSizeInBytes);

  // Synthesize type of the packed column.
  type_ = new(CmpCommon::statementHeap()) SQLChar(CmpCommon::statementHeap(), packedRowSizeInBytes,FALSE);
  isFormatInfoValid_ = TRUE;
}

PackFunc::~PackFunc() {}

NABoolean PackFunc::isCovered(const ValueIdSet& newExternalInputs,
                              const GroupAttributes& newRelExprAnchorGA,
                              ValueIdSet& referencedInputs,
                              ValueIdSet& coveredSubExpr,
                              ValueIdSet& unCoveredExpr) const
{
  // ---------------------------------------------------------------------
  // If the operand is covered, then return its ValueId in coveredSubExpr.
  // ---------------------------------------------------------------------
  ValueIdSet localSubExpr;
  for(Lng32 i = 0; i < (Lng32)getArity(); i++)
  {
    if(newRelExprAnchorGA.covers(child(i)->getValueId(),
                                 newExternalInputs,
                                 referencedInputs,
                                 &localSubExpr))
    {
      coveredSubExpr += child(i)->getValueId();
    }
  }

  // ---------------------------------------------------------------------
  // The packing function is coerced to fail the coverage test even when
  // its operands isCovered(). This is because only the Pack node can
  // evaluate a packing function. The function is associated with a Pack
  // node at the very beginning and we don't allow it to be pushed down
  // even if the function's operands are covered at the Pack node's child.
  // ---------------------------------------------------------------------
  return FALSE;
}

void PackFunc::getLeafValuesForCoverTest(ValueIdSet& leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const
{
  // see note in isCovered().
  leafValues += getValueId();
}

ItemExpr* PackFunc::copyTopNode(ItemExpr* derivedNode, CollHeap* outHeap)
{
  ItemExpr* result;

  if(derivedNode == NULL)
    result = new (outHeap) PackFunc (child(0),child(1));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);
}

UDFunction::~UDFunction()
{
  delete udfDesc_;
}

Int32 UDFunction::getArity() const
{
  return getNumChildren(); // Function class member function.
}

const NAString UDFunction::getText() const
{
  return functionName_.getExternalName();
}

HashValue UDFunction::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= functionName_.getExternalName() + actionName_;

  return result;
}

NABoolean Function::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  return TRUE;
}

NABoolean UDFunction::duplicateMatch(const ItemExpr & other) const
{
  // Check to see if the arguments are the same, among other things.
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  UDFunction &o = (UDFunction &) other;

  // compare all local data members of the derived class
  // and return FALSE if they don't match
  if (functionName_.getExternalName() != o.functionName_.getExternalName() ||
      actionName_                     != o.actionName_  ||
      hasSubquery_                    != o.hasSubquery_ ||
      inputVars_                      != o.inputVars_   ||
      outputVars_                     != o.outputVars_)
    return FALSE;

  if (udfDesc_ && o.udfDesc_) // RoutineDesc pointers
    if (!(*udfDesc_ == *o.udfDesc_))
      return FALSE;

  if (udfDesc_ != NULL || o.udfDesc_ != NULL) // Both must be NULL to proceed.
    return FALSE;


  return TRUE;
}

ItemExpr *UDFunction::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  UDFunction *result;

  if (derivedNode == NULL)
    result = new (outHeap) UDFunction(functionName_, getArity(), outHeap);
  else
    result = (UDFunction *) derivedNode;

  result->heap_        = outHeap;
  result->functionName_= functionName_;
  result->functionNamePos_= functionNamePos_;
  result->actionName_  = actionName_  ;
  result->udfDesc_     = udfDesc_ != NULL ? 
                           new (outHeap) RoutineDesc(*udfDesc_, outHeap) : NULL;
  result->hasSubquery_ = hasSubquery_ ;
  result->inputVars_   = inputVars_   ;
  result->outputVars_  = outputVars_  ;


  return Function::copyTopNode(result, outHeap);
}

void UDFunction::unparse(NAString &result,
		      PhaseEnum phase,
                      UnparseFormatEnum form,
		      TableDesc * tabId) const
{
  NAString kwd(getText(), CmpCommon::statementHeap());
  if (form == USER_FORMAT_DELUXE) kwd.toUpper();
  result += kwd;
  result += "(";
  CollIndex lastComma = getNumChildren() -1;
  for (CollIndex i=0; i<(CollIndex ) getNumChildren(); i++)
  {
     child(i)->unparse(result,phase,form,tabId);
     if (i < lastComma) result += ",";
  }
  result += ")";

}

ExprValueId &UDFunction::operator[] (Lng32 index)
{
  return Function::operator[](index);
}
const ExprValueId &UDFunction::operator[] (Lng32 index) const
{
  return Function::operator[](index);
}

ItemExpr *UDFunction::containsUDF()
{
  return this;
}

NABoolean UDFunction::containsIsolatedUDFunction()
{
  NABoolean containsIsolatedUDF(FALSE);

  for (CollIndex i=0; i < (CollIndex) getNumChildren(); i++)
  {
    if (child(i)->containsIsolatedUDFunction() == TRUE)
      containsIsolatedUDF = TRUE;
  }

  if (containsIsolatedUDF == TRUE) 
    return TRUE;

  // We didn't have one as an input parameter, check to see if we
  // are isolated..
  const RoutineDesc *rdesc = getRoutineDesc();
  if (rdesc == NULL || rdesc->getEffectiveNARoutine() == NULL ) return FALSE;  
  return ( rdesc->getEffectiveNARoutine()->isIsolate() ? TRUE : FALSE ) ;
} // UDFunction::containsIsolatedUDFunction

// Get the output degree of this function.  If it is >1, getOutputItem
// should be used to get the ItemExpr of each output.
int UDFunction::getOutputDegree()
{
  CCMPASSERT(getRoutineDesc() != NULL);
  if (!getRoutineDesc()) return -1; // Routine desc not set.  Error.
  else 
    return getRoutineDesc()->getOutputColumnList().entries();
}

// Get i'th output argument of UDF as ItemExpr.
ItemExpr *UDFunction::getOutputItem(unsigned int i)
{
  CCMPASSERT(getRoutineDesc() != NULL);
  if (!getRoutineDesc()) return NULL; // Routine desc not set.  Error.
  else 
  {
    if (i < getRoutineDesc()->getOutputColumnList().entries())
      return getRoutineDesc()->getOutputColumnList()[i].getItemExpr();
    else return NULL; // 'i' out of bounds.  Error.
  }
}

ItemExpr * NotIn::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  NotIn *result;

  if (derivedNode == NULL)
    result = new (outHeap) NotIn();
  else
    result = (NotIn *) derivedNode;

  //copy the data members
  result->equivEquiPredicate_ = equivEquiPredicate_;
  result->equivNonEquiPredicate_ = equivNonEquiPredicate_;
  result->isOneInnerBroadcastRequired_ = isOneInnerBroadcastRequired_;

  return BiRelat::copyTopNode(result, outHeap);
}

ValueId NotIn::createEquivEquiPredicate() const
{
  CMPASSERT ( child(0)->getOperatorType() != ITM_ITEM_LIST && 
              child(1)->getOperatorType() != ITM_ITEM_LIST) 

  ItemExpr * newPred = new (CmpCommon::statementHeap()) 
                        BiRelat(ITM_EQUAL, 
                                child(0),
                                child(1));
   
  newPred->synthTypeAndValueId(TRUE);
  
  return newPred->getValueId();
}
ValueId NotIn::createEquivNonEquiPredicate() const
{
  CMPASSERT ( child(0)->getOperatorType() != ITM_ITEM_LIST && 
              child(1)->getOperatorType() != ITM_ITEM_LIST) 

  ItemExpr * newPred = new (CmpCommon::statementHeap()) 
                        BiRelat(ITM_NOT_EQUAL, 
                                child(0),
                                child(1));
            
  newPred = new (CmpCommon::statementHeap()) 
                        UnLogic(ITM_IS_TRUE, newPred);
 
  newPred = new (CmpCommon::statementHeap()) 
                        UnLogic(ITM_NOT, newPred);

  newPred->synthTypeAndValueId(TRUE);
    
  return newPred->getValueId();
}

void NotIn::cacheEquivEquiPredicate()
{
      
  if (getEquivEquiPredicate() == NULL_VALUE_ID)
  {
    ValueId vid = createEquivEquiPredicate();
        
    ItemExpr * itm = vid.getItemExpr();
        
    ((BiRelat *)itm)->setIsNotInPredTransform(TRUE); 
        
    ((BiRelat *)itm)->setOuterNullFilteringDetected(
                          getOuterNullFilteringDetected());
    ((BiRelat *)itm)->setInnerNullFilteringDetected(
                          getInnerNullFilteringDetected());

    setEquivEquiPredicate(vid);
  }
}

void NotIn::cacheEquivNonEquiPredicate()
{
  if (getEquivNonEquiPredicate() == NULL_VALUE_ID )
  {
    setEquivNonEquiPredicate(
		  createEquivNonEquiPredicate());
  }
}


void  NotIn::cacheIsOneInnerBroadcastRequired()
{

  if (isOneInnerBroadcastRequired_ == NotIn::NOT_SET)
  {

    isOneInnerBroadcastRequired_ = NotIn::NOT_REQUIRED;

    ItemExpr * child0 = child(0);
    const NAType &outerType = child0->getValueId().getType();

    if (outerType.supportsSQLnull()  &&
        !getOuterNullFilteringDetected())
    {
      isOneInnerBroadcastRequired_ = NotIn::REQUIRED;
    }
  }
  
}//NotIn::cacheIsOneInnerBroadcastRequired()




// -----------------------------------------------------------------------
// member functions for class BiLogic
// -----------------------------------------------------------------------
Int32 BiLogic::getArity() const { return 2; }

NABoolean BiLogic::isAPredicate() const { return TRUE; }

// return true iff we're an expansion of a LIKE predicate
NABoolean BiLogic::isLike() const
{
  switch (getOperatorType()) {
  case ITM_AND:
  case ITM_OR:
    {
      Int32 arity = getArity();
      for (Int32 x = 0; x < arity; x++) {
        switch (child(x)->getOperatorType()) {
        case ITM_GREATER_EQ:
        case ITM_LESS:
          if (!((BiRelat*)child(x).getPtr())->derivativeOfLike())
            return FALSE; // we're not an expansion of a LIKE predicate
          else // we may be part of an expansion of a LIKE predicate
            continue; // keep looking
          break;
        default:
          return FALSE; // we're not an expansion of a LIKE predicate
          break;
        }
      }
      return TRUE; // yes, we are an expansion of a LIKE predicate
    }
    break;
  default:
    return FALSE; // we're not an expansion of a LIKE predicate
  }
}

// -----------------------------------------------------------------------
// BiLogic::isCovered()
// -----------------------------------------------------------------------
NABoolean BiLogic::isCovered
                      (const ValueIdSet& newExternalInputs,
		       const GroupAttributes& coveringGA,
		       ValueIdSet& referencedInputs,
		       ValueIdSet& coveredSubExpr,
		       ValueIdSet& unCoveredExpr) const
{
  if (getOperatorType() == ITM_OR)
    {
      // ----------------------------------------------------------------
      // The subtree that is rooted in an OR can contain values that
      // are produced by different sources. The entire expression tree
      // including the OR at the root is covered, if the subtree that
      // is rooted in the OR is covered. Otherwise it is not covered.
      // The coverage test insists on a complete coverage of all the
      // expressions at the leaves.
      // ----------------------------------------------------------------
      ValueIdSet leafValues;
      getLeafValuesForCoverTest(leafValues, coveringGA, newExternalInputs);
      return leafValues.isCovered(newExternalInputs,
				  coveringGA,
				  referencedInputs,
				  coveredSubExpr,
				  unCoveredExpr);
    }
  else
    return ItemExpr::isCovered(newExternalInputs,
			       coveringGA,
			       referencedInputs,
			       coveredSubExpr,
			       unCoveredExpr);
} // BiLogic::isCovered()

NABoolean BiLogic::duplicateMatch(const ItemExpr& other) const
{
  return genericDuplicateMatch(other);
}

ItemExpr * BiLogic::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  BiLogic *result;

  if (derivedNode == NULL)
    result = new (outHeap) BiLogic(getOperatorType());
  else
    result = (BiLogic*)derivedNode;

  result->setNumLeaves(getNumLeaves());
  result->createdFromINlist_ = createdFromINlist_;

  return ItemExpr::copyTopNode(result, outHeap);
}

ItemExpr* BiLogic::getINlhs() 
{
  if (createdFromINlist() && child(1)->getOperatorType() == ITM_EQUAL)
    return child(1)->child(0);
  else
    return NULL;
}

const NAString BiLogic::getText() const
{
  switch (getOperatorType())
    {
    case ITM_AND:
      return "and";
    case ITM_OR:
      return "or";
    default:
      return "unknown BiLogic";
    } // switch
} // BiLogic::getText()

QR::ExprElement BiLogic::getQRExprElem() const
{
  return QR::QRBinaryOperElem;
}

// -----------------------------------------------------------------------
// member functions for class UnLogic
// -----------------------------------------------------------------------
Int32 UnLogic::getArity() const { return 1; }

NABoolean UnLogic::isAPredicate() const { return TRUE; }

const NAString UnLogic::getText() const
{
  switch (getOperatorType())
    {
    case ITM_NOT:
      return "not";
    case ITM_IS_TRUE:
      return "is true";
    case ITM_IS_FALSE:
      return "is false";
    case ITM_IS_NULL:
      return "is null";
    case ITM_IS_NOT_NULL:
      return "is not null";
    case ITM_IS_UNKNOWN:
      return "is unknown";
    case ITM_IS_NOT_UNKNOWN:
      return "is not unknown";
    default:
      return "unknown UnLogic??";
    } // switch
} // UnLogic::getText()

NABoolean UnLogic::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  return TRUE;
}

ItemExpr * UnLogic::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) UnLogic(getOperatorType());
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

QR::ExprElement UnLogic::getQRExprElem() const
{
  return QR::QRUnaryOperElem;
}

// -----------------------------------------------------------------------
// member functions for class BiRelat
// -----------------------------------------------------------------------
NABoolean BiRelat::derivativeOfLike()
{
  if (originalLikeExprId() != NULL_VALUE_ID)
    return TRUE;
  else
    return FALSE;
}

void BiRelat::adjustRowcountAndUecForLike(const ColStatDescSharedPtr& colStatDesc,
					CostScalar rowCountBeforePreds,
					CostScalar totalUecBeforePreds,
					CostScalar baseUecBeforePreds)
{
  // This range predicate is a derivative of Like predicate.
  // Hence we cannot use the usual selectivity obtained after
  // applying range predicates. Here we use a portion of the
  // default selectivity for like predicates to obtain the
  // resultant rowcount. We still continue to use the histograms
  // obtained after applying range predicates the usual way
  // The selectivity would be applied to the inital rowcount
  // before any predicates were applied.

  ColStatsSharedPtr colStats = colStatDesc->getColStatsToModify() ;

  CostScalar nrc  = MIN_ONE_CS(rowCountBeforePreds * getLikeSelectivity());
  CostScalar nuec = MIN_ONE_CS(totalUecBeforePreds * getLikeSelectivity());
  CostScalar baseuec = MIN_ONE_CS(baseUecBeforePreds * getLikeSelectivity());


  if ( colStats->isUnique() )
  {
    // If this is a UNIQUE column, UEC == rowcount
    nuec = nrc;
  }
  else
  {
    nuec = MINOF(nrc, nuec);
  }

  HistogramSharedPtr hist = colStats->getHistogramToModify() ;
  hist->condenseToSingleInterval();
  colStats->setIsCompressed(TRUE);

  // first, set the aggregate values
  colStats->setRowsAndUec  ( nrc, nuec );
  colStats->setRedFactor   ( csOne );
  colStats->setUecRedFactor( csOne );
  colStats->setBaseUec(baseuec);

  // Set first interval's rowcount and uec.
  hist->getFirstInterval().setRowsAndUec( nrc, nuec );

  // from this point forward, we're going to consider this a fake histogram
  colStats->setFakeHistogram();
}

Int32 BiRelat::getArity() const { return 2; }

NABoolean BiRelat::isAPredicate() const { return TRUE; }

const NAString BiRelat::getText() const
{
  switch (getOperatorType())
    {
    case ITM_EQUAL:
      return "=";
    case ITM_NOT_EQUAL:
      return "<>";
    case ITM_LESS:
      return "<";
    case ITM_LESS_EQ:
      return "<=";
    case ITM_GREATER:
      return ">";
    case ITM_GREATER_EQ:
      return ">=";
    default:
      return "unknown BiRelat";
    } // switch

 } // BiRelat::getText()

NABoolean BiRelat::duplicateMatch(const ItemExpr& other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  const BiRelat &o = (BiRelat &) other;

  if (specialNulls_ != o.specialNulls_ OR
      specialMultiValuePredicateTransformation_ !=
      o.specialMultiValuePredicateTransformation_ OR
      directionVector_ != o.directionVector_ /* ptr comparison only */ OR
      isaPartKeyPred_ != o.isaPartKeyPred_ OR
      originalLikeExprId_ != o.originalLikeExprId_ OR
      likeSelectivity_ != o.likeSelectivity_ OR
      derivedFromMCRP_ != o.derivedFromMCRP_ OR
      preferForSubsetScanKey_ != o.preferForSubsetScanKey_
      )
    return FALSE;


  // do this if this a range predicate derived from a MCRP
  // A MCRP (MultiColumnRangePredicate) is a predicate of the form
  // (a, b, c) > (1, 2, 3). Such a predicate is transformed to
  // (a >= 1) and ((a, b, c) > (1, 2, 3)). Notice the transformation
  // adds predicate (a >= 1). The derivedFromMCRP flag is set to TRUE
  // for predicate (a >= 1).
  if ((derivedFromMCRP_ == TRUE) &&
      (listOfComparisonExprs_ != o.listOfComparisonExprs_))
  {
    if (listOfComparisonExprIds_.entries() != o.listOfComparisonExprIds_.entries())
      return FALSE;

    for (CollIndex i=0; i < listOfComparisonExprIds_.entries(); i++)
    {
      if (listOfComparisonExprIds_[i] != o.listOfComparisonExprIds_[i])
        return FALSE;
    }
  }

  return TRUE;
}

ItemExpr * BiRelat::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  BiRelat *result;

  if (derivedNode == NULL)
    result = new (outHeap) BiRelat(getOperatorType());
  else
    result = (BiRelat *) derivedNode;

  // copy the data members
  result->specialNulls_ = specialNulls_;
  result->specialMultiValuePredicateTransformation_ =
    specialMultiValuePredicateTransformation_;
  result->isaPartKeyPred_ = isaPartKeyPred_;
  result->originalLikeExprId_ = originalLikeExprId_;

  result->setDirectionVector(directionVector_);
  result->likeSelectivity_ = likeSelectivity_;
  result->derivedFromMCRP_ = derivedFromMCRP_;
  result->listOfComparisonExprs_ = listOfComparisonExprs_;
  result->listOfComparisonExprIds_ = listOfComparisonExprIds_;
  result->leftMCRPChildList_ = leftMCRPChildList_;
  result->rightMCRPChildList_ = rightMCRPChildList_;
  result->preferForSubsetScanKey_ = preferForSubsetScanKey_;

  result->createdFromINlist_ = createdFromINlist_;
  
  result->collationEncodeComp_  = collationEncodeComp_;
  result->isNotInPredTransform_ = isNotInPredTransform_;
  result->outerNullFilteringDetected_= outerNullFilteringDetected_;
  result->innerNullFilteringDetected_ = innerNullFilteringDetected_;

  result->rollupColumnNum_ = rollupColumnNum_;

  result->flags_ = flags_;

  return ItemExpr::copyTopNode(result, outHeap);
}

void BiRelat::getLeftMCRPChildList(ValueIdList & leftMCRPChildList)
{
  if ((!isDerivedFromMCRP()) ||
      (!listOfComparisonExprIds_.entries()))
    return;

  if (leftMCRPChildList_.entries())
  {
    leftMCRPChildList = leftMCRPChildList_;
    return;
  }

  // iterate over the list of comparisons
  for(CollIndex i=0; i < listOfComparisonExprIds_.entries(); i++)
  {
    //get comparison at location i
    BiRelat * comparison = (BiRelat *) listOfComparisonExprIds_[i].getItemExpr();

    ItemExpr * leftChild = (ItemExpr *) comparison->child(0);
    ItemExpr * rightChild = (ItemExpr *) comparison->child(1);
    leftMCRPChildList_.insert(leftChild->getValueId());
    rightMCRPChildList_.insert(rightChild->getValueId());
  }

  leftMCRPChildList = leftMCRPChildList_;
}

void BiRelat::getRightMCRPChildList(ValueIdList & rightMCRPChildList)
{
  if ((!isDerivedFromMCRP()) ||
      (!listOfComparisonExprIds_.entries()))
    return;

  if (rightMCRPChildList_.entries())
  {
    rightMCRPChildList = rightMCRPChildList_;
    return;
  }

  // iterate over the list of comparisons
  for(CollIndex i=0; i < listOfComparisonExprIds_.entries(); i++)
  {
    //get comparison at location i
    BiRelat * comparison = (BiRelat *) listOfComparisonExprIds_[i].getItemExpr();

    ItemExpr * leftChild = (ItemExpr *) comparison->child(0);
    ItemExpr * rightChild = (ItemExpr *) comparison->child(1);
    leftMCRPChildList_.insert(leftChild->getValueId());
    rightMCRPChildList_.insert(rightChild->getValueId());
  }

  rightMCRPChildList = rightMCRPChildList_;
}

QR::ExprElement BiRelat::getQRExprElem() const
{
  return QR::QRBinaryOperElem;
}

OperatorTypeEnum BiRelat::getRelaxedComparisonOpType() const
{
  switch (getOperatorType())
   {
   case ITM_LESS:
     return ITM_LESS_EQ;

   case ITM_GREATER:
     return ITM_GREATER_EQ;

   case ITM_NOT_EQUAL:
     CMPASSERT(0); // the relaxed form would be TRUE,
                   // but we don't handle that right now

   default:
     break;
   }

 return getOperatorType();
}


// Member functions for class KeyRangeCompare //

const NAString KeyRangeCompare::getText() const
{

  NAString result("", CmpCommon::statementHeap());

  switch (getOperatorType())
    {
    case ITM_EQUAL:
      result += "=";
      break;
    case ITM_NOT_EQUAL:
      result += "<>";
      break;
    case ITM_LESS:
      result += "<";
      break;
    case ITM_LESS_EQ:
      result += "<=";
      break;
    case ITM_GREATER:
      result += ">";
      break;
    case ITM_GREATER_EQ:
      result += ">=";
      break;
    default:
      result +=  "unknown BiRelat";
      return result;
    } // switch

  result += " (KEY_RANGE_COMPARE)";

  return result;

 } // KeyRangeCompare::getText()

// -----------------------------------------------------------------------
// member functions for class ConstValue
// -----------------------------------------------------------------------
// constructor for an untyped NULL constant
ConstValue::ConstValue()
: ItemExpr(ITM_CONSTANT)
     , isNull_(IS_NULL)
     , type_(new (CmpCommon::statementHeap()) SQLUnknown(CmpCommon::statementHeap(), TRUE))
     , value_(NULL)
     , storageSize_(0)
     , text_(new (CmpCommon::statementHeap()) NAString("NULL", CmpCommon::statementHeap()))
     , textIsValidatedSQLLiteralInUTF8_(FALSE)
     , isSystemSupplied_(FALSE)
     , locale_strval(0)
     , locale_wstrval(0)
     , isStrLitWithCharSetPrefix_(FALSE)
     , rebindNeeded_(FALSE)
{
}

// constructor for a numeric constant
ConstValue::ConstValue(Lng32 intval, NAMemory * outHeap)
           : ItemExpr(ITM_CONSTANT)
           , isNull_(IS_NOT_NULL)
           , textIsValidatedSQLLiteralInUTF8_(FALSE)
           , type_(new (CmpCommon::statementHeap()) SQLInt(CmpCommon::statementHeap(), TRUE, FALSE))
	   , isSystemSupplied_(FALSE)
           , locale_strval(0)
           , locale_wstrval(0)
           , isStrLitWithCharSetPrefix_(FALSE)
           , rebindNeeded_(FALSE)
{
  Int64 lintval = intval;
  char buf[TEXT_DISPLAY_LENGTH];
  char * S = buf;
  Int32 len = 0;
  sprintf(S,"%d %n",intval, &len);
  S[len++] = '\0';
  text_ = new (outHeap) NAString(S, outHeap);
  storageSize_ = sizeof(Lng32);
  value_ = (void *)( new (outHeap) char[storageSize_] );
  // copy the bit pattern as is
  memcpy(value_,(void *)(&intval),(Int32)storageSize_);
}

ConstValue::ConstValue(const NAString & strval,
             enum CharInfo::CharSet charSet,
             enum CharInfo::Collation collation,
             enum CharInfo::Coercibility coercibility,
             NAMemory * outHeap)
: ItemExpr(ITM_CONSTANT), isNull_(IS_NOT_NULL),
  textIsValidatedSQLLiteralInUTF8_(FALSE), isStrLitWithCharSetPrefix_(FALSE),
  isSystemSupplied_(FALSE), locale_wstrval(0), rebindNeeded_(FALSE)
{
   initCharConstValue(strval, charSet, collation, coercibility, FALSE, outHeap);
   locale_strval = new (outHeap) NAString
    ((char*)strval.data(), strval.length(), outHeap);
}

void ConstValue::initCharConstValue
(
     const NAString & strval,
     enum CharInfo::CharSet charSet,
     enum CharInfo::Collation collation,
     enum CharInfo::Coercibility coercibility,
     NABoolean isCaseInSensitive,
     NAMemory * outHeap
     )
{

  if (strval.length() == 0)
    {
      // create a varchar constant of length 0, in this case.
      type_ = new (outHeap)
		SQLVarChar(outHeap, 0, FALSE, FALSE, FALSE,
			   charSet, collation, coercibility);
      storageSize_ = type_->getVarLenHdrSize();
      value_ = (void *)( new (outHeap) 
			 char[storageSize_] );
      str_pad((char *)value_, (Int32)storageSize_, '\0');
    }
  else
    {

      // A "mini-cache" to avoid proc call, for performance,
      // local to the SINGLE-byte-charset NAString ctor
      // (distinct from the double-byte-charset ctor to prevent cache churn).
      static THREAD_P CharInfo::CharSet cachedCS  = CharInfo::UnknownCharSet;
      static THREAD_P Int32               cachedBPC = 1;
      if (cachedCS != charSet) {
        cachedCS = charSet;
	cachedBPC = CharInfo::maxBytesPerChar(charSet);
      }
      Int32 num_of_chars = (Int32)strval.length() / cachedBPC;
      if (CharInfo::isVariableWidthMultiByteCharSet(charSet))
      {
        Int32 actualCharsCount = ComputeStrLenInUCS4chars
          ( strval.data()             // const char * pStr
          , (Int32) strval.length()   // const Int32  strLenInBytes
          , charSet                   // const CharInfo::CharSet cs
          );
        CMPASSERT(actualCharsCount >= 0); // no errors
        type_ = new (outHeap) SQLChar (outHeap,  CharLenInfo ( actualCharsCount
                                                    , strval.length() // str len in bytes
                                                    )
                                      , FALSE           // allowSQLnull
                                      , FALSE           // isUpShifted
                                      , FALSE           // isCaseInsensitive
                                      , FALSE           // varLenFlag
                                      , charSet
                                      , collation
                                      , coercibility
                                      , /*encoding*/charSet
                                      );
      }
      else
      {
        if (charSet == CharInfo::BINARY)
          type_ = new (outHeap)
            SQLBinaryString(outHeap, strval.length(), FALSE, FALSE);
        else
          type_ = new (outHeap)
            SQLChar(outHeap, num_of_chars, FALSE, FALSE, FALSE, FALSE,
                    charSet, collation, coercibility);
      }

      storageSize_ = strval.length();
      value_ = (void *)( new (outHeap)
			 char[storageSize_] );
      memcpy(value_, (void *)(strval.data()), (Int32)storageSize_);
    }

  ((CharType*)type_)->setCaseinsensitive(isCaseInSensitive);

  text_ = new (outHeap)
    NAString(strval,
	     outHeap);
  textIsValidatedSQLLiteralInUTF8_ = FALSE;
}

ConstValue::ConstValue(const NAWString& wstrval,
             enum CharInfo::CharSet charSet,
             enum CharInfo::Collation collation,
             enum CharInfo::Coercibility coercibility,
             NAMemory * outHeap,
             enum CharInfo::CharSet strLitPrefixCharSet
)
 : ItemExpr(ITM_CONSTANT), isNull_(IS_NOT_NULL),
   isSystemSupplied_(FALSE),
   value_(0),
   text_(0),
   locale_strval(0),
   locale_wstrval(0),
   isStrLitWithCharSetPrefix_(FALSE),
   rebindNeeded_(FALSE)
{
   initCharConstValue(wstrval, charSet, collation, coercibility, FALSE, outHeap,
                      strLitPrefixCharSet);

   locale_wstrval = new (CmpCommon::statementHeap()) NAWString
      (wstrval.data(), wstrval.length(), CmpCommon::statementHeap());
}

void ConstValue::initCharConstValue(const NAWString& strval,
             enum CharInfo::CharSet charSet,
             enum CharInfo::Collation collation,
             enum CharInfo::Coercibility coercibility,
             NABoolean isCaseInSensitive,
             NAMemory * outHeap,
             enum CharInfo::CharSet strLitPrefixCharSet)
{
  if (strval.length() == 0)
    {
      // create a varchar constant of length 0, in this case.
      type_ = new (outHeap)
		SQLVarChar(outHeap, 0, FALSE, FALSE, FALSE,
			   charSet, collation, coercibility);
      storageSize_ = type_->getVarLenHdrSize();
      value_ = (void *)( new (outHeap) 
			 NAWchar[storageSize_] );
      wc_str_pad((NAWchar *)value_, (Int32)storageSize_, '\0');
    }
  else
    {
      // A "mini-cache" to avoid proc call, for performance,
      // local to the DOUBLE-byte-charset NAString ctor
      // (distinct from the single-byte-charset ctor to prevent cache churn).
      static CharInfo::CharSet cachedCS  = CharInfo::UNICODE;
      static Int32               cachedBPC = sizeof(NAWchar);

      if (cachedCS != charSet) {
        cachedCS = charSet;

        if (cachedCS == CharInfo::UnknownCharSet) {
          cachedCS = CharInfo::UNICODE;
          cachedBPC = sizeof(NAWchar);
        } else {
          cachedBPC = CharInfo::maxBytesPerChar(cachedCS);

          // make sure the incoming charset is double-byte
          CMPASSERT(cachedBPC == (Int32)sizeof(NAWchar));
        }
      }


      Int32 num_of_chars = (Int32)strval.length();

      type_ = new (outHeap)
		SQLChar(outHeap, num_of_chars, FALSE, FALSE, FALSE, FALSE,
			charSet, collation, coercibility);

      storageSize_ = cachedBPC * strval.length();
      value_ = (void *)( new (outHeap) 
			 NAWchar[storageSize_] );
      memcpy(value_, (void *)(strval.data()), (Int32)storageSize_);
    }

  ((CharType*)type_)->setCaseinsensitive(isCaseInSensitive);

  text_ = new (outHeap)
	    NAString((char*)strval.data(), storageSize_,
	     outHeap);
  textIsValidatedSQLLiteralInUTF8_ = FALSE;
}


ConstValue::ConstValue(NAString strval, NAWString wstrval,
             enum CharInfo::Collation collation,
             enum CharInfo::Coercibility coercibility,
             NAMemory * outHeap)
 : ItemExpr(ITM_CONSTANT), isNull_(IS_NOT_NULL),
   value_(0),
   text_(0),
   textIsValidatedSQLLiteralInUTF8_(FALSE),
   isSystemSupplied_(FALSE),
   isStrLitWithCharSetPrefix_(FALSE),
   rebindNeeded_(FALSE)
{
   initCharConstValue(strval, CharInfo::UnknownCharSet,
          collation, coercibility, FALSE, outHeap);

/*
   type_ =  new (CmpCommon::statementHeap())
       SQLVarChar(0, FALSE, FALSE,
                  CharInfo::UnknownCharSet, collation, coercibility);

   storageSize_ = type_->getVarLenHdrSize();
*/

   locale_strval = new (CmpCommon::statementHeap()) NAString
    ((char*)strval.data(), strval.length(), CmpCommon::statementHeap());

   locale_wstrval = new (CmpCommon::statementHeap()) NAWString
    (wstrval.data(), wstrval.length(), CmpCommon::statementHeap());
}


const NAType* ConstValue::pushDownType(NAType& newType,
				       enum NABuiltInTypeEnum defaultQualifier)
{
   if ( newType.getTypeQualifier() == NA_CHARACTER_TYPE &&
        type_ -> getTypeQualifier() == NA_CHARACTER_TYPE
      )
   {
     CharType &newCT = (CharType&)newType;
     enum CharInfo::CharSet newCS = ((const CharType&)newType).getCharSet();
     CharType* ct = (CharType*)type_;

     if ( ct ->getCharSet() != newCS ) {

       if ( newCS == CharInfo::UNICODE ) {
	 assert(locale_wstrval);
         // fix 10-070329-5979 by passing in the case-sensitivity flag.
         // Should re-implement this with collation some day.
	 initCharConstValue(*locale_wstrval, newCS,
			    ct->getCollation(),
			    ct->getCoercibility(),
                            ct->isCaseinsensitive());
       } else {

	 if ( locale_strval == NULL ) {

	   assert(locale_wstrval);

	   // init locale_strval from locale_wstrval
	   Lng32 wlen = (Lng32)(locale_wstrval->length());
	   Lng32 bufLen = wlen * CharInfo::maxBytesPerChar(newCS) + 1; // add the NULL
	   char* buf = new (CmpCommon::statementHeap()) char[bufLen];

	   Lng32 cLen = UnicodeStringToLocale(newCS, locale_wstrval->data(), wlen,
					     buf, bufLen, TRUE, FALSE);

	   if ( cLen == 0 ) {  // If conversion fails, do not change the type.
	     NADELETEBASIC(buf,CmpCommon::statementHeap());
	     return type_;
	   }

	   locale_strval = new (CmpCommon::statementHeap()) NAString
	     (buf, cLen, CmpCommon::statementHeap());

	   NADELETEBASIC(buf,CmpCommon::statementHeap());
	 }


	 // fix 10-040527-6468 (CQD infer_charset doesn't work with
	 // kanji character set correctly). We do not change this ConstValue
	 // object's attributes if the lendth of the literal is odd. The
	 // compatibility check code will catch the incompatible error (e.g.,
	 // T.KANJI_C1 = _unknown'abc').
	 if ( CharInfo::is_NCHAR_MP(newCS) == FALSE ||
	      locale_strval -> length() % SQL_DBCHAR_SIZE == 0
	      )
	   {
             // fix 10-070329-5979 by passing in the case-sensitivity flag.
             // Should re-implement this with collation some day.
	     initCharConstValue(*locale_strval, newCS,
				ct->getCollation(),
				ct->getCoercibility(),
                                ct->isCaseinsensitive());
	   }
       }
     }
     return synthesizeType();
   }

   return &newType;
}

// this constructor creates a constant of the given type and initializes
// it with the value.
ConstValue::ConstValue(const NAType * type, void * value, Lng32 value_len,
		       NAString * literal, NAMemory * outHeap)
  : ItemExpr(ITM_CONSTANT)
  , isNull_(IS_NOT_NULL)
  , type_(type)
  , isSystemSupplied_(FALSE)
  , locale_strval(0)
  , locale_wstrval(0)
  , isStrLitWithCharSetPrefix_(FALSE)
  , rebindNeeded_(FALSE)
{
  CMPASSERT(value_len > 0);
  CMPASSERT(type_);
  storageSize_ = value_len;
  value_ = (void *)( new (outHeap) char[storageSize_] );
  memcpy(value_,(void *)value,(Int32)storageSize_);

  if(type)
    type_ = type->newCopy(outHeap);

  NABoolean isNull = FALSE;

  // If a literal was not supplied, call a method to convert
  // the value from its binary form to UTF-8 and add any necessary
  // syntax modifiers like the charset and date and time qualifiers
  if (literal == NULL)
    {
      if (type_)
        textIsValidatedSQLLiteralInUTF8_ =
          type_->createSQLLiteral((const char *)value,
                                  text_,
                                  isNull,
                                  outHeap);
      DCMPASSERT(textIsValidatedSQLLiteralInUTF8_); // for now
      if (!textIsValidatedSQLLiteralInUTF8_)
        text_ = new (outHeap) NAString("<unknown value>", outHeap);
    }
  else
    {
      text_ = new (outHeap) NAString(*literal, outHeap);
      textIsValidatedSQLLiteralInUTF8_ = FALSE;
      if (type->supportsSQLnull())
        {
          // call the NAType base class method
          // just to check for a NULL value (sets isNull)
          NAString *dummyPtr;

          type->NAType::createSQLLiteral((const char *) value,
                                         dummyPtr,
                                         isNull,
                                         outHeap);
        }
    }

  if (isNull)
    isNull_ = IS_NULL;
}

/*soln:10-050710-9594 begin */

ConstValue::ConstValue(const NAType * type, void * value, Lng32 value_len,
		       NAString *lstrval, NAWString *wstrval, 
                     NAString * literal, NAMemory * outHeap, IsNullEnum isNull)
  : ItemExpr(ITM_CONSTANT)
  , isNull_(isNull)
  , type_(type)
  , isSystemSupplied_(FALSE)
  , locale_strval(0)
  , locale_wstrval(0)
  , isStrLitWithCharSetPrefix_(FALSE)
  , rebindNeeded_(FALSE)
{
  CMPASSERT(value_len > 0);
  CMPASSERT(type_);
  storageSize_ = value_len;
  value_ = (void *)( new (outHeap) char[storageSize_] );
  memcpy(value_,(void *)value,(Int32)storageSize_);

  if(type)
    type_ = type->newCopy(outHeap);

  // If a literal was not supplied, the text for this constant
  // is the bit pattern for the given value. Note that embedded
  // ascii nulls in the bit pattern are copied as is.
  if (literal == NULL)
    text_ = new (outHeap)
      NAString((char *)value,(UInt32)value_len, CmpCommon::statementHeap());
  else
    text_ = new (outHeap) NAString
      (*literal, outHeap);
  textIsValidatedSQLLiteralInUTF8_ = FALSE;

  if(lstrval)
     locale_strval = new (outHeap) NAString
      ((char*)lstrval->data(), lstrval->length(), outHeap);

  if(wstrval)
     locale_wstrval = new (outHeap) NAWString
      ((NAWchar *)wstrval->data(), wstrval->length(), outHeap);
}

/*soln:10-050710-9594 end */

// constructor for an extremal (min or max) value of a given type
// The allowNull parameter specifies whether NULL should be considered
// when generating the value; if it is TRUE and we want the max value,
// and the type is nullable, then NULL will be returned instead of the
// max non-null value.
ConstValue::ConstValue(const NAType * type,
		       const NABoolean wantMinValue,
		       const NABoolean includeNull,
                       NAMemory * outHeap)
: ItemExpr(ITM_CONSTANT)
, isNull_(IsNullEnum(type->supportsSQLnull() && includeNull && !wantMinValue))
, type_(type)
, isSystemSupplied_(FALSE)
, locale_strval(0)
, locale_wstrval(0)
, isStrLitWithCharSetPrefix_(FALSE)
, rebindNeeded_(FALSE)
{
  assert(type_);

  storageSize_ = type->getTotalSize();

  assert(storageSize_ > 0);

  char *storage = new (outHeap) char[storageSize_];
  value_ = (void *)storage;

  // if nullable, make the null indicator bytes 0 or NULL
  // depending on whether min or max value is needed.
  if (type->supportsSQLnull())
    {
      short indicatorVal;

      // the min value is non-NULL, the max value is the NULL
      // value unless we want the max non-NULL value
      if (wantMinValue || !includeNull)
	indicatorVal = 0;
      else
	indicatorVal = -1;

      // move the NULL indicator into the first few bytes of the value
      switch (type->getSQLnullHdrSize())
	{
	case 2:
	  *(short *)value_ = indicatorVal;
          break;
	case 4:
	  *(Lng32 *)value_ = (Lng32) indicatorVal;
          break;
	default:
	  CMPASSERT(0); // unsupported type of NULL indicator
	}
    }

  Lng32 startOfData = type->getSQLnullHdrSize();
  Lng32 templen = storageSize_ - startOfData;

  if (wantMinValue) // min value
  {
    type->minRepresentableValue(&storage[startOfData],
                                &templen,
                                NULL,
                                outHeap);
    text_ = new (outHeap) NAString("<min>", outHeap);
  }
  else
  {
    type->maxRepresentableValue(&storage[startOfData],
                                &templen,
                                NULL,
                                outHeap);
   text_ = new (outHeap) NAString("<max>", outHeap);
  }
  textIsValidatedSQLLiteralInUTF8_ = FALSE;
}

// A copy constructor used by the SampleSize derived class
//
ConstValue::ConstValue(OperatorTypeEnum otype,
                       ConstValue *other,
                       NAMemory * outHeap)
  : ItemExpr(otype)
//  , isNull_(other.isNull())
  , type_(other->getType())
  , storageSize_(other->getStorageSize())
  , textIsValidatedSQLLiteralInUTF8_(other->textIsValidatedSQLLiteralInUTF8_)
  , isSystemSupplied_(other->isSystemSupplied_)
  , locale_strval(other->locale_strval)
  , locale_wstrval(other->locale_wstrval)
  , isStrLitWithCharSetPrefix_(other->isStrLitWithCharSetPrefix_)
  , rebindNeeded_(other->rebindNeeded_)
{
  value_ = (void *)( new (outHeap) char[storageSize_] );
  memcpy(value_,(void *)other->getConstValue(),(Int32)storageSize_);
  text_ = new (outHeap)
               NAString(other->getText(), outHeap);
}

ConstValue::ConstValue(const ConstValue& s, NAHeap *h)
  : ItemExpr(ITM_CONSTANT), isNull_(s.isNull_), type_(s.type_)
  , storageSize_(s.storageSize_)
  , textIsValidatedSQLLiteralInUTF8_(s.textIsValidatedSQLLiteralInUTF8_)
  , isSystemSupplied_(s.isSystemSupplied_)
  , isStrLitWithCharSetPrefix_(s.isStrLitWithCharSetPrefix_)
  , rebindNeeded_(s.rebindNeeded_)
{
  if (s.value_) {
    value_ = (void *)( new (h) char[storageSize_] );
    memcpy(value_,(void *)s.value_,(Int32)storageSize_);
  }
  if (s.text_) {
    text_ = new (h) NAString(*s.text_, h);
  }
}

ConstValue::ConstValue(const ConstValue& s)
  : ItemExpr(ITM_CONSTANT), isNull_(s.isNull_), type_(s.type_)
  , storageSize_(s.storageSize_), value_(s.value_)
  , textIsValidatedSQLLiteralInUTF8_(s.textIsValidatedSQLLiteralInUTF8_)
  , text_(s.text_), isSystemSupplied_(s.isSystemSupplied_)
  , isStrLitWithCharSetPrefix_(s.isStrLitWithCharSetPrefix_)
  , rebindNeeded_(s.rebindNeeded_)
{
}

ConstValue::~ConstValue()
{
  // type_ can be shared by multiple consts --
  // see ConstValue::copyTopNode for a good example --
  // so we must not delete it!
     // delete type_;
  NADELETEBASIC((char*)value_,CmpCommon::statementHeap());   // value_ is a void*

  if (text_)
    NADELETEBASIC((NAString*)text_,CmpCommon::statementHeap());
}

NABoolean ConstValue::isAUserSuppliedInput() const    { return TRUE; }

void ConstValue::changeStringConstant(const NAString* strval)
{
   NADELETEBASIC((char*)value_,CmpCommon::statementHeap());   // value_ is a void*

   if (strval -> length() == 0)
   {
     // create a varchar constant of length 0, in this case.
     storageSize_ = type_->getVarLenHdrSize();
     value_ = (void *)( new (CmpCommon::statementHeap()) char[storageSize_] );
     str_pad((char *)value_, (Int32)storageSize_, '\0');
   } else {
     storageSize_ = strval -> length();
     value_ = (void *)( new (CmpCommon::statementHeap()) char[storageSize_] );
     memcpy(value_,(void *)(strval -> data()),(Int32)storageSize_);
   }

   *text_ = *strval;
}

NABoolean ConstValue::isAFalseConstant() const
{
  NABoolean result = FALSE;

  if (type_->getTypeQualifier() == NA_BOOLEAN_TYPE && !isNull())
    {
      CMPASSERT(storageSize_ == sizeof(Int32));
      if (*(reinterpret_cast<Int32 *>(value_)) == 0)
        result = TRUE; // that means the constant is FALSE!!
    }

  return result;          
}

NABoolean ConstValue::isExactNumeric() const
{
  return (type_->getTypeQualifier() == NA_NUMERIC_TYPE AND
	  ((NumericType *)type_)->isExact());
}

// exact numeric value can only be returned for certain types
// and if the value is within max largeint range.
NABoolean ConstValue::canGetExactNumericValue() const
{
  if (isExactNumeric())
    {
      NumericType &t = (NumericType &) *type_;

      // if unsigned largeint and value greater than largeint max,
      // cannot return exact numeric value.
      if ((t.getFSDatatype() == REC_BIN64_UNSIGNED) &&
          ((*(UInt64*)value_) > LLONG_MAX))
        return FALSE;

      // for now we can't do it for arbitrary exact numeric types, sorry
      if (NOT t.isDecimal() AND
	  NOT t.isComplexType() AND
	  NOT t.supportsSQLnull() AND
	  NOT t.getPrefixSize())
        return TRUE;
    }
  return FALSE;
}

Int64 ConstValue::getExactNumericValue(Lng32 &scale) const
{
  CMPASSERT(canGetExactNumericValue());

  Int64 result = 0;
  NumericType &t = (NumericType &) *type_;

  // for now we are looking at a binary representation of a number
  // that is either 2, 4, or 8 bytes.
  scale = t.getScale();
  CMPASSERT(t.getNominalSize() == storageSize_);
  switch (storageSize_)
    {
    case 1:
      if (t.isUnsigned())
	result = *((UInt8 *) value_);
      else
	result = *((Int8 *) value_);
      break;

    case 2:
      if (t.isUnsigned())
	result = *((unsigned short *) value_);
      else
	result = *((short *) value_);
      break;

    case 4:
      if (t.isUnsigned())
	result = uint32ToInt64(*((ULng32 *) value_));
      else
	result = *((Lng32 *) value_);
      break;

    case 8:
      if (t.isUnsigned())
        {
          if ((*(UInt64*)value_) > LLONG_MAX)
            {
              CMPASSERT(0);
            }
          else
            result = *(UInt64*)value_;
        }
      else
        result = *((Int64 *) value_);
      break;

    default:
      CMPASSERT(0);
      break;
    }

  return result;
}

NABoolean ConstValue::canGetApproximateNumericValue() const
{
  // return TRUE if this is an approximate numeric type or if we
  // can return an exact numeric value
  return (type_->getTypeQualifier() == NA_NUMERIC_TYPE &&
          ((!((NumericType *) type_)->isExact() ||
            canGetExactNumericValue())));
}

double ConstValue::getApproximateNumericValue() const
{
  CMPASSERT(canGetApproximateNumericValue());

  switch (type_->getFSDatatype())
    {
    case REC_IEEE_FLOAT32:
      return *((float *) value_);

    case REC_IEEE_FLOAT64:
      return *((double *) value_);

    default:
      {
        Int32 scale;
        double tempResult = (double) getExactNumericValue(scale);
        return tempResult * pow(10,-scale);
      }
    }
}

void ConstValue::getOffsetsInBuffer(int &nullIndOffset,
                                    int &vcLenOffset,
                                    int &dataOffset)
{
  int nextOffset = 0;

  // This uses the alignment method of the ConstVal, which is slightly
  // different from other formats, no alignment is used.
  // See also method ExpGenerator::placeConstants()
  if (type_->supportsSQLnull())
    {
      nullIndOffset = nextOffset;
      nextOffset += type_->getSQLnullHdrSize();
    }
  else
    nullIndOffset = -1;

  if (type_->isVaryingLen())
    {
      vcLenOffset = nextOffset;
      nextOffset += type_->getVarLenHdrSize();
    }
  else
    vcLenOffset = -1;

  dataOffset = nextOffset;
}

// The implementation checks for identity rather than equality,
// that is, the following method returns TRUE when the two
// ConstValues are identical in all respects except that they
// are implemented in two different storage locations.
NABoolean ConstValue::operator== (const ItemExpr& other) const	// virtual meth
{
  if (ItemExpr::operator==(other)) return TRUE;

  if (getOperatorType() != other.getOperatorType()) return FALSE;

  const ConstValue &otherConst = (const ConstValue &)other;

  // There is room for more creativity here, but it requires
  // the use of type conversion functions.

  if (type_ != otherConst.type_) return FALSE;

  if (storageSize_ != otherConst.storageSize_) return FALSE;

  // Perform a byte-by-byte comparison of the bit pattern for the value.
  if (memcmp(value_, otherConst.value_, (size_t)storageSize_)) return FALSE;

  return TRUE;
}

ConstValue * ConstValue::castToConstValue(NABoolean & negate_it)
{
  negate_it = FALSE;
  return this;
}

Int32 ConstValue::getArity() const { return 0; }

NAString ConstValue::getConstStr(NABoolean transformNeeded) const
{
  if ((*text_ == "<min>") || (*text_ == "<max>"))
    return *text_ ;

  if (getType()->getTypeQualifier() == NA_DATETIME_TYPE &&
      getType()->getPrecision() != SQLDTCODE_MPDATETIME)
  {
    return getType()->getSimpleTypeName() + " '" + *text_ + "'";
  }
  else if(getType()->getTypeQualifier() == NA_CHARACTER_TYPE)
  {
    CharType* chType = (CharType*)getType();
    NAString txt;

    if ( transformNeeded )
      txt = getText();
    else
      txt = getTextForQuery(QUERY_FORMAT);

    // if result doesn't have a charset specifier already, add one
    if (txt.index(SQLCHARSET_INTRODUCER_IN_LITERAL) == 0)
      return txt;
    else
      return chType->getCharSetAsPrefix() + txt;
  }
  else
  {
    return *text_;
  }
}

// Genesis 10-980402-1556 (see Binder)
ConstValue * ConstValue::toUpper(CollHeap *h)
{
  ConstValue *cv;

  if (isNull()) return this;                  // nothing to be done

  const CharType *typ = (const CharType *)getType();

  if (getType()->getTypeQualifier() != NA_CHARACTER_TYPE)
    return this;

  switch(typ->getCharSet())
    {
      case CharInfo::KSC5601_MP:
      case CharInfo::KANJI_MP:
        {
          // MP does not allow SQL upper/lower functions
          // to be applicable to KANJI/KSC5601 charsets.
          return this;
        }

      case CharInfo::UNICODE:
        {
          // upshift the text and put it into wcUpshift
          NAWString wcUpshift(h);
          unicode_char_set::to_upper((NAWchar *)value_,
                                     typ->getStrCharLimit(),
                                     wcUpshift);

          // create new ConstValue with the upshifted text
          cv = new (h) ConstValue(wcUpshift,
                                  typ->getCharSet(),
                                  typ->getCollation(),
                                  typ->getCoercibility());
	  cv->changeType(typ);
          break;
        }

      default:
        {
          const unsigned char *c = (const unsigned char*)(text_->data());

          // check if it's already upshifted
          for (; *c; c++)
            if (islower(*c)) break;
          if (!*c) return this;

          // upshift the text and return a new constant
          NAString upshift(*text_);
          upshift.toUpper();
          cv = new (h) ConstValue(upshift,
                                  typ->getCharSet(),
                                  typ->getCollation(),
                                  typ->getCoercibility());
          cv->changeType(typ);
          break;
        }
    }

  CMPASSERT(*typ == *cv->getType());
  return cv;
}

const NAString ConstValue::getText4CacheKey() const
{
  if(getType()->getTypeQualifier() != NA_CHARACTER_TYPE) {
    return getText();
  }
  else {
    NAString result = getText();

    // we want to return _charset'strfoo' instead of 'strfoo'.
    // This is to fix genesis case 10-040616-0347 "NF: query cache does not
    // work properly on || for certain character set". The root cause here
    // was ItemExpr::generateCacheKey() incorrectly returning the same string
    // keys for both
    //   select ?p1 || _ksc5601'arg' from ...
    //   select ?p1 || _kanji'arg from ...
    // The solution is to make them different via charset prefixes.
    if (result.index(SQLCHARSET_INTRODUCER_IN_LITERAL) != 0)
      result.prepend(((CharType*)getType())->getCharSetAsPrefix());

    return result;
  }
}

const NAString ConstValue::getTextForQuery() const
{
  if(getType()->getTypeQualifier() == NA_CHARACTER_TYPE)
    {
      NAString result("\'", CmpCommon::statementHeap());
      if (text_) result += *text_;
      RemoveTrailingZeros(result);

      result += "\'";
      return result;
    }
  else
    return *text_;
}

const NAString ConstValue::getTextForQuery(UnparseFormatEnum form) const
{
  if(getType()->getTypeQualifier() != NA_CHARACTER_TYPE ||
     textIsValidatedSQLLiteralInUTF8_)
    return *text_;

  //
  // getType()->getTypeQualifier() == NA_CHARACTER_TYPE)
  //

  NABoolean charSetConvFailed = FALSE;
  switch (form)
  {
  case QUERY_FORMAT:
  case MV_SHOWDDL_FORMAT:
  case USER_FORMAT_DELUXE:
    {
      NAString result((NAMemory *)CmpCommon::statementHeap());
      if (text_)
      {
        switch (((CharType*)getType())->getCharSet())
        {
        case CharInfo::UCS2:
          {
            // Assume that *text_ contains Unicode UCS-2/UTF-16 encoding values.
            // For SeaQuest, query text should be in Unicode UTF-8 encoding format.
            NAString *utf8Str = unicodeToChar ( (const NAWchar *)text_->data() // source
                                              , text_->length/*in_bytes*/() / BYTES_PER_NAWCHAR
                                              , CharInfo::UTF8                 // target CS
                                              , CmpCommon::statementHeap()     // heap for target
                                              );
            if (utf8Str) // Translation was successful
            {
              result += *utf8Str;
              delete utf8Str;
              utf8Str = NULL;
            }
            else // Translation failed - Assume *text_ already in UTF-8 encoding
            {
              result += *text_;
              charSetConvFailed = TRUE;
            }
          }
          break;
        case CharInfo::ISO88591:
          {
            NAString utf8String = Latin1StrToUTF8 ( *text_, CmpCommon::statementHeap() );
            if (utf8String.isNull()) // Translation failed -  Assume *text_ already in UTF-8 encoding
            {
              result += *text_;
              charSetConvFailed = TRUE;
            }
            else // Translation was successful
              result += utf8String;
          }
          break;
          // case CharInfo::SJIS:
          //   {
          //     NAString utf8Str2 = SjisStrToUTF8 ( *text_, CmpCommon::statementHeap() );
          //     if (utf8Str2.isNull()) // Translation failed -  Assume *text_ already in UTF-8 encoding
          //     {
          //       result += *text_;
          //       charSetConvFailed = TRUE;
          //     }
          //     else // Translation was successful
          //       result += utf8Str2;
          //   }
          //   break;
        default: // Assume *text_ already in UTF-8 encoding
          result += *text_;
          break;
        } // switch (((CharType*)getType())->getCharSet())

      } // if (text_)

      RemoveTrailingZeros(result);

      NAString extStrLit;
      if (result.isNull())
        extStrLit = "''";
      else if (charSetConvFailed)
      {
        extStrLit  = "'";
        extStrLit += result;
        extStrLit += "'";
      }
      else
        ToQuotedString(extStrLit/*out*/, result/*in - internalString*/, TRUE/*in - encloseInQuotes*/);

      return extStrLit;
    }
    break;

  default:
    return getTextForQuery();
    break;
  } // switch (((CharType*)getType())->getCharSet())

  return getTextForQuery(); // to keep the C++ compiler happy
}

const NAString ConstValue::getText() const
{
  if(getType()->getTypeQualifier() == NA_CHARACTER_TYPE)
    {
      NAString result = getTextForQuery(QUERY_FORMAT);

      // Change imbedded NULL and \377 chars to \0 and \377
      // This comes up in key values quite often.
      size_t index = 0;
      while((index = result.first('\0', index)) != NA_NPOS
	    && index != result.length())
	result(index,1) = "\\0";
      index = 0;
      while((index = result.first('\377', index)) != NA_NPOS
	    && index != result.length())
	result(index,1) = "\\377";

      return result;
    }
  else
    return *text_;
}

NABoolean ConstValue::isEmptyString() const
{
  if (getType()->getTypeQualifier() == NA_CHARACTER_TYPE) {
    if (text_ && text_->length() == 0) { return TRUE; }
  }
  return FALSE;
}

void ConstValue::unparse(NAString &result,
			 PhaseEnum /*phase*/,
			 UnparseFormatEnum form,
			 TableDesc * tabId) const
{
  if (form == EXPLAIN_FORMAT)
    {
      // for EXPLAIN, abbreviate very long text values:
      // drop any repeated text from the end, and drop any text that
      // is more than 30 characters

      NAString fullText(getText(), CmpCommon::statementHeap());
      const Int32 tooLong = 30;     // keep strings shorter than this
      const Int32 shortEnough = 10; // leave strings of this length alone

      // get the full text in query text format first
      fullText = getTextForQuery(QUERY_FORMAT);

      // then truncate it if too long
      if (fullText.length() > shortEnough)
	{
	  NABoolean truncated = FALSE;
	  // the last character often is a quote and should be preserved
	  char lastchar = fullText[fullText.length()-1];

	  // more than <tooLong> characters are unlikely to convey
	  // a lot of extra info
	  if (fullText.length() > tooLong)
	    {
	      fullText.remove(tooLong);
              // if there are unprintable characters in the string,
              // remove entire escape sequences
              if (fullText.last('\\') != NA_NPOS)
                fullText.remove(fullText.last('\\'));
              truncated = TRUE;
	    }

	  // try to reduce the text even further if the information is
	  // repetitive at the end of the string
	  CollIndex len = fullText.length();

	  // skip the last character unless we have already removed it
	  if (NOT truncated)
	    len--;

          // remove repeating patterns of length 1, 2, or 4 from the end
          // of the string (patterns could be ' ', '\0', '377', etc.)
	  while (len >= 8 AND fullText(len-4,4) == fullText(len-8,4))
	    len -= 4;
	  while (len >= 4 AND fullText(len-2,2) == fullText(len-4,2))
	    len -= 2;
	  while (len >= 2 AND fullText(len-1,1) == fullText(len-2,1))
	    len -= 1;

	  // remove all the repetitive information except 3 characters
	  // but only if it's worth it (we have to add 4 extra characters
	  // to indicate the omission and want to leave 3 repeating chars...)
	  if (len+7 < fullText.length())
	    {
	      fullText.remove(len+3);
	      truncated = TRUE;
	    }

	  if (truncated)
	    {
	      fullText += " ...";
	      fullText += lastchar;
	    }
	}
      result += fullText;
    }
  else if ((form == MVINFO_FORMAT) || (form == QUERY_FORMAT) ||
           (form == MV_SHOWDDL_FORMAT) || (form == COMPUTED_COLUMN_FORMAT))
    {
    if (getType()->getTypeQualifier() == NA_CHARACTER_TYPE)
      {
        CharType* chType = (CharType*)getType();

        if (form == MVINFO_FORMAT)
          result += getText();
        else
          {
            if (!textIsValidatedSQLLiteralInUTF8_)
              result += chType->getCharSetAsPrefix();

            if ((form == MV_SHOWDDL_FORMAT) || (form == QUERY_FORMAT))
              result += getTextForQuery(form);
            else
              result += getTextForQuery();
          }
      }
    else
      {
        if (getType()->getTypeQualifier() == NA_DATETIME_TYPE &&
            !textIsValidatedSQLLiteralInUTF8_)
        {
          if (getType()->getTypeName() == "DATE")
              result += " DATE '";
          else if (getType()->getTypeName() == "TIME")
              result += " TIME '";
          else if (getType()->getTypeName() == "TIMESTAMP")
              result += " TIMESTAMP '";
        }
        result += getText();
        if (getType()->getTypeQualifier() == NA_DATETIME_TYPE &&
            !textIsValidatedSQLLiteralInUTF8_)
          result += "'";
      }
    } // MVINFO_FORMAT || QUERY_FORMAT || MV_SHOWDDL_FORMAT || COMPUTED_COLUMN_FORMAT
  else if ((form == USER_FORMAT_DELUXE) ||
	   (form == HIVE_MD_FORMAT))
    {
      if (getType()->getTypeQualifier() == NA_CHARACTER_TYPE)
        {
          // Intentionally not adding the string literal character set prefix
          // to avoid updating the regression test expected results.
          result += getTextForQuery(form);
        }
      else
        result += getText();
    }
  else
    {
      // in all other cases use the getText() method
      result += getText();
    }
}

const NAString * ConstValue::getRawText() const
{
  return text_;
}

HashValue ConstValue::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= isNull();

  if (NOT isNull())
    {
      // hash binary representation of value, but no more than 10
      // bytes (diminishing return for hashing very long strings)
      Int32 maxc = MINOF(storageSize_, 10);
      for (Int32 i=0; i < maxc; i++)
	result ^= ((char *)value_)[i];
    }

  return result;
}

NABoolean ConstValue::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  ConstValue &o = (ConstValue &) other;

  // compare any local data members of the derived class
  // and return FALSE if they don't match
  if (NOT (*type_ == *(o.type_)))
    return FALSE;

  if (isNull() != o.isNull())
    return FALSE;

  if (isNull())
    return TRUE;

  if (storageSize_ != o.storageSize_)
    return FALSE;

  if (str_cmp((char *) value_, (char *)o.value_, storageSize_) != 0 OR
      isSystemSupplied_ != o.isSystemSupplied_)
    return FALSE;

  return TRUE;
}

ItemExpr * ConstValue::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ConstValue *result;

  if (derivedNode == NULL)
    {
      // According to the constructor that initiates isNull_ to NULL the value_
      // void pointer is set to NULL, however there are places in the code
      // like SearchKeyBounds::::computeMissingKeyValue() that creates a 
      // ConstValue with isNull set to IS_NULL and Value set to NULL
      // indicator.  So it is not right to assume that when isNULL is set
      // value will be empty.  So creating an empty ConstValue only when
      // isNULL() is true and value_ not set, otherwise we go to else
      // and call a constructor that copies isNULL_ flag along with value_
      if (isNull() && !value_)
        {
          //CMPASSERT(!value_);
          result = new (outHeap) ConstValue; // a NULL
        }
      else
        {
          /* soln:10-050710-9594 begin */
          /* Changed the constructor that is being called so that value
           * of locale_strval and locale_wstrval are not lost
           */
          result = new (outHeap) ConstValue(type_,value_,storageSize_,
                                            locale_strval, locale_wstrval,
                                            text_,outHeap, isNull_);
          
          /* soln:10-050710-9594 end */

          result->textIsValidatedSQLLiteralInUTF8_ =
            textIsValidatedSQLLiteralInUTF8_;
          result->setRebindNeeded(isRebindNeeded());
        }
    }
  else
    result = (ConstValue *) derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

NABoolean ConstValue::isCovered(const ValueIdSet& /* newExternalInputs */,
				const GroupAttributes& /* coveringGA */,
				ValueIdSet& /* referencedInputs */,
				ValueIdSet& /* coveredSubExpr */,
				ValueIdSet& /*unCoveredExpr*/) const
{
  // ---------------------------------------------------------------------
  // Constants are always covered
  // ---------------------------------------------------------------------
  return TRUE;
} // ConstValue::isCovered()

// ------------------------------------------------------------------------
// computeHashValues creates a hash value for a constant.
// Hash value is created only for character and numeric column types
// ------------------------------------------------------------------------
UInt32 ConstValue::computeHashValue(const NAType& columnType) 
{
  UInt32 hashValue = 0;
  switch ( getType()->getTypeQualifier() )
   {
     case NA_CHARACTER_TYPE:
       {
         UInt32 flags = ExHDPHash::NO_FLAGS;
         const CharType *cType = (CharType *)(&columnType);

         if(cType->getCharSet() == CharInfo::UNICODE) {
           flags = ExHDPHash::SWAP_TWO;
         }
     // For a case-insensitive char type, it is necessary to upper-case the 
     // boundary value first before compute the hash. This is to mimic the
     // run-time behavior where values of such char type are always 
     // upper shifted before case-insensitive comparison, being hashed or
     // encoded. Upshifting has already been done during encoding step.
         hashValue = ExHDPHash::hash((char*)(getConstValue()), flags, getStorageSize());
       }
       break;

     case NA_NUMERIC_TYPE :

       // only handle non-big NUMERICs that require 8-or-less bytes to store
       // for now.
       if ( getType()->getTypeName() == LiteralNumeric AND
            columnType.getTypeName() == LiteralNumeric AND
            columnType.getNominalSize() <= 8
          ) 
       {

         // 10-080124-0055 (Skewbuster numeric uneven distribution 
         // different scales).
         // Convert the constant to the exact NUMERIC type of the column
         // before applying the hash on it. This is necessary because for
         // example 4 bytes is sufficient to represent a small scale boundary 
         // value such as 1.0, while the storage size of the column can be 
         // 8-bytes. Without converting the number of bytes of the value to 
         // that of the column, the hashing result can be different.
         // 
         // There is still a chance that the skews are not busted when the
         // fact table side has a scale smaller than that of the dimension
         // table. For example numeric(18,2) on fact side and 
         // numeric(18, 4) on dimension side. In this case, the hash values 
         // computed for the fact table side will not match the actual 
         // values used for repartition or hashing because a CAST will be 
         // used on the fact side and repartition and hashing will use the
         // output from the CAST.

         const SQLNumeric& type = (const SQLNumeric&)(columnType);

         Lng32 len = type.getNominalSize();

         char result[8];

         // Ignore errors because the boundary value pointed by cvPtr
         // is read from the histogram table and should be have been cleared
         // off any precision or scale problems by the update histogram
         // utility.
         short retCode = convDoIt((char*)(getConstValue()),
                  getStorageSize(),
                  (short)getType()->getFSDatatype(), 
                  getType()->getPrecision(),
                  getType()->getScale(), 
                  result,
                  len, 
                  (short)(type.getFSDatatype()), 
                  type.getPrecision(), 
                  type.getScale(), 
                  NULL /* null varchar ptr*/, 0 /* varchar len*/);

         DCMPASSERT(retCode == ex_expr::EXPR_OK);

         UInt32 flags = ExHDPHash::NO_FLAGS;

         switch(type.getFSDatatype()) {
         case REC_NUM_BIG_UNSIGNED:
         case REC_NUM_BIG_SIGNED:
         case REC_BIN16_SIGNED:
         case REC_BIN16_UNSIGNED:
           flags = ExHDPHash::SWAP_TWO;
           break; 
         case REC_BIN32_SIGNED:
         case REC_BIN32_UNSIGNED:
         case REC_IEEE_FLOAT32:
           flags = ExHDPHash::SWAP_FOUR;
           break;
         case REC_BIN64_SIGNED:
         case REC_BIN64_UNSIGNED:
         case REC_IEEE_FLOAT64:
           flags = ExHDPHash::SWAP_EIGHT;
           break;
         }

         hashValue = ExHDPHash::hash(result, flags, len);
       }
       break;
 
     default:
       break;
   }
  return hashValue;
}

// -----------------------------------------------------------------------
// does the value of this constant (if char) has trailing blanks
// -----------------------------------------------------------------------
NABoolean ConstValue::valueHasTrailingBlanks()
{
   NABoolean hasATrailingBlanks = FALSE;

   if (value_ && (getType()->getTypeQualifier() == NA_CHARACTER_TYPE))
   {
      const CharType *typ = (const CharType *)getType();
      if (typ->getCharSet() == CharInfo::UNICODE)
      {
          NAWchar* nawCharVal = (NAWchar *) value_;
          Int32 bytesPerChar = (CharInfo::maxBytesPerChar)(typ->getCharSet());
          Int32 lastPos = (storageSize_/bytesPerChar)-1;
          if ((lastPos >= 0) && (nawCharVal[lastPos] == L' '))
          { 
             hasATrailingBlanks = TRUE;
          }
      }
      else
      {
          char* charVal = (char *) value_;
          Int32 lastPos = storageSize_-1;
          if ((lastPos >= 0) && (charVal[lastPos] == ' '))
          {
             hasATrailingBlanks = TRUE;
          }
          
      }
   }
   return hasATrailingBlanks;
}

QR::ExprElement ConstValue::getQRExprElem() const
{
  return QR::QRScalarValueElem;
}

// -----------------------------------------------------------------------
// member functions for class RenameCol
// -----------------------------------------------------------------------
Int32 RenameCol::getArity() const { return (child(0) ? 1 : 0); }

const NAString RenameCol::getText() const
{
  return ("rename as " + newColRefName_->getColName());
}

HashValue RenameCol::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class
  result ^= newColRefName_->getColRefAsString();

  return result;
}

NABoolean RenameCol::duplicateMatch(const ItemExpr & other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  RenameCol &o = (RenameCol &) other;

  // compare any local data members of the derived class
  // and return FALSE if they don't match
  if (NOT (*newColRefName_ == *(o.newColRefName_)))
    return FALSE;

  return TRUE;
}

ItemExpr * RenameCol::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
  {
    RenameCol *newRename = new (outHeap)
      RenameCol(NULL,
		new (outHeap)
		ColRefName(newColRefName_->getColName(),
			   newColRefName_->getCorrNameObj(),
                           outHeap));
    newRename->setTargetColumnClass(getTargetColumnClass());
    result = newRename;
  }
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class Convert
// -----------------------------------------------------------------------
Int32 Convert::getArity() const { return 1; }

HashValue Convert::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class

  return result;
}

ItemExpr * Convert::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  Convert *result;

  if (derivedNode == NULL)
    result = new (outHeap) Convert(NULL);
  else
    result = (Convert *) derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class Assign
// -----------------------------------------------------------------------
Int32 Assign::getArity() const  { return 2; }

NABoolean Assign::isCovered(const ValueIdSet& newExternalInputs,
			    const GroupAttributes& coveringGA,
			    ValueIdSet& referencedInputs,
			    ValueIdSet& coveredSubExpr,
			    ValueIdSet& unCoveredExpr ) const
{
  // ---------------------------------------------------------------------
  // Compute the referenced input values, covered subexpressions and
  // uncovered expressions for the Assign.
  // ---------------------------------------------------------------------
  // The target is never covered
  if (coveringGA.covers(getSource(), newExternalInputs,
			referencedInputs,
			&coveredSubExpr, &unCoveredExpr))
    coveredSubExpr += getSource();

  // ---------------------------------------------------------------------
  // Regardless of whether its operands are covered, the Assignment
  // must be performed by the RelExpr with which it is associated.
  // ---------------------------------------------------------------------
  return FALSE;  // sorry, i am a special case
} // Assign::isCovered()

ItemExpr * Assign::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  Assign *result;

  if (derivedNode == NULL)
    result = new (outHeap) Assign(NULL, NULL);
  else
    result = (Assign *) derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

HashValue Assign::topHash()
{
  HashValue result = ItemExpr::topHash();

  // hash any local data members of the derived class

  return result;
}

// -----------------------------------------------------------------------
// member functions for class ItemList
// -----------------------------------------------------------------------
Int32 ItemList::getArity() const { return 2; }

NABoolean ItemList::isCovered
                      (const ValueIdSet& newExternalInputs,
		       const GroupAttributes& coveringGA,
		       ValueIdSet& referencedInputs,
		       ValueIdSet& coveredSubExpr,
		       ValueIdSet& unCoveredExpr) const
{
  ValueId exprId;
  NABoolean exprIsCovered = TRUE;

  NABoolean coverFlag;

  for (Lng32 i = 0; i < (Lng32)getArity(); i++)
    {
      ItemExpr *thisChild = (ItemExpr *)child(i);
      ItemExprList childList(thisChild,0);
      for (CollIndex j = 0; j < childList.entries(); j++)
	{
	  // Recurse, inserting leaf valueids into the cov/uncov sets
	  // without looking at or setting the local exprIsCovered var
	  exprId = childList[j]->getValueId();
	  coverFlag = coveringGA.covers(exprId,
					newExternalInputs,
					referencedInputs,
					&coveredSubExpr,
					&unCoveredExpr);
	  if (coverFlag)
	    coveredSubExpr += exprId;

	  exprIsCovered &= coverFlag;

	} // for j'th item in list

    } // for i'th child

  return exprIsCovered;

} // ItemList::isCovered()

ItemExpr * ItemList::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemList *result;

  if (derivedNode == NULL)
    result = new (outHeap) ItemList(NULL,NULL);
  else
    result = (ItemList *) derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

NABoolean ItemList::duplicateMatch(const ItemExpr& other) const
{
  if (NOT genericDuplicateMatch(other))
    return FALSE;

  return TRUE;
}

NABoolean ItemList::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  ItemList * otherList = (ItemList *) other;

  return 
      (this->numOfItems() == otherList->numOfItems()) && 
      (this->constChild() == otherList->constChild()) ;
}
// -----------------------------------------------------------------------
// member functions for Subquery operators
// -----------------------------------------------------------------------
Int32 Subquery::getArity() const
{
  switch (getOperatorType())
    {
    case ITM_ROW_SUBQUERY:
    case ITM_EXISTS:
    case ITM_NOT_EXISTS:
      return 0;
    default:
      return 1;
    }
} // Subquery::getArity()

// Virtual method
NABoolean Subquery::isASubquery() const { return TRUE; }

NABoolean Subquery::isAnAnySubquery() const
{
  switch (getOperatorType())
    {
    case ITM_EQUAL_ANY:
    case ITM_NOT_EQUAL_ANY:
    case ITM_GREATER_ANY:
    case ITM_GREATER_EQ_ANY:
    case ITM_LESS_EQ_ANY:
    case ITM_LESS_ANY:
      return TRUE;
    default:
      return FALSE;
    }

} // Subquery::isAnAnySubquery()

NABoolean Subquery::isAnAllSubquery() const
{
  switch (getOperatorType())
    {
    case ITM_EQUAL_ALL:
    case ITM_NOT_EQUAL_ALL:
    case ITM_GREATER_ALL:
    case ITM_GREATER_EQ_ALL:
    case ITM_LESS_EQ_ALL:
    case ITM_LESS_ALL:
      return TRUE;
    default:
      return FALSE;
    }

} // Subquery::isAnAnySubquery()

const NAString Subquery::getText() const
{
  return "unknown subquery";
} // Subquery::getText()

void Subquery::addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const
{
  xlist.insert(tableExpr_);
  llist.insert("Subquery");
}

NABoolean Subquery::containsSubquery()
{
  return TRUE;
} // Subquery::containsSubquery

// <aviv> BEGIN
ItemExpr *
Subquery::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;
  if (derivedNode == NULL) {
	RelExpr* tableExprCopy = tableExpr_->copyTree(outHeap);
    result = new (outHeap) Subquery(getOperatorType(), tableExprCopy);
  }
  else
    result = derivedNode;

  ((Subquery *)result)->setAvoidHalloweenR2(getAvoidHalloweenR2());
  return ItemExpr::copyTopNode(result, outHeap);
}

// output degree functions to handle multi-output subqueries.
// Returns the number of outputs and the ItemExpr for each.
Int32 Subquery::getOutputDegree()
{
  if (!getRETDesc()) return -1; // Routine desc not set.  Error.
  else 
    return getRETDesc()->getColumnList()->entries();
}

ItemExpr *Subquery::getOutputItem(UInt32 i) 
{ 
  if (!getRETDesc()) return NULL; // Routine desc not set.  Error.
  else 
  {
    if (i < getRETDesc()->getColumnList()->entries())
      return getRETDesc()->getValueId(i).getItemExpr();
    else return NULL; // 'i' out of bounds.  Error.
  }
}
// <aviv> END


// -----------------------------------------------------------------------
// member functions for row subquery operators
// -----------------------------------------------------------------------
const NAString RowSubquery::getText() const
{
  //return "row subquery";
  NAString tmp(CmpCommon::statementHeap());
  if (getSubquery()->child(0))
    getSubquery()->child(0)->unparse(tmp);	// unparse below the subq root
  else
    getSubquery()->unparse(tmp);		// unparse the tuple
  return tmp;
}

ItemExpr *
RowSubquery::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;
  if (derivedNode == NULL) {
	RelExpr* tableExprCopy = getSubquery()->copyTree(outHeap);
    result = new (outHeap) RowSubquery(tableExprCopy);
  }
  else
    result = derivedNode;

  return Subquery::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for Quantified comparison subquery operators
// -----------------------------------------------------------------------
const NAString QuantifiedComp::getText() const
{
  switch (getOperatorType())
    {
    case ITM_EQUAL_ANY:
      return "= any";
    case ITM_NOT_EQUAL_ANY:
      return "<> any";
    case ITM_GREATER_ANY:
      return "> any";
    case ITM_GREATER_EQ_ANY:
      return ">= any";
    case ITM_LESS_EQ_ANY:
      return "<= any";
    case ITM_LESS_ANY:
      return "< any";
    case ITM_EQUAL_ALL:
      return "= all";
    case ITM_NOT_EQUAL_ALL:
      return "<> all";
    case ITM_GREATER_ALL:
      return "> all";
    case ITM_GREATER_EQ_ALL:
      return ">= all";
    case ITM_LESS_EQ_ALL:
      return "<= all";
    case ITM_LESS_ALL:
      return "< all";
    default:
      return "? any|all ";
    }

} // QuantifiedComp::getText()

// Instead of this next function, we could add a wildcard to OperatorTypeEnum
// and augment the OperatorType::match() function in ExprNode.cpp...
static NABoolean isQuantifiedComp(const ItemExpr *ie)
{
  switch (ie->getOperatorType())
    {
    case ITM_EQUAL_ANY:
    case ITM_NOT_EQUAL_ANY:
    case ITM_GREATER_ANY:
    case ITM_GREATER_EQ_ANY:
    case ITM_LESS_EQ_ANY:
    case ITM_LESS_ANY:
    case ITM_EQUAL_ALL:
    case ITM_NOT_EQUAL_ALL:
    case ITM_GREATER_ALL:
    case ITM_GREATER_EQ_ALL:
    case ITM_LESS_EQ_ALL:
    case ITM_LESS_ALL:
      return TRUE;
    default:
      return FALSE;
    }

} // isQuantifiedComp()


// <aviv> BEGIN
ItemExpr *
QuantifiedComp::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  QuantifiedComp *result;
  if (derivedNode == NULL) {
    RelExpr* tableExprCopy = getSubquery()->copyTree(outHeap);

    // NOTICE: using child(0) here is based on the Subquery and ItemExpr constructors
    //         This is bad programming !!! A solution could be to change the order of
    //		   the parameters in this ctor and using defaults (NULL), yet it involves
    //		   changes in many other places in the code <aviv>
    result = new (outHeap) QuantifiedComp(getOperatorType(),
                                          (child(0))->copyTree(outHeap),
                                          tableExprCopy,
                                          getAvoidHalloweenR2());
  }
  else
    result = (QuantifiedComp*)derivedNode;

  result->createdFromINlist_ = createdFromINlist_;
  result->createdFromALLpred_ = createdFromALLpred_;

  return Subquery::copyTopNode(result, outHeap);
}
// <aviv> END


// -----------------------------------------------------------------------
// member functions for In subquery operators
// -----------------------------------------------------------------------
const NAString InSubquery::getText() const { return "in"; }

// <aviv> BEGIN
ItemExpr *
InSubquery::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;
  if (derivedNode == NULL) {
	RelExpr* tableExprCopy = getSubquery()->copyTree(outHeap);

	// NOTICE: using child(0) here is based on the Subquery and ItemExpr constructors
	//         This is bad programming !!! A solution could be to change the order of
	//		   the parameters in this ctor and using defaults (NULL), yet it involves
	//		   changes in many other places in the code <aviv>
    result = new (outHeap) InSubquery(getOperatorType(), (child(0))->copyTree(outHeap), tableExprCopy);
  }
  else
    result = derivedNode;

  return Subquery::copyTopNode(result, outHeap);
}
// <aviv> END


// --------------------------------------------------------------
// member functions for Abs operator
// --------------------------------------------------------------

ItemExpr * Abs::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Abs(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for CodeVal operator
// --------------------------------------------------------------

ItemExpr * CodeVal::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) CodeVal(getOperatorType(), child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

const NAString CodeVal::getText() const
{
  // to fix case 10-040504-4753  (Incorrect error text when using NULL as k
  // CODE_VALUE function parameter). The operator type for CodeVal is
  // NO_OPERATOR_TYPE until after the type is synthesized. getText() can be
  // called before that (e.g., in checkForSQLnullChild()).
  //
  // Use the general code_value name for such case.
  //
  if (getOperatorType() == NO_OPERATOR_TYPE )
     return "code_value";
  else
     return BuiltinFunction::getText();
}


// --------------------------------------------------------------
// member functions for AggrMinMax
// --------------------------------------------------------------

ItemExpr * AggrMinMax::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) AggrMinMax(NULL, NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for AggrGrouping function
// --------------------------------------------------------------
ItemExpr * AggrGrouping::copyTopNode(ItemExpr * derivedNode, CollHeap* outHeap)
{
  AggrGrouping *result;

  if (derivedNode == NULL)
    result = new (outHeap) AggrGrouping(-1);
  else
    result = (AggrGrouping*)derivedNode;

  result->rollupGroupIndex_ = rollupGroupIndex_;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for Exists subquery operators
// -----------------------------------------------------------------------

Exists::Exists(RelExpr  * tableExpr)
     : Subquery(ITM_EXISTS, tableExpr) {}

Exists::~Exists() {}

const NAString Exists::getText() const 
{
  if (isNotExists())
    return "not_exists";
  else
    return "exists";
}

// <aviv> BEGIN
ItemExpr *
Exists::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;
  if (derivedNode == NULL) {
	RelExpr* tableExprCopy = getSubquery()->copyTree(outHeap);
    result = new (outHeap) Exists(tableExprCopy);
    if (isNotExists())
      result->setOperatorType(getOperatorType());
  }
  else
    result = derivedNode;

  return Subquery::copyTopNode(result, outHeap);
}
// <aviv> END


// --------------------------------------------------------------
// member functions for Overlaps operator
// --------------------------------------------------------------
ItemExpr * Overlaps::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;
  if (derivedNode == NULL)
    result = new (outHeap) Overlaps(child(0), child(1), child(2), child(3));
  else
    result - derivedNode;

  return CacheableBuiltinFunction::copyTopNode(result, outHeap);
}

void Overlaps::unparse(NAString &result
		                   , PhaseEnum phase
                       , UnparseFormatEnum form
		                   , TableDesc * tabId) const
{
  result += "(";
  child(0)->unparse(result,phase,form,tabId);
  result += ", ";
  child(1)->unparse(result,phase,form,tabId);
  result += ") ";

  NAString kwd(getText(), CmpCommon::statementHeap());
  if (form == USER_FORMAT_DELUXE) kwd.toUpper();
  result += kwd;

  result += " (";
  child(2)->unparse(result,phase,form,tabId);
  result += ", ";
  child(3)->unparse(result,phase,form,tabId);
  result += ")";
}


// --------------------------------------------------------------
// member functions for Between operator
// --------------------------------------------------------------
ItemExpr * Between::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
  {
    result = new (outHeap) Between(NULL,NULL,NULL);
    ((Between*)result)->leftBoundryIncluded_ = leftBoundryIncluded_;
    ((Between*)result)->rightBoundryIncluded_ = rightBoundryIncluded_;

    // NOTICE: we are copying the pointer, not the allocating new memory
    // and copying the object. because of this the vector must never be
    // destroyed in any destructor
    ((Between*)result)->pDirectionVector_ = pDirectionVector_;

  }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

void Between::unparse(NAString &result,
		      PhaseEnum phase,
                      UnparseFormatEnum form,
		      TableDesc * tabId) const
{
  child(0)->unparse(result,phase,form,tabId);

  result += " ";
  NAString kwd(getText(), CmpCommon::statementHeap());
  if (form == USER_FORMAT_DELUXE) kwd.toUpper();
  result += kwd;
  result += " ";

  child(1)->unparse(result,phase,form,tabId);

  kwd = " and ";
  if (form == USER_FORMAT_DELUXE) kwd.toUpper();
  result += kwd;

  child(2)->unparse(result,phase,form,tabId);
}

NABoolean Between::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  Between * tmp = (Between *) other;

  return 
    (this->leftBoundryIncluded_ == tmp->leftBoundryIncluded_) &&
    (this->rightBoundryIncluded_ == tmp->rightBoundryIncluded_ ) &&
    (this->pDirectionVector_ == tmp->pDirectionVector_);

}
// --------------------------------------------------------------
// member functions for BoolResult operator
// --------------------------------------------------------------

BoolResult::~BoolResult() {}

ItemExpr * BoolResult::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) BoolResult(NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for BoolVal operator
// --------------------------------------------------------------
ItemExpr * BoolVal::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    {
      if (getArity() == 0)
	result = new (outHeap) BoolVal(getOperatorType());
      else
	result = new (outHeap) BoolVal(getOperatorType(), NULL);
    }
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for Case operator
// --------------------------------------------------------------
ItemExpr * Case::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Case(NULL, NULL);
  else
    result = derivedNode;

  //++MV bug fix
  if (caseOperand_)
	((Case*) result)->caseOperand_ = caseOperand_->copyTree(outHeap);

  ((Case*) result)->caseOperandWasNullable_ = caseOperandWasNullable_;
  //--MV

  return BuiltinFunction::copyTopNode(result, outHeap);
}


// This method recursively removes from coveredSubExpr,
// all IfThenElse nodes that are part of 'expr' and for those IfThenElse
// nodes, adds its children to coveredSubExpr.
// input: expr, a non-null IfThenElse node.
// Added another parameter boolean ifThenEsleExists, which would be passed
// from the parent ifThenElse to its children. This will ensure that if there
// is more than two levels of if_then_else nesting, then even the inner
// if then else is correctly fixed. Sol: 10-041025-1038
void Case::fixIfThenElse(ItemExpr * expr, ValueIdSet& coveredSubExpr, NABoolean ifThenElseExists) const
{
  if ((! expr) ||
      (expr->getOperatorType() != ITM_IF_THEN_ELSE))
    return;

  // remove expr from coveredSubExpr, if it is there
  if (coveredSubExpr.contains(expr->getValueId()))
    {
      ifThenElseExists = TRUE;
      coveredSubExpr.remove(expr->getValueId());
    }

  // and add all its children to it
  for (Lng32 index = 0; index < expr->getArity(); index++)
    {
      if (expr->child(index))
	{
	  if (expr->child(index)->getOperatorType() == ITM_IF_THEN_ELSE)
	    fixIfThenElse(expr->child(index), coveredSubExpr, ifThenElseExists);
	  else if (ifThenElseExists)
	    coveredSubExpr.insert(expr->child(index)->getValueId());
	}
    }
}

NABoolean Case::isCovered
                  (const ValueIdSet& newExternalInputs,
		   const GroupAttributes& coveringGA,
		   ValueIdSet& referencedInputs,
		   ValueIdSet& coveredSubExpr,
		   ValueIdSet& unCoveredExpr ) const
{
  ValueIdSet coveredSubExprForCase ;
  NABoolean retVal =
  BuiltinFunction::isCovered(newExternalInputs, coveringGA,
			       referencedInputs, coveredSubExprForCase,
			       unCoveredExpr);

  // The case expression is covered as a unit. Parts of a case 
  // expression must not be pushed down independently of each other
  // as this cause divide by zero type errors (if the case expression
  // has different branches for the zero and non-zero cases, see soln.
  // 10-081107-7119). We do not column references and leaf values
  // to be counted as covered if they are. We use the getLeafValuesForCoverTest
  // for this purpose. Note that Case::getLeafValuesForCoverTest() returns 
  // itself.
  const ValueIdSet emptySet;
  //coveredSubExprForCase.getLeafValuesForCoverTest(coveredSubExpr, coveringGA, emptySet);
  coveredSubExprForCase.getLeafValuesForCoverTest(coveredSubExpr, coveringGA, newExternalInputs);

  return retVal;

}

void Case::unparse(NAString &result,
		       PhaseEnum phase,
		       UnparseFormatEnum form,
		       TableDesc * tabId) const
{
  // Is this for MV use?
  if ((form != MV_SHOWDDL_FORMAT) &&
      (form != QUERY_FORMAT) &&
      (form != COMPUTED_COLUMN_FORMAT) )
  {
    // No - sorry, use the default code.
    ItemExpr::unparse(result,phase, form, tabId);
  }
  else
  {
    // Yes - avoid the parenthasis:
    // Instead of "case (when....)"
    // Do "case when ...
    result += "case";
    child(0)->unparse(result, phase, form, tabId);
  }
}

void Case::getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                     const GroupAttributes& coveringGA,
                                     const ValueIdSet & newExternalInputs) const
{
  leafValues += getValueId();
} // Case::getLeafValuesForCoverTest()

NABoolean Case::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  Case * otherCase = (Case *) other;

  return  
      (this->getCaseOperand()== otherCase->getCaseOperand()) && 
      (this->caseOperandWasNullable() == otherCase->caseOperandWasNullable());
}
// --------------------------------------------------------------
// member functions for Cast operator
// --------------------------------------------------------------

Cast::Cast(ItemExpr *val1Ptr, const NAType *type, OperatorTypeEnum otype,
	   NABoolean checkForTrunc, NABoolean noStringTrunWarnings)
  :
  CacheableBuiltinFunction(otype, 1, val1Ptr), type_(type),
  reverseDataErrorConversionFlag_(FALSE),
  flags_(0)
{
  ValueId vid = val1Ptr ? val1Ptr->getValueId() : NULL_VALUE_ID;

  checkForTruncation_ = FALSE;
  if (checkForTrunc)
    if (vid == NULL_VALUE_ID)
      checkForTruncation_ = TRUE;
    else if ( type && type->getTypeQualifier()         == NA_CHARACTER_TYPE &&
              vid.getType().getTypeQualifier() == NA_CHARACTER_TYPE )
    {
       if ( type->getNominalSize() < vid.getType().getNominalSize() )
        checkForTruncation_ = TRUE;
       else
       { // If UTF8 and *any* chance of truncation error
         // Ex: casting CHAR(8 BYTES) to CHAR(3) 
          if ( ((CharType *)type)->getCharSet() == CharInfo::UTF8  &&
               ((CharType *)type)->getStrCharLimit() < vid.getType().getNominalSize() )
          checkForTruncation_ = TRUE;
       }
    }
             
  setNoStringTruncationWarnings(noStringTrunWarnings);
}

Cast::Cast(ItemExpr *val1Ptr, ItemExpr *errorOutPtr, const NAType *type,
	   OperatorTypeEnum otype, NABoolean checkForTrunc,
           NABoolean reverseDataErrorConversionFlag,
           NABoolean noStringTrunWarnings)
  :
  CacheableBuiltinFunction(otype, 2, val1Ptr, errorOutPtr), type_(type),
  reverseDataErrorConversionFlag_(reverseDataErrorConversionFlag),
  flags_(0)
{
  checkForTruncation_ = checkForTrunc;
  setNoStringTruncationWarnings(noStringTrunWarnings);
}

Cast::~Cast() {}

ConstValue * Cast::castToConstValue(NABoolean & negate_it)
{
  return child(0)->castToConstValue(negate_it);
}

ItemExpr * Cast::foldConstants(ComDiagsArea * diagsArea,
			       NABoolean newTypeSynthesis)
{
  // Convert a cast on top of a constant into a new constant with
  // the correct type. The reason for implementing this special
  // case of constant folding is that the optimizer needs to generate
  // the actual binary representation of constants in some cases
  // (e.g. when generating the split ranges of a partitioning scheme).

  ItemExpr *result = this;

  if (child(0)->getOperatorType() == ITM_CONSTANT)
    {
      const NAType &t = child(0)->getValueId().getType();

      // simple case, the cast is unnecessary because the child
      // already has the correct type: eliminate this node
      if (t == *type_)
	return child(0);

      // try one more case, mapping of numeric data types:

    }

  return ItemExpr::foldConstants(diagsArea,newTypeSynthesis);
}

NABoolean Cast::isCovered
                   (const ValueIdSet& newExternalInputs,
		    const GroupAttributes& coveringGA,
		    ValueIdSet& referencedInputs,
		    ValueIdSet& coveredSubExpr,
		    ValueIdSet& unCoveredExpr) const
{
  NABoolean coverFlag = TRUE;
  if (coveringGA.covers(getExpr()->getValueId(),
			newExternalInputs, referencedInputs,
			&coveredSubExpr, &unCoveredExpr))
    coveredSubExpr += getExpr()->getValueId();
  else
    coverFlag = FALSE;

  if (getOperatorType() == ITM_INSTANTIATE_NULL)
    return FALSE; //
  else
    return coverFlag;
} // Cast::isCovered()

// MV -- 
// If this is a Cast over an NATypeToItem, return this ValueId instead
// of the child's. This fixes a problem with using the TupleList for the
// MVLog command.
void Cast::getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                     const GroupAttributes& coveringGA,
                                     const ValueIdSet & newExternalInputs) const
{
  if(ITM_NATYPE == child(0)->getOperatorType())
  {
    leafValues += getValueId();
  }
  else
  {
    // Otherwise - call the super class.
    BuiltinFunction::getLeafValuesForCoverTest(leafValues, coveringGA, newExternalInputs);
  }
}

ItemExpr * Cast::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    {
      result = new (outHeap) Cast(NULL, type_->newCopy(outHeap),
		this->getOperatorType(), this->checkForTruncation_);

      // Fix CR 10-010426-2464: Copy other members of the class as well
      ((Cast *)result)->reverseDataErrorConversionFlag_ =
		this->reverseDataErrorConversionFlag_;

      ((Cast *)result)->flags_ = this->flags_;
    }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

ItemExpr * CastConvert::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) CastConvert(NULL, getType()->newCopy(outHeap));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

ItemExpr * CastType::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) 
      CastType(NULL, 
               (getType() ? getType()->newCopy(outHeap) : NULL));
  else
    result = derivedNode;
  
  ((CastType*)result)->makeNullable_ = makeNullable_;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

NABoolean Cast::isEquivalentForCodeGeneration(const ItemExpr * other)
  {
  NABoolean rc = FALSE;        // assume failure

  if (hasBaseEquivalenceForCodeGeneration(other))
    {
    // we know that other is a Cast, and that the children are equivalent
    Cast * otherCast = (Cast *)other;
    const NAType & otherCastType = *(otherCast->type_);

    if (type_->operator==(otherCastType))
      if (checkForTruncation_ == otherCast->checkForTruncation_)
        rc = TRUE;        // the two casts can be produced by the same code
    }

  return rc;
  }

NABoolean Cast::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  Cast * tmp = (Cast *) other;
  
  return  
      (this->type_->operator == (*(tmp->type_) )) &&
      (this->checkForTruncation_ == tmp->checkForTruncation_ ) &&
      (this->reverseDataErrorConversionFlag_ == tmp->reverseDataErrorConversionFlag_ ) &&
    //      (this->noStringTruncationWarnings_ == tmp->noStringTruncationWarnings_  ) &&
      (this->flags_ == tmp->flags_);
}
// --------------------------------------------------------------
// member functions for CharFunc operator
// --------------------------------------------------------------

ItemExpr * CharFunc::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL) {
    result = new (outHeap) CharFunc(charSet_, child(0));
  }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}


// --------------------------------------------------------------
// member functions for IfThenElse operator
// --------------------------------------------------------------
ItemExpr * IfThenElse::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) IfThenElse(NULL, NULL, NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

NABoolean IfThenElse::isCharTypeMatchRulesRelaxable()
{
   NABoolean ok =
             child(0)->isCharTypeMatchRulesRelaxable() &&
             child(1)->isCharTypeMatchRulesRelaxable();

   if (child(2).getPtr() != NULL)
      ok = ok && child(2)->isCharTypeMatchRulesRelaxable();

   return ok;
}

void IfThenElse::unparse(NAString &result,
		       PhaseEnum phase,
		       UnparseFormatEnum form,
		       TableDesc * tabId) const
{
  // Is this for MV use?
  if ( (form != MV_SHOWDDL_FORMAT) &&
       (form != QUERY_FORMAT) &&
       (form != COMPUTED_COLUMN_FORMAT))
  {
    // No - sorry, use the default code.
    ItemExpr::unparse(result, phase, form, tabId);
  }
  else
  {
    // Instead of: case( if_then_else(cond1, op1, if_then_else(cond2, op2, op3)))
    // do this: CASE WHEN cond1 THEN op1 WHEN cond2 THEN op2 ELSE op3 END
    result += " WHEN ";
    child(0)->unparse(result, phase, form, tabId);
    result += " THEN ";
    child(1)->unparse(result, phase, form, tabId);
    // Any more nested conditions?
    if (child(2)->getOperatorType() == ITM_IF_THEN_ELSE)
    {
      // Yes - Recursive call to the next condition
      child(2)->unparse(result, phase, form, tabId);
    }
    else
    {
      // No - Do the ELSE.
      result += " ELSE ";
      child(2)->unparse(result, phase, form, tabId);
      result += " END ";
    }
  }
}

//----------------------------------------------------------------
// member functions of RaiseError operator
//----------------------------------------------------------------

RaiseError::~RaiseError() {}

const NAString RaiseError::getText() const
{
  return NAString("RaiseError(") + getConstraintName() + ")";
}

ItemExpr * RaiseError::copyTopNode (ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    {
      if (getArity() > 0) // Do we have string expressions?
        result = new (outHeap) RaiseError(getSQLCODE(),
                                          getConstraintName(),
                                          child(0)->copyTree(outHeap),
                                          outHeap);
      else
        result = new (outHeap) RaiseError(getSQLCODE(),
                                          getConstraintName(),
                                          getTableName(),
                                          optionalStr_,
                                          type_,
                                          outHeap);
    }
  else
    result = derivedNode;
  
  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for Narrow operator
// --------------------------------------------------------------

Narrow::~Narrow() {}

// Note:  We simply inherit isCovered() from Cast

ItemExpr * Narrow::copyTopNode(ItemExpr *derivedNode,
			       CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
  {
    // Begin_Fix 10-040114-2431
    // 02/18/2004
    // changed to copy data member matchChildNullability_
    result = new (outHeap) Narrow(NULL,
                                  NULL,
                                  getType()->newCopy(outHeap),
                                  ITM_NARROW,
                                  FALSE,
                                  matchChildNullability_);
    // End_Fix 10-040114-2431

    ((Narrow*)result)->setMatchChildType(matchChildType());
  }
  else
    result = derivedNode;

  // $$$
  // $$$ shouldn't this last line instead be something like
  // $$$   Cast::copyTopNode(result, outHeap); ?!!??
  return ItemExpr::copyTopNode(result, outHeap);
}

NABoolean Narrow::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  Narrow * tmp = (Narrow *) other;

  return 
    (this->matchChildNullability_ == tmp->matchChildNullability_);

}
// -----------------------------------------------------------------------
// member functions for class InstantiateNull
// -----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Walk through an ItemExpr tree and gather the ValueIds of those
// expressions that behave as if they are "leaves" for the sake of
// the coverage test, e.g., expressions that have no children, or
// aggregate functions, or instantiate null. These are usually values
// that are produced in one "scope" and referenced above that "scope"
// in the dataflow tree for the query.
// ----------------------------------------------------------------------
void InstantiateNull::getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                                const GroupAttributes& coveringGA,
                                                const ValueIdSet & newExternalInputs) const
{
  leafValues += getValueId();
} // InstantiateNull::getLeafValuesForCoverTest()

ItemExpr * InstantiateNull::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap)
      InstantiateNull(NULL, getType()->newCopy(outHeap));
  else
    result = derivedNode;

  ((InstantiateNull *)result)->beginLOJTransform_ = beginLOJTransform_;
  return BuiltinFunction::copyTopNode(result, outHeap);
}

NABoolean InstantiateNull::isAColumnReference( )
  { 
    if (child(0)) {
     /*  return child(0)->isAColumnReference();
     commenting this line out as it causes regression tests to fail */
      if ((child(0)->getOperatorType() != ITM_VALUEIDUNION) &&
	  (NOT child(0)->isAnAggregate()))
	  return TRUE;
    }
      return FALSE;
  }

NABoolean InstantiateNull::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  InstantiateNull * tmp = (InstantiateNull *) other;

  return  
    (this->NoCheckforLeftToInnerJoin == tmp->NoCheckforLeftToInnerJoin ) &&
    (this->beginLOJTransform_ == tmp->beginLOJTransform_ );
}

// InstantiateNull is never equivalent to another instance.  Even when
// two instances are similar, they cannot be considered equivalent
// since they may have come from different outer joins.  The
// InstantiateNull ItemExpr operates in the context of a join, but
// this is not recorded in the data members of the class.
NABoolean InstantiateNull::isEquivalentForCodeGeneration(const ItemExpr * other)
{
  return FALSE;
}



// --------------------------------------------------------------
// member functions for BitOperFunc operator
// --------------------------------------------------------------
Int32 BitOperFunc::getArity() const
{
  if (child(0) == NULL)
    return 0;
  else if (child(1) == NULL)
    return 1;
  else if (child(2) == NULL)
    return 2;
  else
    return 3;
}

ItemExpr * BitOperFunc::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) BitOperFunc(getOperatorType(), NULL,NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

const NAString BitOperFunc::getText() const
{
  switch (getOperatorType())
    {
    case ITM_BITAND:       return "bitand";
    case ITM_BITOR:        return "bitor";
    case ITM_BITXOR:       return "bitxor";
    case ITM_BITNOT:       return "bitnot";
    case ITM_BITEXTRACT:   return "bitextract";
    default:
      return "unknown bit func";
    } // switch
}

// --------------------------------------------------------------
// member functions for CompEncode operator
// --------------------------------------------------------------

CompEncode::~CompEncode() {}

ItemExpr * CompEncode::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) CompEncode(NULL, 
                                      descFlag_, 
                                      length_,
                                      collationType_,
                                      regularNullability_,
                                      outHeap);
  else
    result = derivedNode;

  ((CompEncode *) result)->caseinsensitiveEncode_ = caseinsensitiveEncode_;
  ((CompEncode *) result)->encodedCollation_ = encodedCollation_;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

Lng32  CompEncode::getEncodedLength(  const CharInfo::Collation collation,
				  const CollationInfo::CollationType ct,
				  const Lng32 srcLength,
				  const NABoolean nullable)
{
  CMPASSERT(CollationInfo::isSystemCollation(collation));
    	
  Int32 nPasses= CollationInfo::getCollationNPasses(collation);

  switch (ct)
  {
    case CollationInfo::Sort:
    {
      return nPasses *  (srcLength + 1 ) +  
	      (nullable? ExpTupleDesc::KEY_NULL_INDICATOR_LENGTH: 0 )  ;
    }
    case CollationInfo::Compare:
    {
	return nPasses * (srcLength + 1 ) ;
    }
    case CollationInfo::Search:
    {
      return nPasses * srcLength;
    }
    default:
    {
	    CMPASSERT(0);
	    return NULL;
    }
  }
}

// --------------------------------------------------------------
// member functions for CompDecode operator
// --------------------------------------------------------------

CompDecode::~CompDecode() {}

ItemExpr * CompDecode::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) CompDecode(NULL,
                                      unencodedType_->newCopy(outHeap),
                                      descFlag_,
				      length_,
                                      collationType_,
				      regularNullability_,
                                      outHeap);
  else
    result = derivedNode;

  return CompEncode::copyTopNode(result, outHeap);
}


// --------------------------------------------------------------
// member functions for LOBoper operator
// --------------------------------------------------------------
ItemExpr * LOBoper::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  LOBoper *result;

  if (derivedNode == NULL)
    result = new (outHeap) LOBoper(getOperatorType(), NULL, NULL, NULL,obj_);
  else
    result = (LOBoper*)derivedNode;

  result->lobNum() = lobNum();
  result->lobStorageType() = lobStorageType();
  result->lobStorageLocation() = lobStorageLocation();

  return BuiltinFunction::copyTopNode(result, outHeap);
}

ItemExpr * LOBinsert::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  LOBinsert *result;

  if (derivedNode == NULL)
    result = new (outHeap) LOBinsert(NULL, NULL, obj_, append_);
  else
    result = (LOBinsert*)derivedNode;

  result->insertedTableObjectUID() = insertedTableObjectUID();
  result->insertedTableSchemaName() = insertedTableSchemaName();
  //  result->lobNum() = lobNum();
  result->lobSize() = lobSize();
  result->lobFsType() = lobFsType();

  return LOBoper::copyTopNode(result, outHeap);
}

ItemExpr * LOBselect::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) LOBselect(NULL, NULL, obj_);
  else
    result = derivedNode;

  return LOBoper::copyTopNode(result, outHeap);
}

ItemExpr * LOBdelete::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) LOBdelete(NULL);
  else
    result = derivedNode;

  return LOBoper::copyTopNode(result, outHeap);
}

ItemExpr * LOBupdate::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  LOBupdate *result;

  if (derivedNode == NULL)
    result = new (outHeap) LOBupdate(NULL, NULL, NULL,obj_, append_);
  else
    result = (LOBupdate*)derivedNode;

  result->updatedTableObjectUID() = updatedTableObjectUID();
  result->updatedTableSchemaName() = updatedTableSchemaName();

  return LOBoper::copyTopNode(result, outHeap);
}
/*
Int32 LOBupdate::getArity() const
{
  if (obj_ == EMPTY_LOB_)
    return 0;
  else
    return getNumChildren();
}
*/
ItemExpr * LOBconvert::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) LOBconvert(NULL,obj_,tgtSize_);
  else
    result = derivedNode;

  return LOBoper::copyTopNode(result, outHeap);
}

ItemExpr * LOBconvertHandle::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) LOBconvertHandle(NULL, obj_);
  else
    result = derivedNode;

  return LOBoper::copyTopNode(result, outHeap);
}

ItemExpr * LOBextract::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) LOBextract(NULL, tgtSize_);
  else
    result = derivedNode;

  return LOBoper::copyTopNode(result, outHeap);
}


// --------------------------------------------------------------
// member functions for Concat operator
// --------------------------------------------------------------
ItemExpr * Concat::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Concat(NULL, NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for Format operator
// --------------------------------------------------------------

Format::~Format() {}

ItemExpr * Format::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  Format *result;

  if (derivedNode == NULL)
    result = new (outHeap) Format(NULL, formatStr_, formatCharToDate_);
  else
    result = (Format*)derivedNode;

  result->formatCharToDate_ = formatCharToDate_;
  result->formatType_ = formatType_;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for Hash operator
// --------------------------------------------------------------

ItemExpr * Hash::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Hash(NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for HashComp operator
// --------------------------------------------------------------

HashComb::~HashComb() {}

ItemExpr * HashComb::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) HashComb(NULL,NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for HashDistPartHash operator
// Hash Function used by Hash Partitioning. This function cannot change
// once Hash Partitioning is released!  Defined for all data types,
// returns a 32 bit non-nullable hash value for the data item.
//--------------------------------------------------------------
HashDistPartHash::~HashDistPartHash() {}

ItemExpr *
HashDistPartHash::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) HashDistPartHash(NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for HashDistPartHashComp operator
// This function is used to combine two hash values to produce a new
// hash value. Used by Hash Partitioning. This function cannot change
// once Hash Partitioning is released!  Defined for all data types,
// returns a 32 bit non-nullable hash value for the data item.
// --------------------------------------------------------------

HashDistPartHashComb::~HashDistPartHashComb() {}

ItemExpr *
HashDistPartHashComb::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) HashDistPartHashComb(NULL,NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for HiveFunc operator
// --------------------------------------------------------------
ItemExpr * HiveHash::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) HiveHash(NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for HiveHashComb operator
// --------------------------------------------------------------
ItemExpr * HiveHashComb::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) HiveHashComb(NULL,NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}



// --------------------------------------------------------------
// member functions for MathFunc operator
// --------------------------------------------------------------
Int32 MathFunc::getArity() const
{
  if (child(0) == NULL)
    return 0;
  else if (child(1) == NULL)
    return 1;
  else
    return 2;
}

ItemExpr * MathFunc::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) MathFunc(getOperatorType(), NULL,NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

const NAString MathFunc::getText() const
{
  switch (getOperatorType())
    {
    case ITM_ABS:          return "abs";
    case ITM_ACOS:         return "acos";
    case ITM_ASIN:         return "asin";
    case ITM_ATAN:         return "atan";
    case ITM_ATAN2:        return "atan2";
    case ITM_CEIL:         return "ceiling";
    case ITM_COS:          return "cos";
    case ITM_COSH:         return "cosh";
    case ITM_DEGREES:      return "degrees";
    case ITM_EXP:          return "exp";
    case ITM_EXPONENT:     return "'**'";
    case ITM_FLOOR:        return "floor";
    case ITM_LOG:          return "log";
    case ITM_LOG10:        return "log10";
    case ITM_LOG2:         return "log2";
    case ITM_PI:           return "pi";
    case ITM_POWER:        return "power";
    case ITM_RADIANS:      return "radians";
    case ITM_ROUND:        return "round";
    case ITM_SCALE_TRUNC:  return "truncate";
    case ITM_SIN:          return "sin";
    case ITM_SINH:         return "sinh";
    case ITM_SQRT:         return "sqrt";
    case ITM_TAN:          return "tan";
    case ITM_TANH:         return "tanh";

    default:
      return "unknown math func";
    } // switch
}

// --------------------------------------------------------------
// member functions for Modulus operator
// --------------------------------------------------------------

Modulus::~Modulus() {}

ItemExpr * Modulus::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Modulus(NULL,NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for Repeat operator
// --------------------------------------------------------------

ItemExpr * Repeat::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Repeat(NULL, NULL);
  else
    result = derivedNode;
  
  ((Repeat *) result)->setMaxLength(getMaxLength());
  ((Repeat *) result)->maxLengthWasExplicitlySet_ = maxLengthWasExplicitlySet_;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for Replace operator
// --------------------------------------------------------------

ItemExpr * Replace::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Replace(NULL, NULL, NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for ReplaceNull operator
// --------------------------------------------------------------

ItemExpr * ReplaceNull::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ReplaceNull(NULL, NULL, NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for HashDistrib operator
// --------------------------------------------------------------

HashDistrib::~HashDistrib() {}

ItemExpr * HashDistrib::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  CMPASSERT (derivedNode != NULL);

  return BuiltinFunction::copyTopNode(derivedNode, outHeap);
}

// --------------------------------------------------------------
// member functions for Hash2Distrib operator
// --------------------------------------------------------------

Hash2Distrib::~Hash2Distrib() {}

ItemExpr * Hash2Distrib::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Hash2Distrib(NULL,NULL);
  else
    result = derivedNode;

  return HashDistrib::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for ProgDistrib operator
// --------------------------------------------------------------

ProgDistrib::~ProgDistrib() {}

ItemExpr * ProgDistrib::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ProgDistrib(NULL,NULL);
  else
    result = derivedNode;

  return HashDistrib::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for ProgDistribKey operator
// --------------------------------------------------------------

ProgDistribKey::~ProgDistribKey() {}

ItemExpr *
ProgDistribKey::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ProgDistribKey(NULL,NULL,NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for PAGroup operator
// --------------------------------------------------------------

PAGroup::~PAGroup() {}

ItemExpr * PAGroup::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) PAGroup(NULL,NULL, NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for InverseOrder operator
// --------------------------------------------------------------

ItemExpr * InverseOrder::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) InverseOrder(NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for In operator
// --------------------------------------------------------------
ItemExpr * In::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) In(NULL, NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}
// --------------------------------------------------------------
// member functions for NATypeToItem operator
// --------------------------------------------------------------
Int32 NATypeToItem::getArity() const { return 0; }
const NAString NATypeToItem::getText() const
{  return natype_pointer->getTypeSQLname(TRUE); }

ItemExpr * NATypeToItem::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) NATypeToItem( natype_pointer );
  else
    result = derivedNode;

  return ItemExpr::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------
// member functions for NamedTypeToItem operator
// --------------------------------------------------------------

ItemExpr* NamedTypeToItem::copyTopNode(ItemExpr *derivedNode, CollHeap *outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) NamedTypeToItem(name_.data(),
                                           natype_pointer,
                                           outHeap);
  else
    result = derivedNode;

  return NATypeToItem::copyTopNode(result, outHeap);
}

const NAString NamedTypeToItem::getText() const
{
  return name_;
}

// --------------------------------------------------------------
// member functions for NoOp operator
// --------------------------------------------------------------

NoOp::~NoOp() {}

ItemExpr * NoOp::copyTopNode(ItemExpr * derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) NoOp(NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
}


// -----------------------------------------------------------------------
// member functions for class RandomNum
// -----------------------------------------------------------------------

RandomNum::~RandomNum() {}

ItemExpr * RandomNum::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) RandomNum(NULL, simpleRandom_);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // RandomNum::copyTopNode()

// MV, 
// -----------------------------------------------------------------------
// member functions for class GenericUpdateOutputFunction
// -----------------------------------------------------------------------
ItemExpr * GenericUpdateOutputFunction::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) GenericUpdateOutputFunction(getOperatorType());
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // GenericUpdateOutputFunction::copyTopNode()

// Triggers - 
// -----------------------------------------------------------------------
// member functions for class InternalTimestamp
// -----------------------------------------------------------------------
InternalTimestamp::~InternalTimestamp() {}

ItemExpr * InternalTimestamp::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) InternalTimestamp();
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Timestamp::copyTopNode()
//++Triggers, 

// -----------------------------------------------------------------------
// member functions for class UniqueExecuteId
// -----------------------------------------------------------------------

UniqueExecuteId::~UniqueExecuteId() {}

ItemExpr * UniqueExecuteId::copyTopNode(ItemExpr *derivedNode,
					 CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) UniqueExecuteId();
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);

} // UniqueExecuteId::copyTopNode()


// -----------------------------------------------------------------------
// member functions for class GetTriggersStatus
// -----------------------------------------------------------------------
GetTriggersStatus::~GetTriggersStatus() {}

ItemExpr * GetTriggersStatus::copyTopNode(ItemExpr *derivedNode,
										  CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) GetTriggersStatus();
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result,outHeap);

} // GetTriggersStatus::copyTopNode()

// -----------------------------------------------------------------------
// member functions for GetBitValueAt
// -----------------------------------------------------------------------
GetBitValueAt::~GetBitValueAt() {}

ItemExpr * GetBitValueAt::copyTopNode(ItemExpr *derivedNode,
									  CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
  {
    result = new (outHeap) GetBitValueAt(child(0), child(1));
  }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // GetBitValueAt::copyTopNode()

//--Triggers, 

//++MV, 
// -----------------------------------------------------------------------
// member functions for IsBitwiseAndTrue
// -----------------------------------------------------------------------
IsBitwiseAndTrue::~IsBitwiseAndTrue() {}

ItemExpr * IsBitwiseAndTrue::copyTopNode(ItemExpr *derivedNode,
					 CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
  {
    result = new (outHeap) IsBitwiseAndTrue(child(0), child(1));
  }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // IsBitwiseAndTrue::copyTopNode()

//--MV, 


// -----------------------------------------------------------------------
// member functions for class Mask
// -----------------------------------------------------------------------

Mask::~Mask() {}

ItemExpr * Mask::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Mask(ITM_MASK_SET, NULL, NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Mask::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class Shift
// -----------------------------------------------------------------------

Shift::~Shift() {}

ItemExpr * Shift::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Shift(ITM_SHIFT_RIGHT, NULL, NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Shift::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class AnsiUSERFunction
// -----------------------------------------------------------------------
AnsiUSERFunction::~AnsiUSERFunction() {}

NABoolean AnsiUSERFunction::isAUserSuppliedInput() const	{ return TRUE; }

ItemExpr * AnsiUSERFunction::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) AnsiUSERFunction(getOperatorType());
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // AnsiUSERFunction::copyTopNode()

// -----------------------------------------------------------------------
// member functions for class MonadicUSERFunction
// -----------------------------------------------------------------------
MonadicUSERFunction::~MonadicUSERFunction() {}

NABoolean MonadicUSERFunction::isAUserSuppliedInput() const	{ return TRUE; }

ItemExpr * MonadicUSERFunction::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) MonadicUSERFunction(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // MonadicUSERFunction::copyTopNode()

NABoolean MonadicUSERFunction::isCovered
                   (const ValueIdSet& newExternalInputs,
		    const GroupAttributes& coveringGA,
		    ValueIdSet& referencedInputs,
		    ValueIdSet& coveredSubExpr,
		    ValueIdSet& unCoveredExpr) const
{
  // If the argument of USER function is not a constant then it can be
  // evaluated anywhere in the tree. So check for its coverage
  ValueIdSet localSubExpr;
  for (Lng32 i = 0; i < (Lng32)getArity(); i++)
    {
      if ( coveringGA.covers(child(i)->getValueId(),
			     newExternalInputs,
			     referencedInputs,
			     &localSubExpr) )
	{
	  coveredSubExpr += child(i)->getValueId();
	}
    }

  // The USER function is not pushed down.

  return FALSE;

} // MonadicUSERFunction::isCovered

// -----------------------------------------------------------------------
// member functions for class MonadicUSERIDFunction
// -----------------------------------------------------------------------
MonadicUSERIDFunction::~MonadicUSERIDFunction() {}

ItemExpr * MonadicUSERIDFunction::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) MonadicUSERIDFunction(child(0));
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // MonadicUSERIDFunction::copyTopNode()



// -----------------------------------------------------------------------
// member functions for class Translate
// -----------------------------------------------------------------------
Translate::Translate(ItemExpr *valPtr, NAString* map_table_name)
: CacheableBuiltinFunction(ITM_TRANSLATE, 1, valPtr)
{

  if ( _strcmpi(map_table_name->data(), "UNICODE_TO_SJIS") == 0 ||
       _strcmpi(map_table_name->data(), "UTOSJ") == 0
     )
    map_table_id_ = Translate::UNICODE_TO_SJIS;
  else
    if ( _strcmpi(map_table_name->data(), "UCS2TOSJIS") == 0)
      map_table_id_ = Translate::UCS2_TO_SJIS;
    else
      if ( _strcmpi(map_table_name->data(), "UCS2TOUTF8") == 0)
        map_table_id_ = Translate::UCS2_TO_UTF8;
      else
        if ( _strcmpi(map_table_name->data(), "UCS2TOISO88591") == 0 )
          map_table_id_ = Translate::UNICODE_TO_ISO88591;
        else
          if ( _strcmpi(map_table_name->data(), "SJIS_TO_UNICODE") == 0 ||
               _strcmpi(map_table_name->data(), "SJTOU") == 0
            )
            map_table_id_ = Translate::SJIS_TO_UNICODE;
          else
            if ( _strcmpi(map_table_name->data(), "SJISTOUCS2") == 0)
              map_table_id_ = Translate::SJIS_TO_UCS2;
            else
              if ( _strcmpi(map_table_name->data(), "UTF8TOUCS2") == 0 )
                map_table_id_ = Translate::UTF8_TO_UCS2;
              else
                if ( _strcmpi(map_table_name->data(), "ISO88591TOUCS2") == 0 )
                  map_table_id_ = Translate::ISO88591_TO_UNICODE;

  else if ( _strcmpi(map_table_name->data(), "ISO88591TOUTF8") == 0 )
    map_table_id_ = Translate::ISO88591_TO_UTF8;
  else if ( _strcmpi(map_table_name->data(), "UTF8TOISO88591") == 0 )
    map_table_id_ = Translate::UTF8_TO_ISO88591;
  else if ( _strcmpi(map_table_name->data(), "SJISTOUTF8") == 0 )
    map_table_id_ = Translate::SJIS_TO_UTF8;
  else if ( _strcmpi(map_table_name->data(), "UTF8TOSJIS") == 0 )
    map_table_id_ = Translate::UTF8_TO_SJIS;
  else if ( _strcmpi(map_table_name->data(), "GBKTOUTF8") == 0 )
    map_table_id_ = Translate::GBK_TO_UTF8;

                else
                  if ( _strcmpi(map_table_name->data(), "KANJITOISO88591") == 0 )
                    map_table_id_ = Translate::KANJI_MP_TO_ISO88591;
                  else
                    if ( _strcmpi(map_table_name->data(), "KSC5601TOISO88591") == 0 )
                      map_table_id_ = Translate::KSC5601_MP_TO_ISO88591;
                    else
                      map_table_id_ = UNKNOWN_TRANSLATION;

  allowsSQLnullArg() = FALSE;
}

Translate::Translate(ItemExpr *valPtr, Int32 map_table_id)
       : CacheableBuiltinFunction(ITM_TRANSLATE, 1, valPtr)
{
    map_table_id_ = map_table_id;
    allowsSQLnullArg() = FALSE;
}

ItemExpr * Translate::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) Translate(child(0), getTranslateMapTableId());
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // Translate::copyTopNode()


// do not know why. But if this function is put inside Translate's
// class definition, cl.exe aborts with the following info:
// cl.exe ... sooutput.cpp
//../rogue\rw/locale.h(243) : fatal error C1001: INTERNAL COMPILER ERROR
//                (compiler file 'msc1.cpp', line 1188)
//    Please choose the Technical Support command on the Visual C++
//    Help menu, or open the Technical Support help file for more information
NABoolean Translate::isCharTypeMatchRulesRelaxable()
{
  return child(0)->isCharTypeMatchRulesRelaxable();
}

void Translate::unparse(NAString &result,
                        PhaseEnum phase,
                        UnparseFormatEnum form,
                        TableDesc* tabId) const
{
  if (CmpCommon::getDefault(MVQR_LOG_QUERY_DESCRIPTORS) != DF_DUMP_MV)
    return CacheableBuiltinFunction::unparse(result, phase, form, tabId);
  else
  {
    result += "translate(";
    child(0)->unparse(result, phase, form, tabId);
    result += " using ";
    NAString translation;
    switch (map_table_id_)
    {
      case UNICODE_TO_SJIS         : translation="UCS2_TO_SJIS";        break;
      case UNICODE_TO_ISO88591     : translation="UCS2TOISO88591";      break;
      case ISO88591_TO_UNICODE     : translation="ISO88591TOUCS2";      break;
      case SJIS_TO_UNICODE         : translation="SJIS_TO_UCS2";        break;
      case UCS2_TO_SJIS            : translation="UCS2TOSJIS";          break;
      case SJIS_TO_UCS2            : translation="SJISTOUCS2";          break;
      case UCS2_TO_UTF8            : translation="UCS2TOUTF8";          break;
      case UTF8_TO_UCS2            : translation="UTF8TOUCS2";          break;
      case UTF8_TO_SJIS            : translation="UTF8TOSJIS";          break;
      case SJIS_TO_UTF8            : translation="SJISTOUTF8";          break;
      case UTF8_TO_ISO88591        : translation="UTF8TOISO88591";      break;
      case ISO88591_TO_UTF8        : translation="ISO88591TOUTF8";      break;
      case KANJI_MP_TO_ISO88591    : translation="KANJITOISO88591";     break;
      case KSC5601_MP_TO_ISO88591  : translation="KSC5601TOISO88591";   break;
      case UNKNOWN_TRANSLATION     : translation="UNKNOWN_TRANSLATION"; break;                                                  
    }
    result += translation;
    result += ")";
  }                                               
}                                                 
                                                  
HbaseColumnLookup::~HbaseColumnLookup()
{
}

ItemExpr * HbaseColumnLookup::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseColumnLookup(child(0), hbaseCol_, naType_);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // HbaseColumnLookup::copyTopNode()
                                                  
HbaseColumnsDisplay::~HbaseColumnsDisplay()
{
}

ItemExpr * HbaseColumnsDisplay::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseColumnsDisplay(child(0), csl_, displayWidth_);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // HbaseColumnsDisplay::copyTopNode()

HbaseColumnCreate::~HbaseColumnCreate()
{
}

ItemExpr * HbaseColumnCreate::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  HbaseColumnCreate *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseColumnCreate(hccol_);
  else
    result = (HbaseColumnCreate*)derivedNode;

  result->colNameMaxLen_ = colNameMaxLen_;
  result->colValMaxLen_ = colValMaxLen_;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // HbaseColumnLookup::copyTopNode()
                                                  
SequenceValue::~SequenceValue()
{
}

ItemExpr * SequenceValue::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  SequenceValue *result;

  if (derivedNode == NULL)
    result = new (outHeap) SequenceValue(seqCorrName_, currVal_, nextVal_);
  else
    result = (SequenceValue*)derivedNode;

  result->naTable_ = naTable_;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // SequenceValue::copyTopNode()
        
// HbaseTimestamp                                          
HbaseTimestamp::~HbaseTimestamp()
{
}

ItemExpr * HbaseTimestamp::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  HbaseTimestamp *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseTimestamp(col_);
  else
    result = (HbaseTimestamp*)derivedNode;

  result->colIndex_ = colIndex_;
  result->colName_ = colName_;
  result->tsVals_ = tsVals_;

  return BuiltinFunction::copyTopNode(result, outHeap);
} // HbaseTimestamp::copyTopNode()
  
// HbaseTimestampRef                                          
HbaseTimestampRef::~HbaseTimestampRef()
{
}

ItemExpr * HbaseTimestampRef::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  HbaseTimestampRef *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseTimestampRef(col_);
  else
    result = (HbaseTimestampRef*)derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
} // HbaseTimestamp::copyTopNode()
  
NABoolean HbaseTimestamp::isCovered
(const ValueIdSet& newExternalInputs,
 const GroupAttributes& coveringGA,
 ValueIdSet& referencedInputs,
 ValueIdSet& coveredSubExpr,
 ValueIdSet& unCoveredExpr) const
{
  //return TRUE;
  return FALSE;
}

// HbaseVersion                                          
HbaseVersion::~HbaseVersion()
{
}

ItemExpr * HbaseVersion::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  HbaseVersion *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseVersion(col_);
  else
    result = (HbaseVersion*)derivedNode;

  result->colIndex_ = colIndex_;
  result->colName_ = colName_;
  result->tsVals_ = tsVals_;

  return BuiltinFunction::copyTopNode(result, outHeap);
} // HbaseVersion::copyTopNode()
  
// HbaseVersionRef                                          
HbaseVersionRef::~HbaseVersionRef()
{
}

ItemExpr * HbaseVersionRef::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  HbaseVersionRef *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseVersionRef(col_);
  else
    result = (HbaseVersionRef*)derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
} // HbaseVersion::copyTopNode()
  
NABoolean HbaseVersion::isCovered
(const ValueIdSet& newExternalInputs,
 const GroupAttributes& coveringGA,
 ValueIdSet& referencedInputs,
 ValueIdSet& coveredSubExpr,
 ValueIdSet& unCoveredExpr) const
{
  //return TRUE;
  return FALSE;
}

// RowNumFunc                        
RowNumFunc::~RowNumFunc()
{
}

ItemExpr * RowNumFunc::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  RowNumFunc *result;

  if (derivedNode == NULL)
    result = new (outHeap) RowNumFunc();
  else
    result = (RowNumFunc*)derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // RowNumFunc::copyTopNode()
                                                  
NABoolean RowNumFunc::isCovered
                   (const ValueIdSet& newExternalInputs,
		    const GroupAttributes& coveringGA,
		    ValueIdSet& referencedInputs,
		    ValueIdSet& coveredSubExpr,
		    ValueIdSet& unCoveredExpr) const
{
  // The ROWNUM function is not pushed down.
  return FALSE;
} // RowNumFunc::isCovered
                                                  
// -----------------------------------------------------------------------
// Member functions for ItmSequenceFunction
//------------------------------------------------------------------------
// -----------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------
ItmSequenceFunction::~ItmSequenceFunction() {}

NABoolean ItmSequenceFunction::isASequenceFunction() const   // virtual method
  { return TRUE; }

NABoolean ItmSequenceFunction::isCovered(const ValueIdSet& newExternalInputs,
                               const GroupAttributes& newRelExprAnchorGA,
                               ValueIdSet& referencedInputs,
                               ValueIdSet& coveredSubExpr,
                               ValueIdSet& unCoveredExpr) const
{
  // ---------------------------------------------------------------------
  // If the operand is covered, then return its ValueId in coveredSubExpr.
  // ---------------------------------------------------------------------
  ValueIdSet localSubExpr;
  for(Lng32 i = 0; i < (Lng32)getArity(); i++)
  {

     if (child(i)->getOperatorType() == ITM_ITEM_LIST)
      {
        // child is a multi-valued expression, test coverage on individuals
        // 
        ExprValueId treePtr = child(i);

        ItemExprTreeAsList values(&treePtr,
                                  ITM_ITEM_LIST,
                                  RIGHT_LINEAR_TREE);

        CollIndex nc = values.entries();
        for (CollIndex m = 0; m < nc; m++)
          {
            if(newRelExprAnchorGA.covers(values[m]->getValueId(),
                                         newExternalInputs,
                                         referencedInputs,
                                         &localSubExpr))
              {
                coveredSubExpr += values[m]->getValueId();
              }
            coveredSubExpr += localSubExpr;
          }
      }
    else 
      {

	if(newRelExprAnchorGA.covers(child(i)->getValueId(),
				     newExternalInputs,
				     referencedInputs,
				     &localSubExpr))
	{
	  coveredSubExpr += child(i)->getValueId();
	}
	coveredSubExpr += localSubExpr;
    }

  }
  // ---------------------------------------------------------------------
  // The ItmSequenceFunction function is coerced to fail the coverage test even
  // when its operands isCovered(). This is because only the RelSequence node
  // can evaluate the function. The function is associated with a RelSequence
  // node at the very beginning and we don't allow it to be pushed down
  // even if the function's operands are covered at the node's child.
  // ---------------------------------------------------------------------
  return FALSE;
}

void ItmSequenceFunction::getLeafValuesForCoverTest(ValueIdSet& leafValues, 
                                                    const GroupAttributes& coveringGA,
                                                    const ValueIdSet & newExternalInputs) const
{
  // ItmSequenceFunction is considered a leaf operator for cover test.
  leafValues += getValueId();
}


const NAString ItmSequenceFunction::getText() const
{
  switch (getOperatorType())
    {
    case ITM_DIFF1:
      return "diff1";
    case ITM_DIFF2:
      return "diff2";
    case ITM_LAST_NOT_NULL:
      return "lastnotnull";
    case ITM_MOVING_AVG:
      return "movingavg";
    case ITM_MOVING_COUNT:
      return "movingcount";
    case ITM_MOVING_MAX:
      return "movingmax";
    case ITM_MOVING_MIN:
      return "movingmin";
    case ITM_MOVING_RANK:
      return "movingrank";
    case ITM_MOVING_SDEV:
      return "movingsdev";
    case ITM_MOVING_SUM:
      return "movingsum";
    case ITM_MOVING_VARIANCE:
      return "movingvariance";
    case ITM_RUNNING_AVG:
      return "runningavg";
    case ITM_RUNNING_CHANGE:
      return "rows since changed";
    case ITM_RUNNING_COUNT:
      return "runningcount";
    case ITM_RUNNING_MAX:
      return "runningmax";
    case ITM_RUNNING_MIN:
      return "runningmin";
    case ITM_RUNNING_RANK:
      return "runningrank";
    case ITM_RUNNING_SDEV:
      return "runningsdev";
    case ITM_RUNNING_SUM:
      return "runningsum";
    case ITM_RUNNING_VARIANCE:
      return "runningvariance";
    case ITM_OFFSET:
      if (getArity() == 1 )// internally created offsets
      {
        char str[30]; 
        str_sprintf(str, "offset[%d]",((ItmSeqOffset *)this)->getOffsetConstantValue());
        return str;
      }
      else
      {
        return "offset";
      }
    case ITM_THIS:
      return "this";
    case ITM_NOT_THIS:
      return "not this";

    case ITM_OLAP_COUNT:
      return "olap count";
    case ITM_OLAP_MAX:
      return "olap max";
    case ITM_OLAP_MIN:
      return "olap min";
    case ITM_OLAP_RANK:
      return "olap rank";
    case ITM_OLAP_DRANK:
      return "olap dense rank";
    case ITM_OLAP_SDEV:
      return "olap sdev";
    case ITM_OLAP_SUM:
      return "olap sum";
    case ITM_OLAP_VARIANCE:
      return "olap variance";

    default:
      return "unknown sequence function";
    } // switch
}

ItemExpr * ItmSequenceFunction::copyTopNode(ItemExpr *derivedNode,
                                            CollHeap* outHeap)
{
  ItmSequenceFunction *result = NULL;

  if (derivedNode == NULL)
   ABORT(
   "copyTopNode() can only be called for a derived class of ItmSequenceFunction"
         );
  else
    result = (ItmSequenceFunction *)derivedNode;

  // Copy OLAP Window Function information.
  //
  result->isOLAP_ = isOLAP_;
  result->olapPartitionBy_ = olapPartitionBy_;
  result->olapOrderBy_ = olapOrderBy_;
  result->isTDFunction_ = isTDFunction_;

  return BuiltinFunction::copyTopNode(result, outHeap);
} // ItmSequenceFunction::copyTopNode()

ItemExpr * ItmSequenceFunction::transformTDFunction(BindWA * bindWA)
{
  if (getOperatorType() != ITM_RUNNING_RANK) // we are only doing td rank in this ohase
    return this;

  if (!isTDFunction())
    return this;

  if (!olapPartitionBy_)
    return this;

  ItmSequenceFunction *partClause = NULL;    

  partClause = new (bindWA->getCurrentScope()->collHeap())
                    ItmSeqRunningFunction(ITM_RUNNING_CHANGE,
                                          olapPartitionBy_->copyTree(bindWA->getCurrentScope()->collHeap()) );

  partClause->setIsTDFunction(TRUE);
  partClause->setOlapOrderBy(olapOrderBy_->copyTree(bindWA->getCurrentScope()->collHeap()));
  partClause->setOlapPartitionBy(olapPartitionBy_->copyTree(bindWA->getCurrentScope()->collHeap()));

  ItmSequenceFunction *seqFunc = new (bindWA->getCurrentScope()->collHeap())
                                      ItmSeqMovingFunction(ITM_MOVING_RANK, child(0), partClause);

  seqFunc->setIsTDFunction(TRUE);
  seqFunc->setOlapOrderBy(olapOrderBy_->copyTree(bindWA->getCurrentScope()->collHeap()));
  seqFunc->setOlapPartitionBy(olapPartitionBy_->copyTree(bindWA->getCurrentScope()->collHeap()));
                                    
  return seqFunc;
}

// -----------------------------------------------------------------------
// member functions for ItmSeqRunningFunction
// -----------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------
ItmSeqRunningFunction::~ItmSeqRunningFunction() {}

NABoolean ItmSeqOlapFunction::isOlapFunction() const   // virtual method
  { return TRUE; }


ItmSeqOlapFunction::~ItmSeqOlapFunction() {}


ItemExpr * ItmSeqRunningFunction::copyTopNode(ItemExpr *derivedNode,
                                              CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    {
     result = new (outHeap) ItmSeqRunningFunction(getOperatorType(),
                                                  child(0));
     ((ItmSeqRunningFunction *)result)->setIsOLAP(isOLAP());
    }
  else
    result = derivedNode;

  return ItmSequenceFunction::copyTopNode(result, outHeap);
} // ItmSeqRunningFunction::copyTopNode()


ItemExpr * ItmSeqOlapFunction::copyTopNode(ItemExpr *derivedNode,
                                              CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    {
     result = new (outHeap) ItmSeqOlapFunction(getOperatorType(),
                                                  child(0));
    }
  else
    result = derivedNode;

  ((ItmSeqOlapFunction *)result)->frameStart_ = frameStart_;
  ((ItmSeqOlapFunction *)result)->frameEnd_ = frameEnd_;

  return ItmSequenceFunction::copyTopNode(result, outHeap);
} // ItmSeqOlapFunction::copyTopNode()


NABoolean
ItmSeqOlapFunction::inverseOLAPOrder(CollHeap *heap)
{
  //if (frameStart_ != -INT_MAX) {
  if (getOlapOrderBy())
    {
      ItemExprList orderList(getOlapOrderBy(),0);

      ItemExpr *newOrder = NULL;
      for (CollIndex i = 0; i < orderList.entries(); i++) {

        ItemExpr *itm = orderList[i];
        if (itm->getOperatorType() == ITM_INVERSE) {
          itm = itm->child(0);
        } else {
          itm = new (heap) InverseOrder(itm);
        }
        if(newOrder) {
          newOrder = new(heap) ItemList(newOrder, itm);
        } else {
          newOrder = itm;
        }
      }
      
      setOlapOrderBy(newOrder);
    }

  Lng32 olapRowsTemp = frameEnd_;
  frameEnd_ = -frameStart_;
  frameStart_ = -olapRowsTemp;
  return TRUE;
  //}
  //return FALSE;
}

NABoolean ItmSeqOlapFunction::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  ItmSeqOlapFunction * otherOSeq = (ItmSeqOlapFunction *) other;

  return 
      (this->frameStart_ == otherOSeq->frameStart_ &&
       this->frameEnd_ == otherOSeq->frameEnd_) ;
}

NABoolean ItmSequenceFunction::isEquivalentForBinding(const ItemExpr * other)
{
  
  
  if ( getOperatorType() != other->getOperatorType() || getArity() != other->getArity() )
  {
    return FALSE;
  }
  
  for (Lng32 i = 0; (i < getArity()); i++)
  {
    if (child(i)->isASequenceFunction())
    {
      ItmSequenceFunction * seqFunc = (ItmSequenceFunction * ) child(i).getValueId().getItemExpr();
      if (!seqFunc->isEquivalentForBinding(other->child(i)))
      {
	return FALSE;
      }
    }
    else 
    {
      if (!child(i)->hasBaseEquivalenceForCodeGeneration(other->child(i)))
    {
      return FALSE;
    }
    }
  }
  return TRUE;
 }
// -----------------------------------------------------------------------
// member functions for Offset
// -----------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------
ItmSeqOffset::~ItmSeqOffset() {}

// -----------------------------------------------------------------------
Int32 ItmSeqOffset::getArity() const
{
  if (child(1))
  {
    if (child(2))
      return 3;
    else
      return 2;
  }
  else
    return 1;
}

ItemExpr * ItmSeqOffset::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
   {
    switch (getArity())
    {
      case 2:
         result = new (outHeap) ItmSeqOffset(child(0), child(1), NULL, 
                                             nullRowIsZero());
         break;

      case 3:
         result = new (outHeap) ItmSeqOffset(child(0), child(1), child(2),
                                             nullRowIsZero());
         break;

      default:
         result = new (outHeap) ItmSeqOffset( child(0),
                                             nullRowIsZero(),
                                              getOffsetConstantValue(),
                                              isLeading(),
                                              winSize());
         break;
    }
    ((ItmSeqOffset *)result)->setIsOLAP(isOLAP());
   }
  else
   {
    result = derivedNode;
   }

  return ItmSequenceFunction::copyTopNode(result, outHeap);

} // ItmSeqOffset::copyTopNode()

NABoolean ItmSeqOffset::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  ItmSeqOffset * otherOffset = (ItmSeqOffset *) other;

  return 
    (this->offsetConstantValue_ == otherOffset->offsetConstantValue_) &&
    (this->nullRowIsZero_ == otherOffset->nullRowIsZero_) &&
    (this->leading_ == otherOffset->leading_) &&
    (this->winSize_ == otherOffset->winSize_);
}

// -----------------------------------------------------------------------
// member functions for DIFF1
// -----------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------
ItmSeqDiff1::~ItmSeqDiff1() {}

// -----------------------------------------------------------------------

ItemExpr * ItmSeqDiff1::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
   {
    if (getArity() == 1)
    {
     result = new (outHeap) ItmSeqDiff1(child(0));
    }
    else
    {
     result = new (outHeap) ItmSeqDiff1(child(0), child(1));
    }
   }
  else
   {
    result = derivedNode;
   }

  return ItmSequenceFunction::copyTopNode(result, outHeap);

} // ItmSeqDiff1::copyTopNode()

// -----------------------------------------------------------------------
// member functions for DIFF2
// -----------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------
ItmSeqDiff2::~ItmSeqDiff2() {}

// -----------------------------------------------------------------------

ItemExpr * ItmSeqDiff2::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
   {
    if (getArity() == 1)
    {
     result = new (outHeap) ItmSeqDiff2(child(0));
    }
    else
    {
     result = new (outHeap) ItmSeqDiff2(child(0), child(1));
    }
   }
  else
   {
    result = derivedNode;
   }

  return ItmSequenceFunction::copyTopNode(result, outHeap);

} // ItmSeqDiff2::copyTopNode()

// -----------------------------------------------------------------------
// member functions for ItmSeqRowsSince
// -----------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------
ItmSeqRowsSince :: ~ItmSeqRowsSince() {};

ItemExpr  *ItmSeqRowsSince :: copyTopNode(ItemExpr *derivedNode,
                                          CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    {
     if (getArity() == 1)
     {
      result = new (outHeap) ItmSeqRowsSince(child(0), NULL, includeCurrentRow());
     }
     else
     {
      result = new (outHeap) ItmSeqRowsSince(child(0), child(1), includeCurrentRow());
     }
    }
  else
  {
   result = derivedNode;
  }

  return ItmSequenceFunction::copyTopNode(result, outHeap);
} // ItmSeqRowsSince::copyTopNode()

const NAString ItmSeqRowsSince :: getText() const
{
  if (includeCurrentRow())
   return "rows since inclusive";
  else
   return "rows since";
}  // ItmSeqRowsSince::getText()

// -----------------------------------------------------------------------
// member functions for ItmSeqMovingFunction
// -----------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------
ItmSeqMovingFunction::~ItmSeqMovingFunction() {}

ItemExpr * ItmSeqMovingFunction::copyTopNode(ItemExpr *derivedNode,
                                             CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL) {
    result = new (outHeap) ItmSeqMovingFunction(getOperatorType(),
                                                child(0), child(1), child(2));
  ((ItmSeqMovingFunction *)result)->setIsOLAP(isOLAP());
  }
  else
    result = derivedNode;

  if (this->getSkipMovingMinMaxTransformation() == TRUE)
  {
   ((ItmSeqMovingFunction *)result)->setSkipMovingMinMaxTransformation();
  }

  return ItmSequenceFunction::copyTopNode(result, outHeap);
} // ItmSeqMovingFunction::copyTopNode()

NABoolean ItmSeqMovingFunction::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;
  if (getOperatorType() != other->getOperatorType() || 
        getArity() != other->getArity())
    return FALSE;

  ItmSeqMovingFunction * otherMSeq = (ItmSeqMovingFunction *) other;

  return 
      (this->skipMovingMinMaxTransformation_ == otherMSeq->skipMovingMinMaxTransformation_) ;
}
// -----------------------------------------------------------------------
// member functions for THIS function
// -----------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------
ItmSeqThisFunction::~ItmSeqThisFunction() {}

ItemExpr * ItmSeqThisFunction::copyTopNode(ItemExpr *derivedNode,
                                           CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
   {
     result = new (outHeap) ItmSeqThisFunction (child(0));
   }
  else
   {
    result = derivedNode;
   }

  return ItmSequenceFunction::copyTopNode(result, outHeap);

} // ItmSeqThisFunction::copyTopNode()

// -----------------------------------------------------------------------
// member functions for NotTHIS function
// -----------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------

ItmSeqNotTHISFunction::~ItmSeqNotTHISFunction(){}

ItemExpr *ItmSeqNotTHISFunction::copyTopNode(ItemExpr *derivedNode,
                                             CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
   {
     result = new (outHeap) ItmSeqNotTHISFunction (child(0));
   }
  else
   {
    result = derivedNode;
   }

  return ItmSequenceFunction::copyTopNode(result, outHeap);

} // ItmSeqNotTHISFunction::copyTopNode()

// -----------------------------------------------------------------------
// member functions for ItmScalarMinMax
// -----------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------
ItmScalarMinMax::~ItmScalarMinMax() {};

ItemExpr *ItmScalarMinMax::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
   result = new (outHeap)ItmScalarMinMax(getOperatorType(), child(0), child(1));
  else
   result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
} // ItmScalarMinMax::copyTopNode

NABoolean ItemExpr::containsOneRowAggregate()
{

  if ( getOperatorType() == ITM_ONE_ROW )
     return TRUE;

  for (Int32 i=0; i < getArity(); i++)
  {
    if (child(i)->castToItemExpr()->containsOneRowAggregate())
      return TRUE;
  }
  return FALSE;
} // ItemExpr::containsOneRowAggregate()

NABoolean ItemExpr::containsOpType(OperatorTypeEnum opType) const
{

  if ( getOperator().match(opType) )
     return TRUE;

  for (Int32 i=0; i < getArity(); i++)
  {
    if (child(i)->castToItemExpr()->containsOpType(opType))
      return TRUE;
  }
  return FALSE;
} // ItemExpr::containsOpType()

// If I am untransformed, I tell my parent, which in turn will mark itself
// as untransformed, and tell its parent, ..., all the way up to the
// original place where this method was called.

NABoolean ItemExpr::markPathToUnTransformedNode()
{
  NABoolean retval = FALSE ;

  if ( !nodeIsTransformed() )
    retval = TRUE ;

  for (Int32 i=0; i < getArity(); i++)
  {
    if ( child(i)->castToItemExpr()->markPathToUnTransformedNode() )
      {
        markAsUnTransformed() ;
        retval = TRUE ;
        // NB: Since this function is marking nodes as untransformed, we
        // can't simply break after the first child which is found to
        // be untransformed.
      }
  }
  return retval;
} // ItemExpr::markPathToUnTransformedNode()


NABoolean ItemExpr::isEquivalentForCodeGeneration(const ItemExpr * other)
  {
  NABoolean rc = FALSE;// by default, return FALSE (suppress common subexpr elimination

  if (getArity() == 0)  // to limit needless recursion
    {
    if (hasBaseEquivalenceForCodeGeneration(other))
      {
      // we know other is non-null, has same operator type, same arity, and
      // that its children are equivalent

      OperatorTypeEnum myOp = getOperatorType();
      const ItemExpr & refOther = (const ItemExpr &) *other;

      // as it happens, with 0-arity operators, we know that operator==
      // should return false for all cases below (because of the way the
      // code in hasBaseEquivalenceForCodeGeneration() works; but I want
      // to see if virtual forms of operator== are selected)

      if (myOp == ITM_CONSTANT)
        {
        rc = (operator==(refOther));
        }
      else if (myOp == ITM_REFERENCE)
        {
        rc = (operator==(refOther));
        }
      else if (myOp == ITM_BASECOLUMN)
        {
        rc = (operator==(refOther));
        }
      else if (myOp == ITM_INDEXCOLUMN)
        {
        rc = (operator==(refOther));
        }
      }
    }
  return rc;
  }


NABoolean ItemExpr::hasBaseEquivalenceForCodeGeneration(const ItemExpr * other)
  {
  NABoolean rc = FALSE;    // assume not

  if (other)
    {
    const ItemExpr & refOther = (const ItemExpr &) *other;  // we know it exists

    if (operator==(refOther))  // $$$$ will this get virtual operator==????
      rc = TRUE;    // equal nodes are considered equivalent
    else
      {
      if (getOperatorType() == other->getOperatorType())
        {
        Lng32 arity = getArity();

        if (arity == other->getArity())
          {
          // make sure children are equivalent
          rc = TRUE;       // be optimistic now; assume equivalent
          for (Lng32 i = 0; (i < arity) && (rc); i++)
            {
            if (!(child(i)->isEquivalentForCodeGeneration(other->child(i))))
              rc = FALSE;   // oops -- found non-equivalent children
            }
          }  // end if arity's are equal
        }  // end if oper types are equal
      }  // end else nodes aren't equal
    }  // end if other is non-null

  return rc;
  }

  ItemExpr* ItemExpr::getParticularItemExprFromTree(NAList<Lng32>& childNum, 
                                          NAList<OperatorTypeEnum>& opType) const
  {
    ItemExpr * root = (ItemExpr *) this;
    for (CollIndex i = 0; i < childNum.entries(); i++)
    {
      if ((root->getArity() > childNum[i]) && 
          (root->child(childNum[i])) && 
          (root->child(childNum[i])->getOperatorType() == opType[i])) 
      {
          root = root->child(childNum[i]);
      }
      else
      {
        DCMPASSERT(FALSE);
        return NULL;
      }
    }
    return root;
  }

ItemExpr* ItemExpr::removeRangeSpecItems(NormWA* normWA)
{
  for (Lng32 i = 0; i < getArity(); i++)
  {
    child(i) = child(i)->removeRangeSpecItems(normWA);
  }
  return this;
};

// Raj P - 1/01
// Support for OUT parameters in Stored Procedures for Java
// Set Parameter Mode, Ordinal Position and Variable Index for a
// host variable or dynamic parameter
void
HostVar::setPMOrdPosAndIndex( ComColumnDirection paramMode,
			      Int32 ordinalPosition,
			      Int32 index )
{
    paramMode_ = paramMode;
    ordinalPosition_ = ordinalPosition;
    hvIndex_ = index;
}

NABoolean HostVar::isSystemGeneratedOutputHV() const
{  
  return (isSystemGenerated() &&
           getName() == "_sys_ignored_CC_convErrorFlag"); 
}

void
DynamicParam::setPMOrdPosAndIndex( ComColumnDirection paramMode,
				   Int32 ordinalPosition,
				   Int32 index )
{
    paramMode_ = paramMode;
    ordinalPosition_ = ordinalPosition;
    dpIndex_ = index;
}

ComColumnDirection ItemExpr::getParamMode () const
{
  CMPASSERT (0);
  return COM_UNKNOWN_DIRECTION;
};

Int32 ItemExpr::getOrdinalPosition () const
{
  CMPASSERT (0);
  return -1;
}

Int32 ItemExpr::getHVorDPIndex () const
{
  CMPASSERT (0);
  return -1;
}

NABoolean ItemExpr::isARangePredicate() const
{
  OperatorTypeEnum oper = getOperatorType();

  if ((oper == ITM_GREATER)    ||
      (oper == ITM_GREATER_EQ) ||
      (oper == ITM_LESS)       ||
      (oper == ITM_LESS_EQ))
  {
    return TRUE;
  }

  return FALSE;
}


QR::ExprElement Function::getQRExprElem() const
{
  return QR::QRFunctionElem;
}

// Flipping the tree in 1 pass top->bottom
//
//ITEM_LIST  (OLD TREE)
//    /   \
//   4    ITEM_LIST
//            /   \
//           3   ITEM_LIST
//                  /    \
//                 2      1
//
//ITEM_LIST   (NEW TREE)
//   /    \	
//   1     ITEM_LIST       
//          /     \               
//         2     ITEM_LIST
//                 /    \
//                 3     4

ItemExpr * ItemExpr::reverseTree() 
{
	ItemExpr *grammarTree = this;
	ItemExpr *topNode = NULL;


	// Special Case: 1, 2 & 3 columns. Base of tree
	// only 1 column.
	if (grammarTree->getOperatorType()!=ITM_ITEM_LIST) 
		return grammarTree;

	if (grammarTree->child(1)->getOperatorType()==ITM_ITEM_LIST ) {
		
		// more than 2 columns	
		topNode = grammarTree->child(1)->child(1);
		grammarTree->child(1)->child(1) = grammarTree->child(0);

		if (topNode->getOperatorType() != ITM_ITEM_LIST) { 
			// extactly 3 columns
			grammarTree->child(0) = topNode;		
			return grammarTree;
		}
		else {
			// more than 3 columns
			grammarTree->child(0) = topNode->child(0);
		}
	}
	else {
		// exactly 2 columns
		ItemExpr *temp = grammarTree->child(1);
		grammarTree->child(1) = grammarTree->child(0);
		grammarTree->child(0) = temp; 
		return grammarTree;
	}

	while (topNode) {

		if (topNode->child(1)->getOperatorType()==ITM_ITEM_LIST ) { 
		  // not bottom of tree
			ItemExpr *temp = topNode->child(1);
			topNode->child(1) = grammarTree;
			grammarTree = topNode;
			topNode = temp;
			grammarTree->child(0) = topNode->child(0);
		}
		else {  // bottom of the tree
			topNode->child(0) = topNode->child(1);
			topNode->child(1) = grammarTree;
			grammarTree = topNode;
			break;
		}
	}

	return grammarTree;

} // end of function

QR::ExprElement ItemExpr::getQRExprElem() const
{
  return QR::QRNoElem;
}

QR::ExprElement ConstantParameter::getQRExprElem() const
{
  return QR::QRScalarValueElem;
}

// Constructor for the wrapper class RangeSpecRef:
RangeSpecRef::RangeSpecRef(OperatorTypeEnum otype,
			   OptNormRangeSpec* range,
			   ItemExpr *colValueId,
			   ItemExpr *reConsIExpr)
  : ItemExpr(ITM_RANGE_SPEC_FUNC,colValueId,reConsIExpr),
    range_(range)
{}

/* Get the already generated ValueId */
DisjunctArray * RangeSpecRef::mdamTreeWalk()
{
  return new (CmpCommon::statementHeap()) DisjunctArray(new (CmpCommon::statementHeap()) ValueIdSet(getValueId()));
}

/* Need to get the access rightchild to print out the value , rightchild is reconstructed though */
/* This will affect explain "xx" output, this needs to be completed while we get a pointer to the right child
Item expression, then it will print something like <column> "Range in" Item expression corresponding to 
<subranges> */ 
const NAString RangeSpecRef::getText()  const
{   
  return "Range in";
}

short RangeSpecRef::mdamPredGen(Generator * generator,
                                MdamPred ** head,
                                MdamPred ** tail,
                                MdamCodeGenHelper & mdamHelper,
                                ItemExpr * parent)
{
  // return dummy;
  return 0;
}

Int32 RangeSpecRef::getArity() const 
{ 
  return 2; 
}

// delete this and the range_ object associated with this, if present */
RangeSpecRef::~RangeSpecRef()
{
  if(range_)
    delete range_;
}

ItemExpr * RangeSpecRef::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  RangeSpecRef *result;
  if (derivedNode == NULL)
    // Need a Deep copy of range_ object. - Change needs to be done after
    // Copy ctor is available.
    // Curently range_->clone(outHeap) gives Null value, while copying the tree
    // So again back to shallow copy
    result = new (outHeap) RangeSpecRef(getOperatorType(),range_, NULL,NULL);
  else
    result = (RangeSpecRef *) derivedNode;

  // copy the data members
  return ItemExpr::copyTopNode(result, outHeap);
}

short RangeSpecRef::codeGen(Generator* generator)
{
  child(1)->codeGen(generator);
  return 0;
}

void RangeSpecRef::unparse(NAString &result,
                           PhaseEnum phase,
                           UnparseFormatEnum form,
                           TableDesc * tabId) const
{
  // Don't include rangespec op if query format -- it won't parse.
  if (form == QUERY_FORMAT || form == COMPUTED_COLUMN_FORMAT)
    child(1)->unparse(result, phase, form, tabId);
  else
    ItemExpr::unparse(result, phase, form, tabId);
}

ItemExpr* RangeSpecRef::removeRangeSpecItems(NormWA* normWA)
{
  return getRangeObject()->getRangeItemExpr(normWA);
};

void RangeSpecRef::getValueIdSetForReconsItemExpr(ValueIdSet &outvs)
{
  ValueIdSet items;
  ItemExpr* ie;
  child(1)->convertToValueIdSet(items, NULL, ITM_ITEM_LIST);
  for (ValueId vid=items.init(); items.next(vid); items.advance(vid))
  {
    ie = vid.getItemExpr();
    if (!ie->isLike())
      ie->convertToValueIdSet(outvs, NULL, ITM_AND);
    else
      outvs.insert(vid);  // Add LIKE expansion without decomposing
  }
}

ItemExpr* revertBackToOldTree(CollHeap *heap, ItemExpr* newTree) 
{
  if(newTree->getOperatorType() != ITM_AND && 
     newTree->getOperatorType() != ITM_OR)
  {
    if (newTree->getOperatorType() == ITM_RANGE_SPEC_FUNC ) 
    {
      return(newTree->child(1));
    }
    else
    {
      return (newTree);	
    }
  }
  else
  {
    ItemExpr* newLeftNode = revertBackToOldTree(heap,newTree->child(0));
    ItemExpr* newRightNode = revertBackToOldTree(heap,newTree->child(1));
    assert(newLeftNode != NULL && newRightNode != NULL);
    newTree->setChild(0, newLeftNode);
    newTree->setChild(1, newRightNode);
    return newTree;
  }
}

// This method reverts back to Old Tree as valueIdSet from transformed Tree as valueIdSet.
void revertBackToOldTreeUsingValueIdSet(  ValueIdSet& inputSet /* IN */, ValueIdSet& outputSet /* OUT */) 
{
	ValueIdSet orSet,outorSet;
	ItemExpr * inputItemExprTree = NULL;
    for (ValueId predId = inputSet.init();
       inputSet.next(predId);
	   inputSet.advance(predId) ){
		if( predId.getItemExpr()->getOperatorType() == ITM_RANGE_SPEC_FUNC){
          outputSet += predId.getItemExpr()->child(1)->castToItemExpr()->getValueId();
		  if(inputItemExprTree != NULL)
		  {
            outputSet += inputItemExprTree->getValueId();
			inputItemExprTree = NULL;
			outorSet.clear();
		  }
		}
		else if( predId.getItemExpr()->getOperatorType() == ITM_OR){
        predId.getItemExpr()->convertToValueIdSet(orSet, NULL, ITM_OR, FALSE);
        for (ValueId predIdOr = orSet.init();
             orSet.next(predIdOr);
	         orSet.advance(predIdOr) ){
			 if(predIdOr.getItemExpr()->getOperatorType() == ITM_RANGE_SPEC_FUNC){
				 outorSet += predIdOr.getItemExpr()->child(1)->castToItemExpr()->getValueId();
			 }
			 else
			  outorSet += predIdOr;
			}
		  if(outorSet.entries())
		  inputItemExprTree = outorSet.rebuildExprTree(ITM_OR,FALSE,FALSE);
		}
		else
		{
         outputSet += predId;
		  if(inputItemExprTree != NULL)
		  {
            outputSet += inputItemExprTree->getValueId();
			inputItemExprTree = NULL;
			outorSet.clear();
		  }
		}
	}// for (1)
	if(inputItemExprTree != NULL)
	{
      outputSet += inputItemExprTree->getValueId();
	  inputItemExprTree = NULL;
	  outorSet.clear();
	}
}


NABoolean LOBoper::isCovered
(const ValueIdSet& newExternalInputs,
 const GroupAttributes& coveringGA,
 ValueIdSet& referencedInputs,
 ValueIdSet& coveredSubExpr,
 ValueIdSet& unCoveredExpr) const
{
  // If the argument is not a constant then it can be
  // evaluated anywhere in the tree. So check for its coverage
  ValueIdSet localSubExpr;
  for (Lng32 i = 0; i < (Lng32)getArity(); i++)
    {
      if ( coveringGA.covers(child(i)->getValueId(),
			     newExternalInputs,
			     referencedInputs,
			     &localSubExpr) )
	{
	  coveredSubExpr += child(i)->getValueId();
	}
    }

  // cannot be pushed down. Must be evaluated in master exe.
  return FALSE;
}

// Evalaute the exprssion at compile time. Assume all operands are constants.
// Return NULL if the computation fails and CmpCommon::diags() may be side-affected.
ConstValue* ItemExpr::evaluate(CollHeap* heap)
{
  ValueIdList exprs;
  exprs.insert(getValueId());

  const NAType& dataType = getValueId().getType();

  Lng32 decodedValueLen = dataType.getNominalSize() + dataType.getSQLnullHdrSize();

  char staticDecodeBuf[200];
  Lng32 staticDecodeBufLen = 200;

  char* decodeBuf = staticDecodeBuf;
  Lng32 decodeBufLen = staticDecodeBufLen;

  // For character types, multiplying by 6 to deal with conversions between
  // any two known character sets allowed. See CharInfo::maxBytesPerChar()
  // for a list of max bytes per char for each supported character set.  
  Lng32 factor = (DFS2REC::isAnyCharacter(dataType.getFSDatatype())) ? 6 : 1;

  if ( staticDecodeBufLen < decodedValueLen * factor) {
    decodeBufLen = decodedValueLen * factor;
    decodeBuf = new (STMTHEAP) char[decodeBufLen];
  }

  Lng32 resultLength = 0;
  Lng32 resultOffset = 0;

  // Produce the decoded key. Refer to 
  // ex_function_encode::decodeKeyValue() for the 
  // implementation of the decoding logic.
  ex_expr::exp_return_type rc = exprs.evalAtCompileTime
    (0, ExpTupleDesc::SQLARK_EXPLODED_FORMAT, decodeBuf, decodeBufLen,
     &resultLength, &resultOffset, CmpCommon::diags()
     );


  ConstValue* result = NULL;

  if ( rc == ex_expr::EXPR_OK ) {
    CMPASSERT(resultOffset == dataType.getPrefixSizeWithAlignment());
    // expect the decodeBuf to have this layout
    // | null ind. | varchar length ind. | alignment | result |
    // |<---getPrefixSizeWithAlignment-------------->|
    // |<----getPrefixSize-------------->|

    // The method getPrefixSizeWithAlignment(), the diagram above,
    // and this code block assumes that varchar length ind. is
    // 2 bytes if present. If it is 4 bytes we should fail the 
    // previous assert

    // Next we get rid of alignment bytes by prepending the prefix
    // (null ind. + varlen ind.) to the result. ConstValue constr.
    // will process prefix + result. The assert above ensures that 
    // there are no alignment fillers at the beginning of the 
    // buffer. Given the previous assumption about size
    // of varchar length indicator, alignment bytes will be used by
    // expression evaluator only if column is of nullable type.
    // For a description of how alignment is computed, please see
    // ExpTupleDesc::sqlarkExplodedOffsets() in exp/exp_tuple_desc.cpp

    if (dataType.getSQLnullHdrSize() > 0)
      memmove(&decodeBuf[resultOffset - dataType.getPrefixSize()], 
                        decodeBuf, dataType.getPrefixSize());
    result =
      new (heap) 
      ConstValue(&dataType,
                 (void *) &(decodeBuf[resultOffset - 
                                      dataType.getPrefixSize()]),
                 resultLength+dataType.getPrefixSize(),
                 NULL,
                 heap);
  }

  return result;
}

ItemExpr * ItmLagOlapFunction::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
    ItemExpr *result = NULL;

    if (derivedNode == NULL)
    {
        switch (getArity()) { 
           case 2:
               result = new (outHeap) ItmLagOlapFunction(child(0), child(1));
               break;
           case 3:
               result = new (outHeap) ItmLagOlapFunction(child(0), child(1), child(2));
               break;
           default:
               CMPASSERT(FALSE);
        }
    }
    else              
        result = derivedNode;                 

  return ItmSeqOlapFunction::copyTopNode(result, outHeap);
}

ItmLeadOlapFunction::~ItmLeadOlapFunction() {}

ItemExpr * 
ItmLeadOlapFunction::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    {
     switch (getArity()) { 
      case 2:
         result = new (outHeap) ItmLeadOlapFunction(child(0), child(1));
         break;
      
      case 1:
      default:
         result = new (outHeap) ItmLeadOlapFunction(child(0));
         break;
     }
    }
  else              
    result = derivedNode;                 

  ((ItmLeadOlapFunction*)result)->setOffset(getOffset());

  return ItmSeqOlapFunction::copyTopNode(result, outHeap);
}

NABoolean ItmLeadOlapFunction::hasEquivalentProperties(ItemExpr * other)
{
  if (other == NULL)
    return FALSE;

  if (getOperatorType() != other->getOperatorType() ||
        getArity() != other->getArity())
    return FALSE;

  //return getOffsetExpr()->hasEquivalentProperties(((ItmLeadOlapFunction*)other)->getOffsetExpr());
  return TRUE;
}

ItemExpr *ItmLeadOlapFunction::transformOlapFunction(CollHeap *heap)
{
   return this;
}

SplitPart::~SplitPart() {}

ItemExpr * SplitPart::copyTopNode(ItemExpr *derivedNode, CollHeap *outHeap)
{
      ItemExpr *result = NULL;
      if (derivedNode == NULL)
        result = new (outHeap) SplitPart(child(0), child(1), child(2));
      else
        result = derivedNode;

     return BuiltinFunction::copyTopNode(result, outHeap);
}

