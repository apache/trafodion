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

#include "PartKeyDist.h"   
#include "hs_const.h"     /* HS_MAX_BOUNDARY_LEN */
#include "parser.h"      /* for the parser of EncodedValues */


// -----------------------------------------------------------------------
//  methods on PartitionKeyDistribution class -- just ctors for now
// -----------------------------------------------------------------------

PartitionKeyDistribution::PartitionKeyDistribution () 
     : partitionBoundaries_ (HistogramSharedPtr(0), HISTHEAP), /* init to invalid value */
       partitionFactors_ (HISTHEAP, 0),       /* init to invalid value */
       partitionKeyId_ (NULL_VALUE_ID),       /* init to invalid value */
       objectInfoIsValid_ (FALSE)             /* init to invalid value */
{} 


PartitionKeyDistribution::PartitionKeyDistribution (
     const RangePartitioningFunction & partFunc,
     const ColStatDescList           & inputHistograms)
     : partitionBoundaries_ (HistogramSharedPtr(0), HISTHEAP), /* init to invalid value */
       partitionFactors_ (HISTHEAP, 0),       /* init to invalid value */
       partitionKeyId_ (NULL_VALUE_ID),       /* init to invalid value */
       objectInfoIsValid_ (FALSE)             /* init to invalid value */
{

  // -----------------------------------------------------------------------
  //  Get the partition boundaries:
  // -----------------------------------------------------------------------
  NAList<EncodedValueList *> boundaries(CmpCommon::statementHeap());
  const RangePartitionBoundaries *rpBoundariesPtr = partFunc.getRangePartitionBoundaries();

  // There are n+1 partition boundaries for n partitions:
  const CollIndex numPartBounds = CollIndex(partFunc.getCountOfPartitions()+1);
  for (CollIndex i=0; i < numPartBounds; i++)
    {
      EncodedValueList *evl = new (CmpCommon::statementHeap()) 
                              EncodedValueList(CmpCommon::statementHeap(), 0);

      boundaries.insertAt(i, evl);
      const ItemExprList *boundary = rpBoundariesPtr->getBoundaryValues(i);

      // transform to encoded value list
      for (CollIndex j=0; j < boundary->entries(); j++)
        {
          EncodedValue ev((*boundary)[j], FALSE /* "negate" */);
          (boundaries[i])->insertAt(j,ev);
        }
    }

  const ValueIdList listOfPartitionKeys = partFunc.getKeyColumnList();
  const ValueIdList listOfPartitionKeyOrders = partFunc.getOrderOfKeyValues() ;

  // CSDL::divideHistogramAtPartitionBoundaries() returns FALSE if, for
  // some reason, the histogram-to-partition-boundary-list mapping fails
  //
  // if it returns TRUE, then we know that the PartitionKeyDistribution
  // data members partitionBoundaries_ and partitionKeyId_ have been
  // set to valid values.

  if ( inputHistograms.divideHistogramAtPartitionBoundaries (
       listOfPartitionKeys,      /* in  */
       listOfPartitionKeyOrders, /* in  */
       boundaries,               /* in  */
       partitionKeyId_,          /* out */
       isPartitionKeyAscending_, /* out */
       partitionBoundaries_,     /* out */
       partitionFactors_         /* out */ ) == TRUE )
    {
      objectInfoIsValid_ = TRUE ; 
    }

} // PartitionKeyDistribution::PartitionKeyDistribution 

CollIndex
PartitionKeyDistribution::getHistIdxFromPartIdx (CollIndex extIdx) const
{
  // Since our "internal" representation of the partition boundaries
  // doesn't (necessarily) share the same numbering scheme as the
  // "external" world's view of the partition boundaries, we need to
  // be able to convert from the "external" to "internal" view.
  //
  // Note that there is a n-to-1 mapping between the "external" partitions
  // and the "internal" histogram intervals.  Thus, we can convert exactly
  // from "external" to "internal", but not the other way (a *range* of
  // partitions could be returned, if this functionality is required
  // ... but I don't think it is).
  //
  // Note also that if the partitioning key is in DESCENDING order (a
  // value of FALSE stored in isPartitionKeyAscending_), then we need
  // to provide the "inverse" hist idx.  See PartKeyDist.h for a careful
  // description of what's going on.

  DCMPASSERT(extIdx >= 0 AND extIdx < getNumPartitions());

  CollIndex countParts, interval ;
  

  if ( isPartitionKeyAscending_ == FALSE )
    {
      CollIndex numParts = getNumPartitions() ;
      // now "flip-flop", since we reversed the boundaries, so we have to
      // "reverse" everything we do when talking about them 
      extIdx = numParts - extIdx - 1; 
    }

  extIdx += 1 ; // we're zero-based when talking about Partition#
  countParts = 0 ; 

  for ( interval = 1 ; interval < partitionFactors_.entries() ; interval++ )
    {
      countParts += partitionFactors_[interval] ;
      if ( countParts >= extIdx )
        break ;
    }

  DCMPASSERT ( countParts >= extIdx ) ; // if not, the index was bad! (or logic wrong!!!)

  // "interval" contains the HistInt index corresponding to the partition
  // requested; OR, it contains the HistInt corresponding to a number of
  // partitions, one of which is the one that was requested.
  return interval ; 
}


CostScalar
PartitionKeyDistribution::getRowsForPartition(CollIndex extIdx) const 
{
  // The first entry in the histogram will never contain rows because
  // it corresponds to a "partition" for all rows below "MIN".
  // No rows satisfy this. Thus, the first partition is described
  // by the first interval.
  DCMPASSERT(extIdx >= 0 AND extIdx < getNumPartitions());
  
  // remember : each entry in the histogram represents potentially many 
  // different partition boundaries, each with the same partition boundary value.
  CollIndex interval = getHistIdxFromPartIdx (extIdx) ;

  // If there is more than one partition represented by this histogram
  // interval, then we assume that all of the rows in this histogram
  // interval are uniformly distributed between all of them.
  return (*Hist())[interval].getCardinality() / partitionFactors_[interval] ;
}

CostScalar
PartitionKeyDistribution::getUecForPartition (CollIndex extIdx) const 
{
  // remember : each entry in the histogram represents potentially many 
  // different partition boundaries, each with the same partition boundary value.
  CollIndex interval = getHistIdxFromPartIdx (extIdx) ;

  // If there is more than one partition represented by this histogram
  // interval, then we assume that all of the uec in this histogram
  // interval are uniformly distributed between all of them.
  return (*Hist())[interval].getUec() / partitionFactors_[interval];
}

CollIndex
PartitionKeyDistribution::getNumPartitions () const
{
  // remember : each entry in the histogram represents potentially many 
  // different partition boundaries, each with the same partition boundary value.
  CollIndex numParts = 0 ; 

  // don't count that first HistInt's counter 
  for ( CollIndex i = 1 ; i < partitionFactors_.entries() ; i++ )
    numParts += partitionFactors_[i] ; 

  // CollIndex numParts = Hist()->entries() - 1;
  return numParts;
}
 
CollIndex
PartitionKeyDistribution::getMaxPartitionFactor() const
{

  CollIndex max = 0;
  for ( CollIndex i = 1 ; i < partitionFactors_.entries() ; i++ )
    {
      if (partitionFactors_[i] > max)
        max = partitionFactors_[i];
    }

  return max;
}
  
// eof
