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

class GetSQLCatalogsReply {
	odbc_SQLSvc_GetSQLCatalogs_exc_ m_p1;
	String m_p2;
	SQLItemDescList_def m_p3;
	ERROR_DESC_LIST_def m_p4;
	String proxySyntax = "";

	// -------------------------------------------------------------
	GetSQLCatalogsReply(LogicalByteArray buf, String addr, InterfaceConnection ic) throws CharacterCodingException,
			UnsupportedCharsetException, SQLException {
		buf.setLocation(Header.sizeOf());

		m_p1 = new odbc_SQLSvc_GetSQLCatalogs_exc_();
		m_p1.extractFromByteArray(buf, addr, ic);

		if (m_p1.exception_nr == TRANSPORT.CEE_SUCCESS) {
			m_p2 = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);

			m_p3 = new SQLItemDescList_def(buf, true, ic);

			m_p4 = new ERROR_DESC_LIST_def();
			m_p4.extractFromByteArray(buf, ic);

			proxySyntax = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
		}
	}
}
