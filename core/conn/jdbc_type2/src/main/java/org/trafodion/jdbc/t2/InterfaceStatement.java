/* ************************************************************************
// @@@ START COPYRIGHT @@@
//
//// Licensed to the Apache Software Foundation (ASF) under one
//// or more contributor license agreements.  See the NOTICE file
//// distributed with this work for additional information
//// regarding copyright ownership.  The ASF licenses this file
//// to you under the Apache License, Version 2.0 (the
//// "License"); you may not use this file except in compliance
//// with the License.  You may obtain a copy of the License at
////
////   http://www.apache.org/licenses/LICENSE-2.0
////
//// Unless required by applicable law or agreed to in writing,
//// software distributed under the License is distributed on an
//// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//// KIND, either express or implied.  See the License for the
//// specific language governing permissions and limitations
//// under the License.
////
//// @@@ END COPYRIGHT @@@
//**************************************************************************/
//

package org.trafodion.jdbc.t2;

import java.math.BigDecimal;
import java.sql.DataTruncation;
import java.sql.Date;
import java.sql.SQLException;
import java.sql.Time;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Locale;
import java.util.logging.Level;

class InterfaceStatement {
    InterfaceConnection ic_;
    private long rowCount_;
    static final short SQL_DROP = 1;
    static short EXTERNAL_STMT = 0;
    int sqlStmtType_ = TRANSPORT.TYPE_UNKNOWN;
    int stmtType_ = 0;
    InterfaceNativeStmt inStmt_;
    int queryTimeout_;
    String stmtLabel_;
    String cursorName_;
    SQLMXStatement stmt_;

    int sqlQueryType_;
    long stmtHandle_ = 0;
    int estimatedCost_;
    boolean prepare2 = false;
    boolean stmtIsLock = false; 

    // used for SPJ transaction
    static Class LmUtility_class_ = null;
    static java.lang.reflect.Method LmUtility_getTransactionId_ = null;

    PrepareReply pr_;

    // ----------------------------------------------------------------------
    InterfaceStatement(SQLMXStatement stmt) throws SQLException {
        this.ic_ = ((SQLMXConnection) stmt.getConnection()).getServerHandle();
        queryTimeout_ = stmt.queryTimeout_;
        stmtLabel_ = stmt.getStmtLabel_();
        cursorName_ = stmt.cursorName_;
        inStmt_ = new InterfaceNativeStmt(this);
        stmt_ = stmt;
    };

    public int getSqlQueryType() {
        return sqlQueryType_;
    }

    private String convertDateFormat(String dt) {
        String tokens[] = dt.split("[/]", 3);

        if (tokens.length != 3) {
            return dt;
        }
        StringBuffer sb = new StringBuffer();
        sb.append(tokens[0]).append("-").append(tokens[1]).append("-").append(tokens[2]);
        return sb.toString();
    }

