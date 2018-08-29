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
* File:         Inlining.cpp
* Description:  Methods for the inlining of triggers.
*
* Created:      6/23/98
* Language:     C++
* Status:       $State: Exp $
*
*/ 

#define INITIALIZE_OLD_AND_NEW_NAMES  // used in Triggers.h
#define  SQLPARSERGLOBALS_FLAGS	   // must precede all #include's
#define  SQLPARSERGLOBALS_CONTEXT_AND_DIAGS

#include "Sqlcomp.h"
#include "AllItemExpr.h"
#include "AllRelExpr.h"
#include "BindWA.h"
#include "GroupAttr.h"
#include "parser.h"
#include "StmtNode.h"
#include "Inlining.h"
#include "ex_error.h"
#include "Triggers.h"
#include "TriggerDB.h"
#include "TriggerEnable.h"
#include "StmtDDLCreateTrigger.h"
#include "MVInfo.h"
#include "Refresh.h"
#include "ChangesTable.h"
#include "MvRefreshBuilder.h"
#include "MjvBuilder.h"
#include "ItmFlowControlFunction.h"
#include <CmpMain.h>
#include "RelSequence.h"

#ifdef NA_DEBUG_GUI
	#include "ComSqlcmpdbg.h"
#endif

#include "SqlParserGlobals.h"		// must be last #include

#define DISABLE_TRIGGERS 0
#define DISABLE_RI       0

extern THREAD_P NABoolean GU_DEBUG;

static const char NEWTable [] = "NEW";    // QSTUFF:  corr for embedded d/u
static const char OLDTable [] = "OLD";    // QSTUFF:  corr for embedded d/u

/*******************************************************************************
****  Independant Utility Functions
******************************************************************************
*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// Create a CorrName to the temp table from the subject table name.
//////////////////////////////////////////////////////////////////////////////
/*static CorrName *calcTempTableName(const CorrName &theTable, CollHeap *heap) 
{
  const QualifiedName &tableName = theTable.getQualifiedNameObj();

  CorrName *result = new(heap) 
    CorrName(subjectNameToTrigTemp(tableName.getObjectName()),
	     heap,
	     tableName.getSchemaName(),
	     tableName.getCatalogName());

  // Specify the trigger temporary table namespace.
  result->setSpecialType(ExtendedQualName::TRIGTEMP_TABLE); 
  return result;
}

//////////////////////////////////////////////////////////////////////////////
// Does this column name start with NEW_COLUMN_PREFIX?
//////////////////////////////////////////////////////////////////////////////
static NABoolean isNewCol(const NAString& colName)
{
  if (colName.length() < sizeof(NEW_COLUMN_PREFIX))
    return FALSE;
  
  return (colName(0,sizeof(NEW_COLUMN_PREFIX)-1) == NEW_COLUMN_PREFIX);
}

//////////////////////////////////////////////////////////////////////////////
// Does this column name start with OLD_COLUMN_PREFIX?
//////////////////////////////////////////////////////////////////////////////
static NABoolean isOldCol(const NAString& colName) 
{
  if (colName.length() < sizeof(OLD_COLUMN_PREFIX))
    return FALSE;

  return (colName(0,sizeof(OLD_COLUMN_PREFIX)-1) == OLD_COLUMN_PREFIX);
}

//////////////////////////////////////////////////////////////////////////////
// Remove the temp table column name prefix.
//////////////////////////////////////////////////////////////////////////////
static void FixTempColName(NAString *colName)
{
  CMPASSERT (sizeof(OLD_COLUMN_PREFIX) == sizeof(NEW_COLUMN_PREFIX));

  colName->remove(0,sizeof(OLD_COLUMN_PREFIX)-1); // remove the prefix
} 

//////////////////////////////////////////////////////////////////////////////
// Does this column name from the temp table contain the @ sign? 
// If so  - it must be either a NEW@ or an OLD@ column.
// If not - it must be part of either the primary or clustering keys.
//////////////////////////////////////////////////////////////////////////////
static NABoolean isSingleCopyColumn(const NAString& colName)
{
  return !colName.contains(NON_SQL_TEXT_CHAR);
}*/

//////////////////////////////////////////////////////////////////////////////
// Utility function used by the fixTentativeRETDesc() method below.
// Creates a Cast ItemExpr around the parameter, so it stays the same but
// gets a new ValueId.
//////////////////////////////////////////////////////////////////////////////
static ValueId wrapWithCastExpr(BindWA *bindWA, ValueId col, CollHeap *heap)
{
  ItemExpr *expr = col.getItemExpr(); 
  ItemExpr *cast = new(heap) Cast(expr, col.getType().newCopy(heap));
  cast->bindNode(bindWA);
  return cast->getValueId();
}
    
/*****************************************************************************
******************************************************************************
****  Inlining functions of classes other than GenericUpdate
******************************************************************************
*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// Is this CorrName using a name of a trigger transition table (from the
// REFERENCING clause)?
// Find if we are in the scope of a trigger action, and if the names match.
// onlyNew has a default value of FALSE, which is overridden only for DDL 
// semantic checks of before triggers.
//////////////////////////////////////////////////////////////////////////////
NABoolean CorrName::isATriggerTransitionName(BindWA *bindWA, NABoolean onlyNew) const
{
  BindScope *scope = bindWA->findNextScopeWithTriggerInfo();

  if ((scope==NULL) || (scope->context()->triggerObj() == NULL))
    return FALSE;

  const NAString& objName = getQualifiedNameObj().getObjectName();
  if (onlyNew)
    return scope->context()->triggerObj()->isNewTransitionName(objName);
  else
    return scope->context()->triggerObj()->isTransitionName(objName);
}

//////////////////////////////////////////////////////////////////////////////
// This is a Scan on a temp table inside the action of a statement trigger.
// Add the uniqifier WHERE expression for it.
// Put a RelRoot node with a select list on top of it, to select only the 
// needed columns. Above the RelRoot, put a RenameTable node to change the
// scanned temp-table to the corrlation name used in the trigger action.
//////////////////////////////////////////////////////////////////////////////
RelExpr *Scan::buildTriggerTransitionTableView(BindWA *bindWA)
{
  BindScope *scope = bindWA->findNextScopeWithTriggerInfo();

  CMPASSERT((scope!=NULL) && (scope->context()->triggerObj() != NULL));

  StmtDDLCreateTrigger *createTriggerNode = 
    scope->context()->triggerObj();

  if (!createTriggerNode->isStatement())
  {
    // 11019 Only statement triggers can select from the transition table.
    *CmpCommon::diags() << DgSqlCode(-11019);
    bindWA->setErrStatus();
    return this;
  }

  // The transition table name must not have catalog/schema.
  QualifiedName& objName = userTableName_.getQualifiedNameObj();
  if  (objName.getSchemaName() != "")
  {
    // *** 4057 Correlation name MYNEW conflicts with qualified identifier of table CAT.SCHM.MYNEW.
    *CmpCommon::diags() << DgSqlCode(-4057)
	<< DgString0(objName.getObjectName())
	<< DgTableName(objName.getQualifiedNameAsAnsiString());
    bindWA->setErrStatus();
    return this;
  }

  if (bindWA->inDDL())
  {
    // This bind is for DDL semantic checks during a CREATE TRIGGER statement.
    // When the first trigger is created, the temp table does not yet exist,
    // so we replace the scan on the temp table by a scan on the subject
    // table. This is actually simpler since the column names stay the same.

    // Set the name of the table to be scanned to be the fully qualified 
    // subject table.
    if (userTableName_.getCorrNameAsString() == "")
      userTableName_.setCorrName(objName.getObjectName());
    objName = createTriggerNode->getTableNameObject();

    // Don't fix the name in the LocList.
    userTableName_.getQualifiedNameObj().setNamePosition(0);

    // Use the RI flag to skip incrementing the NATable reference counter.
    NABoolean inRIFlag = bindWA->getCurrentScope()->context()->inRIConstraint();
    bindWA->getCurrentScope()->context()->inRIConstraint() = TRUE;

    RelExpr *boundNode = bindNode(bindWA);
    if (bindWA->errStatus()) 
      return this;

    // Remove the SYSKEY column from the current RETDesc. It is illegal 
    // for a trigger action to select the SYSKEY from the transition table.
    ColRefName syskeyName("SYSKEY");
    bindWA->getCurrentScope()->getRETDesc()->delColumn(bindWA, syskeyName, SYSTEM_COLUMN);	

    // Restore the previous BindWA state.
    bindWA->getCurrentScope()->context()->inRIConstraint() = inRIFlag;

    return boundNode;
  }
  else
  {
    // OK - this is DML time - build the select list for the root and rename.

    QualifiedName& thisTable = userTableName_.getQualifiedNameObj();

    ChangesTable::RowsType scanType;
    if (createTriggerNode->isOldTransitionName(thisTable.getObjectName()))
    {
      scanType = ChangesTable::DELETED_ROWS;
    }
    else
    {
      CMPASSERT(createTriggerNode->isNewTransitionName(thisTable.getObjectName()));
      scanType = ChangesTable::INSERTED_ROWS;
    }

    CorrName subjectTable = createTriggerNode->getTableNameObject();
    subjectTable.setCorrName(userTableName_.getExposedNameAsString());

    TriggersTempTable tempTableObj(subjectTable, this, scanType, bindWA);

    RelExpr *transformedScan = tempTableObj.transformScan();

    return transformedScan->bindNode(bindWA);    // Bind the result
  }
}

//////////////////////////////////////////////////////////////////////////////
// This method does the insertion of a MVImmediate trigger in order to
// refresh the specified ON STATEMENT MJV after an insert operation.
//
// The refresh action is by default implemented as a statement after trigger.
// This implies that the trigger is always driven by a scan from the
// temp-table. The overhead of inserting into the temp-table, scanning it, and
// deleting from it afterwards is relatively high when the number of rows is
// quite small. So, when the insert is driven by a Tuple or TupleList node
// (with up to a certain number of tuples in it), we implement the refresh
// action as a row after trigger, and thus avoid using the temp-table at whole.
// When before triggers exist we don't consider the optimization, since the
// values in the Tuple (TupleList) are not the actual values inserted into the
// table (they are modified by the before trigger).
// 
//////////////////////////////////////////////////////////////////////////////
void Insert::insertMvToTriggerList(BeforeAndAfterTriggers *list,
				   BindWA                 *bindWA,
				   CollHeap               *heap,
				   const QualifiedName    &mvName,
				   MVInfoForDML           *mvInfo,
				   const QualifiedName    &subjectTable,
				   UpdateColumns          *updatedCols)
{
  CMPASSERT(getOperatorType() == REL_UNARY_INSERT);

  CorrName mvCorrName(mvName, heap);
  // The namespace is set in order to allow special update directly on the MV
  mvCorrName.setSpecialType(ExtendedQualName::MV_TABLE);

  // Instansiate the apprpriate builder
  MjvImmInsertBuilder *triggerBuilder = new(heap)
    MjvImmInsertBuilder(mvCorrName, mvInfo, this, bindWA);

  // By default, the refresh is implemented as an after statement trigger
  ComGranularity granularity = COM_STATEMENT;

  // If MV_AS_ROW_TRIGGER default value is ON or if we deal with insert of only
  // 1 row (i.e. the child is REL_TUPLE), generate the optimized refresh tree,
  // in which case the refresh is implemented as a row after trigger.
  if (child(0)->getOperatorType() == REL_TUPLE ||
      CmpCommon::getDefault(MV_AS_ROW_TRIGGER) == DF_ON)
  {
    // build the optimized version of the tree as requested
    granularity = COM_ROW;
    triggerBuilder->optimizeForFewRows();
  }

  // Instansiate the MVImmediate trigger
  MVImmediate *mvTrigger = new(heap) MVImmediate(bindWA,
						 triggerBuilder,
						 mvName,
						 subjectTable,
						 COM_INSERT,
						 granularity,
						 updatedCols);

  // Register the trigger in the general list of triggers

  if (granularity == COM_STATEMENT)
  {
    list->addNewAfterStatementTrigger(mvTrigger);
  }
  else
  {
    list->addNewAfterRowTrigger(mvTrigger);
  }
}

//////////////////////////////////////////////////////////////////////////////
// This method does the insertion of a MVImmediate trigger in order to
// refresh the specified ON STATEMENT MJV after an update operation.
//
// The update operation may be one of two types:
//
// 1. NONE of the updated columns participate in the clustering index of
//    the table and/or join predicate of the MJV. This is called direct update.
// 2. Some of the above columns are updated. This is called indirect update.
//
//    For direct update, a single row trigger that directly updates the
//    appropriate columns is sufficient to refresh the MJV.
//
//    For indirect update, we should have two MVImmediate triggers defined:
// 1) a row after trigger that deletes each row in the MJV that corresponds to
//    the updated ones.
// 2) a statement after trigger that inserts all the rows that result from
//    applying the join expression of the MJV between the delta on the subject
//    table (the updated rows sotred in the temp-table) and the other tables
//    participating in the MJV (the ones that were not changed).
//
//////////////////////////////////////////////////////////////////////////////
void Update::insertMvToTriggerList(BeforeAndAfterTriggers *list,
				   BindWA                 *bindWA,
				   CollHeap               *heap,
				   const QualifiedName    &mvName,
				   MVInfoForDML           *mvInfo,
				   const QualifiedName    &subjectTable,
				   UpdateColumns          *updatedCols)
{
  CMPASSERT(getOperatorType() == REL_UNARY_UPDATE);

  CorrName mvCorrName(mvName, heap);
  // The namespace is set in order to allow special update directly on the MV
  mvCorrName.setSpecialType(ExtendedQualName::MV_TABLE);

  switch(checkUpdateType(mvInfo, subjectTable, updatedCols))
  {
  case DIRECT:
    {
    /////////////////////////////////////////////////////////
    // adding a row trigger to directly update rows in the MV
    /////////////////////////////////////////////////////////

    // Instansiate the apprpriate builder
    MvRefreshBuilder *triggerBuilder = new(heap)
      MjvImmDirectUpdateBuilder(mvCorrName, mvInfo, this, bindWA);

    // Instansiate the MVImmediate trigger
    MVImmediate *mvTrigger = new(heap) MVImmediate(bindWA,
						   triggerBuilder,
						   mvName,
						   subjectTable,
						   COM_UPDATE,
						   COM_ROW,
						   updatedCols);

    // Register the trigger in the general list of triggers
    list->addNewAfterRowTrigger(mvTrigger);
    break;
    }
  case INDIRECT:
    {
    ///////////////////////////////////////////////////////////////////
    // PART I: adding a row trigger to delete updated rows from the MJV
    ///////////////////////////////////////////////////////////////////

    // Instansiate the apprpriate builder
    MvRefreshBuilder *rowTriggerBuilder = new(heap)
      MjvImmDeleteBuilder(mvCorrName, mvInfo, this, bindWA);

    // Instansiate the MVImmediate trigger
    MVImmediate *mvRowTrigger = new(heap) MVImmediate(bindWA,
						      rowTriggerBuilder,
						      mvName,
						      subjectTable,
						      COM_DELETE,
						      COM_ROW,
						      updatedCols);

    // Register the trigger in the general list of triggers
    list->addNewAfterRowTrigger(mvRowTrigger);

    //////////////////////////////////////////////////////////////////////////
    // PART II: adding a statement trigger to insert the new rows into the MJV
    //////////////////////////////////////////////////////////////////////////

    // Instansiate the apprpriate builder
    MvRefreshBuilder *stmtTriggerBuilder = new(heap)
      MjvImmInsertBuilder(mvCorrName, mvInfo, this, bindWA);

    // Instansiate the MVImmediate trigger
    MVImmediate *mvStmtTrigger = new(heap) MVImmediate(bindWA,
						       stmtTriggerBuilder,
						       mvName,
						       subjectTable,
						       COM_INSERT,
						       COM_STATEMENT,
						       updatedCols);

    // Register the trigger in the general list of triggers
    list->addNewAfterStatementTrigger(mvStmtTrigger);
    break;
    }
  default:
    break; // update is IRELEVANT - nothing to do
  }
}

//////////////////////////////////////////////////////////////////////////////
// This method checks which type of update is it:
// IRELEVANT, DIRECT or INDIRECT.
// 
// An update is considered as IRELEVANT until proven otherwise. The list of
// columns of the subject table that are in use by the MJV is scanned. For
// each column we check if it is updated in the current update operation. If
// so, the update is not IRELEVANT anymore, and is considered DIRECT until
// proven otherwise. Once we find an indirect-update column that is updated
// in the current operation, the update is considered INDIRECT and the loop
// is stopped at once. MJV columns of type complex cause INDIRECT update also.
//
//////////////////////////////////////////////////////////////////////////////
Update::MvUpdateType Update::checkUpdateType(MVInfoForDML *mvInfo,
					     const QualifiedName &subjectTable,
					     UpdateColumns *updatedCols) const
{
  MvUpdateType updateType = IRELEVANT;
  mvInfo->initUsedObjectsHash(); // initialization for the searching
  const MVUsedObjectInfo *mvUseInfo = mvInfo->findUsedInfoForTable(subjectTable);
  const LIST (Lng32) &colsUsedByMv = mvUseInfo->getUsedColumnList();
  for (CollIndex i = 0; i < colsUsedByMv.entries(); i++ )
  {
    // Check whether the column was updated in the current update operation.
    // If so, this update is not IRELEVANT anymore. Otherwise, skip this column.
    if (updatedCols->contains(colsUsedByMv[i]))
    {
      updateType = DIRECT;
    }
    else
    {
      continue;
    }

    // Check whether the column is an indirect-update column in the MJV, which
    // means that updating it may not only affect the corresponding row in the
    // MJV, but also affect other rows in it.
    if (mvUseInfo->isIndirectUpdateCol(colsUsedByMv[i]))
    {
      updateType = INDIRECT;
      break; // no further search is needed
    }
  }

  return updateType;
}

//////////////////////////////////////////////////////////////////////////////
// This method does the insertion of a MVImmediate trigger in order to
// refresh the specified ON STATEMENT MJV after a delete operation.
//
// The refresh action is implemented as a row after trigger.
//
//////////////////////////////////////////////////////////////////////////////
void Delete::insertMvToTriggerList(BeforeAndAfterTriggers *list,
				   BindWA                 *bindWA,
				   CollHeap               *heap,
				   const QualifiedName    &mvName,
				   MVInfoForDML           *mvInfo,
				   const QualifiedName    &subjectTable,
				   UpdateColumns          *updatedCols)
{
  CMPASSERT(getOperatorType() == REL_UNARY_DELETE);

  CorrName mvCorrName(mvName, heap);
  // The namespace is set in order to allow special update directly on the MV
  mvCorrName.setSpecialType(ExtendedQualName::MV_TABLE);

  // Instansiate the apprpriate builder
  MvRefreshBuilder *triggerBuilder = new(heap)
    MjvImmDeleteBuilder(mvCorrName, mvInfo, this, bindWA);

  // Instansiate the MVImmediate trigger
  MVImmediate *mvTrigger = new(heap) MVImmediate(bindWA,
						 triggerBuilder,
						 mvName,
						 subjectTable,
						 COM_DELETE,
						 COM_ROW,
						 updatedCols);

  // Register the trigger in the general list of triggers
  list->addNewAfterRowTrigger(mvTrigger);
}

/*****************************************************************************
******************************************************************************
****  Inlining methods of the GenericUpdate class
******************************************************************************
*****************************************************************************/

// This method was originaly called 
// setRETDescForTSJTree().
//
// For GenericUpdate Referential Integrity, Index Maintenance and Triggers
// (for IM, compare the createIM*() functions).
//
// Note that, if a RETDesc needs to be created, correlation names of
// "OLD@" and/or "NEW@" are given to the tables and columns.
// These names are safe to use because they contain a special non-Ansi
// character (the "@"), which is only accepted by Parser if a special
// "internal-only" flag is set as described for RI below.
//
// (Without the "@", the names "OLD" and "NEW" would NOT be safe to use:
// although they are in ReservedWords.h's PotentialAnsiReservedWords list,
// RI and IM could have a name collision ambiguity or misbinding due to a
// user table or index named "OLD" or "NEW" as a *delimited identifier*.
// Note also that since RI does need to use the Parser, we can't use
// CorrName::isFabricated()...)
//
// This function always sets gu's RETDesc to *non-empty*, signifying that
// this GenericUpdate produces outputs.  This RETDesc is also context
// for our being called multiple times.
//
// Concatenate **internal** correlation names (used by Binder)
// and **external** Ansi delimited-identifier names (used by Parser only).
//
// RI will use the latter set of names when building parseable predicate text;
// i.e. it should pass them to RefConstraint::getPredicateText().
//

RETDesc *GenericUpdate::createOldAndNewCorrelationNames(BindWA *bindWA, NABoolean createRETDescOnly)
{
  CMPASSERT(getOperatorType() == REL_UNARY_INSERT ||
            getOperatorType() == REL_UNARY_UPDATE ||
	    getOperatorType() == REL_UNARY_DELETE);

  CMPASSERT(getRETDesc());
  CMPASSERT(getRETDesc()->isEmpty());	

  RETDesc *rd;

  if (getOperatorType() != REL_UNARY_DELETE) 
  {
    // INSERT or UPDATE --
    // GenericUpdate::bindNode has previously set its TableDesc to the
    // desc whose column valueid's represent the new/source/after values.
    // Put these into a new RETDesc with columns all named "NEW@.<colname>"
    //
    CorrName corrName(getTableDesc()->getCorrNameObj().getQualifiedNameObj(),
	bindWA->wHeap(),
	NEWCorr);
    rd = new (bindWA->wHeap()) RETDesc(bindWA, getTableDesc(), &corrName);

    // IM: Add all columns in rd as local reference to current scope.
    // This is needed if the current scope describes a view which selects
    // a proper subset of all the basetable columns -- here we say that
    // yes, this view can produce *all* the b.t.cols as outputs...
    //
    ValueIdList vidList;
    rd->getValueIdList(vidList,  USER_AND_SYSTEM_COLUMNS);
    for (CollIndex i = 0; i < vidList.entries(); i++)
      bindWA->getCurrentScope()->addLocalRef(vidList[i]);

    // ##IM:## Having done the above for local refs, oughtn't we also now
    // ##recompute inputs?
    // ##  bindWA->getCurrentScope()->mergeOuterRefs(bindWA->getCurrentScope()->getOuterRefs());
    // ##  gu->getGroupAttr()->addCharacteristicInputs(bindWA->getCurrentScope()->getOuterRefs());
  }
  else
  {
    // DELETE -- init an empty RETDesc, and next merge in "OLD@.<col>"'s
    rd = new (bindWA->wHeap()) RETDesc(bindWA);
  }

  
  if ((getOperatorType() != REL_UNARY_INSERT) || 
      getUpdateCKorUniqueIndexKey() ||
      ((getOperatorType() == REL_UNARY_INSERT) &&((Insert *)this)->isMerge()) ||
      ((getOperatorType() == REL_UNARY_INSERT) &&((Insert *)this)->xformedEffUpsert()))
  {
    // DELETE or UPDATE --
    // Now merge the old/target/before valueid's (the Scan child RETDesc)
    // into this RETDesc such that these cols are all named "OLD@.<col>"
    //
    Scan *scan ;
    if (getOperatorType() != REL_UNARY_INSERT)
      scan = getScanNode();
    else 
      scan = getLeftmostScanNode();
   
    if ((getOperatorType() == REL_UNARY_INSERT) &&((Insert *)this)->xformedEffUpsert())
      {
	RelSequence *olapChild = getOlapChild();
	CorrName corrName(getTableDesc()->getCorrNameObj().getQualifiedNameObj(), 
			  bindWA->wHeap(),
			  OLDCorr);

	for (short i = 0; i< olapChild->getRETDesc()->getDegree();i++)
	  {
	    // we remembered if the original columns was from the right side of
	    // this olap node so add those to the RetDesc since those are the 
	    //ones we want to delete from the dependent indexes.
	    if ((olapChild->getRETDesc()->getValueId(i)).getItemExpr()->origOpType() == ITM_INSTANTIATE_NULL)
	      {		    
		rd->addColumn(bindWA, 
			      ColRefName(olapChild->getRETDesc()->getColRefNameObj(i).getColName(), corrName),
			      olapChild->getRETDesc()->getValueId(i),
			      USER_COLUMN,
			      olapChild->getRETDesc()->getHeading(i));
		    
	      }	    
	  }
	rd->addColumns(bindWA, *olapChild->getRETDesc()->getSystemColumnList(),  SYSTEM_COLUMN,&corrName);
	

      }	
    
    else
      {
	CMPASSERT(scan);
	CorrName corrName(scan->getTableDesc()->getCorrNameObj().getQualifiedNameObj(), 
			  bindWA->wHeap(),
			  OLDCorr);

	rd->addColumns(bindWA, *scan->getRETDesc(), &corrName);
      }
  }
   

  Set_SqlParser_Flags(ALLOW_FUNNY_IDENTIFIER);	// allow "@" processing
  Set_SqlParser_Flags(DELAYED_RESET); // allow multiple parser calls.

  CMPASSERT(!rd->isEmpty());

  // we only need the RETDesc to use it later for the transformation. Don't change the
  // RETDesc of the current operator or the current bind scope.
  if (!createRETDescOnly)
  {
    delete getRETDesc();			// safe because empty
    setRETDesc(rd);
    bindWA->getCurrentScope()->setRETDesc(rd);
  }

  return rd;

} // GenericUpdate::createOldAndNewCorrelationNames()

