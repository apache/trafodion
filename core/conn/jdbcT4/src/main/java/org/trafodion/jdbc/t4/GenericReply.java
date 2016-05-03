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
import java.util.Locale;

class GenericReply {
	byte[] replyBuffer;

	// ----------------------------------------------------------
	GenericReply(Locale locale, LogicalByteArray buf) throws SQLException {
		Header header = new Header();
		long bufferLength = 0;

		buf.setLocation(0);
		header.extractFromByteArray(buf);
		buf.setLocation(Header.sizeOf());
		bufferLength = header.total_length_;
		replyBuffer = buf.extractByteArray(bufferLength);

	} // end marshal

} // end class GenericReply
