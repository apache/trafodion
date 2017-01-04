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
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.sql.Blob;
import java.sql.SQLException;
import java.util.logging.Level;
import java.util.logging.LogRecord;

public class TrafT4Blob implements Blob {
	private byte[] data_ = null;
	private boolean isFreed_ = false;
	private String lobHandle_ = null;
	private TrafT4Connection connection_ = null;

	public TrafT4Blob(TrafT4Connection connection, String lobHandle) throws SQLException{
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

		this.data_ = reply.lobDataValue;
	}

	public InputStream getBinaryStream() throws SQLException {
		testAvailability();
		return new ByteArrayInputStream(this.data_);
	}

	public InputStream getBinaryStream(long pos, long length) throws SQLException {
		testAvailability();
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(),
				"getBinaryStream()");
		return null;
	}

	/*
	 * Retrieves all or part of the BLOB value that this Blob object represents,
	 * as an array of bytes. This byte array contains up to length consecutive
	 * bytes starting at position pos.
	 *  */
	public byte[] getBytes(long pos, int length) throws SQLException {
		testAvailability();

		byte[] buf = null;
		int startPos = (int)pos - 1;
		int endPos = startPos + length;

		if((startPos > data_.length) || (startPos < 0) || (length < 0))
			throw TrafT4Messages.createSQLException(this.connection_.props_, this.connection_.getLocale(),
					"out_of_lob_bound", null);

		if((endPos - 1) > data_.length) {
			length = data_.length - startPos;
		}
		buf = new byte[length];
		System.arraycopy(data_, startPos, buf, 0, length);

		return buf;
	}

	public int setBytes(long pos, byte[] bytes) throws SQLException {
		testAvailability();

		return setBytes(pos, bytes, 0, bytes.length);
	}

	public int setBytes(long pos, byte[] bytes, int offset, int len) throws SQLException {
		testAvailability();
		OutputStream out = setBinaryStream(pos);

		try {
			out.write(bytes, offset, len);
		} catch (IOException e) {
			throw TrafT4Messages.createSQLException(this.connection_.props_, this.connection_.getLocale(),
					"blob_io_error", e.getMessage());
		} finally {
			try {
				out.close();
			} catch (IOException e2) {

			}
		}
		return len;
	}

	public long position(Blob pattern, long start) throws SQLException {
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "position()");
		return 0;
	}

	public long position(byte[] pattern, long start) throws SQLException {
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(), "position()");
		return 0;
	}

	public OutputStream setBinaryStream(long pos) throws SQLException {
		TrafT4Messages.throwUnsupportedFeatureException(connection_.props_, connection_.getLocale(),
				"setBinaryStream()");
		return null;
	}

	public void truncate(long len) throws SQLException {
		testAvailability();

		if (len < 0 || len > this.data_.length) {
			throw TrafT4Messages.createSQLException(this.connection_.props_, this.connection_.getLocale(),
					"out_of_lob_bound", null);
		}

		byte[] newData = new byte[(int) len];

		System.arraycopy(this.data_, 0, newData, 0, (int) len);

		this.data_ = newData;
	}

	/*
	 * After free has been called, any attempt to invoke a method other than
	 * free will result in a SQLException being thrown.
	 */
	private void testAvailability() throws SQLException {
		if (isFreed_)
			throw TrafT4Messages.createSQLException(null, null, "lob_has_been_freed", null);
	}

	public long length() throws SQLException {
		testAvailability();
		return data_.length;
	}

	public void free() throws SQLException {
		data_ = null;
		isFreed_ = true;
	}
}
