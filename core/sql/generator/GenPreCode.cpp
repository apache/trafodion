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
 * File:         GenPreCode.C
 * Description:  Fixes up the query tree before code generation.
 *               This is the post-opt and pre-gen stage.
 * Created:      4/15/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS   // must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "Platform.h"
#include <math.h>
#include "OperTypeEnum.h"

#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "AllRelExpr.h"
#include "RelPackedRows.h"
#include "Generator.h"
#include "GenExpGenerator.h"
#include "dfs2rec.h"
#include "vegrewritepairs.h"
#include "exp_clause_derived.h"
#include "keycolumns.h"
#include "ValueDesc.h"
#include "BindWA.h"
#include "TriggerDB.h"
#include "Cost.h"
#include "CostMethod.h"
#include "ItmFlowControlFunction.h"
#include "UdfDllInteraction.h"
#include "StmtDDLNode.h"

#include "NATable.h"
#include "NumericType.h"

#include "CmpStatement.h"
#include "OptimizerSimulator.h"
#include "ItemFunc.h"
#include "ControlDB.h"
#include "CmpSeabaseDDL.h"
#include "NAExecTrans.h"
#include "exp_function.h"
#include "SqlParserGlobals.h"      // must be last #include

extern ItemExpr * buildComparisonPred (	ItemExpr *, ItemExpr *, ItemExpr *,
					OperatorTypeEnum,
					NABoolean specialNulls=FALSE //++MV - Irena
				      );


// -----------------------------------------------------------------------
// generateKeyExpr()
//
// This method is used by the code generator for building expressions
// that are of the form <key column> = <value> for each key column.
//
// Parameters:
//
// const ValueIdSet & externalInputs
//    IN : The set of values that are available here and can be
//         used for replacing any wildcards that appear in the
//         listOfKeyValues.
//
// const ValueIdList & listOfKeyColumns
//    IN : A read-only reference to the list of key columns
//         corresponding to which certain key values have
//         been chosen.
//
// const ValueIdList & listOfKeyValues
//    IN : A read-only reference to a list of key values that
//         are chosen for the corresponding listOfKeyColumns.
//         Values for missing key columns have already been
//         computed and supplied in this list.
//
// ValueIdList & listOfKeyExpr
//    OUT: An assignment expression of the form <key column> = <value>
//         for each key column.
//
// -----------------------------------------------------------------------
static void generateKeyExpr(const ValueIdSet & externalInputs,
			    const ValueIdList & listOfKeyColumns,
			    const ValueIdList & listOfKeyValues,
                            ValueIdList & listOfKeyExpr,
			    Generator* generator,
                            NABoolean replicatePredicates = FALSE)
  {
  BiRelat * keyExpr;
  CollIndex keyCount = listOfKeyColumns.entries();

  for (CollIndex keyNum = 0; keyNum < keyCount; keyNum++)
    {
      // Build the assignment expression.
      ItemExpr *ieKeyVal = listOfKeyValues[keyNum].getItemExpr()
	->replaceVEGExpressions(externalInputs, externalInputs,
                                FALSE, NULL, replicatePredicates);
      ItemExpr *ieKeyCol = listOfKeyColumns[keyNum].getItemExpr();
      ValueId KeyColId = ieKeyCol->getValueId();

      keyExpr = new(generator->wHeap()) BiRelat(ITM_EQUAL,
						ieKeyCol,
						ieKeyVal);

      // Bind it so potential incompatible data type issues are handled
      keyExpr->setSpecialNulls(TRUE);  // allow NULL ieKeyVal
      keyExpr->bindNode(generator->getBindWA());

      // Synthesize its type for and assign a ValueId to it.
      keyExpr->synthTypeAndValueId();

      // INsert it in the list of key expressions
      listOfKeyExpr.insertAt(keyNum, keyExpr->getValueId());
    } // end For Loop

} // static generateKeyExpr()

static NABoolean processConstHBaseKeys(Generator * generator,
                                       RelExpr *relExpr,
                                       const SearchKey *skey,
                                       const IndexDesc *idesc,
                                       const ValueIdSet &executorPreds,
                                       NAList<HbaseSearchKey*> &mySearchKeys,
                                       ListOfUniqueRows &listOfUpdUniqueRows,
                                       ListOfRangeRows  &listOfUpdSubsetRows)
{
  if (! skey)
    return TRUE;

  // convert built-in search key to entries with constants, if possible
  if (skey->areAllKeysConstants(TRUE))
    {
      ValueIdSet nonKeyColumnSet;
      idesc->getNonKeyColumnSet(nonKeyColumnSet);

      // seed keyPreds with only the full key predicate from skey
      ValueIdSet keyPreds = skey->getFullKeyPredicates();

      // include executorPreds and selection predicates 
      // but exclude the full key predicates.
      ValueIdSet exePreds;

      exePreds += executorPreds;
      exePreds += relExpr->getSelectionPred();
      exePreds.subtractSet(keyPreds);

      ValueId falseConst = NULL_VALUE_ID;
      if (exePreds.containsFalseConstant(falseConst))
        keyPreds += falseConst;

      HbaseSearchKey::makeHBaseSearchKeys(
           skey,
           skey->getIndexDesc()->getIndexKey(),
           skey->getIndexDesc()->getOrderOfKeyValues(),
           relExpr->getGroupAttr()->getCharacteristicInputs(),
           TRUE, /* forward scan */
           keyPreds,
           nonKeyColumnSet,
           idesc,
           relExpr->getGroupAttr()->getCharacteristicOutputs(),
           mySearchKeys);

      // Include any remaining key predicates that have not been 
      // picked up (to be used as the HBase search keys).
      exePreds += keyPreds;

      if(falseConst != NULL_VALUE_ID)
      {
	for (CollIndex i = 0; i<mySearchKeys.entries(); i++ ) 
        {
          HbaseSearchKey* searchKey = mySearchKeys[i];
	  searchKey->setIsFalsePred(TRUE);
	}
      }

      TableDesc *tdesc = NULL;
      if (mySearchKeys.entries()>0)
        {
          switch (relExpr->getOperatorType())
            {
            case REL_HBASE_ACCESS:
              {
                HbaseAccess *hba = static_cast<HbaseAccess *>(relExpr);
                hba->setSearchKey(NULL);
                hba->executorPred() = exePreds;
                tdesc = hba->getTableDesc();
              }
              break;

            case REL_HBASE_DELETE:
              {
                HbaseDelete *hbd = static_cast<HbaseDelete *>(relExpr);
                hbd->setSearchKey(NULL);
                hbd->beginKeyPred().clear();
                hbd->endKeyPred().clear();
                hbd->executorPred() = exePreds;
                tdesc = hbd->getTableDesc();
              }
              break;

            case REL_HBASE_UPDATE:
              {
                HbaseUpdate *hbu = static_cast<HbaseUpdate *>(relExpr);
                hbu->setSearchKey(NULL);
                hbu->beginKeyPred().clear();
                hbu->endKeyPred().clear();
                hbu->executorPred() = exePreds;
                tdesc = hbu->getTableDesc();
              }
              break;

            default:
              CMPASSERT(tdesc); // unsupported operator type
              break;
            }  // switch
          relExpr->selectionPred().clear();
        }

      if (HbaseAccess::processSQHbaseKeyPreds(generator, 
					      mySearchKeys,
					      listOfUpdUniqueRows,
					      listOfUpdSubsetRows))
	return FALSE;

    } // key uses all constants
  return TRUE;
}

//
// replaceVEGExpressions1() - a helper routine for ItemExpr::replaceVEGExpressions()
//
// NOTE: The code in this routine came from the previous version of 
//       ItemExpr::replaceVEGExpressions().   It has been pulled out
//       into a separate routine so that the C++ compiler will produce
//       code that needs signficantly less stack space for the 
//       recursive ItemExpr::replaceVEGExpressions() routine.
//
ItemExpr * ItemExpr::replaceVEGExpressions1( VEGRewritePairs* lookup )
{
   // see if this expression is already in there
   ValueId rewritten;
   if (lookup->getRewritten(rewritten /* out */, getValueId()))
   {
     if (rewritten == NULL_VALUE_ID)
       return NULL;
     else
       return rewritten.getItemExpr();
   }
   return (ItemExpr *)( (char *)(NULL) -1 ) ;
}

//
// replaceVEGExpressions2() - a helper routine for ItemExpr::replaceVEGExpressions()
//
// NOTE: The code in this routine came from the previous version of
//       ItemExpr::replaceVEGExpressions().   It has been pulled out
//       into a separate routine so that the C++ compiler will produce
//       code that needs signficantly less stack space for the
//       recursive ItemExpr::replaceVEGExpressions() routine.
//
void ItemExpr::replaceVEGExpressions2( Int32 index 
                                     , const ValueIdSet& availableValues
                                     , const ValueIdSet& inputValues
                                     ,       ValueIdSet& currAvailableValues
                                     , const GroupAttributes * left_ga
                                     , const GroupAttributes * right_ga
                                     )
{
   // If we have asked that the EquiPredicate resolve 
   // each child of the equipred by available values from the 
   // respectively input GAs, make sure we pick the right one.

   // First we find out what GA covers the current EquiPred child
   // we are processing (0 or 1), and pick the one that covers, unless
   // both GAs do. If both GAs cover, the just make sure we pick a 
   // different one for each child. The hash join will later fix up
   // the predicate expression to match its children.
   // If none of the GAs covers, we have a problem...

   // This fix was put in to solve solution: 10-100722-1962

   ValueIdSet dummy;

   NABoolean leftGaCovers = left_ga->covers(child(index)->getValueId(),
                                             inputValues,
                                             dummy);
   NABoolean rightGaCovers = right_ga->covers(child(index)->getValueId(),
                                             inputValues,
                                             dummy);

   if (leftGaCovers == FALSE && rightGaCovers == FALSE)
     {
       // for the moment it is assumed that this code is only
       // executed for hash and merge joins, and in general each 
       // side of the expression should be coverd by a child.
       // So if we have neither, we have a problem ..
       cout << "Unable to pick GA to use: " << getArity() << endl;
       CMPASSERT(FALSE);
     }
   else
     {
       const GroupAttributes *coveringGa =  NULL;
       currAvailableValues.clear();
       currAvailableValues += inputValues;
       if (leftGaCovers && rightGaCovers)
         coveringGa = (index == 0 ? left_ga : right_ga);
       else
         coveringGa = (leftGaCovers ? left_ga : right_ga);

       currAvailableValues += coveringGa->getCharacteristicOutputs();
       
     }
}

// -----------------------------------------------------------------------
// ItemExpr::replaceVEGExpressions()
// It performs a top-down, left-to-right tree walk in the ItemExpr tree
// and expands any wildcards (VEGReference or VEGPredicate expressions)
// by replacing them with an expression that belongs to the
// availableValues.
// IF isKeyPredicate is TRUE then the ItemExpr is a KeyPredicate:
// A KeyPredicate is of a restricted form. If we are here it is
// because the predicate is a KeyPredicate. Then, it must satisfy
// very specific characteristics (see Key::isAKeyPredicate(...))
// for instance, one of its sides must be a key column
// This method *guarantees* that a key predicate will be
// generated from the rewritten predicate (i.e. we avoid
// cases like VegRef{T1.A, 2} > 7 being generated like
// 2 > 7 when T1.A is a key column.
// -----------------------------------------------------------------------
ItemExpr * ItemExpr::replaceVEGExpressions
                        (const ValueIdSet& availableValues,
                         const ValueIdSet& inputValues,
			 NABoolean thisIsAnMdamKeyPredicate,
			 VEGRewritePairs* lookup,
			 NABoolean replicateExpression,
                         const ValueIdSet * joinInputAndPotentialOutput,
                         const IndexDesc * iDesc,
                         const GroupAttributes * left_ga,
                         const GroupAttributes * right_ga)
{
  // ---------------------------------------------------------------------
  // If this expression has already been resolved because it exists in
  // availableValues, the replacement of VEGReferences is not required.
  // ---------------------------------------------------------------------
  if (availableValues.contains(getValueId()))
    return this;  // terminate processing

  ItemExpr* iePtr = this;

  if (lookup && replicateExpression)  // if lookup table is present
    {
       ItemExpr* tmpIePtr = ItemExpr::replaceVEGExpressions1( lookup ) ;
       if ( tmpIePtr != (ItemExpr *)( (char *)(NULL) -1 ) )
          return tmpIePtr ;
    };

  if (replicateExpression)
    iePtr = copyTopNode(0, CmpCommon::statementHeap());
  // virtual copy constructor

  // -----------------------------------------------------------------------
  // In the case of mdam key predicates we need to be careful with
  // binary operators whose child is a VegRef that contains both a
  // key column and a constant because the rewrite logic for VEGRef
  // favors the generation of constants over other ItemExprs. In
  // MDAM we *need* to generate the key column and not the constant.
  // With the gated logic below we ensure this.
  // -----------------------------------------------------------------------

  if (thisIsAnMdamKeyPredicate)
    {

#if DEBUG
      // at the moment it is assumed the left and right ga's are only
      // used for hash/merge joins equijoin predicates and with the
      // mdamKeyPredicate flag turned off. If this assumption is no longer
      // true we need to add some additional code in this "if" clause.
      GENASSERT(left_ga == NULL && right_ga == NULL);
#endif

      switch (getArity())
	{
	case 0: // const, VEGRef, and VEGPred have arity 0
	  break; // If it reached here it means that
	  // the ItemExpr does not need to do any special
	  // processing for this operator (i.e. a constant)
	  // VEG predicates should never reach here
	case 1: // Example: T1.A IS NULL
	  {
	    ItemExpr *newChild;
	    // the child must be a key column:
	    newChild =
	      child(0)->replaceVEGExpressions(availableValues
					      ,inputValues
					      ,TRUE // no constants!
					      ,lookup
					      ,replicateExpression
                                              ,joinInputAndPotentialOutput
                                       ,iDesc
					      );
	    if (newChild != iePtr->child(0))
	      {
		if (replicateExpression)
		  iePtr = iePtr->copyTopNode(NULL, CmpCommon::statementHeap());
		iePtr->child(0) = newChild;
	      }
	  }
	  break;
	case 2:
	case 3:
	  {

	    // Rewrite children (one of them MUST be a key column, the
	    // other MUST be a constant or a host var)
	    ItemExpr
	      *leftChild = NULL,
	      *rightChild = NULL,
	      *thirdChild = NULL;
	    OperatorTypeEnum newOperType = getOperatorType();

	    if ((child(0)->getOperatorType() == ITM_VEG_REFERENCE)
		OR
		(child(1)->getOperatorType() == ITM_VEG_REFERENCE))
	      {

		//---------------------------------------------------------
		// Assume we have an expression of
                // the form VegRef{T1.A, 2} > 7
		//------------------------------------------------------

		// Force the generation of a key column by
		// telling replacevegexprs not to generate them:
		leftChild =
		  child(0)->replaceVEGExpressions(availableValues
						  ,inputValues
						  ,TRUE // want key col
						  ,lookup
						  ,replicateExpression
                                                  ,joinInputAndPotentialOutput
                                     ,iDesc
                                                  );
                // generate a constant in this branch
		rightChild =
		  child(1)->replaceVEGExpressions(availableValues
						  ,inputValues
						  ,FALSE // want constant
						  ,lookup
						  ,replicateExpression
                                                  ,joinInputAndPotentialOutput
                                                  ,iDesc
                                                  );

		// However, the above will fail if the predicate is
		// of the form
                // 7 < VegRef{T1.A,2}, thus, if it failed, redrive with
		// the roles reversed:
		if (leftChild == NULL OR rightChild == NULL)
		  {
		    leftChild =
		      child(1)->replaceVEGExpressions(availableValues
						      ,inputValues
						      ,TRUE // want constant
						      ,lookup
						      ,replicateExpression
                                                      ,joinInputAndPotentialOutput
                                         ,iDesc
                                                      );
		    rightChild =
		      child(0)->replaceVEGExpressions(availableValues
						      ,inputValues
						      ,FALSE // want key col
						      ,lookup
						      ,replicateExpression
                                                      ,joinInputAndPotentialOutput
                                         ,iDesc
                                                      );

		    // We have reversed the operands, reverse
		    // the operator if it is a greater/eq BiRelat operator:
		    switch(getOperatorType())
		      {
		      case ITM_LESS:
		      case ITM_LESS_EQ:
		      case ITM_GREATER:
		      case ITM_GREATER_EQ:
			// need to reverse!
			newOperType =
			  ((BiRelat*)iePtr)->getReverseOperatorType();
			break;
		      }

		  } // if need to reverse operands

		// now we must have succeeded!
		CMPASSERT(leftChild != NULL && rightChild != NULL);

	      } // if one of the children of the operator is a reference
	    else
	      {
		// No children are references, normal rewrite:

		leftChild =
		  child(0)->replaceVEGExpressions(availableValues,
						  inputValues,
						  FALSE, // constants OK
						  lookup,
						  replicateExpression,
                                      joinInputAndPotentialOutput,
                                      iDesc);
		rightChild =
		  child(1)->replaceVEGExpressions(availableValues,
						  inputValues,
						  FALSE, // constants OK
						  lookup,
						  replicateExpression,
                                      joinInputAndPotentialOutput,
                                      iDesc);

		CMPASSERT(leftChild != NULL && rightChild != NULL);

	      }

	    if (getArity() == 3)
	      { // rewrite the exclusion part of the PA key predicate:
		thirdChild =
		  child(2)->replaceVEGExpressions(availableValues,
						  inputValues,
						  thisIsAnMdamKeyPredicate,
						  lookup,
						  replicateExpression,
                                      joinInputAndPotentialOutput,
                                      iDesc);
	      }

	    if (iePtr->child(0) != (void *)leftChild OR
		iePtr->child(1) != (void *)rightChild OR
		(thirdChild AND iePtr->child(2) != (void *)thirdChild) OR
		iePtr->getOperatorType() != newOperType)
	      {
		// we have to change data members, make a copy of the
		// node if other users may share this node
		if (replicateExpression)
		  iePtr = iePtr->copyTopNode(NULL, CmpCommon::statementHeap());

		// Set the left and right children of the iePtr
		// to their rewritten nodes:
		// $$ What happens to all those nodes that were
		// $$ replicated and the rewrite failed?
		iePtr->child(0) = leftChild;
		iePtr->child(1) = rightChild;
		if (thirdChild)
		  iePtr->child(2) = thirdChild;
                iePtr->setOperatorType(newOperType);
	      }
	    break;
	  } // case 2, case 3

	default:
	  // $$ modify this when predicates of arity > 3 come into
	  // $$ existance
	  cout << "Invalid arity: " << getArity() << endl;
	  CMPASSERT(FALSE); // No predicates of arity > 3 (so far)
	}
    }
  else // ItemExpr is not an mdam key predicate, go ahead with the rewrite:
    for (Lng32 index = 0; index < getArity(); index++)
      {
        ValueIdSet currAvailableValues(availableValues);

        if (left_ga != NULL && 
            right_ga != NULL && 
            getArity() == 2 )
          {
            ItemExpr::replaceVEGExpressions2( index 
                                            , availableValues
                                            , inputValues
                                            , currAvailableValues
                                            , left_ga
                                            , right_ga
                                            ) ;
          }

	ItemExpr *newChild = child(index)->replaceVEGExpressions(
             currAvailableValues,
	     inputValues,
	     FALSE, // this is not a key predicate
	     lookup,
	     replicateExpression,
             joinInputAndPotentialOutput,
             iDesc);

        if ( newChild->isPreCodeGenNATypeChanged())
           iePtr->setpreCodeGenNATypeChangeStatus();

	// is the result a different ItemExpr or does iePtr not point to
	// the (possibly unchanged) result yet?
	if (iePtr->child(index) != (void *)newChild)
	  {
	    if (iePtr == this AND replicateExpression)
	      {
		// don't change "this" if it may be shared, make a
		// copy instead and also copy the unchanged children
		// so far
		iePtr = iePtr->copyTopNode(NULL, CmpCommon::statementHeap());
		for (Int32 j = 0; j < index; j++)
		  iePtr->child(j) = this->child(j);
	      }
	    iePtr->child(index) = newChild;
	  }
      }

  if(lookup && replicateExpression && iePtr != this)
    {
      iePtr->synthTypeAndValueId(FALSE);
      lookup->insert(getValueId(), iePtr->getValueId());
    }

  return iePtr;
} // ItemExpr::replaceVEGExpressions()

// -----------------------------------------------------------------------
// ValueIdUnion::replaceVEGExpressions()
// The parameter replicateExpression is ignored because the
// ValueIdUnion implements a special policy for rewriting
// an ItemExpr, in that it manages three sets of values.
// -----------------------------------------------------------------------
ItemExpr * ValueIdUnion::replaceVEGExpressions
                            (const ValueIdSet& availableValues,
			     const ValueIdSet& inputValues,
			     NABoolean thisIsAnMdamKeyPredicate,
			     VEGRewritePairs* lookup,
                             NABoolean replicateExpression,
                             const ValueIdSet * joinInputAndPotentialOutput,
                             const IndexDesc * iDesc,
                             const GroupAttributes * left_ga,
                             const GroupAttributes * right_ga)

{
  CMPASSERT(NOT thisIsAnMdamKeyPredicate); // sanity check
  // we are ignoring the replicateExpression and 
  // joinInputAndPotentialOutput flags ..

  ValueIdUnion * viduPtr = (ValueIdUnion *)this;

  // ---------------------------------------------------------------------
  // If this expression has already been resolved because it exists in
  // availableValues, the replacement of VEGExpressions is not required.
  // ---------------------------------------------------------------------

  if (availableValues.contains(getValueId()) )
    return this;

  for(CollIndex i = 0; i < entries(); i++) {
    viduPtr->
      setSource(i,
		(viduPtr->getSource(i).getItemExpr()
		 ->replaceVEGExpressions(availableValues,inputValues,
                                   thisIsAnMdamKeyPredicate,lookup, 
                                   FALSE, /* replicateExpression default */ 
                                   NULL,/*joinInputAndPotentialOutput default*/
                                   iDesc,
                                   left_ga,
                                   right_ga))
		->getValueId());
  }

  // If the result is not this ValueIdUnion
  if (viduPtr->getResult() != viduPtr->getValueId())
    viduPtr->setResult((viduPtr->getResult().getItemExpr()
			->replaceVEGExpressions(availableValues,
						inputValues,
						thisIsAnMdamKeyPredicate,
                                                  lookup,
                                                  FALSE,/*replicateExpression*/
                                                  NULL, /*joinInputAndPotentialOutput*/ 
                                                  iDesc,
                                                  left_ga,
                                                  right_ga))
		       ->getValueId());

  return this;
} // ValueIdUnion::replaceVEGExpressions()


// -----------------------------------------------------------------------
// VEGPredicate::replaceVEGExpressions()
// The parameter replicateExpression is ignored because the
// VEGPredicate implements a special policy for rewriting
// an ItemExpr. The policies are implemented by replaceVEGPredicate().
// -----------------------------------------------------------------------
ItemExpr * VEGPredicate::replaceVEGExpressions
                            (const ValueIdSet& availableValues,
			     const ValueIdSet& inputValues,
			     NABoolean /* thisIsAnMdamKeyPredicate*/,
			     VEGRewritePairs* lookup,
			     NABoolean /*replicateExpression*/,
                             const ValueIdSet * joinInputAndPotentialOutput,
                             const IndexDesc * iDesc,
                             const GroupAttributes * /* left_ga */,
                             const GroupAttributes * /* right_ga */)
{
  // we ignore the thisIsAnMdamKeyPredicate flag, and so we also ignore the
  // iDesc for VEGPredicates. No need to guarantee a keyColumn.

  return replaceVEGPredicate(availableValues,inputValues,lookup,joinInputAndPotentialOutput);

} // VEGPredicate::replaceVEGExpressions()


// -----------------------------------------------------------------------
// VEGReference::replaceVEGExpressions()
// The parameter replicateExpression is ignored because the
// VEGReference implements a special policy for rewriting
// an ItemExpr. The policies are implemented by replaceVEGReference().
// -----------------------------------------------------------------------
ItemExpr * VEGReference::replaceVEGExpressions
                            (const ValueIdSet& availableValues,
			     const ValueIdSet& inputValues,
			     NABoolean thisIsAnMdamKeyPredicate,
			     VEGRewritePairs* /*lookup*/,
			     NABoolean /*replicateExpression*/,
                             const ValueIdSet * joinInputAndPotentialOutput,
                             const IndexDesc * iDesc,
                             const GroupAttributes * /* left_ga */ ,
                             const GroupAttributes * /* right_ga */ )
{
  // we ignore the replicateExpression, lookup and 
  // joinInputAndPotentialOutput parameters.

  return replaceVEGReference(availableValues,inputValues,
                             thisIsAnMdamKeyPredicate, iDesc);

} // VEGReference::replaceVEGExpressions()

// -----------------------------------------------------------------------
// ItemExpr::replaceOperandsOfInstantiateNull()
// This method is used by the code generator for replacing the
// operands of an ITM_INSTANTIATE_NULL with a value that belongs
// to availableValues.
// -----------------------------------------------------------------------
void ItemExpr::replaceOperandsOfInstantiateNull(
                                           const ValueIdSet & availableValues,
                                           const ValueIdSet & inputValues)
{
  switch (getOperatorType())
  {
    case ITM_INSTANTIATE_NULL:
    {
      child(0) = child(0)->replaceVEGExpressions(availableValues,inputValues);
      break;
    }

    default:
    {
      for (Lng32 i = 0; i < getArity(); i++)
      {
        child(i) = child(i)->replaceVEGExpressions(availableValues,
                                                   inputValues);
      }
      break;
    }
  }

} // ItemExpr::replaceOperandsOfInstantiateNull()

// -----------------------------------------------------------------------
// VEG::setBridgeValue()
// -----------------------------------------------------------------------
void VEG::setBridgeValue(const ValueId & bridgeValueId)
{
  bridgeValues_ += bridgeValueId;
} // VEG::setBridgeValue()

// -----------------------------------------------------------------------
// VEG::markAsReferenced()
// Add a member of the set to the referenced values set to indicate
// that it has been used (at least once) in a "=" predicate that
// was generated by the code generator.
// -----------------------------------------------------------------------
void VEG::markAsReferenced(const ValueId & vid)
{
  referencedValues_ += vid;
  switch (vid.getItemExpr()->getOperatorType())
    {
    case ITM_INDEXCOLUMN:
      // Also add the ValueId of the column from the base table, which is
      // used as the key column for an index.
      referencedValues_ += ((IndexColumn *)(vid.getItemExpr()))
	                      ->getDefinition();
      break;
    default:
      break;
    }

} // VEG::markAsReferenced()

// -----------------------------------------------------------------------
// VEGPredicate::replaceVEGPredicate
//
// This method is used by the code generator for replacing a
// reference to a VEGPredicate with an tree of equality predicates.
// Each equality predicate is between two values that belong to
// the VEG as well as to availableValues.
//
// Terminology :
// ***********
// VEG
//   A ValueId Equality Group. It is a set of values such that its members
//   have an equality predicate specified on them.
//
// availableValues
//   This is the set of values that are available at the relational operator
//   with which the VEGPredicate is associated. It is usually the set union
//   of the Charactersitic Inputs of the operator with the Characteristic
//   Outputs of each of its children.
//
// inputValues
//   This is the set of values that is being provided to this node
//   from above, and therefore is constant for each invocation of
//   the operator when executing.
//   This are good values to use to build key predicates.
//
// bridgeValues
//   This is a set of values for which "=" predicates MUST be generated
//   for correctness as well as to guarantee that transitivity is upheld.
//   For example, the following query:
//
//    select ax, by, cx, dy
//      from (select A.x, B.y from A join B on A.x = B.y) T1(ax,by)
//      join (select C.x, D.y from C join D on C.x = D.y) T2(cx,dy)
//        on T1.ax = T2.cx
//
//   shows two "islands" (self-contained pool of rows) defined by the
//   derived tables T1 and T2 respectively. It is possible to deduce
//   that A.x = D.y only after the predicate A.x = C.x has been applied.
//   The values A.x, C.x establish the transitivity between the two
//   islands. Such values are called inter-island links or bridge values.
//
// referencedValues
//   A subset of the members of the VEG. Each member in this set is
//   referenced in at least one "=" predicate that was generated by
//   a call to replaceVEGPredicate.
//
// unboundValues
//   The unbound values of a VEG are those that require an "="
//   predicate to be generated between them. It is given by
//   bridge values union available values intersect members of the VEG.
//
// Note that if the outputs of the join have already been resolved then 
// joinInputAndPotentialOutput should really be joinInputAndOutputValues.
// All potential output values are no longer available, only the resolved
// values. Please see similar comment in Hashjoin::PrecodeGen.
// -----------------------------------------------------------------------
ItemExpr * VEGPredicate::replaceVEGPredicate(const ValueIdSet& origAvailableValues,
                                             const ValueIdSet& origInputValues,
                                             VEGRewritePairs* lookup,
                                             const ValueIdSet * joinInputAndPotentialOutput)
{
  // If we want processing to be idempotent, check to see if we have
  // already written this VEGPredicate.  And if so, return the rewritten
  // result.
  if (lookup)  // if lookup table is present
    {
      // see if this expression is already in there
      ValueId rewritten;
      if (lookup->getRewritten(rewritten /* out */,getValueId()))
	{
	  if (rewritten == NULL_VALUE_ID)
	    return NULL;
	  else
	    return rewritten.getItemExpr();
	}
    };

  // We assume that inputValues is a (perhaps improper) subset of
  // available values. Verify this.
  ValueIdSet scratchPad;
  scratchPad = origInputValues;
  scratchPad -= origAvailableValues;
  GenAssert(scratchPad.isEmpty(),"NOT scratchPad.isEmpty()");

  // Replace VEGReferences in the members of this VEG.
  // Copy values in the set and expand wild cards in the copy.
  ValueIdSet vegMembers;
  vegMembers.replaceVEGExpressionsAndCopy(getVEG()->getAllValues());

  // Constants are not passed as input values but they are available.
  // Have availableValues and availableInputs contain the VEG members
  // that are constant values.
  ValueIdSet availableValues = origAvailableValues;
  ValueIdSet inputValues = origInputValues;

  ValueIdSet vegConstants;
  vegMembers.getConstants(vegConstants);
  availableValues += vegConstants;
  inputValues += vegConstants;

  // If each member of this VEG is referenced in at least one "=" predicate
  // that was generated here and there is only one "unbound" value remaining,
  // then we are done. Terminate the generation of more "=" predicates.
  if ( (vegMembers == getVEG()->getReferencedValues())
       AND (getVEG()->getBridgeValues().entries() < 2) )
    return NULL;

  ItemExpr * rootPtr = NULL;
  // We can only bind those values that are available here.
  ValueIdSet valuesToBeBound = vegMembers;
  valuesToBeBound.intersectSet(availableValues);

  ValueIdSet unReferencedValues = vegMembers;
  unReferencedValues -= getVEG()->getReferencedValues();

    // Compute the set of values that are available, but
    // are already referenced and are not a bridge value.
    scratchPad = valuesToBeBound;
    scratchPad -= unReferencedValues;
    scratchPad -= getVEG()->getBridgeValues();
    valuesToBeBound -= scratchPad;

  // look for an invariant among the input values
  ValueIdSet vegInputs = valuesToBeBound;
  vegInputs.intersectSet(inputValues);

  // If we didn't have any input values that were a member of the
  // VEG then pick the invariant from the bridge Values
  if (vegInputs.isEmpty())
    {
      vegInputs = valuesToBeBound;
      vegInputs.intersectSet(getVEG()->getBridgeValues());
    }

  // If no input values are part of the VEG and there are
  // no available bridge value then just pick any of the
  // remaining (unreferenced) values
  if (vegInputs.isEmpty())
    {
      vegInputs = valuesToBeBound;
    }

  // look for an invariant value
  ValueId iterExprId, invariantExprId;
  NABoolean invariantChosen = FALSE;

  if (NOT vegInputs.isEmpty())
    {
      for (invariantExprId = vegInputs.init();
	   vegInputs.next(invariantExprId);
	   vegInputs.advance(invariantExprId))
	{
	  //check if the item expr is a non-strict constant
	  //a strict constant is somethine like cos(1)
	  //where as cos(?p) can be considered a constant
	  //in the non-strict definition since it remains
	  //constant for a given execution of a query - Solution 10-020912-1647
	  if (invariantExprId.getItemExpr()->doesExprEvaluateToConstant(FALSE))

	    {
              invariantChosen = TRUE;
	      break;
	    }

	} // endfor
      // if invariantExprId does not contain the ValueId of a constant value,
      // then it must be initialized to contain any one value from
      // the input values.
      if (NOT invariantChosen)
	{
          if (vegInputs.entries() <= 1)
            vegInputs.getFirst(invariantExprId);
          else {
            // The  EXISTS query reported in case 10-091027-8459, soln
            // 10-091028-5770 exposed a flaw in this code that used to 
            // implicitly assume that the first element of vegInputs is
            // always a valid choice for an invariantExprId. When replacing
            // a semijoin's VEGPredicate, the invariantExprId must be a
            // member of that semijoin's characteristic output. Otherwise,
            // *Join::preCodeGen hjp.replaceVEGExpressions() will silently
            // delete that equijoin predicate and incorrectly generate a
            // cartesian product.
            scratchPad = vegInputs;
            if (joinInputAndPotentialOutput) {
              // for an outer join, joinInputAndPotentialOutput will have
              // instantiate_null wrappers. intersectSetDeep digs into
              // those wrappers.
              scratchPad.intersectSetDeep(*joinInputAndPotentialOutput);
            }
#ifdef _DEBUG
            // we want to GenAssert here but regress/core/test027 raises
            // a false alarm. So, for now, we don't.
            // GenAssert(!scratchPad.isEmpty(),"vegInputs.isEmpty()");
#endif
            if (scratchPad.isEmpty())
              vegInputs.getFirst(invariantExprId); 
            else
              scratchPad.getFirst(invariantExprId);
          }
	}

      // remove it from further consideration
      valuesToBeBound -= invariantExprId;

    } // endif (NOT vegInputs.isEmpty())
  else // have no values
    {
      // The predicate pushdown logic places predicates on those
      // operators where it knows that values will be available
      // for evaluating the predicate.
      // If you have reached this point because of a bug,
      // ****************************************************************
      // DO NOT EVEN CONSIDER COMMENTING OUT THE FOLLOWING ASSERT.
      // ****************************************************************
      GenAssert(NOT valuesToBeBound.isEmpty(),"valuesToBeBound.isEmpty()");
      // ****************************************************************
      // YOU WILL BE DELIBERATELY MASKING OUT A SERIOUS BUG IF YOU
      // DISABLE THE ASSERT STATEMENT ABOVE. DON'T TOUCH IT!
      // ****************************************************************
    }

  if (valuesToBeBound.entries() >=  1)
    {
      // Replace this reference to the VEG with a tree of '=' predicates.
      for (iterExprId = valuesToBeBound.init();
	   valuesToBeBound.next(iterExprId);
	   valuesToBeBound.advance(iterExprId))
	{
	  rootPtr = buildComparisonPred
			( rootPtr, iterExprId.getItemExpr(),
			  invariantExprId.getItemExpr(), ITM_EQUAL,
			  getSpecialNulls() //++MV - Irena
			);
	  getVEG()->markAsReferenced(iterExprId);
	}
    }
  else
    {
      // We have only the invariant. Generate an IS NOT NULL if it
      // is nullable and has not been compared with someone else.
      // MVs:
      // If specialNulls option is set, nulls are values (null=null)
      // and ITM_IS_NOT_NULL  filters out some valid rows also.
      // For more info on specialNulls -- see <ItemOther.h>
      if (NOT getVEG()->getReferencedValues().contains(invariantExprId) &&
          invariantExprId.getType().supportsSQLnull() &&
	  NOT getVEG()->getVEGPredicate()->getSpecialNulls() // ++MV - Irena
	 )
        {
          rootPtr = new(CmpCommon::statementHeap())
		UnLogic(ITM_IS_NOT_NULL, invariantExprId.getItemExpr());
        }
    }

  // mark as referenced the invariant. Make it the Bridge value
  getVEG()->markAsReferenced(invariantExprId);
  getVEG()->removeBridgeValues(valuesToBeBound);
  getVEG()->setBridgeValue(invariantExprId);

  // Assign a ValueId to the "=" and synthesize the type for the expression.
  if (rootPtr != NULL) {
    rootPtr->synthTypeAndValueId();
  // If there is a lookup table, enter the rewritten tree in the table
  if (lookup)
    {
    if (rootPtr)
      lookup->insert(getValueId(),rootPtr->getValueId());
    else
      lookup->insert(getValueId(),NULL_VALUE_ID);
    }
  }
  // Return the tree of '=' predicates (or NULL)
  return rootPtr;
} // VEGPredicate::replaceVEGPredicate()

// -----------------------------------------------------------------------
// VEGReference::replaceVEGReference
// This method is used by the code generator. for replacing a
// VEGReference with one of its candidate values
// thisIsAnMdamKeyPredicate is FALSE by default. However, when
// key predicates are being rewritten, it should be set to TRUE
// when we need to guarantee that a key column must be generated by
// the veg reference.
// In this case,
// then bridge values MUST NOT be usen because we need to pick either
// a constant or a key column (depending on the child we are
// working on (see ItemExpr::replaceVEGExpressions(...))
// -----------------------------------------------------------------------
ItemExpr *
VEGReference::replaceVEGReference(const ValueIdSet &origAvailableValues,
                                  const ValueIdSet &origInputValues,
                                  NABoolean thisIsAnMdamKeyPredicate,
                                  const IndexDesc *iDesc)
{

  ItemExpr *result = NULL;

  #ifndef _DEBUG
    const NABoolean VEG_DEBUG = FALSE;
  #else
    NABoolean VEG_DEBUG = getenv("VEG_DEBUG") != NULL;
  #endif

  // We assume that inputValues is a (perhaps improper) subset of
  // available values. Verify it.
  ValueIdSet scratchPad;
  scratchPad = origInputValues;
  scratchPad -= origAvailableValues;
  GenAssert(scratchPad.isEmpty(),"NOT scratchPad.isEmpty()");

  // Copy values in the set and expand wild cards in the copy.
  ValueIdSet valuesToBeBound;
  valuesToBeBound.replaceVEGExpressionsAndCopy(getVEG()->getAllValues());

  // Constants are not passed as input values but they are available
  // Have availableValues and availableInputs contain the VEG members
  // that are constant values
  ValueIdSet availableValues = origAvailableValues;
  ValueIdSet inputValues = origInputValues;

  // --------------------------------------------------------------------
  // Don't add constants if the caller don't want them to be generated
  // from this vegref (i.e. when thisIsAnMdamKeyPredicate is TRUE)
  // --------------------------------------------------------------------

  ValueIdSet vegConstants;
  valuesToBeBound.getConstants(vegConstants);
  if (NOT thisIsAnMdamKeyPredicate)
    {
      availableValues += vegConstants;
      inputValues += vegConstants;
    }

  if (VEG_DEBUG)
    {
      NAString av,iv,vb,vr;
      availableValues.unparse(av);
      inputValues.unparse(iv);
      valuesToBeBound.unparse(vb);
      ValueIdSet thisVegRef(getValueId());
      thisVegRef.unparse(vr);
      cout << endl;
      cout << "VEGReference " << getValueId() << " (" << vr << "):" << endl;
      cout << "AV: " << av << endl;
      cout << "IV: " << iv << endl;
      cout << "VB: " << vb << endl;
    }

  // -----------------------------------------------------------------------
  // 
  // The commented out code implements a different resolution strategies
  // for VEGReference. Inputs are no longer favored. This is in order to
  // handle peculiar scenario where a predicate is not pushed down to the
  // right hand side of a NJ even if it's covered because of the special
  // semantics of the NJ itself (left join). The inputs from the operators
  // in the right leg of the NJ shouldn't be used to resolve the output
  // values since the VEGPred which relates the two hasn't been evaluated.
  //
  // This support is not ready yet for FCS, and therefore the code has been
  // commented out.
  // -----------------------------------------------------------------------
#if 0
  // non-input available values:
  ValueIdSet nonInputAvailableValues = availableValues;
  nonInputAvailableValues -= inputValues;
#endif

  // We can only bind those values that are available here.
  valuesToBeBound.intersectSet(availableValues);

#if 0
  // try using nonInputAvailableValues first.
  ValueIdSet nonInputValuesToBeBound = valuesToBeBound;
  nonInputValuesToBeBound.intersectSet(nonInputAvailableValues);

  // try not to use input values since some predicate might not have
  // be evaluated yet.
  if ( (NOT thisIsAnMdamKeyPredicate) AND
       (NOT nonInputValuesToBeBound.isEmpty()) )
  {
    // Try to pick a bridge value.
    ValueIdSet candidateValues = nonInputValuesToBeBound;
    candidateValues.intersectSet(getVEG()->getBridgeValues());

    // If unsuccessful, try to pick any of the remaining unreferenced.
    if (candidateValues.isEmpty())
    {
      candidateValues = nonInputValuesToBeBound;
    }

    CMPASSERT(NOT candidateValues.isEmpty());
    ValueId resultVid;
    candidateValues.getFirst(resultVid);
    return resultVid.getItemExpr();
  }
#endif


  if (thisIsAnMdamKeyPredicate )
    {
      GenAssert(iDesc != NULL,"VEGReference::replaceVEGReference: Mdam KeyPredicates flag requires an iDesc to go with");
      if (iDesc != NULL)
        {
        ValueIdSet keyCols = iDesc->getIndexKey();
  
        for (ValueId exprId = keyCols.init();
             keyCols.next(exprId);
             keyCols.advance(exprId))
          {
   
            // pick the first value - assuming it is the key column..
            if (valuesToBeBound.contains(exprId)) 
              {
                result = exprId.getItemExpr();
                break;
              }
          }
        }

      if (result && NOT (result->getValueId().getType() == getValueId().getType()) )
          result->setpreCodeGenNATypeChangeStatus();

      return result; // A null is fine here.

    }

  // look for an invariant among the input values
  ValueIdSet vegInputs = valuesToBeBound;
  vegInputs.intersectSet(inputValues);

  // If we didn't have any input values that were a member of the
  // VEG then pick the invariant from the bridge Values
  // Do not use bridge values for key predicates:
  if ((NOT thisIsAnMdamKeyPredicate) && vegInputs.isEmpty())
    {
      vegInputs = valuesToBeBound;
      vegInputs.intersectSet(getVEG()->getBridgeValues());
      if (VEG_DEBUG)
	{
	  NAString vb,br;
	  valuesToBeBound.unparse(vb);
	  // Stupid, ValueIdSet::unparse should be declared const;
	  // for now, just cast away constness...
	  ValueIdSet(getVEG()->getBridgeValues()).unparse(br);
	  cout << "VB: " << vb << endl;
	  cout << "BR: " << br << endl;
	}
    }

  // If no input values are part of the VEG and there are
  // no available bridge value then just pick any of the
  // remaining (unreferenced) values
  if (vegInputs.isEmpty())
    {
      vegInputs = valuesToBeBound;
    }

  // look for a constant value
  ValueId invariantExprId;
  NABoolean invariantChosen = FALSE;

  if (NOT vegInputs.isEmpty())
    {
      for (invariantExprId = vegInputs.init();
	   vegInputs.next(invariantExprId);
	   vegInputs.advance(invariantExprId))
	{
	  //check if the item expr is a non-strict constant
	  //a strict constant is somethine like cos(1)
	  //where as cos(?p) can be considered a constant
	  //in the non-strict definition since it remains
	  //constant for a given execution of a query - Solution 10-020912-1647
	  if (invariantExprId.getItemExpr()->doesExprEvaluateToConstant(FALSE))
	    {
              invariantChosen = TRUE;
	      break;
	    }

	} // endfor
      // if invariantExprId does not contain the ValueId of a constant value,
      // then it must be initialized to contain any one value from
      // the input values.
      if (NOT invariantChosen)
	{
	  vegInputs.getFirst(invariantExprId);
	}

      // we found the invariant assign it!
      result = invariantExprId.getItemExpr();
      CMPASSERT(result != NULL);

    } // endif (NOT vegInputs.isEmpty())
  else // have no values
    {
      // It is ok for an MDAM key pred to not have valuesToBeBound because
      // this is how ItemExpr::replaceVEGExpressions guarantees the generation of
      // key predicates. It expects a NULL pointer sometimes
      if (NOT thisIsAnMdamKeyPredicate)
      {
        // If there is a VEGReference to the value then a member of
        // the VEG should be available.
        GenAssert(NOT valuesToBeBound.isEmpty(),"valuesToBeBound.isEmpty()");
      }
    }

  // result can be NULL only if thisIsAnMdamKeyPredicate is TRUE (see note above)
  if (NOT thisIsAnMdamKeyPredicate)
  {
    CMPASSERT(result);
  }

  if (VEG_DEBUG)
  {
    // coverity cid 10004 thinks result may be null but we know it is not.
    // coverity[var_deref_model]
	cout << "Result: " << result->getValueId() << endl;
  }

  // see if NAType has changed, if so need to rebind it
  if (result && NOT (result->getValueId().getType() == getValueId().getType()) )
  {
     result->setpreCodeGenNATypeChangeStatus();
  }
  return result;

} // VEGReference::replaceVEGReference()


// -----------------------------------------------------------------------
// RelExpr::getOutputValuesOfMyChilren()
// Accumulates the characteristic outputs of all my children for
// operators that have one or more children. Returns the
// potential output values for operators that can have no children.
// -----------------------------------------------------------------------
void RelExpr::getOutputValuesOfMyChildren(ValueIdSet & vs) const
{
  ValueIdSet valueMask;
  Lng32 nc = getArity();
  if (nc > 0)
    {
      for (Lng32 i = 0; i < nc; i++)
	{
	  valueMask += child(i)->getGroupAttr()->getCharacteristicOutputs();
	}
    }
  else // if leaf operators, use all available values
    {
      getPotentialOutputValues(valueMask);
    }

  // Copy values in the set and expand wild cards in the copy.
  vs.clear();
  vs.replaceVEGExpressionsAndCopy(valueMask);

} // RelExpr::getOutputValuesOfMyChildren()

// -----------------------------------------------------------------------
// RelExpr::getInputValuesFromParentAndChildren()
// Uses getOutputValuesOfMyChildren() to collect the output values
// and adds the characteristic input values of this operator to them.
// -----------------------------------------------------------------------
void RelExpr::getInputValuesFromParentAndChildren(ValueIdSet & vs) const
{
  getOutputValuesOfMyChildren(vs);
  vs += getGroupAttr()->getCharacteristicInputs();
} // RelExpr::getInputValuesFromParentAndChildren()

// -----------------------------------------------------------------------
// RelExpr::getInputAndPotentialOutputValues()
// Uses getPotentialOutputs() to collect the output values
// and adds the characteristic input values of this operator to them.
// -----------------------------------------------------------------------
void RelExpr::getInputAndPotentialOutputValues(ValueIdSet & vs) const
{
  ValueIdSet potentialOutputValues;
  getPotentialOutputValues(potentialOutputValues);
  potentialOutputValues += getGroupAttr()->getCharacteristicInputs();
  vs.clear();
  vs.replaceVEGExpressionsAndCopy(potentialOutputValues);

} // RelExpr::getInputAndPotentialOutputValues()

// -----------------------------------------------------------------------
// GenericUpdate::replaceVEGExpressionsAndGet...
// -----------------------------------------------------------------------
void GenericUpdate::getInputValuesFromParentAndChildren(ValueIdSet & vs) const
{
  ValueIdSet updTableCols;
  ValueIdSet vs2;

  updTableCols.insertList (getIndexDesc()->getIndexColumns());
  //      updTableCols.insertList(getTableDesc()->getColumnVEGList());
  vs2.replaceVEGExpressionsAndCopy(updTableCols);


  getOutputValuesOfMyChildren(vs);
  vs += getGroupAttr()->getCharacteristicInputs();
  vs += vs2;
} // GenericUpdate::getInputValuesFromParentAndChildren()

// -----------------------------------------------------------------------
// HbaseDelete::replaceVEGExpressionsAndGet...
// -----------------------------------------------------------------------
void HbaseDelete::getInputValuesFromParentAndChildren(ValueIdSet & vs) const
{
  // Do not include IndexColumn as the input values. Otherwise, we will
  // have duplicated predicates in Executor predicate in HbaseDelete.

  getOutputValuesOfMyChildren(vs);
  vs += getGroupAttr()->getCharacteristicInputs();
} // HbaseDelete::getInputValuesFromParentAndChildren()

// -----------------------------------------------------------------------
// RelExpr::preCodeGen()
//
// RelExpr * result
//    OUT: a node that calls preCodeGen for its child should replace
//         that child with the result value. This allows preCodeGen
//         to transform the RelExpr tree. Examples for such trans-
//         formations are additional exchange nodes for repartitioning.
// Generator * generator
//  INOUT: a global work area with useful helper methods
// const ValueIdSet & externalInputs
//     IN: a value id set with values that already have been
//         replaced such that they don't contain VEGies any more.
//         Use this set to replace VEGies for expressions that depend
//         on the characteristic inputs of the node.
// ValueIdSet & pulledNewInputs
//    OUT: a set of value ids that the node wants to add to its
//         characteristic inputs ("pull" from its parent). There are
//         several cases in which we need to add value ids to
//         characteristic inputs during preCodeGen:
//         a) partition input variables for parallel execution,
//         b) the COMMON datetime function which needs to be generated
//            by the root node,
//         c) an "open cursor timestamp" that helps a materialize node
//            to decide whether it can reuse its materialized table.
// -----------------------------------------------------------------------
RelExpr * RelExpr::preCodeGen(Generator * generator,
			      const ValueIdSet & externalInputs,
			      ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  // Check if the pivs of this operator and it's child are the same.
  // If they are not, make them the same.
  replacePivs();

  // Resolve the VEGReferences and VEGPredicates, if any, that appear
  // in the Characteristic Inputs, in terms of the externalInputs.
  getGroupAttr()->resolveCharacteristicInputs(externalInputs);

  // My Characteristic Inputs become the external inputs for my children.
  Int32 nc = getArity();
  for (Int32 index = 0; index < nc; index++)
    {
      ValueIdSet childPulledInputs;

      child(index) = child(index)->preCodeGen(generator,
					      externalInputs,
					      childPulledInputs);

      if (! child(index).getPtr())
	return NULL;

      // process additional input value ids the child wants
      getGroupAttr()->addCharacteristicInputs(childPulledInputs);
      pulledNewInputs += childPulledInputs;
    }

  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  ValueIdSet availableValues;
  getInputAndPotentialOutputValues(availableValues);

  // Rewrite the selection predicates.
  NABoolean replicatePredicates = TRUE;
  selectionPred().replaceVEGExpressions
                     (availableValues,
		      getGroupAttr()->getCharacteristicInputs(),
		      FALSE, // no need to generate key predicates here
		      0 /* no need for idempotence here */,
		      replicatePredicates
		     );

  getGroupAttr()->resolveCharacteristicOutputs
                     (availableValues,
		      getGroupAttr()->getCharacteristicInputs());

  markAsPreCodeGenned();

  return this;
} // RelExpr::preCodeGen

//
// Recuvsively call the method on each RelExpr node, accumulating
// # of rows from each node.
//
void RelExpr::prepareDopReduction(Generator* generator)
{
  pcgEspFragment* currentEspFragmentPCG = generator->getCurrentEspFragmentPCG();

  if ( currentEspFragmentPCG )
     currentEspFragmentPCG->accumulateRows(getEstRowsUsed());

  Int32 nc = getArity();

  for (Lng32 i = 0; i < nc; i++)
  {
     child(i)->prepareDopReduction(generator);
  }
}

void Exchange::prepareDopReduction(Generator* generator)
{
   pcgEspFragment* parentEspFragPCG = generator->getCurrentEspFragmentPCG();

   //
   // Save the current pcg fragment and add myself as the child to it.
   //
   if ( parentEspFragPCG ) {
     parentEspFragPCG->accumulateRows(getEstRowsUsed());
     parentEspFragPCG->addChild(this);
   }

   //
   // Let the global pointer point at my pcg esp fragment (for the
   // fragment rooted at me). Do this only for above-DP2 Exchanges.
   // Note a PA is represented by an Exchange with "execute in Master or ESP"
   // as location. So a PA exchange with a SCAN as a child will have an empty
   // childPcgEsp array.
   //
   generator->setCurrentEspFragmentPCG
     (
       (getPhysicalProperty()->getPlanExecutionLocation() != EXECUTE_IN_DP2)
               ? getEspFragPCG() : NULL
     );

  child(0)->prepareDopReduction(generator);

  //
  // Restore the pcg esp fragment
  //
  generator->setCurrentEspFragmentPCG(parentEspFragPCG);

  // Try to reduce the dop and if it fails, invalidate any proposed
  // dop reductions for this.
  //
  if ( parentEspFragPCG &&
       parentEspFragPCG ->tryToReduceDoP() == FALSE )
     parentEspFragPCG->invalidate();
}

RelExpr * RelRoot::preCodeGen(Generator * generator,
			      const ValueIdSet & /* externalInputs */,
			      ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  // For all the inputVars, if it is with UNKNOWN data type, make it a
  // varchar type. This is from SQL/MP extension. Example query
  // select ?p1 from any-table;
  if (isTrueRoot()) {
    CollIndex i;
    ValueId vid;
    ValueIdList vidList = inputVars();
    for ( i=0; i < vidList.entries(); i++ )
      if ((vid=vidList[i]).getType().getTypeQualifier() == NA_UNKNOWN_TYPE) {
	vid.coerceType(NA_CHARACTER_TYPE);
      }
  }

  // if root has GET_N indication set, insert a FirstN node.
  // Usually this transformation is done in the binder, but in
  // some special cases it is not.
  // For example, if there is an 'order by' in the query, then
  // the Sort node is added by the optimizer. In this case, we
  // want to add the FirstN node on top of the Sort node and not
  // below it. If we add the FirstN node in the binder, the optimizer
  // will add the Sort node on top of the FirstN node. Maybe we
  // can teach optimizer to do this.
  if ((getFirstNRows() != -1) || (getFirstNRowsParam()))
    {
      // As of JIRA TRAFODION-2840, the binder always adds the FirstN,
      // except in the case of output rowsets. So, the only time we 
      // should now be going through this code is a SELECT query using
      // output rowsets + FirstN + ORDER BY. A follow-on JIRA,
      // TRAFODION-2924, will take care of that case and delete this code.
      // (As a matter of design, it is highly undesireable to sometimes
      // create the FirstN in the Binder and sometimes in the Generator;
      // that means that any FirstN-related semantic checks in the 
      // intervening passes will need two completely separate 
      // implementations.)

      RelExpr * firstn = new(generator->wHeap()) FirstN(child(0),
							getFirstNRows(),
							needFirstSortedRows(),   
                                                        getFirstNRowsParam());

      // move my child's attributes to the firstN node.
      // Estimated rows will be mine.
      firstn->setEstRowsUsed(getEstRowsUsed());
      firstn->setMaxCardEst(getMaxCardEst());
      firstn->setInputCardinality(child(0)->getInputCardinality());
      firstn->setPhysicalProperty(child(0)->getPhysicalProperty());
      firstn->setGroupAttr(child(0)->getGroupAttr());
      //10-060516-6532 -Begin
      //When FIRSTN node is created after optimization phase, the cost
      //of that node does not matter.But, display_explain and explain
      //show zero operator costs and rollup cost which confuses the user.
      //Also, the VQP crashes when cost tab for FIRSTN node is selected.
      //So, creating a cost object will fix this.
      //The operator cost is zero and rollup cost is same as it childs.
      Cost* firstnNodecost = new HEAP Cost();
      firstn->setOperatorCost(firstnNodecost);
      Cost* rollupcost = (Cost *)(child(0)->getRollUpCost());
      *rollupcost += *firstnNodecost;
      firstn->setRollUpCost(rollupcost);
      //10-060516-6532 -End

      setChild(0, firstn);

      // reset firstN indication in the root node.
      setFirstNRows(-1);
      setFirstNRowsParam(NULL);
    }

  if (isTrueRoot())
    {
      // Set the internal format to use for the plan being generated ...
      // Checks the CQD COMPRESSED_INTERNAL_FORMAT to decide whether to use
      // SQLARK_EXPLODED_FORMAT or SQLMX_ALIGNED_FORMAT as the internal
      // data format
      // When the CIF CQD is set to SYSTEM we decide whether to use aligned or exploded format
      // as the tuple format for the whole query. In precodeGEn we visit all the copy
      // operators (Hash join, hash group by, exchange and sort) in a query
      // tree and keep a count of the nodes that are in favor of aligned format and those
      // that are in favor of exploded format.
      // The final decision about the tuple format for the whole query will depend on those two
      // numbers. if the number of nodes in favor of aligned format is greater than those
      // in favor of exploded than aligned format is select otherwise exploded is selected
      // The function that determine the format for each of the copy operators + relroot
      // is determineInternalFormat(..) is is called in the precodeGen of the copy operators
      generator->initNNodes();
      isCIFOn_ = FALSE;

      if ((CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) == DF_ON )||
          generator->isFastExtract() ||
          generator->containsFastExtract())
      {
         isCIFOn_ = TRUE;
         generator->setCompressedInternalFormat();
      }
      else
      if ( CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) == DF_OFF )
      {
        generator->setExplodedInternalFormat();
      }
      else
      {
         NABoolean resize = FALSE;
         NABoolean considerBufferDefrag = FALSE;
         ValueIdSet vidSet = child(0)->getGroupAttr()->getCharacteristicOutputs();
         ExpTupleDesc::TupleDataFormat tupleFormat =
                                 generator->determineInternalFormat(
                                                   vidSet,
                                                   this,
                                                   resize,
                                                   RelExpr::CIF_SYSTEM,
                                                   FALSE,
                                                   considerBufferDefrag);

         if (tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
         {
           generator->incNCIFNodes();
         }
         else
         {
           generator->decNCIFNodes();
         }
      }
      //generator->setInternalFormat();

      // Some operators will revert the internal format back to exploded format
      // when they are directly under the root node - such as the top level ESPs,
      // Sort, and HJ operators.
      // This is so there is no bottleneck in the master flipping the data back
      // to exploded format (required for bulk move out).
      child(0)->setParentIsRoot( TRUE );

      // create a list of NATypes corresponding to each entry in the
      // userColumnList_ in RETDesc. Used by generator to convert to
      // this type during output expr code gen.
      // The value ids in userColumnList_ cannot be used as the type
      // corresponding to that value id may change due to VEG transformation
      // in the preCodeGen phase.
      if (getRETDesc()->createNATypeForUserColumnList(CmpCommon::statementHeap()))
	{
	  // error case.
	  GenAssert(FALSE, "error from createNATypeForUserColumnList.");
	}

      if ( (child(0)->getOperatorType() == REL_EXCHANGE) &&
	   (child(0)->child(0)->getOperatorType() == REL_COMPOUND_STMT) )
	{
	  ((Exchange *)((RelExpr *)child(0)))->setDP2TransactionIndicator( TRUE );
	}
    }

  unsigned short prevNumBMOs = 0;
  CostScalar prevBMOsMemoryUsage;

  if (isTrueRoot())
    {
      if (oltOptInfo().oltAnyOpt())
        {
          if (treeContainsEspExchange())
            {
              // turn off oltQueryOptimization if the query plan contains an
              // esp_exchange.
              // 10-070316-3325: childOperType_ = REL_UNARY_DELETE
              // 10-080118-9942: select query contains esp_exchange that is
              // not directly under root.
              oltOptInfo().setOltOpt(FALSE);
            }
          else if (childOperType() == REL_SCAN)
	    {
              // if this was a scan query to start with but is no longer
              // a scan query(which means it got transformed to join, etc),
              // then turn off oltQueryOptimization.
	      RelExpr *childExpr = child(0)->castToRelExpr();

	      if (childExpr->getOperatorType() == REL_FIRST_N)
		childExpr = childExpr->child(0)->castToRelExpr();

              if ((childExpr->getOperatorType() != REL_EXCHANGE) &&
		  (childExpr->getOperatorType() != REL_HBASE_ACCESS))
                oltOptInfo().setOltCliOpt(FALSE);
            }
        } // oltAnyOpt

      *generator->oltOptInfo() = oltOptInfo();
 
      if (generator->oltOptInfo()->oltAnyOpt())
	{
	  // Also, PubSub streams' STREAM_TIMEOUT not handled by opt'd root
          if (getGroupAttr()->isStream())
            {
	      generator->oltOptInfo()->setOltCliOpt(FALSE);
            }


	  if (CmpCommon::getDefault(EID_SPACE_USAGE_OPT) == DF_ON)
	    {
	      generator->setDoEidSpaceUsageOpt(TRUE);
	    }
	  else
	    {
	      generator->setDoEidSpaceUsageOpt(FALSE);
	    }

	  // olt opt not chosen if ALL stats are being collected.
	  // We may support this case later.
          // In case of operator stats, don't disable OLT optimization
          // But, when the query is OLT optimized, switch it to pertable stats
	  if ((generator->computeStats()) &&
	      ((generator->collectStatsType() == ComTdb::ALL_STATS)))
 	    generator->oltOptInfo()->setOltOpt(FALSE);
	  
	  if (CmpCommon::getDefault(OLT_QUERY_OPT) == DF_OFF)
	    generator->oltOptInfo()->setOltOpt(FALSE);

	  // In the case of an embedded insert,
	  // do not execute the query OLT optimized.
	  
	  if (getGroupAttr()->isEmbeddedInsert())
            generator->oltOptInfo()->setOltMsgOpt(FALSE);
	  
#ifdef _DEBUG
	  if (getenv("NO_OLT_QUERY_OPT"))
	    generator->oltOptInfo()->setOltOpt(FALSE);
#endif

	  if (generator->oltOptInfo()->oltEidOpt())
	    {
	      generator->oltOptInfo()->setOltEidLeanOpt(FALSE);

	      if (generator->doEidSpaceUsageOpt())
		{
		  generator->oltOptInfo()->setOltEidLeanOpt(TRUE);
		}
	    }

	  if (CmpCommon::getDefault(OLT_QUERY_OPT_LEAN) == DF_OFF)
	    generator->oltOptInfo()->setOltEidLeanOpt(FALSE);
	} // oltAnyOpt

      // mark exchange operator for maxOneRow optimization.
      RelExpr *childExpr = child(0)->castToRelExpr();
      NABoolean doMaxOneRowOpt = TRUE;
      NABoolean doMaxOneInputRowOpt = FALSE;
      NABoolean firstN = FALSE;
      RelExpr *exchExpr = NULL;
      if (NOT generator->doEidSpaceUsageOpt())
	{
	  doMaxOneRowOpt = FALSE;
	  doMaxOneInputRowOpt = FALSE;
	}
      else
	{
	  doMaxOneRowOpt = TRUE;
	  doMaxOneInputRowOpt = TRUE;
	}

      if (childExpr->getOperatorType() == REL_FIRST_N)
	{
	  firstN = TRUE;
	  if (((FirstN *)childExpr)->getFirstNRows() != 1)
	    doMaxOneRowOpt = FALSE;

	  childExpr = childExpr->child(0)->castToRelExpr();
	}

      if ((childExpr->getOperatorType() != REL_EXCHANGE) ||
	  (childExpr->child(0)->castToRelExpr()->
	   getPhysicalProperty()->getPlanExecutionLocation() != EXECUTE_IN_DP2))
	{
	  doMaxOneRowOpt = FALSE;
	  doMaxOneInputRowOpt = FALSE;
	}
      else
	{
	  exchExpr = childExpr;
	  childExpr = childExpr->child(0)->castToRelExpr();
          if (NOT childExpr->getOperator().match(REL_FORCE_ANY_SCAN))
	    {
	      doMaxOneInputRowOpt = FALSE;
	    }
	  else if (childExpr->getOperatorType() == REL_FILE_SCAN)
	    {
	      FileScan * s = (FileScan *)childExpr;
	      if (NOT firstN)
		doMaxOneRowOpt = FALSE;

	      if ((s->getGroupAttr()->isStream()) ||
		  (s->accessOptions().accessType() == TransMode::SKIP_CONFLICT_ACCESS_))
		{
		  //doMaxOneInputRowOpt = FALSE;
		  //doMaxOneRowOpt = FALSE;
		}
	    }
	}

      if (doMaxOneRowOpt)
	{
	  exchExpr->oltOptInfo().setMaxOneRowReturned(TRUE);
	}

      if (doMaxOneInputRowOpt)
	{
	  exchExpr->oltOptInfo().setMaxOneInputRow(TRUE);
	}

      generator->setUpdErrorInternalOnError(FALSE);

      if (rollbackOnError())
	generator->setUpdErrorOnError(FALSE);
      else
	generator->setUpdErrorOnError(TRUE);

      if (CmpCommon::getDefault(UPD_ABORT_ON_ERROR) == DF_ON)
	generator->setUpdAbortOnError(TRUE);
      else
	generator->setUpdAbortOnError(FALSE);

      if (CmpCommon::getDefault(UPD_PARTIAL_ON_ERROR) == DF_ON)
	generator->setUpdPartialOnError(TRUE);
      else
	generator->setUpdPartialOnError(FALSE);

      if (CmpCommon::getDefault(UPD_SAVEPOINT_ON_ERROR) == DF_ON)
	generator->setUpdSavepointOnError(TRUE);
      else
	generator->setUpdSavepointOnError(FALSE);

      generator->setSkipUnavailablePartition(FALSE);
      if ((childOperType() == REL_SCAN) &&
	  (CmpCommon::getDefault(SKIP_UNAVAILABLE_PARTITION) == DF_ON))
	generator->setSkipUnavailablePartition(TRUE);

      if (avoidHalloween_)
        { 
          // At beginning of preCodeGen, assume DP2Locks will be 
          // used.  The NestedJoin::preCodeGen will change this
          // if its left child is a sort.
          generator->setHalloweenProtection(Generator::DP2LOCKS);
        }

      if (generator->getBindWA()->getUdrStoiList().entries () > 0)
        generator->setAqrEnabled(FALSE);

      // Reset the accumulated # of BMOs for the fragment
      prevNumBMOs = generator->replaceNumBMOs(0);

    } // true root

  // propagate the need to return top sorted N rows to all sort
  // nodes in the query.
  if (needFirstSortedRows() == TRUE)
    {
      needSortedNRows(TRUE);
    }

  // Delete any VEGReference that appear in the Characteristic Inputs.
  // The Characteristic Inputs of the root of the execution plan MUST
  // only contain external dataflow inputs that are provided by the
  // user. The VEGReferences may have been introduced as a side-effect
  // of predicate pushdown. They are redundant in the Characteristic
  // Inputs of the root.
  ValueIdSet availableValues;
  for (ValueId exprId = getGroupAttr()->getCharacteristicInputs().init();
                        getGroupAttr()->getCharacteristicInputs().next(exprId);
                        getGroupAttr()->getCharacteristicInputs().advance(exprId) )
    {
      if (exprId.getItemExpr()->getOperatorType() != ITM_VEG_REFERENCE)
	availableValues += exprId;
    }

  getGroupAttr()->setCharacteristicInputs(availableValues);

  // If this is the root for a parallel extract producer query then
  // there should be an Exchange node immediately below and we need to
  // set a flag in that Exchange.
  if (numExtractStreams_ > 0)
  {
    if (child(0)->getOperatorType() == REL_EXCHANGE)
    {
      Exchange *e = (Exchange *) child(0)->castToRelExpr();
      e->setExtractProducerFlag();
    }
    // fix for soln 10-090506-1407: parallel extract for a union distinct 
    // can sometimes have root->mapvalueidsl->exchange. It should be OK.
    else if (child(0)->getOperatorType() == REL_MAP_VALUEIDS &&
             child(0)->child(0)->getOperatorType() == REL_EXCHANGE)
    {
      Exchange *e = (Exchange *) child(0)->child(0)->castToRelExpr();
      e->setExtractProducerFlag();
    }
  }

  //
  // If there is no hard requirement on #ESPs, reduce the dop based on 
  // the total # of rows processed per ESP. The reduction can modify
  // the number of partitions attribute of the partition function stored
  // in the synthesized physical property of an Exchange operator.
  // 
  // CQD DOP_REDUCTION_ROWCOUNT_THRESHOLD set to 0.0 will disable the 
  // feature.

   float threshold;
   ActiveSchemaDB()->
      getDefaults().getFloat(DOP_REDUCTION_ROWCOUNT_THRESHOLD, threshold);

  if ( threshold > 0.0 && CURRSTMT_OPTDEFAULTS->getRequiredESPs() <= 0 ) {

    generator->setCurrentEspFragmentPCG(NULL); // reset the 'global' 
                                               // to the current esp frag.

    RelExpr::prepareDopReduction(generator);
    RelExpr::doDopReduction();
  }

  // Now walk through the execution plan and initialize it for code generation.
  child(0) = child(0)->preCodeGen(generator,
				  getGroupAttr()->getCharacteristicInputs(),
				  pulledNewInputs);
  if (! child(0).getPtr())
    return NULL;

  if (! RelExpr::preCodeGen(
       generator,
       getGroupAttr()->getCharacteristicInputs(),
       pulledNewInputs))
      return NULL;

  if ( isTrueRoot() && CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) == DF_SYSTEM)
  {
    if (generator->getNCIFNodes()>0)
    {
      isCIFOn_ = TRUE;
      generator->setCompressedInternalFormat();
    }
    else
    {
      generator->setExplodedInternalFormat();
      isCIFOn_ = FALSE;
    }

  }


  // If the RelRoot is marked as a parallel extract producer then the
  // root's child must be an Exchange and the child must also be
  // marked for parallel extract. Even though we checked the type of
  // the child a few lines above, we do it again here because the call
  // to RelExpr::preCodeGen can potentially eliminate Exchange nodes.
  NABoolean extractPlanLooksOK = TRUE;
  if (numExtractStreams_ > 0)
  {
    if (child(0)->getOperatorType() == REL_EXCHANGE)
    {
      Exchange *e = (Exchange *) child(0)->castToRelExpr();
      if (!e->getExtractProducerFlag())
        extractPlanLooksOK = FALSE;
    }
    // fix for soln 10-090506-1407: parallel extract for a union distinct 
    // can sometimes have root->mapvalueidsl->exchange. It should be OK.
    else if (child(0)->getOperatorType() == REL_MAP_VALUEIDS &&
             child(0)->child(0)->getOperatorType() == REL_EXCHANGE)
    {
      Exchange *e = (Exchange *) child(0)->child(0)->castToRelExpr();
      if (!e->getExtractProducerFlag())
        extractPlanLooksOK = FALSE;
    }
    else
    {
      extractPlanLooksOK = FALSE;
    }
    if (!extractPlanLooksOK)
    {
      *CmpCommon::diags() << DgSqlCode(-7004);
      GenExit();
      return NULL;
    }
  }
  
  // Accumulate the values that are provided as inputs by my parent
  // together with the values that are produced as outputs by my
  // children. Use these values for rewriting the VEG expressions.
  getInputValuesFromParentAndChildren(availableValues);

  // Rebuild the computable expressions using a bridge value, if possible
  compExpr().replaceVEGExpressions
                (availableValues, getGroupAttr()->getCharacteristicInputs());
  // Rebuild the required order
  reqdOrder().replaceVEGExpressions
                 (availableValues, getGroupAttr()->getCharacteristicInputs());
  // Rebuild the pkey list
  pkeyList().replaceVEGExpressions
                 (availableValues, getGroupAttr()->getCharacteristicInputs());

  // add internally generated inputs to the input vars and make sure that
  // the root isn't left with "pulled" input values that aren't "internal"
  // inputs (the assert will most likely fire for leftover partition input
  // variables)
  inputVars().insertSet(generator->getInternalInputs());
  pulledNewInputs -= (ValueIdSet) inputVars();
  GenAssert(pulledNewInputs.isEmpty(),"root can't produce these values");

  // Do not rollback on error for INTERNAL REFRESH commands.
  if (isRootOfInternalRefresh())
    {
      generator->setUpdErrorInternalOnError(TRUE);
      generator->setUpdAbortOnError(FALSE);
      generator->setUpdPartialOnError(FALSE);
      generator->setUpdSavepointOnError(FALSE);
    }

  // do not abort transaction for internal compiles, even if abort
  // is needed for this statement.
  // Catman depends on no abort for individual IUD stmts.
  // It aborts the transaction when it gets an error from cli.
  if ( ( CmpCommon::context()->internalCompile() == CmpContext::INTERNAL_MODULENAME ) ||
       ( CmpCommon::statement()->isSMDRecompile() )
     )
    {
      generator->setUpdErrorInternalOnError(TRUE);
      generator->setUpdAbortOnError(FALSE);
      generator->setUpdPartialOnError(FALSE);
      generator->setUpdSavepointOnError(FALSE);
    }

  oltOptInfo().setOltCliOpt(generator->oltOptInfo()->oltCliOpt());

  if ((isTrueRoot()) &&
      (CmpCommon::getDefault(LAST0_MODE) == DF_ON) &&
      (child(0)))
      {
	OperatorTypeEnum op = child(0)->getOperatorType();
	if (op != REL_DESCRIBE &&
	    op != REL_EXPLAIN &&
	    op != REL_DDL &&
	    op != REL_LOCK &&
	    op != REL_UNLOCK &&
	    op != REL_SET_TIMEOUT && 
	    op != REL_STATISTICS &&
	    op != REL_TRANSACTION &&
	    op != REL_EXE_UTIL)
	  {
	    // do not return any rows at runtime.
	    // Setting of -2 tells executor to simulate [last 0]
	    // without having to put [last 0] in the query.
	    setFirstNRows(-2);
	  }
      }

  if (isTrueRoot())
    {
      // if warnings 6008 or 6011 were raised, set missingStats indication.
      if (CmpCommon::diags()->containsWarning(SINGLE_COLUMN_STATS_NEEDED) ||
	  CmpCommon::diags()->containsWarning(SINGLE_COLUMN_STATS_NEEDED_AUTO)) 
      {
	generator->compilerStatsInfo().setMissingStats(TRUE);
      }

      // change the following number(16) to whatever is considered 'large'.
      //#define LARGE_NUMBER_OF_JOINS 16      
      //if (generator->compilerStatsInfo().totalJoins() > LARGE_NUMBER_OF_JOINS)
      //generator->compilerStatsInfo().setLargeNumOfJoins(TRUE);

      // set mandatoryXP indication in generator.
      if (hasMandatoryXP()) 
      generator->compilerStatsInfo().setMandatoryCrossProduct(TRUE);

      // Remember # of BMOs that children's preCodeGen found for my fragment.
      setNumBMOs( generator->replaceNumBMOs(prevNumBMOs) );

      // Compute the total available memory quota for BMOs
      NADefaults &defs               = ActiveSchemaDB()->getDefaults();

      // total per node
      double m = defs.getAsDouble(BMO_MEMORY_LIMIT_PER_NODE_IN_MB) * (1024*1024);

      generator->setBMOsMemoryLimitPerNode(m);

    }


  if (isTrueRoot())
    {
      if (generator->isAqrWnrInsert())
        {
          ExeUtilWnrInsert * wi = new(generator->getBindWA()->wHeap())
            ExeUtilWnrInsert(generator->utilInsertTable(),
                             child(0)->castToRelExpr());
          child(0)->markAsBound();
          wi->bindNode(generator->getBindWA());
          if (generator->getBindWA()->errStatus())
            return NULL;

          // Use the same characteristic inputs and outputs as my child
          wi->setGroupAttr(new(generator->wHeap())
                           GroupAttributes(*(child(0)->getGroupAttr())));

          //pass along some of the  estimates
          wi->setEstRowsUsed(child(0)->getEstRowsUsed());
          wi->setMaxCardEst(child(0)->getMaxCardEst());
          wi->setInputCardinality(child(0)->getInputCardinality());
          wi->setPhysicalProperty(child(0)->getPhysicalProperty());
          wi->setOperatorCost(0);
          wi->setRollUpCost(child(0)->getRollUpCost());

          if (! wi->preCodeGen(generator,
                    getGroupAttr()->getCharacteristicInputs(),
                    pulledNewInputs))
            return NULL;

          child(0) = wi;
        }

    }

  
  // if blob values are being selected out, retrieve them and return them either in file
  // or as a stream
  if (isTrueRoot())
    {
      RETDesc * rd = getRETDesc();
      const ColumnDescList * cdl = rd->getColumnList();
      for (CollIndex i = 0; i < compExpr().entries(); i++)
	{
	  ValueId val_id = compExpr()[i];
	  ItemExpr * expr = val_id.getItemExpr();
	  if ((val_id.getType().isLob()))/* &&
	      ((expr->getOperatorType() == ITM_BASECOLUMN) ||
              (expr->getOperatorType() == ITM_INDEXCOLUMN)))*/
	    {
	      LOBconvertHandle * lc = new(generator->wHeap())
		LOBconvertHandle(val_id.getItemExpr(), LOBoper::STRING_);
	      
	      lc->bindNode(generator->getBindWA());
	      lc->preCodeGen(generator);
	      
	      compExpr().removeAt(i);
	      compExpr().insertAt(i, lc->getValueId());

	      ColumnDesc  *cd = (*cdl)[i];
	      
	      NAColumn * col = cd->getValueId().getNAColumn(TRUE);
              if (col)
                {
                  lc->lobNum() = col->lobNum();
                  lc->lobStorageType() = col->lobStorageType();
                  lc->lobStorageLocation() = col->lobStorageLocation();
                }
	      cd->setValueId(lc->getValueId());

	      rd->changeNATypeForUserColumnList(i, &lc->getValueId().getType());

	    }
	} // for

   
      if (getPredExprTree())
        {
          getPredExprTree()->preCodeGen(generator);
        }
    } // isTrueRoot
  
  setHdfsAccess(generator->hdfsAccess());

  generator->finetuneBMOEstimates();

  markAsPreCodeGenned();

#ifdef _DEBUG
  if(getenv("SHOW_PLAN"))
  {
    NAString plan;
    unparse(plan);
    printf("PLAN: %s\n",convertNAString(plan,generator->wHeap()));
  }
#endif

  return this;
} // RelRoot::preCodeGen

RelExpr * Join::preCodeGen(Generator * generator,
			   const ValueIdSet & externalInputs,
			   ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  // Check if the pivs of this operator and it's child are the same.
  // If they are not, make them the same.
  replacePivs();

  // In the case of an embedded insert,
  // and there is a selection predicate,
  // we need to retrieve the stored available outputs
  // from the GenericUpdate group attr.

  ValueIdSet availableGUOutputs;

  // clear any prefix sort key 
  generator->clearPrefixSortKey();

  if (getGroupAttr()->isEmbeddedInsert() &&
      !selectionPred().isEmpty()         &&
      getArity() > 1)
    {
      if (child(1)->getArity() > 0)
	child(1)->child(0)->getInputAndPotentialOutputValues(availableGUOutputs);
    }


  NABoolean isALeftJoin = (getOperator().match(REL_ANY_LEFT_JOIN));
  NABoolean isARightJoin = (getOperator().match(REL_ANY_RIGHT_JOIN));
  ValueIdSet availableValues;
  ValueIdSet childPulledInputs;

   if (isALeftJoin)
    {
      ValueId instNullId, exprId, vid;

      // Prune the nullInstatiatedOutputs list.Retain only those values
      // that are either:
      //   1) The external dataflow inputs to the Join.
      //   2) The Characteristic Outputs of the Join.
      //   3) The Characteristic Outputs of the first child of the Join.
      //   4) Values required for evaluating the selection expression
      //      on the Join.
      // Discard all other values.

      availableValues = getGroupAttr()->getCharacteristicInputs();
      availableValues += child(0)->getGroupAttr()->getCharacteristicOutputs();

      ValueIdSet discardSet;
      CollIndex ne = nullInstantiatedOutput().entries();
      for (CollIndex j = 0; j < ne; j++) 
	{
	  instNullId = nullInstantiatedOutput_[j];
	  GenAssert(instNullId.getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL,"NOT instNullId.getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL");
	  // Access the operand of the InstantiateNull
	  exprId = (((InstantiateNull *)(instNullId.getItemExpr()))
					    ->getExpr()->getValueId());
	  if ( (NOT availableValues.contains(exprId)) AND
	       (NOT getGroupAttr()->getCharacteristicOutputs()
		                      .referencesTheGivenValue(instNullId, vid)) AND
	       (NOT selectionPred().referencesTheGivenValue(instNullId, vid)) )
            {
	      discardSet += nullInstantiatedOutput_[j];
            }
	}
      // Delete all those elements that do not require null instantiation.
      for (exprId = discardSet.init(); discardSet.next(exprId);
	   discardSet.advance(exprId))
	{
	  nullInstantiatedOutput_.remove(exprId);
	}
    } // endif (getOperator().match(REL_ANY_LEFT_JOIN))
  else                                 // Null Instantiation will not be necessary.
    nullInstantiatedOutput().clear();  // clear in case a LJ was transformed to an IJ


  if (isARightJoin)
    {
      ValueId instNullIdForRightJoin, exprIdForRightJoin, vidForRightJoin;
      ValueIdSet discardSetForRightJoin;

      // Prune the nullInstatiatedOutputs list.Retain only those values
      // that are either:
      //   1) The external dataflow inputs to the Join.
      //   2) The Characteristic Outputs of the Join.
      //   3) The Characteristic Outputs of the second  child of the Join.
      //   4) Values required for evaluating the selection expression
      //      on the Join.
      // Discard all other values.

      availableValues = getGroupAttr()->getCharacteristicInputs();
      availableValues += child(1)->getGroupAttr()->getCharacteristicOutputs();

      CollIndex neR = nullInstantiatedForRightJoinOutput().entries();
      for (CollIndex j = 0; j < neR; j++) 
	{
	  instNullIdForRightJoin = nullInstantiatedForRightJoinOutput_[j];
	  GenAssert(instNullIdForRightJoin.getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL,"NOT instNullId.getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL");
	  // Access the operand of the InstantiateNull
	  exprIdForRightJoin  = (((InstantiateNull *)(instNullIdForRightJoin.getItemExpr()))
					    ->getExpr()->getValueId());
	  if ( (NOT availableValues.contains(exprIdForRightJoin)) AND
	       (NOT getGroupAttr()->getCharacteristicOutputs()
		                      .referencesTheGivenValue(instNullIdForRightJoin, 
							       vidForRightJoin)) AND
	       (NOT selectionPred().referencesTheGivenValue(instNullIdForRightJoin, 
							    vidForRightJoin)) )
            {
	      discardSetForRightJoin += nullInstantiatedForRightJoinOutput_[j];
            }
	}
      // Delete all those elements that do not require null instantiation.
      for (exprIdForRightJoin = discardSetForRightJoin.init(); 
	   discardSetForRightJoin.next(exprIdForRightJoin);
	   discardSetForRightJoin.advance(exprIdForRightJoin))
	{
	  nullInstantiatedForRightJoinOutput_.remove(exprIdForRightJoin);
	}
    } // endif (getOperator().match(REL_ANY_RIGHT_JOIN))
  else                                 // Null Instantiation will not be necessary.
    nullInstantiatedForRightJoinOutput().clear();  // clear in case a LJ was transformed to an IJ
  
  // Resolve the VEGReferences and VEGPredicates, if any, that appear
  // in the Characteristic Inputs, in terms of the externalInputs.
  getGroupAttr()->resolveCharacteristicInputs(externalInputs);
  availableValues = getGroupAttr()->getCharacteristicInputs();

  bool precodeHalloweenLHSofTSJ = false;
  bool savePrecodeHalloweenLHSofTSJ = false;
  if ((getHalloweenForceSort() != NO_SELF_REFERENCE) &&
      (generator->getR251HalloweenPrecode()))
  {
    savePrecodeHalloweenLHSofTSJ = 
          generator->setPrecodeHalloweenLHSofTSJ(true);
    precodeHalloweenLHSofTSJ  = true;
    
    if (getHalloweenForceSort() == FORCED)
      generator->setHalloweenSortForced();
  }
  
  NABoolean savedOltMsgOpt = generator->oltOptInfo()->oltMsgOpt();

  // My Characteristic Inputs become the external inputs for my left child.
  child(0) = child(0)->preCodeGen(generator,availableValues,childPulledInputs);
  if (! child(0).getPtr())
    return NULL;

  // For HashJoin Min/Max optimization 
  if (isHashJoin()) 
    {
      HashJoin *hj = (HashJoin *)this;

      for(CollIndex i = hj->getStartMinMaxIndex(); i < hj->getEndMinMaxIndex(); i++) 
        {
          // A scan may have decided to use the min/max values that
          // belongs to this join, remove them from the
          // childPulledInputs.  We do not need to pull them from the
          // parent as this Hash Join will generate them.
          if(generator->getWillUseMinMaxKeys()[i] != NULL_VALUE_ID) {
            childPulledInputs -= generator->getMinVals()[i];
            childPulledInputs -= generator->getMaxVals()[i];
          }

          // Clear the candidate values generated by this HashJoin, We
          // are done with the left child, so no one else can use
          // these values.
          generator->getMinMaxKeys()[i] = NULL_VALUE_ID;
          generator->getMinVals()[i] =  NULL_VALUE_ID;
          generator->getMaxVals()[i] = NULL_VALUE_ID;

        }

      // if we have both equi join preds and a beforejoin pred
      // Set a flag that will cause beforeJoinPred to be evaluated prior
      // join equi pred during execution. This helps with join explosion
      // if there are frequent matching values and the beforeJoinPred is
      // highly selective. There is no downside to evaluating beforeJoinPred
      // early, if it contains vids from outer only
      if (!(getEquiJoinPredicates().isEmpty() || getJoinPred().isEmpty() || 
	    isAntiSemiJoin()))
      {
	ValueIdSet coveredPreds, dummy2, dummy3, uncoveredPreds ;
	child(0)->getGroupAttr()->coverTest(getJoinPred(),
					    getGroupAttr()->getCharacteristicInputs(),
					    coveredPreds, dummy2, NULL,
					    &uncoveredPreds);
	// set the flag only if all the non-equi-join preds are covered
	if  ((getJoinPred().entries() == coveredPreds.entries()) &&
	      uncoveredPreds.isEmpty())
	  setBeforeJoinPredOnOuterOnly();
      }

    }

  if (precodeHalloweenLHSofTSJ)
  {
    generator->setPrecodeHalloweenLHSofTSJ(savePrecodeHalloweenLHSofTSJ);
    if (generator->getUnblockedHalloweenScans() == 0)
      {
        // Turn off DP2_LOCKS for codeGen, using either the FORCED_SORT
        // or PASSIVE values.
        if (getHalloweenForceSort() == FORCED)
          {
            generator->setHalloweenProtection(Generator::FORCED_SORT);
         }
        else
          generator->setHalloweenProtection(Generator::PASSIVE);
      }
    else if (updateSelectValueIdMap() && updateTableDesc() && 
	    (NOT updateTableDesc()->getNATable()->getClusteringIndex()->hasSyskey()))
      {
        // if the key columns of the table being inserted into are 
        // equal to constants or inputs then no sort is required 
        // to enforce Halloween blocking. Example statements are
        // update tt set a = 1  ;(a is the primary key for table tt)
        // insert into tt select * from tt where a = 1 ;
        ValueIdList reqdOrder ;
        updateSelectValueIdMap()->rewriteValueIdListDown(
            updateTableDesc()->getClusteringIndex()->getOrderOfKeyValues(),
            reqdOrder);
        reqdOrder.removeCoveredExprs(
            getGroupAttr()->getCharacteristicInputs());
        if (reqdOrder.isEmpty())
          {
            generator->setHalloweenProtection(Generator::PASSIVE);
          }
      }
  }

  NABoolean leftMultipleRowsReturned =
    generator->oltOptInfo()->multipleRowsReturned();

  // if nested join and left child could return multiple rows, then
  // disable olt msg opt for the right child. This is done since
  // olt msg opt can only handle input and output of max 1 row.
  if ((getOperatorType() == REL_NESTED_JOIN) ||
      (getOperatorType() == REL_LEFT_NESTED_JOIN) ||
      (getOperatorType() == REL_NESTED_SEMIJOIN) ||
      (getOperatorType() == REL_NESTED_ANTI_SEMIJOIN) ||
      (getOperatorType() == REL_NESTED_JOIN_FLOW))
    {
      if (generator->oltOptInfo()->multipleRowsReturned())
	{
	  generator->oltOptInfo()->setOltMsgOpt(FALSE);
	}
    }

  // process additional input value ids the child wants
  // (see RelExpr::preCodeGen())
  getGroupAttr()->addCharacteristicInputs(childPulledInputs);
  pulledNewInputs += childPulledInputs;
  availableValues += childPulledInputs;
  childPulledInputs.clear();

  // If this is a tuple substitution join that is implemented by the nested join
  // method, then the values produced as output by my left child can be used as
  // "external" inputs by my right child.
  NABoolean replicatePredicates = TRUE;
  
  ValueIdSet joinInputAndPotentialOutput;
  getInputAndPotentialOutputValues(joinInputAndPotentialOutput);

  if (isTSJ() || beforeJoinPredOnOuterOnly())
    {
      availableValues += child(0)->getGroupAttr()->getCharacteristicOutputs();

      // For a TSJ the joinPred() is a predicate between the inputs
      // and the first child that could not be pushed down to the first
      // child because it is either a left join or an anti-semi-join

      // if beforeJoinPredOnOuterOnly is true, then we have an outer join
      // and an other_join_predicate that has values from the outer side alone
      // We do not need the inner row to evaluate the other_join_predicate 
      // in this case.

      // Rebuild the join predicate tree now
      joinPred().replaceVEGExpressions
                    (availableValues,
                     getGroupAttr()->getCharacteristicInputs(),
                     FALSE, // no key predicates here
                     0 /* no need for idempotence here */,
                     replicatePredicates,
                     NULL /* not a groupByAgg */,
                     &joinInputAndPotentialOutput
                    );
    }

  bool didSetRHS = false;
  bool saveSetRHS = false;
  if (generator->getPrecodeHalloweenLHSofTSJ() &&
      isNestedJoin())
  {
    saveSetRHS = generator->setPrecodeRHSofNJ(true);
    didSetRHS = true;
  }

  // Process the right child
  child(1) = child(1)->preCodeGen(generator,availableValues,childPulledInputs);

  if (! child(1).getPtr())
    return NULL;

  if (didSetRHS)
    generator->setPrecodeRHSofNJ(saveSetRHS);

  NABoolean rightMultipleRowsReturned =
    generator->oltOptInfo()->multipleRowsReturned();

  if (leftMultipleRowsReturned || rightMultipleRowsReturned)
    generator->oltOptInfo()->setMultipleRowsReturned(TRUE);

  // process additional input value ids the child wants
  // (see RelExpr::preCodeGen())
  getGroupAttr()->addCharacteristicInputs(childPulledInputs);
  pulledNewInputs += childPulledInputs;

  // Accumulate the values that are provided as inputs by my parent
  // together with the values that are produced as outputs by my
  // children. Use these values for rewriting the VEG expressions.
  getInputValuesFromParentAndChildren(availableValues);

  // Rebuild the join predicate tree, for the general case where both inner
  // and outer row is needed to evaluate the predicate.
  if (! (isTSJ() || beforeJoinPredOnOuterOnly()))
    joinPred().replaceVEGExpressions
                  (availableValues,
		   getGroupAttr()->getCharacteristicInputs(),
		   FALSE, // no key predicates here
		   0 /* no need for idempotence here */,
		   replicatePredicates,
                   NULL /* not a groupByAgg */,
                   &joinInputAndPotentialOutput
                  );

  if (isALeftJoin)
    {

      // Replace the operands of the ITM_INSTANTIATE_NULL with values from
      // the Characteristic Outputs of the right child.
      // The following values are available for resolving the nullInstantiatedOuptut
      //   1) The external dataflow inputs to the Join.
      //   2) The Characteristic Outputs of the second (right) child of the Join.
      //   3) The Characteristic Outputs of the first(left)child of the Join.
      //      Needed when nested_join plan is chosen.

      availableValues = getGroupAttr()->getCharacteristicInputs();
      availableValues += child(1)->getGroupAttr()->getCharacteristicOutputs();
      availableValues += child(0)->getGroupAttr()->getCharacteristicOutputs();

      nullInstantiatedOutput_.replaceOperandsOfInstantiateNull
                                 (availableValues,
				  getGroupAttr()->getCharacteristicInputs());
    }

  if (isARightJoin)
    {

      // Replace the operands of the ITM_INSTANTIATE_NULL with values from
      // the Characteristic Outputs of the left child.
      // The following values are available for resolving the nullInstantiatedForRightJoinOutput
      //   1) The external dataflow inputs to the Join.
      //   2) The Characteristic Outputs of the first (left) child of the Join.

      availableValues = getGroupAttr()->getCharacteristicInputs();
      availableValues += child(0)->getGroupAttr()->getCharacteristicOutputs();

      nullInstantiatedForRightJoinOutput_.replaceOperandsOfInstantiateNull
	(availableValues,
	 getGroupAttr()->getCharacteristicInputs());
    }

  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  getInputAndPotentialOutputValues(availableValues);

  // If this is an embedded insert, with a selection predicate,
  // add in the characteristic outputs from the generic update RelExpr
  if (getGroupAttr()->isEmbeddedInsert() &&
      !selectionPred().isEmpty())
    {
      availableValues += availableGUOutputs;
    }

  // Rebuild the selection predicate tree.
  selectionPred().replaceVEGExpressions
                     (availableValues,
		      getGroupAttr()->getCharacteristicInputs(),
		      FALSE, // no need for key predicates here
		      0 /* no need for idempotence here */,
		      replicatePredicates
		     );

  //New code was added to avoid the following situation:
  //
  // Query: select max(t1.a) from t1,t2 where t1.a = t2.b;
  // Plan:         shortcut_groupby
  //                        |
  //                  esp_exchange
  //                        |
  //                    merge_join in parallel 4 ways on
  //                        |
  //                    |         |
  //                  scan t2    scan T1
  //
  //   By the time we get to precodegen merge_join has orderby
  //  on VEG(a,b) and characteristic output VEG(a,b)
  //  because scan T2 get precode gen'd first it resolves its
  //  orderby VEG(a,b) to t2.b this also changes orderby VEG
  //  in merge_join and thereafter to T2.b. Now when merge join
  // resolves it characteristic output it resolves it to T1.a because
  // T1 is first in the from clause and T1.a has a smaller value id and
  // so the combined set of T1. and T2's characteristic output has T1.a
  // in front of T2.b. Now esp_exchange during code gen time expects
  // T2.b to be characteristic output of the child because it needs to
  // do merge of sorted streams of its orderby value which is T2.b.
  // this causes an assertion failure because merge_join produces T1.a.
  // Following code counters that by making sure that if the sort key is
  // part of the available values then characteristic output first gets
  // resolved by sortkey then by rest of the available values.
  //

  ValueIdSet sortKey = getPhysicalProperty()->getSortKey();
  sortKey = sortKey.simplifyOrderExpr();
  sortKey.intersectSet(availableValues);

  if(sortKey.entries())
  {
    ValueIdSet reqOutput = getGroupAttr()->getCharacteristicOutputs();
    ValueIdSet copyOfSet(reqOutput);
    ValueIdSet inputValues;
    ValueIdSet newExpr;
    ItemExpr * iePtr;
  // ---------------------------------------------------------------------
  // Iterate over the predicate factors in the given predicate tree.
  // ---------------------------------------------------------------------
  for (ValueId exprId = copyOfSet.init();
        copyOfSet.next(exprId); copyOfSet.advance(exprId))
    {
      // -----------------------------------------------------------------
      // Walk through the item expression tree and replace any
      // VEGPredicates or VEGReferences that are found.
      // -----------------------------------------------------------------
      iePtr = exprId.getItemExpr()->replaceVEGExpressions(availableValues,
							  inputValues,
							  FALSE,
							  NULL,
							  FALSE);
      if (iePtr)			      // expression was not discarded
	{
	  iePtr->synthTypeAndValueId(TRUE);   // redrive type synthesis
          if (iePtr != exprId.getItemExpr())  // a replacement was done
	    {
	      reqOutput.subtractElement(exprId);        // remove existing ValueId
	      reqOutput += iePtr->getValueId(); // replace with a new one
	    }
	}

    } // loop over predTree



    getGroupAttr()->setCharacteristicOutputs(reqOutput);
  }
  // Rewrite the Characteristic Outputs.
  getGroupAttr()->resolveCharacteristicOutputs
                     (availableValues,
		      getGroupAttr()->getCharacteristicInputs());

  // propagate the children olt settings in case of a pushed down to dp2 NLJ
  if ( !getPhysicalProperty()->executeInDP2() OR
       !(generator->getBindWA()->getTopRoot()->getInliningInfo()).isUsedForMvLogging()
     )
  {

     generator->oltOptInfo()->setOltMsgOpt(savedOltMsgOpt);
  }

  // In the case of an embedded insert,
  // set the generator is embedded insert flag to TRUE.

  if (getGroupAttr()->isEmbeddedInsert())
    generator->setEmbeddedInsert(TRUE) ;
  
  markAsPreCodeGenned();

  // Done.
  return this;
} // Join::preCodeGen()

RelExpr * GenericUtilExpr::preCodeGen(Generator * generator,
				      const ValueIdSet & externalInputs,
				      ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! RelExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  if (xnNeeded())
    {
      generator->setUpdSavepointOnError(FALSE);
      generator->setUpdPartialOnError(FALSE);
    }

  markAsPreCodeGenned();

  // Done.
  return this;
}

RelExpr * ExeUtilExpr::preCodeGen(Generator * generator,
				  const ValueIdSet & externalInputs,
				  ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! GenericUtilExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  if (NOT aqrSupported())
    generator->setAqrEnabled(FALSE);

  markAsPreCodeGenned();

  // Done.
  return this;
}

// xnCanBeStarted is set to true if the whole ddl operation can run in one transaction
// It is set to false, then the DDL implementation methods manages the transaction
short DDLExpr::ddlXnsInfo(NABoolean &isDDLxn, NABoolean &xnCanBeStarted)
{
  ExprNode * ddlNode = getDDLNode();

  xnCanBeStarted = TRUE;
  // When the DDL transaction is not turned on via CQD
  if (NOT ddlXns()) 
  { 
     if ((dropHbase()) ||
       (purgedata()) ||
       (initHbase()) ||
       (createMDViews()) ||
       (dropMDViews()) ||
       (initAuth()) ||
       (dropAuth()) ||
       (createRepos()) ||
       (dropRepos()) ||
       (upgradeRepos()) ||
       (addSchemaObjects()) ||
       (updateVersion()))
     {
        // transaction will be started and commited in called methods.
         xnCanBeStarted = FALSE;
     }
  
     if (((ddlNode) && (ddlNode->castToStmtDDLNode()) &&
       (NOT ddlNode->castToStmtDDLNode()->ddlXns())) &&
      ((ddlNode->getOperatorType() == DDL_DROP_SCHEMA) ||
       (ddlNode->getOperatorType() == DDL_CLEANUP_OBJECTS) ||
       (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ADD_CONSTRAINT_PRIMARY_KEY) ||
       (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ALTER_COLUMN_SET_SG_OPTION) ||
       (ddlNode->getOperatorType() == DDL_CREATE_INDEX) ||
       (ddlNode->getOperatorType() == DDL_POPULATE_INDEX) ||
       (ddlNode->getOperatorType() == DDL_CREATE_TABLE) ||
       (ddlNode->getOperatorType() == DDL_ALTER_TABLE_DROP_COLUMN) ||
       (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ALTER_COLUMN_DATATYPE) ||
       (ddlNode->getOperatorType() == DDL_DROP_TABLE)))
     {
        // transaction will be started and commited in called methods.
        xnCanBeStarted = FALSE;
     }
     isDDLxn = FALSE;
  }
  else  // When the DDL transaction is turned on
  {
     isDDLxn = FALSE;
     if (ddlNode && ddlNode->castToStmtDDLNode() &&
        ddlNode->castToStmtDDLNode()->ddlXns())
     isDDLxn = TRUE;

     if (purgedata() || upgradeRepos())
        // transaction will be started and commited in called methods.
        xnCanBeStarted = FALSE;

     if (initHbase() && producesOutput())  // returns status
       xnCanBeStarted = FALSE;

     if ((ddlNode && ddlNode->castToStmtDDLNode() &&
          ddlNode->castToStmtDDLNode()->ddlXns()) &&
            ((ddlNode->getOperatorType() == DDL_CLEANUP_OBJECTS) ||
             (ddlNode->getOperatorType() == DDL_ALTER_TABLE_DROP_COLUMN) ||
             (ddlNode->getOperatorType() == DDL_ALTER_SCHEMA) ||
             (ddlNode->getOperatorType() == DDL_CREATE_INDEX) ||
             (ddlNode->getOperatorType() == DDL_POPULATE_INDEX) ||
             (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ALTER_COLUMN_DATATYPE) ||
             (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ADD_CONSTRAINT_PRIMARY_KEY) ||
             (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ALTER_HBASE_OPTIONS) ||
             (ddlNode->getOperatorType() == DDL_ALTER_INDEX_ALTER_HBASE_OPTIONS) ||
             (ddlNode->getOperatorType() == DDL_ALTER_TABLE_RENAME) ||
             (ddlNode->getOperatorType() == DDL_ON_HIVE_OBJECTS)))
     {
        // transaction will be started and commited in called methods.
        xnCanBeStarted = FALSE;
     }
  } 
  return 0;
}

RelExpr * DDLExpr::preCodeGen(Generator * generator,
			      const ValueIdSet & externalInputs,
			      ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;
  
  if (! GenericUtilExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;
  
  if ((specialDDL()) ||
      (initHbase()))
    {
      generator->setAqrEnabled(FALSE);
    }
  
  NABoolean startXn = FALSE;
  NABoolean ddlXns = FALSE;
  if (ddlXnsInfo(ddlXns, startXn))
    return NULL;

  if (ddlXns && startXn)
    xnNeeded() = TRUE;
  else
    xnNeeded() = FALSE;

  markAsPreCodeGenned();
  
  // Done.
  return this;
}

RelExpr * NestedJoinFlow::preCodeGen(Generator * generator,
				     const ValueIdSet &externalInputs,
				     ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  /*  child(0) = child(0)->preCodeGen(
       generator,
       externalInputs,
       pulledNewInputs);
  if (! child(0).getPtr())
    return NULL;
  */

  RelExpr * nj =
    NestedJoin::preCodeGen(generator, externalInputs, pulledNewInputs);

  if (nj == NULL)
    return NULL;


  return nj;
}

RelExpr * NestedJoin::preCodeGen(Generator * generator,
                                 const ValueIdSet &externalInputs,
                                 ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  NABoolean espExchangeWithMerge = FALSE;
  NABoolean childIsBlocking = FALSE;

  if ((getHalloweenForceSort() != NO_SELF_REFERENCE) &&
      (!generator->getR251HalloweenPrecode()))
    {
      GenAssert(Generator::NOT_SELF_REF != generator->getHalloweenProtection(),
              "Inconsistency in Generator and NestedJoin.");

      // Look for either of two patterns on the left hand side:
      // sort or exchange+sort.  
      if (child(0)->getOperatorType() == REL_SORT) 
        childIsBlocking = TRUE;
      else
        if ((child(0)->getOperatorType() == REL_EXCHANGE) &&
            (child(0)->child(0)->getOperatorType() == REL_SORT))
          {
            childIsBlocking = TRUE;

            // The espExchangeWithMerge flag is used to conditionally
            // assert that the exchange will merge.  The assertion
            // is deferred until after preCodeGen on the left subtree,
            // because the Exchange::doesMerge() method should not be
            // called until Exchange::preCodeGen is finished.
            espExchangeWithMerge = TRUE;
          }

      if (childIsBlocking)
        {
          if (getHalloweenForceSort() == FORCED)
            {
              if (espExchangeWithMerge)
                ((Sort *)(child(0)->child(0).getPtr()))->
                        markAsHalloweenProtection();
              else
                ((Sort *)(child(0).getPtr()))->markAsHalloweenProtection();

              generator->setHalloweenProtection(Generator::FORCED_SORT);
           }
          else
            generator->setHalloweenProtection(Generator::PASSIVE);
        }
      else if (updateSelectValueIdMap() && updateTableDesc() && 
	      (NOT updateTableDesc()->getNATable()->getClusteringIndex()->hasSyskey()))
        {
          // if the key columns of the table being inserted into are 
          // equal to constants or inputs then no sort is required 
          // to enforce Halloween blocking. Example statements are
          // update tt set a = 1  ;(a is the primary key for table tt)
          // insert into tt select * from tt where a = 1 ;
          ValueIdList reqdOrder ;
          updateSelectValueIdMap()->rewriteValueIdListDown(
              updateTableDesc()->getClusteringIndex()->getOrderOfKeyValues(),
              reqdOrder);
          reqdOrder.removeCoveredExprs(
              getGroupAttr()->getCharacteristicInputs());
          if (reqdOrder.isEmpty())
            {
              generator->setHalloweenProtection(Generator::PASSIVE);
            }
        }
    }

  // Insert a probe cache above the inner table if applicable
  if ( isProbeCacheApplicable(
       castToRelExpr()->getPhysicalProperty()->getPlanExecutionLocation()
                             )
     )
    {
      ProbeCache *probeCache = 
        new (generator->wHeap()) ProbeCache( 
          child(1), getDefault(GEN_PROBE_CACHE_NUM_ENTRIES),
          generator->wHeap());

      // look for an aggregate right child node
      RelExpr     *rightChildExpr = child(1).getPtr();
      GroupByAgg  *rightChildGrby = NULL;
      RelExpr     *rightChildExch = NULL;
      MapValueIds *rightChildMvi  = NULL;
      ValueIdMap  *optionalMap    = NULL;
      NABoolean   done            = FALSE;

      while (!done)
        {
          if (rightChildExpr->getOperator().match(REL_ANY_GROUP))
            {
              rightChildGrby = (GroupByAgg *) rightChildExpr;
              done = TRUE;
            }
          else if (rightChildExpr->getOperator() == REL_EXCHANGE)
            {
              if (rightChildExch == NULL)
                rightChildExch = rightChildExpr;
              else
                done = TRUE; // can't handle more than one exchange
            }
          else if (rightChildExpr->getOperator() == REL_MAP_VALUEIDS)
            {
              if (rightChildMvi == NULL)
                {
                  rightChildMvi = (MapValueIds *) rightChildExpr;
                  optionalMap   = &rightChildMvi->getMap();
                }
              else
                done = TRUE; // can't handle more than one MVI
            }
          else
            done = TRUE;

          if (!done)
            rightChildExpr = rightChildExpr->child(0);
        }

      // Among other things, this will give the probeCache
      // the characteristic inputs and outputs of the 
      // inner table.
      probeCache->setGroupAttr(new(generator->wHeap())
			    GroupAttributes(*(child(1)->getGroupAttr())));

      // Try to pull up predicates from the child, if that reduces
      // the char. inputs sent to the child. We only try this right
      // now if the child is an aggregate or groupby.
      if (rightChildGrby && 
          CmpCommon::getDefault(NESTED_JOIN_CACHE_PREDS) != DF_OFF &&
          (// if right child exchange exists, it must have same char inputs
           rightChildExch == NULL ||
           rightChildExch->getGroupAttr()->getCharacteristicInputs() ==
           rightChildGrby->getGroupAttr()->getCharacteristicInputs()) &&
          (rightChildMvi == NULL ||
           rightChildMvi->getGroupAttr()->getCharacteristicInputs() ==
           rightChildGrby->getGroupAttr()->getCharacteristicInputs()))
        {
          ValueIdSet pcAvailableInputs(
               probeCache->getGroupAttr()->getCharacteristicInputs());

          // predicates can refer to both char. inputs and outputs
          pcAvailableInputs += 
            probeCache->getGroupAttr()->getCharacteristicOutputs();

          // note that this will overwrite the ProbeCache's selection preds
          rightChildGrby->tryToPullUpPredicatesInPreCodeGen(
               pcAvailableInputs,
               probeCache->selectionPred(),
               optionalMap);

          // adjust char. inputs of intervening nodes - this is not
          // exactly good style, just overwriting the char. inputs, but
          // hopefully we'll get away with it at this stage in the
          // processing
          if (rightChildExch)
            rightChildExch->getGroupAttr()->setCharacteristicInputs(
                 rightChildGrby->getGroupAttr()->getCharacteristicInputs());
          if (rightChildMvi)
            rightChildMvi->getGroupAttr()->setCharacteristicInputs(
                 rightChildGrby->getGroupAttr()->getCharacteristicInputs());
        }

      // propagate estimates, physical properties, and costings 
      // from the child to the ProbeCache:
      probeCache->setEstRowsUsed(child(1)->getEstRowsUsed());
      probeCache->setMaxCardEst(child(1)->getMaxCardEst());
      probeCache->setInputCardinality(child(1)->getInputCardinality());
      probeCache->setPhysicalProperty(child(1)->getPhysicalProperty());
      probeCache->setOperatorCost(0);
      probeCache->setRollUpCost(child(1)->getRollUpCost());

      // Glue the ProbeCache to the NestedJoin's right leg.
      child(1) = probeCache;
    }                     
  if (isTSJForUndo())
  {
  
    Sort *sortNode = new(generator->wHeap()) Sort(child(0));

      ItemExpr *sk = new (generator->wHeap()) SystemLiteral(1);
      sk->synthTypeAndValueId(TRUE);

      ValueIdList skey;
      skey.insert(sk->getValueId());

      sortNode->getSortKey() = skey;

    // Use the same characteristic inputs and outputs as the left child
    sortNode->setGroupAttr(new(generator->wHeap())
			   GroupAttributes(*(child(0)->getGroupAttr())));
    //pass along some of the  estimates 
    sortNode->setEstRowsUsed(child(0)->getEstRowsUsed());
    sortNode->setMaxCardEst(child(0)->getMaxCardEst());
    sortNode->setInputCardinality(child(0)->getInputCardinality());
    sortNode->setPhysicalProperty(child(0)->getPhysicalProperty());
    sortNode->setCollectNFErrors();
    sortNode->setOperatorCost(0);
    sortNode->setRollUpCost(child(0)->getRollUpCost());
    child(0) = sortNode;
  }

  if ( childIsBlocking && 
      generator->preCodeGenParallelOperator() )
  {
    if (espExchangeWithMerge == FALSE) 
    {
      // A "halloween sort" needs to ensure that if it is parallel, but executes
      // in the same ESP as the generic update's TSJ flow node, then the Sort
      // will block until all scans are finished.  
      ((Sort *)(child(0).getPtr()))->doCheckAccessToSelfRefTable();
    }
    else
    {
      // An ESP Exchange can be eliminated in its preCodeGen method if it is 
      // redundant.  If this happens, then the Sort will be executing in the 
      // same ESP as the TSJ after all.  So we set this flag now, so that the 
      // Exchange preCodeGen will call doCheckAccessToSelfRefTable() for the 
      // Sort before eliminating itself.  This is part of the fix for Sol 
      // 10-090310-9876.
      ((Exchange *)(child(0).getPtr()))->markHalloweenSortIsMyChild();
    }
  }
  RelExpr * re =
    Join::preCodeGen(generator, externalInputs, pulledNewInputs);

  if ( espExchangeWithMerge &&
      (child(0)->getOperatorType() == REL_EXCHANGE))
    GenAssert(((Exchange *)((RelExpr *)child(0)))->doesMerge(), 
      "Exchange operator does not block for Halloween problem.");

  generator->compilerStatsInfo().nj()++;

  return re;
}


RelExpr * MergeJoin::preCodeGen(Generator * generator,
				const ValueIdSet & externalInputs,
				ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! Join::preCodeGen(generator, externalInputs, pulledNewInputs))
    return 0;

  // Accumulate the values that are provided as inputs by my parent
  // together with the values that are produced as outputs by my
  // children. Use these values for rewriting the VEG expressions.
  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);

  // find if the left child and/or the right child will have atmost
  // one matching row. If so, an faster merge join implementation
  // will be used at runtime.
  ValueIdSet vidSet = getOrderedMJPreds();
  ValueIdSet valuesUsedForPredicates;

  computeValuesReqdForPredicates(vidSet,
				 valuesUsedForPredicates);
  leftUnique() =
    child(0)->getGroupAttr()->isUnique(valuesUsedForPredicates);

  rightUnique() =
    child(1)->getGroupAttr()->isUnique(valuesUsedForPredicates);


  ValueIdList mjp(getOrderedMJPreds());

  NABoolean replicatePredicates = TRUE;
  /* For merge join the characteristic outputs have already been resolved 
     by the time the equijoin preds are resolved below. The outputs are 
     resolved at the very end of Join::precodegen, which was called a few
     lines above. Therefore when we resolve the equijoin preds we have 
     only the actually resolved output values available. We do not have 
     all the potential output values available. 
  */
  ValueIdSet joinInputAndOutputValues;
  joinInputAndOutputValues = getGroupAttr()->getCharacteristicInputs();
  joinInputAndOutputValues += getGroupAttr()->getCharacteristicOutputs();

  // Pass in the children GAs so that the equipreds can have one side
  // resolved to one child and the other side resolved to the other child.
  // solution 10-100722-1962
  mjp.replaceVEGExpressions
         (availableValues,
	  getGroupAttr()->getCharacteristicInputs(),
	  FALSE, // no key predicates here
	  0 /* no need for idempotence here */,
	  replicatePredicates,
          NULL /* not a groupByAgg */,
          &joinInputAndOutputValues,
          NULL /* no indexDesc since we have no key preds*/,
          child(0)->getGroupAttr(),
          child(1)->getGroupAttr());

  // must have at least 1 merge join predicate
  GenAssert(!mjp.isEmpty(),"mjp.isEmpty()");

  // The generator expects the merge join predicates to be in the form
  // leftcol = rightcol where leftcol references a column from the left
  // table and rightcol references a column from the right table. Switch
  // the expression if it is the other way around. Also handle rare cases
  // where a VEGPred is resolved into two equalities connected by an AND.
  //
  ValueIdSet dummy1;
  ValueIdList newJoinPreds;
  ValueIdList newLeftOrder;
  ValueIdList newRightOrder;
  CollIndex ne = (CollIndex)(mjp.entries());
  NABoolean isANewJoinPred ;

  for (CollIndex i = 0; i < ne; i++)
  {
    // Will store all the conjuncts under the pred mjp[i] being considered.
    ValueIdSet conjuncts;
    conjuncts.clear();
    conjuncts.insert(mjp[i]);
    ValueIdSet finerConjuncts;

    do
    {
      finerConjuncts.clear();
      // Go through the set of conjuncts, breaking down any AND seen into
      // finer conjuncts.
      //
      for (ValueId vid = conjuncts.init();
                         conjuncts.next(vid);
                         conjuncts.advance(vid))
      {
        ItemExpr *pred = vid.getItemExpr();

        if (pred->getOperatorType() == ITM_AND)
        {
          // Found another AND, break it down into finer conjuncts. Store
          // them in finerConjuncts so that we can return to them later.
          //
          finerConjuncts.insert(pred->child(0)->getValueId());
          finerConjuncts.insert(pred->child(1)->getValueId());
        }
        else
        {
          // This is the "finest" conjunct - cannot be broken down further.
          // Make sure it's in the form of (leftCol = rightCol). Add the
          // equality predicate to the final list of MJ predicates. leftOrder
          // and rightOrder are set up correspondingly so that they match up
          // with the predicates.
          //
          GenAssert(pred->getOperatorType() == ITM_EQUAL,
                   "pred->getOperatorType() != ITM_EQUAL");

          ItemExpr *left = pred->child(0)->castToItemExpr();
          ItemExpr *right = pred->child(1)->castToItemExpr();
          isANewJoinPred = TRUE;

          NABoolean child0Covered = child(0).getGroupAttr()->covers(left->getValueId(),
					      getGroupAttr()->getCharacteristicInputs(),
					      dummy1) ;

          NABoolean child1Covered = child(1).getGroupAttr()->covers(right->getValueId(),
						  getGroupAttr()->getCharacteristicInputs(),
						  dummy1) ;

          if (NOT (child0Covered && child1Covered))
          {
	    //++MV - Irena
	    // save the pred's specialNulls_ flag before replacing the pred
 	    BiRelat *biRelat = new(generator->wHeap()) BiRelat(ITM_EQUAL, right, left);
	    // restore specialNulls_
	    biRelat->setSpecialNulls(((BiRelat*)pred)->getSpecialNulls());
	    biRelat->bindNode(generator->getBindWA());
	    pred = biRelat;
	    //--MV - Irena
	    
	    child0Covered = child(0).getGroupAttr()->covers(right->getValueId(),
						  getGroupAttr()->getCharacteristicInputs(),
						  dummy1) ;
	    
	    child1Covered = child(1).getGroupAttr()->covers(left->getValueId(),
						  getGroupAttr()->getCharacteristicInputs(),
						  dummy1) ;

            if(!(child0Covered && child1Covered))
            {
              if (isInnerNonSemiJoin())
              {
                selectionPred() += pred->getValueId();
              }
            else
              {
                // for an outer or semi join, the ON clause is stored in "joinPred"
                // while the WHERE clause is stored in "selectionPred".
                joinPred() += pred->getValueId();
              }
              isANewJoinPred = FALSE;
            }
          }

          if (isANewJoinPred)
          {
          // Store the finest conjuncts in the final list of MJ predicates.
          // Make sure the list is matched up with corresponding leftOrder
          // and rightOrder.
          //
          newJoinPreds.insert(pred->getValueId());
          newLeftOrder.insert(getLeftSortOrder()[i]);
          newRightOrder.insert(getRightSortOrder()[i]);
        }
        }
      } // for over conjuncts.

      // Come back to process the new set of broken-down conjuncts if the set
      // is non-empty.
      //
      conjuncts = finerConjuncts;
    } while (NOT conjuncts.isEmpty());
  } // for over mjp.
  if (ne > 0)
    GenAssert(NOT newJoinPreds.isEmpty(), "MergeJoin::PreCodeGen has no resolved join predicates");


  // Count merge join as a Big Memory Operator (BMO) if use of BMO quota
  // is enabled for merge join.
  if (CmpCommon::getDefaultLong(MJ_BMO_QUOTA_PERCENT) != 0)
  {
    generator->incrNumBMOs();
  }

  setOrderedMJPreds(newJoinPreds);
  setLeftSortOrder(newLeftOrder);
  setRightSortOrder(newRightOrder);

  generator->compilerStatsInfo().mj()++;

  markAsPreCodeGenned();
  return this;

} //  MergeJoin::preCodeGen()

RelExpr * HashJoin::preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if ( CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) == DF_SYSTEM)
  {
    NABoolean resize = FALSE;
    NABoolean defrag = FALSE;
    ValueIdSet vidSet0 = child(0)->getGroupAttr()->getCharacteristicOutputs();
    ValueIdSet vidSet1 = child(1)->getGroupAttr()->getCharacteristicOutputs();

    ExpTupleDesc::TupleDataFormat tupleFormat =
                  determineInternalFormat( vidSet1,
                                           vidSet0,
                                           this,
                                           resize,
                                           generator,
                                           FALSE,
                                           defrag);

    cacheTupleFormatAndResizeFlag(tupleFormat, resize, defrag);

    if (tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
    {
      generator->incNCIFNodes();
    }
    else
    {
      generator->decNCIFNodes();
    }
	}	
  // Determine if we should attempt to use the HashJoin min/max optimization.
  NABoolean useMinMaxOpt = 
    ((CmpCommon::getDefault(GEN_HSHJ_MIN_MAX_OPT) == DF_ON) &&
     ! getEquiJoinPredicates().isEmpty() &&
     ! isLeftJoin() &&
     ! isRightJoin() &&
     ! isAntiSemiJoin());

  // These indexes define the subset of min max values which belong to this HashJoin.
  CollIndex startMinMaxIndex = 0;
  CollIndex endMinMaxIndex = 0;

  // If min/max opt is used, these lists are used to hold local copies of the
  // generators min and max values.  These are the min and max values
  // generated by HashJoins that may be used by scans.
  ValueIdList myMinVals;
  ValueIdList myMaxVals;

  // If min/max opt is used, this list are used to hold a local copy
  // of the generators minmaxKeys.  These are the values for which min
  // and max values are available
  ValueIdList myMinMaxKeys;

  if (useMinMaxOpt) {

    // This HashJoin will append to the end of the generator lists.
    //
    startMinMaxIndex = generator->getMinMaxKeys().entries();

    // Find the candidate values from the right hand side of the join.
    // For now, only consider VEGPreds.
    for (ValueId valId = getEquiJoinPredicates().init();
	 getEquiJoinPredicates().next(valId);
	 getEquiJoinPredicates().advance(valId)) {
      ItemExpr * itemExpr = valId.getItemExpr();

      NAType *mmType = NULL;
      if (itemExpr->getOperatorType() == ITM_VEG_PREDICATE) {

        VEGPredicate *vPred = (VEGPredicate *)itemExpr;
        VEGReference *vRef = vPred->getVEG()->getVEGReference();

        mmType = vRef->getValueId().getType().newCopy(generator->wHeap());

      }

      // mmType is the type of the VEGRef relating a left and right value.
      // We will compute the Min and Max using this type
      if(mmType) {

        // Min/Max are typed as nullable.
        mmType->setNullable(true);

        // Construct the host vars which will represent the min and
        // max values for this join key.

        char name[80];
        sprintf(name, "_sys_MinVal%d", generator->getMinMaxKeys().entries());

        ItemExpr *minVal = new(generator->wHeap()) 
          HostVar(name,
                  mmType,
                  TRUE);

        sprintf(name, "_sys_MaxVal%d", generator->getMinMaxKeys().entries());

        ItemExpr *maxVal = new(generator->wHeap())
          HostVar(name,
                  mmType,
                  TRUE);

        minVal->synthTypeAndValueId();
        maxVal->synthTypeAndValueId();

        // Insert the value and min and max into generator lists to
        // make the available to scans as key predicates.
        generator->getMinMaxKeys().insert(itemExpr->getValueId());
        generator->getMinVals().insert(minVal->getValueId());
        generator->getMaxVals().insert(maxVal->getValueId());
        
        // Initialize the 'will use' list to a NULL_VALUE_ID.  A scan
        // that decides to use the min max values will change this
        // entry to be the same as the corresponding entry in the
        // minMaxKeys list.
        generator->getWillUseMinMaxKeys().insert(NULL_VALUE_ID);

      }
    }
    
    // This is the end index (exclusive) for this HashJoin.
    endMinMaxIndex = generator->getMinMaxKeys().entries();

    // Keep local copies of the generators lists.
    myMinVals = generator->getMinVals();
    myMaxVals = generator->getMaxVals();
    myMinMaxKeys = generator->getMinMaxKeys();

  }

  // Register the start and end indexes for this Hash Join
  // (Join::preCodeGen() needs to have access to the indexes)
  setStartMinMaxIndex(startMinMaxIndex);
  setEndMinMaxIndex(endMinMaxIndex);

  if (! Join::preCodeGen(generator, externalInputs, pulledNewInputs))
    return NULL;

  // List for min and max values that will be used by a scan and which
  // will be generated by this HashJoin
  minMaxVals_.clear();
  minMaxCols_.clear();

  {

    // For each min/max value belonging to this HashJoin, check to see
    // if any scan decided to use it.  If so, add the min and max
    // values to the list.  Also, clear the 'will use' entry as no
    // other HashJoin can supply this value.
    for (CollIndex i = startMinMaxIndex; i < endMinMaxIndex; i++)
      {
        if (generator->getWillUseMinMaxKeys()[i] != NULL_VALUE_ID) 
          {

            minMaxVals_.insert(myMinVals[i]);
            minMaxVals_.insert(myMaxVals[i]);

            VEGPredicate *vPred = (VEGPredicate *)myMinMaxKeys[i].getItemExpr();
            VEGReference *vRef = vPred->getVEG()->getVEGReference();

            minMaxCols_.insert(vRef->getValueId());

            generator->getWillUseMinMaxKeys()[i] = NULL_VALUE_ID;
          }
      }

    // If we have some minMaxCols, then replace any VEGReferences.
    if (minMaxCols_.entries()) 
      {
        ValueIdSet availForMinMax;
        availForMinMax += child(1)->getGroupAttr()->getCharacteristicOutputs();
        availForMinMax += getGroupAttr()->getCharacteristicInputs();
        minMaxCols_.replaceVEGExpressions(availForMinMax,
                                          getGroupAttr()->getCharacteristicInputs());
      }
  }

  // Accumulate the values that are provided as inputs by my parent
  // together with the values that are produced as outputs by my
  // children. Use these values for rewriting the VEG expressions.
  ValueIdSet availableValues;


  getInputValuesFromParentAndChildren(availableValues);

  ValueIdSet hjp(getEquiJoinPredicates());

  NABoolean replicatePredicates = TRUE;

  /* For hash join the characteristic outputs have already been resolved 
     by the time the equijoin preds are resolved below. The outputs are 
     resolved at the very end of Join::precodegen, which was called a few
     lines above. Therefore when we resolve the equijoin preds we have 
     only the actually resolved output values available. We do not have 
     all the potential output values available. 
  */
  ValueIdSet joinInputAndOutputValues;
  joinInputAndOutputValues = getGroupAttr()->getCharacteristicInputs();
  joinInputAndOutputValues += getGroupAttr()->getCharacteristicOutputs();

  // Pass in the children GAs so that the equipreds can have one side
  // resolved to one child and the other side resolved to the other child.
  // solution 10-100722-1962
  hjp.replaceVEGExpressions
         (availableValues,
	  getGroupAttr()->getCharacteristicInputs(),
	  FALSE, // no key predicates here
	  0 /* no need for idempotence here */,
	  replicatePredicates,
          NULL /* not a groupByAgg */,
          &joinInputAndOutputValues,
          NULL /* no indexDesc since we have no key preds*/,
          child(0)->getGroupAttr(),
          child(1)->getGroupAttr());
  

  // Will store the rewritten hjp's which compile with the format of
  // leftCol= rightCol.
  //
  ValueIdSet newJoinPreds;

  if (hjp.isEmpty())
  {
  }
  else
  {
    // The generator expects the hash join predicates to be in the form
    // leftcol = rightcol where leftcol references a column from the left
    // table and rightcol references a column from the right table. Switch
    // the expression if it is the other way around. Also handle rare cases
    // where a VEGPred is resolved into two equalities connected by an AND.
    //
    ValueIdSet dummy1;
    NABoolean isANewJoinPred ;


    do
    {
      ValueIdSet finerConjuncts;
      finerConjuncts.clear();


      for (ValueId vid = hjp.init();
                         hjp.next(vid);
                         hjp.advance(vid))
      {
        ItemExpr *pred = vid.getItemExpr();

        // Break this up into the finer conjuncts. Store them in a separate
        // set so that we can return to it later.
        // of the set so that we could return
        if (pred->getOperatorType() == ITM_AND)
        {
          finerConjuncts.insert(pred->child(0)->getValueId());
          finerConjuncts.insert(pred->child(1)->getValueId());
        }
        else
        {
          GenAssert(pred->getOperatorType() == ITM_EQUAL,
                   "pred->getOperatorType() != ITM_EQUAL");

          ItemExpr *left = pred->child(0)->castToItemExpr();
          ItemExpr *right = pred->child(1)->castToItemExpr();
          isANewJoinPred = TRUE;

          NABoolean child0Covered = child(0).getGroupAttr()->covers(left->getValueId(),
					      getGroupAttr()->getCharacteristicInputs(),
					      dummy1) ;
	  
          NABoolean child1Covered = child(1).getGroupAttr()->covers(right->getValueId(),
						  getGroupAttr()->getCharacteristicInputs(),
						  dummy1) ;

          if (NOT (child0Covered && child1Covered))
          {
	    //++MV - Irena
	    // save the pred's specialNulls_ flag before replacing the pred
 	    BiRelat *biRelat = new(generator->wHeap()) BiRelat(ITM_EQUAL, right, left);
	    // restore specialNulls_
	    biRelat->setSpecialNulls(((BiRelat*)pred)->getSpecialNulls());
	    biRelat->bindNode(generator->getBindWA());
	    pred = biRelat;
	    //--MV - Irena
	    
	    child0Covered = child(0).getGroupAttr()->covers(right->getValueId(),
						  getGroupAttr()->getCharacteristicInputs(),
						  dummy1) ;
	    
	    child1Covered = child(1).getGroupAttr()->covers(left->getValueId(),
						  getGroupAttr()->getCharacteristicInputs(),
						  dummy1) ;

            if(!(child0Covered && child1Covered))
            {
              if (isInnerNonSemiJoin())
              {
                selectionPred() += pred->getValueId();
              }
            else
              {
                // for an outer or semi join, the ON clause is stored in "joinPred"
                // while the WHERE clause is stored in "selectionPred".
                joinPred() += pred->getValueId();
              }
              isANewJoinPred = FALSE;
            }
          }
          if (isANewJoinPred)
          newJoinPreds.insert(pred->getValueId());
	}
    } // for over hjp.
    
    // Come back to process the new set of broken-down conjuncts if the set
    // is non-empty.
    //
    hjp = finerConjuncts;
    } while (NOT hjp.isEmpty());
    GenAssert(NOT newJoinPreds.isEmpty(), "HashJoin::PreCodeGen has no resolved join predicates");
  }

  // Value IDs given to the right/inner child
  ValueIdSet valuesGivenToRightChild =
    child(1)->getGroupAttr()->getCharacteristicInputs();

  if ( ! valuesGivenToRightChild.isEmpty() )  {

    // Accumulate the values that are provided as inputs by my parent
    // together with the values that are produced as outputs by my
    // children. Use these values for rewriting the VEG expressions.
    ValueIdSet availableValues;
    const ValueIdSet & HJInputs = getGroupAttr()->getCharacteristicInputs();
    getInputValuesFromParentAndChildren(availableValues);

    valuesGivenToRightChild.replaceVEGExpressions(availableValues, HJInputs);
  }

  // before computing the move and check expressions, add one more
  // value to "valuesGivenToRightChild": a statement execution count
  // that will cause re-hashing each time the statement is
  // re-executed. It is not legal to keep a hash table across
  // statement executions (and possibly transactions).
  ValueId execCount = generator->getOrAddStatementExecutionCount();
  valuesGivenToRightChild += execCount;
  pulledNewInputs += execCount;
  getGroupAttr()->addCharacteristicInputs(pulledNewInputs);

  // add move and search expressions
  for (ValueId val_id = valuesGivenToRightChild.init();
       valuesGivenToRightChild.next(val_id);
       valuesGivenToRightChild.advance(val_id))  {

    ItemExpr * item_expr = val_id.getItemExpr();

    // add this converted value to the map table.
    Convert * conv_node = new(generator->wHeap()) Convert (item_expr);

    // bind/type propagate the new node
    conv_node->bindNode(generator->getBindWA());

    moveInputValues().insert(conv_node->getValueId());

    // add the search condition
    BiRelat * bi_relat = new(generator->wHeap())
      BiRelat(ITM_EQUAL, item_expr, conv_node);
    bi_relat->allocValueId();
    checkInputValues().insert(bi_relat->getValueId());

  }  // for val_id

  // Count this BMO and add its needed memory to the total needed
  generator->incrNumBMOs();

  if ((ActiveSchemaDB()->getDefaults()).getAsDouble(BMO_MEMORY_LIMIT_PER_NODE_IN_MB) > 0)
    generator->incrBMOsMemory(getEstimatedRunTimeMemoryUsage(generator, TRUE));


  // store the transformed predicates back into the hash join node
  storeEquiJoinPredicates(newJoinPreds);

  generator->compilerStatsInfo().hj()++;

  //
  // case of hash anti semi join optimization (NOT IN)
  // add/build expression to detect inner and outer null :
  // checkOuteNullexpr_ and checkInnerNullExpr_ 
  addCheckNullExpressions(generator->wHeap());

  markAsPreCodeGenned();
  return this;

} //  HashJoin::preCodeGen()

RelExpr * FileScan::preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  const PartitioningFunction* myPartFunc = getPartFunc();

  NABoolean usePartKeyPreds =
    (isHbaseTable() &&
     myPartFunc &&
     myPartFunc->isPartitioned() &&
     !myPartFunc->isAReplicationPartitioningFunction());

  if (isRewrittenMV())
    generator->setNonCacheableMVQRplan(TRUE);

  if (usePartKeyPreds)
    {
      // partition key predicates will be applied to this file scan,
      // "pull" the partition input values from the parent
      pulledNewInputs += myPartFunc->getPartitionInputValues();
      getGroupAttr()->addCharacteristicInputs(myPartFunc->getPartitionInputValues());
    }

  // Resolve the VEGReferences and VEGPredicates, if any, that appear
  // in the Characteristic Inputs, in terms of the externalInputs.
  getGroupAttr()->resolveCharacteristicInputs(externalInputs);

  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  ValueIdSet availableValues;

  getInputAndPotentialOutputValues(availableValues);

  sampledColumns().replaceVEGExpressions
    (availableValues, getGroupAttr()->getCharacteristicInputs());

  // Rewrite the partitioning function in terms of the available values.
  if (getIndexDesc()->isPartitioned())
    getIndexDesc()->getPartitioningFunction()->preCodeGen(availableValues);

  // VEGPredicates that are key predicates but are also replicated in
  // the executor predicates must be replaced with the same expression
  // in both the places after they are rewritten.  The VEGRewritePairs
  // data structure, when passed to replaceVEGExpressions(), causes
  // replaceVEGExpressions() to be idempotent.

  VEGRewritePairs  vegPairs(generator->wHeap());
  ValueIdSet partKeyPredsHBase;

  if (usePartKeyPreds)
    {
      // add the partitioning key predicates to this scan node,
      // to make sure that each ESP reads only the part of the
      // data that it is supposed to process

      ValueId saltCol;

      if (myPartFunc->isATableHashPartitioningFunction())
        {
          // find the _SALT_ column and make a partitioning key
          // predicate for it
          const ValueIdList &keyCols = getIndexDesc()->getIndexKey();

          // the first salt column we find in the key is the one
          // we are looking for
          for (CollIndex i=0; i<keyCols.entries(); i++)
            if (keyCols[i].isSaltColumn())
              {
                saltCol = keyCols[i];
                break;
              }

          if (saltCol != NULL_VALUE_ID)
            ((TableHashPartitioningFunction *) myPartFunc)->
              createPartitioningKeyPredicatesForSaltedTable(saltCol);
        }

      partKeyPredsHBase = myPartFunc->getPartitioningKeyPredicates();
    }

  if (getMdamKeyPtr() != NULL)
    {

      NABoolean replicatePredicates = TRUE;

      //      mdamKeyPtr()->print(); // for debugging purposes

      ValueIdSet executorPredicates;

      ValueIdSet augmentedPreds = getSelectionPredicates();

      const LogPhysPartitioningFunction *logPhysPartFunc =
	getPartFunc()->castToLogPhysPartitioningFunction();

      if (!partKeyPredsHBase.isEmpty())
        {
          augmentedPreds += partKeyPredsHBase;
          mdamKeyPtr()->setNoExePred(FALSE);
        }

      augmentedPreds += getComputedPredicates();

      if ( logPhysPartFunc != NULL )
      {
	LogPhysPartitioningFunction::logPartType logPartType =
	  logPhysPartFunc->getLogPartType();
	if ( logPartType == LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING
	  OR logPartType == LogPhysPartitioningFunction::HORIZONTAL_PARTITION_SLICING
	  )
	  augmentedPreds += logPhysPartFunc->getPartitioningKeyPredicates();
      }

      mdamKeyPtr()->preCodeGen(executorPredicates,
			       augmentedPreds,
			       availableValues,
			       getGroupAttr()->getCharacteristicInputs(),
			       &vegPairs,
			       replicatePredicates,
                               !partKeyPredsHBase.isEmpty());
      setExecutorPredicates(executorPredicates);

      //      mdamKeyPtr()->print(); // for debugging purposes


    }
  else if (! isHiveTable() &&
           (getSearchKey() || !partKeyPredsHBase.isEmpty()))
    {
      // ---------------------------------------------------
      // --------------------- Rewrite preds for search key:
      // ---------------------------------------------------

      if (!partKeyPredsHBase.isEmpty())
        {
          // These predicates can compete with other key predicates;
          // decide which of them to use as key preds and which as
          // executor preds:

          // - No search key: Use part key preds as search key
          // - Search key with non-unique preds: Replace it with
          //   a new search key with part key preds
          // - Search key with unique preds (unlikely, this shouldn't
          //   have been a parallel query): add part key preds as
          //   executor preds

          ValueIdSet combinedInputs(externalInputs);
          combinedInputs += pulledNewInputs;
          ValueIdSet existingKeyPreds;

          if (getSearchKey()) 
            existingKeyPreds += getSearchKey()->getKeyPredicates();
   
          // create a new search key that has the partitioning key preds
          SearchKey * partKeySearchKey = 
            myPartFunc->createSearchKey(getIndexDesc(),
                                        combinedInputs,
                                        existingKeyPreds);
          ValueIdSet exePreds(partKeySearchKey->getExecutorPredicates());
          NABoolean replaceSearchKey = !(getSearchKey() &&
                                         getSearchKey()->isUnique());

          if (getSearchKey()) 
            exePreds += getSearchKey()->getExecutorPredicates();

          ValueId falseConst = NULL_VALUE_ID;
          if (exePreds.containsFalseConstant(falseConst))
            replaceSearchKey = FALSE;

          // pick one search key and add the remaining
          // predicates (if any) to exePreds
          if (replaceSearchKey)
            setSearchKey(partKeySearchKey);
          else
            exePreds += partKeySearchKey->getKeyPredicates();
          searchKey()->setExecutorPredicates(exePreds);
        }

      NABoolean replicatePredicates = TRUE;

      setExecutorPredicates(searchKey()->getExecutorPredicates());

      // Rebuild the search key expressions
      ValueIdSet& keyPred = searchKey()->keyPredicates();
      keyPred.replaceVEGExpressions
	(availableValues,
	 getGroupAttr()->getCharacteristicInputs(),
	 FALSE, // no need for key predicate generation here
	 &vegPairs,
	 replicatePredicates);

      // Rebuild the executor  predicate tree
      executorPred().replaceVEGExpressions
	(availableValues,
	 getGroupAttr()->getCharacteristicInputs(),
	 FALSE, // no need for key predicate generation here
	 &vegPairs,
	 replicatePredicates
	 );

      // Generate the begin and end keys.

      if ( getDoUseSearchKey() ) {
     
      generateKeyExpr(getGroupAttr()->getCharacteristicInputs(),
		      getIndexDesc()->getIndexKey(),
		      getSearchKey()->getBeginKeyValues(),
		      beginKeyPred_,
		      generator,
                      replicatePredicates);
      generateKeyExpr(getGroupAttr()->getCharacteristicInputs(),
		      getIndexDesc()->getIndexKey(),
		      getSearchKey()->getEndKeyValues(),
		      endKeyPred_,
		      generator,
                      replicatePredicates);
      }

      // Check to see if there are any MIN/MAX values coming from a
      // HashJoin which could be used as begin/end key values for the
      // leading key of this scan.  Don't consider doing this if this
      // is a unique scan (can't improve on that) or if the leading
      // key is already unique or if both the begin and end key are
      // exclusive (min max are inclusive and no easy way to mix
      // them).
      if (generator->getMinMaxKeys().entries() &&
         (getSearchKey()->getBeginKeyValues()[0] !=
          getSearchKey()->getEndKeyValues()[0]) &&
         (!getSearchKey()->isBeginKeyExclusive() ||
          !getSearchKey()->isEndKeyExclusive())) {

        // The keys of the scan.
        const ValueIdList &keys = getIndexDesc()->getIndexKey();
        ValueId      minMaxKeyCol = keys[0];
        IndexColumn *ixCol = (IndexColumn *) (minMaxKeyCol.getItemExpr());
        BaseColumn  *baseCol = NULL;
        ValueId      underlyingCol;
        NABoolean    needToComputeActualMinMax = FALSE;
        ItemExpr    *computedColExpr = NULL;
        
        // The candidate values for min and max.
        const ValueIdList &minMaxKeys = generator->getMinMaxKeys();
    
        CollIndex keyIdx = NULL_COLL_INDEX;

        // Determine how min/max is related to begin/end.  depends
        // on ordering (ASC vs DESC) and scan direction (forward vs
        // reverse)
        NABoolean ascKey = 
          getIndexDesc()->getNAFileSet()->getIndexKeyColumns().isAscending(0);

        if(getReverseScan())
          ascKey = !ascKey;

        // If the leading key column is a divisioning column, then
        // look for min/max values of an underlying column
        GenAssert(ixCol->getOperatorType() == ITM_INDEXCOLUMN,
                  "unexpected object type");
        baseCol = 
          (BaseColumn *) (((IndexColumn *) ixCol)->getDefinition().getItemExpr());
        GenAssert(baseCol->getOperatorType() == ITM_BASECOLUMN,
                  "unexpected object type");
        if (baseCol->getNAColumn()->isDivisioningColumn()) {
          ValueIdSet underlyingCols;
          baseCol->getUnderlyingColumnsForCC(underlyingCols);

          if (underlyingCols.entries() == 1) {
            // We have a leading division column that's computed from
            // 1 base column, now get the underlying column and the
            // divisioning expression
            needToComputeActualMinMax = TRUE;
            underlyingCols.getFirst(minMaxKeyCol);
            computedColExpr = baseCol->getComputedColumnExpr().getItemExpr();
            BaseColumn  *underlyingBaseCol =
              (BaseColumn *) minMaxKeyCol.getItemExpr();
            GenAssert(underlyingBaseCol->getOperatorType() == ITM_BASECOLUMN,
                      "unexpected object type");
            // the computed column expression has been rewritten to use
            // VEGRefs, so get the corresponding VEGRef for the underlying column
            underlyingCol = underlyingBaseCol->getTableDesc()->
              getColumnVEGList()[underlyingBaseCol->getColNumber()];
          }
        }

        // Check all the candidate values.  If any one of them matches
        // the leading key of this scan, then select it for use in the
        // begin/end key value of the leading key.

        // Scalar min/max functions cause an exponential growth when
        // combined with each other, see ItmScalarMinMax::codeGen()
        Int32 limitItems = 3 ; // use at most 3

        for(CollIndex i = 0; i < minMaxKeys.entries() && limitItems; i++) {
          ValueId mmKeyId = minMaxKeys[i];
          if(mmKeyId != NULL_VALUE_ID) {
            ItemExpr *mmItem = mmKeyId.getItemExpr();
      
            if (mmItem->getOperatorType() == ITM_VEG_PREDICATE) {
              VEGPredicate *vPred = (VEGPredicate *)mmItem;
              const ValueIdSet &members = vPred->getVEG()->getAllValues();

              if (members.contains(minMaxKeyCol)) {

                // some other operator is producing min/max values
                // for our leading key column, now check whether we
                // can use them

                keyIdx = i;

                // Indicate in the 'will use' list that we will use these
                // min/max values.  This will indicate to the HashJoin that
                // it should produce these values.
                generator->getWillUseMinMaxKeys()[keyIdx] =
                  generator->getMinMaxKeys()[keyIdx];
                addMinMaxHJColumn(baseCol->getValueId());

                limitItems-- ; // one more is used

                // If we can use a min/max value for the begin key, do so...
                if(!getSearchKey()->isBeginKeyExclusive()) {
                  ItemExpr *keyPred = getBeginKeyPred()[0].getItemExpr();
                  ItemExpr *currentBeg = keyPred->child(1);

                  // Get the proper begin key (min or max) that came from
                  // the HashJoin
                  ValueId hashJoinBeg = (ascKey ?
                                         generator->getMinVals()[keyIdx] :
                                         generator->getMaxVals()[keyIdx]);

                  // Construct an expression which determines at runtime
                  // which BK to use.  Either the existing one or the one
                  // coming from HashJoin whichever is larger (smaller).
                  // 
                  ItemExpr *newBeg = hashJoinBeg.getItemExpr();

                  if (needToComputeActualMinMax) {
                    ValueIdMap divExprMap;
                    ValueId computedBeg;

                    // If hashJoinBeg is :sysHV1 and the computed column
                    // expression is A/100, then the begin value for
                    // the computed column is :sysHV1/100. Do this
                    // rewrite by using a ValueIdMap
                    divExprMap.addMapEntry(underlyingCol, hashJoinBeg);
                    divExprMap.rewriteValueIdDown(computedColExpr->getValueId(),
                                                  computedBeg);
                    newBeg = computedBeg.getItemExpr();
                  }

                  newBeg = new (generator->wHeap())
                    ItmScalarMinMax((ascKey ? ITM_SCALAR_MAX : ITM_SCALAR_MIN),
                                    currentBeg, 
                                    newBeg);
                  newBeg->synthTypeAndValueId();
                  // Replace the RHS of the key pred.
                  keyPred->child(1) = newBeg->getValueId();

                  // The value coming from the HashJoin must be in out inputs.
                  getGroupAttr()->addCharacteristicInputs(hashJoinBeg);

                  // And we must pull those values from the HashJoin.
                  pulledNewInputs += hashJoinBeg;
                  availableValues += hashJoinBeg;
                }

                // If we can use a min/max value for the end key, do so...
                if(!getSearchKey()->isEndKeyExclusive()) {
                  ItemExpr *keyPred = getEndKeyPred()[0].getItemExpr();
                  ItemExpr *currentEnd = keyPred->child(1);

                  // Get the proper end key (max or min) that came from
                  // the HashJoin
                  ValueId hashJoinEnd = (ascKey ?
                                         generator->getMaxVals()[keyIdx] :
                                         generator->getMinVals()[keyIdx]);

                  // Construct an expression which determines at runtime
                  // which EK to use.  Either the existing one or the one
                  // coming from HashJoin whichever is smaller (larger).
                  // 
                  ItemExpr *newEnd = hashJoinEnd.getItemExpr();

                  if (needToComputeActualMinMax) {
                    ValueIdMap divExprMap;
                    ValueId computedEnd;

                    divExprMap.addMapEntry(underlyingCol, hashJoinEnd);
                    divExprMap.rewriteValueIdDown(computedColExpr->getValueId(),
                                                  computedEnd);
                    newEnd = computedEnd.getItemExpr();
                  }

                  newEnd = new (generator->wHeap())
                    ItmScalarMinMax((ascKey ? ITM_SCALAR_MIN : ITM_SCALAR_MAX),
                                    currentEnd, 
                                    newEnd);
                  newEnd->synthTypeAndValueId();

                  // Replace the RHS of the key pred.
                  keyPred->child(1) = newEnd->getValueId();

                  // The value coming from the HashJoin must be in out inputs.
                  getGroupAttr()->addCharacteristicInputs(hashJoinEnd);

                  // And we must pull those values from the HashJoin.
                  pulledNewInputs += hashJoinEnd;
                  availableValues += hashJoinEnd;
                }
              }
            }
          }
        }
      }
    }
  else
    {
      // Hive table scan (HBase scan has executor preds set up already)
      if (isHiveTable())
        setExecutorPredicates(selectionPred());

      // Rebuild the executor  predicate tree
      executorPred().replaceVEGExpressions
	(availableValues,
	 getGroupAttr()->getCharacteristicInputs(),
	 FALSE, // no need for key predicate generation here
	 &vegPairs,
	 TRUE);

      if (isHiveTable())
        {
          // assign individual files and blocks to each ESPs
          ((NodeMap *) getPartFunc()->getNodeMap())->assignScanInfos(hiveSearchKey_);
          if (CmpCommon::getDefault(USE_LIBHDFS) == DF_ON)
             generator->setProcessLOB(TRUE);
	  
	  // flag set for HBase scan in HbaseAccess::preCodeGen
	  // unique scan unlikely for hive scans except 
	  // with predicate on virtual cols.
	  if (!(searchKey() && searchKey()->isUnique()))
	    generator->oltOptInfo()->setMultipleRowsReturned(TRUE);
        }
    }

  
  // Selection predicates are not needed anymore:
  selectionPred().clear();

  // Add the sampled columns to the set of available values. This is
  // basically a kluge to get the GroupAttributes right.

  availableValues += sampledColumns();

  // This call also rewrites predicates
  // $$$ Does it need vegPairs too? $$$
  getGroupAttr()->resolveCharacteristicOutputs
    (availableValues,
     getGroupAttr()->getCharacteristicInputs());

  generator->oltOptInfo()->mayDisableOperStats(&oltOptInfo());
  if (isHbaseTable() || isSeabaseTable()) {
     int beginTransForSelect = ActiveSchemaDB()->getDefaults().getAsLong(BEGIN_TRANSACTION_FOR_SELECT);
     switch (beginTransForSelect) {
        case 2: 
           if (accessOptions().accessType() != TransMode::SKIP_CONFLICT_ACCESS_
             && accessOptions().accessType() != TransMode::READ_UNCOMMITTED_ACCESS_)
              generator->setTransactionFlag(TRUE);
           // no break here because the transaction is required for updatable select when it is 2
        case 1:
           if (generator->updatableSelect())
              generator->setTransactionFlag(TRUE);
           break;
        default:
           break;
     }
  }
  markAsPreCodeGenned();
  return this;
} // FileScan::preCodeGen()


RelExpr * GenericUpdate::preCodeGen(Generator * generator,
				    const ValueIdSet & externalInputs,
				    ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! RelExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  // Determine whether OLT optimization must be avoided.
  if (getGroupAttr()->isEmbeddedUpdateOrDelete())
    {
      generator->oltOptInfo()->setOltMsgOpt(FALSE);

      generator->setUpdAbortOnError(TRUE);
      generator->setUpdSavepointOnError(FALSE);
      generator->setUpdPartialOnError(FALSE);
      generator->setUpdErrorOnError(FALSE);
    }

  if ((accessOptions().accessType() == TransMode::SKIP_CONFLICT_ACCESS_) ||
      (getGroupAttr()->isStream()) ||
      (newRecBeforeExprArray().entries() > 0)) // set on rollback
    {
      generator->oltOptInfo()->setOltEidOpt(FALSE);
      oltOptInfo().setOltEidOpt(FALSE);
      setExpandShortRows(FALSE);

      generator->setUpdAbortOnError(TRUE);
      generator->setUpdSavepointOnError(FALSE);
      generator->setUpdPartialOnError(FALSE);
      generator->setUpdErrorOnError(FALSE);
    }

  // If RI, IM, MV or triggers are being used, abort on error.
  // This is because internal data consistency
  // cannot be guaranteed for these cases.
  if ((getInliningInfo().hasInlinedActions()) ||
      (getInliningInfo().isEffectiveGU()))
    {
      // cannot do partial updates.
      generator->setUpdPartialOnError(FALSE);

      if (CmpCommon::getDefault(COMP_BOOL_206) == DF_ON)
	{
	  if (NOT ((getInliningInfo().hasTriggers()) ||
		   (getInliningInfo().hasRI()) ||
		   (getInliningInfo().hasIM()) ||
		   (getInliningInfo().isMVLoggingInlined())))
	    {
	      generator->setUpdAbortOnError(TRUE);
	      generator->setUpdSavepointOnError(FALSE);
	      generator->setUpdErrorOnError(FALSE);
	    }
	  else
	    generator->setUpdErrorOnError(FALSE);
	}
      else
	{
	  // abort on error for non-IM cases(RI,MV,Trig).
	  if ((NOT getInliningInfo().hasIM()) ||
	      (getInliningInfo().hasRI()))
	    {
	      generator->setUpdAbortOnError(TRUE);
	      generator->setUpdSavepointOnError(FALSE);
	      generator->setUpdErrorOnError(FALSE);
	    }
	  else
	    generator->setUpdErrorOnError(FALSE);
	}
    }

  // If RI, MV or triggers are being used, turn off the lean optimization for
  // the complete plan; all other optimizations will still apply.
  if ( generator->oltOptInfo()->oltEidLeanOpt() &&
       ( getInliningInfo().hasTriggers()  ||
         getInliningInfo().hasRI()        ||
         getInliningInfo().isMVLoggingInlined() ) )
  {
    generator->oltOptInfo()->setOltEidLeanOpt(FALSE);
    oltOptInfo().setOltEidLeanOpt(FALSE);
  }
 if (updateCurrentOf())
    generator->setAqrEnabled(FALSE);


 if ((((NATable*)getTableDesc()->getNATable())->getTableType() == ExtendedQualName::GHOST_TABLE) &&

     (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) &&

     (CmpCommon::getDefault(AUTO_QUERY_RETRY) == DF_SYSTEM) &&

     (NOT generator->aqrEnabled()))

   {

     generator->setAqrEnabled(TRUE);

   }
  // Accumulate the values that are provided as inputs by my parent
  // together with the values that are produced as outputs by my
  // children. Use these values for rewriting the VEG expressions.
  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);

  newRecExpr_.replaceVEGExpressions
                 (availableValues,
		  getGroupAttr()->getCharacteristicInputs());

  newRecBeforeExpr_.replaceVEGExpressions
                 (availableValues,
		  getGroupAttr()->getCharacteristicInputs());

  executorPred().replaceVEGExpressions
	          (availableValues,
		    getGroupAttr()->getCharacteristicInputs());

  // VEGPredicates that are key predicates but are also replicated
  // in the executor predicates must be replaced with the same
  // expression in both places after they are rewritten.
  // Therefore, we want replaceVEGExpressions() processing to be
  // idempotent.  By passing the VEGRewritePairs data structure
  // to replaceVEGExpressions(), we get idempotence.

  VEGRewritePairs lookup(generator->wHeap());   // so replaceVEGExpressions will be idempotent

  if (getSearchKey() == NULL)
    {
      // Begin and end key preds may already be available.
      beginKeyPred_.replaceVEGExpressions
                       (availableValues,
			getGroupAttr()->getCharacteristicInputs(),
			FALSE, // no need for key predicate generation here
			&lookup);
      endKeyPred_.replaceVEGExpressions
                     (availableValues,
		      getGroupAttr()->getCharacteristicInputs(),
		      FALSE,  // no need for key predicate generation here
		      &lookup);

      // In the case of an embedded insert from VALUES,
      // any predicates need to have their VEGreferences resolved.
      if (getGroupAttr()->isEmbeddedInsert())
	{

          NABoolean replicatePredicates = TRUE;

	  // Rebuild the executor predicate tree
	  executorPred().replaceVEGExpressions
			    (availableValues,
			     getGroupAttr()->getCharacteristicInputs(),
			     FALSE,  // no need for key predicate generation
			     &lookup,
			     replicatePredicates
			    );
	}
    }
  else
    {
      // Build begin and end key predicates from the search key structure.

      //## It *might* be a good idea to add here:
      //##   CMPASSERT(beginKeyPred_.isEmpty() && endKeyPred_.isEmpty());
      //## as that *seems* to be the assumption here.
      //## (But I haven't the time to make the change and test it.)

      ValueIdSet& keyPred = getSearchKey()->keyPredicates();

      NABoolean replicatePredicates = TRUE;

      // Rebuild the search key expressions
      keyPred.replaceVEGExpressions
                 (availableValues,
		  getGroupAttr()->getCharacteristicInputs(),
		  FALSE, // no need for key predicate generation
		  &lookup,
		  replicatePredicates);

      // Rebuild the executor  predicate tree
      executorPred().replaceVEGExpressions
	                (availableValues,
			 getGroupAttr()->getCharacteristicInputs(),
			 FALSE,  // no need for key predicate generation
			 &lookup,
			 replicatePredicates
			);

      // Generate the begin and end keys.
      generateKeyExpr(getGroupAttr()->getCharacteristicInputs(),
		      getIndexDesc()->getIndexKey(),
		      getSearchKey()->getBeginKeyValues(),
		      beginKeyPred_,
		      generator);
      generateKeyExpr(getGroupAttr()->getCharacteristicInputs(),
		      getIndexDesc()->getIndexKey(),
		      getSearchKey()->getEndKeyValues(),
		      endKeyPred_,
		      generator);
    }

  // ---------------------------------------------------------------------
  // Rewrite the check constraint expressions.
  // ---------------------------------------------------------------------
  checkConstraints().replaceVEGExpressions
                        (availableValues,
			 getGroupAttr()->getCharacteristicInputs());

  generator->setFoundAnUpdate(TRUE);
  generator->setPartnAccessChildIUD();

#ifdef _DEBUG
  // Compile in the index maintenance ... just for testing
  //
  if(getenv("IM_COMPILE"))
    generator->imUpdateRel() = this;
#endif

  if (oltOptLean() &&
      ((isinBlockStmt()) ||
       (getTableDesc()->getNATable()->hasAddedColumn()) ||
       (getTableDesc()->getNATable()->hasVarcharColumn())))
  {
    oltOptInfo().setOltEidLeanOpt(FALSE);
  }

  generator->setSkipUnavailablePartition(FALSE);

  if (isMtsStatement())
    generator->setEmbeddedIUDWithLast1(TRUE) ;

  if (isMerge())
    {
      // Accumulate the values that are provided as inputs by my parent
      // together with the values that are produced as outputs by my
      // children. Use these values for rewriting the VEG expressions.
      ValueIdSet availableValues;
      getInputValuesFromParentAndChildren(availableValues);
      
      mergeInsertRecExpr().replaceVEGExpressions
	(availableValues,
	 getGroupAttr()->getCharacteristicInputs());

      mergeUpdatePred().replaceVEGExpressions
	(availableValues,
	 getGroupAttr()->getCharacteristicInputs());

      ValueIdList tempVIDlist;
      getTableDesc()->getIdentityColumn(tempVIDlist);
      NAColumn *identityCol = NULL;
      if (tempVIDlist.entries() > 0)
	{
	  ValueId valId = tempVIDlist[0];
	  identityCol = valId.getNAColumn();
	}

      if (((getOperatorType() == REL_HBASE_DELETE) ||
           (getOperatorType() == REL_HBASE_UPDATE)) &&
          (getTableDesc()->getNATable()->getClusteringIndex()->hasSyskey()))
	{
	  *CmpCommon::diags() << DgSqlCode(-3241) 
			      << DgString0(" SYSKEY not allowed.");
	  GenExit();
	}

      if ((getOperatorType() != REL_HBASE_UPDATE) &&
          (mergeInsertRecExpr().entries() > 0) &&
          (CmpCommon::getDefault(COMP_BOOL_175) == DF_OFF))
	{
          // MERGE with INSERT is limited to HBase updates unless
          // the CQD is on
	  *CmpCommon::diags() << DgSqlCode(-3241)
			      << DgString0(" This MERGE is not allowed with INSERT.");
	  GenExit();
	}

      if (oltOpt())
	{
	  // if no update expr and only insert expr is specified for 
	  // this MERGE stmt, turn off olt opt.
	  // 
	  if (newRecExprArray().entries() == 0)
	    oltOptInfo().setOltEidOpt(FALSE);

	  oltOptInfo().setOltEidLeanOpt(FALSE);
	}

      generator->setUpdErrorOnError(FALSE);
      generator->setUpdSavepointOnError(FALSE);
    } // isMerge

  generator->oltOptInfo()->mayDisableOperStats(&oltOptInfo());

  // Part of the fix for Soln 10-100425-9755.  Don't AQR a 
  // positioned update/delete because part of the recovery
  // for the error that triggers the AQR is rollback transaction
  // and this causes the referenced cursor to be closed.  The other
  // part of the fix is in compiler cache: positioned update/deletes
  // will not be cached, and this should reduce the need to handle
  // errors with AQR, e.g., timestamp mismatch errors.
  if (updateCurrentOf())
    generator->setAqrEnabled(FALSE);


  if ((((NATable*)getTableDesc()->getNATable())->getTableType() == ExtendedQualName::GHOST_TABLE) &&

      (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) &&

      (CmpCommon::getDefault(AUTO_QUERY_RETRY) == DF_SYSTEM) &&

      (NOT generator->aqrEnabled()))

    {

      generator->setAqrEnabled(TRUE);

    }
  if (getTableDesc()->getNATable()->hasLobColumn())
    {
      oltOptInfo().setOltOpt(FALSE);
      generator->oltOptInfo()->setOltOpt(FALSE);
      //enabling AQR to take care of the lock conflict error 8558 that
      // should be retried.
      //      generator->setAqrEnabled(FALSE);
      generator->setUpdAbortOnError(TRUE);
      generator->setUpdSavepointOnError(FALSE);
    }

  if ((isNoRollback()) ||
      (generator->getTransMode()->getRollbackMode() == TransMode::NO_ROLLBACK_))
    {
      generator->setWithNoRollbackUsed(isNoRollback());
      if (CmpCommon::getDefault(AQR_WNR) == DF_OFF)
        generator->setAqrEnabled(FALSE);
    }

  if (((getInliningInfo().hasInlinedActions()) ||
       (getInliningInfo().isEffectiveGU())) &&
      (getInliningInfo().hasRI()))
    {
      generator->setRIinliningForTrafIUD(TRUE);
    }

  if (precondition_.entries() > 0)
  {
    ValueIdSet availableValues;
    getInputValuesFromParentAndChildren(availableValues);  
    precondition_.
      replaceVEGExpressions(availableValues,
			    getGroupAttr()->getCharacteristicInputs());
  }

  markAsPreCodeGenned();

  return this;
} // GenericUpdate::preCodeGen()

RelExpr * Update::preCodeGen(Generator * generator,
			     const ValueIdSet & externalInputs,
			     ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;
  
  if (! GenericUpdate::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  markAsPreCodeGenned();

  return this;
}

RelExpr * MergeUpdate::preCodeGen(Generator * generator,
				  const ValueIdSet & externalInputs,
				  ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;
  
  if (! Update::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  markAsPreCodeGenned();

  return this;
}

RelExpr * UpdateCursor::preCodeGen(Generator * generator,
				   const ValueIdSet & externalInputs,
				   ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! Update::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  // primary key columns cannot be updated, yet. After RI support
  // is in, they could be updated.
   const NAColumnArray & key_column_array =
     getTableDesc()->getNATable()->getClusteringIndex()->getIndexKeyColumns();

  ValueIdSet& val_id_set = newRecExpr();
  ValueId val_id;
  for (val_id = val_id_set.init(); val_id_set.next(val_id); val_id_set.advance(val_id))
    {
      ItemExpr * item_expr = val_id.getItemExpr();

      for (short i = 0; i < getTableDesc()->getNATable()->getKeyCount(); i++)
	{
	  const char * key_colname = key_column_array[i]->getColName();
	  const char * upd_colname = ((BaseColumn *)
	  			      (item_expr->child(0)->castToItemExpr()))->
				      getColName();

	  if ((strcmp(key_colname, upd_colname) == 0) &&
              (item_expr->getOperatorType() == ITM_ASSIGN) &&
              (((Assign*)item_expr)->isUserSpecified()))
	    {
	      *CmpCommon::diags() << DgSqlCode(-4033) 
				  << DgColumnName(key_colname);

	      GenExit();
	    }
	}
    }
  generator->oltOptInfo()->mayDisableOperStats(&oltOptInfo());
  markAsPreCodeGenned();

  return this;
} // UpdateCursor::preCodeGen()

RelExpr * Delete::preCodeGen(Generator * generator,
                             const ValueIdSet & externalInputs,
                             ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;
  
  if (! GenericUpdate::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  markAsPreCodeGenned();

  return this;
}

RelExpr * MergeDelete::preCodeGen(Generator * generator,
				  const ValueIdSet & externalInputs,
				  ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;
  
  if (! Delete::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  markAsPreCodeGenned();

  return this;
}

static NABoolean hasColReference(ItemExpr * ie)
{
  if (! ie)
    return FALSE;

  if ((ie->getOperatorType() == ITM_BASECOLUMN) ||
      (ie->getOperatorType() == ITM_INDEXCOLUMN) ||
      (ie->getOperatorType() == ITM_REFERENCE))
    return TRUE;

  for (Lng32 i = 0; i < ie->getArity(); i++)
    {
      if (hasColReference(ie->child(i)))
	return TRUE;
    }
  
  return FALSE;
}

void HbaseAccess::addReferenceFromItemExprTree(ItemExpr * ie,
                                               NABoolean addCol, NABoolean addHBF,
                                               ValueIdSet &colRefVIDset)
{
  if (! ie)
    return;
  
  if ((ie->getOperatorType() == ITM_BASECOLUMN) ||
      (ie->getOperatorType() == ITM_INDEXCOLUMN) ||
      (ie->getOperatorType() == ITM_REFERENCE))
    {
      if (addCol)
        colRefVIDset.insert(ie->getValueId());

      return;
    }

  if (ie->getOperatorType() == ITM_HBASE_TIMESTAMP)
    {
      if (addHBF)
        {
          colRefVIDset.insert(ie->getValueId());
        }

      return;
    }

  if (ie->getOperatorType() == ITM_HBASE_VERSION)
    {
      if (addHBF)
        {
          colRefVIDset.insert(ie->getValueId());
        }

      return;
    }

  for (Lng32 i = 0; i < ie->getArity(); i++)
    {
      addReferenceFromItemExprTree(ie->child(i), addCol, addHBF, colRefVIDset);
    }
  
  return;
}

void HbaseAccess::addColReferenceFromVIDlist(const ValueIdList &exprList,
					     ValueIdSet  &colRefVIDset)
{
  for (CollIndex i = 0; i < exprList.entries(); i++)
    {
      addReferenceFromItemExprTree(exprList[i].getItemExpr(), 
                                   TRUE, FALSE, colRefVIDset);
    }
}

void HbaseAccess::addReferenceFromVIDset(const ValueIdSet &exprList,
                                         NABoolean addCol, NABoolean addHBF,
                                         ValueIdSet  &colRefVIDset)
{
  for (ValueId v = exprList.init(); exprList.next(v); exprList.advance(v))
    {
      addReferenceFromItemExprTree(v.getItemExpr(), addCol, addHBF, colRefVIDset);
    }
}

void HbaseAccess::addColReferenceFromRightChildOfVIDarray(ValueIdArray &exprList,
							  ValueIdSet &colRefVIDset)
{

  for (CollIndex i = 0; i < exprList.entries(); i++)
    {
      addReferenceFromItemExprTree(exprList[i].getItemExpr()->child(1), 
                                   TRUE, FALSE, colRefVIDset);
    }
}

static NABoolean isEqGetExpr(ItemExpr * ie, ValueId &vid, NABoolean &isConstParam,
			     const char * colName)
{
  NABoolean found = FALSE;
  isConstParam = FALSE;

  if (ie && ie->getOperatorType() == ITM_EQUAL)
    {
      ItemExpr * child0 = ie->child(0)->castToItemExpr();
      ItemExpr * child1 = ie->child(1)->castToItemExpr();
      
      if ((ie->child(0)->getOperatorType() == ITM_BASECOLUMN) &&
	  (((BaseColumn*)ie->child(0)->castToItemExpr())->getNAColumn()->getColName() == colName) &&
	  (NOT hasColReference(ie->child(1))))
	{
	  if (ie->child(1)->getOperatorType() == ITM_CONSTANT)
	    {
	      found = TRUE;
	      vid = ie->child(1)->getValueId();
	    }
	  else if (ie->child(1)->getOperatorType() == ITM_CACHE_PARAM)
	    {
	      found = TRUE;
	      isConstParam = TRUE;
	      vid = ie->child(1)->getValueId();
	    }
	}
      else if ((ie->child(1)->getOperatorType() == ITM_BASECOLUMN) &&
	       (((BaseColumn*)ie->child(1)->castToItemExpr())->getNAColumn()->getColName() == colName) &&
	       (NOT hasColReference(ie->child(0))))
	{
	  if (ie->child(0)->getOperatorType() == ITM_CONSTANT)
	    {
	      found = TRUE;
	      vid = ie->child(0)->getValueId();
	    }
	  else if (ie->child(0)->getOperatorType() == ITM_CACHE_PARAM)
	    {
	      found = TRUE;
	      isConstParam = TRUE;
	      vid = ie->child(0)->getValueId();
	    }
	}
      else if ((ie->child(0)->getOperatorType() == ITM_INDEXCOLUMN) &&
	       (((IndexColumn*)ie->child(0)->castToItemExpr())->getNAColumn()->getColName() == colName) &&
	       (NOT hasColReference(ie->child(1))))
	{
	  if (ie->child(1)->getOperatorType() == ITM_CONSTANT)
	    {
	      found = TRUE;
	      vid = ie->child(1)->getValueId();
	    }
	  else if (ie->child(1)->getOperatorType() == ITM_CACHE_PARAM)
	    {
	      found = TRUE;
	      isConstParam = TRUE;
	      vid = ie->child(1)->getValueId();
	    }
	}
      else if ((ie->child(1)->getOperatorType() == ITM_INDEXCOLUMN) &&
	       (((IndexColumn*)ie->child(1)->castToItemExpr())->getNAColumn()->getColName() == colName) &&
	       (NOT hasColReference(ie->child(0))))
	{
	  if (ie->child(0)->getOperatorType() == ITM_CONSTANT)
	    {
	      found = TRUE;
	      vid = ie->child(0)->getValueId();
	    }
	  else if (ie->child(0)->getOperatorType() == ITM_CACHE_PARAM)
	    {
	      found = TRUE;
	      isConstParam = TRUE;
	      vid = ie->child(0)->getValueId();
	    }
	}
      else if ((ie->child(0)->getOperatorType() == ITM_REFERENCE) &&
	       (((ColReference*)ie->child(0)->castToItemExpr())->getCorrNameObj().getQualifiedNameObj().getObjectName() == colName)  &&
	       (NOT hasColReference(ie->child(1))))
	{
	  if (ie->child(1)->getOperatorType() == ITM_CONSTANT)
	    {
	      found = TRUE;
	      vid = ie->child(1)->getValueId();
	    }
	  else if (ie->child(1)->getOperatorType() == ITM_CACHE_PARAM)
	    {
	      found = TRUE;
	      isConstParam = TRUE;
	      vid = ie->child(1)->getValueId();
	    }
	}
      else if ((ie->child(1)->getOperatorType() == ITM_REFERENCE) &&
	       (((ColReference*)ie->child(1)->castToItemExpr())->getCorrNameObj().getQualifiedNameObj().getObjectName() == colName)  &&
	       (NOT hasColReference(ie->child(0))))
	{
	  if (ie->child(0)->getOperatorType() == ITM_CONSTANT)
	    {
	      found = TRUE;
	      vid = ie->child(0)->getValueId();
	    }
	  else if (ie->child(0)->getOperatorType() == ITM_CACHE_PARAM)
	    {
	      found = TRUE;
	      isConstParam = TRUE;
	      vid = ie->child(0)->getValueId();
	    }
	}
    }

  return found;
}

RelExpr * HbaseDelete::preCodeGen(Generator * generator,
				  const ValueIdSet & externalInputs,
				  ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  // if a column list is specified, make sure all column names are of valid hbase
  // column name format ("ColFam:ColNam")
  if (csl())
    {
      for (Lng32 i = 0; i < csl()->entries(); i++)
	{
	  const NAString * nas = (*csl())[i];

	  std::string colFam;
	  std::string colName;
	  if (nas)
	    {
	      ExFunctionHbaseColumnLookup::extractColFamilyAndName(
								   nas->data(), -1, FALSE, colFam, colName);
	    }
	  
	  if (colFam.empty())
	    {
	      *CmpCommon::diags() << DgSqlCode(-1426)
				  << DgString0(nas->data());
	      GenExit();
	    }
	  
	} // for
    } // if

  if (!processConstHBaseKeys(
           generator,
           this,
           getSearchKey(),
           getIndexDesc(),
           executorPred(),
           getHbaseSearchKeys(),
           listOfDelUniqueRows_,
           listOfDelSubsetRows_))
    return NULL;

  if (! Delete::preCodeGen(generator, externalInputs, pulledNewInputs))
    return NULL;

  if (((getTableDesc()->getNATable()->isHbaseRowTable()) ||
       (getTableDesc()->getNATable()->isHbaseCellTable())) &&
      (producesOutputs()))
    {
      *CmpCommon::diags() << DgSqlCode(-1425)
			  << DgTableName(getTableDesc()->getNATable()->getTableName().
					 getQualifiedNameAsAnsiString())
                          << DgString0("Reason: Cannot return values from an hbase insert, update or delete.");
      GenExit();
    }

   NABoolean isAlignedFormat = getTableDesc()->getNATable()->isAlignedFormat(getIndexDesc());

  if  (producesOutputs()) 
    {
      retColRefSet_ = getIndexDesc()->getIndexColumns();
    }
  else
    {
      ValueIdSet colRefSet;

      // create the list of columns that need to be retrieved from hbase .
      // first add all columns referenced in the executor pred.
      HbaseAccess::addReferenceFromVIDset(executorPred(), TRUE, TRUE, colRefSet);

      if ((getTableDesc()->getNATable()->getExtendedQualName().getSpecialType() == ExtendedQualName::INDEX_TABLE))
        {
	  for (ValueId valId = executorPred().init();
	       executorPred().next(valId);
	       executorPred().advance(valId))
            {
              ItemExpr * ie = valId.getItemExpr();
              if (ie->getOperatorType() == ITM_EQUAL)
                {
                  BiRelat * br = (BiRelat*)ie;
                  br->setSpecialNulls(TRUE);
                }
            }
        } // index_table


      if ((getTableDesc()->getNATable()->isHbaseRowTable()) ||
	  (getTableDesc()->getNATable()->isHbaseCellTable()) ||
           isAlignedFormat)
	{
	  for (Lng32 i = 0; i < getIndexDesc()->getIndexColumns().entries(); i++)
	    {
	      retColRefSet_.insert(getIndexDesc()->getIndexColumns()[i]);
	    }
        }

      for (ValueId valId = colRefSet.init();
           colRefSet.next(valId);
           colRefSet.advance(valId))
        {
          ValueId dummyValId;
          if (NOT getGroupAttr()->getCharacteristicInputs().referencesTheGivenValue(valId, dummyValId))
            {
              if ((valId.getItemExpr()->getOperatorType() == ITM_HBASE_TIMESTAMP) ||
                  (valId.getItemExpr()->getOperatorType() == ITM_HBASE_VERSION))
                {
                  *CmpCommon::diags() << DgSqlCode(-3242)
                                      << DgString0("Illegal use of Hbase Timestamp or Hbase Version function.");

                  GenExit();
                }

              retColRefSet_.insert(valId);
            }
        }

      if (NOT ((getTableDesc()->getNATable()->isHbaseRowTable()) ||
               (getTableDesc()->getNATable()->isHbaseCellTable()) ||
               (isAlignedFormat)))
        {
          // add all the key columns. If values are missing in hbase, then atleast the key
          // value is needed to retrieve a row. 
          HbaseAccess::addColReferenceFromVIDlist(getIndexDesc()->getIndexKey(), retColRefSet_);
        }

      if (getTableDesc()->getNATable()->hasLobColumn())
        {
          for (Lng32 i = 0; i < getIndexDesc()->getIndexColumns().entries(); i++)
            {
              const ValueId vid = getIndexDesc()->getIndexColumns()[i];
              retColRefSet_.insert(vid);
            }
        }
    }

  NABoolean inlinedActions = FALSE;
  if ((getInliningInfo().hasInlinedActions()) ||
      (getInliningInfo().isEffectiveGU()))
    inlinedActions = TRUE;
 
  NABoolean isUnique = FALSE;
  if (listOfDelSubsetRows_.entries() == 0)
    {
      if ((getSearchKey() && getSearchKey()->isUnique()) &&
	  (listOfDelUniqueRows_.entries() == 0))
	isUnique = TRUE;
      else if ((NOT (getSearchKey() && getSearchKey()->isUnique())) &&
	       (listOfDelUniqueRows_.entries() == 1) &&
	       (listOfDelUniqueRows_[0].rowIds_.entries() == 1))
	isUnique = TRUE;
    }
 
  NABoolean hbaseRowsetVSBBopt = 
    (CmpCommon::getDefault(HBASE_ROWSET_VSBB_OPT) == DF_ON);
  if ((getTableDesc()->getNATable()->isHbaseMapTable()) ||
      (getTableDesc()->getNATable()->isHbaseRowTable()) ||
      (getTableDesc()->getNATable()->isHbaseCellTable()))
    hbaseRowsetVSBBopt = FALSE;

  if (getInliningInfo().isIMGU()) {
     // There is no need to do checkAndDelete for IM
     canDoCheckAndUpdel() = FALSE;
     uniqueHbaseOper() = FALSE;
     if ((generator->oltOptInfo()->multipleRowsReturned()) &&
	  (hbaseRowsetVSBBopt) &&
         (NOT generator->isRIinliningForTrafIUD()) &&
         (NOT getTableDesc()->getNATable()->hasLobColumn()))
       uniqueRowsetHbaseOper() = TRUE;
  }
  else
  if (isUnique)
    {
      //If this unique delete is not part of a rowset operation , 
      //don't allow it to be cancelled. 
      if (!generator->oltOptInfo()->multipleRowsReturned())
	generator->setMayNotCancel(TRUE);
      uniqueHbaseOper() = TRUE;
      canDoCheckAndUpdel() = FALSE;
      if ((NOT producesOutputs()) &&
	  (NOT  inlinedActions) &&
          (executorPred().isEmpty()))
	{
	  if ((generator->oltOptInfo()->multipleRowsReturned()) &&
	      (hbaseRowsetVSBBopt) &&
	      (NOT generator->isRIinliningForTrafIUD()) &&
              (NOT getTableDesc()->getNATable()->hasLobColumn()))
	    uniqueRowsetHbaseOper() = TRUE;
	  else if ((NOT generator->oltOptInfo()->multipleRowsReturned()) &&
		   (listOfDelUniqueRows_.entries() == 0))
	    {
	      if ((CmpCommon::getDefault(HBASE_CHECK_AND_UPDEL_OPT) == DF_ON) &&
		  (CmpCommon::getDefault(HBASE_SQL_IUD_SEMANTICS) == DF_ON) &&
                  (NOT isAlignedFormat))
	      canDoCheckAndUpdel() = TRUE;
	    }
	}
    }

  if ((producesOutputs()) &&
      ((NOT isUnique) || (getUpdateCKorUniqueIndexKey())))
    {
      // Cannot do olt msg opt if:
      //   -- values are to be returned and unique operation is not being used.
      //   -- or this delete was transformed from an update of pkey/index key
      // set an indication that multiple rows will be returned.
      generator->oltOptInfo()->setMultipleRowsReturned(TRUE);
      generator->oltOptInfo()->setOltCliOpt(FALSE);
    }

  if (getTableDesc()->getNATable()->hasLobColumn())
    {
      canDoCheckAndUpdel() = FALSE;
      uniqueRowsetHbaseOper() = FALSE;
    }

  generator->setUpdSavepointOnError(FALSE);
  generator->setUpdPartialOnError(FALSE);
  
  // if unique oper with no index maintanence and autocommit is on, then
  // do not require a trnsaction. 
  // Use hbase or region transactions. 
  // Hbase guarantees single row consistency.
  Int64 transId = -1;
  if (CmpCommon::getDefault(TRAF_NO_DTM_XN) == DF_ON)
    {
      // no transaction needed
      noDTMxn() = TRUE;
    }
  else if ((uniqueHbaseOper()) &&
           (NOT cursorHbaseOper()) &&
           (NOT uniqueRowsetHbaseOper()) &&
           (NOT inlinedActions) &&
           (generator->getTransMode()->getAutoCommit() == TransMode::ON_) &&
           (! NAExecTrans(0, transId)) &&
           (NOT generator->oltOptInfo()->multipleRowsReturned()))
    {
      // no DTM transaction needed
      useRegionXn() = FALSE;
      if (CmpCommon::getDefault(TRAF_USE_REGION_XN) == DF_ON)
        useRegionXn() = TRUE;
    }
  else
    {
      generator->setTransactionFlag(TRUE);

      if ((NOT uniqueHbaseOper()) ||
	  (cursorHbaseOper()) ||
	  (uniqueRowsetHbaseOper()) ||
	  (inlinedActions) ||
	  (generator->oltOptInfo()->multipleRowsReturned()))
	generator->setUpdAbortOnError(TRUE);
    }

  // flag for hbase tables
  generator->setHdfsAccess(TRUE);
  
  markAsPreCodeGenned();

  return this;  
}

RelExpr * HbaseUpdate::preCodeGen(Generator * generator,
				  const ValueIdSet & externalInputs,
				  ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (getTableDesc()->getNATable()->isHbaseMapTable())
    {
      *CmpCommon::diags() << DgSqlCode(-1425)
			  << DgTableName(getTableDesc()->getNATable()->getTableName().
					 getQualifiedNameAsAnsiString())
                          << DgString0("Reason: update not yet supported.");
      GenExit();
    }

  if (!processConstHBaseKeys(
           generator,
           this,
           getSearchKey(),
           getIndexDesc(),
           executorPred(),
           getHbaseSearchKeys(),
           listOfUpdUniqueRows_,
           listOfUpdSubsetRows_))
    return NULL;

  //  if (! GenericUpdate::preCodeGen(generator, externalInputs, pulledNewInputs))
  //    return NULL;
  if (! UpdateCursor::preCodeGen(generator, externalInputs, pulledNewInputs))
    return NULL;

  CollIndex totalColCount = getTableDesc()->getColumnList().entries();
  NABoolean isAlignedFormat = getTableDesc()->getNATable()->isAlignedFormat(getIndexDesc());

  if (isAlignedFormat &&
      (newRecExprArray().entries() > 0) &&
      (newRecExprArray().entries() <  totalColCount))
    {
      ValueIdArray holeyArray(totalColCount);
      
      Lng32 i;
      for (i = 0; i < newRecExprArray().entries(); i++)
        {
          ItemExpr * assign = newRecExprArray()[i].getItemExpr();
          const NAColumn *nacol = assign->child(0).getNAColumn();
          Lng32 colPos = nacol->getPosition();
          holeyArray.insertAt(colPos, assign->getValueId());
        } // for
      
      for (i = 0; i < totalColCount; i++)
       {
         if (! (holeyArray.used(i)))
           {
             BaseColumn * bc = (BaseColumn*)getTableDesc()->getColumnList()[i].getItemExpr();
             CMPASSERT(bc->getOperatorType() == ITM_BASECOLUMN);

             ValueId srcId = getIndexDesc()->getIndexColumns()[i];
             
             ItemExpr * an = 
               new(generator->wHeap()) Assign(bc, srcId.getItemExpr(), FALSE);
             an->bindNode(generator->getBindWA());
             holeyArray.insertAt(i, an->getValueId());
           } // if
       } // for
      
      newRecExprArray().clear();
      newRecExprArray() = holeyArray;
    } // if aligned

  if ((isMerge()) &&
      (mergeInsertRecExpr().entries() > 0))
    {
      if ((listOfUpdSubsetRows_.entries() > 0) ||
	  (getSearchKey() && (NOT getSearchKey()->isUnique())))
	{
	  *CmpCommon::diags() << DgSqlCode(-3241)
			      << DgString0(" Non-unique ON clause not allowed with INSERT.");
	  GenExit();
	}
    }

  if (((getTableDesc()->getNATable()->isHbaseRowTable()) ||
       (getTableDesc()->getNATable()->isHbaseCellTable())) &&
      (producesOutputs()))
    {
      *CmpCommon::diags() << DgSqlCode(-1425)
			  << DgTableName(getTableDesc()->getNATable()->getTableName().
					 getQualifiedNameAsAnsiString())
                          << DgString0("Reason: Cannot return values from an hbase insert, update or delete.");
      GenExit();
    }

  NABoolean canDoRowsetOper = TRUE;
  NABoolean canDoCheckAndUpdate = TRUE;
  NABoolean needToGetCols = FALSE;
  if (producesOutputs())
    {
      retColRefSet_ = getIndexDesc()->getIndexColumns();
    }
  else
    {
      ValueIdSet colRefSet;

      // create the list of columns that need to be retrieved from hbase .
      // first add all columns referenced in the executor pred.
      HbaseAccess::addReferenceFromVIDset(executorPred(), TRUE, TRUE, colRefSet);

      if ((getTableDesc()->getNATable()->getExtendedQualName().getSpecialType() == ExtendedQualName::INDEX_TABLE))
        {
	  for (ValueId valId = executorPred().init();
	       executorPred().next(valId);
	       executorPred().advance(valId))
            {
              ItemExpr * ie = valId.getItemExpr();
              if (ie->getOperatorType() == ITM_EQUAL)
                {
                  BiRelat * br = (BiRelat*)ie;
                  br->setSpecialNulls(TRUE);
                }
            }
        }

      // add all columns referenced in the right side of the update expr.
      HbaseAccess::addColReferenceFromRightChildOfVIDarray(newRecExprArray(), colRefSet);

      if (isMerge())
	HbaseAccess::addReferenceFromVIDset(mergeUpdatePred(), TRUE, FALSE, colRefSet);

      if ((getTableDesc()->getNATable()->isHbaseRowTable()) ||
	  (getTableDesc()->getNATable()->isHbaseCellTable()) ||
          (isAlignedFormat))
	{
	  for (Lng32 i = 0; i < getIndexDesc()->getIndexColumns().entries(); i++)
	    {
	      retColRefSet_.insert(getIndexDesc()->getIndexColumns()[i]);
	    }
	}
      else
	{
	  for (ValueId valId = colRefSet.init();
	       colRefSet.next(valId);
	       colRefSet.advance(valId))
	    {
	      ValueId dummyValId;
	      if (NOT getGroupAttr()->getCharacteristicInputs().referencesTheGivenValue(valId, dummyValId))
                {
                  if ((valId.getItemExpr()->getOperatorType() == ITM_HBASE_TIMESTAMP) ||
                      (valId.getItemExpr()->getOperatorType() == ITM_HBASE_VERSION))
                    {
                      *CmpCommon::diags() << DgSqlCode(-3242)
                                          << DgString0("Illegal use of Hbase Timestamp or Hbase Version function.");
                      
                      GenExit();
                    }
                  
                  retColRefSet_.insert(valId);
                }
	    }
	}

      if (retColRefSet_.entries() > 0)
	{
	  needToGetCols = TRUE;
	  canDoRowsetOper = FALSE;
	  canDoCheckAndUpdate = FALSE;
	}

      // nullable and added columns in the row may be missing. That will cause
      // a row to not be returned if those are the only columns that are being
      // retrieved. 
      // To make sure that a row is always returned, add the key columns. These are
      // guaranteed to be present in an hbase row.
      HbaseAccess::addColReferenceFromVIDlist(getIndexDesc()->getIndexKey(), retColRefSet_);
    }

  NABoolean inlinedActions = FALSE;
  if ((getInliningInfo().hasInlinedActions()) ||
      (getInliningInfo().isEffectiveGU()))
    inlinedActions = TRUE;
  
  NABoolean isUnique = FALSE;
  if (listOfUpdSubsetRows_.entries() == 0)
    {
      if ((getSearchKey() && getSearchKey()->isUnique()) &&
	  (listOfUpdUniqueRows_.entries() == 0))
	isUnique = TRUE;
      else if ((NOT (getSearchKey() && getSearchKey()->isUnique())) &&
	       (listOfUpdUniqueRows_.entries() == 1) &&
	       (listOfUpdUniqueRows_[0].rowIds_.entries() == 1))
	isUnique = TRUE;
    }
  if (getInliningInfo().isIMGU()) {
     // There is no need to checkAndPut for IM
     canDoCheckAndUpdel() = FALSE;
     uniqueHbaseOper() = FALSE;
     if ((generator->oltOptInfo()->multipleRowsReturned()) &&
	      (CmpCommon::getDefault(HBASE_ROWSET_VSBB_OPT) == DF_ON) &&
	      (NOT generator->isRIinliningForTrafIUD()))
	 uniqueRowsetHbaseOper() = TRUE;
  }
  else
  if (isUnique)
    {
      //If this unique delete is not part of a rowset operation , 
      //don't allow it to be cancelled.
      if (!generator->oltOptInfo()->multipleRowsReturned())
	generator->setMayNotCancel(TRUE);
      uniqueHbaseOper() = TRUE;

      canDoCheckAndUpdel() = FALSE;

      if ((NOT isMerge()) &&
	  (NOT producesOutputs()) &&
	  (executorPred().isEmpty()) &&
	  (NOT needToGetCols) &&
	  (NOT inlinedActions))
	{
	  if ((generator->oltOptInfo()->multipleRowsReturned()) &&
	      (CmpCommon::getDefault(HBASE_ROWSET_VSBB_OPT) == DF_ON) &&
	      (NOT generator->isRIinliningForTrafIUD()))
	    uniqueRowsetHbaseOper() = TRUE;
	  else if ((NOT generator->oltOptInfo()->multipleRowsReturned()) &&
		   (listOfUpdUniqueRows_.entries() == 0))
	    {
	      if ((CmpCommon::getDefault(HBASE_CHECK_AND_UPDEL_OPT) == DF_ON) &&
                  (NOT isAlignedFormat))
		canDoCheckAndUpdel() = TRUE;
	    }
	}
    }
  else if (producesOutputs())
    {
      // Cannot do olt msg opt if:
      //   -- values are to be returned and unique operation is not being used.
      // set an indication that multiple rows will be returned.
      generator->oltOptInfo()->setMultipleRowsReturned(TRUE);
      generator->oltOptInfo()->setOltCliOpt(FALSE);
    }

  generator->setUpdSavepointOnError(FALSE);
  generator->setUpdPartialOnError(FALSE);

  // if unique oper with no index maintanence and autocommit is on, then
  // do not require a transaction. 
  // Use hbase or region transactions. 
  // Hbase guarantees single row consistency.
  Int64 transId = -1;
  if (CmpCommon::getDefault(TRAF_NO_DTM_XN) == DF_ON)
    {
      // no transaction needed
      noDTMxn() = TRUE;
    }
  else if ((uniqueHbaseOper()) &&
           (NOT isMerge()) &&
           (NOT cursorHbaseOper()) &&
           (NOT uniqueRowsetHbaseOper()) &&
           (NOT inlinedActions) &&
           (generator->getTransMode()->getAutoCommit() == TransMode::ON_) &&
           (! NAExecTrans(0, transId)) &&
           (NOT generator->oltOptInfo()->multipleRowsReturned()))
    {
      // no DTM transaction needed
      useRegionXn() = FALSE;
      if (CmpCommon::getDefault(TRAF_USE_REGION_XN) == DF_ON)
        useRegionXn() = TRUE;
    }
  else
    {
      generator->setTransactionFlag(TRUE);

      if ((NOT uniqueHbaseOper()) ||
	  (isMerge()) ||
	  (cursorHbaseOper()) ||
	  (uniqueRowsetHbaseOper()) ||
	  (inlinedActions) ||
	  (generator->oltOptInfo()->multipleRowsReturned()))
	generator->setUpdAbortOnError(TRUE);
    }

  // flag for hbase tables
  generator->setHdfsAccess(TRUE);
  if (getTableDesc()->getNATable()->hasLobColumn())
    {
      for (CollIndex i = 0; i < newRecExprArray().entries(); i++)
	{
	  NAColumn * col = 
	    newRecExprArray()[i].getItemExpr()->child(0)->castToItemExpr()->
	    getValueId().getNAColumn(TRUE);

	  ItemExpr * val = 
	    newRecExprArray()[i].getItemExpr()->child(1)->castToItemExpr();
	  
	  if ((col->getType()->isLob()) &&
	      (val->getOperatorType() == ITM_LOBUPDATE))
	    {
	      LOBupdate * lu = (LOBupdate*)val;

	      lu->updatedTableObjectUID() = 
		getIndexDesc()->getPrimaryTableDesc()->
		getNATable()->objectUid().castToInt64();
	      
	      lu->updatedTableSchemaName() = "\"";
	      lu->updatedTableSchemaName() += 
		getTableDesc()->getNATable()->
		getTableName().getCatalogName();
	      lu->updatedTableSchemaName().append("\".\"");
	      lu->updatedTableSchemaName().
		append(getTableDesc()->getNATable()->
		       getTableName().getSchemaName());
	      lu->updatedTableSchemaName() += "\"";
              lu->lobSize() = col->getType()->getPrecision();
	      lu->lobNum() = col->lobNum();
	     
              if (lu->lobStorageType() == Lob_Empty)
                    {
                      lu->lobStorageType() = col->lobStorageType();
                    }
              if (lu->lobStorageType() != col->lobStorageType())
                    {
                      *CmpCommon::diags() << DgSqlCode(-1432)
                                          << DgInt0((Int32)lu->lobStorageType())
                                          << DgInt1((Int32)col->lobStorageType())
                                          << DgString0(col->getColName());
                      GenExit();
                    }
	      lu->lobStorageLocation() = col->lobStorageLocation();
	    }
	} // for
    } // if
 
  markAsPreCodeGenned();

  return this;  
}

RelExpr * HiveInsert::preCodeGen(Generator * generator,
                              const ValueIdSet & externalInputs,
                              ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  generator->setHiveAccess(TRUE);
  return GenericUpdate::preCodeGen(generator, externalInputs, pulledNewInputs);
}


RelExpr * HbaseInsert::preCodeGen(Generator * generator,
                              const ValueIdSet & externalInputs,
                              ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  // char. outputs are set to empty after in RelExpr::genPreCode sometimes,
  // after a call to resolveCharOutputs. We need to remember if a returnRow
  // tdb flag should be set, even if no output columns are required
  if (getIsTrafLoadPrep() && !getGroupAttr()->getCharacteristicOutputs().isEmpty())
    setReturnRow(TRUE);

  if (! GenericUpdate::preCodeGen(generator, externalInputs, pulledNewInputs))
    return NULL;

  NABoolean inlinedActions = FALSE;
  if ((getInliningInfo().hasInlinedActions()) ||
      (getInliningInfo().isEffectiveGU()))
    inlinedActions = TRUE;

  // Allow projecting rows if the upsert has IM. 
  if (inlinedActions && isUpsert())
    setReturnRow(TRUE);

  if (((getTableDesc()->getNATable()->isHbaseRowTable()) ||
       (getTableDesc()->getNATable()->isHbaseCellTable())) &&
      (producesOutputs()))
    {
      *CmpCommon::diags() << DgSqlCode(-1425)
			  << DgTableName(getTableDesc()->getNATable()->getTableName().
					 getQualifiedNameAsAnsiString())
                          << DgString0("Reason: Cannot return values from an hbase insert, update or delete.");
      GenExit();
    }

  if ((isUpsert()) &&
      ((getInsertType() == Insert::VSBB_INSERT_USER) ||
       (getInsertType() == Insert::UPSERT_LOAD)))
    {
      // Remove this restriction
      /* if ((inlinedActions || producesOutputs())&& !getIsTrafLoadPrep())
         setInsertType(Insert::SIMPLE_INSERT);*/
      
    }

  

  
  // if there are blob columns, use simple inserts.
  if ( getTableDesc()->getNATable()->hasLobColumn())
    {
      setInsertType(Insert::SIMPLE_INSERT);

      NAColumnArray colArray;
      NAColumn *tgtCol, *srcCol;

      for (CollIndex ii = 0;
           ii < newRecExprArray().entries(); ii++)
	{
	  ItemExpr *assignExpr =
	    newRecExprArray()[ii].getItemExpr();
	  
	  ValueId tgtValueId =
	    assignExpr->child(0)->castToItemExpr()->getValueId();

	  ValueId srcValueId =
	    assignExpr->child(1)->castToItemExpr()->getValueId();

	  tgtCol = tgtValueId.getNAColumn( TRUE );
	  srcCol = srcValueId.getNAColumn( TRUE );
	  ItemExpr * child1Expr = assignExpr->child(1);
	  if (srcValueId.getType().isLob())
	    {
	      LOBinsert * li = NULL;
	      if ((child1Expr->getOperatorType() != ITM_LOBINSERT) &&
		  (child1Expr->getOperatorType() != ITM_LOBUPDATE))
		{
		  li = new(generator->wHeap())
		    LOBinsert(child1Expr, NULL, LOBoper::LOB_);
                  
		  li->insertedTableObjectUID() = 
		    getIndexDesc()->getPrimaryTableDesc()->
		    getNATable()->objectUid().castToInt64();

		  li->insertedTableSchemaName() = "\"";
		  li->insertedTableSchemaName() += 
		    getTableDesc()->getNATable()->
		    getTableName().getCatalogName();
		  li->insertedTableSchemaName().append("\".\"");
		  li->insertedTableSchemaName().
		    append(getTableDesc()->getNATable()->
			   getTableName().getSchemaName());
		  li->insertedTableSchemaName() += "\"";
		  
		
                  li->lobSize() = tgtValueId.getType().getPrecision();
		  li->lobFsType() = tgtValueId.getType().getFSDatatype();

		  li->lobNum() = tgtCol->lobNum();
                  if ((child1Expr->getOperatorType() == ITM_CONSTANT) && 
                      !(((ConstValue *)child1Expr)->isNull()))
                    {
                    if (srcCol->lobStorageType() != tgtCol->lobStorageType())
                      {
                        *CmpCommon::diags() << DgSqlCode(-1432)
                                            << DgInt0((Int32)srcCol->lobStorageType())
                                            << DgInt1((Int32)tgtCol->lobStorageType())
                                            << DgString0(tgtCol->getColName());
                        GenExit();
                      }
                    }
                    else
                      if ((child1Expr->getOperatorType() == ITM_BASECOLUMN)||
                          (child1Expr->getOperatorType() == ITM_INDEXCOLUMN))
                        {
                          if (srcCol->lobStorageType() != tgtCol->lobStorageType())
                            {
                              *CmpCommon::diags() << DgSqlCode(-1432)
                                                  << DgInt0((Int32)srcCol->lobStorageType())
                                                  << DgInt1((Int32)tgtCol->lobStorageType())
                                                  << DgString0(tgtCol->getColName());
                              GenExit();
                            }
                          
                        }
                  li->lobStorageType() = tgtCol->lobStorageType();

		  li->lobStorageLocation() = tgtCol->lobStorageLocation();

		  li->bindNode(generator->getBindWA());

		  child1Expr = li;

		  assignExpr->child(1) = child1Expr;
		}
	      else if (child1Expr->getOperatorType() == ITM_LOBINSERT)
		{
		  li = (LOBinsert*)child1Expr;
		  li->insertedTableObjectUID() = 
		    getIndexDesc()->getPrimaryTableDesc()->
		    getNATable()->objectUid().castToInt64();

		  li->insertedTableSchemaName() = "\"";
		  li->insertedTableSchemaName() += 
		    getTableDesc()->getNATable()->
		    getTableName().getCatalogName();
		  li->insertedTableSchemaName().append("\".\"");
		  li->insertedTableSchemaName().
		    append(getTableDesc()->getNATable()->
			   getTableName().getSchemaName());
		  li->insertedTableSchemaName() += "\"";
		  
		  li->lobNum() = tgtCol->lobNum();
                  //If we are initializing an empty_lob, assume the storage 
                  //type of the underlying column
                  if (li->lobStorageType() == Lob_Empty)
                    {
                      li->lobStorageType() = tgtCol->lobStorageType();
                    }
                  if (li->lobStorageType() != tgtCol->lobStorageType())
                    {
                      *CmpCommon::diags() << DgSqlCode(-1432)
                                          << DgInt0((Int32)li->lobStorageType())
                                          << DgInt1((Int32)tgtCol->lobStorageType())
                                          << DgString0(tgtCol->getColName());
                        GenExit();
                      }
		  li->lobStorageLocation() = tgtCol->lobStorageLocation();
		  
		  li->lobSize() = tgtValueId.getType().getPrecision();

		  if (li->lobFsType() != tgtValueId.getType().getFSDatatype())
		    {
		      // create a new LOBinsert node since fsType has changed.
		      ItemExpr * liChild = li->child(0);
		      ItemExpr * liChild1 = li->child(1);
		      li = new(generator->wHeap())
			LOBinsert(liChild, liChild1, li->getObj(),FALSE, 
                                  li->lobAsVarchar());
		      
		      li->insertedTableObjectUID() = 
			getIndexDesc()->getPrimaryTableDesc()->
			getNATable()->objectUid().castToInt64();
		      
		      li->insertedTableSchemaName() = "\"";
		      li->insertedTableSchemaName() += 
			getTableDesc()->getNATable()->
			getTableName().getCatalogName();
		      li->insertedTableSchemaName().append("\".\"");
		      li->insertedTableSchemaName().
			append(getTableDesc()->getNATable()->
			       getTableName().getSchemaName());
		      li->insertedTableSchemaName() += "\"";
		      
		      //li->lobSize() = srcValueId.getType().getPrecision();
                      li->lobSize() = tgtValueId.getType().getPrecision();
		      li->lobFsType() = tgtValueId.getType().getFSDatatype();

		      li->lobNum() = tgtCol->lobNum();
		     
		      li->lobStorageLocation() = tgtCol->lobStorageLocation();
		      
		      li->bindNode(generator->getBindWA());

		      assignExpr->child(1) = li;
		    }
		} // lobinsert

	      GenAssert(li, "must have a LobInsert node");
	    } // lob
	}
    }


  if ((getInsertType() == Insert::SIMPLE_INSERT)  &&
      (NOT getTableDesc()->getNATable()->hasLobColumn()))
    uniqueHbaseOper() = TRUE;

  generator->setUpdSavepointOnError(FALSE);
  generator->setUpdPartialOnError(FALSE);

  // if unique oper with no index maintanence and autocommit is on, then
  // do not require a trnsaction. 
  // Use hbase or region transactions. 
  // Hbase guarantees single row consistency.
  Int64 transId = -1;
  if ((CmpCommon::getDefault(TRAF_NO_DTM_XN) == DF_ON) ||
      (isNoRollback()) ||
      ((isUpsert()) && (insertType_ == UPSERT_LOAD)))
    {
      // no transaction needed
      noDTMxn() = TRUE;
    }
  else if ((uniqueHbaseOper()) &&
           (NOT uniqueRowsetHbaseOper()) &&
           (NOT inlinedActions) &&
           (generator->getTransMode()->getAutoCommit() == TransMode::ON_) &&
           (! NAExecTrans(0, transId)) &&
           (NOT generator->oltOptInfo()->multipleRowsReturned()))
    {
      // no DTM transaction needed
      useRegionXn() = FALSE;
      if (CmpCommon::getDefault(TRAF_USE_REGION_XN) == DF_ON)
        useRegionXn() = TRUE;
    }
  else
    {
      generator->setTransactionFlag(TRUE);

      if ((NOT uniqueHbaseOper()) ||
	  (uniqueRowsetHbaseOper()) ||
	  (inlinedActions) ||
	  (generator->oltOptInfo()->multipleRowsReturned()))
	generator->setUpdAbortOnError(TRUE);
    }

  return this;
}

RelExpr * ExeUtilHiveTruncateLegacy::preCodeGen(Generator * generator,
                                                const ValueIdSet & externalInputs,
                                                ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  return ExeUtilExpr::preCodeGen(generator,externalInputs,pulledNewInputs);
}

RelExpr * ExeUtilLobExtract::preCodeGen(Generator * generator,
                                       const ValueIdSet & externalInputs,
                                       ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! ExeUtilExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  ValueIdSet availableValues;
  for (ValueId exprId = getGroupAttr()->getCharacteristicInputs().init();
       getGroupAttr()->getCharacteristicInputs().next(exprId);
       getGroupAttr()->getCharacteristicInputs().advance(exprId) )
    {
      if (exprId.getItemExpr()->getOperatorType() != ITM_VEG_REFERENCE)
       availableValues += exprId;
    }
  
  getGroupAttr()->setCharacteristicInputs(availableValues);
  getInputValuesFromParentAndChildren(availableValues);

  if (handle_)
    handle_->replaceVEGExpressions
      (availableValues, getGroupAttr()->getCharacteristicInputs());
  
  markAsPreCodeGenned();

   // Done.
   return this;
 }

RelExpr * ExeUtilLobUpdate::preCodeGen(Generator * generator,
                                       const ValueIdSet & externalInputs,
                                       ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! ExeUtilExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  ValueIdSet availableValues;
  for (ValueId exprId = getGroupAttr()->getCharacteristicInputs().init();
       getGroupAttr()->getCharacteristicInputs().next(exprId);
       getGroupAttr()->getCharacteristicInputs().advance(exprId) )
    {
      if (exprId.getItemExpr()->getOperatorType() != ITM_VEG_REFERENCE)
       availableValues += exprId;
    }
  
  getGroupAttr()->setCharacteristicInputs(availableValues);
  getInputValuesFromParentAndChildren(availableValues);

  if (handle_)
    handle_->replaceVEGExpressions
      (availableValues, getGroupAttr()->getCharacteristicInputs());
  xnNeeded() = TRUE;
  markAsPreCodeGenned();

   // Done.
   return this;
 }

RelExpr * HashGroupBy::preCodeGen(Generator * generator,
                                 const ValueIdSet & externalInputs,
                                 ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;


  if ( CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) == DF_SYSTEM)
  {
    NABoolean resize = FALSE;
    NABoolean defrag = FALSE;
    ValueIdSet vidSet = child(0)->getGroupAttr()->getCharacteristicOutputs();

    ExpTupleDesc::TupleDataFormat tupleFormat =
                  determineInternalFormat( vidSet,
                                           this,
                                           resize,
                                           generator,
                                           FALSE,
                                           defrag);

    cacheTupleFormatAndResizeFlag(tupleFormat, resize, defrag);

    if (tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
    {
      generator->incNCIFNodes();
    }
    else
    {
      generator->decNCIFNodes();
    }
  }


  return GroupByAgg::preCodeGen(generator, externalInputs, pulledNewInputs);

}
RelExpr * GroupByAgg::preCodeGen(Generator * generator,
				 const ValueIdSet & externalInputs,
				 ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  // Check if the pivs of this operator and it's child are the same.
  // If they are not, make them the same.
  replacePivs();

  generator->clearPrefixSortKey();
  // Resolve the VEGReferences and VEGPredicates, if any, that appear
  // in the Characteristic Inputs, in terms of the externalInputs.
  getGroupAttr()->resolveCharacteristicInputs(externalInputs);
  // My Characteristic Inputs become the external inputs for my child.
  child(0) = child(0)->preCodeGen(generator,
				  getGroupAttr()->getCharacteristicInputs(),
				  pulledNewInputs);
  if (! child(0).getPtr())
    return NULL;

  if ((getOperatorType() == REL_SHORTCUT_GROUPBY)
      && (getFirstNRows() == 1))
    {
      RelExpr * firstnNode = new(generator->wHeap()) FirstN(child(0),
							    getFirstNRows(),
							    FALSE /* [any n] is good enough */);
      firstnNode->setEstRowsUsed(getEstRowsUsed());
      firstnNode->setMaxCardEst(getMaxCardEst());
      firstnNode->setInputCardinality(child(0)->getInputCardinality());
      firstnNode->setPhysicalProperty(child(0)->getPhysicalProperty());
      firstnNode->setGroupAttr(child(0)->getGroupAttr());
      //10-060516-6532 -Begin
      //When FIRSTN node is created after optimization phase, the cost
      //of that node does not matter.But, display_explain and explain
      //show zero operator costs and rollup cost which confuses the user.
      //Also, the VQP crashes when cost tab for FIRSTN node is selected.
      //So, creating a cost object will fix this.
      //The operator cost is zero and rollup cost is same as it childs.
      Cost* firstnNodecost = new HEAP Cost();
      firstnNode->setOperatorCost(firstnNodecost);
      Cost* rollupcost = (Cost *)(child(0)->getRollUpCost());
      *rollupcost += *firstnNodecost;
      firstnNode->setRollUpCost(rollupcost);
      //10-060516-6532 -End

      firstnNode =
	firstnNode->preCodeGen(generator,
			       getGroupAttr()->getCharacteristicInputs(),
			       pulledNewInputs);
      if (! firstnNode)
	return NULL;

      setChild(0, firstnNode);
    }

  getGroupAttr()->addCharacteristicInputs(pulledNewInputs);

  // Accumulate the values that are provided as inputs by my parent
  // together with the values that are produced as outputs by my
  // children. Use these values for rewriting the VEG expressions.
  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);
  NABoolean replicatePredicates = TRUE;

  // Rebuild the grouping expressions tree. Use bridge values, if possible
  groupExpr().replaceVEGExpressions
                 (availableValues,
		  getGroupAttr()->getCharacteristicInputs(),
                  FALSE, // No key predicates need to be generated here
		  NULL,
		  replicatePredicates,
                  &getGroupAttr()->getCharacteristicOutputs());

  // Rebuild the rollup grouping expressions tree. Use bridge values, if possible
  rollupGroupExprList().replaceVEGExpressions
                 (availableValues,
		  getGroupAttr()->getCharacteristicInputs(),
                  FALSE, // No key predicates need to be generated here
		  NULL,
		  replicatePredicates,
                  &getGroupAttr()->getCharacteristicOutputs());

  // Rebuild the aggregate expressions tree
  aggregateExpr().replaceVEGExpressions
                     (availableValues,
  	 	      getGroupAttr()->getCharacteristicInputs());

  if (CmpCommon::getDefault(COMP_BOOL_211) == DF_ON) 
  {
    ValueIdSet constantsInGroupExpr ;
    groupExpr().getConstantExprs(constantsInGroupExpr,FALSE);
    if (constantsInGroupExpr.entries() > 0)
    {
      if (constantsInGroupExpr.entries() == groupExpr().entries())
      {
        ValueId vid ;
        constantsInGroupExpr.getFirst(vid);
        constantsInGroupExpr.remove(vid);
      }
      groupExpr() -= constantsInGroupExpr ;
    }
  }

  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  getInputAndPotentialOutputValues(availableValues);

  selectionPred().replaceVEGExpressions
                     (availableValues,
		      getGroupAttr()->getCharacteristicInputs(),
		      FALSE, // No key predicates need to be generated here
		      NULL,
		      replicatePredicates);

  getGroupAttr()->resolveCharacteristicOutputs
                      (availableValues,
		       getGroupAttr()->getCharacteristicInputs());

  // if the grouping is executed in DP2, we don't do overflow
  // handling. This also means, that it is a partial group by
  // Do not do overflow handling for any partial groupby.
  //
  NABoolean isPartialGroupBy = (isAPartialGroupByNonLeaf() ||
                                isAPartialGroupByLeaf());

  // The old way, only groupbys in DP2 are considered partial
  //
  if (CmpCommon::getDefault(COMP_BOOL_152) == DF_ON) {
    isPartialGroupBy = executeInDP2();
  }
  
  if ((getOperatorType() == REL_HASHED_GROUPBY) && !isPartialGroupBy) {
    // Count this BMO and add its needed memory to the total needed
    generator->incrNumBMOs();

  if ((ActiveSchemaDB()->getDefaults()).getAsDouble(BMO_MEMORY_LIMIT_PER_NODE_IN_MB) > 0)
      generator->incrBMOsMemory(getEstimatedRunTimeMemoryUsage(generator, TRUE));

  }

  markAsPreCodeGenned();

  // Done.
  return this;
} // GroupByAgg::preCodeGen()

RelExpr * MergeUnion::preCodeGen(Generator * generator,
                                 const ValueIdSet & externalInputs,
                                 ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  // A temporary union (blocked union introduced for inlining after trigger)
  // should not get here. Should be removed in optimization phase.
  GenAssert(!getIsTemporary(), "Expecting this blocked union to be removed by this phase");

  // Check if the pivs of this operator and it's child are the same.
  // If they are not, make them the same.
  replacePivs();

  // clear any prefix sort key in generator work area
  generator->clearPrefixSortKey();

  // Resolve the VEGReferences and VEGPredicates, if any, that appear
  // in the Characteristic Inputs, in terms of the externalInputs.
  getGroupAttr()->resolveCharacteristicInputs(externalInputs);

  // Predicate pushdown causes the Characteristic Inputs and Outputs
  // of the union to be set precisely to those values that are
  // required by one of its descendants or by one of its ancestors,
  // respectively. However, the colMapTable_ contains all the values
  // that the MergeUnion is capable of producing. The colMapTable_
  // is rebuilt here to contain exactly those values that appear in
  // the Characteristic Outputs.
  //
  // The output of the union is defined by the ValueIdUnion
  // expressions that are maintained in the colMapTable_.
  //

  ValueIdSet charOutputs = getGroupAttr()->getCharacteristicOutputs();
  colMapTable().clear();

  for (ValueId v = charOutputs.init();
       charOutputs.next(v); charOutputs.advance(v))
  {
    if (v.getItemExpr()->getOperatorType() != ITM_VALUEIDUNION)
    {
      // "other" available values besides the value being considered.
      ValueIdSet availableValues = charOutputs;
      availableValues -= v;

      // -------------------------------------------------------------------
      // see whether the value being considered is covered by the remaining
      // values. that is, whether it is an expression in termes of the
      // other vid union's.
      // -------------------------------------------------------------------
      ValueIdSet outputId;
      outputId.insert(v);
      outputId.removeUnCoveredExprs(availableValues);

      // -------------------------------------------------------------------
      // v removed from outputId. that means it's not covered by remaining
      // vid union's. add the vid union's v is in terms of to colMapTable.
      // the node needs to produce it. Instead of producing the expression,
      // change the node to produce just the vid union, the expression can
      // be evaluatated at the parent.
      // -------------------------------------------------------------------
      if (outputId.isEmpty())
      {
        Int32 leftIndex = getLeftMap().getTopValues().index(v);
        Int32 rightIndex = getRightMap().getTopValues().index(v);

        CMPASSERT((leftIndex != NULL_COLL_INDEX) &&
                  (rightIndex != NULL_COLL_INDEX));

        ItemExpr *ptr = new(CmpCommon::statementHeap())
          ValueIdUnion(getLeftMap().getBottomValues()[leftIndex],
                       getRightMap().getBottomValues()[rightIndex],v);
        v.replaceItemExpr(ptr);
	colMapTable().insert(v);
      }
    }
    else
      colMapTable().insert(v);
  }

  // My Characteristic Inputs become the external inputs for my children.
  Lng32 nc = (Lng32)getArity();
  const ValueIdSet & inputs = getGroupAttr()->getCharacteristicInputs();
  for (Lng32 index = 0; index < nc; index++)
  {
    ValueIdSet pulledInputs;
    child(index) = child(index)->preCodeGen(generator,inputs,pulledInputs);
    if (child(index).getPtr() == NULL) return NULL;
    pulledNewInputs += pulledInputs;
  }

  // Accumulate the values that are provided as inputs by my parent
  // together with the values that are produced as outputs by my
  // children. Use these values for rewriting the VEG expressions.
  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);

  // Rebuild the colMapTable
  colMapTable().replaceVEGExpressions(availableValues,inputs);

  // Rebuild the sortOrder.
  sortOrder_.replaceVEGExpressions(availableValues,inputs);

  // Rebuild the merge expression
  if (mergeExpr_)
  {
    mergeExpr_ = mergeExpr_->replaceVEGExpressions(availableValues,inputs);
    //10-061219-1283:Set the second arugment to TRUE to redrive typesynthesis of children.
    mergeExpr_->synthTypeAndValueId(TRUE,TRUE);
  }

  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  getInputAndPotentialOutputValues(availableValues);

  // Rebuild the selection predicate tree.
  selectionPred().replaceVEGExpressions(availableValues,inputs);
  getGroupAttr()->resolveCharacteristicOutputs(availableValues,inputs);
  // Rebuild the conditional expression.
  condExpr().replaceVEGExpressions(availableValues,
				   getGroupAttr()->getCharacteristicInputs());

  if (!getUnionForIF() && !getInliningInfo().isIMUnion())
    generator->oltOptInfo()->setMultipleRowsReturned(TRUE);

  markAsPreCodeGenned();

  return this;
} // MergeUnion::preCodeGen()

RelExpr * MapValueIds::preCodeGen(Generator * generator,
				  const ValueIdSet & externalInputs,
				  ValueIdSet &pulledNewInputs)
{
  const ValueIdList &upperValues = map_.getTopValues();
  const ValueIdList &lowerValues = map_.getBottomValues();

  if (nodeIsPreCodeGenned())
    return this;

  // Check if the pivs of this operator and it's child are the same.
  // If they are not, make them the same.
  replacePivs();

  // Resolve the VEGReferences and VEGPredicates, if any, that appear
  // in the Characteristic Inputs, in terms of the externalInputs.
  getGroupAttr()->resolveCharacteristicInputs(externalInputs);

  // My Characteristic Inputs become the external inputs for my children.
  child(0) = child(0)->preCodeGen(
       generator,
       getGroupAttr()->getCharacteristicInputs(),
       pulledNewInputs);
  if (child(0).getPtr() == NULL)
    return NULL;

  getGroupAttr()->addCharacteristicInputs(pulledNewInputs);

  if (cseRef_)
    {
      // -------------------------------------------------------------
      // This MapValueIds represents a common subexpression.
      //
      // We need to take some actions here to help with VEG rewrite,
      // since we eliminated some nodes from the tree, while the
      // VEGies still contain all equated values, including those that
      // got eliminated. Furthermore, the one tree that was chosen for
      // materialization got moved and we need to make sure that the
      // place where we scan the temp table produces the same ValueIds
      // that were marked as "Bridge Values" when we processed the
      // insert into temp statement.
      // -------------------------------------------------------------

      ValueIdSet cseVEGPreds;
      const ValueIdList &vegCols(cseRef_->getColumnList());
      ValueIdSet nonVegCols(cseRef_->getNonVEGColumns());
      NABoolean isAnalyzingConsumer =
        (CmpCommon::statement()->getCSEInfo(cseRef_->getName())->
           getIdOfAnalyzingConsumer() == cseRef_->getId());
      ValueIdSet availableValues(
           getGroupAttr()->getCharacteristicInputs());

      valuesNeededForVEGRewrite_ += cseRef_->getNonVEGColumns();
      availableValues += valuesNeededForVEGRewrite_;

      // find all the VEG predicates of the original columns that this
      // common subexpression represents...
      for (CollIndex v=0; v<vegCols.entries(); v++)
        if (vegCols[v].getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
          {
            // look at one particular VEG that is produced by this
            // query tree
            VEG *veg =
              static_cast<VEGReference *>(vegCols[v].getItemExpr())->getVEG();

            if (isAnalyzingConsumer && veg->getBridgeValues().entries() > 0)
              {
                // If we are looking at the analyzing consumer, then
                // its child tree "C" got transformed into an
                // "insert overwrite table "temp" select * from "C".

                // This insert into temp statement chose some VEG
                // member(s) as the "bridge value(s)". Find these bridge
                // values and choose one to represent the VEG here.
                const ValueIdSet &vegMembers(veg->getAllValues());

                // collect all VEG members produced and subtract them
                // from the values to be used for VEG rewrite
                ValueIdSet subtractions(cseRef_->getNonVEGColumns());
                // then add back only the bridge value
                ValueIdSet additions;

                // get the VEG members produced by child C
                subtractions.intersectSet(vegMembers);

                // augment the base columns with their index columns,
                // the bridge value is likely an index column
                for (ValueId v=subtractions.init();
                     subtractions.next(v);
                     subtractions.advance(v))
                  if (v.getItemExpr()->getOperatorType() == ITM_BASECOLUMN)
                    {
                      subtractions +=
                        static_cast<BaseColumn *>(v.getItemExpr())->getEIC();
                    }

                // now find a bridge value (or values) that we can
                // produce
                additions = subtractions;
                additions.intersectSet(veg->getBridgeValues());

                // if we found it, then adjust availableValues
                if (additions.entries() > 0)
                  {
                    availableValues -= subtractions;
                    availableValues += additions;
                    // do the same for valuesNeededForVEGRewrite_,
                    // which will be used for rewriting the char.
                    // outputs
                    valuesNeededForVEGRewrite_ -= subtractions;
                    valuesNeededForVEGRewrite_ += additions;
                  }
              }

            cseVEGPreds += veg->getVEGPredicate()->getValueId();
          } // a VEGRef

      // Replace the VEGPredicates, pretending that we still have
      // the original tree below us, not the materialized temp
      // table. This will hopefully keep the bookkeeping in the
      // VEGies correct by setting the right referenced values
      // and choosing the right bridge values.
      cseVEGPreds.replaceVEGExpressions(
           availableValues,
           getGroupAttr()->getCharacteristicInputs());

    } // this MapValueIds is for a common subexpression

  // ---------------------------------------------------------------------
  // The MapValueIds node describes a mapping between expressions used
  // by its child tree and expressions used by its parent tree. The
  // generator will make sure that the output values of the child tree
  // and the input values from the parent get passed in the correct
  // buffers.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Replacing VEGReferences in those mapped expressions is not possible
  // in all cases; we have to restrict the kind of mappings that can
  // be done for expressions involving VEGs. This method assumes that
  // references to VEGs do not get altered during the rewrite, in other
  // words it assumes mappings of the kind
  //
  //     a)          sum(VEGRef(a,b,c))    <---->    VEGRef(a,b,c)
  //
  // and it disallows mappings of the kind
  //
  //     b)          count(VEGRef(a,b,c))  <----->   1
  //     c)          VEGRef(a,b,c)         <----->   VEGRef(d,e,f)
  //
  // Mappings of type b) will still work, as long as the VEGRef is contained
  // in some other mapped expression. A possible extension is to store
  // in the MapValueIds node which element(s) of which VEGRef should
  // be replaced in this step, but this information is hard to get
  // during optimization, unless we are looking at a scan node.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // The map contains many mappings, not all of which will have to
  // be evaluated by the generator. Only those values that are either
  // characteristic output values or are referenced by characteristic
  // output values will actually be mapped at execution time. Therefore
  // we first determine the actually needed mappings with the coverTest
  // method.
  // ---------------------------------------------------------------------

  GroupAttributes emptyGA;
  ValueIdSet coveredExpr;
  ValueIdSet referencedUpperValues;
  ValueIdMap newMap;

  emptyGA.setCharacteristicInputs(getGroupAttr()->getCharacteristicInputs());

  emptyGA.coverTest(
       getGroupAttr()->getCharacteristicOutputs(), // the expressions needed
       upperValues,            // offer the upper values as extra inputs
       coveredExpr,            // doesn't matter which outputs are covered
       referencedUpperValues); // get those upper values needed by the outputs

  // Compute the values that are available here.
  ValueIdSet lowerAvailableValues;
  getOutputValuesOfMyChildren(lowerAvailableValues);
  lowerAvailableValues += getGroupAttr()->getCharacteristicInputs();

  // The VEGReferences that are resolved can appear as leaves of the
  // expressions contained in lowerAvailableValues. These values are
  // required for remapping the upperValues.
  ValueIdSet leafValues;

  ValueId x;
  for (x = lowerAvailableValues.init();
       lowerAvailableValues.next(x);
       lowerAvailableValues.advance(x))
    x.getItemExpr()->getLeafValueIds(leafValues);
  lowerAvailableValues += leafValues;

  ValueIdSet upperAvailableValues(valuesNeededForVEGRewrite_);
  // The addition of the lower available values is only necessary to
  // avoid an assertion failure in VEGReference::replaceVEGReference().
  upperAvailableValues += lowerAvailableValues;

  // ---------------------------------------------------------------------
  // now walk through each needed mapping and replace wildcards in both its
  // upper and lower expressions
  // ---------------------------------------------------------------------
  for (CollIndex i = 0; i < upperValues.entries(); i++)
    {
      if (referencedUpperValues.contains(upperValues[i]))
	{
	  ItemExpr *newUpper;
	  ItemExpr *newLower;
	  // This mapping is actually required, expand wild cards for it

          // We used to resolve the upper values using the
          // upperAvailableValues.  Note that these available values
          // might not actually be available to this node.  This could
          // sometimes cause problems if the VEGRef was resolved to the
          // 'wrong' value and the value is in a VEGPRed above.  This
          // would cause VEGPRed to be resolved incorrectly and
          // possibly drop some join predicates.

          // Don't need to replace the VEGgies in the upper since they
          // will never be codeGen'ed.  Just need to replace them with
          // a suitable substitute.

          // If it is a VEG_REF, then replace it with a surrogate
          // (NATypeToItem) otherwise leave it as is.  (Don't use the
          // surrogate for all upper values because there are some
          // MVIds that have BaseColumns in the upper values. These
          // MVIds are introduced by Triggers.  And these BaseColumns
          // are used in other operators in other parts of the tree
          // where they are expected to always be BaseColumns. So
          // mapping them here will cause problems elsewhere).  In any
          // case, all we need to do here is to get rid of the
          // VEGRefs.
          //

	  newLower = lowerValues[i]
                       .getItemExpr()
                           ->replaceVEGExpressions
                                (lowerAvailableValues,
				 getGroupAttr()->getCharacteristicInputs());

          newUpper = upperValues[i].getItemExpr();

          if (upperValues[i] != lowerValues[i]) 
          {
             if (newUpper->getOperatorType() == ITM_VEG_REFERENCE) 
             {
               if (valuesNeededForVEGRewrite_.entries() > 0)
                 // If this node is used to map the outputs of one
                 // table to those of another, upperAvailableValues
                 // has been constructed to contain the base column a
                 // vegref should map to, so we use that instead of a
                 // created surrogate.
                 newUpper = newUpper->replaceVEGExpressions
                                (upperAvailableValues,
				 getGroupAttr()->getCharacteristicInputs());
               else {
                 NAType *mapType =
                   upperValues[i].getType().newCopy(generator->wHeap());
       
                 // Create replacement for VEGRef
                 //
                 ItemExpr *mapping =
                   new(generator->wHeap()) NATypeToItem(mapType);
       
                 ValueId id = upperValues[i];
       
                 // Replace in ValueDescArray.  All instances of this ID
                 // will now map to the surrogate.
                 //
                 id.replaceItemExpr(mapping);
       
                 newUpper = upperValues[i].getItemExpr();
               }
             }
          } else {
                // since they are the same, make upper equal to lower..
             newUpper = newLower;
          }


	  // add the mapping that may have been rewritten to the new map
	  newMap.addMapEntry(newUpper->getValueId(),newLower->getValueId());
	}
    }


  // now replace the map with the recomputed mappings
  map_ = newMap;

  // The selectionPred() on a MapValueId should have been pushed down
  // by the optimizer.
  GenAssert(selectionPred().isEmpty(),"NOT selectionPred().isEmpty()");

  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  // Be thrifty. Reuse coveredExpr for gathering the input and output values.
  getInputAndPotentialOutputValues(coveredExpr);

  // Add the value that is being fabricated by the MapValueIds to the values
  // that are produced by its child and flow throught the MapValueIds.
  lowerAvailableValues += coveredExpr;

  getGroupAttr()->resolveCharacteristicOutputs
                     (lowerAvailableValues,
		      getGroupAttr()->getCharacteristicInputs());

  markAsPreCodeGenned();

  return this;
} // MapValueIds::preCodeGen()

RelExpr * Sort::preCodeGen(Generator * generator,
			   const ValueIdSet & externalInputs,
			   ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
      return this;
    //else
    //  cerr << "Possible error..."

  // Check if the pivs of this operator and it's child are the same.
  // If they are not, make them the same.
  replacePivs();

  // Resolve the VEGReferences and VEGPredicates, if any, that appear
  // in the Characteristic Inputs, in terms of the externalInputs.
  getGroupAttr()->resolveCharacteristicInputs(externalInputs);

  // if doing Partial Sorting, store partial sort key in generator work area
  // if the split-top node is providing this underneath, protect the order
  // else clear the partial sort key
  ValueIdList prefixSortKey = getPrefixSortKey();

  generator->clearPrefixSortKey();

  if (!prefixSortKey.isEmpty())
    generator->setPrefixSortKey(prefixSortKey);

  PhysicalProperty* unPreCodeGendPP = NULL;

  // Protect against scan of self-referencing table partitions 
  // completing asynchronously, thus allowing the various instances
  // of SORT to start returning rows before all scans are complete.
  // Let the PartitionAccess::preCodeGen and Exchange::preCodeGen
  // work together to detect this.  Part of the fix for solution 
  // 10-071204-9253.

  bool doCheckUnsycHalloweenScans = false;
  
  // solution 10-100310-8659
  bool fixSolution8659 = false;   
  int numUnblockedHalloweenScansBefore = 
    generator->getUnblockedHalloweenScans();
  bool needToRestoreLSH = false;
  bool saveLSH = generator->getPrecodeHalloweenLHSofTSJ();
  
  // This is the pre-R2.5.1 test that triggers the check unblocked access.
  // Note that it indirectly depends on COMP_BOOL_166 OFF.
  if (checkAccessToSelfRefTable_)
    doCheckUnsycHalloweenScans = true;

  // This is the R2.5.1 way -- see solution 10-100310-8659.
  if ((generator->getPrecodeHalloweenLHSofTSJ()) &&
      (!generator->getPrecodeRHSofNJ()))
  {
    if (generator->getHalloweenSortForced())
      markAsHalloweenProtection();

    if (generator->preCodeGenParallelOperator() &&
        !generator->getHalloweenESPonLHS())
    {
      doCheckUnsycHalloweenScans = true;
      fixSolution8659 = true;
    }
    else
    {
      // This serial sort is enough to block the 
      // scan of the target table.  No need for further
      // checking.  Notice this serial vs. parallel sort test
      // was made in NestedJoin::preCodeGen before the fix
      // for 10-100310-8659.
      doCheckUnsycHalloweenScans  = false;

      // More for 10-100310-8659 - don't call incUnblockedHalloweenScans
      // below this node. 
      generator->setPrecodeHalloweenLHSofTSJ(false);
      needToRestoreLSH = true;

      GenAssert(generator->unsyncdSortFound() == FALSE, 
        "Unknown operator set unsyncdSortFound.");
    }
  }

  if (doCheckUnsycHalloweenScans)
  {
    generator->setCheckUnsyncdSort(TRUE);

    // Preserve a copy of the child's physical properties
    // as it is before preCodeGen is called for the child.  
    // Also, in this copy of the physical properties, use 
    // a copy of the child's partitioning function.  This 
    // will be used in case we need to insert an ESP for 
    // halloween protection.
    unPreCodeGendPP = new (CmpCommon::statementHeap()) 
        PhysicalProperty(*child(0)->getPhysicalProperty(),
		     child(0)->getPhysicalProperty()->getSortKey(),
                     child(0)->getPhysicalProperty()->getSortOrderType(), 
                     child(0)->getPhysicalProperty()->getDp2SortOrderPartFunc(),
		     child(0)->getPhysicalProperty()->
                            getPartitioningFunction()->copy()
                    );
  }

  if ( CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) == DF_SYSTEM)
  {
    NABoolean resize = FALSE;
    NABoolean defrag = FALSE;
    // get the char outputs and not  the child's
    ValueIdSet vidSet = getGroupAttr()->getCharacteristicOutputs();

    ExpTupleDesc::TupleDataFormat tupleFormat =
                  determineInternalFormat( vidSet,
                                           this,
                                           resize,
                                           generator,
                                           FALSE,
                                           defrag);

    cacheTupleFormatAndResizeFlag(tupleFormat, resize, defrag);

    if (tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
    {
      generator->incNCIFNodes();
    }
    else
    {
      generator->decNCIFNodes();
    }
  }

  // My Characteristic Inputs become the external inputs for my child.
  child(0) = child(0)->preCodeGen(generator,
				  getGroupAttr()->getCharacteristicInputs(),
				  pulledNewInputs);

  generator->clearPrefixSortKey();
  
  if (! child(0).getPtr())
    return NULL;

  if (needToRestoreLSH)
    generator->setPrecodeHalloweenLHSofTSJ(saveLSH);

  getGroupAttr()->addCharacteristicInputs(pulledNewInputs);

  // Accumulate the values that are provided as inputs by my parent
  // together with the values that are produced as outputs by my
  // children. Use these values for rewriting the VEG expressions.
  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);

  // ----------------------------------------------------------------------
  // Replace VEGReferences in the order by list
  // Bugfix: sol# 10-020909-1555/56: the last argument, if not explicitly
  // stated, defaults to FALSE, and causes a shallow copy of the tree.
  // ----------------------------------------------------------------------
  sortKey_.replaceVEGExpressions
              (availableValues,
	       getGroupAttr()->getCharacteristicInputs(),
	       FALSE, // default
	       NULL,  // default
	       TRUE); // bugfix

  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  getInputAndPotentialOutputValues(availableValues);

  getGroupAttr()->resolveCharacteristicOutputs
                     (availableValues,
		      getGroupAttr()->getCharacteristicInputs());

  //Consider Sort as part of BMO memory participant if not partial sort.
  if (prefixSortKey.entries() == 0 ||
     CmpCommon::getDefault(COMP_BOOL_84) == DF_ON)
    {
      if (CmpCommon::getDefault(SORT_MEMORY_QUOTA_SYSTEM) != DF_OFF) {
        generator->incrNumBMOs();

        if ((ActiveSchemaDB()->getDefaults()).getAsDouble(BMO_MEMORY_LIMIT_PER_NODE_IN_MB) > 0)
          generator->incrBMOsMemory(getEstimatedRunTimeMemoryUsage(generator, TRUE));
      }
    }


  markAsPreCodeGenned();  

  // Part of the fix for solution 10-071204-9253.
  // Modified for 10-100310-8659
  if (doCheckUnsycHalloweenScans && generator->unsyncdSortFound())
    {
      RelExpr *newChild = generator->insertEspExchange(child(0), 
						       unPreCodeGendPP);
      
      ((Exchange *)newChild)->markAsHalloweenProtection();
      
      newChild = 
	newChild->preCodeGen(generator, externalInputs, pulledNewInputs);
      
      GenAssert(newChild->getOperatorType() == REL_EXCHANGE, 
		"Exchange eliminated despite our best efforts.");

      child(0) = newChild;
          
      // Now that an ESP is inserted above the scans, this sort operator
      // does block the scans, so we can discount them.
      if (fixSolution8659)
	{
	  generator->setUnsyncdSortFound(FALSE);
	  generator->setUnblockedHalloweenScans(
				       numUnblockedHalloweenScansBefore);
	}
    }
  topNRows_ = generator->getTopNRows();
  return this;

} // Sort::preCodeGen()

RelExpr * SortFromTop::preCodeGen(Generator * generator,
				  const ValueIdSet & externalInputs,
				  ValueIdSet &pulledNewInputs)
{
  return Sort::preCodeGen(generator, externalInputs, pulledNewInputs);
}

RelExpr *ProbeCache::preCodeGen(Generator * generator,
			   const ValueIdSet & externalInputs,
			   ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  // Check if the pivs of this operator and it's child are the same.
  // If they are not, make them the same.
  replacePivs();

  // Resolve the VEGReferences and VEGPredicates, if any, that appear
  // in the Characteristic Inputs, in terms of the externalInputs.
  getGroupAttr()->resolveCharacteristicInputs(externalInputs);

  // My Characteristic Inputs become the external inputs for my child.
  child(0) = child(0)->preCodeGen(generator,
				  getGroupAttr()->getCharacteristicInputs(),
				  pulledNewInputs);
  if (! child(0).getPtr())
    return NULL;

  // add one more value to "valuesGivenToChild_": a statement execution 
  // count that will invalidate cache each time the statement is 
  // re-executed. It would be incorrect to cache across
  // statement executions (and possibly transactions).
  ValueId execCount = generator->getOrAddStatementExecutionCount();
  pulledNewInputs += execCount;
  
  getGroupAttr()->addCharacteristicInputs(pulledNewInputs);

  // Accumulate the values that are provided as inputs by my parent
  // together with the values that are produced as outputs by my
  // children. Use these values for rewriting the VEG expressions.
  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);

  // Rewrite the selection predicates.
  NABoolean replicatePredicates = TRUE;
  selectionPred().replaceVEGExpressions
                     (availableValues,
		      getGroupAttr()->getCharacteristicInputs(),
		      FALSE, // no need to generate key predicates here
		      0 /* no need for idempotence here */,
		      replicatePredicates
		     );

  getGroupAttr()->resolveCharacteristicOutputs
                     (availableValues,
		      getGroupAttr()->getCharacteristicInputs());

/*
  TBD - maybe ProbeCache as BMO memory participant??
  if(CmpCommon::getDefault(PROBE_CACHE_MEMORY_QUOTA_SYSTEM) != DF_OFF)
    generator->incrNumBMOs();

  if ((ActiveSchemaDB()->getDefaults()).getAsDouble(BMO_MEMORY_LIMIT_PER_NODE_IN_MB) > 0)
    generator->incrNBMOsMemoryPerNode(getEstimatedRunTimeMemoryUsage(generator, TRUE));
*/
  markAsPreCodeGenned();
  return this;
    
} // ProbeCache::preCodeGen()


RelExpr * Exchange::preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  // Set a flag if this is a parallel extract consumer. The flag for
  // extract producer queries gets set earlier in RelRoot::codeGen()
  if (child(0)->getOperatorType() == REL_EXTRACT_SOURCE)
  {
    isExtractConsumer_ = TRUE;
    GenAssert(!isExtractProducer_,
              "One extact query cannot be both producer and consumer");
  }

  const PhysicalProperty* sppOfChild = child(0)->getPhysicalProperty();

  NABoolean PivsReplaced = FALSE;

  if (sppOfChild->getPlanExecutionLocation() == EXECUTE_IN_DP2) {
    // If this is not an ESP exchange, then check if the pivs of this op
    // and it's child are the same. If they are not, make them the same.
    // We don't do this for an ESP exchange because an ESP exchange
    // denotes an ESP process boundary and the child's pivs
    // do not have to be the same as the parent and in fact should
    // not be the same.
    replacePivs();
    PivsReplaced = TRUE;
  }

  RelExpr *result = this;

  // ---------------------------------------------------------------------
  // copy important info from the properties into data members
  // ---------------------------------------------------------------------
   
  storePhysPropertiesInNode(generator->getPrefixSortKey());

  // If this is a parallel extract producer query:
  // - do a few checks to make sure the plan is valid
  // - store a copy of the root's select list
  if (isExtractProducer_)
  {
    RelRoot *root = generator->getBindWA()->getTopRoot();

    // The plan is valid if this is an ESP exchange the number of
    // bottom partitions matches the number of requested streams.
    ComUInt32 numRequestedStreams = root->getNumExtractStreams();
    ComUInt32 numBottomEsps = (ComUInt32)
      getBottomPartitioningFunction()->getCountOfPartitions();
    if (!isEspExchange() || (numRequestedStreams != numBottomEsps))
    {
      *CmpCommon::diags() << DgSqlCode(-7004);
      GenExit();
      return NULL;
    }

    // Make a copy of the root's select list
    extractSelectList_ = new (generator->wHeap())
      ValueIdList(root->compExpr());

    // Do a coverage test to see find values in the select list that
    // this operator cannot already provide
    ValueIdSet valuesIDontHave(*extractSelectList_);
    ValueIdSet coveredExpr;
    ValueIdSet referencedUpperValues;
    getGroupAttr()->coverTest(valuesIDontHave,        // expressions needed
                              externalInputs,         // extra inputs
                              coveredExpr,            // covered exprs
                              referencedUpperValues); // new values needed

    // Add the needed values to characteristic inputs
    pulledNewInputs += referencedUpperValues;
    getGroupAttr()->addCharacteristicInputs(referencedUpperValues);
  }

  // ---------------------------------------------------------------------
  // Resolve the VEGReferences and VEGPredicates, if any, that appear
  // in the Characteristic Inputs, in terms of the externalInputs.
  // ---------------------------------------------------------------------
  ValueIdSet saveCharInputs = getGroupAttr()->getCharacteristicInputs();
  getGroupAttr()->resolveCharacteristicInputs(externalInputs);

  // variables that store the result of the major decisions:
  //
  // makeThisExchangeAPapa: if this is a PAPA node, then make this
  //                        node the PAPA (and add a PA below it)
  // eliminateThisExchange: get rid of this node either because
  //                        it represents a sole PA or because it is
  //                        a redundant ESP exchange
  // topPartFunc_:          the partitioning function produced by
  //                        this node after we're done with preCodeGen
  // bottomPartFunc_:       the partitioning function produced by
  //                        the child of this node
  // paPartFunc:            the partitioning function produced by the
  //                        PA node inserted below
  // lbpf                   LogPhysPartitioningFunction of the child,
  //                        if the child has such a part. function
  NABoolean makeThisExchangeAPapa            = FALSE;
  NABoolean eliminateThisExchange            = FALSE;
  const PartitioningFunction *paPartFunc     = topPartFunc_;
  const LogPhysPartitioningFunction *lppf    = NULL;


  if (isDP2Exchange() AND
      bottomPartFunc_->isALogPhysPartitioningFunction())
  {
    lppf = bottomPartFunc_->castToLogPhysPartitioningFunction();

    if (lppf->getUsePapa() || getGroupAttr()->isEmbeddedUpdateOrDelete())
    {

      // Will a merge of sorted streams need to be done?
      if (NOT sortKeyForMyOutput_.isEmpty())
      {
        Lng32 maxPartsPerGroup;

        // Since a merge of sorted streams is needed, we must
        // ensure that there is one PA for every partition in every
        // process. The optimizer should already have set this up
        // correctly, but sometimes, due to plan stealing, the value
        // can be wrong. This code is really a patch for the plan
        // stealing problem. We could try to fix the plan stealing
        // problem, but that would adversely affect compile time.
        // To set the number of clients (i.e. PAs) we must cast away
        // the const-ness, sorry.
        if (topPartFunc_->isAGroupingOf(*bottomPartFunc_,
                                        &maxPartsPerGroup))
        {
          ((LogPhysPartitioningFunction*)lppf)->setNumOfClients(
            maxPartsPerGroup * topPartFunc_->getCountOfPartitions());
        }
        else
        {
          ((LogPhysPartitioningFunction*)lppf)->setNumOfClients(
            bottomPartFunc_->getCountOfPartitions() *
            topPartFunc_->getCountOfPartitions());
        }
      }


      // Keep this exchange and make it the PAPA node. The PA
      // nodes below the PAPA will actually produce a partitioning
      // scheme that is identical to that of the DP2 operator below,
      // since the PAPA splits its requests into smaller ones that
      // do not span DP2 partition boundaries.
      makeThisExchangeAPapa = TRUE;
      paPartFunc = bottomPartFunc_;
    }
  }

  if  (!PivsReplaced && isRedundant_)
    replacePivs();

  // flag to decide whether to use the characteristic inputs or outputs
  // as input the to the CIF determineInternalFormatFunction
  // if the the child is an insert or update then we consider the chars input
  // otherwise we use the chars outputs
  NABoolean useCharInputs = FALSE;

  // ---------------------------------------------------------------------
  // If the child of this Exchange executes in DP2, then allocate a
  // PartitionAccess operator. It should have the same Group Attributes
  // as its child.
  // ---------------------------------------------------------------------
  NABoolean savedOltMsgOpt = generator->oltOptInfo()->oltMsgOpt();

  NABoolean inputOltMsgOpt = generator->oltOptInfo()->oltMsgOpt();

  unsigned short prevNumBMOs = generator->replaceNumBMOs(0);

  // These are used to fix solution 10-071204-9253 and for 
  // solution 10-100310-8659.
  bool needToRestoreParallel = false;
  NABoolean savedParallelSetting = FALSE;
  bool needToRestoreCheckUnsync = false;
  NABoolean savedCheckUnsyncdSort = FALSE;
  bool needToRestoreLHS = false;
  bool halloweenLHSofTSJ = generator->getPrecodeHalloweenLHSofTSJ();
  bool needToRestoreESP = false;
  bool halloweenESPonLHS = generator->getHalloweenESPonLHS();

  if (isEspExchange() && getBottomPartitioningFunction()->isPartitioned())
    {
      // Tell any child NJ that its Halloween blocking operator (SORT)
      // is operating in parallel.
      savedParallelSetting = generator->preCodeGenParallelOperator();
      generator->setPreCodeGenParallelOperator(TRUE);
      needToRestoreParallel = true;
    }

  if (isEspExchange() && halloweenLHSofTSJ)
    {
      if ( !isRedundant_ )
        {
          // Tell any parallel SORT below that it doesn't have to check
          // unsyncd access.
          needToRestoreESP = true;
          generator->setHalloweenESPonLHS(true);
        }

      savedCheckUnsyncdSort = generator->checkUnsyncdSort();
      if (savedCheckUnsyncdSort == TRUE)
        {
          // savedCheckUnsyncdSort tells me there is a parallel SORT above this 
          // exchange.  This ESP guarantees that all instances of the SORT will
          // block until all instances of this ESP finish. So tell any child 
          // PARTITION ACCESS that its scan of a self-referencing is sync'd.
          generator->setCheckUnsyncdSort(FALSE);
          needToRestoreCheckUnsync = true;

          // More for 10-100310-8659 - don't call incUnblockedHalloweenScans
          // below this node. 
          halloweenLHSofTSJ = generator->setPrecodeHalloweenLHSofTSJ(false);
          needToRestoreLHS = true;
        }
    }
  else if (isEspExchange() && 
           // this isPartitioned() condition is probably a bug, but 
           // to be safe I am not fixing it now.
           getBottomPartitioningFunction()->isPartitioned())
    {

      // Tell any child PARTITION ACCESS that its scan of a self-referencing
      // table is synchronized by an ESP exchange.  That is, any blocking
      // SORT operator above this exchange will not get any rows until all
      // scans have finished.
      savedCheckUnsyncdSort = generator->checkUnsyncdSort();
      generator->setCheckUnsyncdSort(FALSE);

      needToRestoreCheckUnsync = true;
    }

  if (halloweenSortIsMyChild_ && isRedundant_)
  {
    // Before eliminating itself, and before preCodeGen'ing the child
    // tree, this Exchange will tell its child (a Sort) that it needs to 
    // check for unsynchronized access to the target table of a 
    // self-referencing update.  This is part of the fix for 
    // solution 10-090310-9876.
    ((Sort *)(child(0).getPtr()))->doCheckAccessToSelfRefTable();

    // Note for solution 10-100310-8659 -- the halloweenSortIsMyChild_
    // flag will only be set when the COMP_BOOL_166 is used to revert
    // to pre-bugfix behavior.  With the fix for 10-100310-8659, the 
    // Sort uses the Generator's flags (precodeHalloweenLHSofTSJ and
    // precodeRHSofNJ) to know if it needs check access to the target 
    // table.  In other words, unless COMP_BOOL_166 is used, this
    // is dead code.
  }
  if ( CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) == DF_SYSTEM)
  {
    NABoolean resize = FALSE;
    NABoolean defrag = FALSE;
    ValueIdSet vidSet;
    if (!useCharInputs)
    {
      vidSet = child(0)->getGroupAttr()->getCharacteristicOutputs();
    }
    else
    {
      vidSet = saveCharInputs;
    }

    ExpTupleDesc::TupleDataFormat tupleFormat =
                  determineInternalFormat( vidSet,
                                           this,
                                           resize,
                                           generator,
                                           FALSE,
                                           defrag);

    cacheTupleFormatAndResizeFlag(tupleFormat, resize, defrag);

    if (tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
    {
      generator->incNCIFNodes();
    }
    else
    {
      generator->decNCIFNodes();
    }
  }

  // For HashJoin MIN/MAX optimization.  If this is an ESP Exchange,
  // block all candidate values for min/max optimization from going
  // below this Exchange.  Restore them upon return from
  // preCodeGen'ing the child.
  ValueIdList minMaxKeys, minVals, maxVals, willUseMinMaxKeys;
  if(isEspExchange()) {

    // Save the current values.
    minMaxKeys = generator->getMinMaxKeys();
    minVals = generator->getMinVals();
    maxVals = generator->getMaxVals();
    willUseMinMaxKeys = generator->getWillUseMinMaxKeys();

    
    // Clear the current values.
    generator->getMinMaxKeys().clear();
    generator->getMinVals().clear();
    generator->getMaxVals().clear();
    generator->getWillUseMinMaxKeys().clear();
  }

  // ---------------------------------------------------------------------
  // Perform preCodeGen on the child (including PA node if we created it)
  // ---------------------------------------------------------------------
  child(0) = child(0)->preCodeGen(
       generator,
       getGroupAttr()->getCharacteristicInputs(),
       pulledNewInputs);

  // For HashJoin MIN/MAX optimization.
  if(isEspExchange()) {
    // Restore the saved values.
    generator->getMinMaxKeys() = minMaxKeys;
    generator->getMinVals() = minVals;
    generator->getMaxVals() = maxVals;
    generator->getWillUseMinMaxKeys() = willUseMinMaxKeys;
  }

  if (needToRestoreParallel)
    generator->setPreCodeGenParallelOperator(savedParallelSetting);
  if (needToRestoreCheckUnsync)
    generator->setCheckUnsyncdSort(savedCheckUnsyncdSort);
  if (needToRestoreLHS)
    generator->setPrecodeHalloweenLHSofTSJ(halloweenLHSofTSJ);
  if (needToRestoreESP)
    generator->setHalloweenESPonLHS(halloweenESPonLHS);

  setNumBMOs( generator->replaceNumBMOs(prevNumBMOs) );

  if (! child(0).getPtr())
    return NULL;

  generator->oltOptInfo()->setOltMsgOpt(savedOltMsgOpt);

  // Decide whether this Exchange should try to eliminate itself.
  if (child(0)->castToRelExpr()->getOperatorType() == REL_EXE_UTIL)
    {
      // No, the REL_EXE_UTIL must execute in an ESP.
    }
  else if (skipRedundancyCheck_)
    {
      // No, the ESP was inserted just to force blocking of 
      // data from SORT instances, to help prevent Halloween
      // problem -- see Soln 10-071204-9253.
    }
  else
    {
      // Yes, perform the redundancy check.
      eliminateThisExchange = (isRedundant_ OR
          (isDP2Exchange() AND NOT makeThisExchangeAPapa));
    }

  // ---------------------------------------------------------------------
  // Determine which partition input values need to be supplied by our
  // parent and which are produced by this exchange node.  PA or PAPA
  // exchange nodes (DP2 exchange nodes) do not produce any partition
  // input values themselves, just ask the parent to produce the PIVs
  // needed by the child. ESP exchanges produce the PIVs for their bottom
  // partition function, and this is also true for added repartitioning
  // exchanges.
  // ---------------------------------------------------------------------
  if (isEspExchange())
    {
      pulledNewInputs -= bottomPartFunc_->getPartitionInputValues();
      setBottomPartitionInputValues(
	   bottomPartFunc_->getPartitionInputValuesLayout());
    }

  getGroupAttr()->addCharacteristicInputs(pulledNewInputs);

  // ---------------------------------------------------------------------
  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  // ---------------------------------------------------------------------
  ValueIdSet availableValues;

  getInputAndPotentialOutputValues(availableValues);

  // Accumulate the values that are provided as inputs by my parent
  // together with the values that are produced as outputs by my
  // children. Use these values for rewriting the VEG expressions.

  // ---------------------------------------------------------------------
  // Rewrite the copy of the sort key which will be used for merging
  // rows. The VEGRef on the column being sorted may be preceeded by 
  // an InverseOrder itemExpr (in case the shortcut_grby rule has fired)
  // The InverseOrder itemExpr will not perform a copy of the sortKey
  // before replacing VEGExpressions unless replicateExpression is set 
  // to TRUE below. This avoids inverse(VEGRef_60(T1.a = T2.a)) being 
  // resolved to T1.a in two different exchange nodes, even though T1.a 
  // is not available at the second exchange node.
  // ---------------------------------------------------------------------
  NABoolean replicateExpression = TRUE;
  sortKeyForMyOutput_.replaceVEGExpressions
    (availableValues,
     getGroupAttr()->getCharacteristicInputs(),
     FALSE, // no key predicates here
     0 /* no need for idempotence here */,
     replicateExpression
     );

  // ---------------------------------------------------------------------
  // Rewrite the partitioning expression, if the repartitioning function
  // contains one. A ReplicationPartitioningFunction does not contain
  // a partititioning expression because it uses a broadcast for
  // replicating rows to its consumers.
  // ---------------------------------------------------------------------
  if (isEspExchange())
  {
    PartitioningFunction * rpf;

    // need to cast away const-ness to create partitioning expr, sorry
    rpf = (PartitioningFunction *) topPartFunc_;

    rpf->createPartitioningExpression();
    rpf->preCodeGen(availableValues);
  }

  // ---------------------------------------------------------------------
  // For a parallel extract producer query, rewrite our copy of the
  // root's select list
  // ---------------------------------------------------------------------
  if (isExtractProducer_)
  {
    extractSelectList_->
      replaceVEGExpressions(availableValues,
                            getGroupAttr()->getCharacteristicInputs());
  }

  // ---------------------------------------------------------------------
  // Resolve characteristic outputs.
  // ---------------------------------------------------------------------
  getGroupAttr()->resolveCharacteristicOutputs
                      (availableValues,
		       getGroupAttr()->getCharacteristicInputs());

  generator->oltOptInfo()->mayDisableOperStats(&oltOptInfo());

  // ---------------------------------------------------------------------
  // From here on we add or remove exchange nodes, but this node is
  // ready and does not need to be processed again should we call
  // preCodeGen for it again.
  // ---------------------------------------------------------------------
  markAsPreCodeGenned();

  // ---------------------------------------------------------------------
  // Eliminate this exchange if it simply represented the PA node or
  // if it is redundant. Do not eliminate the exchange if it is a
  // parallel extract producer or consumer.
  // ---------------------------------------------------------------------
  if (isExtractProducer_ || isExtractConsumer_)
    eliminateThisExchange = FALSE;

  if (eliminateThisExchange)
  {
    result = child(0).getPtr();

    // transfer the # of BMOs to generator as
    // this exchange node is to be discarded.
    generator->incrNumBMOsPerFrag(getNumBMOs());
  }

  if ((isEspExchange()) &&
      (NOT eliminateThisExchange))
    {
      //      generator->setUpdAbortOnError(TRUE);
      generator->setUpdSavepointOnError(FALSE);
      generator->setUpdErrorOnError(FALSE);
      
      generator->compilerStatsInfo().exchangeOps()++;
      
      generator->compilerStatsInfo().dop() =
	(UInt16)MAXOF(generator->compilerStatsInfo().dop(),
		      getBottomPartitioningFunction()->getCountOfPartitions());
      
      
      if ( getNumBMOs() > 0 )
	generator->incTotalESPs();
      
      // If the exchange uses SeaMonster, set a flag in the generator
      // to indicate that some part of the query does use SeaMonster
      if (thisExchangeCanUseSM(generator->getBindWA()))
        generator->setQueryUsesSM(TRUE);
      
    } // isEspExchange() && !eliminateThisExchange
  

  
  return result;
  
} // Exchange::preCodeGen()

RelExpr * Tuple::preCodeGen(Generator * generator,
		       const ValueIdSet & externalInputs,
		       ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! RelExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  // Accumulate the values that are provided as inputs by my parent
  // together with the values that are produced as outputs by my
  // children. Use these values for rewriting the VEG expressions.
  ValueIdSet availableValues = getGroupAttr()->getCharacteristicInputs();
  tupleExpr().replaceVEGExpressions
                 (availableValues,
		  getGroupAttr()->getCharacteristicInputs());

  return this;
}

ItemExpr * BuiltinFunction::preCodeGen(Generator * generator)
{
  ItemExpr * retExpr = NULL;

  if (nodeIsPreCodeGenned())
    return getReplacementExpr();
	
  if (CmpCommon::getDefault(OR_PRED_KEEP_CAST_VC_UCS2) == DF_ON) {
    // part of temporary workaround to yotta dp2 killer problem:
    // keep cast for upper(cast name as varchar(n) char set ucs2)
    switch (getOperatorType()) {
    case ITM_UPPER:
    case ITM_LOWER:
    case ITM_SUBSTR:
    case ITM_TRIM:
      if (child(0)->getOperatorType() == ITM_CAST) {
        Cast *arg = (Cast*)child(0)->castToItemExpr();
        const NAType& typ = arg->getValueId().getType();
        if (arg->matchChildType() &&
            arg->child(0)->getValueId().getType() == typ &&
            typ.getTypeQualifier() == NA_CHARACTER_TYPE && 
            typ.isVaryingLen() &&
            ((CharType*)(&typ))->getCharSet() == CharInfo::UCS2) {
          // don't skip codegen for the cast of 
          // "upper(cast name as varchar(n) char set ucs2) IN <inlist>"
          arg->setMatchChildType(FALSE);
        }
      }
    }
  }

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  switch (getOperatorType())
    {
    case ITM_QUERYID_EXTRACT:
      {
        // convert arguments to ISO88591 character set

        if (child(0)->castToItemExpr()->getValueId().getType().getTypeQualifier() ==
            NA_CHARACTER_TYPE)
          {
            const CharType &typ0 =
              (const CharType &) (child(0)->castToItemExpr()->getValueId().getType());
            if (typ0.getCharSet() != CharInfo::ISO88591)
              {
                // the executor method assumes an ASCII string for the query id, so
                // convert the value to a fixed char type in the ISO88591 char set
                SQLChar * newTyp0 = new(generator->wHeap())
                  SQLChar(generator->wHeap(), typ0.getCharLimitInUCS2or4chars(),
                          typ0.supportsSQLnullLogical(),
                          typ0.isUpshifted(),
                          typ0.isCaseinsensitive(),
                          typ0.isVaryingLen(),
                          CharInfo::ISO88591);
                
                child(0) = new (generator->wHeap()) Cast(child(0), newTyp0);
                child(0)->bindNode(generator->getBindWA());
                child(0) = child(0)->preCodeGen(generator);
              }
          }

        if (child(1)->castToItemExpr()->getValueId().getType().getTypeQualifier() ==
            NA_CHARACTER_TYPE)
          {
            const CharType &typ1 =
              (const CharType &) (child(1)->castToItemExpr()->getValueId().getType());
            if (typ1.getCharSet() != CharInfo::ISO88591)
              {
                // the executor method assumes an ASCII string for the query id, so
                // convert the value to a fixed char type in the ISO88591 char set
                SQLChar * newTyp1 = new(generator->wHeap())
                  SQLChar(generator->wHeap(), typ1.getCharLimitInUCS2or4chars(),
                          typ1.supportsSQLnullLogical(),
                          typ1.isUpshifted(),
                          typ1.isCaseinsensitive(),
                          typ1.isVaryingLen(),
                          CharInfo::ISO88591);
                
                child(1) = new (generator->wHeap()) Cast(child(1), newTyp1);
                child(1)->bindNode(generator->getBindWA());
                child(1) = child(1)->preCodeGen(generator);
              }
          }
      }
      retExpr = this;
      break;


    default:
      {
	retExpr = this;
      }
      break;
    } // switch

  setReplacementExpr(retExpr);

  markAsPreCodeGenned();
  return retExpr;
}

/*
ItemExpr * Abs::preCodeGen(Generator * generator)
{
  // The ABS function has the distinction of being the sole BuiltinFunction
  // that a) generates a new replacementExpr tree
  // and b) can appear in the select-list (compExpr).
  //
  // What happens is that code is generated for the ABS replacement CASE
  // TWICE, once in PartitionAccess eid, once in RelRoot generateOutputExpr:
  // the latter fails with a GenMapTable assert failing to find info for
  // the column in "SELECT ABS(col) FROM t;"
  // ("SELECT ABS(-1) FROM t;" and "SELECT ABS(col),col FROM T;" work fine --
  // but of course they generate twice as much code as necessary,
  // however harmless/idempotent it may be...)
  //
  // We therefore cannot handle this one discrepant case neatly in
  // preCodeGen/codeGen -- it is fixed instead by having the Binder
  // upstream rewrite an ABS as the equivalent CASE.
  //
  // Genesis 10-980112-5942.
  //
  GenAssert(FALSE, "Abs::preCodeGen should be unreachable code!");
  return NULL;

//if (nodeIsPreCodeGenned())
//  return getReplacementExpr();
//
//ItemExpr * newExpr =
//  generator->getExpGenerator()->createExprTree(
//    "CASE WHEN @A1 < 0 THEN - @A1 ELSE @A1 END", 0, 1, child(0));
//
//newExpr->bindNode(generator->getBindWA());
//setReplacementExpr(newExpr->preCodeGen(generator));
//markAsPreCodeGenned();
//return getReplacementExpr();

}
*/

ItemExpr * Abs::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return getReplacementExpr();

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  NAType * result_type = (NAType *)(&(getValueId().getType()));
  NAType * type_op1 = (NAType *)(&(child(0)->castToItemExpr()->getValueId().getType()));

  if (! (*result_type == *type_op1))
    {
      // Insert a cast node to convert child to a result type.
      child(0) = new (generator->wHeap())
	Cast(child(0), result_type);

      child(0)->bindNode(generator->getBindWA());

      child(0) = child(0)->preCodeGen(generator);
      if (! child(0).getPtr())
	return NULL;
    }

  markAsPreCodeGenned();
  return this;
} // Abs::preCodeGen()

ItemExpr * AggrMinMax::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return getReplacementExpr();

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  // if my child's attributes EXCEPT for nullability are not the
  // same as mine, do a conversion.
  NABoolean doConversion = FALSE;
  const NAType &myType = getValueId().getType();
  const NAType &childType = child(0)->castToItemExpr()->getValueId().getType();
  if (NOT (myType == childType)) // something is different
    {
      if ((myType.supportsSQLnull() &&
	   childType.supportsSQLnull()) ||
	  ((NOT myType.supportsSQLnull()) &&
	   (NOT childType.supportsSQLnull())))
	doConversion = TRUE; // both nullable or not nullable,
                             // something else is different
      else if (myType.supportsSQLnull() &&
	       NOT childType.supportsSQLnull())
	{
	  // create a new my type with the same null attr as child.
	  NAType * newType = myType.newCopy(generator->wHeap());
	  newType->resetSQLnullFlag();

	  if (NOT(*newType == childType))
	    doConversion = TRUE;

	  delete newType;
	}
      else
	{
	  // Fix for solution ID 10-031121-1505)
	  // I dont think we the following assert is correct
	  // During VEG resolution a MIN/MAX() function can have a
	  // NON-NULLABLE child replaced by a nullable child, consider
	  // as an example the following query where i2 is not null:
	  //
	  // SELECT MIN(T0.i2)
          // FROM D12 T0
          // WHERE
          //  ?pa2  =  T0.i2
          // GROUP BY T0.i1;
          //
          // In the above case i2 will be replaced by ?pa2 when the VEG
          // (i2, ?pa2) is resolved. Therefore it is possible to get a
          // nullable child for a non-nullable aggregate. In the above
          // case the aggregate is non-nullable because i2 is non-nullable.
          // In such a case MIN(?pa2) would never be executed if ?pa2 is NULL
          // because predicate '?pa2 = T0.i2' will not select any rows when
          // ?pa2 is NULL (I am not sure how a parameter is set to NULL, for host
          // vars we can use the NULL indicator, not sure how we pass in NULL using
          // parameters).
          //
          // Assert on the following condition
	  // The condition where I am not nullable and my child is nullable,
	  // is an error case.
	  //GenAssert(0, "AggrMinMax::preCodeGen::Should not reach here.");
	  doConversion = TRUE;
	}
    }

  if (doConversion)
    {
      // Insert a cast node to convert child to a result type.
      child(0) = new (generator->wHeap()) Cast(child(0), &myType);

      child(0)->bindNode(generator->getBindWA());

      child(0) = child(0)->preCodeGen(generator);
      if (! child(0).getPtr())
	return NULL;
    }

  markAsPreCodeGenned();
  return this;
} // AggrMinMax::preCodeGen()

ItemExpr *Overlaps::preCodeGen(Generator *generator)
{
  if (nodeIsPreCodeGenned())
    return getReplacementExpr();

  for (Int32 i = 0; i < getArity(); ++i)
    {
      if (child(i)) 
        {
          const NAType &type = 
            child(i)->getValueId().getType();
          const DatetimeType *operand = (DatetimeType *)&type;

          if (type.getTypeQualifier() == NA_DATETIME_TYPE
                 && (operand->getPrecision() == SQLDTCODE_DATE))
            {
              child(i) = new (generator->wHeap()) 
                Cast(child(i), new (generator->wHeap()) 
                     SQLTimestamp(generator->wHeap(), TRUE));

              child(i)->bindNode(generator->getBindWA());
            }
        }
    }

  //General Rules:
  //1) ... 2) ... 3) ... 
  //4) if D1(child(0)) is the null value or if E1(child(1))<D1,
  //   then let S1 = E1 and let T1 = D1.
  //   Otherwise, let S1 = D1 and let T1 = E1.
  //
  ItemExpr *S1 = NULL;
  ItemExpr *T1 = NULL;
  S1 = generator->getExpGenerator()->createExprTree(
      "CASE WHEN (@A2<@A1 OR @A1 IS NULL) THEN @A2 ELSE @A1 END"
      , 0
      , 2
      , child(0), child(1)); 
  T1 = generator->getExpGenerator()->createExprTree(
      "CASE WHEN (@A2<@A1 OR @A1 IS NULL) THEN @A1 ELSE @A2 END"
      , 0
      , 2
      , child(0), child(1));

  child(0) = S1->bindNode(generator->getBindWA());
  child(1) = T1->bindNode(generator->getBindWA());


  //General Rules:
  //1) ... 2) ... 3) ... 4) ... 5) ...
  //6) if D2(child(2)) is the null value or if E2(child(3))<D2,
  //   then let S2 = E2 and let T2 = D2.
  //   Otherwise, let S2 = D2 and let T2 = E2.
  //
  ItemExpr *S2 = NULL;
  ItemExpr *T2 = NULL;
  S2 = generator->getExpGenerator()->createExprTree(
      "CASE WHEN (@A2<@A1 OR @A1 IS NULL) THEN @A2 ELSE @A1 END"
      , 0
      , 2
      , child(2), child(3)); 
  T2 = generator->getExpGenerator()->createExprTree(
      "CASE WHEN (@A2<@A1 OR @A1 IS NULL) THEN @A1 ELSE @A2 END"
      , 0
      , 2
      , child(2), child(3));

  child(2) = S2->bindNode(generator->getBindWA());
  child(3) = T2->bindNode(generator->getBindWA());

  ItemExpr *newExpr = 
    generator->getExpGenerator()->createExprTree(
  //General Rules:
  //1) ... 2) ... 3) ... 4) ... 5) ... 6) ...
  //7) The result of the <overlaps predicate> is 
  //   the result of the following expression:
        "(@A1 > @A3 AND NOT (@A1 >= @A4 AND @A2 >= @A4))"
        " OR "
        "(@A3 > @A1 AND NOT (@A3 >= @A2 AND @A4 >= @A2))"
        " OR "
        "(@A1 = @A3 AND (@A2 <> @A4 OR @A2=@A4))"
        , 0
        , 4
        , child(0), child(1), child(2), child(3));

  newExpr->bindNode(generator->getBindWA());
  setReplacementExpr(newExpr->preCodeGen(generator));
  markAsPreCodeGenned();
  return getReplacementExpr();
}


ItemExpr * Between::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return getReplacementExpr();

  // transform "A BETWEEN B AND C" to "A >= B AND A <= C"
  ItemExpr * newExpr =
    generator->getExpGenerator()->createExprTree(
      "@A1 >= @A2 AND @A1 <= @A3", 0, 3, child(0), child(1), child(2));

  newExpr->bindNode(generator->getBindWA());
  setReplacementExpr(newExpr->preCodeGen(generator));
  markAsPreCodeGenned();
  return getReplacementExpr();
}

// BiArithCount::preCodeGen
//
// The BiArithCount executor clause requires that all of the operands
// be of the same type. preCodeGen introduces cast operators on the
// input operands if necessary to enforce this requirement.
//
ItemExpr * BiArithCount::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  // Get a local handle on common generator objects.
  //
  CollHeap *wHeap = generator->wHeap();

  const NAType &resultType = getValueId().getType();
  const NAType &op1Type = child(0)->castToItemExpr()->getValueId().getType();
  const NAType &op2Type = child(1)->castToItemExpr()->getValueId().getType();

  // If the first operand type does not match that of the result,
  // cast it to the result type.
  //
  if(!(op1Type == resultType))
    {
      child(0) = new(wHeap) Cast(child(0)->castToItemExpr(),
				resultType.newCopy(wHeap),
				ITM_CAST);
      child(0)->synthTypeAndValueId();
    }

  // Ditto for the second operand.
  //
  if(!(op2Type == resultType))
    {
      child(1) = new(wHeap) Cast(child(1)->castToItemExpr(),
				 resultType.newCopy(wHeap),
				 ITM_CAST);
      child(1)->synthTypeAndValueId();
    }

  return BiArith::preCodeGen(generator);
}

// BiArithSum::preCodeGen
//
// The BiArithSum executor clause requires that all of the operands
// be of the same type. preCodeGen introduces cast operators on the
// input operands if necessary to enforce this requirement.
//
ItemExpr * BiArithSum::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  // Get a local handle on common generator objects.
  //
  CollHeap *wHeap = generator->wHeap();

  // Get a handle on the operand types.
  //
  const NAType &resultType = getValueId().getType();
  const NAType &op1Type = child(0)->castToItemExpr()->getValueId().getType();
  const NAType &op2Type = child(1)->castToItemExpr()->getValueId().getType();

  // If the first operand type does not match that of the result,
  // cast it to the result type.
  //
  if(!(op1Type == resultType))
    {
      child(0) = new(wHeap) Cast(child(0)->castToItemExpr(),
				resultType.newCopy(wHeap),
				ITM_CAST);
      child(0)->synthTypeAndValueId();
    }

  // Ditto for the second operand.
  //
  if(!(op2Type == resultType))
    {
      child(1) = new(wHeap) Cast(child(1)->castToItemExpr(),
				 resultType.newCopy(wHeap),
				 ITM_CAST);
      child(1)->synthTypeAndValueId();
    }

  ItemExpr *result = BiArith::preCodeGen(generator);
  if (! result)
    return NULL;

  ItemExpr *outExpr = NULL;
  Lng32 rc = generator->getExpGenerator()->foldConstants(child(0), &outExpr);
  if ((rc == 0) &&
      (outExpr))
    {
      child(0) = outExpr->preCodeGen(generator);
    }

  rc = generator->getExpGenerator()->foldConstants(child(1), &outExpr);
  if ((rc == 0) &&
      (outExpr))
    {
      child(1) = outExpr->preCodeGen(generator);
    }

  return this;
}

ItemExpr * BiArith::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  NAType * result_type = (NAType *)(&(getValueId().getType()));
  NAType * type_op1 = (NAType *)(&(child(0)->castToItemExpr()->getValueId().getType()));
  NAType * type_op2 = (NAType *)(&(child(1)->castToItemExpr()->getValueId().getType()));

  if (result_type->isComplexType())
    {
      if ((getOperatorType() == ITM_PLUS) ||
	  (getOperatorType() == ITM_MINUS))
	{
	  child(0) = generator->getExpGenerator()->matchScales(child(0)->getValueId(),
							       *result_type);

	  child(1) = generator->getExpGenerator()->matchScales(child(1)->getValueId(),
							       *result_type);
	}
      else
	if (getOperatorType() == ITM_DIVIDE)
	  {
	    // before doing the division, the numerator has to be upscaled.
	    // Lets find out how much.
	    // NS = numerator scale
	    // DS = denominator scale
	    // RS = result scale
	    // Upscale = (RS - NS) + DS
	    // Newscale = NS + Upscale = RS + DS

	    Lng32 newscale
	      = ((NumericType *)result_type)->getScale() +
		((NumericType *)type_op2)->getScale();

	    if (newscale != ((NumericType *)type_op1)->getScale()) {
	      NAType * new_type = result_type->newCopy(generator->wHeap());
	      ((NumericType *)new_type)->setScale(newscale);
	      child(0) = generator->getExpGenerator()->matchScales(
							 child(0)->getValueId(),
							 *new_type);
	    }
	  }

      type_op1 = (NAType *)(&(child(0)->getValueId().getType()));
      type_op2 = (NAType *)(&(child(1)->getValueId().getType()));

      if (result_type->getFSDatatype() == type_op1->getFSDatatype())
	{
	  if (((getOperatorType() == ITM_PLUS) ||
	       (getOperatorType() == ITM_MINUS) ||
	       (getOperatorType() == ITM_DIVIDE)) &&
	      (result_type->getNominalSize() != type_op1->getNominalSize()))
	    {
	      child(0) = new(generator->wHeap()) Cast(child(0), result_type);
	    }
	}
      else
	{
	  if ((getOperatorType() == ITM_PLUS) ||
	      (getOperatorType() == ITM_MINUS) ||
	      (getOperatorType() == ITM_DIVIDE))
	    {
	      child(0) = new(generator->wHeap()) Cast(child(0), result_type);
	    }
	  else
	    {
	      child(0) =
	        new(generator->wHeap()) Cast(child(0),
	                 result_type->synthesizeType(SYNTH_RULE_PASS_THRU_NUM,
                                                     *type_op1,
	                                             *result_type,
						     generator->wHeap()));
	    }
	}

      if (result_type->getFSDatatype() == type_op2->getFSDatatype())
	{
	  if (((getOperatorType() == ITM_PLUS) ||
	       (getOperatorType() == ITM_MINUS)) &&
	      (result_type->getNominalSize() != type_op2->getNominalSize()))
	    {
	      child(1) = new(generator->wHeap()) Cast(child(1), result_type);
	    }
	}
      else
	{
	  if ((getOperatorType() == ITM_PLUS) ||
	      (getOperatorType() == ITM_MINUS))
	    {
	      child(1) = new(generator->wHeap()) Cast(child(1), result_type);
	    }
	  else
	    {
	      child(1) =
	        new(generator->wHeap()) Cast(child(1),
	                 result_type->synthesizeType(SYNTH_RULE_PASS_THRU_NUM,
                                                     *type_op2,
	                                             *result_type,
						     generator->wHeap()));
	    }
	}

      child(0)->bindNode(generator->getBindWA());
      child(1)->bindNode(generator->getBindWA());

      child(0) = child(0)->preCodeGen(generator);
      if (! child(0).getPtr())
	return NULL;

      child(1) = child(1)->preCodeGen(generator);
      if (! child(1).getPtr())
	return NULL;

      markAsPreCodeGenned();

      return this;
    }

  // following is for simple types.
  SimpleType * attr_result = (SimpleType *)
    (ExpGenerator::convertNATypeToAttributes
     (getValueId().getType(), generator->wHeap()));
  SimpleType * attr_op1 = (SimpleType *)
    (ExpGenerator::convertNATypeToAttributes
     (child(0)->getValueId().getType(), generator->wHeap()));
  SimpleType * attr_op2 = (SimpleType *)
    (ExpGenerator::convertNATypeToAttributes
     (child(1)->getValueId().getType(), generator->wHeap()));

  // see if conversion needed before arithmetic operation could be done.

  Int32 matchScale = 0;
  if (result_type->getTypeQualifier() == NA_NUMERIC_TYPE)
    {
      // match scales
      if ((getOperatorType() == ITM_PLUS) ||
	  (getOperatorType() == ITM_MINUS))
	{
	  child(0) = generator->getExpGenerator()->matchScales(child(0)->getValueId(),
							       *result_type);

	  child(1) = generator->getExpGenerator()->matchScales(child(1)->getValueId(),
							       *result_type);
	}
      else
	if (getOperatorType() == ITM_DIVIDE)
	  {
	    // before doing the division, the numerator has to be upscaled.
	    // Lets find out how much.
	    // NS = numerator scale
	    // DS = denominator scale
	    // RS = result scale
	    // Upscale = (RS - NS) + DS
	    // Newscale = NS + Upscale = RS + DS

	    Lng32 newscale
	      = ((NumericType *)result_type)->getScale() +
		((NumericType *)type_op2)->getScale();

	    if (newscale != ((NumericType *)type_op1)->getScale()) {
	      NAType * new_type = result_type->newCopy(generator->wHeap());
	      ((NumericType *)new_type)->setScale(newscale);
	      child(0) = generator->getExpGenerator()->matchScales(
							 child(0)->getValueId(),
							 *new_type);
              matchScale = 1;
	    }
	  }
    }

  else if (result_type->getTypeQualifier() == NA_INTERVAL_TYPE) {
    switch (getOperatorType()) {
    case ITM_PLUS:
    case ITM_MINUS:
      if (type_op1->getTypeQualifier() == NA_DATETIME_TYPE) {
        Lng32 fp1 = ((DatetimeType *) type_op1)->getFractionPrecision();
        Lng32 fp2 = ((DatetimeType *) type_op2)->getFractionPrecision();
        if (fp1 < fp2) {
          child(0) = new(generator->wHeap()) Cast(child(0), type_op2);
          child(0)->bindNode(generator->getBindWA());
        } else if (fp1 > fp2) {
          child(1) = new(generator->wHeap()) Cast(child(1), type_op1);
          child(1)->bindNode(generator->getBindWA());
        }
      } else {
        child(0) = generator->getExpGenerator()->matchIntervalEndFields(
                                                   child(0)->getValueId(),
                                                   *result_type);
        child(1) = generator->getExpGenerator()->matchIntervalEndFields(
                                                   child(1)->getValueId(),
                                                   *result_type);
        child(0) = generator->getExpGenerator()->matchScales(
                                                   child(0)->getValueId(),
                                                   *result_type);
        child(1) = generator->getExpGenerator()->matchScales(
                                                   child(1)->getValueId(),
                                                   *result_type);
        type_op1 = (NAType *)(&(child(0)->getValueId().getType()));
        type_op2 = (NAType *)(&(child(1)->getValueId().getType()));
        if (result_type->getNominalSize() != type_op1->getNominalSize()) {
          child(0) = new(generator->wHeap()) Cast(child(0), result_type);
          child(0)->bindNode(generator->getBindWA());
        }
        if (result_type->getNominalSize() != type_op2->getNominalSize()) {
          child(1) = new(generator->wHeap()) Cast(child(1), result_type);
          child(1)->bindNode(generator->getBindWA());
        }
      }
      break;
    case ITM_TIMES: {
      //
      // Unfortunately, the multiply node may be the root ItemExpr node, and
      // we can't change the root ItemExpr node since its ValueId has already
      // been stored away in the parent RelExpr's ValueIdLists.  We'll have to
      // move the expression down, e.g.
      //
      //   *   <-- same root -->   *
      //  / \                     / \
      // I   N      becomes      I   1
      //                         |
      //                         *
      //                        / \
      //                       N   N
      //                       |
      //                       I
      //
      if (type_op1->getTypeQualifier() == NA_INTERVAL_TYPE)
        child(0) = generator->getExpGenerator()->convertIntervalToNumeric(
                                                   child(0)->getValueId());
      else
        child(1) = generator->getExpGenerator()->convertIntervalToNumeric(
                                                   child(1)->getValueId());
      char str[20];
      strcpy(str, "@A1 * @A2");
      child(0) = generator->getExpGenerator()->createExprTree(str, 0,
                                                              2,
                                                              child(0),
                                                              child(1));
      child(0)->bindNode(generator->getBindWA());
      child(0) = generator->getExpGenerator()->convertNumericToInterval(
                                                 child(0)->getValueId(),
                                                 *result_type);
      strcpy(str, "001"); // to make sure it is not a tinyint
      child(1) = generator->getExpGenerator()->createExprTree(str, CharInfo::ISO88591);
      child(1)->bindNode(generator->getBindWA());
      type_op2 = (NAType *)(&(child(1)->getValueId().getType()));
      if ((result_type->getNominalSize() != type_op2->getNominalSize()) ||
          (type_op2->getFSDatatype() != REC_BIN16_SIGNED)) {
        IntervalType *interval = (IntervalType *) result_type;
        const Int16 DisAmbiguate = 0;
        child(1) = new(generator->wHeap()) Cast(child(1),
                            new(generator->wHeap()) SQLNumeric(generator->wHeap(), TRUE, /* signed */
                                           interval->getTotalPrecision(),
                                           0,
                                           DisAmbiguate, // added for 64bit proj.
                                           interval->supportsSQLnull()));
        child(1)->bindNode(generator->getBindWA());
      }
      break;
    }
    case ITM_DIVIDE: {
      //
      // Unfortunately, the divide node may be the root ItemExpr node, and
      // we can't change the root ItemExpr node since its ValueId has already
      // been stored away in the parent RelExpr's ValueIdLists.  We'll have to
      // move the expression down, e.g.
      //
      //  div  <-- same root -->  div
      //  / \                     / \
      // I   N      becomes      I   1
      //                         |
      //                        div
      //                        / \
      //                       N   N
      //                       |
      //                       I
      //
      child(0) = generator->getExpGenerator()->convertIntervalToNumeric(
                                                 child(0)->getValueId());
      char str[20];
      strcpy(str, "@A1 / @A2");
      child(0) = generator->getExpGenerator()->createExprTree(str, 0,
                                                              2,
                                                              child(0),
                                                              child(1));
      child(0)->bindNode(generator->getBindWA());
      child(0) = generator->getExpGenerator()->convertNumericToInterval(
                                                 child(0)->getValueId(),
                                                 *result_type);
      strcpy(str, "001"); // to make sure it is not a tinyint
      child(1) = generator->getExpGenerator()->createExprTree(str, CharInfo::ISO88591);
      child(1)->bindNode(generator->getBindWA());
      type_op2 = (NAType *)(&(child(1)->getValueId().getType()));
      if ((result_type->getNominalSize() != type_op2->getNominalSize()) ||
          (type_op2->getFSDatatype() != REC_BIN16_SIGNED)) {
        IntervalType *interval = (IntervalType *) result_type;
        const Int16 DisAmbiguate = 0;
        child(1) = new(generator->wHeap()) Cast(child(1),
                            new(generator->wHeap()) SQLNumeric(generator->wHeap(), TRUE, /* signed */
                                           interval->getTotalPrecision(),
                                           0,
                                           DisAmbiguate, // added for 64bit proj.
                                           interval->supportsSQLnull()));
        child(1)->bindNode(generator->getBindWA());
      }
      break;
    }
    default:
      break;
    }
  } else if (result_type->getTypeQualifier() == NA_DATETIME_TYPE) {
    switch (getOperatorType()) {
    case ITM_PLUS:
    case ITM_MINUS: {
      if ((type_op1->getTypeQualifier() == NA_INTERVAL_TYPE) &&
          (((IntervalType*) type_op1)->getEndField() == REC_DATE_SECOND)) {
        Lng32 sourceScale = ((IntervalType *) type_op1)->getFractionPrecision();
        Lng32 targetScale = ((DatetimeType *) type_op2)->getFractionPrecision();
        child(0) = generator->getExpGenerator()->scaleBy10x(
                                                   child(0)->getValueId(),
                                                   targetScale - sourceScale);
      } else if ((type_op2->getTypeQualifier() == NA_INTERVAL_TYPE) &&
                 (((IntervalType*) type_op2)->getEndField() == REC_DATE_SECOND)) {
        Lng32 targetScale = ((DatetimeType *) type_op1)->getFractionPrecision();
        Lng32 sourceScale = ((IntervalType *) type_op2)->getFractionPrecision();
        child(1) = generator->getExpGenerator()->scaleBy10x(
                                                   child(1)->getValueId(),
                                                   targetScale - sourceScale);
      }

      // Extend the datetime to contain a YEAR field if needed.  The
      // value will need to be extended if it contains a DAY field but
      // does not already contain a YEAR field.  This is necessary
      // since with the introduction of non-standard SQL/MP datetime
      // types, it is possible to have a datetime value which has a
      // DAY field but not a YEAR or not a MONTH field.  In this
      // situation, it is not possible to define a meaningful way to
      // do the operation.  Does the DAY field wrap at 30, 31, 28, or
      // 29.  So to make this operation meaningful, the value is
      // extended to the current timestamp.
      //
      if (type_op1->getTypeQualifier() == NA_DATETIME_TYPE) {
        if(((DatetimeType *) type_op1)->containsField(REC_DATE_DAY) &&
           ! ((DatetimeType *) type_op1)->containsField(REC_DATE_YEAR)) {

          // Need to extend the given datetime value in order to be
          // able to do the operation.  Extend the value out to the
          // YEAR field.
          //
          DatetimeType *extendedType =
            DatetimeType::constructSubtype(type_op1->supportsSQLnull(),
                                           REC_DATE_YEAR,
                                           ((DatetimeType *)type_op1)->getEndField(),
                                           ((DatetimeType *)type_op1)->getFractionPrecision(),
                                           generator->wHeap());

          // Cast the given value to the extended type.
          //
          child(0) = new (generator->wHeap()) Cast(child(0), extendedType);
          child(0)->bindNode(generator->getBindWA());
        }
      } else {
        if(((DatetimeType *) type_op2)->containsField(REC_DATE_DAY) &&
           ! ((DatetimeType *) type_op2)->containsField(REC_DATE_YEAR)) {

          // Need to extend the given datetime value in order to be
          // able to do the operation.  Extend the value out to the
          // YEAR field.
          //
          DatetimeType *extendedType =
            DatetimeType::constructSubtype(type_op2->supportsSQLnull(),
                                           REC_DATE_YEAR,
                                           ((DatetimeType *)type_op2)->getEndField(),
                                           ((DatetimeType *)type_op2)->getFractionPrecision(),
                                           generator->wHeap());

          // Cast the given value to the extended type.
          //
          child(1) = new (generator->wHeap()) Cast(child(1), extendedType);
          child(1)->bindNode(generator->getBindWA());
        }
      }

      break;
    }
    default:
      break;
    }
  }

  // NABoolean convertRoundedDivResult = FALSE;

  // If this arith operation is supported at runtime, then no
  // conversion is needed. Done for result numeric type only.
  if (result_type->getTypeQualifier() == NA_NUMERIC_TYPE)
    {
      child(0) = child(0)->preCodeGen(generator);
      if (! child(0).getPtr())
	return NULL;

      child(1) = child(1)->preCodeGen(generator);
      if (! child(1).getPtr())
	return NULL;

      attr_result = (SimpleType *)
	(ExpGenerator::convertNATypeToAttributes
	 (getValueId().getType(), generator->wHeap()));
      attr_op1 = (SimpleType *)
	(ExpGenerator::convertNATypeToAttributes
	 (child(0)->getValueId().getType(), generator->wHeap()));
      attr_op2 = (SimpleType *)
	(ExpGenerator::convertNATypeToAttributes
	 (child(1)->getValueId().getType(), generator->wHeap()));

      ex_arith_clause temp_clause(getOperatorType(), NULL, NULL, 
				  getRoundingMode(), getDivToDownscale());
      if (temp_clause.isArithSupported(getOperatorType(),
				       attr_op1,
				       attr_op2,
				       attr_result
				       ))
	{
	  markAsPreCodeGenned();

	  return this;
	}
    }

  // if the datatype or lengths of child and this don't match, then
  // conversion is needed.
  type_op1 = (NAType *)(&(child(0)->getValueId().getType()));
  type_op2 = (NAType *)(&(child(1)->getValueId().getType()));
  if ((result_type->getTypeQualifier() != NA_INTERVAL_TYPE) &&
      (result_type->getTypeQualifier() != NA_DATETIME_TYPE) &&
      ((result_type->getFSDatatype() != type_op1->getFSDatatype()) ||
       (result_type->getNominalSize() != type_op1->getNominalSize())))
    {
      // If the result type is not a float, make sure that the following
      // Cast does not scale (for floats we have do do scaling). This is
      // done by using the result type but changing the scale to the scale
      // of the operand
      NAType * new_type = result_type->newCopy(generator->wHeap());
      if ((result_type->getFSDatatype() < REC_MIN_FLOAT) ||
	  (result_type->getFSDatatype() > REC_MAX_FLOAT)) {
	((NumericType *)new_type)->
	  setScale(((NumericType *)type_op1)->getScale());
      };
      child(0) = new(generator->wHeap()) Cast(child(0), new_type,
					      ITM_CAST, FALSE);
    }

  if ((result_type->getTypeQualifier() != NA_INTERVAL_TYPE) &&
      (result_type->getTypeQualifier() != NA_DATETIME_TYPE) &&
      ((result_type->getFSDatatype() != type_op2->getFSDatatype()) ||
       (result_type->getNominalSize() != type_op2->getNominalSize())))
    {
      NAType * new_type = result_type->newCopy(generator->wHeap());
      if ((result_type->getFSDatatype() < REC_MIN_FLOAT) ||
	  (result_type->getFSDatatype() > REC_MAX_FLOAT) ||
           matchScale)
      {
	((NumericType *)new_type)->
	  setScale(((NumericType *)type_op2)->getScale());
      };
      child(1) = new(generator->wHeap()) Cast(child(1), new_type,
					      ITM_CAST, FALSE);
    }

  child(0)->bindNode(generator->getBindWA());
  child(1)->bindNode(generator->getBindWA());

  child(0) = child(0)->preCodeGen(generator);
  if (! child(0).getPtr())
     return NULL;

  child(1) = child(1)->preCodeGen(generator);
  if (! child(1).getPtr())
     return NULL;

  markAsPreCodeGenned();

  return this;
} // BiArith::preCodeGen()

ItemExpr * UnArith::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  return this;
}

ItemExpr * BiLogic::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  ItemExpr *result = this;
  ItemExpr *INlhs = NULL;
  if (CmpCommon::getDefault(OR_PRED_ADD_BLOCK_TO_IN_LIST) == DF_ON 
      && createdFromINlist() && (INlhs=getINlhs())!=NULL)
    {
      // ItmBlockFunction serves like the "C/C++ comma" expression that
      // 1) evaluates its 1st operand, 2nd operand, and
      // 2) returns its 2nd operand as value of that expression.
      // ItmBlockFunction also has the codegen property that
      //   its 1st operand is evaluated (codegen'ed) only once
      //   even if 1st operand occurs multiple times in 2nd operand.
      // So, given "UPPER(n) IN ('a', 'b')" that has been converted to
      //     ItmBlockFunction
      //     /               \
      //    U                 OR
      //                    /    \
      //                  =        =
      //                /   \    /   \
      //               U     a  U     b
      // "UPPER(n)", represented as U, is evaluated once even if 
      // it's used multiple times in the OR expression.

      // Trying to add ItmBlockFunction early in the parser (ie, in
      // sqlparseraux.cpp convertINvaluesToOR() causes a lot of grief
      // especially in cardinality estimation code. So, we resort to
      // doing it late, here in precodegen.
      result = new(generator->wHeap()) ItmBlockFunction(INlhs, result);
      result->synthTypeAndValueId();
      result->markAsPreCodeGenned();
      return result;
    }

  markAsPreCodeGenned();
  return result;
}

ItemExpr * BiRelat::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  // transform multivalue predicates to single-value comparisons.
  ItemExpr * newNode = transformMultiValuePredicate();
  if (newNode)
    {
      #ifdef _DEBUG
      // NAString unp;
      // unparse(unp);
      // cerr << "BiRelat::preCodeGen - " << unp << " needed to be transformed!"
      //    << endl;
      // I don't think we should ever have an untransformed MVP at this stage!
      #endif

      // transformMultiValuePredicate() cannot do synthTypeAndValue()
      // because it is also called from the normalizer in places
      // where it needs to postpone it.
      newNode->synthTypeAndValueId();
      return newNode->preCodeGen(generator);
    }

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  NAType * type_op1 = (NAType *)(&(child(0)->getValueId().getType()));
  NAType * type_op2 = (NAType *)(&(child(1)->getValueId().getType()));

  if ((type_op1->isComplexType()) || (type_op2->isComplexType()))
    {
      // find the 'super' type
      const NAType *result_type = type_op1->synthesizeType(SYNTH_RULE_UNION,
                                                           *type_op1,
                                                           *type_op2,
							   generator->wHeap());
      CMPASSERT(result_type);
      if (result_type->getTypeQualifier() == NA_NUMERIC_TYPE)
	{
	  // match scales
	  child(0) = generator->getExpGenerator()->matchScales(child(0)->getValueId(),
							       *result_type);

	  child(1) = generator->getExpGenerator()->matchScales(child(1)->getValueId(),
							       *result_type);
	}

      type_op1 = (NAType *)(&(child(0)->getValueId().getType()));
      type_op2 = (NAType *)(&(child(1)->getValueId().getType()));

      if ((result_type->getFSDatatype() != type_op1->getFSDatatype()) ||
	  (result_type->getNominalSize() != type_op1->getNominalSize()))
	{
	  child(0) = new(generator->wHeap()) Cast(child(0), result_type);
	}

      if ((result_type->getFSDatatype() != type_op2->getFSDatatype()) ||
	  (result_type->getNominalSize() != type_op2->getNominalSize()))
	{
	  child(1) = new(generator->wHeap()) Cast(child(1), result_type);
	}

      child(0)->bindNode(generator->getBindWA());
      child(1)->bindNode(generator->getBindWA());

      child(0) = child(0)->preCodeGen(generator);
      if (! child(0).getPtr())
	return NULL;

      child(1) = child(1)->preCodeGen(generator);
      if (! child(1).getPtr())
	return NULL;

      markAsPreCodeGenned();

      return this;
    }

  const NAType &type1A =
    child(0)->castToItemExpr()->getValueId().getType();
  const NAType &type2A =
    child(1)->castToItemExpr()->getValueId().getType();

  if ((type1A.getTypeQualifier() == NA_CHARACTER_TYPE) &&
      (type2A.getTypeQualifier() == NA_CHARACTER_TYPE))
    {
      const CharType &cType1A = (CharType&)type1A;
      const CharType &cType2A = (CharType&)type2A;
      
      CharInfo::Collation cType1A_coll = cType1A.getCollation();
      CharInfo::Collation cType2A_coll = cType2A.getCollation();

      //
      // When Implicit Casting And Translation feature is enabled, it is
      // possible for the binder to allow a comparision between an ISO88591-type
      // value and a UCS2-type value to be passed through to the generator.
      // If that happens, we throw in a Translate node at this point.
      //
      CharInfo::CharSet cType1A_CS  = cType1A.getCharSet() ;
      CharInfo::CharSet cType2A_CS = cType2A.getCharSet() ;

      if ( ( cType1A_CS  != cType2A_CS )               &&
           ( cType1A_CS  != CharInfo::UnknownCharSet ) &&
           ( cType2A_CS != CharInfo::UnknownCharSet ) )
      {
         Int32 chld_to_trans = 0;
         if ( cType1A_CS != CharInfo::ISO88591 )
         {
            if ( (cType1A_CS == CharInfo::UNICODE) ) chld_to_trans = 1;
            if ( (cType1A_CS == CharInfo::UTF8) && (cType2A_CS != CharInfo::UNICODE) ) chld_to_trans = 1;
            if ( (cType1A_CS == CharInfo::SJIS) && (cType2A_CS == CharInfo::ISO88591) ) chld_to_trans = 1;
         }
         Int32 tran_type = Translate::UNKNOWN_TRANSLATION;
         if ( chld_to_trans == 0 )
             tran_type = find_translate_type( cType1A_CS, cType2A_CS );
         else
             tran_type = find_translate_type( cType2A_CS, cType1A_CS );

	 ItemExpr * newChild = NULL;
         newChild = new (generator->wHeap()) Translate(child(chld_to_trans), tran_type);
         newChild = newChild->bindNode(generator->getBindWA());
         newChild = newChild->preCodeGen(generator);
         if (! newChild)
            return NULL;
         setChild(chld_to_trans, newChild);
      }
      else if ( cType1A_coll != cType2A_coll &&
                cType1A_CS == CharInfo::ISO88591 &&
                cType1A_CS == cType2A_CS &&
                child(1)->getOperatorType() == ITM_CONSTANT &&
                CollationInfo::isSystemCollation(cType1A_coll))
      {
         ItemExpr * pNewChild2 = NULL;
         NAType * pNewType2 = cType2A.newCopy(generator->wHeap());
         CharType * pNewCType2 = NULL;
         if (pNewType2 != NULL)
           pNewCType2 = (CharType*)pNewType2;
         if (pNewCType2 != NULL)
           pNewCType2->setCollation(cType1A_coll);
         pNewChild2 = new (generator->wHeap()) Cast(child(1), pNewCType2);
         pNewChild2 = pNewChild2->bindNode(generator->getBindWA());
         pNewChild2 = pNewChild2->preCodeGen(generator);
         if (pNewChild2 == NULL)
            return NULL;
         setChild(1, pNewChild2);
      }
      // Regenerate the types...before we continue with rest of code
      type_op1 = (NAType *)(&(child(0)->getValueId().getType()));
      type_op2 = (NAType *)(&(child(1)->getValueId().getType()));

      ItemExpr * pChild1 = child(1)->castToItemExpr();
      const NAType &type1 = pChild1->getValueId().getType();
      const CharType &cType1 = (CharType&)type1;

      ItemExpr * pChild2 = child(1)->castToItemExpr();
      const NAType &type2 = pChild2->getValueId().getType();
      const CharType &cType2 = (CharType&)type2;

      CharInfo::Collation coll1 = cType1.getCollation();
      CharInfo::Collation coll2 = cType2.getCollation();

      CMPASSERT(coll1==coll2);

      if (CollationInfo::isSystemCollation(coll1))
        {
          setCollationEncodeComp(TRUE);
          {

            ItemExpr * newIe1 = child(0);
            ItemExpr * newIe2 = child(1);
	      
            if (! (cType1 == cType2)) 
	      {

		NAType *resultType ;
		Lng32 len = MAXOF(cType1.getMaxLenInBytesOrNAWChars(), cType2.getMaxLenInBytesOrNAWChars());
		Lng32 Prec= MAXOF(cType1.getStrCharLimit(), cType2.getStrCharLimit());
        	     
		if (len != cType1.getMaxLenInBytesOrNAWChars()) 
                  {
                    if (DFS2REC::isAnyVarChar(cType1.getFSDatatype()))
                      {
                        resultType = new (generator->wHeap())
                          SQLVarChar( generator->wHeap(),
                                      CharLenInfo(Prec, len),
                                      cType1.supportsSQLnull(), 
                                      cType1.isUpshifted(),
                                      cType1.isCaseinsensitive(),
                                      cType1.getCharSet(),
                                      cType1.getCollation(),
                                      cType1.getCoercibility()
                                      );
                      }
                    else
                      {
                        resultType = new (generator->wHeap())
                          SQLChar( generator->wHeap(),
                                   CharLenInfo(Prec, len),
                                   cType1.supportsSQLnull(), 
                                   cType1.isUpshifted(),
                                   cType1.isCaseinsensitive(),
                                   FALSE,
                                   cType1.getCharSet(),
                                   cType1.getCollation(),
                                   cType1.getCoercibility()
                                   );
                      }

                    newIe1 = new(generator->wHeap()) Cast(newIe1,resultType);
                  }

		if (len != cType2.getMaxLenInBytesOrNAWChars()) 
                  {
                    if (DFS2REC::isAnyVarChar(cType2.getFSDatatype()))
                      {
                        resultType = new (generator->wHeap())
                          SQLVarChar( generator->wHeap(),
                                      CharLenInfo(Prec, len),
                                      cType2.supportsSQLnull(), 
                                      cType2.isUpshifted(),
                                      cType2.isCaseinsensitive(),
                                      cType2.getCharSet(),
                                      cType2.getCollation(),
                                      cType2.getCoercibility()
                                      );
                      }
                    else
                      {
                        resultType = new (generator->wHeap())
                          SQLChar( generator->wHeap(),
                                   CharLenInfo(Prec, len),
                                   cType2.supportsSQLnull(), 
                                   cType2.isUpshifted(),
                                   cType2.isCaseinsensitive(),
                                   FALSE,
                                   cType2.getCharSet(),
                                   cType2.getCollation(),
                                   cType2.getCoercibility()
                                   );
                      }

                    newIe2 = new(generator->wHeap()) Cast(newIe2,resultType);
                  }
	      }

            ItemExpr * newEncode;
	      
            newEncode = 
              new(generator->wHeap()) 
              CompEncode(newIe1,FALSE, -1, CollationInfo::Compare);

            newEncode->bindNode(generator->getBindWA());
            newEncode = newEncode->preCodeGen(generator);
            if (!newEncode)
              return NULL;
            setChild(0, newEncode);

            newEncode = 
              new(generator->wHeap()) 
              CompEncode(newIe2, FALSE, -1,CollationInfo::Compare);

            newEncode->bindNode(generator->getBindWA());
            newEncode = newEncode->preCodeGen(generator);
            if (!newEncode)
              return NULL;
            setChild(1, newEncode);
          }

        }
      else
        {
          // update both operands if case insensitive comparions
          // are to be done.

          NABoolean doCIcomp =
            ((cType1.isCaseinsensitive()) && (cType2.isCaseinsensitive()));

          ItemExpr * newChild = NULL;
          if ((doCIcomp) &&
              (NOT cType1.isUpshifted()))
            {
              newChild = child(0);

              // Add UPPER except if it is NULL constant value.
              if (newChild->getOperatorType() != ITM_CONSTANT || !((ConstValue *)newChild)->isNull())
                newChild = new (generator->wHeap()) Upper(newChild);

              newChild = newChild->bindNode(generator->getBindWA());
              if (! newChild || generator->getBindWA()->errStatus())
                return NULL;

              newChild = newChild->preCodeGen(generator);
              if (! newChild)
                return NULL;

              setChild(0, newChild);
            }

          if ((doCIcomp) &&
              (NOT cType2.isUpshifted()))
            {
              newChild = child(1);

              // Add UPPER except if it is NULL constant value.
              if (newChild->getOperatorType() != ITM_CONSTANT || !((ConstValue *)newChild)->isNull())
                newChild = new (generator->wHeap()) Upper(newChild);

              newChild = newChild->bindNode(generator->getBindWA());
              if (! newChild || generator->getBindWA()->errStatus())
                return NULL;

              newChild = newChild->preCodeGen(generator);
              if (! newChild)
                return NULL;

              setChild(1, newChild);
            }
        }
    }

  // following is for simple types.
  const NAType &type1B =
    child(0)->castToItemExpr()->getValueId().getType();
  const NAType &type2B =
    child(1)->castToItemExpr()->getValueId().getType();

  SimpleType * attr_op1 = (SimpleType *)
    (ExpGenerator::convertNATypeToAttributes(type1B, generator->wHeap()));
  SimpleType * attr_op2 = (SimpleType *)
    (ExpGenerator::convertNATypeToAttributes(type2B, generator->wHeap()));

  ex_comp_clause temp_clause;

  temp_clause.setInstruction(getOperatorType(),
			     attr_op1,
			     attr_op2
			     );

  if ((temp_clause.getInstruction() == COMP_NOT_SUPPORTED) &&
      (type1B.getTypeQualifier() == NA_NUMERIC_TYPE) &&
      (type2B.getTypeQualifier() == NA_NUMERIC_TYPE))
    {
      const NumericType &numOp1 = (NumericType&)type1B;
      const NumericType &numOp2 = (NumericType&)type2B;

      if ((numOp1.isExact() && numOp2.isExact()) &&
          ((numOp1.getFSDatatype() == REC_BIN64_UNSIGNED) ||
           (numOp2.getFSDatatype() == REC_BIN64_UNSIGNED)))
        {
          if (numOp1.getFSDatatype() == REC_BIN64_UNSIGNED)
            {
              // add a Cast node to convert op2 to sqllargeint.
              ItemExpr * newOp2 =
                new (generator->wHeap())
                Cast(child(1),
                     new (generator->wHeap())
                     SQLLargeInt(generator->wHeap(), numOp2.isSigned(),
                                 numOp2.supportsSQLnull()));
              
              newOp2 = newOp2->bindNode(generator->getBindWA());
              newOp2 = newOp2->preCodeGen(generator);
              if (! newOp2)
                return NULL;
              
              setChild(1, newOp2);

              attr_op2 = (SimpleType *)
                (ExpGenerator::convertNATypeToAttributes(
                     newOp2->getValueId().getType(), generator->wHeap()));
            }
          else 
           {
              // add a Cast node to convert op1 to sqllargeint.
              ItemExpr * newOp1 =
                new (generator->wHeap())
                Cast(child(0),
                     new (generator->wHeap())
                     SQLLargeInt(generator->wHeap(), numOp1.isSigned(),
                                 numOp1.supportsSQLnull()));
              
              newOp1 = newOp1->bindNode(generator->getBindWA());
              newOp1 = newOp1->preCodeGen(generator);
              if (! newOp1)
                return NULL;
              
              setChild(0, newOp1);

              attr_op1 = (SimpleType *)
                (ExpGenerator::convertNATypeToAttributes(
                     newOp1->getValueId().getType(), generator->wHeap()));
           }

          temp_clause.setInstruction(getOperatorType(),
                                     attr_op1,
                                     attr_op2
                                     );
        } // convert
    }
  
  if (temp_clause.getInstruction() != COMP_NOT_SUPPORTED)
    {
      NABoolean doConstFolding = FALSE;
      if ((temp_clause.getInstruction() == ASCII_COMP) &&
	  (CmpCommon::getDefault(CONSTANT_FOLDING) == DF_ON))
	{
	  if (((child(0)->getOperatorType() == ITM_CONSTANT) &&
	       (child(1)->getOperatorType() != ITM_CONSTANT)) ||
	      ((child(1)->getOperatorType() == ITM_CONSTANT) &&
	       (child(0)->getOperatorType() != ITM_CONSTANT)) &&
	      (type_op1->getFSDatatype() == REC_BYTE_F_ASCII) &&
	      (type_op2->getFSDatatype() == REC_BYTE_F_ASCII))
	    {
	      if (((child(0)->getOperatorType() == ITM_CONSTANT) &&
		   (type_op1->getNominalSize() < type_op2->getNominalSize())) ||
		  ((child(1)->getOperatorType() == ITM_CONSTANT) &&
		   (type_op2->getNominalSize() < type_op1->getNominalSize())))
		{
		  doConstFolding = TRUE;
		}
	    }
	}

      if (NOT doConstFolding)
	{
	  markAsPreCodeGenned();

	  return this;
	}
    }

  // conversion needed before comparison could be done.

  // find the 'super' type
  UInt32 flags =
            ((CmpCommon::getDefault(LIMIT_MAX_NUMERIC_PRECISION) == DF_ON)
            ? NAType::LIMIT_MAX_NUMERIC_PRECISION : 0);
  if (CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON)
    {
      flags |= NAType::ALLOW_INCOMP_OPER;
    }
  
  const NAType *result_type = type_op1->synthesizeType(SYNTH_RULE_UNION,
                                                       *type_op1,
                                                       *type_op2,
						       generator->wHeap(),
                                                       &flags);
  CMPASSERT(result_type);
  if (result_type->getTypeQualifier() == NA_NUMERIC_TYPE)
    {
      // match scales
      child(0) = generator->getExpGenerator()->matchScales(child(0)->getValueId(),
							   *result_type);
      child(1) = generator->getExpGenerator()->matchScales(child(1)->getValueId(),
							   *result_type);
    }
  else if (result_type->getTypeQualifier() == NA_DATETIME_TYPE) {
    Lng32 fp1 = ((DatetimeType *) type_op1)->getFractionPrecision();
    Lng32 fp2 = ((DatetimeType *) type_op2)->getFractionPrecision();
    Lng32 fpResult = ((DatetimeType *) result_type)->getFractionPrecision();
    if (fp1 != fpResult) {
      child(0) = new(generator->wHeap()) Cast(child(0), result_type,
					      ITM_CAST, FALSE);
      child(0)->bindNode(generator->getBindWA());
    }
    if (fp2 != fpResult) {
      child(1) = new(generator->wHeap()) Cast(child(1), result_type,
					      ITM_CAST, FALSE);
      child(1)->bindNode(generator->getBindWA());
    }
  } else if (result_type->getTypeQualifier() == NA_INTERVAL_TYPE) {
    child(0) = generator->getExpGenerator()->matchIntervalEndFields(
                                               child(0)->getValueId(),
                                               *result_type);
    child(1) = generator->getExpGenerator()->matchIntervalEndFields(
                                               child(1)->getValueId(),
                                               *result_type);
    child(0) = generator->getExpGenerator()->matchScales(
                                               child(0)->getValueId(),
                                               *result_type);
    child(1) = generator->getExpGenerator()->matchScales(
                                               child(1)->getValueId(),
                                               *result_type);
    type_op1 = (NAType *)(&(child(0)->getValueId().getType()));
    type_op2 = (NAType *)(&(child(1)->getValueId().getType()));
    if (result_type->getNominalSize() != type_op1->getNominalSize()) {
      child(0) = new(generator->wHeap()) Cast(child(0), result_type,
					      ITM_CAST, FALSE);
      child(0)->bindNode(generator->getBindWA());
    }
    if (result_type->getNominalSize() != type_op2->getNominalSize()) {
      child(1) = new(generator->wHeap()) Cast(child(1), result_type,
					      ITM_CAST, FALSE);
      child(1)->bindNode(generator->getBindWA());
    }
  }

  // if the datatype or lengths of child and this don't match, then
  // conversion is needed.
  type_op1 = (NAType *)(&(child(0)->getValueId().getType()));
  type_op2 = (NAType *)(&(child(1)->getValueId().getType()));
  if ((result_type->getTypeQualifier() != NA_INTERVAL_TYPE) &&
      ((result_type->getFSDatatype() != type_op1->getFSDatatype()) ||
       (result_type->getNominalSize() != type_op1->getNominalSize())))
    {
      child(0) = new(generator->wHeap()) Cast(child(0), result_type,
					      ITM_CAST, FALSE);
    }

  if ((result_type->getTypeQualifier() != NA_INTERVAL_TYPE) &&
      ((result_type->getFSDatatype() != type_op2->getFSDatatype()) ||
       (result_type->getNominalSize() != type_op2->getNominalSize())))
    {
      child(1) = new(generator->wHeap()) Cast(child(1), result_type,
					      ITM_CAST, FALSE);
    }

  // bind/type propagate the new nodes
  child(0)->bindNode(generator->getBindWA());
  child(1)->bindNode(generator->getBindWA());

  child(0) = child(0)->preCodeGen(generator);
  if (! child(0).getPtr())
     return NULL;

  child(1) = child(1)->preCodeGen(generator);
  if (! child(1).getPtr())
     return NULL;

  ItemExpr *outExpr = NULL;
  Lng32 rc = generator->getExpGenerator()->foldConstants(child(0), &outExpr);
  if ((rc == 0) &&
      (outExpr))
    {
      child(0) = outExpr->preCodeGen(generator);
    }

  rc = generator->getExpGenerator()->foldConstants(child(1), &outExpr);
  if ((rc == 0) &&
      (outExpr))
    {
      child(1) = outExpr->preCodeGen(generator);
    }

  markAsPreCodeGenned();

  return this;
} // BiRelat::preCodeGen()

ItemExpr * Assign::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  child(1) = generator->getExpGenerator()->matchIntervalEndFields(
                                             child(1)->getValueId(),
                                             getValueId().getType());
  child(1) = generator->getExpGenerator()->matchScales(child(1)->getValueId(),
					     getValueId().getType());
  child(1)->bindNode(generator->getBindWA());
  child(1) =  child(1)->preCodeGen(generator);
  if (! child(1).getPtr())
    return NULL;

  markAsPreCodeGenned();

  return this;
} // Assign::preCodeGen()

ItemExpr * BaseColumn::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  ItemExpr * i = convertExternalType(generator);
  if (i == NULL)
    return NULL;

  return i;
}

ItemExpr * BitOperFunc::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (getOperatorType() == ITM_BITEXTRACT)
    {
      // convert 2nd and 3rd operands to Int32 signed.
      for (Int32 i = 1; i < getArity(); i++)
	{
	  const NAType &typ = child(i)->getValueId().getType();
       
	  if (typ.getFSDatatype() != REC_BIN32_UNSIGNED)
	    {
	      ItemExpr * newChild =
		new (generator->wHeap()) 
		Cast(child(i), 
		     new (generator->wHeap()) SQLInt(generator->wHeap(), FALSE,
						 typ.supportsSQLnullLogical()));
	      setChild(i, newChild);	

	      child(i)->bindNode(generator->getBindWA());
	      
	      child(i) = child(i)->preCodeGen(generator);
	      if (! child(i).getPtr())
		return NULL;  
	    } // if
	} // for
    }
  else
    {
      for (Int32 i = 0; i < getArity(); i++)
	{
	  const NAType &typ = child(i)->getValueId().getType();
	  
	  if (NOT (getValueId().getType() == typ))
	    {
	      NAType *resultType = 
		getValueId().getType().newCopy(generator->wHeap());
	      
	      ItemExpr * newChild =
		new (generator->wHeap()) Cast(child(i), resultType);
	      setChild(i, newChild);
	    }
	  
	  child(i)->bindNode(generator->getBindWA());
	  
	  child(i) = child(i)->preCodeGen(generator);
	  if (! child(i).getPtr())
	    return NULL;
	}
    }

  markAsPreCodeGenned();
  return this;
} // BitOperFunc::preCodeGen()

ItemExpr * Cast::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  child(0)->bindNode(generator->getBindWA());
  child(0) = child(0)->preCodeGen(generator);

  if (! child(0).getPtr())
    return NULL;

  // if a special cast node, see if my child's data attributes are
  // the same as my data attributes. If they are, return pointer to
  // my child.
  if ((matchChildType()) &&
      (child(0)->getValueId().getType() == getValueId().getType()))
    {
      markAsPreCodeGenned();
      return child(0);
    }

  NABuiltInTypeEnum sourceTypeQual =
    child(0)->getValueId().getType().getTypeQualifier();
  NABuiltInTypeEnum targetTypeQual =
    getValueId().getType().getTypeQualifier();

  // If this is a NARROW operation, but it is not possible to result
  // in an error, no reason to use NARROW.  Convert the NARROW to the
  // equivalent CAST.
  if (getOperatorType() == ITM_NARROW)
    {
      const NAType * sourceType = &(child(0)->getValueId().getType());

      const NAType * targetType = &(getValueId().getType());

      if (!sourceType->errorsCanOccur(*targetType))
        {

          ItemExpr *c = new(generator->wHeap()) Cast(child(0), targetType);
          c->bindNode(generator->getBindWA());
          return c->preCodeGen(generator);
        
        }
    }

  if (generator->getExpGenerator()->handleUnsupportedCast(this))
    return NULL;

  const NAType &srcNAType = child(0)->getValueId().getType();
  const NAType &tgtNAType = getValueId().getType();
  short srcFsType = srcNAType.getFSDatatype();
  short tgtFsType = tgtNAType.getFSDatatype();

  if ((sourceTypeQual == NA_NUMERIC_TYPE) &&
      (targetTypeQual == NA_DATETIME_TYPE))
    {
      // binder has already verified that this is a valid conversion
      // in special1 mode.
      NumericType &sourceType =
      (NumericType &)(child(0)->getValueId().getType());
      DatetimeType &targetType =
	(DatetimeType &)(getValueId().getType());

      if (sourceType.getFSDatatype() != REC_BIN64_SIGNED)
	{
	  // doing a numeric to date conversion
	  // convert source to largeint.
	  ItemExpr * newChild =
	    new (generator->wHeap())
	    Cast(child(0),
		 new (generator->wHeap())
		 SQLLargeInt(generator->wHeap(), TRUE,
			     child(0)->castToItemExpr()->
			     getValueId().getType().supportsSQLnull()));
	  newChild = newChild->bindNode(generator->getBindWA());
	  newChild = newChild->preCodeGen(generator);
	  if (! newChild)
	    return NULL;

	  setChild(0, newChild);
	}
    }

  if ((sourceTypeQual == NA_DATETIME_TYPE) &&
      (targetTypeQual == NA_NUMERIC_TYPE))
    {
      // binder has already verified that this is a valid conversion
      // in special1 mode.
      DatetimeType &sourceType =
      (DatetimeType &)(child(0)->getValueId().getType());
      NumericType &targetType =
	(NumericType &)(getValueId().getType());

      if (targetType.getFSDatatype() != REC_BIN64_SIGNED)
	{
	  // doing a date to numeric conversion.
	  // convert source to largeint.
	  ItemExpr * newChild =
	    new (generator->wHeap())
	    Cast(child(0),
		 new (generator->wHeap())
		 SQLLargeInt(generator->wHeap(), TRUE,
			     child(0)->castToItemExpr()->
			     getValueId().getType().supportsSQLnull()));
	  newChild = newChild->bindNode(generator->getBindWA());
	  newChild = newChild->preCodeGen(generator);
	  if (! newChild)
	    return NULL;

	  setChild(0, newChild);
	}

    } // numeric to date conversion

  if ((CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON) &&
      (sourceTypeQual == NA_NUMERIC_TYPE) &&
      (targetTypeQual == NA_INTERVAL_TYPE))
    { 
      NumericType &sourceType =
	(NumericType &)(child(0)->getValueId().getType());

      if (NOT sourceType.isExact())
        {
	  // doing a float numeric to interval conversion.
	  // convert source to corresponding exact numeric (largeint).
          // This is the largest interval type that is supported.
	  ItemExpr * newChild =
	    new (generator->wHeap())
	    Cast(child(0),
		 new (generator->wHeap())
		 SQLLargeInt(generator->wHeap(), TRUE,
                             child(0)->castToItemExpr()->
                             getValueId().getType().supportsSQLnull()));
	  newChild = newChild->bindNode(generator->getBindWA());
	  newChild = newChild->preCodeGen(generator);
	  if (! newChild)
	    return NULL;
          
	  setChild(0, newChild);
	}
      
    } // numeric to date conversion

  if ((sourceTypeQual == NA_DATETIME_TYPE) &&
      (targetTypeQual == NA_DATETIME_TYPE)) {
    DatetimeType &sourceType =
      (DatetimeType &)(child(0)->getValueId().getType());
    DatetimeType &targetType =
      (DatetimeType &)(getValueId().getType());

    if (targetType.getStartField() < sourceType.getStartField()) {
      // Must provide some fields from the current time stamp
      //

      // The following code generates the current timestamp as a
      // string and extracts the needed leading fields and appends to
      // this the given value (child(0)) as a string. The result is a
      // string which contains the given datetime value extended to
      // the YEAR field with the current timestamp.
      //
      // Buffer to hold new expression string.
      //
      char str[200];

      // Offset (in bytes) from the start of the current timestamp
      // (represented as a char. string) to the first field needed in
      // the extension.
      //
      // - Subtract 1 from the start field to make the value zero based.
      //
      // - Each field has a least 3 bytes (2 for the value and 1 for the
      //   delimiter)
      //
      // - Add 1, since the substring function is 1 based.
      //
      Int32 leadFieldsOffset = ((targetType.getStartField() - 1) * 3) + 1;

      // - Add 2 extra for the year field if it is being skiped over
      //   since it has 4 bytes of value.
      //
      if (leadFieldsOffset > 1)
        leadFieldsOffset += 2;

      // Size (in bytes) of the leading fields represented as a
      // character string taken from the current timestamp
      //
      // - Subtract 1 from the start field to make the value zero based.
      //
      // - Each field has a least 3 bytes (2 for the value and 1 for the
      //   delimiter)
      //
      // - Add 2 extra for the year field (which will always be one of
      //   the extended fields) since it has 4 bytes of value.
      //
      // - Subtract the leadFieldsOffset ( - 1 to make it zero based).
      //
      Int32 leadFieldsSize = ((((sourceType.getStartField() - 1) * 3) + 2)
                            - (leadFieldsOffset - 1));

      // Size (in bytes) of the source value represented as a
      // character string.
      //
      Int32 sourceFieldsSize = sourceType.getDisplayLength();

      // Construct an expression (string) to concatinate the given
      // value with the required fields from the current timestamp as
      // a string, then cast this string as a datetime value, that can
      // be cast to the desired result.
      //
      // Example :
      //
      // cast(DATETIME 'dd hh:mm:ss' DAY TO SECOND as DATETIME MONTH to MINUTE)
      //
      // current timestamp (as string)       | "YYYY-MM-DD HH:MM:SS.FFFFFF"
      //                                     |
      // leadFieldsOffset = ((2-1)*3)+1 +2 = |  --6--^
      //                                     |
      // leadFieldsSize = (((3-1)*3)+2) - 5 =|       ^3^
      //                                     |
      // result of substring(cts from 1 to 8)|      "MM-"
      //                                     |
      // value to be extended (as string)    |         "dd hh:mm:ss"
      //                                     |
      // result of string concat. (as string)|      "MM-dd hh:mm:ss"
      //                                     |
      // Cast to a datetime MONTH TO SECOND  |    Mdhms
      //                                     |
      // Original (this) cast to result      |    Mdhm
      //
      str_sprintf(str,
                  "CAST((SUBSTRING(CAST(CURRENT AS CHAR(19)) "
                  "FROM %d FOR %d) || CAST(@A1 AS CHAR(%d))) "
                  "AS DATETIME %s TO %s)",
                  leadFieldsOffset,
                  leadFieldsSize,
                  sourceFieldsSize,
                  targetType.getFieldName(targetType.getStartField()),
                  ((sourceType.getEndField() == REC_DATE_SECOND)
                   ? "FRACTION(6)"
                   : sourceType.getFieldName(sourceType.getEndField())));

      GenAssert(str_len(str) < 199,"Internal Datetime Error Cast::preCodeGen");

      ItemExpr * newExpr =
        generator->getExpGenerator()->createExprTree(str, 0,
                                                     1, child(0));
      newExpr->bindNode(generator->getBindWA());
      child(0) = newExpr->preCodeGen(generator);

    }

  }

  // Call matchScales only if both datatypes aren't intervals.
  // (We make the exception for intervals because Cast is able
  // to match the scales of intervals itself.)
  // Also, we suppress the call to matchScales() for a narrow.
  // This is because narrow will handle the scaling differently.
  // Conversions from float to bignum are also not scaled here. Scaling
  // is done in BigNum::castFrom method.
  if (NOT ((getOperatorType() == ITM_NARROW) ||
	   ((sourceTypeQual == NA_INTERVAL_TYPE) &&
	    (targetTypeQual == NA_INTERVAL_TYPE)) ||
	   ((DFS2REC::isFloat(srcFsType)) &&
	    (DFS2REC::isBigNum(tgtFsType)))))
    {
      child(0) = generator->getExpGenerator()->matchScales(
					    child(0)->getValueId(),
				            getValueId().getType());
      child(0) = child(0)->preCodeGen(generator);
      if (! child(0).getPtr())
	return NULL;
    }

  //  For a numeric NARROW, check if scaling is needed.
  if (targetTypeQual == NA_NUMERIC_TYPE &&
      getOperatorType() == ITM_NARROW)
    {
      GenAssert(sourceTypeQual == NA_NUMERIC_TYPE,
        "source type and target type incompatible in NARROW");

      const NumericType * sourceNumType
	= (const NumericType *)(&child(0)->getValueId().getType());
      const NumericType * targetNumType
	= (const NumericType *)(&getValueId().getType());

      if (sourceNumType->getScale() != targetNumType->getScale())
        {
          // We need to scale the value.  We don't want to use the
	  // usual scaling method of simply multiplying or dividing
	  // the result because we need to capture truncations
	  // and overflows at run time.  The Narrow operator supports
	  // scaling for the BigNum-to-any-numeric type case.
	  // Therefore, we first cast the value to BigNum,
	  // then narrow it down.
	  // Soln 10-041105-1519
          // Dont introduce the CAST operator if the target is already a BigNum
          // because NARROW does not support scaling for the BigNum-to-BigNum
          // case. Use the usual scaling method instead.
          if (targetNumType->isBigNum())
          {
            child(0) = generator->getExpGenerator()->matchScales(
              child(0)->getValueId(), *targetNumType);
          }
          else
          {
	    Lng32 intermediatePrecision = sourceNumType->getPrecision();
	    Lng32 intermediateScale = sourceNumType->getScale();

	    // SQLBigNum takes decimal precision, so if the source
	    // has binary precision, we need to adjust.

	    if (sourceNumType->binaryPrecision())
	      {

		// Can fit three binary digits in the space of one
		// decimal digit. The '+5' in the precision calculation
		// allows for an extra digit before and after the
		// radix point.

		intermediatePrecision = (intermediatePrecision+5)/3;
	      }

	    // If we need to cast an approximate, increase the length
	    // and scale so that the number can be represented now that
	    // it won't have an exponent.

	    // In each of the cases below, the formula used to calculate
	    // precision is:
	    //
	    // intermediatePrecision = 2 * <max exponent>
	    //    + <# significant digits in mantissa> + 1
	    //
	    // We use 2 * <max exponent> to take into account the
	    // maximum positive exponent as well as the maximum
	    // negative exponent.
	    //
	    // The formula used to calculate scale is:
	    //
	    // intermediateScale = <max exponent> +
	    //   <# significant digits in mantissa> - 1
	    //
	    // Here the exponent and digits are understood to be decimal,
	    // not binary.
	    //
	    // For the various kinds of floats we have:
	    //
	    // Kind         Max exponent   Decimal digits in Mantissa
	    // -----------  ------------   --------------------------
	    // IEEE 32 bit            38        7
	    // IEEE 64 bit           308       17

	    if (sourceNumType->getFSDatatype() == REC_IEEE_FLOAT32)
	      {
		intermediatePrecision = 84;  // (2 x 38) + 7 + 1 = 84
		intermediateScale = 44;   // 38 + 7 - 1 = 44
	      }

	    else if (sourceNumType->getFSDatatype() == REC_IEEE_FLOAT64)
	      {
		intermediatePrecision = 634; // (2 x 308) + 17 + 1 = 634
		intermediateScale = 324;  // 308 + 17 - 1 = 324
	      }

	    NAType * intermediateType =
	      new(generator->wHeap())
		SQLBigNum(generator->wHeap(), intermediatePrecision,
			  intermediateScale,
			  (sourceNumType->isBigNum() &&
			   ((SQLBigNum*)sourceNumType)->isARealBigNum()),
			  TRUE, // make it signed
			  sourceNumType->supportsSQLnull());

	    child(0) = new(generator->wHeap()) Cast(child(0),intermediateType);

	    child(0)->bindNode(generator->getBindWA());

            if (generator->getExpGenerator()->handleUnsupportedCast((Cast*)child(0)->castToItemExpr()))
              return NULL;

	    // To suppress insertion of multiplying/dividing, mark Cast as
	    // already pre-code-genned.

	    child(0)->markAsPreCodeGenned();
	  }
	}
    }

 
  if ((sourceTypeQual == NA_CHARACTER_TYPE) &&
      ((tgtFsType == REC_BLOB) ||
       (tgtFsType == REC_CLOB)))
    {
      LOBconvertHandle * lc = new(generator->wHeap())
	LOBconvertHandle(child(0), LOBoper::LOB_);

      lc->bindNode(generator->getBindWA());
      lc->preCodeGen(generator);
      
      child(0) = lc;
    }

  if (getArity() > 1)
    {
      child(1)->bindNode(generator->getBindWA());
      child(1) = child(1)->preCodeGen(generator);
      if (! child(1).getPtr())
	return NULL;
    }

  ItemExpr *result = this;

  markAsPreCodeGenned();
  return result;
} // Cast::preCodeGen()

ItemExpr * CharFunc::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  const NAType &typ1 = child(0)->getValueId().getType();

  // Insert a cast node to convert child to an INT.
  child(0) = new (generator->wHeap())
    Cast(child(0), new (generator->wHeap()) SQLInt(generator->wHeap(), FALSE,
						   typ1.supportsSQLnullLogical()));

  child(0)->bindNode(generator->getBindWA());

  child(0) = child(0)->preCodeGen(generator);
  if (! child(0).getPtr())
    return NULL;

  markAsPreCodeGenned();
  return this;
} // CharFunc::preCodeGen()

ItemExpr * CompEncode::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  // during key encode expr generation, no need to convert external
  // column types(like tandem floats) to their internal
  // equivalent(ieee floats). Avoid doing preCodeGen in these cases.
  // Do this only for child leaf nodes (columns, hostvar, params, literals).
  //
  if (NOT (child(0)->getValueId().getType().isExternalType() &&
	   child(0)->getArity() == 0)) {
    child(0) = child(0)->preCodeGen(generator);
  }

  markAsPreCodeGenned();

  return this;
} // CompEncode::preCodeGen()

ItemExpr * CompDecode::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  return CompEncode::preCodeGen(generator);

} // CompDecode::preCodeGen()

ItemExpr * Convert::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  // Since this CONVERT will convert its child to the original
  // ExternalType, no need to ask it to first be cast to an internal
  // type.  So, do not call precodegen in these cases.
  // Do this only for child leaf nodes (columns, hostvar, params, literals).
  //
  if (NOT (child(0)->getValueId().getType().isExternalType() &&
	   child(0)->getArity() == 0)) {
    child(0) = child(0)->preCodeGen(generator);
  }

  markAsPreCodeGenned();

  return this;
} // Convert::preCodeGen()

ItemExpr * ConvertTimestamp::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;
  //
  // If the operand is not a largeint with a scale of 0, convert it to one.
  //
  NumericType *numeric = (NumericType *)(&(child(0)->getValueId().getType()));
  if ((numeric->getFSDatatype() != REC_BIN64_SIGNED) ||
      (numeric->getScale() != 0)) {
    child(0) = new(generator->wHeap())
                 Cast(child(0),
                      new(generator->wHeap())
                        SQLLargeInt(generator->wHeap(), TRUE, numeric->supportsSQLnull()));
    child(0)->bindNode(generator->getBindWA());
  }
  child(0) = child(0)->preCodeGen(generator);
  if (! child(0).getPtr())
    return NULL;

  markAsPreCodeGenned();
  return this;
} // ConvertTimestamp::preCodeGen()

ItemExpr * Extract::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;
  //
  // If the operand is an interval and the extract field is not the end field,
  // convert the interval to the units of the extract field.
  // Set the dataconversionerror param to Cast so conversion error
  // (truncation) could be ignored at runtime.
  //
  NAType * type_op1 = (NAType *)(&(child(0)->getValueId().getType()));
  if ((type_op1->getTypeQualifier() == NA_INTERVAL_TYPE) &&
      (getExtractField() < ((IntervalType *) type_op1)->getEndField())) {
    IntervalType *interval = (IntervalType *) type_op1;
    ItemExpr *dataConvError = new(generator->wHeap()) ConstValue(1234567890);
    child(0) = new(generator->wHeap())
                 Cast(child(0), dataConvError,
                      new(generator->wHeap())
                        SQLInterval(generator->wHeap(), interval->supportsSQLnull(),
                                    interval->getStartField(),
                                    interval->getLeadingPrecision(),
                                    getExtractField()),
		      ITM_NARROW);
    child(0)->bindNode(generator->getBindWA());
  }
  child(0) = child(0)->preCodeGen(generator);
  if (! child(0).getPtr())
    return NULL;

  markAsPreCodeGenned();
  return this;
} // Extract::preCodeGen()

ItemExpr * Format::preCodeGen(Generator * generator)
{
  return BuiltinFunction::preCodeGen(generator);
}

ItemExpr * JulianTimestamp::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;
  //
  // If the operand is not a timestamp with a fractional precision of 6,
  // convert it to one.
  //
  DatetimeType *dt = (DatetimeType *)(&(child(0)->getValueId().getType()));
  if ((dt->getSubtype() != DatetimeType::SUBTYPE_SQLTimestamp) ||
      (dt->getFractionPrecision() != 6)) {
    child(0) = new(generator->wHeap())
                 Cast(child(0),
                      new(generator->wHeap())
                        SQLTimestamp(generator->wHeap(), dt->supportsSQLnull(), 6));
    child(0)->bindNode(generator->getBindWA());
  }
  child(0) = child(0)->preCodeGen(generator);
  if (! child(0).getPtr())
    return NULL;
  markAsPreCodeGenned();
  return this;
} // JulianTimestamp::preCodeGen()

ItemExpr * Hash::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return getReplacementExpr();

  ItemExpr *result = this;

  // ---------------------------------------------------------------------
  // In the optimizer, a hash function accepts a comma-separated list
  // of columns. In the executor, replace this with the HashComb of the hash
  // functions of the individual list elements. NOTE: once error handling
  // is in place we need to make sure that no errors are generated from
  // this.
  // ---------------------------------------------------------------------
  if (child(0)->getOperatorType() == ITM_ITEM_LIST)
    {
      // child is a multi-valued expression, transform into multiple
      // hash expressions
      ExprValueId treePtr = child(0);

      ItemExprTreeAsList hashValues(&treePtr,
				    ITM_ITEM_LIST,
				    LEFT_LINEAR_TREE);

      // this expression becomes the hash operator for the first
      // hash value
      child(0) = hashValues[0];

      const NAType &childType = child(0)->getValueId().getType();

      if(childType.getTypeQualifier() == NA_CHARACTER_TYPE) {

	const CharType &chType = (CharType&)childType;
	CharInfo::Collation coll = chType.getCollation();
	
	if (CollationInfo::isSystemCollation(coll))
	    {
	      child(0) = new(generator->wHeap()) 
		CompEncode(child(0),FALSE, -1, CollationInfo::Compare);
	      child(0) = child(0)->bindNode(generator->getBindWA());
	    }
	  else
	    {
	      //--------------------------
	      if ((chType.isCaseinsensitive()) &&
		  (NOT casesensitiveHash()) &&
		  (NOT chType.isUpshifted())) {
		child(0) = new (generator->wHeap()) Upper(child(0));
		child(0) = child(0)->bindNode(generator->getBindWA());
	      }
	    }
	  }

      // add hash expressions for all other hash values and HashComb
      // them together
      CollIndex nc = hashValues.entries();
      for (CollIndex i = 1; i < nc; i++)
	{
          ItemExpr *hi = hashValues[i];

	  const NAType &childType = hi->getValueId().getType();

	  if(childType.getTypeQualifier() == NA_CHARACTER_TYPE) {

	    const CharType &chType = (CharType&)childType;
	    CharInfo::Collation coll = chType.getCollation();
	    
            if (CollationInfo::isSystemCollation(coll))
	      {
		hi = new(generator->wHeap()) 
		    CompEncode(hi,FALSE, -1, CollationInfo::Compare);

		hi = hi->bindNode(generator->getBindWA());
	      }
	      else
	      {
		//-----------------------------
		if ((chType.isCaseinsensitive()) &&
		    (NOT casesensitiveHash()) &&
		    (NOT chType.isUpshifted())) {
		  hi = new (generator->wHeap()) Upper(hi);
		  hi = hi->bindNode(generator->getBindWA());
		}
		  //-----------------------
	      }
	  }

	  ItemExpr *hv = new(generator->wHeap()) Hash(hi);
	  result = new(generator->wHeap()) HashComb(result,hv);
	}

      result->bindNode(generator->getBindWA());
    } else {
      const NAType &childType = child(0)->getValueId().getType();

      if(childType.getTypeQualifier() == NA_CHARACTER_TYPE) {

	const CharType &chType = (CharType&)childType;
	CharInfo::Collation coll = chType.getCollation();
	
        if (CollationInfo::isSystemCollation(coll))
	  {
	    child(0) = new (generator->wHeap()) 
	      CompEncode(child(0), FALSE, -1, CollationInfo::Compare);
	    
	    child(0) = child(0)->bindNode(generator->getBindWA());
          }
	  else
	  {
	    if ((chType.isCaseinsensitive()) &&
		(NOT casesensitiveHash()) &&
		(NOT chType.isUpshifted())) {
	      child(0) = new (generator->wHeap()) Upper(child(0));
	      child(0) = child(0)->bindNode(generator->getBindWA());
	    }
          }

      }
    }

  // do generic tasks for pre-code generation (e.g. recurse to the children)
  setReplacementExpr(result->ItemExpr::preCodeGen(generator));
  markAsPreCodeGenned();
  return getReplacementExpr();
} // Hash::preCodeGen()

ItemExpr * HiveHash::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return getReplacementExpr();

  ItemExpr *result = this;

  // ---------------------------------------------------------------------
  // In the optimizer, a hash function accepts a comma-separated list
  // of columns. In the executor, replace this with the HashComb of the hash
  // functions of the individual list elements. NOTE: once error handling
  // is in place we need to make sure that no errors are generated from
  // this.
  // ---------------------------------------------------------------------
  if (child(0)->getOperatorType() == ITM_ITEM_LIST)
    {
      // child is a multi-valued expression, transform into multiple
      // hash expressions
      ExprValueId treePtr = child(0);

      ItemExprTreeAsList hivehashValues(&treePtr,
				    ITM_ITEM_LIST,
				    LEFT_LINEAR_TREE);

      // this expression becomes the hash operator for the first
      // hash value
      child(0) = hivehashValues[0];

      const NAType &childType = child(0)->getValueId().getType();

      if(childType.getTypeQualifier() == NA_CHARACTER_TYPE) {

	const CharType &chType = (CharType&)childType;
	CharInfo::Collation coll = chType.getCollation();
	
	if (CollationInfo::isSystemCollation(coll))
	    {
	      child(0) = new(generator->wHeap()) 
		CompEncode(child(0),FALSE, -1, CollationInfo::Compare);
	      child(0) = child(0)->bindNode(generator->getBindWA());
	    }
	  else
	    {
	      //--------------------------
	      if ((chType.isCaseinsensitive()) &&
		  (NOT casesensitiveHash()) &&
		  (NOT chType.isUpshifted())) {
		child(0) = new (generator->wHeap()) Upper(child(0));
		child(0) = child(0)->bindNode(generator->getBindWA());
	      }
	    }
	  }

      // add hash expressions for all other hash values and HiveHashComb
      // them together
      CollIndex nc = hivehashValues.entries();
      for (CollIndex i = 1; i < nc; i++)
	{
          ItemExpr *hi = hivehashValues[i];

	  const NAType &childType = hi->getValueId().getType();

	  if(childType.getTypeQualifier() == NA_CHARACTER_TYPE) {

	    const CharType &chType = (CharType&)childType;
	    CharInfo::Collation coll = chType.getCollation();
	    
            if (CollationInfo::isSystemCollation(coll))
	      {
		hi = new(generator->wHeap()) 
		    CompEncode(hi,FALSE, -1, CollationInfo::Compare);

		hi = hi->bindNode(generator->getBindWA());
	      }
	      else
	      {
		//-----------------------------
		if ((chType.isCaseinsensitive()) &&
		    (NOT casesensitiveHash()) &&
		    (NOT chType.isUpshifted())) {
		  hi = new (generator->wHeap()) Upper(hi);
		  hi = hi->bindNode(generator->getBindWA());
		}
		  //-----------------------
	      }
	  }

	  ItemExpr *hv = new(generator->wHeap()) HiveHash(hi);
	  result = new(generator->wHeap()) HiveHashComb(result,hv);
	}

      result->bindNode(generator->getBindWA());
    } else {
      const NAType &childType = child(0)->getValueId().getType();

      if(childType.getTypeQualifier() == NA_CHARACTER_TYPE) {

	const CharType &chType = (CharType&)childType;
	CharInfo::Collation coll = chType.getCollation();
	
        if (CollationInfo::isSystemCollation(coll))
	  {
	    child(0) = new (generator->wHeap()) 
	      CompEncode(child(0), FALSE, -1, CollationInfo::Compare);
	    
	    child(0) = child(0)->bindNode(generator->getBindWA());
          }
	  else
	  {
	    if ((chType.isCaseinsensitive()) &&
		(NOT casesensitiveHash()) &&
		(NOT chType.isUpshifted())) {
	      child(0) = new (generator->wHeap()) Upper(child(0));
	      child(0) = child(0)->bindNode(generator->getBindWA());
	    }
          }

      }
    }

  // do generic tasks for pre-code generation (e.g. recurse to the children)
  setReplacementExpr(result->ItemExpr::preCodeGen(generator));
  markAsPreCodeGenned();
  return getReplacementExpr();
} // Hash::preCodeGen()

// --------------------------------------------------------------
// member functions for HashDistPartHash operator
// Hash Function used by Hash Partitioning. This function cannot change
// once Hash Partitioning is released!  Defined for all data types,
// returns a 32 bit non-nullable hash value for the data item.
//--------------------------------------------------------------


ItemExpr * HashDistPartHash::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return getReplacementExpr();

  ItemExpr *result = this;

  // ---------------------------------------------------------------------
  // In the optimizer, a hash function accepts a comma-separated list
  // of columns. Replace this with the HashComb of the hash functions
  // of the individual list elements.
  // ---------------------------------------------------------------------
  if (child(0)->getOperatorType() == ITM_ITEM_LIST)
    {
      // child is a multi-valued expression, transform into multiple
      // hash expressions
      ExprValueId treePtr = child(0);

      ItemExprTreeAsList hashValues(&treePtr,
				    ITM_ITEM_LIST,
				    LEFT_LINEAR_TREE);

      // this expression becomes the hash operator for the first
      // hash value
      child(0) = hashValues[0];

      const NAType &childType = child(0)->getValueId().getType();

      if(childType.getTypeQualifier() == NA_CHARACTER_TYPE) {

	const CharType &chType = (CharType&)childType;
	
	CharInfo::Collation coll = chType.getCollation();

        if (CollationInfo::isSystemCollation(coll))
	  {
	    if (child(0)->getOperatorType() == ITM_NARROW)
	    {
	      ItemExpr* narrowsChild = child(0)->child(0);
	      const NAType &narrowsChildType= narrowsChild->getValueId().getType();
	      CMPASSERT(narrowsChildType.getTypeQualifier() == NA_CHARACTER_TYPE);
	      NAType *newType= narrowsChildType.newCopy(generator->wHeap());
	      CharType * newCharType = (CharType *) newType;
	      newCharType->setDataStorageSize(chType.getDataStorageSize());
  	    
	      child(0)->getValueId().changeType(newCharType);

	    }

	    child(0) = new(generator->wHeap()) 
			CompEncode(child(0),FALSE, -1, CollationInfo::Compare);

	    child(0) = child(0)->bindNode(generator->getBindWA());
	  }
	  else
	  {
	    if ((chType.isCaseinsensitive()) &&
		(NOT chType.isUpshifted())) {
	      child(0) = new (generator->wHeap()) Upper(child(0));
	      child(0) = child(0)->bindNode(generator->getBindWA());
	    }
	  }
      }

      // add hash expressions for all other hash values and HashComb
      // them together
      CollIndex nc = hashValues.entries();
      for (CollIndex i = 1; i < nc; i++)
	{
          ItemExpr *hi = hashValues[i];

	  const NAType &childType = hi->getValueId().getType();

	  if(childType.getTypeQualifier() == NA_CHARACTER_TYPE) {

	    const CharType &chType = (CharType&)childType;

	    CharInfo::Collation coll = chType.getCollation();

            if (CollationInfo::isSystemCollation(coll))
	    {
	      //Solution 10-081216-8006
	      if (hi->getOperatorType() == ITM_NARROW)
	      {
		ItemExpr* narrowsChild = hi->child(0);
		const NAType &narrowsChildType= narrowsChild->getValueId().getType();
		CMPASSERT(narrowsChildType.getTypeQualifier() == NA_CHARACTER_TYPE);
		NAType *newType= narrowsChildType.newCopy(generator->wHeap());
		CharType * newCharType = (CharType *) newType;
		newCharType->setDataStorageSize(chType.getDataStorageSize());
    	    
		hi->getValueId().changeType(newCharType);

	      }

		hi = new(generator->wHeap()) 
		      CompEncode(hi,FALSE, -1, CollationInfo::Compare);

		hi = hi->bindNode(generator->getBindWA());
	    }
	    else
	    {
  		if ((chType.isCaseinsensitive()) &&
		    (NOT chType.isUpshifted())) {
		  hi = new (generator->wHeap()) Upper(hi);
		  hi = hi->bindNode(generator->getBindWA());
		}
	      }
          }

	  ItemExpr *hv =
            new(generator->wHeap()) HashDistPartHash(hi);
	  result = new(generator->wHeap())
            HashDistPartHashComb(result,hv);
	}

      result->bindNode(generator->getBindWA());
    } else {
      const NAType &childType = child(0)->getValueId().getType();

      if(childType.getTypeQualifier() == NA_CHARACTER_TYPE) {

	const CharType &chType = (CharType&)childType;

	CharInfo::Collation coll = chType.getCollation();

        if (CollationInfo::isSystemCollation(coll))
	{
	  //Solution 10-081216-8006
	  if (child(0)->getOperatorType() == ITM_NARROW)
	  {
	    ItemExpr* narrowsChild = child(0)->child(0);
	    const NAType &narrowsChildType= narrowsChild->getValueId().getType();
	    CMPASSERT(narrowsChildType.getTypeQualifier() == NA_CHARACTER_TYPE);
	    NAType *newType= narrowsChildType.newCopy(generator->wHeap());
	    CharType * newCharType = (CharType *) newType;
	    newCharType->setDataStorageSize(chType.getDataStorageSize());
	    
	    child(0)->getValueId().changeType(newCharType);

	  }

	  child(0) = new(generator->wHeap()) 
			CompEncode(child(0),FALSE, -1, CollationInfo::Compare);

	  child(0) = child(0)->bindNode(generator->getBindWA());
	}
	else
	{
	    if ((chType.isCaseinsensitive()) &&
		(NOT chType.isUpshifted())) {
	      child(0) = new (generator->wHeap()) Upper(child(0));
	      child(0) = child(0)->bindNode(generator->getBindWA());
	    }
	  }
      }
    }

  // do generic tasks for pre-code generation (e.g. recurse to the children)
  setReplacementExpr(result->ItemExpr::preCodeGen(generator));
  markAsPreCodeGenned();
  return getReplacementExpr();
} // HashDistPartHash::preCodeGen()


ItemExpr * HostVar::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  ItemExpr * i = convertExternalType(generator);
  if (i == NULL)
    return NULL;

  return i;
}

ItemExpr * IndexColumn::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  ItemExpr * i = convertExternalType(generator);
  if (i == NULL)
    return NULL;

  return i;
}

ItemExpr * Generator::addCompDecodeForDerialization(ItemExpr * ie, NABoolean isAlignedFormat)
{
  if (!ie)
    return NULL;

  if ((ie->getOperatorType() == ITM_BASECOLUMN) ||
      (ie->getOperatorType() == ITM_INDEXCOLUMN))
    {
      if (! isAlignedFormat && HbaseAccess::isEncodingNeededForSerialization(ie))
	{
	  ItemExpr * newNode = new(wHeap()) CompDecode
	    (ie, &ie->getValueId().getType(), FALSE, TRUE);

	  newNode->bindNode(getBindWA());
	  if (getBindWA()->errStatus())
	    return NULL;

	  newNode = newNode->preCodeGen(this);
	  if (! newNode)
	    return NULL;  
	  return newNode;
	}
      else
	return ie;
    }

  for (Lng32 i = 0; i < ie->getArity(); i++)
    {
      ItemExpr * nie = addCompDecodeForDerialization(ie->child(i), isAlignedFormat);
      if (nie)
	ie->setChild(i, nie);
    }

  return ie;
}

ItemExpr * HbaseTimestamp::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return getReplacementExpr();

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  markAsPreCodeGenned();

  return this;
}

ItemExpr * HbaseVersion::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return getReplacementExpr();

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  markAsPreCodeGenned();

  return this;
}

ItemExpr * LOBoper::preCodeGen(Generator * generator)
{
  generator->setProcessLOB(TRUE);

  return BuiltinFunction::preCodeGen(generator);
}

ItemExpr * LOBconvert::preCodeGen(Generator * generator)
{
  NAColumn * col = child(0)->getValueId().getNAColumn(TRUE);
  if (col)
    {
      lobNum() = col->lobNum();
      lobStorageType() = col->lobStorageType();
      lobStorageLocation() = col->lobStorageLocation();
    }
  
  return LOBoper::preCodeGen(generator);
}


ItemExpr * LOBupdate::preCodeGen(Generator * generator)
{
  return LOBoper::preCodeGen(generator);
}

ItemExpr * MathFunc::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  // for ROUND, if the first operand is a BigNum, don't cast
  // the children to DOUBLE PRECISION; but do make sure the
  // second operand is an integer
  NABoolean castIt = TRUE;
  if (getOperatorType() == ITM_ROUND)
    {
      const NAType &typ0 = child(0)->getValueId().getType();
      if (((const NumericType &)typ0).isBigNum())
        {
          castIt = FALSE;

          if (getArity() > 1)
            {
              const NumericType &typ1 = (const NumericType &)child(1)->getValueId().getType();
              if (!typ1.isInteger() || !DFS2REC::isBinaryNumeric(typ1.getFSDatatype()))
                {
                  child(1) = new (generator->wHeap())
                    Cast(child(1),
                     new (generator->wHeap()) SQLLargeInt(
                      generator->wHeap(), 
                      typ1.isSigned(), 
                      typ1.supportsSQLnullLogical()));
                  child(1)->bindNode(generator->getBindWA());
                }
            }
        }
    }

  for (Int32 i = 0; i < getArity(); i++)
    {
      if (castIt)
        {
          const NAType &typ = child(i)->getValueId().getType();

          // Insert a cast node to convert child to a double precision.
          child(i) = new (generator->wHeap())
	    Cast(child(i),
	     new (generator->wHeap()) SQLDoublePrecision(
		  generator->wHeap(), typ.supportsSQLnullLogical()));

          child(i)->bindNode(generator->getBindWA());
        }

      child(i) = child(i)->preCodeGen(generator);
      if (! child(i).getPtr())
	return NULL;
    }

  markAsPreCodeGenned();
  return this;
} // MathFunc::preCodeGen()

ItemExpr * Modulus::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  for (Int32 i = 0; i < 2; i++)
    {
      const NumericType &typ
	= (NumericType&)child(i)->getValueId().getType();

      if (typ.isDecimal())
	{
	  // Insert a cast node to convert child to an LARGEINT.
	  child(i) = new (generator->wHeap())
	    Cast(child(i), new (generator->wHeap()) SQLLargeInt(generator->wHeap(), TRUE,
								typ.supportsSQLnullLogical()));
	}

      child(i)->bindNode(generator->getBindWA());

      child(i) = child(i)->preCodeGen(generator);
      if (! child(i).getPtr())
	return NULL;
    }

  markAsPreCodeGenned();
  return this;
} // Modulus::preCodeGen()

ItemExpr * ItemExpr::convertExternalType(Generator * generator)
{
  BindWA * bindWA = generator->getBindWA();

  if (getValueId().getType().isExternalType())
    {
      // this type is not supported internally.
      // Convert it to an equivalent internal type.
      ItemExpr * c = new (bindWA->wHeap())
	Cast(this, getValueId().getType().equivalentType(bindWA->wHeap()));

      c->synthTypeAndValueId();

      // mark 'this' as precodegenned so we don't go thru
      // this path again.
      markAsPreCodeGenned();
      c = c->preCodeGen(generator);
      unmarkAsPreCodeGenned();

      return c;
    }
  else
    return this;
}

ItemExpr * Parameter::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  ItemExpr * i = convertExternalType(generator);
  if (i == NULL)
    return NULL;

  return i;
}

ItemExpr * PivotGroup::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return getReplacementExpr();

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  ItemExpr * childExpr = child(0)->castToItemExpr();

  const NAType &type1 = 
    childExpr->getValueId().getType();
 
  if (type1.getTypeQualifier() != NA_CHARACTER_TYPE) 
    {
      Lng32 displayLen = type1.getDisplayLength(
                                                type1.getFSDatatype(),
                                                type1.getNominalSize(),
                                                type1.getPrecision(),
                                                type1.getScale(),
                                                0);

      NAType * newType = new(generator->getBindWA()->wHeap()) 
        SQLVarChar(generator->getBindWA()->wHeap(), displayLen, type1.supportsSQLnull());

      childExpr = new (generator->getBindWA()->wHeap()) Cast(childExpr, newType);
      
      childExpr = childExpr->bindNode(generator->getBindWA());
      if (! childExpr || generator->getBindWA()->errStatus())
        return NULL;

      childExpr = childExpr->preCodeGen(generator);
      if (! childExpr)
	return NULL;

      child(0) = childExpr;
    }

  markAsPreCodeGenned();

  return this;
}

ItemExpr * RandomNum::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (child(0))
    {
      const NAType &typ1 = child(0)->getValueId().getType();

      // Insert a cast node to convert child to an INT.
      child(0) = new (generator->wHeap())
	Cast(child(0), new (generator->wHeap()) SQLInt(generator->wHeap(), FALSE,
						       typ1.supportsSQLnullLogical()));

      child(0)->bindNode(generator->getBindWA());

      child(0) = child(0)->preCodeGen(generator);
      if (! child(0).getPtr())
	return NULL;
    }

  markAsPreCodeGenned();
  return this;
} // RandomNum::preCodeGen()

ItemExpr * Repeat::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  const NAType &typ2 = child(1)->getValueId().getType();

  // Insert a cast node to convert child 2 to an INT.
  child(1) = new (generator->wHeap())
    Cast(child(1), new (generator->wHeap()) SQLInt(generator->wHeap(), FALSE,
						   typ2.supportsSQLnullLogical()));

  child(1)->bindNode(generator->getBindWA());

  for (Int32 i = 0; i < getArity(); i++)
    {
      if (child(i))
	{
	  child(i) = child(i)->preCodeGen(generator);
	  if (! child(i).getPtr())
	    return NULL;
	}
    }

  markAsPreCodeGenned();
  return this;
} // Repeat::preCodeGen()

ItemExpr *ReplaceNull::preCodeGen(Generator *generator)
{
  if (nodeIsPreCodeGenned())
    return getReplacementExpr();

  NAType *dstAType = getValueId().getType().newCopy(generator->wHeap());
  const NAType& dstBType = getValueId().getType();

  if(child(0) == child(1))
    {
      dstAType->setNullable(TRUE);
    }

  child(1) = new(generator->wHeap()) Cast(child(1), dstAType);
  child(2) = new(generator->wHeap()) Cast(child(2), &dstBType);

  child(1)->bindNode(generator->getBindWA());
  child(2)->bindNode(generator->getBindWA());

  setReplacementExpr(ItemExpr::preCodeGen(generator));
  markAsPreCodeGenned();
  return getReplacementExpr();
}

ItemExpr * TriRelational::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return getReplacementExpr();

  // ---------------------------------------------------------------------
  // The executor does not handle tri-relational operators. It either
  // handles key exclusion expressions if the operator is part or a key
  // predicate, or the tri-relational operator gets converted into
  // a case statement (see comment in file ItemFunc.h).
  // ---------------------------------------------------------------------
  NABoolean lessOrLe = (getOperatorType() == ITM_LESS_OR_LE);
  BiRelat *exclusive = new(generator->wHeap()) BiRelat(
       (IFX lessOrLe THENX ITM_LESS ELSEX ITM_GREATER),
       child(0),
       child(1));
  BiRelat *inclusive = new(generator->wHeap()) BiRelat(
       (IFX lessOrLe THENX ITM_LESS_EQ ELSEX ITM_GREATER_EQ),
       child(0),
       child(1));
  exclusive->setSpecialNulls(getSpecialNulls());
  inclusive->setSpecialNulls(getSpecialNulls());

  ItemExpr * result = new(generator->wHeap()) Case(
       NULL,
       new(generator->wHeap()) IfThenElse(
	    child(2),
	    exclusive,
	    inclusive));

  result->bindNode(generator->getBindWA());

  // do generic tasks for pre-code generation (e.g. recurse to the children)
  setReplacementExpr(result->preCodeGen(generator));
  markAsPreCodeGenned();
  return getReplacementExpr();
} // TriRelational::preCodeGen()

ItemExpr *
HashDistrib::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  // Assert that the operands are unsigned int.
  //
  NumericType *numeric = (NumericType *)(&(child(0)->getValueId().getType()));
  GenAssert(numeric->getFSDatatype()==REC_BIN32_UNSIGNED &&
	    numeric->getScale()==0,
	    "invalid first operand type to function HashDistrib");

  numeric = (NumericType *)(&(child(1)->getValueId().getType()));
  GenAssert(numeric->getFSDatatype()==REC_BIN32_UNSIGNED &&
	    numeric->getScale()==0,
	    "invalid second operand type to function HashDistrib");

  markAsPreCodeGenned();
  return this;
}

ItemExpr * ProgDistribKey::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  // Assert that all operands are of type unsigned int.
  //
  for (Int32 i=0; i<3; i++)
    {
      NumericType *numeric =
        (NumericType *)(&(child(i)->getValueId().getType()));

      GenAssert(numeric->getFSDatatype()==REC_BIN32_UNSIGNED &&
		numeric->getScale()==0,
		"invalid operand type to function ProgDistribKey");
    }

  markAsPreCodeGenned();
  return this;
}

ItemExpr *
PAGroup::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  // Assert that the operands are unsigned int.
  //
  NumericType *numeric = (NumericType *)(&(child(0)->getValueId().getType()));
  GenAssert(numeric->getFSDatatype()==REC_BIN32_UNSIGNED &&
	    numeric->getScale()==0,
	    "invalid first operand type to function PAGroup");

  numeric = (NumericType *)(&(child(1)->getValueId().getType()));
  GenAssert(numeric->getFSDatatype()==REC_BIN32_UNSIGNED &&
	    numeric->getScale()==0,
	    "invalid second operand type to function PAGroup");

  numeric = (NumericType *)(&(child(2)->getValueId().getType()));
  GenAssert(numeric->getFSDatatype()==REC_BIN32_UNSIGNED &&
	    numeric->getScale()==0,
	    "invalid third operand type to function PAGroup");

  markAsPreCodeGenned();
  return this;
}

ItemExpr *
ScalarVariance::preCodeGen(Generator *generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! ItemExpr::preCodeGen(generator))
    return NULL;

  NumericType *result_type =
    (NumericType *)(&(getValueId().getType()));

  NumericType *type_op1 =
    (NumericType *)(&(child(0)->castToItemExpr()->getValueId().getType()));

  NumericType *type_op2 =
    (NumericType *)(&(child(1)->castToItemExpr()->getValueId().getType()));

  NumericType *type_op3 =
    (NumericType *)(&(child(1)->castToItemExpr()->getValueId().getType()));

  GenAssert(result_type->getTypeQualifier() == NA_NUMERIC_TYPE &&
	    type_op1->getTypeQualifier() == NA_NUMERIC_TYPE &&
	    type_op2->getTypeQualifier() == NA_NUMERIC_TYPE &&
	    type_op3->getTypeQualifier() == NA_NUMERIC_TYPE &&
	    !result_type->isExact() &&
	    !type_op1->isExact() &&
	    !type_op2->isExact() &&
	    !type_op3->isExact() &&
	    result_type->getBinaryPrecision() == SQL_DOUBLE_PRECISION &&
	    type_op1->getBinaryPrecision() == SQL_DOUBLE_PRECISION &&
	    type_op2->getBinaryPrecision() == SQL_DOUBLE_PRECISION &&
	    type_op3->getBinaryPrecision() == SQL_DOUBLE_PRECISION,
	    "ScalarVariance: Invalid Inputs");

  markAsPreCodeGenned();
  return this;
}

ItemExpr * Substring::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  child(0) = child(0)->preCodeGen(generator);
  if (! child(0).getPtr())
    return NULL;

  for (Int32 i = 1; i < getArity(); i++)
    {
      if (child(i))
	{
	  const NAType &typ1 = child(i)->getValueId().getType();

	  // Insert a cast node to convert child to an INT.
	  child(i) = new (generator->wHeap())
	    Cast(child(i), new (generator->wHeap()) SQLInt(generator->wHeap(), TRUE,
							   typ1.supportsSQLnullLogical()));

	  child(i)->bindNode(generator->getBindWA());

	  child(i) = child(i)->preCodeGen(generator);
	  if (! child(i).getPtr())
	    return NULL;
	}
    }

  markAsPreCodeGenned();
  return this;
} // Substring::preCodeGen()


ItemExpr * ItemExpr::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  Lng32 nc = (Lng32)getArity();

  for (Lng32 index = 0; index < nc; index++)
    {
      child(index) = child(index)->preCodeGen(generator);
      if (! child(index).getPtr())
	return NULL;
    }

  markAsPreCodeGenned();

  return this;
} // ItemExpr::preCodeGen()

// ---------------------------------------------------------
// Methods for class VEGRewritePairs
// ---------------------------------------------------------
VEGRewritePairs::VEGRewritePairs(CollHeap* heap)
  : heap_(heap),
    vegRewritePairs_(&valueIdHashFunc, 1009, TRUE, heap)
{
} 

ULng32 VEGRewritePairs::valueIdHashFunc(const CollIndex & v) 
{
  return (ULng32)v;
}

const VEGRewritePairs::VEGRewritePair *
VEGRewritePairs::getPair( const ValueId& original) const
{
  CollIndex k(original);
  return vegRewritePairs_.getFirstValue(&k);

} // getPair(..)


NABoolean VEGRewritePairs::
getRewritten(ValueId& rewritten, const ValueId& original) const
{
  NABoolean found = FALSE;
  const VEGRewritePairs::VEGRewritePair * vrPairPtr = NULL;
  if (vrPairPtr = getPair(original)){
      rewritten = vrPairPtr->getRewritten();
      found = TRUE;
  }

  return found;
} // getRewritten

VEGRewritePairs::~VEGRewritePairs()
{
  clear();

} // VEGRewritePairs::~VEGRewritePairs()

void
VEGRewritePairs::insert(const ValueId& original,
			const ValueId& rewritten)
{
  // Precondition:
  // original must have not been rewritten before:
  CMPASSERT(getPair(original) == NULL);

  VEGRewritePairs::VEGRewritePair * vrPairPtr =
    new (heap_) VEGRewritePairs::VEGRewritePair(original,rewritten);
  CMPASSERT(vrPairPtr != NULL);

  CollIndex* key = (CollIndex*) new (heap_) CollIndex(original);


  vegRewritePairs_.insert(key, vrPairPtr);
}


void VEGRewritePairs::VEGRewritePair::print(FILE *ofd) const
{
  Lng32 orId = CollIndex(original_),
    reId = CollIndex(rewritten_);
  fprintf(ofd,"<%d, %d>",orId,reId);
}


void VEGRewritePairs::print( FILE* ofd,
			     const char* indent,
			     const char* title) const
{
  BUMP_INDENT(indent);
  fprintf(ofd,"%s %s\n%s",NEW_INDENT,title,NEW_INDENT);

  CollIndex *key;
  VEGRewritePair *value;
  NAHashDictionaryIterator<CollIndex, VEGRewritePair> iter(vegRewritePairs_);

  for (CollIndex i=0; i < iter.entries(); i++)
    {
      iter.getNext(key, value);
      value->print(ofd);
    }
}

// PhysTranspose::preCodeGen() -------------------------------------------
// Perform local query rewrites such as for the creation and
// population of intermediate tables, for accessing partitioned
// data. Rewrite the value expressions after minimizing the dataflow
// using the transitive closure of equality predicates.
//
// PhysTranspose::preCodeGen() - is basically the same as the RelExpr::
// preCodeGen() except that here we replace the VEG references in the
// transUnionVals() as well as the selectionPred().
//
// Parameters:
//
// Generator *generator
//    IN/OUT : A pointer to the generator object which contains the state,
//             and tools (e.g. expression generator) to generate code for
//             this node.
//
// ValueIdSet &externalInputs
//    IN    : The set of external Inputs available to this node.
//
//
RelExpr * PhysTranspose::preCodeGen(Generator * generator,
				    const ValueIdSet &externalInputs,
				    ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  // Check if the pivs of this operator and it's child are the same.
  // If they are not, make them the same.
  replacePivs();

  // Resolve the VEGReferences and VEGPredicates, if any, that appear
  // in the Characteristic Inputs, in terms of the externalInputs.
  //
  getGroupAttr()->resolveCharacteristicInputs(externalInputs);

  // My Characteristic Inputs become the external inputs for my children.
  //
  Int32 nc = getArity();
  for (Int32 index = 0; index < nc; index++)
    {
      ValueIdSet childPulledInputs;

      child(index) = child(index)->preCodeGen(generator,
					      externalInputs,
					      pulledNewInputs);
      if (! child(index).getPtr())
	return NULL;

      // process additional input value ids the child wants
      getGroupAttr()->addCharacteristicInputs(childPulledInputs);
      pulledNewInputs += childPulledInputs;
    }

  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  //
  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);

  // The transUnionVals have access to only the Input Values.
  // These can come from the parent of be the outputs of the child.
  //
  for(CollIndex v = 0; v < transUnionVectorSize(); v++) {
    ValueIdList valIdList = transUnionVector()[v];

    valIdList.replaceVEGExpressions
      (availableValues,
       getGroupAttr()->getCharacteristicInputs());
  }

  // The selectionPred has access to the output values generated by transpose.
  // as well as any input values from the parent or child.
  //
  getInputAndPotentialOutputValues(availableValues);


  // Rewrite the selection predicates.
  //
  NABoolean replicatePredicates = TRUE;
  selectionPred().replaceVEGExpressions
    (availableValues,
     getGroupAttr()->getCharacteristicInputs(),
     FALSE, // no key predicates here
     0 /* no need for idempotence here */,
     replicatePredicates
     );

  // Replace VEG references in the outputs and remove redundant
  // outputs.
  //
  getGroupAttr()->resolveCharacteristicOutputs
    (availableValues,
     getGroupAttr()->getCharacteristicInputs());
  generator->oltOptInfo()->setMultipleRowsReturned(TRUE);
  markAsPreCodeGenned();

  return this;
} // PhysTranspose::preCodeGen

// -----------------------------------------------------------------------
// PhyPack::preCodeGen() is basically the same as RelExpr::preCodeGen().
// It replaces the VEG's in its packingExpr_ as well as selectionPred_.
// -----------------------------------------------------------------------
RelExpr* PhyPack::preCodeGen(Generator* generator,
                             const ValueIdSet& externalInputs,
			     ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  // Check if the pivs of this operator and it's child are the same.
  // If they are not, make them the same.
  replacePivs();

  // Resolve the VEGReferences and VEGPredicates, if any, that appear
  // in the Characteristic Inputs, in terms of the externalInputs.
  //
  getGroupAttr()->resolveCharacteristicInputs(externalInputs);

  // My Characteristic Inputs become the external inputs for my children.
  //
  Int32 nc = getArity();
  for(Int32 index = 0; index < nc; index++)
  {
    ValueIdSet childPulledInputs;

    child(index) = child(index)->preCodeGen(generator,
					    externalInputs,
					    pulledNewInputs);
    if(! child(index).getPtr()) return NULL;

    // process additional input value ids the child wants
    getGroupAttr()->addCharacteristicInputs(childPulledInputs);
    pulledNewInputs += childPulledInputs;
  }

  if (getFirstNRows() != -1)
    {
      RelExpr * firstn = new(generator->wHeap()) FirstN(child(0),
                                                        getFirstNRows(),
                                                        FALSE /* [any n] is good enough */);

      // move my child's attributes to the firstN node.
      // Estimated rows will be mine.
      firstn->setEstRowsUsed(getEstRowsUsed());
      firstn->setMaxCardEst(getMaxCardEst());
      firstn->setInputCardinality(child(0)->getInputCardinality());
      firstn->setPhysicalProperty(child(0)->getPhysicalProperty());
      firstn->setGroupAttr(child(0)->getGroupAttr());
      //10-060516-6532 -Begin
      //When FIRSTN node is created after optimization phase, the cost
      //of that node does not matter.But, display_explain and explain
      //show zero operator costs and rollup cost which confuses the user.
      //Also, the VQP crashes when cost tab for FIRSTN node is selected.
      //So, creating a cost object will fix this.
      //The operator cost is zero and rollup cost is same as it childs.
      Cost* firstnNodecost = new HEAP Cost();
      firstn->setOperatorCost(firstnNodecost);
      Cost* rollupcost = (Cost *)(child(0)->getRollUpCost());
      *rollupcost += *firstnNodecost;
      firstn->setRollUpCost(rollupcost);
      //10-060516-6532 -End

      firstn =
        firstn->preCodeGen(generator,
                           getGroupAttr()->getCharacteristicInputs(),
                           pulledNewInputs);
      if (! firstn)
        return NULL;

      setChild(0, firstn);
    }

  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  //
  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);
  const ValueIdSet& inputValues = getGroupAttr()->getCharacteristicInputs();

  // Replace VEG's in both the packing expression and the packing factor.
  //
  packingFactor().replaceVEGExpressions(availableValues,inputValues);
  packingExpr().replaceVEGExpressions(availableValues,inputValues);

  // The selectionPred has access to the output values generated by Pack.
  //
  getInputAndPotentialOutputValues(availableValues);

  // Rewrite the selection predicates.
  //
  NABoolean replicatePredicates = TRUE;
  selectionPred().replaceVEGExpressions(availableValues,
                                        inputValues,
                                        FALSE, // no key predicates here
                                        0 /* no need for idempotence here */,
                                        replicatePredicates
                                        );

  // Replace VEG references in the outputs and remove redundant outputs.
  //
  getGroupAttr()->resolveCharacteristicOutputs(availableValues,inputValues);

  markAsPreCodeGenned();
  return this;

} // PhyPack::preCodeGen()


//
//PrecodeGen method for class PhysicalTuple list
//This was put in as a fix for cr 10-010327-1947.
//Before the fix the RelExpr was getting to the generator
//with a VEGRef still in it, because the VEGRef from the
//tupleExpr had not be removed and resolved correctly.
RelExpr * PhysicalTuple::preCodeGen(Generator * generator,
                              const ValueIdSet& externalInputs,
                              ValueIdSet& pulledNewInputs_)
{
  ValueIdSet availableValues = externalInputs;
  tupleExpr().replaceVEGExpressions
                (availableValues, externalInputs);

  return (RelExpr::preCodeGen(generator, availableValues, pulledNewInputs_));
} // PhysicalTuple::preCodeGen()
//

RelExpr * PhysicalTupleList::preCodeGen(Generator * generator,
                              const ValueIdSet& externalInputs,
                              ValueIdSet& pulledNewInputs_)
{
  ValueIdSet availableValues = externalInputs;
  tupleExpr().replaceVEGExpressions
                (availableValues, externalInputs);

  generator->oltOptInfo()->setMultipleRowsReturned(TRUE);

  return (RelExpr::preCodeGen(generator, availableValues, pulledNewInputs_));
} // PhysicalTupleList::preCodeGen()

RelExpr * CompoundStmt::preCodeGen(Generator * generator,
			           const ValueIdSet & externalInputs,
			           ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  // Check if the pivs of this operator and it's child are the same.
  // If they are not, make them the same.
  replacePivs();

  ValueIdSet availableValues;
  ValueIdSet childPulledInputs;

  // Resolve the VEGReferences and VEGPredicates, if any, that appear
  // in the Characteristic Inputs, in terms of the externalInputs.
  getGroupAttr()->resolveCharacteristicInputs(externalInputs);
  availableValues = getGroupAttr()->getCharacteristicInputs();

  // This is similar to what is done in Join::precodeGen when we have a TSJ.
  // A compound statement node behaves in a similar way to a TSJ node since
  // it flows values from left to right.

  // My Characteristic Inputs become the external inputs for my left child.
  child(0) = child(0)->preCodeGen(generator,availableValues,childPulledInputs);
  if (! child(0).getPtr())
    return NULL;

   // process additional input value ids the child wants
  // (see RelExpr::preCodeGen())
  getGroupAttr()->addCharacteristicInputs(childPulledInputs);
  pulledNewInputs += childPulledInputs;
  availableValues += childPulledInputs;
  childPulledInputs.clear();

  // The values produced as output by my left child can be used as
  // "external" inputs by my right child.
  availableValues += child(0)->getGroupAttr()->getCharacteristicOutputs();

  // Process the right child
  child(1) = child(1)->preCodeGen(generator,availableValues,childPulledInputs);
  if (! child(1).getPtr())
    return NULL;

  // process additional input value ids the child wants
  // (see RelExpr::preCodeGen())
  getGroupAttr()->addCharacteristicInputs(childPulledInputs);
  pulledNewInputs += childPulledInputs;

  // Accumulate the values that are provided as inputs by my parent
  // together with the values that are produced as outputs by my
  // children. Use these values for rewriting the VEG expressions.
  getInputValuesFromParentAndChildren(availableValues);

  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  getInputAndPotentialOutputValues(availableValues);

  // Rewrite the selection predicates.
  NABoolean replicatePredicates = TRUE;
  selectionPred().replaceVEGExpressions
                     (availableValues,
		      getGroupAttr()->getCharacteristicInputs(),
		      FALSE, // no need to generate key predicates here
		      0 /* no need for idempotence here */,
		      replicatePredicates
		     );

  getGroupAttr()->resolveCharacteristicOutputs
                     (availableValues,
		      getGroupAttr()->getCharacteristicInputs());

  // Xn will be aborted if there is any IUD stmt within this CS and
  // an error occurs at runtime.
  if (generator->foundAnUpdate())
    {
      //generator->setUpdAbortOnError(TRUE);
      generator->setUpdSavepointOnError(FALSE);
      generator->setUpdErrorOnError(FALSE);
      //generator->setUpdPartialOnError(FALSE);
    }

  generator->setAqrEnabled(FALSE);

  markAsPreCodeGenned();

  return this;
} // CompoundStmt::preCodeGen

RelExpr * FirstN::preCodeGen(Generator * generator,
			     const ValueIdSet & externalInputs,
			     ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;


  if (getFirstNRows() > 0)
     generator->setTopNRows(getFirstNRows());
  else
     generator->setTopNRows(ActiveSchemaDB()->getDefaults().getAsULong(GEN_SORT_TOPN_THRESHOLD));

  if (! RelExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  markAsPreCodeGenned();

  return this;
} // FirstN::preCodeGen

RelExpr * RelRoutine::preCodeGen (Generator * generator,
					     const ValueIdSet &externalInputs,
					     ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;
  
  if (!RelExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;
  
  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  //
  ValueIdSet availableValues;
  availableValues = getGroupAttr()->getCharacteristicInputs();

  const ValueIdSet &inputValues = getGroupAttr()->getCharacteristicInputs();
  getProcInputParamsVids().replaceVEGExpressions(availableValues, inputValues);
  generator->setAqrEnabled(FALSE);

  markAsPreCodeGenned();
  
  return this;
}

RelExpr * IsolatedNonTableUDR::preCodeGen (Generator * generator,
					     const ValueIdSet &externalInputs,
					     ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;
  
  if (!RelRoutine::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  // The VEG expressions in the selection predicates and the characteristic
  // outputs can reference any expression that is either a potential output
  // or a characteristic input for this RelExpr. Supply these values for
  // rewriting the VEG expressions.
  //
  ValueIdSet availableValues;
  availableValues = getGroupAttr()->getCharacteristicInputs();

  const ValueIdSet &inputValues = getGroupAttr()->getCharacteristicInputs();
  getNeededValueIds().replaceVEGExpressions(availableValues, inputValues);
  
  markAsPreCodeGenned();
  
  return this;
}

RelExpr * PhysicalTableMappingUDF::preCodeGen(Generator * generator,
                                              const ValueIdSet &externalInputs,
                                              ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;
  
  if (!RelRoutine::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);

  for(Int32 i = 0; i < getArity(); i++)
  {
    ValueIdList &childOutputs(getChildInfo(i)->getOutputIds());
    ValueIdList origChildOutputs(childOutputs);

    childOutputs.replaceVEGExpressions(
         availableValues,
         getGroupAttr()->getCharacteristicInputs());

    for (CollIndex j=0; j<childOutputs.entries(); j++)
      if (NOT(childOutputs[j].getType() == origChildOutputs[j].getType()))
        {
          // VEG rewrite changed the type.
          // Since we recorded the original type of the input
          // column and exposed this type to the UDF writer, don't
          // change the type now. Instead, add a cast back to the
          // original type.
          ItemExpr *castToOrigType = new(CmpCommon::statementHeap())
            Cast(childOutputs[j].getItemExpr(),
                 origChildOutputs[j].getType().newCopy());

          castToOrigType->synthTypeAndValueId();
          childOutputs[j] = castToOrigType->getValueId();
        }
  }

  planInfo_ = getPhysicalProperty()->getUDRPlanInfo();
  if (!getDllInteraction()->finalizePlan(this, planInfo_))
    return NULL;
  
  markAsPreCodeGenned();
  
  return this;
}

RelExpr * PhysicalFastExtract::preCodeGen (Generator * generator,
					     const ValueIdSet &externalInputs,
					     ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (getIsMainQueryOperator())
    generator->setIsFastExtract(TRUE);
  else
    generator->setContainsFastExtract(TRUE);

  if (!RelExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);

  getSelectListIds().replaceVEGExpressions(availableValues, externalInputs);

  if (isAppend())
    generator->setAqrEnabled(FALSE);

  // This relation is a linear fit to cpu consumption data observed during a 
  // performance run, while extracting data from the LINEITEM table. CPU Usage
  // can go from 0% to 50% according to this relation. CPU Usage is determined
  // by 2 factors (a) bytes of data extracted and (b) % non-character 
  // (termed numeric below) columns in each row (computed based on total max 
  // row size and tol non-char column size). Both factors have equal weigth, 
  // i.e. they can contribute at most 25% towards Cpu usage. For upto 50 GB
  // extracted data the bytes of extracted data increases linearly from 0% to
  // 25%. After 50 GB (total volume across all partitions), the contribution to
  // cpu usage from bytes extracted does not increase. Similarly the a table
  // all non-char columns can contribute upto 25% towards cpu usage. The numeric
  // factor is also weighted by the volume of data extracted.

  const Int32  plateauTabSizeInGB = 50;
  const float  weightOfBaseExtract = 0.5;
  const float  weightOfNumericExpressionEval = 0.5;
  const Int32  maxPossibleCpuUsage = 50 ; // in percentage units
  Int32 rowLength = child(0).getGroupAttr()->getCharacteristicOutputs().getRowLength();
  Int32 numericRowLength = child(0).getGroupAttr()->
			getCharacteristicOutputs().getRowLengthOfNumericCols();
  float numericRowLengthRatio = ((float) numericRowLength)/rowLength ;

  double bytesExtractedInGB = (getEstRowsUsed().value()*rowLength)/(1024*1024*1024);
  double bytesExtractedRatio = bytesExtractedInGB/plateauTabSizeInGB ;
  if (bytesExtractedRatio > 1)
	bytesExtractedRatio = 1;

  Int32 maxCpuUsage = (Int32) (maxPossibleCpuUsage*bytesExtractedRatio*(weightOfBaseExtract + 
                        (weightOfNumericExpressionEval*numericRowLengthRatio)));
  generator->setMaxCpuUsage(maxCpuUsage);

  markAsPreCodeGenned();

  return this;
}

RelExpr * RelLock::preCodeGen (Generator * generator,
			       const ValueIdSet &externalInputs,
			       ValueIdSet &pulledNewInputs)
{

  if (nodeIsPreCodeGenned())
    return this;

  // Since the newExch node is added as the parent
  // to SequenceGenerator node, this method gets
  // called again during the preCodeGen of t
  // newExch.
  if(parallelExecution_) 
    {
      // Add an exchange node here so this could be executed in ESP.
      RelExpr * exchange = new(generator->wHeap()) Exchange (this);
      exchange->setPhysicalProperty(this->getPhysicalProperty());
      exchange->setGroupAttr(this->getGroupAttr());
      
      markAsPreCodeGenned();

      exchange = exchange->preCodeGen(generator, externalInputs, pulledNewInputs);

      // Done.
      return exchange;

      /*
      RelExpr *newExch = 
	generator->insertEspExchange(this, getPhysicalProperty());
      
      ((Exchange *)newExch)->makeAnESPAccess();
      
      markAsPreCodeGenned();

      RelExpr * exch = 
	newExch->preCodeGen(generator, externalInputs, pulledNewInputs);
      
      return exch;
      */
    }
  
  if (!RelExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  markAsPreCodeGenned();

  return this;
}

RelExpr * StatisticsFunc::preCodeGen(Generator * generator,
				     const ValueIdSet & externalInputs,
				     ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! RelExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  // don't collect stats for stats func itself
  generator->setComputeStats(FALSE);

  markAsPreCodeGenned();

  // Done.
  return this;
}

RelExpr * ExeUtilGetStatistics::preCodeGen(Generator * generator,
					   const ValueIdSet & externalInputs,
					   ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! ExeUtilExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  // don't collect stats for stats func itself
  generator->setComputeStats(FALSE);

  markAsPreCodeGenned();

  // Done.
  return this;
}

RelExpr * ExeUtilWnrInsert::preCodeGen(Generator * generator,
                                   const ValueIdSet & externalInputs,
                                   ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! ExeUtilExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  markAsPreCodeGenned();

  return this;
}



ItemExpr * PositionFunc::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! BuiltinFunction::preCodeGen(generator))
    return NULL;

  const NAType &type1 =
    child(0)->castToItemExpr()->getValueId().getType();
  const NAType &type2 =
    child(1)->castToItemExpr()->getValueId().getType();
  
  CMPASSERT(
      (type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
      (type2.getTypeQualifier() == NA_CHARACTER_TYPE))
  
  const CharType &cType1 = (CharType&)type1;
  const CharType &cType2 = (CharType&)type2;

  CharInfo::Collation coll1 = cType1.getCollation();
  CharInfo::Collation coll2 = cType2.getCollation();

  CMPASSERT(coll1==coll2);

  setCollation(coll1);

  if (CollationInfo::isSystemCollation(coll1))
  {
    
    {
      ItemExpr * newEncode = 
	new(generator->wHeap()) 
	CompEncode(child(0),FALSE, -1, CollationInfo::Search);

      newEncode = newEncode->bindNode(generator->getBindWA());
      newEncode = newEncode->preCodeGen(generator);
      if (!newEncode)
	return NULL;
      setChild(0, newEncode);


      newEncode = 
	new(generator->wHeap()) 
	CompEncode(child(1), FALSE, -1,CollationInfo::Search);

      newEncode->bindNode(generator->getBindWA());
      newEncode = newEncode->preCodeGen(generator);
      if (!newEncode)
	return NULL;
      setChild(1, newEncode);
    }

  }

  markAsPreCodeGenned();
  return this;

} // PositionFunc::preCodeGen()


ItemExpr * Trim::preCodeGen(Generator * generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! BuiltinFunction::preCodeGen(generator))
    return NULL;

  const NAType &type1 =
    child(0)->castToItemExpr()->getValueId().getType();
  const NAType &type2 =
    child(1)->castToItemExpr()->getValueId().getType();
  
  CMPASSERT(
      (type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
      (type2.getTypeQualifier() == NA_CHARACTER_TYPE))
  
  const CharType &cType1 = (CharType&)type1;
  const CharType &cType2 = (CharType&)type2;

  CharInfo::Collation coll1 = cType1.getCollation();
  CharInfo::Collation coll2 = cType2.getCollation();

  CMPASSERT(coll1==coll2);

  setCollation(coll1);

  markAsPreCodeGenned();
  return this;

} // Trim::preCodeGen()

ItemExpr * NotIn::preCodeGen(Generator * generator)
{
  if (child(0)->getOperatorType() == ITM_ITEM_LIST)
  {//Multicolumn NotIn should not reach this far
    GenAssert(FALSE,"Multicolumn NotIn should not have reached this far");
    return NULL;
  }

  if (nodeIsPreCodeGenned())
  {
    return getReplacementExpr();
  }
  // if single column NOT IN reaches pre-code generation, then replace it with 
  // non equi-predicate form (NE)
  // An example of cases where NotIn reaches this far is a aquery like
  // select * from ta where (select sum(a2) from ta) not in (select b2 from tb);
  // where the NotIn predicate gets pushed down and is not caught at optimization 
  // time

  ValueId vid = createEquivNonEquiPredicate();

  ItemExpr * newPred = vid.getItemExpr();

  setReplacementExpr(newPred->preCodeGen(generator));
  markAsPreCodeGenned();

  return getReplacementExpr();


} // NotIn::preCodeGen()

short HbaseAccess::processSQHbaseKeyPreds(Generator * generator,
					  NAList<HbaseSearchKey*>& searchKeys,
					  ListOfUniqueRows &listOfUniqueRows,
					  ListOfRangeRows &listOfRangeRows)
{
  Int32 ct = 0;
  HbaseUniqueRows getSpec;
  getSpec.rowTS_ = -1;
  
  for (CollIndex i = 0; i<searchKeys.entries(); i++ ) 
    {
      HbaseSearchKey* searchKey = searchKeys[i];

      ValueIdSet newSelectionPreds;
      
      if ( searchKey->isUnique() ) 
	{
	  // Since we fill one rowId per entry, we will be using getRow() form of Get.
	  if ( (ct=searchKey->getCoveredLeadingKeys()) > 0 )
	    {
	      NAString result;
	      ValueIdList keyValues = searchKey->getBeginKeyValues();
	      
	      keyValues.convertToTextKey(searchKey->getKeyColumns(), result);
	      getSpec.rowIds_.insert(result);
	    } 
	  
	  //	  getSpec.addColumnNames(searchKey->getRequiredOutputColumns());
	} 
      else 
	{
	  // Multiple rows. Do Scan
	  HbaseRangeRows scanSpec;
	  scanSpec.beginKeyExclusive_ = FALSE;
	  scanSpec.endKeyExclusive_ = FALSE;
	  scanSpec.rowTS_ = -1;
	  
          if ((( !searchKey->areAllBeginKeysMissing() ) && 
	       ((ct=searchKey->getCoveredLeadingKeys()) > 0 )) ||
	      searchKey->isFalsePred())
	  {
	    ValueIdList beginKeyValues = searchKey->getBeginKeyValues();
	    beginKeyValues.convertToTextKey(searchKey->getKeyColumns(), 
					    scanSpec.beginRowId_);
	    scanSpec.beginKeyExclusive_ = 
              searchKey->isBeginKeyExclusive();
	  } 
	  
	  if ((( !searchKey->areAllEndKeysMissing() ) && 
	       (ct=searchKey->getCoveredLeadingKeys())) ||
	      searchKey->isFalsePred())
	  {
	    ValueIdList endKeyValues = searchKey->getEndKeyValues();
	    endKeyValues.convertToTextKey(searchKey->getKeyColumns(), 
					  scanSpec.endRowId_);
	    scanSpec.endKeyExclusive_ = searchKey->isEndKeyExclusive();
	  }  
	 
	  listOfRangeRows.insertAt(listOfRangeRows.entries(), scanSpec);
	}
    } // for
  
  if (getSpec.rowIds_.entries() > 0)
    listOfUniqueRows.insert(getSpec);
  
  return 0;
}

short HbaseAccess::processNonSQHbaseKeyPreds(Generator * generator,
				       ValueIdSet &preds,
				       ListOfUniqueRows &listOfUniqueRows,
				       ListOfRangeRows &listOfRangeRows)
{
  ValueId vid;
  ValueId eqRowIdValVid;
  ValueId eqColNameValVid;
  ItemExpr * ie = NULL;
  NABoolean rowIdFound = FALSE;
  NABoolean colNameFound = FALSE;
  NABoolean isConstParam = FALSE;

  ValueIdList newPredList;
  NABoolean addToNewPredList;
 
  HbaseUniqueRows hg;
  HbaseRangeRows hs;
 
  for (vid = preds.init(); 
       (preds.next(vid)); 
       preds.advance(vid))
    {
      ie = vid.getItemExpr();
      
      addToNewPredList = TRUE;
      ConstValue * constVal = NULL;
      if ((NOT rowIdFound) && (isEqGetExpr(ie, eqRowIdValVid, isConstParam, 
					   "ROW_ID")))
	{
	  rowIdFound = TRUE;
	  
	  if (isConstParam)
	    {
	      ConstantParameter*cp = (ConstantParameter*)eqRowIdValVid.getItemExpr();
	      constVal = cp->getConstVal();
	    }
	  else
	    constVal = (ConstValue*)eqRowIdValVid.getItemExpr();
	  NAString rid = *constVal->getRawText();
	  hg.rowIds_.insert(rid); 
	  
	  hg.rowTS_ = -1;
	  
	  addToNewPredList = FALSE;
	}
      
      if (isEqGetExpr(ie, eqColNameValVid, isConstParam, "COL_NAME"))
	{
	  colNameFound = TRUE;
	  
	  if (isConstParam)
	    {
	      ConstantParameter*cp = (ConstantParameter*)eqColNameValVid.getItemExpr();
	      constVal = cp->getConstVal();
	    }
	  else
	    constVal = (ConstValue*)eqColNameValVid.getItemExpr();
	  NAString col = *constVal->getRawText();
	  hg.colNames_.insert(col); 
	  hs.colNames_.insert(col);
	  
	  addToNewPredList = FALSE;
	}
      
      if (addToNewPredList)
	newPredList.insert(vid);
    } // for
  
  if ((rowIdFound) || (colNameFound))
    {
      preds.clear();
      preds.insertList(newPredList);
    }
  
  if (rowIdFound)
    {
      listOfUniqueRows.insert(hg);
    }
  else
    {
      hs.rowTS_ = -1;
      listOfRangeRows.insert(hs);
    }

  //  markAsPreCodeGenned();
  
  // Done.
  return 0;
}

////////////////////////////////////////////////////////////////////////////
// To push down, the predicate must have the following form:
//    <column>  <op>  <value-expr>
//
// and all of the following conditions must be met:
//
//      <column>:       a base table or index column which can be serialized.
//                            serialized: either the column doesn't need encoding, like
//                                            an unsigned integer,  or the column
//                                            was declared with the SERIALIZED option.
//      <op>:              eq, ne, gt, ge, lt, le
//      <value-expr>:  an expression that only contains const or param values, and
//                            <value-expr>'s datatype is not a superset of <column>'s datatype.
//
/////////////////////////////////////////////////////////////////////////////
NABoolean HbaseAccess::isHbaseFilterPred(Generator * generator, ItemExpr * ie,
					 ValueId &colVID, ValueId &valueVID,
					 NAString &op,
					 NABoolean &removeFromOrigList)
{
  NABoolean found = FALSE;
  removeFromOrigList = FALSE;
  NABoolean hbaseLookupPred = FALSE;
  NABoolean flipOp = FALSE;  // set to TRUE when column is child(1)

  if (ie && 
      ((ie->getOperatorType() >= ITM_EQUAL) &&
       (ie->getOperatorType() <= ITM_GREATER_EQ)))
    {
      ItemExpr * child0 = ie->child(0)->castToItemExpr();
      ItemExpr * child1 = ie->child(1)->castToItemExpr();
      
      if ((ie->child(0)->getOperatorType() == ITM_BASECOLUMN) &&
	  (NOT hasColReference(ie->child(1))))
	{
	  found = TRUE;
	  colVID = ie->child(0)->getValueId();
	  valueVID = ie->child(1)->getValueId();
	}
      else if ((ie->child(1)->getOperatorType() == ITM_BASECOLUMN) &&
	       (NOT hasColReference(ie->child(0))))
	{
	  found = TRUE;
          flipOp = TRUE;
	  colVID = ie->child(1)->getValueId();
	  valueVID = ie->child(0)->getValueId();
	}
      else if ((ie->child(0)->getOperatorType() == ITM_INDEXCOLUMN) &&
	       (NOT hasColReference(ie->child(1))))
	{
	  found = TRUE;
	  colVID = ie->child(0)->getValueId();
	  valueVID = ie->child(1)->getValueId();
	}
      else if ((ie->child(1)->getOperatorType() == ITM_INDEXCOLUMN) &&
	       (NOT hasColReference(ie->child(0))))
	{
	  found = TRUE;
          flipOp = TRUE;
	  colVID = ie->child(1)->getValueId();
	  valueVID = ie->child(0)->getValueId();
	}
      else if ((ie->child(0)->getOperatorType() == ITM_REFERENCE) &&
	       (NOT hasColReference(ie->child(1))))
	{
	  found = TRUE;
	  colVID = ie->child(0)->getValueId();
	  valueVID = ie->child(1)->getValueId();
	}
      else if ((ie->child(1)->getOperatorType() == ITM_REFERENCE) &&
	       (NOT hasColReference(ie->child(0))))
	{
	  found = TRUE;
          flipOp = TRUE;
	  colVID = ie->child(1)->getValueId();
	  valueVID = ie->child(0)->getValueId();
	}
      else if ((ie->child(0)->getOperatorType() == ITM_HBASE_COLUMN_LOOKUP) &&
	       (NOT hasColReference(ie->child(1))))
	{
	  HbaseColumnLookup * hcl = (HbaseColumnLookup*)ie->child(0)->castToItemExpr();
	  if (hcl->getValueId().getType().getTypeQualifier() == NA_CHARACTER_TYPE)
	    {
	      hbaseLookupPred = TRUE;

	      ItemExpr * newCV = new(generator->wHeap()) ConstValue(hcl->hbaseCol());
	      newCV = newCV->bindNode(generator->getBindWA());
	      newCV = newCV->preCodeGen(generator);
	      
	      found = TRUE;
	      colVID = newCV->getValueId();
	      valueVID = ie->child(1)->getValueId();
	    }
	}
      else if ((ie->child(1)->getOperatorType() == ITM_HBASE_COLUMN_LOOKUP) &&
	       (NOT hasColReference(ie->child(0))))
	{
	  HbaseColumnLookup * hcl = (HbaseColumnLookup*)ie->child(1)->castToItemExpr();
	  if (hcl->getValueId().getType().getTypeQualifier() == NA_CHARACTER_TYPE)
	    {
	      hbaseLookupPred = TRUE;

	      ItemExpr * newCV = new(generator->wHeap()) ConstValue(hcl->hbaseCol());
	      newCV = newCV->bindNode(generator->getBindWA());
	      newCV = newCV->preCodeGen(generator);
	      
	      found = TRUE;
              flipOp = TRUE;
	      colVID = newCV->getValueId();
	      valueVID = ie->child(0)->getValueId();
	    }
	}
    }

  if (found)
    {
      const NAType &colType = colVID.getType();
      const NAType &valueType = valueVID.getType();

      NABoolean generateNarrow = FALSE;
      if (NOT hbaseLookupPred)
	{
	  generateNarrow = valueType.errorsCanOccur(colType);
	  if ((generateNarrow)  || // value not a superset of column
	      (NOT columnEnabledForSerialization(colVID.getItemExpr())))
	    found = FALSE;
	}

      if (found)
	{
	  if (colType.getTypeQualifier() == NA_CHARACTER_TYPE)
	    {
	      const CharType &charColType = (CharType&)colType;
	      const CharType &charValType = (CharType&)valueType;
	      
	      if ((charColType.isCaseinsensitive() || charValType.isCaseinsensitive()) ||
		  (charColType.isUpshifted() || charValType.isUpshifted()))
		found = FALSE;
	    }
	  else if (colType.getTypeQualifier() == NA_NUMERIC_TYPE)
	    {
	      const NumericType &numType = (NumericType&)colType;
	      const NumericType &valType = (NumericType&)valueType;
	      if (numType.isBigNum() || valType.isBigNum())
		found = FALSE;
	    }
	}

      if (found)
	{
	  if ((ie) && (((BiRelat*)ie)->addedForLikePred()) &&
	      (valueVID.getItemExpr()->getOperatorType() == ITM_CONSTANT))
	    {
	      // remove trailing '\0' characters since this is being pushed down to hbase.
	      ConstValue * cv = (ConstValue*)(valueVID.getItemExpr());
	      char * cvv = (char*)cv->getConstValue();
	      Lng32 len = cv->getStorageSize() - 1;
	      while ((len > 0) && (cvv[len] == '\0'))
		len--;

	      NAString newCVV(cvv, len+1);

	      ItemExpr * newCV = new(generator->wHeap()) ConstValue(newCVV);
	      newCV = newCV->bindNode(generator->getBindWA());
	      newCV = newCV->preCodeGen(generator);
	      valueVID = newCV->getValueId();
	    }

	  ItemExpr * castValue = NULL;
          if (NOT hbaseLookupPred)
            castValue = new(generator->wHeap()) Cast(valueVID.getItemExpr(), &colType);
          else
            {
              castValue = new(generator->wHeap()) Cast(valueVID.getItemExpr(), &valueVID.getType());
            }

	  if ((NOT hbaseLookupPred) &&
	      (isEncodingNeededForSerialization(colVID.getItemExpr())))
	    {
	      castValue = new(generator->wHeap()) CompEncode
		(castValue, FALSE, -1, CollationInfo::Sort, TRUE, FALSE);
	    }

	  castValue = castValue->bindNode(generator->getBindWA());
	  castValue = castValue->preCodeGen(generator);
	  
	  valueVID = castValue->getValueId();

	  // hbase pred evaluation compares the column byte string with the
	  // value byte string. It doesn't have a notion of nullability.
	  // For a nullable value stored in database, the first byte represents
	  // if the value is a null value.
	  // During pred evaluation in hbase, a null value could either get filtered
	  // out due to byte string comparison, or it may get returned back.
	  // For ex,   <col>  <gt>  <value>
	  //      will return TRUE if the first byte of <col> is a null value.
	  //      Similary, <col> <lt> <value> 
	  //      will return FALSE if the first byte of <col> is a null value.
	  // If the a null value gets filtered out, then that is correct semantics.
	  // But if the null value gets returned to executor, then it still need to be
	  // filtered out. To do that, the predicate need to be evaluated in executor
	  // with proper null semantics.
	  //
	  // Long story short, do not remove the original pred if the col or value is
	  // nullable.
	  //   
	  if ((colType.supportsSQLnull()) ||
	      (valueType.supportsSQLnull()))
	    {
	      removeFromOrigList = FALSE;
	    }
	  else
	    {
	      removeFromOrigList = TRUE;
	    }

	  if (ie->getOperatorType() == ITM_EQUAL)
	    op = "EQUAL";
	  else  if (ie->getOperatorType() == ITM_NOT_EQUAL)
	    op = "NOT_EQUAL";
	  else  if (ie->getOperatorType() == ITM_LESS)
            {
            if (flipOp)
              op = "GREATER";
            else
	      op = "LESS";
            }
	  else  if (ie->getOperatorType() == ITM_LESS_EQ)
            {
            if (flipOp)
              op = "GREATER_OR_EQUAL";
            else
	      op = "LESS_OR_EQUAL";
            }
	  else  if (ie->getOperatorType() == ITM_GREATER)
            {
            if (flipOp)
              op = "LESS";
            else
	      op = "GREATER";
            }
	  else  if (ie->getOperatorType() == ITM_GREATER_EQ)
            {
            if (flipOp)
              op = "LESS_OR_EQUAL";
            else
	      op = "GREATER_OR_EQUAL";
            }
	  else
	    op = "NO_OP";
	}
    }

  return found;
}

short HbaseAccess::extractHbaseFilterPreds(Generator * generator, 
					   ValueIdSet &preds, ValueIdSet &newExePreds)
{
   if (CmpCommon::getDefault(HBASE_FILTER_PREDS) == DF_OFF)
       return 0;
   // cannot push preds for aligned format row
   NABoolean isAlignedFormat = getTableDesc()->getNATable()->isAlignedFormat(getIndexDesc());

   if (isAlignedFormat)
     return 0;
  
   for (ValueId vid = preds.init(); 
       (preds.next(vid)); 
       preds.advance(vid))
    {
      ItemExpr * ie = vid.getItemExpr();
      ValueId colVID;
      ValueId valueVID;
      NABoolean removeFromOrigList = FALSE;
      NAString op;
      NABoolean isHFP =
        isHbaseFilterPred(generator, ie, colVID, valueVID, op, removeFromOrigList);
      
      if (isHFP) 
        {
          hbaseFilterColVIDlist_.insert(colVID);
          hbaseFilterValueVIDlist_.insert(valueVID);
          
          opList_.insert(op);
          
          if (NOT removeFromOrigList)
            newExePreds.insert(vid);
        }
      else 
        {
          newExePreds.insert(vid);
        }
      
    } // end for
  
  return 0;
}

////////////////////////////////////////////////////////////////////////////
// To push down, the predicate must have the following form:
//  xp:=  <column>  <op>  <value-expr>
//  xp:=  <column> is not null (no support for hbase lookup)
//  xp:=  <column> is null (no support for hbase lookup)
//    (xp:=<column> like <value-expr> not yet implemented)
//  xp:=<xp> OR <xp> (not evaluated in isHbaseFilterPredV2, but by extractHbaseFilterPredV2)
//  xp:=<xp> AND <xp>(not evaluated in isHbaseFilterPredV2, but by extractHbaseFilterPredV2)
//
// and all of the following conditions must be met:
//
//      <column>:       a base table or index column which can be serialized and belong to the table being scanned.
//                            serialized: either the column doesn't need encoding, like
//                                            an unsigned integer,  or the column
//                                            was declared with the SERIALIZED option.
//                      it also must not be an added column with default non null.
//      <op>:              eq, ne, gt, ge, lt, le
//      <value-expr>:  an expression that only contains const or param values, and
//                     <value-expr>'s datatype is not a superset of <column>'s datatype.
//
// colVID, valueID and op are output parameters.
/////////////////////////////////////////////////////////////////////////////
NABoolean HbaseAccess::isHbaseFilterPredV2(Generator * generator, ItemExpr * ie,
                     ValueId &colVID, ValueId &valueVID,
                     NAString &op)
{
  NABoolean foundBinary = FALSE;
  NABoolean foundUnary = FALSE;
  NABoolean hbaseLookupPred = FALSE;
  NABoolean flipOp = FALSE;  // set to TRUE when column is child(1)

  if (ie &&
      ((ie->getOperatorType() >= ITM_EQUAL) &&
       (ie->getOperatorType() <= ITM_GREATER_EQ))) //binary operator case
    {//begin expression
      ItemExpr * child0 = ie->child(0)->castToItemExpr();
      ItemExpr * child1 = ie->child(1)->castToItemExpr();

      if ((ie->child(0)->getOperatorType() == ITM_BASECOLUMN) &&
      (NOT hasColReference(ie->child(1))))
    {
      foundBinary = TRUE;
      colVID = ie->child(0)->getValueId();
      valueVID = ie->child(1)->getValueId();
    }
      else if ((ie->child(1)->getOperatorType() == ITM_BASECOLUMN) &&
           (NOT hasColReference(ie->child(0))))
    {
      foundBinary = TRUE;
      flipOp = TRUE;
      colVID = ie->child(1)->getValueId();
      valueVID = ie->child(0)->getValueId();
    }
      else if ((ie->child(0)->getOperatorType() == ITM_INDEXCOLUMN) &&
           (NOT hasColReference(ie->child(1))))
    {
      foundBinary = TRUE;
      colVID = ie->child(0)->getValueId();
      valueVID = ie->child(1)->getValueId();
    }
      else if ((ie->child(1)->getOperatorType() == ITM_INDEXCOLUMN) &&
           (NOT hasColReference(ie->child(0))))
    {
      foundBinary = TRUE;
      flipOp = TRUE;
      colVID = ie->child(1)->getValueId();
      valueVID = ie->child(0)->getValueId();
    }
      else if ((ie->child(0)->getOperatorType() == ITM_HBASE_COLUMN_LOOKUP) &&
           (NOT hasColReference(ie->child(1))))
    {
      HbaseColumnLookup * hcl = (HbaseColumnLookup*)ie->child(0)->castToItemExpr();
      if (hcl->getValueId().getType().getTypeQualifier() == NA_CHARACTER_TYPE)
        {
          hbaseLookupPred = TRUE;

          ItemExpr * newCV = new(generator->wHeap()) ConstValue(hcl->hbaseCol());
          newCV = newCV->bindNode(generator->getBindWA());
          newCV = newCV->preCodeGen(generator);

          foundBinary = TRUE;
          colVID = newCV->getValueId();
          valueVID = ie->child(1)->getValueId();
        }
    }
      else if ((ie->child(1)->getOperatorType() == ITM_HBASE_COLUMN_LOOKUP) &&
           (NOT hasColReference(ie->child(0))))
    {
      HbaseColumnLookup * hcl = (HbaseColumnLookup*)ie->child(1)->castToItemExpr();
      if (hcl->getValueId().getType().getTypeQualifier() == NA_CHARACTER_TYPE)
        {
          hbaseLookupPred = TRUE;

          ItemExpr * newCV = new(generator->wHeap()) ConstValue(hcl->hbaseCol());
          newCV = newCV->bindNode(generator->getBindWA());
          newCV = newCV->preCodeGen(generator);

          foundBinary = TRUE;
          flipOp = TRUE;
          colVID = newCV->getValueId();
          valueVID = ie->child(0)->getValueId();
        }
    }
    }//end binary operators
  else if (ie && ((ie->getOperatorType() == ITM_IS_NULL)||(ie->getOperatorType() == ITM_IS_NOT_NULL))){//check for unary operators
      ItemExpr * child0 = ie->child(0)->castToItemExpr();
      if ((ie->child(0)->getOperatorType() == ITM_BASECOLUMN) ||
          (ie->child(0)->getOperatorType() == ITM_INDEXCOLUMN)){
          foundUnary = TRUE;
          colVID = ie->child(0)->getValueId();
          valueVID = NULL_VALUE_ID;
      }

  }//end unary operators

  //check if found columns belong to table being scanned (so is not an input to the scan node)
  if (foundBinary || foundUnary){
    ValueId dummyValueId;
    if (getGroupAttr()->getCharacteristicInputs().referencesTheGivenValue(colVID,dummyValueId)){
        foundBinary=FALSE;
        foundUnary=FALSE;
    }
  }
  //check if not an added column with default non null
  if ((foundBinary || foundUnary)&& (NOT hbaseLookupPred)){
        if (colVID.isColumnWithNonNullNonCurrentDefault()){
            foundBinary=FALSE;
            foundUnary=FALSE;
        }
  }

  if (foundBinary)
    {
      const NAType &colType = colVID.getType();
      const NAType &valueType = valueVID.getType();

      NABoolean generateNarrow = FALSE;
      if (NOT hbaseLookupPred)
    {
      generateNarrow = valueType.errorsCanOccur(colType);
      if ((generateNarrow)  || // value not a superset of column
          (NOT columnEnabledForSerialization(colVID.getItemExpr())))
          foundBinary = FALSE;
    }

      if (foundBinary)
    {
      if (colType.getTypeQualifier() == NA_CHARACTER_TYPE)
        {
          const CharType &charColType = (CharType&)colType;
          const CharType &charValType = (CharType&)valueType;

          if ((charColType.isCaseinsensitive() || charValType.isCaseinsensitive()) ||
          (charColType.isUpshifted() || charValType.isUpshifted()))
         foundBinary = FALSE;
        }
      else if (colType.getTypeQualifier() == NA_NUMERIC_TYPE)
        {
          const NumericType &numType = (NumericType&)colType;
          const NumericType &valType = (NumericType&)valueType;
          if (numType.isBigNum() || valType.isBigNum())
         foundBinary = FALSE;
        }
    }

      if (foundBinary)
    {
      if ((ie) && (((BiRelat*)ie)->addedForLikePred()) &&
          (valueVID.getItemExpr()->getOperatorType() == ITM_CONSTANT))
        {
          // remove trailing '\0' characters since this is being pushed down to hbase.
          ConstValue * cv = (ConstValue*)(valueVID.getItemExpr());
          char * cvv = (char*)cv->getConstValue();
          Lng32 len = cv->getStorageSize() - 1;
          while ((len > 0) && (cvv[len] == '\0'))
            len--;

          NAString newCVV(cvv, len+1);

          ItemExpr * newCV = new(generator->wHeap()) ConstValue(newCVV);
          newCV = newCV->bindNode(generator->getBindWA());
          newCV = newCV->preCodeGen(generator);
          valueVID = newCV->getValueId();
        }

      ItemExpr * castValue = NULL;
          if (NOT hbaseLookupPred)
            castValue = new(generator->wHeap()) Cast(valueVID.getItemExpr(), &colType);
          else
            {
              castValue = new(generator->wHeap()) Cast(valueVID.getItemExpr(), &valueVID.getType());
            }

      if ((NOT hbaseLookupPred) &&
          (isEncodingNeededForSerialization(colVID.getItemExpr())))
        {
          castValue = new(generator->wHeap()) CompEncode
        (castValue, FALSE, -1, CollationInfo::Sort, TRUE, FALSE);
        }

      castValue = castValue->bindNode(generator->getBindWA());
      castValue = castValue->preCodeGen(generator);

      valueVID = castValue->getValueId();

      NAString nullType;

      if ((colType.supportsSQLnull()) ||
          (valueType.supportsSQLnull()))
        {
          nullType = "_NULL";
        }
      else
        {
          nullType = "";
        }

      // append -NULL to the operator to signify the java code generating pushdown filters to handle NULL semantic logic
      if (ie->getOperatorType() == ITM_EQUAL)
          op = "EQUAL"+nullType;
      else  if (ie->getOperatorType() == ITM_NOT_EQUAL)
        op = "NOT_EQUAL"+nullType;
      else  if (ie->getOperatorType() == ITM_LESS){
          if (flipOp)
              op = "GREATER"+nullType;
          else
              op = "LESS"+nullType;
      }
      else  if (ie->getOperatorType() == ITM_LESS_EQ){
          if (flipOp)
              op = "GREATER_OR_EQUAL"+nullType;
          else
              op = "LESS_OR_EQUAL"+nullType;
      }else  if (ie->getOperatorType() == ITM_GREATER){
          if (flipOp)
              op = "LESS"+nullType;
          else
              op = "GREATER"+nullType;
      }else  if (ie->getOperatorType() == ITM_GREATER_EQ){
          if (flipOp)
              op = "LESS_OR_EQUAL"+nullType;
          else
              op = "GREATER_OR_EQUAL"+nullType;
      }else
        op = "NO_OP"+nullType;
    }
    }
  if (foundUnary){
      const NAType &colType = colVID.getType();
      NAString nullType;

      if (colType.supportsSQLnull())
        {
          nullType = "_NULL";
        }
      else
        {
          nullType = "";
        }
      if (ie->getOperatorType() == ITM_IS_NULL)
              op = "IS_NULL"+nullType;
      else if (ie->getOperatorType() == ITM_IS_NOT_NULL)
              op = "IS_NOT_NULL"+nullType;
  }

  return foundBinary || foundUnary;
}
short HbaseAccess::extractHbaseFilterPredsVX(Generator * generator,
           ValueIdSet &preds, ValueIdSet &newExePreds){
    //separate the code that should not belong in the recursive function
       if (CmpCommon::getDefault(HBASE_FILTER_PREDS) == DF_OFF)
        return 0;
       // check if initial (version 1) implementation
       if (CmpCommon::getDefault(HBASE_FILTER_PREDS) == DF_MINIMUM)
        return extractHbaseFilterPreds(generator,preds,newExePreds);

       // if here, we are DF_MEDIUM
       // cannot push preds for aligned format row
       NABoolean isAlignedFormat = getTableDesc()->getNATable()->isAlignedFormat(getIndexDesc());

       if (isAlignedFormat)
         return 0;
       //recursive function call
       opList_.insert("V2");//to instruct the java side that we are dealing with predicate pushdown V2 semantic, add "V2" marker
       extractHbaseFilterPredsV2(generator,preds,newExePreds,FALSE);
       return 0;

}

// return true if successfull push down of node
NABoolean HbaseAccess::extractHbaseFilterPredsV2(Generator * generator,
                       ValueIdSet &preds, ValueIdSet &newExePreds, NABoolean checkOnly)
{

    // the isFirstAndLayer is used to allow detecting top level predicate that can still be pushed to executor
    int addedNode=0;
    for (ValueId vid = preds.init();
       (preds.next(vid));
       preds.advance(vid))
    {
      ItemExpr * ie = vid.getItemExpr();

      // if it is AND operation, recurse through left and right children
      if (ie->getOperatorType() == ITM_AND){
          ValueIdSet leftPreds;
          ValueIdSet rightPreds;
          leftPreds += ie->child(0)->castToItemExpr()->getValueId();
          rightPreds += ie->child(1)->castToItemExpr()->getValueId();
          //cannot be first AND layer, both left and right must be pushable to get anything pushed
          if(extractHbaseFilterPredsV2(generator, leftPreds, newExePreds, TRUE)&&
             extractHbaseFilterPredsV2(generator, rightPreds, newExePreds, TRUE)){// both left and right child must match
              if(!checkOnly){
                  extractHbaseFilterPredsV2(generator, leftPreds, newExePreds, FALSE);//generate tree
                  extractHbaseFilterPredsV2(generator, rightPreds, newExePreds, FALSE);//generate tree
                  opList_.insert("AND");
              }
            if (preds.entries()==1)
                return TRUE;
          }
          else{
              if(!checkOnly){
                  newExePreds.insert(vid);
              }
              if (preds.entries()==1)
                    return FALSE;
          }
          continue;
          // the OR case is easier, as we don t have the case of top level expression that can still be pushed to executor
      }//end if AND
      else if(ie->getOperatorType() == ITM_OR){
          ValueIdSet leftPreds;
          ValueIdSet rightPreds;
          leftPreds += ie->child(0)->castToItemExpr()->getValueId();
          rightPreds += ie->child(1)->castToItemExpr()->getValueId();
          //both left and right must be pushable to get anything pushed
          if(extractHbaseFilterPredsV2(generator, leftPreds, newExePreds, TRUE)&&
             extractHbaseFilterPredsV2(generator, rightPreds, newExePreds, TRUE)){// both left and right child must match
              if(!checkOnly){
                  extractHbaseFilterPredsV2(generator, leftPreds, newExePreds, FALSE);//generate tree
                  extractHbaseFilterPredsV2(generator, rightPreds, newExePreds, FALSE);//generate tree
                  opList_.insert("OR");
                  if (addedNode>0)opList_.insert("AND"); // if it is not the first node add to the push down, AND it with the rest
                  addedNode++; // we just pushed it down, so increase the node count pushed down.
              }
              if (preds.entries()==1)
               return TRUE;
          }
          else{// if predicate cannot be pushed down
              if(!checkOnly){
                  newExePreds.insert(vid);
              }
              if (preds.entries()==1)
                    return FALSE;
          }
          continue;
      }//end if OR

      ValueId colVID;
      ValueId valueVID;

      NAString op;
      NABoolean isHFP =
        isHbaseFilterPredV2(generator, ie, colVID, valueVID, op);

      if (isHFP && !checkOnly){// if pushable, push it
          hbaseFilterColVIDlist_.insert(colVID);
          if (valueVID != NULL_VALUE_ID) hbaseFilterValueVIDlist_.insert(valueVID);// don't insert valueID for unary operators.
          opList_.insert(op);
          if (addedNode>0)opList_.insert("AND"); // if it is not the first node add to the push down, AND it with the rest
          addedNode++; // we just pushed it down, so increase the node count pushed down.
        }else if (!checkOnly){//if not pushable, pass it for executor evaluation.
            newExePreds.insert(vid);
        }
      if (preds.entries()==1){
          return isHFP; // if we are not on the first call level, where we can have multiple preds, exit returning the pushability
      }

    } // end for

  return TRUE;//don't really care, means we are top level.
}


void HbaseAccess::computeRetrievedCols()
{
  GroupAttributes     fakeGA;
  ValueIdSet          requiredValueIds(getGroupAttr()->
				       getCharacteristicOutputs());
  ValueIdSet          coveredExprs;

  // ---------------------------------------------------------------------
  // Make fake group attributes with all inputs that are available to
  // the file scan node and with no "native" values.
  // Then call the "coverTest" method, offering it all the index columns
  // as additional inputs. "coverTest" will mark those index columns that
  // it actually needs to satisfy the required value ids, and that is
  // what we actually want. The actual cover test should always succeed,
  // otherwise the FileScan node would have been inconsistent.
  // ---------------------------------------------------------------------

  fakeGA.addCharacteristicInputs(getGroupAttr()->getCharacteristicInputs());
  requiredValueIds += selectionPred();
  requiredValueIds += executorPred();

  fakeGA.coverTest(requiredValueIds,              // char outputs + preds
		   getIndexDesc()->getIndexColumns(), // all index columns
		   coveredExprs,                  // dummy parameter
		   retrievedCols());               // needed index cols

  //
  // *** This CMPASSERT goes off sometimes, indicating an actual problem.
  // Hans has agreed to look into it (10/18/96) but I (brass) am
  // commenting it out for now, for sake of my time in doing a checking.
  //
  //  CMPASSERT(coveredExprs == requiredValueIds);

}

RelExpr * HbaseAccess::preCodeGen(Generator * generator,
				  const ValueIdSet & externalInputs,
				  ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  const PartitioningFunction* myPartFunc = getPartFunc();

  // use const HBase keys only if we don't have to add
  // partitioning key predicates
  if ( myPartFunc == NULL ||
       !myPartFunc->isPartitioned() ||
       myPartFunc->isAReplicationPartitioningFunction())
    if (!processConstHBaseKeys(
           generator,
           this,
           getSearchKey(),
           getIndexDesc(),
           executorPred(),
           getHbaseSearchKeys(),
           listOfUniqueRows_,
           listOfRangeRows_))
      return NULL;

  if (! FileScan::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  //compute isUnique:
  NABoolean isUnique = FALSE;
  if (listOfRangeRows_.entries() == 0)
    {
      if ((searchKey() && searchKey()->isUnique()) &&
    (listOfUniqueRows_.entries() == 0))
  isUnique = TRUE;
      else if ((NOT (searchKey() && searchKey()->isUnique())) &&
         (listOfUniqueRows_.entries() == 1) &&
         (listOfUniqueRows_[0].rowIds_.entries() == 1))
  isUnique = TRUE;
    }

  // executorPred() contains an ANDed list of predicates.
  // if hbase filter preds are enabled, then extracts those preds from executorPred()
  // which could be pushed down to hbase.
  // Do this only for non-unique scan access.
  ValueIdSet newExePreds;
  ValueIdSet* originExePreds = new (generator->wHeap())ValueIdSet(executorPred()) ;//saved for futur nullable column check

  if (CmpCommon::getDefault(HBASE_FILTER_PREDS) != DF_MINIMUM){ // the check for V2 and above is moved up before calculating retrieved columns
      if ((NOT isUnique) &&
          (extractHbaseFilterPredsVX(generator, executorPred(), newExePreds)))
        return this;

      // if some filter preds were found, then initialize executor preds with new exe preds.
      // newExePreds may be empty which means that all predicates were changed into
      // hbase preds. In this case, nuke existing exe preds.
      if (hbaseFilterColVIDlist_.entries() > 0)
        setExecutorPredicates(newExePreds);
  }

  ValueIdSet colRefSet;

  computeRetrievedCols();
  for (ValueId valId = retrievedCols().init();
       retrievedCols().next(valId);
       retrievedCols().advance(valId))
    {
      ValueId dummyValId;
      if ((valId.getItemExpr()->getOperatorType() != ITM_CONSTANT) &&
	  (getGroupAttr()->getCharacteristicOutputs().referencesTheGivenValue(valId, dummyValId)))
	colRefSet.insert(valId);
    }

  if (getTableDesc()->getNATable()->isHbaseCellTable())
    {
      for (Lng32 i = 0; i < getIndexDesc()->getIndexColumns().entries(); i++)
	{
	  //	  retColRefSet_.insert(getIndexDesc()->getIndexColumns()[i]);
	}
    }
  else if (getTableDesc()->getNATable()->isHbaseRowTable())
    {
      NASet<NAString> * hbaseColNameSet = 
	generator->getBindWA()->hbaseColUsageInfo()->hbaseColNameSet
	((QualifiedName*)&getTableDesc()->getNATable()->getTableName());

      NABoolean starFound = FALSE;
      for (Lng32 ij = 0; ij < hbaseColNameSet->entries(); ij++)
	{
	  NAString &colName = (*hbaseColNameSet)[ij];

	  retHbaseColRefSet_.insert(colName);

	  if (colName == "*")
	    starFound = TRUE;
	}

      if (starFound)
	retHbaseColRefSet_.clear();
    }
  else
    {
      // create the list of columns that need to be retrieved from hbase .
      // first add all columns referenced in the executor pred.
      HbaseAccess::addReferenceFromVIDset(executorPred(), TRUE, TRUE, colRefSet);


      HbaseAccess::addReferenceFromVIDset
        (getGroupAttr()->getCharacteristicOutputs(), TRUE, TRUE, colRefSet);

      for (ValueId valId = colRefSet.init();
	   colRefSet.next(valId);
	   colRefSet.advance(valId))
	{
	  ValueId dummyValId;
	  if (NOT getGroupAttr()->getCharacteristicInputs().referencesTheGivenValue(valId, dummyValId))
            {
              retColRefSet_.insert(valId);

              if (valId.getItemExpr()->getOperatorType() == ITM_HBASE_TIMESTAMP)
                {
                  Lng32 colNumber = ((BaseColumn*)((HbaseTimestamp*)valId.getItemExpr())->col())->getColNumber();
                  ValueId colVID = getIndexDesc()->getIndexColumns()[colNumber];
                  retColRefSet_.insert(colVID);
                }

              if (valId.getItemExpr()->getOperatorType() == ITM_HBASE_VERSION)
                {
                  Lng32 colNumber = ((BaseColumn*)((HbaseVersion*)valId.getItemExpr())->col())->getColNumber();
                  ValueId colVID = getIndexDesc()->getIndexColumns()[colNumber];
                  retColRefSet_.insert(colVID);
                }

            }
        }

      // add key columns. If values are missing in hbase, then atleast the key
      // value is needed to retrieve a row.
      //only if needed. If there is already a non nullable non added non nullable with default columns in the set, we should not need to add
      //any other columns.
      if (CmpCommon::getDefault(HBASE_FILTER_PREDS) == DF_MEDIUM && getMdamKeyPtr() == NULL){ //only enable column retrieval optimization with DF_MEDIUM and not for MDAM scan
          bool needAddingNonNullableColumn = true; //assume we need to add one non nullable column
          for (ValueId vid = retColRefSet_.init();// look for each column in th eresult set if one match the criteria non null non added non nullable with default
                  retColRefSet_.next(vid);
                  retColRefSet_.advance(vid))
          {
            if (originExePreds->isNotNullable(vid)){// it is non nullable
                OperatorTypeEnum operatorType = vid.getItemExpr()->getOperatorType();
                if ((operatorType == ITM_BASECOLUMN || operatorType == ITM_INDEXCOLUMN) && !vid.isColumnWithNonNullNonCurrentDefault()){//check if with non null or non current default... notgood
                    needAddingNonNullableColumn = false; // we found one column meeting all criteria
                    break;
                }
            }
          }
          if (needAddingNonNullableColumn){ // ok now we need to add one key column that is not nullable
              bool foundAtLeastOneKeyColumnNotNullable = false;
              for(int i=getIndexDesc()->getIndexKey().entries()-1; i>=0;i--)// doing reverse search is making sure we are trying to avoid to use _SALT_ column
                                                                         // because _SALT_ is physicaly the last column therefore we don't skip columns optimally if using _SALT_ column
              {
                  ValueId vaId = getIndexDesc()->getIndexKey()[i];
                  if ( (vaId.getItemExpr()->getOperatorType() == ITM_BASECOLUMN && !((BaseColumn*)vaId.getItemExpr())->getNAColumn()->getType()->supportsSQLnullPhysical())||
                          (vaId.getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN && !((IndexColumn*)vaId.getItemExpr())->getNAColumn()->getType()->supportsSQLnullPhysical())
                          ){ //found good key column candidate?
                      HbaseAccess::addReferenceFromItemExprTree(vaId.getItemExpr(),TRUE,FALSE,retColRefSet_); // add it
                      foundAtLeastOneKeyColumnNotNullable = true; //tag we found it
                      break; // no need to look further
                  }
              }
              if (!foundAtLeastOneKeyColumnNotNullable){//oh well, did not find any key column non nullable, let s add all key columns
                  HbaseAccess::addColReferenceFromVIDlist(getIndexDesc()->getIndexKey(), retColRefSet_);
              }
          }
      }else //end if DF_MEDIUM
          HbaseAccess::addColReferenceFromVIDlist(getIndexDesc()->getIndexKey(), retColRefSet_);
    }

  if ((getMdamKeyPtr()) &&
      ((listOfRangeRows_.entries() > 0) ||
       (listOfUniqueRows_.entries() > 0)))
    {
      GenAssert(0, "listOfRange/Unique cannot be used if mdam is chosen.");

      return NULL;
    }

  // flag for both hive and hbase tables
  generator->setHdfsAccess(TRUE);

  if (!isUnique)
      generator->oltOptInfo()->setMultipleRowsReturned(TRUE) ;

  // Do not allow cancel of unique queries but allow cancel of queries 
  // that are part of a rowset operation. 
  if ((isUnique) &&
      (NOT generator->oltOptInfo()->multipleRowsReturned()))
    {
      generator->setMayNotCancel(TRUE);
      uniqueHbaseOper() = TRUE;
    }
  else
    {
      generator->oltOptInfo()->setOltCliOpt(FALSE);

      if (isUnique)
	{
	  if ((CmpCommon::getDefault(HBASE_ROWSET_VSBB_OPT) == DF_ON) &&
	      (NOT generator->isRIinliningForTrafIUD()) &&
	      (searchKey() && searchKey()->isUnique()))
	    {
	      uniqueRowsetHbaseOper() = TRUE;
	    }
	}
    }

  // executorPred() contains an ANDed list of predicates.
  // if hbase filter preds are enabled, then extracts those preds from executorPred()
  // which could be pushed down to hbase.
  // Do this only for non-unique scan access.
  if (CmpCommon::getDefault(HBASE_FILTER_PREDS) == DF_MINIMUM){ //keep the check for pushdown after column retrieval for pushdown V1.
    if ((NOT isUnique) &&
        (extractHbaseFilterPreds(generator, executorPred(), newExePreds)))
      return this;

  // if some filter preds were found, then initialize executor preds with new exe preds.
  // newExePreds may be empty which means that all predicates were changed into
  // hbase preds. In this case, nuke existing exe preds.
  if (hbaseFilterColVIDlist_.entries() > 0)
      setExecutorPredicates(newExePreds);
  }//DF_MINIMUM

  snpType_ = SNP_NONE;
  DefaultToken  tok = CmpCommon::getDefault(TRAF_TABLE_SNAPSHOT_SCAN);
  if (tok == DF_LATEST)
    //latest snapshot -- new way used with scan independent from bulk unload
    snpType_= SNP_LATEST;
  else if (tok == DF_SUFFIX)
    //the exsiting where snapshot scan is used with bulk unload
    snpType_ = SNP_SUFFIX;

  markAsPreCodeGenned();
  
  // Done.
  return this;
}

RelExpr * HbaseAccessCoProcAggr::preCodeGen(Generator * generator,
					    const ValueIdSet & externalInputs,
					    ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! HbaseAccess::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  // Rebuild the aggregate expressions tree
  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);
  aggregateExpr().replaceVEGExpressions
                     (availableValues,
  	 	      getGroupAttr()->getCharacteristicInputs());

  markAsPreCodeGenned();
  
  // Done.
  return this;
}

RelExpr * ExeUtilHbaseCoProcAggr::preCodeGen(Generator * generator,
					    const ValueIdSet & externalInputs,
					    ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! ExeUtilExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  // Rebuild the aggregate expressions tree
  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);
  aggregateExpr().replaceVEGExpressions
                     (availableValues,
  	 	      getGroupAttr()->getCharacteristicInputs());

  markAsPreCodeGenned();
  
  // Done.
  return this;
}

RelExpr * ExeUtilOrcFastAggr::preCodeGen(Generator * generator,
                                         const ValueIdSet & externalInputs,
                                         ValueIdSet &pulledNewInputs)
{
  if (nodeIsPreCodeGenned())
    return this;

  if (! ExeUtilExpr::preCodeGen(generator,externalInputs,pulledNewInputs))
    return NULL;

  // Rebuild the aggregate expressions tree
  ValueIdSet availableValues;
  getInputValuesFromParentAndChildren(availableValues);
  aggregateExpr().replaceVEGExpressions
                     (availableValues,
  	 	      getGroupAttr()->getCharacteristicInputs());

  markAsPreCodeGenned();
  
  // Done.
  return this;
}

ItemExpr * SplitPart::preCodeGen(Generator *generator)
{
  if (nodeIsPreCodeGenned())
    return this;

  child(0) = child(0)->preCodeGen(generator);
  if (! child(0).getPtr())
    return NULL;

  child(1) = child(1)->preCodeGen(generator);
  if (! child(1).getPtr())
    return NULL;

  for (Int32 i = 2; i < getArity(); i++)
    {
      if (child(i))
        {
          const NAType &typ1 = child(i)->getValueId().getType();
          
          //Insert a cast node to convert child to an INT.
          child(i) = new (generator->wHeap())
            Cast(child(i), new (generator->wHeap()) SQLInt(generator->wHeap(), TRUE,
                                                         typ1.supportsSQLnullLogical()));

          child(i)->bindNode(generator->getBindWA());
          child(i) = child(i)->preCodeGen(generator);
          if (! child(i).getPtr())
            return NULL;

        }
    }

  markAsPreCodeGenned();
  return this;
}
