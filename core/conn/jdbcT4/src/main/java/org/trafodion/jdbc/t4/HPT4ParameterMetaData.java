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

public class HPT4ParameterMetaData implements java.sql.ParameterMetaData {

	public String getParameterClassName(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}
		return inputDesc[param - 1].getColumnClassName();
	}

	public int getParameterCount() throws SQLException {
		return inputDesc.length;
	}

	public int getParameterMode(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}
		return inputDesc[param - 1].paramMode_;
	}

	public int getParameterType(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}
		return inputDesc[param - 1].dataType_;
	}

	public String getParameterTypeName(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].getColumnTypeName(props.getLocale());
	}

	public int getPrecision(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].precision_;
	}

	public int getScale(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].scale_;
	}

	public int isNullable(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].isNullable_;
	}

	public boolean isSigned(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}
		return inputDesc[param - 1].isSigned_;
	}

	// ////////////////////////
	// begin custom accessors//
	// ////////////////////////
	public int getRowLength() throws SQLException {
		// this is the same for all params
		// only if we have no input params will we throw an error
		if (inputDesc.length == 0) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[0].rowLength_;
	}

	public int getDisplaySize(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].displaySize_;
	}

	public int getFSDataType(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].fsDataType_;
	}

	public int getMaxLength(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].maxLen_;
	}

	public int getNoNullOffset(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].noNullValue_;
	}

	public int getNullOffset(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].nullValue_;
	}

	public int getOdbcCharset(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].odbcCharset_;
	}

	public int getSqlCharset(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].sqlCharset_;
	}

	public int getSqlDataType(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].sqlDataType_;
	}

	public int getSqlDatetimeCode(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].sqlDatetimeCode_;
	}

	public int getSqlOctetLength(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].sqlOctetLength_;
	}

	public int getSqlPrecision(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].sqlPrecision_;
	}

	// /////////////////////////////////
	// these are legacy names...do not remove these yet even though they are
	// duplicate
	// i will depricate these before 2.3 release
	// ///////////////////////////////

	/**
	 * @deprecated
	 */
	public int getSqlTypeCode(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].dataType_;
	}

	/**
	 * @deprecated
	 */
	public int getSqlLength(int param) throws SQLException {
		if (param > inputDesc.length) {
			throw HPT4Messages.createSQLException(props, props.getLocale(), "invalid_desc_index", null);
		}

		return inputDesc[param - 1].maxLen_;
	}

	HPT4ParameterMetaData(TrafT4PreparedStatement stmt, HPT4Desc[] inputDesc) {
		this.props = stmt.connection_.props_;
		this.inputDesc = inputDesc;

		if (props.t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(stmt.connection_.props_, stmt, inputDesc);
			stmt.connection_.props_.t4Logger_.logp(Level.FINE, "HPT4ParameterMetaData", "", "", p);
		}
	}

	T4Properties props;
	HPT4Desc[] inputDesc;
	public Object unwrap(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isWrapperFor(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}
}
