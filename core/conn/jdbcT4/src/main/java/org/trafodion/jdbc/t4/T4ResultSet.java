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

package org.trafodion.jdbc.t4;

import java.nio.charset.CharacterCodingException;
import java.nio.charset.UnsupportedCharsetException;
import java.sql.SQLException;

final class T4ResultSet {
	private String m_stmtLabel;
	private InterfaceConnection m_ic; 
	private T4Connection m_serverConnection; 
	static final short SQL_CLOSE = 0;

	boolean m_processing = false;

	T4ResultSet(InterfaceResultSet ir) throws SQLException {
		m_ic = ir.ic_;
		m_serverConnection = m_ic.getT4Connection();
		m_stmtLabel = ir.stmtLabel_;

		if (m_stmtLabel == null) {
			m_serverConnection.throwInternalException();

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

	FetchReply Fetch(int sqlAsyncEnable, int queryTimeout, int stmtHandle, int stmtCharset, int maxRowCnt,
			String cursorName, int cursorCharset, String stmtOptions) throws SQLException {

		try {
			m_serverConnection.getInputOutput().setTimeout(queryTimeout);

			LogicalByteArray wbuffer = FetchMessage.marshal(m_ic.getDialogueId(), sqlAsyncEnable, queryTimeout, stmtHandle,
					m_stmtLabel, stmtCharset, maxRowCnt, 0 // infinite row size
					, cursorName, cursorCharset, stmtOptions, this.m_ic);

			LogicalByteArray rbuffer = getReadBuffer(TRANSPORT.SRVR_API_SQLFETCH, wbuffer);

			//
			// Process output parameters
			//
			FetchReply frr = new FetchReply(rbuffer, m_ic);

			return frr;
		} // end try
		catch (SQLException se) {
			throw se;
		} catch (CharacterCodingException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_,  m_serverConnection.getLocale(),
					"translation_of_parameter_failed", "FetchMessage", e.getMessage());
			se.initCause(e);
			throw se;
		} catch (UnsupportedCharsetException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_,  m_serverConnection.getLocale(), "unsupported_encoding", e
					.getCharsetName());
			se.initCause(e);
			throw se;
		} catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_,  m_serverConnection.getLocale(), "fetch_perf_message_error", e
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
			m_serverConnection.getInputOutput().setTimeout(m_ic.getQueryTimeout());

			LogicalByteArray wbuffer = CloseMessage.marshal(m_ic.getDialogueId(), m_stmtLabel, SQL_CLOSE, this.m_ic);

			LogicalByteArray rbuffer = getReadBuffer(TRANSPORT.SRVR_API_SQLFREESTMT, wbuffer);

			CloseReply cr = new CloseReply(rbuffer, m_serverConnection.getNCSAddress().getIPorName(), m_ic);

			return cr;
		} // end try
		catch (SQLException se) {
			throw se;
		} catch (CharacterCodingException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_,  m_serverConnection.getLocale(),
					"translation_of_parameter_failed", "CloseMessage", e.getMessage());
			se.initCause(e);
			throw se;
		} catch (UnsupportedCharsetException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_,  m_serverConnection.getLocale(), "unsupported_encoding", e
					.getCharsetName());
			se.initCause(e);
			throw se;
		} catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_,  m_serverConnection.getLocale(), "close_message_error", e
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
			buf = m_serverConnection.getReadBuffer(odbcAPI, wbuffer);
			m_processing = false;
		} catch (SQLException se) {
			m_processing = false;
			throw se;
		}
		return buf;
	}
}
