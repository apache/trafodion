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
#ifndef PARTITION_KEY_DISTRIBUTION_H
#define PARTITION_KEY_DISTRIBUTION_H
/* -*-C++-*-

****************************************************************************
*
* File:         PartKeyDist.h
* Description:
*
* Created:      8/5/1998
* Language:     C++
*
*
**************************************************************************** */

#include "ColStatDesc.h"   /* histogram classes */
#include "PartFunc.h"      /* partitioning function */

// -----------------------------------------------------------------------
//  The following classes are defined in this file.
// -----------------------------------------------------------------------
class PartitionKeyDistribution ; // used to model active partitions



// -----------------------------------------------------------------------
// class PartitionKeyDistribution : 
// --------------------------------
// representation of partitioning key boundary information inside a histogram, 
// in order to model the number of active partitions during a query; read-only
// wrapper for constructed ColStats object; primary work (deeply embedded
// inside ctor) is calls to Histogram::createMergeTemplate() and
// ColStats::populateTemplate()
//
// PartitionKeyDistribution is intended to represent a list of partition
// boundaries, repesenting how a particular table is partitioned between
// disks.
//
// $$$ In all methods of this class, we must be very careful to never call 
// $$$
// $$$   ColStats::removeRedundantEmpties()
// $$$
// $$$ because then we might lose some partition boundary information!
//
// Note that another way of implementing this class would be to embed a 
// ColStatDescList inside a wrapper class, and through that wrapper allow
// manipulation of the CSDL.  However, this approach will not work for
// several reasons, due to the way that histogram synthesis is performed.
//
// Basically, whenever a predicate is applied to a histogram, we end up
// removing intervals from that histogram.  But the semantics of the 
// partition boundaries object says that its boundaries must never be
// modified.  Similarly, whenever we call the ColStats routine
// mentioned above (removeRedundantEmpties), any intervals which are empty
// are merged together, thus destroying important information!
//
// It is for this reason that this class has to be used AFTER any
// necessary histogram synthesis is performed.  If before-and-after
// comparison is required, then two different objects of type
// PartitionKeyDistribution must be created.
//
//
// $$$ Another trickiness:
// $$$
// $$$ All of the internal histogram code assumes, and depends, on the fact
// $$$ that all histogram interval boundaries are in ASCENDING order.  If the
// $$$ partition boundaries represent a clustering key which is in DESCENDING
// $$$ order, then the PartitionKeyDistribution class needs to do some
// $$$ careful interfacing with the histogram code.
// $$$
// $$$ 1. when the internal histogram is created, the order of the boundaries
// $$$    provided to the PartitionKeyDistribution have to be inserted in
// $$$    the inverse order from what is provided;
// $$$
// $$$ 2. any PartitionKeyDistribution accessor method (e.g., 
// $$$    getRowsForPartition()) which needs to know about a specific partition
// $$$    number, needs to check the internal flag (isPartitionKeyAscending_);
// $$$    if this flag is FALSE, signifying DESCENDING order, then we need to
// $$$    return the information for the partition signified by the 
// $$$    (numPartitions-i-1), not the ith partition.  I.e., we need to do
// $$$    another flip-flop.
// $$$
// $$$ The function which does (1) is the PartitionKeyDistribution ctor;
// $$$
// $$$ The functions which have to do (2) are currently only:
// $$$
// $$$   . getRowsForPartition()
// $$$   . getUecForPartition()
// $$$
// $$$ However, since both of these routines use
// $$$
// $$$   . getHistIdxFromPartIdx()
// $$$
// $$$ this code change only needs to be done in a single place.
//
//
// Use of class PartitionKeyDistribution
// -------------------------------------
//
// Before using an object of this class after it's been constructed, it's
// necessary to consult whether the object isValid() ; if not, then the
// values contained in the class are bogus and should not (cannot) be
// used.
//
// This class contains only those methods which its users require; more can
// be added as needed.
//
//   isValid() : described above -- if FALSE, this object cannot be used!
//
//   getRowcount() : the total number of rows in all partitions
//
//   getRowsForPartition() : the rowcount in the i'th partition
//
//   getUecForPartition() : the uec in the i'th partition
//
//   getNumPartitions() : the total number of partitions, empty+full
//
//   getNumActivePartitions() : the number of partitions containing rows
//
//   getPartitionKey() : the key column (one of 'listOfPartitionKeys') being used
//     as the leading partitioning key -- the one we used to create the
//     partitioned boundary list (which is one of 'listOfPartitionBounds')
// -----------------------------------------------------------------------

