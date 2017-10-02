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
* File:         MvRefreshBuilder.cpp
* Description:  Class hierarchy of MvRefreshBuilder 
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
#include "parser.h"
#include "MVInfo.h"
#include "MVJoinGraph.h"
#include "Refresh.h"
#include "BinderUtils.h"
#include "ChangesTable.h"
#include "MvRefreshBuilder.h"
#include "MavRelRootBuilder.h"

// ===========================================================================
// ===========================================================================
// ===================  class  LogsInfo  ==============
// ===========================================================================
// ===========================================================================

LogsInfo::LogsInfo(DeltaDefinition &deltaDef,
		  MVInfoForDML    *mvInfo,
		  BindWA	  *bindWA)
  : deltaDef_(deltaDef),
    mvInfo_(mvInfo),
    bindWA_(bindWA),
    currentMvIudlog_(NULL),
    rangeTableName_(NULL),
    rangeNaTable_(NULL),
    baseTableDirectionVector_(NULL)
{
  CorrName *tableName = new(bindWA->wHeap()) 
    CorrName(*deltaDef_.getTableName());

  currentMvIudlog_ = new(bindWA->wHeap()) 
    MvIudLog(*tableName, bindWA, REL_SCAN);
  
  if (bindWA->errStatus())
    return;

  currentMvIudlog_->setDeltaDefinition(&deltaDef);
  
  currentMvIudlog_->setMvInfo(mvInfo_);

  baseTableDirectionVector_ = 
    BinderUtils::buildClusteringIndexDirectionVector(getBaseNaTable(), bindWA->wHeap());

  if (deltaDef.useRangeLog())
  {
    rangeTableName_ = calcRangeLogName(getBaseTableName(), bindWA->wHeap());
    rangeNaTable_ = bindWA->getNATable(*rangeTableName_);
  }
}

LogsInfo::~LogsInfo()
{
  delete currentMvIudlog_;
}

//////////////////////////////////////////////////////////////////////////////
// Create a CorrName to the range log from the subject table name.
//////////////////////////////////////////////////////////////////////////////
CorrName *LogsInfo::calcRangeLogName(const CorrName &theTable, 
				     CollHeap *heap) const
{
  return 
    ChangesTable::buildChangesTableCorrName(theTable.getQualifiedNameObj(), 
					    COMMV_RANGE_LOG_SUFFIX,
					    ExtendedQualName::RANGE_LOG_TABLE,
					    heap);
}


// ===========================================================================
// ===========================================================================
// ===================  class  MvRefreshBuilder  =============================
// ===========================================================================
// ===========================================================================

// Full Ctor for all other sub-classes.
MvRefreshBuilder::MvRefreshBuilder(const CorrName&  mvName,
                                   MVInfoForDML    *mvInfo,
                                   Refresh	   *refreshNode,
				   BindWA          *bindWA)
  : mvCorrName_(mvName, bindWA->wHeap()),
    deltaDefList_(refreshNode ? refreshNode->getDeltaDefList() : NULL),
    phase_       (refreshNode ? refreshNode->getPhase()        : NULL),
    refreshNode_(refreshNode),
    heap_(bindWA->wHeap()),
    bindWA_(bindWA),
    mvInfo_(mvInfo),
    logsInfo_(NULL)
{
}

//----------------------------------------------------------------------------
MvRefreshBuilder::~MvRefreshBuilder() 
{
  delete mvInfo_;
  delete logsInfo_;
}

// ===========================================================================
// ==== Methods for building the bottom part of the refresh tree - called
// ==== by buildScanLogBlock(), replacing the Scan of the base table with
// ==== a Scan on the logs).
// ===========================================================================

//----------------------------------------------------------------------------
// Build the sub tree for reading the IUD log records
RelExpr *MvRefreshBuilder::buildReadIudLogBlock() 
{
  MvIudLog &iudLog = getLogsInfo().getMvIudlog();
  Scan *scanNode = 
    iudLog.buildScan(ChangesTable::ALL_ROWS);

  // Fix the cardinality of the IUD log.
  // This call will set the cardinality of this scan to 
  // the IUD log cardinality * (1/getNumOfScansInUnionBackbone())
  // The IUD log cardinality available in the optimizer phase
  fixScanCardinality(scanNode, 1 / getNumOfScansInUnionBackbone(), 0);

  ItemExpr *selectionPred = buildSelectionPredicateForScanOnIudLog();

  if (selectionPred)
  {
    scanNode->addSelPredTree(selectionPred);
  }

  RelExpr *topNode = scanNode;

  ItemExpr *rootSelectList = buildSelectionListForScanOnIudLog();

  topNode = new(heap_) RelRoot(topNode, REL_ROOT, rootSelectList);

  // If the table has a correlation name, than we just leave it as is.
  // Otherwise, we need a RenameTable node on top.
  if (scanNode->getTableName().getCorrNameAsString() == "")
  {
    topNode = new(heap_) 
      RenameTable(TRUE, topNode, getLogsInfo().getIudLogTableName());
  }

  if (useUnionBakeboneToMergeEpochs())
  {
    // Have the rows ordered by CI, not by @EPOCH, CI.
    topNode = buildUnionBakeboneToMergeEpochs(topNode);
  }

  return topNode;
}

//----------------------------------------------------------------------------
ItemExpr *MvRefreshBuilder::buildSelectionListForScanOnIudLog() const
{
  // Build @Op, according to the row type (inserted or deleted).
  ItemExpr *opExpr = constructOpExpression();
  
  //
  // Attempt to select the columns from the IUD log in the same order as
  // they are used in the base table.  This helps ensure that both sides of the
  // Join have the same column ordering.
  //
  // 1) Build select list as all CTRL cols from the IUD log 
  //     
  const NAColumnArray &logCols = getLogsInfo().getIudLogNaTable()->getNAColumnArray();
  CollIndex numberOfColumns = logCols.entries();
  ItemExpr *ctrlColList = NULL;
  for (CollIndex i=0; i<numberOfColumns; i++)
  {
    const NAColumn *naCol = logCols[i];
    ColRefName *colName = new(heap_) ColRefName(naCol->getColName());

    // just add the control cols from the IUD log
    if (!strncmp(colName->getColName().data(), 
                  COMMV_CTRL_PREFIX , 
                  sizeof(COMMV_CTRL_PREFIX)-1))
    {      
      ItemExpr *colRef = new(heap_)ColReference(colName);

      if( NULL==ctrlColList )
      {
        ctrlColList = colRef;
      }
      else
      {
        ctrlColList = new(heap_) ItemList(ctrlColList,colRef);
      }
    }
  }

  //
  // 2) Add to the list all of the columns in the base table that 
  // are not mvused cols  
  ItemExpr *baseColList = buildBaseTableColumnList();

  // the column list will be the control columns and the base columns
  // used by the MV
  ItemExpr *colList = new(heap_) ItemList(ctrlColList,baseColList);

  // The root select list is 
  // (IUD log ctrl cols, base table cols used by the MV, <opExpr> AS @OP).
  ItemExpr *rootSelectList = new(heap_) ItemList(colList, opExpr);

  return rootSelectList;
}  // of MvRefreshBuilder::buildSelectionListForScanOnIudLog()
//----------------------------------------------------------------------------
// Method: MvRefreshBuilder::buildBaseTableColumnList
//
// Description:
//    Build the list of columns from the base table which are used by
//    this MV
//
ItemExpr *MvRefreshBuilder::buildBaseTableColumnList(Lng32 specialFlags) const
{
  const NATable *baseTable = getLogsInfo().getBaseNaTable();
  const NAColumnArray &baseCols = baseTable->getNAColumnArray();
 
  // find the used object to determine whether this column is used by the MV
  const QualifiedName& tableName = 
        getLogsInfo().getBaseTableName().getQualifiedNameObj();

  const MVUsedObjectInfo *usedObject = 
        getMvInfo()->findUsedInfoForTable(tableName);
  CMPASSERT(usedObject != NULL);  

  const NAColumnArray &baseCiCols = 
        baseTable->getClusteringIndex()->getIndexKeyColumns();

  CollIndex numberOfColumns = baseCols.entries();
  ItemExpr *baseColList = NULL;
  for (CollIndex i=0; i<numberOfColumns; i++)
  {
    const NAColumn *naCol = baseCols[i];
    ColRefName *colName = new(heap_) ColRefName(naCol->getColName());
    ItemExpr *colRef = NULL;
      
    if( "SYSKEY" == colName->getColName() && 
        (specialFlags & SP_USE_AT_SYSKEY) )
    {
      colRef = new(heap_)ColReference(
                  new(heap_) ColRefName(COMMV_BASE_SYSKEY_COL));
    }
    else if ("SYSKEY" != colName->getColName())
    {
      // If the column is used by the MV, add it to the list
      if (usedObject->isUsedColumn(i) ||
          baseCiCols.contains((NAColumn*)naCol) )
      {      
        colRef = new(heap_)ColReference(colName);        
      }
    }

    if( NULL==baseColList )
    {
      baseColList = colRef;
    }
    else
    {
      baseColList = new(heap_) ItemList(baseColList,colRef);
    }
  }
  return baseColList;
}

//////////////////////////////////////////////////////////////////////////////
// This method is overriden in MvMultiTxnMavBuilder
ItemExpr *MvRefreshBuilder::buildSelectionPredicateForScanOnIudLog() const
{
  if (!useUnionBakeboneToMergeEpochs())
  {
    // The Range log predicate on the epoch is the same as for the IUD log.
    return MvIudLog::buildLogEpochPredicate(&getLogsInfo().getDeltaDefinition(), 
					    heap_);
  }
  return NULL;
}

//----------------------------------------------------------------------------
// This is the main method used for replacing the Scan on the base table
// by a sub-tree reading the logs (IUD log and the range log ).It is possible
// that a read from the IUD log or the range log is not needed
//
//
//	      RenameTable
//	           |
//	        RelRoot
//		   | 
//  	       MergeUnion
//	      /          \
// Read IUD records    Read Ranges      
//  (Sub Tree)		(Sub Tree)
//
//
RelExpr *MvRefreshBuilder::buildLogsScanningBlock(const QualifiedName& baseTable)
{
  DeltaDefinition *deltaDef = getRefreshNode()->getDeltaDefinitionFor(baseTable);
  CMPASSERT(deltaDef != NULL);

  // (re)Build the LogsInfo for the current table logs
  // This object is also used for storing information on the current delta
  // therefor we need to (re)build it for each delta reading block
  delete logsInfo_;
  logsInfo_ = new LogsInfo(*deltaDef, getMvInfo(), bindWA_);
  if (bindWA_->errStatus() || logsInfo_ == NULL)
    return NULL;

  RelExpr *topNode = NULL;
  RelExpr *scanIUDLogBlock = NULL;
  RelExpr *scanRangeLogBlock = NULL;

  if (deltaDef->useIudLog())
  {
    scanIUDLogBlock = buildReadIudLogBlock();
  }

  // If we have data in the range log - read it as well.
  if (deltaDef->useRangeLog())
  {
    scanRangeLogBlock = buildReadRangesBlock();
  }
  // If both logs are used we need to union their results
  if (deltaDef->useIudLog() && deltaDef->useRangeLog())
  {
    CMPASSERT(scanIUDLogBlock != NULL && scanRangeLogBlock != NULL);

    // Add a uniform select list to prepare for a Union with the range log.
    scanIUDLogBlock = buildRootOverIUDLog(scanIUDLogBlock);

    // Now build the Union between the RangeLogBlock and scanIUDLogBlock.
    topNode = buildUnionBetweenRangeAndIudBlocks(scanIUDLogBlock, scanRangeLogBlock);
    topNode = new(heap_) RelRoot(topNode); // Open another scope.
  }
  else
  {
    if (deltaDef->useIudLog())
    {
      topNode = scanIUDLogBlock;
    }
    else
    {
      topNode = scanRangeLogBlock;
    }
  }

  // Give it all the name of the base table.
  topNode = new(heap_) RenameTable(TRUE, 
				   topNode, 
				   getLogsInfo().getBaseTableName());

  CMPASSERT(topNode);

  return topNode;
}  // of MvRefreshBuilder::buildLogsScanningBlock()

//----------------------------------------------------------------------------
// Excluded from coverage test - used only with range logging.
Union *MvRefreshBuilder::buildUnionBetweenRangeAndIudBlocks(RelExpr *scanIUDLogBlock,
							    RelExpr *scanRangeLogBlock) const
{
  Union *unionNode = new(heap_) Union(scanIUDLogBlock, scanRangeLogBlock,
                                      NULL, NULL, REL_UNION,
                                      CmpCommon::statementHeap(), TRUE);

  if (needAlternateCIorder())
  {
    const CorrName tableNameCorr("");
    ItemExpr *baseCI = BinderUtils::buildClusteringIndexVector(
			     getLogsInfo().getBaseNaTable(), 
			     heap_, 
			     &tableNameCorr, 
			     SP_USE_AT_SYSKEY, 
			     getLogsInfo().getBaseTableDirectionVector());

    ItemExpr *alternateOrder = buildAlternateCIorder(baseCI, tableNameCorr);
  
    unionNode->addAlternateRightChildOrderExprTree(alternateOrder);
  }

  return unionNode;
}

//----------------------------------------------------------------------------
// Excluded from coverage test - used only with range logging.
NABoolean MvRefreshBuilder::needAlternateCIorder() const
{
  return isGroupByAprefixOfTableCKeyColumns();
}

//----------------------------------------------------------------------------
// Excluded from coverage test - used only with range logging.
ItemExpr *MvRefreshBuilder::buildAlternateCIorder(ItemExpr *ciColumns, const CorrName &tableNameCorr) const
{
  const NAString endRangePrefix(COMMV_ENDRANGE_PREFIX);

  // The "end range" vector of the range log.
  ItemExpr *rangeEndCI = BinderUtils::buildClusteringIndexVector(
					getLogsInfo().getBaseNaTable(), 
					heap_, 
					&tableNameCorr, 
					SP_USE_AT_SYSKEY,
					getLogsInfo().getBaseTableDirectionVector(),
					&endRangePrefix);

  ItemExpr *rangeIdCol = new(heap_)
    ColReference(new(heap_)
      ColRefName(COMMV_RANGE_ID_COL, tableNameCorr, heap_));

  ItemExprList alternateOrderVector(rangeEndCI, heap_);
  alternateOrderVector.insertTree(rangeIdCol);
  alternateOrderVector.insertTree(ciColumns);

  return alternateOrderVector.convertToItemExpr();
}

