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
#ifndef _SCAN_OPTIMIZER_H
#define _SCAN_OPTIMIZER_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:  ScanOptimizer.h
 * RCS:          
 * Description:  Compute the cost and generate the appropiate key for the
 *                the scan.
 * Code location: ScanOptimizer.C
 *
 * Created:      //96
 * Language:     C++
 *
 * Purpose:	  Simple Cost Vector Reduction changes to class
 *                FileScanBasicCost
 *

 *
 *
 *
 *****************************************************************************
 */
// -----------------------------------------------------------------------

#include "GroupAttr.h"
#include "RelExpr.h"
#include "RelScan.h"
#include "disjuncts.h"
#include "Cost.h"
#include "CostScalar.h"
#include "opt.h"
#include "NABasicObject.h"
#include "ColStatDesc.h"
#include "Stats.h"

// -----------------------------------------------------------------------
// forward declarations
// -----------------------------------------------------------------------

class FileScanBasicCost;
class FileScanCostList;

void
removeConstantsFromTargetSortKey(ValueIdList* targetSortKey,
                                 ValueIdMap* map) ;
NABoolean
isOrderedNJFeasible(ValueIdList leftKeys, ValueIdList rightKeys) ;

NABoolean
ordersMatch(const InputPhysicalProperty* ipp,
            const IndexDesc* indexDesc,
            const ValueIdList* innerOrder,
            const ValueIdSet& charInputs,
            NABoolean partiallyInOrderOK,
            NABoolean& probesForceSynchronousAccess,
	    
	    // if TRUE, do not assert, just return FALSE.
	    NABoolean noCmpAssert = FALSE);

// getDp2CacheSizeInBlocks()
// Given a block size, this method returns the number of blocks in
// cache for blocks of that size.
//
CostScalar getDP2CacheSizeInBlocks(const CostScalar& blockSizeInKb);

// -----------------------------------------------------------------------
// class Histograms facilitate the deep copy and use of ColStatDescList's
// -----------------------------------------------------------------------
class Histograms : public NABasicObject {
public:

  Histograms(CollHeap* heap):colStatDescList_(heap) {}
  Histograms():colStatDescList_(CmpCommon::statementHeap()) {}
  Histograms(const ColStatDescList& colStatDescList);

  virtual ~Histograms();
  
  // -----------------------------------------------------------------------
  // --- Accessors:
  // -----------------------------------------------------------------------
  // get the number of histograms for this table:
  CollIndex entries() const
  { return colStatDescList_.entries(); }

  // get the ColStatDesc in the i-th position:
  const ColStatDesc& operator[](CollIndex i) const
  { return *(colStatDescList_[i]); }
  
  // Check whether a ColStatDesc for a given column already exists:
  NABoolean contains(ValueId & column) const
  { return colStatDescList_.contains(column); }
    
  
  // get the number of total rows in the table after
  // predicates were applied:
  CostScalar getRowCount() const;


  NABoolean getColStatDescForColumn(CollIndex index, /* out */
                                            const ValueId& column) const;

  const ColStats& getColStatsForColumn(const ValueId& column) const;
  // $$$ this next method is to work around the fact
  // $$$ that prototype hist. code do not support fake histograms
  ColStatsSharedPtr getColStatsPtrForColumn(const ValueId& column) const;
  // get multiColUecCount for set of columns, would work even for single 
  // columns as 
  CostScalar getUecCountForColumns(const ValueIdSet& columns) const;
  // -----------------------------------------------------------------------
  // This method returns the ColStatsSharedPtr for the ColStats that references
  // the given predicate if it exists, otherwise it returns NULL
  // -----------------------------------------------------------------------
  ColStatsSharedPtr getColStatsPtrForPredicate(const ValueId& predicate) const
  { return colStatDescList_.getColStatsPtrForPredicate(predicate); }
  
  // Returns TRUE if at least one of the histograms is a
  // fake histogram. A fake histogram is a histogram that
  // was synthesized by ColStats from info. other than statistics.
  // We need to test for this because we don't want to cost
  // MDAM relying on fake histograms. MDAM must not be chosen
  // when there are fake histograms.
  NABoolean containsAtLeastOneFake() const;


  // $$$ In theory, there should always be at least
  // on ColStats for every column of a base
  // table, even if no statistics has been collected,
  // However, as of today (04/11/97), this is not true.
  // Therefore, to avoid braking the regressions
  // I will test for this case using the function below.
  // When this situation is corrected, this function should
  // always return FALSE.
  NABoolean isEmpty() const
  { return colStatDescList_.isEmpty(); }


  // -----------------------------------------------------------------------
  // Mutators
  // -----------------------------------------------------------------------
  void append(const ColStatDescSharedPtr& colStatDesc);
  
  // we need some way of propagating the multi-column uec list
  inline void setCSDLUecList (const MultiColumnUecList * other)
  { 
    colStatDescList_.insertIntoUecList (other) ;
  }