    // ----------------------------------------------------------------------
    /**
     * This method will take an object and convert it to the approperite format
     * for sending to TrafT2.
     * 
     * @param locale
     *            The locale for this operation
     * @param pstmt
     *            The prepared statement associated with the object
     * @param paramValue
     *            The object to convert
     * @param paramNumber
     *            The parameter number associated with this object
     * @param values
     *            The array to place the converted object into
     */
    void convertObjectToSQL2(Locale locale, SQLMXStatement pstmt, Object paramValue, int paramRowCount, int paramNumber,
            byte[] values, int rowNumber) throws SQLException {
        byte[] tmpBarray = null;
        int i;
        BigDecimal tmpbd;

        int precision = pstmt.inputDesc_[paramNumber].precision_;
        int scale = pstmt.inputDesc_[paramNumber].scale_;
        int sqlDatetimeCode = pstmt.inputDesc_[paramNumber].sqlDatetimeCode_;
        int FSDataType = pstmt.inputDesc_[paramNumber].fsDataType_;
        int OdbcDataType = pstmt.inputDesc_[paramNumber].dataType_;
        int maxLength = pstmt.inputDesc_[paramNumber].sqlOctetLength_;
        int dataType = pstmt.inputDesc_[paramNumber].sqlDataType_;
        int dataCharSet = pstmt.inputDesc_[paramNumber].sqlCharset_;
        int dataLen;

        // setup the offsets
        int noNullValue = pstmt.inputDesc_[paramNumber].noNullValue_;
        int nullValue = pstmt.inputDesc_[paramNumber].nullValue_;
        int dataLength = pstmt.inputDesc_[paramNumber].maxLen_;

        boolean shortLength = precision < 0x7FFF;
        int dataOffset = shortLength ? 2 : 4;

        if (dataType == InterfaceResultSet.SQLTYPECODE_VARCHAR_WITH_LENGTH) {
            dataLength += dataOffset;

            if (dataLength % 2 != 0)
                dataLength++;
        }

        noNullValue = (noNullValue * paramRowCount) + (rowNumber * dataLength);

        if (nullValue != -1)
            nullValue = (nullValue * paramRowCount) + (rowNumber * 2);

        if (paramValue == null) {
            if (nullValue == -1) {
                throw Messages.createSQLException(pstmt.connection_.t2props, locale,
                        "null_parameter_for_not_null_column", new Integer(paramNumber));
            }

            // values[nullValue] = -1;
            Bytes.insertShort(values, nullValue, (short) -1, this.ic_.getByteSwap());
            return;
        }

        switch (dataType) {
            case InterfaceResultSet.SQLTYPECODE_CHAR:
                if (paramValue == null) {
                    // Note for future optimization. We can probably remove the next
                    // line,
                    // because the array is already initialized to 0.
                    Bytes.insertShort(values, noNullValue, (short) 0, this.ic_.getByteSwap());
                } else if (paramValue instanceof byte[]) {
                    tmpBarray = (byte[]) paramValue;
                } else if (paramValue instanceof String) {
                    String charSet = "";

                    try {
                        if (this.ic_.getISOMapping() == InterfaceUtilities.SQLCHARSETCODE_ISO88591
                                && !this.ic_.getEnforceISO() && dataCharSet == InterfaceUtilities.SQLCHARSETCODE_ISO88591)
                            charSet = ic_.t2props_.getIso88591EncodingOverride();
                        else
                        {
                            if(dataCharSet == InterfaceUtilities.SQLCHARSETCODE_UNICODE && this.ic_.getByteSwap()) {
                                charSet = "UTF-16BE";
                            }
                            else {
                                charSet = InterfaceUtilities.getCharsetName(dataCharSet);
                            }
                        }
                        tmpBarray = ((String) paramValue).getBytes(charSet);
                    } catch (Exception e) {
                        throw Messages.createSQLException(pstmt.connection_.t2props, locale, "unsupported_encoding",
                                charSet);
                    }
                } // end if (paramValue instanceof String)
                else {
                    throw Messages.createSQLException(pstmt.connection_.t2props, locale, "invalid_parameter_value",
                            "CHAR data should be either bytes or String for column: " + paramNumber);
                }

                //
                // We now have a byte array containing the parameter
                //

                dataLen = tmpBarray.length;
                if (maxLength >= dataLen) {
                    System.arraycopy(tmpBarray, 0, values, noNullValue, dataLen);
                    // Blank pad for rest of the buffer
                    if (maxLength > dataLen) {
                        if (dataCharSet == InterfaceUtilities.SQLCHARSETCODE_UNICODE) {
                            // pad with Unicode spaces (0x00 0x32)
                            int i2 = dataLen;
                            while (i2 < maxLength) {
                                values[noNullValue + i2] = (byte) ' ';
                                values[noNullValue + (i2 + 1)] = (byte) 0 ;
                                i2 = i2 + 2;
                            }
                        } else {
                            Arrays.fill(values, (noNullValue + dataLen), (noNullValue + maxLength), (byte) ' ');
                        }
                    }
                } else {
                    throw Messages.createSQLException(pstmt.connection_.t2props, locale, "invalid_string_parameter",
                            "CHAR input data is longer than the length for column: " + paramNumber);
                }

                break;
            case InterfaceResultSet.SQLTYPECODE_VARCHAR:
                if (paramValue instanceof byte[]) {
                    tmpBarray = (byte[]) paramValue;
                } else if (paramValue instanceof String) {
                    String charSet = "";

                    try {
                        if (this.ic_.getISOMapping() == InterfaceUtilities.SQLCHARSETCODE_ISO88591
                                && !this.ic_.getEnforceISO() && dataCharSet == InterfaceUtilities.SQLCHARSETCODE_ISO88591)
                            charSet = ic_.t2props_.getIso88591EncodingOverride();
                        else
                        {
                            if(dataCharSet == InterfaceUtilities.SQLCHARSETCODE_UNICODE && this.ic_.getByteSwap())
                                charSet = "UTF-16LE";
                            else
                                charSet = InterfaceUtilities.getCharsetName(dataCharSet);
                        }
                        tmpBarray = ((String) paramValue).getBytes(charSet);
                    } catch (Exception e) {
                        throw Messages.createSQLException(pstmt.connection_.t2props, locale, "unsupported_encoding",
                                charSet);
                    }

                } // end if (paramValue instanceof String)
                else {
                    throw Messages.createSQLException(pstmt.connection_.t2props, locale, "invalid_parameter_value",
                            "VARCHAR data should be either bytes or String for column: " + paramNumber);
                }

                dataLen = tmpBarray.length;
                if (maxLength > dataLen) {
                    Bytes.insertShort(values, noNullValue, (short) dataLen, this.ic_.getByteSwap());
                    System.arraycopy(tmpBarray, 0, values, noNullValue + 2, dataLen);
                } else {
                    throw Messages.createSQLException(pstmt.connection_.t2props, locale, "invalid_parameter_value",
                            "VARCHAR input data is longer than the length for column: " + paramNumber);
                }
                break;
            case InterfaceResultSet.SQLTYPECODE_DATETIME:
                Date tmpdate = null;
                switch (sqlDatetimeCode) {
                    case InterfaceResultSet.SQLDTCODE_DATE:
                        try {
                            if (((String) paramValue)
                                    .matches("(\\d{4}-\\d{1,2}-\\d{1,2})|(\\d{1,2}\\.\\d{1,2}\\.\\d{4})|(\\d{1,2}/\\d{1,2}/\\d{4})")) {
                                tmpdate = Date.valueOf((String) ((String) paramValue)
                                        .replaceFirst("(\\d{1,2})\\.(\\d{1,2})\\.(\\d{4})",	"$3-$2-$1")
                                        .replaceFirst("(\\d{1,2})/(\\d{1,2})/(\\d{4})",	"$3-$1-$2"));
                            }else{
                                throw new IllegalArgumentException();
                            }
                        } catch (IllegalArgumentException iex) {
                            throw Messages
                                .createSQLException(
                                        pstmt.connection_.t2props,
                                        locale,
                                        "invalid_parameter_value",
                                        "["+paramValue+"] Date format is incorrect or date value is invalide. "
                                        + "  Supported format: YYYY-MM-DD, MM/DD/YYYY, DD.MM.YYYY");
                        }
                        try {
                            byte[] temp1 = tmpdate.toString().getBytes("ASCII");
                            System.arraycopy(temp1, 0, values, noNullValue, temp1.length);
                        } catch (java.io.UnsupportedEncodingException e) {
                            Object[] messageArguments = new Object[1];
                            messageArguments[0] = e.getMessage();
                            throw Messages.createSQLException(pstmt.connection_.t2props, locale, "unsupported_encoding",
                                    messageArguments);
                        }
                        break;
                    case InterfaceResultSet.SQLDTCODE_TIMESTAMP:
                        Timestamp tmpts;
                        try {
                            tmpts = Timestamp.valueOf((String) paramValue);
                        } catch (IllegalArgumentException iex) {
                            throw Messages.createSQLException(pstmt.connection_.t2props, locale, "invalid_parameter_value",
                                    "Timestamp data format is incorrect for column: " + paramNumber + " = " + paramValue);
                        }

                        // ODBC precision is nano secs. JDBC precision is micro secs
                        // so substract 3 from ODBC precision.
                        maxLength = maxLength - 3;
                        try {
                            tmpBarray = tmpts.toString().getBytes("ASCII");
                        } catch (java.io.UnsupportedEncodingException e) {
                            Object[] messageArguments = new Object[1];
                            messageArguments[0] = e.getMessage();
                            throw Messages.createSQLException(pstmt.connection_.t2props, locale, "unsupported_encoding",
                                    messageArguments);
                        }
                        dataLen = tmpBarray.length;

                        if (maxLength > dataLen) {
                            System.arraycopy(tmpBarray, 0, values, noNullValue, dataLen);

                            // Don't know when we need this. padding blanks. Legacy??
                            Arrays.fill(values, (noNullValue + dataLen), (noNullValue + maxLength), (byte) ' ');
                        } else {
                            System.arraycopy(tmpBarray, 0, values, noNullValue, maxLength);
                        }
                        break;
                    case InterfaceResultSet.SQLDTCODE_TIME:
                        // If the OdbcDataType is equal to Types.Other, that means
                        // that this is HOUR_TO_FRACTION and should be treated
                        // as a Type.Other --> see in SQLDesc.java
                        if (OdbcDataType != java.sql.Types.OTHER) // do the processing
                            // for TIME
                        {
                            Time tmptime;
                            try {
                                if (paramValue instanceof byte[]) {
                                    tmptime = Time.valueOf(new String((byte[]) paramValue, "ASCII"));
                                } else {
                                    tmptime = Time.valueOf(paramValue.toString());
                                }
                                byte[] tempb1 = tmptime.toString().getBytes("ASCII");
                                System.arraycopy(tempb1, 0, values, noNullValue, tempb1.length);
                            } catch (IllegalArgumentException iex) {
                                throw Messages.createSQLException(pstmt.connection_.t2props, locale,
                                        "invalid_parameter_value", "Time data format is incorrect for column: " + paramNumber
                                        + " = " + paramValue);
                            } catch (java.io.UnsupportedEncodingException e) {
                                Object[] messageArguments = new Object[1];
                                messageArguments[0] = e.getMessage();
                                throw Messages.createSQLException(pstmt.connection_.t2props, locale, "unsupported_encoding",
                                        messageArguments);
                            }
                            break;
                        } else {
                            // SQLMXDesc.SQLDTCODE_HOUR_TO_FRACTION data type!!!
                            // let the next case structure handle it
                        }
                    case SQLMXDesc.SQLDTCODE_YEAR:
                    case SQLMXDesc.SQLDTCODE_YEAR_TO_MONTH:
                    case SQLMXDesc.SQLDTCODE_MONTH:
                    case SQLMXDesc.SQLDTCODE_MONTH_TO_DAY:
                    case SQLMXDesc.SQLDTCODE_DAY:
                    case SQLMXDesc.SQLDTCODE_HOUR:
                    case SQLMXDesc.SQLDTCODE_HOUR_TO_MINUTE:
                    case SQLMXDesc.SQLDTCODE_MINUTE:
                    case SQLMXDesc.SQLDTCODE_MINUTE_TO_SECOND:
                        // case SQLMXDesc.SQLDTCODE_MINUTE_TO_FRACTION:
                    case SQLMXDesc.SQLDTCODE_SECOND:
                        // case SQLMXDesc.SQLDTCODE_SECOND_TO_FRACTION:
                    case SQLMXDesc.SQLDTCODE_YEAR_TO_HOUR:
                    case SQLMXDesc.SQLDTCODE_YEAR_TO_MINUTE:
                    case SQLMXDesc.SQLDTCODE_MONTH_TO_HOUR:
                    case SQLMXDesc.SQLDTCODE_MONTH_TO_MINUTE:
                    case SQLMXDesc.SQLDTCODE_MONTH_TO_SECOND:
                        // case SQLMXDesc.SQLDTCODE_MONTH_TO_FRACTION:
                    case SQLMXDesc.SQLDTCODE_DAY_TO_HOUR:
                    case SQLMXDesc.SQLDTCODE_DAY_TO_MINUTE:
                    case SQLMXDesc.SQLDTCODE_DAY_TO_SECOND:
                        // case SQLMXDesc.SQLDTCODE_DAY_TO_FRACTION:
                    default:
                        if (paramValue instanceof String) {
                            try {
                                tmpBarray = ((String) paramValue).getBytes("ASCII");
                            } catch (Exception e) {
                                throw Messages.createSQLException(pstmt.connection_.t2props, locale, "unsupported_encoding",
                                        "ASCII");
                            }
                        } else if (paramValue instanceof byte[]) {
                            tmpBarray = (byte[]) paramValue;
                        } else {
                            throw Messages.createSQLException(pstmt.connection_.t2props, locale,
                                    "invalid_cast_specification", "DATETIME data should be either bytes or String for column: "
                                    + paramNumber);
                        }
                        dataLen = tmpBarray.length;
                        if (maxLength == dataLen) {
                            System.arraycopy(tmpBarray, 0, values, noNullValue, maxLength);
                        } else if (maxLength > dataLen) {
                            System.arraycopy(tmpBarray, 0, values, noNullValue, dataLen);

                            // Don't know when we need this. padding blanks. Legacy??
                            Arrays.fill(values, (noNullValue + dataLen), (noNullValue + maxLength), (byte) ' ');
                        } else {
                            throw Messages.createSQLException(pstmt.connection_.t2props, locale, "invalid_parameter_value",
                                    "DATETIME data longer than column length: " + paramNumber);
                        }
                        break;
                }
                break;
            case InterfaceResultSet.SQLTYPECODE_INTERVAL:
                if (paramValue instanceof byte[]) {
                    tmpBarray = (byte[]) paramValue;
                } else if (paramValue instanceof String) {
                    try {
                        tmpBarray = ((String) paramValue).getBytes("ASCII");
                    } catch (Exception e) {
                        throw Messages.createSQLException(pstmt.connection_.t2props, locale, "unsupported_encoding",
                                "ASCII");
                    }
                } else {
                    throw Messages.createSQLException(pstmt.connection_.t2props, locale, "invalid_cast_specification",
                            "INTERVAL data should be either bytes or String for column: " + paramNumber);
                }

                dataLen = tmpBarray.length;
                if (maxLength >= dataLen) {
                    dataLen = tmpBarray.length;
                    if (maxLength == dataLen) {
                        System.arraycopy(tmpBarray, 0, values, noNullValue, maxLength);
                    } else if (maxLength > dataLen) {
                        System.arraycopy(tmpBarray, 0, values, noNullValue, dataLen);

                        // Don't know when we need this. padding blanks. Legacy??
                        Arrays.fill(values, (noNullValue + dataLen), (noNullValue + maxLength), (byte) ' ');
                    }
                } else {
                    throw Messages.createSQLException(pstmt.connection_.t2props, locale, "invalid_parameter_value",
                            "INTERVAL data longer than column length: " + paramNumber);
                }

                break;
            case InterfaceResultSet.SQLTYPECODE_VARCHAR_WITH_LENGTH:
            case InterfaceResultSet.SQLTYPECODE_VARCHAR_LONG:
                if (paramValue instanceof byte[]) {
                    tmpBarray = (byte[]) paramValue;
                } else if (paramValue instanceof String) {
                    String charSet = "";

                    try {
                        if (this.ic_.getISOMapping() == InterfaceUtilities.SQLCHARSETCODE_ISO88591
                                && !this.ic_.getEnforceISO() && dataCharSet == InterfaceUtilities.SQLCHARSETCODE_ISO88591)
                            charSet = ic_.t2props_.getIso88591EncodingOverride();
                        else
                        {
                            if(dataCharSet == InterfaceUtilities.SQLCHARSETCODE_UNICODE && this.ic_.getByteSwap())
                                charSet = "UTF-16LE";
                            else
                                charSet = InterfaceUtilities.getCharsetName(dataCharSet);
                        }
                        tmpBarray = ((String) paramValue).getBytes(charSet);
                    } catch (Exception e) {
                        throw Messages.createSQLException(pstmt.connection_.t2props, locale, "unsupported_encoding",
                                charSet);
                    }
                } // end if (paramValue instanceof String)
                else {
                    throw Messages.createSQLException(pstmt.connection_.t2props, locale, "invalid_cast_specification",
                            "VARCHAR data should be either bytes or String for column: " + paramNumber);
                }

                dataLen = tmpBarray.length;
                if (maxLength > (dataLen + dataOffset)) {
                    maxLength = dataLen + dataOffset;

                    // TODO: should this be swapped?!
                    if(shortLength) {
                        System.arraycopy(Bytes.createShortBytes((short) dataLen, this.ic_.getByteSwap()), 0, values,
                                noNullValue, dataOffset);
                    } else {
                        System.arraycopy(Bytes.createIntBytes(dataLen, this.ic_.getByteSwap()), 0, values,
                                noNullValue, dataOffset);
                    }

                    System.arraycopy(tmpBarray, 0, values, (noNullValue + dataOffset), dataLen);
                } else {
                    throw Messages.createSQLException(pstmt.connection_.t2props, locale, "invalid_string_parameter",
                            "VARCHAR data longer than column length: " + paramNumber);
                }
                break;
            case InterfaceResultSet.SQLTYPECODE_INTEGER:
                tmpbd = Utility.getBigDecimalValue(locale, paramValue);
                if (scale > 0) {
                    tmpbd = tmpbd.movePointRight(scale);
                }

                // data truncation check
                if (pstmt.roundingMode_ == BigDecimal.ROUND_UNNECESSARY) {
                    Utility.checkLongTruncation(paramNumber, tmpbd);

                }
                Utility.checkIntegerBoundary(locale, tmpbd);

                // check boundary condition for Numeric.
                Utility.checkDecimalBoundary(locale, tmpbd, precision);

                Bytes.insertInt(values, noNullValue, tmpbd.intValue(), this.ic_.getByteSwap());
                break;
            case InterfaceResultSet.SQLTYPECODE_INTEGER_UNSIGNED:
                tmpbd = Utility.getBigDecimalValue(locale, paramValue);
                if (scale > 0) {
                    tmpbd = tmpbd.movePointRight(scale);
                }

                // data truncation check
                if (pstmt.roundingMode_ == BigDecimal.ROUND_UNNECESSARY) {
                    Utility.checkLongTruncation(paramNumber, tmpbd);

                    // range checking
                }
                Utility.checkUnsignedIntegerBoundary(locale, tmpbd);

                // check boundary condition for Numeric.
                Utility.checkDecimalBoundary(locale, tmpbd, precision);

                Bytes.insertInt(values, noNullValue, tmpbd.intValue(), this.ic_.getByteSwap());
                break;
            case InterfaceResultSet.SQLTYPECODE_SMALLINT:
                tmpbd = Utility.getBigDecimalValue(locale, paramValue);
                if (scale > 0) {
                    /* No necessary to move point, it's moved already in set method*/
                }

                // data truncation check
                if (pstmt.roundingMode_ == BigDecimal.ROUND_UNNECESSARY) {
                    Utility.checkLongTruncation(paramNumber, tmpbd);

                    // range checking
                }
                Utility.checkShortBoundary(locale, tmpbd);

                // check boundary condition for Numeric.
                Utility.checkDecimalBoundary(locale, tmpbd, precision);

                Bytes.insertShort(values, noNullValue, tmpbd.shortValue(), this.ic_.getByteSwap());
                break;
            case InterfaceResultSet.SQLTYPECODE_SMALLINT_UNSIGNED:
                tmpbd = Utility.getBigDecimalValue(locale, paramValue);
                if (scale > 0) {
                    tmpbd = tmpbd.movePointRight(scale);
                }

                // data truncation check
                if (pstmt.roundingMode_ == BigDecimal.ROUND_UNNECESSARY) {
                    Utility.checkLongTruncation(paramNumber, tmpbd);

                    // range checking
                }
                Utility.checkSignedShortBoundary(locale, tmpbd);

                // check boundary condition for Numeric.
                Utility.checkDecimalBoundary(locale, tmpbd, precision);

                Bytes.insertShort(values, noNullValue, tmpbd.shortValue(), this.ic_.getByteSwap());
                break;
            case InterfaceResultSet.SQLTYPECODE_LARGEINT:
                tmpbd = Utility.getBigDecimalValue(locale, paramValue);

                if (scale > 0) {
                    tmpbd = tmpbd.movePointRight(scale);

                    // check boundary condition for Numeric.
                }
                Utility.checkDecimalBoundary(locale, tmpbd, precision);
                Bytes.insertLong(values, noNullValue, tmpbd.longValue(), this.ic_.getByteSwap());
                break;
            case InterfaceResultSet.SQLTYPECODE_DECIMAL:
            case InterfaceResultSet.SQLTYPECODE_DECIMAL_UNSIGNED:

                // create an parameter with out "."
                try {
                    tmpbd = Utility.getBigDecimalValue(locale, paramValue);
                    if (scale > 0) {
                        // do nothing for now because it's point moved already.
                    }

                    tmpbd = Utility.setScale(tmpbd, scale, pstmt.roundingMode_);

                    // data truncation check.
                    if (pstmt.roundingMode_ == BigDecimal.ROUND_UNNECESSARY) {
                        Utility.checkLongTruncation(paramNumber, tmpbd);

                        // get only the mantissa part
                    }
                    try {
                        tmpBarray = String.valueOf(tmpbd.longValue()).getBytes("ASCII");
                    } catch (java.io.UnsupportedEncodingException e) {
                        Object[] messageArguments = new Object[1];
                        messageArguments[0] = e.getMessage();
                        throw Messages.createSQLException(pstmt.connection_.t2props, locale, "unsupported_encoding",
                                messageArguments);
                    }
                } catch (NumberFormatException nex) {
                    throw Messages.createSQLException(pstmt.connection_.t2props, locale, "invalid_parameter_value",
                            "DECIMAL data format incorrect for column: " + paramNumber + ". Error is: " + nex.getMessage());
                }

                dataLen = tmpBarray.length;

                // pad leading zero's if datalen < maxLength
                int desPos = 0;
                int srcPos = 0;
                boolean minus = false;

                // check if data is negative.
                if (tmpbd.signum() == -1) {
                    minus = true;
                    srcPos++;
                    dataLen--;
                }

                // pad beginning 0 for empty space.
                int numOfZeros = maxLength - dataLen;

                // DataTruncation is happening.
                if (numOfZeros < 0) {
                    throw Messages.createSQLException(pstmt.connection_.t2props, locale, "data_truncation_exceed", new int[]{dataLen, maxLength});
                }

                for (i = 0; i < numOfZeros; i++) {
                    values[noNullValue + desPos] = (byte) '0';
                    desPos = desPos + 1;
                }
                System.arraycopy(tmpBarray, srcPos, values, noNullValue + desPos, dataLen);

                // handling minus sign in decimal. OR -80 with the first byte for
                // minus
                if (minus) {
                    values[noNullValue] = (byte) ((byte) (-80) | values[noNullValue]);
                }
                break;
            case InterfaceResultSet.SQLTYPECODE_REAL:
                tmpbd = Utility.getBigDecimalValue(locale, paramValue);
                Utility.checkFloatBoundary(locale, tmpbd);
                float fvalue = tmpbd.floatValue();
                int bits = Float.floatToIntBits(fvalue);

                Bytes.insertInt(values, noNullValue, bits, this.ic_.getByteSwap());
                break;
            case InterfaceResultSet.SQLTYPECODE_FLOAT:
                tmpbd = Utility.getBigDecimalValue(locale, paramValue);
                Utility.checkFloatBoundary(locale, tmpbd);
                Bytes.insertLong(values, noNullValue, Double.doubleToLongBits(tmpbd.doubleValue()), this.ic_.getByteSwap());
                break;
            case InterfaceResultSet.SQLTYPECODE_DOUBLE:
                tmpbd = Utility.getBigDecimalValue(locale, paramValue);
                Utility.checkDoubleBoundary(locale, tmpbd);
                Bytes.insertLong(values, noNullValue, Double.doubleToLongBits(tmpbd.doubleValue()), this.ic_.getByteSwap());
                break;
            case InterfaceResultSet.SQLTYPECODE_NUMERIC:
            case InterfaceResultSet.SQLTYPECODE_NUMERIC_UNSIGNED:
                tmpbd = Utility.getBigDecimalValue(locale, paramValue);
                byte[] b = InterfaceUtilities.convertBigDecimalToSQLBigNum(tmpbd, maxLength, scale);
                System.arraycopy(b, 0, values, noNullValue, maxLength);
                break;
                // You will not get this type, since server internally converts it
                // SMALLINT, INTERGER or LARGEINT
            case InterfaceResultSet.SQLTYPECODE_DECIMAL_LARGE:
            case InterfaceResultSet.SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
            case InterfaceResultSet.SQLTYPECODE_BIT:
            case InterfaceResultSet.SQLTYPECODE_BITVAR:
            case InterfaceResultSet.SQLTYPECODE_BPINT_UNSIGNED:
            default:
                throw Messages.createSQLException(pstmt.connection_.t2props, locale, "restricted_data_type", null);
        }
    } // end convertObjectToSQL2

