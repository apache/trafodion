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
* File:         Stats.cpp
* Description:  This file includes the source file for statistics
*               related information.
*
* Created:      3/16/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

#include <string.h>
#include "Stats.h"
#include "Sqlcomp.h"
#include "ItemColRef.h"
#include "opt.h"
#include "Analyzer.h"
#include "Cost.h"
#include "CompException.h"
#include "NLSConversion.h" // For conversion to unicode strings
#include "ComCextdecs.h" // For Timestamp related calls
#include <queue>
#include "QCache.h"

#include "exp_function.h"

// Specify the format for printing a Int64
#define FMT_INT64 PF64

// -----------------------------------------------------------------------
//  methods on HistInt class
// -----------------------------------------------------------------------
HistInt::HistInt(Int32 intNum, const NAWchar *intBoundary, const NAColumnArray &columns,
		 CostScalar card, CostScalar uec, NABoolean boundInc, CostScalar card2mfv)
     : rows_(card),
       uec_(uec),
       boundInc_(boundInc),
       hash_(0),
       rows2mfv_(card2mfv),
       MCBoundary_(STMTHEAP)
{
   if(intBoundary)
   {
      EncodedValue ev(intBoundary, columns, NULL /* do not care the cv values */ );
      boundary_ = ev;

      // construct the MC encoded boundary value
      if (columns.entries() > 1)
      {
          setupMCBoundary ();
      }
   }
   else
	   boundary_ = UNINIT_ENCODEDVALUE;
}

// setup the multi-column boundary value for this HistInt
void
HistInt::setupMCBoundary ()
{
   if (CmpCommon::getDefault(HBASE_RANGE_PARTITIONING_MC_SPLIT) == DF_ON)
   {
      const NormValueList* nvl = boundary_.getValueList();

      if (nvl && (nvl->entries () > 1))
      {
         for (Int32 i=0; i < nvl->entries(); i++)
         {
           EncodedValue ev;
           ev.setValue(nvl->at(i));
           MCBoundary_.insert(ev);
         }
      }
   }
}

void
HistInt::copy (const HistInt& other)
{
  boundary_ = other.boundary_;
  rows_     = other.rows_;
  uec_      = other.uec_;
  boundInc_ = other.boundInc_;
  hash_     = other.hash_;
  rows2mfv_  = other.rows2mfv_;
  MCBoundary_ = other.MCBoundary_;
}

// the following is used to maintain the semantic : uec <= rows
void
HistInt::setCardAndUec (CostScalar card, CostScalar uec)
{
  //10-040430-5649-begin
  //These lines were previously commented out as setCardinality
  //and setUec did rounding of card and uec values anyway.
  //But,Under rare cases the compiler crashed in MINOF macro
  //While handling extreamly low values, so it became necessary
  //to round these values before we use them.
  card.roundIfZero() ;
  uec.roundIfZero() ;
  //10-040430-5649-end
  setCardinality(card) ;
  setUec (MINOF(card,uec)) ;
}

void HistInt::setCardinality (CostScalar card)
{
  if (card < csZero)
  {
    // min cardinality of an interval is zero
    CCMPASSERT (card >= csZero) ;
    card = csZero;
  }
  card.roundIfZero();
  rows_ = card ;
}

void HistInt::setCardinality2mfv (CostScalar card)
{
  if (card < csZero)
  {
    // min cardinality of 2mfv is zero
    CCMPASSERT (card >= csZero) ;
    card = csZero;
  }
  card.roundIfZero();
  rows2mfv_ = card ;
}

void HistInt::setUec (CostScalar uec)
{
  if (uec < csZero)
  {
     // min UEC of an interval is zero
     CCMPASSERT (uec >= csZero)  ;
     uec = csZero;
  }
  uec.roundIfZero();
  uec_ = uec ;
}

// ---------------------------------------------------------------------
// HistInt::mergeInterval, merges the left and right HistInts based 
// on the mergeMethod. This is a helper method for ColStats::mergeColStats
// ----------------------------------------------------------------------
CostScalar 
HistInt::mergeInterval(const HistInt & left,
                     const HistInt & right,
                     CostScalar scaleRowCount,
                     MergeType mergeMethod)

{
  CostScalar numRows = csZero;
  CostScalar numUec, numFudgedUec;
  const CostScalar leftUEC        = left.getUec();
  const CostScalar leftRowCount   = left.getCardinality();
  const CostScalar rightUEC       = right.getUec();
  const CostScalar rightRowCount  = right.getCardinality();

  const CostScalar maxUEC = MAXOF (leftUEC, rightUEC) ;
  const CostScalar minUEC = MINOF (leftUEC, rightUEC) ;


  // now, interpolate the new uec and rowcount for this interval
  switch (mergeMethod)
    {
    case INNER_JOIN_MERGE:
    case OUTER_JOIN_MERGE:  /* for equijoin portion of outer join */
      numUec = minUEC ;

      if (numUec.isGreaterThanZero() AND scaleRowCount.isGreaterThanZero() )
      {
        const CostScalar lRowperMaxuec = leftRowCount / maxUEC;
        const CostScalar rRowperScale  = rightRowCount / scaleRowCount;
        numRows = lRowperMaxuec * rRowperScale;
      }
      break;

    case SEMI_JOIN_MERGE:
      numUec = minUEC ;

      if (numUec.isGreaterThanZero()) // implies leftUEC > 0, no div-zero possibility
        {
          numRows = leftRowCount * ( numUec / leftUEC);
        }
      break;

  case ANTI_SEMI_JOIN_MERGE:
      numUec = MAXOF((CostScalar)CostPrimitives::getBasicCostFactor( HIST_DEFAULT_SEL_FOR_JOIN_EQUAL ) * leftUEC, 
                     leftUEC - rightUEC) ;
      if (numUec.isGreaterThanZero()) // implies leftUEC > 0, no div-zero possibility
        numRows = leftRowCount * ( numUec / leftUEC ) ;

    break ;

    case LEFT_JOIN_OR_MERGE:

      // After the result of the inner join portion of an Outer Join is
      // known, one needs to do something like an OR between that inner
      // join result (*this) and the original pre-join column's histogram
      // (*otherStats), to calculate the actual outer join result.
      //
      // The UEC is always that of the original (right/other) table.
      //  (properly scaled)
      if (rightUEC.isZero())
        numUec = 0;
      else
        numUec = rightUEC;

      numFudgedUec = MIN_ONE (numUec) ;

      // The rowCount varies on a case by case basis
      if (leftUEC.isZero())
        {
          // if innerjoin result has no rows, all rows are from original
          numRows = rightRowCount;
        }
      else 
        {
          // else result is all innerjoin rows + original unmatched rows
          numRows = leftRowCount +
            ((rightRowCount / numFudgedUec) * (numUec - leftUEC));

          // guarantee rowCount and UEC is never less than it was originally.
          //  (the above formula can/will improperly decrease it)
          numRows = MAXOF (numRows, rightRowCount) ;
        }
      break;

    case UNION_MERGE:
      numUec = maxUEC ;
      numRows = leftRowCount + rightRowCount;
      break;

    case OR_MERGE:
      numUec = maxUEC ;
      numRows = MAXOF( leftRowCount, rightRowCount );
      break;

    case AND_MERGE:
      numUec = minUEC ;
      numRows = MINOF( leftRowCount, rightRowCount );
      break;

    default:
      break ;
    } // switch (mergeMethod)

  // prevent UEC from exceeding rowCount....
  if ( numUec > numRows )
    numUec = numRows;

  this->setCardAndUec (numRows, numUec);
  return maxUEC;
} // mergeInterval

void
HistInt::display (FILE *f, const char * prefix, const char * suffix,
                  CollHeap *c, char *buf) const
{
  Space * space = (Space *)c;
  char mybuf[1000];
  snprintf(mybuf, sizeof(mybuf), "%sBound  ", prefix);
  PRINTIT(f, c, space, buf, mybuf);

  if (boundInc_)
  {
    sprintf(mybuf, "<= ");
    PRINTIT(f, c, space, buf, mybuf);
  }
  else
  {
    sprintf(mybuf, "<  ");
    PRINTIT(f, c, space, buf, mybuf);
  }
  boundary_.display(f, prefix, suffix, c, buf);
  snprintf(mybuf, sizeof(mybuf), " : rows=%f,uec=%f %s\n",
	   rows_.value(), uec_.value(), suffix);
  PRINTIT(f, c, space, buf, mybuf);
}

// -----------------------------------------------------------------------
//  methods on Interval "wrapper class"
// -----------------------------------------------------------------------

//
// Here is what Intervals look like :
//
// HistInts:
//
//#    0    1    2    3    4    5
//
//row  0    2    0    3    1    2
//uec  0    3    0    1    2    3
//
//val  1    2    4    4    5    7
//     |    |    |_3__|    |    |
//     |_2__|    |    |    |_2__|
//     |    |    |    |_1__|    |
//     |    |_0__|    |    |    |
//
//       I1   I2   I3   I4   I5
//
//row    2    0    3    1    2
//uec    2    0    1    1    2
//hi     2    4    4    5    7
//lo     1    2    4    4    5
//
// I1..I5 are the Intervals corresponding to
// the underlying HistInts
// --> I assert it's easier to work with Intervals
//     than HistInts, since they're what we're actually
//     concerned with -- the intervals between HistInt
//     boundaries (the "bars" in a histogram), not the
//     HistInts themselves
//
// So, Interval N lies between (*hist_)[N] and (*hist_)[N+1]

// ----------------------------------------------------------------------
// Interval::containsAFrequentValue 
// Does this interval contain a frequent value. The answer is YES if
// the UEC of that interval is 1 and if the rowcount of that interval
// is twice the average frequency of the histogram to which this column
// belongs
// ----------------------------------------------------------------------
NABoolean Interval::containsAFrequentValue(const CostScalar & thresholdFreq) const
{
   if (( getUec() <= 1.0 ) && (getRowcount() >= thresholdFreq))
     return TRUE;
   else
    return FALSE;  
}

// -----------------------------------------------------------------------
// merge two Intervals into one
// --> for simplicity, we only merge low-to-high, so the
//     OTHER interval must come directly after THIS interval
// -----------------------------------------------------------------------
NABoolean
Interval::merge (Interval & other)
{
  // if the intervals are not valid, return without merging
  if (!OK() || ! other.OK()) return FALSE;

  // for simplicity, we only merge low-to-high
  if (loIndex_+1 != other.loIndex_ )
  {
    CCMPASSERT ( loIndex_+1 == other.loIndex_ ) ;
    return FALSE;
  }

  CostScalar newUec  = getUec()      + other.getUec() ;
  CostScalar newRows = getRowcount() + other.getRowcount() ;

  hist_->removeAt(loIndex_+1) ;

  other.setInvalid() ;

  setRowsAndUec (newRows, newUec) ;
  return TRUE;
}

// -----------------------------------------------------------------------
// returns TRUE when the Interval is not up to specs
// (i.e., rowcount/uec is >0 and <1)
// -----------------------------------------------------------------------
NABoolean
Interval::canBeMerged() const
{
  if (!OK()) return FALSE ;
  CostScalar uec = getUec();
  CostScalar row = getRowcount();

  // This is take care of intevals which have uecs like 0.999993333. We do not want them
  // to be merged with the previous interval.

  if ( (uec.getValue() < COSTSCALAR_EPSILON) &&
       (row.getValue() < COSTSCALAR_EPSILON) )
    return TRUE;
  else
    return FALSE;

  return ( row > csZero && row < csOne );
}

// -----------------------------------------------------------------------
//
// iterators on the Interval object
//
// -----------------------------------------------------------------------

void
Interval::next ()
{
  if ( isLast() )
    setInvalid() ; // anything after me is no good!
  else {
    loIndex_++ ;
    hiInt_ = ((*hist_)[loIndex_+1]);
  }
}

void
Interval::prev ()
{
  if ( isFirst() )
    setInvalid() ; // anything previous to me is no good!
  else {
    loIndex_-- ;
    hiInt_  = ((*hist_)[loIndex_+1]);
  }
}

// -----------------------------------------------------------------------
// Interval sanity check
//
// NB: We can't call the Interval class member functions in this method,
// because they all call this, and we hate infinite recursion!
// -----------------------------------------------------------------------
#ifndef NDEBUG
NABoolean
Interval::OK () const
{
  if (!isValid() )
  {
    CCMPASSERT( isValid() ) ;
    return FALSE;
  }

  if (hist_->entries() == 1 )
  {
    CCMPASSERT( hist_->entries() != 1 ) ;
    return FALSE;
  }

  if ((*hist_)[loIndex_+1].getUec().isLessThanZero() )
  {
    CCMPASSERT( (*hist_)[loIndex_+1].getUec().isGreaterOrEqualThanZero() ) ; // getUec() >= 0
    (*hist_)[loIndex_+1].setCardAndUec(0,0);
  }

  if ((*hist_)[loIndex_+1].getCardinality().isLessThanZero() )
  {
    CCMPASSERT( (*hist_)[loIndex_+1].getCardinality().isGreaterOrEqualThanZero() ) ;  // getRowcount() >= 0
    (*hist_)[loIndex_+1].setCardAndUec(0,0);
  }

  if ( (*hist_)[loIndex_].getBoundary() == (*hist_)[loIndex_+1].getBoundary() )
    { // isSingleValued()
      // removed this first one, since it's impossible to know precisely
      // how many uec's are in an interval without first looking at the
      // reduction factor (which we can't see from the histogram level ...)
      // CMPASSERT( (*hist_)[loIndex_+1].getUec() <= 1 ) ;

      //Removing the following 2 assertions because they are causing assertion
      //failures in OPTDML02 regression test. The test uses fake statistics
      //that someone has manually generated. These statistics are incorrect, but
      //does not explain why it used to be work and now it fails?
      //These assertions serve as a good sanity check, therefore we should put
      //the assertion back in for the next release.
      //assertion1: CMPASSERT( ! (*hist_)[loIndex_].isBoundIncl() ) ; // isLoBoundInclusive()
      //assertion2: CMPASSERT( (*hist_)[loIndex_+1].isBoundIncl() ) ; // isHiBoundInclusive()
    }
    return TRUE;
}
#endif

// -----------------------------------------------------------------------
//
// answers the question: does THIS Interval contain parameter value?
//
// -----------------------------------------------------------------------
NABoolean
Interval::containsValue (const EncodedValue & value) const
{
  const EncodedValue hiBound = this->hiBound() ;
  const EncodedValue loBound = this->loBound() ;

  // CASE 1 : value is less than lower bound
  if ( loBound > value )
    return FALSE ;
  // CASE 2 : value is greater than upper bound
  else if ( hiBound < value )
    return FALSE ;
  // CASE 3 : value is equal to lower bound, and the
  //          Interval's lower bound is inclusive
  else if ( loBound == value)
    {
      if ( isLoBoundInclusive() )
        return TRUE ;
      else
        return FALSE ;
    }
  // CASE 4 : value is equal to upper bound, and the
  //          Interval's upper bound is inclusive
  else if ( hiBound == value )
    {
      if ( isHiBoundInclusive() )
        return TRUE ;
      else
        return FALSE ;
    }
  // CASE 5 : value is between lower and upper bounds
  else if ( loBound < value && value < hiBound )
    return TRUE ;
  // CASE 6 : is this possible?
  else
    return FALSE ;
}

// removing a NULL interval, if it exists
void ColStats::removeNullInterval()
{
  if ( isNullInstantiated() ) // used only for _shapeChanged_ flag maint.
    {
      histogram_->removeNullInterval() ;

      // after removing NULL interval remove the NULL value from skewValue list too
	  if ( (!isOrigFakeHist()) )
      {
        FrequentValueList & frequentValues = getModifableFrequentValues();
        frequentValues.removeNULLAsFrequentValue();
      }

      setShapeChanged (TRUE) ;
    }
}

// reporting the number of NULLs / NULL-uecs in that interval
CostScalar
ColStats::getNullCount() const
{
  if ( isNullInstantiated() )
    {
      Interval null = histogram_->getLastInterval() ;
      return null.getRowcount() ;
    }
  else
    {
      return 0 ;
    }
}

CostScalar
ColStats::getNullUec() const
{
  if ( isNullInstantiated() )
    {
      Interval null = histogram_->getLastInterval() ;
      return null.getUec() ;
    }
  else
    {
      return 0 ;
    }
}


// setting the number of NULLs and NULL-uecs in that interval
void
ColStats::setNullRowsAndUec (CostScalar nulls, CostScalar nullUec)
{
  if (!isNullInstantiated() )
  {
    // if the histogram does not contain a NULL Interval, nothing to do
    CCMPASSERT ( isNullInstantiated() ) ;
    return;
  }
  Interval null = histogram_->getLastInterval() ;
  null.setRowsAndUec (nulls, nullUec) ;
  setShapeChanged (TRUE) ;
}
// -----------------------------------------------------------------------
// we want to maintain a *very* important histogram semantic :
//
//                      uecs <= rows
//
// ==> this is *very* important!
//
// The following routine maintains this semantic at the ColStats level;
// other functions (HistInt::setCardAndUec(), Interval::setRowsAndUec())
// work toward the same goal at the individual interval level.
// -----------------------------------------------------------------------

void ColStats::setRowsAndUec (CostScalar rows, CostScalar uec, NABoolean allowMinusOne)
{
  // if this is skewed, then we need to adjust the uec reduction factor
  // The operator greater than does some arithmetic manipulations, which
  // can lead to overflow conditions, if the uec and the row counts are
  // very small. Since uec and rows are later rounded to Zero if very small,
  // it should be safe to first round and then compare.
  uec.round();
  rows.round();
  if ( uec > rows )
    {
      uecRedFactor_ *= rows / uec ;
      uec = rows ;
    }

  rows = MIN_ONE_CS(rows);

  // consistency check so that we will not have rows >> uec = 0
  if( uec.isZero() && !rows.isZero() )
    uec = csOne;

  setRowcount (rows) ;
  setTotalUec (uec, allowMinusOne) ;
}

void ColStats::setRowcount (CostScalar row)
{
  if (row < csZero)
  {
    // min rowcount is zero
    CCMPASSERT (row >= csZero) ;
    row = csZero;
  }
  else
    row.roundIfZero();
  rowcount_ = row ;
}

void ColStats::setTotalUec (CostScalar uec, NABoolean allowMinusOne)
{
  if (uec < csZero)
  {
    if (allowMinusOne == TRUE)
       uec = csMinusOne;
    else
    {
       // min UEC is zero
       CCMPASSERT (uec >= csZero) ;
       uec = csZero;
    }
  }
  else
    uec.roundIfZero();
  totalUec_ = uec ;
}

void ColStats::setBaseUec (CostScalar uec)
{
  if (uec < csZero)
  {
    // min UEC is zero
    CCMPASSERT (uec >= csZero) ;
    uec = csZero;
  }
  else
    uec.roundIfZero();
  baseUec_ = uec ;
}

void ColStats::setBaseRowCount (CostScalar row)
{
  if (row < -1)
  {
    // reset baserowcount to -1
    CCMPASSERT (row >= -1) ;
    return;
  }

  row.roundIfZero() ;
  baseRowCount_ = row ;
}

// the following is used to store the sum-of-max-uec-per-interval value in
// mergeColStats, for later perusal/resetting in estimateCardinality
void ColStats::setSumOfMaxUec (CostScalar value)
{
  if (value < 0)
  {
    // min sum of max UEC is zero
    CCMPASSERT (value >= 0) ;
    value = 0;
  }
  sumOfMaxUec_ = value;
}

// we have to be extremely careful about rounding the reduction factors
// because they can legitimately become very close to zero but not equal
// to zero (e.g., join between 2 1-billion row tables returns 1 row ==>
// redfactor == 1e-18)
void ColStats::setRedFactor (CostScalar rowred)
{
  if (rowred < 0)
  {
    // min row reduction is 0, resulting in 0 rows
    CCMPASSERT (rowred >= 0) ;
    rowred = 0;
  }
  else
    rowred.roundIfExactlyZero() ;
  rowRedFactor_ = rowred ;
}

void ColStats::setUecRedFactor (CostScalar uecred)
{
  if (uecred < 0)
  {
    // min uec reduction is zero, resulting in 0 uec
    CCMPASSERT (uecred >= 0) ;
    uecred = 0;
  }
  else
    uecred.roundIfExactlyZero() ;
  uecRedFactor_ = uecred ;
}

//-----------------------------------------------------------------------
// static ColStats::deepCopy()
// Creates a new ColStats by doing a shallow copy of other. Then it does
// a shallow copy of the Histogram object(private member is Histogram pointer
// so this is necessary but a deep copy of the Histogram is not necessary).
//-----------------------------------------------------------------------
ColStatsSharedPtr 
ColStats::deepCopy(const ColStats& other, NAMemory * heap, 
                   NABoolean useColumnPositions, NABoolean copyIntervals)
{
  ColStatsSharedPtr result(new(heap)ColStats(other, heap, !useColumnPositions));
  HistogramSharedPtr histogram;
  if (copyIntervals)
    histogram = new(heap)Histogram(*(other.getHistogram()),heap);
  else
    histogram = new(heap)Histogram(heap);

  result->setHistogram(histogram);

  if ( (!other.isOrigFakeHist()) )
  {
    result->setFrequentValue(other.getFrequentValues());
  }
  unsigned short members =(short) (other.columns_.entries());
  for(unsigned short i=0;i<members;i++)
  {
    if (useColumnPositions)
    {
      // use "lean" representation of columns
      result->colPositions_ += (other.columns_[i])->getPosition();
    }
    else
    {
      // a member by member deepCopy of NAColumnArray columns_
      result->columns_[i]= NAColumn::deepCopy(*(other.columns_[i]),heap);
    }
  }
  return result;
}

// creates a deep copy of single-column histogram from cache.
// sets deep copy's column to col.
ColStatsSharedPtr 
ColStats::deepCopySingleColHistFromCache
(const ColStats& other, NAColumn& col, NAMemory * heap, 
 NABoolean copyIntervals)
{
  CMPASSERT(other.columns_.entries() <= 1);
  ColStatsSharedPtr result = ColStats::deepCopy(other, heap, FALSE, 
                                                copyIntervals);
  result->columns_.insert(&col);
	return result;
}

// -----------------------------------------------------------------------
//
// want to remove all HistInts above boundary interval
//              __
// |  |  |  |  |  |  |  |  |
// 0  1  2  3  4  5  6  7  8
//             boun
//
// ==> remove 3 (==entries()-boundary.getLoIndex()-1)
//                        9 - 4 - 2
//
// -----------------------------------------------------------------------
void
ColStats::deleteIntervalsAbove (const Interval & boundary)
{
  CollIndex boundaryIndex = boundary.getLoIndex() ;
  if (( histogram_->entries() + boundaryIndex ) < 2)
  {
    return;
  }

  CollIndex index = histogram_->entries() - boundaryIndex - 2 ;
  CollIndex i ;
  for ( i = 1 ; i <= index ; i++ )
    {
      histogram_->removeAt(boundaryIndex+2) ;
    }

  if ( index > 0 ) // i.e., if we actually removed any
    {
      setShapeChanged (TRUE) ;
    }
}

// -----------------------------------------------------------------------
//
// want to remove all HistInts below boundary interval
//              __
// |  |  |  |  |  |  |  |  |
// 0  1  2  3  4  5  6  7  8
//             boun
//
// ==> remove 4 (==boundary.getLoIndex())
//
// NB: this function invalidates the parameter Interval
// -----------------------------------------------------------------------
void
ColStats::deleteIntervalsBelow (Interval & boundary)
{
  CollIndex index = boundary.getLoIndex() ;

  CollIndex i ;
  for ( i = 1 ; i <= index ; i++ )
    {
      histogram_->removeAt(0) ;
    }

  if ( index > 0 )  // i.e., if we actually removed any
    {
      setShapeChanged (TRUE) ;
      boundary.setInvalid() ;
      (*histogram_)[0].setCardAndUec (0,0) ; // maintain Histogram's
      //                                     // internal semantics!
    }
}


Interval
Histogram::getNextInterval(const Interval & current) const
{
  if (current.isLast()) // test boundary conditions
    {
      return Interval() ;
    }
  else
    {
      Interval nxt = current ;
      nxt.next() ;
      return nxt ;
    }
}

Interval
Histogram::getPrevInterval(const Interval & current) const
{
  if (current.isFirst()) // test boundary conditions
    {
      return Interval() ;
    }
  else
    {
      Interval prv = current ;
      prv.prev() ;
      return prv ;
    }
}

// -----------------------------------------------------------------------
// simple helper function that does the work of inserting an Interval into
// a pre-existing histogram; does this work for four special cases
//
//   1. histogram is empty
//   1a. histogram non-empty, but inserting a NULL interval
//   2. histogram needs a new interval at top (look for NULL!)
//   3. histogram needs a new interval at bottom
//
// this function assumes that the histogram we're passed isn't
// simply a NULL histogram (2 NULL HistInts, nothing else)
// -----------------------------------------------------------------------
void
Histogram::insertZeroInterval (const EncodedValue & loBound,
                               const EncodedValue & hiBound,
                               NABoolean isNewBoundIncluded)
{
  // 3 cases
  // CASE 1: if no HistInts currently in Histogram,
  //         simply create the two HistInts and insert 'em
  //
  // CASE 1a: used to insert a NULL interval at the end of the
  //          histogram

  if ( numIntervals() == 0 || loBound.isNullValue() )
    {
      // we need to insert TWO HistInt's; row/uec init at 0
      HistInt newLo (loBound, FALSE) ;
      HistInt newHi (hiBound, isNewBoundIncluded) ;
      insert (newLo) ;
      insert (newHi) ;
      return ;
    }

  // CASE 2: loBound == the last Interval's boundary value
  Interval last = getLastNonNullInterval() ;

  if (!last.isValid() )
  {
    // if the histogram is not valid, clear the histogram
    // and insert an interval with given boundaries
    CCMPASSERT ( last.isValid() ) ;
    this->clear();
    HistInt newLo (loBound, FALSE) ;
    HistInt newHi (hiBound, isNewBoundIncluded) ;
    insert (newLo) ;
    insert (newHi) ;
    return ;
  }

  // otherwise, this function shouldn't have been called!

  if ( loBound == last.hiBound() )
    {
      HistInt newHi (hiBound, isNewBoundIncluded) ;
      insertAt (last.getLoIndex()+2, newHi) ;

      return ;
    }

  // CASE 3: hiBound == the first Interval's boundary value
  Interval first = getFirstInterval() ;

  if (first.isNull())
  {
    // if first interval is NULL interval, nothing to do
    CCMPASSERT (!first.isNull()) ;
    return;
  }

  // otherwise this function shouldn't have been called

  if ( hiBound == first.loBound() )
    {
      HistInt newLo (loBound, !isNewBoundIncluded) ;
      // inverse because the low bound of an Interval sees the opposite of
      // the HistInt flag
      insertAt (0, newLo) ;
      return ;
    }

  CCMPASSERT(FALSE) ; // misuse of this function!
  // nothing to do, return
}

void
Histogram::insertZeroInterval (const CostScalar& loBound,
                               const CostScalar& hiBound,
                               NABoolean isNewBoundIncluded)
{
   insertZeroInterval (EncodedValue(loBound.getValue()),
                       EncodedValue(hiBound.getValue()),
                       isNewBoundIncluded);
}

void
Histogram::insertZeroInterval (const NormValueList& loBound,
                               const NormValueList& hiBound,
                               NABoolean isNewBoundIncluded)
{
   insertZeroInterval (EncodedValue(loBound),
                       EncodedValue(hiBound),
                       isNewBoundIncluded);
}

// -----------------------------------------------------------------------
// simple auxiliary function which condenses a histogram into a single
// interval, maintaining the same max/min values and aggregate rows/uec
//
// if there are both non-NULL and NULL intervals, we remove the NULL
// interval (for convenience of later functions)
// -----------------------------------------------------------------------
void
Histogram::condenseToSingleInterval()
{
  if (numIntervals() == 0)
  {
    CCMPASSERT (numIntervals() > 0) ; // makes no sense for an empty histogram
    insertZeroInterval (UNINIT_ENCODEDVALUE, UNINIT_ENCODEDVALUE, TRUE) ;
    return;
  }

  if ( numIntervals() == 1 ) return ; // already a single interval

  CostScalar rows = 0, uec = 0 ;
  EncodedValue max, min ;
  NABoolean loBoundIncl, hiBoundIncl = FALSE;

  Interval iter = getFirstInterval() ;
  min = iter.loBound() ;

  // bad special case: it's hard to decide what to do is when we have a
  // NULL-interval as well as a non-NULL interval
  // --> in this case, we remove the null interval
  if ( isNullInstantiated() )
    {
      removeNullInterval() ;
    }

  loBoundIncl = iter.isLoBoundInclusive() ;

  while ( iter.isValid() ) // we break out when we hit the last one
    {
      rows += iter.getRowcount() ;
      uec  += iter.getUec() ;
      if ( iter.isLast() )
        {
          max = iter.hiBound() ;
          hiBoundIncl = iter.isHiBoundInclusive() ;
          break ;
        }
      iter.next() ;
    }

  this->clear() ;
  this->insertZeroInterval (min, max, hiBoundIncl) ;

  iter = getFirstInterval() ;
  iter.setLoBoundInclusive (loBoundIncl) ;
  iter.setRowsAndUec (rows, uec) ;
}

// is there a NULL interval in the Histogram?
NABoolean
Histogram::isNullInstantiated() const
{
  if ( numIntervals() == 0 )
    {
      return FALSE ;
    }
  else
    {
      Interval last = getLastInterval() ;
      if ( last.loBound().isNullValue() && last.hiBound().isNullValue() )
      {
        // semantics require that there must be 0 or 2+
        // HistInts besides the NULL interval
        if (entries() == 3)
        {
          CCMPASSERT ("Illegal number of intervals in the histogram");
          return FALSE;
        }

        return TRUE ;
      }
      // if either is NULL, but not both, then we screwed up somewhere
      CCMPASSERT ( !last.loBound().isNullValue() ) ;
      CCMPASSERT ( !last.hiBound().isNullValue() ) ;
      return FALSE ;
    }
}

// removing that NULL interval (assuming it exists)
void
Histogram::removeNullInterval()
{
  if (NOT isNullInstantiated() )
  {
    // no null interval. Nothing to remove
    CCMPASSERT ( isNullInstantiated() ) ;
    return;
  }
  // remove both NULL-valued HistInts
  removeAt (entries()-1) ;
  removeAt (entries()-1) ;
}

// inserting a NULL interval (assuming it doesn't already exist)
void
Histogram::insertNullInterval()
{
  if (isNullInstantiated() )
  {
    // if the NULL interval already exists, return. Nothing more to do.
    CCMPASSERT ( !isNullInstantiated() ) ;
    return;
  }
  insertZeroInterval (NULL_ENCODEDVALUE, NULL_ENCODEDVALUE, TRUE) ;
}


// -----------------------------------------------------------------------
// Method to reduce the number of histogram intervals
// -----------------------------------------------------------------------
void Histogram::reduceNumHistInts(Criterion reductionCriterion,
                                  Source invokedFrom)
{
	//if reduction criterion is none then return
	if(reductionCriterion == NONE)
		return;

    //interval object used to iterate of intervals
    Interval iter ;
    //iterate over the intervals of this histogram
    for ( iter = getFirstInterval() ;
		  iter.isValid() && !iter.isNull();
		  /* no automatic increment */)
          {
			  if ( iter.isLast() ) break ; // only one interval in total; done

			  // at this point, we know another interval exists
              Interval next = getNextInterval (iter) ;

              if ( next.isNull() ) break; // do not merge NULL intervals!

              //if the current interval or the next interval has row count of
              //zero (which implies UEC = 0) then merge the current with the next.
              if ((iter.getRowcount() == csZero) || (next.getRowcount() == csZero))
              {
                if(!iter.merge(next))
                  iter.next();
			  }
			  //if the current interval is approximately equal to the next
			  //interval then merge current with next
			  else if ( iter.compare(invokedFrom, reductionCriterion, next))
			  {
              	if(!iter.merge(next))
                  iter.next();
			  }
			  //if current and next are not approximately equal then iterate
			  //over to the next interval.
			  else{
				  iter.next();
			  }
		  }

}

// compute the extended boundaries of an interval when compared to its neighbors. The method does not
// have any side affect on the interval or its neighbors . This is used by the HQC logic
void Histogram::computeExtendedIntRange (Interval& currentInt, Criterion& reductionCriterion, 
                                         EncodedValue& hiBound, EncodedValue& loBound, 
                                         NABoolean& hiBoundInclusive, NABoolean& loBoundInclusive)
{
    // nothing to do if Criterion is NONE
    if (reductionCriterion == NONE)
       return;

    NABoolean intervalExtended = FALSE;

    // try merging with the subsequent intervals
    Interval nextInt = getNextInterval (currentInt);

    while (nextInt.isValid() && !nextInt.isNull() && currentInt.compare(AFTER_FETCH, reductionCriterion, nextInt))
    {
        intervalExtended = TRUE;
        hiBound = nextInt.hiBound();
        hiBoundInclusive = nextInt.isHiBoundInclusive();
        nextInt = getNextInterval (nextInt);
    }

    // try merging with the preceeding intervals
    Interval prevInt = getPrevInterval (currentInt);
    while (prevInt.isValid() && currentInt.compare(AFTER_FETCH, CRITERION1, prevInt))
    {
        intervalExtended = TRUE;
        loBound = prevInt.loBound();
        loBoundInclusive = prevInt.isLoBoundInclusive();
        prevInt = getPrevInterval (prevInt);
    }

    ostream* hqc_ostream=CURRENTQCACHE->getHQCLogFile();
    if (intervalExtended && hqc_ostream)
    {
       *hqc_ostream << "  -- HQC performed an interval extention  -- \n" 
                    << "    Interval Initial boundaries are: " << endl
                    << "\t LOW:  [" << currentInt.loBound().getDblValue()  << "]" << endl
                    << "\t HIGH: [" << currentInt.hiBound().getDblValue() << "]" << endl; 

       *hqc_ostream << "    Result Interval boundaries: "  << endl
                    << "\t LOW:  [" << loBound.getDblValue() << "] with low bound" << (loBoundInclusive? " ": " NOT ") << "inclusive" << endl
                    << "\t HIGH: [" << hiBound.getDblValue() << "] with high bound" << (hiBoundInclusive? " " : " NOT ") << "inclusive" << endl;
    }
}


CostScalar Histogram::mergeSVIWithNextAndSetMaxFreq()
{
  CostScalar maxFreq = -1.0;

  //interval object used to iterate of intervals
  Interval iter ;

  NABoolean firstInterval = TRUE;

  //iterate over the intervals of this histogram
  for ( iter = getFirstInterval() ;
    iter.isValid() && !iter.isNull(); )
  {
	  // if the frequency of the interval is less than zero, we assume the frequency
	  // to be equal to the rowcount
	  CostScalar currFreq = iter.getRowcount() / (iter.getUec()).minCsOne();

	  if (currFreq > maxFreq)
		maxFreq = currFreq;

	  if ( iter.isLast() ) break ; // only one interval in total; done

    // at this point, we know another interval exists
    Interval next = getNextInterval (iter) ;

    if ( next.isNull() ) break; // do not merge NULL intervals!

	//if the current interval is SVI then merge the current with the next.
	if ( (iter.isSingleValued()) &&  !firstInterval &&
		 (iter.getRowcount() < (next.getRowcount() / next.getUec() / 0.50) ) )
    {
	  NABoolean mergeSuccessful = iter.merge(next);
      if (!mergeSuccessful)
        iter.next();
    }
	else
	  iter.next();

	firstInterval = FALSE;
  }
  return maxFreq;
}

// -----------------------------------------------------------------------
// Finds where in the list of HistInts to place the new HistInt.  Then,
// divides the rows/uecs from the divided Interval into the two new
// Intervals (or, if this HistInt boundary already exists, jumps to next
// step).
//
// Finally, removes the intervals above or below the indentified interval
// boundary.  That is, for < operations, we remove all Intervals above
// this one; for > ops, we destory all Histints below it.
//
// ** This function assumes that the value we're looking for is NOT equal
// ** to the max or min value of the Histogram.  Those cases should have
// ** already been handled by the calling function.  We don't want to
// ** handle those here because we already handle so many boundary
// ** conditions in this function!
//
// Sadly, this function is a mess!  I cannot think of any easy way to
// clean it up, since the boundary cases are so incredibly thorny.  But
// for any case, it should be easy to verify it's doing the right thing.
// -----------------------------------------------------------------------
void
ColStats::divideHistogramAlongBoundaryValue(const EncodedValue & value,
                                            OperatorTypeEnum splitOperator)
{
  // any NULL Intervals should have been removed by now. If not do it now
  if (isNullInstantiated() )
  {
    CCMPASSERT ( !isNullInstantiated() ) ;
    removeNullInterval();
  }

  if ( histogram_->numIntervals() == 0 )
    return ;

  // remove the values based on the splitOperator from the skew value list 
  // if split operator is LESS_THAN, implying keep only value that are less than
  // the given below, we remove all values greater than or equal to the given 
  // value from the frequentValueList. The Boolean flag == TRUE implies include
  // include the value while deleting. FALSE means exclude the given value
  if ( (!isOrigFakeHist()) )
  {
      FrequentValueList & frequentValueList = getModifableFrequentValues();
      switch (splitOperator)
        {
        case ITM_LESS_EQ:
          frequentValueList.deleteFrequentValuesAboveOrEqual (value, FALSE) ;
		  break;

        case ITM_GREATER_EQ:
          frequentValueList.deleteFrequentValuesBelowOrEqual (value, FALSE) ;
		  break;

        case ITM_LESS:
          frequentValueList.deleteFrequentValuesAboveOrEqual (value, TRUE) ;
		  break;

        case ITM_GREATER:
          frequentValueList.deleteFrequentValuesBelowOrEqual (value, TRUE) ;
          break ;
        }
  }

  Interval iter = histogram_->getFirstInterval() ;

  // we want to iterate through the Intervals until we reach
  // the first one where value is >= the low boundary
  while ( value > iter.hiBound() && !iter.isLast() )
    iter.next() ;
  if ( iter.hiBound() == value )
    {
      if (iter.isLast())
      {
        CCMPASSERT ( !iter.isLast() ) ;
        setFakeHistogram(TRUE);
      }

      iter.next() ;
    }
  // the reason we do this last step (placing the equal boundary
  // as the lower bound of iter) is to set up the check for
  // the SVI --> if iter, which has value as its lower boundary,
  // is an SVI, then we certainly don't have to subdivide the
  // Histogram any further

  CollIndex iterIndex = iter.getLoIndex() ;

  // when splitOperator is ITM_LESS_EQ or ITM_GREATER_EQ, the
  // following should always be true --> unless we're calling
  // this function from somewhere besides newUpperBound / newLowerBound
  if ( iter.isSingleValued() )
    {
      switch (splitOperator)
        {
          // for <= value, del above iterIndex   (keep the SVI)
          // for >= value, del below iterIndex   (keep the SVI)
          // for <  value, del above iterIndex-1 ('rm' the SVI)
          // for >  value, del below iterIndex+1 ('rm' the SVI)
        case ITM_LESS_EQ:
          deleteIntervalsAbove (iter) ;
          return ;

        case ITM_GREATER_EQ:
          deleteIntervalsBelow (iter) ;
          return ;

        case ITM_LESS:
          if (iter.isFirst())
          {
            CCMPASSERT ( !iter.isFirst() ) ;
            // nothing to divide, return
            setFakeHistogram(TRUE);
            return;
          }
          iter.prev() ;
          deleteIntervalsAbove (iter) ;
          return ;

        case ITM_GREATER:
          if(iter.isLast())
          {
            CCMPASSERT ( !iter.isLast() ) ;
            setFakeHistogram(TRUE);
            return;
          }
          iter.next() ;
          deleteIntervalsBelow (iter) ;
          return ;

        default:
          CCMPASSERT(FALSE) ; //misuse of this function!
          return ;
        }
    }

  if ( value == iter.loBound() )
    {
      // time to check the annoying & complicated boundary cases
      //
      // 0    1    2    3    4    5    6    HistInt#
      // <    <    <    <=   <    <=   <=   BoundsIncl
      // |    |    |    |    |    |    |
      // |    | I1 |iter| I2 |    |    |
      // 2    3    4    5    6    7    8    Value
      // value: 4
      // iter.isLoBoundInclusive: TRUE
      // I1: [3,4)  iter: [4,5]  I2: (5,6)
      // ==> for <= 4, iter --> [4,4]+(4,5]
      //     [3,4)[4,4](4,5]    del above iterIndex   (==index of SVI)
      // ==> for >= 4, do not need an SVI
      //     [3,4)[4,5]         del below iterIndex   (==index of iter)
      // ==> for <  4, do not need an SVI
      //     [3,4)[4,5]         del above iterIndex-1 (==index of I1)
      // ==> for >  4, iter --> [4,4]+(4,5]
      //     [3,4)[4,4](4,5]    del below iterIndex+1 (==index of iter')

      if ( iter.isLoBoundInclusive() == TRUE )
        {
          switch (splitOperator)
            {
            case ITM_LESS_EQ:
              histogram_->insertSingleValuedInterval (value) ;
              // the above function messes up iter, so we need
              // a "fresh" copy
              iter = Interval (iterIndex,histogram_) ;
              deleteIntervalsAbove (iter) ;
              return ;

            case ITM_GREATER_EQ:
              deleteIntervalsBelow (iter) ;
              return ;

            case ITM_LESS:
              if (iter.isFirst())
              {
                CCMPASSERT ( !iter.isFirst() ) ; // debugging
                setFakeHistogram(TRUE);
                return;
              }
              iter.prev() ;
              deleteIntervalsAbove (iter) ;
              return ;

            case ITM_GREATER:
              histogram_->insertSingleValuedInterval (value) ;
              // the above function messes up iter, so we need
              // a "fresh" copy
              iter = Interval (iterIndex+1,histogram_) ;
              deleteIntervalsBelow (iter) ;
              return ;

            default:
              CCMPASSERT(FALSE) ; //misuse of this function!
              return ;
            }
        }
      // 0    1    2    3    4    5    6    HistInt#
      // <    <    <=   <=   <    <=   <=   BoundsIncl
      // |    |    |    |    |    |    |
      // |    | I1 |iter| I2 |    |    |
      // 2    3    4    5    6    6    7    Value
      // value: 4
      // iter.isLoBoundInclusive: FALSE
      // I1: [3,4]  iter: (4,5]  I2: (5,6)
      // ==> for <= 4, do not need an SVI
      //     [3,4](4,5]         del above iterIndex-1 (==index of I1)
      // ==> for >= 4, I1 --> (3,4)+[4,4]
      //     [3,4)[4,4](4,5]    del below iterIndex   (==index of SVI)
      // ==> for < 4, I1 --> (3,4)+[4,4]
      //     [3,4)[4,4](4,5]    del above iterIndex-1 (==index of I1)
      // ==> for > 4, do not need an SVI
      //     [3,4](4,5]         del below iterIndex   (==index of iter)

      else // iter.isLoBoundInclusive() == FALSE
        {
          switch (splitOperator)
            {
            case ITM_LESS_EQ:
              if (iter.isFirst())
              {
                CCMPASSERT ( !iter.isFirst() ) ; // debugging
                setFakeHistogram(TRUE);
                return;
              }
              iter.prev() ;
              deleteIntervalsAbove (iter) ;
              return ;

            case ITM_GREATER_EQ:
              histogram_->insertSingleValuedInterval (value) ;
              // the above function messes up iter, so we need
              // a "fresh" copy
              iter = Interval (iterIndex,histogram_) ;
              deleteIntervalsBelow (iter) ;
              return ;

            case ITM_LESS:
              if (iter.isFirst())
              {
                CCMPASSERT ( !iter.isFirst() ) ; // debugging
                setFakeHistogram(TRUE);
                return;
              }
              histogram_->insertSingleValuedInterval (value) ;
              // the above function messes up iter, so we need
              // a "fresh" copy
              iter = Interval(iterIndex-1,histogram_) ;
              deleteIntervalsAbove (iter) ;
              return ;

            case ITM_GREATER:
              deleteIntervalsBelow (iter) ;
              return ;

            default:
              CCMPASSERT(FALSE) ; //misuse of this function!
              return ;
            }
        }
    } // value == iter.loBound()


  // *********************************************************
  // now handle the NON-boundary cases (the easy ones)
  //
  //before:
  // 0    1    2         3    4    5
  // |    |    |         |    |    |     value: 7.5
  // |    |    |         |    |    |     iterIndex: 2
  // 3    5    7         8    9    10
  //              iter
  //after:
  // 0    1    2    3    4    5    6
  // |    |    |    |    |    |    |     value: 7.5
  // |    |    |    |    |    |    |     iterIndex: 2
  // 3    5    7   7.5   8    9    10
  //           lower upper

  // OK, now we know that the boundary value we're inserting isn't
  // equal to an Interval boundary

  // what we do now is very similar to what we did for
  // Histogram::insertSingleValuedInterval() below

  // first, cache values from ITER that we'll need later
  const EncodedValue loBoundary = iter.loBound() ;
  const EncodedValue hiBoundary = iter.hiBound() ;
  const CostScalar rows = iter.getRowcount() ;
  const CostScalar uec  = iter.getUec() ;

  // now, build the HistInt and insert it
  HistInt newHistInt (value) ;
  histogram_->insertAt(iterIndex+1, newHistInt) ;

  // Q1: how do we set the boundary inclusive flag of the new Interval?
  // A1: set the hiBound of the lower interval as follows:
  //
  //     [1,3] <= 2 --> [1,2](2,3] --> [1,2]   boundIncl: TRUE
  //     [1,3] <  2 --> [1,2)[2,3] --> [1,2)   boundIncl: FALSE
  //     [1,3] >= 2 --> [1,2)[2,3] --> [2,3]   boundIncl: FALSE
  //     [1,3] >  2 --> [1,2](2,3] --> (2,3]   boundIncl: TRUE
  //
  // Q2: and when we're done, which is the place from which
  //     we delete Intervals?
  // A2: 'lower' for <=,<, 'upper' for >=,>

  Interval lower (iterIndex,  histogram_) ;
  Interval upper (iterIndex+1,histogram_) ;

  switch ( splitOperator )
    {
    case ITM_LESS_EQ:
    case ITM_GREATER:
      lower.setHiBoundInclusive (TRUE) ;
      break ;

    case ITM_LESS:
    case ITM_GREATER_EQ:
      lower.setHiBoundInclusive (FALSE) ;
      break ;

    default:
      // misuse of this function!
      // set the histogram as fake and return without applying the predicate
      CCMPASSERT(FALSE) ; 
      return ;
    }

  NAList<Interval> spanList(CmpCommon::statementHeap());
  spanList.clear() ; // probably unnecessary

  spanList.insert (lower) ;
  spanList.insert (upper) ;

  Interval::distributeRowsAndUec (spanList,
                                  rows,
                                  uec,
                                  loBoundary,
                                  hiBoundary) ;

  // Don't forget to delete the Intervals!
  if ( splitOperator == ITM_LESS_EQ || splitOperator == ITM_LESS )
    deleteIntervalsAbove (lower) ;
  else
    deleteIntervalsBelow (upper) ;
}
//Helper method to adjust Rowcount for rolling columns
void
ColStats::adjustRowcountforRollingColumns(ConstValue * constant)
{
  Lng32 filler = 0;
  CostScalar totalRowCount = 0, totalUec = 0, iterRowCount = 0, iterUec = 0;
  NAString dateTxt = ("(");
  EncodedValue encodedCurTime = EncodedValue(constant, FALSE);

  if (encodedCurTime == UNINIT_ENCODEDVALUE)
    return;

  HistogramSharedPtr hist = getHistogramToModify();
  Interval first = hist->getFirstInterval();	    
  Interval last = hist->getLastNonNullInterval();

  double timeEncompassedInHistogram = (last.hiBound().getDblValue() - first.loBound().getDblValue());
  // For histograms with UEC equal to 1, timeEncompassedInHistogram can become zero since the 
  // histogram will contain only one interval and the value of last.hiBound will be equal to first.loBound
  // To handle such cases, we ensure that the time encompassed in histogram cannot have value 
  // lower than the UEC of the last non-null interval. We also ensure it is atleast 1, to avoid divide
  // by zero.

  timeEncompassedInHistogram = MAXOF(timeEncompassedInHistogram, 1.0);

  // Create the new interval with an extra day to ensure that the density of the histograms even after
  // applying the equality predicates is not lost. The issue can be seen for between predicates such that
  // both the values being looked for lie outside the histogram boundaries. 
  // Example, the histogram has dates till 08-13-2010. The predicate being applied to the column is 
  // between 08-23-2010 and 08-29-2010. The histogram will be first extrapolated for 08-23-2010, and then
  // the predicate >= 08-23-2010 will be applied. While doing this we loose the original density of the
  // histogram. Now when the histogram is extrapolated for 08-29-2010, it could result in incorrect estimates
  // To prevent such issues, the histogram will be extrapolated for 08-24-2010 instead of 08-23-2010. This will
  // ensure that when we apply >= 08-23-2010 kind of predicate we actually save the density of values. For less
  // than predicate, this value will anyway be chopped hence will not have an impact on the cardinality
  // To add hist_Num_Additional_Days_To_Extrapolate extra day add (24 * 60 * 60) * histNumOfAddDaysToExtrapolate
  // to the encodedCurtime
  encodedCurTime = encodedCurTime.getDblValue() + (CURRSTMT_OPTDEFAULTS->histNumOfAddDaysToExtrapolate() * 86400);

  double timeEncompassedInNewInterval = (encodedCurTime.getDblValue() - last.hiBound().getDblValue());

  if ((timeEncompassedInNewInterval <= 0))
    return;

  hist->insertZeroInterval(last.hiBound().getDblValue(), encodedCurTime, TRUE);

  totalRowCount = getRowcount().getValue();
  totalUec = getTotalUec().getValue();

  if(!totalUec.isGreaterThanZero())
  {
    CCMPASSERT (totalUec.isGreaterThanZero()) ;
    totalUec = csOne;
  }

  iterUec = (totalUec * timeEncompassedInNewInterval) / timeEncompassedInHistogram;
  iterRowCount = (totalRowCount * iterUec) / totalUec;

  Interval newLast = hist->getLastNonNullInterval();
  newLast.setRowsAndUec(iterRowCount, iterUec);
  
  totalUec += iterUec;
  totalRowCount += iterRowCount;

  setMaxValue(encodedCurTime);
  setRowsAndUec(totalRowCount, totalUec);
  setIsARollingColumn();
}

// -----------------------------------------------------------------------
// do the work of inserting, into the histogram, a SVI
//
// assumes that the SVI's value falls inside (not-inclusive-of)
// the min-max of the histogram
//
// after inserting the necessary one (or two) HistInts,
// calculates the correct # of rows/uecs for the SVI and
// subtracts the appropriate amount from the interval that
// previously contained this value in the histogram
// -----------------------------------------------------------------------
CollIndex
Histogram::insertSingleValuedInterval (const EncodedValue & value,
                                       NABoolean distributeRowsAndUec)
{
  // first, find the Interval that contains the
  // value for our soon-to-be-created SVI
  if ( numIntervals() == 0 )
    return NULL_COLL_INDEX ;

  Interval iter = getFirstInterval() ;
  while ( !iter.containsValue (value) )
    iter.next() ;

  if ( !iter.containsValue (value) )
    return NULL_COLL_INDEX ; // something no good

  const CostScalar rows = iter.getRowcount() ;
  const CostScalar uec  = iter.getUec() ;
  CollIndex iterIdx = iter.getLoIndex() ;

  // OK, we've found the interval that should contain
  // the SVI; now, let's build our SVI

  Interval theSVI ;
  NAList<Interval> spanList(CmpCommon::statementHeap());
  spanList.clear() ; // probably unnecessary

  const EncodedValue loBoundary = iter.loBound() ;
  const EncodedValue hiBoundary = iter.hiBound() ;
  CollIndex retval ;

  // Get the SharedPtr object stored within the "this" pointer so it can
  // be used within this function.
  HistogramSharedPtr thisPtr = HistogramSharedPtr::getIntrusiveSharedPtr(this);

  // there are three cases to consider

  // CASE 1 : value is equal to lower bound
  if ( value == loBoundary && iter.isLoBoundInclusive() )
    {
      // create a S.V.I. for value
      // --> this is simpler if THIS is already a S.V.I.
      if ( iter.isSingleValued() )
        {
          return iterIdx; // an SVI with the desired value already exists
        }
      else // otherwise, we need to split this Interval into
        // // two pieces; one for the S.V.I. for 'value', and
        // // one for the rest of ITER
        {
          // the new one just needs to be a copy of the
          // current lower boundary
          HistInt newHistInt (value) ;

          insertAt(iterIdx+1, newHistInt) ;
          //NB: at this point, ITER is no longer usable
          //    ==> we need to create the two resulting Intervals

          theSVI = Interval (iterIdx,  thisPtr);
          Interval newHigh  (iterIdx+1,thisPtr);

          retval = iterIdx ;

          theSVI.setLoBoundInclusive (TRUE) ;
          theSVI.setHiBoundInclusive (TRUE) ;

          spanList.insert(theSVI) ;
          spanList.insert(newHigh) ;

          //before:
          // rows |      12       |
          // uec  |       3       |
          //     lo              hi
          //            iter
          //
          //transition: (right after we insert the new HistInt)
          // rows |   ?   |  12   |
          // uec  |   ?   |   3   |
          //     lo      lo      hi
          //      theSVI    newHigh
          //
          //after: (figure out how much is in S.V.I.)
          // rows |   4   |   8   |
          // uec  |   1   |   2   |
          //     lo      lo      hi
          //      theSVI    newHigh
        }
    }
  // CASE 2 : value is equal to upper bound
  else if ( value == hiBoundary && iter.isHiBoundInclusive() )
    {
      //NB: we've already handled the S.V.I.==S.V.I. case above

      // the new one just needs to be a copy of the
      // current upper boundary
      HistInt newHistInt (value) ;

      insertAt(iterIdx+1, newHistInt) ;
      //NB: at this point, ITER is no longer usable
      //    ==> we need to create the two resulting Intervals

      Interval newLow   (iterIdx,  thisPtr) ;
      theSVI = Interval (iterIdx+1,thisPtr) ;

      retval = iterIdx + 1 ;

      theSVI.setLoBoundInclusive (TRUE) ;
      theSVI.setHiBoundInclusive (TRUE) ;

      spanList.insert(newLow) ;
      spanList.insert(theSVI) ;

      //before:
      // rows |      12       |
      // uec  |       3       |
      //     lo              hi
      //            ITER
      //
      //transition1: (right after we insert the new HistInt)
      // rows |   ?   |  12   |
      // uec  |   ?   |   3   |
      //     lo      hi      hi
      //      newLow     theSVI
      //
      //after: (figure out how much is in S.V.I.)
      // rows |   8   |   4   |
      // uec  |   2   |   1   |
      //     lo      hi      hi
      //      newLow     theSVI
    }
  // CASE 3 : value is between lower and upper bound
  //          (this one is very similar to the others)
  else
    {
      if (value < loBoundary || value > hiBoundary )
      {
        // nothing to do, value is outside the boundaries. 
        // return NULL_COLL_INDEX as the index of the interval;
        CCMPASSERT ( loBoundary < value && value < hiBoundary ) ;
        return NULL_COLL_INDEX;
      }

      // for this case, we need to insert TWO new (equal) HistInts
      HistInt newHistInt (value) ;

      // insert it twice
      insertAt(iterIdx+1, newHistInt) ;
      insertAt(iterIdx+1, newHistInt) ;
      //NB: at this point, ITER is no longer usable
      //    ==> we need to create the three resulting Intervals

      Interval newLow   (iterIdx,  thisPtr);
      theSVI = Interval (iterIdx+1,thisPtr);
      Interval newHigh  (iterIdx+2,thisPtr);

      retval = iterIdx + 1 ;

      theSVI.setLoBoundInclusive (TRUE) ;
      theSVI.setHiBoundInclusive (TRUE) ;

      spanList.insert(newLow) ;
      spanList.insert(theSVI) ;
      spanList.insert(newHigh) ;
    }

    // distribute rows and uec of the interval only if the caller is not going
    // to compute it later
    if (distributeRowsAndUec)
    {
      //
      // redistribute the rows/uec
      //
      Interval::distributeRowsAndUec (spanList,
                                      rows,
                                      uec,
                                      loBoundary,
                                      hiBoundary) ;
     }
    else
    {
      // set the RC and UEC of the new interval with the total RC and UEC of 
      // the parent interval. We will set the correct rowcount and uec based
      // on the values from frequent value list
      theSVI.setRowsAndUec(rows, uec);
    }

  return retval ; // the index of the SVI
}

// -----------------------------------------------------------------------
// returns TRUE if THIS spans the OTHER interval
// -----------------------------------------------------------------------
NABoolean
Interval::spans (const Interval & other) const
{
  // invalid intervals are not/do not span anything!
  if ( !other.isValid() || !isValid() ) return FALSE ;

  // there are several ways in which an interval can span another
  // interval
  const EncodedValue hiBound = this->hiBound() ;
  const EncodedValue loBound = this->loBound() ;
  const EncodedValue otherHiBound = other.hiBound() ;
  const EncodedValue otherLoBound = other.loBound() ;

  // case ZERO: handle NULLs first
  //
  // TRUE only if all boundaries are NULL
  if ( hiBound.isNullValue()      &&
       loBound.isNullValue()      &&
       otherHiBound.isNullValue() &&
       otherLoBound.isNullValue() )
    return TRUE ;

  // otherwise, FALSE if any is NULL
  if ( hiBound.isNullValue()      ||
       loBound.isNullValue()      ||
       otherHiBound.isNullValue() ||
       otherLoBound.isNullValue() )
    return FALSE ;

  // case ONE: THIS has an upper bound that is larger than OTHER's,
  //           and a smaller bound that is smaller
  //this
  //  |         |
  //  |         |
  //
  //     |   |
  //     |   |
  //other
  if ( hiBound  > otherHiBound &&
       loBound  < otherLoBound )
    return TRUE ; // this is always true

  // for all later cases, we need to know the inclusiveness information

  const NABoolean isHiInclusive = this->isHiBoundInclusive() ;
  const NABoolean isLoInclusive = this->isLoBoundInclusive() ;
  const NABoolean isOtherHiInclusive = other.isHiBoundInclusive() ;
  const NABoolean isOtherLoInclusive = other.isLoBoundInclusive() ;

  // case TWO: THIS has an upper bound that is equal to OTHER's,
  //           and a smaller bound that is smaller
  //this
  //  |         |
  //  |         |
  //
  //     |      |
  //     |      |
  //other
  if ( hiBound == otherHiBound &&
       loBound  < otherLoBound )
    if ( isOtherHiInclusive && !isHiInclusive )
      return FALSE ; // other is inclusive, I am not
    else
      return TRUE ;

  // case THREE: THIS has an upper bound that is greater than OTHER's,
  //             and a smaller bound that is equal
  //this
  //  |         |
  //  |         |
  //
  //  |      |
  //  |      |
  //other
  if ( hiBound  > otherHiBound &&
       loBound == otherLoBound )
    if ( isOtherLoInclusive && !isLoInclusive )
      return FALSE ; // other is inclusive, I am not
    else
      return TRUE ;

  // case FOUR: THIS has an upper bound that is equal to OTHER's,
  //            and a smaller bound that is also equal
  //this
  //  |         |
  //  |         |
  //
  //  |         |
  //  |         |
  //other
  if ( hiBound == otherHiBound &&
       loBound == otherLoBound )
    if ( isOtherHiInclusive && !isHiInclusive ||
         isOtherLoInclusive && !isLoInclusive )
      return FALSE ; // other is inclusive, I am not
    else
      return TRUE ;

  // case FIVE: NONE OF THE ABOVE
  return FALSE ; // in all other cases, nope
}

// -----------------------------------------------------------------------
// this function does the work of distributing THIS's
// uec/rowcount to the Intervals (in another histogram,
// most likely) in spanList
// -----------------------------------------------------------------------
void
Interval::distributeRowsAndUec (LIST(Interval) & spanList,
                                CostScalar rowsRemaining,
                                CostScalar uecsRemaining,
                                const EncodedValue & loBoundary,
                                const EncodedValue & hiBoundary)
{
  // This function does the work of distributing an Interval's
  // Rows/Uec to a list of sub-Intervals.  It's assumed that all of
  // the sub-intervals (spanList) are spanned (see Interval::span())
  // by the hi/lo boundary info.  Bounds inclusive flags should have
  // been checked before calling this function!
  //
  // The reason it's not a member function, and instead takes four
  // parameters from the Interval, is because we sometimes need to call
  // this function (e.g., see ColStats::removeSingleValue()) where we're
  // subdividing up an Interval into smaller pieces.
  //
  // For the usage of this function from, e.g., ColStats::populateTemplate(),
  // we're working with an Interval from one Histogram and a list of
  // Intervals from another Histogram.
  //
  // So the general use of this function is to not require that the
  // target Intervals and the source Interval NOT NECESSARILY be from
  // different Histograms.  The logic is useful both when the source
  // and target Intervals are from the same Histogram, and when they
  // are not.

  // First we want to see if there are any single-valued intervals
  // in spanList; if so, we will treat these specially

  // We believe these intervals contain more accurate information
  // than the rest of the intervals (this is part of the histogram
  // semantics); thus, we first allocate to each 1 uec & row from
  // those being distributed.  If there is not enough uec/rowcount
  // to give each single-valued interval 1/1, then we distribute
  // what there is to all of them (and give no rowcount/uec
  // to any of the other intervals).

  // For any "left-over" uec/row totals, we divide this
  // evenly between all intervals, pro-rated per interval size
  // (hiBound - loBound)

  // first, check to see if there's anything to do!
  if ( rowsRemaining.isZero() || uecsRemaining.isZero() )
    return ; // nothing to distribute!

  CollIndex singleCount = 0 ; // # of single-valued intervals
  CollIndex i ;
  const CollIndex spanListEntries = spanList.entries();
  for ( i = 0 ; i < spanListEntries ; i++)
    if ( spanList[i].isSingleValued() )
      singleCount++ ;

  CostScalar rowsPerSingle = 0 ; // # of rows to allocate per S.V.I.
  CostScalar uecsPerSingle = 0 ; // # of uecs to allocate per S.V.I.

  if (singleCount > 0 )
    {
      // for small values of uecsRemaining, we have to be careful!
      uecsPerSingle =
        MINOF(uecsRemaining/singleCount, // case where uecsRemaining < singleCount
              1.0) ; // "usual case" (we hope! :-)
      rowsPerSingle =
        MINOF(rowsRemaining/singleCount, // case where uecsRemaining < 1
              ((CostScalar)1.0/uecsRemaining) * rowsRemaining) ; // "usual case"
    } // otherwise these vars keep their initial values above

  // for singleCount == 0, these are no-ops
  uecsRemaining -= uecsPerSingle * singleCount ;
  rowsRemaining -= rowsPerSingle * singleCount ;

  if (uecsRemaining.isLessThanZero())
  {
    CCMPASSERT (uecsRemaining.isGreaterOrEqualThanZero()) ;
    uecsRemaining = 0;
  }

  if (rowsRemaining.isLessThanZero())
  {
    // UECs should not go below zero
    CCMPASSERT (rowsRemaining.isGreaterOrEqualThanZero()) ;
    rowsRemaining = 0;
  }

  // loop through the intervals and distribute the uecs & rowcount
  CostScalar rows;
  CostScalar uec;
  CostScalar factorHi;
  CostScalar factorLo;
  CostScalar factorDiff;
  for ( i = 0 ; i < spanListEntries ; i++ )
  {
    if ( spanList[i].isSingleValued() )
    {
      rows = rowsPerSingle;
      uec  = uecsPerSingle;
    }
    else // we distribute an amount of uecs/rows proportional
    {  // to the size of the interval
      if ( rowsRemaining.isZero() || uecsRemaining.isZero() )
      {
	// don't take any chances with values that're "essentially" zero
	rows = csZero;
	uec  = csZero;
      }
      else
      {
	factorHi =
	  (CostScalar) spanList[i].hiBound().ratio (loBoundary,hiBoundary) ;
	factorLo =
	  (CostScalar) spanList[i].loBound().ratio (loBoundary,hiBoundary) ;


	// The subtraction of two costScalars, which are very close to zero,
	// can lead to overflow error. This happens during comparison of two
	// CostScalars. Round the costScalars to zero, before doing a comparison.
	factorHi.roundIfExactlyZero();
	factorLo.roundIfExactlyZero();
	factorDiff = (factorHi - factorLo).minCsZero();
	rows = rowsRemaining * factorDiff;
	uec  = uecsRemaining * factorDiff;
      }
    }

    spanList[i].setRowsAndUec( rows, uec );
  } // for loop
}

//Compare this interval against its adjacent interval (other).
//The adjacent interval should meet the hi boundary of the
//current interval. The comparison performed is based on
//parameters invokedFrom, and reductionCriterion.
NABoolean Interval::compare(Source invokedFrom,
                            Criterion reductionCriterion,
                            Interval & other)
{
	switch(reductionCriterion)
	{
		case CRITERION1:
			return satisfiesCriterion1(invokedFrom, other);
		case CRITERION2:
			return satisfiesCriterion2(invokedFrom, other);
		default:
			break;
	}
	return FALSE;
}

//this method checks if this interval and the adjacent interval
//which meets this intervals hi boundary, satisfy merge criterion1
NABoolean Interval::satisfiesCriterion1(Source invokedFrom,Interval & other)
{
        // do not compress intervals that contain skew values
	if ( getUec() == 1.0 ) return FALSE;

	//get constant alpha from optdefaults
	double alpha = CURRSTMT_OPTDEFAULTS->histogramReductionConstantAlpha();

	//check validity of alpha, it should be
	//between 0 and 1
	if((alpha > 1.0) || (alpha < 0.0))
		return FALSE;

	//the fudge-factor / Permissible Ratio
	double pr = 0.1;

	//get Permissible Ratio (PR) from optdefaults
	if(invokedFrom == INTERMEDIATE)
	{
		pr = CURRSTMT_OPTDEFAULTS->intermediateHistogramReductionFF();
	}
	else
	{
		pr = CURRSTMT_OPTDEFAULTS->baseHistogramReductionFF();
	};

	//make sure pr is within some sane limit
	//I am assuming QA will try to crash it
	//using a very high or very low pr value
	if(pr < 0)
		return FALSE;

	if(pr > 1000000000000LL)
		return TRUE;

	//get my interval lenght
	double myDistance = hiBound().getDblValue() - loBound().getDblValue();

	if (myDistance <= DBL_MIN)
		return FALSE;

	//get neighor interval's length
	double neighborDistance = other.hiBound().getDblValue() - other.loBound().getDblValue();

	if (neighborDistance <= DBL_MIN)
		return FALSE;

	//get my row count
	double myRowCount = getRowcount().getValue();
	//get neighbor's row count
	double neighborRowCount = other.getRowcount().getValue();

	//get my Unique Entry Count (UEC)
	double myUEC = getUec().getValue();
	//get neighbor's UEC
	double neighborUEC = other.getUec().getValue();

	//Do some checks to guarantee no overflow
	double ourMin = 10 * pow(DBL_MIN,0.25);
	double ourMax = 0.1 * pow(DBL_MAX,0.25);

	if((myDistance < ourMin)||
	   (myDistance > ourMax))
	   return FALSE;

	if((neighborDistance < ourMin)||
	   (neighborDistance > ourMax))
	   return FALSE;

	if((myRowCount < ourMin)||
	   (myRowCount > ourMax))
	   return FALSE;

	if((neighborRowCount < ourMin)||
	   (neighborRowCount > ourMax))
	   return FALSE;

	if((myUEC < ourMin)||
	   (myUEC > ourMax))
	   return FALSE;

	if((neighborUEC < ourMin)||
	   (neighborUEC > ourMax))
	   return FALSE;

	//calculate my row density
	double myRowDensity = myRowCount / myDistance;
	//calculate neighbor's row density
	double neighborRowDensity = neighborRowCount / neighborDistance;

	//calculate my unique entry density
	double myUniqueEntryDensity = myUEC / myDistance;
	//calculate my unique entry density
	double neighborUniqueEntryDensity = neighborUEC / neighborDistance;

	//Do calculatioin to see if the two intervals are approximately equal

	//Do the following calculations here so the results
	//can be reused later, this is done for performance
	double myDistanceSquared = SQUARE(myDistance);
	double neighborDistanceSquared = SQUARE(neighborDistance);
	double alphaSquared = SQUARE(alpha);

	//calculate Acceptable Difference (AD) for row density

	//calculate my tolerance
	//tolerance is defined in the histogram intervals reduction design doc
	double myToleranceSquared = alphaSquared * (myRowCount / myDistanceSquared);
	//calculate neighbor's tolerance
	double neighborToleranceSquared = alphaSquared * (neighborRowCount / neighborDistanceSquared);

	//calculate Relative Permissible Difference (RPD)
	//RPD is defined in the histogram intervals reduction design doc
	double rpdSquared = (pr * (myRowDensity + neighborRowDensity) / 2);
	rpdSquared = SQUARE(rpdSquared);

	//get the square of the acceptable difference (AD)
	double adSquared = rpdSquared + myToleranceSquared + neighborToleranceSquared;

	//calculate difference in row density
	double differenceSquared = myRowDensity - neighborRowDensity;
	differenceSquared = SQUARE(differenceSquared);

    //check if difference in row density is within acceptable limits
    //to consider it approximately equal
	if(!(differenceSquared < adSquared))
		return FALSE;

    //calculate Acceptable Difference (AD) for Unique Entry density

    //calculate my tolerance
    myToleranceSquared = alphaSquared * (myUEC / myDistanceSquared);
    //calculate neighbors tolerance
    neighborToleranceSquared = alphaSquared * (neighborUEC / neighborDistanceSquared);

    //calculate Relative Permissible Difference (RPD)
	//RPD is defined in the histogram intervals reduction design doc
    rpdSquared = (pr * (myUniqueEntryDensity + neighborUniqueEntryDensity) / 2);
    rpdSquared = SQUARE(rpdSquared);

    //get the square of the acceptable difference (AD)
	adSquared = rpdSquared + myToleranceSquared + neighborToleranceSquared;

	//calculate difference in row density
	differenceSquared = myUniqueEntryDensity - neighborUniqueEntryDensity;
    differenceSquared = SQUARE(differenceSquared);

	//check if difference in row density is within acceptable limits
	//to consider it approximately equal
	if(!(differenceSquared < adSquared))
		return FALSE;

	return TRUE;
}

//This method checks if this interval and the adjacent interval
//which meets this interval on the hi boundary, satisfy
//merge criterion 2
NABoolean Interval::satisfiesCriterion2(Source invokedFrom, Interval & other)
{
        // do not compress intervals that contain skew values
	if ( getUec() == 1.0 ) return FALSE;

    //get constant alpha from optdefaults
	double alpha = CURRSTMT_OPTDEFAULTS->histogramReductionConstantAlpha();

	//check validity of alpha, it should be
	//between 0 and 1
	if((alpha > 1.0) || (alpha < 0.0))
		return FALSE;

	//the fudge-factor / Permissible Ratio
	double pr = 0.1;

	//get Permissible Ratio (PR) from optdefaults
	if(invokedFrom == INTERMEDIATE)
	{
		pr = CURRSTMT_OPTDEFAULTS->intermediateHistogramReductionFF();
	}
	else
	{
		pr = CURRSTMT_OPTDEFAULTS->baseHistogramReductionFF();
	};

	//make sure pr is within some sane limit
	//I am assuming QA will try to crash it
	//using a very high or very low pr value
	if(pr < 0)
		return FALSE;

	if(pr > 1000000000000LL)
		return TRUE;

	//get my row count
	double myRowCount = getRowcount().getValue();
	//get neighbor's row count
	double neighborRowCount = other.getRowcount().getValue();

	//get my Unique Entry Count (UEC)
	double myUEC = getUec().getValue();
	//get neighbor's UEC
	double neighborUEC = other.getUec().getValue();

    //Do some checks to guarantee no overflow
	double ourMin = 10 * pow(DBL_MIN,0.25);
	double ourMax = 0.1 * pow(DBL_MAX,0.25);

	if((myRowCount < ourMin)||
	   (myRowCount > ourMax))
	   return FALSE;

	if((neighborRowCount < ourMin)||
	   (neighborRowCount > ourMax))
	   return FALSE;

	if((myUEC < ourMin)||
	   (myUEC > ourMax))
	   return FALSE;

	if((neighborUEC < ourMin)||
	   (neighborUEC > ourMax))
	   return FALSE;

	//calculate my rows per unique entry
	double myRowsPerUE = myRowCount / myUEC;
	//calculate neighbor's rows per unique entry
	double neighborRowsPerUE = neighborRowCount / neighborUEC;

    //Do calculation to see if the two intervals are approximately equal

	//Do the following calculations here so the results
	//can be reused later, this is done for performance
	double alphaSquared = SQUARE(alpha);

	//calculate Acceptable Difference (AD) for row density

	//calculate my tolerance
	//tolerance is defined in the histogram intervals reduction design doc
	double myToleranceSquared = alphaSquared * (myRowCount / SQUARE(myUEC));
	//calculate neighbor's tolerance
	double neighborToleranceSquared = alphaSquared * (neighborRowCount / SQUARE(neighborUEC));

	//calculate Relative Permissible Difference (RPD)
	//RPD is defined in the histogram intervals reduction design doc
	double rpdSquared = (pr * (myRowsPerUE + neighborRowsPerUE) / 2);
	rpdSquared = SQUARE(rpdSquared);

	//get the square of the acceptable difference (AD)
	double adSquared = rpdSquared + myToleranceSquared + neighborToleranceSquared;

	//calculate difference in row density
	double differenceSquared = myRowsPerUE - neighborRowsPerUE;
	differenceSquared = SQUARE(differenceSquared);

    //check if difference in row density is within acceptable limits
    //to consider it approximately equal
	if(!(differenceSquared < adSquared))
		return FALSE;

	return TRUE;
}

void
Interval::display (FILE *f, const char * prefix, const char * suffix) const
{
  fprintf (f, "%sLoBound ", prefix);
  if (isLoBoundInclusive())
    fprintf (f, "<= ");
  else
    fprintf (f, "<  ");
  loBound().display(f);
  fprintf (f, " : rows=%f,uec=%f %s\n",
	   getRowcount().value(), getUec().value(), suffix);

  fprintf (f, "%sHiBound ", prefix);
  if (isHiBoundInclusive())
    fprintf (f, "<= ");
  else
    fprintf (f, "<  ");
  hiBound().display(f);
}

// -----------------------------------------------------------------------
//  methods on Histogram class
// -----------------------------------------------------------------------

// simple helper class for ::createMergeTemplate, ::condenseToPartitionBoundaries

class HistIntVal
{
public:
  HistIntVal (const HistInt & init) :
       val_(init.getBoundary()), incl_(init.isBoundIncl()), hash_(init.getHash()) {}

  HistIntVal (const HistIntVal & other) :
       val_(other.val_), incl_(other.incl_), hash_(other.hash_) {}

  HistInt buildHistInt() { return HistInt(val_, incl_, hash_) ; }

  NABoolean operator == (const HistIntVal & rhs) const
  { return (val_ == rhs.val_ && incl_ == rhs.incl_) ; }

  NABoolean operator != (const HistIntVal & rhs) const
  { return NOT (*this == rhs) ; }

  NABoolean operator <  (const HistIntVal & rhs) const
  {
    if ( (val_  < rhs.val_) ||
         (val_ == rhs.val_  && incl_==FALSE && rhs.incl_==TRUE) )
      return TRUE ;
    else
      return FALSE ;
  }

  NABoolean operator <= (const HistIntVal & rhs) const
  {
    if ( (val_  < rhs.val_) ||
         (val_ == rhs.val_  && (incl_ == FALSE || rhs.incl_==TRUE)) )
      return TRUE ;
    else
      return FALSE ;
  }

  // the data members -- public for convenience
  const EncodedValue & val_ ;
  const UInt32         hash_ ;
  const NABoolean      incl_ ;

private:
  HistIntVal() ; // never create an uninitialized one!
};

// -----------------------------------------------------------------------
//  createMergeTemplate
//  Given two histograms, create a Template histogram to use in subsequent
//  merge operations involving those two histograms.  E.g., If the two
//  histograms are involved in a equality-join, or need to be Unioned due
//  to an OR.
//  equiMerge indicates whether or not the operation being done involves
//  an equality based constraint where only overlapping intervals are of
//  interest.
// -----------------------------------------------------------------------
HistogramSharedPtr
Histogram::createMergeTemplate (const HistogramSharedPtr& otherHistogram,
                                NABoolean equiMerge) const
{
  HistogramSharedPtr histTemplate(new (HISTHEAP) Histogram (HISTHEAP));

  // -----------------------------------------------------------------------------
  // STEP 0 : handle the simplest case first : if one of the histograms has
  // zero or 1 Intervals, create template with the other histogram. If
  // both histograms have 0 or 1 intervals, then we create an empty template
  // and return. There is nothing that we can do here
  // -----------------------------------------------------------------------------
  if ( this->entries() < 2 || otherHistogram->entries() < 2 )
    {
      if ( equiMerge )
        return histTemplate ; // no qualifying intervals
      else
        {
          if ( ( this->entries() < 2 ) && ( otherHistogram->entries() >= 2 ) )
          {
            CCMPASSERT ( this->entries() >= 2 ) ;
            histTemplate = new (HISTHEAP) Histogram (*otherHistogram, HISTHEAP) ;
          }
          else
          {
            if ((otherHistogram->entries() < 2) && ( this->entries() >= 2 ) )
            {
              CCMPASSERT ( otherHistogram->entries() >= 2 ) ;
              histTemplate = new (HISTHEAP) Histogram (*this, HISTHEAP) ;
            }
          }

          for (CollIndex i = 0 ; i < histTemplate->entries() ; i++ )
            (*histTemplate)[i].setCardAndUec(0,0) ;

          return histTemplate ;
        }
    }

  // OK, at this point we know both histograms have Intervals

  // -----------------------------------------------------------------------------
  // STEP 1: first, assume it's not an equiMerge, so just collect all
  // intervals and put 'em in histTemplate
  // -----------------------------------------------------------------------------

  // keep track of the minimum's because we're at the beginning of the
  // array now
  HistIntVal thisMin  (this->firstHistInt()) ;
  HistIntVal otherMin (otherHistogram->firstHistInt()) ;

  CollIndex thisEntries = this->entries() ;
  CollIndex otherEntries = otherHistogram->entries() ;

  CollIndex iT = 0 ; // "index of this"
  CollIndex iO = 0 ; // "index of other"

  // to keep this loop simpler, we do not allow the indices iT,iO to go beyond
  // the size of their respective arrays -- instead,
  NABoolean thisDone = FALSE ;
  NABoolean otherDone = FALSE ;

  // this loop finishes when we've processed every HistInt in each histogram
  while (1)
    {
      if (iT >= thisEntries)
      {
        // index of this is greater than this histogram entries. assume
        // this is done
        CCMPASSERT ( iT < thisEntries ) ;  // sanity check
        iT = thisEntries - 1;
        thisDone = TRUE;
      }

      if (iO >= otherEntries)
      {
        CCMPASSERT ( iO < otherEntries ) ; // sanity check
        iO = otherEntries - 1;
        // assume other histogram is done
        otherDone = TRUE;
      }

      HistIntVal thisInt ((*this)[iT]) ;
      HistIntVal otherInt ((*otherHistogram)[iO]) ;

      if ( (thisInt < otherInt) && NOT thisDone )
        {
          histTemplate->insert ( thisInt.buildHistInt() ) ;
          iT++ ;
        }
      else if ( (otherInt < thisInt) && NOT otherDone )
        {
          histTemplate->insert ( otherInt.buildHistInt() ) ;
          iO++ ;
        }
      else if ( NOT thisDone ) // thisInt == otherInt
        {
          histTemplate->insert ( thisInt.buildHistInt() ) ;
          iT++ ;
          iO++ ;
        }
      else
        {
          if (otherDone)
          {
            CCMPASSERT ( NOT otherDone ) ;
            break;
          }
          histTemplate->insert ( otherInt.buildHistInt() ) ;
          iO++ ;
        }

      if ( iT == thisEntries )
        {
          iT-- ;
          thisDone = TRUE ;
        }
      if ( iO == otherEntries )
        {
          iO-- ;
          otherDone = TRUE ;
        }

      // check: have we processed every HistInt in both lists?
      if ( thisDone && otherDone )
        break ;
    }

    NABoolean validHistTemp = TRUE;

  // sanity check
    if  ( histTemplate->entries() < 2 ||
          histTemplate->entries() > thisEntries+otherEntries)
    {
      // if the histogram template created has incorrect intervals, then just undo
      // whatever has been done till now, and create a single interval histogram
      // with uninitialized min / max 
      CCMPASSERT ( histTemplate->entries() >= 2 &&
              histTemplate->entries() <= thisEntries+otherEntries) ; 
      validHistTemp = FALSE;
    }

  // -----------------------------------------------------------------------------
  // STEP 2: now, we handle the case that it's an equiMerge -- basically,
  // we may need to remove some intervals from the template created
  // in step 1
  // -----------------------------------------------------------------------------

  EncodedValue minVal (UNINIT_ENCODEDVALUE) ;
  EncodedValue maxVal (UNINIT_ENCODEDVALUE) ;

  if ( equiMerge )
    {
      // In the loop above we have already made sure that iT/oT == thisEntries - 1/
      // otherEntries-1. 
      // just in case, they are not. Make them equal now
      if (iT != thisEntries-1)
      {
        CCMPASSERT (iT == thisEntries-1) ;
        iT = thisEntries-1;
      }

      if (iO != otherEntries-1)
      {
        CCMPASSERT (iO == otherEntries-1) ;
        iO = otherEntries-1;
      }

      HistIntVal thisMax  ((*this)[iT]) ;
      HistIntVal otherMax ((*otherHistogram)[iO]) ;

      // time to check for case where there is no overlap whatsoever :
      if ( thisMax <= otherMin OR otherMax <= thisMin )
        {
          return HistogramSharedPtr(new (HISTHEAP) Histogram (HISTHEAP));
        }

      HistIntVal overlapMin (thisMin < otherMin ? otherMin : thisMin ) ;
      HistIntVal overlapMax ( thisMax < otherMax ? thisMax : otherMax ) ;

      iT = 0 ;

      // set the min and max in the histogram template, if it is valid so far
      // else just use min and max values collected from two histograms to create
      // a single interval histogram template
      if (validHistTemp)
      {
        // first, remove the HistInts in histTemplate that are too small
        while ( HistIntVal((*histTemplate)[iT]) < overlapMin )
          {
            histTemplate->removeAt(iT) ; // iT==0
          }
        minVal = overlapMin.val_;

          // when you come out of the loop, the teh interval we are at should be equal to minimum
          if ( HistIntVal((*histTemplate)[iT]) != overlapMin )
          {
            CCMPASSERT ( HistIntVal((*histTemplate)[iT]) == overlapMin ) ; // sanity check
            (*histTemplate)[iT].setBoundary(minVal);
          }
        iT++ ;

        // now, increment iT until we reach the HistInt that's equal to overlapMax
        while ( HistIntVal((*histTemplate)[iT]) < overlapMax )
        {
          iT++ ;
        }

        maxVal = overlapMax.val_;
        // when you come out of the loop, the the interval we are at should be equal to maximum
        if ( HistIntVal((*histTemplate)[iT]) != overlapMax )
        {
          CCMPASSERT ( HistIntVal((*histTemplate)[iT]) == overlapMax ) ; // sanity check
          (*histTemplate)[iT].setBoundary(maxVal);
        }
 

        // Now increment iT to point to next interval
        iT++;

        // Remove any values that are greater than overlapMax and the
        // value of iT should be equal to histTemplate->entries() -1
        // if there are cases where two intervals have the same boundary
        // remove the second one, as we do not want to have more than one
        // intervals with the same value
        while (( iT < histTemplate->entries() ) &&
               ( overlapMax <= HistIntVal((*histTemplate)[iT]) ) )             
          histTemplate->removeAt(iT);


        // finally, if the last interval represents NULLs (since we're an
        // equiMerge), then the NULL interval must be removed to retain SQL
        // semantics ("nothing is equal to NULL"). This is possible only if
        // there are atleast 2 intervals left
        if ((histTemplate->entries() >= 2) && ( histTemplate->isNullInstantiated() ))
          {
            histTemplate->removeNullInterval() ;
          }
      }
    } // if equi-merge

    // should at least contain overlapMin, overlapMax !
    // if not, create a new template with one interval
    if ( (histTemplate->entries() == 1) ||
         !validHistTemp)
    {
       // sanity check
      CCMPASSERT(histTemplate->entries() != 1);
      // clear whatever has been done till now
      histTemplate->clear();
      // insert an interval with boundaries equal to overlapMin and overlapMax
      histTemplate->insertZeroInterval(minVal, maxVal, TRUE);
    }

  return histTemplate;
}  //  createMergeTemplate

// -----------------------------------------------------------------------
// populateTemplate
// Update THIS's histogram template with the interval-adjusted data
// from the input histogram OTHER.  This routine assumes that no data
// is present in THIS, other than its interval boundaries.
//
// The special case of single-valued intervals makes this routine more
// complex than might be expected.  When individual OTHER intervals
// map to a set of THIS intervals that includes single-valued intervals,
// all of those spanned intervals must be processed as a group.
// A single-valued interval is represented by two adjoining intervals with
// identical boundary values.
//
// Those intervals are special because of the semantics, or convention, for
// predicates of the form 'a=<literal>' which presumes that there will be
// values of 'a' that are equal to the specified <literal>.
//
// This routine depends upon the fact that for the Overlapping Portion
// of THIS and OTHER, the intervals' boundaries in OTHER are a proper
// subset of those in THIS.
//
// This routine is even more complicated because we have to be
// careful to deal with NULL intervals properly
// -----------------------------------------------------------------------
void
ColStats::populateTemplate (const ColStatsSharedPtr& otherStats)
{
  if ( histogram_->numIntervals() == 0 ||
       otherStats->getHistogram()->numIntervals() == 0 )
    return ;

  // this
  //      |    |    |    |
  //      |    |    |    |
  // 0    2    2    4    5    6  <-- boundary values
  // |         |         |    |
  // |         |         |    |
  // other
  //
  // notice how one 'other' Interval spans potentially
  // multiple 'this' intervals; also notice that in the
  // "overlap area" [2,5], there aren't any interval boundaries
  // in 'other' that are not also in 'this'
  // --> this is achieved in createMergeTemplate()

  // the plan:
  // 0. THIS is the template being populated by OTHER
  // 1. calculate which intervals in THIS are spanned by
  //    the OTHERinterval; we start by looking at THISinterval,
  //    then step through until we hit an Interval in this
  //    whose hiBound is >= OTHERinterval's hiBound
  // 2. adjust the intervals in THIS to have matching uec/rowcount totals
  // 3. get the next OTHER interval
  // 4. get the next THIS interval (the one after the last one in
  //    spanList)

  HistogramSharedPtr thisHist = this->getHistogram() ;
  HistogramSharedPtr otherHist = otherStats->getHistogram() ;

  Interval thisInterval = thisHist->getFirstInterval() ;
  Interval otherInterval = otherHist->getFirstInterval() ;

  CostScalar rowRedFactor = otherStats->getRedFactor() ;
  CostScalar uecRedFactor = otherStats->getUecRedFactor() ;

  NAList<Interval> spanList(CmpCommon::statementHeap());
  NABoolean notIncrementedThisIter ;

  // we stop iterating after we process the last interval in THIS list
  //
  // NB: this loop is not *completely* optimal; but considering all of the
  // complicated things we have to keep track of to get it right (in
  // particular, the possibility of NULL intervals in one or both of the
  // histograms), it's still reasonably clear and understandable, so the
  // present state is acceptable
  //
  // *** see the Histogram design document for an explanation of everthing
  // *** that's going on in this function!
  while ( thisInterval.isValid() )
    {
      spanList.clear() ; // start with a clean slate
      notIncrementedThisIter = TRUE ;

      while ( otherInterval.spans (thisInterval) )
        {
          spanList.insert(thisInterval) ;
          thisInterval.next() ;
          notIncrementedThisIter = FALSE ;
        }

      // if none are spanned, then we started with an "early"
      // otherInterval (or, we're near the end of the process and are in
      // the middle of handling the NULL values)
      if (spanList.entries() > 0)
        {
          if ( otherInterval.getUec().isGreaterThanZero() AND
               otherInterval.getRowcount().isGreaterThanZero() )
		  {
            // only try to distribute non-zero values!

			CostScalar iRows = rowRedFactor * otherInterval.getRowcount();
			CostScalar iUec = otherInterval.getUec();

			iUec = MINOF(iRows, iUec);

            Interval::distributeRowsAndUec (spanList,
                                            iRows,
                                            iUec,
                                            otherInterval.loBound(),
                                            otherInterval.hiBound()) ;
		  }
          // thisInterval is the next one we're going to try to span
        }

      // unless we already have, we need to increment one or other of the
      // Intervals, else we have the possibility of an infinite loop
      if ( notIncrementedThisIter == TRUE )
        {
          // if OTHER is larger than THIS, then it would be wrong to increment THIS
          // (unless we've only got NULL intervals left)
          if ( !thisInterval.isLast() AND
               // case a : OTHER  > THIS
               (otherInterval.hiBound() >  thisInterval.hiBound()) OR
               // case b : OTHER == THIS, boundary-inclusiveness makes it >
               (otherInterval.hiBound() == thisInterval.hiBound() AND
                otherInterval.isHiBoundInclusive() == TRUE AND
                thisInterval.isHiBoundInclusive() == FALSE) )
            thisInterval.next() ;
          else if ( !otherInterval.isLast() )
            otherInterval.next() ;
          else
            thisInterval.next() ;
        }
    }

  //
  // cleanup: how many rows & uecs are in the template?
  //
  // NB: we've already applied the reduction factors above (in the call to
  // distributeRowsAndUec(); from now on, they're both one
  //
  setRowsAndUecFromHistogram() ;
  CostScalar newRowcount = getRowcount() ;

  //
  // cleanup #2 : did we end up populating THIS with enough
  // rows from OTHER?
  //

  // $$$ this fraction is ad-hoc (i.e., KLUDGE)
  const CostScalar MIN_POPULATED_FACTOR = CostScalar(0.0005) * otherStats->getRowcount();
  CostScalar requiredMinimum;
  // The below code is checking for a value between 1 and 10 and it
  // has been added with regard to the KLUDGE (max 10)mentioned below if the
  // kludge is changed then this code needs to be changed
  if ( MIN_POPULATED_FACTOR.isGreaterThanZero() )
  {
    requiredMinimum = MIN_POPULATED_FACTOR * otherStats->getRedFactor() ;
    if ( requiredMinimum.getValue() > 10.0 )
      requiredMinimum = CostScalar(10.0);
    else
      requiredMinimum.minCsOne();
  }
  else
    requiredMinimum = csOne;

  //CostScalar requiredMinimum = MIN_POPULATED_FACTOR * otherStats->getRedFactor() ;
  //requiredMinimum = MIN_ONE (requiredMinimum) ;   // want at least one row!
  //requiredMinimum = MINOF (requiredMinimum, 10) ; // $$$ kludge^n

  if ( newRowcount < requiredMinimum && newRowcount.isGreaterThanZero() )
    {
      // TOO FEW ROWS!  NEED TO RECOVER!

      // calculate the difference between the required minimum number of
      // rows in the result histogram; then apply this factor to all intervals
      // of the result histogram

      // then, do the same thing with the uec

      // first, calculate a reasonable number of UEC to survive
      CostScalar calculatedUec =
        ColStatDesc::calculateCorrectResultUec (otherStats->getRowcount(),
                                                requiredMinimum,
                                                otherStats->getTotalUec()) ;
      calculatedUec = MINOF (calculatedUec, requiredMinimum) ; // uec <= rc
      CostScalar newTotalUec = getTotalUec() ;

      CostScalar rowFactor = requiredMinimum / newRowcount ; // should be > 1
      CostScalar uecFactor ;
      if ( newTotalUec < calculatedUec && newTotalUec.isGreaterThanZero() ) // avoid div-by-zero!
        uecFactor = calculatedUec / newTotalUec ; // should be > 1
      else
        uecFactor = csOne ; // don't reduce, in this case

      newTotalUec = MAXOF (newTotalUec, calculatedUec) ;

      CollIndex i ;
      CostScalar rows, uec ;
      for ( i = 1 ; i < histogram_->entries() ; i++ )
        {
          rows = (*histogram_)[i].getCardinality() ;
          uec  = (*histogram_)[i].getUec() ;
          (*histogram_)[i].setCardAndUec ( rows * rowFactor, uec * uecFactor ) ;
        }

      // update the aggregate information, though no one's likely to look at it
      setRowsAndUec (requiredMinimum, newTotalUec) ;

      // the result is now fake, though no one's likely to look at it
      setFakeHistogram (TRUE) ;
   }
   else if ( newRowcount.isZero() AND requiredMinimum.isGreaterThanZero() )
   {
     // create a 1-interval histogram, no fuss
     CostScalar calculatedUec =
       ColStatDesc::calculateCorrectResultUec (otherStats->getRowcount(),
                                               requiredMinimum,
                                               otherStats->getTotalUec()) ;
     calculatedUec = MINOF (calculatedUec, requiredMinimum) ; // uec <= rc

     // now, condense the histogram to a single interval
     histogram_->condenseToSingleInterval() ;
     setRowsAndUec (requiredMinimum, calculatedUec) ;
     setFakeHistogram (TRUE) ;

     // populate that first interval with rc/uec
     Interval first = histogram_->getFirstInterval() ;
     first.setRowsAndUec (requiredMinimum, calculatedUec) ;
   }
}

// --------------------------------------------------------------------
// ColStats::condenseToPartitionBoundaries
//
// utility routine used by ColStatDescList::divideHistogramAtPartitionBoundaries()
//
// Given two histograms (THIS & PARAM), merges all intervals in THIS
// that do not occur in PARAM.
//
// Note that we automatically merge-away SVI's, since they do not occur
// in partition-key lists
//
// Note also that if THIS has HistInts that are lower than the minimum of
// PARAM (or, similarly, that are larger than the max), then we trust that
// the histogram is out-of-date with respect to the partitioning
// boundaries, and we simply set the boundary-values equal to the ones
// specified by the partitioning key.  Note that we do this as a separate
// step since it's not clear whether we'll get min/max info from the
// partitioning key boundary value information anyway ...
// --------------------------------------------------------------------

NABoolean
Histogram::condenseToPartitionBoundaries (const HistogramSharedPtr& partitionBoundaries)
{
  // THIS was built from a call to ::createMergeTemplate of partitionBoundaries
  // and another histogram; at the very least, there are as many intervals in
  // THIS as there are in partitionBoundaries (probably more)
  // if the number of source histogram intervals is less than the resultant histogram
  // intervals, return false, indicating that the histogram cannot be condensed
  // to partition boundaries
  if ( this->entries() < partitionBoundaries->entries() ) 
  {
    CCMPASSERT ( this->entries() >= partitionBoundaries->entries() ) ;
    return FALSE;
  }

  // algorithm :
  //
  // first, remove all HistInts in THIS which have boundaries > max, < min of
  // partitionBoundaries
  //
  // then, loop over the HistInts in partitionBoundaries
  //   iter through the HistInts in THIS whose boundary value <= the pB[i]
  //     add up the rows, uec, set the HistInt == pB[i] to have these sums as rows/uec

  // first, remove any entries in THIS that are -less- than any in
  // partitionBoundaries
  const HistIntVal firstBoundary ( (*partitionBoundaries)[0] ) ;
  while ( this->entries() > 0 && HistIntVal ((*this)[0]) < firstBoundary )
    this->removeAt(0) ;

  // next, remove any entries in THIS that are -larger- than any in
  // partitionBoundaries
  const HistIntVal lastBoundary ( (*partitionBoundaries)[partitionBoundaries->entries()-1] ) ;
  while ( this->entries() > 0 && lastBoundary < HistIntVal((*this)[this->entries()-1]) )
    this->removeAt (this->entries()-1) ;


  // now, iterate over the partitionBoundaries
  // --> for each one, merge any "extra" HistInts that provide finer
  //     granularity than we want (i.e., any HistInts whose boundaries aren't in
  //     the partition-boundary list)
  CollIndex partIdx ;

  for ( partIdx = 1 ;
        partIdx < partitionBoundaries->entries() && partIdx < this->entries() ;
        partIdx++)
    {
      const HistIntVal partBoundary ( (*partitionBoundaries)[partIdx] ) ;

      CostScalar num_rows = 0 ;
      CostScalar num_uec = 0 ;

      // find the HistInt whose boundary is equal to partBoundary
      while ( partIdx < this->entries() )
        {
          const HistIntVal thisBoundary ((*this)[partIdx]) ;
          if ( partBoundary < thisBoundary )
          {
            // this should not happen, if it did, then we messed up somewhere
            // return FALSE
            CCMPASSERT ( thisBoundary <= partBoundary ) ; // sanity check
            return FALSE;
          }

          num_rows += (*this)[partIdx].getCardinality() ;
          num_uec  += (*this)[partIdx].getUec() ;
          if ( thisBoundary == partBoundary )
            {
              (*this)[partIdx].setCardAndUec (num_rows, num_uec) ;
              break ; // break out to outer while loop --> do rows/uec for next partn bound
            }
          else
            {
              this->removeAt(partIdx) ;
            }
        } // while loop

    } // for loop over HistInts in partitionBoundaries

  // make sure our result is what we expect!
#ifndef NDEBUG
  CCMPASSERT (this->entries() == partitionBoundaries->entries() ) ;
  CollIndex i ;
  for ( i = 0 ; i < this->entries() ; i++ )
    CCMPASSERT ( HistIntVal ((*this)[i]) == HistIntVal ((*partitionBoundaries)[i]) ) ;
#endif

  return TRUE ;
}


// --------------------------------------------------------------------
// ColStats::insertZeroInterval
// Insert an interval if number of intervals is zero, or histogram is NULL
// with boundariues of interval equal to minimum and max of colstats
// and rowcount and uec equal to aggregate rowcount and uec of colstats
// ---------------------------------------------------------------------
void
ColStats::insertZeroInterval()
{
  if (histogram_ == NULL)
    histogram_ = HistogramSharedPtr(new (HISTHEAP) Histogram(HISTHEAP));

  histogram_->insertZeroInterval(getMinValue(), getMaxValue(), TRUE);
  Interval first = histogram_->getFirstInterval();
  first.setRowsAndUec(getRowcount(), getTotalUec());
  return;
}
// --------------------------------------------------------------------
// ColStats::removeRedundantEmpties
//
// Following operations such as joins a histogram may have a series of
// intervals containing zero rows.  In that situation, compress out the
// redundant empty histogram intervals.
// --------------------------------------------------------------------
void
ColStats::removeRedundantEmpties()
{
  // if the NULL interval has zero rows, remove it
  if ( getNullCount().isZero() )
    removeNullInterval() ;

  if (histogram_->numIntervals() == 0)
    {
      // no intervals in the histograms. 
      return ;
    }

  Interval iter = histogram_->getFirstInterval() ;
  Interval next = histogram_->getNextInterval (iter) ;

  // rows    0   0   1   0   0   0   1   0
  //       |   |   |   |   |   |   |   |   |
  //       |   |   |   |   |   |   |   |   |
  // int.    1   2   3   4   5   6   7   8

  // the following loop will remove the interval boundary
  // between a pair of adjoining zero-row intervals

  while (!iter.isLast())
    {
      if ( iter.getRowcount().isZero() AND
           iter.getLoIndex() == 0 )
        {
          // the list of HistInts started with two HistInts that had 0
          // rowcount --> remove the lower of these
          histogram_->removeAt (0) ;
          iter = histogram_->getFirstInterval() ;
          next = histogram_->getNextInterval (iter) ;
        }
      else if ( iter.getRowcount().isZero()  AND
                next.getRowcount().isZero() )
        {
          histogram_->removeAt (next.getLoIndex()) ;
          next = histogram_->getNextInterval (iter) ;
          iter.refreshHiInt();
          setShapeChanged (TRUE) ;
        }
      else
        {
          iter = next ;
          next = histogram_->getNextInterval (iter) ;
        }
    }
    // at the end of this loop, there are at least two HistInts
    // remaining. If nit create an intervak with aggregate rowcount and UEC
    // and boundary equal to min and max of the colstats
    if (histogram_->numIntervals() == 0)
    {
      CCMPASSERT (histogram_->numIntervals() != 0) ;
      insertZeroInterval();
      return ;
    }

    // special case #1 : 2 intervals, second one is NULL
    // --> to maintain proper histogram semantics, must remove the non-NULL HistInt
    iter = histogram_->getFirstInterval() ;
    iter.next() ;
    if ( histogram_->numIntervals() == 2 &&
       iter.isNull() )
    {
      histogram_->removeAt(0) ;
    }

  // special case #2 : last interval has 0-rows
  // NB: we already handled the zero-rows-in-NULL-interval case earlier, so we don't
  // need to worry about it any more.
  if ( histogram_->numIntervals() > 1 ) // we handle the one-interval & zero-row case next
    {
      iter = histogram_->getLastInterval() ;
      if ( iter.getRowcount().isZero() )
        {
          histogram_->removeAt (iter.getLoIndex()+1) ;
        }
    }

  // special case #3 : one interval, 0-rows in it
  if ( histogram_->numIntervals() > 0 )
    {
      iter = histogram_->getFirstInterval() ;
      if ( histogram_->numIntervals() == 1 &&
           iter.getRowcount().isZero() )
        {
          clearHistogram() ;
        }
    }

  // The first Interval of the Histogram might be a 0-row (as might the
  // last), but this does not violate our Histogram
  // semantics.  So we don't bother checking for this situation.

  setMaxMinValuesFromHistogram() ;

} // removeRedundantEmpties

// -----------------------------------------------------------------------
//  Histogram display methods
// -----------------------------------------------------------------------

// to be called from the debugger
void
Histogram::display() const
{
  Histogram::print();
}

void
Histogram::print (FILE *f, const char * prefix, const char * suffix,
                  CollHeap *c, char *buf) const
{
  Space * space = (Space *)c;
  char mybuf[1000];
  snprintf(mybuf, sizeof(mybuf), "%sHistogram : %s\n", prefix, suffix);
  PRINTIT(f, c, space, buf, mybuf);
  if (entries() != 0)
    {
      for (CollIndex i = 0; i < entries(); i++)
      (*this)[i].display(f, "     ", "", c, buf);
    }
}

THREAD_P Int64 ColStats::fakeHistogramIDCounter_=ColStats::USTAT_HISTOGRAM_ID_THRESHOLD;

NABoolean ColStats::isUSTATGeneratedHistID(ComUID id)
{
  return id <= ComUID(USTAT_HISTOGRAM_ID_THRESHOLD);
}

ComUID ColStats::nextFakeHistogramID()
{
  return ComUID(++fakeHistogramIDCounter_);
}

// -----------------------------------------------------------------------
//  methods on ColStats class
// -----------------------------------------------------------------------
ColStats::ColStats (ComUID& histid, CostScalar uec, CostScalar rowcount,
                    CostScalar baseRowCount,
                    NABoolean unique, NABoolean shapeChanged,
                    const HistogramSharedPtr& dist,
                    NABoolean modified, CostScalar rowRedFactor,
                    CostScalar uecRedFactor, Int32 avgVCharSize,
                    NAMemory* heap ,
                    NABoolean allowMinusOne) :
              columns_(heap),
	      colPositions_(heap),
              minValue_(UNINIT_ENCODEDVALUE),
              maxValue_(UNINIT_ENCODEDVALUE),
              maxFreq_(-1.0),
              scaleFactor_(1.0),
              _flags_(0),
              heap_(heap),
              histogramID_(histid),
              frequentValues_(heap), 
              avgVarcharSize_(avgVCharSize),
              mcSkewedValueList_(heap),
              afterFetchIntReductionAttempted_(FALSE)
{
  // this assertion is invalid: stmt heap is null during static compilation
  //  CMPASSERT( heap != NULL ) ;

  baseUec_     = uec;
  uecBeforePred_ = uec;    // uec before predicates
  sumOfMaxUec_ = 0 ; // only used during join synthesis
  setRedFactor    (rowRedFactor) ;
  setUecRedFactor (uecRedFactor) ;
  setBaseRowCount (baseRowCount) ;
  setRowsAndUec   (rowcount, uec, allowMinusOne) ;

  histogram_ = dist;

  setUnique       (unique) ;
  setAlmostUnique (unique);
  setModified     (modified) ;
  setShapeChanged (shapeChanged) ;
    setFakeHistogram (FALSE) ;
    setOrigFakeHist (FALSE) ;
  setObsoleteHistogram (FALSE) ;
  setIsCompressed (FALSE);

  // these three flags are set during histogram synthesis
  setMinSetByPred  (FALSE) ;
  setMaxSetByPred  (FALSE) ;
  setRecentJoin    (FALSE) ;
  setUpStatsNeeded (FALSE) ;
  setVirtualColForHist ( FALSE );
  setIsARollingColumn (FALSE);
  setIsColWithBndryConflict (FALSE);
  setSelectivitySetUsingHint (FALSE);

  setMaxMinValuesFromHistogram() ;

  maxIntervalCount_ = 0 ;
  populateColumnSetFromColumnArray();
}

ColStats::ColStats (const ColStats &other, NAMemory* h, NABoolean assignColArray) :
             columns_(h),
	     colPositions_(other.colPositions_,h),
             minValue_(UNINIT_ENCODEDVALUE),
             maxValue_(UNINIT_ENCODEDVALUE),
             maxFreq_(-1.0),
             scaleFactor_(1.0),
             heap_(h),
             histogramID_ (other.histogramID_),
             frequentValues_(h),
             mcSkewedValueList_(other.mcSkewedValueList_, h)
{
  if (assignColArray)
    columns_         = other.columns_;

  // copy the reference only to the Histogram class
  histogram_       = other.histogram_;
  minValue_        = other.minValue_ ;
  maxValue_        = other.maxValue_ ;
  maxFreq_		   = other.maxFreq_;
  scaleFactor_	   = other.scaleFactor_;
  baseUec_         = other.baseUec_;
  uecBeforePred_   = other.uecBeforePred_;
  baseRowCount_    = other.baseRowCount_;
  sumOfMaxUec_     = other.sumOfMaxUec_ ;
  frequentValues_  = other.frequentValues_;
  afterFetchIntReductionAttempted_ = other.afterFetchIntReductionAttempted_;

  setRedFactor     (other.rowRedFactor_) ;
  setUecRedFactor  (other.uecRedFactor_ ) ;

    // Make sure we can make a copy of a UDF one where we defaulted the UEC 
    // to minusOne.
  setRowsAndUec    (other.rowcount_, other.totalUec_, other.totalUec_ == csMinusOne) ;

  _flags_ = other._flags_;
  maxIntervalCount_ = other.maxIntervalCount_ ;
  avgVarcharSize_ = other.avgVarcharSize_;
  populateColumnSetFromColumnArray();
}

// populate NAColumnArray with this ColumnSet
void ColStats::populateColumnArray
(const ColumnSet& cols, const NATable* table)
{
  if (table)
  {
    for (CollIndex x=cols.init(); cols.next(x); cols.advance(x))
    { 
      columns_.insert(table->getNAColumnArray().getColumnByPos(x));
      colPositions_ += x;
    }
  }
}

// populate ColumnSet with this NAColumnArray
void ColStats::populateColumnSetFromColumnArray()
{
  for (CollIndex x=0; x < columns_.entries(); x++)
  { 
    colPositions_ += columns_[x]->getPosition();
  }
}

ColStats::~ColStats()
{
  colPositions_.clear();
  frequentValues_.clear();
}
void ColStats::deepDelete()
{
  columns_.deepDelete();
  histogram_ = 0;
  colPositions_.clear();
  frequentValues_.clear();
}

void ColStats::deepDeleteFromHistogramCache()
{
  columns_.deepDelete();
  
  //histogram_ is a shared pointer
  //when deleting from cache we
  //just want to get rid of the histogram
  //We do this since the object pointed to
  //may not delete if shared pointer ref count
  //does not go down to zero. Not deleting
  //the histogram object can cause leaks
  Histogram * histPtr = histogram_.get();
  histogram_.reset();
  delete histPtr;

  colPositions_.clear();
  frequentValues_.clear();
}

HistogramSharedPtr
ColStats::getHistogramToModify()
{
  if (NOT isModified())
    {
      if (histogram_ != NULL)
        histogram_ = HistogramSharedPtr(new(heap_) Histogram (*histogram_, heap_));

      setModified (TRUE) ;
    }
  return histogram_;
}

// converts this histogram to fake. This could be because of some 
// problem in the histogram, like incorrect boundary vales.
// First condense the intervals into 1 interval, and then
// set the flags appropriately
void
ColStats::createFakeHist()
{
  EncodedValue lowBound = getMinValue();
  EncodedValue highBound = getMaxValue();

  EncodedValue dummyVal(0.0);

  if (lowBound > highBound)
  {
    // if minimum value specified by update stats is greater than the 
    // max value, set min value as the default min value for that
    // column type
    lowBound = dummyVal.minMaxValue(getStatColumns()[0]->getType(), TRUE);
  }

  CostScalar uec = MINOF(getRowcount(), getTotalUec() );

  setToSingleInterval ( lowBound,
			highBound,
			getRowcount(),
			uec ) ;

  // now we have to undo some of the automatic flag-setting
  // of ColStats::setToSingleInterval()
  setMinSetByPred (FALSE) ;
  setMaxSetByPred (FALSE) ;
  setShapeChanged (FALSE) ;
  setFakeHistogram (TRUE) ;

  setOrigFakeHist  (TRUE) ;
  // since fake histogram intervals are always single interval histograms
  // we will treat them as compressed
  setIsCompressed  (TRUE) ;
}

// --------------------------------------------------------------------
// ColStats::compressToSingleInt
// This method calls Histogram::condenseToSingleInterval, and also sets
// the isCompressed flag to TRUE
// --------------------------------------------------------------------
void
ColStats::compressToSingleInt()
{
  if (histogram_->numIntervals() > 1 )
  {
    CostScalar rowcount = getRowcount();
    CostScalar uec = getTotalUec();

    if(baseRowCount_ == rowcount)
    {
      CostScalar nullrc = getNullCount();
      CostScalar nulluec = ((nullrc > 0) ? csOne : csZero);

      rowcount -= nullrc;
      uec -= nulluec;
    }

    removeNullInterval();
    computeMaxFreqOfCol(TRUE);
    histogram_->condenseToSingleInterval();
    setRowsAndUec (rowcount, uec);
  }
  else
    computeMaxFreqOfCol(TRUE);

  this->setIsCompressed(TRUE);
}

// -----------------------------------------------------------------------
// After we've mangled the heck out of the histogram, we've often lost
// track of what the total rowcount/uec are.
// -----------------------------------------------------------------------
void
ColStats::setRowsAndUecFromHistogram()
{
  CostScalar newRowcount = 0 ;
  CostScalar newTotalUec = 0 ;

  for ( Interval iter = histogram_->getFirstInterval() ;
		iter.isValid() ;
		iter.next() ) // break when we've processed the last Interval
  {
    CostScalar iRows = rowRedFactor_ * iter.getRowcount();
	newRowcount += iRows;

	CostScalar iUec = iter.getUec();

	newTotalUec += MINOF(iRows, iUec);

  }

  setRowsAndUec (newRowcount, newTotalUec) ;
}

// -----------------------------------------------------------------------
// After we've mangled the heck out of the histogram, we've often lost
// track of what the current min/max values are.
// -----------------------------------------------------------------------
void
ColStats::setMaxMinValuesFromHistogram()
{
  // CASE 0 : zero intervals
  if ( histogram_ == NULL || histogram_->numIntervals() == 0 )
    {
      minValue_ = maxValue_ = UNINIT_ENCODEDVALUE ;
    }
  // CASE 1 : one interval
  // NB : if FIRST is a NULL interval, it's handled just fine
  else if ( histogram_->numIntervals() == 1 )
    {
      Interval first = histogram_->getFirstInterval() ;
      minValue_ = first.loBound() ;
      maxValue_ = first.hiBound() ;
    }
  // CASE 2 : more than one interval
  // NB: we avoid the last NULL interval (if it exists)
  else
    {
      Interval first = histogram_->getFirstInterval() ;
      Interval last = histogram_->getLastNonNullInterval() ;
      minValue_ = first.loBound() ;
      maxValue_ = last.hiBound() ;
    }
}

void
ColStats::setStatColumn(NAColumn * column)
{
  // insert at the 0th position the columnId for which this
  // colStat has been created. Used for Insert operations

  // If the colStats has been prepared from a valid column, then the
  // column_ would already have an entry. In this case, replace the current
  // column_ with the new column. If the colStat is being prepared as result
  // of Union / Transpose column, then there would not be any valid column_
  // entry. In this case, create a column_ entry and insert the new column
  // in that

  if (columns_.entries() == 0)
    columns_.insertAt(0, column);
  else
    columns_[0] = column;
}

// a minor variation on a THIS = OTHER assignment operator
void
ColStats::overwrite( const ColStats &other )
{
  HistogramSharedPtr otherCopy(new (other.heap_)
                               Histogram (*(other.histogram_), other.heap_));
  FrequentValueList * otherFreqListCopy = new (STMTHEAP) 
                  FrequentValueList(other.getFrequentValues(), STMTHEAP);

  histogram_       = otherCopy;
  this->setFrequentValue(*otherFreqListCopy);

  setRedFactor    (other.rowRedFactor_) ;
  setUecRedFactor (other.uecRedFactor_) ;

    // Make sure we can make a copy of a UDF one where we defaulted the UEC 
    // to minusOne.
  setRowsAndUec   (other.rowcount_, other.totalUec_, other.totalUec_ == csMinusOne) ;

  baseUec_         = other.baseUec_;
  uecBeforePred_   = other.uecBeforePred_;
  setUnique        (other.isUnique()) ;
  setAlmostUnique  (other.isAlmostUnique());
  setModified      (FALSE) ;
  setShapeChanged  (other.isShapeChanged()) ;
  setObsoleteHistogram (other.isObsoleteHistogram()) ;
  setFakeHistogram (other.isFakeHistogram()) ;
  setOrigFakeHist  (other.isOrigFakeHist()) ;
  setSmallSampleHistogram (other.isSmallSampleHistogram());
  setIsCompressed  (other.isCompressed());
  setMinSetByPred  (other.isMinSetByPred()) ;
  setMaxSetByPred  (other.isMaxSetByPred()) ;
  setVirtualColForHist ( other.isVirtualColForHist() );
  setUpStatsNeeded (other.isUpStatsNeeded()) ;
  setIsARollingColumn (other.isARollingColumn());
  setMaxMinValuesFromHistogram() ;
  setIsColWithBndryConflict (other.isColWithBndryConflict());
  setSelectivitySetUsingHint (other.isSelectivitySetUsingHint());
  afterFetchIntReductionAttempted_ = other.afterFetchIntReductionAttempted_;
}  // overwrite

// -----------------------------------------------------------------------
// Histogram Manipulation Routines:
//
// ColStats::modifyStats
//   Synthesize the effect of
//    ITM_IS_NULL, ITM_IS_NOT_NULL, ITM_IS_UNKNOWN, ITM_IS_NOT_UNKNOWN,
//    ITM_EQUAL, ITM_NOT_EQUAL, ITM_LESS, ITM_LESS_EQ, ITM_GREATER, and
//    ITM_GREATER_EQ predicates.
//
// This routine presumes that the given predicate has been determined to
// be applicable to the THIS ColStats.
// -----------------------------------------------------------------------
void
ColStats::modifyStats (ItemExpr * pred, CostScalar *maxSelectivity)
{
  getHistogramToModify();  // get a writeable copy.....

  if ( histogram_ == NULL || histogram_->numIntervals() == 0 )
    {
      CCMPASSERT (histogram_ != NULL) ;
      // $$$ synthesize the effect on just the MIN and MAX values??
      // $$$ Weird special case: Can we have a non-NULL min/max if the
      // $$$ histogram is empty/missing??  
      // If there is no histogram_, create an empty histogram and return.
      insertZeroInterval();
      return;
    }

  // Begin Set-Up to perform the given Predicate........
  const ValueId predValueId = pred->getValueId();
  OperatorTypeEnum op       = pred->getOperatorType();

  // initialize the new total rowcount and uec
  CostScalar newRowcount  = 0;
  CostScalar newUec       = 0;
  CostScalar origRowcount = rowcount_;
  CostScalar origUec      = totalUec_;

  NABoolean negate = FALSE;

  // find the constant value (if any) in the predicate
  EncodedValue lowBound (UNINIT_ENCODEDVALUE) ;
  EncodedValue highBound = lowBound ;

  ItemExpr * rhs = NULL;
  ConstValue * constant = NULL;

  if (pred->getArity() > 1)
    {
      rhs = pred->child(1);
      constant = rhs->castToConstValue(negate);

      const NAType* colType = getStatColumns()[0]->getType();
      if ((colType->getTypeQualifier() == NA_CHARACTER_TYPE) && 
         ((CharType*)colType)->isCaseinsensitive() && constant &&
         (((CharType*)colType)->getCharSet() != CharInfo::UNICODE))
	constant = constant->toUpper(HISTHEAP);


     // Fix to ALM#4991
     if(constant == NULL) {
       if (rhs->getOperatorType() == ITM_VEG_REFERENCE) {

         const VEG * veg = ((VEGReference *)rhs)->getVEG();
         ValueId constId = veg->getAConstant();

         if(constId != NULL_VALUE_ID)
           constant = constId.getItemExpr()->castToConstValue( negate );

      } else {

         if ((op == ITM_EQUAL) &&
             (rhs->getOperatorType() == ITM_CACHE_PARAM) )
         {

           ItemExpr * constantExpr = ((ConstantParameter *)rhs)->getConstVal();

           if (constantExpr != NULL)
             constant =  constantExpr->castToConstValue(negate);        
         }// cache_param
      } // not aveg_reference
     }

      // COLUMN <op> constant predicate?
      // if so, does column match the leading prefix of histogram?
      if (constant != NULL)
	{
	  // get the encoded format for the constant
	  lowBound = EncodedValue (constant, negate);
          highBound = lowBound ;
	}
    }

  switch (op)
    {
    case ITM_IS_NULL:
    case ITM_IS_UNKNOWN:
      isNull (FALSE);
      break;

    case ITM_IS_NOT_NULL:
    case ITM_IS_NOT_UNKNOWN:
      isNull (TRUE);
      break;

    case ITM_EQUAL:
      setToSingleValue (lowBound, constant);
      break;
    case ITM_NOT_EQUAL:
      removeSingleValue (lowBound, constant);
      break;
    case ITM_LESS:
      newUpperBound (lowBound, constant, FALSE);
      break;
    case ITM_LESS_EQ:
      newUpperBound (highBound, constant, TRUE);
      break;
    case ITM_GREATER:
      newLowerBound (highBound, constant, FALSE);
      break;
    case ITM_GREATER_EQ:
      newLowerBound (lowBound, constant, TRUE);
      break;
    default:
      return;
    }

  newRowcount = getRowcount();
  newUec      = getTotalUec();

#ifndef NDEBUG
  // $$$ I'm pretty sure the code below is already
  // $$$ taken care of in the routines above
  // $$$ --> the assertion is just a test of this

  // Determine whether or not the prior predicate did anything.
  // It is important that ColStats are only marked as SHAPE-
  // CHANGED when they actually have changed.
  if ( origRowcount != newRowcount || origUec != newUec )
    {
      CCMPASSERT (isShapeChanged() == TRUE) ;
      setShapeChanged(TRUE);
      // pretty sure the new hi/lo values are set correctly
    }
#endif
  
  // for max cardinality estimates, the selectivity of each applied 
  // predicate is important. It is needed in computing maxSelectivity.
  // Do this only for cases where maxselectivity(p) == selectivity(p).
  if (maxSelectivity && pred->maxSelectivitySameAsSelectivity())
    {
      *maxSelectivity = MINOF(newRowcount / origRowcount, *maxSelectivity);
    }

  return;
}  // modifyStats

// -----------------------------------------------------------------------
// simplestPreds
//   Used only for a Special Case:   column_a <op> column_a
// -----------------------------------------------------------------------

void
ColStats::simplestPreds (ItemExpr * pred)
{
  // Begin Set-Up to perform the given Predicate........
  const ValueId predValueId = pred->getValueId();
  OperatorTypeEnum op       = pred->getOperatorType();

  // doable, simple, special, case: column_a <op> column_a
  switch (op)
    {
    case ITM_NOT_EQUAL:
    case ITM_LESS:
    case ITM_GREATER:
      getHistogramToModify();
      if ( histogram_ != NULL )
        {
          clearHistogram() ; // predicate eliminates all rows
          return ;
        }
      else
        {
          CCMPASSERT (FALSE) ; // why would the histogram ever be NULL?
          histogram_ = new (HISTHEAP) Histogram (HISTHEAP);
        }
      break;

    case ITM_EQUAL:
    case ITM_LESS_EQ:
    case ITM_GREATER_EQ: // these predicates are all no-ops
    default:             // treat any other predicate as a no-op.
      break;
    }
}

// ---------------------------------------------------------------------
// ColStats::populateTemplateOfFakeHist
// This method populates the template created for fake histogram, by 
// setting the MIN and the MAX value of the fake histogram equal to the
// MIN and the MAX value of the real histogram to which it is being joined.
// Fake histograms are all single interval histograms. Along with the MIN 
// and the MAX values, the method also sets the low boundary and the 
// upper boundary of the single interval of the fake histogram equal to the 
// new MIN and the MAX values. Row count and the UEC of the fake histogram
// are not changed.
// ----------------------------------------------------------------------

void ColStats::populateTemplateOfFakeHist(const ColStatsSharedPtr& fakeHistogram,
                                          const ColStatsSharedPtr& realHistogram)
{
  HistogramSharedPtr thisHist = this->getHistogram() ;

  // if there are no histogram intervals, nothing to do. the aggregate
  // values are set outside in the calling method

  if (thisHist->numIntervals() == 0)
    return;

  EncodedValue newLoBound = realHistogram->getMinValue();
  EncodedValue newUpBound = realHistogram->getMaxValue();
  CostScalar numRows = fakeHistogram->getRowcount();
  CostScalar numUecs = fakeHistogram->getTotalUec();

  Interval thisInterval = thisHist->getFirstInterval() ;

  thisInterval.setRowsAndUec( numRows, numUecs );

  // Since this and other are fake histograms, 
  // they should have only one interval

  thisInterval.setLoBound (newLoBound) ;
  thisInterval.setHiBound (newUpBound) ; 

  // set the aggregate values
  setRedFactor    (1.0) ;
  setUecRedFactor (1.0) ;

  minValue_ = newLoBound ;
  maxValue_ = newUpBound ;
  setRowsAndUecFromHistogram() ;
}

// -----------------------------------------------------------------------
//  mergeColStats
//   Perform a merge operation between the two histograms in the supplied
//     column statistics of 'this'.
//   Retain all interesting interval boundaries.
//   For an inner join (mergeMethod == InnerJoin, or == OuterJoin), use
//     the equations for inner equi-join.
//   For a semi-join (mergeMethod == SemiJoin) use the equations for a
//     equality semi-join.
//   For a 'union' (mergeMethod == Union) use the maxs of the UECs, and
//     sum of the RowCounts
//   For an 'OR' (mergeMethod == Or) use the maxs of the UECs, and of
//     the RowCounts
// -----------------------------------------------------------------------
void
ColStats::mergeColStats (const ColStatsSharedPtr& otherStats,
                         MergeType mergeMethod,
                         NABoolean isNumeric,
                         OperatorTypeEnum exprOpCode,
                         NABoolean mergeFVs)
{
  // look for the special case where histogram info is missing
  if ( histogram_ == NULL ||
       histogram_->entries() == 0 ||
       otherStats->getHistogram() == NULL ||
       otherStats->getHistogram()->entries() == 0 )
    {
      recoverFromMergeColStats(otherStats, isNumeric, mergeMethod);
      return;
    }

  // merge SVI for histograms and set max frequency
  // merge any single valued intervals with the next interval before 
  // doing a join. This is because of the way we distribute rows and uec
  // in the intervals.

  CostScalar maxFreql = csMinusOne;
  CostScalar maxFreqr = csMinusOne;

  if ( !this->isFakeHistogram())
  {
    maxFreql = histogram_->mergeSVIWithNextAndSetMaxFreq();
    maxFreql = MIN_ONE_CS(maxFreql/scaleFactor_);
  }

  // We will make a deep copy of the right Table, as we might
  // need to merge any single valued interval
  ColStatsSharedPtr otherStatsCopy = ColStats::deepCopy(*(otherStats),HISTHEAP);

  NABoolean useCompressedHistogramsForMerge = FALSE;
  if((exprOpCode != REL_JOIN) && (this->isCompressed() || otherStatsCopy->isCompressed()))
    useCompressedHistogramsForMerge = TRUE;

  if ( !otherStats->isFakeHistogram())
  {
    maxFreqr = otherStatsCopy->getHistogramToModify()->mergeSVIWithNextAndSetMaxFreq();


    // set the max frequency of the left child, only if it is
    // not a semi-join.
    if ( ( mergeMethod != SEMI_JOIN_MERGE) &&
	 (mergeMethod != ANTI_SEMI_JOIN_MERGE) )
	    maxFreqr = (maxFreqr/otherStatsCopy->getScaleFactor()).minCsOne();
  }

  if (CmpCommon::getDefault(COMP_BOOL_42) == DF_OFF)
  {
    // set the max frequencies for the two children as these will be used 
    setMaxFreq(maxFreql);
    otherStatsCopy->setMaxFreq(maxFreqr);
  }

  NABoolean maxSetByPredFlag = FALSE;
  NABoolean minSetByPredFlag = FALSE;

  // set maxSetByPreds and minSetByPreds flags based on the max and the
  // min values of the merging histograms
  this->setMaxAndMinSetByPredFlags(otherStatsCopy,
                                   maxSetByPredFlag,
				   minSetByPredFlag);

  // merge frequent values of the two histograms. Scaling needs to be done only for joins when
  // a cross product is performed between left and the right histograms 
  NABoolean scaleFreq = TRUE;
  if ( ((exprOpCode != REL_JOIN) && !useCompressedHistogramsForMerge) || 
       (( mergeMethod == SEMI_JOIN_MERGE) ||
       (mergeMethod == ANTI_SEMI_JOIN_MERGE) ) )
       scaleFreq = FALSE;

  NABoolean isResultOrigAFakeHistogram =
    this->isOrigFakeHist() && otherStatsCopy->isOrigFakeHist() ;

  // should we include skewed value while estimating join cardinality row count? if yes,
  // set adjRowCount to TRUE
  NABoolean adjRowCount = FALSE;
  NABoolean isRCAdjusted = FALSE;
  if ( (CmpCommon::getDefault(HIST_INCLUDE_SKEW_FOR_NON_INNER_JOIN) == DF_ON) &&
       !isResultOrigAFakeHistogram &&
       (exprOpCode == REL_JOIN)    &&
       (mergeMethod == INNER_JOIN_MERGE) )
    adjRowCount = TRUE;

  if (mergeFVs ||
      CmpCommon::getDefault(HIST_MERGE_FREQ_VALS_FIX) == DF_OFF)
    isRCAdjusted = this->mergeFrequentValues(otherStatsCopy, scaleFreq, mergeMethod, adjRowCount);

  CostScalar newRowCount = 0;
  CostScalar newUec = 0;
  CostScalar maxUecSum = 0;

  QueryAnalysis *qa = QueryAnalysis::Instance();

  if ( (CmpCommon::getDefault(COMP_BOOL_42) == DF_ON)  &&
       (qa && qa->isCompressedHistsViable()) &&
       ((exprOpCode == REL_JOIN) || useCompressedHistogramsForMerge) &&
       (mergeMethod == INNER_JOIN_MERGE) ) 
  {
    // compute join cardinality using frequent values
    maxUecSum = this->mergeCompressedHistograms(otherStatsCopy, 
                                                newRowCount, newUec,
                                                mergeMethod);
  }
  else
  {
     // do the actual join by merging histogram intervals
     maxUecSum = this->mergeWithExpandedHistograms(otherStatsCopy, isNumeric, 
                                                  newRowCount, newUec,
                                                  mergeMethod);
    if ( adjRowCount && isRCAdjusted &&
        this->getFrequentValues().entries() > 0 )
      newRowCount += this->getFrequentValues().getMaxFrequency();
  }

  HistogramSharedPtr targetHistogram = getHistogram();

  // if it is a join related merge, do the selectivity adjustments for
  // indirect reductions
  if (isAJoinRelatedMerge(mergeMethod))
  {
    // $$$ should this flag be set in more cases?
    setRecentJoin (TRUE) ; // result histogram is the result of a recent join

    // Make adjustments to the resulting UEC and rowcount if the UECs were
    // reduced due to independent predicates (preds not on this column)

      CostScalar selAdj = this->adjustSelectivity(otherStatsCopy, newUec, mergeMethod);

      if (mergeMethod == ANTI_SEMI_JOIN_MERGE)
        selAdj = csOne;

      newRowCount *= selAdj;
      newUec *= selAdj;


    // Apply the adjustments to the new histogram

    // $$$ mar: after this step, should merge this histogram's intervals
    //          which have >0,<1 row or uec
    if (selAdj.isLessThanOne())
    {
      CollIndex i = 1;

      while (i < targetHistogram->entries()) 
      {
	    CostScalar tempUec    = selAdj * (*targetHistogram)[i].getUec();
	    CostScalar tempRows   = selAdj * (*targetHistogram)[i].getCardinality();
	    (*targetHistogram)[i].setCardAndUec (tempRows, tempUec);
	    i++;
      }
      // remove any histogram intervals with zero UEC
      // if selAdj is 0, they're all zero right now
      removeRedundantEmpties() ; 
    }
  } 
  // $$$ ****************************************************************
  // need to decide how to propagate the various
  // flags past this function
  //
  // shapeChanged_
  // maxSetByPred_
  // minSetByPred_
  // isFakeHistogram_
  // isOrigFakeHist_
  //
  // 1. shapeChanged_ :
  // . for OR_MERGE, this flag is TRUE if one side or the other is TRUE
  // . for all others, set this flag TRUE in all cases
  // 2. maxSetByPred_ :
  //    minSetByPred_ :
  // . for UNION_MERGE, TRUE only if TRUE for both sides
  // . for OR_MERGE, TRUE only if TRUE for both sides
  // . for AND_MERGE, TRUE only if TRUE for both sides
  // . for INNER_JOIN_MERGE,
  // . for OUTER_JOIN_MERGE
  // . for SEMI_JOIN_MERGE && ANTI_SEMI_JOIN_MERGE
  // . for LEFT_JOIN_OR_MERGE
  // 3. isFakeHistogram_
  // . for all of them, this flag is TRUE if one side or other is TRUE
  // 4. isOrigFakeHist_
  //  . for all of them, this flag is TRUE only if both sides are TRUE
  // shapeChanged_

  if ( mergeMethod == OR_MERGE)
    {
      setShapeChanged (isShapeChanged() || otherStatsCopy->isShapeChanged()) ;
      baseUec_ = newUec;
    }
  else
  {
    // Sol: 10-090414-0801. Set teh baseUec_ for anti-semi-join as teh baseUec_ of the left side
    setShapeChanged (TRUE) ;
    if ( mergeMethod == ANTI_SEMI_JOIN_MERGE )
      baseUec_ = baseUec_ ; // $$$ not right, but I don't know what's the right thing to do
    else
    {
      baseUec_ = MINOF (baseUec_, otherStatsCopy->baseUec_);
      uecBeforePred_ = MINOF (uecBeforePred_, otherStatsCopy->uecBeforePred_);
    }
  }

  Interval last = targetHistogram->getLastNonNullInterval() ;
  if ( !last.isValid() )
  {
    // this means that the target merge template is empty or a
    // single-NULL-interval histogram; in either case, we don't
    // really care about the max/min-set-by-pred flags!
    minSetByPredFlag =  maxSetByPredFlag = FALSE;
    if ((newRowCount == 0) && (targetHistogram->entries() == 0))
    {
      insertZeroInterval();
      Interval first = histogram_->getFirstInterval();
      first.setRowsAndUec(newRowCount, newUec);
    }
  }

  // minSetByPred_, maxSetByPred_
  setMinSetByPred (minSetByPredFlag) ;
  setMaxSetByPred (maxSetByPredFlag) ;

  // is the result of this merge going to be fake?  tentatively, only if
  // both the inputs are fake
  NABoolean isResultAFakeHistogram =
    (this->isFakeHistogram() && otherStatsCopy->isFakeHistogram()) ||
    isResultOrigAFakeHistogram;

  // isFakeHistogram_
  setFakeHistogram (isResultAFakeHistogram) ;
  setOrigFakeHist (isResultOrigAFakeHistogram) ;
  setUpStatsNeeded (isUpStatsNeeded() || otherStatsCopy->isUpStatsNeeded()) ;
  setVirtualColForHist (isVirtualColForHist() || otherStatsCopy->isVirtualColForHist() );

  setRedFactor    (1.0) ;
  setUecRedFactor (1.0) ;
  setRowsAndUec   (newRowCount, newUec) ;
  setSumOfMaxUec  (MAXOF(sumOfMaxUec_, MAXOF(otherStatsCopy->sumOfMaxUec_,
                        MAXOF (maxUecSum, MAXOF (baseUec_, otherStatsCopy->baseUec_))))) ;

  scaleFactor_ = csOne;
  if (CmpCommon::getDefault(COMP_BOOL_42) == DF_ON)
  {
    // resultant frequency is the max of frequencies of resultant and right histogram
    this->computeMaxFreqOfCol(TRUE);
  }

  setMaxFreq(MAXOF(getMaxFreq(), maxFreqr) );

  // setUnique (FALSE) ; // this flag was set before this method was called
  setModified (TRUE) ;

  reduceToMaxIntervalCount() ; // remove HistInts if necessary ...

  reduceIntermediateHistInts(mergeMethod, isNumeric);
}  // mergeColStats

// ------------------------------------------------------------
// minSetByPred_ , maxSetByPred_ flags indicate if the boundaries
// for the histograms were set by application of predicates. The values
// for these flags for the target merged histogram are calculated below
// ------------------------------------------------------------

void
ColStats::setMaxAndMinSetByPredFlags(const ColStatsSharedPtr & otherStatsCopy,
				     NABoolean &maxSetByPredFlag,
				     NABoolean &minSetByPredFlag)
{
  
  maxSetByPredFlag = minSetByPredFlag = FALSE;
  // The values of these flags depend on if the target histogram max 
  // or min value were picked from the left or the right child, and 
  // if these max and min values were a result of some predicate

  EncodedValue leftMax, leftMin, rightMax, rightMin ;
  Interval last, first ;

  last = otherStatsCopy->getHistogram()->getLastNonNullInterval() ;
  if ( !last.isValid() )
    rightMax = rightMin = NULL_ENCODEDVALUE ;
  else
  {
    rightMax = last.hiBound() ;
    first = otherStatsCopy->getHistogram()->getFirstInterval() ;
    rightMin = first.loBound() ;
  }

  last = histogram_->getLastNonNullInterval() ;
  if ( !last.isValid() )
    return ;
  
  leftMax = last.hiBound() ;
  first = histogram_->getFirstInterval() ;
  leftMin = first.loBound() ;

  if ( last.hiBound() == leftMax && last.hiBound() == rightMax )
    maxSetByPredFlag = this->isMaxSetByPred() && otherStatsCopy->isMaxSetByPred() ;
  else if ( last.hiBound() == leftMax )
    maxSetByPredFlag = this->isMaxSetByPred() ;
  else if ( last.hiBound() == rightMax )
    maxSetByPredFlag = otherStatsCopy->isMaxSetByPred() ;
  else
    maxSetByPredFlag = FALSE ;

  first = histogram_->getFirstInterval() ;
  if ( first.loBound() == leftMin && first.loBound() == rightMin )
    minSetByPredFlag = this->isMinSetByPred() && otherStatsCopy->isMinSetByPred() ;
  else if ( first.loBound() == leftMin )
    minSetByPredFlag = this->isMinSetByPred() ;
  else if ( first.loBound() == rightMin )
    minSetByPredFlag = otherStatsCopy->isMinSetByPred() ;
  else
    minSetByPredFlag = FALSE ;
} // setMaxAndMinSetByPredFlags

// ---------------------------------------------------------------------
// graceful recovery in case of any error while merging two histograms, 
// ---------------------------------------------------------------------
void ColStats::recoverFromMergeColStats(const ColStatsSharedPtr& otherStats,
                         NABoolean isNumeric,
                         MergeType mergeMethod)
{
   if (histogram_ == NULL)
   {
    CCMPASSERT (histogram_ != NULL) ;
    insertZeroInterval();
   }

   if (otherStats->histogram_ == NULL)
   {
     CCMPASSERT (otherStats->getHistogram() != NULL );
     otherStats->insertZeroInterval();
   }
  // Can't always construct a precise result histogram, but when one
  //   can't one sometimes *can* produce a meaningful single-interval
  //   result.
  //   E.g., for a Union use the sum of the RowCounts, the MAX of the
  //       UECs, and the widest possible value range.
  mergeWithEmptyHistogram (otherStats, mergeMethod);
  reduceIntermediateHistInts(mergeMethod, isNumeric);
} // recoverFromMergeColStats

// ---------------------------------------------------------------------
// The join cardinality can be computed either by merging histogram
// intervals or merging frequent value lists. In this method we compute
// join cardinality using histogram intervals
// ----------------------------------------------------------------------
CostScalar
ColStats::mergeWithExpandedHistograms (const ColStatsSharedPtr& otherStats,
                                       NABoolean isNumeric,
				       CostScalar & newRowcount,
				       CostScalar & newUec,
                                       MergeType mergeMethod)
{
  // ------------------------------------------------------------------
  // CREATE A MERGE TEMPLATE for the result of the merge operation
  // ------------------------------------------------------------------

  //  ( left = this; right = other )
  const NABoolean createTemplateWithEquimerge =
    ( mergeMethod == UNION_MERGE          ||
      mergeMethod == OR_MERGE             ||
      mergeMethod == LEFT_JOIN_OR_MERGE   ||
      mergeMethod == ANTI_SEMI_JOIN_MERGE ? FALSE : TRUE ) ;

  HistogramSharedPtr leftHistogram =
    histogram_->createMergeTemplate (otherStats->getHistogram(),
                                     createTemplateWithEquimerge) ;

  NABoolean isResultAFakeHistogram = FALSE;
  // ----------------------------------------------------------------
  // RECOVER FROM ZERO INTERVALS IN MERGE TEMPLATE
  // ----------------------------------------------------------------

  // Gotcha : we never want to produce a zero-interval template, because
  // this will result in a zero-row merge
  //
  // So we now need to check : are there zero intervals in the template?
  // if so, we probably want to change that so that we get a single
  // interval in the template (from MIN(minvalues) to MAX(maxvalues)) with
  // 1 row/uec
  //
  if ( leftHistogram->entries() == 0 )
  {
    // Throw an assertion in debug mode, but in release mode
    // create an empty histogram and continue with compilation
    if(!createTemplateWithEquimerge)
    {
      CCMPASSERT (createTemplateWithEquimerge) ; // if this isn't true, something is very wrong
      recoverFromMergeColStats(otherStats, isNumeric, mergeMethod);
      setFakeHistogram(TRUE);
      return getSumOfMaxUec();
    }

    isResultAFakeHistogram = handleMergeTemplateWithZeroIntervals(otherStats, leftHistogram);
  }

  // ---------------------------------------------------------------------
  // POPULATE TEMPLATE
  // ---------------------------------------------------------------------

  // copy that template for the use of the 2nd (right) source histogram
  HistogramSharedPtr rightHistogram(new (heap_) Histogram (*leftHistogram, heap_));

  // and, copy it again to create a target for the merge process
  HistogramSharedPtr targetHistogram(new (heap_) Histogram (*leftHistogram, heap_));
  
  isResultAFakeHistogram = this->populateLeftAndRightTemplates(otherStats, 
                                                               leftHistogram, 
                                                               rightHistogram,
                                                               targetHistogram);

  // --------------------------------------------------------------------
  // MERGE HISTOGRAM INTERVALS
  // --------------------------------------------------------------------
  
  CostScalar scaleRowCount = rowcount_ ;
  CollIndex i = 1; // skip first HistInt which has 0 rows/uec

  // Perform the 'merge' of the two now normalized histograms.  Place
  // results in targetHistogram.
  // In the following, be careful to try and retain the actual UEC's, but
  // don't do division by a UEC that is less than one.
  CostScalar maxUecSum = csZero;
  while (i < targetHistogram->entries())
    {
      maxUecSum += (*targetHistogram)[i].mergeInterval((*leftHistogram)[i],
                                                       (*rightHistogram)[i],
                                                        scaleRowCount,
                                                        mergeMethod);
      newRowcount += (*targetHistogram)[i].getCardinality();
      newUec += (*targetHistogram)[i].getUec();
      i++;
    }

  // update 'this' column statistics with the merged histogram, and other
  //   altered data.
  // check for a possibly empty histogram
  histogram_ = targetHistogram;
  // remove any redundant empty intervals from the result histogram
  removeRedundantEmpties() ; //NB: this may clear the histogram
  
  setFakeHistogram(isResultAFakeHistogram);

  return maxUecSum;
} // mergeWithExpandedHistograms

// ---------------------------------------------------------------------
// The join cardinality can be computed either by merging histogram
// intervals or merging frequent value lists. In this method we compute
// join cardinality using frequent values
// ----------------------------------------------------------------------
CostScalar
ColStats::mergeCompressedHistograms (const ColStatsSharedPtr& otherStats,
                                    CostScalar &newRowcount,
                                    CostScalar &newUec,
                                    MergeType mergeMethod)
{
  CostScalar maxUec = MAXOF(getSumOfMaxUec(), otherStats->getSumOfMaxUec() );
  if(mergeMethod != INNER_JOIN_MERGE)
    return maxUec;

  if (!this->isCompressed())
    this->compressToSingleInt();
  if (!otherStats->isCompressed())
    otherStats->compressToSingleInt();

  // merge left and right histogram intervals based on the join type

  newRowcount = csZero;
  newUec = csZero;

  // now adjust newRowcount computed from interval by the frequent values from each histogram
  const FrequentValueList &leftFreqValList = getFrequentValues();

  // Get the UECs for continuum after having removed the stolen values
  double adjUC1 = getAdjContinuumUEC().getValue();
  double adjUC2 = otherStats->getAdjContinuumUEC().getValue();

  // get the frequency of the continuum after having removed the stolen
  // frequencies. 
  double adjRC1 = getAdjContinuumFreq().getValue();
  double adjRC2 = otherStats->getAdjContinuumFreq().getValue();

  // Final Rowcounts and UECs for continuums
  double joinUECForContinuum = MINOF (adjUC1, adjUC2);
  double joinRCForContinuum = 0;
  
  double maxAdjUC = MAXOF(adjUC1, adjUC2);

  if (maxAdjUC > 0)
    joinRCForContinuum = (adjRC1 * adjRC2)/maxAdjUC;

  // Final join cardinality will be the sum of frequent values and the rowcount
  // of the continuum values

  double RF1 = leftFreqValList.getTotalFrequency().getValue(); 
  newRowcount = joinRCForContinuum + RF1;
  newUec = joinUECForContinuum + leftFreqValList.getTotalProbability().getValue();
  newUec = MINOF(newRowcount, newUec);

  // set the total rowcount and the UEC in the histogram interval
  HistogramSharedPtr targetHistogram(new (heap_) Histogram (*histogram_, heap_));
  // Boundaries of the resultant histogram are inherited from the left histogram
  // which are set to max and mins of the data type
  // set the rowcount and the UEC equal to the newly computed rowcount and UEC
  // since the histogram has been compressed, there will be only interval
  Interval iter = targetHistogram->getFirstInterval() ;
  if (iter.isValid() )
    iter.setRowsAndUec (newRowcount, newUec);

  histogram_ = targetHistogram;
  return maxUec;
} // mergeCompressedHistograms

// --------------------------------------------------------------------
// adjust selectivity computed by either merging histogram intervals
// or frequent value lists to take into account any indirect reductions
// ---------------------------------------------------------------------
CostScalar
ColStats::adjustSelectivity(const ColStatsSharedPtr& otherStats,
			    const CostScalar & newUec,
                            MergeType mergeMethod)
{
    // Make adjustments to the resulting UEC and rowcount if the UECs were
    // reduced due to independent predicates (preds not on this column)
    //
    // Use the baseUec_ to determine the amount of original matching and
    // the newUec to determine the amount of overlap

    // New approach to selectivity adjustment and is defined as follows:
    // Selectivity adjustment is defined as the ratio of the super set UEC based on correlated 
    // assumption to the super set UEC based on active assumption. Correlated UEC is the UEC 
    // obtained after applying the reductions from local predicates. Independent UEC is the base UEC 
    // without any reductions. In the new approach, the selectivity adjustments take data distribution
    // into consideration. If independent assumption is OFF, no selectivity adjustment is made.
    // Otherwise, the following formulae will be used.
    //
    // Selectivity Adjustment (SA) =  SuperSet UEC based on correlated assumption / SuperSet UEC based on underlying active assumption;

    CostScalar selAdj = csOne;
    if (CURRSTMT_OPTDEFAULTS->histAssumeIndependentReduction())
    {
      // SSU - Superset UEC based on underlying data distribution assumption
      CostScalar SSU = csOne;

      if(CURRSTMT_OPTDEFAULTS->histOptimisticCardOpt() == 1)
      {
	CostScalar totalUecOfLargerBaseUec = baseUec_ >= otherStats->baseUec_ ? totalUec_ : otherStats->totalUec_ ;
	SSU = MAXOF(MINOF(baseUec_ , otherStats->baseUec_), totalUecOfLargerBaseUec) ;
      }
      else
	SSU = MAXOF(baseUec_ , otherStats->baseUec_);

      // Selectivity Adjustment = SSU on correlated assumption / SSU based on underlying active assumption
      selAdj = (MAXOF(totalUec_ , otherStats->totalUec_) / SSU).maxCsOne();
    }

    CCMPASSERT (NOT selAdj.isGreaterThanOne() /*selAdj <= 1*/) ;

    selAdj = selAdj.maxCsOne();
    return selAdj;
} // adjustSelectivity

// ------------------------------------------------------------------------
// populate left and right histogram templates created for merge. The
// histograms will be populated based on if the stats exist for both 
// children or not
// ------------------------------------------------------------------------
NABoolean
ColStats::populateLeftAndRightTemplates(const ColStatsSharedPtr & otherStatsCopy,
					HistogramSharedPtr & leftHistogram, 
					HistogramSharedPtr & rightHistogram,
					HistogramSharedPtr & targetHistogram)
{
  ColStats leftStats(leftHistogram, HISTHEAP);
  ColStats rightStats(rightHistogram, HISTHEAP);

  // Create a shared pointer to "this" with proper reference count.
  ColStatsSharedPtr thisSharedPtr = ColStatsSharedPtr::getIntrusiveSharedPtr(this);

  // ----------------------------------------------------------------
  // When we join an actual histogram with the fake histogram, the 
  // cardinality goes down to 1. This is because the MIN and the MAX
  // of the fake histogram range from -infinity to +infinity. And when
  // the interval boundaries of this fake histograms are matched to the
  // actual histogram being joined, the row and the uec reduction is huge
  // which leads to very low cardinality.
  // We do the fix by setting the MIN and the MAX of the fake interval
  // equal to the MIN and the MAX of the histogram being joined.
  // ------------------------------------------------------------------

  NABoolean thisOriginallyFake = this->isOrigFakeHist();
  NABoolean otherOriginallyFake = otherStatsCopy->isOrigFakeHist();

  NABoolean isResultAFakeHistogram = thisOriginallyFake && otherOriginallyFake;


  if (thisOriginallyFake && !otherOriginallyFake)
  {
    leftStats.populateTemplateOfFakeHist(thisSharedPtr, otherStatsCopy);
    rightStats.populateTemplate (otherStatsCopy) ;
  }
  else
    if (otherOriginallyFake && !thisOriginallyFake )
    {
      rightStats.populateTemplateOfFakeHist(otherStatsCopy, thisSharedPtr);
      leftStats.populateTemplate (thisSharedPtr) ;
    }
    else
    {
      // Update the UEC and RowCounts of the left and right templates with the
      // actual histogram's data adjusted to the templates' interval boundaries.
      // The results are properly scaled by their reduction factors.....
      leftStats.populateTemplate (thisSharedPtr) ;
      rightStats.populateTemplate (otherStatsCopy) ;
    }

  // ----------------------------------------------------------------
  // *****************************************************
  // RECOVER FROM COLLAPSED INTERVALS IN POPULATE-TEMPLATE
  // *****************************************************
  // Gotcha:
  // After populateTemplate has done its thing, it checks to make sure
  // that a certain minimum number of rows from the populat-ING template
  // (this, otherStats) ended up in the populat-ED template (leftStats,
  // rightStats).  If this wasn't the case, then that template was squished
  // down to one interval (spanning the max/min values) and that given
  // minimum number of rows (plus an appropriate number of uecs) was
  // placed in that single interval.
  //
  // If this happened for one, then update the other and targetHistogram,
  // too
  // ----------------------------------------------------------------

  if ( leftHistogram->entries() !=  rightHistogram->entries() OR
       leftHistogram->entries() != targetHistogram->entries() )
    {
      leftHistogram->condenseToSingleInterval() ; // one of these
      rightHistogram->condenseToSingleInterval() ; // is redundant
      targetHistogram->condenseToSingleInterval() ;
      this->setIsCompressed(TRUE);
      isResultAFakeHistogram = TRUE ; // $$$ the result of this merge is now fake
    }
  return isResultAFakeHistogram;
} // populateLeftAndRightTemplates

//This method returns the reduction criterion to apply
//when merging the hist ints of a histogram (for the
//purpose of reducing the number of histogram's intervals).
//The method factors in the location from where the reduction
//is invoked (parameter invokedFrom), the desired reductionCriterion
//to apply (parameter reductionCriterion) and if the histogram caching
//should be considered or ignored.
Criterion ColStats::decideReductionCriterion(Source invokedFrom,
                                             Criterion reductionCriterion,
                                             const NAColumn * column,
                                             NABoolean ignoreHistogramCachingFlag)
{
	//cannot reduce multicolumn stats
	if(getStatColumns().entries() > 1)
          return NONE;

	//if invoked histograms for base tables
	//have been obtained using FetchHistograms
	if(invokedFrom == AFTER_FETCH)
	{
		//check if histogram caching is on
		if(CURRSTMT_OPTDEFAULTS->cacheHistograms()&&
		   (!ignoreHistogramCachingFlag))
		{
			//if datatype of the column is numeric
			if(column->isNumeric())
			{
				return reductionCriterion;
			}
			//datatype of column is non-numeric
			else
			{
				//cannot apply criterion 1 to non-numeric
				//columns
				if(reductionCriterion == CRITERION1)
				{
					return NONE;
				}
				else
				{
					return reductionCriterion;
				}
			}
		}
		//histogram caching is off
		//or we want to ignore the fact that
		//histogram caching is on / off
		else
		{
			//if datatype of column is numeric
			if(column->isNumeric())
			{
				//if column has range or join pred
				if(column->hasRangePred()||column->hasJoinPred())
				{
					return CRITERION1;
				}
				//column does not have range or join pred
				else
				{
					return CRITERION2;
				}
			}
			//datatype of column is non-numeric
			else{
				//if column has range or join pred
				//we can only use criterion1,
				//but criterion 1 can only be applied
				//to numeric columns
				if(column->hasRangePred()||column->hasJoinPred())
				{
					return NONE;
				}
				//there is no range of join pred
				else
				{
					return CRITERION2;
				}
			}
		}
	}
	//if invoked after a new histogram has been generated
	//as a result of a relational operator like join.
	else
	{
		    //if column is numeric
			if(column->isNumeric())
			{
				return CRITERION1;
			}
			//column is non-numeric
			else
			{
				return CRITERION2;
			}

	}
	return NONE;
};

//reduce the number of histogram intervals in the histogram
//referenced by this ColStats Object
void ColStats::reduceNumHistInts(Source invokedFrom, Criterion reductionCriterion)
{
	//if there is no histogram return
	if(!histogram_)
		return; 

	//dont do anything for fake histograms
	if(isFakeHistogram())
		return;

	//multicolumn stats, dont reduce
	if(columns_.entries() > 1)
		return;

	//if there are only two histints or less
	//we dont need to reduce
	if(histogram_->entries() <= 2)
		return;

	//Column whoes histogram is referred to
	//by this ColStats object
	const NAColumn * column = getStatColumns()[0];

	//reduce the number of histogram intervals
	histogram_->reduceNumHistInts(decideReductionCriterion(invokedFrom, reductionCriterion, column),
	                              invokedFrom);
}

// -----------------------------------------------------------------------
// This is a helper method for reducing intermediate histograms
// -----------------------------------------------------------------------
void ColStats::reduceIntermediateHistInts(MergeType mergeMethod, NABoolean isNumeric)
{
  if(CURRSTMT_OPTDEFAULTS->reduceIntermediateHistograms())
  {
    if(isAJoinRelatedMerge(mergeMethod) ||
     (mergeMethod == LEFT_JOIN_OR_MERGE))
    {
      Criterion criterion;
      if(isNumeric)
	criterion = CRITERION1;
      else
	criterion = CRITERION2;
      histogram_->reduceNumHistInts(criterion,INTERMEDIATE);
    }
  }
}

// -----------------------------------------------------------------------
// countFailedProbes
//
// This routine is used by physical costing to determine the number of
// key predicate 'probes' performed during a Nested Join which did not
// produce any result rows.
// THIS provides the ColStats of the appropriate columns in the Input
// EstLogProp;  otherStats provides the result of the key predicate join
// done with the base table.   An INNER Join is assumed.
// -----------------------------------------------------------------------
CostScalar
ColStats::countFailedProbes (const ColStatsSharedPtr& otherStats) const
{
  // look for the special case of missing/empty join Result.
  if ( otherStats->getHistogram() == NULL         OR
       otherStats->getHistogram()->entries() == 0 OR
       otherStats->getRowcount().isZero() )
    {
      return getRowcount();  // all probes failed.
    }

  // first create a template;  ( left = this; right = other )
  HistogramSharedPtr leftHistogram =
    histogram_->createMergeTemplate (otherStats->getHistogram(), FALSE);

  // copy that template for the use of the 2nd (right) source histogram
  HistogramSharedPtr rightHistogram(new (heap_) Histogram (*leftHistogram, heap_));

  // Create a shared pointer to "this" with proper reference count.
  ColStatsSharedPtr thisSharedPtr = ColStatsSharedPtr::getIntrusiveSharedPtr(this);

  ColStats leftStats  (leftHistogram, HISTHEAP) ;
  ColStats rightStats (rightHistogram, HISTHEAP) ;
  // Update the UEC and RowCounts of the left and right templates with the
  // actual histogram's data adjusted to the templates' interval boundaries.
  leftStats.populateTemplate(thisSharedPtr) ;
  rightStats.populateTemplate(otherStats) ;

  // be careful! populateTemplate may have compressed the intervals if
  // the resulting rowcount was too low!
  if ( leftHistogram->entries() != rightHistogram->entries() )
    {
      leftHistogram->condenseToSingleInterval() ; // one of these
      rightHistogram->condenseToSingleInterval() ; // is redundant
      CCMPASSERT ( leftHistogram->entries() == rightHistogram->entries() ) ;
    }

  CostScalar
    totalFailedProbes= 0,
    failedProbesForInterval,
    leftUEC,
    leftRowCount,
    rightUEC,
    rightRowCount;
  CollIndex i = 1;

  // Perform the failed probe count on the two normalized histograms.
  while (i < leftHistogram->entries())
    {
      // left is Pre-Join
      leftUEC = (*leftHistogram)[i].getUec();
      leftRowCount = (*leftHistogram)[i].getCardinality();

      // right is Post-Join
      rightUEC = (*rightHistogram)[i].getUec();
      rightRowCount = (*rightHistogram)[i].getCardinality();

      DCMPASSERT(rightUEC.isGreaterOrEqualThanZero() AND leftUEC.isGreaterOrEqualThanZero());

      // The failed probe count varies on a case by case basis
      if (rightUEC.isLessThanOne() OR leftUEC.isLessThanOne())
        {
          // don't attempt to compute failed probes if uec's are less than one:
          failedProbesForInterval = 0.;
        }
      else if (rightUEC.isZero())
        {
          // if the right table has no rows, then all probes will fail
          failedProbesForInterval = leftRowCount;
        }
      else if (leftUEC < rightUEC)
        {
          // if the left table has fewer UEC than right, then no probes can fail.
          failedProbesForInterval = 0.;
        }
      else
        {
          // else count the number of the original's unmatched rows
          failedProbesForInterval = ((leftRowCount / leftUEC) * (leftUEC - rightUEC));
        }

      totalFailedProbes += failedProbesForInterval;
      i++;
    }

  return totalFailedProbes;
}


// -----------------------------------------------------------------------
// copyAndScaleHistogram
//
// in the given ColStats, replace the current histogram with a copy that
// has had all of its interval's rowcounts multiplied by the specified
// scale.
// At the same time, apply any current reduction factor to those same
// histogram buckets.
// -----------------------------------------------------------------------

void
ColStats::copyAndScaleHistogram (CostScalar scale)
{
  if ( getHistogram() == NULL )
    return ;

  histogram_ = HistogramSharedPtr(new (heap_) Histogram(*histogram_, heap_));

  if ( (!isOrigFakeHist()) )
  {
    this->setFrequentValue(getFrequentValues());
  }

  // now scale the histogram
  scaleHistogram (scale) ;
}

void
ColStats::scaleHistogram (CostScalar scale,
                          CostScalar uecScale,
                          NABoolean scaleFreqValList)
{
  if ( getHistogram() == NULL )
    return;

  // set the scale factor of the histogram with what ever the histogram
  // is being scaled by. The method is called for making deep copies. We 
  // don't want to loose the scale then. Hence update the scale factor
  // only when it is not equal to one. 
  if (scale != csOne)
    scaleFactor_ = scale;

  HistogramSharedPtr hist = histogram_ ; // convenience

  if (scale.isGreaterThanOne() /* > 1 */)
  {
    setUnique (FALSE) ; // any previously UNIQUE column is no longer, truly UNIQUE
  }

  CostScalar newRowcount = 0 ;
  CostScalar newUec = 0 ;
  CostScalar iRows ;
  CostScalar iUec ;

  // Update each histogram interval, as well as the aggregate statistics.

  //
  // iterate through the histogram and individually scale
  // all of the Intervals
  //
  Interval iter ;
  CostScalar iRowsRed = scale * rowRedFactor_;

  // If row reduction and UEC reduction factors are 1, there is nothing
  // to scale, so return.
  //*************************************************************************
  // IMP: When we skip the loop of applying reductions, in case all reduction 
  // factors are 1, and there is a deep copy being performed, I found 
  // we still got change in cardinalities. Ideally this should not happen, as
  // we are not modifying the histograms.
  // This happens, because we have an additional logic of 
  // isSingleValuedInterval() in this loop. For O_CLERK (ORDERS table), 
  // we originally have 100,000 UEC, when we do a deep copy, the UEC 
  // should still remain the same. But it gets changed to 1. This is because the 
  // MIN, MAX and the interval boundaries are converted to encoded values. 
  // Eventhough the loboundary and the high boundary of this interval are 
  // (''Clerk#000000055'') and (''Clerk#000000237'') respectively, 
  // the encoded values, because of their representation are the same. 
  // Hence the interval is treated as a single valued interval, and the 
  // UEC of the interval is set to 1. Since it is a single interval histogram, 
  // the total UEC is also changed from 100,000 to 1.
  // Because of the change in the code (skipping of the loop), this problem will
  // atleast not happen for deepcopies, but can still happen when a reduction
  // needs to be applied. Normally we should have only equality predicates
  // for such type of columns, which will anyway result in UEC equal to 1
  // -      Jan 6, 2005
  // ***************************************************************************

  if ( (scale == 1) &&
	   (uecScale == 1) &&
	  (rowRedFactor_ == 1) &&
	  (uecRedFactor_ == 1) )
  {
	return;
  }
  else
  {
        if (uecScale > csOne)
        {
          CCMPASSERT ("UEC can never increase");
          uecScale = csOne;
        }

	for ( iter = hist->getFirstInterval() ;
		  iter.isValid() ;
		  iter.next() ) // break when we've processed the last Interval
	{
	  iRows = iter.getRowcount() * iRowsRed;
	  iUec = iter.getUec();

	  iUec = uecScale * iUec;
	  iUec = MINOF(iRows, iUec);

	  if (scale.isLessThanOne() AND isUnique()) // if column is UNIQUE, set uec == rows
		iUec = iRows ;

	  // setRowsAndUec, sets UEC to minimum of rows and uec
	  iter.setRowsAndUec (iRows, iUec);

	  newRowcount += iRows;
	  newUec += MINOF(iUec, iRows);
	}
  }

    // after having scaled the rows in the intervals, 
    // scale the frequencies in the frequentValues list by the same amount
    // rowRedFactor * scale

  if (scaleFreqValList)
  {
    FrequentValueList & frequentValueList = getModifableFrequentValues();
    frequentValueList.scaleFreqAndProbOfFrequentValues(iRowsRed, 1); 
  }

#ifndef DO_NOT_MERGE_INTERVALS

  // Our current histogram semantics say that we do not allow
  // intervals to have uec/rowcount information that is more than
  // 0 and less than 1.  So the following loop goes through all
  // of the intervals and combines them as necessary to conform
  // to this specification.
  //
  // NB: Intervals which have uec/rowcount of 0/0 are legitimate
  //     and should not be forgotten!
  //
  // NB: We leave NULL-instantiated intervals alone

  // the following loop stops when we hit the last interval, having
  // successfully merged all intervals whose uec/rowcount were
  // between 0 & 1 (non-inclusive)
  for ( iter = hist->getFirstInterval() ;
        iter.isValid() && !iter.isNull() ; // do not merge NULL intervals!
        /* no automatic increment */
        )
    {
      if ( iter.canBeMerged() )
        {
          if ( iter.isFirst() ) // combine with 2nd interval
            {
              if ( iter.isLast() ) break ; // only one interval in total; done

              // at this point, we know another interval exists
              Interval next = hist->getNextInterval (iter) ;
              if ( next.isNull() ) break ; // do not merge NULL intervals!
              iter.merge (next) ; // now loop again with iter as before
            }
          else if ( iter.isLast() )
            {
              // can't be the first interval since we already
              // checked that case
              Interval prev = hist->getPrevInterval (iter) ;
              prev.merge (iter) ; // (we only merge "up")

              // prev might have been ==0 before --> so we'll check
              // in next loop
              iter = prev ;
            }
          else // have to choose between neighbors to merge with
            {
              Interval next = hist->getNextInterval (iter) ;
              Interval prev = hist->getPrevInterval (iter) ;
              // have to decide which to merge with
              // decision : merge with the neighbor whose
              //            boundary is closest to mine

              const EncodedValue loBound = iter.loBound() ;
              const EncodedValue hiBound = iter.hiBound() ;
              const EncodedValue prevBound = prev.loBound() ;
              const EncodedValue nextBound = next.hiBound() ;

              // since loBound > prevBound, and nextBound > hiBound,
              // the calculation below should always be correct
              //
              // $$$ clean up this code to use EncodedValue::ratio()
              // $$$ or write another EncodedValue method !!!
              if ( ((loBound.getDblValue() - prevBound.getDblValue()) >=
                   (nextBound.getDblValue() - hiBound.getDblValue())) &&
                   !next.isNull() ) // do not merge NULL intervals!
                {
                  // there's more "distance" between me and
                  // my prev neighbor than between me and
                  // my next neighbor --> so merge with next
                  iter.merge (next) ;
                  // since we haven't looked at next before,
                  // we may need to work with iter again
                }
              else
                {
                  // otherwise, do the opposite
                  prev.merge (iter) ;  // (we only merge "up")

                  // prev might have been ==0 before --> so we'll check
                  // in next loop
                  iter = prev ;
                }
            }
        }
      else
        iter.next() ; // get next Interval
    }

#endif /* #ifndef DO_NOT_MERGE_INTERVALS */

  if (hist->numIntervals() == 0)
    {
      newRowcount = rowcount_ * scale ;
      newUec = MINOF(totalUec_,newRowcount) ;
    }

  // if we are trying to scale a histogram, whose row count is zero, then
  // we don't want to work with intervals, instead we would be better off
  // condensing the intervals of that histogram, and setting the row count
  // and the uec of that histogram to one.

  if ( newRowcount.isZero() )
  {
      if ( hist->entries() > 1 )
        hist->condenseToSingleInterval();

    // Set first interval's rowcount and uec.
    hist->getFirstInterval().setRowsAndUec( csOne, csOne );

    // This rowcount and uec will be used later to set the total rowcount and
    // uec of the histogram. Hence set that to one.
    newRowcount = csOne;
    newUec = csOne;
    setIsCompressed(TRUE);
  }


  setRedFactor    (1.0) ;
  setUecRedFactor (1.0) ;

  if (scale.isGreaterThanOne())
  {
    CostScalar oldRowcount = getRowcount();
     setBaseRowCount(oldRowcount);	//set baseRowCount with the rowCountBefore the cross-products
  }

  //after having set the baseRowCount, now initialize the total rowCount with the newRowCount

  setRowsAndUec   (newRowcount, newUec) ;

}  // copyAndScaleHistogram

// --------------------------------------------------------------------
// ColStats::getAccRowCountAboveOrEqThreshold
// This method returns the total row count and total UEC of intervals
// whose frequency is greater than or equal to the threshold value
// --------------------------------------------------------------------
void 
ColStats::getAccRowCountAboveOrEqThreshold ( CostScalar & accRowCnt, /* out */
											 CostScalar & accUec,   /* out */
											 CostScalar thresVal)
{
  accRowCnt = 0;
  accUec = 0;

  CostScalar thisIterFreq = 1;

  HistogramSharedPtr hist = getHistogram();
  if (hist->numIntervals() == 0)
  {
	// if number of intervals is 0, treat it like a single interval
	// histogram and set the accRowCnt and accUec from total row count
	// and uec of the histogram if the frequency is greater than or
	// equal to the threshold value. Else set them to 0

	thisIterFreq = getRowcount() / getTotalUec();

    if (thisIterFreq >= thresVal)
	{
	  accRowCnt = getRowcount();
	  accUec = getTotalUec();
	}
    return;
  }

  Interval iter = hist->getFirstInterval();

  while ( iter.isValid() && !iter.isNull() )
  {
	// if the frequency of the interval is less than zero, we assume the frequency
	// to be equal to the rowcount

    thisIterFreq = iter.getRowcount()/(iter.getUec()).minCsOne();

    if (thisIterFreq >= thresVal)
	{
	  accRowCnt += iter.getRowcount();
	  accUec += iter.getUec();
	}

    iter.next();
    continue; 
  }

  return;
} // ColStats::getAccRowCountAboveOrEqThreshold

void
ColStats::setMaxFreq(CostScalar val)
{
  if (CmpCommon::getDefault(COMP_BOOL_42) == DF_ON)
  {
    // if there is any rowreduction that still needs to be applied
    // to the histogram, then use that too to adjust frequencies.
    // For example: sum of rowcount from intervals is 1000, and there is
    // one element in the frequent value list, with frequency equal to 100
    // Lets say some reduction has happened to the histogram such that its 
    // rowcount now is 100, this means that the rowreduction factor is 0.1
    // This reduction will be applied to the intervals and the frequent values
    // later, resulting in frequency in teh list to 10.

    val = val * getRedFactor();
    if (scaleFactor_ > csOne)
    {
      maxFreq_ = val/rowcount_;
      maxFreq_ *= scaleFactor_;
    }
    else
      maxFreq_ = val/rowcount_;

    maxFreq_ = maxFreq_.maxCsOne();
  }
  else
    maxFreq_ = val;
}

CostScalar
ColStats::getMaxFreq() const
{
  if (CmpCommon::getDefault(COMP_BOOL_42) == DF_ON)
  {
    if (scaleFactor_ > csOne)
     return maxFreq_*rowcount_/scaleFactor_;
    else
      return maxFreq_*rowcount_;
  }
  return maxFreq_;
}

void 
ColStats::computeMaxFreqOfCol(NABoolean forced)
{
  if ((forced == FALSE) && (getMaxFreq() > csZero))
	return;

	HistogramSharedPtr hist = getHistogram();
	if (hist->numIntervals() == 0)
	{
	  setMaxFreq(csMinusOne);
	  return;
	}

   CostScalar maxFreq = csMinusOne;
   NABoolean useHighFreq = CURRSTMT_OPTDEFAULTS->useHighFreqInfo();
   // Do not have to loop over all intervals if mfv info is availble, as
   // the max frequency of the column is the max of mfvs of these intervals.
   const FrequentValueList &freqList = this->getFrequentValues();
   if (freqList.entries() > 0)
   {
     CostScalar maxFreqFromFreqList = freqList.getMaxFrequency();
     if (maxFreqFromFreqList > maxFreq)
       maxFreq = maxFreqFromFreqList;
   } else { 
       Interval iter = hist->getFirstInterval();
  
       while ( iter.isValid() && !iter.isNull() )
       {
         // if the frequency of the interval is less than zero, we assume the frequency
         // to be equal to the rowcount
    
         //Avoid divide-by-zero exception
         CostScalar iterUec = iter.getUec();
         if(iterUec == csZero)
           iterUec = csOne;
    
         CostScalar thisIterFreq = csZero;
    
         if (useHighFreq)
           thisIterFreq = iter.getRowcount2mfv();
    
         if ( thisIterFreq == csZero )
            thisIterFreq = iter.getRowcount()/iterUec;
    
         if (maxFreq < thisIterFreq)
           maxFreq = thisIterFreq;
    
         iter.next();
     }
  }
  setMaxFreq(maxFreq);
}

// -----------------------------------------------------------------------
// reduceToMaxIntervalCount()
//
// reduce (by merging) the number of histogram intervals to be
// at most maxIntervalCount_, a value that the user has set
// -----------------------------------------------------------------------
void
ColStats::reduceToMaxIntervalCount()
{
  CollIndex maxIntervalCount = getMaxIntervalCount() ;
  if (histogram_->entries() == 0)
    return;
  CollIndex intervalCount    = histogram_->entries() - 1 ;

  // if the user says he wants less than 4 intervals (5 HistInts), don't
  // bother reducing at all; also, if there are already fewer intervals
  // than the user's upper bound, nothing to do.
  if ( intervalCount < 4 || maxIntervalCount < 4 || maxIntervalCount >= intervalCount )
    return ;

  // otherwise, we're definitely going to be modifying this histogram
  getHistogramToModify() ;
  HistogramSharedPtr hist = histogram_ ; // convenience

  // For convenience, we use a very simple algorithm to decide which
  // intervals to merge (we simply merge every N-1 intervals, where N is
  // the "factor" we need to reduce -- that is, the proportion
  // intervalCount : maxIntervalCount )
  CollIndex reductionFactor = intervalCount / maxIntervalCount ;

  // how many more do we have, after we remove the factor?
  const CollIndex additionalRows = intervalCount - (reductionFactor * maxIntervalCount) ;

  // if there are an additional 25% of intervals left over, bump up the reduction factor by 1
  if ( (additionalRows * 1.0) > (maxIntervalCount * 0.25) )
    reductionFactor++ ;

  if ( reductionFactor == 1 ) // we're currently within 20%, close enough
    return ;

  //
  // now, for every (reductionFactor) intervals, merge the first (reductionFactor-1)
  //

  CollIndex numKept = 1, numMerged = 0 ;
  Interval iter = hist->getFirstInterval() ;

  // the following loop attempts to avoid the complexity of boundary conditions
  // --> i.e., keep the first interval, and only loop maxIntervalCount-1 times,
  // to avoid the last-interval/null-interval complexity
  for ( iter = hist->getNextInterval (iter) ;
        iter.isValid() && !iter.isNull() && numKept < maxIntervalCount ;
        /* no automatic increment */
        )
    {
      // if this is the last interval, break. Nothing more to merge
      if ( iter.isLast() ) break ;

      if ( numMerged < (reductionFactor-1) ) // merge the next into the current
        {
          Interval next = hist->getNextInterval (iter) ;

	  // Do not merge intervals that are null or are not valid
	  if ( next.isNull() || !next.isValid()) break ;

          iter.merge (next) ; // now loop again with iter as before
          numMerged++ ;
        }
      else // we've merged (extraFactor-1) already; keep this one & move on
        {
          iter = hist->getNextInterval (iter) ;
          numKept++ ;
          numMerged = 0 ;
        }
    }
} // ColStats::reduceToMaxIntervalCount()

//
// transform the number of histogram intervals to 
// maxIntervalCount_ interval, a value that the user has set.
// 
// This version is different from reduceToMaxIntervalCount() in that
// the transform is driven by the # of rowcount in each interval. 
// 

HistogramSharedPtr ColStats::transformOnIntervals(Int32 numIntvs)
{
  CollIndex intervalCount    = histogram_->entries() - 1 ;

  // for now, just do the transformation for the leading key column
  NAColumnArray& colArray =  statColumns();
  const NAColumn* col = colArray[0];
  const NAType* nt = col->getType();
 
  CostScalar rc = getRowcount();

  CostScalar avgRcPerIntNew = getRowcount() / numIntvs;
  CostScalar currentRcNew = 0;

  CostScalar lowB = getMinValue().getDblValue();
  CostScalar hiB  = getMaxValue().getDblValue();

  HistogramSharedPtr newHist(new(heap_) Histogram (heap_));

  HistogramSharedPtr hist = getHistogram();

  Int32 n = hist->numIntervals();

  Interval iter;
  CostScalar availableRC;
  CostScalar lowBInt;
  CostScalar hiBInt;

  if ( numIntvs > 1 ) {
     for ( iter = hist->getFirstInterval();
           iter.isValid() && !iter.isNull();
           iter = hist->getNextInterval (iter)
         )
     {
         CostScalar rcInt = iter.getRowcount();
   
         lowBInt = iter.loBound().getDblValue();
         hiBInt = iter.hiBound().getDblValue();
   
         // if this is the last interval, break. Nothing more to worry
         if ( iter.isLast() ) break ;
   
         if ( currentRcNew + rcInt < avgRcPerIntNew ) {
            currentRcNew += rcInt;
         } else {
   
            EncodedValue mfv;
            CostScalar freqMFV;
   
            if ( iter.getMFV(getFrequentValues(), mfv, freqMFV) ) {
   
               CostScalar r1;
               iter.getRCSmallerThanMFV(mfv, freqMFV, r1);
   
               CostScalar mfvInSC(mfv.getDblValue());
   
               if ( r1 > 0.0 ) {
   
                  // handle r1
   
                  availableRC = r1;
                  iter.makeSplits(
                              newHist,
                              nt,
                              avgRcPerIntNew, 
                              currentRcNew, 
                              availableRC,
                              lowB, lowBInt, mfvInSC, TRUE
                            );
               }
   
               if ( freqMFV > 0.0 ) {
   
                  // handle mfv
                  availableRC = freqMFV;
                  iter.makeSplits(
                              newHist,
                              nt,
                              avgRcPerIntNew, 
                              currentRcNew, 
                              availableRC, 
                              lowB, mfvInSC, mfvInSC, FALSE
                           );
               }
   
   
               CostScalar r2 = iter.getRowcount() - freqMFV - r1; r2.minCsZero();
   
               if ( r2 > 0.0 ) {
   
                  // handle r2
   
                  availableRC = r2;
                  iter.makeSplits(
                              newHist,
                              nt,
                              avgRcPerIntNew, 
                              currentRcNew, 
                              availableRC, 
                              lowB, mfvInSC, hiBInt, TRUE);
               }
   
            } else {
               // no MFV, do the splits for the entire interval. 
    
               availableRC = rcInt;
   
               iter.makeSplits(
                           newHist,
                           nt,
                           avgRcPerIntNew, 
                           currentRcNew, 
                           availableRC,
                           lowB, lowBInt, hiBInt, TRUE
                          );
            }
   
            // When we reach here: currentRcNew >= 0 and availableRC == 0  
   
         }
      } // for loop
   }

   // insert the last interval
   newHist->insertZeroInterval(lowB, hiB, TRUE /*bound included */);

   return newHist;

} // ColStats::transformOnIntervals()
   

void Interval::makeSplits(
                          HistogramSharedPtr& newHist,
                          const NAType* nt, 
                          const CostScalar newHeight, 
                          CostScalar& newRC,        // rc already filled; 
                                                    // On exit, reset to 0 after a complete fill;
                                                    //          else, the partially filled RC
                          CostScalar& availableRC,  // on extry: rc available; 
                                                    // on exit: 0.0
                          CostScalar& lowB,         // On entry: the low bound to use to insert the new 
                                                    // first interval. 
                                                    // On exit: the current last low bound to use
                                                    // to insert a new interval. 
                          const CostScalar& lowBInt,// the low and high bound in which availableRC
                          const CostScalar& hiBInt, // #rows resides. The two bounds are used to
                                                    // compute the new high bound(s) for new intervals
                          NABoolean allowSplits)     
{
    CostScalar toFill = newHeight - newRC;

    if ( availableRC < toFill )  {
       newRC += availableRC;
       availableRC = 0.0;
       return;
    } else
    if ( availableRC == toFill )  {

       newHist->insertZeroInterval(lowB, hiBInt, TRUE /*bound included */);

       newRC = 0.0;
       availableRC = 0.0;
       lowB = hiBInt;

       return;

    }  else {

      CostScalar hiB;

      if ( allowSplits ) {

         // Do the split
         hiB = lowBInt + ( hiBInt - lowBInt) * ( toFill / availableRC );

         hiB = hiB.round(); // round to closest integer

         if ( hiB > hiBInt ) hiB = hiBInt; // and cap the value by hiBInt

         availableRC -= toFill;

      } else {

         // No split is allowed, take all the rows
         hiB = hiBInt;
         availableRC = 0.0;
      }

      newHist->insertZeroInterval(lowB, hiB, TRUE /*bound included */);
      lowB = hiB;
         
      newRC = 0.0; // reset after a complete fill

      // if all rows are taken, return.
      if ( availableRC == 0.0 )
        return;

    }

    // split the remaining availableRC into multiple newHeight chunks. 
    // For every chunk, create a new interval. The remaining rows are returned
    // without creating a new interval for them.
    while ( availableRC > newHeight ) {

       // split the rows proportionally
       CostScalar split = lowBInt + (hiBInt - lowBInt) * ( newHeight / availableRC );

       split = split.round(); // round to closest integer

       if ( split > hiBInt ) split = hiBInt; // and cap the value by hiBInt
       
       newHist->insertZeroInterval(lowB, split, TRUE /*bound included */);

       lowB = split;

       availableRC -= newHeight;
    }

    newRC = availableRC;

    return;
}

NABoolean
Interval::getMFV(const FrequentValueList& list, 
                 EncodedValue& mfv, CostScalar& freq)
{
  EncodedValue lo = loBound();
  EncodedValue hi = hiBound();

  for (CollIndex index = 0; index < list.entries(); index++)
  {
    mfv = list[index].getEncodedValue();
    if ( ((isLoBoundInclusive() && lo <= mfv) || lo < mfv) && 
         ((isHiBoundInclusive() && mfv <= hi) || mfv < hi) )
    {
       freq = list[index].getFrequency();
       return TRUE;
    }
  }
  return FALSE;
}

//--------------------------------------
// MC version of the interval split code
//--------------------------------------

//
// transform the number of histogram intervals to 
// maxIntervalCount_ interval, a value that the user has set.
// 
// This version is different from reduceToMaxIntervalCount() in that
// the transform is driven by the # of rowcount in each interval. 
// 

HistogramSharedPtr ColStats::transformOnIntervalsForMC(Int32 numIntvs)
{
  CollIndex intervalCount = histogram_->entries() - 1 ;

  CostScalar rc = getRowcount();

  CostScalar avgRcPerIntNew = getRowcount() / numIntvs;
  CostScalar currentRcNew = 0;

  HistogramSharedPtr newHist(new(heap_) Histogram (heap_));

  HistogramSharedPtr hist = getHistogram();

  NormValueList lowbp;
  NormValueList hibp;

  MCboundaryValueList lMCb = hist->getFirstInterval().loMCBound();
  MCboundaryValueList hMCb = hist->getLastInterval().hiMCBound();
  lMCb.getValueList(lowbp);

  hMCb.getValueList(hibp);

  Int32 n = hist->numIntervals();

  Interval iter;
  CostScalar availableRC;
  NormValueList* lowBInt;
  NormValueList* hiBInt;

  if ( numIntvs > 1 ) {
     for ( iter = hist->getFirstInterval();
           iter.isValid() && !iter.isNull();
           iter = hist->getNextInterval (iter)
         )
     {
         CostScalar rcInt = iter.getRowcount();
   
         lowBInt = const_cast<NormValueList*> (iter.loBound().getValueList());
         hiBInt = const_cast<NormValueList*> (iter.hiBound().getValueList());
   
         // if this is the last interval, break. Nothing more to worry
         if ( iter.isLast() ) break ;
   
         if ( currentRcNew + rcInt < avgRcPerIntNew ) {
            currentRcNew += rcInt;
         } else {
   
            MCboundaryValueList mfv;
            CostScalar freqMFV;
   
            if ( iter.getMFV(getMCSkewedValueList(), mfv, freqMFV) ) {
   
               CostScalar r1;
               iter.getRCSmallerThanMFV(mfv, freqMFV, r1);
   
               NormValueList vlist;
               mfv.getValueList (vlist);
               NormValueList* mfvInSC = &vlist;
   
               if ( r1 > 0.0 ) {
   
                  // handle r1
   
                  availableRC = r1;
                  iter.makeSplitsForMC(
                              newHist,
                              avgRcPerIntNew, 
                              currentRcNew, 
                              availableRC,
                              &lowbp, lowBInt, mfvInSC, TRUE
                            );
               }
   
               if ( freqMFV > 0.0 ) {
   
                  // handle mfv
                  availableRC = freqMFV;
                  iter.makeSplitsForMC(
                              newHist,
                              avgRcPerIntNew, 
                              currentRcNew, 
                              availableRC, 
                              &lowbp, mfvInSC, mfvInSC, FALSE
                           );
               }
   
   
               CostScalar r2 = iter.getRowcount() - freqMFV - r1; r2.minCsZero();
   
               if ( r2 > 0.0 ) {
   
                  // handle r2
   
                  availableRC = r2;
                  iter.makeSplitsForMC(
                              newHist,
                              avgRcPerIntNew, 
                              currentRcNew, 
                              availableRC, 
                              &lowbp, mfvInSC, hiBInt, TRUE);
               }
   
            } else {
               // no MFV, do the splits for the entire interval. 
    
               availableRC = rcInt;
   
               iter.makeSplitsForMC(
                           newHist,
                           avgRcPerIntNew, 
                           currentRcNew, 
                           availableRC,
                           &lowbp, lowBInt, hiBInt, TRUE
                          );
            }
   
            // When we reach here: currentRcNew >= 0 and availableRC == 0  
   
         }
      } // for loop
   }

   // insert the last interval
   newHist->insertZeroInterval(lowbp, hibp, TRUE);

   return newHist;

} // ColStats::transformOnInvervals()
   
void Interval::makeSplitsForMC( HistogramSharedPtr& newHist,
                          const CostScalar newHeight, 
                          CostScalar& newRC,            // rc already filled; 
                                                        // On exit, reset to 0 after a complete fill;
                                                        //          else, the partially filled RC
                          CostScalar& availableRC,      // on extry: rc available; 
                                                        // on exit: 0.0
                          NormValueList* lowB,          // On entry: the low bound to use to insert the new 
                                                        // first interval. 
                                                        // On exit: the current last low bound to use
                                                        // to insert a new interval. 
                          NormValueList*& lowBInt,      // the low and high bound in which availableRC
                          NormValueList*& hiBInt,       // #rows resides. The two bounds are used to
                                                        // compute the new high bound(s) for new intervals
                          NABoolean allowSplits)     
{
    CostScalar toFill = newHeight - newRC;

    if ( availableRC < toFill )  {
       newRC += availableRC;
       availableRC = 0.0;
       return;
    } else
    if ( availableRC == toFill )  {

       newHist->insertZeroInterval(*lowB, *hiBInt, TRUE);

       newRC = 0.0;
       availableRC = 0.0;
       *lowB = *hiBInt;

       return;

    }  else {

      NormValueList hiB;

      if ( allowSplits ) {

         // Do the split
         // hiB = lowBInt + ( hiBInt - lowBInt) * ( toFill / availableRC );
         NormValueList x = (*hiBInt);
         x = (x - (*lowBInt)) * ( toFill.getValue() / availableRC.getValue() );
         hiB = x + (*lowBInt);

         hiB.round(); // round to closest integer

         if ( hiB.compare(hiBInt) == MORE ) 
            hiB = *hiBInt; // and cap the value by hiBInt

         availableRC -= toFill;

      } else {

         // No split is allowed, take all the rows
         hiB = *hiBInt;
         availableRC = 0.0;
      }

      newHist->insertZeroInterval(*lowB, hiB, TRUE);
      *lowB = hiB;
         
      newRC = 0.0; // reset after a complete fill

      // if all rows are taken, return.
      if ( availableRC == 0.0 )
        return;

    }

    // split the remaining availableRC into multiple newHeight chunks. 
    // For every chunk, create a new interval. The remaining rows are returned
    // without creating a new interval for them.
    while ( availableRC > newHeight ) {

       // split the rows proportionally
       //NormValueList split = lowBInt + (hiBInt - lowBInt) * ( newHeight.getValue() / availableRC.getValue() );
       NormValueList split = (*hiBInt);
       split = (split - (*lowBInt)) * ( newHeight.getValue() / availableRC.getValue() );
       split = split + (*lowBInt);

       split.round(); // round to closest integer

       if ( split.compare(hiBInt) == MORE ) 
            split = *hiBInt; // and cap the value by hiBInt
       
       newHist->insertZeroInterval(*lowB, split, TRUE);

       *lowB = split;

       availableRC -= newHeight;
    }

    newRC = availableRC;

    return;
}


NABoolean
Interval::getMFV(const MCSkewedValueList& list,  
                 MCboundaryValueList& mfv, CostScalar& freq)
{
  MCboundaryValueList lo = loMCBound();
  MCboundaryValueList hi = hiMCBound();

  for (CollIndex index = 0; index < list.entries(); index++)
  {
    mfv = MCboundaryValueList(list[index]->getEncodedValue()->getValueList());
    if ( ((isLoBoundInclusive() && lo <= mfv) || lo < mfv) && 
         ((isHiBoundInclusive() && mfv <= hi) || mfv < hi) )
    {
       freq = list[index]->getFrequency();
       return TRUE;
    }
  }
  return FALSE;
}


// Guess the rowcount and uec of values smaller than mfv
void Interval::getRCSmallerThanMFV(const MCboundaryValueList& mfv, 
                                         const CostScalar& freqMFV,
                                         CostScalar& rc)
{
   if ( isHiBoundInclusive() && mfv == hiMCBound() ) {
     rc = MIN_ZERO(getRowcount() - freqMFV);
     return;
   }

   if ( isLoBoundInclusive() && mfv == loMCBound() ) {
     rc = 0.0;
     return;
   }

   // mfv is somewhere in the middle of the range. Assume mfv divides
   // the range equally and half of the values smaller than it.

   rc =  MIN_ZERO((getRowcount() - freqMFV) / 2);

   return;
}

// Guess the rowcount and uec of values smaller than mfv
void Interval::getRCSmallerThanMFV(const EncodedValue& mfv, 
                                         const CostScalar& freqMFV,
                                         CostScalar& rc)
{
   if ( isHiBoundInclusive() && mfv == hiBound() ) {
     rc = MIN_ZERO(getRowcount() - freqMFV);
     return;
   }

   if ( isLoBoundInclusive() && mfv == loBound() ) {
     rc = 0.0;
     return;
   }

   // mfv is somewhere in the middle of the range. Assume mfv divides
   // the range equally and half of the values smaller than it.

   rc =  MIN_ZERO((getRowcount() - freqMFV) / 2);

   return;
}


// -----------------------------------------------------------------------
// nullAugmentHistogram
//
// Increase the rowcount by adding a NULL interval with
//       targetRowCount - rowcount_ NULLs
// -----------------------------------------------------------------------
void
ColStats::nullAugmentHistogram(CostScalar targetRowCount)
{
  HistogramSharedPtr targetHistogram = getHistogramToModify();

  NABoolean insertNULLSkewValue = FALSE;
  CostScalar nullFreq;

  if ( NOT isNullInstantiated() )
  {
    insertNullInterval() ; // if there wasn't one already, there is now
    insertNULLSkewValue = TRUE;
  }

  // Since the rowcount and uecs are always rounded before being stored into
  // the histogram. To make a fair comparison, round the targetRowCount too
  // This would avoid situations where the targetRowCount is say 19.6 and the rowcount
  // of the histogram is 20. It is possible to have fractional target rowcount
  // because of the costscalar arithmetic. But if after rounding the targetRowCount
  // becomes smaller than the initial rowcount of the histogram, then we need to 
  // investigate. Sol: 10-090115-8452 
  targetRowCount = targetRowCount.round();
  
  CostScalar difference = targetRowCount - rowcount_ ;
  if (difference < 0)
  {
    // if for some reason the numbers of rows to be augmented > rowcount
    // of the histogram, reduce the targetRowcount, so that the difference
    // is treated as zero. This basically means that there is no NULL interval
    // added to the histogram
    CCMPASSERT (difference.isGreaterOrEqualThanZero()) ;
    difference = 0;
  }

  if ( difference.isZero())
    {
      setNullRowsAndUec (0,0) ;
      nullFreq = csZero;
    }
  else
    {
      CostScalar nullRows = 0 ;
      if ( NOT rowRedFactor_.isExactlyZero() ) // avoid div-by-zero!
        nullRows = difference / rowRedFactor_ ;

      nullRows += getNullCount();

      CostScalar nullUec  = MINOF(nullRows, 1) ; // not more than nullRows!

      setNullRowsAndUec  (nullRows, nullUec) ;
      setRowsAndUec (targetRowCount, totalUec_ + (nullUec * uecRedFactor_)) ;
      //                                          ^^^^^^^^^^^^^^^^^^^^^^^
      //                                           (probably less than 1)
	  nullFreq = nullRows;
    }

  if ( histogram_->numIntervals() == 1 ) // i.e., only NULL values in histogram
    {
      setMaxMinValuesFromHistogram() ;
    }

  // insert NULL skew value too
  if ( (insertNULLSkewValue) )
  {
    UInt32 hashValue = 666654765;  // hash value for NULL as used by the executor in exp_functions.cpp
    EncodedValue boundary;
    boundary.setValueToNull();
    FrequentValueList &svList = getModifableFrequentValues();

    FrequentValue newV(hashValue, nullFreq, csOne, boundary);
    svList.insertFrequentValue(newV);
  }
}  // nullAugmentHistogram

// --------------------------------------------------------------------
// ColStats::makeGrouped
//
// Following a GroupBy operation (In the special case where a single
// ColStats covers all grouping columns), intervals within that columns
// histogram can't have more rows than they have unique values.
// --------------------------------------------------------------------
void
ColStats::makeGrouped()
{
  HistogramSharedPtr targetHistogram = getHistogramToModify();

  //$$$ we handle the zero-interval case below
  //  if ( targetHistogram->numIntervals() == 0 )
  //  return; // nothing to do.

  CostScalar totalRowCount = 0;

  Interval iter ;

  for ( iter = targetHistogram->getFirstInterval() ;
        iter.isValid() ;
        iter.next() )
  {
	CostScalar oldRC = iter.getRowcount();
	CostScalar newRC = MINOF(oldRC, iter.getUec());

	iter.setRowsAndUec (newRC, newRC) ;

	totalRowCount += newRC ;

	// Remove the frequent value list for this histogram, as now the
	// frequency of each value will be 1
	if ( (oldRC != newRC) )
	{
      frequentValues_.clear();
	}
  }

  if ( targetHistogram->numIntervals() == 0)
    totalRowCount = MINOF( rowcount_ * rowRedFactor_ , totalUec_ * uecRedFactor_) ;

  setRedFactor    (1.0) ;
  setUecRedFactor (1.0) ;
  setRowsAndUec   (totalRowCount, totalRowCount) ;

  setShapeChanged (TRUE) ;
} // makeGrouped()

// -----------------------------------------------------------------------
// To be called from the debugger
void
ColStats::display() const
{
  ColStats::print();
}

void
ColStats::print (FILE *f, const char * prefix, const char * suffix,
                 CollHeap *c, char *buf, NABoolean hideDetail) const
{
  Space * space = (Space *)c;
  char mybuf[1000];
  if (!hideDetail)
  {
  snprintf(mybuf, sizeof(mybuf), "%sHistogram ID = " PF64 " %s\n", prefix, histogramID_.getKey(), suffix);
  PRINTIT(f, c, space, buf, mybuf);
  }

  if (isFakeHistogram())
  {
    sprintf(mybuf, "***FAKE*** histogram\n");
    PRINTIT(f, c, space, buf, mybuf);
  }
  if (isOrigFakeHist())
  {
    sprintf(mybuf, "***Histogram with NO statistics\n");
    PRINTIT(f, c, space, buf, mybuf);
  }
  if (isSmallSampleHistogram())
  {
    sprintf(mybuf, "***Histogram with SMALL SAMPLE statistics\n");
    PRINTIT(f, c, space, buf, mybuf);
  }
  if (isRecentJoin())
  {
    sprintf(mybuf, "***RECENT JOIN***\n");
    PRINTIT(f, c, space, buf, mybuf);
  }
  if (isUnique())
  {
    sprintf(mybuf, "***UNIQUE COLUMN***\n");
    PRINTIT(f, c, space, buf, mybuf);
  }
  if (isMinSetByPred() || isMaxSetByPred())
    {
    sprintf(mybuf, "***") ;
    PRINTIT(f, c, space, buf, mybuf);
      if (isMinSetByPred())
    {
      sprintf(mybuf,"MIN");
      PRINTIT(f, c, space, buf, mybuf);
    }
      if (isMaxSetByPred())
    {
      sprintf(mybuf,"MAX");
      PRINTIT(f, c, space, buf, mybuf);
    }
    sprintf(mybuf, " SET BY PRED***\n");
    PRINTIT(f, c, space, buf, mybuf);
  }

  if (isSelectivitySetUsingHint())
  {
    sprintf(mybuf, "***SELECTIVITY SET USING HINT***\n");
    PRINTIT(f, c, space, buf, mybuf);
  }

  if (!hideDetail)
  {
  sprintf(mybuf, "Columns:\n");
  PRINTIT(f, c, space, buf, mybuf);

  columns_.print(f, DEFAULT_INDENT, "NAColumnArray", c, buf);
  }

  snprintf(mybuf, sizeof(mybuf), "%s   TotalUEC = %f \n", prefix, totalUec_.value());
  PRINTIT(f, c, space, buf, mybuf);

  snprintf(mybuf, sizeof(mybuf), "%s   Rowcount = %f \n", prefix, rowcount_.value());
  PRINTIT(f, c, space, buf, mybuf);

  snprintf(mybuf, sizeof(mybuf), "%s   BaseUEC  = %f (pre-current-join-uec)\n", 
          prefix, baseUec_.value());
  PRINTIT(f, c, space, buf, mybuf);

  snprintf(mybuf, sizeof(mybuf), "%s   Max Frequency = %f \n", 
          prefix, getMaxFreq().value());
  PRINTIT(f, c, space, buf, mybuf);

  snprintf(mybuf, sizeof(mybuf), "%s   Encoded MinValue = ", prefix);
  PRINTIT(f, c, space, buf, mybuf);
  minValue_.display (f, DEFAULT_INDENT, "", c, buf);

  snprintf(mybuf, sizeof(mybuf), "\n%s   Encoded MaxValue = ", prefix);
  PRINTIT(f, c, space, buf, mybuf);
  maxValue_.display (f, DEFAULT_INDENT, "", c, buf);

  snprintf(mybuf, sizeof(mybuf), "\n%s   RowRedFactor = %f;  UecRedFactor = %f %s\n",
	  prefix, rowRedFactor_.value(), uecRedFactor_.value(), suffix);
  PRINTIT(f, c, space, buf, mybuf);

  // display the frequent value list
  NABoolean displayMFV = (CmpCommon::getDefault(USTAT_SHOW_MFV_INFO) == DF_ON);
  if (displayMFV) 
  {
  if (frequentValues_.entries() != 0)
    {
      frequentValues_.print(f, "   ","",c,buf);
    }
  else
    {
      sprintf(mybuf,"Empty frequentValues_\n");
      PRINTIT(f, c, space, buf, mybuf);
    }
  }

  // Now, display the histogram
  if (histogram_ != NULL)
  {
    histogram_->print(f, "   ", "", c, buf);
  }
  else
  {
    sprintf(mybuf,"NULL histogram_!\n");
    PRINTIT(f, c, space, buf, mybuf);
}
}

void ColStats::trace(FILE* f, NATable* table)
{
  fprintf (f, "histogram:");
  populateColumnSetFromColumnArray();
  colPositions_.printColsFromTable(f, table);
  Int64 templl = (Int64) getTotalUec().value();
  fprintf (f, "uec:" PF64 " ", templl);
  templl = (Int64) getRowcount().value();
  fprintf (f, "rowcount:" PF64 " ", templl);
  fprintf (f, "intervals:%d \n", (*histogram_).entries());
}

// -----------------------------------------------------------------------
// When one, or both, of the two to-be-combined column statistics has no
//   histogram it is still possible to (sometimes) create a useful result
//   histogram.  This private utility routine attempts to deal with that
//   case.   There are two cases to deal with:
//   - a {legitimate} zero-row ColStats;
//   - a manufactured ColStats with UEC, and RowCount but no histogram.
// -----------------------------------------------------------------------
void
ColStats::mergeWithEmptyHistogram (const ColStatsSharedPtr& otherStats,
                                   MergeType mergeMethod)
{
  CostScalar leftRowCount  = getRowcount();
  CostScalar leftUEC       = getTotalUec();
  CostScalar rightRowCount = otherStats->getRowcount();
  CostScalar rightUEC      = otherStats->getTotalUec();

  CostScalar maxUEC = MAXOF (leftUEC, rightUEC) ;
  sumOfMaxUec_ = MAXOF(sumOfMaxUec_, MAXOF(otherStats->getSumOfMaxUec(), maxUEC));

  CostScalar originalRowCount = leftRowCount ;

  CostScalar numUec = 0;
  CostScalar numRows = 0;

  NABoolean attributesSet = FALSE;

  switch (mergeMethod) {
  case INNER_JOIN_MERGE:
  case OUTER_JOIN_MERGE:
    numUec = MINOF( leftUEC, rightUEC );

    if (numUec.isGreaterThanZero() && originalRowCount.isGreaterThanZero())
      numRows = ( leftRowCount * rightRowCount ) / maxUEC / originalRowCount;
    break;

  case SEMI_JOIN_MERGE:
    numUec = MINOF( leftUEC, rightUEC );

    if (numUec.isGreaterThanZero())
      {
	numRows = leftRowCount * ( numUec / leftUEC );

	// When there is a fractional number of rows in a bucket of
	// the inner table, the number of row calculated for inner-
	// joins can be less than that calculated for a semi-joins.
	// In real life, the number or rows from an inner-join can
	// never be less that that for the similar semi-join.

	// CostScalar numRowsTemp = leftRowCount * rightRowCount /
        //   MAXOF( leftUEC, rightUEC );
        //
	// numRows = ( numRows <= numRowsTemp ? numRows : numRowsTemp );
      }
    else
      numRows = 0 ;

    baseUec_ = MINOF(baseUec_, otherStats->baseUec_);
    uecBeforePred_ = MINOF(uecBeforePred_, otherStats->uecBeforePred_);
    break;

  case ANTI_SEMI_JOIN_MERGE:
      numUec = MAXOF((CostScalar)CostPrimitives::getBasicCostFactor( HIST_DEFAULT_SEL_FOR_JOIN_EQUAL ) * leftUEC, 
                     leftUEC - rightUEC) ;
      if (numUec.isGreaterThanZero()) // implies leftUEC > 0, no div-zero possibility
        numRows = leftRowCount * ( numUec / leftUEC ) ;

    baseUec_ = MINOF(baseUec_, otherStats->baseUec_) ;
    uecBeforePred_ = MINOF(uecBeforePred_, otherStats->uecBeforePred_);

    break ;

  case LEFT_JOIN_OR_MERGE:

    // After the result of the inner join portion of an Outer Join is
    // known, one needs to do something like an OR between that inner
    // join result (*this) and the original pre-join column's histogram
    // (*otherStats), to calculate the actual outer join result.
    if (rightUEC.isZero())
      numUec = 0;
    else
      numUec = MIN_ONE (rightUEC) ;

    // The rowCount varies on a case by case basis
    if (leftUEC.isZero())
      {
	// if innerjoin result has no rows, all rows are from original
	setMinValue( otherStats->getMinValue() );
	setMaxValue( otherStats->getMaxValue() );
	if (otherStats->getHistogram() == NULL)
	  setHistogram ( new (HISTHEAP) Histogram (HISTHEAP) );
	else
	  setHistogram ( new (HISTHEAP)
                         Histogram (*(otherStats->getHistogram()), HISTHEAP) );

	setRedFactor    (otherStats->getRedFactor()) ;
	setUecRedFactor (otherStats->getUecRedFactor()) ;
	setRowsAndUec   (rightRowCount, numUec) ;
	attributesSet = TRUE;
      }
    else if (numUec.isZero())
      {
	// if original has no rows, then result also has no rows
	numUec = 0;
	numRows = 0;
      }
    else
      {
	// else result is all innerjoin rows + original unmatched rows
	numRows = leftRowCount +
	  ((rightRowCount / numUec) * (numUec - leftUEC));

	// guarantee rowCount is never less than it was originally.
	//  (the above formula can/will improperly decrease it)
	numRows = MAXOF( numRows, rightRowCount );
      }

    break;

  case UNION_MERGE:

    // if one of the row-counts is Zero, then the histogram from the
    // other colStats can/should be retained.
    // With the reduction factors from that other histogram......
    if (leftRowCount.isZero()  && rightRowCount.isGreaterThanZero())
      {
	setMinValue (otherStats->getMinValue());
	setMaxValue (otherStats->getMaxValue());
	if (otherStats->getHistogram() == NULL)
	  setHistogram ( new (HISTHEAP) Histogram (HISTHEAP) );
	else
	  setHistogram ( new (HISTHEAP)
                         Histogram (*(otherStats->getHistogram()),
                                    HISTHEAP) );

	setRedFactor    (otherStats->getRedFactor()) ;
	setUecRedFactor (otherStats->getUecRedFactor()) ;
	setRowsAndUec   (rightRowCount, rightUEC) ;
	attributesSet = TRUE;
      }
    else if (rightRowCount.isZero()  && leftRowCount.isGreaterThanZero())
      {
	attributesSet = TRUE; // no-op.  The result is what is presently in THIS.
      }
    else
      {
	numUec = maxUEC ;
	numRows = leftRowCount + rightRowCount;
      }

    break;

  case OR_MERGE:

    // if one of the row-counts is Zero, then the histogram from the
    // other colStats can/should be retained.
    if (leftRowCount.isZero() && rightRowCount.isGreaterThanZero())
      {
        setMinValue (otherStats->getMinValue()) ;
	setMaxValue (otherStats->getMaxValue()) ;
	if (otherStats->getHistogram() == NULL)
	  setHistogram ( new (HISTHEAP) Histogram (HISTHEAP) );
	else
	  setHistogram (new (HISTHEAP)
                        Histogram (*(otherStats->getHistogram()), HISTHEAP) );

	setRedFactor    (otherStats->getRedFactor()) ;
	setUecRedFactor (otherStats->getUecRedFactor()) ;
	setRowsAndUec   (rightRowCount, rightUEC) ;
	baseUec_ = rightUEC ;
        uecBeforePred_ = otherStats->getUecBeforePreds();
	attributesSet = TRUE;
      }
    else if (rightRowCount.isZero()  && leftRowCount.isGreaterThanZero())
      {
	attributesSet = TRUE; // no-op.  The result is what is presently in THIS.
      }
    else
      {
	numUec = maxUEC ;
	numRows = MAXOF( leftRowCount, rightRowCount );

        baseUec_ = numUec;
      }
    break;

  case AND_MERGE:
    // if either histogram's rowcount is zero, the result is zero
    if (leftRowCount.isZero() || rightRowCount.isZero())
      {
        clearHistogram() ;
        attributesSet = TRUE;
      }
    else // we do the best we can
      {
        numUec  = MINOF (leftUEC, rightUEC) ;
        numRows = MINOF (leftRowCount, rightRowCount) ;

        baseUec_ = numUec ;
      }
    break ;

  default:
    CCMPASSERT(FALSE) ; // should never happen!
    // but if it does, we will compute it like a cross product
    break ;
  }

  if(!attributesSet)
  {
    setMinValue (UNINIT_ENCODEDVALUE);
    setMaxValue (UNINIT_ENCODEDVALUE);
    setHistogram ( new (HISTHEAP) Histogram (HISTHEAP) );

    setRedFactor    (1.0) ;
    setUecRedFactor (1.0) ;
    setRowsAndUec   (numRows, numUec) ;
  }

  if (isAJoinRelatedMerge(mergeMethod))
  {
    // Make adjustments to the resulting UEC and rowcount if the UECs were
    // reduced due to independent predicates (preds not on this column)
    //
    //  Use the baseUec_ to determine the amount of original matching and the
    //  newUec to determine the amount of overlap

    CostScalar  selAdj = csZero ;

    if ( totalUec_.isZero() && otherStats->totalUec_.isZero() )
      ; // avoid div-by-zero
    else if (baseUec_ < otherStats->baseUec_)
      {
        if ( otherStats->baseUec_.isGreaterThanZero() ) // avoid div-by-zero!
          {
            selAdj = ((baseUec_ / otherStats->baseUec_) * (otherStats->totalUec_
                / MINOF(otherStats->totalUec_, totalUec_))).maxCsOne();
            selAdj *= ((numUec / MINOF(otherStats->totalUec_, totalUec_))).maxCsOne();
          }
      }
    else // baseUec_ >= otherStats->baseUec_
      {
        if ( baseUec_.isGreaterThanZero() ) // avoid div-by-zero!
          {
            selAdj = ((otherStats->baseUec_ / baseUec_) * (totalUec_
                / MINOF(otherStats->totalUec_, totalUec_))).maxCsOne();
            selAdj *= ((numUec / MINOF(otherStats->totalUec_, totalUec_))).maxCsOne();
          }
      }

    numRows *= selAdj;
    numUec *= selAdj;
    setRedFactor    (selAdj) ;
    setUecRedFactor (selAdj) ;
    setRowsAndUec   (numRows, numUec) ;
  }

}  // mergeWithEmptyHistogram


NABoolean
ColStats::handleMergeTemplateWithZeroIntervals(const ColStatsSharedPtr& otherStats, 
						HistogramSharedPtr& leftHistogram)
{
  // We need to check : are there zero intervals in the template?
  // if so, we probably want to change that so that we get a single
  // interval in the template (from MIN(minvalues) to MAX(maxvalues)) with
  // 1 row/uec
  //
  // --> of course, don't do this if both MIN(maxvalues) and
  // MAX(minvalues) (the inner, non-intersecting boundary values) have
  // their respective max(min)-set-by-pred flags to be true
  // ----------------------------------------------------------------

  //
  // we clearly have non-overlapping histograms;
  //
  // this is strictly less than other (or vice versa)
  //
  //  |      |     |       |
  //  |      |     |       |
  //    this         other
  // t.m    t.M   o.m     o.M    (t.m = this.min; t.M = this.Max; etc.)
  //
  // But we have to be very careful with NULL values.  Our best bet is
  // to create new copies of leftHistogram,rightHistogram, remove their
  // NULL intervals (if any), and then see if either has zero intervals
  // after that -- if so, then the empty template-histogram is correct.
  //
  // Otherwise, there are two cases to consider:
  //
  // where t.m <= t.M < o.m <= o.M // CASE 1
  //
  // (or o.m <= o.M < t.m <= t.M)  // CASE 2

  NABoolean isResultAFakeHistogram = FALSE;
  HistogramSharedPtr thisCopy(new Histogram(*histogram_, HISTHEAP));
  HistogramSharedPtr otherCopy(new Histogram(*(otherStats->getHistogram()), HISTHEAP));

  HistIntVal thisMin  (thisCopy->firstHistInt()) ;
  HistIntVal otherMin (otherCopy->firstHistInt()) ;

  HistIntVal thisMax  (thisCopy->lastHistInt()) ;
  HistIntVal otherMax (otherCopy->lastHistInt()) ;

  // remove the NULL intervals from the copies ('cuz we're building
  // an equi-merge template)
  if ( thisCopy->isNullInstantiated() )   thisCopy->removeNullInterval() ;
  if ( otherCopy->isNullInstantiated() )  otherCopy->removeNullInterval() ;

  // if either of these histograms has zero intervals in it (before or
  // after we remove the NULL intervals) then the merge result is zero
  NABoolean eitherIsJustNULLs = ( (thisCopy->entries() == 0) OR
                                  (otherCopy->entries() == 0) ) ;

  EncodedValue max, innerMax, min, innerMin ;
  NABoolean innerMaxSetByPred = FALSE, innerMinSetByPred = FALSE;
  if(!eitherIsJustNULLs)
  {
    if ( otherMax < thisMax )
      { // CASE 1 above
	DCMPASSERT ( otherMax <= thisMin ) ; // sanity check
	max      = this->getMaxValue() ;
	innerMin = this->getMinValue() ;

	innerMinSetByPred = this->isMinSetByPred() ;
	innerMaxSetByPred = otherStats->isMaxSetByPred() ;

	innerMax = otherStats->getMaxValue() ;
	min      = otherStats->getMinValue() ;
      }
    else
      { // CASE 2 above
	DCMPASSERT ( thisMax <= otherMin) ; // sanity check
	max      = otherStats->getMaxValue() ;
	innerMin = otherStats->getMinValue() ;

	innerMinSetByPred = otherStats->isMinSetByPred() ;
	innerMaxSetByPred = this->isMaxSetByPred() ;

	innerMax = this->getMaxValue() ;
	min      = this->getMinValue() ;
      }
  }
  if ( (innerMinSetByPred AND innerMaxSetByPred) OR eitherIsJustNULLs )
    {
      // two cases where we accept that the template histogram should
      // be NULL :
      //   1. the inner boundaries were both set by predicates
      //   2. one (or both) of the source histograms is just a NULL interval
      //      (which disappears during the equi-merge)
    }
  else
    // otherwise, we need to create a fake, 1 interval histogram spanning
    // max and min
    {
      if ( innerMinSetByPred )
        {
          // we know that the minimum can't be smaller than the innerMin
          min = innerMin ;
        }
      else if ( innerMaxSetByPred )
        {
          // we know that the maximum can't be larger than the innerMax
          max = innerMax ;
        }
      leftHistogram->insertZeroInterval (min, max, TRUE) ;
      // finally, update the fake histogram flag
      isResultAFakeHistogram = TRUE ;
    }
  return isResultAFakeHistogram;
}

// -----------------------------------------------------------------------
// ColStats::newLowerBound
//
// The following method is invoked to synthesize the effect of a
//     column >(=) lowBound predicate.
// -----------------------------------------------------------------------
void
ColStats::newLowerBound (const EncodedValue & newLoBound,
                         ConstValue* constExpr, NABoolean boundIncluded)
{
  getHistogramToModify() ;

  //
  // in all cases, we remove any existing NULL values
  //
  removeNullInterval() ;

  //
  // if there aren't any Intervals, we're done
  //
  if ( histogram_->numIntervals() == 0 OR
       getRowcount().isZero() OR getTotalUec().isZero() )
    {
      clearHistogram() ;
      return ;
    }

  Interval first = histogram_->getFirstInterval() ;
  Interval last = histogram_->getLastInterval() ;

  //
  // several cases to try :
  //
  // CASE 1: if the new lower bound is less than the current
  //         lower bound ==> check :
  //           if ( minBoundSetByPred_ ) already, do nothing
  //           o.w., set minBoundSetByPred_ = TRUE, and create a
  //           0-row/0-uec Interval at the bottom of the Histogram
  //

  if ( newLoBound < first.loBound() )
    {
      if ( isMinSetByPred() == FALSE )
        {
          first.setLoBound (newLoBound) ;
          first.setLoBoundInclusive (boundIncluded) ;
          minValue_ = newLoBound ;
          setMinSetByPred (TRUE) ;
          setShapeChanged (TRUE) ; // $$$ is this right?
        }
      return ; // this new interval does not affect the row/uec aggregates
    }

  // CASE 2:   if the new lower bound is equal to the current
  //           lower bound
  //      2a : isLoBoundInclusive() == TRUE  && boundIncluded == TRUE
  //           <  <= <=
  //           |  |  |  [3,7]  [3,inf)   set minSetByPred_ = TRUE
  //           3  7  9
  //      2b : isLoBoundInclusive() == TRUE  && boundIncluded == FALSE
  //           <  <= <=
  //           |  |  |  [3,7]  (3,inf)   removeSingleValue(3) --> result: (3,7]
  //           3  7  9
  //      2c : isLoBoundInclusive() == FALSE && boundIncluded == TRUE
  //           <= <= <=
  //           |  |  |  (3,7]  [3,inf)   if !minSetByPred_, add a zero-row SVI (value 3)
  //           3  7  9                                      and set minSetByPred_ = TRUE
  //      2d : isLoBoundInclusive() == FALSE && boundIncluded == FALSE
  //           <= <= <=
  //           |  |  |  (3,7]  (3,inf)   set minSetByPred_ = TRUE
  //           3  7  9
  if ( newLoBound == first.loBound() )
    {
      if ( first.isLoBoundInclusive() == boundIncluded )
        {
          setMinSetByPred (TRUE) ;
        }
      else if ( first.isLoBoundInclusive() == TRUE )
        {
          removeSingleValue (newLoBound, constExpr) ;
        }
      else
        {
          if ( isMinSetByPred() == FALSE )
            {
              first.setLoBound (newLoBound) ;
              first.setLoBoundInclusive (boundIncluded) ;
              minValue_ = newLoBound ;
              setMinSetByPred (TRUE) ;
              setShapeChanged (TRUE) ;
            }
        }
      return ; // in all cases, we're done
    } // newLoBound == first.loBound()

  // CASE 3: if the new lower bound is greater than the current
  //         upper bound ...
  //         --> in normal circumstances, we simply say phooey, this
  //         results in zero rows, end of story
  //         --> however, due to our semantics of "trusting"
  //         the user and using the min/maxSetByPred_ flags, we never
  //         return 0 rows unless we're 100% *certain* the result is 0 rows
  //     3a: new lower bound is greater than the max value allowed
  //         by this datatype
  //     3b: maxSetByPred_ is TRUE
  // ==> for both A & B, we zero-out the histogram
  //     3c: otherwise
  // ==> for this case, we create a new histogram, with one interval,
  //     from the new lower boundary to the upper limit of this datatype's
  //     values, and give this interval 1 row/1 uec
  if ( newLoBound >  last.hiBound() ||
       // see the comments for case 4 (b-d) below to understand the rest of this
       // logical expression
       ( newLoBound == last.hiBound() &&
         (last.isHiBoundInclusive() == FALSE || boundIncluded == FALSE) ) )
    {
      // in all cases, the result is now fake
      setFakeHistogram (TRUE) ;

      // first, calculate the max upper value of this datatype
      EncodedValue datatypeMaxValue (WIDE_("(<)"), columns_ ) ;

      if ( newLoBound > datatypeMaxValue || isMaxSetByPred() == TRUE )
        clearHistogram() ;
      else
      {
        // NB: this is NOT the same as setToSingleValue !
        // (this sets all the flags except fake hist)
        setToSingleInterval (newLoBound, datatypeMaxValue, 1, 1) ;
        getModifableFrequentValues().deleteFrequentValuesBelowOrEqual (newLoBound, TRUE) ;
      }
      return ;
    }

  // CASE 4:   if the new lower bound is equal to the current
  //           upper bound
  //      4a : isHiBoundInclusive() == TRUE  && boundIncluded == TRUE
  //           <  <= <=
  //           |  |  |    (7,9]  [9,inf)   setToSingleValue(9)
  //           3  7  9
  //      4b : isHiBoundInclusive() == TRUE  && boundIncluded == FALSE
  //           <  <= <=
  //           |  |  |    (7,9]  (9,inf)   nix entire histogram
  //           3  7  9
  //      4c : isHiBoundInclusive() == FALSE && boundIncluded == TRUE
  //           <  <= <
  //           |  |  |    (7,9)  [9,inf)   nix entire histogram
  //           3  7  9
  //      4d : isHiBoundInclusive() == FALSE && boundIncluded == FALSE
  //           <  <= <
  //           |  |  |    (7,9)  (9,inf)   nix entire histogram
  //           3  7  9
  if ( newLoBound == last.hiBound() )
    {
      // the flags are both TRUE, since we covered the other cases above
      setToSingleValue (newLoBound, constExpr) ;
      return ;
    }

  // CASE 5: newLoBound is between the current hi/lo values of the Histogram
  //         (the usual case)

  // first, find the Interval containing this value
  // next, divide that interval into two pieces, as necessary
  // third, remove all Intervals above the bottom piece of that Interval

  // to differentiate the results between > and >=, we always
  // insert a SVI at the boundary value in the case of >= ;
  // similar to how we assume the user "knows something" when
  // he specifies equality with something that's below the histogram's
  // boundaries, we are assuming that the value associated with
  // the >= predicate has some significance.
  if ( boundIncluded )
    {
      histogram_->insertSingleValuedInterval (newLoBound) ;
      divideHistogramAlongBoundaryValue (newLoBound, ITM_GREATER_EQ) ;
    }
  else
    {
      divideHistogramAlongBoundaryValue (newLoBound, ITM_GREATER) ;
    }

  //
  // cleanup: how many rows & uecs remain?
  //
  const CostScalar oldTotalUec = totalUec_ ;
  setRowsAndUecFromHistogram() ;
  baseUec_ = baseUec_ / oldTotalUec * totalUec_ ;

  minValue_     = newLoBound ;
  setMinSetByPred (TRUE) ;

  // sanity check before we go
  first = histogram_->getFirstInterval() ;
  if (first.loBound() != newLoBound)
  {
    CCMPASSERT (first.loBound() == newLoBound) ;
    // These should be equal, since we made sure, just in case it is not
    // set that equal, and make histogram fake.
    first.setLoBound(newLoBound);
    setFakeHistogram(TRUE);
  }
}



// -----------------------------------------------------------------------
// Synthesize the effect of column <(=) newUpBound
// -----------------------------------------------------------------------
void
ColStats::newUpperBound (const EncodedValue & newUpBound, ConstValue* constExpr,
                         NABoolean boundIncluded)
{
  getHistogramToModify() ;

  //
  // in all cases, we remove any existing NULL values
  //
  removeNullInterval() ;

  //
  // if there aren't any Intervals, we're done
  // if there aren't any rows or uecs, we're also done
  //
    if ( histogram_->numIntervals() == 0 ||
       getRowcount().isZero() || getTotalUec().isZero() )
    {
      clearHistogram() ; // nix the entire thing
      return ;
    }

  Interval first = histogram_->getFirstInterval() ;
  Interval last = histogram_->getLastInterval() ;

  //
  // several cases to try :
  //
  // CASE 1: if the new upper bound is greater than the current
  //         greater bound ==> check :
  //           if ( maxBoundSetByPred_ ) already, do nothing
  //           o.w., set maxBoundSetByPred_ = TRUE, and create a
  //         0-row/0-uec Interval at the top of the Histogram
  //
  if ( newUpBound >  last.hiBound() )
    {
      if ( isMaxSetByPred() == FALSE)
        {
          last.setHiBound (newUpBound) ;
          last.setHiBoundInclusive (boundIncluded) ;
          maxValue_ = newUpBound ;
          setMaxSetByPred (TRUE) ;
          setShapeChanged (TRUE) ;
        }
      return ; // this new interval does not affect the row/uec aggregates
    }

  // CASE 2 : if the new upper bound is equal to the current
  //          upper bound
  //      2a : isHiBoundInclusive() == TRUE  && boundIncluded == TRUE
  //           <  <= <=
  //           |  |  |  (7,9]  (-inf,9]   set maxSetByPred_ = TRUE
  //           3  7  9
  //      2b : isHiBoundInclusive() == TRUE  && boundIncluded == FALSE
  //           <  <= <=
  //           |  |  |  (7,9]  (-inf,9)   removeSingleValue(9) --> result: (7,9)
  //           3  7  9
  //      2c : isHiBoundInclusive() == FALSE && boundIncluded == TRUE
  //           <  <= <
  //           |  |  |  (7,9)  (-inf,9]   if !maxSetByPred_, add a zero-row SVI (value 9)
  //           3  7  9                                       and set maxSetByPred_ = TRUE
  //      2d : isHiBoundInclusive() == FALSE && boundIncluded == FALSE
  //           <  <= <
  //           |  |  |  (7,9)  (-inf,9)   set maxSetByPred_ = TRUE
  //           3  7  9
  if ( newUpBound == last.hiBound() )
    {
      if ( last.isHiBoundInclusive() == boundIncluded )
        {
          setMaxSetByPred (TRUE) ;
        }
      else if ( last.isHiBoundInclusive() == TRUE )
        {
          removeSingleValue (newUpBound, constExpr);
        }
      else
        {
          if ( isMaxSetByPred() == FALSE )
            {
              last.setHiBound (newUpBound) ;
              last.setHiBoundInclusive (boundIncluded) ;
              maxValue_ = newUpBound ;
              setMaxSetByPred (TRUE) ;
              setShapeChanged (TRUE) ;
            }
        }
      return ; // in all cases, we're done
    } // newUpBound == last.hiBound()


  // CASE 3: if the new upper bound is less than the current
  //         lower bound ...
  //         --> in normal circumstances, we simply say phooey, this
  //         results in zero rows, end of story
  //         --> however, due to our semantics of "trusting"
  //         the user and using the min/maxSetByPred_ flags, we never
  //         return 0 rows unless we're 100% *certain* the result is 0 rows
  //     3a: new upper bound is less than the min value allowed
  //         by this datatype
  //     3b: maxSetByPred_ is TRUE
  // ==> for both A & B, we zero-out the histogram
  //     3c: otherwise
  // ==> for this case, we create a new histogram, with one interval,
  //     from the lower limit of this datatype's values up to the new
  //     upper boundary, and give this interval 1 row/1 uec
  if ( newUpBound <  first.loBound() ||
       // see the comments below to understand the rest of this
       // logical expression
       ( newUpBound == first.loBound() &&
         (first.isLoBoundInclusive() == FALSE || boundIncluded == FALSE) ) )
    {
      // in all cases, the result is now fake
      setFakeHistogram (TRUE) ;

      // first, calculate the max upper value of this datatype
      EncodedValue datatypeMinValue (WIDE_("(>)"), columns_ ) ;

      if ( newUpBound < datatypeMinValue || isMinSetByPred() == TRUE )
        clearHistogram() ;
      else
      {
        // NB: this is NOT the same as setToSingleValue !
        // (this sets all the flags except fake hist)
        setToSingleInterval (datatypeMinValue, newUpBound, 1, 1) ;
        getModifableFrequentValues().deleteFrequentValuesAboveOrEqual (newUpBound, TRUE) ;
      }
      return ;
    }

  // CASE 4 :  if the new upper bound is equal to the current
  //           lower bound
  //      4a : isLoBoundInclusive() == TRUE  && boundIncluded == TRUE
  //           <  <  <=
  //           |  |  |    [3,7) (-inf,3]   setToSingleValue(3)
  //           3  7  9
  //      4b : isLoBoundInclusive() == TRUE  && boundIncluded == FALSE
  //           <  <  <=
  //           |  |  |    [3,7) (-inf,3)   nix entire histogram
  //           3  7  9
  //      4c : isLoBoundInclusive() == FALSE && boundIncluded == TRUE
  //           <= <  <=
  //           |  |  |    (3,7) (-inf,3]   nix entire histogram
  //           3  7  9
  //      4d : isLoBoundInclusive() == FALSE && boundIncluded == FALSE
  //           <= <  <=
  //           |  |  |    (3,7) (-inf,3)   nix entire histogram
  //           3  7  9
  if ( newUpBound == first.loBound() )
    {
      // the flags are both TRUE, since we covered the other cases above
      setToSingleValue (newUpBound, constExpr) ;
      return ;
    }

  // CASE 5: newUpBound is between the current hi/lo values of the Histogram
  //         (the usual case)

  // first, find the Interval containing this value
  // next, divide that interval into two pieces, as necessary
  // third, remove all Intervals above the bottom piece of that Interval

  // to differentiate the results between < and <=, we always
  // insert a SVI at the boundary value in the case of <= ;
  // similar to how we assume the user "knows something" when
  // he specifies equality with something that's below the histogram's
  // boundaries, we are assuming that the value associated with
  // the >= predicate has some significance.
  if ( boundIncluded )
    {
      histogram_->insertSingleValuedInterval (newUpBound) ;
      divideHistogramAlongBoundaryValue (newUpBound, ITM_LESS_EQ) ;
    }
  else
    {
      divideHistogramAlongBoundaryValue (newUpBound, ITM_LESS) ;
    }

  //
  // cleanup: how many rows & uecs remain?
  //
  const CostScalar oldTotalUec = totalUec_ ;
  setRowsAndUecFromHistogram() ;
  baseUec_ = baseUec_ / oldTotalUec * totalUec_ ;

  maxValue_     = newUpBound ;
  setMaxSetByPred (TRUE) ;

  // sanity check before we go
  last = histogram_->getLastInterval() ;
  if (last.hiBound() != newUpBound)
  {
    CCMPASSERT (last.hiBound() == newUpBound) ;
    // These should be equal, since we made sure, just in case it is not
    // set that equal, and make histogram fake.
    last.setHiBound(newUpBound);
    setFakeHistogram(TRUE);
  }
}

// -----------------------------------------------------------------------
// ColStats::setToSingleInterval
//
// A helper routine for setToSingleValue() and isNull()
// (assumes we already have a histogram we're allowed to modify)
// --> nixes the current histogram, puts in its place a 2-HistInt
//     histogram with the two parameters as the minbound/maxbound
// --> maintains the histogram semantic of having the first HistInt
//     always have 0 row/0 uec
// -----------------------------------------------------------------------
void
ColStats::setToSingleInterval (const EncodedValue & newLoBound,
                               const EncodedValue & newUpBound,
                               CostScalar numRows,
                               CostScalar numUecs)
{
  // want to be careful to keep track of the shape-changed flag
  Interval first = histogram_->getFirstInterval() ;

  if ( first.isValid()                 &&
       histogram_->numIntervals() == 1 &&
       first.loBound() == newLoBound   &&
       first.hiBound() == newUpBound   &&
       first.getRowcount() == numRows  &&
       first.getUec()      == numUecs )
    {
      // even though our values have not changed, now they're "vindicated"
      // by the application of some predicate
      setMinSetByPred (TRUE) ;
      setMaxSetByPred (TRUE) ;
      return ; // nothing more to do
    }

  histogram_->clear() ;
  histogram_->insertZeroInterval (newLoBound, newUpBound, TRUE) ;
  first = histogram_->getFirstInterval() ;
  first.setRowsAndUec (numRows, numUecs) ;

  // set the aggregate values
  setRedFactor    (1.0) ;
  setUecRedFactor (1.0) ;
  baseUec_ = numUecs ;
  setRowsAndUec   (numRows, numUecs) ;

  // set the flags
  setMinSetByPred (TRUE) ;
  setMaxSetByPred (TRUE) ;
  setShapeChanged (TRUE) ;

  minValue_ = newLoBound ;
  maxValue_ = newUpBound ;
}

void ColStats::adjustMaxSelectivity(const EncodedValue& normValue,
                                    ConstValue* constExpr,
                                    CostScalar *totalRows,
                                    CostScalar *maxSelectivity)
{
  if (totalRows == NULL || *totalRows <= csZero ||
      isVirtualColForHist() ||
      histogram_->numIntervals() == 0 ||
      getRowcount().isZero() || getTotalUec().isZero())
    return ;
  Interval first = histogram_->getFirstInterval() ;
  Interval last = histogram_->getLastInterval() ;

  EncodedValue datatypeMaxValue (L"(<)", columns_) ;
  EncodedValue datatypeMinValue (L"(>)", columns_) ;

  if (normValue < datatypeMinValue || normValue > datatypeMaxValue)
    return;

  if ( normValue < first.loBound() ||
       ( normValue == first.loBound() &&
         !first.isLoBoundInclusive() ) )
     return;

  if ( normValue > last.hiBound() ||
       ( normValue == last.hiBound() &&
         !last.isHiBoundInclusive() ) )
     return;


  // First, find the value in the most frequent value list. If it is
  // there, then use the frequency to update the maxSelectivity.
  NABoolean useHighFreq = CURRSTMT_OPTDEFAULTS->useHighFreqInfo();

  if (useHighFreq)
  {
     FrequentValueList &freqList = getModifableFrequentValues();
     CollIndex index = 0;

     FrequentValue key(normValue, constExpr, columns_[0]->getType()); 

     if ( freqList.getfrequentValueIndex(key, index) ) 
     {
     
        const FrequentValue & freqV = freqList[index];
        *maxSelectivity = MINOF(freqV.getFrequency() / (*totalRows), *maxSelectivity);
        return; 
     }
  }
   
  // second, find the Interval that contains the value 
  HistogramSharedPtr hist = this->getHistogram();

  if ( hist->numIntervals() == 0 )
    return; 

  Interval iter = hist->getFirstInterval() ;
  while ( !iter.containsValue (normValue) )
    iter.next() ;

  if ( !iter.containsValue (normValue) )
    return; // something no good

  CostScalar rows = iter.getRowcount() ;
  CostScalar uec  = iter.getUec() ;
  CollIndex iterIdx = iter.getLoIndex() ;

  // Three scenarios to consider:
  // 1. If constant is the MFV, take Rc from frequent value list and return, 
  //    the code getfrequentValueIndex(0) above computes maxSelectivity.
  // 2. If constant is not an MFV and if 2mfv exists, then take Rc as 2mfv rowcount.
  // 3. If constant is not an MFV, and 2mfv doesn't exist (for whatever reason)
  //    , compute max selectivity using "Rc of constant interval minus MFV freq"

  // compute maxSelectivity for scenario 2 now:
  if (useHighFreq && iter.getRowcount2mfv() > csZero)
      *maxSelectivity =
          MINOF(iter.getRowcount2mfv() / (*totalRows), *maxSelectivity);
  else
  {
    // compute maxSelectivity for scenario 3 now:
    // rows is for the whole interval, it contains MFV, others, so we need
    // subtract MFV rowcount.

    // get mfv information
    CostScalar mfvCnt = csZero;
    CostScalar totalMfvRc = csZero;

    if (useHighFreq)
    {
      getTotalFreqInfoForIntervalWithValue(normValue, totalMfvRc, mfvCnt);
      rows -= totalMfvRc;
      uec -=  mfvCnt;
    }

    // maxSelectivity(X=constant) ==
    // (rows in constant's histogram interval - uec + 1) / total rows

    // we do this here & now, before any interpolation occurs
    // to protect our maxSelectivity from interpolation drift
    *maxSelectivity = (rows - uec + 1) / *totalRows;
  }
}

// -----------------------------------------------------------------------
// ColStats::setToSingleValue
//
// Synthesize the effect of an equality predicate against a constant
// i.e. reduce the histogram to a single, single-valued, interval.
// -----------------------------------------------------------------------
void
ColStats::setToSingleValue (const EncodedValue & newValue, ConstValue* constExpr,
                            CostScalar *totalRows, FrequentValue* fv)
{
  getHistogramToModify() ;

  // **** temporary solution ******
  // For Transpose columns, which is formed from all constant values, such as
  // Transpose 1,2,3 as val, or for Rowset columns we shall do the things
  // in a different manner. Since we do not keep the minimum and the
  // maximum values of the constants as the interval boundary (this
  // would be very expensive, looking on the frequency it will be used)

  if (isVirtualColForHist() )
  {
    // we do not do any checks about the boundaries, just set the boundary equal
    // to the new value
    setToSingleInterval (newValue, newValue, 1, 1) ;
    setFakeHistogram (TRUE) ;
    return;
  }
  // for all cases, proceed the normal way
  //
  // in all cases, we remove any existing NULL values
  //
  removeNullInterval() ;

  //
  // if there aren't any Intervals, we're done
  //
  if ( histogram_->numIntervals() == 0 ||
       getRowcount().isZero() || getTotalUec().isZero() )
    {
      clearHistogram() ; // nix the entire thing
      return ;
    }

  //
  // first : if the newValue being set is less than the minimum allowed by
  // the datatype (or greater than the max), then nix the entire histogram
  //
  EncodedValue datatypeMaxValue (WIDE_("(<)"), columns_ ) ;
  EncodedValue datatypeMinValue (WIDE_("(>)"), columns_ ) ;
  if ( newValue < datatypeMinValue || newValue > datatypeMaxValue )
    {
      clearHistogram() ;
      frequentValues_.clear();
      return ;
    }


  Interval first = histogram_->getFirstInterval() ;
  Interval last = histogram_->getLastInterval() ;

  //
  // if the value to be set isn't inside the hi/lo bounds
  // of the histogram, remove all of 'em
  //
  // ==> UNLESS we haven't set the flags minSetByPred_/maxSetByPred_,
  //     in which case we assume the user has a clue and so we nix
  //     the entire histogram except for a single Interval containing
  //     newValue. In such a case, if the histogram is not originally
  //     fake, we set the rowcount equal to average rowcount otherwise
  //     we set the rowcount equal to 1. UEC is always set to 1
  //
  if ( newValue < first.loBound() ||
       ( newValue == first.loBound() &&
         !first.isLoBoundInclusive() ) )
    {
      if ( isMinSetByPred() == TRUE )
        {
          clearHistogram() ; // nix the entire thing,
          //                 // wipe out max/min value settings
        }
      else
        {
          if(!isOrigFakeHist())
            setToSingleInterval (newValue, newValue, (baseRowCount_/uecBeforePred_).minCsOne(), 1) ;
          else
            setToSingleInterval (newValue, newValue, 1, 1) ;

          //setToSingleInterval() method sets all the flags except fake hist
          setFakeHistogram (TRUE) ;
        }
        // remove the skew Value list from the histogram, 
        // as the value lies outside the histogram range
	if ( (!isOrigFakeHist()) )
            frequentValues_.clear();

      return ;
    }

  if ( newValue > last.hiBound() ||
       ( newValue == last.hiBound() &&
         !last.isHiBoundInclusive() ) )
    {
      if ( isMaxSetByPred() == TRUE )
        {
          clearHistogram() ;
        }
      else
        {
          if(!isOrigFakeHist())
	    setToSingleInterval (newValue, newValue, (baseRowCount_/uecBeforePred_).minCsOne(), 1) ;
	  else
	    setToSingleInterval (newValue, newValue, 1, 1) ;

          //setToSingleInterval() method sets all the flags except fake hist
          setFakeHistogram (TRUE) ;
        }
      frequentValues_.clear();
      return ;
    }

  // do the work of creating a single-valued interval
  // based on this value
  //
  FrequentValueList & frequentValueList = getModifableFrequentValues();
  NABoolean useMFVs = (((frequentValueList.entries() > 0) && CURRSTMT_OPTDEFAULTS->useHighFreqInfo())
    ? TRUE
    : FALSE);

  // get the MFV row count and number of MFVs corresponding to the interval we are interested in.
  // The retvale returns the index in teh histogram where the new interval has been added, which is
  // the parent index + 1. So subtract 1 from index to access the correct frequent value.
  
  EncodedValue mfvEV = UNINIT_ENCODEDVALUE;
  CostScalar mfvCnt = csZero;
  CostScalar totalMfvRc = csZero;

  NABoolean distributeRowsAndUec = TRUE;
  if ( useMFVs )
    distributeRowsAndUec = getTotalFreqInfoForIntervalWithValue(newValue, totalMfvRc, mfvCnt);

  CollIndex index = histogram_->insertSingleValuedInterval(newValue, distributeRowsAndUec) ;

  // need to use the MFV info for the SVI
  Interval theSVI (index, histogram_) ;

  ConstValue* tempConstExpr = NULL;
  // trim away trailing blanks to avoid bad encoding of strings with
  // trailing spaces
  if ((CmpCommon::getDefault(HIST_REMOVE_TRAILING_BLANKS) == DF_ON) &&
      constExpr && 
      (constExpr->getType()->getTypeQualifier() == NA_CHARACTER_TYPE) &&
      constExpr->valueHasTrailingBlanks())
  {
     const CharType *typ = (const CharType *)constExpr->getType();
     if (typ->getCharSet() == CharInfo::UNICODE)
     {
        Int32 bytesPerChar = (CharInfo::maxBytesPerChar)(typ->getCharSet());
        Int32 stringSize = constExpr->getStorageSize()/bytesPerChar;
        NAWString constString((NAWchar *)(constExpr->getConstValue()), stringSize);
        TrimNAWStringSpace(constString, NAString::trailing);
        tempConstExpr = new (HISTHEAP) ConstValue(constString,
                                                  typ->getCharSet(),
                                                  typ->getCollation(),
                                                  typ->getCoercibility());
     }
     else
     {
        NAString constString(constExpr->getRawText()->data());
        constString = constString.strip(NAString::trailing);
        tempConstExpr = new (HISTHEAP) ConstValue(constString,
                                                  typ->getCharSet(),
                                                  typ->getCollation(),
                                                  typ->getCoercibility());
     }
     constExpr = tempConstExpr;
  }

  if (!isOrigFakeHist())
  {
    // delete all but the given value from the frequent value list
    FrequentValue key(newValue, constExpr, columns_[0]->getType());
    frequentValueList.deleteAllButThisFreqVal(key);
  }

  // only one entry left in the frequent value list after removing all
  // that is not the newValue. Use the frequeny as the rowcount.
  index = 0;
  if ( useMFVs )
  {
    if((frequentValueList.entries() > 0 ) && 
        ( frequentValueList.getfrequentValueIndex(
                 (fv) ? (*fv) : FrequentValue(newValue, constExpr, columns_[0]->getType()),
                 index) == TRUE ) )
    {
      CostScalar rows = frequentValueList[index].getFrequency();
      theSVI.setRowsAndUec(rows, 1.0);
      setRowsAndUec(rows * rowRedFactor_, csOne  * uecRedFactor_);
    } 
    else 
    {
      // constant in the predicate is not an MFV
      // RC for the value = (rowcount of the interval - totalMfvRc)/(total Uec - mfvCnt)
      CostScalar iterUec = theSVI.getUec();
      NABoolean intervalHasOnlyFreqValues = (iterUec == mfvCnt);
      iterUec = (iterUec - mfvCnt);
      // iterUec should not be zero. That would mean that it was a single valued interval
      // whose value was also present in the frequent value list, still for some reason the
      // optimizer did not find it in the frequent value list
      // The value should also not be negative, as that would mean that we have missed 
      // out some special case, and not computed the number of frequent values matching
      // this interval correctly.
      // If either of that happens, we shall go with the intervalRC and interUec. The estimate
      // may be higher, but we should be able to avoid nested join plans
      if (iterUec < csOne && !intervalHasOnlyFreqValues)
      {
        CCMPASSERT("Number of frequent values matching the equality constant not computed correctly");
        iterUec = csOne;
      }

      CostScalar iterRC = theSVI.getRowcount();
      iterRC = (iterRC - totalMfvRc)/iterUec;
      // The same explanation as for iterUec holds for iterRC too. The iterRC should not go below
      // 1. If that happens, use the rowcount from the interval
      if (iterRC < csOne && !intervalHasOnlyFreqValues)
      {
        CCMPASSERT("Number of frequent values matching the equality constant not computed correctly");
        iterRC = (theSVI.getRowcount()/iterUec).minCsOne();
      }

      theSVI.setRowsAndUec(iterRC, iterRC.isGreaterThanZero() ? 1.0 : 0.0);
      setRowsAndUec (iterRC * rowRedFactor_,
                    (iterRC.isGreaterThanZero() ? csOne : csZero) * uecRedFactor_) ;

    }
  }
  else
  {
    setRowsAndUec (theSVI.getRowcount() * rowRedFactor_,
                   theSVI.getUec() * uecRedFactor_ ) ;
  }

  baseUec_ = totalUec_ ;
  //
  // now we want to remove all HistInts except for
  // this SVI
  //              __
  // |  |  |  |  |  |  |  |  |
  // 0  1  2  3  4  5  6  7  8
  //             i
  //
  // Want to remove 4 preceding (==index)
  //  __
  // |  |  |  |  |
  // 0  1  2  3  4
  // i
  //
  // Then, want to remove 3 later (==entries()-2)

  // NB: for improved performance, we always try to walk
  //     LIST objects from front-to-back (see Collections.cpp
  //     to see how this is a lot faster than back-to-front)

  // remove the higher, then lower, Intervals
  deleteIntervalsAbove(theSVI) ;
  deleteIntervalsBelow(theSVI) ;

  // set the min and max of the histogram 
  minValue_ = maxValue_ = newValue ;

  setMinSetByPred (TRUE) ;
  setMaxSetByPred (TRUE) ;
  if (histogram_->entries() < 2)
  {
    // we messed up somewhere. recover by clearing the histogram and 
    // inserting an interval with boundary equal to the new value
    // since we messed up somewhere, lets set the fake histogram flag to true
    CCMPASSERT (histogram_->entries() == 2) ;
    insertZeroInterval();
    setFakeHistogram(TRUE);
  }

  // check to make sure the results are what we wanted
  theSVI = Interval(0,histogram_) ;
  if(!theSVI.isSingleValued() )
  {
    // if it is not a single valued interval.
    // undo whatever we have done, insert a zero interval with
    // min and max value 
    CCMPASSERT ( theSVI.isSingleValued() ) ;
    clearHistogram();
    insertZeroInterval();
    setFakeHistogram(TRUE);
  }

  //
  // cleanup : update the aggregate information
  //

  if (tempConstExpr)
  {
     NADELETE(tempConstExpr, ConstValue, HISTHEAP);
  }
  setShapeChanged (TRUE) ;
}

// -----------------------------------------------------------------------
//  ColStats::removeSingleValue
//
// The following method is invoked to synthesize the effect of a
//    column NOT= <constant> predicate.
// Please note that the new encoded value must comprise all columns of THIS
// ColStats.  This method has the effect (in general) of adding an interval
// containing no rows to the interval containing the specified constant.
// -----------------------------------------------------------------------
void
ColStats::removeSingleValue (const EncodedValue & newValue, ConstValue* constExpr)
{
  getHistogramToModify() ;

  //
  // in all cases, we remove any existing NULL values
  //
  removeNullInterval() ;

  //
  // if there aren't any Intervals, we're done
  //
  if ( histogram_->numIntervals() == 0 ||
       getRowcount().isZero() || getTotalUec().isZero() )
    {
      clearHistogram() ;
      return ;
    }

  Interval first = histogram_->getFirstInterval() ;
  Interval last = histogram_->getLastInterval() ;

  //
  // if the value to be removed isn't inside the hi/lo bounds
  // of the histogram, do nothing
  //
  if (
       ( newValue  < first.loBound() || newValue  > last.hiBound()  )
                                     ||
       ( newValue == first.loBound() && !first.isLoBoundInclusive() )
                                     ||
       ( newValue == last.hiBound()  && !last.isHiBoundInclusive()  )
       )
    {
      return ;
    }

  //
  // Now that we've reached this point, we know that we have
  // a non-trivial case.  Handle it.
  //

  // place an SVI in the histogram, if one doesn't already exist
  // with the appropriate value

  // we cache this value for keeping track of the shape-changed flag
  CollIndex entriesBefore = histogram_->entries() ;

  CollIndex index = histogram_->insertSingleValuedInterval (newValue) ;

  Interval theSVI(index,histogram_) ;

  if (!( theSVI.isSingleValued() ))
  {
    CCMPASSERT ( theSVI.isSingleValued() ) ;
    clearHistogram();
    insertZeroInterval();
    setFakeHistogram(TRUE);
  }

  // how many rows/uecs are we removing ...?
  CostScalar rowsRemoved = rowRedFactor_ * theSVI.getRowcount() ;
  CostScalar uecsRemoved = uecRedFactor_ * theSVI.getUec() ;

  // set the s-c flag
  if ( histogram_->entries() != entriesBefore ||
       rowsRemoved.isGreaterThanZero() || uecsRemoved.isGreaterThanZero() )
    setShapeChanged (TRUE) ;

  // now remove the rows & uecs (representing the value) from the histogram
  theSVI.setRowsAndUec (0, 0) ;

  //
  // cleanup : count up the remaining rows and uecs
  //
  // NB: we do nothing with the minSetByPred_/maxSetByPred_ flags
  //     as a result of this function
  //
  // instead of adding up all of the HistInts, instead we simply
  // subtract what was found to be in the SVI

  // already applied the reduction factors above
  // A sanity check - we do not want rowsRemoved or uecsRemoved, to
  // be more than were available.
  CostScalar newRows;
  CostScalar newUecs;
  newRows = MIN_ZERO(rowcount_ - rowsRemoved);
  newUecs = MIN_ZERO(totalUec_ - uecsRemoved);

  setRowsAndUec (newRows, newUecs) ;
  baseUec_ = totalUec_ ;

  if ( (!isOrigFakeHist()) )
  {
    FrequentValueList & frequentValueList = getModifableFrequentValues();
    // remove the value from skew value list too
    FrequentValue key(newValue, constExpr, columns_[0]->getType());
    frequentValueList.deleteFrequentValue(key);
  }
}

// -----------------------------------------------------------------------
// Do the work of removing all HistInts and resetting all aggregate
// information
// -----------------------------------------------------------------------

void
ColStats::clearHistogram()
{
  if ( histogram_->entries() > 0 OR
       getRowcount().isGreaterThanZero()   OR // insurance: maybe some function (?!) which
       getTotalUec().isGreaterThanZero() )    // removed HistInts forgot to set this flag
    setShapeChanged (TRUE) ;
  setObsoleteHistogram (FALSE) ;
  setFakeHistogram  (TRUE) ; // NB: do not change "upStatsNeeded" flag
  setOrigFakeHist (TRUE) ;
  setMinSetByPred   (TRUE) ;
  setMaxSetByPred   (TRUE) ;
  histogram_->clear() ;
  setRedFactor    (0) ;
  setUecRedFactor (0) ;
  baseUec_ = 0 ;
  setRowsAndUec   (0, 0) ;
  setMinValue (UNINIT_ENCODEDVALUE) ;
  setMaxValue (UNINIT_ENCODEDVALUE) ;
  setToSingleInterval (UNINIT_ENCODEDVALUE,
                       UNINIT_ENCODEDVALUE, 0, 0) ; // avoid empty histograms!
  frequentValues_.clear();
  setIsCompressed(TRUE);
}

// -----------------------------------------------------------------------
// Synthesize the effect of
//         IS [NOT] NULL and IS [NOT] UNKNOWN
// -----------------------------------------------------------------------
void
ColStats::isNull (NABoolean notFlag)
{
  getHistogramToModify() ;

  //
  // if there aren't any Intervals, we're done
  //
  if ( histogram_->entries() == 0 ||
       getRowcount().isZero() || getTotalUec().isZero() )
    {
      clearHistogram() ;
      return ;
    }

  //
  // CASE 1 : notFlag == FALSE ; i.e., predicate == IS NULL / IS UNKNOWN
  //
  if ( notFlag == FALSE )
    {
      if ( getNullCount().isZero() ) // not any NULLs, we're probably done
        {
          // CASE 1a: zero NULLs, there should be 0, clear & finish
          if ( isMinSetByPred() == TRUE || isMaxSetByPred() == TRUE )
            {
              // yes, we're *SURE*
              clearHistogram() ;
              return ;
            }
          // no, we're not *SURE* -- so we clear out the Histogram
          // say there's 1 NULL (with 1 uec) left
          //
          // CASE 1b: zero NULLs, there should be 1
          else
            {
              // this sets all the flags except fake hist
              setToSingleInterval (NULL_ENCODEDVALUE, NULL_ENCODEDVALUE, 1, 1) ;

              setFakeHistogram (TRUE) ;
          }
        }
      else
        {
          // these are set by the subroutine below -- we don't want to
          // lose these values
          CostScalar rowRed = getRedFactor() ;
          CostScalar uecRed = getUecRedFactor() ;

          // this sets all the flags except fake hist
          setToSingleInterval (NULL_ENCODEDVALUE, NULL_ENCODEDVALUE,
                               getNullCount(), getNullUec()) ;

          setRedFactor    (rowRed) ;
          setUecRedFactor (uecRed) ;
        }
    }
  //
  // CASE 2: IS NOT NULL / IS NOT UNKNOWN
  //
  else
    {
      CostScalar numRows = getRowcount() ;
      CostScalar numUecs = getTotalUec() ;

      if ( getNullCount().isGreaterThanZero() )
        {
          numRows -= getNullCount() * rowRedFactor_ ;
          numUecs -= getNullUec()   * uecRedFactor_ ;
          setShapeChanged (TRUE) ;

          removeNullInterval() ;
        }

      if ( histogram_->numIntervals() == 0 || // are there no
           numRows.isZero() || numUecs.isZero() )     // Intervals?
        {
          clearHistogram() ;
          return ;
        }

      setRowsAndUec (numRows, numUecs) ;
      baseUec_ = numUecs ;
    }
}

// -----------------------------------------------------------------------
//  methods on StatsList class
// -----------------------------------------------------------------------

StatsList::~StatsList()
{
}

//reduce the number of histogram intervals for histograms
//referenced by the ColStats that make up this StatsList
void StatsList::reduceNumHistInts(Source invokedFrom,
                                  Criterion reductionCriterion)
{
	//iterate over all the ColStats invoking the reduction of number
	//of histogram intervals on each of the ColStats
	for(UInt32 idx=0; idx < entries(); idx++){
		if((*this)[idx])
			(*this)[idx]->reduceNumHistInts(invokedFrom, reductionCriterion);
	}
}

//reduce the number of histogram intervals for histograms
//referenced by the ColStats that make up this StatsList
void StatsList::reduceNumHistIntsAfterFetch(NATable& table)
{

  NABoolean hbasePartitioning = table.isHbaseTable() &&
         (CmpCommon::getDefault(HBASE_STATS_PARTITIONING) != DF_OFF);

  NAFileSet* nfs = table.getClusteringIndex();
  const NAColumnArray& ncas = nfs->getAllColumns();
  Lng32 leadingKeyColPos = ncas[0]->getPosition();

  //iterate over all the ColStats invoking the reduction of number
  //of histogram intervals on each of the ColStats
  const NAColumnArray& colArray = table.getNAColumnArray();
  for(UInt32 idx=0; idx < entries(); idx++)
  {
    ColStatsSharedPtr colStats = (*this)[idx];

    if ((colStats) && (colStats->statColumns().entries() == 1) && 
        (!colStats->isCompressed()) && !colStats->isSingleIntHist())
    {
      NAColumn * column = colStats->statColumns()[0];
      if (column)
      {
        //get the position of the column in the table
        short colPos =(short) column->getPosition();

        NABoolean isAKeyColumn = (column->isIndexKey() OR 
                                  column->isPrimaryKey());

        // do not reduce the #intervals for the leading primary key 
        // column of a hbase table when stats-split is possible.
        if (hbasePartitioning && isAKeyColumn && 
            colPos == leadingKeyColPos)
           continue;

        //check if this column requires full histograms
        NABoolean requiresFullHist = column->isReferencedForHistogram();

        if(requiresFullHist) 
        {
          if(CURRSTMT_OPTDEFAULTS->reduceBaseHistograms())
          {
            //if reduce num hist ints is on
            //get a reference to the full histogram's col stats
            //decide which version to use, then set statsToInsertFrom
            //to reference the stats list of the correct version.
            colStats->setAfterFetchIntReductionAttempted();
            switch (colStats->decideReductionCriterion
                    (AFTER_FETCH,CRITERION1,column,TRUE))
            {
            case CRITERION1:
              colStats->reduceNumHistInts(AFTER_FETCH, CRITERION1);
              break;
            case CRITERION2:
              colStats->reduceNumHistInts(AFTER_FETCH, CRITERION2);
              break;
            default:
              break;
            }
          }
        }
      }
    }
  }
}

void StatsList::deepDelete()
{
  unsigned short members = (UInt32)this->entries();
	for( unsigned short i=0;i<members;i++)
	{
		(*this)[i]->deepDelete();
	}
}
//------------------------------------------------------------------------
// StatsList::deepCopy()
// does a deep copy using other. This method is currently only being used
// by HistogramCache to create a copy to cache and to return to the caller
// groupUecValues_ and groupUecColumns_ do not need to be deep copied
// because FetchHistograms does not return/load these two members
//------------------------------------------------------------------------
void StatsList::deepCopy(const StatsList& other, NAMemory * heap)
{
	unsigned short members = (short)other.entries();
	for(unsigned short i=0;i<members;i++)
	{
		(*this)[i] = ColStats::deepCopy(*(other[i]),heap);
	}
	DCMPASSERT(NOT this->groupUecValues_.entries())
	DCMPASSERT(NOT this->groupUecColumns_.entries())
	DCMPASSERT(NOT this->groupMCSkewedValueLists_.entries())
}

//-------------------------------------------------------------------------
// StatsList::insertByPosition()
// Histogram that have reference to the passed column position is copied
// A set of ColStat pointers ("dupList") is used to prevent inserting
// multi-column statistics more than once.
//-------------------------------------------------------------------------
void StatsList::insertByPosition(const StatsList & other,
                                 const Lng32 position,
                                 SET(ColStats*) &dupList)
{
	for(UInt32 i = 0; i < other.entries(); i++)
	{
		ColStatsSharedPtr otherStats(other[i]);
		const NAColumnArray &otherColumns = otherStats->getStatColumns();

		// Skip to the next ColStats if these stats don't contain
		// this column position.
		if (!otherColumns.getColumnByPos(position))
			continue;

		// At this point, we don't want to add duplicate stats to
		// the StatsList.  For single-column stats, there is no problem.
		// Those are added without additional checkin.  For multi-column
		// stats, we check previous stats that have already been inserted.
		if (otherColumns.entries() == 1)
		{
			this->insertAt(this->entries(), otherStats);
		}
		else
		{
			// NASet<T>::insert() returns TRUE when an item is inserted
			// successfully, and FALSE if the item exists.  Only
			// insert the ColStats into the StatsList if it hasn't
			// already been inserted.  This is only necessary for
			// multi-column statistics.
			// Also, the dupList is short-lived so we are safe dealing
			// with the actual pointer in this list without dealing
			// with a SET of SharedPtr objects.
			if (dupList.insert(otherStats.get()))
			{
				this->insertAt(this->entries(), otherStats);
			}
		}
	}
}


// returns the UEC count from the histogram identified by the parameter
//position. Position here is the position of the column in the table
CostScalar StatsList::getSingleColumnUECCount(const Lng32 position) const
{
	//loop through all the ColStats referenced by this StatsList object
	for(UInt32 i =0;i<entries();i++)
		{
			//if the current ColStats reference has this column
			//and its NAColumnArray has one entry (which means
			//that the ColStats object represents a single column)
			//then return the current ColStats reference
			if(((*this)[i]->getStatColumns().entries()==1) &&
			   ((*this)[i]->getStatColumns().getColumnByPos(position)))
			{
				return (*this)[i]->getTotalUec();
			}
	}
	return -1;

}
//returns are reference to the ColStats object representing
//the single column statistics for the column identified by
//the parameter position
ColStatsSharedPtr StatsList::getSingleColumnColStats(const Lng32 position)
{
	//loop through all the ColStats referenced by this StatsList object
	for(UInt32 i =0;i<entries();i++)
		{
			//if the current ColStats reference has this column
			//and its NAColumnArray has one entry (which means
			//that the ColStats object represents a single column)
			//then return the current ColStats reference
			if(((*this)[i]->getStatColumns().entries()==1) &&
			   ((*this)[i]->getStatColumns().getColumnByPos(position)))
			{
				return (*this)[i];
			}
	}
	//No ColStats reference to single column statistics
	//were found, so return NULL
	return NULL;
};
//--------------------------------------------------------------------------
// StatsList::insertCompressedCopy()
// This method is a helper for caching histograms. It makes a deep copy of
// full histogram that references the column positon. Then it makes it look
// like compressed histogram by deleting the 'histogram' structure and then
// makes sure that the column is also at a proper state
//--------------------------------------------------------------------------
ColStatsSharedPtr StatsList::insertCompressedCopy(const StatsList & realStat,
										   const Lng32 position,
										   NABoolean state)
{
	for(UInt32 i=0;i<realStat.entries();i++)
	{
		NAColumnArray columns = realStat[i]->getStatColumns();
		if(columns.entries() ==1 &&
			columns.getColumn(Lng32(0))->getPosition() == position)
		{
			this->insertAt(this->entries(),ColStats::deepCopy(*realStat[i],heap_));
			ColStatsSharedPtr tempStat = (*this)[this->entries()-1];
			tempStat->setHistogram(HistogramSharedPtr(new(heap_) Histogram(heap_)));
			if(state)
			tempStat->getStatColumns().getColumn(Lng32(0))->
				setReferenced();
			else
			tempStat->getStatColumns().getColumn(Lng32(0))->
				setNotReferenced();
			break;
		}
	}
	return (*this)[this->entries()-1];
}

//---------------------------------------------------------------------------
// StatsList::insertDeepCopyList()
// Adds/inserts deep copy of the list of histograms. Method guards against
// duplication of histograms(due to its way of use, it only needs to do that
// for multi-column histogram). If one of the current single column histogram
// has a reference to a multi-column histogram passed in then we should not add
// it because the multi-column histogram was added when the single column
// histogram was added
//---------------------------------------------------------------------------
void StatsList::insertDeepCopyList(const StatsList & other)
{
	NAList<Lng32> positionList(CmpCommon::statementHeap(),other.entries());
	for(UInt32 i=0;i<other.entries();i++)
	{
		NAColumnArray colArray(CmpCommon::statementHeap());
		colArray = other[i]->getStatColumns();
		if(colArray.entries()==1){
			this->insertAt(this->entries(),ColStats::deepCopy(*(other[i]),heap_));
			positionList.insertAt(positionList.entries(),colArray.getColumn(Lng32(0))->getPosition());
		}
		else
		{
			NABoolean doCopy = TRUE;
			for(UInt32 j=0;j<this->entries();j++)
			{
				NAColumnArray statColumns = (*this)[j]->getStatColumns();
				Lng32 position = statColumns.getColumn(Lng32(0))->getPosition();
				if(statColumns.entries()==1 && NOT positionList.contains(position)
					&& colArray.getColumnByPos(position))
				{
					doCopy = FALSE;
					break;
				}
			}
			if(doCopy)
			{
				this->insertAt(this->entries(),ColStats::deepCopy(*(other[i]),heap_));
			}
		}
	}
}
//-------------------------------------------------------------------------
// Overloaded assignment opearator to make sure that the heap also does not
// get copied
//-------------------------------------------------------------------------
StatsList& StatsList::operator=(const StatsList& list)
{
	SHPTR_LIST(ColStatsSharedPtr)::operator=(list);
	this->groupUecColumns_ = list.groupUecColumns_;
	this->groupUecValues_  = list.groupUecValues_;
	this->groupMCSkewedValueLists_ = list.groupMCSkewedValueLists_;
        
	return *this;
}

void
StatsList::display() const
{
  StatsList::print() ;
}

void
StatsList::print (FILE *f, const char * prefix, const char * suffix,
                  CollHeap *c, char *buf) const
{
  Space * space = (Space *)c;
  char mybuf[1000];

  snprintf(mybuf, sizeof(mybuf), "%sStatsList : %s\n", prefix, suffix);
  PRINTIT(f, c, space, buf, mybuf);

  if (entries() != 0)
    {
      // can't simply call ColStats::print() because the ValueId's haven't
      // (might not have) been bound yet
      for (CollIndex i = 0; i < entries(); i++)
        {
          ColStatsSharedPtr iter = (*this)[i] ;

      sprintf(mybuf, "Histograms for columns: ");
      PRINTIT(f, c, space, buf, mybuf);

      iter->getStatColumns().print(f, prefix, suffix, c, buf);

      snprintf(mybuf, sizeof(mybuf), "%s   TotalUEC = %f \n", prefix, 
              iter->getTotalUec().value());
      PRINTIT(f, c, space, buf, mybuf);

      sprintf(mybuf, "%s   Rowcount = %f \n", prefix, 
              iter->getRowcount().value());
      PRINTIT(f, c, space, buf, mybuf);

      snprintf(mybuf, sizeof(mybuf), "%s   Encoded MinValue = ", prefix);
      PRINTIT(f, c, space, buf, mybuf);
      iter->getMinValue().display (f, prefix, suffix, c, buf);

      snprintf(mybuf, sizeof(mybuf), "\n%s         Encoded MaxValue = ", prefix);
      PRINTIT(f, c, space, buf, mybuf);
      iter->getMaxValue().display (f, prefix, suffix, c, buf);

      snprintf(mybuf, sizeof(mybuf), "\n%s     RowRedFactor = %f;  UecRedFactor = %f %s\n",
              prefix, iter->getRedFactor().value(), 
              iter->getUecRedFactor().value(), suffix);
      PRINTIT(f, c, space, buf, mybuf);

          // Now, display the histogram
          if (iter->getHistogram() != NULL)
        iter->getHistogram()->print(f, "   ", "", c, buf);
          else
      {
        sprintf(mybuf,"NULL histogram !\n");
        PRINTIT(f, c, space, buf, mybuf);
        }
    }
}
}

void StatsList::trace (FILE *f, NATable* table) const
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    (*this)[i]->trace(f, table);
  }
}

// return true iff all fake histograms
NABoolean StatsList::allFakeStats() const
{
  NABoolean allFake = TRUE;
  for (UInt32 i=0; i<entries() AND allFake; i++)
  {
    if (!((*this)[i])->isFakeHistogram())
      allFake = FALSE;
  }
  return allFake;
}

// return count of single column histograms (include fake histograms)
Int32 StatsList::getSingleColumnCount() const
{
  UInt32 count = 0;
  for(UInt32 i=0; i<entries();i++)
  {
    if (((*this)[i]->getStatColumns()).entries() == 1)
      count++;
  }
  return count;
}

// return count of multi-column histograms
Int32 StatsList::getMultiColumnCount() const
{
  UInt32 count = 0;
  for(UInt32 i=0; i<entries();i++)
  {
    if (((*this)[i]->getStatColumns()).entries() > 1)
      count++;
  }
  return count;
}

// construct a memory efficient representation of colArray
ColumnSet::ColumnSet(const NAColumnArray& colArray, NAMemory *heap) 
  : ClusteredBitmap(heap)
{
  for (CollIndex c = 0; c < colArray.entries(); c++) 
  {
    addElement(colArray[c]->getPosition());
  }
}

void 
ColumnSet::display() const
{
  ColumnSet::print();
}

void ColumnSet::print() const
{
  ULng32 i = 0;
  printf("{");
  for (CollIndex x=init(); next(x); advance(x) )
  {
    printf("%4d ", x);
    if (++i < entries())
    {
      printf(",");
    }
  }
  printf("}");
}

// define "<" ordering of NAColumn names
bool operator< (const NAColumn& col1, const NAColumn& col2)
{
  return col1.getColName().compareTo(col2.getColName()) < 0;
}

// print these column names in alphabetical order
void ColumnSet::printColsFromTable(FILE *ofd, NATable *table) const
{
  if (!ofd) return;

  CollIndex x;
  ULng32 i = 0, colCount = entries();
  if (!table)
  {
    for (x=init(); next(x); advance(x) )
    {
      fprintf(ofd, "%d", x);
      if ((++i < colCount) && (colCount>1))
        fprintf(ofd, ",");
    }
  }
  else 
  {
    // declare a priority_queue and specify the order as <
    priority_queue<NAColumn, vector<NAColumn>, 
      less<vector<NAColumn>::value_type> > pCols;

    // add column names
    for (x=init(); next(x); advance(x) )
    {
      pCols.push(*table->getNAColumnArray().getColumnByPos(x));
    }

    // print column names
    i = 0;
    while (!pCols.empty())
    {
      fprintf(ofd,"%s", pCols.top().getColName().data());
      if ((++i < colCount) && (colCount>1))
        fprintf(ofd,",");
      pCols.pop();
    }
  }
  fprintf(ofd," ");
}

void MultiColumnHistogram::display() const
{
  MultiColumnHistogram::print();
}

void MultiColumnHistogram::print(FILE *ofd, NATable* table) const
{
  fprintf(ofd, "histogram: ");
  columns_.printColsFromTable(ofd, table);
  Int64 templl = (Int64) uec_.value();
  fprintf(ofd, "uec:" PF64 " ", templl);
  templl = (Int64) rows_.value();
  fprintf(ofd, "rowcount:" PF64 " ", templl);
  fprintf(ofd, "intervals:2 \n");
}

MultiColumnHistogramList::~MultiColumnHistogramList()
{
  MultiColumnHistogram * multHistPtr = NULL;
  while(getFirst(multHistPtr))
  {
    if(multHistPtr) delete multHistPtr;
  }
}

// add this multi-colum histogram to this list 
// (avoid adding any duplicate multi-column histograms)

//mcStat is "fat" STMTHEAP representation of multi-column histogram.
//singleColPositions is the set of columns whose single-column histograms
//that have already been processed (ie, added to HistogramsCacheEntry).
//Assumption: a multi-column histogram is retrieved when 
//histograms for any of its columns are retrieved.
//e.g. Table T1(a int, b int, c int)
//histograms: {a},{b},{c},{a,b},{a,c},{b,c},{a,b,c}
//If histograms for column a are fetched we will get 
//histograms: {a}, {a,b}, {a,c}, {a,b,c}
//If histograms for column b are fetched we will get
//histograms: {b}, {a,b}, {b,c}, {a,b,c}
//Therefore to avoid duplicated multicolumn stats being inserted
//we pass down the list of single columns for which we have stats

void
MultiColumnHistogramList::addMultiColumnHistogram
(const ColStats & mcStat, ColumnSet * singleColPositions)
{
  if (mcStat.getStatColumns().entries() > 1)
  {
    // get columns of this multi-column histogram
    ColumnSet tempColumns(mcStat.getStatColumns(), heap_);

        
    // are this set of columns already in the list?
    if ((!singleColPositions) ||
        (!(tempColumns.intersectSet(*singleColPositions).entries())))
    {
      // get columns of this multi-column histogram
      // can't use tempColumns since intersectSet can 
      // can have a side effect
      ColumnSet columns(mcStat.getStatColumns(), heap_);
      // add multi-column histogram to list
      ComUID id(mcStat.getHistogramId());
      CostScalar uec = mcStat.getTotalUec();
      CostScalar rows = mcStat.getRowcount();

      MCSkewedValueList * mcSkewedValueList = new (heap_) MCSkewedValueList (mcStat.getMCSkewedValueList(), heap_);

      ColStatsSharedPtr mcStatsCopy = ColStats::deepCopy(mcStat, heap_); 

      MultiColumnHistogram *mcHistogram = new(heap_) 
        MultiColumnHistogram(columns, uec, rows, id, mcSkewedValueList, mcStatsCopy, heap_);

      insertAt(entries(), mcHistogram);
    }
  }
}

// add these multi-column histograms to this list.
// no checking for duplicate multi-column histograms.
// used for adding multicolumn histograms for 1st time
// in HistogramsCacheEntry::HistogramsCacheEntry() constructor.
void
MultiColumnHistogramList::addMultiColumnHistograms
(const StatsList & colStats)
//used in the process of populating this "lean" ContextHeap representation
//from the "fat" colStats representation of multi-column histograms.
{
  // how many multi-column histograms are in colStats?
  Int32 multiColumnCount = colStats.getMultiColumnCount(); 
  if (multiColumnCount > 0) 
  {
    // is this multi-column histogram already in the list?
    for(UInt32 i=0; i<colStats.entries();i++) 
    {
      addMultiColumnHistogram(*colStats[i]);
    }
  }
}

void MultiColumnHistogramList::display() const
{
  MultiColumnHistogramList::print();
}

void MultiColumnHistogramList::print (FILE *ofd, NATable* table) const
{
  for (CollIndex x=0; x<entries(); x++)
  {
    at(x)->print(ofd, table);
  }
}

//reduce the number of histogram intervals in the histogram
//referenced by this ColStats Object
void ColStats::compressColStatsForQueryPreds(ItemExpr * lowerBound,
                                             ItemExpr * upperBound,
                                             NABoolean  hasJoinPred)
{
  //if there is no histogram return
  if(!histogram_)
    return;

  //dont do anything for fake histograms
  if(isFakeHistogram())
    return;

  //multicolumn stats, dont reduce
  if(columns_.entries() > 1)
    return;

  //if there are only two histints or less
  //we dont need to reduce
  if(histogram_->entries() <= 2)
    return;

  //reduce the number of histogram intervals
  histogram_->compressHistogramForQueryPreds(lowerBound, upperBound, hasJoinPred);
}

// ----------------------------------------------------------------------------
// Method to reduce the number of histogram intervals based on range predicates
// example predicates
// * t1.col1 < 3
// * t1.col1 > 1
// * t1.col1 > 1 and t1.col1 < 3
// ----------------------------------------------------------------------------
void Histogram::compressHistogramForQueryPreds(ItemExpr * lowerBound,
                                               ItemExpr * upperBound,
                                               NABoolean  hasJoinPred)
{
  // don't compress if less than 4 intervals
  if (numIntervals() < 4)
    return;

  // should the histogram be compressed to a single interval
  NABoolean compressToSingleInterval = FALSE;

  // Validate Parameters - Begin

  // Get lowest and the highest values in this histogram
  // This is used for checking if a given value false within
  // a histogram's boundary or outside of it.
  EncodedValue minEncodedValue = getFirstInterval().loBound();
  EncodedValue maxEncodedValue = getLastNonNullInterval().hiBound();

  // EncodedValues for the upper and lower bounds passed in
  EncodedValue * lowerBoundEncodedValue = NULL;
  EncodedValue * upperBoundEncodedValue = NULL;

  // if a lower bound was passed in
  if (lowerBound)
  {
    // create an EncodedValue to represent the lower bound
    lowerBoundEncodedValue = new (CmpCommon::statementHeap())
                              EncodedValue(lowerBound, FALSE);
  }
  else{
    // a lower bound was not passed in

    // create an EncodedValue to represent the lower bound
    lowerBoundEncodedValue = new (CmpCommon::statementHeap())
                              EncodedValue(minEncodedValue);

  }

  // if a upper bound was passed in
  if (upperBound)
  {
    // create an EncodedValue to represent the upper bound
    upperBoundEncodedValue = new (CmpCommon::statementHeap())
                              EncodedValue(upperBound, FALSE);
  }
  else{
    // a upper bound was not passed in

    // create an EncodedValue to represent the upper bound
    upperBoundEncodedValue = new (CmpCommon::statementHeap())
                              EncodedValue(maxEncodedValue);
  }

  // if lowerBound is higher than upperBound
  // e.g. a > 3 and a < 2
  if ((*lowerBoundEncodedValue) > (*upperBoundEncodedValue))
    compressToSingleInterval = TRUE;

  if (lowerBound)
  {
    // if the lower bound is smaller than the smallest value
    // in the histogram
    if ((*lowerBoundEncodedValue) < minEncodedValue)
    {
      (*lowerBoundEncodedValue) = minEncodedValue;
    }

    // if the lower bound is larger than the largest value
    if ((*lowerBoundEncodedValue) > maxEncodedValue)
    {
      compressToSingleInterval = TRUE;
    }
  }

  if (upperBound)
  {
    // if the upper bound is larger than the largest value
    // in the histogram
    if ((*upperBoundEncodedValue) > maxEncodedValue)
    {
      (*upperBoundEncodedValue) = maxEncodedValue;
    }

    // if the upper bound is smaller than the smallest value
    if ((*upperBoundEncodedValue) < minEncodedValue)
    {
      compressToSingleInterval = TRUE;
    }
  }
  // Validate Parameters - End

  // keep in mind by this point in the code
  // if compressToSingleInterval != FALSE then it is
  // guaranteed that:
  // lowerBoundEncodedValue <= upperBoundEncodedValue]

  // Another important thing to keep in mind is that
  // there should be a lower and upper bound by this
  // point in the code.
  // If the upper bound is not passed in we set the upper
  // bound to be the highest value in the histogram.
  // If the lower bound is not passed in we set the lower
  // bound to the the lowest value in the histogram.

  Int32 state = 0; // 0 = looking for lower bound
                 // 1 = looking for upper bound
                 // 2 = found both lower and upper bounds

  if (compressToSingleInterval)
    state = 2;

  //interval object used to iterate over histogram intervals
  Interval iter = getFirstInterval();

  // get a handle to the next interval
  Interval next = getNextInterval (iter);

  if ((state != 2) &&
      (iter.containsValue(*lowerBoundEncodedValue)))
  {
    // we found the lower bound in the very first interval

    state = 1; // i.e. between

    // mext interval is the last, return
    if (next.isLast()) return;


    if (iter.containsValue(*upperBoundEncodedValue))
    {
      // we also found the upper bound in the very first interval

      // this means both the lower and the upper bound are in the
      // first interval
      state = 2; // i.e. after

      // skip the first interval
      iter.next();
    }
    else if (next.containsValue(*upperBoundEncodedValue))
    {
      // found the upper bound in the second interval

      // this means the first interval has the lower bound
      // and the second interval has the upper bound
      state = 2; // i.e. after

      // skip the first and the second intervals
      iter.next();
      iter.next();
    }
    else{
      // the lower bound is in the first interval
      // but the upper bound is not in the first
      // or the second interval

      // skip the first interval
      iter.next();
    }
  }

  //iterate over the intervals of this histogram
  for ( /* initialized above */ ;
        iter.isValid() && !iter.isNull();
        /* no automatic increment */)
  {
    // if this is the last interval, then break out and return
    if ( iter.isLast() ) break;

    // at this point, we know another interval exists
    Interval next = getNextInterval (iter) ;

    // null interval, i.e. interval that
    // contains stats for null values is last
    if ( next.isNull() ) break; // do not merge NULL intervals!

    // if we have found both the lower and the upper bounds
    if (state == 2)
    {
      // compress i.e. merge the next
      // interval into the current interval
      if (!iter.merge(next))
        iter.next();
      continue;
    }

    // if we are looking for the upper bound
    if (state == 1)
    {
      // check if next interval contains the upper bound
      if (next.containsValue(*upperBoundEncodedValue))
      {
        // next interval does contain the upper bound

        // set state to indicate we found both lower
        // and upper bounds
        state = 2;

        // if next interval is the last interval break and return
        if (next.isLast()) break;

        // skip next interval
        iter.next();
        iter.next();

      }
      else
      {
        // next interval does not contain the upper bound

        // if this column has a join predicate
        // then don't compress intervals
        // that fall between the lower and
        // the upper bounds
        if (hasJoinPred)
        {
          iter.next();
        }
        else{
          // compress i.e. merge the next
          // interval into the current interval
          if (!iter.merge(next))
            iter.next();
        }
      }
    }

    // if we are looking for the lower bound
    if (state == 0)
    {

      // check if the next interval contains the lower bound
      if (next.containsValue(*lowerBoundEncodedValue))
      {
        // next interval does contain the lower bound
        // therefore we need to skip over it

        // if next interval is the last interval break
        if (next.isLast()) break;

        // set state to indicate that now we are looking
        // for the upper bound
        state = 1;

        // check if the next interval also contains the
        // upper bound
        if (next.containsValue(*upperBoundEncodedValue))
        {
          // the next interval does contain the upper bound
          // therefore set state to indicate we have found
          // both the lower and the upper bounds
          state = 2;

          // since the next interval contains both the bounds
          // don't merge it into the current interval (i.e. variable
          // iter), rather skip over the next interval
          iter.next();
          iter.next();
        }
        else{
          // the next interval does not contain the upper bound

          // check the interval adjacent to the next interval
          iter.next();
          next = getNextInterval(iter);

          // if next interval is the last interval break and return
          if (next.isLast()) break;

          // if next interval contains the upper bound
          if (next.containsValue(*upperBoundEncodedValue))
          {
            state = 2;

            // skip over the next interval
            iter.next();
            iter.next();
          }
          else{
            // iterate to the next interval
            iter.next();
          }

        }
      }
      else
      {
        // next interval does not contain the lower bound
        // compress i.e. merge the next
        // interval into the current interval
        if (!iter.merge(next))
          // somthing went wrong during merge, skip to next interval
          iter.next();
      }
    }
  }
}

// -----------------------------------------------------------------------
// Method to calculate the selectivity for an equality predicate 
// example t1.col1 = 2
// 
// Algorithm:
// 1) Determine the interval which contains the literal.
// 2) Selectivity is equal to the rows of the interval / UEC of the interval.
// 3) the selectivity is equal to total row count / total UEC when Equality 
//    is with a host var or a constant expression or if the histogram is fake.
//
// Input:
//    constVal - an item expression representing a constant literal or 
//               a host var
//    totalRowcount - total rowcount of this histogram (from ColStats)
//    totalUEC  - total UEC of this histogram (from ColStats)
//  
// Output:
//    selectivity: - the computed selectivity when TRUE is returned
//                   undefined otherwise
//
// Return: TRUE - if the selectivity is computable from the histogram
//         FALSE - otherwise
// -----------------------------------------------------------------------
NABoolean 
Histogram::computeSelectivityForEquality(
                 ItemExpr * constVal, 
                 CostScalar totalRowcount, CostScalar totalUEC,
                 CostScalar& selectivity)
{
  // create a EncodedValue from the constVal
  const EncodedValue encodedConstVal(constVal, FALSE);

  Interval last = getLastInterval();

  // handle NULL case first
  if ( encodedConstVal.isNullValue() == TRUE ) {
      if ( last.isNull() ) {
        selectivity = last.getRowcount() / last.getUec();
        return TRUE;
      } else
        return FALSE;
  } 

  // handle host var next
  if ( constVal->getOperatorType() == ITM_HOSTVAR )
  {
    selectivity = totalRowcount / totalUEC;
    return TRUE;
  }

  if ( constVal->getOperatorType() != ITM_CONSTANT )
    return FALSE;

  // handle constant case last by iterating over the intervals of 
  // this histogram
  for ( Interval iter = getFirstInterval(); ; iter.next())
  {
    if ( !iter.isValid() || iter.isNull() ) {
      if ( iter == last ) 
         break;
      else
         continue;
    }

    // check if next interval contains constVal
    if ( iter.containsValue(encodedConstVal) ) {
       selectivity = iter.getRowcount() / iter.getUec();
       return TRUE;
    }

    if ( iter == last ) 
      break;
  }

  // neither the NULL constant nor in any intervals
  // return total rowcount / total uec
  selectivity = totalRowcount / totalUEC;
  return TRUE;
}


void SkewedValueList::insertInOrder(const EncodedValue& skewed)
{
   CollIndex i;
   for (i=0; i<entries(); i++) {
      const EncodedValue& x = (*this)[i];
      if ( x == skewed )
         return;
      else
      if ( skewed > x ) {
         break;
      }
   }
   insertAt(i, skewed);
}

const NAString SkewedValueList::getText() const
{
   NAString result("[");

   const NAType* naType = getNAType(); 
   if ( !needToComputeFinalHash() ) {
     // TRUE MCSB case. All hash values are computed. Each skew is 
     // represented by a dot character.
     for (CollIndex i=0; i<entries(); i++) 
       result += ".";
   } else
   if ( naType->useHashRepresentation() == FALSE ) 
   {
     for (CollIndex i=0; i<entries()-1; i++) {
        result += (*this)[i].getText(FALSE, /* no surrounding parenthesis */
                                     FALSE  /* no fractional part */
                                    )  + ", ";
     }
     result += (*this)[entries()-1].getText(FALSE, FALSE);
   } else {

     for (CollIndex i=0; i<entries(); i++) {
        if ( (*this)[i].getValue().isNull() == FALSE ) {
           result += ".";
        } else 
           result += (*this)[i].getText(FALSE, /* no surrounding parenthesis */
                                        FALSE  /* no fractional part */
                                        ) ;
     }
   }
   result += "]";
   return result;
}

MCSkewedValue & MCSkewedValue::operator= (const MCSkewedValue& other)
{
  if (this != &other)
  {
    NAWchar * boundaryVal =  new(heap_) NAWchar[na_wcslen(other.boundary_)+ 1];
    na_wcscpy(boundaryVal, (NAWchar*)other.boundary_);
    boundary_ = boundaryVal;
    frequency_ = other.frequency_;
    hash_ = other.hash_;
    mcEncodedValue_ = new (heap_) EncodedValue(*(((MCSkewedValue &)other).mcEncodedValue_), ((MCSkewedValue &)other).heap_);
  }
  return *this;
}

void MCSkewedValue::print (FILE *f, const char * prefix,
	    const char * suffix, CollHeap *c, char *buf) const
{
  Space * space = (Space *)c;
  char mybuf[1000];

  snprintf(mybuf, sizeof(mybuf), "%sMCSkewedValue : %s\n", prefix, suffix);
  PRINTIT(f, c, space, buf, mybuf);
  snprintf(mybuf, sizeof(mybuf), "%sBoundary : %s", prefix, suffix);
  PRINTIT(f, c, space, buf, mybuf);

  Lng32 wlen = na_wcslen(boundary_) + 10;
  char* wbuf = new (heap_) char[wlen * 2];
  na_wsprintf((wchar_t *)wbuf, WIDE_("%s"), boundary_);

  //swprintf((wchar_t *)mybuf, na_wcslen(boundary_), boundary_);

  PRINTIT(f, c, space, buf, wbuf);

  snprintf(mybuf, sizeof(mybuf), "%sEncodedValue = ", prefix);
  PRINTIT(f, c, space, buf, mybuf);
  mcEncodedValue_->display (f, DEFAULT_INDENT, "", c, buf);
  snprintf(mybuf, sizeof(mybuf), "%sFrequency : %f\n", prefix, frequency_.value());
  PRINTIT(f, c, space, buf, mybuf);
}

void MCSkewedValue::display() const
{
  MCSkewedValue::print();
}

void MCSkewedValueList::print (FILE *f, const char * prefix,
	    const char * suffix, CollHeap *c, char *buf) const
{
  Space * space = (Space *)c;
  char mybuf[1000];

  snprintf(mybuf, sizeof(mybuf), "%sMCSkewedValueList : %s\n", prefix, suffix);
  PRINTIT(f, c, space, buf, mybuf);

  if(entries() == 0)
  {
    sprintf(mybuf,"Empty MCSkewedValueList !\n");
    PRINTIT(f, c, space, buf, mybuf);
  }

  for (CollIndex i = 0; i < entries(); i++)
    at(i)->print();
}

void MCSkewedValueList::display() const
{
  MCSkewedValueList::print();
}

MCSkewedValueList::MCSkewedValueList(const MCSkewedValueList & mcsvl, NAMemory *h)
  :NAList<MCSkewedValue *>(h ? h : CmpCommon::statementHeap()),
   heap_(h ? h : CmpCommon::statementHeap())
{
    for (CollIndex i = 0; i < mcsvl.entries(); i++)
    {
      MCSkewedValue * otherMCSV = mcsvl.at(i);
      addMCSkewedValue(otherMCSV);
    }
}

MCSkewedValueList & MCSkewedValueList::operator= (const MCSkewedValueList& other)
{
  if (this != &other)
    LIST(MCSkewedValue *)::operator= (other);
  return *this;
}

// NAHashDictionary class requires the following operator to be defined.
NABoolean MCSkewedValueList::operator==(const MCSkewedValueList& mcsvl)
{
  if (entries() != mcsvl.entries())
    return FALSE;
  else
  {
    for (CollIndex i = 0; i < entries(); i++)
    {
      MCSkewedValue *thisMCSV = at(i);
      MCSkewedValue *otherMCSV = mcsvl.at(i);

      if(!(*thisMCSV == *otherMCSV))
        return FALSE;
    }
  }
  return TRUE;
}

void MCSkewedValueList::mergeMCSkewedValueList(MCSkewedValueList * leftSide, 
                                               MCSkewedValueList * rightSide,
                                               CostScalar avgRowcountForNonSkewValuesOnLeftSide,
                                               CostScalar avgRowcountForNonSkewValuesOnRightSide,
                                               MergeType mergeMethod)
{
  NAWchar * newBound = NULL;
  CostScalar newFreq;
  EncodedValue * newEV = NULL;

  CollIndex leftIndex = 0;
  CollIndex rightIndex = 0;
  
  CollIndex leftSideEntries = 0;
  if(leftSide)
    leftSideEntries = leftSide->entries();
  CollIndex rightSideEntries = 0;
  if(rightSide)
    rightSideEntries = rightSide->entries();

  while ( leftIndex < leftSideEntries ||
          rightIndex < rightSideEntries ) 
  {
    if((leftIndex < leftSideEntries) &&
       (rightIndex < rightSideEntries))
    {
      MCSkewedValue * leftV = leftSide->at(leftIndex);
      MCSkewedValue * rightV = rightSide->at(rightIndex);

      CostScalar leftFreq = leftV->getFrequency();
      CostScalar rightFreq = rightV->getFrequency();

      if ( *leftV == *rightV ) 
      { 
        if(mergeMethod == INNER_JOIN_MERGE || mergeMethod == OUTER_JOIN_MERGE)
          newFreq = leftFreq * rightFreq;
        else if(mergeMethod == SEMI_JOIN_MERGE)
          newFreq = leftFreq;
        else if(mergeMethod == ANTI_SEMI_JOIN_MERGE)
          newFreq = 0;
        else if(mergeMethod == UNION_MERGE)
          newFreq = leftFreq + rightFreq;
        else if(mergeMethod == OR_MERGE)
          newFreq = MAXOF(leftFreq, rightFreq);
        else if(mergeMethod == AND_MERGE)
          newFreq = MINOF(leftFreq, rightFreq);

        newBound = (NAWchar * )leftV->getBoundary();
        newEV = (EncodedValue *)leftV->getEncodedValue();

        leftIndex++;
        rightIndex++;
      }
      else if ( *leftV < *rightV )
      {
        if(mergeMethod == INNER_JOIN_MERGE || mergeMethod == OUTER_JOIN_MERGE)
        {
          newBound = (NAWchar * )leftV->getBoundary();
          newFreq = leftV->getFrequency() * avgRowcountForNonSkewValuesOnRightSide;
          newEV = (EncodedValue *)leftV->getEncodedValue();
        }
        leftIndex++;
      }
      else
      {
        if(mergeMethod == INNER_JOIN_MERGE || mergeMethod == OUTER_JOIN_MERGE)
        {
          newBound = (NAWchar * )rightV->getBoundary();
          newFreq = rightV->getFrequency() * avgRowcountForNonSkewValuesOnLeftSide;
          newEV = (EncodedValue *)rightV->getEncodedValue();
        }
        rightIndex++;
      }
    }
    else if((leftIndex < leftSideEntries) &&
            (rightIndex == rightSideEntries))
    {
      if(mergeMethod == INNER_JOIN_MERGE || mergeMethod == OUTER_JOIN_MERGE)
      {
        MCSkewedValue * leftV = leftSide->at(leftIndex);
        newBound = (NAWchar * )leftV->getBoundary();
        newFreq = leftV->getFrequency() * avgRowcountForNonSkewValuesOnRightSide;
        newEV = (EncodedValue *)leftV->getEncodedValue();
      }
      leftIndex++;
    }
    else if((leftIndex == leftSideEntries) &&
            (rightIndex < rightSideEntries))
    {
      if(mergeMethod == INNER_JOIN_MERGE || mergeMethod == OUTER_JOIN_MERGE)
      {
        MCSkewedValue * rightV = rightSide->at(rightIndex);
        newBound = (NAWchar * )rightV->getBoundary();
        newFreq = rightV->getFrequency() * avgRowcountForNonSkewValuesOnLeftSide;
        newEV = (EncodedValue *)rightV->getEncodedValue();
      }
      rightIndex++;
    }

    if(newBound)
    {
      newFreq = newFreq.minCsOne();
      MCSkewedValue *newV = new (STMTHEAP) MCSkewedValue(newBound,
                                                         newFreq,
                                                         newEV,
                                                         0,
                                                         STMTHEAP);
      addMCSkewedValue(newV);
      newBound = NULL;
    }
  }
}
void MCSkewedValueList::addMCSkewedValue(MCSkewedValue * newValue)
{
  addMCSkewedValue(newValue->getBoundary(),
                   newValue->getFrequency(),
                   *(newValue->getEncodedValue()),
                   newValue->getHash());
}

void MCSkewedValueList::addMCSkewedValue(const NAWchar * boundary, 
                                         CostScalar frequency,
                                         const EncodedValue & eV,
                               			 UInt32 hash)
{
  NAWchar * boundaryVal =  new(heap_) NAWchar[na_wcslen(boundary)+ 1];
  na_wcscpy(boundaryVal, (NAWchar*)boundary);
  EncodedValue * encodedVal = new (heap_) EncodedValue (eV, heap_);
  MCSkewedValue *mcSkewedValue = new(heap_) MCSkewedValue(boundaryVal, frequency, encodedVal, hash, heap_);
  insert(mcSkewedValue);
}

void ColStats::addMCSkewedValue(const NAWchar * boundary, CostScalar frequency)
{
  const  NAColumnArray colArray = getStatColumns();
  ConstValue** cvPtrs = new STMTHEAP ConstValuePtrT[colArray.entries()];
  EncodedValue eV = EncodedValue (boundary, colArray, cvPtrs);
  UInt32 hash = eV.computeRunTimeHashValue(colArray, boundary, cvPtrs);
  mcSkewedValueList_.addMCSkewedValue(boundary, frequency, eV, hash);
  NADELETEBASIC(cvPtrs, STMTHEAP);
}

// to be called from the debugger
void
FrequentValueList::display() const
{
  FrequentValueList::print();
}

void
FrequentValueList::print (FILE *f, const char * prefix, const char * suffix,
                          CollHeap *c, char *buf) const
{
  Space * space = (Space *)c;
  char mybuf[1000];
  snprintf(mybuf, sizeof(mybuf), "%sFrequent Values : %s\n", prefix, suffix);
  PRINTIT(f, c, space, buf, mybuf);
  if (entries() != 0)
    {
      for (CollIndex i = 0; i < entries(); i++)
      (*this)[i].print(f, "     ","", c, buf);
    }
}

void FrequentValue::print (FILE *f,
	      const char * prefix,
	      const char * suffix,
                           CollHeap *c, char *buf) const
{
  Space * space = (Space *)c;
  char mybuf[1000];
  snprintf(mybuf, sizeof(mybuf), "%sHash Val = %u ", prefix, getHash());
  PRINTIT(f, c, space, buf, mybuf);
  snprintf(mybuf, sizeof(mybuf), "%s  Encoded Val = ", prefix);
  PRINTIT(f, c, space, buf, mybuf);
  getEncodedValue().display (f, DEFAULT_INDENT, "", c, buf);
  snprintf(mybuf, sizeof(mybuf), "%s  Freq. = %f ", prefix, getFrequency().value());
  PRINTIT(f, c, space, buf, mybuf);
  snprintf(mybuf, sizeof(mybuf), "%s  Probab. = %f \n", prefix, getProbability().value());
  PRINTIT(f, c, space, buf, mybuf);
}

FrequentValue::FrequentValue(UInt32 hashValue, 
                             CostScalar frequency, 
                             CostScalar probability,
                             EncodedValue value) 
{
  hash_ = hashValue;
  frequency_ = frequency;
  probability_ = probability;
  encodedValue_ = value;
}

FrequentValue::FrequentValue(const EncodedValue& normValue,
                             ConstValue* cv,
                             const NAType* colType,
                             CostScalar freq, CostScalar prob)
: hash_(0), frequency_(freq), probability_(prob), encodedValue_(normValue) 
{
   if ( normValue.isNullValue() )
     hash_ = 666654765;
   else {
      if ( cv && 
           colType->useHashRepresentation()&&
           colType->useHashInFrequentValue() 
         )
      {
        //const NAType* colType = columns[0]->getType();

        if ((colType->getTypeQualifier() == NA_CHARACTER_TYPE) && 
          ((CharType*)colType)->isCaseinsensitive() && 
          (((CharType*)colType)->getCharSet() != CharInfo::UNICODE))
          cv = cv->toUpper(HISTHEAP);

         hash_ = cv->computeHashValue(*colType);
      }
   } 
}


void 
ColStats::createAndAddSkewedValue(const wchar_t *boundary, Interval &iter)
{
  HistogramSharedPtr hist = this->getHistogram();
  if ( (hist == NULL) || (hist->numIntervals() == 0))
    return;

  // Set the threshold to the MIN of the average rowcount per
  // unique value, and COMP_INT_44 (default to 1 million).
  double int44 = (ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_INT_44);

  CostScalar thresholdFreq = MINOF((getRowcount() / getTotalUec()) * 2, int44);
  if (iter.containsAFrequentValue(thresholdFreq))
  {
    return createAndAddFrequentValue(boundary, iter);
  }
}

void ColStats::createAndAddFrequentValue(const wchar_t *boundary, Interval &iter)
{
  HistogramSharedPtr hist = this->getHistogram();
  if ( (hist == NULL) || (hist->numIntervals() == 0))
    return;

  FrequentValueList & frequentValueList = getModifableFrequentValues();

  if (frequentValueList.isFull())
      return;

  // get the columns for this histogram
  const NAColumnArray &columns = this->getStatColumns();

  // add the hash value to the frequent value list

  // collect NULL values too for skew
  CostScalar frequency;
  if (iter.isNull() ) 
  {
    UInt32 hash = 666654765;  // hash value for NULL as used by the executor in exp_functions.cpp
    EncodedValue boundaryEV; boundaryEV.setValueToNull();
    frequency = iter.getRowcount();

    FrequentValue newV(hash, frequency, csOne, boundaryEV);
    frequentValueList.insertFrequentValue(newV);
  }
  else 
  {
    frequency = iter.getRowcount() / iter.getUec();

    ConstValue** cvPtrs = new STMTHEAP ConstValuePtrT[columns.entries()];
    EncodedValue ev(boundary, columns, cvPtrs);
    FrequentValue newV(ev, cvPtrs[0], columns[0]->getType(), frequency);

    // the probability of the frequent value is one when it is added to the list
    frequentValueList.insertFrequentValue(newV);

    NADELETEBASIC(cvPtrs, STMTHEAP);
  }

}

NABoolean FrequentValueList::isFull()
{
  const ULng32 maxSkewValues = CURRSTMT_OPTDEFAULTS->maxSkewValuesDetected();

  if (entries() > maxSkewValues)
    return TRUE;
  else
    return FALSE;
}

NABoolean ColStats::mergeFrequentValues(ColStatsSharedPtr& otherStats, NABoolean scaleFreq,
                                        MergeType mergeMethod, NABoolean adjRowCount)
{
  NABoolean isRCAdjusted = FALSE;
  FrequentValueList & leftFrequentValueList = getModifableFrequentValues();
  FrequentValueList & rightFrequentValueList = otherStats->getModifableFrequentValues();

  if (CmpCommon::getDefault(COMP_BOOL_42) == DF_OFF)
  {
    CostScalar leftAverageFreq = getRowcount()/getTotalUec();
    CostScalar rightAverageFreq = otherStats->getRowcount()/otherStats->getTotalUec();

    CostScalar scaleFactor = (getScaleFactor()*otherStats->getScaleFactor()).minCsOne();
    if (scaleFreq)
    {
      leftAverageFreq = (leftAverageFreq / scaleFactor);
      rightAverageFreq = (rightAverageFreq / scaleFactor);
    }

    CollIndex i;

    if (leftFrequentValueList.entries() != 0)
      leftFrequentValueList.scaleFreqAndProbOfFrequentValues(rightAverageFreq, csOne);

    for (i = 0; i < rightFrequentValueList.entries(); i++)
    {
      const FrequentValue rightFrequentValue = rightFrequentValueList[i];

      CostScalar newFreq = (rightFrequentValue.getFrequency()) * leftAverageFreq;
      FrequentValue newV(rightFrequentValue);
      newV.setFrequency(newFreq);
      newV.setProbability(csOne);

      leftFrequentValueList.insertFrequentValue(newV);
    }  
  }
  else
  {
    // use the new merging method

	// first merge the frequent frequent values into one list
    FrequentValueList * resultFreqValList = new (STMTHEAP) FrequentValueList(STMTHEAP);
    // temporarily save matched frequent values for later use in this method
    FrequentValueList * tmpLeftFreqValList = new (STMTHEAP) FrequentValueList(STMTHEAP);
    FrequentValueList * tmpRightFreqValList = new (STMTHEAP) FrequentValueList(STMTHEAP);

  	// collect some basic information from both sides
    // Total rowcount from side 1 
    double RT1 = getRowcount().getValue();
    // Total rowcount from side 2 
    double RT2 = otherStats->getRowcount().getValue();
    if (scaleFreq)
    {
      RT1 = RT1 / getScaleFactor().getValue();
      RT2 = RT2 / otherStats->getScaleFactor().getValue();
    }

    double UT1 = getTotalUec().getValue();
    // Total UEC from side 2 
    double UT2 = otherStats->getTotalUec().getValue();

    // get the continuum values. In the absence of frequent values
    // these will be same as total values so that is where we start from
    double RC1 = RT1;
    double RC2 = RT2;
    double UC1 = UT1;
    double UC2 = UT2;

    // get the count of frequent values from both lists
    double UF1 = leftFrequentValueList.entries();
    double UF2 = rightFrequentValueList.entries();

   // if there are no frequent values then there is nothing to do
    if ( (UF1 > 0) || (UF2 > 0))
    {
      CostScalar scaleFactor = csOne;
      
      if (scaleFreq)
        scaleFactor = (getScaleFactor()*otherStats->getScaleFactor()).minCsOne();

      // Total probability of frequent values for side1 and side2
      // Probability of a frequent value changes as the histograms
      // are scaled. Lets say we starts with 100 rows of a frequent
      // value. The probability of that value is 1. Now lets say the
      // histogram is reduced is by 200, such that rowcount or the frequency
      // of that value becomes 0.5. This would reduce the probability
      // of that frequent value to 0.5 too. Now lets say the histogram is
      // scaled up by a factor of 100, taking the row count or the frequency
      // to 50, the probability of this value will continue to be 0.5
      double UP1 = leftFrequentValueList.getTotalProbability().getValue();
      double UP2 = rightFrequentValueList.getTotalProbability().getValue();

      // Total rowcount of remaining frequent values for side1 
      double RF1 = leftFrequentValueList.getTotalFrequency().getValue();
      // Total rowcount of remaining frequent values for side 2 
      double RF2 = rightFrequentValueList.getTotalFrequency().getValue();

      // The histograms and subsequently were scaled up as a result of cross product
      // done before doing the join. So scale them now to reflect the actual rowcounts
      if (scaleFreq)
      {
        RF1 = RF1 / getScaleFactor().getValue();
        RF2 = RF2 / otherStats->getScaleFactor().getValue();
      }

      // Get the continuum values by subtracting the frequent values from total
      RC1 -= RF1;
      RC2 -= RF2;
      UC1 -= UP1;
      UC2 -= UP2;

      // merge frequent value from side one with that of side 2
      // add to the resultFreqValeList those frequent values which appear on both the 
      // sides. As a side effect of this, leftFrequentValueList and rightFrequentValueList
      // get modifed. They now contain remaining values that did not match the frequent
      // values of the other side.
      // Resultant frequency = left Freq * right frequency / scale by which these two histograms
      // have been merged.

      resultFreqValList->mergeFreqFreqValues(leftFrequentValueList, rightFrequentValueList, scaleFactor,
                                             mergeMethod, tmpLeftFreqValList, tmpRightFreqValList);

      // update the frequent value counts with the remaining frequent values, as these
      // will be joined to the continuum values from the other side
      UF1 = leftFrequentValueList.entries();
      UF2 = rightFrequentValueList.entries();

      UP1 = leftFrequentValueList.getTotalProbability().getValue();
      UP2 = rightFrequentValueList.getTotalProbability().getValue();

      // Total rowcount of remaining frequent values for side1 
      RF1 = leftFrequentValueList.getTotalFrequency().getValue();
      // Total rowcount of remaining frequent values for side 2 
      RF2 = rightFrequentValueList.getTotalFrequency().getValue();

      // The histograms and subsequently were scaled up as a result of cross product
      // done before doing the join. So scale them now to reflect the actual rowcounts
      if (scaleFreq)
      {
        RF1 = RF1 / getScaleFactor().getValue();
        RF2 = RF2 / otherStats->getScaleFactor().getValue();
      }

      // Determine how many frequent values from one side would match to the continuum values
      // from the other side. It should be a minimum of the number of frequent value from
      // this side
      double US2 = MINOF(UF1, UC2 * (UP1/UT1));
      // Matching values between frequent values and continuum values
      // For side 2
      double US1 = MINOF(UF2, UC1 * (UP2/UT2));

      // save probability adjustment for frequent values too that
      // do not exist on the other side
      double adjProb1 = 1;
      double adjProb2 = 1;

      if (UF1 > 0) 
        adjProb1 = US2/UF1;
      if (UF2 > 0)
        adjProb2 = US1/UF2;
      
      // Remaining rowcounts for both sides, after having adjusted the
      // frequencies stolen by the other sides. These will need to be scaled down too
      // as these reflect the cross product. If all values from the histogram have been 
      // moved to frequent values, then continuum values would be zero. No need
      // to do any adjustment then
      if (UF1 > 0)
      {
        // Adjust the frequency side 1 with the average frequency of
        // side 2 multiplied by the values from the other side
        // that would match with each value of this side
        CostScalar adjFreq1 = csZero;
        if ( (RC2 > 0) && (UC2 > 0))
        {
          adjFreq1 = (RC2 / UC2);
          // Traverse the first frequent value list, looking for elements in 
          // the second frequent value list. 
          // since these were scaled up during cross product, we need to scale them
          // down now

          // if OR_MERGE type, then simply add both sides frequent value lists
          if ( (CmpCommon::getDefault(HIST_INCLUDE_SKEW_FOR_NON_INNER_JOIN) == DF_ON) 
                && mergeMethod == OR_MERGE )
            resultFreqValList->scaleAndAppend(leftFrequentValueList,
                                              1, 1, getScaleFactor());
          else
            resultFreqValList->scaleAndAppend(leftFrequentValueList, 
                                              adjFreq1, adjProb1, 
                                              getScaleFactor());
        }
      }

      if (UF2 > 0)
      {
        CostScalar adjFreq2 = csZero;
        
        if ( (UC1 > 0)&& (RC1 > 0) )
        {
          adjFreq2 = (RC1 / UC1) ;
          // after having traversed all left frequent values, traverse 
          // the remaining right frequent value list and add these values
          // to the final frequent value list

          // if OR_MERGE type, then simply add both sides frequent value lists
          if ( (CmpCommon::getDefault(HIST_INCLUDE_SKEW_FOR_NON_INNER_JOIN) == DF_ON)
                && mergeMethod == OR_MERGE )
            resultFreqValList->scaleAndAppend(rightFrequentValueList,
                                              1, 1, otherStats->getScaleFactor());
          else
            resultFreqValList->scaleAndAppend(rightFrequentValueList, 
                                              adjFreq2, adjProb2,
                                              otherStats->getScaleFactor());
        }
      }

     // after having computed the steal values, adjusted the continuum values accordingly
     if (UC1 > 0) 
     {
       RC1 -= RC1*US1/UC1;
       UC1 = UC1 - US1;
     }

     if (UC2 > 0)
     {
       RC2 -= RC2*US2/UC2;
       UC2 = UC2 - US2;
     }
    }
    
    if ( tmpLeftFreqValList->entries() > 0 &&
         tmpRightFreqValList->entries() > 0 &&
         adjRowCount )
    {
      // get frequent value of the max frequency from the list.
      EncodedValue value (UNINIT_ENCODEDVALUE) ;
      FrequentValue mostFreqValue = resultFreqValList->getMostFreqValue();
      
      // search for most frequent value in THIS and OTHER ColStats and remove
      // corresponding rowcounts.
      value = mostFreqValue.getEncodedValue();

      // first try if most frequent value is stored in temp freqlists. If yes, we have
      // common skewed values, and need their original frequencies (b4 cross product)
      CostScalar leftMaxFreq = csZero;
      FrequentValue leftMostFreqValue = tmpLeftFreqValList->getMostFreqValue(value);
      FrequentValue rightMostFreqValue = tmpRightFreqValList->getMostFreqValue(value);

      if ( (value == leftMostFreqValue.getEncodedValue()) &&
           (value == rightMostFreqValue.getEncodedValue()) )
      {
        leftMaxFreq = leftMostFreqValue.getFrequency() * leftMostFreqValue.getProbability();

        HistogramSharedPtr hist = getHistogramToModify();
        Interval iter = hist->getFirstInterval() ;
        while ( iter.isValid() )
        {
          if ( iter.containsValue (value) )
            break;
          if ( iter.isLast())
            break;
          else
            iter.next() ;
        }

        CostScalar rows = csZero;
        CostScalar uec  = csZero;
        // make sure we have the correct interval
        if ( iter.containsValue (value) )
        {
          rows = iter.getRowcount();
          uec = iter.getUec();
          rows -= leftMaxFreq;
          rows = MAXOF(rows, 1.0);
          uec--;
          uec = MAXOF(uec, 1.0);
          iter.setRowsAndUec(rows, uec);
          isRCAdjusted = TRUE;
        }
         
         //  do the same thing for right interval
        CostScalar rightMaxFreq = csZero;
        rightMaxFreq = rightMostFreqValue.getFrequency() * rightMostFreqValue.getProbability();

        hist = otherStats->getHistogramToModify();
        iter = hist->getFirstInterval();

        while ( iter.isValid() )
        {
          if ( iter.containsValue (value) )
            break;
          if ( iter.isLast() )
            break;
          else
            iter.next() ;
        }
        // make sure we have the correct interval
        if ( iter.containsValue (value) )
        {
          rows = iter.getRowcount();
          uec = iter.getUec();
          rows -= rightMaxFreq;
          rows = MAXOF(rows, 1.0);
          uec--;
          uec = MAXOF(uec, 1.0);
          iter.setRowsAndUec(rows, uec);
          isRCAdjusted = TRUE;
        }
      }
    }

    setFrequentValue(*resultFreqValList);

    // save the remaining continuum values for later use
    setAdjContinuumUEC(UC1);
    otherStats->setAdjContinuumUEC(UC2);

   // save the frequency of the remaining continuum values for later use
    setAdjContinuumFreq(RC1);
    otherStats->setAdjContinuumFreq(RC2);

    delete tmpLeftFreqValList;
    delete tmpRightFreqValList;
  }
  return isRCAdjusted;
}

void FrequentValueList::mergeFreqFreqValues(FrequentValueList &leftFrequentValueList, 
                                            FrequentValueList &rightFrequentValueList,
                                            CostScalar scaleFactor,
                                            MergeType mergeMethod,
                                            FrequentValueList *tmpLeftFreqValueList,
                                            FrequentValueList *tmpRightFreqValueList)
{
  CollIndex leftIndex = 0;
  CollIndex rightIndex = 0;
      
  while ( leftIndex < leftFrequentValueList.entries() &&
          rightIndex < rightFrequentValueList.entries() 
        ) 
  {
    FrequentValue & leftV = leftFrequentValueList[leftIndex];
    FrequentValue & rightV = rightFrequentValueList[rightIndex];

    if ( leftV == rightV ) { 

      CostScalar newFreq;
      if ( (CmpCommon::getDefault(HIST_INCLUDE_SKEW_FOR_NON_INNER_JOIN) == DF_ON)
            && mergeMethod == OR_MERGE )
      {
        newFreq = MAXOF(leftV.getFrequency(), rightV.getFrequency());
      }
      else
      {
        // temporarily save left and right frequent values
        tmpLeftFreqValueList->insertFrequentValue(leftV);
        tmpRightFreqValueList->insertFrequentValue(rightV);

        // if both match, then the resultant frequency is a 
        // product of the two frequencies
        newFreq = leftV.getFrequency();
        newFreq = newFreq * (rightV.getFrequency());

        // since the frequencies were scaled up due to cross product
        // we need to scale it down now
        newFreq = newFreq / scaleFactor;
      }
      CostScalar probability = MINOF(leftV.getProbability(),
                                     rightV.getProbability());

      probability = MINOF(probability, newFreq).maxCsOne();

      // make sure the frequency is atleast 1
      newFreq = newFreq.minCsOne();

      // use leftV to hold the merged item
      leftV.setFrequency(newFreq);
      leftV.setProbability(probability);

      // add the new value into the resultant frequent value list
      // and remove them from the original frequent value lists
      this->insertFrequentValue(leftV);

      leftFrequentValueList.removeAt(leftIndex);
      rightFrequentValueList.removeAt(rightIndex);

      //leftIndex--;
      //rightIndex--;
    } else
    if ( leftV < rightV )  
       leftIndex++;
    else
       rightIndex++;
  }
}

void
FrequentValueList::scaleAndAppend(FrequentValueList & sourceFrequentValueList,
                                  CostScalar adjFreq,
                                  CostScalar adjProb,
                                  CostScalar scaleFactor)
{
  for (CollIndex sourceIndex = 0; sourceIndex < sourceFrequentValueList.entries(); sourceIndex ++)
  {
    // get the frequent value from the right side
    FrequentValue & sourceFrequentValue = sourceFrequentValueList[sourceIndex];

    CostScalar newFreq = sourceFrequentValue.getFrequency() / scaleFactor;

    // the value does not exist on the other side.
    // compute how many matches can be found for this value on the other side
    newFreq = newFreq * adjFreq;
    
    // since this value was scaled up, scale it down now, to get the correct frequency
    CostScalar newProb = sourceFrequentValue.getProbability();
    newProb = newProb * adjProb;
    // probability should be minimum of frequency and probability
    newProb = MINOF(newProb, newFreq);
    newProb = newProb.maxCsOne();
    sourceFrequentValue.setProbability(newProb);

    newFreq = newFreq.minCsOne();
    sourceFrequentValue.setFrequency(newFreq);
    // now add this value into the resultant frequent value list
    this->insertFrequentValue(sourceFrequentValue);
  }
}

NABoolean 
FrequentValueList::getfrequentValueIndex(const FrequentValue& key,
                                         CollIndex & index) const
{
  // index is the input and the output parameter. We start with the
  // input index and return the index of the element found
  for (;index < entries(); index++)
  {
    const FrequentValue & frequentValue = (*this)[index];
    if (key == frequentValue )
    {
      // entry for hash value exists, return TRUE
      return TRUE;
    }
    else
      if (key < frequentValue)
      {
        // since these are placed in order of the (encodedvalue, hash) value
        // large frequentValue means that the key value does not exist
        return FALSE;
      }
  }
  return FALSE;
}

CostScalar 
FrequentValueList::getTotalFrequency() const
{
   CostScalar totalFrequency = csZero;
  for (CollIndex index = 0; index < entries(); index++)
  {
    FrequentValue freqVal = (*this)[index];
    CostScalar freq = freqVal.getFrequency() * freqVal.getProbability();
    totalFrequency += freq;
  }
  return totalFrequency;
}

CostScalar
FrequentValueList::getMaxFrequency() const
{
  CostScalar maxFrequency = csZero;
  for (CollIndex index = 0; index < entries(); index++)
  {
    FrequentValue freqVal = (*this)[index];
    CostScalar freq = freqVal.getFrequency() * freqVal.getProbability();
    if (freq > maxFrequency)
      maxFrequency = freq;
  }
  return maxFrequency;
}

CostScalar 
FrequentValueList::getTotalProbability() const
{
  CostScalar totalProbability = csZero;
  for (CollIndex index = 0; index < entries(); index++)
    totalProbability += (*this)[index].getProbability();

  return totalProbability;
}

void 
FrequentValueList::insertFrequentValue(const FrequentValue & key)
{
  if ( (key.getEncodedValue() == UNINIT_ENCODEDVALUE) ||
       (key.getFrequency() <= csZero) )
     return;

  CollIndex j = 0;

  for (j = 0; j < entries(); j++)
  {
    FrequentValue & frequentValue = (*this)[j];
    if (key == frequentValue)
    {
      // MFV also happened to be skewed value that was put as part
      // of insertSkewedValue earlier. Do not duplicate the value
      return;
    }
    else
      if (key < frequentValue)
       break;
  }

  this->insertAt(j, key);
}

void FrequentValueList::scaleFreqAndProbOfFrequentValues(CostScalar freqScale,
                                                         CostScalar probScale)
{
  if ((freqScale == 1) && (probScale == 1))
	return;

  for (CollIndex j = 0; j < entries(); j++)
  {
    FrequentValue &thisFrequentValue = (*this)[j];
    double newFreq = thisFrequentValue.getFrequency().getValue() *freqScale.getValue();
    double newProb = thisFrequentValue.getProbability().getValue();
    
    if (probScale < 1)
      newProb *= probScale.getValue();

    newProb = MINOF(newProb, newFreq); 

    if (CmpCommon::getDefault(COMP_BOOL_42) == DF_ON)
      newFreq = MAXOF(newFreq, 1.0);
    thisFrequentValue.setFrequency(newFreq);
    thisFrequentValue.setProbability(newProb);
  }
}

void 
FrequentValueList::removeNULLAsFrequentValue()
{

  // since NULL is the last interval in the histogram, we will assume that
  // the entry for NULL interval will be towards the end of the list
  // unless ofcourse the two skew lists have been merged. Hence we will
  // start looking for NULL value from the end of the list

  for (CollIndex j = 0; j < entries(); j++)
  {
    EncodedValue boundary = (*this)[j].getEncodedValue();
    if (boundary.isNullValue() )
    {
      this->removeAt(j);
	  j--;
      break;
    }    
  }
}

void
FrequentValueList::deleteFrequentValuesAboveOrEqual(const EncodedValue & val, NABoolean include)
{

  for (CollIndex j = 0; j < entries(); j++)
  {
    EncodedValue value = (*this)[j].getEncodedValue();
    if (value > val)
    {
      this->removeAt(j);
	  j--;
	  continue;
    } 
	else
	{
	  if ( (value == val) && include)
	  {
		this->removeAt(j);
	    j--;

	  }
	}
  }
}

void
FrequentValueList::deleteFrequentValuesBelowOrEqual(const EncodedValue & val, NABoolean include)
{

  for (CollIndex j = 0; j < entries(); j++)
  {
    EncodedValue value = (*this)[j].getEncodedValue();
    if (value < val)
    {
      this->removeAt(j);
	  j--;
	  continue;
    } 
	else
	{
	  if ( (value == val) && include)
	  {
		this->removeAt(j);
	    j--;

	  }
	}
  }
}

void
FrequentValueList::deleteAllButThisFreqVal(const FrequentValue& val)
{
  for (CollIndex j = 0; j < entries(); j++)
  {
    if ((*this)[j].getEncodedValue() != val.getEncodedValue())
    {
      this->removeAt(j);
	  j--;
    }    
  }
}

void
FrequentValueList::deleteFrequentValue(const FrequentValue& val)
{
  for (CollIndex j = 0; j < entries(); j++)
  {
    if ((*this)[j] == val)
    {
      this->removeAt(j);
	  j--;
    }    
  }
}

NABoolean 
ColStats::getTotalFreqInfoForIntervalWithValue(EncodedValue newValue, 
                                               CostScalar & totalMfvRc,
                                               CostScalar &mfvCnt) 
{
  totalMfvRc = csZero;
  mfvCnt = csZero;

  Interval iter = histogram_->getFirstInterval() ;
  while ( !iter.containsValue (newValue) )
    iter.next() ;

  if ( !iter.containsValue (newValue) )
    return TRUE;

  FrequentValueList & frequentValueList = getModifableFrequentValues();
  EncodedValue loBoundary = iter.loBound() ;
  EncodedValue hiBoundary = iter.hiBound() ;
  totalMfvRc = frequentValueList.freqOfGivenEncodedVal(newValue, loBoundary, hiBoundary, mfvCnt);

  return FALSE;
}

CostScalar
FrequentValueList::freqOfGivenEncodedVal(EncodedValue mfvEV,
                                         EncodedValue loBoundary,
                                         EncodedValue hiBoundary,
                                         CostScalar &mfvCnt) const
{
  CostScalar totalMfvRc = csZero;
  for (CollIndex i = 0; i < entries(); i++)
  {
    EncodedValue mfv = (*this)[i].getEncodedValue();
    // mfv belongs to an interval lower than the interval to which the 
    // value we are looking for belongs to, so continue to traverse
    if (mfv <= loBoundary)
      continue;

    // mfv belongs to an interval higher than the interval to which the 
    // value we are looking for belongs to. No need to traverse
    if (mfv > hiBoundary)
      continue;

    // mfv belongs to interval we are interested in
    mfvCnt++;
    totalMfvRc += (*this)[i].getFrequency();
  }
  return totalMfvRc;
}


FrequentValue
FrequentValueList::getMostFreqValue() const
{
  CostScalar maxFrequency = csZero;
  CollIndex maxIndex = 0;
  for (CollIndex index = 0; index < entries(); index++)
  {
    FrequentValue freqVal = (*this)[index];
    CostScalar freq = freqVal.getFrequency() * freqVal.getProbability();
    if (freq > maxFrequency)
    {
      maxFrequency = freq;
      maxIndex = index;
    }
  }
  return (*this)[maxIndex];
}

FrequentValue
FrequentValueList::getMostFreqValue(EncodedValue  value) const
{
  for (CollIndex index = 0; index < entries(); index++)
  {
    FrequentValue freqVal = (*this)[index];
    if (freqVal.getEncodedValue() == value) {
      return freqVal;
    }
  }
  return (*this)[0];
}


// for each MC histogram we have two boundary values b_low and b_high. Assuming we have r regions we would like
// to distributed the data to.
//
// b_low   = (l1, ....., ln) where n is the number of columns in the MC
// b_high  = (h1, ......, hn)
//
// then the ranges that will be created are as follows
//
// - for range 1 the begin key will be b_low
// - for all other ranges k from 2 to r-1, the begin key is (vk1,...,vkn) where vki is computed as follow:
//     vki = v(k-1)i + (hi-li)/n
//
void MCboundaryValueList::getMinMax (const MCboundaryValueList& lv, const MCboundaryValueList& hv, Int32 numParts, LIST(MCboundaryValueList) &vals)
{
   vals.insert(lv);

   for (Int32 i = 1; i < numParts; i++)
   {
      MCboundaryValueList nv;
      // generated a mc boundary value based on the previous generated boundary value
      for (Int32 j = 0; j < lv.entries(); j ++)
      {
         double dbv1 = vals[i-1][j].getDblValue ();
         dbv1 += ((hv[j].getDblValue () - lv[j].getDblValue ())/numParts);
         EncodedValue ev (dbv1);
         nv.insert(ev);
      }
      vals.insert(nv);
   }
}

NAString* MCboundaryValueList::convertToString (const NAColumnArray& colArray, NABoolean forLastInterval)
{
   NAString* val = new (heap_) NAString("");

   // Note that the number of MC columns
   // might be less then that of the number of columns
   for (Int32 i = 0; i< colArray.entries(); i++)
   {
      const NAType* nt = colArray[i]->getType();

      double ev = 0;

      if (forLastInterval)
         ev = nt->getMaxValue();
      else if (i >= this->entries())
         ev = nt->getMinValue();
      else
         ev = (*this)[i].getDblValue ();

      NAString* vStr = nt->convertToString (ev, heap_);

      if (i != 0)
        (*val) += ", ";

      (*val) += (*vStr);
   }

   return val;
}

void MCboundaryValueList::display() const
{
  print();
}

void MCboundaryValueList::print( FILE* ofd,
            const char* indent,
            const char* title) const
{
   
   char NEW_INDENT2[] = "   ";
   fprintf(ofd,"%s%s: ",NEW_INDENT2, title);
   if (this->entries() == 0)
   {
       fprintf(ofd,"empty list\n");
   }

   fprintf(ofd,"list with %d items\n",this->entries());
   fprintf(ofd,"%svalues: ",NEW_INDENT2);
   for (Int32  i = 0; i < this->entries(); i++)
   {
      fprintf(ofd," val: ");
      ((*this)[i].getValue()).display(ofd);

   }

   fprintf(ofd,"\n");
}