//////////////////////////////////////////////////////////////////////////////
// Add a virtual column to the RETDesc of this GenericUpdate node.
// Currently used for the Unique Execute ID column (before triggers),
// and for MV logging: the current Epoch, row type and row count.
//////////////////////////////////////////////////////////////////////////////
ValueId GenericUpdate::addVirtualColumn(BindWA     *bindWA, 
				        ItemExpr   *colExpr, 
				        const char *colName,
				        CollHeap   *heap)
{
  RETDesc *retDesc = getRETDesc();

  colExpr->bindNode(bindWA);
  ValueId exprValueId = wrapWithCastExpr(bindWA, colExpr->getValueId(), heap);
  ColRefName virtualColName(colName);
  retDesc->addColumn(bindWA, virtualColName, exprValueId);
  return exprValueId;
}

//////////////////////////////////////////////////////////////////////////////
// Inline the temp insert sub-tree directly into the backbone.
// The resulting tree looks like this:
//          TSJ
//         /  \
//   topNode   RelRoot
//               |
//             LeafInsert
//////////////////////////////////////////////////////////////////////////////
RelExpr *GenericUpdate::inlineTempInsert(RelExpr  *topNode, 
					 BindWA   *bindWA, 
					 TriggersTempTable& tempTableObj, 
					 NABoolean isDrivenByBeforeTriggers, 
					 NABoolean isTopMostTSJ,
					 CollHeap *heap)
{
  // The new top node should be a left TSJ for after triggers,
  // and a normal TSJ for before triggers.
  OperatorTypeEnum JoinType = 
    (  (isDrivenByBeforeTriggers || isTopMostTSJ)
       ? REL_TSJ
       : REL_LEFT_TSJ);
  
  if (isDrivenByBeforeTriggers)
    tempTableObj.setBeforeTriggersExist();

  RelExpr *insertNode = tempTableObj.buildInsert(!isDrivenByBeforeTriggers);
  if (bindWA->errStatus()) 
    return NULL;

  RelRoot *rootNode = new (heap) RelRoot(insertNode);
  rootNode->setRootFlag(FALSE);
  rootNode->setEmptySelectList();

  topNode = new(heap) Join(topNode, rootNode, JoinType);
  topNode->getInliningInfo().setFlags(II_DrivingTempInsert | 
                                      II_SingleExecutionForTriggersTSJ |
				      II_AccessSetNeeded);
  // Indicate to the Normalizer that this TSJ cannot be optimized away,
  // and that if the write operation is implemented via a cursor then 
  // it cannot be a "flow" cursor operation - i.e. it cannot utilize
  // a TSJFlow node.
  // Genesis case #10-990116-7164
  ((Join *)topNode)->setTSJForWrite(TRUE);
  if (bindWA->getHostArraysArea() && bindWA->getHostArraysArea()->getTolerateNonFatalError())
    {
      ((Join *)topNode)->setTolerateNonFatalError(RelExpr::NOT_ATOMIC_);
      ((Join *)topNode)->setTSJForSetNFError(TRUE);
    }
  return topNode;
}

//////////////////////////////////////////////////////////////////////////////
// Do we need Index Maintenance on this GenericUpdate node?
// We do if there are indexes except the clustering index. In case this is 
// an Update, at least one such index must match ant of the columns actually 
// being updated.
//////////////////////////////////////////////////////////////////////////////
NABoolean GenericUpdate::isIMNeeded(UpdateColumns *updatedColumns)
{
  NABoolean imNeeded = FALSE;

  if (!getTableDesc()->hasSecondaryIndexes()) 
    return FALSE;

  const LIST(IndexDesc *) indexList = getTableDesc()->getIndexes();
  for (CollIndex i=0; (i<indexList.entries()) && !imNeeded; i++) 
  {
    IndexDesc *index = indexList[i];

    // The base table itself is an index (the clustering index);
    // obviously IM need not deal with it.
    if (index->isClusteringIndex())
      continue;
 
    // An index always needs maintenance on an Insert or Delete...
    if((getOperatorType() != REL_UNARY_UPDATE) ||
       (isMerge()))
      imNeeded = TRUE;
    else
    {
      // This is Update - check if columns match.
      CMPASSERT(updatedColumns!=NULL);
      const ValueIdList &indexColumns = index->getIndexColumns();
      for (CollIndex j=0; j < indexColumns.entries() && !imNeeded; j++)
      {
	Lng32 indexCol = indexColumns[j].getNAColumn()->getPosition();
	if (updatedColumns->contains(indexCol))
	{
	  imNeeded = TRUE;
	  break;
	}
      } // for k
    } // else
  } // for j

  return imNeeded;
}

//////////////////////////////////////////////////////////////////////////////
// Inline Index Maintainance.
// We get here only if isIMNeeded() returned TRUE.
// The result looks like this:
//          TSJ
//         /   \
//   topNode    IM
//////////////////////////////////////////////////////////////////////////////
RelExpr *GenericUpdate::inlineIM(RelExpr   *topNode, 
				 BindWA    *bindWA, 
				 NABoolean isLastTSJ,
				 UpdateColumns *updatedColumns,
				 CollHeap  *heap,
				 NABoolean useInternalSyskey,
				 NABoolean rowTriggersPresents)
{
  // Create the tree that handles Index Maintainance.
  RelExpr *imTree = createIMTree(bindWA, updatedColumns, useInternalSyskey);

  // If no IM tree was created than we have a bug!
  CMPASSERT(imTree != NULL);

  if (bindWA->isTrafLoadPrep())
    return imTree;

  // Drive IM and RI trees using a TSJ on top of the GenericUpdate node.
  topNode = new(bindWA->wHeap()) 
    Join(topNode, 
	 imTree, 
	 (isLastTSJ ? REL_TSJ : REL_LEFT_TSJ));
  topNode->getInliningInfo().setFlags(II_DrivingIM | II_AccessSetNeeded);
  if (rowTriggersPresents)
     topNode->getInliningInfo().setFlags(II_SingleExecutionForTriggersTSJ);
  // Indicate to the Normalizer that this TSJ cannot be optimized away,
  // and that if the write operation is implemented via a cursor then 
  // it cannot be a "flow" cursor operation - i.e. it cannot utilize
  // a TSJFlow node.
  // Genesis case #10-990116-7164
  ((Join *)topNode)->setTSJForWrite(TRUE);

  return topNode;
}

//////////////////////////////////////////////////////////////////////////////
// Add to a trigger WHEN clause, the check if the trigger is enabled.
// Result condition ItemExpr tree looks like this:
//
//                           AND
//                          /   \
//              GetBitValueAt    WHEN clause
//	             /         \
// GetTriggerStatus       TriggerIndex
//
//////////////////////////////////////////////////////////////////////////////
static ItemExpr *addCheckForTriggerEnabled(BindWA    *bindWA,
					   ItemExpr  *whenClause, 
					   Trigger   *triggerObj,
					   CollHeap  *heap)
{
  // Register the trigger timestamp in the list managed by bindWA. The
  // returned value is the index into the trigger array for this RelExpr
  CollIndex triggerIndex = bindWA->addTrigger(triggerObj->getTimeStamp());

  CollIndex MaxTriggersPerStatement = MAX_TRIGGERS_PER_STATEMENT;
  
// debugging for coverage
#ifndef NDEBUG
  char* env = getenv("TESTING_MAX_TRIGGERS_PER_STATEMENT");
  if (env)
  {
     MaxTriggersPerStatement = atol(env);
  }
#endif

  if (triggerIndex >= MaxTriggersPerStatement)
  {
    // There are more than 256 triggers in this statement
    *CmpCommon::diags() << DgSqlCode(-11001);
    bindWA->setErrStatus();
    return NULL;
  }

  ItemExpr *enableCheck = new(heap) 
    GetBitValueAt(new(heap) GetTriggersStatus(), 
		  new(heap) ConstValue(triggerIndex) );

  // Check if whenClause is empty or TRUE
  if (whenClause == NULL || whenClause->getOperatorType() == ITM_RETURN_TRUE)
    return enableCheck;
  else {

    /*
    * for proper typing and transformation use correct AND subtree as follows:
    * (AND (AND (NOT_NULL GBVA) (NOT_EQUALS GBVA 0)) whenClause)
    * Here enableCheck is GBVA (GetBitValueAt) 
    */

    ItemExpr *notNullNode = new(heap) UnLogic (ITM_IS_NOT_NULL, enableCheck);
    
    ItemExpr *constZeroNode = new(heap) ConstValue(0);
    
    ItemExpr *notZeroNode = new(heap) BiRelat(ITM_NOT_EQUAL, enableCheck, constZeroNode);

    enableCheck = new(heap) BiLogic(ITM_AND, notNullNode, notZeroNode);
    
    return new(heap) BiLogic(ITM_AND, enableCheck, whenClause);
  }
}


// auxiliary function that reutrns an expression that wraps a RaiseError
// built-in function.
static ItemExpr *addTrigActionExcept (Trigger *trigObj, CollHeap *heap)
{
  ItemExpr *trigActionExceptExprPred =
     new (heap) RaiseError(ComDiags_TrigActionExceptionSQLCODE, 
                   trigObj->getTriggerName(),
                   trigObj->getSubjectTableName());
 
  return trigActionExceptExprPred;
}

//////////////////////////////////////////////////////////////////////////////
// This function is only called for row triggers. Look for all update 
// nodes in the action of the trigger and flag them as being in the action
// of a row trigger
//////////////////////////////////////////////////////////////////////////////
static void flagUpdateAsRowTrigger(RelExpr  *topNode)
{
   if (topNode)
   {
      if (topNode->getOperatorType() == REL_UNARY_UPDATE)
      {
         topNode->getInliningInfo().setFlags(II_InActionOfRowTrigger);
      }

      flagUpdateAsRowTrigger((RelExpr *)topNode->child(0));
      flagUpdateAsRowTrigger((RelExpr *)topNode->child(1));
   }
}

//////////////////////////////////////////////////////////////////////////////
// Statement triggers are blocked by an ordered union node, but then 
// driven to execute in parallel, by connecting them using a left linear tree
// of union nodes. Row triggers are similarly attached to each other, but are
// connected to the trigger backbone as part of the "Pipelined actions".
// The result looks like this for three triggers:
// The trigger transformation code expects this trigger group to be 
// in a left linear tree structure.
//
// Statement triggers:        Row triggers:
//          OU
//         /  \
//   topNode   U                   U
//            / \                 / \
//           U   ST3             U   RT3
//          / \                 / \
//        ST1  ST2            RT1  RT2
//////////////////////////////////////////////////////////////////////////////
static RelExpr *inlineTriggerGroup(RelExpr	     *topNode, 
				   const TriggerList *triggers, 
				   NABoolean	     isRow, 
				   CollHeap	     *heap,
				   BindWA	     *bindWA)
{
  RelExpr *topUnion = NULL;

  if ((triggers == NULL) || (triggers->entries() == 0))
    return topNode;

  // Now do the rest, and connect them with Union nodes.
  for (CollIndex i=0; i<triggers->entries(); i++)
  {
    // Get the Trigger object.
    Trigger *current = (*triggers)[i]; 
    // Get the trigger action tree.
    RelExpr *triggerTree = current->getParsedTrigger(bindWA);
    if (bindWA->errStatus())
      return NULL;

    triggerTree->getInliningInfo().setFlags(II_TriggerRoot);
    triggerTree->getInliningInfo().setTriggerObject(current);

    // The check whether the trigger is enabled is applicable only for regular
    // triggers. ON STATEMENT MVs (MVImmediate triggers) are always enabled.
    if ( !(current->isMVImmediate()) )
    {
      Union *triggerRoot = (Union *)triggerTree->getChild(0);  // Get past the RelRoot
      if (isRow)
        triggerRoot = (Union *)triggerRoot->getChild(0); // Get past the RenameReference
      if (triggerRoot != NULL && triggerRoot->getOperatorType() == REL_UNION)
      {
        // mark the update nodes as being in the action of a row trigger
        // so IM can be set appropriately of this case to avoid a data 
        // corruption - refer to method createIMNodes for more info on this issue
        flagUpdateAsRowTrigger((RelExpr *)triggerRoot->getChild(0));
        ItemExpr *whenClause = triggerRoot->removeCondExprTree();
        triggerRoot->addCondExprTree(
	  addCheckForTriggerEnabled(bindWA, whenClause, current, heap));
        triggerRoot->addTrigExceptExprTree(
	  addTrigActionExcept(current, heap));

	// if we are in a not atomic statement, set a flag in the unary_union node
	// this flag will passed to the executor through the union tdb. During execution
	// if condExpr() evaluates to truw and this flag is set, then error -30029 is raised.
	if (bindWA->getHostArraysArea() && 
	      bindWA->getHostArraysArea()->getTolerateNonFatalError()) 
	{
	  triggerRoot->setInNotAtomicStatement();
	}
      }
    }

    if (i == 0) // topUnion is still NULL?
      topUnion = triggerTree;
    else
    {
      topUnion = new(heap) Union(topUnion, triggerTree, NULL, NULL, REL_UNION, 
                                 CmpCommon::statementHeap(), TRUE);

      if (bindWA->getHostArraysArea() && 
        bindWA->getHostArraysArea()->getTolerateNonFatalError()) 
      {
	((Union *)topUnion)->setInNotAtomicStatement();
      }

    }

    topUnion->getInliningInfo().setFlags(II_AccessSetNeeded);

  }

  if (isRow) 
    return topUnion;
  else 
  {
    Union *newOU = new(heap) Union(topNode, topUnion, NULL, NULL, REL_UNION,
                                   CmpCommon::statementHeap(), TRUE);
    newOU->setOrderedUnion();
    newOU->setNoOutputs();
    newOU->getInliningInfo().setFlags(II_DrivingStatementTrigger | 
 				      II_AccessSetNeeded);

    if (bindWA->getHostArraysArea() && 
        bindWA->getHostArraysArea()->getTolerateNonFatalError()) 
      {
	newOU->setInNotAtomicStatement();
      }
    return newOU;
  }
}

//////////////////////////////////////////////////////////////////////////////
// Create a dummy sub-tree as a place-holder for missing pipelined actions
// (RI or row triggers), in order to maintain a regular tree.
// Such dummy statements are later removed by the trigger transformation code.
//////////////////////////////////////////////////////////////////////////////
static RelExpr *createDummyStatement(CollHeap *heap)
{
  Tuple *tupleNode = new(heap) Tuple(new(heap) ConstValue(0));
  tupleNode->getInliningInfo().setFlags(II_DummyStatement);

  RelRoot *result = new RelRoot(tupleNode);
  result->setRootFlag(FALSE);
  result->setEmptySelectList();
  return result;
}


//////////////////////////////////////////////////////////////////////////////
// Inline the pipelined actions: RI and row triggers.
// The root node above the TSJ has an empty select-list, so that the Union
// node above it will accept it as compatible with it's other child. It also
// will not open a new BindScope when bound, so that when the binding will 
// reach the already bound 'this' node - it will be in the same scope. This 
// avoids problems when calculating inputs.
//
// The resulting tree looks like this:
//         RelRoot
//           |
//          TSJ
//         /  \
//   topNode   U
//            / \
//          RI   Row
//               Triggers
////////////////////////////////////////////////////////////////////////////// 
RelExpr *GenericUpdate::inlinePipelinedActions(RelExpr *topNode, 
					       BindWA *bindWA, 
					       TriggerList *rowTriggers, 
					       RefConstraintList *riList,
					       CollHeap *heap)
{
  RelExpr *resultTree=topNode;
  
  if ((rowTriggers != NULL) || (riList != NULL))
  {
    RelExpr *rowTriggersTree=NULL;
    RelExpr *riTree=NULL;
    
    // Create the tree that handles all row triggers
    if (rowTriggers != NULL)
    {
      rowTriggersTree = inlineTriggerGroup(NULL,
					   rowTriggers,
					   TRUE,
					   heap,
					   bindWA);
    }
    else
    {
      rowTriggersTree = createDummyStatement(heap);
    }
    
    // Create the tree that handles Referencial Integrity.
    if (riList != NULL) 
      riTree = inlineRI(bindWA, riList, heap);
    else
      riTree = createDummyStatement(heap);
    
    resultTree = new(heap) Union(riTree, rowTriggersTree, NULL, NULL, 
                                 REL_UNION, CmpCommon::statementHeap(), TRUE);
    resultTree->getInliningInfo().setFlags(II_AccessSetNeeded);
    if (riList!=NULL) 
      resultTree->getInliningInfo().setFlags(II_DrivingRI);
    if (rowTriggers != NULL)
      resultTree->getInliningInfo().setFlags(II_DrivingRowTrigger);
    
    // Put a RelRoot on top of the Union to avoid trashing the current scope.
    resultTree = new(heap) RelRoot(resultTree);
    ((RelRoot *)resultTree)->setRootFlag(FALSE);
    ((RelRoot *)resultTree)->setEmptySelectList();
    
    // Drive IM and RI trees using a TSJ on top of the GenericUpdate node.
    NABoolean needsOutputs = isMtsStatement() ||
      (getUpdateCKorUniqueIndexKey() && (getOperatorType() == REL_UNARY_DELETE));
    OperatorTypeEnum joinType = needsOutputs ? REL_ANTI_SEMITSJ : REL_TSJ;
    resultTree = new(bindWA->wHeap()) Join(topNode, resultTree, joinType);
    resultTree->getInliningInfo().setFlags(II_AccessSetNeeded);
    resultTree->getInliningInfo().setFlags(II_DrivingPipelinedActions);

    // disable parallele execution for TSJs that control row triggers
    // execution. Parallel execution for triggers TSJ introduces the 
    // potential for non-deterministic execution 
    if (rowTriggers)
       resultTree->getInliningInfo().setFlags(II_SingleExecutionForTriggersTSJ);

    // Indicate to the Normalizer that this TSJ cannot be optimized away,
    // and that if the write operation is implemented via a cursor then 
    // it cannot be a "flow" cursor operation - i.e. it cannot utilize
    // a TSJFlow node.
    // Genesis case #10-990116-7164
    ((Join *)resultTree)->setTSJForWrite(TRUE);

  }
  
  RelRoot *rootNode = new(heap) RelRoot(resultTree);
  rootNode->setRootFlag(FALSE);
  rootNode->setEmptySelectList();
  rootNode->setDontOpenNewScope(); 
  
  return rootNode;
} // GenericUpdate::inlinePipelinedActions

//////////////////////////////////////////////////////////////////////////////
// Create the sub-tree that deletes the affected set from the temporary table.
// The corresponding SQL text is 
//    DELETE FROM <temp table> WHERE <Uniquifier> = <Uniquifier value>;
//
// The result looks like this:
//          OU
//         /  \
//   topNode   Delete
//              |
//             Scan (temp table)
//////////////////////////////////////////////////////////////////////////////
RelExpr *GenericUpdate::inlineTempDelete(BindWA   *bindWA, 
					 RelExpr  *topNode, 
					 TriggersTempTable& tempTableObj, 
					 CollHeap *heap)
{
  RelExpr *deleteSubTree = tempTableObj.buildDelete();

  Union   *newOU      = new(heap) Union(topNode, deleteSubTree, NULL, NULL,
                                        REL_UNION,CmpCommon::statementHeap(),
                                        TRUE);
  newOU->setOrderedUnion();
  newOU->getInliningInfo().setFlags(II_DrivingTempDelete | 
				    II_AccessSetNeeded);
  return newOU;
}

