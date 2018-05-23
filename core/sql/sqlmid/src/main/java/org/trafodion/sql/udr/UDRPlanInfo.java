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
import java.nio.ByteBuffer;


/** Describes the query plan used for a UDR invocation
 *
 *  <p>Objects of this type are used together with UDRInvocationInfo
 *  objects and in the future they may contain additional info on
 *  plan-related such as the chosen partitioning and ordering.
 */
public class UDRPlanInfo extends TMUDRSerializableObject {

    /** Special degrees of parallelism.
     *
     *  <p> Values that can be used in the setDesiredDegreeOfParallelism()
     *  method, in addition to positive numbers for the degree of
     *  parallelism (DoP).
     *
     *  @see UDR#describeStatistics
     */
    public enum SpecialDegreeOfParallelism
    {
        /** Optimizer decides DoP */
        ANY_DEGREE_OF_PARALLELISM     (0), 
        /** Optimizer decides DoP based on dataflow heuristics. */
        DEFAULT_DEGREE_OF_PARALLELISM (-1),
        /**  Execute the UDF with the largest degree of parallelism allowed. */
        MAX_DEGREE_OF_PARALLELISM     (-2), 
        /**  Execute one instance of the on every Trafodion node.
             Used internally for maintenance UDFs. */
        ONE_INSTANCE_PER_NODE         (-3);  
        
        private final int parType_;
        
        SpecialDegreeOfParallelism(int val) {
            parType_ = val;
        }
          
        public int getSpecialDegreeOfParallelism() {
            return parType_;
        }
    };

   // Functions for use by UDR writer, both at compile and at run time
    /**
     *  Get a unique id for a given plan within a UDR invocation.
     *
     *  @return Plan number for this object, relative to the invocation.
     */
    int getPlanNum() {
        return planNum_;
    }

    /**
     *  Retreive cost per row
     *  @return costPerRow
     */
    public long getCostPerRow() {
        return costPerRow_;
    }
    /**
     *  Return the desired degree of parallelism for this plan.
     *
     *  @see UDRPlanInfo#setDesiredDegreeOfParallelism(int)
     *  @return Degree of parallelism to be used for this plan alternative
     *          (positive) or one of the enum values in
     *          UDRPlanInfo#SpecialDegreeOfParallelism (zero or negative).
     */
    public int getDesiredDegreeOfParallelism() {
        return degreeOfParallelism_;
    }

