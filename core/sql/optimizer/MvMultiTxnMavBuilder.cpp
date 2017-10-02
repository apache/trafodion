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
* File:         MvMultiTxnMavBuilder.cpp
* Description:  Class MvMultiTxnMavBuilder and MultiTxnDEMavBuilder, 
*               part of the class hierarchy of MvRefreshBuilder, 
*               for MV INTERNAL REFRESH command.
*
* Created:      12/27/2000
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/

#include "AllItemExpr.h"
#include "AllRelExpr.h"
#include "BindWA.h"
#include "MVInfo.h"
#include "RelSequence.h"
#include "BinderUtils.h"
#include "ChangesTable.h"
#include "MvRefreshBuilder.h"

// ===========================================================================
// ===========================================================================
// ===================  class  MvMultiTxnMavBuilder    =======================
// ===========================================================================
// ===========================================================================

//----------------------------------------------------------------------------
// For Multi-Transactional refresh, if this is a phase 1 or a catchup 
// activation, we need to read the previous context rows and pipeline them
// as input to the refresh tree. 
RelExpr *MvMultiTxnMavBuilder::buildRefreshTree()
{
  CMPASSERT(pMultiTxnClause_ != NULL);

  RelExpr *refreshTree = MavBuilder::buildRefreshTree();

  if (pMultiTxnClause_->isPhase1() || 
      pMultiTxnClause_->isCatchup()  )
  {
    // Open a new scope above the refresh tree.
    refreshTree = new(heap_) RelRoot(refreshTree);

    // Build the sub-tree for reading the previous context rows.
    RelExpr *readContextTree = buildReadPreviousContext();

    // Connect the two trees with a TSJ to pipeline the context rows
    // to the refresh tree.
    refreshTree = new(heap_) Join(readContextTree, refreshTree, REL_TSJ);
  }

  return refreshTree;
}  // MvMultiTxnMavBuilder::buildRefreshTree()

//----------------------------------------------------------------------------
// Build the sub-tree for reading the phase 1 and catchup context rows from
// the log. The phase 1 row is given the name StartCtx, and the catchup row
// is given the name EndCtx. The phase 1 row is deleted from the log.
// Here is the tree:
//                   Join
//                 /      \
//               /          \
//         RelRoot          RelRoot
//            |                |
//         LeftTSJ           Rename
//        /       \            |
//   Rename      RelRoot     Scan
//      |           |
//   RelRoot    LeafDelete
//      |
//    Scan
//
// Since both phase 1 and catchup are optional, the tree can be constructed
// with either one or both sub-trees. When both are specified, the Join node
// is a cross join that unites both rows to one row.
RelExpr *MvMultiTxnMavBuilder::buildReadPreviousContext()
{
  NABoolean  isPhase1    = (pMultiTxnClause_->getPhase() == 1);
  ItemExpr  *catchupNo   = pMultiTxnClause_->getCatchup();
  RelExpr   *phase1Tree  = NULL;
  RelExpr   *catchupTree = NULL;

  // Build the object for the IUD log for reading context rows.
  DeltaDefinition *deltaDef = getDeltaDefList()->at(0);
  MvLogForContextRows 
    logTableObj(*deltaDef->getTableName(), 
                getMvCorrName(),
		deltaDef->getBeginEpoch(),
		catchupNo, 
		bindWA_);
  if (bindWA_->errStatus())
    return NULL;

  if (isPhase1)
  { // Build the phase 1 sub-tree
    // Build the Scan on the log to read thw Phase1 context row.
    Scan *scanNode = logTableObj.buildScan(ChangesTable::PHASE1_ROWS);

    // Build the Delete node on top of the Scan.
    Delete  *deleteNode = new(heap_) 
      Delete(scanNode->getTableName(), NULL, REL_UNARY_DELETE, scanNode);
    deleteNode->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;
    // Make the Delete node project the deleted row as its output.
    deleteNode->getInliningInfo().setFlags(II_NeedGuOutputs);

    // Put a root on top.
    RelRoot *rootNode   = new(heap_) RelRoot(deleteNode);

    // Build the GroupBy to detect when the context log is empty.
    RelExpr *groupByNode = buildErrorOnNoContext(rootNode);

    // And rename the row to START_CTX
    phase1Tree = 
      buildPhase1SelectList(groupByNode, logTableObj.getSubjectNaTable());
  }

  if (catchupNo)
  { // Build the catchup sub-tree.

    // Build the Scan on the log to read thw Catchup context row.
    Scan *scanNode = logTableObj.buildScan(ChangesTable::CATCHUP_ROWS);
    RelRoot *rootNode = new(heap_) RelRoot(scanNode);
    rootNode->setDontOpenNewScope();

    RelExpr *renameNode = new(heap_) 
      RenameTable(rootNode, getEndCtxName());
    RelRoot *topRoot = new(heap_) RelRoot (renameNode);
    topRoot->setDontOpenNewScope();
    catchupTree = topRoot;
  }

  // Return the sub-tree that was created, or a join of both.
  RelExpr *topNode = phase1Tree;
  if (topNode == NULL)
    topNode = catchupTree;
  else
    if (catchupTree != NULL)
    {
      // This is a cross join, only one row from each side.
      topNode = new(heap_) Join(phase1Tree, catchupTree); 
    }

  CMPASSERT(topNode != NULL);
  return topNode;
}  // MvMultiTxnMavBuilder::buildReadPreviousContext()

//----------------------------------------------------------------------------
// This method builds a GroupBy node with a OneTrue aggregate predicate. The 
// selection predicate will evaluate to TRUE when at least one row is returned
// by the child. Whwne the child returns no rows, the predicate will cause
// an error to be generated, using the RaiseError expression. This will tell
// the refresh utility that there are no context rows in the context log, so 
// the work is done.
// The selection predicate is:
//           case
//            |
//          IfThenElse
//            |
//           / \
//         /  |  \
//       =   True  RaiseError
//     /   \
//    0     COUNT(*)
RelExpr *MvMultiTxnMavBuilder::buildErrorOnNoContext(RelExpr *topNode)
{
  ItemExpr *countStar = new(heap_)
    Aggregate(ITM_COUNT, 
              new (heap_) SystemLiteral(1),
              FALSE /*i.e. not distinct*/,
              ITM_COUNT_STAR__ORIGINALLY, '!');

  ItemExpr *newAggExpr = new (heap_) 
    BiRelat(ITM_NOT_EQUAL, countStar, new(heap_) SystemLiteral(0));

  // 12316 No corresponding context row was found in the context log.
  ItemExpr *errorExpr = new (heap_) RaiseError(12316);

  // Case( If (COUNT(*) = 0) THEN TRUE ELSE RaiseError).
  ItemExpr *grbySelectionPred = new (heap_)
    Case(NULL, new (heap_) 
      IfThenElse(newAggExpr,
		 new (heap_) BoolVal(ITM_RETURN_TRUE),
		 errorExpr));

  // Create a GroupBy on topNode, and attach the new case as a 
  // "having" predicate.
  RelExpr * newGrby = new(heap_) GroupByAgg(topNode, REL_GROUPBY);
  newGrby->addSelPredTree(grbySelectionPred);
  
  return newGrby;
}

//----------------------------------------------------------------------------
// The GroupBy node needs to project the CI columns, without adding them
// as grouping columns (this will cause the selection predicate to be 
// ignored when no context row is read from the context table). So we add 
// for each CI column, a MAX(col) aggregate, that will allow it to pass the 
// GroupBy node.
RelExpr *MvMultiTxnMavBuilder::buildPhase1SelectList(RelExpr       *topNode,
						     const NATable *baseNaTable)
{
  CorrName emptyCorrName("");
  ItemExpr *ciCols = 
    BinderUtils::buildClusteringIndexVector(baseNaTable,
                                            heap_, 
					    &emptyCorrName,
					    SP_USE_AT_SYSKEY);

  ItemExprList ciColsList(ciCols, heap_);   // List of references for CI cols
  ItemExprList aggregatedCiColsList(heap_); // List of MAX(col) 
  ItemExprList renameColList(heap_);        // List of RenameCols
  for (CollIndex i=0; i<ciColsList.entries(); i++)
  {
    ItemExpr *currentCol = ciColsList[i];

    // Add the MAX(col) to the select list.
    aggregatedCiColsList.insert(new(heap_) Aggregate(ITM_MAX, currentCol));

    // Add the RenameCol to the rename list.
    if (currentCol->getOperatorType() == ITM_RENAME_COL)
      renameColList.insert(currentCol->copyTree(heap_));
    else
    {
      CMPASSERT(currentCol->getOperatorType() == ITM_REFERENCE);
      ColReference *currentColRef = (ColReference *)currentCol;
      renameColList.insert(new(heap_) 
        RenameCol(NULL, &currentColRef->getColRefNameObj()));
    }
  }

  // Use the list of MAX(col) aggregates as the RelRoot select list.
  RelRoot *topRoot = new(heap_) 
    RelRoot(topNode, REL_ROOT, aggregatedCiColsList.convertToItemExpr());
  topRoot->setDontOpenNewScope(); 

  // Use the list of RenameCols in the RenameTable node.
  RelExpr *renameNode = new(heap_) 
    RenameTable(topRoot, getStartCtxName(), renameColList.convertToItemExpr());

  return renameNode;
}

