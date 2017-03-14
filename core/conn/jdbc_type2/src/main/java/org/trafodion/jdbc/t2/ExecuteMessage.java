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

class ExecuteMessage {
    // ----------------------------------------------------------
    static LogicalByteArray marshal(long dialogueId, int sqlAsyncEnable, int queryTimeout, int inputRowCnt,
            int maxRowsetSize, int sqlStmtType, int stmtHandleKey, int stmtType, String sqlString, int sqlStringCharset,
            String cursorName, int cursorNameCharset, String stmtLabel, int stmtLabelCharset, String stmtExplainLabel,
            SQL_DataValue_def inputDataValue, SQLValueList_def inputValueList, byte[] txId, boolean isUserBuffer,
            InterfaceConnection ic

            ) throws CharacterCodingException, UnsupportedCharsetException

    {
        int wlength = 0;
        LogicalByteArray buf;

        byte[] sqlStringBytes = ic.encodeString(sqlString, InterfaceUtilities.SQLCHARSETCODE_UTF8);
        byte[] cursorNameBytes = ic.encodeString(cursorName, InterfaceUtilities.SQLCHARSETCODE_UTF8);
        byte[] stmtLabelBytes = ic.encodeString(stmtLabel, InterfaceUtilities.SQLCHARSETCODE_UTF8);
        byte[] stmtExplainLabelBytes = ic.encodeString(stmtExplainLabel, InterfaceUtilities.SQLCHARSETCODE_UTF8);
        wlength += TRANSPORT.size_long; // dialogueId
        wlength += TRANSPORT.size_int; // sqlAsyncEnable
        wlength += TRANSPORT.size_int; // queryTimeout
        wlength += TRANSPORT.size_int; // inputRowCnt
        wlength += TRANSPORT.size_int; // maxRowsetSize
        wlength += TRANSPORT.size_int; // sqlStmtType
        wlength += TRANSPORT.size_int; // stmtHandleKey
        wlength += TRANSPORT.size_int; // stmtType
        wlength += TRANSPORT.size_bytesWithCharset(sqlStringBytes); // +sqlStringCharset
        wlength += TRANSPORT.size_bytesWithCharset(cursorNameBytes); // +cursorNameCharset
        wlength += TRANSPORT.size_bytesWithCharset(stmtLabelBytes); // +stmtLabelCharset
        wlength += TRANSPORT.size_bytes(stmtExplainLabelBytes);

        if (!isUserBuffer) {
            wlength += inputDataValue.sizeof();
            wlength += TRANSPORT.size_bytes(txId); // transId
        }
        buf = new LogicalByteArray(wlength, 0, true);

        buf.insertLong(dialogueId);
        buf.insertInt(sqlAsyncEnable);
        buf.insertInt(queryTimeout);
        buf.insertInt(inputRowCnt);
        buf.insertInt(maxRowsetSize);
        buf.insertInt(sqlStmtType);
        buf.insertInt(0);  // we don't need stmtHandleKey in this buffer, and it even may cause overflow
        buf.insertInt(stmtType);
        buf.insertStringWithCharset(sqlStringBytes, sqlStringCharset);
        buf.insertStringWithCharset(cursorNameBytes, cursorNameCharset);
        buf.insertStringWithCharset(stmtLabelBytes, stmtLabelCharset);
        buf.insertString(stmtExplainLabelBytes);

        if (isUserBuffer) {
            buf.setDataBuffer(inputDataValue.userBuffer);

            byte[] trailer = null;
            if (txId == null || txId.length == 0) {
                trailer = new byte[4];
                for (int i = 0; i < 4; ++i) {
                    trailer[i] = (byte) 0;
                }
            } else {
                int len = txId.length + 1;
                trailer = new byte[4 + txId.length + 1];

                trailer[0] = (byte) ((len >>> 24) & 0xff);
                trailer[1] = (byte) ((len >>> 16) & 0xff);
                trailer[2] = (byte) ((len >>> 8) & 0xff);
                trailer[3] = (byte) ((len) & 0xff);
                System.arraycopy(txId, 0, trailer, 4, txId.length);
                trailer[len + 4 - 1] = '\0';
            }

            buf.setTrailer(trailer);
        } else {
            byte[] c = inputDataValue.buffer;
            inputDataValue.insertIntoByteArray(buf);
            buf.insertString(txId);
        }

        return buf;
    }
}
