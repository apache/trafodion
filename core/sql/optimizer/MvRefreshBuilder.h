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
#ifndef REFRESHBUILDER_H
#define REFRESHBUILDER_H

/* -*-C++-*-
******************************************************************************
*
* File:         MvRefreshBuilder.h
* Description:  Definition of class Refresh for MV INTERNAL REFRESH command.
*
* Created:      12/27/2000
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/


#include "ComMvDefs.h"
#include "RelMisc.h"
#include "Refresh.h"
#include "ChangesTable.h"

// classes defined in this file
class MvRefreshBuilder;
class MvRecomputeBuilder;
class MavBuilder;
class MinMaxMavBuilder;
class MultiDeltaMavBuilder;
class MvMultiTxnMavBuilder;
class PipelinedMavBuilder;
class MultiDeltaRefreshMatrix;
class MultiDeltaRefreshMatrixRow;
class MavRelRootBuilder;
struct MinMaxColExtraInfo;

// Forward references 
class LogsInfo;
class DeltaDefinitionPtrList;
class IntegerList;
class CorrName;
class QualifiedName;
class Refresh;
class MVColumnInfo;
class MvIudLog;
class MVJoinGraph;
class Union;
class RelSequence;

//----------------------------------------------------------------------------
// The MvRefreshBuilder class hierarchy is responsible for the actual
// inlining work of building the refresh RelExpr trees.
// For ON REQUEST MVs, if the refresh is not pipelined, only one object is 
// built and used. In case of pipelined refresh, several objects are built, 
// each for a single level of MV refresh.
// The MvRefreshBuilder class itself is abstract.
// Here is the class hierarchy:
// MvRefreshBuilder :
//     MvRecomputeBuilder   
//     MjvBuilder (defined in MjvBuilder.h/cpp)
//         MjvImmInsertBuilder
//         MjvImmDeleteBuilder
//         MjvImmDirectUpdateBuilder
//     MavBuilder     
//         MultiDeltaMavBuilder
//         MvMultiTxnMavBuilder
//         MinMaxMavBuilder
//         PipelinedMavBuilder
//----------------------------------------------------------------------------
class MvRefreshBuilder : public NABasicObject
{
public:

  MvRefreshBuilder(const CorrName&  mvName,
		   MVInfoForDML    *mvInfo,
		   Refresh	   *refreshNode,
		   BindWA          *bindWA);

private:
  // Copy Ctor and = operator are not implemented.
  MvRefreshBuilder(const MvRefreshBuilder& other);
#pragma nowarn(1026)   // warning elimination 
  MvRefreshBuilder& operator=(const MvRefreshBuilder& other);
#pragma warn(1026)  // warning elimination 

public:
  enum { MAX_EPOCH_FOR_UNION_BACKBONE = 50 };

public:
  virtual ~MvRefreshBuilder();

  // Accessors to private data members
  const CorrName&     getMvCorrName()  const { return mvCorrName_; }
  Refresh            *getRefreshNode() const { return refreshNode_; }
  MVInfoForDML       *getMvInfo()      const { return mvInfo_; }
  Lng32		      getPhase()       const { return phase_; }
  const DeltaDefinitionPtrList *getDeltaDefList() const 
  { return deltaDefList_;}
  void  setDeltaDefList(const DeltaDefinitionPtrList *deltaDef)
  { deltaDefList_ = deltaDef;}

  // Build the refresh RelExpr tree. This pure virtual method is respossible
  // for building the main refresh tree. This is done differently for every 
  // refresh type.
  virtual RelExpr *buildRefreshTree() = 0;

  // Transform the Scan on the base table from the MV SELECT tree, to scan
  // the logs instead. This is the basic implementation that handles the IUD
  // log and the range log. It is overridden by MvMultiTxnMavBuilder for
  // that special case.
  virtual RelExpr *buildLogsScanningBlock(const QualifiedName&  baseTable);

  virtual NABoolean needAlternateCIorder() const;
  
  virtual Union *buildUnionBetweenRangeAndIudBlocks(RelExpr *scanIUDLogBlock,
						    RelExpr *scanRangeLogBlock) const;

protected:

  virtual RelRoot *buildRootWithUniformSelectList(RelExpr *topNode,
					          ItemExpr *opExpr,
					          const CorrName *nameOverride,
                                                  NABoolean forRange) const;

  LogsInfo &getLogsInfo() const
  {
    CMPASSERT(logsInfo_);
    return *logsInfo_;
  }

  // Prepare for Multi-Delta refresh (both MAV and MJV)
  MultiDeltaRefreshMatrix *prepareProductMatrix(NABoolean supportPhases, 
                                                NABoolean checkOnlyInserts);

  // Prepare a single join product according to the product matrix row.
  RelRoot *prepareProductFromMatrix(
			  const MultiDeltaRefreshMatrix& productMatrix,
			  RelRoot *rawJoinProduct,
			  Int32 rowNumber,
			  NABoolean isLast);

  // Any reason not to implement multi-delta optimization? 
  // (implemented differently by MAV and MJV)
  virtual NABoolean avoidMultiDeltaOptimization() { return FALSE; };

  // Methods called internally for reading the IUD log
  // ----------------------------------------------------

  virtual RelExpr *buildReadIudLogBlock();
 
  virtual Int32 getNumOfScansInUnionBackbone() const;

  void fixScanCardinality(RelExpr *node,
    			  double cardinalityFactor, 
			  Cardinality cardinality) const;

  // Construct the appropriate expression for the Op@ virtual column.
  // The value of this column is 1 for inserted rows, and -1 for deleted rows.
  ItemExpr *constructOpExpression() const;

  // buildReadIudLogBlock callee
  virtual ItemExpr *buildSelectionPredicateForScanOnIudLog() const;

  virtual ItemExpr *buildSelectionListForScanOnIudLog() const;

  virtual ItemExpr *buildBaseTableColumnList(Lng32 specialFlags=0) const;

  // Have a uniform select list over the IUD log 
  virtual RelRoot *buildRootOverIUDLog(RelExpr *topNode) const;

  // Methods called internally for reading the range log
  // ----------------------------------------------------

  virtual RelExpr *buildReadBaseTable() const;

  // Construct a join between the base table and the range log
  virtual RelExpr *buildReadRangesBlock() const;
  
  virtual ItemExpr *buildEndRangeVector() const;

  virtual ItemExpr *buildBeginRangeVector() const;
  
  // Construct a join between the base table and the range log
  virtual RelExpr *buildReadRangeLogBlock() const;
  
  // buildReadRangeLogBlock() callee
  virtual RelRoot *buildRootOverRangeBlock(RelExpr *topNode) const;
  // buildReadRangeLogBlock() callee
  virtual RelExpr *buildJoinBaseTableWithRangeLog(RelExpr *scanRangeLog, 
						  RelExpr *scanBaseTable) const;

  // Used by buildUnionWithRangeLog(), Overridden by MvMultiTxnMavBuilder
  virtual ItemExpr *buildSelectionPredicateForScanOnRangeLog() const;

  // Construct the join predicate between the base table and the range log.
  // Overridden by MvMultiTxnMavBuilder for additional predicates.
  virtual ItemExpr *buildRangeLogJoinPredicate() const;

  virtual ItemExpr *buildRangeLogJoinPredicateWithCols(
					       ItemExpr *rangeType,
					       ItemExpr *baseCI,
					       ItemExpr *beginRangeCI,
					       ItemExpr *endRangeCI) const;


  virtual NABoolean useUnionBakeboneToMergeEpochs() const;

  virtual RelExpr  *buildUnionBakeboneToMergeEpochs(RelExpr *topNode) const;

  virtual Int32 isGroupByAprefixOfTableCKeyColumns() const { return FALSE; }

  virtual ItemExpr *buildAlternateCIorder(ItemExpr *ciColumns, const CorrName &tableNameCorr) const;

  // Was INTERNAL REFRESH invoked with DE LEVEL 3?
  NABoolean wasFullDE() const;

  // The statement heap (taken from bindWA).
  CollHeap		*heap_;

  BindWA *bindWA_;

private:

  // Data members.
  // ---------------------------------------------------- 
  // The name of the MV being refreshed.
  CorrName		 mvCorrName_;
  // The Delta Definitions of the logs used.
  const DeltaDefinitionPtrList *deltaDefList_;  
  // Which phase of multi-delta refresh is this invocation?
  Lng32		         phase_;			 
  MVInfoForDML		*mvInfo_;
  Refresh		*refreshNode_;
  LogsInfo		*logsInfo_;

};  // class MvRefreshBuilder

//----------------------------------------------------------------------------
// The simplest Refresh tree: RECOMPUTE.
//----------------------------------------------------------------------------
class MvRecomputeBuilder : public MvRefreshBuilder
{
public:
  MvRecomputeBuilder(const CorrName&	     mvName,
		     MVInfoForDML	    *mvInfo,
		     NABoolean		     noDeleteOnRecompute,
		     BindWA                 *bindWA)
  : MvRefreshBuilder(mvName, mvInfo, NULL, bindWA),
    noDeleteOnRecompute_(noDeleteOnRecompute)
  {}

  virtual ~MvRecomputeBuilder() {}

private:
  // Copy Ctor and = operator are not implemented.
  MvRecomputeBuilder(const MvRecomputeBuilder& other);
#pragma nowarn(1026)   // warning elimination 
  MvRecomputeBuilder& operator=(const MvRecomputeBuilder& other);
#pragma warn(1026)  // warning elimination 

public:
  virtual RelExpr *buildRefreshTree();

private:


  RelExpr *buildInsertToMvSubTree() const;
  RelExpr *buildDeleteFromMvSubTree() const;

private:
  NABoolean		  noDeleteOnRecompute_;

};  // class MvRecomputeBuilder

//----------------------------------------------------------------------------
// Builds the refresh tree of a single-delta MAV. 
// This MAV reads the logs of its base tables for the refresh operation.
//----------------------------------------------------------------------------
class MavBuilder : public MvRefreshBuilder
{
public:
  MavBuilder(const CorrName&  mvName,
	     MVInfoForDML    *mvInfo,
	     Refresh	     *refreshNode,
	     NABoolean	      isProjectingMvDelta,
	     BindWA          *bindWA);

private:
  // Copy Ctor and = operator are not implemented.
  MavBuilder(const MavBuilder& other);
#pragma nowarn(1026)   // warning elimination 
  MavBuilder& operator=(const MavBuilder& other);
#pragma warn(1026)  // warning elimination 

public:
  enum  GroupOpNumbers { GOP_INSERT = 1, 
			 GOP_DELETE, 
			 GOP_UPDATE,
			 GOP_MINMAX_RECOMPUTE_FROM_UPDATE,
			 GOP_MINMAX_RECOMPUTE_FROM_INSERT };

  virtual ~MavBuilder() {}

  // The implementation-specific part of building the MAV refresh tree.
  // This method is overridden by every sub-class.
  virtual RelExpr *buildRefreshTree();

  static const char *getVirtualOpColumnName()  { return virtualOpColumnName_; }
  static const char *getVirtualGopColumnName() { return virtualGopColumnName_; }
  static const char *getVirtualIsLastColName() { return virtualIsLastColumnName_; }

  // These are correlation names to "tables".
  static const char *getSysDeltaName()  { return sysDeltaName_; }
  static const char *getSysCalcName()   { return sysCalcName_; }
  static const char *getSysMavName()    { return sysMavName_; }
  static const char *getStartCtxName()  { return startCtxName_; }
  static const char *getEndCtxName()    { return endCtxName_; }
  static const char *getMinMaxMavName() { return minMaxMavName_; }

  // These are name suffixes of extra aggregate columns added for each 
  // Min/Max column.
  static const char *getExtraColSuffixForIns() { return extraColSuffixForIns_; }
  static const char *getExtraColSuffixForDel() { return extraColSuffixForDel_; }

protected:  

  // Internal methods used by buildRefreshTree().

  // The actual building of the refresh tree, that is the same for all 
  // MAV sub-classes.
  RelExpr *buildTheMavRefreshTree(RelExpr         *mvSelectTree,
				  DeltaDefinition *deltaDef);

  // build the left side of the refresh tree - the DCB.
  RelExpr *buildDeltaCalculationBlock(RelExpr *mvSelectTree);

  // Construct a selection predicate on the MAV group by columns.
  ItemExpr *buildGroupByColumnsPredicate(const NAString& table1Name, 
					 const NAString& table2Name = "",
					 const NABoolean ignoreAlias = FALSE) const;

  // build the right side of the refresh tree - the DPB.
  RelExpr  *buildDeltaProcessingBlock(DeltaDefinition *deltaDef);

  // Build the Delta Processing Block Insert sub-tree.
  RelExpr  *buildDPBInsert(DeltaDefinition *deltaDef, RelExpr *topNode) const;
  RelExpr  *buildDPBInsertNodes(const NAString& sourceTable) const;

  // Build the Delta Processing Block Delete sub-tree.
  RelExpr  *buildDPBDelete(RelExpr *topNode);
  
  // Build the Delta Processing Block Update sub-tree.
  // The Update expressions are of the form:
  // (a is a MAV aggregate column, b is a MAV groupby column)
  // SET   expressions: a = tableForSet.a 
  // WHERE expressions: b = tableForWhere.b
  RelExpr  *buildDPBUpdate(const NAString& tableForSet,
			   const NAString& tableForWhere) const;

  // Build a stack of 5 RelRoot nodes to calculate the SYS_CALC columns.
  RelExpr *buildLotsOfRootsForCalcCalculation(RelExpr *topNode, RelExpr *mvSelectTree);

  NABoolean isProjectingMvDelta() const { return isProjectingMvDelta_; }

  virtual NABoolean useUnionBakeboneToMergeEpochs() const;
	
  virtual Int32 isGroupByAprefixOfTableCKeyColumns() const;
  
  // Accessors for static data members
  const char *getGopTableName()  const { return gopTableName_; }
  const char *getGopCol1Name()   const { return gopCol1Name_; }
  const char *getGopCol2Name()   const { return gopCol2Name_; }

private:

  // All the methods below are used for the generic support of MIN/MAX.

  // Delta Processing Block
  RelExpr *buildDPBMinMaxIfCondition(const RelExpr *updateNode) const;
  
  RelExpr *createSignal() const; 

  // Builds a Signal for this class.
  // Overridden by MinMaxOptimizedMavBuilder.
  virtual RelExpr *buildMinMaxRecomputationBlock() const;

private:
  // TRUE if this is not the top most refresh builder.
  NABoolean       isProjectingMvDelta_;

  // TRUE when there is no need to check if the Min/Max value was deleted.
  NABoolean       canSkipMinMax_;

  // This attribute has lazy evaluation, thus we use int instead of boolean.
  // -1 is unintialized, 0 is false,1 is true
  Int32 isGroupByAprefixOfTableCKeyColumns_;

  // This is needed for accessing the NATable of the base tables for
  // MIN/MAX optimizations.
  BindWA *pBindWA; 

  // "Virtual" columns are columns that are manualy added to the RETDesc.
  static const char virtualOpColumnName_[];
  static const char virtualGopColumnName_[];
  static const char virtualIsLastColumnName_[];

  // These are correlation names to "tables".
  static const char sysDeltaName_[];
  static const char sysCalcName_[];
  static const char sysMavName_[];
  static const char startCtxName_[];
  static const char endCtxName_[];
  static const char minMaxMavName_[];
  
  // These names are used in pipelined refresh, for the TupleList join.
  static const char gopTableName_[];
  static const char gopCol1Name_[];
  static const char gopCol2Name_[];

  // These are name suffixes of extra aggregate columns added for each 
  // Min/Max column.
  static const char extraColSuffixForIns_[];
  static const char extraColSuffixForDel_[];
};  // class MavBuilder


//----------------------------------------------------------------------------
class MinMaxOptimizedMavBuilder : public MavBuilder
{

public:
  MinMaxOptimizedMavBuilder(const CorrName&  mvName,
		   MVInfoForDML    *mvInfo,
		   Refresh         *refreshNode,
		   NABoolean        isProjectingMvDelta,
		   BindWA          *bindWA)
  : MavBuilder(mvName, mvInfo, refreshNode, isProjectingMvDelta, bindWA)
  {}

  virtual ~MinMaxOptimizedMavBuilder() {}

private:
  // Copy Ctor and = operator are not implemented.
  MinMaxOptimizedMavBuilder(const MinMaxOptimizedMavBuilder& other);
#pragma nowarn(1026)   // warning elimination 
  MinMaxOptimizedMavBuilder& operator=(const MinMaxOptimizedMavBuilder& other);
#pragma warn(1026)  // warning elimination 

protected:
  virtual RelExpr *buildMinMaxRecomputationBlock() const;

  void fixGroupingColumns(RelRoot* pRoot) const;

  void removeGroupingColsFromSelectList(RelRoot* pRoot) const;
}; // MinMaxOptimizedMavBuilder


//----------------------------------------------------------------------------
class MultiDeltaMavBuilder : public MavBuilder
{
public:
  MultiDeltaMavBuilder(const CorrName&  mvName,
		       MVInfoForDML    *mvInfo,
		       Refresh         *refreshNode,
		       NABoolean        isProjectingMvDelta,
		       BindWA          *bindWA)
  : MavBuilder(mvName, mvInfo, refreshNode, isProjectingMvDelta, bindWA),
    isDuplicatesOptimized_(FALSE)
  {}

private:
  // Copy Ctor and = operator are not implemented.
  MultiDeltaMavBuilder(const MultiDeltaMavBuilder& other);
#pragma nowarn(1026)   // warning elimination 
  MultiDeltaMavBuilder& operator=(const MultiDeltaMavBuilder& other);
#pragma warn(1026)  // warning elimination 

public:

  virtual ~MultiDeltaMavBuilder() {}

  virtual RelExpr *buildRefreshTree();

protected:
  // Any reason not to implement multi-delta optimization? 
  // (implemented differently by MAV and MJV)
  virtual NABoolean avoidMultiDeltaOptimization();

  virtual NABoolean isTableExpressionPresent(RelExpr *currentNode);

private:
  void bindJoinProduct(RelRoot  *product, NABoolean isSignPlus);

  RelExpr *buildRawDeltaCalculationTree(
			  const MultiDeltaRefreshMatrix& productMatrix);

  void prepareRetdescForUnion(RETDesc  *retDesc, NABoolean isSignPlus);

  NABoolean collectOpExpressions(RETDesc           *retDesc,
	    		         const ColumnDesc  *columnDesc,
				 ItemExpr          *&newOpExpr);

  RelExpr *getMvSelectTree() { return mvSelectTree_; }

private:
  NABoolean isDuplicatesOptimized_;
  RelExpr *mvSelectTree_;
};  // class MultiDeltaMavBuilder

//----------------------------------------------------------------------------
class MvMultiTxnMavBuilder : public MavBuilder
{
public:
  MvMultiTxnMavBuilder(const CorrName&  mvName,
		       MVInfoForDML    *mvInfo,
		       Refresh	       *refreshNode,
		       NABoolean        isProjectingMvDelta,
		       BindWA          *bindWA)
  : MavBuilder(mvName, mvInfo, refreshNode, isProjectingMvDelta, bindWA),
    pMultiTxnClause_(refreshNode->getNRowsClause())
  {}

private:
  // Copy Ctor and = operator are not implemented.
  MvMultiTxnMavBuilder(const MvMultiTxnMavBuilder& other);
#pragma nowarn(1026)   // warning elimination 
  MvMultiTxnMavBuilder& operator=(const MvMultiTxnMavBuilder& other);
#pragma warn(1026)  // warning elimination 

public:

  virtual ~MvMultiTxnMavBuilder() {}

  virtual RelExpr *buildRefreshTree();

  RelExpr *buildLogsScanningBlock(const QualifiedName& baseTable);

protected:

  virtual RelExpr  *buildReadPreviousContext();

  RelExpr *buildErrorOnNoContext(RelExpr *topNode);
  RelExpr *buildPhase1SelectList(RelExpr *topNode, const NATable *baseNaTable);

  virtual RelExpr  *buildReadIudLogBlock() ;

  // buildReadIudLogBlock callee
  virtual ItemExpr *buildSelectionPredicateForScanOnIudLog() const;

  virtual NABoolean needAlternateCIorder() const;

  virtual Union	   *buildUnionBetweenRangeAndIudBlocks(RelExpr *scanIUDLogBlock,
						       RelExpr *scanRangeLogBlock) const;

  virtual ItemExpr *buildSelectionPredicateForScanOnRangeLog() const;

  virtual ItemExpr *buildCatchupSelectionPredicateForScanOnRangeLog() const;
  
  virtual ItemExpr *buildPhase1SelectionPredicateForScanOnRangeLog() const;

  virtual ItemExpr *buildSelectionListForScanOnIudLog() const;

  virtual ItemExpr *buildRangeLogJoinPredicate() const;

  virtual ItemExpr *addContextPredicatesOnIUDLog() const;

  virtual NAString *getSequenceByPrefixColName() const
  {
    return NULL;
  }

  virtual NABoolean useUnionBakeboneToMergeEpochs() const;

  virtual RelSequence *buildSequenceOnScan(RelExpr *topNode, ItemExpr *isLastExpr) const;
  
  virtual RelRoot  *buildRootOverSequence(RelExpr *topNode, ItemExpr *isLastExpr) const;

  virtual ItemExpr *buildSequenceIsLastExpr() const;

  virtual RelExpr  *buildInsertContextNode();

  virtual RelExpr  *buildInsertContextTree(RelExpr  *leftTopNode);

  const NRowsClause *pMultiTxnClause_;

};  // class MvMultiTxnMavBuilder

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Exclude from coverage testing - used only with range loggiing
class MultiTxnDEMavBuilder : public MvMultiTxnMavBuilder
{
public:
    MultiTxnDEMavBuilder(const CorrName&  mvName,
		     MVInfoForDML    *mvInfo,
		     Refresh	     *refreshNode,
		     NABoolean	      isProjectingMvDelta,
		     BindWA          *bindWA)
  : MvMultiTxnMavBuilder(mvName, mvInfo, refreshNode, isProjectingMvDelta, bindWA)
  {}

private:
  // Copy Ctor and = operator are not implemented.
  MultiTxnDEMavBuilder(const MultiTxnDEMavBuilder& other);
#pragma nowarn(1026)   // warning elimination 
  MultiTxnDEMavBuilder& operator=(const MultiTxnDEMavBuilder& other);
#pragma warn(1026)  // warning elimination 

public:

  virtual ~MultiTxnDEMavBuilder() {}

protected:
  virtual const char  *getVirtualRangeSpecialCol() const { return virtualRangeSpecialCol_; }
  
  virtual NAString    *getSequenceByPrefixColName() const 
  { 
    return new(heap_) NAString(sequenceByPrefixColName_); 
  }

  virtual NAString    *getLastNotNullPrefixColName() const 
  { 
    return new(heap_) NAString(lastNotNullPrefixColName_); 
  }
  
  // The Range of values that @SPECIAL can recieve
  enum {
    VIRTUAL_BEGIN_RANGE,		
    TABLE_ROWS,			
    IUD_LOG_ROWS		
  };

  // Have a uniform select list over the IUD log 
  virtual RelRoot     *buildRootOverIUDLog(RelExpr *topNode) const;

  // Have a uniform select list over the Range log 
  virtual RelRoot     *buildRootOverRangeBlock(RelExpr *topNode) const;

  ItemExpr	      *buildComputedTableSyskey(ItemExpr *colExpr) const;

  void		      buildSortKeyColumnsForRangeBlock(ItemExprList &sortKeyCols) const;

  void		      addSyskeyToSortKeyColumnsForRangeBlock(ItemExprList &sortKeyCols) const;
  
  RelRoot	      *buildRootWithFilterForEmptyRanges(RelExpr  *topNode) const;

  virtual RelSequence *buildSequenceOnScan(RelExpr *topNode, ItemExpr *isLastExpr) const;

  virtual RelRoot     *buildRootOverSequence(RelExpr *topNode, ItemExpr *isLastExpr) const;

  RelRoot	      *buildFinalRootOverSequence(RelExpr *topNode) const;

  ItemExpr	      *buildSelectionOverSequence() const;

  ItemExpr	      *buildIsCoveredByRange() const;
  
  ItemExpr	      *buildIsPhysicallyCoveredByRangeBoundries() const;
  
  // buildReadRangeLogBlock() callee
  virtual RelExpr     *buildJoinBaseTableWithRangeLog(RelExpr *scanRangeLog, 
						      RelExpr *scanBaseTable) const;

  virtual RelExpr     *buildReadRangeLogBlock() const;

private:
  static const char   virtualRangeSpecialCol_[];
  static const char   sequenceByPrefixColName_[];
  static const char   lastNotNullPrefixColName_[];

};  // MultiTxnDEMavBuilder

//----------------------------------------------------------------------------
// Builds the refresh tree of a single-delta pipelined MAV. 
// This MAV gets its input pipelined by the refresh tree of the used MV.
class PipelinedMavBuilder : public MavBuilder
{
public:
  PipelinedMavBuilder(const CorrName&      mvName,
		      MVInfoForDML        *mvInfo,
		      Refresh	          *refreshNode,
		      NABoolean	           isProjectingMvDelta,
		      const QualifiedName *pipeliningSource,
		      BindWA              *bindWA);

private:
  // Copy Ctor and = operator are not implemented.
  PipelinedMavBuilder(const PipelinedMavBuilder& other);
#pragma nowarn(1026)   // warning elimination 
  PipelinedMavBuilder& operator=(const PipelinedMavBuilder& other);
#pragma warn(1026)  // warning elimination 

public:
  virtual ~PipelinedMavBuilder() {}

  // called by Refresh::bindnode() for building the next level refresh tree
  // and connecting it as a "view" that is projecting the data.
  RelExpr *buildAndConnectPipeliningRefresh(RelExpr *pipeliningTree);

protected:
  QualifiedName *getPipeliningSource()
  { return getDeltaDefList()->at(0)->getTableName(); }

  // Methods called by buildAndConnectPipeliningRefresh().
  // ---------------------------------------------------- 
  RelExpr *buildJoinWithTupleList(RelExpr *topNode);
  RelRoot *buildRenameToLog(RelExpr *topNode);

};  // class PipelinedMavBuilder

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class LogsInfo : public NABasicObject
{
public:

  LogsInfo(DeltaDefinition &deltaDef,
				  MVInfoForDML    *mvInfo,
				  BindWA	  *bindWA);

  virtual ~LogsInfo();

private:
  // Copy Ctor and = operator are not implemented.
  LogsInfo(const LogsInfo& other);
#pragma nowarn(1026)   // warning elimination 
  LogsInfo& operator=(const LogsInfo& other);
#pragma warn(1026)  // warning elimination 

public:

  DeltaDefinition &getDeltaDefinition() { return deltaDef_; }

  MvIudLog	  &getMvIudlog() { return *currentMvIudlog_; }
  
  IntegerList	  *getBaseTableDirectionVector() { return baseTableDirectionVector_; }

  BindWA	  *getBindWA() { return bindWA_; }

  const NATable   *getBaseNaTable() { return currentMvIudlog_->getSubjectNaTable(); }

  const NATable   *getIudLogNaTable() { return currentMvIudlog_->getNaTable(); }

  const NATable   *getRangeNaLogTable() 
  {
    CMPASSERT(rangeNaTable_);
    return rangeNaTable_; 
  }

  const CorrName  &getBaseTableName() { return currentMvIudlog_->getSubjectTableName(); }

  const CorrName  &getIudLogTableName() { return *currentMvIudlog_->getTableName(); }

  const CorrName  &getRangeLogTableName() 
  { 
    CMPASSERT(rangeTableName_);
    return *rangeTableName_; 
  }

private:
  // Given the base table name, construct the range log name from it.
  CorrName        *calcRangeLogName(const CorrName &theTable, CollHeap *heap) const;

  
  DeltaDefinition &deltaDef_;
  MvIudLog	  *currentMvIudlog_;
  IntegerList	  *baseTableDirectionVector_;
  MVInfoForDML    *mvInfo_;
  BindWA	  *bindWA_;

  NATable	  *rangeNaTable_;
  CorrName	  *rangeTableName_;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class MultiDeltaRefreshMatrixRow : public NABasicObject
{
private:
  enum rowSign  { SIGN_MINUS = 0, SIGN_PLUS=1 };
  enum scanType { SCAN_TABLE = 0, SCAN_DELTA=1 };

public:
  // Ctor for creating an all-but-last SCAN_TABLE row.
  MultiDeltaRefreshMatrixRow(Int32 length, Int32 maxLength, CollHeap *heap);

  // Copy Ctor.
  MultiDeltaRefreshMatrixRow(const MultiDeltaRefreshMatrixRow& other);

  // Dtor
  virtual ~MultiDeltaRefreshMatrixRow() {}

  // Mutators
  void initArray();
  void flipSignAndLastTable();
  void addColumn(scanType type = SCAN_TABLE);

  // Accessors
  NABoolean  isSignPlus() const 
    { return sign_==SIGN_PLUS; }
  
  NABoolean  isScanOnDelta(CollIndex i) const 
    { return tableScanTypes_[i]==SCAN_DELTA; }

#ifndef NDEBUG
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
	     const char* title = "MultiDeltaRefreshMatrixRow") const;
#endif

private:
  rowSign	  sign_;           // The sign of this matrix row.
  Int32		  currentLength_;  // How many tables so far.
  Int32		  maxLength_;      // The array size.
  ARRAY(scanType) tableScanTypes_; // The matrix row itself.
  CollHeap       *heap_;           // Used by the copy Ctor.
};  // class MultiDeltaRefreshMatrixRow

//----------------------------------------------------------------------------
class MultiDeltaRefreshMatrix : public NABasicObject
{
public:
  MultiDeltaRefreshMatrix(Int32	        maxNumOfRows,
                          MVJoinGraph  *joinGraph,
			  CollHeap     *heap);

  virtual ~MultiDeltaRefreshMatrix();

  // Mutators
  void addTable(NABoolean readDelta);
  void setThisPhase(Int32 phase);
  void calculatePhases();

  // Accessors
  const MultiDeltaRefreshMatrixRow *getRow(Int32 i) const;

  Int32       getNumOfRows() const { return numOfRows_; }
  Int32       getRowLength() const { return currentRowLength_; }
  NABoolean isLastPhase()  const { return isLastPhase_; }
  Lng32      getTableIndexFor(Int32 index) const { return tableIndexMapping_[index];}
  Int32       getFirstRowForThisPhase()   const { return firstRowForThisPhase_; }
  Int32       getNumOfRowsForThisPhase()  const { return numOfRowsForThisPhase_; }
  void      setPhasesSupported(NABoolean supported) { isPhasesSupported_ = supported; }
  NABoolean isPhasesSupported()     const { return isPhasesSupported_; }
  NABoolean isTooManyDeltasError()  const { return TooManyDeltasError_; }
  NABoolean isDuplicatesOptimized() const { return isDuplicatesOptimized_; }
  void      disableDuplicateOptimization() { isDuplicatesOptimized_ = FALSE; }

#ifndef NDEBUG
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
	     const char* title = "MultiDeltaRefreshMatrix") const;
#endif

private:
  Int32	    numOfRows_;              // How many rows so far.
  Int32	    currentRowLength_;       // How many tables so far.
  Int32	    maxRowLength_;           // No. of tables in join product
  CollHeap *heap_;                   // The binder (statement) heap.
  // The actual matrix is an array of matrix rows.
  ARRAY(MultiDeltaRefreshMatrixRow *) theMatrix_;
  // The mapping from the tableIndex to the array of used objects in MVInfo.
  // This mapping is created by the join graph algorithm.
  ARRAY(Lng32) tableIndexMapping_;

  // data members for division to phases.
  NABoolean isPhasesSupported_;      // Do we need to support divition to phases?
  Int32	    thisPhase_;              // PHASE parameter to refresh.
  NABoolean isLastPhase_;            // Are more phases needed?
  Int32	    firstRowForThisPhase_;   // Where does this phase start?
  Int32	    numOfRowsForThisPhase_;  // Ho many rows in this phase?
  NABoolean TooManyDeltasError_;     // Abort rather than risk an optimizer crash.
  NABoolean isDuplicatesOptimized_;  // Do not build full matrix, since duplicates
                                     // are eliminated elsewhere.

  // Heuristic parameters taken from the Defaults table:
  // Maximum number of tables in a join for doing everything in one phase.
  Int32       maxJoinSizeForSinglePhase_;  // Default is 3
  // Minimum join size for doing a single matrix row per phase.
  Int32       minJoinSizeForSingleProductPerPhase_; // Default is 8
  // When the join size is between the two parameters above - how many
  // matrix rows to do per phase.
  Int32       phaseSizeForMidRange_;       // Default is 6
  // If there are too many deltas, the optimizer may explode.
  Int32       maxDeltasThreshold_;         // Default is 31 rows (Six deltas)

};  // class MultiDeltaRefreshMatrix


#endif
