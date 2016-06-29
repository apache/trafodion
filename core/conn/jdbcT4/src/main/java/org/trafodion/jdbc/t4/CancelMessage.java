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

class CancelMessage {
	static LogicalByteArray marshal(int dialogueId, int srvrType, String srvrObjRef, int stopType,
			InterfaceConnection ic) throws UnsupportedCharsetException, CharacterCodingException {
		int wlength = Header.sizeOf();
		LogicalByteArray buf = null;

		byte[] srvrObjRefBytes = ic.encodeString(srvrObjRef, 1);

		wlength += TRANSPORT.size_int; // dialogueId
		wlength += TRANSPORT.size_int; // srvrType
		wlength += TRANSPORT.size_bytes(srvrObjRefBytes); // srvrObjReference
		wlength += TRANSPORT.size_int; // stopType

		buf = new LogicalByteArray(wlength, Header.sizeOf(), ic.getByteSwap());

		buf.insertInt(dialogueId);
		buf.insertInt(srvrType);
		buf.insertString(srvrObjRefBytes);
		buf.insertInt(stopType);

		return buf;
	}
}