/*****************************************************************************
******************************************************************************
****  Methods for handling of before triggers.
******************************************************************************
*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// Find all the NEW@columns in the RETDesc of the GU that is about to be 
// replaced by a "tentative execution" node. These columns point to columns 
// in the IUD TableDesc that will soon be not valid anymore. The names NEW@.*
// should instead reference the appropriate expresions from the new record 
// expression.
//////////////////////////////////////////////////////////////////////////////
void GenericUpdate::fixTentativeRETDesc(BindWA *bindWA, CollHeap *heap)
{
  RETDesc *retDesc = getRETDesc();

  // The name of the subject table should no longer be in the XTNM.
  bindWA->getCurrentScope()->getXTNM()->remove(&getTableName());

  // Create a "virtual column" called @EXECID for the value of the Unique 
  // Execute ID. All the before trigger SIGNAL expressions will be piggy-
  // backed on this "column" before it is inserted into the temp table.
  ItemExpr *execId = new(heap) UniqueExecuteId();
  addVirtualColumn(bindWA, execId, InliningInfo::getExecIdVirtualColName(), heap);

  if (getOperatorType() == REL_UNARY_DELETE)
    return;  // No NEW@ values for Delete.

  CorrName newCorr(NEWCorr);
  ColumnDescList newCols(heap);
  ValueIdArray& newRecordExpr = newRecExprArray();
  ValueId *foundAssignToCol;

  // Get all the NEW@ columns from the GU's RETDesc.
  newCols.insert(*(retDesc->getQualColumnList(newCorr)));

  // The getQualColumnList() call does not return SYSKEY, so if it's there
  // find it and add it to the list.
  NAString    syskeyName("SYSKEY", heap);
  ColRefName  newSyskeyName(syskeyName, newCorr, heap);
  ColumnNameMap *newSyskeyMap = retDesc->findColumn(newSyskeyName);
  if (newSyskeyMap != NULL)
  {
    ColumnDesc *newSyskey = newSyskeyMap->getColumnDesc();
    // Make sure SYSKEY is not already in the list.
    CMPASSERT(!newCols.contains(newSyskey));
    newCols.insert(newSyskey);
  }

  for (CollIndex i=0; i<newCols.entries(); i++) // For each NEW@ column,
  {
    ValueId tempValueId;
    foundAssignToCol = NULL;
    const ColRefName& colName = newCols[i]->getColRefNameObj();

    // Lookup the column name in the target of an Assign node in the 
    // new record expression of the GU.
    for (CollIndex j=0; j<newRecordExpr.entries(); j++)
    {
      ItemExpr *currentExpr = newRecordExpr[j].getItemExpr(); 
      CMPASSERT(currentExpr->getOperatorType() == ITM_ASSIGN);
      Assign *currentAssign = (Assign *)currentExpr;
      
      ItemExpr *target = currentAssign->getTarget().getItemExpr();
      if (target->getOperatorType() == ITM_BASECOLUMN)
      {
	// Do the column names match?
	const NAString& trgtCol = ((BaseColumn *)target)->getColName();
	if (!trgtCol.compareTo(colName.getColName()))
	{
	  // Save the ValueId of the Assign source expression.
	  tempValueId = currentAssign->getSource();
          foundAssignToCol = &tempValueId;
	  // No need to check the rest of the new record expression.
	  break;  
	}
      }
    }  // for each Assign in the newRecExpr

    // Delete the current NEW@.* column from the RETDesc.
    if (colName.getColName() == "SYSKEY")
    {
      retDesc->delColumn(bindWA, colName, SYSTEM_COLUMN);
    }
    else
    {
      retDesc->delColumn(bindWA, colName, USER_COLUMN);
    }
    // it is no longer a local reference in the current scope.
    ValueId colValId = newCols[i]->getValueId();
    bindWA->getCurrentScope()->removeLocalRef(colValId);

    // Did we find a matching Assign expression?
    if (foundAssignToCol != NULL)
    {
      ValueId newExpr = *foundAssignToCol;

      // Yes - Make NEW@.* reference it.
      if (getGroupAttr()->isCharacteristicInput(newExpr))
	newExpr = wrapWithCastExpr(bindWA, newExpr, heap);

      retDesc->addColumn(bindWA, colName, newExpr);
    }
    else
    {
      // No - this must be a column that is not SET into, in an Update node.
      CMPASSERT (getOperatorType() == REL_UNARY_UPDATE);

      // OK - just reference the appropriate OLD@.* value.
      CorrName   oldCorr(OLDCorr);
      ColRefName oldColName(colName.getColName(), oldCorr);
      ValueId    oldCol = retDesc->findColumn(oldColName)->getValueId();

      // Wrap it with a Cast expression to give it a different ValueId
      // than the original OLD@.* column, thus marking it as a
      // duplicate entry in the XCNM.
      retDesc->addColumn(bindWA, colName, wrapWithCastExpr(bindWA, oldCol, heap));
    }
  }  // for each NEW@ column

  // The next block handles the value of the SYSKEY column in the 
  // temporary table for Insert operations. The problem is that the row
  // is inserted into the temp table before it is inserted into the 
  // subject table, and so - before the actual SYSKEY value was 
  // generated for it. Since this column is part of the primary key of
  // the temp table, and the rest of the primary key columns may be 
  // identical, this SYSKEY value must be unique for each row.
  // Solution - give it the value of JulianTimestamp.
  // The Timestamp ItemExpr is a new BuiltInFunction that is evaluated
  // for each row.
  if (getOperatorType() == REL_UNARY_INSERT) 
  {
    // Does this RETDesc have a NEW@.SYSKEY column?
    CorrName   newCorr(NEWCorr);
    ColRefName syskey("SYSKEY", newCorr);
    if (retDesc->findColumn(syskey) != NULL)
    {
      // code added to make sure that the generated julian
      // timestamp is unique. This change is added to support
      // bulk inserts (including rowset inserts) when before
      // row triggers are used.
      ItemExpr* counterExpr, *incrementExpr;
      counterExpr = new (heap) ItmPersistentExpressionVar(0);
      incrementExpr = new (heap) ItmBlockFunction
          (counterExpr, new (heap) Assign (counterExpr,
           new (heap) BiArith(ITM_PLUS, counterExpr,
           new (heap) ConstValue(1))));

      // Synthesize the types and value IDs for the new items
      incrementExpr->synthTypeAndValueId(TRUE);
      
      // Construct a new JulianTimestamp expression
      ItemExpr *fakeSyskey = new (heap) 
        BiArith(ITM_PLUS, incrementExpr, new (heap)
	  JulianTimestamp(new (heap) InternalTimestamp));

      fakeSyskey->bindNode(bindWA);

      // Make NEW@.SYSKEY point to the timestamp expression.
      retDesc->delColumn(bindWA, syskey, SYSTEM_COLUMN);
      retDesc->addColumn(bindWA, syskey, fakeSyskey->getValueId());
    }
  }  // if REL_UNARY_INSERT
}  // End of GenericUpdate::fixTentativeRETDesc()

//////////////////////////////////////////////////////////////////////////////
// For Update nodes, add the predicate: OLD@.<ci> = NEW@.<ci>
//////////////////////////////////////////////////////////////////////////////
void GenericUpdate::addPredicateOnClusteringKey(BindWA   *bindWA, 
						RelExpr  *tentativeNode, 
						CollHeap *heap)
{  
  TableDesc *newTableDesc = getTableDesc();
  const IndexDesc *newCI = newTableDesc->getClusteringIndex();
  const ValueIdList& newCICols = newCI->getIndexKey();
  CorrName newCorrName(NEWCorr);
  CorrName oldCorrName(OLDCorr);

  ItemExpr *predicate = NULL;
  for (CollIndex i=0; i<newCICols.entries(); i++)
  {
    ItemExpr *currentCol = newCICols[i].getItemExpr();
    CMPASSERT(currentCol->getOperatorType() == ITM_INDEXCOLUMN);
    IndexColumn *currentBaseCol = (IndexColumn *) currentCol;
    const NAString& colName = currentBaseCol->getNAColumn()->getColName();

    ColRefName *newColName = new(heap) ColRefName(colName, newCorrName, heap);
    ColRefName *oldColName = new(heap) ColRefName(colName, oldCorrName, heap);

    ItemExpr *currentPredicate = new(heap)
      BiRelat(ITM_EQUAL, 
              new(heap) ColReference(newColName),
              new(heap) ColReference(oldColName));

    if (predicate == NULL)
      predicate = currentPredicate;
    else
      predicate = new(heap) BiLogic(ITM_AND, predicate, currentPredicate);
  }

  CMPASSERT(predicate != NULL);
  predicate->bindNode(bindWA);
  if (bindWA->errStatus()) 
    return;

  tentativeNode->selectionPred().insert(predicate->getValueId());
}

//////////////////////////////////////////////////////////////////////////////
// This method replaces the GenericUpdate node that fired the triggers (this)
// with a "tentative execution" node.
//////////////////////////////////////////////////////////////////////////////
RelExpr *GenericUpdate::createTentativeGU(BindWA *bindWA, CollHeap *heap)
{
  // 'this' is the GU that started the whole mess. Let's kill it!
  // Create a dummy Rename node.
  RelExpr *tentativeNode = new(heap) 
    RenameTable(child(0), "TentativeGU");     
  // Fix the NEW@ cols to point to the right expressions.
  fixTentativeRETDesc(bindWA, heap); 
  // Give it my RETDesc with all the NEW and OLD stuff
  tentativeNode->setRETDesc(getRETDesc());      
  tentativeNode->setGroupAttr(getGroupAttr());

  ColumnNameMap *execIdCol = 
    getRETDesc()->findColumn(InliningInfo::getExecIdVirtualColName());
  CMPASSERT(execIdCol != NULL);

  ValueIdList outputs;
  getRETDesc()->getValueIdList(outputs, USER_AND_SYSTEM_COLUMNS);
  outputs.remove(execIdCol->getValueId());
  tentativeNode->getGroupAttr()->addCharacteristicOutputs(outputs);

  // For Update nodes, add the predicate: OLD@.<ci> = NEW@.<ci>s
  if (getOperatorType() == REL_UNARY_UPDATE)
  {
    addPredicateOnClusteringKey(bindWA, tentativeNode, heap);
    if (bindWA->errStatus()) 
      return NULL;
  }

  tentativeNode->markAsBound(); // No more binding is needed here.
  return tentativeNode;
}

//////////////////////////////////////////////////////////////////////////////
// Find the position of all columns SET to by this before Trigger, and add
// them to colToSet. This way, when we call TriggerDB for after triggers we
// will get triggers that fire on these columns too.
//////////////////////////////////////////////////////////////////////////////
void BeforeTrigger::addUpdatedColumns(UpdateColumns *colsToSet, 
				      const NATable *naTable)
{
  if (setList_ == NULL)  // Does this trigger have a SET clause?
    return;

  // For each Assign expression, add the col position to ColsToSet.
  for (CollIndex i=0; i<setList_->entries(); i++)
  {
    Lng32 targetColPosition = getTargetColumn(i, NULL, naTable);
    CMPASSERT(targetColPosition != -1);
    colsToSet->addColumn(targetColPosition);
  }
}

//////////////////////////////////////////////////////////////////////////////
// This method is called only when before triggers exist, and it builds the 
// left side of the inlining tree - the tentative execution tree.
// 1. Create the tentative GU node, that replaces this node.
// 2. Build a totem of before triggers on top of the tentative GU node.
//    Columns updated by before triggers are added to the list of columns
//    updated by the triggering statement, passed in colsToSet.
// 3. Inline the temp insert above the before triggers.
// 4. Add a RelRoot. This root has an empty select-list, so that the Union
//    node above it will accept it as compatible with it's other child. It
//    also will not open a new BindScope when bound, so that when the binding
//    will reach the already bound tentative node - it will be in the same
//    scope. This avoids problems when calculating inputs.
//
// The result looks like this, with 3 before triggers BT1, BT2 and BT3:
// (the triggers are already sorted by timestamp, BT1 is the oldest)
//          RelRoot
//            |
//           TSJ
//          /   \
//       BT3    Temp Insert
//        |
//       BT2
//        |
//       BT1
//        |
//    TentativeGU (replacing this)
//        |
//    this->child(0)
//////////////////////////////////////////////////////////////////////////////
RelExpr *GenericUpdate::createTentativeSubTree(BindWA *bindWA, 
					       TriggerList *beforeTriggers, 
					       UpdateColumns *updatedColumns, 
					       TriggersTempTable& tempTableObj,
					       CollHeap *heap)
{
  const NATable *naTable = getTableDesc()->getNATable();

  // Step 1.
  RelExpr *topNode = createTentativeGU(bindWA, heap);  
  if (bindWA->errStatus()) 
    return NULL;

  // Step 2.
  Trigger *current;
  BeforeTrigger *triggerNode=0;
  for (CollIndex i=0; i<beforeTriggers->entries(); i++)   
  {
    current = (*beforeTriggers)[i];
    triggerNode = (BeforeTrigger *)current->getParsedTrigger(bindWA);
    if (bindWA->errStatus())
      return this;

    CMPASSERT(triggerNode->getOperatorType() == REL_BEFORE_TRIGGER);

    // BeforeTrigger nodes are created childless,
    // And the child is always added before binding it.
    triggerNode->child(0) = topNode;
    topNode = triggerNode;

    if (updatedColumns != NULL) // Is this an UPDATE op?
    {
      // Add columns changed by the trigger to the columns changed
      // by the original UPDATE operation.
      triggerNode->addUpdatedColumns(updatedColumns, naTable);
    }
    ItemExpr *whenClause = triggerNode->getWhenClause();
    whenClause = addCheckForTriggerEnabled(bindWA, whenClause, current, heap);
    triggerNode->setWhenClause(whenClause);
  }

  // Step 3.
  topNode = inlineTempInsert(topNode, bindWA, tempTableObj, TRUE, TRUE, heap);  
  if (bindWA->errStatus()) 
    return NULL;
  // Save a pointer to the TSJ for tansformNod().
  CMPASSERT(triggerNode);
  triggerNode->setParentTSJ(topNode);  

  // Step 4
  RelRoot *rootNode = new(heap) RelRoot(topNode);
  rootNode->setRootFlag(FALSE);
  rootNode->setEmptySelectList();			
  rootNode->setDontOpenNewScope();
  return rootNode;
} 

//////////////////////////////////////////////////////////////////////////////
// The effective GU affects the rows in the affected set into the subject
// table itself. Since this operation is different in Insert, Update and
// Delete, I implemented it as a virtual method, that is implemented by the 
// child classes only.
//////////////////////////////////////////////////////////////////////////////
// we are not supposed to get here
RelExpr *GenericUpdate::createEffectiveGU(BindWA   *bindWA, 
					  CollHeap *heap, 
					  TriggersTempTable& tempTableObj,
					  GenericUpdate **effectiveGUNode,
					  UpdateColumns *colsToSet)
{
  CMPASSERT(FALSE); // Not supposed to get here !!!
  return NULL;
}

//////////////////////////////////////////////////////////////////////////////
// Create an Insert node that inserts the NEW@ values into the subject table.
//////////////////////////////////////////////////////////////////////////////
RelExpr *Insert::createEffectiveGU(BindWA   *bindWA, 
				   CollHeap *heap, 
				   TriggersTempTable& tempTableObj, 
				   GenericUpdate **effectiveGUNode,
				   UpdateColumns *colsToSet)
{
  // Create the Scan on the temporary table.
  Scan *tempScanNode = 
    tempTableObj.buildScan(ChangesTable::INSERTED_ROWS);

  CorrName& tempTable = tempScanNode->getTableName();
  ItemExpr *selectList = tempTableObj.buildBaseColsSelectList(tempTable);

  // Build a RelRoot on top of the Scan node.
  RelRoot *rootNode = new(heap) RelRoot(tempScanNode, REL_ROOT, selectList);
  rootNode->setRootFlag(FALSE);

  // Create an Insert node above the RelRoot.
  // The newRecExpr will be created during binding if the Insert node.
  GenericUpdate *gu = new(heap) 
    Insert(getTableName(), NULL, REL_UNARY_INSERT, rootNode);

  // If this GU is an action of a trigger - don't count rows affected.
  gu->rowsAffected() = rowsAffected(); 
  gu->getInliningInfo().setFlags(II_EffectiveGU);

  *effectiveGUNode = gu;
  return gu;
}

//////////////////////////////////////////////////////////////////////////////
// Create an Update node that updates rows in the subject table according to 
// primary key, and sets them to the NEW@ values.
// Normally, the Update node itself does not have any outputs, because the 
// OLD and NEW values are taken from the temp table. However, when MV 
// logging is needed, the Update node must project the CurrentEpoch column.
//////////////////////////////////////////////////////////////////////////////
RelExpr *Update::createEffectiveGU(BindWA   *bindWA, 
				   CollHeap *heap, 
				   TriggersTempTable& tempTableObj, 
    				   GenericUpdate **effectiveGUNode,
				   UpdateColumns *colsToSet)
{
  ItemExpr *assignList=NULL;
  Assign *assignExpr=NULL;
  CorrName newCorrName(NEWCorr);

  NABoolean mvLoggingRequired = isMvLoggingRequired();

  // Get the columns of the subject table.
  const NAColumnArray &subjectColumns = 
    getTableDesc()->getNATable()->getNAColumnArray(); 

  for (CollIndex i=0; i<subjectColumns.entries(); i++)
  {
    // If this column was not SET into, no need to change it.
    if (!colsToSet->contains(i))
      continue;

    NAColumn *currentColumn = subjectColumns.getColumn(i);
    const NAString &colName = currentColumn->getColName();

    // Cannot update a clustering/primary key column!
    // This error is caught during binding in DDL. 
    // see BeforeTrigger::bindSetClause() in BindRelExpr.cpp
    CMPASSERT(!currentColumn->isClusteringKey() && !currentColumn->isPrimaryKey());

    NAString tempColName(colName);
    assignExpr = new(heap) 
      Assign(new(heap) ColReference(
	new(heap) ColRefName(colName)),
	new(heap) ColReference(new(heap) ColRefName(tempColName, newCorrName)),
	FALSE);
    if (assignList==NULL)
      assignList = assignExpr;
    else
      assignList = new(heap) ItemList(assignExpr, assignList);
  }

  // The selection predicate on the Scan is on the NEW@ clustering index cols.
  // The left side of the equation is using the "@SYSKEY" column while the right
  // side uses the SYSKEY column.
  NAString newName(NEWCorr);
  ItemExpr *selectionPredicate = new(heap)
    BiRelat(ITM_EQUAL,
	    tempTableObj.buildClusteringIndexVector(&newName,TRUE),
	    tempTableObj.buildClusteringIndexVector() );

  Scan *baseScanNode = new(heap) Scan(getTableName());
  baseScanNode->addSelPredTree(selectionPredicate);
  GenericUpdate *effectiveGu = new(heap) 
    Update(getTableName(), NULL, REL_UNARY_UPDATE, baseScanNode, assignList);

  // If this GU is an action of a trigger - don't count rows affected.
  effectiveGu->rowsAffected() = rowsAffected(); 
  effectiveGu->getInliningInfo().setFlags(II_EffectiveGU);

  RelRoot *effectiveRoot = new(heap) RelRoot(effectiveGu);
  if (!mvLoggingRequired)
    effectiveRoot->setEmptySelectList();

  RelExpr *joinTemps = tempTableObj.buildOldAndNewJoin();

  OperatorTypeEnum joinType = mvLoggingRequired ? REL_TSJ : REL_LEFT_TSJ;
  Join *tsjNode = new(heap) Join(joinTemps, effectiveRoot, joinType);
  tsjNode->setTSJForWrite(TRUE);

  *effectiveGUNode = effectiveGu;
  return tsjNode;
}

//////////////////////////////////////////////////////////////////////////////
// Create a Delete node that deletes rows from the subject table according
// to the primary from the row read by the temp Scan node.
//////////////////////////////////////////////////////////////////////////////
RelExpr *Delete::createEffectiveGU(BindWA   *bindWA, 
  				   CollHeap *heap, 
				   TriggersTempTable& tempTableObj, 
				   GenericUpdate **effectiveGUNode,
				   UpdateColumns *colsToSet)
{
  // Create the Scan on the temporary table.
  Scan *tempScanNode = tempTableObj.buildScan(ChangesTable::DELETED_ROWS);

  Delete *gu = new(heap) 
    Delete(getTableName(), NULL, REL_UNARY_DELETE, tempScanNode);

  // If this GU is an action of a trigger - don't count rows affected.
  gu->rowsAffected() = rowsAffected(); 
  gu->getInliningInfo().setFlags(II_EffectiveGU);

  *effectiveGUNode = gu;
  return gu;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RelExpr *GenericUpdate::bindEffectiveGU(BindWA *bindWA)
{
  if (getInliningInfo().hasPipelinedActions())
  {
    setNoFlow(TRUE);

    if (getOperatorType() != REL_UNARY_UPDATE)
    {
      createOldAndNewCorrelationNames(bindWA);
    }

    if (isMvLoggingRequired())
    {
      prepareForMvLogging(bindWA, bindWA->wHeap());
    }

    ValueIdList outputs;
    getRETDesc()->getValueIdList(outputs, USER_AND_SYSTEM_COLUMNS);
    setPotentialOutputValues(outputs);
  }
  // case of before triggers and after statement triggers where the after triggers are in conflict
  // and the effective GU is either an insert or a delete.
  else if (getOperatorType() != REL_UNARY_UPDATE)
  {
      RETDesc *rd = createOldAndNewCorrelationNames(bindWA, TRUE /* only create RETDesc */);
      getInliningInfo().buildTriggerBindInfo(bindWA, rd, bindWA->wHeap());
      delete rd;
  }
  // indicate that this is a subject table for enable/disable
  getOptStoi()->getStoi()->setSubjectTable(TRUE);

  return this;
}

