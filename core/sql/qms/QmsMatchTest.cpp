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
// File:         MatchOutput.cpp
// Description:  
//               
//               
//               
//
// Created:      08/28/08
// ***********************************************************************

#include "QmsMVDetails.h"
#include "QmsMVCandidate.h"
#include "QmsMatchTest.h"
#include "Range.h"


//========================================================================
//  Class RewriteInstructionsItem
//========================================================================

RewriteInstructionsItem::RewriteInstructionsItem(const QRElementPtr  queryElement,
						 const QRElementPtr  mvElement,
						 ResultCode	      resultCode,
						 ResultStatus	      status,
						 ADD_MEMCHECK_ARGS_DEF(CollHeap* heap))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
    ,queryElement_(queryElement)
    ,mvElement_(mvElement)
    ,resultCode_(resultCode)
    ,secondaryResultCode_(RC_INVALID_RESULT)
    ,status_(status)
    ,subElements_(new (heap) compositeMatchingResults(ADD_MEMCHECK_ARGS_PASS(heap)))
    ,extraHubTables_(heap)
    ,backJoinTables_(heap)
{
}

// ***************************************************************************
// ***************************************************************************
RewriteInstructionsItem::RewriteInstructionsItem(const RewriteInstructionsItem& other,
						 ADD_MEMCHECK_ARGS_DEF(CollHeap* heap))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
    ,queryElement_(other.queryElement_)
    ,mvElement_(other.mvElement_)
    ,resultCode_(other.resultCode_)
    ,secondaryResultCode_(other.secondaryResultCode_)
    ,status_(other.status_)
    ,subElements_(new (heap) compositeMatchingResults(ADD_MEMCHECK_ARGS_PASS(heap)))
    ,extraHubTables_(heap)
    ,backJoinTables_(heap)
{
  extraHubTables_.insert(other.extraHubTables_);
  backJoinTables_.insert(other.backJoinTables_);
  addSubElements(other.subElements_);
}

// ***************************************************************************
// ***************************************************************************
RewriteInstructionsItem::~RewriteInstructionsItem()
{
  deletePtr(subElements_);
}

// ***************************************************************************
// ***************************************************************************
void RewriteInstructionsItem::addSubElement(RewriteInstructionsItemPtr subElement)
{ 
  // Collect the special tables at the top.
  extraHubTables_.insert(subElement->getExtraHubTables());
  backJoinTables_.insert(subElement->getBackJoinTables());

  // Add the rewrite instructions item to the list.
  subElements_->insert(subElement); 
}

// ***************************************************************************
// ***************************************************************************
void RewriteInstructionsItem::addSubElements(const RewriteInstructionsItemList& subElements, 
					     NABoolean isSpecial)
{ 
  if (subElements.entries() == 0)
    return;

  if (isSpecial)
  {
    // There are backjoin or extra hub tables involved - insert one by one.
    CollIndex maxEntries = subElements.entries();
    for (CollIndex i=0; i<maxEntries; i++)
      addSubElement(subElements[i]);
  }
  else
  {
    // No special tables involved - just insert the list.
    subElements_->insert(subElements); 
  }
}

// ***************************************************************************
// ***************************************************************************
void RewriteInstructionsItem::addSubElements(compositeMatchingResultsPtr other)
{
  addSubElements(other->providedInputs_, FALSE);
  addSubElements(other->notProvidedInputs_, FALSE);
  addSubElements(other->outsideInputs_, FALSE);
  addSubElements(other->indirectInputs_, FALSE);
  addSubElements(other->extrahubInputs_, TRUE);
  addSubElements(other->backJoinInputs_, TRUE);
}

// ***************************************************************************
// ***************************************************************************
const char* RewriteInstructionsItem::getResultString(ResultCode rc)
{
  switch (rc)
  {
    case RC_OUTSIDE	    : return "Outside";
    case RC_PROVIDED	    : return "Provided";
    case RC_NOTPROVIDED	    : return "NotProvided";
    case RC_REJECT	    : return "Reject";
    case RC_CONTINUE	    : return "Continue";
    case RC_EXTRAHUB	    : return "ExtraHub";
    case RC_BACKJOIN	    : return "BackJoin";
    case RC_INDIRECT	    : return "Indirect";
    case RC_INPUTS_PROVIDED : return "InputsProvided";
    case RC_EXPR_REWRITE    : return "Expression Rewrite";
    case RC_MATCH_RANGE     : return "RangeSpec comparison";
    case RC_SKIP_ME         : return "Skip me";
    case RC_ROLLUP	    : return "Rollup";
    case RC_ROLLUP_EXPR	    : return "Rollup expression";
    case RC_AGGR_EXPR       : return "Aggregate expression";
    case RC_INVALID_RESULT  : return "Invalid result";
    default		    : return "Invalid result code";
  }
}

const char* RewriteInstructionsItem::getResultString()
{
  return getResultString(getResultCode());
}

//========================================================================
//  Class compositeMatchingResults
//========================================================================

// ***************************************************************************
// ***************************************************************************
compositeMatchingResults::~compositeMatchingResults()
{
  for (CollIndex i=0; i<providedInputs_.entries(); i++)
    deletePtr(providedInputs_[i]);
  providedInputs_.clear();

  for (CollIndex i=0; i<notProvidedInputs_.entries(); i++)
    deletePtr(notProvidedInputs_[i]);
  notProvidedInputs_.clear();

  for (CollIndex i=0; i<outsideInputs_.entries(); i++)
    deletePtr(outsideInputs_[i]);
  outsideInputs_.clear();

  for (CollIndex i=0; i<extrahubInputs_.entries(); i++)
    deletePtr(extrahubInputs_[i]);
  extrahubInputs_.clear();

  for (CollIndex i=0; i<backJoinInputs_.entries(); i++)
    deletePtr(backJoinInputs_[i]);
  backJoinInputs_.clear();

  for (CollIndex i=0; i<indirectInputs_.entries(); i++)
    deletePtr(indirectInputs_[i]);
  indirectInputs_.clear();
}

// ***************************************************************************
// ***************************************************************************
void compositeMatchingResults::insert(RewriteInstructionsItemPtr item)
{
  switch(item->getResultCode())
  {
    case RC_PROVIDED:
      providedInputs_.insert(item);
      break;

    case RC_NOTPROVIDED:
      notProvidedInputs_.insert(item);
      break;

    case RC_OUTSIDE:
      outsideInputs_.insert(item);
      break;

    case RC_EXTRAHUB:
      extrahubInputs_.insert(item);
      break;

    case RC_BACKJOIN:
      backJoinInputs_.insert(item);
      break;

    case RC_INDIRECT:
      indirectInputs_.insert(item);
      break;

    case RC_SKIP_ME:
      break;

    default:
      assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL, 
                        FALSE, QRLogicException, 
			"Unexpected return code."); 
  } // switch
}

// ***************************************************************************
// ***************************************************************************
void compositeMatchingResults::insert(const RewriteInstructionsItemList& itemList)
{
  switch(itemList[0]->getResultCode())
  {
    case RC_PROVIDED:
      providedInputs_.insert(itemList);
      break;

    case RC_NOTPROVIDED:
      notProvidedInputs_.insert(itemList);
      break;

    case RC_OUTSIDE:
      outsideInputs_.insert(itemList);
      break;

    case RC_EXTRAHUB:
      extrahubInputs_.insert(itemList);
      break;

    case RC_BACKJOIN:
      backJoinInputs_.insert(itemList);
      break;

    case RC_INDIRECT:
      indirectInputs_.insert(itemList);
      break;

    default:
      assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                        FALSE, QRLogicException, 
			"Unexpected return code."); 
  } // switch
}

//========================================================================
//  Class MatchTest
//========================================================================

// ***************************************************************************
// Destructor - delete the rewrite instructionS LISTS.
// ***************************************************************************
MatchTest::~MatchTest()
{
  CollIndex maxEntries = listOfFinalInstructions_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
    deletePtr(listOfFinalInstructions_[i]);
  listOfFinalInstructions_.clear();

  maxEntries = listOfPendingWork_.entries();
  for (CollIndex j=0; j<maxEntries; j++)
    deletePtr(listOfPendingWork_[j]);
  listOfPendingWork_.clear();
}

// ***************************************************************************
// Add an item of rewrite instructions to the collection.
// ***************************************************************************
void MatchTest::addRewriteInstructions(RewriteInstructionsItemPtr rewrite) 
{
  ResultCode rc = rewrite->getResultCode();
  if (rewrite->hasSpecialSubElement())
  {
    candidate_->addExtraHubAndBackJoinTables(rewrite);
  }

  if (rewrite->isFinal())
    listOfFinalInstructions_.insert(rewrite);
  else
    listOfPendingWork_.insert(rewrite);
}

// ***************************************************************************
// ***************************************************************************
NABoolean MatchTest::matchPass2() 
{
  // Is there any pending work from Pass 1?
  if (isFinal())
  {
    // No - good, than we are done.
    return TRUE;
  }
  else
  {
    // Yes - then lets check them out.
    RewriteInstructionsItemList& pendingList = getListOfPendingWork();
    Int32 maxEntries = pendingList.entries();
    for (Int32 i=maxEntries-1; i>=0; i--)
    {
      RewriteInstructionsItemPtr pending = pendingList[i];

      // Remove the item from the pending list.
      // matchPass2OnElement() will add any needed items to the final list.
      pendingList.removeAt(i);

      // First verify that all the back-join and extra-hub columns are Final.
      if (verifyExtraHubAndBackJoinColumns(pending) == FALSE)
      {
	// Cannot be provided - disqualify the MV.
	deletePtr(pending);
	return FALSE;
      }
      
      // Are the resulting rewrite instructions final?
      if (pending->isFinal())
      {
	// Yes - add to the Final list.
	addRewriteInstructions(pending);
      }
      else
      {
	// Call the sub-class specific method to handle the real work.
	if (matchPass2OnElement(pending) == FALSE)
	{
	  deletePtr(pending);
	  return FALSE;
	}
      }

      //deletePtr(pending);
    }
  }

  // All the pending tests passed.
  return TRUE;
}  // MatchTest::matchPass2()

// ***************************************************************************
// ***************************************************************************
NABoolean MatchTest::verifyExtraHubAndBackJoinColumns(RewriteInstructionsItemPtr pending)
{
  ResultCode rc = pending->getResultCode();
  if (rc != RC_BACKJOIN &&
      rc != RC_EXTRAHUB &&
      pending->getSubElements()->backJoinInputs_.entries() == 0 &&
      pending->getSubElements()->extrahubInputs_.entries() == 0 )
  {
    // Nothing to do here.
    return TRUE;
  }


  if (rc == RC_BACKJOIN)
  {
    // Nothing much to check about Back-Joins.
    pending->setResultCode(RC_NOTPROVIDED);
    pending->setResultStatus(RS_FINAL);
  }
  if (rc == RC_EXTRAHUB)
  {
    if (verifyExtraHubColumn(pending) == FALSE)
    {
      return FALSE;
    }
    pending->setResultCode(RC_PROVIDED);
    pending->setResultStatus(RS_FINAL);
  }
  else if (pending->getActualQueryElement()->getElementType() == ET_Expr)
  {
    // Check the input columns for back joins.
    RewriteInstructionsItemList& backJoinList    = pending->getSubElements()->backJoinInputs_;
    for (Int32 i=backJoinList.entries()-1; i>=0; i--)
    {
      RewriteInstructionsItemPtr rewrite = backJoinList[i];

      rewrite->setResultCode(RC_NOTPROVIDED);
      rewrite->setResultStatus(RS_FINAL);
    }

    // Check the input columns for extra hubs.
    RewriteInstructionsItemList& extraHubList = pending->getSubElements()->extrahubInputs_;
    RewriteInstructionsItemList& providedList = pending->getSubElements()->providedInputs_;
    for (Int32 j=extraHubList.entries()-1; j>=0; j--)
    {
      RewriteInstructionsItemPtr rewrite = extraHubList[j];
      extraHubList.removeAt(j);

      if (verifyExtraHubColumn(rewrite) == FALSE)
      {
	// We have removed rewrite from its list, so we need to delete it explicitly.
	deletePtr(rewrite);
	return FALSE;
      }
      rewrite->setResultCode(RC_PROVIDED);
      rewrite->setResultStatus(RS_FINAL);

      providedList.insert(rewrite);
    }

    if (pending->getSubElements()->indirectInputs_.entries() == 0 &&
        pending->getSecondaryResultCode() != RC_ROLLUP)
    {
      // We handled all the easy cases - the expression is now Final.
      pending->setResultStatus(RS_FINAL);
    }
  }
  else if (pending->getActualQueryElement()->getElementType() == ET_JoinPred)
  {
    // Check the input columns for extra-hubs.
    const compositeMatchingResultsPtr subElements = pending->getSubElements();
    NABoolean foundGoodOne = FALSE;

    // If we have any extra-hub columns, use oine of them.
    if (subElements->extrahubInputs_.entries() > 0)
    {
      RewriteInstructionsItemList& extraHubList = subElements->extrahubInputs_;

      for (Int32 k=extraHubList.entries()-1; k>=0; k--)
      {
        RewriteInstructionsItemPtr rewrite = extraHubList[k];
        extraHubList.removeAt(k);

        if (foundGoodOne || verifyExtraHubColumn(rewrite) == FALSE)
        {
	  // We have removed rewrite from its list, so we need to delete it explicitly.
	  deletePtr(rewrite);
        }
        else
        {
          foundGoodOne = TRUE;
          pending->setResultCode(RC_PROVIDED);
          pending->setResultStatus(RS_FINAL);
          pending->setMvElement(rewrite->getMvElement());
        }
      } // for k
    } // if extrahub

    // Check the input columns for back joins.
    RewriteInstructionsItemList& backJoinList    = pending->getSubElements()->backJoinInputs_;
    NABoolean hasBackJoins = backJoinList.entries() > 0;

    //// Delete the sub-elements in the back-join list.
    //for (int l=backJoinList.entries()-1; l>=0; l--)
    //{
    //  RewriteInstructionsItemPtr rewrite = backJoinList[l];
    //  backJoinList.removeAt(l);
    //  deletePtr(rewrite);
    //}

    if (!foundGoodOne)
      if (!hasBackJoins)
        return FALSE;
      else
      {
        pending->setResultCode(RC_NOTPROVIDED);
        pending->setSecondaryResultCode(RC_BACKJOIN);
        pending->setResultStatus(RS_FINAL);
      }
  }

  return TRUE;
}  // verifyExtraHubAndBackJoinColumns()

// ***************************************************************************
// For extra hub tables, we now know the table is matched OK,
// but we need to verify the column is Provided.
// ***************************************************************************
NABoolean MatchTest::verifyExtraHubColumn(RewriteInstructionsItemPtr pending)
{
  QROutputPtr mvCol = NULL;
  const QRColumnPtr queryCol = pending->getActualQueryElement()->downCastToQRColumn();
  if (isProvidedColumn(queryCol, mvCol) == FALSE)
  {
    // The column is not Provided, so disqualify the MV.
    const NAString& colName = queryCol->getFullyQualifiedColumnName();
    candidate_->logMVWasDisqualified1("extra-hub column %s cannot be provided.", colName);
    return FALSE;
  }
  pending->setResultCode(RC_PROVIDED);
  pending->setMvElement(mvCol);
  pending->setResultStatus(RS_FINAL);

  return TRUE;
}  // MatchTest::verifyExtraHubColumn()

// ***************************************************************************
// A generic method for generating the result descriptor elements.
// The specific details are implemented by sub-classes.
// ***************************************************************************
NABoolean MatchTest::generateDescriptor(QRCandidatePtr resultDesc)
{
  RewriteInstructionsItemList& rewriteList = getListOfFinalInstructions();
  for (CollIndex i=0; i<rewriteList.entries(); i++)
  {
    RewriteInstructionsItemPtr rewrite = rewriteList[i];

    switch (rewrite->getResultCode())
    {
      case RC_PROVIDED:
	if (generateProvidedElement(resultDesc, rewrite) == FALSE)
	  return FALSE;
	break;

      case RC_NOTPROVIDED:
	if (generateNotProvidedElement(resultDesc, rewrite) == FALSE)
	  return FALSE;
	break;

      case RC_OUTSIDE:
	// No need to specify Outside elements in the result descriptor.
	//if (generateOutsideElement(resultDesc, rewrite) == FALSE)
	//  return FALSE;
	break;

      case RC_SKIP_ME:
	break;

      default:
	// These are the only result codes allowed for final rewrite instructions.
	assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                          FALSE, QRLogicException, 
			  "Unexpected result code for a final element."); 
    }
  }

  return TRUE;
}  // MatchTest::generateDescriptor()

// ***************************************************************************
// ***************************************************************************
RewriteInstructionsItemPtr MatchTest::matchColumn(const QRColumnPtr colElem, 
						  NABoolean	    fromExpr,
                                                  NABoolean         isAnEqualitySet) 
{
  if (isAnEqualitySet)
    return matchSingleColumn(colElem, fromExpr); // Avoid recursion.

  if (!candidate_->getQueryDetails()->isFromJoin(colElem))
    return matchSingleColumn(colElem, fromExpr);
  else 
  {
    const QRJoinPredPtr jp = candidate_->getQueryDetails()->getJoinPred(colElem);
    RewriteInstructionsItemPtr rewrite = matchEqualitySet(jp);
    if (rewrite)
    {
      // The rewrite instructions should reference the column, not the join pred.
      rewrite->setQueryElement(colElem);
    }
    return rewrite;
  }
}

