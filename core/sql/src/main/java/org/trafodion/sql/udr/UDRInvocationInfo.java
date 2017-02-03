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

import java.util.Vector;
import java.nio.ByteBuffer;


 /**
   *  Describes an invocation of a UDR
   *  
   *  <p> This combines the description of the UDR, its names and
   *  properties with the parameters, input and output table layouts
   *  and other information. An object of this class is passed to
   *  most methods defined by the UDR writer. It can be used to
   *  get input and parameter data and to set values of table-valued
   *  output columns.
  */

public class UDRInvocationInfo extends TMUDRSerializableObject
{

    /**
     *  Type of a TMUDF: Generic, Mapper or Reducer.
     */
    public enum FuncType  // The type of this UDF.
    {
        /** The Trafodion compiler will make only the most conservative 
         * assumptions about this type of UDF. */
        GENERIC, 
        /** A UDF that behaves like a mapper. A mapper does not
         *  carry any state between rows it reads from its
         *  table-valued inputs. It produces zero or more output
         *  rows per input row. Because no state is kept between
         *  rows, the Trafodion compiler can automatically
         *  parallelize execution and push predicates down to
         *  the table-valued inputs. */
        MAPPER,  
        /**  A reducer requires the data to be partitioned on
         * a set of columns. The UDF does not carry any state
         * between groups of rows with the same partition column
         * values, but it may carry state within such groups.
         * This allows the compiler to parallelize execution and
         * to push predicates on the partitioning column(s) down
         * to table-valued inputs. */
            REDUCER,
        /**  Same as REDUCER, except that in this case the
         * UDF does not require the rows belonging to a key
         * to be grouped together, they can be non-contiguous
         * (NC). This can avoid a costly sort of the input
         * table in cases where a highly reducing UDF can keep
         * a table of all the keys in memory. */
        REDUCER_NC;

        private static FuncType[] allValues = values();
        public static FuncType fromOrdinal(int n) {return allValues[n];}

    };

    /**
     *  Type of SQL operations done in the UDF
     */
    public enum SQLAccessType
    {
        /** The UDR contains no SQL operations that use the
         *  default connection to Trafodion. Note that it
         *  may still connect to other JDBC data sources,
         *  including Trafodion with a user id and password. */
        CONTAINS_NO_SQL,
        /** Does read-only SQL operations, implying that
         *  the UDF has no side-effects on the database
         *  through its SQL operations.
         *  NOTE: Currently not supported for Java TMUDFs! */
        READS_SQL,
        /** Potentially modifies SQL tables as a side-effect.
         *  NOTE: Currently not supported for Java TMUDFs! */
        MODIFIES_SQL;

        private static SQLAccessType[] allValues = values();
        public static SQLAccessType fromOrdinal(int n) {return allValues[n];}

    };

    /**
     *  Indicates whether the UDF needs a transaction and
     *  possibly which type of transaction.
     */
    public enum SQLTransactionType
    {
        /** The UDR requires no transaction, typically because
         *  it performs no SQL operations that require one. */
        REQUIRES_NO_TRANSACTION,
        /** The SQL operations done in the UDR may require a
         *  transaction. */
        REQUIRES_SQL_TRANSACTION;

        private static SQLTransactionType[] allValues = values();
        public static SQLTransactionType fromOrdinal(int n) {return allValues[n];}

    };

    /**
     *  Indicates what the effective user id for SQL operations
     *  performed by the UDR.
     */
    public enum SQLRightsType
    {
        /** SQL operations are done with the user id of the
         *  invoker of the UDR.
         *  NOTE: Currently not supported for Java TMUDFs! */
        INVOKERS_RIGHTS,
        /** SQL operations are done with the user id of the
         *  invoker of the UDR.
         *  NOTE: Currently not supported for Java TMUDFs! */
        DEFINERS_RIGHTS;

        private static SQLRightsType[] allValues = values();
        public static SQLRightsType fromOrdinal(int n) {return allValues[n];}

    };

    /**
     *  Indicates the level of trust the system has in the UDR.
     */
    public enum IsolationType
    {
        /** The UDR is not trusted to run in the same process
         *  and with the same user id as the Trafodion engine,
         *  it runs in a separate process and with a different,
         *  less powerful user id.
         *  NOTE: Currently not supported for Java TMUDFs! */
        ISOLATED,
        /** The UDR is fully trusted and can run in processes
         *  of the engine, using the same user id as the engine.
         *  Note that this potentially allows the UDR to break
         *  any SQL security rule, to see any Trafodion users's
         *  data, to corrupt internal data structures and
         *  possibly to read, write and corrupt HDFS and HBase
         *  data, even data that does not belong to Trafodion. */
        TRUSTED;

        private static IsolationType[] allValues = values();
        public static IsolationType fromOrdinal(int n) {return allValues[n];}

    };

    /**
     *  Call phase for the UDR interface
     *  
     *  <p> This is of limited interest for UDR writers and mostly
     *  used internally to ensure method calls are not done at
     *  the wrong time.
     */
    public enum CallPhase
    {
        UNKNOWN_CALL_PHASE           (0),
        COMPILER_INITIAL_CALL       (10),
        COMPILER_DATAFLOW_CALL      (20),
        COMPILER_CONSTRAINTS_CALL   (30),
        COMPILER_STATISTICS_CALL    (40),
        COMPILER_DOP_CALL           (50),
        COMPILER_PLAN_CALL          (60),
        COMPILER_COMPLETION_CALL    (70),
        RUNTIME_WORK_CALL          (110);

        private final int phaseNum;

        CallPhase(int val) {
            this.phaseNum = val;
        }

        public int phaseNum() {
            return phaseNum;
        }

       public static CallPhase getEnum(int x) {
          switch (x) {
          case 0:
            return UNKNOWN_CALL_PHASE;
          case 10:
            return COMPILER_INITIAL_CALL;
          case 20:
            return COMPILER_DATAFLOW_CALL;
          case 30:
            return COMPILER_CONSTRAINTS_CALL;
          case 40:
            return COMPILER_STATISTICS_CALL;
          case 50:
            return COMPILER_DOP_CALL;
          case 60:
            return COMPILER_PLAN_CALL;
          case 70:
            return COMPILER_COMPLETION_CALL;
          case 110:
            return RUNTIME_WORK_CALL;
          default:
            return UNKNOWN_CALL_PHASE;
        }
      };
    }

    /**
     *  Values used for the UDR_DEBUG_FLAGS CQD
     *
     *  <p> use cqd UDR_DEBUG_FLAGS 'num' in SQL to set these, add up
     *  the flags (in decimal) that you want to set
     */
    public enum DebugFlags
    {
        DEBUG_INITIAL_RUN_TIME_LOOP_ONE   (1),
        DEBUG_INITIAL_RUN_TIME_LOOP_ALL   (2),
        DEBUG_INITIAL_COMPILE_TIME_LOOP   (4),
        DEBUG_LOAD_MSG_LOOP               (8),
        TRACE_ROWS                        (16),
        PRINT_INVOCATION_INFO_INITIAL     (32),
        PRINT_INVOCATION_INFO_END_COMPILE (64),
        PRINT_INVOCATION_INFO_AT_RUN_TIME (128),
        VALIDATE_WALLS                    (256);