/*****************************************************************************
******************************************************************************
****  Methods for handling Index Maintenance.
******************************************************************************
*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// See comments in common/ComTransInfo.h
//////////////////////////////////////////////////////////////////////////////
static void setScanLockForIM(const RelExpr *re)
{
  if (re->getOperator().match(REL_SCAN)) {
     Scan *rs = (Scan *)re;
     rs->accessOptions().setScanLockForIM(TRUE);
  }
  for (Int32 i = 0; i < re->getArity(); ++i ) {
    if (re->child(i)) 
       setScanLockForIM(re->child(i));
  }
}


// All table info in these createIM*() methods must come from the TableDesc.
// In particular, use of getTableName() is wrong:  that is the name of the
// topmost view if the target table is a view.  The TableDesc always represents
// the underlying *base* table.
//
RelExpr *GenericUpdate::createIMTree(BindWA *bindWA,
				     UpdateColumns *updatedColumns,
				     NABoolean useInternalSyskey)
{
  RelExpr *imTree = NULL;

  NAString origCorr(getTableDesc()->getCorrNameObj().getCorrNameAsString(),
		    bindWA->wHeap());

  const LIST(IndexDesc *) indexList = getTableDesc()->getIndexes();
  for (CollIndex i=0; i < indexList.entries(); i++) {

    IndexDesc *index = indexList[i];

    // The base table itself is an index (the clustering index);
    // obviously IM need not deal with it.
    //
    if (!index->isClusteringIndex()) {

      // An index always needs maintenance on an Insert or Delete...
      //
      NABoolean imNeeded = 
	((getOperatorType() != REL_UNARY_UPDATE) ||
	 (isMerge()));

      // ...but for an Update, it needs maint if it contains any of the columns
      // being updated.  The test for intersection must use column position
      // (which is always the position in the *base table*, so comparisons
      // *are* meaningful!).  Someday should be abstracted into a shared ##
      // method also used by ItemConstr.h ColSignature stuff... 	 ##
      //
      if (!imNeeded) {
        const ValueIdList &indexColumns = index->getIndexColumns();

      for (CollIndex j=0; j < indexColumns.entries() && !imNeeded; j++) {
        Lng32 indexCol = indexColumns[j].getNAColumn()->getPosition();

        if (updatedColumns != NULL)  // -- Triggers
          imNeeded |= updatedColumns->contains(indexCol);
        else {
          for (CollIndex k=0; k < newRecExprArray().entries(); k++) {
            Lng32 tableCol = newRecExprArray()[k].getItemExpr()->child(0).
	      					  getNAColumn()->getPosition();

            if (indexCol <= tableCol) {
              if (indexCol == tableCol) imNeeded = TRUE;
                break;  // newRecExprArray is ordered by position, so break if <=
			}
		  } // for k
		} // else
	  } // for j

	#ifndef NDEBUG
	  if (imNeeded && GU_DEBUG)
	    cerr << "imNeeded: " << index->getNAFileSet()->getExtFileSetName()
	         << endl;
	#endif
      } // Update, need to test whether IM is needed for this index

      if (imNeeded)
        if (!imTree)
        {
          imTree = createIMNodes(bindWA, useInternalSyskey,
                                 index, producedMergeIUDIndicator_);
          if (getOperatorType() == REL_UNARY_UPDATE ||
              getOperatorType() == REL_UNARY_DELETE)
            setScanLockForIM(child(0));
          if (bindWA->isTrafLoadPrep())
            imTree->setChild(0,this);
        }
        else
        {
          if (!bindWA->isTrafLoadPrep())
          {
            imTree = new (bindWA->wHeap()) 
              Union(imTree, createIMNodes(bindWA, useInternalSyskey,
                                          index, producedMergeIUDIndicator_),
                    NULL, NULL, REL_UNION, CmpCommon::statementHeap(), TRUE, TRUE);
            imTree->setBlockStmt(isinBlockStmt());
            imTree->getInliningInfo().setFlags(II_isIMUnion);
          } // not bulk load
          else {
            RelExpr * oldIMTree = imTree;
            imTree = createIMNodes(bindWA, useInternalSyskey, index,
                                   producedMergeIUDIndicator_);
            imTree->setChild(0,oldIMTree);
          } // is bulk load
        }

    } // !clusteringIndex
  } // loop over all indexes

  // ##IM: This extra RelRoot is probably unnecessary (wasteful),
  // ##    due to createIMNode*() always returning a RelRoot-topped tree --
  // ##    but I didn't have time to remove it and re-test.
  if (imTree && imTree->getOperatorType() != REL_ROOT)
    imTree = new (bindWA->wHeap()) RelRoot(imTree);

  getTableDesc()->getCorrNameObj().setCorrName(origCorr);

  return imTree;
} // GenericUpdate::createIMTree()


// Here, we make virgin ColReferences, which when bound will be found
// in an outer scope engendered by our (and/or GenericUpdate's) interposing
// a RelRoot between us and our parent GenericUpdate --
// and the LeafXxx will add these outer refs to its characteristic inputs.
static RelExpr *createIMNode(BindWA *bindWA, 
			     CorrName &tableCorrName,
			     const CorrName &indexCorrName,
			     IndexDesc *index,
                             const ValueId &mergeIUDIndicator,
			     NABoolean isIMInsert,
			     NABoolean useInternalSyskey,
                             NABoolean isForUpdateOrMergeUpdate,
                             NABoolean mergeDeleteWithInsertOrMergeUpdate,
			     NABoolean isEffUpsert)
{
   
 // See createOldAndNewCorrelationNames() for info on OLDCorr/NEWCorr
  
  // A merge statement can perform an update or an insert if a matching row 
  // is detected or not, correspondingly. In either case, a update operation is 
  // performed on the index table. If a unique index is involved, a update
  // operation would be able to delete a corresponding row in the index table
  // using the index "key" column before inserting a new row. If it is a insert
  // operation on the base table, then a update on the index table should not 
  // delete a row that corresponds to a different row in the base table(index
  // key belonging to a different row). Hence in this case, it is better to always
  // match not only the index key but also remaining columns in the index table
  // that correspond to the base table. Hence we introduce 
  // robustDelete below. This flag could also be called 
  // isIMOnAUniqueIndexForMerge
  NABoolean robustDelete = (mergeDeleteWithInsertOrMergeUpdate && index->isUniqueIndex()) || 
                           (isEffUpsert && index->isUniqueIndex());

  tableCorrName.setCorrName(isIMInsert ?  NEWCorr : OLDCorr);
  
  ItemExprList *colRefList = new(bindWA->wHeap()) ItemExprList(bindWA->wHeap());
  
  const ValueIdList &indexColVids = ((isIMInsert || robustDelete )? 
				     index->getIndexColumns() : 
				     index->getIndexKey());
  ItemExpr *preCond = NULL; // pre-condition for delete, insert if any
  for (CollIndex i=0; i < indexColVids.entries(); i++) {

    const NAString &colName = indexColVids[i].getNAColumn()->getColName();

    NAString realColName = colName;
    if (useInternalSyskey && colName == "SYSKEY")
    {
      realColName = "@SYSKEY";
    }

    ColReference *colRef =
      new (bindWA->wHeap()) ColReference
        (new (bindWA->wHeap()) ColRefName (realColName, tableCorrName, bindWA->wHeap()));

    colRefList->insert(colRef);
  }

  // There are 4 cases here. Following table shows when precondition
  // expression is addded
  // Index Type/IM operation->  Delete  | Insert
  // Non-unique Index           Yes        No 
  // Unique Index               Yes        Yes
  if ((!isIMInsert && isForUpdateOrMergeUpdate)||robustDelete || (!isIMInsert && isEffUpsert))
    {
      // For delete nodes that are part of an update or merge update or upsert,
      // generate a comparison expression between old and new index column 
      // values and suppress the delete if no columns change. This avoids the
      // situation where we delete and then re-insert the same index
      // row within one millisecond and get the same HBase timestamp
      // value assigned. In that case, the delete will win out over
      // the insert, even though the insert happens later in time. The
      // HBase-trx folks are also working on a change to avoid that.
      // similar checks are added for unique index insert (into the index
      // table only, using the robustDelete flag above). 
      // Since we do checkandput for unique indexes, putting 
      // an existing row (key + value) will raise a uniqueness violation,
      // while from the user's point of view no change in the uninque index
      // table is expected.

      CorrName predValues(tableCorrName);

      if (isIMInsert)
	predValues.setCorrName(OLDCorr);
      else
	predValues.setCorrName(NEWCorr);

      for (CollIndex cc=0; cc<colRefList->entries(); cc++)
        {
          ColReference *predColRef = static_cast<ColReference *>
	    ((*colRefList)[cc]);

	  BiRelat *comp1Col  = NULL;
          // create a predicate OLD@.<col> = NEW@.<col>
	  comp1Col = new (bindWA->wHeap())
	    BiRelat(ITM_EQUAL,
		    predColRef,
		    new (bindWA->wHeap()) ColReference(
			 new (bindWA->wHeap()) ColRefName(
	    		 predColRef->getColRefNameObj().getColName(),
			 predValues,
			 bindWA->wHeap())),
                    TRUE); // special NULLs, treat NULL == NULL as true

          if (preCond == NULL)
            preCond = comp1Col;
          else
            preCond = new (bindWA->wHeap()) BiLogic(ITM_AND, preCond, comp1Col);
        }

      // the actual condition is that the values are NOT the same
      preCond = new (bindWA->wHeap()) UnLogic(ITM_NOT, preCond);
    }

  // If we got an IUD indicator passed in, that means that we have
  // an IM tree for update, but the actual operation may be an
  // insert or a delete, and therefore we may need to suppress the
  // index delete or insert, respectively, with a precondition.
  if (mergeIUDIndicator != NULL_VALUE_ID)
    {
      ItemExpr *iudCond = 
        new(bindWA->wHeap()) BiRelat(
             ITM_NOT_EQUAL,
             mergeIUDIndicator.getItemExpr(),
             new(bindWA->wHeap()) SystemLiteral(
                  (isIMInsert ? "D" : "I"),
                  CharInfo::ISO88591));

      if (preCond == NULL) 
        preCond = iudCond;
      else
        preCond = new (bindWA->wHeap()) BiLogic(
             ITM_AND,
             iudCond,
             preCond);
    }

  // NULL tableDesc here, like all Insert/Update/Delete ctors in SqlParser,
  // because the LeafXxx::bindNode will call GenericUpdate::bindNode
  // which will do the appropriate createTableDesc.
  GenericUpdate *imNode;
  if (isIMInsert)
    {
      if (!bindWA->isTrafLoadPrep())
      {
        imNode = new (bindWA->wHeap()) 
	  LeafInsert(indexCorrName, NULL, colRefList, REL_LEAF_INSERT,
		     preCond, bindWA->wHeap());
        HostArraysWA * arrayWA = bindWA->getHostArraysArea() ;
        if (arrayWA && arrayWA->hasHostArraysInTuple()) {
          if (arrayWA->getTolerateNonFatalError() == TRUE)
            imNode->setTolerateNonFatalError(RelExpr::NOT_ATOMIC_);    
        }
        // For index maintenance on non-unique HBase indexes we can use a put.
        // In fact we must use a put, otherwise we'll get a uniqueness constraint
        // violation for those rows that didn't change and therefore didn't get
        // deleted, due to the precondition (see below).
        if (!index->isUniqueIndex())
          imNode->setNoCheck(TRUE);
      } // regular insert
      else {
        imNode = new (bindWA->wHeap()) Insert(indexCorrName,
                 NULL,
                 REL_UNARY_INSERT,
                 NULL);
        ((Insert *)imNode)->setBaseColRefs(colRefList);
        ((Insert *)imNode)->setInsertType(Insert::UPSERT_LOAD);
        ((Insert *)imNode)->setIsTrafLoadPrep(true);
        ((Insert *)imNode)->setNoIMneeded(TRUE);
      } // traf load prep
    }
  else
    {
      imNode = new (bindWA->wHeap()) LeafDelete(indexCorrName,
                                                NULL,
                                                colRefList,
                                                (robustDelete )?TRUE:FALSE,
                                                REL_LEAF_DELETE,
                                                preCond,
                                                bindWA->wHeap());
      imNode->setReferencedMergeIUDIndicator(mergeIUDIndicator);
    }

   // The base table's rowsAffected() will get set in ImplRule.cpp,
  // but we don't want any of these indexes' rowsAffected to be computed
  // (if I insert one row into a table, I want to see "1 row(s) inserted",
  // not 1 + number of indexes being maintained!).
  imNode->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;

  // Do not collect STOI info for security checks.
  imNode->getInliningInfo().setFlags(II_AvoidSecurityChecks);
  
  // Set the flag that this GU is part of IM
  imNode->getInliningInfo().setFlags(II_isIMGU);

  if (bindWA->isTrafLoadPrep())
    return imNode;

  // Add a root here to prevent error 4056 when binding the LeafDelete+Insert
  // pair for an Update.
  return new (bindWA->wHeap()) RelRoot(imNode);
} // static createIMNode()

RelExpr *GenericUpdate::createIMNodes(BindWA *bindWA,
				      NABoolean useInternalSyskey,
				      IndexDesc *index,
                                      const ValueId &mergeIUDIndicator)
{
  // We call getExtFileSetObj (returns QualifiedName),
  // NOT getExtFileSetName (returns NAString),
  // hence the CorrName ctor sets up entire QualifiedName,
  // NOT just an (erroneously delimited) objectName part of one.
  //
  CorrName indexCorrName(index->getNAFileSet()->getExtFileSetObj());
  indexCorrName.setSpecialType(ExtendedQualName::INDEX_TABLE);

  CorrName &tableCorrName = getTableDesc()->getCorrNameObj();
  if(tableCorrName.isVolatile())
    indexCorrName.setIsVolatile(TRUE);

  RelExpr *indexInsert = NULL, *indexDelete = NULL, *indexOp = NULL;
  NABoolean isForUpdateOrMergeUpdate = (getOperatorType() == REL_UNARY_UPDATE ||
                           isMergeUpdate());
  
  NABoolean isEffUpsert = ((getOperatorType() == REL_UNARY_INSERT) && ((Insert *)this)->xformedEffUpsert());
  if (indexCorrName.getUgivenName().isNull())
    {
      indexCorrName.setUgivenName(tableCorrName.getUgivenName());
    }

  // Create a list of base columns ColReferences for
  // ALL the index columns as AFTER/NEW columns.
  //
  if (getOperatorType() == REL_UNARY_INSERT ||
      getOperatorType() == REL_UNARY_UPDATE || isEffUpsert)
    indexInsert = indexOp = createIMNode(bindWA,
					 tableCorrName,
					 indexCorrName,
					 index,
                                         mergeIUDIndicator,
					 TRUE,
					 useInternalSyskey,
                                         isForUpdateOrMergeUpdate,
                                         isMerge(),
					 isEffUpsert);

  // Create a list of base columns ColReferences for
  // ONLY the index KEY columns as BEFORE/OLD columns.
  //
  if (getOperatorType() == REL_UNARY_DELETE ||
      getOperatorType() == REL_UNARY_UPDATE ||
      isEffUpsert)
    {
      NABoolean mergeDeleteWithInsertOrMergeUpdate = isMerge();
      if (mergeDeleteWithInsertOrMergeUpdate && 
          (getOperatorType() == REL_UNARY_DELETE) && 
          (!insertValues()))
        // merge delete without an insert
        mergeDeleteWithInsertOrMergeUpdate = FALSE;
      
      indexDelete = indexOp = createIMNode(bindWA,
					 tableCorrName,
                                         indexCorrName,
					 index,
                                         mergeIUDIndicator,
					 FALSE,
    			       		 useInternalSyskey,
                                         isForUpdateOrMergeUpdate,
                                         mergeDeleteWithInsertOrMergeUpdate,
					 isEffUpsert);
    }

  if ((getOperatorType() == REL_UNARY_UPDATE) || isEffUpsert){
    indexOp = new (bindWA->wHeap()) Union(indexDelete, indexInsert, 
                                          NULL, NULL, REL_UNION, 
                                          CmpCommon::statementHeap(),TRUE,TRUE);

    // is this in a compound statement?
    indexOp->setBlockStmt(isinBlockStmt());

    // is this GU driven by a row trigger
    // this a temporary fix to prevent data corruption when
    // the update operation is in the action of a row
    // trigger. The data corruption is the result of the 
    // insert side of the IM tree lagging behind the delete
    // in the case where multiple request to update the same rows
    // are flowing to this update node.
    // This is also the case when updates are being driven 
    // by rowsets.
    // Changing this code that  unconditionally blocked the ordered union
    // to handle all cases of IM updates.
    //We can use the ordered union in the case where we have the sequence operator 
    // in the tree on the left side to remove duplicates before it flows to the 
    // IM tree. This is to improve performance.
    // 
    if (this->getInliningInfo().isInActionOfRowTrigger() ||
        (bindWA->getHostArraysArea() && !isEffUpsert))
    {
      ((Union *)indexOp)->setBlockedUnion();
    }
    else
    {
      ((Union *)indexOp)->setOrderedUnion();
    }

    // Add a root just to be consistent, so all returns from this method
    // are topped with a RelRoot.

    // Set this Union is part of IM
    indexOp->getInliningInfo().setFlags(II_isIMUnion);
    indexOp = new (bindWA->wHeap()) RelRoot(indexOp);
  }

  return indexOp;
} // GenericUpdate::createIMNodes()


/*****************************************************************************
******************************************************************************
****  Methods for handling Undo (for an insert) .
******************************************************************************
*****************************************************************************/

// All table info in these createUndo*() methods must come from the TableDesc.
// In particular, use of getTableName() is wrong:  that is the name of the
// topmost view if the target table is a view.  The TableDesc always represents
// the underlying *base* table.
//
RelExpr *GenericUpdate::createUndoTree(BindWA *bindWA,
				       UpdateColumns *updatedColumns,
				       NABoolean useInternalSyskey,
				       NABoolean imOrRiPresent,
				       NABoolean ormvPresent,
				       TriggersTempTable  *tempTableObj)
{
  RelExpr *undoTree = NULL;

  NAString origCorr(getTableDesc()->getCorrNameObj().getCorrNameAsString(),
		    bindWA->wHeap());

  const LIST(IndexDesc *) indexList = getTableDesc()->getIndexes();
 

   
  NABoolean undoNeeded = getOperatorType() == REL_UNARY_INSERT;
  if (!undoNeeded)
    return NULL;
  if (imOrRiPresent)
    {
      for (CollIndex i=0; i < indexList.entries(); i++) 
	{

	  IndexDesc *index = indexList[i];
	  if (undoNeeded)
	    if (!undoTree)
	      undoTree = createUndoNodes(bindWA, useInternalSyskey, index);
	    else
	      {
		undoTree = new (bindWA->wHeap()) 
		  Union(undoTree, createUndoNodes(bindWA, useInternalSyskey, index),
			NULL, NULL, REL_UNION, CmpCommon::statementHeap(), TRUE, TRUE);
		undoTree->setBlockStmt(isinBlockStmt());
	      }

	  
	} // loop over all indexes
    }
  if (ormvPresent)
    {
      if (!undoTree)
	undoTree = createUndoIUDLog(bindWA);
      else	
	undoTree = new (bindWA->wHeap()) 
	  Union(undoTree, createUndoIUDLog(bindWA),
		NULL, NULL, REL_UNION, CmpCommon::statementHeap(), TRUE, TRUE);
    }
  if (tempTableObj)
    {
      if (!undoTree)
	undoTree = createUndoTempTable(tempTableObj,bindWA);
      else	
	undoTree = new (bindWA->wHeap()) 
	  Union(undoTree, createUndoTempTable(tempTableObj,bindWA),
		NULL, NULL, REL_UNION, CmpCommon::statementHeap(), TRUE, TRUE); 
    }

  // ##IM: This extra RelRoot is probably unnecessary (wasteful),
  // ##    due to createIMNode*() always returning a RelRoot-topped tree --
  // ##    but I didn't have time to remove it and re-test.
  if (undoTree && undoTree->getOperatorType() != REL_ROOT)
    undoTree = new (bindWA->wHeap()) RelRoot(undoTree);

  getTableDesc()->getCorrNameObj().setCorrName(origCorr);

  return undoTree;
} // GenericUpdate::createUndoTree()


RelExpr * GenericUpdate::createUndoTempTable(TriggersTempTable *tempTableObj,BindWA *bindWA)
{
  
  // TriggersTempTable *tempTableObj = new(bindWA->wHeap()) TriggersTempTable(this, bindWA);
  const NAColumnArray &tempColumns = tempTableObj->getNaTable()->getNAColumnArray(); 
  ItemExprList *tempColRefList = new(bindWA->wHeap()) ItemExprList(bindWA->wHeap());
  /*  CorrName tempCorrName = *(tempTableObj->calcTargetTableName(tempTableObj->getSubjectTableName().getQualifiedNameObj()));*/
  CorrName tempCorrName = *(tempTableObj->getTableName());
  CorrName origTempCorrName = tempCorrName;
  tempCorrName.setCorrName( NEWCorr);
  for (CollIndex i=0; i<tempColumns.entries(); i++) 
    {
      NAString tempColName(tempColumns.getColumn(i)->getColName());
   

      ColReference *tempColRef = new(bindWA->wHeap()) 
	ColReference(new(bindWA->wHeap()) ColRefName(tempColName, tempCorrName));

      tempColRefList->insert(tempColRef);
    }

  RelExpr *delTemp = new (bindWA->wHeap()) LeafDelete(origTempCorrName, NULL, 
						      tempColRefList,FALSE);
  ((LeafDelete *)delTemp)->setTrigTemp(tempTableObj);
  ((GenericUpdate *)delTemp)->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;
 
  return new (bindWA->wHeap()) RelRoot(delTemp); 
 
  
}

// Here, we make virgin ColReferences, which when bound will be found
// in an outer scope engendered by our (and/or GenericUpdate's) interposing
// a RelRoot between us and our parent GenericUpdate --
// and the LeafXxx will add these outer refs to its characteristic inputs.
static RelExpr *createUndoNode(BindWA *bindWA, 
			     CorrName &tableCorrName,
			     const CorrName &indexCorrName,
			     IndexDesc *index,
			     NABoolean useInternalSyskey)
{
   
 // See createOldAndNewCorrelationNames() for info on OLDCorr/NEWCorr
  CorrName btCorrName = tableCorrName;
   
  tableCorrName.setCorrName( NEWCorr);
  
  
  ItemExprList *colRefList = new(bindWA->wHeap()) ItemExprList(bindWA->wHeap());
  
  const ValueIdList &indexColVids =  
    (index->isUniqueIndex()? index->getIndexColumns(): index->getIndexKey());
  
    

  
  for (CollIndex i=0; i < indexColVids.entries(); i++) {

    const NAString &colName = indexColVids[i].getNAColumn()->getColName();

    NAString realColName = colName;
    if (useInternalSyskey && colName == "SYSKEY")
    {
      realColName = "@SYSKEY";
    }

    ColReference *colRef =
      new (bindWA->wHeap()) ColReference
        (new (bindWA->wHeap()) ColRefName (realColName, tableCorrName, bindWA->wHeap()));

    colRefList->insert(colRef);
  }

  

  // NULL tableDesc here, like all Insert/Update/Delete ctors in SqlParser,
  // because the LeafXxx::bindNode will call GenericUpdate::bindNode
  // which will do the appropriate createTableDesc.
  GenericUpdate *delIndex; 
 
 
     
	
 
  if (index->isClusteringIndex())
    delIndex = new (bindWA->wHeap()) LeafDelete(btCorrName, NULL, 
						    colRefList);
  else
    delIndex = new (bindWA->wHeap()) LeafDelete(indexCorrName, NULL 
							,colRefList 
							,index->isUniqueIndex());
	
         
  // The base table's rowsAffected() will get set in ImplRule.cpp,
  // but we don't want any of these indexes' rowsAffected to be computed
  // (if I insert one row into a table, I want to see "1 row(s) inserted",
  // not 1 + number of indexes being maintained!).
  delIndex->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;

  // Do not collect STOI info for security checks.
  delIndex->getInliningInfo().setFlags(II_AvoidSecurityChecks);
  
  
  // Add a root here to prevent error 4056 when binding the LeafDelete+Insert
  // pair for an Update.
  return new (bindWA->wHeap()) RelRoot(delIndex); 	 
  
    
  return NULL;
} // static createUndoNode()

RelExpr *GenericUpdate::createUndoNodes(BindWA *bindWA,
				      NABoolean useInternalSyskey,
				      IndexDesc *index)
{
  // We call getExtFileSetObj (returns QualifiedName),
  // NOT getExtFileSetName (returns NAString),
  // hence the CorrName ctor sets up entire QualifiedName,
  // NOT just an (erroneously delimited) objectName part of one.
  //
  
  CorrName indexCorrName(index->getNAFileSet()->getExtFileSetObj());
  indexCorrName.setSpecialType(ExtendedQualName::INDEX_TABLE);

  CorrName &tableCorrName = getTableDesc()->getCorrNameObj();
  RelExpr *undoInsert = NULL;

  if (indexCorrName.getUgivenName().isNull())
    {
      indexCorrName.setUgivenName(tableCorrName.getUgivenName());
    }

  if(tableCorrName.isVolatile())
    indexCorrName.setIsVolatile(TRUE);

  // Create a list of base columns ColReferences for
  // ALL the index columns as AFTER/NEW columns.
  //
  
    undoInsert =  createUndoNode(bindWA, 
				 tableCorrName, 
				 indexCorrName,
				 index, 
				 useInternalSyskey);

 

    return undoInsert;
} // GenericUpdate::createUndoNodes()
/*****************************************************************************
******************************************************************************
****  Methods for handling undo from IUD log
******************************************************************************
*****************************************************************************/



RelExpr *GenericUpdate::createUndoIUDLog(BindWA *bindWA)
{
 						    
  MvIudLog logTableObj(this, bindWA);
  RelExpr *undoIUDNode = logTableObj.buildInsert(TRUE,
                                                 ChangesTable::ALL_ROWS,
                                                 TRUE,
                                                 TRUE);
  return undoIUDNode;
}

