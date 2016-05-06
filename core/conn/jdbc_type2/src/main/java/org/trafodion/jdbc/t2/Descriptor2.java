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

class Descriptor2 {
    int noNullValue_;
    int nullValue_;
    int version_;
    int dataType_;
    int datetimeCode_;
    int maxLen_;
    int precision_;
    int scale_;
    int nullInfo_;
    int signed_;
    int odbcDataType_;
    int odbcPrecision_;
    int sqlCharset_;
    int odbcCharset_;
    int fsDataType_;
    String colHeadingNm_;
    String tableName_;
    String catalogName_;
    String schemaName_;
    String headingName_;
    int intLeadPrec_;
    int paramMode_;

    private int rowLength;

    public void setRowLength(int len) {
        rowLength = len;
    }

    public int getRowLength() {
        return rowLength;
    }

    public Descriptor2(LogicalByteArray buf, InterfaceConnection ic) throws CharacterCodingException,
           UnsupportedCharsetException {
               noNullValue_ = buf.extractInt();
               nullValue_ = buf.extractInt();
               version_ = buf.extractInt();
               dataType_ = buf.extractInt();
               datetimeCode_ = buf.extractInt();
               maxLen_ = buf.extractInt();
               precision_ = buf.extractInt();
               scale_ = buf.extractInt();
               nullInfo_ = buf.extractInt();
               signed_ = buf.extractInt();
               odbcDataType_ = buf.extractInt();
               odbcPrecision_ = buf.extractInt();
               sqlCharset_ = buf.extractInt();
               odbcCharset_ = buf.extractInt();
               fsDataType_ = buf.extractInt();

               colHeadingNm_ = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
               tableName_ = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
               catalogName_ = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
               schemaName_ = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
               headingName_ = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
               intLeadPrec_ = buf.extractInt();
               paramMode_ = buf.extractInt();
    }
}
