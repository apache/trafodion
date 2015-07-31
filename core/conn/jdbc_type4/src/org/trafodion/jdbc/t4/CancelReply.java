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

class CancelReply {
	odbc_Dcs_StopSrvr_exc_ m_p1_exception;

	// -------------------------------------------------------------
	CancelReply(LogicalByteArray buf, InterfaceConnection ic) throws SQLException, CharacterCodingException,
			UnsupportedCharsetException {
		buf.setLocation(Header.sizeOf());

		m_p1_exception = new odbc_Dcs_StopSrvr_exc_();
		m_p1_exception.extractFromByteArray(buf, ic);
	}
}
