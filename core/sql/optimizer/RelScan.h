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
#ifndef RELSCAN_H
#define RELSCAN_H
/* -*-C++-*-
******************************************************************************
*
* File:         RelScan.h
* Description:  Relational scan (both physical and logical operators)
* Created:      4/28/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

#include "ComSmallDefs.h"
#include "ObjectNames.h"
#include "RelExpr.h"
#include "SearchKey.h"
#include "mdam.h"
#include "mdamkey.h"
#include "disjuncts.h"
#include "OptUtilIncludes.h"
#include "CmpContext.h"
#include "SchemaDB.h"
#include "HbaseSearchSpec.h"
#include "OptHints.h"
#include "ExpHbaseDefs.h"
#include <vector>

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class Scan;
class FileScan;

// a helper class used by class Scan
class ScanIndexInfo;
class IndexProperty;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class BindWA;
class CostScalar;
class Generator;
class GenericUpdate;
class LogicalProperty;
class NormWA;
class SimpleCostVector;
class TableDesc;
class MVInfoForDML;
class DeltaDefinition;
class MvRefreshBuilder;
////////////////////
class QueryAnalysis;
class CANodeId;
class JBBSubsetAnalysis;
////////////////////

class QRDescGenerator;
class RangeSpecRef;

class MVMatch;
class CommonSubExprRef;


/*************************
MdamFlags used to indicate promise of key access of an index
*************************/
#ifndef MDAM_FLAGS
#define MDAM_FLAGS
enum MdamFlags	{ UNDECIDED,
		   MDAM_ON,
		   MDAM_OFF
		};
#endif

/******************************
IndexJoinSelectivityEnum indicates if the cost of index join would
exceed the cost of scanning the base table or not.
*******************************/
#ifndef INDEX_JOIN_SLECTIVITY
#define INDEX_JOIN_SLECTIVITY
enum IndexJoinSelectivityEnum { 
				INDEX_ONLY_INDEX,
				INDEX_JOIN_VIABLE, 
				EXCEEDS_BT_SCAN
			      }; 	 
#endif




// -----------------------------------------------------------------------
// A helper class for class Scan that identifies sets of indexes to be
// used in transformations of this scan to an index join (a join between
// an index and another scan node). Only the friends should use this
// class.
// -----------------------------------------------------------------------
class ScanIndexInfo : public NABasicObject
{
friend class Scan;
friend class IndexJoinRule1;
friend class IndexJoinRule2;
friend class OrOptimizationRule;
public:
  ScanIndexInfo(
              const ValueIdSet& inputsToIndex,
              const ValueIdSet& outputsFromIndex,
              const ValueIdSet& indexPredicates,
              const ValueIdSet& joinPredicates,
              const ValueIdSet& outputsFromRightScan,
              const ValueIdSet& indexColumns,
              IndexProperty* ixProp
             );

  ScanIndexInfo(const ScanIndexInfo & other);

private:

  ValueIdSet       inputsToIndex_;
  ValueIdSet       outputsFromIndex_;
  ValueIdSet       indexPredicates_;
  ValueIdSet       joinPredicates_;
  ValueIdSet       outputsFromRightScan_;
  ValueIdSet	   indexColumns_;
  SET(IndexProperty *) usableIndexes_;
  NABoolean        transformationDone_;
};


/****************************************************************
Helper class used to encapsulate properties of an index like promise
of key access, viability of index join. Default inputLogProp used to
calculate cost of index join.
****************************************************************/
class IndexProperty : public NABasicObject
{

public:

  IndexProperty(IndexDesc * index,
		MdamFlags mdamFlag = MDAM_ON,
		IndexJoinSelectivityEnum selectivity = INDEX_ONLY_INDEX,
		const EstLogPropSharedPtr& inELP = NULL):
		index_(index),mdamFlag_(mdamFlag),
		selectivityEnum_(selectivity),
		inputEstLogProp_(inELP)
  {}

  //mutator functions
  void setSelectivity(IndexJoinSelectivityEnum selectivity)
  { selectivityEnum_ = selectivity; }
  void setMdamFlag(MdamFlags mdamFlag)
  { mdamFlag_ = mdamFlag ; }
  void setInputEstLogProp(const EstLogPropSharedPtr& inELP)
  { inputEstLogProp_ = inELP; }
  //Accessor functions
  IndexDesc * getIndexDesc() const
    { return index_; }
  MdamFlags getMdamFlag() const
    { return mdamFlag_; }
  IndexJoinSelectivityEnum getSelectivity() const
    { return selectivityEnum_; }
  EstLogPropSharedPtr getInputEstLogProp() const
  { return inputEstLogProp_; }
  void updatePossibleIndexes(SET(IndexProperty *) & ixOnlyIndexes, Scan * );
  COMPARE_RESULT compareIndexPromise(const IndexProperty *) const;

private:
  IndexDesc * index_;
  MdamFlags mdamFlag_;
  IndexJoinSelectivityEnum selectivityEnum_;
  EstLogPropSharedPtr inputEstLogProp_;
};
// -----------------------------------------------------------------------
// The Scan operator forms the leaf nodes of a tree of relational
// expressions.
// -----------------------------------------------------------------------

class Scan : public RelExpr
{
public:

  // constructor
  Scan(OperatorTypeEnum otype = REL_SCAN,
       CollHeap *oHeap = CmpCommon::statementHeap(),
           NABoolean stream = FALSE ) :
         RelExpr(otype, NULL, NULL, oHeap),
         userTableName_("", oHeap),
         tabId_(NULL),
         numIndexJoins_(0),
         indexOnlyIndexes_(oHeap),
	 indexOnlyScans_(oHeap),
         indexJoinScans_(oHeap),
         possibleIndexJoins_(oHeap),
         isSingleVPScan_(FALSE),
         stoi_(NULL),
         samplePercent_(-1.0),
         clusterSize_(0),
         scanFlags_(0),
          // QSTUFF
         stream_ (stream),
         embeddedUpdateOrDelete_ (FALSE),
	 forcedIndexInfo_(FALSE),
         suppressHints_(FALSE),
         baseCardinality_(0),
          // QSTUFF
         isRewrittenMV_(FALSE),
         matchingMVs_(oHeap),
         hbaseAccessOptions_(NULL),
         commonSubExpr_(NULL)
     {} 

  Scan(const CorrName& name,
           NABoolean stream = FALSE) :
         RelExpr(REL_SCAN, NULL, NULL, CmpCommon::statementHeap()),
         userTableName_(name, CmpCommon::statementHeap()),
         tabId_(NULL),
         numIndexJoins_(0),
         indexOnlyIndexes_(CmpCommon::statementHeap()),
	 indexOnlyScans_(CmpCommon::statementHeap()),
         indexJoinScans_(CmpCommon::statementHeap()),
         possibleIndexJoins_(CmpCommon::statementHeap()),
         isSingleVPScan_(FALSE),
         stoi_(NULL),
         samplePercent_(-1.0),
         clusterSize_(0),
         scanFlags_(0),
                 // QSTUFF
         stream_ (stream),
         embeddedUpdateOrDelete_ (FALSE),
	 forcedIndexInfo_(FALSE),
         suppressHints_(FALSE),
         baseCardinality_(0),
          // QSTUFF
         isRewrittenMV_(FALSE),
         matchingMVs_(CmpCommon::statementHeap()),
         hbaseAccessOptions_(NULL),
         commonSubExpr_(NULL)
     {} 

  Scan(const CorrName& name,
          TableDesc * tabId,
           OperatorTypeEnum otype = REL_SCAN,
           CollHeap *oHeap = CmpCommon::statementHeap(),
           NABoolean stream = FALSE) :
         RelExpr(otype, NULL, NULL, oHeap),
         userTableName_(name, oHeap),
         tabId_(tabId),
         numIndexJoins_(0),
         indexOnlyIndexes_(oHeap),
	 indexOnlyScans_(oHeap),
         indexJoinScans_(oHeap),
         possibleIndexJoins_(oHeap),
         isSingleVPScan_(FALSE),
         stoi_(NULL),
         samplePercent_(-1.0),
         clusterSize_(0),
         scanFlags_(0),
                 // QSTUFF
         stream_ (stream),
         embeddedUpdateOrDelete_ (FALSE),
	 forcedIndexInfo_(FALSE),
         suppressHints_(FALSE),
         baseCardinality_(0),
          // QSTUFF
         isRewrittenMV_(FALSE),
         matchingMVs_(oHeap),
         hbaseAccessOptions_(NULL),
         commonSubExpr_(NULL)
     {} 

