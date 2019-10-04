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

class T4_Dcs_Connect {

	/**
	 * This method will establish an initial connection to the ODBC association
	 * server.
	 * 
	 * @param locale
	 *            The locale associated with this operation
	 * @param inContext
	 *            A CONNETION_CONTEXT_def object containing connection
	 *            information
	 * @param userDesc
	 *            A USER_DESC_def object containing user information
	 * @param srvrType
	 *            A server type
	 * @param retryCount
	 *            The number of times to retry the connection
	 * 
	 * @retrun A ConnectReply class representing the reply from the association
	 *         server is returned
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */
	static ConnectReply getConnection(T4Properties t4props, InterfaceConnection ic_, CONNECTION_CONTEXT_def inContext,
			USER_DESC_def userDesc, int srvrType, short retryCount) throws SQLException {
		Locale locale = ic_.getLocale();

		if (inContext == null || userDesc == null) {
			SQLException se = TrafT4Messages.createSQLException(t4props, locale, "internal_error", null);
			SQLException se2 = TrafT4Messages.createSQLException(t4props, locale, "contact_traf_error", null);

			se.setNextException(se2);
			throw se;
		}
		InputOutput io1 = null;
		try {
			LogicalByteArray rbuffer;
			LogicalByteArray wbuffer;

			// Do marshaling of input parameters.
			wbuffer = ConnectMessage.marshal(inContext, userDesc, srvrType, retryCount, T4Connection.INCONTEXT_OPT1_CLIENT_USERNAME, 0, Vproc.getVproc(), ic_);

			// Get the address of the ODBC Association server.
			T4Address address1 = new T4Address(t4props, locale, ic_.getUrl());

			// Open the connection
			io1 = address1.getInputOutput();
                        io1.setInterfaceConnection(ic_);
			io1.setTimeout(ic_.getLoginTimeout());
			io1.openIO();
			io1.setConnectionIdleTimeout(ic_.getConnectionTimeout());

			// Send message to the ODBC Association server.
			rbuffer = io1.doIO(TRANSPORT.AS_API_GETOBJREF, wbuffer);

			// Process output parameters
			ConnectReply cr1 = new ConnectReply(rbuffer, ic_);

			// Close IO
			io1.setTimeout(ic_.t4props_.getLoginTimeout());
			io1.closeIO(); // Note, we are re-using the wbuffer
                        io1 = null;

			String name1 = null;
			if (address1.m_ipAddress != null) {
				name1 = address1.m_ipAddress;
			} else if (address1.m_machineName != null) {
				name1 = address1.m_machineName;

			}
			cr1.fixupSrvrObjRef(t4props, locale, name1);
			
			ic_.setConnStrHost(address1.getIPorName());

			return cr1;
		} catch (SQLException se) {
			throw se;
		} catch (CharacterCodingException e) {
			SQLException se = TrafT4Messages.createSQLException(ic_.t4props_, locale, "translation_of_parameter_failed",
					"ConnectMessage", e.getMessage());
			se.initCause(e);
			throw se;
		} catch (UnsupportedCharsetException e) {
			SQLException se = TrafT4Messages.createSQLException(ic_.t4props_, locale, "unsupported_encoding", e
					.getCharsetName());
			se.initCause(e);
			throw se;
		} catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(t4props, locale, "as_connect_message_error", e
					.getMessage());

			se.initCause(e);
			throw se;
		} finally {
			if (io1 != null)
			    io1.closeIO();
		}
	}
}
