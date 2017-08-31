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
* File:         MavRelRootBuilder.cpp
* Description:  Class MavRelRootBuilder, used by MavBuilder
*               for MV INTERNAL REFRESH command.
*
* Created:      12/27/2000
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/

#include "Sqlcomp.h"  // for SQLDoublePrecision
#include "AllItemExpr.h"
#include "AllRelExpr.h"
#include "MVInfo.h"
#include "MvRefreshBuilder.h"
#include "MavRelRootBuilder.h"

// ===========================================================================
// ===========================================================================
// ===================  class  MavRelRootBuilder    =========================
// ===========================================================================
// ===========================================================================

//----------------------------------------------------------------------------
// 
MavRelRootBuilder::MavRelRootBuilder(const MVInfoForDML *mvInfo, 
                                     CollHeap           *heap)
  : heap_           (heap),
    posOfCountStar_ (mvInfo->getPosOfCountStar()),
    mavCols_        (mvInfo->getMVColumns()),
    deltaCorrName_  (MavBuilder::getSysDeltaName(), heap_),
    mavCorrName_    (MavBuilder::getSysMavName(),   heap_),
    calcCorrName_   (MavBuilder::getSysCalcName(),  heap_),
    countCols_      (heap_),
    sumCols_        (heap_),
    otherCols_      (heap_),
    groupByCols_    (heap_),
    extraMinMaxColRefs_(heap_),
    minMaxRecomputeOnUpdateCondition_(NULL),
    minMaxRecomputeOnInsertCondition_(NULL),
    isMavWithoutGroupBy_(FALSE)
{
}

//----------------------------------------------------------------------------
// No need to directly delete any of the pointers, because they all point 
// to data belonging to MVInfo. The only thing we need to do is clear 
// extraMinMaxColRefs_, because the ItemExprList Dtor deletes contained
// pointers.
MavRelRootBuilder::~MavRelRootBuilder() 
{
  extraMinMaxColRefs_.clear();
}

//----------------------------------------------------------------------------
// Prepare for creating the SYS_CALC columns, by divide the list of MAV 
// columns to 4 groups:
// 1. GroupBy columns - inserted directly into the RETDesc without change.
// 2. COUNT columns               - collected in countCols_, done in phase1.
// 3. SUM, MIN and MAX columns    - collected in sumCols_, done in phase2.
// 4. AVG, STDDEV, VARIANCE cols  - collected in otherCols_, done in phase3.
void MavRelRootBuilder::init()
{
  // Divide the MAV columns to groups:
  for (CollIndex i=0; i<mavCols_.entries(); i++)
  {
#pragma nowarn(1506)   // warning elimination 
    MVColumnInfo *currentCol = mavCols_[i];
#pragma warn(1506)  // warning elimination 

    if (currentCol->getColType() != COM_MVCOL_AGGREGATE)
    {  
      // This is a group-by column, or a constant that does not change.
      groupByCols_.insert(currentCol);
    }
    else
    {
      // This is an aggregate column. insert into the correct list.
      switch (currentCol->getOperatorType())
      {
	case ITM_COUNT: 
	case ITM_COUNT_NONULL: 
	  // No dependent columns, done in phase1.
	  countCols_.insert(currentCol);
	  break;

	case ITM_SUM:
	case ITM_MIN:
	case ITM_MAX:
	  // Use the SYS_CALC COUNT columns, done in phase2.
	  sumCols_.insert(currentCol);
	  break;

	case ITM_AVG:
	case ITM_STDDEV:
	case ITM_VARIANCE:
	  // Use the SYS_CALC COUNT and SUM columns, done in phase3.
	  otherCols_.insert(currentCol);
	  break;

	default:
	  CMPASSERT(FALSE);
      }
    }
  }
}  // MavRelRootBuilder::init()

//////////////////////////////////////////////////////////////////////////////
// Build a stack of 5 RelRoot nodes to calculate the SYS_CALC columns:
//     RelRoot (*, <GOP column>)
//        |
//     RelRoot (*, <AVG, STDDEV & VARIANCE cols>)
//        |
//     RelRoot (*, <sum, min & max columns>)
//        |
//     RelRoot (*, <count columns>)
//        |
//     topNode
//
//////////////////////////////////////////////////////////////////////////////
RelExpr *MavRelRootBuilder::buildCalcCalculationRootNodes(RelExpr *dcbTree)
{
  RelRoot *topNode;
  topNode = buildRootForCount (dcbTree);
  topNode = buildRootForSum   (topNode);
  topNode = buildRootForOthers(topNode);
  topNode = buildRootForGop   (topNode);

  topNode->addSelPredTree(buildSelfCancelingDeltaPredicate());

  return topNode;
}  // MavRelRootBuilder::buildCalcCalculationRootNodes(

//////////////////////////////////////////////////////////////////////////////
// First work on the RelRoot of the MAV select tree to fix the aggregate
// expressions for COUNT, SUM, MIN and MAX, and remove expressions for
// AVG, STDDEV and VARIANCE.
// Then, add another root node to re-introduce expressions for AVG, STDDEV
// and VARIANCE. this time using the fixed expressions of the first root node.
// The result tree is:
//     RelRoot (*, <AVG, STDDEV and VARIANCE columns>)
//        |
//   RenameTable  (to SYS_DELTA)
//        |
//   mvSelectTree (with a fixed select list
//////////////////////////////////////////////////////////////////////////////
RelExpr *MavRelRootBuilder::buildDeltaCalculationRootNodes(RelExpr  *mvSelectTree,
							   NABoolean canSkipMinMax,
							   NABoolean wasFullDE)
{
  checkForMavWithoutGroupBy(mvSelectTree);
  fixDeltaColumns(mvSelectTree, canSkipMinMax, wasFullDE);

  RelExpr *topNode = new(heap_) 
    RenameTable(mvSelectTree, MavBuilder::getSysDeltaName());

  if (isMavWithoutGroupBy())
    topNode = buildRootForNoGroupBy(topNode);

  topNode = buildRootForDelta(topNode);

  return topNode;
}  // MavRelRootBuilder::buildDeltaCalculationRootNodes()

