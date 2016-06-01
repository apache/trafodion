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

class SQLValue_def {
	int dataType;
	short dataInd;
	SQL_DataValue_def dataValue;
	int dataCharSet;

	// ----------------------------------------------------------
	int sizeof() {
		return TRANSPORT.size_int * 2 + TRANSPORT.size_short + dataValue.sizeof();
	}

	// ----------------------------------------------------------
	void insertIntoByteArray(LogicalByteArray buf) {
		buf.insertInt(dataType);
		buf.insertShort(dataInd);
		dataValue.insertIntoByteArray(buf);
		buf.insertInt(dataCharSet);
	}

	// ----------------------------------------------------------
	void extractFromByteArray(LogicalByteArray buf) throws SQLException {
		dataType = buf.extractInt();
		dataInd = buf.extractShort();

		dataValue = new SQL_DataValue_def();
		dataValue.extractFromByteArray(buf);

		dataCharSet = buf.extractInt();
	}
}
