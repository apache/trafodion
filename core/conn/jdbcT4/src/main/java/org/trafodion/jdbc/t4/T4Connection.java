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
import java.util.Locale;
import java.util.logging.Level;

class T4Connection {
	protected Locale m_locale;
	protected int m_dialogueId;
	protected NCSAddress m_ncsAddress;
	private InputOutput m_io;
	private USER_DESC_def m_userDesc;
	private CONNECTION_CONTEXT_def m_inContext;
	private String m_sessionName;
	InterfaceConnection m_ic;

	static final int INCONTEXT_OPT1_SESSIONNAME = 0x80000000; // (2^31)
	static final int INCONTEXT_OPT1_FETCHAHEAD = 0x40000000; // (2^30)
	static final int INCONTEXT_OPT1_CERTIFICATE_TIMESTAMP = 0x20000000; //(2^29)
	static final int INCONTEXT_OPT1_CLIENT_USERNAME = 0x10000000; //(2^28)

	T4Connection(InterfaceConnection ic) throws SQLException {
		if (ic == null) {
			throwInternalException();

		}
		m_ic = ic;
		m_locale = ic.getLocale();
		m_dialogueId = ic.getDialogueId();
		m_ncsAddress = ic.getNCSAddress();
		m_userDesc = ic.getUserDescription();
		m_inContext = ic.getInContext();
		m_sessionName = ic.getSessionName();

		if (m_dialogueId < 1 || m_ncsAddress == null || m_userDesc == null || m_inContext == null) {
			throwInternalException();

		}
		m_io = m_ncsAddress.getInputOutput();
		if (m_io == null) {
			throwInternalException();
		}
		m_io.setDialogueId(m_dialogueId);
		m_io.setConnectionIdleTimeout(ic.getConnectionTimeout());
		// trace_connection - AM
		m_io.setInterfaceConnection(ic);
		m_io.setNetworkTimeoutInMillis(ic.t4props_.getNetworkTimeoutInMillis());
		m_io.openIO();
		getInputOutput().setTimeout(ic.getLoginTimeout());
		checkConnectionIdleTimeout();
		resetConnectionIdleTimeout();
	}

	public void finalizer() {
		closeTimers();
	}

	protected int getDialogueId() {
		return m_dialogueId;
	}

	protected Locale getLocale() {
		return m_locale;
	}

	protected String getSessionName() {
		return this.m_sessionName;
	}

	protected NCSAddress getNCSAddress() {
		return m_ncsAddress;
	}

	void closeTimers() {
		if (m_io != null) {
			m_io.closeTimers();
		}
	}

	protected void reuse() {
		resetConnectionIdleTimeout();
	}

	private void setConnectionIdleTimeout() {
		m_io.startConnectionIdleTimeout();
	}

	private void resetConnectionIdleTimeout() {
		m_io.resetConnectionIdleTimeout();
	}

	private void checkConnectionIdleTimeout() throws SQLException {
		if (m_io.checkConnectionIdleTimeout()) {
			try {
				m_ic.close();
			} catch (SQLException sqex) {
				// ignores
			}
			throw TrafT4Messages.createSQLException(m_ic.t4props_, m_locale, "ids_s1_t00", null);
		}
	}

	protected boolean connectionIdleTimeoutOccured() {
		return m_io.checkConnectionIdleTimeout();
	}

	protected InputOutput getInputOutput() throws SQLException {
		checkConnectionIdleTimeout();
		resetConnectionIdleTimeout();
		return m_io;
	}

	protected void throwInternalException() throws SQLException {
		T4Properties tempP = null;

		if (m_ic != null) {
			tempP = m_ic.t4props_;

		}
		SQLException se = TrafT4Messages.createSQLException(tempP, m_locale, "internal_error", null);
		SQLException se2 = TrafT4Messages.createSQLException(tempP, m_locale, "contact_traf_error", null);

		se.setNextException(se2);
		throw se;
	}

