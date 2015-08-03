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
// File:         QmsWorkloadAnalysis.cpp
// Description:  Includes methods for the ProposedMV and WorkloadAnalysis classes.
//
// Created:      05/17/2011
// ***********************************************************************

#include <fstream>
#include "QmsWorkloadAnalysis.h"
#include "QRLogger.h"

//========================================================================
//  Class ProposedMV
//========================================================================

// ***********************************************************************
// ***********************************************************************
ProposedMV::~ProposedMV()
{
  QRTRACER("~ProposedMV()");

  CollIndex i;
  // The list of MVPairs needs to be deleted.
  CollIndex howmanyPairs = mvPairList_.entries();
  for (i=0; i<howmanyPairs; i++)
    deletePtr(mvPairList_[i]);
  mvPairList_.clear();

  // Delete the outputs that were added for this Proposed MV.
  CollIndex howmanyOutputs = addedOutputs_.entries();
  for (i=0; i<howmanyOutputs; i++)
    deletePtr(addedOutputs_[i]);
  addedOutputs_.clear();

  // Delete the table IDs of tables we add an LOJ flag to.
  CollIndex howmanyTables = addedLojTables_.entries();
  for (i=0; i<howmanyTables; i++)
    delete addedLojTables_[i];

  mapList_.clear();

  for (i=0; i<stuffToDelete_.entries(); i++)
  {
    QRElementPtr elem = stuffToDelete_[i];
    // This delete is crashing.
    //delete elem;
  }
  stuffToDelete_.clear();
}

// ***********************************************************************
// ***********************************************************************
const NAString* ProposedMV::getHashKeyForElement(const QRElementPtr elem)
{
  const QRElementPtr trueElem = elem->getReferencedElement();
  if (trueElem->getElementType() == ET_Expr)
  {
    QRExprPtr expr = trueElem->downCastToQRExpr();
    if (expr->getInfo()!=NULL) 
      return new(heap_) NAString(expr->getInfo()->getText(), heap_);
    else
      return new(heap_) NAString(expr->getSortName(), heap_);
  }
  else if (trueElem->getElementType() == ET_Column)
  {
    QRColumnPtr col = trueElem->downCastToQRColumn();
    return mapList_[0]->getMVDetails()->calcIDQualifiedColumnName(col->getTableID(), col->getColumnName(), heap_);
  }  
  else
  {
    return new(heap_) NAString(trueElem->getSortName(), heap_);
  }
}

// ***********************************************************************
// The first MV in the list is picked to initialize the internal data 
// structures, and all the other MVs are later compared to it.
// ***********************************************************************
void ProposedMV::initFrom(QRJoinSubGraphMapPtr map)
{
  QRTRACER("ProposedMV::initFrom()");

  MVDetailsPtr mv = map->getMVDetails();
  hasGroupBy_ = map->getSubGraph()->hasGroupBy();

  // The GroupBy and select lists start empty (additions to the first MV)
  // Copy the lists of range and residual predicates, so some of them 
  // can be removed.
  QRJBBPtr jbb = mv->getJbbDetails()->getJbbDescriptor();
  QRHubPtr hub = jbb->getHub();

  // Initialize select list
  // Select list elements are inserted into a hash table so we can 
  // easily check for duplicates.
  const QROutputListPtr outputs = jbb->getOutputList();
  CollIndex maxEntries = outputs->entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    const QROutputPtr  output = (*outputs)[i];
    const QRElementPtr outputItem = output->getOutputItem();
    const NAString* outName = getHashKeyForElement(outputItem);
    selectList_.insert(outName, output);
    selectArray_.insertAt(output->getColPos(), output);
  }

  // Initialize GroupBy list.
  // GroupBy list elements are inserted into a hash table so we can 
  // easily check for duplicates.
  if (jbb->getGroupBy()                   && 
      jbb->getGroupBy()->getPrimaryList() && 
      jbb->getGroupBy()->getPrimaryList()->entries() > 0)
  {
    const ElementPtrList& groupBy = jbb->getGroupBy()->getPrimaryList()->getList();
    CollIndex maxEntries = groupBy.entries();
    for (CollIndex i=0; i<maxEntries; i++)
    {
      QRElementPtr gb = groupBy[i]->getReferencedElement();
      const NAString* gbName = getHashKeyForElement(gb);
      groupingColumns_.insert(gbName, gb);
    }
  }

  // Initialize range predicates
  QRRangePredListPtr rangePreds = hub->getRangePredList();
  if (rangePreds && rangePreds->entries())
    rangePredList_.insert(rangePreds->getList());

  // Initialize residual predicates.
  QRResidualPredListPtr residualPreds = hub->getResidualPredList();
  if (residualPreds && residualPreds->entries())
    residualPredList_.insert(residualPreds->getList());
}

// ***********************************************************************
// * Add a single MVDEtails object to the list.
// ***********************************************************************
void ProposedMV::addMV(MVDetailsPtr mv, QRJoinSubGraphMapPtr map)
{
  QRTRACER("ProposedMV::addMV()");
  map->setMVDetails(mv);
  mapList_.insert(map);

  if (!isInitialized_)
  {
    // Initialize from the first MV inserted.
    isInitialized_ = TRUE;
    initFrom(map);
  }
}

// ***********************************************************************
// * Add a list of MVDEtails objects.
// ***********************************************************************
void ProposedMV::addMVs(const SubGraphMapList& maps)
{
  QRTRACER("ProposedMV::addMVs()");
  mapList_.insert(maps);

  if (!isInitialized_)
  {
    // Initialize from the first MV inserted.
    isInitialized_ = TRUE;
    initFrom(maps[0]);
  }
}

