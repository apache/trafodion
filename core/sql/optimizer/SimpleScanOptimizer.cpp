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
 *****************************************************************************
 *
 * File:         SimpleScanOptimizer.cpp
 * Description:  Simplified Costing for Scan operators
 * Created:      04/2003
 * Language:     C++
 *
 *****************************************************************************
 */

#include "SimpleScanOptimizer.h"
#include "AllRelExpr.h"

#ifdef DEBUG
#define SFSOWARNING(x) fprintf(stdout, "SimpleFileScan optimizer warning: %s\n", x);
#else
#define SFSOWARNING(x)
#endif

// Get any extra key predicates from the partitioning function.  The
// partitioning function will provide extra key predicates when it
// represents logical sub-partitioning.
//
const ValueIdSet & 
SimpleFileScanOptimizer::getPartitioningKeyPredicates() const
{
  const LogPhysPartitioningFunction *logPhysPartFunc =
    getContext().getPlan()->getPhysicalProperty()->getPartitioningFunction()->
    castToLogPhysPartitioningFunction();
  CMPASSERT(logPhysPartFunc);

  return logPhysPartFunc->getPartitioningKeyPredicates();
}

// getNumBlocksForRows()
// Compute the number of blocks needed to hold the given number of
// rows for this scan.  Assume that the rows are dense (no
// fragmentation or slack).  Assume that the subset starts on a block
// boundary resulting in possibly one partial block at the end of the
// subset (This is why there is a 'getCeiling' included).  This may
// underestimate the number of blocks by 1, if the subset starts
// mid-block.
//
CostScalar
SimpleFileScanOptimizer::getNumBlocksForRows(const CostScalar &numRows) const
{

  // RMW should a '+1' be added to this to account for partial blocks at
  // both ends of the subset?
  //
  return (numRows/getEstimatedRecordsPerBlock()).getCeiling();

}

// getResultSetCardinalityPerScan() 
// Compute the estimated number of rows per scan (active partition),
// based on the result set cardinality computed outside of the Scan
// Optimizer and the number of active partitions for this Scan.
//
CostScalar 
SimpleFileScanOptimizer::getResultSetCardinalityPerScan() const
{
  return (getResultSetCardinality()/getEstNumActivePartitionsAtRuntime()).getCeiling();
}

// getRowsInDP2Buffer()
// Compute the number of rows of this Scan that will fit in a DP2 Buffer.
//
CostScalar
SimpleFileScanOptimizer::getRowsInDP2Buffer() const
{
  // Default size of a DP2 Message Buffer (in KBytes).
  //
  const CostScalar dp2MessageBufferSizeInKb 
    (CostPrimitives::getBasicCostFactor(DP2_MESSAGE_BUFFER_SIZE));

  // rows that fit in dp2 buffer:
  return
    (dp2MessageBufferSizeInKb / getRecordSizeInKb()).getFloor();
}

SimpleFileScanOptimizer::SimpleFileScanOptimizer(
                        const FileScan& assocFileScan
                        ,const CostScalar& resultSetCardinality
                        ,const Context& myContext
                        ,const ValueIdSet &externalInputs) :
  ScanOptimizer(assocFileScan
                ,resultSetCardinality
                ,myContext
                ,externalInputs),

  // Cache some values that are used often
  //
  recordSizeInKb_(assocFileScan.getIndexDesc()->getRecordSizeInKb()),
  blockSizeInKb_(assocFileScan.getIndexDesc()->getBlockSizeInKb()),
  estimatedRecordsPerBlock_(assocFileScan.getIndexDesc()->
                            getEstimatedRecordsPerBlock()),

  // Will be calculated during computeSingleSubsetSize()
  //
  singleSubsetSize_(0),
  singleSubsetPreds_(),

  // Will be constructed during constructSearchKey() 
  // This search key is not owned by this object and will therefore
  // not be destroyed when the destructor of this object is called.
  // This search key will be returned to the caller
  //
  searchKey_(NULL),

  // Will be calculated during categorizeMultiProbes().
  //
  // For MultiProbe Scans
  //
  failedProbes_(0),
  indexBlocksLowerBound_(0),
  blksPerSuccProbe_(0),
  keyPrefixUEC_(1),
  partialOrderProbes_(FALSE),
  dataRows_(0),
  effectiveTotalRowCount_(0),
  totalRowCount_(0)
{
}

SimpleFileScanOptimizer::~SimpleFileScanOptimizer()
{
  // Do not destroy the SearchKey pointed to by the searchKey_
  // pointer, it is being returned to the caller.
}

// Optimize
// Construct a Search Key (MDAM or Single Subset)
// Produce a Cost object for this scan and context.
// Compute the number of blocks to read per access. 
//
Cost *
SimpleFileScanOptimizer::optimize(SearchKey*& searchKeyPtr   /* out */
                                  , MdamKey*& mdamKeyPtr     /* out */
                                  )
{
  DCMPASSERT(searchKeyPtr == NULL);
  DCMPASSERT(mdamKeyPtr == NULL);

  // For now, the simple scan optimizer only handles single-subsets
  // (not MDAM).  When MDAM is supported, will have to add logic here
  // to detect the need to do MDAM costing.

  // Construct the single subset search key.
  //
  searchKeyPtr = constructSearchKey();

  // Compute the size of this single subset and cache the size and the
  // single subset predicates in local datamembers.
  //
  computeSingleSubsetSize(); 
  
  // Compute the cost for this single subset
  if ( CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON )
    return scmComputeCostForSingleSubset();
  else
    return computeCostForSingleSubset();

}  // SimpleFileScanOptimizer::optimize()


// Does the partitioning function associated with this scan represent
// logical sub-partitioning.
//
NABoolean
SimpleFileScanOptimizer::isLogicalSubPartitioned() const
{
  const LogPhysPartitioningFunction *logPhysPartFunc =
    getContext().getPlan()->getPhysicalProperty()->getPartitioningFunction()->
    castToLogPhysPartitioningFunction();
  
  if ( logPhysPartFunc != NULL )
  {    
    LogPhysPartitioningFunction::logPartType
      logPartType = logPhysPartFunc->getLogPartType();

    if (logPartType == LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING ||
        logPartType == LogPhysPartitioningFunction::HORIZONTAL_PARTITION_SLICING) {
      return TRUE;
    }
  }
  return FALSE;
}


// Construct the Search Key (MDAM or Single Subset)
// (for now just the Single subset is considered)
// Return pointer to search key and cache in local datamember.
//
SearchKey *
SimpleFileScanOptimizer::constructSearchKey()
{
  // We do not need to incude the flatten version of RANGE SPEC predicates 
  // to the exePreds because SearchKey is capable of handling such predicates.
  // In addition, we do not want to confuse the SearchKey::ini() method 
  // about what are the original predicates and what are derived predicates, 
  // since combing these two forms will confuse SearchKey::makeHBaseSearchKey() 
  // and cause the missing of the end keys (JIRA1449).
  ValueIdSet exePreds(getRelExpr().getSelectionPred());

  const Disjuncts *curDisjuncts = &(getDisjuncts());

  if(isLogicalSubPartitioned()) {

    exePreds += getPartitioningKeyPredicates();

    curDisjuncts = new(CmpCommon::statementHeap()) MaterialDisjuncts(exePreds);
  }

  ValueIdSet nonKeyColumnSet;
  getIndexDesc()->getNonKeyColumnSet(nonKeyColumnSet);
  
  SearchKey *searchKeyPtr =  new(CmpCommon::statementHeap())
    SearchKey(getIndexDesc()->getIndexKey()
              ,getIndexDesc()->getOrderOfKeyValues()
              ,getExternalInputs()
              ,isForwardScan()
              ,exePreds
              ,*curDisjuncts
              ,nonKeyColumnSet
              ,getIndexDesc()
              );

  searchKey_ = searchKeyPtr;
  return searchKeyPtr;
} // SimpleFileScanOptimizer::constructSearchKey()

// Compute the single subset size and predicates and cache the values in
// local datamembers
//
void
SimpleFileScanOptimizer::computeSingleSubsetSize()
{
  singleSubsetSize_ = 0;
  singleSubsetPreds_ = getSearchKey()->getKeyPredicates();

  if (getSearchKey()->isUnique()) {

    // If the searchKey is unique, then by definition, the size is 1
    //
    singleSubsetSize_ = 1;

  } else if(getSearchKey()->getExecutorPredicates().entries() == 0) {

    // If there are no executor predicates, then the single subset
    // will not be reduced any further.  Therefore, the single subset
    // represents the result of this scan.  We have already estimated
    // the result set size, so use it here.
    //
    singleSubsetSize_ = getResultSetCardinality();

  } else {

    // Otherwise, we must compute the single subset size.
    //
    ColumnOrderList keyPredsByCol(getIndexDesc()->getIndexKey());

    getSearchKey()->getKeyPredicatesByColumn(keyPredsByCol);

    CollIndex singleSubsetPrefixColumn;
    NABoolean itIsSingleSubset =
      keyPredsByCol.getSingleSubsetOrder(singleSubsetPrefixColumn);

    if(getSearchKey()->getKeyPredicates().entries() > 0) {
      
      singleSubsetPreds_.clear();

      for (CollIndex pred = 0; pred <= singleSubsetPrefixColumn; pred++)
        {
          const ValueIdSet *preds = keyPredsByCol[pred];
          if(preds) {
             singleSubsetPreds_ += *preds;
          }
        }
      
      IndexDescHistograms innerHistograms(*getIndexDesc(),
                                          singleSubsetPrefixColumn + 1);

      const SelectivityHint * selHint = getFileScan().getSelectivityHint();
      const CardinalityHint * cardHint = getFileScan().getCardinalityHint();

      // Exclude the added computed predicates since they do not contribute
      // to the cardinality. 
      ValueIdSet predicatesToUse(singleSubsetPreds_);
      predicatesToUse -= getFileScan().getComputedPredicates();

      innerHistograms.applyPredicates(predicatesToUse, getRelExpr(), selHint, cardHint, REL_SCAN);

      // Now, compute the number of rows:
      singleSubsetSize_ = innerHistograms.getRowCount();

    } else {
      IndexDescHistograms innerHistograms(*getIndexDesc(),
                                          singleSubsetPrefixColumn + 1);
      
      // A full table scan.
      singleSubsetSize_ = innerHistograms.getRowCount();
    }
  }
} // SimpleFileScanOptimizer::computeSingleSubsetSize()

// computeNumberOfBlocksToReadPerAccess()
// Estimate the number of blocks that DP2 needs to read per access for
// this Scan. This value is captured by the FileScan Node and passed
// to DP2 by the executor, DP2 uses it to decide whether it will do
// read ahead or not.
//
// Side-Affects - sets the numberOfBlocksToReadPerAccess_ data member
// of ScanOptimizer.  
//
void
SimpleFileScanOptimizer::computeNumberOfBlocksToReadPerAccess(
            FileScanBasicCost *basicCostPtr)
{

  const CostScalar numKBytes(basicCostPtr->getSingleSubsetNumKBytes());

  // KB for index blocks plus data blocks for one partition for all
  // probes:
  CostScalar blocksToRead 
    ((numKBytes/getBlockSizeInKb()).getCeiling());

  Lng32 indexBlocks = MAXOF(getIndexDesc()->getIndexLevels(),1);

  // Substract index blocks. minCsOne is another sanity check.
  blocksToRead =(blocksToRead - indexBlocks).minCsOne();

  // Store in ScanOptimizer object.  FileScan will later grab this
  // value.
  //
  setNumberOfBlocksToReadPerAccess(blocksToRead);

} // SimpleFileScanOptimizer::computeNumberOfBlocksToReadPerAccess(...)

