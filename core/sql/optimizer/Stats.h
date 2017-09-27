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
#ifndef STATS_HDR
#define STATS_HDR

/* -*-C++-*-
 ******************************************************************************
 *
 * File:         Stats.h
 * Description:  This file includes definitions for statistics related
 *               information used by the NOAH optimizer.
 *
 * Created:      March 16, 1994
 * Language:     C++
 *
 *
 *
 *
 ******************************************************************************
 */


// -----------------------------------------------------------------------
//  Include Files
// -----------------------------------------------------------------------
#include <math.h>
#include "BaseTypes.h"
#include "CostScalar.h"
#include "Collections.h"
#include "NAType.h"
#include "ValueDesc.h"
#include "EncodedValue.h"
#include "SharedPtrCollections.h"

// -----------------------------------------------------------------------
// macro to get the square of a number
// -----------------------------------------------------------------------
#ifndef SQUARE
#define SQUARE(x) ((x)*(x))
#endif

// -----------------------------------------------------------------------
//  The following classes are defined in this file.
// -----------------------------------------------------------------------
class HistInt;                    // Histogram interval
class Interval ;                  // an intelligent interface to HistInts
class Histogram;                  // Histogram: ordered collection of histogram intervals
class ColStats ;                  // Column Statistics
class StatsList;                  // List of column statistics
class FrequentValue;     // hash value, encoded value and frequency of skewed values
class FrequentValueList;
class ColumnId;
class ColumnSet;
class MultiColumnHistogram;
class MultiColumnHistogramList;

typedef IntrusiveSharedPtr<ColStats> ColStatsSharedPtr;
typedef IntrusiveSharedPtr<Histogram> HistogramSharedPtr;

//enumerated types used in histogram intervals reduction

//Source identifies the location from where the reduction
//of the number of histogram intervals is invoked.
//AFTER_FETCH implies after statistics have be fetched
//using FetchHistogram
//INTERMEDIATE implies after a new histogram has been
//generated through an intermediate relational operation
//like a join
enum Source {AFTER_FETCH, INTERMEDIATE};

//Criterion identifies the criterion to use while merging
//two intervals for the purpose of reducing the number of
//intervals in a histogram
//NONE implies that no two intervals should not be merged
//CRITERION1 implies that two intervals should be merged
//using criterion 1 as defined in the histogram intervals
//reduction design document
//CRITERION2 implies that two intervals should be merged
//using criterion 2 as defined in the histogram intervals
//reduction design document
enum Criterion {NONE=0, CRITERION1=1, CRITERION2=2};

// ----------------------------------------------------------------------
// An indication of how pairs of ColStats are to be combined/merged.
// The following summarizes the impact on each ColStats' interval:
// INNER_JOIN_MERGE:  'Typical' join
//    numUec  = MINOF(leftUEC, rightUEC)
//    numRows = (leftRowCount * rightRowCount) / MAXOF(leftUEC, rightUEC)
// SEMI_JOIN_MERGE:
//	numUec  = MINOF( leftUEC, rightUEC )
//    numRows = leftRowCount * (numUec / leftUEC)
// ANTI_SEMI_JOIN_MERGE:
//    numUec  = MAXOF (0, leftUEC-rightUEC)
//    numRows = leftRowCount * (numUec / leftUEC)
// OUTER_JOIN_MERGE:
//    similar to INNER_JOIN_MERGE, except it retains two copies of the
//    INNER_JOIN_MERGE's result.
//    [one to be LEFT_JOIN_OR_MERGED, the other to be null-augmented.]
// LEFT_JOIN_OR_MERGE:
//    merges the OUTER_JOIN_MERGE's result with the original histogram of
//    the outer-join column.
//    numUec  = rightUEC
//    numRows = leftRowCount + ((rightRowCount/numUec) * (numUec-leftUEC))
// UNION_MERGE:
//	numUec  = MAXOF( leftUEC, rightUEC )
//    numRows = leftRowCount + rightRowCount
// OR_MERGE:
//    numUec  = MAXOF( leftUEC, rightUEC )
//    numRows = MAXOF( leftRowCount, rightRowCount )
// AND_MERGE:
//    numUec  = MINOF( leftUEC, rigthUEC )
//    numRows = MINOF( leftRowcount, rightRowcount )
// ----------------------------------------------------------------------
enum MergeType { INNER_JOIN_MERGE, SEMI_JOIN_MERGE, ANTI_SEMI_JOIN_MERGE,
                 OUTER_JOIN_MERGE, LEFT_JOIN_OR_MERGE,  
                 UNION_MERGE, OR_MERGE, AND_MERGE };

// ColumnSet is a collection of ColumnIds
class ColumnSet : public ClusteredBitmap
{
 public: 
  // constructor
  ColumnSet(NAMemory *heap) : ClusteredBitmap(heap) {}

  // construct a memory efficient representation of colArray
  ColumnSet(const NAColumnArray& colArray, NAMemory *heap);

  // copy constructor
  ColumnSet(const ColumnSet& other, NAMemory *heap)
    : ClusteredBitmap(other,heap) {}

  ~ColumnSet() {}

  // Iterator methods for a ColumnSet
  // use the iterators in a for loop like this (assuming you have a
  // ColumnSet S over which you want to iterate)
  // for (CollIndex x=0;  S.next(x); S.advance(x) )
  //    { /* x is the current element */ }
  CollIndex init() const  { return CollIndex(0); }
  NABoolean next(CollIndex &x) const { return nextUsed(x); }
  void advance(CollIndex & x) const  { x++; }

  void display() const;

  void print() const;
  void printColsFromTable( FILE* ofd, NATable* table) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(ColumnSet);
  ColumnSet(); // should not be called, should get link error.
};

// -----------------------------------------------------------------------
//  MC Skewed Value List - contains skewed values for a column group.
//  Information is stored in two lists which need to be maintained in 
//  sync: List of distinct values and corresponding frequency
// -----------------------------------------------------------------------
class MCSkewedValue : public NABasicObject
{
public:

  MCSkewedValue(NAMemory * h = 0) : boundary_((NAWchar *)(L"")), frequency_(-1), heap_(h) {}

  MCSkewedValue(NAWchar * boundary, CostScalar frequency, EncodedValue * eV, UInt32 hash, NAMemory * h = 0) :
    boundary_(boundary) ,
    frequency_(frequency), 
    mcEncodedValue_(eV),
    hash_(hash),
    heap_(h) 
    {
    };

  ~MCSkewedValue() {};

  MCSkewedValue(const MCSkewedValue& x) : 
     boundary_(x.boundary_), frequency_(x.frequency_), mcEncodedValue_(x.mcEncodedValue_), hash_(x.hash_) {};

  MCSkewedValue & operator=(const MCSkewedValue& other);

  inline NABoolean operator==(const MCSkewedValue& other) const
  { return (*mcEncodedValue_ == *(other.mcEncodedValue_)); }
  inline NABoolean operator!=(const MCSkewedValue& other) const
  { return (*mcEncodedValue_ != *(other.mcEncodedValue_)); }
  inline NABoolean operator<(const MCSkewedValue& other) const
  { return (*mcEncodedValue_ < *(other.mcEncodedValue_)); }
  inline NABoolean operator<=(const MCSkewedValue& other) const
  { return (*mcEncodedValue_ <= *(other.mcEncodedValue_)); }
  inline NABoolean operator>(const MCSkewedValue& other) const
  { return (*mcEncodedValue_ > *(other.mcEncodedValue_)); }
  inline NABoolean operator>=(const MCSkewedValue& other) const
  { return (*mcEncodedValue_ >= *(other.mcEncodedValue_)); }

  const NAWchar * getBoundary() { return boundary_; }
  CostScalar getFrequency() { return frequency_; } 
  const EncodedValue * getEncodedValue () { return mcEncodedValue_ ; }
  const UInt32 getHash () { return hash_; }

  void display() const;
  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "",
              CollHeap *c=NULL, char *buf=NULL) const;

private:  
  NAWchar * boundary_ ;
  CostScalar frequency_ ;
  EncodedValue * mcEncodedValue_;
  UInt32 hash_;
  NAMemory * heap_;
};

class MCSkewedValueList : public NAList<MCSkewedValue *>
{
public:
  // constructor 
  MCSkewedValueList(NAMemory *h=0)
    : NAList<MCSkewedValue *>(h),heap_(h) {};

  MCSkewedValueList(const MCSkewedValueList & mcsvl, NAMemory *h=0);

  ~MCSkewedValueList() {};

  void addMCSkewedValue(MCSkewedValue *newValue);
  void addMCSkewedValue(const NAWchar * boundary, CostScalar frequency, const EncodedValue & eV, UInt32 hash);

  MCSkewedValueList & operator=(const MCSkewedValueList& other);
  NABoolean operator==(const MCSkewedValueList& other);

  void mergeMCSkewedValueList(MCSkewedValueList * leftSide, 
                              MCSkewedValueList * rightSide,
                              CostScalar avgRowcountForNonSkewValuesOnLeftSide,
                              CostScalar avgRowcountForNonSkewValuesOnRightSide,
                              MergeType mergeMethod = INNER_JOIN_MERGE);

  void display() const;
  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "",
              CollHeap *c=NULL, char *buf=NULL) const;

private:  
  NAMemory * heap_;
};

// MultiColumnHistogram is the memory-efficient contextheap 
// representation of a table's multi-column histogram
class MultiColumnHistogram : public NABasicObject
{
 public: 
  MultiColumnHistogram
    (ColumnSet&  columns,
     CostScalar& uec,
     CostScalar& rows,
     ComUID&     id,
     MCSkewedValueList *mcSkewedValueList,
     ColStatsSharedPtr colStatsPtr,
     NAMemory*   heap
     ) 
    : columns_(columns, heap), uec_(uec), rows_(rows)
    , histogramID_(id), mcSkewedValueList_(mcSkewedValueList)
    , colStatsPtr_(colStatsPtr), heap_(heap) {}

  ~MultiColumnHistogram() {columns_.clear();}

  CostScalar uec()  const { return uec_; }
  CostScalar rows() const { return rows_; }
  ComUID     id()   const { return histogramID_; }

  const ColumnSet& cols() const { return columns_; }
  const MCSkewedValueList *getMCSkewedValueList() const { return mcSkewedValueList_; }

  ColStatsSharedPtr getColStatsPtr()   const { return colStatsPtr_; }

  void display() const;
  void print( FILE* ofd=stdout, NATable* table=NULL) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(MultiColumnHistogram);

  ColumnSet  columns_;
  CostScalar uec_;
  CostScalar rows_;
  ComUID     histogramID_;
  MCSkewedValueList *mcSkewedValueList_;
  ColStatsSharedPtr colStatsPtr_;
  NAMemory  *heap_;
};

// MultiColumnHistogramList is the memory-efficient contextheap 
// representation of a table's multi-column histograms only
class MultiColumnHistogramList : public NAList<MultiColumnHistogram*> 
{
 public: 
  // constructor 
  MultiColumnHistogramList(NAMemory * heap)
    : NAList<MultiColumnHistogram*>(heap), heap_(heap) {}

  // destructor
  ~MultiColumnHistogramList();

  // add this multi-colum histogram to this list 
  // (avoid adding any duplicate multi-column histograms)
  void addMultiColumnHistogram(const ColStats & mcStat, 
                               ColumnSet* singleColPositions=NULL);