//////////////////////////////////////////////////////////////////////////////
// Construct the expression for @Op, according to the value of the ROW_TYPE
// column from the log:
// CASE 
//   WHEN (ROW_TYPE = 0) OR (ROW_TYPE = 2) THEN 1
//   ELSE                                      -1
// END
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvRefreshBuilder::constructOpExpression() const
{
  ItemExpr *valueOneCond = new (heap_) 
    BiLogic(ITM_OR, 
	    BinderUtils::buildPredOnCol(ITM_EQUAL, // (ROW_TYPE = 0)
		                        COMMV_OPTYPE_COL, 
					ComMvRowType_Insert, 
					heap_),
	    BinderUtils::buildPredOnCol(ITM_EQUAL, // (ROW_TYPE = 2)
		                        COMMV_OPTYPE_COL,
					ComMvRowType_InsertOfUpdate, 
					heap_) );

  // If (valueOneCond) then 1 else -1
  ItemExpr *opExpr = new(heap_) Case(NULL, new(heap_) 
    IfThenElse(valueOneCond, 
  	       new(heap_) SystemLiteral( 1),
	       new(heap_) SystemLiteral(-1)));

  RenameCol *opColumn = 
    new(heap_) RenameCol(
		opExpr, 
		new(heap_) ColRefName(MavBuilder::getVirtualOpColumnName(), 
				      getLogsInfo().getBaseTableName()));

  return opColumn;
}

//----------------------------------------------------------------------------
// Build a vector of clustering index columns with @BR_ prefix
// Excluded from coverage test - used only with range logging.
ItemExpr *MvRefreshBuilder::buildBeginRangeVector() const
{
  const NAString beginRangePrefix(COMMV_BEGINRANGE_PREFIX);

  return BinderUtils::buildClusteringIndexVector(
					       getLogsInfo().getBaseNaTable(), 
					       heap_, 
					       &getLogsInfo().getRangeLogTableName(),
					       SP_USE_AT_SYSKEY,
					       getLogsInfo().getBaseTableDirectionVector(),						
					       &beginRangePrefix);
}

//----------------------------------------------------------------------------
// Build a vector of clustering index columns with @ER_ prefix
// Excluded from coverage test - used only with range logging.
ItemExpr *MvRefreshBuilder::buildEndRangeVector() const
{
  const NAString endRangePrefix(COMMV_ENDRANGE_PREFIX);

  return BinderUtils::buildClusteringIndexVector(
					getLogsInfo().getBaseNaTable(), 
					heap_, 
					&getLogsInfo().getRangeLogTableName(), 
					SP_USE_AT_SYSKEY,
					getLogsInfo().getBaseTableDirectionVector(),
					&endRangePrefix);
}

//----------------------------------------------------------------------------
// The Join predicate between the Range log and the base table on the 
// Clustering index is (CI stands for the clustering index vector):
// Effectivly, this predicate implements the four possible range types
// (closed or open from both sides). The direction vector gives the 
// direction (ascending/descending) of each column in the clustering index.
// For multi transactional refresh, this method is overridden and two 
// additional predicates are added.
// Excluded from coverage test - used only with range logging.
ItemExpr *MvRefreshBuilder::buildRangeLogJoinPredicate() const
{
  

  ItemExpr *baseCI =   // The clustering index vector of the base table.
    BinderUtils::buildClusteringIndexVector(getLogsInfo().getBaseNaTable(),
					       heap_,
					       &getLogsInfo().getBaseTableName(),
					       SP_USE_AT_SYSKEY);

  // The "begin range" vector of the range log.
  ItemExpr *beginCI = buildBeginRangeVector();

  // The "end range" vector of the range log.
  ItemExpr *endCI = buildEndRangeVector();

  // The RANGE_TYPE column of the range log.
  ItemExpr *rangeType = new(heap_)
    ColReference(new(heap_) ColRefName(COMMV_RANGE_TYPE_COL) );

  return buildRangeLogJoinPredicateWithCols(rangeType, baseCI, beginCI, endCI);
}

//----------------------------------------------------------------------------
//   a) BASE_TABLE.CI >= RANGE_LOG.BEGIN_CI DIRECTEDBY <direction-vector>
//      AND 
//   b) BASE_TABLE.CI <= RANGE_LOG.END_CI   DIRECTEDBY <direction-vector>
//      AND
//   c) CASE RANGE_LOG.RANGE_TYPE
//   c3)  WHEN 3 TRUE
//   c2)  WHEN 2 CI<> RANGE_LOG.BEGIN_CI
//   c1)  WHEN 1 CI<> RANGE_LOG.END_CI
//   c0)  WHEN 0 CI<> RANGE_LOG.BEGIN_CI AND CI<> RANGE_LOG.END_CI
//      END
// Excluded from coverage test - used only with range logging.
ItemExpr *MvRefreshBuilder::buildRangeLogJoinPredicateWithCols(
						       ItemExpr *rangeType,
						       ItemExpr *baseCI,
						       ItemExpr *beginRangeCI,
						       ItemExpr *endRangeCI) const
{
  IntegerList *directionVector = getLogsInfo().getBaseTableDirectionVector();
			
  // a)
  BiRelat *predA = new(heap_) BiRelat(ITM_GREATER_EQ, 
                                      baseCI, 
				      beginRangeCI);

  predA->setDirectionVector(directionVector);

  // b)
  BiRelat *predB = new(heap_) BiRelat(ITM_LESS_EQ, 
                                      baseCI->copyTree(heap_), 
				      endRangeCI);
  
  predB->setDirectionVector(directionVector);

  // c)
  ItemExpr *predC3 = new(heap_) BoolVal(ITM_RETURN_TRUE);

  ItemExpr *predC2 = new(heap_) BiRelat(ITM_NOT_EQUAL, 
					baseCI->copyTree(heap_),
					beginRangeCI->copyTree(heap_) );

  ItemExpr *predC1 = new(heap_) BiRelat(ITM_NOT_EQUAL, 
					baseCI->copyTree(heap_),
					endRangeCI->copyTree(heap_) );

  ItemExpr *predC0 = new(heap_) BiLogic(ITM_AND,
					predC1->copyTree(heap_),
					predC2->copyTree(heap_) );

  // Now combine the four parts of condiion c) together:
  ItemExpr *predC  = new(heap_) 
    Case(rangeType, new(heap_) 
      IfThenElse(new(heap_) SystemLiteral(ComMvRangeClosedBothBounds), // If 0
	         predC0, new(heap_)
      IfThenElse(new(heap_) SystemLiteral(ComMvRangeClosedLowerBound), // If 1
	         predC1, new(heap_)
      IfThenElse(new(heap_) SystemLiteral(ComMvRangeClosedUpperBound), // If 2
		 predC2,
		 predC3) ) ) );

  // Finally, AND together the three parts of the predicate.
  ItemExpr *predicate = new(heap_) BiLogic(ITM_AND, predA, predB);

  predicate = new(heap_) BiLogic(ITM_AND, predicate, predC) ;  

  return predicate;
}

//----------------------------------------------------------------------------
// This virtual method is overridden by MvMultiTxnMavBuilder for some specific
// predicates needed for multi-transactional refresh. The predicate here is
// the one in the Scan node on the range log itself - not to confuse with the
// range log join predicate in the method above.
// Excluded from coverage test - used only with range logging.
ItemExpr *MvRefreshBuilder::buildSelectionPredicateForScanOnRangeLog() const
{ 
  if (!useUnionBakeboneToMergeEpochs())
  {
    // The Range log predicate on the epoch is the same as for the IUD log.
    return MvIudLog::buildLogEpochPredicate(&getLogsInfo().getDeltaDefinition(), 
					    heap_);
  }
  return NULL;
}

//----------------------------------------------------------------------------
// Have a uniform select list over the IUD log.
// Columns added by parent class are: 
// 				    1. Base table columns
//				    2. @OP column 
// Columns added by this class are:
//				    3. @BR columns = NULL
//		    		    4. @ER columns = NULL
//				    5. @TS column
// We add the columns in order to construct the union between the range 
// and the IUD read blocks
//----------------------------------------------------------------------------
RelRoot *MvRefreshBuilder::buildRootOverIUDLog(RelExpr *topNode) const
{
  ItemExpr *opExpr = new(heap_) ColReference(new(heap_) 
			  ColRefName(MavBuilder::getVirtualOpColumnName(), 
				     getLogsInfo().getIudLogTableName() ));

  RelRoot *root = buildRootWithUniformSelectList(
		topNode,
		opExpr,
		&getLogsInfo().getIudLogTableName(),
                FALSE);

  if (!getLogsInfo().getDeltaDefinition().useRangeLog())
    return root;

// Excluded from coverage test - used only with range logging.

  const NAString beginRangePrefix(COMMV_BEGINRANGE_PREFIX);
  const NAString endRangePrefix(COMMV_ENDRANGE_PREFIX);

  // The "begin range" vector of the range log.
  ItemExpr *beginRangeCI = BinderUtils::buildClusteringIndexVector(
					       getLogsInfo().getBaseNaTable(), 
					       heap_, 
					       &getLogsInfo().getIudLogTableName(),
					       SP_USE_AT_SYSKEY | SP_USE_NULL,
					       NULL,						
					       &beginRangePrefix);
  // The "end range" vector of the range log.
  ItemExpr *endRangeCI =  BinderUtils::buildClusteringIndexVector(
					getLogsInfo().getBaseNaTable(), 
					heap_, 
					&getLogsInfo().getIudLogTableName(), 
					SP_USE_AT_SYSKEY | SP_USE_NULL,
					NULL,
					&endRangePrefix);

  ItemExpr *rangeIdCol = new(heap_) 
    RenameCol(new(heap_) SystemLiteral(), new(heap_) ColRefName(COMMV_RANGE_ID_COL));

  root->addCompExprTree(beginRangeCI);
  root->addCompExprTree(endRangeCI);
  root->addCompExprTree(rangeIdCol);

  return root;
}  // MvRefreshBuilder::buildRootOverIUDLog()

//----------------------------------------------------------------------------
// Have a uniform select list over the RANGE log.
// Columns added by me: 
//		      1. Base table columns
//		      2. @OP column = 1
//		      3. @BR columns
//		      4  @ER columns
//                    5. Range ID
//----------------------------------------------------------------------------
// Excluded from coverage test - used only with range logging.
RelRoot *MvRefreshBuilder::buildRootOverRangeBlock(RelExpr *topNode) const
{
  ItemExpr *opExpr = new(heap_) SystemLiteral(1); // Range log has only inserts.

  RelRoot *root = buildRootWithUniformSelectList(
		topNode,
		opExpr,
		&getLogsInfo().getBaseTableName(),
                TRUE);

  const NAString beginRangePrefix(COMMV_BEGINRANGE_PREFIX);
  // The "begin range" vector of the range log.
  ItemExpr *beginRangeCI = BinderUtils::buildClusteringIndexVector(
					       getLogsInfo().getBaseNaTable(), 
					       heap_, 
					       &getLogsInfo().getRangeLogTableName(),
					       SP_USE_AT_SYSKEY,
					       NULL,						
					       &beginRangePrefix);

  const NAString endRangePrefix(COMMV_ENDRANGE_PREFIX);
  // The "end range" vector of the range log.
  ItemExpr *endRangeCI = BinderUtils::buildClusteringIndexVector(
					getLogsInfo().getBaseNaTable(), 
					heap_, 
					&getLogsInfo().getRangeLogTableName(), 
					SP_USE_AT_SYSKEY,
					NULL,
					&endRangePrefix);

  ItemExpr *rangeIdCol = new(heap_) 
    ColReference(new(heap_) 
      ColRefName(COMMV_RANGE_ID_COL, getLogsInfo().getRangeLogTableName(), heap_));


  root->addCompExprTree(beginRangeCI);  
  root->addCompExprTree(endRangeCI);
  root->addCompExprTree(rangeIdCol);

  return root;
}  // MvRefreshBuilder::buildRootOverRangeBlock()


//////////////////////////////////////////////////////////////////////////////
// Have a uniform select list over the logs
// (<all-table-column-names>, @OP). This select list all the
// base table columns, but not the IUD log columns (except @OP).
//////////////////////////////////////////////////////////////////////////////

RelRoot *MvRefreshBuilder::buildRootWithUniformSelectList(RelExpr *topNode,
							  ItemExpr *opExpr,
							  const CorrName *nameOverride,
                                                          NABoolean forRange) const
{
  // all of the table column names used by the MV
  ItemExpr *leftRootSelectList = buildBaseTableColumnList(SP_USE_AT_SYSKEY);

  RenameCol *opColumn = new(heap_) 
    RenameCol(opExpr, new(heap_) ColRefName(MavBuilder::getVirtualOpColumnName(), 
					    *nameOverride));

  // The root select list is (<all-table-column-names>, @OP).
  // Can't use * here because we are eliminating the extra @ columns
  // of the IUD log,
  leftRootSelectList = new(heap_)ItemList(leftRootSelectList,opColumn);

  return new(heap_) RelRoot(topNode, REL_ROOT, leftRootSelectList);
}


//----------------------------------------------------------------------------
// Build a join between the base table and the range log, and Union the 
// result with the IUD log (passed in topNode).
// The constructed tree looks as follows:
//
//			   Join
//	                 /      \
//	             Scan        Scan
//	         (Range Log)  (Base table)
//
// Excluded from coverage test - used only with range logging.
RelExpr *MvRefreshBuilder::buildReadRangesBlock() const
{
  CMPASSERT(getLogsInfo().getDeltaDefinition().useRangeLog());

  // Build a scan node over the range log table
  RelExpr *scanRangeLog = buildReadRangeLogBlock();

  // This is the Scan on the base table.
  RelExpr *scanBaseTable = buildReadBaseTable();

  RelExpr *joinNode = buildJoinBaseTableWithRangeLog(scanRangeLog, scanBaseTable);

  Lng32 coveredRows = getLogsInfo().getDeltaDefinition().getCoveredRows();

  // Limit the Join cardinality with the coveredRows information
  if (coveredRows != 0)
  {
    joinNode->addCardConstraint(
	      (Cardinality) 0, 
	      (Cardinality) coveredRows);
  }

  RelRoot *root = buildRootOverRangeBlock(joinNode);

  // Set an empty MvBindContext in the root. This way the Scan on the
  // base table will not be switched into a Scan on the log.
  MvBindContext *emptyContext = new(heap_) MvBindContext(heap_);
  root->setMvBindContext(emptyContext);
  
  return root;
}