    private SQLWarningOrError[] mergeErrors(SQLWarningOrError[] client, SQLWarningOrError[] server) {
        SQLWarningOrError[] target = new SQLWarningOrError[client.length + server.length];

        int si = 0; // server index
        int ci = 0; // client index
        int ti = 0; // target index

        int sr; // server rowId
        int cr; // client rowId

        int so = 0; // server offset

        while (ci < client.length && si < server.length) {
            cr = client[ci].rowId;
            sr = server[si].rowId + so;

            if (cr <= sr || server[si].rowId == 0) {
                so++;
                target[ti++] = client[ci++];
            } else {
                server[si].rowId += so;
                target[ti++] = server[si++];
            }
        }

        // we only have one array left
        while (ci < client.length) {
            target[ti++] = client[ci++];
        }

        while (si < server.length) {
            if (server[si].rowId != 0)
                server[si].rowId += so;
            target[ti++] = server[si++];
        }

        return target;
    }

    SQL_DataValue_def fillInSQLValues2(Locale locale, SQLMXStatement stmt, int paramRowCount, int paramCount,
            Object[] paramValues, ArrayList clientErrors) throws SQLException

    {
        SQL_DataValue_def dataValue = new SQL_DataValue_def();

        if (paramRowCount == 0 && paramValues != null && paramValues.length > 0)
            paramRowCount = 1; // fake a single row if we are doing inputParams
        // for an SPJ

        // TODO: we should really figure out WHY this could happen
        if (stmt.inputParamsLength_ < 0) {
            dataValue.buffer = new byte[0];
            dataValue.length = 0;
        } else {
            int bufLen = stmt.inputParamsLength_ * paramRowCount;

            dataValue.buffer = new byte[bufLen];

            for (int row = 0; row < paramRowCount; row++) {
                for (int col = 0; col < paramCount; col++) {
                    try {
                        convertObjectToSQL2(locale, stmt, paramValues[row * paramCount + col], paramRowCount, col,
                                dataValue.buffer, row - clientErrors.size());
                    } catch (TrafT2Exception e) {
                        if (paramRowCount == 1) // for single rows we need to
                            // throw immediately
                            throw e;

                        clientErrors.add(new SQLWarningOrError(row + 1, e.getErrorCode(), e.getMessage(), e
                                    .getSQLState()));
                        break; // skip the rest of the row
                    }
                }
            }

            // fix the column offsets if we had errors
            if (clientErrors.size() > 0) {
                int oldOffset;
                int newOffset;
                int noNullValue;
                int nullValue;
                int colLength;
                int dataType;

                for (int i = 1; i < paramCount; i++) // skip the first col
                {
                    noNullValue = stmt.inputDesc_[i].noNullValue_;
                    nullValue = stmt.inputDesc_[i].nullValue_;
                    colLength = stmt.inputDesc_[i].maxLen_;
                    dataType = stmt.inputDesc_[i].dataType_;
                    if (dataType == InterfaceResultSet.SQLTYPECODE_VARCHAR_WITH_LENGTH
                            || dataType == InterfaceResultSet.SQLTYPECODE_VARCHAR_LONG
                            || dataType == InterfaceResultSet.SQLTYPECODE_VARCHAR) {
                        int dataOffset = (colLength < 0x7FFF) ? 2 : 4;
                        colLength += dataOffset;

                        if (colLength % 2 != 0)
                            colLength++;
                            }

                    if (nullValue != -1) {
                        oldOffset = nullValue * paramRowCount;
                        newOffset = oldOffset - (nullValue * clientErrors.size());
                        System.arraycopy(dataValue.buffer, oldOffset, dataValue.buffer, newOffset,
                                2 * (paramRowCount - clientErrors.size()));
                    }

                    oldOffset = noNullValue * paramRowCount;
                    newOffset = oldOffset - (noNullValue * clientErrors.size());
                    System.arraycopy(dataValue.buffer, oldOffset, dataValue.buffer, newOffset, colLength
                            * (paramRowCount - clientErrors.size()));
                }
            }

            dataValue.length = stmt.inputParamsLength_ * (paramRowCount - clientErrors.size());
        }
        return dataValue;
    }