//----------------------------------------------------------------------------
// Check if the query for this MAV is without a GROUP BY clause.
// The RelExpr tree for a query with no GROUP BY clause, does not have
// a GroupBy node in it. 
void MavRelRootBuilder::checkForMavWithoutGroupBy(RelExpr *mvSelectTree) 
{
  // Start from the top of the RelExpe tree.
  RelExpr *node = mvSelectTree;
  // Look for a GroupBy node.
  while (node->child(0) != NULL &&
         node->getOperatorType() != REL_GROUPBY)
    node = node->child(0);

  if (node->getOperatorType() != REL_GROUPBY)
    isMavWithoutGroupBy_ = TRUE;
  else
  {
    // Check if the child of our child is another GroupBy node.
    // This can only be the result of multi-delta optimization on a 
    // no-groupby MAV.
    if ( (node->child(0)->getOperatorType() == REL_ROOT)            &&
         (node->child(0)->child(0)->getOperatorType() == REL_GROUPBY) )
    {
      // Found another GroupBy node below.
      // Now verify that the top one has no grouping columns.
      GroupByAgg *groupByNode = (GroupByAgg *)node;
      if (groupByNode->getGroupExprTree() == NULL)
	isMavWithoutGroupBy_ = TRUE;
    }
  }
}  // MavRelRootBuilder::checkForMavWithoutGroupBy()

//----------------------------------------------------------------------------
// Transform each aggregate in the MAV select list, to use the @OP column in
// order to calculate deleted rows correctly:
// COUNT(*) => SUM(@OP)
// COUNT(a) => SUM(IF (a IS NULL) THEN 0 ELSE @OP) 
// SUM(a)   => SUM(a * @OP)
// MIN(a) & MAX(a) => Special treatment (see handleDeltaMinMaxColumns()).
// AVG, STDDEV and VARIANCE are derived from other COUNT and SUM columns. 
// They are removed from this select list to avoid divide by zero problems
// in case COUNT=0.
void MavRelRootBuilder::fixDeltaColumns(RelExpr   *mvSelectTree, 
					NABoolean  canSkipMinMax,
				        NABoolean  wasFullDE)
{
  // Get the MAV select list from the mvSelectTree.
  CMPASSERT(mvSelectTree->getOperatorType() == REL_ROOT);
  RelRoot *mavSelectRoot = (RelRoot *)mvSelectTree;

  ItemExprList mavSelectList(mavSelectRoot->removeCompExprTree(), heap_);
  ItemExprList newSelectList(heap_);

  const NAString& opName = MavBuilder::getVirtualOpColumnName();

  for (CollIndex i=0; i<mavSelectList.entries(); i++)
  {
    ItemExpr *selectListExpr = mavSelectList[i];

    CMPASSERT(selectListExpr->getOperatorType() == ITM_RENAME_COL);
    const NAString& colName = 
      ((RenameCol*)selectListExpr)->getNewColRefName()->getColName();
    ItemExpr *aggregateExpr = selectListExpr;
    while ((aggregateExpr->getOperatorType() == ITM_RENAME_COL) ||
           (aggregateExpr->getOperatorType() == ITM_CAST))
      aggregateExpr = aggregateExpr->child(0);

    // Assuming all aggregates are top-most functions as explained in the 
    // MV external spec.
    ItemExpr *aggregateOperand = aggregateExpr->child(0);
    ItemExpr *newExpr   = NULL;

    if (!aggregateExpr->containsAnAggregate())
    {
      // Non-aggregate columns (group-by cols) are added unchanged.
    }
    else
      switch (aggregateExpr->getOperatorType())
      {
	case ITM_COUNT_NONULL:
	  // COUNT(a) => SUM(IF (a IS NULL) THEN 0 ELSE @OP) 
	  newExpr = new(heap_) 
	    Aggregate(ITM_SUM, new(heap_) 
	      Case(NULL, new(heap_)
		IfThenElse (new(heap_) UnLogic(ITM_IS_NULL, aggregateOperand),
    			    new(heap_) SystemLiteral(0),
			    new(heap_) ColReference(new(heap_) 
			      ColRefName(opName)))));
	  selectListExpr->child(0) = newExpr;
	  break;

	case ITM_COUNT:
	  // COUNT(*) => SUM(@OP)
	  newExpr = new(heap_) 
	    Aggregate(ITM_SUM, 
    		      new(heap_) ColReference(new(heap_) ColRefName(opName)));
	  selectListExpr->child(0) = newExpr;
	  break;

	case ITM_SUM:
	  // SUM(a) => SUM(a * @OP)
	  newExpr = new(heap_)
	    BiArith(ITM_TIMES,
		    aggregateOperand,
		    new(heap_) ColReference(new(heap_) ColRefName(opName)));
	  aggregateExpr->child(0) = newExpr;
	  break;

	case ITM_MIN:
	case ITM_MAX:
	  // Handle Min/Max if not all deltas are INSERT ONLY.
	  if (!canSkipMinMax)
	    handleDeltaMinMaxColumns(aggregateExpr, colName, newSelectList, wasFullDE);
	  // Add the SYS_DELTA Min/Max aggregate as is.
	  break;

	case ITM_AVG:
	case ITM_DIVIDE:
	case ITM_VARIANCE:
	case ITM_STDDEV:
	  // Remove these expressions from the select list.
	  selectListExpr = NULL;
	  break;

	default:
	  // A new type of aggregate?
	  CMPASSERT(FALSE);
	  break;
      }

    if (selectListExpr!=NULL)
      newSelectList.insert(selectListExpr);
  }

  ItemExpr *selList = newSelectList.convertToItemExpr();
  mavSelectRoot->addCompExprTree(selList);

  mavSelectList.clear(); // Don't delete all the entries from the Dtor.
}  // MavRelRootBuilder::fixDeltaColumns()

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MavRelRootBuilder::handleDeltaMinMaxColumns(ItemExpr        *aggregateExpr,
						 const NAString&  colName,
						 ItemExprList&    newSelectList,
						 NABoolean        wasFullDE)
{
  // 1. Add the column expressions to the MAV select list
  ItemExpr *extraInsCol = 
    buildExtraMinMaxExpr(aggregateExpr, colName, AGG_FOR_INSERT);
  newSelectList.insert(extraInsCol);

  ItemExpr *extraDelCol = 
    buildExtraMinMaxExpr(aggregateExpr, colName, AGG_FOR_DELETE);
  newSelectList.insert(extraDelCol);

  // 2. Create column references to the new columns.
  ColReference *colInsRef = new(heap_) 
    ColReference(createExtraAggName(colName, AGG_FOR_INSERT));
  extraMinMaxColRefs_.insert(colInsRef->copyTree(heap_));
  ColReference *colDelRef = new(heap_) 
    ColReference(createExtraAggName(colName, AGG_FOR_DELETE));
  extraMinMaxColRefs_.insert(colDelRef->copyTree(heap_));

  // 3. Build the MinMax recompute conditional expression.
  // The last parameter should be TRUE only if a full DE was performed
  // by the refresh utility.
  addToMinMaxRecomputeCondition(colName, colInsRef, colDelRef, 
				aggregateExpr->getOperatorType(), 
				wasFullDE);
}  // MavRelRootBuilder::handleDeltaMinMaxColumns()

