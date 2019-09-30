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
 * Filename    : SQLMXDataSource.java
 * Description :
 *
 */
/*
 * Methods Changed: getConnection(),getReference(),
 * 		  : getDataSourceProperties(),getMinPoolSize()
 *                : createExternalCallableStatment()
 * Methods Added: setInitialPoolSize(int), getInitialPoolSize(),
 * 				  getInternalMinPoolSize(), getInternalMaxPoolSize()
 */

/*
 * Methods Changed: static{} block
 *                : getConnection()
 */
package org.apache.trafodion.jdbc.t2;

import java.io.PrintWriter;
import java.sql.*;
import java.text.SimpleDateFormat;

import javax.sql.*;

import java.util.Date;
import java.util.Properties;
import javax.naming.Context;
import javax.naming.Referenceable;
import javax.naming.NamingException;
import javax.naming.Reference;
import javax.naming.StringRefAddr;

import java.util.Locale;
import java.lang.ref.*;
import java.util.HashMap;
import java.util.logging.Logger;
import org.apache.trafodion.jdbc.t2.*;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.FileOutputStream;

/**
 * <p>
 * A <code>DataSource</code> object is a factory for Connection objects. An
 * object that implements the DataSource interface will typically be registered
 * with a JNDI service provider. A JDBC driver that is accessed via the
 * <code>DataSource</code> API does not automatically register itself with the
 * <code>DriverManager</code> object.
 *
 * @version Trafodion JDBC/MX
 */