//----------------------------------------------------------------------------
// refinement of the base class
RelExpr *MvMultiTxnMavBuilder::buildReadIudLogBlock() 
{
  // For Multi-Txn refresh we need to add the original selection predicates.
  // These predicates are normally put above the scan log block, so they stay
  // on the Sequence node. This is dangerous because the row that should be 
  // inserted as a context row, can be rejectedby these predicates. The 
  // original predicates are saved in the MVInfo.
  getLogsInfo().getMvIudlog().setAddOrigScanPredicateFlag(TRUE);
  RelExpr *topNode = MvRefreshBuilder::buildReadIudLogBlock();

  if (!getLogsInfo().getDeltaDefinition().useRangeLog())
  {
    // Build the select list of the root.
    // We need all the table's columns, not only the CI.
    topNode = buildRootOverIUDLog(topNode);
  }
  // else { The root node is already built for the union with the range read block
  //        so it is not necessary  }

  return topNode;
}  // MvMultiTxnMavBuilder::buildReadIudLogBlock()

//----------------------------------------------------------------------------
// In the Internal refresh multi-txn tree we union the records from the IUD log and  
// the table records that are joined with the range log 
// (join predicate: table.CI between range.begin and range.end). The union must generate the 
// records orderd by the IUD log CI and the table CI (the same union cols). In order to accomplish   
// that, we demand from the union to sort the records by CI and we replace the requested physical 
// order from the right child (range log joined with the table) with the following 
// vector (end range cols,rangeId,table clustering index)
// The Refresh utility DE phase make sure that the range log contains only 
// non overlapping ranges and therefore this ordering is the same
// as ordering by the table clustering index, however by this ordering vector we accomplish 
// the following optimization :
//
//	   UNION		       UNION
//	/	  \		    /	      \
//  Scan 	  Sort		  Scan 	  Nested Join		  
// Iud log	   | 		 Iud log   /      \
//	       Nested Join  -->		 Sort    Scan 
//	        /      \		  |        T
//	    Scan        Scan		 Scan  
//	  Range Log       T	       Range Log
//
// Ofcourse that sorting the range log records is must cheaper then sorting 
// the table records after the join.
// Excluded from coverage test - used only with range logging.
Union *MvMultiTxnMavBuilder::buildUnionBetweenRangeAndIudBlocks(RelExpr *scanIUDLogBlock,
							      RelExpr *scanRangeLogBlock) const
{
  Union *unionNode = 
    MvRefreshBuilder::buildUnionBetweenRangeAndIudBlocks(scanIUDLogBlock, scanRangeLogBlock);

  return unionNode;
}  // MvMultiTxnMavBuilder::buildUnionBetweenRangeAndIudBlocks()

//----------------------------------------------------------------------------
// Excluded from coverage test - used only with range logging.
NABoolean MvMultiTxnMavBuilder::needAlternateCIorder() const
{
  return TRUE;
}

//----------------------------------------------------------------------------
ItemExpr *MvMultiTxnMavBuilder::buildSelectionListForScanOnIudLog() const
{
  return MvRefreshBuilder::buildSelectionListForScanOnIudLog();
}  // MvMultiTxnMavBuilder::buildSelectionListForScanOnIudLog()

//----------------------------------------------------------------------------
ItemExpr *MvMultiTxnMavBuilder::buildSelectionPredicateForScanOnIudLog() const
{
  DeltaDefinition *deltaDef = &getLogsInfo().getDeltaDefinition();
  
  ItemExpr *selectionPred = NULL;
  
  if (!useUnionBakeboneToMergeEpochs())
  {
     selectionPred = MvRefreshBuilder::buildSelectionPredicateForScanOnIudLog();
  }

  if (pMultiTxnClause_->isCatchup() || pMultiTxnClause_->isPhase1())
  {
    ItemExpr *contextSelectionPred = addContextPredicatesOnIUDLog();

    if (selectionPred)
    {
      selectionPred =  new(heap_) BiLogic(ITM_AND, 
					  selectionPred, 
					  contextSelectionPred);
    }
    else
    {
      selectionPred = contextSelectionPred;
    }
  }

  return selectionPred;
}  // MvMultiTxnMavBuilder::buildSelectionPredicateForScanOnIudLog()

//----------------------------------------------------------------------------
ItemExpr *MvMultiTxnMavBuilder::addContextPredicatesOnIUDLog() const

{
  ItemExpr *result = NULL;
  ItemExpr *phase1Predicate = NULL;
  ItemExpr *CatchupPredicate = NULL;

  if (pMultiTxnClause_->isPhase1())
  {
    // Add to scanNode predicate on Log.CI >= StartCtx.CI
    ItemExpr *logVector = 
      BinderUtils::buildClusteringIndexVector(getLogsInfo().getBaseNaTable(),
					      heap_,
					      &getLogsInfo().getIudLogTableName(),
					      SP_USE_AT_SYSKEY);


    const CorrName startCtxName(getStartCtxName());

    ItemExpr *startCtxVector = 
      BinderUtils::buildClusteringIndexVector(getLogsInfo().getBaseNaTable(),
					      heap_,
					      &startCtxName,
					      SP_USE_AT_SYSKEY);

    BiRelat *selectionPredicate = new(heap_)
      BiRelat(ITM_GREATER_EQ, logVector, startCtxVector);

    selectionPredicate->setDirectionVector(
	  getLogsInfo().getBaseTableDirectionVector());

    phase1Predicate = selectionPredicate;
  }

  if (pMultiTxnClause_->isCatchup())
  {
    // Add to scanNode predicate on Log.CI < EndCtx.CI
    ItemExpr *logVector = 
      BinderUtils::buildClusteringIndexVector(getLogsInfo().getBaseNaTable(),
					      heap_,
					      &getLogsInfo().getIudLogTableName(),
					      SP_USE_AT_SYSKEY);

    const CorrName endCtxName(getEndCtxName());
    ItemExpr *endCtxVector = 
      BinderUtils::buildClusteringIndexVector(getLogsInfo().getBaseNaTable(),
					      heap_,
					      &endCtxName,
					      SP_USE_AT_SYSKEY);

    BiRelat *selectionPredicate = new(heap_)
      BiRelat(ITM_LESS, logVector, endCtxVector);

    selectionPredicate->setDirectionVector(
	  getLogsInfo().getBaseTableDirectionVector());

    CatchupPredicate = selectionPredicate;
  }

  if (phase1Predicate != NULL && CatchupPredicate !=NULL)
  {
     return new(heap_) BiLogic(ITM_AND, phase1Predicate, CatchupPredicate);
  }

  if (phase1Predicate != NULL)
  {
    return phase1Predicate;
  }
  
  CMPASSERT(CatchupPredicate!=NULL);

  return CatchupPredicate;
}  // MvMultiTxnMavBuilder::addContextPredicatesOnIUDLog()