  // add these multi-colum histograms to this list 
  // (avoid adding any duplicate multi-column histograms)
  void addMultiColumnHistograms(const StatsList & colStats);

  void display() const;
  void print( FILE* ofd=stdout, NATable* table=NULL) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(MultiColumnHistogramList);

  NAMemory  *heap_;
};

class FrequentValue : public NABasicObject
{
public:

  FrequentValue(UInt32 hashValue, CostScalar frequency, 
                CostScalar probability, EncodedValue value);

  FrequentValue() : hash_(0), frequency_(-1), probability_(0), encodedValue_(UNINIT_ENCODEDVALUE) {}

  FrequentValue(const FrequentValue& x) : 
     hash_(x.hash_), frequency_(x.frequency_), probability_(x.probability_), encodedValue_(x.encodedValue_) {}

  FrequentValue(const EncodedValue& normValue, ConstValue* itemPtr, const NAType*,
                CostScalar frequency = csOne, CostScalar probability = csOne
               );

  ~FrequentValue() {};

  inline UInt32 getHash() const { return hash_; }
  inline CostScalar getFrequency() const { return frequency_; }
  inline CostScalar getProbability() const { return probability_; }
  inline EncodedValue getEncodedValue () const { return encodedValue_ ; }

  void setFrequency(CostScalar freq) { frequency_ = freq; }
  void setProbability(CostScalar prob = csOne) { probability_ = prob; }

  NABoolean operator ==(const FrequentValue& x) const
   { return (encodedValue_.isNullValue() && x.encodedValue_.isNullValue() ) ||
            (encodedValue_ == x.encodedValue_ && hash_ == x.hash_); }

  NABoolean operator >(const FrequentValue& x) const
   { return (encodedValue_.isNullValue() && !x.encodedValue_.isNullValue()) || 
            (encodedValue_ > x.encodedValue_ || 
             (encodedValue_ == x.encodedValue_ && hash_ > x.hash_)); }

  NABoolean operator <(const FrequentValue& x) const
   { return (!encodedValue_.isNullValue() && x.encodedValue_.isNullValue()) ||
            (encodedValue_ < x.encodedValue_ || 
             (encodedValue_ == x.encodedValue_ && hash_ < x.hash_)); }

  void print (FILE *f, const char * prefix, const char * suffix,
              CollHeap *c=NULL, char *buf=NULL) const;

private:
  UInt32 hash_;
  CostScalar frequency_;
  CostScalar probability_;
  EncodedValue encodedValue_;
};
  

class FrequentValueList : public LIST(FrequentValue)
{
public:
  FrequentValueList (NAMemory* h,CollIndex initLen =0) 
    : LIST(FrequentValue) (h,initLen)
  { };

  FrequentValueList (const FrequentValueList & fvlist, NAMemory* h) :
    LIST(FrequentValue)(fvlist, h)
    {}

  ~FrequentValueList() {};

  NABoolean isFull();

  void insertFrequentValue(const FrequentValue & freqValue);

  void scaleFreqAndProbOfFrequentValues(CostScalar freqScale,
                                        CostScalar probScale);

  void deleteFrequentValuesAboveOrEqual(const EncodedValue & val, NABoolean include = FALSE);

  void deleteFrequentValuesBelowOrEqual(const EncodedValue & val, NABoolean include = FALSE);

  void deleteAllButThisFreqVal(const FrequentValue& key);

  void deleteFrequentValue (const FrequentValue& key);

  void removeNULLAsFrequentValue();

  CostScalar getTotalFrequency() const;
  CostScalar getTotalProbability() const;
  CostScalar getMaxFrequency() const;
  FrequentValue getMostFreqValue(EncodedValue value) const;
  FrequentValue getMostFreqValue() const;

  void mergeFreqFreqValues(FrequentValueList &leftFrequentValueList, 
                           FrequentValueList &rightFrequentValueList,
                           CostScalar scaleFactor,
                           MergeType mergeMethod,
                           FrequentValueList *tmpLeftFreqValueList,
                           FrequentValueList *tmpRightFreqValueList);

  void scaleAndAppend(FrequentValueList & rightFrequentValueList,
                      CostScalar adjFreq,
                      CostScalar adjProb,
                      CostScalar scaleFactor);

  NABoolean getfrequentValueIndex(const FrequentValue&, CollIndex & index) const;

  CostScalar freqOfGivenEncodedVal(EncodedValue mfvEV, 
                                   EncodedValue loBoundary,
                                   EncodedValue hiBoundary,
                                   CostScalar &mfvCnt) const;
  void display() const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "",
              CollHeap *c=NULL, char *buf=NULL) const;
};

// a list to encapsulate an MC list of encoded values
class MCboundaryValueList : public LIST(EncodedValue)
{
public:
   MCboundaryValueList (NAMemory* h=STMTHEAP) : heap_ (h), LIST(EncodedValue) (h) {};

   MCboundaryValueList (const MCboundaryValueList& other, NAMemory* h=STMTHEAP) : heap_ (h), LIST(EncodedValue) (h) 
   {
      for (Int32 i=0; i < other.entries(); i++)
      {
         this->insert(other[i]);
      }
   } 

   MCboundaryValueList (const NormValueList* nvl, NAMemory* h=STMTHEAP) : heap_ (h), LIST(EncodedValue) (h) 
   {
      for (Int32 i=0; i < nvl->entries(); i++)
      {
        EncodedValue ev;
        ev.setValue(nvl->at(i));
        this->insert(ev);
      }
   };
  
   COMPARE_RESULT compare (const MCboundaryValueList &other) const
   {
      DCMPASSERT (this->entries() == other.entries());
      for (Int32  i = 0; i < this->entries(); i++)
      {
          if ((*this)[i].compare(other[i]) == SAME)
            continue;
          return ((*this)[i].compare(other[i]));
      }
      return (SAME);
   }

   NABoolean operator< (const MCboundaryValueList &other) const
   {
      return (this->compare(other) == LESS);
   }
   NABoolean operator<= (const MCboundaryValueList &other) const
   {
      COMPARE_RESULT  result = this->compare(other);
      return ((result == SAME) || (result == LESS));
   }
   NABoolean operator> (const MCboundaryValueList &other) const
   {
      return (this->compare(other) == MORE);
   }
   NABoolean operator>= (const MCboundaryValueList &other) const
   {
      COMPARE_RESULT  result = this->compare(other);
      return ((result == SAME) || (result == MORE));
   }

   void getValueList (NormValueList& list)
   {
      for (Int32  i = 0; i < this->entries(); i++)
      {
          list.insertAt(i, ((*this)[i].getValue()));
      }
   }

   NAString* convertToString (const NAColumnArray& colArray, NABoolean forLastInterval);
 
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
   void getMinMax (const MCboundaryValueList& lv, const MCboundaryValueList& hv, Int32 numParts, LIST(MCboundaryValueList) &vals);

   // ---------------------------------------------------------------------
   //   // Print
   // ---------------------------------------------------------------------
   void display() const ;
 
   void print( FILE* ofd = stdout,
               const char* indent = DEFAULT_INDENT,
               const char* title = "MCboundaryValueList") const;

   ~MCboundaryValueList () {};
private:
  NAMemory * heap_;
};


// -----------------------------------------------------------------------
//  HistInt: "HISTogram INTerval"
// -----------------------------------------------------------------------

class HistInt
{
friend class Interval ;

protected:
  // copy method
  void copy (const HistInt& other);

public:

  // -----------------------------------------------------------------------
  //  Constructors
  // -----------------------------------------------------------------------
  HistInt () : rows_(0), uec_(0), boundary_(UNINIT_ENCODEDVALUE), boundInc_(FALSE), hash_(0), rows2mfv_(0),
               MCBoundary_(STMTHEAP)
  {}


  HistInt (const EncodedValue & value, NABoolean boundIncluded = FALSE, UInt32 hash = 0)
       : rows_(0), uec_(0), boundary_(value), boundInc_(boundIncluded), hash_(hash), rows2mfv_(0),
         MCBoundary_(STMTHEAP)
  {
     setupMCBoundary ();
  }

  HistInt(Int32 intNum, const NAWchar *intBoundary, const NAColumnArray &columns,
	  CostScalar card, CostScalar uec, NABoolean boundInc = TRUE, CostScalar card2mfv = 0) ;

  // copy constructor
  HistInt (const HistInt & other) : MCBoundary_(STMTHEAP)
  { copy(other); }

public:

  // assignment operator
  HistInt & operator = (const HistInt& other)
  {
    if (this != &other)
      copy(other);
    return *this;
  }

  void setupMCBoundary ();

  // comparison operator
  NABoolean operator == (const HistInt& other) const
  {
  if(this->boundary_.compare(other.boundary_) == SAME
      && this->boundInc_ == other.boundInc_
      && this->MCBoundary_.compare(other.MCBoundary_) == SAME
      && (this->rows_) == other.rows_
      && (this->uec_) == other.uec_
     )
      return TRUE;
  else return FALSE;
      }

  // -----------------------------------------------------------------------
  //  Destructor
  // -----------------------------------------------------------------------
  virtual ~HistInt() {}

  // ---------------------------------------------------------------------
  //  Accessor Functions
  // ---------------------------------------------------------------------
  inline const EncodedValue & getBoundary () const { return boundary_ ; }
  inline const MCboundaryValueList & getMCBoundary () const { return MCBoundary_ ; }
  inline NABoolean isBoundIncl ()            const { return boundInc_ ; }
  inline CostScalar getCardinality ()        const { return rows_ ; }
  inline CostScalar getUec()                 const { return uec_; }
  inline UInt32 getHash()                    const { return hash_; }
  inline CostScalar getCardinality2mfv()      const { return rows2mfv_ ; }

  inline CostScalar getFudgedUec ()          const
  { return ( uec_ == csZero ? uec_ :  MAXOF(uec_, csOne) ) ; }
  inline NABoolean isNull ()                 const
  { return boundary_.isNullValue() ; }

  // ---------------------------------------------------------------------
  //  Manipulation Methods
  // ---------------------------------------------------------------------
  inline void setBoundary (const EncodedValue & intBound)
  { boundary_ = intBound ; }
  inline void setBoundIncl(NABoolean boundIncl = TRUE)
  { boundInc_ = boundIncl ; }

  // ---------------------------------------------------------------------
  // HistInt::mergeInterval, merges the left and right intervals based 
  // on the mergeMethod. This is a helper method for ColStats::mergeColStats
  // ----------------------------------------------------------------------
  CostScalar mergeInterval(const HistInt & left,
                          const HistInt & right,
                          CostScalar scaleRowCount,
                          MergeType mergeMethod = INNER_JOIN_MERGE
);

  // the following is used to maintain the semantic : uec <= rows
  void setCardAndUec (CostScalar card, CostScalar uec) ;
  void setCardinality2mfv(CostScalar card) ;

  // -----------------------------------------------------------------------
  //  Utility routines
  // -----------------------------------------------------------------------
  void display (FILE *f = stdout,
		const char * prefix = DEFAULT_INDENT,
		const char * suffix = "",
                CollHeap *c=NULL, char *buf=NULL) const;

private:
  // these first two methods are private because we never want them used
  // individually ==> use the ::setCardAndUec() routine instead