// ***********************************************************************
// * Set the name of the proposed MV, based on its number.
// ***********************************************************************
void ProposedMV::setName(Int32 num)
{
  QRTRACER("ProposedMV::setName()");
  char buffer[16];
  sprintf(buffer, "ProposedMV%d", num);
  name_ = buffer;
}

// ***********************************************************************
// Create the list of MVPair objects.
// Each MVPair is composed of the first query, and one of the others.
// So that the first query is compared to all the rest.
// ***********************************************************************
void ProposedMV::initializeMVPairList()
{
  QRTRACER("ProposedMV::initializeMVPairList()");
  QRJoinSubGraphMapPtr firstQuery = mapList_[0];
  CollIndex howmanyQueries = mapList_.entries();
  for (CollIndex i=1; i<howmanyQueries; i++)
  {
    QRJoinSubGraphMapPtr otherQuery = mapList_[i];
    MVPairPtr pair = new (heap_) 
      MVPair(firstQuery, otherQuery, ADD_MEMCHECK_ARGS_PASS(heap_));
    mvPairList_.insert(pair);
  }
}

// ***********************************************************************
// * Analyze the range and residual predicates of the MVs to find the
// * subset of common predicates.
// ***********************************************************************
void ProposedMV::findReducedPredicateSet()
{
  QRTRACER("ProposedMV::findReducedPredicateSet()");
  // If this Proposed MV represents a single query, we still need to 
  // go over the predicate list and remove NotProvided predicates.
  //if (mapList_.entries() == 1)
  //  return;

  // Create the list of MVPair objects.
  initializeMVPairList();

  // Start with the select list
  findInclusiveSelectList();

  // Check the bitmaps to see if we can skip the range and residual analysis.
  checkBitmapsFirst();
  handleRangePredicates();
  handleResidualPredicates();
}

// ***********************************************************************
// Make sure the proposed MV's select list includes all the columns 
// required by all the MVs.
// ***********************************************************************
void ProposedMV::findInclusiveSelectList()
{
  QRTRACER("ProposedMV::findInclusiveSelectList()");

  MVDetailsPtr firstMV = mapList_[0]->getMVDetails();

  CollIndex howmanyPairs = mvPairList_.entries();
  for (CollIndex queryNo=0; queryNo<howmanyPairs; queryNo++)
  {
    MVPairPtr thisPair = mvPairList_[queryNo];
    QROutputListPtr  outputList = 
      thisPair->getQueryDetails()->getJbbDetailsList()[0]->getJbbDescriptor()->getOutputList();

    CollIndex maxOutputs = outputList->entries();
    for (CollIndex i=0; i<maxOutputs; i++)
    {
      QROutputPtr output = (*outputList)[i];
      addElementToSelectList(output->getOutputItem());
    }
  }
}

// ***********************************************************************
// By checking the predicate bitmaps, we can quickly find if there are
// no common predicate columns to the query set.
// In this case, we can skip the detailed analysis, and eliminate all 
// the range and/or residual predicates from the proposed MV.
// ***********************************************************************
void ProposedMV::checkBitmapsFirst()
{
  QRTRACER("ProposedMV::checkBitmapsFirst()");
  // Get the list of Hub tables of the first query (which is common to all).
  const ElementPtrList& jbbcs = 
    mapList_[0]->getMVDetails()->getJbbDetails()->getJbbDescriptor()->getHub()->getJbbcList()->getList();
  CollIndex pairs = mvPairList_.entries();
  // These flags are set to TRUE when we find tables for which there are 
  // columns that have range or residual predicates in all the workload queries.
  NABoolean anyTableWithCommonRangePreds = FALSE;
  NABoolean anyTableWithCommonResidualPreds = FALSE;

  // For each table in the hub
  CollIndex maxEntries=jbbcs.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    // Get the table from the first query.
    const QRTablePtr table = jbbcs[i]->downCastToQRTable();
    const NAString& tableID = table->getID();

    // Create work bitmaps.
    XMLBitmap  rangeBitmap(heap_); 
    XMLBitmap  residualBitmap(heap_);
    // Initialize them from the table of the first query
    rangeBitmap = table->getRangeBits();
    residualBitmap = table->getResidualBits();

    // We can skip the loop if there are no range and residual preds.
    if (!rangeBitmap.isEmpty() || !residualBitmap.isEmpty())
    {
      // Intersect the bitmap with the bitmap from the corresponding table
      // of each pair.
      for (CollIndex j=0; j<pairs; j++)
      {
        MVPairPtr pair = mvPairList_[j];
        pair->intersectColumnBitmapsForTable(tableID , rangeBitmap, residualBitmap);

        // If both bitmaps are empty (no common columns), quit the loop.
        if (rangeBitmap.isEmpty() && residualBitmap.isEmpty())
          break;
      }
    }

    // If this table for every query has any common used columns,
    // Than we need to run the full range and/or residual analysis.
    anyTableWithCommonRangePreds    |= rangeBitmap.isEmpty();
    anyTableWithCommonResidualPreds |= residualBitmap.isEmpty();

    // If we already know that we can't skip both tests, quit the loop.
    if (anyTableWithCommonRangePreds && anyTableWithCommonResidualPreds)
      break;
  }  // for (jbbcs)

  if (!anyTableWithCommonRangePreds)
  {
    // We can skip the range predicate full analysis.
    canSkipRangePreds_ = TRUE;
    // Make sure all the needed columns are provided by the proposed MV.
    for (CollIndex predNo=0; predNo<rangePredList_.entries(); predNo++)
      handleRemovedRangePredicate(rangePredList_[predNo]);
    rangePredList_.clear();
  }

  if (!anyTableWithCommonResidualPreds)
  {
    // We can skip the residual  predicate full analysis.
    canSkipResidualPreds_ = TRUE;
    // Make sure all the needed columns are provided by the proposed MV.
    for (CollIndex predNo=0; predNo<residualPredList_.entries(); predNo++)
      handleRemovedResidualPredicate(residualPredList_[predNo]);
    residualPredList_.clear();
  }
}

