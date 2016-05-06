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

class SQLItemDesc_def {
    int version;
    int dataType;
    int datetimeCode;
    int maxLen;
    short precision;
    short scale;
    byte nullInfo;
    String colHeadingNm;
    byte signType;
    int ODBCDataType;
    short ODBCPrecision;
    int SQLCharset;
    int ODBCCharset;
    String TableName;
    String CatalogName;
    String SchemaName;
    String Heading;
    int intLeadPrec;
    int paramMode;

    void extractFromByteArray(LogicalByteArray buf, InterfaceConnection ic) throws UnsupportedCharsetException,
         CharacterCodingException {
             version = buf.extractInt();
             dataType = buf.extractInt();
             datetimeCode = buf.extractInt();
             maxLen = buf.extractInt();
             precision = buf.extractShort();
             scale = buf.extractShort();
             nullInfo = buf.extractByte();
             signType = buf.extractByte();
             ODBCDataType = buf.extractInt();
             ODBCPrecision = buf.extractShort();
             SQLCharset = buf.extractInt();
             ODBCCharset = buf.extractInt();
             colHeadingNm = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
             TableName = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
             CatalogName = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
             SchemaName = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
             Heading = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
             intLeadPrec = buf.extractInt();
             paramMode = buf.extractInt();
    }
}
