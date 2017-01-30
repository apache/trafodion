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

package org.trafodion.sql.udr;

import java.util.Iterator;
import java.io.File;
import java.io.IOException;

/**
 * This class represents the code associated with a UDR.
 * <p>
 * UDR writers can create a derived class and implement these methods
 * for their specific UDR. The base class also has default methods
 * for all but the runtime call {@link UDR#processData(UDRInvocationInfo, UDRPlanInfo)}. See 
 * https://wiki.trafodion.org/wiki/index.php/Tutorial:_The_object-oriented_UDF_interface
 * for examples.
 * <p>
 * A UDR writer can decide to override none, some or all of the virtual         
 * methods that comprise the complier interaction. The run-time interaction
 * {@link UDR#processData(UDRInvocationInfo, UDRPlanInfo)}, must always be provided.
 * <p>
 * When overriding methods, the UDR writer has the option to call the default
 * method to do part of the work, and then to implement additional logic.
 * <p>
 * Multiple UDRs could share the same subclass of UDR. The UDR name is passed
 * in UDRInvocationInfo, so the logic can depend on the name.
 * <p>
 * A single query may invoke the same UDR more than once. A different
 * UDRInvocationInfo object will be passed for each such invocation.
 * <p>
 * The UDR object or the object of its derived class may be reused for
 * multiple queries, so its life time can exceed that of a UDRInvocation
 * object.
 * <p>
 * Different instances of UDR (or derived class)objects will be created
 * in the processes that compile and execute a query.
 * <p>
 * Based on the previous three bullets, UDR writers should not store state
 * that relates to a UDR invocation in a UDR (or derived) object. There are
 * special classes to do that. It is ok to use the UDR derived class to store
 * resources that are shared between UDR invocations, such as connections to
 * server processes etc. These need to be cleaned up in {@link UDR#close()}.
 * <p>
 * The optimizer may try different execution plans for a UDR invocation, e.g.
 * with different partitioning and ordering of input and/or output data. These
 * alternative plans share the same UDRInvocationInfo object but they will use
 * different UDRPlanInfo objects.
 */
public abstract class UDR
{
    
    /** Default constructor, to be used by derived classes
     */
    public UDR()
    {
    };
    
    /** Currently not used.
     *
     *  <p>This might be used in the future as a final call, to allow
     *  the UDR to deallocate any resources it holds, like connections
     *  to other databases. Right now, it is advisable to deallocate
     *  any resources that could cause a leak before exiting the
     *  processData() method.
     */
    public void close()
    {
    };

    // compile time interface for UDRs

    /**
     * First method called during compilation of a TMUDF invocation.
     * <p>
     *  Describe the output columns of a TMUDF, based on a description of
     *  its parameters (including parameter values that are specified as a
     *  constant) and the description of the table-valued input columns.
     * <p>
     *  When the compiler calls this, it will have set up the formal and
     *  actual parameter descriptions as well as an output column
     *  description containing all the output parameters defined in the
     *  CREATE FUNCTION DDL (if any).
     * <p>
     *  This method should do a general check of things it expects that can be
     *  validated at this time such as input table columns. It should then generate
     *  a description of the table-valued output columns, if applicable
     *  and if the columns provided at DDL time are not sufficient. The
     *  "See also" section points to methods to set these values.
     * <p>
     *  Columns of the table-valued output can be declard as "pass-thru"
     *  columns to make many optimizations simpler.
     * <p>
     *  This method must also add or alter the formal parameter list
     *  to match the list of actual parameters.
     * <p>
     *  The default implementation does nothing. If this method is not used, all
     *  parameters and result table columns must be declared in the
     *  CREATE TABLE MAPPING FUNCTION DDL.
     
     *  @see UDRInvocationInfo#addFormalParameter(ColumnInfo)
     *  @see UDRInvocationInfo#setFuncType(FuncType)
     *  @see UDRInvocationInfo#addPassThruColumns()
     *  @see TupleInfo#addColumn(ColumnInfo)
     *  @see TupleInfo#addIntColumn(String, boolean)
     *  @see TupleInfo#addLongColumn(String, boolean)
     *  @see TupleInfo#addCharColumn(String, int, boolean, TypeInfo.SQLCharsetCode, TypeInfo.SQLCollationCode)
     *  @see TupleInfo#addVarCharColumn(String, int, boolean, TypeInfo.SQLCharsetCode, TypeInfo.SQLCollationCode)
     *  @see TupleInfo#addColumns(Vector)
     *  @see TupleInfo#addColumnAt(ColumnInfo, int)
     *  @see TupleInfo#deleteColumn(int)
     *  @see TupleInfo#deleteColumn(String)
     *
     *  @param info A description of the UDR invocation.
     *  @throws UDRException If an exception occured in the UDR
     */
    public void describeParamsAndColumns(UDRInvocationInfo info)
        throws UDRException
    {
        
    };