// ***************************************************************************
// Match a single, full (not referencing) column (Pass 1)
// The options are:
// - Outside	  : The column's table is not covered by the MV.
// - Extra-hub	  : The column's table is in the MV's extra-hub. Verify in Pass 2.
// - Provided	  : The column is provided by the MV
// - BackJoinable : The column's table is covered by the MV, but the column 
//                  itself is not provided by the MV, however, the table's
//                  key columns are provided, so we can get the needed column 
//                  by using a back-join.
// - Indirect	  : This column is used as an input column to one of the MV's
//		    output expressions.
// - NULL	  : None of the above, the column cannot be provided by the 
//		    MV in any way. Disqualify the MV.
// ***************************************************************************
RewriteInstructionsItemPtr MatchTest::matchSingleColumn(const QRColumnPtr colElem, 
						        NABoolean	  fromExpr) 
{
  RewriteInstructionsItemPtr rewrite = NULL;
  const NAString* extraHubID = NULL;
  if(isOutsideColumn(colElem, extraHubID))
  {
    rewrite = new(heap_) 
      RewriteInstructionsItem(colElem, NULL, RC_OUTSIDE, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
    return rewrite;
  } 
  else if (extraHubID != NULL)
  {
    rewrite = new(heap_) 
      RewriteInstructionsItem(colElem, NULL, RC_EXTRAHUB, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
    rewrite->addExtraHubTable(extraHubID);
    return rewrite;
  }

  // If we get here, the column's table is in the MV hub.
  QROutputPtr mvCol = NULL;
  if (isProvidedColumn(colElem, mvCol))
  {
    rewrite = new(heap_) 
      RewriteInstructionsItem(colElem, mvCol, RC_PROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
    return rewrite;
  }

  if (isBackJoinableColumn(colElem))
  {
    rewrite = new(heap_) 
      RewriteInstructionsItem(colElem, NULL, RC_BACKJOIN, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
    rewrite->addBackJoinTable(&colElem->getTableID());
    rewrite->setSecondaryResultCode(RC_BACKJOIN);
    return rewrite;
  }

  if (fromExpr)
  {
    rewrite = isIndirectColumn(colElem);
    if (rewrite != NULL)
      return rewrite;
    // If not Indirect, fall through to failure.
  }

  // The column cannot be provided by the MV, so disqualify the MV.
  const NAString& colName = colElem->getFullyQualifiedColumnName();
  candidate_->logMVWasDisqualified1("column %s cannot be provided.", colName);

  return NULL;
}  // MatchTest::matchColumn()


// ***************************************************************************
// Check if this column of the query descriptor is an "Outside" column,
// which means that its a column of a table that is not covered by the MV.
// 1. If the column's table is covered by the MVMemo subgraph, its in the MV 
//    hub, so its definitly not an Outside column.
// 2. If the column's table is not covered by the entire MV, its definitly Outside.
// 3. Otherwise, it is covered by the MV's extra-hub tables, and we don't know 
//    yet if we can use those (maybe the join preds don't match). So return
//    TRUE for now, but also return the ID of the extra hub table. This will be
//    checked later, and if it does not match, the MV candidate will be disqualified.
// The results are cached in the MVCandidate object.
// ***************************************************************************
NABoolean MatchTest::isOutsideColumn(const QRColumnPtr colElem, const NAString*& extraHubID)
{
  const DescriptorDetailsPtr queryDetails = candidate_->getQueryDetails();
  const MVDetailsPtr mvDetails = candidate_->getMvDetails();
  QRJoinSubGraphPtr subGraph = candidate_->getJbbSubset()->getSubGraph();
  QRJoinGraphPtr joinGraph = subGraph->getParentGraph();

  const NAString& tableID =  colElem->getTableID();
  NABoolean isOutside = FALSE;
  // Check if we have already cached the result for this table.
  if (candidate_->probeCacheForTableID(&tableID, isOutside, extraHubID))
    return isOutside;

  // Both the column element and the join graph are from the query descriptor
  // So we can directly use the ID mapping.
  JoinGraphTablePtr tableNode = joinGraph->getTableByID(tableID);

  // If the table is covered by the common subgraph, it is definitly not Outside.
  if (tableNode != NULL && subGraph->contains(tableNode))
  {
    candidate_->cacheTableID(&tableID, FALSE, NULL);
    return FALSE;
  }

  // What if its covered by the MV extra-hub tables?
  // If so, its a different case - and should be put in the pendingWork queue
  // until we check the join preds to those tables.
  const NAString& tableName = queryDetails->getTableNameFromColumn(colElem);
  if (tableName == "")
  {
    // This table is in a JBB that was ommitted from the query descriptor,
    // because it has no MVs on it. So its definetly Outside.
    candidate_->cacheTableID(&tableID, TRUE, NULL);
    return TRUE;
  }

  // This is one of the few places we can lookup a table by its name, 
  // because we are not looking for a specific table instance, just existance,
  // so self-joins are not a problem.
  const QRTablePtr mvTable = mvDetails->getTableFromName(tableName);

  // If the table is not used by the MV at all - its RC_OUTSIDE.
  if (mvTable == NULL)
  {
    candidate_->cacheTableID(&tableID, TRUE, NULL);
    return TRUE;
  }
  // We get here when table table is not part of the matched subGraph, which 
  // is the MV Hub, but it is used by the MV, so it must be in the MV extra-hub.
  // Some more Pass 2 matching is needed to verify that its useful.
  extraHubID = &colElem->getTableID();
  candidate_->cacheTableID(&tableID, FALSE, extraHubID);
  return FALSE;
}  // MatchTest::isOutsideColumn()

// ***************************************************************************
// Is this column provided by the MV?
// ***************************************************************************
NABoolean MatchTest::isProvidedColumn(const QRColumnPtr colElem, 
				      QROutputPtr&	mvCol)
{
  const MVDetailsPtr mvDetails = candidate_->getMvDetails();

  // Get the MV base table object from the query table ID.
  const BaseTableDetailsPtr baseTable = 
    candidate_->getMvTableForQueryID(colElem->getTableID());

  // Look it up in the MV hash table.
  mvCol = mvDetails->getOutputByColumnName(baseTable->getTableElement()->getID(), 
					   colElem->getColumnName(), heap_);

  // The column is Provided if its an output column of the MV.
  return (mvCol !=  NULL);
}  // MatchTest::isProvidedColumn()

// ***************************************************************************
// Can the column be provided by using a back-join?
// ***************************************************************************
NABoolean MatchTest::isBackJoinableColumn(const QRColumnPtr colElem)
{
  const MVDetailsPtr mvDetails = candidate_->getMvDetails();
  const DescriptorDetailsPtr queryDetails = candidate_->getQueryDetails();

  // Get the MV base table object from the query table ID.
  const BaseTableDetailsPtr baseTable = candidate_->getMvTableForQueryID(colElem->getTableID());

  // Now check if the table is back-joinable (if its key columns are 
  // visible through the MV).
  return baseTable->getTableElement()->isKeyCovered();
}  // MatchTest::isBackJoinableColumn()

// *****************************************************************************
// This method is called only for columns that are inputs of query 
// output expressions. It checks if this column is an input of an 
// MV output expression, so there is a chance that the MV provides
// a sub-expression of the needed query expression.
// *****************************************************************************
RewriteInstructionsItemPtr MatchTest::isIndirectColumn(const QRColumnPtr  colElem)
{
  const MVDetailsPtr mvDetails = candidate_->getMvDetails();

  // Get the MV base table object from the query table ID.
  const BaseTableDetailsPtr baseTable = candidate_->getMvTableForQueryID(colElem->getTableID());

  // Now check if this column is used in the MV descriptor as an input to 
  // one of the output expressions
  const QROutputPtr mvOutput = 
    mvDetails->getOutputByInputColumns(baseTable->getTableElement()->getID(), 
				       colElem->getColumnName(), heap_);

  if (mvOutput == NULL)
    return NULL;
  else
  {
    // Keep the MV output expression for the Pass 2 algorithm.
    RewriteInstructionsItemPtr rewrite = new(heap_) 
      RewriteInstructionsItem(colElem, mvOutput, RC_INDIRECT, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
    return rewrite;
  }
}  // MatchTest::isIndirectColumn()

// ***************************************************************************
// Find if the list of input columns from a query expression are provided
// by the MV:
//    For all the input columns, call MatchOutputColumn()
//	If ANY of the input columns was blocked
//	  Return NULL
//	If ALL the input columns are Outside
//	  Return Outside
//      If ALL the inputs columns are Provided
//        Return a final NotProvided with the rewrite instructions.
//     Otherwise
//        Return an intermediate NotProvided with all the intermediate results.
//        Pass 2 will sort things out.
// ***************************************************************************
RewriteInstructionsItemPtr MatchTest::findExpressionInputs(const ElementPtrList& queryColList,
							   const QRElementPtr    queryElement) 
{
  RewriteInstructionsItemPtr rewrite = NULL;

  if (queryColList.entries() == 0)
  {
    // An expression with no inputs is NotProvided.
    rewrite = new(heap_) 
      RewriteInstructionsItem(queryElement, NULL, RC_NOTPROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
    rewrite->setSecondaryResultCode(RC_NO_INPUTS);
    return rewrite;
  }
		
  // Analyze the expresion inputs
  compositeMatchingResultsPtr intermediateResults = 
    new(heap_) compositeMatchingResults(ADD_MEMCHECK_ARGS(heap_));
  MatchInputColumnList(queryColList, intermediateResults, FALSE);

  // Now check the analysis results
  if (intermediateResults->wasBlockedColumn_)
  {
    // At least one of the expression inputs cannot be provided by the MV.
    // Disqualify the MV.
    candidate_->logMVWasDisqualified1("Expression %s cannot be provided.", queryElement->getID());
    deletePtr(intermediateResults);
    return NULL;
  }
  else if (intermediateResults->outsideInputs_.entries() == queryColList.entries())
  {
    // All the columns are RC_OUTSIDE
    // Pick the first one for the rewrite.
    rewrite = intermediateResults->outsideInputs_[0];
    rewrite->setQueryElement(queryElement);

    intermediateResults->outsideInputs_.removeAt(0);
  }
  else if (intermediateResults->providedInputs_.entries() == queryColList.entries())
  {
    // All the expression inputs are directly provided by the MV.
    rewrite = new(heap_) 
      RewriteInstructionsItem(queryElement, NULL, RC_NOTPROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
    rewrite->setSecondaryResultCode(RC_INPUTS_PROVIDED);

    // Copy the provided members to the result
    rewrite->addSubElements(intermediateResults->providedInputs_, FALSE);
    intermediateResults->providedInputs_.clear();
  }
  else
  {
    // The expression is NotProvided, which means it can probably be computed using the MV columns.
    // Give all the intermediate data to the Pass 2 algorithm.
    rewrite = new(heap_) 
      RewriteInstructionsItem(queryElement, NULL, RC_NOTPROVIDED, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
    rewrite->setSecondaryResultCode(RC_INPUTS_PROVIDED);

    rewrite->addSubElements(intermediateResults);
    intermediateResults->clear();
  }

  deletePtr(intermediateResults);
  return rewrite;
}  // MatchTest::findExpressionInputs()


// ***************************************************************************
// Match an equality set to an MV's output list.
// The rules are:
// 1. First match all the equality set columns
// 2. If ALL the the equality set members are Outside 
//      Then the entire equality set is a final Outside.
//      Pick the first member for the rewrite instructions.
// 3. If ANY of the members is a final Provided
//      Then the entire equality set is Provided.
//      Pick the first Provided member for the rewrite instructions.
// 4. If any of the members is a back-join or an extra-hub
//      The the equality set is an intermediate NotProvided.
// 5. Otherwise
//      Disquality the MV.
// ***************************************************************************
RewriteInstructionsItemPtr MatchTest::matchEqualitySet(const QRJoinPredPtr joinPred)
{
  const ElementPtrList& equalitySet = joinPred->getEqualityList();
  compositeMatchingResultsPtr intermediateResults = 
    new(heap_) compositeMatchingResults(ADD_MEMCHECK_ARGS(heap_));
  RewriteInstructionsItemPtr rewrite = NULL;

  // Analyze the equality set members
  MatchInputColumnList(equalitySet, intermediateResults, TRUE);

  // Check Outside first, because it had ALL semantics
  if (intermediateResults->outsideInputs_.entries() == equalitySet.entries())
  {
    // All the columns are RC_OUTSIDE
    // Pick the first one for the rewrite.
    rewrite = intermediateResults->outsideInputs_[0];
    rewrite->setQueryElement(joinPred);
    intermediateResults->outsideInputs_.removeAt(0);
  }
  // At least one of the columns in the equality set was not an Outside, so 
  // this is not an Outside result. Check the other options, this time using 
  // ANY semantics.
  else if (intermediateResults->providedInputs_.entries() > 0)
  {
    // We have at least one member that is Provided.
    // Pick the first one for the rewrite.
    rewrite = intermediateResults->providedInputs_[0];
    rewrite->setQueryElement(joinPred);

    intermediateResults->providedInputs_.removeAt(0);
  }
  else if (intermediateResults->backJoinInputs_.entries() > 0 ||
           intermediateResults->extrahubInputs_.entries() > 0)
  {
    // Problematic scenario TBD:
    // This equality set has no Provided members, but it does have back-join
    // and/or extra-hub members. When we defer the decision to Pass 2, and an 
    // extra-hub table cannot be verified, it will disqualify the MV, even 
    // though there may be other alternatives still open.
    // A possible solution may be to have a separate list of non-critical 
    // extra-hub tables, that do not disqualify the MV when fail to verify.

    // We have at least one member that is either Backjoinable or RC_EXTRAHUB.
    // Keep all the data for Pass 2 processing.
    rewrite = new(heap_) 
      RewriteInstructionsItem(joinPred, NULL, RC_NOTPROVIDED, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));

    // Copy the backjoinable members to the result
    rewrite->addSubElements(intermediateResults->backJoinInputs_, TRUE);
    // Clear them from the intermediate results so they don't get deleted at the end of this method.
    intermediateResults->backJoinInputs_.clear();

    // Copy the extrahub members to the result
    rewrite->addSubElements(intermediateResults->extrahubInputs_, TRUE);
    // Clear them from the intermediate results so they don't get deleted at the end of this method.
    intermediateResults->extrahubInputs_.clear();
  }
  else
  {
    // None of the equality set members is covered by the MV.
    candidate_->logMVWasDisqualified1("output equality set %s cannot be provided.", joinPred->getID());
    rewrite = NULL;
  }

  deletePtr(intermediateResults);
  return rewrite;

}  // MatchTest::matchEqualitySet()

// ***************************************************************************
// For a list of columns that can be either the list of exprerssion inputs, 
// or an equality set list of members, match every member of the list, and 
// arrange the results according to the result code in separate lists.
// ***************************************************************************
void MatchTest::MatchInputColumnList(const ElementPtrList&          queryColList, 
				     compositeMatchingResultsPtr    matchingResults,
				     NABoolean			    isAnEqualitySet) 
{
  // These lists are for collecting results from individual inputs.
  const MVDetailsPtr mvDetails = candidate_->getMvDetails();
  const DescriptorDetailsPtr queryDetails = candidate_->getQueryDetails();

  CollIndex numInputs = queryColList.entries();
  for (CollIndex i=0; i<numInputs; i++)
  {
    RewriteInstructionsItemPtr rewrite = NULL;

    QRElementPtr queryElem = queryColList[i]->getReferencedElement();

    switch(queryElem->getElementType())
    {
      case ET_Column:
      {
	QRColumnPtr queryInputCol = queryElem->downCastToQRColumn();

        if (isAnEqualitySet && queryDetails->isColumnFromLojTable(queryInputCol))
        {
          // This is a column of an LOJ table within a JoinPred.
          // Skip it in search of the other column.
          continue;
        }

	// Call the single column matching method.
	rewrite = matchColumn(queryInputCol, !isAnEqualitySet, isAnEqualitySet);
	break;
      }

      case ET_JoinPred:
      {
	// The input column is an equality set.
	assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                          !isAnEqualitySet, QRLogicException, 
			  "Equality sets should not reference other equality sets.");
	const QRJoinPredPtr joinPred = queryElem->downCastToQRJoinPred();
	rewrite = matchEqualitySet(joinPred);
	break;
      }

      case ET_Expr:
      {
	assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                          isAnEqualitySet, QRLogicException, 
			  "Expressions should not list other expressions as inputs.");
	// TBD - expressions as inputs of equality sets
	break;
      }

      default:
      {
	// Not expecting elements other than those above.
	assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                          FALSE, QRLogicException, 
			  "Unexpected input column list member."); 
      }
    } // case on element type

    if (rewrite == NULL)
    {
      // This input column is not provided by the MV. 
      // So the query output expression cannot be computed from this MV.
      matchingResults->wasBlockedColumn_ = TRUE;
      // If this is an expression input list - quit now because the expression cannot be provided.
      // If this is an equality set - continue because we can use some other set member.
      if (!isAnEqualitySet)
	return;
      else 
      {
        QRLogger::log(CAT_MATCHTST_MVDETAILS, LL_INFO,
                      "Ignore last disqualification message.");
        continue;
      }
    }

    matchingResults->insert(rewrite);

    // If this is an equality set, one provided column is all we need.
    if (rewrite->getResultCode() == RC_PROVIDED && isAnEqualitySet)
      return;

  } // for loop on input columns.

}  // MatchTest::MatchInputColumnList()


// ***************************************************************************
// Generate the QRMVColumn element for a provided column.
// ***************************************************************************
QRMVColumnPtr MatchTest::generateProvidedMvColumn(RewriteInstructionsItemPtr rewrite)
{
  // Construct the MVColumn element
  QRMVColumnPtr mvColumn = new (heap_) QRMVColumn(ADD_MEMCHECK_ARGS(heap_));
  // Get the name of the MV column from the MV output element.
  const QRElementPtr mvElem = rewrite->getMvElement();
  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                    mvElem != NULL, QRLogicException, 
		    "Expecting an MV element for a Provided output column.");

  const QROutputPtr mvCol = mvElem->downCastToQROutput();
  mvColumn->setMVColName(mvCol->getName());

  // If the query element is an Output element, reference the ID of the internal
  // column/expression/join-pred.
  QRElementPtr queryElem = rewrite->getActualQueryElement();
  mvColumn->setRef(queryElem->getID());

  // Leave the MV attribute for now. Hopefully we will not need it.
  return mvColumn;
}

// ***************************************************************************
// Generate the QRColumn element for a NotProvided column.
// ***************************************************************************
QRColumnPtr MatchTest::generateNotProvidedColumn(RewriteInstructionsItemPtr rewrite)
{
  // Construct the MVColumn element
  QRColumnPtr resultColumn = new (heap_) QRColumn(ADD_MEMCHECK_ARGS(heap_));
  // Get the name of the column from the query element.
  const QRElementPtr queryElem = rewrite->getActualQueryElement();
  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                    queryElem != NULL, QRLogicException, 
		    "Expecting a query element for a NotProvided output column.");
  resultColumn->setRef(queryElem->getID());

  if (queryElem->getElementType() == ET_Column)
  {
    const QRColumnPtr queryCol = queryElem->downCastToQRColumn();
    resultColumn->setFullyQualifiedColumnName(queryCol->getFullyQualifiedColumnName());
    resultColumn->setTableID(queryCol->getTableID());
  }
  else
  {
    assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                      queryElem->getElementType() == ET_JoinPred, QRLogicException, 
  		      "Expecting a either a Column or JoinPred here.");
    
    RewriteInstructionsItemPtr subRewrite = rewrite->getSubElements()->backJoinInputs_[0];
    assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                      subRewrite, QRLogicException, 
  		      "Expecting a back-join sub-element.");

    const QRElementPtr querySubElem = subRewrite->getQueryElement();
    const QRColumnPtr queryCol = querySubElem->downCastToQRColumn();
    resultColumn->setFullyQualifiedColumnName(queryCol->getFullyQualifiedColumnName());
  }

  // Leave the MV attribute for now. Hopefully we will not need it.
  return resultColumn;
}