//----------------------------------------------------------------------------
// This method is called by Scan::bindNode(), when getting to the Scan on
// the base table in the original MV select statement. The scanNode parameter
// was already transformed to a scan on the log. Here we add additional 
// optional selection predicates that compare to the two previous context 
// rows, so the range of rows read is From StartCtx (Phase 1) To EndCtx 
// (Catchup). On top of the Scan node we then build the rest of the sub-tree.
RelExpr *MvMultiTxnMavBuilder::buildLogsScanningBlock(const QualifiedName&  baseTable)
{ 
  CMPASSERT(pMultiTxnClause_ != NULL);

  // Call the superclass method to add the Rename node.
  RelExpr *topNode = MvRefreshBuilder::buildLogsScanningBlock(baseTable);

  if (bindWA_->errStatus() || topNode == NULL)
    return NULL;

// Build the Sequence node to detect the last row to be processed.
  ItemExpr *isLastExpr = buildSequenceIsLastExpr();
  RelSequence *sequenceNode = buildSequenceOnScan(topNode, isLastExpr);

/*  sequenceNode->addCardConstraint(
	      (Cardinality) 0, 
	      (Cardinality) pMultiTxnClause_->getCommitEach());

*/
  topNode = buildRootOverSequence(sequenceNode, isLastExpr);

  // If not done, insert a context row for the next activation.
  topNode = buildInsertContextTree(topNode);

  return topNode;
}  // MvMultiTxnMavBuilder::buildLogsScanningBlock()

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvMultiTxnMavBuilder::buildSequenceIsLastExpr() const
{
 const NATable *baseNaTable = getLogsInfo().getBaseNaTable();

  // RUNNINGSUM(1) >= <CommitEachRows>
  ItemExpr *runningCountPred = new(heap_)   
    BiRelat(ITM_GREATER_EQ, 
	    new(heap_) ItmSeqRunningFunction (ITM_RUNNING_SUM,
	                                      new(heap_) SystemLiteral(1)),
	    new(heap_) SystemLiteral(pMultiTxnClause_->getCommitEach()));

  // Log.CI <> OFFSET(Log.CI, 1)
  ItemExpr *nonEqualCiPred = new(heap_)
    BiRelat(ITM_NOT_EQUAL, 
	    BinderUtils::buildClusteringIndexVector(baseNaTable, heap_, NULL, SP_USE_AT_SYSKEY),
	    BinderUtils::buildClusteringIndexVector(baseNaTable, heap_, 
	                                            NULL, SP_USE_AT_SYSKEY | SP_USE_OFFSET) );

  // isLast is the logical AND of these two expressions.
  ItemExpr *isLastExpr = 
    new(heap_) BiLogic(ITM_AND, runningCountPred, nonEqualCiPred);

  return isLastExpr;
}  // MvMultiTxnMavBuilder::buildSequenceIsLastExpr()

//----------------------------------------------------------------------------
// Build the Sequence node on top of the logs.
// The Sequence expression isLast becomes TRUE on the last row that is to be 
// projected up from the Sequence node (the row that should be inserted into 
// the log as the context row, but not aggregated by the GroupBy). It becomes
// TRUE when:
// 1) # of rows processed so far (RUNNINGSUM(1)) >= the COMMIT EACH parameter.
//    AND
// 2) The value of the clustering index columns of the current row is not 
//    equal to that of the last row.
// The isLast expression is used as a cancel expression in the Sequence node,
// and is also projected to be used fort inserting the next context row.
// During execution, the cancel expression is evaluated for each row. When
// it becomes TRUE, the Sequence node begins cancel processing, to avoid
// reading any more data.
RelSequence *MvMultiTxnMavBuilder::buildSequenceOnScan(RelExpr *topNode, ItemExpr *isLastExpr) const
{
  const NATable *baseNaTable = getLogsInfo().getBaseNaTable();
  
  // The order vector for the Sequence node (SEQUENCE BY syntax) is the 
  // clustering index.
  ItemExpr *orderVector = BinderUtils::buildClusteringIndexVector(
			   baseNaTable, 
			   heap_, 
			   &getLogsInfo().getBaseTableName(), 
			   SP_USE_AT_SYSKEY, 
			   getLogsInfo().getBaseTableDirectionVector(),
			   getSequenceByPrefixColName());

  if (getLogsInfo().getDeltaDefinition().useRangeLog() &&
      !getLogsInfo().getDeltaDefinition().useIudLog())
  {
    const CorrName &tableNameCorr = getLogsInfo().getBaseTableName();

    // For sort optimization we use an order vector of @ER columns,@RANGE_ID,CI columns
    orderVector = buildAlternateCIorder(orderVector, tableNameCorr);
  }

  // Build the Sequence node, and add to it the cancel expression.
  RelSequence *sequenceNode = new(heap_) RelSequence(topNode, orderVector);
  sequenceNode->setCancelExprTree(isLastExpr);
  
  return sequenceNode;
}  // MvMultiTxnMavBuilder::buildSequenceOnScan()

//----------------------------------------------------------------------------
RelRoot *MvMultiTxnMavBuilder::buildRootOverSequence(RelExpr *topNode, 
						   ItemExpr *isLastExpr) const
{
  // Give the isLast expression the name @ISLAST
  ItemExpr *isLastColumn = new(heap_) 
    RenameCol(isLastExpr, new(heap_) ColRefName(getVirtualIsLastColName()));

  // The root select list is (*, isLastExpr).
  ColRefName *star = new(heap_) ColRefName(1);  // isStar
  star->setStarWithSystemAddedCols();
  ItemExpr *rootSelectList = new(heap_) ItemList(new(heap_) ColReference(star), isLastColumn);
    
  RelRoot *rootNode = new(heap_) RelRoot(topNode, REL_ROOT, rootSelectList);
  rootNode->setDontOpenNewScope(); 

  return rootNode;

} // MvMultiTxnMavBuilder::buildRootOverSequence()

//----------------------------------------------------------------------------
// Build the sub-tree that inserts the context row into the log.
// The result looks like this:
//            RelRoot
//               |
//           SEMI TSJ
//            /     \
//  leftTopNode      RelRoot
//                     |
//                   Rename
//                     |
//                  Cond.Union (IF)
//                   /        \
//             RelRoot        RelRoot
//                |              |
//            LeafInsert       Tuple
//
// In execution time, all the rows are pipelined from the Sequence node
// (passed as the leftTopNode parameter), through the TSJ to the conditional
// Union node. The condition expression is isLast, so all the rows except
// the last row are sent to the Tuple node, that returns the TRUE constant.
// The last row is sent to the Insert node, inserted into the log as the 
// context row, and a FALSE constant is returned. The conditional Union
// node projects the union of both constants, as a single column of output.
// This column is renamed @IF_OUTPUT.@IF_OUTPUT_COL and projected to the TSJ
// node, where it is evaluated as the Join predicate. The result is that all
// the data rows are projected, but the last row is not.
RelExpr *MvMultiTxnMavBuilder::buildInsertContextTree(RelExpr  *leftTopNode)
{
  NAString ifOutputColName  ("@IF_OUTPUT_COL");
  NAString ifOutputTableName("@IF_OUTPUT");
  CorrName ifOutputCorrName(ifOutputTableName);

  // Build the LeafInsert on the Log (returns FALSE).
  RelExpr *insertNode = buildInsertContextNode();

  // The Tuple node, returns TRUE.
  ItemExpr *tupleExpr = new(heap_) BoolVal(ITM_RETURN_TRUE);
  RelExpr  *tupleNode = new(heap_) Tuple(tupleExpr);
  tupleNode = new(heap_) RelRoot(tupleNode);

  // The conditional predicate of the IF node
  ItemExpr *ifCondition = new(heap_) 
    ColReference(new(heap_) ColRefName(getVirtualIsLastColName()));

  // The conditional Union node.
  Union *ifNode = new(heap_) Union(insertNode, tupleNode, NULL, ifCondition,
				   REL_UNION, CmpCommon::statementHeap(), TRUE);

  // Build the select list of a single column called @IF_OUTPUT_COL.
  ItemExpr *renameList = new(heap_)
    RenameCol(NULL, new(heap_) ColRefName(ifOutputColName) );    

  // The Rename node
  RelExpr *rightTopNode = new(heap_) 
    RenameTable(ifNode, ifOutputTableName, renameList);
  rightTopNode = new(heap_) RelRoot(rightTopNode);

  Join *joinNode = new(heap_) 
    Join(leftTopNode, rightTopNode, REL_SEMITSJ);

  joinNode->setTSJForWrite(TRUE);

  //We must guarantee that the sub-tree which includes 
  //the insert into the context table will only be executed once.
  //We achieve it by preventing the transformation of the TSJ node that 
  //inserts the context row to a physical node 
  //if the number of input request is more than 1.
  joinNode->getInliningInfo().setFlags(II_SingleExecutionForTSJ);

  // Open an additional bind scope to isolate the tree below.
  RelExpr *topNode = new(heap_) RelRoot(joinNode);
  return topNode;
}  // MvMultiTxnMavBuilder::buildInsertContextTree()

