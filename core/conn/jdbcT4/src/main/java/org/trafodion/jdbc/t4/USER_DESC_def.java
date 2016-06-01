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

class USER_DESC_def {
	int userDescType;
	byte[] userSid;
	String domainName;
	String userName;
	byte[] password;

	byte[] domainNameBytes;
	byte[] userNameBytes;

	int sizeOf(InterfaceConnection ic) throws CharacterCodingException, UnsupportedCharsetException {
		int size = 0;

		domainNameBytes = ic.encodeString(domainName, 1);
		userNameBytes = ic.encodeString(userName, 1);

		size += TRANSPORT.size_int; // descType

		size += TRANSPORT.size_bytes(userSid);
		size += TRANSPORT.size_bytes(domainNameBytes);
		size += TRANSPORT.size_bytes(userNameBytes);
		size += TRANSPORT.size_bytes(password);

		return size;
	}

	void insertIntoByteArray(LogicalByteArray buf) {
		buf.insertInt(userDescType);

		buf.insertString(userSid);
		buf.insertString(domainNameBytes);
		buf.insertString(userNameBytes);
		buf.insertString(password);
	}
}
