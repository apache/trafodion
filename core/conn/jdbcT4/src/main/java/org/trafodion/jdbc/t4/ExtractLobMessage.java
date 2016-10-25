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

class ExtractLobMessage {

	static final short LOB_EXTRACT_LEN                 = 0;
	static final short LOB_EXTRACT_BUFFER              = LOB_EXTRACT_LEN + 1;
	static final short LOB_EXTRATC_BOTH_LEN_AND_BUFFER = LOB_EXTRACT_BUFFER + 1;

	static LogicalByteArray marshal(short extractType, String lobHandle, int lobHandleCharset, long lobLength, InterfaceConnection ic) throws CharacterCodingException, UnsupportedCharsetException{
		int wlength = Header.sizeOf();
		LogicalByteArray buf;

		byte[] lobHandleBytes = ic.encodeString(lobHandle, InterfaceUtilities.SQLCHARSETCODE_UTF8);

		wlength += TRANSPORT.size_int;
		//wlength += TRANSPORT.size_long; // length of lobHandle

		if (lobHandle.length() > 0) {
			wlength += TRANSPORT.size_bytesWithCharset(lobHandleBytes);
		}

		if (lobLength > 0) {
			wlength += TRANSPORT.size_long;
		}

		buf = new LogicalByteArray(wlength, Header.sizeOf(), ic.getByteSwap());

		buf.insertInt(extractType);
		//buf.insertInt(lobHandle.length());
		buf.insertStringWithCharset(lobHandleBytes, lobHandleCharset);
		return buf;
	}
}