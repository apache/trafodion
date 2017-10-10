// **********************************************************************
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
// **********************************************************************

#include "Platform.h"

#include "BloomFilter.h"
#include "dfs2rec.h"
#include "hs_const.h"
#include "hs_log.h"
#include <iostream>

struct HSColGroupStruct;

template <class T>
struct KeyFreqPair
{
  KeyFreqPair(T value = 0, UInt32 frequency = 0)
    : val(value), freq(frequency)
    {}

  ~KeyFreqPair() {};

  NABoolean operator<=(const KeyFreqPair& other)
  {
     if (val < other.val)
       return TRUE;

     if (val == other.val)
       return (freq <= other.freq);

     return FALSE;
  }

  NABoolean operator<(const KeyFreqPair& other)
  {
     if (val < other.val)
       return TRUE;

     if (val == other.val)
       return (freq < other.freq);

     return FALSE;
  }

  NABoolean operator==(const KeyFreqPair& other)
  {
    return (val  == other.val &&
            freq == other.getFreq());
  }

  KeyFreqPair& operator=(const KeyFreqPair& other)
  {
    val  = other.val;
    freq = other.freq;
    return *this;
  }

  T val;
  UInt32 freq;
};

/**
 * The purpose of this class is to allow a generic (independent
 * of the type it is instantiated with) pointer to FastStatsHist.
 */
class AbstractFastStatsHist : public NABasicObject
{
public:
    AbstractFastStatsHist() {}
  virtual ~AbstractFastStatsHist() {}

  virtual void addRowset(Lng32 numRows) = 0;
  virtual void actuate(Lng32 numIntervals) = 0;
};

template <class T>
class FastStatsHist : public AbstractFastStatsHist
{
public:
  FastStatsHist(HSColGroupStruct* group, FastStatsCountingBloomFilter* cbf)
    : group_(group),
      cbf_(cbf),
      nullCount_(0),
      totalFreq_(0)
    {}

  ~FastStatsHist()
  {
    delete cbf_;
  }

  // @ZXbl -- temporary value used for sample rate (needed to estimate uec/rc).
  //          This needs to be changed to come from the sample rate used when
  //          the sample table is created during bulk load.
#if __GNUC_MINOR__ == 8
  static constexpr double SAMPLE_RATE = .01;
#else
  static const double SAMPLE_RATE = .01;
#endif

  void addRowset(Lng32 numRows);
  void actuate(Lng32 numIntervals);

private:
  HSColGroupStruct* group_;
  FastStatsCountingBloomFilter* cbf_;
  Int64 nullCount_;
  Int64 totalFreq_;
  T min_, max_;
};


/**
 * Class representing an interval of a histogram. It is derived from an NAArray
 * of EncodeValueFreqPair objects, and so consists of some number of distinct
 * values, each paired with its frequency of occurrence.
 */
template <class T>
class FSInterval : public NAArray<KeyFreqPair<T>>
{
public:
  /**
   * Constructs an interval with the given row count and uec (0 by default).
   *
   * @param heap Heap to use in call to NAArray ctor.
   * @param count Initial number of elements in the NAArray.
   * @param rc Initial row count of the interval.
   * @param uec Initial UEC of the interval.
   */
  FSInterval(NAHeap* heap = NULL, Int32 count = 0, Int32 rc =0, Int32 uec = 0)
   : NAArray<KeyFreqPair<T>>(heap, count),
     freqCount_(0), sorted_(FALSE),
     uec_(uec), rc_(rc), low_(0.0)
   {}

  ~FSInterval()
  {}

  //NABoolean sorted() { return sorted_; };
  //void sort();

  // find the Ksmallest elements such that their freq total is just over kInFreq
  // the data is sort.
  //UInt32 findKSmallestViaQsort(Int64 kInFreq, UInt32 left, FSInterval& result, EncodedValue& b);

  Int32 getFreqCount() const { return freqCount_; }

  // return the scaled up values of RC and UEC
  Int32 getRC() const { return rc_; }
  Int32 getUec() const { return uec_; }
  double getLow() { return low_; }

  //void mergeInterval(FSInterval& x, UInt32 left = 0)
  //   { mergeInterval(x, left, x.entries()-1); }

  //void mergeInterval(FSInterval& x, UInt32 left, UInt32 right) ;

  void append(KeyFreqPair<T>& x)
  {
    this->insertAt(this->entries(), x);
    freqCount_ += x.freq;
  }

  //NABoolean verifyKeys(double upper);

  void estimateRowsAndUecs(double sample_rate, float skRatio);

  //ostream& display(ostream&, const char* title = "" );

private:
  void swap(UInt32 left, UInt32 right)
  {
    KeyFreqPair<T> tmp = this->at(left);
    this->at(left) = this->at(right);
    this->at(right) = tmp;
  }

  Int32 freqCount_;
  NABoolean sorted_;

  Int32 rc_;
  Int32 uec_;

  double low_;
};


/**
 * A histogram, consisting of a list of FSInterval objects.
 */
template <class T>
class FSHistogram : public NAList<FSInterval<T>>
{
public:
  /**
   * Constructs a histogram with the given number of intervals, all of the
   * same height.
   *
   * @param heap Heap to use in NAList ctor.
   * @param buckets Number of intervals to create for the new histogram.
   * @param height Height of each interval. Passed to each interval ctor as
   *               the initial number of array elements, but the interval's
   *               initial rowcount is left at 0.
   */
  FSHistogram(NAHeap* heap, Int32 buckets, Int32 height)
     : buckets_(buckets), height_(height),
       heap_(heap), NAList<FSInterval<T>>(heap, buckets)
  {
    for (CollIndex i=0; i<buckets; i++)
      this->insert(FSInterval<T>(heap, height));
  };

  ~FSHistogram()
  {};

  Int32 keyCountAt(Int32 bucketIdx);

  void increaseBuckets(NAHeap* heap, Int32 more)
  {
    Int32 n = this->entries();
    for (CollIndex i=0; i<more; i++)
      this->insertAt(n+i, FSInterval<T>(heap, 10));
  }

  void convertToEQHistogram(Int32 height,
                            FSHistogram& equiHeightHistogram,
                            NAList<T>& boundaries)
  {}

  void estimateRowsAndUecs(double sample_rate, float skRatio)
  {
    for (CollIndex i=0; i<this->entries(); i++)
      {
        FSInterval<T>& intv = this->at(i);
        intv.estimateRowsAndUecs(sample_rate, skRatio);
      }
  }

  //void sortAllIntervals();

  //NABoolean verifyIntervals();

  void display(ostream& out, const char* title)
  {
    out << title << endl;
    out << "Total intervals=" << this->entries() << endl;

    Int64 totalKeys = 0;
    Int64 totalFreq = 0;

    for (CollIndex i=0; i<this->entries(); i++)
      {
        FSInterval<T>& intv = this->at(i);

        totalKeys += intv.entries();
        totalFreq += intv.getFreqCount();

        out << " intv[" << i << "]: keys="
            << intv.entries()
            << ", totalFreq="
            << intv.getFreqCount()
            << ". ScaledUp: rc="
            << intv.getRC()
            << "; uec="
            << intv.getUec()
            << "; low="
            //<< std::setprecision(15)
            << intv.getLow()
            << endl;
        //intv.display();
      }

    cout << "total keys=" << totalKeys << endl;
    cout << "total frequency=" << totalFreq << endl;
  }

  //void displayRowsAndUecs(ostream& out, const char* title);

protected:
  Int32 buckets_;
  Int32 height_;
  NAHeap* heap_;

};