	// --------------------------------------------------------------------------------
	protected LogicalByteArray getReadBuffer(short odbcAPI, LogicalByteArray wbuffer) throws SQLException {
		// trace_connection - AM
		if (m_ic.t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(m_ic.t4props_);
			String temp = "LogicalByteArray";
			m_ic.t4props_.t4Logger_.logp(Level.FINEST, "T4Connection", "getReadBuffer", temp, p);
		}
		LogicalByteArray rbuffer = m_io.doIO(odbcAPI, wbuffer);

		return rbuffer;
	}

	// --------------------------------------------------------------------------------
	/**
	 * This class corresponds to the ODBC client driver function
	 * odbc_SQLSvc_InitializeDialogue_pst_ as taken from odbccs_drvr.cpp.
	 * @version 1.0
	 * 
	 * This method will make a connection to an ODBC server. The ODBC server's
	 * locaiton This method will make a connection to an ODBC server. The ODBC
	 * server's locaiton (i.e. ip address and port number), were provided by an
	 * earlier call to the ODBC association server.
	 * 
	 * @param inContext
	 *            a CONNETION_CONTEXT_def object containing connection
	 *            information
	 * @param userDesc
	 *            a USER_DESC_def object containing user information
	 * @param inContext
	 *            a CONNECTION_CONTEXT_def object containing information for
	 *            this connection
	 * @param dialogueId
	 *            a unique id identifing this connection as supplied by an
	 *            earlier call to the association server
	 * 
	 * @retrun a InitializeDialogueReply class representing the reply from the
	 *         ODBC server is returned
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */

