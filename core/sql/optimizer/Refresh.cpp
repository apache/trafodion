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
* File:         Refresh.cpp
* Description:  Binding methods of class Refresh for MV INTERNAL REFRESH command.
*
* Created:      12/21/99
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
#include "parser.h"
#include "StmtNode.h"
#include "Inlining.h"
#include "Triggers.h"
#include "MVInfo.h"
#include "MVJoinGraph.h"
#include "Refresh.h"
#include "MvRefreshBuilder.h"
#include "MjvBuilder.h"
#include "RelSequence.h"
#include "BinderUtils.h"
#include "ChangesTable.h"
#include <CmpMain.h>

#ifdef NA_DEBUG_GUI
#include "ComSqlcmpdbg.h"
#endif

#include "SqlParserGlobals.h"		// must be last #include


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// ----------------   member functions for class Refresh  ----------------
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// Ctor for RECOMPUTE
Refresh::Refresh(const QualifiedName& mvName, 
		 ComBoolean           noDelete,
		 CollHeap            *oHeap)
  : BinderOnlyNode(REL_REFRESH, oHeap),
    refreshType_(RECOMPUTE),
    mvName_(mvName, oHeap),
    deltaDefList_(NULL),
    phase_(0),
    pNRowsClause_(NULL),
    pipelineClause_(NULL),
    noDeleteOnRecompute_(noDelete),
    bindContext_(NULL),
    additionalPhaseNeeded_(FALSE)
{}


//////////////////////////////////////////////////////////////////////////////
// Ctor for SINGLEDELTA
Refresh::Refresh(const QualifiedName&          mvName, 
		 const DeltaDefinitionPtrList *pDeltaDefList,
		 NRowsClause                  *pOptionalNRowsClause,
		 PipelineClause               *pOptionalPipelineClause,
		 CollHeap                     *oHeap)
  : BinderOnlyNode(REL_REFRESH, oHeap),
    refreshType_(SINGLEDELTA),
    mvName_(mvName, oHeap),
    deltaDefList_(pDeltaDefList),
    phase_(0),
    pNRowsClause_(pOptionalNRowsClause),
    pipelineClause_(pOptionalPipelineClause),
    noDeleteOnRecompute_(FALSE),
    bindContext_(NULL),
    additionalPhaseNeeded_(FALSE)
{}

//////////////////////////////////////////////////////////////////////////////
// Ctor for MULTIDELTA
Refresh::Refresh(const QualifiedName&          mvName, 
		 const DeltaDefinitionPtrList *pDeltaDefList,
		 Lng32	                       phaseVal,
		 PipelineClause               *pOptionalPipelineClause,
		 CollHeap                     *oHeap)
  : BinderOnlyNode(REL_REFRESH, oHeap),
    refreshType_(MULTIDELTA),
    mvName_(mvName, oHeap),
    deltaDefList_(pDeltaDefList),
    phase_(phaseVal),
    pNRowsClause_(NULL),
    pipelineClause_(pOptionalPipelineClause),
    noDeleteOnRecompute_(FALSE),
    bindContext_(NULL),
    additionalPhaseNeeded_(FALSE)
{}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Refresh::~Refresh() 
{
}