  Scan(OperatorTypeEnum otype,
       const CorrName& name,
       CollHeap *oHeap = CmpCommon::statementHeap()) :
         RelExpr(otype, NULL, NULL, oHeap),
         userTableName_(name, CmpCommon::statementHeap()),
         tabId_(NULL),
         numIndexJoins_(0),
         indexOnlyIndexes_(CmpCommon::statementHeap()),
	 indexOnlyScans_(CmpCommon::statementHeap()),
         indexJoinScans_(CmpCommon::statementHeap()),
         possibleIndexJoins_(CmpCommon::statementHeap()),
         isSingleVPScan_(FALSE),
         stoi_(NULL),
         samplePercent_(-1.0),
         clusterSize_(0),
         scanFlags_(0),
                 // QSTUFF
	 stream_ (FALSE),
         embeddedUpdateOrDelete_ (FALSE),
	 forcedIndexInfo_(FALSE),
         suppressHints_(FALSE),
         baseCardinality_(0),
         // QSTUFF
         isRewrittenMV_(FALSE),
         hbaseAccessOptions_(NULL),
         matchingMVs_(CmpCommon::statementHeap()),
         commonSubExpr_(NULL)
     {} 

  // virtual destructor
  virtual ~Scan() {}

  // append an ascii-version of RelExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // is this entire expression cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // change literals of a cacheable query into ConstantParameters
  virtual RelExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;

  virtual NABoolean isHbaseScan() { return FALSE; }

  // get and set the table name
  virtual const CorrName & getTableName() const         { return userTableName_; }
  virtual CorrName & getTableName()               { return userTableName_; }
  virtual const CorrName * getPtrToTableName() const    { return &userTableName_; }

  virtual void setTableName(CorrName &userTableName)
  {
    userTableName_ = userTableName;
  }

  // Methods for accessing and updating the TableDesc
  TableDesc * getTableDesc() const              { return tabId_; }
  void setTableDesc(TableDesc * tdesc)          { tabId_ = tdesc; }

  TableDesc * getTableDescForBaseSelectivity() { return tabId_; }

    // This method computes the base selectivity for this table. It is defined
  // cardinality after applying all local predicates with empty input logical
  // properties over the base cardinality without hints
  CostScalar computeBaseSelectivity() const;

  const SET(IndexProperty *) & getIndexOnlyIndexes() { return indexOnlyIndexes_; }
  const LIST(ScanIndexInfo *) & getPossibleIndexJoins()
                                                { return possibleIndexJoins_; }
  Lng32 getNumIndexJoins()                       { return numIndexJoins_; }
  void setNumIndexJoins(Lng32 n)                 { numIndexJoins_ = n; }

  void setHbaseAccessOptions(HbaseAccessOptions *v) { hbaseAccessOptions_ = v; }
  HbaseAccessOptions *getHbaseAccessOptions() const { return hbaseAccessOptions_; }

  // the maximal number of index joins that a scan node will be
  // transformed into
  static const Lng32 MAX_NUM_INDEX_JOINS;

  NABoolean isHiveTable() const;
  NABoolean isHbaseTable() const;
  NABoolean isSeabaseTable() const;

  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // MV --
  NABoolean virtual isIncrementalMV() { return TRUE; }
  void virtual collectMVInformation(MVInfoForDDL *mvInfo,
				    NABoolean isNormalized);
  RelExpr *bindExpandedMaterializedView(BindWA *bindWA, NATable *naTable);
  void projectCurrentEpoch(BindWA *bindWA);

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  virtual void transformNode(NormWA & normWARef,
                             ExprGroupId & locationOfPointerToMe);

  // Each operator supports a (virtual) method for rewriting its
  // value expressions.
  virtual void rewriteNode(NormWA & normWARef);

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  virtual void getPotentialOutputValuesAsVEGs(ValueIdSet & outputs) const ; 

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);

  // synthesize complementary Referential opt constraints
  // used to eliminate redundant joins and match extra-hub tables.
  void synthCompRefOptConstraints(NormWA * normWAPtr) ;

  // finish synthesis of logical properties by setting the cardinality
  // attributes
  virtual void finishSynthEstLogProp();

  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);


  // Generate the read access set.
  virtual SubTreeAccessSet * calcAccessSets(CollHeap *heap);

  // reconcile GroupAttribute
  virtual NABoolean reconcileGroupAttr(GroupAttributes *newGroupAttr);

  // find out which indexes on the base table are usable for this scan
  void addIndexInfo();
  NABoolean updateableIndex(IndexDesc*, ValueIdSet& preds, 
                            NABoolean & needsHalloweenProtection);
  NABoolean requiresHalloweenForUpdateUsingIndexScan();

  // iterate over the usable indexes (index-only and index joins)
  CollIndex numUsableIndexes();

  IndexDesc *getUsableIndex(CollIndex indexNum,
			    IndexProperty **indexOnlyInfo = NULL,
			    ScanIndexInfo **indexJoinInfo = NULL);

  // add all base cols that contribute to local VEGPredicates to vs
  void addBaseColsFromVEGPreds(ValueIdSet &vs) const;

  // set the list of indexonly indexes externally

  void setIndexOnlyScans(const SET(IndexProperty *) &ixonly)
                                           { indexOnlyIndexes_ = ixonly; }
  const SET(IndexDesc *)& deriveIndexOnlyIndexDesc();
  const SET(IndexDesc *)& deriveIndexJoinIndexDesc();
  const SET(IndexDesc *)& getIndexJoinIndexDesc(){ return indexJoinScans_; }

  const LIST(ScanIndexInfo *) &getIndexInfo()
                                           { return possibleIndexJoins_; }
  const ValueIdSet &getComputedPredicates() const { return generatedCCPreds_; }

  // set (restrict) the potential output values that this node can produce
  void setPotentialOutputValues(const ValueIdSet &po)
                                               { potentialOutputs_ = po; }

  Cardinality getBaseCardinality() const
                                              { return baseCardinality_; }

  void setBaseCardinality(Cardinality newCard)
                                           { baseCardinality_ = newCard; }

  void setTableAttributes(CANodeId nodeId);

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);
  void copyIndexInfo(RelExpr *derivedNode);
  void removeIndexInfo();
  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const;

  virtual void computeMyRequiredResources(RequiredResources & reqResources,
                                      EstLogPropSharedPtr & inLP);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  const StmtLevelAccessOptions  accessOptions() const { return accessOptions_; }
        StmtLevelAccessOptions &accessOptions()       { return accessOptions_; }

  const SelectivityHint * getSelectivityHint() const
              { return (suppressHints_ ? NULL : tabId_->getSelectivityHint()); }
  const CardinalityHint * getCardinalityHint() const 
              { return (suppressHints_ ? NULL : tabId_->getCardinalityHint()); }

  // ---------------------------------------------------------------------
  // Scan::normalizeNode()
  // Invokes ItemExpr::normalizeNode() & then performs mdamBuildDisjuncts
  // ---------------------------------------------------------------------
  RelExpr * normalizeNode ( NormWA & normWARef );
 
  virtual NABoolean prepareMeForCSESharing(
       const ValueIdSet &outputsToAdd,
       const ValueIdSet &predicatesToRemove,
       const ValueIdSet &commonPredicatesToAdd,
       const ValueIdSet &inputsToRemove,
       ValueIdSet &valuesForVEGRewrite,
       ValueIdSet &keyColumns,
       CSEInfo *info);

 // synthesizes compRefOpt constraints.
  virtual void processCompRefOptConstraints(NormWA * normWAPtr) ;

  // checks if the scan on the referenced table contains predicates of the form
  //table.col = 1, or table.col > 1, or (fktable.col = 1 and fktable.col = table.col)
  // in these examples the col in table.col has to be a non key column. If its a key
  //column then the predicate is not reducing in the foreignkey-uniquekey sense because
  // if there table.uniquekey = fktable.foriegnkey type predicate then that predicate
  // will ensure that table.uniquekey = 1 type predicate is also applied to the 
  // foreign key side and therefore it does not destroy the FK-UK relation.
  NABoolean containsReducingPredicates(const ValueIdSet& keyCols) const;

  void setForceIndexInfo() { forcedIndexInfo_ = TRUE; }
  void resetForceIndexInfo() { forcedIndexInfo_ = FALSE; }
  NABoolean isIndexInfoForced() const { return forcedIndexInfo_; }

  void setSuppressHints(NABoolean x = TRUE)            { suppressHints_ = x; }
  NABoolean areHintsSuppressed() const              { return suppressHints_; }

  // ---------------------------------------------------------------------
  // Vertical Partitioning related methods
  // ---------------------------------------------------------------------
  // checks if TableDesc has vertical partitions
  NABoolean isVerticalPartitionTableScan() const
                      { return (!tabId_->getVerticalPartitions().isEmpty()); }

  // Returns isSingleVPScan
  NABoolean isSingleVerticalPartitionScan() const  { return isSingleVPScan_; }

  // Sets isSingleVPScan
  void setSingleVerticalPartitionScan(NABoolean isSingleVPScan)
                                         { isSingleVPScan_ = isSingleVPScan; }

  // find out which vertical partitions are needed for this scan
  // and what columns does each of them provide
  void getRequiredVerticalPartitions
       (SET(IndexDesc  *) & requiredVPs,
        SET(ValueIdSet *) & columnsProvidedByVP) const;


  // if update where current of, updateQry is set to TRUE.
  // if del where cur of, set to FALSE.
  void bindUpdateCurrentOf(BindWA *bindWA, NABoolean updateQry);

  ValueIdList & pkeyHvarList()                    { return pkeyHvarList_; }

  OptSqlTableOpenInfo *getOptStoi() const               { return stoi_; };
  void setOptStoi(OptSqlTableOpenInfo *stoi)            { stoi_ = stoi; };

  Float32 samplePercent() const               { return samplePercent_; };
  void samplePercent(Float32 sp)              { samplePercent_ = sp; };

  NABoolean checkForCardRange(const ValueIdSet & setOfPredicates,
                        CostScalar & newRowCount /* in and out */);

  NABoolean isSampleScan() const            { return samplePercent_ >= 0; };
  inline ValueIdSet & sampledColumns() { return sampledColumns_; };
  inline const ValueIdSet & sampledColumns() const { return sampledColumns_; };

  Lng32 clusterSize() const                  { return clusterSize_; };
  void clusterSize(Lng32 cs)                 { clusterSize_ = cs; };
  NABoolean isClusterSampling() const       { return clusterSize_ > 0; };

  NABoolean noSecurityCheck() { return (scanFlags_ & NO_SECURITY_CHECK) != 0; }
  void setNoSecurityCheck(NABoolean v)
  { (v ? scanFlags_ |= NO_SECURITY_CHECK : scanFlags_ &= ~NO_SECURITY_CHECK); }

  // QSTUFF VV

  // --------------------------------------------------------------------
  // This routine checks whether a table is both read and updated
  // --------------------------------------------------------------------
  virtual rwErrorStatus checkReadWriteConflicts(NormWA & normWARef);

  // QSTUFF ^^

  // 10-040128-2749 -begin
  // check if Mdam is enabled based on the context,Current Controltable
  // setting and defaults.
  NABoolean isMdamEnabled(const Context *context);
  // 10-040128-2749 -end

  // MV --
  // Force the Scan to be in the reverse order of the clustering index.
  NABoolean forceInverseOrder() const { return (scanFlags_ & FORCE_INVERSE_ORDER) != 0; }
  void setForceInverseOrder(NABoolean v=TRUE)
  { (v ? scanFlags_ |= FORCE_INVERSE_ORDER : scanFlags_ &= ~FORCE_INVERSE_ORDER); }

  void setExtraOutputColumns(ValueIdSet outputCols)
    { extraOutputColumns_ = outputCols; }
  const ValueIdSet& getExtraOutputColumns() const
    { return extraOutputColumns_;}

  void setRewrittenMV() { isRewrittenMV_ = TRUE; }
  NABoolean isRewrittenMV() const { return isRewrittenMV_;}

  //////////////////////////////////////////////////////
  virtual NABoolean pilotAnalysis(QueryAnalysis* qa);
  JBBSubsetAnalysis* getJBBSubsetAnalysis();
  //////////////////////////////////////////////////////

  ItemExpr * applyAssociativityAndCommutativity(QRDescGenerator *descGenerator,
             CollHeap *heap, ItemExpr *ptrToOldTree, NormWA& normWARef,
             NABoolean& transformationStatus);
  
  NABoolean shrinkNewTree(OperatorTypeEnum op, ItemExpr *ptrToNewTree, 
                          RangeSpecRef* xref, NormWA& normWARef);

  // for MVQR: return the list of matching MVs
  NAList<MVMatch*> getMatchingMVs()
      { return matchingMVs_; };

  // Insert an MV match (for query rewrite purposes) into the list of potential
  // replacements for the Scan. Preferred matches are inserted at the
  // front of the list, non-preferred matches go at the end.
  void addMVMatch(MVMatch* match, NABoolean isPreferred)
  {
    if (isPreferred)
      matchingMVs_.insertAt(0, match);
    else
      matchingMVs_.insert(match);
  }

  CommonSubExprRef *getCommonSubExpr() const        { return commonSubExpr_; }
  void setCommonSubExpr(CommonSubExprRef *cse)       { commonSubExpr_ = cse; }

