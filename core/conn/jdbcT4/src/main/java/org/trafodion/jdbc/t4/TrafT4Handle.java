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
import java.sql.SQLWarning;
import java.sql.Statement;

public abstract class TrafT4Handle {
	SQLWarning sqlWarning_;

    // work for situations when allow multi-queries,
    // resultSet.getStatement() should return Statement object that produced this ResultSet object
    protected Statement multiQueriesStmt;

	public void clearWarnings() throws SQLException {
		sqlWarning_ = null;
	}

	public SQLWarning getWarnings() throws SQLException {
		return sqlWarning_;
	}

	void setSQLWarning(T4Properties t4props, String messageId, Object[] messageArguments) {
		SQLWarning sqlWarningLeaf = TrafT4Messages.createSQLWarning(t4props, messageId, messageArguments);
		if (sqlWarning_ == null) {
			sqlWarning_ = sqlWarningLeaf;
		} else {
			sqlWarning_.setNextWarning(sqlWarningLeaf);
		}
	}

	// Method used by JNI layer to set the warning
	void setSqlWarning(SQLWarning sqlWarning) {
		if (sqlWarning_ == null) {
			sqlWarning_ = sqlWarning;
		} else {
			sqlWarning_.setNextWarning(sqlWarning);
		}
	}

	// Method added to check if the connection had any errors
	// This calls the abstract method closeErroredConnection()
	//returns true if a connection error occured
	boolean performConnectionErrorChecks(SQLException se) {
		if (se instanceof TrafT4Exception) {
			TrafT4Exception sqlmx_e = (TrafT4Exception) se;
			if (sqlmx_e.messageId.equals(ERROR_SOCKET_WRITE_ERROR) || sqlmx_e.messageId.equals(ERROR_SOCKET_READ_ERROR)
					|| sqlmx_e.messageId.equals(ERROR_SOCKET_IS_CLOSED_ERROR)
					|| sqlmx_e.messageId.equals(ERROR_INVALID_CONNECTION) || sqlmx_e.messageId.equals(ERROR_IDS_08_S01)
					|| sqlmx_e.messageId.equals(IDS_S1_T00) 
					|| sqlmx_e.messageId.equals(ERROR_SOCKET_OPEN)) {
				closeErroredConnection(sqlmx_e);
				return true;
			}
		}
		
		return false;
	}

	abstract void closeErroredConnection(TrafT4Exception se);

	static final String ERROR_IDS_08_S01 = new String("ids_08_s01");
	static final String ERROR_INVALID_CONNECTION = new String("invalid_connection");
	static final String ERROR_SOCKET_WRITE_ERROR = new String("socket_write_error");
	static final String ERROR_SOCKET_READ_ERROR = new String("socket_read_error");
	static final String ERROR_SOCKET_IS_CLOSED_ERROR = new String("socket_is_closed_error");
	static final String IDS_S1_T00 = new String("ids_s1_t00");
	static final String ERROR_SOCKET_OPEN = new String("socket_open_error");
}