  // -----------------------------------------------------------------------
  // isAnIndexJoin returns TRUE when this histogram and
  // the histogram in the inputEstLogProp represent an index join,
  // it returns true otherwise. The optional parameter
  // innerColumnListToRemovePtr must be a properly allocated,
  // empty, ValueIdListPtr when non-null. If non-null, it returns
  // the indices of those histograms in this that are also
  // in the inputEstLogProp (i.e. they refer to index columns)
  // -----------------------------------------------------------------------
  NABoolean
  isAnIndexJoin(const EstLogProp& inputEstLogProp
                ,LIST(CollIndex) *innerIndexColumnListPtr=NULL) const;

  // -----------------------------------------------------------------------
  //  Utility functions
  // -----------------------------------------------------------------------

  void displayHistogramForColumn(const ValueId& column) const;

  void display() const;

  void print (FILE *f = stdout, 
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "") const;

  
  
  //---- Mutators:
  
  // Apply a set of predicates to this histogram
  void applyPredicates(const ValueIdSet& predicates, 
                      const RelExpr & scan,
		      const SelectivityHint * selHint = NULL,
		      const CardinalityHint * cardHint = NULL,
		      OperatorTypeEnum opType = ITM_FIRST_ITEM_OP);
  // Apply a single predicate to this histogram
  void applyPredicate(const ValueId& predicate, 
                     const RelExpr & scan,
		     const SelectivityHint * selHint = NULL,
		     const CardinalityHint * cardHint = NULL,
		     OperatorTypeEnum opType = ITM_FIRST_ITEM_OP);

  // -----------------------------------------------------------------------
  // Use isMDAM = FALSE when using this in single subset
  // Use isMDAM = TRUE when using this in MDAM. In this case
  //              the routine will assume that we are not in an
  //              index join (it should be the case because
  //              code in the early stages should make sure that
  //              we don't attempt MDAM in an index join). This is
  //              not a correctness issue, it is an efficiency issue.
  // -----------------------------------------------------------------------
  void
  applyPredicatesWhenMultipleProbes(
       const ValueIdSet& predicates
     ,const EstLogProp& inputEstLogProp
     ,const ValueIdSet& inputValues
     ,const NABoolean isMDAM
     ,const SelectivityHint * selHint=NULL
      ,const CardinalityHint * cardHint=NULL
     ,NABoolean * isAnIndexJoin=NULL
     ,OperatorTypeEnum opType = ITM_FIRST_ITEM_OP);



  const ColStatDescList& getColStatDescList() const;
  
protected:

    
  ColStatDescList colStatDescList_;


}; // Histograms


// -----------------------------------------------------------------------
// Class IndexDescHistograms
// Encapsulates histogram handling for scan costing of key sequenced files
// -----------------------------------------------------------------------
class IndexDescHistograms : public Histograms {
public:
  // The constructor creates a ColStatDescList
  // from the IndexDesc raw histograms. It will contain
  // columnPosition entries; one entry for each column in
  // all positions <= columnPosition.
  // 1 <= columnPosition <= number of key columns in indexDesc key
  IndexDescHistograms(const IndexDesc& indexDesc,
                      const CollIndex columnPosition);

  // -----------------------------------------------------------------------
  // Accessors:
  // -----------------------------------------------------------------------

  const IndexDesc& getIndexDesc() const
  { return indexDesc_; }

  
  NABoolean isMultiColUecInfoAvail()const;

  // -----------------------------------------------------------------------
  // Mutators:
  // -----------------------------------------------------------------------


  CostScalar 
  computeFailedProbes(const Histograms& outerHistograms
		      ,const ValueIdSet& keyPreds
		      ,const ValueIdSet& inputValues
		      ,const ValueIdSet& operatorValues
		      ) const;

  // Add a histogram to the colStatDescList. This also
  // sinchronizes the histogram we are adding with the
  // existing histograms in IndexDescHistograms
  void appendHistogramForColumnPosition(const CollIndex& columnPosition);

  //-------------------------------------------------------------------------
  //This function is used to get a better estimate of how many uec's that need
  //to be skipped by MDAM for a specific column. It can compute this number
  //if there is multi column uec information for the columns up to the 
  //column under consideration. Then it needs uec/multi-col uec information of
  //some preceding columns.
  //Example : Columns a, b, c, d . We are trying to compute the estimated uec
  //for d. Answer: 
  // can be multicolUec for ((a and/or b and/or c) and d)
  //        ----------------------------
  //        multicolUec for a and/or b and/or c(best we can do is a,b,c)
  //
  //if the denominator is a,b,c then the numerator must be a,b,c,d
  // we must have comparable sets as numerator and denominator
  //Input: columnOrderList which has the columnIdList for the index/table
  //       indexOfcolum in the order list that we have to compute uec info for
  //Output: estimateduec for the column and true as return value
  //        if there isn't enough information then we return false.
  //-------------------------------------------------------------------------- 
  NABoolean estimateUecUsingMultiColUec(
                               const ColumnOrderList& keyPredsByCol,/*in*/
                               const CollIndex& indexOfColumn,/*in*/
                                CostScalar& estimatedUec/*out*/);

private:
  const IndexDesc& indexDesc_;
              

}; // IndexDescHistograms

  

