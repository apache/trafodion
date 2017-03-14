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

class PrepareMessage {
    // ----------------------------------------------------------
    static LogicalByteArray marshal(long dialogueId, int sqlAsyncEnable, int queryTimeout, short stmtType,
            int sqlStmtType, String stmtLabel, int stmtLabelCharset, String cursorName, int cursorNameCharset,
            String moduleName, int moduleNameCharset, long moduleTimestamp, String sqlString, int sqlStringCharset,
            String stmtOptions, String stmtExplainLabel, int maxRowsetSize, byte[] txId, InterfaceConnection ic)
        throws CharacterCodingException, UnsupportedCharsetException {
        int wlength = 0;
        LogicalByteArray buf = null;

        byte[] stmtLabelBytes = ic.encodeString(stmtLabel, InterfaceUtilities.SQLCHARSETCODE_UTF8);
        byte[] cursorNameBytes = ic.encodeString(cursorName, InterfaceUtilities.SQLCHARSETCODE_UTF8);
        byte[] moduleNameBytes = ic.encodeString(moduleName, InterfaceUtilities.SQLCHARSETCODE_UTF8);
        byte[] sqlStringBytes = ic.encodeString(sqlString, InterfaceUtilities.SQLCHARSETCODE_UTF8);
        byte[] stmtOptionsBytes = ic.encodeString(stmtOptions, InterfaceUtilities.SQLCHARSETCODE_UTF8);
        byte[] stmtExplainLabelBytes = ic.encodeString(stmtExplainLabel, InterfaceUtilities.SQLCHARSETCODE_UTF8);

        wlength += TRANSPORT.size_long; // dialogueId
        wlength += TRANSPORT.size_int; // sqlAsyncEnable
        wlength += TRANSPORT.size_int; // queryTimeout
        wlength += TRANSPORT.size_short; // stmtType
        wlength += TRANSPORT.size_int; // sqlStmtType
        wlength += TRANSPORT.size_bytesWithCharset(stmtLabelBytes); // +stmtCharset
        wlength += TRANSPORT.size_bytesWithCharset(cursorNameBytes); // +cursorCharset
        wlength += TRANSPORT.size_bytesWithCharset(moduleNameBytes);
        if (moduleName != null && moduleName.length() > 0) {
            wlength += TRANSPORT.size_long; // moduleTimestamp
        }
        wlength += TRANSPORT.size_bytesWithCharset(sqlStringBytes); // +sqlStringCharset
        wlength += TRANSPORT.size_bytes(stmtOptionsBytes);
        wlength += TRANSPORT.size_bytes(stmtExplainLabelBytes);
        wlength += TRANSPORT.size_int; // maxRowsetSize
        wlength += TRANSPORT.size_bytes(txId); // transId

        buf = new LogicalByteArray(wlength, 0, true);

        buf.insertLong(dialogueId);
        buf.insertInt(sqlAsyncEnable);
        buf.insertInt(queryTimeout);
        buf.insertShort(stmtType);
        buf.insertInt(sqlStmtType);
        buf.insertStringWithCharset(stmtLabelBytes, stmtLabelCharset);
        buf.insertStringWithCharset(cursorNameBytes, cursorNameCharset);
        buf.insertStringWithCharset(moduleNameBytes, moduleNameCharset);
        if (moduleName != null && moduleName.length() > 0) {
            buf.insertLong(moduleTimestamp);
        }
        buf.insertStringWithCharset(sqlStringBytes, sqlStringCharset);
        buf.insertString(stmtOptionsBytes);
        buf.insertString(stmtExplainLabelBytes);
        buf.insertInt(maxRowsetSize);
        buf.insertString(txId);

        return buf;
    }
}
