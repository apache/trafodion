/*******************************************************************************
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
 *******************************************************************************/

package org.trafodion.jdbc.t2;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.sql.Date;
import java.sql.SQLException;
import java.sql.Time;
import java.sql.Timestamp;
import java.text.DecimalFormat;
import java.text.FieldPosition;
import java.util.Properties;
import java.util.logging.Level;

class InterfaceResultSet {
    InterfaceConnection ic_;
    String stmtLabel_;
    SQLMXStatement stmt_;

    /* CHAR/CHARACTER */
    static final int SQLTYPECODE_CHAR = 1;

    /* NUMERIC */
    static final int SQLTYPECODE_NUMERIC = 2;
    static final int SQLTYPECODE_NUMERIC_UNSIGNED = -201;

    /* DECIMAL */
    static final int SQLTYPECODE_DECIMAL = 3;

    static final int SQLTYPECODE_DECIMAL_UNSIGNED = -301;
    static final int SQLTYPECODE_DECIMAL_LARGE = -302;
    static final int SQLTYPECODE_DECIMAL_LARGE_UNSIGNED = -303;

    /* INTEGER/INT */
    static final int SQLTYPECODE_INTEGER = 4;

    static final int SQLTYPECODE_INTEGER_UNSIGNED = -401;
    static final int SQLTYPECODE_LARGEINT = -402;

    /* SMALLINT */
    static final int SQLTYPECODE_SMALLINT = 5;

    static final int SQLTYPECODE_SMALLINT_UNSIGNED = -502;

    static final int SQLTYPECODE_BPINT_UNSIGNED = -503;

    /*
     * DOUBLE depending on precision
     */
    static final int SQLTYPECODE_FLOAT = 6;

    /*
    */
    static final int SQLTYPECODE_REAL = 7;

    /*
    */
    static final int SQLTYPECODE_DOUBLE = 8;

    /* DATE,TIME,TIMESTAMP */
    static final int SQLTYPECODE_DATETIME = 9;

    /* TIMESTAMP */
    static final int SQLTYPECODE_INTERVAL = 10;

    /* no ANSI value 11 */

    /* VARCHAR/CHARACTER VARYING */
    static final int SQLTYPECODE_VARCHAR = 12;

    /* SQL/MP stype VARCHAR with length prefix:
     * */
    static final int SQLTYPECODE_VARCHAR_WITH_LENGTH = -601;

    /* LONG VARCHAR/ODBC CHARACTER VARYING */
    static final int SQLTYPECODE_VARCHAR_LONG = -1; /* ## NEGATIVE??? */

    /* no ANSI value 13 */

    /* BIT */
    static final int SQLTYPECODE_BIT = 14; /* not supported */

    /* BIT VARYING */
    static final int SQLTYPECODE_BITVAR = 15; /* not supported */

    /* NCHAR -- CHAR(n) CHARACTER SET s -- where s uses two bytes per char */
    static final int SQLTYPECODE_CHAR_DBLBYTE = 16;

    /* NCHAR VARYING -- VARCHAR(n) CHARACTER SET s -- s uses 2 bytes per char */
    static final int SQLTYPECODE_VARCHAR_DBLBYTE = 17;

    /* Date/Time/TimeStamp related constants */
    static final int SQLDTCODE_DATE = 1;
    static final int SQLDTCODE_TIME = 2;
    static final int SQLDTCODE_TIMESTAMP = 3;
    static final int SQLDTCODE_MPDATETIME = 4;
    static final int dateLength = 10;
    static final int timeLength = 8;
    static final int timestampLength = 26;

    InterfaceNativeResultset inRst_;

    static Properties javaLangToJavaNio = null;

    SQLMXResultSet rs_;

    InterfaceResultSet(SQLMXResultSet rs) throws SQLException {
        this.ic_ = ((SQLMXConnection) rs.connection_).getServerHandle();
        stmtLabel_  = rs.stmt_.getStmtLabel_();
        inRst_ = new InterfaceNativeResultset(this);
        rs_ = rs;
    };

