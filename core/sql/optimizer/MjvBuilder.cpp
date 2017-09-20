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
* File:         MjvBuilder.cpp
* Description:  Implementation for MjvBuilder class hierarchy.
*
* Created:      07/02/2000
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/

#include "AllItemExpr.h"
#include "AllRelExpr.h"
#include "ItemSample.h"
#include "MVInfo.h"
#include "BindWA.h"
#include "parser.h"
#include "MjvBuilder.h"

// ===========================================================================
// ===========================================================================
// ===================  class  MjvBuilder  ===================================
// ===========================================================================
// ===========================================================================

///////////////////////////////////////////////////////////////////////////////
// Builds the scanning node of the MJV. The scan selects from the MJV only rows
// having the same CI as the affected rows in the IUD node.
//
// The input parameters:
//   corrName    : The correlation name of the base table.
//   baseNaTable : The NATable of the base table.
///////////////////////////////////////////////////////////////////////////////
Scan *
MjvBuilder::buildScanOfMJV(const CorrName &corrName, const NATable *baseNaTable) const
{
  Scan *scanNode = new(heap_) Scan(getMvCorrName());

  // Attaching selection predicate to select only rows with specific CI value
  ItemExpr *selectionPred = buildEqualityPredOnClusteringIndex(corrName, baseNaTable);
  scanNode->addSelPredTree(selectionPred);

  return scanNode;
}

///////////////////////////////////////////////////////////////////////////////
// Builds the selection predicate for use in scanning MJV. This predicate cause
// scanning only those rows in the MJV that corresponds to the affected rows in
// the subject table. Since there's a secondary index on the base table's CI
// columns, the scanning will be effecient.
//
// The resulting tree looks like this:
//
//
//         topNode
//           |
//          AND
//          / \
//         /   \
//        /     \
//      AND      =
//      / \     / \
//    ... ...  /   \
//            /     \
//          mvCol  baseCol
//
// The input parameters:
//   tableName   : The correlation name of the base table.
//   baseNaTable : The NATable of the base table.
//
///////////////////////////////////////////////////////////////////////////////
ItemExpr *
MjvBuilder::buildEqualityPredOnClusteringIndex(const CorrName &tableName,
					       const NATable  *baseNaTable) const
{
  ItemExpr *eqList = NULL;
  ItemExpr *eqExpr = NULL;

  // Finding out the CI columns of the subject table
  const NAColumnArray keyColumns = 
    baseNaTable->getClusteringIndex()->getIndexKeyColumns();
  // Finding the columns of the MJV
  MVColumns &mvCols = getMvInfo()->getMVColumns();

  // Constructing the eqaulity predicates between the CI columns of the subject
  // table and their eqvivalent ones in the MJV. The predicates forms a left
  // linear tree.

  for (CollIndex i = 0; i < keyColumns.entries(); i++)
  {
    // Finding the equivalent column in the MJV.
    MVColumnInfo *mvCol =
      mvCols.getMvColInfoByBaseColumn(baseNaTable->getTableName(),
				      keyColumns[i]->getPosition());

    // Build the column references for use in the equation predicate
    ColReference *baseColumn = new(heap_)
      ColReference(new(heap_) ColRefName(keyColumns[i]->getColName(), tableName, heap_));
    CMPASSERT(mvCol);
    ColReference *mvColumn = new(heap_)
      ColReference(new(heap_) ColRefName(mvCol->getColName(), getMvCorrName(), heap_));
    eqExpr = new(heap_) BiRelat(ITM_EQUAL, mvColumn, baseColumn);

    if (eqList == NULL) // is this the first equality in tree?
    {
      eqList = eqExpr;
    }
    else
    {
      eqList = new(heap_) BiLogic(ITM_AND, eqList, eqExpr);
    }
  }

  return eqList;
}

///////////////////////////////////////////////////////////////////////////////
// Builds the new record expression for use in the direct-update refresh tree.
//
// The resulting tree looks like this:
//
//
//          topNode
//             |
//       ITM_ITEM_LIST
//          /     \
//         /       \
//        /         \
// ITM_ITEM_LIST   ASSIGN
//      / \          / \
//    ... ...       /   \
//                 /     \
//              mvCol  NEW@.<col>
//
///////////////////////////////////////////////////////////////////////////////
ItemExpr *MjvBuilder::buildUpdatePredicate(const CorrName &inputCorrName,
					   const NATable  *baseNaTable, 
					   SET(MVColumnInfo*) SetOfAffectedColumns) const
{
  ItemExpr *assignList = NULL;
  ItemExpr *assignExpr = NULL;

  // Finding out the list of columns in the subject table
  const NAColumnArray &baseCols = baseNaTable->getNAColumnArray();

  // Constructing the assignment predicates between each affected columns in
  // the MJV and the the appropriate expression trees representing them. The
  // predicate forms a left linear tree.

  for (CollIndex i = 0; i < SetOfAffectedColumns.entries(); i++)
  {
    // get the expression tree for the current affected MJV column
    MVColumnInfo *affectedCol = SetOfAffectedColumns[i];
    const NAString &origColName =
      baseCols[affectedCol->getOrigColNumber()]->getColName();

    ItemExpr *exprTree = getExpressionTreeForColumn(affectedCol, origColName, inputCorrName);

    // Building the assignment predicate
    ColReference *mvColumn = new(heap_)
      ColReference(new(heap_) ColRefName(affectedCol->getColName(),
					 getMvCorrName(),
					 heap_));
    assignExpr = new(heap_) Assign(mvColumn, exprTree);

    // Attach the new assignment into the left linear assignment tree
    if (assignList == NULL) // is this the first assignment in tree?
    {
      assignList = assignExpr;
    }
    else
    {
      assignList = new(heap_) ItemList(assignList, assignExpr);
    }
  }

  return assignList;
}

//////////////////////////////////////////////////////////////////////////////
// Construct the expression tree to be assigned for the MJV column. All the
// columns in the expression are taken from the set of NEW@ values.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MjvBuilder::getExpressionTreeForColumn(const MVColumnInfo *affectedCol,
					         const NAString      &colName,
						 const CorrName	     &inputCorrName) const
{
  const NAString &exprText = affectedCol->getNormalizedColText();
  ItemExpr *exprTree = NULL;
  if (exprText.length() > 0)
  {
    // Since update operation that affects complex MJV columns is
    // automatically considered indirect-update, at this point the only
    // computed columns we might encounter are columns with simple
    // functions.
    CMPASSERT(affectedCol->getColType() == COM_MVCOL_FUNCTION)

    // Parse the expression behind the column to consruct its tree
    Parser parser(CmpCommon::context());
    exprTree = parser.getItemExprTree(exprText.data(),
				      exprText.length());
    // Add the NEW@ correlation name to each column in the expression.
    addCorrNameToExpr(exprTree, inputCorrName);
  }
  else
  {
    // The expression tree behind the column is simply the corresponding
    // column in the subject table
    exprTree = new(heap_)
      ColReference(new(heap_) ColRefName(colName, inputCorrName, heap_));
  }

  // At this point, exprTree must already be set
  CMPASSERT(exprTree != NULL)

  return exprTree;
}