protected:

  // Find the most promising index from the LIST, for index joins
  IndexProperty* findSmallestIndex(const LIST(ScanIndexInfo *)&) const;

  // Find the most promising index from the set, for index only scans
  IndexProperty* findSmallestIndex(const SET(IndexProperty *)& indexes) const;


  // A help method to compute the cpu resource 
  CostScalar
  computeCpuResourceRequired(const CostScalar& rowsToScan, const CostScalar& rowSize);

  // Compute the cpu resource for index join scan and index only scan.
  CostScalar computeCpuResourceForIndexJoinScans(CANodeId tableId);
  CostScalar computeCpuResourceForIndexOnlyScans(CANodeId tableId);

  // Compute the amount of work for one index join scan.

  // indexPredicates specifies the predicates on the index. The amout of the 
  // work is only computed for the key columns contained in the predicates.
  CostScalar computeCpuResourceForIndexJoin(CANodeId tableId, IndexDesc* iDesc,
                                            ValueIdSet& indexPredicates,
                                            CostScalar& rowsToScan);

  // The work is only computed for the key columns contained in ikeys.
  CostScalar computeCpuResourceForIndexJoin(CANodeId tableId, IndexDesc* iDesc,
                                            const ValueIdList& ikeys,
                                            CostScalar& rowsToScan);

  inline void setComputedPredicates(const ValueIdSet &ccPreds)
                                             { generatedCCPreds_ = ccPreds; }

private:

  // the first param to this method, vid, is assumed to a OR predicate
  // that meets all logical conditions such that it can be transformed
  // to a IN subquery to be implemeneted using a semijoin
  // this method applies various heuristics and returns a TRUE/FALSE
  // answer as to whether applying the transformation will yield
  // better plans. Method is called in Normalizer so we use 
  // heuristics rather than computed cost.
  NABoolean passSemiJoinHeuristicCheck(ValueId vid, Lng32 numValues, 
                                      Lng32 numParams, ValueId colVid) const ;

  // NO_SECURITY_CHECK: if this Scan node was created as part of another
  // operation (ex., UPDATE becomes UPDATE(SCAN) in parser),
  // then no security check is needed.
  enum ScanFlags { NO_SECURITY_CHECK    = 0x0001,
		   FORCE_INVERSE_ORDER  = 0x0002};

  Cardinality      baseCardinality_;   // from table statistics

  // the user-specified name of the table
  CorrName userTableName_;

  // a unique identifier for the table
  TableDesc * tabId_;

  // The following two fields are set once by calling addIndexInfo().
  // all indexes that can deliver all the required values
  SET(IndexProperty *) indexOnlyIndexes_;
  SET(IndexDesc *) indexOnlyScans_;
  SET(IndexDesc *) indexJoinScans_;
  // all indexes that can't deliver all the required values plus
  // some info about each of those indexes, calculated by addIndexInfo
  LIST(ScanIndexInfo *) possibleIndexJoins_;
  ValueIdSet generatedCCPreds_;

  // the values that this logical scan node can deliver after this
  // set has been initialized to something other than an empty set
  ValueIdSet potentialOutputs_;

  // indicate how many index joins on this table we've already done before
  // creating this node
  Lng32 numIndexJoins_;

  // Contains user specified BROWSE, STABLE or REPEATABLE access type and
  // user specified SHARE or EXCLUSIVE lock mode.
  StmtLevelAccessOptions accessOptions_;

  // See bindUpdateCurrentOf() for details on how this list is created.
  ValueIdList pkeyHvarList_;

  // Indicates whether this Scan node is a Scan of a single vertical
  // partition.  It applies to only Scan nodes that have been generated by
  // the VPJoinRule from a Scan of a vertically partitioned table.  It is
  // set to TRUE in VPJoinRule::nextSubstitute.
  NABoolean isSingleVPScan_;

  // tables open information which is necessary during the open of a sql table.
  OptSqlTableOpenInfo *stoi_;

  // -- Triggers
  RelExpr *buildTriggerTransitionTableView(BindWA *bindWA);

  // The following data members are added to enable sampling in the
  // scan operator. Currently Scan performs sampling also only if:
  //      1. There are no selection predicates
  //      2. The scan is not on multiple VP tables
  //      3. Reverse ordering is not specified
  // The Sample enabled scan performs only random sampling with relative
  // size specification (as a percentage of the table size).

  // The sampled columns - for mapping the value ids between RelSample and
  // Scan
  //
  ValueIdSet sampledColumns_;

  // Sample size in percentage (of the table size). Set to > 0.0 if sampling
  // and -1.0 otherwise.
  //
  Float32 samplePercent_;

  // Cluster size for sampling (in number of blocks). Set to > 0 if cluster
  // sampling, 0 otherwise.
  //
  Lng32 clusterSize_;

  // MV --
  // These are columns prepared by the TCB during execution and projected
  // for logging purposes.
  ValueIdSet extraOutputColumns_;
  NABoolean isRewrittenMV_;       // default is FALSE

  // see enum ScanFlags
  ULng32 scanFlags_;

  // QSTUFF
  NABoolean embeddedUpdateOrDelete_;
  // QSTUFF

  //?johannes?
  // scan produces a continuous stream without returning an end-of-data
  NABoolean stream_;

  // is this scan created by a rule that wants to force a specific index?
  NABoolean forcedIndexInfo_;

  // do not apply hints (e.g. used for inner table of index join)
  NABoolean suppressHints_;

  // hbase options. Like: number of trafodion row versions to retrieve from hbase.
  HbaseAccessOptions *hbaseAccessOptions_;

  // List of MV matches that can be substituted for the SCAN using query rewrite.
  NAList<MVMatch*> matchingMVs_;

  // pointer to the common subexpression, if this is a scan of a
  // materialized common subexpr
  CommonSubExprRef *commonSubExpr_;
};