  // NB: the values for these should always be >= 0 !
  // --> and if the value is extremely low (e.g., 1e-16), round it to zero
// warning elimination (removed "inline")
  void setCardinality (CostScalar card);
// warning elimination (removed "inline")
  void setUec (CostScalar uec);

  EncodedValue boundary_;      // histint boundary (upper bound)
  MCboundaryValueList MCBoundary_;
  NABoolean boundInc_;         // TRUE ==> boundary is inclusive
  CostScalar rows_;            // histint cardinality
  CostScalar uec_;             // histint UEC
  UInt32 hash_;                // histint hash for some SQL data type T
                               // such that any v of T we have 
                               // EncodedValues(v) != v. CHAR is one such
                               // SQL data type.
  CostScalar rows2mfv_;         // rowcount for the 2nd most frequent value
                               // in the interval
};  // HistInt


// -----------------------------------------------------------------------
//  Histogram Class - a collection of intervals
// -----------------------------------------------------------------------
class Histogram : public LIST (HistInt)
{
  friend class Interval ;

public:

  Histogram(NAMemory* h) : LIST(HistInt)(h)
    {}

  Histogram (const Histogram & hist, NAMemory* h) :
    LIST(HistInt)(hist, h)
    {}

private:
  // we prevent people from creating Histograms that aren't explicitly put
  // on a heap! code doing this will not link!
  Histogram () ;
  Histogram (const Histogram & hist) ;

public:

  // ---------------------------------------------------------------------
  //  Histogram Manipulation routines
  // ---------------------------------------------------------------------
  // Given two histograms, create a Template histogram to use in subsequent
  //  merge operations involving those two histograms.

  // newest, simplest version of cMT -- it works and is much easier to
  // understand
  HistogramSharedPtr createMergeTemplate (const HistogramSharedPtr& otherHistogram,
                                          NABoolean equiMerge) const;

  // ----------------------------------------------------------------------
  // utility routine used by ColStatDescList::divideHistogramAtPartitionBoundaries()
  //
  // given a two histograms, merges all intervals in THIS that do not occur in
  // partitionBoundaries
  //
  // if there are any intervals in THIS that are outside the range of
  // partitionBoundaries, then returns FALSE
  //
  // returns TRUE if successful without errors, FALSE otherwise
  NABoolean condenseToPartitionBoundaries (const HistogramSharedPtr& partitionBoundaries) ;

public:


  // -----------------------------------------------------------------------
  // simplifying, oft-used utility routines
  // (2 versions, because sometimes this is a useful shortcut from
  // within other const member functions)
  // -----------------------------------------------------------------------
  inline HistInt& firstHistInt()             { return (*this)[0] ; }
  inline const HistInt& firstHistInt() const { return (*this)[0] ; }

  inline HistInt& lastHistInt()              { return (*this)[this->entries()-1] ; }
  inline const HistInt& lastHistInt() const  { return (*this)[this->entries()-1] ; }

  // -----------------------------------------------------------------------
  // Interval : a simplification of HistInts
  // --> needed because of all the hassles/headaches caused by
  //     "single-value intervals"
  // -----------------------------------------------------------------------

  // returns the # of intervals in the Histogram
  // this number will be somewhere in between 0 and entries()
// warning elimination (removed "inline")
  CollIndex numIntervals() const
  {
    DCMPASSERT( entries() != 1 ) ; // generic Histogram sanity check

    if( entries() == 1)
    {
      // log the message to the event log
      // SQLMXLoggingArea::logSQLMXAssertionFailureEvent(__FILE__, __LINE__, "Histogram has just one HistInt");

      Histogram* tempHist = (Histogram *)this;
      tempHist->clear();
    }

    if ( entries() == 0 )
      return 0 ;

    return (entries()-1) ;
  }

  // -----------------------------------------------------------------------
  // NULL-handling Histogram methods
  // -----------------------------------------------------------------------

  // is there a NULL interval in this Histogram?
   NABoolean isNullInstantiated() const ;
  // remove that NULL interval if it exists
   void removeNullInterval() ;
  // insert a NULL interval if it doesn't exist
   void insertNullInterval () ;

  // NB: Intervals are indexed from 1..numIntervals()

  // inserts, as necessary, an SVI into the list of HistInts
  // with the appropriate value ;
  // assigns this SVI the appropriate row/uec values, subtracting from
  // the neighboring Intervals as necessary
  CollIndex insertSingleValuedInterval (const EncodedValue & value,
                                        NABoolean distributeRowsAndUec = TRUE);

  CostScalar mergeSVIWithNextAndSetMaxFreq();

  //
  // Does the work of splitting the Histogram at a certain boundary
  // value.
  //
  // This may or may not require inserting a HistInt (if value is not
  // equal to a current HistInt boundary value, we will need to insert
  // a new HistInt -- and even if value IS equal to a current HistInt
  // value, we often need to insert another HistInt, depending on
  // whether we're splitting the Histogram for a <=,>=,< or > operation.
  // (We may need to create an S.V.I. in order to split the Histogram
  // in the right place.)

  // inserts a new zero-row/uec interval into the Histogram
  // --> NB, can only do this if this Interval is above the
  //     range of current Intervals, below them, or none currently exist
  //
  // The 'isNewBoundIncluded' flag determines the boundary inclusiveness
  // of the new Interval.
  void insertZeroInterval (const EncodedValue & loValue,
                           const EncodedValue & hiValue,
                           NABoolean isNewBoundIncluded) ;
  void insertZeroInterval (const CostScalar& loValue,
                           const CostScalar& hiValue,
                           NABoolean isNewBoundIncluded) ;

  void insertZeroInterval (const NormValueList& loValue,
                           const NormValueList& hiValue,
                           NABoolean isNewBoundIncluded) ;


  // 2 steps to condense a histogram into a single interval:
  // 1. takes all of the current intervals and adds up the rows / uecs
  // 2. creates a single interval with the same max/min/row/uec values
  void condenseToSingleInterval();

  // Histogram intervals reduction methods
  // Method to reduce the number of intervals 
  void reduceNumHistInts(Criterion reductionCriterion, 
                                 Source invokedFrom = AFTER_FETCH);

  // compute the extended boundaries of an interval when compared to its neighbors. The method does not
  // have any side affect on the interval or its neighbors . This is used by the HQC logic
  void computeExtendedIntRange (Interval& currentInt, Criterion& reductionCriterion, 
                                EncodedValue& hiBound, EncodedValue& loBound, 
                                NABoolean& hiBoundInclusive, NABoolean& loBoundInclusive);


  // ----------------------------------------------------------------------------
  // Method to reduce the number of histogram intervals based on query predicates
  // example predicates
  // * t1.col1 = 3
  // * t1.col1 < 3
  // * t1.col1 > 1
  // * t1.col1 > 1 and t1.col1 < 3
  // * t1.col1 = t2.col1 // i.e. a join predicate
  // ----------------------------------------------------------------------------
  void compressHistogramForQueryPreds(ItemExpr * lowerBound,
                                      ItemExpr * upperBound,
                                      NABoolean hasJoinPred = FALSE);

  // to calculate the selectivity for an equality predicate
  NABoolean computeSelectivityForEquality(ItemExpr * constVal, // input
                                          CostScalar totalRowcount,  // input
                                          CostScalar totalUEC, // input
                                          CostScalar& selectivity // output
                                          );

  // iterator methods
  inline Interval getFirstInterval() const ;
  inline Interval getLastInterval() const ;
  inline Interval getLastNonNullInterval () const ;
// warning elimination (removed "inline")
  Interval getNextInterval(const Interval & current) const ;
// warning elimination (removed "inline")
  Interval getPrevInterval(const Interval & current) const ;

  // returns the interval #'d by index
  inline Interval getInterval (CollIndex index) const ;

  // -----------------------------------------------------------------------
  //  Utility routines
  // -----------------------------------------------------------------------
  void display() const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "",
              CollHeap *c=NULL, char *buf=NULL) const;

  // This is part of the IntrusiveSharedPtr mechanism.
  INTRUSIVE_SHARED_PTR(Histogram);
private:

  // removed the heap ptr because we don't need it
  //  NAMemory* heap_;
}; // Histogram


// -----------------------------------------------------------------------
//  Interval - an attempt to abstract away the nastiness
//             of "single-valued intervals"
//  purpose: mainly used to enumerate through the Histogram
//  easi/
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
//
//  RESTRICTION RESTRICTION RESTRICTION RESTRICTION RESTRICTION RESTRICTION
//
//  As a performance improvement this class now keeps a local copy
//  of (*hist_)[N+1] as HistInt hiInt. Need to make sure you have the
//  right copy or else there will be assertion failure in debug version.
//  Procedure refreshHiInt() will get the local copy in sync. But use
//  this call miserly as it is expensive.
//
// downfalls to avoid:
//
// 1.  Interval iter = histogram->getFirstInterval();
//     Interval iter1 = iter;
//     iter.setRowCount(45);
//
//      In the above case although the iter and histogram got modified, iter1
//      did not. Developer should set the rowcount fisrt then do the assignment
//      or call refreshHiInt() for iter1.
//
// 2.   Interval iter = histogram->getFirstInterval();
//      iter.next();
//      histogram.removeat(iter.getLoIndex() +1);
//
//      Now iter's hiInt is bad because it was removed by histogram so
//      developer needs to call refreshHiInt()
// -----------------------------------------------------------------------

#pragma nowarn(270)   // warning elimination
class Interval : public NABasicObject
{
public:
  //
  // ctors
  // NB: no need for a dtor
  //
  Interval () :
      loIndex_(NULL_COLL_INDEX), hist_(0)
  {}

  Interval (CollIndex startIndex, const HistogramSharedPtr& hist) :
       loIndex_(startIndex), hist_(hist) ,
       hiInt_((*hist)[startIndex+1])

  {}

  // copy ctor
  Interval (const Interval & other) :
   loIndex_(other.loIndex_), hist_(other.hist_) ,
   hiInt_((*hist_)[loIndex_+1])

  {}

  Interval & operator = (const Interval & other)
  {
    loIndex_ = other.loIndex_ ;
    hist_    = other.hist_ ;
    if(hist_)
    {
      hiInt_ = ((*hist_)[loIndex_+1]);
    }
    return *this ;
  }

  //
  // simple inline methods
  //

  // needed for Collections classes, for some reason
  inline NABoolean operator == (const Interval & other)
  { return ( loIndex_ == other.loIndex_ && hist_ == other.hist_ ) ; }

  // am I the first/last Interval in the histogram?
  inline NABoolean isFirst() const ;
  inline NABoolean isLast() const ;

  // gets/sets the uec & rowcount values
// warning elimination (removed "inline")
  CostScalar getUec() const
  {
  OK() ;
  DCMPASSERT(hiInt_ == ((*hist_)[loIndex_+1]));
  return hiInt_.getUec() ;
  }

// warning elimination (removed "inline")
  CostScalar getRowcount() const
  {
  OK() ;
  DCMPASSERT(hiInt_ == ((*hist_)[loIndex_+1]));
  return hiInt_.getCardinality() ;
  }

  UInt32 getHash() const
  {
  OK() ;
  DCMPASSERT(hiInt_ == ((*hist_)[loIndex_+1]));
  return hiInt_.getHash() ;
  }