//////////////////////////////////////////////////////////////////////////////
// Add the inputCorrName correlation name to each column mentioned in the function
// behind the computed column. This method recursively scan the expression
// tree.
//
//////////////////////////////////////////////////////////////////////////////
void MjvBuilder::addCorrNameToExpr(ItemExpr *expr, const CorrName &inputCorrName) const
{
  // The stop condition
  if (expr == NULL)
  {
    return;
  }

  // Add the inputCorrName correlation name to this column reference item
  if (expr->getOperatorType() == ITM_REFERENCE)
  {
    ((ColReference *) expr)->getCorrNameObj() = inputCorrName;
  }

  Int32 arity = expr->getArity();
  for (Int32 i = 0; i < arity; i++)
  {
    addCorrNameToExpr(expr->child(i), inputCorrName); // apply change to childs
  }
}

//////////////////////////////////////////////////////////////////////////////
// Setup the bind context to replace the scan on the base table by a scan 
// on the IUD log or triggers temp table.
// Note that setBindingOnStatementMv() or setBindingMvRefresh() is done by 
// the calling method.
//////////////////////////////////////////////////////////////////////////////
void MjvBuilder::setupBindContext(RelRoot *topNode, 
			          RelExpr *scanBlock,
				  const QualifiedName *baseTableName)
{
  // Setup a new MvBindContext for replacing the scanning on the subject table
  MvBindContext *bindContext = new(heap_) MvBindContext(heap_);
  bindContext->setRefreshBuilder(this);
  bindContext->setReplacementFor(baseTableName, scanBlock);
  topNode->setMvBindContext(bindContext);
}

//////////////////////////////////////////////////////////////////////////////
// Construct a set of MVColumnInfo objects for all updated MJV columns.
//////////////////////////////////////////////////////////////////////////////
SET(MVColumnInfo*) MjvBuilder::collectAllAffectedColumns(const IntegerList *updatedCols,
							 const QualifiedName &qualTableName ) const
{
  SET(MVColumnInfo*) affectedCols(STMTHEAP);

  // Finding out the list of columns in the MJV
  const MVColumns &mvCols = getMvInfo()->getMVColumns();

  getMvInfo()->initUsedObjectsHash(); // initialization for the searching
  const MVUsedObjectInfo *usedObjectInfo = getMvInfo()->findUsedInfoForTable(qualTableName);
  
  for (CollIndex i = 0; i < updatedCols->entries(); i++)
  {
    Int32 updatedColumn = updatedCols->at(i);

    // Finding the affected column(s) in the MJV.
    const MVColumnInfoList *affectedMvCols =
      mvCols.getAllMvColsAffectedBy(qualTableName, updatedColumn);

    if (affectedMvCols == NULL)
    {
      continue; // skip updated columns that are not affecting the MJV
    }

    // add each affected column to the set (duplicates are rejected)
    for (CollIndex j = 0; j < affectedMvCols->entries(); j++ )
    {
      affectedCols.insert((*affectedMvCols)[j]);
    }
  }

  return affectedCols;
}

// ===========================================================================
// ===========================================================================
// ===================  class  MjvImmediateBuilder  ==========================
// ===========================================================================
// ===========================================================================

///////////////////////////////////////////////////////////////////////////////
// Building the refresh tree after a direct-update/delete  operations on the
// subject table.
//
// The resulting tree looks like this:
//
//         RelRoot
//           |
//   Delete/Update MJV
//           |
//         Scan MJV -- where CI = CI of deleted/updated row of base table
//
///////////////////////////////////////////////////////////////////////////////
RelExpr *
MjvImmediateBuilder::buildDeleteOrDirectUpdateBlock(const CorrName &corrName, 
					   const NATable *baseNaTable) const
{
  // Build scan node of the MJV
  Scan *scanNode = buildScanOfMJV(corrName, baseNaTable);

  // Building the appropriate node (delete / update) on top of the scan node
  GenericUpdate *guNode = buildGUNodeOfMJV(scanNode);

  // The refresh sub-tree doesn't output anything
  RelRoot *topNode = new(heap_) RelRoot(guNode);
  topNode->setEmptySelectList();

  return topNode;
}

///////////////////////////////////////////////////////////////////////////////
// Builds the node that updates the MJV. To directly update the MJV we should
// use the special namespace of MVs. Moreover, the updated rows of the MJV
// should not be considered the the overall count of rows.
///////////////////////////////////////////////////////////////////////////////
GenericUpdate *
MjvImmediateBuilder::buildGUNodeOfMJV(RelExpr *belowSubTree) const
{
  // build the specific GU node.
  GenericUpdate *guNode = buildActualGUOfMJV(belowSubTree);
  
  // Affected rows should not be counted.
  guNode->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;  

  return guNode;
}

///////////////////////////////////////////////////////////////////////////////
// This method aborts any security checks in the refresh tree of the MJV. The
// method recursively scans the refresh tree, and for each scan node the
// collection of security data is avoided.
///////////////////////////////////////////////////////////////////////////////

void
MjvImmediateBuilder::avoidSecurityCheckInRefreshTree(RelExpr *refreshTree) const
{
  if (refreshTree->getOperatorType() == REL_SCAN ||
      refreshTree->getOperator().match(REL_ANY_GEN_UPDATE))
  {
    // avoid collecting security data if the currnet node
    refreshTree->getInliningInfo().setFlags(II_AvoidSecurityChecks);
  }

  for (CollIndex i = 0; i < (CollIndex)refreshTree->getArity(); i++)
  {
    // avoid collecting security data for each sub-tree of the current node
    avoidSecurityCheckInRefreshTree(refreshTree->getChild(i)->castToRelExpr());
  }
}

// ===========================================================================
// ===========================================================================
// ===================  class  MjvImmInsertBuilder  ==========================
// ===========================================================================
// ===========================================================================

//////////////////////////////////////////////////////////////////////////////
// Building the refresh tree needed after inserts into the subject table.
//
// Building a block to return the delta on the subject table. This block is
// inserted into the MvBindContext hash table as a replacement for scanning
// the subject table. This MvBindContext is attached to the newly opened scope
// (the scope is opened by putting the RelRoot node above the insert node).
// This way the replacement will take place only at this particular scope.
//
// The resulting tree (after binding) will look like this:
//
//       Insert MJV
//           |
//           ^
//          / \
//         /   \   - MJV logic
//        <_____>
//         |
//         |
//       generated block
// instead of scan subject table
//
//////////////////////////////////////////////////////////////////////////////
RelExpr *
MjvImmInsertBuilder::buildRefreshTree()
{
  // Get the MJV tree
  RelExpr *mvSelectTree = getMvInfo()->buildMVSelectTree();

  // Put Insert node on top of it.
  GenericUpdate *guNode = buildGUNodeOfMJV(mvSelectTree);
  
  RelRoot *topNode = new(heap_) RelRoot(guNode);
  topNode->setEmptySelectList();

  // avoid security checks in the built refresh tree
  avoidSecurityCheckInRefreshTree(topNode);

  // Setup a new MvBindContext for replacing the scanning on the subject table
  bindWA_->setBindingOnStatementMv(); // enabling MvBindContext mechanism
  RelExpr *scanBlock = buildScanBlock();
  setupBindContext(topNode, scanBlock,
                   &((getIudNode()->getTableName()).getQualifiedNameObj()));

  return topNode;
}