// ***************************************************************************
// Match an expression (Pass 1). Here is the algorithm from 
// the IS document:
// 1. If the expression text is provided by the MV, and the expression's 
//    input columns match as well
//      Return Provided.
// 2. If all the input columns are provided by the MV
//      Return NotProvided.
// 3. Otherwise, return NULL.
//
// ***************************************************************************
RewriteInstructionsItemPtr MatchTest::matchExpression (const QRExprPtr expr) 
{
  RewriteInstructionsItemPtr rewrite = NULL;

  // 1. Check if the MV has an output expression with matching text
  //    Skip this test for rollup aggregate expressions.
  if (candidate_->getAggregateMatchingType() != AMT_MAV_AQ_DG ||
      expr->getExprRoot()->containsAnAggregate() == FALSE)
  {
    rewrite = findMatchingMvExpression(expr->getExprRoot());
    if (rewrite != NULL)
    {
      rewrite->setQueryElement(expr);
      return rewrite;
    }
  }

  // 2. A matching MV output expression was not found. 
  // Check if at least the input columns are Provided.
  const ElementPtrList& queryInputs = expr->getInputColumns(heap_);
  if (queryInputs.entries() == 0)
  {
    // Sometimes expressions have no column inputs, like COUNT(*).
    switch (candidate_->getAggregateMatchingType())
    {
      case AMT_MAV_AQ_MG:
        // If this is a matching MAV, fail because it should have matched a MAV aggregate expression.
        candidate_->logMVWasDisqualified1("output expression %s is not Provided.", 
					  expr->getID().data());
        return NULL;

      case AMT_MJV_AQ:
        // If its on an MJV, its a Final Outside match, because it can be computed on the MJV.
        rewrite = new(heap_) 
          RewriteInstructionsItem(expr, NULL, RC_OUTSIDE, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
        break;

      case AMT_MAV_AQ_DG:
        // If this is a rollup matching case, defer to Pass 2.
        rewrite = new(heap_) 
          RewriteInstructionsItem(expr, NULL, RC_NOTPROVIDED, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
        if (expr->getExprRoot()->containsAnAggregate())
          rewrite->setSecondaryResultCode(RC_ROLLUP);
        break;
      case AMT_MJV_JQ:
        candidate_->logMVWasDisqualified1("output expression %s is not Provided.", 
					  expr->getID().data());
        break;
    }
  }
  else
  {
    // The expression has inputs. Is it an aggregate expression on a MAV?
    if (expr->getExprRoot()->containsAnAggregate() && 
        candidate_->getAggregateMatchingType() != AMT_MJV_AQ)
    {
      // Yes, its an aggregate expression. Deal with it in Pass 2.
      rewrite = new(heap_) 
        RewriteInstructionsItem(expr, NULL, RC_NOTPROVIDED, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
      if (candidate_->getAggregateMatchingType() == AMT_MAV_AQ_DG)
        rewrite->setSecondaryResultCode(RC_ROLLUP);
    }
    else
    {
      // No. Try to find its input columns.
      rewrite = findExpressionInputs(queryInputs, expr);
    }
  }

  return rewrite;
}  // MatchTest::matchExpression()

// ***************************************************************************
// Check if the MV provides a matching output expression, including its 
// input columns.
// ***************************************************************************
RewriteInstructionsItemPtr MatchTest::findMatchingMvExpression(const QRExplicitExprPtr expr) 
{
  // Check if the MV has an output expression with matching text
  const MVDetailsPtr mvDetails = candidate_->getMvDetails();
  const DescriptorDetailsPtr queryDetails = candidate_->getQueryDetails();

  NAString exprText(heap_);
  expr->unparse(exprText, FALSE);
  const QROutputPtr mvOutput1 = mvDetails->getOutputByExprText(exprText);
  if (mvOutput1 == NULL)
    return NULL;

  // MV output expression found. Now lets match its input columns.
  ElementPtrList queryColList(heap_);
  expr->getInputColumns(queryColList, heap_);

  // The MV may have several output expressions with the same normalized 
  // expression text, but different input columns. We need to loop on them, 
  // so if one is disqualified, we can check the others.
  const QRElementHash& outputByExprText = mvDetails->getOutputByExprTextHash();
  QRElementHashIterator iterator(outputByExprText, &exprText);

  // Loop on the iterator on all the output expressions with the same expression text.
  const NAString* key; // we don't really use this - it will always be equal to exprText.
  QRElementPtr mvOutputElem;
  for ( CollIndex j=0; j<iterator.entries(); j++) 
  {
    iterator.getNext (key, mvOutputElem); 
    const QROutputPtr mvOutput = mvOutputElem->downCastToQROutput();

    // Get to the actual MV expression
    QRExprPtr mvExpr = mvOutput->getOutputItem()->getReferencedElement()->downCastToQRExpr();

    // Now match the expression's input lists
    if (matchExpressionInputs(queryColList, mvExpr->getInputColumns(heap_)))
    {
      // This expression matched just fine.
      RewriteInstructionsItemPtr rewrite = NULL;

      // Let's see if we can also find rewrite instructions for the individual input columns
      if (queryColList.entries() == 0)
      {
	// This expression has no input columns (maybe a COUNT(*)? )
	rewrite = new(heap_) 
	  RewriteInstructionsItem(expr, mvOutput, RC_PROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));  
      }
      else
      {
	// Find if the input columns are Provided.
	rewrite = findExpressionInputs(queryColList, expr);

        if (rewrite != NULL)
        {
           // Verify that all the input columns are Provided. 
           compositeMatchingResultsPtr subElements  = rewrite->getSubElements();
           if (subElements->entries() != subElements->providedInputs_.entries())
           {
             // Otherwise ignore the input columns (and use only the entire expression).
             deletePtr(rewrite);
             rewrite = NULL;
           }
        }

	if (rewrite == NULL)
	{
	  // No - Not all input columns are Provided. No big deal, just return
	  // the rewrite instructions for the entire expression.
          // Undo the disqualification of the MV when finding expression inputs.
          candidate_->reQualify();
	  rewrite = new(heap_) 
	    RewriteInstructionsItem(expr, mvOutput, RC_PROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
	}
	else
	{
	  // Yes, Combine the two.
	  rewrite->setMvElement(mvOutput);
	  rewrite->setResultCode(RC_PROVIDED);
	  rewrite->setResultStatus(RS_FINAL);
	}
      }
      return rewrite;
    }
    // This expression failed to match on the input list. continue to next expression.

  } // for on iterator.

  // All the expressions with the matching text failed on their inputs.
  return NULL;

}  // MatchOutput::findMatchingMvExpression()

// ***************************************************************************
// This method accepts two expression input lists as inputs: one from the query 
// and one from the MV. Both are from expressions with the same normalized predicate
// expression text. This method checks if the list of input columns match as well.
// This method returns TRUE when a match is found, anf FALSE otherwise.
// ***************************************************************************
NABoolean MatchTest::matchExpressionInputs(const ElementPtrList& queryInputColumns,
					   const ElementPtrList& mvInputColumns)
{
  // The input lists must have the same number of entries to match.
  if (queryInputColumns.entries() != mvInputColumns.entries())
    return FALSE;
 
  CollIndex maxEntries = mvInputColumns.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    // The MV and query input lists must be ordered by their appearance in the predicate expression.
    // The matchByName() method is only implemented for the QRColumn and QRJoinPred classes.
    QRColumnPtr   queryColumn = queryInputColumns[i]->getReferencedElement()->downCastToQRColumn();
    QRJoinPredPtr query_jp    = candidate_->getQueryDetails()->getJoinPred(queryColumn);
    QRColumnPtr   mvColumn    = mvInputColumns[i]->getReferencedElement()->downCastToQRColumn();
    QRJoinPredPtr mv_jp       = candidate_->getQueryDetails()->getJoinPred(mvColumn);

    if (query_jp == NULL)
    {
      // If the query input has no equi-join preds on it, the MV input
      // must not have any either.
      // So if the MV input column is a join pred, it must be a different expression.
      if (mv_jp != NULL)
	return FALSE;

      if (matchColumnsByNameAndID(queryColumn, mvColumn) == FALSE)
	return FALSE;
    }
    else if (query_jp)
    {
      // Lets check the MV input.
      if (mv_jp != NULL)
      {
	// Its a join pred. Since in case of a match, the query join pred
	// must be a superset of the MV join pred, we can just pick the first 
	// column in the join pred, and match it to any column in the query equality set.
	const ElementPtrList& mvEqualityList = mv_jp->getEqualityList();
	mvColumn = mvEqualityList[0]->downCastToQRColumn();
      }

      // OK, we have the MV input column, now match it against the columns 
      // in the query equality set.
      const ElementPtrList& queryEqualityList = query_jp->getEqualityList();
      CollIndex maxEntries = queryEqualityList.entries();
      NABoolean matchFound = FALSE;
      for (CollIndex j=0; j<maxEntries; j++)
      {
	const QRColumnPtr queryColumn = queryEqualityList[j]->getReferencedElement()->downCastToQRColumn();
	if (matchColumnsByNameAndID(queryColumn, mvColumn))
	{
	  // Found a match for this input. Break from this loop, and continue
	  // with the next input.
	  matchFound = TRUE;
	  break;
	}
      }
      // Match not found for this input. 
      if (matchFound == FALSE)
	return FALSE;
    }
    else
    {
      return FALSE;
    }
  } // for
  return TRUE;
}  // MatchTest::matchExpressionInputs()

// ***************************************************************************
// Match an MV column to a query column by table ID and column name.
// Since the two columns are from different descriptors, the query table ID
// must first be translated to the corresponding table ID from the MV descriptor.
// ***************************************************************************
NABoolean MatchTest::matchColumnsByNameAndID(const QRColumnPtr queryColumn, const QRColumnPtr mvColumn)
{
  // Match the column name first.
  if (queryColumn->getColumnName() != mvColumn->getColumnName())
    return FALSE;

  // Now check the table IDs.
  const NAString& queryTableID = queryColumn->getTableID();
  const NAString& mvTableID    = mvColumn->getTableID();
  // Translate the query tableID to the corresponding ID from the MV descriptor.
  const BaseTableDetailsPtr baseTable = candidate_->getMvTableForQueryID(queryTableID, FALSE);
  if (baseTable == NULL)
    return FALSE;

  // Are they the same?
  return (baseTable->getID() == mvTableID);
} // MatchTest::matchColumnsByNameAndID()

// ***************************************************************************
// Add the Provided and BackJoin inputs as outputs of the MV candidate.
// In case the optimizer will not be able to use the Provided expression, 
// it will try to use the input columns.
// ***************************************************************************
void MatchTest::addInputsToOutputList(RewriteInstructionsItemPtr rewrite)
{
  if (candidate_->getAggregateMatchingType() == AMT_MAV_AQ_DG)
    return;

  RewriteInstructionsItemList& providedList = rewrite->getSubElements()->providedInputs_;
  CollIndex maxEntries = providedList.entries();
  RewriteInstructionsItemPtr columnRewrite = NULL;
  for (CollIndex i=0; i<maxEntries; i++)
  {
    candidate_->addOutputColumn(providedList[i]);
  }

  if (rewrite->getResultCode() == RC_PROVIDED)
    return;

  RewriteInstructionsItemList& backJoinList = rewrite->getSubElements()->backJoinInputs_;
  maxEntries = backJoinList.entries();
  for (CollIndex j=0; j<maxEntries; j++)
  {
    candidate_->addOutputColumn(backJoinList[j]);
  }

  RewriteInstructionsItemList& extraHubList = rewrite->getSubElements()->extrahubInputs_;
  maxEntries = extraHubList.entries();
  for (CollIndex k=0; k<maxEntries; k++)
  {
    candidate_->addOutputColumn(extraHubList[k]);
  }

  // If the expression itself is not Provided, we may be able to skip it
  // and leave only the input columns.
  // We can do that if some of its inputs are Outside, or if its an
  // aggregate expression on top of an MJV.
  QRExprPtr expr = NULL;
  QRElementPtr queryElem = rewrite->getQueryElement();
  if (queryElem->getElementType()== ET_RangePred)
  {
    QRRangePredPtr pred = static_cast<QRRangePredPtr>(queryElem);
    expr = pred->getRangeItem()->getReferencedElement()->downCastToQRExpr();
  }
  else
  {
    expr = rewrite->getActualQueryElement()->downCastToQRExpr();
  }

  if (rewrite->getSubElements()->outsideInputs_.entries() > 0 ||
      ( expr->getExprRoot()->containsAnAggregate() &&
	candidate_->getAggregateMatchingType() == AMT_MJV_AQ) )
  {
    rewrite->setResultCode(RC_SKIP_ME);
  }
}  //  addInputsToOutputList()

// ***************************************************************************
// Use a visitor to find aggregate functions, and solve any that are found.
// ***************************************************************************
RewriteInstructionsItemPtr MatchTest::findInternalAggregateFunctions(QRExplicitExprPtr treeExpr)
{
  AggregateCollectorVisitorPtr visitor = new(heap_) 
    AggregateCollectorVisitor(ADD_MEMCHECK_ARGS(heap_));
  treeExpr->treeWalk(visitor);

  RewriteInstructionsItemPtr rewrite = new(heap_) 
    RewriteInstructionsItem(treeExpr, NULL, RC_NOTPROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));

  if (candidate_->getAggregateMatchingType() == AMT_MAV_AQ_MG)
  {
    rewrite->setSecondaryResultCode(RC_AGGR_EXPR);
  }
  else
  {
    rewrite->setSecondaryResultCode(RC_ROLLUP_EXPR);
  }

  // Try to find all the aggregate functions used by the expression.
  FunctionNodesList& aggrFunctions = visitor->getAggregateFunctionList();
  CollIndex maxEntries = aggrFunctions.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    // Look for the corresponding aggregate function as an MV output.
    RewriteInstructionsItemPtr funcRewrite = findAggregateFunction(aggrFunctions[i]);
    if (funcRewrite == NULL)
    {
      deletePtr(visitor);
      deletePtr(rewrite);
      return NULL;
    }

    // Add it to the expression rewrite instructions.
    rewrite->addSubElement(funcRewrite);
  } // for

  // Check for any columns used in the expressions outside the aggregate functions.
  ColumnNodesList& simpleCols = visitor->getSimpleColumnList();
  maxEntries = simpleCols.entries();
  for (CollIndex j=0; j<maxEntries; j++)
  {
    QRColumnPtr col = simpleCols[j]->getReferencedElement()->downCastToQRColumn();
    RewriteInstructionsItemPtr colRewrite = matchColumn(col, FALSE); 
    if (colRewrite != NULL && !colRewrite->isFinal())
    {
      deletePtr(colRewrite);
      colRewrite = NULL;
    }
    if (colRewrite == NULL)
    {
      deletePtr(visitor);
      deletePtr(rewrite);
      return NULL;
    }

    // Add it to the expression rewrite instructions.
    rewrite->addSubElement(colRewrite);
  }

  // Check for any join preds used in the expressions outside the aggregate functions.
  JoinPredNodesList& joinedCols = visitor->getJoinedColumnList();
  maxEntries = joinedCols.entries();
  for (CollIndex k=0; k<maxEntries; k++)
  {
    QRJoinPredPtr col = joinedCols[k]->getReferencedElement()->downCastToQRJoinPred();
    RewriteInstructionsItemPtr colRewrite = matchEqualitySet(col); 
    if (colRewrite != NULL && !colRewrite->isFinal())
    {
      deletePtr(colRewrite);
      colRewrite = NULL;
    }
    if (colRewrite == NULL)
    {
      deletePtr(visitor);
      deletePtr(rewrite);
      return NULL;
    }

    // Add it to the expression rewrite instructions.
    rewrite->addSubElement(colRewrite);
  }

  deletePtr(visitor);

  return rewrite;
}

// ***************************************************************************
// ***************************************************************************
RewriteInstructionsItemPtr MatchTest::findAggregateFunction(QRFunctionPtr func)
{
  RewriteInstructionsItemPtr rewrite = NULL;

  // Collect the function's input columns.
  ElementPtrList inputList(heap_);
  QRExplicitExprPtr funcArgument = func->getArguments()[0];
  funcArgument->getInputColumns(inputList, heap_, TRUE);

  // Is this a rollup case?
  if (candidate_->getAggregateMatchingType() == AMT_MAV_AQ_DG)
  {
    // Check for COUNT(*) as a special case
    if (func->getAggregateFunc() == QRFunction::AFT_COUNTSTAR)
    {
      rewrite = new(heap_) 
        RewriteInstructionsItem(func, NULL, RC_NOTPROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
      rewrite->setSecondaryResultCode(RC_ROLLUP_ON_GROUPING);
      return rewrite;
    }

    // Maybe its a rollup on a grouping column?
    if (areAllInputsGroupingColumns(func, inputList))
    {
      // All the input columns are MV grouping columns.
      // Now check that they are Provided by the MV.
      rewrite = findExpressionInputs(inputList, func);
      if (rewrite == NULL)
        return NULL;

      if (!rewrite->isFinal())
      {
        deletePtr(rewrite);
        return NULL;
      }

      rewrite->setSecondaryResultCode(RC_ROLLUP_ON_GROUPING);
      return rewrite;
    }
    else
    {
      // Rollup, but not on MV grouping columns.
      // Do not allow distinct aggregate functions to match.
      if (func->isDistinctAggregate() )
      {
        candidate_->logMVWasDisqualified1("DISTINCT aggregate function %s cannot be rolled up.",
				          func->getID().data());
        return NULL;
      }
    }
  }  // if (rollup)

  // We get here if its not a rollup case, or if its a rollup case but
  // not on MV grouping columns.
  // Look for the corresponding aggregate function as an MV output.
  rewrite = findMatchingMvExpression(func);
  if (rewrite != NULL)
  {
    if (rewrite->isFinal())
      return rewrite;
    else
      deletePtr(rewrite);
  }

  // Last try - maybe this is a COUNT(col) on a NOT NULL column,
  // so we can use COUNT(*) instead?
  QROutputPtr countStarOutput = candidate_->getMvDetails()->getCountStar();
  if (func->getAggregateFunc() == QRFunction::AFT_COUNT && countStarOutput != NULL)
  {
    // OK, this is a COUNT() function, and the MV has a COUNT(*) output.
    // Check that all the input columns are not nullable.
    if (func->isCountStarEquivalent(heap_))
    {
      // Yes! - we can use COUNT(*) instead.
      rewrite = new(heap_) 
        RewriteInstructionsItem(func, countStarOutput, RC_PROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
      return rewrite;
    }
  }

  // Give up on this aggregate function.
  candidate_->logMVWasDisqualified1("aggregate function %s cannot be matched.",
				    func->getID().data());
  return NULL;
}

// ***************************************************************************
// The extra grouping columns are the ones being rolled up – columns that 
// are in the MV’s grouping list, but not in the query’s grouping list. An 
// aggregate function is on an MV grouping column when its inputs are in this list.
// ***************************************************************************
NABoolean MatchTest::areAllInputsGroupingColumns(QRFunctionPtr func, ElementPtrList& inputList)
{
  // Are all the inputs MV grouping columns?
  for (CollIndex i=0; i<inputList.entries(); i++)
  {
    const QRColumnPtr inputCol = inputList[i]->getFirstColumn();
    if (!isExtraGroupingColumn(inputCol))
      return FALSE;
  }

  return TRUE;
}

// ***************************************************************************
// Generate a result QRExpr element for an expression involving aggregate functions.
// The rewrite instructions for the individual aggregate functions have already
// been prepared as Provided subElements, by the Pass 2 code.
// First we go over the list of subElements, creating an MVColumn element for
// each aggregate function used in the expression. Each MVColumn element is 
// inserted into a hash table, with the ID of the corresponding query sub-expression
// as the key. Then we make a deep copy of the query expression tree, and in
// the process, switch every sub-expression whose ID we find in the hash table
// with the corresponding MVColumn element.
// Finaly, the output expression itself is created, referencing the query one.
// ***************************************************************************
QRExprPtr MatchTest::generateAggregateExpressionOutput(RewriteInstructionsItemPtr rewrite)
{
  // Init the hash table.
  subExpressionRewriteHash subExprHash(hashKey, INIT_HASH_SIZE_SMALL, FALSE, heap_);

  compositeMatchingResultsPtr subExprs = rewrite->getSubElements();
  // Verify all the subElements are Provided.
  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                       subExprs->entries() == subExprs->providedInputs_.entries()
                                         + subExprs->notProvidedInputs_.entries()
                                         + subExprs->backJoinInputs_.entries(),
                       QRLogicException, 
                      "Expecting all subexpressions to be Provided, NotProvided, or back join.");
  RewriteInstructionsItemList rewriteList(heap_);
  rewriteList.insert(subExprs->providedInputs_);
  rewriteList.insert(subExprs->notProvidedInputs_);
  rewriteList.insert(subExprs->backJoinInputs_);

  // For each subElement
  CollIndex maxEntries = subExprs->entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    RewriteInstructionsItemPtr subExprRewrite = rewriteList[i];
    const NAString& key = subExprRewrite->getQueryElement()->getID();
    QRExplicitExprPtr exprRewrite = NULL;

    if (candidate_->getAggregateMatchingType() == AMT_MAV_AQ_DG)
    {
      if (subExprRewrite->getQueryElement()->getElementType() == ET_Function)
      {
        // This is a rolled-up aggregate function.
        if (subExprRewrite->getSecondaryResultCode() != RC_ROLLUP_ON_GROUPING)
        {
          // This is a normal Rollup case. Additional aggregation is needed above the MVColumn.
          exprRewrite = generateRollupAggregateFunction(subExprRewrite);
        }
        else
        {
          // This rollup over a grouping column. 
          exprRewrite = generateRollupOverGroupingAggregateFunction(subExprRewrite);
        }
      }
      else
      {
        // This is a grouping column used in the expression.
        exprRewrite = generateProvidedMvColumn(subExprRewrite);
      }
    }
    else
    {
      // This is a matching grouping list. Every matched sub-expression
      // is a Provided MVColumn.
      if (subExprRewrite->getSecondaryResultCode() == RC_BACKJOIN)
        exprRewrite = generateNotProvidedColumn(subExprRewrite);
      else
        exprRewrite = generateProvidedMvColumn(subExprRewrite);
    }

    if (exprRewrite == NULL)
    {
      cleanupSubExprHash(subExprHash);
      return NULL;
    }
    subExprHash.insert(&key, exprRewrite);
  }

  // Create the result expression tree from the query expression tree, with 
  // the needed sub-expressions switched to MVColumn elements.
  QRExprPtr queryExpr = rewrite->getActualQueryElement()->getReferencedElement()->downCastToQRExpr();
  QRExplicitExprPtr resultTreeExpr = NULL;
  try
  {
    resultTreeExpr = queryExpr->getExprRoot()->deepCopyAndSwitch(subExprHash, heap_);
  }
  catch(QRDescriptorException ex)
  {
    // We get here if we cannot properly rewrite all the query column elements.
    // Release objects stored in subExprHash before returning.
    cleanupSubExprHash(subExprHash);
    return NULL;
  }

  // Create the expression on top of it.
  QRExprPtr expr = new(heap_) QRExpr(FALSE, ADD_MEMCHECK_ARGS(heap_));
  expr->setExprRoot(resultTreeExpr);
  expr->setRef(queryExpr->getID());

  return expr;
}  // generateAggregateExpressionOutput()