// -----------------------------------------------------------------------
// Class ScanOptimizer
// This is a helper class that performs the following tasks:
// 1.- Generates the different possible keys for a Scan
// 2.- Estimates the cost for every key
// 3.- Chooses the cheapest key and deletes all others
// 4.- Returns the cheapest key and its cost
// -----------------------------------------------------------------------
class MdamTrace;
class ScanOptimizer : public NABasicObject // Abstract class
{
  friend class MdamTrace;
public:
  enum ncmRowSizeFactorType {TUPLES_ROWSIZE_FACTOR = 0,
                            SEQ_IO_ROWSIZE_FACTOR,
                            RAND_IO_ROWSIZE_FACTOR};
  ScanOptimizer(const FileScan& associatedFileScan
                ,const CostScalar& resultSetCardinality
		,const Context& myContext
		,const ValueIdSet& externalInputs);

  virtual ~ScanOptimizer();

  // -----------------------------------------------------------------------
  // Accessors
  // -----------------------------------------------------------------------
  
  virtual CollIndex getNumActivePartitions() const;
  virtual CollIndex getEstNumActivePartitionsAtRuntime() const;
  virtual CollIndex getNumActiveDP2Volumes() const;
  virtual CollIndex getEstNumActivePartitionsAtRuntimeForHbaseRegions() const;

  Lng32 getNumberOfBlocksToReadPerAccess() const
  {
    // make sure that value has been initialized before using it:
    CMPASSERT(numberOfBlocksToReadPerAccess_ > -1);
    return numberOfBlocksToReadPerAccess_; 
  }

  const CostScalar getEstRowsAccessed() const
  {
    return estRowsAccessed_;
  }

  virtual Cost * optimize(SearchKey *& searchKeyPtr // out
			  ,MdamKey *&mdamKeyPtr     // out 
                         ) = 0;
  
  // Return the appropriate Scan Optimizer to use for the given scan
  // with the given context.
  //
  static ScanOptimizer *
  getScanOptimizer(const FileScan& associatedFileScan
                   ,const CostScalar& resultSetCardinality
                   ,const Context& myContext
                   ,const ValueIdSet &externalInputs
                   ,CollHeap* heap = CmpCommon::statementHeap());

  // For use of getScanOptimizer().  Made public for testing purposes.
  //
  static NABoolean 
  useSimpleFileScanOptimizer(const FileScan& associatedFileScan
                             ,const Context& myContext
                             ,const ValueIdSet &externalInputs);

  static NABoolean 
  canStillConsiderMDAM(const ValueIdSet partKeyPreds,
					 const ValueIdSet nonKeyColumnSet,
					 const Disjuncts &curDisjuncts,
					 const IndexDesc * indexDesc,
					 const ValueIdSet externalInputs);

  // get and set various probing counters for all partitions.

  // the total number of probes
  const CostScalar getProbes() const {  return probes_; };

  // the number of probes returning data
  const CostScalar getSuccessfulProbes() const { return successfulProbes_; };

  // the number of probes that are unique (returning one row each)
  const CostScalar getUniqueProbes() const { return uniqueProbes_; };

  // the number of succssful probes returning more than one row
  const CostScalar getDuplicateSuccProbes() const 
          { return duplicateSuccProbes_; };

  // the number of tuples processed
  const CostScalar getTuplesProcessed() const { return tuplesProcessed_; };

  void setProbes(CostScalar x) {  probes_ = x; };
  void setSuccessfulProbes(CostScalar x) { successfulProbes_ = x; };
  void setUniqueProbes(CostScalar x) { uniqueProbes_ = x; };
  void setDuplicateSuccProbes(CostScalar x) { duplicateSuccProbes_ = x; };
  void setTuplesProcessed(CostScalar x) { tuplesProcessed_ = x; };

protected:

  // get the pointer to the object with reusable simple cost vectors 
  // or a new object if sharable not found or list is empty.  SP 09/18/00
  FileScanBasicCost* shareBasicCost(NABoolean &sharedCostFound);

  // -----------------------------------------------------------------------
  // This method computes the cost object out of the first row
  // and last row cost vectors. It also factors in the effect
  // of synchronous access, something that will be probably best
  // done inside the Cost constructor
  // -----------------------------------------------------------------------
  Cost*  computeCostObject(const SimpleCostVector& firstRow
			   ,const SimpleCostVector& lastRow
                          ) const;