// -----------------------------------------------------------------------
// FileScan : Physical Operator
//
// The filescan operator differs from its logical Scan operator in
// that selection predicates have been further divided into.
// Also, an Index has been attached to it and it is guaranteed to
// be an index only scan (the predicates_ are covered by the
// attached index columns)
// -----------------------------------------------------------------------

// Forward so that FileScan knows about it:
class FileScanOptimizer; // declared in ScanOptimizer.h
class FileScan : public Scan
{
public:

  FileScan(const CorrName& tableName,
           TableDesc * tableDescPtr,
           const IndexDesc *indexDescPtr,
           const NABoolean isReverseScan,
           const Cardinality& baseCardinality,
           StmtLevelAccessOptions& accessOptions,
           GroupAttributes * groupAttributesPtr,
           const ValueIdSet& selectionPredicates,
           const Disjuncts& disjuncts,
           const ValueIdSet &generatedCCPreds,
           OperatorTypeEnum otype = REL_FILE_SCAN
           );

  FileScan(const CorrName& name,
           TableDesc * tabId,
           const IndexDesc *indexDesc,
           OperatorTypeEnum otype = REL_FILE_SCAN,
           CollHeap *oHeap = CmpCommon::statementHeap())
  : Scan (name, tabId, otype, oHeap), indexDesc_(indexDesc),
    executorPredTree_(NULL),  reverseScan_(FALSE),
    mdamKeyPtr_(NULL),
    disjunctsPtr_(NULL),
    pathKeys_(NULL),
    partKeys_(NULL),
    hiveSearchKey_(NULL),
    numberOfBlocksToReadPerAccess_ (-1),
    estRowsAccessed_ (0),
    mdamFlag_(UNDECIDED),
    skipRowsToPreventHalloween_(FALSE),
    probes_(0),
    successfulProbes_(0),
    uniqueProbes_(0),
    duplicateSuccProbes_(0),
    failedProbes_(0),
    tuplesProcessed_(0),
    computedNumOfActivePartitions_(-1)
  {}

  // destructor
  virtual ~FileScan() {}

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  virtual NABoolean patternMatch(const RelExpr & other) const;
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  // the FileScan node is purely physical
  virtual NABoolean isLogical() const;
  virtual NABoolean isPhysical() const;

  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  PhysicalProperty * synthHiveScanPhysicalProperty(const Context *context,
                                                   const Lng32    planNumber,
                                                   ValueIdList   &sortOrderVEG);

 PhysicalProperty * synthHbaseScanPhysicalProperty(const Context *context,
                                                   const Lng32    planNumber,
                                                   ValueIdList   &sortOrderVEG);

  // Obtain a pointer to a CostMethod object that provides access
  // to the cost estimation functions for nodes of this type.
  virtual CostMethod* costMethod() const;

  // method to do code generation
  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet & externalInputs,
                               ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  short codeGenForHive(Generator*);
  short genForTextAndSeq(Generator * generator,
                             Queue * &hdfsFileInfoList,
                             Queue * &hdfsFileRangeBeginList,
                             Queue * &hdfsFileRangeNumList,
                             char* &hdfsHostName,
                             Int32 &hdfsPort,
                             NABoolean &doMultiCursor,
                             NABoolean &doSplitFileOpt);
  static short genForOrc(Generator * generator,
                         const HHDFSTableStats* hTabStats,
                         const PartitioningFunction * mypart,
                         Queue * &hdfsFileInfoList,
                         Queue * &hdfsFileRangeBeginList,
                         Queue * &hdfsFileRangeNumList,
                         char* &hdfsHostName,
                         Int32 &hdfsPort);
  
 static TrafDesc *createHbaseTableDesc(const char * table_name);

  // Return a (short-lived) reference to the respective predicates
  const ValueIdList & getBeginKeyPred() const    { return beginKeyPred_; }
  const ValueIdList & getEndKeyPred() const        { return endKeyPred_; }

  // executor predicates accessor functions:
  const ValueIdSet&  getExecutorPredicates() const
  { return executorPredicates_; }
  void setExecutorPredicates(const ValueIdSet& executorPredicates)
  { executorPredicates_ = executorPredicates;
    executorPredicates_ -= getComputedPredicates(); }
  // Naked access to executorPredicates_ only because there is code
  // in precodegen that needs this (old code) (don't use this, use
  // get/set functions above):
  ValueIdSet& executorPred()                    { return executorPredicates_; }
  ItemExpr * getExecutorPredTree()              { return executorPredTree_; }
  ValueIdSet & retrievedCols()                  { return retrievedCols_; }

  NABoolean getReverseScan() const              { return reverseScan_; }
//  void setReverseScan(NABoolean rs)           { reverseScan_ = rs; }

//  IndexDesc * indexDesc()                     { return indexDesc_; }
  const IndexDesc * getIndexDesc() const        { return indexDesc_; }


  void setIndexDesc (const IndexDesc* idx) {indexDesc_ = idx;}

  // for code generator who shouldn't access phys. props:
  const PartitioningFunction *getPartFunc() const;

  //used to retrieve the operator type
  virtual const NAString getText() const;
  //used to retrieve the description
  virtual const NAString getTypeText() const;

  inline bool isFullScanPresent()const
  {
    if (pathKeys_ && pathKeys_->isFullScanPresent())
      return true;
    return false;
  }
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const;


  void setSearchKey(SearchKey *searchKeyPtr)
  { pathKeys_ = searchKeyPtr; }
  const SearchKey * getSearchKeyPtr() const
  { return pathKeys_; }
  const SearchKey * getSearchKey() const
  { return getSearchKeyPtr(); }

  // Naked access to pathKeys_ only because there is code
  // in precodegen that needs this (old code):
  SearchKey* searchKey()
  { return pathKeys_; }

  HivePartitionAndBucketKey * getHiveSearchKey()            { return hiveSearchKey_; }
  void setHiveSearchKey(HivePartitionAndBucketKey *s)          { hiveSearchKey_ = s; }