// computeCostForSingleSubset()
// Compute the Cost for this single subset Scan.
//
// Attempts to find an existing basic cost object which can be reused.
//
// Computes or reuses the first row and last row cost
// vectors. (IOTime, CPUTime, SeekIOTime, IdleTime, numProbes)
//
// OUTPUTS:
// Return - A NON-NULL Cost* representing the Cost for this scan node
//
// Side-Affects - computes and sets the numberOfBlocksToReadPerAccess_
// data member of ScanOptimizer.  This value will be captured by the
// FileScan node and passed to DP2 by the executor, DP2 uses it to
// decide whether it will do read ahead or not.
//
Cost*
SimpleFileScanOptimizer::computeCostForSingleSubset()
{

  FileScanBasicCost *basicCostPtr;

  // Either get a shared basic cost object or create a new empty one.
  //
  NABoolean sharedCost = getSharedCostObject(basicCostPtr);

  // Determine if this scan is receiving multiple probes.  If so,
  // use the MultiProbe Scan Optimizer methods.  Other wise use the
  // single probe methods.
  //
  CostScalar repeatCount =  
      getContext().getPlan()->getPhysicalProperty()->
      getDP2CostThatDependsOnSPP()->getRepeatCountForOperatorsInDP2();

  NABoolean multiProbeScan =  repeatCount.isGreaterThanOne() OR
      (getContext().getInputLogProp()->getColStats().entries() > 0);

  if(!sharedCost) {

    // If the basic cost object is newly created (not shared), we must
    // recompute the cost vectors. (IOTime, CPUTime, SeekIOTime,
    // IdleTime, numProbes)
    //

    if ( multiProbeScan ) 
    {
      computeCostVectorsMultiProbes(basicCostPtr);
    } else {
      computeCostVectors(basicCostPtr);
    }
  }
  else
  { // Determine whether probes order matches the scan's clustering
    // order. For the first time when BasicCost is computed the order
    // requirement could be different. There fore in case we reuse
    // BasicCost we need to doublecheck the need for synchronous access.
    // Synchronous access flag and BasicCost are orthogonal and we 
    // shouldn't save this flag with BasicCost.
    // We need to do this only for multiProbeScan. For single probe
    // (in case of merge joins) synchronous access will be
    // forced implicitly by using the number of partition of logical
    // part.function. (see the logic in computeCostObject() ).

    if ( multiProbeScan )
    {
      const InputPhysicalProperty* ippForMe =
        getContext().getInputPhysicalProperty();

      NABoolean partiallyInOrderOK = TRUE;
      NABoolean probesForceSynchronousAccess = FALSE;

      if ((ippForMe != NULL) AND
           SimpleFileScanOptimizer::ordersMatch(ippForMe,
                  getIndexDesc(),
                  &(getIndexDesc()->getOrderOfKeyValues()),
                  getRelExpr().getGroupAttr()->getCharacteristicInputs(),
                  partiallyInOrderOK,
                  probesForceSynchronousAccess))
      {
        setInOrderProbesFlag(TRUE);
        setProbesForceSynchronousAccessFlag(probesForceSynchronousAccess);
      }
      else if ((ippForMe != NULL) AND
         (ippForMe->getAssumeSortedForCosting()) AND
         (!(ippForMe->getExplodedOcbJoinForCosting())) AND
         (ippForMe->getNjOuterOrderPartFuncForNonUpdates() == NULL))
      {
         // assumeSortedForCosting_ flag is set for two cases:
         // case 1: when input is rowset.
         // case 2: when left child partFunc njOuterPartFuncForNonUpdates_ 
         //         is passed for NJ plan 0. This is only for cost estimation 
         //         of exchange operator and not scan operator.
         // So we should come here only for case1. 
         // To avoid case2, we check njOuterPartFuncForNonUpdates_ for NULL.
        setInOrderProbesFlag(TRUE);
      }
      else
      {
        setInOrderProbesFlag(FALSE);
      }
    }
  }
  
  SimpleCostVector &firstRow = basicCostPtr->getFRBasicCostSingleSubset();
  SimpleCostVector &lastRow = basicCostPtr->getLRBasicCostSingleSubset();

  // Compute the Cost object for the physical scan from the simple
  // cost vectors for the first and last row.
  //

  if (CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) 
  {
     // compute multiplicative factor = probesAtBusiestStream/ProbesPerScan_
     // Multiply the last row cost by the factor
     // Note that the last row cost is per Partition
     CostScalar probesAtBusyStream
                           =  getContext().getPlan()->getPhysicalProperty()->
                               getDP2CostThatDependsOnSPP()->
                               getProbesAtBusiestStream();

     CostScalar probesPerScan = lastRow.getNumProbes();
     // probes must be > 0  if not assert.
     DCMPASSERT(probesPerScan.isGreaterThanZero());
    
     probesPerScan = MAXOF(probesPerScan, 1);
    
     CostScalar multiplicativeFactor = 
               (probesAtBusyStream/probesPerScan).minCsOne();

     lastRow.setCPUTime(lastRow.getCPUTime() * multiplicativeFactor);
     lastRow.setIOTime(lastRow.getIOTime() * multiplicativeFactor);
  }
// OCB Cost Adjustment for KEYCOLS_NOT_COVERED 
  CostScalar ocbAdjFactor = (ActiveSchemaDB()->getDefaults())\
                                  .getAsDouble(OCB_COST_ADJSTFCTR);  
  DP2CostDataThatDependsOnSPP *dp2CostInfo =
     (DP2CostDataThatDependsOnSPP *)(getContext().getPlan()->getPhysicalProperty())-> getDP2CostThatDependsOnSPP();
  
  SimpleCostVector lastRowC = lastRow;
  lastRowC.setCPUTime(lastRowC.getCPUTime() * ocbAdjFactor);
  lastRowC.setIOTime(lastRowC.getIOTime() * ocbAdjFactor);
  if( ( multiProbeScan ) && ( CmpCommon::getDefault(COMP_BOOL_53) == DF_ON ) 
   && ( getContext().getReqdPhysicalProperty()->getOcbEnabledCostingRequirement() ) 
   && (dp2CostInfo != NULL) 
   && (isLowerBound(firstRow,lastRowC))) {
       switch(dp2CostInfo->getRepeatCountState())
      {
        case DP2CostDataThatDependsOnSPP::KEYCOLS_NOT_COVERED:
        lastRow.setCPUTime(lastRowC.getCPUTime());
        lastRow.setIOTime(lastRowC.getIOTime());
		break;

		default:
        break;
      }
   } 

  Cost *costPtr = computeCostObject(firstRow, lastRow);
  DCMPASSERT(costPtr);

  // Estimate the number of blocks that DP2 needs to read per access for
  // this Scan.
  computeNumberOfBlocksToReadPerAccess(basicCostPtr);
  // Store estimated dp2 rows accessed, this will be used in generator.
  setEstRowsAccessed(basicCostPtr->getEstRowsAccessed());

  return costPtr;

} // SimpleFileScanOptimizer::computeCostForSingleSubset()

// getSharedCostObject() 
// Determine if a shareable BasicCost object is available and usable.
//
// If a usable shared basic cost object is available, return it as is.
//
// If an unusable shared basic cost object is available, clear it and
// return it as a nonshared basic cost object.
//
// If a shared basic cost object is not available, create a new one
// and return it as a nonshared basic cost object.
//
// OUTPUTS: 
// Return - NABoolean - TRUE if a usable shared BasicCost object was available.
//                      FALSE if a nonusable or new BasicCost object.
// *&basicCostPtr - set to the BasicCost object (shared or new)
//
NABoolean
SimpleFileScanOptimizer::getSharedCostObject(FileScanBasicCost *&basicCostPtr)
{

  NABoolean sharedCostFound = FALSE;
  
  // Look for an appropriate shared BasicCost object, if none is
  // found, create a new empty one.
  //
  basicCostPtr = shareBasicCost(sharedCostFound);

  SimpleCostVector &firstRow = basicCostPtr->getFRBasicCostSingleSubset();

  SimpleCostVector &lastRow = basicCostPtr->getLRBasicCostSingleSubset();

  // If a share BasicCost object was found ..
  //
  if (sharedCostFound) {

    // If it is shared and usable, return it as a shared BasicCost
    // object.
    //
    if (firstRow.getCPUTime() > csZero AND
        lastRow.getCPUTime() > csZero AND
        CURRSTMT_OPTDEFAULTS->reuseBasicCost() )
      {
        return TRUE;
      }
    else
      // Otherwise, clear it so that it can be reused as an empty
      // nonshared BasicCost object.
      //
      {
        firstRow.reset();
        lastRow.reset();
        return FALSE;
      }
  }

  // Otherwise, a new BasicCost object was created, return it as a
  // nonshared BasicCost object.
  //
  return FALSE;
}   // SimpleFileScanOptimizer::getSharedCostObject()

// SimpleFileScanOptimizer::estimateNumberOfSeeksPerScan() 
// Estimate the of seeks required for this single subset per scan
// (partition).  Include the seeks required for accessing the index
// blocks.
//
// Need to Seek to every index level except the root, plus need
// to seek to first data block.
//
// Even if there are no rows expected in the result set, the
// seeks still need to be performed.
//
// Do we need to seek twice this amount, once for the begin key
// and once for the end key??????
// This is a very good point. From DP2 it is known now that instead
// of using IOBlocksToReadPerAccess as ScanOptimizer presumes
// DP2 ignores it end use end key to search for the end of data blocks
//
// Returns: the number of seeks required for a single subset access to
// a partition (this is equal to the number of index levels plus sone
// extra in case of logical subpartitioning.).
//
CostScalar
SimpleFileScanOptimizer::estimateNumberOfSeeksPerScan(const CostScalar &seqKB) const
{
  // indexLevels is the max index levels of all partitions.
  // Assume the worst case
  //
  Lng32 indexBlocks = MAXOF(getIndexDesc()->getIndexLevels(),1);

  CostScalar seeks(indexBlocks);

  // We need an adjustment to the number of seeks in case of large
  // sequential scan. Usually it is not possible to read the whole 
  // partition without interruption from system or concurrent DP2
  // operations. Previously this adjustment was done only for the
  // case of logical sub-partitioning when 2 ESPs are accessing
  // the same partition.
  // Assume that DP2 will not switch after every read but on 
  // average after every DP2_SEQ_READS_WITHOUT_SEEKS (internal CQD) 
  // reads. We will ignore adjustment if it is smaller than 1 seek.

  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  ULng32  maxDp2ReadInKB = 
    defs.getAsULong(DP2_MAX_READ_PER_ACCESS_IN_KB);
  ULng32 seqReadsBeforeSwitch =
    defs.getAsULong(DP2_SEQ_READS_WITHOUT_SEEKS);
    
  seeks += (seqKB/maxDp2ReadInKB/seqReadsBeforeSwitch).getFloor();
  
  // temporary patch for solution 10-041104-1450. To make hash join
  // look more expensive that nested join if right table is not very
  // small we add one seek to the right scan.
  if ( seqKB > 8.0 )
  {
      seeks++;
  }

  return seeks;

} // SimpleFileScanOptimizer::estimateNumberOfSeeksPerScan(...)

// SimpleFileScanOptimizer::estimateSeqKBReadPerScan()
// Estimate the amount (in KBytes) of data read per scan (partition).
// Include the index blocks which must be read.
// 
// Need to read every index level except the root (always in cache?)
//
// Even if there are no rows expected in the result set, the index
// blocks still need to be read.
//
// Do we need to read (some of) the index blocks twice, once for the begin key
// and once for the end key??????
//
// Returns: the number of sequential KB read for a single subset access to
// a partition (this includes index and data blocks)
//
CostScalar
SimpleFileScanOptimizer::estimateSeqKBReadPerScan() const
{

  // Compute the index blocks touched:
  //
  // indexLevels is the max index levels of all partitions.
  // Assume at least 1 block (root) needs to be read.
  //
  Lng32 indexBlocks = MAXOF(getIndexDesc()->getIndexLevels(),1);

  // For single subset, all rows are contiguous. Get the number of KB
  // required to hold the expected result.  Assumes rows are dense (no
  // fragmentation or slack)
  // 
  CostScalar blocksRead(getNumBlocksForRows(getSingleSubsetSize()));

  // Assume that the blocks are evenly distributed over the active
  // partitions.
  //
  blocksRead = (blocksRead / getNumActivePartitions()).getCeiling();

  // Include the indexBlocks in the blocks to be read.
  // indexBlocks is already 'per scan' so no need to divide by NumActParts
  //
  blocksRead = blocksRead + indexBlocks;

  // Covert Blocks to KB
  //
  return blocksRead * getBlockSizeInKb();
} // SimpleFileScanOptimizer::estimateSeqKBReadPerScan()

// SimpleFileScanOptimizer::computeCostVectors()
// Computes the cost vectors (First and Last Row) for this scan.
// Computes:
//    - KBytes
//    - CPU Instructions
//    - Number of seeks
//    - Idle time
//    - number of probes
// for both cost vectors.
//
// OUTPUTS: firstRow and lastRow SimpleCostVectors of the BasicCost
// object are populated.
//
void
SimpleFileScanOptimizer::computeCostVectors(FileScanBasicCost *basicCostPtr)
  const
{

  SimpleCostVector &firstRow = basicCostPtr->getFRBasicCostSingleSubset();

  SimpleCostVector &lastRow = basicCostPtr->getLRBasicCostSingleSubset();

  CostScalar seqKBFR;
  CostScalar seqKBLR;

  // Compute the sequential KBytes accessed to produce the first and
  // last rows.
  estimateKB(seqKBFR, seqKBLR);

  firstRow.addKBytesToIOTime(seqKBFR);
  lastRow.addKBytesToIOTime(seqKBLR);

  // Record the total KB accessed in the BasicCost object.
  //
  basicCostPtr->setSingleSubsetNumKBytes(seqKBLR);
  basicCostPtr->setEstRowsAccessed(getSingleSubsetSize());

  // Estimate the of seeks required for this single subset per scan
  // (partition).  Include the seeks required for accessing the index
  // blocks.
  //
  const CostScalar seeksPerScanFR(estimateNumberOfSeeksPerScan(seqKBFR));
  const CostScalar seeksPerScan(estimateNumberOfSeeksPerScan(seqKBLR));
  firstRow.addSeeksToIOTime(seeksPerScanFR);
  lastRow.addSeeksToIOTime(seeksPerScan);

  const CostScalar selectedRowsPerScan(getResultSetCardinalityPerScan());

  const CostScalar rowsPerScan 
    ((getSingleSubsetSize()/getEstNumActivePartitionsAtRuntime()).getCeiling());

  // Estimate minimum number of rows that must be processed in order
  // to get the first row.  This is typically the number of rows that
  // will fit in a DP2 buffer.  If there are fewer rows than this in
  // the result, then only that many rows need to be processed.  This
  // will be used to compute the CPU instruction required to produce
  // the first row.
  //
  const CostScalar dp2Rows(getRowsInDP2Buffer());

  const CostScalar rowsPerScanFR(MINOF(dp2Rows, rowsPerScan));

  const CostScalar selectedRowsPerScanFR (MINOF(dp2Rows, selectedRowsPerScan));

  CostScalar cpuInstructionsFR; 
  CostScalar cpuInstructionsLR;
  
  // Estimate CPU Instructions (includes Per Request, Per Row and Per
  // KB number of CPU Instructions) for First and Last row accessed.
  //
  estimateCPUInstructions(seeksPerScan,
                          rowsPerScanFR,
                          rowsPerScan,
                          selectedRowsPerScanFR,
                          selectedRowsPerScan,
                          seqKBFR,
                          seqKBLR,
                          cpuInstructionsFR,
                          cpuInstructionsLR);

  firstRow.addInstrToCPUTime(cpuInstructionsFR);
  lastRow.addInstrToCPUTime(cpuInstructionsLR);

  // Estimate the amount of Idle time used by this Scan.  Includes
  // time to open secondary partitions (all but the first).
  //
  const CostScalar idleTime(estimateIdleTime());

  firstRow.addToIdleTime(idleTime);
  lastRow.addToIdleTime(idleTime);

  // For single subset, non-nested join, there is only one probe.
  //
  firstRow.setNumProbes(csOne);
  lastRow.setNumProbes(csOne);

} // SimpleFileScanOptimizer::computeCostVectors(...)

