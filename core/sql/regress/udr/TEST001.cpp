// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2000-2015 Hewlett-Packard Development Company, L.P.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <math.h>
#include <limits>
#include "sqludr.h"

using namespace tmudr;

// C++ Sessionize TMUDF
// Note: This code is similar to in the tutorial on the wiki
// https://wiki.trafodion.org/wiki/index.php/Tutorial:_The_object-oriented_UDF_interface

class Sessionize : public UDR
{
public:

  // determine output columns dynamically at compile time
  void describeParamsAndColumns(UDRInvocationInfo &info);

  // eliminate unused columns and help with predicate pushdown
  void describeDataflowAndPredicates(UDRInvocationInfo &info);

  // generate constraints for the table-valued result
  void describeConstraints(UDRInvocationInfo &info);

  // estimate result cardinality
  void describeStatistics(UDRInvocationInfo &info);

  // this method is only used to set the cost
  void describeDesiredDegreeOfParallelism(UDRInvocationInfo &info,
                                          UDRPlanInfo &plan);

  // override the runtime method
  void processData(UDRInvocationInfo &info,
                   UDRPlanInfo &plan);
};

extern "C" UDR * SESSIONIZE_DYNAMIC()
{
  return new Sessionize();
}

// Define data that gets passed between compiler phases
class InternalColumns : public UDRWriterCompileTimeData
{
public:
  InternalColumns(int idCol, int tsCol) :
       idCol_(idCol), tsCol_(tsCol) {}

  int getIdColumn() const { return idCol_; }
  int getTsColumn() const { return tsCol_; }

private:

  int idCol_;
  int tsCol_;
};

void Sessionize::describeParamsAndColumns(UDRInvocationInfo &info)
{
  // First, do some validation of the parameters and set
  // PARTITION BY and ORDER BY columns
  int idCol = -1;
  int tsCol = -1;

  // Make sure we have exactly one table-valued input, otherwise
  // generate a compile error
  if (info.getNumTableInputs() != 1)
    throw UDRException(38000,
                       "%s must be called with one table-valued input",
                       info.getUDRName().data());

  // check whether the first two arguments identify
  // an arbitrary column and an exact numeric column
  if (info.par().isAvailable(0))
    {
      const PartitionInfo &queryPartInfo = info.in().getQueryPartitioning();
      PartitionInfo newPartInfo;

      // This will raise an error if the column name
      // specified in the first parameter doesn't exist
      idCol = info.in().getColNum(info.par().getString(0));

      // make sure the query didn't specify a conflicting
      // PARTITION BY clause
      if (queryPartInfo.getType() == PartitionInfo::PARTITION &&
          (queryPartInfo.getNumEntries() != 1 ||
           queryPartInfo.getColumnNum(0) != idCol))
        throw UDRException(38001,
                           "Query PARTITION BY not compatible with id column %s",
                           info.par().getString(0).c_str());

      // Set this user id column as the required PARTITION BY column
      newPartInfo.setType(PartitionInfo::PARTITION);
      newPartInfo.addEntry(idCol);
      info.setChildPartitioning(0, newPartInfo);
    }
  else
    throw UDRException(38001,"First scalar parameter must be a string constant");

  // make sure the second parameter specifies the name of
  // an existing input column of type exact numeric
  if (info.par().isAvailable(1))
    {
      // This will raise an error if the column name
      // specified in the second parameter doesn't exist
      tsCol = info.in().getColNum(info.par().getString(1));
      const TypeInfo &typ = info.in().getColumn(tsCol).getType();
      const OrderInfo &queryOrderInfo = info.in().getQueryOrdering();
      OrderInfo newOrderInfo;

      if (typ.getSQLTypeSubClass() != TypeInfo::EXACT_NUMERIC_TYPE)
        throw UDRException(38002, "Second parameter must be the name of an exact numeric column");

      // check for a conflicting ORDER BY in the query
      if (queryOrderInfo.getNumEntries() > 0 &&
          (queryOrderInfo.getColumnNum(0) != tsCol ||
           queryOrderInfo.getOrderType(0) == OrderInfo::DESCENDING))
        throw UDRException(
             38900,
             "Query ORDER BY conflicts with specified timestamp column %s",
             info.par().getString(1).c_str());

      // make a new ORDER BY clause with just the timestamp column
      newOrderInfo.addEntry(tsCol);
      info.setChildOrdering(0, newOrderInfo);
    }
  else
    throw UDRException(38003,"Second scalar parameter must be a string constant");
 
  // To demonstrate state that gets passed between compiler phases and
  // to avoid looking up the id column and timestamp column each time,
  // store those as UDR Writer data in the UDRInvocationInfo object
  /* TBD: uncomment when this is allowed
  info.setUDRWriterCompileTimeData(new InternalColumns(idCol, tsCol));
  */

  // Second, define the output parameters

  // add the columns for session id and sequence number
  // (sequence_no is a unique sequence number within the session)
  info.out().addLongColumn("SESSION_ID");  // column number 0
  info.out().addLongColumn("SEQUENCE_NO"); // column number 1
 
  // Make all the input table columns also output columns,
  // those are called "pass-through" columns. The default
  // parameters of this method add all the columns of the
  // first input table.
  info.addPassThruColumns();

  // set the function type, sessionize behaves like a reducer in
  // MapReduce. Session ids are local within rows that share the
  // same id column value.
  info.setFuncType(UDRInvocationInfo::REDUCER);
}

