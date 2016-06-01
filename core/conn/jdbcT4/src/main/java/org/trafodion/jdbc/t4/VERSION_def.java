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

class VERSION_def {
	short componentId;
	short majorVersion;
	short minorVersion;
	int buildId;

	static int sizeOf() {
		return TRANSPORT.size_int + TRANSPORT.size_short * 3;
	}

	void insertIntoByteArray(LogicalByteArray buf) {
		buf.insertShort(componentId);
		buf.insertShort(majorVersion);
		buf.insertShort(minorVersion);
		buf.insertInt(buildId);
	}
	
	static int sizeOfChar() {
		return 50;
	}
	
	void insertIntoByteArrayChar(LogicalByteArray buf, InterfaceConnection ic) throws CharacterCodingException, UnsupportedCharsetException{
		buf.insertFixedString(ic.encodeString("" + componentId, 1), 10);
		buf.insertFixedString(ic.encodeString("" + majorVersion, 1), 10);
		buf.insertFixedString(ic.encodeString("" + minorVersion, 1), 10);
		buf.insertFixedString(ic.encodeString("" + buildId, 1), 20);
	}

	void extractFromByteArray(LogicalByteArray buf) {
		componentId = buf.extractShort();
		majorVersion = buf.extractShort();
		minorVersion = buf.extractShort();
		buildId = buf.extractInt();
	}
}