        private final int flagVal;
        
        DebugFlags(int val) {
            this.flagVal = val;
        }

        public int flagVal() {
            return flagVal; 
        }
    };

    // there are no public constructors for this class

    // const Functions for use by UDR writer, both at compile and at run time
    /**
     *  Get the UDR name.
     *
     *  @return Fully qualified name (catalog.schema.name) of the UDR.
     */
    public String getUDRName() { return name_; }
    /**
     *  Get number of table-valued inputs provided.
     *
     *  @return Number of table-valued inputs provided.
     */
    public int getNumTableInputs() { return numTableInputs_; }
    /**
     *  Get description of a table-valued input.
     *
     *  @param childNum   0-based index of input table required. Currently only the value 0 is allowed.   
     *  @return TableInfo reference for the table-valued input.
     *  @throws UDRException
     */
    public TableInfo in(int childNum) throws UDRException {
        if (childNum < 0 || childNum >= numTableInputs_)
            throw new UDRException(38909, "Invalid child table number %d", childNum);
        return inputTableInfo_[childNum];
    }
    /**
     *  Get description of table-valued input with index 0.
     *
     *  @return TableInfo reference for the table-valued input.
     *  @throws UDRException
     */
    public TableInfo in( ) throws UDRException {
        return in(0) ;
    }
    /**
     *  Get description of the table-valued result.
     *  @return TableInfo reference for the table-valued output.
     */
    public TableInfo out() {
        return outputTableInfo_;
    }
    /**
     *  Get call phase.
     *
     *  <p> This call is not normally needed, since we know which method
     *  of UDR we are in. However, in some cases where the UDR
     *  writer wants to use code in multiple call phases this might
     *  be useful.
     *
     *  @return Enum for the call phase we are in.
     */
    public CallPhase getCallPhase() { return callPhase_; }
    void setCallPhase(int cp) { callPhase_ = CallPhase.getEnum(cp); }
    /**
     *  Get current user.
     *
     *  <p> Get the id of the current user, which is the effective
     *  user id at the time. This is usually the same as
     *  the session user, except when a view or UDR uses "definer
     *  privileges", substituting the current user with the
     *  definer of the view or UDR. In SQL, this value is
     *  called CURRENT_USER.
     *
     *  @see UDRInvocationInfo#getSessionUser()
     *  @return Current user.
     */
    public String getCurrentUser() { return currentUser_; }
    /**
     *  Get session user.
     *
     *  <p> Get the id of the session user, which is the user who
     *  connected to the database. This is usually the same as
     *  the current user, except when a view or UDR uses "definer
     *  privileges", substituting the current user with the
     *  definer of the view or UDR. In SQL, this value is
     *  called SESSION_USER.
     *
     *  @see UDRInvocationInfo#getCurrentUser()
     *  @return Session user.
     */
    public String getSessionUser() { return sessionUser_; }
    /**
     *  Get current role.
     *  @return Current role.
     */
    public String getCurrentRole() { return currentRole_; }
    /**
     *  Get query id.
     *
     *  <p> The query id is only available at runtime. It is an empty
     *  string at compile time.
     *
     *  @return Query id.
     */
    public String getQueryId() { return queryId_; }
    /**
     *  Check whether we are in the compile time interface.
     *  @return true at compile time, false at run-time.
     */
    public boolean isCompileTime() {
        return (callPhase_.ordinal() >= CallPhase.COMPILER_INITIAL_CALL.ordinal() &&
                callPhase_.ordinal() <= CallPhase.COMPILER_COMPLETION_CALL.ordinal());
    }
    /**
     *  Check whether we are in the run-time interface.
     *  @return false at compile time, true at run-time.
     */
    public boolean isRunTime() {
        return (callPhase_.ordinal() >= CallPhase.RUNTIME_WORK_CALL.ordinal());
    }
    /**
     *  Get debugging flags, set via CONTROL QUERY DEFAULT.
     *
     *  <p> Debug flags are set via the UDR_DEBUG_FLAGS CONTROL QUERY DEFAULT
     *  at compile time. This returns the value of this CQD. Usually not
     *  needed.
     *
     *  @return Value the UDR_DEBUG_FLAGS CQD has or had at compile time).
     */
    public int getDebugFlags() { return debugFlags_ ;}
    /**
     *  Get the function type of this UDR invocation.
     *
     *  <p> Returns the function type that can be set by the UDR writer
     *  with the setFuncType() method.
     *
     *  @see UDRInvocationInfo#setFuncType(FuncType)
     *
     *  @return Enum of the function type.
     */
    public FuncType getFuncType() { return funcType_; }
    /**
     *  Get the formal parameters of the UDR invocation.
     *
     *  <p> Formal parameters are available only at compile time.
     *  They are either defined in the CREATE FUNCTION DDL or through
     *  the compile time interface. Note that number and types of formal
     *  and actual parameters must match, once we return from the
     *  describeParamsAndColumns() call, otherwise an error will be generated.
     *
     *  @return Formal parameter description.
     */
    public ParameterListInfo getFormalParameters() {
        return formalParameterInfo_;
    }
    /**
     *  Get parameters of the UDR invocation.
     *
     *  <p> These are the actual parameters. At compile time, if a constant
     *  has been used, the value of this constant is available, using
     *  getString(), getInt() etc. methods. The isAvailable() method indicates
     *  whether the parameter is indeed available at compile time. Parameters
     *  are always available at run-time.
     *
     *  @return Parameter description.
     */
    public ParameterListInfo par() { // actual parameters
        return actualParameterInfo_;
    }
    /**
     *  Return number of predicates to be applied in the context of this UDF.
     *
     *  <p> Don't use this method from within UDR#describeParamsAndColumns(UDRInvocationInfo),
     *  since the predicates are not yet set up in that phase.
     *
     *  @return Number of predicates.
     *  @throws UDRException
     */
    public int getNumPredicates() throws UDRException {
        // predicates are not yet set up in the initial call
        validateCallPhase(CallPhase.COMPILER_DATAFLOW_CALL, 
                          CallPhase.RUNTIME_WORK_CALL,
                          "UDRInvocationInfo.getNumPredicates()");
        return predicates_.size();
    }
    /**
     *  Get the description of a predicate to be applied.
     *
     *  @return Description of the predicate.
     *
     *  @see UDRInvocationInfo#setPredicateEvaluationCode(int, PredicateInfo.EvaluationCode)
     *  @throws UDRException
     */
    public PredicateInfo getPredicate(int i) throws UDRException {
        if (i < 0 || i >= predicates_.size())
            throw new UDRException(
                                   38900,
                                   "Trying to access predicate %d of a PredicateInfo object with %d predicates",
                                   i, predicates_.size());
        
        return predicates_.elementAt(i);
    }
    /**
     *  Check whether a given predicate is a comparison predicate.
     *
     *  <p> This returns whether it is safe to use method getComparisonPredicate().
     *
     *  @see UDRInvocationInfo#getComparisonPredicate(int)
     *
     *  @param i Number/ordinal index of the predicate.
     *  @return true if predcate i is a comparison predicate, false otherwise.
     *  @throws UDRException
     */
    public boolean isAComparisonPredicate(int i) throws UDRException {
         return getPredicate(i).isAComparisonPredicate();
    }
    /**
     *  Get a comparison predicate
     *
     *  <p> Note: This will throw an exception if predicate i is not a
     *  comparison predicate. Use method isAComparisonPredicate() to
     *  make sure this is the case. Note also that the numbering
     *  scheme is the same as that for getPredicate, so if there is
     *  a mix of different predicate types, the numbers of comparison
     *  predicates are not contiguous.
     *
     *  @see UDRInvocationInfo#getPredicate(int)
     *  @see UDRInvocationInfo#isAComparisonPredicate(int)
     *  @param i Number/ordinal of the predicate to retrieve.
     *  @return Comparison predicate.
     *  @throws UDRException
     */
    public ComparisonPredicateInfo getComparisonPredicate(int i) throws UDRException {
        if (!isAComparisonPredicate(i))
            throw new UDRException(38900,
                                   "Predicate %d is not a comparison predicate",
                                   i);
        return (ComparisonPredicateInfo) predicates_.elementAt(i);
    }

