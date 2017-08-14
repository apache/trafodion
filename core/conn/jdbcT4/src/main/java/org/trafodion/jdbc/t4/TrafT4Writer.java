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

import java.io.CharArrayWriter;
import java.io.IOException;
import java.sql.SQLException;

public class TrafT4Writer extends CharArrayWriter {
	private boolean isClosed_ = true;
	private TrafT4Lob lob_;

	TrafT4Writer(TrafT4Lob lob, long pos) throws SQLException {
	    this.lob_ = lob;
	}

	private void testAvailability() throws IOException {
		if (isClosed_) {
			throw new IOException("Output stream is closed");
		}
	}

	@Override
	public void close() {
		lob_.data_ = new String(this.toCharArray());
		try {
			if (lob_.connection_ != null) {
				lob_.flush();
			}
		} catch (SQLException e) {
				e.printStackTrace();
		}
	}
}
