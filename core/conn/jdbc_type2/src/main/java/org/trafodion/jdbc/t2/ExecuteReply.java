/*******************************************************************************
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
 *******************************************************************************/

package org.trafodion.jdbc.t2;

import java.nio.charset.CharacterCodingException;
import java.nio.charset.UnsupportedCharsetException;

class ExecuteReply {
    int returnCode;
    int totalErrorLength;
    SQLWarningOrError[] errorList;
    long rowsAffected;
    int queryType;
    int estimatedCost;
    byte[] outValues;

    int numResultSets;
    Descriptor2[][] outputDesc;
    String stmtLabels[];

    int outputParamLength;
    int outputNumberParams;

    String[] proxySyntax;

    // ----------------------------------------------------------
    ExecuteReply(LogicalByteArray buf, InterfaceConnection ic) throws CharacterCodingException,
        UnsupportedCharsetException {

            buf.setLocation(0);

            returnCode = buf.extractInt();

            totalErrorLength = buf.extractInt();

            if (totalErrorLength > 0) {
                errorList = new SQLWarningOrError[buf.extractInt()];
                for (int i = 0; i < errorList.length; i++) {
                    errorList[i] = new SQLWarningOrError(buf, ic, InterfaceUtilities.SQLCHARSETCODE_UTF8);
                }
            }

            int outputDescLength = buf.extractInt();
            if (outputDescLength > 0) {
                outputDesc = new Descriptor2[1][];

                outputParamLength = buf.extractInt();
                outputNumberParams = buf.extractInt();

                outputDesc[0] = new Descriptor2[outputNumberParams];
                for (int i = 0; i < outputNumberParams; i++) {
                    outputDesc[0][i] = new Descriptor2(buf, ic);
                    outputDesc[0][i].setRowLength(outputParamLength);
                }
            }
            rowsAffected = buf.extractUnsignedInt();
            queryType = buf.extractInt();
            estimatedCost = buf.extractInt();

            // 64 bit rowsAffected
            // this is a horrible hack because we cannot change the protocol yet
            // rowsAffected should be made a regular 64 bit value when possible
            rowsAffected |= ((long) estimatedCost) << 32;

            outValues = buf.extractByteArray();

            numResultSets = buf.extractInt();

            if (numResultSets > 0) {
                outputDesc = new Descriptor2[numResultSets][];
                stmtLabels = new String[numResultSets];
                proxySyntax = new String[numResultSets];

                for (int i = 0; i < numResultSets; i++) {
                    buf.extractInt(); // int stmt_handle

                    stmtLabels[i] = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);

                    buf.extractInt(); // long stmt_label_charset
                    outputDescLength = buf.extractInt();

                    int outputParamsLength = 0;
                    int outputNumberParams = 0;
                    Descriptor2[] outputParams = null;

                    if (outputDescLength > 0) {
                        outputParamsLength = buf.extractInt();
                        outputNumberParams = buf.extractInt();

                        outputParams = new Descriptor2[outputNumberParams];
                        for (int j = 0; j < outputNumberParams; j++) {
                            outputParams[j] = new Descriptor2(buf, ic);
                            outputParams[j].setRowLength(outputParamsLength);
                        }
                    }
                    outputDesc[i] = outputParams;
                    proxySyntax[i] = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
                }
            }

            String singleSyntax = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);

            if (proxySyntax == null) {
                proxySyntax = new String[1];
                proxySyntax[0] = singleSyntax;
            }
    }
}