//----------------------------------------------------------------------------
// Build the LeafInsert node that inserts the context row into the log.
RelExpr *MvMultiTxnMavBuilder::buildInsertContextNode()
{
  DeltaDefinition *deltaDef = getDeltaDefList()->at(0);
  MvLogForContextRows 
    logTableObj(*deltaDef->getTableName(), 
                getMvCorrName(),
		deltaDef->getBeginEpoch(),
		pMultiTxnClause_->getCatchup(), 
		bindWA_);

  if (bindWA_->errStatus())
    return NULL;

  RelExpr *insertNode = logTableObj.buildInsert(TRUE);

  CMPASSERT(insertNode->getOperatorType() == REL_UNARY_INSERT ||
            insertNode->getOperatorType() == REL_LEAF_INSERT);
  // The inserted context row is the only row that should be counted in
  // the internal refresh statement. The refresh utility uses this to
  // decide if another invocation is needed.
  ((GenericUpdate *)insertNode)->rowsAffected() = 
    GenericUpdate::COMPUTE_ROWSAFFECTED;  

  // The projected FALSE value is used in the TSJ join predicate.
  ItemExpr *tupleExpr = new(heap_) BoolVal(ITM_RETURN_FALSE);
  RelRoot *rootNode = new (heap_) RelRoot(insertNode, REL_ROOT, tupleExpr);

  return rootNode;
}  // MvMultiTxnMavBuilder::buildInsertContextNode()

//////////////////////////////////////////////////////////////////////////////
// This function adds 3 predicates :
// a) A predicate on "@EPOCH" column - the range of epochs
// b) Catchup predicates
// c) Phase1 predicates
// Excluded from coverage test - used only with range logging.
ItemExpr *MvMultiTxnMavBuilder::buildSelectionPredicateForScanOnRangeLog() const
{
  // a)
  // The epoch predicate is build in the parent because it is common for all 
  // sub-classes
  ItemExpr *result = MvRefreshBuilder::buildSelectionPredicateForScanOnRangeLog();

  if (pMultiTxnClause_->isCatchup())
  {
    // b)
    ItemExpr *pred = buildCatchupSelectionPredicateForScanOnRangeLog();
  
    if (result == NULL)
    {
      result = pred;
    }
    else
    {
      result = new(heap_) BiLogic(ITM_AND, result, pred);
    }
  }

  if (pMultiTxnClause_->isPhase1())
  {
    // c)
    ItemExpr *pred = buildPhase1SelectionPredicateForScanOnRangeLog();

    if (result == NULL)
    {
      result = pred;
    }
    else
    {
      result = new(heap_) BiLogic(ITM_AND, result, pred);
    }
  }
 
  return result;
}

//////////////////////////////////////////////////////////////////////////////
// Add the following selection predicate to the Scan of the range log for 
// multi-transactional operation:
//   a) RangeLog.BR_CI <  EndCtx.CI   DIRECTEDBY <direction-vector>
//
// Predicate a) is only added for Catchup activations.
// Excluded from coverage test - used only with range logging.
ItemExpr *MvMultiTxnMavBuilder::buildCatchupSelectionPredicateForScanOnRangeLog() const
{
  // a)
  ItemExpr *beginRangeCI = buildBeginRangeVector();

  const CorrName endCtxName(getEndCtxName());
  ItemExpr *endCtx = 
    BinderUtils::buildClusteringIndexVector(getLogsInfo().getBaseNaTable(), 
					    heap_, 
                                            &endCtxName, 
					    SP_USE_AT_SYSKEY);
  
  BiRelat *predA = new(heap_) BiRelat(ITM_LESS, beginRangeCI, endCtx);

  predA->setDirectionVector(getLogsInfo().getBaseTableDirectionVector());

  return predA;

} // MvMultiTxnMavBuilder::buildCatchupSelectionPredicateForScanOnRangeLog()

//////////////////////////////////////////////////////////////////////////////
// Add the following selection predicate to the Scan of the range log for 
// multi-transactional operation:
//   a) RangeLog.ER_CI >= StartCtx.CI DIRECTEDBY <direction-vector>
//        AND
//   b) CASE RangeLog.RangeType OF
//        2      THEN TRUE    -- Range closed on upper bound [)
//        3      THEN TRUE    -- Range closed on both bounds []
//        ELSE   RangeLog.ER_CI <> StartCtx.CI
// Predicate a) and b) are only added for Phase 1 activations.
// Predicate c) makes sure that predicate b) is correct with respect to the
// range type. When the upper boundary of the range is closed, the condition
// is >=, otherwise it is >.
// Excluded from coverage test - used only with range logging.
ItemExpr *MvMultiTxnMavBuilder::buildPhase1SelectionPredicateForScanOnRangeLog() const
{
  // a)
  ItemExpr *endRangeCI = buildEndRangeVector();

  const CorrName startCtxName(getStartCtxName());

  ItemExpr *startCtx = 
    BinderUtils::buildClusteringIndexVector(getLogsInfo().getBaseNaTable(), 
					    heap_, 
                                            &startCtxName, 
					    SP_USE_AT_SYSKEY);
  
  BiRelat *predB = new(heap_) BiRelat(ITM_GREATER_EQ, endRangeCI, startCtx);

  predB->setDirectionVector(getLogsInfo().getBaseTableDirectionVector());

  // b)
  ItemExpr *predC2 = new(heap_) BoolVal(ITM_RETURN_TRUE);
  ItemExpr *predC3 = new(heap_) BoolVal(ITM_RETURN_TRUE);
  ItemExpr *predC01 = new(heap_) 
    BiRelat(ITM_NOT_EQUAL, 
	    endRangeCI->copyTree(heap_),
	    startCtx->copyTree(heap_) );

  ItemExpr *rangeType = new(heap_)
    ColReference(new(heap_) ColRefName(COMMV_RANGE_TYPE_COL) );

  ItemExpr *predC  = new(heap_) 
    Case(rangeType, new(heap_) 
      IfThenElse(new(heap_) SystemLiteral(ComMvRangeClosedUpperBound), // If 2
		 predC2, new(heap_)
      IfThenElse(new(heap_) SystemLiteral(ComMvRangeOpenBothBounds),   // If 3
	         predC3, 
		 predC01) ) );

  return new(heap_) BiLogic(ITM_AND, predB, predC);

}  // MvMultiTxnMavBuilder::buildPhase1SelectionPredicateForScanOnRangeLog()

//----------------------------------------------------------------------------
// For multi transactional refresh, there are two additional predicates to
// the range log join predicate, in addition to the predicates in
// MvRefreshBuilder::buildRangeLogJoinPredicate().
// When phase 1:
//    StartCtx.CI <= BASE_TABLE.CI DIRECTEDBY <direction-vector>
// When Catchup
//    EndCtx.CI   >  BASE_TABLE.CI DIRECTEDBY <direction-vector>
// Excluded from coverage test - used only with range logging.
ItemExpr *MvMultiTxnMavBuilder::buildRangeLogJoinPredicate() const
{
  // Call the base class for the normal predicate.
  ItemExpr *predicate = MvRefreshBuilder::buildRangeLogJoinPredicate();

  // For simple phase 0 with no catchup, no need for additional predicates.
  if (!pMultiTxnClause_->isPhase1() && !pMultiTxnClause_->isCatchup())
    return predicate;

  ItemExpr *baseCI = BinderUtils::buildClusteringIndexVector(
				getLogsInfo().getBaseNaTable(), 
				heap_,
				NULL,
				SP_USE_AT_SYSKEY);

  if (pMultiTxnClause_->isPhase1())
  {
    // Phase 1 predicate:
    CorrName startCtxName(getStartCtxName());
    ItemExpr *startCtxCI =
      BinderUtils::buildClusteringIndexVector(getLogsInfo().getBaseNaTable(), 
					      heap_, 
					      &startCtxName, 
					      SP_USE_AT_SYSKEY);

    BiRelat *predD = new(heap_)
      BiRelat(ITM_LESS_EQ, startCtxCI, baseCI);
    predD->setDirectionVector(getLogsInfo().getBaseTableDirectionVector());

    predicate = new(heap_) BiLogic(ITM_AND, predicate, predD);
  }

  if (pMultiTxnClause_->isCatchup())
  {
    // If we have already used baseCI for phase 0, than make another copy.
    if (pMultiTxnClause_->isPhase1())
      baseCI = baseCI->copyTree(heap_);

    // Catchup predicate:
    CorrName endCtxName(getEndCtxName());
    ItemExpr *endCtxCI =
      BinderUtils::buildClusteringIndexVector(getLogsInfo().getBaseNaTable(), 
					      heap_, 
					      &endCtxName, 
					      SP_USE_AT_SYSKEY);

    BiRelat *predE = new(heap_)
      BiRelat(ITM_GREATER, endCtxCI, baseCI);
    predE->setDirectionVector(getLogsInfo().getBaseTableDirectionVector());

    predicate = new(heap_) BiLogic(ITM_AND, predicate, predE);
  }

  return predicate;
}  // MvMultiTxnMavBuilder::buildRangeLogJoinPredicate()