  // adds Explain information to this node. Implemented in
  // Generator/GenExplain.C
  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator);


  MdamKey* mdamKeyPtr()  { return mdamKeyPtr_; }
  const MdamKey* getMdamKeyPtr() const { return mdamKeyPtr_; }
  void setMdamKey(MdamKey *mdamKeyPtr)
  { mdamKeyPtr_ = mdamKeyPtr; }

  MdamFlags getMdamFlag()const { return mdamFlag_; }
  void setMdamFlag(MdamFlags mdamFlag) { mdamFlag_ = mdamFlag; }
  const Disjuncts& getDisjuncts() const;

  virtual NABoolean okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace* pws, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) ;

  virtual void addPartKeyPredsToSelectionPreds(
                 const ValueIdSet& partKeyPreds,
                 const ValueIdSet& pivs);


  virtual Lng32 getNumberOfBlocksToReadPerAccess() const
  {
    // make sure that value has been initialized before using it:
    CMPASSERT(numberOfBlocksToReadPerAccess_ > -1);
    return numberOfBlocksToReadPerAccess_;
  }

  virtual void setNumberOfBlocksToReadPerAccess(const Lng32& blocks)
  {
    DCMPASSERT(blocks > -1);
    numberOfBlocksToReadPerAccess_ = blocks;
  }

 virtual PlanPriority computeOperatorPriority
    (const Context* context,
     PlanWorkSpace *pws=NULL,
     Lng32 planNumber=0);

  // -----------------------------------------------------
  // generate CONTROL QUERY SHAPE fragment for this node.
  // -----------------------------------------------------
  virtual short generateShape(CollHeap * space, char * buf, NAString * shapeStr = NULL);

  inline const CostScalar getEstRowsAccessed() const 
    { return estRowsAccessed_; }
  inline void setEstRowsAccessed(CostScalar r)  { estRowsAccessed_ = r; }


  virtual void setSkipRowsToPreventHalloween() 
    { skipRowsToPreventHalloween_ = TRUE; }

  inline const ValueIdList &minMaxHJColumns() const { return minMaxHJColumns_; }
  inline void addMinMaxHJColumn(const ValueId &col) { minMaxHJColumns_.insert(col); }

  CostScalar getProbes() const { return probes_;};
  CostScalar getSuccessfulProbes() const { return successfulProbes_; };
  CostScalar getUniqueProbes() const { return uniqueProbes_; };
  CostScalar getDuplicatedSuccProbes() const { return duplicateSuccProbes_; }
  CostScalar getFailedProbes() const { return failedProbes_; }
  CostScalar getTuplesProcessed() const { return tuplesProcessed_; }

  void setProbes(CostScalar  x) { probes_ = x;};
  void setSuccessfulProbes(CostScalar  x) { successfulProbes_ = x; };
  void setUniqueProbes(CostScalar  x) { uniqueProbes_ = x; };
  void setDuplicatedSuccProbes(CostScalar  x) { duplicateSuccProbes_ = x; }
  void setFailedProbes(CostScalar  x) { failedProbes_ = x; }
  void setTuplesProcessed(CostScalar x) { tuplesProcessed_ = x; }

  void setDoUseSearchKey(NABoolean x) { doUseSearchKey_ = x; };
  NABoolean getDoUseSearchKey() { return doUseSearchKey_ ; };

  RangePartitioningFunction*
     createRangePartFuncForHbaseTableUsingStats(
           Lng32& partns,
           const ValueIdSet& partitioningKeyColumns,
           const ValueIdList& partitioningKeyColumnsList,
           const ValueIdList& partitioningKeyColumnsOrder);

  Int32 getComputedNumOfActivePartiions()  const { return computedNumOfActivePartitions_; }

protected:

  void setRetrievedCols(const ValueIdSet &retrievedCols)
                                           { retrievedCols_ = retrievedCols; }

private:


  //--------------- Disjuncts -------------------
  MdamKey *mdamKeyPtr_;
  const Disjuncts *disjunctsPtr_;
  // computed predicates that are reflected in the disjuncts
  // (those are predicates on computed columns that are
  // computed from regular predicates)

  //--------------- Search key ----------------

  // A key object. It cannot be constant because some of its
  // members are modified in GenPreCode.C:FileScan:preCodeGen
  SearchKey * pathKeys_;

  // a search key for the partitioning key (maybe same as pathKeys_)
  SearchKey * partKeys_;

  // For Hive tables, a bit mask of selected Hive partitions and buckets
  HivePartitionAndBucketKey *hiveSearchKey_;

  // the index descriptor
  const IndexDesc *indexDesc_;
  // scan direction
  const NABoolean reverseScan_;


  //--------- Members to be used at code generation:
  // executor predicates
  ValueIdSet executorPredicates_;
  ItemExpr * executorPredTree_;

  // Columns to be retrieved from filesystem
  ValueIdSet retrievedCols_;

  // key predicates
  ValueIdList beginKeyPred_;
  ValueIdList endKeyPred_;

  // helper functions:
  void init();

  // Estimate of number of blocks that DP2 needs to read
  // per access. This value is passed to DP2 by the executor,
  // DP2 uses it to decide whether it will do read ahead
  // or not.
  // Its value is -1 if uninitialized
  Lng32 numberOfBlocksToReadPerAccess_;
  MdamFlags mdamFlag_;

    // This flag is needed only by EXPLAIN.
  NABoolean skipRowsToPreventHalloween_;
  // Estimated number of Dp2 rows accessed.
  CostScalar estRowsAccessed_;

  // columns used for min/max hash join optimization
  ValueIdList minMaxHJColumns_;

  /////////////////////////////////////////////////////
  // probe counts, transfered from ScanOptimizerFileScan
  /////////////////////////////////////////////////////

  // The total number of probes 
  CostScalar probes_;

  // The total number of probes that return data
  CostScalar successfulProbes_;

  // The number of distinct probes (successful and failed)
  CostScalar uniqueProbes_;

  // The number of successful probes (successfulProbes_) that
  // are not unique.  duplicateSuccProbes = successfulProbes -
  // uniqueSuccProbes.  
  CostScalar duplicateSuccProbes_;

  // the number of probes do not return 
  // data.  failedProbes = probes - successfulProbes
  CostScalar failedProbes_;

  // total number of tuples processed under 'probes_' probes
  CostScalar tuplesProcessed_;

  NABoolean doUseSearchKey_;

  // number of active partitions computed only from the Range Part Func
  // and the search key (partKey_)
  Int32 computedNumOfActivePartitions_;

}; // class FileScan


class DP2Scan : public FileScan
{
public:

  // constructor
  DP2Scan(const CorrName& tableName,
          TableDesc * tableDescPtr,
          const IndexDesc *indexDescPtr,
          const NABoolean isReverseScan,
          const Cardinality& baseCardinality,
          StmtLevelAccessOptions& accessOptions,
          GroupAttributes * groupAttributesPtr,
          const ValueIdSet& selectionPredicates,
          const Disjuncts& disjuncts
         );

  DP2Scan(const CorrName & name,
          TableDesc *tableDesc,
          const IndexDesc *indexDesc,
          OperatorTypeEnum otype = REL_FILE_SCAN)
  : FileScan (name, tableDesc, indexDesc, otype)
    {
    }

  // Obtain a pointer to a CostMethod object that provides access
  // to the cost estimation functions for nodes of this type.
  virtual CostMethod* costMethod() const;
};

// -----------------------------------------------------------------------
/*!
*  \brief HbaseAccess Class.
*/
// -----------------------------------------------------------------------

//
//
// At some point of time, we need to revisit the class hierarchy for Scan
// HbaseAccess, FileScan. Quite many code in FileScan (such as costing) is
// relevant to both HbaseAccess and Hive scan, and yet there are those not
// relevant at all (MDAM). A possible new class hierarchy can be as follows.
//
// Scan -> FileScan(with general cost) -> SQFileScan (SQ specific costing code)-> DP2SCan
//                                     -> HiveFileSCan
//                                     -> HbaseFileSCan
//

class HbaseAccess : public FileScan 
{
public:
  enum AccessType
  {
    SELECT_,
    INSERT_,
    UPDATE_,
    DELETE_,
    COPROC_AGGR_
  };

  enum SNPType
  {
    SNP_NONE,
    SNP_SUFFIX,
    SNP_LATEST
  };
  //latest snapshot support --
  enum LatestSnpSupportEnum  {
    latest_snp_supported,
    latest_snp_index_table,// index table not supported with latest snapshot
    latest_snp_no_snapshot_available,//snapshot not available
    latest_snp_not_trafodion_table, // not a trafodion table
    latest_snp_small_table //table is smaller than a certain threshold
  };

  class SortValue
  {
  public:
    SortValue() 
      {
      };
    
    bool operator < (const SortValue & other ) const
    {
      return (str_ < other.str_);
    }
    
    bool operator == (const SortValue & other ) const
    {
      return (str_ == other.str_);
    }
    
    NAString str_;
    ValueId vid_;
  };

  // constructor

  //! HbaseAccess Constructor
  HbaseAccess(CorrName &corrName,
	      OperatorTypeEnum otype = REL_HBASE_ACCESS,
	      CollHeap *oHeap = CmpCommon::statementHeap());