/****************************************************************************/
/****************************************************************************/
void MatchTest::cleanupSubExprHash(subExpressionRewriteHash& subExprHash)
{
  subExpressionRewriteHashIterator iter(subExprHash);
  const NAString* key;
  QRExplicitExprPtr value;
  for (CollIndex x = 0; x < iter.entries(); x++) 
  {
    // Get the next sub expression.
    iter.getNext(key, value); 
    deletePtr(value);
  }
}

/****************************************************************************/
/****************************************************************************/
NABoolean MatchTest::isExtraGroupingColumn(QRColumnPtr col)
{
  GroupingList*	extras = candidate_->getExtraGroupingColumns();
  CollIndex maxEntries = extras->entries();

  for (CollIndex i=0; i<maxEntries; i++)
  {
    QRElementPtr elem = (*extras)[i]->getReferencedElement();

    // Handle simple columns.
    if (elem->getElementType() == ET_Column)
    {
      QRColumnPtr groupingCol = elem->downCastToQRColumn();
      if (matchColumnsByNameAndID(col, groupingCol))
        return TRUE;
    }

    // Handle equality sets - check every element in the set.
    if (elem->getElementType() == ET_JoinPred)
    {
      const QRJoinPredPtr mvJoinPred = elem->downCastToQRJoinPred();
      const ElementPtrList& mvEqualityList = mvJoinPred->getEqualityList();
      CollIndex eqSetSize = mvEqualityList.entries();
      for (CollIndex j=0; j<eqSetSize; j++)
      {
	const QRElementPtr eqElement = mvEqualityList[j];
        if (eqElement->getElementType() != ET_Column)
          continue;
        const QRColumnPtr eqCol = eqElement->downCastToQRColumn();
        if (matchColumnsByNameAndID(col, eqCol))
          return TRUE;
      }
    }
  }

  return FALSE;
}


// ***************************************************************************
// ***************************************************************************
QRFunctionPtr MatchTest::generateRollupAggregateFunction(RewriteInstructionsItemPtr rewrite)
{
  // This is the query aggregate function.
  QRElementPtr queryElem = rewrite->getActualQueryElement();
  QRFunctionPtr queryFunction;
  if (queryElem->getElementType() == ET_Expr)
  {
    queryFunction = queryElem->downCastToQRExpr()->getExprRoot()->downCastToQRFunction();
  }
  else
  {
    assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                      queryElem->getElementType() == ET_Function, QRLogicException, 
    		      "Expecting either a QRExpr or QRFunction.");
    queryFunction = queryElem->downCastToQRFunction();
  }

  NAString aggrName(heap_);
  QRFunction::AggregateFunctionType aggrType = QRFunction::AFT_NONE;
  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                    !queryFunction->isDistinctAggregate(), QRLogicException, 
		    "DISTINCT aggregate fucttions cannot be rolled up.");

  switch (queryFunction->getAggregateFunc())
  {
    case QRFunction::AFT_SUM:
    case QRFunction::AFT_MIN:
    case QRFunction::AFT_MAX:
    case QRFunction::AFT_ONEROW:
    case QRFunction::AFT_ANY_TRUE:
      // SUM, MIN and MAX, as well as ONEROW and ANY_TRUE
      // use the same aggregate function in the rollup.
      aggrName = queryFunction->getFunctionName();
      aggrType = queryFunction->getAggregateFunc();
      break;

    case QRFunction::AFT_COUNT:
    case QRFunction::AFT_COUNTSTAR:
      // COUNT needs SUM to be calculated in the rollup.
      aggrName = "sum";
      aggrType = QRFunction::AFT_SUM;
      break;

    case QRFunction::AFT_ONE_TRUE:
      // ONE_TRUE needs ANY_TRUE to be calculated in the rollup.
      aggrName = "any_true";
      aggrType = QRFunction::AFT_ANY_TRUE;
      break;

    case QRFunction::AFT_ONE_ROW:
      // ONE_ROW is the old implementation of ONEROW, and cannot be rolled up.
      assertLogAndThrow1(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                         FALSE, QRLogicException, 
	                 "Aggregate function %s cannot be rolled up.", queryFunction->getFunctionName().data());
      break;
  }

  // Create the new aggregate function.
  QRFunctionPtr rollupFunction = new(heap_) QRFunction(ADD_MEMCHECK_ARGS(heap_));
  rollupFunction->setFunctionName(aggrName);
  rollupFunction->setAggregateFunc(aggrType);
  // Use the MV column as its argument.
  QRMVColumnPtr mvColumn = generateProvidedMvColumn(rewrite);
  rollupFunction->addArgument(mvColumn);

  return rollupFunction;
}  // generateRollupAggregateFunction()

// ***************************************************************************
// ***************************************************************************
QRFunctionPtr MatchTest::generateRollupOverGroupingAggregateFunction(RewriteInstructionsItemPtr rewrite)
{
  // This is the query aggregate function.
  QRElementPtr queryElem = rewrite->getActualQueryElement();
  QRFunctionPtr queryFunction;
  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                    queryElem->getElementType() == ET_Function, QRLogicException, 
    		    "Expecting a QRFunction in generateRollupOverGroupingAggregateFunction().");
  queryFunction = queryElem->downCastToQRFunction();
  // Which aggregate should we use?
  NAString aggrName(heap_);
  QRFunction::AggregateFunctionType aggrType = QRFunction::AFT_NONE;

  // Create the new aggregate function.
  QRFunctionPtr rollupFunction = new(heap_) QRFunction(ADD_MEMCHECK_ARGS(heap_));
  NABoolean needCountStarArgument = FALSE;
  NABoolean needActualArgument = TRUE;

  switch (queryFunction->getAggregateFunc())
  {
    case QRFunction::AFT_COUNTSTAR:
      // COUNT(*) translates into SUM(count_star)
      aggrType = QRFunction::AFT_SUM;
      aggrName = "sum";
      needCountStarArgument = TRUE;
      needActualArgument = FALSE;
      break;

    case QRFunction::AFT_COUNT:
      // COUNT(g) translates into COUNT( CASE WHEN g IS NULL THEN 0 ELSE count_star END )
      // Let the analyzer construct this expression, just instruct it to do it
      // by using this special aggregate type.
      aggrType = QRFunction::AFT_COUNT_ON_GROUPING;
      aggrName = "count on grouping";
      needCountStarArgument = TRUE;
      break;

    case QRFunction::AFT_SUM:
      // SUM(g) translates into SUM(g*count_star)
      // Let the analyzer construct this expression, just instruct it to do it
      // by using this special aggregate type.
      aggrType = QRFunction::AFT_SUM_ON_GROUPING;
      aggrName = "sum on grouping";
      needCountStarArgument = TRUE;
      break;

    case QRFunction::AFT_MIN:
    case QRFunction::AFT_MAX:
      // MIN(g) and MAX(g) are regular MIN/MAX.
    case QRFunction::AFT_COUNT_DISTINCT:
      // COUNT(DISTINCT g) can be rolled up, and stays the same.
    case QRFunction::AFT_SUM_DISTINCT:
      // SUM(DISTINCT g) can be rolled up, and stays the same.
      aggrName = queryFunction->getFunctionName();
      aggrType = queryFunction->getAggregateFunc();
      break;
  }

  rollupFunction->setFunctionName(aggrName);
  rollupFunction->setAggregateFunc(aggrType);

  if (needActualArgument)
  {
    QRElementPtr queryArg = queryFunction->getArguments()[0]->getReferencedElement();
    if (queryArg->getElementType() == ET_Column ||
        queryArg->getElementType() == ET_JoinPred )
    {
      // The argument to the aggregate function is a simple column.
      // Use the MV column as its argument.
      RewriteInstructionsItemPtr colRewrite = rewrite->getSubElements()->providedInputs_[0];
      assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                        colRewrite != NULL, QRLogicException, 
   		        "Expecting a rewrite of the aggregate function argument.");
      QRMVColumnPtr mvColumn = generateProvidedMvColumn(colRewrite);
      rollupFunction->addArgument(mvColumn);
    }
    else
    {
      candidate_->logMVWasDisqualified("Can't handle aggr(expr(g)) yet, for rollup over grouping column.");
      deletePtr(rollupFunction);
      return NULL;
    }
  }

  if (needCountStarArgument)
  {
    // Find the MV COUNT(*) column.
    QROutputPtr countStarOutput = candidate_->getMvDetails()->getCountStar();
    if (countStarOutput == NULL)
    {
       candidate_->logMVWasDisqualified("the MV does not have a COUNT(*) column.");
       deletePtr(rollupFunction);
       return NULL;
    }

    // Construct the COUNT(*) MVColumn element
    QRMVColumnPtr countStarMVCol = new (heap_) QRMVColumn(ADD_MEMCHECK_ARGS(heap_));
    // Get the name of the MV column from the MV output element.
    countStarMVCol->setMVColName(countStarOutput->getName());
    // Add it as the first argument.
    rollupFunction->addArgument(countStarMVCol);
  }

  return rollupFunction;
}  // generateRollupOverGroupingAggregateFunction()

//========================================================================
//  Class MatchOutput
//========================================================================


// ***************************************************************************
// Do the initial Pass 1 analysis of all output columns/expressions.
// For each output element, this method finds the actual column or expression
// And calls the next methods to match it.
// An output element can fall into one of the following options:
// 1. A QRColumn element, or a reference to one.
//    Call matchColumn().
// 2. An expression (QRExpr) or a reference to one.
//    Call matchExpression().
// 3. A reference to an equality set.
//    Call matchEqualitySet().
// When any of these methods rturn FALSE, it means that the output element 
// cannot be provided using this MV, so the MV is disqualified, and its 
// matching is aborted.
// ***************************************************************************
NABoolean MatchOutput::matchPass1() 
{
  // Loop over the outputs in the Query descriptor
  const DescriptorDetailsPtr queryDetails = candidate_->getQueryDetails();
  const QRJBBPtr  queryJbb = candidate_->getQueryJbb();
  const QROutputListPtr outputList = queryJbb->getOutputList();

  CollIndex outputListEntries = (outputList ? outputList->entries() : 0);
  for (CollIndex i=0; i<outputListEntries; i++)
  {
    RewriteInstructionsItemPtr rewrite = NULL;
    const QROutputPtr output = (*outputList)[i];
    QRElementPtr element = output->getOutputItem()->getReferencedElement();

    // Now check what is this element that we need to match.
    switch (element->getElementType())
    {
      case ET_Column:
      {
	// Case 1: A QRColumn element.
	const QRColumnPtr col = element->downCastToQRColumn();
	rewrite = matchColumn(col, FALSE);
	break;
      }

      case ET_Expr:
      {
	// Case 2: A QRExpr element.
	const QRExprPtr expr = element->downCastToQRExpr();
	rewrite = matchExpression(expr);
	if (rewrite != NULL) // && rewrite->isFinal())
	{
	  // For output expressions, add the input columns to the output list as well.
	  addInputsToOutputList(rewrite);
	}
	break;
      }

      case ET_JoinPred:
      {
	// Case 3 - A join predicate (Equality set).
	const QRJoinPredPtr joinPredElement = element->downCastToQRJoinPred();
	rewrite = matchEqualitySet(joinPredElement);
	break;
      }

      default:
      {
	// Output elements can only be QRColumn, QRExpr or referencing QRJoinPred.
	assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                          FALSE, QRLogicException, 
			  "Unexpected output list element."); 
	return FALSE;
      }
    } // switch on element type.

    // Lets check the matching results
    if (rewrite == NULL)
    {
      // Reject the MV.
      return FALSE;
    }
    else
    {
      // Matching successful. Add the rewrite instructions to the list.
      rewrite->setQueryElement(output);
      addRewriteInstructions(rewrite);
    }

  } // for loop on query outputs
  return TRUE;
}  // MatchOutput::matchPass1()

// ***************************************************************************
// ***************************************************************************
NABoolean MatchOutput::matchPass2OnElement(RewriteInstructionsItemPtr pending)
{
  if (pending->getActualQueryElement()->getElementType() != ET_Expr)
  {
    // simple column outputs are not supposed to get here.
    // TBD: Change to assertLogAndThrow()
    candidate_->logMVWasDisqualified1("Column output %s got to Pass 2.",
                                      pending->getQueryElement()->getID());
    return FALSE;
  }

  if (pending->getActualQueryElement()->downCastToQRExpr()->getExprRoot()->containsAnAggregate())
  {
    // Handle rollup aggregates
    return matchAggregateExpressions(pending);
  }
  else
  {
    candidate_->logMVWasDisqualified1("Output expression %s cannot be Provided.", 
                                      pending->getQueryElement()->getID());

    return FALSE;
  }
}

// ***************************************************************************
// Rollup expressions means that both the MV and the query are aggregates, 
// and the query grouping list is a subset of the MV's grouping list.
// In this case output expressions are never Provided, because even if the 
// two expressions look identical (for example SUM(a) and SUM(a)), they really 
// are not.
// 1. Check if the expression is a supported aggregate function:
//    COUNT(*), COUNT(a), SUM(a), AVG(a), MIN(a), MAX(a), as well as
//    STDDEV(a), STDDEV(a,b) and VARIANCE(a). 
// 2. If so - check if the MV Provides the needed aggregates to calculate 
//    the required aggregates result. Rewrite using the MV aggregate column.
// 3. Maybe this expression is using supported aggregate functions, so its
//    more complex but still rewriteable.
// ***************************************************************************
NABoolean MatchOutput::matchAggregateExpressions(RewriteInstructionsItemPtr pending)
{
  QROutputPtr queryOutputExpr = pending->getQueryElement()->downCastToQROutput();
  QRExprPtr queryExpr = pending->getActualQueryElement()->downCastToQRExpr();
  QRExplicitExprPtr treeExpr = queryExpr->getExprRoot();

  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                    treeExpr->containsAnAggregate(), QRLogicException, 
		    "Expecting Rollup aggreate functions.");

  RewriteInstructionsItemPtr rewrite = NULL;

  // 1. Is the top level an aggregate function?
  if (treeExpr->getElementType() == ET_Function && 
      treeExpr->downCastToQRFunction()->isAnAggregate())
  {
    // Look for the corresponding aggregate function as an MV output.
    RewriteInstructionsItemPtr funcRewrite = 
      findAggregateFunction(queryExpr->getExprRoot()->downCastToQRFunction());
    if (funcRewrite == NULL)
      return FALSE;

    // The result code is set to NOTPROVIDED. Set it to PROVIDED as a sub-element.
    funcRewrite->setResultCode(RC_PROVIDED);

    // We got a final rewrite. However, its not really provided, because
    // of the rollup. Change it to a NotProvided.
    rewrite = new(heap_) 
      RewriteInstructionsItem(queryOutputExpr, NULL, RC_NOTPROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));

    assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                      candidate_->getAggregateMatchingType() == AMT_MAV_AQ_DG, QRLogicException, 
		      "Simple aggreate functions don't get here - they are Provided.");
    rewrite->addSubElement(funcRewrite);
    rewrite->setSecondaryResultCode(RC_ROLLUP_EXPR);
  }
  else
  {
    // 2. Use a visitor to find aggregate functions, and solve any that are found.
    rewrite = findInternalAggregateFunctions(treeExpr);

    if (rewrite == NULL)
      return FALSE;

    rewrite->setQueryElement(queryOutputExpr);
  }

  // And Put it in the Final list.
  addRewriteInstructions(rewrite);
  deletePtr(pending);
  return TRUE;
}  // matchAggregateExpressions()

// ***************************************************************************
// Create a new QROutput element for the result descriptor.
// ***************************************************************************
QROutputPtr MatchOutput::createNewResultElement(RewriteInstructionsItemPtr rewrite)
{
  // Construct the new element
  QROutputPtr outputElement = new(heap_) QROutput(ADD_MEMCHECK_ARGS(heap_));
  // Set the result attribute.
  outputElement->setResult(rewrite->getDescriptorResultCode());
  // Set the ref attribute.
  outputElement->setRef(rewrite->getQueryElement()->getID());

  return outputElement;
}

// ***************************************************************************
// Create a new QROutput element for the result descriptor, set to Provided.
// ***************************************************************************
NABoolean MatchOutput::generateProvidedElement(QRCandidatePtr resultDesc, 
					       RewriteInstructionsItemPtr rewrite)
{
  // Create the new Output element, Add the MVColumn to it.
  QROutputPtr outputElement = createNewResultElement(rewrite);

  QRMVColumnPtr mvColumn = generateProvidedMvColumn(rewrite);
  outputElement->setOutputItem(mvColumn);
  // Add it to the result descriptor.
  resultDesc->getOutputList()->addItem(outputElement);
  return TRUE;
}

