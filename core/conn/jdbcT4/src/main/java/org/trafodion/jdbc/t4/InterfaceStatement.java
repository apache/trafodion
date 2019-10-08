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

import java.io.IOException;
import java.io.InputStream;
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
import java.util.regex.Matcher;
import java.util.regex.Pattern;

class InterfaceStatement {
	InterfaceConnection ic_;
	private long rowCount_;
	static final short SQL_DROP = 1;
	static short EXTERNAL_STMT = 0;
	int sqlStmtType_ = TRANSPORT.TYPE_UNKNOWN;
	int stmtType_ = 0;
	T4Statement t4statement_;
	int queryTimeout_;
	String stmtLabel_;
	String cursorName_;
	TrafT4Statement stmt_;

	int sqlQueryType_;
	int stmtHandle_;
	int estimatedCost_;
	boolean prepare2 = false;

	// used for SPJ transaction
	static Class LmUtility_class_ = null;
	static java.lang.reflect.Method LmUtility_getTransactionId_ = null;

	PrepareReply pr_;

	// ----------------------------------------------------------------------
	InterfaceStatement(TrafT4Statement stmt) throws SQLException {
		this.ic_ = ((TrafT4Connection) stmt.getConnection()).getServerHandle();
		queryTimeout_ = stmt.queryTimeout_;
		stmtLabel_ = stmt.stmtLabel_;
		cursorName_ = stmt.cursorName_;
		t4statement_ = new T4Statement(this);
		stmt_ = stmt;
		sqlQueryType_ = TRANSPORT.SQL_QUERY_TYPE_NOT_SET;
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
	 * for sending to TrafT4.
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
	void convertObjectToSQL2(Locale locale, TrafT4Statement pstmt, Object paramValue, int paramRowCount, int paramNumber,
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

        int dataOffset = 2;
        boolean shortLength = false;
        if (dataType == InterfaceResultSet.SQLTYPECODE_VARCHAR_WITH_LENGTH) {
            shortLength = precision < Math.pow(2, 15);
            dataOffset = ((shortLength) ? 2 : 4);
            dataLength += dataOffset;

            if (dataLength % 2 != 0)
                dataLength++;
        } else if (dataType == InterfaceResultSet.SQLTYPECODE_BLOB || dataType == InterfaceResultSet.SQLTYPECODE_CLOB) {
            shortLength = false;
            dataOffset = 4;
            dataLength += dataOffset;

            if (dataLength % 2 != 0)
                dataLength++;
        }

		if (nullValue != -1)
			nullValue = (nullValue * paramRowCount) + (rowNumber * 2);

        noNullValue = (noNullValue * paramRowCount) + (rowNumber * dataLength);
		if (paramValue == null) {
			if (nullValue == -1) {
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale,
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
            } else if (paramValue instanceof byte[] || paramValue instanceof String) {
                String charSet = "";

                try {
                    if (this.ic_.getISOMapping() == InterfaceUtilities.SQLCHARSETCODE_ISO88591
                            && !this.ic_.getEnforceISO() && dataCharSet == InterfaceUtilities.SQLCHARSETCODE_ISO88591)
                        charSet = ic_.t4props_.getISO88591();
                    else {
                        if (dataCharSet == InterfaceUtilities.SQLCHARSETCODE_UNICODE && this.ic_.getByteSwap())
                            charSet = "UTF-16LE";
                        else
                            charSet = InterfaceUtilities.getCharsetName(dataCharSet);
                    }
                    if (paramValue instanceof byte[]) {
                        tmpBarray = (new String((byte[]) paramValue)).getBytes(charSet);
                    } else {
                        tmpBarray = (((String) paramValue)).getBytes(charSet);
                    }
                } catch (Exception e) {
                    throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "unsupported_encoding",
                            charSet);
                }
            } // end if (paramValue instanceof String)
			else {
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_parameter_value",
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
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_string_parameter",
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
						charSet = ic_.t4props_.getISO88591();
					else
					{
						if(dataCharSet == InterfaceUtilities.SQLCHARSETCODE_UNICODE && this.ic_.getByteSwap())
							charSet = "UTF-16LE";
						else
							charSet = InterfaceUtilities.getCharsetName(dataCharSet);
					}
					tmpBarray = ((String) paramValue).getBytes(charSet);
				} catch (Exception e) {
					throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "unsupported_encoding",
							charSet);
				}

			} // end if (paramValue instanceof String)
			else {
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_parameter_value",
						"VARCHAR data should be either bytes or String for column: " + paramNumber);
			}

			dataLen = tmpBarray.length;
			if (maxLength > dataLen) {
				Bytes.insertShort(values, noNullValue, (short) dataLen, this.ic_.getByteSwap());
				System.arraycopy(tmpBarray, 0, values, noNullValue + 2, dataLen);
			} else {
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_parameter_value",
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
					throw TrafT4Messages
							.createSQLException(
									pstmt.connection_.props_,
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
					throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "unsupported_encoding",
							messageArguments);
				}
				break;
			case InterfaceResultSet.SQLDTCODE_TIMESTAMP:
				Timestamp tmpts = null;
				try {
					String tmpStr = (String) paramValue;
					String pattern = "(\\d{4}-\\d{1,2}-\\d{1,2}):(.*)";
					if(tmpStr != null && tmpStr.matches(pattern)) {
						tmpStr = tmpStr.replaceFirst(pattern, "$1 $2");
					}
					tmpts = Timestamp.valueOf(tmpStr);
				} catch (IllegalArgumentException iex) {
					throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_parameter_value",
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
					throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "unsupported_encoding",
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
						throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale,
								"invalid_parameter_value", "Time data format is incorrect for column: " + paramNumber
										+ " = " + paramValue);
					} catch (java.io.UnsupportedEncodingException e) {
						Object[] messageArguments = new Object[1];
						messageArguments[0] = e.getMessage();
						throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "unsupported_encoding",
								messageArguments);
					}
					break;
				} else {
					// TrafT4Desc.SQLDTCODE_HOUR_TO_FRACTION data type!!!
					// let the next case structure handle it
				}
			case TrafT4Desc.SQLDTCODE_YEAR:
			case TrafT4Desc.SQLDTCODE_YEAR_TO_MONTH:
			case TrafT4Desc.SQLDTCODE_MONTH:
			case TrafT4Desc.SQLDTCODE_MONTH_TO_DAY:
			case TrafT4Desc.SQLDTCODE_DAY:
			case TrafT4Desc.SQLDTCODE_HOUR:
			case TrafT4Desc.SQLDTCODE_HOUR_TO_MINUTE:
			case TrafT4Desc.SQLDTCODE_MINUTE:
			case TrafT4Desc.SQLDTCODE_MINUTE_TO_SECOND:
				// case TrafT4Desc.SQLDTCODE_MINUTE_TO_FRACTION:
			case TrafT4Desc.SQLDTCODE_SECOND:
				// case TrafT4Desc.SQLDTCODE_SECOND_TO_FRACTION:
			case TrafT4Desc.SQLDTCODE_YEAR_TO_HOUR:
			case TrafT4Desc.SQLDTCODE_YEAR_TO_MINUTE:
			case TrafT4Desc.SQLDTCODE_MONTH_TO_HOUR:
			case TrafT4Desc.SQLDTCODE_MONTH_TO_MINUTE:
			case TrafT4Desc.SQLDTCODE_MONTH_TO_SECOND:
				// case TrafT4Desc.SQLDTCODE_MONTH_TO_FRACTION:
			case TrafT4Desc.SQLDTCODE_DAY_TO_HOUR:
			case TrafT4Desc.SQLDTCODE_DAY_TO_MINUTE:
			case TrafT4Desc.SQLDTCODE_DAY_TO_SECOND:
				// case TrafT4Desc.SQLDTCODE_DAY_TO_FRACTION:
			default:
				if (paramValue instanceof String) {
					try {
						tmpBarray = ((String) paramValue).getBytes("ASCII");
					} catch (Exception e) {
						throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "unsupported_encoding",
								"ASCII");
					}
				} else if (paramValue instanceof byte[]) {
					tmpBarray = (byte[]) paramValue;
				} else {
					throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale,
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
					throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_parameter_value",
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
					throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "unsupported_encoding",
							"ASCII");
				}
			} else {
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_cast_specification",
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
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_parameter_value",
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
						charSet = ic_.t4props_.getISO88591();
					else
					{
						if(dataCharSet == InterfaceUtilities.SQLCHARSETCODE_UNICODE && this.ic_.getByteSwap())
							charSet = "UTF-16LE";
						else
							charSet = InterfaceUtilities.getCharsetName(dataCharSet);
					}
					tmpBarray = ((String) paramValue).getBytes(charSet);
				} catch (Exception e) {
					throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "unsupported_encoding",
							charSet);
				}
			} // end if (paramValue instanceof String)
			else {
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_cast_specification",
						"VARCHAR data should be either bytes or String for column: " + paramNumber);
			}

			dataLen = tmpBarray.length;
			if (maxLength > (dataLen + dataOffset)) { 
				maxLength = dataLen + dataOffset;

                                if (shortLength) {
				   System.arraycopy(Bytes.createShortBytes((short) dataLen, this.ic_.getByteSwap()), 0, values, noNullValue, dataOffset);
                                } else {
				   System.arraycopy(Bytes.createIntBytes((int) dataLen, this.ic_.getByteSwap()), 0, values, noNullValue, dataOffset);
                                }
				System.arraycopy(tmpBarray, 0, values, (noNullValue + dataOffset), dataLen);
			} else {
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_string_parameter",
						"VARCHAR data longer than column length: " + paramNumber);
			}
			break;
		case InterfaceResultSet.SQLTYPECODE_BLOB:
			if (paramValue instanceof InputStream) {
				InputStream is = (InputStream)paramValue;
				dataLen = 0;
				try {
					int bytesRead = is.read(values, noNullValue + dataOffset, maxLength - dataOffset);
					dataLen = bytesRead;
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				System.arraycopy(Bytes.createIntBytes(dataLen, this.ic_.getByteSwap()), 0, values, noNullValue, dataOffset);
			}
			else {
				tmpBarray = (byte[])paramValue;
				dataLen = tmpBarray.length;
				dataOffset = 4;
	            if (maxLength > dataLen) {
	                System.arraycopy(Bytes.createIntBytes(dataLen, this.ic_.getByteSwap()), 0, values, noNullValue, dataOffset);
	                System.arraycopy(tmpBarray, 0, values, (noNullValue + dataOffset), dataLen);
	            } else {
	                throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "23",
	                        "BLOB input data is longer than the length for column: " + paramNumber);
	            }
			}
			break;
		case InterfaceResultSet.SQLTYPECODE_BINARY:
		case InterfaceResultSet.SQLTYPECODE_VARBINARY:
			if (paramValue instanceof InputStream) {
				InputStream is = (InputStream)paramValue;
				dataLen = 0;
				try {
					int bytesRead = is.read(values, noNullValue + dataOffset, maxLength - dataOffset);
					dataLen = bytesRead;
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				System.arraycopy(Bytes.createIntBytes(dataLen, this.ic_.getByteSwap()), 0, values, noNullValue, dataOffset);
			}
			else {
				tmpBarray = (byte[])paramValue;
				dataLen = tmpBarray.length;
				dataOffset = 4;
	            if (maxLength > dataLen) {
	                System.arraycopy(Bytes.createIntBytes(dataLen, this.ic_.getByteSwap()), 0, values, noNullValue, dataOffset);
	                System.arraycopy(tmpBarray, 0, values, (noNullValue + dataOffset), dataLen);
	            } else {
	                throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "23",
	                        "BINARY input data is longer than the length for column: " + paramNumber);
	            }
			}
			break;

		case InterfaceResultSet.SQLTYPECODE_CLOB:
			String charSet = "";

			try {
				if (this.ic_.getISOMapping() == InterfaceUtilities.SQLCHARSETCODE_ISO88591
						&& !this.ic_.getEnforceISO() && dataCharSet == InterfaceUtilities.SQLCHARSETCODE_ISO88591)
					charSet = ic_.t4props_.getISO88591();
				else
				{
					if(dataCharSet == InterfaceUtilities.SQLCHARSETCODE_UNICODE && this.ic_.getByteSwap())
						charSet = "UTF-16LE";
					else
						charSet = InterfaceUtilities.getCharsetName(dataCharSet);
				}
				tmpBarray = ((String) paramValue).getBytes("UTF-8");
			} catch (Exception e) {
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "unsupported_encoding",
						charSet);
			}
			dataLen = tmpBarray.length;
			dataOffset = 4;
			if (maxLength > dataLen) {
			    System.arraycopy(Bytes.createIntBytes(dataLen, this.ic_.getByteSwap()), 0, values, noNullValue, dataOffset);
			    System.arraycopy(tmpBarray, 0, values, (noNullValue + dataOffset), dataLen);
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

		case InterfaceResultSet.SQLTYPECODE_TINYINT:
			tmpbd = Utility.getBigDecimalValue(locale, paramValue);
			if (scale > 0) {
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_parameter_value",
						"Cannot have scale for param: " + paramNumber);
                        }
                        
			Utility.checkSignedTinyintBoundary(locale, tmpbd);

			Bytes.insertShort(values, noNullValue, tmpbd.byteValueExact(), this.ic_.getByteSwap());

                        break;
		case InterfaceResultSet.SQLTYPECODE_TINYINT_UNSIGNED:
			tmpbd = Utility.getBigDecimalValue(locale, paramValue);
			if (scale > 0) {
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_parameter_value",
						"Cannot have scale for param: " + paramNumber);
                        }
                        
			Utility.checkUnsignedTinyintBoundary(locale, tmpbd);

			Bytes.insertShort(values, noNullValue, tmpbd.byteValue(), this.ic_.getByteSwap());
                        break;
		case InterfaceResultSet.SQLTYPECODE_SMALLINT:
			tmpbd = Utility.getBigDecimalValue(locale, paramValue);
			if (scale > 0) {
				tmpbd = tmpbd.movePointRight(scale);
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

			Utility.checkUnsignedShortBoundary(locale, tmpbd);

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
			Utility.checkLongBoundary(locale, tmpbd);
			Utility.checkDecimalBoundary(locale, tmpbd, precision);
			Bytes.insertLong(values, noNullValue, tmpbd.longValue(), this.ic_.getByteSwap());
			break;
		case InterfaceResultSet.SQLTYPECODE_LARGEINT_UNSIGNED:
			tmpbd = Utility.getBigDecimalValue(locale, paramValue);

			if (scale > 0) {
				tmpbd = tmpbd.movePointRight(scale);

				// check boundary condition for Numeric.
			}
			Utility.checkUnsignedLongBoundary(locale, tmpbd);
			Bytes.insertLong(values, noNullValue, tmpbd.longValue(), this.ic_.getByteSwap());
			break;
		case InterfaceResultSet.SQLTYPECODE_DECIMAL:
		case InterfaceResultSet.SQLTYPECODE_DECIMAL_UNSIGNED:

			// create an parameter with out "."
			try {
				tmpbd = Utility.getBigDecimalValue(locale, paramValue);
				if (scale > 0) {
					tmpbd = tmpbd.movePointRight(scale);

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
					throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "unsupported_encoding",
							messageArguments);
				}
			} catch (NumberFormatException nex) {
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "invalid_parameter_value",
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
				throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "data_truncation_exceed", new int[]{dataLen, maxLength});
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
		case InterfaceResultSet.SQLTYPECODE_BOOLEAN:
			tmpbd = Utility.getBigDecimalValue(locale, paramValue);

			Bytes.insertByte(values, noNullValue, tmpbd.byteValue());
			break;
		// You will not get this type, since server internally converts it
		// SMALLINT, INTERGER or LARGEINT
		case InterfaceResultSet.SQLTYPECODE_DECIMAL_LARGE:
		case InterfaceResultSet.SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
		case InterfaceResultSet.SQLTYPECODE_BIT:
		case InterfaceResultSet.SQLTYPECODE_BITVAR:
		case InterfaceResultSet.SQLTYPECODE_BPINT_UNSIGNED:
		default:
                    if (ic_.t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
                        Object p[] = T4LoggingUtilities.makeParams(stmt_.connection_.props_, locale, pstmt, paramValue,
                                                                   paramNumber);
                        String temp = "Restricted_Ddatatype_Error" + "datatype = " + dataType;
                        ic_.t4props_.t4Logger_.logp(Level.FINEST, "InterfaceStatementtt", "convertObjectToSQL222", temp, p);
                    }
                    
                    throw TrafT4Messages.createSQLException(pstmt.connection_.props_, locale, "restricted_data_type", null);
		}
		if (ic_.t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities
					.makeParams(stmt_.connection_.props_, locale, pstmt, paramValue, paramNumber);
			String temp = "datatype = " + dataType;
			ic_.t4props_.t4Logger_.logp(Level.FINEST, "InterfaceStatement", "convertObjectToSQL2", temp, p);
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

	SQL_DataValue_def fillInSQLValues2(Locale locale, TrafT4Statement stmt, int paramRowCount, int paramCount,
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
					} catch (TrafT4Exception e) {
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
							|| dataType == InterfaceResultSet.SQLTYPECODE_BLOB
							|| dataType == InterfaceResultSet.SQLTYPECODE_CLOB
							|| dataType == InterfaceResultSet.SQLTYPECODE_VARCHAR_LONG
							|| dataType == InterfaceResultSet.SQLTYPECODE_VARCHAR) {
                                                boolean shortLength = colLength < Math.pow(2, 15);
                                                int dataOffset = ((shortLength) ? 2 : 4);
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

	// -------------------------------------------------------------
	short getSqlStmtType(String str) {
		short rt1 = TRANSPORT.TYPE_UNKNOWN;
		switch (sqlQueryType_) {
			case TRANSPORT.SQL_SELECT_UNIQUE:
			case TRANSPORT.SQL_SELECT_NON_UNIQUE:
				rt1 = TRANSPORT.TYPE_SELECT;
				break;
			case TRANSPORT.SQL_INSERT_UNIQUE:
			case TRANSPORT.SQL_INSERT_NON_UNIQUE:
			case TRANSPORT.SQL_INSERT_RWRS:
				if (stmt_ .inputDesc_ != null && stmt_.inputDesc_.length > 0)
					rt1 = TRANSPORT.TYPE_INSERT_PARAM;
				else
					rt1 = TRANSPORT.TYPE_INSERT;
				break;
			case TRANSPORT.SQL_UPDATE_UNIQUE:
			case TRANSPORT.SQL_UPDATE_NON_UNIQUE:
				rt1 = TRANSPORT.TYPE_UPDATE;
				break;
			case TRANSPORT.SQL_DELETE_UNIQUE:
			case TRANSPORT.SQL_DELETE_NON_UNIQUE:
				rt1 = TRANSPORT.TYPE_DELETE;
				break;
			case TRANSPORT.SQL_CALL_NO_RESULT_SETS:
			case TRANSPORT.SQL_CALL_WITH_RESULT_SETS:
				rt1 = TRANSPORT.TYPE_CALL;
				break;
			default:
				break;
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
	static TrafT4Desc[] NewDescArray(SQLItemDescList_def desc) {
		int index;
		TrafT4Desc[] trafT4DescArray;
		SQLItemDesc_def SQLDesc;

		if (desc.list == null || desc.list.length == 0) {
			return null;
		}

		trafT4DescArray = new TrafT4Desc[desc.list.length];

		for (index = 0; index < desc.list.length; index++) {
			SQLDesc = desc.list[index];
			boolean nullInfo = (((new Byte(SQLDesc.nullInfo)).shortValue()) == 1) ? true : false;
			boolean signType = (((new Byte(SQLDesc.signType)).shortValue()) == 1) ? true : false;
			trafT4DescArray[index] = new TrafT4Desc(SQLDesc.dataType, (short) SQLDesc.datetimeCode, SQLDesc.maxLen,
					SQLDesc.precision, SQLDesc.scale, nullInfo, SQLDesc.colHeadingNm, signType, SQLDesc.ODBCDataType,
					SQLDesc.ODBCPrecision, SQLDesc.SQLCharset, SQLDesc.ODBCCharset, SQLDesc.CatalogName,
					SQLDesc.SchemaName, SQLDesc.TableName, SQLDesc.dataType, SQLDesc.intLeadPrec, SQLDesc.paramMode);
		}
		return trafT4DescArray;
	}

	// -------------------------------------------------------------
	static TrafT4Desc[] NewDescArray(Descriptor2[] descArray) {
		int index;
		TrafT4Desc[] trafT4DescArray;
		Descriptor2 desc;

		if (descArray == null || descArray.length == 0) {
			return null;
		}

		trafT4DescArray = new TrafT4Desc[descArray.length];

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
			trafT4DescArray[index] = new TrafT4Desc(desc.noNullValue_, desc.nullValue_, desc.version_, desc.dataType_,
					(short) desc.datetimeCode_, desc.maxLen_, (short) desc.precision_, (short) desc.scale_, nullInfo,
					signType, desc.odbcDataType_, desc.odbcPrecision_, desc.sqlCharset_, desc.odbcCharset_,
					desc.colHeadingNm_, desc.tableName_, desc.catalogName_, desc.schemaName_, desc.headingName_,
					desc.intLeadPrec_, desc.paramMode_, desc.dataType_, desc.getRowLength());
		}
		return trafT4DescArray;
	}

	// -------------------------------------------------------------
	// Interface methods
	void executeDirect(int queryTimeout, TrafT4Statement stmt) throws SQLException {
		short executeAPI = stmt.getOperationID();
		byte[] messageBuffer = stmt.getOperationBuffer();
		GenericReply gr = null;

		gr = t4statement_.ExecuteGeneric(executeAPI, messageBuffer);
		stmt.operationReply_ = gr.replyBuffer;

		if (ic_.t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(stmt_.connection_.props_, queryTimeout, stmt);
			String temp = "Exiting ExecDirect.";
			ic_.t4props_.t4Logger_.logp(Level.FINEST, "InterfaceStatement", "executeDirect", temp, p);
		}
	} // end executeDirect

	// --------------------------------------------------------------------------
	int close() throws SQLException {
		int rval = 0;
		CloseReply cry_ = null;
		ic_.isConnectionOpen();
		if (ic_.t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(stmt_.connection_.props_);
			String temp = "Closing = " + stmtLabel_;
			ic_.t4props_.t4Logger_.logp(Level.FINEST, "InterfaceStatement", "close", temp, p);
		}

		cry_ = t4statement_.Close();
		switch (cry_.m_p1.exception_nr) {
		case TRANSPORT.CEE_SUCCESS:

			// ignore the SQLWarning for the static close
			break;
		case odbc_SQLSvc_Close_exc_.odbc_SQLSvc_Close_SQLError_exn_:
			TrafT4Messages.throwSQLException(stmt_.connection_.props_, cry_.m_p1.SQLError);
		default:
			throw TrafT4Messages.createSQLException(stmt_.connection_.props_, ic_.getLocale(), "ids_unknown_reply_error",
					null);
		} // end switch

		return cry_.m_p2; // rowsAffected
	} // end close

	// --------------------------------------------------------------------------
	void cancel(long startTime) throws SQLException {
		// currently there are no callers to this statement
		// It is important that callers specify the startTime correctly for cancel work as expected.
		ic_.cancel(startTime);
	}

	// --------------------------------------------------------------------------
	// Interface methods for prepared statement
	void prepare(String sql, int queryTimeout, TrafT4PreparedStatement pstmt) throws SQLException {
		int sqlAsyncEnable = 0;
		this.stmtType_ = this.EXTERNAL_STMT;
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

//3196 - NDCS transaction for SPJ	
//		if (ic_.t4props_.getSPJEnv())
//			txId = getUDRTransaction(this.ic_.getByteSwap());
//		else
//			txId = Bytes.createIntBytes(0, false);
		txId = Bytes.createIntBytes(0, false);

		PrepareReply pr = t4statement_.Prepare(sqlAsyncEnable, (short) this.stmtType_, this.sqlStmtType_,
				pstmt.stmtLabel_, stmtLabelCharset, cursorName, cursorNameCharset, moduleName, moduleNameCharset,
				moduleTimestamp, sqlString, sqlStringCharset, stmtOptions, maxRowsetSize, txId);

		pr_ = pr;
		this.sqlQueryType_ = pr.sqlQueryType;
		this.sqlStmtType_ = getSqlStmtType(sqlString);

		switch (pr.returnCode) {
		case TRANSPORT.SQL_SUCCESS:
		case TRANSPORT.SQL_SUCCESS_WITH_INFO:
			TrafT4Desc[] OutputDesc = InterfaceStatement.NewDescArray(pr.outputDesc);
			TrafT4Desc[] InputDesc = InterfaceStatement.NewDescArray(pr.inputDesc);
			pstmt.setPrepareOutputs2(InputDesc, OutputDesc, pr.inputNumberParams, pr.outputNumberParams,
					pr.inputParamLength, pr.outputParamLength, pr.inputDescLength, pr.outputDescLength);

			if (pr.errorList != null && pr.errorList.length > 0) {
				TrafT4Messages.setSQLWarning(stmt_.connection_.props_, pstmt, pr.errorList);
			}

			this.stmtHandle_ = pr.stmtHandle;

			break;

		case odbc_SQLSvc_Prepare_exc_.odbc_SQLSvc_Prepare_SQLError_exn_:

		default:
			TrafT4Messages.throwSQLException(stmt_.connection_.props_, pr.errorList);
		}

		if (ic_.t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(stmt_.connection_.props_, sql, queryTimeout, pstmt);
			String temp = "Exiting prepare...";
			ic_.t4props_.t4Logger_.logp(Level.FINEST, "InterfaceStatement", "prepare", temp, p);
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
			ic_.t4props_.t4Logger_.logp(Level.FINEST, "InterfaceStatement", "getUDRTransaction",
					"Error calling UDR for transaction id");

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
	// executeDirect
			, String sql, TrafT4Statement stmt

	) throws SQLException {
		cursorName_ = stmt.cursorName_;
		rowCount_ = 0;

		int sqlAsyncEnable = (stmt.getResultSetHoldability() == TrafT4ResultSet.HOLD_CURSORS_OVER_COMMIT) ? 1 : 0;
		int inputRowCnt = paramRowCount;
		int maxRowsetSize = stmt.getMaxRows();
		String sqlString = (sql == null) ? stmt.getSQL() : sql;
		int sqlStringCharset = 1;
		int cursorNameCharset = 1;
		int stmtLabelCharset = 1;
		byte[] txId;
		ArrayList clientErrors = new ArrayList();

//3196 - NDCS transaction for SPJ	
//		if (ic_.t4props_.getSPJEnv())
//			txId = getUDRTransaction(this.ic_.getByteSwap());
//		else if (stmt.transactionToJoin != null)
//			txId = stmt.transactionToJoin;
//		else if (stmt.connection_.transactionToJoin != null)
//			txId = stmt.connection_.transactionToJoin;
//		else
//			txId = Bytes.createIntBytes(0, false); // 0 length, no data
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
			stmt.outputDesc_ = null; // clear the output descriptors
		}

		if (stmt.usingRawRowset_ == true) {
			executeAPI = TRANSPORT.SRVR_API_SQLEXECUTE2;
			inputDataValue = new SQL_DataValue_def();
			inputDataValue.userBuffer = stmt.rowwiseRowsetBuffer_;
			inputDataValue.length = stmt.rowwiseRowsetBuffer_.limit() - 4;

			if (this.sqlQueryType_ == TRANSPORT.SQL_INSERT_RWRS) // use the param values
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

			if (ic_.t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(stmt_.connection_.props_, paramRowCount, paramCount,
						paramValues, queryTimeout, stmt);
				String temp = "invoke ==> Execute2";
				ic_.t4props_.t4Logger_.logp(Level.FINEST, "InterfaceStatement", "execute", temp, p);
			}
		}

		ExecuteReply er = t4statement_.Execute(executeAPI, sqlAsyncEnable, inputRowCnt - clientErrors.size(),
				maxRowsetSize, this.sqlStmtType_, this.stmtHandle_, sqlString, sqlStringCharset, this.cursorName_,
				cursorNameCharset, stmt.stmtLabel_, stmtLabelCharset, inputDataValue, inputValueList, txId,
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

		stmt_.result_set_offset = 0;
		rowCount_ = er.rowsAffected;
		
		int numStatus;
		
		if (stmt_.connection_.props_.getDelayedErrorMode())
		{
			if (stmt_._lastCount > 0) {
				numStatus = stmt_._lastCount;
			}
			else {
				numStatus = inputRowCnt;
			}
		}
		else
		{
			numStatus = inputRowCnt;
		}

	    if (numStatus < 1)
	    {
	    	numStatus = 1;
	    }

	    stmt.batchRowCount_ = new int[numStatus];
	    boolean batchException = false;	//3164

	    if (stmt_.connection_.props_.getDelayedErrorMode() && stmt_._lastCount < 1) {
			Arrays.fill(stmt.batchRowCount_, -2); // fill with success
	    } 
	    else if (er.returnCode == TRANSPORT.SQL_SUCCESS || er.returnCode == TRANSPORT.SQL_SUCCESS_WITH_INFO
				|| er.returnCode == TRANSPORT.NO_DATA_FOUND) {
			Arrays.fill(stmt.batchRowCount_, -2); // fill with success

			// if we had errors with valid rowIds, update the array
			if (er.errorList != null)
			{
				for (int i = 0; i < er.errorList.length; i++) {
					int row = er.errorList[i].rowId - 1;
					if (row >= 0 && row < stmt.batchRowCount_.length) {
						stmt.batchRowCount_[row] = -3;
						batchException = true;	//3164
					}
				}
			}

            // Sometimes users may set schema through stmt.exec("set schema xx") instead of
            // conn.setSchema("xx"), so there need to update local schema when it success
            if (this.sqlQueryType_ == TRANSPORT.SQL_SET_SCHEMA) {
                String schema = extractSchema(sqlString);
                System.out.println("extract schema : "+schema);
                ic_.setSchemaDirect(schema);
            }

			// set the statement label if we didnt get one back.
			if (er.stmtLabels == null || er.stmtLabels.length == 0) {
				er.stmtLabels = new String[1];
				er.stmtLabels[0] = stmt.stmtLabel_;
			}

			// get the descriptors from the proper location
			TrafT4Desc[][] desc = null;

			// try from execute data first
			if (er.outputDesc != null && er.outputDesc.length > 0) {
				desc = new TrafT4Desc[er.outputDesc.length][];

				for (int i = 0; i < er.outputDesc.length; i++) {
					desc[i] = InterfaceStatement.NewDescArray(er.outputDesc[i]);
				}
			}
			// try from the prepare data
			else if (stmt.outputDesc_ != null && stmt.outputDesc_.length > 0) {
				desc = new TrafT4Desc[1][];
				desc[0] = stmt.outputDesc_;
			}

			if (sqlQueryType_ == TRANSPORT.SQL_CALL_NO_RESULT_SETS ||
					sqlQueryType_ == TRANSPORT.SQL_CALL_WITH_RESULT_SETS) {
				TrafT4CallableStatement cstmt = (TrafT4CallableStatement) stmt;
				Object[] outputValueArray;
				if(er.returnCode == TRANSPORT.NO_DATA_FOUND) { //this should really only happen with LAST0 specified
					if (null != cstmt.outputDesc_) {
					    outputValueArray = new Object[cstmt.outputDesc_.length];
					} else {
					    outputValueArray = null;
					}
				}
				else {
					outputValueArray = InterfaceResultSet.getExecute2Outputs(cstmt.connection_, cstmt.outputDesc_, 
						er.outValues, this.ic_.getByteSwap());
				}
				
				cstmt.setExecuteCallOutputs(outputValueArray, (short) er.rowsAffected);
				stmt.setMultipleResultSets(er.numResultSets, desc, er.stmtLabels, er.proxySyntax);
			} else {
				// fix until we start returning numResultsets for more than just
				// SPJs
				if (desc != null && desc.length > 0 && er.numResultSets == 0) {
					er.numResultSets = 1;
				}

				if (er.outValues != null && er.outValues.length > 0) {
					stmt.setExecute2Outputs(er.outValues, (short) er.rowsAffected, false, er.proxySyntax, desc[0]);
				} else {
					stmt.setMultipleResultSets(er.numResultSets, desc, er.stmtLabels, er.proxySyntax);
				}
			}
			if (er.errorList != null) {
				TrafT4Messages.setSQLWarning(stmt_.connection_.props_, stmt, er.errorList);
			}
		} else {
			Arrays.fill(stmt.batchRowCount_, -3); // fill with failed
			TrafT4Messages.throwSQLException(stmt_.connection_.props_, er.errorList);
		}
	    //3164
	    if (batchException) {
	    	TrafT4Messages.throwSQLException(stmt_.connection_.props_, er.errorList);
	    }
	}

	private String extractSchema(String sqlString) {
		String schemaRegex = "(SET)\\s+(SCHEMA)\\s+([a-zA-Z0-9]+\\s*\\.)\\s*([a-zA-Z0-9]+)\\s*";
		Pattern pattern = Pattern.compile(schemaRegex);
		Matcher m = pattern.matcher(sqlString.toUpperCase());
		while (m.find()) {
			return m.group(m.groupCount());
		}
		return "";
    	}

	protected T4Statement getT4statement() 
	{
		return t4statement_;
    	}
} // end class InterfaceStatement
