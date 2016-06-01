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

class ConnectReply {
	odbc_Dcs_GetObjRefHdl_exc_ m_p1_exception;
	String m_p2_srvrObjRef;
	int m_p3_dialogueId;
	String m_p4_dataSource;
	byte[] m_p5_userSid;
	VERSION_LIST_def m_p6_versionList;
	int isoMapping;
	boolean byteSwap;
	
	boolean securityEnabled;
	int serverNode;
	int processId;
	byte [] timestamp;
	String clusterName;

        String serverHostName="";
	Integer serverNodeId=0;
	Integer serverProcessId=0;
	String serverProcessName="";
	String serverIpAddress="";
	Integer serverPort=0;	
	
        String remoteHost;
	String remoteProcess;

	private NCSAddress m_ncsAddr_;

	// -------------------------------------------------------------
	ConnectReply(LogicalByteArray buf, InterfaceConnection ic) throws SQLException, UnsupportedCharsetException,
			CharacterCodingException {
		buf.setLocation(Header.sizeOf());

		m_p1_exception = new odbc_Dcs_GetObjRefHdl_exc_();
		m_p1_exception.extractFromByteArray(buf, ic);
		
		this.byteSwap = buf.getByteSwap();

		if (m_p1_exception.exception_nr == TRANSPORT.CEE_SUCCESS) {
			m_p3_dialogueId = buf.extractInt();
			m_p4_dataSource = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
			m_p5_userSid = buf.extractByteString(); // byteString -- only place
			// used -- packed length
			// does not include the null
			// term
			m_p6_versionList = new VERSION_LIST_def();
			
			//buf.setByteSwap(false);
			m_p6_versionList.extractFromByteArray(buf);
			//buf.setByteSwap(this.byteSwap);

			buf.extractInt(); //old iso mapping
			
			/*if ((m_p6_versionList.list[0].buildId & InterfaceConnection.CHARSET) > 0) {
				isoMapping = 
			} else {*/
				isoMapping = 15;
			//}
		        serverHostName = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
			serverNodeId = buf.extractInt();
			serverProcessId = buf.extractInt();
			serverProcessName = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
			serverIpAddress = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
			serverPort = buf.extractInt();

			m_p2_srvrObjRef = String.format("TCP:%s:%d.%s,%s/%d:ODBC", serverHostName, serverNodeId, serverProcessName, serverIpAddress, serverPort);	
			
                        if((m_p6_versionList.list[0].buildId & InterfaceConnection.PASSWORD_SECURITY) > 0) {
				securityEnabled = true;
				serverNode = serverNodeId;
				processId = serverProcessId;
				timestamp = buf.extractByteArray(8);
				clusterName = ic.decodeBytes(buf.extractString(), 1);
			}
			else {
				securityEnabled = false;
			}
		}
	}

	// -------------------------------------------------------------
	void fixupSrvrObjRef(T4Properties t4props, Locale locale, String name) throws SQLException {
		//
		// This method will replace the domain name returned from the
		// Association server, with a new name.
		//
		m_ncsAddr_ = null;

		if (m_p2_srvrObjRef != null) {
			remoteHost = m_p2_srvrObjRef.substring(4,m_p2_srvrObjRef.indexOf('$') - 1);
			remoteProcess = m_p2_srvrObjRef.substring(m_p2_srvrObjRef.indexOf('$'), m_p2_srvrObjRef.indexOf(','));
			
			try {
				m_ncsAddr_ = new NCSAddress(t4props, locale, m_p2_srvrObjRef);
			} catch (SQLException e) {
				throw e;
			}

			// use your best guess if m_machineName was not found
			if (m_ncsAddr_.m_machineName == null) {
				if (m_ncsAddr_.m_ipAddress == null) {
					m_ncsAddr_.m_machineName = name;
				} else {
					m_ncsAddr_.m_machineName = m_ncsAddr_.m_ipAddress;
				}
			}

			m_p2_srvrObjRef = m_ncsAddr_.recreateAddress();
			m_ncsAddr_.validateAddress();
			m_ncsAddr_.setInputOutput();

			return;
		} // end if

	} // end fixupSrvrObjRef

	NCSAddress getNCSAddress() {
		return m_ncsAddr_;
	}
}