// SimpleFileScanOptimizer::estimateKB()
// Compute the sequential KBytes accessed to produce the first and
// last rows. 
//
// OUTPUTS: 
// seqKBFR - The number of KBytes to be accessed to produce the first row.
// seqKBLR - The number of KBytes to be accessed to produce the last row.
//
void
SimpleFileScanOptimizer::estimateKB(CostScalar &seqKBFR,
                                    CostScalar &seqKBLR) const
{

  // total number of blocks to be read sequentially In order to
  // produce the last row, need to read all required blocks.
  //
  seqKBLR = estimateSeqKBReadPerScan();

  // Estimate minimum number of KB that must be processed in order to
  // get the first row.  This is typically the size in KB of a DP2
  // buffer.  If the result contains fewer KB than this, then
  // only the result needs to be processed.
  //
  CostScalar dp2BufferSize
    (CostPrimitives::getBasicCostFactor(DP2_MESSAGE_BUFFER_SIZE));

  // Number of KB required to produce the first row
  //
  seqKBFR = MINOF(dp2BufferSize, seqKBLR);

} // SimpleFileScanOptimizer::estimateKB()


// SimpleFileScanOptimizer::estimateCPUInstructions()
// Estimate CPU Instructions (includes Per Request, Per Row and Per KB
// number of CPU Instructions) for First and Last row accessed.
//
// Inputs: 
//
//    seeksPerScan - The number of seeks to be performed by this Scan
//    (single partition).
//
//    rowsPerScanFR - The number of rows accessed by this scan before
//    returning the first row (single partition)
//
//    rowsPerScanLR - The number of rows accessed by this scan (single
//    partition)
//
//    seqKBFR - The number of KBytes to be accessed by this Scan to
//    produce the first row (single partition).
//
//    seqKBLR - The number of KBytes to be accessed by this Scan to
//    produce the last row (single partition).
//
// Outputs:
// cpuInstructionsFR - Number of CPU instructions to access the First row.
// cpuInstructionsLR - Number of CPU instructions to access the Last row.
//
void
SimpleFileScanOptimizer::estimateCPUInstructions(
                  const CostScalar &seeksPerScan,
                  const CostScalar &rowsPerScanFR,
                  const CostScalar &rowsPerScanLR,
                  const CostScalar &selectedRowsPerScanFR,
                  const CostScalar &selectedRowsPerScanLR,
                  const CostScalar &seqKBFR,
                  const CostScalar &seqKBLR,
                  CostScalar &cpuInstructionsFR,
                  CostScalar &cpuInstructionsLR) const
{

  // Estimate per-request CPU instructions.  This includes
  // performing a binary search of each index block to find the
  // appropriate entry.
  //
  CostScalar instrPerReqst(estimateCPUPerReqstInstrs());

  // Add in the per data request overhead cost paid by the cpu
  //
  instrPerReqst += CostPrimitives::getBasicCostFactor(CPUCOST_DATARQST_OVHD);

  // Estimate the per-row CPU instructions.  This includes the
  // instructions necessary to copy the row from DP2 to the EXEINDP2.
  //
  const CostScalar instrPerRow(estimateCPUPerRowInstrs());

  // Estimate per-seek CPU instructions.  This is the extra cost
  // necessary for each seek.  For now this is just a default value.
  // Commented out because it was never used
  //const CostScalar instrPerSeek(estimateCPUPerSeekInstrs());

  // Estimate per-KB CPU instructions.  This is the extra cost
  // necessary to copy each KB from DP2 to the EXEINDP2.  For now this
  // is just a default value.
  //
  const CostScalar instrPerKB(estimateCPUPerKBInstrs());

  // Estimate the number of instruction reqquired to evaluate the
  // Executor predicates on one row.
  //
  const CostScalar instrPerExePredPerRow(estimateCPUExePredPerRowInstrs());

  // First Row CPU instructions.
  //
  cpuInstructionsFR = 
    instrPerReqst
    //+ (instrPerSeek * seeksPerScan)
    + (instrPerKB * seqKBFR)
    + (instrPerRow * selectedRowsPerScanFR)
    + (instrPerExePredPerRow * rowsPerScanFR);

  // Last Row CPU instructions.
  //
  cpuInstructionsLR = 
    instrPerReqst
    //+ (instrPerSeek * seeksPerScan)
    + (instrPerKB * seqKBLR)
    + (instrPerRow * selectedRowsPerScanLR)
    + (instrPerExePredPerRow * rowsPerScanLR);

  // Sanity Check
  //
  DCMPASSERT(cpuInstructionsFR <= cpuInstructionsLR);

} // SimpleFileScanOptimizer::estimateCPUInstructions(...)

// estimateCPUPerReqstInstrs()
// Estimate the number of CPU instructions used for each request.
// This includes performing a binary search of each index block,
// comparing the keys (byte compare of encoded keys) to find the
// appropriate entry. And includes the default per request overhead.
//
// Return - the number of CPU instruction required to search the index
// blocks to find the appropriate data block for this request.
//
CostScalar
SimpleFileScanOptimizer::estimateCPUPerReqstInstrs() const
{
  // We do a binary search after we get the data block in memory to
  // find the matching row. So in worst case scenario it would just be
  // ln(recordsPerBlock).  log function will give us the natural
  // logarithm. So, to get logX = lnX/log2 1.44 * lnX.

  // RMW - Shouldn't this use the number of records per index block
  // which could be quite different 
  // RMW - Shouldn't this use the key size in bytes rather than the
  // number of key columns.
  // RMW - Shouldn't this use the cost of byte compare (not pred compare)
  //
  /*
  CostScalar instrPerReqst =
    ( 1.44 * log((getEstimatedRecordsPerBlock()).getValue())
      * getIndexDesc()->getIndexKey().entries()
      * CostPrimitives::getBasicCostFactor(CPUCOST_PREDICATE_COMPARISON)
      * getIndexDesc()->getIndexLevels() );
  */
  // Change according to Bob's comments. There is a small 
  // overestimation because of data block having less rows that index
  // block but we don't need very high accuracy here.
  // Do we need to add syskey processing in case of clustering non-
  // unique index? SP. Do we want to keep entriesPerIndexBlocks as
  // a separate attribute?

  const IndexDesc * index = getIndexDesc();
  const double avrgSlack = 0.7;
               
  // Assume 8 bytes per key overhead (pointer to low level block)
  // we'' use ceiling because block with K key would have K+1 pointers)
  CostScalar entriesPerIndexBlock = ( index->getBlockSizeInKb() * 
    1024 * avrgSlack / (8 + index->getKeyLength())).getCeiling();
  CostScalar instrPerReqst =
    ( // number of comparisons per index blocks
      1.44 * log(entriesPerIndexBlock.getValue()) * 
      // comparison cost, will need a special CQD later
      index->getKeyLength() * CostPrimitives::
        getBasicCostFactor(CPUCOST_COMPARE_COMPLEX_DATA_TYPE_PER_BYTE)
      // number of index blocks to go through
      * index->getIndexLevels() 
    );

  return instrPerReqst;
} // SimpleFileScanOptimizer::estimateCPUPerReqstInstrs()

// estimateCPUPerRowInstrs()
// Estimate the number of CPU instructions used to copy each row from
// DP2 to the EXEINDP2.
//
// Return - the number of CPU instruction required to copy each row from
// DP2 to the EXEINDP2.
//
CostScalar
SimpleFileScanOptimizer::estimateCPUPerRowInstrs() const
{
  // Currently we don't copy rows from DP2Cache to EID. We only
  // copy selected row columns required by charOutput. Using
  // record size in KB which is file table record size could
  // cause huge overestimation of CPU cost for Scan.
  // Do we want to count only data we really copy? SP.
  // ------------------------------------------------------------
  // The per row overhead, regardless of the row size
  //
  CostScalar instrPerRowMovedOutOfTheCache
    (CostPrimitives::getBasicCostFactor(CPUCOST_COPY_ROW_OVERHEAD) +
     CostPrimitives::getBasicCostFactor(CPUCOST_SCAN_OVH_PER_ROW));

  // The number of CPU instructions dependent on the row size.
  //
  instrPerRowMovedOutOfTheCache +=
    // previously recordSizeInKb*(copyCostPerByte*1024+ovhPerKB)
    getRelExpr().getGroupAttr()->getRecordLength() *
    ((CostPrimitives::getBasicCostFactor(CPUCOST_COPY_ROW_PER_BYTE)) 
  + CostPrimitives::getBasicCostFactor(CPUCOST_SCAN_OVH_PER_KB)/1024);

  return  instrPerRowMovedOutOfTheCache;
} // SimpleFileScanOptimizer::estimateCPUPerRowInstrs()

// estimateCPUExePredPerRowInstrs() 
// Estimate the number of CPU instructions required to evaluate the
// executor predicate on one row.
//
// Return - the number of CPU instruction required to evaluate the
// executor predicate on one row.
//
// The number of CPU instructions is estimated by counting the number
// of operators in the executor predicates.
// The following ItemExprs are counted as operators:
//   - The implied AND operators of the ValueIdSet
//   - Non-Leaf ItemExprs
//   - VEG Predicates
//
CostScalar
SimpleFileScanOptimizer::estimateCPUExePredPerRowInstrs() const
{
  // A copy of the executor predicates
  //
  ValueIdSet exePreds = getSearchKey()->getExecutorPredicates();

  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  // fsoToUse - 
  // IF 0 - Use original "Complex" File Scan Optimizer. (Default)
  // IF 1 - Use logic below to determine FSO to use.
  // IF 2 - Use logic below to determine FSO to use, but also use new
  //        executor predicate costing.
  // IF >2 - Always use new "Simple" File Scan Optimizer.
  //
  ULng32 fsoToUse = defs.getAsULong(FSO_TO_USE);

  // Should we use the old way of costing predicates?
  //
  if (fsoToUse < 2) {
    return (exePreds.entries() * 
            CostPrimitives::getBasicCostFactor(CPUCOST_PREDICATE_COMPARISON));
  }

  // Used to keep track of values to examine in next iteration
  //
  ValueIdSet children;

  // Count the number of operators (non-leaf) in the predicate
  //
  ULng32 numOps = 0;

  // Account for the implied AND operators in the ValueIdSet
  //
  if(exePreds.entries() > 1) {
    numOps = (exePreds.entries() - 1) * 2;
  }

  // Iterate over the levels of the executor predicates.  exePreds
  // will contain the value Ids for all ItemExprs at a given level of
  // the expression tree.
  //
  while(exePreds.entries()) {

    // Look at each expression at this level.
    //
    for(ValueId valId=exePreds.init();
        exePreds.next(valId);
        exePreds.advance(valId)) {

      ItemExpr *oper = valId.getItemExpr();
    
      Int32 numChildren = oper->getArity();

      if(numChildren == 0 &&
         (oper->getOperatorType() == ITM_VEG_PREDICATE )) {

        // If this is a VEGPred count it as an operator. 
        //
        // This could be simply a NOT NULL test
        // or an equality pred
        // or a tree of equality preds
        // or nothing.
        // For a scan (and that is what we have here), this is most likely a
        // NOT NULL predicate or nothing.
        //
        numOps++;
      } else if (numChildren > 0) {
        // If this is a non-leaf node, count this as an operator
        //
        numOps += numChildren;

        // Schedule all the children for examination
        //
        for(Int32 i = 0; i < numChildren; i++) {
          children += oper->child(i)->getValueId();
        }
      }
    }
    exePreds = children;
    children.clear();
  }

  // Compute cost for executor predicates Assume each operator has the
  // same cost, the cost of a single predicate comparison
  //
  return
    (CostPrimitives::getBasicCostFactor(CPUCOST_PREDICATE_COMPARISON ) *
     numOps) / 2;

} // SimpleFileScanOptimizer::estimateCPUExePredPerRowInstrs()

// estimateCPUPerSeekInstrs()
// Estimate the per-seek CPU instructions.  This is the extra CPU cost
// necessary for each seek.  For now this is just a default value.
// 
//CostScalar
//SimpleFileScanOptimizer::estimateCPUPerSeekInstrs() const
//{
  // Add a per seek cost for the cost of moving
  // data from disk to DP2:
  //
//  return CostPrimitives::getBasicCostFactor(CPUCOST_SCAN_DSK_TO_DP2_PER_SEEK);
//}

// estimateCPUPerKBInstrs()
// Estimate per-KB CPU instructions.  This is the extra CPU cost
// necessary to copy each KB from DP2 to the EXEINDP2.  For now this
// is just a default value.
//
CostScalar
SimpleFileScanOptimizer::estimateCPUPerKBInstrs() const
{
  // Add a per kb cost for the cost of moving
  // every block from disk to DP2:
  return CostPrimitives::getBasicCostFactor(CPUCOST_SCAN_DSK_TO_DP2_PER_KB);
}

