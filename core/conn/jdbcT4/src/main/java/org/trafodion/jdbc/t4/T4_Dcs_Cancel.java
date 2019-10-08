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

import java.sql.SQLException;
import java.util.Locale;

/*******************************************************************************
 * 
 * This class corresponds to the ODBC client driver function
 * odbcas_ASSvc_StopSrvr_pst_ as taken from odbcas_drvr.cpp.
 * @version 1.0
 ******************************************************************************/

class T4_Dcs_Cancel {

	/**
	 * This method will establish an initial connection to the ODBC association
	 * server.
	 * 
	 * @param locale
	 *            The locale associated with this operation
	 * @param dialogueId
	 *            A dialogue ID
	 * @param srvrType
	 *            A server type
	 * @param srvrObjRef
	 *            A server object reference
	 * @param stopType
	 *            The stop type
	 * 
	 * @retrun A CancelReply class representing the reply from the association
	 *         server is returned
	 * 
	 * @exception A
	 *                SQLException is thrown
	 */

	static CancelReply cancel(T4Properties t4props, InterfaceConnection ic_, int dialogueId, int srvrType,
			String srvrObjRef, int stopType) throws SQLException {
		Locale locale = ic_.getLocale();

		try {
			LogicalByteArray rbuffer;
			LogicalByteArray wbuffer;
			//
			// Do marshaling of input parameters.
			//
			wbuffer = CancelMessage.marshal(dialogueId, srvrType, srvrObjRef, stopType, ic_);

			//
			// Get the address of the ODBC Association server.
			//
			// T4Address address1 = new T4Address(t4props, locale,
			// ic_.getUrl());
			T4Address address1 = new T4Address(t4props, locale, t4props.getUrl());

			//
			// Send message to the ODBC Association server.
			//
			InputOutput io1 = address1.getInputOutput();
			io1.setInterfaceConnection(ic_);
			io1.openIO();
			io1.setTimeout(ic_.t4props_.getNetworkTimeout());
			io1.setConnectionIdleTimeout(ic_.getConnectionTimeout());

			rbuffer = io1.doIO(TRANSPORT.AS_API_STOPSRVR, wbuffer);

			//
			// Process output parameters
			//

			CancelReply cr1 = new CancelReply(rbuffer, ic_);

			//
			// Close IO
			//
			// io1.setTimeout(ic_.t4props_.getCloseConnectionTimeout());
			io1.setTimeout(ic_.t4props_.getNetworkTimeout());
			io1.closeIO();
			return cr1;
		} // end try
		catch (SQLException se) {
			throw se;
		} catch (Exception e) {
			SQLException se = TrafT4Messages.createSQLException(t4props, locale, "as_cancel_message_error", e
					.getMessage());

			se.initCause(e);
			throw se;
		} // end catch

	} // end getConnection

} // T4_Dcs_Connect
