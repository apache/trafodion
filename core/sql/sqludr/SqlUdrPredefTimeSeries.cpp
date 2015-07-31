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

#include "SqlUdrPredefTimeSeries.h"
#include <limits>

// sample invocation of the TIMESERIES TMUDF:
//
// SELECT time_slice, a, b, c_LLI -- note the suffix applied by the UDF
// FROM UDF(timeseries(
//        -- input table with partition by/order by 
//        TABLE(SELECT a,b,c FROM t PARTITION BY a ORDER BY b),
//        'TIME_SLICE',          -- name of time slice column
//        INTERVAL '30' SECOND,  -- length of time slice
//        'C',                   -- name of first column to interpolate
//        'LLI'))                -- instructions how to interpolate
//                               -- (Last value, Linear, ignore nulls)
//        -- optionally followed by more pairs of columns and instructions
//
//        Allowed instructions (lower or upper case):
//        FC[I] First value, constant interpolation
//        LC[I] Last  value, constant interpolation
//        FL[I] First value, linear interpolation
//        LL[I] Last  value, linear interpolation

// factory method

extern "C" UDR * TRAF_CPP_TIMESERIES()
{
  return new TimeSeries();
}

void TimeSeries::describeParamsAndColumns(UDRInvocationInfo &info)
{
  InternalColumns internalCols(info);

  // create PARTITION BY output columns, one passthru column
  // for every column that appears in PARTITION BY
  const PartitionInfo &part = info.in().getQueryPartitioning();
  int numPartCols = part.getNumEntries();

  for (int pc=0; pc<numPartCols; pc++)
    info.addPassThruColumns(0, part.getColumnNum(pc), part.getColumnNum(pc));

  // since we work locally in a partition, set the function type
  // of this TMUDF to REDUCER
  info.setFuncType(UDRInvocationInfo::REDUCER);

  // produce the time column, it has the same type as the
  // ORDER BY column that defines the input time value
  // and its name is specified by parameter 0
  const TypeInfo &timeType =
    info.in().getColumn(internalCols.getTimeSliceInColNum()).getType();

  info.out().addColumn(ColumnInfo(info.par().getString(0).c_str(),
                                  timeType));

  // produce aggregate columns
  for (int a=0; a<internalCols.getNumAggrCols(); a++)
    {
      TimeSeriesAggregate tsa = internalCols.getAggrColumn(a);
      std::string outColName(info.par().getString(2*a + 2));
      TypeInfo inColType(
           info.in().getColumn(tsa.getInputColNum()).getType());

      // append suffix to input column name to form the output column
      // name, make those all capitals to avoid delimited identifiers
      outColName += "_";
      if (tsa.isFirstVal())
        outColName += "F";
      else
        outColName += "L";
      if (tsa.isConstInterpol())
        outColName += "C";
      else
        outColName += "L";
      if (tsa.isIgnoreNulls())
        outColName += "I";

      if (tsa.isConstInterpol())
        {
          // add a column with the same data type as the original
          // column, but make it nullable if it isn't already
          inColType.setNullable(true);
          info.out().addColumn(ColumnInfo(outColName.c_str(), inColType));
        }
      else
        // add a "DOUBLE" output column to allow interpolation
        info.out().addColumn(ColumnInfo(
                                  outColName.c_str(),
                                  TypeInfo(TypeInfo::DOUBLE_PRECISION,
                                           0,
                                           true)));
    }

  // add formal parameters with types that match the actual ones
  for (int p=0; p<info.par().getNumColumns(); p++)
    {
      char parName[20];

      snprintf(parName, sizeof(parName), "PAR_%d", p);
      info.addFormalParameter(ColumnInfo(parName,
                                         info.par().getColumn(p).getType()));
    } 
}