//----------------------------------------------------------------------------
// Do not use unions when their are too many epochs because it will blow the
// optimizer plan and when their is a single epoch (it is not needed)
NABoolean MvMultiTxnMavBuilder::useUnionBakeboneToMergeEpochs() const
{
  DeltaDefinition *deltaDef = &getLogsInfo().getDeltaDefinition();

  if (Refresh::SINGLEDELTA != getRefreshNode()->GetRefreshType())
    return FALSE;

  if (deltaDef->getEndEpoch() - deltaDef->getBeginEpoch() > 
      MAX_EPOCH_FOR_UNION_BACKBONE)
    return FALSE;

  if (deltaDef->getBeginEpoch() == deltaDef->getEndEpoch())
    return FALSE;

  return TRUE;
}

// ===========================================================================
// ===========================================================================
// ===================  class  MultiTxnDEMavBuilder    =======================
// ===========================================================================
// ===========================================================================

// Exclude from coverage testing - used only with range loggiing

// This class performs a single rows vs. ranges duplicate elimination algorithm 
// in the internal refresh delta computation block.
// The algorithm ignores IUD log rows that are "covered" by a range.
// An IUD log row is covered by a range when it is between the range boundries 
// and the action on the table for that row was done after the range operation
// (so the IUD log syskey will be greater then the range id of the range log, 
// which is the IUD syskey of the BR row, updated in the range log by the 
// refresh utility duplicate elimination range resolution stage)

// The main idea of the algorithm is to have the union of the rows from the 
// IUD log and base table join with the range log, sorted by the base table 
// CI. Then for each record, by looking up with the help of the LAST_NOT_NULL 
// sequence function, decide whether the record is "covered" by a range or not.
// This requires that for each range there will be at least one row that its 
// CI vector will be equal to the BR vector of the range and it will have all 
// the BR, ER, and RangeID values of that range. The following algorithm 
// produces this extra row that is later referenced by the value of its 
// "@SPECIAL" column (that is equal to VIRTUAL_BEGIN_RANGE). This algorithm 
// stronglly relies on the range resolution stage of the duplicate elimination
// algorithm in the refresh utility. After that stage we assume that the range
// log does not contain overlapping ranges

const char MultiTxnDEMavBuilder::virtualRangeSpecialCol_[] = "@SPECIAL";
const char MultiTxnDEMavBuilder::sequenceByPrefixColName_[] = "@SORTKEY_";
const char MultiTxnDEMavBuilder::lastNotNullPrefixColName_[] = "@LNN_";

//----------------------------------------------------------------------------
// There is an additional requirement for the order of rows under the sequence
// node, due to the following case:
// A range with a BR vector that equals to the CI vector of a single row IUD 
// log record. In this case ordering by CI alone will not be enough
// and the duplicate elimination algorithm requires that the virtual range 
// column will be ordered before those singles. Of course the order is 
// significant for singles that where logged after the range because they are 
// the candidates for the filtering.
// Adding the syskey to the order by clause will not degrade performance because
// the syskey is a part of the IUD log clustering index and a part of the range
// log clustering index. (The syskey column is the union of the iud log syskey and
// the range log rangeID column)
RelSequence *MultiTxnDEMavBuilder::buildSequenceOnScan(RelExpr *topNode, 
						       ItemExpr *isLastExpr) const
{
  RelSequence *sequenceNode =
    MvMultiTxnMavBuilder::buildSequenceOnScan(topNode, isLastExpr);

  // The ts column is the union of the iud log @TS and the range log 
  // rangeId column
  ItemExpr *tsCol = new(heap_) ColReference(new(heap_) ColRefName(COMMV_TS_COL));

  sequenceNode->addRequiredOrderTree(tsCol);

  return sequenceNode;
}  // MultiTxnDEMavBuilder::buildSequenceOnScan()

//----------------------------------------------------------------------------
// 1. Cover the range fields with LAST_NOT_NULL sequence function. This is done
//    for the use of the selection predicate that will filter IUD covered rows.
//    The new columns will have a name prefix of LNN.
//
// 2. Add a selection predicate for filtering the IUD rows that are covered by a 
//    range
//
// The root selection list is:
//
// 1. Star
// 2. LAST_NOT_NULL(@BR_) AS @LNN_@BR_
// 3. LAST_NOT_NULL(@ER_) AS @LNN_@ER_
// 4. LAST_NOT_NULL(@RANGE_ID) AS @LNN_@RANGE_ID
// 5. LAST_NOT_NULL(@RANGE_TYPE) AS @LNN_@RANGE_TYPE
RelRoot *MultiTxnDEMavBuilder::buildRootOverSequence(RelExpr *topNode, 
						     ItemExpr *isLastExpr) const
{
  const CorrName &tableNameCorr = getLogsInfo().getBaseTableName();
  
  RelRoot *root = new(heap_) RelRoot(topNode, REL_ROOT);

  ColRefName *starRef = new(heap_) ColRefName(1);  // isStar
  starRef->setStarWithSystemAddedCols();
  ItemExpr *star = new(heap_) ColReference(starRef);

  // The "begin range" vector of the range log.
  const NAString beginRangePrefix(COMMV_BEGINRANGE_PREFIX);  
  ItemExpr *rangeBeginCI = BinderUtils::buildClusteringIndexVector(
					       getLogsInfo().getBaseNaTable(), 
					       heap_, 
					       &tableNameCorr,
					       SP_USE_AT_SYSKEY | SP_USE_LAST_NOT_NULL,
					       NULL,						
					       &beginRangePrefix,
					       getLastNotNullPrefixColName());
  
  // The "end range" vector of the range log.
  const NAString endRangePrefix(COMMV_ENDRANGE_PREFIX);
  ItemExpr *rangeEndCI = BinderUtils::buildClusteringIndexVector(
					getLogsInfo().getBaseNaTable(), 
					heap_, 
					&tableNameCorr, 
					SP_USE_AT_SYSKEY | SP_USE_LAST_NOT_NULL,
					NULL,
					&endRangePrefix,
					getLastNotNullPrefixColName());

  ItemExpr *rangeIdCol = new(heap_) 
    RenameCol(new(heap_) 
      ItmSeqRunningFunction (ITM_LAST_NOT_NULL, new(heap_)
	ColReference(new(heap_)
	  ColRefName(COMMV_RANGE_ID_COL, getLogsInfo().getBaseTableName(), heap_))),
    new(heap_) ColRefName(*getLastNotNullPrefixColName() + COMMV_RANGE_ID_COL, getLogsInfo().getBaseTableName(), heap_));
  
  ItemExpr *rangeTypeCol = new(heap_) 
    RenameCol(new(heap_) 
      ItmSeqRunningFunction (ITM_LAST_NOT_NULL,  new(heap_)
	ColReference(new(heap_)
	  ColRefName(COMMV_RANGE_TYPE_COL, getLogsInfo().getBaseTableName(), heap_))),
    new(heap_) ColRefName(*getLastNotNullPrefixColName() + COMMV_RANGE_TYPE_COL, getLogsInfo().getBaseTableName(), heap_));


  root->addCompExprTree(star);
  root->addCompExprTree(rangeBeginCI);
  root->addCompExprTree(rangeEndCI);
  root->addCompExprTree(rangeIdCol);
  root->addCompExprTree(rangeTypeCol);

  root->setDontOpenNewScope();

  // Called the overriden function for creating the second root and add the filter to it.
  // isLastExpr is TRUE when for the last line that is processed in this phase.
  root = MvMultiTxnMavBuilder::buildRootOverSequence(root, isLastExpr);

  ItemExpr *selectionPred = buildSelectionOverSequence();

  root->addSelPredTree(selectionPred);

  root = buildFinalRootOverSequence(root);

  return root;
}  // MultiTxnDEMavBuilder::buildRootOverSequence()

