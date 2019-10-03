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
 * Filename    : T2Driver.java
 * Description :
 */
package org.apache.trafodion.jdbc.t2;


import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;
import java.util.logging.Logger;

public class T2Driver extends T2Properties implements java.sql.Driver {
	// The vproc needs to be exposed public so JdbcT2.java can access it
	public static String driverVproc = DriverInfo.driverVproc;
	
	// java.sql.Driver interface methods
	public boolean acceptsURL(String url) {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_acceptsURL].methodEntry();
		try {
			// Do we need to ignorecase or white spaces, may be not
			if (url.indexOf("jdbc:t2jdbc:", 0) == 0)
				return true;
			else
				return false;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_acceptsURL].methodExit();
		}
	}
	public java.sql.Connection connect(String url, java.util.Properties info)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_connect].methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive)
			debug[methodId_connect].methodParameters("url=" + url + ", info="
					+ JdbcDebug.debugObjectStr(info));
		try {
			Connection connect = null;
			String key;
			SQLMXDataSource ds = null;
			
			String delimiter = "|";
			if (acceptsURL(url)) {
				weakConnection.gcConnections();
				
				initialize(info);
				
				String stmtatomicity_ = getStmtatomicity();
				if (stmtatomicity_ != null) {
					if (stmtatomicity_.equalsIgnoreCase("on")) {
						stmtatomicityval_ = 1;
						stmtatomicity_ = stmtatomicity_.toUpperCase();
					} else
						stmtatomicity_ = "OFF";
				} else
					stmtatomicity_ = "OFF";
				String Spjrs_;
				int Spjrsval_ = 0;
				
				Spjrs_ = getSpjrs();
				if (Spjrs_ != null) {
					if (Spjrs_.equalsIgnoreCase("on")) {
						Spjrsval_ = 1;
						Spjrs_ = Spjrs_.toUpperCase();
					} else
						Spjrs_ = "OFF";
				} else
					Spjrs_ = "OFF";
				
				traceFlag_ = getTraceFlag();
				
				// Set the driver default encoding to the current java encoding.
				// Only way to get the current encoding is through the
				// OutputStreamWriter using the ByteArrayOutputStream constructor.
				setDefaultEncoding(new java.io.OutputStreamWriter(
						new java.io.ByteArrayOutputStream()).getEncoding());

				// Set the ISO88591 charset encoding override if not null
				if (getIso88591EncodingOverride() != null)
					setCharsetEncodingOverride(SQLMXDesc.SQLCHARSETCODE_ISO88591,getIso88591EncodingOverride());
				
				if (info != null) {


					// Note1: The url is not part of the key since the url
					// (currently) is always "jdbc:sqlmx:"
					// This will have to change if additional url's are added.
					// Note2: The delimiter is required to always ensure a
					// unique key is created
					// RFE: Batch update improvements
					key = getLanguage() + delimiter + getCatalog() + delimiter + getSchema()
							+ delimiter + getBatchBinding() + delimiter
							+ getMaxPoolSize() + delimiter + getMinPoolSize() + delimiter
							+ getMaxStatements() + delimiter + getTraceFlag() + delimiter
							+ delimiter + getTransactionMode() + delimiter
							+ getIso88591EncodingOverride() + delimiter
							+ getContBatchOnError();

					// The generated key is converted to upper case to make it
					// unique
					// to allow lower and/or upper case string properties.
					key = key.toUpperCase();
					ds = (SQLMXDataSource) dsCache_.get(key);
					if (ds == null) {
						ds = new SQLMXDataSource(getProperties());

						dsCache_.put(key, ds);
					}
				} 
				connect = ds.getConnection();			
				
			}
			return connect;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_connect].methodExit();
		}
	}

	public int getMajorVersion() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMajorVersion].methodEntry();
		try {
			if (JdbcDebugCfg.traceActive)
				debug[methodId_getMajorVersion].methodReturn(Integer
						.toString(databaseMajorVersion_));
			return databaseMajorVersion_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMajorVersion].methodExit();
		}
	}

	public int getMinorVersion() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getMinorVersion].methodEntry();
		try {
			if (JdbcDebugCfg.traceActive)
				debug[methodId_getMinorVersion].methodReturn(Integer
						.toString(databaseMinorVersion_));
			return databaseMinorVersion_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getMinorVersion].methodExit();
		}
	}

	public java.sql.DriverPropertyInfo[] getPropertyInfo(String url,
			java.util.Properties info) {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getPropertyInfo].methodEntry();
		try {
			if (acceptsURL(url)) {
				return super.getPropertyInfo(url, info);
			}

			return null;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getPropertyInfo].methodExit();
		}
	}

	public boolean jdbcCompliant() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_jdbcCompliant].methodEntry();
		try {
			return true;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_jdbcCompliant].methodExit();
		}
	}

	static int getDatabaseMajorVersion() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDatabaseMajorVersion].methodEntry();
		try {
			return databaseMajorVersion_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDatabaseMajorVersion].methodExit();
		}
	}

	static int getDatabaseMinorVersion() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDatabaseMinorVersion].methodEntry();
		try {
			return databaseMinorVersion_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDatabaseMinorVersion].methodExit();
		}
	}

	// Fields
	private static T2Driver singleton_;

    private static Locale locale_;
	
	static int stmtatomicityval_;

	// dateFormat object used to format the timestamps printed in jdbcTrace
	// output.
	// Output format set at: [08/01/05 09:52:54]
	public static DateFormat dateFormat = new SimpleDateFormat(
			"MM/dd/yy hh:mm:ss");

	// Text which will be pre-appended to jdbcTrace output.
	public static String traceText = "jdbcTrace:[";

	// Number of characters required to strip off the "org.apache.trafodion.jdbc.t2."
	// from the method name in jdbcTrace output.
	public static int REMOVE_PKG_NAME = 17;

	// JDBC/MX vproc string to log in jdbcTrace driver output.
	public static String printTraceVproc = traceText
			+ dateFormat.format(new Date()) + "]:"
			+ "TRACING JDBC/MX VERSION: " + driverVproc;

	// Trace control levels and bit flags used to minimize jdbcTrace output.
	public static final int POOLING_LVL = 1; // Trace Connection and Statement
	// Pooling only
	public static final int LOB_LVL = 2; // Trace LOB code path only
	public static final int ENTRY_LVL = 3; // Trace all Java method entry

	static final int DATABASE_MAJOR_VERSION_R20 = 3; //changed for R3.0 TCF
	static final int DATABASE_MINOR_VERSION_R20 = 0;
	static final int databaseMajorVersion_;
	static final int databaseMinorVersion_;
	static int pid_;

	static HashMap<String, SQLMXDataSource> dsCache_;

	static int traceFlag_;
	// Constructors
	public T2Driver() {
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
			debug[methodId_T2Driver].methodEntry();
			debug[methodId_T2Driver].methodExit();
		}
	}

	// Native methods
	static native int getPid();

	native static void SQLMXInitialize(String language);

	native static void setDefaultEncoding(String encoding);

	native static void checkLibraryVersion(String javaVproc);

	native static void setCharsetEncodingOverride(int charset, String encoding);

	private native static int getDatabaseMajorVersionJNI();

	private native static int getDatabaseMinorVersionJNI();

	private static int methodId_acceptsURL = 0;
	private static int methodId_getPropertyValue = 1;
	private static int methodId_connect = 2;
	private static int methodId_getStaticMajorVersion = 3;
	private static int methodId_getStaticMinorVersion = 4;
	private static int methodId_getMajorVersion = 5;
	private static int methodId_getMinorVersion = 6;
	private static int methodId_getPropertyInfo = 7;
	private static int methodId_jdbcCompliant = 8;
	private static int methodId_getDatabaseMajorVersion = 9;
	private static int methodId_getDatabaseMinorVersion = 10;
	private static int methodId_T2Driver = 11;
	private static int totalMethodIds = 12;
	private static JdbcDebug[] debug;

	// initializer to register the Driver with the Driver manager
	static {
		String className = "T2Driver";

		if (JdbcDebugCfg.entryActive) {
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_acceptsURL] = new JdbcDebug(className, "acceptsURL");
			debug[methodId_getPropertyValue] = new JdbcDebug(className,
					"getPropertyValue");
			debug[methodId_connect] = new JdbcDebug(className, "connect");
			debug[methodId_getStaticMajorVersion] = new JdbcDebug(className,
					"getStaticMajorVersion");
			debug[methodId_getStaticMinorVersion] = new JdbcDebug(className,
					"getStaticMinorVersion");
			debug[methodId_getMajorVersion] = new JdbcDebug(className,
					"getMajorVersion");
			debug[methodId_getMinorVersion] = new JdbcDebug(className,
					"getMinorVersion");
			debug[methodId_getPropertyInfo] = new JdbcDebug(className,
					"getPropertyInfo");
			debug[methodId_jdbcCompliant] = new JdbcDebug(className,
					"jdbcCompliant");
			debug[methodId_getDatabaseMajorVersion] = new JdbcDebug(className,
					"getDatabaseMajorVersion");
			debug[methodId_getDatabaseMinorVersion] = new JdbcDebug(className,
					"getDatabaseMinorVersion");
			debug[methodId_T2Driver] = new JdbcDebug(className,
					"T2Driver");
		}

		singleton_ = new T2Driver();
		// Register the Driver with the Driver Manager
		try {
			DriverManager.registerDriver(singleton_);
		} catch (SQLException e) {
			singleton_ = null;
			e.printStackTrace();
		}

		dsCache_ = new HashMap<String, SQLMXDataSource>();
		//used to obtain 64 bit or 32bit JVM and load appropriately
		String dataModel = System.getProperty("sun.arch.data.model");
		if (dataModel !=null && dataModel.equals("64")){
			System.loadLibrary("jdbcT2");	
		}else {
			System.loadLibrary("jdbcT2");
		}

		// Verify that the JDBC Java libaray is compatible with the JNI library
		checkLibraryVersion(DriverInfo.driverVproc);
		
		// Initialize Java objects, methods references into gJNICache
		SQLMXInitialize(locale_.getLanguage());
		
    	// Get the major and minor database version numbers that
		// were setup in SQLMXInitialize()
		databaseMajorVersion_ = getDatabaseMajorVersionJNI();
		databaseMinorVersion_ = getDatabaseMinorVersionJNI();
		pid_ = getPid();


	}


	public Logger getParentLogger() throws SQLFeatureNotSupportedException {
		// TODO Auto-generated method stub
		return null;
	}


}