// ***********************************************************************
// Do the full analysis for range predicates.
// ***********************************************************************
void ProposedMV::handleRangePredicates()
{
  QRTRACER("ProposedMV::handleRangePredicates()");
  if (canSkipRangePreds_)
    return;

  CollIndex howmanyPairs = mvPairList_.entries();

  // This loop goes backwards, so we can delete entries as we go.
  for (Int32 predNo=rangePredList_.entries()-1; predNo>=0; predNo--)
  {
    NABoolean foundInAll = TRUE;
    QRRangePredPtr querySidePred = rangePredList_[predNo];

    if (querySidePred->getResult() == QRElement::NotProvided)
    {
      // This predicate was skipped by the compiler, and is not fully described
      // in the descriptor, because its either too big or uses problematic characters.
      // Only its input columns are listed, and those should be added to the 
      // proposed MV's select list.
      rangePredList_.removeAt(predNo);
      handleRemovedRangePredicate(querySidePred);
    }

    if (querySidePred->getRangeItem()->getElementType() == ET_Expr &&
    	  querySidePred->getRangeItem()->downCastToQRExpr()->getExprRoot()->containsAnAggregate(heap_))
    {
    	// This is a HAVING predicate. If we leave it here, we will not be able 
    	// to rollup the proposed MV to match queries.
    	// The aggregate function of the HAVING predicate is typically 
    	// already in the select list, so just skip it.
      rangePredList_.removeAt(predNo);
    }

    for (CollIndex queryNo=0; foundInAll && queryNo<howmanyPairs; queryNo++)
    {
      MVPairPtr thisPair = mvPairList_[queryNo];
      if (thisPair->checkRangePredicate(querySidePred) == FALSE)
      {
        // This range predicate was not found in this query.
        // Remove the predicate from the proposed MV
        // and add its input columns to the select list + GroupBy.
        foundInAll = FALSE;
        rangePredList_.removeAt(predNo);
        handleRemovedRangePredicate(querySidePred);
      }  // if not found
    }  // for queryNo
  }  // for predNo

  // We are done with the firstMV range preds.
  // Now check if there ar any uncovered preds on the other MVs
  for (CollIndex queryNo=0; queryNo<howmanyPairs; queryNo++)
  {
    MVPairPtr thisPair = mvPairList_[queryNo];
    NAPtrList<QRRangePredPtr>& remainingPreds = thisPair->getRemainingRangePreds();
    for (CollIndex predNo=0; predNo<remainingPreds.entries(); predNo++)
    {
      // Get the list of range preds we did not cover
      QRRangePredPtr otherPred = remainingPreds[predNo];
      QRElementPtr rangeItem = otherPred->getRangeItem();
      QRColumnPtr rangeColumn = NULL;

      if (rangeItem->getElementType() == ET_Column)
      {
        rangeColumn = rangeItem->downCastToQRColumn();
      }
      else
      {
	// Find all the MV range preds using the same expression text
	const QRExprPtr rangeExpr = rangeItem->downCastToQRExpr();
        const ElementPtrList& inputColumns = rangeExpr->getInputColumns(heap_);
	// A range expression can have only one input column.
	rangeColumn = inputColumns[0]->downCastToQRColumn();
      }

      rangeColumn = rangeColumn->getReferencedElement()->downCastToQRColumn();

      addMvSideColumnToSelectList(rangeColumn, thisPair);

    } // for predNo
  }  // for queryNo
}

void ProposedMV::addMvSideColumnToSelectList(const QRColumnPtr mvCol, MVPairPtr thisPair)
{
  // OK, we have the "MV" side (otherMV) column.
  // Now we need to translate that to the query side (firstMV).
  MVDetailsPtr queryDetails = thisPair->getQuerySubGraphMap()->getMVDetails();
  const BaseTableDetailsPtr baseTable = 
    thisPair->getQueryTableForMvID(mvCol->getTableID());

  // Look it up in the query side hash table.
  const NAString& querySideTableID = baseTable->getTableElement()->getID();
  QRElementPtr queryCol = 
    queryDetails->getOutputByColumnName(querySideTableID, 
	                                mvCol->getColumnName(), heap_);

  if (queryCol)
  {
    // The query side MV (the proposed MV) already has an output for this column.
    // No need to add it - Job's done.
  }
  else
  {
    // We need to add this column to the proposed MV, but we don't have a QRColumn
    // element from the "query" side.
    // So let's create one using the copy Ctor.
    QRColumnPtr newColumn = new (heap_) QRColumn(*mvCol, ADD_MEMCHECK_ARGS(heap_));
    // Just fix the table ID to the correct descriptor.
    newColumn->setTableID(querySideTableID);
    addElementToSelectList(newColumn);
    // Don't forget to delete it when we are done.
    stuffToDelete_.insert(newColumn);  
  }
}

