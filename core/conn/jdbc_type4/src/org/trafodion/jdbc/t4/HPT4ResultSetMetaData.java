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
import java.util.logging.Level;

public class HPT4ResultSetMetaData implements java.sql.ResultSetMetaData {

	// begin required methods
	public String getCatalogName(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].catalogName_;
	}

	public String getColumnClassName(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].getColumnClassName();
	}

	public int getColumnCount() throws SQLException {
		return outputDesc_.length;
	}

	public int getColumnDisplaySize(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].displaySize_;
	}

	public String getColumnLabel(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}

		return (outputDesc_[column - 1].columnLabel_ == null) ? outputDesc_[column - 1].name_
				: outputDesc_[column - 1].columnLabel_;
	}

	public String getColumnName(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].name_;
	}

	public int getColumnType(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].dataType_;
	}

	public String getColumnTypeName(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].getColumnTypeName(connection_.getLocale());
	}

	public int getPrecision(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].precision_;
	}

	public int getScale(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].scale_;
	}

	public String getSchemaName(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].schemaName_;
	}

	public String getTableName(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].tableName_;
	}

	public boolean isAutoIncrement(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].isAutoIncrement_;
	}

	public boolean isCaseSensitive(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].isCaseSensitive_;
	}

	public boolean isCurrency(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].isCurrency_;
	}

	public boolean isDefinitelyWritable(int column) throws SQLException {
		return true;
	}

	public int isNullable(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].isNullable_;
	}

	public boolean isReadOnly(int column) throws SQLException {
		return false;
	}

	public boolean isSearchable(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].isSearchable_;
	}

	public boolean isSigned(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].isSigned_;
	}

	public boolean isWritable(int column) throws SQLException {
		return true;
	}

	// ////////////////////////
	// begin custom accessors//
	// ////////////////////////

	public int getFSDataType(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].fsDataType_;
	}

	public int getMaxLength(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].maxLen_;
	}

	public int getOdbcCharset(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].odbcCharset_;
	}

	public int getRowLength() throws SQLException {
		// this is the same for all params
		// only if we have no input params will we throw an error
		if (outputDesc_.length == 0) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.props_.getLocale(),
					"invalid_desc_index", null);
		}

		return outputDesc_[0].rowLength_;
	}

	public int getSqlCharset(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].sqlCharset_;
	}

	public int getSqlPrecision(int column) throws SQLException {
		if (column > outputDesc_.length) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].sqlPrecision_;
	}

	public int getSqlDatetimeCode(int param) throws SQLException {
		return stmt_.ist_.pr_.outputDesc[param - 1].datetimeCode_;
	}

	// /////////////////////////////////
	// these are legacy names...do not remove these yet even though they are
	// duplicate
	// ///////////////////////////////

	/**
	 * @deprecated
	 */
	public String cpqGetCharacterSet(int column) throws SQLException {
		if ((column > outputDesc_.length) || (column <= 0)) {
			throw HPT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_desc_index",
					null);
		}
		return outputDesc_[column - 1].getCharacterSetName();
	}

	/**
	 * @deprecated
	 */
	public int getSqlTypeCode(int param) throws SQLException {
		return stmt_.ist_.pr_.outputDesc[param - 1].dataType_;
	} // end getSqlTypeCode

	/**
	 * @deprecated
	 */
	public int getSqlLength(int param) throws SQLException {
		return stmt_.ist_.pr_.outputDesc[param - 1].maxLen_;
	} // end getSqlTypeCode

	HPT4ResultSetMetaData(TrafT4Statement stmt, HPT4Desc[] outputDesc) {
		if (stmt.connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(stmt.connection_.props_, stmt, outputDesc);
			stmt.connection_.props_.t4Logger_.logp(Level.FINE, "HPT4ResultSetMetaData", "", "", p);
		}

		connection_ = stmt.connection_;
		outputDesc_ = outputDesc;
		stmt_ = stmt;
	}

	HPT4ResultSetMetaData(TrafT4ResultSet resultSet, HPT4Desc[] outputDesc) {
		if (resultSet.connection_.props_.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(resultSet.connection_.props_, resultSet, outputDesc);
			resultSet.connection_.props_.t4Logger_.logp(Level.FINE, "HPT4ResultSetMetaData", "", "", p);
		}

		resultSet_ = resultSet;
		connection_ = resultSet_.connection_;
		outputDesc_ = outputDesc;
		stmt_ = resultSet.stmt_;
	}

	TrafT4ResultSet resultSet_;
	TrafT4Connection connection_;
	HPT4Desc[] outputDesc_;
	TrafT4Statement stmt_;
	public Object unwrap(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isWrapperFor(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}
}
