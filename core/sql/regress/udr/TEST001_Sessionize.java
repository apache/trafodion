/**********************************************************************
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
**********************************************************************/

import org.trafodion.sql.udr.*;

class TEST001_Sessionize extends UDR
{
    // Define data that gets passed between compiler phases
    static class InternalColumns extends UDRWriterCompileTimeData
    {
        public InternalColumns(int idCol, int tsCol)
            {
                idCol_ = idCol;
                tsCol_ = tsCol;
            }

        public int getIdColumn() { return idCol_; }
        public int getTsColumn() { return tsCol_; }

        private final int idCol_;
        private final int tsCol_;
    };

    public TEST001_Sessionize()
    {}

    boolean generatedColumnsAreUsed(UDRInvocationInfo info)
    {
        boolean result = false;

        try {
            if (info.out().getColumn("SESSION_ID").getUsage() ==
                ColumnInfo.ColumnUseCode.USED)
                result = true;
        }
        catch (UDRException e) {
        }

        try {
            if (info.out().getColumn("SEQUENCE_NO").getUsage() ==
                ColumnInfo.ColumnUseCode.USED)
                result = true;
        }
        catch (UDRException e) {
        }

        return result;
    }

    // determine output columns dynamically at compile time
    @Override
    public void describeParamsAndColumns(UDRInvocationInfo info)
        throws UDRException
    {
        // First, do some validation of the parameters and set
        // PARTITION BY and ORDER BY columns
        int idCol = -1;
        int tsCol = -1;

        // Make sure we have exactly one table-valued input, otherwise
        // generate a compile error
        if (info.getNumTableInputs() != 1)
            throw new UDRException(38000,
                                   "%s must be called with one table-valued input",
                                   info.getUDRName());

        // check whether the first two arguments identify
        // an arbitrary column and an exact numeric column
        if (info.par().isAvailable(0))
            {
                PartitionInfo queryPartInfo = info.in().getQueryPartitioning();
                PartitionInfo newPartInfo = new PartitionInfo();

                // This will raise an error if the column name
                // specified in the first parameter doesn't exist
                idCol = info.in().getColNum(info.par().getString(0));

                // make sure the query didn't specify a conflicting
                // PARTITION BY clause
                if (queryPartInfo.getType() == PartitionInfo.PartitionTypeCode.PARTITION &&
                    (queryPartInfo.getNumEntries() != 1 ||
                     queryPartInfo.getColumnNum(0) != idCol))
                    throw new UDRException(38001,
                                           "Query PARTITION BY not compatible with id column %s",
                                           info.par().getString(0));

                // Set this user id column as the required PARTITION BY column
                newPartInfo.setType(PartitionInfo.PartitionTypeCode.PARTITION);
                newPartInfo.addEntry(idCol);
                info.setChildPartitioning(0, newPartInfo);
            }
        else
            throw new UDRException(38001,"First scalar parameter must be a string constant");

        // make sure the second parameter specifies the name of
        // an existing input column of type exact numeric
        if (info.par().isAvailable(1))
            {
                // This will raise an error if the column name
                // specified in the second parameter doesn't exist
                tsCol = info.in().getColNum(info.par().getString(1));
                TypeInfo typ = info.in().getColumn(tsCol).getType();
                OrderInfo queryOrderInfo = info.in().getQueryOrdering();
                OrderInfo newOrderInfo = new OrderInfo();

                if (typ.getSQLTypeSubClass() != TypeInfo.SQLTypeSubClassCode.EXACT_NUMERIC_TYPE)
                    throw new UDRException(38002, "Second parameter must be the name of an exact numeric column");

                // check for a conflicting ORDER BY in the query
                if (queryOrderInfo.getNumEntries() > 0 &&
                    (queryOrderInfo.getColumnNum(0) != tsCol ||
                     queryOrderInfo.getOrderType(0) == OrderInfo.OrderTypeCode.DESCENDING))
                    throw new UDRException(
                        38900,
                        "Query ORDER BY conflicts with specified timestamp column %s",
                        info.par().getString(1));

                // make a new ORDER BY clause with just the timestamp column
                newOrderInfo.addEntry(tsCol);
                info.setChildOrdering(0, newOrderInfo);
            }
        else
            throw new UDRException(38003,"Second scalar parameter must be a string constant");
 
        // To demonstrate state that gets passed between compiler phases and
        // to avoid looking up the id column and timestamp column each time,
        // store those as UDR Writer data in the UDRInvocationInfo object
        /* TBD: uncomment when this is allowed
           info.setUDRWriterCompileTimeData(new InternalColumns(idCol, tsCol));
        */

        // Second, define the output parameters

        // add the columns for session id and sequence number
        // (sequence_no is a unique sequence number within the session)
        info.out().addLongColumn("SESSION_ID", false);  // column number 0
        info.out().addLongColumn("SEQUENCE_NO", false); // column number 1
 
        // Make all the input table columns also output columns,
        // those are called "pass-through" columns. The default
        // parameters of this method add all the columns of the
        // first input table.
        info.addPassThruColumns();

        // set the function type, sessionize behaves like a reducer in
        // MapReduce. Session ids are local within rows that share the
        // same id column value.
        info.setFuncType(UDRInvocationInfo.FuncType.REDUCER);
    }