// ***********************************************************************
// Remove a range predicate from the proposed MV
// and add its input columns to the select list + GroupBy.
// ***********************************************************************
void ProposedMV::handleRemovedRangePredicate(QRRangePredPtr pred)
{
  QRTRACER("ProposedMV::handleRemovedRangePredicates()");
  const QRElementPtr inp = pred->getRangeItem()->getReferencedElement();
  const QRColumnPtr rangeCol = inp->getFirstColumn();  // Handle join preds and expressions.
//  assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_ERROR, 
//                    (rangeCol), QRLogicException, 
//		    "Problem getting range column.");
  // Temporary fix !!!
  if (rangeCol == NULL)
    return; 

  addElementToSelectList(rangeCol);
}

// ***********************************************************************
// Do the full analysis for residual predicates.
// ***********************************************************************
void ProposedMV::handleResidualPredicates()
{
  QRTRACER("ProposedMV::handleResidualPredicates()");
  
  if (canSkipResidualPreds_)
    return;

  CollIndex howmanyPairs = mvPairList_.entries();

  // This loop goes backwards, so we can delete entries as we go.
  for (Int32 predNo=residualPredList_.entries()-1; predNo>=0; predNo--)
  {
    NABoolean foundInAll = TRUE;
    QRExprPtr querySidePred = residualPredList_[predNo];
    
    if (querySidePred->getResult() == QRElement::NotProvided)
    {
      // This predicate was skipped by the compiler, and is not fully described
      // in the descriptor, because its either too big or uses problematic characters.
      // Only its input columns are listed, and those should be added to the 
      // proposed MV's select list.
      residualPredList_.removeAt(predNo);
      handleRemovedResidualPredicate(querySidePred);
    }

    if (querySidePred->getExprRoot()->containsAnAggregate(heap_))
    {
    	// This is a HAVING predicate. If we leave it here, we will not be able 
    	// to rollup the proposed MV to match queries.
    	// The aggregate function of the HAVING predicate is typically 
    	// already in the select list, so just skip it.
      residualPredList_.removeAt(predNo);
    }

    for (CollIndex queryNo=0; foundInAll && queryNo<howmanyPairs; queryNo++)
    {
      if (mvPairList_[queryNo]->checkResidualPredicate(querySidePred) == FALSE)
      {
        // This residual predicate was not found in this query.
        // Remove the predicate from the proposed MV
        // and add its input columns to the select list + GroupBy.
        foundInAll = FALSE;
        residualPredList_.removeAt(predNo);
        handleRemovedResidualPredicate(querySidePred);
      }  // if not found
    }  // for queryNo
  }  // for predNo

  // We are done with the firstMV residual preds.
  // Now check if there ar any uncovered preds on the other MVs
  for (CollIndex queryNo=0; queryNo<howmanyPairs; queryNo++)
  {
    MVPairPtr thisPair = mvPairList_[queryNo];
    NAPtrList<QRExprPtr>& remainingPreds = thisPair->getRemainingResidualPreds();
    for (CollIndex predNo=0; predNo<remainingPreds.entries(); predNo++)
    {
      // Get the list of residual preds we did not cover
      QRExprPtr otherPred = remainingPreds[predNo]->downCastToQRExpr();

      // Find all the MV residual predicate input columns
      const ElementPtrList& inputColumns = otherPred->getInputColumns(heap_);
      CollIndex maxEntries = inputColumns.entries();
      for (CollIndex inp=0; inp<maxEntries; inp++)
      {
        QRColumnPtr inputColumn = inputColumns[inp]->downCastToQRColumn();
        addMvSideColumnToSelectList(inputColumn, thisPair);
      }

    } // for predNo
  }  // for queryNo
}

// ***********************************************************************
// Add the input columns of this residual predicate to the 
// select list + GroupBy.
// ***********************************************************************
void ProposedMV::handleRemovedResidualPredicate(QRExprPtr pred)
{
  QRTRACER("ProposedMV::handleRemovedResidualPredicates()");
  const ElementPtrList& inputColumns = pred->getInputColumns(heap_);
  for (CollIndex inp=0; inp<inputColumns.entries(); inp++)
  {
    QRColumnPtr col = inputColumns[inp]->getFirstColumn()->downCastToQRColumn();
    addElementToSelectList(col);
  }
}

// ***********************************************************************
// ***********************************************************************
void ProposedMV::handleLeftOuterJoins()
{
  QRTRACER("ProposedMV::handleLeftOuterJoins()");

  CollIndex howmanyPairs = mvPairList_.entries();

  QRJBBPtr jbb = mapList_[0]->getMVDetails()->getJbbDetails()->getJbbDescriptor();
  const QRJBBCListPtr  queryHubTables = jbb->getHub()->getJbbcList();

  CollIndex maxEntries = queryHubTables->entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    const QRElementPtr queryJBBC = queryHubTables->getList()[i];
    if (queryJBBC->getElementType() != ET_Table)
      continue;
    const QRTablePtr queryTable = queryJBBC->downCastToQRTable();
    if (queryTable->hasLOJParent())
    {
      // This table already is under an LOJ, no need to add it.
      continue;
    }

    NABoolean foundInAny = FALSE;
    for (CollIndex queryNo=0; !foundInAny && queryNo<howmanyPairs; queryNo++)
    {
      if (mvPairList_[queryNo]->checkForLOJ(queryTable) == TRUE)
      {
        // This LOJ table was not found in this query.
        // Add it to the list of added LOJ tables.
        foundInAny = TRUE;
        addedLojTables_.insert(&queryTable->getID());
      }  // if not found
    }  // for queryNo
  }  // for predNo
}