  //  Wrapper for SCM Cost constructor, used by SCM only.
  Cost * scmCost( CostScalar tuplesProcessed,
		  CostScalar tuplesProduced,
		  CostScalar tuplesSent,
		  CostScalar ioRand,
		  CostScalar ioSeq,
		  CostScalar noOfProbes,
		  CostScalar input1RowSize,
		  CostScalar input2RowSize,
		  CostScalar outputRowSize,
		  CostScalar probeRowSize);

  CostScalar scmRowSizeFactor( CostScalar rowSize ,
         ncmRowSizeFactorType rowSizeFactoryType = TUPLES_ROWSIZE_FACTOR);

  // -----------------------------------------------------------------------
  // Use isMDAM = FALSE when using this in single subset
  // Use isMDAM = TRUE when using this in MDAM. In this case
  //              the routine will assume that we are not in an
  //              index join (it should be the case because
  //              code in the early stages should make sure that
  //              we don't attempt MDAM in an index join). This is
  //              not a correctness issue, it is an efficiency issue.
  // -----------------------------------------------------------------------
  virtual void
  categorizeProbes(CostScalar& successfulProbes /* out */
		   ,CostScalar& uniqueSuccProbes /* out */
		   ,CostScalar& duplicateSuccProbes /* out */
		   ,CostScalar& failedProbes /* out */
		   ,CostScalar& uniqueFailedProbes    
		   ,const CostScalar& probes
		   ,const ValueIdSet& preds
		   ,const Histograms& outerHistograms
                   ,const NABoolean isMDAM
                   ,CostScalar * dataRows = NULL
		   ) const;

  // Accesors:

  const CostScalar & getSingleSubsetSize() const
  {
    return singleSubsetSize_;
  }

  const CostScalar getResultSetCardinality() const
  {
    return resultSetCardinality_;
  }

  const CostScalar getIndexLevelsSeeks() const;
  

  NABoolean getInOrderProbesFlag() const
  { return inOrderProbes_; }

  NABoolean getProbesForceSynchronousAccessFlag() const
  { return probesForceSynchronousAccess_; }

  const Context& getContext() const
  {
    return context_;
  }

  // Mutators:

  void setSingleSubsetSize(const CostScalar & singleSubsetSize)
  {
    singleSubsetSize_ = singleSubsetSize;
  }

  void setInOrderProbesFlag(NABoolean probesAreInOrder) 
    { inOrderProbes_ = probesAreInOrder; }

  void
  setProbesForceSynchronousAccessFlag(NABoolean probesForceSynchronousAccess) 
    { probesForceSynchronousAccess_ = probesForceSynchronousAccess; }

  void setNumberOfBlocksToReadPerAccess(const Lng32& blocks) 
  {
    DCMPASSERT(blocks > -1);
    numberOfBlocksToReadPerAccess_ = blocks;
  }

  void setEstRowsAccessed(CostScalar rows)
  {
    estRowsAccessed_ = rows;
  }

  // With overflow checks
  void setNumberOfBlocksToReadPerAccess(const CostScalar& blocks);


  const RelExpr& getRelExpr() const
  {
    return fileScan_;
  }
  
  const FileScan& getFileScan() const
  {
    return fileScan_;
  }
  
  const IndexDesc* getIndexDesc() const
  {
    return fileScan_.getIndexDesc();
  }

  NABoolean isForwardScan() const
  {
    return (NOT fileScan_.getReverseScan());
  }

  NABoolean getMdamFlag() const
  {
    return fileScan_.getMdamFlag();
  }

  const Disjuncts& getDisjuncts() const
  {
    return fileScan_.getDisjuncts();
  }

  NABoolean isMdamForced() const
  {
    return isMdamForced(fileScan_, getContext());
  }

  // Static version, used by useSimpleFileScanOptimizer()
  //
  static NABoolean isMdamForced(const FileScan& fileScan
                                ,const Context& myContext);

  // Static method, used by useSimpleFileScanOptimizer()
  // Determine whether MDAM is Forced ON, Forced OFF, or ENABLED.
  //
  ScanForceWildCard::scanOptionEnum
  static getMdamStatus(const FileScan& fileScan
                                      ,const Context& myContext);

  NABoolean isMdamEnabled() const;

  const ValueIdSet &getExternalInputs() const { return externalInputs_; }

#ifndef NDEBUG
  // for printing debug info:

  void
  printCostObject(const Cost * costPtr) const;
#endif

protected:
  // For scans where we cost both single subset and MDAM scans,
  // this is the number of rows in a single subset scan (before
  // executor predicates are applied).
  //
  // TODO: Figure out generalizations for MultiProbe Scans
  //
  CostScalar singleSubsetSize_;

  // This is the total number of probes for all active partitions.
  // The value is cached in categorizeMultiProbes().
  //
  // For MultiProbe Scans
  //
  CostScalar probes_;

