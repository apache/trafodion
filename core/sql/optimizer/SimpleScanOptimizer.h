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
#ifndef _SIMPLE_SCAN_OPTIMIZER_H
#define _SIMPLE_SCAN_OPTIMIZER_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SimpleScanOptimizer.h
 * Description:  Class SimpleFileScanOptimzer
 *               Simplified Costing for Scan operators
 * Created:      04/2003
 * Language:     C++
 *
 *****************************************************************************
 */

#pragma once

#include "ScanOptimizer.h"

// -----------------------------------------------------------------------
// The class SimpleFileScanOptimizer performs several actions:
// 1.- Builds a key for the access method
// 2.- Computes the cost for the access method
// -----------------------------------------------------------------------
class SimpleFileScanOptimizer : public ScanOptimizer
{
  friend class MDAMCostWA;
  friend class MDAMOptimalDisjunctPrefixWA;
  friend class MdamTrace;
public:

  // Constructor
  //
  SimpleFileScanOptimizer(const FileScan& assocFileScan
                          ,const CostScalar& resultSetCardinality
                          ,const Context& myContext
                          ,const ValueIdSet &externalInputs);

  // Copy constructor, never called, not implemented!
  //
  SimpleFileScanOptimizer(const SimpleFileScanOptimizer &sfso, CollHeap *h=0); 

  // Destructor
  //
  virtual ~SimpleFileScanOptimizer();

  // -----------------------------------------------------------------------
  // optimize performs several actions:
  // 1.- Creates the Search key
  // 2.- Computes and returns the cost of the access method.
  // 3.- Computes the number of blocks to read per access.
  // -----------------------------------------------------------------------
  virtual Cost * optimize(SearchKey*& searchKeyPtr   // out
			  ,MdamKey*&  mdamKeyPtr     // out
			  );

  // Compute the Cost for this single subset Scan using the simple costing model.
  // OUTPUTS:
  // Return - A NON-NULL Cost* representing the Cost for this scan node
  //
  // Side-Affects - computes and sets the numberOfBlocksToReadPerAccess_
  // data member of ScanOptimizer.  This value will be captured by the
  // FileScan node and passed to DP2 by the executor, DP2 uses it to
  // decide whether it will do read ahead or not.
  //
  Cost* scmComputeCostForSingleSubset();

  // Return the cached value for single subset size
  // The size of the single subset scanned by this Scan node.
  // Computed and cached by computeSingleSubsetSize()
  //
  inline const CostScalar &getSingleSubsetSize() const
                                                  { return singleSubsetSize_; }

private:

  // Construct the Search Key (MDAM or Single Subset)
  // (for now just the Single subset is considered)
  // Return pointer to search key and cache in local datamember.
  //
  SearchKey * constructSearchKey();

  // Return the cached pointer to the single subset search key.
  // The value was cached in constructSearchKey()
  //
  inline const SearchKey *getSearchKey() const { return searchKey_; }

  // Returns True if probes are in Full order otherwise False.
  // Sets a partialy order flag if probes are partially ordered.
  NABoolean
  ordersMatch(const InputPhysicalProperty* ipp,
            const IndexDesc* indexDesc,
            const ValueIdList* innerOrder,
            const ValueIdSet& charInputs,
            NABoolean partiallyInOrderOK,
            NABoolean& probesForceSynchronousAccess) ;

  // Compute the size (in rows) of the single subset scanned by this
  // scan node.  Cache the size in a local data member and also cache the
  // key predicates that define the single subset.
  //
  void computeSingleSubsetSize();

  // Return the cached set of single subset predicates.
  // The key predicates which define the single subset. 
  // Computed and cached by computeSingleSubsetSize()
  //
  inline const ValueIdSet &getSingleSubsetPreds() const
                                                 { return singleSubsetPreds_; }
  // Does the partitioning function associated with this scan
  // represent logical sub-partitioning.
  // (If so, we must consider the extra predicates from the
  // partitioning function)
  //
  NABoolean isLogicalSubPartitioned() const;

  // does leading key column has predicate?
  NABoolean isLeadingKeyColCovered();
  