    /**
     * Eliminate unneeded columns and decide where to execute predicates.
     * <p>
     * This is the second call in the compiler interaction, after
     * describeParamsAndColumns(). When the compiler calls this, it will have
     * marked the UDF result columns with a usage code, indicating any output
     * columns that are not required for this particular query. It will also have
     * created a list of predicates that need to be evaluated.
     * <p>
     * This method can mark any of the columns of the table-valued inputs as not
     * used, based on the result column usage and internal needs of the UDF. It can
     * also decide where to evaluate each predicate, a) on the UDF result,
     * b) inside the UDF and c) in the table-valued inputs.
     * <p>
     * The default implementation does not mark any of the table-valued input
     * columns as unused. Predicate handling in the default implementation
     * depends on the function type:
     * <ul>
     * <li> 
     * GENERIC: No predicates are pushed down, because the compiler does not
     *          know whether any of the eliminated rows might have altered the
     *          output of the UDF. One example is the "sessionize" UDF, where
     *          eliminated rows can lead to differences in session ids.
     * </li>
     * <li>
     * MAPPER:  All predicates on pass-thru columns are pushed down to table-valued
     *          inputs. Since the UDF carries no state between the input rows it
     *          sees, eliminating any input rows will not alter results for other
     *          rows.
     * </li>
     * <li>
     * REDUCER: Only predicates on the PARTITION BY columns will be pushed to
     *          table-valued inputs. These predicates may eliminate entire groups
     *          of rows (partitions), and since no state is carried between such
     *          groups that is valid.
     * </li>
     * </ul>
     * <p>
     *  @see ColumnInfo#getUsage()
     *  @see UDRInvocationInfo#setFuncType(FuncType)
     *  @see UDRInvocationInfo#setChildColumnUsage(int, int, ColumnInfo.ColumnUseCode)
     *  @see UDRInvocationInfo#setUnusedPassthruColumns()
     *  @see UDRInvocationInfo#pushPredicatesOnPassthruColumns()
     *  @see UDRInvocationInfo#setPredicateEvaluationCode(int, PredicateInfo.EvaluationCode)
     *
     *  @param info A description of the UDR invocation.
     *  @throws UDRException If an exception occured in the UDR
     */
    public void describeDataflowAndPredicates(UDRInvocationInfo info)  
        throws UDRException
    {
        switch (info.getFuncType())
        {
        case GENERIC:
            break;

        case MAPPER:
            // push as many predicates as possible to the children
            info.pushPredicatesOnPassthruColumns();
            break;

        case REDUCER:
        {
            int partitionedChild = -1;

            // find a child that uses a PARTITION BY
            for (int c=0; c<info.getNumTableInputs(); c++)
                if (info.in(c).getQueryPartitioning().getType() ==
                    PartitionInfo.PartitionTypeCode.PARTITION)
                    partitionedChild = c;

            if (partitionedChild >= 0)
            {
                final PartitionInfo partInfo =
                    info.in(partitionedChild).getQueryPartitioning();
                int numPredicates = info.getNumPredicates();

                // walk through all comparison predicates
                for (int p=0; p<numPredicates; p++)
                    if (info.isAComparisonPredicate(p))
                    {
                        // a predicate on column "predCol"
                        int predCol = 
                            info.getComparisonPredicate(p).getColumnNumber();

                        // check whether predCol appears in the PARTITION BY clause
                        Iterator<Integer> it = partInfo.getPartCols().iterator();
                        while (it.hasNext())
                            if (predCol == it.next().intValue())
                                // yes, this is a predicate on a partitioning column,
                                // push it down if possible
                                info.pushPredicatesOnPassthruColumns(p,-1);
                    }
            }
        }
            break;

        default:
            throw new UDRException(
                                   38900,
                                   "Invalid UDR Function type: %s",
                                   info.getFuncType().name());
        }
    };

    /**
     *  
     *  @param info A description of the UDR invocation.
     *  @throws UDRException If an exception occured in the UDR
     */
    public void describeConstraints(UDRInvocationInfo info)
        throws UDRException
    {
    };