    // -------------------------------------------------------------------
    // from nskieee.cpp -- find the length for the data based on datatype
    //
    static int dataLengthFetchPerf(int SQLDataType, int SQLDateTimeCode, int SQLOctetLength, int maxRowLen,
            int bufferLen, int ODBCDataType, int ODBCPrecision) {
        int allocLength = 0;
        switch (SQLDataType) {
            case SQLTYPECODE_INTERVAL:
                allocLength = SQLOctetLength;
                break;
            case SQLTYPECODE_VARCHAR_WITH_LENGTH:
            case SQLTYPECODE_VARCHAR_LONG:
            case SQLTYPECODE_VARCHAR_DBLBYTE:
            case SQLTYPECODE_BITVAR:
                int dataOffset = (maxRowLen < 0x7FFF) ? 2 : 4;
                allocLength = bufferLen + dataOffset;
                break;
            case SQLTYPECODE_CHAR:

                allocLength = SQLOctetLength; // null terminated
                if (maxRowLen > 0) {
                    allocLength = (allocLength > maxRowLen) ? (maxRowLen + 1) : (allocLength);
                }
                break;
            case SQLTYPECODE_BIT:
            case SQLTYPECODE_CHAR_DBLBYTE:
            case SQLTYPECODE_VARCHAR:
                allocLength = SQLOctetLength - 1; // no null at the end
                if (maxRowLen > 0) {
                    allocLength = (allocLength > maxRowLen) ? (maxRowLen + 1) : (allocLength);
                }
                break;
            case SQLTYPECODE_DATETIME:
                switch (SQLDateTimeCode) {
                    case SQLDTCODE_DATE:
                        allocLength = dateLength;
                        break;
                    case SQLDTCODE_TIME:
                        if (ODBCDataType == java.sql.Types.OTHER) // For
                        {
                            allocLength = SQLOctetLength;
                        } else {
                            allocLength = timeLength;
                        }
                        break;
                    case SQLDTCODE_TIMESTAMP:

                        allocLength = ODBCPrecision;
                        break;
                    default:
                        allocLength = SQLOctetLength;
                        break;
                }

                break;
            default:
                allocLength = SQLOctetLength; // exclude nullable
                break;
        }
        return allocLength;
    }