    // Functions available at compile time only
    // call this from describePlanProperties() or earlier
    /**
     *  Set the desired degree of parallelism.
     *
     *  <p> Here are some special values that can be set, in
     *  addition to positive numbers. These are defined in
     *  class UDRPlanInfo.
     *<ul>
     *  <li> ANY_DEGREE_OF_PARALLELISM:
     *        This will allow the optimizer to choose any degree
     *        of parallelism, including 1 (serial execution)
     *  <li> DEFAULT_DEGREE_OF_PARALLELISM:
     *        Currently the same as ANY_DEGREE_OF_PARALLELISM.
     *        The optimizer will use a heuristic based on
     *        the estimated cardinality (which you can set in
     *        the {@link UDR#describeStatistics} interface).
     *  <li> MAX_DEGREE_OF_PARALLELISM:
     *        Choose the highest possible degree of parallelism.
     *  <li> ONE_INSTANCE_PER_NODE:
     *        Start one parallel instance on every Trafodion node.
     *        This is mostly meant for internal TMUDFs, e.g. a
     *        TMUDF to read the log files on every node.
     * </ul>
     *  @see UDRPlanInfo#getDesiredDegreeOfParallelism()
     *  @see UDR#describeStatistics
     *  @see TableInfo#setEstimatedNumRows
     *  @param dop desired degree of parallelism (a positive number or
     *             one of the enum values in
     *             UDRPlanInfo#SpecialDegreeOfParallelism).
     *  @throws UDRException
     */
    public void setDesiredDegreeOfParallelism(int dop) throws UDRException
    {
        invocationInfo_.validateCallPhase(UDRInvocationInfo.CallPhase.COMPILER_DOP_CALL,
                                          UDRInvocationInfo.CallPhase.COMPILER_DOP_CALL,
                                          "UDRPlanInfo.setDesiredDegreeOfParallelism()");

        degreeOfParallelism_ = dop;
    }
    /**
     *  Estimate of average time taken to produce an output row, in microseconds
     *
     *  @throws UDRException
     */
    public void setCostPerRow(long microseconds) throws UDRException {
        invocationInfo_.validateCallPhase(UDRInvocationInfo.CallPhase.COMPILER_DOP_CALL,
                                          UDRInvocationInfo.CallPhase.COMPILER_PLAN_CALL,
                                          "UDRPlanInfo.setCostPerRow()");

        costPerRow_ = microseconds;
    }
    /**
     *  Get data to persist between calls of the optimizer interface
     *
     *  @see UDRPlanInfo#setUDRWriterCompileTimeData(UDRWriterCompileTimeData)
     *  @return UDR writer-specific data that was previously attached or NULL.
     *  @throws UDRException
     */
    public UDRWriterCompileTimeData getUDRWriterCompileTimeData() throws UDRException {
        invocationInfo_.validateCallPhase(UDRInvocationInfo.CallPhase.COMPILER_DATAFLOW_CALL,
                                          UDRInvocationInfo.CallPhase.COMPILER_COMPLETION_CALL,
                                          "UDRPlanInfo.getUDRWriterCompileTimeData()");
        
        return udrWriterCompileTimeData_;
    }
    /**
     *  Set data to persist between calls of the optimizer interface
     *
     *  <p> This call can be used to attach an object derived from class
     *  UDRWriterCompileTimeData to the UDRPlanInfo object. Once
     *  attached, the data will be carried between the stages of the
     *  optimizer interface and can be used to keep state. Note that
     *  this data will be deleted at the end of the optimizer phase and
     *  will not persist until runtime.
     *
     *  <p> Use this method to keep data that is specific to a query plan
     *  alternative, represented by the UDRPlanInfo object. Use
     *  UDRInvocationInfo::setUDRWriterCompileTimeData() to keep data
     *  that is common for the entire UDR invocation.
     *
     *  @see UDRInvocationInfo#setUDRWriterCompileTimeData(UDRWriterCompileTimeData)
     *  @see UDRPlanInfo#getUDRWriterCompileTimeData()
     *  @param compileTimeData UDR writer-defined compile-time data to attach.
     *  @throws UDRException
     */
    public void setUDRWriterCompileTimeData(UDRWriterCompileTimeData compileTimeData) 
        throws UDRException {
         invocationInfo_.validateCallPhase(UDRInvocationInfo.CallPhase.COMPILER_DATAFLOW_CALL,
                                           UDRInvocationInfo.CallPhase.COMPILER_COMPLETION_CALL,
                                           "UDRPlanInfo.setUDRWriterCompileTimeData()");

         // for now we can't allow this for C++. 
         // We need to look into how it could work in Java
         throw new UDRException(
                                38912,
                                "UDRPlanInfo.setUDRWriterCompileTimeData() not yet supported");
         

         // udrWriterCompileTimeData_ = compileTimeData;

    }