  // get the rowcount of the 2nd most frequent value
  CostScalar getRowcount2mfv() const
  {
  OK() ;
  DCMPASSERT(hiInt_ == ((*hist_)[loIndex_+1]));
  return hiInt_.getCardinality2mfv() ;
  }

  // search list for the MFV that is contained in this Interval. Also
  // return the MFV's frequency. Return FALSE if this interval does not
  // associate with any skew values.
  NABoolean 
  getMFV(const FrequentValueList&, EncodedValue& mfv, CostScalar& freq);

  NABoolean 
  getMFV(const MCSkewedValueList&,  MCboundaryValueList& mfv, CostScalar& freq);


  void getRCSmallerThanMFV(const EncodedValue& mfv,
                                 const CostScalar& freqMFV,
                                 CostScalar& rc);

  void getRCSmallerThanMFV(const MCboundaryValueList& mfv,
                                 const CostScalar& freqMFV,
                                 CostScalar& rc);


  void makeSplits(
                          HistogramSharedPtr& newHist,
                          const NAType* nt,
                          const CostScalar newHeight,
                          CostScalar& newRC,        // On entry: the # to fill;
                                                    // On exit: #rc unfilled
                          CostScalar& availableRC,  // On extry: rc available;
                                                    // On exit: the # of rows remaining in the interval
                          CostScalar& lowB,         // On entry: the low bound to use to insert the new
                                                    // first interval.
                                                    // On exit: the current last low bound to use
                                                    // to insert a new interval.
                          const CostScalar& lowBInt,// the low and high bound in which availableRC
                          const CostScalar& hiBInt, // #rows resides. The two bounds are used to
                                                    // compute the new high bound(s) for new intervals
                          NABoolean allowSplits);


  void makeSplitsForMC(   HistogramSharedPtr& newHist,
                          const CostScalar newHeight,
                          CostScalar& newRC,        // On entry: the # to fill;
                                                    // On exit: #rc unfilled
                          CostScalar& availableRC,  // On extry: rc available;
                                                    // On exit: the # of rows remaining in the interval
                          NormValueList* lowB,      // On entry: the low bound to use to insert the new
                                                    // first interval.
                                                    // On exit: the current last low bound to use
                                                    // to insert a new interval.
                          NormValueList*& lowBInt,  // the low and high bound in which availableRC
                          NormValueList*& hiBInt,   // #rows resides. The two bounds are used to
                                                    // compute the new high bound(s) for new intervals
                          NABoolean allowSplits);

private:
  // these two are private because we never want them used individually
  // ==> use the following routine instead
  inline void setUec (const CostScalar & value) ;
  inline void setRowcount (const CostScalar & value) ;

public:
  inline void setRowsAndUec (const CostScalar & rows,
                             const CostScalar & uec) ;
  
  inline void refreshHiInt();
  // gets/sets the lo/hi boundary values
  inline const EncodedValue& loBound() const ;

  inline const MCboundaryValueList& loMCBound() const;

  const MCboundaryValueList& hiMCBound() const
  {
     DCMPASSERT(hiInt_ == ((*hist_)[loIndex_+1]));
     return hiInt_.getMCBoundary() ;
  }

// warning elimination (removed "inline")
  const EncodedValue& hiBound() const
  {
    DCMPASSERT(hiInt_ == ((*hist_)[loIndex_+1]));
    return hiInt_.getBoundary() ;
  }

  inline void setLoBound (const EncodedValue & newLo) ;
  inline void setHiBound (const EncodedValue & newHi) ;

  // gets/sets the bounds incl values
  inline NABoolean isLoBoundInclusive() const ;
// warning elimination (removed "inline")
  NABoolean isHiBoundInclusive() const
  {
    DCMPASSERT(hiInt_ == ((*hist_)[loIndex_+1]));
    return ( hiInt_.isBoundIncl() ) ;
  }
  inline void setLoBoundInclusive (NABoolean value) ;
  inline void setHiBoundInclusive (NABoolean value) ;

  inline NABoolean isSingleValued() const { return (loBound() == hiBound()) ; }
  inline CollIndex getLoIndex() const { OK() ; return loIndex_ ; }

  inline NABoolean isNull() const { return (hiBound().isNullValue()) ; }

  // make sure we don't use Intervals that have been compromised
  inline void setInvalid() { hist_ = 0; }
  inline NABoolean isValid() const
  {
    return ( (hist_ != 0) &&
             ((loIndex_+2) <= hist_->entries()) ) ;
  }
  // was loIndex_ <= hist_->entries()-2, but we hate underflow!

  //
  // non-trivial Interval functions
  //

  // sets THIS equal to the interval that comes after/before THIS
  void next () ;
  void prev () ;
  
  // Does this interval contain a frequent value?
  NABoolean containsAFrequentValue(const CostScalar & thresholdFreq) const;

  // merge self with 'other'
  NABoolean merge (Interval & other) ;

  // is the UEC or Rowcount too low?
  // that is, greater than 0 but less than 1
  // --> returns TRUE when this is true
  NABoolean canBeMerged() const ;

  // figures out which intervals, starting with 'startInterval', are
  // spanned by THIS (NB: startInterval is most likely in another
  // histogram)
  NABoolean spans (const Interval & startInterval) const ;

  // figures out whether THIS contains 'value'
  NABoolean containsValue (const EncodedValue & value) const ;

  // removes 'value' from THIS
  // --> will probably require creating additional Intervals,
  //     in order to create the single-valued 0-uec/0-rowcount
  //     Interval
  void removeValue (const EncodedValue & value) ;

  // -----------------------------------------------------------------------
  // the following function originally was a global function because it's
  // useful in places where you don't want to call it on an Interval
  // object; however, to avoid polluting the global namespace, and to make
  // it easier to find for anyone else who wants to use it, it's now an
  // Interval method.  However, note that it does not use any state
  // information from the calling object.
  //
  // distributes uecs/rows contained between loBound & hiBound to the
  // Intervals in 'spanList'
  // -----------------------------------------------------------------------
  static void distributeRowsAndUec (LIST(Interval) & spanList,
                                    CostScalar rows,
                                    CostScalar uecs,
                                    const EncodedValue & loBound,
                                    const EncodedValue & hiBound) ;

  // -----------------------------------------------------------------------
  // methods used for histogram intervals reduction
  // -----------------------------------------------------------------------

  // compare this interval with the adjacent interval (which should be passed in via
  // other) and return TRUE if they are equal based on the Criterion passed in
  NABoolean compare(Source invokedFrom, Criterion reductionCriterion, Interval & other);

  // compare this interval with the adjacent interval (which should be passed in via
  // other) and return TRUE if they are equal based on criterion 1.
  // The definition of criterion 1 can be found in the histogram intervals reduction
  // design document
  NABoolean satisfiesCriterion1(Source invokedFrom, Interval & other);

  // compare this interval with the adjacent interval (which should be passed in via
  // other) and return TRUE if they are equal based on criterion 2.
  // The definition of criterion 2 can be found in the histogram intervals reduction
  // design document
  NABoolean satisfiesCriterion2(Source invokedFrom, Interval & other);

  // -----------------------------------------------------------------------
  //  Utility routines
  // -----------------------------------------------------------------------
  void display (FILE *f = stdout,
		const char * prefix = DEFAULT_INDENT,
		const char * suffix = "") const;

private:

#ifndef NDEBUG
  // internal consistency check -- for debugging only
  NABoolean OK() const;
#else
  inline NABoolean OK() const {return TRUE;};
#endif

  CollIndex loIndex_ ;         // which HistInt do we start with?
  HistogramSharedPtr hist_ ;
  HistInt hiInt_;            // the histogram we're interfacing with
} ;
#pragma warn(270)  // warning elimination


// get the next/prev Interval

//
// For bounds inclusive information, keep in mind the
// semantics of the HistInt isBoundIncl_ flag.  It's
// usually true, except for the zeroth HistInt (which
// means that the first interval's lower bound is
// bounds inclusive), or the lower interval in a S.V.I.,
// which means that the S.V.I. is completely bounds
// inclusive.
//
// 0    1    2    3    4
//
// 1    2    3    3    4
// <    <=   <    <=   <=
// |    |    |    |    |
// |    |    |    |    |
// |    |    |    |    |
//   I1   I2   I3   I4
//
// So, given the above HistInts :
//
//   I1 spans 1 and 2, including the boundaries 1 & 2
//   I2 spans 2 and 3, not including either boundary
//   I3 spans 3 and 3, including that boundary
//   I4 spans 3 and 4, including 4 but not 3
//

inline NABoolean
Interval::isLoBoundInclusive() const
{
  return ( NOT ((*hist_)[loIndex_]).isBoundIncl() ) ;
}



// set the inclusive boundary values

inline void
Interval::setLoBoundInclusive (NABoolean value)
{
  ((*hist_)[loIndex_]).setBoundIncl (NOT value);
}

inline void
Interval::setHiBoundInclusive (NABoolean value)
{
  HistInt & temp = ((*hist_)[loIndex_+1]);
  temp.setBoundIncl (value) ;
  hiInt_ = temp;
}

// return lo/hi bound information

inline const EncodedValue&
Interval::loBound() const
{
  return ((*hist_)[loIndex_]).getBoundary() ;
}

inline const MCboundaryValueList&
Interval::loMCBound() const
{
  return ((*hist_)[loIndex_]).getMCBoundary() ;
}


// set lo/hi boundaries

inline void
Interval::setLoBound (const EncodedValue& newLo)
{
 ((*hist_)[loIndex_]).setBoundary (newLo) ;
}

inline void
Interval::setHiBound (const EncodedValue& newHi)
{
  HistInt & temp = ((*hist_)[loIndex_+1]);
  temp.setBoundary (newHi);
  hiInt_ = temp;
}

// set uec/rowcount information

inline void
Interval::setUec (const CostScalar & value)
{
  HistInt & temp = ((*hist_)[loIndex_+1]);
  temp.setUec (value) ;
  hiInt_ = temp;
}

inline void
Interval::setRowcount (const CostScalar & value)
{
  HistInt & temp = ((*hist_)[loIndex_+1]);
  temp.setCardinality (value) ;
  hiInt_ = temp;
}

inline void
Interval::setRowsAndUec (const CostScalar & rows,
                         const CostScalar & uec)
{
  HistInt & temp = ((*hist_)[loIndex_+1]);
  temp.setCardAndUec (rows,uec) ;
  hiInt_ = temp;
}

// is this the first/last interval?

inline NABoolean
Interval::isFirst() const
{ OK() ; return ( loIndex_ == 0 ) ; }

inline NABoolean
Interval::isLast() const
{ OK() ; return ( loIndex_+2 == hist_->entries() ) ; }

// refresh HiInt
inline void
Interval::refreshHiInt()
{ hiInt_ = ((*hist_)[loIndex_+1]); }
//
// histogram member functions that interface directly with
// the Interval member functions
//


inline Interval
Histogram::getFirstInterval() const
{
  if ( numIntervals() == 0 )
    return Interval() ; // 0 intervals ==> no first exists

  HistogramSharedPtr histPtr = HistogramSharedPtr::getIntrusiveSharedPtr(this);
  return Interval(0,histPtr);
}

