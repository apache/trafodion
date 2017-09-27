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
* File:         ChangesTable.cpp
* Description:  Insertion, scanning and deleting from a changes table
*               such as the triggers temporary table, and the MV log.
* Created:      10/10/2000
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "NumericType.h"
#include "AllRelExpr.h"
#include "AllItemExpr.h"
#include "BindWA.h"
#include "NATable.h"
#include "ChangesTable.h"
#include "ComMvDefs.h"
#include "Refresh.h"
#include "MvLog.h"
#include "MVInfo.h"
#include "parser.h"
#include "ItmFlowControlFunction.h"

/*****************************************************************************
******************************************************************************
****  Class TriggersTempTable
******************************************************************************
*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
ChangesTable::ChangesTable(const CorrName&   name, 
			   OperatorTypeEnum  opType,
			   BindWA           *bindWA,
			   RowsType          scanType)
  : naTable_(NULL),
    subjectTable_(name,bindWA->wHeap()),
    subjectNaTable_(NULL),
    opType_(opType),
    tableCorr_(NULL),
    scanType_(scanType),
    scanNode_(NULL),
    corrNameForNewRow_(NEWCorr,bindWA->wHeap()),
    corrNameForOldRow_(OLDCorr,bindWA->wHeap()),
    bindWA_(bindWA),
    heap_(bindWA->wHeap())
{
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
ChangesTable::ChangesTable(const GenericUpdate *baseNode, 
			   BindWA              *bindWA)
  : naTable_(NULL),
    subjectTable_(baseNode->getTableDesc()->getCorrNameObj(),bindWA->wHeap()),
    subjectNaTable_(baseNode->getTableDesc()->getNATable()),
    opType_(baseNode->getOperatorType()),
    tableCorr_(NULL),
    scanNode_(NULL),
    corrNameForNewRow_(NEWCorr,bindWA->wHeap()),
    corrNameForOldRow_(OLDCorr,bindWA->wHeap()),
    bindWA_(bindWA),
    heap_(bindWA->wHeap())
{
  CMPASSERT(baseNode != NULL);
  if (baseNode->getUpdateCKorUniqueIndexKey() &&
      baseNode->getOperatorType() == REL_UNARY_INSERT)
	opType_ = REL_UNARY_UPDATE;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
ChangesTable::ChangesTable(Scan     *baseNode, 
			   RowsType  scanType, 
			   BindWA   *bindWA)
  : subjectTable_(baseNode->getTableName(),bindWA->wHeap()),
    subjectNaTable_(NULL),
    naTable_(NULL),
    opType_(baseNode->getOperatorType()),
    tableCorr_(NULL),
    scanType_(scanType),
    scanNode_(baseNode),
    corrNameForNewRow_(NEWCorr,bindWA->wHeap()),
    corrNameForOldRow_(OLDCorr,bindWA->wHeap()),
    bindWA_(bindWA),
    heap_(bindWA->wHeap())

{
  CMPASSERT(baseNode != NULL);
}

//////////////////////////////////////////////////////////////////////////////
// This method is called after the Ctors of sub-classes of ChangesTable,
// in order to initialize the changes table name and NATable pointer.
// calcTargetTableName() is a pure virtual function, and so cannot be called 
// by the base class Ctor.
//////////////////////////////////////////////////////////////////////////////
void ChangesTable::initialize()
{
  CMPASSERT(subjectTable_ != ""); // Subject table name must be initialized.

  // Calculate the changes table name from the subject table name.
  const NATable *subjectNaTable = getSubjectNaTable();
  if (subjectNaTable && subjectNaTable->getIsSynonymTranslationDone())
  {
    QualifiedName subjectQualName(subjectNaTable->getSynonymReferenceName(),
                                  3, // minNameParts
                                  bindWA_->wHeap(),
                                  bindWA_);
    tableCorr_ = calcTargetTableName(subjectQualName);
  }
  else
    tableCorr_ =
    calcTargetTableName(getSubjectTableName().getQualifiedNameObj());

  // If the table is using late name resolution, the log should as well.
  if (getSubjectTableName().getPrototype() && supportsLateBinding())
  {
    HostVar *logPrototype = new(bindWA_->wHeap())
      HostVar(*getSubjectTableName().getPrototype());

    // Mark the table type on the prototype.
    logPrototype->setSpecialType(tableCorr_->getSpecialType());
    tableCorr_->setPrototype(logPrototype);
  }

  // Save the state of the RI flag
  BindContext *currentBindContext = bindWA_->getCurrentScope()->context();
  NABoolean prevRiState = currentBindContext->inRIConstraint();
  // Set the RI flag to allow calling getNATable() without counting it.
  currentBindContext->inRIConstraint() = TRUE;

  // Get the NATable of the changes table.
  naTable_ = bindWA_->getNATable(*tableCorr_);

  // if we hit any error while constructing naTable_, we should
  // catch it here as we will try to dereference it 
  CMPASSERT(naTable_!=NULL);

  if (subjectNaTable_ == NULL)
  {
    subjectNaTable_ = bindWA_->getNATable(subjectTable_);
    CMPASSERT(subjectNaTable_) // NATable must have been found!
  }

  // Restore the RI flag.
  currentBindContext->inRIConstraint() = prevRiState;
}

//////////////////////////////////////////////////////////////////////////////
// build the node to insert the OLD@ and NEW@ data into the changes table.
// Insert and Delete operations each cause a single row to be inserted into
// the changes table per affected set row. Update operations cause two rows
// to be inserted into the changes table per affected set row: one with the
// OLD data and one with the NEW data.
// For update operations, we can enforce insertion of only one set (NEW/OLD)
// using the second parameter. This parameter is defaulted to ALL_ROWS, so if
// not explicitly requested, it doesn't affect the insert at whole.
//////////////////////////////////////////////////////////////////////////////
RelExpr *ChangesTable::buildInsert(NABoolean useLeafInsert,
				   Lng32 enforceRowsTypeForUpdate,
				   NABoolean isUndo,
				   NABoolean isForMvLogging
                                  ) const
{
  ItemExprList   *tupleColRefList = NULL;
  ItemExpr       *insItemList = NULL;
  ItemExpr       *delItemList = NULL;
  NABoolean       needOld=FALSE, needNew=FALSE;
  NABoolean	  isUpdate=FALSE, isInsert=FALSE, isDelete=FALSE;

  // Get the columns of the temp table.
  const NAColumnArray &tempColumns = getNaTable()->getNAColumnArray(); 

  switch (getOpType())  // What type of GenericUpdate node is this?
  {	
    case REL_UNARY_INSERT:  // hhhheeeelllpppp - I'm falling to next case
    case REL_LEAF_INSERT:   needNew=TRUE;
                            isInsert=TRUE;
			    break;    
    case REL_UNARY_DELETE:  needOld=TRUE;
                            isDelete=TRUE;
			    break;
    case REL_UNARY_UPDATE:  needNew=TRUE; 
			    needOld=TRUE;
			    isUpdate=TRUE;
			    break;
    default : CMPASSERT(FALSE);
  }

  // For update operations, the second parameter controls which type of rows
  // is inserted into the temp-table. When both sets of values are inserted,
  // the insert is unary insert. When only one set of values is inserted, a
  // leaf insert is used.
  if (getOpType() == REL_UNARY_UPDATE)
  {
    CMPASSERT(needOld && needNew);

    if (!(enforceRowsTypeForUpdate & INSERTED_ROWS))
    {
      needNew = FALSE;
    }

    if (!(enforceRowsTypeForUpdate & DELETED_ROWS))
    {
      needOld = FALSE;
    }

    // use LeafInsert node for UPDATE operation as well, unless we need both
    // sets of values (NEW and OLD).
    if (needNew && needOld)
    {
      useLeafInsert=FALSE;
    }
  }

  if (useLeafInsert)
    tupleColRefList = new(heap_) ItemExprList(heap_);

  // Now for each of the log columns, 
  // enter the correct value to be inserted.
  ItemExpr *delColExpr, *insColExpr;
  for (CollIndex i=0; i < tempColumns.entries(); i++) 
  {
#pragma nowarn(1506)   // warning elimination 
    NAColumn *currentTempCol = tempColumns.getColumn(i);
#pragma warn(1506)  // warning elimination 
    const NAString& colName = currentTempCol->getColName();

    delColExpr = insColExpr = NULL;
    if (colName.data()[0] == '@')
    {
      if (needOld)
	delColExpr = createAtColExpr(currentTempCol, FALSE, isUpdate);
      if (needNew)
	insColExpr = createAtColExpr(currentTempCol, TRUE,  isUpdate, isUndo);
    }
    else
    {
      if (!colName.compareTo("SYSKEY"))
      {	
	// This is the log SYSKEY column
        // iud log no longer has a SYSKEY
        if (needOld && useLeafInsert)
	  delColExpr = createSyskeyColExpr(colName, FALSE);
        if (needNew && useLeafInsert)
	  insColExpr = createSyskeyColExpr(colName, TRUE);
      }
      else
      {
	if (needOld)
	  delColExpr = createBaseColExpr(colName, FALSE, isUpdate);
	if (needNew)
	  insColExpr = createBaseColExpr(colName, TRUE,  isUpdate);
      }
    }

    // Add to column list of the new record expression.
    if (useLeafInsert)
    {
      if (needOld)
	tupleColRefList->insert(delColExpr);
      if (needNew)
	tupleColRefList->insert(insColExpr);
    }
    else
    {
      if (needOld && delColExpr!=NULL)
	if (delItemList == NULL)
	  delItemList = delColExpr;
	else
	  delItemList = new (heap_) ItemList(delItemList, delColExpr);

      if (needNew && insColExpr!=NULL)
	if (insItemList == NULL)
	  insItemList = insColExpr;
	else
	  insItemList = new (heap_) ItemList(insItemList, insColExpr);
    }
  }

  Insert *insertNode;
  Union  *unionNode = NULL;
  if (useLeafInsert)
  {
    // Create the leaf Insert node for INSERT or DELETE operations into the log table.
    insertNode = new (heap_) 
      LeafInsert(*getTableName(), NULL, tupleColRefList);
  }
  else
  {
    Tuple *delTuple = NULL, *insTuple = NULL;
    RelExpr *belowInsert = NULL;

    if (needOld)
      belowInsert = delTuple = new(heap_) Tuple(delItemList);

    if (needNew)
      belowInsert = insTuple = new(heap_) Tuple(insItemList);

    if (isUpdate)
    {
      unionNode = new(heap_) Union(delTuple, insTuple, 
					  NULL, NULL, REL_UNION, 
					  CmpCommon::statementHeap(), TRUE);
      setTuplesUnionType(unionNode);      
      belowInsert = unionNode;
    }

    insertNode = new(heap_) 
      Insert(*getTableName(), NULL, REL_UNARY_INSERT, belowInsert);
  }

  // Rows inserted into the log table should not be counted.
  insertNode->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;  

  // set MvLogging flags in TrueRoot, Union, and Insert nodes
  if( isForMvLogging &&
      ((isUpdate  && CmpCommon::getDefault(MV_LOG_PUSH_DOWN_DP2_UPDATE) == DF_ON) ||
       (isDelete  && CmpCommon::getDefault(MV_LOG_PUSH_DOWN_DP2_DELETE) == DF_ON) ||
       (isInsert  && CmpCommon::getDefault(MV_LOG_PUSH_DOWN_DP2_INSERT) == DF_ON) ))
  {
    bindWA_->getTopRoot()->getInliningInfo().setFlags(II_isUsedForMvLogging);
    insertNode->getInliningInfo().setFlags(II_isUsedForMvLogging);

    if( NULL != unionNode )
    {
      unionNode->getInliningInfo().setFlags(II_isUsedForMvLogging);
    }
  }

  return insertNode;
}  // ChangesTable::buildInsert()


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RelExpr *ChangesTable::buildDelete(RowsType whichRows) const
{
  // First build a Scan on the table.
  RelExpr *scanNode = buildScan(whichRows);

  // Build the Delete node on top of the Scan.
  Delete  *deleteNode = new(heap_) 
    Delete(*getTableName(), NULL, REL_UNARY_DELETE, scanNode);
  deleteNode->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;

  // Put a root on top.
  RelRoot *rootNode   = new(heap_) RelRoot(deleteNode);
  rootNode->setRootFlag(FALSE);
  rootNode->setEmptySelectList();

  return rootNode;
}  // ChangesTable::buildDelete()

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RelExpr *ChangesTable::transformScan() const
{
  CMPASSERT(scanNode_ != NULL);

  // If the Scan table does not have a correlation name, than the target
  // name should be made the correlation name.
  // Otherwise - the target name is redundant, and the Scan correlation 
  // name stays.
  const NAString& baseCorrName = 
    scanNode_->getTableName().getCorrNameAsString();

  const NAString& transitionName = 
    baseCorrName != "" ?
    baseCorrName       :
    isEmptyDefaultTransitionName()          ?
      *(new (heap_) NAString("", heap_)) :
      scanNode_->getTableName().getQualifiedNameObj().getObjectName();

  tableCorr_->setCorrName(transitionName);
  scanNode_->getTableName() = *tableCorr_;

  ItemExpr *selectionPredicate = createSpecificWhereExpr(scanType_);
  scanNode_->addSelPredTree(selectionPredicate); 

  return scanNode_;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Scan *ChangesTable::buildScan(RowsType type) const
{
  Scan *scanNode = new(heap_) Scan(*getTableName());

  // Add uniqifier predicates to the scan node
  ItemExpr *selectionPredicate = createSpecificWhereExpr(type);
  scanNode->addSelPredTree(selectionPredicate);

  return scanNode;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RelExpr *ChangesTable::buildOldAndNewJoin() const
{
  NAString newName(NEWCorr);
  NAString oldName(OLDCorr);

  // Create two Scan nodes on the temporary table for the OLD and NEW data).
  Scan *tempScanOldNode = buildScan(ChangesTable::DELETED_ROWS);
  RenameTable *renameOld = new(heap_)
    RenameTable(tempScanOldNode, OLDCorr);

  Scan *tempScanNewNode = buildScan(ChangesTable::INSERTED_ROWS);
  RenameTable *renameNew = new(heap_)
    RenameTable(tempScanNewNode, NEWCorr);

  // Join the two Scans for one long row
  ItemExpr *joinPredicate = new(heap_)
    BiRelat(ITM_EQUAL,
	    buildClusteringIndexVector(&newName, TRUE),
	    buildClusteringIndexVector(&oldName, TRUE) );

  Join *joinNode = new(heap_)
    Join(renameOld, renameNew, REL_JOIN, joinPredicate);
  // This join must be forced to a merge join.
//  joinNode->forcePhysicalJoinType(Join::MERGE_JOIN_TYPE);

  return joinNode;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
ItemExpr *ChangesTable::buildBaseColsSelectList(const CorrName& tableName) const
{
  ItemExpr *selectList=NULL;
  ItemExpr *colRef;

  // Get the columns of the subject table.
  const NAColumnArray &subjectColumns = 
    getSubjectNaTable()->getNAColumnArray(); 

  for (CollIndex i=0; i<subjectColumns.entries(); i++) 
  {
#pragma nowarn(1506)   // warning elimination 
    NAString colName(subjectColumns.getColumn(i)->getColName());
#pragma warn(1506)  // warning elimination 
    if (!colName.compareTo("SYSKEY"))
      continue;  // Skip SYSKEY.

    colRef = new(heap_) 
      ColReference(new(heap_) ColRefName(colName, tableName,heap_));

    if (selectList==NULL)
      selectList = colRef;
    else
      selectList = new(heap_) ItemList(selectList, colRef);
  }

  return selectList;
}

//////////////////////////////////////////////////////////////////////////////
// Build a selection-list of the columns of the ChangesTable that have a
// corresponding column in the subject table.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *
ChangesTable::buildColsCorrespondingToBaseSelectList() const
{
  ItemExpr *selectList=NULL;
  ItemExpr *colRef;

  // Get the columns of the ChangesTable.
  const NAColumnArray &columns = getNaTable()->getNAColumnArray(); 

  for (CollIndex i=0; i<columns.entries(); i++) 
  {
#pragma nowarn(1506)   // warning elimination 
    NAString colName(columns.getColumn(i)->getColName());
#pragma warn(1506)  // warning elimination 
    if (colName.data()[0] == '@' && colName.compareTo("@SYSKEY"))
      continue;  // Skip any special column that is not @SYSKEY.

    colRef = new(heap_) 
      ColReference(new(heap_) ColRefName(colName, *getTableName(),heap_));

    if (selectList==NULL)
      selectList = colRef;
    else
      selectList = new(heap_) ItemList(selectList, colRef);
  }

  return selectList;
}

//////////////////////////////////////////////////////////////////////////////
// Build a list of ColReferences to the columns of the base table's 
// clustering index columns. 
// When the second parameter (useInternalSyskey) is TRUE, the returned vector
// is using the "@SYSKEY" column instead of "SYSKEY" column.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *ChangesTable::buildClusteringIndexVector(const NAString *corrName,
						   NABoolean useInternalSyskey) const
{
  ItemExpr *result = NULL;

  const NATable *naTable = getSubjectNaTable();
  const NAColumnArray &indexColumns = 
    naTable->getClusteringIndex()->getIndexKeyColumns();

  NAString tableName(""), schemaName(""), catalogName("");
  if (corrName != NULL)
  {
    tableName = *corrName; 
  }
  else
  {
    const QualifiedName& qualName = naTable->getTableName();
    schemaName  = qualName.getSchemaName();
    catalogName = qualName.getCatalogName();
    tableName   = naTable->getTableName().getObjectName();
  }
  CorrName tableNameCorr(tableName, heap_, schemaName, catalogName);

  CollIndex lastCol  = indexColumns.entries()-1;
  for (CollIndex i=0; i<=lastCol; i++)
  {
    NAString colName = indexColumns[i]->getColName();

    if (useInternalSyskey && !colName.compareTo("SYSKEY"))
    {
      colName = "@SYSKEY"; // "@SYSKEY" corresponds to SYSKEY column
    }

    ItemExpr *colExpr = new(heap_) 
      ColReference(new(heap_) ColRefName(colName, tableNameCorr,heap_));

    if (result == NULL)
      result = colExpr;
    else
      result = new(heap_) ItemList(result, colExpr);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////////
// Build a list of RenameCols to map from the changes table column names to
// their euqivalent names in the subject table. 
//////////////////////////////////////////////////////////////////////////////
ItemExpr *ChangesTable::buildRenameColsList() const
{
  ItemExpr *renameList=NULL;

  // Get the columns of the subject table.
  const NAColumnArray &subjectColumns = 
    getSubjectNaTable()->getNAColumnArray(); 

  for (CollIndex i=0; i<subjectColumns.entries(); i++) 
  {
#pragma nowarn(1506)   // warning elimination 
    const NAString subjectCol(subjectColumns.getColumn(i)->getColName());
#pragma warn(1506)  // warning elimination 
    const NAString *colName = &subjectCol;
    if (!subjectCol.compareTo("SYSKEY"))
    {
      // @SYSKEY in the changes table is the equivalent of the SYSKEY
      // column in the subject table.
      colName = new (heap_) NAString("@SYSKEY", heap_);
    }

    ColReference *newColName = new(heap_)
      ColReference(new (heap_) ColRefName(*colName, *getTableName(), heap_));

    RenameCol *renCol = new(heap_)
      RenameCol(newColName,
		new (heap_) ColRefName(subjectCol, getSubjectTableName(), heap_));

    if (renameList==NULL)
      renameList = renCol;
    else
      renameList = new(heap_) ItemList(renameList, renCol);
  }

  return renameList;
} // ChangesTable::buildRenameColsList

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
ItemExpr *ChangesTable::createBaseColExpr(const NAString& colName, 
					  NABoolean       isInsert,
					  NABoolean       isUpdate) const
{
  const CorrName& corrName = 
    isInsert ? getCorrNameForNewRow() : getCorrNameForOldRow();

  return new(heap_) ColReference(new(heap_)
    ColRefName(colName, corrName, heap_));
}

//////////////////////////////////////////////////////////////////////////////
// Take the name of the subject table, and add to it the suffix to create 
// the name of the changes table.
//////////////////////////////////////////////////////////////////////////////
void ChangesTable::addSuffixToTableName(NAString& tableName, const NAString& suffix)
{
  size_t nameLen = tableName.length();
  ComBoolean isQuoted = FALSE;

  // special case: string ends with a quote char
  if ( '"' == tableName[ nameLen - 1 ] ) 
  {
    tableName.resize(nameLen-1);
    isQuoted = TRUE;
  }

  tableName.append( suffix );

  if (isQuoted) 
    tableName.append( "\"" );   // '"'
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
CorrName* ChangesTable::buildChangesTableCorrName(const QualifiedName& tableName,
						  const NAString&      suffix,
						  ExtendedQualName::SpecialTableType tableType,
						  CollHeap            *heap)
{
  NAString changesTableName(tableName.getObjectName());
  addSuffixToTableName(changesTableName, suffix);
  CorrName *result = new(heap) 
    CorrName(changesTableName,
	     heap,
	     tableName.getSchemaName(),
	     tableName.getCatalogName());

  // Specify the trigger temporary table namespace.
  result->setSpecialType(tableType); 
  return result;
}

/*****************************************************************************
******************************************************************************
****  Class TriggersTempTable
******************************************************************************
*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
TriggersTempTable::TriggersTempTable(const GenericUpdate *baseNode, 
				     BindWA              *bindWA)
  : ChangesTable(baseNode, bindWA),
    beforeTriggersExist_(FALSE),
    boundExecId_(NULL)
{  
  initialize();
}

//////////////////////////////////////////////////////////////////////////////
// This Ctor is used to transform the scan on the temp-table inside the 
// trigger action sub-tree of a statement trigger.
// (see Scan::buildTriggerTransitionTableView in Inlining.cpp)
//////////////////////////////////////////////////////////////////////////////
TriggersTempTable::TriggersTempTable(const CorrName &subjectTableName,
				     Scan           *baseNode, 
				     RowsType        scanType, 
				     BindWA         *bindWA)
  : ChangesTable(subjectTableName, NO_OPERATOR_TYPE, bindWA, scanType),
    beforeTriggersExist_(FALSE),
    boundExecId_(NULL)
{
  initialize();
  scanNode_ = baseNode;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RelExpr *TriggersTempTable::transformScan() const
{
  // The base class method fixes the scanned table name and takes care
  // of the uniquifier selection predicates.
  RelExpr *scanNode = ChangesTable::transformScan();

  // The select list of this root will filter away all the special columns 
  // that are not columns of the subject table.
  ItemExpr *selectList = buildBaseColsSelectList(*getTableName());
  RelRoot *rootNode = new(heap_) RelRoot(scanNode, REL_ROOT, selectList);

  NAString transitionName = tableCorr_->getCorrNameAsString();

  RenameTable *renameNode = new(heap_) RenameTable(rootNode, transitionName);

  return renameNode;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
ItemExpr *TriggersTempTable::createAtColExpr(const NAColumn *naColumn,
					     NABoolean       isInsert,
					     NABoolean       isUpdate,
					     NABoolean       isUndo) const
{
  const NAString& colName = naColumn->getColName();

  if (!colName.compareTo(UNIQUEEXE_COLUMN))
  {
    // With before triggers, the UniqueExecuteId column is used to propagate
    // the Signal actions. If this is an update, only one signal should be 
    // fired, not two.
    if (getBeforeTriggersExist() && 
        (!isUpdate || isInsert)   )
    {
      return new(heap_) 
	ColReference(new(heap_) 
	  ColRefName(InliningInfo::getExecIdVirtualColName(),heap_));
    }
    else
    {
      if (boundExecId_ != NULL)
      {
	// Use the already bound existing exec id function.
	return boundExecId_;
      }
      else
      {
	// Normal case - use the UniqueExecuteId builtin function.
	return new(heap_) UniqueExecuteId();
      }
    }
  }

  if (!colName.compareTo(UNIQUEIUD_COLUMN))
  {
    BindWA::uniqueIudNumOffset offset =
      isInsert                      ? 
      BindWA::uniqueIudNumForInsert : 
      BindWA::uniqueIudNumForDelete;

    return 
      new(heap_) ConstValue(bindWA_->getUniqueIudNum(offset));
  }

  if (!colName.compareTo("@SYSKEY"))
  {
    return createBaseColExpr("SYSKEY", isInsert, TRUE);
  }

  return NULL;
}  // TriggersTempTable::createColumnExpression()

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// iud log no longer has a SYSKEY
ItemExpr *TriggersTempTable::createSyskeyColExpr(const NAString& colName,
						 NABoolean       isInsert) const
{
  CMPASSERT(FALSE);
  return NULL;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
CorrName *TriggersTempTable::calcTargetTableName(const QualifiedName &tableName) const
{
  return buildChangesTableCorrName(tableName, 
                                   TRIG_TEMP_TABLE_SUFFIX,          // __TEMP
				   ExtendedQualName::TRIGTEMP_TABLE,
				   heap_);
}  // TriggersTempTable::calcTargetTableName()


//////////////////////////////////////////////////////////////////////////////
//
// createSpecificWhereExpr
//
// Create the uniqifier where expresion for the scan node on the temp table.
// This method is used for scanning the temp table for statement triggers,
// for the temp Delete node, and for the effective GU of before triggers.
//
// The uniqifier fields UNIQUE_EXECUTE_ID and UNIQUE_IUD_ID are used to
// identify the rows in a temporary table belong to a specific IUD and a
// specific query execution
//
//////////////////////////////////////////////////////////////////////////////
ItemExpr *TriggersTempTable::createSpecificWhereExpr(RowsType type) const
{
  // Create the ItemExpr for the function UniqueExecuteId()
  ItemExpr *col1 = new(heap_) 
    ColReference(new(heap_) ColRefName(UNIQUEEXE_COLUMN,heap_));

  ItemExpr *execId;
  if (boundExecId_ != NULL)
  {
    // Use the already bound existing exec id function.
    execId = boundExecId_;
  }
  else
  {
    // Normal case - use the UniqueExecuteId builtin function.
    execId  = new(heap_) UniqueExecuteId();
  }
  BiRelat *predExecId = new(heap_) BiRelat(ITM_EQUAL, col1, execId);

  ItemExpr *result;
  BiRelat *predIudId = NULL;
  if (type == ALL_ROWS)
    result = predExecId;
  else
  {
    // Create the ItemExpr for the constatnt UniqueIudNum 
    ItemExpr *col2 = new(heap_) 
      ColReference(new(heap_) ColRefName(UNIQUEIUD_COLUMN,heap_));

    // Compare it to the correct offset.
    BindWA::uniqueIudNumOffset offset =
      type == INSERTED_ROWS         ? 
      BindWA::uniqueIudNumForInsert : 
      BindWA::uniqueIudNumForDelete;
    ItemExpr *iudConst = new(heap_) ConstValue(bindWA_->getUniqueIudNum(offset));
    predIudId = new(heap_) BiRelat(ITM_EQUAL, col2, iudConst);

    result = new(heap_) BiLogic(ITM_AND, predExecId, predIudId);
  }

  return result;
}  // TriggersTempTable::createSpecificWhereExpr()

//////////////////////////////////////////////////////////////////////////////
//
// buildScanForMV
//
// Create a sub-tree that read's only the NEW values from the temp-table, for
// use as a replacement tree for the scan on the subject table in the MV tree.
// This replacement is taking place in insert operations that needs to update
// an ON STATEMENT MV defined on the inserted table.
//
// The built sub-tree is as follows:
//
//   RenameTable (temp-table -> subject-table, "@syskey" -> syskey)
//        |
//     RelRoot (remove Uniquifier columns)
//        |
//  Scan temp-table (NEW@ values only)
//
// Note: The RelRoot node that removes the uniquifier columns is needed, since
//       the RenameTable node ensures that the two tables have the same degree.
///////////////////////////////////////////////////////////////////////////////
RelExpr *TriggersTempTable::buildScanForMV() const
{
  // build scan to select NEW@ values only
  Scan *scanNode = buildScan(ChangesTable::INSERTED_ROWS);

  // build RelRoot to reduce to only columns that correspond to any column
  // in the subject table
  ItemExpr *selectList = buildColsCorrespondingToBaseSelectList();
  RelRoot *rootNode = new (heap_) RelRoot(scanNode, REL_ROOT, selectList);

  // build RenameTable to rename table name and columns' names to immitate
  // a scan on the subject table to the rest of the MV sub-tree
  ItemExpr *renameList = buildRenameColsList();
  RelExpr *renameNode = new(heap_)
    RenameTable(TRUE, rootNode, getSubjectTableName(), renameList);

  return renameNode;
} //TriggersTempTable::buildScanForMV

/*****************************************************************************
******************************************************************************
****  Class MvIudLog
******************************************************************************
*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// Ctor for Refresh - building the Scan nodes.
//////////////////////////////////////////////////////////////////////////////
MvIudLog::MvIudLog(CorrName&         name, 
		   BindWA           *bindWA, 
		   OperatorTypeEnum  opType)
  : ChangesTable(name, opType, bindWA),
    needsRangeLogging_(FALSE),
    updatedColumns_(NULL),
    deltaDef_(NULL),
    addOrigScanPredicate_(FALSE),
    mvInfo_(NULL)
{
  initialize();
}

//////////////////////////////////////////////////////////////////////////////
// Ctor for logging - building the Insert node.
//////////////////////////////////////////////////////////////////////////////
MvIudLog::MvIudLog(const GenericUpdate *baseNode, 
		   BindWA              *bindWA)
  : ChangesTable(baseNode, bindWA),
    needsRangeLogging_(FALSE),
    updatedColumns_(NULL),
    deltaDef_(NULL),
    addOrigScanPredicate_(FALSE),
    mvInfo_(NULL)
{
  initialize();
  initRangeLogging();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void MvIudLog::initRangeLogging()
{
  if (getOpType() == REL_UNARY_INSERT)
  {
    const ComMvAttributeBitmap& mvBitmap = 
      getSubjectNaTable()->getMvAttributeBitmap();
    if (mvBitmap.getAutomaticRangeLoggingRequired())
      needsRangeLogging_=TRUE;
  }
}

//////////////////////////////////////////////////////////////////////////////
// Identify the @ columns of the MV log, and call specific methods for
// building the appropriate expressions.
// This method is used for MvIudLog and its sub-classes, but the
// specific methods are overridden by the sub-classes.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::createAtColExpr(const NAColumn *naColumn, 
				    NABoolean       isInsert,
				    NABoolean       isUpdate,
				    NABoolean       isUndo) const
{
  const NAString& colName = naColumn->getColName();
  ItemExpr *result = NULL;

  switch (colName.data()[sizeof(COMMV_CTRL_PREFIX)-1])
  {
    case 'E' : // @EPOCH
      CMPASSERT(!colName.compareTo(COMMV_EPOCH_COL));
      result = createColExprForEpoch();
      break;

    case 'O' : // @OPERATION_TYPE
      CMPASSERT(!colName.compareTo(COMMV_OPTYPE_COL));
      result = createColExprForOpType(isInsert, isUpdate, isUndo);
      break;

    case 'I' : // @IGNORE
      CMPASSERT(!colName.compareTo(COMMV_IGNORE_COL));
      result = createColExprForIgnore();
      break;

    case 'U' : // @UPDATE_BITMAP
      CMPASSERT(!colName.compareTo(COMMV_BITMAP_COL));
      result = createColExprForBitmap(naColumn, isUpdate);
      break;

    case 'R' : // @RANGE_SIZE
      CMPASSERT(!colName.compareTo(COMMV_RANGE_SIZE_COL));
      result = createColExprForRangeSize(isInsert);
      break;

    case 'S' : // @SYSKEY - This is the base table SYSKEY column
      CMPASSERT(!colName.compareTo(COMMV_BASE_SYSKEY_COL));
      result = createColExprForAtSyskey(isInsert);
      break;

    case 'T' : // @TS - timestamp
      CMPASSERT(!colName.compareTo(COMMV_TS_COL));
      result = createColExprForAtTimestamp(isInsert,isUpdate,isUndo);
      break;

    case 'A' : // @ALIGNMENT
      // ALIGNMENT is not used 
      CMPASSERT(!colName.compareTo(COMMV_ALIGNMENT_COL));
      {
        Int32 bitmapSize = naColumn->getType()->getNominalSize();
	NAString *bitmap = new(heap_) NAString('\0', bitmapSize, heap_);
	result = new(heap_) SystemLiteral(*bitmap);
      }
      break;

    default  : // Unknown column
      CMPASSERT(FALSE);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @EPOCH column.
// Insert the CurrentEpoch function pipelined from the triggering node.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::createColExprForEpoch() const
{
  return new(heap_)
    ColReference(new(heap_) 
      ColRefName(InliningInfo::getEpochVirtualColName(),heap_));
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @OPERATION_TYPE column.
// If range logging is enabled - the value is generated by the VSBBInsert 
// node. Otherwise use the values for Insert/Delete/Update.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::createColExprForOpType(NABoolean isInsert,
					   NABoolean isUpdate,
					   NABoolean isUndo) const
{
  ItemExpr *result = NULL;
  if (needsRangeLogging_)
  {
    CMPASSERT(isInsert)
    result = new(heap_) 
      ColReference(new(heap_) 
	ColRefName(InliningInfo::getRowTypeVirtualColName(),heap_));
  }
  else
  {
    Int32 typeValue = isInsert ? ComMvRowType_Insert : ComMvRowType_Delete;
    if (isUndo)
      typeValue = ComMvRowType_Delete;
    if (isUpdate)
      typeValue |= ComMvRowType_Update;
    result = new(heap_) SystemLiteral(typeValue);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @IGNORE column.
// Use zero. This field is later updated by the refresh utility during
// the duplicate elimination algorithm.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::createColExprForIgnore() const
{
  return new(heap_) SystemLiteral(0);
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @UPDATE_BITMAP column.
// If it is an Update operation - calculate a bitmap of updated columns.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::createColExprForBitmap(const NAColumn *naColumn,
					   NABoolean isUpdate) const
{
  ItemExpr *result = NULL;

  if (!isUpdate)
  {
    result = new(heap_) SystemLiteral(); // NULL
  }
  else
  {
    Int32 bitmapSize = naColumn->getType()->getNominalSize();

    CMPASSERT(updatedColumns_ != NULL);
    unsigned char *colsBitmap = new(heap_) unsigned char[bitmapSize];
    updatedColumns_->markColumnsOnBitmap(colsBitmap, bitmapSize);
    NAString *bitmap = new(heap_) NAString((const char *) colsBitmap, bitmapSize, heap_);

    result = new(heap_) SystemLiteral(*bitmap);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @RANGE_SIZE column.
// If range logging is enabled, the value is generated by the VSBBInsert node.
// Otherwise use 0 for Delete and 1 for Insert.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::createColExprForRangeSize(NABoolean isInsert) const
{
  ItemExpr *result = NULL;

  if (needsRangeLogging_)
  {
    // range logging is not supported 
    CMPASSERT(isInsert);
    result = new(heap_) 
      ColReference(new(heap_) 
	ColRefName(InliningInfo::getRowCountVirtualColName(),heap_));
  }
  else
  {
    result = new(heap_) 
      SystemLiteral(isInsert ? 1 : 0);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @SYSKEY column.
// This is the base table SYSKEY column.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::createColExprForAtSyskey(NABoolean isInsert) const
{
  const CorrName& corrName = 
    isInsert ? getCorrNameForNewRow() : getCorrNameForOldRow();

  return new(heap_) 
    ColReference(new(heap_) ColRefName("SYSKEY", corrName, heap_));
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the SYSKEY column.
// This is the log SYSKEY column. Use 0 since the real value is generated
// by DP2 as the timestamp of insertion.
//////////////////////////////////////////////////////////////////////////////
// iud log no longer have a syskey, we now enforce uniqueness using a @TS
ItemExpr *MvIudLog::createSyskeyColExpr(const NAString& colName,
					NABoolean       isInsert) const
{
  return new(heap_) SystemLiteral(0);
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @TS column.
// Generate a unique, sequential value for each new row in the iud log
// The intention of this method is to generate a timestamp and increment
// by some value for each new row to avoid duplicate timestamps. 
//
// The value is incremented by 3 in order to leave room for updates where
// each side of the union begins with the same persistent value. The insert
// side of the union will use an additional +1.  Additionally, leave room
// for undo which uses a +2 on top of the persistent value.
// 
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::createColExprForAtTimestamp(NABoolean isInsert,
                                                NABoolean isUpdate,
                                                NABoolean isUndo) const
{
  ItemExpr *counterExpr, *incrementExpr, *tsExpr;
  counterExpr = new (heap_) ItmPersistentExpressionVar(0);
  incrementExpr = new (heap_) ItmBlockFunction
                    (counterExpr, 
                    new (heap_) Assign (counterExpr,
                        new (heap_) BiArith(ITM_PLUS, 
                                            counterExpr,
                                            new (heap_) ConstValue(ROW_TS_INCR))));

  // Synthesize the types and value IDs for the new items
  incrementExpr->synthTypeAndValueId(TRUE);

  tsExpr = new (heap_) 
          BiArith(ITM_PLUS, 
            incrementExpr, 
            new(heap_) ColReference(
              new(heap_) ColRefName(InliningInfo::getMvLogTsColName(),heap_)));
      
  // This is the +1 on the insert row of an update operation
  // or a +2 for an undo row
  if ( (isUpdate && isInsert ) || isUndo )
  {
    Lng32 incrAmt = ( isUndo ) ? 2 : 1;
    tsExpr = new (heap_)
              BiArith(ITM_PLUS,tsExpr,new (heap_)SystemLiteral(incrAmt));
  }

  return tsExpr;
}

//////////////////////////////////////////////////////////////////////////////
// Calculate the log name from the base table name.
//////////////////////////////////////////////////////////////////////////////
CorrName *MvIudLog::calcTargetTableName(const QualifiedName &tableName) const
{
  return buildChangesTableCorrName(tableName, 
                                   COMMV_IUD_LOG_SUFFIX,
				   ExtendedQualName::IUD_LOG_TABLE,
				   heap_);
}

//////////////////////////////////////////////////////////////////////////////
// The Epoch Predicate is: 
//	singleEpoch:	@EPOCH = <beginEpoch>
//	otherwise:	@EPOCH BETWEEN <beginEpoch> AND <endEpoch>
// The Epoch predicate is the same for the range log as well. This is why
// this method is declared static - it is called by buildJoinWithRangeLog().
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::buildLogEpochPredicate(const DeltaDefinition *deltaDef,
					   CollHeap              *heap)
{
  CMPASSERT(deltaDef != NULL);
  Lng32 beginEpoch = deltaDef->getBeginEpoch();
  Lng32 endEpoch   = deltaDef->getEndEpoch();
  NABoolean singleEpoch = (beginEpoch == endEpoch);
  
  ItemExpr *epochPred  = NULL;

  if (singleEpoch)
  {
    epochPred =	
      BinderUtils::buildPredOnCol(ITM_EQUAL, COMMV_EPOCH_COL, beginEpoch, heap);
  }
  else
  {
    epochPred =	new(heap)	
      Between(new(heap) ColReference(new(heap) ColRefName(COMMV_EPOCH_COL,heap)),
	      new(heap) SystemLiteral(beginEpoch),
	      new(heap) SystemLiteral(endEpoch));
  }

  return epochPred;
}

//////////////////////////////////////////////////////////////////////////////
// Build the log selection predicate on the @EPOCH, @IGNORE and 
// @OPERATION_TYPE,@BITMAP columns.
// This predicate depends on two factors: the scan type (inserts only, deletes 
// only or both) and if its a single epoch refresh (beginEpoch = endEpoch).
// This predicate has four parts:
// epochPred  is: 
//	singleEpoch:	@EPOCH = <beginEpoch>
//	otherwise:	@EPOCH BETWEEN <beginEpoch> AND <endEpoch>
// ignorePred is: 
//	singleEpoch:	N/A
//	otherwise:	@IGNORE < <beginEpoch>
// opTypePred depends on scanType (should ignore range rows):
//	if SCAN_INSERTS: (@OPERATION_TYPE = 0) OR (@OPERATION_TYPE = 2)
//	if SCAN_DELETES: (@OPERATION_TYPE = 1) OR (@OPERATION_TYPE = 3)
//	if SCAN_BOTH   :  @OPERATION_TYPE < 4
// updateBitmapPred :  (@BITMAP IS NULL) OR ((MV.BITMAP & @BITMAP) > 0)
//		       We use here a special builtin function that evalutes
//		       this expression
//////////////////////////////////////////////////////////////////////////////
#pragma nowarn(262)   // warning elimination 
ItemExpr *MvIudLog::createSpecificWhereExpr(RowsType type) const
{
  CMPASSERT(deltaDef_ != NULL);
  Lng32 beginEpoch       = deltaDef_->getBeginEpoch();
  Lng32 endEpoch         = deltaDef_->getEndEpoch();
  NABoolean singleEpoch = (beginEpoch == endEpoch);
  ItemExpr *result      = NULL;

  result = 
    BinderUtils::buildPredOnCol(ITM_LESS, 
				COMMV_IGNORE_COL, 
				beginEpoch, 
				heap_);
  
  ItemExpr *opTypePred = NULL;
  switch (type)
  {
    case INSERTED_ROWS:
      opTypePred = new (heap_) 
	BiLogic(ITM_OR, 
    	        BinderUtils::buildPredOnCol(ITM_EQUAL, 
		                            COMMV_OPTYPE_COL, 
					    ComMvRowType_Insert, 
					    heap_),
    	        BinderUtils::buildPredOnCol(ITM_EQUAL, 
		                            COMMV_OPTYPE_COL, 
					    ComMvRowType_InsertOfUpdate, 
					    heap_) );
      break;

    case DELETED_ROWS:
      opTypePred = new (heap_) 
	BiLogic(ITM_OR,
    	        BinderUtils::buildPredOnCol(ITM_EQUAL, 
		                            COMMV_OPTYPE_COL, 
					    ComMvRowType_Delete, 
					    heap_),
    	        BinderUtils::buildPredOnCol(ITM_EQUAL, 
		                            COMMV_OPTYPE_COL, 
					    ComMvRowType_DeleteOfUpdate, 
					    heap_) );
      break;

    case ALL_ROWS:
      // Since all the range operations are logged in negative epochs
      // we do not need to screen them while we are reading the IUD records 
      // (always in positive epochs)
      break;

    default: CMPASSERT(FALSE);
  }
  if (opTypePred)
  {
    if (result)
    {
      result = new(heap_) BiLogic(ITM_AND, result, opTypePred);
    }
    else
    {
      result = opTypePred;
    }
  }

  ItemExpr *usedColPredicate = buildBitmapColPredicate();
  if (result)
  {
    result = new(heap_) BiLogic(ITM_AND, result, usedColPredicate);
  }
  else
  {
    result = usedColPredicate;
  }
  
  result->setSelectivityFactor(1.0);

  // For Multi-Txn refresh we need to add the original selection predicates.
  if (getAddOrigScanPredicateFlag())
  {
    ItemExpr *origScanPredicate = buildOrigScanPredicate();
    if (origScanPredicate != NULL)
      result = new(heap_) BiLogic(ITM_AND, result, origScanPredicate);
  }

  return result;
}
#pragma warn(262)  // warning elimination 

//////////////////////////////////////////////////////////////////////////////
// The orig selection predicate is on the base table columns. This recursive 
// method finds all such column references, and fixes them to be on the 
// log table instead.
//////////////////////////////////////////////////////////////////////////////
NABoolean MvIudLog::fixReferencesFromBaseTableToLog(ItemExpr *expr) const
{
  NABoolean result = TRUE;

  if (expr->getArity() > 0)
  {
    for (Int32 i=0; i<expr->getArity(); i++)
      result &= fixReferencesFromBaseTableToLog(expr->child(i));
  }
  else
  {
    if (expr->getOperatorType() == ITM_REFERENCE)
    {
      ColReference *colRef = (ColReference *)expr;
      QualifiedName& tableName = 
	colRef->getCorrNameObj().getQualifiedNameObj();

      // Ignore predicates that reference other tables.
      if (tableName != getSubjectTableName().getQualifiedNameObj())
	return FALSE;

      tableName = getTableName()->getQualifiedNameObj();
    }
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////////
// For Multi-Txn refresh we need to add the original selection predicates.
// Get the predicate text from MVInfo, parse them and return them.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::buildOrigScanPredicate() const
{
  const QualifiedName& tableName = getSubjectTableName().getQualifiedNameObj();
  const MVUsedObjectInfo *usedObject = 
    getMvInfo()->findUsedInfoForTable(tableName);

  const NAString& textPredicate = usedObject->getSelectionPredicates();
  if (textPredicate == "")
    return NULL;

  Parser parser(bindWA_->currentCmpContext());
  ItemExpr *origScanPredicate = 
    parser.getItemExprTree((char *)textPredicate.data());

  ItemExprList predicateList(origScanPredicate, bindWA_->wHeap(), ITM_AND);
#pragma nowarn(1506)   // warning elimination 
  for (Int32 i=predicateList.entries()-1; i>=0; i--)
#pragma warn(1506)  // warning elimination 
  {
    if (fixReferencesFromBaseTableToLog(predicateList[i]) == FALSE)
      predicateList.removeAt(i);
  }
  ItemExpr *fixedPredicate = predicateList.convertToItemExpr();
  return fixedPredicate;
}

//////////////////////////////////////////////////////////////////////////////
// updateBitmapPred :  (@OPERATION_TYPE = 0) INSERT 
//			OR 
//		       (@OPERATION_TYPE = 1) DELETE
//			OR
//		       (@BITMAP IS NULL) OR 
//			OR
//		      ((MV.BITMAP & @BITMAP) > 0) 
//		       We use here a special builtin function that evalutes
//		       this expression
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::buildBitmapColPredicate() const
{
  const LIST(Lng32) &usedColumns = 
    mvInfo_->getUsedColumns(getSubjectTableName().getQualifiedNameObj());
  ItemExpr *constantBitmap = constructUpdateBitmapFromList(usedColumns);

  ItemExpr *bitmapCol =  new(heap_) 
                 ColReference(new(heap_) ColRefName(COMMV_BITMAP_COL, heap_));

  ItemExpr *isBitwiseTrue = new(heap_) IsBitwiseAndTrue(bitmapCol,constantBitmap );

  ItemExpr *isBitmapColNull = new(heap_) UnLogic(ITM_IS_NULL,bitmapCol);

  ItemExpr *insertRecord = BinderUtils::buildPredOnCol(ITM_EQUAL, 
					      COMMV_OPTYPE_COL, 
					      ComMvRowType_Insert, 
					      heap_);

  ItemExpr *deleteRecord = BinderUtils::buildPredOnCol(ITM_EQUAL, 
					      COMMV_OPTYPE_COL, 
					      ComMvRowType_Delete, 
					      heap_);
  
  return new(heap_) 
    BiLogic(ITM_OR, insertRecord, new(heap_)
	BiLogic(ITM_OR, deleteRecord, new(heap_)
	  BiLogic(ITM_OR, isBitmapColNull, isBitwiseTrue)));  
}

//////////////////////////////////////////////////////////////////////////////
// Create a constant bitmap in the size of the bitmap log column
// from the input parameter list of integers.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::constructUpdateBitmapFromList(const LIST(Lng32) &columnList) const
{
  CMPASSERT (mvInfo_ != NULL);

  // Find out the size of the bitmap column in the iud log
  const NAColumnArray &logColumns = getNaTable()->getNAColumnArray(); 
  
  NAColumn *currentTempCol = NULL;
  
  for (CollIndex i=0; i < logColumns.entries(); i++) 
  {
#pragma nowarn(1506)   // warning elimination 
    currentTempCol = logColumns.getColumn(i);
#pragma warn(1506)  // warning elimination 
    if (!currentTempCol->getColName().compareTo(COMMV_BITMAP_COL))
      break;
  }

  CMPASSERT(!currentTempCol->getColName().compareTo(COMMV_BITMAP_COL));

  Int32 bitmapSize = currentTempCol->getType()->getNominalSize();

  // Initialize a constant bitmap from the used cols list of the mv  
  unsigned char *usedColsBitmap = new(heap_) unsigned char[bitmapSize];

  for (int i=0; i<bitmapSize; i++)
    usedColsBitmap[i] = 0;

  // Iterate through the column list and set the corresponding bit in the
  // bitmap
  for (CollIndex j=0; j < columnList.entries(); j++) 
  {
    CollIndex b = columnList[j];
    // set bit number b in the array
    if (b/8 < bitmapSize)
      usedColsBitmap[b/8] |= 0x01 << b % 8;
  }
  
  // Construct a constant string expression from the bitmap
  NAString *bitmap = new(heap_) NAString((char *) usedColsBitmap, bitmapSize, heap_);

  ItemExpr *result = new(heap_) SystemLiteral(*bitmap);

  return result;
}

//////////////////////////////////////////////////////////////////////////////
// This method builds an expression that evaluates to 1 only for rows that 
// represent an indirect update operation (has to be implemented by a 
// delete-insert combination).
// We rely on the fact that the update bitmap is not null only for update rows.
//     (@BITMAP IS NOT NULL)
//	AND
//    ((MV.BITMAP & @BITMAP) > 0) 
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLog::buildIndirectUpdateExpression() const
{
  const MVUsedObjectInfo *usedObject = 
    mvInfo_->findUsedInfoForTable(getSubjectTableName().getQualifiedNameObj());

  const LIST(Lng32) &mvInfoIndirectCols = usedObject->getIndirectUpdateCols();

  ItemExpr *constantBitmap = 
    constructUpdateBitmapFromList(mvInfoIndirectCols);

  ItemExpr *bitmapCol = new(heap_) 
    ColReference(new(heap_) ColRefName(COMMV_BITMAP_COL, heap_));

  ItemExpr *isBitwiseTrue = new(heap_) 
    IsBitwiseAndTrue(bitmapCol, constantBitmap);

  ItemExpr *isBitmapColNotNull = new(heap_) 
    UnLogic(ITM_IS_NOT_NULL, bitmapCol);

  return new(heap_) 
    BiLogic(ITM_AND, isBitmapColNotNull, isBitwiseTrue);
}

// For mvIudLog, the union above Tuples must be ordered.
void MvIudLog::setTuplesUnionType(Union *unionNode) const
{
  unionNode->setOrderedUnion();
}

/*****************************************************************************
******************************************************************************
****  Class MvIudLogForMvLog
****  used by the MVLOG command
******************************************************************************
*****************************************************************************/

