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

final class InterfaceNativeResultset extends InterfaceNativeConnect {
    private String m_stmtLabel;
    static final short SQL_CLOSE = 0;

    boolean m_processing = false;

    InterfaceNativeResultset(InterfaceResultSet ir) throws SQLException {
        super(ir.ic_);
        m_stmtLabel = ir.stmtLabel_;

        if (m_stmtLabel == null) {
            throwInternalException();
        }
    }

    /**
     * This method will send a fetch rowset command to the server.
     * 
     * @param maxRowCnt
     *            the maximum rowset count to return
     * @param maxRowLen
     *            the maximum row length to return
     * @param sqlAsyncEnable
     *            a flag to enable/disable asynchronies execution
     * @param queryTimeout
     *            the number of seconds before the query times out
     * 
     * @retrun a FetchPerfReply class representing the reply from the ODBC
     *         server is returned
     * 
     * @exception A
     *                SQLException is thrown
     */

    FetchReply Fetch(int sqlAsyncEnable, int queryTimeout, long stmtHandle, int stmtCharset, int maxRowCnt,
            String cursorName, int cursorCharset, String stmtOptions) throws SQLException {

        try {
            byte[] bArray = null;
            int totalBytesReturned;

            LogicalByteArray buffer = FetchMessage.marshal(this.m_dialogueId, sqlAsyncEnable, queryTimeout, stmtHandle,
                    m_stmtLabel, stmtCharset, maxRowCnt, 0 // infinite row size
                    , cursorName, cursorCharset, stmtOptions, this.m_ic);

            bArray = TrafT2NativeInterface.processRequest(TRANSPORT.SRVR_API_SQLFETCH,
                    this.m_dialogueId, stmtHandle, buffer.getBuffer());

            if(bArray == null || bArray.length < 4) {
                SQLException se = Messages.createSQLException(null, m_locale, "problem_with_server_fetch", null);
                SQLException se2 = Messages.createSQLException(null, m_locale, "fetch_reply_buffer_null_or_invalid", null);

                se.setNextException(se2);
                throw se;
            }

            totalBytesReturned = bArray.length;
            buffer.reset();
            buffer.resize(totalBytesReturned); // make sure the buffer is big enough
            System.arraycopy(bArray, 0, buffer.getBuffer(), 0, totalBytesReturned);
            //
            // Process output parameters
            //
            FetchReply frr = new FetchReply(buffer, m_ic);

            return frr;
        } // end try
        catch (SQLException se) {
            throw se;
        } catch (CharacterCodingException e) {
            SQLException se = Messages.createSQLException(m_ic.t2props_, m_locale,
                    "translation_of_parameter_failed", "FetchMessage", e.getMessage());
            se.initCause(e);
            throw se;
        } catch (UnsupportedCharsetException e) {
            SQLException se = Messages.createSQLException(m_ic.t2props_, m_locale, "unsupported_encoding", e
                    .getCharsetName());
            se.initCause(e);
            throw se;
        } catch (Exception e) {
            SQLException se = Messages.createSQLException(m_ic.t2props_, m_locale, "fetch_perf_message_error", e
                    .getMessage());

            se.initCause(e);
            throw se;
        } // end catch

    } // end FetchPerf

    /**
     * This method will send an close command, which does not return any
     * rowsets, to the ODBC server.
     * 
     * @retrun A CloseReply class representing the reply from the ODBC server is
     *         returned
     * 
     * @exception A
     *                SQLException is thrown
     */

    CloseReply Close() throws SQLException {

        try {

            LogicalByteArray wbuffer = CloseMessage.marshal(m_dialogueId, m_stmtLabel, SQL_CLOSE, this.m_ic);

            LogicalByteArray rbuffer = null;

            CloseReply cr = new CloseReply(rbuffer, null, m_ic);

            return cr;
        } // end try
        catch (SQLException se) {
            throw se;
        } catch (CharacterCodingException e) {
            SQLException se = Messages.createSQLException(m_ic.t2props_, m_locale,
                    "translation_of_parameter_failed", "CloseMessage", e.getMessage());
            se.initCause(e);
            throw se;
        } catch (UnsupportedCharsetException e) {
            SQLException se = Messages.createSQLException(m_ic.t2props_, m_locale, "unsupported_encoding", e
                    .getCharsetName());
            se.initCause(e);
            throw se;
        } catch (Exception e) {
            SQLException se = Messages.createSQLException(m_ic.t2props_, m_locale, "close_message_error", e
                    .getMessage());

            se.initCause(e);
            throw se;
        } // end catch

    } // end Close

    // --------------------------------------------------------------------------------
    protected LogicalByteArray getReadBuffer(short odbcAPI, LogicalByteArray wbuffer) throws SQLException {
        LogicalByteArray buf = null;

        try {
            m_processing = true;
            buf = super.getReadBuffer(odbcAPI, wbuffer);
            m_processing = false;
        } catch (SQLException se) {
            m_processing = false;
            throw se;
        }
        return buf;
    }

}