    // -------------------------------------------------------------------
    // get the column value data in String format
    Object getFetchString(SQLMXConnection conn, int scale, int SQLDataType, int SQLDatetimeCode, int FSDataType,
            byte[] ibuffer, int byteIndex, int byteLen, int SQLcharset, int ODBCDataType) throws SQLException {
        Object retObj;
        String tmpStr;
        byte[] tbuffer;
        BigDecimal tmpbd;

        switch (SQLDataType) {

            case SQLTYPECODE_CHAR:
                tbuffer = new byte[byteLen];
                System.arraycopy(ibuffer, byteIndex, tbuffer, 0, byteLen);
                retObj = tbuffer;
                break;
            case SQLTYPECODE_VARCHAR:
                tbuffer = new byte[byteLen];
                System.arraycopy(ibuffer, byteIndex, tbuffer, 0, byteLen);
                retObj = tbuffer;
                break;
            case SQLTYPECODE_INTERVAL:
                tbuffer = new byte[byteLen];
                System.arraycopy(ibuffer, byteIndex, tbuffer, 0, byteLen);
                retObj = tbuffer;
                break;
            case SQLTYPECODE_VARCHAR_WITH_LENGTH:
            case SQLTYPECODE_VARCHAR_LONG:
                tbuffer = new byte[byteLen - 2];
                System.arraycopy(ibuffer, byteIndex + 2, tbuffer, 0, byteLen - 2);

                // retObj = new String(tbuffer); Swastik for LOB Support 10/29/2004
                retObj = tbuffer;
                break;
            case SQLTYPECODE_DATETIME:
                tmpStr = new String(Bytes.read_chars(ibuffer, byteIndex, byteLen));
                switch (SQLDatetimeCode) {
                    case SQLDTCODE_DATE:
                        retObj = Date.valueOf(tmpStr);
                        break;
                    case SQLDTCODE_TIMESTAMP:
                        retObj = Timestamp.valueOf(tmpStr);
                        break;
                    case SQLDTCODE_TIME:

                        // Need to add code here to check if it's
                        // SQLMXDesc.SQLDTCODE_HOUR_TO_FRACTION
                        if (ODBCDataType != java.sql.Types.OTHER) {
                            retObj = Time.valueOf(tmpStr);
                            break;
                        } else {
                            // Do default processing as it is
                            // SQLMXDesc.SQLDTCODE_HOUR_TO_FRACTION
                        }
                    default:
                        retObj = tmpStr;
                        break;
                }
                break;
            case SQLTYPECODE_SMALLINT:
                short sValue = Bytes.extractShort(ibuffer, byteIndex, this.ic_.getByteSwap());
                retObj = new Short(sValue);
                if (scale > 0) {
                    retObj = new BigDecimal(new BigInteger(retObj.toString()), scale);
                }
                break;
            case SQLTYPECODE_SMALLINT_UNSIGNED:
                retObj = new Integer(Bytes.extractUShort(ibuffer, byteIndex, this.ic_.getByteSwap()));
                if (scale > 0) {
                    retObj = new BigDecimal(new BigInteger(retObj.toString()), scale);
                }
                break;
            case SQLTYPECODE_INTEGER:
                retObj = new Integer(Bytes.extractInt(ibuffer, byteIndex, this.ic_.getByteSwap()));
                if (scale > 0) {
                    retObj = new BigDecimal(new BigInteger(retObj.toString()), scale);
                }
                break;
            case SQLTYPECODE_INTEGER_UNSIGNED:
                retObj = new Long(Bytes.extractUInt(ibuffer, byteIndex, this.ic_.getByteSwap()));
                if (scale > 0) {
                    retObj = new BigDecimal(new BigInteger(retObj.toString()), scale);
                }
                break;
            case SQLTYPECODE_LARGEINT:
                tbuffer = new byte[byteLen];
                System.arraycopy(ibuffer, byteIndex, tbuffer, 0, byteLen);
                retObj = new BigInteger(tbuffer);
                if (scale > 0) {
                    retObj = new BigDecimal((BigInteger) retObj, scale);
                }
                break;
            case SQLTYPECODE_NUMERIC:
            case SQLTYPECODE_NUMERIC_UNSIGNED:
                switch (FSDataType) {
                    case 130:
                    case 131:
                        tmpStr = String.valueOf(Bytes.extractShort(ibuffer, byteIndex, this.ic_.getByteSwap()));
                        break;
                    case 132:
                    case 133:
                        tmpStr = String.valueOf(Bytes.extractInt(ibuffer, byteIndex, this.ic_.getByteSwap()));
                        break;
                    case 134:
                        tmpStr = String.valueOf(Bytes.extractLong(ibuffer, byteIndex, this.ic_.getByteSwap()));
                        break;
                    default:
                        throw Messages.createSQLException(conn.t2props, conn.getLocale(), "restricted_data_type", null);
                }
                retObj = new BigDecimal((new BigInteger(tmpStr)), scale);
                break;
            case SQLTYPECODE_DECIMAL:
            case SQLTYPECODE_DECIMAL_UNSIGNED:
            case SQLTYPECODE_DECIMAL_LARGE:
            case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
                String retStr;

                // check if the sign is minus (-80)
                byte sign = (byte) (ibuffer[byteIndex] & (byte) (-80));

                // first byte = inbyte - (-80)
                if (sign == (byte) (-80)) {
                    byte firstByte = (byte) (ibuffer[byteIndex] - (byte) (-80));
                    retStr = "-" + firstByte + String.valueOf(Bytes.read_chars(ibuffer, byteIndex + 1, byteLen - 1));
                } else {
                    retStr = String.valueOf(Bytes.read_chars(ibuffer, byteIndex, byteLen));
                }
                retObj = new BigDecimal(new BigInteger(retStr), scale);
                break;
            case SQLTYPECODE_REAL:
                retObj = new Float(Float.intBitsToFloat(Bytes.extractInt(ibuffer, byteIndex, this.ic_.getByteSwap())));
                break;
            case SQLTYPECODE_DOUBLE:
            case SQLTYPECODE_FLOAT:
                retObj = new Double(Double.longBitsToDouble(Bytes.extractLong(ibuffer, byteIndex, this.ic_.getByteSwap())));
                break;
            case SQLTYPECODE_BIT:
            case SQLTYPECODE_BITVAR:
            case SQLTYPECODE_BPINT_UNSIGNED:
            default:
                throw Messages.createSQLException(conn.t2props, conn.getLocale(), "restricted_data_type", null);
        }
        return retObj;
    } // end getFetchString

