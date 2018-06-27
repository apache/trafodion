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

class ExtractLobReply {
	int totalErrorLength;	
	SQLWarningOrError[] errorList;

	odbc_SQLSvc_ExtractLob_exc_ m_p1;
	String proxySyntax = "";

	long lobLength = 0;
	long extractLen = 0;
	byte[] extractData = null;

	public short extractAPIType = 0;

	ExtractLobReply(LogicalByteArray buf, InterfaceConnection ic) throws SQLException {
		buf.setLocation(Header.sizeOf());

		try {
		m_p1 = new odbc_SQLSvc_ExtractLob_exc_();
		m_p1.extractFromByteArray(buf, ic);

		if (m_p1.exception_nr == TRANSPORT.CEE_SUCCESS) {

			extractAPIType = buf.extractShort();
			switch (extractAPIType) {
			case 0:
				lobLength = buf.extractLong();
				break;
			case 1:
				extractLen = buf.extractLong();
				extractData = buf.extractByteArray(extractLen);
				break;
			case 2:
				break;
			default:
				break;
			}
		}
		}
		catch (SQLException e) {
			throw e;
		}
		catch (Exception e) {
			throw TrafT4Messages.createSQLException(ic.t4props_, ic.getLocale(), "unsupported_encoding", "UTF-8");
		}
	}
}