// estimateIdleTime()
// Estimate the amount of Idle time used by this Scan.
//
// CPUCOST_SUBSET_OPEN lumps together all the overhead needed to
// set-up the access to each partition. Thus it is a blocking cost,
// nothing can overlap with it.  Since scans are not blocking, by
// definition, put the cost in idle time: this is the cost for opening
// all the partitions but the first partition.  The first partition is
// opened before the ScanOptimizer is called. During opening the first
// partition, the necessary info on the file is also acquired so it is
// more expensive and cannot be overlaid.  Root accounts for the cost
// of opening the first partition of all the tables.
//
CostScalar
SimpleFileScanOptimizer::estimateIdleTime() const
{
  // Default cost to open subsequent partitions.
  //
  CostScalar idleTime 
    (CostPrimitives::getBasicCostFactor(CPUCOST_SUBSET_OPEN_AFTER_FIRST));

  // Open all Partitions, but the first.
  //
  idleTime = idleTime * MAXOF(0, (getNumActivePartitions() - 1));

  // Convert from CPU instructions to seconds.
  // (MSCF_ET_CPU is the cpu-to-time multiplier)
  //
  idleTime = idleTime * CostPrimitives::getBasicCostFactor(MSCF_ET_CPU);

  return idleTime;
}

// Return the repeat Count for this scan.  The repeat count is an
// estimate of the number of probes that this scan (partition) will
// receive.
// A return value of >= 1.0 indicates that this is a multiprobe scan
// (below a nested join).
// A return value of csZero (0.0) indicates that this is a single
// probe scan.
//
CostScalar
SimpleFileScanOptimizer::getRepeatCount() const
{
  CostScalar repeatCount =  getContext().getPlan()->getPhysicalProperty()->
    getDP2CostThatDependsOnSPP()->getRepeatCountForOperatorsInDP2();

  if ((repeatCount > csOne) OR
      (getContext().getInputLogProp()->getColStats().entries() > 0)) {

    // indicate that it is multiprobe;
    return repeatCount;
  } else {

    // Indicates that this is single-probe.
    return csZero;
  }
}

CostScalar SimpleFileScanOptimizer::getProbeCacheCostAdjFactor() const
{
  CostScalar probeCacheCostAdj = csOne;
  Lng32 cacheEntries =  (ActiveSchemaDB()->getDefaults()).getAsLong(GEN_PROBE_CACHE_NUM_ENTRIES);

  Lng32  downCacheSize = (ActiveSchemaDB()->getDefaults()).getAsLong(GEN_PROBE_CACHE_SIZE_DOWN);

  // upper bound cost adjustment as a saftey net
  CostScalar ubound = (ActiveSchemaDB()->getDefaults()).getAsDouble(NCM_NJ_PC_THRESHOLD);

  // skip probe cache cost adj if threshold is more than 100%
  // way of disabling fix for testing purpose
  if (ubound > 1.0)
    return probeCacheCostAdj; 
  
  // ignore cacheEntries if it is smaller than downCacheSize.
  cacheEntries = MAXOF(cacheEntries, downCacheSize);
  // get number of streams (ESPs) 
  Lng32 countOfStreams = 
  getContext().getPlan()->getPhysicalProperty()->getCurrentCountOfCPUs();  

  CostScalar dupSuccProbes = getDuplicateSuccProbes(); 
  CostScalar totalProbes = getProbes(); 
  CostScalar uniqProbes = getUniqueProbes(); 

  // borderline case, if all probes are duplicate, meaning only one value,
  // we still need to make one IO for the first probe.
  if (dupSuccProbes == totalProbes)
    dupSuccProbes = dupSuccProbes - csOne;

  CostScalar probeCacheRatio = dupSuccProbes / totalProbes;
  CostScalar probeCacheHitRatio;

  // compute cache hit ratio. Full benefit for order probes case.
  if ( getInOrderProbesFlag() )
    probeCacheHitRatio = csOne;
  else // Random order case
  {
    // determine if we can safely divide probes by DOP.
    // when probecache column is part of partition key, we can safely divide.
    NABoolean safeToDivide = FALSE;
    // get access to probecache column, which is nothing but char.inputs
    ValueIdSet myCharInputs = getRelExpr().getGroupAttr()->getCharacteristicInputs();
    // get access to partition function of my sibling via IPP.
    const InputPhysicalProperty* ippForMe = getContext().getInputPhysicalProperty();

    if (ippForMe != NULL)
    {
      const PartitioningFunction* ch0PartFunc = NULL;
      // first try plan0 location, if it's not there try location of other plans

      ch0PartFunc = ippForMe->getNjOuterOrderPartFuncForNonUpdates();
      if (ch0PartFunc == NULL)
        ch0PartFunc = ippForMe->getNjOuterOrderPartFunc();

      ValueIdSet ch0PartKey;
      if (ch0PartFunc != NULL)
        ch0PartKey = ch0PartFunc->getPartitioningKey();

      // if either probe cache column or left child partkey is empty, we can't 
      // divide
      if (NOT(myCharInputs.isEmpty() OR ch0PartKey.isEmpty()))
        // if my char.input contains child0 PartKey, we can safely divide
        if (myCharInputs.contains(ch0PartKey))
          safeToDivide = TRUE;
    }

    // if cache is too small to hold all unique entries,
    // compute fraction of probes that will get a cache hit
    // (conservative estimate, assuming no ordering of probes
    // and no gain from the countOfStream parallel caches)
    if (safeToDivide)
      uniqProbes = uniqProbes.value() / countOfStreams;
    probeCacheHitRatio = (cacheEntries / uniqProbes.value());
    probeCacheHitRatio.maxCsOne();
  }

  // apply upper bound threshold, currently 100% cost adjustment
  probeCacheRatio = probeCacheRatio * probeCacheHitRatio * ubound;
  probeCacheCostAdj = (1 - probeCacheRatio.value());

  return probeCacheCostAdj;
}

// This method is different from NestedJoin::isProbeCacheApplicable
// in a sense that it returns true only if RHS of NJ is file_scan_unique.
// Basically we don't want to apply cost adjustment for semi and anti_semi joins
NABoolean SimpleFileScanOptimizer::isProbeCacheApplicable() const
{
  // right child is file scan unique
  // probes are duplicate
  // NJ probe cache CQD is ON

  if ( getSearchKey()->isUnique()                         &&
       (duplicateSuccProbes_ > 0)                         &&
       (CmpCommon::getDefault(NESTED_JOIN_CACHE) == DF_ON) )
    return TRUE;
  else
    return FALSE;
}

// SimpleFileScanOptimizer::categorizeMultiProbes()
// Computes and caches the following metrics regarding probes:
//
//  probes_ - the total number of probes for all active partitions.
//    this could be different from outer probes - result cardinality 
//    of InputLogPtoperties. The fan-out of each outer probe - 
//    the average number of active partitions one outer probe goes
//    to should be taken into account when estimating uniqueSuccProbes
//    and uniqueFailedProbes    
//
//  successfulProbes_ - the number of probes (probes_) for all AP
//    that produce some data.
//
//  uniqueProbes_ - the number of distinct probes (probes_) for all
//    AP. Includes successful and failed probes.
//
//  duplicateSuccProbes_ - the number of successful probes
//    (successfulProbes_) for all AP that are not unique.  
//     duplicateSuccProbes = successfulProbes - uniqueSuccProbes.
//
//  failedProbes_ - the number of probes (probes_) that do not result
//  in data.  failedProbes = probes - successfulProbes.
//
//  dataRows_ - the number of rows accessed by all successful probes.
//
//  blksPerSuccProbe_ - the estimated number of blocks produced by
//    each successful probe on average.
//
//  inOrderProbesFlag - indicates if the probes arrive in order
//    relative to the clustering key of the access path.
//
// Return Value: void - Computed values are cached in data members.
//
void
SimpleFileScanOptimizer::categorizeMultiProbes(NABoolean *isAnIndexJoin)
{

  // first get the histograms for the probes:
  //
  Histograms
    outerHistograms(getContext().getInputLogProp()->getColStats());

  // Costing works by estimating the resources of the whole, unpartitioned
  // table, thus getting the total probes by multiplying by the active
  // partitions. This is because to estimate successful and failed probes
  // we need to use histograms that provide statistics for the table in 
  // the whole. Then total resources spent will be divided by the number 
  // of AP, cost vector is estimated for 1 AP only. Finally, depending on
  // the level of parallelism, number of CPUs executing DP2s, and
  // synchronous/asynchronous access to APs the cost object will be
  // computed for the whole Scan. This logic results in lots of confusion
  // in the current implementation. It is absolutely critical to understand
  // at each step are we calculating something for all APs, for one
  // partition only or for the group of partitions providing results for
  // the same ESP.
  //
  if (CmpCommon::getDefault(NCM_HBASE_COSTING) == DF_ON)
    probes_ = (getRepeatCount() *
               getEstNumActivePartitionsAtRuntimeForHbaseRegions()).minCsOne();
  else
    probes_ = (getRepeatCount() * getEstNumActivePartitionsAtRuntime()).minCsOne();

  // all the probes that don't have duplicates
  //
  CostScalar uniqueSuccProbes(csZero);

  CostScalar uniqueFailedProbes(csZero);
    
  ValueIdSet singleSubsetPreds(getSingleSubsetPreds());

  categorizeProbes(successfulProbes_       // out
                   ,uniqueSuccProbes       // out
                   ,duplicateSuccProbes_   // out
                   ,failedProbes_          // out
                   ,uniqueFailedProbes	   // out
                   ,probes_
                   ,singleSubsetPreds
                   ,outerHistograms
                   ,FALSE                  // this is not MDAM!
                   ,&dataRows_
                   ,isAnIndexJoin
                    );
  uniqueProbes_ = uniqueSuccProbes + uniqueFailedProbes; 
 
  // -----------------------------------------------------------
  // Compute the rows and blks per successful probe:
  // -----------------------------------------------------------

  CostScalar rowsPerSuccessfulProbe(csZero);
  if (successfulProbes_ >= csOne)
    {
      rowsPerSuccessfulProbe = dataRows_/successfulProbes_;
    }

  blksPerSuccProbe_ =
    (rowsPerSuccessfulProbe / getEstimatedRecordsPerBlock()).getCeiling();

  // No successful probe can grab less than one block:
  //
  if ((successfulProbes_ >= csOne) && (blksPerSuccProbe_ < csOne))
    {
      blksPerSuccProbe_ = csOne;
    }


  // Determine whether probes order matches the scan's clustering
  // order:
  //
  const InputPhysicalProperty* ippForMe =
    getContext().getInputPhysicalProperty();

  NABoolean partiallyInOrderOK = TRUE;
  NABoolean probesForceSynchronousAccess = FALSE;

  if ((ippForMe != NULL) AND
      SimpleFileScanOptimizer::ordersMatch(ippForMe,
                  getIndexDesc(),
                  &(getIndexDesc()->getOrderOfKeyValues()),
                  getRelExpr().getGroupAttr()->getCharacteristicInputs(),
                  partiallyInOrderOK,
                  probesForceSynchronousAccess))
    {
      setInOrderProbesFlag(TRUE);
      setProbesForceSynchronousAccessFlag(probesForceSynchronousAccess);
    }
  else if ((ippForMe != NULL) AND
          (ippForMe->getAssumeSortedForCosting()) AND
          (!(ippForMe->getExplodedOcbJoinForCosting())) AND
          (ippForMe->getNjOuterOrderPartFuncForNonUpdates() == NULL))           
    {
         // assumeSortedForCosting_ flag is set for two cases:
         // case 1: when input is rowset.
         // case 2: when left child partFunc njOuterPartFuncForNonUpdates_ 
         //         is passed for NJ plan 0. This is only for cost estimation 
         //         of exchange operator and not scan operator.
         // So we should come here only for case1. 
         // To avoid case2, we check njOuterPartFuncForNonUpdates_ for NULL.
       setInOrderProbesFlag(TRUE);
    }
  else
    {
      setInOrderProbesFlag(FALSE);
    }

} // SimpleFileScanOptimizer::categorizeMultiProbes()