    // Functions available at compile time only
    // use the next six only from describeParamsAndColumns()
    /**
     *  Add a formal parameter to match an actual parameter.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns(UDRInvocationInfo) method.
     *
     *  @see UDR#describeParamsAndColumns(UDRInvocationInfo)
     *
     *  @param param Info with name and type of the formal parameter.
     *
     *  @throws UDRException
     */
    public void addFormalParameter(ColumnInfo param) throws UDRException{
        validateCallPhase(CallPhase.COMPILER_INITIAL_CALL, 
                          CallPhase.COMPILER_INITIAL_CALL,
                          "UDRInvocationInfo.addFormalParameter()");
        formalParameterInfo_.addColumn(param);
    }

    /**
     *  Set the function type of this UDR invocation.
     *
     *  <p> Use this simple method with some caution, since it has an effect
     *  on how predicates are pushed down through TMUDFs with table-valued
     *  inputs. See UDR#describeDataflowAndPredicates(UDRInvocationInfo) for details. 
     *  The function type also influences the default degree of parallelism for a TMUDF.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns(UDRInvocationInfo) method.
     *
     *  @see UDRInvocationInfo#getFuncType()
     *  @see UDR#describeParamsAndColumns(UDRInvocationInfo)
     *  @see UDR#describeDataflowAndPredicates(UDRInvocationInfo)
     *  @see UDRPlanInfo#setDesiredDegreeOfParallelism(int)
     *
     *  @param type Function type of this UDR invocation.
     *  @throws UDRException
     */
    public void setFuncType(FuncType type) throws UDRException {
        validateCallPhase(CallPhase.COMPILER_INITIAL_CALL, 
                          CallPhase.COMPILER_INITIAL_CALL,
                          "UDRInvocationInfo.setFuncType()");
        
        funcType_ = type;
        
        // also set the default value for partitioning of table-valued inputs
        // to ANY, if this UDF is a mapper, to allow parallel execution
        // without restrictions
        if (type == FuncType.MAPPER &&
            getNumTableInputs() == 1 &&
            in().getQueryPartitioning().getType() == PartitionInfo.PartitionTypeCode.UNKNOWN)
            inputTableInfo_[0].getQueryPartitioning().setType(PartitionInfo.PartitionTypeCode.ANY);
    }
    /**
     *  Add columns of table-valued inputs as output columns.
     *
     *  <p> Many TMUDFs make the column values of their table-valued inputs available
     *  as output columns. Such columns are called "pass-thru" columns. This
     *  method is an easy interface to create such pass-thru columns. Note that
     *  if a column is marked as pass-thru column, the UDF must copy the input
     *  value to the output (e.g. with the copyPassThruData() method). If it fails
     *  to do that, incorrect results may occur, because the compiler makes
     *  the assumptions that these values are the same.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns(UDRInvocationInfo) method.
     *
     *  @see UDR#describeParamsAndColumns(UDRInvocationInfo)
     *  @see ProvenanceInfo
     *  @see ColumnInfo#getProvenance()
     *
     *  @param inputTableNum    Index of table-valued input to add.
     *  @param startInputColNum First column of the table-valued input to add
     *                          as an output column.
     *  @param endInputColNum   Last column of the table-valued input to add
     *                          as an output column (note this is inclusive)
     *                          or -1 to add all remaining column.
     *  @throws UDRException
     */
    public void addPassThruColumns(int inputTableNum,
                                   int startInputColNum,
                                   int endInputColNum) throws UDRException {
        validateCallPhase(CallPhase.COMPILER_INITIAL_CALL, 
                          CallPhase.COMPILER_INITIAL_CALL,
                          "UDRInvocationInfo.addPassThruColumns()");

        // Adding one or more columns from an input (child) table as output columns
        // The advantage of doing this is that the query optimizer can automatically
        // apply some optimizations:
        //
        // - Push predicates on output columns down to input tables. This reduces the
        //   number of rows that have to be processed by the TMUDF.
        // - If a table-valued input is ordered, the TMUDF output is assumed to be
        //   also ordered on the corresponding columns.
        // - Similar for partitioning.
        // - If there are histogram statistics on an input column, these statistics
        //   will be used for the output columns as well, even though the TMUDF may
        //   eliminate some input rows and duplicate others, so the total row count
        //   and frequencies of values may or may not be usable.
        
        if (endInputColNum == -1)
            endInputColNum = in(inputTableNum).getNumColumns() - 1;
        
        for (int c=startInputColNum; c<=endInputColNum; c++)
         {
             // make a copy of the input column
             outputTableInfo_.addColumn(in(inputTableNum).getColumn(c));
             
             // change the provenance info of the column
             outputTableInfo_.getColumn(outputTableInfo_.getNumColumns()-1).
                 setProvenance(new ProvenanceInfo(inputTableNum, c));
         }
    }
    
