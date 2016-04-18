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

import java.io.Serializable;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.util.BitSet;

class InsertRow extends BaseRow implements Serializable, Cloneable {

	private BitSet colsInserted;
	private int cols;

	InsertRow(int i) {
		origVals = new Object[i];
		colsInserted = new BitSet(i);
		cols = i;
	}

	protected Object getColumnObject(int i) throws SQLException {
		if (!colsInserted.get(i - 1)) {
			throw HPT4Messages.createSQLException(null, null, "no_column_value_specified", null);
		} else {
			return origVals[i - 1];
		}
	}

	protected void initInsertRow() {
		for (int i = 0; i < cols; i++) {
			colsInserted.clear(i);

		}
	}

	/*
	 * protected boolean isCompleteRow(RowSetMetaData rowsetmetadata) throws
	 * SQLException { for(int i = 0; i < cols; i++) if(!colsInserted.get(i) &&
	 * rowsetmetadata.isNullable(i + 1) == 0) return false; return true; }
	 */

	protected void markColInserted(int i) {
		colsInserted.set(i);
	}

	protected void setColumnObject(int i, Object obj) {
		origVals[i - 1] = obj;
		markColInserted(i - 1);
	}

	protected void insertRow(PreparedStatement insertStmt, BitSet paramCols) throws SQLException {
		int i;
		int j;

		for (i = 0, j = 1; i < cols; i++) {
			if (paramCols.get(i)) {
				insertStmt.setObject(j++, origVals[i]);
			}
		}
		insertStmt.execute();
		initInsertRow();
	}
}