    // eliminate unused columns and help with predicate pushdown
    @Override
    public void describeDataflowAndPredicates(UDRInvocationInfo info)
        throws UDRException
    {
        // Start with the default behavior for a reducer, pushing down
        // any predicates on the key/id column.
        super.describeDataflowAndPredicates(info);

        // Make sure we don't require any unused passthru columns
        // from the child/input table. NOTE: This can change the
        // column numbers for our id and timestamp columns!
        info.setUnusedPassthruColumns();

        // That could have set our timestamp column or user id
        // column as unused, however. So, make sure these two
        // columns are definitely included.

        boolean genColsAreUsed = generatedColumnsAreUsed(info);

        if (genColsAreUsed)
            {
                // first, recompute the id and timestamp column numbers
                InternalColumns state = new TEST001_Sessionize.InternalColumns(
                    info.in().getColNum(info.par().getString(0)),
                    info.in().getColNum(info.par().getString(1)));

                // second, include the id/timestamp columns
                info.setChildColumnUsage(0, state.getIdColumn(), ColumnInfo.ColumnUseCode.USED);
                info.setChildColumnUsage(0, state.getTsColumn(), ColumnInfo.ColumnUseCode.USED);

                // third, if any of the generated columns is needed, then produce them all,
                // this "all or none" approach just makes our life a bit easier
                for (int i=0; i<2; i++)
                  if (info.out().getColumn(i).getUsage() == ColumnInfo.ColumnUseCode.NOT_PRODUCED)
                    info.out().getColumn(i).setUsage(ColumnInfo.ColumnUseCode.NOT_USED);
            }

        // Walk through predicates and find additional ones to push down
        // or to evaluate locally
        for (int p=0; p<info.getNumPredicates(); p++)
          {
            if (!genColsAreUsed)
              {
                  // If session_id/sequence_no are not used in the query, then
                  // we can push all predicates to the children.
                  info.setPredicateEvaluationCode(p, PredicateInfo.EvaluationCode.EVALUATE_IN_CHILD);
              }
            else if (info.isAComparisonPredicate(p))
              {
                  // For demo purposes, accept predicates of the
                  // form "session_id < const" to be evaluated in the UDF.
                  ComparisonPredicateInfo cpi = info.getComparisonPredicate(p);

                  if (cpi.getColumnNumber() == 0 /* SESSION_ID */ &&
                      cpi.getOperator() == PredicateInfo.PredOperator.LESS &&
                      cpi.hasAConstantValue())
                      info.setPredicateEvaluationCode(p, PredicateInfo.EvaluationCode.EVALUATE_IN_UDF);
              }
          }
    }