// ***********************************************************************
// ***********************************************************************
NABoolean ProposedMV::isAddedLojTable(const NAString& id)
{
  return addedLojTables_.contains(&id);
}

// ***********************************************************************
// A predicate using this column was removed from the proposed MV.
// To make sure it can be applied on top of the proposed MV by MVQR,
// it must be added to the select list, and therefore to the GroupBy list 
// as well, while avoiding duplicates in both cases.
// ***********************************************************************
void ProposedMV::addElementToSelectList(const QRElementPtr elem)
{
  QRTRACER("ProposedMV::addElementToSelectList()");
  const NAString* colName = getHashKeyForElement(elem);

  // Add the column to the GroupBy list if not already in there.
  if (hasGroupBy_ && elem->getElementType() == ET_Column)
  {
    if (groupingColumns_.getFirstValue(colName) == NULL)
      groupingColumns_.insert(colName, elem);
  }

  // Add the column or expression to the select list if not already in there.
  if (selectList_.getFirstValue(colName) == NULL)
  {
    const QROutputPtr newOutput = new (heap_) QROutput(ADD_MEMCHECK_ARGS(heap_));
    newOutput->setOutputItem(elem);
    Int32 ordinal = selectList_.entries() + 1;
    newOutput->setColPos(ordinal);
    selectList_.insert(colName, newOutput);
    addedOutputs_.insert(newOutput);
    selectArray_.insertAt(ordinal, newOutput);
  }
}

// ***********************************************************************
// Create the text for the output file.
// ***********************************************************************
void ProposedMV::reportResults(NAString& text, NABoolean addComments)
{
  QRTRACER("ProposedMV::reportResults()");
  char buffer[100];
  assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_ERROR,
                    mapList_.entries() > 0, QRLogicException,
                    "Proposed MV with no MVDetails objects.");

  // Provide the proposed MV's name.
  if (addComments)
  {
    sprintf(buffer, "-- ProposedMV: %.80s\n", name_.data());
    text += buffer;

    // Provide the list of queries it uses.
    CollIndex queries  = mapList_.entries();
    sprintf(buffer, "-- Covers %d queries:\n", queries);
    text += buffer;
    for (CollIndex i=0; i<queries; i++)
    {
      text += "--   id: ";
      text += mapList_[i]->getMVDetails()->getMVName();
      text += "\n";
    }
  }

  Int32 joinSize = getJoinSize();
  sprintf(buffer, "-- Join size is: %d.\n", joinSize);
  text += buffer;

//QRLogger::instance().log(CAT_QMS_MAIN, LL_DEBUG, "Beginning MV:");
//QRLogger::instance().log(CAT_QMS_MAIN, LL_DEBUG, text.data());

  // Provide the CREATE MV sql.
  toSQL(text);

  if (addComments)
    text += "--=========================================================================\n\n";
}

// ***********************************************************************
// Create SQL for the CREATE MV command.
// ***********************************************************************
void ProposedMV::toSQL(NAString& text)
{
  NAString havingClause;
  QRTRACER("ProposedMV::toSQL()");
  text += "CREATE MV ";
  text += name_;
  text += "\n  REFRESH BY USER INITIALIZE BY USER ENABLE QUERY REWRITE AS\n";

  unparseSelectClause(text);
  unparseFromClause(text);
  unparseWhereAndHavingClauses(text, havingClause);
  unparseGroupByClause(text);

  if (havingClause != "")
  {
    text += "  HAVING  ";
    text += havingClause;
  }

  text += ";\n";
}

// ***********************************************************************
// Generate the text for the SELECT clause.
// ***********************************************************************
void ProposedMV::unparseSelectClause(NAString& text)
{
  QRTRACER("ProposedMV::unparseSelectClause()");
  text += "  SELECT  ";
 
  CollIndex maxEntries = selectArray_.entries();
  for (CollIndex i=1; i<=maxEntries; i++)
  {
    QROutputPtr output = selectArray_[i];
    if (output)
    {
      if (i>1)
        text += "          ";
      text += unparseColumnOrExpr(output->getOutputItem()->getReferencedElement());
      text += " ";
      text += output->getName();

      // Add the output name, derived from its ordinal.
      char buffer[10];
      sprintf(buffer, "Col%d", i);
      text += buffer;

      if (i<maxEntries)
        text += ", ";
      text += "\n";
    }
  }
  
  if (hasGroupBy_ && mapList_[0]->getMVDetails()->getCountStar() == NULL)
  {
  	// If the MV has a GroupBy, but no COUNT(*), its a good idea to add it
  	// for future rollups.
  	text += "         ,COUNT(*) count_star\n";
  }
}