inline Interval
Histogram::getLastInterval() const
{
  if ( numIntervals() == 0 )
    return Interval() ; // 0 intervals ==> no last exists

  HistogramSharedPtr histPtr = HistogramSharedPtr::getIntrusiveSharedPtr(this);
  return Interval(entries()-2,histPtr);
}

inline Interval
Histogram::getLastNonNullInterval() const
{
  if ( isNullInstantiated() )
    {
      if ( entries() == 2 )
        return Interval() ; // there is only that single, NULL interval

      HistogramSharedPtr histPtr = HistogramSharedPtr::getIntrusiveSharedPtr(this);
      return Interval (entries()-4,histPtr);
    }
  else
    {
      return getLastInterval() ;
    }
}

inline Interval
Histogram::getInterval (CollIndex index) const
{
  if ( ! (1 <= index && index <= numIntervals()) ) // make sure this function
    return Interval() ;                            // is used properly

  HistogramSharedPtr histPtr = HistogramSharedPtr::getIntrusiveSharedPtr(this);
  return Interval(index-1,histPtr);
}

// this special histogramid is checked in ColStats::ColStats 
// constructor to determine whether a histogram is fake. 
const Int32 FAKEHISTOGRAMID=10000;

// -----------------------------------------------------------------------
//  Column Statistics
//
//  The following class represents statistics for an individual
//  column or set of columns.  The column(s) are identified by an
//  NAColumnArray.  This class contains a reference to histogram statistics,
//  as well as aggregated column statistics.  For some column(s), it
//  may be possible to have only the aggregated column(s) statistics and
//  no histogram statistics.
// -----------------------------------------------------------------------
class ColStats : public NABasicObject
{
  static THREAD_P Int64 fakeHistogramIDCounter_;
public:
  // special fake histogramids are above this value, but,
  // histogramIDs generated by update statistics
  // will be less than this 
  static const Int32 USTAT_HISTOGRAM_ID_THRESHOLD=0x7FFFFFFF;
  static ComUID nextFakeHistogramID();
  static NABoolean isUSTATGeneratedHistID(ComUID id);

  // -----------------------------------------------------------------------
  //  Constructors
  // -----------------------------------------------------------------------
  ColStats (ComUID & histid, CostScalar uec = 0, CostScalar rowcount = 0,
            CostScalar baseRowCount = -1,
	    NABoolean unique = FALSE, NABoolean shapeChanged = FALSE,
	    const HistogramSharedPtr& desc = 0, NABoolean modified = FALSE,
	    CostScalar rowRedFactor = 1.0, CostScalar uecRedFactor = 1.0,
                 Int32 avgVarcharSize = 0,  
	    NAMemory* heap=0, NABoolean allowMinusOne=FALSE);

  // copy constructor
  ColStats (const ColStats &other, NAMemory* h, NABoolean assignColArray=TRUE);

  // sometimes we want an uninitialized ColStats object
  ColStats (const HistogramSharedPtr& hist, NAMemory* h) :
       histogram_(hist), columns_(h), heap_(h), histogramID_(0),
       minValue_(UNINIT_ENCODEDVALUE),
       maxValue_(UNINIT_ENCODEDVALUE),
    frequentValues_(h), colPositions_(h), mcSkewedValueList_(h)
  {
    rowcount_     = totalUec_     = baseUec_ = sumOfMaxUec_ = uecBeforePred_ = 0 ;
    baseRowCount_ = -1;
    rowRedFactor_ = uecRedFactor_ = 1 ;
    maxIntervalCount_ = 0 ;
	maxFreq_ = -1.0;

    avgVarcharSize_ = 0;
    afterFetchIntReductionAttempted_ = FALSE;

    //NB: flags' values *must* be set by set* functions
    setUnique        (FALSE) ;
    setAlmostUnique  (FALSE);
    setModified      (FALSE) ;
    setShapeChanged  (FALSE) ;
    setFakeHistogram (TRUE)  ;
    setOrigFakeHist  (FALSE) ;
    setSmallSampleHistogram (FALSE);
    setMinSetByPred  (FALSE) ;
    setMaxSetByPred  (FALSE) ;
    setRecentJoin    (FALSE) ;
    setUpStatsNeeded (FALSE) ;
    setVirtualColForHist   (FALSE) ;
    setSmallSampleHistogram(FALSE) ;
    setIsCompressed  (FALSE);
    setIsARollingColumn (FALSE);
    setIsColWithBndryConflict (FALSE);
    setSelectivitySetUsingHint (FALSE);
  }
// deepCopy()
// Creates a new ColStats and makes a deepCopy of it using other
static ColStatsSharedPtr deepCopy(const ColStats& other, NAMemory * heap,
                                  NABoolean useColumnPositions=FALSE,
                                  NABoolean copyIntervals=TRUE);

// copy histogram into cache. 
// use "lean" representation of columns.
static ColStatsSharedPtr deepCopyHistIntoCache
  (const ColStats& other, NAMemory * heap)
  { return deepCopy(other, heap, TRUE); }

// creates a deep copy of single-column histogram from cache.
// sets deep copy's column to col.
static ColStatsSharedPtr deepCopySingleColHistFromCache
  (const ColStats& other, NAColumn& col, NAMemory * heap, 
   NABoolean copyIntervals);

private:
  // we prevent people from creating ColStats that aren't explicitly put
  // on a heap! code doing this will not link!
  ColStats() ;
  ColStats (const ColStats &other) ;
public:

  // -----------------------------------------------------------------------
  //  Destructor
  // -----------------------------------------------------------------------
  virtual ~ColStats();
  virtual void deepDelete();
  void deepDeleteFromHistogramCache();
  // ---------------------------------------------------------------------
  //  Accessor functions
  // ---------------------------------------------------------------------
  inline const  NAColumnArray & getStatColumns () const { return columns_     ; }
  inline HistogramSharedPtr getHistogram       () const { return histogram_   ; }
  inline const ComUID& getHistogramId          () const {return histogramID_;}
  inline const NABoolean isSingleIntHist() 
    { return getHistogram() ? (getHistogram()->entries()<=2) : TRUE; }
  inline const  EncodedValue & getMinValue     () const { return minValue_    ; }
  inline const  EncodedValue & getMaxValue     () const { return maxValue_    ; }

  inline CollIndex getMaxIntervalCount         () const { return maxIntervalCount_ ; }

  const FrequentValueList &getFrequentValues() const { return frequentValues_; }
  FrequentValueList &getModifableFrequentValues() { return frequentValues_; }
  void setFrequentValue(const FrequentValueList &newFrequentValue) {frequentValues_ = newFrequentValue;}

  const MCSkewedValueList &getMCSkewedValueList() const { return mcSkewedValueList_; }
  void addMCSkewedValue(const NAWchar * boundary, CostScalar frequency);
  void setMCSkewedValueList(const MCSkewedValueList &mcSkewedValueList) {mcSkewedValueList_ = mcSkewedValueList;}

  // since we're careful about only setting >= 0 values for these numbers,
  // we don't have to do a check here
  inline CostScalar getRowcount     () const { return rowcount_         ; }
  inline CostScalar getTotalUec     () const { return totalUec_         ; }
  inline CostScalar getSumOfMaxUec  () const { return sumOfMaxUec_      ; }
  inline CostScalar getRedFactor    () const { return rowRedFactor_     ; }
  inline CostScalar getUecRedFactor () const { return uecRedFactor_     ; }
  inline CostScalar getBaseUec      () const { return baseUec_          ; }
  inline CostScalar getBaseRowCount () const { return baseRowCount_     ; }
  inline CostScalar getUecBeforePreds () const { return uecBeforePred_  ; }

  // report the status of various flags
  inline NABoolean isUnique	      () const { return (_flags_ & _unique_) != 0	     ; }
  inline NABoolean isModified	      () const { return (_flags_ & _modified_) != 0	     ; }
  inline NABoolean isShapeChanged     () const { return (_flags_ & _shapeChanged_) != 0	     ; }
  inline NABoolean isFakeHistogram    () const { return (_flags_ & _isFakeHistogram_) != 0   ; }
  inline NABoolean isObsoleteHistogram() const { return (_flags_ & _isObsoleteHistogram_)!= 0; }
  inline NABoolean isOrigFakeHist     () const { return (_flags_ & _isOrigFakeHist_) != 0    ; }
  inline NABoolean isMinSetByPred     () const { return (_flags_ & _minSetByPred_) != 0	     ; }
  inline NABoolean isMaxSetByPred     () const { return (_flags_ & _maxSetByPred_) != 0	     ; }
  inline NABoolean isRecentJoin	      () const { return (_flags_ & _recentJoin_) != 0	     ; }
  inline NABoolean isUpStatsNeeded    () const { return (_flags_ & _updateStatsNeeded_) != 0 ; }
  inline NABoolean isVirtualColForHist() const { return (_flags_ & _virtualColForHist_) != 0 ; }
  inline NABoolean isAlmostUnique     () const { return (_flags_ & _almostUnique_) != 0	     ; }
  inline NABoolean isSmallSampleHistogram() const { return (_flags_ & _isSmallSampleHistogram_) != 0; }
  inline NABoolean isCompressed       () const { return (_flags_ & _isCompressed_) != 0      ; }
  inline NABoolean isARollingColumn   () const { return (_flags_ & _isARollingColumn_) != 0      ; }
  inline NABoolean isColWithBndryConflict () const { return (_flags_ & _isColWithBndryConflict_) != 0 ; }
  inline NABoolean isSelectivitySetUsingHint () const { return (_flags_ & _selectivitySetUsingHint_) != 0 ; }
  inline NABoolean isMCforHbasePartitioning () const { return (_flags_ & _mcForHbasePartitioning_) != 0 ; }


  Int32 getAvgVarcharSize() const { return avgVarcharSize_; };
  // ---------------------------------------------------------------------
  //  Mutator functions
  // ---------------------------------------------------------------------

  // return a pointer to the object so you can modify it
  inline NAColumnArray & statColumns() { return columns_; }
  HistogramSharedPtr getHistogramToModify();
  ColumnSet& getStatColumnPositions() { return colPositions_; }

  // populate NAColumnArray with this ColumnSet
  void populateColumnArray(const ColumnSet& cols, const NATable* table);

  // populate my NAColumnSet from my ColumnArray
  void populateColumnSetFromColumnArray();

  void createAndAddSkewedValue(const wchar_t *boundary, Interval &iter);
  void createAndAddFrequentValue(const wchar_t *boundary, Interval &iter);

  NABoolean mergeFrequentValues(ColStatsSharedPtr& otherStats,
                                NABoolean scaleFreq = TRUE,
                                MergeType mergeMethod = INNER_JOIN_MERGE,
                                NABoolean adjRowCount = FALSE);

  NABoolean getTotalFreqInfoForIntervalWithValue(EncodedValue mfvEV, 
                                                 CostScalar & totalMfvRc,
                                                 CostScalar &mfvCnt) ;

  void createFakeHist();

  void compressToSingleInt();

  // setMinValue
  inline void setMinValue (const NAWchar *theValue)
                         { minValue_ = EncodedValue (theValue, getStatColumns()) ; }
  inline void setMinValue (const EncodedValue & minValue) { minValue_ = minValue ; }
  // setMaxValue
  inline void setMaxValue (const NAWchar *theValue)
                         { maxValue_ = EncodedValue (theValue, getStatColumns()) ; }
  inline void setMaxValue (const EncodedValue & maxValue) { maxValue_ = maxValue ; }