//////////////////////////////////////////////////////////////////////////////
// This is where all the action begins.
// The simple RelRoot-Refresh tree constructed by the parser, is transformed
// to a complex RelExpr tree that implements the MV refresh algorithm.
// The method: build a MvRefreshBuilder object, have it build the MV refresh
// tree, and then bind the resulting tree. The bind may cause additional 
// levels of pipelined refresh to be constructed and bound as well.
//////////////////////////////////////////////////////////////////////////////
RelExpr *Refresh::bindNode(BindWA *bindWA)
{
  CollHeap *heap = bindWA->wHeap();
  // for DEFAULT_SCHEMA_ACCESS_ONLY, there is no need to check here
  // because this fucntion is only called internally
  mvName_.applyDefaults(bindWA->getDefaultSchema());
  if (deltaDefList_ != NULL)
    deltaDefList_->applyDefaultSchemaToAll(bindWA);
  if (pipelineClause_ != NULL)
    pipelineClause_->applyDefaultSchemaToAll(bindWA);

  CorrName  mvCorrName(mvName_);
  MVInfoForDML *mvInfo = getMvInfo(bindWA, mvCorrName);
  if (mvInfo == NULL)
    return NULL;

  // User maintained MVs can only be recomputed (for initialization).
  if (mvInfo->getRefreshType() == COM_BY_USER &&
      refreshType_ != RECOMPUTE )
  return NULL;

  if (verifyDeltaDefinition(bindWA, mvInfo))
    return NULL;

  // From now on, the binder needs to know we are binding a refresh statement.
  bindWA->setBindingMvRefresh();
  bindWA->setPropagateOpAndSyskeyColumns();

  RelExpr *refreshTree = buildCompleteRefreshTree(bindWA, mvInfo);
  if (bindWA->errStatus() || refreshTree==NULL) 
    return this;

  // Bind the refresh tree.
  refreshTree = refreshTree->bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  // If this is a multi-delta refresh, and we are not done yet - we need
  // to be called again with another phase.
  // Signal to the refresh utility using  warning.
  if (getAdditionalPhaseNeeded())
  {
    // 12304 An additional phase is needed to complete the refresh.
    *CmpCommon::diags() << DgSqlCode(12304);
  }

  // Delete all data members before saying goodbye.
  cleanupBeforeSelfDestruct();

  // Return the bound refresh tree to the root. The Refresh node itself has 
  // done its job, and is not part of the tree anymore.
  return refreshTree;
}