    /**
     *  Add all columns of first table-valued input as output columns.
     *
     *  <p> Many TMUDFs make the column values of their table-valued inputs available
     *  as output columns. Such columns are called "pass-thru" columns. This
     *  method is an easy interface to create such pass-thru columns. Note that
     *  if a column is marked as pass-thru column, the UDF must copy the input
     *  value to the output (e.g. with the copyPassThruData() method). If it fails
     *  to do that, incorrect results may occur, because the compiler makes
     *  the assumptions that these values are the same.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns(UDRInvocationInfo) method.
     *
     *  @see UDR#describeParamsAndColumns(UDRInvocationInfo)
     *  @see ProvenanceInfo
     *  @see ColumnInfo#getProvenance()
     *
     *  @throws UDRException
     */
    public void addPassThruColumns( ) throws UDRException {
        addPassThruColumns(0,0,-1);
    }
    
    /**
     *  Set the PARTITION BY info for a table-valued input.
     *
     *  <p> This method allows the UDR writer to override the
     *  PARTITION BY syntax specified for a table-valued input
     *  in the query. Use it to change the required partitioning.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns() method.
     *
     *  @see UDR#describeParamsAndColumns(UDRInvocationInfo)
     *
     *  @param inputTableNum Number of table-valued input to set.
     *  @param partInfo New information on required partitioning for this input table.
     *  @throws UDRException
     */
    public void setChildPartitioning(int inputTableNum,
                                     PartitionInfo partInfo) throws UDRException {
        validateCallPhase(CallPhase.COMPILER_INITIAL_CALL, 
                          CallPhase.COMPILER_INITIAL_CALL,
                          "UDRInvocationInfo::setChildPartitioning()");
        if (inputTableNum < 0 || inputTableNum >= numTableInputs_)
            throw new UDRException(38900, 
                                   "Invalid child table number %d", inputTableNum);

        inputTableInfo_[inputTableNum].setQueryPartitioning(new PartitionInfo(partInfo));
    }
    /**
     *  Set the ORDER BY info for a table-valued input.
     *
     *  <p> This method allows the UDR writer to override the
     *  ORDER BY syntax specified for a table-valued input
     *  in the query. Use it to change the required order.
     *
     *  <p> Only use this method from within the
     *  UDR#describeParamsAndColumns(UDRInvocationInfo) method.
     *
     *  @see UDR#describeParamsAndColumns(UDRInvocationInfo)
     *
     *  @param inputTableNum Number of table-valued input to set.
     *  @param orderInfo New information on required order for this input table.
     *  @throws UDRException
     */
    public void setChildOrdering(int inputTableNum,
                                 OrderInfo orderInfo) throws UDRException {
         validateCallPhase(CallPhase.COMPILER_INITIAL_CALL, 
                           CallPhase.COMPILER_INITIAL_CALL,
                           "UDRInvocationInfo::setChildOrder()");
         if (inputTableNum < 0 || inputTableNum >= numTableInputs_)
             throw new UDRException(38900, 
                                    "Invalid child table number %d", inputTableNum);
         inputTableInfo_[inputTableNum].setQueryOrdering(new OrderInfo(orderInfo));
    }
    
    /**
     *  Set the usage information for a column of a table-valued input
     *
     *  <p> This method allows the UDR writer to specify whether a given
     *  child column is needed or not.
     *
     *  <p> Only use this method from within the
     *  UDR#describeDataflowAndPredicates(UDRInvocationInfo) method.
     *
     *  @see UDRInvocationInfo#setUnusedPassthruColumns()
     *  @see UDR#describeDataflowAndPredicates(UDRInvocationInfo)
     *
     *  @param inputTableNum  Number of table-valued input to set.
     *  @param inputColumnNum Column number for the column to set.
     *  @param usage          New usage for this column.
     *  @throws UDRException
     */
    // use only from describeDataflowAndPredicates()
    public void setChildColumnUsage(int inputTableNum,
                                    int inputColumnNum,
                                    ColumnInfo.ColumnUseCode usage) throws UDRException {
        in(inputTableNum); // validate inputTableNum
        inputTableInfo_[inputTableNum].getColumn(inputColumnNum).setUsage(usage);
    }
    /**
     *  Mark any passthru columns that are not needed as unused.
     *
     *  <p> For any passthru columns that are marked as NOT_USED or NOT_PRODUCED in
     *  the table-valued result, set the corresponding input columns
     *  to NOT_USED as well. Note that this assumes that the UDF
     *  does not need these columns, either! The usage for the passthru column
     *  itself is also set to NOT_PRODUCED, since the UDF could not produce
     *  the column without having access to the corresponding input column.
     *
     *  <p> Only use this method from within the
     *  UDR#describeDataflowAndPredicates(UDRInvocationInfo) method.
     *
     *  @see UDRInvocationInfo#addPassThruColumns()
     *  @see UDRInvocationInfo#setChildColumnUsage(int,int,ColumnInfo.ColumnUseCode)
     *  @see UDR#describeDataflowAndPredicates(UDRInvocationInfo)
     *
     *  @throws UDRException
     */
    public void setUnusedPassthruColumns() throws UDRException {
        int numOutCols = out().getNumColumns();
        
        // loop over output columns
        for (int oc=0; oc<numOutCols; oc++)
        {
            ColumnInfo colInfo = out().getColumn(oc);
            ColumnInfo.ColumnUseCode usage = colInfo.getUsage();
            ProvenanceInfo prov = colInfo.getProvenance();
            int it = prov.getInputTableNum();
            int ic = prov.getInputColumnNum();

            // is this a pass-thru column that is not used?
            if (it >= 0 && ic >= 0 &&
                (usage == ColumnInfo.ColumnUseCode.NOT_USED ||
                 usage == ColumnInfo.ColumnUseCode.NOT_PRODUCED))
            {
                setChildColumnUsage(it, ic, ColumnInfo.ColumnUseCode.NOT_USED);
                // also make sure the output column is not produced, since
                // we could not get its value from the table-valued input
                colInfo.setUsage(ColumnInfo.ColumnUseCode.NOT_PRODUCED);
            }
        }
    }
    /**
     *  Decide where to evaluate a predicate.
     *
     *  <p> Only use this method from within the
     *  UDR#describeDataflowAndPredicates() method.
     *
     *  @see UDRInvocationInfo#getPredicate(int)
     *  @see UDR#describeDataflowAndPredicates(UDRInvocationInfo)
     *
     *  @param predicateNum Number/index of predicate returned by getPredicate()
     *                      method.
     *  @param c            Evaluation code for this predicate.
     *  @throws UDRException
     */
    public void setPredicateEvaluationCode(int predicateNum,
                                           PredicateInfo.EvaluationCode c) 
    throws UDRException {
        validateCallPhase(CallPhase.COMPILER_DATAFLOW_CALL, 
                          CallPhase.COMPILER_DATAFLOW_CALL,
                          "UDRInvocationInfo.setPredicateEvaluationCode()");
        
        // validate index
        PredicateInfo pred = getPredicate(predicateNum);

        if (c == PredicateInfo.EvaluationCode.EVALUATE_IN_UDF &&
            pred.isAComparisonPredicate() &&
            !(((ComparisonPredicateInfo)pred).hasAConstantValue()))
            throw new UDRException(
                                   38900,
                                   "Comparison predicate %d cannot be evaluated in the UDF since it does not refer to a constant value",
                                   predicateNum);
        predicates_.elementAt(predicateNum).setEvaluationCode(c);
    }
    /**
     *  Push predicates on pass-thru columns to the table-valued input.
     *
     *  <p> Push one or more predicates to their corresponding table-valued input,
     *  if they reference only columns from that input, otherwise leave the
     *  predicate(s) unchanged.
     *
     *  <p> Only use this method from within the
     *  UDR#describeDataflowAndPredicates(UDRInvocationInfo) method.
     *
     *  @see PredicateInfo#setEvaluationCode(PredicateInfo.EvaluationCode)
     *  @see UDR#describeDataflowAndPredicates(UDRInvocationInfo)
     *
     *  @param startPredNum Number/index of first predicate to be pushed.
     *  @param lastPredNum  Number/index of last predicate to be pushed (inclusive)
     *                      or -1 to push all remaining predicates.
     *  @throws UDRException
     */
    public void pushPredicatesOnPassthruColumns(int startPredNum,
                                                int lastPredNum) throws UDRException {
        validateCallPhase(CallPhase.COMPILER_DATAFLOW_CALL, 
                          CallPhase.COMPILER_DATAFLOW_CALL,
                          "UDRInvocationInfo::pushPredicatesOnPassthruColumns()");

        int numPreds = getNumPredicates();
        
        // loop over predicates in the specified range
        for (int p = startPredNum;
             p<numPreds && (p<=lastPredNum || lastPredNum == -1);
             p++)
            if (isAComparisonPredicate(p))
            {
                ComparisonPredicateInfo cpi = getComparisonPredicate(p);
                
                if (out().getColumn(cpi.getColumnNumber()).
                    getProvenance().isFromInputTable())
                    // Yes, this predicate is a comparison predicate on a pass-thru
                    // column (note we do not allow predicates of the form
                    // "col1 op col2"). Push it down.
                    setPredicateEvaluationCode(p,PredicateInfo.EvaluationCode.EVALUATE_IN_CHILD);
            }   
    }

