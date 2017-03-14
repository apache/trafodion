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
import java.nio.ByteBuffer;

class PrepareReply {
    int returnCode;
    int totalErrorLength;
    SQLWarningOrError[] errorList;
    int sqlQueryType;
    long stmtHandle;
    int estimatedCost;

    int inputDescLength;
    int inputParamLength;
    int inputNumberParams;
    Descriptor2[] inputDesc;

    int outputDescLength;
    int outputParamLength;
    int outputNumberParams;
    Descriptor2[] outputDesc;

    // -------------------------------------------------------------
    PrepareReply(LogicalByteArray buf, InterfaceConnection ic) throws CharacterCodingException,
        UnsupportedCharsetException {
            buf.setLocation(0);

            returnCode = buf.extractInt();

            // should check SQL_SUCCESS or SQL_SUCCESS_WITH_INFO
            if (returnCode == TRANSPORT.SQL_SUCCESS || returnCode == TRANSPORT.SQL_SUCCESS_WITH_INFO) {
                if (returnCode == TRANSPORT.SQL_SUCCESS_WITH_INFO) {
                    totalErrorLength = buf.extractInt();

                    if (totalErrorLength > 0) {
                        errorList = new SQLWarningOrError[buf.extractInt()];
                        for (int i = 0; i < errorList.length; i++) {
                            errorList[i] = new SQLWarningOrError(buf, ic, InterfaceUtilities.SQLCHARSETCODE_UTF8);
                        }
                    }
                }
                sqlQueryType = buf.extractInt();
                stmtHandle = buf.extractLong();
                estimatedCost = buf.extractInt();

                inputDescLength = buf.extractInt();
                if (inputDescLength > 0) {
                    inputParamLength = buf.extractInt();
                    inputNumberParams = buf.extractInt();

                    inputDesc = new Descriptor2[inputNumberParams];
                    for (int i = 0; i < inputNumberParams; i++) {
                        inputDesc[i] = new Descriptor2(buf, ic);
                        inputDesc[i].setRowLength(inputParamLength);
                    }
                }

                outputDescLength = buf.extractInt();
                if (outputDescLength > 0) {
                    outputParamLength = buf.extractInt();
                    outputNumberParams = buf.extractInt();

                    outputDesc = new Descriptor2[outputNumberParams];
                    for (int i = 0; i < outputNumberParams; i++) {
                        outputDesc[i] = new Descriptor2(buf, ic);
                        outputDesc[i].setRowLength(outputParamLength);
                    }
                }
            } else {
                totalErrorLength = buf.extractInt();

                if (totalErrorLength > 0) {
                    errorList = new SQLWarningOrError[buf.extractInt()];
                    for (int i = 0; i < errorList.length; i++) {
                        errorList[i] = new SQLWarningOrError(buf, ic, ic.getISOMapping());
                    }
                }
            }
    }
}