//////////////////////////////////////////////////////////////////////////////
// Build the specific GU node for insert operations.
//////////////////////////////////////////////////////////////////////////////
GenericUpdate *
MjvImmInsertBuilder::buildActualGUOfMJV(RelExpr *subTreeBelow) const
{
  return new (heap_) Insert(getMvCorrName(), NULL, REL_UNARY_INSERT, subTreeBelow);
}

//////////////////////////////////////////////////////////////////////////////
// Build the block that will replace the scanning of the subject table.
//
// If an optimized version of the is tree is requested (see
// Insert::insertMvToTriggerList in inlining.cpp), use it. Otherwise, use the
// block built by TriggersTempTable object.
//
// There's no need to avoid security checks in the built block, since there
// are already no security checks on scan nodes of temp-table.
//////////////////////////////////////////////////////////////////////////////
RelExpr *
MjvImmInsertBuilder::buildScanBlock() const
{
  if (optimized_)
  {
    // Build the optimized tree - tuple with NEW@ values
    return buildOptimizedScanBlock();
  }
  else
  {
    // Build standard tree - scan NEW@ values from the temp-table
    TriggersTempTable tempTableObj(getIudNode(), bindWA_);

    return tempTableObj.buildScanForMV();
  }
}

//////////////////////////////////////////////////////////////////////////////
// Build the optimized version of the replacing block.
// The resulting tree looks like this:
//
//  RenameTable (NEW@ --> subject table)
//      |
//    Tuple     (with NEW@ values)
//
//////////////////////////////////////////////////////////////////////////////
RelExpr *
MjvImmInsertBuilder::buildOptimizedScanBlock() const
{
  CorrName newCorr(NEWCorr);

  // Finding out the list of columns in the subject table
  const NAColumnArray &colList =
    getIudNode()->getTableDesc()->getNATable()->getNAColumnArray();

  // Get the subject table's name
  const CorrName &tableName = getIudNode()->getTableName();

  // Build the tuple expression and the renaming list for the columns

  ItemExpr *renameList = NULL;
  ItemExpr *tupleExpr = NULL;
  for (CollIndex i = 0; i < colList.entries(); i++)
  {
    const NAString &colName = colList[i]->getColName();

    // Wrap the column with a Cast to get a new ValueId, so the Tuple
    // node will still require the original columns as input.

    ItemExpr *col = new(heap_)
      ColReference(new(heap_) ColRefName(colName, newCorr));
    ItemExpr *castCol = new(heap_) Cast(col, colList[i]->getType());

    // Add the column to the tuple expression

    if (tupleExpr == NULL)
    {
      tupleExpr = castCol;
    }
    else
    {
      tupleExpr = new(heap_) ItemList(tupleExpr, castCol);
    }

    // Add the column to the rename list

    RenameCol *renCol = new(heap_)
      RenameCol(col,
		new(heap_) ColRefName(colName, tableName));

    if (renameList == NULL)
      renameList = renCol;
    else
      renameList = new(heap_) ItemList(renameList, renCol);
  }

  // Build the Tuple with the not-covered items
  Tuple *optTuple = new(heap_) Tuple(tupleExpr);

  // Rename the table name and columns to immitate the scan of the subject
  // table for the rest of the MJV tree.
  RenameTable *renameNode = new(heap_)
    RenameTable(optTuple,
	        tableName.getExposedNameAsString(),
	        renameList,
	        heap_);

  return renameNode;
}

// ===========================================================================
// ===========================================================================
// ===================  class  MjvImmDeleteBuilder  ==========================
// ===========================================================================
// ===========================================================================

//////////////////////////////////////////////////////////////////////////////
// Builds the refresh tree after a delete operation.
//////////////////////////////////////////////////////////////////////////////

RelExpr *
MjvImmDeleteBuilder::buildRefreshTree()
{
  const NATable *baseNaTable = getIudNode()->getTableDesc()->getNATable();
  RelExpr *topNode = buildDeleteOrDirectUpdateBlock(CorrName(OLDCorr), baseNaTable);

    // avoid security checks in the built refresh tree
  avoidSecurityCheckInRefreshTree(topNode);

  return topNode;
}

//////////////////////////////////////////////////////////////////////////////
// Build the specific GU node for delete operations.
//////////////////////////////////////////////////////////////////////////////
GenericUpdate *
MjvImmDeleteBuilder::buildActualGUOfMJV(RelExpr *subTreeBelow) const
{
  return new (heap_) Delete(getMvCorrName(), NULL, REL_UNARY_DELETE, subTreeBelow);
}

// ===========================================================================
// ===========================================================================
// ===================  class  MjvImmDirectUpdateBuilder  ====================
// ===========================================================================
// ===========================================================================

//////////////////////////////////////////////////////////////////////////////
// Builds the refresh tree after a direct-update operation.
//////////////////////////////////////////////////////////////////////////////
RelExpr *
MjvImmDirectUpdateBuilder::buildRefreshTree()
{
  const NATable *baseNaTable = getIudNode()->getTableDesc()->getNATable();
  RelExpr *topNode = buildDeleteOrDirectUpdateBlock(CorrName(NEWCorr), baseNaTable);

    // avoid security checks in the built refresh tree
  avoidSecurityCheckInRefreshTree(topNode);

  return topNode;
}

//////////////////////////////////////////////////////////////////////////////
// Build the specific GU node for direct-update operations.
//////////////////////////////////////////////////////////////////////////////
GenericUpdate *
MjvImmDirectUpdateBuilder::buildActualGUOfMJV(RelExpr *subTreeBelow) const
{
  // construct a set of all affected columns in the MJV. All the affected
  // columns will have to have an assignment expression.
  IntegerList *updatedCols = StoiToIntegerList(getIudNode()->getOptStoi()->getStoi());
  const QualifiedName &qualTableName = getIudNode()->getTableName().getQualifiedNameObj();
  SET(MVColumnInfo*) SetOfAffectedColumns = 
    collectAllAffectedColumns(updatedCols, qualTableName);

  const NATable *baseNaTable = getIudNode()->getTableDesc()->getNATable();
  ItemExpr *updatePredicate = 
    buildUpdatePredicate(CorrName(NAString(NEWCorr)), 
			 baseNaTable,
			 SetOfAffectedColumns);

  return new(heap_)
    Update(getMvCorrName(), NULL, REL_UNARY_UPDATE, subTreeBelow, updatePredicate );
}