    private static String padZero(long i, int len) {
        String s = String.valueOf(i);

        while (s.length() < len)
            s = '0' + s;

        return s;
    }

    // -------------------------------------------------------------------
    // get the column value data from Execute2 in String format
    static Object getExecute2FetchString(SQLMXConnection conn, SQLMXDesc desc, byte[] values, int noNullValue,
            int ODBCDataType, boolean useOldDateFormat, boolean swap) throws SQLException {
        Object retObj;
        String tmpStr;
        byte[] tbuffer;
        BigDecimal tmpbd;
        int length;
        int year, month, day, hour, minute, second;
        long nanoSeconds;

        switch (desc.sqlDataType_) {
            case SQLTYPECODE_CHAR:
                length = desc.sqlOctetLength_ - 1;    /* since it's 1 byte longer in descriptor than actual*/
                tbuffer = new byte[length];
                System.arraycopy(values, noNullValue, tbuffer, 0, length);
                retObj = tbuffer;
                break;
            case SQLTYPECODE_VARCHAR:
            case SQLTYPECODE_VARCHAR_WITH_LENGTH:
            case SQLTYPECODE_VARCHAR_LONG:
                boolean shortLength = desc.precision_ < Math.pow(2, 15);
                int dataOffset = noNullValue + ((shortLength) ? 2 : 4);

                length = (shortLength) ? Bytes.extractShort(values, noNullValue, swap) : Bytes.extractInt(values, noNullValue, swap);

                tbuffer = new byte[length];
                int len = values.length - (dataOffset);
                System.arraycopy(values, (dataOffset), tbuffer, 0, (length > len) ? len : length);
                retObj = tbuffer;
                break;
            case SQLTYPECODE_INTERVAL:
                length = desc.sqlOctetLength_;
                tbuffer = new byte[length];
                System.arraycopy(values, noNullValue, tbuffer, 0, length);
                retObj = tbuffer;
                break;
            case SQLTYPECODE_DATETIME:
                switch (desc.sqlDatetimeCode_) {
                    case SQLDTCODE_DATE:
                        if (!useOldDateFormat) // new date format, only for bulk move
                        {
                            // "yyyy-mm-dd"
                            year = Bytes.extractUShort(values, noNullValue, swap);
                            month = values[noNullValue + 2];
                            day = values[noNullValue + 3];
                            String t = padZero(year,4) + "-" + padZero(month,2) + "-" + padZero(day,2);
                            retObj = Date.valueOf(t);
                        } else {// do the old way
                            length = dateLength;
                            retObj = Date.valueOf(new String(Bytes.read_chars(values, noNullValue, length)));
                        }
                        break;
                    case SQLDTCODE_TIMESTAMP:
                        if (!useOldDateFormat) // new date format, if not SQLCatalogs
                        {
                            // yyyy-mm-dd hh:mm:ss.fffffffff
                            year = Bytes.extractUShort(values, noNullValue, swap);
                            month = values[noNullValue + 2];
                            day = values[noNullValue + 3];
                            hour = values[noNullValue + 4];
                            minute = values[noNullValue + 5];
                            second = values[noNullValue + 6];

                            if (desc.sqlPrecision_ > 0) {
                                nanoSeconds = Bytes.extractUInt(values, noNullValue + 7, swap);

                                if (nanoSeconds > 999999) // returned in microseconds
                                    nanoSeconds = 0;

                                // apply leading 0's for string conversion
                                tmpStr = "" + nanoSeconds;
                                length = tmpStr.length();
                                for (int i = 0; i < desc.sqlPrecision_ - length; i++) {
                                    tmpStr = "0" + tmpStr;
                                }
                            } else {
                                tmpStr = "0";
                            }

                            retObj = Timestamp.valueOf(padZero((int) year, 4) + "-" + padZero(month, 2) + "-" + padZero(day, 2)
                                    + " " + padZero(hour, 2) + ":" + padZero(minute, 2) + ":" + padZero(second, 2) + "."
                                    + tmpStr);
                        } else { // do the old way
                            length = desc.maxLen_;
                            retObj = Timestamp.valueOf(new String(Bytes.read_chars(values, noNullValue, length)));
                        }
                        break;

                    case SQLDTCODE_TIME:
                        if (ODBCDataType == java.sql.Types.OTHER) // For
                            // SQLMXDesc.SQLDTCODE_HOUR_TO_FRACTION
                        {
                            length = desc.sqlOctetLength_;
                            retObj = new String(Bytes.read_chars(values, noNullValue, length));
                        } else {
                            length = timeLength;
                            if (!useOldDateFormat) // new date format, only for bulk
                                // move
                            {
                                // "hh:mm:ss"
                                hour = values[noNullValue];
                                minute = values[noNullValue + 1];
                                second = values[noNullValue + 2];

                                if (desc.sqlPrecision_ > 0) {
                                    nanoSeconds = Bytes.extractUInt(values, noNullValue + 3, swap);

                                    if (nanoSeconds > 999999) // returned in microseconds
                                        nanoSeconds = 0;

                                    String formatStr = "";
                                    for(int i=0;i<desc.sqlPrecision_;i++)
                                        formatStr += "0";

                                    StringBuffer sb = new StringBuffer();
                                    DecimalFormat format = new DecimalFormat("00");
                                    format.format(hour, sb, new FieldPosition(0));
                                    sb.append(':');
                                    format.format(minute, sb, new FieldPosition(0));
                                    sb.append(':');
                                    format.format(second, sb, new FieldPosition(0));
                                    sb.append('.');
                                    format = new DecimalFormat(formatStr);
                                    format.format(nanoSeconds, sb, new FieldPosition(0));

                                    retObj = sb.toString();
                                } else {
                                    retObj = Time.valueOf(String.valueOf(hour) + ":" + String.valueOf(minute) + ":"
                                            + String.valueOf(second));
                                }
                            } else
                                // do the old way
                                retObj = Time.valueOf(new String(Bytes.read_chars(values, noNullValue, length)));
                        }
                        break;
                    default:
                        length = desc.sqlOctetLength_;
                        retObj = new String(Bytes.read_chars(values, noNullValue, length));
                        break;
                }
                retObj = retObj.toString();
                break;
            case SQLTYPECODE_SMALLINT:
                short sValue = Bytes.extractShort(values, noNullValue, swap);
                retObj = new Short(sValue);
                if (desc.scale_ > 0) {
                    retObj = new BigDecimal(new BigInteger(retObj.toString()), desc.scale_);
                }
                break;
            case SQLTYPECODE_SMALLINT_UNSIGNED:
                int signedSValue = Bytes.extractUShort(values, noNullValue, swap);
                if (desc.scale_ > 0) {
                    tmpbd = new BigDecimal(new BigInteger(String.valueOf(signedSValue)), (int) desc.scale_);
                } else {
                    tmpbd = new BigDecimal(String.valueOf(signedSValue));
                }
                retObj = tmpbd;
                break;
            case SQLTYPECODE_INTEGER:
                retObj = new Integer(Bytes.extractInt(values, noNullValue,swap));
                if (desc.scale_ > 0) {
                    retObj = new BigDecimal(new BigInteger(retObj.toString()), desc.scale_);
                }
                break;
            case SQLTYPECODE_INTEGER_UNSIGNED:
                retObj = new Long(Bytes.extractUInt(values, noNullValue, swap));
                if (desc.scale_ > 0) {
                    retObj = new BigDecimal(new BigInteger(retObj.toString()), desc.scale_);
                }
                break;
            case SQLTYPECODE_LARGEINT:
                retObj = new Long(Bytes.extractLong(values, noNullValue, swap));
                if (desc.scale_ > 0) {
                    retObj = new BigDecimal(new BigInteger(retObj.toString()), desc.scale_);
                }
                break;
            case SQLTYPECODE_NUMERIC:
            case SQLTYPECODE_NUMERIC_UNSIGNED:
                tbuffer = new byte[desc.sqlOctetLength_];
                System.arraycopy(values, noNullValue, tbuffer, 0, desc.sqlOctetLength_);
                retObj = InterfaceUtilities.convertSQLBigNumToBigDecimal(tbuffer, desc.scale_, swap);
                break;
            case SQLTYPECODE_DECIMAL:
            case SQLTYPECODE_DECIMAL_UNSIGNED:
            case SQLTYPECODE_DECIMAL_LARGE:
            case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
                String retStr;

                // check if the sign is minus (-80)
                byte sign = (byte) (values[noNullValue] & (byte) (-80));

                // first byte = inbyte - (-80)
                if (sign == (byte) (-80)) {
                    byte firstByte = (byte) (values[noNullValue] - (byte) (-80));
                    retStr = "-" + firstByte
                        + String.valueOf(Bytes.read_chars(values, noNullValue + 1, desc.sqlOctetLength_ - 1));
                } else {
                    retStr = String.valueOf(Bytes.read_chars(values, noNullValue, desc.sqlOctetLength_));
                }
                retObj = new BigDecimal(new BigInteger(retStr), desc.scale_);
                break;
            case SQLTYPECODE_REAL:
                retObj = new Float(Float.intBitsToFloat(Bytes.extractInt(values, noNullValue, swap)));
                break;
            case SQLTYPECODE_DOUBLE:
            case SQLTYPECODE_FLOAT:
                retObj = new Double(Double.longBitsToDouble(Bytes.extractLong(values, noNullValue, swap)));
                break;
            case SQLTYPECODE_BIT:
            case SQLTYPECODE_BITVAR:
            case SQLTYPECODE_BPINT_UNSIGNED:
            default:
                throw Messages.createSQLException(conn.t2props, conn.getLocale(), "restricted_data_type", null);
        }
        return retObj;
    } // end getExecute2FetchString

