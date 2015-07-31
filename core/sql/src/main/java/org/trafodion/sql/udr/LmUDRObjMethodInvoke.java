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
/* -*-Java-*-
 ******************************************************************************
 *
 * File:         LmUDRObjMethodInvoke.java
 * Description:  Language Manager's Java-side JNI code to invoke methods
 *               on objects that are derived from class tmudr::UDR.
 *
 * Created:      4/30/2015
 * Language:     Java
 *
 *
 ******************************************************************************
 */
package org.trafodion.sql.udr;

import java.util.Vector;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class LmUDRObjMethodInvoke
{
    UDR udr_;
    UDRInvocationInfo invocationInfo_;
    Vector<UDRPlanInfo> planInfos_;

    // factory method, make a new object of this class
    static LmUDRObjMethodInvoke makeNewObj(
            UDR udrObj,
            byte[] serializedInvocationInfo,
            byte[] serializedPlanInfo) throws UDRException
    {
        LmUDRObjMethodInvoke result = new LmUDRObjMethodInvoke();

        result.udr_ = udrObj;
        result.planInfos_ = new Vector<UDRPlanInfo>();

        int planNum = 0; // default plan number at runtime

        if (serializedPlanInfo == null || serializedPlanInfo.length <= 0)
            planNum = -1;  // no plan info provided (compile time)

        planNum = result.unpackObjects(serializedInvocationInfo,
                                       serializedPlanInfo,
                                       planNum);

        return result;
    }

    public static class ReturnInfo
    {
        // return status:
        // <0: Internal error, check for Java exception
        // 0:  Success
        // >0: User-generated error, returnedSQLState_ and
        //     returnedErrorMessage_ have details
        int returnStatus_;
        String returnedSQLState_;
        String returnedMessage_;
        byte [] returnedInvocationInfo_;
        byte [] returnedPlanInfo_;

        ReturnInfo(int s)
        {
            returnStatus_ = s;
        }
    }

    public ReturnInfo invokeRoutineMethod(
         int callPhase,
         byte [] serializedInvocationInfo,
         byte [] serializedPlanInfo,
         int planNum,
         byte [] inputRow) throws UDRException
    {
        ReturnInfo result = new ReturnInfo(0);

        try {

            planNum = unpackObjects(serializedInvocationInfo,
                                    serializedPlanInfo,
                                    planNum);
            invocationInfo_.par().setRow(inputRow);
            invocationInfo_.setCallPhase(callPhase);

            switch (invocationInfo_.getCallPhase()) {
            case COMPILER_INITIAL_CALL:
                if ((invocationInfo_.getDebugFlags() &
                     UDRInvocationInfo.DebugFlags.PRINT_INVOCATION_INFO_INITIAL.flagVal()) != 0)
                    invocationInfo_.print();
                udr_.describeParamsAndColumns(invocationInfo_);
                break;
            case COMPILER_DATAFLOW_CALL:
                udr_.describeDataflowAndPredicates(invocationInfo_);
                break;
            case COMPILER_CONSTRAINTS_CALL:
                udr_.describeConstraints(invocationInfo_);
                break;
            case COMPILER_STATISTICS_CALL:
                udr_.describeStatistics(invocationInfo_);
                break;
            case COMPILER_DOP_CALL:
                udr_.describeDesiredDegreeOfParallelism(invocationInfo_,
                                                        planInfos_.get(planNum));
                break;
            case COMPILER_PLAN_CALL:
                udr_.describePlanProperties(invocationInfo_,
                                            planInfos_.get(planNum));
                break;
            case COMPILER_COMPLETION_CALL:
                udr_.completeDescription(invocationInfo_,
                                         planInfos_.get(planNum));
                if ((invocationInfo_.getDebugFlags() &
                     UDRInvocationInfo.DebugFlags.PRINT_INVOCATION_INFO_END_COMPILE.flagVal()) != 0)
                    {
                        invocationInfo_.print();
                        System.out.println("\n");
                        for (int i=0; i<planInfos_.size(); i++)
                            if (planInfos_.get(i) != null)
                            {
                              if (i == planNum)
                                System.out.println("++++++++++ Chosen plan: ++++++++++");
                              else
                                System.out.println("-------- Plan not chosen: --------");
                              planInfos_.get(i).print();
                            }
                    }
                break;
            case RUNTIME_WORK_CALL:
                if (planNum < 0 && planInfos_.size() > 0)
                    planNum = 0; // info was provided in makeNewObj()
                // allocate child input and output rows
                for (int c=0; c<invocationInfo_.getNumTableInputs(); c++)
                    invocationInfo_.in(c).setRow(
                        new byte[invocationInfo_.in(c).getRecordLength()]);
                invocationInfo_.out().setRow(
                    new byte[invocationInfo_.out().getRecordLength()]);          
                if ((invocationInfo_.getDebugFlags() &
                     UDRInvocationInfo.DebugFlags.PRINT_INVOCATION_INFO_AT_RUN_TIME.flagVal()) != 0)
                    {
                        invocationInfo_.print();
                        System.out.println("\n");
                        planInfos_.get(planNum).print();
                    }
                udr_.processData(invocationInfo_,
                                 planInfos_.get(planNum));
                break;
            default:
                throw new UDRException(
                    38900,
                    "Invalid call phase %d in LmUDRObjMethodInvoke.invokeRoutineMethod()",
                    callPhase);
            }
        } catch (UDRException e) {
            // provide info on the exception in the return status
            result.returnedSQLState_ = e.getSQLState();
            result.returnedMessage_  = e.getMessage();
            result.returnStatus_     = 1;
        }

        // at compile time, return modified invocation/plan info
        if (invocationInfo_.getCallPhase() != UDRInvocationInfo.CallPhase.RUNTIME_WORK_CALL)
            packObjects(result, planNum);

        return result;
    }

    public void setRuntimeInfo(String qid,
                               int totalNumInstances,
                               int myInstanceNum)
    {
        invocationInfo_.setRuntimeInfo(qid, totalNumInstances, myInstanceNum);
    }

    private int validatePlanNum(
         byte [] serializedPlanInfo,
         int planNum) throws UDRException
    {
        int validatedPlanNum = planNum;

        if (serializedPlanInfo != null && serializedPlanInfo.length > 0)
            {
                // make sure the passed plan number is valid and reasonable
                if (planNum < 0 || planNum > 100000)
                    throw new UDRException(
                        38900,
                        "Invalid plan number %d passed to LmUDRObjMethodInvoke.invokeRoutineMethod()",
                        planNum);

                // our planInfos list may be too short, make sure we
                // have an actual UDRPlanInfo object at the right
                // position
                while (planNum >= planInfos_.size())
                    planInfos_.add(new UDRPlanInfo(invocationInfo_, planInfos_.size()));
            }
        else if (planNum != -1)
            throw new UDRException(
                38900,
                "Plan number should be -1 when not passing a UDRPlanInfo to a method invocation");

        return validatedPlanNum;
    }

    private int unpackObjects(
         byte [] serializedInvocationInfo,
         byte [] serializedPlanInfo,
         int planNum) throws UDRException
    {
        if (serializedInvocationInfo != null && serializedInvocationInfo.length > 0)
            {
                ByteBuffer iiByteBuffer = ByteBuffer.wrap(serializedInvocationInfo);

                iiByteBuffer.order(ByteOrder.LITTLE_ENDIAN);
                if (invocationInfo_ == null)
                    invocationInfo_ = new UDRInvocationInfo();
                invocationInfo_.deserialize(iiByteBuffer);
            }
        else if (invocationInfo_ == null)
            throw new UDRException(
                38900,
                "Invocation info is required for object method creation or call");

        planNum = validatePlanNum(serializedPlanInfo, planNum);
        if (planNum >= 0)
            {
                ByteBuffer piByteBuffer = ByteBuffer.wrap(serializedPlanInfo);

                piByteBuffer.order(ByteOrder.LITTLE_ENDIAN);
                planInfos_.get(planNum).deserialize(piByteBuffer);
            }

        return planNum;
    }

    private void packObjects(ReturnInfo ri,
                             int planNum) throws UDRException
    {
        ri.returnedInvocationInfo_ = new byte[invocationInfo_.serializedLength()];

        ByteBuffer iiByteBuffer = ByteBuffer.wrap(ri.returnedInvocationInfo_);

        iiByteBuffer.order(ByteOrder.LITTLE_ENDIAN);
        invocationInfo_.serialize(iiByteBuffer);

        if (planNum >= 0)
            {
                ri.returnedPlanInfo_ = new byte[planInfos_.get(planNum).serializedLength()];
                ByteBuffer piByteBuffer = ByteBuffer.wrap(ri.returnedPlanInfo_);

                piByteBuffer.order(ByteOrder.LITTLE_ENDIAN);
                planInfos_.get(planNum).serialize(piByteBuffer);
            }
        else
            {
                ri.returnedPlanInfo_ = null;
            }
    }
}