  // This is the number of probes (probes_) that produce some data.
  // The value is cached in categorizeMultiProbes().
  //
  // For MultiProbe Scans
  //
  CostScalar successfulProbes_;

  // This is the number of distinct probes (probes_).  Includes
  // successful and failed probes.  The value is cached in
  // categorizeMultiProbes().
  //
  // For MultiProbe Scans
  //
  CostScalar uniqueProbes_;

  // This is the number of successful probes (successfulProbes_) that
  // are not unique.  duplicateSuccProbes = successfulProbes -
  // uniqueSuccProbes.  The value is cached in
  // categorizeMultiProbes().
  //
  // For MultiProbe Scans
  //
  CostScalar duplicateSuccProbes_;

  CostScalar tuplesProcessed_;

private:

  // The associated FileScan node
  const FileScan& fileScan_;

  // The cardinality of the synthesized statistics for the scan:
  const CostScalar resultSetCardinality_;

  // Estimated number of Dp2 rows accessed:
  CostScalar estRowsAccessed_;

  // The context in which the scan is being optimized:
  const Context &context_;

  // In addition to the scan node's characteristic inputs there
  // may be other inputs that we want to consider for key predicates,
  // such as partition input variables. This is why we have a separate
  // data member for the external inputs.
  ValueIdSet externalInputs_;

  // Estimate of number of blocks that DP2 needs to read
  // per access. This value is passed to DP2 by the executor,
  // DP2 uses it to decide whether it will do read ahead
  // or not.
  // Its value is -1 if uninitialized
  Lng32 numberOfBlocksToReadPerAccess_;

  // Indicates if the probes are completely in order, or partially
  // in order, but cache is big enough so that we get the same 
  // benefit as if the probes were completely in order.
  NABoolean inOrderProbes_;

  // Indicates if the probes are completely in order accross partitions,
  // and so the access to multiple partitions of the inner table will
  // be serialized.
  NABoolean probesForceSynchronousAccess_;

}; // class ScanOptimizer


// -----------------------------------------------------------------------
// The class fileScanOptimizer performs several actions:
// 1.- Decides the access method for the scan
// 2.- Computes the cost for the access method
// 3.- Builds a key for the access method and attaches
//     that key to the scan.
// -----------------------------------------------------------------------
class MDAMCostWA;
class MDAMOptimalDisjunctPrefixWA;
class NewMDAMCostWA;
class NewMDAMOptimalDisjunctPrefixWA;
class FileScanOptimizer : public ScanOptimizer
{
  friend class MDAMCostWA;
  friend class MDAMOptimalDisjunctPrefixWA;
  friend class NewMDAMCostWA;
  friend class NewMDAMOptimalDisjunctPrefixWA;
  friend class MdamTrace;
public:

  FileScanOptimizer(const FileScan& associatedFileScan
		    ,const CostScalar& resultSetCardinality
                    ,const Context& myContext
		    ,const ValueIdSet &externalInputs) :
    ScanOptimizer(associatedFileScan
                  ,resultSetCardinality
                  ,myContext
                  ,externalInputs)
    ,rawInnerHistograms_(*(associatedFileScan.getIndexDesc()),
                         associatedFileScan.getIndexDesc()->getIndexKey().entries())
    {
    }

  virtual ~FileScanOptimizer();

  // -----------------------------------------------------------------------
  // Accessors:
  // -----------------------------------------------------------------------

  const IndexDescHistograms& getRawInnerHistograms() const
  { return rawInnerHistograms_; }

  // -----------------------------------------------------------------------
  // Mutators:
  // -----------------------------------------------------------------------

  // -----------------------------------------------------------------------
  // optimize performs several actions:
  // 1.- Picks the best access method (single subset or MDAM)
  // 2.- Creates the appropiate key
  // 3.- Returns the appropiate key and makes sure only one key
  //     gets generated (Single subset key XOR MdamKey)
  // 4.- Computes and returns the cost of the chosen access method.
  // -----------------------------------------------------------------------
  virtual Cost * optimize(SearchKey*& searchKeyPtr   // out
			  ,MdamKey*&  mdamKeyPtr     // out
			  );

private:
  
   // Pass the join histograms when available
  void computeNumberOfBlocksToReadPerAccess(const Cost& scanCost,
                                            NABoolean &isMDAM,
                                            CostScalar numKBytes);

  void
  computeIOForFullCacheBenefit(
       CostScalar& seeks /* out */
       ,CostScalar& sequential_io /* out */
       ,const CostScalar& beginBlocksLowerBound
       ,const CostScalar& totalBlocksLowerBound
       ,const CostScalar& indexBlocks) const;

  void
  computeSeekForDp2ReadAheadAndProbeOrder(
        CostScalar& seekComputedWithDp2ReadAhead,
	const CostScalar& finalRows,
	const CostScalar& uniqueProbes, 
        const CostScalar& beginBlocksLowerBound, 
        const CostScalar& totalBlocksLowerBound, 
        const CostScalar& innerBlocksUpperBound,
        const CostScalar& dp2CacheSize,
        const NABoolean inOrderProbes) const;