// ***************************************************************************
// Create a new QROutput element for the result descriptor, set to NotProvided.
// ***************************************************************************
NABoolean MatchOutput::generateNotProvidedElement(QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite)
{
  ResultCode secondaryRC = rewrite->getSecondaryResultCode();
  QROutputPtr outputElement = NULL;
  switch (secondaryRC)
  {
    case RC_BACKJOIN:
    {
      const NAString& id = rewrite->getQueryElement()->getID();
      QRColumnPtr rColumn = generateNotProvidedColumn(rewrite);
      outputElement = new(heap_) QROutput(ADD_MEMCHECK_ARGS(heap_));
      outputElement->setOutputItem(rColumn);
      outputElement->setRef(id);
      outputElement->setResult(QRElement::NotProvided);
    }
    break;

    case RC_ROLLUP:
    {
      outputElement = generateRollupAggregateOutput(rewrite);
      if (outputElement == NULL)
      {
        candidate_->logMVWasDisqualified1("Output expression %s cannot be Provided.", 
	                                  rewrite->getQueryElement()->getID().data());
        return FALSE;
      }
    }
    break;

    case RC_INPUTS_PROVIDED:
    case RC_ROLLUP_EXPR:
    case RC_AGGR_EXPR:
    {
      QRExprPtr exprElement = generateAggregateExpressionOutput(rewrite);
      if (exprElement == NULL)
      {
        candidate_->logMVWasDisqualified1("Output expression %s cannot be Provided.", 
	                                  rewrite->getQueryElement()->getID().data());
        return FALSE;
      }
      else
      {
        outputElement = createNewResultElement(rewrite);
        outputElement->setOutputItem(exprElement);
      }
    }
    break;

    default:
      // Not implemented yet.
      candidate_->logMVWasDisqualified1("NotProvided output list is not fully implemented for %s.", 
	                                rewrite->getResultString(secondaryRC));
      return FALSE;
  }

  // Add the new Output element to the result descriptor.
  resultDesc->getOutputList()->addItem(outputElement);
  return TRUE;
}  // MatchOutput::generateNotProvidedElement()

// ***************************************************************************
// ***************************************************************************
QROutputPtr MatchOutput::generateRollupAggregateOutput(RewriteInstructionsItemPtr rewrite)
{
  // Generate the correct aggregate function to rollup over the MV aggregate function.
  QRFunctionPtr rollupFunction = generateRollupAggregateFunction(rewrite);

  // Create the expression on top of it.
  QRExprPtr expr = new(heap_) QRExpr(FALSE, ADD_MEMCHECK_ARGS(heap_));
  expr->setExprRoot(rollupFunction);
  expr->setRef(rewrite->getActualQueryElement()->getID());

  QROutputPtr outputElement = createNewResultElement(rewrite);
  outputElement->setOutputItem(expr);

  return outputElement;
}  // generateRollupAggregateOutput()

// ***************************************************************************
// Are the rewrite instructions for an Outside column.
// ***************************************************************************
NABoolean MatchOutput::isFromOutside(RewriteInstructionsItemPtr rewrite)
{
  const QRElementPtr queryElem = rewrite->getActualQueryElement()->getReferencedElement();
  if (queryElem->getElementType() == ET_Column)
  {
    const QRColumnPtr queryCol = queryElem->downCastToQRColumn();
    const NAString& queryTableID = queryCol->getTableID();
    const NAString* extraHubID;
    if (isOutsideColumn(queryCol, extraHubID))
      return TRUE;
  }

  return FALSE;
}

// ***************************************************************************
// Remove firstRewrite from the list of final rewrite instructions, and 
// replace it with newRewrite.
// ***************************************************************************
void MatchOutput::switchRewrites(RewriteInstructionsItemPtr newRewrite, 
				 RewriteInstructionsItemPtr firstRewrite,
				 const NAString& MVColName)
{
  const NAString& id = newRewrite->getActualQueryElement()->getID();
  const NAString& firstID = firstRewrite->getActualQueryElement()->getID();

  OutputRewriteInstructionsByID_.remove(&firstID);
  OutputRewriteInstructionsByID_.insert(&id, newRewrite);

  if (MVColName != "")
  {
    OutputRewriteInstructionsByMvColName_.remove(&MVColName);
    OutputRewriteInstructionsByMvColName_.insert(&MVColName, newRewrite);
  }

  getListOfFinalInstructions().remove(firstRewrite);
  getListOfFinalInstructions().insert(newRewrite);
}

// ***************************************************************************
// Override the default behaviour in order to avoid duplicates.
// ***************************************************************************
void MatchOutput::addRewriteInstructions(RewriteInstructionsItemPtr rewrite)
{
  if (rewrite->isFinal() == FALSE)
  {
    // Don't bother with intermediate results.
    MatchTest::addRewriteInstructions(rewrite);
  }
  else
  {
    // For final results, check if the ID of the query element is already
    // stored in the hash table.
    const NAString& id = rewrite->getActualQueryElement()->getID();
    const QRElementPtr mvOutput = rewrite->getMvElement();
    QRElementPtr mvElem = NULL;
    static NAString empty = "";
    NAString& MVColName = empty;
    if (mvOutput)
    {
      mvElem = mvOutput->downCastToQROutput()->getOutputItem()->getReferencedElement();
      if (mvElem->getElementType() == ET_Column)
        MVColName = mvElem->downCastToQRColumn()->getColumnName();
    }

    // If we need to avoid this ID, ignore it.
    if (OutputsToAvoidByID_.contains(&id) == TRUE)
      return;

    if (OutputRewriteInstructionsByID_.contains(&id) == FALSE)
    {
      // Its a new entry
      if (mvOutput && mvOutput->getElementType() == ET_Output)
      {
        if (OutputRewriteInstructionsByMvColName_.contains(&MVColName))
	{
	  // There already is an Output element for this MV column.

	  // If the new rewrite is for an Outside column, ignore it and leave the existing one.
	  if (isFromOutside(rewrite))
	  {
	    deletePtr(rewrite);
	    return;
	  }

	  // Find the existing entry
          RewriteInstructionsItemPtr firstRewrite = 
	    OutputRewriteInstructionsByMvColName_.getFirstValue(&MVColName);

	  // If the existing rewrite is on an Outside column, and the new one isn't, 
	  // Switch between them.
	  if (isFromOutside(firstRewrite))
	  {
	    switchRewrites(rewrite, firstRewrite, MVColName);
	    deletePtr(firstRewrite);
	    return;
	  }
	}
	OutputRewriteInstructionsByMvColName_.insert(&MVColName, rewrite);
      }

      // Insert it to the hash table, and keep it.
      OutputRewriteInstructionsByID_.insert(&id, rewrite);
      MatchTest::addRewriteInstructions(rewrite);
    }
    else
    {
      // Its a duplicate (by query ID)
      RewriteInstructionsItemPtr firstRewrite = OutputRewriteInstructionsByID_.getFirstValue(&id);
      // Prefer to reference an output query element.
      const NAString& firstID = firstRewrite->getQueryElement()->getID();
      const NAString& secondID = rewrite->getQueryElement()->getID();
      if (firstID.data()[0] != 'O' && secondID.data()[0] == 'O')
      {
	// Switch between them.
        switchRewrites(rewrite, firstRewrite, MVColName);
	deletePtr(firstRewrite);
      }
      else
      {
	deletePtr(rewrite);
      }
    }
  }
}  // MatchOutput::addRewriteInstructions()

// ***************************************************************************
// Override the default behaviour in order to avoid duplicates.
// ***************************************************************************
void MatchOutput::addOutputToAvoid(const NAString& id)
{
  // Add it to the list of IDs to avoid.
  OutputsToAvoidByID_.insert(&id, &id);

  // Check if we already have it in the output list.
  if (OutputRewriteInstructionsByID_.contains(&id) == TRUE)
  {
    // Yes, its in there. Remove it.
    RewriteInstructionsItemPtr existingRewrite = OutputRewriteInstructionsByID_.getFirstValue(&id);
    getListOfFinalInstructions().remove(existingRewrite);
    OutputRewriteInstructionsByID_.remove(&id);
    deletePtr(existingRewrite);
  }
}

//========================================================================
//  Class MatchRangePredicates
//========================================================================

// ***************************************************************************
// Run the Pass 1 algorithm of range predicate test.
// 1. First do the bitmap test, to see if we can quickly disqualify the MV.
// 2. Check the predicates one by one.
//    For each query range predicate
//      If the range predicate is on an expression
//        
//      Else (the range pred is on a single column or equality set)
//        Get the column
//        Find the MV predicates on this column through the BaseTableDetails.
//        If no MV predicates - this column PASSED as a final NotProvided.
//        If MV predicates exist - Mark for Pass 2.
// Returns TRUE if the MV passed this test, FALSE if it was disqualified.
// ***************************************************************************
NABoolean MatchRangePredicates::matchPass1()
{
  if (matchPredicateBitmaps() == FALSE)
    return FALSE;

  const JBBDetailsPtr mvJbbDetails = candidate_->getMvDetails()->getJbbDetails();
  const QRRangePredListPtr  queryRangePredList = candidate_->getQueryJbb()->getHub()->getRangePredList();

  if (queryRangePredList==NULL || queryRangePredList->getList().isEmpty())
  {
    if (!mvJbbDetails->hasNoRangePredicates())
    {
      candidate_->logMVWasDisqualified("the MV has range predicates, and the query does not.");
      return FALSE;
    }
    else
    {
      // No range predicates in both the MV and the query. 
      // Nothing to do.
      return TRUE;
    }
  }

  // Copy the list of MV range predicates.
  // Any matched predicate will be removed from this list, so we can check
  // that no MV preds were left out.
  if (!mvJbbDetails->hasNoRangePredicates())
    mvFullPredList_.insert(mvJbbDetails->getJbbDescriptor()->getHub()->getRangePredList()->getList());

  // The query has predicates, so we need to check them one-by-one.
  const RangePredPtrList&  queryRangePreds = queryRangePredList->getList();
  for (CollIndex i=0; i<queryRangePreds.entries(); i++)
  {
    //RewriteInstructionsItemPtr rewrite = NULL;
    const QRRangePredPtr queryRangePred = queryRangePreds[i];
    QRElementPtr rangeItem = queryRangePred->getRangeItem()->getReferencedElement();
    ElementType rangeElemType = rangeItem->getElementType();
    assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                      ( rangeElemType == ET_Column   || 
		        rangeElemType == ET_JoinPred || 
		        rangeElemType == ET_Expr       ), 
		      QRLogicException, 
		      "Expecting range items to be columns, equality sets or expressions.");

    if (rangeElemType == ET_Expr)
    {
      // Do the matching work for a range pred on an expression.
      if (matchPredOnExpr(queryRangePred) == FALSE)
	return FALSE;
    }
    else if (rangeElemType == ET_Column)
    {
      const QRColumnPtr rangeColumn = rangeItem->downCastToQRColumn();

      // Do the matching work for a range pred on a Column.
      if (matchPredOnColumn(queryRangePred, rangeColumn) == FALSE)
	return FALSE;
    }
    else
    {
      // Range item must be a join pred.
      QRJoinPredPtr joinPred = rangeItem->downCastToQRJoinPred();

      if (matchPredOnEqualitySet(queryRangePred, joinPred) == FALSE)
        return FALSE;
    } // Range pred is on join predicate.

  } // for on range predicates.

  if (mvFullPredList_.entries() > 0)
  {
    candidate_->logMVWasDisqualified1("MV range predicate %s was not matched by the query.", 
				      mvFullPredList_[0]->getID());
    return FALSE;
  }

  return TRUE;
}  // MatchRangePredicates::matchPass1()

// ***************************************************************************
// Match a range predicate on an equality set.
// The algorithm is:
// 1. For each column in the equality set:
//    1.1 If its an Outside column, ignore it (continue with the loop).
//    1.2 Otherwise call matchPredOnColumn() on it, return TRUE if it was matched.
// 2. If all the columns were Outside - return TRUE.
// ***************************************************************************
NABoolean MatchRangePredicates::matchPredOnEqualitySet(const QRRangePredPtr queryRangePred, const QRJoinPredPtr joinPred)
{
  const ElementPtrList& equalitySet = joinPred->getEqualityList();
  CollIndex maxEntries = equalitySet.entries();
  NABoolean insideColFound = FALSE;
  for (CollIndex i=0; i<maxEntries; i++)
  {
    QRElementPtr eqMember = equalitySet[i]->getReferencedElement();
    if (eqMember->getElementType() != ET_Column)
      continue;

    QRColumnPtr eqColumn = eqMember->downCastToQRColumn();
    const NAString* extraHubID = NULL;
    if (isOutsideColumn(eqColumn, extraHubID))
      continue;
    insideColFound = TRUE;

    if (matchPredOnColumn(queryRangePred, eqColumn))
      return TRUE;
  }

  // If all eqality set columns are Outside, this range predicate can be ignored.
  return !insideColFound;
}