//----------------------------------------------------------------------------
// Build the scan on the table and rename the table syskey column if exists
// to @SYSKEY
// Excluded from coverage test - used only with range logging.
RelExpr *MvRefreshBuilder::buildReadBaseTable() const
{
  RelExpr *topNode = new(heap_) Scan(getLogsInfo().getBaseTableName());

  Lng32 coveredRows = getLogsInfo().getDeltaDefinition().getCoveredRows();

  // Fix the cardinality of the base table to its initial cardinality.
  // For hash join and merge join this is the exact cardinality.
  // For nested-join because the fact that the ranges in the range log
  // do not overlap the scan on the table will not produce more than
  // the table cardinality .
  fixScanCardinality(topNode, 1.0, coveredRows);

  // The root select list is (*, isLastExpr).
  ColRefName *star = new(heap_) ColRefName(1);  // isStar
  
  ItemExpr *rootSelectList = new(heap_) ColReference(star);
  
  if (getLogsInfo().getBaseNaTable()->getClusteringIndex()->hasSyskey())
  {
    ItemExpr *syskeyCol = new(heap_) 
      RenameCol(new(heap_)
	ColReference(new(heap_) 
	  ColRefName("SYSKEY", getLogsInfo().getBaseTableName())), new(heap_) 
	ColRefName(COMMV_SYSKEY_COL, getLogsInfo().getBaseTableName()));

    rootSelectList = new(heap_) ItemList(rootSelectList, syskeyCol);
  }
  
  RelRoot *rootNode = new(heap_) RelRoot(topNode, REL_ROOT, rootSelectList);

  return rootNode;
}

//----------------------------------------------------------------------------
// Excluded from coverage test - used only with range logging.
RelExpr *MvRefreshBuilder::buildJoinBaseTableWithRangeLog(RelExpr *scanRangeLog, 
							  RelExpr *scanBaseTable) const
{
  // build the Join between the two tables.
  ItemExpr *joinPredicate = buildRangeLogJoinPredicate();
  Join *joinNode = NULL;

  CMPASSERT(getLogsInfo().getDeltaDefinition().getDELevel() != DeltaOptions::NO_DE);

  joinNode = new(heap_) 
    Join(scanRangeLog, scanBaseTable, REL_JOIN, joinPredicate);

  joinNode->setTSJForWrite(TRUE);
  joinNode->forcePhysicalJoinType(Join::NESTED_JOIN_TYPE);
  joinNode->getInliningInfo().setFlags(II_JoinChildsOrder);

  return joinNode;
}

//----------------------------------------------------------------------------
// This function builds a sub-tree that will read the range log records.
// For optimization reasons, in some cases , we prefer to use a merge union
// sub-tree instead of using a simple scan and a sort node on top.
// Excluded from coverage test - used only with range logging.
RelExpr *MvRefreshBuilder::buildReadRangeLogBlock() const
{
  // This is the Scan on the range log table.
  Scan *scanNode = new(heap_) Scan(getLogsInfo().getRangeLogTableName());

  RelExpr *topNode = scanNode;

  // The Range log predicate on the epoch is the same as for the IUD log.
  ItemExpr *selectionPred = buildSelectionPredicateForScanOnRangeLog();

  if (selectionPred)
    topNode->addSelPredTree(selectionPred);

  if (useUnionBakeboneToMergeEpochs())
  {
    fixScanCardinality(topNode, 
   			 1 / getNumOfScansInUnionBackbone(),		     
			 (Cardinality) getLogsInfo().getDeltaDefinition().getNumOfRanges());

    topNode = buildUnionBakeboneToMergeEpochs(topNode);
  }
  else
  {
    // Fix the cardinality of the range log by the number of ranges.
    fixScanCardinality(topNode, 
		       1,		     
		       (Cardinality) getLogsInfo().getDeltaDefinition().getNumOfRanges());
  }

  return topNode;
}

//----------------------------------------------------------------------------
// Set the ForceCardinalityInfo for this node in order to fix the estimated 
// cardinality in the optimizer phase
void MvRefreshBuilder::fixScanCardinality(RelExpr *node,
  					  double cardinalityFactor, 
					  Cardinality cardinality) const
{
  node->getInliningInfo().BuildForceCardinalityInfo(cardinalityFactor,
						    cardinality,
						    heap_);
}


//----------------------------------------------------------------------------
// Build a left linear tree of merge Union nodes in order to avoid sorting
// the data. For example, if the syntax was ... BETWEEN epoch1 AND epoch2...
// The result tree will be:
//                             MergeUnion
//                              /     \
//                          . . .     Scan Log
//                      MergeUnion    (epoch2)
//                       /     \
//               MergeUnion    Scan Log
//                /     \      (epoch1+2)
//         Scan Log     Scan Log
//  (@EPOCH=epoch1)     (epoch1+1)
//
// Each Scan reads the data ordered by the clustering key columns of the 
// base table, which are also the next columns of the log CI after the @EPOCH.
// The merge union nodes then merge the rows, keeping them ordered by the CI
// without the need to sort them.
// This trick becomes very inefficient as the number of epochs grow. We 
// will probably add a check here, and use a sort after all, if the number 
// is too large.
RelExpr *MvRefreshBuilder::buildUnionBakeboneToMergeEpochs(RelExpr *topNode) const
{
  DeltaDefinition &deltaDef = getLogsInfo().getDeltaDefinition();

   // Add another root above the Scan for the epoch predicate.
  topNode = new(heap_) RelRoot(topNode);

  RelExpr  *unionTree = topNode;
  ItemExpr *epochPred =	NULL;

  for (Lng32 epoch = deltaDef.getBeginEpoch()+1; 
       epoch <= deltaDef.getEndEpoch(); 
       epoch++)
  {
    RelExpr *newScan = topNode->copyTree(heap_);

    epochPred = 
      BinderUtils::buildPredOnCol(ITM_EQUAL, COMMV_EPOCH_COL, epoch, heap_);

    newScan->addSelPredTree(epochPred);

    unionTree = new(heap_) Union(unionTree, newScan, NULL, NULL, REL_UNION,
                                 CmpCommon::statementHeap(), TRUE);
  }

  // Put the predicate on topNode (the first Scan node) after we are
  // done duplicating it.
  epochPred = 
    BinderUtils::buildPredOnCol(ITM_EQUAL, COMMV_EPOCH_COL, 
                                deltaDef.getBeginEpoch(), heap_);

  topNode->addSelPredTree(epochPred);

  return unionTree;
}  // MvRefreshBuilder::buildUnionBakeboneToMergeEpochs()

//----------------------------------------------------------------------------
// The union backbone is currently only needed in MAV's
NABoolean MvRefreshBuilder::useUnionBakeboneToMergeEpochs() const
{
  return FALSE;
}

//----------------------------------------------------------------------------
// This function returns the number of scan operators that are used to read
// the IUD/Range log table
Int32 MvRefreshBuilder::getNumOfScansInUnionBackbone() const
{
  DeltaDefinition &deltaDef = getLogsInfo().getDeltaDefinition();

  return deltaDef.getEndEpoch() - deltaDef.getBeginEpoch() + 1;
}

//----------------------------------------------------------------------------
// Prepare the product matrix for multi-delta refresh (Both MAV and MJV):
// 1. Build the Join Graph. This is the graph of join predicates between 
//    the tables, and takes into account RI constraints.
// 2. Find the optimal solution to travel the graph while using as many 
//    RI constraints as possible, in order to enable RI optimizations.
// 3. Build the Product Matrix according to the graph solution.
// 4. Find the relevant matrix rows according to the current activation phase.
MultiDeltaRefreshMatrix *MvRefreshBuilder::prepareProductMatrix(NABoolean supportPhases, 
								NABoolean checkOnlyInserts)
{
  // Verify that we have Multi-Delta
  CMPASSERT(getDeltaDefList()->entries() > 1); 

  // 1. Build the Join Graph
  //    The join graph is deleted as part of the MVInfo, by the builder Dtor.
  MVJoinGraph *joinGraph = 
    getMvInfo()->initJoinGraph(bindWA_, getDeltaDefList(), checkOnlyInserts);

  // 2. Find the optimal solution to the graph.
  CMPASSERT(joinGraph != NULL);
  joinGraph->findBestOrder();

  // The max number of rows is 2^m, where m is the number of non-empty deltas.
  Int32 maxNumOfRows = (Int32)pow(2, getDeltaDefList()->entries());

  // 3. Build the Product Matrix according to the graph solution.
  // The matrix is constructed with the first row, that serves as the 
  // induction base.
  MultiDeltaRefreshMatrix *productMatrix = new(heap_)
    MultiDeltaRefreshMatrix(maxNumOfRows, joinGraph, heap_);
  productMatrix->setPhasesSupported(supportPhases);

  if (productMatrix->isDuplicatesOptimized() &&
      avoidMultiDeltaOptimization()             )
  {
    productMatrix->disableDuplicateOptimization();
  }

  const MVJoinGraphSolution& graphSolution = joinGraph->getSolution();
  const ARRAY(Lng32)& theRoute = graphSolution.getRoute();

  // Every iteration adds another table (another column to the matrix),
  // and if the delta for that table should be read, add rows as well.
  for (CollIndex i=productMatrix->getRowLength(); i<theRoute.entries(); i++)
  {
    Lng32 tableIndex = theRoute[i];
    NABoolean isOptimized = graphSolution.isTableOnRI(tableIndex);
    NABoolean isEmptyDelta = 
      joinGraph->getTableObjectAt(tableIndex)->isEmptyDelta();
    // The delta should be read if it is non-empty, and if it cannot be 
    // optimized away using RI.
    NABoolean readDelta = !isEmptyDelta && !isOptimized;
    // Add the table to the matrix. If readDelta is TRUE, this effectivly 
    // doubles the matrix size.
    productMatrix->addTable(readDelta);
  }

  // 4. Find relevant matrix rows according to the current phase.
  productMatrix->setThisPhase(getPhase());

  // If there are too many deltas - Abort rather than risk an optimizer crash.
  if (productMatrix->isTooManyDeltasError())
  {
    // 12319 ZZZZZ 99999 ADVANCED MAJOR DBADMIN Too many deltas: Materialized View $0~TableName cannot be incrementally refreshed because $0~Int0 of its base tables have changed.
      *CmpCommon::diags() << DgSqlCode(-12319);
      *CmpCommon::diags() << DgTableName(getMvCorrName().getQualifiedNameAsString());
      *CmpCommon::diags() << DgInt0(getDeltaDefList()->entries());
      bindWA_->setErrStatus();
      return NULL;
  }

  // Will we be done in this phase?
  if (!productMatrix->isLastPhase())
  {
    if (getMvInfo()->isMinMaxUsed())
    { // Multi-phase refresh not allowed on min/max MAVs. Must Recompute.
      // 12308 Internal Refresh error - Min Max values are compromised.
      *CmpCommon::diags() << DgSqlCode(-12308);
      bindWA_->setErrStatus();
      return NULL;
    }
    else
    {
      // Tell Refresh::bindNode() to return a warning:
      // 12304 An additional phase is needed to complete the refresh.
      // If the warning is inserted into the Diags area now, it may get lost
      // before returning to Refresh::bindNode().
      getRefreshNode()->setAdditionalPhaseNeeded();
    }
  }

#ifndef NDEBUG
  if ( CmpCommon::getDefault(MV_DUMP_DEBUG_INFO) == DF_ON )
    productMatrix->print();
#endif

  return productMatrix;
}

//----------------------------------------------------------------------------
// Prepare a single join product according to the product matrix row.
// Used for multi-delta refresh (both MAV and MJV)
RelRoot *MvRefreshBuilder::prepareProductFromMatrix(
				const MultiDeltaRefreshMatrix& productMatrix,
				RelRoot *rawJoinProduct,
				Int32 rowNumber,
				NABoolean isLast) 
{
  const MultiDeltaRefreshMatrixRow *currentRow = 
    productMatrix.getRow(rowNumber);

  RelRoot *product = NULL;
  // A new copy of the join product should be made for each call. If this 
  // is the last call, we can use the original join product.
  if (isLast)
    product = rawJoinProduct;
  else
    product = (RelRoot *)rawJoinProduct->copyTree(heap_);

  // Mark in the binder context the names of tables for which we should 
  // read the log instead of the table.
  MvBindContext *mvBinderContext = new(heap_) MvBindContext(heap_);
  mvBinderContext->setRefreshBuilder(this);

  // For each table in the join product
  for (Int32 i=0; i<productMatrix.getRowLength(); i++)
  {
    // If for this matrix row, we should read the table's log
    if (currentRow->isScanOnDelta(i))
    {
      // Get the table's index into the MVInfo used object list.
      Lng32 tableIndex = productMatrix.getTableIndexFor(i);

      // Find the table name.
      MVUsedObjectInfo *usedObject = 
	getMvInfo()->getUsedObjectsList()[tableIndex];
      const QualifiedName& tableName = 
	usedObject->getObjectName();

      // Add the table name to the switch list.
      RelExpr *scanLogBlock = buildLogsScanningBlock(tableName);
      if (bindWA_->errStatus() || scanLogBlock == NULL)
	return NULL;

      mvBinderContext->setReplacementFor(&tableName, scanLogBlock);

      usedObject->initColNameMap(getLogsInfo().getBaseNaTable(), heap_);
    }
  }

  product->setMvBindContext(mvBinderContext);

  return product;
}

// ===========================================================================
// ===========================================================================
// ===================  class  MvRecomputeBuilder  ===========================
// ===========================================================================
// ===========================================================================

//////////////////////////////////////////////////////////////////////////////
// Build the recompute tree:
//
//           Ordered Union
//          /             \
//      RelRoot          RelRoot
//         |                |
//      Delete           Insert MV 
//      from MV             |
//         |             MV Select 
//      Scan MV          Query
//////////////////////////////////////////////////////////////////////////////


