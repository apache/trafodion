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
* File:         MvLog.cpp
* Description:  implementation of the MVLOG command
* Created:      09/25/2000
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "MvLog.h"

#include "BindWA.h"
#include "NATable.h"
#include "PartFunc.h"
#include "ItemOther.h"
#include "ItemLog.h"
#include "ItemFunc.h"
#include "AllRelExpr.h"
#include "ChangesTable.h"
#include "ComMvAttributeBitmap.h"

// MVLOG command is not supported

const MvLogInternalNames MvLog::internalNames_;

const char MvLogInternalNames::beginRangeSuffix_[] = "_BEGIN";
const char MvLogInternalNames::endRangeSuffix_[]   = "_FIRST";
const char MvLogInternalNames::brSuffix_[]         = "_BR";
const char MvLogInternalNames::erSuffix_[]         = "_ER";
const char MvLogInternalNames::endSuffix_[]        = "_END";

const char MvLogInternalNames::finalResultName_[]  = "TOP_NAME";
const char MvLogInternalNames::tupleListName_[]    = "TUPLE_LIST";
const char MvLogInternalNames::tupleListRoot1_[]   = "TUPLE_LIST_P1";
const char MvLogInternalNames::tupleListRoot2_[]   = "TUPLE_LIST_P2";
const char MvLogInternalNames::getFirstRoot_[]     = "FIRST1_SCAN";
const char MvLogInternalNames::brIsGreater_[]      = "BR_IS_GREATER";
const char MvLogInternalNames::erIsLess_[]         = "ER_IS_LESS";
const char MvLogInternalNames::simpleRangeTuple_[] = "SIMPLE_RANGE_TUPLE";


//----------------------------------------------------------------------------
MvLog::MvLog(const QualifiedName *tableName,
	     const ItemExpr *columnNames,
	     ItemExpr *rangeBegin,
	     ItemExpr *rangeEnd,
	     CollHeap *oHeap)
  : BinderOnlyNode(REL_MVLOG, oHeap),
    tableName_  (tableName),
    pColumnNamesItem_(columnNames),
    pBeginRange_ (rangeBegin),
    pEndRange_   (rangeEnd),
    isRangePartitioned_(TRUE),
    pBeginRangeExprList_(pBeginRange_, oHeap),
    pEndRangeExprList_(pEndRange_, oHeap),
    pBindWA_	 (NULL),
    ciBoundries_(oHeap),
    tupleListPhase1CorrName_(getNames().getTupleListRoot1(), oHeap),
    tupleListPhase2CorrName_(getNames().getTupleListRoot2(), oHeap),
    tupleListPhase3CorrName_(getNames().getTupleListName(), oHeap), 
    tupleListCorrName_(getNames().getTupleListName(), oHeap),
    first1NodeCorrName_(getNames().getGetFirstRoot(), oHeap),
    emptyCorrName_("", oHeap),
    pRelevantCiPositionsList_(oHeap),
    mvLogColNamesList_(oHeap),
    baseTableCiNamesList_(oHeap),
    nonCiColsNamesList_(oHeap),
    nonCiColsExprList_(oHeap),
    beginColNamesList_(oHeap),
    endColNamesList_(oHeap),
    first1ColNamesList_(oHeap),
    brColNamesList_(oHeap),
    erColNamesList_(oHeap),
    colsMinDefualtValList_(oHeap),
    colsMaxDefualtValList_(oHeap),
    pDirectionVector_(NULL),
    pNaTable_(NULL),
    heap_(oHeap)
{
}

//----------------------------------------------------------------------------
MvLog::~MvLog() 
{
}

//----------------------------------------------------------------------------
void MvLog::cleanupBeforeSelfDestruct()
{
  delete tableName_;
  delete pColumnNamesItem_;
  delete pBeginRange_;
  delete pEndRange_;

  // Do NOT delete pDirectionVector_! It is used by constructed expressions.
}

//----------------------------------------------------------------------------
const NAString MvLog::getText() const
{
  return "mvlog";
}

//----------------------------------------------------------------------------
RelExpr * MvLog::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  CMPASSERT(FALSE); // not used.
  return NULL;
}

//----------------------------------------------------------------------------
void MvLog::addLocalExpr(LIST(ExprNode *) &xlist,
			      LIST(NAString) &llist) const
{
  RelExpr::addLocalExpr(xlist,llist);
}

//----------------------------------------------------------------------------
// The bindNode() method builds the RelExpr tree that does the work, binds it
// and return it instead of itself.
// The schematic tree that is built is as follows:
//                        TSJ
//                      /     \
//                 Read        Root
//                 Epoch        |
//                 Block       TSJ
//                           /     \
//                       Rename      Insert
//                      TOP_NAME     Log
//                         |         Block
//                        Root
//                         |
//                       Union
//                      /     \
//                   Root      EndRange
//                    |        Tuple
//                   TSJ       Block
//                 /     \
//              Tuple     GetFirst
//              List      Block
//              Block
//
// Each of the 5 blocks is described in the method tht builds it.
//
RelExpr *MvLog::bindNode(BindWA *bindWA)
{
  pBindWA_  = bindWA;

  if (initializeInternalDataStructures() == FALSE)
    return NULL;

  RelExpr * topNode = NULL;
  if (isRangePartitioned_)
  {
    // Prepare the lists of partition boundary values.
    constructBoundriesList();

    topNode = buildTupleListBlock();
    topNode = buildJoinWithGetFirstBlock(topNode);
    topNode = buildUnionWithEndRangeTupleList(topNode);
  }
  else
  {
    topNode = buildSimpleRangeTuple();
  }
  topNode = buildFinalSelectList(topNode);
  topNode = buildInsertIntoLog(topNode);
  topNode = buildScanEpochFromTable(topNode);

  // Make sure no outputs are pipelined out.
  RelRoot *topRoot = new(heap_) RelRoot(topNode);
  topRoot->setEmptySelectList();

  // Bind the constructed tree.
  RelExpr *mvLogTree = topRoot->bindNode(bindWA);

  // Delete all data members before saying goodbye.
  cleanupBeforeSelfDestruct();

  // Return the bound MVLog tree instead of this.
  return mvLogTree;
} // MvLog::bindNode

//----------------------------------------------------------------------------
// Returns FALSE if an error was encountered.
NABoolean MvLog::initializeInternalDataStructures()
{
  CorrName corrName(*tableName_);
  pNaTable_ = pBindWA_->getNATable(corrName, FALSE);  
  if (pBindWA_->errStatus())
    return FALSE;

  if (!isTableValidForMvLog())
    return FALSE;

  isRangePartitioned_ = isTableRangePartitioned();
  pDirectionVector_ = 
    BinderUtils::buildClusteringIndexDirectionVector(pNaTable_, heap_);

  // Prepare the lists of column names (all columns, CI columns etc.).
  buildBaseTableColumnNamesLists();
  if (pBindWA_->errStatus())
    return FALSE;

  checkMvLogColsAreCiPrefix();
  if (pBindWA_->errStatus())
    return FALSE;

  // Prepare the column names for use by other methods.
  buildAllRefNames();

  return TRUE;
}