// ***************************************************************************
// ***************************************************************************
NABoolean MatchRangePredicates::matchPredOnExpr(const QRRangePredPtr queryRangePred)
{
  QRElementPtr rangeItem = queryRangePred->getRangeItem()->getReferencedElement();
  const QRExprPtr queryRangeExpr = rangeItem->downCastToQRExpr();
  const ElementPtrList& queryInputColumns = queryRangeExpr->getInputColumns(heap_);
  RewriteInstructionsItemPtr rewrite = NULL;

  // If this is an aggregate expression in a rollup query
  // don't bother matching the expression text.
  // Go streight to Pass 2.
  if (candidate_->getAggregateMatchingType() == AMT_MAV_AQ_DG  &&
      queryRangeExpr->getExprRoot()->containsAnAggregate() )
  {
    rewrite = new(heap_) 
      RewriteInstructionsItem(queryRangePred, NULL, RC_ROLLUP_EXPR, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
  }
  else
  {
    const NAString& predText = queryRangeExpr->getExprText();
    RangePredPtrList* mvPredsOnExpr = candidate_->getMvDetails()->getJbbDetails()->getRangePredsOnExpression(predText);

    if (mvPredsOnExpr != NULL)
    {
      // Both MV and query have a range pred using this text. Now match the input columns.
      for (CollIndex j = 0; j<mvPredsOnExpr->entries(); j++)
      {
        QRRangePredPtr mvRangePred = (*mvPredsOnExpr)[j];
        const QRExprPtr mvRangeExpr = mvRangePred->getRangeItem()->downCastToQRExpr();
        if (matchExpressionInputs(queryInputColumns, mvRangeExpr->getInputColumns(heap_)) == FALSE)
	  continue; // Try next MV range pred with same text but different input columns.
        else
        {
	  // Input columns match. Now we need to match the range itself. Do that in Pass 2.
	  rewrite = new(heap_) 
	    RewriteInstructionsItem(queryRangePred, mvRangePred, RC_MATCH_RANGE, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
	  mvFullPredList_.remove(mvRangePred);	 
	  break; // We found a match, we can stop looking.
        }
      }
    }
  }

  if (rewrite == NULL)
  {
    // The MV has no range predicate using this text, so we need to provide rewrite instructions.
    // We need to verify that the pred input columns are available as an MV output.

    if ( candidate_->getAggregateMatchingType() == AMT_MAV_AQ_MG &&
         queryRangeExpr->getExprRoot()->containsAnAggregate() )
    {
      // This is an aggregate query on a matching MAV,
      // Try to match aggregate functions, in Pass 2.
      rewrite = new(heap_) 
        RewriteInstructionsItem(queryRangePred, NULL, RC_AGGR_EXPR, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
    }
    else
    {
      // This is on an MJV. Try to find the input columns.
      rewrite = findExpressionInputs(queryInputColumns, queryRangePred);
      if (rewrite &&
	  rewrite->isFinal() &&
	  rewrite->getSecondaryResultCode() == RC_INPUTS_PROVIDED &&
	  queryRangeExpr->getExprRoot()->containsAnAggregate())
      {
	// A HAVING predicate on an MJV
	// Add the expression inputs to the output list.
        addInputsToOutputList(rewrite);
	// And then throw away the Range predicate itself.
	deletePtr(rewrite);
	return TRUE;
      }
    }
  }

  if (rewrite == NULL)
  {
    candidate_->logMVWasDisqualified1("Query range pred %s cannot be provided.", queryRangePred->getID().data());
    return FALSE;  // No match found for this residual pred. Disqualify the MV candidate.
  }

  addRewriteInstructions(rewrite);
  return TRUE;
}  // MatchRangePredicates::matchPredOnExpr()

// ***************************************************************************
// ***************************************************************************
NABoolean MatchRangePredicates::matchPredOnColumn(const QRRangePredPtr queryRangePred, 
                                                  const QRColumnPtr    rangeColumn)
{
  QRRangePredPtr mvRangePred = NULL;
  RewriteInstructionsItemPtr rewrite = NULL;

  // Find the corresponding MV range predicate (if any).
  if (candidate_->getMvDetails()->getJbbDetails()->hasNoRangePredicates() == FALSE)
  {
    // Find the corresponding MV table (if any)
    BaseTableDetailsPtr baseTable = candidate_->getMvTableForQueryID(rangeColumn->getTableID(), FALSE);
    if (baseTable == NULL)
    {
      // This is an Outside table.
      // We can ignore this range predicate.
      return TRUE;
    }

    // Get the MV range pred on this column.
    mvRangePred = baseTable->getRangeColumnPredicatesFor(rangeColumn->getColIndex());
  }

  if (mvRangePred == NULL)
  {
    // The MV has no predicate on this column, so we need to provide rewrite instructions.
    // We need to verify that the range column is available as an MV output.
    RewriteInstructionsItemPtr columnRewrite = matchColumn(rangeColumn, FALSE);

    // The range column is not available - disqualify the MV.
    if (columnRewrite == NULL)
      return FALSE;

    // The primary result code is NotPrvided.
    // Its final if the column rewrite is Final.
    rewrite = new(heap_) 
      RewriteInstructionsItem(queryRangePred, NULL, RC_NOTPROVIDED, columnRewrite->getResultStatus(), ADD_MEMCHECK_ARGS(heap_));
    rewrite->addSubElement(columnRewrite);
  }
  else
  {
    // Both MV and query have a range pred on this column. We will match them in Pass 2.
    mvFullPredList_.remove(mvRangePred);
    rewrite = new(heap_) 
      RewriteInstructionsItem(queryRangePred, mvRangePred, RC_MATCH_RANGE, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
  }

  addRewriteInstructions(rewrite);
  return TRUE;
}  // MatchRangePredicates::matchPredOnColumn()

// ***************************************************************************
// Check if the query predicate bitmaps are a superset of the MV predicate bitmaps.
// This method checks the bitmaps for both range and residual predicates, 
// because the algorithm is the same for both, and it doesn't make sense to 
// repeat the entire loop for the residual predicates in the MatchResidualPredicates
// class for the sake of organized code.
// If the query range predicate bitmap is not a superset of the MV bitmap for
// all the involved tables, then the MV has predicates that are not covered by
// the query, and is very quickly disqualified.
// Return TRUE if the MV passed, and FALSE if it is disqualified.
// ***************************************************************************
NABoolean MatchRangePredicates::matchPredicateBitmaps()
{
  JBBDetailsPtr mvJbbDetails = candidate_->getMvDetails()->getJbbDetails();

  // If the MV has no range and residual preds, there is no need to check the bitmaps.
  if (mvJbbDetails->hasNoRangePredicates() && mvJbbDetails->hasNoResidualPredicates())
    return TRUE;

  // Iterate on the tables in the join sub-graph, which correspond to the MV hub tables.
  QRJoinSubGraphPtr subGraph = candidate_->getJbbSubset()->getSubGraph();
  for( subGraph->reset(); subGraph->hasNext(); subGraph->advance() )
  {
    // Get the table query descriptor ID, and from it the QRTable pointer.
    const JoinGraphTablePtr queryJoinGraphTable = subGraph->getCurrent();
    const NAString& queryTableID = queryJoinGraphTable->getID();
    const QRTablePtr queryTable = candidate_->getQueryDetails()->getElementForID(queryTableID)->downCastToQRTable();

    // Get the query predicate bitmaps
    const XMLBitmap& queryRangeBits = queryTable->getRangeBits();
    const XMLBitmap& queryResidualBits = queryTable->getResidualBits();

    // Find the MV table ID using the MVMemo mapping.
    BaseTableDetailsPtr mvTableDetails = candidate_->getMvTableForQueryID(queryTableID);

    // Get the MV predicate bitmaps
    const XMLBitmap& mvRangeBits = mvTableDetails->getRangeBits();
    const XMLBitmap& mvResidualBits = mvTableDetails->getResidualBits();

    // Perform the actual test.
    // Any failure disqualifies the MV.
    if (queryRangeBits.contains(mvRangeBits) == FALSE)
    {
      candidate_->logMVWasDisqualified1("Table %s failed on range bitmap.", queryTable->getTableName().data());
      return FALSE; 
    }
    if (queryResidualBits.contains(mvResidualBits) == FALSE)
    {
      candidate_->logMVWasDisqualified1("Table %s failed on residual bitmap.", queryTable->getTableName().data());
      return FALSE; 
    }
  }

  return TRUE;
}  // MatchRangePredicates::matchPredicateBitmaps()

// ***************************************************************************
// Run the Pass 2 algorithm of range predicate test.
// Returns TRUE if the MV passed this test, FALSE if it was disqualified.
// ***************************************************************************
NABoolean MatchRangePredicates::matchPass2OnElement(RewriteInstructionsItemPtr pending)
{
  const QRRangePredPtr queryPred = pending->getQueryElement()->downCastToQRRangePred();

  if (pending->getResultCode() == RC_AGGR_EXPR ||
      pending->getResultCode() == RC_ROLLUP_EXPR)
  {
    QRElementPtr treeElem =
      queryPred->getRangeItem()->getReferencedElement()->downCastToQRExpr()
        ->getExprRoot()->getReferencedElement();
    QRExplicitExprPtr treeExpr = static_cast<QRExplicitExprPtr>(treeElem);
    // Use a visitor to find aggregate functions, and solve any that are found.
    RewriteInstructionsItemPtr exprRewrite = findInternalAggregateFunctions(treeExpr);

    if (exprRewrite == NULL)
    {
      candidate_->logMVWasDisqualified1("Range HAVING predicate %s is not Provided by the MV.",
                                        queryPred->getID().data());
      return FALSE;
    }

    RewriteInstructionsItemPtr rewrite = new(heap_) 
      RewriteInstructionsItem(queryPred->getRangeItem(), NULL, RC_NOTPROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));

    exprRewrite->setQueryElement(queryPred->getRangeItem());
    rewrite->addSubElement(exprRewrite);
    rewrite->setQueryElement(queryPred);
    rewrite->setSecondaryResultCode(pending->getResultCode());
    addRewriteInstructions(rewrite);
    deletePtr(pending);
    return TRUE;
  }

  if (pending->getResultCode() == RC_NOTPROVIDED)
  {
    if (pending->getSubElements()->indirectInputs_.entries() > 0)
    {
      if (queryPred->getRangeItem()->getElementType() == ET_Expr) 
      {
        // The range expression may be an output of the MV,
        QRExprPtr rangeExpr = queryPred->getRangeItem()->downCastToQRExpr();
        RewriteInstructionsItemPtr exprRewrite = findMatchingMvExpression(rangeExpr->getExprRoot());
        if (exprRewrite == NULL)
        {
	  // The range expression cannot be provided. Disqualify the MV.
	  // logMVWasDisqualified1() was already called by matchColumn();
	  return NULL;
        }
        else if (exprRewrite->getResultStatus() == RS_FINAL &&
                 exprRewrite->getResultCode() == RC_PROVIDED)
        {
          // The range expression is an output of the MV.

          // Clear the indirect sub-elements first.
          RewriteInstructionsItemList& indirect = pending->getSubElements()->indirectInputs_;
          for (CollIndex i=0; i<indirect.entries(); i++)
            deletePtr(indirect[i]);
          indirect.clear();

          // Now add the new rewrite instructions.
          exprRewrite->setQueryElement(rangeExpr);
 	  pending->addSubElement(exprRewrite);
        }
        else
        {
          candidate_->logMVWasDisqualified1("Range expression %s is not Provided by the MV.",
                                            queryPred->getID().data());
          deletePtr(exprRewrite);
          return NULL;
        }
      }
    }

    // This must have been a back join or extra-hub table.
    // If we got here, it means its OK.
    pending->setResultStatus(RS_FINAL);
    addRewriteInstructions(pending);
    return TRUE;
  }

  assertLogAndThrow1(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                     pending->getResultCode() == RC_MATCH_RANGE, QRLogicException, 
    "Unexpected Pass 2 range pred result code: %s.", pending->getResultString()); 

  // Get the RangeSpec object from the Query descriptor
  const RangeSpec* queryRangeSpec = queryPred->getRangeSpec(heap_);

  // Get the RangeSpec object from the MV descriptor
  const QRRangePredPtr mvPred = pending->getMvElement()->downCastToQRRangePred();
  const RangeSpec* mvRangeSpec = mvPred->getRangeSpec(heap_);

  if (queryRangeSpec->isEqual(*mvRangeSpec))
  {
    // The range predicates are identical - return Provided.
    pending->setResultCode(RC_PROVIDED);
  }
  else if (mvPred->getMustMatch() || queryPred->getMustMatch())
  {
    // The predicate must match exactly, but they don't
    candidate_->logMVWasDisqualified1("The range predicate %s does not exactly match any MV predicate.",
				      queryPred->getID().data());
    return FALSE;
  }
  else
  {
    // The predicates are not identical, but they don't need to be.
    if (mvRangeSpec->subsumes(queryRangeSpec))
    {
      // The query predicate subsumes the MV predicate - NotProvided.
      pending->setResultCode(RC_NOTPROVIDED);

      // Last thing to check - is the range column provided as an output?
      // Otherwise we can't use the range pred on the MV...
      QRElementPtr rangeItem = queryPred->getRangeItem()->getReferencedElement();
      RewriteInstructionsItemPtr columnRewrite = NULL;
      NABoolean wasAdded = FALSE;
      switch(rangeItem->getElementType())
      {
        case ET_Column:
        {
          QRColumnPtr rangeCol = rangeItem->downCastToQRColumn();
          columnRewrite = matchColumn(rangeCol, FALSE);
        }
        break;

        case ET_JoinPred:
        {
          QRJoinPredPtr rangeJoin = rangeItem->downCastToQRJoinPred();
          columnRewrite = matchEqualitySet(rangeJoin);
        }
        break;

        case ET_Expr:
        {
          // It must be a range predicate on an expression.
          QRExprPtr rangeExpr = queryPred->getRangeItem()->downCastToQRExpr();
          columnRewrite = matchExpression(rangeExpr);
          if (columnRewrite != NULL)
          {
            // We can only use a Final rewrite using an MV output expression.
            if (columnRewrite->isFinal() == FALSE)
            {
              deletePtr(columnRewrite);
              columnRewrite = NULL;
              candidate_->logMVWasDisqualified1("range predicate %s cannot be Provided.", 
					queryPred->getID().data());
              return FALSE;
            }

            columnRewrite->setQueryElement(rangeExpr);
            // Is the entire range expression Provided by the MV.
            if (columnRewrite->getMvElement() == NULL)
            {
              // No - Only the input columns are Provided
	      pending->addSubElements(columnRewrite->getSubElements());
              columnRewrite->getSubElements()->clear();
              deletePtr(columnRewrite);
              columnRewrite = NULL;
              wasAdded = TRUE;
            }
          }
        }
        break;
      } // end switch()

      if (columnRewrite == NULL)
      {
	// The range column cannot be provided. Disqualify the MV.
	// logMVWasDisqualified1() was already called by matchColumn();
        if (!wasAdded)
	  return NULL;
      }
      else
      {
	pending->addSubElement(columnRewrite);
      }

    }
    else
    {
      // The MV predicate does not subsume the query predicate - disqualify!
      candidate_->logMVWasDisqualified1("The query range predicate %s is not subsumed by the corresponding MV predicate.", 
					queryPred->getID().data());
      return FALSE;
    }
  }

  // Insert the rewrite instruction to the final list.
  pending->setResultStatus(RS_FINAL);
  addRewriteInstructions(pending);
  return TRUE;
}  // MatchRangePredicates::matchPass2OnElement()

// ***************************************************************************
// Create a new QRRangePred element for the result descriptor.
// ***************************************************************************
QRRangePredPtr MatchRangePredicates::createNewResultElement(RewriteInstructionsItemPtr rewrite)
{
  // Construct the new element
  QRRangePredPtr rangeElement = new(heap_) QRRangePred(ADD_MEMCHECK_ARGS(heap_));
  // Set the result attribute.
  rangeElement->setResult(rewrite->getDescriptorResultCode());
  // Set the ref attribute.
  rangeElement->setRef(rewrite->getQueryElement()->getID());

  return rangeElement;
}

// ***************************************************************************
// A Provided range predicate needs only the reference to the matched 
// query element.
// ***************************************************************************
NABoolean MatchRangePredicates::generateProvidedElement(QRCandidatePtr resultDesc, 
							RewriteInstructionsItemPtr rewrite)
{
  // Create the new Range element, mark it as Provided.
  QRRangePredPtr rangeElement = createNewResultElement(rewrite);

  // Add it to the result descriptor.
  resultDesc->getRangePredList()->addItem(rangeElement);
  return TRUE;
}

// ***************************************************************************
// A NotProvided range predicate includes the actual range predicate with 
// rewrite instructions for the input columns.
// ***************************************************************************
NABoolean MatchRangePredicates::generateNotProvidedElement(QRCandidatePtr resultDesc, 
							   RewriteInstructionsItemPtr rewrite)
{
  // Create the new Output element, mark it as NotProvided, and ref the query range pred.
  QRRangePredPtr rangeElement = createNewResultElement(rewrite);
  QRElementPtr   rangeItem = NULL;

  // Get the rewrite for the range column.
  if (rewrite->getSecondaryResultCode() == RC_AGGR_EXPR ||
      rewrite->getSecondaryResultCode() == RC_ROLLUP_EXPR)
  {
    RewriteInstructionsItemPtr exprRewrite = rewrite->getSubElements()->notProvidedInputs_[0];
    assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                      exprRewrite, QRLogicException, 
		      "Expected a single NotProvided sub-element for aggregate range predicates."); 
    rangeItem = generateAggregateExpressionOutput(exprRewrite);
    if (rangeItem == NULL)
    {
      candidate_->logMVWasDisqualified1("Range HAVING predicate %s cannot be Provided.", 
	                                rewrite->getQueryElement()->getID().data());
      return FALSE;
    }
    else
    {
      // Set the result attribute.
      rangeElement->setResult(rewrite->getDescriptorResultCode());
      // Set the ref attribute.
      rangeElement->setRef(rewrite->getQueryElement()->getID());
    }
  }
  else 
  {
    compositeMatchingResultsPtr subElements = rewrite->getSubElements();

    if (rewrite->getSecondaryResultCode() == RC_NO_INPUTS)
    {
      // The range item is an expression with no input columns, such as (COUNT(*)
      // Create a duplicate range item for it in the result descriptor.
      subExpressionRewriteHash subExprHash(hashKey, INIT_HASH_SIZE_SMALL, FALSE, heap_);
      QRRangePredPtr rangePred = rewrite->getQueryElement()->downCastToQRRangePred();
      QRExprPtr rangeExpr = rangePred->getRangeItem()->downCastToQRExpr();
      QRExplicitExprPtr ExprRoot = rangeExpr->getExprRoot()->deepCopyAndSwitch(subExprHash, heap_);
      QRExprPtr resultExpr = new(heap_) QRExpr(FALSE, ADD_MEMCHECK_ARGS(heap_));
      resultExpr->setRef(rangeExpr->getID());
      resultExpr->setExprRoot(ExprRoot);

      rangeItem = resultExpr;
    }
    else
    {
      if (subElements->providedInputs_.entries() == 1)
      {
	QRMVColumnPtr mvColumn = generateProvidedMvColumn(subElements->providedInputs_[0]);
	rangeItem = mvColumn;
      }
      else if (subElements->notProvidedInputs_.entries() == 1)
      {
	QRColumnPtr backJoinColumn = generateNotProvidedColumn(subElements->notProvidedInputs_[0]);
	rangeItem = backJoinColumn;
      }
      else if (subElements->backJoinInputs_.entries() == 1)
      {
	QRColumnPtr backJoinColumn = generateNotProvidedColumn(subElements->backJoinInputs_[0]);
	rangeItem = backJoinColumn;
      }
      else if (subElements->outsideInputs_.entries() == 1)
      {
	QRColumnPtr outsideColumn = generateNotProvidedColumn(subElements->outsideInputs_[0]);
	rangeItem = outsideColumn;
      }

      assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
			rangeItem != NULL, QRLogicException, 
			"Expecting either Provided, Outside or BackJoin inputs.");
    }
  }

  // Add it to the result descriptor.
  rangeElement->setRangeItem(rangeItem);
  resultDesc->getRangePredList()->addItem(rangeElement);
  return TRUE;
}  // MatchRangePredicates::generateNotProvidedElement()

//***************************************************************************
// Used by workload analysis.
//***************************************************************************
QRRangePredPtr MatchRangePredicates::checkPredicate(QRRangePredPtr querySidePred)
{
  QRRangePredPtr mvRangePred = NULL;
  QRElementPtr rangeItem = querySidePred->getRangeItem()->getReferencedElement();
  ElementType rangeElemType = rangeItem->getElementType();
  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                    ( rangeElemType == ET_Column   || 
		      rangeElemType == ET_JoinPred || 
		      rangeElemType == ET_Expr       ), 
		    QRLogicException, 
		    "Expecting range items to be columns, equality sets or expressions.");

  if (rangeElemType == ET_Expr)
  {
    // Find all the MV range preds using the same expression text
    const QRExprPtr queryRangeExpr = rangeItem->downCastToQRExpr();
    const NAString& predText = queryRangeExpr->getExprText();
    RangePredPtrList* mvPredsOnExpr = candidate_->getMvDetails()->getJbbDetails()->getRangePredsOnExpression(predText);
    if (mvPredsOnExpr == NULL)
      return NULL;
    
    // Both MV and query have a range pred using this text. Now match the input columns.
    const ElementPtrList& queryInputColumns = queryRangeExpr->getInputColumns(heap_);
    NABoolean foundMatch = FALSE;
    for (CollIndex j = 0; j<mvPredsOnExpr->entries(); j++)
    {
      mvRangePred = (*mvPredsOnExpr)[j];
      const QRExprPtr mvRangeExpr = mvRangePred->getRangeItem()->downCastToQRExpr();
      if (matchExpressionInputs(queryInputColumns, mvRangeExpr->getInputColumns(heap_)))
      {
        foundMatch = TRUE;
        break;  // Found a match
      }
    }

    // Did not find a matching range pred on the same expression text and input columns.
    if (!foundMatch)
      return NULL;
  }
  else 
  {
    // If its a join pred, get the first column.
    QRColumnPtr rangeColumn = rangeItem->getFirstColumn();

    // Find the corresponding MV range predicate (if any).
    if (candidate_->getMvDetails()->getJbbDetails()->hasNoRangePredicates() == FALSE)
    {
      // Find the corresponding MV table (if any)
      BaseTableDetailsPtr baseTable = candidate_->getMvTableForQueryID(rangeColumn->getTableID(), FALSE);
      if (baseTable)
      {
        // Get the MV range pred on this column.
        mvRangePred = baseTable->getRangeColumnPredicatesFor(rangeColumn->getColIndex());
      }
    }
  } // if not expreesion.

  // Is there a matching range predicate on the same column?
  if (mvRangePred == NULL)
    return NULL;

  // OK, we get here when we found an MV side range predicate with a matching 
  // range item (column or expression).
  // Now we need to match the range spec.
  // For now, we only check for an exact match.
  const RangeSpec* mvRangeSpec = mvRangePred->getRangeSpec(heap_);
  const RangeSpec* queryRangeSpec = querySidePred->getRangeSpec(heap_);
  if (queryRangeSpec->isEqual(*mvRangeSpec))
    return mvRangePred;
  else
    return NULL;
}

//========================================================================
//  Class MatchResidualPredicates
//========================================================================

//***************************************************************************
// Run the Pass 1 algorithm of residual predicate test.
// The bitmap test was already done by the range predicate code, so now 
// check the predicates one by one.
// For each query residual predicate
//   Using the predicate text, find any corresponding MV predicates.
//   Match the input columns of those predicates.
//   If a matching MV predicate was found
//     Its a final Provided.
//   Otherwise
//     Check if the predicate input columns are provided by the MV.
//     If they are blocked
//       Disqualify the MV.
//     If the are all Provided
//       The predicate is a final NotProvided.
//     Otherwise
//       The predicate is an intermediate NotProvided, postpone to Pass 2.
//       
// Returns TRUE if the MV passed this test, FALSE if it was disqualified.
// ***************************************************************************
NABoolean MatchResidualPredicates::matchPass1()
{
  const JBBDetailsPtr mvJbbDetails = candidate_->getMvDetails()->getJbbDetails();

  const QRResidualPredListPtr  queryResidualPredList = 
    candidate_->getQueryJbb()->getHub()->getResidualPredList();
  if (queryResidualPredList==NULL || queryResidualPredList->getList().isEmpty())
  {
    if (!mvJbbDetails->hasNoResidualPredicates())
    {
      candidate_->logMVWasDisqualified("it has residual predicates and the Query does not.");
      return FALSE;
    }
    else
    {
      // No residual predicates in both the MV and the query. 
      // Nothing to do.
      return TRUE;
    }
  }

  ResidualPredPtrList usedDuplicates;
  const ResidualPredPtrList&  queryResidualPreds = queryResidualPredList->getList();
  // Copy the list of MV residual predicates.
  // Any matched predicate will be removed from this list, so we can check
  // that no MV preds were left out.
  if (!mvJbbDetails->hasNoResidualPredicates())
    mvFullPredList_.insert(mvJbbDetails->getJbbDescriptor()->getHub()->getResidualPredList()->getList());

  for (CollIndex i=0; i<queryResidualPreds.entries(); i++)
  {
    const QRExprPtr queryResidualPred = queryResidualPreds[i];
    RewriteInstructionsItemPtr rewrite = matchResidualPredicate(queryResidualPred,
                                                                usedDuplicates);
    if (rewrite == NULL)
    {
      candidate_->logMVWasDisqualified1("Query residual predicate %s was not matched by the query.", 
					                              queryResidualPred->getID());
      return FALSE;  // No match found for this residual pred. Disqualify the MV candidate.
    }
    else
    {
      if (rewrite->getResultCode() == RC_SKIP_ME)
	deletePtr(rewrite);
      else
        addRewriteInstructions(rewrite);  // We found a match, save the rewrite instructions.
    }

  } // for

  if (mvFullPredList_.entries() > 0)
  {
    candidate_->logMVWasDisqualified1("MV residual predicate %s was not matched by the query.", 
                                      mvFullPredList_[0]->getID());
    return FALSE;
  }

  return TRUE;
}  // MatchResidualPredicates::matchPass1()