// -----------------------------------------------------------------------
// INPUT:
//      probes: the total probes coming in
//      preds: the predicates to compute the join
//
// OUTPUT:
//      successfuleProbes: those probes that match any data
//      uniqueSuccProbes: those successful probes that do not have duplicates
//      duplicateSuccProbes: successfulProbes - uniqueSuccProbes
//      failedProbes: probes - successfulProbes
// -----------------------------------------------------------------------
void
SimpleFileScanOptimizer::categorizeProbes(CostScalar& successfulProbes /* out */
                                ,CostScalar& uniqueSuccProbes /* out */
                                ,CostScalar& duplicateSuccProbes /* out */
                                ,CostScalar& failedProbes /* out */
                                ,CostScalar& uniqueFailedProbes /* out */  
                                ,const CostScalar& probes
                                ,const ValueIdSet& preds
                                ,const Histograms& outerHistograms
                                ,const NABoolean isMDAM
                                ,CostScalar * dataRows
                                ,NABoolean *isAnIndexJoin
                                 ) const
{
  CostScalar numOuterProbes = 
    (getContext().getInputLogProp())->getResultCardinality();

  if (outerHistograms.isEmpty())
  {
    // In general, this should not happen. Even for cross product outer
    // histogram should contain some information about columns in outer
    // table characteristic output. One known case is when VEG of equi-join
    // predicate contains a const, then outerHistograms are not created.
    // Probably, it is worth to revisit this and still generate histograms,
    // they could be useful later. Currently, let's have a special case
    // here. We'd like to find any other special cases and treat them
    // separately. We'll generate assert in debug compiler if the case
    // is not known yet. For release compiler we will return all probes
    // as successful.

    NABoolean allPredHaveConstExpr = TRUE; 

    for( ValueId predId=preds.init(); 
         preds.next(predId); preds.advance(predId))
    {
      const ItemExpr *pred = predId.getItemExpr();
      if (((VEGPredicate *)pred)->getOperatorType() == ITM_VEG_PREDICATE)
      {
        const VEG * predVEG = ((VEGPredicate *)pred)->getVEG();
        const ValueIdSet & VEGGroup = predVEG->getAllValues();

        if (VEGGroup.referencesAConstExpr())
          continue;
        else
        {
           allPredHaveConstExpr = FALSE;
           break;
        }
      }
    }//end of check for A Constant Expression

    if (allPredHaveConstExpr)
    {
      successfulProbes = probes;
      if (CmpCommon::getDefault(NCM_HBASE_COSTING) == DF_ON)
        uniqueSuccProbes = getEstNumActivePartitionsAtRuntimeForHbaseRegions();
      else
        uniqueSuccProbes = getNumActivePartitions();
      duplicateSuccProbes = probes - uniqueSuccProbes;
      failedProbes = uniqueFailedProbes = csZero;
      *dataRows = numOuterProbes * getEffectiveTotalRowCount();
      return;
    }

    // other special cases will be put here

    successfulProbes = uniqueSuccProbes = probes;
    duplicateSuccProbes = failedProbes = csZero;   
    // This will assert debug compiler when comp_bool_38 is ON
    // to run regression and skip this assert for debug
    // compiler this CQD needs to be set to OFF (default)
    // 
    if ( CmpCommon::getDefault(COMP_BOOL_38) == DF_ON )
    {
      SFSOWARNING("Outer histogram is empty for multiprob scan(NJ)");
      DCMPASSERT(0);
    }
  }
  else
  {
      // If there are input values we are in a nested join case,
      // else it must be a cartesian product:
      const ValueIdSet& inputValues =
	getRelExpr().getGroupAttr()->getCharacteristicInputs();

      // get this filescan statistics:
      IndexDescHistograms
        joinedHistograms(*getIndexDesc(),
                         getIndexDesc()->getIndexKey().entries() );

      // Since there are input values, apply the join preds
      // to estimate the failed probes:
      // Apply the join predicates to the local statistics:
      joinedHistograms.applyPredicatesWhenMultipleProbes(
                   preds
                   ,*(getContext().getInputLogProp())
                   ,inputValues
                   ,isMDAM
                   ,NULL
                   ,NULL
                   ,isAnIndexJoin
                   ,REL_SCAN);
      failedProbes = csZero; // assume no probes will fail
      if (NOT inputValues.isEmpty())
	{

        if (NOT preds.isEmpty())
	    {
	      // and compute the failed probes:
	      const ValueIdSet& operatorValues =
		getIndexDesc()->getIndexKey();
	      failedProbes =
		joinedHistograms.computeFailedProbes(outerHistograms
						     ,preds
						     ,inputValues
						     ,operatorValues
						     );
              if(CmpCommon::getDefault(COMP_BOOL_110) == DF_ON)
	      {
	         DCMPASSERT(probes >= failedProbes);              
	      }
              failedProbes = MINOF(probes,failedProbes);
	    }
	} // if there are input values

      if(dataRows) *dataRows = joinedHistograms.getRowCount();
      // ------------------------------------------------------------------
      // Compute successful probes:
      // -------------------------------------------------------------------
      successfulProbes = probes - failedProbes;

      // --------------------------------------------------------------
      // Compute unique probes:
      // -------------------------------------------------------------
      // Combine unique entry count of the probes:
      // (assume independence of columns and compute
      // Compute combined UEC of outer histograms):
      CostScalar uniqueEntryCountOfProbes = csOne;
      CostScalar histUEC;

      ValueIdSet columnsOfVegPreds;
      for( ValueId predId=preds.init(); 
         preds.next(predId); preds.advance(predId))
      {
         const VEGPredicate *vegPred = 
           (VEGPredicate *)predId.getItemExpr();
         if ( vegPred->getOperatorType() == ITM_VEG_PREDICATE )
         {
           columnsOfVegPreds += vegPred->getVEG()->getAllValues();
         }
      }

      for (CollIndex i=0; i < outerHistograms.entries(); i++)
      {
        if ( columnsOfVegPreds.contains( 
               outerHistograms[i].getColumn()) OR
             columnsOfVegPreds.contains( 
               outerHistograms[i].getVEGColumn()) OR
             inputValues.contains(
               outerHistograms[i].getVEGColumn())
           )
        {
          histUEC = outerHistograms[i].getColStats()
            ->getTotalUec().getCeiling();

    	  uniqueEntryCountOfProbes *= histUEC;
        }
      }

      //should not be more than # of probes
      if (uniqueEntryCountOfProbes > probes)
        uniqueEntryCountOfProbes = probes;

      // should not be less than 1
      uniqueEntryCountOfProbes.minCsOne();

      // The unique successful probes are the successful
      // fraction of their UEC:
      uniqueSuccProbes = MINOF( 
        ((successfulProbes/numOuterProbes)*uniqueEntryCountOfProbes),
	     successfulProbes);

      uniqueFailedProbes = MINOF(
        ((failedProbes/numOuterProbes)*uniqueEntryCountOfProbes),
          failedProbes );

      // --------------------------------------------------------------
      // Compute successful probes that are duplicates of another
      // successful probe:
      // --------------------------------------------------------------
      duplicateSuccProbes = successfulProbes - uniqueSuccProbes;
  }
} // SimpleFileScanOptimizer::categorizeProbes(...)

// Estimate the number of different index blocks visited for 
// MultiProbe Scan. This method will use the number of unique probes
// and total key length of index columns instead of universal "rule 
// of thumb" from IndexDesc::getEstimatedIndexBlocksLowerBound()

void SimpleFileScanOptimizer::estimateIndexBlockUsageMultiProbeScan()
{
  const IndexDesc * index = getIndexDesc();
  Lng32 curLevel = index->getIndexLevels();
  const double avrgSlack = 0.7;

  CostScalar leafProbesPerScan = 
    (uniqueProbes_ / getNumActivePartitions()).getCeiling();				
  CostScalar innerBlocksPerScan = (getEffectiveTotalRowCount()/
    getEstimatedRecordsPerBlock()/getNumActivePartitions()).getCeiling();
               
  // Assume 8 bytes per key overhead (pointer to low level block)
  // we'' use ceiling because block with K key would have K+1 pointers)
  CostScalar entriesPerIndexBlock = (index->getBlockSizeInKb() * 1024 / 
    (8 + index->getKeyLength()) * avrgSlack).getCeiling();
           
  // to prevent looping in while
  if ( entriesPerIndexBlock.isGreaterThanOne() )
  {
    // Will not have more pointers than inner blocks per Scan (divided  
    // by entriesPerIndexBlocs give the number of index leaf blocks).
   	  
    CostScalar currentLevelIndexBlocks = MINOF(leafProbesPerScan,
      innerBlocksPerScan/entriesPerIndexBlock).getCeiling();
                
    CostScalar totalIndexBlocks = currentLevelIndexBlocks;
    while( --curLevel > 0 )
    {
      currentLevelIndexBlocks = 
        (currentLevelIndexBlocks / entriesPerIndexBlock).getCeiling();
      totalIndexBlocks += currentLevelIndexBlocks;
    }
    indexBlocksLowerBound_ = totalIndexBlocks;
  }
  else
  {
    // This shouldn't happen but we need to set some big value for rls 
    // compiler if it did.             
    indexBlocksLowerBound_ = innerBlocksPerScan;
    DCMPASSERT(0);
  }

} // SimpleScanOptimizer::estimateIndexBlockUsageMultiProbeScan()

// SimpleFileScanOptimizer::computeCostVectorsMultiProbe()
// Computes the cost vectors (First and Last Row) for this scan.
// Computes:
//    - KBytes
//    - CPU Instructions
//    - Number of seeks
//    - Idle time
//    - number of probes
// for both cost vectors.
//
// OUTPUTS: firstRow and lastRow SimpleCostVectors of the BasicCost
// object are populated.
//
void
SimpleFileScanOptimizer::computeCostVectorsMultiProbes(FileScanBasicCost *basicCostPtr)
{
  // Effective Total Row Count is the size of the bounding subset of
  // all probes.  Typically this will be all the rows of the table,
  // but if all probes are restricted to a subset of rows (e.g. the
  // key predicate contains leading constants) then the effective row
  // count will be less than the total row count.
  //
  estimateEffTotalRowCount(totalRowCount_, effectiveTotalRowCount_);

  categorizeMultiProbes();

  // Adjustments for Partial order case.
  // Divide right child total rows  and left child probes by the by the total
  // uec of the right child key prefix.
  CostScalar oldEffectiveTotalRowCount;;
  CostScalar oldSuccessfulProbes;
  CostScalar oldDuplicateSuccProbes;
  CostScalar oldFailedProbes;
  CostScalar oldUniqueProbes;
  CostScalar oldProbes;

  if (getPartialOrderProbesFlag())
  {
    oldEffectiveTotalRowCount = effectiveTotalRowCount_;
    effectiveTotalRowCount_ =
      (effectiveTotalRowCount_/keyPrefixUEC_).getCeiling();

    oldSuccessfulProbes = successfulProbes_;
    successfulProbes_ = (successfulProbes_/keyPrefixUEC_).getCeiling();
    oldDuplicateSuccProbes = duplicateSuccProbes_;
    duplicateSuccProbes_ = (duplicateSuccProbes_/keyPrefixUEC_).getCeiling();
    oldFailedProbes = failedProbes_;
    failedProbes_ = (failedProbes_/keyPrefixUEC_).getCeiling();
    oldUniqueProbes = uniqueProbes_;
    uniqueProbes_ = (uniqueProbes_/keyPrefixUEC_).getCeiling();
    oldProbes = probes_;
    probes_ = (probes_/keyPrefixUEC_).getCeiling();
  }

  estimateIndexBlockUsageMultiProbeScan();


  SimpleCostVector &firstRow = basicCostPtr->getFRBasicCostSingleSubset();

  SimpleCostVector &lastRow = basicCostPtr->getLRBasicCostSingleSubset();

  CostScalar seqKBFR;
  CostScalar seqKBLR;

  // Compute the sequential KBytes accessed to produce the first and
  // last rows.
  estimateKBMultiProbe(seqKBFR, seqKBLR);

  seqKBLR = seqKBLR * keyPrefixUEC_;
  firstRow.addKBytesToIOTime(seqKBFR);
  lastRow.addKBytesToIOTime(seqKBLR);

  // Record the total KB accessed in the BasicCost object.
  //
  basicCostPtr->setSingleSubsetNumKBytes(seqKBLR);
  basicCostPtr->setEstRowsAccessed(getDataRows());

  // Estimate the of seeks required for this single subset per scan
  // (partition).  Include the seeks required for accessing the index
  // blocks.
  //
  CostScalar seeksLR(estimateNumberOfSeeksPerScanMultiProbe());
  CostScalar seeksFR(seeksLR/getProbes());
  seeksFR = seeksFR.getCeiling();

  firstRow.addSeeksToIOTime(seeksFR);
  lastRow.addSeeksToIOTime(seeksLR);

  // Restore old values.
  if (getPartialOrderProbesFlag())
  {
    effectiveTotalRowCount_ = oldEffectiveTotalRowCount;
    successfulProbes_ = oldSuccessfulProbes;
    duplicateSuccProbes_ = oldDuplicateSuccProbes;
    failedProbes_ = oldFailedProbes;
    uniqueProbes_ = oldUniqueProbes;
    probes_ = oldProbes;
  }

  DCMPASSERT(seeksFR <= seeksLR);

  CostScalar selectedRowsPerScan(getResultSetCardinalityPerScan());

  const CostScalar rowsPerScan 
    ((getDataRows()/getNumActivePartitions()).getCeiling());

  // Patch for Sol.10-031024-0755 (Case 10-031024-9406).
  
     selectedRowsPerScan = MINOF(selectedRowsPerScan, rowsPerScan);

  // Estimate minimum number of rows that must be processed in order
  // to get the first row.  This is typically the number of rows that
  // will fit in a DP2 buffer.  If there are fewer rows than this in
  // the result, then only that many rows need to be processed.  This
  // will be used to compute the CPU instruction required to produce
  // the first row.
  //
  const CostScalar dp2Rows(getRowsInDP2Buffer());

  const CostScalar rowsPerScanFR(MINOF(dp2Rows, rowsPerScan));

  const CostScalar selectedRowsPerScanFR(MINOF(dp2Rows, selectedRowsPerScan));

  CostScalar cpuInstructionsFR; 
  CostScalar cpuInstructionsLR;
  
  // Estimate CPU Instructions (includes Per Request, Per Row and Per
  // KB number of CPU Instructions) for First and Last row accessed.
  //
  estimateCPUInstructionsMultiProbe(seeksLR,
                                    rowsPerScanFR,
                                    rowsPerScan,
                                    selectedRowsPerScanFR,
                                    selectedRowsPerScan,
                                    seqKBFR,
                                    seqKBLR,
                                    cpuInstructionsFR,
                                    cpuInstructionsLR);

  firstRow.addInstrToCPUTime(cpuInstructionsFR);
  lastRow.addInstrToCPUTime(cpuInstructionsLR);

  // Estimate the amount of Idle time used by this Scan.  Includes
  // time to open secondary partitions (all but the first).
  //
  const CostScalar idleTime(estimateIdleTime());

  firstRow.addToIdleTime(idleTime);
  lastRow.addToIdleTime(idleTime);

  const CostScalar probesPerScan
    ((getProbes()/getNumActivePartitions()).getCeiling());

  firstRow.setNumProbes(probesPerScan);
  lastRow.setNumProbes(probesPerScan);

} // SimpleFileScanOptimizer::computeCostVectorsMultiProbes(...)