//----------------------------------------------------------------------------
// The select list here is: (<fixed-count-cols>, <sum-col-refs>)
// The count cols become:
// Case( If <count-col> IS NULL THEN 0 ELSE <count-col>)
// Since the count cols were transformed from COUNT to SUM(@OP), They will 
// now evaluate to NULL instead of 0 when no data is returned, for MAVs 
// without a GROUP BY clause. This is what is fixed here.
//----------------------------------------------------------------------------
RelRoot *MavRelRootBuilder::buildRootForNoGroupBy(RelExpr *topNode)
{
  ItemExprList selectList(heap_);

  for (CollIndex i=0; i<countCols_.entries(); i++)
  {
    const MVColumnInfo *currentMavColumn = countCols_[i];
    const NAString& colName = currentMavColumn->getColName();

    // Build a col reference to the SYS_DELTA column.
    ItemExpr *sysDeltaColExpr = new(heap_)
      ColReference(new(heap_) ColRefName(colName, deltaCorrName_));

    ItemExpr *condExpr = new(heap_)
      Case(NULL, new(heap_)
	IfThenElse(new(heap_) UnLogic(ITM_IS_NULL, sysDeltaColExpr),
		   new(heap_) SystemLiteral(0),
		   sysDeltaColExpr->copyTree(heap_)));

    ItemExpr *renamedColExpr = new(heap_) 
      RenameCol(condExpr, new(heap_) ColRefName(colName, deltaCorrName_));

    selectList.insert(renamedColExpr);
  }

  // Add the SUM cols.
  addColsToSelectList(sumCols_, selectList, deltaCorrName_);

  // Add the extra Min/Max columns.
  selectList.insert(extraMinMaxColRefs_);

  // The select list is ready. Create the root over topNode.
  RelRoot *newRoot = new(heap_) 
    RelRoot(topNode, REL_ROOT, selectList.convertToItemExpr());
  newRoot->setDontOpenNewScope();

  selectList.clear();
  return newRoot;
}  // MavRelRootBuilder::buildRootForNoGroupBy()

//----------------------------------------------------------------------------
// The select list is: (<fixed-count-cols>, <col-ref-to-sum-cols>, 
//----------------------------------------------------------------------------
RelRoot *MavRelRootBuilder::buildRootForDelta(RelExpr *topNode)
{
  // Start the select list with SYS_MAV.*
  ColRefName *star = new(heap_) ColRefName(TRUE, heap_);
  ItemExpr *starReference = new(heap_) ColReference(star);

  const CorrName emptyCorrName("", heap_);

  ItemExprList newColumns(heap_);
  newColumns.insert(starReference);

  for (CollIndex i=0; i<otherCols_.entries(); i++)
  {
    const MVColumnInfo *currentMavColumn = otherCols_[i];
    const NAString& colName = currentMavColumn->getColName();

    const NAType *desiredType = new(heap_) SQLDoublePrecision(heap_, TRUE);
    ItemExpr *countDepCol = buildDepColExpr(emptyCorrName, currentMavColumn->getDepCol1());
    ItemExpr *sumDepCol   = buildDepColExpr(emptyCorrName, currentMavColumn->getDepCol2());
    ItemExpr *sum2DepCol  = NULL;
    
    ItemExpr *newColExpr = NULL;
    switch(currentMavColumn->getOperatorType())
    {
      case ITM_AVG:
	newColExpr = new(heap_)
	  BiArith(ITM_DIVIDE, sumDepCol, countDepCol);
	break;

      case ITM_STDDEV:
      case ITM_VARIANCE:
	sum2DepCol = buildDepColExpr(emptyCorrName, currentMavColumn->getDepCol3());

	newColExpr = new(heap_)
	  ScalarVariance(currentMavColumn->getOperatorType(),
	                 new(heap_) Cast(sum2DepCol,  desiredType), 
			 new(heap_) Cast(sumDepCol,   desiredType), 
			 new(heap_) Cast(countDepCol, desiredType));
	break;

      default: CMPASSERT(FALSE);
    }

    ItemExpr *condExpr = new(heap_)
      Case(NULL, new(heap_)
	IfThenElse(new(heap_) BiRelat(ITM_EQUAL, 
				      countDepCol,
				      new(heap_) SystemLiteral(0)),
		   new(heap_) SystemLiteral(),
		   newColExpr));

    ColRefName *newColName = new(heap_) ColRefName(colName, deltaCorrName_);
    ItemExpr *renamedColExpr = new(heap_) RenameCol(condExpr, newColName);

    newColumns.insert(renamedColExpr);
  }

  // The select list is ready. Create the root over topNode.
  RelRoot *newRoot = new(heap_) 
    RelRoot(topNode, REL_ROOT, newColumns.convertToItemExpr());

  newColumns.clear();

  return newRoot;
}  // MavRelRootBuilder::buildRootForDelta()