// ***********************************************************************
// Generate the text for the GROUP BY clause.
// ***********************************************************************
void ProposedMV::unparseGroupByClause(NAString& text)
{
  QRTRACER("ProposedMV::unparseGroupByClause()");
  if (hasGroupBy_ == FALSE)
    return;

  text += "  GROUP BY ";

  // First, go over the select list, and use ordinals for any column 
  // found in the group by list.
  CollIndex maxEntries = selectArray_.entries();
  for (CollIndex i=1; i<=maxEntries; i++)
  {
    QROutputPtr output = selectArray_[i];
    const NAString* elemText = getHashKeyForElement(output->getOutputItem());
    if (groupingColumns_.getFirstValue(elemText))
    {
      // Found it. Write the ordinal number.
      char buffer[10];
      sprintf(buffer, "%d", i);
      text += buffer;

      // Now remove it from the hash, so we can see what's left.
      groupingColumns_.remove(elemText);
      delete elemText;

      if (!groupingColumns_.isEmpty())
        text += ", ";
    }
  }

  // Now add the other elements using unparsed text.
  ElementHashIterator outsIterator(groupingColumns_);
  const NAString* key = NULL;
  QRElementPtr value = NULL;
  maxEntries = outsIterator.entries();
  for (CollIndex j = 0; j < maxEntries; j++) 
  {
    outsIterator.getNext(key, value); 
    // Get the column correlation name or the expression unparsed text.
    text += unparseColumnOrExpr(value);

    if (j<maxEntries-1)
      text += ", ";
  }

  text += "\n";
}

// ***********************************************************************
// Generate the text for the FROM clause.
// We must use the explicit form: FROM t1 JOIN t2 ON <pred>
// in order to support left outer joins.
// ***********************************************************************
void ProposedMV::unparseFromClause(NAString& text)
{
  QRTRACER("ProposedMV::unparseFromClause()");
  text += "  FROM ";

  // Use the join graph to find connected tables.
  QRJoinGraphPtr joinGraph = mapList_[0]->getSubGraph()->getParentGraph();
  // Start with an empty subgraph, and build the join graph from it.
  QRJoinSubGraphPtr subGraph = new(heap_) 
    QRJoinSubGraph(joinGraph,ADD_MEMCHECK_ARGS(heap_));
  if (hasGroupBy_)
    subGraph->setGroupBy();

  // Pick a table to start from.
  JoinGraphTablePtr firstTable = joinGraph->getFirstTable();
  text += getFromClauseForTable(firstTable);
  subGraph->addTable(firstTable);

  // while there are still tables to add
  while (!subGraph->isFull())
  {
    JoinGraphTablePtr nextTable = joinGraph->getNextTable(subGraph);
    if (nextTable == NULL)
      break;

    // Join to the next table
    const QRTablePtr tableElem = nextTable->getTableElement();
    if (tableElem->hasLOJParent() ||
        addedLojTables_.contains(&tableElem->getID()))
      text += "  LEFT OUTER JOIN ";
    else
      text += "  INNER JOIN ";
    text += getFromClauseForTable(nextTable);

    // Add the ON predicate
    text += getOnClauseFor(subGraph, nextTable);

    subGraph->addTable(nextTable);
  }

  deletePtr(subGraph);
}

// ***********************************************************************
// The FROM clause for a table includes its fully qualified name, 
// followed by its correlation name.
// ***********************************************************************
NAString ProposedMV::getFromClauseForTable(JoinGraphTablePtr table)
{
  QRTRACER("ProposedMV::getFromClauseForTable()");
  QRTablePtr tableElem = table->getTableElement();
  return tableElem->getTableName() + " " + tableElem->getCorrelationName() + "\n";
}

// ***********************************************************************
// Get the column name qualified by its table's correlation name.
// ***********************************************************************
NAString ProposedMV::getColumnCorrelationName(QRColumnPtr column)
{
  QRTRACER("ProposedMV::getColumnCorrelationName()");
  QRJoinGraphPtr joinGraph = mapList_[0]->getSubGraph()->getParentGraph();
  QRTablePtr table = joinGraph->getTableByID(column->getTableID())->getTableElement();
  
  if (table->getCorrelationName() == "")
    return column->getFullyQualifiedColumnName();
  else
    return table->getCorrelationName() + "." + column->getColumnName();
}

// ***********************************************************************
// If its an expression: return its unparsed text.
// If its a column: return its column correlation name.
// If its a join pred: return the correlation name of its first column.
// ***********************************************************************
NAString ProposedMV::unparseColumnOrExpr(const QRElementPtr elem)
{
  QRTRACER("ProposedMV::unparseColumnOrExpr()");
  if (elem->getElementType() == ET_Expr)
    return elem->downCastToQRExpr()->getInfo()->getText();
  else
  {
    // If its a join pred, get the first column.
    QRColumnPtr firstCol = elem->getFirstColumn();
    return getColumnCorrelationName(firstCol);
  }
}

