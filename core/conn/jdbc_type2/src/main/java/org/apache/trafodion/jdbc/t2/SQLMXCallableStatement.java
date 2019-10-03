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

/* -*-java-*-
 * Filename    : SQLMXCallableStatement.java
 * Description :
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.math.BigDecimal;
import java.util.Calendar;
import java.util.Map;
import java.net.URL;
import java.io.InputStream;
import java.io.Reader;

public class SQLMXCallableStatement extends SQLMXPreparedStatement implements
java.sql.CallableStatement {
public Array getArray(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getArray_I].methodEntry();
        try {
            clearWarnings();
            Messages.throwUnsupportedFeatureException(connection_.locale_,
                    "getArray()");
            return null;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getArray_I].methodExit();
        }
    }

public Array getArray(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getArray_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getArray(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getArray_L].methodExit();
        }
    }

public BigDecimal getBigDecimal(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getBigDecimal_I].methodEntry();
        try {
            BigDecimal retValue;
            String data;

            validateGetInvocation(parameterIndex);
            inputDesc_[parameterIndex - 1]
            .checkValidNumericConversion(connection_.locale_);
            // String returned may not be numeric in case of SQL_CHAR,
            // SQL_VARCHAR and SQL_LONGVARCHAR
            // fields. Hoping that java might throw invalid value exception
            data = getString(parameterIndex);
            if (data == null) {
                wasNull_ = true;
                return null;
            } else {
                wasNull_ = false;
                try {
                    retValue = new BigDecimal(data);
                } catch (NumberFormatException e) {
                    throw Messages.createSQLException(connection_.locale_,
                            "invalid_cast_specification", null);
                }
                return retValue;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getBigDecimal_I].methodExit();
        }
    }

    @Deprecated
public BigDecimal getBigDecimal(int parameterIndex, int scale)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getBigDecimal_II].methodEntry();
        try {
            BigDecimal retValue;

            retValue = getBigDecimal(parameterIndex);
            if (retValue != null)
            return retValue.setScale(scale);
            else
            return null;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getBigDecimal_II].methodExit();
        }
    }

public BigDecimal getBigDecimal(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getBigDecimal_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getBigDecimal(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getBigDecimal_L].methodExit();
        }
    }

    @Deprecated
public BigDecimal getBigDecimal(String parameterName, int scale)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getBigDecimal_LI].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getBigDecimal(parameterIndex, scale);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getBigDecimal_LI].methodExit();
        }
    }

public Blob getBlob(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getBlob_I].methodEntry();
        try {
            Messages.throwUnsupportedFeatureException(connection_.locale_,
                    "getBlob()");
            return null;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getBlob_I].methodExit();
        }
    }

public Blob getBlob(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getBlob_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getBlob(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getBlob_L].methodExit();
        }
    }

public boolean getBoolean(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getBoolean_I].methodEntry();
        try {
            String data;

            validateGetInvocation(parameterIndex);
            inputDesc_[parameterIndex - 1]
            .checkValidNumericConversion(connection_.locale_);
            data = getString(parameterIndex);
            if (data != null) {
                wasNull_ = false;
                return (!data.equals("0"));
            } else {
                wasNull_ = true;
                return false;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getBoolean_I].methodExit();
        }
    }

public boolean getBoolean(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getBoolean_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getBoolean(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getBoolean_L].methodExit();
        }
    }

public byte getByte(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getByte_I].methodEntry();
        try {
            String data;

            validateGetInvocation(parameterIndex);
            inputDesc_[parameterIndex - 1]
            .checkValidNumericConversion(connection_.locale_);
            data = getString(parameterIndex);
            if (data != null) {
                wasNull_ = false;
                return Byte.parseByte(data);
            } else {
                wasNull_ = true;
                return 0;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getByte_I].methodExit();
        }
    }

public byte getByte(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getByte_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getByte(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getByte_L].methodExit();
        }
    }

public byte[] getBytes(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getBytes_I].methodEntry();
        try {
            validateGetInvocation(parameterIndex);
            // BINARY, VARBINARY, LONGVARBINARY not supported
            throw Messages.createSQLException(connection_.locale_,
                    "datatype_not_supported", null);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getBytes_I].methodExit();
        }

    }

public byte[] getBytes(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getBytes_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getBytes(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getBytes_L].methodExit();
        }
    }

public Clob getClob(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getClob_I].methodEntry();
        try {
            Messages.throwUnsupportedFeatureException(connection_.locale_,
                    "getClob()");
            return null;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getClob_I].methodExit();
        }
    }

public Clob getClob(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getClob_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getClob(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getClob_L].methodExit();
        }
    }

public Date getDate(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getDate_I].methodEntry();
        try {
            int dataType;
            String dateStr;
            Date retValue;

            validateGetInvocation(parameterIndex);
            dataType = inputDesc_[parameterIndex - 1].dataType_;
            if (dataType != Types.CHAR && dataType != Types.VARCHAR
                    && dataType != Types.LONGVARCHAR && dataType != Types.DATE
                    && dataType != Types.TIMESTAMP)
            throw Messages.createSQLException(connection_.locale_,
                    "restricted_data_type", null);
            dateStr = getString(parameterIndex);
            if (dateStr != null) {
                wasNull_ = false;
                try {
                    retValue = Date.valueOf(dateStr);
                } catch (IllegalArgumentException e) {
                    throw Messages.createSQLException(connection_.locale_,
                            "invalid_cast_specification", null);
                }
                return retValue;
            } else {
                wasNull_ = true;
                return null;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getDate_I].methodExit();
        }
    }

public Date getDate(int parameterIndex, Calendar cal) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getDate_IL].methodEntry();
        try {
            Date sqlDate;
            java.util.Date d;

            sqlDate = getDate(parameterIndex);
            if (sqlDate != null) {
                if (cal != null) {
                    cal.setTime(sqlDate);
                    d = cal.getTime();
                    sqlDate = new Date(d.getTime());
                }
                return sqlDate;
            } else
            return (sqlDate);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getDate_IL].methodExit();
        }
    }

public Date getDate(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getDate_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getDate(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getDate_L].methodExit();
        }
    }

public Date getDate(String parameterName, Calendar cal) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getDate_LL].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getDate(parameterIndex, cal);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getDate_LL].methodExit();
        }
    }

    //**************************************************************************
    // **
    // *
    // * METHOD: getDouble
    // *
    // * DESCRIPTION: This routine is called directly or by getDouble(string) to
    // return
    // * the double floating point value from the resultset. All of the
    // * result set data values are stored as strings, and have to be converted
    // * from a string to a double floating point value.
    // *
    // * INPUT: columIndex
    // *
    // * RETURN: double - a double floating point value, which is the result of
    // the
    // * string to floating point conversion.
    // *
    //**************************************************************************
    // **
public double getDouble(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getDouble_I].methodEntry();
        try {
            String data;

            validateGetInvocation(parameterIndex);
            inputDesc_[parameterIndex - 1]
            .checkValidNumericConversion(connection_.locale_);
            data = getString(parameterIndex);
            if (data != null) {
                try {
                    wasNull_ = false;
                    return Double.parseDouble(data);
                } catch (NumberFormatException e1) {
                    throw Messages.createSQLException(connection_.locale_,
                            "invalid_cast_specification", null);
                }
            } else {
                wasNull_ = true;
                return 0;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getDouble_I].methodExit();
        }
    }

public double getDouble(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getDouble_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getDouble(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getDouble_L].methodExit();
        }
    }

    //**************************************************************************
    // **
    // *
    // * METHOD: getFloat
    // *
    // * DESCRIPTION: This routine is called directly or by getFloat(string) to
    // * return the floating point value from the resultset. All of the
    // * result set data values are stored as strings, and have to be
    // * converted from a string to a floating point value.
    // *
    // * INPUT: columIndex
    // *
    // * RETURN: float - a 32-bit floating point value, which is the result of
    // the
    // * string to floating point conversion.
    // *
    //**************************************************************************
    // **
public float getFloat(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getFloat_I].methodEntry();
        try {
            String data;

            validateGetInvocation(parameterIndex);
            inputDesc_[parameterIndex - 1]
            .checkValidNumericConversion(connection_.locale_);
            data = getString(parameterIndex);
            if (data != null) {
                try {
                    wasNull_ = false;
                    return Float.parseFloat(data);
                } catch (NumberFormatException e1) {
                    throw Messages.createSQLException(connection_.locale_,
                            "invalid_cast_specification", null);
                }
            } else {
                wasNull_ = true;
                return 0;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getFloat_I].methodExit();
        }
    }

public float getFloat(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getFloat_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getFloat(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getFloat_L].methodExit();
        }
    }

public int getInt(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getInt_I].methodEntry();
        try {
            String data;

            validateGetInvocation(parameterIndex);
            inputDesc_[parameterIndex - 1]
            .checkValidNumericConversion(connection_.locale_);
            data = getString(parameterIndex);
            if (data != null) {
                wasNull_ = false;
                return Integer.parseInt(data);
            } else {
                wasNull_ = true;
                return 0;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getInt_I].methodExit();
        }
    }

public int getInt(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getInt_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getInt(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getInt_L].methodExit();
        }
    }

public long getLong(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getLong_I].methodEntry();
        try {
            String data;

            validateGetInvocation(parameterIndex);
            inputDesc_[parameterIndex - 1]
            .checkValidNumericConversion(connection_.locale_);
            data = getString(parameterIndex);
            if (data != null) {
                wasNull_ = false;
                return Long.parseLong(data);
            } else {
                wasNull_ = true;
                return 0;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getLong_I].methodExit();
        }
    }

public long getLong(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getLong_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getLong(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getLong_L].methodExit();
        }
    }

public Object getObject(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getObject_I].methodEntry();
        try {
            int dataType;
            byte byteValue;
            short shortValue;
            int intValue;
            long longValue;
            float floatValue;
            double doubleValue;
            boolean booleanValue;

            validateGetInvocation(parameterIndex);
            dataType = inputDesc_[parameterIndex - 1].dataType_;
            switch (dataType) {
                case Types.TINYINT:
                intValue = getByte(parameterIndex);
                if (wasNull_)
                return null;
                else
                return new Integer(intValue);
                case Types.SMALLINT:
                intValue = getShort(parameterIndex);
                if (wasNull_)
                return null;
                else
                return new Integer(intValue);
                case Types.INTEGER:
                intValue = getInt(parameterIndex);
                if (wasNull_)
                return null;
                else
                return new Integer(intValue);
                case Types.BIGINT:
                longValue = getLong(parameterIndex);
                if (wasNull_)
                return null;
                else
                return new Long(longValue);
                case Types.REAL:
                floatValue = getFloat(parameterIndex);
                if (wasNull_)
                return null;
                else
                return new Float(floatValue);
                case Types.FLOAT:
                case Types.DOUBLE:
                doubleValue = getDouble(parameterIndex);
                if (wasNull_)
                return null;
                else
                return new Double(doubleValue);
                case Types.DECIMAL:
                case Types.NUMERIC:
                return getBigDecimal(parameterIndex);
                case Types.BIT:
                booleanValue = getBoolean(parameterIndex);
                if (wasNull_)
                return null;
                else
                return new Boolean(booleanValue);
                case Types.CHAR:
                case Types.VARCHAR:
                case Types.LONGVARCHAR:
                return getString(parameterIndex);
                case Types.BINARY:
                case Types.VARBINARY:
                case Types.LONGVARBINARY:
                return getBytes(parameterIndex);
                case Types.DATE:
                return getDate(parameterIndex);
                case Types.TIME:
                return getTime(parameterIndex);
                case Types.TIMESTAMP:
                return getTimestamp(parameterIndex);
                default:
                throw Messages.createSQLException(connection_.locale_,
                        "restricted_data_type", null);
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getObject_I].methodExit();
        }
    }

public Object getObject(int parameterIndex, Map map) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getObject_IL].methodEntry();
        try {
            Messages.throwUnsupportedFeatureException(connection_.locale_,
                    "getObject()");
            return null;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getObject_IL].methodExit();
        }
    }

public Object getObject(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getObject_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getObject(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getObject_L].methodExit();
        }
    }

public Object getObject(String parameterName, Map map) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getObject_LL].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getObject(parameterIndex, map);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getObject_LL].methodExit();
        }
    }

public Ref getRef(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getRef_I].methodEntry();
        try {
            Messages.throwUnsupportedFeatureException(connection_.locale_,
                    "getRef()");
            return null;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getRef_I].methodExit();
        }
    }

public Ref getRef(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getRef_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getRef(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getRef_L].methodExit();
        }
    }

public short getShort(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getShort_I].methodEntry();
        try {
            String data;

            validateGetInvocation(parameterIndex);
            inputDesc_[parameterIndex - 1]
            .checkValidNumericConversion(connection_.locale_);
            data = getString(parameterIndex);
            if (data != null) {
                wasNull_ = false;
                return Short.parseShort(data);
            } else {
                wasNull_ = true;
                return 0;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getShort_I].methodExit();
        }
    }

public short getShort(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getShort_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getShort(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getShort_L].methodExit();
        }
    }

public String getString(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getString_I].methodEntry();
        try {
            validateGetInvocation(parameterIndex);
            if ((paramValues_ == null) || paramValues_.isNull(parameterIndex)) {
                wasNull_ = true;
                return (null);
            }

            wasNull_ = false;
            return paramValues_.getString(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getString_I].methodExit();
        }
    }

public String getString(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getString_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getString(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getString_L].methodExit();
        }
    }

public Time getTime(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getTime_I].methodEntry();
        try {
            int dataType;
            String timeStr;
            Time retValue;

            validateGetInvocation(parameterIndex);
            dataType = inputDesc_[parameterIndex - 1].dataType_;
            if (dataType != Types.CHAR && dataType != Types.VARCHAR
                    && dataType != Types.LONGVARCHAR && dataType != Types.TIME
                    && dataType != Types.TIMESTAMP)
            throw Messages.createSQLException(connection_.locale_,
                    "restricted_data_type", null);
            timeStr = getString(parameterIndex);
            if (timeStr != null) {
                try {
                    wasNull_ = false;
                    retValue = Time.valueOf(timeStr);
                } catch (IllegalArgumentException e) {
                    throw Messages.createSQLException(connection_.locale_,
                            "invalid_cast_specification", null);
                }
                return retValue;
            } else {
                wasNull_ = true;
                return null;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getTime_I].methodExit();
        }
    }

public Time getTime(int parameterIndex, Calendar cal) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getTime_IL].methodEntry();
        try {
            Time sqlTime;
            java.util.Date d;

            sqlTime = getTime(parameterIndex);
            if (sqlTime != null) {
                if (cal != null) {
                    cal.setTime(sqlTime);
                    d = cal.getTime();
                    sqlTime = new Time(d.getTime());
                }
                return sqlTime;
            } else
            return (sqlTime);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getTime_IL].methodExit();
        }
    }

public Time getTime(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getTime_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getTime(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getTime_L].methodExit();
        }
    }

public Time getTime(String parameterName, Calendar cal) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getTime_LL].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getTime(parameterIndex, cal);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getTime_LL].methodExit();
        }
    }

public Timestamp getTimestamp(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getTimestamp_I].methodEntry();
        try {
            int dataType;
            String timestampStr;
            Timestamp retValue;

            validateGetInvocation(parameterIndex);
            dataType = inputDesc_[parameterIndex - 1].dataType_;
            if (dataType != Types.CHAR && dataType != Types.VARCHAR
                    && dataType != Types.LONGVARCHAR && dataType != Types.DATE
                    && dataType != Types.TIMESTAMP)
            throw Messages.createSQLException(connection_.locale_,
                    "restricted_data_type", null);
            timestampStr = getString(parameterIndex);
            if (timestampStr != null) {
                try {
                    wasNull_ = false;
                    retValue = Timestamp.valueOf(timestampStr);
                } catch (IllegalArgumentException e) {
                    throw Messages.createSQLException(connection_.locale_,
                            "invalid_cast_specification", null);
                }
                return retValue;
            } else {
                wasNull_ = true;
                return null;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getTimestamp_I].methodExit();
        }
    }

public Timestamp getTimestamp(int parameterIndex, Calendar cal)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getTimestamp_IL].methodEntry();
        try {
            Timestamp sqlTimestamp;
            java.util.Date d;
            int nanos;

            sqlTimestamp = getTimestamp(parameterIndex);
            if (sqlTimestamp != null) {
                if (cal != null) {
                    nanos = sqlTimestamp.getNanos();
                    cal.setTime(sqlTimestamp);
                    d = cal.getTime();
                    sqlTimestamp = new Timestamp(d.getTime());
                    sqlTimestamp.setNanos(nanos);
                }
                return sqlTimestamp;
            } else
            return (sqlTimestamp);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getTimestamp_IL].methodExit();
        }
    }

public Timestamp getTimestamp(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getTimestamp_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getTimestamp(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getTimestamp_L].methodExit();
        }
    }

public Timestamp getTimestamp(String parameterName, Calendar cal)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getTimestamp_LL].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getTimestamp(parameterIndex, cal);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getTimestamp_LL].methodExit();
        }
    }

public URL getURL(int parameterIndex) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getURL_I].methodEntry();
        try {
            clearWarnings();
            Messages.throwUnsupportedFeatureException(connection_.locale_,
                    "getURL()");
            return null;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getURL_I].methodExit();
        }
    }

public URL getURL(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getURL_L].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            return getURL(parameterIndex);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getURL_L].methodExit();
        }
    }

public void registerOutParameter(int parameterIndex, int sqlType)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_registerOutParameter_II].methodEntry();
        try {
            // Ignoring sqlType and scale
            validateGetInvocation(parameterIndex);
            if (inputDesc_[parameterIndex - 1].paramMode_ == DatabaseMetaData.procedureColumnOut)
            inputDesc_[parameterIndex - 1].isValueSet_ = true;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_registerOutParameter_II].methodExit();
        }
    }

public void registerOutParameter(int parameterIndex, int sqlType, int scale)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_registerOutParameter_III].methodEntry();
        try {
            // Ignoring sqlType and scale
            validateGetInvocation(parameterIndex);
            if (inputDesc_[parameterIndex - 1].paramMode_ == DatabaseMetaData.procedureColumnOut)
            inputDesc_[parameterIndex - 1].isValueSet_ = true;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_registerOutParameter_III].methodExit();
        }
    }

public void registerOutParameter(int parameterIndex, int sqlType,
            String typeName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_registerOutParameter_IIL].methodEntry();
        try {
            // Ignoring sqlType and typeName
            validateGetInvocation(parameterIndex);
            if (inputDesc_[parameterIndex - 1].paramMode_ == DatabaseMetaData.procedureColumnOut)
            inputDesc_[parameterIndex - 1].isValueSet_ = true;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_registerOutParameter_IIL].methodExit();
        }
    }

public void registerOutParameter(String parameterName, int sqlType)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_registerOutParameter_LI].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            registerOutParameter(parameterIndex, sqlType);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_registerOutParameter_LI].methodExit();
        }
    }

public void registerOutParameter(String parameterName, int sqlType,
            int scale) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_registerOutParameter_LII].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            registerOutParameter(parameterIndex, sqlType, scale);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_registerOutParameter_LII].methodExit();
        }
    }

public void registerOutParameter(String parameterName, int sqlType,
            String typeName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_registerOutParameter_LIL].methodEntry();
        try {
            int parameterIndex = validateGetInvocation(parameterName);
            registerOutParameter(parameterIndex, sqlType, typeName);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_registerOutParameter_LIL].methodExit();
        }
    }

public void setAsciiStream(String parameterName, InputStream x, int length)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setAsciiStream].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setAsciiStream(parameterIndex, x, length);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setAsciiStream].methodExit();
        }
    }

public void setBigDecimal(String parameterName, BigDecimal x)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setBigDecimal].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setBigDecimal(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setBigDecimal].methodExit();
        }
    }

public void setBinaryStream(String parameterName, InputStream x, int length)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setBinaryStream].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setBinaryStream(parameterIndex, x, length);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setBinaryStream].methodExit();
        }
    }

public void setBoolean(String parameterName, boolean x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setBoolean].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setBoolean(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setBoolean].methodExit();
        }
    }

public void setByte(String parameterName, byte x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setByte].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setByte(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setByte].methodExit();
        }
    }

public void setBytes(String parameterName, byte[] x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setBytes].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setBytes(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setBytes].methodExit();
        }
    }

public void setCharacterStream(String parameterName, Reader reader,
            int length) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setCharacterStream].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setCharacterStream(parameterIndex, reader, length);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setCharacterStream].methodExit();
        }
    }

public void setDate(String parameterName, Date x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setDate_LL].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setDate(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setDate_LL].methodExit();
        }
    }

public void setDate(String parameterName, Date x, Calendar cal)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setDate_LLL].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setDate(parameterIndex, x, cal);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setDate_LLL].methodExit();
        }
    }

public void setDouble(String parameterName, double x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setDouble].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setDouble(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setDouble].methodExit();
        }
    }

public void setFloat(String parameterName, float x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setFloat].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setFloat(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setFloat].methodExit();
        }
    }

public void setInt(String parameterName, int x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setInt].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setInt(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setInt].methodExit();
        }
    }

public void setLong(String parameterName, long x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setLong].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setLong(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setLong].methodExit();
        }
    }

public void setNull(String parameterName, int sqlType) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setNull_LI].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setNull(parameterIndex, sqlType);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setNull_LI].methodExit();
        }
    }

public void setNull(String parameterName, int sqlType, String typeName)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setNull_LIL].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setNull(parameterIndex, sqlType, typeName);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setNull_LIL].methodExit();
        }
    }

public void setObject(String parameterName, Object x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setObject_LL].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setObject(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setObject_LL].methodExit();
        }
    }

public void setObject(String parameterName, Object x, int targetSqlType)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setObject_LLI].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setObject(parameterIndex, x, targetSqlType);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setObject_LLI].methodExit();
        }
    }

public void setObject(String parameterName, Object x, int targetSqlType,
            int scale) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setObject_LLII].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setObject(parameterIndex, x, targetSqlType, scale);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setObject_LLII].methodExit();
        }
    }

public void setShort(String parameterName, short x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setShort].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setShort(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setShort].methodExit();
        }
    }

public void setString(String parameterName, String x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setString].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setString(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setString].methodExit();
        }
    }

public void setTime(String parameterName, Time x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setTime_LL].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setTime(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setTime_LL].methodExit();
        }
    }

public void setTime(String parameterName, Time x, Calendar cal)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setTime_LLL].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setTime(parameterIndex, x, cal);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setTime_LLL].methodExit();
        }
    }

public void setTimestamp(String parameterName, Timestamp x)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setTimestamp_LL].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setTimestamp(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setTimestamp_LL].methodExit();
        }
    }

public void setTimestamp(String parameterName, Timestamp x, Calendar cal)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setTimestamp_LLL].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setTimestamp(parameterIndex, x, cal);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setTimestamp_LLL].methodExit();
        }
    }

    @Deprecated
public void setUnicodeStream(String parameterName, InputStream x, int length)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setUnicodeStream].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setUnicodeStream(parameterIndex, x, length);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setUnicodeStream].methodExit();
        }
    }

public void setURL(String parameterName, URL x) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setURL].methodEntry();
        try {
            int parameterIndex = validateSetInvocation(parameterName);
            setURL(parameterIndex, x);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setURL].methodExit();
        }
    }

public boolean wasNull() {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_wasNull].methodEntry();
        try {
            return wasNull_;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_wasNull].methodExit();
        }
    }

public boolean execute() throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_execute].methodEntry();
        try {
            if (JdbcDebugCfg.traceActive)
            debug[methodId_execute].traceOut(JdbcDebug.debugLevelStmt,
                    "Pre-executeCall:  resultSetMax_ = " + resultSetMax_
                    + ", resultSetIndex_= " + resultSetIndex_
                    + ", isSPJResultSet_= " + isSPJResultSet_);

            /*
             * RFE: Connection synchronization Connection object is now
             * synchronized.
             */
            long beginTime=0,endTime,timeTaken;