    // -------------------------------------------------------------------
    void setFetchOutputs(SQLMXResultSet rs, int rowsAffected, boolean endOfData, byte[] outputDataValue)
        throws SQLException

    {
        ObjectArray[] rowArray;
        Object[] objectArray;
        Object columnValue;

        int columnCount;
        int rowIndex;
        int columnIndex;
        int byteIndex = 0;
        short SQLDataInd = 0;
        int byteLen = 0;
        int maxRowLen = rs.connection_.ic_.getTransportBufferSize(); // maxRowLen

        rowArray = new ObjectArray[rowsAffected];

        // get the number of colums
        columnCount = rs.getNoOfColumns();

        for (rowIndex = 0; rowIndex < rowsAffected; rowIndex++) {
            objectArray = new Object[columnCount];

            for (columnIndex = 0; columnIndex < columnCount; columnIndex++) {
                SQLDataInd = new Byte(outputDataValue[byteIndex++]).shortValue();

                if (SQLDataInd == 0) {
                    short varDataLen;
                    if (outputDataValue.length > (byteIndex + 2)) {
                        varDataLen = Bytes.extractShort(outputDataValue, byteIndex, this.ic_.getByteSwap());
                    } else {
                        varDataLen = 0;

                    }
                    byteLen = dataLengthFetchPerf(rs.outputDesc_[columnIndex].sqlDataType_,
                            rs.outputDesc_[columnIndex].sqlDatetimeCode_, rs.outputDesc_[columnIndex].sqlOctetLength_,
                            maxRowLen, // maxLength
                            varDataLen, rs.outputDesc_[columnIndex].dataType_, rs.outputDesc_[columnIndex].precision_);

                    columnValue = getFetchString(rs.connection_, rs.outputDesc_[columnIndex].scale_,
                            rs.outputDesc_[columnIndex].sqlDataType_, rs.outputDesc_[columnIndex].sqlDatetimeCode_,
                            rs.outputDesc_[columnIndex].fsDataType_, outputDataValue, byteIndex, byteLen,
                            rs.outputDesc_[columnIndex].sqlCharset_, rs.outputDesc_[columnIndex].dataType_);

                    byteIndex = byteIndex + byteLen;

                    switch (rs.outputDesc_[columnIndex].sqlDataType_) {
                        case SQLTYPECODE_VARCHAR_WITH_LENGTH:
                        case SQLTYPECODE_VARCHAR_LONG:
                        case SQLTYPECODE_VARCHAR_DBLBYTE:
                        case SQLTYPECODE_BITVAR:
                        case SQLTYPECODE_CHAR:
                        case SQLTYPECODE_CHAR_DBLBYTE:
                        case SQLTYPECODE_VARCHAR:
                            byteIndex++;
                            break;
                    }

                    if (columnValue == null) {
                        throw Messages
                            .createSQLException(rs.connection_.t2props, ic_.getLocale(), "null_data", null);
                    }
                } else {
                    columnValue = null;

                }
                objectArray[columnIndex] = columnValue;
            }
            rowArray[rowIndex] = new ObjectArray(columnCount, objectArray);
        }
        rs.setFetchOutputs(rowArray, rowsAffected, endOfData);
    }