// MVLOG command is currently not supported
//////////////////////////////////////////////////////////////////////////////
// Ctor for building the Insert node.
//////////////////////////////////////////////////////////////////////////////
MvIudLogForMvLog::MvIudLogForMvLog(CorrName&  tableName, 
				   BindWA    *bindWA)
  : MvIudLog(tableName, bindWA, REL_UNARY_UPDATE)
{
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the -@EPOCH column.
// 
// we log the ranges into negative epochs in order to provide efficent 
// reading from the IUD log of the range records 
//
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLogForMvLog::createColExprForEpoch() const
{
  ItemExpr *epochColExpr = MvIudLog::createColExprForEpoch();

  ItemExpr *epochExpr = new(heap_)BiArith(ITM_TIMES, 
					  epochColExpr, 
					  new(heap_) SystemLiteral(-1));
  return epochExpr;
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @OPERATION_TYPE column.
// In the context row, this field is used for storing the MSW of the MV UID.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLogForMvLog::createColExprForOpType(NABoolean isInsert,
					           NABoolean isUpdate,
						   NABoolean isUndo) const
{
  Int32 typeValue = isInsert                ? 
                  ComMvRowType_EndRange   : 
                  ComMvRowType_BeginRange;
  return new(heap_) SystemLiteral(typeValue);
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @IGNORE column.
// In the context row, this field is used for storing the Begin Epoch number.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLogForMvLog::createColExprForIgnore() const
{
  return new(heap_) SystemLiteral(0);
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @UPDATE_BITMAP column.
// Not used for context rows.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLogForMvLog::createColExprForBitmap(const NAColumn *naColumn,
					           NABoolean isUpdate) const
{
  return new(heap_) SystemLiteral(); // NULL.;
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @RANGE_SIZE column.
// In the context row, this field is used for storing the LSW of the MV UID.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLogForMvLog::createColExprForRangeSize(NABoolean isInsert) const
{
  return new(heap_) SystemLiteral(10000);  // Should be greater than 0.
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @SYSKEY column.
// This is the base table SYSKEY column.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLogForMvLog::createColExprForAtSyskey(NABoolean isInsert) const
{
  return createBaseColExpr("SYSKEY", isInsert, TRUE);
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the @TS column.
// Not used for context rows.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLogForMvLog::createColExprForAtTimestamp(NABoolean isInsert,
                                                        NABoolean isUpdate,
                                                        NABoolean isUndo) const
{
  return new(heap_) SystemLiteral(0); 
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvIudLogForMvLog::createBaseColExpr(const NAString& colName, 
					      NABoolean       isInsert,
					      NABoolean       isUpdate) const
{
  NAString virtualColumnName(colName);
  if (isInsert)
    virtualColumnName.append(MvLogInternalNames::getEndRangeSuffix());
  else
    virtualColumnName.append(MvLogInternalNames::getBeginRangeSuffix());

  return new(heap_) 
    ColReference(new(heap_) ColRefName(virtualColumnName, heap_));
}


/*****************************************************************************
******************************************************************************
****  Class MvLogForContextRows
****  used by multi-transactional activatoins of INTERNAL REFRESH, for 
****  saving context rows.
******************************************************************************
*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// Context logs are using the name of the MV (with no suffix), but the 
// columns of the base table. The ChangesTable data members are initialized  
// as follows:
// tableCorr_      - Context log name.
// subjectTable_   - Name of MV.
// naTable_        - NATable of context log.
// subjectNaTable_ - NATable of base table.
//////////////////////////////////////////////////////////////////////////////
MvLogForContextRows::MvLogForContextRows(const QualifiedName&  baseTableName,
					 const CorrName&       mvName,
					 Lng32                  epochNumber,
					 ItemExpr	      *catchupNo,
					 BindWA               *bindWA)
  : ChangesTable(mvName, REL_UNARY_INSERT, bindWA),
    epochNumber_(epochNumber),
    catchupNo_(catchupNo)
{
  // Save the state of the RI flag
  BindContext *currentBindContext = bindWA_->getCurrentScope()->context();
  NABoolean prevRiState = currentBindContext->inRIConstraint();
  // Set the RI flag to allow calling getNATable() without counting it.
  currentBindContext->inRIConstraint() = TRUE;

  // Get the NATable of the base table.
  CorrName baseTableCorrName(baseTableName);
  subjectNaTable_ = bindWA_->getNATable(baseTableCorrName);

  // Restore the RI flag.
  currentBindContext->inRIConstraint() = prevRiState;

  // Initialize the rest of the data members.
  initialize();

  corrNameForNewRow_ = baseTableName;
  // Old names are not used
}

//////////////////////////////////////////////////////////////////////////////
// Calculate the log name from the base table name.
// The context table has the same name as the MV (no suffix), and is in the
// range log name space.
//////////////////////////////////////////////////////////////////////////////
CorrName *MvLogForContextRows::calcTargetTableName(const QualifiedName &tableName) const
{
  return buildChangesTableCorrName(tableName, 
                                   "",          // No Suffix
				   ExtendedQualName::RANGE_LOG_TABLE,
				   heap_);
}

//////////////////////////////////////////////////////////////////////////////
// For the MV context log, there are only two @ column: @EPOCH and @SYSKEY.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvLogForContextRows::createAtColExpr(const NAColumn *naColumn, 
					       NABoolean       isInsert,
					       NABoolean       isUpdate,
					       NABoolean isUndo) const
{
  const NAString& colName = naColumn->getColName();

  if (colName.data()[1] == 'E')
  {
    CMPASSERT(!colName.compareTo(COMMV_EPOCH_COL));
    return new(heap_) SystemLiteral(epochNumber_);
  }
  else
  {
    CMPASSERT(!colName.compareTo(COMMV_SYSKEY_COL));
    return new(heap_) 
      ColReference(new(heap_) ColRefName(COMMV_SYSKEY_COL,
                                         getCorrNameForNewRow(),heap_));
  }
}

//////////////////////////////////////////////////////////////////////////////
// build the expression to insert into the log SYSKEY column.
// There is no log SYSKEY column for the context log!
//////////////////////////////////////////////////////////////////////////////
// should never get here
ItemExpr *MvLogForContextRows::createSyskeyColExpr(const NAString& colName,
						   NABoolean       isInsert) const
{
  CMPASSERT(FALSE);
  return NULL;
}

//////////////////////////////////////////////////////////////////////////////
// Build the selection predicate for reading a context row from the log. 
// This method is used for reading the phase 1 context and the catchup 
// context.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MvLogForContextRows::createSpecificWhereExpr(RowsType type) const
{
  ItemExpr *contextEpochType = NULL;
  switch(type)
  {
    case PHASE1_ROWS  :
      // For Phase 1 Scans, the @EPOCH column should equal the begin epoch 
      // number from the INTERNAL REFRESH delta definition.
      contextEpochType = new(heap_) SystemLiteral(epochNumber_);
      break;

    case CATCHUP_ROWS :
      // For Catchup Scans, the @EPOCH column should equal the CATCHUP 
      // parameter.
      contextEpochType = catchupNo_;

      break;

    default: CMPASSERT(FALSE);
  }

  ItemExpr *contextEpochExpr = new(heap_) 
    BiRelat(ITM_EQUAL, 
	    new(heap_) ColReference(new(heap_) ColRefName(COMMV_EPOCH_COL,heap_) ),
	    contextEpochType);


  return contextEpochExpr;
}