//////////////////////////////////////////////////////////////////////////////
// Since the destructor is never really called, at least clean up 
// internal data before disappearing. Should be called before returning 
// from bindNode().
//////////////////////////////////////////////////////////////////////////////
void Refresh::cleanupBeforeSelfDestruct()
{
  delete deltaDefList_;
  delete pipelineClause_;
  delete pNRowsClause_;
  delete bindContext_;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RelExpr *Refresh::buildCompleteRefreshTree(BindWA *bindWA, MVInfoForDML *mvInfo)
{
  CollHeap *heap = bindWA->wHeap();

  MvRefreshBuilder *firstBuilder = constructRefreshBuilder(bindWA, mvInfo);
  if (bindWA->errStatus())
    return NULL;

  RelExpr *topTree = firstBuilder->buildRefreshTree();

  // For recompute the refresh tree does not scan the log.
  // For MJV, it sometimes does, and when it does, it handles it itself.
  // So, for these types, the refresh tree is ready.
  if (refreshType_        == RECOMPUTE || 
      mvInfo->getMVType() == COM_MJV)
  {
    delete firstBuilder;
    return topTree;
  }

  MvBindContext *bindContext = new(heap) MvBindContext(heap);
  bindContext->setRefreshBuilder(firstBuilder);

  // In multi-delta, this is done inside the refresh tree.
  if (refreshType_ == SINGLEDELTA)
  {
    CMPASSERT(deltaDefList_->entries() == 1);
    DeltaDefinition *deltaDef = deltaDefList_->at(0);
    const QualifiedName *baseTableName = deltaDef->getTableName();

    RelExpr *scanLogBlock = 
      firstBuilder->buildLogsScanningBlock(*baseTableName);
    if (bindWA->errStatus() || scanLogBlock == NULL)
      return NULL;

    bindContext->setReplacementFor(baseTableName, scanLogBlock);
  }

  if (pipelineClause_ != NULL)
    topTree = constructPipelinedBuilders(bindWA, bindContext, topTree);
  if (topTree == NULL)
    return NULL;

  // Make sure no data is projected out.
  RelRoot *rootNode = new(heap) RelRoot(topTree);
#ifndef NDEBUG
  if (!getenv("DEBUG_DCB"))
#endif
    rootNode->setEmptySelectList();

  rootNode->setMvBindContext(bindContext);
  RelExpr *topNode = rootNode;

  // Save the bind context for cleanup.
  bindContext_ = bindContext;

  return topNode;
}  // Refresh::buildCompleteRefreshTree()

//////////////////////////////////////////////////////////////////////////////
// Determine the correct MvRefreshBuilder sub-class to handle the refresh job,
// construct, and return it.
// This method is called from two places in the code: 
//   1) From Refresh::bindNode() with isPipelined = FALSE, for a non-pipelined
//      refresh job.
//   2) From PipelinedMavBuilder::buildRefreshTree() with isPipelined = TRUE, 
//      for the lowest level of a pipelined refresh job.
//////////////////////////////////////////////////////////////////////////////
MvRefreshBuilder *Refresh::constructRefreshBuilder(BindWA       *bindWA,
						   MVInfoForDML *mvInfo)
{
  CollHeap *heap = bindWA->wHeap();

  CorrName  mvCorrName(mvName_);
  mvCorrName.setSpecialType(ExtendedQualName::MV_TABLE);

  MvRefreshBuilder *treeBuilder = NULL;
  switch (refreshType_)
  {
    case RECOMPUTE:
      {
	treeBuilder = new(heap) 
	  MvRecomputeBuilder(mvCorrName, mvInfo, noDeleteOnRecompute_, bindWA);
	break;
      }

    case SINGLEDELTA:
      // Verify no Multidelta in SINGLEDELTA definition.
      CMPASSERT(deltaDefList_->entries() == 1); 
      // Fall through to multidelta.

    case MULTIDELTA:
      CMPASSERT(mvInfo->getRefreshType() == COM_ON_REQUEST);

      switch (mvInfo->getMVType())
      {
	case COM_MAV:
	case COM_MAJV:
	  {
	    treeBuilder = 
	      constructMavSpecifichBuilder(bindWA, mvCorrName, mvInfo);
	    break;
	  }

	case COM_MJV:
	  {
	    treeBuilder = 
	      constructMjvSpecifichBuilder(bindWA, mvCorrName, mvInfo);
	    break;
	  }

	default:
	  CMPASSERT(FALSE);
      }
      break;

    default:
      // Unknown refresh type
      CMPASSERT(FALSE);
  }

  CMPASSERT(treeBuilder!=NULL);
  return treeBuilder;
}  // Refresh::constructRefreshBuilder()

//////////////////////////////////////////////////////////////////////////////
// Construct a refresh builder for a MAV.
//////////////////////////////////////////////////////////////////////////////
MvRefreshBuilder *Refresh::constructMavSpecifichBuilder(BindWA       *bindWA,
							CorrName      mvCorrName,
							MVInfoForDML *mvInfo)
{
  CollHeap *heap = bindWA->wHeap();
  MvRefreshBuilder *treeBuilder = NULL;

  // Is this MAV refresh pipelning its results to another refresh?
  NABoolean isProjectingMvDelta = (pipelineClause_ != NULL);

  if (deltaDefList_->entries() > 1)
  {
    // We have more than one delta, we need a multi-delta builder.
    treeBuilder = new(heap) 
      MultiDeltaMavBuilder(mvCorrName, mvInfo, this, isProjectingMvDelta, bindWA);
  }
  else if (getNRowsClause() != NULL)
  {
    // We have a "COMMIT EACH" clause, need the multi-txn builder.
    if (deltaDefList_->at(0)->useIudLog() && deltaDefList_->at(0)->useRangeLog() 
#ifndef NDEBUG // ?????? TEMP
      && !getenv( "REFRESH_WITH_DE" ) 
#endif
	)
    {
      treeBuilder = new(heap) 
	MultiTxnDEMavBuilder(mvCorrName, mvInfo, this, isProjectingMvDelta, bindWA);
#ifndef NDEBUG 
      cout << endl << " Executing Internal refresh with Duplicate elimination " << endl;
#endif
    }
    else
    {
      treeBuilder = new(heap) 
	MvMultiTxnMavBuilder(mvCorrName, mvInfo, this, isProjectingMvDelta, bindWA);
    }
  }
  else if (canUseMinMaxBuilder(bindWA, mvInfo))
  {
    // We have a Min/Max MAV with a supporting index.
    treeBuilder = new(heap) 
      MinMaxOptimizedMavBuilder(mvCorrName, mvInfo, this, isProjectingMvDelta, bindWA); 
  }
  else
  {
    // Just a simple MAV.
    treeBuilder = new(heap) 
      MavBuilder(mvCorrName, mvInfo, this, isProjectingMvDelta, bindWA);
  }

  return treeBuilder;
}

//////////////////////////////////////////////////////////////////////////////
// Construct a refresh builder for an MJV.
//////////////////////////////////////////////////////////////////////////////
MvRefreshBuilder *Refresh::constructMjvSpecifichBuilder(BindWA       *bindWA,
							CorrName      mvCorrName,
							MVInfoForDML *mvInfo)
{
  CollHeap *heap = bindWA->wHeap();
  MvRefreshBuilder *treeBuilder = NULL;

  if (deltaDefList_->entries() > 1)
  {
    // We have more than one delta, we need a multi-delta builder.
    treeBuilder = new(heap) 
      MjvOnRequestMultiDeltaBuilder(mvCorrName, mvInfo, this, bindWA);
  }
//  else if (getNRowsClause() != NULL)
//  {
//    // We have a "COMMIT EACH" clause, need the multi-txn builder.
//      treeBuilder = new(heap) 
//	MvMultiTxnMjvBuilder(mvCorrName, mvInfo, this, bindWA);
//  }
  else
  {
    // Just a simple MJV.
    treeBuilder = new(heap) 
      MjvOnRequestBuilder(mvCorrName, mvInfo, this, bindWA);
  }

  return treeBuilder;
}

//////////////////////////////////////////////////////////////////////////////
// Construct builders for all pipelined mvs, use them to build the pipelined 
// refresh trees and put the pointers to these trees in the bindContext.
//////////////////////////////////////////////////////////////////////////////
RelExpr *Refresh::constructPipelinedBuilders(BindWA        *bindWA, 
					     MvBindContext *bindContext,
					     RelExpr       *firstRefreshTree)
{
  const QualifiedName *prevMvName    = &mvName_;
  const QualifiedName *currentMvName = getNextLevelMv(mvName_);
  const QualifiedName *nextMvName    = NULL;
  CMPASSERT(currentMvName != NULL);
  RelExpr *topTree = firstRefreshTree;

  while (currentMvName != NULL)
  {
    CorrName  mvCorrName(*currentMvName);
    mvCorrName.setSpecialType(ExtendedQualName::MV_TABLE);

    MVInfoForDML *mvInfo = getMvInfo(bindWA, mvCorrName);
    if (mvInfo == NULL)
      return NULL;
    CMPASSERT(mvInfo->getMVType()==COM_MAV || mvInfo->getMVType()==COM_MAJV);

    nextMvName = getNextLevelMv(*currentMvName);
    NABoolean isPipelined = (nextMvName != NULL);
    PipelinedMavBuilder *pipelinedBuilder = new(bindWA->wHeap()) 
      PipelinedMavBuilder(mvCorrName, mvInfo, this, isPipelined, prevMvName, bindWA);

    RelExpr *pipelinedTree = 
      pipelinedBuilder->buildAndConnectPipeliningRefresh(topTree);

    bindContext->setReplacementFor(prevMvName, pipelinedTree);

    topTree = pipelinedBuilder->buildRefreshTree();

    // This builder is done.
    delete pipelinedBuilder;

    prevMvName    = currentMvName;
    currentMvName = nextMvName;
  }

  return topTree;
}


//////////////////////////////////////////////////////////////////////////////
// Do we need to use the special Min/Max builder?
// The answer is yes iff:
//   1. The MAV uses the MIN and/or MAX aggregate functions.
//   2. This is a single delta refresh.
//   3. This is a MAV on a single base table.
//   4. This base table has a supporting index on the MAV GroupBy columns.
//      (Supporting index means that the MAV GroupBy columns must be a prefix
//       of the index columns).
//////////////////////////////////////////////////////////////////////////////
NABoolean Refresh::canUseMinMaxBuilder(BindWA *bindWA, MVInfoForDML *mvInfo)
{
  // Check condition no. 1 
  if (mvInfo->isMinMaxUsed() == FALSE)
    return FALSE;

  // Verify condition no. 2 - we are not supposed to get here otherwise.
  CMPASSERT(deltaDefList_->entries() == 1);

  // Check condition no. 3
  if (!mvInfo->isMvOnSingleTable())
    return FALSE;

  // Check condition no. 4.
  if (!doesBaseTableHaveSupportingIndex(bindWA, mvInfo))
    return FALSE;
 
  return TRUE;
} // Refresh::canUseMinMaxBuilder()

//////////////////////////////////////////////////////////////////////////////
// Does this base table have a supporting index on the MAV GroupBy columns?
// (Supporting index means that the MAV GroupBy columns must be a prefix
// of the index columns).
//////////////////////////////////////////////////////////////////////////////
NABoolean Refresh::doesBaseTableHaveSupportingIndex(BindWA *bindWA, 
						    MVInfoForDML *mvInfo) const
{
  CollIndex i;
  LIST (MVUsedObjectInfo*)& UsedObjList = mvInfo->getUsedObjectsList();
  MVUsedObjectInfo* pUsedTable = UsedObjList[0];

  // Extract GroupBy columns
  const MVColumns& pMvInfoColumnList = 
    mvInfo->getMVColumns();
  LIST (MVColumnInfo *) mvGroupByColumns(bindWA->wHeap());
  for (	i = 0 ; i < pMvInfoColumnList.entries() ; i++)
  {
#pragma nowarn(1506)   // warning elimination 
    MVColumnInfo *currentMvColInfo = pMvInfoColumnList[i];
#pragma warn(1506)  // warning elimination 
    if (COM_MVCOL_GROUPBY == currentMvColInfo->getColType())
    {
      mvGroupByColumns.insert(currentMvColInfo);
    }
  }

  // If the MAV does not have any group by columns - than there is
  // no supporting index.
  if (mvGroupByColumns.entries() == 0)
    return FALSE;

  // Get the NATable
  QualifiedName underlyingTableName = pUsedTable->getObjectName();
  CorrName corrTableName(underlyingTableName);
  NATable * pNaTable = bindWA->getNATable(corrTableName, FALSE);
  
  // Construct table GroupBy
  NAColumnArray tableGroupByArray;
  const NAColumnArray & columnArray = pNaTable->getNAColumnArray();
  for ( i = 0 ; i < mvGroupByColumns.entries() ; i++)
  {
    Int32 tableColNum = (mvGroupByColumns)[i]->getOrigColNumber();
    NAColumn * pColumn = columnArray.getColumn(tableColNum);
    tableGroupByArray.insert(pColumn);
  }

  // Check the clustering Index.
  const NAFileSet *pClusteringIndexFileSet = pNaTable->getClusteringIndex();
  const NAColumnArray& ciColumns = 
    pClusteringIndexFileSet->getIndexKeyColumns();

  if (TRUE == areGroupByColsAnIndexPrefix(tableGroupByArray, ciColumns))
  {
    return TRUE;
  }

  // Check any secondary indices.
  const NAFileSetList & indexFileSetList = pNaTable->getIndexList();
  for ( i = 0 ; i < indexFileSetList.entries() ; i++)
  {
    const NAFileSet *pSecondaryIndexFileSet = indexFileSetList[i];
    const NAColumnArray& siColumns = 
      pSecondaryIndexFileSet->getIndexKeyColumns();

    if (TRUE == areGroupByColsAnIndexPrefix(tableGroupByArray, siColumns))
    {
      return TRUE;
    }
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// For a given vector of index columns, check if the GroupBy columns are 
// a prefix of the index, in any order.
//////////////////////////////////////////////////////////////////////////////
NABoolean Refresh::areGroupByColsAnIndexPrefix(const NAColumnArray & groupByCols,
				               const NAColumnArray & columnArray) const
{
  if(groupByCols.entries() > columnArray.entries())
    return FALSE;

  // For nGroupBys GroupBy columns, check that the first nGroupBys columns 
  // of the index are GroupBy columns.
  CollIndex nGroupBys = groupByCols.entries();
  for (CollIndex indexCol(0); indexCol < nGroupBys; indexCol++)
  {
    NABoolean foundIt = FALSE;
    const NAString& indexColName = columnArray[indexCol]->getColName();
    for (CollIndex groupByCol(0); groupByCol < nGroupBys; groupByCol++)
    {
      if(groupByCols[groupByCol]->getColName() == indexColName)
      {
	foundIt = TRUE;
	break;
      }
    }
    if (!foundIt)
      return FALSE;
  }
  
  return TRUE;
} // Refresh::groupByContainedAsPrefix

//////////////////////////////////////////////////////////////////////////////
// return the MV that fromMV is pipelining to.
//////////////////////////////////////////////////////////////////////////////
const QualifiedName *Refresh::getNextLevelMv(const QualifiedName& fromMV) const
{
  const PipelineClause *pipelineClause = getPipelineClause();

  if (fromMV == mvName_)
  {
    const QualNamePtrList *firstDef = pipelineClause->getFirstPipelineDef();
    CMPASSERT(firstDef->entries() == 1);
    return firstDef->at(0);
  }
  else
  {
    return pipelineClause->getNextLevelMv(fromMV);
  }
}  // Refresh::getNextLevelMv()

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
MVInfoForDML *Refresh::getMvInfo(BindWA *bindWA, CorrName& mvToRefresh) const
{
  // Get the NATable for the MV.
  NATable *mavNaTable = bindWA->getNATable(mvToRefresh);
  if (bindWA->errStatus()) 
    return NULL;
  CMPASSERT(mavNaTable!=NULL);
  CMPASSERT(mavNaTable->isAnMV());

  // If this is an incremental refresh, verify that the MV is initialized.
  // Recompute can be done on un-initialized.
  // Don't check if the MV is unavailable - the utility may have set it to
  // unavailable during the refresh operation.
  const ComMvAttributeBitmap& bitmap = mavNaTable->getMvAttributeBitmap();
  if (!bitmap.getIsMvUnAvailable() && bitmap.getIsMvUnInitialized())
    CMPASSERT(refreshType_ == RECOMPUTE);

  // Now get the MVInfo.
  MVInfoForDML *mvInfo = mavNaTable->getMVInfo(bindWA);
  CMPASSERT(mvInfo != NULL);
  mvInfo->initUsedObjectsHash();

  return mvInfo;
}  // Refresh::getMvInfo()

//////////////////////////////////////////////////////////////////////////////
// Verify that all the tables in the delta definition list are indeed used
// by this MV.
//////////////////////////////////////////////////////////////////////////////
NABoolean Refresh::verifyDeltaDefinition(BindWA *bindWA, MVInfoForDML *mvInfo) const
{
  const DeltaDefinitionPtrList *deltaDefs = getDeltaDefList();
  if (deltaDefs == NULL)
    return FALSE;

  for (CollIndex i=0; i<deltaDefs->entries(); i++)
  {
    const QualifiedName *tableName = deltaDefs->at(i)->getTableName();
    if (mvInfo->findUsedInfoForTable(*tableName) == NULL)
    {
      // 12314 Table $0~TableName is not used by this materialized view.
      *CmpCommon::diags() << DgSqlCode(-12314)
	  << DgTableName(tableName->getQualifiedNameAsString());
      bindWA->setErrStatus();
      return TRUE;
    }
  }

  return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
const NAString Refresh::getText() const
{
  return "refresh";
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Exclude from coverage testing - Must be implemented as a RelExpr subclass, but not used.
RelExpr *Refresh::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  Refresh *result = NULL;

  if (derivedNode == NULL)
  {
    switch (refreshType_)
    {
      case RECOMPUTE:	
	result = new(outHeap) Refresh(mvName_, 
	                              noDeleteOnRecompute_,
				      outHeap);
	break;

      case SINGLEDELTA:	
	result = new(outHeap) Refresh(mvName_, 
	                              deltaDefList_, 
				      pNRowsClause_, 
				      pipelineClause_, 
				      outHeap);
	break;

      case MULTIDELTA:
	result = new(outHeap) Refresh(mvName_, 
	                              deltaDefList_, 
				      phase_, 
				      pipelineClause_, 
				      outHeap);
	break;

      default: CMPASSERT(FALSE);
    }
  }
  else
    result = (Refresh *) derivedNode;

  return BinderOnlyNode::copyTopNode(result, outHeap);
}  // Refresh::copyTopNode()

//////////////////////////////////////////////////////////////////////////////
// Return the DeltaDefinition for the table named 'name'.
//////////////////////////////////////////////////////////////////////////////
DeltaDefinition *Refresh::getDeltaDefinitionFor(const QualifiedName& name)
{
  return deltaDefList_->findEntryFor(name);
}  // Refresh::getDeltaDefinitionFor()

//////////////////////////////////////////////////////////////////////////////
// Below this line it's Ori's code...
//////////////////////////////////////////////////////////////////////////////


//----------------------------------------------------------------------------
//class PipelineClause

//----------------------------------------------------------------------------
PipelineClause::~PipelineClause() 
{
  delete firstPipelineDef_;
  delete pipelineDefList_;
}

//----------------------------------------------------------------------------
// Apply the default catalog and schema to all the MV names.
// The syntax supports multi-parent pipelining.
void PipelineClause::applyDefaultSchemaToAll(BindWA *bindWA)
{
  const SchemaName &defaultSchema = bindWA->getDefaultSchema();

  const QualNamePtrList& nameList=*firstPipelineDef_;
  for (CollIndex i=0; i<nameList.entries(); i++)
    nameList[i]->applyDefaults(defaultSchema);

  if (pipelineDefList_ == NULL)
    return;

  for (CollIndex j=0; j<pipelineDefList_->entries(); j++)
  {
    pipelineDefList_->at(j)->getFromClause()->applyDefaults(defaultSchema);
    QualNamePtrList *nameList2 = pipelineDefList_->at(j)->getToClause();
    for (CollIndex k=0; k<nameList.entries(); k++)
      nameList2->at(k)->applyDefaults(defaultSchema);
  }
}

//----------------------------------------------------------------------------
// return the MV that fromMV is pipelining to.
const QualifiedName *PipelineClause::getNextLevelMv(const QualifiedName& fromMV) const
{
  if (pipelineDefList_ != NULL)
  {
    // Go through the next levels to find toMV.
    for (CollIndex i=0; i<pipelineDefList_->entries(); i++)
    {
      const PipelineDef *pipelineLevel = pipelineDefList_->at(i);
      if (*pipelineLevel->getFromClause() == fromMV)
      {
	const QualNamePtrList *pipelineToList = pipelineLevel->getToClause();
	CMPASSERT(pipelineToList->entries() == 1);
	return pipelineToList->at(0);
      }
    }
  }

  // If we get here - This is the top most MV, that is not pipelined anymore.
  return NULL;
}

//----------------------------------------------------------------------------
//class DeltaDefinition

DeltaDefinition::~DeltaDefinition()
{
  delete updatedColumnList_;
  delete pDeltaOptions_;
}

void DeltaDefinition::synthesize()
{
  DeltaDefLogs *pDeltaDefLogs = NULL;
  
  switch(pDeltaOptions_->getDELevel())
  {
    case DeltaOptions::NO_DE:
      insertOnly_ = pDeltaOptions_->getIsInsertOnly();
      break;
	    
    case DeltaOptions::RANGE_RESOLUTION_ONLY:
    case DeltaOptions::RANGE_AND_CROSS_TYPE_RESOLUTIONS:
    case DeltaOptions::ALL:
      pDeltaDefLogs = pDeltaOptions_->getDeltaStats();
      break;
	    
    default:
      CMPASSERT(FALSE);
  }
  
  if(NULL == pDeltaDefLogs)
  {
    return;
  }
	
  // RANGE DEF
  switch(pDeltaDefLogs->getRangeDef()->getOption())
  {
    case DeltaDefRangeLog::NO_LOG:
      useRangeLog_ = FALSE;
      break;

    case DeltaDefRangeLog::ALL:
      coveredRows_ = pDeltaDefLogs->getRangeDef()->getCoveredRows();
      //no need to break;

    case DeltaDefRangeLog::CARDINALITY_ONLY:
      numOfRanges_ = pDeltaDefLogs->getRangeDef()->getNumOfRanges();
      useRangeLog_ = TRUE;
      break;

    default:
      CMPASSERT(FALSE);

  }

  // IUD DEF
  switch(pDeltaDefLogs->getIUDDef()->getOption())
  {
  case DeltaDefIUDLog::NO_LOG:
    useIudLog_ = FALSE;
    break;
  case DeltaDefIUDLog::INSERT_ONLY:
  case DeltaDefIUDLog::NO_STAT:
    useIudLog_ = TRUE;
    break;
  case DeltaDefIUDLog::STAT:
    useIudLog_ = TRUE;
    iudInsertedRows_ = 
      pDeltaDefLogs->getIUDDef()->getStatistics()->getNumRowsInserted();
    iudDeletedRows_ =  
      pDeltaDefLogs->getIUDDef()->getStatistics()->getNumRowsDeleted();
    iudUpdatedRows_ =  
      pDeltaDefLogs->getIUDDef()->getStatistics()->getNumRowsUpdated();
    updatedColumnList_ = 
      pDeltaDefLogs->getIUDDef()->getStatistics()->getOptionalColumnList();
    break;

  default:
    CMPASSERT(FALSE);
  }
} // DeltaDefinition::synthesize


//  Returns TRUE if all the updated columns of this delta are not used by MV
NABoolean DeltaDefinition::updateColumnsNotUsedByMV(MVInfoForDML *mvInfo)
{
  processUpdateColumns(mvInfo);
  return !containsUpdateColUsedByMv_;
}

// Process update columns of the delta to populate data members
// containsIndirectUpdateColumn_, containsDirectUpdateColumn_ and
// containsUpdateColUsedByMv_
void DeltaDefinition::processUpdateColumns(MVInfoForDML *mvInfo)
{
  if (updateColumnsProcessed_)
    return;

  mvInfo->initUsedObjectsHash();
  const MVUsedObjectInfo *usedObject = mvInfo->
                              findUsedInfoForTable(*getTableName());

  const IntegerList *updatedCols = getUpdatedColumnList();
  if (updatedCols==NULL)
  {
    // We have no statistics on updated column - we have to assume the worst case.
    containsIndirectUpdateColumn_ = TRUE;
    containsDirectUpdateColumn_   = TRUE;
    containsUpdateColUsedByMv_    = TRUE;
  }
  else
  {
    for (CollIndex i = 0; i < updatedCols->entries(); i++ )
    {
      Lng32 updatedCol = updatedCols->at(i);

      if (usedObject->isIndirectUpdateCol(updatedCol))
      {
	containsUpdateColUsedByMv_ = TRUE;
	containsIndirectUpdateColumn_ = TRUE;
      }
      else if (usedObject->isUsedColumn(updatedCol))
      {
	containsUpdateColUsedByMv_ = TRUE;
	containsDirectUpdateColumn_ = TRUE;
      }

      // if already seen one DIRECT update column, one INDIRECT update column
      // and one update column that is used by the MV
      if (containsIndirectUpdateColumn_ &&
	  containsDirectUpdateColumn_ &&
	  containsUpdateColUsedByMv_)
	break;
    }
  }

  updateColumnsProcessed_ = TRUE;
}

// Returns TRUE if the updated columns of this delta contains a 
// DIRECT update column
NABoolean DeltaDefinition::containsDirectUpdateColumn(MVInfoForDML *mvInfo)
{
  processUpdateColumns(mvInfo);
  return containsDirectUpdateColumn_;
}


// Returns TRUE if the updated columns of this delta contains 
// an INDIRECT update column
NABoolean DeltaDefinition::containsIndirectUpdateColumn(MVInfoForDML *mvInfo)
{
  processUpdateColumns(mvInfo);
  return containsIndirectUpdateColumn_;
}


//----------------------------------------------------------------------------
// Class DeltaDefinitionPtrList

void DeltaDefinitionPtrList::applyDefaultSchemaToAll(BindWA *bindWA) const
{
  const SchemaName &defaultSchema = bindWA->getDefaultSchema();
  for (CollIndex i=0; i<entries(); i++)
    at(i)->getTableName()->applyDefaults(defaultSchema);
}

DeltaDefinition *DeltaDefinitionPtrList::findEntryFor(const QualifiedName& tableName) const
{
  for (CollIndex i=0; i<entries(); i++)
  {
    DeltaDefinition *deltaDef = at(i);
    if (tableName == *deltaDef->getTableName())
      return deltaDef;
  }
  return NULL;
}

NABoolean DeltaDefinitionPtrList::areAllDeltasInsertOnly() const
{
  for (CollIndex i=0; i<entries(); i++)
    if (!at(i)->isInsertOnly())
      return FALSE;

  return TRUE;
}

NABoolean DeltaDefinitionPtrList::areAllDeltasWithFullDE() const
{
  for (CollIndex i=0; i<entries(); i++)
    if (!at(i)->isFullDE())
      return FALSE;

  return TRUE;
}