// ***********************************************************************
// Generate the text for the ON clause.
// ***********************************************************************
NAString ProposedMV::getOnClauseFor(QRJoinSubGraphPtr subGraph, JoinGraphTablePtr nextTable)
{
  QRTRACER("ProposedMV::getOnClauseFor()");
  NABoolean onClauseStarted = FALSE;
  NAString onClause;

  // Get the join predicate used from the join graph.
  const EqualitySetList& eqSets = nextTable->getEqualitySets();
  for (CollIndex i=0; i<eqSets.entries(); i++)
  {
    // We only need join predicates that connect the next table to the subgraph.
    JoinGraphEqualitySetPtr eqSet = eqSets[i];
    if (!eqSet->isConnectedTo(subGraph))
      continue;

    // This is the side of the join predicate involving the next table.
    const JoinGraphHalfPredicatePtr targetHalfPred = eqSet->findHalfPredTo(nextTable);
    // Now find the other one.
    HalfPredicateList& halfPreds = eqSet->getHalfPredicates();
    JoinGraphTablePtr sourceTable = NULL;
    for (CollIndex j=0; j<=halfPreds.entries(); j++)
    {
      const JoinGraphHalfPredicatePtr halfPred = halfPreds[j];
      // Skip the half pred pointing to nextTable.
      if (halfPred == targetHalfPred)
        continue;

      // Skip half preds pointing to tables still not in the subGraph.
      if (!subGraph->contains(halfPred->getTable()))
        continue;

      // Found a source table.
      // Finally we can start generating some SQL.
      if (!onClauseStarted)
      {
        onClause += "    ON ";
        onClauseStarted = TRUE;
      }
      else
      {
        onClause += " AND ";
      }

      // Here comes the actual predicate text
      onClause += unparseColumnOrExpr(halfPred->getElementPtr()->getReferencedElement());
      onClause += " = ";
      onClause += unparseColumnOrExpr(targetHalfPred->getElementPtr()->getReferencedElement());

      // No need to find additional connected tables in the equality set.
      break;
    }

  }

  onClause += "\n";
  return onClause;
}

// ***********************************************************************
// Generate the text for the WHERE and HAVING clauses, from the range and 
// residual predicates. 
// ***********************************************************************
void ProposedMV::unparseWhereAndHavingClauses(NAString& text, NAString& havingClause)
{
  QRTRACER("ProposedMV::unparseWhereAndHavingClauses()");

  NAString whereClause = "";
  unparseRangePreds(whereClause, havingClause);
  unparseResidualPreds(whereClause, havingClause);
  
  // If there are no predicates, don't add the WHERE keyword.
  if (whereClause != "")
  {
    text += "  WHERE ";
    text += whereClause;    
  }
}

// ***********************************************************************
// A range pred looks like this:
// (<subrange-text> OR <subrange-text> ...)
// Where the subrange-text looks like one of these (handled by QRRangeOperator::unparse() ):
//   <range-item> <operator> <literal>
//   <range-item> IN ( <literal-list> )
//   <range-item> BETWEEN <literal> AND <literal>
//   <range-item> {> | >=} <literal> AND <range-item> {< | <=} <literal>
//   <range-item> IS NULL
// ***********************************************************************
void ProposedMV::unparseRangePreds(NAString& whereClause, NAString& havingClause)
{
  QRTRACER("ProposedMV::unparseRangePreds()");
  if (canSkipRangePreds_)
    return;

  CollIndex maxEntries = rangePredList_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    QRRangePredPtr pred = rangePredList_[i];

    // Unparse the range item.
    NAString rangeItemText = unparseColumnOrExpr(pred->getRangeItem()->getReferencedElement());
    // Get the range operator list (subranges)
    const NAPtrList<QRRangeOperatorPtr>& opList = pred->getOperatorList();
    CollIndex opEntries = opList.entries();
    NAString predText;
    if (opEntries > 0)
    {
      predText += "( ";
	    for (CollIndex j=0; j<opEntries; j++)
	    {
	      opList[j]->unparse(predText, rangeItemText);
	      if (j < opEntries-1)
	        predText += " OR ";
	    }
	    predText += ")";
    }
    else
    {
    	// Empty operator list - this is only IS NOT NULL.
      predText += rangeItemText;
    	predText += " IS NOT NULL ";
    }
    
    // Does the range expression contain an aggregate function?
    QRElementPtr rangeItem = pred->getRangeItem();
    if ((rangeItem->getElementType() == ET_Expr) &&
        rangeItem->downCastToQRExpr()->getExprRoot()->containsAnAggregate(heap_))
    {
      // Add it to the HAVING clause
      addPredicateText(havingClause, predText);
    }
    else
    {
      // Add it to the WHERE clause.
      addPredicateText(whereClause, predText);
    }
  }
}

// ***********************************************************************
// Residual predicates are just expressions - use their unparsed text.
// ***********************************************************************
void ProposedMV::unparseResidualPreds(NAString& whereClause, NAString& havingClause)
{
  QRTRACER("ProposedMV::unparseResidualPreds()");
  if (canSkipResidualPreds_)
    return;

  CollIndex maxEntries = residualPredList_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    // Does the predicate contain an aggregate function?
    QRExprPtr pred = residualPredList_[i];
    if (pred->getExprRoot()->containsAnAggregate(heap_))
    {
      // Add it to the HAVING clause
      addPredicateText(havingClause, pred->getInfo()->getText());
    }
    else
    {
      // Add it to the WHERE clause.
      addPredicateText(whereClause, pred->getInfo()->getText());
    }
  }
}

// ***********************************************************************
// ***********************************************************************
void ProposedMV::addPredicateText(NAString& text, const NAString& pred)
{
  if (text != "")
    text += "    AND ";
  text += pred;
  text += "\n";
}

// ***********************************************************************
// ***********************************************************************
Int32 ProposedMV::getJoinSize()
{
  QRJoinGraphPtr joinGraph = mapList_[0]->getSubGraph()->getParentGraph();
  return joinGraph->entries();
}

//========================================================================
//  Class WorkloadAnalysis
//========================================================================

// ***********************************************************************
// ***********************************************************************
WorkloadAnalysis::~WorkloadAnalysis()
{
  QRTRACER("~WorkloadAnalysis::()");
  CollIndex maxEntries = proposedMVsList_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    deletePtr(proposedMVsList_[i]);
  }
  proposedMVsList_.clear();
}

