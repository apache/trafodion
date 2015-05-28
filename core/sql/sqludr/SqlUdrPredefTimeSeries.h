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

#ifndef SQLUDR_PREDEF_TIMESERIES_H
#define SQLUDR_PREDEF_TIMESERIES_H

#include "sqludr.h"

using namespace tmudr;

// derive a class from UDR

class TimeSeries : public UDR
{
public:

  // determine output columns dynamically at compile time
  void describeParamsAndColumns(UDRInvocationInfo &info);

  // override the runtime method
  void processData(UDRInvocationInfo &info,
                   UDRPlanInfo &plan);

};

// Factory method

extern "C" UDR * TRAF_CPP_TIMESERIES();

// An object that represents a nullable value of a type at a given
// point in time
template<class T>
class NullableTimedValue
{
public:
  NullableTimedValue()                            { isNull_ = true; t_ = 0; }
  NullableTimedValue(bool isNull, time_t t, T v) :
                                             isNull_(isNull), t_(t), v_(v) {}
  bool isNull() const                                     { return isNull_; }
  time_t getTime() const                                       { return t_; }
  T getVal() const                                             { return v_; }
  NullableTimedValue<double> interpolateLinear(
       time_t t,
       const NullableTimedValue &nextHigherVal);

private:
  bool isNull_;
  time_t t_;
  T v_;

};

// One time slice aggregate function
class TimeSeriesAggregate
{
public:
  TimeSeriesAggregate(const TupleInfo &inTup,
                      const TupleInfo &outTup,
                      int  inputColNum,
                      int  outputColNum,
                      bool isFirstVal,
                      bool isConstInterpol,
                      bool isIgnoreNulls);
  ~TimeSeriesAggregate();

  int  getOutputColNum() const                      { return outputColNum_; }
  int  getInputColNum() const                        { return inputColNum_; }
  bool isFirstVal() const                             { return isFirstVal_; }
  bool isConstInterpol() const                   { return isConstInterpol_; }
  bool isIgnoreNulls() const                       { return isIgnoreNulls_; }

  void initPartition();
  int numEntries();
  time_t getTime(int entry);
  void readInputCol(time_t t, int sliceNum);
  void finalizePartition();
  void setOutputCol(time_t startTime, int sliceNum, time_t width);

private:
  // Description of the aggregate function and its columns
  const TupleInfo &inTup_;
  const TupleInfo &outTup_;
  int  inputColNum_;
  int  outputColNum_;
  bool isFirstVal_;
  bool isConstInterpol_;
  bool isIgnoreNulls_;
  bool useLong_;

  // vectors of first/last values per time slice, collected for one partition
  std::vector<NullableTimedValue<long> >   lValues_;
  std::vector<NullableTimedValue<double> > dValues_;

  // state of the vectors when reading and generating rows
  int currIx_;
  int currSliceNum_;
  int entriesForThisTimeSlice_;

};

// Define data that gets passed between compiler phases and runtime
class InternalColumns : public UDRWriterCompileTimeData
{
public:
  InternalColumns(const UDRInvocationInfo &info);
  ~InternalColumns();
  int getNumCols() const     { return getFirstAggrCol() + getNumAggrCols(); }
  int getTimeSliceInColNum() const                    { return tsInColNum_; }
  int getTimeSliceOutColNum() const                  { return numPartCols_; }
  int getNumPartCols() const                         { return numPartCols_; }
  int getNumAggrCols() const                      { return columns_.size(); }
  int getFirstAggrCol() const           { return numTSCols_ + numPartCols_; }
  TimeSeriesAggregate &getAggrColumn(int i)        { return *(columns_[i]); }

  void initializePartition();
  bool isSamePartition();
  int  getNumTimeSlices();
  void readInputCols();
  void finalizePartition();
  void setOutputCols(int sliceNum);

private:

  const UDRInvocationInfo &info_;

  // number of timestamp and partitioning columns
  int tsInColNum_;
  int numTSCols_;
  int numPartCols_;

  // key of the current partition we are processing
  std::vector<std::string> currPartKey_;

  // null indicators for current partition key
  std::vector<bool> currPartKeyNulls_;

  // width of one time slice in time_t units (seconds)
  time_t timeSliceWidth_;

  // start time, rounded down to a multiple of
  // the time slice width
  time_t partitionStartTime_;

  // current end time of the partition, set in
  // processRowForPartition()
  time_t partitionCurrTime_;

  // Array of objects that handle the individual aggregates
  std::vector<TimeSeriesAggregate *> columns_;
};

#endif
