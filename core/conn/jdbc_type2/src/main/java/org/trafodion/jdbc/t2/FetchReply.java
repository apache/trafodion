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

class FetchReply {
    int returnCode;
    int totalErrorLength;
    SQLWarningOrError[] errorList;
    int rowsAffected;
    int outValuesFormat;
    byte[] outValues;

    // -------------------------------------------------------------
    public FetchReply(LogicalByteArray buf, InterfaceConnection ic) throws CharacterCodingException,
           UnsupportedCharsetException {
               buf.setLocation(0);

               returnCode = buf.extractInt();

               if (returnCode != TRANSPORT.SQL_SUCCESS && returnCode != TRANSPORT.NO_DATA_FOUND) {
                   totalErrorLength = buf.extractInt();
                   if (totalErrorLength > 0) {
                       errorList = new SQLWarningOrError[buf.extractInt()];
                       for (int i = 0; i < errorList.length; i++) {
                           errorList[i] = new SQLWarningOrError(buf, ic, InterfaceUtilities.SQLCHARSETCODE_UTF8);
                       }
                   }
               }

               if (errorList == null) {
                   errorList = new SQLWarningOrError[0];
               }

               rowsAffected = buf.extractInt();
               outValuesFormat = buf.extractInt();

               if (returnCode == TRANSPORT.SQL_SUCCESS || returnCode == TRANSPORT.SQL_SUCCESS_WITH_INFO) {
                   outValues = buf.extractByteArray();
               }
    }
}