  CostScalar getMaxFreq() const;

  // This method returns the total row count and total UEC of intervals
  // whose frequency is greater than or equal to the threshold value
  void getAccRowCountAboveOrEqThreshold (CostScalar & accRowCnt, /* out */
										 CostScalar & accUec,   /* out */
										 CostScalar thresVal = 1.0); /* in optional */

  CostScalar getScaleFactor() const {return scaleFactor_; }

  void computeMaxFreqOfCol(NABoolean forced = FALSE);

  void setMaxFreq(CostScalar freq);

  void setScaleFactor(CostScalar scale)
  { scaleFactor_ = scale; }

  CostScalar getAdjContinuumUEC() const {return adjContinuumUEC_; } ;
  CostScalar getAdjContinuumFreq() const {return adjContinuumFreq_; };

  void setAdjContinuumUEC(CostScalar adjContinuumUEC) { adjContinuumUEC_ = adjContinuumUEC.minCsZero(); }
  void setAdjContinuumFreq(CostScalar adjContinuumFreq) { adjContinuumFreq_ = adjContinuumFreq.minCsZero(); }

  // setHistogram
  inline void setHistogram (const HistogramSharedPtr& dist)   { histogram_ = dist ; }

  // insert zero interval with boundaries equal to min and max and rows and uec from the colstats
  void insertZeroInterval();

  // setMaxIntervalCount() -- this value cannot be less than 2
  // NB: The only place where this fn should be called is in NATable::getStatistics()
  inline void setMaxIntervalCount(CollIndex number)
                                         { maxIntervalCount_ = MAXOF (number, 2) ; }

  // set the rowcount/totalUec values by examining the underlying Histogram
  void setRowsAndUecFromHistogram() ;
  // set the min/min values by examining the underlying Histogram
  void setMaxMinValuesFromHistogram() ;

  // set the columns_ of the Histogram. These are presently being used by the
  // GenericUpdate, when the histograms are created for the top columns of the
  // Update.

  void setStatColumn (NAColumn * column);

  // setting the impt aggregate values -- all of which should always be >= 0 !
  // (and if they're really close to zero, e.g., 1e-16, round them to zero)
private:
  // these two are private because we never want them used individually
  // ==> use the ::setRowsAndUec() routine instead
// warning elimination (removed "inline")
  void setRowcount (CostScalar row);

// warning elimination (removed "inline")
  // We have added the allowMinusOne flag to allow uecs to be 
  // initialized to minusOne. This is only used for UDFs, and the minusOne
  // is used to indicate that we do not have valid UEC information for a 
  // particular output. The UDF costing code will look for the minusOne flag 
  // as an indicator that it needs to compute an UEC from the Functions inputs 
  // as a fallback.
  void setTotalUec (CostScalar uec, NABoolean allowMinusOne = FALSE);

public:
  // the following is used to maintain the semantic of uec <= rows
  // See comment above as to the use of the allowMinusOne flag. 
  void setRowsAndUec (CostScalar rows, CostScalar uec, NABoolean allowMinusOne=FALSE) ;

// warning elimination (removed "inline")
  void setBaseRowCount (CostScalar row);

  void setBaseUec(CostScalar uec);

  // the following is used to store the sum-of-max-uec-per-interval value in
  // mergeColStats, for later perusal/resetting in estimateCardinality
  void setSumOfMaxUec (CostScalar value);

  // the following is used to store the base UEC for that column before applying
  // predicate to it.
  void setUecBeforePred (CostScalar value)
                                  { uecBeforePred_ = value ; }

  // we have to be extremely careful about rounding the reduction factors
  // because they can legitimately become very close to zero but not equal
  // to zero (e.g., join between 2 1-billion row tables returns 1 row ==>
  // redfactor == 1e-18)
// warning elimination (remove "inline")
  void setRedFactor (CostScalar rowred);

// warning elimination (removed "inline")
  void setUecRedFactor (CostScalar uecred);

  // setting the flags
  inline void setUnique		   (NABoolean flag)	   { flag ? _flags_ |= _unique_		     : _flags_ &= ~_unique_;	         }
  inline void setModified	   (NABoolean flag = TRUE) { flag ? _flags_ |= _modified_	     : _flags_ &= ~_modified_;	         }
  inline void setShapeChanged	   (NABoolean flag = TRUE) { flag ? _flags_ |= _shapeChanged_	     : _flags_ &= ~_shapeChanged_;       }
  inline void setFakeHistogram	   (NABoolean flag = TRUE) { flag ? _flags_ |= _isFakeHistogram_     : _flags_ &= ~_isFakeHistogram_;    }
  inline void setObsoleteHistogram (NABoolean flag = TRUE) { flag ? _flags_ |= _isObsoleteHistogram_ : _flags_ &= ~_isObsoleteHistogram_;}
  inline void setOrigFakeHist	   (NABoolean flag = TRUE) { flag ? _flags_ |= _isOrigFakeHist_	     : _flags_ &= ~_isOrigFakeHist_;     }
  inline void setMinSetByPred	   (NABoolean flag)        { flag ? _flags_ |= _minSetByPred_	     : _flags_ &= ~_minSetByPred_;       }
  inline void setMaxSetByPred	   (NABoolean flag)        { flag ? _flags_ |= _maxSetByPred_	     : _flags_ &= ~_maxSetByPred_;       }
  inline void setRecentJoin	   (NABoolean flag = TRUE) { flag ? _flags_ |= _recentJoin_	     : _flags_ &= ~_recentJoin_;	 }
  inline void setUpStatsNeeded	   (NABoolean flag)        { flag ? _flags_ |= _updateStatsNeeded_   : _flags_ &= ~_updateStatsNeeded_;  }
  inline void setVirtualColForHist (NABoolean flag = TRUE) { flag ? _flags_ |= _virtualColForHist_   : _flags_ &= ~_virtualColForHist_;  }
  inline void setAlmostUnique	   (NABoolean flag)        { flag ? _flags_ |= _almostUnique_        : _flags_ &= ~_almostUnique_;       }
  inline void setSmallSampleHistogram (NABoolean flag = TRUE) { flag ? _flags_ |= _isSmallSampleHistogram_ : _flags_ &= ~_isSmallSampleHistogram_; }
  inline void setIsCompressed      (NABoolean flag = TRUE) { flag ? _flags_ |= _isCompressed_        : _flags_ &= ~_isCompressed_;       }
  inline void setIsARollingColumn  (NABoolean flag = TRUE) { flag ? _flags_ |= _isARollingColumn_    : _flags_ &= ~_isARollingColumn_;       }
  inline void setIsColWithBndryConflict (NABoolean flag = TRUE) { flag ? _flags_ |= _isColWithBndryConflict_ : _flags_ &= ~_isColWithBndryConflict_; }
  inline void setSelectivitySetUsingHint (NABoolean flag = TRUE) { flag ? _flags_ |= _selectivitySetUsingHint_ : _flags_ &= ~_selectivitySetUsingHint_; }
  inline void setMCforHbasePartitioning (NABoolean flag = TRUE) { flag ? _flags_ |= _mcForHbasePartitioning_ : _flags_ &= ~_mcForHbasePartitioning_; }
  
  // a minor variation on a THIS = OTHER assignment operator
  void overwrite (const ColStats &other) ;

  // ---------------------------------------------------------------------
  //  Comparison of two column statistics :
  //  Column statistics are equivalent if they are on the same set of cols
  // ---------------------------------------------------------------------
  inline NABoolean operator== (const ColStats & other)
  { return (this == &other) ; }

  // ---------------------------------------------------------------------
  // NULL handling routines
  // --> trying to make the handling of NULLs a little easier ...
  // ---------------------------------------------------------------------

  // is there a NULL interval in the Histogram?
// warning elimination (removed "inline")
  NABoolean isNullInstantiated () const ;

  // removing that NULL interval, if it exists
// warning elimination (removed "inline")
  void removeNullInterval () ;

  // inserting a NULL interval, if it doesn't already exist
// warning elimination (removed "inline")
  void insertNullInterval () ;

  // reporting the number of NULLs / NULL-uecs in that interval
// warning elimination (removed "inline")
  CostScalar getNullCount () const ;
// warning elimination (removed "inline")
  CostScalar getNullUec ()   const ;

  // setting the number of NULLs and NULL-uecs in that interval
 // warning elimination (removed "inline")
  void setNullRowsAndUec (CostScalar nulls, CostScalar nullUec) ;

  // ---------------------------------------------------------------------
  //  Histogram Manipulation routines
  // ---------------------------------------------------------------------
  // Following operations such as joins a histogram may have a series of
  // intervals containing zero rows.  In that situation, compress out the
  // redundant empty histogram intervals.
  void removeRedundantEmpties() ;

  void modifyStats (ItemExpr * pred, CostScalar *maxSelectivity=NULL);

  void simplestPreds (ItemExpr * pred) ;

  // Synthesize the effect of column <(=) upBound
  void newUpperBound (const EncodedValue & upBound, ConstValue* constExpr, NABoolean boundIncl);


  // Synthesize the effect of column >(=) lowBound
  void newLowerBound (const EncodedValue & lowBound, ConstValue* constExpr, NABoolean boundIncl);
  // Synthesize the effect of an equality predicate against a constant that
  // covers all columns of the histogram:
  // i.e. reduce the histogram to a single, single-valued, interval.
  void setToSingleValue (const EncodedValue & newValue, ConstValue* constExpr,
                         CostScalar *totalRows=NULL, FrequentValue* fv=NULL);

  // a helper function called by ::setToSingleValue() and ::isNull()
  //
  // does the work of removing all HistInts and adding 2 so that
  // the resulting ColStats has the corresponding min/max values
  void setToSingleInterval (const EncodedValue & newLoBound,
                            const EncodedValue & newUpBound,
                            CostScalar numRows,
                            CostScalar numUecs) ;

  // Synthesize the effect of column NOT= newValue
  void removeSingleValue (const EncodedValue & newValue, ConstValue* consExpr);

  // THIS contains a histogram template (created by createMergeTemplate());
  // adjust this template to have uec/row contents equivalent to
  // those in 'other.'
  void populateTemplate(const ColStatsSharedPtr& other) ;

  // NB: this used to be a Histogram member function named
  // 'adjustBoundaries', but this didn't describe what the function does

  // Synthesize the effect of
  //         IS [NOT] NULL and IS [NOT] UNKNOWN
  void isNull (NABoolean notFlag);

  // Do the work of clearing the Histogram and nullifying the
  // aggregate information
  void clearHistogram() ;

  // Finds where in the list of HistInts to place the new HistInt.  Then,
  // divides the rows/uecs from the divided Interval into the two new
  // Intervals (or, if this HistInt boundary already exists, jumps to next
  // step).
  //
  // Finally, removes the intervals above or below the indentified interval
  // boundary.  That is, for < operations, we remove all Intervals above
  // this one; for > ops, we destory all Histints below it.
  void divideHistogramAlongBoundaryValue(const EncodedValue & value,
                                         OperatorTypeEnum splitOperator) ;

  // simple helper functions that prune the Histogram above/below
  // the given boundary Interval; also, sets the s-c flag if
  // anything's actually deleted
  //
  // NB: the second of these invalidates the parameter Interval
  void deleteIntervalsAbove (const Interval & boundary) ;
  void deleteIntervalsBelow (Interval & boundary) ;