//----------------------------------------------------------------------------
// First, do the COUNT columns.
// The select list of this root is (*, <count cols>).
// For COUNT, the SYS_CALC.col expr is:
//   SYS_MAV.col + SYS_DELTA.col
// For MAVs without a GROUP BY clause, this is later fixed by 
// buildRootForNoGroupBy().
//----------------------------------------------------------------------------
RelRoot *MavRelRootBuilder::buildRootForCount(RelExpr *topNode)
{
  // Start the select list with *
  ItemExprList selectList(heap_);
  addStarToSelectList(selectList);

  addColsToSelectList(groupByCols_, selectList, deltaCorrName_, &calcCorrName_);

  // For each column in the COUNT list
  for (CollIndex i=0; i<countCols_.entries(); i++)
  {
    MVColumnInfo *currentCol = countCols_[i];
    const NAString& colName = currentCol->getColName();

    // Find the column in the SYS_DELTA and SYS_MAV lists.
    ItemExpr *sysDeltaColExpr = new(heap_)
      ColReference(new(heap_) ColRefName(colName, deltaCorrName_));

    ItemExpr *sysMavColExpr = new(heap_)
      ColReference(new(heap_) ColRefName(colName, mavCorrName_));
      
    ItemExpr *newCalcExpr = new(heap_) 
      BiArith(ITM_PLUS, sysDeltaColExpr, sysMavColExpr);

    // Add the col expression as SYS_CALC.col
    ColRefName *calcColName = new(heap_) ColRefName(colName, calcCorrName_);
    ItemExpr *newCalcCol = new(heap_) RenameCol(newCalcExpr, calcColName);

    selectList.insert(newCalcCol);
  }

  // The select list is ready. Create the root over topNode.
  RelRoot *newRoot = new(heap_) 
    RelRoot(topNode, REL_ROOT, selectList.convertToItemExpr());
  newRoot->setDontOpenNewScope();

  selectList.clear();
  return newRoot;
}  // MavRelRootBuilder::buildRootForCount()

//----------------------------------------------------------------------------
// Next, do the SUM, MIN and MAX columns, using the already calculated 
// and bound COUNT columns (SYS_CALC.dep1).
// For col=SUM(a), the new expr is:
//   CASE
// 1)  	WHEN (SYS_CALC.dep1 = 0)     THEN NULL
// 2)  	WHEN (SYS_MAV.col IS NULL)   THEN SYS_DELTA.col
// 3)  	WHEN (SYS_DELTA.col IS NULL) THEN SYS_MAV.col
//   	ELSE SYS_MAV.col + SYS_DELTA.col
//   END
// For col=MIN(a), the else clause is replaced by (no MIN ItemExpr):
//      WHEN (SYS_MAV.col < SYS_DELTA.col) THEN SYS_MAV.col
//      ELSE SYS_DELTA.col
// For col=MAX(a), the else clause is:
//      WHEN (SYS_MAV.col > SYS_DELTA.col) THEN SYS_MAV.col
//      ELSE SYS_DELTA.col
//----------------------------------------------------------------------------
RelRoot *MavRelRootBuilder::buildRootForSum(RelExpr *topNode)
{
  // Start the select list with *
  ItemExprList selectList(heap_);
  addStarToSelectList(selectList);

  // For each column in the COUNT list
  for (CollIndex i=0; i<sumCols_.entries(); i++)
  {
    MVColumnInfo *currentCol = sumCols_[i];
    const NAString& colName = currentCol->getColName();

    // Find the column in the SYS_DELTA and SYS_MAV lists.
    ItemExpr *sysDeltaColExpr = new(heap_)
      ColReference(new(heap_) ColRefName(colName, deltaCorrName_));

    ItemExpr *sysMavColExpr = new(heap_)
      ColReference(new(heap_) ColRefName(colName, mavCorrName_));
      
    // Find the dependent SYS_CALC COUNT column.
    ItemExpr *calcDep1 = 
      buildDepColExpr(calcCorrName_, currentCol->getDepCol1());

    // Build the condition bottom-up - do the ELSE clause first.
    ItemExpr *elseClause = NULL;
    if (currentCol->getOperatorType() == ITM_SUM)
    {
      // The ELSE clause for SUM is (SYS_MAV.col + SYS_DELTA.col).
      elseClause = new(heap_) 
        BiArith(ITM_PLUS, sysDeltaColExpr, sysMavColExpr);
    }
    else
    {
      // The ELSE clause for MIN is:
      //    WHEN (SYS_MAV.col < SYS_DELTA.col) THEN SYS_MAV.col
      //    ELSE SYS_DELTA.col
      // For MAX the < operator is replaced by >.
      OperatorTypeEnum sign = 
        (currentCol->getOperatorType()==ITM_MIN ? ITM_LESS : ITM_GREATER);
      elseClause = new(heap_) 
        IfThenElse(new(heap_) BiRelat(sign, sysMavColExpr, sysDeltaColExpr),
  		   sysMavColExpr,
		   sysDeltaColExpr);
    }

    // The 3rd WHEN clause: WHEN (SYS_DELTA.col IS NULL) THEN SYS_MAV.col
    ItemExpr *when3Clause = new(heap_)
      IfThenElse(new(heap_) UnLogic(ITM_IS_NULL, sysDeltaColExpr),
		 sysMavColExpr,
		 elseClause);

    // The 2nd WHEN clause: WHEN (SYS_MAV.col IS NULL) THEN SYS_DELTA.col
    ItemExpr *when2Clause = new(heap_)
      IfThenElse(new(heap_) UnLogic(ITM_IS_NULL, sysMavColExpr),
		 sysDeltaColExpr,
		 when3Clause);

    // The 1st WHEN clause: WHEN (SYS_CALC.dep1 = 0) THEN NULL
    ItemExpr *when1Clause = new(heap_)
      IfThenElse(new(heap_) BiRelat(ITM_EQUAL, 
    				    calcDep1,
				    new(heap_) SystemLiteral(0)),
		 new(heap_) SystemLiteral(), // NULL constant
		 when2Clause);

    // Build the CASE on top.
    ItemExpr *newCalcExpr = new(heap_) Case(NULL, when1Clause);

    // Add the col expression as SYS_CALC.col
    ColRefName *calcColName = new(heap_) ColRefName(colName, calcCorrName_);
    ItemExpr *newCalcCol = new(heap_) RenameCol(newCalcExpr, calcColName);

    selectList.insert(newCalcCol);
  }

  // The select list is ready. Create the root over topNode.
  RelRoot *newRoot = new(heap_) 
    RelRoot(topNode, REL_ROOT, selectList.convertToItemExpr());
  newRoot->setDontOpenNewScope();

  selectList.clear();
  return newRoot;
}  // MavRelRootBuilder::buildRootForSum()