//----------------------------------------------------------------------------
// Make sure running the MVLOG command on this table is legal.
// It is not legal for AUTOMATIC RANGELOG tables, or if logging
// is not required at all.
NABoolean MvLog::isTableValidForMvLog() const
{
  const ComMvAttributeBitmap& bitmap = pNaTable_->getMvAttributeBitmap();
  if (!bitmap.getEnableMVLOGExecution())
  {
    ComRangeLogType logType = bitmap.getRangeLogType();
    if (logType != COM_MANUAL_RANGELOG && logType != COM_MIXED_RANGELOG)
    {
      // 12309 MVLOG command can only be used on MANUAL RANGELOG and MIXED RANGELOG tables.
      *CmpCommon::diags() << DgSqlCode(-12309);
    }
    else
    {
      // 12310 No Materialized Views are defined on table $0~TableName.
      *CmpCommon::diags() << DgSqlCode(-12310)
	  << DgTableName(tableName_->getQualifiedNameAsString());
    }
    pBindWA_->setErrStatus();
    return FALSE;
  }

  return TRUE;
}
  
//----------------------------------------------------------------------------
// Returns FALSE if the table is hash partitioned, or if there is only
// a single partition.
NABoolean MvLog::isTableRangePartitioned() const
{
  const PartitioningFunction *partFunc =
    pNaTable_->getClusteringIndex()->getPartitioningFunction();
  if (partFunc->isAHashPartitioningFunction() ||
      partFunc->isAHashDistPartitioningFunction() ||
      partFunc->isAHash2PartitioningFunction() )
  {
    // Table is Hash partitioned.
    return FALSE;
  }
  else
  {
    if (partFunc->isASinglePartitionPartitioningFunction())
    {
      // Table has a single partition.
      return FALSE;
    }
    else
    {
      CMPASSERT(partFunc->isARangePartitioningFunction());
    }
  }
  return TRUE;
}

//----------------------------------------------------------------------------
// Prepare column name lists for use by other methods. The lists are:
// 1. The columns from the MVLOG command line.
// 2. The base table clustering index columns.
// 3. The base table columns that are not in the clustering index.
void MvLog::buildBaseTableColumnNamesLists()
{
  CMPASSERT(NULL != pNaTable_);

  const NAFileSet * pCiFileSet = pNaTable_->getClusteringIndex(); 

  // MVLOG COMMAND LINE NAMES
  ItemExprList columnNamesList((ItemExpr *)pColumnNamesItem_, heap_);
  for (CollIndex i(0); i < columnNamesList.entries() ; i++)
  {
    ItemExpr * pItemExpr = columnNamesList[i];
    CMPASSERT(ITM_REFERENCE == pItemExpr->getOperatorType());
    ColReference * pColRef = (ColReference*)pItemExpr;

    const NAString& colRefName =  pColRef->getColRefNameObj().getColName();
    mvLogColNamesList_.insert(new(heap_)NAString(colRefName, heap_));
  }

  // BASE TABLE CI COLUMN NAMES
  const NAColumnArray & ciColumns = pCiFileSet->getIndexKeyColumns();
  CollIndex numberOfMvLogCols = mvLogColNamesList_.entries();
  if (numberOfMvLogCols > ciColumns.entries())
  {
    // 12311 The column list is not a prefix of the clustering key columns of table $0~TableName.
    *CmpCommon::diags() << DgSqlCode(-12311)
	<< DgTableName(tableName_->getQualifiedNameAsString());
    pBindWA_->setErrStatus();
    return;
  }

  CollIndex lastMvLogColIndex = numberOfMvLogCols -1;
  for ( CollIndex j = 0 ; j < ciColumns.entries() ; j++)
  {
    const NAString& ciColName = ciColumns[j]->getColName();
    baseTableCiNamesList_.insert(new (heap_)NAString(ciColName, heap_));
  }

  // Initialize the list of non-clustering-key columns (both names and min 
  // values). The min values are the values inserted into the log, since 
  // they are not read anyway.
  const NAColumnArray& allBaseTableCols = pNaTable_->getNAColumnArray();
  for (CollIndex k=0; k<allBaseTableCols.entries(); k++)
  {
    const NAColumn *colObj = allBaseTableCols[k];
    const NAString *colName = &colObj->getColName();
    if (!colObj->isClusteringKey())
    {
      nonCiColsNamesList_.insert(colName);

      const NAType *type = colObj->getType();
      ItemExpr *minValue = NULL;
      if (type->supportsSQLnull())
	minValue = new(heap_) Cast(new(heap_) SystemLiteral(), type);
      else
	minValue = new(heap_) SystemLiteral(type, TRUE, TRUE);
      nonCiColsExprList_.insert(minValue);
    }
  }
} // MvLog::buildBaseTableColumnNamesLists

//----------------------------------------------------------------------------
// Verify that the list of columns from the MVLOG command line are a prefix
// of the clustering index columns of the base table.
void MvLog::checkMvLogColsAreCiPrefix() const
{
  CMPASSERT(mvLogColNamesList_.entries() <= baseTableCiNamesList_.entries());

  // For each column from the command line
  for ( CollIndex i(0) ; i < mvLogColNamesList_.entries() ; i++)
  {
    const NAString& tableColName    = *baseTableCiNamesList_[i];
    const NAString& commandColName  = *mvLogColNamesList_[i];

    // check it against the corresponding column in the CI.
    if(tableColName != commandColName)
    {
      // 12311 The column list is not a prefix of the clustering key columns of table $0~TableName.
      *CmpCommon::diags() << DgSqlCode(-12311)
	  << DgTableName(tableName_->getQualifiedNameAsString());
      pBindWA_->setErrStatus();
      return;
    }
  }
} // MvLog::checkMvLogColsAreCiPrefix

