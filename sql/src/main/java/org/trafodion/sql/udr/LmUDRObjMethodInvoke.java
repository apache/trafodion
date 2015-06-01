/**********************************************************************
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
    // dummy class for now
    class UDR {};

    /* temporarily commented out
    UDR udr_;
    UDRInvocationInfo invocationInfo_;
    Vector<UDRPlanInfo> planInfos_;
    */

    // factory method, make a new object of this class
    static LmUDRObjMethodInvoke makeNewObj(
            UDR udrObj,
            byte[] serializedInvocationInfo,
            byte[] serializedPlanInfo) // temporarily commented out throws UDRException
    {
        LmUDRObjMethodInvoke result = new LmUDRObjMethodInvoke();

        /* temporarily commented out
        result.udr_ = udrObj;
        result.unpackObjects(serializedInvocationInfo,
                             serializedPlanInfo,
                             0);

        if (((result.invocationInfo_.getDebugFlags() &
              UDRInvocationInfo.DebugFlags.PRINT_INVOCATION_INFO_INITIAL) != 0) &&
            serializedPlanInfo == null)
            {
                // serializedPlanInfo == null indicates that we are in the compile
                // phase, print the initial value of this object
                result.invocationInfo_.print();
            }
        */

        return result;
    }

    public static class ReturnInfo
    {
        int returnStatus_;
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
         int planNum) // temporarily commented out throws UDRException
    {
        ReturnInfo result = new ReturnInfo(-1);

        planNum = validatePlanNum(serializedPlanInfo, planNum);
        /* temporarily commented out
        unpackObjects(serializedInvocationInfo,
                      serializedPlanInfo,
                      planNum);
        invocationInfo_.setCallPhase(callPhase);

        switch (invocationInfo_.getCallPhase()) {
        case COMPILER_INITIAL_CALL:
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
            break;
        case RUNTIME_WORK_CALL:
            udr_.processData(invocationInfo_,
                             planInfos_.get(planNum));
            break;
        default:
            throw new UDRException(
               38900,
               "Invalid call phase %d in LmUDRObjMethodInvoke.invokeRoutineMethod()",
               callPhase);
        }

        // at compile time, return modified invocation/plan info
        if (invocationInfo_.getCallPhase() != UDRInvocationInfo.CallPhase.RUNTIME_WORK_CALL)
            packObjects(result, 0);
        */

        return result;
    }

    private int validatePlanNum(
         byte [] serializedPlanInfo,
         int planNum) // temporarily commented out throws UDRException
    {
        int validatedPlanNum = planNum;

        /* temporarily commented out
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
                while (planNum < planInfos_.size())
                    planInfos_.add(new UDRPlanInfo(invocationInfo_));
            }
        else if (planNum != -1)
            throw new UDRException(
                38900,
                "Plan number should be -1 when not passing a UDRPlanInfo to a method invocation");
        */

        return validatedPlanNum;
    }

    private void unpackObjects(
         byte [] serializedInvocationInfo,
         byte [] serializedPlanInfo,
         int planNum) // temporarily commented out throws UDRException
    {
        System.out.println("in unpackObjects");

        /* temporarily commented out
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

        if (planNum >= 0)
            {
                ByteBuffer piByteBuffer = ByteBuffer.wrap(serializedPlanInfo);

                piByteBuffer.order(ByteOrder.LITTLE_ENDIAN);
                planInfos_.get(planNum).deserialize(piByteBuffer);
            }
        */

    }

    private void packObjects(ReturnInfo ri,
                             int returnStatus,
                             int planNum)
    {
        ri.returnStatus_ = returnStatus;

        ByteBuffer iiByteBuffer = ByteBuffer.wrap(ri.returnedInvocationInfo_);

        iiByteBuffer.order(ByteOrder.LITTLE_ENDIAN);
        /* temporarily commented out
        invocationInfo_.serialize(iiByteBuffer);

        if (planNum >= 0)
            {
                ByteBuffer piByteBuffer = ByteBuffer.wrap(ri.returnedPlanInfo_);

                piByteBuffer.order(ByteOrder.LITTLE_ENDIAN);
                planInfos_.get(planNum).serialize(piByteBuffer);
            }
        */
    }
}