    boolean hasParameters(String sql) {
        boolean foundParam = false;

        String[] s = sql.split("\"[^\"]*\"|'[^']*'");
        for (int i = 0; i < s.length; i++) {
            if (s[i].indexOf('?') != -1) {
                foundParam = true;
                break;
            }
        }

        return foundParam;
    }

    /**
     * Get the transaction status
     * 
     * @param sql
     * @return
     */
    private short getTransactionStatus(String sql) {
        String tokens[] = sql.split("[^a-zA-Z]+", 3);
        short rt1 = 0;
        if (tokens.length > 1 && tokens[1].equalsIgnoreCase("WORK")) {
            if (tokens[0].equalsIgnoreCase("BEGIN"))
                rt1 = TRANSPORT.TYPE_BEGIN_TRANSACTION;
        } else if (tokens[0].equalsIgnoreCase("COMMIT")
                || tokens[0].equalsIgnoreCase("ROLLBACK")) {
            rt1 = TRANSPORT.TYPE_END_TRANSACTION;
                }

        return rt1;
    }

    // -------------------------------------------------------------
    //TODO: this whole function needs to be rewritten
    short getSqlStmtType(String str) {
        // 7708
        stmtIsLock = false;

        String tokens[] = str.split("[^a-zA-Z]+", 3);
        short rt1 = TRANSPORT.TYPE_UNKNOWN;
        String str3 = "";

        if (tokens[0].length() > 0)
            str3 = tokens[0].toUpperCase();
        else
            str3 = tokens[1].toUpperCase();

        if(str3.equals("JOINXATXN")){
            rt1 = TRANSPORT.TYPE_SELECT;
        }
        else if(str3.equals("WMSOPEN")) {
            rt1 = TRANSPORT.TYPE_QS_OPEN;
        }
        else if(str3.equals("WMSCLOSE")) {
            rt1 = TRANSPORT.TYPE_QS_CLOSE;
        }
        else if(str3.equals("CMDOPEN")) {
            rt1 = TRANSPORT.TYPE_CMD_OPEN;
        }
        else if(str3.equals("CMDCLOSE")) {
            rt1 = TRANSPORT.TYPE_CMD_CLOSE;
        }
        else {
            switch(this.ic_.getMode()) {
                case InterfaceConnection.MODE_SQL:
                    if ((str3.equals("SELECT")) || (str3.equals("SHOWSHAPE")) || (str3.equals("INVOKE"))
                            || (str3.equals("SHOWCONTROL")) || (str3.equals("SHOWDDL")) || (str3.equals("EXPLAIN"))
                            || (str3.equals("SHOWPLAN")) || (str3.equals("REORGANIZE")) || (str3.equals("MAINTAIN"))
                            || (str3.equals("SHOWLABEL")) || (str3.equals("VALUES"))
                            || (str3.equals("REORG")) || (str3.equals("SEL")) || (str3.equals("GET")) || (str3.equals("SHOWSTATS"))
                            || str3.equals("GIVE") || str3.equals("STATUS") || str3.equals("INFO") || str3.equals("LIST")
                       ) {
                        rt1 = TRANSPORT.TYPE_SELECT;
                       }
                    else if (str3.equals("UPDATE") || str3.equals("MERGE")) {
                        rt1 = TRANSPORT.TYPE_UPDATE;
                    } else if (str3.equals("DELETE") || str3.equals("STOP") || str3.equals("START")) {
                        rt1 = TRANSPORT.TYPE_DELETE;
                    } else if (str3.equals("INSERT") || str3.equals("INS") || str3.equals("UPSERT")) {
                        if (hasParameters(str)) {
                            rt1 = TRANSPORT.TYPE_INSERT_PARAM;
                        } else {
                            rt1 = TRANSPORT.TYPE_INSERT;
                        }
                    }
                    else if (str3.equals("CREATE")) {
                        rt1 = TRANSPORT.TYPE_CREATE;
                    } else if (str3.equals("GRANT")) {
                        rt1 = TRANSPORT.TYPE_GRANT;
                    } else if (str3.equals("DROP")) {
                        rt1 = TRANSPORT.TYPE_DROP;
                    } else if (str3.equals("CALL")) {
                        rt1 = TRANSPORT.TYPE_CALL;
                    } else if (str3.equals("EXPLAIN")) {
                        rt1 = TRANSPORT.TYPE_EXPLAIN;
                    } else if (str3.equals("INFOSTATS")) {
                        rt1 = TRANSPORT.TYPE_STATS;
                    }
                    break;
                case InterfaceConnection.MODE_WMS:
                    if (str3.equals("STATUS") || str3.equals("INFO")) {
                        rt1 = TRANSPORT.TYPE_SELECT;
                    }
                    break;
                case InterfaceConnection.MODE_CMD:
                    if (str3.equals("STATUS") || str3.equals("INFO") || str3.equals("LIST")) {
                        rt1 = TRANSPORT.TYPE_SELECT;
                    }
                    else if(str3.equals("ADD") || str3.equals("ALTER")) {
                        rt1 = TRANSPORT.TYPE_INSERT;
                    }
                    else if(str3.equals("DELETE") || str3.equals("STOP") || str3.equals("START")) {
                        rt1 = TRANSPORT.TYPE_DELETE;
                    }
                    break;
            }
        }

        return rt1;

    } // end getSqlStmtType