/*****************************************************************************
******************************************************************************
****  Methods for handling RI constraints
******************************************************************************
*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RefConstraintList *GenericUpdate::getRIs(BindWA *bindWA, 
					 const NATable *naTable)
{
  RefConstraintList *allRIConstraints = new(bindWA->wHeap()) 
    RefConstraintList(bindWA->wHeap());

  if ((getOperatorType() == REL_UNARY_INSERT) || 
      (getOperatorType()== REL_UNARY_UPDATE))
    naTable->getRefConstraints().getRefConstraints(bindWA, 
						   newRecExpr(), 
						   *allRIConstraints);
  
  if ((getOperatorType() == REL_UNARY_DELETE) || 
      (getOperatorType()== REL_UNARY_UPDATE) ||
      ((getOperatorType() == REL_UNARY_INSERT) && ((Insert *)this)->xformedEffUpsert()))
    naTable->getUniqueConstraints().getRefConstraints(bindWA, 
						      newRecExpr(), 
						      *allRIConstraints);

  CollIndex numConstraints = allRIConstraints->entries();

  for(CollIndex index = 0; index < numConstraints; index++)
  {
    if(NOT (allRIConstraints->at(index)->getIsEnforced()))
    {
	allRIConstraints->removeAt(index);
	numConstraints--;
	index--;
    }
  }

  if(allRIConstraints->isEmpty())
  {
    delete allRIConstraints;
    allRIConstraints = NULL;
  }

  return allRIConstraints;
}

//////////////////////////////////////////////////////////////////////////////
// Given the columns being updated, do we need to enforce this RI constraint?
// Check if there is any matching columns in both the RI key columns and
// in the set of columns being updated.
//////////////////////////////////////////////////////////////////////////////
NABoolean RefConstraint::isRINeededForUpdatedColumns(UpdateColumns *UpdatedColumns)
{
  // UpdatedColumns is NULL for INSERT and DELETE. Columns should be matched
  // for UPDATE operations only.

  if (NOT getIsEnforced())
    return FALSE;

  if (UpdatedColumns == NULL)
    return TRUE;

  NABoolean isAReferencingConstraint = isaForeignKeyinTableBeingUpdated() 
					&& getIsEnforced();
  const KeyColumns *riColumns;

  if (isAReferencingConstraint)
    riColumns = &keyColumns();
  else
    riColumns = uniqueConstraintReferencedByMe_.keyColumns_;

  for (CollIndex i=0; i<riColumns->entries(); i++)
  {
    if (UpdatedColumns->contains(riColumns->at(i)->getPosition()))
      return TRUE;
  }
  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Return a list of only the RI constraints needed when updating the
// specified columns.
//////////////////////////////////////////////////////////////////////////////
RefConstraintList *RefConstraintList::getNeededRIs(UpdateColumns *updatedColumns,
						   CollHeap *heap)
{
  // Insert into neededRIs only the constraints that match the columns
  // being updated.
  RefConstraintList *neededRIs = new(heap) RefConstraintList(heap);
  for (CollIndex i=0; i<entries(); i++)
    if (at(i)->isRINeededForUpdatedColumns(updatedColumns))
      neededRIs->insert(at(i));

  return neededRIs;
}

//////////////////////////////////////////////////////////////////////////////
// The tree below with a refConstraint after its bound.
//          TSJ
//         /  \
//   Insert   RelRoot
//     |         |
//   Tuple    GroupByAgg
//               |
//              Scan
//
// This function creates the rightSubtree of the TSJ node.
// Step 1. create the scan node with the predicate.
// Step 2. Create the GroupBy Node with the aggregate expression.
// Step 3. create the RelRoot node.
//////////////////////////////////////////////////////////////////////////////
RelExpr* GenericUpdate::createRISubtree(BindWA *bindWA, 
				        const NATable *naTable, 
				        const RefConstraint& refConstraint,
				        CollHeap *heap)
{
  RelExpr *newScan = NULL;
  NAString constraintName(bindWA->wHeap());
  NAString tableName(bindWA->wHeap());
  Parser parser(bindWA->currentCmpContext());

  NABoolean isReferencingConstraint = 
    (refConstraint.isaForeignKeyinTableBeingUpdated() &&
    refConstraint.getIsEnforced());
  
  // Step 1: Create a scan node with predicate given by the refConstraint.
  const QualifiedName parentQualName = refConstraint.getOtherTableName();
  
  NAString scanPredicateTxt;
  CorrName corrName(naTable->getTableName(), heap,
		    (isReferencingConstraint ? NEWAnsi : OLDAnsi));

  NAString corrNameString = corrName.getCorrNameAsString();
  refConstraint.getPredicateText(scanPredicateTxt, &corrNameString);
  
  constraintName = refConstraint.getConstraintName().getQualifiedNameAsAnsiString();
  tableName = naTable->getTableName().getQualifiedNameAsAnsiString();
  
  // Create the Scan node.
  newScan = new (heap) Scan (CorrName(parentQualName));
  ItemExpr *newScanPredicate = parser.getItemExprTree 
    ((char *)scanPredicateTxt.data());
  newScan->addSelPredTree(newScanPredicate);
  ((Scan *)newScan)->accessOptions().accessType() = TransMode::REPEATABLE_READ_ACCESS_;
  // Do not collect STOI info for security checks.
  newScan->getInliningInfo().setFlags(II_AvoidSecurityChecks);

  
  // Step 2: Create GroupBy Node.
  // Create a selection predicate and attach it to the
  // GroupBy  Node. in the 
  //                      case
  //                       |
  //                   IfThenElse
  //               ________|__________
  //              |        |          |
  //              |        |          |
  //              |        |          |
  //             OR      False    RaiseError
  //           /    \
  //          /      \
  //    OneTrue  (FK1 IS NULL
  //       |          OR     
  //       =      FK2 IS NULL..)
  //     /   \
  //    1     1
  
  ItemExpr *aggSelPredicate = parser.getItemExprTree ("1 = 1");

  ItemExpr *newAggExpr = NULL;
  
  // For inserts we have to see if a value exists in the referenced table
  // and for deletes we have to check that the deleted value does not exist
  // in the referencing table. Update is a delete and an insert, so we
  // have to check both.
  // You can tell whether to use "EXISTS" or "NOT EXISTS" by looking at
  // RefConstraint::isaForeignKeyinTableBeingUpdated
  
  if (isReferencingConstraint) 
  {
    // According to ANSI SQL99 (4.17.2), RI constraint is satisfied if one of the 
    // following conditions is true, depending on the <match option> specified in the
    // <referential constraint definition>:
    // If no < match type> was specified then, for each row R1 of the referencing
    // table, either at least one of the values of the referencing columns in 
    // R1 shall be a null value, or the value of each referencing column in R1 shall
    // be equal to the value of the corresponding referenced  column in
    // some row of the referenced table.

    // The MX 2.0 does not support <match type>, so it would be equivalent to
    // not specifying the <match type>.

    // This MatchOptionPredicate of the form (fk1 IS NULL or fk2 IS NULL)
    // is added to let the FKs with NULL values pass the RI constraint.

    // Also note that this MatchOptionPredicate is evaluated in the GroupBy
    // instead of Scan node. This is because when this predicate is added
    // to the row value constructor in the Scan node, the Optimizer fails to
    // recognise it as a key predicate, hence affecting performance.
    
    NAString matchOptionPredicateTxt;
    refConstraint.getMatchOptionPredicateText(matchOptionPredicateTxt, 
					      &corrNameString);
    
    ItemExpr *matchOptionPred = parser.getItemExprTree
      ((char *)matchOptionPredicateTxt.data());
    newAggExpr = new (heap) BiLogic(ITM_OR, 
				    new (heap) 
				    Aggregate(ITM_ONE_TRUE, aggSelPredicate),
				    matchOptionPred
				    );
    
  }
  else 
  {
    newAggExpr = new (heap) 
      UnLogic(ITM_NOT, new (heap) Aggregate(ITM_ONE_TRUE, aggSelPredicate));
  }
  
  ItemExpr *grbySelectionPred = new (heap)
    Case(NULL,
	 new (heap)
	   IfThenElse(newAggExpr,
		    new (heap) BoolVal(ITM_RETURN_FALSE),
		    new (heap) 
		      RaiseError((Lng32)EXE_RI_CONSTRAINT_VIOLATION, constraintName, 
			       tableName)));

  
  // Create a GroupBy on the newScan, and attach the new case as "having" predicate.
  RelExpr * newGrby = new(heap)
    GroupByAgg(newScan, REL_GROUPBY);
  newGrby->addSelPredTree(grbySelectionPred);
  
  // Create the Root Node.
  RelExpr *newRoot = new (heap) 
    RelRoot(newGrby,
	    TransMode::REPEATABLE_READ_ACCESS_,
	    SHARE_);

  ((RelRoot *)newRoot)->setEmptySelectList();

  return newRoot;
} // createRISubtree()

//////////////////////////////////////////////////////////////////////////////
// Given the ConstraintList, this function creates a RI subtree and returns
// the root of the subtree to the caller.
//////////////////////////////////////////////////////////////////////////////
RelExpr* GenericUpdate::inlineRI (BindWA *bindWA, 
				  const RefConstraintList *refConstraints,
				  CollHeap *heap)
{
  Int32 entries = 0;
  RelExpr *riSubtree = NULL;
  const NATable *naTable = getTableDesc()->getNATable();

  CMPASSERT (!refConstraints->isEmpty())
  
  if ((entries = refConstraints->entries())) 
  {
    riSubtree = createRISubtree(bindWA, naTable, *(refConstraints->at(0)), heap);
    for (Int32 i=1; i < entries; i++) 
    {
      riSubtree = new(heap) 
	Union(createRISubtree(bindWA, naTable, *(refConstraints->at(i)), heap),
	      riSubtree, NULL, NULL, REL_UNION, CmpCommon::statementHeap(), TRUE);
    } //end of for
  } 

  CMPASSERT(riSubtree);

  // Create the Root Node, only if the "riSubtree" is not a RelRoot.
  // "riSubtree" will be a relroot when there is only one RI constraint.
  // Avoding duplicate roots.
  if (riSubtree->getOperatorType() != REL_ROOT) {
    riSubtree = new (heap) 
      RelRoot(riSubtree,
              TransMode::REPEATABLE_READ_ACCESS_,
              SHARE_);
    ((RelRoot *)riSubtree)->setEmptySelectList();
  }

  riSubtree->getInliningInfo().setFlags(II_ActionOfRI);
  return riSubtree;
} // inlineRI.

/*****************************************************************************
******************************************************************************
****  Methods for handling MV Logging
******************************************************************************
*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// Logging should NOT be done in the following cases:
//    1. DELETE FROM MV
//    2. Update/Delete on INSERTLOG table.
//    3. NOLOG option specified
//    4. Pipelined refresh.
//    5. Recompute.
// In the first 3 cases, the table should be marked as inconsistent, and 
// getIsInsertLog() returns INCONSISTENT_NOLOG.
// In the cases of pipelined refresh and recompute, logging is 
// disabled, but the table is NOT marked as inconsistent (CONSISTENT_NOLOG).
// P.S. Marking the table as inconsistent is not implemented yet.
//////////////////////////////////////////////////////////////////////////////
NABoolean GenericUpdate::isMvLoggingRequired()
{
  const ComMvAttributeBitmap& bitmap = 
    getTableDesc()->getNATable()->getMvAttributeBitmap();

  if (!bitmap.getLoggingRequired())
  {
    // Just in case the user used NOLOG when logging is not required. We don't 
    // want to mark the table as inconsistent.
    isNoLogOperation_ = NORMAL_LOGGING; 
    return FALSE;
  }

#if 0
  // A set loggingRequired flag does not mean the MVs on this table
  // are initialized. If all the ON REQUEST MVs using this table
  // are not initialized, no logging is required.
  // This is commented out until we update the redefinition timestamp
  // of the base table when we initialize the MV.
  const UsingMvInfoList& mvsUsingMe = getTableDesc()->getNATable()->getMvsUsingMe();
  NABoolean foundInitializedMVs = FALSE;
  CollIndex maxEntries = mvsUsingMe.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    const UsingMvInfo* mv = mvsUsingMe[i];

    // Ignore MVs that are not ON REQUEST.
    if ( mv->getRefreshType() ==  COM_ON_REQUEST &&
         mv->isInitialized() )
    {
      // The MV is initialized. 
      // No need to continue the loop - logging is required.
      foundInitializedMVs = TRUE;
      break;
    }
  }
  if (foundInitializedMVs == FALSE)
  {
    // No initialized ON REQUEST MVs were found, so no reason to log.
    // Do not cache this query, to avoid breaking such statements
    // after the MV has been initialized.
    setNonCacheable();
    return FALSE;
  }
#endif

  // Check for Update/Delete on an INSERTLOG table
  if (getOperatorType() != REL_UNARY_INSERT &&
      bitmap.getIsInsertLog() )
  {
    setNoLogOperation(FALSE); // This is an inconsistent operation.
  }

  // Marking the table as inconsistent is not implemented yet,
  // therefore the INCONSISTENT_NOLOG mode is not considered logging.
  // When implemented, it will mean writing the CurrentEpoch to the UMD
  // table, and will be handled just like logging.
  return (isNoLogOperation() == NORMAL_LOGGING);
//return (isNoLogOperation() == NORMAL_LOGGING || 
//        isNoLogOperation() == INCONSISTENT_NOLOG);  
}

//////////////////////////////////////////////////////////////////////////////
// Add to the RETDesc the virtual columns needed for MV logging.
//////////////////////////////////////////////////////////////////////////////
void GenericUpdate::prepareForMvLogging(BindWA   *bindWA, 
					CollHeap *heap)
{
  // Create a "virtual column" called @CURRENT_EPOCH for the CurrentEpoch 
  // function. This function must be evaluated on the GU, and pipelined to
  // the Log Insert node.
  ItemExpr *currEpoch = new(heap) GenericUpdateOutputFunction(ITM_CURRENTEPOCH);
  ValueId epochId = 
    addVirtualColumn(bindWA, currEpoch, InliningInfo::getEpochVirtualColName(), heap);
  ValueId rowTypeId, rowCountId;
  if (getOperatorType() == REL_UNARY_INSERT &&
      getTableDesc()->getNATable()->getMvAttributeBitmap().getAutomaticRangeLoggingRequired())
  {
    // dead code, range logging is not supported
    ItemExpr *rowType  = new(heap) GenericUpdateOutputFunction(ITM_VSBBROWTYPE);
    ItemExpr *rowCount = new(heap) GenericUpdateOutputFunction(ITM_VSBBROWCOUNT);
    rowTypeId  = 
      addVirtualColumn(bindWA, rowType, InliningInfo::getRowTypeVirtualColName(), heap);
    rowCountId = 
      addVirtualColumn(bindWA, rowCount, InliningInfo::getRowCountVirtualColName(), heap);
  }

  ItemExpr *tsOutExpr = new (heap) 
                  GenericUpdateOutputFunction(ITM_JULIANTIMESTAMP, 
                                              1,
                                              new (heap) InternalTimestamp);

  ValueId tsId = addVirtualColumn(bindWA, 
                                  tsOutExpr, 
                                  InliningInfo::getMvLogTsColName(), 
                                  heap);


  ValueIdSet potentialOutputs;
  getPotentialOutputValues(potentialOutputs);
  potentialOutputs += epochId;
  potentialOutputs += tsOutExpr->getValueId();
  setPotentialOutputValues(potentialOutputs);
  getInliningInfo().setFlags(II_isMVLoggingInlined);

  // for push down
  if ( ((getOperatorType() == REL_UNARY_UPDATE)  && CmpCommon::getDefault(MV_LOG_PUSH_DOWN_DP2_UPDATE) == DF_ON) || 
       ((getOperatorType() == REL_UNARY_DELETE)  && CmpCommon::getDefault(MV_LOG_PUSH_DOWN_DP2_DELETE) == DF_ON) || 
       ((getOperatorType() == REL_UNARY_INSERT)  && CmpCommon::getDefault(MV_LOG_PUSH_DOWN_DP2_INSERT) == DF_ON) )
      getInliningInfo().setFlags(II_isUsedForMvLogging);
}

//////////////////////////////////////////////////////////////////////////////
// Insert the OLD and NEW values into the MV IUD Log
//////////////////////////////////////////////////////////////////////////////
RelExpr *GenericUpdate::createMvLogInsert(BindWA   *bindWA, 
					  CollHeap *heap,
					  UpdateColumns *updatedColumns,
					  NABoolean projectMidRangeRows)
{
  MvIudLog logTableObj(this, bindWA);
  logTableObj.setUpdatedColumns(updatedColumns);
  RelExpr *insertNode = logTableObj.buildInsert(TRUE,
                                                ChangesTable::ALL_ROWS,
                                                FALSE,
                                                TRUE);

  if (bindWA->errStatus())
    return NULL;

  prepareForMvLogging(bindWA, heap);

  if (projectMidRangeRows)
  {
    // Set the flag on this node.
    // This flag should be set even when range logging is off.
    getInliningInfo().setFlags(II_ProjectMidRangeRows);
  }

  RelExpr *topNode = insertNode;
  if (logTableObj.needsRangeLogging() && projectMidRangeRows)
  {
    // dead code, range logging is not supported
    RelRoot *rootNode = new (heap) RelRoot(insertNode);
    rootNode->setEmptySelectList();

    ItemExpr *noIgnoreCondition = new(heap)
      BiRelat(ITM_NOT_EQUAL,
	      new(heap) ConstValue(ComMvRowType_MidRange),
	      new(heap) ColReference(new(heap) 
		ColRefName(InliningInfo::getRowTypeVirtualColName())) );

    //  for the "else" of the 'when clause'
    ItemExpr *noOpArg  = new (heap) ConstValue(0);
    RelExpr  *noOp     = new (heap) Tuple(noOpArg);
    RelRoot  *noOpRoot = new (heap) RelRoot(noOp);
    noOpRoot->setEmptySelectList();
	      
    Union *ifNode = new(heap) Union
      (rootNode, noOpRoot, NULL, noIgnoreCondition, REL_UNION,
       CmpCommon::statementHeap(), TRUE);
    ifNode->setCondUnary();

    topNode = ifNode;
  }

  RelRoot *rootNode = new (heap) RelRoot(topNode);
  rootNode->setEmptySelectList();
  rootNode->getInliningInfo().setFlags(II_ActionOfRI); 

  return rootNode;
}

// This helper function is called only from GenericUpdate::handleInlining and GenericUpdate::getTriggeredMVs
// It checks if we are compiling a NOT ATOMIC statement as raises the appropriate
// error/warning. A warning is raised for ODBC and the statement will be compiled as an 
// ATOMIC statement. This method retuns TRUE if an error is raised and returns FALSE otherwise
NABoolean GenericUpdate::checkForNotAtomicStatement(BindWA *bindWA, Lng32 sqlcode, NAString objname, NAString tabname)
{
  if (bindWA->getHostArraysArea() &&
	    bindWA->getHostArraysArea()->getTolerateNonFatalError()) {
      
      if (CmpCommon::getDefault(ODBC_PROCESS) != DF_ON) {
	*CmpCommon::diags() << DgSqlCode(-sqlcode) 
	  << DgString0(objname) 
	  << DgString1(tabname);
	bindWA->setErrStatus();
	return TRUE;
      }
      else {
	bindWA->getHostArraysArea()->setTolerateNonFatalError(FALSE);
	setTolerateNonFatalError(RelExpr::UNSPECIFIED_);
	*CmpCommon::diags() << DgSqlCode(sqlcode)
	  << DgString0(objname) 
	  << DgString1(tabname);
      }

    }
  return FALSE ;
}
 
/*****************************************************************************
******************************************************************************
****  Methods for handling ON STATEMENT MVs
******************************************************************************
*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// This method inserts for each ON STATEMENT MV, that is affected by the
// action at the IUD node, a MVImmediate trigger(s) to the list of triggers to
// be fired on the subject table. These MVImmediate triggers are responsible for
// refreshing the MVs.
//
// The algorithm is as follows:
//
// 1. get list of MVs defined on the subject table
// 2. for each MV in the list do
//    2.1 ensure refresh type is "ON STATEMENT" - if not, skip MV
//    2.2 verify that the MV is initialized - if not, skip MV
//    2.3 add MV to the list of triggers (call insertMvToTriggerList)
//
// For now, only ON STATEMENT MJVs are supported!!!
//
//////////////////////////////////////////////////////////////////////////////
BeforeAndAfterTriggers *
GenericUpdate::getTriggeredMvs(BindWA                 *bindWA,
			       BeforeAndAfterTriggers *list,
			       UpdateColumns          *updatedColumns)
{
  CollHeap *heap = bindWA->wHeap();

  const NATable *subjectNA = tabId_->getNATable();
  CMPASSERT(subjectNA != NULL);

  const UsingMvInfoList &mvList = subjectNA->getMvsUsingMe();
  if (mvList.isEmpty())
  {
    // No MVs are to be refreshed - return the given list as is
    return list;
  }
   else {  //check for any non-atomic statements
     const PartitioningFunction *partFunc =
       subjectNA->getClusteringIndex()->getPartitioningFunction();


     for (CollIndex i = 0; i < mvList.entries(); i++)
       {
	 if ((CmpCommon::getDefault(NAR_DEPOBJ_ENABLE2) == DF_OFF) ||
	     ((partFunc->isARangePartitioningFunction())))

	   {
	     if (checkForNotAtomicStatement(bindWA,30033,
					    (mvList[i]->getMvName()).getQualifiedNameAsAnsiString(),
					    (subjectNA->getTableName()).getQualifiedNameAsAnsiString()))
	       {
		 return list;
	       } 
	   }
	
	 if (isNoRollback() ||
	     (CmpCommon::transMode()->getRollbackMode() == TransMode::NO_ROLLBACK_)) {
	   *CmpCommon::diags() << DgSqlCode(-3232) 
			       << DgString0((subjectNA->getTableName()).getQualifiedNameAsAnsiString()) 
			       << DgString1("Materialized View :")
			       << DgString2((mvList[i]->getMvName()).getQualifiedNameAsAnsiString());
	   bindWA->setErrStatus();

	   return list ;
	 }
       }
   }


  // If the given list of triggers is still empty, allocate a new one for the
  // new MVImmediate trigger(s) that might be added
  BeforeAndAfterTriggers *newList = list;
  if (newList == NULL)
  {
    newList = new(heap)
      BeforeAndAfterTriggers(NULL, NULL, NULL, subjectNA->getRedefTime());
  }

  // Search for MVs that should be refreshed
  for (CollIndex i = 0; i < mvList.entries(); i++)
  {
    // If MV is not ON STATEMENT - skip it
    if (mvList[i]->getRefreshType() != COM_ON_STATEMENT)
    {
      continue;
    }

    // If MV not intialized - skip it
    if (!mvList[i]->isInitialized())
    {
      continue;
    }

    // Retreive NATable of the MV. Return on error.
    CorrName mvCorr = CorrName(mvList[i]->getMvName(), heap);
    NATable *naTableMv = bindWA->getNATable(mvCorr);
    if (bindWA->errStatus())
    {
      return list;
    }

    // Retreive MVInfo for the MV. Return on error.
    MVInfoForDML *mvInfo = naTableMv->getMVInfo(bindWA);
    if (mvInfo == NULL)
    {
      return list;
    }

    // For now, only ON STATMENET MJVs are supported!
    if (mvInfo->getMVType() != COM_MJV)
    {
      continue;
    }

    if (bindWA->getTopRoot() != NULL)
      bindWA->getTopRoot()->setContainsOnStatementMV(TRUE);

    // The MV have passed all the general pre-conditions. Do some specific
    // checks and if it passes, add it to the triggers list.
    insertMvToTriggerList(newList,
			  bindWA,
			  heap,
			  mvList[i]->getMvName(),
			  mvInfo,
			  getTableName().getQualifiedNameObj(),
			  updatedColumns);
  }

  if (newList->entries() == 0)
  {
    //"newList" will be freed by statementHeap
    // add code annotation to prevent Coverity RESORUCE_LEAK checking error
    // coverity[leaked_storage]
    return NULL; // the list doesn't contain any triggers
  }

  // If there are only immediate MVs but not triggers, and this is an 
  // embedded IUD statement, abort with an error.
  if (list == NULL &&
      (getGroupAttr()->isEmbeddedUpdateOrDelete() || getGroupAttr()->isEmbeddedInsert()))
  {
    *CmpCommon::diags() << DgSqlCode(-12118);
    bindWA->setErrStatus();
    return NULL;
  }

  return newList; // return the updated list (including added MVs, if any)
}

//////////////////////////////////////////////////////////////////////////////
// This method does the actual insertion of the MVImmediate trigger to the
// list of triggers to be fired. Implemented only for derived classes!
//
//////////////////////////////////////////////////////////////////////////////

// we are not supposed to get here
void GenericUpdate::insertMvToTriggerList(BeforeAndAfterTriggers *list,
					  BindWA                 *bindWA,
					  CollHeap               *heap,
					  const QualifiedName    &mvName,
					  MVInfoForDML           *mvInfo,
					  const QualifiedName    &subjectTable,
					  UpdateColumns          *updateCols)
{
  CMPASSERT(false); // not implemented in GenericUpdate
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RelExpr *GenericUpdate::inlineOnlyRIandIMandMVLogging(BindWA *bindWA,
						      RelExpr *topNode,
						      NABoolean needIM,
						      RefConstraintList *riConstraints, 
						      NABoolean isMVLoggingRequired,
						      UpdateColumns *columns, 
						      CollHeap *heap)
{
  RelExpr *imTree = NULL;
  RelExpr *undoTree = NULL;
  RelExpr *riTree = NULL;
  RelExpr *mvTree = NULL;
  RelExpr *result = NULL;

  if (needIM || (riConstraints!=NULL) || isMVLoggingRequired)
  {
    if (topNode->getFirstNRows() >= 0)
	{
	  // create a firstN node to delete N rows.
	  FirstN * firstn = new(bindWA->wHeap())
	    FirstN(topNode, topNode->getFirstNRows(), FALSE /* No ordering requirement */);
	  firstn->bindNode(bindWA);
	  if (bindWA->errStatus())
	    return NULL;

	  topNode->setFirstNRows(-1);
	  topNode = firstn;
	}
  }
  // Create the tree that handles Index Maintainance.
  if (needIM)
    {
      // here we don't use the internal syskey column name ("@SYSKEY") since the
      // whole backbone is driven by the IUD node on the subject table
      imTree = createIMTree(bindWA, columns, FALSE);

    }
  

  // Create the tree that handles RI
  if (riConstraints!=NULL)
    riTree = inlineRI(bindWA, riConstraints, heap);

  // Create the tree for MV Logging.
  if (isMVLoggingRequired)
  {
    // When RI/IM/Triggers are not inlined, we can skip the projection of
    // the rows that are not Single/BeginRange/EndRange.
    NABoolean projectMidRangeRows = TRUE;
    if (imTree==NULL && riTree==NULL)
      projectMidRangeRows = FALSE;

    mvTree = createMvLogInsert(bindWA, heap, columns, projectMidRangeRows);
    if (mvTree != NULL)
    {
      // Use REL_SEMITSJ if it should not project any outputs
      // to the IM or RI trees.
      OperatorTypeEnum 
	joinType = (imTree || riTree) ? REL_LEFT_TSJ : REL_TSJ;
         NABoolean needsOutputs = isMtsStatement() ||
	  (getUpdateCKorUniqueIndexKey() && (getOperatorType() == REL_UNARY_DELETE));
	if (needsOutputs && joinType == REL_TSJ)
	  joinType = REL_ANTI_SEMITSJ;
//	joinType = (imTree || riTree) ? REL_ANTI_SEMITSJ : REL_SEMITSJ;
  
	Join *logTSJ = new(heap) Join (topNode, mvTree, joinType);
	logTSJ->getInliningInfo().setFlags(II_DrivingMvLogInsert);
	logTSJ->setTSJForWrite(TRUE);

        if ( ((getOperatorType() == REL_UNARY_UPDATE)  && CmpCommon::getDefault(MV_LOG_PUSH_DOWN_DP2_UPDATE) != DF_ON) ||
             ((getOperatorType() == REL_UNARY_DELETE)  && CmpCommon::getDefault(MV_LOG_PUSH_DOWN_DP2_DELETE) != DF_ON) ||
             ((getOperatorType() == REL_UNARY_INSERT)  && CmpCommon::getDefault(MV_LOG_PUSH_DOWN_DP2_INSERT) != DF_ON) )
           logTSJ->setAllowPushDown (FALSE);

	if (bindWA->getHostArraysArea() && bindWA->getHostArraysArea()->getTolerateNonFatalError())
	  logTSJ->setTolerateNonFatalError(RelExpr::NOT_ATOMIC_);
	topNode = logTSJ;
    }
  }

  result = imTree;
  if (riTree != NULL)
  {
    if (result == NULL)
    {
      result = riTree;
    }
    else
    {
      result = new (heap) Union(imTree, riTree, NULL, NULL, REL_UNION,
                                CmpCommon::statementHeap(), TRUE);
    }
  }
 
 // For NAR , generate an undo tree as well
  if (bindWA->getHostArraysArea() && bindWA->getHostArraysArea()->getTolerateNonFatalError() && (imTree || riTree)) 
    undoTree = createUndoTree(bindWA,columns,FALSE,(imTree||riTree),isMVLoggingRequired,NULL);
   if (undoTree && result)
    {
      OperatorTypeEnum joinType= REL_TSJ;
      Join * joinResultUndo = new (heap) Join(result,undoTree,joinType);      
      joinResultUndo->setTSJForWrite(TRUE);
      joinResultUndo->setTolerateNonFatalError(RelExpr::NOT_ATOMIC_);
      joinResultUndo->setTSJForUndo(TRUE); 
      result = joinResultUndo;
    }
  
  
   if (bindWA->isTrafLoadPrep())
     return result ;


  if (result!=NULL)
  {
    // This RelRoot opens a new BindScope, so that Union::bindChildern()
    // will not overwrite the RETDesc of the current scope with the NEW 
    // and OLD values.
    RelRoot *rootNode = new(heap) RelRoot(result);
    rootNode->setRootFlag(FALSE);
    rootNode->setEmptySelectList();

    OperatorTypeEnum joinOp;
    if ((topNode->child(0).getGroupAttr()->getEmbeddedIUD()) ||
	isMtsStatement() || // This is an embedded IUD statement 
	                   // (i.e. an IUD statement that has an outer select)
	(getUpdateCKorUniqueIndexKey() && (getOperatorType() == REL_UNARY_DELETE)))  
    {
      // originally index maintenance was using a TSJ joining the
      // tuples to be deleted from the base table with the tuples to
      // be deleted in the invidual indices. When returning tuples to
      // the user this causes the tuples to be deleted multiplied by
      // the number of indices. Therefore we now use an ANTI_SEMITSJ 
      // which does not require LeafDeletes to return tuples...i.e. it 
      // only return the leaf tuple if nothing is returned from the right 
      // child...this is what we want.
      joinOp = REL_ANTI_SEMITSJ;
    }
    else
    {
      joinOp = REL_TSJ;
    }

    Join *newTSJ = new(heap) Join (topNode, rootNode, joinOp);
    topNode = newTSJ;
  }
  
  ((Join *)topNode)->setTSJForWrite(TRUE);

  if (isMerge())
    {
      ((Join *)topNode)->setTSJForMerge(TRUE);

      if (isMergeUpdate())
	{
	  if (((MergeUpdate*)this)->insertValues())
	    ((Join *)topNode)->setTSJForMergeWithInsert(TRUE);
	}
      else
	{
	  if (((MergeDelete*)this)->insertValues())
	    ((Join *)topNode)->setTSJForMergeWithInsert(TRUE);
	}
    }

  if (bindWA->getHostArraysArea() && bindWA->getHostArraysArea()->getTolerateNonFatalError())
    { 
      ((Join *)topNode)->setTolerateNonFatalError(RelExpr::NOT_ATOMIC_);
      ((Join *)topNode)->setTSJForSetNFError(TRUE);
    }
  return topNode;
    
}