// SimpleFileScanOptimizer::estimateCPUInstructionsMultiprobe()
// Estimate CPU Instructions (includes Per Request, Per Row and Per KB
// number of CPU Instructions) for First and Last row accessed.
//
// Inputs: 
//
//    seeksPerScan - The number of seeks to be performed by this Scan
//    (single partition).
//
//    rowsPerScanFR - The number of rows accessed by this scan before
//    returning the first row (single partition).
//
//    rowsPerScanLR - The number of rows accessed by this scan (single
//    partition).
//
//    seqKBFR - The number of KBytes to be accessed by this Scan to
//    produce the first row (single partition).
//
//    seqKBLR - The number of KBytes to be accessed by this Scan to
//    produce the last row (single partition).
//
// Outputs:
// cpuInstructionsFR - Number of CPU instructions to access the First row.
// cpuInstructionsLR - Number of CPU instructions to access the Last row.
//
void
SimpleFileScanOptimizer::estimateCPUInstructionsMultiProbe(
                  const CostScalar &seeksPerScan,
                  const CostScalar &rowsPerScanFR,
                  const CostScalar &rowsPerScanLR,
                  const CostScalar &selectedRowsPerScanFR,
                  const CostScalar &selectedRowsPerScanLR,
                  const CostScalar &seqKBFR,
                  const CostScalar &seqKBLR,
                  CostScalar &cpuInstructionsFR,
                  CostScalar &cpuInstructionsLR) const
{

  const CostScalar probesPerScan
    ((getProbes()/getNumActivePartitions()).getCeiling());

  const CostScalar 
    succProbesPerScan
    ((getSuccessfulProbes()/getNumActivePartitions()).getCeiling());

  cpuInstructionsFR = 0.0;
  cpuInstructionsLR = 0.0;

  if (probesPerScan > csZero)
    {

      // Estimate the per-request CPU instructions.  This includes
      // performing a binary search of each index block to find the
      // appropriate entry.
      //
      // Instructions to traverse the B-Tree for successful requests
      // +
      // Instructions to traverse the B-Tree for unsuccessful requests
      //
      CostScalar instrPerReqst(csZero);

      // number of instructions to traverse btree for one probe.
      //
      const CostScalar cpuForBTreeComparisons(estimateCPUPerReqstInstrs());

      // Successful requests: We assume that the cpu cost for *each*
      // successful request is a constant overhead plus a function of the
      // number of index blocks traversed and the length of key columns

      if(succProbesPerScan > csZero)
        {
          instrPerReqst += 
            (succProbesPerScan *
             (cpuForBTreeComparisons +
              CostPrimitives::getBasicCostFactor(CPUCOST_DATARQST_OVHD)));
        }

      const CostScalar failedRequestsPerScan =
        probesPerScan - succProbesPerScan;

      if (failedRequestsPerScan > csZero)
        {
          instrPerReqst += 
            (failedRequestsPerScan * (cpuForBTreeComparisons +
              CostPrimitives::getBasicCostFactor(CPUCOST_DATARQST_OVHD))
            );

        }


      // Estimate the per-row CPU instructions.  This includes the
      // instructions necessary to copy the row from DP2 to the EXEINDP2.
      //
      const CostScalar instrPerRow(estimateCPUPerRowInstrs());

      // Estimate per-seek CPU instructions.  This is the extra cost
      // necessary for each seek.  For now this is just a default value.
      // Commented out because it was never used. SP.
      //const CostScalar instrPerSeek(estimateCPUPerSeekInstrs());

      // Estimate the per-KB CPU instructions.  This is the extra cost
      // necessary to copy each KB from volume to DP2 cache. For now this
      // is just a default value.
      //
      const CostScalar instrPerKB(estimateCPUPerKBInstrs());

      // Estimate the number of instruction required to evaluate the
      // Executor predicates on one row.
      //
      const CostScalar instrPerExePredPerRow(estimateCPUExePredPerRowInstrs());

      // First Row CPU instructions.
      //
      cpuInstructionsFR = 
        // assume that the first row gets its data in the successful request:
        (instrPerReqst/probesPerScan)

       // + (instrPerSeek * seeksPerScan)
        + (instrPerKB * seqKBFR)

        // Only rows that are selected are moved out of the cache:
        + (instrPerRow * selectedRowsPerScanFR)

        + (instrPerExePredPerRow * rowsPerScanFR);

      // Last Row CPU instructions.
      //
      cpuInstructionsLR = 
        instrPerReqst
        //+ (instrPerSeek * seeksPerScan)
        + (instrPerKB * seqKBLR)
        + (instrPerRow * selectedRowsPerScanLR)
        + (instrPerExePredPerRow * rowsPerScanLR);

      // Sanity Check
      //
      DCMPASSERT(cpuInstructionsFR <= cpuInstructionsLR);
    }


} // SimpleFileScanOptimizer::estimateCPUInstructionsMultiProbe(...)

// SimpleFileScanOptimizer::estimateNumberOfSeeksPerScanMultiProbe() 
// Estimate the of seeks required for this all probes per scan
// (partition).  Include the seeks required for accessing the index
// blocks.
//
// Need to Seek to every index level except the root, plus need
// to seek to first data block.
//
// Do we need to seek twice this amount, once for the begin key
// and once for the end key??????
// We, probably, do. Dp2 does it instead of using 
// IOBlocksToReadPerAccess when deciding about prefetch.
//
// Returns: the number of seeks required for all probes to access
// a partition 
//
CostScalar
SimpleFileScanOptimizer::estimateNumberOfSeeksPerScanMultiProbe() const
{
  // -------------------------------------------------------------
  // Compute the blocks in the inner table:
  // -------------------------------------------------------------
  const CostScalar
    totalRowsInInnerTable = getEffectiveTotalRowCount();

  CostScalar innerBlocksUpperBound
    (totalRowsInInnerTable/getEstimatedRecordsPerBlock());
  innerBlocksUpperBound = innerBlocksUpperBound.getCeiling();

  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  const CostScalar uniqueProbes = getUniqueProbes();

  ///////////////////////////////////////////////////////////////////////
  // fix the
  // estimation for seeks beginBlocksLowerBound = no. of unique
  // entries WHICH ARE SUCCESSFUL in matching with inner table
  // totalBlocksLowerBound = no. of blocks returning from the OUTER
  // TABLE after applying the predicate
  //
  // innerBlocksUpperBound = Total size of inner table
  // DBB = Distance between the blocks
  // MSD = Distance for average seek
  // CostScalar(0.025) = (seek time is between .2ms and 8ms ignore
  //                      latency, so fixed is .2ms/8ms=0.025)
  // (0.008) Limiting the rate of increase upto twice that of inner
  // blocks and then using a step function
  //
  // STEP FUNCTION so as to decrease the rate of increase after twice
  // of inner blocks cost for fixed seeks is the, % of probes which
  // can be read with minimum seek time finalRows are the final
  // resulting rows, matched with outertable and qualified with <
  // predicate To the readAhead cost the Mrows * finalRows is added,
  // this is done in order to calculate the cost of moving the rows to
  // the executor. Once the rows from the outer table come in and they
  // are compared with the innertable rows, the successful rows will
  // be returned to the executor, so the more the number of rows
  // returned to the executor the more the cost.
  ///////////////////////////////////////////////////////////////////////

  CostScalar seekComputedWithDp2ReadAhead(csZero);
  CostScalar indexBlocksLowerBound = getIndexBlocksLowerBound();
  CostScalar extraIndexBlocks(csZero);

  if(getInOrderProbesFlag())
    {
      
      // uniqueProbes calculated from CategorizeProbes should be at
      // least one.
      //
      CMPASSERT(uniqueProbes > csZero);

      const CostScalar DBB=(innerBlocksUpperBound / uniqueProbes);
      const CostScalar MSD = defs.getAsULong(NJ_MAX_SEEK_DISTANCE);
      const CostScalar Ulimit = defs.getAsDouble(NJ_INC_UPTOLIMIT);
      const CostScalar Mrows  = defs.getAsDouble(NJ_INC_MOVEROWS);

      CostScalar fixedSeeks;
      CostScalar Limit = csTwo * innerBlocksUpperBound;

      if (uniqueProbes > Limit)
        {
          const CostScalar Alimit = defs.getAsDouble(NJ_INC_AFTERLIMIT);

          CostScalar extraProbes = uniqueProbes - Limit;
          fixedSeeks = Ulimit * Limit + Alimit * extraProbes;
        }
      else
        {
          fixedSeeks = Ulimit * (uniqueProbes-1);
        }

      CostScalar variableSeeks = uniqueProbes - fixedSeeks;

      CostScalar finalRows=getResultSetCardinality();

      seekComputedWithDp2ReadAhead = 
        csOne 
        + fixedSeeks
        + (variableSeeks * (DBB/MSD))
        + (Mrows * finalRows);
    }
  else
    {
      // It is not clear why are we talking about DP2 readAhead 
      // in the case of random probes. It makes sense only if the 
      // whole table/index partition fits into 
      // DP2_MAX_READ_PER_ACCESS. SP.
      
      // This has to use DP2Cache in the way similar to estimateSeqKB.
      // Duplicate probes may require lots of extra seeks.

      const CostScalar uniqueSuccProbes = MINOF(uniqueProbes,
        getSuccessfulProbes() - getDuplicateSuccProbes());

      // The begin blocks denote the starting data
      // block for the subset that each succ. probe generates
      //
      const CostScalar beginBlocksLowerBound = 
        MINOF(uniqueSuccProbes, innerBlocksUpperBound);

	  // Assume that even if the whole index doesn't fit into cache
      // we will have to read all index blocks (1 seek per block)
      // plus 1 seek per extraIndexBlock if index does not fit in 
      // DP2Cache (only for the index leaf blocks - rest will stay
	  // in cache).
     
      CostScalar 
        cacheSize(getDP2CacheSizeInBlocks(getBlockSizeInKb()));

      if ( indexBlocksLowerBound >= cacheSize )
      {
        CostScalar cacheMissProbabilityForIndexBlock =
          (indexBlocksLowerBound - cacheSize)/indexBlocksLowerBound;
        extraIndexBlocks = (getProbes()/getNumActivePartitions() -
          cacheSize) * cacheMissProbabilityForIndexBlock;
        // each probe will cause a seek at data block level
        seekComputedWithDp2ReadAhead = getProbes();
      }
      else
      {
        cacheSize -= indexBlocksLowerBound;
      
        const CostScalar 
          cacheSizeForAll(cacheSize * getNumActiveDP2Volumes());

        if (getSearchKey()->isUnique()) 
        {
          // In this case for each probe no more than one data
          // block is accessed. Readahead won't be used.
          CostScalar estBlocksAccessedByUniqueProbes =
            MINOF(uniqueProbes, innerBlocksUpperBound);
          if ( estBlocksAccessedByUniqueProbes <= cacheSizeForAll )
          {
            seekComputedWithDp2ReadAhead = 
              estBlocksAccessedByUniqueProbes;
          }
          else
          {
            // Assume random unique probes never cause read ahead
            CostScalar cacheMissProbabilityForUniqueProbe =
              ((estBlocksAccessedByUniqueProbes - cacheSizeForAll)
                /estBlocksAccessedByUniqueProbes).minCsZero();

            seekComputedWithDp2ReadAhead = 
              cacheSizeForAll + // First probes to fill the cache
              (getProbes() - cacheSizeForAll) * // rest could miss
              cacheMissProbabilityForUniqueProbe;
          }
        }
        else
        {
          // SearchKey is not unique. In this case each
          // successful probe could bring several (or even all)
          // blocks of inner table. One unique failed probe would 
          // access one data block. Readahead could be used.
          CostScalar uniqueFailedProbes = 
              (uniqueProbes - uniqueSuccProbes).minCsZero();

          CostScalar innerBlocksAccessedBySuccProbes =
            (uniqueSuccProbes * getBlksPerSuccProbe());

          CostScalar innerBlocksAccessedByAllProbes = 
            innerBlocksAccessedBySuccProbes + uniqueFailedProbes;
                    
          if(innerBlocksAccessedByAllProbes <= cacheSizeForAll)
          {
            // inner table completely fits in cache
            const CostScalar maxDp2ReadInKB
            (defs.getAsULong(DP2_MAX_READ_PER_ACCESS_IN_KB));

            const CostScalar 
              maxDp2ReadInBlocks(maxDp2ReadInKB/getBlockSizeInKb());

            if ( innerBlocksAccessedByAllProbes < 
                 maxDp2ReadInKB * getNumActivePartitions())
            {
              // first read will read the whole table/index 
              // partition
              seekComputedWithDp2ReadAhead = 
                getNumActivePartitions();
            }
            else
            {
              /*
              seekComputedWithDp2ReadAhead = 
                MINOF(beginBlocksLowerBound,
                  innerBlocksUpperBound/maxDp2ReadInBlocks);
              */
              // The formula above is too optimistic, this needs
              // a separate investigation, Use conservative estimate
              // when only the first read causes read ahead.
              seekComputedWithDp2ReadAhead = 
                (beginBlocksLowerBound - maxDp2ReadInBlocks).minCsOne();
            }
          }
          else
          {
            if ( getBlksPerSuccProbe() > cacheSizeForAll )
            {
              // the result of successful probe does not fit in cache
              // therefore each duplicate probe will have to reread
              // all necessary blocks and have an extra seek. The 
              // number of seeks is the same as the number of probes
              seekComputedWithDp2ReadAhead = getProbes();
            }
            else
            {
              // table does not fit in cache extra data block seeks 
              // needed, there is a chance that successful or failed 
              // probe hits the cache. Assume that successful and 
              // failed probes share the cache proportionally the 
              // number of blocks they accessed.

              // Part of cache to keep successful probe blocks
              CostScalar partOfSuccProbes = 
                cacheSizeForAll * innerBlocksAccessedBySuccProbes 
                / innerBlocksAccessedByAllProbes;

              // Part of cache to keep failed probe blocks
              CostScalar partOfFailedProbes = 
                (cacheSizeForAll - partOfSuccProbes).minCsZero();
            
              // part of succ probes that fits in its part of cache
              CostScalar cacheHitProbForSucc = (partOfSuccProbes / 
                innerBlocksAccessedBySuccProbes).maxCsOne();
                     
              // part of failed probes that fits in its part of cache
              CostScalar cacheHitProbForFail(csZero);
              if ( uniqueFailedProbes.isGreaterThanZero() ) 
                cacheHitProbForFail =
                (partOfFailedProbes / uniqueFailedProbes).maxCsOne();

            // assume that each random unique probe causes 1 seek
            seekComputedWithDp2ReadAhead = getUniqueProbes() + 
              getDuplicateSuccProbes() * 
              (csOne - cacheHitProbForSucc) +
              (getFailedProbes() - uniqueFailedProbes) * 
              (csOne - cacheHitProbForFail);
            }
          }
        }
      }
    }
  
  // Assuming indexBlocksLowerBound is for one partition only
  return (seekComputedWithDp2ReadAhead/getNumActivePartitions() + 
          indexBlocksLowerBound + extraIndexBlocks).getCeiling();

}//SimpleFileScanOptimizer::estimateNumberOfSeeksPerScanMultiProbe()