  void
  computeIOForRandomCase(
       CostScalar& seeks /* out */
       ,CostScalar& sequential_io /* out */
       ,const CostScalar& blksPerSuccProbe
       ,const CostScalar& beginBlocksLowerBound
       ,const CostScalar& totalBlocksLowerBound
       ,const CostScalar& successfulProbes
       ,const CostScalar& failedProbes
       ,const CostScalar& probes) const;

  void computeIOForFullTableScan(
     CostScalar& dataRows /* out */
     ,CostScalar& seeks /* out */
     ,CostScalar& sequential_io /* out */
     ,const CostScalar& probes) const;

  void computeCostVectors(
       SimpleCostVector& firstRow /* out */
       ,SimpleCostVector& lastRow /* out */
       ,CostScalar& seqKBytesPerScan /* out */
       ,const CostScalar& totalRows
       ,const CostScalar& subsetRequests
       ,const CostScalar& successfulSubsetRequests
       ,const CostScalar& seeks
       ,const CostScalar& sequential_io
       ,const ValueIdSet& keyPredicates
       ,const ValueIdSet& exePreds
       ,const CostScalar& incomingProbes // probes incoming to the operator
       ) const;

  void computeCostVectorsForMultipleSubset(
       SimpleCostVector& firstRow /* out */
       ,SimpleCostVector& lastRow /* out */
       ,CostScalar& seqKBytesPerScan /* out */
       ,const CostScalar& totalRows
       ,const CostScalar& subsetRequests
       ,const CostScalar& successfulSubsetRequests
       ,const CostScalar& seeks
       ,const CostScalar& sequential_io
       ,const ValueIdSet& keyPredicates
       ,const ValueIdSet& exePreds
       ,const CostScalar& incomingProbes // probes incoming to the operator
       ,const CostScalar& mdamNetPredCnt // the sum of preds in all disjuncts
       ) const;
    

  // -----------------------------------------------------------------------
  // Computes the cost for non-MDAM case. It will return NULL if,
  // while computing the cost, the predicate expression
  // for any key column contains a CONFLICT and the
  // breakOnConflictFlag is TRUE. This last thing
  // is needed because MDAM knows how to resolve a conflict,
  // however, it MUST be set to false when non-mdam is being
  // forced.
  // -----------------------------------------------------------------------
  Cost * computeCostForSingleSubset(SearchKey& searchKey /* in/out */
				    ,const NABoolean& breakOnConflict
				    ,CostScalar & numKBytes
                                   );
				    
  // -----------------------------------------------------------------------
  // Computes the cost for MDAM. It will return NULL if,
  // while computing the cost, the cost exceeds or equals the
  // cost bound provided as input.
  // -----------------------------------------------------------------------
  Cost * computeCostForMultipleSubset(MdamKey* mdamKeyPtr /* in/out */
                                      ,const Cost* costBoundPtr
                                      ,NABoolean mdamForced
				      ,CostScalar & numKBytes
                                      ,NABoolean checkExePreds
                                      ,NABoolean mdanTypeIsCommon 
                                      ,MdamKey*& sharedMdamKeyPtr
				      );
  Cost * oldComputeCostForMultipleSubset(MdamKey* mdamKeyPtr /* in/out */
                                      ,const Cost* costBoundPtr
                                      ,NABoolean mdamForced
				      ,CostScalar & numKBytes
                                      ,NABoolean checkExePreds
                                      ,NABoolean mdanTypeIsCommon 
                                      ,MdamKey*& sharedMdamKeyPtr
				      );

  Cost* newComputeCostForMultipleSubset
    ( MdamKey* mdamKeyPtr,
    const Cost * costBoundPtr,
    NABoolean mdamForced,
    CostScalar & numKBytes,
    ValueIdSet exePreds,
    NABoolean checkExePreds,
    NABoolean mdamTypeIsCommon,
    MdamKey *&sharedMdamKeyPtr );
  
  Cost* scmComputeCostForSingleSubset();

  Cost* scmRewrittenComputeCostForMultipleSubset
    ( MdamKey* mdamKeyPtr,
    const Cost * costBoundPtr,
    NABoolean mdamForced,
    CostScalar & numKBytes,
    ValueIdSet exePreds,
    NABoolean checkExePreds,
    MdamKey *&sharedMdamKeyPtr );

  Cost* scmComputeCostForMultipleSubset
    ( MdamKey* mdamKeyPtr,
    const Cost * costBoundPtr,
    NABoolean mdamForced,
    CostScalar & numKBytes,
    ValueIdSet exePreds,
    NABoolean checkExePreds,
    NABoolean mdamTypeIsCommon,
    MdamKey *&sharedMdamKeyPtr );