    // ----------------------------------------------------------------------------
    void setExecute2FetchOutputs(SQLMXResultSet rs, int rowsAffected, boolean endOfData, byte[] values)
        throws SQLException {

        Object[] objectArray;
        Object columnValue;
        DataWrapper[] rowArray = new DataWrapper[rowsAffected];

        int columnCount = rs.getNoOfColumns();
        int rowIndex;
        int columnIndex;
        int byteIndex = 0;
        int SQLDataInd = 0;
        int byteLen = 0;
        int maxRowLen = rs.connection_.ic_.getTransportBufferSize(); // maxRowLen

        objectArray = new Object[columnCount];

        int dataLength = 0;

        if (rs.outputDesc_ != null && rs.outputDesc_.length > 0) {
            dataLength = rs.outputDesc_[0].rowLength_;
        }

        int rowOffset = 0;
        for (rowIndex = 0; rowIndex < rowsAffected; rowIndex++) {
            rowOffset = rowIndex * dataLength;

            for (columnIndex = 0; columnIndex < columnCount; columnIndex++) {
                int noNullValueOffset = rs.outputDesc_[columnIndex].noNullValue_;
                int nullValueOffset = rs.outputDesc_[columnIndex].nullValue_;

                if (nullValueOffset != -1)
                    nullValueOffset += rowOffset;
                if (noNullValueOffset != -1)
                    noNullValueOffset += rowOffset;

                if (nullValueOffset != -1 && Bytes.extractShort(values, nullValueOffset, true) == -1) {
                    columnValue = null;
                } else {
                    columnValue = getExecute2FetchString(rs.connection_, rs.outputDesc_[columnIndex], values,
                            noNullValueOffset, rs.outputDesc_[columnIndex].dataType_, rs.useOldDateFormat(), true);
                    if (columnValue == null) {
                        throw Messages
                            .createSQLException(rs.connection_.t2props, ic_.getLocale(), "null_data", null);
                    }
                } // end if else

                objectArray[columnIndex] = columnValue;
            } // end for

            rowArray[rowIndex] = new DataWrapper(columnCount);
            for (int colIndex=0; colIndex < columnCount; colIndex++) {
                rowArray[rowIndex].setObject(colIndex+1, objectArray[colIndex]);
            }
        }
        rs.setFetchOutputs(rowArray, rowsAffected, endOfData, 0);

    } // end setExectue2FetchOutputs