  HbaseAccess(CorrName &corrName,
	      TableDesc *tableDesc,
	      IndexDesc *indexDesc,
              const NABoolean isReverseScan,
              const Cardinality& baseCardinality,
              StmtLevelAccessOptions& accessOptions,
              GroupAttributes * groupAttributesPtr,
              const ValueIdSet& selectionPredicates,
              const Disjuncts& disjuncts,
              const ValueIdSet& generatedCCPreds,
	      OperatorTypeEnum otype = REL_HBASE_ACCESS,
              CollHeap *oHeap = CmpCommon::statementHeap());
 
  HbaseAccess(CorrName &corrName,
	      NABoolean isRW, NABoolean isCW,
	      CollHeap *oHeap = CmpCommon::statementHeap());
  
  HbaseAccess(OperatorTypeEnum otype = REL_HBASE_ACCESS,
	      CollHeap *oHeap = CmpCommon::statementHeap());

  // destructors
  //! ~HbaseAccess Destructor
  virtual ~HbaseAccess();

  virtual RelExpr *bindNode(BindWA *bindWAPtr);

  virtual ExplainTuple * addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
						ComTdb * tdb,
						Generator *generator);
  
  //! getPotentialOutputValues method
  // a virtual function for computing the potential outputValues
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  virtual Lng32 numParams() { return 1; }

  // acessors
  //! getVirtualTableName method
  //  returns a const char pointer to the name of the virtual Table
  // should return a CorrName?##
  virtual const char *getVirtualTableName()
  {
    return getTableName().getQualifiedNameObj().getObjectName().data();
  }

  //used to retrieve the operator type
  virtual const NAString getText() const;

  //! getArity method
  // get the degree of this node (it is a leaf op).
  virtual Int32 getArity() const { return 0; }

  // mutators

  //! createVirtualTableDesc method
  //  creates a TrafDesc for the Virtual Table
  //  virtual TrafDesc *createVirtualTableDesc();
  static TrafDesc *createVirtualTableDesc(const char * name,
					     NABoolean isRW = FALSE,
					     NABoolean isCW = FALSE, 
                                             NAArray<HbaseStr> * hbaseKeys = NULL);

  static TrafDesc *createVirtualTableDesc(const char * name,
					     NAList<char*> &colNameList,
					     NAList<char*> &colValList);

  // return TRUE, if col lengths in virtual desc is same as current default values.
  // return FALSE, if they are different. If they are, caller will clear the cache
  // and generate a new descriptor.
  static NABoolean validateVirtualTableDesc(NATable * naTable);

  NAList<HbaseSearchKey*>& getHbaseSearchKeys() { return listOfSearchKeys_; };
  void addSearchKey(HbaseSearchKey* sk) 
    { listOfSearchKeys_.insertAt(listOfSearchKeys_.entries(), sk); };
  
  //! getPotentialOutputValues method
  //  computes the output values the node can generate
  //  Relies on TableValuedFunctions implementation.

  // determine which columns to retrieve from the file
  void computeRetrievedCols();

  //! preCodeGen method 
  //  method to do preCode generation
  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet & externalInputs,
                               ValueIdSet &pulledNewInputs);

  static short processNonSQHbaseKeyPreds(Generator * generator,
				      ValueIdSet &preds,
				      ListOfUniqueRows &listOfUniqueRows,
				      ListOfRangeRows &listOfRangeRows);

  static short processSQHbaseKeyPreds(Generator * generator,
				      NAList<HbaseSearchKey*>& searchKeys,
				      ListOfUniqueRows &listOfUniqueRows,
				      ListOfRangeRows &listOfRangeRows);
  
  //! codeGen method
  // virtual method used by the generator to generate the node
  virtual short codeGen(Generator*);

  //! copyTopNode
  //  used to create an Almost complete copy of the node
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  //! synthLogProp method
  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);

  //! synthEstLogProp method
  // used by the compiler to estimate cost
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  //! getText method
  //  used to display the name of the node.
  //  virtual const NAString getText() const;

  AccessType &accessType() { return accessType_; }

  NABoolean isRW() { return isRW_; }
  NABoolean isCW() { return isCW_; }

  virtual NABoolean isLogical() const { return FALSE; }
  virtual NABoolean isPhysical() const { return TRUE; }

  virtual NABoolean isHbaseScan() { return TRUE; }

  static int createAsciiColAndCastExpr(Generator * generator,
				       const NAType &givenType,
				       ItemExpr *&asciiValue,
				       ItemExpr *&castValue,
                                       NABoolean srcIsInt32Varchar = FALSE);

  static int createAsciiColAndCastExpr2(Generator * generator,
				       ItemExpr * colNode,
				       const NAType &givenType,
				       ItemExpr *&asciiValue,
				       ItemExpr *&castValue,
                                       NABoolean alignedFormat = FALSE);

  // used for hbase map table where data is stored in string format
  static int createAsciiColAndCastExpr3(Generator * generator,
				       const NAType &givenType,
				       ItemExpr *&asciiValue,
                                        ItemExpr *&castValue);

  static short genRowIdExpr(Generator * generator,
			    const NAColumnArray & keyColumns,				
			    NAList<HbaseSearchKey*>& searchKeys,
			    ex_cri_desc * work_cri_desc,
			    const Int32 work_atp,
			    const Int32 rowIdAsciiTuppIndex,
			    const Int32 rowIdTuppIndex,
			    ULng32 &rowIdAsciiRowLen,
			    ExpTupleDesc* &rowIdAsciiTupleDesc,
			    UInt32 &rowIdLength,
			    ex_expr* &rowIdExpr,
                            NABoolean encodeKeys);
  
 static short genRowIdExprForNonSQ(Generator * generator,
			    const NAColumnArray & keyColumns,				
			    NAList<HbaseSearchKey*>& searchKeys,
			    ex_cri_desc * work_cri_desc,
			    const Int32 work_atp,
			    const Int32 rowIdAsciiTuppIndex,
			    const Int32 rowIdTuppIndex,
			    ULng32 &rowIdAsciiRowLen,
			    ExpTupleDesc* &rowIdAsciiTupleDesc,
			    UInt32 &rowIdLength,
			    ex_expr* &rowIdExpr);
 
  static short genListsOfRows(Generator * generator,
			      NAList<HbaseRangeRows> &listOfRangeRows,
			      NAList<HbaseUniqueRows> &listOfUniqueRows,
			      Queue* &tdbListOfRangeRows,
			      Queue* &tdbListOfUniqueRows);
  
  static short genColName(Generator * generator,
			  ItemExpr * col_node,
			  char* &colNameInList);

  static short genListOfColNames(Generator * generator,
				 const IndexDesc * indexDesc,
				 ValueIdList &columnList,
				 Queue* &listOfColNames);

  static void addReferenceFromItemExprTree(ItemExpr * ie,
                                    NABoolean addCol, NABoolean addHBF,
                                    ValueIdSet &colRefVIDset);
  
  static void addColReferenceFromVIDlist(const ValueIdList &exprList,
					 ValueIdSet &colRefVIDset);

  static void addReferenceFromVIDset(const ValueIdSet &exprList,
                                     NABoolean addCol, NABoolean addHBF,
                                     ValueIdSet &colRefVIDset);
  
  static void addColReferenceFromRightChildOfVIDarray(ValueIdArray &exprList,
						      ValueIdSet &colRefVIDset);

  static short createHbaseColId(const NAColumn * nac,
				NAString &cid,
				NABoolean isSecondaryIndex = FALSE);

  static short returnDups(std::vector<SortValue> &myvector,
                          ValueIdList &srcVIDlist, ValueIdList &dupVIDlist);

  static short createSortValue(ItemExpr * col_node,
			       std::vector<SortValue> &myvector,
			       NABoolean isSecondaryIndex = FALSE);

  static short sortValues (const ValueIdList &inList,
			   ValueIdList &sortedList, 
			   NABoolean isSecondaryIndex = FALSE);

  static short sortValues (const ValueIdSet &inSet,
                           ValueIdList &sortedList,
                           ValueIdList &srcVIDlist,
                           ValueIdList &dupVIDlist,
			   NABoolean isSecondaryIndex = FALSE);

  static short sortValues (NASet<NAString> &inSet,
			   NAList<NAString> &sortedList);

  static NABoolean columnEnabledForSerialization(ItemExpr * colNode);

  static NABoolean isEncodingNeededForSerialization(ItemExpr * colNode);

  //const IndexDesc* getIndexDesc() const { return indexDesc_; };

  NABoolean &uniqueHbaseOper() { return uniqueHbaseOper_; }
  NABoolean &uniqueRowsetHbaseOper() { return uniqueRowsetHbaseOper_; }
  NABoolean uniqueRowsetHbaseOper() const { return uniqueRowsetHbaseOper_; }

  NABoolean isHbaseFilterPred(Generator * generator, ItemExpr * ie,
			      ValueId &colVID, ValueId &valueVID,
			      NAString &op,
			      NABoolean &removeFromOrigList);

  short extractHbaseFilterPreds(Generator * generator, ValueIdSet &preds,
                                ValueIdSet &newExePreds);

  NABoolean isHbaseFilterPredV2(Generator * generator, ItemExpr * ie,
                                ValueId &colVID, ValueId &valueVID,
                                NAString &op);

  short extractHbaseFilterPredsVX(Generator * generator,ValueIdSet &preds, ValueIdSet &newExePreds);

  NABoolean extractHbaseFilterPredsV2(Generator * generator, ValueIdSet &preds, ValueIdSet &newExePreds,
                                      NABoolean checkOnly);

  NABoolean isSnapshotScanFeasible(LatestSnpSupportEnum snpNotSupported,
                                   char * tableName);

 protected:

  // The search keys built during optimization.
  NAList<HbaseSearchKey*> listOfSearchKeys_;

  ListOfUniqueRows listOfUniqueRows_;
  ListOfRangeRows listOfRangeRows_;
  
  //! HbaseAccess Copy Constructor
  // Copy constructor without heap is not implemented nor allowed.
  HbaseAccess(const HbaseAccess &other);

  //CorrName corrName_;

  AccessType accessType_;

  ItemExpr * rowIdCast_;

  NABoolean isRW_;
  NABoolean isCW_;

  // list of columns that need to be retrieved from hbase. These are used
  // in executor preds, update/merge expressions or returned back as output
  // values.
  ValueIdSet retColRefSet_;

  NASet<NAString> retHbaseColRefSet_;

  NABoolean uniqueHbaseOper_;
  NABoolean uniqueRowsetHbaseOper_;

  // these lists are populated during the preCodeGen phase.
  ValueIdList hbaseFilterColVIDlist_;
  ValueIdList hbaseFilterValueVIDlist_;
  NAList<NAString> opList_;
  SNPType snpType_;

}; // class HbaseAccess