  Cost* scmComputeMDAMCostForHbase
    ( CostScalar& totalRows
      ,CostScalar& seeks
      ,CostScalar& sequential_io
      ,CostScalar& incomingProbes );
  
#ifndef NDEBUG
  void runMdamTests
   ( const MdamKey* mdamKeyPtr,
     const Cost * costBoundPtr,
     NABoolean mdamForced,
     ValueIdSet exePreds,
     NABoolean checkExePreds,
     NABoolean mdamTypeIsCommon 
     );
#endif

  NABoolean isMultipleProbes() const;
  const ScanForceWildCard* findScanForceWildCard() const;
  CollIndex computeLastKeyColumnOfDisjunct(const ColumnOrderList & keyPredsByCol);
  const CostScalar getIncomingProbes() const;
// return true if has resuable shared basic cost for this mdam
  NABoolean getSharedCost(FileScanBasicCost * &fileScanBasicCostPtr /*out, never NULL*/
    ,NABoolean & hasLostBefore /*out*/
    ,SimpleCostVector * &disjunctsFRPtr /*out never NULL*/
    ,SimpleCostVector * &disjunctsLRPtr /*out never NULL*/
    ,CostScalar & numKBytes /*out*/
    ,MdamKey* & sharedMdamKeyPtr /*out*/
    ,NABoolean mdamTypeIsCommon /*in*/);
  // -----------------------------------------------------------------------
  // Helper methods:
  // -----------------------------------------------------------------------
  // -----------------------------------------------------------------------
  // Returns TRUE if the cost resulting from firstRow and lastRow
  // exceeds or equals the given cost bound
  // -----------------------------------------------------------------------
  NABoolean exceedsBound(const Cost *costBoundPtr
			,const SimpleCostVector& firstRow
			,const SimpleCostVector& lastRow
                        ) const;

 
  NABoolean hasTooManyDisjuncts() const;

  NABoolean isMDAMFeasibleForHBase(const IndexDesc* idesc, ValueIdSet& preds);
 
private:

  IndexDescHistograms rawInnerHistograms_;

}; // class FileScanOptimizer

// -----------------------------------------------------------------------
// Class FileScanBasicCost
// 1.- The object of this class contains 3 pairs (for the first and
//     last row goals) of simple cost vectors depending on the costing
//     type: single subset, MDAM common, MDAM disjuncts. The purpose
//     of this class is to reuse these simple cost vectors whenever
//     possible like synchronous and asynchronous access to the same
//     table. Every index descriptor will contain the list of such 
//     objects. If context of current FileScanOptimizer is similar 
//     to one in the list then corresponding firstRow and lastRow cost 
//     vectors will be used to compute CostObject. If not, a new object 
//     will be added to the list and its simple cost vectors will be 
//     computed using current context logical and physical properties.
// 2.- Provides mutator type acces to simple cost vectors and
//     noExePreds flag that also has to be reused.
// 3.- hasSameBasicProperties() function checks if context that created
//     this object for this IndexDesc and the context passed as a 
//     parameter have the same basic properties therefore - the same 
//     simple cost vectors
// -----------------------------------------------------------------------

class FileScanBasicCost : public NABasicObject
{
public:

  FileScanBasicCost(const Context * currentContext): 
     basicCostContext_(currentContext), 
     basicFRCostSingleSubset_(NULL),
     basicLRCostSingleSubset_(NULL),
     basicFRCostMdamCommon_(NULL),
     basicLRCostMdamCommon_(NULL),
     basicFRCostMdamDisjuncts_(NULL),
     basicLRCostMdamDisjuncts_(NULL),
     mdamCommonKeyPtr_(NULL),
     mdamDisjunctsKeyPtr_(NULL),
     mdamCommonLost_(FALSE),
     mdamDisjunctsLost_(FALSE)

        {CMPASSERT(basicCostContext_ != NULL);}
  
  FileScanBasicCost(const FileScanBasicCost & other):
     basicCostContext_(other.basicCostContext_),
     basicFRCostSingleSubset_(other.basicFRCostSingleSubset_),
     basicLRCostSingleSubset_(other.basicLRCostSingleSubset_),
     basicFRCostMdamCommon_(other.basicFRCostMdamCommon_),
     basicLRCostMdamCommon_(other.basicLRCostMdamCommon_),
     basicFRCostMdamDisjuncts_(other.basicFRCostMdamDisjuncts_),
     basicLRCostMdamDisjuncts_(other.basicLRCostMdamDisjuncts_),
     mdamCommonKeyPtr_(other.getMdamKeyPtr(TRUE)),
     mdamDisjunctsKeyPtr_(other.getMdamKeyPtr(FALSE)),
     mdamCommonLost_(other.mdamCommonLost_),
     mdamDisjunctsLost_(other.mdamDisjunctsLost_)

	{CMPASSERT(basicCostContext_ != NULL);}
  