//////////////////////////////////////////////////////////////////////////////
// Utility method to create an IntegerList of updated columns from the STOI.
//////////////////////////////////////////////////////////////////////////////
IntegerList *
MjvImmDirectUpdateBuilder::StoiToIntegerList(SqlTableOpenInfo *stoi) const
{
  IntegerList *intList = new(heap_) IntegerList();

  for (short i = 0; i < stoi->getColumnListCount(); i++)
  {
    intList->insert(stoi->getUpdateColumn(i));
  }

  return intList;
}

// ===========================================================================
// ===========================================================================
// ==================  class  MjvOnRequestBuilder  ===========================
// ===========================================================================
// ===========================================================================

// Initialization of static data members
const char MjvOnRequestBuilder::virtualIndirectColumnName_[] = "INDIRECT@";

//----------------------------------------------------------------------------
//  ON REQUEST MJV refresh needs to handle all the cases, according to the 
//  input from the refresh utility.
//
//                RelRoot
//                   |
//                Ordered
//                 Union
//                /     \
//               /       \
//         Deletion      Insertion
//         Block         Block
//
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestBuilder::buildRefreshTree()
{
  RelExpr *topNode = NULL;
  NABoolean needDeletionBlock  = isNeedDeletionBlock();
  NABoolean needInsertionBlock = isNeedInsertionBlock();

  if (needDeletionBlock && !needInsertionBlock)
  {
    // Need just the Delta Deletion block.
    topNode = buildMjvDeletionBlock();
  }
  else if (!needDeletionBlock && needInsertionBlock)
  {
    // Need just the Delta Insertionblock.
    topNode = buildMjvInsertionBlock();

    if (topNode == NULL)
      return NULL;
  }
  else if (!needDeletionBlock && !needInsertionBlock)
  {
    // Actually, we don't need any of them, because only unused columns
    // were updated. Create a NO-OP: a Tuple node that does nothing.
    ItemExpr *noOpArg = new (heap_) SystemLiteral(0);
    topNode = new (heap_) Tuple(noOpArg);
  }
  else
  {
    // Need both blocks.
    // Build the Delta Deletion Block
    RelExpr *deletionBlock = buildMjvDeletionBlock();
    // Build the Delta Insertion Block
    RelExpr *insertionBlock = buildMjvInsertionBlock();

    if (insertionBlock == NULL)
      return NULL;

    // And the ordered union that connects them.
    Union *unionNode = new(heap_) 
      Union(deletionBlock, insertionBlock, NULL, NULL,
	    REL_UNION, CmpCommon::statementHeap(), TRUE);
    unionNode->setOrderedUnion();

    topNode = unionNode;
  }
  
  RelRoot *topRoot = new(heap_) RelRoot(topNode);

  // There are no outputs here.
  topRoot->setEmptySelectList();

  return topRoot;
}

//----------------------------------------------------------------------------
// A deletion block is needed when the IUD log has deleted rows
// or updated rows where atleast one update column is used by the MV
//----------------------------------------------------------------------------
NABoolean MjvOnRequestBuilder::isNeedDeletionBlockSpecific(DeltaDefinition *deltaDef)
{
  if ( (deltaDef->getIudDeletedRows() == 0) &&
       ((deltaDef->getIudUpdatedRows() == 0) ||
        deltaDef->updateColumnsNotUsedByMV(getMvInfo())) )
    return FALSE;
  else
    return TRUE;
}

//----------------------------------------------------------------------------
// A deletion block is needed when the IUD log has deleted rows
// or updated rows where atleast one update column is used by the MV
//----------------------------------------------------------------------------
NABoolean MjvOnRequestBuilder::isNeedDeletionBlock()
{
  // This class handles single delta only.
  CMPASSERT(getDeltaDefList()->entries() == 1); 
  DeltaDefinition *deltaDef = (*getDeltaDefList())[0];

  return isNeedDeletionBlockSpecific(deltaDef);
}

//----------------------------------------------------------------------------
// A direct update is needed only when the IUD log has updated rows
// and atleast one of the updated columns is a DIRECT update column
//----------------------------------------------------------------------------
NABoolean MjvOnRequestBuilder::isNeedDirectUpdateSpecific(DeltaDefinition *deltaDef)
{
  return (deltaDef->getIudUpdatedRows() > 0 &&
          deltaDef->containsDirectUpdateColumn(getMvInfo()));
}

//----------------------------------------------------------------------------
// An inserion block is needed when the IUD log has inserted rows or 
// updated rows where atleast one of the updated columns is an INDIRECT
// update column, or when the range log is not empty.
//----------------------------------------------------------------------------
NABoolean MjvOnRequestBuilder::isNeedInsertionBlockSpecific(DeltaDefinition *deltaDef)
{
  if ( (deltaDef->getIudInsertedRows() > 0) ||
       ((deltaDef->getIudUpdatedRows()  > 0) && 
        (deltaDef->containsIndirectUpdateColumn(getMvInfo()))) ||
       (deltaDef->useRangeLog() == TRUE)  )
    return TRUE;
  else
    return FALSE;
}

//----------------------------------------------------------------------------
// An inserion block is needed when the IUD log has inserted rows or
// updated rows where atleast one of the updated columns is an INDIRECT
// update column, or when the range log is not empty.
//----------------------------------------------------------------------------
NABoolean MjvOnRequestBuilder::isNeedInsertionBlock()
{
  // This class handles single delta only.
  CMPASSERT(getDeltaDefList()->entries() == 1); 
  DeltaDefinition *deltaDef = (*getDeltaDefList())[0];

  return isNeedInsertionBlockSpecific(deltaDef);
}

//----------------------------------------------------------------------------
// Single delta version.
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestBuilder::buildMjvDeletionBlock()
{
  CMPASSERT(getDeltaDefList()->entries() == 1); 
  DeltaDefinition *deltaDef = (*getDeltaDefList())[0];

  return buildMjvDeletionBlockSpecific(deltaDef);
}