//----------------------------------------------------------------------------
// Next, do the other columns (AVG, STDDEV, VARIANCE), using the already
// calculated and bound COUNT and SUM columns.
// There are 3 dependent columns here (found using MVInfo):
//   dep1 is COUNT(a),
//   dep2 is SUM(a),
//   dep3 is SUM(a*a) (only for STDDEV and VARIANCE)
//   depCount is COUNT(a)
// In the case of STDDEV with a weight parameter - STDDEV(a,b), then
//   dep1 is SUM(b),
//   dep2 is SUM(a*b),
//   dep3 is SUM(a*a*b)
//   depCount is COUNT(a*b) and is the dependent column of dep2.
//
// For col=AVG(a), the new expr is:
//   CASE
//      WHEN (SYS_CALC.depCount = 0)  THEN NULL
//      ELSE SYS_CALC.dep2 / SYS_CALC.dep1
//   END
//
// For STDDEV and VARIANCE its:
//   CASE
//      WHEN (SYS_CALC.depCount = 0)  THEN NULL
//      ELSE ScalarVariance(SYS_CALC.dep3, SYS_CALC.dep2, SYS_CALC.dep1)
//   END
// depCount is the COUNT dependent column of dep2. This works also in the
// case of STDDEV with a weight parameter
//----------------------------------------------------------------------------
RelRoot *MavRelRootBuilder::buildRootForOthers(RelExpr *topNode)
{
  // Start the select list with *
  ItemExprList selectList(heap_);
  addStarToSelectList(selectList);

  // For each column in the COUNT list
  for (CollIndex i=0; i<otherCols_.entries(); i++)
  {
    MVColumnInfo *currentCol = otherCols_[i];
    const NAString& colName = currentCol->getColName();

    const NAType *desiredType = new(heap_) SQLDoublePrecision(heap_, TRUE);

    // Find the dependent SYS_CALC COUNT and SUM columns.
    ItemExpr *calcDep1 = 
      buildDepColExpr(calcCorrName_, currentCol->getDepCol1());
    ItemExpr *calcDep2 = 
      buildDepColExpr(calcCorrName_, currentCol->getDepCol2());
    ItemExpr *calcDep3 = NULL;
    ItemExpr *calcDepCount = NULL;
    if (currentCol->getOperatorType() == ITM_AVG)
    {
      calcDepCount = calcDep1;
    }
    else
    {
      // dep3 and depCount are needed only for STDDEV and VARIANCE.
      calcDep3 = buildDepColExpr(calcCorrName_, currentCol->getDepCol3());

      // depCount is the dereferenced dep1 column of dep2.
      MVColumnInfo *dep2ColInfo =
	mavCols_.getMvColInfoByIndex(currentCol->getDepCol2());
      calcDepCount = 
	buildDepColExpr(calcCorrName_, dep2ColInfo->getDepCol1());
    }

    // Build the condition bottom-up - do the ELSE clause first.
    ItemExpr *elseClause = NULL;
    if (currentCol->getOperatorType() == ITM_AVG)
    {
      // For AVG, the ELSE clause is (SYS_CALC.dep2 / SYS_CALC.dep1).
      elseClause = new(heap_)
        BiArith(ITM_DIVIDE, calcDep2, calcDep1);
    }
    else
    {
      // For STDDEV and VARIANCE, the ELSE clause is
      // ScalarVariance(SYS_CALC.dep3, SYS_CALC.dep2, SYS_CALC.dep1)
      elseClause = new(heap_)
        ScalarVariance(currentCol->getOperatorType(), 
		       new(heap_) Cast(calcDep3, desiredType), 
		       new(heap_) Cast(calcDep2, desiredType), 
		       new(heap_) Cast(calcDep1, desiredType));
    }

    // The WHEN clause is the same for AVG, STDDEV and VARIANCE:
    // WHEN (SYS_CALC.depCount = 0)  THEN NULL
    ItemExpr *whenClause = new(heap_)
      IfThenElse(new(heap_) BiRelat(ITM_EQUAL, 
				    calcDepCount,
				    new(heap_) SystemLiteral(0)),
		 new(heap_) SystemLiteral(), // NULL constant
		 elseClause);

    // Put the CASE on top.
    ItemExpr *newCalcExpr = new(heap_) Case(NULL, whenClause);

    // Add the col expression as SYS_CALC.col
    ColRefName *calcColName = new(heap_) ColRefName(colName, calcCorrName_);
    ItemExpr *newCalcCol = new(heap_) RenameCol(newCalcExpr, calcColName);

    selectList.insert(newCalcCol);
  }

  // The select list is ready. Create the root over topNode.
  RelRoot *newRoot = new(heap_) 
    RelRoot(topNode, REL_ROOT, selectList.convertToItemExpr());
  newRoot->setDontOpenNewScope();

  selectList.clear();
  return newRoot;
}  // MavRelRootBuilder::buildRootForOthers()