  // Get any extra key predicates from the partitioning function.  The
  // partitioning function will provide extra key predicates when it
  // represents logical sub-partitioning.
  //
  const ValueIdSet& getPartitioningKeyPredicates() const;

  // Estimate the number of blocks that DP2 needs to read per access
  // for this Scan. This value is captured by the FileScan Node and
  // passed to DP2 by the executor, DP2 uses it to decide whether it
  // will do read ahead or not.
  //
  // Side-Affects - sets the numberOfBlocksToReadPerAccess_ data member
  // of ScanOptimizer.  
  // 
  void computeNumberOfBlocksToReadPerAccess(FileScanBasicCost *basicCostPtr);

  // Compute the Cost for this single subset Scan.
  //
  // Attempts to find an existing basic cost object which can be reused.
  //
  // Computes or resuses the first row and last row cost
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
  Cost * computeCostForSingleSubset();

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
  // Return - NABoolean 
  //     - TRUE if a usable shared BasicCost object was available.
  //     - FALSE if a nonusable or new BasicCost object. 
  // *&basicCostPtr - set to the BasicCost object (shared or new)
  //
  NABoolean getSharedCostObject(FileScanBasicCost *&basicCostPtr);

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
  //
  // Returns: the number of seeks required for a single subset access to
  // a partition (this is equal to the number of index levels).
  //
  CostScalar estimateNumberOfSeeksPerScan(const CostScalar &seqKB) const;

  // Estimate the amount (in KBytes) of data read per scan (partition).
  // Include the index blocks which must be read.
  // 
  // Need to read to every index level except the root (always in cache?)
  //
  // Even if there are no rows expected in the result set, the index
  // blocks still need to be read.
  //
  // Do we need to read (some of) the index blocks twice, once for the
  // begin key and once for the end key??????
  //
  // Returns: the number of sequential KB read for a single subset access to
  // a partition (this include index and data blocks)
  //
  CostScalar estimateSeqKBReadPerScan() const;
  
  // Computes the cost vectors (First and Last Row) for this scan.
  // Computes:
  //    - KBytes
  //    - CPU Instructions
  //    -  Number of seeks
  //    - Idle time
  //    - number of probes
  // for both cost vectors.
  //
  // OUTPUTS: firstRow and lastRow SimpleCostVectors of the BasicCost
  // object are populated.
  //
  void computeCostVectors(FileScanBasicCost *basicCostPtr) const;

  // Computes the cost vectors for this scan using the simple costing model.
  // Computes:
  //    - number of tuples processed
  //    - number of tuples produced
  //    - sequential IOs
  //
  // OUTPUTS: lastRow SimpleCostVectors of the BasicCost
  // object are populated.
  //
  Cost* scmComputeCostVectors();
  Cost* scmComputeCostVectorsMultiProbes(); 
  Cost* scmComputeCostVectorsForHbase();
  Cost* scmComputeCostVectorsMultiProbesForHbase();

  // Compute the sequential KBytes accessed to produce the first and
  // last rows. 
  //
  // OUTPUTS: 
  // seqKBFR - The number of KBytes to be accessed to produce the first row.
  // seqKBLR - The number of KBytes to be accessed to produce the last row.
  //
  void estimateKB(CostScalar &seqKBFR, CostScalar &seqKBLR) const;

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
  estimateCPUInstructions(const CostScalar &seeksPerScan,
                          const CostScalar &rowsPerScanFR,
                          const CostScalar &rowsPerScanLR,
                          const CostScalar &selectedrowsPerScanFR,
                          const CostScalar &selectedrowsPerScanLR,
                          const CostScalar &seqKBFR,
                          const CostScalar &seqKBLR,
                          CostScalar &cpuInstructionsFR,
                          CostScalar &cpuInstructionsLR) const;

  // Estimate the number of CPU instructions used for each request.
  // This includes performing a binary search of each index block,
  // comparing the keys (byte compare of encoded keys) to find the
  // appropriate entry. And includes the default per request overhead.
  //
  // Return - the number of CPU instruction required to search the index
  // blocks to find the appropriate data block for this request.
  //
  CostScalar estimateCPUPerReqstInstrs() const;