// SimpleFileScanOptimizer::estimateTotalRowCount()
// Estimate the real and effective total row count for the table.  
// The Effective Total Row Count is the size of the bounding subset 
// of all probes.  Typically this will be all the rows of the table, 
// but if all probes are restricted to a subset of rows (e.g. the 
// key predicate contains leading constants) the effective row 
// count will be less than the total row count.
//
// Returns: The estimated real and effective row count.
//
void
SimpleFileScanOptimizer::estimateEffTotalRowCount(
                            CostScalar &realRowCount,
                            CostScalar &effRowCount) const
{
  // CR 10-010822-4815
  // Query 04 in OBI was producing a bad plan as the seeks for Nested
  // Join were computed without taking into account if the blocks of
  // inner table are consecutive or not. This is fixed by computing
  // the seeks after getting the right count of blocks by getting the
  // rows for the key predicates on the leading columns with a Value
  // Equality Group referencing a constant.  "totalPreds" is the set
  // of all predicates of all key columns with a constant for all the
  // index values find the keyPredicates, for every predicate check if
  // it is a VEG and that VEG has a constant, if so then add the
  // predicate to the "totalPreds". Repeat this only for the leading
  // columns, if the column doesn't have a constant then stop
  // searching. Apply the "totalPreds" to the histograms to get the
  // rowcount, divide the rowcount by recordsperblock to get the
  // blocks. If no constant then pass the total blocks for inner table

  // Determine if all probes will be contained within a subset of the
  // whole table.  (Key predicate contains leading constants. e.g. k1
  // = 3 and k2 = 4 and k3 = fk1)
  //

  // The set of leading constant equality predicates.
  //
  ValueIdSet totalPreds;

  // Did we find at least one leading column with a constant equality
  // predicate.
  //
  NABoolean hasAtleastOneConstExpr = FALSE;

  // Organize the predicates by key columns.
  //
  ColumnOrderList keyPredsByCol(getIndexDesc()->getIndexKey());
  
  getSearchKey()->getKeyPredicatesByColumn(keyPredsByCol);

  // How many of the leading predicates represent a single subset.
  //
  CollIndex singleSubsetPrefixColumn;
  keyPredsByCol.getSingleSubsetOrder(singleSubsetPrefixColumn);

  // Examine the predicates on the single subset key columns.
  // RMW - BUG - Could this code break out of the inner loop too early?
  //
  for (CollIndex Indx=0; Indx <= singleSubsetPrefixColumn; Indx++)
    {
      // predicate on column which is part of key
      //
      const ValueIdSet *keyPreds = keyPredsByCol[Indx];
      
      // Is there an constant equality predicate on this column
      //
      NABoolean curFound = FALSE;

      if (keyPreds AND NOT keyPreds->isEmpty())
        {
          ValueId predId;

          for(predId=keyPreds->init(); //Inner for loop for all the
              keyPreds->next(predId);  //the predicates of the column
              keyPreds->advance(predId))
            {

              const ItemExpr *pred = predId.getItemExpr();

              if (pred->getOperatorType() == ITM_VEG_PREDICATE)
                {
                  const VEG *predVEG = ((VEGPredicate *)pred)->getVEG();
                  const ValueIdSet &VEGGroup = predVEG->getAllValues();

                  if (VEGGroup.referencesAConstExpr())
                    {
                      totalPreds += predId;
                      hasAtleastOneConstExpr = TRUE;
                      curFound = TRUE;

                    } //end of check for A Constant Expression
                  else
                    break;  // break out of inner loop.  May also
                            // break out of outer loop

                } //end of "if" Operator to be VEG predicate

            } //end of inner loop "for(predId=partPreds->init();...)"

        } //end of "if" predicates to be non empty

      if (NOT curFound)
        break;

    } //end of outer loop "for (CollIndex i=0; i <= singleSubsetPre...)"

  
  // Get the histograms for the inner table (this scan)
  IndexDescHistograms 
    innerHistograms(*getIndexDesc(),
                    getIndexDesc()->getIndexKey().entries());

  //GET ROW count
  realRowCount = innerHistograms.getRowCount();
      
  // If there are leading constant equality predicate, then use these
  // predicates to determine the effective row count.  Otherwise use
  // the whole table row count.
  //
  if( hasAtleastOneConstExpr )
    {
      const SelectivityHint * selHint = getFileScan().getSelectivityHint();
      const CardinalityHint * cardHint = getFileScan().getCardinalityHint();

      innerHistograms.applyPredicates(totalPreds, getRelExpr(), selHint, cardHint, REL_SCAN);

      //GET ROW count
      effRowCount = innerHistograms.getRowCount();
    }
  else
    {
      effRowCount = realRowCount;
    }

  // This is a simple patch until we figure out the way to properly 
  // estimate effRowCount. When we know the number of active 
  // this estimate cannot exceed the ratio of actPart over total
  // of part multiplied by realRowCount. This could be done by using
  // more predicates in totalPreds variable.

  // All this casting away is because mutable is not supported
  NodeMap * nodeMapPtr = (NodeMap *)(getContext().getPlan()->
    getPhysicalProperty()->getPartitioningFunction()->getNodeMap());

  CostScalar actPart = getEstNumActivePartitionsAtRuntimeForHbaseRegions();

  if (CmpCommon::getDefault(NCM_HBASE_COSTING) == DF_OFF)
    actPart = getNumActivePartitions();

  effRowCount = MINOF( effRowCount, 
                       realRowCount * actPart/nodeMapPtr->getNumEntries() );

  return;
} // SimpleFileScanOptimizer::estimateEffTotalRowCount()

// SimpleFileScanOptimizer::estimateKBMultiProbe()
// Compute the sequential KBytes accessed to produce the first and
// last rows. 
//
// OUTPUTS: 
// seqKBFR - The number of KBytes to be accessed to produce the first row.
// seqKBLR - The number of KBytes to be accessed to produce the last row.
//
void
SimpleFileScanOptimizer::estimateKBMultiProbe(CostScalar &seqKBFR,
                                              CostScalar &seqKBLR) const
{
  // total number of blocks to be read sequentially In order to
  // produce the last row, need to read all required blocks.
  //
  seqKBLR = estimateSeqKBReadPerScanMultiProbe();
  
  // Estimate minimum number of KB that must be processed in order to
  // get the first row.  This is typically the size in KB of a DP2
  // buffer.  If the result contains fewer KB than this, then
  // only the result needs to be processed.
  //
  CostScalar dp2BufferSize
    (CostPrimitives::getBasicCostFactor(DP2_MESSAGE_BUFFER_SIZE));

  // Number of KB required to produce the first row
  //
  seqKBFR = MINOF(dp2BufferSize, seqKBLR);

} // SimpleFileScanOptimizer::estimateKBMultiProbe(...)

// SimpleFileScanOptimizer::estimateSeqKBReadPerScanMultiProbe()
// Estimate the amount (in KBytes) of data read per scan (partition).
// Include the index blocks which must be read.
// 
// Consider these cases:
//   Full Cache Benefit - Duplicate probes always get a cache hit.
//   No Cache Benefit   - Duplicate probes never get a cache hit.
//   Random access      - Duplicate probes may get a cache hit.
//
// Returns: the number of sequential KB read for access to
// a partition (this includes index and data blocks)
//
CostScalar
SimpleFileScanOptimizer::estimateSeqKBReadPerScanMultiProbe() const
{
  CostScalar seqBlocks;

  // Compute the blocks in the inner table:
  //
  const CostScalar
    totalRowsInInnerTable = getEffectiveTotalRowCount();

  const CostScalar innerBlocks
    (totalRowsInInnerTable/getEstimatedRecordsPerBlock());

  CostScalar innerBlocksPerScan
    (innerBlocks/getNumActivePartitions());
  innerBlocksPerScan = innerBlocksPerScan.getCeiling();

  // Number of distinct blocks accessed 
  // SP. This is a misnomer, if each probe - assume unique - access 1 block 
  // it may exceed the number of total blocks in inner table. It could be 
  // called innerBlocksAccessedByProbes
  //
  const CostScalar uniqBlocks 
    ((getSuccessfulProbes() - getDuplicateSuccProbes()) * getBlksPerSuccProbe());

  CostScalar uniqBlocksPerScan
    (uniqBlocks/getNumActivePartitions());
  uniqBlocksPerScan = uniqBlocksPerScan.getCeiling();

  // Cache size for one DP2 process for this Block size
  //
  CostScalar cacheSize(getDP2CacheSizeInBlocks(getBlockSizeInKb()));

  CostScalar indexBlocksLowerBound = getIndexBlocksLowerBound();
  CostScalar extraIndexBlocks(csZero);

  CostScalar maxBlocksToRead =
    // all blocks for successful probes
    (getSuccessfulProbes()*getBlksPerSuccProbe() + 
    // all blocks for failed probes
    getFailedProbes());

  // We will assume that index blocks will stay in DP2 cache whenever possible
  // meaning that data block cannot force index block out of cache. Therefore,
  // data blocks can use only the rest of DP2cache after index blocks. If index
  // block lower bound cannot fit in cache completely we will add extra index
  // blocks to read and there will be no cache hits for data blocks.

  if ( indexBlocksLowerBound >= cacheSize )
  {
    if ( getInOrderProbesFlag() )
    {
      // It is hardly possible that duplicate probe won't find any of
      // index blocks in DP2cache, so there won't be extra index blocks to read,
      // each unique probe will read, if necessary, only blocks counted in
      // indexBloksLowerBound. Because each duplicate probe will need the same
      // data blocks there will be no extra data blocks if all blocks per
      // successful probe fit in the cache
      // We would ignore 2-3 blocks at most of index path to these data blocks.
      if ( getBlksPerSuccProbe() <= cacheSize )
      {
        if (CmpCommon::getDefault(COMP_BOOL_28) == DF_OFF)
        {
          seqBlocks = 
          (getUniqueProbes()/getProbes())* // ratio of unique probes
          maxBlocksToRead;
        }
        else
        {
          seqBlocks = MINOF(innerBlocks,
          (getUniqueProbes()/getProbes())* // ratio of unique probes
          maxBlocksToRead);
        }
      }
      else
        // duplicate probes require reread of the table because beginning
        // of the previous probe was already lost
      {
          seqBlocks = maxBlocksToRead;
      }

    }
    else
    // this is the worst case, even index blocks need new read
    {
      CostScalar cacheMissProbabilityForIndexBlock =
        (indexBlocksLowerBound - cacheSize)/indexBlocksLowerBound;
      extraIndexBlocks = ( getProbes()/getNumActivePartitions() -
        cacheSize) * cacheMissProbabilityForIndexBlock;

      seqBlocks = maxBlocksToRead;
    }
  }
  else
  {
    cacheSize -= indexBlocksLowerBound;
  

    // Total cache size for all active DP2s.  Note that some partitions
    // may share the same DP2 and thus will share the same cache.
    //
    const CostScalar totCacheSize(cacheSize * getNumActiveDP2Volumes());

    if ( (innerBlocks <= totCacheSize)           // Whole table fits in cache
         OR                                    
         (uniqBlocks + getFailedProbes() <= totCacheSize) // all blocks accessed
         OR                                      
         // Duplicate probe finds data in cache
         ( (getBlksPerSuccProbe() <= cacheSize) AND getInOrderProbesFlag() )
       ) 
    {
      // Full cache benefit. Duplicate probes always get a cache hit.
      seqBlocks = MINOF(innerBlocks, uniqBlocks + getFailedProbes());

    } else if ((getBlksPerSuccProbe() > cacheSize) &&
              getInOrderProbesFlag()) {

    // No benefit from cache, Duplicate probes always must reread
    // blocks.  Probes come in order, so duplicate probes will be
    // together, but the cache cannot hold the whole request.  The
    // first block(s) has already been flushed from the cache.  So
    // must reread all these blocks again.
    //
    seqBlocks = maxBlocksToRead;

    } else {

    // orders don't match, and cache can hold only a portion of the result.
    // RANDOM case:
    //
    // Must read the uniqBlocks
    // Must read some of the duplicate blocks.
    // Calculate these extraDataBlocks based on an estimate cache hit rate.
    //
    // the hit probability is the fraction of the cache size over the
    // total blocks to be read:
    //

    CostScalar cacheHitProbability(csOne);

    CostScalar lowerBoundBlockCount = uniqBlocks + getFailedProbes();

    cacheHitProbability = (totCacheSize/lowerBoundBlockCount).maxCsOne();
    
    // Duplicate Probes get a cache hit with a probability of
    // cacheHitProbability
    //
    // Duplicate probes that result in extra block reads (cache miss)
 
    const CostScalar 
      extraDataBlocks = (getDuplicateSuccProbes() * getBlksPerSuccProbe()) 
                        * (csOne - cacheHitProbability);
     
    seqBlocks = lowerBoundBlockCount // unique blocks, need to read each one
      + extraDataBlocks;             // Duplicate blocks may get a cache hit.
    
    }

  }
  // Assume that the blocks are evenly distributed over the active
  // partitions.
  //
  seqBlocks = (seqBlocks / getNumActivePartitions() +
               indexBlocksLowerBound + extraIndexBlocks).getCeiling();
  
  // Convert from Blocks to KB
  //
  return (seqBlocks * getBlockSizeInKb());
} // SimpleFileScanOptimizer::estimateSeqKBReadPerScanMultiProbe()