void TimeSeries::processData(UDRInvocationInfo &info,
                             UDRPlanInfo &plan)
{
  InternalColumns internalCols(info);
  bool haveMoreRows = false;
  bool haveRowFromPreviousLoop = true; // will read one below

  // this will exit right away if there are no rows,
  // doing this make the logic below a bit simpler
  haveMoreRows = getNextRow(info);

  while (haveMoreRows)
    {
      bool inSamePartition = true;

      // read all rows of one partition,
      // plus the next row or end of data
      while (inSamePartition)
        {
          if (!haveRowFromPreviousLoop)
            {
              // try to read a new row
              if (getNextRow(info))
                {
                  // is this row still from the same partition?
                  if ((inSamePartition = internalCols.isSamePartition()) == false)
                    haveRowFromPreviousLoop = true;
                }
              else
                {
                  // we reached end-of-data, exit this loop
                  // and emit the last partition
                  haveMoreRows = false;
                  inSamePartition = false;
                }
            }
          else
            {
              // consume the row we read in the last iteration
              // or in the initialization code, this indicates
              // the start of a new partition

              // prepare for the next partition
              internalCols.initializePartition();
              haveRowFromPreviousLoop = false;
            }

          if (inSamePartition)
            {
              // remember all the columns of this row for
              // the computed time slice number
              internalCols.readInputCols();
            }
        } // read rows of one partition...

      // we read all rows from one partition (plus one more, maybe),
      // now emit the rows

      // prepare to emit all time slices for this partition
      internalCols.finalizePartition();

      int numTimeSlices = internalCols.getNumTimeSlices();

      // loop over time slices and emit rows
      for (int s=0; s<numTimeSlices; s++)
        {
          // generate an output record for one time slice of
          // one partition
          internalCols.setOutputCols(s);

          // emit a row for one time slice
          emitRow(info);
        }
    } // emitPartition
}

template<class T>
NullableTimedValue<double> NullableTimedValue<T>::interpolateLinear(
     time_t t, const NullableTimedValue<T> &nextHigherVal)
{
  // return a NULL value if t is outside the range of
  // "this" and "nextHigherVal" or if "this" or
  // "nextHigherVal" is a NULL value
  if (t < t_ ||
      t > nextHigherVal.t_ ||
      isNull_ ||
      nextHigherVal.isNull_)
    return NullableTimedValue<double>(true, t, 0.0);

  // this should not be common, but avoid division by zero for this unlikely case
  if (t_ == nextHigherVal.t_)
    return NullableTimedValue<double>(false, t, (v_ + nextHigherVal.v_)/2);

  return NullableTimedValue<double>(
       false,
       t,
       static_cast<double>(v_) +
       (static_cast<double>(t)-t_)/(nextHigherVal.t_-t_) * (nextHigherVal.v_ - v_));
}

TimeSeriesAggregate::TimeSeriesAggregate(const TupleInfo &inTup,
                                         const TupleInfo &outTup,
                                         int  inputColNum,
                                         int  outputColNum,
                                         bool isFirstVal,
                                         bool isConstInterpol,
                                         bool isIgnoreNulls) :
     inTup_(inTup),
     outTup_(outTup),
     inputColNum_(inputColNum),
     outputColNum_(outputColNum),
     isFirstVal_(isFirstVal),
     isConstInterpol_(isConstInterpol),
     isIgnoreNulls_(isIgnoreNulls),
     currIx_(0)
{
  // use a long data type for constant interpolation when
  // the underlying column is an exact numeric, otherwise
  // use a double
  useLong_ = (isConstInterpol &&
              inTup_.getColumn(inputColNum_).getType().getSQLTypeSubClass()
                 == TypeInfo::EXACT_NUMERIC_TYPE);
}

TimeSeriesAggregate::~TimeSeriesAggregate()
{
}

void TimeSeriesAggregate::initPartition()
{
  if (useLong_)
    lValues_.clear();
  else
    dValues_.clear();

  currIx_ = currSliceNum_ = -1;
  entriesForThisTimeSlice_ = 0;
}

int TimeSeriesAggregate::numEntries()
{
  if (useLong_)
    return lValues_.size();
  else
    return dValues_.size();
}

time_t TimeSeriesAggregate::getTime(int entry)
{
  if (useLong_)
    return lValues_[entry].getTime();
  else
    return dValues_[entry].getTime();
}