//----------------------------------------------------------------------------
// Prepare lists of column names, that are derived from the base table 
// column names, with several different suffixes.
void MvLog::buildAllRefNames()
{
  for (CollIndex i(0); i < baseTableCiNamesList_.entries() ; i++)
  {
    const NAString& colRefName = *baseTableCiNamesList_[i]; 

    NAString *pBeginName = new(heap_) NAString(colRefName, heap_);
    pBeginName->append(getNames().getBeginRangeSuffix()); // "_BEGIN"
    beginColNamesList_.insert(pBeginName);

    NAString *pEndName = new(heap_) NAString(colRefName, heap_);
    pEndName->append(getNames().getEndSuffix());           // "_END"
    endColNamesList_.insert(pEndName);

    NAString *pBRName = new(heap_) NAString(colRefName, heap_);
    pBRName->append(getNames().getBrSuffix());             // "_BR"
    brColNamesList_.insert(pBRName); 

    NAString *pERName = new(heap_) NAString(colRefName, heap_);
    pERName->append(getNames().getErSuffix());             // "_ER"
    erColNamesList_.insert(pERName);
    
    NAString *pFirst1Name = new(heap_) NAString(colRefName, heap_);
    pFirst1Name->append(getNames().getEndRangeSuffix());  // "_FIRST"
    first1ColNamesList_.insert(pFirst1Name);
  }
} // MvLog::buildAllRefNames

//----------------------------------------------------------------------------
// Here we create default values for CI columns. These values are used when
// not all the CI columns are specified by the user.
// we actually create two default lists - one is a MAX default value and the 
// other is the MIN default value.
// Notice that if a column is DESC, than the max is actually a MIN. And vice 
// versa.
void MvLog::buildClusteringIndexDefaultValues(const NAFileSet * pCiFileSet)
{
  const NAColumnArray& ciColumns = pCiFileSet->getIndexKeyColumns();

  // These values are used as parameters to ConstValue.
  enum { MAX = FALSE, MIN = TRUE };
  NABoolean nullable = FALSE; // all CI are not nullable

  CMPASSERT(pRelevantCiPositionsList_.entries() == mvLogColNamesList_.entries());
  
  for(CollIndex i(0) ; i < ciColumns.entries() ; i++)
  {
    NAColumn     *pCol = ciColumns[i];
    const NAType *type = pCol->getType(); 
    ConstValue   *orderingMinVal = NULL;
    ConstValue   *orderingMaxVal = NULL;
    
    if (DESCENDING == pCol->getClusteringKeyOrdering())
    {
      orderingMinVal = new (heap_) SystemLiteral(type, MAX, nullable);
      orderingMaxVal = new (heap_) SystemLiteral(type, MIN, nullable);
    }
    else
    {
      orderingMinVal = new (heap_) SystemLiteral(type, MIN, nullable);
      orderingMaxVal = new (heap_) SystemLiteral(type, MAX, nullable);
    }
    
    colsMinDefualtValList_.insert(orderingMinVal);
    colsMaxDefualtValList_.insert(orderingMaxVal);
  }
} // MvLog::buildClusteringIndexDefaultValues


//----------------------------------------------------------------------------
void MvLog::constructBoundriesList()
{
  const NAFileSet * pCiFileSet = pNaTable_->getClusteringIndex(); 

  getColumnsPositionsInCI(pCiFileSet); // kept in pRelevantCiPositionsList_

  buildClusteringIndexDefaultValues(pCiFileSet);

  CMPASSERT(pBeginRangeExprList_.entries() == pEndRangeExprList_.entries());
  
  CollIndex numberOfCICols =  colsMinDefualtValList_.entries();
  CMPASSERT(numberOfCICols == colsMaxDefualtValList_.entries());

  CollIndex suffixIndex = pBeginRangeExprList_.entries();
  for ( ; suffixIndex < numberOfCICols ; suffixIndex++)
  {
    ItemExpr * minValue = 
      colsMinDefualtValList_[suffixIndex]->copyTree(heap_);
    
    ItemExpr * maxValue = 
      colsMaxDefualtValList_[suffixIndex]->copyTree(heap_);

    pBeginRangeExprList_.insert(minValue);
    pEndRangeExprList_.insert(maxValue);
  }

  CMPASSERT(pBeginRangeExprList_.entries() == 
	    baseTableCiNamesList_.entries());
  
  CMPASSERT(pEndRangeExprList_.entries() == 
	    baseTableCiNamesList_.entries());

  ciBoundries_.insert(&pBeginRangeExprList_);
  addClusteringIndexBoundries(ciBoundries_, pCiFileSet);
  ciBoundries_.insert(&pEndRangeExprList_);
} // MvLog::constructBoundriesList

//----------------------------------------------------------------------------
void MvLog::getColumnsPositionsInCI(const NAFileSet * pCiFileSet) 
{
  const NAColumnArray& ciColumns = pCiFileSet->getIndexKeyColumns();

  CMPASSERT(mvLogColNamesList_.entries() <= ciColumns.entries());
  
  CollIndex inputColIndex(0); 
  for ( ; inputColIndex < mvLogColNamesList_.entries() ; inputColIndex++)
  {
    const NAString& ciColName = ciColumns[inputColIndex]->getColName();
    const NAString& colRefName =  *mvLogColNamesList_[inputColIndex];

    if(ciColName == colRefName)
    {
      pRelevantCiPositionsList_.insert(inputColIndex);
    }
    else // not a CI prefix
    {
      // 12311 The column list is not a prefix of the clustering key columns of table $0~TableName.
      *CmpCommon::diags() << DgSqlCode(-12311)
	  << DgTableName(tableName_->getQualifiedNameAsString());
      pBindWA_->setErrStatus();
      return;
    }
  }
} // MvLog::getColumnsPositionsInCI

//----------------------------------------------------------------------------
// extract from the FileSet the CI boundries and from each boundry get only 
// the columns indicated by the columnsCiPositionsList
void MvLog::addClusteringIndexBoundries(LIST(ItemExprList*) & ciBoundries,
				        const NAFileSet * pCiFileSet) const
{
  PartitioningFunction *pPartFunction = pCiFileSet->getPartitioningFunction();

  if (pPartFunction->isARangePartitioningFunction())
  { 
    const RangePartitioningFunction *pRangePartFunction = 
      pPartFunction->castToRangePartitioningFunction();
    CMPASSERT(pRangePartFunction!=NULL);

    addRangePartitionBoundries(ciBoundries, pRangePartFunction);
  }
} // MvLog::addClusteringIndexBoundries

