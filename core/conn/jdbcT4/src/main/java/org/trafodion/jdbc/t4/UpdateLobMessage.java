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

public class UpdateLobMessage {
	static LogicalByteArray marshal(short type, String lobHandle, long totalLength, long offset, byte[] data, long pos,
			long length,
			InterfaceConnection ic) throws SQLException {
		int wlength = Header.sizeOf();
		LogicalByteArray buf;

		wlength += TRANSPORT.size_short;

		byte[] lobHandleByets;
		try {
			lobHandleByets = ic.encodeString(lobHandle, InterfaceUtilities.SQLCHARSETCODE_UTF8);

			wlength += TRANSPORT.size_int;
			if (lobHandle.length() > 0) {
				wlength += TRANSPORT.size_bytesWithCharset(lobHandleByets);
			}

			wlength += TRANSPORT.size_long;
			wlength += TRANSPORT.size_long;
			wlength += TRANSPORT.size_long;

			wlength += data.length;

			buf = new LogicalByteArray(wlength, Header.sizeOf(), ic.getByteSwap());

			buf.insertInt(type);
			buf.insertStringWithCharset(lobHandleByets, InterfaceUtilities.SQLCHARSETCODE_ISO88591);

			buf.insertLong(totalLength);
			buf.insertLong(offset);
			buf.insertLong(length);

			buf.insertByteArray(data, (int) pos, (int) length);
			//buf.setDataBuffer(data_);

			return buf;
		} catch (Exception e) {
			throw TrafT4Messages.createSQLException(ic.t4props_, ic.getLocale(), "unsupported_encoding", "UTF-8");
		}
	}
}