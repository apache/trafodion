// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
// **********************************************************************

#include "Platform.h"
#include "hs_faststats.h"
#include "hs_globals.h"

template <class T> void FastStatsHist<T>::addRowset(Lng32 numRows)
{
  T* dataPtr = (T*)group_->nextData;
  Int32 dataLen = group_->ISlength;
  short* nullIndic = group_->nullIndics;

  if (totalFreq_ == 0)
    min_ = max_ = *dataPtr;

  totalFreq_ += numRows;  //@ZXbl -- should exclude nulls from this count?

  // Use strNextData instead of nextData for char types.
  switch (group_->ISdatatype)
    {
      case REC_BYTE_F_ASCII:
        // This fails for non-char* instantiations of the template.
        // Also, it ISdatatype for char is ISFixedChar or similar.
        // When these types are added, watch out for ptr arith
        // when advancing dataPtr below.
        //dataPtr = (char*)group_->strNextData;
        //break;
      case REC_BYTE_F_DOUBLE:
      case REC_BYTE_V_ASCII:
      case REC_BYTE_V_DOUBLE:
        HS_ASSERT(FALSE);
        break;
      default:
        break;
    }

  for (Int64 i=0; i<numRows; i++)
    {
      if (nullIndic && *nullIndic++ == -1)
        nullCount_++;
      else
        {
          // Update min or max if the encoded value is not within their range.
          // @ZXbl -- need to encode if char, using EncVal_encodeString (in optimizer/EncodedValue.cpp).
          HS_ASSERT(!DFS2REC::isAnyCharacter(group_->ISdatatype));
          if (min_ > *dataPtr)
             min_ = *dataPtr;
          else if (max_ < *dataPtr)
             max_ = *dataPtr;
          cbf_->insert((char*)dataPtr, dataLen);
        }

      dataPtr++;
    }
}

template <class T> void FastStatsHist<T>::actuate(Lng32 numEHIntervals)
{
  //cbf_->printfreq(group_->colNames->data());
  Int32 numEWIntervals = 4 * numEHIntervals;
  T width = (max_ - min_ + (numEWIntervals - 1)) / numEWIntervals;
  if (width == 0) // range of values (e.g., _SALT_) may be less than # intervals
    width = 1;

  // Average height of intervals in equi-width histogram. This is passed to the
  // histogram ctor below when creating the equi-width histogram, and is used as
  // the initial number of elements in the NAArray underlying the interval (an
  // array of value/frequency pairs).
  Int32 keysPerEWInterval = cbf_->getAllKeys().entries() / numEWIntervals;

  //  FSHistogram* equiWidthHist =
  //      new(STMTHEAP) FSHistogram(STMTHEAP, numEWIntervals, keysPerEWInterval);
  FSHistogram<T> equiWidthHist(STMTHEAP, numEWIntervals, keysPerEWInterval);

  // Now compute the equi-width histogram.

  const NAList<simple_cbf_key>& keys = cbf_->getAllKeys();
  CollIndex intvlInx;
  UInt64 freq;
  T keyVal;

  // Iterate over each distinct value found and add it to the correct interval
  // of the equi-width histogram.
  for (CollIndex i=0; i<keys.entries(); i++ )
  {
     const simple_cbf_key& key = keys[i];

     // Look up the key in the CBF and find its frequency of occurrence.
     if (!cbf_->contain(key.getKey(), key.getKeyLen(), &freq))
       continue;  // why would the key not be found in CBF?

     // compute the interval index for the key
     keyVal = *((T*)key.getKey());
     intvlInx = (CollIndex)((keyVal - min_) / width);
     if (intvlInx == numEWIntervals)
       intvlInx--;

     if (intvlInx < 0)
       continue;  // shouldn't happen if min/max maintained correctly

     // Insert the encoded value and freq pair into the interval.
     KeyFreqPair<T> vf(keyVal, (UInt32)freq);
     equiWidthHist[intvlInx].append(vf);
  }

  //equiWidthHist.display(cout, "Equi-width histogram:");

  // Now convert the equi-width histogram into equal height one

  //float skRatio = 0.05;
  float skRatio = 1.00;

  // Set the target interval height to the total frequency divided by the
  // desired number of intervals.
  Int32 height = totalFreq_ / numEHIntervals;

  // This is an estimate of the number of distinct values that will be
  // represented in each interval of the equi-height histogram. It will
  // only be exactly correct when each distinct value has the same
  // frequency. It is only used as an arg to the ctor to construct the
  // equi-height histogram, saying what the initial number of elements in
  // the NAArray of intervals should be.
  Int32 keysPerEHInterval = keys.entries() / numEHIntervals;

  FSHistogram<T> equiHeightHist(STMTHEAP, numEHIntervals, keysPerEHInterval);

  // First allocate 'numEHIntervals' intervals. May require more.
  NAList<T> boundaries(STMTHEAP, numEHIntervals);

  equiWidthHist.convertToEQHistogram(height, equiHeightHist, boundaries);
  //equiHeightHist.display(cout, "Equa-height Histogram");
  equiHeightHist.estimateRowsAndUecs(FastStatsHist::SAMPLE_RATE, skRatio);
  //equaHeightHistogram.displayRowsAndUecs(cout, "========== Computed UECs ===========");
}

