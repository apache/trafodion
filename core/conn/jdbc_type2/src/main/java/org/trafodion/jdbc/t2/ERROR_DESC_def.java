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

class ERROR_DESC_def {
    int rowId;
    int errorDiagnosticId;
    int sqlcode;
    String sqlstate;
    String errorText;
    int operationAbortId;
    int errorCodeType;
    String Param1;
    String Param2;
    String Param3;
    String Param4;
    String Param5;
    String Param6;
    String Param7;

    // ----------------------------------------------------------
    void extractFromByteArray(LogicalByteArray buffer1, InterfaceConnection ic) throws CharacterCodingException,
         UnsupportedCharsetException {
             rowId = buffer1.extractInt();
             errorDiagnosticId = buffer1.extractInt();
             sqlcode = buffer1.extractInt();

             // Note, SQLSTATE is logically 5 bytes, but ODBC uses 6 bytes for some
             // reason.
             sqlstate = ic.decodeBytes(buffer1.extractByteArray(6), 1);
             errorText = ic.decodeBytes(buffer1.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);

             operationAbortId = buffer1.extractInt();
             errorCodeType = buffer1.extractInt();
             Param1 = ic.decodeBytes(buffer1.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
             Param2 = ic.decodeBytes(buffer1.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
             Param3 = ic.decodeBytes(buffer1.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
             Param4 = ic.decodeBytes(buffer1.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
             Param5 = ic.decodeBytes(buffer1.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
             Param6 = ic.decodeBytes(buffer1.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
             Param7 = ic.decodeBytes(buffer1.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
    }
}