// NOTE: this class is currently not being used.
class HbaseAccessCoProcAggr : public HbaseAccess
{
public:
  HbaseAccessCoProcAggr(CorrName &corrName,
			ValueIdSet &aggrExpr,
			TableDesc *tableDesc,
			IndexDesc *indexDesc,
			const NABoolean isReverseScan,
			const Cardinality& baseCardinality,
			StmtLevelAccessOptions& accessOptions,
			GroupAttributes * groupAttributesPtr,
			const ValueIdSet& selectionPredicates,
			const Disjuncts& disjuncts,
			CollHeap *oHeap = CmpCommon::statementHeap());

  HbaseAccessCoProcAggr(CorrName &corrName,
			ValueIdSet &aggrExpr,
			CollHeap *oHeap = CmpCommon::statementHeap());

  HbaseAccessCoProcAggr(CollHeap *oHeap = CmpCommon::statementHeap());
 
  // destructors
  //! ~HbaseAccess Destructor
  virtual ~HbaseAccessCoProcAggr();

  virtual RelExpr *bindNode(BindWA *bindWAPtr);

  //used to retrieve the operator type
  virtual const NAString getText() const;

  //  method to do preCode generation
  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet & externalInputs,
                               ValueIdSet &pulledNewInputs);

  //! codeGen method
  // virtual method used by the generator to generate the node
  virtual short codeGen(Generator*);

  //! copyTopNode
  //  used to create an Almost complete copy of the node
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  virtual void getPotentialOutputValues(ValueIdSet & outputValues) const;

  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
						  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);
  
  virtual NABoolean isLogical() const { return TRUE; }
  virtual NABoolean isPhysical() const { return TRUE; }
  
  ValueIdSet &aggregateExpr() { return aggregateExpr_; }
  const ValueIdSet &aggregateExpr() const { return aggregateExpr_; }
 protected:

  // list of columns that need to be retrieved from hbase. These are used
  // in executor preds, update/merge expressions or returned back as output
  // values.
  ValueIdSet aggregateExpr_;

}; // class HbaseAccessCoProcAggr

// -----------------------------------------------------------------------
// The Describe class represents scan on a 'virtual' table that contains
// the output of the DESCRIBE(SHOWDDL) command.
// -----------------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination
class Describe : public Scan
{
public:
  enum Format
  {
    INVOKE_,  // describe sql/mp INVOKE style
    SHOWSTATS_, //display histograms for specified table
    SHORT_,   // just show ddl for table
    LONG_,    // show everything about this table (ddl, indexes, views, etc)
    PLAN_,    // return information about runtime plan. Currently, details
              // about expressions and clauses are the only info returned.
              // For internal debugging use only. Not externalized to users.
    LABEL_,   // For showlabel
    SHAPE_,   // For SHOWSHAPE. Generates a CONTROL QUERY SHAPE statement for
              // the given query.
    TRANSACTION_, // display current transaction settings in effect for
                  // those items that can be set by a SET TRANSACTION statement.
    SET_,     // For SHOWSET. Shows all runtime SET stmts in effect.
    ENVVARS_, // For 'get envvars'. Shows all envvars in effect.

    // show the corresponding control options that are in effect.
    CONTROL_FIRST_,
    CONTROL_ALL_ = CONTROL_FIRST_,
    CONTROL_SHAPE_,
    CONTROL_DEFAULTS_,
    CONTROL_TABLE_,
    CONTROL_SESSION_,
    CONTROL_LAST_ = CONTROL_SESSION_,

    LEAKS_,    // For SHOWLEAKS.
    STATIC_PLAN_,// same as PLAN_ but from static pre-compiled programs.
                 // Not externalized.
    SHOWQRYSTATS_ // display histograms for specified query
  };
  enum ShowcontrolOption
  {
    MATCH_FULL_,
    MATCH_FULL_NO_HEADER_,     // internal option, meant for use by utilities
    MATCH_PARTIAL_,
    MATCH_NONE_
  };

  // --------------------------------------------------------------------------
  // Note: Parser passes in value UR of parameter labelAnsiNameSpace for
  // procedures if the keyword PROCEDURE is specified, even though the valid
  // namespace value of the procedure is TA. SHOWDDL has been changed to
  // no longer rely on the PROCEDURE keyword to support stored procedures.
  // --------------------------------------------------------------------------

  Describe(char * originalQuery,
           const CorrName &describedTableName,
           Format format = LONG_,
           ComAnsiNameSpace labelAnsiNameSpace = COM_TABLE_NAME,
           ULng32 flags = 0,
	   NABoolean header = TRUE)
    : Scan(REL_DESCRIBE),
      describedTableName_(describedTableName, CmpCommon::statementHeap()),
      pUUDFName_(NULL),
      format_(format),
      labelAnsiNameSpace_(labelAnsiNameSpace), // See comments above
      flags_(flags),
      header_(header),  // header = FALSE will produce showcontrol output with
                        // any header information. This option will be used by
		        //utilities and is not externalised.
      isSchema_(false),
      toTryPublicSchema_(false),
      authIDClass_(COM_UNKNOWN_ID_CLASS),
      isComponent_(false)
    {
      setNonCacheable();
      if (originalQuery)
        {
          originalQuery_ = new (CmpCommon::statementHeap())
                           char[strlen(originalQuery)+1];
          strcpy(originalQuery_, originalQuery);
        }
      else
        originalQuery_ = NULL;
    }