// ***********************************************************************
// ***********************************************************************
NABoolean WorkloadAnalysis::addProposedMV(ProposedMVPtr pmv)
{
  QRTRACER("WorkloadAnalysis::addProposedMV()");

  // Filter duplicate queries (because of self-joins)
  SubGraphMapList mapList = pmv->getMapList();
  NAString addedList;
  NAString skippedList;
  // The loop goes backwards because duplicate MVs are removed from the list
  // as we go.
  for (Int32 i=mapList.entries()-1; i>=0; i--)
  {
    QRJoinSubGraphMapPtr map = mapList[i];
    const NAString* queryName = &(map->getMVDetails()->getMVName());
    // Did we already insert this query?
    if (inventoryHash_.contains(queryName))
    {
      // Yes - ignore it.
      mapList.removeAt(i);
      skippedList += *queryName;
      skippedList += " ";
    }
    else
    {
      // No - add it to the inventory.
      inventoryHash_.insert(queryName, queryName);
      addedList += *queryName;
      addedList += " ";
    }
  }

  // Did we ignore all the queries?
  if (mapList.isEmpty())
  {
    // Yes - ignore the ProposedMV object.
    QRLogger::log(CAT_QMS_MAIN, LL_DEBUG,
                  "Ignoring duplicate queries: %s.", skippedList.data());
    return FALSE;
  }
  else
  {
    // Set the name of this proposed MV from its ordinal number.
    pmv->setName(nextMV_++);
    proposedMVsList_.insert(pmv);
    QRLogger::log(CAT_QMS_MAIN, LL_DEBUG,
                  "Inserting ProposedMV: %s using: %s and ignoring: ", 
                  pmv->getName().data(),
                  addedList.data(), 
                  skippedList.data());
    return TRUE;
  }
}

// ***********************************************************************
// ***********************************************************************
void WorkloadAnalysis::reportResults(ofstream& ofs, Int32 minQueriesPerMV)
{
  QRTRACER("WorkloadAnalysis::reportResults()");
  QRLogger::instance().log(CAT_QMS_MAIN, LL_INFO,
    "Initiating workload analysis.");

  // Perform common predicate analysis.
  findReducedPredicateSet();

  // Dump it also to the log file.
  QRLogger::instance().log(CAT_QMS_MAIN, LL_INFO,
    "Workload analysis result:\n");

  ofs << "CREATE SCHEMA  TRAFODION.MVQR_WA;\n";
  ofs << "SET CATALOG TRAFODION;\n";
  ofs << "SET SCHEMA  TRAFODION.MVQR_WA;\n";
  ofs << "CONTROL QUERY DEFAULT MVQR_REWRITE_LEVEL '4';\n\n";

  Int32 maxJoinSize = getMaxJoinSize();
  if (maxJoinSize > 10)
  {
    char buffer[64];
    sprintf(buffer, "CONTROL QUERY DEFAULT MVQR_MAX_MV_JOIN_SIZE '%d';\n\n", maxJoinSize);
    ofs << buffer;
  }

  // Generate the SQL to the output file.
  CollIndex maxEntries = proposedMVsList_.entries();
  NAString pmvText;
  for (CollIndex i=0; i<maxEntries; i++)
  {
    QRLogger::instance().log(CAT_QMS_MAIN, LL_DEBUG, "Producing SQL for MV %d of %d.", i, maxEntries);
    pmvText = "";
    proposedMVsList_[i]->reportResults(pmvText);
    // Log the result one MV at a time.
    QRLogger::instance().log1(CAT_QMS_MAIN, LL_INFO, pmvText.data());
    ofs << pmvText;
  }
}

// ***********************************************************************
// Perform common predicate analysis.
// ***********************************************************************
void WorkloadAnalysis::findReducedPredicateSet()
{
  QRTRACER("WorkloadAnalysis::findReducedPredicateSet()");
  // This loop goes backwards so we can remove any problematic 
  // proposed MVs on the way.
  for (Int32 i=proposedMVsList_.entries()-1; i>=0; i--)
  {
    ProposedMVPtr pmv = proposedMVsList_[i];
    try
    {
      pmv->findReducedPredicateSet();
    }
    catch(...)
    {
      QRLogger::instance().log(CAT_QMS_MAIN, LL_ERROR, "Exception thrown during workload analysis. MV %s skipped.", pmv->getName().data());
      CollIndex maxEntries = pmv->getMapList().entries();
      NAString skippedQueries("Skipped queries: ");
      for (CollIndex j=0; j<maxEntries; j++)
      {
      	skippedQueries += pmv->getMapList()[j]->getMVDetails()->getMVName();
      	skippedQueries += ", ";
      }
      QRLogger::instance().log(CAT_QMS_MAIN, LL_ERROR, skippedQueries.data());
      	
      proposedMVsList_.removeAt(i);
      deletePtr(pmv);
    }
  }
}

// ***********************************************************************
// ***********************************************************************
Int32 WorkloadAnalysis::getMaxJoinSize()
{
  QRTRACER("WorkloadAnalysis::getMaxJoinSize()");
  Int32 maxJoinSize=0;

  Int32 maxEntries = proposedMVsList_.entries();
  for (Int32 i=0; i<maxEntries; i++)
  {
    ProposedMVPtr pmv = proposedMVsList_[i];
    Int32 joinSize = pmv->getJoinSize();
    if (joinSize > maxJoinSize)
      maxJoinSize = joinSize;
  }

  return maxJoinSize;
}