    /**
     *  Push all predicates on pass-thru columns to the table-valued input.
     *
     *  <p> Push all predicates to their corresponding table-valued input,
     *  if they reference only columns from that input, otherwise leave the
     *  predicate(s) unchanged.
     *
     *  <p> Only use this method from within the
     *  UDR#describeDataflowAndPredicates(UDRInvocationInfo) method.
     *
     *  @see PredicateInfo#setEvaluationCode(PredicateInfo.EvaluationCode)
     *  @see UDR#describeDataflowAndPredicates(UDRInvocationInfo)
     *
     *  @throws UDRException
     */
    public void pushPredicatesOnPassthruColumns( ) throws UDRException {
        pushPredicatesOnPassthruColumns(0,-1);
    }
    /**
     *  Propagate constraints for UDFs that return one result row for
     *  every input row.
     *
     *  <p> Use this method only if the UDF returns no more than one result row for
     *  every input row it reads from its single table-valued input. Note that
     *  it is ok for the UDF to return no result rows for some input rows.
     *  Wrong results may be returned by SQL statements involving this UDF if
     *  the UDF does at runtime not conform to the 1x1 relationship of rows.
     *
     *  <p> Only use this method from within the 
     *  UDR#describeConstraints(UDRInvocationInfo) method.
     *
     *  @param exactlyOneRowPerInput Indicates whether the UDF returns exactly
     *                               one output row (true) or at most one output
     *                               row (false) for every input row.
     *  @throws UDRException
     */
    public void propagateConstraintsFor1To1UDFs(boolean exactlyOneRowPerInput)
    throws UDRException {
        validateCallPhase(CallPhase.COMPILER_CONSTRAINTS_CALL, 
                          CallPhase.COMPILER_CONSTRAINTS_CALL,
                          "UDRInvocationInfo::propagateConstraintsFor1To1UDFs()");
        
        if (getNumTableInputs() == 1)
        {
            int numConstraints = in().getNumConstraints();
            int numOutputCols = out().getNumColumns();
            
            for (int c=0; c<numConstraints; c++)
                switch (in().getConstraint(c).getType())
                {
                case CARDINALITY:
                {
                    CardinalityConstraintInfo cc = 
                        (CardinalityConstraintInfo) (in().getConstraint(c));
                    
                    // add a cardinality constraint to the parent with
                    // an adjusted lower bound of 0 if exactlyOneRowPerInput
                    // is false
                    out().
                        addCardinalityConstraint(
                                                 new CardinalityConstraintInfo(
                                                                               (exactlyOneRowPerInput ?
                                                                                cc.getMinNumRows() :
                                                                                0),
                                                                               cc.getMaxNumRows()));
                }
                break;

                case UNIQUE:
                {
                    UniqueConstraintInfo ucParent = new UniqueConstraintInfo();
                    UniqueConstraintInfo ucChild = 
                        (UniqueConstraintInfo)(in().getConstraint(c));
                    int numUniqueCols = ucChild.getNumUniqueColumns();
                    
                    // translate child columns into parent columns
                    for (int uc=0; uc<numUniqueCols; uc++)
                        for (int oc=0; oc<numOutputCols; oc++)
                            if (out().getColumn(oc).getProvenance().getInputColumnNum()
                                == ucChild.getUniqueColumn(uc))
                                ucParent.addColumn(oc);
                    
                    if (ucParent.getNumUniqueColumns() == numUniqueCols)
                        // we were able to translate all the unique columns on the
                        // child into unique columns of the parent, add the constraint
                        out().addUniquenessConstraint(ucParent);
                }
                break;
                
                default:
                    // should not see this
                    break;
                }
        }
    }
    /**
     *  Get data to persist between calls of the compile-time interface
     *
     *  <p> The UDR writer must use a static or dynamic cast to get a pointer
     *  to the derived class.
     *
     *  <p> Only use this method at compile time.
     *
     *  @see UDRInvocationInfo#setUDRWriterCompileTimeData(UDRWriterCompileTimeData)
     *
     *  @return UDR writer-specific data that was previously attached or NULL.
     *  @throws UDRException
     */
    // use anytime during compilation
    public UDRWriterCompileTimeData getUDRWriterCompileTimeData() throws UDRException {
        validateCallPhase(CallPhase.COMPILER_INITIAL_CALL, 
                          CallPhase.COMPILER_COMPLETION_CALL,
                          "UDRInvocationInfo::getUDRWriterCompileTimeData()");
        
        return udrWriterCompileTimeData_;
    }
    /**
     *  Set data to persist between calls of the compile-time interface
     *
     *  <p> This call can be used to attach an object derived from class
     *  UDRWriterCompileTimeData to the UDRInvocationInfo object. Once
     *  attached, the data will be carried between the stages of the
     *  compiler interface and can be used to keep state. Note that
     *  this data will be deleted at the end of the compiler phase and
     *  will not persist until runtime.
     *
     *  <p> Only use this method at compile time.
     *
     *  <p> To keep state for specific plan alternatives, use the
     *  UDRPlanInfo#setUDRWriterCompileTimeData(UDRWriterCompileTimeData) method.
     *
     *  @see UDRInvocationInfo#getUDRWriterCompileTimeData()
     *  @see UDRPlanInfo#setUDRWriterCompileTimeData(UDRWriterCompileTimeData)
     *
     *  @param compileTimeData UDR writer-defined compile-time data to attach.
     *  @throws UDRException
     */
    public void setUDRWriterCompileTimeData(UDRWriterCompileTimeData compileTimeData) 
    throws UDRException {
        validateCallPhase(CallPhase.COMPILER_INITIAL_CALL, 
                          CallPhase.COMPILER_PLAN_CALL,
                          "UDRInvocationInfo::setUDRWriterCompileTimeData()");
        
        udrWriterCompileTimeData_ = compileTimeData;
    }

