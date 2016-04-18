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

class FetchMessage {
	// ----------------------------------------------------------
	static LogicalByteArray marshal(int dialogueId, int sqlAsyncEnable, int queryTimeout, int stmtHandle,
			String stmtLabel, int stmtCharset, int maxRowCnt, int maxRowLen, String cursorName, int cursorCharset,
			String stmtOptions, InterfaceConnection ic) throws CharacterCodingException, UnsupportedCharsetException {
		int wlength = Header.sizeOf();
		LogicalByteArray buf;

		byte[] stmtLabelBytes = ic.encodeString(stmtLabel, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		byte[] cursorNameBytes = ic.encodeString(cursorName, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		byte[] stmtOptionsBytes = ic.encodeString(stmtOptions, InterfaceUtilities.SQLCHARSETCODE_UTF8);

		wlength += TRANSPORT.size_int; // dialogueId
		wlength += TRANSPORT.size_int; // sqlAsyncEnable
		wlength += TRANSPORT.size_int; // queryTimeout
		wlength += TRANSPORT.size_int; // stmtHandle
		wlength += TRANSPORT.size_bytesWithCharset(stmtLabelBytes);
		wlength += TRANSPORT.size_long; // maxRowCnt
		wlength += TRANSPORT.size_long; // maxRowLen
		wlength += TRANSPORT.size_bytesWithCharset(cursorNameBytes);
		wlength += TRANSPORT.size_bytes(stmtOptionsBytes);

		buf = new LogicalByteArray(wlength, Header.sizeOf(), ic.getByteSwap());

		buf.insertInt(dialogueId);
		buf.insertInt(sqlAsyncEnable);
		buf.insertInt(queryTimeout);
		buf.insertInt(stmtHandle);
		buf.insertStringWithCharset(stmtLabelBytes, stmtCharset);
		buf.insertLong(maxRowCnt);
		buf.insertLong(maxRowLen);
		buf.insertStringWithCharset(cursorNameBytes, cursorCharset);
		buf.insertString(stmtOptionsBytes);

		return buf;
	}
}