//----------------------------------------------------------------------------
// Finally, Add the @GOP (Group Op) column- The select list is (*, <GOP expr>)
// This column is either 1, 2, 3 or 4 (see enum GroupOpNumbers).
// This is later used as the condition that routes the request down to the
// MAV Insert, MAV Delete or MAV Update nodes.
// @GOP is defined as (cnts represents the column name for COUNT(*)):
// CASE 
//   WHEN (SYS_MAV.cnts IS NULL)	                THEN GOP_INSERT
//   WHEN (SYS_MAV.cnts = (0 - SYS_DELTA.cnts))         THEN GOP_DELETE
//   WHEN <minMaxRecomputeCondition_>                   THEN GOP_MINMAX_RECOMPUTE
//   ELSE                                                    GOP_UPDATE
// END
// The minMaxRecomputeCondition_ condition is built during the 
// buildRootForDelta() phase, and checks if any of the current Min/Max values
// were deleted, so that recalculation is needed.
//
// A predicate is added to the RelRoot node to filter self-cancelling delta
// rows. The condition for a self-cancelling row is:
//   (SYS_MAV.cnts IS NULL AND SYS_DELTA.cnts = 0)
//----------------------------------------------------------------------------
RelRoot *MavRelRootBuilder::buildRootForGop(RelExpr *topNode)
{
  // Find the MAV column name for COUNT(*).
  const NAString& countStarColName =
    mavCols_[posOfCountStar_]->getColName();

  // The condition for Insert to MAV
  ItemExpr *insCondition = new(heap_) // (MAV.cnts IS NULL)
    UnLogic(ITM_IS_NULL, 
	    new(heap_) ColReference(new(heap_) 
	      ColRefName(countStarColName, mavCorrName_)));

  // The condition for Delete from MAV
  // (MAV.cnts = (0 - SYS_DELTA.cnts))
  ItemExpr *delCondition = new(heap_)
    BiRelat(ITM_EQUAL, 
  	    new(heap_) ColReference(new(heap_) 
	      ColRefName(countStarColName, mavCorrName_)),
	    new(heap_) BiArith(ITM_MINUS, 
    			       new(heap_) SystemLiteral(0),
			       new(heap_) ColReference(new(heap_) 
			         ColRefName(countStarColName, deltaCorrName_))));

  // These are the constants for all possible GOP column values.
  ItemExpr *constInsert = new(heap_) SystemLiteral(MavBuilder::GOP_INSERT);
  ItemExpr *constDelete = new(heap_) SystemLiteral(MavBuilder::GOP_DELETE);
  ItemExpr *constUpdate = new(heap_) SystemLiteral(MavBuilder::GOP_UPDATE);
  ItemExpr *constRecompFromUpdate = new(heap_) SystemLiteral(MavBuilder::GOP_MINMAX_RECOMPUTE_FROM_UPDATE);
  ItemExpr *constRecompFromInsert = new(heap_) SystemLiteral(MavBuilder::GOP_MINMAX_RECOMPUTE_FROM_INSERT);

  // The condition for recompute when a min/max value was deleted was created
  // during the buildRootForDelta() phase. If there are no Min/Max columns,
  // it is NULL, and therefore redundant.
  ItemExpr *lastClause;
  if (minMaxRecomputeOnUpdateCondition_ == NULL)
    lastClause = constUpdate;
  else
    lastClause = new(heap_) 
      IfThenElse(minMaxRecomputeOnUpdateCondition_, 
                 constRecompFromUpdate, 
		 constUpdate);

  ItemExpr *insertClause;
  if (minMaxRecomputeOnInsertCondition_ == NULL)
    insertClause = constInsert;
  else
    insertClause = new(heap_) 
      Case(NULL, new(heap_) IfThenElse(minMaxRecomputeOnInsertCondition_, 
                 constRecompFromInsert, 
		 constInsert));

  // Build the CASE and IF operators
  ItemExpr *gopExpression = new(heap_) 
    Case(NULL, 
	 new(heap_) IfThenElse(insCondition, insertClause,
	 new(heap_) IfThenElse(delCondition, constDelete, lastClause)));

  // Add the GOP expression as SYS_CALC.@GOP.
  ColRefName *gopColName = new(heap_)
    ColRefName(MavBuilder::getVirtualGopColumnName(), calcCorrName_);
  ItemExpr *gopColumn = new(heap_) RenameCol(gopExpression, gopColName);

  // Start the select list with *
  ItemExprList selectList(heap_);
  addStarToSelectList(selectList);

  selectList.insert(gopColumn);

  // The select list is ready. Create the root over topNode.
  RelRoot *newRoot = new(heap_) 
    RelRoot(topNode, REL_ROOT, selectList.convertToItemExpr());
  newRoot->setDontOpenNewScope();

  selectList.clear();
  return newRoot;
}  // MavRelRootBuilder::buildRootForGop()