  // Estimate the number of CPU instructions used to copy each row from
  // DP2 to the EXEINDP2.
  //
  // Return - the number of CPU instruction required to copy each row from
  // DP2 to the EXEINDP2.
  //
  CostScalar estimateCPUPerRowInstrs() const;

  // Estimate the per-row (of single subset) number of CPU instruction
  // required to evaluate the executor predicates
  //
  CostScalar estimateCPUExePredPerRowInstrs() const;

  // Estimate the per-seek CPU instructions.  This is the extra CPU cost
  // necessary for each seek.  For now this is just a default value.
  //
  CostScalar estimateCPUPerSeekInstrs() const;

  // Estimate the per-KB CPU instructions.  This is the extra CPU cost
  // necessary to copy each KB from DP2 to the EXEINDP2.  For now this
  // is just a default value.
  //
  CostScalar estimateCPUPerKBInstrs() const;

  // Estimate the amount of Idle time used by this Scan.
  //
  // CPUCOST_SUBSET_OPEN lumps together all the overhead needed to
  // set-up the access to each partition. Thus it is a blocking cost,
  // nothing can overlap with it.  Since scans are not blocking, by
  // definition, put the cost in idle time: this is the cost for
  // opening all the partitions but the first partition.  The first
  // partition is opened before the ScanOptimizer is called. During
  // opening the first partition, the necessary info on the file is
  // also acquired so it is more expensive and cannot be overlaid.
  // Root accounts for the cost of opening the first partition of all
  // the tables.
  //
  CostScalar estimateIdleTime() const;

  // Compute the number of blocks needed to hold the given number of
  // rows for this scan.  Assume that the rows are dense (no
  // fragmentation or slack).  Assume that there is a partial block at
  // each end of the subset.  This may overestimate the number of
  // blocks by 1, especially when there are very few rows.
  //
  CostScalar getNumBlocksForRows(const CostScalar &numRows) const;

  // Compute the estimated number of rows per scan (active partition),
  // based on the result set cardinality computed outside of the Scan
  // Optimizer and the number of active partitions for this Scan.
  //
  CostScalar getResultSetCardinalityPerScan() const;

  // Compute the number of rows of this Scan that will fit in a DP2 Buffer.
  //
  CostScalar getRowsInDP2Buffer() const;

  // Return the cached value of the recordSizeinKB for this Scan.  Value
  // was initialized from the IndexDesc of the associated FileScan.
  //
  inline const CostScalar &getRecordSizeInKb() const { return recordSizeInKb_;}

  // Return the cached value of the blockSizeinKB for this Scan.  Value
  // was initialized from the IndexDesc of the associated FileScan.
  //
  inline const CostScalar &getBlockSizeInKb() const { return blockSizeInKb_; }

  // Return the repeat Count for this scan.  The repeat count is an
  // estimate of the number of probes that this scan (partition) will
  // receive.
  // A return value of >= 1.0 indicates that this is a multiprobe scan
  // (below a nested join).
  // A return value of csZero (0.0) indicates that this is a single
  // probe scan.
  //
  CostScalar getRepeatCount() const;

  // Return the cached value of the estimatedRecordsPerBlock_ for this
  // Scan.  Value was initialized from the IndexDesc of the associated
  // FileScan.
  //
  inline const CostScalar &getEstimatedRecordsPerBlock() const
                                         { return estimatedRecordsPerBlock_; }

  // Addtional methods for Multiprobe scans
  //

  // Estimate the of seeks required for this all probes per scan
  // (partition).  Include the seeks required for accessing the index
  // blocks.
  //
  // Need to Seek to every index level except the root, plus need to
  // seek to first data block.
  //
  // Do we need to seek twice this amount, once for the begin key and
  // once for the end key??????
  //
  // Returns: the number of seeks required for all probes to access a
  // partition
  //
  CostScalar estimateNumberOfSeeksPerScanMultiProbe() const;

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
  CostScalar estimateSeqKBReadPerScanMultiProbe() const;

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
  void computeCostVectorsMultiProbes(FileScanBasicCost *basicCostPtr);

