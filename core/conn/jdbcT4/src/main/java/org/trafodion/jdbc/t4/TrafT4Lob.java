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

import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.sql.SQLException;
import java.sql.Types;
import java.util.logging.Level;
import java.util.logging.LogRecord;

public abstract class TrafT4Lob {

	protected String lobHandle_ = null;
	protected TrafT4Connection connection_ = null;
	protected boolean isFreed_ = true;
	protected int lobType = Types.BLOB;
	protected long length = 0;

	protected Object data_ = null;

	protected TrafT4OutputStream outputStream_ = null;

	public TrafT4Lob(TrafT4Connection connection, String lobHandle, Object data, int type) throws SQLException {
		data_ = data;
		lobType = type;
		this.connection_ = connection;
		this.lobHandle_ = lobHandle;

		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, lobHandle);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getClob", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, connection, lobHandle);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("getClob");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}
		if (lobHandle_ != null)
			read();
		isFreed_ = false;
	}

	public TrafT4Lob(TrafT4Connection connection, byte[] data) throws SQLException {
		this.connection_ = null;
		this.data_ = data;
		this.lobHandle_ = null;
		this.isFreed_ = false;
	}

	public void read() throws SQLException {
		if (this.connection_ == null) {
			throw TrafT4Messages.createSQLException(connection_.props_, null, null, null);
		}
		if (connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, this.connection_, this.lobHandle_);
			connection_.props_.t4Logger_.logp(Level.FINE, "TrafT4ResultSet", "getClob", "", p);
		}
		if ( connection_.props_.t4Logger_.isLoggable(Level.FINE) && connection_.props_.getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(connection_.props_, this.connection_, this.lobHandle_);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ResultSet");
			lr.setSourceMethodName("retrieveFromDB");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			connection_.props_.getLogWriter().println(temp);
		}

		T4Connection t4connection = this.connection_.getServerHandle().getT4Connection();
		LogicalByteArray wbuffer = ExtractLobMessage.marshal(ExtractLobMessage.LOB_EXTRACT_LEN, lobHandle_, 1, 0,
				connection_.ic_);

		LogicalByteArray rbuffer = t4connection.getReadBuffer(TRANSPORT.SRVR_API_EXTRACTLOB, wbuffer);
		ExtractLobReply reply = new ExtractLobReply(rbuffer, connection_.ic_);
		length = reply.lobLength;

		byte[] fetchData_ = new byte[(int) length];

		try {
			int pos = 0;
            int chunkSize = connection_.props_.getLobChunkSize() * 1024 * 1024;
			while (pos < length) {
				int remainSize = (int) (length - pos);
				int fecthSize = remainSize < chunkSize ? remainSize : chunkSize;
				wbuffer =  ExtractLobMessage.marshal(ExtractLobMessage.LOB_EXTRACT_BUFFER, lobHandle_, 1, fecthSize, connection_.ic_);
				rbuffer = t4connection.getReadBuffer(TRANSPORT.SRVR_API_EXTRACTLOB, wbuffer);
				reply = new ExtractLobReply(rbuffer, connection_.ic_);
				System.arraycopy(reply.extractData, 0, fetchData_, pos, (int) reply.extractLen);
				pos += reply.extractLen;
			}

		}
		catch(SQLException se) {
			throw se;
		}
		finally {
			// close the LOB cursor
			wbuffer = ExtractLobMessage.marshal(ExtractLobMessage.LOB_CLOSE_CURSOR, lobHandle_, 1, 0, connection_.ic_);
			rbuffer = t4connection.getReadBuffer(TRANSPORT.SRVR_API_EXTRACTLOB, wbuffer);
			reply = new ExtractLobReply(rbuffer, connection_.ic_);
		}
		switch (lobType) {
		case Types.BLOB:
			data_ = fetchData_;
			break;
		case Types.CLOB:
			try {
			    if (length == 0) {
			        data_ = "";
			    }
			    else {
			        data_ = new String(fetchData_, "UTF-8");
			    }
			} catch (UnsupportedEncodingException e) {
				throw TrafT4Messages.createSQLException(this.connection_.ic_.t4props_, this.connection_.ic_.getLocale(),
						"unsupported_encoding", "UTF-8");
			}
			break;
		}
	}

	void flush() throws SQLException {
		if (this.connection_ == null) {
			throw TrafT4Messages.createSQLException(connection_.props_, null, null, null);
		}

		if (this.lobHandle_ == null)
		    return ;
		T4Connection t4connection = this.connection_.getServerHandle().getT4Connection();

        final long chunkSize = connection_.props_.getLobChunkSize() * 1024 * 1024;
		LogicalByteArray wbuffer = null;

		byte[] valueBuffer = null;

		try {
			switch (lobType) {
			case Types.BLOB:
				valueBuffer = (byte[]) data_;
				break;

			case Types.CLOB:
				if (data_ instanceof String) {
					valueBuffer = ((String) data_).getBytes("UTF-8");
				} else if (data_ instanceof char[]) {
					valueBuffer = String.valueOf(data_).getBytes("UTF-8");
				} else if (data_ instanceof byte[]) {
					valueBuffer = (byte[]) data_;
				}
				break;

			default:
			}
		} catch (Exception e) {
			throw TrafT4Messages.createSQLException(connection_.props_, connection_.ic_.getLocale(),
					"unsupported_encoding_msg", null);
		}

		for (long i = 0; i < valueBuffer.length;) {
			if (valueBuffer.length - i > chunkSize) {
				wbuffer = UpdateLobMessage.marshal((short) 1, lobHandle_, valueBuffer.length, i, valueBuffer, i,
						chunkSize,
						this.connection_.ic_);
				i += chunkSize;
			}
			else {
				wbuffer = UpdateLobMessage.marshal((short) 1, lobHandle_, valueBuffer.length, i, valueBuffer, i,
						valueBuffer.length - i, this.connection_.ic_);
				i = valueBuffer.length;
			}
			LogicalByteArray rbuffer = t4connection.getReadBuffer(TRANSPORT.SRVR_API_UPDATELOB, wbuffer);
			UpdateLobReply ur = new UpdateLobReply(rbuffer, connection_.ic_);

			switch (ur.m_p1.exception_nr) {
			case TRANSPORT.CEE_SUCCESS:
				if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
					Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
					connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4Lob", "flush",
							"ur_.m_p1.exception_nr = TRANSPORT.CEE_SUCCESS", p);
				}

				break;
			default:
				if (connection_.props_.t4Logger_.isLoggable(Level.FINER) == true) {
					Object p[] = T4LoggingUtilities.makeParams(connection_.props_);
					connection_.props_.t4Logger_.logp(Level.FINER, "TrafT4Lob", "flush",
							"case ur.m_p1.exception_nr deafult", p);
				}
				throw TrafT4Messages.createSQLException(connection_.props_, connection_.ic_.getLocale(),
						"lob_io_error", null);
			}

		}
	}

	protected OutputStream setOutputStream(long pos) throws SQLException {
		outputStream_ = new TrafT4OutputStream(this, pos);
		return outputStream_;
	}
}