    /**
     *  Copy values of pass-thru columns from the input to the output table.
     *
     *  <p> This method is an easy way to set the values of the table-valued result
     *  row from their corresponding values in the table-valued inputs.
     *  Note that the UDR must set all the values of the pass-thru columns to
     *  the corresponsing values of the input tables. If it fails to do that,
     *  some optimizations done by Trafodion could lead to wrong results
     *  (e.g. some predicates could be applied incorrectly). Every TMUDF with
     *  table-valued inputs and pass-thru columns should call this method for
     *  every row it emits.
     *
     *  <p> This method can only be called from within 
     *  UDR#processData(UDRInvocationInfo, UDRPlanInfo).
     *
     *  @see UDRInvocationInfo#addPassThruColumns(int,int,int)
     *  @see UDR#processData(UDRInvocationInfo, UDRPlanInfo)
     *
     *  @param inputTableNum    Number of table-valued input to copy from.
     *  @param startInputColNum First column number in the input table to copy
     *  @param endInputColNum   Last column number in the input table to copy
     *                          (inclusive) or -1 to copy all remaining columns
     *  @throws UDRException
     */
    // Functions available at run-time only
    public void copyPassThruData(int inputTableNum,
                                 int startInputColNum,
                                 int endInputColNum) throws UDRException {
        // no need to validate call phase, this will raise an exception at compile time
        // validateCallPhase(CallPhase.RUNTIME_INITIAL_CALL, 
        //                   CallPhase.RUNTIME_FINAL_CALL,
        //                   "UDRInvocationInfo::copyPassThruData()");
        
        int endColNum = endInputColNum;
        int numOutCols = out().getNumColumns();

        if (endInputColNum < 0 ||
            endInputColNum >= in(inputTableNum).getNumColumns())
            endColNum = in(inputTableNum).getNumColumns() - 1;
        
        // loop through the output columns and pick up those that
        // are passed through from the specified input columns
        for (int oc=0; oc<numOutCols; oc++)
        {
            ProvenanceInfo prov = out().getColumn(oc).getProvenance();
            int it = prov.getInputTableNum();
            int ic = prov.getInputColumnNum();
            
            if (it == inputTableNum &&
                ic >= startInputColNum &&
                ic <= endColNum)
            {
                // this output column is passed through from the range
                // of input columns selected, copy it
                TypeInfo ty = out().getColumn(oc).getType();
                
                switch (ty.getSQLTypeSubClass())
                {
                case FIXED_CHAR_TYPE:
                case VAR_CHAR_TYPE:
                case DATE_TYPE:
                case TIME_TYPE:
                case TIMESTAMP_TYPE:
                case LOB_SUB_CLASS:
                {
                    // In the C++ version we use getRaw(), because in C++,
                    // the UDF sees characters in the encoding used by SQL.
                    // In Java the UDF sees only Unicode characters, so we
                    // use getString() here, to do the conversion.
                    String str = in(it).getString(ic);
                    
                    if (in(it).wasNull())
                        out().setNull(oc);
                    else
                        out().setString(oc, str);
                }
                break;

                case EXACT_NUMERIC_TYPE:
                case YEAR_MONTH_INTERVAL_TYPE:
                case DAY_SECOND_INTERVAL_TYPE:
                case BOOLEAN_SUB_CLASS:
                {
                    long l = in(it).getLong(ic);
                    
                    if (in(it).wasNull())
                        out().setNull(oc);
                    else
                        out().setLong(oc, l);
                }
                break;
                
                case APPROXIMATE_NUMERIC_TYPE:
                {
                    double d = in(it).getDouble(ic);
                    
                    if (in(it).wasNull())
                        out().setNull(oc);
                    else
                        out().setDouble(oc, d);
                }
                break;

                case UNDEFINED_TYPE_SUB_CLASS:
                default:
                    throw new UDRException(
                                           38900,
                                           "Invalid or unsupported type subclass in UDRInvocationInfo::copyPassThruData: %d",
                                           ty.getSQLTypeSubClass().ordinal());
                }
            }
        }
    }

    /**
     *  Copy values of all pass-thru columns from the first input to the output table.
     *
     *  <p> This method is an easy way to set the values of the table-valued result
     *  row from their corresponding values in the table-valued inputs.
     *  Note that the UDR must set all the values of the pass-thru columns to
     *  the corresponsing values of the input tables. If it fails to do that,
     *  some optimizations done by Trafodion could lead to wrong results
     *  (e.g. some predicates could be applied incorrectly). Every TMUDF with
     *  table-valued inputs and pass-thru columns should call this method for
     *  every row it emits.
     *
     *  <p> This method can only be called from within 
     *  UDR#processData(UDRInvocationInfo, UDRPlanInfo).
     *
     *  @see UDRInvocationInfo#addPassThruColumns(int,int,int)
     *  @see UDR#processData(UDRInvocationInfo, UDRPlanInfo)
     *
     *  @throws UDRException
     */
    public void copyPassThruData() throws UDRException {
        copyPassThruData(0,0,-1);
    }
    /**
     *  Get the number of parallel instances working on this UDR invocation.
     *
     *  <p> Use this method to find out how many parallel instances are
     *  executing this UDR.
     *
     *  <p> This method can only be called from within
     *  UDR#processData(UDRInvocationInfo, UDRPlanInfo).
     *
     *  @see UDRInvocationInfo#getMyInstanceNum()
     *  @return Number of parallel instances for this UDR invocation.
     *  @throws UDRException
     */
    public int getNumParallelInstances() throws UDRException {
        validateCallPhase(CallPhase.RUNTIME_WORK_CALL, 
                          CallPhase.RUNTIME_WORK_CALL,
                          "UDRInvocationInfo.getNumParallelInstances()");

        return totalNumInstances_;
    }
    /**
 * Get the instance number of this runtime process.
 *
 *  <p> Use this method to find out which of the parallel instances
 *  executing a UDR this process is.
 *
 *  <p> This method can only be called from within
 *  UDR#processData(UDRInvocationInfo, UDRPlanInfo).
 *
 *  @see UDRInvocationInfo#getNumParallelInstances()
 *  @return A number between 0 and getNumParallelInstances() - 1.
 *  @throws UDRException
 */
    public int getMyInstanceNum() throws UDRException {  // 0 ... getNumInstances()-1
        validateCallPhase(CallPhase.RUNTIME_WORK_CALL, 
                          CallPhase.RUNTIME_WORK_CALL,
                          "UDRInvocationInfo.getMyInstanceNum()");
        
        return myInstanceNum_;
    }