class PartitionKeyDistribution
{
public:
  // "empty ctor" : the empty PKD, useful as a comparison with real ones, ...
  PartitionKeyDistribution () ;  

  // "new ctor" : this is the interface we want, but it's currently unwritten, as
  // it's not immediately clear how to convert from the RangePartitioningFunction
  // to the LIST(EncodedValueList) & LIST(ValueId)
  PartitionKeyDistribution (const RangePartitioningFunction & partFunc,
                            const ColStatDescList           & inputHistograms); 

  inline CostScalar getRowcount      () const { return CS().getRowcount() ; }
  CollIndex         getNumPartitions () const;
  inline ValueId    getPartitionKey  () const { return partitionKeyId_    ; }
  inline NABoolean  isValid          () const { return objectInfoIsValid_ ; }
  
  // i=0 is the first partition and so on...
  CostScalar getRowsForPartition (CollIndex i) const; 

  CostScalar getUecForPartition  (CollIndex i) const;


  // iter through the histogram, count up the number of intervals that
  // have >0 rows
  inline CollIndex getNumActivePartitions() const
  {
    HistogramSharedPtr hist = Hist() ; 
    CollIndex entries = hist->entries(), numActive = 0 ; 
    for ( CollIndex i = 0 ; i < entries ; i++ )
      {
        if ( (*hist)[i].getCardinality() > 0 ) 
          {
            // remember: each Histogram interval represents potentially many
            // partition boundaries
            numActive += partitionFactors_[i] ; 
          }
      }
    return numActive ; 
  }

  // -----------------------------------------------------------------------
  //  This is used for the row count estimation code. It returns the partitions
  //  in the interval holding the most partitions.
  // -----------------------------------------------------------------------
  CollIndex getMaxPartitionFactor() const;

private:
  // partitionBoundaries_ is a ColStats, whose internal Histogram encodes
  // information about the partitions' boundary values
  ColStats      partitionBoundaries_ ;
  // partitionFactors_ is a LIST the same size as the internal Histogram
  // inside of partitionBoundaries_; this factor is used to compensate for
  // various Histogram methods' inability to handle the case where the same
  // partition boundary exists for multiple consecutive partition boundaries.
  //
  // --> The rowcount/uec for each interval inside partitionBoundares_ actually
  // represents the rows/uec for potenially *many* partitions.
  //
  // Maintained semantics: 
  // ---------------------
  // 1. (partitionFactors_.entries() == Hist->entries())
  // 2. for all entries, partitionFactors_[i] >= 1 
  // 3. The values in partitionFactor_ must not be altered after the initial setup
  //    inside the PartKeyDist ctor.
  //
  CollIndexList partitionFactors_ ; 
  // this routine does the work of converting from the "external" partition #
  // to the "internal" histogram interval #
  CollIndex getHistIdxFromPartIdx (CollIndex i) const ;

  // partitioning key corresponding to the boundary values
  ValueId   partitionKeyId_ ;      
  // what's the order of 'partitionKeyId_' : ASC (TRUE) or DESC (FALSE)
  NABoolean isPartitionKeyAscending_ ;   

  NABoolean objectInfoIsValid_ ;   // is the current object valid?

  // accessor functions to make writing this class's functions easier
  inline const ColStats  & CS   () const { return partitionBoundaries_ ; }
  inline HistogramSharedPtr Hist () const { return partitionBoundaries_.getHistogram() ; }

  // do not use the following ctor or assignment!
  PartitionKeyDistribution (const PartitionKeyDistribution &) ; 
  PartitionKeyDistribution & operator = (const PartitionKeyDistribution &) ; 
};


#endif /* PARTITION_KEY_DISTRIBUTION_H */
