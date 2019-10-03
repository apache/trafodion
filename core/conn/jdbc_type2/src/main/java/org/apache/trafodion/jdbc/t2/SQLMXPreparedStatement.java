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
 * Filename : SQLMXPreparedStatement.java
 */
package org.apache.trafodion.jdbc.t2;

import java.io.InputStream;
import java.io.Reader;
import java.io.UnsupportedEncodingException;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.math.RoundingMode;
import java.net.URL;
import java.sql.Array;
import java.sql.BatchUpdateException;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.DatabaseMetaData;
import java.sql.Date;
import java.sql.NClob;
import java.sql.ParameterMetaData;
import java.sql.Ref;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.RowId;
import java.sql.SQLException;
import java.sql.SQLXML;
import java.sql.Time;
import java.sql.Timestamp;
import java.sql.Types;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Locale;
import java.util.Arrays;
//import com.tandem.tmf.Current;	// Linux port - ToDo

/**
 * An object that represents a precompiled SQL statement.
 *
 * <p>
 * A SQL Statement is pre-compiled and stored in a PreparedStatement object.
 * This object can then be used to efficiently execute this statement multiple
 * times.
 * </p>
 *
 * <p>
 * <B>Note:</B> The setter methods (<tt>setShort</tt>, <tt>setString</tt>, and
 * so on) for setting IN parameter must specify types that are compatible with
 * the defined SQL type of the input parameter. For instance, if the IN
 * parameter has SQL type <tt>INTEGER</tt>, then the method <tt>setInt</tt>
 * should be used. The Trafodion JDBC does not follow this specification and will allow
 * writing to all most all SQL column types. The only exceptions would be the
 * <tt>TIME</tt>, <tt>TIMESTAMP</tt>, <tt>DATE</tt>, <tt>CLOB</tt>, and
 * <tt>BLOB</tt>. <B>If these extensions are use the Java code may not run on
 * any other platform.</B>
 * </p>
 *
 * <p>
 * If arbitrary parameter type conversions are required, then the setObject
 * method should be used with a target SQL type.
 * </p>
 *<p>
 * In the following example of setting a parameter, <tt>con</tt> represents an
 * active connection:
 *</p>
 *<ul PLAIN> <tt><pre>
 * 	PreparedStatement pstmt =
 * 	  con.prepareStatement("UPDATE EMPLOYEES SET SALARY = ? WHERE ID =?");
 * 	pstmt.setBigDecimal(1, 153833.00);
 * 	pstmt.setInt(2,110592);
 * </ul>
 * </tt></pre>
 * <p>
 * In the next example, the use of temporary parameters are used, and the SQL/MX
 * uses a data type of long for the temporary parameters:
 * </p>
 * <ul PLAIN> <tt><pre>
 * 	 PreparedStatement pstmt = con.preparedStatement("SELECT * FROM
 * 	 Inventory  p WHERE (p.cost / ? = ?) AND(p.partId = ?)");
 * 	 pstmt.setLong(1, 2);
 * 	 pstmt.setLong(2,1000000);
 * 	 pstmt.setString(3, "Trafodion Printer");
 * </ul>
 * </tt></pre>
 * <p>
 * The following table shows the different numeric SQL/MX column types and the
 * JDBC method that should be used to write to the column. Also included is the
 * maximum data value for each of the column types. The DECIMAL column type is
 * not included at this time, since nothing has changed in the Java code for
 * this column type.
 * </p>
 * <p>
 * Please note that the NUMERIC column type is being limited by SQL/MX. They
 * have specified that the maximum precision for a NUMERIC column type is 18. I
 * have been able to get 19 digits into a NUMERIC column type, but I do not
 * think that it is supported. The storage size for the NUMERIC column type can
 * be 16 bits, 32 bits, or 64 bit, depending upon the precision and scale
 * values.
 * </p>
 * <table cols=3 border >
 * <caption align="top" > SQL/MX COLUMN TYPES NAD JDBC SETTER METHODS </caption>
 * <tr>
 * <th>SQL/MX COLUMN TYPE</th>
 * <th>JDBC SETTER METHOD</th>
 * <th>JDBC TYPE</th>
 * <th>MAX VAULE</th>
 * </tr>
 * <tr>
 * <td>SMALLINT</td>
 * <td>setShort</td>
 * <td>Types.SMALLINT</td>
 * <td>32767</td>
 * </tr>
 * <tr>
 * <td>SMALLINT UNSIGNED</td>
 * <td>setInt</td>
 * <td>Types.SMALLINT</td>
 * <td>65535</td>
 * </tr>
 * <tr>
 * <td>INTEGER</td>
 * <td>setInt</td>
 * <td>Types.INTEGER</td>
 * <td>2147483647</td>
 * </tr>
 * <tr>
 * <td>INTEGER UNSIGNED</td>
 * <td>setLong</td>
 * <td>Types.INTEGER</td>
 * <td>4294967295</td>
 * </tr>
 * <tr>
 * <td>LARGEINT</td>
 * <td>setLong</td>
 * <td>Types.BIGINT</td>
 * <td>9223372036854775807</td>
 * </tr>
 * <tr>
 * <td>NUMERIC(p,s)</td>
 * <td>setBigDecimal</td>
 * <td>Types.NUMERIC</td>
 * <td>999999999999999999</td>
 * </tr>
 * <tr>
 * <td>NUMERIC(p,s) UNSIGNED</td>
 * <td>setBigDecimal</td>
 * <td>Types.NUMERIC</td>
 * <td>999999999999999999</td>
 * </tr>
 * </table>
 *
 * @see ResultSet
 * @see org.apache.trafodion.jdbc.t2.SQLMXConnection#prepareStatement
 * @version Trafodion JDBC/MX
 *
 */