    /**
     *  Print the object, for use in debugging.
     *  @throws UDRException 
     *
     *  @see UDR#debugLoop()
     *  @see UDRInvocationInfo.DebugFlags#PRINT_INVOCATION_INFO_AT_RUN_TIME
     */
    // Functions for debugging
    public void print() throws UDRException {
        CallPhase savedCallPhase = callPhase_;
        callPhase_ = CallPhase.UNKNOWN_CALL_PHASE; // we may call methods that would otherwise fail the check
        System.out.print(String.format("\nUDRInvocationInfo\n-----------------\n"));
        System.out.print(String.format("UDR Name                   : %s\n", getUDRName()));
        System.out.print(String.format("Num of table-valued inputs : %d\n", getNumTableInputs()));
        System.out.print(String.format("Call phase                 : %s\n", callPhaseToString(savedCallPhase)));
        System.out.print(String.format("Debug flags                : 0x%x\n", getDebugFlags()));
        System.out.print(String.format("Function type              : %s\n", (funcType_ == FuncType.GENERIC ? "GENERIC" :
                                                                      (funcType_ == FuncType.MAPPER ? "MAPPER" :
                                                                       (funcType_ == FuncType.REDUCER ? "REDUCER" :
                                                                        (funcType_ == FuncType.REDUCER_NC ? "REDUCER_NC" :
                                                                         "Invalid function type"))))));
        System.out.print(String.format("User id                    : %s\n", getCurrentUser()));
        System.out.print(String.format("Session user id            : %s\n", getSessionUser()));
        System.out.print(String.format("User role                  : %s\n", getCurrentRole()));
        if (isRunTime())
            System.out.print(String.format("Query id                   : %s\n", getQueryId()));
        
        System.out.print(String.format("Formal parameters          : ("));
        boolean needsComma = false;
        for (int p=0; p<getFormalParameters().getNumColumns(); p++)
        {   
            if (needsComma)
                System.out.print(String.format(", "));
            System.out.print(String.format("%s", getFormalParameters().getColumn(p).toString()));
            needsComma = true;
        }
        System.out.print(String.format(")\n"));
        
        System.out.print(String.format("Actual parameters          : ("));
        needsComma = false;
        ParameterListInfo pli = par();
  
        for (int p=0; p < pli.getNumColumns(); p++)
        {
            
            if (needsComma)
                System.out.print(String.format(", "));

            if (pli.isAvailable(p))
                {
                    String strVal = pli.getString(p);

                    if (pli.wasNull())
                        System.out.print(String.format("NULL"));
                    else
                        System.out.print(String.format("'%s'", strVal));
                }
                else
                {
                    // no value available, print name and type
                    String buf = pli.getColumn(p).toString(true);
                    System.out.print(String.format("\n        "));
                    System.out.print(buf);
                }
            needsComma = true;
        }
        System.out.print(String.format(")\n"));
        
        if (udrWriterCompileTimeData_ != null)
        {
            System.out.print(String.format("UDR Writer comp. time data : "));
            udrWriterCompileTimeData_.print();
            System.out.print(String.format("\n"));
        }
        
        if (isRunTime()) {
            System.out.print(String.format("Instance number            : %d of %d\n",
                                           getMyInstanceNum(),
                                           getNumParallelInstances()));
        }
        
        for (int c=0; c<getNumTableInputs(); c++)
        {
            System.out.print(String.format("\nInput TableInfo %d\n-----------------\n", c));
            in(c).print();
        }
        System.out.print(String.format("\nOutput TableInfo\n----------------\n"));
        outputTableInfo_.print();
        
        if (getNumPredicates() > 0)
        {
            System.out.print(String.format("\nPredicates\n----------\n"));
            
            for (int p=0; p<getNumPredicates(); p++)
            {
                String predString = getPredicate(p).toString(out());
                switch (getPredicate(p).getEvaluationCode())
                {
                case UNKNOWN_EVAL:
                    break;
                case EVALUATE_ON_RESULT:
                    predString += " (evaluated on result)";
                    break;
                case EVALUATE_IN_UDF:
                    predString += " (evaluated by the UDF)";
                    break;
                case EVALUATE_IN_CHILD:
                    predString += " (evaluated in the child)";
                    break;
                default:
                    predString += " -- invalid evaluation code!";
                    break;
                }
                System.out.print(String.format("    %s\n", predString));
            }
        }
        callPhase_ = savedCallPhase;
    }

    // UDR writers can ignore these package-private methods
    static short getCurrentVersion() { return 1; }

    int serializedLength() throws UDRException{
      int result = super.serializedLength() +
              serializedLengthOfString(name_) +
              serializedLengthOfString(currentUser_) +
              serializedLengthOfString(sessionUser_) +
              serializedLengthOfString(currentRole_) +
              serializedLengthOfString(queryId_) +
              9*serializedLengthOfInt();

      int i;

      for (i=0; i<numTableInputs_; i++)
        result += inputTableInfo_[i].serializedLength();

      result += outputTableInfo_.serializedLength();
      result += formalParameterInfo_.serializedLength();
      result += actualParameterInfo_.serializedLength();

      for (i=0; i < predicates_.size(); i++)
      {
        result += predicates_.elementAt(i).serializedLength();
      }

      return result;
    }

    int serialize(ByteBuffer outputBuffer) throws UDRException {
      int i;

      int origPos = outputBuffer.position();

      super.serialize(outputBuffer);

      serializeString(name_,
                      outputBuffer);

      serializeInt(sqlAccessType_.ordinal(),
                   outputBuffer);

      serializeInt(sqlTransactionType_.ordinal(),
                   outputBuffer);

      serializeInt(sqlRights_.ordinal(),
                   outputBuffer);

      serializeInt(isolationType_.ordinal(),
                   outputBuffer);

      serializeInt(debugFlags_,
                   outputBuffer);

      serializeInt(funcType_.ordinal(),
                   outputBuffer);

      serializeInt(callPhase_.ordinal(),
                   outputBuffer);

      serializeString(currentUser_,
                      outputBuffer);

      serializeString(sessionUser_,
                      outputBuffer);

      serializeString(currentRole_,
                      outputBuffer);

      serializeString(queryId_,
                      outputBuffer);

      serializeInt(numTableInputs_,
                   outputBuffer);

      for (i=0; i<numTableInputs_; i++)
        inputTableInfo_[i].serialize(outputBuffer);

      outputTableInfo_.serialize(outputBuffer);

      formalParameterInfo_.serialize(outputBuffer);

      actualParameterInfo_.serialize(outputBuffer);

      serializeInt(predicates_.size(),
                   outputBuffer);

      for (i=0; i < predicates_.size(); i++)
        predicates_.elementAt(i).serialize(outputBuffer);

      int bytesSerialized = outputBuffer.position() - origPos;
      
      validateSerializedLength(bytesSerialized);
      
      return bytesSerialized;
    }