//----------------------------------------------------------------------------
// The condition for a self-cancelling delta
// (SYS_MAV.cnts IS NULL AND SYS_DELTA.cnts = 0)
// If the MAV has no GROUP BY clause, the condition is:
// (SYS_MAV.cnts = 0 AND SYS_DELTA.cnts = 0)
//----------------------------------------------------------------------------
ItemExpr *MavRelRootBuilder::buildSelfCancelingDeltaPredicate()
{
  // Find the MAV column name for COUNT(*).
  const NAString& countStarColName = mavCols_[posOfCountStar_]->getColName();

  ItemExpr *mavCountStarCondition = NULL;
  ItemExpr *mavCountStarName = new(heap_) 
    ColReference(new(heap_) ColRefName(countStarColName, mavCorrName_));
  if (isMavWithoutGroupBy())
  {
    // (SYS_MAV.cnts = 0)
    mavCountStarCondition = new(heap_) 
      BiRelat(ITM_EQUAL, mavCountStarName, new(heap_) SystemLiteral(0));
  }
  else
  {
    // (SYS_MAV.cnts IS NULL)
    mavCountStarCondition = new(heap_) 
      UnLogic(ITM_IS_NULL, mavCountStarName);
  }

  // (mavCountStarCondition AND SYS_DELTA.cnts = 0)
  ItemExpr *selfCancelCondition = new(heap_)  
    BiLogic(ITM_AND,
	    mavCountStarCondition,
	    new(heap_) BiRelat(ITM_EQUAL, 
  			       new(heap_) ColReference(new(heap_) 
				 ColRefName(countStarColName, deltaCorrName_)),
			       new(heap_) SystemLiteral(0)));

  ItemExpr *notSelfCancelingDeltaPredicate = 
    new(heap_) UnLogic(ITM_NOT, selfCancelCondition);

  notSelfCancelingDeltaPredicate->setSelectivityFactor(1.0);

  return notSelfCancelingDeltaPredicate;
}  // MavRelRootBuilder::buildSelfCancelingDeltaPredicate()

//----------------------------------------------------------------------------
// Find the bound expression for column number depIndex from colList.
// depIndex is an index into the MVInfo list of MVColumnInfo objects, so we 
// first find the column name from mavCols_ (the MVInfo column list), and 
// then find that column by name from the RETDesc list (colList).
//----------------------------------------------------------------------------
ItemExpr *MavRelRootBuilder::buildDepColExpr(const CorrName& corrName,
				             Int32             depIndex) const
{
  CMPASSERT(depIndex != -1);

  // Find the column name from the MVInfo list.
  const NAString& depColName = 
    mavCols_.getMvColInfoByIndex(depIndex)->getColName();

  ItemExpr *colRef = new(heap_) 
    ColReference(new(heap_) ColRefName(depColName, corrName));

  return colRef;
}  // MavRelRootBuilder::buildDepColExpr()

//----------------------------------------------------------------------------
// create a name for the new aggregate column - this can be either MIN or MAX
ColRefName *MavRelRootBuilder::createExtraAggName(const NAString& mavColName, 
					          extraColumnType columnType) const
{
  NAString newColName = "@";
  newColName += mavColName;
  newColName += columnType == AGG_FOR_DELETE          ?
		MavBuilder::getExtraColSuffixForDel() :
		MavBuilder::getExtraColSuffixForIns();
		
  return new (heap_) ColRefName(newColName, deltaCorrName_);

} // MavRelRootBuilder::createExtraAggName()

//----------------------------------------------------------------------------
// if the aggregation is MAX(A) then we can create two types of columns:
// INS_MAX_A = MAX(CASE @OP WHEN -1 THEN null ELSE SYS_DELTA.A).
// DEL_MAX_A = MAX(CASE @OP WHEN  1 THEN null ELSE SYS_DELTA.A).
// the 1 & -1 values will be refered to as constOpValue.
ItemExpr *MavRelRootBuilder::buildExtraMinMaxExpr(const ItemExpr  *pMinMaxExpr, 
						  const NAString&  colName,
						  extraColumnType  columnType) const
{
  // if the pMinMaxExpr is the MAX(A) then pAggregationSubject is A.
  ItemExpr *pAggregationSubject = pMinMaxExpr->child(0);
  ItemExpr *copyOfAggSubject = pAggregationSubject->copyTree(heap_);
  ColReference *opCol = new(heap_) 
    ColReference(new(heap_) ColRefName(MavBuilder::getVirtualOpColumnName()));
  ConstValue *pConstOpValue = new(heap_) 
    SystemLiteral( (columnType == AGG_FOR_DELETE) ? 1 : -1);
 
  Case *pCase = new(heap_) 
    Case(NULL, new(heap_)
	 IfThenElse(new(heap_) BiRelat(ITM_EQUAL, opCol, pConstOpValue), 
		    new(heap_) SystemLiteral(), // NULL
		    copyOfAggSubject));
  Aggregate *pNewAggregate = new(heap_)
    Aggregate(pMinMaxExpr->getOperatorType(), pCase);

  ColRefName *extraColName = 
    createExtraAggName(colName, columnType);

  ItemExpr *result = new(heap_) RenameCol(pNewAggregate, extraColName);
  return result;
} // buildExtraMinMaxExpr