template <class T> void FSInterval<T>::estimateRowsAndUecs(double sample_rate, float skRatio)
{
  FrequencyCounts fi_s;

  if (this->entries() == 0)
    {
      uec_ = 0;
      rc_ = 0;
    }
  else if (this->entries() == freqCount_)
    {
      uec_ = 1;
      rc_ = this->entries();
    }
  if (sample_rate == 1.0)
    {
      uec_ = this->entries();
      rc_ = freqCount_;
    }
  else
    {
      Int32 skewCutOff = skRatio * freqCount_;

      Int32 keys = 0;
      for ( Int32 i=0; i<this->entries(); i++ )
        {
          Int32 frequency = this->at(i).freq;

          if (frequency < skewCutOff )
            {
              fi_s.increment(frequency);
              keys++;
            }
        }

      double sampleUec = (double)keys;
      double sampleRowCnt = (double)freqCount_;
      double DshMax = CmpCommon::getDefaultNumeric(USTAT_DSHMAX);
      double coeffOfVar = 0;
      double Duj = 0;
      double Dsh = 0;
      double estTotalRC = sampleRowCnt / sample_rate;
      double uec = lwcUecEstimate(sampleUec, sampleRowCnt, estTotalRC, &fi_s,
                                  DshMax, coeffOfVar, Duj, Dsh);
      uec_ = (Int32)uec;
      rc_ = sampleRowCnt / sample_rate;
    }
}

// Explicit instantiations of template member functions, so their definition
// can appear in this file instead of in .h file.
template void FastStatsHist<int>::addRowset(Lng32 numRows);
template void FastStatsHist<unsigned int>::addRowset(Lng32 numRows);
template void FastStatsHist<short>::addRowset(Lng32 numRows);
template void FastStatsHist<unsigned short>::addRowset(Lng32 numRows);
template void FastStatsHist<long>::addRowset(Lng32 numRows);
template void FastStatsHist<float>::addRowset(Lng32 numRows);
template void FastStatsHist<double>::addRowset(Lng32 numRows);

template void FastStatsHist<int>::actuate(Lng32);
template void FastStatsHist<unsigned int>::actuate(Lng32);
template void FastStatsHist<short>::actuate(Lng32);
template void FastStatsHist<unsigned short>::actuate(Lng32);
template void FastStatsHist<long>::actuate(Lng32);
template void FastStatsHist<float>::actuate(Lng32);
template void FastStatsHist<double>::actuate(Lng32);

template void FSInterval<int>::estimateRowsAndUecs(double, float);
template void FSInterval<unsigned int>::estimateRowsAndUecs(double, float);
template void FSInterval<short>::estimateRowsAndUecs(double, float);
template void FSInterval<unsigned short>::estimateRowsAndUecs(double, float);
template void FSInterval<long>::estimateRowsAndUecs(double, float);
template void FSInterval<float>::estimateRowsAndUecs(double, float);
template void FSInterval<double>::estimateRowsAndUecs(double, float);