    // ----------------------------------------------------------------------------
    // Interface methods
    boolean fetch(long stmtHandle, int maxRowCnt, int queryTimeout, int holdability, SQLMXResultSet rs)
        throws SQLException {
        int sqlAsyncEnable = 0;
        int stmtCharset = 1;
        String cursorName = "";
        int cursorCharset = 1;
        String stmtOptions = "";

        boolean endOfData = false;
        boolean dataFound = false;
        String sqlStmt = ""; // qs_interface

        if (rs_.stmt_ != null && rs_.stmt_.sql_ != null) {
            sqlStmt = rs_.stmt_.sql_.toUpperCase();
        }

        FetchReply fr;

        try {
            fr = inRst_.Fetch(sqlAsyncEnable, queryTimeout, stmtHandle, stmtCharset, maxRowCnt, cursorName,
                    cursorCharset, stmtOptions);

        } catch (SQLException tex) {
            throw tex;
        }

        switch (fr.returnCode) {
            case TRANSPORT.CEE_SUCCESS:
            case TRANSPORT.SQL_SUCCESS_WITH_INFO:

                // do warning processing
                if (fr.errorList.length != 0) {
                    Messages.setSQLWarning(rs.connection_.t2props, rs, fr.errorList);
                }
                //endOfData = (fr.rowsAffected < maxRowCnt) ? true : false;

                if (fr.rowsAffected > 0) {
                    if (rs.keepRawBuffer_ == true) {
                        rs.rawBuffer_ = fr.outValues;
                    }

                    setExecute2FetchOutputs(rs, fr.rowsAffected, endOfData, fr.outValues);

                    dataFound = true;
                }
                break;
            case 100: // fix this
            case odbc_SQLSvc_Fetch_exc_.odbc_SQLSvc_Fetch_SQLNoDataFound_exn_:
                dataFound = false;
                endOfData = true;
                break;

            default:
                Messages.throwSQLException(rs.connection_.t2props, fr.errorList);

        }

        // see if the rows fetched is valid
        return dataFound;
        };