// ***************************************************************************
// ***************************************************************************
RewriteInstructionsItemPtr MatchResidualPredicates::matchResidualPredicate(const QRExprPtr queryResidualPred,
                                                                           ResidualPredPtrList& usedDupMvPreds)
{
  RewriteInstructionsItemPtr rewrite = NULL;
  NABoolean isAggregateExpr = queryResidualPred->getExprRoot()->containsAnAggregate();

  // If this is an aggregate expression in a rollup query
  // don't bother matching the expression text.
  // Go straight to Pass 2.
  if (candidate_->getAggregateMatchingType() == AMT_MAV_AQ_DG  &&
      isAggregateExpr )
  {
    // This is an aggregate query in a rollup query.,
    // Try to match aggregate functions, in Pass 2.
    rewrite = new(heap_) 
      RewriteInstructionsItem(queryResidualPred, NULL, RC_NOTPROVIDED, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
  }
    
  const NAString& predText = queryResidualPred->getExprText();
  // Get the list of MV residual predicates using the same expression text.
  ResidualPredPtrList* mvPredList = candidate_->getMvDetails()->getJbbDetails()->getResidualPreds(predText);

  if (mvPredList != NULL)
  {
    CollIndex mvPredCount = mvPredList->entries();

    // Both MV and query have a residual pred using this text. Now match the input columns.
    for (CollIndex j = 0; j<mvPredList->entries(); j++)
    {
      QRExprPtr mvResidualPred = (*mvPredList)[j];
      if (matchExpressionInputs(queryResidualPred->getInputColumns(heap_), 
                                mvResidualPred->getInputColumns(heap_)) == FALSE)
        continue; // Try next MV residual pred with same text but different input columns.
      else
      {
        // A residual pred on a join operand will appear in each node of an
        // equality set. In some cases, this pred will be duplicated with a
        // different value id, and we must avoid matching the same (with respect
        // to text) mv pred to two distinct (but also with same text) query
        // resid preds, or else one will be left unmatched in mvFullPredList_
        // and result in a spurious disqualification for an unmatched pred.
        if (mvPredCount > 1)  // Only an issue when text matches multiple preds
        {
          if (usedDupMvPreds.contains(mvResidualPred))
            // already matched this one, try another
            continue;
          else
            // make sure we don't match this one again
            usedDupMvPreds.insert(mvResidualPred);
        }
        mvFullPredList_.remove(mvResidualPred);	 
        rewrite = new(heap_) 
          RewriteInstructionsItem(queryResidualPred, mvResidualPred, RC_PROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
        break; // We found a match, we can stop looking.
      }
    }
  }

  if (rewrite == NULL)
  {
    // The MV has no residual predicate using this text, so we need to provide rewrite instructions.
    // We need to verify that the pred input columns are available as an MV output.

    if ( candidate_->getAggregateMatchingType() == AMT_MAV_AQ_MG &&
         isAggregateExpr )
    {
      // This is an aggregate query on a matching MAV,
      // Try to match aggregate functions, in Pass 2.
      rewrite = new(heap_) 
        RewriteInstructionsItem(queryResidualPred, NULL, RC_NOTPROVIDED, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
    }
    else
    {
      // This is on an MJV. Try to find the input columns.
      const ElementPtrList& queryInputElements = 
        queryResidualPred->getInputColumns(heap_);

      if (queryInputElements.entries() > 0)
      {
        rewrite = findExpressionInputs(queryInputElements, queryResidualPred);
	if (rewrite &&
	    rewrite->isFinal() &&
	    rewrite->getSecondaryResultCode() == RC_INPUTS_PROVIDED &&
	    isAggregateExpr)
	{
	  // A HAVING predicate on an MJV
	  // Add the expression inputs to the output list.
	  addInputsToOutputList(rewrite);
	  // The residual predicate itself will be ignored
	  return rewrite;
	}
      }
      else
      {
        // This predicate has no inputs.
        // Mark it as NotProvided.
        rewrite = new(heap_) 
          RewriteInstructionsItem(queryResidualPred, NULL, RC_NOTPROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
	rewrite->setSecondaryResultCode(RC_NO_INPUTS);
      }
    }
  }

  return rewrite;
}  // MatchResidualPredicates::matchResidualPredicate()

// ***************************************************************************
// Run the Pass 2 algorithm of residual predicate test.
// Returns TRUE if the MV passed this test, FALSE if it was disqualified.
// ***************************************************************************
NABoolean MatchResidualPredicates::matchPass2OnElement(RewriteInstructionsItemPtr pending)
{
  if (pending->getResultCode() == RC_NOTPROVIDED)
  {
    // The residual expression may be a HAVING predicate.
    QRExprPtr residExpr = pending->getQueryElement()->downCastToQRExpr();
    QRExplicitExprPtr treeExpr = residExpr->getExprRoot();

    // Use a visitor to find aggregate functions, and solve any that are found.
    RewriteInstructionsItemPtr rewrite = findInternalAggregateFunctions(treeExpr);

    if (rewrite == NULL)
    {
      candidate_->logMVWasDisqualified1("Residual expression %s is not Provided by the MV.",
                                        residExpr->getID().data());
      return FALSE;
    }

    rewrite->setQueryElement(residExpr);
    addRewriteInstructions(rewrite);

    deletePtr(pending);
    return TRUE;
  }
  else
  {
    const char* reason = pending->getResultString();
    candidate_->logMVWasDisqualified1("Residual preds got to Pass 2 on %s.", reason);

    return FALSE;
  }
}

// ***************************************************************************
// Create a new residual pred element (<Expr>) for the result descriptor.
// ***************************************************************************
QRExprPtr MatchResidualPredicates::createNewResultElement(RewriteInstructionsItemPtr rewrite)
{
  // Construct the new element
  QRExprPtr residualElement = new(heap_) QRExpr(TRUE, ADD_MEMCHECK_ARGS(heap_));
  // Set the result attribute.
  residualElement->setResult(rewrite->getDescriptorResultCode());
  // Set the ref attribute.
  residualElement->setRef(rewrite->getQueryElement()->getID());

  return residualElement;
}

// ***************************************************************************
// A Provided residual predicate requires no additional work.
// ***************************************************************************
NABoolean MatchResidualPredicates::generateProvidedElement(QRCandidatePtr resultDesc, 
							   RewriteInstructionsItemPtr rewrite)
{
  // Create the new Output element, mark it as Provided.
  QRExprPtr residualElement = createNewResultElement(rewrite);

  // Add it to the result descriptor.
  resultDesc->getResidualPredList()->addItem(residualElement);
  return TRUE;
}

// ***************************************************************************
// A NotProvided residual predicate includes the actual residual predicate 
// with rewrite instructions for the input columns.
// ***************************************************************************
NABoolean MatchResidualPredicates::generateNotProvidedElement(QRCandidatePtr resultDesc, 
                                                              RewriteInstructionsItemPtr rewrite)
{
  QRExprPtr residualElement = NULL;

  if (rewrite->getSecondaryResultCode() == RC_AGGR_EXPR ||
      rewrite->getSecondaryResultCode() == RC_ROLLUP_EXPR)
  {
    residualElement = generateAggregateExpressionOutput(rewrite);
    if (residualElement == NULL)
    {
      candidate_->logMVWasDisqualified1("Output expression %s cannot be Provided.", 
	                                rewrite->getQueryElement()->getID().data());
      return FALSE;
    }
    else
    {
      residualElement->setResidual(TRUE);
      // Set the result attribute.
      residualElement->setResult(rewrite->getDescriptorResultCode());
      // Set the ref attribute.
      residualElement->setRef(rewrite->getQueryElement()->getID());
    }
  }
  else 
  {
    // Create the new NotProvided Residual element, referencing the query
    // residual pred.
    residualElement = createNewResultElement(rewrite);

    //Get the rewrite instructions for the subexpressions.
    compositeMatchingResultsPtr subExprs = rewrite->getSubElements();
    assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                      subExprs->entries() == subExprs->providedInputs_.entries() + subExprs->backJoinInputs_.entries(), 
                      QRLogicException, "Expecting all subexpressions to be Provided or back joins.");

    // Create a hash table to map query column ids to the mvcols that replace them
    // in the expression.  
    subExpressionRewriteHash subExprHash(hashKey, INIT_HASH_SIZE_SMALL, FALSE, heap_);
    
    // For each subElement, get the id of the query element and the corresponding
    // mv column, and put them in the hash table.
    // Start with Provided inputs.
    RewriteInstructionsItemList& rewriteList = subExprs->providedInputs_;
    for (CollIndex i=0; i<rewriteList.entries(); i++)
    {
      RewriteInstructionsItemPtr subExprRewrite = rewriteList[i];
      const NAString& key = subExprRewrite->getQueryElement()->getID();
      QRMVColumnPtr mvColumn = generateProvidedMvColumn(subExprRewrite);
      subExprHash.insert(&key, mvColumn);
    }

    // Now do back join columns.
    RewriteInstructionsItemList& backJoinList = subExprs->backJoinInputs_;
    for (CollIndex i=0; i<backJoinList.entries(); i++)
    {
      RewriteInstructionsItemPtr subExprRewrite = backJoinList[i];
      const NAString& key = subExprRewrite->getQueryElement()->getID();
      QRColumnPtr bjColumn = generateNotProvidedColumn(subExprRewrite);
      subExprHash.insert(&key, bjColumn);
    }

    // Create the result expression tree from the query expression tree, with 
    // the needed sub-expressions switched to MVColumn elements.
    QRExprPtr queryExpr = rewrite->getActualQueryElement()->downCastToQRExpr();
    QRExplicitExprPtr resultTreeExpr = NULL;

    try
    {
      resultTreeExpr = queryExpr->getExprRoot()->deepCopyAndSwitch(subExprHash, heap_);
    }
    catch(QRDescriptorException ex)
    {
      // We get here if we cannot properly rewrite all the query column elements.
      // Currently we are leaking the memory of the elements stored in subExprHash.
      candidate_->logMVWasDisqualified1("Residual predicate %s cannot be rewritten.", 
                                        queryExpr->getID().data());
      return NULL;
    }

    residualElement->setExprRoot(resultTreeExpr);
  }

  // Add the residual predicate to the result descriptor.
  resultDesc->getResidualPredList()->addItem(residualElement);
  return TRUE;
}

//***************************************************************************
// Used by workload analysis.
//***************************************************************************
QRExprPtr MatchResidualPredicates::checkPredicate(QRExprPtr querySidePred)
{
  const NAString& predText = querySidePred->getExprText();
  // Get the list of MV residual predicates using the same expression text.
  ResidualPredPtrList* mvPredList = candidate_->getMvDetails()->getJbbDetails()->getResidualPreds(predText);

  if (mvPredList != NULL)
  {
    CollIndex mvPredCount = mvPredList->entries();

    // Both MV and query have a residual pred using this text. Now match the input columns.
    for (CollIndex j = 0; j<mvPredList->entries(); j++)
    {
      QRExprPtr mvResidualPred = (*mvPredList)[j];
      if (matchExpressionInputs(querySidePred->getInputColumns(heap_), 
                                mvResidualPred->getInputColumns(heap_)))
        return mvResidualPred;
    }
  }

  return NULL;
}

//========================================================================
//  Class MatchGroupingColumns
//========================================================================

//***************************************************************************
//***************************************************************************
NABoolean MatchGroupingColumns::matchPass1()
{
  QRGroupByPtr groupBy = NULL;

  if (candidate_->getJbbSubset()->isIndirectGroupBy())
  {
    if (VerifyGroupingColumns() == FALSE)
      return FALSE;
  }
  else
  {
    switch(candidate_->getAggregateMatchingType())
    {
      case AMT_MJV_JQ:
        // No Groupby involved.
        break;

      case AMT_MJV_AQ:
        // Aggregate query on MJV.
        // No GROUPBY element is needed in the result descriptor,
        // but we do need to add rewrite instructions for the grouping columns
        // to the output list.
        // Return without  GroupBy element in the result descriptor.
        return VerifyGroupingColumns();

      case AMT_MAV_AQ_MG:
        // No extra grouping is needed - the MV provided it all.
        break;

      case AMT_MAV_AQ_DG:
        // The MV did not provide all the grouping columns - handle the rewrite instructions.
        return VerifyGroupingColumns();
    }
  }

  return TRUE;
}  // MatchGroupingColumns::matchPass1()

/*****************************************************************************/
NABoolean MatchGroupingColumns::VerifyGroupingColumns()
{
  // Do primary grouping list only, for now.
  // TODO: Should do grouping columns from all the tables covered by the MV.
  QRGroupByPtr queryGroupBy = candidate_->getQueryJbb()->getGroupBy();
  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL, 
                    queryGroupBy != NULL, QRLogicException, 
		    "Expecting a query grouping list.");

  // An empty GroupBy means its an aggregate query with no GROUP BY.
  // Its OK, and there are no grouping columns to verify.
  if (queryGroupBy->isEmpty())
    return TRUE;

  const GroupingList& groupingList = queryGroupBy->getPrimaryList()->getList();
  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                    !groupingList.isEmpty(), QRLogicException, 
		    "Query has Group By, but no grouping list.");

  // The list of grouping columns is taken from the query descriptor, 
  // So we cannot use the IDs to find the MV Output column names.
  // We have to find them by name.
  for (CollIndex i=0; i<groupingList.entries(); i++)
  {
    const QRElementPtr groupingElem = groupingList[i]->getReferencedElement();
    if (verifyGroupingElement(groupingElem) == FALSE)
      return FALSE;
  }

  // Return without  GroupBy element in the result descriptor.
  return TRUE;
}

/*****************************************************************************/
NABoolean MatchGroupingColumns::verifyGroupingElement(const QRElementPtr groupingElem, NABoolean isRecursive)
{
  RewriteInstructionsItemPtr rewrite = NULL;
  if (groupingElem->getElementType() == ET_Expr)
  {
    const QRExprPtr groupingExpr = groupingElem->getReferencedElement()->downCastToQRExpr();
    rewrite = findMatchingMvExpression(groupingExpr->getExprRoot());

    if (rewrite == NULL || !rewrite->isFinal())
    {
      candidate_->logMVWasDisqualified1("grouping expression %s is not provided.", 
                                        groupingExpr->getID().data());
      return FALSE;
    }
  }
  else
  {
    assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL, 
                      groupingElem->getElementType() == ET_Column, QRLogicException, 
  		      "Expecting a QRColumn element.");
    const QRColumnPtr groupingCol = groupingElem->downCastToQRColumn();

    if (candidate_->getQueryDetails()->isFromJoin(groupingCol) && !isRecursive)
    {
      // This grouping column is part of an equality set.
      // Check each element of the equality set to see if any of them is Provided by the MV.
      const QRJoinPredPtr jp = candidate_->getQueryDetails()->getJoinPred(groupingCol);
      const ElementPtrList& eqList = jp->getEqualityList();
      CollIndex maxEntries = eqList.entries();
      for (CollIndex i=0; i<maxEntries; i++)
      {
        const QRElementPtr eqElem = eqList[i]->getReferencedElement();
        if (eqElem->getElementType() == ET_Column &&
            verifyGroupingElement(eqElem, TRUE) )
          return TRUE;
      } // End of for loop.

      // If we got here, none of the eqSet's columns is Provided by the MV.
      candidate_->logMVWasDisqualified1("The needed grouping column %s is not Provided.", 
			                groupingElem->getID().data());
      return FALSE;
    }
    // Get the MV base table object from the query table ID.
    const BaseTableDetailsPtr baseTable = candidate_->getMvTableForQueryID(groupingCol->getTableID(), FALSE);

    if (baseTable == NULL)
    {
      if (isRecursive)
        return FALSE;
      else
        rewrite = new(heap_) 
	  RewriteInstructionsItem(groupingElem, NULL, RC_OUTSIDE, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
    }
    else
    {
      const QROutputPtr mvOutput = 
        candidate_->getMvDetails()->getOutputByColumnName(baseTable->getTableElement()->getID(), 
							  groupingCol->getColumnName(), heap_);

      if (mvOutput == NULL)
      {
        // This column has a range predicate on it , where it is equal to a 
        // constant value, so it does not really need to be provided by the MV.
        if (candidate_->getQueryJbbDetails()->isConstColumn(&groupingCol->getID()))
          return TRUE;
      }

      // Check if the column is back-joinable only when this is not a recursive call 
      // that checks for columns of an equality set.
      NABoolean isBackJoinable = FALSE;
      if (!isRecursive)
        isBackJoinable = isBackJoinableColumn(groupingCol);
      if (mvOutput == NULL && !isBackJoinable)
      {
        if (!isRecursive)
        {
          // The grouping column needed for rewrite instructions is not
          // an output of the MV. 
          // Disqualify the MV.
          candidate_->logMVWasDisqualified1("The needed grouping column %s is not Provided.", 
			                    groupingCol->getFullyQualifiedColumnName().data());
        }
        return FALSE;
      }

      // Create rewrite instructions for the grouping column.
      rewrite = new(heap_) 
        RewriteInstructionsItem(groupingCol, mvOutput, RC_PROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));

      if (mvOutput == NULL && isBackJoinable)
      {
        rewrite->setResultStatus(RS_INTERMEDIATE);
        rewrite->addBackJoinTable(&groupingCol->getTableID());
        rewrite->setResultCode(RC_BACKJOIN);
        rewrite->setSecondaryResultCode(RC_BACKJOIN);
      }

      // For MJVs, add it to the output list.
      if (candidate_->getAggregateMatchingType() == AMT_MJV_AQ)
      {
        candidate_->addOutputColumn(rewrite);
      }

    }  // else found baseTable
  }  // else QRColumn

  addRewriteInstructions(rewrite);

  return TRUE;
}

// ***************************************************************************
// A generic method for generating the result descriptor elements.
// The specific details are implemented by sub-classes.
// ***************************************************************************
NABoolean MatchGroupingColumns::generateDescriptor(QRCandidatePtr resultDesc)
{
  // No GroupBy element to generate for MJVs.
  if (candidate_->getAggregateMatchingType() == AMT_MJV_AQ ||
      candidate_->getAggregateMatchingType() == AMT_MJV_JQ)
    return TRUE;

  QRGroupByPtr groupBy = new(heap_) QRGroupBy(ADD_MEMCHECK_ARGS(heap_));

  // Add a reference to the query NodeID of the GroupBy node.
  groupBy->setRef(candidate_->getQueryJbb()->getGroupBy()->getID());

  if (candidate_->getAggregateMatchingType() == AMT_MAV_AQ_MG)
  {
    groupBy->setResult(QRElement::Provided);
  }
  else
  {
    groupBy->setResult(QRElement::NotProvided);

    RewriteInstructionsItemList& rewriteList = getListOfFinalInstructions();
    for (CollIndex i=0; i<rewriteList.entries(); i++)
    {
      RewriteInstructionsItemPtr rewrite = rewriteList[i];
      if (rewrite->getResultCode() == RC_PROVIDED)
      {
        QRMVColumnPtr mvColumn = generateProvidedMvColumn(rewrite);
        groupBy->getPrimaryList()->addElement(mvColumn);
      }
      else
      {
        assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                          rewrite->getResultCode() == RC_OUTSIDE, QRLogicException, 
	      	          "Expecting only Outside and Provided grouping Columns.");

        QRColumnPtr column = generateNotProvidedColumn(rewrite);
        groupBy->getPrimaryList()->addElement(column);
      }
    }
  }

  resultDesc->setGroupBy(groupBy);

  return TRUE;
}  // MatchGroupingColumns::generateDescriptor()