//----------------------------------------------------------------------------
void MvLog::addRangePartitionBoundries(
		    LIST(ItemExprList*) & ciBoundries, 
		    const RangePartitioningFunction *pRangePartFunction) const
{
  const RangePartitionBoundaries* pBoundries = 
    pRangePartFunction->getRangePartitionBoundaries();

  // the first boundry is for the primary partition and therefore NULL
  CMPASSERT(NULL == pBoundries->getBoundaryValues(0));
  
  CollIndex boundryIndex(1);
  for ( ; 
       boundryIndex < (CollIndex)pBoundries->getCountOfPartitions() ; 
       boundryIndex++)
  {
    const ItemExprList* pBoundry = 
      pBoundries->getBoundaryValues(boundryIndex);
    CMPASSERT(NULL != pBoundry);

    ItemExprList *pBoundryCopy = new(heap_)ItemExprList(heap_);

    CollIndex numberOfCICols =  baseTableCiNamesList_.entries();
    CMPASSERT(numberOfCICols == colsMinDefualtValList_.entries());

    CollIndex listIndex(0);
    for (; listIndex < numberOfCICols ; listIndex++)
    {
      ItemExpr * pValue = NULL;
      ItemExpr * pValueCopy = NULL;
      
      if (listIndex < pBoundry->entries())
	pValue = (*pBoundry)[listIndex];
      else
	pValue = colsMinDefualtValList_[listIndex];

      pValueCopy = pValue->copyTree(heap_);

      if (listIndex < pBoundry->entries())
      {
	// we get the boundry ItemExpr with a valueId. 
	// we need to initialize it
	pValueCopy->setValueId(ValueId());
	pValueCopy->markAsUnBound();
      }

      pBoundryCopy->insert(pValueCopy);
    }
    ciBoundries.insert(pBoundryCopy);
  }
} // MvLog::addRangePartitionBoundries


// ===========================================================================
// ===========================================================================
// ========   The TupleList Block
// ===========================================================================
// ===========================================================================

//----------------------------------------------------------------------------
// build a tuple list. on top build a rename and oon it build a root with 
// selection predicates. The notation *_BEGIN means its the vector of 
// clustering index column names, with the suffix _BEGIN.
//
//   Root   } BeginGreaterThanEndPhase : (*_BEGIN <= *_END)
//     |      
//  Rename
//     |    } Phase 3 : (*_BEGIN = MAX(*_BR, *_BEGIN), *_END = MIN(*_ER,*_END))
//   Root 
//     |      
//  Rename
//     |    } Phase 2 : (*_BR, *_BEGIN, *_ER, *_END, BR_IS_GREATER, ER_IS_LESS)
//   Root 
//     |      
//  Rename  } Phase 1 : (*_BEGIN, *_END) 
//     |
// TupleList
//
RelRoot *MvLog::buildTupleListBlock() const
{
  TupleList * pTupleList = constructTupleListFromBoundries(ciBoundries_);

  // *_BEGIN, *_END : the physical partition boundaries.
  RenameTable * pPhase1Rename = constructTupleListPhase1(pTupleList);

  // *_BR, *_BEGIN, *_ER, *_END, BR_IS_GREATER, ER_IS_LESS
  // *_BR and *_ER are the logical range boundaries from the command line.
  // BR_IS_GREATER and ER_IS_LESS are boolean scalars.
  RenameTable * pPhase2Rename = constructTupleListPhase2(pPhase1Rename);

  // *_BEGIN = MAX(*_BR, *_BEGIN), *_END = MIN(*_ER,*_END)
  RenameTable * pPhase3Rename = constructTupleListPhase3(pPhase2Rename);

  // selection predicate on *_BEGIN <= *_END
  RelRoot * pRoot = constructTupleListBeginGreaterThanEndPhase(pPhase3Rename);  
  
  // we want the left side to know the right side
  pRoot->setDontOpenNewScope();

  return pRoot;
} // MvLog::buildTupleListBlock


//----------------------------------------------------------------------------
// build the tuple list node from the physical partition boundaries.
// a tuple is: CI values of partition begin, CI values of partition end.
TupleList *MvLog::constructTupleListFromBoundries(
			      const LIST(ItemExprList*) & ciBoundries) const
{
  ItemExprList listOfTuplesRows(heap_);

  for (CollIndex i(ciBoundries.entries() - 1) ; i > 0 ; i--)
  {
    CMPASSERT(NULL != ciBoundries[i] && NULL != ciBoundries[i - 1]);
    
    const ItemExprList firstList = *(ciBoundries[i - 1]);
    const ItemExprList secondList = *(ciBoundries[i]);

    ItemExprList allList(heap_);

    allList.insert(firstList);
    allList.insert(secondList);

    ItemExpr *pAllColsExpr = 
      BinderUtils::setItemExprFromList(allList, heap_, RIGHT_LINEAR_TREE);
    
    ItemExpr * pCopy = pAllColsExpr->copyTree(heap_);
    ItemExpr *tupleRow = new(heap_) Convert(pCopy);
    listOfTuplesRows.insert(tupleRow);
  }

  ItemExpr * tupleExpr = 
    BinderUtils::setItemExprFromList(listOfTuplesRows, 
				     heap_,
				     RIGHT_LINEAR_TREE);

  return new(heap_) TupleList(tupleExpr);
} // MvLog::constructTupleListFromBoundries

//----------------------------------------------------------------------------
// give the tuple list columns names: *_BEGIN, *_END
RenameTable * MvLog::constructTupleListPhase1(TupleList *pTupleList) const
{
  ItemExprList allCols(heap_);

  appendToExprList(allCols, beginColNamesList_);
  appendToExprList(allCols, endColNamesList_);

  ItemExpr *pRenColsExpr =
    BinderUtils::setItemExprFromList(allCols, heap_, RIGHT_LINEAR_TREE);

  RenameTable *renameNode = new(heap_) 
    RenameTable(TRUE, pTupleList, tupleListPhase1CorrName_, pRenColsExpr , heap_);

  return renameNode;
} // MvLog::constructTupleListPhase1


//----------------------------------------------------------------------------
// Build a Root node with the phase 2 select list, and over it a Rename node
// with the col names: (*_BR, *_BEGIN, *_ER, *_END, BR_IS_GREATER, ER_IS_LESS)
RenameTable * MvLog::constructTupleListPhase2(RelExpr *pPhase1) const
{
  RelRoot *pP2Results = new(heap_) RelRoot(pPhase1);

  ItemExpr * pP2SelectList = buildTupleListPhase2SelectResults();
  pP2Results->addCompExprTree(pP2SelectList);

  RenameTable * pTupleListP2Reanme = buildTupleListPhase2Rename(pP2Results);
  return pTupleListP2Reanme;
} // MvLog::constructTupleListPhase2