void TimeSeriesAggregate::readInputCol(time_t t, int sliceNum)
{
  long lVal = 0;
  double dVal = 0.0;
  bool wasNull;

  if (useLong_)
    lVal = inTup_.getLong(inputColNum_);
  else
    dVal = inTup_.getDouble(inputColNum_);
  wasNull = inTup_.wasNull();

  // check whether we should ignore a NULL value
  if (!(wasNull && isIgnoreNulls_))
    {
      // is this row for a new time slice?
      if (sliceNum > currSliceNum_)
        {
          currSliceNum_ = sliceNum;
          entriesForThisTimeSlice_ = 0;
        }

      // we keep up to 2 entries for each time slice, the
      // first value we see and the last value
      if (entriesForThisTimeSlice_ < 2)
        {
          // first or second value, extend the vectors
          if (useLong_)
            lValues_.push_back(NullableTimedValue<long>(wasNull, t, lVal));
          else
            dValues_.push_back(NullableTimedValue<double>(wasNull, t, dVal));
          currIx_++;
          entriesForThisTimeSlice_++;
        }
      else
        {
          // subsequent value, overwrite the last value seen, we don't need
          // to keep values that are neither the first nor the last value in
          // this time slice
          if (useLong_)
            lValues_[currIx_] = NullableTimedValue<long>(wasNull, t, lVal);
          else
            dValues_[currIx_] = NullableTimedValue<double>(wasNull, t, dVal);
        }
    }
}

void TimeSeriesAggregate::finalizePartition()
{
  // print out the gathered data for debugging
  /*
  printf("Aggregate column # %d\n", outputColNum_);
  for (int i=0; i<numEntries(); i++)
    {
      printf("Time: %d, value: ", static_cast<int>(getTime(i)));
      if (useLong_)
        {
          if (lValues_[i].isNull())
            printf("NULL\n");
          else
            printf("%ld\n", lValues_[i].getVal());
        }
      else
        {
          if (dValues_[i].isNull())
            printf("NULL\n");
          else
            printf("%g\n", dValues_[i].getVal());
        }
    }
  */
  currIx_ = -1;
  currSliceNum_ = entriesForThisTimeSlice_ = -1;
}

void TimeSeriesAggregate::setOutputCol(time_t startTime,
                                       int sliceNum,
                                       time_t width)
{
  time_t targetTime = (isFirstVal_ ? startTime : startTime + width);
  time_t currTime = 0;

  // Find the last entry before our target time, if it exists.
  // Include the target time for first value, exclude it for
  // last value (in that case the target time already belongs
  // to the next time slice)
  while (currIx_+1 < numEntries() &&
         ((isFirstVal_ && getTime(currIx_+1) <= targetTime) ||
          (!isFirstVal_ && getTime(currIx_+1) < targetTime)))
    currIx_++;

  if (currIx_ >= 0)
    currTime = getTime(currIx_);

  if (isConstInterpol_)
    {
      // currIx_ points to the entry with the last known constant
      // value to use or currIx_ is -1 (no last known value exists)
      if (currIx_ < 0 ||
          (useLong_ && lValues_[currIx_].isNull()) ||
          (!useLong_ && dValues_[currIx_].isNull()))
        // produce a NULL value
        outTup_.setNull(outputColNum_);
      else if (useLong_)
        outTup_.setLong(outputColNum_, lValues_[currIx_].getVal());
      else
        outTup_.setDouble(outputColNum_, dValues_[currIx_].getVal());
    } // constant interpolation
  else
    {
      // linear interpolation, always uses a double value

      // currIx_ points to the last known value or is -1 (no last
      // known value exists). Now look for the next known value.
      NullableTimedValue<double> interpolatedVal;

      if (currIx_ >= 0 && currTime == targetTime)
        {
          // currIx_ and nextIx_ point to the one
          // entry describing the value, no interpolation needed
          interpolatedVal = dValues_[currIx_];
        }
      else if (currIx_ < 0 || currIx_+1 >= numEntries())
        {
          // produce a NULL value for these boundary cases where
          // we are missing an upper or lower entry to use for
          // interpolation
          interpolatedVal = NullableTimedValue<double>(true, targetTime, 0.0);
        }
      else
        {
          // currIx_ and the next entry should have time values
          // that form an interval we can use to interpolate
          interpolatedVal =
            dValues_[currIx_].interpolateLinear(
                    targetTime,
                    dValues_[currIx_+1]);
        }

      // now produce the value
      if (interpolatedVal.isNull())
        outTup_.setNull(outputColNum_);
      else
        outTup_.setDouble(outputColNum_, interpolatedVal.getVal());
    } // linear interpolation
}