  Describe(char * originalQuery,
           const SchemaName &schemaName,
           Format format = LONG_,
           ULng32 flags = 0,
	   NABoolean header = TRUE)
    : Scan(REL_DESCRIBE),
      describedTableName_("", CmpCommon::statementHeap()),
      pUUDFName_(NULL),
      schemaQualName_(schemaName, CmpCommon::statementHeap()),
      format_(format),
      flags_(flags),
      header_(header),  // header = FALSE will produce showcontrol output with
                        // any header information. This option will be used by
		        //utilities and is not externalised.
      isSchema_(true),
      toTryPublicSchema_(false),
      authIDClass_(COM_UNKNOWN_ID_CLASS),
      isComponent_(false)
    {
      setNonCacheable();
      if (originalQuery)
        {
          originalQuery_ = new (CmpCommon::statementHeap())
                           char[strlen(originalQuery)+1];
          strcpy(originalQuery_, originalQuery);
        }
      else
        originalQuery_ = NULL;

      if (schemaQualName_.getCatalogName().isNull())
        schemaQualName_.setCatalogName(
	          ActiveSchemaDB()->getDefaultSchema().getCatalogName());

      schemaName_ = ToAnsiIdentifier(schemaQualName_.getCatalogName()) + "." +
                    ToAnsiIdentifier(schemaQualName_.getSchemaName());

      CorrName temp(schemaQualName_.getSchemaName(),CmpCommon::statementHeap(),
		    schemaQualName_.getSchemaName(),schemaQualName_.getCatalogName());

      describedTableName_ = temp;
    }
  // constructor used for SHOWDDL USER and SHOWDDL ROLE
  Describe(char * originalQuery,
           ComIdClass authIDClass,
           const NAString &authIDName,
           Format format = LONG_,
           ULng32 flags = 0,
           NABoolean header = TRUE)
    : Scan(REL_DESCRIBE),
      describedTableName_("", CmpCommon::statementHeap()),
      pUUDFName_(NULL),
      schemaQualName_("", CmpCommon::statementHeap()),
      format_(format),
      flags_(flags),
      header_(header),  // header = FALSE will produce showcontrol output with
                        // any header information. This option will be used by
                        //utilities and is not externalised.
      isSchema_(false),
      toTryPublicSchema_(false),
      labelAnsiNameSpace_(COM_UNKNOWN_NAME),
      authIDClass_ (authIDClass),
      authIDName_(authIDName),
      isComponent_(false)
   {
      setNonCacheable();
      if (originalQuery)
        {
          originalQuery_ = new (CmpCommon::statementHeap())
                           char[strlen(originalQuery)+1];
          strcpy(originalQuery_, originalQuery);
        }
      else
        originalQuery_ = NULL;
    }

  // constructor used for SHOWDDL USER and SHOWDDL ROLE
  Describe(char * originalQuery,
           const NAString &componentName,
           Format format = LONG_,
           ULng32 flags = 0,
           NABoolean header = TRUE)
    : Scan(REL_DESCRIBE),
      describedTableName_("", CmpCommon::statementHeap()),
      pUUDFName_(NULL),
      schemaQualName_("", CmpCommon::statementHeap()),
      format_(format),
      flags_(flags),
      header_(header),  // header = FALSE will produce showcontrol output with
                        // any header information. This option will be used by
                        //utilities and is not externalised.
      isSchema_(false),
      toTryPublicSchema_(false),
      labelAnsiNameSpace_(COM_UNKNOWN_NAME),
      authIDClass_ (COM_UNKNOWN_ID_CLASS),
      componentName_(componentName),
      isComponent_(true)
   {
      setNonCacheable();
      if (originalQuery)
        {
          originalQuery_ = new (CmpCommon::statementHeap())
                           char[strlen(originalQuery)+1];
          strcpy(originalQuery_, originalQuery);
        }
      else
        originalQuery_ = NULL;
    }

  ~Describe()   { delete originalQuery_; delete pUUDFName_; }

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // this is both logical and physical node
  virtual NABoolean isLogical() const   {return TRUE;}
  virtual NABoolean isPhysical() const  {return TRUE;}

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  // method to do code generation
  virtual short codeGen(Generator*);

  // various PC methods

  // get the degree of this node (it is a leaf op).
  virtual Int32 getArity() const          {return 0;}
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);
  virtual const NAString getText() const;

  const CorrName &getDescribedTableName() const
  {
    return describedTableName_;
  }

  Format getFormat() const              {return format_;}

  NABoolean getIsBrief() const
  {
    return (flags_ & 0x00000008);
  }

  NABoolean getIsDetail() const
  {
    return (flags_ & 0x00000004);
  }

  NABoolean getIsExternal() const
  {
    return (flags_ & 0x00000020);
  }

  NABoolean getIsInternal() const
  {
    return (flags_ & 0x00000040);
  }

  NABoolean getIsSchema() const
  {
    return isSchema_;
  }

  NABoolean getIsUser() const
  {
    return (authIDClass_ == COM_USER_CLASS);
  }
   
  NABoolean getIsRole() const
  {
    return (authIDClass_ == COM_ROLE_CLASS);
  }

  NABoolean getIsComponent() const
  {
    return isComponent_;
  }

  ComAnsiNameSpace getLabelAnsiNameSpace() const
  {
    return labelAnsiNameSpace_;
  }

  // TRUE  => output detail (long) label info
  // FALSE => output short label info
  NABoolean getLabelDetail() const
  {
    return (flags_ & 0x00000001);
  }
  void setLabelDetail(const NABoolean wantDetail)
  {
     if(wantDetail)
     		flags_ = flags_ | 0x00000001;
     else
     		flags_ = flags_ & 0xFFFFFFFE;
  }


  NABoolean getDisplayPrivileges() const
  {
    return (flags_ & 0x00000010);
  }

  NABoolean getDisplayPrivilegesOnly() const
  {
    return (flags_ & 0x00000080);
  }

  NABoolean getDisplaySynonymsOnly() const
  {
    return (flags_ & 0x00000100);
  }

  void setLabelAnsiNameSpace(const ComAnsiNameSpace nameSpace)
  {
    labelAnsiNameSpace_ = nameSpace;
  }

  ULng32 getFlags() const
  {
    return flags_;
  }
  void setFlags(const ULng32 flags)
  {
    flags_ = flags;
  }

  NABoolean getHeader() const
  {
    return header_;
  }
  
  const SchemaName& getSchemaQualifiedName() const
  {
    return schemaQualName_;
  }

  const NAString& getSchemaName() const
  {
    return schemaName_;
  }

  const NAString& getAuthIDName() const
  {
    return authIDName_;
  }

  const NAString& getComponentName() const
  {
    return componentName_;
  }

  NABoolean toTryPublicSchema() const
  {
    return toTryPublicSchema_;
  }

  void setToTryPublicSchema(const NABoolean val)
  {
    toTryPublicSchema_=val;
  }

  void setDescribedTableSchema(const NAString cat, const NAString sch)
  {
    describedTableName_.getQualifiedNameObj().setCatalogName(cat);
    describedTableName_.getQualifiedNameObj().setSchemaName(sch);
  }

  const QualifiedName * getUudfQualNamePtr() const
  {
    return pUUDFName_;
  }

  QualifiedName * getUudfQualNamePtr()
  {
    return pUUDFName_;
  }

  const QualifiedName & getUudfQualName() const
  {
    if (pUUDFName_ EQU NULL)
    {
      const_cast<Describe*>(this)->pUUDFName_ =
        new (STMTHEAP) QualifiedName(STMTHEAP);
    }
    return *pUUDFName_;
  }

  QualifiedName & getUudfQualName()
  {
    if (pUUDFName_ EQU NULL)
      pUUDFName_ = new (STMTHEAP) QualifiedName(STMTHEAP);
    return *pUUDFName_;
  }

  void setUudfQualNamePtr(QualifiedName * pUudfQualName) // shallow copy
  {
    pUUDFName_ = pUudfQualName; // shallow copy
    // pUudfQualName should point to a QualifiedName object
    // allocated in the PARSERHEAP().  pUUDFName_ is only used
    // with SHOWDDL ACTION <action-qual-name> ON UNIVERSAL
    // FUNCTION <uudf-qual-name> ... syntax.
  }

  void setUudfQualName(const QualifiedName & src) // deep copy
  {
    if (pUUDFName_ EQU NULL)
      pUUDFName_ = new (STMTHEAP) QualifiedName(STMTHEAP);
    *pUUDFName_ = src;
  }

  NABoolean maybeInMD() const
  {
    return (
      format_ == INVOKE_    ||
      format_ == SHOWSTATS_ ||
      format_ == SHORT_     ||
      format_ == LONG_      ||
      format_ == LABEL_) ;
  }

private:
  CorrName describedTableName_;
  // pUUDFName_ is only used when describedTableName_ contains a routine action
  QualifiedName *pUUDFName_;

  Format format_;

  char * originalQuery_;

  // used for SHOWLABEL
  ComAnsiNameSpace labelAnsiNameSpace_;

  ULng32 flags_;

  NABoolean header_;
  NABoolean isSchema_;
  SchemaName schemaQualName_;
  NAString schemaName_;
  NABoolean toTryPublicSchema_;

  // Added for SHOWDDL USER & SHOWDDL ROLE
  ComIdClass authIDClass_;
  NAString   authIDName_;

  // Added for SHOWDDL component
  NABoolean isComponent_;
  NAString  componentName_;

  // copy ctor
  Describe (const Describe & des) ;  // not defined - DO NOT USE
}; // class Describe
#pragma warn(1506)  // warning elimination

#endif /* RELSCAN_H */