//----------------------------------------------------------------------------
// Add two column vectors to the existing (*_BEGIN, *_END) row: 
// *_BR is the vector of CI values for the logical range start.
// *_ER is the vector of CI values for the logical range end.
// These vectors were specified by the user in the MVLOG command line.
// Two additional boolean scalars are BR_IS_GREATER and ER_IS_LESS:
// BR_IS_GREATER  = (BR > BEGIN) ? TRUE : FALSE.
// ER_IS_LESS     = (ER > END)   ? TRUE : FALSE.
// These are used by the min max evaluation in the next phase.
ItemExpr * MvLog::buildTupleListPhase2SelectResults() const
{
  CMPASSERT(pBeginRangeExprList_.entries() > 0 );
  ItemExpr * pBeginRang = // *_BR
    pBeginRangeExprList_.convertToItemExpr()->copyTree(heap_);
  ItemExpr * pEndRang =   // *_ER
    pEndRangeExprList_.convertToItemExpr()->copyTree(heap_);
  
  ItemExpr * pBeginColumnsRef = // *_BEGIN
    BinderUtils::getNamesListAsItemExpr(beginColNamesList_, heap_,
				        ITM_REFERENCE, 
					tupleListPhase1CorrName_);
  
  ItemExpr * pEndColumnsRef =   // *_END
    BinderUtils::getNamesListAsItemExpr(endColNamesList_, heap_,
				        ITM_REFERENCE, 
					tupleListPhase1CorrName_);
 
  ItemExpr * beginRangeGreaterThanBeginCols = // BR_IS_GREATER
    compareItems(pBeginRang, pBeginColumnsRef, ITM_GREATER, pDirectionVector_);

  ItemExpr * endRangeLessThanEndCols = // ER_IS_LESS
    compareItems(pEndRang, pEndColumnsRef, ITM_LESS, pDirectionVector_);

  ItemExprList columns(heap_);

  columns.insert(pBeginRang);
  columns.insert(pBeginColumnsRef);
  columns.insert(beginRangeGreaterThanBeginCols);

  columns.insert(pEndRang);
  columns.insert(pEndColumnsRef);
  columns.insert(endRangeLessThanEndCols);

  return columns.convertToItemExpr();
} // MvLog::buildTupleListPhase2SelectResults

//----------------------------------------------------------------------------
// Build the phase 2 rename node with the column names:
// (*_BR, *_BEGIN, *_ER, *_END, BR_IS_GREATER, ER_IS_LESS)
RenameTable * MvLog::buildTupleListPhase2Rename(RelExpr * pTupleListP2) const
{
  // Construct the column names list
  ConstStringList columnNamesList(heap_);

  columnNamesList.insert(brColNamesList_);
  columnNamesList.insert(beginColNamesList_);
  columnNamesList.insert(new(heap_) NAString(getNames().getBrIsGreater(), heap_));
  columnNamesList.insert(erColNamesList_);
  columnNamesList.insert(endColNamesList_);
  columnNamesList.insert(new(heap_) NAString(getNames().getErIsLess(), heap_));

  ItemExpr * pColNames = 
    BinderUtils::getNamesListAsItemExpr(columnNamesList, 
					heap_,
					ITM_RENAME_COL,
					emptyCorrName_);

  // And the Rename node itself.
  CorrName P2Name(tupleListPhase2CorrName_);
  RenameTable * pPhase2Rename = new(heap_) 
    RenameTable(TRUE, pTupleListP2, P2Name, pColNames, heap_);

  return pPhase2Rename;
} // MvLog::buildTupleListPhase2Rename

//----------------------------------------------------------------------------
// The result column are (*_BEGIN, *_END) where
// *_BEGIN = MAX(*_BR, *_BEGIN) - done using phase 2 BR_IS_GREATER column
// *_END   = MIN(*_ER, *_END)   - done using phase 2 ER_IS_LESS column
RenameTable * MvLog::constructTupleListPhase3(RelExpr * pPhase2) const
{
  RelRoot *pP3Results = new(heap_)RelRoot(pPhase2);
  ItemExpr * pP3SelectList = buildTupleListPhase3SelectResults();
  pP3Results->addCompExprTree(pP3SelectList);

  RenameTable * pTupleListP3Rename = buildTupleListPhase3Rename(pP3Results);

  return pTupleListP3Rename;
} // MvLog::constructTupleListPhase3

//----------------------------------------------------------------------------
// Construct the MIN and MAX expressions for the CI column vectors, using the 
// condition that is alreay bound by now. 
// For each column col_BEGIN in in *_BEGIN, its new value will be:
// If (BR_IS_GREATER) THEN col_BR ELSE col_BEGIN.
// For each column col_END in in *_END, its new value will be:
// If (ER_IS_LESS) THEN col_ER ELSE col_END.
// The fact that the conditions were already bound, and that the same ValueId
// is used for all the columns in the CI, means that in execution time, this 
// condition will be evaluated once, and the result used for the expressions
// of all the columns that use it.
ItemExpr * MvLog::buildTupleListPhase3SelectResults() const
{
  ItemExprList columnExpresionList(heap_);

  NAString *pMaxDecisionFactor = 
    new(heap_) NAString(getNames().getBrIsGreater(), heap_);
  
  ItemExpr *pMaxColsListExpr = 
    buildConditionalColumnList( brColNamesList_,
				beginColNamesList_,
			       *pMaxDecisionFactor,
				tupleListPhase2CorrName_);

  columnExpresionList.insert(pMaxColsListExpr);

  NAString *pMinDecisionFactor = new(heap_) NAString(getNames().getErIsLess(), heap_);
  ItemExpr *pMinColsListExpr = 
    buildConditionalColumnList( erColNamesList_,
				endColNamesList_,
			       *pMinDecisionFactor,
				tupleListPhase2CorrName_);
  
  columnExpresionList.insert(pMinColsListExpr);
  return columnExpresionList.convertToItemExpr();
} // MvLog::buildTupleListPhase3SelectResults

//----------------------------------------------------------------------------
// Build the phase 3 rename node with the column names: (*_BEGIN, *_END)
RenameTable * MvLog::buildTupleListPhase3Rename(RelExpr * pPhase3) const
{
  ConstStringList columnNamesList(heap_);

  columnNamesList.insert(beginColNamesList_);
  columnNamesList.insert(endColNamesList_);

  ItemExpr * pColNames = 
    BinderUtils::getNamesListAsItemExpr(columnNamesList, 
					heap_,
					ITM_RENAME_COL,
					emptyCorrName_);

  CorrName P3Name(tupleListPhase3CorrName_);

  RenameTable * pPhase3Rename = new(heap_) 
    RenameTable(TRUE, pPhase3, P3Name, pColNames, heap_);

  return pPhase3Rename;
} // MvLog::buildTupleListPhase3Rename

//----------------------------------------------------------------------------
// Construct a Root node with the selection predicate: (BEGIN <= END)
RelRoot * 
MvLog::constructTupleListBeginGreaterThanEndPhase(RelExpr * pPhase) const
{
  ItemExpr * beginExpr = 
    BinderUtils::getNamesListAsItemExpr(beginColNamesList_, 
					heap_,
					ITM_REFERENCE,
					emptyCorrName_);
  ItemExpr * endExpr = 
    BinderUtils::getNamesListAsItemExpr(endColNamesList_, 
					heap_,
					ITM_REFERENCE,
					emptyCorrName_);

  BiRelat * selectPredicate = new (heap_) 
    BiRelat(ITM_LESS_EQ, beginExpr, endExpr);

  selectPredicate->setDirectionVector(pDirectionVector_);

  RelRoot *pResults = new (heap_) RelRoot(pPhase);

  pResults->addSelPredTree(selectPredicate);
  return pResults;

} // MvLog::constructTupleListBeginGreaterThanEndPhase

