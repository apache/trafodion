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
* File:         MVInfo.cpp
* Description:  Definition of class MVInfo.
*
* Created:      5/09/99
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/

#define  SQLPARSERGLOBALS_FLAGS	   // must precede all #include's
#define  SQLPARSERGLOBALS_CONTEXT_AND_DIAGS

#include "Sqlcomp.h"
#include "AllItemExpr.h"
#include "AllRelExpr.h"
#include "BindWA.h"
#include "GroupAttr.h"
#include "MVInfo.h"
#include "Refresh.h"
#include "MVJoinGraph.h"
#include "MJVIndexBuilder.h"
#include "ColIndList.h"
#include "ChangesTable.h"
#include "ElemDDLStoreOptions.h"
#include "QRDescriptor.h"
#include "QueryRewriteHandler.h"

#if !TEXT_IS_SELECT_ONLY
#include "StmtDDLCreateMV.h"
#endif

#include "SqlParserGlobals.h"		// must be last #include

#include "NormWA.h"

void castComputedColumnsToAnsiTypes(BindWA *bindWA, RETDesc *rd, ValueIdList &compExpr);

//////////////////////////////////////////////////////////////////////////////
// Given the ItemExpr tree 'expr', find a leaf that is a base column.
// The output parameter moreThanOne is returned TRUE when there is more 
// than one base column leaf to the tree.
//////////////////////////////////////////////////////////////////////////////
static BaseColumn *findTheBaseColumn(ItemExpr  *expr, 
				     NABoolean *moreThanOne=NULL)
{
  if (moreThanOne!=NULL)
    *moreThanOne = FALSE;

  if (expr->getOperatorType() == ITM_BASECOLUMN)
    return (BaseColumn *)expr;

  BaseColumn *result = NULL;
  ValueIdSet leafs;
  expr->getLeafValueIds(leafs);
  for (ValueId leaf= leafs.init();  leafs.next(leaf); leafs.advance(leaf) ) 
  {
    ItemExpr *node = leaf.getItemExpr();
    if (node->getOperatorType() == ITM_BASECOLUMN)
    {
      if (result==NULL)
	result = (BaseColumn *)node;
      else
      {
        if (moreThanOne!=NULL)
	  *moreThanOne = TRUE;
	return result;
      }
    }
    else if (node->getOperatorType() == ITM_VEG_REFERENCE)
    {
      const VEG *vegRef = ((VEGReference *)node)->getVEG();
      const ValueIdSet& vegMembers = vegRef->getAllValues();
      for (ValueId column = vegMembers.init();
	   vegMembers.next(column);
	   vegMembers.advance(column) ) 
      {
	ItemExpr *baseColExpr = column.getItemExpr();
	if (baseColExpr->getOperatorType() != ITM_BASECOLUMN)
	  continue;

	if (result==NULL)
	  result = (BaseColumn *)baseColExpr;
	else
	{
	  if (moreThanOne!=NULL)
	    *moreThanOne = TRUE;
	  return result;
	}
      }
    }
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
static void findAllTheUsedColumns(ItemExpr          *expr, 
				  TableDesc         *currentTable,
				  LIST(BaseColumn*)& usedCols)
{
  ValueIdSet leafs;
  expr->getLeafValueIds(leafs);

  for (ValueId leaf= leafs.init();  leafs.next(leaf); leafs.advance(leaf) ) 
  {
    ItemExpr *node = leaf.getItemExpr();
    if (node->getOperatorType() == ITM_VEG_REFERENCE)
    {
      VEG *veg = ((VEGReference *)node)->getVEG();
      const ValueIdSet& vegMembers = veg->getAllValues();

      for (ValueId vegMem = vegMembers.init();  vegMembers.next(vegMem); vegMembers.advance(vegMem) ) 
      {
	ItemExpr *expr = vegMem.getItemExpr();
	if (expr->getOperatorType() != ITM_BASECOLUMN)
	  continue;
	BaseColumn *baseCol = (BaseColumn *)expr;
	if (currentTable !=NULL && baseCol->getTableDesc() != currentTable)
	  continue;

	usedCols.insert((BaseColumn *)baseCol);
//	break;
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// Should be called on a Normalized tree.
//////////////////////////////////////////////////////////////////////////////
static NABoolean isTreeLeftLinear(RelExpr *node)
{
  if (node == NULL)
    return TRUE;

  OperatorTypeEnum nodeType = node->getOperatorType();
  if (nodeType == REL_SCAN)
    return TRUE;
  if (nodeType == REL_ISOLATED_SCALAR_UDF &&
      ((RelRoutine *) node->castToRelExpr())->getNARoutine()->isDeterministic())
    return TRUE;

  if (nodeType == REL_JOIN || nodeType == REL_LEFT_JOIN || nodeType == REL_ROUTINE_JOIN)
  {
    if (node->child(1)->getOperatorType() == REL_SCAN ||
        (node->child(1)->getOperatorType() == REL_ISOLATED_SCALAR_UDF &&
         ((RelRoutine *) node->child(1)->castToRelExpr())
             ->getNARoutine()->isDeterministic() )
       )
      return isTreeLeftLinear(node->child(0));
    else return FALSE;
  }
  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
static ItemExpr *SkipCast(ItemExpr *expr)
{
  if (expr->getOperatorType() == ITM_CAST)
    return expr->child(0);
  else if (expr->getOperatorType() == ITM_CHAR &&
           expr->child(0)->getOperatorType() == ITM_CAST)
  {
    expr->child(0) = expr->child(0)->child(0);
    return expr;
  }
  else
    return expr;
}

//////////////////////////////////////////////////////////////////////////////
// Make sure expr is not a non-repeatable function.
//////////////////////////////////////////////////////////////////////////////
static NABoolean isANonRepeatableExpression(const ItemExpr *expr,
					    BindWA	   *bindWA)
{
  if (expr==NULL)
    return FALSE;

  switch(expr->getOperatorType())
  {
    // These are non-repeatable functions - they are not incremental.
    case ITM_CURRENT_TIMESTAMP	:
    case ITM_CURRENT_USER	:
    case ITM_SESSION_USER	:
    case ITM_AUTHTYPE		:
    case ITM_AUTHNAME		:
    case ITM_USER		:
    case ITM_SAMPLE_VALUE	:
      return TRUE;

    // Host vars and parameters are forbidden. Need an error here.
    case ITM_HOSTVAR		:
      if (((HostVar *)expr)->isSystemGenerated())
      {
	// 12317 Materialized views do not support vertically partitioned tables.
	*CmpCommon::diags() << DgSqlCode(-12317);
	bindWA->setErrStatus();
	return TRUE;
      }
      // If not system generated, fall through to next error.

    case ITM_DYN_PARAM		:
      // 12303 Materialized Views do not support host variables and parameters.
      *CmpCommon::diags() << DgSqlCode(-12303);
      bindWA->setErrStatus();
      return TRUE;

    default:
      return FALSE;
  }
}

//============================================================================
//=====  collectMVInformation() methods of the RelExpr class hierarchy  ======
//============================================================================

void RelExpr::collectMVInformation(MVInfoForDDL *mvInfo, 
				   NABoolean isNormalized)
{
  if (!isIncrementalMV())
    mvInfo->setNotIncremental(); // use default reason

  Int32 arity = getArity();
  for (Lng32 i = 0; i < arity; i++)
    child(i)->collectMVInformation(mvInfo, isNormalized);
}

void Join::collectMVInformation(MVInfoForDDL *mvInfo,
				NABoolean isNormalized)
{
  RelExpr::collectMVInformation(mvInfo, isNormalized);
  // In the Join nodes, we need the VEG predicates, prepared
  // during normalization.
  if (isNormalized)
    mvInfo->addJoinNode(this);
}

void Scan::collectMVInformation(MVInfoForDDL *mvInfo,
				NABoolean isNormalized)
{
  RelExpr::collectMVInformation(mvInfo, isNormalized);
  // In the Scan nodes, we need the minimal outputs, that are
  // calculated during normalization.
  if (isNormalized)
    mvInfo->addScanNode(this);
}

void GroupByAgg::collectMVInformation(MVInfoForDDL *mvInfo,
				NABoolean isNormalized)
{
  RelExpr::collectMVInformation(mvInfo, isNormalized);
  // GroupBy information is ready for use after binding.
  if (!isNormalized)
    mvInfo->addGroupByNode(this);
}

void RenameTable::collectMVInformation(MVInfoForDDL *mvInfo,
				       NABoolean isNormalized)
{
  // RenameTable nodes don't exist after the transformNode() phase.
  CMPASSERT(!isNormalized)

  RelExpr::collectMVInformation(mvInfo, isNormalized);
  mvInfo->addViewRenameNode(this);
}


//============================================================================
//======================  class ViewColumnGraph  =============================
//============================================================================

//----------------------------------------------------------------------------
// The parameter is a column descriptor from the RenameTable node on top of
// a view. It has both the name of the view column, and the ValueId of the
// mapped expression.
ViewColumnConnection::ViewColumnConnection(const ColumnDesc *colDesc, 
					   CollHeap *heap)
  : baseColName_(heap),
    viewColName_(colDesc->getColRefNameObj(), heap),
    vid_(colDesc->getValueId()),
    isComplex_(FALSE),
    isInvalid_(FALSE)
{
  ItemExpr *colExpr = colDesc->getValueId().getItemExpr();
  BaseColumn *baseCol;
  if (colExpr->getOperatorType() == ITM_BASECOLUMN)
    baseCol = (BaseColumn *)colExpr;
  else
  {
    isComplex_ = TRUE;
    baseCol = findTheBaseColumn(colExpr);
  }

  if (baseCol == NULL)
  {
    // We get here if a view column does not map to any base table column.
    // This column will not be added to the view graph.
    isInvalid_ = TRUE;
  }
  else
  {
    baseColName_ = baseCol->getNAColumn()->getFullColRefName();
    key_ = new(heap) NAString(baseColName_.getColName(), heap);
  }
}


NABoolean ViewColumnConnection::operator==(const ViewColumnConnection &other) const
{
  if(&other == this)
    return TRUE;
  else
    return FALSE;
}

//============================================================================
// ViewColumnConnection and ViewTableConnection classes ignored from coverage
// testing until MV on view is supported.

//----------------------------------------------------------------------------
ViewTableConnection::~ViewTableConnection()
{
  colConnections_.clearAndDestroy();
}

//----------------------------------------------------------------------------
const ViewColumnConnection *ViewTableConnection::findColumn(const ColRefName& colName) const
{
  return colConnections_.getFirstValue(&colName.getColName());
}

//----------------------------------------------------------------------------
NABoolean ViewTableConnection::contains(const ColRefName& colName) const
{
  return (findColumn(colName) != NULL);
}

//----------------------------------------------------------------------------
const ColRefName& ViewTableConnection::findTopViewOf(const ColRefName& colName) const
{
  const ViewColumnConnection *col = findColumn(colName);
  if (col == NULL)
    return colName;
  else
    return col->getViewColName();
}

//----------------------------------------------------------------------------
NABoolean ViewTableConnection::isMaskedColumn(const ColRefName& col) const
{
  const ViewColumnConnection *colConn = findColumn(col);
  if (colConn == NULL)
    return TRUE;

  return (colConn->getViewName() != getTopViewName());
}

//----------------------------------------------------------------------------
NABoolean ViewTableConnection::isMaskedTable(const QualifiedName& tableName) const
{
  return (tableName != getTopViewName());
}

//----------------------------------------------------------------------------
// Insert a new column connectiopn to the table graph.
void ViewTableConnection::insert(const ViewColumnConnection *col) 
{
  // Look for an existing connection for this base table column.
  const ViewColumnConnection *currentCol = 
    colConnections_.getFirstValue(col->getColName());

  // If one is found - remove it before inserting the new one.
  if (currentCol != NULL)
  {
    CMPASSERT(*currentCol->getColName() == *col->getColName());
    colConnections_.remove(currentCol->getColName());
    delete currentCol;
  }

  // Insert the new connection.
  colConnections_.insert(col->getColName(), col); 

  // We may have a new top view name.
  topViewName_ = col->getViewName();
}

//----------------------------------------------------------------------------
// Display/print, for debugging.
#ifndef NDEBUG
void ViewTableConnection::print(FILE* ofd,
				const char* indent,
				const char* title) const
{
  NAString tableName = getTableName().getQualifiedNameAsString();
  fprintf(ofd, "\n\tTable %s:\n", tableName.data());

  columnConnection& hash = (columnConnection&)colConnections_;
  NAHashDictionaryIterator<const NAString, const ViewColumnConnection> 
    iterator(hash);

  const NAString *key	= NULL;
  const ViewColumnConnection *value = NULL;
  iterator.getNext(key, value);
  while (key != NULL)
  {
    NAString viewColName = value->getViewColName().getColRefAsString();
    fprintf(ofd, "\t\t%s: %s \n", key->data(), viewColName.data());
    iterator.getNext(key, value);
  }
}

void ViewTableConnection::display() const
{
  print();
}
#endif

//============================================================================

//----------------------------------------------------------------------------
ViewColumnGraph::~ViewColumnGraph()
{
  for (CollIndex i=0; i<tableConnections_.entries(); i++)
    delete tableConnections_[i];
}

//----------------------------------------------------------------------------
// This is the hash function on the ValueId.
ULng32 ViewColumnGraph::ValueIdHash(const ValueId& vid)
{
  CollIndex id = (CollIndex)vid;
  return id;
}

//----------------------------------------------------------------------------
const ViewColumnConnection *ViewColumnGraph::findColumn(const ValueId vid) const
{
  return columnValueIds_.getFirstValue(&vid);
}

//----------------------------------------------------------------------------
ViewTableConnection *ViewColumnGraph::findTableConnection(const QualifiedName& tableName) const
{
  for (CollIndex i=0; i<tableConnections_.entries(); i++)
  {
    ViewTableConnection *tableConnection = tableConnections_[i];
    if (tableConnection->getTableName() == tableName)
      return tableConnection;
  }
  return NULL;
}

//----------------------------------------------------------------------------
void ViewColumnGraph::addConnection(ViewColumnConnection *colConnection, 
				    CollHeap		 *heap)
{
  // Insert the new connection into the hash by ValueId.
  columnValueIds_.insert(colConnection->getValueId(), colConnection);

  // Look for an existing table connection
  ViewTableConnection *tableConnection = 
    findTableConnection(colConnection->getTableName());

  // If none are found, create a new one and insert it into the list.
  if (tableConnection == NULL)
  {
    tableConnection = new(heap) 
      ViewTableConnection(colConnection->getTableName(), 
			  colConnection->getViewName(),
			  heap);
    tableConnections_.insert(tableConnection);
  }

  // If the column connection is direct and not complex, insert it into the 
  // table connection.
  if (colConnection->isComplex())
    return;

  tableConnection->insert(colConnection);
}

//----------------------------------------------------------------------------
// This method is called for each view RenameTable node on the RelExpr tree,
// as well as for Scan nodes that have a correlation name. The addSysCols
// parameter is TRUE when called for Scan nodes, for adding SYSKEY.
void ViewColumnGraph::addTableMapping(RETDesc  *retDesc, 
				      NABoolean addSysCols, 
				      CollHeap *heap)
{
  ColumnDescList viewColumns(heap);

  // Get a list of all user (and also system) columns from the RETDesc.
  viewColumns.insert(*retDesc->getColumnList());  
  if (addSysCols)
    viewColumns.insert(*retDesc->getSystemColumnList());  

  // Create a column connection object for each column on the list, and insert
  // it into the graph.
  for (CollIndex col=0; col<viewColumns.entries(); col++)
  {
    ViewColumnConnection *colConn = new(heap) 
      ViewColumnConnection(viewColumns[col], heap);

    // An invalid connection is for a view column that is mapped to an 
    // expression that does not contain any base columns. It is not added 
    // to the graph.
    if (colConn->isInvalid())
      delete colConn;
    else
      addConnection(colConn, heap);
  }
}

//----------------------------------------------------------------------------
NABoolean ViewColumnGraph::contains(const ColRefName& colName) const
{
  ViewTableConnection *tableConnection = 
    findTableConnection(colName.getCorrNameObj().getQualifiedNameObj());
  if (tableConnection == NULL)
    return FALSE;
  else
    return tableConnection->contains(colName);
}

//----------------------------------------------------------------------------
const ColRefName& ViewColumnGraph::findTopViewOf(const ColRefName& colName) const
{
  ViewTableConnection *tableConnection = 
    findTableConnection(colName.getCorrNameObj().getQualifiedNameObj());
  if (tableConnection == NULL)
    return colName;
  else
    return tableConnection->findTopViewOf(colName);
}

//----------------------------------------------------------------------------
// A masked column is one that does not have a mapping to the top most view.
NABoolean ViewColumnGraph::isMaskedColumn(const NAColumn *keyCol) const
{
  ViewTableConnection *tableConnection = 
    findTableConnection(*keyCol->getTableName());
  if (tableConnection == NULL)
    return FALSE;
  else
    return tableConnection->isMaskedColumn(keyCol->getFullColRefName());
}

//----------------------------------------------------------------------------
// A masked table is one that is not visible at the top level, because it
// is masked by a view.
NABoolean ViewColumnGraph::isMaskedTable(const QualifiedName& tableName) const
{
  if (tableName == "")
    return FALSE;

  ViewTableConnection *tableConnection = findTableConnection(tableName);
  if (tableConnection == NULL)
    return FALSE;
  else
    return tableConnection->isMaskedTable(tableName);
}

//----------------------------------------------------------------------------
const ColRefName *ViewColumnGraph::findViewNameFor(const ValueId vid) const
{
  const ViewColumnConnection *colConnection = findColumn(vid);
  if (colConnection == NULL)
    return NULL;
  else
    return &colConnection->getViewColName();
}

//----------------------------------------------------------------------------
// Display/print, for debugging.
#ifndef NDEBUG
void ViewColumnGraph::print(FILE* ofd,
			    const char* indent,
  			    const char* title) const
{
  fprintf(ofd, "\nViewColumnGraph:\n");
  for (CollIndex i=0; i<tableConnections_.entries(); i++)
    tableConnections_[i]->print();

  fprintf(ofd, "ValueId mappings:\n");
  valueIdConnection& hash = (valueIdConnection&)columnValueIds_;
  NAHashDictionaryIterator<const ValueId, const ViewColumnConnection> 
    iterator(hash);

  const ValueId *key = NULL;
  const ViewColumnConnection *value = NULL;
  iterator.getNext(key, value);
  while (key != NULL)
  {
    NAString viewColName = value->getViewColName().getColRefAsString();
    CollIndex vid    = (CollIndex)*value->getValueId();
    fprintf(ofd, "\t\t%d: %s \n", vid, viewColName.data());
    iterator.getNext(key, value);
  }

}

void ViewColumnGraph::display() const
{
  print();
}

#endif


//============================================================================
//=======================  class MVVegPredicateColumn ========================
//============================================================================

//////////////////////////////////////////////////////////////////////////////
// This Ctor is not in the .H file so that the BaseColumn class can be 
// defined there as a forward reference only, without #including additional
// header files.
// isFromVegRef is TRUE when this column is from the outer table of a left join.
//////////////////////////////////////////////////////////////////////////////
MVVegPredicateColumn::MVVegPredicateColumn(const BaseColumn  *baseCol, 
					   NABoolean	      isInLeftJoin,
					   NABoolean	      isFromVegRef,
					   ComMVSUsageType    usageType,
					   const ItemExpr    *complexExpr,
					   CollHeap	     *heap) :
  tableName_(baseCol->getTableDesc()->getCorrNameObj().getQualifiedNameObj(), heap),
  colNumber_(baseCol->getColNumber()),
  isComplex_(complexExpr != NULL),
  joinSide_ (!isInLeftJoin    ? 
	     COM_NO_LEFT_JOIN :
	     (isFromVegRef ? COM_LEFT_OUTER : COM_LEFT_INNER) ),
  complexExpr_(complexExpr),
  usageType_(usageType),
  text_(heap)
{
  if (complexExpr != NULL)
    complexExpr->unparse(text_, DEFAULT_PHASE, MVINFO_FORMAT);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
// Exclude from coverage testing - Debugging code
void MVVegPredicateColumn::print(FILE* ofd, const char* indent, const char* title) const
{
  NAString tableText =  getTableName().getQualifiedNameAsString();
  const char *leftJoinSide  = NULL;
  switch (joinSide_)
  {
    case COM_NO_LEFT_JOIN: leftJoinSide="N"; break;
    case COM_LEFT_INNER  : leftJoinSide="I"; break;
    case COM_LEFT_OUTER  : leftJoinSide="O"; break;
  }

  if (isComplex())
  {
    fprintf(ofd, "Complex: %s ", text_.data());
  }
  else
  {
    fprintf(ofd, "%s(%d) %s, ", 
      tableText.data(), getColNumber(), leftJoinSide);
  }
}

void MVVegPredicateColumn::display() const
{
  print();
}
#endif

//============================================================================
//========================  class MVVegPredicate  ============================
//============================================================================

//////////////////////////////////////////////////////////////////////////////
// Copy Ctor.
//////////////////////////////////////////////////////////////////////////////
MVVegPredicate::MVVegPredicate(const MVVegPredicate& other, CollHeap *heap)
: LIST(const MVVegPredicateColumn *) (heap, other.entries()),
  isOnLeftJoin_(other.isOnLeftJoin_),
  isIncremental_(other.isIncremental_),
  usageType_(other.usageType_)
{
  for (CollIndex i=0; i<other.entries(); i++)
    insert(new(heap) MVVegPredicateColumn(*other[i], heap));
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
MVVegPredicate::~MVVegPredicate() 
{ 
  while (entries()>0) 
  {
    const MVVegPredicateColumn *elem = at(0);
    removeAt(0);
    delete elem;
  }
}

//////////////////////////////////////////////////////////////////////////////
// Add all the columns of the VEG to the list.
// isFromVegRef is TRUE when this column is from the outer table of a left join.
//////////////////////////////////////////////////////////////////////////////
void MVVegPredicate::addVegPredicates(const VEG	         *veg, 
				      NABoolean	          isFromVegRef, 
				      LIST(const VEG *)&  vegPtrList,
				      CollHeap           *heap)
{
  const ValueIdSet& vegMembers = veg->getAllValues();
  for (ValueId column = vegMembers.init();
       vegMembers.next(column);
       vegMembers.advance(column) ) 
  {
    addPredicateMember(column, isFromVegRef, veg, vegPtrList, heap);
  }
}

//////////////////////////////////////////////////////////////////////////////
// Build an MVVegPredicateColumn object and insert it into the list.
//////////////////////////////////////////////////////////////////////////////
void MVVegPredicate::addPredicateMember(ValueId		    vid,
					NABoolean	    isFromVegRef,
					const VEG	   *parentVeg,
					LIST(const VEG *)&  vegPtrList,
					CollHeap           *heap)
{
  ItemExpr *predExpr = vid.getItemExpr();
  OperatorTypeEnum colType = predExpr->getOperatorType();
  switch (colType)
  {
    case ITM_BASECOLUMN:
    {
      // That's the simple case - a base column.
      BaseColumn *baseCol = (BaseColumn *)predExpr;
      MVVegPredicateColumn *col = new(heap)
	MVVegPredicateColumn(baseCol, 
			     isOnLeftJoin(), 
			     isFromVegRef, 
			     usageType_, 
			     NULL, 
			     heap);
      insert(col);
    }
    break;

    case ITM_VEG_REFERENCE:
    {
      // We hit a VEG Reference: Create a new MVVegPredicate object for it,
      // and add one representative from it to the list of this object.
      if (isOnLeftJoin())
	CMPASSERT(!isFromVegRef);
      const VEG *vegRef = ((VEGReference *)predExpr)->getVEG();
      if (parentVeg!=NULL && vegPtrList.contains(parentVeg))
      {
	// The TRUE means its a left join.
	MVVegPredicate nestedVeg(TRUE, heap, usageType_);
	// The next TRUE is because its from a VEG ref.
	nestedVeg.addVegPredicates(vegRef, TRUE, vegPtrList, heap);
	const MVVegPredicateColumn *nestedCol = 
	  nestedVeg.getRepresentativeCol();
	MVVegPredicateColumn *col = new(heap)
	  MVVegPredicateColumn(*nestedCol, heap, TRUE);
	insert(col);
      }
      else
      {
	if (parentVeg!=NULL)
	  vegPtrList.insert(parentVeg);
	addVegPredicates(vegRef, TRUE, vegPtrList, heap);
      }
    }
    break;

    case ITM_INDEXCOLUMN:
      // Skip physical columns.
      return;

    default:
      // The predicate is on an expresssion.
      NABoolean usesMultipleColumns=FALSE;
      BaseColumn *baseCol = 
	findTheBaseColumn(predExpr, &usesMultipleColumns);

      if (baseCol == NULL)
      {
        isIncremental_ = FALSE;
        return;
      }

      // A predicate such as T1.a = T2.b + T2.c is not incremental.
      if (usesMultipleColumns)
        isIncremental_ = FALSE;

      MVVegPredicateColumn *col = new(heap)
	      MVVegPredicateColumn(baseCol, 
				   isOnLeftJoin(), 
				   isFromVegRef, 
				   usageType_, 
				   predExpr,
				   heap);
      insert(col);
      break;
  }
}

//////////////////////////////////////////////////////////////////////////////
// Get one of the VEG columns, that is not complex.
//////////////////////////////////////////////////////////////////////////////
const MVVegPredicateColumn *MVVegPredicate::getRepresentativeCol() const
{
  for (CollIndex i=0; i<entries(); i++)
  {
    const MVVegPredicateColumn *currentCol = at(i);
    if (!currentCol->isComplex())
      return currentCol;
  }
  return at(0);
}

//////////////////////////////////////////////////////////////////////////////
// Used both in DDL (for creating the used columns bitmap), and in DML (for
// building the join graph).
//////////////////////////////////////////////////////////////////////////////
void MVVegPredicate::markPredicateColsOnUsedObjects(MVInfo *mvInfo, NABoolean isDDL)
{
  MVUsedObjectInfo	      *usedInfo = NULL;
  LIST(const QualifiedName *)  tableNames(NULL);
  MVColumnInfoList             mvCols(NULL);
  MVColumnInfo                *colInfo = NULL;

  // For each column in this predicate
  for (CollIndex i=0; i<entries(); i++)
  {
    const MVVegPredicateColumn *currentCol = at(i);
    // Skip complex columns
    if (currentCol->isComplex())
      continue;

    if (isDDL)
    {
      // DDL time - mark the column in the used object as used by join pred.
      usedInfo = mvInfo->findUsedInfoForTable(currentCol->getTableName());
      CMPASSERT(usedInfo!=NULL);
      usedInfo->addIndirectUpdateCol(currentCol->getColNumber());

      colInfo = 
	mvInfo->getMVColumns().getMvColInfoByBaseColumn(currentCol->getTableName(), 
	                                                currentCol->getColNumber());
      if (colInfo != NULL                            && 
	  colInfo->getColType() == COM_MVCOL_BASECOL &&
	  !mvCols.contains(colInfo) )
      {
	mvCols.insert(colInfo);
      }
    }

    // Add the table name to the list of tables on the predicate.
    tableNames.insert(&currentCol->getTableName());
  }

  // If this predicate covers more than one column in the MV select list,
  // mark the others as redundant columns
  if (mvCols.entries() > 1)
  {
    // Find the column that is either the first (smallest ordinal number)
    // or is used by the MV clustering index.
    MVColumnInfo *firstMvColInfo = mvCols[0];
    Lng32 firstMvCol = firstMvColInfo->getColNumber();
    if (!firstMvColInfo->isInMVCI())
    {
      for (CollIndex ix=1; ix<mvCols.entries(); ix++)
      {
	colInfo = mvCols[ix];
	if ((colInfo->getColNumber() < firstMvColInfo->getColNumber()) ||
	    colInfo->isInMVCI())
	  firstMvColInfo = colInfo;
      }
    }
    firstMvCol = firstMvColInfo->getColNumber();

    // Mark the rest as redundant.
    for (CollIndex jx=0; jx<mvCols.entries(); jx++)
    {
      colInfo = mvCols[jx];
      if (colInfo->getColNumber() != firstMvCol)
	colInfo->setAsRedundant(firstMvCol);
    }
  }

  // Mark the predicate for the join graph on the transitive closure, so 
  // that all the tables on the list have a marked predicate to all the 
  // others. The result is that even if the original predicates were on
  // T1.a=T2.b AND T2.b=T3.c, we will also mark T1.a=T3.c.
  for (CollIndex k=0; k<tableNames.entries(); k++)
  {
    const QualifiedName& firstName = *tableNames[k];
    for (CollIndex j=k+1; j<tableNames.entries(); j++)
    {
      const QualifiedName& secondName = *tableNames[j];
      mvInfo->getJoinGraph()->markPredicateBetween(firstName, secondName);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// If this VegPredicate is on tableName/colPosition, return its index.
// otherwise return -1.
//////////////////////////////////////////////////////////////////////////////
Lng32 MVVegPredicate::findIndexFor(const QualifiedName& tableName, 
				  Lng32                 colPosition) const
{
  for (CollIndex i=0; i<entries(); i++)
  {
    const MVVegPredicateColumn *col = at(i);
    if ((col->getTableName() == tableName) && 
	(col->getColNumber() == colPosition))
      return i;
  }
  return -1;
}

//////////////////////////////////////////////////////////////////////////////
// Return the index of some other column on this veg, that covers the one 
// on colIndex. Don't return the index of some duplicate or redundant column
// but rather the first mv column that corresponds to this base col.
//////////////////////////////////////////////////////////////////////////////
Lng32 MVVegPredicate::getCoveringColFor(Lng32 colIndex, const MVInfo *mvInfo) const
{
  for (CollIndex i=0; i<entries(); i++)
  {
    // Don't check colIndex itself.
    if ((Lng32)i == colIndex)
      continue;

    const MVVegPredicateColumn *predCol = at(i);
    const MVColumnInfo *mvCol = 
      mvInfo->getMVColumns().getMvColInfoByBaseColumn(predCol->getTableName(), 
			   		              predCol->getColNumber());
    // The predicate can be on a column not selected by the MV.
    if (mvCol == NULL)
      continue;

    // Make sure its not a duplicate or redundant column.
    if (mvCol->getColType() == COM_MVCOL_BASECOL)
      return mvCol->getColNumber();
  }
  // No covering MV column found.
  return -1;
}

//////////////////////////////////////////////////////////////////////////////
// Override LIST::insert(), in order to mark the usage type.
//////////////////////////////////////////////////////////////////////////////
void MVVegPredicate::insert(const MVVegPredicateColumn *newCol)
{
  if (usageType_ == COM_UNKNOWN_USAGE)
    usageType_ = newCol->getUsageType();

  LIST(const MVVegPredicateColumn *)::insert(newCol);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
// Exclude from coverage testing - Debugging code
void MVVegPredicate::print(FILE* ofd, const char* indent, const char* title) const
{
  fprintf(ofd, "\tVegPredicate:");
  if (usageType_ == COM_DIRECT_USAGE)
    fprintf(ofd, "(Dir) ");
  else
    fprintf(ofd, "(Exp) ");
  for (CollIndex i=0; i<entries(); i++)
    at(i)->print(ofd, indent, title);
  fprintf(ofd, ".\n");
}

void MVVegPredicate::display() const
{
  print();
}
#endif

//============================================================================
//========================  class MVUsedObjectInfo ===========================
//============================================================================

//////////////////////////////////////////////////////////////////////////////
// DDL Ctor for a used table (or MV).
// markKeyCols is TRUE for MJVs.
//////////////////////////////////////////////////////////////////////////////
MVUsedObjectInfo::MVUsedObjectInfo(const Scan  *scanNode, 
				   NABoolean    markKeyCols,
				   CollHeap    *heap)
  : objectName_(scanNode->getTableName().getQualifiedNameObj(),heap),
    objectType_(COM_BASE_TABLE_OBJECT),
    isUsedDirectly_(TRUE),
    usedColumnList_(heap),
    indirectUpdateCols_(heap),
    MVColsReferencingTheCI_(heap),
    ordinalNumber_(-1),
    isInnerTableOfLeftJoin_(FALSE),
    colNameMap_(heap),
    selectionPredicates_("", heap)
{
  TableDesc *tableDesc = scanNode->getTableDesc();
  if (tableDesc->getNATable()->isAnMV())
    objectType_ = COM_MV_OBJECT;

  // The normalizer have aleady computed the minimal outputs of this Scan 
  // node - all the columns that are really used by this MV.
  const ValueIdSet& outputs = scanNode->getGroupAttr()->getCharacteristicOutputs();
  for (ValueId vid = outputs.init();  outputs.next(vid); outputs.advance(vid) ) 
  {
    ItemExpr *expr = vid.getItemExpr();
    LIST(BaseColumn*) usedBaseCols(heap);

    // If the ValueId of the output is of an expression - find all the base
    //  columns that it uses.
    findAllTheUsedColumns(expr, tableDesc, usedBaseCols);
    for (CollIndex colNum=0; colNum<usedBaseCols.entries(); colNum++)
    {
      BaseColumn *baseCol = usedBaseCols[colNum];
      addUsedColumn(baseCol->getColNumber());
    }
  }

  // This is an MJV, mark the clustering index cols as indirect update cols.
  if (markKeyCols)
  {
    const ValueIdList &clusteringKeyCols =
      tableDesc->getClusteringIndex()->getIndexKey();
    for (CollIndex k= 0; k<clusteringKeyCols.entries(); k++)
    {
      Lng32 colPosition = 
	clusteringKeyCols[k].getNAColumn()->getPosition();

      addIndirectUpdateCol(colPosition);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// DDL Ctor for a used view
//////////////////////////////////////////////////////////////////////////////
MVUsedObjectInfo::MVUsedObjectInfo(const RenameTable *viewNode, CollHeap *heap)
  : objectName_(viewNode->getTableName().getQualifiedNameObj(),heap),
    objectType_(COM_VIEW_OBJECT),
    isUsedDirectly_(TRUE),
    usedColumnList_(heap),
    indirectUpdateCols_(heap),
    MVColsReferencingTheCI_(heap),
    ordinalNumber_(-1),
    isInnerTableOfLeftJoin_(FALSE),
    colNameMap_(heap),
    selectionPredicates_("", heap)
{
}

//////////////////////////////////////////////////////////////////////////////
// DDL Ctor for a used UDF
//////////////////////////////////////////////////////////////////////////////
MVUsedObjectInfo::MVUsedObjectInfo(const OptUDFInfo *udf, CollHeap *heap)
  : objectName_(udf->getUDFName(),heap),
    objectType_(COM_USER_DEFINED_ROUTINE_OBJECT),
    internalObjectNameForAction_(udf->getInternalObjectNameForAction()),
    isUsedDirectly_(TRUE),
    usedColumnList_(heap),
    indirectUpdateCols_(heap),
    MVColsReferencingTheCI_(heap),
    ordinalNumber_(-1),
    isInnerTableOfLeftJoin_(FALSE),
    colNameMap_(heap),
    selectionPredicates_("", heap)
{
}

NABoolean MVUsedObjectInfo::operator==(const MVUsedObjectInfo &other) const
{
  if(&other == this)
    return TRUE;
  else
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Returns TRUE if the selection predicates on a Scan node includes
// non-repeatable expressions (so the MV is not incremental).
// Mark columns used by the selection predicates as indirectUpdate columns.
//////////////////////////////////////////////////////////////////////////////
NABoolean MVUsedObjectInfo::processSelectionPredicates(const ItemExpr *expr,
						       BindWA	      *bindWA,
						       NABoolean& toBeRemoved)
{
  if (expr==NULL)
    return FALSE;

  ValueIdSet leafs;
  expr->getLeafValueIds(leafs);
  // Check each of the leaves of this selection predicate.
  for (ValueId leaf= leafs.init();  leafs.next(leaf); leafs.advance(leaf) ) 
  {
    ItemExpr *leafExpr = leaf.getItemExpr();
    BaseColumn *baseCol = NULL;

    switch (leafExpr->getOperatorType())
    {
      case ITM_BASECOLUMN:
        baseCol = (BaseColumn *)leafExpr;
        break;

      case ITM_VEG_REFERENCE:
	// Dig into another level of normalizer VEGs.
        baseCol = findTheBaseColumn(leafExpr); 
        break;

      case ITM_VEG_PREDICATE:
      {
	ValueIdSet vegMembers(((VEGPredicate *)leafExpr)->getVEG()->getAllValues());
	if (processSelectionPredicates(vegMembers, bindWA))
	  return TRUE;
	if (vegMembers.entries() <= 1)
	  toBeRemoved = TRUE;
        break;
      }

      case ITM_INDEXCOLUMN:
	// Ignore Index columns.
	toBeRemoved = TRUE;
	break;

      default:
	// It's a built-in function - is it incremental?
	if (isANonRepeatableExpression(leafExpr, bindWA))
	  return TRUE;
        break;
    }

    // Mark that this column is used by a join predicate, and is therefore
    // an indirect update column.
    if (baseCol != NULL)
    {
      if (getObjectName() ==
	  baseCol->getTableDesc()->getCorrNameObj().getQualifiedNameObj())
        addIndirectUpdateCol(baseCol->getColNumber());
      else
	toBeRemoved = TRUE;
    }
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Returns TRUE if the selection predicates on a Scan node includes
// non-repeatable expressions.
// Mark columns used by the selection predicates as indirectUpdate columns.
//////////////////////////////////////////////////////////////////////////////
NABoolean MVUsedObjectInfo::processSelectionPredicates(ValueIdSet&  expr, 
						       BindWA      *bindWA)
{
  // For each ValueId in the ValueIdSet
  for (ValueId vid = expr.init(); expr.next(vid); expr.advance(vid) ) 
  {
    NABoolean toBeRemoved = FALSE;
    if (processSelectionPredicates(vid.getItemExpr(), bindWA, toBeRemoved))
      return TRUE;

    if (toBeRemoved)
      expr.remove(vid);
  }

  return FALSE;
}

// ---------------------------------------------------------------------
// Display/print, for debugging.
// ---------------------------------------------------------------------
#ifndef NDEBUG
// Exclude from coverage testing - Debugging code
void MVUsedObjectInfo::print( FILE* ofd,	  // = stdout,
			      const char* indent, // = DEFAULT_INDENT,
			      const char* title   // = "MVInfo"
			    ) const
{
  const char *isInner = (isInnerTableOfLeftJoin() ? "LeftInner" : "Normal");

  const char *usageTypeText;
  switch(getCatmanFlags().getUsageType())
  {
    case COM_USER_SPECIFIED : usageTypeText = "UserSpecified"; break;
    case COM_DIRECT_USAGE   : usageTypeText = "Direct"; break;
    case COM_EXPANDED_USAGE : usageTypeText = "Expanded"; break;
    default	            : usageTypeText = "Unknown Usage Type"; break;
  }

  const char *objectTypeText;
  switch(objectType_)
  {
    case COM_BASE_TABLE_OBJECT           : objectTypeText = "Table"; break;
    case COM_MV_OBJECT	                 : objectTypeText = "MV"; break;
    case COM_VIEW_OBJECT                 : objectTypeText = "View"; break;
    case COM_USER_DEFINED_ROUTINE_OBJECT : objectTypeText = "UDF"; break;
    default		                 : objectTypeText = "Unknown Object Type"; break;
  }

  fprintf(ofd, "\tTable no. %d: %s, %s, %s, %s\n\t  Used columns: ", 
	  getOrdinalNumber(),
	  getObjectName().getQualifiedNameAsString().data(),
	  isInner, usageTypeText, objectTypeText);
  for (CollIndex i=0; i<usedColumnList_.entries(); i++)
    fprintf(ofd, " %d", usedColumnList_[i]);
  fprintf(ofd, "\n\t  Indirect Update columns: ");
  for (CollIndex j=0; j<indirectUpdateCols_.entries(); j++)
    fprintf(ofd, " %d", indirectUpdateCols_[j]);
  fprintf(ofd, "\n\t  MV Cols Referencing The CI: ");
  for (CollIndex k=0; k<MVColsReferencingTheCI_.entries(); k++)
    fprintf(ofd, " %d", MVColsReferencingTheCI_[k]);
  fprintf(ofd, ".\n");

  if (getSelectionPredicates() != "")
    fprintf(ofd, "\t  Selection Predicates: %s.\n", getSelectionPredicates().data());
}

void MVUsedObjectInfo::display() const 
{ 
  print(); 
}
#endif

//============================================================================
//========================  class MVColumnInfo ===============================
//============================================================================

//////////////////////////////////////////////////////////////////////////////
// This is the main Ctor for MV columns specified by the user.
//////////////////////////////////////////////////////////////////////////////
MVColumnInfo::MVColumnInfo (MVInfoForDDL&     mvInfoObj, 
			    ComMVType	      mvType,
			    const ColumnDesc *colDesc, 
			    BindWA	     *bindWA,
			    CollHeap	     *heap)
  : colNumber_(mvInfoObj.getNextColIndex()),
    colName_(colDesc->getColRefNameObj().getColName(), heap),
    colDataType_(NULL),
    colType_(COM_MVCOL_UNKNOWN),
    operatorType_(NO_OPERATOR_TYPE),
    isSystem_(FALSE),
    normalizedColText_("", heap),
    dep1_(-1), dep2_(-1), dep3_(-1),
    usageType_(COM_DIRECT_USAGE),
    origTableName_(heap),
    origColNumber_(-1),
    isComplex_(FALSE),
    baseColHashKey_("", heap),
    exprTextForHash_("", heap),
    unBoundText_(mvInfoObj.getUnBoundTextFor(colNumber_)),
    colExpr_(NULL),
    isNotNull_(FALSE),
    isUsed_(FALSE),
    isInMVCI_(FALSE),
    isIndirectUpdate_(FALSE)
{
  CMPASSERT(mvType != COM_MV_UNKNOWN);

  ValueId directColValueId = colDesc->getValueId(); 

  // Get the expression for the column in the direct select list.
  ItemExpr *expr = directColValueId.getItemExpr();

  // Get the Data type after the Cast.
  colDataType_ = &expr->getValueId().getType();
  setNotNullFrom(expr);

  // COM_MV_OTHER means not incremental. It is not an error. Must set
  // the data type correctly before returning.
  if (mvType == COM_MV_OTHER)
    return;

  // don't skip cast if it is of the form "cast(<col-name> as <type>) as <new-col-name>"
  // otherwise the target type will be lost
  if (!((expr->getOperatorType() == ITM_CAST) && (expr->child(0)->getOperatorType() == ITM_BASECOLUMN)))
  {
     // Then skip the Cast
     expr = SkipCast(expr);
     if (expr->getOperatorType() == ITM_INSTANTIATE_NULL)
       expr = expr->child(0);
  }

  operatorType_ = expr->getOperatorType();

  if (mvType == COM_MAV || mvType == COM_MAJV)
    expr = findMavColumnType(expr, mvInfoObj);

  setColTypeAndExpr(expr, mvType, directColValueId, bindWA);

  // Check if the column expression is complex.
  isComplex_ = isColumnExpressionComplex(expr);

  // Complex columns should have the normalized text filled in.
  if (isComplex_ && normalizedColText_=="")
    calcNormalizedTextForComplexCols(mvInfoObj, directColValueId);

  // Find the base column, and check that there is only one.
  NABoolean moreThanOne=FALSE;
  BaseColumn *origCol = findTheBaseColumn(expr, &moreThanOne);
  if (moreThanOne)
    origCol = NULL;
  if (origCol!=NULL)
  {
    origColNumber_ = origCol->getColNumber();
    origTableName_ = 
      origCol->getTableDesc()->getCorrNameObj().getQualifiedNameObj();
  }

  // baseColHashKey_ is an output parameter.
  calcBaseColHashKey(origTableName_, origColNumber_, baseColHashKey_);

}

//////////////////////////////////////////////////////////////////////////////
// Ctor for MAV dependent system columns. See the Internal Refresh document
// for detailed description of which dependent columns are added.
// Dependent columns can be either COUNT or SUM.
//////////////////////////////////////////////////////////////////////////////
MVColumnInfo::MVColumnInfo (const MVColumnInfo& other, 
			    MVInfoForDDL&	mvInfoObj,
			    MVAggFunc		aggIndex, 
			    ItemExpr	       *colExpr,
			    CollHeap	       *heap)
  : colNumber_(mvInfoObj.getNextColIndex()),
    colName_("", heap),
    colDataType_(other.colDataType_),
    colType_(COM_MVCOL_AGGREGATE),
    operatorType_(NO_OPERATOR_TYPE),
    isSystem_(TRUE),
    normalizedColText_("", heap),
    dep1_(-1), dep2_(-1), dep3_(-1),
    usageType_(COM_DIRECT_USAGE),
    origTableName_(other.origTableName_, heap),
    origColNumber_(other.origColNumber_),
    isComplex_(colExpr->getOperatorType() != ITM_BASECOLUMN),
    baseColHashKey_("", heap),
    aggIndex_(aggIndex),
    colExpr_(colExpr),
    isNotNull_(other.isNotNull_),
    exprTextForHash_("", heap),
    unBoundText_(other.unBoundText_),
    isUsed_(FALSE),
    isInMVCI_(FALSE),
    isIndirectUpdate_(FALSE)
{
  // Update the aggregate name and operator type according to the aggregate.
  NAString aggName;
  switch (aggIndex)
  {
    case AGG_COUNT:
      operatorType_ = ITM_COUNT_NONULL;
      aggName = "COUNT";
      if (colDataType_->getTypeQualifier() != NA_NUMERIC_TYPE)
      {
	// This is a dependent COUNT for MIN or MAX on a character column. 
	// The COUNT column type must be a numeric type.
	colDataType_ = new(heap) 
	  SQLLargeInt(heap, TRUE, FALSE); // LARGEINT SIGNED NOT NULL.
      }
      break;

    case AGG_SUM:
      operatorType_ = ITM_SUM;
      aggName = "SUM";
      break;

    default:
      CMPASSERT(FALSE);
  }

  // create a new system column name: SYS_<aggregate name><running number>
  mvInfoObj.NewSystemColumName(aggName, colName_);

  // Create the normalized test.
  CMPASSERT(colExpr != NULL);
  colExpr->unparse(exprTextForHash_, PARSER_PHASE, MV_SHOWDDL_FORMAT);
  normalizedColText_ = aggName + "(" + exprTextForHash_ + ")";
  setNotNullFrom(colExpr);

  // Update the added system column text.
  // Use the unbound text if it exists, and if there are no views
  // that mask the base table. The check on isComplex is to avoid calling
  // isMaskedTable on an empty table name.
  NAString textForAllSysColsText;
  if (unBoundText_ == NULL || 
      !mvInfoObj.isMaskedTable(origTableName_))
    textForAllSysColsText = normalizedColText_;
  else
    textForAllSysColsText = aggName + "(" + *unBoundText_ + ")";

  mvInfoObj.addSystemColumnText(getColName(), textForAllSysColsText);
}

//////////////////////////////////////////////////////////////////////////////
// Ctor for MAV GroupBy columns.
//////////////////////////////////////////////////////////////////////////////
MVColumnInfo::MVColumnInfo (MVInfoForDDL&   mvInfoObj,
			    ItemExpr	   *groupingCol,
			    CollHeap	   *heap)
  : colNumber_(mvInfoObj.getNextColIndex()),
    colName_("", heap),
    colType_(COM_MVCOL_GROUPBY),
    colDataType_(NULL),
    operatorType_(NO_OPERATOR_TYPE),
    isSystem_(TRUE),
    normalizedColText_("", heap),
    dep1_(-1), dep2_(-1), dep3_(-1),
    usageType_(COM_DIRECT_USAGE),
    origTableName_("", heap),
    origColNumber_(-1),
    isComplex_(FALSE),
    baseColHashKey_("", heap),
    aggIndex_(AGG_OTHER),
    isNotNull_(FALSE),
    exprTextForHash_("", heap),
    unBoundText_(NULL),
    isUsed_(FALSE),
    isInMVCI_(FALSE),
    isIndirectUpdate_(FALSE)
{
  CMPASSERT(groupingCol != NULL);

  NAString textForAllSysColsText;
  NAString baseColName;
  BaseColumn *baseCol;
  groupingCol->unparse(normalizedColText_);

  // Is this column seen through a view?
  const ColRefName *viewColName = 
    mvInfoObj.findViewNameFor(groupingCol->getValueId());
  if (viewColName == NULL)
  {
    // No view - use the base column name.
    if (groupingCol->getOperatorType() != ITM_BASECOLUMN)
    {
      // It's a complex expression - find the base column.
      isComplex_ = TRUE;
      while ( (groupingCol!=NULL) && 
              (groupingCol->getOperatorType() != ITM_BASECOLUMN) )
	groupingCol = groupingCol->child(0);

      // There must be at least one.
      CMPASSERT(groupingCol!= NULL && 
	        groupingCol->getOperatorType() == ITM_BASECOLUMN);
    }
    baseCol = (BaseColumn *)groupingCol;
    baseColName = baseCol->getColName();
    textForAllSysColsText = normalizedColText_;
  }
  else
  {
    // Use the view column name instead.
    baseColName = viewColName->getColName();
    textForAllSysColsText = viewColName->getColRefAsString();

    baseCol = findTheBaseColumn(groupingCol);
    if (groupingCol->getOperatorType() != ITM_BASECOLUMN)
      isComplex_ = TRUE;
  }

  // create a new system column name: SYS_<col name><running number>
  mvInfoObj.NewSystemColumName(baseColName, colName_);
  setNotNullFrom(groupingCol);

  origTableName_ = baseCol->getTableDesc()->getCorrNameObj().getQualifiedNameObj(),
  origColNumber_ = baseCol->getColNumber();

  colDataType_ = &groupingCol->getValueId().getType();
  operatorType_ = ITM_BASECOLUMN;
  colExpr_ = SkipCast(groupingCol);

  mvInfoObj.addSystemColumnText(getColName(), textForAllSysColsText);
}

//////////////////////////////////////////////////////////////////////////////
// Ctor for MJV clustering key system added columns.
//////////////////////////////////////////////////////////////////////////////
MVColumnInfo::MVColumnInfo (MVInfoForDDL&   mvInfoObj,
			    NAColumn	   *clusteringKeyCol,
			    CollHeap	   *heap)
  : colNumber_(mvInfoObj.getNextColIndex()),
    colName_("", heap),
    colType_(COM_MVCOL_BASECOL),
    colDataType_(NULL),
    operatorType_(NO_OPERATOR_TYPE),
    isSystem_(TRUE),
    normalizedColText_("", heap),
    dep1_(-1), dep2_(-1), dep3_(-1),
    usageType_(COM_DIRECT_USAGE),
    origTableName_("", heap),
    origColNumber_(-1),
    isComplex_(FALSE),
    baseColHashKey_("", heap),
    aggIndex_(AGG_OTHER),
    isNotNull_(FALSE),
    exprTextForHash_("", heap),
    unBoundText_(NULL),
    isUsed_(FALSE),
    isInMVCI_(FALSE),
    isIndirectUpdate_(FALSE)
{
  // At least one of groupingCol or clusteringKeyCol must be NULL.
  CMPASSERT(clusteringKeyCol != NULL);
  NAString textForAllSysColsText;

  const ColRefName& baseCol = clusteringKeyCol->getFullColRefName();
  const ColRefName& viewCol = 
	  mvInfoObj.findTopViewOf(baseCol);

  // create a new system column name: SYS_<col name><running number>
  mvInfoObj.NewSystemColumName(viewCol.getColName(), colName_);
  textForAllSysColsText = viewCol.getColRefAsAnsiString();
  normalizedColText_ = clusteringKeyCol->getFullColRefNameAsAnsiString();
  colDataType_ = clusteringKeyCol->getType();

  origTableName_ = *clusteringKeyCol->getTableName();
  origColNumber_ =  clusteringKeyCol->getPosition();
  calcBaseColHashKey(origTableName_, origColNumber_, baseColHashKey_);

  if (!mvInfoObj.isLeftJoinInnerTable(origTableName_))
    setNotNull(TRUE);

  operatorType_ = ITM_BASECOLUMN;

  mvInfoObj.addSystemColumnText(getColName(), textForAllSysColsText);
}

//////////////////////////////////////////////////////////////////////////////
// Ctor for the COUNT(*) system added column.
//////////////////////////////////////////////////////////////////////////////
MVColumnInfo::MVColumnInfo (MVInfoForDDL&   mvInfoObj,
			    CollHeap	   *heap)
  : colNumber_(mvInfoObj.getNextColIndex()),
    colName_("", heap),
    colDataType_(NULL),
    operatorType_(NO_OPERATOR_TYPE),
    isSystem_(TRUE),
    normalizedColText_("", heap),
    dep1_(-1), dep2_(-1), dep3_(-1),
    usageType_(COM_DIRECT_USAGE),
    origTableName_("", heap),
    origColNumber_(-1),
    isComplex_(FALSE),
    baseColHashKey_("", heap),
    aggIndex_(AGG_OTHER),
    isNotNull_(FALSE),
    exprTextForHash_("", heap),
    unBoundText_(NULL),
    isUsed_(FALSE),
    isInMVCI_(FALSE),
    isIndirectUpdate_(FALSE)
{
  NAString textForAllSysColsText;

  // create a new system column name: SYS_COUNTSTAR<running number>
  mvInfoObj.NewSystemColumName(NAString("COUNTSTAR"), colName_);
  colDataType_ = new(heap) 
    SQLLargeInt(heap, TRUE, FALSE);  // LARGEINT SIGNED NO NULLS

  setNotNull(TRUE);
  colType_ = COM_MVCOL_AGGREGATE;
  operatorType_ = ITM_COUNT;
  textForAllSysColsText = "COUNT(*)";
  aggIndex_ = AGG_COUNTSTAR;

  mvInfoObj.addSystemColumnText(getColName(), textForAllSysColsText);
}

//////////////////////////////////////////////////////////////////////////////
// Ctor for link only columns (duplicate, redundant or complex).
//////////////////////////////////////////////////////////////////////////////
MVColumnInfo::MVColumnInfo (Int32 		 colNum,
			    const QualifiedName& origTableName,
			    Int32 		 origColNumber,
			    ComMVColType	 colType,
			    CollHeap		*heap )
  : colNumber_(colNum),
    colName_("", heap),
    colDataType_(NULL),
    colType_(colType), 
    operatorType_(NO_OPERATOR_TYPE),
    isSystem_(FALSE),
    normalizedColText_("", heap),
    dep1_(-1), dep2_(-1), dep3_(-1),
    usageType_(COM_DIRECT_USAGE),
    origTableName_(origTableName, heap),
    origColNumber_(origColNumber),
    isComplex_(FALSE),
    baseColHashKey_("", heap),
    isUsed_(FALSE),
    isIndirectUpdate_(FALSE),
    aggIndex_(AGG_OTHER),
    exprTextForHash_("", heap),
    unBoundText_(NULL),
    isNotNull_(FALSE),
    isInMVCI_(FALSE),
    colExpr_(NULL)
{
  calcBaseColHashKey(origTableName_, origColNumber_, baseColHashKey_);
}

//////////////////////////////////////////////////////////////////////////////
// Explicit Ctor for DML
//////////////////////////////////////////////////////////////////////////////
MVColumnInfo::MVColumnInfo (CollHeap		*heap,
			    Int32 		 colNumber,
			    const NAString&      colName,
			    ComMVColType	 colType,
			    OperatorTypeEnum	 operatorType,
			    NABoolean		 isSystem,
			    const NAString	&normalizedColText,
			    Int32 		 dep1,
			    Int32 		 dep2,
			    Int32 		 dep3,
			    ComMVSUsageType	 usageType,
			    const QualifiedName	*origTableName,
			    Int32 		 origColNumber,
			    NABoolean		 isComplex)
  : colNumber_ 		(colNumber),
    colName_ 		(colName, heap),
    colDataType_	(NULL),
    colType_ 		(colType),
    operatorType_ 	(operatorType),
    isSystem_ 		(isSystem),
    normalizedColText_ 	(normalizedColText, heap),
    dep1_ 		(dep1),
    dep2_ 		(dep2),
    dep3_		(dep3),
    usageType_		(usageType),
    origColNumber_	(origColNumber),
    origTableName_	("", heap),
    isComplex_		(isComplex),
    baseColHashKey_     ("", heap),
    isUsed_		(FALSE),
    isIndirectUpdate_	(FALSE),
    aggIndex_		(AGG_OTHER),
    exprTextForHash_	("", heap),
    unBoundText_	(NULL),
    isNotNull_		(FALSE),
    colExpr_		(NULL),
    isInMVCI_           (FALSE)
{
  if (origTableName != NULL)
  {
    origTableName_ = *origTableName;

    // baseColHashKey_ is an output parameter.
    calcBaseColHashKey(origTableName_, origColNumber_, baseColHashKey_);
  }
}

//////////////////////////////////////////////////////////////////////////////
// Copy Ctor
//////////////////////////////////////////////////////////////////////////////
MVColumnInfo::MVColumnInfo(const MVColumnInfo& other, CollHeap *heap)
 :  colNumber_(other.colNumber_),
    colName_(other.colName_, heap),
    colType_(other.colType_),
    operatorType_(other.operatorType_),
    isSystem_(other.isSystem_),
    normalizedColText_(other.normalizedColText_, heap),
    dep1_(other.dep1_),
    dep2_(other.dep2_),
    dep3_(other.dep3_),
    usageType_(other.usageType_),
    origTableName_(other.origTableName_, heap),
    origColNumber_(other.origColNumber_),
    isComplex_(other.isComplex_),
    baseColHashKey_(other.baseColHashKey_, heap),
    isUsed_(other.isUsed_),
    isIndirectUpdate_(other.isIndirectUpdate_),
    aggIndex_(other.aggIndex_),
    isNotNull_(other.isNotNull_),
    colDataType_(NULL),
    exprTextForHash_("", heap),
    unBoundText_(NULL),
    colExpr_(NULL),
    isInMVCI_(FALSE)
{
}

NABoolean MVColumnInfo::operator==(const MVColumnInfo& other) const
{
  if(&other == this)
    return TRUE;
  else
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Calculate a hash key for the orig table and column position.
// The hash key is a string consisting of <table-name>.<col-position>
// This method is defined as static because it is also called by MvColumns.
//////////////////////////////////////////////////////////////////////////////
void MVColumnInfo::calcBaseColHashKey(const QualifiedName& tableName, 
				      Lng32                 position,
				      NAString&            hashKey)
{
  // position is the number of the column in the base table. -1 is ann illegal
  // column number, and it means that the orig column field has not been 
  // initialized, so its not relevant.
  if (position == -1)
    return;

  char positionText[5]; // not expecting more than 9999 columns in one table.

  CMPASSERT(position >= 0 && position <= 9999);
  str_itoa(position, positionText);

  hashKey += tableName.getQualifiedNameAsString();
  hashKey += ".";
  hashKey += positionText;
}

//////////////////////////////////////////////////////////////////////////////
// The colExpr parameter is the select list expression for this MAV column.
// Find if this column is a group by column or an aggregate, and if it's an 
// aggregate, find out which one.
//////////////////////////////////////////////////////////////////////////////
ItemExpr *MVColumnInfo::findMavColumnType(ItemExpr *colExpr, MVInfoForDDL& mvInfoObj)
{
  ItemExpr *expr = colExpr;
  aggIndex_ = findAggrEnum(expr, mvInfoObj);
  if (aggIndex_ != AGG_OTHER)
  {
    ItemExpr *aggExpr = NULL; // used only for AGG_STDDEVW / AGG_VARIANCEW

    if (operatorType_!=ITM_STDDEV && operatorType_!=ITM_VARIANCE)
    { // It's either COUNT, SUM, AVG, MIN or MAX
      // Skip the aggregate function to get to the operand.
      expr = expr->child(0);

      // If this is an AVG(a), than what we have is SUM(a)/COUNT(a)
      // We skipped the division, now get to child(0) of the SUM.
      if (aggIndex_ == AGG_AVG)
      {
	operatorType_ = ITM_AVG;
	expr = SkipCast(expr);
	CMPASSERT(expr->getOperatorType() == ITM_SUM);
	expr = expr->child(0);
      }
    }
    else
    { // This is either STDDEV, STDDEVW, VARIANCE or VARIANCEW
      expr = expr->child(2);              // Take the COUNT child.
      expr = SkipCast(expr);
      if (aggIndex_ == AGG_STDDEVW || aggIndex_ == AGG_VARIANCEW)
      {
	// STDDEV with a weight parameter: Save the entire STDDEV expression.
	CMPASSERT(expr->getOperatorType() == ITM_SUM);
	aggExpr = colExpr;
      }
      else
      {
	CMPASSERT(expr->getOperatorType() == ITM_COUNT_NONULL);
      }
      expr = expr->child(0);		  // Skip the Aggregate
    }
    expr = SkipCast(expr);

    // Unparse the operand to get the operand text for the hash.
    colType_ = COM_MVCOL_AGGREGATE;
    expr->unparse(exprTextForHash_, PARSER_PHASE, MV_SHOWDDL_FORMAT);
    if (aggExpr != NULL)
      expr = aggExpr; // The colExpr_ saved for STDDEVW is the aggregate.
  }
  else if (expr->containsAnAggregate())
  {
    // Aggregate function is not top level function.
    colType_ = COM_MVCOL_COMPLEX;
    return expr;
  }
  else
  {
    // If it's not an aggregate, it must be a group by column.
    colType_ = COM_MVCOL_GROUPBY;
    if (expr->getOperatorType() == ITM_BASECOLUMN)
      operatorType_ = ITM_BASECOLUMN;
  }

  return expr;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
NABoolean MVColumnInfo::isColumnExpressionComplex(ItemExpr *expr)
{
  if ((colType_ == COM_MVCOL_GROUPBY) || (colType_ == COM_MVCOL_BASECOL))
  {
    return FALSE;
  }
  else
  {
    if ((aggIndex_ == AGG_OTHER)   || 
        (aggIndex_ == AGG_STDDEVW) || 
	(aggIndex_ == AGG_VARIANCEW))
    {
      // Unsupported expressions and STDDEVW / VARIANCEW are always complex.
      return TRUE;
    }
    else if (aggIndex_ == AGG_COUNTSTAR)
    {
      // Count(*) is never complex
      return FALSE;
    }
    else
    {
      // Otherwise, it's complex if its not just on a a base column.
      return (expr->getOperatorType() != ITM_BASECOLUMN);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void MVColumnInfo::calcNormalizedTextForComplexCols(MVInfoForDDL& mvInfoObj, 
			                            ValueId       directColValueId)
{
  ItemExpr *colExpr = directColValueId.getItemExpr();
  colExpr->unparse(normalizedColText_, PARSER_PHASE, QUERY_FORMAT);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void MVColumnInfo::setColTypeAndExpr(ItemExpr  *expr, 
				     ComMVType  mvType,
				     ValueId    directColValueId,
				     BindWA    *bindWA)
{
  if (mvType == COM_MJV)
  {
    aggIndex_ = AGG_OTHER;
    if (operatorType_ == ITM_BASECOLUMN)
      colType_ = COM_MVCOL_BASECOL;
    else
    {
      colType_ = COM_MVCOL_FUNCTION;
      colExpr_ = expr;
    }
  }
  else
  {
    // It's a MAV or MAJV.
    CMPASSERT(mvType == COM_MAV || mvType == COM_MAJV)

    CMPASSERT(expr != NULL);
    if (colType_ == COM_MVCOL_GROUPBY)
      colExpr_ = directColValueId.getItemExpr();
    else
      colExpr_ = SkipCast(expr);
  }

  // Non-Repeatable expressions are not incremental.
  if (colExpr_!=NULL && isNonRepeatableExpressionUsed(bindWA))
    colType_ = COM_MVCOL_OTHER; // Set as Not incremental
}

//////////////////////////////////////////////////////////////////////////////
// Set the "not null" attribute of this column according to the bound 
// expression 'expr'.
//////////////////////////////////////////////////////////////////////////////
void MVColumnInfo::setNotNullFrom(ItemExpr *expr)
{
  // For STDDEV(a), this method is passed the expression a*a, that is 
  // constructed from a. The top of the expression is not bound yet.
  // The "not null" info is only in the bound part, so skip the unbound top.
  while (!expr->nodeIsBound())
  {
    CMPASSERT(expr->child(0) != NULL);
    expr = expr->child(0);
  }

  // Check the NAType of the result.
  NABoolean supportsNull = expr->getValueId().getType().supportsSQLnull();
  setNotNull(!supportsNull);
}

//////////////////////////////////////////////////////////////////////////////
// Find which aggregate function is used. The result is in MVAggFunc, so it
// can be used as an index to an array.
//////////////////////////////////////////////////////////////////////////////
MVAggFunc MVColumnInfo::findAggrEnum(ItemExpr *expr, MVInfoForDDL& mvInfoObj)
{
  MVAggFunc result = AGG_OTHER;
  switch (expr->getOperatorType())
  {
    // Check the trivial ones first.
    case ITM_COUNT	  : result = AGG_COUNTSTAR; break;
    case ITM_COUNT_NONULL : result = AGG_COUNT;	    break;
    case ITM_SUM	  : result = AGG_SUM;	    break;
    case ITM_MIN	  : result = AGG_MIN;	    break;
    case ITM_MAX	  : result = AGG_MAX;	    break;

    case ITM_VARIANCE	  : 
      // For VARIANCE take the expression from the COUNT child.
      expr = expr->child(2);
      expr = SkipCast(expr);
      // Check if the expression is COUNT(a) than the function is VARIANCE(a).
      // If it is SUM(b) than the function is VARIANCE(a,b).
      if (expr->getOperatorType() == ITM_SUM)
	result = AGG_VARIANCEW;
      else
      {
	CMPASSERT(expr->getOperatorType() == ITM_COUNT_NONULL);
	result = AGG_VARIANCE;
      }
      break;

    case ITM_STDDEV :
      // For STDDEV take the expression from the COUNT child.
      expr = expr->child(2);
      expr = SkipCast(expr);
      // Check if the expression is COUNT(a) than the function is STDDEV(a).
      // If it is SUM(b) than the function is STDDEV(a,b).
      if (expr->getOperatorType() == ITM_SUM)
	result = AGG_STDDEVW;
      else
      {
	CMPASSERT(expr->getOperatorType() == ITM_COUNT_NONULL);
	result = AGG_STDDEV;
      }
      break;

    case ITM_DIVIDE :
      // Division is allowed only when part of an AVG.
      if (expr->origOpType() == ITM_AVG)
      {
	result = AGG_AVG;
	// Choose the first (SUM) child.
	expr = expr->child(0);
      }
      else
	result = AGG_OTHER;
      break;
  }

  if (result != AGG_OTHER)
  {
    // Make sure we have an aggregate function.
    expr = SkipCast(expr);
    CMPASSERT(expr->isAnAggregate());

    // DISTINCT is not incremental.
    if (((Aggregate *)expr)->isDistinct())
    {
      mvInfoObj.setNotIncremental(MVNI_DISTINCT);
    }
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////////
// Insert this column into the expression hash
//////////////////////////////////////////////////////////////////////////////
void MVColumnInfo::insertIntoExpHash(ExpressionHash& expHash, CollHeap *heap)
{
  // "Other" columns are not incremental and should not be added.
  if (aggIndex_ == AGG_OTHER)
    return;

  // Does the expression hash already have an entry for this expression?
  AggregateArray *aggArray;
  if (expHash.contains(&exprTextForHash_))
  {
    // Yes, retrieve it.
    aggArray = expHash.getFirstValue(&exprTextForHash_);
  }
  else
  {
    // No, allocate a new one.
    aggArray = new(heap) AggregateArray(heap, AGG_OTHER);
    // Initialize all ARRAY elements to NULL. 
    // Uninitialized elements may cause problems.
    for (Int32 i=AGG_OTHER-1; i>=0; i--)
      aggArray->insertAt(i, NULL);
    expHash.insert(&exprTextForHash_, aggArray);
  }
  // Insert this column into the array at index aggIndex_.
  aggArray->at(aggIndex_) = this;
}

//////////////////////////////////////////////////////////////////////////////
// This object is marked as redundant because there is another MV column - 
// number <firstMvCol>, (appearing before this one in the MV column list), 
// that is based on the same base column, or on a base column that is equal 
// to it because of an equal join predicate.
//////////////////////////////////////////////////////////////////////////////
void MVColumnInfo::setAsRedundant(Lng32 firstMvCol) 
{ 
  colType_ = COM_MVCOL_REDUNDANT; 
  dep1_    = firstMvCol;
}

//////////////////////////////////////////////////////////////////////////////
// We need a new column of aggregate type depIndex on expression 
// exprTextForHash. If there is one already in the expression hash, return
// its index. Otherwise construct a new MVColumnInfo object for this column,
// insert it into the expression hash, and then return its index.
//////////////////////////////////////////////////////////////////////////////
Int32 MVColumnInfo::newMavDependentColumn(ExpressionHash&     expHash, 
					const NAString&	    exprTextForHash,
					MVInfoForDDL&	    mvInfoObj,
					MVAggFunc	    depIndex, 
					ItemExpr	   *depExpr,
					CollHeap	   *heap)
{
  // Is it already in the hash?
  AggregateArray *aggArray = expHash.getFirstValue(&exprTextForHash);
  if (aggArray==NULL || aggArray->at(depIndex) == NULL)
  {
    // No, construct a new MVColumnInfo object.
    MVColumnInfo *newDep = new(heap) 
      MVColumnInfo(*this, mvInfoObj, depIndex, depExpr, heap);
    // Insert it into the MVInfo column list.
    mvInfoObj.getMVColumns().insert(newDep, mvInfoObj.isIncrementalWorkNeeded());
    // And into the expression hash.
    newDep->insertIntoExpHash(expHash, heap);
    aggArray = expHash.getFirstValue(&exprTextForHash);
    CMPASSERT(aggArray!=NULL);
  }
  // Return the index of the new column.
  return aggArray->at(depIndex)->getColNumber();
}

//////////////////////////////////////////////////////////////////////////////
// Create the dependent columns of this column. Say this column is an 
// aggregate on base column a.
// All aggregate columns except COUNT need a dependent COUNT(a) column.
// AVG(a), STDDEV(a) and VARIANCE(a) need a dependent SUM(a) column.
// STDDEV(a) and VARIANCE(a) need also SUM(a*a).
// STDDEVW(a,b) and VARIANCE(a,b) need SUM(b), SUM(a*b) and SUM(a*a*b).
// If a is NOT NULL, then COUNT(a) can be replaced by COUNT(*).
//////////////////////////////////////////////////////////////////////////////
void MVColumnInfo::createDependentColumns(ExpressionHash& expHash, 
					  MVInfoForDDL&	  mvInfoObj,
					  CollHeap	 *heap)
{
  // COUNT columns and non-incremental columns don't need dependent columns.
  if ((aggIndex_ == AGG_COUNT    ) ||
      (aggIndex_ == AGG_COUNTSTAR) ||
      (aggIndex_ == AGG_OTHER))
    return;

  // Find the aggregate array for this expression.
  AggregateArray *aggArray = expHash.getFirstValue(&exprTextForHash_);
  CMPASSERT(aggArray != NULL);  // this object should be there.
  MVAggFunc depIndex = AGG_OTHER;
  MVColumnInfo *newDep = NULL;
  NAString newExprText(heap);

  if ((aggIndex_ != AGG_STDDEVW) && (aggIndex_ != AGG_VARIANCEW))
  {
    // SUM, MIN, MAX, AVG, STDDEV and VARIANCE 
    // All have COUNT as the first dependent column.
    if (isNotNull())
    {
      // Use COUNT(*) instead of COUNT(a) when a is NOT NULL.
      dep1_ = mvInfoObj.getPosOfCountStar();
      CMPASSERT(dep1_ != -1);
    }
    else
    {
      ItemExpr *expr = getColExpr();
      if ( (expr->getOperatorType() == ITM_TIMES) &&
	   (expr->child(0)->getValueId() == expr->child(1)->getValueId()))
      {
        // This is COUNT(expr*expr) from STDDEV. Do COUNT(expr) instead.
	expr = expr->child(0);
        expr->unparse(newExprText, PARSER_PHASE, MV_SHOWDDL_FORMAT);
      }
      else
	newExprText = exprTextForHash_;

      // If COUNT(a) is not in yet - create it now.
      dep1_ = newMavDependentColumn(expHash, newExprText, 
				    mvInfoObj, AGG_COUNT, 
				    expr, heap);
    }

    // SUM, MIN and MAX only need the COUNT column. 
    if ( (aggIndex_ == AGG_SUM) ||
	 (aggIndex_ == AGG_MIN) ||
	 (aggIndex_ == AGG_MAX) )
      return;

    // AVG, STDDEV and VARIANCE have SUM(expr) as the second dep column.
    dep2_ = newMavDependentColumn(expHash, exprTextForHash_, 
				  mvInfoObj, AGG_SUM, 
				  getColExpr(), heap);
    // For AVG, COUNT and SUM are enough.
    if (aggIndex_ == AGG_AVG)
      return;	

    // STDDEV and VARIANCE have SUM(expr*expr) as the third dep column.
    depIndex = AGG_SUM;
    ItemExpr *squareExpr = new(heap) 
      BiArith(ITM_TIMES, getColExpr(), getColExpr());
    newExprText="";
    squareExpr->unparse(newExprText, PARSER_PHASE, MV_SHOWDDL_FORMAT);
    dep3_ = newMavDependentColumn(expHash, newExprText, 
				  mvInfoObj, AGG_SUM, 
				  squareExpr, heap);
  }
  else
  { 
    // STDDEVW and VARIANCEW require special treatment!
    dep3_ = newStddevwDepColumn(0, expHash, newExprText, mvInfoObj, heap);
    dep2_ = newStddevwDepColumn(1, expHash, newExprText, mvInfoObj, heap);
    dep1_ = newStddevwDepColumn(2, expHash, newExprText, mvInfoObj, heap);
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Int32 MVColumnInfo::newStddevwDepColumn(Int32             childNo,
				      ExpressionHash& expHash, 
				      const NAString& exprTextForHash,
				      MVInfoForDDL&   mvInfoObj,
				      CollHeap	     *heap)
{
  // Use the parameters of the original STDDEV / VARIANCE expression.
  // Skip all CAST operators in the expression before using unparse().
  ItemExpr *depExpr = colExpr_->child(childNo);
  depExpr = SkipCast(depExpr);
  CMPASSERT(depExpr->getOperatorType() == ITM_SUM);
  depExpr = depExpr->child(0);
  depExpr = SkipCast(depExpr);
  if (depExpr->getOperatorType() == ITM_TIMES)
  {
    depExpr->child(0) = SkipCast(depExpr->child(0));
    depExpr->child(1) = SkipCast(depExpr->child(1));
    if (depExpr->child(1)->getOperatorType() == ITM_TIMES)
    {
      depExpr->child(1)->child(0) = SkipCast(depExpr->child(1)->child(0));
      depExpr->child(1)->child(1) = SkipCast(depExpr->child(1)->child(1));
    }
  }
  // Now do the unparse() to get the hash text.
  NAString newExprText(heap);
  depExpr->unparse(newExprText, PARSER_PHASE, MV_SHOWDDL_FORMAT);
  // Create the new column - they are all SUM, just the operand differs.
  return newMavDependentColumn(expHash, newExprText, 
			       mvInfoObj, AGG_SUM, 
			       depExpr, heap);
}

//////////////////////////////////////////////////////////////////////////////
// Make sure non-repeatable functions are not used in expr.
//////////////////////////////////////////////////////////////////////////////
NABoolean MVColumnInfo::isNonRepeatableExpressionUsed(BindWA *bindWA)
{
  if (colExpr_==NULL)
    return FALSE;

  ValueIdSet leaves;
  colExpr_->getLeafValueIds(leaves);
  for (ValueId leaf= leaves.init();  leaves.next(leaf); leaves.advance(leaf) ) 
  {
    if (isANonRepeatableExpression(leaf.getItemExpr(), bindWA))
      return TRUE;
  }
  return FALSE;
}

// ---------------------------------------------------------------------
// Display/print, for debugging.
// ---------------------------------------------------------------------
#ifndef NDEBUG
// Exclude from coverage testing - Debugging code
void MVColumnInfo::print( FILE* ofd,          //  = stdout,
			  const char* indent, // = DEFAULT_INDENT,
			  const char* title
			) const
{
  const char *colTypeText;
  switch(getColType())
  {
    case COM_MVCOL_GROUPBY   : colTypeText="GroupBy";	break;
    case COM_MVCOL_CONST     : colTypeText="Const";	break;
    case COM_MVCOL_AGGREGATE : colTypeText="Aggregate";	break;
    case COM_MVCOL_OTHER     : colTypeText="Other";	break;
    case COM_MVCOL_FUNCTION  : colTypeText="Function";	break;
    case COM_MVCOL_BASECOL   : colTypeText="BaseCol";	break;
    case COM_MVCOL_DUPLICATE : colTypeText="Duplicate"; break;
    case COM_MVCOL_REDUNDANT : colTypeText="Redundant"; break;
    case COM_MVCOL_COMPLEX   : colTypeText="Complex";   break;
    case COM_MVCOL_UNKNOWN   : ;
    default		     : colTypeText="Unknown";	break;
  }
  const char *opTypeText;
  switch(getOperatorType())
  {
    case ITM_COUNT        : opTypeText="ITM_COUNT";        break;
    case ITM_COUNT_NONULL : opTypeText="ITM_COUNT_NONULL"; break;
    case ITM_SUM	  : opTypeText="ITM_SUM"; 	   break;
    case ITM_AVG          : opTypeText="ITM_AVG";          break;
    case ITM_MIN          : opTypeText="ITM_MIN";          break;
    case ITM_MAX          : opTypeText="ITM_MAX";          break;
    case ITM_STDDEV       : opTypeText="ITM_STDDEV";       break;
    case ITM_VARIANCE     : opTypeText="ITM_VARIANCE";     break;
    case ITM_BASECOLUMN   : opTypeText="ITM_BASECOLUMN";   break;
    default		  : opTypeText="Unknown";          break;
  }
  const char *isSysText   = (isSystem_   ? "TRUE" : "FALSE");
  const char *isNullText  = (isNotNull_  ? "TRUE" : "FALSE");
  const char *compText    = (isComplex_  ? "TRUE" : "FALSE");
  const char *mvciText    = (isInMVCI_   ? "TRUE" : "FALSE");
  const NAString& dataTypeText = (colDataType_==NULL ? NAString("") : colDataType_->getTypeName());
  const char *normalizedText   = (normalizedColText_=="" ? "NULL" : normalizedColText_.data());
  NAString origText = (origTableName_ == ""  ? 
			NAString("NULL")	: 
			origTableName_.getQualifiedNameAsString());

  fprintf(ofd, "%sCol %d, %s, colType %s, opType %s,\n",
	  indent, getColNumber(), colName_.data(), colTypeText, opTypeText);
  fprintf(ofd, "%s    isSystem %s, dataType: %s, isNotNull: %s, isInMVCI: %s\n",
	  indent, isSysText, dataTypeText.data(), isNullText, mvciText);
  fprintf(ofd, "%s    Deps %2d, %2d, %2d, %s\n",
	  indent, dep1_, dep2_, dep3_, normalizedColText_.data());
  fprintf(ofd, "%s    Orig: %s, col %d, complex %s, normalizedText %s\n",
	  indent, origText.data(), origColNumber_, compText, normalizedText);
  if (strcmp(title, "")) fprintf(ofd,"\n");
} // MVColumnInfo::print()

void MVColumnInfo::display() const 
{ 
  print(); 
}
#endif

//============================================================================
//==========================  class MVColumns ================================
//============================================================================

const Lng32 MVColumns::initialHashTableSize_ = 10;

//////////////////////////////////////////////////////////////////////////////
MVColumns::MVColumns(CollHeap *heap)
  : directColumnList_(heap),
    extraColumnList_(heap),
    columnInfoByNameHash_(hashKey, initialHashTableSize_, FALSE, heap),
    columnInfoByBaseHash_(hashKey, initialHashTableSize_, FALSE, heap),
    syskeyIndexCorrection_(0),
    firstSysAddedColumn_(-1),
    heap_(heap)
{
}

//////////////////////////////////////////////////////////////////////////////
MVColumns::MVColumns(CollHeap *heap, const MVColumns& other, NABoolean isIncremental)
  : directColumnList_     (heap, other.directColumnList_.entries()),
    extraColumnList_      (heap, other.extraColumnList_.entries()),
    columnInfoByNameHash_ (hashKey, initialHashTableSize_, FALSE, heap),
    columnInfoByBaseHash_ (hashKey, initialHashTableSize_, FALSE, heap),
    syskeyIndexCorrection_(other.syskeyIndexCorrection_),
    firstSysAddedColumn_  (other.firstSysAddedColumn_),
    heap_                 (heap)
{
  for (CollIndex j=0; j<other.directColumnList_.entries(); j++)
    insert(new(heap_) MVColumnInfo(*other.directColumnList_[j], heap_), isIncremental);

  for (CollIndex l=0; l<other.extraColumnList_.entries(); l++)
    insert(new(heap_) MVColumnInfo(*other.extraColumnList_[l], heap_), isIncremental);
}

//////////////////////////////////////////////////////////////////////////////
void MVColumns::insert(MVColumnInfo *colInfo, NABoolean isIncremental)
{
  if ( (directColumnList_.entries() == 0) &&
       colInfo->getColNumber()      != 0 )
  {
    // We have a SYSKEY situation: 
    // Since a SYSKEY column was added by the system to the MV table, all 
    // column numbers have been incremented by the catman. However, the 
    // catman does not provide us with an MVColumnInfo object for SYSKEY,
    // so we have to correct the index later in the code.
    // First, make sure that the first column is inserted with index 1.
    CMPASSERT(colInfo->getColNumber() == 1);

    syskeyIndexCorrection_ = -1;
  }

  // Remember the position of the first system added column.
  if (firstSysAddedColumn_ == -1 && colInfo->isSystem())
    firstSysAddedColumn_ = colInfo->getColNumber();

  // Insert the colInfo into the correct list.
  if (!isIncremental || colInfo->isARealColumn())
  {
    directColumnList_.insert(colInfo);
  }
  else
  {
    extraColumnList_.insert(colInfo);
  }

  // Update the MV name hash.
  if (colInfo->getColName() != "")
  {
    MVColumnInfo *prevCol = 
      columnInfoByNameHash_.getFirstValue(&colInfo->getColName());

    // Can't have two MV cols with the same name.
    // This error is caught later by the catman, so ignore it now.
    if (prevCol == NULL)
      columnInfoByNameHash_.insert(&colInfo->getColName(), colInfo);
  }

  // Update the base table/column name hash.
  if (colInfo->getBaseColHashKey() != "")
  {
    MVColumnInfoList *prevCols = 
      columnInfoByBaseHash_.getFirstValue(&colInfo->getBaseColHashKey());

    if (prevCols == NULL)
    {
      // This is the first MV column that corresponds to this base column.
      prevCols = new(heap_) MVColumnInfoList(heap_);
      prevCols->insert(colInfo);
      columnInfoByBaseHash_.insert(&colInfo->getBaseColHashKey(), prevCols);
    }
    else
    {
      if (colInfo->getColType() == COM_MVCOL_BASECOL)
      {
	// There already is a previous MV column that corresponds to this 
	// base column - Mark this column as redundant.
        MVColumnInfo *firstCol = prevCols->at(0);
	CMPASSERT(firstCol != NULL);
        if (firstCol->getColType() == COM_MVCOL_BASECOL)
	  colInfo->setAsRedundant(firstCol->getColNumber());
      }

      prevCols->insert(colInfo);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
ULng32 MVColumns::getTotalNumberOfColumns() const 
{ 
  return directColumnList_.entries() + extraColumnList_.entries(); 
}

//////////////////////////////////////////////////////////////////////////////
MVColumnInfo *MVColumns::getMvColInfoByName(const NAString& name) const
{
  return columnInfoByNameHash_.getFirstValue(&name);
}

//////////////////////////////////////////////////////////////////////////////
const MVColumnInfoList *MVColumns::getAllMvColsAffectedBy(const QualifiedName& tableName, Lng32 position) const
{
  NAString hashkey;
  MVColumnInfo::calcBaseColHashKey(tableName, position, hashkey);

  return columnInfoByBaseHash_.getFirstValue(&hashkey);
}

//////////////////////////////////////////////////////////////////////////////
MVColumnInfo *MVColumns::getMvColInfoByBaseColumn(const QualifiedName& tableName, Lng32 position) const
{
  const MVColumnInfoList *colInfoList = 
    getAllMvColsAffectedBy(tableName, position);
  if (colInfoList == NULL)
    return NULL;

  CMPASSERT(colInfoList->entries() > 0);

  MVColumnInfo *colInfo = NULL;

  // if one of the columns is a base column return that one
  for (CollIndex i=0; i < colInfoList->entries(); i++)
  {
     colInfo = colInfoList->at(i);
     if (colInfo->getColType() == COM_MVCOL_BASECOL)
       return colInfo;
  }
  
  colInfo = colInfoList->at(0);
  CMPASSERT(colInfo != NULL);

  if (colInfo->getColType() != COM_MVCOL_REDUNDANT)
    return colInfo;

  // If the column is redundant, there is another MV column that corresponds
  // to this base column, and its index was saved in the dep1_ field.
  CMPASSERT(colInfo->getDepCol1() != -1);
  return getMvColInfoByIndex(colInfo->getDepCol1());
}

//////////////////////////////////////////////////////////////////////////////
MVColumnInfo *MVColumns::getMvColInfoByBaseColumn(const QualifiedName& tableName, 
                                                  Lng32 baseColPosition, Lng32 mvColPosition) const
{
  const MVColumnInfoList *colInfoList = getAllMvColsAffectedBy(tableName, baseColPosition);
  if (colInfoList == NULL)
    return NULL;

  CMPASSERT(colInfoList->entries() > 0);

  MVColumnInfo *MVBaseColInfo = NULL;
  MVColumnInfo *colInfo = NULL;

  if (colInfoList != NULL)
  {
     for (CollIndex i=0; !colInfo && (i < colInfoList->entries()); i++)
     {
        MVBaseColInfo = colInfoList->at(i);
        if (MVBaseColInfo && (MVBaseColInfo->getColType() == COM_MVCOL_REDUNDANT))
            MVBaseColInfo = getMvColInfoByIndex(MVBaseColInfo->getDepCol1());
        if (MVBaseColInfo && (MVBaseColInfo->getColNumber() == (Int32)mvColPosition))
            colInfo = MVBaseColInfo;
     }
   }

  return colInfo;
}

//////////////////////////////////////////////////////////////////////////////
MVColumnInfo *MVColumns::getMvColInfoByIndex(Lng32      index, 
					     NABoolean adjustIndex) const
{
  CMPASSERT(index >= 0);
  Lng32 origIndex = index;

  if (adjustIndex)
  {
    index += syskeyIndexCorrection_;
    CMPASSERT(index >= 0);
  }

  MVColumnInfo *colInfo;
  // Get the column from the direct + extra lists.
  CMPASSERT(index < (Lng32)(entries() + extraColumnList_.entries()));
  if (index < (Lng32)entries())
    colInfo = directColumnList_[index];
  else
    colInfo = extraColumnList_[index - entries()];

  // If the index was adjusted, and its a real column (not in the extra list)
  // verify that the column returned indeed has this index number.
  if (adjustIndex && index < (Lng32)entries())
    CMPASSERT(colInfo->getColNumber() == origIndex);

  return colInfo;
}

//////////////////////////////////////////////////////////////////////////////
// When the name of a column is changed (when the optional column name list
// is provoded), the columnInfoByNameHash_ needs to be updated as well.
//////////////////////////////////////////////////////////////////////////////
void MVColumns::setNewColumnName(MVColumnInfo *colInfo, const NAString& newName)
{
  const NAString& oldName = colInfo->getColName();

  // Remove the colInfo from the hash table 
  // Don't check if there actually was such a column by that name in there
  // because its OK if there isn't - it might have already been removed.
  columnInfoByNameHash_.remove(&oldName);

  // Change thye name of the column.
  colInfo->setNewColumnName(newName);

  // Now re-enter it using the new name.
  columnInfoByNameHash_.insert(&colInfo->getColName(), colInfo);
}

//////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
// Exclude from coverage testing - Debugging code
void MVColumns::print( FILE* ofd,          //  = stdout,
		       const char* indent, // = DEFAULT_INDENT,
		       const char* title
		     ) const
{
  fprintf(ofd, "    Direct Column list:\n");
  for (CollIndex i=0; i<directColumnList_.entries(); i++)
    directColumnList_[i]->print(ofd, CONCAT(indent,"\t"),"");

  if (!extraColumnList_.isEmpty())
  {
    fprintf(ofd, "    Extra Column list:\n");
    for (CollIndex i=0; i<extraColumnList_.entries(); i++)
      extraColumnList_[i]->print(ofd, CONCAT(indent,"\t"),"");
  }

} // MVColumns::print()

void MVColumns::display() const 
{ 
  print(); 
}
#endif

//============================================================================
//===========================  class MVInfo ==================================
//============================================================================

//////////////////////////////////////////////////////////////////////////////
// Minimal Ctor for DDL.
// Initialize only what is known from the CREATE MV syntax.
//////////////////////////////////////////////////////////////////////////////
MVInfo::MVInfo( const NAString&   nameOfMV,
		ComMVRefreshType  refreshType,
		NABoolean	  isRewriteEnabled,
		ComMVStatus	  mvStatus,
		const NAString&   mvSelectText,
		CharInfo::CharSet mvSelectTextCharSet,
		CollHeap	 *heap):
  nameOfMV_(nameOfMV, heap),
  refreshType_(refreshType),
  isRewriteEnabled_(isRewriteEnabled),
  mvStatus_(mvStatus),
  mvSelectText_(mvSelectText, heap),
  mvSelectTextCharSet_(mvSelectTextCharSet),
  isInitialized_(COM_MVSTATUS_INITIALIZED == mvStatus || COM_MVSTATUS_NO_INITIALIZATION == mvStatus ),
  isIncremental_(TRUE),
  isMinMaxUsed_(FALSE),
  mvType_(COM_MV_UNKNOWN),
  usedObjectsList_(heap),
  columnList_(heap),
  eqPredicateList_(heap),
  posOfCountStar_(-1),
  joinGraph_(NULL),
  heap_(heap)
{
}	



//////////////////////////////////////////////////////////////////////////////
// Detailed Ctor for DML
// All the data is read from the SMD.
//////////////////////////////////////////////////////////////////////////////
MVInfo::MVInfo( const NAString&             nameOfMV,
		ComMVRefreshType	    refreshType,
		NABoolean		    isRewriteEnabled,
		ComMVStatus		    mvStatus,
		const NAString&		    mvSelectText,
		CharInfo::CharSet	    mvSelectTextCharSet,
		NABoolean		    isIncremental,
		ComMVType		    mvType,
		LIST (MVUsedObjectInfo *)&  usedObjectsList,
		MVColumnInfoList&	    directColumnList,
		NABoolean		    isLeftLinear,
		LIST (MVVegPredicate *)&    eqPredicateList,
		CollHeap		   *heap) :
    nameOfMV_		(nameOfMV, heap),
    refreshType_	(refreshType),
    isRewriteEnabled_	(isRewriteEnabled),
    mvStatus_		(mvStatus),
    mvSelectText_	(mvSelectText, heap),
    mvSelectTextCharSet_(mvSelectTextCharSet),
    isInitialized_	(COM_MVSTATUS_INITIALIZED == mvStatus || COM_MVSTATUS_NO_INITIALIZATION == mvStatus),
    isIncremental_	(isIncremental),
    isMinMaxUsed_	(FALSE),
    mvType_		(mvType),
    usedObjectsList_	(usedObjectsList, heap),
    columnList_         (heap),
    eqPredicateList_	(eqPredicateList, heap),
    posOfCountStar_	(-1),
    joinGraph_		(NULL),
    heap_		(heap)
{ 
  // Find the index of the COUNT(*) column, and if Min/Max columns are used.
  for (CollIndex i=0; i<directColumnList.entries(); i++)
  {
    switch (directColumnList[i]->getOperatorType())
    {
      case ITM_COUNT: posOfCountStar_ = i;
		      break;
      case ITM_MIN  :
      case ITM_MAX  : isMinMaxUsed_ = TRUE;
		      break;
    }

    MVColumnInfo *colInfo = directColumnList[i];
    columnList_.insert(directColumnList[i], isIncremental);
  }
}

//////////////////////////////////////////////////////////////////////////////
// Copy Ctor
// heap==NULL => copy heap from other. Otherwise, override heap pointer.
// heap_ is defined as the first data member in the class definition, so it
// is initialized first, and used by the other data members.
//////////////////////////////////////////////////////////////////////////////
MVInfo::MVInfo(const MVInfo &other, CollHeap *heap)
 :  nameOfMV_		(other.nameOfMV_, heap_),
    refreshType_	(other.refreshType_),
    isRewriteEnabled_	(other.isRewriteEnabled_),
    mvStatus_		(other.mvStatus_),
    mvSelectText_	(other.mvSelectText_, heap),
    mvSelectTextCharSet_(other.mvSelectTextCharSet_),
    isInitialized_	(other.isInitialized_),
    isIncremental_	(other.isIncremental_),
    isMinMaxUsed_	(other.isMinMaxUsed_),
    mvType_		(other.mvType_),
    usedObjectsList_	(heap_, other.usedObjectsList_.entries()),
    columnList_         (heap_, other.columnList_, other.isIncremental_),
    eqPredicateList_	(heap_, other.eqPredicateList_.entries()),
    posOfCountStar_	(other.posOfCountStar_),
    joinGraph_		(NULL),
    heap_		(heap==NULL ? other.heap_ : heap)
{
  // Copy the elements of all the lists.
  for (CollIndex i=0; i<other.usedObjectsList_.entries(); i++)
    usedObjectsList_.insert(new(heap_) 
      MVUsedObjectInfo(*other.usedObjectsList_[i], heap_));

  for (CollIndex l=0; l<other.eqPredicateList_.entries(); l++)
    eqPredicateList_.insert(new(heap_) 
      MVVegPredicate(*other.eqPredicateList_[l], heap_));
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
MVInfo::~MVInfo()
{
  delete joinGraph_;
}

//////////////////////////////////////////////////////////////////////////////
// Get the list of base columns of tableName used by this MV.
//////////////////////////////////////////////////////////////////////////////
const LIST(Lng32)& MVInfo::getUsedColumns(const QualifiedName& tableName) const
{
  MVUsedObjectInfo *usedInfo = findUsedInfoForTable(tableName);
  CMPASSERT(usedInfo!=NULL);
  return usedInfo->getUsedColumnList();
}

//////////////////////////////////////////////////////////////////////////////
// Get the list of indirect update columns of tableName used by this MV.
//////////////////////////////////////////////////////////////////////////////
const LIST(Lng32)& MVInfo::getIndirectUpdateColumns(const QualifiedName& tableName) const
{
  MVUsedObjectInfo *usedInfo = findUsedInfoForTable(tableName);
  CMPASSERT(usedInfo!=NULL);
  return usedInfo->getIndirectUpdateCols();
}

//////////////////////////////////////////////////////////////////////////////
// Make sure that the RelRoot at the top of the MV select tree has a select
// list with RenameCol objects, to rename the MV columns to their MV names.
// This is needed when the MV is defined without an explicit column name list.
//////////////////////////////////////////////////////////////////////////////
void MVInfo::fixMvSelectList(RelRoot *mvSelectTree) const
{
  // Get the list of MV columns.
  const MVColumns& mvColumns = getMVColumns();

  // Take the curent select list from the root.
  ItemExpr *selectExprList = mvSelectTree->removeCompExprTree();
  RelRoot* alternateRoot = NULL;

  // When the query is SELECT DISTINCT ...
  // The top RelRoot has no select list. Its located on another RelRoot node
  // below the Scalar Aggregate.
  if (selectExprList == NULL)
  {
    if (mvSelectTree->child(0)->getOperatorType() == REL_GROUPBY)
    {
      GroupByAgg* gb = (GroupByAgg*)mvSelectTree->child(0).getPtr();
      if (gb->getGroupExprTree()->getOperatorType() == ITM_REFERENCE)
      {
	ColReference* groupExpr = (ColReference*)gb->getGroupExprTree();
	if (groupExpr->getColRefNameObj().isStar() &&
      	    gb->child(0)->getOperatorType() == REL_ROOT)
	{
	  return;
	  //alternateRoot = (RelRoot*)gb->child(0).getPtr();
	  //selectExprList = alternateRoot->removeCompExprTree();
	}
      }
    }
    // Can't handle an empty select list for now.
    return;
  }

  // Transform it to an actual list.
  ItemExprList selectList(selectExprList, getHeap());

  const CorrName mvCorrName(getNameOfMV(), getHeap());
  CollIndex mvColIndex = 0;
  for (CollIndex selectListIndex=0; 
       selectListIndex < selectList.entries(); 
       selectListIndex++, mvColIndex++)
  {
    const MVColumnInfo *mvCol = mvColumns[mvColIndex];
    const NAString& mvColName = mvCol->getColName();
    ItemExpr *colExpr = selectList[selectListIndex];

    // No need to change this column's name, if it already has a RenameCol
    // to the correct MV column name of top of it. '*' are not renamed either.
    NABoolean needToAddRename = TRUE;
    if (colExpr->getOperatorType() == ITM_RENAME_COL)
    {
      const ColRefName *origColName = ((RenameCol*)colExpr)->getNewColRefName();
      if (origColName->getColName() == mvColName)
	needToAddRename = FALSE;
    }
    else if (colExpr->getOperatorType() == ITM_REFERENCE)
    {
      const ColRefName& origColName = ((ColReference*)colExpr)->getColRefNameObj();
      if (origColName.isStar())
	needToAddRename = FALSE;
    }
    else if (colExpr->getOperatorType() == ITM_USER_DEF_FUNCTION)
    {
      needToAddRename = FALSE;
    }
    if (!needToAddRename)
      continue;

    // Build a new RenameCol on top of the existing expression.
    ItemExpr *newColExpr = new(getHeap()) 
      RenameCol(colExpr, 
                new(getHeap()) ColRefName(mvColName, mvCorrName));

    // Replace the select list expression with the new one.
    selectList[selectListIndex] = newColExpr;
  }

  // Transform the select list back to an ItemExpr tree.
  ItemExpr *newSelectExprList = selectList.convertToItemExpr();

  // Replace the original select list with the fixed one.
  if (alternateRoot != NULL)
    alternateRoot->addCompExprTree(newSelectExprList);
  else
    mvSelectTree->addCompExprTree(newSelectExprList);
}

//----------------------------------------------------------------------------
// This method is used to get the parsed MV select text.
RelRoot *MVInfo::buildMVSelectTree(const NAString* alternativeText, CharInfo::CharSet textCharSet) const
{
  const NAString& MVSelectText = 
    (alternativeText!=NULL ? *alternativeText : getMVSelectText());
  if (alternativeText == NULL) textCharSet = getMVSelectTextCharSet();
  ExprNode *parsedNode;

  // Parse the SQL text.
  Parser parser(CmpCommon::context());
  if (parser.parseDML(MVSelectText.data(), MVSelectText.length(), textCharSet, &parsedNode))
    return NULL;

  // Skip the DDL nodes of the CREATE MV command.
  DDLExpr *parseTree = 
    (DDLExpr *)((StmtNode *)parsedNode->castToStatementExpr())->
      getQueryExpression()->getChild(0);
  CMPASSERT(parseTree->getOperatorType() == REL_DDL); 
  CMPASSERT(parseTree->getDDLNode()->getOperatorType()==DDL_CREATE_MV);
  StmtDDLCreateMV *createMvNode = 
    (StmtDDLCreateMV*)parseTree->getDDLNode();

  RelRoot *mvSelectTree = (RelRoot *)createMvNode->getQueryExpression();
  mvSelectTree->setRootFlag(FALSE);

  // If needed, rename the MV columns to their MV names (needed for MVs 
  // using an explicit column name list).
  fixMvSelectList(mvSelectTree);

  return mvSelectTree;
}

//////////////////////////////////////////////////////////////////////////////
// Find an equal predicate keyColis part of.
//////////////////////////////////////////////////////////////////////////////
Lng32 MVInfo::findEqPredicateCovering(NAColumn *keyCol)
{
  // Iterate over all equal predicates
  for (CollIndex i=0; i<eqPredicateList_.entries(); i++)
  {
    MVVegPredicate *pred = eqPredicateList_[i];

    // Ignore left join predicates.
    if (pred->isOnLeftJoin())
      continue;

    // Is our column used by this predicate?
    Lng32 predIndex = 
      pred->findIndexFor(*keyCol->getTableName(), keyCol->getPosition());
    if (predIndex != -1)
    {
      // Yes - Find the covering base column
      Lng32  coveringCol = pred->getCoveringColFor(predIndex, this);
      return coveringCol;
    }
  }

  return -1;
}	

//////////////////////////////////////////////////////////////////////////////
// Is there an equal predicate between these two columns?
//////////////////////////////////////////////////////////////////////////////
NABoolean MVInfo::isEqPredicateBetween(const QualifiedName& table1, Lng32 col1,
 				       const QualifiedName& table2, Lng32 col2) const
{
  // Iterate over all equal predicates
  for (CollIndex i=0; i<eqPredicateList_.entries(); i++)
  {
    MVVegPredicate *pred = eqPredicateList_[i];

    // Ignore left join predicates.
    if (pred->isOnLeftJoin())
      continue;

    // Does this predicate include both of the columns?
    if ( (pred->findIndexFor(table1, col1) != -1) &&
	 (pred->findIndexFor(table2, col2) != -1) )
      return TRUE;
  }
  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Construct a new join graph with initial size.
//////////////////////////////////////////////////////////////////////////////
void MVInfo::newJoinGraph(Int32 initialSize)
{
  joinGraph_ = new(heap_) MVJoinGraph(initialSize, heap_);
}

//////////////////////////////////////////////////////////////////////////////
// Return the MVUsedObjectInfo object of the table tableName used by this MV.
//////////////////////////////////////////////////////////////////////////////
MVUsedObjectInfo *MVInfo::findUsedInfoForTable(const QualifiedName& tableName) const
{
  for (CollIndex i=0; i<usedObjectsList_.entries(); i++)
    if (usedObjectsList_[i]->getObjectName() == tableName)
      return usedObjectsList_[i];
  
  return NULL;
}

//////////////////////////////////////////////////////////////////////////////
// Recursive method used by areTablesUsedOnlyOnce().
// Adds all the names of the used objects of mvInfo to usedObjectsNames.
// Return TRUE if the tables are used only once.
// On the second recursion level, the method is called on an MVInfoForDML
// object, so it cannot be a method of MVInfoForDDL even though it is used
// at DDL time only.
//////////////////////////////////////////////////////////////////////////////
NABoolean MVInfo::addTableNamesToList(BindWA        *bindWA, 
				      TableNameHash& usedObjectsNames)
{
  // For eacvh of the used objects
  const LIST (MVUsedObjectInfo*)& usedObjects = getUsedObjectsList();
  for (CollIndex objIdx=0; objIdx<usedObjects.entries(); objIdx++)
  {
    const MVUsedObjectInfo *usedInfo = usedObjects[objIdx];

    // If the list already contains this table, abort.
    if (usedObjectsNames.contains(&usedInfo->getObjectName()))
      return FALSE;

    // Add the table name to the list.
    usedObjectsNames.insert(&usedInfo->getObjectName(), usedInfo);

    // Continue the recursion for used MVs.
    if (!usedInfo->isAnMV())
      continue;

    // Get the MVInfo of the used MV.
    CorrName usedMVName(usedInfo->getObjectName());

    // Avoid lookup to parNameLocList for used MV names.
    ParNameLocList *saveNameLocList = bindWA->getNameLocListPtr();
    bindWA->setNameLocListPtr(NULL); 

    // Get the MVInfo of the used MV.
    MVInfoForDML *usedObjMVInfo = 
      bindWA->getNATable(usedMVName)->getMVInfo(bindWA);
    bindWA->setNameLocListPtr(saveNameLocList);

    // Recursivly add the MVs used objects to the list.
    if (usedObjMVInfo->addTableNamesToList(bindWA, usedObjectsNames) == FALSE)
    {
      // An object is used more than once, abort.
      return FALSE;
    }
  }

  return TRUE;
}


// ---------------------------------------------------------------------
// Display/print, for debugging.
// ---------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
// Exclude from coverage testing - Debugging code
void MVInfo::print( FILE* ofd,		// = stdout,
		    const char* indent, // = DEFAULT_INDENT,
		    const char* title   // = "MVInfo"
		  ) const
{
  const char *mvTypeText;
  switch (mvType_)
  {
    case COM_MAV	: mvTypeText = "MAV";    break;
    case COM_MJV	: mvTypeText = "MJV";    break;
    case COM_MAJV	: mvTypeText = "MAJV";   break;
    case COM_MV_OTHER   : mvTypeText = "Other";  break;
    case COM_MV_UNKNOWN : ;
    default		: mvTypeText = "Unknown";break;
  }
  
  const char *refreshTypeText;
  switch (refreshType_)
  {
    case COM_ON_STATEMENT : refreshTypeText = "on Statement"; break;
    case COM_ON_REQUEST	  : refreshTypeText = "on Request";   break;
    case COM_BY_USER	    : refreshTypeText = "by User";      break;
    case COM_UNKNOWN_RTYPE: ;
    default		  : refreshTypeText = "Unknown";   break;
  }
  
  const char *rewriteText = (isRewriteEnabled_ ? "Enabled" : "Disabled");
  const char *initText    = (isInitialized_    ? "TRUE"    : "FALSE");
  const char *incrText    = (isIncremental_    ? "TRUE"    : "FALSE");

  fprintf(ofd, "\n\nMV Name: %s,\n    Refresh %s, Rewrite %s, Initialized %s,\n",
	  getNameOfMV().data(), refreshTypeText, rewriteText, initText);
  fprintf(ofd, "    incremental %s, MVType %s\n",	incrText, mvTypeText);

  // columnLists
  columnList_.print(ofd, indent);

  // usedObjectsList_
  fprintf(ofd, "\n    Used objects list:\n");
  for (CollIndex j=0; j<usedObjectsList_.entries(); j++)
  {
    const MVUsedObjectInfo *usedObj = usedObjectsList_[j];
    usedObj->print(ofd, CONCAT(indent,"\t"),"");
  }

  // eqPredicateList_
  fprintf(ofd, "\n    EQ Join predicates:\n");
  for (CollIndex k=0; k<eqPredicateList_.entries(); k++)
  {
    eqPredicateList_[k]->display();
  }

  if (strcmp(title, "")) fprintf(ofd,"\n");
} // MVInfo::print()

void MVInfo::display() const 
{ 
  print(); 
}
#endif

// - functions used in showddl
NAString MVInfo::getMVRefreshTypaAsString() const
{
  NAString result(heap_);
  switch(refreshType_)
  {
  case COM_ON_STATEMENT:
    result = "REFRESH ON STATEMENT ";
    break;
  case COM_ON_REQUEST:
    result = "REFRESH ON REQUEST ";
    break;
  case COM_RECOMPUTE:
    result = "RECOMPUTE ";
    break;
  case COM_BY_USER:
    result = "REFRESH BY USER ";
    break;
  default:
    CMPASSERT(FALSE);
  }
  return result;
} // MVInfo::getMVRefreshTypaAsString

NAString MVInfo::getIgnoreChangesListAsString(StmtDDLCreateMV *createMvNode) const
{
  NAString result(heap_);
  ElemDDLCreateMVOneAttributeTableList *icList = createMvNode->getIgnoreChangesList();
  // if no ignore changes specified , return.
  if(!icList)
  {
    return result;
  }

  result += "IGNORE CHANGES ON ";
  QualifiedName icTableName = icList->getFirstTableInList();

  NABoolean keepGoing = TRUE;
  while(keepGoing) 
  {
    keepGoing = FALSE;
    result += icTableName.getQualifiedNameAsAnsiString();
    if(icList->listHasMoreTables()) 
    {
      result += ", ";
      icTableName = icList->getNextTableInList();
      keepGoing = TRUE;
    }
  }
  result += "\n  ";
  return result;
} // MVInfo::getIgnoreChangesList

NAString MVInfo::getIgnoreChangesListAsString() const
{
  NAString result(heap_);
  NABoolean seenFirst = FALSE;

  const LIST (MVUsedObjectInfo*)& usedObjects = getUsedObjectsList();

  for (CollIndex ci = 0 ; ci < usedObjects.entries() ; ci++)
  {
    MVUsedObjectInfo* usedInfo = usedObjects[ci];

    MVUsedObjectCatmanFlags&  catmanFlags = usedInfo->getCatmanFlags();

    if ((catmanFlags.getUsageType() == COM_USER_SPECIFIED) &&
        (usedInfo->getObjectType() == COM_BASE_TABLE_OBJECT) &&
        (catmanFlags.getObjectAttributes() == COM_IGNORE_CHANGES))
    {
      if (!seenFirst)
      {
        result += "IGNORE CHANGES ON ";
        result += usedInfo->getObjectName().getQualifiedNameAsAnsiString();
        seenFirst = TRUE;
      }
      else
      {
        result += ", ";
        result += usedInfo->getObjectName().getQualifiedNameAsAnsiString();
      }
    }
  }

  if (seenFirst)
    result += "\n ";

  return result;
}

NAString MVInfo::getInitializationAsString() const
{
  NAString result(heap_);
  switch(mvStatus_) 
  {
  case COM_MVSTATUS_INITIALIZED:
    result = "INITIALIZE ON CREATE ";
    break;
  case COM_MVSTATUS_NOT_INITIALIZED:
    result = "INITIALIZE ON REFRESH ";
    break;
  case COM_MVSTATUS_NO_INITIALIZATION:
    result = "NO INITIALIZATION ";
    break;
  default:
    // this function should be called only when the
    // MV is available.
    CMPASSERT(FALSE);
  }
  return result;
} // MVInfo::getInitializationAsString()

NAString MVInfo::getOptionalNameClause(StmtDDLCreateMV *createMvNode, 
				       const NAString& mvText,
				       CharInfo::CharSet & mvTextCharSet /* inout param */ ) const
{
  CMPASSERT(mvTextCharSet == CharInfo::UTF8);
  NAString opNameClause(heap_);
  const ElemDDLColViewDefArray& defArray = createMvNode->getMVColDefArray();

  const StringPos firstOpenPar = mvText.first('(');
  
  // if optional-name clause is not specified, return.
  // this happens if there is no open paranthesis in the CREATE MV statement
  // or if the first patanthesis is beyond the end of the optional name clause.
  if(NA_NPOS == firstOpenPar ||
    createMvNode->getEndOfOptionalColumnListPosition() < firstOpenPar)
  {
    return opNameClause;
  }

  // The optional names clause starts at the first '('. We already have a pointer 
  // to where it ends.
  opNameClause = mvText;
  opNameClause = 
    opNameClause.remove(createMvNode->getEndOfOptionalColumnListPosition() + 1);
  StringPos start = createMvNode->getStartPosition();

  opNameClause = opNameClause.remove(start, opNameClause.first('(') - start);
  
  // The optional names clause contains system added columns 
  // that need to be cut out.
  const MVColumns& mvCols = getMVColumns();
  for (CollIndex ci(0); ci < mvCols.entries() ; ci++)
  {
    const MVColumnInfo *mvCol = mvCols[ci];
    if (!mvCol->isSystem())
    { // skip columns that are not system added. 
      continue;
    }
    StringPos sysAddedNamePos = opNameClause.index(mvCol->getColName());
    // take out the system added column
    opNameClause = opNameClause.remove(sysAddedNamePos, mvCol->getColName().length());
  }
  
  // removing trailing ',' and empty spaces
  StringPos lastLetterPos = opNameClause.last(')');
  while(opNameClause[lastLetterPos] == ')' ||
	opNameClause[lastLetterPos] == ' ' ||
	opNameClause[lastLetterPos] == ','   )
  {
    opNameClause = opNameClause.remove(lastLetterPos);
    lastLetterPos--;
  }
  opNameClause += ')';

  return opNameClause;
}

// getSelectPhraseAsString :
// This function builds the select-list from the mvSelectTree
// it won't show system added columns.
NAString MVInfo::getSelectPhraseAsString(NAString& sysAddedCols) const 
{
  NAString result(heap_);
  NAString columnList(" SELECT ", heap_);
  
  RelRoot *mvSelectTree = buildMVSelectTree();
  const MVColumns& mvCols = getMVColumns();
  
  // extract the list of columns from the tree.
  CMPASSERT(mvSelectTree);
  ItemExpr *selectExprList = mvSelectTree->removeCompExprTree();
  
  // Transform it to an actual list.
  ItemExprList selectList(selectExprList, heap_);
  
  CollIndex mvColIndex = 0;
  for (CollIndex selectListIndex=0; 
  selectListIndex < selectList.entries(); 
  selectListIndex++, mvColIndex++)
  {
    CMPASSERT(mvColIndex <  mvCols.entries());
    const MVColumnInfo *mvCol = mvCols[mvColIndex];
    const NAString& mvColName = mvCol->getColName();
    ItemExpr *colExpr = selectList[selectListIndex];
    NAString origColName("", heap_);
    
    switch(colExpr->getOperatorType())
    {
    case ITM_RENAME_COL:
      origColName = ((RenameCol*)colExpr)->getNewColRefName()->getColName();
      break;
    case ITM_REFERENCE:
      origColName = ((ColReference*)colExpr)->getColRefNameObj().getColName();
      break;
    default:
      CMPASSERT(FALSE);
    }
    
    result = ""; // clearing result, so unparse won't concatinate its output with the old one
    colExpr->unparse(result, PARSER_PHASE, MV_SHOWDDL_FORMAT);

    // If the column has a renaming, write it
    if (ITM_RENAME_COL == colExpr->getOperatorType())
    {
      result += " AS ";
      result += ToAnsiIdentifier(origColName);  
    }

    if(mvCol->isSystem())
    {
      sysAddedCols += "--  ";
      sysAddedCols += result;
      sysAddedCols += "\n";
      continue;
    } 
    else 
    {
      columnList += result;
    }
    
    columnList += ", ";
  } // for
  columnList = columnList.remove(columnList.last(','));
  
  // "AS" prepending "SELECT..."
  columnList.prepend("\n  AS");
  return columnList;
} // MVInfo::getSelectPhraseAsString

NAString MVInfo::getTextForShowddl(CatROMV *pMV, CharInfo::CharSet & showDDLoutputCharSet /* out param */ ) const
{
  return "";
} // MVInfo::getTextForShowddl

//============================================================================
//===========================  class MVInfoForDDL ============================
//============================================================================

// Initialization of a static const data member.
const char MVInfoForDDL::systemAddedColNamePrefix_[] = "SYS_";

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
MVInfoForDDL::MVInfoForDDL(const NAString		      &nameOfMV,
			   ComMVRefreshType		      refreshType,
			   NABoolean			      isRewriteEnabled,
			   ComMVStatus			      mvStatus,
			   const NAString&		      mvSelectText,
			   CharInfo::CharSet		      mvSelectTextCharSet,
			   const ElemDDLStoreOptKeyColumnList *userSpecifiedCI,
			   CollHeap			      *heap) :
    MVInfo(nameOfMV, refreshType, isRewriteEnabled, mvStatus, mvSelectText, mvSelectTextCharSet, heap),
    scanNodes_(heap),
    joinNodes_(heap),
    groupByNodes_(heap),
    colDescList_(NULL),
    rootNode_(NULL),
    sysColCounter_(0),
    isLeftLinear_(TRUE),
    sysColNames_("", heap),
    sysColExprs_("", heap),
    textForSysCols_("", heap),
    userColumnCount_(-1),
    usesOtherMVs_(FALSE),
    viewColumnGraph_(NULL),
    leftJoinInnerTables_(heap),
    usedViewsList_(heap),
    unBoundColumnsText_(heap),
    realColumnCount_(0),
    userSpecifiedCI_(NULL),
    mvDescriptor_(NULL),
    xmlText_(NULL),
    notIncrementalReason_(MVNI_DEFAULT),
    udfList_(heap)
{
  //
  if(NULL != userSpecifiedCI)
  {
    userSpecifiedCI_ = new (heap) ElemDDLColRefArray(heap);
    *userSpecifiedCI_ = userSpecifiedCI->getKeyColumnArray();
  }
}
//////////////////////////////////////////////////////////////////////////////
MVInfoForDDL::~MVInfoForDDL()
{
  //
  if(NULL != userSpecifiedCI_)
  {
    delete userSpecifiedCI_;
  }
}

//////////////////////////////////////////////////////////////////////////////
// Add a Scan node to the list. 
// This is the source of the used object information.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addScanNode(Scan *newNode) 
{ 
  scanNodes_.insert(newNode); 

  if (newNode->getTableDesc()->getNATable()->isAnMV())
    usesOtherMVs_ = TRUE;

  // Is there a correlation name to this table?
  if (newNode->getTableName().getCorrNameAsString() == "")
    return;

  // Yes - treat it as a view.
  if (viewColumnGraph_ == NULL)
    viewColumnGraph_ = new(getHeap()) ViewColumnGraph(getHeap());

  // The TRUE is for adding system columns as well as user columns.
  viewColumnGraph_->addTableMapping(newNode->getRETDesc(), TRUE, getHeap());
}

//////////////////////////////////////////////////////////////////////////////
// Add a Join node to the list. 
// This is the source of the equal join predicate information.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addJoinNode(Join *newNode)
{ 
  joinNodes_.insert(newNode); 
}

//////////////////////////////////////////////////////////////////////////////
// Add a GroupBy node to the list. 
// This is the source of the grouping columns information.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addGroupByNode(GroupByAgg *newNode)
{
  groupByNodes_.insert(newNode); 
}

//////////////////////////////////////////////////////////////////////////////
// Collect information from a view's RenameTable node.
// This information is used for building the view graph, and for adding 
// views as used objects.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addViewRenameNode(RenameTable *renameNode)
{
  // In a view, the node below the RenameTable is a RelRoot.
  // don't mix with UnPackColumns etc.
  if (renameNode->child(0)->getOperatorType() != REL_ROOT)
    return;

  // Nested selects are added to the view graph, but are not used objects.
  if (renameNode->isView())
  {
    // MV on View is not incremental until we finish fixing the bugs.
    setNotIncremental(MVNI_ON_VIEW);

/*
    // Don't bother with views for non incremental MVs.
    if (isIncrementalWorkNeeded() == FALSE)
      return;
*/

    // Add the view as a used object.
    MVUsedObjectInfo *usedObject = new(getHeap())
      MVUsedObjectInfo(renameNode, getHeap());
   
    NABoolean isUsedObjectInList = FALSE;
      
    // and the used object is not already in the list
    for( CollIndex i = 0; i < usedViewsList_.entries();i++ )
    {
      MVUsedObjectInfo *currentObject = (MVUsedObjectInfo*)usedViewsList_[i];

      // if the object is already in the list, don't add it again
      if( currentObject->getObjectName() == usedObject->getObjectName() )
      {
        isUsedObjectInList = TRUE;
        break;
      }        
    }

    if( ! isUsedObjectInList )
    {
      usedViewsList_.insert(usedObject);
    }
  }

  // Add the information to the view graph.
  if (viewColumnGraph_ == NULL)
    viewColumnGraph_ = new(getHeap()) ViewColumnGraph(getHeap());

  // The FALSE is for adding user columns only (without system columns).
  viewColumnGraph_->addTableMapping(renameNode->getRETDesc(), FALSE, getHeap());
}

//////////////////////////////////////////////////////////////////////////////
// The unbound information is collected from the root select list of the 
// parse tree before binding, and used when adding new system added columns.
//////////////////////////////////////////////////////////////////////////////
const NAString *MVInfoForDDL::getUnBoundTextFor(CollIndex index) const 
{ 
  if (index < unBoundColumnsText_.entries())
    return unBoundColumnsText_[index]; 
  else
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////
// Uses MJVIndexBuilder algorithm to compute the minimal set of 
// MJV secondary indices from the base tables' CIs
//////////////////////////////////////////////////////////////////////////////
LIST(MVColumnInfoList*)* 
MVInfoForDDL::getOptimalMJVIndexList (const LIST(Lng32)* mvCI) 
{
  // create index builder
  MJVIndexBuilder indexBuilder (getHeap());
  IndexList inputRCIList(getHeap()), *outputRCIList;

  // Insert MVCI as the first element of the inputRCIList
  inputRCIList.insert(*mvCI);

  // get RCIs for all component tables
  LIST (MVUsedObjectInfo *)& compObjects = getUsedObjectsList();
  size_t objNum = compObjects.entries();
  size_t ittr;
  ColIndList currRCI;
  for (ittr = 0; ittr < objNum; ittr++)
  {
    MVUsedObjectInfo *currObject = compObjects.at(ittr);

    // only consider those tables that are not in the IGNORE CHANGES clause
    // and whose INSERTLOG attribute is not set
    const NABoolean isNotInIgnoreChanges =
      isUsedObjectNotIgnoreChanges (currObject->getObjectName());
    const NABoolean isNotInsertLog = !currObject->getCatmanFlags().getIsInsertLog();

    if (isNotInsertLog && isNotInIgnoreChanges) 
    {
       currRCI = currObject->getMVColsReferencingTheCI();
       if (currRCI.entries() > 0) 
       {
         inputRCIList.insert(currRCI);
       }
    }
  }
  // run builder over the input
  outputRCIList = indexBuilder.buildIndex(inputRCIList);
  // build list of lists of MVColumnInfo-s corresponding 
  // to the result column numbers
  LIST(MVColumnInfoList*) *outColumnInfoLists = 
	      new LIST(MVColumnInfoList*)(getHeap());
  objNum = outputRCIList->entries();
  
  for(ittr = 0; ittr < objNum; ittr++)
  {
    currRCI = outputRCIList->at(ittr);
    outColumnInfoLists->insert(buildColumnInfoListFromColIndList(currRCI));
  }
  delete outputRCIList;
  return outColumnInfoLists;
}

// This function builds a ColumnInfoList out of ColIndList which is a list
// of column numbers.
MVColumnInfoList* 
MVInfo::buildColumnInfoListFromColIndList(const ColIndList &currentRCI) const
{
  CollIndex currentColNumber;
  MVColumnInfoList *currentColumnInfoList = new (getHeap())
           MVColumnInfoList(getHeap());
  size_t currentRCIsize = currentRCI.entries();
  for(size_t i(0); i < currentRCIsize; i++) 
  {
    currentColNumber = currentRCI.at(i);
    MVColumnInfo *currentColInfo = 
      getMVColumns().getMvColInfoByIndex(currentColNumber);
    currentColumnInfoList->insert(currentColInfo);
  }
  return currentColumnInfoList;
}

//////////////////////////////////////////////////////////////////////////////
// 
// MVInfoForDDL::setNotIncremental
//
// Parameters:
//   MV_NOT_INCREMENTAL notIncrementalReason 
//      - this parameter is used to describe the reason why we're setting
//          this to not incremental
//
//////////////////////////////////////////////////////////////////////////////
void 
MVInfoForDDL::setNotIncremental(const MV_NOT_INCREMENTAL notIncrementalReason) 
{ 
  // make sure reason is valid
  CMPASSERT( isValidNotIncrementalReason( notIncrementalReason ) );

  setIsIncremental(FALSE); 

  // update the reason why we're setting this to notIncremental
  notIncrementalReason_ = notIncrementalReason;
}
//////////////////////////////////////////////////////////////////////////////
// The unbound information is collected from the root select list of the 
// parse tree before binding, and used when adding new system added aggregate
// dependent columns.
// This text is obtained by using unparse() on the operand of the aggregate 
// function.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::extractParsedColumnText(Lng32 colIndex, ItemExpr *colExpr)
{
  // Skip any renames to get to the aggregate.
  while (colExpr->getOperatorType() == ITM_RENAME_COL)
    colExpr = colExpr->child(0);

  // Collect unbound text for aggregate functions only.
  if (colExpr->getOperator().match(ITM_ANY_AGGREGATE))
  {
    // Get to the aggregate function operand.
    ItemExpr *op = colExpr->child(0);
    NAString *colText = new(getHeap()) NAString(getHeap());
    // Unparse from ItemExpr to text.
    op->unparse(*colText, PARSER_PHASE, MV_SHOWDDL_FORMAT);
    // Insert to the list.
    unBoundColumnsText_.insertAt(colIndex, colText);
  }
  else
    unBoundColumnsText_.insertAt(colIndex, NULL);
}

//////////////////////////////////////////////////////////////////////////////
// Save the root node of the MV select tree.
// The select list of this root is the source of the MV column list.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::setRootNode(RelRoot *rootNode, BindWA *bindWA)
{
  rootNode_ = rootNode;

  CMPASSERT(!rootNode->nodeIsBound());

  // Take the select list.
  ItemExpr *selectItemList = rootNode->getCompExprTree();
  if (selectItemList == NULL)
  {
    // When the query is SELECT DISTINCT ...
    // The top RelRoot has no select list. Its located on another RelRoot node
    // below the Scalar Aggregate.
      if (rootNode->child(0)->getOperatorType() == REL_GROUPBY)
    {
      GroupByAgg* gb = (GroupByAgg*)rootNode->child(0).getPtr();
      if (gb->getGroupExprTree()->getOperatorType() == ITM_REFERENCE)
      {
	ColReference* groupExpr = (ColReference*)gb->getGroupExprTree();
	if (groupExpr->getColRefNameObj().isStar() &&
      	    gb->child(0)->getOperatorType() == REL_ROOT)
	{
      	  setNotIncremental(MVNI_DISTINCT);
      	  return;

	  // Once we support SELECT DISTINCT, uncomment this code.
	  //RelRoot* alternateRoot = (RelRoot*)gb->child(0).getPtr();
	  //selectItemList = alternateRoot->removeCompExprTree();
	}
      }
    }
  }

  // Try again...
  if (selectItemList == NULL)
  {
    // The select list mut have at least one column.
    setNotIncremental(MVNI_NO_COLUMNS);
    return;
  }

  // Get the unbound column text.
  ItemExprList selectList(selectItemList, getHeap());
  for (CollIndex colNumber=0; colNumber<selectList.entries(); colNumber++)
    extractParsedColumnText(colNumber, selectList[colNumber]);
}

//////////////////////////////////////////////////////////////////////////////
// Construct a new name for a system added column, in the format:
// SYS_<midText><running number>
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::NewSystemColumName(const NAString& midText, 
				      NAString& colNameString)
{
  // For itoa(), Not expecting < maxColumnsPerTableSupported_ system added cols.
  char number[5];
  NAString  colName("");

  // Iterate until we construct a column name that is not used yet.
  do
  {
    sysColCounter_++;
    sprintf( number, "%d", sysColCounter_ );

    // Construct the name: SYS_<midText><running number>
    colName = systemAddedColNamePrefix_;
    colName.append(midText);
    colName.append(&number[0]);

  } while (getMVColumns().getMvColInfoByName(colName) != NULL);

  colNameString = colName;
}

//////////////////////////////////////////////////////////////////////////////
// A system-added column was just added to the MV.
// Update the two test strings for the added columns accordingly.
// See examples below - the " characters show what is added here.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addSystemColumnText(const NAString& colName, 
				       const NAString& colExp)
{
  // This is the column names list: CREATE MV mv1(col1, col2", col3"...)
  NAString &allSysColNames = getSysColNames();
  allSysColNames.append(", ");
  allSysColNames.append(ToAnsiIdentifier(colName));

  // This is the actual select list: AS SELECT ... ",COUNT(a) AS SYS_COUNT1"
  NAString &allSysColsText = getTextForSysCols();
  allSysColsText.append(", ");
  allSysColsText.append(colExp);
  allSysColsText.append(" AS ");
  allSysColsText.append(ToAnsiIdentifier(colName));

  // This text is used for binding expanded trees - just the expression text.
  NAString &allSysColExprs = getSysColExprs();
  allSysColExprs.append(", ");
  allSysColExprs.append(colExp);
}

//////////////////////////////////////////////////////////////////////////////
// Look up colName in the view graph, see how it is called from the top 
// most view. Return colName if the column is not found in the view graph.
//////////////////////////////////////////////////////////////////////////////
const ColRefName& MVInfoForDDL::findTopViewOf(const ColRefName& colName) const
{
  if (viewColumnGraph_ == NULL)
    return colName;
  else
    return viewColumnGraph_->findTopViewOf(colName);
}

//////////////////////////////////////////////////////////////////////////////
// Access the view graph by column ValueId.
// Return NULL if the column is not found in the view graph.
//////////////////////////////////////////////////////////////////////////////
const ColRefName *MVInfoForDDL::findViewNameFor(const ValueId vid) const
{
  if (viewColumnGraph_ == NULL)
    return NULL;
  else
    return viewColumnGraph_->findViewNameFor(vid);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
NABoolean MVInfoForDDL::isMaskedTable(const QualifiedName& tableName) const
{
  if (viewColumnGraph_ == NULL)
    return FALSE;
  else
    return viewColumnGraph_->isMaskedTable(tableName);
}

//////////////////////////////////////////////////////////////////////////////
// Called from processNormalizedInformation().
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::initUsedObjectsList(BindWA *bindWA)
{
  // The set of NATable pointers is used to detect if a table is used more
  // than once.
  SET(const NATable *) naTables(getHeap()); 

  CollIndex scanNodesNumber = scanNodes_.entries();

  // Prevents calling newJoinGraph() with zero scan nodes which
  // would eventually lead to an assert when calling createHashTable(0),
  // whereby hashSize=0.
  if (scanNodesNumber < 1)
  {
     NAString reason ("No source tables specified");
     *CmpCommon::diags() << DgSqlCode(-12018)
                         << DgString0(reason);
     bindWA->setErrStatus();
     return;
  }

  // Construct the join graph.
  newJoinGraph(scanNodesNumber);

  // For each Scan node on the tree
  for (CollIndex scanNodeIndex=0; 
       scanNodeIndex < scanNodesNumber; 
       scanNodeIndex++)
  {
    Scan *scanNode = scanNodes_[scanNodeIndex];

    // Use the NATable to verify single use only.
    const NATable *naTable = scanNode->getTableDesc()->getNATable();
    if (naTables.contains(naTable))
    {
      // The same table is used more than once.
      setNotIncremental(MVNI_TABLE_REUSED);
      continue;
    }
    naTables.insert(naTable);

    // Build a new used object.
    MVUsedObjectInfo *usedInfo = initUsedObject(scanNode, naTable, bindWA);
    if (bindWA->errStatus())
      return;

    // Insert into the used objects list.
    if (usedInfo != NULL)
      getUsedObjectsList().insert(usedInfo);

    // And into the join graph.
    getJoinGraph()->addTable(scanNodeIndex, scanNodeIndex, naTable);
  }

  // Now make sure Views' usage is marked correctly, the default is
  // COM_USER_SPECIFIED. if the view is "masked", it should be marked DIRECT.
  // Views are not part of the used objects list.
  for (CollIndex j =0; j<usedViewsList_.entries(); j++)
  {
    MVUsedObjectInfo *viewInfo = usedViewsList_[j];
    if (viewColumnGraph_->isMaskedTable(viewInfo->getObjectName()))
      viewInfo->getCatmanFlags().setUsageType(COM_DIRECT_USAGE);
  }

  // Go over the Join predicate list and mark columns used by joins.
  for (CollIndex k=0; k<getEqPredicateList().entries(); k++)
    getEqPredicateList()[k]->markPredicateColsOnUsedObjects(this, TRUE);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
MVUsedObjectInfo *MVInfoForDDL::initUsedObject(Scan          *scanNode, 
					       const NATable *naTable,
					       BindWA        *bindWA)
{
  NABoolean isMjv = (getMVType()==COM_MJV || getMVType()==COM_MAJV);

  // Build a new used object.
  MVUsedObjectInfo *usedInfo = new(getHeap()) 
    MVUsedObjectInfo(scanNode, isMjv, getHeap());

  if (isLeftJoinInnerTable(naTable->getTableName()))
    usedInfo->setAsInnerTable();

  // Check if any nonrepeatabl expressions are used in the selection predicate,
  // and mark columns used by it as indirect update columns.
  CMPASSERT(scanNode->getSelPredTree()==NULL);
  ValueIdSet selectionPreds(scanNode->getSelectionPred());
  if ((CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) &&
     (selectionPreds.entries()))
  {
    ItemExpr * inputItemExprTree = selectionPreds.rebuildExprTree(ITM_AND,FALSE,FALSE);
    ItemExpr * resultOld = revertBackToOldTree(CmpCommon::statementHeap(), inputItemExprTree);
    selectionPreds.clear();
    resultOld->convertToValueIdSet(selectionPreds, NULL, ITM_AND, FALSE);
    //ValueIdSet resultSet;
	//revertBackToOldTreeUsingValueIdSet(selectionPreds, resultSet);
	//selectionPreds.clear();
	//selectionPreds += resultSet;
  }
  
  if (usedInfo->processSelectionPredicates(selectionPreds, bindWA))
  {
    setNotIncremental(MVNI_NON_REPEATABLE_EXPRESSION);
    if (bindWA->errStatus())
      return NULL;
  }

  if (!selectionPreds.isEmpty())
  {
    NAString predicates;
    selectionPreds.unparse(predicates, DEFAULT_PHASE, MVINFO_FORMAT);
    usedInfo->setSelectionPredicates(predicates);
  }

  // Determine the usage type: Direct or user specified.
  ComMVSUsageType usageType = COM_UNKNOWN_USAGE;
  // An object is marked direct when it is part of the direct tree, but it
  // is not visible because it is masked by a view. It is marked as user
  // specified when columns of it can appear in the top most select list.
  if (viewColumnGraph_!= NULL && 
      viewColumnGraph_->isMaskedTable(naTable->getTableName()))
    usageType = COM_DIRECT_USAGE;
  else
    usageType = COM_USER_SPECIFIED;
  
  usedInfo->getCatmanFlags().setUsageType(usageType);

  return usedInfo;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
NABoolean MVInfoForDDL::isLeftJoinInnerTable(const QualifiedName& tableName) const
{ 
  for (CollIndex i=0; i<leftJoinInnerTables_.entries(); i++)
    if (tableName == *(leftJoinInnerTables_[i]))
      return TRUE;

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Called from processNormalizedInformation().
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::initJoinPredicates(BindWA *bindWA)
{
  // The VEG pointer list is used to avoid registering the same VEG more 
  // than once, even through VEGRefs.
  LIST (const VEG *) vegPtrList(getHeap());

  ComMVSUsageType  usageType = COM_DIRECT_USAGE;

  // For each of the Join nodes on the tree
  for (CollIndex i=0; i<joinNodes_.entries(); i++)
  {
    Join *currNode = joinNodes_[i];

    // Is this a left join of any type?
    NABoolean isLeftJoin = (currNode->getOperator().match(REL_ANY_LEFT_JOIN));

    // Create a combined expression of join and selection predicates.
    const ValueIdSet& joinPredicates = currNode->getJoinPred();
    const ValueIdSet& selectionPredicates = currNode->getSelectionPredicates();
    ValueIdSet predicates(joinPredicates);
    predicates += selectionPredicates;

    // For each predicate on the combined expression
    for (ValueId pred = predicates.init();  
	 predicates.next(pred); 
	 predicates.advance(pred) ) 
    {
      MVVegPredicate *mvVegPred = 
	buildNewVegPred(pred, vegPtrList, usageType, isLeftJoin);
      if (mvVegPred == NULL)
	continue;

      // Make sure we have at least two members in this MVVegPredicate.
      CMPASSERT(mvVegPred->entries() >= 2);

      if (!mvVegPred->isIncremental())
        setNotIncremental(MVNI_MULTIPLE_COLUMN_JOIN_PREDICATE);

      if (isLeftJoin)
      {
	// The inner table is the right child.
	Scan *innerScan = (Scan *)(RelExpr *)currNode->child(1);
	if (innerScan->getOperatorType() == REL_SCAN)
	  leftJoinInnerTables_.insert(&innerScan->getTableName().getQualifiedNameObj());
	else
	{
	  // It's a bushy tree, so it's not incremental anyway.
	  CMPASSERT(!isIncremental());
	}
      }

      // Insert the new MVVegPredicate into the list.
      getEqPredicateList().insert(mvVegPred);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
MVVegPredicate *MVInfoForDDL::buildNewVegPred(ValueId             predValueId, 
					      LIST (const VEG *)& vegPtrList,
					      ComMVSUsageType     usageType,
					      NABoolean           isLeftJoin)
{
  ItemExpr *predExpr = predValueId.getItemExpr();
  OperatorTypeEnum predType = predExpr->getOperatorType();
  MVVegPredicate *mvVegPred = NULL;

  // Only VEG predicates and equal operators are considered equal 
  // predicates and are incremental.
  switch (predType)
  {
    case ITM_VEG_PREDICATE:
    {
      const VEG *veg = ((VEGPredicate *)predExpr)->getVEG();
      // Have we seen this VEG before?
      if (vegPtrList.contains(veg))
	break;
      vegPtrList.insert(veg);

      // Construct a new MVVegPredicate object.
      mvVegPred = new(getHeap()) 
	MVVegPredicate(isLeftJoin, getHeap(), usageType);
      // Insert the VEG members into it.
      mvVegPred->addVegPredicates(veg, FALSE, vegPtrList, getHeap());
    }
    break;

    case ITM_EQUAL:
    {
      // Construct a new MVVegPredicate object.
      mvVegPred = new(getHeap()) 
	MVVegPredicate(isLeftJoin, getHeap(), usageType);
      // Insert the two operands into it.
      mvVegPred->addPredicateMember(predExpr->child(0)->getValueId(), 
	                            FALSE, NULL, vegPtrList, getHeap());
      mvVegPred->addPredicateMember(predExpr->child(1)->getValueId(), 
	                            FALSE, NULL, vegPtrList, getHeap());
      if ((mvVegPred->entries() <= 1) &&
          (!mvVegPred->isIncremental()))
      {
        setNotIncremental();
        mvVegPred = NULL;
      }
    }
    break;

    default:
      setNotIncremental(MVNI_NON_EQUAL_JOIN_PREDICATE);
      break;
  }

  return mvVegPred;
}

//////////////////////////////////////////////////////////////////////////////
// MAVs have two types of system added columns: Group by columns and 
// dependent aggregate columns. This method does:
// 1. Initialize the expression hash with the columns specified by the user.
// 2. Add COUNT(*) if needed.
// 3. Add needed Group By columns.
// 4. Add any needed dependent columns.
// The expression hash is a data structure designed to easily identify which
// dependent columns are needed, and which already exist. Basically it is a 
// two dimensional matrix, where the dimensions are the aggregate function, 
// and the expression that is the aggregation operand. The expression 
// dimension is implemented by a hash table - ExpressionHash, where the key 
// is the expression text, and the value is the AggregateArray. the 
// AggregateArray implements the aggregate dimension as a fixed size array 
// of pointers to MVColumnInfo objects, where the index is the aggregate
// enum - MVAggFunc. This way we can find the needed MVColumnInfo object
// on O(1) on both dimensions.
// As a side effect, the posOfCountStar and isMinMaxUsed members are affected.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addMavSystemColumns(BindWA *bindWA)
{
  if (!isIncrementalWorkNeeded())
    return;

  MVColumns& columnList = getMVColumns();

  // Initialize the hash to twice the number of user columns - for efficiency.
  ExpressionHash expHash(hashKey, colDescList_->entries()*2); // Pass NAString::hashKey

  // Add the user columns to the expression hash.
  for (CollIndex i=0; i<columnList.entries(); i++)
  {
    MVColumnInfo *colInfo = columnList[i];
    // Remember the position of the COUNT(*) column.
    if (colInfo->isCountStarColumn())
      setPosOfCountStar(i);
    else
    {
      // Remember if we are using any Min/Max functions.
      if (colInfo->isMinMaxColumn())
	setMinMaxIsUsed();

      // Insert the column into the expression hash.
      colInfo->insertIntoExpHash(expHash, getHeap());
    }
  }

  // Now add a COUNT(*) if not specified by the user.
  if (getPosOfCountStar() == -1)
  {
    MVColumnInfo *colInfo = new (getHeap()) 
      MVColumnInfo(*this, getHeap());
    columnList.insert(colInfo, isIncrementalWorkNeeded());
    setPosOfCountStar(colInfo->getColNumber());
  }

  // Now make sure all the group by columns are in.
  // We will not get here with more than one GroupBy node.
  CMPASSERT(groupByNodes_.entries() <= 1)
  // If there isn't any GroupBy node, skip this step.
  if (groupByNodes_.entries() == 1)
  {
    // Get the list of GroupBy grouping columns.
    GroupByAgg *groupByNode = groupByNodes_[0];
    ValueIdSet groupingCols(groupByNode->groupExpr());

    // Remove from the list any GroupBy columns specified by the user.
    // Identify them by ValueId from the column expression.
    for (CollIndex j=0; j<columnList.entries(); j++)
    {
      MVColumnInfo *colInfo = columnList[j];
      if (colInfo->getColType() == COM_MVCOL_GROUPBY)
  	groupingCols.remove(colInfo->getColExpr()->getValueId());
    }

    // All the remaining columns on the list should be added as system cols.
    for (ValueId groupCol= groupingCols.init();  
	 groupingCols.next(groupCol); 
	 groupingCols.advance(groupCol)) 
    {
      // Create an MVColumnInfo object for the new system column.
      ItemExpr *colExpr = groupCol.getItemExpr();
      MVColumnInfo *colInfo = new (getHeap()) 
	MVColumnInfo (*this, colExpr, getHeap());

      // And add it to the list of MV columns. 
      columnList.insert(colInfo, isIncrementalWorkNeeded());
    }
  }

  // Finally add dependent columns for the aggregates.
  // As dependent columns are added, columnList.entries() grows accordingly,
  // so createDependentColumns() is called for these new columns as well.
  for (CollIndex k=0; k<columnList.entries(); k++)
  {
    MVColumnInfo *colInfo = columnList[k];
    colInfo->createDependentColumns(expHash, *this, getHeap());
  }
}

//////////////////////////////////////////////////////////////////////////////
// For RECOMPUTE MVs we need to keep track of which CI columns from the source
// tables the MV is using so we can use them for the automatic clustering 
// of the MV
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addRecomputeMVUSedObjectsCIColumns(BindWA *bindWA)
{
  if (getRefreshType() != COM_RECOMPUTE)
    return;

  // Now see which clustering key columns of each table are in the select list
  // of the MV
  for (CollIndex scanNodeIndex = 0;
       scanNodeIndex<scanNodes_.entries();
       scanNodeIndex++)
  {
    // Get the fully qualified table name from the tableDesc.
    TableDesc *tDesc = scanNodes_[scanNodeIndex]->getTableDesc();

    addUsedObjectsClusteringInfo(tDesc, bindWA);
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addUsedObjectsClusteringInfo(TableDesc *tDesc,
                                                BindWA    *bindWA)
{
  const ValueIdList &clusteringKeyCols =
    tDesc->getClusteringIndex()->getIndexKey();

  // Find the usedInfo object for the current table.
  MVUsedObjectInfo *usedInfo =
    findUsedInfoForTable(tDesc->getCorrNameObj().getQualifiedNameObj());
  CMPASSERT(usedInfo != NULL);

  // Iterate over all the clustering index columns of this table.
  for (CollIndex ciColIndex = 0;
       ciColIndex < clusteringKeyCols.entries();
       ciColIndex++)
  {
    NAColumn *keyCol = clusteringKeyCols[ciColIndex].getNAColumn();

    // Is this column part of an EQ predicate with another column
    // that is already in the list?
    Lng32 alternateCol = findEqPredicateCovering(keyCol);
  
    MVColumnInfo *existingMvColumn =
      getMVColumns().getMvColInfoByBaseColumn(*keyCol->getTableName(),
                                              keyCol->getPosition());

    // only add the base table column (or its equivalent) if it is not nullable
    if (existingMvColumn == NULL)
    {  // The clustering key column was NOT found in the used column list
       if ((alternateCol != -1) && (getMVColumns()[alternateCol]->isNotNull()))
       {
          // The base column itself was not found in the MV select list,
          // but there is no need to add it because it is "represented" there
          // by some alternate column that is always equal to it because there
          // is an euqal predicate between them.
          usedInfo->addMVColRCI(alternateCol);
       }
    }
    else if (existingMvColumn->isNotNull())
    {
        usedInfo->addMVColRCI(existingMvColumn->getColNumber());
    }
  } // of for i loop on key columns
}

//////////////////////////////////////////////////////////////////////////////
// MJV system added columns are the clustering index columns of used objects,
// that were not added by the user to the MV select list.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addMjvSystemColumns(BindWA *bindWA)
{
  if (!isIncrementalWorkNeeded())
    return;

  // Now see if all the clustering key columns of each table are in the list
  // and if not - add them as system columns.
  for (CollIndex scanNodeIndex = 0; 
       scanNodeIndex<scanNodes_.entries(); 
       scanNodeIndex++)
  {
    // Get the fully qualified table name from the tableDesc.
    TableDesc *tDesc = scanNodes_[scanNodeIndex]->getTableDesc();

    addClusteringIndexAsSystemAdded(tDesc, bindWA);
  }

  // For all computed columns, add all the base cols used by the computed 
  // expression as COMPLEX mv columns.
  addBaseColsUsedByComputedMvColumns();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addClusteringIndexAsSystemAdded(TableDesc *tDesc,
						   BindWA    *bindWA)
{
  const ValueIdList &clusteringKeyCols =
    tDesc->getClusteringIndex()->getIndexKey();

  // Find the usedInfo object for the current table.
  MVUsedObjectInfo *usedInfo = 
    findUsedInfoForTable(tDesc->getCorrNameObj().getQualifiedNameObj());
  CMPASSERT(usedInfo != NULL);

  // Iterate over all the clustering index columns of this table.
  for (CollIndex ciColIndex = 0; 
       ciColIndex < clusteringKeyCols.entries(); 
       ciColIndex++)
  {
    NAColumn *keyCol = clusteringKeyCols[ciColIndex].getNAColumn();
    MVColumnInfo *MVColInfo = 
      getMVColumns().getMvColInfoByBaseColumn(*keyCol->getTableName(),
	                                      keyCol->getPosition());
    Lng32 MVColIndex = ( MVColInfo == NULL ?
			-1		  :
			MVColInfo->getColNumber() );

    // Add a new system added column if needed.
    MVColumnInfo *colInfo = 
      addNewMjvSystemColumnIfNeeded(bindWA, keyCol, MVColIndex);
    if (bindWA->errStatus())
      return;

    // get the value of the INSERTLOG attribute for the MV used object
    const NABoolean isInsertOnlyTable = 
      tDesc->getNATable()->getMvAttributeBitmap().getIsInsertLog();
    usedInfo->getCatmanFlags().setIsInsertLog(isInsertOnlyTable);

    // We constructed a new MVColumnInfo object, add it to our lists.
    if (colInfo != NULL)
    {
      // Add it to the MV columns list.
      getMVColumns().insert(colInfo, isIncrementalWorkNeeded());
      // Add it to the used columns list.
      usedInfo->addUsedColumn(keyCol->getPosition());
      // Add it to the RCI.
      usedInfo->addMVColRCI(colInfo->getColNumber());
    }
    else
    {
      // Add the MV column to the RCI.
      usedInfo->addMVColRCI(MVColIndex);
    }
  } // of for i loop on key columns
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addBaseColsUsedByComputedMvColumns()
{
  MVColumns& columnList = getMVColumns();
  for (CollIndex mvCol=0; mvCol<columnList.entries(); mvCol++)
  {
    MVColumnInfo *colInfo = columnList[mvCol];

    // Insert only base columns (not complex)
    if (colInfo->getColType() != COM_MVCOL_FUNCTION)
      continue;

    LIST(BaseColumn*) usedCols(getHeap());
    findAllTheUsedColumns(colInfo->getColExpr(), NULL, usedCols);
    // If this computed column is only using a single base column (or less)
    // than that column is already entered as the orig column of that MV col.
    if (usedCols.entries() < 2)
      continue;

    // When there are at least two base columns that are used by this computed
    // mv column - than the orig column field is empty, and we need to add 
    // links to all of them.
    for (CollIndex i=0; i<usedCols.entries(); i++)
    {
      NAColumn *baseCol = usedCols[i]->getNAColumn();

      // Do we already have an MVColumnInfo for this one?
      MVColumnInfo *MVBaseColInfo = 
        getMVColumns().getMvColInfoByBaseColumn(*baseCol->getTableName(),
	                                         baseCol->getPosition(), mvCol);

      // If so, don't add duplicate references.
      if (MVBaseColInfo != NULL)
	continue;

      MVColumnInfo *newColInfo = new (getHeap()) 
	MVColumnInfo(colInfo->getColNumber(), 
		     *baseCol->getTableName(),
		     baseCol->getPosition(),
		     COM_MVCOL_COMPLEX,
		     getHeap());

      getMVColumns().insert(newColInfo, isIncrementalWorkNeeded());

      // Mark this as an "indirect update column".
      setColAsIndirectUpdate(newColInfo);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// Check if a new system added column is needed for the base table 
// clustering column keyCol. 
//////////////////////////////////////////////////////////////////////////////
MVColumnInfo *MVInfoForDDL::addNewMjvSystemColumnIfNeeded(BindWA   *bindWA,
							  NAColumn *keyCol,
							  Lng32      MVColIndex) 
{
  MVColumnInfo *colInfo = NULL;

  // Is this column part of an EQ predicate with another column
  // that is already in the list?
  Lng32 alternateCol = findEqPredicateCovering(keyCol);

  MVColumnInfo *existingMvColumn = 
    getMVColumns().getMvColInfoByBaseColumn(*keyCol->getTableName(),
					    keyCol->getPosition());

  // the condition (existingMvColumn->getColType() != COM_MVCOL_BASECOL)
  // is used in the case where the mv has a column that is defined as a function of
  // a source table clustering column(e.g, b + 1 as b1 where b is in the CK of the source table).
  // In this case we don't want to cluster on this column but we want to add a new system column
  // that corresponds to the source table column.
  if ((existingMvColumn == NULL) || (existingMvColumn->getColType() != COM_MVCOL_BASECOL))
  {   // The clustering key column was NOT found in the used column list
    if (alternateCol != -1)
    {
      // The base column itself was not found in the MV select list,
      // but there is no need to add it because it is "represented" there
      // by some alternate column that is always equal to it because there
      // is an euqal predicate between them.

      // Add a "place holder" DUPLICATE column for the system column.
      // The effect is a link between the base column and the MV column
      // that represents it (needed for MJV).
      colInfo = new (getHeap()) 
	MVColumnInfo(alternateCol, 
		     *keyCol->getTableName(),
		     keyCol->getPosition(),
		     COM_MVCOL_DUPLICATE,
		     getHeap());
    }
    else
    {
      // An alternate column was not found, so we need a system added column.
      if (viewColumnGraph_!= NULL		       && 
	  viewColumnGraph_->isMaskedColumn(keyCol) &&
	  keyCol->getColName() != "SYSKEY")
      {
	// The base column is from a masked table - meaning
	// it is masked by a view. However, there is no view
	// column on top of it, so the view does not project 
	// this clustering key. The result: the MV is not 
	// incremental.
	// SYSKEY is an exception - we propagate it ourselves
	// (see RETDesc::propagateMVColumns()).
        setNotIncremental(MVNI_ON_VIEW);

	NAString colName = keyCol->getFullColRefNameAsAnsiString();
	// 12302 MV is not incremental: Clustering Key column $0~string0 is not visible through the view.
	*CmpCommon::diags() << DgSqlCode(-12302)
	  << DgString0(colName);
	bindWA->setErrStatus();
	return NULL;
      }

      // Add a new system column.
      colInfo = new (getHeap()) 
	MVColumnInfo (*this, keyCol, getHeap());
    }
  }
  else
  { // The clustering key column WAS found in the used column list.
    // Does it also appear in an EQ join predicate with another MV col?
    if ( (alternateCol != -1) &&
	 (alternateCol < MVColIndex) )
    {
      // Yes. this is a redundant column: when two MV columns have an
      // equal join predicate between them, the second one is redundant. 
      MVColumnInfo *redundantCol = getMVColumns()[MVColIndex];
      redundantCol->setAsRedundant(alternateCol);
    }
  }

  return colInfo;
}

//////////////////////////////////////////////////////////////////////////////
// Check if the column names colName is mentioned in the user specified
// STORE BY clause of the MV.
//////////////////////////////////////////////////////////////////////////////
NABoolean MVInfoForDDL::isColumnInMVCI(const NAString& colName)
{
  if (userSpecifiedCI_ == NULL)
    return FALSE;

  for (CollIndex i=0; i<userSpecifiedCI_->entries(); i++)
  {
    if (userSpecifiedCI_->at(i)->getColumnName() == colName)
      return TRUE;
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// This method is called by StmtDDLCreateMV::bindNode() after the MV select 
// tree has been bound, and collectMVInformation() was called to collect
// needed information from the tree.
// This method has two main tasks:
// 1. Find the MV type: MAV, MJV etc.
// 2. Initialize the MV column list with the columns specified by the user.
// 3. Add MAV system columns.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::processBoundInformation(BindWA *bindWA)
{
  CMPASSERT(rootNode_ != NULL);

  // Is this a MAV, MJV, MAJV or something else that is not incremental?
  initMVType();
  if (getMVType() == COM_MV_OTHER)
    setNotIncremental();  // use default reason

  // Fix the data type of aggregate columns.
  castComputedColumnsToAnsiTypes(bindWA, rootNode_->getRETDesc(), rootNode_->compExpr());

  // The RETDesc of the root node is where most of the information used to
  // build the MV column list is taken from.
  colDescList_ = rootNode_->getRETDesc()->getColumnList();
  CMPASSERT(colDescList_ != NULL);
  userColumnCount_ = colDescList_->entries();

  // Only one GroupBy node allowed on the direct tree.
  if (groupByNodes_.entries()>1)
      setNotIncremental(MVNI_MULTI_GROUP_BY_CLAUSES);

  // Add the user columns to the column list.
  MVColumns& mvColumns = getMVColumns();
  for (CollIndex i=0; i<colDescList_->entries(); i++)
  {
    MVColumnInfo *colInfo = new (getHeap()) 
      MVColumnInfo(*this, getMVType(), colDescList_->at(i), bindWA, getHeap());

    if (isColumnInMVCI(colInfo->getColName()))
      colInfo->setInMVCI();

    // Insert the initialized object into the column list
    mvColumns.insert(colInfo, isIncrementalWorkNeeded());

    // the column is set to COM_MVCOL_OTHER 
    // when its a non repeatable expression
    if (colInfo->getColType() == COM_MVCOL_OTHER)
      setNotIncremental(MVNI_NON_REPEATABLE_EXPRESSION);  

    // the column is set to COM_MVCOL_COMPLEX for a MAV
    // then an aggregate function is inside a complex expression.
    if ( isIncrementalWorkNeeded()                      && 
        (getMVType()==COM_MAV || getMVType()==COM_MAJV) &&
         colInfo->getColType() == COM_MVCOL_COMPLEX)
      setNotIncremental(MVNI_AGGREGATE_NOT_TOP_LEVEL);
  }

  if ( isIncrementalWorkNeeded() && 
      (getMVType()==COM_MAV || getMVType()==COM_MAJV))
    addMavSystemColumns(bindWA);
}

//////////////////////////////////////////////////////////////////////////////
// This method is called by StmtDDLCreateMV::bindNode() after the MV select 
// tree has been normalized, and collectMVInformation() was called (again) 
// to collect needed information from the tree.
// This method has these main tasks:
// 1. Verify that all GroupBy columns are in the grouping expression of
//    the GroupBy node.
// 2. Check if there is a HAVING clause (not incremental).
// 3. Check if the Join tree is left linear.
// 4. Collect the equal join predicates.
// 5. Initialize the used objects list.
// 6. Run the join graph algorithm to make the join graph is fully connected.
// 7. Add MJV system columns (base table clustering index columns).
// 8. Add views to the used objects list.
// 9. Add UDFs to the used objects list.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::processNormalizedInformation(BindWA *bindWA)
{
  // Verify that all the columns marked as GroupBy cols, are found as such
  // in the GroupBy node.
  // Also - selection predicates on theGroupBy nodes means there is a 
  // Having clause - not incremental.
  if ((getMVType()==COM_MAV || getMVType()==COM_MAJV))
  {
    if (groupByNodes_.entries() > 1)
      setNotIncremental(MVNI_MULTI_GROUP_BY_CLAUSES);

    GroupByAgg *groupByNode = groupByNodes_[0];

    verifyGroupByColumns(groupByNode);

    // Check for a selection predicate - HAVING is not incremental.
    if (!groupByNode->getSelectionPred().isEmpty() ||
	 groupByNode->getSelPredTree() != NULL)
      setNotIncremental(MVNI_HAVING_CLAUSE);
  }

  // The following code saves a copy of the direct join predicates and clears
  // the array for the expanded pass. The direct preds are added back to the
  // array when the work is done.
  LIST (MVVegPredicate *) directEqPredicateList(getEqPredicateList());
  getEqPredicateList().clear();

  if (getMVType()==COM_MJV || getMVType()==COM_MAJV)
  {
    // Find the top most Join node (or the Scan node if there is no Join).
    RelExpr *topJoin = rootNode_;
    while (topJoin != NULL &&
           topJoin->getOperatorType() != REL_SCAN &&
           !topJoin->getOperator().match(REL_ANY_JOIN))
      topJoin = topJoin->child(0);

    // Check if the Join tree is left linear.
    isLeftLinear_ = isTreeLeftLinear(topJoin);
    if (!isLeftLinear_)
      setNotIncremental(MVNI_JOIN_TREE_NOT_LEFT_LINEAR);

    // Collect the equal join predicates from the Join nodes.
    initJoinPredicates(bindWA);
  }

  // Initialize the list of objects (base tables and other MVs)
  // used by this MV.
  initUsedObjectsList(bindWA);
  if (isIncrementalWorkNeeded())
  {
    // Run the join graph algorithm to make the join graph is fully connected.
    if (getJoinGraph()->isFullyConnected() == FALSE)
      setNotIncremental(MVNI_JOIN_GRAPH_NOT_CONNECTED);
    else
      getJoinGraph()->markOrderOfUsedObjects(this); 
  }

  // Adding system columns to an MJV is done after creating the used objects
  // list, because system columns should be added to that list.
  if ( getMVType() == COM_MJV && 
       isIncrementalWorkNeeded())
    addMjvSystemColumns(bindWA);
  else if (getRefreshType() == COM_RECOMPUTE)
    addRecomputeMVUSedObjectsCIColumns (bindWA);

  // Mark the MV CI columns as indirect update columns.
  markMVCIasIndirectUpdate();

  // Before we are done, append the views list to the used objects
  addViewsToUsedObjectList();

  // Before we are done, append the UDFs list to the used objects
  addUDFsToUsedObjectList();
}

//////////////////////////////////////////////////////////////////////////////
// Mark the MV CI columns as indirect update columns.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::markMVCIasIndirectUpdate()
{
  MVColumns& columnList = getMVColumns();

  // Iterate over all the MV columns
  for (CollIndex i=0; i<columnList.entries(); i++)
  {
    MVColumnInfo *colInfo = columnList[(Int32)i];
    // Find those that are part of the MV CI
    if (colInfo->isInMVCI())
    {
      colInfo->setIndirectUpdate();
      setColAsIndirectUpdate(colInfo);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// Make sure the used views have the correct usage: direct or user specified,
// and then add them to the used objects list.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addViewsToUsedObjectList()
{
  // If there are no used views, there is nothing to do here.
  if (usedViewsList_.entries() == 0)
    return;

  // If there are views, there must be a view graph.
  CMPASSERT(viewColumnGraph_ != NULL);

  // For each used view
  for (CollIndex i=0; i<usedViewsList_.entries(); i++)
  {
    MVUsedObjectInfo *currentView = usedViewsList_[i];

    // The default is user specified.
    CMPASSERT(currentView->getCatmanFlags().getUsageType() == COM_USER_SPECIFIED);
    if (viewColumnGraph_->isMaskedTable(currentView->getObjectName()))
    {
      // Mark it as direct if it is masked by another view.
      currentView->getCatmanFlags().setUsageType(COM_DIRECT_USAGE);
    }

    // Insert it into the used objects list.
    getUsedObjectsList().insert(currentView);
  }

  // We don't need the views list anymore. 
  usedViewsList_.clear();
}

//////////////////////////////////////////////////////////////////////////////
// Add any UDFs to the UsedObjectList
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::addUDFsToUsedObjectList()
{
  // If there are no used UDFs, there is nothing to do here.
  const LIST(OptUDFInfo *) &udfList = getUDFList();
  ULng32 numUdfs = udfList.entries();

  for (ULng32 udfIndex = 0; udfIndex < numUdfs; udfIndex++)
  {
    OptUDFInfo *currentUDF = udfList[udfIndex];

    CMPASSERT(currentUDF);
    
    MVUsedObjectInfo *usedObject = new(currentUDF->getHeapPtr())
      MVUsedObjectInfo(currentUDF, getHeap());

    // Insert it into the used objects list.
    getUsedObjectsList().insert(usedObject);
  }

}

//////////////////////////////////////////////////////////////////////////////
// Verify that all the columns marked as GroupBy cols, are found as such
// in the GroupBy node. If not - mark the MV as not incremental.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::verifyGroupByColumns(GroupByAgg *groupByNode)
{
  ValueIdSet &groupingCols = groupByNode->groupExpr();
  MVColumns& columnList = getMVColumns();

  // Iterate over all the MV columns
  for (CollIndex i=0; i<columnList.entries(); i++)
  {
    // Check only column of type GROUPBY
    MVColumnInfo *colInfo = columnList[i];
    if (colInfo->getColType() != COM_MVCOL_GROUPBY)
      continue;
    
    // Verify that the grouping columns contain this column.
    ValueId vid = colInfo->getColExpr()->getValueId();
    if (!groupingCols.containsTheGivenValue(vid))
      setNotIncremental(MVNI_COLUMN_NOT_IN_GROUPING_COLUMNS);
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
NABoolean MVInfoForDDL::areTablesUsedOnlyOnce(BindWA *bindWA)
{
  TableNameHash usedObjectsNames(QualifiedName::hash, 20);

  return addTableNamesToList(bindWA, usedObjectsNames);
}

//////////////////////////////////////////////////////////////////////////////
// This MV can be either a MAV, MJV, MAJV or COM_MV_OTHER which means
// it is not incremental.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::initMVType()
{
  RelExpr   *node = getChildBelowRootAndRenameNodes(rootNode_);
  ComMVType  mvType;

  if (node->getOperator().match(REL_ANY_JOIN) ||
      node->getOperatorType() == REL_SCAN)
    mvType = COM_MJV;
  else
  {
    if ( (node->getOperatorType() != REL_GROUPBY) &&
	 (node->getOperatorType() != REL_AGGREGATE) )
      mvType = COM_MV_OTHER;
    else
    {
      // Skip the GroupBy node.
      node = getChildBelowRootAndRenameNodes(node);

      if (node->getOperator().match(REL_ANY_JOIN))
      {
	if ( (node->getOperatorType() == REL_LEFT_JOIN) ||
	     (node->getOperatorType() == REL_RIGHT_JOIN) )
	  mvType= COM_MV_OTHER; // outer join not allowed in MAJV.
	else
  	  mvType = COM_MAJV;
      }
      else if (node->getOperatorType() == REL_SCAN)
	mvType = COM_MAV;
      else
	mvType= COM_MV_OTHER;
    }
  }
  setMvType(mvType);
} 

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RelExpr *MVInfoForDDL::getChildBelowRootAndRenameNodes(RelExpr *topNode)
{
  RelExpr *node = topNode->child(0);

  // If MV on view, skip RenameTable and RelRoot.
  // The while() loop handle possible multiple views.
  while ( (node->getOperatorType() == REL_RENAME_TABLE) ||
	  (node->getOperatorType() == REL_ROOT)	        ||
	  (node->getOperatorType() == REL_UNPACKROWS) )
    node = node->child(0);

  return node;
}

//////////////////////////////////////////////////////////////////////////////
// Is the specified table name a not ignore changes used object?
//////////////////////////////////////////////////////////////////////////////
NABoolean MVInfoForDDL::isUsedObjectNotIgnoreChanges(const QualifiedName& tableName)
{
  MVUsedObjectInfo *usedInfo;
  if((usedInfo = findUsedInfoForTable(tableName)) == NULL)
  {
    return FALSE;
  }
  if(COM_IGNORE_CHANGES == usedInfo->getCatmanFlags().getObjectAttributes())
  {
    return FALSE;
  }
  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// Mark a column as an MJV indirect update column.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDDL::setColAsIndirectUpdate(MVColumnInfo *colInfo)
{
  if (getMVType() != COM_MJV)
    return;

  const QualifiedName& baseTableName = colInfo->getOrigTableName();
  if (baseTableName != "")
  {
    MVUsedObjectInfo *usedObj = findUsedInfoForTable(baseTableName);
    CMPASSERT(usedObj != NULL);
    usedObj->addIndirectUpdateCol(colInfo->getOrigColNumber());
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
// Exclude from coverage testing - Debugging code
void MVInfoForDDL::print( FILE* ofd,	      // = stdout,
			  const char* indent, // = DEFAULT_INDENT,
			  const char* title   // = "MVInfo"
			) const
{
  MVInfo::print(ofd, indent, title);

  char *LeftLinText = (char *) (isLeftLinear_  ? "TRUE" : "FALSE");
  char *overMvsText = (char *) (usesOtherMVs_  ? "TRUE" : "FALSE");

  fprintf(ofd, "DDL Info: Over MVs %s, Left Linear %s\n", overMvsText, LeftLinText);
  fprintf(ofd, "    System col names: \"%s\".\n", sysColNames_.data());
  fprintf(ofd, "    System col text : \"%s\".\n", textForSysCols_.data());
  fprintf(ofd, "    Parsed col text : ");
  for (CollIndex i=0; i<unBoundColumnsText_.entries(); i++)
  {
    const NAString *text = unBoundColumnsText_[i];
    if (text!=NULL)
      fprintf(ofd, "%s, ", text->data());
  }
  fprintf(ofd,"\n");
} // MVInfoForDDL::print()
#endif

//============================================================================
//===========================  class MVInfoForDML ============================
//============================================================================

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
MVInfoForDML::MVInfoForDML( const NAString	       &NameOfMV,
			    ComMVRefreshType		refreshType,
			    NABoolean			isRewriteEnabled,
			    ComMVStatus			mvStatus,
			    const NAString&		MVSelectText,
			    CharInfo::CharSet		mvSelectTextCharSet,
			    NABoolean			isIncremental,
			    ComMVType			mvType,
			    LIST (MVUsedObjectInfo *)&  usedObjectsList,
			    MVColumnInfoList&           DirectColumnList,
			    NABoolean	  		isLeftLinear,
			    LIST (MVVegPredicate *)&    eqPredicateList,
			    CollHeap		       *heap)
  : MVInfo(NameOfMV, refreshType, isRewriteEnabled, mvStatus, 
           MVSelectText, mvSelectTextCharSet, isIncremental, mvType, 
	   usedObjectsList, DirectColumnList, 
	   isLeftLinear, eqPredicateList, heap),
    usedObjectsHash_(QualifiedName::hash, 20, FALSE, heap)
{
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
MVInfoForDML::~MVInfoForDML() 
{
}

//////////////////////////////////////////////////////////////////////////////
// Return a list of MV column positions for the GroupBy columns.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDML::getMavGroupByColumns(LIST(Lng32)& gbColList) const
{
  const MVColumns& columnList = getMVColumns();

  for (CollIndex i=0; i<columnList.entries(); i++)
  {
    MVColumnInfo *currentCol = columnList[i];
    if (currentCol->getColType() == COM_MVCOL_GROUPBY)
      gbColList.insert(currentCol->getColNumber());
  }
}

//////////////////////////////////////////////////////////////////////////////
// Is this MV defined on a single base table?
//////////////////////////////////////////////////////////////////////////////
NABoolean MVInfoForDML::isMvOnSingleTable() 
{
  return getUsedObjectsList().entries() == 1;
}

//////////////////////////////////////////////////////////////////////////////
// Initialize the join graph for multi-delta internal refresh.
// Init the graph nodes acording to the directly used objects.
// Mark the delta type according to the delta definition list.
// Mark equal predicates according to the eqPredicateList.
// Mark RIs according to the NATable of used objects.
//////////////////////////////////////////////////////////////////////////////
MVJoinGraph *MVInfoForDML::initJoinGraph(BindWA		              *bindWA, 
			                 const DeltaDefinitionPtrList *deltaDefList,
					 NABoolean                     checkOnlyInserts) 
{
  // First of all, find the number of tables in the direct join graph.
  Lng32 directTables = 0;
  for (CollIndex i=0; i < getUsedObjectsList().entries(); i++)
  {
    if (getUsedObjectsList()[i]->isUsedDirectly())
      directTables++;
  }

  newJoinGraph(directTables);

  Lng32 nodeNumber = 0;
  for (CollIndex usedObjectsIndex=0; 
       usedObjectsIndex < getUsedObjectsList().entries(); 
       usedObjectsIndex++)
  {
    const MVUsedObjectInfo *usedInfo = getUsedObjectsList()[usedObjectsIndex];
    if (!usedInfo->isUsedDirectly())
      continue;
    
    CorrName tableName(usedInfo->getObjectName());
    NATable *naTable = bindWA->getNATable(tableName);
    CMPASSERT(naTable != NULL);

    MVJoinTable *newNode = new(getHeap()) 
      MVJoinTable(nodeNumber, usedObjectsIndex, directTables, naTable, getHeap());

    DeltaDefinition *deltaDef =
      deltaDefList->findEntryFor(usedInfo->getObjectName());
    if (deltaDef!=NULL)
    {
      // When deltaDef is not NULL, then the delta is non-empty.
      // Check for two special cases:
      // 1. An insert-only delta
      // 2. When we are only interested in the insertion delta (MJV) then 
      //    if there are no inserted rows we consider it an empty delta.
      //    Updated rows are considered as inserts, only when at least
      //    one updated column is an indirect update (separate delete and 
      //    insert operations).
      if (checkOnlyInserts)
      {
	// When I am only interested in inserts, 
	// any delta with no inserts is considered empty,
	// and any other non-empty delta is considered insert-only.
	// This is the MJV case, so only indirect updates are considered inserts.
	if (deltaDef->isInsertOnly())
	  newNode->setInsertOnlyDelta();
	else if ( (deltaDef->getIudInsertedRows()> 0 )   ||
	         ((deltaDef->getIudUpdatedRows() > 0 ) &&
                  (deltaDef->containsIndirectUpdateColumn(this))))
	  newNode->setInsertOnlyDelta();
	// else - leave as empty delta
      }
      else
      {
	// When I am interested in deletes as well,
	// it is easier...
	// This is the MAV case, so all updates are considered inserts.
        if (deltaDef->isInsertOnly())
  	  newNode->setInsertOnlyDelta();
	else if ( (deltaDef->getIudDeletedRows() == 0) &&
	          (deltaDef->getIudUpdatedRows() == 0)    )
  	  newNode->setInsertOnlyDelta();
	else
	  newNode->setNonEmptyDelta();
      }
    }
    getJoinGraph()->addTable(newNode);

    // Increment the node number only for tables inserted into the array.
    nodeNumber++;
  }
  // nodeNumber is the final number of tables, and is more accurate than
  // what was passed to the Ctor (no. of direct tables).
  getJoinGraph()->setNofTables(nodeNumber); 

  // Go over the Join predicate list and mark columns used by joins.
  // FALSE means its DML time now, not DDL.
  for (CollIndex j=0; j<getEqPredicateList().entries(); j++)
    getEqPredicateList()[j]->markPredicateColsOnUsedObjects(this, FALSE);

  getJoinGraph()->markRiConstraints(bindWA, this);

  return getJoinGraph();
}

//////////////////////////////////////////////////////////////////////////////
// Insert into the usedObjectsHash_ all used objects, by their table
// names and also by their log names.
//////////////////////////////////////////////////////////////////////////////
void MVInfoForDML::initUsedObjectsHash()
{
  LIST (MVUsedObjectInfo*)& usedObjects = getUsedObjectsList();

  if (!usedObjectsHash_.isEmpty())
    return;

  for (CollIndex i=0; i<usedObjects.entries(); i++)
  {
    MVUsedObjectInfo *usedObj = usedObjects[i];

    // Ignore views.
    if (usedObj->getObjectType() == COM_VIEW_OBJECT)
      continue;

    const QualifiedName &qualName = usedObj->getObjectName();

    // Insert the used object to the hash using the table name.
    usedObjectsHash_.insert(&qualName, usedObj);

    CorrName *logName = 
      ChangesTable::buildChangesTableCorrName(qualName, 
                                              COMMV_IUD_LOG_SUFFIX,
					      ExtendedQualName::IUD_LOG_TABLE,
					      getHeap());

    // Insert the used object to the hash using the log name.
    usedObjectsHash_.insert(&logName->getQualifiedNameObj(), usedObj);
  }
}

//////////////////////////////////////////////////////////////////////////////
// This override is implemented using the optimized usedObjectsHash_
//////////////////////////////////////////////////////////////////////////////
const MVUsedObjectInfo *MVInfoForDML::findUsedInfoForTable(const QualifiedName& tableName)
{
  CMPASSERT(!usedObjectsHash_.isEmpty());

  return usedObjectsHash_.getFirstValue(&tableName);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
// Exclude from coverage testing - Debugging code
void MVInfoForDML::print( FILE* ofd,	      // = stdout,
			  const char* indent, // = DEFAULT_INDENT,
			  const char* title   // = "MVInfo"
			) const
{
  MVInfo::print(ofd, indent, title);
  fprintf(ofd, "    MV Select Text: \"%s\".\n", getMVSelectText().data());

  if (strcmp(title, "")) fprintf(ofd,"\n");
} // MVInfoForDDL::print()
#endif

//----------------------------------------------------------------------------
// Create the MV descriptor by taking the MV query from the updated CREATE MV
// text, parsing it, binding, normalizing and analyzing it.
QRMVDescriptorPtr
MVInfo::prepareMvDescriptorTree(NABoolean isIncrementalMV, 
	                              NAString& warningMessage, 
	                              const NAString* alternativeText)
{
  // Create a brand new BindWA.
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), TRUE);
  	
  // Expanded tree is now used only for MAVs, at least until we can support multi-JBB MVs.
  if (getMVType() == COM_MAV)
    bindWA.setExpandMvTree();       // Set the binder Expanded flag

  // Use the new CREATE MV text with the system added columns.
  RelRoot* queryRoot = buildMVSelectTree(alternativeText);
  CMPASSERT(queryRoot!=NULL);
  queryRoot->setRootFlag(TRUE);

  // Bind the expanded tree
  RelExpr* queryExpr = queryRoot->bindNode(&bindWA);
  CMPASSERT(bindWA.errStatus()==0 && queryExpr != NULL);

  // Transform the expanded tree
  NormWA normWA(CmpCommon::context());

  normWA.setCompilingMVDescriptor(TRUE);

  ExprGroupId eg(queryExpr);
  queryExpr->transformNode(normWA, eg);

  // Normalize the expanded tree
  RelRoot *normRoot = (RelRoot *)eg.getPtr();
  normRoot->normalizeNode(normWA);

  // Run the Analyzer to create the MV descriptor.
  queryExpr = queryExpr->semanticQueryOptimizeNode(normWA);
  //QueryAnalysis* queryAnalysis = QueryAnalysis::InitGlobalInstance();
  QueryAnalysis* queryAnalysis = CmpCommon::statement()->initQueryAnalysis();
  queryAnalysis->setMvCreation(TRUE);

  // disable CTS while creating the MV descriptor
  NABoolean savedValue = CURRSTMT_OPTDEFAULTS->histUseSampleForCardEst();
  CURRSTMT_OPTDEFAULTS->setHistUseSampleForCardEst(FALSE);

  queryAnalysis->analyzeThis(queryExpr);

  // restore CTS defaults 
  CURRSTMT_OPTDEFAULTS->setHistUseSampleForCardEst(savedValue);

  queryAnalysis->setMvCreation(FALSE); // Just to be on the safe side...

  QRMVDescriptorPtr descriptor = queryAnalysis->getMvQueryRewriteHandler()->getMvDescriptor();
  
  if (descriptor)
  {
     descriptor->getMisc()->setIsImmediate(getRefreshType() == COM_ON_STATEMENT);
     descriptor->getMisc()->setIsIncremental(isIncrementalMV);
     descriptor->getMisc()->setIsUMV(getRefreshType() == COM_BY_USER);
  }
  else
  {
    warningMessage = queryAnalysis->getMvQueryRewriteHandler()->getWarningMessage();
  }

  return descriptor;
}


//============================================================================
//===================  class MVUsedObjectColNameMap ==========================
//============================================================================

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
MVUsedObjectColNameMap::~MVUsedObjectColNameMap()
{
  // No need to delete the strings (the col names are from the NAColumn array).
  colNameHash_.clear();

  // Delete the array of longs.
  if (colPositionTable_!= NULL)
    collHeap()->deallocateMemory(colPositionTable_);
}

//////////////////////////////////////////////////////////////////////////////
// Initialize the hash table that maps column names to their position in the
// base table. Since the hash table needs pointers to long objects (very 
// inefficient memory management), its better to allocate an array that will 
// hold all those numbers, and use pointers into this array.
//////////////////////////////////////////////////////////////////////////////
void MVUsedObjectColNameMap::initColNameMap(const NATable *naTable, CollHeap *heap)
{
  CMPASSERT(naTable != NULL);

  // It's enough to initialize once.
  if (colPositionTable_ != NULL)
    return;

  const NAColumnArray &naColumns = naTable->getNAColumnArray();
  CollIndex numberOfColumns = naColumns.entries();

  // Allocate an array of numberOfColumns longs.
  colPositionTable_ = 
    (Lng32 *)heap->allocateMemory(numberOfColumns*sizeof(Lng32));

  // Initialize the array using a pointer that is incremented from each
  // position to the next.
  Lng32 *colPositionPtr = colPositionTable_;
  for (CollIndex i=0; i<numberOfColumns; i++, colPositionPtr++)
  {
    const NAColumn *naCol = naColumns[i];
    
    // Initialize the current position of the array with i.
    *colPositionPtr = i;
    // Insert the entry into the hash table.
    colNameHash_.insert(&naCol->getColName(), colPositionPtr);
  }
}

//////////////////////////////////////////////////////////////////////////////
// Find the column position into the base table using the column name.
//////////////////////////////////////////////////////////////////////////////
Lng32 MVUsedObjectColNameMap::getColPositionFor(const NAString& colName) const
{
  CMPASSERT(!isEmpty());
  return *colNameHash_.getFirstValue(&colName);
}