public class SQLMXDataSource extends T2Properties implements
		javax.sql.DataSource, java.io.Serializable, Referenceable {
	/**
	 * <p>
	 * Attempts to establish a database connection.
	 *
	 * @return a Connection to the database
	 * @exception SQLException
	 *                if a database-access error occurs.
	 */
// Made synchronize 
	synchronized public Connection getConnection() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getConnection_V]
					.methodEntry(JdbcDebug.debugLevelPooling);

		try {
			Connection connect;
			SQLMXConnection mxConnect;
			SQLMXConnectionPoolDataSource pds;

			weakConnection.gcConnections();
			//MaxIdleTime is considered only if Connection Pooling is enabled with minPoolSize being > 0
//			if(((this.getMaxIdleTime() > 0) || ((Integer.parseInt(System.getProperty("t2jdbc.maxIdleTime", "0")))> 0) || ((Integer.parseInt(System.getProperty("maxIdleTime", "0"))) > 0))
////					&& ((this.getInternalMaxPoolSize() > 0) || ((Integer.parseInt(System.getProperty("t2jdbc.maxPoolSize", "0"))) > 0) || ((Integer.parseInt(System.getProperty("maxPoolSize", "0"))) > 0))) {
//					&& ((this.getInternalMaxPoolSize() > 0 && this.getInternalMinPoolSize() > 0) || ((Integer.parseInt(System.getProperty("t2jdbc.maxPoolSize", "0"))) > 0 && (Integer.parseInt(System.getProperty("t2jdbc.minPoolSize", "0"))) > 0)
//							|| ((Integer.parseInt(System.getProperty("maxPoolSize", "0"))) > 0 && (Integer.parseInt(System.getProperty("minPoolSize", "0"))) > 0))) {
			if((this.getMaxIdleTime() > 0) && (this.getInternalMaxPoolSize() > 0 && this.getInternalMinPoolSize() > 0) ) {
				if (!threadCreated) {
					synchronized (SQLMXDataSource.class) {
						if (!threadCreated) {
								SQLMXMaxIdleTimeRunnable.getSQLMXMaxIdleTimeRunnable()
										.setSleepTime(this.getMaxIdleTime());
			
							Thread connectionPoolScavenger = new Thread(
									SQLMXMaxIdleTimeRunnable
											.getSQLMXMaxIdleTimeRunnable());
							try {
								connectionPoolScavenger.setDaemon(true);
							} catch (SecurityException secExc) {
								SQLException ex = new SQLException(
										"Daemon thread to monitor the maxIdleTime property could not be started "
												+ secExc.getMessage());
								throw ex;
							}
							connectionPoolScavenger.start();
							threadCreated = true;
						}
					}
				}
			}
			if ((this.getTraceFlag() >= T2Driver.POOLING_LVL) && (this.getLogWriter() == null) && (this.getTraceFile() != null)) {
				synchronized (SQLMXDataSource.class) {
					if (traceWriter_ == null) {
						try {
							traceWriter_ = new PrintWriter(new FileOutputStream(
									this.getTraceFile(), true), true);
						} catch (java.io.IOException e) {
							traceWriter_ = new PrintWriter(System.err, true);
						}
						this.setLogWriter(traceWriter_);
					} else
						this.setLogWriter(traceWriter_);
				}
			}

			//R3.2 changes -- start
			if (this.getQueryExecuteTime() > 0) {
				if (queryExecuteTraceWriter_ == null
							&& this.getT2QueryExecuteLogFile() != null) {
					synchronized(SQLMXDataSource.class){
						File directory = new File(this.getT2QueryExecuteLogFile());
						if (directory.isDirectory()) {
							// Generate a file name.
							String pid = Integer.toString(T2Driver.getPid());
							try {
								T2QueryExecuteLogFile_ = directory
										.getCanonicalPath()
										+ "/" + "t2sqlmxquerytime" + pid + ".log";
							this.setT2QueryExecuteLogFile(T2QueryExecuteLogFile_);
							} catch (IOException e) {
								// TODO Auto-generated catch block
								RuntimeException ex = new RuntimeException(e
										.getMessage());
	
								ex.setStackTrace(e.getStackTrace());
								throw ex;
	
							}
						}
						try {
							queryExecuteTraceWriter_ = new PrintWriter(
									new FileOutputStream(
											getT2QueryExecuteLogFile(), true), true);
							queryExecuteTraceWriter_.println(T2Driver.printTraceVproc);
						} catch (java.io.IOException e) {
							queryExecuteTraceWriter_ = new PrintWriter(System.err,
									true);
						}
					}
				}
			}
			//R3.2 changes -- end


			// Check if it is be treated as PooledConnection
			if (poolManager_ != null)
				mxConnect = (SQLMXConnection) poolManager_.getConnection();
			else {

				int maxPS = getMaxPoolSize();
				int minPS = getMinPoolSize();

//				if ((maxPS == -1) || (maxPS == 0 && minPS == 0)) // Connection pooling disabled
				if (maxPS == -1)
																	// disabled
				{
//					mxConnect = new SQLMXConnection(this,getDataSourceProperties());
					mxConnect = new SQLMXConnection(this,getT2Properties());
					weakConnection.refToDialogueId_.put(mxConnect.pRef_, new Long(mxConnect
							.getDialogueId()));
				} else {

					int maxSt = getMaxStatements();
					int initialPS = getInitialPoolSize();
					int maxIT = getMaxIdleTime();

//					pds = new SQLMXConnectionPoolDataSource();
					pds = new SQLMXConnectionPoolDataSource(getProperties());
					poolManager_ = new SQLMXPooledConnectionManager(pds,getTraceFlag());
					if (poolManager_ != null)
						poolManager_.setLogWriter(this.getLogWriter());
					mxConnect = (SQLMXConnection) poolManager_.getConnection();
				}
			}

			if (url_ != null) {
				mxConnect.url_ = url_;
			}

			if (poolManager_ != null)
				poolManager_.setLogWriter(this.getLogWriter());

			mxConnect.setLogInfo(getTraceFlag(), this.getLogWriter());

			if (getTraceFlag() >= T2Driver.ENTRY_LVL) {
				mxConnect.setTracer(this.getLogWriter());
			}
			return mxConnect;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getConnection_V].methodExit();
		}
	}

	/**
	 * <p>
	 * Attempts to establish a database connection.
	 *
	 * @param user
	 *            the database user on whose behalf the Connection is being made
	 * @param password
	 *            the user's password
	 * @return a Connection to the database
	 * @exception SQLException
	 *                if a database-access error occurs.
	 */
	 // Made synchronize 
	synchronized public Connection getConnection(String username, String password)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getConnection_LL].methodEntry();
		try {
			return getConnection();
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getConnection_LL].methodExit();
		}
	}



	/**
	 * Returns the <code>Reference</code> to this data source.
	 *
	 * @return the <code>Reference</code> to this data source.
	 * @exception throws NamingException if the object factory for this data
	 *            source cannot be found.
	 */
	public Reference getReference() throws NamingException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getReference].methodEntry();
		try {
			Reference ref;

			ref = new Reference(this.getClass().getName(),"org.apache.trafodion.jdbc.t2.SQLMXDataSourceFactory", null);
			ref = super.addReferences(ref);

			return ref;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getReference].methodExit();
		}
	}


	int getInternalMaxPoolSize() {
		if (this.getMaxPoolSize() <= -1) {
			this.setMaxPoolSize(-1);
		} else {
			if (this.getMaxPoolSize() > 0) {
				if (this.getMinPoolSize() != 0 && this.getMinPoolSize() > this.getMaxPoolSize()) {
//					maxPoolSize_ = minPoolSize_;
					this.setMaxPoolSize(this.getMinPoolSize());
				}
			}
		}
//		return maxPoolSize_;
		return getMaxPoolSize();
	}


	int getInternalMinPoolSize() {
		int minPoolSize_ = this.getMinPoolSize();
		int maxPoolSize_ = this.getMaxPoolSize();

		if (maxPoolSize_ > 0 && minPoolSize_ <= 0) {
//			minPoolSize_=0;
			this.setMinPoolSize(0);
			return this.getMinPoolSize();
		}
		if (maxPoolSize_ <= 0 && minPoolSize_ >= maxPoolSize_) {
//			minPoolSize_=0;
			this.setMinPoolSize(0);
			return this.getMinPoolSize();
		}
		if (maxPoolSize_ > 0 && minPoolSize_ > 0) {
			if (minPoolSize_ > maxPoolSize_) {
//				minPoolSize_ = maxPoolSize_;
				this.setMinPoolSize(maxPoolSize_);
			}
//			return minPoolSize_;
			return this.getMinPoolSize();
		}
		if (maxPoolSize_ <= 0 && minPoolSize_ <= 0) {
//			minPoolSize_=0;
			this.setMinPoolSize(0);
			return this.getMinPoolSize();
		}
		return this.getMinPoolSize();
	}


	// Local methods
	Properties getDataSourceProperties() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDataSourceProperties].methodEntry();
		try {
			Properties info;

			info = getProperties();

			return info;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDataSourceProperties].methodExit();
		}
	}

	void setPoolManager(Context nameCtx, String dataSourceName)
			throws Exception {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setPoolManager].methodEntry();
		try {
			Object pds;

			pds = nameCtx.lookup(dataSourceName);
			if (pds instanceof SQLMXConnectionPoolDataSource)
				poolManager_ = new SQLMXPooledConnectionManager(
						(SQLMXConnectionPoolDataSource) pds, this.getTraceFlag());
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setPoolManager].methodExit();
		}
	}

	void setURL(String url) {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setURL].methodEntry();
		try {
			url_ = url;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setURL].methodExit();
		}
	}

	public SQLMXDataSource() {
		super();
		String lang = this.getLanguage();
		if (lang == null)
			locale_ = Locale.getDefault();
		else {
			if (lang.equalsIgnoreCase("ja"))
				locale_ = new Locale("ja", "", "");
			else if (lang.equalsIgnoreCase("en"))
				locale_ = new Locale("en", "", "");
			else
				locale_ = Locale.getDefault();
		}
		
		if (JdbcDebugCfg.entryActive) {
			debug[methodId_SQLMXDataSource].methodEntry();
			debug[methodId_SQLMXDataSource].methodExit();
		}
	}

	public SQLMXDataSource(Properties props) {
		super(props);
		String lang = this.getLanguage();
		if (lang == null)
			locale_ = Locale.getDefault();
		else {
			if (lang.equalsIgnoreCase("ja"))
				locale_ = new Locale("ja", "", "");
			else if (lang.equalsIgnoreCase("en"))
				locale_ = new Locale("en", "", "");
			else
				locale_ = Locale.getDefault();
		}
		
				
		if (JdbcDebugCfg.entryActive) {
			debug[methodId_SQLMXDataSource].methodEntry();
			debug[methodId_SQLMXDataSource].methodExit();
		}
	}

	int loginTimeout_;
	//PrintWriter logWriter_;


	static boolean threadCreated = false;

	static int nowaitOn_;
	static Locale locale_;
	static PrintWriter traceWriter_;

	//R3.2 changes -- start

	private static String T2QueryExecuteLogFile_;
	public static PrintWriter queryExecuteTraceWriter_;



	static PrintWriter prWriter_;

	SQLMXPooledConnectionManager poolManager_;


	// serialVersionUID set to resolve JDK5.0 javax -Xlint:serial definition
	// warning.
	private static final long serialVersionUID = 1L;

	private static int methodId_getConnection_V = 0;
	private static int methodId_getConnection_LL = 1;
	private static int methodId_getLogWriter = 2;
	private static int methodId_setLogWriter = 3;
	private static int methodId_getReference = 4;
	private static int methodId_getDataSourceProperties = 5;
	private static int methodId_setPoolManager = 6;
	private static int methodId_setURL = 7;
	private static int methodId_SQLMXDataSource = 8;

	private static int totalMethodIds = 9;
	private static JdbcDebug[] debug;

		static {

			String className = "SQLMXDataSource";
			if (JdbcDebugCfg.entryActive) {
				debug = new JdbcDebug[totalMethodIds];
				debug[methodId_getConnection_V] = new JdbcDebug(className,
						"getConnection_V");
				debug[methodId_getConnection_LL] = new JdbcDebug(className,
						"getConnection_LL");
				debug[methodId_getLogWriter] = new JdbcDebug(className,
						"getLogWriter");
				debug[methodId_setLogWriter] = new JdbcDebug(className,
						"setLogWriter");
				debug[methodId_getReference] = new JdbcDebug(className,
						"getReference");
				debug[methodId_getDataSourceProperties] = new JdbcDebug(className,
						"getDataSourceProperties");
				debug[methodId_setPoolManager] = new JdbcDebug(className,
						"setPoolManager");
				debug[methodId_setURL] = new JdbcDebug(className, "setURL");

				debug[methodId_SQLMXDataSource] = new JdbcDebug(className,
						"SQLMXDataSource");

			}
	}



	public Logger getParentLogger() throws SQLFeatureNotSupportedException {
		// TODO Auto-generated method stub
		return null;
	}

	public Object unwrap(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isWrapperFor(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}
}