    void close() throws SQLException {
        ic_.isConnectionOpen();
        int rval = 0;
        CloseReply cry_ = null;

        //
        // If the query is non-unique, then close the result set (cursor).
        // If the query was a unique select, then the result set was implicitly
        // closed by NCS.
        //
        if (rs_ != null && rs_.stmt_ != null && rs_.stmt_.ist_ != null
                && rs_.stmt_.ist_.sqlQueryType_ != TRANSPORT.SQL_SELECT_UNIQUE) {
            cry_ = inRst_.Close();

            switch (cry_.m_p1.exception_nr) {
                case TRANSPORT.CEE_SUCCESS:

                    // ignore the SQLWarning for the static close
                    break;
                case odbc_SQLSvc_Close_exc_.odbc_SQLSvc_Close_SQLError_exn_:
                    Messages.throwSQLException(rs_.connection_.t2props, cry_.m_p1.SQLError);
                default:
                    throw Messages.createSQLException(rs_.connection_.t2props, ic_.getLocale(),
                            "ids_unknown_reply_error", null);
            } // end switch
                } // end if

    };

    // ----------------------------------------------------------------------------
    static Object[] getExecute2Outputs(SQLMXConnection conn, SQLMXDesc[] desc, byte[] values, boolean swap) throws SQLException

    {
        Object[] objectArray;
        Object columnValue;
        int columnIndex;
        int columnCount = (desc == null) ? 0 : desc.length;

        objectArray = new Object[columnCount];

        for (columnIndex = 0; columnIndex < columnCount; columnIndex++) {
            int noNullValueOffset = desc[columnIndex].noNullValue_;
            int nullValueOffset = desc[columnIndex].nullValue_;

            if ((nullValueOffset != -1 && Bytes.extractShort(values, nullValueOffset, swap) == -1)
                    || (desc[columnIndex].paramMode_ == SQLMXParameterMetaData.parameterModeIn)) {
                columnValue = null;
            } else {
                columnValue = getExecute2FetchString(conn, desc[columnIndex], values, noNullValueOffset,
                        desc[columnIndex].dataType_, false, swap);
                if (columnValue == null) {
                    throw Messages.createSQLException(conn.t2props, conn.getLocale(), "null_data", null);
                }
            } // end if else

            objectArray[columnIndex] = columnValue;
        } // end for

        return objectArray;

    } // end getExectue2Outputs

}