//----------------------------------------------------------------------------
// Build the MJV Delete block for a specific delta.
// The MJV Deletion block includes handling MJV rows that need to be deleted 
// or directly updated. It looks like this:
//
//                RelRoot
//                   |
//                  TSJ
//                 /   \
//             Scan     IF
//             Log     /   \
//                    /     \  
//                Delete    Update
//                 MJV       MJV
//                  |         |
//                Scan       Scan
//                MJV        MJV
//
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestBuilder::buildMjvDeletionBlockSpecific(DeltaDefinition *deltaDef)
{
  LogsInfo *logsInfo = new LogsInfo(*deltaDef, getMvInfo(), bindWA_);

  // The correlation name of the IUD log is @LOG<block-number>
  const CorrName logCorrName(*getLogName());

  RelExpr *rightSide = NULL;

  // We need Delete block when there are deleted rows or updated rows
  // where atleast one updated column is an INDIRECT update column
  if (deltaDef->getIudDeletedRows() > 0  ||
      ((deltaDef->getIudUpdatedRows() > 0) &&
       deltaDef->containsIndirectUpdateColumn(getMvInfo())))
  {
    rightSide = buildMjvDeleteSubtree(logsInfo, logCorrName);
  }

  // But not always the Direct Update block.
  if (isNeedDirectUpdateSpecific(deltaDef))
  {
    RelExpr *UpdateMjvSide = buildMjvUpdateSubtree(logsInfo, logCorrName);

    if (rightSide != NULL)
      rightSide = buildIfNode(rightSide, UpdateMjvSide);
    else
    {
      rightSide = UpdateMjvSide;
    }
  }

  CMPASSERT(rightSide != NULL);

  const QualifiedName *baseTableName = deltaDef->getTableName();
  RelExpr *scanLog = buildScanLogForOnRequestMjv(*baseTableName, TRUE);
  // Add a Rename node on top to rename the output table to our needs.
  RelExpr *leftSide = new(heap_) 
    RenameTable(TRUE, scanLog, logCorrName);

  // Stream the data from the Scan node to the Delete and Update nodes.
  Join *tsjNode = new(heap_) Join(leftSide, rightSide, REL_TSJ);
  tsjNode->setTSJForWrite(TRUE);

  // Add a root node with an empty select list so we have no outputs.
  RelRoot *topNode = new(heap_) RelRoot(tsjNode, REL_ROOT);
  topNode->setEmptySelectList();
  return topNode;
}

//----------------------------------------------------------------------------
// Build the Delete MJV node.
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestBuilder::buildMjvDeleteSubtree(LogsInfo       *logsInfo, 
						    const CorrName &logCorrName)
{
  const NATable *baseNaTable = logsInfo->getBaseNaTable();

  // Build scan node of the MJV
  Scan *scanNode = buildScanOfMJV(logCorrName, baseNaTable);

  GenericUpdate *guNode = new (heap_) 
      Delete(getMvCorrName(), NULL, REL_UNARY_DELETE, scanNode);
 
  // Affected rows should not be counted.
  guNode->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;  

  // The refresh sub-tree doesn't output anything
  RelRoot *topNode = new(heap_) RelRoot(guNode);
  topNode->setEmptySelectList();

  return topNode;
}

//----------------------------------------------------------------------------
// Build the Update MJV node.
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestBuilder::buildMjvUpdateSubtree(LogsInfo       *logsInfo, 
						    const CorrName &logCorrName)
{
  const NATable *baseNaTable = logsInfo->getBaseNaTable();

  // Build scan node of the MJV
  Scan *scanNode = buildScanOfMJV(logCorrName, baseNaTable);

  // construct a set of all affected columns in the MJV. All the affected
  // columns will have to have an assignment expression.
  IntegerList updatedCols;
  if (logsInfo->getDeltaDefinition().getUpdatedColumnList() != NULL)
    updatedCols.insert(*logsInfo->getDeltaDefinition().getUpdatedColumnList());

  // Remove from the list columns that are in the indirect update column list,
  // because for the rows for which they are updated, we don't get here.
  const MVUsedObjectInfo *usedObject =
    getMvInfo()->findUsedInfoForTable(baseNaTable->getTableName());
  const LIST(Lng32) &mvInfoIndirectCols = usedObject->getIndirectUpdateCols();
  for (CollIndex i=0; i < mvInfoIndirectCols.entries(); i++) 
  {
    Lng32 currentTempCol = mvInfoIndirectCols[i];
    if (updatedCols.contains(currentTempCol))
    {
      updatedCols.remove(currentTempCol);
    }
  }

  const QualifiedName &qualTableName = logsInfo->getBaseTableName().getQualifiedNameObj();
  SET(MVColumnInfo*) SetOfAffectedColumns = 
    collectAllAffectedColumns(&updatedCols, qualTableName);

  ItemExpr *updatePredicate = 
    buildUpdatePredicate(logCorrName, 
  			 baseNaTable,
		         SetOfAffectedColumns);

  GenericUpdate *guNode = new(heap_)
      Update(getMvCorrName(), NULL, REL_UNARY_UPDATE, scanNode, updatePredicate );
 
  // Affected rows should not be counted.
  guNode->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;  

  // The refresh sub-tree doesn't output anything
  RelRoot *topNode = new(heap_) RelRoot(guNode);
  topNode->setEmptySelectList();

  return topNode;
}

//----------------------------------------------------------------------------
// Build the IF node that gets the IUD log data from above and directs it 
// to either the Delete MJV subtree, or the direct update subtree.
// The Scan log node projects only this types of rows:
//   Type 2 : Deleted rows.
//   Type 6 : The deleted row of an indirect update couple.
//   Type 4 : The inserted row of a direct update couple.
// (Couple means the two insert-delete log rows of an update operation).
// The selection predicate on the updated columns (to differentiate direct
// from indirect update) was already done by the Scan log block. So all 
// there's left to do here is to check the OPERATION_TYPE of the row to
// see if its a deleted row (type 2 or 6) or an inserted row (type 4).
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestBuilder::buildIfNode(RelExpr *leftSide, RelExpr *rightSide)
{
  ItemExpr *condition =
    BinderUtils::buildPredOnCol(ITM_NOT_EQUAL, 
				COMMV_OPTYPE_COL,            // "@OPERATION_TYPE"
                                ComMvRowType_InsertOfUpdate, // Type 4 row = 2
				heap_);

  // The IF is implemented as a conditional Union.
  RelExpr *ifNode = new(heap_) 
    Union(leftSide, rightSide, NULL, condition,
          REL_UNION, CmpCommon::statementHeap(), TRUE);

  RelRoot *topNode = new(heap_) RelRoot(ifNode, REL_ROOT);
  topNode->setEmptySelectList();
  return topNode;
}

//----------------------------------------------------------------------------
// Building a block to handle inserted rows. 
// We let the MJV select tree compute the delta, and then insert it into 
// the MJV. We use the MvBindContext mechanism to replace the scan on the 
// base table by a scan on the IUD log.
//
// The resulting tree (after binding) will look like this:
//
//       Insert MJV
//           |
//        RelRoot
//           |
//          /^\
//         /   \   - MJV select query
//        <_____>
//         |
//         |
//     Scan IUD log
//
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestBuilder::buildMjvInsertionBlock()
{
  RelExpr *joinExpression = buildMjvInsertJoin();

  if (joinExpression == NULL)
    return NULL;

  // Put Insert node on top of it.
  GenericUpdate *guNode = new (heap_) 
    Insert(getMvCorrName(), NULL, REL_UNARY_INSERT, joinExpression);

  RelRoot *topNode = new(heap_) RelRoot(guNode);
  topNode->setEmptySelectList();

  return topNode;
}