//----------------------------------------------------------------------------
// Build the refresh tree of a RECOMPUTE refresh:
// 1. If noDeleteOnRecompute_ is not set, delete the contents of the MV.
// 2. Delete any Multi-transactional context rows from the IUD log of
//    the underlying table.
// 3. Insert the results of the MV select into the MV table.
RelExpr *MvRecomputeBuilder::buildRefreshTree()
{

  RelExpr *insertToMvSubTree = buildInsertToMvSubTree();

  if (noDeleteOnRecompute_)
    return insertToMvSubTree;

  RelExpr *topNode = insertToMvSubTree;

  if (!noDeleteOnRecompute_)
  {
    // Prepare the DELETE FROM <mv>.
    RelExpr *leftSubTree = buildDeleteFromMvSubTree();

    // This is an ordered union - do the delete first, and then the insert.
    Union *unionNode = new(heap_) Union(leftSubTree, topNode, NULL, NULL,
                                        REL_UNION, 
                                        CmpCommon::statementHeap(), TRUE);
    unionNode->setOrderedUnion();
    
    topNode = unionNode;
  }

  return topNode;
}

//----------------------------------------------------------------------------
// Build the insert mv from select block
RelExpr *MvRecomputeBuilder::buildInsertToMvSubTree() const
{
  // Get the MV SELECT tree.
  RelExpr *mvSelectTree = getMvInfo()->buildMVSelectTree();
  
  // Put an MV Insert node on top of it.
  Insert *insertMVnode = new(heap_) 
    Insert(getMvCorrName(), NULL, REL_UNARY_INSERT, mvSelectTree);
  
  // Affected rows should not be counted.
  insertMVnode->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;  
  
  // Recompute is a NOLOG operation.
  insertMVnode->setNoLogOperation(TRUE);
  
  RelRoot *topRoot = new(heap_) RelRoot(insertMVnode);
  
  // There are no outputs here.
  topRoot->setEmptySelectList();

  return topRoot;
}

//----------------------------------------------------------------------------
// Build the delete from mv block
RelExpr *MvRecomputeBuilder::buildDeleteFromMvSubTree() const
{
  Scan *scanMVnode = new(heap_) Scan(getMvCorrName());
  
  Delete *deleteMVnode = new(heap_) 
    Delete(getMvCorrName(), NULL, REL_UNARY_DELETE, scanMVnode);
  
  // Recompute is a NOLOG operation.
  deleteMVnode->setNoLogOperation(TRUE);
  
  // Affected rows should not be counted.
  deleteMVnode->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;  

  RelRoot *leftRoot = new(heap_) RelRoot(deleteMVnode);
  
  // There are no outputs here.
  leftRoot->setEmptySelectList();

  return leftRoot;
}

// ===========================================================================
// ===========================================================================
// ===================  class  MavBuilder  ===================================
// ===========================================================================
// ===========================================================================

// Initialization of static data members

// "Virtual" columns are columns that are manualy added to the RETDesc. Their
// names have the @ character so they never interfere with a real column.
const char MavBuilder::virtualOpColumnName_[]     = "@OP";
const char MavBuilder::virtualGopColumnName_[]    = "@GOP";
const char MavBuilder::virtualIsLastColumnName_[] = "@ISLAST";

// These are correlation names to "tables".
const char MavBuilder::sysDeltaName_[]	= "SYS_DELTA";
const char MavBuilder::sysCalcName_[]	= "SYS_CALC";
const char MavBuilder::sysMavName_[]	= "SYS_MAV";
const char MavBuilder::startCtxName_[]	= "SYS_STARTCTX";
const char MavBuilder::endCtxName_[]	= "SYS_ENDCTX";
const char MavBuilder::minMaxMavName_[]	= "SYS_MINMAX_MAV";

// These names are used in pipelined refresh, for the TupleList join.
const char MavBuilder::gopTableName_[]  = "GOP_TABLE";
const char MavBuilder::gopCol1Name_[]   = "GROUPOP";
const char MavBuilder::gopCol2Name_[]   = "OP";

// These are name suffixes of extra aggregate columns added for each 
// Min/Max column.
const char MavBuilder::extraColSuffixForIns_[] = "_INS";
const char MavBuilder::extraColSuffixForDel_[] = "_DEL";


MavBuilder::MavBuilder(const CorrName&  mvName,
		       MVInfoForDML    *mvInfo,
		       Refresh	       *refreshNode,
		       NABoolean        isProjectingMvDelta,
		       BindWA          *bindWA)
: MvRefreshBuilder(mvName, mvInfo, refreshNode, bindWA),
  isProjectingMvDelta_(isProjectingMvDelta),
  canSkipMinMax_(FALSE),
  pBindWA(bindWA),
  isGroupByAprefixOfTableCKeyColumns_(-1)
{
  // The MinMax recalculation block can be skipped if the MV does not use
  // any MIN and MAX functions, or if all the deltas are insert only, for
  // a single phase of activation. (Single phase activation is not checked 
  // here because compiling multi-phase multi-delta refresh og a MAV with 
  // Min/Max functions generates an error.
  // For details see MultiDeltaMavBuilder::buildRefreshTree().
  canSkipMinMax_ = 
    !getMvInfo()->isMinMaxUsed() ||
     getDeltaDefList()->areAllDeltasInsertOnly();                   
}

// ===========================================================================
// ==== Methods for building the bottom part of the refresh tree - called
// ==== bt buildScanLogBlock(), replacing the Scan of the base table with
// ==== a Scan on the logs).
// ===========================================================================

//----------------------------------------------------------------------------
// This method is used for building the most simple type of MAV refresh.
RelExpr *MavBuilder::buildRefreshTree()
{
  RelRoot *mvSelectTree = getMvInfo()->buildMVSelectTree();

  // This class handles single delta only.
  CMPASSERT(getDeltaDefList()->entries() == 1); 
  DeltaDefinition *deltaDef = (*getDeltaDefList())[0];

  // Now go build the tree.
  return buildTheMavRefreshTree(mvSelectTree, deltaDef);
}

//----------------------------------------------------------------------------
// This method is common to all types of MAV refrsh.
RelExpr  *MavBuilder::buildTheMavRefreshTree(RelExpr         *mvSelectTree,
					     DeltaDefinition *deltaDef)
{
  RelExpr * pDCB = buildDeltaCalculationBlock(mvSelectTree);

  // For Debugging Only !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#ifndef NDEBUG
  if (getenv("DEBUG_DCB"))
    return pDCB;
#endif

  RelExpr * pDPB = buildDeltaProcessingBlock(deltaDef);  
    
  // If we need to project outputs to the next level of pipelining, use a
  // left TSJ. Otherwise, use a TSJ so no outputs will be projected.
  Join *tsjNode = new(heap_) 
    Join(pDCB, pDPB, isProjectingMvDelta() ? REL_LEFT_TSJ : REL_TSJ);

  tsjNode->setTSJForWrite(TRUE);
  return tsjNode;
}

// ===========================================================================
// ==== Methods for building the Delta Calculation Block.
// ==========================================================================


//////////////////////////////////////////////////////////////////////////////
// Build the MAV Delta Calculation Block:
//
//               RelRoot 
//             (SYS_CALC)
//                 |
//                LOJ
//             /      \
//    RenameTable     RelRoot
//   (SYS_DELTA)         |
//         |          Scan MAV
//    mvSelectTree   (SYS_MAV)
//////////////////////////////////////////////////////////////////////////////
RelExpr *MavBuilder::buildDeltaCalculationBlock(RelExpr *mvSelectTree)
{
  // Create a correlation name SYS_MAV to the mav table.
  const CorrName mavCorrName(getMvCorrName().getQualifiedNameObj(), 
                             heap_, getSysMavName());

  const MVInfoForDML *mvInfo = getMvInfo();
  CMPASSERT(mvInfo != NULL);

  // All the algorithm is done in the MavRelRootBuilder class.
  MavRelRootBuilder rootBuilder(mvInfo, heap_);

  // Divide the MAV column to the tree groups and handle the GroupBy columns.
  rootBuilder.init();

  NABoolean wasFullDE = getDeltaDefList()->areAllDeltasWithFullDE();

  // Build left side of LOJ
  RelExpr *deltaRootNode = 
    rootBuilder.buildDeltaCalculationRootNodes(mvSelectTree, 
                                               canSkipMinMax_,
					       wasFullDE);

  // Build right side of LOJ
  // the scan of the table will be substituted in bind for a scan of a log
  RelExpr *scanNode = new(heap_) Scan(mavCorrName);
  RelRoot *rightRoot = new(heap_) RelRoot(scanNode);

  // Set an empty MvBindContext in order to prevent the Scan MAV to be switched.
  MvBindContext *emptyContext = new(heap_) MvBindContext(heap_);
  emptyContext->setRefreshBuilder(this);
  rightRoot->setMvBindContext(emptyContext);

  // Build the LOJ
  ItemExpr *gbSelectionPred = 
    buildGroupByColumnsPredicate(getSysDeltaName(), getSysMavName());
  RelExpr *lojNode = new(heap_) 
    Join(deltaRootNode, rightRoot, REL_LEFT_JOIN, gbSelectionPred);

  // Build a stack of 5 RelRoot nodes to calculate the SYS_CALC columns.
  RelExpr *rootNode = rootBuilder.buildCalcCalculationRootNodes(lojNode);

  return rootNode;
}

//////////////////////////////////////////////////////////////////////////////
// Construct a selection predicate on the MAV group by columns.
// The two parameters are correlation names for the two tables (the second 
// is optional).
// The third parameter - ignoreAlias means that in case the GroupBy column
// has an alias, it needs to be bypassed, and the predicate should be written
// using the original column name. This affects the second table only.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MavBuilder::buildGroupByColumnsPredicate(const NAString& table1Name, 
						   const NAString& table2Name,
						   const NABoolean ignoreAlias) const
{
  // Build the two correlation names
  const CorrName table1CorrName(table1Name); 
  const CorrName table2CorrName(table2Name);
  // Get the MAV GroupBy columns from the MVInfo.
  LIST(Lng32) gbColPositionList(heap_);
  getMvInfo()->getMavGroupByColumns(gbColPositionList);
  if (gbColPositionList.entries() == 0)
  {
    // When there are no GroupBy columns, the predicate evaluates to TRUE.
    ItemExpr *result = new(heap_) BoolVal(ITM_RETURN_TRUE);
    return result;
  }

  ItemExpr *predicate = NULL;

  // For each column, build the predicate:
  //     (table1Name.a = table2Name.a) 
  //       OR 
  //     (table1Name.a IS NULL AND table2Name.a IS NULL)
  // For columns that are NOT NULL, the second part is optimized away in the 
  // transform phase.
  for (CollIndex i=0; i<gbColPositionList.entries(); i++)
  {
    // Get the name of the column, by using its position from the GroupBy
    // column list.
    Lng32  gbColIndex = gbColPositionList[i];
    const MVColumnInfo *gbColInfo =
      getMvInfo()->getMVColumns().getMvColInfoByIndex(gbColIndex);
    const NAString& colName = gbColInfo->getColName();

    const NAString* origColName = NULL;

    // In order to get to the original column name, get the original base
    // table name and column number from the MVColumnInfo.
    if (ignoreAlias)
    {
      const QualifiedName& origTableName = gbColInfo->getOrigTableName();
      CorrName* origCorrName = new CorrName(origTableName);
      NATable* origTable = pBindWA->getNATable(*origCorrName);
      const NAColumnArray& origColArray = origTable->getNAColumnArray();
      const NAColumn* origCol = origColArray.getColumn(gbColInfo->getOrigColNumber());
      origColName = &(origCol->getColName());
    }

    // Build two ColRefs for the column - one for each table.
    ItemExpr *colRef1 = new (heap_) 
      ColReference(new(heap_) ColRefName(colName, table1CorrName));    

    ItemExpr *colRef2 = NULL;
    if (!ignoreAlias)
    {
      // Normal case, use the (aliased) column name.
      colRef2 = new(heap_) 
	ColReference(new(heap_) ColRefName(colName, table2CorrName));
    }
    else
    {
      // IgnoreAlias: use the original column name.
      colRef2 = new(heap_) 
	ColReference(new(heap_) ColRefName(*origColName, table2CorrName));
    }

    BiRelat *equalPred = new(heap_)
      BiRelat(ITM_EQUAL, colRef1, colRef2);

    equalPred->setSpecialNulls(TRUE);


    if (predicate == NULL)
    {
      // Initialize the list to the first item.
      predicate = equalPred;
    }
    else
    {
      // Add the new item to an existing list.
      predicate = new(heap_) 
        BiLogic(ITM_AND, predicate, equalPred);
    }
  }

  return predicate;
}


// ===========================================================================
// ==== Methods for building the Delta Processing Block
// ==========================================================================

//////////////////////////////////////////////////////////////////////////////
// Construct the MAV Delta Processing Block.
// Both Scan nodes have a selection predicate on the MAV group-by columns:
//   (g,h,i) = (DELTA.g, DELTA.h, DELTA.i) 
//   where g,h,i reprsent the MAV group-by columns.
//
//         IF(insCond)                      insCond is (@GOP = GOP_INSERT)
//        /          \                 
//    RelRoot          IF(delCond)          delCond is (@GOP = GOP_DELETE)
//       |            /          \          
//    Insert      RelRoot        IF(MinMaxCond)
//       |           |           /           \
//     Tuple      Delete      RelRoot       RelRoot
//                   |          |              |
//                 Scan       Signal        Update
//                                             |
//                                            Scan
//
// All the Scan nodes are on the MAV, and have a selection predicate on the
// groupby columns.
// The IF(insCond) and left sub-tree is skipped if no rows were inserted.
// The IF(delCond) and left sub-tree is skipped if no rows were deleted.
// The IF(MinMaxCond) and left sub-tree is skipped if there are no Min/Max 
// columns, or if the delta is insert only. This IF checks if the current
// Min/Max value for the group was deleted.
// deltaDef has the statistics on the types of rows in the log.
// deltaDef can be NULL is case of pipelined refresh.
//////////////////////////////////////////////////////////////////////////////
RelExpr *MavBuilder::buildDeltaProcessingBlock(DeltaDefinition *deltaDef)
{
  // In some conditions we can optimize the tree by skipping the MAV
  // insert and delete nodes, as well as the MinMax recompute node.
  NABoolean canSkipDeleteMav = FALSE;
  NABoolean canSkipInsertMav = FALSE;

  if (deltaDef != NULL)
  {
    // The MAV delete can be skipped if there are no deleted or updated rows.
    canSkipDeleteMav = deltaDef->isInsertOnly();

    // The MAV Insert can be skipped if there are no inserted or updated rows.
    canSkipInsertMav = deltaDef->isDeleteOnly();
  }

  // Do the Update node (the default)
  // assign CALC values where group by equals DELTA values
  RelExpr *topNode = buildDPBUpdate(getSysCalcName(), getSysDeltaName());

  // If we cannot skip the MinMax check - do it.
  // The initialization is in the MavBuilder Ctor.
  if (!canSkipMinMax_)
    topNode = buildDPBMinMaxIfCondition(topNode);

  // If we cannot skip the MAV Delete node - build it.
  if (!canSkipDeleteMav)
    topNode = buildDPBDelete(topNode);

  // If we cannot skip the MAV Insert node - build it.
  if (!canSkipInsertMav)
    topNode = buildDPBInsert(deltaDef, topNode);

  // Put a root with an empty select list on top (no outputs).
  RelRoot *topRoot = new(heap_) RelRoot(topNode);
  topRoot->setEmptySelectList();
  return topRoot;
} // MavBuilder::buildDeltaProcessingBlock