  void populateTemplateOfFakeHist(const ColStatsSharedPtr& fakeHistogram,
                                  const ColStatsSharedPtr& realHistgoram);

  // Perform an inner- or semi-join equality predicate, or an 'OR' union
  //   operation on two ColStats, updating THIS with the result.
  void mergeColStats (const ColStatsSharedPtr& otherStats,
                      MergeType mergeMethod, NABoolean isNumeric, 
                      OperatorTypeEnum exprOpCode,
                      NABoolean mergeFVs=TRUE) ;

  // ------------------------------------------------------------
  // minSetByPred_ , maxSetByPred_ flags indicate if the boundaries
  // for the histograms were set by application of predicates. The values
  // for these flags for the target merged histogram are calculated below
  // ------------------------------------------------------------
  void setMaxAndMinSetByPredFlags (const ColStatsSharedPtr& otherStatsCopy,
				       NABoolean &maxSetByPred,
				       NABoolean &minSetByPred);
  // ---------------------------------------------------------------------
  // graceful recovery in case of any error while merging two histograms, 
  // ---------------------------------------------------------------------
  void recoverFromMergeColStats(const ColStatsSharedPtr& otherStats,
			        NABoolean isNumeric,
			        MergeType mergeMethod = INNER_JOIN_MERGE);

  // ---------------------------------------------------------------------
  // The join cardinality can be computed either by merging histogram
  // intervals or merging frequent value lists. In this method we compute
  // join cardinality using histogram intervals
  // ----------------------------------------------------------------------
  CostScalar mergeWithExpandedHistograms (const ColStatsSharedPtr& otherStats,
			                                    NABoolean isNumeric,
				                                  CostScalar & newRowcount, 
				                                  CostScalar & newUec,
			                                    MergeType mergeMethod = INNER_JOIN_MERGE);


  // ---------------------------------------------------------------------
  // The join cardinality can be computed either by merging histogram
  // intervals or merging frequent value lists. In this method we compute
  // join cardinality using frequent values
  // ----------------------------------------------------------------------
  CostScalar mergeCompressedHistograms (const ColStatsSharedPtr& otherStats,
				                                CostScalar & newRowcount, 
				                                CostScalar & newUec,
			                                  MergeType mergeMethod = INNER_JOIN_MERGE);

  // --------------------------------------------------------------------
  // adjust selectivity computed by either merging histogram intervals
  // or frequent value lists to take into account any indirect reductions
  // ---------------------------------------------------------------------
  CostScalar adjustSelectivity(const ColStatsSharedPtr& otherStats,
                               const CostScalar & newUec,
			                         MergeType mergeMethod = INNER_JOIN_MERGE);


  // ------------------------------------------------------------------------
  // populate left and right histogram templates created for merge. The
  // histograms will be populated based on if the stats exist for both 
  // children or not
  // ------------------------------------------------------------------------
  NABoolean populateLeftAndRightTemplates(const ColStatsSharedPtr & otherStatsCopy,
					  HistogramSharedPtr & leftHistogram, 
					  HistogramSharedPtr & rightHistogram,
					  HistogramSharedPtr & targetHistogram);

  // -----------------------------------------------------------------------
  // Histogram intervals reduction routines
  // -----------------------------------------------------------------------

  // calculates the reduction criterion to apply for reduction of the
  // number of histogram intervals
  Criterion decideReductionCriterion(Source invokedFrom,
                                     Criterion reductionCriterion,
                                     const NAColumn * column,
                                     NABoolean ignoreHistogramCachingFlag=FALSE);

  // tracks whether a reduction of histograms after fetch histograms was attempted on this colStats
  NABoolean afterFetchIntReductionAttempted () {return afterFetchIntReductionAttempted_; }
  void setAfterFetchIntReductionAttempted () { afterFetchIntReductionAttempted_ = TRUE;}

  //reduce the number of histogram intervals in the histogram
  //referenced by this ColStats object. The reduction criterion
  //depends on parameters invokedFrom, and reductionCriterion
  void reduceNumHistInts(Source invokedFrom,
                         Criterion reductionCriterion);

  // -----------------------------------------------------------------------
  // This utility routine is used by costing to determine the number of
  // key predicate 'probes' performed during a Nested Join which did not
  // produce any result rows.
  // THIS provides the ColStats of the appropriate columns in the Input
  // EstLogProp;  otherStats provides the result of the key predicate join
  // done with the base table.   An INNER Join is assumed.
  // -----------------------------------------------------------------------
  CostScalar countFailedProbes(const ColStatsSharedPtr& otherStats) const;

  // -----------------------------------------------------------------------
  // in the given ColStats, replace the current histogram with a copy that
  // has had all of its interval's rowcounts multiplied by the specified
  // scale.
  // -----------------------------------------------------------------------
  void copyAndScaleHistogram(CostScalar scale) ;

  // -----------------------------------------------------------------------
  // Just replace the current histogram buckets with values which have the
  // reduction factors applied.  An additional scale can be applied.
  //
  // Also, if any intervals have rows<1, rows>0, we merge these with their
  // neighbors.
  // -----------------------------------------------------------------------
  void scaleHistogram (CostScalar scale, CostScalar uecScale = 1, NABoolean scaleFreqValueList = TRUE) ;

  // -----------------------------------------------------------------------
  // Increase the rowcount by adding some NULLs; specifically, adding
  //       targetRowCount - rowcount_ NULLs
  // -----------------------------------------------------------------------
  void nullAugmentHistogram(CostScalar targetRowCount);

  // -----------------------------------------------------------------------
  // Set row counts to match the UECs.
  // i.e., unique-ify the statistics on this column
  // -----------------------------------------------------------------------
  void makeGrouped();


  // compress the ColStats for local predicates on a column
  // The local predicates should involve a constant
  // e.g.
  // * t1.col1 = 3
  // * t1.col1 < 3
  // * t1.col1 > 1
  // * t1.col1 > 1 and t1.col1 < 3
  // * t1.col1 = t2.col1 // i.e. join predicate
  void compressColStatsForQueryPreds(ItemExpr * lowerBound,
                                     ItemExpr * upperBound,
                                     NABoolean  hasJoinPred=FALSE);

  // -----------------------------------------------------------------------
  // The user has set a value in the defaults table
  // <HIST_MAX_NUMBER_OF_INTERVALS> to specify the maximum number of
  // intervals that he wants.  This value can be used to test the
  // tradeoffs between compile-time and rowcount estimation accuracy --
  // because histograms with large numbers of intervals are believed to
  // slow down compilation speed significantly.
  //
  // This routine goes through the Histogram and merges intervals as
  // necessary to get to this upper bound (the data member maxIntervalCount_
  // stores the value from the defaults table).
  // -----------------------------------------------------------------------
  void reduceToMaxIntervalCount() ;

  // 
  // This routine goes through the Histogram and split/merge intervals as
  // necessary to evenly distribute the # of rows into numOfIntervals 
  // intervals. 
  // 
  // For column types that allow 1 to 1 mapping to the
  // encoded value (as double), the split point can be one of the following
  //
  //   1. one or more values before the most frequent value (MFV)
  //   2. the most frequent value (MFV)
  //   3. one or more values after the most frequent value (MFV)
  //
  // Otherwise, only two types of splitting are possible.
  //   1. the most frequent value (MFV)
  //   2. one or more values after the most frequent value (MFV)

  HistogramSharedPtr transformOnIntervals(Int32 numOfIntervals) ;
  HistogramSharedPtr transformOnIntervalsForMC(Int32 numOfIntervals) ;

  //Helper method to adjust Rowcount for rolling columns
  void adjustRowcountforRollingColumns(ConstValue * constant);

  // Adjust the max selectivity via the following steps:
  // 1. If 'value' is in the frequentvaluelist, update the max card with it;
  // 2. Otherwise, find the interval for 'value', update the max card with
  //    the MAX of the 2nd most frequent value, and the average rowcount
  //    in the interval.
  // The seach is using either the normValue or constExpr argument 
  // depending on the data types of the columns on which the frequent
  // list is built. The search chooses an argument that will yield the
  // best resolution. 
  void adjustMaxSelectivity(const EncodedValue& normValue,
                            ConstValue* constExpr,
                            CostScalar *totalRows,
                            CostScalar *maxSelectivity);

  // -----------------------------------------------------------------------
  //  Utility functions
  // -----------------------------------------------------------------------

  void display() const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "",
              CollHeap *c=NULL, char *buf=NULL,
              NABoolean hideDetail=FALSE) const;

  void trace (FILE *f, NATable* table);

  INTRUSIVE_SHARED_PTR(ColStats);

private:
  // -----------------------------------------------------------------------
  // When one, or both, of the two to-be-combined column statistics has no
  // histogram it is still possible to (sometimes) create a useful result
  // histogram.  This private utility routine attempts to deal with that
  // case.
  // -----------------------------------------------------------------------
  void mergeWithEmptyHistogram (const ColStatsSharedPtr& otherStats,
				MergeType mergeMethod) ;

  // -----------------------------------------------------------------------
  // This is a helper method used by mergeColStats() to recover from merge
  // template that consists of zero intervals.
  // -----------------------------------------------------------------------
  NABoolean handleMergeTemplateWithZeroIntervals(const ColStatsSharedPtr& otherStats, 
						  HistogramSharedPtr& leftHistogram);

  // -----------------------------------------------------------------------
  // This is a helper method for determining if the merge is for a join
  // -----------------------------------------------------------------------
  NABoolean isAJoinRelatedMerge(MergeType mergeMethod) const;

  // -----------------------------------------------------------------------
  // This is a helper method for reducing intermediate histograms
  // -----------------------------------------------------------------------
  void reduceIntermediateHistInts(MergeType mergeMethod, NABoolean isNumeric);