//----------------------------------------------------------------------------
// This root will propogate only the base table columns and the isLast expression
// that are needed above the selection on the sequence. 
// It will also rename the "@SORT_KEY_XXXX" columns to the orginal XXXX names
RelRoot *MultiTxnDEMavBuilder::buildFinalRootOverSequence(RelExpr *topNode) const
{
  // Give the isLast expression the name @ISLAST
  ItemExpr *isLastColumn = new(heap_) 
    ColReference(new(heap_) ColRefName(getVirtualIsLastColName()));

  const NATable &baseTable = *getLogsInfo().getBaseNaTable();
  const NAColumnArray &baseColumns = baseTable.getNAColumnArray();
  
  ItemExprList colsList(heap_);

  colsList.insertTree(isLastColumn);

  for (CollIndex i=0; i<baseColumns.entries(); i++)
  {
    const NAString& colName = baseColumns[i]->getColName();
    
    NAString renameColName = colName;

    if (colName == "SYSKEY")
    {
      renameColName = COMMV_BASE_SYSKEY_COL;
    }
    
    ColRefName *colRefName = new(heap_)
      ColRefName(renameColName, getLogsInfo().getBaseTableName(), heap_);

    ItemExpr *colRef = new(heap_) ColReference(colRefName);

    if (baseColumns[i]->isClusteringKey())
    {
      colRef = new(heap_)
	RenameCol(new(heap_)
	  ColReference(new(heap_) 
	    ColRefName(*getSequenceByPrefixColName() + colName, getLogsInfo().getBaseTableName(), heap_)),
	  colRefName);
    }
    
    colsList.insertTree(colRef);
  }

  RelRoot *root = new(heap_) RelRoot(topNode, REL_ROOT, colsList.convertToItemExpr());

  root->setDontOpenNewScope();

  return root;
}

//----------------------------------------------------------------------------
// Build the selection predicate on top of the sequence node
//
// The selection predicate will filter the the rows that are covered by a 
// range.
//
// The base table rows are never covered by any range.
// An IUD row is covered by a range if it is physically covered by the range 
// and the the row.syskey is greater then the range.rangeId
//
// Each IUD row may only be covered by the last range record the was already 
// seen (explained above), this is way we can use the LAST_NOT_NULL 
// function is order to obtain the range values.
//
// Builds the following predicate :
//
//  a) (@SPECIAL = TABLE_ROWS)
//     OR
//  b) (@IS_LAST = TRUE)
//     OR 
//  c) (@SPECIAL = IUD_LOG_ROWS 
//     AND NOT
//  d) (isCoveredByRange)) isCoveredByRange when is explain down below
//
//
//----------------------------------------------------------------------------
ItemExpr *MultiTxnDEMavBuilder::buildSelectionOverSequence() const
{
  ItemExpr* isCoveredByRange = buildIsCoveredByRange();

  ItemExpr* isNotCoveredByRange = new(heap_)
    UnLogic(ITM_NOT, isCoveredByRange);

  ItemExpr *specialCol = new(heap_)
    ColReference(new(heap_)
      ColRefName(getVirtualRangeSpecialCol(), getLogsInfo().getBaseTableName(), heap_));

  // a)  
  ItemExpr *isTableRecords= new(heap_) 
    BiRelat(ITM_EQUAL, specialCol, new(heap_) SystemLiteral(TABLE_ROWS));

  // b)  
  ItemExpr *isLastCol = new(heap_)
    ColReference(new(heap_)
      ColRefName(getVirtualIsLastColName()));

  // c)
  ItemExpr *isIudLogRecords = new(heap_) 
    BiRelat(ITM_EQUAL, specialCol->copyTree(), new(heap_) SystemLiteral(IUD_LOG_ROWS));

  // d)
  ItemExpr *isNotCoveredIudRecords =  new(heap_)
    BiLogic(ITM_AND, isIudLogRecords, isNotCoveredByRange);
  
  ItemExpr *selection = new(heap_) 
    BiLogic(ITM_OR, isTableRecords, new(heap_)
      BiLogic(ITM_OR, isLastCol, isNotCoveredIudRecords));

  return selection;
}  // MultiTxnDEMavBuilder::buildSelectionOverSequence()

//----------------------------------------------------------------------------
// Builds the following predicate :
//
//    (BASE_TABLE.@RANGE_ID IS NOT NULL) 
//	AND 
//    (BASE_TABLE.@RANGE_ID < SYSKEY)
//	AND
//    isPhysicallyCoveredByRangeBoundries() that is explained below.
//----------------------------------------------------------------------------
ItemExpr *MultiTxnDEMavBuilder::buildIsCoveredByRange() const
{
  const CorrName &tableNameCorr = getLogsInfo().getBaseTableName();

  ItemExpr *rangeIdCol = new(heap_) 
      ColReference(new(heap_)
	ColRefName(*getLastNotNullPrefixColName() + COMMV_RANGE_ID_COL, getLogsInfo().getBaseTableName(), heap_));
  
  ItemExpr *logTsCol = new(heap_)
    ColReference(new(heap_)
      ColRefName(COMMV_TS_COL, getLogsInfo().getBaseTableName(), heap_));

  ItemExpr *isRangeHappendBefore = new(heap_) 
    BiLogic(ITM_AND, new(heap_) 
      UnLogic(ITM_IS_NOT_NULL,rangeIdCol), new(heap_)
      BiRelat(ITM_LESS, rangeIdCol->copyTree(), logTsCol));
      
  ItemExpr *isPhysicallyCoveredByRangeBoundries = 
    buildIsPhysicallyCoveredByRangeBoundries();

  ItemExpr *isCoveredByRange = new(heap_) 
    BiLogic(ITM_AND, isRangeHappendBefore, isPhysicallyCoveredByRangeBoundries);

  return isCoveredByRange;
}  // MultiTxnDEMavBuilder::buildIsCoveredByRange() 

//----------------------------------------------------------------------------
// Builds the following predicate :
//
//     BASE_TABLE.CI >= LAST_NOT_NULL(BASE_TABLE.BEGIN_CI) DIRECTEDBY <direction-vector>
//      AND 
//     BASE_TABLE.CI <= LAST_NOT_NULL(BASE_TABLE.END_CI)   DIRECTEDBY <direction-vector>
//      AND
//     CASE LAST_NOT_NULL(BASE_TABLE.RANGE_TYPE)
//      WHEN 3 TRUE
//      WHEN 2 BASE_TABLE.CI<> LAST_NOT_NULL(BASE_TABLE.BEGIN_CI)
//      WHEN 1 BASE_TABLE.CI<> LAST_NOT_NULL(BASE_TABLE.END_CI)
//      WHEN 0 BASE_TABLE.CI<> LAST_NOT_NULL(BASE_TABLE.BEGIN_CI) AND CI<> LAST_NOT_NULL(BASE_TABLE.END_CI)
//      END
//----------------------------------------------------------------------------
ItemExpr *MultiTxnDEMavBuilder::buildIsPhysicallyCoveredByRangeBoundries() const
{
  const CorrName &tableNameCorr = getLogsInfo().getBaseTableName();

  const NAString beginRangePrefix(*getLastNotNullPrefixColName() + COMMV_BEGINRANGE_PREFIX);  
  const NAString endRangePrefix(*getLastNotNullPrefixColName() + COMMV_ENDRANGE_PREFIX);

  IntegerList *directionVector = getLogsInfo().getBaseTableDirectionVector();

  // The clustering index vector of the base table.
  ItemExpr *baseCI = BinderUtils::buildClusteringIndexVector(   
					       getLogsInfo().getBaseNaTable(),
					       heap_,
					       &tableNameCorr,
					       SP_USE_AT_SYSKEY,
					       directionVector);

  

  // The "begin range" vector of the range log.
  ItemExpr *rangeBeginCI = BinderUtils::buildClusteringIndexVector(
					       getLogsInfo().getBaseNaTable(), 
					       heap_, 
					       &tableNameCorr,
					       SP_USE_AT_SYSKEY,
					       directionVector,						
					       &beginRangePrefix);
  
  

  // The "end range" vector of the range log.
  ItemExpr *rangeEndCI = BinderUtils::buildClusteringIndexVector(
					getLogsInfo().getBaseNaTable(), 
					heap_, 
					&tableNameCorr, 
					SP_USE_AT_SYSKEY,
					directionVector,
					&endRangePrefix);

  // The RANGE_TYPE column of the range log.
  ItemExpr *rangeType = new(heap_)
    ColReference(new(heap_) ColRefName(*getLastNotNullPrefixColName() + COMMV_RANGE_TYPE_COL) );

  ItemExpr *isPhysicallyCoveredByRangeBoundries =	
	      buildRangeLogJoinPredicateWithCols(rangeType, baseCI, rangeBeginCI, rangeEndCI);

  return isPhysicallyCoveredByRangeBoundries;
}  // MultiTxnDEMavBuilder::buildIsPhysicallyCoveredByRangeBoundries() 