//////////////////////////////////////////////////////////////////////////////
// Build the Delta Processing Block Update sub-tree.
// The Update expressions are of the form:
// (a is a MAV aggregate column, b is a MAV groupby column)
// SET   expressions: a = tableForSet.a 
// WHERE expressions: b = tableForWhere.b
// This method is also used by MinMaxMavBuilder.
//////////////////////////////////////////////////////////////////////////////
RelExpr *MavBuilder::buildDPBUpdate(const NAString& tableForSet,
				    const NAString& tableForWhere) const
{
  const CorrName assignCorrName (tableForSet, heap_);
  ItemExpr *updatedCols = NULL;

  // Build the WHERE predicate on the GroupBy columns.
  ItemExpr *gbSelectionPred = buildGroupByColumnsPredicate(tableForWhere);
  // Build the Scan MAV node and put the predicate inside.
  Scan *mavScanNode = new(heap_) Scan(getMvCorrName());
  mavScanNode->addSelPredTree(gbSelectionPred);

  // Build the Assign expressions representing the SET clauses.
  // This is done for all aggregate columns of the MAV.
  for (CollIndex i=0; i<getMvInfo()->getMVColumns().entries(); i++)
  {
    MVColumnInfo *currentCol = getMvInfo()->getMVColumns()[i];

    // Skip the GroupBy columns of the MAV - they need not be updated.
    if (currentCol->getColType() != COM_MVCOL_AGGREGATE)
      continue;

    // Build the Assign expression: SET <colName> = <tableForSet>.<colName>
    const NAString& colName = currentCol->getColName();
    ItemExpr *col = new(heap_) 
      Assign(new(heap_) ColReference(new(heap_) ColRefName(colName)),
	     new(heap_) ColReference(new(heap_) ColRefName(colName, 
							   assignCorrName)));

    // Add the new expression to the list.
    if (updatedCols == NULL)
      updatedCols = col;
    else
      updatedCols = new(heap_) ItemList(updatedCols, col);
  }

  // Build the Update node using the assign list.
  Update *mavUpdate = new(heap_) 
    Update(getMvCorrName(), NULL, REL_UNARY_UPDATE, mavScanNode, updatedCols);
  // No need to log MAV changes if this is a pipelined refresh.
  if (isProjectingMvDelta())
    mavUpdate->setNoLogOperation(TRUE);
  // Affected rows should not be counted.
  mavUpdate->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;  
  // Make sure we have no outputs.
  RelRoot *updateRoot = new(heap_) RelRoot(mavUpdate);
  updateRoot->setEmptySelectList();
  return updateRoot;
}

//////////////////////////////////////////////////////////////////////////////
// Build the Delete sub-tree of the MAV Delta Processing Block.
RelExpr *MavBuilder::buildDPBDelete(RelExpr *topNode)
{
  // Build the WHERE predicate on the GroupBy columns.
  ItemExpr *gbSelectionPred = buildGroupByColumnsPredicate(getSysDeltaName());
  // Build the Scan MAV node and put the predicate inside.
  Scan *mavScanNode = new(heap_) Scan(getMvCorrName());
  mavScanNode->addSelPredTree(gbSelectionPred);

  // Build the Delete node. It will delete all the rows the Scan node 
  // projects. No additional predicates are needed.
  Delete *mavDelete = new(heap_) 
    Delete(getMvCorrName(), NULL, REL_UNARY_DELETE, mavScanNode);
  // No need to log MAV changes if this is a pipelined refresh.
  if (isProjectingMvDelta())
    mavDelete->setNoLogOperation(TRUE);
  // Make sure we have no outputs.
  // Affected rows should not be counted.
  mavDelete->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;  

  RelRoot *deleteRoot = new(heap_) RelRoot(mavDelete);
  deleteRoot->setEmptySelectList();

  // Now build the IF node above the Update and Delete.
  // The IF condition is (@GOP = GOP_DELETE)
  ItemExpr *delCondition =
    BinderUtils::buildPredOnCol(ITM_EQUAL, getVirtualGopColumnName(), 
                                GOP_DELETE, heap_);

  // The IF is implemented as a conditional Union.
  topNode = new(heap_) Union(deleteRoot, topNode, NULL, delCondition,
                             REL_UNION, CmpCommon::statementHeap(), TRUE);
  return topNode;
}

//////////////////////////////////////////////////////////////////////////////
// Build the Insert sub-tree of the MAV Delta Processing Block.
RelExpr *MavBuilder::buildDPBInsert(DeltaDefinition *deltaDef, RelExpr *topNode) const
{
  RelExpr *insertRoot = buildDPBInsertNodes(getSysDeltaName());

  // Now build the IF node above the Insert and the rest of the sub-tree..
  // The IF condition is (@GOP = GOP_INSERT)
  ItemExpr *insCondition =
    BinderUtils::buildPredOnCol(ITM_EQUAL, getVirtualGopColumnName(), 
				GOP_INSERT, heap_);

  // The IF is implemented as a conditional Union.
  topNode = new(heap_) Union(insertRoot, topNode, NULL, insCondition,
			     REL_UNION, CmpCommon::statementHeap(), TRUE);
  return topNode;
}

//////////////////////////////////////////////////////////////////////////////
// Build the Insert sub-tree of the MAV Delta Processing Block.
RelExpr *MavBuilder::buildDPBInsertNodes(const NAString& sourceTable) const
{
  // The group-by columns are always taken from the DELTA table.
  // The aggregate columns in the normal case are from SYS_DELTA, 
  // because no corresponding group was found in the MAV (SYS_MAV 
  // values are all NULL).
  // In the in/Max recompute case, a different name is used for the 
  // aggregate columns.
  const CorrName aggregateCorrName(sourceTable, heap_);
  const CorrName groupbyCorrName(getSysDeltaName(), heap_);

  // Build the tuple to insert into the MAV.
  ItemExpr *tupleExpr = NULL;
  for (CollIndex i=0; i<getMvInfo()->getMVColumns().entries(); i++)
  {
    ItemExpr *col = NULL;
    MVColumnInfo *mavCol = getMvInfo()->getMVColumns()[i];
    const NAString& colName = mavCol->getColName();

    // Skip the MAV SYSKEY column.
    if (colName == "SYSKEY")
      continue;

    const CorrName& sourceCorrName = 
      (mavCol->getColType() == COM_MVCOL_GROUPBY ? 
       groupbyCorrName                           :
       aggregateCorrName );

    // Build a reference to the corresponding SYS_DELTA column.
    col = new(heap_) ColReference(new(heap_) 
      ColRefName(colName, sourceCorrName));

    // Add it to th list.
    if (tupleExpr == NULL)
      tupleExpr = col;
    else
      tupleExpr = new(heap_) ItemList(tupleExpr, col);
  }

  // Build the Tuple and the Insert on top of it.
  RelExpr *tupleNode = new(heap_) Tuple(tupleExpr);
  Insert *mavInsert = new(heap_) 
    Insert(getMvCorrName(), NULL, REL_UNARY_INSERT, tupleNode);
  // No need to log MAV changes if this is a pipelined refresh.
  if (isProjectingMvDelta())
    mavInsert->setNoLogOperation(TRUE);
  // Affected rows should not be counted.
  mavInsert->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;  
  // Make sure we have no outputs.
  RelRoot *insertRoot = new(heap_) RelRoot(mavInsert);
  insertRoot->setEmptySelectList();

  return insertRoot;
}

// ===========================================================================
// ==== Methods for supporting Min/Max.
// ==========================================================================

//----------------------------------------------------------------------------
// the DCP CALC node results in two corresponding nodes for each Min/Max 
// column. the new columns are aggregations of the min/max values in the log.
// the aggregations are as such that the rows in the log marked delete are 
// separated from those marked insert. So for each min/max aggregate we end up
// with three columns: MAV_AGGREGATE, LOG_DEL_AGGREGATE, LOG_INS_AGGREGATE.
// What we want to check is if in the case of Min/Max the min/max value has
// not been deleted.
RelExpr *MavBuilder::buildDPBMinMaxIfCondition(const RelExpr *updateNode) const
{
  RelExpr *pMinMaxRecomputationBlock = buildMinMaxRecomputationBlock();

  // Now build the IF node above the Update node
  // The IF condition is (@GOP = GOP_MINMAX_RECOMPUTE)
  ItemExpr *minMaxRecomputeCondition = new (heap_) 
    BiLogic(ITM_OR, 
            BinderUtils::buildPredOnCol(ITM_EQUAL, getVirtualGopColumnName(), GOP_MINMAX_RECOMPUTE_FROM_UPDATE, heap_),
            BinderUtils::buildPredOnCol(ITM_EQUAL, getVirtualGopColumnName(), GOP_MINMAX_RECOMPUTE_FROM_INSERT, heap_) );

  Union *ifNode = new(heap_) 
    Union(pMinMaxRecomputationBlock, (RelExpr *)updateNode, NULL, minMaxRecomputeCondition
          ,REL_UNION,CmpCommon::statementHeap(),TRUE);
  return ifNode;
} // buildDPBMinMaxIfCondition

//----------------------------------------------------------------------------
RelExpr * MavBuilder::createSignal() const 
{
  RaiseError* pError = new (heap_) RaiseError(12308); // positive value == error

  ItemExpr *fakeList = new (heap_) Convert(pError); // convert error
  Tuple *tuple = new (heap_) TupleList(fakeList);

  RelRoot * result = new(heap_) RelRoot(tuple);
  result->setEmptySelectList();

  return result;
} // createSignal


//----------------------------------------------------------------------------
// This method is overridden by MinMaxMavBuilder.
RelExpr * MavBuilder::buildMinMaxRecomputationBlock() const
{
  return createSignal();
}


//----------------------------------------------------------------------------
// Do not use unions when their are too many epochs because it will blow the
// optimizer plan and when their is a single epoch (it is not needed)
NABoolean MavBuilder::useUnionBakeboneToMergeEpochs() const
{
  DeltaDefinition *deltaDef = &getLogsInfo().getDeltaDefinition();

  if (Refresh::SINGLEDELTA != getRefreshNode()->GetRefreshType())
    return FALSE;

  if (deltaDef->getEndEpoch() - deltaDef->getBeginEpoch() > 
      MAX_EPOCH_FOR_UNION_BACKBONE)
    return FALSE;

  if (deltaDef->getBeginEpoch() == deltaDef->getEndEpoch())
    return FALSE;

  // We only want to use the union backbones when the optimal plan can use sort
  // GroupBy without using a sort node. This is possible if the group by columns
  // are a prefix of the table clustering index.
  if (!isGroupByAprefixOfTableCKeyColumns())
    return FALSE;

  return TRUE;
}

//----------------------------------------------------------------------------
// 
// Check if the Group by columns are a prefix of the base table clustering 
// index. The order of the group by columns is not important.
//----------------------------------------------------------------------------
Int32 MavBuilder::isGroupByAprefixOfTableCKeyColumns() const
{
  if (isGroupByAprefixOfTableCKeyColumns_ != -1)
     return isGroupByAprefixOfTableCKeyColumns_;

  const NATable &baseTable = *getLogsInfo().getBaseNaTable();
  const NAColumnArray &indexColumns = 
    baseTable.getClusteringIndex()->getIndexKeyColumns();

  LIST(Lng32) gbColPositionList(heap_);
  getMvInfo()->getMavGroupByColumns(gbColPositionList);
  
  Int32 numOfGroupByCols = gbColPositionList.entries();
  Int32 counter = 0;
  
  for (CollIndex i=0; i<indexColumns.entries(); i++,counter++)
  {
    MVColumnInfo *currentCol = 
      getMvInfo()->getMVColumns().getMvColInfoByBaseColumn(*indexColumns[i]->getTableName(),
  					      indexColumns[i]->getPosition());  

    // If the current col is not a group by column then stop,the prefix has ended
    if (currentCol == NULL  || currentCol->getColType() != COM_MVCOL_GROUPBY)
      break;  
  }

  // Check if the prefix we found is in the same length of the groupBy vector.
  if (counter == numOfGroupByCols)
  {
    const_cast<MavBuilder*>(this)->isGroupByAprefixOfTableCKeyColumns_ = 1;
  }
  else
  {
    const_cast<MavBuilder*>(this)->isGroupByAprefixOfTableCKeyColumns_ = 0;
  }
  
  return isGroupByAprefixOfTableCKeyColumns_;
}


// ===========================================================================
// ===========================================================================
// ==============  class  MinMaxOptimizedMavBuilder  =========================
// ===========================================================================
// ===========================================================================