/*****************************************************************************
******************************************************************************
****  The "main" methods of inlining.
****  Build the trigger backbone and handle all the special cases.
******************************************************************************
*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RelExpr *GenericUpdate::inlineAfterOnlyBackbone(BindWA *bindWA, 
						TriggersTempTable& tempTableObj,
						TriggerList *rowTriggers,
						TriggerList *stmtTriggers,
						RefConstraintList *riConstraints,
						NABoolean needIM,
						NABoolean isMVLoggingRequired,
						UpdateColumns *updatedColumns,
						CollHeap *heap)
{





  RelExpr   *topNode = this;
  if (topNode->getFirstNRows() > 0)
  {
    // create a firstN node to delete N rows.
    FirstN * firstn = new(bindWA->wHeap())
      FirstN(topNode, topNode->getFirstNRows(), FALSE /* No ordering requirement */);
    firstn->bindNode(bindWA);
    if (bindWA->errStatus())
      return NULL;

    topNode->setFirstNRows(-1);
    topNode = firstn;
  }
  
  NABoolean noPipelinedActions = 
    (rowTriggers == NULL) && (riConstraints == NULL);

  NABoolean rowTriggersPresent = (rowTriggers != NULL);

  // First inline MV logging, so it will be pushed to DP2 with the IUD.
  if (isMVLoggingRequired)
  {
    RelExpr *mvTree = createMvLogInsert(bindWA, heap, updatedColumns, TRUE);
    if (mvTree != NULL)
    {
      Join *logTSJ = new(heap) Join(topNode, mvTree, REL_LEFT_TSJ);
      logTSJ->setTSJForWrite(TRUE);
      logTSJ->getInliningInfo().setFlags(II_DrivingMvLogInsert);
      if (bindWA->getHostArraysArea() && bindWA->getHostArraysArea()->getTolerateNonFatalError())
	logTSJ->setTolerateNonFatalError(RelExpr::NOT_ATOMIC_);
      topNode = logTSJ;
    }
  }

  // Next inline Index Maintainance
  if (needIM)
  {
    // here we don't use the internal syskey column name ("@SYSKEY") since the
    // whole backbone is driven by the IUD node on the subject table
    topNode = inlineIM(topNode, bindWA, FALSE, updatedColumns, heap, FALSE, rowTriggersPresent);
  }

  // Next inline the temp Insert
  topNode = inlineTempInsert(topNode,
			     bindWA,
			     tempTableObj,
			     FALSE,
			     noPipelinedActions,
			     heap);
  if (bindWA->errStatus()) 
    return NULL;

  Insert *pTempInsert = (Insert *)(RelExpr *)(topNode->child(1)->child(0));
  CMPASSERT(pTempInsert->getOperatorType() == REL_LEAF_INSERT ||
            pTempInsert->getOperatorType() == REL_UNARY_INSERT  );

  // Inline RI and row triggers.
  topNode = inlinePipelinedActions(topNode, bindWA, 
				   rowTriggers, 
				   riConstraints, 
				   heap);

  // Inline statement triggers
  topNode = inlineTriggerGroup(topNode, stmtTriggers, FALSE, heap, bindWA);

  // Inline the Temp delete.
  topNode = inlineTempDelete(bindWA, topNode, tempTableObj, heap);
  ((Union *)topNode)->setBlockedUnion();
  ((Union *)topNode)->setNoOutputs();
  if (isMtsStatement())
   ((Union *)topNode)->setIsTemporary();
  
  // If we are in an NAR, then set the InNotAtomicStatement flag
  if (bindWA->getHostArraysArea() && 
      bindWA->getHostArraysArea()->getTolerateNonFatalError()) 
    {
      ((Union *)topNode)->setInNotAtomicStatement();
    }

  if (bindWA->errStatus())
  {
     return this;
  }

  // Now bind the resulting tree
  topNode = topNode->bindNode(bindWA);
  if (bindWA->errStatus())
  {
    return this;
  }

  // store information for triggers transformation phase
  getInliningInfo().buildTriggerBindInfo(bindWA, getRETDesc(), heap);

  return topNode;
}




RelExpr *GenericUpdate::inlineAfterOnlyBackboneForUndo(BindWA *bindWA, 
						       TriggersTempTable&  tempTableObj,
						       TriggerList *rowTriggers,
						       TriggerList *stmtTriggers,
						       RefConstraintList *riConstraints,
						       NABoolean needIM,
						       NABoolean isMVLoggingRequired,
						       UpdateColumns *updatedColumns,
						       CollHeap *heap)
{
  RelExpr *imTree = NULL;
  RelExpr *undoTree = NULL;
  RelExpr *riTree = NULL;
  RelExpr *mvTree = NULL;
  RelExpr *result = NULL;
  RelExpr   *topNode = this;
  if (topNode->getFirstNRows() > 0)
    {
      // create a firstN node to delete N rows.
      FirstN * firstn = new(bindWA->wHeap())
	FirstN(topNode, topNode->getFirstNRows(), FALSE /* No ordering requirement */);
      firstn->bindNode(bindWA);
      if (bindWA->errStatus())
	return NULL;

      topNode->setFirstNRows(-1);
      topNode = firstn;
    }
  
  NABoolean noPipelinedActions = 
    (rowTriggers == NULL) && (riConstraints == NULL);

  NABoolean rowTriggersPresent = (rowTriggers != NULL);
 
  // create the temp insert tree here 
      
  RelExpr *tempInsertNode = tempTableObj.buildInsert(TRUE);
  RelRoot *tempInsRoot = new (heap) RelRoot(tempInsertNode);
  tempInsRoot->setRootFlag(FALSE);
  tempInsRoot->setEmptySelectList();

  if (isMVLoggingRequired)  
    mvTree = createMvLogInsert(bindWA, heap, updatedColumns, TRUE);
  
  
  if (mvTree != NULL)
    {
    
      RelRoot *mvRoot = new (heap) RelRoot(mvTree);	       
      if (mvRoot != NULL)
	{
	  Join *logTSJ = new(heap) Join(topNode, mvRoot, REL_LEFT_TSJ);     
	  logTSJ->setTSJForWrite(TRUE);
	  logTSJ->setTolerateNonFatalError(RelExpr::NOT_ATOMIC_);
          logTSJ->getInliningInfo().setFlags(II_DrivingMvLogInsert);
	  topNode = logTSJ;
	}      
    }

  if (tempInsRoot != NULL)
    {
      Join *logTSJ = new(heap) Join(topNode, tempInsRoot, REL_LEFT_TSJ);     
      logTSJ->setTSJForWrite(TRUE);
      logTSJ->setTolerateNonFatalError(RelExpr::NOT_ATOMIC_);
      topNode = logTSJ;
    }
    
  
    

  // Create the tree that handles Index Maintainance.
  if (needIM)
    {
      // here we don't use the internal syskey column name ("@SYSKEY") since the
      // whole backbone is driven by the IUD node on the subject table
      imTree = createIMTree(bindWA, updatedColumns, FALSE);

    }
  // Create the tree that handles RI
  if (riConstraints!=NULL)
    riTree = inlineRI(bindWA, riConstraints, heap);

  result = imTree;
  if (riTree != NULL)
    {
      if (result == NULL)
	{
	  result = riTree;
	}
      else
	{
	  result = new (heap) Union(imTree, riTree, NULL, NULL, REL_UNION,
				    CmpCommon::statementHeap(), TRUE);
	}
    }
  undoTree = createUndoTree(bindWA,updatedColumns,FALSE,(imTree||riTree),isMVLoggingRequired, &tempTableObj);
  if (undoTree && result)
    {
      OperatorTypeEnum joinType= REL_TSJ;
      Join * joinResultUndo = new (heap) Join(result,undoTree,joinType);
      joinResultUndo->setTSJForWrite(TRUE);
      joinResultUndo->setTolerateNonFatalError(RelExpr::NOT_ATOMIC_);
      joinResultUndo->setTSJForUndo(TRUE); 
      result = joinResultUndo;
	 
    }

 

  if (result!=NULL)
    {
      // This RelRoot opens a new BindScope, so that Union::bindChildern()
      // will not overwrite the RETDesc of the current scope with the NEW 
      // and OLD values.
      RelRoot *rootNode = new(heap) RelRoot(result);
      rootNode->setRootFlag(FALSE);
      rootNode->setEmptySelectList();

      OperatorTypeEnum joinOp;
      if ((topNode->child(0).getGroupAttr()->getEmbeddedIUD()) ||
	  isMtsStatement())  // This is an embedded IUD statement 
	// (i.e. an IUD statement that has an outer select)
	{
	  // originally index maintenance was using a TSJ joining the
	  // tuples to be deleted from the base table with the tuples to
	  // be deleted in the invidual indices. When returning tuples to
	  // the user this causes the tuples to be deleted multiplied by
	  // the number of indices. Therefore we now use an ANTI_SEMITSJ 
	  // which does not require LeafDeletes to return tuples...i.e. it 
	  // only return the leaf tuple if nothing is returned from the right 
	  // child...this is what we want.
	  joinOp = REL_ANTI_SEMITSJ;
	}
      else
	{
	  joinOp = REL_TSJ;
	}

      Join *newTSJ = new(heap) Join (topNode, rootNode, joinOp);
      newTSJ->setTSJForWrite(TRUE);
	
      newTSJ->setTolerateNonFatalError(RelExpr::NOT_ATOMIC_);
      newTSJ->setTSJForSetNFError(TRUE);      
      RelRoot *rootNode2 = new(heap) RelRoot(newTSJ);
      rootNode2->setRootFlag(FALSE);
      rootNode2->setEmptySelectList();
	  
      topNode = rootNode2;
    }
    // Inline statement triggers
     topNode = inlineTriggerGroup(topNode, stmtTriggers, FALSE, heap, bindWA);

    // Inline the Temp delete.
     topNode = inlineTempDelete(bindWA, topNode, tempTableObj, heap);
    ((Union *)topNode)->setBlockedUnion();
    ((Union *)topNode)->setNoOutputs();
    if (isMtsStatement())
      ((Union *)topNode)->setIsTemporary();

    // If we are in an NAR, then set the InNotAtomicStatement flag
    if (bindWA->getHostArraysArea() && 
        bindWA->getHostArraysArea()->getTolerateNonFatalError()) 
      {
	((Union *)topNode)->setInNotAtomicStatement();
      }

    if (bindWA->errStatus())
      {
	return this;
      }

    // Now bind the resulting tree
    topNode = topNode->bindNode(bindWA);
    if (bindWA->errStatus())
      {
	return this;
      }

    // store information for triggers transformation phase
    getInliningInfo().buildTriggerBindInfo(bindWA, getRETDesc(), heap);
 
    return topNode;
   
}
//////////////////////////////////////////////////////////////////////////////
// Remove from the inputs of 'node' any ValueIds that do not reference 
// any of the values in realInputs.
//////////////////////////////////////////////////////////////////////////////
static void minimizeInputsForNode(RelExpr *node, ValueIdSet &realInputs)
{
  ValueIdSet inputsOfNode(node->getGroupAttr()->getCharacteristicInputs());
  realInputs.weedOutUnreferenced(inputsOfNode);
  node->getGroupAttr()->setCharacteristicInputs(inputsOfNode);
}

//////////////////////////////////////////////////////////////////////////////
// For each and every node in 'subtree' call minimizeInputsForNode()
// to remove the redundant inputs.
//////////////////////////////////////////////////////////////////////////////
static void minimizeInputsForSubtree(RelExpr *subtree, ValueIdSet &realInputs)
{
  minimizeInputsForNode(subtree, realInputs);

  for (Int32 i=0; i<subtree->getArity(); i++)
    minimizeInputsForSubtree(subtree->child(i), realInputs);
}