    /**
     *  Fourth method of the compiler interface (optional).
     *
     *  <p>Set up statistics for the table-valued result.
     *
     *  <p>When the optimizer calls this method, it will have synthesized
     *  some statistics for the table-valued inputs, if any. The UDR
     *  writer can now indicate the estimated row count for the table-valued
     *  result and estimated number of unique values for the output columns.
     *
     *  <p>The default implementation does nothing. If no estimated cardinality
     *  is set for the output table and no estimated number of unique values
     *  is set for output columns, the optimizer will make default assumptions.
     *  Here are some of these default assumptions:
     *  <ul>
     *  <li>UDRs of type UDRInvocationInfo.MAPPER return one output row for
     *      each row in their largest input table.
     *  <li>UDRs of type UDRInvocationInfo.REDUCER and REDUCER_NC return one
     *      output row for every partition in their largest partitioned input
     *      table.
     *  <li>For output columns that are passthru columns, the estimated
     *      unique entries are the same as for the underlying column in the
     *      table-valued input.
     *  <li>Other default cardinality and unique entry counts can be influenced
     *      with defaults (CONTROL QUERY DEFAULT) in Trafodion SQL.
     *  </ul>
     *
     *  @see UDRInvocationInfo#setFuncType
     *  @see ColumnInfo#getEstimatedUniqueEntries
     *  @see ColumnInfo#setEstimatedUniqueEntries
     *  @see TableInfo#getEstimatedNumRows
     *  @see TableInfo#setEstimatedNumRows
     *  @see TableInfo#getEstimatedNumPartitions
     *
     *  @param info A description of the UDR invocation.
     *  @throws UDRException
     */
    public void describeStatistics(UDRInvocationInfo info)
        throws UDRException
    {
    };
    
    /**
     *  Describe the desired parallelism of a UDR.
     *  <p>
     *  This method can be used to specify a desired degree of
     *  parallelism, either in absolute or relative terms.
     *  <p>
     *  The default behavior is to allow any degree of parallelism for
     *  TMUDFs of function type UDRInvocationInfo.MAPPER or
     *  UDRInvocationInfo.REDUCER (or REDUCER_NC) that have exactly
     *  one table-valued input. The default behavior forces serial
     *  execution in all other cases. The reason is that for a single
     *  table-valued input, there is a natural way to parallelize the
     *  function by parallelizing its input a la MapReduce. In all
     *  other cases, parallel execution requires active participation
     *  by the UDF, which is why the UDF needs to signal explicitly
     *  that it can handle such flavors of parallelism.
     *
     *  Default implementation:
     *  <pre>
        if (info.getNumTableInputs() == 1 &&
            (info.getFuncType() == UDRInvocationInfo.FuncType.MAPPER ||
             info.getFuncType() == UDRInvocationInfo.FuncType.REDUCER ||
             info.getFuncType() == UDRInvocationInfo.FuncType.REDUCER_NC))
          plan.setDesiredDegreeOfParallelism(UDRPlanInfo.ANY_DEGREE_OF_PARALLELISM);
        else
          plan.setDesiredDegreeOfParallelism(1); // serial execution
        </pre>
     * <p>
     *  Note that this is NOT foolproof, and that the TMUDF might still
     *  need to validate the PARTITION BY and ORDER BY syntax used in its
     *  invocation.
     *  <p> Note also that in order to get parallel execution, you may need to
     *  implement the {@link UDR#describeStatistics} interface and provide a
     *  cardinality estimate. Alternatively, you can set the
     *  PARALLEL_NUM_ESPS CQD.
     * <p>
     *  @see UDRPlanInfo#getDesiredDegreeOfParallelism
     *  @see UDRPlanInfo#setDesiredDegreeOfParallelism
     *  @see UDRInvocationInfo#getNumParallelInstances
     *  @see UDRInvocationInfo#setFuncType
     *  @see UDR#describeStatistics
     *  @see TableInfo#setEstimatedNumRows
     *
     *  @param info A description of the UDR invocation.
     *  @param plan Plan-related description of the UDR invocation.
     *  @throws UDRException If an exception occured in the UDR
     */
    public void describeDesiredDegreeOfParallelism(UDRInvocationInfo info,
                                                   UDRPlanInfo plan)
         throws UDRException
    {
        if (info.getNumTableInputs() == 1 &&
            (info.getFuncType() == UDRInvocationInfo.FuncType.MAPPER ||
             info.getFuncType() == UDRInvocationInfo.FuncType.REDUCER ||
             info.getFuncType() == UDRInvocationInfo.FuncType.REDUCER_NC))
            plan.setDesiredDegreeOfParallelism(UDRPlanInfo.SpecialDegreeOfParallelism.
                                               ANY_DEGREE_OF_PARALLELISM.getSpecialDegreeOfParallelism());
        else
            plan.setDesiredDegreeOfParallelism(1); // serial execution
    };