//			if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
            if(connection_.t2props.getQueryExecuteTime() > 0) {
                beginTime=System.currentTimeMillis();
            }
            synchronized (connection_) {
                validateExecuteInvocation();
                if (inputDesc_ != null) {
                    executeCall(connection_.server_, connection_.getDialogueId(),
                            connection_.getTxid(), connection_.autoCommit_,
                            connection_.transactionMode_, stmtId_,
                            inputDesc_.length, getParameters(), queryTimeout_,
                            connection_.iso88591EncodingOverride_);
                } else
                executeCall(connection_.server_, connection_.getDialogueId(),
                        connection_.getTxid(), connection_.autoCommit_,
                        connection_.transactionMode_, stmtId_, 0, null,
                        queryTimeout_,
                        connection_.iso88591EncodingOverride_);
                if (JdbcDebugCfg.traceActive)
                debug[methodId_execute].traceOut(JdbcDebug.debugLevelStmt,
                        "Post-executeCall: resultSetMax_ = "
                        + resultSetMax_ + ", resultSetIndex_ = "
                        + resultSetIndex_ + ", isSPJResultSet_ = "
                        + isSPJResultSet_);
//				if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
                if(connection_.t2props.getQueryExecuteTime() > 0) {
                    endTime = System.currentTimeMillis();
                    timeTaken = endTime - beginTime;
                    printQueryExecuteTimeTrace(timeTaken);
                }

                if (isSPJResultSet_)
                return firstResultSetExists();
                else
                return false;
            } // End sync
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_execute].methodExit();
        }
    }