void Sessionize::describeDataflowAndPredicates(UDRInvocationInfo &info)
{
  // Start with the default behavior for a reducer, pushing down
  // any predicates on the key/id column.
  UDR::describeDataflowAndPredicates(info);

  // Make sure we don't require any unused passthru columns
  // from the child/input table. NOTE: This can change the
  // column numbers for our id and timestamp columns!
  info.setUnusedPassthruColumns();

  // That could have set our timestamp column or user id
  // column as unused, however. So, make sure these two
  // columns are definitely included.

  // first, recompute the id and timestamp column numbers
  InternalColumns state(
         info.in().getColNum(info.par().getString(0)),
         info.in().getColNum(info.par().getString(1)));

  // then include the columns
  info.setChildColumnUsage(0, state.getIdColumn(), ColumnInfo::USED);
  info.setChildColumnUsage(0, state.getTsColumn(), ColumnInfo::USED);

  bool generatedColsAreUsed =
    (info.out().getColumn(0).getUsage() == ColumnInfo::USED ||
     info.out().getColumn(1).getUsage() == ColumnInfo::USED);

  // Walk through predicates and find additional ones to push down
  // or to evaluate locally
  for (int p=0; p<info.getNumPredicates(); p++)
    {
      if (!generatedColsAreUsed)
        {
          // If session_id/sequence_no are not used in the query, then
          // we can push all predicates to the children.
          info.setPredicateEvaluationCode(p, PredicateInfo::EVALUATE_IN_CHILD);
        }
      else if (info.isAComparisonPredicate(p))
        {
          // For demo purposes, accept predicates of the
          // form "session_id < const" to be evaluated in the UDF.
          const ComparisonPredicateInfo &cpi = info.getComparisonPredicate(p);

          if (cpi.getColumnNumber() == 0 /* SESSION_ID */ &&
              cpi.getOperator() == PredicateInfo::LESS &&
              cpi.hasAConstantValue())
            info.setPredicateEvaluationCode(p, PredicateInfo::EVALUATE_IN_UDF);
        }
    }
}

void Sessionize::describeConstraints(UDRInvocationInfo &info)
{
  // The sessionize UDF produces at most one result row for every input
  // row it reads. This means it can propagate certain constraints on
  // its input tables to the result.
  info.propagateConstraintsFor1To1UDFs(false);

  // The id column, together with session id and sequence_no, form a unique key.
  // Generate a uniqueness constraint for that.

  UniqueConstraintInfo uc;

  uc.addColumn(info.out().getColNum(info.par().getString(0)));
  uc.addColumn(0); // the session id is alway column #0
  uc.addColumn(1); // the sequence number alway column #1
  info.out().addUniquenessConstraint(uc);

}