InternalColumns::InternalColumns(const UDRInvocationInfo &info) :
     info_(info)
{
  // expect a single table-valued input
  if (info.getNumTableInputs() != 1)
    throw UDRException(
         38010,
         "TIMESERIES UDF: Expecting one table-valued input");

  const OrderInfo &ord = info.in().getQueryOrdering();
  const PartitionInfo &part = info.in().getQueryPartitioning();

  // perform some basic tests in the first call at compile time
  if (info.getCallPhase() == UDRInvocationInfo::COMPILER_INITIAL_CALL)
    {
      // expect an order by on a time or timestamp expression
      if (ord.getNumEntries() != 1 ||
          ord.getOrderType(0) == OrderInfo::DESCENDING)
        throw UDRException(
             38020,
             "TIMESERIES UDF: Must use ORDER BY with one column for its input table and the order must be ascending");
      TypeInfo::SQLTypeCode typeCode =
        info.in().getColumn(ord.getColumnNum(0)).getType().getSQLType();
      if (typeCode != TypeInfo::TIME &&
          typeCode != TypeInfo::TIMESTAMP)
        throw UDRException(
             38030,
             "TIMESERIES UDF: Must use ORDER BY a TIME or TIMESTAMP column for the input table");

      // we need at least two parameters, time column name and width
      // of time slice
      if (info.par().getNumColumns() < 2)
        throw UDRException(
             38040,
             "TIMESERIES UDF: UDF needs to be called with at least 2 scalar parameters");

      // input parameter 0 (defined in the DDL) is the
      // name of the column containing the time values
      if (!info.par().isAvailable(0) ||
          info.par().getColumn(0).getType().getSQLTypeClass() !=
          TypeInfo::CHARACTER_TYPE)
        throw UDRException(
             38050,
             "TIMESERIES UDF: Expecting a character constant (timestamp alias) as first parameter");

      // check type and value of the time slice width, specified
      // as parameter 1
      if (!info.par().isAvailable(1))
        throw UDRException(
             38060,
             "TIMESERIES UDF: Expecting a constant for the time slice width as second parameter");

      // time slice width must be a day-second interval
      if (info.par().getColumn(1).getType().getSQLTypeSubClass() !=
          TypeInfo::DAY_SECOND_INTERVAL_TYPE)
        throw UDRException(
             38070,
             "TIMESERIES UDF: Second scalar parameter for time slice width must be an interval constant in the day to second range");

      // make sure parameters come in pairs
      if (info.par().getNumColumns() % 2 != 0)
        throw UDRException(
             38080,
             "TIMESERIES UDF: Parameters need to be specified in pairs of column name and instructions");

      // make sure all parameters are specified at compile time
      for (int p=2; p<info.par().getNumColumns(); p++)
        if (!info.par().isAvailable(p))
          throw UDRException(
               38090,
               "TIMESERIES UDF: All parameters must be specified as literals");
    } // initial compile-time checks

  tsInColNum_     = ord.getColumnNum(0);
  numTSCols_      = 1; // always a single timestamp column for now
  numPartCols_    = part.getNumEntries();

  timeSliceWidth_  = info.par().getTime(1);

  // initialize vectors
  for (int p=0; p<numPartCols_; p++)
    {
      currPartKey_.push_back("");
      currPartKeyNulls_.push_back(true);
    }

  int ip = 2;

  while (ip<info.par().getNumColumns())
    {
      std::string colName = info.par().getString(ip);
      std::string instr = info.par().getString(ip+1);
      bool isFirstVal;
      bool isConstInterpol;
      bool isIgnoreNulls;

      // some checks done only during the first compile time call
      if (info.getCallPhase() == UDRInvocationInfo::COMPILER_INITIAL_CALL)
        {
          if (instr.size() < 2 || instr.size() > 3)
            throw UDRException(
                 38100,
                 "TIMESERIES UDF: Expecting instructions with 2 or 3 characters: %s",
                 instr.c_str());

          // validate first character of instructions
          switch (instr[0])
            {
            case 'f':
            case 'F':
            case 'l':
            case 'L':
              break;

            default:
              throw UDRException(
                   38110,
                   "TIMESERIES UDF: Parameter %d should start with F or L for first or last value",
                   ip+2);
            }

          // validate second character of instructions
          switch (instr[1])
            {
            case 'c':
            case 'C':
            case 'l':
            case 'L':
              break;

            default:
              throw UDRException(
                   38120,
                   "TIMESERIES UDF: Parameter %d should have C or L as its second character, for constant or linear interpolation",
                   ip+2);
            }

          if (instr.size() == 3 &&
              instr[2] != 'i' &&
              instr[2] != 'I')
            throw UDRException(
                 38130,
                 "TIMESERIES UDF: Unexpected trailing characters in aggregate instructions: %s",
                 instr.c_str());
        } // compile-time checks

      isFirstVal = (instr[0] == 'F' || instr[0] == 'f');
      isConstInterpol = (instr[1] == 'C' || instr[1] == 'c');
      isIgnoreNulls = (instr.size() > 2);

      columns_.push_back(new TimeSeriesAggregate(
                              info.in(),
                              info.out(),
                              info.in().getColNum(info.par().getString(ip)),
                              getFirstAggrCol() + columns_.size(),
                              isFirstVal,
                              isConstInterpol,
                              isIgnoreNulls));
      ip += 2;
    }
}