	InitializeDialogueReply InitializeDialogue(boolean setTimestamp, boolean downloadCert) throws SQLException {
		try {
			int optionFlags1 = INCONTEXT_OPT1_CLIENT_USERNAME;
			int optionFlags2 = 0;
			
			if(setTimestamp) {
				optionFlags1 |= INCONTEXT_OPT1_CERTIFICATE_TIMESTAMP;
			}

			if (m_sessionName != null && m_sessionName.length() > 0) {
				optionFlags1 |= INCONTEXT_OPT1_SESSIONNAME;
			}

			if (this.m_ic.t4props_.getFetchAhead()) {
				optionFlags1 |= INCONTEXT_OPT1_FETCHAHEAD;
			}

			// trace_connection - AM
			if (m_ic.t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(m_ic.t4props_);
				String temp = "m_dialogueId=" + m_dialogueId;
				m_ic.t4props_.t4Logger_.logp(Level.FINEST, "T4Connection", "InitializeDialogue", temp, p);
			}
			LogicalByteArray wbuffer = InitializeDialogueMessage.marshal(m_userDesc, m_inContext, m_dialogueId,
					optionFlags1, optionFlags2, m_sessionName, m_ic);

			getInputOutput().setTimeout(m_ic.t4props_.getLoginTimeout());

			LogicalByteArray rbuffer = getReadBuffer(TRANSPORT.SRVR_API_SQLCONNECT, wbuffer);

			//
			// Process output parameters
			//
			InitializeDialogueReply idr1 = new InitializeDialogueReply(rbuffer, m_ncsAddress.getIPorName(), m_ic, downloadCert);

			return idr1;
		} catch (SQLException se) {
			throw se;
		} catch (CharacterCodingException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale,
					"translation_of_parameter_failed", "InitializeDialogueMessage", e.getMessage());
			se.initCause(e);
			throw se;
		} catch (UnsupportedCharsetException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale, "unsupported_encoding", e
					.getCharsetName());
			se.initCause(e);
			throw se;
		}

		catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale,
					"initialize_dialogue_message_error", e.getMessage());

			se.initCause(e);
			throw se;
		} // end catch
	} // end InitializeDialogue

	/**
	 * This method will end a connection to an ODBC server. The ODBC server's
	 * locaiton (i.e. ip address and port number), were provided by an earlier
	 * call to the ODBC association server.
	 * 
	 * @retrun a TerminateDialogueReply class representing the reply from the
	 *         ODBC server is returned
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */
	TerminateDialogueReply TerminateDialogue() throws SQLException {
		try {
			// trace_connection - AM
			if (m_ic.t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(m_ic.t4props_);
				String temp = "m_dialogueId=" + m_dialogueId;
				m_ic.t4props_.t4Logger_.logp(Level.FINEST, "T4Connection", "TerminateDialogue", temp, p);
			}
			LogicalByteArray wbuffer = TerminateDialogueMessage.marshal(m_dialogueId, this.m_ic);

			//
			// used m_ic instead of getInputOutput, because getInputOutput
			// implicitly calls close at timeout, which will call
			// TerminateDialogue
			// which causes recursion.
			//
			// m_io.setTimeout(m_ic.t4props_.getCloseConnectionTimeout());
			m_io.setTimeout(m_ic.t4props_.getLoginTimeout());

			LogicalByteArray rbuffer = getReadBuffer(TRANSPORT.SRVR_API_SQLDISCONNECT, wbuffer);

			//
			// Process output parameters
			//
			TerminateDialogueReply tdr1 = new TerminateDialogueReply(rbuffer, m_ncsAddress.getIPorName(), m_ic);

			//
			// Send a close message and close the port if we don't have an
			// error.
			// If there is an error, it's up to the calling routine to decide
			// what to do.
			//
			if (tdr1.m_p1.exception_nr == TRANSPORT.CEE_SUCCESS) {
				m_io.closeIO();
			}

			closeTimers();

			return tdr1;
		} // end try
		catch (SQLException se) {
			throw se;
		} catch (CharacterCodingException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale,
					"translation_of_parameter_failed", "TerminateDialogMessage", e.getMessage());
			se.initCause(e);
			throw se;
		} catch (UnsupportedCharsetException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale, "unsupported_encoding", e
					.getCharsetName());
			se.initCause(e);
			throw se;
		} catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale,
					"terminate_dialogue_message_error", e.getMessage());

			se.initCause(e);
			throw se;
		} // end catch
	} // end TerminateDialogue

	/**
	 * This method will send a set connection option command to the server.
	 * 
	 * @param connetionOption
	 *            The connection option to be set
	 * @param optionValueNum
	 *            The number value of the option
	 * @param optionValueStr
	 *            The string value of the option
	 * 
	 * @retrun a SetConnectionOptionReply class representing the reply from the
	 *         ODBC server is returned
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */
	SetConnectionOptionReply SetConnectionOption(short connectionOption, int optionValueNum, String optionValueStr)
			throws SQLException {

		if (optionValueStr == null) {
			throwInternalException();

		}
		try {

			LogicalByteArray wbuffer = SetConnectionOptionMessage.marshal(m_dialogueId, connectionOption,
					optionValueNum, optionValueStr, this.m_ic);

			getInputOutput().setTimeout(m_ic.getQueryTimeout());

			LogicalByteArray rbuffer = getReadBuffer(TRANSPORT.SRVR_API_SQLSETCONNECTATTR, wbuffer);

			SetConnectionOptionReply scor = new SetConnectionOptionReply(rbuffer, m_ncsAddress.getIPorName(), m_ic);

			return scor;
		} // end try
		catch (SQLException se) {
			throw se;
		} catch (CharacterCodingException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale,
					"translation_of_parameter_failed", "SetConnectionOptionReply", e.getMessage());
			se.initCause(e);
			throw se;
		} catch (UnsupportedCharsetException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale, "unsupported_encoding", e
					.getCharsetName());
			se.initCause(e);
			throw se;
		} catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale,
					"set_connection_option_message_error", e.getMessage());

			se.initCause(e);
			throw se;
		} // end catch
	} // end SetConnectionOption

	/**
	 * This method will send an End Transaction command, which does not return
	 * any rowsets, to the ODBC server.
	 * 
	 * @param transactionOpt
	 *            A transaction opt
	 * 
	 * @retrun A EndTransactionReply class representing the reply from the ODBC
	 *         server is returned
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */
	EndTransactionReply EndTransaction(short transactionOpt) throws SQLException {

		try {
			LogicalByteArray wbuffer = EndTransactionMessage.marshal(m_dialogueId, transactionOpt, this.m_ic);

			getInputOutput().setTimeout(m_ic.getQueryTimeout());

			LogicalByteArray rbuffer = getReadBuffer(TRANSPORT.SRVR_API_SQLENDTRAN, wbuffer);

			EndTransactionReply cr = new EndTransactionReply(rbuffer, m_ncsAddress.getIPorName(), m_ic);
			return cr;
		} // end try
		catch (SQLException se) {
			throw se;
		} catch (CharacterCodingException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale,
					"translation_of_parameter_failed", "EndTransactionMessage", e.getMessage());
			se.initCause(e);
			throw se;
		} catch (UnsupportedCharsetException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale, "unsupported_encoding", e
					.getCharsetName());
			se.initCause(e);
			throw se;
		} catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale, "end_transaction_message_error",
					e.getMessage());

			se.initCause(e);
			throw se;
		} // end catch

	} // end EndTransaction

	/**
	 * This method will send an get SQL catalogs command to the ODBC server.
	 * 
	 * @param stmtLabel
	 *            a statement label for use by the ODBC server
	 * @param APIType
	 * @param catalogNm
	 * @param schemaNm
	 * @param tableNm
	 * @param tableTypeList
	 * @param columnNm
	 * @param columnType
	 * @param rowIdScope
	 * @param nullable
	 * @param uniqueness
	 * @param accuracy
	 * @param sqlType
	 * @param metadataId
	 * @param fkcatalogNm
	 * @param fkschemaNm
	 * @param fktableNm
	 * 
	 * @retrun a GetSQLCatalogsReply class representing the reply from the ODBC
	 *         server is returned
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */
	GetSQLCatalogsReply GetSQLCatalogs(String stmtLabel, short APIType, String catalogNm, String schemaNm,
			String tableNm, String tableTypeList, String columnNm, int columnType, int rowIdScope, int nullable,
			int uniqueness, int accuracy, short sqlType, int metadataId, String fkcatalogNm, String fkschemaNm,
			String fktableNm) throws SQLException {

		if (stmtLabel == null) {
			throwInternalException();

		}
		try {
			LogicalByteArray wbuffer;

			wbuffer = GetSQLCatalogsMessage.marshal(m_dialogueId, stmtLabel, APIType, catalogNm, schemaNm, tableNm,
					tableTypeList, columnNm, columnType, rowIdScope, nullable, uniqueness, accuracy, sqlType,
					metadataId, fkcatalogNm, fkschemaNm, fktableNm, m_ic);

			getInputOutput().setTimeout(m_ic.getQueryTimeout());

			LogicalByteArray rbuffer = getReadBuffer(TRANSPORT.SRVR_API_GETCATALOGS, wbuffer);

			//
			// Process output parameters
			//
			GetSQLCatalogsReply gscr = new GetSQLCatalogsReply(rbuffer, m_ncsAddress.getIPorName(), m_ic);

			return gscr;
		} // end try
		catch (SQLException se) {
			throw se;
		} catch (CharacterCodingException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale,
					"translation_of_parameter_failed", "GetSQLCatalogsMessage", e.getMessage());
			se.initCause(e);
			throw se;
		} catch (UnsupportedCharsetException e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale, "unsupported_encoding", e
					.getCharsetName());
			se.initCause(e);
			throw se;
		} catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(m_ic.t4props_, m_locale,
					"get_sql_catalogs_message_error", e.getMessage());

			se.initCause(e);
			throw se;
		} // end catch

	} // end GetSQLCatalogs

}