//----------------------------------------------------------------------------
// compare two items and return a BoolVal
ItemExpr * MvLog::compareItems( const ItemExpr * pFirst, 
				const ItemExpr * pSecond, 
				OperatorTypeEnum compareType,
				IntegerList * pDirectionVector) const
{
  BiRelat * pCompare = new (heap_) 
    BiRelat(compareType, 
	    ((ItemExpr*)pFirst)->copyTree(heap_), 
	    ((ItemExpr*)pSecond)->copyTree(heap_));

  if(NULL != pDirectionVector)
  {
    pCompare->setDirectionVector(pDirectionVector);
  }

  IfThenElse * pIfElse = new(heap_)
    IfThenElse(pCompare, 
	       new (heap_) BoolVal(ITM_RETURN_TRUE), 
	       new (heap_) BoolVal(ITM_RETURN_FALSE));

  return new(heap_) Case(NULL, pIfElse);

} // MvLog::compareItems

//----------------------------------------------------------------------------
ItemExpr * 
MvLog::buildConditionalColumnList(const ConstStringList&  option1names,
				  const ConstStringList&  option2names,
				  const NAString&         conditionColName,
				  const CorrName&         corrName) const
{
  CMPASSERT(option1names.entries() == option2names.entries());
  
  ItemExprList allConditionCols(heap_);

  ColRefName *pCondColRefName = new(heap_) 
    ColRefName(conditionColName, corrName, heap_);
  ColReference * pCondColRef = new(heap_) 
    ColReference(pCondColRefName);

  for (CollIndex i(0) ; i < option1names.entries() ; i++)
  {
    ConstStringList option1Name(heap_);    
    ConstStringList option2Name(heap_);    

    option1Name.insert(option1names[i]);
    option2Name.insert(option2names[i]);

    ItemExpr * pOp1Expr = 
      BinderUtils::getNamesListAsItemExpr(option1Name, 
					  heap_,
					  ITM_REFERENCE,
					  corrName);
    ItemExpr * pOp2Expr = 
      BinderUtils::getNamesListAsItemExpr(option2Name, 
					  heap_,
					  ITM_REFERENCE,
					  corrName);

    Case * pCase = new(heap_) 
      Case(NULL, new(heap_) IfThenElse(pCondColRef, pOp1Expr, pOp2Expr));

    allConditionCols.insert(pCase);
  }

  return allConditionCols.convertToItemExpr();
  
} // MvLog::buildConditionalColumnList

// ===========================================================================
// ===========================================================================
// ========= The Get First Block
// ===========================================================================
// ===========================================================================

//----------------------------------------------------------------------------
// Build the GetFirst Block:
//   Root  (GetFirst 1 rows)
//    |
//  Rename (FIRST1_SCAN)
//    |
//   Root  (select list: just CI cols)
//    |
//   Scan base table.
RelExpr * MvLog::buildGetFirstNode() const
{
  CorrName tableName(*tableName_, heap_);
    
  Scan    * pScanTable = new(heap_) Scan(tableName);
  pScanTable->setForceInverseOrder();
  
  if (pNaTable_->getKeyCount() > 1)
    pScanTable->getInliningInfo().setFlags(II_MdamForcedByInlining);

  // Build a select list for just the CI columns
  ItemExpr * pCiColsSelectList = 
    BinderUtils::getNamesListAsItemExpr(baseTableCiNamesList_, 
					heap_,
					ITM_REFERENCE,
					emptyCorrName_);
  RelRoot * pRoot = new(heap_) 
    RelRoot(pScanTable, REL_ROOT, pCiColsSelectList);

  // SEL PREDICATE
  ItemExpr * pSelectionPred = buildGetFirstSelctionPredicate();
  pRoot->addSelPredTree(pSelectionPred);

  // BUILD RENAME
  ItemExprList renamedCols(heap_);

  ItemExpr * pRenamedCols = 
    BinderUtils::getNamesListAsItemExpr(first1ColNamesList_, 
					heap_,
					ITM_RENAME_COL,
					emptyCorrName_);

  RenameTable * pRenameTable = new(heap_) 
    RenameTable(TRUE, pRoot, first1NodeCorrName_, pRenamedCols, heap_);

  // we need this root so a new scope will be opened (if not we will run over
  // the Tuple List RetDesc
  RelRoot * pGetFirstRoot = new(heap_) RelRoot(pRenameTable);

  // We only need the first (actually last) row from each partition.
  pGetFirstRoot->needFirstSortedRows() = TRUE;
  pGetFirstRoot->setFirstNRows(1);
  pGetFirstRoot->getInliningInfo().setFlags(II_EnableFirstNRows);

  return pGetFirstRoot;
} // MvLog::buildGetFirstNode

//----------------------------------------------------------------------------
// Our data comes from two sources.
// 1. the Tuple List: *_BEGIN and *_END columns (tBegin, tEnd)
// 2. Clustering Index Column results from the scaned table ( cicScan )
//
// the selectionPred is:
//	cicScan >= tBegin  AND	cicScan < tEnd

ItemExpr * MvLog::buildGetFirstSelctionPredicate() const
{
  const CorrName mainTupleCorrName(tupleListCorrName_);
  
  // Get ALL Sources involved
  ItemExpr * cicScan = 
    BinderUtils::getNamesListAsItemExpr(baseTableCiNamesList_,
					heap_, ITM_REFERENCE, 
					emptyCorrName_);

  ItemExpr * tBegin = 
    BinderUtils::getNamesListAsItemExpr(beginColNamesList_,
					heap_, ITM_REFERENCE, 
					tupleListCorrName_);

  ItemExpr * tEnd = 
    BinderUtils::getNamesListAsItemExpr(endColNamesList_,
					heap_, ITM_REFERENCE, 
					tupleListCorrName_);
  // BEGIN <= CIC < END
  Between * pBetween = new (heap_) 
    Between(cicScan->copyTree(heap_),
	    tBegin->copyTree(heap_), 
	    tEnd->copyTree(heap_), 
	    TRUE,   // scan >= begin 
	    FALSE); // scan < end

  pBetween->setDirectionVector(*pDirectionVector_, heap_);
  return pBetween;
} // MvLog::buildGetFirstSelctionPredicate