public int[] executeBatch() throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_executeBatch].methodEntry();
        try {
            clearWarnings();
            Messages.throwUnsupportedFeatureException(connection_.locale_,
                    "executeBatch()");
            return null;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_executeBatch].methodExit();
        }
    }

public ResultSet executeQuery() throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_executeQuery].methodEntry();
        try {
            /*
             * RFE: Connection synchronization Connection object is now
             * synchronized.
             */
            long beginTime=0,endTime,timeTaken;
//			if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
            if(connection_.t2props.getQueryExecuteTime() > 0) {
                beginTime=System.currentTimeMillis();
            }

            synchronized (connection_) {
                validateExecuteInvocation();
                if (inputDesc_ != null) {
                    executeCall(connection_.server_, connection_.getDialogueId(),
                            connection_.getTxid(), connection_.autoCommit_,
                            connection_.transactionMode_, stmtId_,
                            inputDesc_.length, getParameters(), queryTimeout_,
                            connection_.iso88591EncodingOverride_);
                } else
                executeCall(connection_.server_, connection_.getDialogueId(),
                        connection_.getTxid(), connection_.autoCommit_,
                        connection_.transactionMode_, stmtId_, 0, null,
                        queryTimeout_,
                        connection_.iso88591EncodingOverride_);

//				if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
                if(connection_.t2props.getQueryExecuteTime() > 0) {
                    endTime = System.currentTimeMillis();
                    timeTaken = endTime - beginTime;
                    printQueryExecuteTimeTrace(timeTaken);
                }
                if (JdbcDebugCfg.entryActive)
                debug[methodId_executeQuery]
                .methodParameters("resultSetMax_= " + resultSetMax_
                        + "  resultSetIndex_= " + resultSetIndex_
                        + "  isSPJResultSet_= " + isSPJResultSet_
                        + "  resultSet_= " + resultSet_);
                // To initialize and populate the resultSet_ in SPJRS
                if (isSPJResultSet_)
                firstResultSetExists();

                return resultSet_;
            } // End sync
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_executeQuery].methodExit();
        }
    }