    // call this from completeDescription() or earlier
    /**
     *  Attach a byte array to the plan to be sent to the runtime instances.
     *
     *  <p> Compile time and runtime interfaces of the UDR can be called from
     *  different processes, since UDRs can be executed in parallel and on
     *  different nodes. If the UDR writer would like to carry state from
     *  the compiler interface calls to runtime calls, the best way to achieve
     *  this to attach it using this call and to retrieve the state at runtime
     *  using the UDRPlanInfo#getPlanData() call.
     *
     *  <p> The best place to use this method is from within
     *  UDRPlanInfo#completeDescription() method, since this method is
     *  called on the optimal plan that will be used at runtime. It can
     *  also be called from other methods, and the plan data will be
     *  discarded if the plan is not chosen.
     *
     *  @see UDRPlanInfo#getPlanData()
     *
     *  @param planData A byte array, content defined by the UDR writer, to be
     *         sent to all runtime instances executing the UDR. The buffer
     *         can and should be deleted by the caller after calling this method.
     *  @throws UDRException
     */
    public void addPlanData(byte[] planData) throws UDRException {
        invocationInfo_.validateCallPhase(UDRInvocationInfo.CallPhase.COMPILER_DOP_CALL,
                                          UDRInvocationInfo.CallPhase.COMPILER_COMPLETION_CALL,
                                          "UDRPlanInfo::addPlanData()");
        
        if (planData.length > 0)
        {
            // make a new copy of the input data
            planData_ = planData.clone();
        }
    }
    /**
     *  Retrieve plan data attached to the UDR invocation and plan.
     *
     *  <p> This method can be called at runtime to get state generated at compile time.
     *
     *  @see UDRPlanInfo#addPlanData(byte[])
     *
     *  @return Reference to a byte array with plan data generated by the UDR writer
     *          at compile time.
     */
    public byte[] getPlanData( ) {
        return planData_ ;
    }
    /**
     *  Print the object, for use in debugging.
     *
     *  @see UDRInvocationInfo.DebugFlags#PRINT_INVOCATION_INFO_AT_RUN_TIME
     */
    public void print() throws UDRException
    {
        System.out.println("\nUDRPlanInfo\n-----------------------");
        System.out.println(String.format("Plan number                : %d", planNum_));
        System.out.println(String.format("Cost per row               : %d", costPerRow_));
        System.out.println(String.format("Degree of parallelism      : %d", degreeOfParallelism_));

        if (udrWriterCompileTimeData_ != null)
        {
            System.out.print("UDR Writer comp. time data : ");
            getUDRWriterCompileTimeData().print();
            System.out.print("\n");
        }

        System.out.print("UDF Writer plan data length: ");
        System.out.print(String.format("%d\n", (planData_ != null) ? planData_.length : 0));
    }
    
    // UDR writers can ignore these methods
    static short getCurrentVersion() { return 1; }

    @Override
    int serializedLength() throws UDRException {
      int result = super.serializedLength() +
                   serializedLengthOfLong() +
                   serializedLengthOfInt() +
                   serializedLengthOfBinary(planData_.length);

      return result;
    }

    @Override
    int serialize(ByteBuffer outputBuffer) throws UDRException {
      
      int origPos = outputBuffer.position();

      super.serialize(outputBuffer);

      serializeLong(costPerRow_,
                    outputBuffer);

      serializeInt(degreeOfParallelism_,
                   outputBuffer);

      serializeBinary(planData_,
                      outputBuffer);

      int bytesSerialized = outputBuffer.position() - origPos;

      validateSerializedLength(bytesSerialized);

      return bytesSerialized;
    }

    @Override
    int deserialize(ByteBuffer inputBuffer) throws UDRException {
      int origPos = inputBuffer.position();

      super.deserialize(inputBuffer);

      validateObjectType(TMUDRObjectType.UDR_PLAN_INFO_OBJ);

      costPerRow_ = deserializeLong(inputBuffer);

      degreeOfParallelism_ = deserializeInt(inputBuffer);

      planData_ = deserializeBinary(inputBuffer);

      int bytesDeserialized = inputBuffer.position() - origPos;
      validateSerializedLength(bytesDeserialized);

      return bytesDeserialized;
    }
    

    UDRPlanInfo(UDRInvocationInfo invocationInfo, int planNum) {
        super(TMUDRObjectType.UDR_PLAN_INFO_OBJ,getCurrentVersion());
        invocationInfo_ = invocationInfo;
        planNum_ = planNum;
        costPerRow_ = -1;
        degreeOfParallelism_ = SpecialDegreeOfParallelism.ANY_DEGREE_OF_PARALLELISM.getSpecialDegreeOfParallelism();
    }

    private UDRInvocationInfo invocationInfo_;
    private int planNum_;
    private long costPerRow_;
    private int degreeOfParallelism_;
    private UDRWriterCompileTimeData udrWriterCompileTimeData_;
    private byte planData_[];
}
