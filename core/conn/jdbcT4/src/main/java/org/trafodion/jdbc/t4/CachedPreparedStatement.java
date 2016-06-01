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

import java.sql.PreparedStatement;
import java.sql.SQLException;

public class CachedPreparedStatement {

	PreparedStatement getPreparedStatement() {
		inUse_ = true;
		return pstmt_;
	}

	void setLastUsedInfo() {
		lastUsedTime_ = System.currentTimeMillis();
		noOfTimesUsed_++;
	}

	long getLastUsedTime() {
		return lastUsedTime_;
	}

	String getLookUpKey() {
		return key_;
	}

	void close(boolean hardClose) throws SQLException {
		inUse_ = false;
		pstmt_.close(hardClose);
	}

	CachedPreparedStatement(PreparedStatement pstmt, String key) {
		pstmt_ = (TrafT4PreparedStatement) pstmt;
		key_ = key;
		creationTime_ = System.currentTimeMillis();
		lastUsedTime_ = creationTime_;
		noOfTimesUsed_ = 1;
		inUse_ = true;
	}

	private TrafT4PreparedStatement pstmt_;
	private String key_;
	private long lastUsedTime_;
	private long creationTime_;
	private long noOfTimesUsed_;
	boolean inUse_;
}