public int executeUpdate() throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_executeUpdate].methodEntry();
        try {
            /*
             * RFE: Connection synchronization Connection object is now
             * synchronized.
             */
            long beginTime=0,endTime,timeTaken;
//			if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
            if(connection_.t2props.getQueryExecuteTime() > 0) {
                beginTime=System.currentTimeMillis();
            }
            synchronized (connection_) {
                validateExecuteInvocation();
                if (inputDesc_ != null) {
                    executeCall(connection_.server_, connection_.getDialogueId(),
                            connection_.getTxid(), connection_.autoCommit_,
                            connection_.transactionMode_, stmtId_,
                            inputDesc_.length, getParameters(), queryTimeout_,
                            connection_.iso88591EncodingOverride_);
                } else
                executeCall(connection_.server_, connection_.getDialogueId(),
                        connection_.getTxid(), connection_.autoCommit_,
                        connection_.transactionMode_, stmtId_, 0, null,
                        queryTimeout_,
                        connection_.iso88591EncodingOverride_);

//				if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
                if(connection_.t2props.getQueryExecuteTime() > 0) {
                    endTime = System.currentTimeMillis();
                    timeTaken = endTime - beginTime;
                    printQueryExecuteTimeTrace(timeTaken);
                }
                return rowCount_;
            } // End sync
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_executeUpdate].methodExit();
        }
    }

    // Other methods
    protected void validateGetInvocation(int parameterIndex)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_validateGetInvocation_I].methodEntry();
        try {
            clearWarnings();
            if (connection_.isClosed_)
            throw Messages.createSQLException(connection_.locale_,
                    "invalid_connection", null);
            if (isClosed_)
            throw Messages.createSQLException(connection_.locale_,
                    "invalid_statement", null);
            if (inputDesc_ == null)
            throw Messages.createSQLException(connection_.locale_,
                    "not_a_output_parameter", null);
            if (parameterIndex < 1 || parameterIndex > inputDesc_.length)
            throw Messages.createSQLException(connection_.locale_,
                    "invalid_parameter_index", null);
            if (inputDesc_[parameterIndex - 1].paramMode_ != DatabaseMetaData.procedureColumnInOut
                    && inputDesc_[parameterIndex - 1].paramMode_ != DatabaseMetaData.procedureColumnOut)
            throw Messages.createSQLException(connection_.locale_,
                    "not_a_output_parameter", null);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_validateGetInvocation_I].methodExit();
        }
    }