// ----------------------------------------------------------------------
// attributes:
// ----------------------------------------------------------------------
  NAColumnArray columns_;  // columns for which we have statistics

  ColumnSet colPositions_; 

  // NB: most of the time this array contains only a SINGLE NAColumn *,
  // since the histogram manipulation code does not (and probably never
  // will, due to the complexity) support multicolumn histogram synthesis.
  //
  // ==> however, we can't turn this field into a NAColumn* because class
  // StatsList uses this class to store multicolumn statistics information
  // that it's read from the catalog tables; later on, it's converted into
  // multicolumn uec information; see the files:
  //
  // * NATable.cpp, NATable::getStatistics()
  // here the multicolumn uec information is filtered out, but not
  // before it's stored into two lists inside the StatsList object so
  // that it can be used later
  //
  // * TableDesc.cpp, TableDesc::getColStats()
  // here's where the two StatsList lists containing the multicolumn
  // uec information is converted into a MultiColumnUecList
  //
  // * ColStatDesc.cpp
  // this file contains the class MultiColumnUecList, which is a (read-only)
  // datamember of ColStatDesc

  // the underlying histogram
  ComUID histogramID_;           // ID of histogram
  HistogramSharedPtr histogram_; // the histogram itself

  //reduction factors used for efficiency
  CostScalar uecRedFactor_;  // uec reduction factor
  CostScalar rowRedFactor_;  // reduction factor: if <> 1, need to multiply
  //                         // each interval's rowcount by redFactor

  //Field for refining selectivity
  CostScalar baseUec_;       // base uec for object (initial uec or
  //                         // direct manipulation count

  CostScalar uecBeforePred_;  // uec before applying predicates. This would be different
			      // than base UEC for columns on which the predicate is
			      // being applied

  CostScalar baseRowCount_;  // base rowcount (initial row count)
  CostScalar sumOfMaxUec_; // during a join, we sum up the max uec per
  //                       // interval; used in the rowcount adjustment
  //                       // when we use multicolumn uec info

  // the four aggregate values
  CostScalar totalUec_ ;     // total unique entry count for object
  CostScalar rowcount_ ;     // total rowcount
  EncodedValue minValue_ ;   // lower bound (in encoded format)
  EncodedValue maxValue_ ;   // upper bound (in encoded format)
  CostScalar maxFreq_;		 // max frequency of any calue in this colStats
  CostScalar scaleFactor_;  // used to adjust the maximum frequency to take care
							 // of the cartesian product factor

  MCSkewedValueList mcSkewedValueList_; // List of skewed values for MC histograms
  FrequentValueList frequentValues_; // List of skewed values
  CostScalar adjContinuumUEC_;
  CostScalar adjContinuumFreq_;

  //A bit map to represent all the boolean members used in ColStats
  UInt32 _flags_;

  // These following enum values represent the boolean switches used by ColStats class
  enum Flags                                           
  {
    _unique_                     = 0x00000001,    // uniqueness constraint; not used much
    _modified_                   = 0x00000002,    // indicates that the referenced Histogram has not yet been modified
    _shapeChanged_               = 0x00000004,    // Has the identified histogram changed its shape?
    _isFakeHistogram_            = 0x00000008,    // Is histogram fake still/originally due to update stats?
    _isOrigFakeHist_             = 0x00000010,    // Is histogram originally fake due to update stats?
    _minSetByPred_               = 0x00000020,    // lower bound enforced due to predicate
    _maxSetByPred_               = 0x00000040,    // upper bound enforced due to predicate
    _recentJoin_                 = 0x00000080,    // is this Histogram the result of a (very) recent Join?
                                                  // (flag set in mergeColStats, checked/unset in estCard)
    _virtualColForHist_          = 0x00000100,    // Flag set if the histogram is created for a virtual column such as 
                                                  // that for Transpose expression or Rowset
    _updateStatsNeeded_          = 0x00000200,    // Is this histogram's rowcount so large that it should have its
                                                  // statistics updated?  (This histogram may or may not be fake.)  This
                                                  // flag's value comes from the number of rows in the histogram and the
                                                  // defaults-table value HIST_ROWCOUNT_REQUIRING_STATS.
    _almostUnique_               = 0x00000400,    // This flag indicates uniqueness based on stats
    _isObsoleteHistogram_        = 0x00000800,    // Is histogram obsolete in comparison with other histograms for the same table?
    _isSmallSampleHistogram_     = 0x00001000,    // Is histogram from a small sample (compile time stats).
    _isCompressed_               = 0x00002000,    // FALSE means that the histogram has 
    _isARollingColumn_           = 0x00004000,    // Flag to set if the histogram represents a rolling column.
    _isColWithBndryConflict_     = 0x00008000,    // Flag to indicate that there was a conflicting interval boundary in the
                                                  // histogram as a result the intervals were merged
    _selectivitySetUsingHint_    = 0x00010000,    // User-specified selectivity was set for this histogram
    _mcForHbasePartitioning_     = 0x00020000     // used by hbase to partition data
  };

  CollIndex maxIntervalCount_; // maximum # of intervals the user wants (this can
  //                           // be used to compare tradeoffs in compile-time &
  //                           // rowcount estimation accuracy).

  Int32 avgVarcharSize_;         // average number of chars in columns[0], if
                               // it is of VARCHAR type. 

  NABoolean afterFetchIntReductionAttempted_; // was an attempted made after histogram fetch 
                                              // to reduce the number of intervals for this colStats

  NAMemory* heap_;           // the NAMemory* for dynamic allocation.
}; // ColStats

// ----------------------------------------------------------------------
// the inline NULL-handling ColStats methods
// ----------------------------------------------------------------------

inline NABoolean
ColStats::isNullInstantiated() const
{ return histogram_->isNullInstantiated() ; }


// inserting a NULL interval, if it doesn't already exist
inline void
ColStats::insertNullInterval()
{
  if ( !isNullInstantiated() )
    {
      histogram_->insertNullInterval () ;
      setShapeChanged (TRUE) ;
    }
}

// Are the histograms being merged as part of a join operation
inline NABoolean
ColStats::isAJoinRelatedMerge(MergeType mergeMethod) const
{
  if((mergeMethod == INNER_JOIN_MERGE) ||
     (mergeMethod == SEMI_JOIN_MERGE) ||
     (mergeMethod == ANTI_SEMI_JOIN_MERGE) ||
     (mergeMethod == OUTER_JOIN_MERGE))
     return TRUE;
  else
    return FALSE;
}

// -----------------------------------------------------------------------
//  Statistics List
//    A collection of column statistics.  Column statistics
//    belong to the same set if used for the same equivalence "group"
//    (as defined by Cascades).
// -----------------------------------------------------------------------
class StatsList : public SHPTR_LIST(ColStatsSharedPtr)
{
public:
  StatsList(NAMemory* h,CollIndex initLen =0)
    : heap_(h),
    SHPTR_LIST(ColStatsSharedPtr) (h,initLen),
    groupUecColumns_(h),
    groupUecValues_(h),
    groupMCSkewedValueLists_(h)
  {}

  // ---------------------------------------------------------------------
  //  Virtual Destructor
  // ---------------------------------------------------------------------
  virtual ~StatsList();
  virtual void deepDelete();

  // ---------------------------------------------------------------------
  //  Is list empty?
  // ---------------------------------------------------------------------
  NABoolean is_empty () const { return entries() == 0; }

  void display () const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "",
              CollHeap *c=NULL, char *buf=NULL) const;

  void trace (FILE *f, NATable* table) const;

  //Copy constructor with user specified heap
  StatsList(const StatsList& other, NAMemory * heap):
              heap_(heap),
              SHPTR_LIST(ColStatsSharedPtr) (other,heap),
              groupUecColumns_(other.groupUecColumns_,heap),
              groupUecValues_(other.groupUecValues_,heap),
              groupMCSkewedValueLists_(other.groupMCSkewedValueLists_,heap)
  {};
  // Copy constructor
  StatsList(const StatsList& other):
				heap_(other.heap_),
				SHPTR_LIST(ColStatsSharedPtr) (other,heap_),
				groupUecColumns_(other.groupUecColumns_,heap_),
				groupUecValues_(other.groupUecValues_,heap_),
				groupMCSkewedValueLists_(other.groupMCSkewedValueLists_,heap_)
	{};

  StatsList& operator=(const StatsList& list);

  //reduce the number of histogram intervals in the histograms
  //referenced by the ColStats that are referenced by this StatsList object.
  //The reduction criterion depends on parameters invokedFrom,
  //and reductionCriterion
  void reduceNumHistInts(Source invokedFrom,
                         Criterion reductionCriterion);

  //reduce the number of histogram intervals for histograms
  //referenced by the ColStats that make up this StatsList
  void reduceNumHistIntsAfterFetch(NATable& table);

  // deepCopy()
  // Makes a deep copy of all of its members
  void deepCopy(const StatsList & other, NAMemory * heap);

  // insertByPosition()
  // insert the histograms that have reference to 'position' column
  void insertByPosition(const StatsList & other, const Lng32 position,
                        SET(ColStats*) &dupList);

  // insertCompressedCopy()
  // Takes in full histogram but insert it in compressed form
  ColStatsSharedPtr insertCompressedCopy(const StatsList & realStat,
						const Lng32 position,NABoolean state);

  // insertDeepCopyList()
  // makes a deep copy from the other
  void insertDeepCopyList(const StatsList & other);

  //Returns a reference to a ColStats object representing
  //single column statistics for the column identified by
  //parameter position.
  ColStatsSharedPtr getSingleColumnColStats(const Lng32 position);

  // returns the UEC count from the histogram identified by the parameter
  //position. Position here is the position of the column in the table
  CostScalar getSingleColumnUECCount(const Lng32 position) const;

  // return count of single column histograms (include fake histograms)
  Int32 getSingleColumnCount() const;

  // return count of multi-column histograms
  Int32 getMultiColumnCount() const;

  // return true if all fake histograms or is empty
  NABoolean allFakeStats() const;

  NAMemory * heap() const { return heap_; }

private:
  NAMemory * heap_;

public:

  // these two data members are intentionally left public; otherwise, we
  // need accessor functions, which would be ridiculous for such a small
  // and infrequently-used class

  // these two data members hold the groupUec information that we read in
  // from the system catalogs; we can't convert this data to a
  // MultiColumnUecList until we get to TableDesc::getStatColumns(),
  // because before then we can't do the necessary (NAColumn -> ValueId)
  // mapping
  LIST(NAColumnArray) groupUecColumns_ ;
  LIST(CostScalar)    groupUecValues_ ;
  LIST(MCSkewedValueList)    groupMCSkewedValueLists_ ;
}; //StatsList


// -----------------------------------------------------------------------
//  Skewed Value List
//    A list of skewed values.
// -----------------------------------------------------------------------
class SkewedValueList : public LIST(EncodedValue)
{
public:
  SkewedValueList(NAType* nt, NAMemory* h, CollIndex initLen = 0) :
    natype_(nt), 
    heap_(h), 
    isNullInList_(FALSE), 
    isNullSkewed_(FALSE), 
    LIST(EncodedValue)(h,initLen),
    finalHashComputed_(TRUE) {};

  SkewedValueList(NAMemory* h, CollIndex initLen = 0) :
    natype_(NULL), 
    heap_(h), 
    isNullInList_(FALSE), 
    isNullSkewed_(FALSE), 
    LIST(EncodedValue)(h,initLen),
    finalHashComputed_(TRUE) {};

   ~SkewedValueList() {};

   // insert a skewed value in decending order.
   void insertInOrder(const EncodedValue&);

   // get the list in human-readable form: [v1, v2, ..., vn]
   const NAString getText() const;
   const NAType* getNAType() const { return natype_; };

   NABoolean getIsNullInList() const 
   {
      return isNullInList_;
   }

   void setIsNullInList(NABoolean v)  
   {
      isNullInList_ = v;
   }

   NABoolean getIsNullSkewed() const 
   {
      return isNullSkewed_;
   }

   void setIsNullSkewed(NABoolean v)  
   {
      isNullSkewed_ = v;
   }

    NABoolean hasOnlyNonSkewedNull() const
    {
      return (!getIsNullSkewed() && 
              getIsNullInList() && 
              entries()==1);
    }

   NABoolean needToComputeFinalHash() const { return finalHashComputed_; };
   void setComputeFinalHash(NABoolean x) { finalHashComputed_ = x; };

private:
  NAType*  natype_;
  NAMemory * heap_;
  // TRUE if NULL is in the list regardless of whether it is 
  // skweded or no
  NABoolean isNullInList_;
  // TRUE if NULL is  skewed value
  NABoolean isNullSkewed_;

  NABoolean finalHashComputed_;
};


#endif