//----------------------------------------------------------------------------
// when a MAV is a min/max mav and there are supporting indices for the used
// table's group by columns, if the min/max values are deleted from the MV
// we have to build a Recomputation block that can recompute the min max 
// values for the MV. (Note: If there are no supporting indices the 
// MinMaxOptimizedMavBuilder class whould not be built and the base classes
// method would be called - returning an error indicating that there needs
// to be a recompute ). 
//
// There are different cases here, depending on the operation associated with
// his row. If there is an existing corresponding MAV row that needs to be 
// updated, or if there is no such row, and a new row needs to be inserted.
//
// the recompute block looks like this:
// 
//				TSJ
//			      /	    \
//		    MVSelectTree    IF (from Update)
//                                /    \
//                          Update      Insert
//                           MAV         MAV
//
// since the group by predicates are recieved from the delta calculation block
// (getSysDeltaName()) and their values are known - the group by columns in the 
// MVSelectTree are unnecessary and should be removed for performance reasons
RelExpr *MinMaxOptimizedMavBuilder::buildMinMaxRecomputationBlock() const
{
  RelRoot* pMvSelectTree = getMvInfo()->buildMVSelectTree();

  // go over select list of root and remove any group by columns
  // remember - only aggregate columns are updated.
  fixGroupingColumns(pMvSelectTree);

  RelExpr *renameNode = new(heap_) 
    RenameTable(pMvSelectTree, getMinMaxMavName());
  RelRoot* pLeftNode =  new(heap_) RelRoot(renameNode);
  pLeftNode->setDontOpenNewScope();
  
  // the bind context has a list you can access by getScanLogTablesList().
  // when a scan is encountered - if the scaned table is listed in the list
  // the scan is directed towards the view instead. leaving the list empty 
  // means scan the tables.
  RelExpr *pUpdateNode = buildDPBUpdate(getMinMaxMavName(), getSysDeltaName());
  RelExpr *pInsertNode = buildDPBInsertNodes(getMinMaxMavName());

  ItemExpr *ifCondition = 
    BinderUtils::buildPredOnCol(ITM_EQUAL, getVirtualGopColumnName(), GOP_MINMAX_RECOMPUTE_FROM_UPDATE, heap_);

  Union *ifNode = new(heap_) 
    Union(pUpdateNode, pInsertNode, NULL, ifCondition, REL_UNION,heap_, TRUE);

  RelRoot *ifRoot = new(heap_) RelRoot(ifNode);
  ifRoot->setEmptySelectList();

  RelExpr *tsjNode = new(heap_) Join(pLeftNode, ifRoot, REL_TSJ);
  RelRoot *pTopNode =  new(heap_) RelRoot(tsjNode);

  // Set an empty MvBindContext in the root. This way the Scan on the
  // base table will not be switched into a Scan on the log.
  MvBindContext * emptyContext = new(heap_) MvBindContext(heap_);
  pTopNode->setMvBindContext(emptyContext);

  pTopNode->setEmptySelectList();
  return pTopNode;
} // buildMinMaxRecomputationBlock

//----------------------------------------------------------------------------
// Make sure that the grouping columns are not projected upwards.
// For this we make sure that the grouping columns are removed from the group 
// by node and from the root select list on top.
void MinMaxOptimizedMavBuilder::fixGroupingColumns(RelRoot* pRoot) const
{
  RelExpr *pGroupByNode = pRoot;

  // Find the GroupBy node below the Mv select root.
  while(pGroupByNode != NULL  && 
        pGroupByNode->getOperatorType() != REL_GROUPBY)
    pGroupByNode = pGroupByNode->child(0);

  if (pGroupByNode == NULL)
    return; // there are no grouping columns: "select max(a) from t1;"

  CMPASSERT(REL_GROUPBY == pGroupByNode->getOperatorType());
  ItemExpr * pGroupingCols = 
    ((GroupByAgg*)pGroupByNode)->removeGroupExprTree();
  removeGroupingColsFromSelectList(pRoot);

  // what ever is under the group by node - add the selection pred to it.
  // this way the group by values are set only to what is needed
  // In case the groupBy columns have aliases, the predicate need to use the 
  // original column names, so use the 'ignoreAlias' parameter.
  ItemExpr *gbSelectionPred = buildGroupByColumnsPredicate(getSysDeltaName(), "", TRUE);
  pGroupByNode->child(0)->addSelPredTree(gbSelectionPred);
} // removeGroupingColumns

//----------------------------------------------------------------------------
// Remove the GroupBy columns from the select list of the MV select tree.
// After removing the GroupBy volumns from the GroupBy node, these columns
// can no longer be used in the select list above the GroupBy node.
void MinMaxOptimizedMavBuilder::removeGroupingColsFromSelectList(RelRoot* pRoot) const
{
  // Convert the root select list from an ItemExpr tree to a list.
  ItemExprList selectList(pRoot->removeCompExprTree(), heap_);

  // For each column in the select list.
  // Start from the end backwards, to avoid messing with the indexing.
  for (Int32 i = selectList.entries()-1; i>=0; i--)
  {
    ItemExpr *slCol = selectList[i];

    // Skip RenameCols to get to the actual column.
    CMPASSERT(slCol->getOperatorType() == ITM_RENAME_COL);
    RenameCol *renCol = (RenameCol *)slCol;
    const NAString& mvColName = renCol->getNewColRefName()->getColName();

    MVColumnInfo *mvColInfo = 
      getMvInfo()->getMVColumns().getMvColInfoByName(mvColName);
    CMPASSERT(mvColInfo != NULL);

    // Is it used as a group-by column?
    if (mvColInfo->getColType() != COM_MVCOL_GROUPBY)
      continue;

    // Yes - remove it from the select list.
    selectList.removeAt(i);
  }

  // Convert the shorter list back into an ItemExpr tree.
  ItemExpr *nonGroupedSelectList = selectList.convertToItemExpr();
  // Set it as the Select list.
  pRoot->addCompExprTree(nonGroupedSelectList);
}

// ===========================================================================
// ===========================================================================
// ===================  class  MultiDeltaMavBuilder  =========================
// ===========================================================================
// ===========================================================================

//----------------------------------------------------------------------------
// Build the refresh tree of a multi-delta refresh:
// 1. Build the Join Graph. This is the graph of join predicates between 
//    the tables, and takes into account RI constraints.
// 2. Find the optimal solution to travel the graph while using as many 
//    RI constraints as possible, in order to enable RI optimizations.
// 3. Build the Product Matrix according to the graph solution.
// 4. Find the relevant matrix rows according to the current activation phase.
// 5. Build the Delta tree according to the relevant matrix rows.
// 6. Build the rest of the refresh tree.
RelExpr *MultiDeltaMavBuilder::buildRefreshTree()
{
  mvSelectTree_ = getMvInfo()->buildMVSelectTree(); 

  // Steps 1-4 moved up to prepareProductMatrix(). 
  // supportPhases = TRUE, checkOnlyInserts = FALSE
  MultiDeltaRefreshMatrix *productMatrix = prepareProductMatrix(TRUE, FALSE);
  if (productMatrix == NULL)
    return NULL;

  isDuplicatesOptimized_ = productMatrix->isDuplicatesOptimized();

  // 5. Build the delta tree.
  // The raw DCB is the MV GroupBy fed by a union of all the products (each 
  // product reading a different set of logs according to the corresponding
  // matrix row.
  RelExpr *rawDeltaTree = buildRawDeltaCalculationTree(*productMatrix);
  if (bindWA_->errStatus())
    return NULL;

  // 6. Build the rest of the refresh tree.
  return buildTheMavRefreshTree(rawDeltaTree, NULL);
}

//----------------------------------------------------------------------------
// The "raw" delta calculation tree is the revised MV select tree, without
// any additions above the GroupBy node (these are added in 
// buildDeltaCalculationBlock()). This is the algorithm:
// 1. Get the MV select tree. 
// 2. Find the top most Join node below the GroupBy.
// 3. For each revevant row in the product matrix: 
// 3.1. copy the join product and have it read only the logs from the 
//      corresponding matrix row.
// 3.2. Build the Op expression for the product using the Op columns of the
//      logs, and the matrix row sign.
// 3.3. Connect the product to the tree using a Union node.
// 4. If duplicates can be optimized (when all deltas are insert-only)
//    Set the top Union node to a DISTINCT Union.
// 5. Put the originl GroupBy node at the top of the Union tree.
RelExpr *MultiDeltaMavBuilder::buildRawDeltaCalculationTree(
				const MultiDeltaRefreshMatrix& productMatrix)
{
  // 1. Get the tree for the MV select expression.
  RelExpr *mvSelectTop = getMvSelectTree();  

  // 2. Find the top Join below the root and GroupBy.
  RelExpr *topJoin = mvSelectTop;
  RelExpr *parentOfTopJoin = mvSelectTop;
  while (parentOfTopJoin->getOperatorType() != REL_GROUPBY &&
         topJoin->getOperatorType()         != REL_JOIN)
  {
    parentOfTopJoin = topJoin;
    topJoin = topJoin->child(0);
    CMPASSERT(topJoin != NULL);
  }

  // Put a root on top of the join. This is the basic product.
  RelRoot *rawJoinProduct = new(heap_) RelRoot(topJoin);

  // 3. For each relevant product matrix row
  RelExpr *topNode = NULL; 
  Int32 numOfRowsForThisPhase = productMatrix.getNumOfRowsForThisPhase();
  Int32 firstRowForThisPhase  = productMatrix.getFirstRowForThisPhase();
  for (Int32 rowNumber = firstRowForThisPhase; 
       rowNumber < firstRowForThisPhase + numOfRowsForThisPhase; 
       rowNumber++)
  {
    // 3.1, 3.2 Prepare the join product according to the matrix row.
    NABoolean isLastRow =
      rowNumber == firstRowForThisPhase + numOfRowsForThisPhase - 1;

    RelRoot *product =
      prepareProductFromMatrix(productMatrix, 
			       rawJoinProduct, 
			       rowNumber, 
			       isLastRow);

    bindJoinProduct(product, productMatrix.getRow(rowNumber)->isSignPlus());
    if (bindWA_->errStatus())
      return NULL;

    // 3.3 Add it to the Union tree.
    if (topNode == NULL)
      topNode = product;
    else
      topNode = new(heap_) Union(topNode, product, NULL, NULL, REL_UNION,
                                 CmpCommon::statementHeap(),TRUE);
  }

  // 4. If duplicates can be optimized (when all deltas are insert-only)
  //    Set the top Union node to a DISTINCT Union.
  //    topNode may not be a Union in case of an RI optimization.
  if (isDuplicatesOptimized_  && 
      topNode->getOperatorType() == REL_UNION)
  {
    Union *topUnion = (Union*) topNode;
    topUnion->setDistinctUnion();

    topNode = new (heap_)
	GroupByAgg(new (heap_) RelRoot(topUnion),
		   REL_GROUPBY, 
		   new (heap_) ColReference(new (heap_) ColRefName(TRUE, heap_)));

    if (mvSelectTop == parentOfTopJoin)
    {
      // This happens only for MAVs with no GroupBy (single row MAVs).
      // Here we need to insert another GroupBy node for the top level aggregates.
      // Otherwise, those aggregates will be pushed down into the previous GroupBy,
      // and it will no longer provide the DISTINCT result we need.
      topNode = new (heap_)
	  GroupByAgg(new (heap_) RelRoot(topNode),
		     REL_GROUPBY, 
		     NULL);
    }
  }

  // 5. Put the GroupBy node on top.
  parentOfTopJoin->child(0) = topNode;
  return mvSelectTop;
}

//----------------------------------------------------------------------------
// Prepare a single join product according to the product matrix row.
void MultiDeltaMavBuilder::bindJoinProduct(RelRoot  *product, NABoolean isSignPlus)
{
  // Bind the join product in a new bind scope.
  bindWA_->initNewScope();

  product->bindNode(bindWA_);
  if (bindWA_->errStatus())
    return;

  // Replace the multiple logs @OP expressions by a single @OP expression.
  prepareRetdescForUnion(product->getRETDesc(), isSignPlus);

  // Remove the bind scope we opened.
  bindWA_->removeCurrentScope();
}

//----------------------------------------------------------------------------
// Take the RETDesc of the root of the join product, and give it a canonic 
// structure: all the base table's user columns that are used by the MAV, no 
// special log columns (such as @EPOCH) and a single @OP column representing
// all the corresponding log rows. In order to get to this canonic structure 
// we need to:
// 1. Delete the special log columns from the RETDesc.
// 2. Remove from the RETDesc base columns that are not used by the MAV.
// 3. Replace the @OP  columns: Instead of an @OP column per log used, we 
//    need a single @OP column that is a multiplication of the table's @OP 
//    columns, and of the  product matrix row sign.
//    Example: say the join is on T1, T2, T3 and T4, and in this matrix
//    row T1 and T3 should scan their delta, and the sign is minus. The
//    @OP as well as @EPOCH, @IGNORE etc. should be deleted from the RETDesc, 
//    for both T1 and T3. A new column called @OP should be adde instead: 
//    (T1.@OP * T3.@OP * -1).
void MultiDeltaMavBuilder::prepareRetdescForUnion(RETDesc  *retDesc, NABoolean isSignPlus)
{
  CMPASSERT(retDesc != NULL);
  ColRefName opName(getVirtualOpColumnName());

  ItemExpr *newOpExpr = NULL;

  if (isDuplicatesOptimized_)
  {
    // When duplicates are optimized, it must be an insert-only refresh.
    // All the @Op columns and also the sign must be 1,
    // So the result is always 1.
    newOpExpr = new(heap_) SystemLiteral(1);
  }

  // Get all the columns in the RETDesc.
  const ColumnDescList *userColumns = retDesc->getColumnList();
  // We are beginning this loop from the last element, because we don't want
  // deletions from the RETDesc to affect the location of colums we have not 
  // yet checked. This is why 'i' should be a signed integer (not CollIndex).
  for (Int32 i=userColumns->entries()-1; i>=0; i--)
  {
    ColumnDesc *colDesc = userColumns->at(i);
    const ColRefName& colRefName = colDesc->getColRefNameObj();
    const NAString& colName = colRefName.getColName();

    if (collectOpExpressions(retDesc, colDesc, newOpExpr))
    {
      retDesc->delColumn(bindWA_, colRefName, USER_COLUMN);
      continue;
    }

    // If this is a control column (@EPOCH, @IGNORE etc.) remove it.
    if (!strncmp(colName.data(), COMMV_CTRL_PREFIX , sizeof(COMMV_CTRL_PREFIX)-1))
    {
      retDesc->delColumn(bindWA_, colRefName, USER_COLUMN);
      continue;
    }

    // Filter out base columns that are not used by the MV.
    // This solves the problem of columns that are not loggable.
    ItemExpr *colExpr = colDesc->getValueId().getItemExpr();
    BaseColumn *baseCol = NULL;
    if (colExpr->getOperatorType() == ITM_BASECOLUMN)
    { 
      baseCol = (BaseColumn *)colExpr;
    }
    else if (colExpr->getOperatorType() == ITM_VALUEIDUNION)
    {
      // This must be the Union between the IUD log and the range log join.
      // Lets pick the column from the IUD log.
      ItemExpr *iudLogCol = 
	((ValueIdUnion*)colExpr)->getLeftSource().getItemExpr();
      if (iudLogCol->getOperatorType() == ITM_BASECOLUMN)
        baseCol = (BaseColumn *)iudLogCol;
    }

    if (baseCol == NULL)
      continue;

    const QualifiedName& tableName = 
      baseCol->getTableDesc()->getCorrNameObj().getQualifiedNameObj();

    const MVUsedObjectInfo *usedObject = 
      getMvInfo()->findUsedInfoForTable(tableName);
    CMPASSERT(usedObject != NULL);  

    // Find if the column is used by this MV.
    // If this column is from a base table, use the column position directly.
    // If it is from the IUD log, use the column name to find the position in
    // the base table.
    NABoolean isColumnUsed;
    if (baseCol->getTableDesc()->getCorrNameObj().getSpecialType() != 
        ExtendedQualName::NORMAL_TABLE)
      isColumnUsed = usedObject->isUsedColumn(baseCol->getColName());
    else
      isColumnUsed = usedObject->isUsedColumn(baseCol->getColNumber());

    // If the column is not used - remove it from the RETDesc.
    if (!isColumnUsed)
      retDesc->delColumn(bindWA_, colRefName, USER_COLUMN);
  }

  if (newOpExpr == NULL)
  {
    // We did not find any Op columns in the user column list, look
    // for them in the system column list.
    const ColumnDescList *systemColumns = retDesc->getSystemColumnList();
    for (Int32 j=systemColumns->entries()-1; j>=0; j--)
    {
      ColumnDesc *colDesc = systemColumns->at(j);
      const ColRefName& colRefName = colDesc->getColRefNameObj();
      const NAString& colName = colRefName.getColName();      
      if (collectOpExpressions(retDesc, colDesc, newOpExpr))
      {
        retDesc->delColumn(bindWA_, colRefName, SYSTEM_COLUMN);
        continue;
      }
    }
  }

  CMPASSERT(newOpExpr != NULL);

  if (!isSignPlus)
  {
    // If this row is with a minus sign, negate the new @OP expression.
    newOpExpr = new(heap_) 
      BiArith(ITM_TIMES, newOpExpr, new(heap_) SystemLiteral(-1));
  }

  // Bind the new OP expression
  newOpExpr->bindNode(bindWA_);
  if (bindWA_->errStatus())
    return;

  // And add it to the RETDesc.
  retDesc->addColumn(bindWA_, opName, newOpExpr->getValueId());
}