//========================================================================
//  Class MatchJoinPreds
//========================================================================

// ***************************************************************************
// Run the Pass 1 algorithm of join predicate test.
// The goal is to find join predicates between tables covered by the MV, and
// tables not covered by the MV, and verify that the needed columns from 
// the MV tables are Provided by the MV.
// Join predicates on tables in the extra hub table list are kept for PASS 2.
// Returns TRUE if the MV passed this test, FALSE if it was disqualified.
// ***************************************************************************
NABoolean MatchJoinPreds::matchPass1()
{
  const QRJoinPredListPtr queryHubJoinPreds      = candidate_->getQueryJbb()->getHub()->getJoinPredList();
  const QRJoinPredListPtr queryExtraHubJoinPreds = candidate_->getQueryJbb()->getExtraHub()->getJoinPredList();

  // For each join pred in the hub
  if (queryHubJoinPreds != NULL)
  {
    CollIndex maxEntries = queryHubJoinPreds->entries();
    for (CollIndex i=0; i<maxEntries; i++)
    {
      const QRJoinPredPtr joinPred = (*queryHubJoinPreds)[i];
      if (analyzeJoinPredicate(joinPred, TRUE) == FALSE)
        return FALSE;
    }
  }

  // And in the extra-hub
  if (queryExtraHubJoinPreds != NULL)
  {
    CollIndex maxEntries = queryExtraHubJoinPreds->entries();
    for (CollIndex i=0; i<maxEntries; i++)
    {
      const QRJoinPredPtr joinPred = (*queryExtraHubJoinPreds)[i];
      if (analyzeJoinPredicate(joinPred, TRUE) == FALSE)
        return FALSE;
    }
  }

  return TRUE;
}  // MatchJoinPreds::matchPass1()

// ***************************************************************************
// Analyze a single join predicate to verify the needed columns are Provided
// by the MV.
// ***************************************************************************
NABoolean MatchJoinPreds::analyzeJoinPredicate(const QRJoinPredPtr joinPred, NABoolean isPass1)
{
  // For this join pred, are the equality set members from the MV hub?
  Int32 mvTables  = 0; // how many are from the MV hub?
  Int32 ehTables  = 0; // how many are from the MV extra-hub?
  Int32 outTables = 0; // how many are from Outside tables?
  ColumnNodesList  mvColumns(heap_); // Columns from the MV hub.
  NAPtrList<QRExprPtr>    mvExprs(heap_);   // Expressions from the MV hub.

  const ElementPtrList& eqList = joinPred->getList();
  QRJoinPredPtr referencedHubPred = NULL;
  for (CollIndex i=0; i<eqList.entries(); i++)
  {
    QRElementPtr elem = eqList[i];
    // An extra-hub join pred may reference a hub join pred.
    if (elem->getElementType() == ET_JoinPred)
      referencedHubPred = elem->downCastToQRJoinPred();
    else
      analyzeEQMember(elem, mvTables, ehTables, outTables, mvColumns, mvExprs, isPass1);
  } 

  if (referencedHubPred != NULL)
  {
    const ElementPtrList& eqList2 = referencedHubPred->getList();
    for (CollIndex i=0; i<eqList2.entries(); i++)
    {
      QRElementPtr elem = eqList2[i];
      analyzeEQMember(elem, mvTables, ehTables, outTables, mvColumns, mvExprs, isPass1);
    } 
  }

  // We went through all the elements in the equality set.
  // Now lets check what we found.
  if (ehTables > 0)
  {
    // This join pred uses MV extra-hub tables.
    // Postpose decision to PASS 2, when we know which are really used.
    RewriteInstructionsItemPtr rewrite = new(heap_) 
      RewriteInstructionsItem(joinPred, NULL, RC_NOTPROVIDED, RS_INTERMEDIATE, ADD_MEMCHECK_ARGS(heap_));
    addRewriteInstructions(rewrite);
    return TRUE;
  }
  else if (mvTables == 0)
  {
    // No extra-hub and no hub tables:
    // This join pred is only on tables that are outside the MV.
    // We can ignore it.
    return TRUE;
  }
  else if (outTables == 0)
  {
    // No extra-hub and no Outside tables:
    // This join pred is only on MV hub tables.
    // It must have been checked by MVMemo.
    // We can ignore it here.
    return TRUE;
  }
  else
  {
    // This join pred equates MV hub tables with outside tables with no extrahubs.
    // We need to verify that at least one of the columns used from the hub
    // is provided by the MV.
    QROutputPtr mvCol = NULL;
    NABoolean foundProvided = FALSE;
    QRColumnPtr hubCol = NULL;
    for (CollIndex k=0; k<mvColumns.entries(); k++)
    {
      hubCol = mvColumns[k];
      if (isProvidedColumn(hubCol, mvCol))
      {
        // We found a Provided MV Hub column.
        // Make sure its added to the Output list.
        // Use the ID of the column instead of the ID of the join predicate.
        RewriteInstructionsItemPtr rewrite = new(heap_) 
	  RewriteInstructionsItem(hubCol, mvCol, RC_PROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
	candidate_->addOutputColumn(rewrite, FALSE);
        // Avoid adding an Output element with the ID of the join predicate.
        candidate_->addOutputToAvoid(joinPred->getID());
        foundProvided = TRUE;
        break;
      }
      else
      {
        // This MV hub column is not Provided.
        // continue to the next one.
        continue;
      }
    } // for (k)
    
    if (!foundProvided && mvExprs.entries() > 0)
    {
      // None of the simple columns are Provided, need to check expressions.
      // Here we need all the input columns to be Provided.
      for (CollIndex m=0; m<mvExprs.entries(); m++)
      {
        if (analyzeProvidedExpression(mvExprs[m]))
          foundProvided = TRUE;
      } 
    }

    if (!foundProvided)
    {
      if (hubCol != NULL)
      {
        // None of the MV Hub column are Provided by the MV.
        // This candidate must be disqualified.
        const NAString& colName = hubCol->getFullyQualifiedColumnName();
        candidate_->logMVWasDisqualified1("column %s cannot be provided.", colName);
      }
      else
      {
        candidate_->logMVWasDisqualified1("join predicate %s cannot be provided.", joinPred->getID().data());
      }
      return FALSE;
    }
  } // MV hub and outside tables, no extrahub.

  return TRUE;
}  // MatchJoinPreds::checkJoinPredicate()


// ***************************************************************************
// Check if this equality set member is on an MV hub, extrahub table or 
// on an Outside table.
// ***************************************************************************
void MatchJoinPreds::analyzeEQMember(QRElementPtr   elem,
                                     Int32&         mvTables,
                                     Int32&         ehTables,
                                     Int32&         outTables,
                                     ColumnNodesList& mvColumns,
                                     NAPtrList<QRExprPtr>&   mvExprs,
                                     NABoolean      isPass1)
{
  // Get a simple column from this equality set member.
  QRColumnPtr col = elem->getFirstColumn();

  const NAString* extraHubID = NULL;
  NABoolean isOutside = isOutsideColumn(col, extraHubID);

  if (isPass1 == FALSE   &&
      extraHubID != NULL &&
      candidate_->isAUsedExtraHubTable(extraHubID))
  {
    // We are in Pass2, and this is an MV extra-hub table that is 
    // used by the query.
    // Consider it as an MV hub table.
    extraHubID = NULL;
  }

  if (isOutside)
  {
    // This table is Outside the MV.
    outTables++;
  }
  else if (extraHubID != NULL)
  {
    // This is an MV extra-hub table, and we are in Pass1.
    // Check if its in the used extra hub tables list.
    if (candidate_->isAUsedExtraHubTable(extraHubID))
      ehTables++;
    else
    {
      // If its in the MV extrahub, but not needed by the query,
      // its outside.
      outTables++;
    }
  }
  else
  {
    // This is an MV hub table.
    mvTables++;
    if (elem->getElementType() == ET_Column)
      mvColumns.insert(col);
    else
    {
      assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                        elem->getElementType() == ET_Expr, QRLogicException, 
                        "Expecting only QRExpr elements here.");
      // All the expression's input columns must be Provided.
      // Need to add as a group.
      mvExprs.insert(elem->downCastToQRExpr());
    }
  }
}

// ***************************************************************************
// Analyze an expression on an MV hub table, to check if all its input 
// columns are Provided by the MV.
// ***************************************************************************
NABoolean MatchJoinPreds::analyzeProvidedExpression(const QRExprPtr expr)
{
  RewriteInstructionsItemPtr rewrite = matchExpression(expr);

  if (rewrite == NULL || !rewrite->isFinal())
  {
    // The expression cannot be Provided by the MV.
    if (rewrite)
      deletePtr(rewrite);
    return FALSE;
  }

  // add the input columns to the output list.
  addInputsToOutputList(rewrite);

  // Provided means that the expression itself is Provided,
  // so add it to the Output list as well.
  if (rewrite->getResultCode()==RC_PROVIDED)
    candidate_->addOutputColumn(rewrite, FALSE);

  return TRUE;
}

// ***************************************************************************
// Run the Pass 2 algorithm of join predicates test.
// Returns TRUE if the MV passed this test, FALSE if it was disqualified.
// ***************************************************************************
NABoolean MatchJoinPreds::matchPass2OnElement(RewriteInstructionsItemPtr pending)
{
  QRJoinPredPtr joinPred = pending->getQueryElement()->downCastToQRJoinPred();
  NABoolean result = analyzeJoinPredicate(joinPred, FALSE);
  return result;
}  // MatchJoinPreds::matchPass2OnElement()

// ***************************************************************************
// ***************************************************************************
NABoolean MatchJoinPreds::generateDescriptor(QRCandidatePtr resultDesc)
{
  IDSet& backJoinTables = candidate_->getBackJoinTables();
  if (backJoinTables.entries() == 0)
    return TRUE;

  // For each back-join table
  for (CollIndex i=0; i<backJoinTables.entries(); i++)
  {
    const NAString* tableID = backJoinTables[i];
    const QRTablePtr table = candidate_->getQueryDetails()->getElementForID(*tableID)->downCastToQRTable();

    // Add the join predicate to the back-join table.
    if (addBackJoinPredsForTable(table, resultDesc) == FALSE)
    {
      // if the creation of the join pred was not successful, 
      // Disqualify the MV.
      candidate_->logMVWasDisqualified1("table %s cannot be back-joined.", 
                                         table->getTableName().data());
      return FALSE;
    }

    // Add the Table element to the TableList.
    QRTablePtr newTableElement = new (heap_) QRTable(ADD_MEMCHECK_ARGS(heap_));
    newTableElement->setTableName(table->getTableName());
    newTableElement->setRef(table->getID());
    resultDesc->addBackJoinTable(newTableElement);
  }

  return TRUE;
}

// ***************************************************************************
// ***************************************************************************
NABoolean MatchJoinPreds::addBackJoinPredsForTable(const QRTablePtr table,
                                                   QRCandidatePtr   resultDesc)
{
  const QRKeyPtr theKey = table->getKey();
  if (theKey == NULL)
    return FALSE;

  const BaseTableDetailsPtr baseTable = candidate_->getMvTableForQueryID(table->getID());
  const QRTablePtr mvTable = baseTable->getTableElement();
  CollIndex maxEntries = theKey->entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    // Construct the new JoinPred element
    QRJoinPredPtr joinPred = new(heap_) QRJoinPred(ADD_MEMCHECK_ARGS(heap_));

    QRColumnPtr keyCol = theKey->getElement(i)->getReferencedElement()->downCastToQRColumn();

    // Create the table column side of the join pred.
    QRColumnPtr tableColumn = new (heap_) QRColumn(ADD_MEMCHECK_ARGS(heap_));
    tableColumn->setFullyQualifiedColumnName(keyCol->getFullyQualifiedColumnName());
    tableColumn->setRef(keyCol->getID());
    tableColumn->setTableID(keyCol->getTableID());
    joinPred->addElement(tableColumn);

    // Create the MV column side of the join pred.
    QROutputPtr mvOutput = NULL;
    if (!isProvidedColumn(keyCol, mvOutput))
      return FALSE;

    QRMVColumnPtr mvColumn = new (heap_) QRMVColumn(ADD_MEMCHECK_ARGS(heap_));
    mvColumn->setMVColName(mvOutput->getName());                              
    joinPred->addElement(mvColumn);

    // Add the new JoinPred to the result descriptor.
    resultDesc->addJoinPred(joinPred);
  }

  return TRUE;
}

//========================================================================
//  Class MatchOuterJoins
//========================================================================

// ***************************************************************************
// Go over every table in the hub and extrahub, in both query and MV, 
// and check if they are the inner child of a left join.
// Possibilities are:
//     Query  MV     Case                         Result
// 1   No     No     Normal inner join            Do nothing.
// 2   Yes    No     LOJ query on inner join MV   Disqualify.
// 3   No     Yes    Inner join query on LOJ MV   Add NOT NULL predicate.
// 4   Yes    No     Both query and MV use LOJ    Do Nothing.
// ***************************************************************************
NABoolean MatchOuterJoins::matchPass1()
{
  // Quick shortcut check
  if (!candidate_->getMvDetails()->getJbbDetails()->hasLOJs() &&
      !candidate_->getQueryJbbDetails()->hasLOJs() )
  {
    // Neither the query nor the MV have any LOJs.
    return TRUE;
  }

  const QRJBBCListPtr  queryHubTables      = candidate_->getQueryJbb()->getHub()->getJbbcList();
  const QRTableListPtr queryExtraHubTables = candidate_->getQueryJbb()->getExtraHub()->getTableList();

  // Prepare a list of all hub tables and used extrahub tables.
  NAPtrList<QRTablePtr> allTables(heap_);
  
  CollIndex maxEntries = queryHubTables->entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    const QRElementPtr queryJBBC = queryHubTables->getList()[i];
    if (queryJBBC->getElementType() != ET_Table)
      continue;
    const QRTablePtr queryTable = queryJBBC->downCastToQRTable();
    allTables.insert(queryTable);
  }
 
  if (queryExtraHubTables)
  {
    maxEntries = queryExtraHubTables->entries();
    for (CollIndex i=0; i<maxEntries; i++)
    {
      const QRTablePtr queryTable = queryExtraHubTables->getList()[i];
      if (candidate_->isAUsedExtraHubTable(&queryTable->getID()))
        allTables.insert(queryTable);
    }
  }

  // Now go over the tables in the list
  maxEntries = allTables.entries();
  for (CollIndex j=0; j<maxEntries; j++)
  {
    // Get the LOJ flag
    const QRTablePtr queryTable = allTables[j];
    NABoolean queryTableIsLOJ = queryTable->hasLOJParent();
    const BaseTableDetailsPtr mvTableDetails = candidate_->getMvTableForQueryID(queryTable->getID(), FALSE);
    if (mvTableDetails == NULL)
      continue; // Its an Outside table (not used by the MV).

    const QRTablePtr mvTable = mvTableDetails->getTableElement();
    NABoolean mvTableIsLOJ = mvTable->hasLOJParent();
    
    // Check the Result
    if (queryTableIsLOJ && !mvTableIsLOJ)
    {
      candidate_->logMVWasDisqualified1("Table %s is an inner table of an outer join in the query but not in the MV.", 
                                         queryTable->getTableName().data());
      return FALSE;
    }
    
    if (!queryTableIsLOJ && mvTableIsLOJ)
    {
      return addTheNotNullPred(queryTable, mvTableDetails);
    }    
  }
  return TRUE;
}

// ***************************************************************************
// Take the query table, get the key columns, and iterate over them to find 
// one that is not nullable, and also provided by the MV.
// If found - add the NOT NOLL predicate on it.
// Otherwise - disqualify the MV.
// ***************************************************************************
NABoolean MatchOuterJoins::addTheNotNullPred(const QRTablePtr queryTable, const BaseTableDetailsPtr mvTableDetails)
{
  const ElementPtrList& keyColumns = queryTable->getKey()->getList();
  CollIndex maxEntries = keyColumns.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    const QRColumnPtr keyCol = keyColumns[i]->getReferencedElement()->downCastToQRColumn();
    if (keyCol->isNullable())  // Just making sure...
      continue;
      
    // Maybe there already is a range predicate that does not allow nulls?
    const QRRangePredPtr rangePred = mvTableDetails->getRangeColumnPredicatesFor(keyCol->getColIndex());
    if (rangePred != NULL)
    {
      const RangeSpec* rangeSpec = rangePred->getRangeSpec(heap_);
      if (!rangeSpec->isNullIncluded())
      {
      	// The query already has a range predicate on this column that 
      	// does not allow nulls, so there is no need to add it.
      	return TRUE;
      }	
    }

    // No such range predicate found. We need to add the NOT NULL predicate.
    // Is the column Provided by the MV?
    QROutputPtr mvCol = NULL;
    if (!isProvidedColumn(keyCol, mvCol))
      continue;
    
    RewriteInstructionsItemPtr rewrite = new(heap_) 
      RewriteInstructionsItem(keyCol, mvCol, RC_PROVIDED, RS_FINAL, ADD_MEMCHECK_ARGS(heap_));
    addRewriteInstructions(rewrite);
    
    return TRUE;
  }
  
  candidate_->logMVWasDisqualified1("table %s key columns are not provided for NOT NULL predicate.", 
                                     queryTable->getTableName().data());
  return FALSE; // No eligible key column found.
}

// ***************************************************************************
// Not used.
// ***************************************************************************
NABoolean MatchOuterJoins::matchPass2OnElement(RewriteInstructionsItemPtr pending)
{
  return TRUE;
}

// ***************************************************************************
// For each column in the rewrite instructions, add a NOT NULL range predicate.
// ***************************************************************************
NABoolean MatchOuterJoins::generateDescriptor(QRCandidatePtr resultDesc)
{
  QRRangePredListPtr rangePredicates = resultDesc->getRangePredList();
  
  RewriteInstructionsItemList& rewriteList = getListOfFinalInstructions();
  for (CollIndex i=0; i<rewriteList.entries(); i++)
  {
    RewriteInstructionsItemPtr rewrite = rewriteList[i];

    // Construct the new Range element
    QRRangePredPtr rangeElement = new(heap_) QRRangePred(ADD_MEMCHECK_ARGS(heap_));
    // Set the result attribute.
    rangeElement->setResult(QRElement::NotProvided);
    // Do the range item - the MV column.
    QRMVColumnPtr rangeItem = generateProvidedMvColumn(rewrite);
    rangeElement->setRangeItem(rangeItem);

    // Don't add a RangeSpec - an empty range means NOT NULL.
    // Add it to the result descriptor.
    rangePredicates->addItem(rangeElement);
  }
  
  return TRUE;
}

//========================================================================
//  Class AggregateCollectorVisitor
//========================================================================

// ***************************************************************************
// ***************************************************************************
Visitor::VisitResult AggregateCollectorVisitor::visit(QRElementPtr caller)
{
  switch(caller->getElementType())
  {
    case ET_Function:
    {
      QRFunctionPtr func = caller->downCastToQRFunction();
      if (func->isAnAggregate())
      {
        aggregateFunctions_.insert(func);
        return VR_Skip;
      }
    }
    break;

    case ET_Column:
      simpleColumns_.insert(caller->downCastToQRColumn());
      return VR_Skip;
      break;

    case ET_JoinPred:
      joinedColumns_.insert(caller->downCastToQRJoinPred());
      return VR_Skip;
      break;
  }

  return VR_Continue;
}
 
