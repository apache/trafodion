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

final class T4Statement {
	private int m_queryTimeout;
	private String m_stmtLabel;
	private String m_stmtExplainLabel;
	private static short EXTERNAL_STMT = 0;
	private InterfaceConnection m_ic; 
	private T4Connection m_serverConnection; 

	boolean m_processing = false;

	// -----------------------------------------------------------------------------------
	T4Statement(InterfaceStatement is) throws SQLException {
                m_ic = is.ic_;
		m_serverConnection = m_ic.getT4Connection();
		m_queryTimeout = is.queryTimeout_;
		m_stmtLabel = is.stmtLabel_;
		m_stmtExplainLabel = "";

		if (m_stmtLabel == null) {
		 	m_serverConnection.throwInternalException();
		}
	}// end T4Statement

	// -----------------------------------------------------------------------------------

	ExecuteReply Execute(short executeAPI, int sqlAsyncEnable, int inputRowCnt, int maxRowsetSize, int sqlStmtType,
			int stmtHandle, String sqlString, int sqlStringCharset, String cursorName, int cursorNameCharset,
			String stmtLabel, int stmtLabelCharset, SQL_DataValue_def inputDataValue, SQLValueList_def inputValueList,
			byte[] txId, boolean userBuffer) throws SQLException {
		try {
			m_serverConnection.getInputOutput().setTimeout(m_queryTimeout);

			LogicalByteArray wbuffer = ExecuteMessage.marshal(m_ic.getDialogueId(), sqlAsyncEnable, this.m_queryTimeout,
					inputRowCnt, maxRowsetSize, sqlStmtType, stmtHandle, this.EXTERNAL_STMT, sqlString,
					sqlStringCharset, cursorName, cursorNameCharset, stmtLabel, stmtLabelCharset,
					this.m_stmtExplainLabel, inputDataValue, inputValueList, txId, userBuffer, this.m_ic);

			LogicalByteArray rbuffer = getReadBuffer(executeAPI, wbuffer);

			ExecuteReply er = new ExecuteReply(rbuffer, m_ic);

			return er;
		} catch (SQLException e) {
			throw e;
		} catch (CharacterCodingException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_serverConnection.getLocale(),
					"translation_of_parameter_failed", "ExecuteMessage", e.getMessage());
			se.initCause(e);
			throw se;
		} catch (UnsupportedCharsetException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_serverConnection.getLocale(), "unsupported_encoding", e
					.getCharsetName());
			se.initCause(e);
			throw se;
		} catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_serverConnection.getLocale(), "execute_message_error", e
					.getMessage());
			se.initCause(e);
			throw se;
		}
	} // end Execute

	// -----------------------------------------------------------------------------------
	GenericReply ExecuteGeneric(short executeAPI, byte[] messageBuffer) throws SQLException {
		LogicalByteArray wbuffer = null;
		LogicalByteArray rbuffer = null;
		GenericReply gr = null;

		try {
			m_serverConnection.getInputOutput().setTimeout(m_queryTimeout);
			wbuffer = GenericMessage.marshal(m_serverConnection.getLocale(), messageBuffer, this.m_ic);
			rbuffer = getReadBuffer(executeAPI, wbuffer);
			gr = new GenericReply(m_serverConnection.getLocale(), rbuffer);

			return gr;
		} catch (SQLException se) {
			throw se;
		} catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_serverConnection.getLocale(), "execute_message_error", e
					.getMessage());

			se.initCause(e);
			throw se;
		}
	} // end ExecuteGeneric

	// -----------------------------------------------------------------------------------
	PrepareReply Prepare(int sqlAsyncEnable, short stmtType, int sqlStmtType, String stmtLabel, int stmtLabelCharset,
			String cursorName, int cursorNameCharset, String moduleName, int moduleNameCharset, long moduleTimestamp,
			String sqlString, int sqlStringCharset, String stmtOptions, int maxRowsetSize, byte[] txId

	) throws SQLException {

		if (sqlString == null) {
			m_serverConnection.throwInternalException();
		}
		try {
			m_serverConnection.getInputOutput().setTimeout(m_queryTimeout);

			LogicalByteArray wbuffer = PrepareMessage.marshal(m_serverConnection.getDialogueId(), sqlAsyncEnable, this.m_queryTimeout,
					stmtType, sqlStmtType, stmtLabel, stmtLabelCharset, cursorName, cursorNameCharset, moduleName,
					moduleNameCharset, moduleTimestamp, sqlString, sqlStringCharset, stmtOptions,
					this.m_stmtExplainLabel, maxRowsetSize, txId, this.m_ic);

			LogicalByteArray rbuffer = getReadBuffer(TRANSPORT.SRVR_API_SQLPREPARE, wbuffer);

			PrepareReply pr = new PrepareReply(rbuffer, m_ic);

			return pr;
		} catch (SQLException se) {
			throw se;
		} catch (CharacterCodingException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_serverConnection.getLocale(),
					"translation_of_parameter_failed", "PrepareMessage", e.getMessage());
			se.initCause(e);
			throw se;
		} catch (UnsupportedCharsetException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_serverConnection.getLocale(), "unsupported_encoding", e
					.getCharsetName());
			se.initCause(e);
			throw se;
		} catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_serverConnection.getLocale(), "prepare_message_error", e
					.getMessage());

			se.initCause(e);
			throw se;
		}
	} // end Prepare

	// -----------------------------------------------------------------------------------

	CloseReply Close() throws SQLException {
		try {
			LogicalByteArray wbuffer = CloseMessage.marshal(m_serverConnection.getDialogueId(), m_stmtLabel, InterfaceStatement.SQL_DROP,
					this.m_ic);

			m_serverConnection.getInputOutput().setTimeout(m_queryTimeout);

			LogicalByteArray rbuffer = getReadBuffer(TRANSPORT.SRVR_API_SQLFREESTMT, wbuffer);

			CloseReply cr = new CloseReply(rbuffer, m_serverConnection.getNCSAddress().getIPorName(), m_ic);

			return cr;
		} catch (SQLException se) {
			throw se;
		} catch (CharacterCodingException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_serverConnection.getLocale(),
					"translation_of_parameter_failed", "CloseMessage", e.getMessage());
			se.initCause(e);
			throw se;
		} catch (UnsupportedCharsetException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_serverConnection.getLocale(), "unsupported_encoding", e
					.getCharsetName());
			se.initCause(e);
			throw se;
		} catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_serverConnection.getLocale(), "close_message_error", e
					.getMessage());

			se.initCause(e);
			throw se;
		}
	}

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

	public boolean isProcessing() 
	{
		 return m_processing;
	}

}