//----------------------------------------------------------------------------
// Build the join expression that feeds the Insert node (Single delta version).
// This method is overridden by the Multi-Delta subclass.
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestBuilder::buildMjvInsertJoin()
{
  CMPASSERT(getDeltaDefList()->entries() == 1); 
  DeltaDefinition *deltaDef = (*getDeltaDefList())[0];

  return buildMjvInsertJoinSpecific(deltaDef);
}

//----------------------------------------------------------------------------
// Build the join expression that feeds the Insert node, for a specific delta.
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestBuilder::buildMjvInsertJoinSpecific(DeltaDefinition *deltaDef)
{
  // Get the MJV tree
  RelRoot *mvSelectTree = getMvInfo()->buildMVSelectTree();
  const QualifiedName *baseTableName = deltaDef->getTableName();

  RelExpr *scanLogNode = buildScanLogForOnRequestMjv(*baseTableName, FALSE);

  // Setup a new MvBindContext for replacing the scanning on the subject table
  bindWA_->setBindingMvRefresh(); // enabling MvBindContext mechanism
  setupBindContext(mvSelectTree, scanLogNode, baseTableName);

  return mvSelectTree;
}

//----------------------------------------------------------------------------
// This method adds MJV specific stuff on top of the MV generic log scanning 
// block (that includes IUD + Range log, Union backbone for sorted output etc.)
// The MJV specific stuff is:
// 1. A selection predicate on the specific row types needed by each side
//    of the tree: row types 2, 4, and 6 for the delete\update side, and
//    row types 1 and 7 for the insert side. (see details in the INTERNAL
//    REFRESH document, MJV chapter).
// 2. Filter only the base table columns (+ @OPERATION_TYPE for the delete side),
//    and Rename @SYSKEY to SYSKEY.
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestBuilder::buildScanLogForOnRequestMjv(const QualifiedName& baseTableName,
							  NABoolean isForDelete)
{
  // Build the entire log scanning block (including range log and union backbone)
  RelExpr *scanLogBlock = MvRefreshBuilder::buildLogsScanningBlock(baseTableName);

  // Build a selection predicate on the specific row types for MJV refresh
  ItemExpr *rowTypePredicates = NULL;
  if (isForDelete)
  {
    // This is for row types 2,4 and 6
    rowTypePredicates = buildDeleteSidePredicate();
  }
  else
  {
    // This is for row tyes 1 and 7
    rowTypePredicates = buildInsertSidePredicate();
  }

  // First apply the selection predicate on the @INDIRECT column
  scanLogBlock->addSelPredTree(rowTypePredicates);

  // Then filter only the needed columns (not including @INDIRECT).

  // Select only the base table columns for insertion into the MJV: (*, "@SYSKEY" AS SYSKEY).
  ColRefName *star = new(heap_) ColRefName(1);  // isStar
  ItemExpr *selectList = new(heap_) ColReference(star);

  // Check if the log table has a @SYSKEY column
  CorrName logCorrName(baseTableName);
  logCorrName.setSpecialType(ExtendedQualName::IUD_LOG_TABLE);
  const NATable *logNaTable = bindWA_->getNATable(logCorrName);
  NAColumn *atSyskey = logNaTable->getNAColumnArray().getColumn(COMMV_BASE_SYSKEY_COL);
  if (atSyskey != NULL)
  {
    // The log includes an @SYSKEY column - add it to the list 
    // and rename it to SYSKEY.
    ItemExpr *skColref = new(heap_) ColReference(new(heap_) ColRefName(COMMV_BASE_SYSKEY_COL));
    ItemExpr *skRename = new(heap_) RenameCol(skColref, new(heap_) ColRefName("SYSKEY"));
    selectList = new(heap_) ItemList(selectList, skRename);
  }

  // Add a root node for the select list.
  RelRoot *rootNode = new(heap_) 
    RelRoot(scanLogBlock, REL_ROOT, selectList);

  return rootNode;
}

//----------------------------------------------------------------------------
// Override superclass implementation to provide MJV specific functionality.
// This version is for the Insert side, and used by multi-delta methods.
// The delete version is called directly so it does not need the overridden method.
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestBuilder::buildLogsScanningBlock(const QualifiedName& baseTable)
{
  return buildScanLogForOnRequestMjv(baseTable, FALSE);
}

//----------------------------------------------------------------------------
// The Delete/Update side needs the following row types:
// RowType (see document)        | @OPERATION_TYPE  | @INDIRECT
// ==============================+==================+=============
// 2 - Delete                    | DELETE           | n/a
// 4 - Insert of Direct Update   | INSERT of UPDATE | FALSE
// 6 - Delete of Indirect Update | DELETE of UPDATE | TRUE
//----------------------------------------------------------------------------
ItemExpr *MjvOnRequestBuilder::buildDeleteSidePredicate()
{
  ItemExpr *type2 = buildPredicateOnOpType(ComMvRowType_Delete);

  ItemExpr *type4 = new(heap_) 
    BiLogic(ITM_AND,
            buildPredicateOnOpType(ComMvRowType_InsertOfUpdate),
	    buildPredicateOnIndirect(ITM_RETURN_FALSE));

  ItemExpr *type6 = new(heap_) 
    BiLogic(ITM_AND,
            buildPredicateOnOpType(ComMvRowType_DeleteOfUpdate),
	    buildPredicateOnIndirect(ITM_RETURN_TRUE));
    
  return new(heap_)
    BiLogic(ITM_OR, type2, new(heap_) BiLogic(ITM_OR, type4, type6));              
}

//----------------------------------------------------------------------------
// The Insert side needs the following row types:
// RowType (see document)        | @OPERATION_TYPE  | @INDIRECT
// ==============================+==================+=============
// 1 - Insert                    | INSERT           | n/a
// 7 - Insert of Indirect Update | INSERT of UPDATE | TRUE
//----------------------------------------------------------------------------
ItemExpr *MjvOnRequestBuilder::buildInsertSidePredicate()
{
  ItemExpr *type1 = buildPredicateOnOpType(ComMvRowType_Insert);

  ItemExpr *type7 = new(heap_) 
    BiLogic(ITM_AND,
            buildPredicateOnOpType(ComMvRowType_InsertOfUpdate),
	    buildPredicateOnIndirect(ITM_RETURN_TRUE));
  
  return new(heap_)
      BiLogic(ITM_OR, type1, type7);
}

//----------------------------------------------------------------------------
// Build the predicate (@OPERATION_TYPE = opType)
//----------------------------------------------------------------------------
ItemExpr *MjvOnRequestBuilder::buildPredicateOnOpType(ComMvIudLogRowType opType)
{
  ItemExpr *opTypeCol = new(heap_)
    ColReference(new(heap_) ColRefName(COMMV_OPTYPE_COL) );

  return new(heap_) BiRelat(ITM_EQUAL,
                            opTypeCol,
  		            new(heap_) SystemLiteral(opType) );
}