public class SQLMXPreparedStatement extends SQLMXStatement implements
		java.sql.PreparedStatement {
	// java.sql.PreparedStatement interface methods
	private void checkStringToIntegral() throws SQLException {
		// This is a check for a string being stored into an integral column
		// If the string contains a decimal point, we will throw a numeric
		// conversion
		// exception. This is not the correct action, but due to legacy code,
		// we do not want to change the functionality at this time.
		if (dataWrapper.getDataType(1) == DataWrapper.STRING) {
			String value = dataWrapper.getString(1);
			if ((value != null) && (value.indexOf('.') != -1)) {
				Object[] errString = new Object[1];
				errString[0] = value;
				throw Messages.createSQLException(connection_.locale_,
						"invalid_string_data_format", errString);
			}
		}
	}

	/**
	 * Adds a set of parameters to this <tt>PreparedStatement</tt> object's
	 * batch of commands.
	 *
	 * @exception SQLException
	 *                if a database access error occurs.
	 * @since 1.2
	 * @see java.sql.Statement#addBatch(String sql)
	 */
	public void addBatch() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_addBatch].methodEntry();
		try {
			clearWarnings();
			if (inputDesc_ == null)
				return;
			// Check if all parameters are set for current set
			checkIfAllParamsSet();
			// Add to the number of Rows Count
			if (rowsValue_ == null) {
				rowsValue_ = new ArrayList<Object>();
			} else {
				int sizeOfBatch = -1;
				if(connection_.t2props.getBatchBinding() > 0){
					sizeOfBatch = connection_.t2props.getBatchBinding();
				}
//				if (!(System.getProperty("t2jdbc.batchBinding", "OFF")
//						.equalsIgnoreCase("OFF"))) {
//					sizeOfBatch = Integer.parseInt(System.getProperty(
//							"t2jdbc.batchBinding", "OFF"));
//				} else if (!(System.getProperty("batchBinding", "OFF")
//						.equalsIgnoreCase("OFF"))) {
//					sizeOfBatch = Integer.parseInt(System.getProperty(
//							"batchBinding", "OFF"));
//				}
				if (sizeOfBatch > -1
				  && ( this.sql_.trim().startsWith("INSERT") ||
                                       this.sql_.trim().startsWith("insert") ||
				       this.sql_.trim().startsWith("UPSERT") ||
                                       this.sql_.trim().startsWith("upsert") ||
                                       this.sql_.trim().startsWith("UPDATE") ||
                                       this.sql_.trim().startsWith("update") ||
                                       this.sql_.trim().startsWith("DELETE") ||
                                       this.sql_.trim().startsWith("delete") 
                                      )) {
					if (!(rowsValue_.size() + 1 <= sizeOfBatch)) {
						throw new SQLException(
								"The no. of elements in batch is greater than the batchBinding value.");
					}
				}
			}
			rowsValue_.add(paramContainer_);
			paramRowCount_++;
			paramContainer_ = new DataWrapper(inputDesc_.length);
			// Clear the isValueSet_ and paramValue flag in inputDesc_ 
			for (int i = 0; i < inputDesc_.length; i++) {
				inputDesc_[i].paramValue_ = null;
				inputDesc_[i].isValueSet_ = false;
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_addBatch].methodExit();
		}
	}

	/**
	 * Empties this <tt>Statement</tt> object's current list of SQL commands.
	 * <p>
	 * <B>NOTE:</B> This method is optional.
	 * </p>
	 *
	 * @exception SQLException
	 *                if a database access error occurs or the driver does not
	 *                support batch updates
	 * @since 1.2
	 * @see java.sql.Statement#addBatch(String sql)
	 */
	public void clearBatch() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_clearBatch].methodEntry();
		try {
			clearWarnings();
			if (inputDesc_ == null)
				return;
			if (rowsValue_ != null)
				rowsValue_.clear();
			if (lobObjects_ != null)
				lobObjects_.clear();
			paramRowCount_ = 0;
			// Clear the isValueSet_ flag in inputDesc_
			for (int i = 0; i < inputDesc_.length; i++) {
				inputDesc_[i].isValueSet_ = false;
				paramContainer_.setNull(i + 1);
				inputDesc_[i].paramValue_ = null;
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_clearBatch].methodExit();
		}
	}

	/**
	 * Clears the current parameter values immediately.
	 * <p>
	 * In general, parameter values remain in force for repeated used of a
	 * Statement. Setting a parameter value automatically clears its previous
	 * value. However, in some cases, it is useful to immediately release the
	 * resources used by the current parameter values; this can be done by
	 * calling <tt>clearParameters</tt>.
	 * </p>
	 *
	 * @exception java.sql.SQLException
	 *                if a database access error occurs
	 */
	public void clearParameters() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_clearParameters].methodEntry();
		try {
			// Return if there are no input parameters (otherwise a NPE is
			// thrown)
			if (inputDesc_ == null)
				return;

			// Clear the isValueSet_ flag in inputDesc_
			for (int i = 0; i < inputDesc_.length; i++) {
				inputDesc_[i].isValueSet_ = false;
				paramContainer_.setNull(i + 1);
				inputDesc_[i].paramValue_ = null;
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_clearParameters].methodExit();
		}
	}

	/**
	 * Closes this prepared statement and releases all resources.
	 *
	 * @throws SQLException
	 *             if database error occurs.
	 */
	public void close() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_close].methodEntry();
		try {
			clearWarnings();
			synchronized (connection_) {
				if (isClosed_)
					return;
				try {
					if (!connection_.isClosed_) {
						if (!connection_.isStatementCachingEnabled()) {
							if (resultSet_ != null) {
								if (!resultSet_.isClosed_)
									resultSet_.close(true);
								else{
									close(connection_.server_,
											connection_.getDialogueId(), stmtId_,
											true);
									connection_.hClosestmtCount++;

									if (connection_.out_ != null) {
										if (connection_.traceFlag_ >= T2Driver.POOLING_LVL){
											connection_.out_.println(getTraceId() + "close() "
													+ "Hard closed statement - " + "\""
													+ this.stmtId_ + "\"");
										connection_.out_.println(getTraceId() + "close() "
												+"\""+"HARDCLOSED STMTS COUNT:"+connection_.hClosestmtCount+"\"");
										}
									}


								}
							} else{
								close(connection_.server_,
										connection_.getDialogueId(), stmtId_, true);
								connection_.hClosestmtCount++;

								if (connection_.out_ != null) {
									if (connection_.traceFlag_ >= T2Driver.POOLING_LVL){
										connection_.out_.println(getTraceId() + "close() "
												+ "Hard closed statement - " + "\""
												+ this.stmtId_ + "\"");
									connection_.out_.println(getTraceId() + "close() "
											+"\""+"HARDCLOSED STMTS COUNT:"+connection_.hClosestmtCount+"\"");
									}
								}

							}
						} else{
							logicalClose();

						}
					}
				} finally {
					isClosed_ = true;
					if (!connection_.isStatementCachingEnabled())
						connection_.removeElement(this);
					resultSet_ = null;
				}
			}// End sync
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_close].methodExit();
		}
	}

	/**
	 * Executes the SQL statement in this <tt>PreparedStatement</tt> object,
	 * which may be any kind of SQL statement. Some prepared statements return
	 * multiple results; the <tt>execute</tt> method handles these complex
	 * statements as well as the simpler form of statements handled by the
	 * methods <tt>executeQuery</tt> and <tt>executeUpdate</tt>. This method
	 * should only be used if the return type object is unknown or if there will
	 * be multiple results.
	 * <p>
	 * The <tt>execute</tt> method returns a boolean to indicate the form of the
	 * first result. You must call either the method <tt>getResultSet</tt> or
	 * <tt>getUpdateCount</tt> to retrieve the result; you must call
	 * <tt>getMoreResults</tt> to move to any subsequent result(s).
	 *
	 * @return <tt>true</tt> if the first result is a <tt>ResultSet</tt> object;
	 *         <tt>false</tt> if the first result is an update count or there is
	 *         no result
	 *
	 * @throws SQLException
	 *             if a database access error
	 * @throws SqlException
	 *             if an argument is supplied to this method
	 * @see java.sql.Statement#execute(java.lang.String)
	 * @see java.sql.Statement#getResultSet()
	 * @see java.sql.Statement#getUpdateCount()
	 * @see java.sql.Statement#getMoreResults()
	 */
	public boolean execute() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_execute].methodEntry();
		//**********************************************************************
		// *
		// * Please note that this routine will never return false. It is
		// supposed to
		// * return false when there is no data that resulted from the
		// execution. To
		// * fix the method, a resultSet_.next needs to be performed in order to
		// * check to see if there is any data. The method getResultSet will
		// return
		// * resultSet_ and everything will work the way that the API spec
		// documents.
		// * There was not enough time in V31 to make this change and do enough
		// testing.
		// *
		//**********************************************************************
		// **
		try {
/* Linux port - ToDo tmf.jar related
			Current tx = null;
*/
			int txnState = -1; // holds TMF transaction state code
			boolean txBegin = false; // flag to indicate internal autocommit
			// duties
			boolean ret = false;
			int currentTxid = 0;
			// Reset current txn ID at end if internal txn used for autocommit
			// duties
			Object[] lobObjects;
			validateExecuteInvocation();
			try {
				synchronized (connection_) {
/* Linux port - ToDo tmf.jar related
					tx = new Current();
					txnState = tx.get_status();
*/

					//**********************************************************
					// *****************
					// * If LOB is involved with autocommit enabled an no
					// external Txn,
					// * we must perform the base table (execute) and LOB table
					// (populateLobObjects)
					// * updates/inserts as a single unit of work (data
					// integrity issue).
					// * These updates/inserts will be performed inside an
					// internal transaction
					// * with autocommit disabled. If an SQL/FS exception is
					// caught during the
					// * update/insert, the transaction will be rolled back and
					// the exception will
					// * be reported. Upon success the transaction will be
					// committed and autocommit
					// * will be re-enabled.
					//**********************************************************
					// *****************
/* Linux port - ToDo tmf.jar related
					if (isAnyLob_ && (txnState == Current.StatusNoTransaction)
							&& (connection_.autoCommit_)) {
						currentTxid = connection_.getTxid();
						connection_.setTxid_(0);
						tx.begin();
						txBegin = true;
						connection_.autoCommit_ = false;
					}
*/
					long beginTime=0,endTime,timeTaken;
					if (connection_.t2props.getQueryExecuteTime() > 0)
						beginTime=System.currentTimeMillis();
					boolean currentAC = connection_.autoCommit_;
					if (isAnyLob_) {
						if (outputDesc_ != null)
							throw Messages.createSQLException(connection_.locale_, "lob_as_param_not_support", 
								null);
						else {
							try {
								lobLocators_ = getLobLocators();
								setLobLocators();
							}	
							finally {
							}
						}
					}	
					else {
					// Allocate the result set incase any rows are returned by
					// the execute
						if (outputDesc_ != null)
							resultSet_ = new SQLMXResultSet(this, outputDesc_);
						else
							resultSet_ = null;

						if (inputDesc_ != null) {
							execute(connection_.server_, connection_.getDialogueId(),
								connection_.getTxid(),
								connection_.autoCommit_,
								connection_.transactionMode_, stmtId_,
								cursorName_, isSelect_, paramRowCount_ + 1,
								inputDesc_.length, getParameters(),
								queryTimeout_, isAnyLob_,
								connection_.iso88591EncodingOverride_,
								resultSet_, false);
						} else {
							execute(connection_.server_, connection_.getDialogueId(),
								connection_.getTxid(),
								connection_.autoCommit_,
								connection_.transactionMode_, stmtId_,
								cursorName_, isSelect_, paramRowCount_ + 1, 0,
								null, queryTimeout_, isAnyLob_,
								connection_.iso88591EncodingOverride_,
								resultSet_, false);
						}
					}
					if (isAnyLob_)
						populateLobObjects();
					if (connection_.t2props.getQueryExecuteTime() > 0) {
						endTime = System.currentTimeMillis();
						timeTaken = endTime - beginTime;
						printQueryExecuteTimeTrace(timeTaken);
					}

					if (resultSet_ != null) 
						ret = true;
				}
			} finally {
/*
				if (currentTxid != 0) {
					connection_.setTxid_(currentTxid);
				}
*/
			}

			return ret;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_execute].methodExit();
		}
	}

	public boolean execute(String sql) throws SQLException {
		System.out
				.println("Trafodion recommends that the JDBC Type 2 driver not be used in this context.");
		System.out
				.println("The query ' "
						+ sql
						+ " ' will be ignored as PreparedStatement.execute(), does not take a string parameter.");
		return (execute());
	}

	/**
	 * Submit a batch of commands to the database for execution. This method is
	 * inherited from interface java.sql.Statement.
	 *
	 * @return an array of update counts containing one element for each command
	 *         in the batch. The elements of the array are ordered according to
	 *         the order in which commands were inserted into the batch
	 *
	 * @throws SQLException
	 *             if a database access error occurs, or the driver does not
	 *             support batch statements
	 * @throws BatchUpdateException
	 *             (a subclass of <tt>SQLException</tt>) if one of the commands
	 *             sent to the database fails to execute properly or attempts to
	 *             return a result set.
	 * @see java.sql.Statement#executeBatch()
	 */
	public int[] executeBatch() throws SQLException, BatchUpdateException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_executeBatch].methodEntry();
		try {
			clearWarnings();
			SQLException se;
/* Linux port - ToDo tmf.jar related
			Current tx = null;
*/
			int txnState = -1; // holds TMF transaction state code
			boolean txBegin = false; // flag to indicate internal autcommit
			// duties
			int currentTxid = 0;
			// Reset current txn ID at end if internal txn used for autocommit
			// duties
			boolean contBatchOnError = false;
			if (inputDesc_ == null) {
				se = Messages.createSQLException(connection_.locale_,
						"batch_command_failed", null);
				throw new BatchUpdateException(se.getMessage(), se
						.getSQLState(), new int[0]);
			}
/* Selva
			// Throw a exception if it is a select statement
			if (isSelect_) {
				se = Messages.createSQLException(connection_.locale_,
						"select_in_batch_not_supported", null);
				throw new BatchUpdateException(se.getMessage(), se
						.getSQLState(), new int[0]);
			}
*/
			if (connection_.isClosed_) {

				se = Messages.createSQLException(connection_.locale_,
						"invalid_connection", null);
				throw new BatchUpdateException(se.getMessage(), se
						.getSQLState(), new int[0]);
			}
			synchronized (connection_) {
				try {
/* Linux port - ToDo tmf.jar related
					tx = new Current();
					txnState = tx.get_status();
*/
					//**********************************************************
					// *****************
					// * If LOB is involved with autocommit enabled an no
					// external Txn, we must
					// * perform the base table (execute) and LOB table
					// (populateLobObjects)
					// * updates/inserts as a single unit of work (data
					// integrity issue).
					// * These updates/inserts will be performed inside an
					// internal transaction
					// * with autocommit disabled. If an SQL/FS exception is
					// caught during the
					// * update/insert, the transaction will be rolled back and
					// the exception will
					// * be reported. Upon success the transaction will be
					// committed and autocommit
					// * will be re-enabled.
					//**********************************************************
					// *****************
/* Linux port - ToDo tmf.jar related
					if (isAnyLob_ && (txnState == Current.StatusNoTransaction)
							&& (connection_.autoCommit_)) {
						currentTxid = connection_.getTxid();
						connection_.setTxid_(0);
						tx.begin();
						txBegin = true;
						connection_.autoCommit_ = false;
					}
*/
					if (connection_.contBatchOnErrorval_ == true)
						contBatchOnError = true;

					long beginTime=0,endTime,timeTaken;
					if(connection_.t2props.getQueryExecuteTime() > 0){
					beginTime=System.currentTimeMillis();
					}
					if (isAnyLob_) {
						if (outputDesc_ != null)
							throw Messages.createSQLException(connection_.locale_, "lob_as_param_not_support", 
								null);
						else {
							try {
								lobLocators_ = getLobLocators();
								setLobLocators();
							}	
							finally {
							}
						}
					}	
					else {
					// Allocate the result set incase any rows are returned by
					// the execute
						if (outputDesc_ != null)
							resultSet_ = new SQLMXResultSet(this, outputDesc_);
						else
							resultSet_ = null;

						execute(connection_.server_, connection_.getDialogueId(),
							connection_.getTxid(), connection_.autoCommit_,
							connection_.transactionMode_, stmtId_, cursorName_,
							isSelect_, paramRowCount_, inputDesc_.length,
							getParameters(), queryTimeout_,
							isAnyLob_,
							connection_.iso88591EncodingOverride_, resultSet_,
							contBatchOnError);
					}
					if (isAnyLob_)
						populateLobObjects();
					if(connection_.t2props.getQueryExecuteTime() > 0){
						endTime = System.currentTimeMillis();
						timeTaken = endTime - beginTime;
						printQueryExecuteTimeTrace(timeTaken);
					}

					//**********************************************************
					// *****************
					// * If LOB is involved with AutoCommit enabled an no
					// external Txn,
					// * commit transaction and re-enable autocommit
					//**********************************************************
					// *****************
					if (txBegin) {
						connection_.autoCommit_ = true;
/* Linux port - ToDo tmf.jar related
						tx.commit(false);
*/
						txBegin = false;
					}
				}
/* Linux port - ToDo tmf.jar related
				catch (com.tandem.util.FSException fe1) {
					SQLException se1 = null;
					SQLException se2 = null;

					Object[] messageArguments1 = new Object[2];
					messageArguments1[0] = Short.toString(fe1.error);
					messageArguments1[1] = fe1.getMessage();
					se1 = Messages.createSQLException(connection_.locale_,
							"transaction_error_update", messageArguments1);

					try {
						if (txBegin)
							tx.rollback();
					} catch (com.tandem.util.FSException fe2) {
						Object[] messageArguments2 = new Object[2];
						messageArguments2[0] = Short.toString(fe2.error);
						messageArguments2[1] = fe2.getMessage();
						se2 = Messages.createSQLException(connection_.locale_,
								"transaction_error_update", messageArguments2);
						se2.setNextException(se1);
						throw se2;
					}

					throw se1;
				}
*/
				catch (SQLException e) {
					BatchUpdateException be;
					SQLException se1 = null;

					se = Messages.createSQLException(connection_.locale_,
							"batch_command_failed", null);
					if (batchRowCount_ == null)
						batchRowCount_ = new int[0];
					be = new BatchUpdateException(se.getMessage(), se
							.getSQLState(), batchRowCount_);
					be.setNextException(e);

/* Linux port - ToDo tmf.jar related
					try {
						if (txBegin)
							tx.rollback();
					} catch (com.tandem.util.FSException fe2) {
						Object[] messageArguments = new Object[2];
						messageArguments[0] = Short.toString(fe2.error);
						messageArguments[1] = fe2.getMessage();
						se1 = Messages.createSQLException(connection_.locale_,
								"transaction_error_update", messageArguments);
						se1.setNextException(be);
						throw se1;
					}
*/
					throw be;
				} finally {
					if (currentTxid != 0) {
						connection_.setTxid_(currentTxid);
					}
				}
			}// End sync

			// If no statements to execute, throw an exception
			if (batchRowCount_ == null) {
				se = Messages.createSQLException(connection_.locale_,
						"batch_command_failed", null);
				throw new BatchUpdateException(se.getMessage(), se
						.getSQLState(), new int[0]);
			}

			return batchRowCount_;
		} finally {
			/*
			 * Description: executeBatch() Now resets
			 * current batch elements list after execution
			 */
			clearBatch();
			if (JdbcDebugCfg.entryActive)
				debug[methodId_executeBatch].methodExit();
		}
	}

	/**
	 * Executes the SQL query in this <tt>PreparedStatement</tt> object and
	 * returns the <tt>ResultSet</tt> object generated by the query. This is the
	 * perfered method to use for all queries that return a <tt>ResultSet</tt>.
	 *
	 * @return a <tt>ResultSet</tt> object that contains the data produced by
	 *         the query; never <tt>null</tt>
	 *
	 * @throws SQLException
	 *             if the statement is not a select
	 * @throws SQLException
	 *             if a database access error occurs or the SQL statement does
	 *             not return a <tt>ResultSet</tt> object
	 *
	 * @see java.sql.PreparedStatement#executeQuery()
	 */
	public ResultSet executeQuery() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_executeQuery].methodEntry();
		try {
			validateExecuteInvocation();
			if (!isSelect_){
				throw Messages.createSQLException(connection_.locale_,
						"non_select_invalid", null);
			}

			// Allocate the result set incase any rows are returned by the
			// execute
			if (outputDesc_ != null)
				resultSet_ = new SQLMXResultSet(this, outputDesc_);
			else {
				executeUpdate();
				resultSet_ = null;
				return resultSet_;
			}

			long beginTime=0,endTime,timeTaken;
//			if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
			if(connection_.t2props.getQueryExecuteTime() > 0){
			beginTime=System.currentTimeMillis();
			}
			synchronized (connection_) {
				if (inputDesc_ != null) {
					execute(connection_.server_, connection_.getDialogueId(),
							connection_.getTxid(), connection_.autoCommit_,
							connection_.transactionMode_, stmtId_, cursorName_,
							isSelect_, paramRowCount_ + 1, inputDesc_.length,
							getParameters(), queryTimeout_, isAnyLob_,
							connection_.iso88591EncodingOverride_, resultSet_,
							false);
				} else {
					execute(connection_.server_, connection_.getDialogueId(),
							connection_.getTxid(), connection_.autoCommit_,
							connection_.transactionMode_, stmtId_, cursorName_,
							isSelect_, paramRowCount_ + 1, 0, null,
							queryTimeout_, isAnyLob_,
							connection_.iso88591EncodingOverride_, resultSet_,
							false);
				}
			}// End sync

//			if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
			if(connection_.t2props.getQueryExecuteTime() > 0){
				endTime = System.currentTimeMillis();
				timeTaken = endTime - beginTime;
				printQueryExecuteTimeTrace(timeTaken);
			}
			return resultSet_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_executeQuery].methodExit();
		}
	}

	/**
	 * Executes the SQL statement in this <tt>PreparedStatement</tt> object,
	 * which must be a SQL <tt>INSERT</tt>, <tt>UPDATE</tt>, or <tt>DELETE</tt>
	 * statement; or a SQL statement that returns nothing, such as a DDL
	 * statement. This is the perfered method to use for all DML and DDL
	 * operations.
	 *
	 * @return either (1) the row count for <tt>INSERT</tt>, <tt>UPDATE</tt>, or
	 *         <tt>DELETE</tt> statements or (2) 0 for SQL statements that
	 *         return nothing
	 *
	 * @throws SQLException
	 *             invalid select if the statement is not a <tt>INSERT</tt>,
	 *             <tt>UPDATE</tt>, or <tt>DELETE</tt>
	 * @throws SQLException
	 *             if a database access error occurs or the SQL statement
	 *             returns a <tt>ResultSet</tt> object
	 * @see java.sql.PreparedStatement#executeUpdate()
	 */
	public int executeUpdate() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_executeUpdate].methodEntry();
		try {
/* Linux port - ToDo tmf.jar related
			Current tx = null;
*/
			int txnState = -1; // holds TMF transaction state code
			boolean txBegin = false; // flag to indicate internal autcommit
			// duties
			int currentTxid = 0;
			// Reset current txn ID at end if internal txn used for autocommit
			// duties

			validateExecuteInvocation();
			if (isSelect_)
				throw Messages.createSQLException(connection_.locale_,
						"select_invalid", null);

			// Allocate the result set incase any rows are returned by the
			// execute
			if (outputDesc_ != null)
				resultSet_ = new SQLMXResultSet(this, outputDesc_);
			else
				resultSet_ = null;

			synchronized (connection_) {
				if (inputDesc_ != null) {
					try {
/* Linux port - ToDo tmf.jar related
						tx = new Current();
						txnState = tx.get_status();
*/

						//******************************************************
						// *********************
						// * If LOB is involved with autocommit enabled an no
						// external Txn,
						// * we must perform the base table (execute) and LOB
						// table (populateLobObjects)
						// * updates/inserts as a single unit of work (data
						// integrity issue).
						// * These updates/inserts will be performed inside an
						// internal transaction
						// * with autocommit disabled. If an SQL/FS exception is
						// caught during the
						// * update/insert, the transaction will be rolled back
						// and the exception will
						// * be reported. Upon success the transaction will be
						// committed and autocommit
						// * will be re-enabled.
						//******************************************************
						// *********************
/* Linux port - ToDo tmf.jar related
						if (isAnyLob_
								&& (txnState == Current.StatusNoTransaction)
								&& (connection_.autoCommit_)) {
							currentTxid = connection_.getTxid();
							connection_.setTxid_(0);

							tx.begin();
							txBegin = true;
							connection_.autoCommit_ = false;
						}
*/
						if (isAnyLob_) {
							if (outputDesc_ != null)
								throw Messages.createSQLException(connection_.locale_, "lob_as_param_not_support", 
									null);
							else {
								try {
									lobLocators_ = getLobLocators();
									setLobLocators();
								}	
								finally {
								}
							}
						}	
						else {
							long beginTime=0,endTime,timeTaken;
							if(connection_.t2props.getQueryExecuteTime() > 0){
								beginTime=System.currentTimeMillis();
							}
							execute(connection_.server_, connection_.getDialogueId(),
								connection_.getTxid(),
								connection_.autoCommit_,
								connection_.transactionMode_, stmtId_,
								cursorName_, isSelect_, paramRowCount_ + 1,
								inputDesc_.length, getParameters(),
								queryTimeout_, isAnyLob_,
								connection_.iso88591EncodingOverride_,
								resultSet_, false);

							if (connection_.t2props.getQueryExecuteTime() > 0){
								endTime = System.currentTimeMillis();
								timeTaken = endTime - beginTime;
								printQueryExecuteTimeTrace(timeTaken);
							}
						}
						if (isAnyLob_)
							populateLobObjects();

						//******************************************************
						// *********************
						// * If LOB is involved with AutoCommit enabled an no
						// external Txn,
						// * commit transaction and re-enable autocommit
						//******************************************************
						// *********************
						if (txBegin) {
							connection_.autoCommit_ = true;
/* Linux port - ToDo tmf.jar related
							tx.commit(false);
*/
							txBegin = false;
						}
					}
/* Linux port - ToDo tmf.jar related
					catch (com.tandem.util.FSException fe1) {
						SQLException se1 = null;
						SQLException se2 = null;

						Object[] messageArguments1 = new Object[2];
						messageArguments1[0] = Short.toString(fe1.error);
						messageArguments1[1] = fe1.getMessage();
						se1 = Messages.createSQLException(connection_.locale_,
								"transaction_error_update", messageArguments1);

						try {
							if (txBegin)
								tx.rollback();
						} catch (com.tandem.util.FSException fe2) {
							Object[] messageArguments2 = new Object[2];
							messageArguments2[0] = Short.toString(fe2.error);
							messageArguments2[1] = fe2.getMessage();
							se2 = Messages.createSQLException(
									connection_.locale_,
									"transaction_error_update",
									messageArguments2);
							se2.setNextException(se1);
							throw se2;
						}

						throw se1;
					}
*/
					catch (SQLException se) {
						SQLException se2 = null;

/* Linux port - ToDo tmf.jar related
						try {
							if (txBegin)
								tx.rollback();
						} catch (com.tandem.util.FSException fe2) {
							Object[] messageArguments = new Object[2];
							messageArguments[0] = Short.toString(fe2.error);
							messageArguments[1] = fe2.getMessage();
							se2 = Messages.createSQLException(
									connection_.locale_,
									"transaction_error_update",
									messageArguments);
							se2.setNextException(se);
							throw se2;
						}
*/
						throw se;
					} finally {
						if (currentTxid != 0) {
							connection_.setTxid_(currentTxid);
						}
					}
				} else {
					long beginTime=0,endTime,timeTaken;
//					if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
					if(connection_.t2props.getQueryExecuteTime() > 0){
					beginTime=System.currentTimeMillis();
					}
					execute(connection_.server_, connection_.getDialogueId(),
							connection_.getTxid(), connection_.autoCommit_,
							connection_.transactionMode_, stmtId_, cursorName_,
							isSelect_, paramRowCount_ + 1, 0, null,
							queryTimeout_, isAnyLob_,
							connection_.iso88591EncodingOverride_, resultSet_,
							false);
//					if ((T2Driver.queryExecuteTime_ > 0) || (SQLMXDataSource.queryExecuteTime_> 0) ) {
					if(connection_.t2props.getQueryExecuteTime() > 0){
						endTime = System.currentTimeMillis();
						timeTaken = endTime - beginTime;
						printQueryExecuteTimeTrace(timeTaken);
					}
				}
			}// End sync
			if ((batchRowCount_ == null) || (batchRowCount_.length == 0))
				return 0;
			return batchRowCount_[0];

		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_executeUpdate].methodExit();
		}
	}

	public int executeUpdate(String sql) throws SQLException {
		System.out
				.println("Trafodion recommends that the JDBC Type 2 driver not be used in this context.");
		System.out
				.println("The query ' "
						+ sql
						+ " ' will be ignored as PreparedStatement.executeUpdate(), does not take a string parameter.");
		return (executeUpdate());
	}

	public ResultSetMetaData getMetaData() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMetaData].methodEntry();
		try {
			if (outputDesc_ == null)
				return null;
			return new SQLMXResultSetMetaData(this, outputDesc_);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMetaData].methodExit();
		}
	}

	public ParameterMetaData getParameterMetaData() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getParameterMetaData].methodEntry();
		try {
			if (inputDesc_ != null)
				return new SQLMXParameterMetaData(this, inputDesc_);
			return new SQLMXParameterMetaData(this, new SQLMXDesc[0]);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getParameterMetaData].methodExit();
		}
	}

	public void setArray(int parameterIndex, Array x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setArray].methodEntry();
		try {
			validateSetInvocation(parameterIndex);
			Messages.throwUnsupportedFeatureException(connection_.locale_,
					"setArray()");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setArray].methodExit();
		}
	}

	public void setAsciiStream(int parameterIndex, InputStream x, int length)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setAsciiStream].methodEntry();
		try {
			byte[] buffer;
			int dataType;

			validateSetInvocation(parameterIndex);
			dataType = inputDesc_[parameterIndex - 1].dataType_;

			switch (dataType) {
			case Types.CLOB:
				SQLMXClob clob = null;
				if (x == null) 
					paramContainer_.setNull(parameterIndex);
				else {
					if (length <= connection_.getInlineLobChunkSize()) {
						try {
							int bufLength; 
							byte[] buf = new byte[length];
							bufLength = x.read(buf);
							String inStr;
							if (bufLength == length)
								inStr = new String(buf);
							else
								inStr = new String(buf, 0, bufLength);
							paramContainer_.setString(parameterIndex, inStr);
						} catch (IOException ioe) {
							throw new SQLException(ioe);
						}
					} else {			
						isAnyLob_ = true;
						clob = new SQLMXClob(connection_, null, x, length);
						inputDesc_[parameterIndex - 1].paramValue_ = "";
						paramContainer_.setString(parameterIndex, "");
					}
				}
				addLobObjects(parameterIndex, clob);
				break;
			case Types.BLOB:
				throw Messages.createSQLException(connection_.locale_,
						"restricted_data_type", null);
			case Types.CHAR:
			case Types.VARCHAR:
			case Types.LONGVARCHAR:
			case Types.BINARY: // At this time SQL/MX does not have this column data type
			case Types.VARBINARY: // At this time SQL/MX does not have this column data type
			case Types.LONGVARBINARY: // At this time SQL/MX does not have this column data type
				if (x == null) 
					paramContainer_.setNull(parameterIndex);
				else {
					buffer = new byte[length];
					try {
						x.read(buffer);
					} catch (java.io.IOException e) {
						Object[] messageArguments = new Object[1];
						messageArguments[0] = e.getMessage();
						throw Messages.createSQLException(connection_.locale_,
								"io_exception", messageArguments);
					}
					try {
						paramContainer_.setString(parameterIndex, new String(
								buffer, "ASCII"));
					} catch (java.io.UnsupportedEncodingException e) {
						Object[] messageArguments = new Object[1];
						messageArguments[0] = e.getMessage();
						throw Messages.createSQLException(connection_.locale_,
								"unsupported_encoding", messageArguments);
					}
				}
				break;
			default:
				throw Messages.createSQLException(connection_.locale_,
							"invalid_datatype_for_column", null);
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setAsciiStream].methodExit();
		}
	}

	public void setBigDecimal(int parameterIndex, BigDecimal x)
			throws SQLException {
		/*
		 * Note: The SQL/MX DECIMAL is stored as an ASCII string, padded with
		 * leading ASCII zero. A negative value is indicated by doing an or of
		 * 0x80 on the first byte of the DECIMAL string. The negative value is
		 * set in the C routine, since it much easier to do in C than Java.
		 *
		 * SQL/MX tell me that the only column types that we will see for the
		 * DECIMAL is SQLTYPECODE_DECIMAL and SQLTYPECODE_DECIMAL_UNSIGNED. We
		 * should not get any of the other DECIMAL data types and can be
		 * ignored.
		 *
		 * For the TYPE_FS (see sqlcli.h) only the _SQLDT_DEC_U will be set when
		 * the column is a DECIMAL UNSIGNED. The _SQLDT_DEC_LSS, _SQLDT_DEC_LSE,
		 * _SQLDT_DEC_TSS, and _SQLDT_DEC_TSE are only used host variable
		 * declaration. 
		 *
		 * SQL/MX stores the NUMERIC data type in three different storage sizes,
		 * which are 16 bit, 32 bit and 64 bit. The storage size depends upon
		 * the precision and scale specified at the time when the column was
		 * created.
		 */
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setBigDecimal].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setBigDecimal].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?");
		try {
			int dataType;
			long numValue = 0;
			BigDecimal y = null; // Used to rescale input value if it does not
			// match column scale
			BigInteger bigNum = null; // Used to store the unscaled value

			validateSetInvocation(parameterIndex); // Need to check to see that
			// parameterIndex is valid
			dataType = inputDesc_[parameterIndex - 1].dataType_; // Get the SQL
			// column
			// data type

			if (x == null) {
				paramContainer_.setNull(parameterIndex);
			} else {
				if (JdbcDebugCfg.traceActive)
					debug[methodId_setBigDecimal].traceOut(
							JdbcDebug.debugLevelData, "p-1: "
									+ (parameterIndex - 1)
									+ " inputDesc_[p-1].scale_: "
									+ inputDesc_[parameterIndex - 1].scale_
									+ " (short) x.scale() : "
									+ (short) x.scale());
				switch (dataType) {
				case Types.DECIMAL:
				case Types.NUMERIC: // The scale must match the column or
					// parameter
					int fsDataType = inputDesc_[parameterIndex - 1].fsDataType_; // This
					// variable
					// contains
					// additional
					// information
					// about
					// column

					if (inputDesc_[parameterIndex - 1].scale_ != (short) x
							.scale()) // If the columnType scale is not the same
					// as the parameter scale
					{
						if (x.scale() < 0) // if the scale is neg.
						{ // the unscaledValue is the proper scale used in the
							// database.
							bigNum = x.unscaledValue(); // Note: Min and Max
							// checks are done later
							// in this routine.
							if (JdbcDebugCfg.traceActive)
								debug[methodId_setBigDecimal].traceOut(
										JdbcDebug.debugLevelData, "bigNum: "
												+ bigNum + " x: " + x);
						} else {
							try {
								y = x
										.setScale(inputDesc_[parameterIndex - 1].scale_); // adjust
								// the
								// scale
								// so
								// that
								// the
								// parameter
								// is
								// equal
								// to
								// the
								// column
								// scale
							} catch (ArithmeticException a1) {
								Object[] parmIndex = new Object[2];
								parmIndex[0] = x;
								parmIndex[1] = new Integer(parameterIndex);
								Messages.createSQLWarning(connection_.locale_,
										"rounded_half_up", parmIndex);
								y = x.setScale(
										inputDesc_[parameterIndex - 1].scale_,
										RoundingMode.HALF_UP);
							}
							bigNum = y.unscaledValue();
							if (JdbcDebugCfg.traceActive)
								debug[methodId_setBigDecimal].traceOut(
										JdbcDebug.debugLevelData, "bigNum: "
												+ bigNum + " y: " + y);
						}
					} else {
						bigNum = x.unscaledValue();
						if (JdbcDebugCfg.traceActive)
							debug[methodId_setBigDecimal].traceOut(
									JdbcDebug.debugLevelData, "bigNum: "
											+ bigNum + " x: " + x);
					}
					// Big Num Changes
//					if ((bigNum.compareTo(decIntMaxLong) == 1) || // Is greater
							// than
							// 9223372036854775807
//							(bigNum.compareTo(decIntMinLong) == -1)) // Is less
					// than
					// -9223372036854775808
//					{
//						Object[] parmIndex = new Object[2];
//						parmIndex[0] = x;
//						parmIndex[1] = new Integer(parameterIndex);
//						throw Messages.createSQLException(connection_.locale_,
//								"numeric_value_out_of_range", parmIndex);
//					}
					// Big Num Changes
					numValue = bigNum.longValue(); // Convert BigInteger to a 64
					// bit number

					if (dataType == Types.NUMERIC) {
						switch (fsDataType) // SQL/MX uses different sizes to
						// store
						{ // a NUMERIC, depending on the precision
						case SQLMXDesc.SQLDT_16BIT_SIGNED: // Check to make sure
							// value does not
							// exceed 16 bits
							// singed
							if ((numValue > (long) Short.MAX_VALUE)
									|| (numValue < (long) Short.MIN_VALUE)) {
								Object[] parmIndex = new Object[2];
								parmIndex[0] = x;
								parmIndex[1] = new Integer(parameterIndex);
								throw Messages
										.createSQLException(
												connection_.locale_,
												"numeric_value_out_of_range",
												parmIndex);
							}
							break;
						case SQLMXDesc.SQLDT_16BIT_UNSIGNED: // Check to make
							// sure value
							// does not
							// exceed 16
							// bits unsigned
							if (numValue > unsignedShortMaxValue) // Java does
							// not have
							// any
							// unsigned
							// data
							// types
							{
								Object[] parmIndex = new Object[2];
								parmIndex[0] = x;
								parmIndex[1] = new Integer(parameterIndex);
								throw Messages
										.createSQLException(
												connection_.locale_,
												"numeric_value_out_of_range",
												parmIndex);
							} else if (numValue < unsignedShortMinValue) {
								Object[] parmIndex = new Object[2];
								parmIndex[0] = x;
								parmIndex[1] = new Integer(parameterIndex);
								throw Messages.createSQLException(
										connection_.locale_,
										"negative_to_unsigned", parmIndex);
							}
							break;
						case SQLMXDesc.SQLDT_32BIT_SIGNED: // Check to make sure
							// value does not
							// exceed 32 bits
							// signed
							if ((numValue > (long) Integer.MAX_VALUE)
									|| (numValue < (long) Integer.MIN_VALUE)) {
								Object[] parmIndex = new Object[2];
								parmIndex[0] = x;
								parmIndex[1] = new Integer(parameterIndex);
								throw Messages
										.createSQLException(
												connection_.locale_,
												"numeric_value_out_of_range",
												parmIndex);
							}
							break;
						case SQLMXDesc.SQLDT_32BIT_UNSIGNED: // Check to make
							// sure value
							// does not
							// exceed 32
							// bits unsigned
							if (numValue > unsignedIntMaxValue) // Java does not
							// have any
							// unsigned data
							// types
							{
								Object[] parmIndex = new Object[2];
								parmIndex[0] = x;
								parmIndex[1] = new Integer(parameterIndex);
								throw Messages
										.createSQLException(
												connection_.locale_,
												"numeric_value_out_of_range",
												parmIndex);
							} else if (numValue < unsignedIntMinValue) {
								Object[] parmIndex = new Object[2];
								parmIndex[0] = x;
								parmIndex[1] = new Integer(parameterIndex);
								throw Messages.createSQLException(
										connection_.locale_,
										"negative_to_unsigned", parmIndex);
							}
							break;
						case SQLMXDesc.SQLDT_64BIT_SIGNED:
							break;
							// Big Num Changes
						case SQLMXDesc.SQLDT_NUM_BIG_S:
						case SQLMXDesc.SQLDT_NUM_BIG_U:
							paramContainer_.setBigDecimal(parameterIndex, x);
							break;
							// Big Num Changes
						default:
							throw Messages.createSQLException(
									connection_.locale_,
									"invalid_datatype_for_column", null);
						}
						// Big Num Changes if condition
						if(fsDataType != SQLMXDesc.SQLDT_NUM_BIG_S
								&& fsDataType != SQLMXDesc.SQLDT_NUM_BIG_U) {
						paramContainer_.setLong(parameterIndex, numValue);
						}
						break;
					} else // The column type is a SQLTYPECODE_DECIMAL or
					// SQLTYPECODE_DECIMAL_UNSIGNED
					{
						if ((!inputDesc_[parameterIndex - 1].isSigned_)
								&& (numValue < 0)) // Is this column unsigned
						// and the data value is
						// negative
						{
							Object[] parmIndex = new Object[2];
							parmIndex[0] = x;
							parmIndex[1] = new Integer(parameterIndex);
							throw Messages.createSQLException(
									connection_.locale_,
									"negative_to_unsigned", parmIndex);
						}
						// Set the number of digits that SQL/MX expects
						nf
								.setMinimumIntegerDigits(inputDesc_[parameterIndex - 1].precision_);
						// Convert to a ASCII numeric string with zero padding
						paramContainer_.setString(parameterIndex, nf
								.format(numValue));
						break;
					}

				default:
					setObject(parameterIndex, x, dataType); // Data Object does
					// not match SQL
					// column type
					break;
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setBigDecimal].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given input stream, which will have
	 * the specified number of bytes. When a very large binary value is input to
	 * a <tt>LONGVARBINARY</tt> parameter, it may be more practical to send it
	 * via a <tt>java.io.InputStream</tt>. Data will be read from the stream as
	 * needed until end-of-file is reached.
	 * <P>
	 * <B>Note:</B> This stream object can either be a standard Java stream
	 * object or your own subclass that implements the standard interface.
	 * </p>
	 * <p>
	 * The stream can be used to write <tt>BLOB</tt>, <tt>CHAR</tt>,
	 * <tt>VARCHAR</tt>, or <tt>LONGVARCHAR</tt> SQL column types. The Trafodion JDBC
	 * driver does not support <tt>BINARY</tt>, <tt>VARBINARY</tt>, or
	 * <tt>LONGVARBINARY</tt> SQL column types, at this time.
	 * </p>
	 * <p>
	 * The SQL column types <tt>DOUBLE</tt>, <tt>FLOAT</tt>, <tt>REAL</tt>,
	 * <tt>DECIMAL</tt>, and <tt>NUMERIC</tt> can not be used to input data from
	 * the stream.
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param x
	 *            the Java input stream that contains the binary parameter value
	 * @param length
	 *            the number of bytes in the stream
	 *
	 * @exception SQLException
	 *                restricted data type, invalid data type for column, or I/O
	 *                error
	 */
	public void setBinaryStream(int parameterIndex, InputStream x, int length)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setBinaryStream].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setBinaryStream].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?");
		try {
			byte[] buffer;
			int dataType;

			validateSetInvocation(parameterIndex);
			dataType = inputDesc_[parameterIndex - 1].dataType_;

			switch (dataType) {
			case Types.CLOB:
				throw Messages.createSQLException(connection_.locale_,
						"restricted_data_type", null);
			case Types.BLOB:
				SQLMXBlob blob = null;
				if (x == null) 
					paramContainer_.setNull(parameterIndex);
				else {
					if (length <= connection_.getInlineLobChunkSize()) {
						try {
							int bufLength; 
							byte[] buf = new byte[length];
							bufLength = x.read(buf);
							byte[] inBuf;
							if (bufLength == length)
								inBuf = buf;
							else
								inBuf = Arrays.copyOf(buf, bufLength);
							paramContainer_.setBytes(parameterIndex, inBuf);
						} catch (IOException ioe) {
							throw new SQLException(ioe);
						}
					} else {			
						isAnyLob_ = true;
						blob = new SQLMXBlob(connection_, null, x, length);
						inputDesc_[parameterIndex - 1].paramValue_ = "";
						paramContainer_.setBytes(parameterIndex, new byte[0]);
					}
				}
				addLobObjects(parameterIndex, blob);
				break;
			case Types.DOUBLE:
			case Types.DECIMAL:
			case Types.NUMERIC:
			case Types.FLOAT:
			case Types.BIGINT:
			case Types.INTEGER:
			case Types.SMALLINT:
			case Types.TINYINT:
				throw Messages.createSQLException(connection_.locale_,
							"invalid_datatype_for_column", null);
			default:
				if (x == null) { 
					paramContainer_.setNull(parameterIndex);
				}
				else {
					buffer = new byte[length];

					try {
						x.read(buffer);
					} catch (java.io.IOException e) {
						Object[] messageArguments = new Object[1];
						messageArguments[0] = e.getMessage();
						throw Messages.createSQLException(connection_.locale_,
								"io_exception", messageArguments);
					}
					paramContainer_.setString(parameterIndex,
							new String(buffer));
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setBinaryStream].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given <tt>Blob</tt> object. The
	 * driver converts this to an SQL <tt>BLOB</tt> value when it sends it to
	 * the database.
	 *
	 * @param i
	 *            the first parameter is 1, the second is 2, ...
	 * @param x
	 *            a <tt>Blob</tt> object that maps an SQL <tt>BLOB</tt> value
	 *
	 * @throws SQLException
	 *             invalid data type for column
	 */
	public void setBlob(int parameterIndex, Blob x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setBlob].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setBlob].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?");
		try {
			validateSetInvocation(parameterIndex);
			int dataType = inputDesc_[parameterIndex - 1].dataType_;
			dataType = inputDesc_[parameterIndex - 1].dataType_;
			switch (dataType) {
			case Types.BLOB:
				SQLMXBlob blob = null;
				isAnyLob_ = true;
				if (x == null) 
					setNull(parameterIndex, Types.BLOB);
				else {
					byte[] b = null;
					if ((x instanceof SQLMXBlob) && 
							((b = ((SQLMXBlob)x).getBytes(connection_.getInlineLobChunkSize())) != null)) {
						paramContainer_.setBytes(parameterIndex, b);
					} else {
						isAnyLob_ = true;
						blob = new SQLMXBlob(connection_, null, x);
						inputDesc_[parameterIndex - 1].paramValue_ = "";
						paramContainer_.setBytes(parameterIndex, new byte[0]);
					}
				}
				addLobObjects(parameterIndex, blob);
				break;
			default:
				throw Messages.createSQLException(connection_.locale_,
						"invalid_datatype_for_column", null);
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setBlob].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given Java <tt>boolean</tt> value.
	 * The driver converts this to a SQL <tt>BIT</tt> value when it sends it to
	 * the database.
	 * <P>
	 * <B>Note:</B> The Trafodion JDBC driver does not support a SQL column type of
	 * <tt>BIT</tt> and can not convert the <tt>boolean</tt> value to a SQL
	 * <tt>BIT</tt> value. The Trafodion JDBC driver does allow the use of the
	 * setBoolean method to write to the following SQL column types:
	 *
	 * <ul PLAIN> <tt>TINYINT</tt> </ul> <ul PLAIN> <tt>SMALLINT</tt> </ul> <ul
	 * PLAIN> <tt>INTEGER</tt> </ul> <ul PLAIN> <tt>BIGINT</tt> </ul> <ul PLAIN>
	 * <tt>REAL</tt> </ul> <ul PLAIN> <tt>DOUBLE</tt> </ul> <ul PLAIN>
	 * <tt>DECIMAL</tt> </ul> <ul PLAIN> <tt>NUMERIC</tt> </ul> <ul PLAIN>
	 * <tt>BOOLEAN</tt> </ul> <ul PLAIN> <tt>CHAR</tt> </ul> <ul PLAIN>
	 * <tt>VARCHAR</tt> </ul> <ul PLAIN> <tt>LONGVARCHAR</tt> </ul>
	 * </p>
	 * <p>
	 * The <tt>boolean</tt> value will be converted to the correct SQL column
	 * type.
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param x
	 *            the parameter value
	 *
	 * @throws java.sql.SQLException
	 *             invalid data type for column
	 */
	public void setBoolean(int parameterIndex, boolean x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setBoolean].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setBoolean].methodParameters(Integer
					.toString(parameterIndex)
					+ "," + Boolean.toString(x));
		try {
			dataWrapper.clearColumn(1);
			dataWrapper.setBoolean(1, x);
			setBoolean(parameterIndex);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setBoolean].methodExit();
		}
	}

	private void setBoolean(int parameterIndex) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setBoolean_I].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setBoolean_I].methodParameters("parameterIndex = "
					+ Integer.toString(parameterIndex) + ", wrapper value = "
					+ dataWrapper.getBoolean(1, null));
		try {
			int dataType;

			validateSetInvocation(parameterIndex);
			inputDesc_[parameterIndex - 1]
					.checkValidNumericConversion(connection_.locale_);
			dataType = inputDesc_[parameterIndex - 1].dataType_;

			switch (dataType) {
			case Types.DATE:
			case Types.TIME:
			case Types.TIMESTAMP:
				throw Messages.createSQLException(connection_.locale_,
						"invalid_datatype_for_column", null);
			case Types.BOOLEAN:
				paramContainer_.setString(parameterIndex, Boolean
						.toString(dataWrapper
								.getBoolean(1, connection_.locale_)));
				break;
			default:
				setObject(parameterIndex, dataWrapper, dataType);
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setBoolean_I].methodExit();
		}
	}

	/**
	 * Set a parameter to a Java byte value. The driver converts this to a SQL
	 * <tt>TINYINT</tt> value when it sends it to the database.
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param x
	 *            the parameter value
	 * @exception java.sql.SQLException
	 *                invalid data type for column
	 */

	public void setByte(int parameterIndex, byte x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setByte].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setByte].methodParameters(Integer
					.toString(parameterIndex)
					+ "," + Byte.toString(x));
		try {
			int dataType;

			validateSetInvocation(parameterIndex);
			inputDesc_[parameterIndex - 1]
					.checkValidNumericConversion(connection_.locale_);
			dataType = inputDesc_[parameterIndex - 1].dataType_;

			switch (dataType) {
			case Types.DATE:
			case Types.TIME:
			case Types.TIMESTAMP:
				throw Messages.createSQLException(connection_.locale_,
						"invalid_datatype_for_column", null);
			case Types.TINYINT:
				dataWrapper.clearColumn(1);
				dataWrapper.setByte(1, x);
				setObject(parameterIndex, dataWrapper, dataType);
				break;
			default:
				setObject(parameterIndex, new Byte(x), dataType);
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setByte].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given Java array of bytes. The
	 * driver converts this to a SQL <tt>VARBINARY</tt> or
	 * <tt>LONGVARBINARY</tt> (depending on the argument's size relative to the
	 * driver's limits on <tt>VARBINARY</tt>) when it sends it to the database.
	 * <P>
	 * <B>Note:</B> The Trafodion SQL/MX database <em><B>does not</B></em> support
	 * <tt>VARBINARY</tt> or <tt>LONVARBINARY</tt> at this time. An Trafodion
	 * extension, allows the use of the SQL column types <tt>VARCHAR</tt>,
	 * <tt>LONGVARCHAR</tt>, and <tt>BLOB</tt>.
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param x
	 *            the parameter value
	 *
	 * @exception SQLException
	 *                invalid data type for column
	 */
	public void setBytes(int parameterIndex, byte[] x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setBytes].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setBytes].methodParameters(Integer
					.toString(parameterIndex)
					+ ",byte[]");
		try {
			validateSetInvocation(parameterIndex);
			int dataType = inputDesc_[parameterIndex - 1].dataType_;
            		int dataCharSet = inputDesc_[parameterIndex - 1].sqlCharset_;

			if (x == null && dataType != Types.CLOB && dataType != Types.BLOB) {
				setNull(parameterIndex, java.sql.Types.LONGVARBINARY);
			}

			switch (dataType) {
			case Types.BLOB:
				SQLMXBlob blob = null;
				if (x == null) 
					paramContainer_.setNull(parameterIndex);
				else {
					if (x.length <= connection_.getInlineLobChunkSize()) 
						paramContainer_.setBytes(parameterIndex, x);
					else {
						isAnyLob_ = true;
						blob = new SQLMXBlob(connection_, null, x);
						inputDesc_[parameterIndex - 1].paramValue_ = "";
						paramContainer_.setBytes(parameterIndex, new byte[0]);
					}
				}
				addLobObjects(parameterIndex, blob);
				break;
			case Types.CHAR:
			case Types.VARCHAR:
			case Types.LONGVARCHAR:
				String charSet = SQLMXDesc.SQLCHARSETSTRING_ISO88591;
				if (dataCharSet == SQLMXDesc.SQLCHARSETCODE_UCS2)
					charSet = "UTF-16LE";
				try {
					x = (new String(x)).getBytes(charSet);
				} catch (UnsupportedEncodingException e) {
					throw Messages.createSQLException(connection_.locale_, "unsupported_encoding",
						new Object[] { charSet });
				}
				paramContainer_.setObject(parameterIndex, x);
				break;
			case Types.DATE:
			case Types.TIME:
			case Types.TIMESTAMP:
			case Types.BINARY:
			case Types.VARBINARY:
			case Types.LONGVARBINARY:
				paramContainer_.setObject(parameterIndex, x);
				break;
			default:
				throw Messages.createSQLException(connection_.locale_,
						"invalid_datatype_for_column", null);
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setBytes].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given <tt>Reader</tt> object, which
	 * is the number of characters long. When a very large UNICODE value is
	 * input to a <tt>LONGVARCHAR</tt> parameter, it may be more practical to
	 * send it via a <tt>java.io.Reader</tt> object. The data will be read from
	 * the stream as needed, until end-of-file. The JDBC driver will do any
	 * necessary conversion from UNICODE to the database char format.
	 *
	 * <P>
	 * <B>Note:</B> This stream object can either be a standard Java stream
	 * object or your own subclass that implements the standard interface.
	 * </p>
	 * <p>
	 * This method can not be used to read in data for the following SQL column
	 * types: <ul PLAIN> <tt>BLOB</tt> (<i>throws restricted data type</i>)
	 * </ul> <ul PLAIN> <tt>DECIMAL</tt> </ul> <ul PLAIN> <tt>DOUBLE</tt> </ul>
	 * <ul PLAIN> <tt>FLOAT</tt> </ul> <ul PLAIN> <tt>NUMERIC</tt> </ul>
	 * </p>
	 * <p>
	 * This method can be used to read in character data for the SQL column type
	 * <tt>CLOB</tt>.
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1, the second is 2, ...
	 * @param reader
	 *            the <tt>java.io.Reader</tt> object that contains the UNICODE
	 *            data
	 * @param length
	 *            the number of characters in the stream
	 *
	 * @exception SQLException
	 *                invalid data type for column, restricted data type, or I/O
	 *                error
	 */
	public void setCharacterStream(int parameterIndex, Reader reader, int length)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setCharacterStream].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setCharacterStream].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?," + Integer.toString(length));
		try {
			char[] buffer;
			int dataType;

			validateSetInvocation(parameterIndex);
			dataType = inputDesc_[parameterIndex - 1].dataType_;

			switch (dataType) {
			case Types.CLOB:
				SQLMXClob clob = null;
				if (reader == null) 
					paramContainer_.setNull(parameterIndex);
				else {
					if (length <= connection_.getInlineLobChunkSize()) {
						try {
							int bufLength; 
							char[] buf = new char[length];
							bufLength = reader.read(buf);
							String inStr;
							if (bufLength == length)
								inStr = new String(buf);
							else
								inStr = new String(buf, 0, bufLength);
							paramContainer_.setString(parameterIndex, inStr);
						} catch (IOException ioe) {
							throw new SQLException(ioe);
						}
					} else {
						isAnyLob_ = true;
						clob = new SQLMXClob(connection_, null, reader, length);
						inputDesc_[parameterIndex - 1].paramValue_ = "";
						paramContainer_.setString(parameterIndex, "");
					}
				}
				addLobObjects(parameterIndex, clob);
				break;
			case Types.BLOB:
				throw Messages.createSQLException(connection_.locale_,
							"restricted_data_type", null);
			case Types.DECIMAL:
			case Types.DOUBLE:
			case Types.FLOAT:
			case Types.NUMERIC:
			case Types.BIGINT:
			case Types.INTEGER:
			case Types.SMALLINT:
			case Types.TINYINT:
				throw Messages.createSQLException(connection_.locale_,
							"invalid_datatype_for_column", null);
			default:
				if (reader == null) 
					paramContainer_.setNull(parameterIndex);
				else {
				
					buffer = new char[length];
					try {
						reader.read(buffer);
					} catch (java.io.IOException e) {
						Object[] messageArguments = new Object[1];
						messageArguments[0] = e.getMessage();
						throw Messages.createSQLException(connection_.locale_,
								"io_exception", messageArguments);
					}
					paramContainer_.setString(parameterIndex,
							new String(buffer));
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setCharacterStream].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given <tt>Clob</tt> object. The
	 * driver converts this to an SQL <tt>CLOB</tt> value when it sends it to
	 * the database.
	 *
	 * @param i
	 *            the first parameter is 1, the second is 2, ...
	 * @param x
	 *            a <tt>Clob</tt> object that maps an SQL <tt>CLOB</tt>
	 *
	 * @throws SQLException
	 *             invalid data type for column, or restricted data type.
	 */
	public void setClob(int parameterIndex, Clob x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setClob].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setClob].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?");
		try {
			validateSetInvocation(parameterIndex);
			int dataType = inputDesc_[parameterIndex - 1].dataType_;
			switch (dataType) {
			case Types.CLOB:
				SQLMXClob clob = null;
				if (x == null) 
					setNull(parameterIndex, Types.CLOB);
				else {
					String inStr = null;
					if ((x instanceof SQLMXClob) && 
							((inStr = ((SQLMXClob)x).getString(connection_.getInlineLobChunkSize())) != null)) {
						paramContainer_.setString(parameterIndex, inStr);
					} else {
						isAnyLob_ = true;
						clob = new SQLMXClob(connection_, null, x);
						inputDesc_[parameterIndex - 1].paramValue_ = "";
						paramContainer_.setString(parameterIndex, "");
					}
				}
				addLobObjects(parameterIndex, clob);
				break;
			case Types.DECIMAL:
			case Types.DOUBLE:
			case Types.FLOAT:
			case Types.NUMERIC:
				throw Messages.createSQLException(connection_.locale_,
						"invalid_datatype_for_column", null);
			default:
				throw Messages.createSQLException(connection_.locale_,
						"restricted_data_type", null);
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setClob].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given Java <tt>java.sql.Date</tt>
	 * value. The driver converts this to a SQL <tt>DATE</tt> or SQL
	 * <tt>TIMESTAMP</tt> value when it sends it to the database.
	 *
	 * <p>
	 * <B>Note:</B> An extension to the Trafodion JDBC driver is that the Date
	 * parameter can be used to write to the following additional SQL column
	 * types:
	 *
	 * <ul PLAIN> <tt>CHAR</tt> </ul> <ul PLAIN> <tt>VARCHAR</tt> </ul> <ul
	 * PLAIN> <tt>LONVARCHAR</tt> </ul>
	 *
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param x
	 *            the parameter value
	 * @exception SQLException
	 *                Java data type does not match SQL data type for column
	 */
	public void setDate(int parameterIndex, Date x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setDate_IL].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setDate_IL].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?");
		try {
			int dataType;
			Timestamp t1;

			validateSetInvocation(parameterIndex);
			dataType = inputDesc_[parameterIndex - 1].dataType_;

			if (x == null) {
				paramContainer_.setNull(parameterIndex);
			} else {
				switch (dataType) {
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.DATE:
					paramContainer_.setString(parameterIndex, x.toString());
					break;
				case Types.TIMESTAMP:
					t1 = new Timestamp(x.getTime());
					paramContainer_.setString(parameterIndex, t1.toString());
					break;
				default:
					throw Messages.createSQLException(connection_.locale_,
							"invalid_datatype_for_column", null);
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setDate_IL].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given Java <tt>java.sql.Date</tt>
	 * value. The driver converts this to a SQL <tt>DATE</tt> value when it
	 * sends it to the database.
	 *
	 *<p>
	 * <B>Note:</B> The cal parameter is used, by the Trafodion JDBC driver from now.
	 *</p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1, the second is 2, ...
	 * @param x
	 *            the parameter value
	 * @param cal
	 *            the parameter value of calendar
	 * @exception SQLException
	 *                Java data type does not match SQL data type for column
	 */
	public void setDate(int parameterIndex, Date x, Calendar cal)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setDate_ILL].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setDate_ILL].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?,?");
		try {
			int dataType;
			Timestamp t1;

			validateSetInvocation(parameterIndex);
			dataType = inputDesc_[parameterIndex - 1].dataType_;

			if (x == null) {
				paramContainer_.setNull(parameterIndex);
			} else {
				Date adjustDate = null;
				if (cal != null) {

					java.util.Calendar targetCalendar = java.util.Calendar
							.getInstance(cal.getTimeZone());
					targetCalendar.clear();
					targetCalendar.setTime(x);
					java.util.Calendar defaultCalendar = java.util.Calendar
							.getInstance();
					defaultCalendar.clear();
					defaultCalendar.setTime(x);
					long timeZoneOffset = targetCalendar
							.get(java.util.Calendar.ZONE_OFFSET)
							- defaultCalendar
									.get(java.util.Calendar.ZONE_OFFSET)
							+ targetCalendar.get(java.util.Calendar.DST_OFFSET)
							- defaultCalendar
									.get(java.util.Calendar.DST_OFFSET);
					adjustDate = ((timeZoneOffset == 0) || (x == null)) ? x
							: new java.sql.Date(x.getTime() + timeZoneOffset);
				} else {
					setDate(parameterIndex, x);
					return;
				}
				switch (dataType) {
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.DATE:
					paramContainer_.setString(parameterIndex, adjustDate
							.toString());
					break;
				case Types.TIMESTAMP:
					t1 = new Timestamp(adjustDate.getTime());
					paramContainer_.setString(parameterIndex, t1.toString());
					break;
				default:
					throw Messages.createSQLException(connection_.locale_,
							"invalid_datatype_for_column", null);
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setDate_ILL].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given Java <tt>double</tt> value.
	 * The driver converts this to a SQL <tt>DOUBLE</tt> value when it sends it
	 * to the database.
	 *
	 * <p>
	 * <B>Note:</B> An extended feature of the Trafodion JDBC driver will allow the
	 * setFloat to write data to any compatible SQL column type. The float input
	 * parameter is converted to the correct SQL column type before the data is
	 * written to the database. If the conversion can not be performed, then the
	 * SQLException "conversion not allowed" will be thrown. In the case were a
	 * double is being used to write to a REAL SQL column type and the value is
	 * too big or too small, the number is converted to + or - infinity. This
	 * action conforms to the way that the IEEE floating point number
	 * specification in the handling of overflow and underflow, and has changed
	 * from previous Trafodion JDBC driver releases.
	 * </p>
	 * <p>
	 * The compatible SQL column types are:
	 *
	 * <ul PLAIN> <tt>TINYINT</tt> </ul> <ul PLAIN> <tt>SMALLINT</tt> </ul> <ul
	 * PLAIN> <tt>INTEGER</tt> </ul> <ul PLAIN> <tt>BIGINT</tt> </ul> <ul PLAIN>
	 * <tt>REAL</tt> </ul> <ul PLAIN> <tt>DOUBLE</tt> </ul> <ul PLAIN>
	 * <tt>DECIMAL</tt> </ul> <ul PLAIN> <tt>NUMERIC</tt> </ul> <ul PLAIN>
	 * <tt>BOOLEAN</tt> </ul> <ul PLAIN> <tt>CHAR</tt> </ul> <ul PLAIN>
	 * <tt>VARCHAR</tt> </ul> <ul PLAIN> <tt>LONGVARCHAR</tt> </ul>
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param x
	 *            the parameter value
	 *
	 * @exception SQLException
	 *                Java data type does not match SQL data type for column
	 */

	public void setDouble(int parameterIndex, double x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setDouble].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setDouble].methodParameters(Integer
					.toString(parameterIndex)
					+ "," + Double.toString(x));
		try {
			dataWrapper.clearColumn(1);
			dataWrapper.setDouble(1, x);
			setDouble(parameterIndex);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setDouble].methodExit();
		}
	}

	private void setDouble(int parameterIndex) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setDouble_I].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setDouble_I].methodParameters("parameterValue = "
					+ Integer.toString(parameterIndex) + ", wrapper value = "
					+ dataWrapper.getDouble(1, null));
		try {
			validateSetInvocation(parameterIndex);
			inputDesc_[parameterIndex - 1]
					.checkValidNumericConversion(connection_.locale_);
			int dataType = inputDesc_[parameterIndex - 1].dataType_;

			switch (dataType) {
			case Types.DOUBLE:
				paramContainer_.setDouble(parameterIndex, dataWrapper
						.getDouble(1, connection_.locale_));
				break;
			case Types.FLOAT: // We should not get from SQL/MX it converts all
				// FLOAT to DOUBLE
				paramContainer_.setFloat(parameterIndex, dataWrapper.getFloat(
						1, connection_.locale_));
				break;
			default: // This is an Trafodion extension JDBC 3.0 API does not allow data
				// type conversion
				setObject(parameterIndex, dataWrapper, dataType);
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setDouble_I].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given Java <tt>float</tt> value. The
	 * driver converts this to a SQL <tt>REAL</tt> value when it sends it to the
	 * database.
	 *
	 * <p>
	 * <B>Note:</B> An extended feature of the Trafodion JDBC driver will allow the
	 * setFloat to write data to any compatible SQL column type. The float input
	 * parameter is converted to the correct SQL column type before the data is
	 * written to the database. If the conversion can not be performed, then the
	 * SQLException "conversion not allowed" will be thrown.In the case were a
	 * double is being used to write to a REAL SQL column type and the value is
	 * too big or too small, the number is converted to + or - infinity. This
	 * action conforms to the way that the IEEE floating point number
	 * specification in the handling of overflow and underflow, and has changed
	 * from previous Trafodion JDBC driver releases.
	 * </p>
	 *
	 * <p>
	 * The compatible SQL column types are:
	 *
	 * <ul PLAIN> <tt>TINYINT</tt> </ul> <ul PLAIN> <tt>SMALLINT</tt> </ul> <ul
	 * PLAIN> <tt>INTEGER</tt> </ul> <ul PLAIN> <tt>BIGINT</tt> </ul> <ul PLAIN>
	 * <tt>REAL</tt> </ul> <ul PLAIN> <tt>DOUBLE</tt> </ul> <ul PLAIN>
	 * <tt>DECIMAL</tt> </ul> <ul PLAIN> <tt>NUMERIC</tt> </ul> <ul PLAIN>
	 * <tt>BOOLEAN</tt> </ul> <ul PLAIN> <tt>CHAR</tt> </ul> <ul PLAIN>
	 * <tt>VARCHAR</tt> </ul> <ul PLAIN> <tt>LONGVARCHAR</tt> </ul>
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param x
	 *            the parameter value
	 *
	 * @exception SQLException
	 *                Java data type does not match SQL data type for column
	 */
	public void setFloat(int parameterIndex, float x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setFloat].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setFloat].methodParameters(Integer
					.toString(parameterIndex)
					+ "," + Float.toString(x));
		try {
			dataWrapper.clearColumn(1);
			dataWrapper.setFloat(1, x);
			setFloat(parameterIndex);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setFloat].methodExit();
		}
	}

	private void setFloat(int parameterIndex) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setFloat_I].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setFloat_I].methodParameters("parameterIndex = "
					+ Integer.toString(parameterIndex) + ", wrapper value = "
					+ dataWrapper.getFloat(1, null));
		try {
			validateSetInvocation(parameterIndex);
			inputDesc_[parameterIndex - 1]
					.checkValidNumericConversion(connection_.locale_);
			int dataType = inputDesc_[parameterIndex - 1].dataType_;

			switch (dataType) {
			case Types.REAL:
				paramContainer_.setFloat(parameterIndex, dataWrapper.getFloat(
						1, connection_.locale_));
				break;
			case Types.DATE:
			case Types.TIME:
			case Types.TIMESTAMP:
				throw Messages.createSQLException(connection_.locale_,
						"invalid_datatype_for_column", null);
			default: // This is an Trafodion extension JDBC 3.0 API does not allow data
				// type conversion
				setObject(parameterIndex, dataWrapper, dataType);
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setFloat_I].methodExit();
		}
	}
	
	public void setFetchSize(int rows) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setFetchSize].methodEntry();
		try {
			clearWarnings();
			if (rows < 0)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_fetchSize_value", null);
			// Fetch size of zero means to take best guess.
			// Since we have no statistics, just leave as is.
			if (rows != 0) {
				fetchSize_ = rows;

				synchronized (connection_) {
					// Pass the fetch size change to the driver
					resetFetchSize(connection_.getDialogueId(), stmtId_, fetchSize_);
				}
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setFetchSize].methodExit();
		}
	}
	
	public int getFetchSize() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getFetchSize].methodEntry();
		try {
			clearWarnings();
			return fetchSize_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getFetchSize].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given Java <tt>int</tt> value. The
	 * driver converts this to a SQL <tt>INTEGER</tt> value when it sends it to
	 * the database.
	 *
	 * <p>
	 * <B>Note:</B> An extended feature of the Trafodion JDBC driver will allow the
	 * setInt to write data to any compatible SQL column type. The int input
	 * parameter is converted to the correct SQL column type before the data is
	 * written to the database. If the conversion can not be performed, then the
	 * SQLException <i>conversion not allowed</i> will be thrown.
	 * </p>
	 * <p>
	 * <B>Note:</B> In order to support the SQL/MX column type
	 * <tt>SMALLINT UNSIGNED</tt> the short data value needs to be written using
	 * the <tt>setInt</tt> method. All of Java's primitive data type are signed.
	 * The only way to write 65535 to a <tt>SMALLINT UNSIGNED</tt> column type
	 * is to use the <tt>setInt</tt> method.
	 * </p>
	 * <B>Note:</B> In order to support the SQL/MX column type
	 * <tt>INTEGER UNSIGNED</tt> the integer data value needs to be written
	 * using the <tt>setLong</tt> method. All of Java's primitive data type are
	 * signed. The only way to write 4294967295 to a <tt>INTEGER UNSIGNED</tt>
	 * column type is to use the <tt>setLong</tt> method. </p>
	 * <p>
	 * The compatible SQL column types are:
	 *
	 * <ul PLAIN> <tt>TINYINT</tt> </ul> <ul PLAIN> <tt>SMALLINT</tt> </ul> <ul
	 * PLAIN> <tt>INTEGER</tt> </ul> <ul PLAIN> <tt>BIGINT</tt> </ul> <ul PLAIN>
	 * <tt>REAL</tt> </ul> <ul PLAIN> <tt>DOUBLE</tt> </ul> <ul PLAIN>
	 * <tt>DECIMAL</tt> </ul> <ul PLAIN> <tt>NUMERIC</tt> </ul> <ul PLAIN>
	 * <tt>BOOLEAN</tt> </ul> <ul PLAIN> <tt>CHAR</tt> </ul> <ul PLAIN>
	 * <tt>VARCHAR</tt> </ul> <ul PLAIN> <tt>LONGVARCHAR</tt> </ul>
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param x
	 *            the parameter value
	 *
	 * @exception SQLException
	 *                if an invalid data type for the column
	 * @exception SQLException
	 *                if the numeric value out of range
	 * @exception SQLExcpetion
	 *                if a negative value is for a unsigned column
	 */
	public void setInt(int parameterIndex, int x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setInt].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setInt].methodParameters(Integer
					.toString(parameterIndex)
					+ "," + Integer.toString(x));
		try {
			dataWrapper.clearColumn(1);
			dataWrapper.setInt(1, x);
			setInt(parameterIndex);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setInt].methodExit();
		}
	}

	private void setInt(int parameterIndex) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setInt_I].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setInt_I].methodParameters("parameterIndex = "
					+ Integer.toString(parameterIndex) + ", wrapper value = "
					+ dataWrapper.getInt(1, null));
		try {
			validateSetInvocation(parameterIndex);
			inputDesc_[parameterIndex - 1]
					.checkValidNumericConversion(connection_.locale_);
			int dataType = inputDesc_[parameterIndex - 1].dataType_;

			int value;
			switch (dataType) {
			case Types.INTEGER:
				checkStringToIntegral();
				// If the data value is negative and
				// the column type is unsigned throw exception
				value = dataWrapper.getInt(1, connection_.locale_);
				if ((value < 0)
						&& (inputDesc_[parameterIndex - 1].isSigned_ == false)) {
					Object[] parmIndex = new Object[2];
					parmIndex[0] = new String(dataWrapper.getString(1));
					parmIndex[1] = new Integer(parameterIndex);
					throw Messages.createSQLException(connection_.locale_,
							"negative_to_unsigned", parmIndex);
				}
				paramContainer_.setInt(parameterIndex, value);
				break;
			case Types.SMALLINT: // The only way to write a unsigned smallint is
				// to
				checkStringToIntegral();
				if (inputDesc_[parameterIndex - 1].isSigned_ == false) // use an
				// integer.
				// Only
				// allow
				// SQL/MX
				// SMALLINT
				// UNSIGNED
				{
					// If the value is greater than, then it will not fit
					// in an unsigned short throw an exception
					value = dataWrapper.getInt(1, connection_.locale_);
					if (value > 65535) {
						Object[] parmIndex = new Object[2];
						parmIndex[0] = new String(dataWrapper.getString(1));
						parmIndex[1] = new Integer(parameterIndex);
						throw Messages.createSQLException(connection_.locale_,
								"numeric_value_out_of_range", parmIndex);
					}
					// If the data value is negative throw exception
					else if (value < 0) {
						Object[] parmIndex = new Object[2];
						parmIndex[0] = new String(dataWrapper.getString(1));
						parmIndex[1] = new Integer(parameterIndex);
						throw Messages.createSQLException(connection_.locale_,
								"negative_to_unsigned", parmIndex);
					}
					paramContainer_.setInt(parameterIndex, value);
					break;
				}
				setObject(parameterIndex, dataWrapper, dataType);
				break;
			case Types.DATE:
			case Types.TIME:
			case Types.TIMESTAMP:
				throw Messages.createSQLException(connection_.locale_,
						"invalid_datatype_for_column", null);
			default:
				setObject(parameterIndex, dataWrapper, dataType);
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setInt_I].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given Java <tt>long</tt> value. The
	 * driver converts this to a SQL <tt>BIGINT</tt> value when it sends it to
	 * the database.
	 *
	 * <p>
	 * <B>Note:</B> An extended feature of the Trafodion JDBC driver will allow the
	 * setLong to write data to any compatible SQL column type. The long input
	 * parameter is converted to the correct SQL column type before the data is
	 * written to the database. If the conversion can not be performed, then the
	 * SQLException <i>conversion not allowed</i> will be thrown. The compatible
	 * SQL column types are:
	 *
	 * <ul PLAIN> <tt>TINYINT</tt> </ul> <ul PLAIN> <tt>SMALLINT</tt> </ul> <ul
	 * PLAIN> <tt>INTEGER</tt> </ul> <ul PLAIN> <tt>BIGINT</tt> </ul> <ul PLAIN>
	 * <tt>REAL</tt> </ul> <ul PLAIN> <tt>DOUBLE</tt> </ul> <ul PLAIN>
	 * <tt>DECIMAL</tt> </ul> <ul PLAIN> <tt>NUMERIC</tt> </ul> <ul PLAIN>
	 * <tt>BOOLEAN</tt> </ul> <ul PLAIN> <tt>CHAR</tt> </ul> <ul PLAIN>
	 * <tt>VARCHAR</tt> </ul> <ul PLAIN> <tt>LONGVARCHAR</tt> </ul>
	 * </p>
	 * <B>Note:</B> In order to support the SQL/MX column type
	 * <tt>INTEGER UNSIGNED</tt> the integer data value needs to be written
	 * using the <tt>setLong</tt> method. All of Java's primitive data type are
	 * signed. The only way to write 4294967295 to a <tt>INTEGER UNSIGNED</tt>
	 * column type is to use the <tt>setLong</tt> method. </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param x
	 *            the parameter value
	 *
	 * @exception SQLException
	 *                if an invalid data type for the column
	 * @exception SQLException
	 *                if the numeric value out of range
	 * @exception SQLExcpetion
	 *                if a negative value is for a unsigned column
	 */
	public void setLong(int parameterIndex, long x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setLong].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setLong].methodParameters(Integer
					.toString(parameterIndex)
					+ "," + Long.toString(x));
		try {
			dataWrapper.clearColumn(1);
			dataWrapper.setLong(1, x);
			setLong(parameterIndex);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setLong].methodExit();
		}
	}

	private void setLong(int parameterIndex) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setLong_I].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setLong_I].methodParameters("parameterIndex = "
					+ Integer.toString(parameterIndex) + ", wrapper value = "
					+ dataWrapper.getLong(1, null));
		try {
			validateSetInvocation(parameterIndex);
			inputDesc_[parameterIndex - 1]
					.checkValidNumericConversion(connection_.locale_);

			int dataType = inputDesc_[parameterIndex - 1].dataType_;

			switch (dataType) {
			case Types.BIGINT:
				checkStringToIntegral();
				paramContainer_.setLong(parameterIndex, dataWrapper.getLong(1,
						connection_.locale_));
				break;
			case Types.INTEGER: // The only way to write a unsigned integer is
				// to
				checkStringToIntegral();
				if (inputDesc_[parameterIndex - 1].isSigned_ == false) // use an
				// long
				{
					long value = dataWrapper.getLong(1, connection_.locale_);
					if (value > 4294967295L) // If the value is greater than,
					// then it will not fit
					{ // in an SQL/MX INTEGER UNSIGNED column type
						Object[] parmIndex = new Object[2];
						parmIndex[0] = new String(dataWrapper.getString(1));
						parmIndex[1] = new Integer(parameterIndex);
						throw Messages.createSQLException(connection_.locale_,
								"numeric_value_out_of_range", parmIndex);
					} else if (value < 0) // Since this column type is unsigned,
					// we can not
					{ // allow a negitive number
						Object[] parmIndex = new Object[2];
						parmIndex[0] = new String(dataWrapper.getString(1));
						parmIndex[1] = new Integer(parameterIndex);
						throw Messages.createSQLException(connection_.locale_,
								"negative_to_unsigned", parmIndex);
					}
					paramContainer_.setLong(parameterIndex, value);
					break;
				}
				setObject(parameterIndex, dataWrapper, dataType); // Send this
				// data
				// value to
				// setObjec,
				// since it
				// is not
				break; // a SQL/MX INTEGER UNSIGNED
			default:
				setObject(parameterIndex, dataWrapper, dataType);
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setLong_I].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to SQL <tt>NULL</tt>.
	 * <p>
	 * <B>Note:</B> You must specify the parameters SQL type (although Trafodion SQL/MX
	 * database does not use the sqlType and it is ignored)
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1, etc...
	 * @param sqlType
	 *            the SQL type code defined in java.sql.Types
	 *
	 * @exception SQLException
	 *                if a bad column index.
	 */
	public void setNull(int parameterIndex, int sqlType) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setNull_II].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setNull_II].methodParameters(Integer
					.toString(parameterIndex)
					+ "," + Integer.toString(sqlType));
		try {
			// *** Note: SQL/MX uses the same procedure to set a column to null
			// and JDBC does not need to know the sqlType
			// *** therefore the sqlType is ignored.

			validateSetInvocation(parameterIndex);
			paramContainer_.setNull(parameterIndex);
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setNull_II].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to SQL <tt>NULL</tt>.
	 * <P>
	 * <B>Note:</B> You must specify the parameters SQL type (although Trafodion SQL/MX
	 * database does not use the sqlType and it is ignored)
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1, the second is 2, ...
	 * @param sqlType
	 *            SQL type code defined by java.sql.Types
	 * @param typeName
	 *            argument parameters for null
	 *
	 * @exception SQLException
	 *                if a bad column index.
	 */
	public void setNull(int paramIndex, int sqlType, String typeName)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setNull_IIL].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setNull_IIL].methodParameters(Integer
					.toString(paramIndex)
					+ "," + Integer.toString(sqlType) + "," + typeName);
		try {
			setNull(paramIndex, sqlType);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setNull_IIL].methodExit();
		}
	}

	/**
	 * Sets the value of the designated parameter using the given object. The
	 * second parameter must be of the type <tt>object</tt>; therefore, the
	 * <tt>java.lang</tt> equivalent object should be used for built-in types.
	 * <p>
	 * The JDBC specification specifies a standard mapping from Java
	 * <tt>object</tt> types to SQL types. The given argument will be converted
	 * to the corresponding SQL type before being sent to the database.
	 * </p>
	 * <p>
	 * <B>Note:</b> This method may be used to pass database-specific abstract
	 * data types, by using the a driver-specific Java type. If the object is of
	 * a class implementing the interface <tt>SQLData</tt>, the JDBC driver
	 * should call the method <tt>SQLData.writeSQL</tt> to write it to the SQL
	 * data stream. If, on the other hand, the object is of a class implementing
	 * <tt>Ref</tt>, <tt>Blob</tt>, <tt>Clob</tt>, <tt>Struct</tt>, or
	 * <tt>Array</tt>, the driver should pass it to the database as a value of
	 * the corresponding SQL type.
	 * </p>
	 * <p>
	 * This method throws an exception if there is an ambiguity, for example, if
	 * the object is of a class implementing more than one of the interfaces
	 * named above.
	 * </p>
	 * <p>
	 * <B>Note:</B> The Trafodion JDBC driver does not support the following and will
	 * throw an exception if called with a object of one of these interfaces:
	 * <ul PLAIN> <tt>Array</tt> </ul> <ul PLAIN> <tt>Ref</tt> </ul> <ul PLAIN>
	 * <tt>Struct</tt> </ul> <ul PLAIN> <tt>SQLData</tt> </ul>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1, the second is 2, ...
	 * @param x
	 *            the object containing the input parameter name
	 *
	 * @throws SQLException
	 *             if one of the unsupported interfaces is use or wrong column
	 *             index.
	 */

	public void setObject(int parameterIndex, Object x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setObject_IL].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setObject_IL].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?");
		try {
			if (x == null)
				setNull(parameterIndex, Types.NULL);
			else if (x instanceof BigDecimal)
				setBigDecimal(parameterIndex, (BigDecimal) x);
			else if (x instanceof java.sql.Date)
				setDate(parameterIndex, (Date) x);
			else if (x instanceof java.sql.Time)
				setTime(parameterIndex, (Time) x);
			else if (x instanceof java.sql.Timestamp)
				setTimestamp(parameterIndex, (Timestamp) x);
			else if (x instanceof Double)
				setDouble(parameterIndex, ((Double) x).doubleValue());
			else if (x instanceof Float)
				setFloat(parameterIndex, ((Float) x).floatValue());
			else if (x instanceof Long)
				setLong(parameterIndex, ((Long) x).longValue());
			else if (x instanceof Integer)
				setInt(parameterIndex, ((Integer) x).intValue());
			else if (x instanceof Short)
				setShort(parameterIndex, ((Short) x).shortValue());
			else if (x instanceof Byte)
				setByte(parameterIndex, ((Byte) x).byteValue());
			else if (x instanceof Boolean)
				setBoolean(parameterIndex, ((Boolean) x).booleanValue());
			else if (x instanceof String)
				setString(parameterIndex, (String) x);
			else if (x instanceof byte[])
				setBytes(parameterIndex, (byte[]) x);
			else if (x instanceof Clob)
				setClob(parameterIndex, (Clob) x);
			else if (x instanceof Blob)
				setBlob(parameterIndex, (Blob) x);
			else if (x == dataWrapper) {
				validateSetInvocation(parameterIndex);
				setObject(parameterIndex, x,
						inputDesc_[parameterIndex - 1].dataType_);
			} else
				throw Messages.createSQLException(connection_.locale_,
						"Object_type_not_supported", null);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setObject_IL].methodExit();
		}
	}

	/**
	 * Sets the value of the designated parameter with the given object. The
	 * second argument must be an object type; for integral values, the
	 * <tt>java.lang</tt> equivalent objects should be used.
	 *
	 * <P>
	 * The given Java object will be converted to the given targetSqlType before
	 * being sent to the database. If the object is of type <tt>Struct</tt>,
	 * <tt>Ref</tt>, <tt>java.net.URL</tt> or <tt>Java class</tt>, then a
	 * <i>Object type not supported</i> SQLException will be thrown.
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param x
	 *            the object containing the input parameter value
	 * @param targetSqlType
	 *            The SQL type (<em>as defined in java.sql.Types</em>) to be
	 *            send to the database
	 *
	 * @throws java.sql.SQLException
	 *             if a conversion error or conversion not allowed
	 */
	public void setObject(int parameterIndex, Object x, int targetSqlType)
			throws SQLException {
		//**********************************************************************
		// *****************
		// * Please note: This method needs to be refactored. There was no time
		// to do this
		// * in V31. With this setObject we should be able to replace the string
		// buffer
		// * and start passing Objects to the C layer. This alread being done
		// for the
		// * floating point values. All of these conversion routine sould be
		// pulled
		// * out and made into a Class. This way the getter methods in the
		// resultSet
		// * interface could also use this conversion.
		// *
		// * NOTE: Do we need to consider the local data, when the input is a
		// string. For example
		// * in some places the ',' is used instead of the decimal point '.'. Do
		// we need to
		// * support "123,00" and "123.00" as the same decimal value?
		//**********************************************************************
		// *****************
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setObject_ILI].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setObject_ILI].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?," + Integer.toString(targetSqlType));
		try {
			if (x == null) {
				setNull(parameterIndex, Types.NULL);
			} else {
				// See if the object type can be a wrapper object
				Object dataObj;
				if (x == dataWrapper)
					dataObj = dataWrapper;
				else {
					dataWrapper.clearColumn(1);
					/*
					 * If the Object x is a Date or Time or Timestamp, it will
					 * be converted to String and passed to the DataWrapper
					 * class
					 */
					if (x instanceof java.sql.Date | x instanceof java.sql.Time
							| x instanceof java.sql.Timestamp)
						dataWrapper.setObject(1, x.toString());
					else
						dataWrapper.setObject(1, x);
					// If it cannot be a wrapper, process the original object
					if (dataWrapper.isNull(1))
						dataObj = x;
					else
						dataObj = dataWrapper;
				}
				switch (targetSqlType) // FLOAT, DOUBLE, SMALLINT, INTEGER,
				// BIGINT and NUMERIC
				{ // are binary and need to be converted all other column
				// types should be done the same as V30
				/*
				 * Please note that the floating-point data types do not need
				 * any kind of limit/range checking. This is due to the IEEE 754
				 * floating-point standard. Any time a value is assigned to a
				 * floating-point data type and it is too large the
				 * floating-point data type is set to a positive infinity. In
				 * the case were the value being assigned to the floating-point
				 * is too small, the floating-point data type is assigned the
				 * value negative infinity.
				 *
				 * To sum every thing up, the IEEE 754 has overflow/underflow
				 * built into the standard, and we should not do anything to
				 * change this behavior.
				 */
				case Types.REAL: // The caller has requested that we convert the
					// input data value
					// to a SQL REAL type
					if (dataObj == dataWrapper) {
						setFloat(parameterIndex);
					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.FLOAT: // Caller has requested that we convert
				case Types.DOUBLE: // the input value to a SQL DOUBLE
					if (dataObj == dataWrapper) {
						setDouble(parameterIndex);
					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.DECIMAL:
				case Types.NUMERIC:
					if (dataObj == dataWrapper) {
						if (dataWrapper.getDataType(1) == DataWrapper.BIG_DECIMAL)
							setBigDecimal(parameterIndex,
									(BigDecimal) dataWrapper.getObject(1));
						else

							// converting the object to string causes
							// negative precision for BigDecimal when it is in exponential format
							// so, commented below line and used getDouble instead
							//	setBigDecimal(parameterIndex, new BigDecimal(
							//			dataWrapper.getString(1)));
							//
							setBigDecimal(parameterIndex, new BigDecimal(
								      dataWrapper.getDouble(1,connection_.locale_)));


					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
					if (dataObj == dataWrapper) {
						// Use dataWrapper
						if (dataWrapper.getDataType(1) == DataWrapper.BOOLEAN){


							boolean b = dataWrapper.getBoolean(1,connection_.locale_);
							String bval;
							if(b)
								bval = "1";
							else
								bval = "0";
							setString(parameterIndex, bval);
						}
//							setString(parameterIndex, Boolean
//									.toString(dataWrapper.getBoolean(1,
//											connection_.locale_)));


						else
							setString(parameterIndex, dataWrapper.getString(1));
					} else if (dataObj instanceof java.sql.Date) {
						setString(parameterIndex, ((java.sql.Date) dataObj)
								.toString());
					} else if (dataObj instanceof java.sql.Time) {
						setString(parameterIndex, ((java.sql.Time) dataObj)
								.toString());
					} else if (dataObj instanceof java.sql.Timestamp) {
						setString(parameterIndex,
								((java.sql.Timestamp) dataObj).toString());
					} else if (dataObj instanceof Clob) {
						setString(parameterIndex, ((Clob) dataObj).toString());
					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.BIGINT:
					if (dataObj == dataWrapper) {
						// Use dataWrapper
						byte dataType = dataWrapper.getDataType(1);
						if (dataType == DataWrapper.DOUBLE) {
							Double tempDouble = new Double(dataWrapper
									.getDouble(1, connection_.locale_)); // The
							// method
							// Double.compareto
							// takes
							// care
							// of
							// NaN
							if ((tempDouble.compareTo(doubleLongMax) > 0) || // Is
									// the
									// double
									// too
									// big
									(tempDouble.compareTo(doubleLongMin) < 0)) // Is
							// the
							// negative
							// too
							// big
							{
								Object[] parmIndex = new Object[2];
								if (Double.isNaN(tempDouble))
									parmIndex[0] = tempDouble.toString();
								else
									parmIndex[0] = tempDouble;
								parmIndex[1] = new Integer(parameterIndex);
								throw Messages
										.createSQLException(
												connection_.locale_,
												"numeric_value_out_of_range",
												parmIndex);
							}
							if ((tempDouble.doubleValue() > 0.0)
									&& (tempDouble.doubleValue() < 1.0)) // Is
							// this
							// a
							// fraction
							// less
							// than
							// 1
							{
								Messages.createSQLWarning(connection_.locale_,
										"data_truncation", null);
							}
						} else if (dataType == DataWrapper.FLOAT) {
							Float tempFloat = new Float(dataWrapper.getFloat(1,
									connection_.locale_)); // The method
							// Float.compareTo
							// takes care of NaN
							if ((tempFloat.compareTo(floatLongMax) > 0) || // Is
									// the
									// float
									// too
									// big
									(tempFloat.compareTo(floatLongMin) < 0)) // Is
							// the
							// negative
							// float
							// too
							// big
							{
								Object[] parmIndex = new Object[2];
								if (Float.isInfinite(tempFloat))
									parmIndex[0] = tempFloat.toString();
								else if (Float.isNaN(tempFloat))
									parmIndex[0] = tempFloat.toString();
								else
									parmIndex[0] = tempFloat;
								parmIndex[1] = new Integer(parameterIndex);
								throw Messages
										.createSQLException(
												connection_.locale_,
												"numeric_value_out_of_range",
												parmIndex);
							}
							if ((tempFloat.floatValue() > 0.0)
									&& (tempFloat.floatValue() < 1.0)) // Is
							// this
							// a
							// fraction
							// less
							// than
							// 1
							{
								Messages.createSQLWarning(connection_.locale_,
										"data_truncation", null);
							}
						}
						setLong(parameterIndex);
					} else if (dataObj instanceof java.sql.Date) {
						dataWrapper.clearColumn(1);
						dataWrapper.setLong(1, ((java.sql.Date) dataObj)
								.getTime());
						setLong(parameterIndex);
					} else if (dataObj instanceof java.sql.Time) {
						dataWrapper.clearColumn(1);
						dataWrapper.setLong(1, ((java.sql.Time) dataObj)
								.getTime());
						setLong(parameterIndex);
					} else if (dataObj instanceof java.sql.Timestamp) {
						dataWrapper.clearColumn(1);
						dataWrapper.setLong(1, ((java.sql.Timestamp) dataObj)
								.getTime());
						setLong(parameterIndex);
					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.INTEGER:
					if (dataObj == dataWrapper) {
						// Use dataWrapper
						byte dataType = dataWrapper.getDataType(1);
						if (dataType == DataWrapper.BOOLEAN) {
							setInt(parameterIndex);
						} else {
							/*
							 * We need to make sure that if the dataObj is a
							 * floating point number that it can be stored in a
							 * long. If it can not be stored in a long then we
							 * need to throw an exception.
							 */
							if (dataType == DataWrapper.DOUBLE) {
								Double tempDouble = new Double(dataWrapper
										.getDouble(1, connection_.locale_)); // The
								// method
								// Double.compareto
								// takes
								// care
								// of
								// NaN
								if ((tempDouble.compareTo(doubleLongMax) > 0) || // Is
										// the
										// double
										// too
										// big
										(tempDouble.compareTo(doubleLongMin) < 0)) // Is
								// the
								// negative
								// too
								// big
								{
									Object[] parmIndex = new Object[2];
									if (Double.isNaN(tempDouble))
										parmIndex[0] = tempDouble.toString();
									else
										parmIndex[0] = tempDouble;
									parmIndex[1] = new Integer(parameterIndex);
									throw Messages.createSQLException(
											connection_.locale_,
											"numeric_value_out_of_range",
											parmIndex);
								}
								if ((tempDouble.doubleValue() > 0.0)
										&& (tempDouble.doubleValue() < 1.0)) // Is
								// this
								// a
								// fraction
								// less
								// than
								// 1
								{
									Messages.createSQLWarning(
											connection_.locale_,
											"data_truncation", null);
								}
							} else if (dataType == DataWrapper.FLOAT) {
								Float tempFloat = new Float(dataWrapper
										.getFloat(1, connection_.locale_)); // The
								// method
								// Float.compareTo
								// takes
								// care
								// of
								// NaN
								if ((tempFloat.compareTo(floatLongMax) > 0) || // Is
										// the
										// float
										// too
										// big
										(tempFloat.compareTo(floatLongMin) < 0)) // Is
								// the
								// negative
								// float
								// too
								// big
								{
									Object[] parmIndex = new Object[2];
									if (Float.isInfinite(tempFloat))
										parmIndex[0] = tempFloat.toString();
									else if (Float.isNaN(tempFloat))
										parmIndex[0] = tempFloat.toString();
									else
										parmIndex[0] = tempFloat;
									parmIndex[1] = new Integer(parameterIndex);
									throw Messages.createSQLException(
											connection_.locale_,
											"numeric_value_out_of_range",
											parmIndex);
								}
								if ((tempFloat.floatValue() > 0.0)
										&& (tempFloat.floatValue() < 1.0)) // Is
								// this
								// a
								// fraction
								// less
								// than
								// 1
								{
									Messages.createSQLWarning(
											connection_.locale_,
											"data_truncation", null);
								}
							}
							/*
							 * Now we know that if we have a floating point
							 * value, that it will fit into a long.
							 */
							if (inputDesc_[parameterIndex - 1].isSigned_ == false) // Need
							// to
							// use
							// setLong
							// if
							// this
							// is
							// a
							// SQL/MX
							{ // column type INTEGER UNSIGNED
								setLong(parameterIndex);
							} else if (dataWrapper.getLong(1,
									connection_.locale_) == dataWrapper.getInt(
									1, connection_.locale_)) // The caller may
							// have given us
							// a value that
							// will
							{ // not fit into an integer, if so throw exception
								setInt(parameterIndex);
							} else {
								Object[] parmIndex = new Object[2];
								parmIndex[0] = dataWrapper.getObject(1);
								parmIndex[1] = new Integer(parameterIndex);
								throw Messages
										.createSQLException(
												connection_.locale_,
												"numeric_value_out_of_range",
												parmIndex);
							}
						}
					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.SMALLINT:
					if (dataObj == dataWrapper) {
						byte dataType = dataWrapper.getDataType(1);
						if (dataType == DataWrapper.BOOLEAN) {
							setShort(parameterIndex);
						} else {
							/*
							 * We need to make sure that if the dataObj is a
							 * floating point number that it can be stored in a
							 * long. If it can not be stored in a long then we
							 * need to throw an exception.
							 */
							if (dataType == DataWrapper.DOUBLE) {
								Double tempDouble = new Double(dataWrapper
										.getDouble(1, connection_.locale_)); // The
								// method
								// Double.compareto
								// takes
								// care
								// of
								// NaN
								if ((tempDouble.compareTo(doubleLongMax) > 0) || // Is
										// the
										// double
										// too
										// big
										(tempDouble.compareTo(doubleLongMin) < 0)) // Is
								// the
								// negative
								// too
								// big
								{
									Object[] parmIndex = new Object[2];
									if (Double.isNaN(tempDouble))
										parmIndex[0] = tempDouble.toString();
									else
										parmIndex[0] = tempDouble;
									parmIndex[1] = new Integer(parameterIndex);
									throw Messages.createSQLException(
											connection_.locale_,
											"numeric_value_out_of_range",
											parmIndex);
								}
								if ((tempDouble.doubleValue() > 0.0)
										&& (tempDouble.doubleValue() < 1.0)) // Is
								// this
								// a
								// fraction
								// less
								// than
								// 1
								{
									Messages.createSQLWarning(
											connection_.locale_,
											"data_truncation", null);
								}
							} else if (dataType == DataWrapper.FLOAT) {
								Float tempFloat = new Float(dataWrapper
										.getFloat(1, connection_.locale_)); // The
								// method
								// Float.compareTo
								// takes
								// care
								// of
								// NaN
								if ((tempFloat.compareTo(floatLongMax) > 0) || // Is
										// the
										// float
										// too
										// big
										(tempFloat.compareTo(floatLongMin) < 0)) // Is
								// the
								// negative
								// float
								// too
								// big
								{
									Object[] parmIndex = new Object[2];
									if (Float.isInfinite(tempFloat))
										parmIndex[0] = tempFloat.toString();
									else if (Float.isNaN(tempFloat))
										parmIndex[0] = tempFloat.toString();
									else
										parmIndex[0] = tempFloat;
									parmIndex[1] = new Integer(parameterIndex);
									throw Messages.createSQLException(
											connection_.locale_,
											"numeric_value_out_of_range",
											parmIndex);
								}
								if ((tempFloat.floatValue() > 0.0)
										&& (tempFloat.floatValue() < 1.0)) // Is
								// this
								// a
								// fraction
								// less
								// than
								// 1
								{
									Messages.createSQLWarning(
											connection_.locale_,
											"data_truncation", null); // Issue a
									// warning
									// just
									// in
									// case
								}
							}
							/*
							 * Now we know that if we have a floating point
							 * value, that it will fit into a long.
							 */

							if (dataWrapper.getLong(1, connection_.locale_) == dataWrapper
									.getInt(1, connection_.locale_)) // Will
							// value
							// fit
							// into
							// an
							// integer,
							// if
							// not
							// so
							// throw
							// exception
							{
								if (inputDesc_[parameterIndex - 1].isSigned_ == false) // Need
								// to
								// use
								// setLong
								// if
								// this
								// is
								// a
								// SQL/MX
								{
									setInt(parameterIndex);
								} else {
									if (dataWrapper.getLong(1,
											connection_.locale_) == dataWrapper
											.getShort(1, connection_.locale_)) // Will
									// value
									// not
									// fit
									// into
									// an
									// short,
									// if
									// not
									// so
									// throw
									// exception
									{
										setShort(parameterIndex);
									} else {
										Object[] parmIndex = new Object[2]; // We
										// had
										// an
										// overflow
										parmIndex[0] = dataWrapper.getObject(1);
										parmIndex[1] = new Integer(
												parameterIndex);
										throw Messages.createSQLException(
												connection_.locale_,
												"numeric_value_out_of_range",
												parmIndex);
									}
								}
							} else {
								Object[] parmIndex = new Object[2]; // We had an
								// overflow
								parmIndex[0] = dataWrapper.getObject(1);
								parmIndex[1] = new Integer(parameterIndex);
								throw Messages
										.createSQLException(
												connection_.locale_,
												"numeric_value_out_of_range",
												parmIndex);
							}
						}
					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.TINYINT: // As of V31 there is no SQL/MX TINYINT
					// but that does not prevent someone from using it
					if (dataObj == dataWrapper) {
						byte dataType = dataWrapper.getDataType(1);
						if (dataType == Types.BOOLEAN) {
							setByte(parameterIndex, dataWrapper.getByte(1,
									connection_.locale_));
						} else {
							/*
							 * We need to make sure that if the dataObj is a
							 * floating point number that it can be stored in a
							 * long. If it can not be stored in a long then we
							 * need to throw an exception.
							 */
							if (dataType == DataWrapper.DOUBLE) {
								Double tempDouble = new Double(dataWrapper
										.getDouble(1, connection_.locale_)); // The
								// method
								// Double.compareto
								// takes
								// care
								// of
								// NaN
								if ((tempDouble.compareTo(doubleLongMax) > 0) || // Is
										// the
										// double
										// too
										// big
										(tempDouble.compareTo(doubleLongMin) < 0)) // Is
								// the
								// negative
								// too
								// big
								{
									Object[] parmIndex = new Object[2];
									if (Double.isNaN(tempDouble))
										parmIndex[0] = tempDouble.toString();
									else
										parmIndex[0] = tempDouble;
									parmIndex[1] = new Integer(parameterIndex);
									throw Messages.createSQLException(
											connection_.locale_,
											"numeric_value_out_of_range",
											parmIndex);
								}
								if ((tempDouble.doubleValue() > 0.0)
										&& (tempDouble.doubleValue() < 1.0)) // Is
								// this
								// a
								// fraction
								// less
								// than
								// 1
								{
									Messages.createSQLWarning(
											connection_.locale_,
											"data_truncation", null);
								}
							} else if (dataType == DataWrapper.FLOAT) {
								Float tempFloat = new Float(dataWrapper
										.getFloat(1, connection_.locale_)); // The
								// method
								// Float.compareTo
								// takes
								// care
								// of
								// NaN
								if ((tempFloat.compareTo(floatLongMax) > 0) || // Is
										// the
										// float
										// too
										// big
										(tempFloat.compareTo(floatLongMin) < 0)) // Is
								// the
								// negative
								// float
								// too
								// big
								{
									Object[] parmIndex = new Object[2];
									if (Float.isInfinite(tempFloat))
										parmIndex[0] = tempFloat.toString();
									else if (Float.isNaN(tempFloat))
										parmIndex[0] = tempFloat.toString();
									else
										parmIndex[0] = tempFloat;
									parmIndex[1] = new Integer(parameterIndex);
									throw Messages.createSQLException(
											connection_.locale_,
											"numeric_value_out_of_range",
											parmIndex);
								}
								if ((tempFloat.floatValue() > 0.0)
										&& (tempFloat.floatValue() < 1.0)) // Is
								// this
								// a
								// fraction
								// less
								// than
								// 1
								{
									Messages.createSQLWarning(
											connection_.locale_,
											"data_truncation", null); // Issue a
									// warning
									// just
									// in
									// case
								}
							}
							/*
							 * Now we know that if we have a floating point
							 * value, that it will fit into a long.
							 */
							byte value = dataWrapper.getByte(1,
									connection_.locale_);
							if (dataWrapper.getLong(1, connection_.locale_) == value) {
								setByte(parameterIndex, value);
							} else {
								Object[] parmIndex = new Object[2];
								parmIndex[0] = dataWrapper.getObject(1);
								parmIndex[1] = new Integer(parameterIndex);
								throw Messages
										.createSQLException(
												connection_.locale_,
												"numeric_value_out_of_range",
												parmIndex);
							}
						}
					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.BOOLEAN:
					if (dataObj == dataWrapper) {
						setBoolean(parameterIndex, dataWrapper.getBoolean(1,
								connection_.locale_));
					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.DATE:
					if ((dataObj == dataWrapper)
							&& (dataWrapper.getDataType(1) == DataWrapper.STRING)) {
						String value;
						if (dataObj == dataWrapper)
							value = dataWrapper.getString(1);
						else
							value = (String) dataObj;
						try {
							setDate(parameterIndex, java.sql.Date
									.valueOf(value)); // API does not indicate
							// an exception
						} // but one is being thrown
						catch (IllegalArgumentException i1) {
							Object[] parmIndex = new Object[2];
							parmIndex[0] = value;
							parmIndex[1] = new Integer(parameterIndex);
							throw Messages.createSQLException(
									connection_.locale_, "date_format_error",
									parmIndex);
						}
					} else if (dataObj instanceof Timestamp) {
						setTimestamp(parameterIndex, new Timestamp(
								((Timestamp) dataObj).getTime()));
					} else if (dataObj instanceof Date) {
						setDate(parameterIndex, (Date) dataObj);
					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.NULL:
					setNull(parameterIndex, Types.NULL);
					break;
				case Types.TIME:
					if ((dataObj == dataWrapper)
							&& (dataWrapper.getDataType(1) == DataWrapper.STRING)) {
						String value;
						if (dataObj == dataWrapper)
							value = dataWrapper.getString(1);
						else
							value = (String) dataObj;
						try {
							setTime(parameterIndex, java.sql.Time
									.valueOf(value)); // API does not indicate
							// an exception
						} // but one is being thrown
						catch (IllegalArgumentException i1) {
							Object[] parmIndex = new Object[2];
							parmIndex[0] = value;
							parmIndex[1] = new Integer(parameterIndex);
							throw Messages.createSQLException(
									connection_.locale_, "time_format_error",
									parmIndex);
						}
					} else if (dataObj instanceof Time) {
						setTime(parameterIndex, (Time) dataObj);
					} else if (dataObj instanceof Timestamp) {
						setTime(parameterIndex, new Time(((Timestamp) dataObj)
								.getTime()));
					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.TIMESTAMP:
					if ((dataObj == dataWrapper)
							&& (dataWrapper.getDataType(1) == DataWrapper.STRING)) {
						String value;
						if (dataObj == dataWrapper)
							value = dataWrapper.getString(1);
						else
							value = (String) dataObj;
						try {
							setTimestamp(parameterIndex, java.sql.Timestamp
									.valueOf(value));
						} catch (IllegalArgumentException i1) {
							Object[] parmIndex = new Object[2];
							parmIndex[0] = value;
							parmIndex[1] = new Integer(parameterIndex);
							throw Messages.createSQLException(
									connection_.locale_,
									"timestamp_format_error", parmIndex);
						}
					} else if (dataObj instanceof Timestamp) {
						setTimestamp(parameterIndex, (Timestamp) dataObj);
					} else if (dataObj instanceof Date) {
						setTimestamp(parameterIndex, new Timestamp(
								((Date) dataObj).getTime()));
					} else if (dataObj instanceof Time) {
						setTimestamp(parameterIndex, new Timestamp(
								((Time) dataObj).getTime()));
					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.CLOB:
					if (dataObj instanceof Clob) {
						setClob(parameterIndex, (Clob) dataObj);
					} else if (dataObj == dataWrapper) {
						if (dataWrapper.getDataType(1) == DataWrapper.CLOB) {
							setClob(parameterIndex, (Clob) dataWrapper
									.getObject(1));
						} else {
							paramContainer_.setLong(parameterIndex, dataWrapper
									.getLong(1, connection_.locale_));
						}
					} else {
						if (JdbcDebugCfg.traceActive) {
							debug[methodId_setObject_ILI].traceOut(
									JdbcDebug.debugLevelError,
									"CLOB Conversion Error for " + dataObj);
							if ((dataObj != null)
									&& (dataObj instanceof DataWrapper)) {
								debug[methodId_setObject_ILI].traceOut(
										JdbcDebug.debugLevelError,
										"DataWrapper Type for Column 1 = "
												+ ((DataWrapper) dataObj)
														.getDataTypeString(1));
							}
						}
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.BLOB:
					if (dataObj instanceof Blob) {
						setBlob(parameterIndex, (Blob) dataObj);
					} else if (dataObj == dataWrapper) {
						if (dataWrapper.getDataType(1) == DataWrapper.BLOB) {
							setBlob(parameterIndex, (Blob) dataWrapper
									.getObject(1));
						} else {
							paramContainer_.setLong(parameterIndex, dataWrapper
									.getLong(1, connection_.locale_));
						}
					} else {
						if (JdbcDebugCfg.traceActive) {
							debug[methodId_setObject_ILI].traceOut(
									JdbcDebug.debugLevelError,
									"BLOB Conversion Error for " + dataObj);
							if ((dataObj != null)
									&& (dataObj instanceof DataWrapper)) {
								debug[methodId_setObject_ILI].traceOut(
										JdbcDebug.debugLevelError,
										"DataWrapper Type for Column 1 = "
												+ ((DataWrapper) dataObj)
														.getDataTypeString(1));
							}
						}
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				case Types.LONGVARBINARY:
				case Types.VARBINARY:
					throw Messages.createSQLException(connection_.locale_,
							"conversion_not_allowed", null);
				case Types.OTHER:
					if ((dataObj == dataWrapper)
							&& (dataWrapper.getDataType(1) == DataWrapper.STRING)) {
						String value;
						if (dataObj == dataWrapper)
							value = dataWrapper.getString(1);
						else
							value = (String) dataObj;
						setString(parameterIndex, value);
						break;
					}
					throw Messages.createSQLException(connection_.locale_,
							"conversion_not_allowed", null);
				case Types.ARRAY:
				case Types.BINARY:
				case Types.BIT:
				case Types.DATALINK:
				case Types.JAVA_OBJECT:
				case Types.REF:
					throw Messages.createSQLException(connection_.locale_,
							"datatype_not_supported", null);
				default:
					throw Messages.createSQLException(connection_.locale_,
							"datatype_not_recognized", null);
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setObject_ILI].methodExit();
		}
	}

	/**
	 * Sets the value of the designated parameter with the given object. The
	 * second argument must be an object type; for integral values, the
	 * <tt>java.lang</tt> equivalent objects should be used.
	 *
	 * <P>
	 * The given Java object will be converted to the given targetSqlType before
	 * being sent to the database. If the object is of type <tt>Struct</tt>,
	 * <tt>Ref</tt>, <tt>java.net.URL</tt> or <tt>Java class</tt>, then a
	 * <i>Object type not supported</i> SQLException will be thrown.
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param parameterObj
	 *            the object containing the input parameter value
	 * @param targetSqlType
	 *            The SQL type (<em>as defined in java.sql.Types</em>) to be
	 *            send to the database
	 * @param scale
	 *            For java.sql.Types.DECIMAL or java.sql.Types.NUMERIC types
	 *            this is the number of digits after the decimal. For all other
	 *            types this value will be ignored.
	 *
	 * @throws SQLException
	 *             if a conversion error or conversion not allowed
	 * @see Types
	 */
	public void setObject(int parameterIndex, Object x, int targetSqlType,
			int scale) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setObject_ILII].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setObject_ILII].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?,"
					+ Integer.toString(targetSqlType)
					+ ","
					+ Integer.toString(scale));
		try {
			if (x == null) {
				setNull(parameterIndex, Types.NULL);
			} else {
				// See if the object type can be a wrapper object
				Object dataObj;
				if (x == dataWrapper)
					dataObj = dataWrapper;
				else {
					dataWrapper.clearColumn(1);
					dataWrapper.setObject(1, x);
					// If it cannot be a wrapper, process the original object
					if (dataWrapper.isNull(1))
						dataObj = x;
					else
						dataObj = dataWrapper;
				}
				switch (targetSqlType) // Only Decimal needs to be address here
				{ // all others will be sent to setObject(index,object,sql)
				case Types.DECIMAL:
				case Types.NUMERIC:
					if (dataObj == dataWrapper) {
						// Use dataWrapper
						BigDecimal bigNum = new BigDecimal(dataWrapper
								.getString(1));
						try {
							bigNum = bigNum.setScale(scale);
						} catch (ArithmeticException a1) {
							Object[] parmIndex = new Object[2];
							parmIndex[0] = bigNum;
							parmIndex[1] = new Integer(parameterIndex);
							Messages.createSQLWarning(connection_.locale_,
									"rounded_half_up", parmIndex);

							bigNum = bigNum.setScale(scale,
									BigDecimal.ROUND_HALF_UP);
						}
						setBigDecimal(parameterIndex, bigNum);
					} else {
						throw Messages.createSQLException(connection_.locale_,
								"conversion_not_allowed", null);
					}
					break;
				default:
					setObject(parameterIndex, dataObj, targetSqlType);
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setObject_ILII].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given REF(
	 * <tt>&ltstructured-type&gt</tt> value. The driver converts this to an SQL
	 * <tt>REF</tt> value when it sends it to the database.
	 * <p>
	 * <B>Note:</B>This method is <em><B>unsupported</B></em> my the Trafodion JDBC
	 * driver. If this method is called a <i>Unsupported feature -
	 * {setArray())</i> SQLException will be thrown.
	 * </p>
	 *
	 * @param i
	 *            the first parameter is 1, the second is 2, ...
	 * @param x
	 *            an object representing data of an SQL REF Type
	 *
	 * @throws SQLException
	 *             unsupported feature
	 */
	public void setRef(int i, Ref x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setRef].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setRef].methodParameters(Integer.toString(i) + ",?");
		try {
			validateSetInvocation(i);
			Messages.throwUnsupportedFeatureException(connection_.locale_,
					"setRef()");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setRef].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given Java <tt>short</tt> value. The
	 * driver converts this to a SQL <tt>SMALLINT</tt> value when it sends it to
	 * the database.
	 * <p>
	 * <B>Note:</B> An extended feature of the Trafodion JDBC driver will allow the
	 * setShort to write data to any compatible SQL column type. The
	 * <tt>short</tt> input parameter is converted to the correct SQL column
	 * type before the data is written to the database. If the conversion can
	 * not be performed, then the SQLException <i>conversion not allowed</i>
	 * will be thrown. The compatible SQL column types are:
	 *
	 * <ul PLAIN> <tt>TINYINT</tt> </ul> <ul PLAIN> <tt>SMALLINT</tt> </ul> <ul
	 * PLAIN> <tt>INTEGER</tt> </ul> <ul PLAIN> <tt>BIGINT</tt> </ul> <ul PLAIN>
	 * <tt>REAL</tt> </ul> <ul PLAIN> <tt>DOUBLE</tt> </ul> <ul PLAIN>
	 * <tt>DECIMAL</tt> </ul> <ul PLAIN> <tt>NUMERIC</tt> </ul> <ul PLAIN>
	 * <tt>BOOLEAN</tt> </ul> <ul PLAIN> <tt>CHAR</tt> </ul> <ul PLAIN>
	 * <tt>VARCHAR</tt> </ul> <ul PLAIN> <tt>LONGVARCHAR</tt> </ul>
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param x
	 *            the parameter value
	 *
	 * @exception java.sql.SQLException
	 *                if a database access error occurs
	 */
	public void setShort(int parameterIndex, short x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setShort].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setShort].methodParameters(Integer
					.toString(parameterIndex)
					+ "," + Short.toString(x));
		try {
			dataWrapper.clearColumn(1);
			dataWrapper.setShort(1, x);
			setShort(parameterIndex);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setShort].methodExit();
		}
	}

	private void setShort(int parameterIndex) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setShort_I].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setShort_I].methodParameters("parameterIndex = "
					+ Integer.toString(parameterIndex) + ", wrapper value = "
					+ dataWrapper.getString(1));
		try {
			validateSetInvocation(parameterIndex);

			inputDesc_[parameterIndex - 1]
					.checkValidNumericConversion(connection_.locale_);
			int dataType = inputDesc_[parameterIndex - 1].dataType_;

			switch (dataType) {
			case Types.SMALLINT:
				checkStringToIntegral();
				short value = dataWrapper.getShort(1, connection_.locale_);
				if ((value < 0)
						&& (inputDesc_[parameterIndex - 1].isSigned_ == false)) {
					Object[] parmIndex = new Object[2];
					parmIndex[0] = new String(dataWrapper.getString(1));
					parmIndex[1] = new Integer(parameterIndex);
					throw Messages.createSQLException(connection_.locale_,
							"negative_to_unsigned", parmIndex);
				}
				paramContainer_.setShort(parameterIndex, value);
				break;
			case Types.DATE:
			case Types.TIME:
			case Types.TIMESTAMP:
				throw Messages.createSQLException(connection_.locale_,
						"invalid_datatype_for_column", null);
			default:
				setObject(parameterIndex, dataWrapper, dataType);
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setShort_I].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the Java <tt>String</tt> value. The
	 * driver converts this to a SQL <tt>VARCHAR</tt> or <tt>LONGVARCHAR</tt>
	 * value (depending on the arguments size relative to the driver's limits on
	 * <tt>VARCHAR</tt> values) when it sends it to the database.
	 * <p>
	 * <B>Note:</B> An extended feature of the Trafodion JDBC driver will allow the
	 * setString to write data to any compatible SQL column type. The
	 * <tt>String</tt> input parameter is converted to the correct SQL column
	 * type before the data is written to the database. If the conversion can
	 * not be performed, then the SQLException <i>conversion not allowed</i>
	 * will be thrown. The compatible SQL column types are:
	 *
	 * <ul PLAIN> <tt>TINYINT</tt> </ul> <ul PLAIN> <tt>SMALLINT</tt> </ul> <ul
	 * PLAIN> <tt>INTEGER</tt> </ul> <ul PLAIN> <tt>BIGINT</tt> </ul> <ul PLAIN>
	 * <tt>REAL</tt> </ul> <ul PLAIN> <tt>DOUBLE</tt> </ul> <ul PLAIN>
	 * <tt>DECIMAL</tt> </ul> <ul PLAIN> <tt>NUMERIC</tt> </ul> <ul PLAIN>
	 * <tt>BOOLEAN</tt> </ul> <ul PLAIN> <tt>CHAR</tt> </ul> <ul PLAIN>
	 * <tt>VARCHAR</tt> </ul> <ul PLAIN> <tt>LONGVARCHAR</tt> </ul> <ul PLAIN>
	 * <tt>DATE</tt> </ul> <ul PLAIN> <tt>TIME</tt> </ul> <ul PLAIN>
	 * <tt>TIMESTAMP</tt> </ul> <ul PLAIN> <tt>CLOB</tt></u>
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...
	 * @param x
	 *            the parameter value
	 *
	 * @throws SQLException
	 *             if a database access error occurs
	 */
	public void setString(int parameterIndex, String x) throws SQLException {
		/*
		 * Please note, do not attempt to optimize any of the time, timestamp,
		 * and date data types. SQLJ has a test were they enter the value
		 * "1988-10-25:10:10:10.101010" using either a setString(n,String) or
		 * setObject(n,String). This timestamp format is shown in the SQL/MX
		 * Reference Manual as not a valid timestamp format, and the Java API
		 * indicates that it is not a valid format, however; mxci will accept
		 * the format and so does the current version of this driver. Java will
		 * not accept it. Unless you are willing to rewrite the Timestamp class,
		 * I would suggest that no one touch the time, timestamp and date data
		 * types.
		 */
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setString].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setString].methodParameters(Integer
					.toString(parameterIndex)
					+ "," + x);
		try {
			validateSetInvocation(parameterIndex);
			int dataType = inputDesc_[parameterIndex - 1].dataType_;

			if (x == null && dataType != Types.CLOB && dataType != Types.BLOB) {
				setNull(parameterIndex, Types.NULL);
			} else {
				switch (dataType) {
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
					/*
					 * Description: Date,Time and Timestamp
					 * now adheres to Java Standard for Date Time and Timestamp
					 * type.
					 */
					paramContainer_.setString(parameterIndex, x);
					break;

				case Types.DATE: // Need by SQLJ to pass QA tests
					x = (Date.valueOf(x.trim())).toString();
					paramContainer_.setString(parameterIndex, x);
					break;
				case Types.TIME: // Need by SQLJ to pass QA tests
					x = (Time.valueOf(x.trim())).toString();
					paramContainer_.setString(parameterIndex, x);
					break;
				case Types.TIMESTAMP: // Need by SQLJ to pass QA tests
					x = (Timestamp.valueOf(x.trim())).toString();
					if (x.length() > 26) {
						x = x.substring(0, 26);
					}


				case Types.OTHER: // This type maps to the SQL/MX INTERVAL
					paramContainer_.setString(parameterIndex, x);
					break;
				case Types.CLOB: // WLS extension: CLOB should to be able to
					SQLMXClob clob = null;
					if (x == null) 
						paramContainer_.setNull(parameterIndex);
					else {
						if (x.length() <= connection_.getInlineLobChunkSize()) {
							paramContainer_.setString(parameterIndex, x);
						} else {
							isAnyLob_ = true;
							clob = new SQLMXClob(connection_, null, x);
							inputDesc_[parameterIndex - 1].paramValue_ = "";
							paramContainer_.setString(parameterIndex, "");
						}
					}
					addLobObjects(parameterIndex, clob);
					break;
				case Types.BLOB:
					SQLMXBlob blob = null;
					if (x == null) 
						paramContainer_.setNull(parameterIndex);
					else {
						if (x.length() <= connection_.getInlineLobChunkSize()) {
							paramContainer_.setBytes(parameterIndex, x.getBytes());
						} else {
							isAnyLob_ = true;
							blob = new SQLMXBlob(connection_, null, x.getBytes());
							inputDesc_[parameterIndex - 1].paramValue_ = "";
							paramContainer_.setBytes(parameterIndex, new byte[0]);
						}
					}
					addLobObjects(parameterIndex, blob);
					break;
				case Types.ARRAY:
				case Types.BINARY:
				case Types.BIT:
				case Types.DATALINK:
				case Types.JAVA_OBJECT:
				case Types.REF:
					throw Messages.createSQLException(connection_.locale_,
							"datatype_not_supported", null);
				case Types.BIGINT:
				case Types.INTEGER:
				case Types.SMALLINT:
				case Types.TINYINT:
				case Types.DECIMAL:
				case Types.NUMERIC:
					// SQLJ is using numeric string with leading/trailing
					// whitespace
					dataWrapper.clearColumn(1);
					dataWrapper.setString(1, x.trim());
					setObject(parameterIndex, dataWrapper, dataType);
					break;
				case Types.BOOLEAN:
				case Types.DOUBLE:
				case Types.FLOAT:
				case Types.LONGVARBINARY:
				case Types.NULL:
				case Types.REAL:
				case Types.VARBINARY:
					dataWrapper.clearColumn(1);
					dataWrapper.setString(1, x);
					setObject(parameterIndex, dataWrapper, dataType);
					break;
				default:
					throw Messages.createSQLException(connection_.locale_,
							"datatype_not_recognized", null);
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setString].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given <tt>java.sql.Time</tt> value.
	 * The driver converts this to a SQL <tt>TIME</tt> value when it sends it to
	 * the database.
	 * <p>
	 * <B>Note:</B> An extended feature of the Trafodion JDBC driver will allow the
	 * setTime to write data to any compatible SQL column type. The
	 * <tt>Time</tt> input parameter is converted to the correct SQL column type
	 * before the data is written to the database. If the conversion can not be
	 * performed, then the SQLException <i>restricted data type</i> will be
	 * thrown. The compatible SQL column types are:
	 *
	 * <ul PLAIN> <tt>CHAR</tt> </ul> <ul PLAIN> <tt>VARCHAR</tt> </ul> <ul
	 * PLAIN> <tt>LONGVARCHAR</tt> </ul> <ul PLAIN> <tt>TIME</tt> </ul> <ul
	 * PLAIN> <tt>TIMESTAMP</tt> </ul>
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...));
	 * @param x
	 *            the parameter value
	 *
	 * @throws SQLException
	 *             Java data type does not match SQL data type for column
	 */
	public void setTime(int parameterIndex, Time x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setTime_IL].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setTime_IL].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?");
		try {
			int dataType;
			Timestamp t1;

			validateSetInvocation(parameterIndex);
			dataType = inputDesc_[parameterIndex - 1].dataType_;

			if (x == null) {
				paramContainer_.setNull(parameterIndex);
			} else {
				switch (dataType) {
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.TIME:
					paramContainer_.setString(parameterIndex, x.toString());
					break;
				case Types.TIMESTAMP:
					t1 = new Timestamp(x.getTime());
					paramContainer_.setString(parameterIndex, t1.toString());
					break;
				default:
					throw Messages.createSQLException(connection_.locale_,
							"invalid_datatype_for_column", null);
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setTime_IL].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given <tt>java.sql.Time</tt> value,
	 * using the given <tt>Calendar</tt> object. The driver uses the
	 * <tt>Calendar</tt> object to construct an SQL <tt>TIME</tt> value, which
	 * the driver then sends it to the database. With a <tt>Calendar</tt>
	 * object, the driver can calculate the time taking into account a custom
	 * timezone. If no <tt>Calendar</tt> object is specified, the driver uses
	 * the default timezone, which is that of the virtual machine running the
	 * application.
	 * <p>
	 * <B>Note:</B> The Trafodion JDBC driver uses the <tt>Calendar</tt> parameter. An
	 * extended feature of the Trafodion JDBC driver will allow the setTime to write
	 * data to any compatible SQL column type. The <tt>Time</tt> input parameter
	 * is converted to the correct SQL column type before the data is written to
	 * the database. If the conversion can not be performed, then the
	 * SQLException <i>restricted data type</i> will be thrown. The compatible
	 * SQL column types are:
	 *
	 * <ul PLAIN> <tt>CHAR</tt> </ul> <ul PLAIN> <tt>VARCHAR</tt> </ul> <ul
	 * PLAIN> <tt>LONGVARCHAR</tt> </ul> <ul PLAIN> <tt>TIME</tt> </ul> <ul
	 * PLAIN> <tt>TIMESTAMP</tt> </ul>
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...));
	 * @param x
	 *            the parameter value
	 * @param cal
	 *            the <tt>Calendar</tt> object the driver will use to construct
	 *            the time
	 *
	 * @throws SQLException
	 *             Java data type does not match SQL data type for column
	 */
	public void setTime(int parameterIndex, Time x, Calendar cal)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setTime_ILL].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setTime_ILL].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?,?");
		try {
			int dataType;
			Timestamp t1;

			validateSetInvocation(parameterIndex);
			dataType = inputDesc_[parameterIndex - 1].dataType_;

			if (x == null) {
				paramContainer_.setNull(parameterIndex);
			} else {
				Time adjustTime = null;
				if (cal != null) {
					java.util.Calendar targetCalendar = java.util.Calendar
							.getInstance(cal.getTimeZone());
					targetCalendar.clear();
					targetCalendar.setTime(x);
					java.util.Calendar defaultCalendar = java.util.Calendar
							.getInstance();
					defaultCalendar.clear();
					defaultCalendar.setTime(x);
					long timeZoneOffset = targetCalendar
							.get(java.util.Calendar.ZONE_OFFSET)
							- defaultCalendar
									.get(java.util.Calendar.ZONE_OFFSET)
							+ targetCalendar.get(java.util.Calendar.DST_OFFSET)
							- defaultCalendar
									.get(java.util.Calendar.DST_OFFSET);
					adjustTime = ((timeZoneOffset == 0) || (x == null)) ? x
							: new java.sql.Time(x.getTime() + timeZoneOffset);
				} else {
					setTime(parameterIndex, x);
					return;
				}

				switch (dataType) {
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.TIME:
					paramContainer_.setString(parameterIndex, adjustTime
							.toString());
					break;
				case Types.TIMESTAMP:
					t1 = new Timestamp(adjustTime.getTime());
					paramContainer_.setString(parameterIndex, t1.toString());
					break;
				default:
					throw Messages.createSQLException(connection_.locale_,
							"invalid_datatype_for_column", null);
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setTime_ILL].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given <tt>java.sql.Timestamp</tt>
	 * value. The driver converts this to a SQL <tt>TIMESTAMP</tt> value when it
	 * sends it to the database.
	 * <p>
	 * <B>Note:</B> An extended feature of the Trafodion JDBC driver will allow the
	 * setTimestamp to write data to any compatible SQL column type. The
	 * <tt>Timestamp</tt> input parameter is converted to the correct SQL column
	 * type before the data is written to the database. If the conversion can
	 * not be performed, then the SQLException <i>restricted data type</i> will
	 * be thrown. The compatible SQL column types are:
	 *
	 * <ul PLAIN> <tt>CHAR</tt> </ul> <ul PLAIN> <tt>VARCHAR</tt> </ul> <ul
	 * PLAIN> <tt>LONGVARCHAR</tt> </ul> <ul PLAIN> <tt>TIME</tt> </ul> <ul
	 * PLAIN> <tt>TIMESTAMP</tt> </ul> <ul PLAIN> <tt>DATE</tt> </ul>
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...));
	 * @param x
	 *            the parameter value
	 *
	 * @throws SQLException
	 *             Java data type does not match SQL data type for column
	 */
	public void setTimestamp(int parameterIndex, Timestamp x)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setTimestamp_IL].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setTimestamp_IL].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?");
		try {
			int dataType;
			Date d1;
			Time t1;

			validateSetInvocation(parameterIndex);
			dataType = inputDesc_[parameterIndex - 1].dataType_;

			if (x == null) {
				paramContainer_.setNull(parameterIndex);
			} else {
				switch (dataType) {
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.TIMESTAMP:
					/*
					 * paramContainer_.setString(parameterIndex, x.toString());
					 */
					String temp = x.toString();
					if (temp.length() > 26)
						temp = temp.substring(0, 26);
					paramContainer_.setString(parameterIndex, temp);
					break;
				case Types.TIME:
					t1 = new Time(x.getTime());
					paramContainer_.setString(parameterIndex, t1.toString());
					break;
				case Types.DATE:
					d1 = new Date(x.getTime());
					paramContainer_.setString(parameterIndex, d1.toString());
					break;
				default:
					throw Messages.createSQLException(connection_.locale_,
							"invalid_datatype_for_column", null);
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setTimestamp_IL].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given <tt>java.sql.Timestamp</tt>
	 * value, using the given <tt>Calendar</tt> object. The driver uses the
	 * <tt>Calendar</tt> object to construct an SQL <tt>TIMESTAMP</tt> value,
	 * which the driver then sends it to the database. With a <tt>Calendar</tt>
	 * object, the driver can calculate the timestamp taking into account a
	 * custom timezone. If no <tt>Calendar</tt> object is specified, the driver
	 * uses the default timezone, which is that of the virtual machine running
	 * the application.
	 * <p>
	 * <B>Note:</B> The Trafodion JDBC driver uses the <tt>Calendar</tt> parameter. An
	 * extended feature of the Trafodion JDBC driver will allow the setTime to write
	 * data to any compatible SQL column type. The <tt>Timestamp</tt> input
	 * parameter is converted to the correct SQL column type before the data is
	 * written to the database. If the conversion can not be performed, then the
	 * SQLException <i>restricted data type</i> will be thrown. The compatible
	 * SQL column types are:
	 *
	 * <ul PLAIN> <tt>CHAR</tt> </ul> <ul PLAIN> <tt>VARCHAR</tt> </ul> <ul
	 * PLAIN> <tt>LONGVARCHAR</tt> </ul> <ul PLAIN> <tt>TIME</tt> </ul> <ul
	 * PLAIN> <tt>TIMESTAMP</tt> </ul> <ul PLAIN> <tt>DATE</tt> </ul>
	 * </p>
	 *
	 * @param parameterIndex
	 *            the first parameter is 1...));
	 * @param x
	 *            the parameter value
	 * @param cal
	 *            the <tt>Calendar</tt> object the driver will use to construct
	 *            the time
	 *
	 * @throws SQLException
	 *             Java data type does not match SQL data type for column
	 */
	public void setTimestamp(int parameterIndex, Timestamp x, Calendar cal)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setTimestamp_ILL].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setTimestamp_ILL].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?,?");
		try {
			int dataType;
			Date d1;
			Time t1;

			validateSetInvocation(parameterIndex);
			dataType = inputDesc_[parameterIndex - 1].dataType_;

			if (x == null) {
				paramContainer_.setNull(parameterIndex);
			} else {
				Timestamp adjustedTimestamp = null;
				if (cal != null) {
					java.util.Calendar targetCalendar = java.util.Calendar
							.getInstance(cal.getTimeZone());
					targetCalendar.clear();
					targetCalendar.setTime(x);
					java.util.Calendar defaultCalendar = java.util.Calendar
							.getInstance();
					defaultCalendar.clear();
					defaultCalendar.setTime(x);
					long timeZoneOffset = targetCalendar
							.get(java.util.Calendar.ZONE_OFFSET)
							- defaultCalendar
									.get(java.util.Calendar.ZONE_OFFSET)
							+ targetCalendar.get(java.util.Calendar.DST_OFFSET)
							- defaultCalendar
									.get(java.util.Calendar.DST_OFFSET);
					adjustedTimestamp = ((timeZoneOffset == 0) || (x == null)) ? x
							: new java.sql.Timestamp(x.getTime()
									+ timeZoneOffset);
					if (x != null) {
						adjustedTimestamp.setNanos(x.getNanos());
					}
				} else {

					setTimestamp(parameterIndex, x);
					return;
				}

				switch (dataType) {
				case Types.CHAR:
				case Types.VARCHAR:
				case Types.LONGVARCHAR:
				case Types.TIMESTAMP:
					// java.sql.Timestamp timsp = new
					// java.sql.Timestamp(cal.getTime().getTime());
					// paramContainer_.setString(parameterIndex,
					// timsp.toString());
					paramContainer_.setString(parameterIndex, adjustedTimestamp
							.toString());
					break;
				case Types.TIME:
					t1 = new Time(cal.getTime().getTime());
					paramContainer_.setString(parameterIndex, t1.toString());
					break;
				case Types.DATE:
					d1 = new Date(cal.getTime().getTime());
					paramContainer_.setString(parameterIndex, d1.toString());
					break;
				default:
					throw Messages.createSQLException(connection_.locale_,
							"invalid_datatype_for_column", null);
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setTimestamp_ILL].methodExit();
		}
	}

	/**
	 * @deprecated
	 */
	@Deprecated
	public void setUnicodeStream(int parameterIndex, InputStream x, int length)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setUnicodeStream].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setUnicodeStream].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?," + Integer.toString(length));
		try {
			int dataType;
			byte[] buffer = new byte[length];
			String s;

			validateSetInvocation(parameterIndex);
			dataType = inputDesc_[parameterIndex - 1].dataType_;

			if (x == null) {
				paramContainer_.setNull(parameterIndex);
			} else {
				switch (dataType) {
				case Types.DECIMAL:
				case Types.DOUBLE:
				case Types.FLOAT:
				case Types.NUMERIC:
				case Types.SMALLINT:
				case Types.INTEGER:
				case Types.BIGINT:
				case Types.TINYINT:
					throw Messages.createSQLException(connection_.locale_,
							"invalid_datatype_for_column", null);
				default:
					try {
						x.read(buffer, 0, length);
					} catch (java.io.IOException e) {
						Object[] messageArguments = new Object[1];
						messageArguments[0] = e.getMessage();
						throw Messages.createSQLException(connection_.locale_,
								"io_exception", messageArguments);
					}
					try {
						s = new String(buffer, "UnicodeBig");
						paramContainer_.setString(parameterIndex, s);
					} catch (java.io.UnsupportedEncodingException e) {
						Object[] messageArguments = new Object[1];
						messageArguments[0] = e.getMessage();
						throw Messages.createSQLException(connection_.locale_,
								"unsupported_encoding", messageArguments);
					}
					break;
				}
			}
			inputDesc_[parameterIndex - 1].isValueSet_ = true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setUnicodeStream].methodExit();
		}
	}

	/**
	 * Sets the designated parameter to the given <tt>java.net.URL</tt> value.
	 * The driver converts this to an SQL <tt>DATALINK</tt> value when it sends
	 * it to the database.
	 * <p>
	 * <B>Note:</B>This method is <em><B>unsupported</B></em> by the Trafodion JDBC
	 * driver. If this method is called a <i>Unsupported feature -
	 * {setURL())</i> SQLException will be thrown.
	 * </p>
	 *
	 * @param i
	 *            the first parameter is 1, the second is 2, ...
	 * @param x
	 *            the <tt>java.net.URL</tt> object to be set
	 *
	 * @throws SQLException
	 *             Unsupported feature.
	 */
	public void setURL(int parameterIndex, URL x) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setURL].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setURL].methodParameters(Integer
					.toString(parameterIndex)
					+ ",?");
		try {
			validateSetInvocation(parameterIndex);
			Messages.throwUnsupportedFeatureException(connection_.locale_,
					"setURL()");
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setURL].methodExit();
		}
	}

	// Other methods
	protected void validateExecuteInvocation() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_validateExecuteInvocation].methodEntry();
		try {
			clearWarnings();
			if (isClosed_)
				throw Messages.createSQLException(connection_.locale_,
						"stmt_closed", null);
			if (connection_.isClosed_)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_connection", null);
			// close the previous resultset, if any
			if (resultSet_ != null)
				resultSet_.close();
			if (paramRowCount_ > 0)
				throw Messages.createSQLException(connection_.locale_,
						"function_sequence_error", null);
			checkIfAllParamsSet();
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_validateExecuteInvocation].methodExit();
		}
	}

	private void checkIfAllParamsSet() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_checkIfAllParamsSet].methodEntry();
		try {
			int paramNumber;

			if (inputDesc_ == null)
				return;
			for (paramNumber = 0; paramNumber < inputDesc_.length; paramNumber++) {
				if (!inputDesc_[paramNumber].isValueSet_) {
					Object[] messageArguments = new Object[2];
					messageArguments[0] = new Integer(paramNumber + 1);
					messageArguments[1] = new Integer(paramRowCount_ + 1);
					throw Messages.createSQLException(connection_.locale_,
							"parameter_not_set", messageArguments);
				}
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_checkIfAllParamsSet].methodExit();
		}
	}

	private void validateSetInvocation(int parameterIndex) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_validateSetInvocation].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_validateSetInvocation].methodParameters(Integer
					.toString(parameterIndex));
		try {
			if (isClosed_)
				throw Messages.createSQLException(connection_.locale_,
						"stmt_closed", null);
			if (connection_.isClosed_)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_connection", null);
			if (inputDesc_ == null)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_parameter_index", null);
			if (parameterIndex < 1 || parameterIndex > inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_parameter_index", null);
			if (inputDesc_[parameterIndex - 1].paramMode_ == DatabaseMetaData.procedureColumnOut)
				throw Messages.createSQLException(connection_.locale_,
						"is_a_output_parameter", null);
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_validateSetInvocation].methodExit();
		}
	}

	Object getParameters() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getParameters].methodEntry();
		try {
			if (paramRowCount_ == 0)
				return (paramContainer_);
			if (paramRowCount_ == 1)
				return (rowsValue_.get(0));

			DataWrapper[] valueArray = new DataWrapper[paramRowCount_];
			int length = rowsValue_.size();
			for (int i = 0; i < length; i++) {
				valueArray[i] = (DataWrapper) rowsValue_.get(i);
			}
			return valueArray;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getParameters].methodExit();
		}
	}

	 void copyParameters(SQLMXPreparedStatement other) 
	{
		paramRowCount_ = other.paramRowCount_;
		paramContainer_ = other.paramContainer_;
		rowsValue_ = other.rowsValue_;
		for (int paramNumber = 0; paramNumber < inputDesc_.length; paramNumber++)
			inputDesc_[paramNumber].isValueSet_ = true;
	}

	void logicalClose() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_logicalClose].methodEntry();
		try {
			if(!isClosed_){
			isClosed_ = true;
			if (rowsValue_ != null)
				rowsValue_.clear();
			if (lobObjects_ != null)
				lobObjects_.clear();
			paramRowCount_ = 0;
			if (resultSet_ != null)
				resultSet_.close();
			// Clear the isValueSet_ flag in inputDesc_
			if (inputDesc_ != null) {
				for (int i = 0; i < inputDesc_.length; i++) {
					inputDesc_[i].isValueSet_ = false;
					paramContainer_.setNull(i + 1);
					inputDesc_[i].paramValue_ = null;
				}
			}
			isAnyLob_ = false;
			/*
			 * Description : Identify and close the duplicate statement.
			 * Additional argument 'this' passed.
			 */

			connection_.lClosestmtCount++;

			if (connection_.out_ != null) {
				if (connection_.traceFlag_ >= T2Driver.POOLING_LVL){
					connection_.out_.println(getTraceId() + "close() "
							+ "Logically closed statement - " + "\""
							+ this.stmtId_ + "\"");
				connection_.out_.println(getTraceId() + "close() "
						+"\""+"LOGICALCLOSED STMTS COUNT:"+connection_.lClosestmtCount + "\"");
				}
			}
			connection_.closePreparedStatement(this, connection_, sql_.trim(),
					resultSetType_, resultSetConcurrency_,
					resultSetHoldability_);
		}
		}finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_logicalClose].methodExit();
		}
	}

	// Method used by JNI Layer to update the results of Prepare
	// Venu chabged stmtId from int to long for 64 bit
	void setPrepareOutputs(SQLMXDesc[] inputDesc, SQLMXDesc[] outputDesc,
			int txid, int inputParamCount, int outputParamCount, long stmtId)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setPrepareOutputs].methodEntry();
		try {
			inputDesc_ = inputDesc;
			outputDesc_ = outputDesc;
			paramRowCount_ = 0;

			// Prepare updates inputDesc_ and outputDesc_
			if (inputDesc_ != null)
				paramContainer_ = new DataWrapper(inputDesc_.length);
			else
				paramContainer_ = null;
			connection_.setTxid_(txid);
			stmtId_ = stmtId;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setPrepareOutputs].methodExit();
		}
	}

	// Method used by JNI layer to update the results of Execute
	void setExecuteOutputs(int[] rowCount, int totalRowCount,
			DataWrapper[] fetchedRows, int fetchedRowCount, int txid)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setExecuteOutputs].methodEntry();
		try {
			batchRowCount_ = rowCount;

			if (outputDesc_ != null) {
				if (resultSet_ == null) {
					// result set should not be null at this point
					Object[] messageArguments = new Object[1];
					messageArguments[0] = "ResultSet not allocated while processing execute call";
					throw Messages.createSQLException(connection_.locale_,
							"programming_error", messageArguments);
				}

				// Check if the transaction is started by this Select statement
				if (connection_.getTxid() == 0 && txid != 0)
					resultSet_.txnStarted_ = true;
				rowCount_ = -1;
			} else {
				resultSet_ = null;
				rowCount_ = totalRowCount;
			}

			connection_.setTxid_(txid);

			// Populate the result set if row information was returned on the
			// execute
			if ((fetchedRows != null) && (resultSet_ != null)) {
				resultSet_.setFetchOutputs(fetchedRows, fetchedRowCount, true,
						txid);
			}
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setExecuteOutputs].methodExit();
		}
	}

	int getOutFSDataType(int paramCount) throws SQLException // 0 -based
	{
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getOutFSDataType].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_getOutFSDataType].methodParameters(Integer
					.toString(paramCount));
		try {
			if (paramCount >= inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_parameter_index", null);
			return inputDesc_[paramCount].fsDataType_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getOutFSDataType].methodExit();
		}
	}

	int getOutScale(int paramCount) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getOutScale].methodEntry();
		try {
			if (paramCount >= inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_parameter_index", null);
			return inputDesc_[paramCount].scale_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getOutScale].methodExit();
		}
	}

	int getSQLDataType(int paramCount) throws SQLException // 0 -based
	{
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSQLDataType].methodEntry();
		try {
			if (paramCount >= inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_parameter_index", null);
			return inputDesc_[paramCount].sqlDataType_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSQLDataType].methodExit();
		}
	}

	int getSQLOctetLength(int paramCount) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSQLOctetLength].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_getSQLOctetLength].methodParameters(Integer
					.toString(paramCount));
		try {
			if (paramCount >= inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_parameter_index", null);
			return inputDesc_[paramCount].sqlOctetLength_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSQLOctetLength].methodExit();
		}
	}

	int getPrecision(int paramCount) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getPrecision].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_getPrecision].methodParameters(Integer
					.toString(paramCount));
		try {
			if (paramCount >= inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_parameter_index", null);
			return inputDesc_[paramCount].precision_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getPrecision].methodExit();
		}
	}

	int getScale(int paramCount) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getScale].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_getScale].methodParameters(Integer
					.toString(paramCount));
		try {
			if (paramCount >= inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_parameter_index", null);
			return inputDesc_[paramCount].scale_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getScale].methodExit();
		}

	}

	int getSqlDatetimeCode(int paramCount) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getSqlDatetimeCode].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_getSqlDatetimeCode].methodParameters(Integer
					.toString(paramCount));
		try {
			if (paramCount >= inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_parameter_index", null);
			return inputDesc_[paramCount].sqlDatetimeCode_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getSqlDatetimeCode].methodExit();
		}
	}

	int getFSDataType(int paramCount) throws SQLException // 0 -based
	{
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getFSDataType].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_getFSDataType].methodParameters(Integer
					.toString(paramCount));
		try {
			if (paramCount >= inputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_parameter_index", null);
			return inputDesc_[paramCount].fsDataType_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getFSDataType].methodExit();
		}
	}

	void reuse(SQLMXConnection connection, int resultSetType,
			int resultSetConcurrency, int resultSetHoldability)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_reuse].methodEntry();
		try {
			if (resultSetType != ResultSet.TYPE_FORWARD_ONLY
					&& resultSetType != ResultSet.TYPE_SCROLL_INSENSITIVE
					&& resultSetType != ResultSet.TYPE_SCROLL_SENSITIVE)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_resultset_type", null);
			if (resultSetType == ResultSet.TYPE_SCROLL_SENSITIVE) {
				resultSetType_ = ResultSet.TYPE_SCROLL_INSENSITIVE;
				setSQLWarning(null, "scrollResultSetChanged", null);
			} else
				resultSetType_ = resultSetType;
			if (resultSetConcurrency != ResultSet.CONCUR_READ_ONLY
					&& resultSetConcurrency != ResultSet.CONCUR_UPDATABLE)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_resultset_concurrency", null);
			resultSetConcurrency_ = resultSetConcurrency;
			resultSetHoldability_ = resultSetHoldability;
			queryTimeout_ = connection_.queryTimeout_;
			fetchSize_ = SQLMXResultSet.DEFAULT_FETCH_SIZE;
			maxRows_ = 0;
			fetchDirection_ = ResultSet.FETCH_FORWARD;
			isClosed_ = false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_reuse].methodExit();
		}
	}

	public void close(boolean hardClose) throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_close].methodEntry();
		if (JdbcDebugCfg.traceActive)
			debug[methodId_close].methodParameters(Boolean.toString(hardClose));
		try {
			clearWarnings();
			synchronized (connection_) {
				if (connection_.isClosed_)
					return;
				try {
					if (hardClose){
						close(connection_.server_, connection_.getDialogueId(),
								stmtId_, true);
					connection_.hClosestmtCount++;

					if (connection_.out_ != null) {
						if (connection_.traceFlag_ >= T2Driver.POOLING_LVL){
							connection_.out_.println(getTraceId() + "close() "
									+ "Hard closed statement - " + "\""
									+ this.stmtId_ + "\"");
						connection_.out_.println(getTraceId() + "close() "
								+"\""+"HARDCLOSED STMTS COUNT:"+connection_.hClosestmtCount+"\"");
						}
					}

					}
					else
						logicalClose();
				} finally {
					isClosed_ = true;
					if (hardClose)
						connection_.removeElement(this);
				}
			}// End sync
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_close].methodExit();
		}
	}

	void populateLobObjects() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_populateLobObjects].methodEntry();
		try {
			int len;
			Object lob;

			if (lobObjects_ != null) {
				len = lobObjects_.size();
				for (int i = 0; i < len; i++) {
					lob = lobObjects_.get(i);
					if (lob instanceof SQLMXClob) 
						((SQLMXClob) lob).populate();
					else
						((SQLMXBlob) lob).populate();
				}
			}
			isAnyLob_ = false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_populateLobObjects].methodExit();
		}
	}

	// Constructors with access specifier as "default"
	SQLMXPreparedStatement(SQLMXConnection connection, String sql)
			throws SQLException {
		this(connection, sql, ResultSet.TYPE_FORWARD_ONLY,
				ResultSet.CONCUR_READ_ONLY, connection.holdability_);
	}

	SQLMXPreparedStatement(SQLMXConnection connection, String sql,
			int resultSetType, int resultSetConcurrency) throws SQLException {
		this(connection, sql, resultSetType, resultSetConcurrency,
				connection.holdability_);
	}

	SQLMXPreparedStatement(SQLMXConnection connection, String sql,
			int resultSetType, int resultSetConcurrency,
			int resultSetHoldability) throws SQLException {
		super(connection, resultSetType, resultSetConcurrency,
				resultSetHoldability);

		if (JdbcDebugCfg.entryActive)
			debug[methodId_SQLMXPreparedStatement_LLIII].methodEntry();
		try {
			if (connection.isClosed_)
				throw Messages.createSQLException(connection_.locale_,
						"invalid_connection", null);
			isSelect_ = getStmtSqlType(sql);
			short stmtType = SQLMXConnection.getSqlStmtType(sql);
			sql_ = sql;
			this.setSqlType(stmtType);
			if(stmtType == SQLMXConnection.TYPE_CONTROL){
				isCQD = true;
			}else {
				isCQD = false;
			}
			batchBindingSize_ = connection.batchBindingSize_;
			nf.setGroupingUsed(false); // Turn off adding any digit separators
			// used in setBigDecimal
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_SQLMXPreparedStatement_LLIII].methodExit();
		}
	}

	// native methods
	native void prepare(String server, long dialogueId, int txid,
			boolean autoCommit, String stmtLabel, String sql, boolean isSelect,
			int queryTimeout, int holdability, int batchSize, int fetchSize);

	private native void execute(String server, long dialogueId, int txid,
			boolean autoCommit, int txnMode, long stmtId, String cursorName,
			boolean isSelect, int paramRowCount, int paramCount,
			Object paramValues, int queryTimeout, boolean isAnyLob,
			String iso88591Encoding, SQLMXResultSet resultSet,
			boolean contBatchOnError) throws SQLException;

	private native void resetFetchSize(long dialogueId, long stmtId, int fetchSize);

	// fields
	SQLMXDesc[] inputDesc_;
	int paramRowCount_;
	String moduleName_;
	int moduleVersion_;
	long moduleTimestamp_;
	int batchBindingSize_;

	ArrayList<Object> rowsValue_;
	DataWrapper paramContainer_;
	boolean isAnyLob_;
	ArrayList<SQLMXLob> lobObjects_;
	ArrayList<String> lobColNames_;
	ArrayList<Integer> lobColIds_;
	String[] lobLocators_;
	boolean lobColDone_;

	boolean isCQD;
	short sqlType;
	private DataWrapper dataWrapper = new DataWrapper(1);

	private static final NumberFormat nf = NumberFormat
			.getIntegerInstance(Locale.US); // Used by setBigDecimal for SQL/MX
	// SQLTYPECODE_DECIMAL
	private static final long unsignedShortMaxValue = 65535L;
	private static final long unsignedShortMinValue = 0L;
	private static final long unsignedIntMaxValue = 4294967295L;
	private static final long unsignedIntMinValue = 0L;

	/*
	 * The following static BigInteger values are used to see if a BigDecimal
	 * will overflow the storage size when converted. All NUMERIC data types are
	 * sent to SQL/MX as unscaled types and must fit into the storage size for
	 * the conversion.
	 *
	 * For Example:
	 *
	 * If you have a BigDecimal and you what to convert it to a int. You would
	 * use the BigDecimal.intValue(), but you will have to first check to make
	 * sure that the BigDecimal.unscaledValue() is less than Integer.MAX_VALUE
	 * but greater than Integer.MIN_VALUE. If you do not do this check, you will
	 * not get the correct value when BigDecimal is too big or small.
	 */

	private static final BigInteger decIntMaxLong = new BigInteger(String
			.valueOf(Long.MAX_VALUE));
	private static final BigInteger decIntMinLong = new BigInteger(String
			.valueOf(Long.MIN_VALUE));
	private static final Double doubleLongMax = new Double(Long.MAX_VALUE); // The
	// max
	// long
	// as
	// a
	// Double
	private static final Double doubleLongMin = new Double(Long.MIN_VALUE); // The
	// min
	// long
	// as
	// a
	// Double
	private static final Float floatLongMax = new Float(Long.MAX_VALUE); // The
	// max
	// long
	// as
	// a
	// Float(can
	// be
	// different
	// than
	// a
	// Double)
	private static final Float floatLongMin = new Float(Long.MIN_VALUE); // The
	// min
	// long
	// as
	// a
	// Float(can
	// be
	// different
	// than
	// a
	// Double)

	private static int methodId_addBatch = 0;
	private static int methodId_clearBatch = 1;
	private static int methodId_clearParameters = 2;
	private static int methodId_execute = 3;
	private static int methodId_executeBatch = 4;
	private static int methodId_executeQuery = 5;
	private static int methodId_executeUpdate = 6;
	private static int methodId_getMetaData = 7;
	private static int methodId_getParameterMetaData = 8;
	private static int methodId_setArray = 9;
	private static int methodId_setAsciiStream = 10;
	private static int methodId_setBigDecimal = 11;
	private static int methodId_setBinaryStream = 12;
	private static int methodId_setBlob = 13;
	private static int methodId_setBoolean = 14;
	private static int methodId_setBoolean_I = 15;
	private static int methodId_setByte = 16;
	private static int methodId_setBytes = 17;
	private static int methodId_setCharacterStream = 18;
	private static int methodId_setClob = 19;
	private static int methodId_setDate_IL = 20;
	private static int methodId_setDate_ILL = 21;
	private static int methodId_setDouble = 22;
	private static int methodId_setDouble_I = 23;
	private static int methodId_setFloat = 24;
	private static int methodId_setFloat_I = 25;
	private static int methodId_setInt = 26;
	private static int methodId_setInt_I = 27;
	private static int methodId_setLong = 28;
	private static int methodId_setLong_I = 29;
	private static int methodId_setNull_II = 30;
	private static int methodId_setNull_IIL = 31;
	private static int methodId_setObject_IL = 32;
	private static int methodId_setObject_ILI = 33;
	private static int methodId_setObject_ILII = 34;
	private static int methodId_setRef = 35;
	private static int methodId_setShort = 36;
	private static int methodId_setShort_I = 37;
	private static int methodId_setString = 38;
	private static int methodId_setTime_IL = 39;
	private static int methodId_setTime_ILL = 40;
	private static int methodId_setTimestamp_IL = 41;
	private static int methodId_setTimestamp_ILL = 42;
	private static int methodId_setUnicodeStream = 43;
	private static int methodId_setURL = 44;
	private static int methodId_validateExecuteInvocation = 45;
	private static int methodId_checkIfAllParamsSet = 46;
	private static int methodId_validateSetInvocation = 47;
	private static int methodId_addParamValue = 48;
	private static int methodId_getParameters = 49;
	private static int methodId_logicalClose = 50;
	private static int methodId_setPrepareOutputs = 51;
	private static int methodId_setExecuteOutputs = 52;
	private static int methodId_getOutFSDataType = 53;
	private static int methodId_getOutScale = 54;
	private static int methodId_getSQLDataType = 55;
	private static int methodId_getSQLOctetLength = 56;
	private static int methodId_getPrecision = 57;
	private static int methodId_getScale = 58;
	private static int methodId_getFSDataType = 59;
	private static int methodId_getSqlDatetimeCode = 60;
	private static int methodId_reuse = 61;
	private static int methodId_close = 62;
	private static int methodId_populateLobObjects = 63;
	private static int methodId_SQLMXPreparedStatement_LLIII = 65;
	private static int methodId_SQLMXPreparedStatement_LLIJLZI = 66;
	private static int methodId_setFetchSize = 67;
	private static int methodId_getFetchSize = 68;
	
	private static int totalMethodIds = 69;
	
	private static JdbcDebug[] debug;

	static {
		String className = "SQLMXPreparedStatement";
		if (JdbcDebugCfg.entryActive) {
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_addBatch] = new JdbcDebug(className, "addBatch");
			debug[methodId_clearBatch] = new JdbcDebug(className, "clearBatch");
			debug[methodId_clearParameters] = new JdbcDebug(className,
					"clearParameters");
			debug[methodId_execute] = new JdbcDebug(className, "execute");
			debug[methodId_executeBatch] = new JdbcDebug(className,
					"executeBatch");
			debug[methodId_executeQuery] = new JdbcDebug(className,
					"executeQuery");
			debug[methodId_executeUpdate] = new JdbcDebug(className,
					"executeUpdate");
			debug[methodId_getMetaData] = new JdbcDebug(className,
					"getMetaData");
			debug[methodId_getParameterMetaData] = new JdbcDebug(className,
					"getParameterMetaData");
			debug[methodId_setArray] = new JdbcDebug(className, "setArray");
			debug[methodId_setAsciiStream] = new JdbcDebug(className,
					"setAsciiStream");
			debug[methodId_setBigDecimal] = new JdbcDebug(className,
					"setBigDecimal");
			debug[methodId_setBinaryStream] = new JdbcDebug(className,
					"setBinaryStream");
			debug[methodId_setBlob] = new JdbcDebug(className, "setBlob");
			debug[methodId_setBoolean] = new JdbcDebug(className, "setBoolean");
			debug[methodId_setBoolean_I] = new JdbcDebug(className,
					"setBoolean_I");
			debug[methodId_setByte] = new JdbcDebug(className, "setByte");
			debug[methodId_setBytes] = new JdbcDebug(className, "setBytes");
			debug[methodId_setCharacterStream] = new JdbcDebug(className,
					"setCharacterStream");
			debug[methodId_setClob] = new JdbcDebug(className, "setClob");
			debug[methodId_setDate_IL] = new JdbcDebug(className, "setDate_IL");
			debug[methodId_setDate_ILL] = new JdbcDebug(className,
					"setDate_ILL");
			debug[methodId_setDouble] = new JdbcDebug(className, "setDouble");
			debug[methodId_setDouble_I] = new JdbcDebug(className,
					"setDouble_I");
			debug[methodId_setFloat] = new JdbcDebug(className, "setFloat");
			debug[methodId_setFloat_I] = new JdbcDebug(className, "setFloat_I");
			debug[methodId_setInt] = new JdbcDebug(className, "setInt");
			debug[methodId_setInt_I] = new JdbcDebug(className, "setInt_I");
			debug[methodId_setLong] = new JdbcDebug(className, "setLong");
			debug[methodId_setLong_I] = new JdbcDebug(className, "setLong_I");
			debug[methodId_setNull_II] = new JdbcDebug(className, "setNull_II");
			debug[methodId_setNull_IIL] = new JdbcDebug(className,
					"setNull_IIL");
			debug[methodId_setObject_IL] = new JdbcDebug(className,
					"setObject_IL");
			debug[methodId_setObject_ILI] = new JdbcDebug(className,
					"setObject_ILI");
			debug[methodId_setObject_ILII] = new JdbcDebug(className,
					"setObject_ILII");
			debug[methodId_setRef] = new JdbcDebug(className, "setRef");
			debug[methodId_setShort] = new JdbcDebug(className, "setShort");
			debug[methodId_setShort_I] = new JdbcDebug(className, "setShort_I");
			debug[methodId_setString] = new JdbcDebug(className, "setString");
			debug[methodId_setTime_IL] = new JdbcDebug(className, "setTime_IL");
			debug[methodId_setTime_ILL] = new JdbcDebug(className,
					"setTime_ILL");
			debug[methodId_setTimestamp_IL] = new JdbcDebug(className,
					"setTimestamp_IL");
			debug[methodId_setTimestamp_ILL] = new JdbcDebug(className,
					"setTimestamp_ILL");
			debug[methodId_setUnicodeStream] = new JdbcDebug(className,
					"setUnicodeStream");
			debug[methodId_setURL] = new JdbcDebug(className, "setURL");
			debug[methodId_validateExecuteInvocation] = new JdbcDebug(
					className, "validateExecuteInvocation");
			debug[methodId_checkIfAllParamsSet] = new JdbcDebug(className,
					"checkIfAllParamsSet");
			debug[methodId_validateSetInvocation] = new JdbcDebug(className,
					"validateSetInvocation");
			debug[methodId_addParamValue] = new JdbcDebug(className,
					"addParamValue");
			debug[methodId_getParameters] = new JdbcDebug(className,
					"getParameters");
			debug[methodId_logicalClose] = new JdbcDebug(className,
					"logicalClose");
			debug[methodId_setPrepareOutputs] = new JdbcDebug(className,
					"setPrepareOutputs");
			debug[methodId_setExecuteOutputs] = new JdbcDebug(className,
					"setExecuteOutputs");
			debug[methodId_getOutFSDataType] = new JdbcDebug(className,
					"getOutFSDataType");
			debug[methodId_getOutScale] = new JdbcDebug(className,
					"getOutScale");
			debug[methodId_getSQLDataType] = new JdbcDebug(className,
					"getSQLDataType");
			debug[methodId_getSQLOctetLength] = new JdbcDebug(className,
					"getSQLOctetLength");
			debug[methodId_getPrecision] = new JdbcDebug(className,
					"getPrecision");
			debug[methodId_getScale] = new JdbcDebug(className, "getScale");
			debug[methodId_getFSDataType] = new JdbcDebug(className,
					"getFSDataType");
			debug[methodId_getSqlDatetimeCode] = new JdbcDebug(className,
					"getSqlDatetimeCode");
			debug[methodId_reuse] = new JdbcDebug(className, "reuse");
			debug[methodId_close] = new JdbcDebug(className, "close");
			debug[methodId_populateLobObjects] = new JdbcDebug(className,
					"populateLobObjects");
			debug[methodId_SQLMXPreparedStatement_LLIII] = new JdbcDebug(
					className, "SQLMXPreparedStatement_LLIII");
			debug[methodId_SQLMXPreparedStatement_LLIJLZI] = new JdbcDebug(
					className, "SQLMXPreparedStatement_LLIJLZI");
		}
	}
	short getSqlType() {
		return sqlType;
	}

	void setSqlType(short sqlStmtType) {
		this.sqlType = sqlStmtType;
	}


	public boolean isClosed() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public void setPoolable(boolean poolable) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public boolean isPoolable() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public void closeOnCompletion() throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public boolean isCloseOnCompletion() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public Object unwrap(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isWrapperFor(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public void setRowId(int parameterIndex, RowId x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNString(int parameterIndex, String value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}
	
	public void setNCharacterStream(int parameterIndex, Reader value,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNClob(int parameterIndex, NClob value) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setClob(int parameterIndex, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setBlob(int parameterIndex, InputStream inputStream, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNClob(int parameterIndex, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setSQLXML(int parameterIndex, SQLXML xmlObject)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setAsciiStream(int parameterIndex, InputStream x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setBinaryStream(int parameterIndex, InputStream x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setCharacterStream(int parameterIndex, Reader reader,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setAsciiStream(int parameterIndex, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setBinaryStream(int parameterIndex, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setCharacterStream(int parameterIndex, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNCharacterStream(int parameterIndex, Reader value)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setClob(int parameterIndex, Reader reader) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setBlob(int parameterIndex, InputStream inputStream)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void setNClob(int parameterIndex, Reader reader) throws SQLException {
		// TODO Auto-generated method stub
		
	}
    public String getInputDescCatalogName(int parameterIndex){
        return inputDesc_[parameterIndex].catalogName_;
    }

    public String getInputDescTableName(int parameterIndex){
        return inputDesc_[parameterIndex].tableName_;
    }
    public String getInputDescName(int parameterIndex){
        return inputDesc_[parameterIndex].name_;
    }

	private void addLobObjects(int parameterIndex, SQLMXLob x)
	{
		if (lobObjects_ == null) {
			lobObjects_ = new ArrayList<SQLMXLob>();
			lobColNames_ = new ArrayList<String>();
			lobColIds_ = new ArrayList<Integer>();
		}
		if (! lobColDone_) {	
			lobColNames_.add(getInputDescName(parameterIndex-1));
			lobColIds_.add(parameterIndex);
		}
		lobObjects_.add(x);
	}

	private int getNumLobColumns() 
	{
		if (lobColNames_ == null)
			return 0;
		else
			return lobColNames_.size();
	}

	private String getLobColumns() 
	{
		if (lobColNames_ == null)
			return "";
		StringBuilder colNames = new StringBuilder();
		colNames.append(lobColNames_.get(0));
		for (int i = 1; i < lobColNames_.size(); i++)
			colNames.append(", ").append(lobColNames_.get(i));
		return colNames.toString();
	}

	private void setLobLocators() throws SQLException 
	{
		if (lobLocators_.length != lobObjects_.size())
			throw Messages.createSQLException(connection_.locale_, "lob_objects_and_locators_dont_match", null);
		int lobLocatorIdx = 0;
		for (SQLMXLob lobObject : lobObjects_) {
			if (lobObject != null)
				lobObject.setLobLocator(lobLocators_[lobLocatorIdx]);
			lobLocatorIdx++;
		}
	}

	private String[] getLobLocators() throws SQLException
	{
     	   SQLMXPreparedStatement lobLocatorStmt = null;
	   SQLMXResultSet lobLocatorRS = null;
	   int lBatchBindingSize = connection_.batchBindingSize_;
	   try {
		//String selectForLobLocator = "select " + getLobColumns() + " from ( " + this.sql_ + " ) as x";
		String selectForLobLocator = "select * from ( " + this.sql_ + " ) as x";
		connection_.batchBindingSize_ = paramRowCount_;
		lobLocatorStmt = (SQLMXPreparedStatement)connection_.prepareStatement(selectForLobLocator);
		lobLocatorStmt.copyParameters(this); 
		if (paramRowCount_ == 0)
			lobLocatorRS = (SQLMXResultSet)lobLocatorStmt.executeQuery();
		else {
			int[] batchRet = lobLocatorStmt.executeBatch();
			lobLocatorRS = (SQLMXResultSet)lobLocatorStmt.getResultSet();
		}
		int numLocators = ((paramRowCount_ == 0 ? paramRowCount_ = 1 : paramRowCount_) * getNumLobColumns());
		String lobLocators[] = new String[numLocators];
		int locatorsIdx = 0;
		while (lobLocatorRS.next()) {
			for (int i = 0; i < getNumLobColumns() ; i++) {
				if (locatorsIdx < lobLocators.length)
					//lobLocators[locatorsIdx++] = lobLocatorRS.getString(i+1);
					lobLocators[locatorsIdx++] = lobLocatorRS.getLobLocator(lobColIds_.get(i));
				else
					throw Messages.createSQLException(connection_.locale_, 
						"locators out of space" , null);
			}	
		}
		return lobLocators;
	   } finally {
		if (lobLocatorRS != null)
			lobLocatorRS.close();
		if (lobLocatorStmt != null)
			lobLocatorStmt.close();
		connection_.batchBindingSize_ = lBatchBindingSize;
	   }
	}

//------------------------------
}