    // generate constraints for the table-valued result
    @Override
    public void describeConstraints(UDRInvocationInfo info)
        throws UDRException
    {
        // The sessionize UDF produces at most one result row for every input
        // row it reads. This means it can propagate certain constraints on
        // its input tables to the result.
        info.propagateConstraintsFor1To1UDFs(false);

        if (generatedColumnsAreUsed(info))
          {
            // The id column, together with session id and
            // sequence_no, form a unique key.  Generate a
            // uniqueness constraint for that.

            UniqueConstraintInfo uc = new UniqueConstraintInfo();

            uc.addColumn(info.out().getColNum(info.par().getString(0)));
            uc.addColumn(0); // the session id is always column #0
            uc.addColumn(1); // the sequence number is always column #1
            info.out().addUniquenessConstraint(uc);
          }
    }

    // estimate result cardinality
    @Override
    public void describeStatistics(UDRInvocationInfo info)
        throws UDRException
    {
        // We set the function type to REDUCER earlier. The Trafodion compiler
        // estimates one output row per input partition for reducer function,
        // unless the UDF specifies another value. Since our sessionize UDF
        // returns one output row per input row, make sure the optimizer has
        // a better cardinality estimate.

        // Crude estimate, assume each predicate evaluated by the UDF
        // reduces the number of output columns by 50%. At this point, only
        // predicates that are evaluated by the UDF are left in the list.
        double selectivity = Math.pow(2,-info.getNumPredicates());
        long resultRowCount =
            (long) (info.in().getEstimatedNumRows() * selectivity);

        info.out().setEstimatedNumRows(resultRowCount);
    }

    // this method is only used to set the cost
    @Override
    public void describeDesiredDegreeOfParallelism(UDRInvocationInfo info,
                                                   UDRPlanInfo plan)
        throws UDRException
    {
        // call the base class method for determining DoP
        super.describeDesiredDegreeOfParallelism(info, plan);

        // add cost info, not necessarily realistic
        plan.setCostPerRow(999999);
    }

    // override the runtime method
    @Override
    public void processData(UDRInvocationInfo info,
                            UDRPlanInfo plan)
        throws UDRException
    {
        boolean genColsAreUsed = generatedColumnsAreUsed(info);

        // read the three parameters and convert the first two into column numbers
        int userIdColNum    = -1;
        int timeStampColNum = -1;
        long timeout        = -1;

        if (genColsAreUsed)
          {
            // read the three parameters and convert the first two into column numbers
            userIdColNum    = info.in(0).getColNum(info.par().getString(0));
            timeStampColNum = info.in(0).getColNum(info.par().getString(1));
            timeout         = info.par().getLong(2);
          }

        // variables needed for computing the session id
        long lastTimeStamp = 0;
        String lastUserId = "";
        long currSessionId = 1;
        long currSequenceNo = 1;
        int maxSessionId = 999999999;

        if (info.getNumPredicates() > 0)
          {
            // based on the describeDataflowAndPredicates() method, this must be
            // a predicate of the form SESSION_ID < const that we need
            // to evaluate inside this method
            maxSessionId = Integer.parseInt(info.getComparisonPredicate(0).getConstValue());
          }

        // loop over input rows
        while (getNextRow(info))
          {
            if (genColsAreUsed)
              {
                long timeStamp = info.in(0).getLong(timeStampColNum);
                String userId = info.in(0).getString(userIdColNum);

                if (lastUserId.compareTo(userId) != 0)
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
                  throw new UDRException(
                    38001,
                    "Got negative or descending timestamps %ld, %ld",
                    lastTimeStamp, timeStamp);

                lastTimeStamp = timeStamp;
              }

            // this evaluates the SQL predicate on SESSION_ID
            if (currSessionId < maxSessionId)
              {
                if (genColsAreUsed)
                  {
                    // produce session_id and sequence_no output columns
                    info.out().setLong(0, currSessionId);
                    info.out().setLong(1, currSequenceNo);
                  }

                // produce the remaining columns and emit the row
                info.copyPassThruData();
                emitRow(info);
                currSequenceNo++;
              }
          }
    }
}