//////////////////////////////////////////////////////////////////////////////
// The binding process adds redundant inputs to the temp insert subtree.
// This usually works out in the transformation and normalization, but
// when the triggering action is an Update with sub-queries - it does not.
// The real inputs for the temp insert subtree, are the OLD values, plus 
// the UniqueExecuteId. Remove from thecharacteristic inputs of every node
// of the temp insert subtree, all the values that do not reference the
// real inputs as defined above.
//////////////////////////////////////////////////////////////////////////////
void GenericUpdate::removeRedundantInputsFromTempInsertTree(BindWA  *bindWA, 
							    RelExpr *tentativeSubtree)
{
  // The OLD values are the outputs of the Scan node below the triggering IUD 
  // node (this node).
  Scan *scanNode = getScanNode();
  CMPASSERT(scanNode != NULL);
  ValueIdSet realInputs(scanNode->getGroupAttr()->getCharacteristicOutputs());

  // All instantiations of the UniqueExcuteId funcction have the same ValueId.
  // Once the transformation code changes are merged in, we can take the ExecId
  // ValueId from the BindInfo.
  ItemExpr *execId = new(bindWA->wHeap()) UniqueExecuteId();
  execId->bindNode(bindWA);
  realInputs += execId->getValueId();

  // Find the top node of the temp insert subtree.(through the RelRoot and TSJ).
  RelExpr *tempInsertNode = tentativeSubtree->child(0)->child(1);
  CMPASSERT(tempInsertNode->getOperatorType() == REL_ROOT);
  CMPASSERT(tempInsertNode->child(0)->getOperatorType() == REL_UNARY_INSERT);

  // Now remove the redundant inputs.
  minimizeInputsForSubtree(tempInsertNode, realInputs);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RelExpr *GenericUpdate::inlineBeforeAndAfterBackbone(BindWA *bindWA, 
						     RelExpr *tentativeSubtree,
						     TriggersTempTable& tempTableObj,
						     TriggerList *rowTriggers,
						     TriggerList *stmtTriggers,
						     RefConstraintList *riConstraints,
						     NABoolean needIM,
						     NABoolean isMVLoggingRequired,
						     UpdateColumns *updatedColumns,
						     CollHeap *heap)
{
  // Create the effective Insert, Update or Delete node.
  GenericUpdate *effectiveGuNode = NULL;
  RelExpr *effectiveGuRootNode = 
    createEffectiveGU(bindWA, heap, tempTableObj, &effectiveGuNode, updatedColumns);

  CMPASSERT(effectiveGuNode != NULL);

  RelExpr *topNode = effectiveGuRootNode;
  // used by the generator to indicate that this plan has triggers so
  // if the trigger is dropped the plan is recompiled
  topNode->getInliningInfo().setFlags(II_hasTriggers);

  // We only use the FiringTriggers flag if this GU will have to generate
  // the NEW and OLD values for Row triggers, RI or IM.
  if (getInliningInfo().hasPipelinedActions())
  {
    effectiveGuNode->getInliningInfo().setFlags(II_hasPipelinedActions); 
  }

  // First inline MV logging, so it will be pushed to DP2 with the IUD.
  if (isMVLoggingRequired)
  {
    RelExpr *mvTree = createMvLogInsert(bindWA, heap, updatedColumns, TRUE);
    if (mvTree != NULL)
    {
      OperatorTypeEnum joinType = REL_LEFT_TSJ;
      if (!rowTriggers && !riConstraints && !needIM)
	joinType = REL_TSJ;
      Join *logTSJ = new(heap) Join(topNode, mvTree, joinType);
      logTSJ->setTSJForWrite(TRUE);

      // disable parallele execution for TSJs that control row triggers
      // execution. Parallel execution for triggers TSJ introduces the
      // potential for non-deterministic execution
      if (rowTriggers)
        logTSJ->getInliningInfo().setFlags(II_SingleExecutionForTriggersTSJ);

      topNode = logTSJ;
    }
  }

  if (needIM)
  {
    // Next inline Index Maintainance
    NABoolean imIsLastTSJ = ((rowTriggers==NULL) && (riConstraints==NULL));
    // here we do use the internal syskey column name ("@SYSKEY") since the
    // whole backbone is driven by the temp-table insert node

    NABoolean rowTriggersPresent = (rowTriggers != NULL);

    if (getOperatorType() == REL_UNARY_INSERT)
    {
       topNode = inlineIM(topNode, bindWA, imIsLastTSJ, updatedColumns, 
                          heap, FALSE, rowTriggersPresent);
    }
    else  // REL_UNARY_UPDATE or REL_UNARY_DELETE
    { 
       topNode = inlineIM(topNode, bindWA, imIsLastTSJ, updatedColumns, 
                          heap, TRUE, rowTriggersPresent);
    }
  }

  // Next inline RI and row triggers (temp Insert is on the tentative side).
  topNode = inlinePipelinedActions(topNode,
                                   bindWA, 
				   rowTriggers, 
				   riConstraints, 
				   heap);

  // Inline statement triggers
  topNode = inlineTriggerGroup(topNode, stmtTriggers, FALSE, heap, bindWA);

  // Inline the Temp delete.
  topNode = inlineTempDelete(bindWA, topNode, tempTableObj, heap);
  topNode->getInliningInfo().setFlags(II_BeforeTriggersExist);

  // Open a new scope for the after-trigger part.
  topNode = new(heap) RelRoot(topNode); 
  ((RelRoot *)topNode)->setRootFlag(FALSE);

  // Join the two parts of the backbone using a blocked Union node.
  Union *topUnion = new(heap) Union(tentativeSubtree, topNode, NULL, NULL,
                                    REL_UNION, CmpCommon::statementHeap(),
                                    TRUE);
  topUnion->setBlockedUnion();
  topUnion->setNoOutputs();
  topUnion->getInliningInfo().setFlags(II_DrivingBeforeTriggers);
 
  // If we are in an NAR, then set the InNotAtomicStatement flag
  if (bindWA->getHostArraysArea() && 
      bindWA->getHostArraysArea()->getTolerateNonFatalError()) 
    {
	topUnion->setInNotAtomicStatement();
    }
  
  topNode = topUnion;

  // Now bind the resulting tree
  topNode = topNode->bindNode(bindWA);
  if (bindWA->errStatus())
    return this;

  // store information for triggers transformation phase
  InliningInfo &info = effectiveGuNode->getInliningInfo();

  if (!(info.hasPipelinedActions()) && getOperatorType() != REL_UNARY_UPDATE)
  {
     // case of before triggers and after statement triggers where the after triggers are in conflict
     // and the effective GU is either an insert or a delete.
     info.getTriggerBindInfo()->setBackboneIudNum(bindWA->getUniqueIudNum());
  }
  else
  {
     info.buildTriggerBindInfo(bindWA, effectiveGuRootNode->getRETDesc(), heap);
  }


  // The binding process adds redundant inputs to the temp insert subtree.
  // This usually works out in the transformation and normalization, but
  // when the triggering action is an Update with sub-queries - it does not.
  if (getOperatorType() == REL_UNARY_UPDATE)
    removeRedundantInputsFromTempInsertTree(bindWA, tentativeSubtree);

  return topNode;
}

//////////////////////////////////////////////////////////////////////////////
// Is this backbone cascaded from a row trigger?
//////////////////////////////////////////////////////////////////////////////
NABoolean GenericUpdate::shouldForbidMaterializeNodeHere(BindWA *bindWA)
{
  // If this backbone is cascaded from a row after trigger, Do not allow the
  // optimizer to use any Materialize nodes below this point.
  BindScope *triggerScope = NULL;
  // Skip this if the flag was already set by a trigger above us.
  if (!getInliningInfo().isMaterializeNodeForbidden())
  {
    // For each scope above us, that has a trigger object
    while ((triggerScope = bindWA->findNextScopeWithTriggerInfo(triggerScope))
           != NULL)
    {
      StmtDDLCreateTrigger* triggerObj = triggerScope->context()->triggerObj();
      // If its not a row after trigger - continue searching.
      if (triggerObj->isStatement() || !triggerObj->isAfter())
	continue;

      return TRUE;
    }
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void GenericUpdate::InliningFinale(BindWA *bindWA, RelExpr *topNode, 
                                   RETDesc *origRETDesc)
{
  // QSTUFF
  // this expression is executed once all inlining and binding 
  // for index maintenance, RI and triggers has been done. We replace the 
  // current scope with a RETDesc containing references to old and new column
  // values as can be referenced in the return clause of an embedded delete or
  // and embedded update 
  
  if (getGroupAttr()->isEmbeddedUpdateOrDelete() ||
      isMtsStatement() ||
      (getUpdateCKorUniqueIndexKey() && (getOperatorType() == REL_UNARY_DELETE)))
    {
      
      // lets only return the implicit old and new table columns
      if (!getRETDesc()) delete getRETDesc();
      
      CorrName corrNEWTable
	(getTableDesc()->getCorrNameObj().getQualifiedNameObj(), 
	 bindWA->wHeap(),NEWTable);
      
      if (getOperatorType() == REL_UNARY_UPDATE  ||
	  getOperatorType() == REL_UNARY_INSERT){
	// expose NEW table columns
	setRETDesc(new (bindWA->wHeap()) 
		   RETDesc(bindWA,getTableDesc(),&corrNEWTable));
      }
      else
	setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA));
      
      if (getOperatorType() == REL_UNARY_UPDATE ||
	  getOperatorType() == REL_UNARY_DELETE)
	{
	  CorrName corrOLDTable
	    (getScanNode(TRUE)->getTableDesc()->getCorrNameObj().getQualifiedNameObj(), 
	     bindWA->wHeap(),OLDTable
	     );
	  
	  // expose OLD table columns
	  getRETDesc()->
	    addColumns(bindWA, *getScanNode(TRUE)->getRETDesc(), &corrOLDTable);
	  
	}
      
      // allow the TSJRule to be used to transform Updates/Deletes
      setNoFlow(TRUE);
      
      // record the GenericUpdateRoot property in the group
      // attributes of the root of the generic update tree.
      // this is used by pushdowncovered expression to prevent
      // expression to be push beyond the root of a generic update
      // tree
      topNode->getGroupAttr()->setGenericUpdateRoot(TRUE);
      
      // set current scope to contain NEW and OLD tables only               
      origRETDesc = getRETDesc();       
    }
  
  if (bindWA->inDDL())
    return;
  
  //QSTUFF

  ValueIdList outputs;
  getRETDesc()->getValueIdList(outputs, USER_AND_SYSTEM_COLUMNS);
  addPotentialOutputValues(outputs);

  // If ever extend the SQL syntax to such non-Ansi constructs as
  // INSERT INTO TI (DELETE FROM TD ... );
  //		SELECT * FROM (UPDATE TU SET ... ) X;
  // then we'll want to revisit these next two lines,
  // (resetting our RETDesc to an empty one) because otherwise
  // this nonempty one'll become our parent RelRoot's compExpr(),
  // which will cause 
  // 		RelRoot::preCodeGen - compExpr().replaceVEGExpressions - 
  //		VEGReference::replaceVEGReference
  // to assert with "no available values hence valuesToBeBound.isEmpty".
  //
  // QSTUFF
  // please see above..we made the extensions and did fixed whats 
  // referred to above
  // select * from (delete from x)y, z where y.x = z.x;
  // select * from (update x set x = x + 1) y, z where y.x = z.x;
  if (!getGroupAttr()->isEmbeddedUpdateOrDelete() &&
      !isMtsStatement() &&
      !(getUpdateCKorUniqueIndexKey() && (getOperatorType() == REL_UNARY_DELETE)))
  {
    delete getRETDesc();
    setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA));
  }

    // QSTUFF
  topNode->setRETDesc(getRETDesc());
  bindWA->getCurrentScope()->setRETDesc(origRETDesc);
}

// Helper function used to check the existence of at least one update
// trigger whose explicit columns match at least one of the columns
// supplied as a second parameter
NABoolean atLeastOneMatch(const BeforeAndAfterTriggers *allTriggers,
                          const UpdateColumns *columns)
{
    TriggerList *trigs = NULL;

    assert(allTriggers != NULL);

    // check the before row triggers for a match
    if (allTriggers->getBeforeTriggers() != NULL)
    {
       trigs = 
         allTriggers->getBeforeTriggers()->getColumnMatchingTriggers (columns);
    }

    // check the after row triggers for a match
    if ((trigs == NULL) && (allTriggers->getAfterRowTriggers() != NULL))
    {
       trigs = 
        allTriggers->getAfterRowTriggers()->getColumnMatchingTriggers (columns);
    }

    // check the after statement triggers for a match
    if ((trigs == NULL) && (allTriggers->getAfterStatementTriggers() != NULL))
    {
       trigs = 
        allTriggers->getAfterStatementTriggers()->getColumnMatchingTriggers (columns);
    } 

    // at least one match was found
    if (trigs != NULL)
    {
       return TRUE;
    }

    // no match
    return FALSE;
}