    // -------------------------------------------------------------
    long getRowCount() {
        return rowCount_;
    }

    // -------------------------------------------------------------
    void setRowCount(long rowCount) {
        if (rowCount < 0) {
            rowCount_ = -1;
        } else {
            rowCount_ = rowCount;
        }
    }

    // -------------------------------------------------------------
    static SQLMXDesc[] NewDescArray(Descriptor2[] descArray) {
        int index;
        SQLMXDesc[] SQLMXDescArray;
        Descriptor2 desc;

        if (descArray == null || descArray.length == 0) {
            return null;
        }

        SQLMXDescArray = new SQLMXDesc[descArray.length];

        for (index = 0; index < descArray.length; index++) {
            desc = descArray[index];
            boolean nullInfo = false;
            boolean signType = false;

            if (desc.nullInfo_ != 0) {
                nullInfo = true;
            }
            if (desc.signed_ != 0) {
                signType = true;

            }
            SQLMXDescArray[index] = new SQLMXDesc(desc.noNullValue_, desc.nullValue_, desc.version_, desc.dataType_,
                    (short) desc.datetimeCode_, desc.maxLen_, (short) desc.precision_, (short) desc.scale_, nullInfo,
                    signType, desc.odbcDataType_, desc.odbcPrecision_, desc.sqlCharset_, desc.odbcCharset_,
                    desc.colHeadingNm_, desc.tableName_, desc.catalogName_, desc.schemaName_, desc.headingName_,
                    desc.intLeadPrec_, desc.paramMode_, desc.fsDataType_, desc.getRowLength());
        }
        return SQLMXDescArray;
    }