//----------------------------------------------------------------------------
// Have a uniform select list over the IUD log.
// Columns added by parent class are: 
// 				    2. Base table columns
//				    3. @OP column 
//				    4. @BR columns = NULL
//		    		    5. @ER columns = NULL
//				    6. Range ID = NULL
// Columns added by this class are:
//				    1. Sortkey Columns = CI columns
//				    6. Row Type = NULL
//				    7. Range Type = NULL
//				    8. IUD @TS column
// We add the columns in order to construct the union between the range 
// and the IUD read blocks
//----------------------------------------------------------------------------
RelRoot *MultiTxnDEMavBuilder::buildRootOverIUDLog(RelExpr *topNode) const
{
  RelRoot *root = MvMultiTxnMavBuilder::buildRootOverIUDLog(topNode);

  // MvMultiTxnMavBuilder::buildRootOverIUDLog() creates a select list with the 
  // base CI columns .
  // Since the sort key columns must come before the base CI columns, we need
  // to insert the sort key columns as the first columns in the select list.
  ItemExpr *sortKeyColumns =   
    BinderUtils::buildClusteringIndexVector(getLogsInfo().getBaseNaTable(),
					    heap_,
					    &getLogsInfo().getIudLogTableName(),
					    SP_USE_AT_SYSKEY ,
					    NULL,
					    NULL,
					    getSequenceByPrefixColName());

  ItemExpr *selection = root->removeCompExprTree();

  selection = new(heap_) ItemList(sortKeyColumns, selection);

  root->addCompExprTree(selection);

  ItemExpr *specialCol = new(heap_) 
    RenameCol(new(heap_) SystemLiteral(IUD_LOG_ROWS), 
      new(heap_) ColRefName(getVirtualRangeSpecialCol()));

  ItemExpr *rangeTypeCol = new(heap_) 
    RenameCol(new(heap_) SystemLiteral(), new(heap_) ColRefName(COMMV_RANGE_TYPE_COL));

  // the @TS column of the IUD log
  ItemExpr * tsCol =  new(heap_)                          
                         ColReference(new(heap_) ColRefName(COMMV_TS_COL,heap_));

  root->addCompExprTree(specialCol);
  root->addCompExprTree(rangeTypeCol);
  root->addCompExprTree(tsCol);

  // The IUD log syskey is add automatically 
  return root;
}  // MultiTxnDEMavBuilder::buildRootOverIUDLog()

//----------------------------------------------------------------------------
// This function has two functionalities:
// 1. filter empty ranges - 
//    We need to filter the rows with @SPECIAL = TABLE_ROWS and CI = NULL.
//    These rows represent ranges that select no data from the base table.
//    This may be possible if after a range insert a massive delete was 
//    executed on the same range of clustering index values. Of course we 
//    still need the rows with @SPECIAL = VIRTUAL_BEGIN_RANGE for the 
//    duplicate elimination algorithm.
// 
// 2. Have a uniform select list over the IUD log. 
//    ( The number is the order of the columns in the select list)
//
//    Columns added by parent class are: 
//      2. CI columns
//      3. @OP column = 1
//      4. @BR columns
//      5  @ER columns
//    Columns added by this class are:
//      1. Sortkey Columns
//      6. Row Type (is this a virtual range or a table row)
//      7. Range Type (Are the range boundaries closed ?) 
//      8. Range ID
//      9. Range ID as syskey	
//----------------------------------------------------------------------------
RelRoot *MultiTxnDEMavBuilder::buildRootOverRangeBlock(RelExpr *topNode) const
{
  // Build a root for filtering rows with @SPECIAL = TABLE_ROWS and CI = NULL.
  // These ranges select no data from the base table.
  // This may be possible if after a range insert a massive delete was 
  // executed on the same range of clustering index.
  topNode = buildRootWithFilterForEmptyRanges(topNode);

  RelRoot *root = MvMultiTxnMavBuilder::buildRootOverRangeBlock(topNode);

  // MvMultiTxnMavBuilder::buildRootOverIUDLog() creates a select list with the 
  // base CI columns .
  // Since the sort key columns must come before the base CI columns, we need
  // to insert the sort key columns as the first columns in the select list.
  // build the necessary select list from the base table
  ItemExprList sortKeyCols(heap_);

  buildSortKeyColumnsForRangeBlock(sortKeyCols);

  ItemExpr *selection = root->removeCompExprTree();

  selection = new(heap_) ItemList(sortKeyCols.convertToItemExpr(), selection);

  root->addCompExprTree(selection);

  ItemExpr *rangeTypeCol = new(heap_) 
    ColReference(new(heap_) 
      ColRefName(COMMV_RANGE_TYPE_COL, getLogsInfo().getRangeLogTableName(), heap_));

  ItemExpr *rangeIdCol = new(heap_) 
    ColReference(new(heap_) 
      ColRefName(COMMV_RANGE_ID_COL, getLogsInfo().getRangeLogTableName(), heap_));

  ItemExpr *specialCol = new(heap_) 
    ColReference(new(heap_) 
      ColRefName(getVirtualRangeSpecialCol()));

  // rangeId is the time of the logging of the range record    
  ItemExpr *rangeSyskeyOfLogging = new(heap_) 
    RenameCol(rangeIdCol->copyTree(), new(heap_) 
      ColRefName(COMMV_RANGE_SYSKEY_COL, getLogsInfo().getRangeLogTableName()));      

  root->addCompExprTree(specialCol);
  root->addCompExprTree(rangeTypeCol);
  root->addCompExprTree(rangeSyskeyOfLogging);

  return root;
}  //  MultiTxnDEMavBuilder::buildRootOverRangeBlock()

//----------------------------------------------------------------------------
// For every CI column with his equivalent @BR column produce the 
//    following expression:
//	CASE(NULL, IF col IS NULL THEN @br ELSE col)
void MultiTxnDEMavBuilder::buildSortKeyColumnsForRangeBlock(ItemExprList &sortKeyCols) const
{
  const NATable &baseTable = *getLogsInfo().getBaseNaTable();
  const NAColumnArray &Columns = baseTable.getClusteringIndex()->getIndexKeyColumns();

  for (CollIndex i=0; i< Columns.entries(); i++)
  {
    NAString colName = Columns[i]->getColName();

    if ("SYSKEY" ==  Columns[0]->getColName())
    {
      addSyskeyToSortKeyColumnsForRangeBlock(sortKeyCols);
      continue;
    }

    ItemExpr *colExpr = new(heap_) 
      ColReference(new(heap_) 
	ColRefName(colName, getLogsInfo().getBaseTableName(), heap_));
    
    NAString rangeBeginColName = 
      COMMV_BEGINRANGE_PREFIX + Columns[i]->getColName();

    ItemExpr *rangeBeginColRef = 
      new(heap_) ColReference(
	new(heap_) ColRefName(rangeBeginColName, 
			      getLogsInfo().getRangeLogTableName(), 
			      heap_));

    // Generate the computed CI cols 
    ItemExpr *computedColExpr = new(heap_) 
      RenameCol(new(heap_) 
	Case(NULL, new(heap_)
	  IfThenElse(new(heap_) 
	    UnLogic(ITM_IS_NULL, 
		    colExpr->copyTree()), 
		    rangeBeginColRef->copyTree(), 
		    colExpr->copyTree())), new(heap_) 
	ColRefName(*getSequenceByPrefixColName() + colName, getLogsInfo().getBaseTableName(), heap_));

    sortKeyCols.insertTree(computedColExpr);

  }
}  // MultiTxnDEMavBuilder::buildSortKeyColumnsForRangeBlock()