// This method returns TRUE if the columnDesc parameter is an @OP column
// That needs to be deleted from the RETDesc.
// newOpExpr is an Input/Output parameter that returns the updated @Op exprtession.
NABoolean MultiDeltaMavBuilder::collectOpExpressions(RETDesc           *retDesc,
	    				             const ColumnDesc  *columnDesc,
				                     ItemExpr	      *&newOpExpr)
{
  ColRefName opName(getVirtualOpColumnName());
  const ColRefName& colRefName = columnDesc->getColRefNameObj();
  const NAString& colName = colRefName.getColName();

  // If this is the table's @OP column, add it to the new @OP expression
  // and then remove it from the RETDesc.
  if (colName == opName.getColName())
  {
    // No need for the calculation is duplicates are optimized.
    if (isDuplicatesOptimized_ == FALSE)
    {
      // Get the bound expression for this @OP
      ItemExpr *thisOpCol = columnDesc->getValueId().getItemExpr();
      // The new @OP expression is the multiplication of all the deltas
      // @OP columns.
      if (newOpExpr == NULL)
        newOpExpr = thisOpCol;
      else
        newOpExpr = new(heap_) BiArith(ITM_TIMES, newOpExpr, thisOpCol);
    }

    return TRUE;
  }

  return FALSE;
}

//----------------------------------------------------------------------------
// For MAVs, multi-delta optimization is not supported for base tables
// that have a SYSKEY column. 
NABoolean MultiDeltaMavBuilder::avoidMultiDeltaOptimization()
{
  // Look for a SYSKEY column in tables that have a non-empty delta.
  const DeltaDefinitionPtrList *deltaDefList = getDeltaDefList();
  for (CollIndex i=0; i<deltaDefList->entries(); i++)
  {
    DeltaDefinition *deltaDef = deltaDefList->at(i);
    QualifiedName *baseTableName = deltaDef->getTableName();
    CorrName *baseCorrName = new CorrName(*baseTableName);
    NATable *baseNATable = bindWA_->getNATable(*baseCorrName);
    const NAFileSet *ci = baseNATable->getClusteringIndex();
    const NAColumnArray &indexCols = ci->getIndexKeyColumns();

    if (indexCols.getColumn("SYSKEY") != NULL)
      return TRUE;
  }  

  // Disable multi-delta optimization if table expression is present   
  RelExpr *mvSelectTop = getMvSelectTree();
  CMPASSERT( NULL != mvSelectTop );
  if( isTableExpressionPresent( mvSelectTop ) )
  {
     return TRUE;
  } 

  return FALSE;   
}
//----------------------------------------------------------------------------
// Method: MultiDeltaMavBuilder::isTableExpressionPresent
//
// Description:
//    Determine whether this tree contains a Rename node between
//    a GroupBy and a Join.
//
// Called by: MultiDeltaMavBuilder::avoidMultiDeltaOptimization
//     This is called as a way to disable multi-delta optimization 
//     when a table expression is present.
//
// Parameters:
//     RelExpr *currentNode
//           - call with the root of the tree.  
// Return:
//     NABoolean
//           - return TRUE if the table expr is present.  This condition
//             becomes TRUE when a Rename node is found between a GroupBy
//             and a Join.
//
NABoolean MultiDeltaMavBuilder::isTableExpressionPresent(RelExpr *currentNode)
{  
    RelExpr *rootNode = currentNode;
    NABoolean groupByFound = TRUE;

    // Find the GroupBy node.
    while (REL_GROUPBY != currentNode->getOperatorType() )
    {
      currentNode = currentNode->child(0);

      // couldn't find a group by, start over looking for a 
      // rename before the join
      if( NULL == currentNode )
      {
        currentNode = rootNode;
        groupByFound = FALSE;
        break;
      }
    }

    // If didn't find a GroupBy, look for a REL_ROOT with aggregate functions
    while( (!groupByFound) && 
         !( REL_ROOT == currentNode->getOperatorType() &&
             ((RelRoot*)currentNode)->getCompExprTree()->containsOpType(ITM_ANY_AGGREGATE) ) )
    {
      currentNode = currentNode->child(0);
      CMPASSERT( currentNode != NULL );
    }

    // Is there a RenameTable node before the Join?
    while (REL_JOIN         != currentNode->getOperatorType() &&
           REL_RENAME_TABLE != currentNode->getOperatorType() )
    {
      currentNode = currentNode->child(0);
      CMPASSERT(currentNode != NULL);
    }
   
    if (REL_JOIN == currentNode->getOperatorType() )
      return FALSE;
    else
      return TRUE;
}

// ===========================================================================
// ===========================================================================
// ===================  class  PipelinedMavBuilder   =========================
// ===========================================================================
// ===========================================================================

PipelinedMavBuilder::PipelinedMavBuilder(const CorrName&      mvName,
					 MVInfoForDML        *mvInfo,
					 Refresh	     *refreshNode,
					 NABoolean	      isPipelined,
					 const QualifiedName *pipeliningSource,
					 BindWA              *bindWA)
  : MavBuilder(mvName, mvInfo, refreshNode, isPipelined, bindWA)
{
  // Pipelined builders do not use the delta definition list of the Refresh
  // class (that was initialized in the MvRefreshBuilder Ctor).
  // Instead, the input is read from a pipelining refresh tree, that we
  // cannot predict the type of its outputs.
  // Therefore, we use a "NO DE" type delta definition for this builder.
  DeltaDefinition *noDeDeltaDef = new(heap_)
    DeltaDefinition((QualifiedName *)pipeliningSource);

  DeltaDefinitionPtrList *newDeltaDefList = new(heap_) 
    DeltaDefinitionPtrList(heap_);

  newDeltaDefList->insert(noDeDeltaDef);

  // Override the value set in the base class.
  setDeltaDefList(newDeltaDefList);
}

// ===========================================================================
// ==== Methods for connecting two pipelined refresh trees.
// ==========================================================================

//----------------------------------------------------------------------------
// called by Scan::bindnode() for building the next level refresh tree and
// connecting it as a "view" that is projecting the data.
// The pipelined data should apear to the top MV refresh as if it was read
// from the bottom MV IUD log, instead of produced by refreshing it.
RelExpr *PipelinedMavBuilder::buildAndConnectPipeliningRefresh(RelExpr *pipeliningTree)
{
  CMPASSERT(pipeliningTree != NULL);

  // Join with the GOP_TABLE to split update rows to two rows each.
  RelExpr *topNode = buildJoinWithTupleList(pipeliningTree);
  // Prepare the final select list to be as if read from the IUD log
  // of the bottom MV.
  RelRoot *topRoot = buildRenameToLog(topNode);

  return topRoot;
}

//----------------------------------------------------------------------------
// Builds a "table" named "GOP_TABLE" using a TupleList node, with the 
// following contents:	        Join
//   GroupOp   | Op           /      \    
//  -----------|-----   topNode    RenameTable (to GOP_TABLE(GroupOp, Op))
//  GOP_INSERT |  1                     |
//  GOP_DELETE | -1                  RelRoot   (Open a new bind scope)
//  GOP_UPDATE | -1     		|
//  GOP_UPDATE |  1		    TupleList
// This "table" is then joined with topNode (the pipelining refresh tree)
// with the join predicate (@GOP = GroupOp). This simulates reading from
// the MAV IUD log, where Update operations have two rows each.
RelExpr *PipelinedMavBuilder::buildJoinWithTupleList(RelExpr *topNode)
{
  const NAString gopTableName(getGopTableName());
  const NAString gopCol1Name (getGopCol1Name());
  const NAString gopCol2Name (getGopCol2Name());

  // Construct the four rows
  ItemExpr *tupleRow1 = new(heap_) 
    Convert(new(heap_)
      ItemList(new(heap_) SystemLiteral(GOP_INSERT),
	       new(heap_) SystemLiteral(1)));
  ItemExpr *tupleRow2 = new(heap_) 
    Convert(new(heap_)
      ItemList(new(heap_) SystemLiteral(GOP_DELETE),
	       new(heap_) SystemLiteral(-1)));
  ItemExpr *tupleRow3 = new(heap_)
    Convert(new(heap_)
      ItemList(new(heap_) SystemLiteral(GOP_UPDATE),
	       new(heap_) SystemLiteral(-1)));
  ItemExpr *tupleRow4 = new(heap_) 
    Convert(new(heap_)
      ItemList(new(heap_) SystemLiteral(GOP_UPDATE),
	       new(heap_) SystemLiteral(1)));

  // Construct the tuple expression from the four rows
  ItemExpr *tupleExpr = new(heap_) 
    ItemList(tupleRow1, 
	     new(heap_) ItemList(tupleRow2,
				 new(heap_) ItemList(tupleRow3, tupleRow4)));

  // Construct the TupleList node with the tuple expression.
  RelExpr *tupleNode = new(heap_) TupleList(tupleExpr);
  // Build a root node to open a new scope.
  RelExpr *rightTop = new(heap_) RelRoot(tupleNode);

  // Now build the rename node.
  CorrName gopCorrName(gopTableName);
  ColRefName *col1RefName = new(heap_) ColRefName(gopCol1Name, heap_);
  ColRefName *col2RefName = new(heap_) ColRefName(gopCol2Name, heap_);
  ItemExpr *renExpr = new(heap_) 
    ItemList(new(heap_) RenameCol(NULL, col1RefName),
	     new(heap_) RenameCol(NULL, col2RefName));

  rightTop = new(heap_) 
    RenameTable(TRUE, rightTop, gopTableName, renExpr, heap_);

  // Construct the join predicate (@GOP = GOP_TABLE.GroupOp)
  ColReference *leftGopCol = new(heap_) 
    ColReference(new(heap_) ColRefName(getVirtualGopColumnName(), heap_));
  ColReference *rightGopCol = new(heap_) 
    ColReference(new(heap_) ColRefName(gopCol1Name, gopTableName, heap_));
  ItemExpr *joinPred = new(heap_) 
    BiRelat(ITM_EQUAL, leftGopCol, rightGopCol);

  // Join the resulting TupleList node with topNode
  // A REL_JOIN instead of a TSJ does not work for pipelined multi-txn refresh.
  Join *tsjNode = new(heap_) Join(topNode, rightTop, REL_TSJ, joinPred);
  tsjNode->setTSJForWrite(TRUE);

  return tsjNode;
}