protected int validateGetInvocation(String parameterName)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_validateGetInvocation_L].methodEntry();
        try {
            int i;

            if (isClosed_)
            throw Messages.createSQLException(connection_.locale_,
                    "invalid_statement", null);
            if (inputDesc_ == null)
            throw Messages.createSQLException(connection_.locale_,
                    "not_a_output_parameter", null);
            for (i = 0; i < inputDesc_.length; i++) {
                if (parameterName.equalsIgnoreCase(inputDesc_[i].name_))
                return i + 1;
            }
            throw Messages.createSQLException(connection_.locale_,
                    "invalid_parameter_name", null);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_validateGetInvocation_L].methodExit();
        }
    }

private int validateSetInvocation(String parameterName) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_validateSetInvocation].methodEntry();
        try {
            int i;

            if (isClosed_)
            throw Messages.createSQLException(connection_.locale_,
                    "stmt_closed", null);
            if (inputDesc_ == null)
            throw Messages.createSQLException(connection_.locale_,
                    "invalid_parameter_index", null);
            for (i = 0; i < inputDesc_.length; i++) {
                if (parameterName.equalsIgnoreCase(inputDesc_[i].name_))
                return i + 1;
            }
            throw Messages.createSQLException(connection_.locale_,
                    "invalid_parameter_name", null);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_validateSetInvocation].methodExit();
        }
    }

    void setExecuteCallOutputs(DataWrapper wrapper, short returnResultSet,
            int txid, int ResultSetMax, int ResultSetIndex, boolean isSPJRS) {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setExecuteCallOutputs].methodEntry();
        try {
            paramValues_ = wrapper;
            returnResultSet_ = returnResultSet;
            connection_.setTxid_(txid);
            resultSetMax_ = ResultSetMax;
            resultSetIndex_ = ResultSetIndex;
            isSPJResultSet_ = isSPJRS;

            if (JdbcDebugCfg.traceActive)
            debug[methodId_setExecuteCallOutputs].traceOut(
                    JdbcDebug.debugLevelStmt, "paramValues_ = "
                    + paramValues_ + ", returnResultSet_ = "
                    + returnResultSet_ + ", connection_.txid_ = "
                    + connection_.getTxid() + ", resultSetMax_ = "
                    + resultSetMax_ + ", resultSetIndex_ = "
                    + resultSetIndex_ + ", isSPJResultSet_ = "
                    + isSPJResultSet_);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setExecuteCallOutputs].methodExit();
        }
    }

    // Constructors with access specifier as "default"
    SQLMXCallableStatement(SQLMXConnection connection, String sql)
    throws SQLException {
        super(connection, sql);
        if (JdbcDebugCfg.entryActive) {
            debug[methodId_SQLMXCallableStatement_LL].methodEntry();
            debug[methodId_SQLMXCallableStatement_LL].methodExit();
        }
    }

    SQLMXCallableStatement(SQLMXConnection connection, String sql,
            int resultSetType, int resultSetConcurrency) throws SQLException {
        super(connection, sql, resultSetType, resultSetConcurrency);
        if (JdbcDebugCfg.entryActive) {
            debug[methodId_SQLMXCallableStatement_LLII].methodEntry();
            debug[methodId_SQLMXCallableStatement_LLII].methodExit();
        }
    }

    SQLMXCallableStatement(SQLMXConnection connection, String sql,
            int resultSetType, int resultSetConcurrency,
            int resultSetHoldability) throws SQLException {
        super(connection, sql, resultSetType, resultSetConcurrency,
                resultSetHoldability);
        if (JdbcDebugCfg.entryActive) {
            debug[methodId_SQLMXCallableStatement_LLIII].methodEntry();
            debug[methodId_SQLMXCallableStatement_LLIII].methodExit();
        }
    }

    // native methods
    native void prepareCall(String server, long dialogueId, int txid,
            boolean autoCommit, int txnMode, String stmtLabel, String sql,
            int queryTimeout, int holdability, int fetchSize);