//----------------------------------------------------------------------------
ItemExpr * MvLog::buildColumnNotEqualBoundriesExpr(ItemExpr * pColumn) const
{
  // this is returned incase there are no boundries
  ItemExpr * result = new (heap_) BoolVal(ITM_RETURN_TRUE);
  
  // the first and the last boundries are the BEGIN RANGE and END RANGE
  // we don't need them
  for (CollIndex i(1) ; i < ciBoundries_.entries() - 2 ; i++)
  {
    CMPASSERT(NULL != ciBoundries_[i] );

    ItemExprList * pBoundry = ciBoundries_[i];
    CMPASSERT(NULL != pBoundry);
    ItemExpr * boundryItem = pBoundry->convertToItemExpr();
    
    BiRelat * colNEQboundry = new (heap_)
      BiRelat(ITM_NOT_EQUAL,
	      pColumn->copyTree(heap_),
	      boundryItem->copyTree(heap_));
    
    result = new (heap_) BiLogic(ITM_AND, result, colNEQboundry);
  }

  return result;
} // MvLog::buildColumnNotEqualBoundriesExpr

//----------------------------------------------------------------------------
// the tsj outputs tuples of the form BEGIN, END, FIRST 
RelExpr * 
MvLog::buildJoinWithGetFirstBlock(RelExpr * topNode) const
{
  RelExpr * getFirst1Node = buildGetFirstNode();
  Join *pTsj = new(heap_) Join(topNode, getFirst1Node, REL_TSJ);
  pTsj->setTSJForWrite(TRUE);

  RelRoot *rootNode = new(heap_) RelRoot(pTsj);

  ItemExprList tsjResult(heap_);
  appendToExprList(tsjResult, beginColNamesList_, ITM_REFERENCE);
  appendToExprList(tsjResult, first1ColNamesList_, ITM_REFERENCE);
  ItemExpr * tsjSelectList =  tsjResult.convertToItemExpr();

  rootNode->addCompExprTree(tsjSelectList);

  return rootNode;
} // MvLog::buildJoinWithGetFirstBlock

// ===========================================================================
// ===========================================================================
// ======= End Range Block
// ===========================================================================
// ===========================================================================

//----------------------------------------------------------------------------
// Builds a TupleList with a row with the range from EndRange to EndRange.
// EndRange is the end range values given by the user on the command line.
// The result is a range that covers one base table row, if it exists, and
// an empty range otherwise. This row will not be further processed, and will
// be unioned with the processed results of the main TupleList, and entered 
// into the IUD log. 
RelExpr * MvLog::buildUnionWithEndRangeTupleList(RelExpr * topNode) const
{
  LIST(ItemExprList*) endRangeBoundry(heap_);
  constructEndRangeBoundries(endRangeBoundry, FALSE);
  CMPASSERT(2 == endRangeBoundry.entries());

  TupleList * pSingleTupleList = 
    constructTupleListFromBoundries(endRangeBoundry);
  RelRoot   * pRoot  = new (heap_) RelRoot(pSingleTupleList);

  topNode = new (heap_) Union(topNode, pRoot, NULL, NULL, REL_UNION,
                              CmpCommon::statementHeap(),TRUE);
  RelRoot *rootNode = new (heap_) RelRoot(topNode);
  rootNode->setDontOpenNewScope();

  return topNode;
} // MvLog::buildUnionOverTsj

//----------------------------------------------------------------------------
// The boundry is from Start or End Range to End Range, 
// each boundry is flipped over.
// Notice that we do not copy the End Range ItemExpr nodes since they can be 
// HostVars and do not need to be replicated
// Normally, needStartRange is FALSE to create a single row range on End Range.
// For hash partitioned tables, the boundary is from Start to End Range.
void 
MvLog::constructEndRangeBoundries(LIST(ItemExprList*)& endRangeBoundry,
				  NABoolean            needStartRange) const
{
  // Create the two mirrored lists
  ItemExprList * pEndRangeList = new (heap_) ItemExprList(heap_);
  ItemExprList * pBeginRangeList = NULL;
  if (needStartRange)
    pBeginRangeList = new (heap_) ItemExprList(heap_);

  CollIndex numOfEntries = pEndRangeExprList_.entries();
  for (CollIndex i(0) ; i < numOfEntries ; i++ )
  {
    pEndRangeList->insert(pEndRangeExprList_[(numOfEntries - 1) - i]);
    if (needStartRange)
      pBeginRangeList->insert(pBeginRangeExprList_[(numOfEntries - 1) - i]);
  }

  // Insert the boundaries in mirrored order.
  endRangeBoundry.insert(pEndRangeList);

  if (needStartRange)
    endRangeBoundry.insert(pBeginRangeList);
  else
    endRangeBoundry.insert(pEndRangeList);

} // MvLog::constructEndRangeBoundries

// ===========================================================================
// ===========================================================================
// ======= Insert Into IUD Log Block.
// ===========================================================================
// ===========================================================================

//----------------------------------------------------------------------------
RelExpr *MvLog::buildInsertIntoLog(RelExpr *topNode) const
{
  CorrName tableName(*tableName_);
  MvIudLogForMvLog logTableObj(tableName, pBindWA_);
  if (pBindWA_->errStatus())
    return NULL;

  RelExpr *insertNode = logTableObj.buildInsert(FALSE,
                                                ChangesTable::ALL_ROWS,
                                                FALSE, TRUE);

  RelRoot *rootNode = new(heap_) RelRoot(insertNode);

  Join    *tsjNode = new(heap_) Join(topNode, rootNode, REL_TSJ);
  tsjNode->setTSJForWrite(TRUE);

  rootNode = new(heap_) RelRoot(tsjNode);

  return rootNode;
}  // MvLog::buildInsertIntoLog()

// ===========================================================================
// ===========================================================================
// ======= Read Current Epoch Block
// ===========================================================================
// ===========================================================================