//----------------------------------------------------------------------------
// Build the predicate (@INDIRECT = val)
//----------------------------------------------------------------------------
ItemExpr *MjvOnRequestBuilder::buildPredicateOnIndirect(OperatorTypeEnum val)
{
  ItemExpr *indirectCol = new(heap_)
    ColReference(new(heap_) ColRefName(getVirtualIndirectColumnName()) );

  return new(heap_) BiRelat(ITM_EQUAL,
	                    indirectCol,
			    new(heap_) BoolVal(val) );
}

//----------------------------------------------------------------------------
// Override method from MvRefreshBuilder to add the indirect update flag
// instead of @OP. So here, the select list is: 
//     (*, <indirect-update-expression> AS @INDIRECT )
//----------------------------------------------------------------------------
ItemExpr *MjvOnRequestBuilder::buildSelectionListForScanOnIudLog() const
{
  MvIudLog &iudLog = getLogsInfo().getMvIudlog();
  ItemExpr *indirectExpr = iudLog.buildIndirectUpdateExpression();

  CorrName logName(*getLogName());

  RenameCol *indirectCol = new(heap_) 
    RenameCol(indirectExpr, 
              new(heap_) ColRefName(getVirtualIndirectColumnName(), logName));

  ColRefName *star = new(heap_) ColRefName(1);  // isStar
  star->setStarWithSystemAddedCols();  // Don't block system added columns .
  ItemExpr *starExpr = new(heap_) ColReference(star);

  ItemExpr *rootSelectList = new(heap_) ItemList(starExpr, indirectCol);

  return rootSelectList;
}

//----------------------------------------------------------------------------
// We get here when we need to read from the Range log.
// The Union between the range log and the IUD log should have the @INDIRET
// and the @OPERATION_TYPE columns on both sides, as they are later used 
// by the INDIRECT predicate.
//----------------------------------------------------------------------------
// Exclude from coverge testing - used only for range logging
RelRoot *MjvOnRequestBuilder::buildRootOverIUDLog(RelExpr *topNode) const
{
  ItemExpr *OperationTypeCol = new(heap_) 
    ColReference(new(heap_) 
      ColRefName(COMMV_OPTYPE_COL, getLogsInfo().getIudLogTableName(), heap_));

  ItemExpr *IndirectCol = new(heap_) 
    ColReference(new(heap_) 
      ColRefName(getVirtualIndirectColumnName(), getLogsInfo().getIudLogTableName(), heap_));

  RelRoot *result = MvRefreshBuilder::buildRootOverIUDLog(topNode);
  result->addCompExprTree(OperationTypeCol);
  result->addCompExprTree(IndirectCol);
  return result;
}

//----------------------------------------------------------------------------
// We get here when we need to read from the Range log.
// The Union between the range log and the IUD log should have the @INDIRET
// and the @OPERATION_TYPE columns on both sides, as they are later used 
// by the INDIRECT predicate.
//----------------------------------------------------------------------------
// Exclude from coverge testing - used only for range logging
RelRoot *MjvOnRequestBuilder::buildRootOverRangeBlock(RelExpr *topNode) const
{
  ItemExpr *OperationTypeCol = new(heap_) 
    RenameCol(new(heap_) SystemLiteral(ComMvRowType_Insert), 
              new(heap_) ColRefName(COMMV_OPTYPE_COL));

  ItemExpr *IndirectCol = new(heap_) 
    RenameCol(new(heap_) SystemLiteral(), // NULL
              new(heap_) ColRefName(getVirtualIndirectColumnName()));

  RelRoot *result = MvRefreshBuilder::buildRootOverRangeBlock(topNode);
  result->addCompExprTree(OperationTypeCol);
  result->addCompExprTree(IndirectCol);
  return result;
}

//----------------------------------------------------------------------------
// Have a uniform select list over the logs
// Override MvRefreshBuilder method, so @OP is not added to the list
// for MJV. This is because @OP is only used in the deletion part of MJV
// refresh, where the range log is not read, so we never get here.
// We do get here for the insertion part, where we must not have @OP.
//----------------------------------------------------------------------------
// Exclude from coverge testing - used only for range logging
RelRoot *MjvOnRequestBuilder::buildRootWithUniformSelectList(RelExpr *topNode,
							  ItemExpr *opExpr,
							  const CorrName *nameOverride) const
{
  ItemExpr *rootSelectList = 
    BinderUtils::buildClusteringIndexVector(
				  getLogsInfo().getBaseNaTable(), 
				  heap_, 
				  nameOverride, 
				  SP_ALL_COLUMNS | SP_SYSKEY_AS_USER | SP_USE_AT_SYSKEY);

  return new(heap_) RelRoot(topNode, REL_ROOT, rootSelectList);
}


// ===========================================================================
// ===========================================================================
// =============  class  MjvOnRequestMultiDeltaBuilder  ======================
// ===========================================================================
// ===========================================================================

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const NAString *MjvOnRequestMultiDeltaBuilder::getLogName() const
{
  char name[20]; // 11 + 4 + 1 = 16 bytes
  snprintf(name, sizeof(name), "@LOG%d", deleteBlockCounter_);
  return new (heap_) NAString(name, heap_);
}

//----------------------------------------------------------------------------
// Is there any delta for which we need the Deletion block?
//----------------------------------------------------------------------------
NABoolean MjvOnRequestMultiDeltaBuilder::isNeedDeletionBlock() 
{ 
  const DeltaDefinitionPtrList *deltaDefList = getDeltaDefList();
  for (CollIndex i=0; i<deltaDefList->entries(); i++)
  {
    DeltaDefinition *deltaDef = (*getDeltaDefList())[i];
    if (isNeedDeletionBlockSpecific(deltaDef))
      return TRUE;
  }

  return FALSE;
}

//----------------------------------------------------------------------------
// Is there any delta for which we need the Insertion block?
//----------------------------------------------------------------------------
NABoolean MjvOnRequestMultiDeltaBuilder::isNeedInsertionBlock() 
{ 
  const DeltaDefinitionPtrList *deltaDefList = getDeltaDefList();
  for (CollIndex i=0; i<deltaDefList->entries(); i++)
  {
    DeltaDefinition *deltaDef = (*getDeltaDefList())[i];
    if (isNeedInsertionBlockSpecific(deltaDef))
      return TRUE;
  }

  return FALSE;
}