    // -------------------------------------------------------------
    // Interface methods
    void executeDirect(int queryTimeout, SQLMXStatement stmt) throws SQLException {
        GenericReply gr = null;

        stmt.operationReply_ = gr.replyBuffer;

    } // end executeDirect

    // --------------------------------------------------------------------------
    int close() throws SQLException {
        int rval = 0;
        CloseReply cry_ = null;

        switch (cry_.m_p1.exception_nr) {
            case TRANSPORT.CEE_SUCCESS:

                // ignore the SQLWarning for the static close
                break;
            case odbc_SQLSvc_Close_exc_.odbc_SQLSvc_Close_SQLError_exn_:
                Messages.throwSQLException(stmt_.connection_.t2props, cry_.m_p1.SQLError);
            default:
                throw Messages.createSQLException(stmt_.connection_.t2props, ic_.getLocale(), "ids_unknown_reply_error",
                        null);
        } // end switch

        return cry_.m_p2; // rowsAffected
    } // end close

    // --------------------------------------------------------------------------
    // Interface methods for prepared statement
    void prepare(String sql, int queryTimeout, SQLMXPreparedStatement pstmt) throws SQLException {
        int sqlAsyncEnable = 0;
        this.stmtType_ = this.EXTERNAL_STMT;
        this.sqlStmtType_ = getSqlStmtType(sql);
        this.setTransactionStatus(pstmt.connection_, sql);
        int stmtLabelCharset = 1;
        String cursorName = pstmt.cursorName_;
        int cursorNameCharset = 1;
        String moduleName = pstmt.moduleName_;
        int moduleNameCharset = 1;
        long moduleTimestamp = pstmt.moduleTimestamp_;
        String sqlString = sql;
        int sqlStringCharset = 1;
        String stmtOptions = "";
        int maxRowsetSize = pstmt.getMaxRows();

        byte[] txId;

        txId = Bytes.createIntBytes(0, false);

        if (sqlStmtType_ == TRANSPORT.TYPE_STATS) {
            throw Messages.createSQLException(pstmt.connection_.t2props, ic_.getLocale(), "infostats_invalid_error",
                    null);
        } else if (sqlStmtType_ == TRANSPORT.TYPE_CONFIG) {
            throw Messages.createSQLException(pstmt.connection_.t2props, ic_.getLocale(),
                    "config_cmd_invalid_error", null);
        }

        PrepareReply pr = inStmt_.Prepare(sqlAsyncEnable, (short) this.stmtType_, this.sqlStmtType_, this.stmtHandle_,
                this.stmtLabel_, stmtLabelCharset, cursorName, cursorNameCharset, moduleName, moduleNameCharset,
                moduleTimestamp, sqlString, sqlStringCharset, stmtOptions, maxRowsetSize, txId);

        pr_ = pr;
        this.sqlQueryType_ = pr.sqlQueryType;

        switch (pr.returnCode) {
            case TRANSPORT.SQL_SUCCESS:
            case TRANSPORT.SQL_SUCCESS_WITH_INFO:
                this.stmtHandle_ = pr.stmtHandle;
                SQLMXDesc[] OutputDesc = InterfaceStatement.NewDescArray(pr.outputDesc);
                SQLMXDesc[] InputDesc = InterfaceStatement.NewDescArray(pr.inputDesc);

                pstmt.setPrepareOutputs2(InputDesc, OutputDesc, pr.inputNumberParams, pr.outputNumberParams,
                        pr.inputParamLength, pr.outputParamLength, pr.inputDescLength, pr.outputDescLength, this.stmtHandle_);

                if (pr.errorList != null && pr.errorList.length > 0) {
                    Messages.setSQLWarning(stmt_.connection_.t2props, pstmt, pr.errorList);
                }
                break;
            case odbc_SQLSvc_Prepare_exc_.odbc_SQLSvc_Prepare_SQLError_exn_:
            default:
                Messages.throwSQLException(stmt_.connection_.t2props, pr.errorList);
        }

    };