NABoolean GenericUpdate::checkNonSupportedTriggersUse(BindWA *bindWA, 
                           QualifiedName &subjectTable,
                           ComOperation op,
                           BeforeAndAfterTriggers *allTriggers)
{
  CollHeap *heap = bindWA->wHeap();

  // disable embedded update and delete as trigger events
  if ((allTriggers != NULL) && (getGroupAttr()->isEmbeddedUpdateOrDelete()))
  {
       // for update operations we have to make sure that there is at least
       // one trigger defined on the column updated by the operation
       if (op == COM_UPDATE)
       {
           // Get the list of updated columns.
           UpdateColumns *columns = new(heap) UpdateColumns(stoi_->getStoi());

           // find at least one trigger that is fired by the columns updated
           // by this operation 
           if (atLeastOneMatch(allTriggers, columns))
           {
               *CmpCommon::diags() << DgSqlCode(-11027);
               bindWA->setErrStatus();
               // "columns" will be freed by statementHeap. 
               // add code annotation to prevent Coverity checking error
               // coverity[leaked_storage]
               return TRUE;
           }
                 
       }
       else // delete - all delete triggers are considered
       {
           *CmpCommon::diags() << DgSqlCode(-11027);
           bindWA->setErrStatus();
           return TRUE;
       }

  }

  // disable embedded insert as trigger events
  if ((allTriggers != NULL) && (getGroupAttr()->isEmbeddedInsert()))
  {
    *CmpCommon::diags() << DgSqlCode(-11027);
    bindWA->setErrStatus();
    return TRUE;
  }

  // the set clause of SET ON ROLLBACK statements may not change
  // columns on which update triggers are defined 
  if (newRecBeforeExpr() != NULL)
  {
        BeforeAndAfterTriggers *allTriggers2 = allTriggers;

        // for SET ON ROLLBACK delete statements, we are not interested
        // in the triggers fired by the delete operation but rather by the
        // triggers defined on the columns updated by the SET ON ROLLBACK
        // clause. These triggers are update triggers not delete triggers.
        if (op == COM_DELETE)
        {
           ComOperation op2 = COM_UPDATE;
           allTriggers2 =
               bindWA->getSchemaDB()->getTriggerDB()->getTriggers(subjectTable,
                                         op2, bindWA);
           if (bindWA->errStatus())
             return TRUE;   
        }

        if (allTriggers2 != NULL)
        {
            ValueId exprId;
            UpdateColumns *columns = 
                        new(heap) UpdateColumns((SqlTableOpenInfo *)NULL);
            for (exprId = newRecBeforeExpr().init(); 
               newRecBeforeExpr().next(exprId); 
               newRecBeforeExpr().advance(exprId))
            {
                 ItemExpr *thisIE = exprId.getItemExpr();
                 columns->addColumn((thisIE->child(0).getNAColumn())->getPosition());
            }

           // if at least one of the columns updated in SET ON ROLLBACK clause 
           // of the SET ON ROLLBACK statement is a subject column of an update
           //  trigger, raise an error message
           if (atLeastOneMatch(allTriggers2, columns))
           {
               *CmpCommon::diags() << DgSqlCode(-11026);
               bindWA->setErrStatus();
               // "columns" will be freed by statementHeap. 
               // add code annotation to prevent Coverity checking error
               // coverity[leaked_storage]
               return TRUE;   
           }
       }
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Handle the inlining of Triggers, RI, IM, MV logging and ON STATEMENT MVs.
// This method is called from the end of the bindNode() methods of Insert,
// Update and Delete.
// The trigger backbone is different if before triggers exist.
// Please read the Triggers internal documentation before trying to understand
// this code.
//////////////////////////////////////////////////////////////////////////////
RelExpr * GenericUpdate::handleInlining(BindWA *bindWA, RelExpr *boundExpr)
{
  RETDesc  *origScopeRETDesc = bindWA->getCurrentScope()->getRETDesc();
  CorrName &subjectTableCorr = getTableDesc()->getCorrNameObj();

  if (bindWA->inDDL())
  {
    // some QSTUFF code in inlineingFinale() should be executed when we
    // are in create view statement.
    InliningFinale(bindWA, boundExpr, origScopeRETDesc);
    return boundExpr;
  }

  // MultiCommit is currently only valid for DELETE statements
  if (NOT getOperator().match(REL_ANY_DELETE)
      &&
      CmpCommon::transMode()->getMultiCommit() == TransMode::MC_ON_)
    {
      *CmpCommon::diags() << DgSqlCode(-4351);	 
      bindWA->setErrStatus();  
      return boundExpr;
    }
  
  // We don't do views.
  // ignore location specified operations.
  if ( (getTableDesc()->getNATable()->getViewText() != NULL) ||
       (subjectTableCorr.isLocationNameSpecified() ))
    return boundExpr;

  if (subjectTableCorr.getSpecialType() == ExtendedQualName::SG_TABLE)
    {
      InliningFinale(bindWA, boundExpr, origScopeRETDesc);
      return boundExpr;  // Nothing for us to do here.
    }

  // A "DELETE [FIRST n] FROM <IUD-log-table>"
  // is the only case we allow a special table through here.
  NABoolean firstN_OnIudLogTable = FALSE;
  if ((subjectTableCorr.getSpecialType() == ExtendedQualName::IUD_LOG_TABLE) &&
      (getFirstNRows() > 0) )
    firstN_OnIudLogTable = TRUE;

  // Don't waste time on special tables like index etc.
  // The IUD log is allowed here because we allow delete with multi commit on it.
  if ((subjectTableCorr.getSpecialType() != ExtendedQualName::NORMAL_TABLE) &&
      (subjectTableCorr.getSpecialType() != ExtendedQualName::MV_TABLE)     &&
      !firstN_OnIudLogTable                                                 &&
      !getInliningInfo().isNeedGuOutputs() )
    return boundExpr;

  // no inlining for the effective GU of a before trigger
  if (getInliningInfo().isEffectiveGU())
    return bindEffectiveGU(bindWA);

  if (getInliningInfo().isNeedGuOutputs())
  {
    // The OLD/NEW outputs of this GU node are needed for some purpose
    // other than triggers/RI/IM etc.
    createOldAndNewCorrelationNames(bindWA);

    ValueIdList outputs;
    getRETDesc()->getValueIdList(outputs, USER_AND_SYSTEM_COLUMNS);
    setPotentialOutputValues(outputs);

    setNoFlow(TRUE);

    return boundExpr;
  }

  // If code future changes cause this assertion to fail, we need to make
  // sure our code still works.
  CMPASSERT(boundExpr == this);

  ComOperation op;
  switch (getOperatorType()) 
  {
    case REL_UNARY_INSERT: op = COM_INSERT; break;
    case REL_UNARY_UPDATE: op = COM_UPDATE; break;
    case REL_UNARY_DELETE: op = COM_DELETE; break;
    default : return boundExpr;  // We only handle these three operators.
  }

  CollHeap *heap = bindWA->wHeap();
  QualifiedName& subjectTable = subjectTableCorr.getQualifiedNameObj();

  // get all triggers in the triggerDB, and RIs from the SchemaDB.
#if DISABLE_TRIGGERS
  BeforeAndAfterTriggers *allTriggers = NULL;
#else
  BeforeAndAfterTriggers *allTriggers = 0;

     if ((isIgnoreTriggers() == FALSE) && !firstN_OnIudLogTable )
  {
    if (getUpdateCKorUniqueIndexKey())
    {
      // if this the delete node of updateCKorUniqueIndexKey then skip
      // inlining triggers altogether (allTriggers remains NULL).
      if (op == COM_INSERT)
      {
	op = COM_UPDATE;
	allTriggers = bindWA->getSchemaDB()->getTriggerDB()->getTriggers(subjectTable, op, bindWA);
      }
    }
    else
      {
	allTriggers = bindWA->getSchemaDB()->getTriggerDB()->getTriggers(subjectTable, op, bindWA);

	if ((allTriggers == NULL) && (isMerge()))
	  {
	    // Triggers are not supported with Merge statement.
	    // if update part of merge didn't cause any triggers to be inlined,
	    // check for insert triggers.
	    // These triggers will be inlined here but an error will be
	    // returned during preCodeGen.
	    allTriggers = bindWA->getSchemaDB()->getTriggerDB()->getTriggers(
		 subjectTable, COM_INSERT, bindWA);
	  }
      }
  }
#endif

  if (bindWA->errStatus())
     return NULL;

  if (allTriggers) {
    getInliningInfo().setFlags(II_hasTriggers);
    bindWA->setInTrigger();
  }

  // certain triggers uses are currently not supported and errors
  // are raised if these uses are tried
  if (checkNonSupportedTriggersUse(bindWA, subjectTable, op, allTriggers))
  {
     return this;
  }

#if DISABLE_RI
  RefConstraintList *riConstraints = NULL; 
#else
  RefConstraintList *riConstraints = 
    getRIs(bindWA, getTableDesc()->getNATable());
#endif

  if (riConstraints) 
    getInliningInfo().setFlags(II_hasRI);

#if DISABLE_MV_LOGGING
  NABoolean isMVLoggingRequired = FALSE;
#else
  NABoolean isMVLoggingRequired = isMvLoggingRequired();
#endif

  RelExpr  *topNode=boundExpr;

  // Get the list of updated columns.
  UpdateColumns *columns = NULL;
  if (op == COM_UPDATE)
    columns = new(heap) UpdateColumns(stoi_->getStoi());

  // Get only the before triggers that match these columns.
  TriggerList *beforeTriggers = NULL;
  if ((allTriggers != NULL) && (allTriggers->getBeforeTriggers() != NULL))
    beforeTriggers = allTriggers->getBeforeTriggers()->getColumnMatchingTriggers(columns);

  NABoolean tsjRETDescCreated = FALSE;

  if (beforeTriggers != NULL) {   
    if (checkForNotAtomicStatement(bindWA, 30027, 
				  ((*beforeTriggers)[0])->getTriggerName(), 
				  subjectTable.getQualifiedNameAsAnsiString())) {
	return this;
    }

    tsjRETDescCreated = TRUE;
    createOldAndNewCorrelationNames(bindWA);
  }

  // Save the previous IudNum, it will be restored at the end of the 
  // inlining of the current backbone.
  Lng32 prevIudNum = bindWA->getUniqueIudNum();
  // Set the IudNum for the current generic update backbone.
  // This value is used later as part of the uniquifier
  // as the temporary table column UNIQUEIUD_COLUMN
  bindWA->setUniqueIudNum();

  // When triggers defined on the subject table, it is assured that the
  // temp-table exists for this table. Since during initialization of the
  // TriggersTempTable object there's a seek for the temp-table's NATable, we
  // must be sure this table actually exists before creating that object.

  TriggersTempTable *tempTableObj = NULL;
  if (allTriggers != NULL)
  {
    tempTableObj = new(heap) TriggersTempTable(this, bindWA);
  }

  // Build the before triggers side of the inlining backbone.
  // This method also adds to 'columns' any column that is updated by before triggers.
  RelExpr  *beforeSubtree = NULL;
  if (beforeTriggers != NULL) 
    beforeSubtree = createTentativeSubTree(bindWA, 
					   beforeTriggers, 
					   columns,	
					   *tempTableObj, 
					   heap);

  if (bindWA->errStatus()) 
    return NULL;

#if DISABLE_TRIGGERS
  // Nothing to do.
#else
  if ((allTriggers != NULL) && (allTriggers->getAfterStatementTriggers() != NULL))
    {
    TriggerList *pureStmtTriggers = allTriggers->getAfterStatementTriggers()->getColumnMatchingTriggers(columns);
    // There are statement triggers which are not for statement MVs.
    // The ones used for statement MVs will be pupulated with the
    // getTriggeredMVs call below.
    if (pureStmtTriggers)
      if (checkForNotAtomicStatement(bindWA, 30034, 
				     ((*pureStmtTriggers)[0])->getTriggerName(), 
				     subjectTable.getQualifiedNameAsAnsiString())) 
	{
	  return this;
	}
    }
  // Now that we know the final set of columns updated in the query, we
  // can determine whether the update of the MVs is direct or indirect.
  allTriggers = getTriggeredMvs(bindWA, allTriggers, columns);
  if (bindWA->errStatus())
    return this;
#endif

  if ((allTriggers   == NULL) && 
      (riConstraints == NULL) && 
      !isMVLoggingRequired    &&
      !getTableDesc()->hasSecondaryIndexes())
  {
    InliningFinale(bindWA, boundExpr, origScopeRETDesc);
    bindWA->resetUniqueIudNum(prevIudNum); // restore the saved IudNum
    return boundExpr;  // Nothing for us to do here.
  }

  // Get the row and statement triggers that match the updated column list.
  TriggerList *rowTriggers  = NULL;
  if ((allTriggers != NULL) && (allTriggers->getAfterRowTriggers() != NULL))
    rowTriggers = allTriggers->getAfterRowTriggers()->getColumnMatchingTriggers(columns);
  TriggerList *stmtTriggers = NULL;
  if ((allTriggers != NULL) && (allTriggers->getAfterStatementTriggers() != NULL))
    stmtTriggers = allTriggers->getAfterStatementTriggers()->getColumnMatchingTriggers(columns);

  if (rowTriggers != NULL) {   
    if (checkForNotAtomicStatement(bindWA, 30034, 
				  ((*rowTriggers)[0])->getTriggerName(), 
				  subjectTable.getQualifiedNameAsAnsiString())) 
    {
	return this;
    }
    else if (isNoRollback() ||
	      (CmpCommon::transMode()->getRollbackMode() == TransMode::NO_ROLLBACK_)) 
    {
	*CmpCommon::diags() << DgSqlCode(-3232) 
	<< DgString0(subjectTable.getQualifiedNameAsAnsiString()) 
	<< DgString1("After Trigger :")
	<< DgString2(((*rowTriggers)[0])->getTriggerName());
	bindWA->setErrStatus();

	return this ;
    }
  }
  else if (stmtTriggers != NULL) { 
   if ((CmpCommon::getDefault(NAR_DEPOBJ_ENABLE2) == DF_OFF) )
     {
       if (checkForNotAtomicStatement(bindWA, 30034, 
				      ((*stmtTriggers)[0])->getTriggerName(), 
				      subjectTable.getQualifiedNameAsAnsiString())) 
	 {
	   return this;
	 }
     }
    else if (isNoRollback() ||
	      (CmpCommon::transMode()->getRollbackMode() == TransMode::NO_ROLLBACK_)) 
    {
	*CmpCommon::diags() << DgSqlCode(-3232) 
	<< DgString0(subjectTable.getQualifiedNameAsAnsiString()) 
	<< DgString1("After Trigger :")
	<< DgString2(((*stmtTriggers)[0])->getTriggerName());
	bindWA->setErrStatus();

	return this ;
    }
  }
  

  if ((stmtTriggers != NULL) || (beforeTriggers != NULL))
  {
    NAString trigName = stmtTriggers ? ((*stmtTriggers)[0])->getTriggerName() : ((*beforeTriggers)[0])->getTriggerName() ;
    if (getFirstNRows() > 0)
    {
      // first N delete not supported with before triggers and after statement triggers.
      *CmpCommon::diags() << DgSqlCode(-11045) 
	  << DgString0(trigName) 
	  << DgString1(subjectTable.getQualifiedNameAsAnsiString());
	bindWA->setErrStatus();
	return this;
    }
  }

  // Filter only the RI constraints that match the updated columns.
  if (riConstraints != NULL)
  {
    riConstraints = riConstraints->getNeededRIs(columns, heap);
    if (riConstraints->isEmpty())
      riConstraints = NULL;		
    else // There are RI constraints that need to be enforced.  
      {
	// Disallow embedded delete on a referenced table.
	// Disallow embedded updates on columns which are part of an RI constraint.
	if (getGroupAttr()->isEmbeddedDelete()) 
	  {
	    *CmpCommon::diags() << DgSqlCode(-4183);
	    bindWA->setErrStatus();
	    return this;
	  }
	if (getGroupAttr()->isEmbeddedUpdate()) 
	  {
	    *CmpCommon::diags() << DgSqlCode(-4184);
	    bindWA->setErrStatus();
	    return this;
	  }
	if ((CmpCommon::getDefault(NAR_DEPOBJ_ENABLE2) == DF_OFF) )
	  {
       
	    if (checkForNotAtomicStatement(bindWA, 30028, 
					   (riConstraints->at(0)->getConstraintName()).getQualifiedNameAsAnsiString(),
					   subjectTable.getQualifiedNameAsAnsiString())) {
	      return this ;
	    }
	  }
	  
	if (isNoRollback() ||
		  (CmpCommon::transMode()->getRollbackMode() == TransMode::NO_ROLLBACK_)) {
	  *CmpCommon::diags() << DgSqlCode(-3232) 
	  << DgString0(subjectTable.getQualifiedNameAsAnsiString()) 
	  << DgString1("Referential Intergrity Constraint :")
	  << DgString2((riConstraints->at(0)->getConstraintName()).getQualifiedNameAsAnsiString());
	  bindWA->setErrStatus();

	  return this ;
	}
	
      }
  }

  NABoolean needIM = isIMNeeded(columns);

  if (needIM) {
    if ((isNoRollback() || 
	(CmpCommon::transMode()->getRollbackMode() == TransMode::NO_ROLLBACK_)) ||
	(bindWA->getHostArraysArea() &&
	bindWA->getHostArraysArea()->getTolerateNonFatalError())) {
      NAString indexName;
      const LIST(IndexDesc *) indexList = getTableDesc()->getIndexes();
      for (CollIndex i=0; i<indexList.entries(); i++) 
	{
	  IndexDesc *index = indexList[i];
	  if (!(index->isClusteringIndex())) {
	    indexName = index->getExtIndexName();
	    break;
	  }
	}
      if ((CmpCommon::getDefault(NAR_DEPOBJ_ENABLE) == DF_OFF) )
	{
	  if (checkForNotAtomicStatement(bindWA, 
					 30026, 
					 indexName, 
					 subjectTable.getQualifiedNameAsAnsiString())) 
	    {
	      return this;
	
	    }
	}
      if (isNoRollback() ||
	  (CmpCommon::transMode()->getRollbackMode() == TransMode::NO_ROLLBACK_)) {
	*CmpCommon::diags() << DgSqlCode(-3232) 
			    << DgString0(subjectTable.getQualifiedNameAsAnsiString()) 
			    << DgString1("Index :")
			    << DgString2(indexName);
	bindWA->setErrStatus();

	return this ;
	  
      }
    }
    getInliningInfo().setFlags(II_hasIM);
  }

  // Do we have any triggers to work on?
  if ((beforeTriggers == NULL) && 
      (rowTriggers    == NULL) && 
      (stmtTriggers   == NULL) )
  {
     bindWA->resetUniqueIudNum(prevIudNum);

     if (!needIM && !isMVLoggingRequired && (riConstraints  == NULL))
     {
	// We get here only if this is an Update operation, and there are 
	// triggers/IM/RI defined on Update on this table, but all of them
	// are on columns other than the ones updated.
	// e.g. A trigger AFTER UPDATE OF (a) ON T1,
	// and a triggering statement like: UPDATE T1 SET B=5;
        InliningFinale(bindWA, boundExpr, origScopeRETDesc);
	CMPASSERT (op == COM_UPDATE);		
	return boundExpr;
     }

     // OK, so we have no triggers - just RI, IM or both.
     createOldAndNewCorrelationNames(bindWA);
     setNoFlow(TRUE);
     getInliningInfo().setFlags(II_hasPipelinedActions); 

     topNode = 
       inlineOnlyRIandIMandMVLogging(bindWA, 
				     this, 
				     needIM, 
				     riConstraints, 
				     isMVLoggingRequired, 
				     columns, 
				     heap);

     if (needIM)
     {
        topNode->getInliningInfo().setFlags(II_DrivingIM);
     }

     topNode = topNode->bindNode(bindWA);
     // Create the tree that handles Index Maintainance.
     if (needIM)
     {
       // don't allow index maintenance on non-audited tables unless
       // this is enabled by a default
       // We allow index maintenance in an Internal refresh statement 
       if (!getTableDesc()->getClusteringIndex()->getNAFileSet()->isAudited() &&
	   !bindWA->isBindingMvRefresh())
       {
         NAString dummyTokString(bindWA->wHeap());
         const NATable *naTable = bindWA->getNATable(getTableName());
         DefaultToken imAllowed =
            bindWA->getSchemaDB()->getDefaults().token(IUD_NONAUDITED_INDEX_MAINT,
						       dummyTokString);

         switch (imAllowed)
         {
           case DF_ON:
             // go ahead and do it, the user asked for it
             break;
           case DF_WARN:
             // emit a warning and continue
             *CmpCommon::diags() << DgSqlCode(4203) <<
               DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
             break;
           default:
             // stop with an error, index maintenance is not allowed on
             // nonaudited tables
             *CmpCommon::diags() << DgSqlCode(-4203) <<
               DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
             bindWA->setErrStatus();
             return this;
         }
       }
     }

     if (bindWA->errStatus())
     {
       return boundExpr;
     }

     getInliningInfo().setFlags(II_hasInlinedActions);

     InliningFinale(bindWA, topNode, origScopeRETDesc);
     return topNode;
  }

  // Now, that we know for sure there are triggers to be fired, tempTableObj
  // must be initialized. In case there are no regular triggers (not MVImmediate)
  // defined on the table, the tempTableObj is not initialized yet (see initializing
  // of tempTableObj above).

  if (tempTableObj == NULL)
  {
    tempTableObj = new(heap) TriggersTempTable(this, bindWA);
  }

  setNoFlow(TRUE);

  if (!tsjRETDescCreated)
    createOldAndNewCorrelationNames(bindWA);

  if ( (rowTriggers   != NULL) ||
       (riConstraints != NULL) ||
	needIM                 ||
	isMVLoggingRequired)
    getInliningInfo().setFlags(II_hasPipelinedActions); 

  // Forbid the use of the Materialize node by the optimizer, for the entire
  // backbone, if we are now cascaded from a row after trigger.
  NABoolean forbidMaterializeNodeHere = shouldForbidMaterializeNodeHere(bindWA);
  Int32 prevStateOfFlags = 0;
  if (forbidMaterializeNodeHere)
  {
    // Set this InliningInfo flag in every node being bound (see RelExpr::bindSelf())
    // but save the previous state first.
    prevStateOfFlags = bindWA->getInliningInfoFlagsToSetRecursivly();
    bindWA->setInliningInfoFlagsToSetRecursivly(II_MaterializeNodeForbidden);
  }

  // Create and bind the rest of the trigger backbone.
  if (beforeTriggers == NULL)
    {
      if (bindWA->getHostArraysArea() && bindWA->getHostArraysArea()->getTolerateNonFatalError()&& (needIM || riConstraints))
	{
	  
	  topNode = inlineAfterOnlyBackboneForUndo(bindWA, 
					    *tempTableObj,
					    rowTriggers,
					    stmtTriggers,
					    riConstraints,
					    needIM,
					    isMVLoggingRequired,
					    columns,
					    heap);
	}
      else
	{
	  topNode = inlineAfterOnlyBackbone(bindWA, 
					    *tempTableObj,
					    rowTriggers,
					    stmtTriggers,
					    riConstraints,
					    needIM,
					    isMVLoggingRequired,
					    columns,
					    heap);
	}
    }
  else
    topNode = inlineBeforeAndAfterBackbone(bindWA, 
					   beforeSubtree,
					   *tempTableObj,
					   rowTriggers,
					   stmtTriggers,
					   riConstraints,
					   needIM,
					   isMVLoggingRequired,
					   columns,
					   heap);

  if (forbidMaterializeNodeHere)
  {
    // Restore to the the previous state. This effectivly resets the flag
    // we set before binding, so we don't affect sibtrees that will be bound 
    // after us.
    bindWA->setInliningInfoFlagsToSetRecursivly(prevStateOfFlags);
  }

  bindWA->resetUniqueIudNum(prevIudNum);
  // indicate that this is a subject table for enable/disable
  getOptStoi()->getStoi()->setSubjectTable(TRUE);

  getInliningInfo().setFlags(II_hasInlinedActions);

  InliningFinale(bindWA, topNode, origScopeRETDesc);
  return topNode;
}

//////////////////////////////////////////////////////////////////////////////
// This method checks to see if an update should be transformed into 
// insert/delete nodes.  This is true if the update is on a primary key and
// special conditions don't exist.
//
// Return value: 
// TRUE  - if the update is on a clustering or unique index key and no special
//         conditions exist.  Causes the update to be transformed to a delete
//         followed by an insert with intervening order by.
// FALSE - if the update is on a clustering or unique index key and no special
//         conditions exist.  OR
//         NOT update is on a clustering or unique index key.
//////////////////////////////////////////////////////////////////////////////
NABoolean Update::updatesClusteringKeyOrUniqueIndexKey(BindWA *bindWA)
{
  // This CQD must be ON or AGGRESSIVE in order to enable the transformation 
  // for special conditions.  If not, return false
  if (CmpCommon::getDefault(UPDATE_CLUSTERING_OR_UNIQUE_INDEX_KEY) == DF_OFF)
    return FALSE;

  ULng32 numberofKeys = getTableDesc()->
                         getClusteringIndex()->getIndexKey().entries();

  NABoolean hasSysKey = getTableDesc()->getClusteringIndex()->
                            getNAFileSet()->hasSyskey();

  // this restriction (i.e unable to transform update of unique index key
  // if base table has only syskey as clustering key may be removable
  // the intention is to revisit this once a similar problem has been 
  // solved for the Halloween feature.
  if ((numberofKeys == 1) AND hasSysKey)
    return FALSE;  // This should never be reached.

  // 1. Determine whether columns being updated are clustering key columns
  //    of base table or of a unique index.  Set flags used later.
  const LIST(IndexDesc *) & ixlist = getTableDesc()->getIndexes();
  NABoolean isUniqueIndexCol = FALSE;
  NABoolean isClusteringKeyCol = FALSE;
  NAString ckColName;  // Save column name if is clustering key.
  Scan * scanNode = getScanNode();
  const ValueIdList colUpdated = scanNode->getTableDesc()->getColUpdated();
  for (CollIndex indexNo = 0; indexNo < ixlist.entries(); indexNo++)
  {
    if (isUniqueIndexCol && isClusteringKeyCol)
      break ;

    IndexDesc *idesc = ixlist[indexNo];
    if (idesc->isUniqueIndex() || idesc->isClusteringIndex())
    {
      const ValueIdList indexKey = idesc->getIndexKey();

      for (CollIndex i = 0; i < colUpdated.entries(); i++)
      {
        ItemExpr *updateCol = colUpdated[i].getItemExpr();
        CMPASSERT(updateCol->getOperatorType() == ITM_BASECOLUMN);
        for (CollIndex j = 0; j < indexKey.entries(); j++)
        {
          ItemExpr *keyCol = indexKey[j].getItemExpr();
          ItemExpr *baseCol = ((IndexColumn*)keyCol)->getDefinition().getItemExpr();
          CMPASSERT(baseCol->getOperatorType() == ITM_BASECOLUMN);
          if (((BaseColumn*)updateCol)->getColNumber() ==
              ((BaseColumn*)baseCol)->getColNumber())
          {
            if (idesc->isUniqueIndex())
              isUniqueIndexCol = TRUE;
            else {
              isClusteringKeyCol = TRUE;
              ckColName = ((BaseColumn*)updateCol)->getColName();
            }
          }
        }
      } // for (CollIndex ...)
    }
  } // for (CollIndex ...)

  if ((CmpCommon::getDefault(UPDATE_CLUSTERING_OR_UNIQUE_INDEX_KEY) == DF_AGGRESSIVE) &&
      (NOT isClusteringKeyCol)) 
    return FALSE;

  // 2. If columns being updated are unique index or clustering key columns, 
  //    check for 5 unsupported special cases and issue error in each case.
  if (isUniqueIndexCol || isClusteringKeyCol)
  {
    // 2a. Check for first three unsupported special cases:
    //     i.   User declared cursor and performing update current of.
    //     ii.  Update statement with set on rollback.
    //     iii. Update with a select surrounding it.
    if ( updateCurrentOf() || 
         newRecBeforeExpr().entries() > 0 || 
         getGroupAttr()->isEmbeddedUpdate()
       )
    {
      if (isUniqueIndexCol && NOT isClusteringKeyCol)
	return FALSE; // column being updated is unique key but one of the 
        // three special cases apply. Revert to delete followed by insert 
        // without intervening sort.  NOT AN ERROR.

      // Set appropriate error for first three special cases.
      if ( updateCurrentOf() )
	*CmpCommon::diags() << DgSqlCode(-4118) ;
      if (newRecBeforeExpr().entries() > 0)
	*CmpCommon::diags() << DgSqlCode(-4199) ;
      if (getGroupAttr()->isEmbeddedUpdate())
	*CmpCommon::diags() << DgSqlCode(-4198) ;
      bindWA->setErrStatus();
      return FALSE ; // ERROR condition.
    }
     
    // 2b. Check for unsupported special case 4:
    //     iv. There is an MV on this table that is defined on 
    //         the clustering key(s).
    const NATable *naTable = bindWA->getNATable(getTableName());
    if (naTable)
    {
      // Check for MVs on table.
      const UsingMvInfoList &mvList = naTable->getMvsUsingMe();
      if (!mvList.isEmpty())
      {
        // MV(s) exist.  Check to see if any MV is ON STATEMENT and is
        // significant.  (Update on a primary key with an MV on that 
        // key is not supported - return FALSE.)
        for (CollIndex i = 0; i < mvList.entries(); i++)
          if (mvList[i]->isInitialized() && 
              mvList[i]->getRefreshType() == COM_ON_STATEMENT)
          {
            CorrName mvCorr = CorrName(mvList[i]->getMvName(), bindWA->wHeap());
            NATable *naTableMv = bindWA->getNATable(mvCorr);
            if (bindWA->errStatus()) return FALSE;
            MVInfoForDML *mvInfo = naTableMv->getMVInfo(bindWA);
            // getMVInfo() reads from metadata, but saves the info found
            // and will be called later anyway during binding.
            UpdateColumns *columns = NULL;
            columns = new(bindWA->wHeap()) UpdateColumns(getOptStoi()->getStoi());
            if (checkUpdateType(mvInfo, naTable->getTableName(), columns) !=
                IRELEVANT)
            {
              if (isClusteringKeyCol) // update CK not supported in this case.
              {
                *CmpCommon::diags() << DgSqlCode(-4033) << DgColumnName(ckColName);
                bindWA->setErrStatus();
              }
              return FALSE;
            }
          }
      }
    }

if (isIgnoreTriggers() == FALSE)
    {
      ComOperation op;
      op = COM_UPDATE; 
      BeforeAndAfterTriggers *allTriggers = 0;
      QualifiedName& subjectTable = getTableDesc()->getCorrNameObj().getQualifiedNameObj();
      allTriggers = bindWA->getSchemaDB()->getTriggerDB()->getTriggers(subjectTable, op, bindWA);
      UpdateColumns *columns = NULL;
      columns = new(bindWA->wHeap()) UpdateColumns(getOptStoi()->getStoi());
      TriggerList *beforeTriggers = NULL;
      if ((allTriggers != NULL) && (allTriggers->getBeforeTriggers() != NULL))
        beforeTriggers = allTriggers->getBeforeTriggers()->getColumnMatchingTriggers(columns);
      if (beforeTriggers)
      {
        if (isClusteringKeyCol) // update CK not supported if table has beforeTriggers.
        {
          *CmpCommon::diags() << DgSqlCode(-4033) << DgColumnName(ckColName);
          bindWA->setErrStatus();
        }
        return FALSE;
      }
    }

    return TRUE;  // Column being updated is CK or unique index key and no special cases apply.
  } // if (isUniqueIndexCol || isClusteringKeyCol)

  return FALSE ;  // Column being updated is not CK or unique index key
}

RelExpr *Update::transformUpdatePrimaryKey(BindWA *bindWA)
{

   Delete * delNode = new (bindWA->wHeap())
			Delete(CorrName(getTableDesc()->getCorrNameObj(), bindWA->wHeap()),
				NULL,
				REL_UNARY_DELETE,
				child(0),
				NULL);
   delNode->setNoLogOp(CONSISTENT_NOLOG);
   delNode->setUpdateCKorUniqueIndexKey(TRUE);
   delNode->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;

   ValueIdList selectList, sourceColsList, lhsOfSetClause;
   
   getTableDesc()->getUserColumnList(selectList);
   getScanIndexDesc()->getPrimaryTableDesc()->getUserColumnList(sourceColsList);
   ValueId vid ;
   CollIndex pos;

   // newRecExprArray is a list of assigns. For each assign
   // child(0) is the LHS of the set clause and child(1) is
   // the RHS of the SET clause

   for (CollIndex i=0; i < newRecExprArray().entries(); i++)
    {
      lhsOfSetClause.insertAt(i,newRecExprArray().at(i).getItemExpr()->child(0).getValueId());
    }

   for (CollIndex i=0; i < selectList.entries(); i++)
    {
      if ((pos = lhsOfSetClause.index(selectList[i])) == NULL_COLL_INDEX)
	selectList[i] = sourceColsList[i];
      else
	selectList[i] = newRecExprArray().at(pos).getItemExpr()->child(1).getValueId();
    }

    for (CollIndex i=0; i < oldToNewMap().getTopValues().entries(); i++) {
      BaseColumn *col = (BaseColumn *) oldToNewMap().getBottomValues()[i].getItemExpr();
      NABoolean addToOldToNewMap = TRUE;

      // Copy the oldToNewMap.
      if (col->getNAColumn()->isComputedColumnAlways()) {
        // Computed columns can be copied from delete to insert if they don't
        // change. Don't include the column in this map, though, if one of
        // the underlying columns gets updated, because the value of the
        // computed column has to be recomputed. That computation will be
        // done in the new insert node.
        ValueIdSet underlyingCols;

        col->getUnderlyingColumnsForCC(underlyingCols);

        if (NOT underlyingCols.intersect(lhsOfSetClause).isEmpty())
          addToOldToNewMap = FALSE;
      }

      // Copy the oldToNewMap.
      if (addToOldToNewMap)
        delNode->oldToNewMap().addMapEntry(oldToNewMap().getTopValues()[i],
                                           oldToNewMap().getBottomValues()[i]);
    }

   RelRoot * rootNode = new (bindWA->wHeap())
			RelRoot(delNode, 
				REL_ROOT,
				selectList.rebuildExprTree(ITM_ITEM_LIST));

   RelExpr * boundExpr;
   Insert * insNode = new (bindWA->wHeap())
		Insert(CorrName(getTableDesc()->getCorrNameObj(),bindWA->wHeap()),
				getTableDesc(), // insert gets the same tabledesc as the update
				REL_UNARY_INSERT,
				rootNode,
				NULL);
   insNode->setNoLogOp(isNoLogOperation());
   insNode->setSubqInUpdateAssign(subqInUpdateAssign());

   if (this->rowsAffected() == GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED)
      insNode->rowsAffected() = GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED;
   else
      insNode->rowsAffected() = GenericUpdate::COMPUTE_ROWSAFFECTED;
      
   insNode->setUpdateCKorUniqueIndexKey(TRUE);
   InliningInfo inlineInfo = getInliningInfo();
   insNode->setInliningInfo(&inlineInfo);
   if (CmpCommon::getDefault(UPDATE_CLUSTERING_OR_UNIQUE_INDEX_KEY) == DF_ON) {
     insNode->setAvoidHalloween(TRUE);
     insNode->setHalloweenCannotUseDP2Locks(TRUE);
   }
   // used to convey updated columns to insert node's stoi
   // during inlining the update node is not present anymore, we read the 
   // insert's stoi to figure out which columns are updated.
   SqlTableOpenInfo * scanStoi = getLeftmostScanNode()->getOptStoi()->getStoi();
   short updateColsCount = getOptStoi()->getStoi()->getColumnListCount();
   scanStoi->setColumnListCount(updateColsCount);
   scanStoi->setColumnList(new (bindWA->wHeap()) short[updateColsCount]);
   for (short i=0; i<updateColsCount; i++)
    scanStoi->setUpdateColumn(i,getOptStoi()->getStoi()->getUpdateColumn(i));

   boundExpr = insNode->bindNode(bindWA);
   return boundExpr;
}