//Venu changed dialogueId and stmtId from int to long for 64 bit
private native void executeCall(String server, long dialogueId, int txid,
            boolean autoCommit, int txnMode, long stmtId, int inputParamCount,
            Object inputParamValues, int queryTimeout, String iso88591Encoding);

    // fields
    boolean wasNull_;
    short returnResultSet_;
    DataWrapper paramValues_;

private static int methodId_getArray_I = 0;
private static int methodId_getArray_L = 1;
private static int methodId_getBigDecimal_I = 2;
private static int methodId_getBigDecimal_II = 3;
private static int methodId_getBigDecimal_L = 4;
private static int methodId_getBigDecimal_LI = 5;
private static int methodId_getBlob_I = 6;
private static int methodId_getBlob_L = 7;
private static int methodId_getBoolean_I = 8;
private static int methodId_getBoolean_L = 9;
private static int methodId_getByte_I = 10;
private static int methodId_getByte_L = 11;
private static int methodId_getBytes_I = 12;
private static int methodId_getBytes_L = 13;
private static int methodId_getClob_I = 14;
private static int methodId_getClob_L = 15;
private static int methodId_getDate_I = 16;
private static int methodId_getDate_IL = 17;
private static int methodId_getDate_L = 18;
private static int methodId_getDate_LL = 19;
private static int methodId_getDouble_I = 20;
private static int methodId_getDouble_L = 21;
private static int methodId_getFloat_I = 22;
private static int methodId_getFloat_L = 23;
private static int methodId_getInt_I = 24;
private static int methodId_getInt_L = 25;
private static int methodId_getLong_I = 26;
private static int methodId_getLong_L = 27;
private static int methodId_getObject_I = 28;
private static int methodId_getObject_IL = 29;
private static int methodId_getObject_L = 30;
private static int methodId_getObject_LL = 31;
private static int methodId_getRef_I = 32;
private static int methodId_getRef_L = 33;
private static int methodId_getShort_I = 34;
private static int methodId_getShort_L = 35;
private static int methodId_getString_I = 36;
private static int methodId_getString_L = 37;
private static int methodId_getTime_I = 38;
private static int methodId_getTime_IL = 39;
private static int methodId_getTime_L = 40;
private static int methodId_getTime_LL = 41;
private static int methodId_getTimestamp_I = 42;
private static int methodId_getTimestamp_IL = 43;
private static int methodId_getTimestamp_L = 44;
private static int methodId_getTimestamp_LL = 45;
private static int methodId_getURL_I = 46;
private static int methodId_getURL_L = 47;
private static int methodId_registerOutParameter_II = 48;
private static int methodId_registerOutParameter_III = 49;
private static int methodId_registerOutParameter_IIL = 50;
private static int methodId_registerOutParameter_LI = 51;
private static int methodId_registerOutParameter_LII = 52;
private static int methodId_registerOutParameter_LIL = 53;
private static int methodId_setAsciiStream = 54;
private static int methodId_setBigDecimal = 55;
private static int methodId_setBinaryStream = 56;
private static int methodId_setBoolean = 57;
private static int methodId_setDate_LL = 58;
private static int methodId_setDate_LLL = 59;
private static int methodId_setDouble = 60;
private static int methodId_setFloat = 61;
private static int methodId_setInt = 62;
private static int methodId_setLong = 63;
private static int methodId_setNull_LI = 64;
private static int methodId_setNull_LIL = 65;
private static int methodId_setObject_LL = 66;
private static int methodId_setObject_LLI = 67;
private static int methodId_setObject_LLII = 68;
private static int methodId_setShort = 69;
private static int methodId_setString = 70;
private static int methodId_setTime_LL = 71;
private static int methodId_setTime_LLL = 72;
private static int methodId_setTimestamp_LL = 73;
private static int methodId_setTimestamp_LLL = 74;
private static int methodId_setUnicodeStream = 75;
private static int methodId_setURL = 76;
private static int methodId_wasNull = 77;
private static int methodId_execute = 78;
private static int methodId_executeBatch = 79;
private static int methodId_executeQuery = 80;
private static int methodId_executeUpdate = 81;
private static int methodId_validateGetInvocation_I = 82;
private static int methodId_validateGetInvocation_L = 83;
private static int methodId_validateSetInvocation = 84;
private static int methodId_setExecuteCallOutputs = 85;
private static int methodId_setByte = 86;
private static int methodId_setBytes = 87;
private static int methodId_setCharacterStream = 88;
private static int methodId_SQLMXCallableStatement_LL = 89;
private static int methodId_SQLMXCallableStatement_LLII = 90;
private static int methodId_SQLMXCallableStatement_LLIII = 91;
private static int methodId_SQLMXCallableStatement_LLIJL = 92;
private static int totalMethodIds = 93;
private static JdbcDebug[] debug;

    static {
        String className = "SQLMXCallableStatement";
        if (JdbcDebugCfg.entryActive) {
            debug = new JdbcDebug[totalMethodIds];
            debug[methodId_getArray_I] = new JdbcDebug(className, "getArray[I]");
            debug[methodId_getArray_L] = new JdbcDebug(className, "getArray[L]");
            debug[methodId_getBigDecimal_I] = new JdbcDebug(className,
                    "getBigDecimal[I]");
            debug[methodId_getBigDecimal_II] = new JdbcDebug(className,
                    "getBigDecimal[II]");
            debug[methodId_getBigDecimal_L] = new JdbcDebug(className,
                    "getBigDecimal[L]");
            debug[methodId_getBigDecimal_LI] = new JdbcDebug(className,
                    "getBigDecimal[LI]");
            debug[methodId_getBlob_I] = new JdbcDebug(className, "getBlob[I]");
            debug[methodId_getBlob_L] = new JdbcDebug(className, "getBlob[L]");
            debug[methodId_getBoolean_I] = new JdbcDebug(className,
                    "getBoolean[I]");
            debug[methodId_getBoolean_L] = new JdbcDebug(className,
                    "getBoolean[L]");
            debug[methodId_getByte_I] = new JdbcDebug(className, "getByte[I]");
            debug[methodId_getByte_L] = new JdbcDebug(className, "getByte[L]");
            debug[methodId_getBytes_I] = new JdbcDebug(className, "getBytes[I]");
            debug[methodId_getBytes_L] = new JdbcDebug(className, "getBytes[L]");
            debug[methodId_getClob_I] = new JdbcDebug(className, "getClob[I]");
            debug[methodId_getClob_L] = new JdbcDebug(className, "getClob[L]");
            debug[methodId_getDate_I] = new JdbcDebug(className, "getDate[I]");
            debug[methodId_getDate_IL] = new JdbcDebug(className, "getDate[IL]");
            debug[methodId_getDate_L] = new JdbcDebug(className, "getDate[L]");
            debug[methodId_getDate_LL] = new JdbcDebug(className, "getDate[LL]");
            debug[methodId_getDouble_I] = new JdbcDebug(className,
                    "getDouble[I]");
            debug[methodId_getDouble_L] = new JdbcDebug(className,
                    "getDouble[L]");
            debug[methodId_getFloat_I] = new JdbcDebug(className, "getFloat[I]");
            debug[methodId_getFloat_L] = new JdbcDebug(className, "getFloat[L]");
            debug[methodId_getInt_I] = new JdbcDebug(className, "getInt[I]");
            debug[methodId_getInt_L] = new JdbcDebug(className, "getInt[L]");
            debug[methodId_getLong_I] = new JdbcDebug(className, "getLong[I]");
            debug[methodId_getLong_L] = new JdbcDebug(className, "getLong[L]");
            debug[methodId_getObject_I] = new JdbcDebug(className,
                    "getObject[I]");
            debug[methodId_getObject_IL] = new JdbcDebug(className,
                    "getObject[IL]");
            debug[methodId_getObject_L] = new JdbcDebug(className,
                    "getObject[L]");
            debug[methodId_getObject_LL] = new JdbcDebug(className,
                    "getObject[LL]");
            debug[methodId_getRef_I] = new JdbcDebug(className, "getRef[I]");
            debug[methodId_getRef_L] = new JdbcDebug(className, "getRef[L]");
            debug[methodId_getShort_I] = new JdbcDebug(className, "getShort[I]");
            debug[methodId_getShort_L] = new JdbcDebug(className, "getShort[L]");
            debug[methodId_getString_I] = new JdbcDebug(className,
                    "getString[I]");
            debug[methodId_getString_L] = new JdbcDebug(className,
                    "getString[L]");
            debug[methodId_getTime_I] = new JdbcDebug(className, "getTime[I]");
            debug[methodId_getTime_IL] = new JdbcDebug(className, "getTime[IL]");
            debug[methodId_getTime_L] = new JdbcDebug(className, "getTime[L]");
            debug[methodId_getTime_LL] = new JdbcDebug(className, "getTime[LL]");
            debug[methodId_getTimestamp_I] = new JdbcDebug(className,
                    "getTimestamp[I]");
            debug[methodId_getTimestamp_IL] = new JdbcDebug(className,
                    "getTimestamp[IL]");
            debug[methodId_getTimestamp_L] = new JdbcDebug(className,
                    "getTimestamp[L]");
            debug[methodId_getTimestamp_LL] = new JdbcDebug(className,
                    "getTimestamp[LL]");
            debug[methodId_getURL_I] = new JdbcDebug(className, "getURL[I]");
            debug[methodId_getURL_L] = new JdbcDebug(className, "getURL[L]");
            debug[methodId_registerOutParameter_II] = new JdbcDebug(className,
                    "registerOutParameter[II]");
            debug[methodId_registerOutParameter_III] = new JdbcDebug(className,
                    "registerOutParameter[III]");
            debug[methodId_registerOutParameter_IIL] = new JdbcDebug(className,
                    "registerOutParameter[IIL]");
            debug[methodId_registerOutParameter_LI] = new JdbcDebug(className,
                    "registerOutParameter[LI]");
            debug[methodId_registerOutParameter_LII] = new JdbcDebug(className,
                    "registerOutParameter[LII]");
            debug[methodId_registerOutParameter_LIL] = new JdbcDebug(className,
                    "registerOutParameter[LIL]");
            debug[methodId_setAsciiStream] = new JdbcDebug(className,
                    "setAsciiStream");
            debug[methodId_setBigDecimal] = new JdbcDebug(className,
                    "setBigDecimal");
            debug[methodId_setBinaryStream] = new JdbcDebug(className,
                    "setBinaryStream");
            debug[methodId_setBoolean] = new JdbcDebug(className, "setBoolean");
            debug[methodId_setDate_LL] = new JdbcDebug(className, "setDate[LL]");
            debug[methodId_setDate_LLL] = new JdbcDebug(className,
                    "setDate[LLL]");
            debug[methodId_setDouble] = new JdbcDebug(className, "setDouble");
            debug[methodId_setFloat] = new JdbcDebug(className, "setFloat");
            debug[methodId_setInt] = new JdbcDebug(className, "setInt");
            debug[methodId_setLong] = new JdbcDebug(className, "setLong");
            debug[methodId_setNull_LI] = new JdbcDebug(className, "setNull[LI]");
            debug[methodId_setNull_LIL] = new JdbcDebug(className,
                    "setNull[LIL]");
            debug[methodId_setObject_LL] = new JdbcDebug(className,
                    "setObject[LL]");
            debug[methodId_setObject_LLI] = new JdbcDebug(className,
                    "setObject[LLI]");
            debug[methodId_setObject_LLII] = new JdbcDebug(className,
                    "setObject[LLII]");
            debug[methodId_setShort] = new JdbcDebug(className, "setShort");
            debug[methodId_setString] = new JdbcDebug(className, "setString");
            debug[methodId_setTime_LL] = new JdbcDebug(className, "setTime[LL]");
            debug[methodId_setTime_LLL] = new JdbcDebug(className,
                    "setTime[LLL]");
            debug[methodId_setTimestamp_LL] = new JdbcDebug(className,
                    "setTimestamp[LL]");
            debug[methodId_setTimestamp_LLL] = new JdbcDebug(className,
                    "setTimestamp[LLL]");
            debug[methodId_setUnicodeStream] = new JdbcDebug(className,
                    "setUnicodeStream");
            debug[methodId_setURL] = new JdbcDebug(className, "setURL");
            debug[methodId_wasNull] = new JdbcDebug(className, "wasNull");
            debug[methodId_execute] = new JdbcDebug(className, "execute");
            debug[methodId_executeBatch] = new JdbcDebug(className,
                    "executeBatch");
            debug[methodId_executeQuery] = new JdbcDebug(className,
                    "executeQuery");
            debug[methodId_executeUpdate] = new JdbcDebug(className,
                    "executeUpdate");
            debug[methodId_validateGetInvocation_I] = new JdbcDebug(className,
                    "validateGetInvocation[I]");
            debug[methodId_validateGetInvocation_L] = new JdbcDebug(className,
                    "validateGetInvocation[L]");
            debug[methodId_validateSetInvocation] = new JdbcDebug(className,
                    "validateSetInvocation");
            debug[methodId_setExecuteCallOutputs] = new JdbcDebug(className,
                    "setExecuteCallOutputs");
            debug[methodId_setByte] = new JdbcDebug(className, "setByte");
            debug[methodId_setBytes] = new JdbcDebug(className, "setBytes");
            debug[methodId_setCharacterStream] = new JdbcDebug(className,
                    "setCharacterStream");
            debug[methodId_SQLMXCallableStatement_LL] = new JdbcDebug(
                    className, "SQLMXCallableStatement[LL]");
            debug[methodId_SQLMXCallableStatement_LLII] = new JdbcDebug(
                    className, "SQLMXCallableStatement[LLII]");
            debug[methodId_SQLMXCallableStatement_LLIII] = new JdbcDebug(
                    className, "SQLMXCallableStatement[LLIII]");
            debug[methodId_SQLMXCallableStatement_LLIJL] = new JdbcDebug(
                    className, "SQLMXCallableStatement[LLIJL]");
        }
    }