    // used to keep the same transaction inside an SPJ. we call out to the UDR
    // server and use their transaction for all executes.
    byte[] getUDRTransaction(boolean swapBytes) throws SQLException {
        byte[] ret = null;

        try {
            // To get references to method
            InterfaceStatement.LmUtility_class_ = Class.forName("com.tandem.sqlmx.LmUtility");
            InterfaceStatement.LmUtility_getTransactionId_ = InterfaceStatement.LmUtility_class_.getMethod(
                    "getTransactionId", new Class[] {});

            // To invoke the method
            short[] tId = (short[]) InterfaceStatement.LmUtility_getTransactionId_.invoke(null, new Object[] {});

            ret = new byte[tId.length * 2];

            for (int i = 0; i < tId.length; i++) {
                Bytes.insertShort(ret, i * 2, tId[i], swapBytes);
            }
        } catch (Exception e) {
            String s = e.toString() + "\r\n";
            StackTraceElement[] st = e.getStackTrace();

            for (int i = 0; i < st.length; i++) {
                s += st[i].toString() + "\r\n";
            }

            throw new SQLException(s);
        }

        return ret;
    }

    // -------------------------------------------------------------------
    void execute(short executeAPI, int paramRowCount, int paramCount, Object[] paramValues, int queryTimeout
            , String sql, SQLMXStatement stmt // executeDirect
            ) throws SQLException {
        cursorName_ = stmt.cursorName_;
        rowCount_ = 0;

        int sqlAsyncEnable = (stmt.getResultSetHoldability() == SQLMXResultSet.HOLD_CURSORS_OVER_COMMIT) ? 1 : 0;
        int inputRowCnt = paramRowCount;
        int maxRowsetSize = stmt.getMaxRows();
        String sqlString = (sql == null) ? stmt.getSQL() : sql;
        int sqlStringCharset = 1;
        int cursorNameCharset = 1;
        int stmtLabelCharset = 1;
        byte[] txId;
        ArrayList clientErrors = new ArrayList();

        if (stmt.transactionToJoin != null)
            txId = stmt.transactionToJoin;
        else if (stmt.connection_.transactionToJoin != null)
            txId = stmt.connection_.transactionToJoin;
        else
            txId = Bytes.createIntBytes(0, false); // 0 length, no data

        SQL_DataValue_def inputDataValue;
        SQLValueList_def inputValueList = new SQLValueList_def();
        byte[] inputParams = null;

        if (executeAPI == TRANSPORT.SRVR_API_SQLEXECDIRECT) {
            sqlStmtType_ = getSqlStmtType(sql);
            setTransactionStatus(stmt.connection_, sql);
            stmt.outputDesc_ = null; // clear the output descriptors
        }

        if (stmt.usingRawRowset_ == true) {
            executeAPI = TRANSPORT.SRVR_API_SQLEXECUTE2;
            inputDataValue = new SQL_DataValue_def();
            inputDataValue.userBuffer = stmt.rowwiseRowsetBuffer_;
            inputDataValue.length = stmt.rowwiseRowsetBuffer_.limit() - 4;

            if (this.sqlQueryType_ == 16) // use the param values
            {
                try {
                    inputRowCnt = Integer.parseInt(paramValues[0].toString());
                    maxRowsetSize = Integer.parseInt(paramValues[1].toString());
                } catch (Exception e) {
                    throw new SQLException(
                            "Error setting inputRowCnt and maxRowsetSize.  Parameters not set or invalid.");
                }
            } else {
                inputRowCnt = paramRowCount - 1;
            }
        } else { 
            inputDataValue = fillInSQLValues2(ic_.getLocale(), stmt, inputRowCnt, paramCount, paramValues, clientErrors);
        }

        ExecuteReply er = inStmt_.Execute(executeAPI, sqlAsyncEnable, inputRowCnt - clientErrors.size(),
                maxRowsetSize, this.sqlStmtType_, this.stmtHandle_, sqlString, sqlStringCharset, this.cursorName_,
                cursorNameCharset, stmt.getStmtLabel_(), stmtLabelCharset, inputDataValue, inputValueList, txId,
                stmt.usingRawRowset_);

        if (executeAPI == TRANSPORT.SRVR_API_SQLEXECDIRECT) {
            this.sqlQueryType_ = er.queryType;
        }

        if (clientErrors.size() > 0) {
            if (er.errorList == null)
                er.errorList = (SQLWarningOrError[]) clientErrors.toArray(new SQLWarningOrError[clientErrors.size()]);
            else
                er.errorList = mergeErrors((SQLWarningOrError[]) clientErrors
                        .toArray(new SQLWarningOrError[clientErrors.size()]), er.errorList);
        }

        //stmt_.result_set_offset = 0;
        rowCount_ = er.rowsAffected;
        stmt.setRowCount64(rowCount_);

        int numStatus = inputRowCnt;

        if (numStatus < 1)
        {
            numStatus = 1;
        }

        stmt.batchRowCount_ = new int[numStatus];
        boolean batchException = false;	//3164

        if (er.returnCode == TRANSPORT.SQL_SUCCESS || er.returnCode == TRANSPORT.SQL_SUCCESS_WITH_INFO
                || er.returnCode == TRANSPORT.NO_DATA_FOUND) {
            Arrays.fill(stmt.batchRowCount_, -2); // fill with success

            if (er.errorList != null) // if we had errors with valid rowIds,
                // update the array
                {
                    for (int i = 0; i < er.errorList.length; i++) {
                        int row = er.errorList[i].rowId - 1;
                        if (row >= 0 && row < stmt.batchRowCount_.length) {
                            stmt.batchRowCount_[row] = -3;
                            batchException = true;	//3164
                        }
                    }
            }

            //set the statement mode as the command succeeded
            if (sqlStmtType_ == TRANSPORT.TYPE_QS_OPEN) {
                this.ic_.setMode(InterfaceConnection.MODE_WMS);
            } else if (sqlStmtType_ == TRANSPORT.TYPE_QS_CLOSE) {
                this.ic_.setMode(InterfaceConnection.MODE_SQL);
            } else if(sqlStmtType_ == TRANSPORT.TYPE_CMD_OPEN) {
                this.ic_.setMode(InterfaceConnection.MODE_CMD);
            } else if(sqlStmtType_ == TRANSPORT.TYPE_CMD_CLOSE) {
                this.ic_.setMode(InterfaceConnection.MODE_SQL);
            }

            // set the statement label if we didnt get one back.
            if (er.stmtLabels == null || er.stmtLabels.length == 0) {
                er.stmtLabels = new String[1];
                er.stmtLabels[0] = stmt.getStmtLabel_();
            }

            // get the descriptors from the proper location
            SQLMXDesc[][] desc = null;

            // try from execute data first
            if (er.outputDesc != null && er.outputDesc.length > 0) {
                desc = new SQLMXDesc[er.outputDesc.length][];

                for (int i = 0; i < er.outputDesc.length; i++) {
                    desc[i] = InterfaceStatement.NewDescArray(er.outputDesc[i]);
                }
            }
            // try from the prepare data
            else if (stmt.outputDesc_ != null && stmt.outputDesc_.length > 0) {
                desc = new SQLMXDesc[1][];
                desc[0] = stmt.outputDesc_;
            }

            if (this.sqlStmtType_ == TRANSPORT.TYPE_CALL) {
                /* This should be implemented for SPJ calls */
                /*
                   SQLMXCallableStatement cstmt = (SQLMXCallableStatement) stmt;
                   Object[] outputValueArray;
                   if(er.returnCode == TRANSPORT.NO_DATA_FOUND) { //this should really only happen with LAST0 specified
                   outputValueArray = new Object[cstmt.outputDesc_.length];
                   }
                   else {
                   outputValueArray = InterfaceResultSet.getExecute2Outputs(cstmt.connection_, cstmt.outputDesc_, 
                   er.outValues, this.ic_.getByteSwap());
                   }

                   cstmt.setExecuteCallOutputs(outputValueArray, (short) er.rowsAffected);
                   */
                //stmt.setMultipleResultSets(er.numResultSets, desc, er.stmtLabels, er.proxySyntax);
            } else {
                // fix until we start returning numResultsets for more than just
                // SPJs
                if (desc != null && desc.length > 0 && er.numResultSets == 0) {
                    er.numResultSets = 1;
                }

                /* * This should be implemented for rowset fetch */
                if (er.outValues != null && er.outValues.length > 0) {
                    stmt.setExecute2Outputs(er.outValues, (short) er.rowsAffected, false, er.proxySyntax, desc[0]);
                } else {
                    stmt.setMultipleResultSets(er.numResultSets, desc, er.stmtLabels, er.proxySyntax);
                }
            }
            if (er.errorList != null) {
                Messages.setSQLWarning(stmt_.connection_.t2props, stmt, er.errorList);
            }
        } else {
            Arrays.fill(stmt.batchRowCount_, -3); // fill with failed
            Messages.throwSQLException(stmt_.connection_.t2props, er.errorList);
        }
        //3164
        if (batchException) {
            Messages.throwSQLException(stmt_.connection_.t2props, er.errorList);
        }
            }

    protected void setTransactionStatus(SQLMXConnection conn, String sql) {
        short tranStatus = getTransactionStatus(sql);
        if(tranStatus == TRANSPORT.TYPE_BEGIN_TRANSACTION){
            conn.setBeginTransaction(true);
        }else if (tranStatus == TRANSPORT.TYPE_END_TRANSACTION){
            conn.setBeginTransaction(false);
        }

    }

} // end class InterfaceStatement