// -----------------------------------------------------------------------
//  SimpleFileScanOptimizer::ordersMatch method
//  This method determines if the inner table (right child) sort key is in
//  the same order as the outer table (left child)sort key. Only applicable
//  when processing right child of a nested join operator.
//
// INPUT PARAMS:
//   ipp: The input physical properties of the right child.
//   indexDesc: The index descriptor of the right child.
//   innerOrder : the sort key for the right child.
//   charInputs: The characteristic inputs for this operator
//   partiallyInOrderOK: TRUE if the order can be used even if the probes
//                       are not completely in order. This will be TRUE
//                       for read, update and delete, and FALSE for insert.
// OUTPUT PARAMS:
//   probesForceSynchronousAccess: TRUE if the probes are completely
//     in order across multiple partitions and the clustering key is
//     the same as the partitioning key. FALSE otherwise.
// RETURN VALUE:
//   TRUE if there is a match complete between the probes order and the
//   table's (or index's) order. FALSE otherwise.
// -----------------------------------------------------------------------
NABoolean
SimpleFileScanOptimizer::ordersMatch(
                       const InputPhysicalProperty* ipp,
                       const IndexDesc* indexDesc,
                       const ValueIdList* innerOrder,
                       const ValueIdSet& charInputs,
                       NABoolean partiallyInOrderOK,
                       NABoolean& probesForceSynchronousAccess)
{
  ValueIdList innerOrderProbeCols;
  ValueIdList uncoveredCols;
  // temporary var to keep column Ids without inverse for possible
  // use to get UEC for this column from histograms
  ValueIdList innerOrderProbeColsNoInv;

  CollIndex numInOrderCols = 0;

  NABoolean partiallyInOrder = FALSE;
  NABoolean fullyInOrder = FALSE;

  probesForceSynchronousAccess = FALSE;

  if ((ipp != NULL) AND (!(ipp->getAssumeSortedForCosting())) AND (!(ipp->getExplodedOcbJoinForCosting())))
  {
    // Shouldn't have an ipp if there are no outer order columns!
    if ((ipp->getNjOuterOrder() == NULL) OR
         ipp->getNjOuterOrder()->isEmpty())
    {
      CCMPASSERT(FALSE);
      return FALSE;
    }
    // An ipp should also have the outer expression partitioning function!
    if (ipp->getNjOuterOrderPartFunc() == NULL)
    {
      CCMPASSERT(FALSE);
      return FALSE;
    }
    // Should not have passed the ipp if this access path could not
    // use the outer order, so there MUST be at least one sort key column!
    if (innerOrder->isEmpty())
    {
      CCMPASSERT(FALSE);
      return FALSE;
    }

    // Get the physical partitioning function for the access path
    const PartitioningFunction* physicalPartFunc =
      indexDesc->getPartitioningFunction();
    // If the outer order is not a DP2 sort order, and the access path
    // is range partitioned, then need to check if the probes will force
    // synchronous access to the partitions of this access path. The
    // probes will force synchronous access if the leading partitioning
    // key column is the same as the leading clustering key column.
    if ((ipp->getNjDp2OuterOrderPartFunc() == NULL) AND
        (physicalPartFunc != NULL)) // will be NULL for unpart tables
    {
      const RangePartitioningFunction* rpf =
        physicalPartFunc->castToRangePartitioningFunction();
      if (rpf != NULL)
      {
        ValueIdList partKeyAsList;
        // Get the partitioning key as a list
        partKeyAsList = rpf->getKeyColumnList();
        CCMPASSERT(NOT partKeyAsList.isEmpty());

        // Get the leading partitioning key column
        ValueId leadingPartKeyCol = partKeyAsList[0];

        // Get the leading clustering key column - remove any INVERSE node
        ValueId leadingClustKeyCol = (*innerOrder)[0];
        leadingClustKeyCol =
          leadingClustKeyCol.getItemExpr()->removeInverseOrder()->getValueId();

        if (leadingClustKeyCol == leadingPartKeyCol)
          probesForceSynchronousAccess = TRUE;
      } // end if a range partitioned table
    } // end if not a DP2 sort order and a partitioned table

    // Determine which columns of the index sort key are probe columns,
    // up to the first column not covered by a constant or probe column.
    // To do this we call the "findNJEquiJoinCols" method. The equijoin
    // cols are the probe columns, i.e. the values coming from the outer
    // child. For read these will be the equijoin columns, for write
    // they are the key values of the records that need to be written.
    //
    // Note that for write, all the call to this method really does
    // is eliminate any columns that are covered by constants or
    // params/host vars. This is because all key columns will
    // always be probe columns, since we always need all the key cols
    // to accurately determine the record to insert/update/delete. So,
    // we could just call the method "removeCoveredExprs" for write.
    // This would require adding another parameter to the ordersMatch
    // method to distinguish write from read. This is considered
    // undesirable, and so is not done.

    innerOrderProbeCols =
      innerOrder->findNJEquiJoinCols(
        ipp->getNjOuterCharOutputs(),
        charInputs,
        uncoveredCols);

    // There MUST be some probe columns, otherwise there should not
    // have been an ipp!
    if (innerOrderProbeCols.isEmpty())
    {
      CCMPASSERT(FALSE);
      return FALSE;
    }

    if ( CmpCommon::getDefault(COMP_BOOL_96) == DF_OFF )
    {
      // Before doing any processing, must eliminate any duplicates from
      // the innerOrder key. Index sort keys can have 
      // duplicate valueid's referring to base columns,
      // we need to remove them to check for partial order.
      innerOrderProbeCols.removeDuplicateValueIds();
    }

    ValueIdList njOuterOrder = *(ipp->getNjOuterOrder());
    // Sol 10-040303-3781. The number of entries of innerOrderProbCols(5)
    // could be greater than of njOuterOrder(3). In this case we hit ABORT
    // in Collections.cpp for unused element of njOuterOrder. The
    // following restriction on loop iteration prevents it. We need to
    // investigate details why we got innerOredrProbCols bigger than
    // njOuetrOredr iin the first place. That corresponding case will
    // be created.
    CollIndex maxInOrderCols =
        MINOF(njOuterOrder.entries(), innerOrderProbeCols.entries());
    fullyInOrder = TRUE;

    // Determine if the leading inner order column and the leading
    // column of the outer order are the same.
    ValueId innerOrderCol;
    ValueId noInverseInnerOrderCol;
    NABoolean innerOrderColIsDesc = FALSE;
    ValueId outerOrderCol;
    ValueId noInverseOuterOrderCol;
    NABoolean outerOrderColIsDesc = FALSE;

    do
    {
      // Remove any inverse node on the inner order column
      // and remember if there was one.
      innerOrderCol = innerOrderProbeCols[numInOrderCols];
      noInverseInnerOrderCol =
        innerOrderCol.getItemExpr()->removeInverseOrder()->getValueId();
      innerOrderProbeColsNoInv.insert(noInverseInnerOrderCol);
      innerOrderColIsDesc = FALSE;
      if (noInverseInnerOrderCol != innerOrderCol)
        innerOrderColIsDesc = TRUE;

      // Remove any inverse node on the leading outer order column
      // and remember if there was one.
      outerOrderCol = njOuterOrder[numInOrderCols];
      noInverseOuterOrderCol =
        outerOrderCol.getItemExpr()->removeInverseOrder()->getValueId();
      outerOrderColIsDesc = FALSE;
      if (noInverseOuterOrderCol != outerOrderCol)
        outerOrderColIsDesc = TRUE;

      // The column of the inner table sort key and the
      // the column of the outer table sort key must be
      // the same. If one is DESC, they must both be DESC.
      if ((noInverseInnerOrderCol != noInverseOuterOrderCol) OR
          (innerOrderColIsDesc != outerOrderColIsDesc))
        fullyInOrder = FALSE;
      else if (numInOrderCols == 0)
      {
        // The leading inner order column is in the same order as the
        // leading outer order column, so the probes are at least
        // partially in order. If all remaining inner order columns
        // are in order then the probes will be completely in order.
        partiallyInOrder = TRUE;
        numInOrderCols++;
      }
      else
      {
        numInOrderCols++;
      }

    } while ((numInOrderCols < maxInOrderCols) AND fullyInOrder);
    // Since there is an ipp, the probes must be at least partially in the
    // same order,  because we checked this before passing the ipp!
    if (NOT partiallyInOrder)
    {
      // The following line has been commented as it exposed a bug, after
      // the value of CQD HIST_NO_STATS_UEC was changed from 100 to 2.
      // Commented this after discussions, created a solution
      // 10-071217-9517.

//      CCMPASSERT(FALSE);
      return FALSE;
    }
  } // end if ipp exists

  // For the probes to be fully order, numInOrderCols must be same as
  // equi join key predicates and uncoveredCols should be empty
  if (fullyInOrder && (numInOrderCols==innerOrderProbeCols.entries()) &&
      uncoveredCols.isEmpty())
  {
    return TRUE;
  }
  else if (partiallyInOrder AND partiallyInOrderOK)
  {
    // Get a list of colstats. Each colstats contains the
    // histogram data for one column of the table.
    const ColStatDescList& csdl =
      indexDesc->getPrimaryTableDesc()->getTableColStats();
    const MultiColumnUecList * uecList =  csdl.getUecList();

    CostScalar multiColUec = csZero;

    CostScalar rowcount = getRelExpr().getGroupAttr()
      ->getResultCardinalityForEmptyInput();

    ValueIdList inOrderbaseColProbes;
    CollIndex baseTableColIndex;
    // get base column valueIds for elements in innerOrderProbeCols
    for (CollIndex keyColIndex = 0; keyColIndex < numInOrderCols; keyColIndex++)
    {
      // Sol 10-040303-3781. Previously we were retrieving columns from
      // innerOrderProbeCols list. The column 0 of innerOrderProbCols had the
      // type ITM_INVERSE. This type was not processed in
      // getColStatDescForColumn,
      // as a result baseTableColIndex was left uninitialized and we hit
      // ABORT in Collections.cpp. Another change - if column cannot be found
      // and getColStatDescIndexForColumn returns FALSE we should consider it
      // as situation with m=not matching columns and return FALSE as we did
      // already many times in this method.

      if (NOT csdl.getColStatDescIndexForColumn(
            baseTableColIndex, innerOrderProbeColsNoInv[keyColIndex])
         )
      {
        CCMPASSERT(FALSE);
        return FALSE;
      }
      inOrderbaseColProbes.insert(csdl[baseTableColIndex]->getColumn());
    }

    if (uecList)
      multiColUec = uecList->lookup(inOrderbaseColProbes);

    // If partial order keys have skew meaning for each key prefix
    // join returns more rows than NESTED_JOINS_LEADING_KEY_SKEW_THRESHOLD,
    // then treat it as random order and cost it very high.
    // This indirect way of accounting data skew on partitions

    // The direct way of accounting partition skew would be to divide the
    // accessed rows by number of skewed partitions instead of total partitions.

    Lng32 nj_leading_key_skew_threshold =
      (Lng32)(ActiveSchemaDB()->getDefaults()).
      getAsLong(NESTED_JOINS_LEADING_KEY_SKEW_THRESHOLD);

    if ( multiColUec.isGreaterThanZero() &&
         rowcount/multiColUec < CostScalar(nj_leading_key_skew_threshold) )
      partialOrderProbes_ = TRUE;
  } // end if partially in order

  return FALSE;

} // SimpleFileScanOptimizer::ordersMatch()

NABoolean SimpleFileScanOptimizer::isLeadingKeyColCovered()
{
  ValueIdSet allPreds;
  allPreds = getSingleSubsetPreds();
  allPreds += getRelExpr().getGroupAttr()->getCharacteristicInputs();

  NABoolean foundKey = FALSE;
  ValueIdSet allReferencedBaseCols;
  allPreds.findAllReferencedBaseCols(allReferencedBaseCols);

  CollIndex x = 0;
  const IndexDesc *iDesc = getIndexDesc();
  const ValueIdList *currentIndexSortKey = &(iDesc->getOrderOfKeyValues());

  for (x = 0; x < (*currentIndexSortKey).entries() && !foundKey; x++)
  {
    ValueId firstkey = (*currentIndexSortKey)[x];
    ItemExpr *cv;
    NABoolean isaConstant = FALSE;
    ValueId firstkeyCol;
    ColAnalysis *colA = firstkey.baseColAnalysis(&isaConstant, firstkeyCol);
    if (!colA) // no column analysis
      break; // we can't go further, break out of the loop.
    // skip computed columns and constant predicates
    if (isaConstant || firstkeyCol.isSaltColumn() ||
        firstkeyCol.isDivisioningColumn())
      continue; // try next prefix column
    if (colA->getConstValue(cv,FALSE/*useRefAConstExpr*/))
       continue; // try next prefix column

    // any predicate on first nonconstant prefix key column?
    if (allReferencedBaseCols.containsTheGivenValue(firstkeyCol))
     // nonconstant prefix key matches predicate
     foundKey = TRUE;
    else
      break; // predicate is not a key predicate, cost it high
  }

  return foundKey;
}