    int deserialize(ByteBuffer inputBuffer) throws UDRException{
      int tempInt = 0;

      int origPos = inputBuffer.position();

      super.deserialize(inputBuffer);

      validateObjectType(TMUDRObjectType.UDR_INVOCATION_INFO_OBJ);

      name_ = deserializeString(inputBuffer);

      tempInt = deserializeInt(inputBuffer);
      sqlAccessType_ = SQLAccessType.fromOrdinal(tempInt);

      tempInt = deserializeInt(inputBuffer);
      sqlTransactionType_ = SQLTransactionType.fromOrdinal(tempInt);

      tempInt = deserializeInt(inputBuffer);
      sqlRights_ = SQLRightsType.fromOrdinal(tempInt);

      tempInt = deserializeInt(inputBuffer);
      isolationType_ = IsolationType.fromOrdinal(tempInt);

      debugFlags_ = deserializeInt(inputBuffer);

      tempInt = deserializeInt(inputBuffer);
      funcType_ = FuncType.fromOrdinal(tempInt);

      tempInt = deserializeInt(inputBuffer);
      callPhase_ = CallPhase.getEnum(tempInt);

      currentUser_ = deserializeString(inputBuffer);

      sessionUser_ = deserializeString(inputBuffer);

      currentRole_ = deserializeString(inputBuffer);

      queryId_ = deserializeString(inputBuffer);

      numTableInputs_ = deserializeInt(inputBuffer);

      for (int i=0; i<numTableInputs_; i++)
      {
        if (inputTableInfo_[i] == null)
          inputTableInfo_[i] = new TableInfo();
        inputTableInfo_[i].deserialize(inputBuffer);
      }

      if (outputTableInfo_ == null)
        outputTableInfo_ = new TableInfo();
      outputTableInfo_.deserialize(inputBuffer);

      if (formalParameterInfo_ == null)
        formalParameterInfo_ = new ParameterListInfo();
      formalParameterInfo_.deserialize(inputBuffer);

      if (actualParameterInfo_ == null)
        actualParameterInfo_ = new ParameterListInfo();
      actualParameterInfo_.deserialize(inputBuffer);

      tempInt = deserializeInt(inputBuffer);
      predicates_.clear();

      for (int p=0; p<tempInt; p++)
      {
        switch (getNextObjectType(inputBuffer))
        {
        case COMP_PREDICATE_INFO_OBJ:
          {
            ComparisonPredicateInfo pred = new ComparisonPredicateInfo();

            pred.deserialize(inputBuffer);

            predicates_.add(pred);
          }
          break;

        default:
          throw new UDRException(
               38900,
               "Found invalid predicate object of type %d",
               getNextObjectType(inputBuffer).getValue());
        }
      }
     int bytesDeserialized = inputBuffer.position() - origPos;
     validateDeserializedLength(bytesDeserialized);

     return bytesDeserialized;
    }
    

    UDRInvocationInfo() {
        super(TMUDRObjectType.UDR_INVOCATION_INFO_OBJ,
              getCurrentVersion()) ;
        numTableInputs_ = 0;
        callPhase_ = CallPhase.UNKNOWN_CALL_PHASE;
        funcType_ = FuncType.GENERIC;
        debugFlags_ = 0;
        //udrWriterCompileTimeData_(NULL),
        totalNumInstances_ = 0;
        myInstanceNum_ = 0;
        inputTableInfo_ = new TableInfo[MAX_INPUT_TABLES];
        predicates_ = new Vector<PredicateInfo>();
    }

    void validateCallPhase(CallPhase start,
                           CallPhase end,
                           String callee) throws UDRException {
        if (callPhase_.ordinal() < start.ordinal() && 
            callPhase_ != CallPhase.UNKNOWN_CALL_PHASE)
            throw new UDRException(
                                   38900,
                                   "Method %s cannot be called before the %s phase",
                                   callee,
                                   callPhaseToString(start));
        if (callPhase_.ordinal() > end.ordinal())
            throw new UDRException(
                                   38900,
                                   "Method %s cannot be called after the %s phase",
                                   callee,
                                   callPhaseToString(end));
    }
    private String callPhaseToString(CallPhase c) {
        switch(c)
        {
        case UNKNOWN_CALL_PHASE:
            return "unknown";
        case COMPILER_INITIAL_CALL:
            return "describeParamsAndColumns()";
        case COMPILER_DATAFLOW_CALL:
            return "describeDataflowAndPredicates()";
        case COMPILER_CONSTRAINTS_CALL:
            return "describeConstraints()";
        case COMPILER_STATISTICS_CALL:
            return "describeStatistics()";
        case COMPILER_DOP_CALL:
            return "describeDesiredDegreeOfParallelism()";
        case COMPILER_PLAN_CALL:
            return "describePlanProperties()";
        case COMPILER_COMPLETION_CALL:
            return "completeDescription()";
        case RUNTIME_WORK_CALL:
            return "runtime work call";
        default:
            return "invalid call phase!";
        }
    }

    void setRuntimeInfo(String qid,
                        int totalNumInstances,
                        int myInstanceNum)
    {
        // set information that is not yet known at compile time
        queryId_ = qid;
        totalNumInstances_ = totalNumInstances;
        myInstanceNum_ = myInstanceNum;
    }

    private static final int MAX_INPUT_TABLES = 2;

    private String name_;
    private SQLAccessType sqlAccessType_;
    private SQLTransactionType sqlTransactionType_;
    private SQLRightsType sqlRights_;
    private IsolationType isolationType_;
    private int numTableInputs_;
    private CallPhase callPhase_;
    private String currentUser_;
    private String sessionUser_;
    private String currentRole_;
    private String queryId_;
    private TableInfo inputTableInfo_[];
    private TableInfo outputTableInfo_;
    private int debugFlags_;
    private FuncType funcType_;
    private ParameterListInfo formalParameterInfo_;
    private ParameterListInfo actualParameterInfo_;
    private Vector<PredicateInfo> predicates_;
    private UDRWriterCompileTimeData udrWriterCompileTimeData_;
    private int totalNumInstances_;
    private int myInstanceNum_;

  };