void Sessionize::describeStatistics(UDRInvocationInfo &info)
{
  // We set the function type to REDUCER earlier. The Trafodion compiler
  // estimates one output row per input partition for reducer function,
  // unless the UDF specifies another value. Since our sessionize UDF
  // returns one output row per input row, make sure the optimizer has
  // a better cardinality estimate.

  // Crude estimate, assume each predicate evaluated by the UDF
  // reduces the number of output columns by 50%. At this point, only
  // predicates that are evaluated by the UDF are left in the list.
  double selectivity = pow(2,-info.getNumPredicates());
  long resultRowCount =
    static_cast<long>(info.in().getEstimatedNumRows() * selectivity);

  info.out().setEstimatedNumRows(resultRowCount);
}

void Sessionize::describeDesiredDegreeOfParallelism(UDRInvocationInfo &info,
                                                    UDRPlanInfo &plan)
{
  // call the base class method for determining DoP
  UDR::describeDesiredDegreeOfParallelism(info, plan);

  // add cost info, not necessarily realistic
  plan.setCostPerRow(999999);
}

void Sessionize::processData(UDRInvocationInfo &info,
                             UDRPlanInfo &plan)
{
  // read the three parameters and convert the first two into column numbers
  int userIdColNum    = info.in(0).getColNum(info.par().getString(0));
  int timeStampColNum = info.in(0).getColNum(info.par().getString(1));
  long timeout        = info.par().getLong(2);

  // variables needed for computing the session id
  long lastTimeStamp = 0;
  std::string lastUserId;
  long currSessionId = 1;
  long currSequenceNo = 1;
  int maxSessionId = 999999999;

  if (info.getNumPredicates() > 0)
    {
      // based on the describeDataflowAndPredicates() method, this must be
      // a predicate of the form SESSION_ID < const that we need
      // to evaluate inside this method
      std::string maxValue = info.getComparisonPredicate(0).getConstValue();

      sscanf(maxValue.c_str(), "%d", &maxSessionId);
    }

  // loop over input rows
  while (getNextRow(info))
  {
    long timeStamp = info.in(0).getLong(timeStampColNum);
    std::string userId = info.in(0).getString(userIdColNum);

    if (lastUserId != userId)
      {
        // reset timestamp check and start over with session id 0
        lastTimeStamp = 0;
        currSessionId = 1;
        currSequenceNo = 1;
        lastUserId = userId;
      }

    long tsDiff = timeStamp - lastTimeStamp;

    if (tsDiff > timeout && lastTimeStamp > 0)
      {
        currSessionId++;
        currSequenceNo = 1;
      }
    else if (tsDiff < 0)
      throw UDRException(
           38001,
           "Got negative or descending timestamps %ld, %ld",
           lastTimeStamp, timeStamp);

    lastTimeStamp = timeStamp;

    // this evaluates the SQL predicate on SESSION_ID
    if (currSessionId < maxSessionId)
      {
        // produce session_id and sequence_no output columns
        info.out().setLong(0, currSessionId);
        info.out().setLong(1, currSequenceNo);

        // produce the remaining columns and emit the row
        info.copyPassThruData();
        emitRow(info);
        currSequenceNo++;
      }
   }
}

class FibonacciUDF : public UDR
{
  // override any methods where the UDF author would
  // like to change the default behavior

  void processData(UDRInvocationInfo &info,
                   UDRPlanInfo &plan);

};

extern "C" UDR * Fibonacci()
{
  return new FibonacciUDF();
}

void FibonacciUDF::processData(UDRInvocationInfo &info,
                               UDRPlanInfo &plan)
{
  // input parameters: (int startRow, int numResultRows)
  int startRow = info.par().getInt(0);
  int numResultRows = info.par().getInt(1);
  long fibonacciNumber = 0;
  long previousResult = 1;
  long temp = 0;
  int ordinal=0;

  // produce fibonacci numbers and emit rows
  // ---------------------------------------
  while (1)
    {
      if (ordinal >= startRow)
        {
          // set result parameters (int ordinal, long fibonacci_number)
          info.out().setInt(0, ordinal);
          info.out().setLong(1, fibonacciNumber);
          emitRow(info);
        }

      // did we produce numResultRows already?
      if (++ordinal >= startRow+numResultRows)
        break;

      if (fibonacciNumber > std::numeric_limits<long>::max()/2)
        throw UDRException(38001, "Upper limit exceeded");

      // pre-compute the next row
      temp = fibonacciNumber;
      fibonacciNumber += previousResult;
      previousResult = temp;
    }
}