InternalColumns::~InternalColumns()
{
  for (std::vector<TimeSeriesAggregate *>::iterator it = columns_.begin();
       it != columns_.end();
       it++)
    delete *it;
}

void InternalColumns::initializePartition()
{
  const PartitionInfo &pi = info_.in().getQueryPartitioning();

  // get first value of the time stamp column and round it down
  // to the next multiple of the time stamp width
  partitionStartTime_ = info_.in().getTime(tsInColNum_);
  partitionStartTime_ -= partitionStartTime_ % timeSliceWidth_;

  // start and end time are the same for the first row
  partitionCurrTime_ = partitionStartTime_;

  // remember the partition key values as an array of strings
  for (int p=0; p<numPartCols_; p++)
    {
      currPartKey_[p] = info_.in().getString(pi.getColumnNum(p));
      currPartKeyNulls_[p] = info_.in().wasNull();
    }

  for (std::vector<TimeSeriesAggregate *>::iterator it = columns_.begin();
       it != columns_.end();
       it++)
    (*it)->initPartition();
}

bool InternalColumns::isSamePartition()
{
  const PartitionInfo &pi = info_.in().getQueryPartitioning();
  std::string pkVal;
  bool wasNull;

  // compare partitioning keys
  for (int p=0; p<numPartCols_; p++)
    {
      pkVal = info_.in().getString(pi.getColumnNum(p));
      wasNull = info_.in().wasNull();
        
      if (wasNull != currPartKeyNulls_[p] ||
          (!wasNull && pkVal != currPartKey_[p]))
        return false;
    }

  // partition key matched, we are still in the same partition
  return true;
}

int InternalColumns::getNumTimeSlices()
{
  return (partitionCurrTime_ - partitionStartTime_) / timeSliceWidth_ + 1;
}

void InternalColumns::readInputCols()
{
  int sliceNum;

  // remember the highest time value we have seen for this partition
  partitionCurrTime_ = info_.in().getTime(tsInColNum_);
  sliceNum = getNumTimeSlices() - 1;

  for (std::vector<TimeSeriesAggregate *>::iterator it = columns_.begin();
       it != columns_.end();
       it++)
    (*it)->readInputCol(partitionCurrTime_, sliceNum);
}

void InternalColumns::finalizePartition()
{
  for (std::vector<TimeSeriesAggregate *>::iterator it = columns_.begin();
       it != columns_.end();
       it++)
    (*it)->finalizePartition();
}

void InternalColumns::setOutputCols(int sliceNum)
{
  time_t timeSliceStartTime = partitionStartTime_ + sliceNum * timeSliceWidth_;

  // set the partitioning columns
  for (int p=0; p<numPartCols_; p++)
    {
      if (currPartKeyNulls_[p])
        info_.out().setNull(p);
      else
        info_.out().setString(p,currPartKey_[p]);
    }

  // set the time slice column
  info_.out().setTime(
       numPartCols_,
       timeSliceStartTime);

  // set the aggregate columns
  for (std::vector<TimeSeriesAggregate *>::iterator it = columns_.begin();
       it != columns_.end();
       it++)
    (*it)->setOutputCol(
         timeSliceStartTime,
         sliceNum,
         timeSliceWidth_);
}