//----------------------------------------------------------------------------
// Build a Scan node over the base table, and read from it only the 
// CurrentEpoch. This is done with this tree:
//                TSJ
//              /     \
//            /        Rest of tree
//          /
//    Rename  (READ_EPOCH)
//      |
//     Root   (select MIN(CurrentEpoch())
//      |
//    GroupBy (scalar - no GroupBy expression)
//      |
//     Root   (Get First 1 rows)
//      |
//     Scan
//
// If the base table is empty, an error 12315 will be generated.
// The current epoch number of the base table is used in every
// row that MVLOG inserts into the IUD log. Since we are not updating
// the base table here (as in a logging operation) we use a Scan node
// to get the current epoch number of the table. Because we don't really 
// need to read any rows from the base table, we just read the first row
// which will be projected with the Current Epoch column. The Aggregation
// insures that the result is a scalar and not table valued.
RelExpr *MvLog::buildScanEpochFromTable(RelExpr *topNode) const
{
  CorrName tableName(*tableName_, heap_);
    
  Scan    *scanNode = new(heap_) Scan(tableName);
  scanNode->getInliningInfo().setFlags(II_isMVLoggingInlined);

  RelRoot *rootNode = new(heap_) RelRoot(scanNode);
  rootNode->needFirstSortedRows() = TRUE;
  rootNode->setFirstNRows(1);
  rootNode->getInliningInfo().setFlags(II_EnableFirstNRows);

  GroupByAgg *groupByNode = new(heap_)
      GroupByAgg(rootNode);

  ItemExpr *selectList = buildEpochSelectList();
  RelRoot *rootNode2 = new(heap_) 
    RelRoot(groupByNode, REL_ROOT, selectList);

  ItemExpr *renameList = new(heap_)
    RenameCol(NULL, new(heap_) 
      ColRefName(InliningInfo::getEpochVirtualColName()));
  RenameTable *renameNode = new(heap_)
    RenameTable(rootNode2, "READ_EPOCH", renameList);

  Join    *tsjNode = new(heap_) Join(renameNode, topNode, REL_TSJ);
  tsjNode->setTSJForWrite(TRUE);

  return tsjNode;
}  // MvLog::buildScanEpochFromTable()

//----------------------------------------------------------------------------
// The value we project here is Epoch = MIN(CurrentEpoch). This returns NULL 
// when the table is empty, so we wrap it with this expression:
// Case( If   (minEpoch IS NULL AND RaiseError(12315)
//       Then  minEpoch
//       Else  minEpoch)
// The value returned is always the Epoch value, but if it is NULL, the error 
// will cancel execution. This trick is necessary because the type of 
// RaiseError is Boolean, so putting it as the returned value in the 
// Then or Else clauses, will not type-check.
ItemExpr *MvLog::buildEpochSelectList() const
{
  // minEpoch = MIN("@CURRENT_EPOCH")
  ItemExpr *minEpoch = new(heap_)
    Aggregate(ITM_MIN, new(heap_) ColReference(new(heap_) 
      ColRefName(InliningInfo::getEpochVirtualColName())));

  // The condition is (minEpoch IS NULL AND RaiseError)
  ItemExpr *condition = new(heap_)
    BiLogic(ITM_AND,
            new(heap_) UnLogic(ITM_IS_NULL, minEpoch),
	    new(heap_) RaiseError(12315));

  ItemExpr *selectList = new(heap_)
    Case(NULL, new(heap_)
	  IfThenElse(condition, minEpoch, minEpoch));

  return selectList;
}

//----------------------------------------------------------------------------
// This method is used when there is a single partition, or hash partitioning.
// Builds a TupleList with a single row with the begin and end range values
// given by the user on the command line.
RelExpr *MvLog::buildSimpleRangeTuple()
{
  constructBoundriesList();

  LIST(ItemExprList*) endRangeBoundry(heap_);
  constructEndRangeBoundries(endRangeBoundry, TRUE);
  CMPASSERT(2 == endRangeBoundry.entries());

  TupleList * pSingleTupleList = 
    constructTupleListFromBoundries(endRangeBoundry);
  RelRoot   * pRoot  = new (heap_) RelRoot(pSingleTupleList);

  ItemExprList renameColList(heap_);
  appendToExprList(renameColList, beginColNamesList_);
  appendToExprList(renameColList, first1ColNamesList_);

  ItemExpr *renameItemList = 
    BinderUtils::setItemExprFromList(renameColList, heap_, LEFT_LINEAR_TREE);
  RenameTable *renameNode = new(heap_)
    RenameTable(pRoot, getNames().getSimpleRangeTuple(), renameItemList);

  return renameNode;
}

//----------------------------------------------------------------------------
// The final select list includes both *_BEGIN and *_FIRST columns. Both lists
// include all the columns of the base table - the non-CI columns are added
// here as constants of the minimum value of their type.
RelExpr *MvLog::buildFinalSelectList(RelExpr *topNode)
{
  ItemExprList selectList(heap_);
  ItemExprList renameList(heap_);

  // Build the Root select list
  appendToExprList(selectList, beginColNamesList_, ITM_REFERENCE);

  for (CollIndex i=0; i<nonCiColsExprList_.entries(); i++)
    selectList.insert(nonCiColsExprList_[i]->copyTree(heap_));

  appendToExprList(selectList, first1ColNamesList_, ITM_REFERENCE);

  for (CollIndex j=0; j<nonCiColsExprList_.entries(); j++)
    selectList.insert(nonCiColsExprList_[j]->copyTree(heap_));

  ItemExpr *selectListExpr =
    BinderUtils::setItemExprFromList(selectList, heap_, LEFT_LINEAR_TREE);

  // Build the Root
  RelRoot *rootNode = new(heap_)
    RelRoot(topNode, REL_ROOT, selectListExpr);

  // Create name lists for the non-CI coluns with _BEGIN and _FIRST suffixs
  ConstStringList nonCiBeginNames(heap_);
  ConstStringList nonCiFirstNames(heap_);
  for (CollIndex k=0; k < nonCiColsNamesList_.entries() ; k++)
  {
    const NAString& colName = *nonCiColsNamesList_[k]; 

    NAString *pBeginName = new(heap_) NAString(colName, heap_);
    pBeginName->append(getNames().getBeginRangeSuffix()); // "_BEGIN"
    nonCiBeginNames.insert(pBeginName);

    NAString *pFirstName = new(heap_) NAString(colName, heap_);
    pFirstName->append(getNames().getEndRangeSuffix());   // "_FIRST"
    nonCiFirstNames.insert(pFirstName);
  }

  // Build the Rename list
  appendToExprList(renameList, beginColNamesList_);
  appendToExprList(renameList, nonCiBeginNames);
  appendToExprList(renameList, first1ColNamesList_);
  appendToExprList(renameList, nonCiFirstNames);

  ItemExpr *renameListExpr =
    BinderUtils::setItemExprFromList(renameList, heap_, LEFT_LINEAR_TREE);

  // Build the Rename on top of the Root.
  RenameTable *renameNode = new(heap_) 
    RenameTable(rootNode, getNames().getFinalResultName(), renameListExpr , heap_);

  return renameNode;
}

//----------------------------------------------------------------------------
// Shortcut to BinderUtils::appendToExprList();
// itemType has a default value of ITM_RENAME_COL.
void MvLog::appendToExprList(ItemExprList&            toAddto, 
			     const ConstStringList&   columnNames,
			     OperatorTypeEnum         itemType) const
{
  BinderUtils::appendToExprList(toAddto,  
                                columnNames,      
				heap_, 
				itemType, 
				emptyCorrName_);
}