  ~FileScanBasicCost() {};

  SimpleCostVector & 
    getFRBasicCostSingleSubset() {return basicFRCostSingleSubset_;}
  
  SimpleCostVector &
    getLRBasicCostSingleSubset() {return basicLRCostSingleSubset_;}
  
  SimpleCostVector & 
    getFRBasicCostMdamCommon() {return basicFRCostMdamCommon_;}
  
  SimpleCostVector &
    getLRBasicCostMdamCommon() {return basicLRCostMdamCommon_;}
  
  SimpleCostVector & 
    getFRBasicCostMdamDisjuncts() {return basicFRCostMdamDisjuncts_;}
  
  SimpleCostVector &
    getLRBasicCostMdamDisjuncts() {return basicLRCostMdamDisjuncts_;}

  MdamKey * getMdamKeyPtr(NABoolean mdamType) const 
   {return (mdamType ? mdamCommonKeyPtr_ : mdamDisjunctsKeyPtr_);}

  void setMdamKeyPtr(MdamKey *mdamKeyPtr, NABoolean mdamType) 
   { mdamType ? mdamCommonKeyPtr_ = mdamKeyPtr 
              : mdamDisjunctsKeyPtr_ = mdamKeyPtr;}

  NABoolean hasSameBasicProperties(const Context & currentContext) const; 

  // To set the kbytes for singlesubset, common & disjunt

  void setSingleSubsetNumKBytes(CostScalar numKBytes)
  { singleSubsetNumKBytes = numKBytes; }

  void setEstRowsAccessed(CostScalar estRows)
  { estRowsAccessed_ = estRows; }

  void setMdamCommonNumKBytes(CostScalar numKBytes)
  { mdamCommonNumKBytes = numKBytes; }

  void setMdamDisjunctsNumKBytes(CostScalar numKBytes)
  { mdamDisjunctsNumKBytes = numKBytes; }

  // To get the kbytes from singlesubset, common * disjunt

  CostScalar getSingleSubsetNumKBytes() 
  { return singleSubsetNumKBytes; }

  CostScalar getEstRowsAccessed()
  { return estRowsAccessed_; }

  CostScalar getMdamCommonNumKBytes()
  { return mdamCommonNumKBytes; }

  CostScalar getMdamDisjunctsNumKBytes()
  { return mdamDisjunctsNumKBytes; }

  NABoolean hasMdamCommonLost() const;
  NABoolean hasMdamDisjunctsLost() const;
  void setMdamCommonLost(NABoolean);
  void setMdamDisjunctsLost(NABoolean);

  private:

     FileScanBasicCost(): 
        basicCostContext_(NULL), 
        basicFRCostSingleSubset_(NULL),
        basicLRCostSingleSubset_(NULL),
        basicFRCostMdamCommon_(NULL),
        basicLRCostMdamCommon_(NULL),
        basicFRCostMdamDisjuncts_(NULL),
        basicLRCostMdamDisjuncts_(NULL),
        mdamCommonKeyPtr_(NULL),
        mdamDisjunctsKeyPtr_(NULL),
	singleSubsetNumKBytes(csZero),
	estRowsAccessed_(csZero),
	mdamCommonNumKBytes(csZero),
	mdamDisjunctsNumKBytes(csZero),
	mdamCommonLost_(FALSE),
	mdamDisjunctsLost_(FALSE)

	{CMPASSERT(basicCostContext_ != NULL);}

     const Context * basicCostContext_;
     SimpleCostVector basicFRCostSingleSubset_;
     SimpleCostVector basicLRCostSingleSubset_;
     SimpleCostVector basicFRCostMdamCommon_;
     SimpleCostVector basicLRCostMdamCommon_;
     SimpleCostVector basicFRCostMdamDisjuncts_;
     SimpleCostVector basicLRCostMdamDisjuncts_;
     MdamKey * mdamCommonKeyPtr_;
     MdamKey * mdamDisjunctsKeyPtr_;
     NABoolean mdamCommonLost_;
     NABoolean mdamDisjunctsLost_;

     // Three costScalars had to been defined to preserve
     // Kbytes in order to calculate the blocksperaccess as
     // the simple Cost Vector has been reduced to have only
     // total time for IO.
     
     CostScalar singleSubsetNumKBytes;
     CostScalar estRowsAccessed_;
     CostScalar mdamCommonNumKBytes;
     CostScalar mdamDisjunctsNumKBytes;
};


// -----------------------------------------------------------------------
// List of basic cost objects. Every index descriptor will have this
// list to provide acces to individual basic cost objects with th help
// of standard collection tools like insert(), entries() and index 
// access( overloaded [])
// -----------------------------------------------------------------------

class FileScanCostList : public LIST (FileScanBasicCost *) 
{
  public:
     FileScanCostList(NAMemory *h) : 
	LIST (FileScanBasicCost *) (h) {};
};


#endif
// eof 

