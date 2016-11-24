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

import java.io.InputStream;
import java.io.OutputStream;
import java.io.Reader;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.io.Writer;
import java.sql.Clob;
import java.sql.SQLException;
import java.util.logging.Level;
import java.util.logging.LogRecord;

public class TrafT4Clob implements Clob {

	private String data_ = null;
	private boolean isFreed_ = false;
	private String lobHandle_ = null;
	private TrafT4Connection connection_ = null;

	public TrafT4Clob(TrafT4Connection connection, String lobHandle) throws SQLException {
		if (connection == null) {
			throw TrafT4Messages.createSQLException(connection_.props_, null, null, null);
		}

		this.connection_ = connection;
		this.lobHandle_ = lobHandle;
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, lobHandle);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getClob", "", p);
		}
		if (connection_.props_.getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, lobHandle);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getClob");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		T4Connection t4connection = this.connection_.getServerHandle().getT4Connection();
		LogicalByteArray wbuffer = ExtractLobMessage.marshal(ExtractLobMessage.LOB_EXTRACT_BUFFER, lobHandle_, 1, 0,
				connection_.ic_);
		LogicalByteArray rbuffer = t4connection.getReadBuffer(TRANSPORT.SRVR_API_EXTRACTLOB, wbuffer);
		ExtractLobReply reply = new ExtractLobReply(rbuffer, connection_.ic_);

		try {
			this.data_ = new String(reply.lobDataValue, "UTF-8");
		} catch (UnsupportedEncodingException e) {
			throw TrafT4Messages.createSQLException(this.connection_.ic_.t4props_, this.connection_.ic_.getLocale(),
					"unsupported_encoding", "UTF-8");
		}
	}
	public void free() throws SQLException {
		data_ = null;
		isFreed_ = true;
	}

	public InputStream getAsciiStream() throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Reader getCharacterStream() throws SQLException {
		testAvailability();

		if (this.data_ != null) {
			return new StringReader(this.data_);
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

		if (endIndex > this.data_.length()) {
			throw TrafT4Messages.createSQLException(this.connection_.props_, this.connection_.getLocale(), null, null);
		}

		return this.data_.substring(beginIndex, endIndex);
	}

	public long length() throws SQLException {
		testAvailability();
		if (data_ != null) {
			return data_.length();
		}

		return 0;
	}

	public long position(String searchstr, long start) throws SQLException {
		start--;

		if (start <= 0 || start > this.data_.length()) {
			throw TrafT4Messages.createSQLException(this.connection_.props_, null, null, null);
		}
		int pos = this.data_.indexOf(searchstr, (int) start);

		if (pos == -1) {
			return -1;
		}

		return pos - 1;
	}

	public long position(Clob searchstr, long start) throws SQLException {
		return position(searchstr.getSubString(1L, (int) searchstr.length()), start);
	}

	public OutputStream setAsciiStream(long pos) throws SQLException {
		TrafT4Messages.throwUnsupportedFeatureException(this.connection_.props_, this.connection_.getLocale(),
				"setAsciiStream()");
		return null;
	}

	public Writer setCharacterStream(long pos) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public int setString(long pos, String str) throws SQLException {
		int startIndex = (int) pos - 1;
		if (startIndex < 0) {
			throw TrafT4Messages.createSQLException(this.connection_.props_, this.connection_.getLocale(),
					"out_of_lob_bound_msg", null);
		}
		if (str == null) {
			throw TrafT4Messages.createSQLException(this.connection_.props_, this.connection_.getLocale(), "clob_null_string_para", null);
		}

		StringBuilder buf = new StringBuilder(this.data_);

		int len = str.length();

		buf.replace(startIndex, startIndex + len, str);
		this.data_ = buf.toString();
		return len;
	}

	public int setString(long pos, String str, int offset, int len) throws SQLException {
		int start = (int) pos - 1;
		String replacestr = str.substring(offset, offset + len);
		StringBuilder buf = new StringBuilder(this.data_);

		buf.replace(start, start + replacestr.length(), replacestr);

		this.data_ = buf.toString();

		return len;
	}

	public void truncate(long len) throws SQLException {
		if (len > this.data_.length()) {
			throw TrafT4Messages.createSQLException(connection_.props_, null, null, null);
		}
		this.data_ = this.data_.substring(0, (int) len);
	}

	private void testAvailability() throws SQLException {
		if (isFreed_)
			throw TrafT4Messages.createSQLException(null, null, "lob_has_been_freed", null);
	}

}