public RowId getRowId(int parameterIndex) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public RowId getRowId(String parameterName) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public void setRowId(String parameterName, RowId x) throws SQLException {
        // TODO Auto-generated method stub

    }

public void setNString(String parameterName, String value)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public void setNCharacterStream(String parameterName, Reader value,
            long length) throws SQLException {
        // TODO Auto-generated method stub

    }

public void setNClob(String parameterName, NClob value) throws SQLException {
        // TODO Auto-generated method stub

    }

public void setClob(String parameterName, Reader reader, long length)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public void setBlob(String parameterName, InputStream inputStream,
            long length) throws SQLException {
        // TODO Auto-generated method stub

    }

public void setNClob(String parameterName, Reader reader, long length)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public NClob getNClob(int parameterIndex) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public NClob getNClob(String parameterName) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public void setSQLXML(String parameterName, SQLXML xmlObject)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public SQLXML getSQLXML(int parameterIndex) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public SQLXML getSQLXML(String parameterName) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public String getNString(int parameterIndex) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public String getNString(String parameterName) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public Reader getNCharacterStream(int parameterIndex) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public Reader getNCharacterStream(String parameterName) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public Reader getCharacterStream(int parameterIndex) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public Reader getCharacterStream(String parameterName) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public void setBlob(String parameterName, Blob x) throws SQLException {
        // TODO Auto-generated method stub

    }

public void setClob(String parameterName, Clob x) throws SQLException {
        // TODO Auto-generated method stub

    }

public void setAsciiStream(String parameterName, InputStream x, long length)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public void setBinaryStream(String parameterName, InputStream x, long length)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public void setCharacterStream(String parameterName, Reader reader,
            long length) throws SQLException {
        // TODO Auto-generated method stub

    }

public void setAsciiStream(String parameterName, InputStream x)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public void setBinaryStream(String parameterName, InputStream x)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public void setCharacterStream(String parameterName, Reader reader)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public void setNCharacterStream(String parameterName, Reader value)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public void setClob(String parameterName, Reader reader)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public void setBlob(String parameterName, InputStream inputStream)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public void setNClob(String parameterName, Reader reader)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public Object getObject(int parameterIndex, Class type)
    throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public Object getObject(String parameterName, Class type)
    throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public byte[] getSQLBytes(int parameterIndex) throws SQLException {
        return paramValues_.getSQLBytes(parameterIndex);
    }
}