//----------------------------------------------------------------------------
// This function hold the same functionality as buildSortKeyColumnsForRangeBlock()
// but only deals with the SYSKEY column 
void MultiTxnDEMavBuilder::addSyskeyToSortKeyColumnsForRangeBlock(ItemExprList &sortKeyCols) const
{
  ItemExpr *colExpr = new(heap_) 
    ColReference(new(heap_) 
      ColRefName(COMMV_BASE_SYSKEY_COL, getLogsInfo().getBaseTableName(), heap_));

  ItemExpr *rangeBeginColRef = new(heap_) 
    ColReference(new(heap_) 
      ColRefName("@BR_SYSKEY", getLogsInfo().getRangeLogTableName(), heap_));

  ItemExpr *caseExpr = new(heap_) 
      Case(NULL, new(heap_)
	IfThenElse(new(heap_) 
	  UnLogic(ITM_IS_NULL,colExpr), rangeBeginColRef, colExpr));

  ItemExpr *newColExpr = new(heap_) 
    RenameCol(caseExpr, new(heap_) 
      ColRefName(*getSequenceByPrefixColName() + COMMV_BASE_SYSKEY_COL, 
                 getLogsInfo().getBaseTableName(), 
		 heap_));

  sortKeyCols.insertTree(newColExpr);
}  // MultiTxnDEMavBuilder::addSyskeyToSortKeyColumnsForRangeBlock()

//----------------------------------------------------------------------------
// Filter empty ranges by the following predicate
// 
// (@SPECIAL = VIRTUAL_BEGIN_RANGE) OR (CI[0] IS NOT NULL)
//
RelRoot *MultiTxnDEMavBuilder::buildRootWithFilterForEmptyRanges(RelExpr *topNode) const
{
  NAString firstCIColName = 
    getLogsInfo().getBaseNaTable()->getClusteringIndex()->getIndexKeyColumns()[0]->getColName();

  if (firstCIColName == "SYSKEY")
  {
    firstCIColName = COMMV_BASE_SYSKEY_COL;
  }

  // The projection name should be different from the actual name of the 
  // first CI column, so add a '@F' (used to be 'F@') prefix to it.
  // Names with the @ prefix are reserved for internal use.  Please read
  // the descriptions in w:/sqlshare/ReservedInternalNames.cpp for more
  // information.
  NAString firstCIColNameForProjection(COMMV_CTRL_PREFIX "F");
  firstCIColNameForProjection += firstCIColName;

  ItemExpr *firstClusteringBaseCol = new(heap_) 
    RenameCol(new(heap_) 
      ColReference(new(heap_) 
	ColRefName(firstCIColName, getLogsInfo().getBaseTableName(), heap_)), new(heap_)
	ColRefName(firstCIColNameForProjection, getLogsInfo().getRangeLogTableName(), heap_));

  ItemExpr *specialColRef = new(heap_) 
    ColReference(new(heap_) ColRefName(getVirtualRangeSpecialCol()));

  // The root select list is (*, isLastExpr).
  ColRefName *star = new(heap_) ColRefName(1);  // isStar
  star->setStarWithSystemAddedCols();
  
  ItemExpr *rootSelectList = new(heap_) 
    ItemList(new(heap_) ColReference(star), firstClusteringBaseCol);

  RelRoot *root = new(heap_) RelRoot(topNode, REL_ROOT, rootSelectList);
  
  ItemExpr *selection = new(heap_) 
    BiLogic(ITM_OR, new(heap_)
      BiRelat(ITM_EQUAL, 
	specialColRef->copyTree(),
	new(heap_) SystemLiteral(VIRTUAL_BEGIN_RANGE)), new(heap_)
      UnLogic(ITM_IS_NOT_NULL,firstClusteringBaseCol));      

  root->addSelPredTree(selection);

  root->setDontOpenNewScope(); 

  return root;
}  // MultiTxnDEMavBuilder::buildRootWithFilterForEmptyRanges()

//----------------------------------------------------------------------------
// The following methods contruct the following tree by refining 
// MvMultiTxnMavBuilder methods.
//
//			    Left Join
//	                   /         \
//                      Root         Scan
//                       |	 (Base table)
//	             Transpose        
//	                 |           
//		        Scan  
//	            (Range Log) 
//
// This sub tree produces n + 1 rows for a range that selects n rows from 
// the base table. The extra row is called a virtual range row. For it the 
// base columns clustering index vector is equal to the BR vector of that 
// range. This is done in order to place the virtual range row in the begining 
// of the range CI (obtained by sorting the rows by CI).
// This way we can later define if an IUD row is covered by a range or not
RelExpr *MultiTxnDEMavBuilder::buildJoinBaseTableWithRangeLog(RelExpr *scanRangeLog, 
							      RelExpr *scanBaseTable) const
{
  // We select the rows from the table with the same predicate as in multi-txn
  ItemExpr *joinPredicate = buildRangeLogJoinPredicate();

  // In our tree the range log produces two rows for every range (obtained by 
  // the Transpose node) therefor we need to avoid sending both of the rows 
  // to the right child. By adding @SPECIAL = TABLE_ROWS to the join prdicate 
  // we accomplish exactly that.
  ItemExpr *specialColRef = new(heap_) 
    ColReference(new(heap_) 
      ColRefName(getVirtualRangeSpecialCol()));

  ItemExpr *forTableRowsOnly = new(heap_)
    BiRelat(ITM_EQUAL, 
      specialColRef, new(heap_) 
        SystemLiteral(TABLE_ROWS));

  joinPredicate = new(heap_) 
    BiRelat(ITM_AND, 
	    joinPredicate,
	    forTableRowsOnly);

  Join *joinNode = new(heap_) 
    Join(scanRangeLog, scanBaseTable, REL_LEFT_JOIN, joinPredicate);

  // We force nested join to obtain order by BR,RangeId,CI
  joinNode->forcePhysicalJoinType(Join::NESTED_JOIN_TYPE);

  return joinNode;
}  // MultiTxnDEMavBuilder::buildJoinBaseTableWithRangeLog()

//----------------------------------------------------------------------------
// Build a Transpose node on top of the scan 
//
// This sub tree is equivalent to the follwing query :
// SELECT * FROM rangeLog, TRANSPOSE (VIRTUAL_BEGIN_RANGE),(TABLE_ROWS) as @SPECIAL 
//
// The query will produce two rows for each range - one with a value of TABLE_ROWS
// which will later on be used to select the range from the table and the other 
// VIRTUAL_BEGIN_RANGE that will be used to mark the begining of a new range.
RelExpr *MultiTxnDEMavBuilder::buildReadRangeLogBlock() const
{
  RelExpr *readRangesSubTree = MvMultiTxnMavBuilder::buildReadRangeLogBlock();

  ItemExpr *tupleRow1 = new(heap_) Convert(new(heap_) SystemLiteral(VIRTUAL_BEGIN_RANGE));
  ItemExpr *tupleRow2 = new(heap_) Convert(new(heap_) SystemLiteral(TABLE_ROWS));
  
  // Construct the values expression 
  // The VIRTUAL_BEGIN_RANGE must be before the TABLE_ROWS in order to preserve the
  // join order
  ItemExpr *valuesExpr = new(heap_) ItemList(tupleRow1, tupleRow2);

  ColReference *specialColRef = new(heap_) 
    ColReference(new(heap_) 
    ColRefName(getVirtualRangeSpecialCol(), heap_));

  ItemExpr *tranposeValus = new(heap_) ItemList(valuesExpr, specialColRef);

  // Transpose values must end with Null
  tranposeValus = new(heap_) ItemList(tranposeValus, NULL);

  RelExpr* topNode = new(heap_) Transpose(tranposeValus, NULL, readRangesSubTree);

  // The "end range" vector of the range log.
  ItemExpr *endRangeCI = buildEndRangeVector();

  ItemExpr *rangeIdCol = new(heap_)
    ColReference(new(heap_)
      ColRefName(COMMV_RANGE_ID_COL, getLogsInfo().getRangeLogTableName(), heap_));

  ItemExprList uniqueCols(endRangeCI, heap_);
  uniqueCols.insertTree(rangeIdCol);

  // Add a unique constraint to the Transpose node on @ER,@RANGE_ID columns
  // This constraint is correct because of the resolution stage of the DE alg in the 
  // refresh utility (No overlapping ranges).
  // We must add this optimizer contraint in order to allow the optimization of nested-join's
  // sort order.
  topNode->addUniqueColumnsTree(uniqueCols.convertToItemExpr());

  return topNode;
}  // MultiTxnDEMavBuilder::buildReadRangeLogBlock()