//----------------------------------------------------------------------------
// Used to connect two refresh trees of pipelined MVs.
// The results of the bottom MV are processed and renamed to be as if just
// read off the log of the bottom MV, for the refresh of the top MV.
// For each of the base table columns, the correct value needs to be selected 
// between the 3 sets: DELTA, MAV and CALC. We decide which one according to
// the GOP column. Possible combinations are according to the GOP_TABLE table:
//   GroupOp   | Op  | Operation         | Use
//  -----------|-----|-------------------|------------
//  GOP_DELETE | -1  | Delete            | SYS_MAV
//  GOP_INSERT |  1  | Insert            | SYS_DELTA
//  GOP_UPDATE | -1  | Delete of Update  | SYS_MAV   (subtract previous value)
//  GOP_UPDATE |  1  | Insert of Update  | SYS_CALC  (add new calculated value)
//
RelRoot *PipelinedMavBuilder::buildRenameToLog(RelExpr *topNode)
{
        CorrName baseCorrName (*getPipeliningSource(), heap_);
  const CorrName deltaCorrName(getSysDeltaName(), heap_);
  const CorrName calcCorrName (getSysCalcName(), heap_);
  const CorrName mavCorrName  (getSysMavName(), heap_);
  const NAString gopCol1Name  (getGopCol1Name());
  const NAString gopCol2Name  (getGopCol2Name());
  const CorrName gopCorrName  (getGopTableName());
  const NAColumnArray &baseColumns = 
    bindWA_->getNATable(baseCorrName)->getNAColumnArray();

  // Rename GOP_TABLE.OP to <log_name>.@OP
  ColRefName *gopTableGopColName = new(heap_)
    ColRefName(gopCol1Name, gopCorrName, heap_);
  ColRefName *gopTableOpColName = new(heap_)
    ColRefName(gopCol2Name, gopCorrName, heap_);

  // Start the select list with the OP column. The other columns are added 
  // in the loop below.
  ItemExpr *selectList = new(heap_)
    RenameCol(new(heap_) ColReference(gopTableOpColName),
  	      new(heap_) ColRefName(getVirtualOpColumnName(), heap_));

  // The expression for column 'col' is:
  // CASE WHEN (GOP_TABLE.OP      = -1)         THEN SYS_MAV.col   - Delete
  //      WHEN (GOP_TABLE.GROUPOP = GOP_INSERT) THEN SYS_DELTA.col - Insert
  //      ELSE                                       SYS_CALC.col  - Insert of Update
  // Since the two conditions don't use the column itself, construct them 
  // before the loop, and copy them for each column.
  ItemExpr *opCondition =
    BinderUtils::buildPredOnCol(ITM_EQUAL, gopCol2Name, -1, heap_);

  ItemExpr *gopCondition = 
    BinderUtils::buildPredOnCol(ITM_EQUAL, gopCol1Name, GOP_INSERT, heap_);

  for (CollIndex i=0; i<baseColumns.entries(); i++)
  {
    const NAString& newColName = baseColumns[i]->getColName();

    // Skip the SYSKEY column.
    if (newColName == "SYSKEY") 
      continue;

    ItemExpr *mavColRef = new(heap_) ColReference(new(heap_)  // SYS_MAV.col
      ColRefName(newColName, mavCorrName, heap_));
    ItemExpr *deltaColRef = new(heap_) ColReference(new(heap_)// SYS_DELTA.col
      ColRefName(newColName, deltaCorrName, heap_));
    ItemExpr *calcColRef = new(heap_) ColReference(new(heap_) // SYS_CALC.col
      ColRefName(newColName, calcCorrName, heap_));

    // This is the conditional expression of the column.
    ItemExpr *colExpr = new(heap_)
      Case(NULL, new(heap_)
	IfThenElse(opCondition->copyTree(heap_),
	           mavColRef->copyTree(heap_),
		   new(heap_) IfThenElse(gopCondition->copyTree(heap_), 
					 deltaColRef->copyTree(heap_), 
					 calcColRef->copyTree(heap_)) ) );

    // Give it a name: <base table name>.col
    RenameCol *renCol = new(heap_)
      RenameCol(colExpr, new(heap_)
	ColRefName(newColName, baseCorrName, heap_));

    // Add it to the select list.
    selectList = new(heap_) ItemList(selectList, renCol);
  }

  // Build the root node using the select list.
  RelRoot *topRoot = new(heap_) 
    RelRoot(topNode, REL_ROOT, selectList);

  return topRoot;
}  // PipelinedMavBuilder::buildRenameToLog()

// ===========================================================================
// ===========================================================================
// ===================  class  MultiDeltaRefreshMatrixRow    =================
// ===========================================================================
// ===========================================================================

//----------------------------------------------------------------------------
// Ctor for creating a matrix row that is initialized to all SCAN_TABLE 
// except the last table that is initialized to SCAN_DELTA. This Ctor is used 
// for the first row of the matrix, and for the last row of every matrix
// doubling.
MultiDeltaRefreshMatrixRow::MultiDeltaRefreshMatrixRow(Int32 length, 
						       Int32 maxLength, 
						       CollHeap *heap)
  : sign_(SIGN_PLUS),
    currentLength_(length),
    maxLength_(maxLength),
    tableScanTypes_(heap, maxLength),
    heap_(heap)
{
  CMPASSERT(length <= maxLength);
  initArray();
  for (Int32 i=0; i<length-1; i++)
    tableScanTypes_[i] = SCAN_TABLE;
  tableScanTypes_[length-1] = SCAN_DELTA;
}

//----------------------------------------------------------------------------
// Copy Ctor.
MultiDeltaRefreshMatrixRow::MultiDeltaRefreshMatrixRow(const MultiDeltaRefreshMatrixRow& other)
  : sign_(other.sign_),
    currentLength_(other.currentLength_),
    maxLength_(other.maxLength_),
    tableScanTypes_(other.tableScanTypes_, other.heap_),
    heap_(other.heap_)
{
}

//----------------------------------------------------------------------------
void MultiDeltaRefreshMatrixRow::initArray()
{
  // The array will assert on accessing an un-initialized element, so we 
  // initialize maxLength elements to SCAN_TABLE.
  for (Int32 i=0; i<maxLength_; i++)
    tableScanTypes_.insert(i, SCAN_TABLE);
}

//----------------------------------------------------------------------------
// Used after creating a new row using the copy Ctor, while doubling the 
// matrix. Flip the sign of the row, and the type of the last table so far.
void MultiDeltaRefreshMatrixRow::flipSignAndLastTable()
{
  if (sign_ == SIGN_MINUS)
    sign_ = SIGN_PLUS;
  else
    sign_ = SIGN_MINUS;

  scanType& lastTable = tableScanTypes_[currentLength_-1];
  if (lastTable == SCAN_TABLE)
    lastTable = SCAN_DELTA;
  else
    lastTable = SCAN_TABLE;
}

//----------------------------------------------------------------------------
// Add a new column (another table) to the row, and set its type.
void MultiDeltaRefreshMatrixRow::addColumn(scanType type)
{
  CMPASSERT(currentLength_ < maxLength_);
  tableScanTypes_[currentLength_] = type;
  currentLength_++;
}

#ifndef NDEBUG
// Exclude from coverage testing - Debugging code
void MultiDeltaRefreshMatrixRow::print(FILE* ofd,
	     const char* indent,
	     const char* title) const
{
  char sign = (isSignPlus() ? '+' : '-');
  fprintf(ofd, "%c ", sign);
  for (Int32 i=0; i<currentLength_; i++)
    if (isScanOnDelta(i))
      fprintf(ofd, "d  ");
    else
      fprintf(ofd, "T  ");

  fprintf(ofd, "\n");
}

void MultiDeltaRefreshMatrixRow::display() const
{
  print();
}
#endif

// ===========================================================================
// ===========================================================================
// ===================  class  MultiDeltaRefreshMatrix    ====================
// ===========================================================================
// ===========================================================================

//----------------------------------------------------------------------------
// Construct the matrix from the join graph solution, and add the first row.
MultiDeltaRefreshMatrix::MultiDeltaRefreshMatrix(Int32	        maxNumOfRows,
						 MVJoinGraph   *joinGraph,
						 CollHeap      *heap)
  : numOfRows_(1),
    currentRowLength_(0),
    maxRowLength_(0),
    thisPhase_(0),
    isLastPhase_(TRUE),
    firstRowForThisPhase_(0),
    numOfRowsForThisPhase_(0),
    tableIndexMapping_(heap),
    heap_(heap),
    theMatrix_(heap, maxNumOfRows),
    TooManyDeltasError_(FALSE),
    isDuplicatesOptimized_(joinGraph->isInsertOnlyRefresh())
{
  // =========================================================
  // == These are the parameters of the heuristic function. ==
  // =========================================================
  maxJoinSizeForSinglePhase_ =
    CmpCommon::getDefaultLong(MV_REFRESH_MDELTA_MAX_JOIN_SIZE_FOR_SINGLE_PHASE);
  minJoinSizeForSingleProductPerPhase_ =
    CmpCommon::getDefaultLong(MV_REFRESH_MDELTA_MIN_JOIN_SIZE_FOR_SINGLE_PRODUCT_PHASE);
  phaseSizeForMidRange_ =
    CmpCommon::getDefaultLong(MV_REFRESH_MDELTA_PHASE_SIZE_FOR_MID_RANGE);
  maxDeltasThreshold_ =
    CmpCommon::getDefaultLong(MV_REFRESH_MDELTA_MAX_DELTAS_THRESHOLD);

  // Find the solution for the join graph.
  CMPASSERT(joinGraph != NULL);
  const MVJoinGraphSolution& graphSolution = joinGraph->getSolution();
  const ARRAY(Lng32)& theRoute = graphSolution.getRoute();
  // Get the mapping between the route tableIndex to the MVInfo used tables.
  joinGraph->getRouteOfSolution(tableIndexMapping_);

  maxRowLength_ = theRoute.entries();
 
  Int32 firstNonEmptyDelta = 0; 
  while (joinGraph->getTableObjectAt(firstNonEmptyDelta)->isEmptyDelta())
    firstNonEmptyDelta++;
  currentRowLength_ = firstNonEmptyDelta+1;

  // Initialize the matrix to NULL pointers to avoid assert on accessing
  // an un-initialized element.
  for (Int32 i=0; i<maxNumOfRows; i++)
    theMatrix_.insert(i, NULL);

  // Create the first matrix row.
  MultiDeltaRefreshMatrixRow *firstRow = new(heap)
    MultiDeltaRefreshMatrixRow(currentRowLength_, maxRowLength_, heap);

  // And insert it into the matrix.
  theMatrix_[0] = firstRow;
}

//----------------------------------------------------------------------------
MultiDeltaRefreshMatrix::~MultiDeltaRefreshMatrix()
{
  // Delete all the matrix rows.
  for (CollIndex i=0; i<theMatrix_.entries(); i++)
    delete theMatrix_[i];
}

//----------------------------------------------------------------------------
// Add a column (a table) table to the matrix. readDelta is FALSE if the
// table has an empty delta, or if the delta was optimized away.
void MultiDeltaRefreshMatrix::addTable(NABoolean readDelta)
{
  // Add a column to the matrix (initialized to SCAN_TABLE).
  for (Int32 i = 0; i<numOfRows_; i++)
    theMatrix_[i]->addColumn();
  currentRowLength_++;

  // If no delta, no need to double the matrix.
  if (!readDelta)
    return;

  if (isDuplicatesOptimized_ == FALSE)
  {
    // Double the matrix: For all rows currently in the matrix:
    for (Int32 j = 0; j<numOfRows_; j++)
    {
      // Copy the row to a new row.
      MultiDeltaRefreshMatrixRow *matrixRow = new(heap_)
	MultiDeltaRefreshMatrixRow(*theMatrix_[j]);
      // Flip its sign and the type of the last table from SCAN_TABLE 
      // to SCAN_DELTA.
      matrixRow->flipSignAndLastTable();
      // Insert it at the end of the matrix.
      theMatrix_[numOfRows_+j] = matrixRow;
    }
    // We have doubled the number of rows in the matrix.
    numOfRows_ *= 2;
  }

  // Now add the last row - All full with SCAN_TABLE except the last 
  // table with SCAN_DELTA.
  MultiDeltaRefreshMatrixRow *lastRow = new(heap_)
    MultiDeltaRefreshMatrixRow(currentRowLength_, maxRowLength_, heap_);
  theMatrix_[numOfRows_] = lastRow;
  numOfRows_++;
}

//----------------------------------------------------------------------------
// Set the phase given by the Refresh utility.
// This determined which of the matrix's rows will be used in this activation.
void MultiDeltaRefreshMatrix::setThisPhase(Int32 phase)
{
  thisPhase_ = phase;

  calculatePhases();

  // Sanity check - Our first row should be within the array size.
  CMPASSERT(firstRowForThisPhase_ < numOfRows_);

  // Do we need any more phases?
  isLastPhase_ = 
    (firstRowForThisPhase_ + numOfRowsForThisPhase_ >= numOfRows_);
}

//----------------------------------------------------------------------------
// This is a heuristic function for determining the division of the work
// to phases. Update the phase related data members accordingly.
//----------------------------------------------------------------------------
void MultiDeltaRefreshMatrix::calculatePhases()
{
  if ( (isPhasesSupported() == FALSE)    &&
       (numOfRows_ > maxDeltasThreshold_) &&
       (isDuplicatesOptimized_ == FALSE)    )
  {
    TooManyDeltasError_ = TRUE;
    return;
  }

  // Small joins execute in a single phase.
  if ( (isPhasesSupported() == FALSE)                    ||
       (currentRowLength_ <= maxJoinSizeForSinglePhase_)  || 
       (isDuplicatesOptimized_ == TRUE)                     )
  {
    firstRowForThisPhase_  = 0;
    numOfRowsForThisPhase_ = numOfRows_;
    return;
  }

  // Big joins execute a single product at a time.
  if (currentRowLength_ >= minJoinSizeForSingleProductPerPhase_)
  {
    firstRowForThisPhase_  = thisPhase_;
    numOfRowsForThisPhase_ = 1;
    return;
  }

  // Mid-range joins are done a batch at a time.
  firstRowForThisPhase_ = thisPhase_ * phaseSizeForMidRange_;
  Int32 numberOfRowsLeft  = numOfRows_ - firstRowForThisPhase_;
  // The size of this phase is min(numberOfRowsLeft, phaseSizeForMidRange)
  if (numberOfRowsLeft > phaseSizeForMidRange_)
    numOfRowsForThisPhase_ = phaseSizeForMidRange_;
  else 
    numOfRowsForThisPhase_ = numberOfRowsLeft;
}

//----------------------------------------------------------------------------
const MultiDeltaRefreshMatrixRow *MultiDeltaRefreshMatrix::getRow(Int32 i) const
{
  CMPASSERT(i < numOfRows_);
  return theMatrix_[i];
}

#ifndef NDEBUG
// Exclude from coverage testing - Debugging code
void MultiDeltaRefreshMatrix::print(FILE* ofd,
	     const char* indent,
	     const char* title) const
{
  fprintf(ofd, "  Product Matrix:\n   Sign ");
  for (Int32 i=0; i<currentRowLength_; i++)
    fprintf(ofd, "%3d", getTableIndexFor(i));
  fprintf(ofd, "\n");

  for (Int32 j=0; j<numOfRows_; j++)
  {
    fprintf(ofd, "  %3d ) ", j);
    getRow(j)->print(ofd, "    ");
  }

  fprintf(ofd, "For phase %d, doing rows %d to %d.\n",
          thisPhase_, firstRowForThisPhase_, firstRowForThisPhase_+numOfRowsForThisPhase_-1);
  fprintf(ofd, "\n");
}

void MultiDeltaRefreshMatrix::display() const
{
  print();
}
#endif


