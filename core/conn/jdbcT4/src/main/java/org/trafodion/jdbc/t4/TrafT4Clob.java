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

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.Reader;
import java.io.StringReader;
import java.io.Writer;
import java.sql.Clob;
import java.sql.SQLException;
import java.sql.Types;

public class TrafT4Clob extends TrafT4Lob implements Clob {
	private boolean isFreed_ = false;
	private TrafT4Connection connection_ = null;

	public TrafT4Clob(TrafT4Connection connection, String lobHandle, String data) throws SQLException {
		super(connection, lobHandle, data, Types.CLOB);

		if (this.data_ == null)
			this.data_ = "";
	}

	public void free() throws SQLException {
		data_ = null;
		isFreed_ = true;
	}

	public InputStream getAsciiStream() throws SQLException {
		testAvailability();
		if (data_ != null) {
			return new ByteArrayInputStream(((String) data_).getBytes());
		}

		return null;
	}

	public Reader getCharacterStream() throws SQLException {
		testAvailability();

		if (this.data_ != null) {
			return new StringReader((String) data_);
		}

		return null;
	}

	public Reader getCharacterStream(long pos, long length) throws SQLException {
		testAvailability();

		return new StringReader(getSubString(pos, (int) length));
	}

	public String getSubString(long pos, int length) throws SQLException {
		testAvailability();

		int beginIndex = (int) pos - 1;
		int endIndex = beginIndex + length;

		if (this.data_ == null) {
			return null;
		}

		if (endIndex > ((String) data_).length()) {
			throw TrafT4Messages.createSQLException(this.connection_.props_, this.connection_.getLocale(), null, null);
		}

		return ((String) data_).substring(beginIndex, endIndex);
	}

	public long length() throws SQLException {
		testAvailability();
		if (data_ != null) {
			return ((String) data_).length();
		}

		return 0;
	}

	public long position(String searchstr, long start) throws SQLException {
		testAvailability();
		//start--;

		if (start <= 0 || start > ((String) data_).length()) {
			throw TrafT4Messages.createSQLException(this.connection_.props_, null, null, null);
		}
		start--;

		int pos = ((String) data_).indexOf(searchstr, (int) start);

		if (pos == -1) {
			return -1;
		}

		return pos;
	}

	public long position(Clob searchstr, long start) throws SQLException {
		return position(searchstr.getSubString(1L, (int) searchstr.length()), start);
	}

	public OutputStream setAsciiStream(long pos) throws SQLException {
		testAvailability();
		if (pos < 1) {
			throw TrafT4Messages.createSQLException(this.connection_.props_, this.connection_.getLocale(),
					"out_of_lob_bound_msg", null);
		}

		return setOutputStream(pos);
	}

	public Writer setCharacterStream(long pos) throws SQLException {
		testAvailability();
		TrafT4Writer writer = new TrafT4Writer(this, pos);
		return writer;
	}

	public int setString(long pos, String str) throws SQLException {
		testAvailability();
		int startIndex = (int) pos - 1;
		if (startIndex < 0) {
			throw TrafT4Messages.createSQLException(this.connection_.props_, this.connection_.getLocale(),
					"out_of_lob_bound_msg", null);
		}
		if (str == null) {
			throw TrafT4Messages.createSQLException(this.connection_.props_, this.connection_.getLocale(), "clob_null_string_para", null);
		}

		StringBuilder buf = new StringBuilder(((String) data_));

		int len = str.length();

		buf.replace(startIndex, startIndex + len, str);
		this.data_ = buf.toString();
		this.flush();
		return len;
	}

	public int setString(long pos, String str, int offset, int len) throws SQLException {
		testAvailability();
		int start = (int) pos - 1;
		String replacestr = str.substring(offset, offset + len);
		StringBuilder buf = new StringBuilder(((String) data_));

		buf.replace(start, start + replacestr.length(), replacestr);

		this.data_ = buf.toString();

		this.flush();
		return len;
	}

	public void truncate(long len) throws SQLException {
		testAvailability();
		if (len > ((String) data_).length()) {
			throw TrafT4Messages.createSQLException(connection_.props_, null, null, null);
		}
		this.data_ = ((String) data_).substring(0, (int) len);
		this.flush();
	}

	private void testAvailability() throws SQLException {
		if (isFreed_)
			throw TrafT4Messages.createSQLException(null, null, "lob_has_been_freed", null);
	}

}
