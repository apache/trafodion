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
import java.sql.SQLException;

final class InterfaceNativeStmt extends InterfaceNativeConnect {
    private int m_queryTimeout;
    private String m_stmtLabel;
    private String m_stmtExplainLabel;
    private static short EXTERNAL_STMT = 0;

    boolean m_processing = false;

    // -----------------------------------------------------------------------------------
    InterfaceNativeStmt(InterfaceStatement is) throws SQLException {
        super(is.ic_);
        m_queryTimeout = is.queryTimeout_;
        m_stmtLabel = is.stmtLabel_;
        m_stmtExplainLabel = "";

        if (m_stmtLabel == null) {
            throwInternalException();
        }
    }// end T2Statement

    // -----------------------------------------------------------------------------------
    PrepareReply Prepare(int sqlAsyncEnable, short stmtType, int sqlStmtType, long stmtHandle, String stmtLabel, 
            int stmtLabelCharset, String cursorName, int cursorNameCharset, String moduleName, int moduleNameCharset,
            long moduleTimestamp, String sqlString, int sqlStringCharset, String stmtOptions, int maxRowsetSize,
            byte[] txId) throws SQLException {

        if (sqlString == null) {
            throwInternalException();
        }

        byte[] bArray = null;
        int totalBytesReturned;

        try {
            LogicalByteArray buffer = PrepareMessage.marshal(this.m_dialogueId, sqlAsyncEnable, this.m_queryTimeout,
                    stmtType, sqlStmtType, stmtLabel, stmtLabelCharset, cursorName, cursorNameCharset, moduleName,
                    moduleNameCharset, moduleTimestamp, sqlString, sqlStringCharset, stmtOptions,
                    this.m_stmtExplainLabel, maxRowsetSize, txId, this.m_ic);

            bArray = TrafT2NativeInterface.processRequest(TRANSPORT.SRVR_API_SQLPREPARE,
                    this.m_dialogueId, stmtHandle, buffer.getBuffer());

            if(bArray == null || bArray.length < 4) {
                SQLException se = Messages.createSQLException(null, m_locale, "problem_with_server_execute", null);
                SQLException se2 = Messages.createSQLException(null, m_locale, "execute_reply_buffer_null_or_invalid", null);

                se.setNextException(se2);
                throw se;
            }

            totalBytesReturned = bArray.length;
            buffer.reset();
            buffer.resize(totalBytesReturned); // make sure the buffer is big enough
            System.arraycopy(bArray, 0, buffer.getBuffer(), 0, totalBytesReturned);

            PrepareReply pr = new PrepareReply(buffer, m_ic);

            return pr;
        } catch (SQLException se) {
            throw se;
        } catch (CharacterCodingException e) {
            SQLException se = Messages.createSQLException(m_ic.t2props_, m_locale,
                    "translation_of_parameter_failed", "PrepareMessage", e.getMessage());
            se.initCause(e);
            throw se;
        } catch (UnsupportedCharsetException e) {
            SQLException se = Messages.createSQLException(m_ic.t2props_, m_locale, "unsupported_encoding", e
                    .getCharsetName());
            se.initCause(e);
            throw se;
        } catch (Exception e) {
            SQLException se = Messages.createSQLException(m_ic.t2props_, m_locale, "prepare_message_error", e.getMessage());
            se.initCause(e);
            throw se;
        }
    } // end Prepare

    public ExecuteReply Execute(short executeAPI, int sqlAsyncEnable, int inputRowCnt, int maxRowsetSize, int sqlStmtType,
            long stmtHandle, String sqlString, int sqlStringCharset, String cursorName, int cursorNameCharset,
            String stmtLabel, int stmtLabelCharset, SQL_DataValue_def inputDataValue, SQLValueList_def inputValueList,
            byte[] txId, boolean userBuffer) throws SQLException {

        byte[] bArray = null;
        int totalBytesReturned;

        try {
            LogicalByteArray buffer = ExecuteMessage.marshal(this.m_dialogueId, sqlAsyncEnable, this.m_queryTimeout,
                    inputRowCnt, maxRowsetSize, sqlStmtType, 0, this.EXTERNAL_STMT, sqlString,
                    sqlStringCharset, cursorName, cursorNameCharset, stmtLabel, stmtLabelCharset,
                    this.m_stmtExplainLabel, inputDataValue, inputValueList, txId, userBuffer, this.m_ic);

            bArray = TrafT2NativeInterface.processRequest(executeAPI, this.m_dialogueId, stmtHandle, buffer.getBuffer());

            if(bArray == null || bArray.length < 4) {
                SQLException se = Messages.createSQLException(null, m_locale, "problem_with_server_execute", null);
                SQLException se2 = Messages.createSQLException(null, m_locale, "execute_reply_buffer_null_or_invalid", null);

                se.setNextException(se2);
                throw se;
            }

            totalBytesReturned = bArray.length;
            buffer.reset();
            buffer.resize(totalBytesReturned); // make sure the buffer is big enough
            System.arraycopy(bArray, 0, buffer.getBuffer(), 0, totalBytesReturned);

            ExecuteReply er = new ExecuteReply(buffer, m_ic);

            return er;
        } catch (SQLException e) {
            throw e;
        } catch (CharacterCodingException e) {
            SQLException se = Messages.createSQLException(m_ic.t2props_, m_locale,
                    "translation_of_parameter_failed", "ExecuteMessage", e.getMessage());
            se.initCause(e);
            throw se;
        } catch (UnsupportedCharsetException e) {
            SQLException se = Messages.createSQLException(m_ic.t2props_, m_locale, "unsupported_encoding", e
                    .getCharsetName());
            se.initCause(e);
            throw se;
        } catch (Exception e) {
            SQLException se = Messages.createSQLException(m_ic.t2props_, m_locale, "execute_message_error", e
                    .getMessage());
            se.initCause(e);
            throw se;
        }
    }

}