  // SimpleFileScanOptimizer::categorizeMultiProbes()
  // Computes and caches the following metrics regarding probes:
  //
  //  probes_ - the total number of probes for all active partitions.
  //
  //  successfulProbes_ - the number of probes (probes_) that produce
  //  some data.
  //
  //  uniqueProbes_ - the number of distinct probes (probes_).  Includes
  //  successful and failed probes.
  //
  //  duplicateSuccProbes_ - the number of successful probes
  //  (successfulProbes_) that are not unique.  duplicateSuccProbes =
  //  successfulProbes - uniqueSuccProbes.
  //
  //  failedProbes_ - the number of probes (probes_) that do not result
  //  in data.  failedProbes = probes - successfulProbes.
  //
  //  dataRows_ - the number of rows produced by all successful probes.
  //  blksPerSuccProbe_ - the estimated number of blocks produced by
  //  each successful probe on average.
  //
  //  inOrderProbesFlag - indicates if the probes arrive in order
  //  relative to the clustering key of the access path.
  //
  // Return Value: void - Computed values are cached in data members.
  //
  void categorizeMultiProbes(NABoolean *isAnIndexJoin=NULL);


  // -----------------------------------------------------------------------
  // This is taken from ScanOptimizer.h
  // The intension is to change categorizaProbes method for SimpleScanOptimizer
  // while keeping the old version for ScanOptimizer, so it couls be used to
  // compare the old and new logic. Th rational for this is to have simpler
  // logic of categorizing probes for SimpleScanOptimizer.
  //
  // Use isMDAM = FALSE when using this in single subset
  // Use isMDAM = TRUE when using this in MDAM. In this case
  //              the routine will assume that we are not in an
  //              index join (it should be the case because
  //              code in the early stages should make sure that
  //              we don't attempt MDAM in an index join). This is
  //              not a correctness issue, it is an efficiency issue.
  // -----------------------------------------------------------------------
  void
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
	           ,NABoolean * isAnIndexJoin=NULL
		   ) const;

  // Estimate the number of different index blocks visited for 
  // MultiProbe Scan. This method will use the number of unique probes
  // and total key length of index columns instead of universal "rule 
  // of thumb" from IndexDesc::getEstimatedIndexBlocksLowerBound()
  void estimateIndexBlockUsageMultiProbeScan();

  // Estimate the real and effective total row count for the table.  The
  // Effective Total Row Count is the size of the bounding subset of
  // all probes.  Typically this will be all the rows of the table,
  // but if all probes are restricted to a subset of rows (e.g. the
  // key predicate contains leading constants) the the effective row
  // count will be less than the total row count.
  //
  // Returns: The estimated real and effective row count.
  //
  void estimateEffTotalRowCount(CostScalar &realRowCount,
                                CostScalar &effRowCount) const;

  // Compute the sequential KBytes accessed to produce the first and
  // last rows. 
  //
  // OUTPUTS: 
  // seqKBFR - The number of KBytes to be accessed to produce the first row.
  // seqKBLR - The number of KBytes to be accessed to produce the last row.
  //
  void estimateKBMultiProbe(CostScalar &seqKBFR, CostScalar &seqKBLR) const;

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
  void estimateCPUInstructionsMultiProbe(
                  const CostScalar &seeksPerScan,
                  const CostScalar &rowsPerScanFR,
                  const CostScalar &rowsPerScanLR,
                  const CostScalar &selectedRowsPerScanFR,
                  const CostScalar &selectedRowsPerScanLR,
                  const CostScalar &seqKBFR,
                  const CostScalar &seqKBLR,
                  CostScalar &cpuInstructionsFR,
                  CostScalar &cpuInstructionsLR) const;

  CostScalar getProbeCacheCostAdjFactor() const;
  NABoolean  isProbeCacheApplicable() const;

  // Private Data members -
  // Cache these values from IndexDesc in this class.
  //
  CostScalar recordSizeInKb_;
  CostScalar blockSizeInKb_;
  CostScalar estimatedRecordsPerBlock_;

  // Compute these values during computeSingleSubsetSize()
  //
  // The size of the single subset scanned by this Scan node.
  //
  CostScalar singleSubsetSize_;

  // The key predicates which define the single subset.
  //
  ValueIdSet singleSubsetPreds_;

  // Will be constructed during constructSearchKey() 
  // This search key is not owned by this object and will therefore
  // not be destroyed when the destructor of this object is called.
  // This search key will be returned to the caller
  //
  SearchKey *searchKey_;

  
  // Private inline accessors to local datamembers
  // For Multiprobe scans
  //

  inline const CostScalar getFailedProbes() const     { return failedProbes_; }

  // Return the cached value of number of data rows.  This is the
  // number of rows produced by all successful probes.
  // The value was cached in categorizeMultiProbes().
  // 
  // For MultiProbe Scans
  //
  inline const CostScalar &getDataRows() const            { return dataRows_; }

  // Return the cached value of effective total row count.  This is the
  // size of the bounding subset of all probes.  Typically this will be
  // all the rows of the table, but if all probes are restricted to a
  // subset of rows (e.g. the key predicate contains leading constants)
  // then the effective row count will be less than the total row count.
  // The value was cached in computeCostVectorsMultiProbes().
  //
  //  For MultiProbe Scans
  //
  inline const CostScalar &getEffectiveTotalRowCount() const
                                            { return effectiveTotalRowCount_; }

  // Return the cached value of real total row count.
  // The value was cached in computeCostVectorsMultiProbes().
  //
  //  For MultiProbe Scans
  //
  inline const CostScalar &getTotalRowCount() const { return totalRowCount_; }

  // Return the cached value of the lower bound of the number of index
  // blocks for the table. This value is the estimate of the number of
  // blocks all probes touch in their way down in every level of the
  // B-tree. see IndexDesc::getEstimatedIndexBlocksLowerBound().
  // The value was cached in computeCostVectorsMultiProbes().
  //
  //  For MultiProbe Scans
  //
  inline const CostScalar &getIndexBlocksLowerBound() const
                                            { return indexBlocksLowerBound_; }

  // Return the cached value of number of blocks for each successful
  // probe.  This is the estimated number of blocks produced by each
  // successful probe on average.
  // The value was cached in categorizeMultiProbes().
  // 
  // For MultiProbe Scans
  //
  inline const CostScalar &getBlksPerSuccProbe() const
                                                 { return blksPerSuccProbe_; }

  // Return the value of partialy order probes flag.
  // The value was cached in orderMatch().
  inline const NABoolean  getPartialOrderProbesFlag() const
                                               { return partialOrderProbes_; }

  // This is the number of probes (probes_) that do not result in
  // data.  failedProbes = probes - successfulProbes.  The value is
  // cached in categorizeMultiProbes().
  // 
  // For MultiProbe Scans
  //
  CostScalar failedProbes_;

  // This is the number of rows produced by all successful probes.
  // The value is cached in categorizeMultiProbes().
  // 
  // For MultiProbe Scans
  //
  CostScalar dataRows_;

  // This is the size of the bounding subset of all probes.  Typically
  // this will be all the rows of the table, but if all probes are
  // restricted to a subset of rows (e.g. the key predicate contains
  // leading constants) then the effective row count will be less than
  // the total row count.  The value is cached in
  // computeCostVectorsMultiProbes().
  //
  //  For MultiProbe Scans
  //
  CostScalar effectiveTotalRowCount_;

  // The total row count of the inner table.
  //
  //  For MultiProbe Scans
  //
  CostScalar totalRowCount_;

  // This value is the estimate of the number of blocks all probes
  // touch on their way down in every level of the B-tree. see
  // IndexDesc::getEstimatedIndexBlocksLowerBound().  The value is
  // cached in computeCostVectorsMultiProbes().
  //
  //  For MultiProbe Scans
  //
  CostScalar indexBlocksLowerBound_;

  // This is the estimated number of blocks produced by each
  // successful probe on average.  The value is cached in
  // categorizeMultiProbes().
  // 
  // For MultiProbe Scans
  //
  CostScalar blksPerSuccProbe_;

  // This is total UEC of the key prefix for the Right child.
  CostScalar keyPrefixUEC_;

  // Indicates if the probes are partially in order.
  NABoolean partialOrderProbes_;

}; // class SimpleFileScanOptimizer

#endif