//----------------------------------------------------------------------------
// Build the conditional expression for deciding if the min/max value of a
// group was deleted from the MAV. 
// This method builds the expression for a specific MAV min/max column. 
// For a MAV column named col, the expression is:
//     SYS_DELTA.@col_DEL IS NOT NULL
//     AND
//     SYS_DELTA.@col_DEL op1 SYS_MAV.col
//     AND
//   ( SYS_DELTA.@col_INS IS NOT NULL   
//     OR   
//     SYS_DELTA.@col_DEL op2 SYS_DELTA.@col_INS )
// op1 is called logVsMavRelation and is <= for MIN, and >= for MAX.
// If full Duplicate Elimination was not performed by the refresh utility, 
// than op2 (called delVsInsertRelation) is the same as op1. Otherwise it 
// does not cover the = case (which means it is < instead of <=, and > instead
// of >=).
#pragma nowarn(262)   // warning elimination 
ItemExpr *MavRelRootBuilder::buildMinMaxRecomputeOnUpdateCondition(const NAString&  mavColName,
								   ColReference    *deltaInsCol,
								   ColReference    *deltaDelCol,
								   OperatorTypeEnum operatorType,
								   NABoolean        wasFullDE)
{
  NABoolean        noDE = TRUE;
  OperatorTypeEnum logVsMavRelation;
  OperatorTypeEnum delVsInsertRelation;

  ColReference *mavColNameRef = new(heap_)
    ColReference(new(heap_) ColRefName(mavColName, mavCorrName_));
  
  if (operatorType == ITM_MIN)
  {
    logVsMavRelation    = ITM_LESS_EQ;
    delVsInsertRelation = wasFullDE  ? 
                          ITM_LESS   : 
                          ITM_LESS_EQ;
  }
  else
  {
    CMPASSERT(operatorType ==  ITM_MAX);
    logVsMavRelation    = ITM_GREATER_EQ;
    delVsInsertRelation = wasFullDE    ? 
                          ITM_GREATER  : 
                          ITM_GREATER_EQ;
  }  

  ItemExpr * delIsNull = new(heap_)
    UnLogic(ITM_IS_NOT_NULL, deltaDelCol->copyTree(heap_));
  ItemExpr * insIsNull = new(heap_)
    UnLogic(ITM_IS_NULL, deltaInsCol->copyTree(heap_));
    
  BiRelat *pLogVsMav = new(heap_)
    BiRelat(logVsMavRelation, 
            deltaDelCol->copyTree(heap_), 
	    mavColNameRef);
  BiRelat *pDelVsIns = new(heap_)
    BiRelat(delVsInsertRelation, 
            deltaDelCol->copyTree(heap_), 
	    deltaInsCol->copyTree(heap_));

  // look at the drawing on top
  BiLogic * finalCondition = new(heap_) 
    BiLogic(ITM_AND, 
            delIsNull, 
	    new(heap_) BiLogic(ITM_AND, 
	                       pLogVsMav, 
			       new(heap_) BiLogic(ITM_OR, 
			                          insIsNull, 
						  pDelVsIns)));

  return finalCondition;
}  // MavRelRootBuilder::buildMinMaxRecomputeOnUpdateCondition()
#pragma warn(262)  // warning elimination 

//----------------------------------------------------------------------------
// Build the conditional expression for deciding if the min/max value of a
// new group was deleted.
// This method builds the expression for a specific MAV min/max column. 
// For a MAV column named col, the expression is:
//   SYS_DELTA.@col_DEL IS NOT NULL
//   AND
//   SYS_DELTA.@col_DEL = SYS_DELTA.@col_INS
// This condition is TRUE when, for example, for a new group (that does not 
// yet appear in the MAV, the operations are: INSERT 10, INSERT 20, DELETE 20.
// In this case we cannot know the new MAX value without recomputing it, 
// unless DE LEVEL is 3. Then - all the Insert-Delete pairs are deleted, and
// the Min/Max computation is straight forward.
ItemExpr *MavRelRootBuilder::buildMinMaxRecomputeOnInsertCondition(ColReference *deltaInsCol,
								   ColReference *deltaDelCol)
{
  ItemExpr * delIsNotNull = new(heap_)
    UnLogic(ITM_IS_NOT_NULL, deltaDelCol->copyTree(heap_));
    
  BiRelat *pDelVsIns = new(heap_)
    BiRelat(ITM_EQUAL, 
            deltaDelCol->copyTree(heap_), 
	    deltaInsCol->copyTree(heap_));

  BiLogic *finalCondition = new(heap_) 
    BiLogic(ITM_AND, delIsNotNull, pDelVsIns);

  return finalCondition;
}  // MavRelRootBuilder::buildMinMaxRecomputeOnInsertCondition()

//----------------------------------------------------------------------------
// The complete condition for Update vs. Recompute is the logical OR of all
// the conditions for specific min/max columns.
void MavRelRootBuilder::addToMinMaxRecomputeCondition(const NAString&  mavColName,
						       ColReference    *deltaInsCol,
						       ColReference    *deltaDelCol,
						       OperatorTypeEnum operatorType,
						       NABoolean        wasFullDE)
{
  ItemExpr *newCondition = 
    buildMinMaxRecomputeOnUpdateCondition(mavColName,
					  deltaInsCol,
					  deltaDelCol,
					  operatorType,
					  wasFullDE);

  if (minMaxRecomputeOnUpdateCondition_ == NULL)
    minMaxRecomputeOnUpdateCondition_ = newCondition;
  else
    minMaxRecomputeOnUpdateCondition_ = new(heap_)
      BiLogic(ITM_OR, minMaxRecomputeOnUpdateCondition_, newCondition);

  // If DE LEVEL is 3 (ALL), the next condition will never happen.
  if (wasFullDE)
    return;

  newCondition = 
    buildMinMaxRecomputeOnInsertCondition(deltaInsCol,
					  deltaDelCol);

  if (minMaxRecomputeOnInsertCondition_ == NULL)
    minMaxRecomputeOnInsertCondition_ = newCondition;
  else
    minMaxRecomputeOnInsertCondition_ = new(heap_)
      BiLogic(ITM_OR, minMaxRecomputeOnInsertCondition_, newCondition);
}  // MavRelRootBuilder::addToMinMaxRecomputeCondition()

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MavRelRootBuilder::addColsToSelectList(LIST(MVColumnInfo *) listOfCols, 
					    ItemExprList&        selectList,
					    const CorrName&      corrName,
					    const CorrName      *renameTo)
{
  for (CollIndex i=0; i<listOfCols.entries(); i++)
  {
    const NAString& colName = listOfCols[i]->getColName();

    ItemExpr *colRef = new(heap_) 
      ColReference(new(heap_) ColRefName(colName, corrName));

    if (renameTo != NULL)
    {
      ColRefName *targetName = new(heap_) ColRefName(colName, *renameTo);
      colRef = new(heap_) RenameCol(colRef, targetName);
    }

    selectList.insert(colRef);
  }
}  // MavRelRootBuilder::addColsToSelectList()

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MavRelRootBuilder::addStarToSelectList(ItemExprList& selectList)
{
  ColRefName *star = new(heap_) ColRefName(TRUE, heap_);
  star->setStarWithSystemAddedCols();
  ItemExpr *starRef = new(heap_) ColReference(star);

  selectList.insert(starRef);
}  // MavRelRootBuilder::addStarToSelectList()