    /**
     *  Sixth method of the compiler interface (optional).
     *
     *  The query optimizer calls this method once for every plan alternative
     *  considered for a UDR invocation. It provides the required partitioning
     *  and ordering of the result. The UDR writer can decide whether these
     *  requirements are acceptable to the UDR and whether any partitioning
     *  or ordering of the table-valued inputs is required to produce the required
     *  result properties.
     *
     *  <p>This interface is currently not used.
     *  
     *  @param info A description of the UDR invocation.
     *  @param plan Plan-related description of the UDR invocation.
     *  @throws UDRException
     */
    public void describePlanProperties(UDRInvocationInfo info,
                                       UDRPlanInfo plan)
        throws UDRException 
    { 
    };
    
    /**
     *  Final call of the compiler interaction for TMUDFs.
     *  <p>
     *  This final compile time call gives the UDF writer the opportunity
     *  to examine the chosen query plan, to pass information on to the
     *  runtime method, using {@link UDRPlanInfo#addPlanData(byte[]) addPlanData}, and to
     *  clean up any resources related to the compile phase of a particular TMUDF
     *  invocation.
     *  <p>
     *  The default implementation does nothing.
     *  <p>
     *  @see UDRPlanInfo#addPlanData(byte[])
     *  @see UDRPlanInfo#getUDRWriterCompileTimeData()
     *  @see UDRInvocationInfo#getUDRWriterCompileTimeData()
     * 
     *  @param info A description of the UDR invocation.
     *  @param plan Plan-related description of the UDR invocation.
     *  @throws UDRException If an exception occured in the UDR
     */
    public void completeDescription(UDRInvocationInfo info,
                                    UDRPlanInfo plan)
        throws UDRException 
    { 
    };

    // run time interface for TMUDFs and scalar UDFs (once supported)

    /**
     *  Runtime code for UDRs.
     * <p>
     * This method must be implemented in derived class.
     * 
     *  @param info A description of the UDR invocation.
     *  @param plan Plan-related description of the UDR invocation.
     *  @throws UDRException If an exception occured in the UDR
     */
    public abstract void processData(UDRInvocationInfo info,
                                     UDRPlanInfo plan)
         throws UDRException;

    // methods to be called from the run time interface for UDRs:

    // these match the SQLUDR_Q_STATE enum in file core/sqludr/sqludr.h
    public final int SQLUDR_Q_MORE   = 1;
    public final int SQLUDR_Q_EOD    = 2;
    public final int SQLUDR_Q_CANCEL = 3;
    
    public static class QueueStateInfo
    {
        public int queueState_; // one of the three values above

        QueueStateInfo(int v)
        {
            queueState_ = v;
        }
    }

    // native methods used by these calls
    // use this command to regenerate the C headers:
    // javah -d $TRAF_HOME/../sql/langman org.trafodion.sql.udr.UDR 
    private native void SpInfoGetNextRowJava(byte[] rowData,
                                             int tableIndex,
                                             QueueStateInfo queueState);
    private native void SpInfoEmitRowJava(byte[] rowData,
                                          int tableIndex,
                                          QueueStateInfo queueState);

    // read a row from an input table

