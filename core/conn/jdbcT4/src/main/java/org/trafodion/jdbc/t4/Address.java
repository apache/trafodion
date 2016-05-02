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

/**********************************************************
 * This class represents an address reference.
 *
 *
 **********************************************************/

import java.net.InetAddress;
import java.sql.SQLException;
import java.util.Locale;
import java.util.Properties;

abstract class Address {
	protected Locale m_locale;
	protected T4Properties m_t4props;
	protected String m_ipAddress;
	protected String m_machineName;
	protected String m_processName;
	protected Integer m_portNumber;
	protected Properties m_properties;
	InetAddress[] m_inetAddrs;
	protected int m_type;
	protected String m_url;
	protected InputOutput m_io;

	/**
	 * The constructor.
	 * 
	 * @param addr
	 *            The addr has two forms:
	 * 
	 * DriverManager getConnection addr parameter format for connecting via the
	 * Fast JDBC Type 3 driver.
	 * 
	 * jdbc:subprotocol:subname
	 * 
	 * Where:
	 * 
	 * subprotocol = t4jdbc
	 * 
	 * subname = //<{IP Address|Machine Name}[:port]>/<database name>
	 * 
	 * Example: jdbc:t4jdbc://130.168.200.30:1433/database1
	 * 
	 * 
	 * ODBC server connect format returned by the ODBC Association Server.
	 * 
	 * TCP:\<{IP Address|Machine Name}>.<Process Name>/<port>:ODBC
	 * 
	 */

	// ----------------------------------------------------------
	Address(T4Properties t4props, Locale locale, String addr) throws SQLException {
		m_t4props = t4props;
		m_locale = locale;
		m_url = addr;
	}

	abstract String recreateAddress();

	// ----------------------------------------------------------
	String getIPorName() {
		if (m_machineName != null) {
			return m_machineName;
		} else {
			return m_ipAddress;
		}
	} // end getIPorName

	protected boolean validateAddress() throws SQLException {
		String IPorName = getIPorName();
		try {
			m_inetAddrs = InetAddress.getAllByName(IPorName);
		} catch (Exception e) {
			SQLException se = HPT4Messages.createSQLException(m_t4props, m_locale, "address_lookup_error", m_url, e
					.getMessage());
			se.initCause(e);
			throw se;
		}
		return true;
	}

	// ----------------------------------------------------------
	Integer getPort() {
		return m_portNumber;
	} // end getIPorName

	void setInputOutput() {
		m_io = new InputOutput(m_locale, this);
	}

	InputOutput getInputOutput() {
		return m_io;
	}
} // end class Address