//----------------------------------------------------------------------------
// Build the Union backbone to collect Deletion blocks for all deltas.
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestMultiDeltaBuilder::buildMjvDeletionBlock() 
{
  RelExpr *topNode = NULL;
  const DeltaDefinitionPtrList *deltaDefList = getDeltaDefList();

  // Loop over all deltas.
  for (CollIndex i=0; i<deltaDefList->entries(); i++)
  {
    DeltaDefinition *deltaDef = (*getDeltaDefList())[i];
    // Does this specific delta need a Deletion block?
    if (!isNeedDeletionBlockSpecific(deltaDef))
      continue;

    // Build the Deletion block for this delta
    RelExpr *newBlock = buildMjvDeletionBlockSpecific(deltaDef);
    incrementDeleteBlockCounter();
    
    // Build the Union backbone using Ordered Union nodes.
    if (topNode == NULL)
      topNode = newBlock;
    else
    {
      Union *unionNode = new (heap_) 
	Union(topNode, newBlock, NULL, NULL,
	      REL_UNION, CmpCommon::statementHeap(), TRUE);
      unionNode->setOrderedUnion();
      topNode = unionNode;
    }
  }

  // If Deletion block is not needed for any table, we should not have gotten here.
  CMPASSERT(topNode != NULL);

  return topNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestMultiDeltaBuilder::buildMjvInsertJoin()
{
  // supportPhases = FALSE, checkOnlyInserts = TRUE
  MultiDeltaRefreshMatrix *productMatrix = prepareProductMatrix(FALSE, TRUE);

  if (productMatrix == NULL)
    return NULL;
  else
    return buildDeltaCalculationTree(*productMatrix);
}

//----------------------------------------------------------------------------
// The Insertion delta calculation tree is built from join products, each 
// of them is the MV select tree. The join proiducts are joined together by 
// either a Union (for + rows), or an Anti- Semijoin (for - rows).
// This is the algorithm:
// 1. Get the MV select tree.
// 2. For each revevant row in the product matrix: 
// 2.1. copy the join product and have it read only the logs from the 
//      corresponding matrix row.
// 2.2. Connect the product to the tree using a Union  or Anti SemiJoin node.
// 3. If duplicates can be optimized (when all deltas are insert-only)
//    Set the top Union node to a DISTINCT Union.
// Note that even though division to phases is not currently supported for MJV
// multi-delta refresh, this code is ready to support it once it will be implemented.
//----------------------------------------------------------------------------
RelExpr *MjvOnRequestMultiDeltaBuilder::buildDeltaCalculationTree(
				const MultiDeltaRefreshMatrix& productMatrix)
{
  // 1. Get the tree for the MV select expression.
  RelRoot *rawJoinProduct = getMvInfo()->buildMVSelectTree();

  // 2. For each relevant product matrix row
  RelExpr *topNode = NULL; 
  const NAString rightName("@RIGHT");
  const NAString leftName("@LEFT");
  const CorrName leftCorrName(leftName);

  Int32 numOfRowsForThisPhase = productMatrix.getNumOfRowsForThisPhase();
  Int32 firstRowForThisPhase  = productMatrix.getFirstRowForThisPhase();
  for (Int32 rowNumber = firstRowForThisPhase; 
       rowNumber < firstRowForThisPhase + numOfRowsForThisPhase; 
       rowNumber++)
  {
    // 2.1, Prepare the join product according to the matrix row.
    NABoolean isLastRow =
      rowNumber == firstRowForThisPhase + numOfRowsForThisPhase - 1;

    RelRoot *product =
      prepareProductFromMatrix(productMatrix, 
			       rawJoinProduct, 
			       rowNumber, 
			       isLastRow);

    // 2.2 Add it to the Union tree.
    if (topNode == NULL)
    {
      // Make sure the sign of the first row is +.
      CMPASSERT(productMatrix.getRow(rowNumber)->isSignPlus());
      topNode = new RenameTable(product, leftName);
    }
    else
    {
      if (productMatrix.getRow(rowNumber)->isSignPlus())
      {
	// Plus matrix rows: use a Union node
        topNode = new(heap_) 
	  Union(topNode, product, NULL, NULL, REL_UNION,
                CmpCommon::statementHeap(), TRUE);
      }
      else
      {
	// Minus matrix rows: Use Anti-Semi Join

	// Give the right join product the @RIGHT corr name.
        RelExpr *rightProduct = new RenameTable(product, rightName);

	// Build the join predicate on the MJV clustering index columns.
        ItemExpr *joinPredicate = buildAntiSemiJoinPredicate(rightName, leftName);
        Join *joinNode = new(heap_) 
	  Join(topNode, rightProduct, REL_ANTI_SEMITSJ, joinPredicate);

	// Now add a root node with a selection predicate: (@LEFT.*)
	ItemExpr *selectionPredicate = new (heap_)
	  ColReference(new (heap_) ColRefName(1, leftCorrName));

	topNode = new(heap_)
	  RelRoot(joinNode, REL_ROOT, selectionPredicate);
      }
    }
  }

  // 3. If duplicates can be optimized (when all deltas are insert-only)
  //    Set the top Union node to a DISTINCT Union.
  CMPASSERT(productMatrix.isDuplicatesOptimized());
  if (productMatrix.isDuplicatesOptimized())
  {
    // Make sure the top most node is a Union.
    // There is a chance that only a single delta has inserts, so all the 
    // others are ignored here.
    if (topNode->getOperatorType() == REL_UNION)
    {
      Union *topUnion = (Union*) topNode;
      topUnion->setDistinctUnion();

      topNode = new (heap_)
	RelRoot(new (heap_)
	  GroupByAgg(new (heap_) RelRoot(topUnion),
		     REL_GROUPBY, 
		     new (heap_) ColReference(new (heap_) ColRefName(TRUE, heap_))));
    }
  }

  return topNode;
}

//----------------------------------------------------------------------------
// Build a predicate on the clustering index columns of the MJV, for use 
// in the Anti-Semi Join, for subtracting join products with a minus sign.
//----------------------------------------------------------------------------
ItemExpr *MjvOnRequestMultiDeltaBuilder::buildAntiSemiJoinPredicate(NAString rightName, 
								    NAString leftName)

{
  ItemExpr *result = NULL;

  // Get the clustering index columns of the MJV
  CorrName mvName(getMvCorrName());
  const NAColumnArray &indexColumns = 
    bindWA_->getNATable(mvName)->getClusteringIndex()->getIndexKeyColumns();

  // The predicate is: @LEFT.<col> = @RIGHT.<col>
  const CorrName *rightCorrName = new (heap_) CorrName(rightName);
  const CorrName *leftCorrName  = new (heap_) CorrName(leftName);

  // For each column in the clustering index of the MJV
  for (CollIndex i=0; i<indexColumns.entries(); i++)
  {
    NAString colName = indexColumns[i]->getColName();

    // Skip the MV SYSKEY column because it is not created by the join product.
    if (colName == "SYSKEY")
      continue;

    ItemExpr *rightColExpr = new(heap_) 
      ColReference(new(heap_) ColRefName(colName, *rightCorrName, heap_));

    ItemExpr *leftColExpr = new(heap_) 
      ColReference(new(heap_) ColRefName(colName, *leftCorrName, heap_));

    ItemExpr *equalExpr = new (heap_) 
      BiRelat(ITM_EQUAL, leftColExpr, rightColExpr);

    if (result == NULL)
      result = equalExpr;
    else
      result = new(heap_) BiLogic(ITM_AND, result, equalExpr);
  }

  return result;
}  // buildClusteringIndexVector()