    /**
     *  Read a row of the first table-value input.
     *  <p>
     *  This method can only be called from within processData().
     *  
     *  @param info A description of the UDR invocation.
     *  @return true if another row could be read, false if it reached end of data.
     *  @throws UDRException If an exception occured in the UDR
     */
    public final boolean getNextRow(UDRInvocationInfo info) throws UDRException
    {
        QueueStateInfo qs = new QueueStateInfo(SQLUDR_Q_MORE);
        int tableIndex = 0; // for now

        SpInfoGetNextRowJava(info.in(tableIndex).getRow().array(), tableIndex, qs);

        // trace rows, if enabled
        if ((info.getDebugFlags() & UDRInvocationInfo.DebugFlags.TRACE_ROWS.flagVal()) != 0)
          switch (qs.queueState_)
            {
            case SQLUDR_Q_MORE:
                traceRow(info, info.in(tableIndex), tableIndex, "(%d) Input row from table %d: %s\n");
                break;
              
            case SQLUDR_Q_EOD:
                System.out.printf("(%d) Input table %d reached EOD\n",
                                  info.getMyInstanceNum(), tableIndex);
                break;
                
            case SQLUDR_Q_CANCEL:
                System.out.printf("(%d) Cancel request from input table %d\n",
                                  info.getMyInstanceNum(), tableIndex);
                break;
                
            default:
                System.out.printf("(%d) Invalid queue state %d from input table %d\n",
                                  info.getMyInstanceNum(), qs.queueState_, tableIndex);
            }

        return (qs.queueState_ == SQLUDR_Q_MORE);
    }

    // produce a result row
    
    /**
     *  Emit a row of a table-valued result.
     *  <p>
     *  This method can only be called from within UDR#processData(UDRInvocationInfo, UDRPlanInfo).
     *  
     *  @param info A description of the UDR invocation.
     * 
     *  @throws UDRException If an exception occured in the UDR
     */
    public final void emitRow(UDRInvocationInfo info) throws UDRException
    { 
        QueueStateInfo qs = new QueueStateInfo(SQLUDR_Q_MORE);

        // trace rows, if enabled
        if ((info.getDebugFlags() & UDRInvocationInfo.DebugFlags.TRACE_ROWS.flagVal()) != 0)
            traceRow(info, info.out(), 0, "(%1$d) Emitting row: %3$s\n");

        SpInfoEmitRowJava(info.out().getRow().array(),
                          0,
                          qs);
    }

    // methods for debugging
    /**
     *  Debugging hook for UDRs.
     *
     *  This method is called in debug Trafodion builds when certain
     *  flags are set in the UDR_DEBUG_FLAGS CQD (CONTROL QUERY DEFAULT).
     *  See https://wiki.trafodion.org/wiki/index.php/Tutorial:_The_object-oriented_UDF_interface#Debugging_UDF_code
     *  for details.
     *
     *  The default implementation prints out the process id and then
     *  goes into an endless loop. The UDF writer can then attach a
     *  debugger, set breakpoints and force the execution out of the loop.
     *
     *  Note that the printout of the pid may not always be displayed on
     *  a terminal, for example if the process is executing on a different node.
     */
    public final void debugLoop()
    {
        int debugLoop = 1;
        int myPid = 0;
        try {myPid = Integer.parseInt(new File("/proc/self").getCanonicalFile().getName());}
        catch (IOException e1) {}
        
        System.out.print(String.format("Process %d entered a loop to be able to debug it\n", myPid));
        
        // go into a loop to allow the user to attach a debugger,
        // if requested, set debugLoop = 2 in the debugger to get out
        while (debugLoop < 2)
            debugLoop = 1-debugLoop;
        
    };

    public void traceRow(UDRInvocationInfo info,
                         TupleInfo ti,
                         int tableIndex,
                         String formattedMsg) throws UDRException
    {
        String row = ti.getDelimitedRow('|',true, '"', 0, -1);
        int numHexChars = 0;
        
        // replace any control characters with escape sequences
        for (int c=0; c<row.length(); c++)
            if (row.charAt(c) < ' ')
                numHexChars++;

        if (numHexChars > 0)
            {
                // each char below 32 gets replaced by 4 characters \xnn
                StringBuilder sb =  new StringBuilder(row.length() + 3*numHexChars);
                for (int i=0; i<row.length(); i++)
                    if (row.charAt(i) < ' ')
                        sb.append(String.format("\\x%02x", (byte) row.charAt(i)));
                    else
                        sb.append(row.charAt(i));
                row = sb.toString();
            }
        System.out.printf(formattedMsg,
                          info.getMyInstanceNum(), tableIndex, row);
    }

    // methods for versioning of this interface
    public final int getCurrentVersion() { return 1; }

    /**
     *  For versioning, return features supported by the UDR writer.
     * <p>
     *  This method can be used in the future to facilitate changes in
     *  the UDR interface. UDR writers will be able to indicte through this
     *  method whether they support new features.
     * <p>
     *  The default implementation returns 0 (no extra features are supported).
     * <p>
     *  @return A yet to be determined set of bit flags or codes for supported features.
     */
    public int getFeaturesSupportedByUDF(){ return 0; }
    
}