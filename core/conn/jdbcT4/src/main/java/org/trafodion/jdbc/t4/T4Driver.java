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

import java.sql.DriverManager;
import java.sql.DriverPropertyInfo;
import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;

/**
 * <p>
 * JDBC Type 4 Driver implementation.
 * </p>
 * <p>
 * Description: <code>T4Driver</code> is an implementation of the
 * <code>java.sql.Driver</code> interface. The Java SQL framework allows for
 * multiple database drivers to be loaded in single java program. The
 * <code>T4Driver</code> can be loaded programatically by
 * <code>Class.forName("org.trafodion.jdbc.t4.T4Driver") </code> or by passing
 * <code>-Djdbc.drivers=org.trafodion.jdbc.t4.T4Driver</code> in the command line of
 * the Java program.
 * </p>
 * <p>
 * Licensed to the Apache Software Foundation (ASF)
 * </p>
 * 
 * @see java.sql.DriverManager
 * @see java.sql.Connection
 */
public class T4Driver extends T4Properties implements java.sql.Driver {
	/**
	 * Retrieves whether the Type 4 driver determined that it can open a
	 * connection to the given URL. Typically drivers return true if they
	 * recognize the subprotocol specified in the URL and false if they do not.
	 * For Type 4 driver to recognize the protocol, the URL must start with
	 * <code>jdbc:t4jdbc</code>.
	 * 
	 * @param url
	 *            The URL of the database.
	 * @return true if the Type 4 driver recognizes the given URL; otherwise,
	 *         false.
	 */
	public boolean acceptsURL(String url) throws SQLException {
		if (t4GlobalLogger.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null,filterOutPWD(url));
			t4GlobalLogger.logp(Level.FINE, "T4Driver", "acceptsURL", "", p);
		}
		if ( t4GlobalLogger.isLoggable(Level.FINE) && getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(null, filterOutPWD(url));
			lr.setParameters(p);
			lr.setSourceClassName("T4Driver");
			lr.setSourceMethodName("acceptsURL");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			getLogWriter().println(temp);
		}

		return T4Address.acceptsURL(url);
	}

	/**
	 * Attempts to make an NDCS connection to the given URL. The Type 4 driver
	 * returns "null" when it determines that the driver is the wrong kind of
	 * driver to connect to the given URL. This occurence is common; for
	 * example, when the JDBC driver manager is requested to connect to a given
	 * URL, the driver manager it passes the URL to each loaded driver in turn.
	 * The Type 4 driver throws an SQLException when it has trouble connecting
	 * to NDCS. You can use the <code>java.util.Properties</code> argument to
	 * pass arbitrary string name-value pairs as connection arguments.
	 * Typically, you should include "user" and "password" properties in the
	 * Properties object.
	 * 
	 * @param url
	 *            The URL string of format
	 *            <code>jdbc:t4jdbc://host:port/:</code>
	 * @param info
	 *            <code>java.util.Properties</code> object containing
	 *            name-value pair.
	 * @return The <code>TrafT4Connection</code> object.
	 * @throws SQLException
	 *             When an error occurs connecting to NDCS.
	 * @see T4Properties
	 */
	public java.sql.Connection connect(String url, Properties info) throws SQLException {
		Properties tempinfo = null;
		if (t4GlobalLogger.isLoggable(Level.FINE) == true) {
			tempinfo = new Properties();
			tempinfo.putAll(info);
			tempinfo.remove("password");
			Object p[] = T4LoggingUtilities.makeParams(null, filterOutPWD(url), tempinfo);
			t4GlobalLogger.logp(Level.FINE, "T4Driver", "connect", "", p);
		}
		if ( t4GlobalLogger.isLoggable(Level.FINE) && getLogWriter() != null ) {
			if (tempinfo == null) {
				tempinfo = new Properties();
				tempinfo.putAll(info);
				tempinfo.remove("password");
			}
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(null, filterOutPWD(url), tempinfo);
			lr.setParameters(p);
			lr.setSourceClassName("T4Driver");
			lr.setSourceMethodName("connect");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			getLogWriter().println(temp);
		}

		if (logger.isLoggable(Level.INFO)) {
			logger.entering("TrafT4DataSource", "getConnection");
		}

		String key = null;
		TrafT4DataSource ds = null;

		if (acceptsURL(url)) {
			 synchronized(this) {
				// properties in the info take precedence.
				// Note, setURL also assigns the T4Properties that are on the URL
				Properties urlProps = setURL(url);
	
				//
				// Merge any property that is in the url but not in info.
				//
				if (urlProps != null && info != null) {
					Enumeration en1 = urlProps.propertyNames();
					String key1 = null;
	
					while (en1 != null && en1.hasMoreElements() == true) {
						key1 = (String) en1.nextElement();
						if (info.getProperty(key1) == null) {
							info.setProperty(key1, urlProps.getProperty(key1));
						}
					}
				}
	
				// If info is false, but there were properties on the URL,
				// the URL properties were already set when we called setURL.
				if (info != null) {
					initialize(info);
					if (getSQLException() != null) {
						throw TrafT4Messages.createSQLException(null, getLocale(), "invalid_property", getSQLException());
					}
				}
				if (getMaxPoolSize() != -1) {
					key = getUrl() + getCatalog() + getSchema() + getUser() + getPassword() + getServerDataSource();
	
					ds = (TrafT4DataSource) dsCache_.get(key);
	
					if (ds == null) {
						ds = new TrafT4DataSource(getProperties());
						dsCache_.put(key, ds);
					}
				} else {
					ds = new TrafT4DataSource(getProperties());
				}
			 }
			 
			return ds.getConnection(ds.getUser(), ds.getPassword());
		} else {
			return null;
		}
	}

	private String filterOutPWD(String url) {
		return url.replaceAll("password\\s*\\=\\s*[^;]*", "");
	}

	/**
	 * Retrieves the driver's major JDBC version number. For the Type 4 Driver,
	 * the number should be 3.
	 * 
	 * @return 3.
	 */
	public int getMajorVersion() {
		if (t4GlobalLogger.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null);
			t4GlobalLogger.logp(Level.FINE, "T4Driver", "getMajorVersion", "", p);
		}
		try {
			if ( t4GlobalLogger.isLoggable(Level.FINE) && getLogWriter() != null ) {
				LogRecord lr = new LogRecord(Level.FINE, "");
				Object p[] = T4LoggingUtilities.makeParams(null);
				lr.setParameters(p);
				lr.setSourceClassName("T4Driver");
				lr.setSourceMethodName("getMajorVersion");
				T4LogFormatter lf = new T4LogFormatter();
				String temp = lf.format(lr);
				getLogWriter().println(temp);
			}
		} catch (SQLException se) {
			// ignore
		}

		return 3;
	}

	/**
	 * Gets the Type 4 driver's minor version number. For the Type 4 driver, the
	 * number should be 11.
	 * 
	 * @return 11
	 */
	public int getMinorVersion() {
		if (t4GlobalLogger.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null);
			t4GlobalLogger.logp(Level.FINE, "T4Driver", "getMinorVersion", "", p);
		}
		try {
			if ( t4GlobalLogger.isLoggable(Level.FINE) && getLogWriter() != null ) {
				LogRecord lr = new LogRecord(Level.FINE, "");
				Object p[] = T4LoggingUtilities.makeParams(null);
				lr.setParameters(p);
				lr.setSourceClassName("T4Driver");
				lr.setSourceMethodName("getMinorVersion");
				T4LogFormatter lf = new T4LogFormatter();
				String temp = lf.format(lr);
				getLogWriter().println(temp);
			}
		} catch (SQLException se) {
			// ignore
		}

		return 11;
	}

	/**
	 * Gets information about the possible properties for the Type 4 driver. The
	 * <code>getPropertyInfo</code> method is intended to allow a generic GUI
	 * tool to determine the properties that the tool should prompt from a human
	 * to get enough information to connect to NDCS. Depending on the values the
	 * human has supplied so far, additional values might be necessary, so you
	 * might need to iterate though several calls to the
	 * <code>getPropertyInfo</code> method.
	 * 
	 * @param url
	 *            The URL of the database to which to connect
	 * @param info
	 *            <code>java.util.Properties</code> object containing
	 *            name-value pairs. The Type 4 driver ignores the Properties
	 *            parameter passed to the driver.
	 * @return array of <code>DriverPropertyInfo</code> containing property
	 *         details.
	 */

	public DriverPropertyInfo[] getPropertyInfo(String url, Properties info) throws SQLException {
		Properties tempinfo = null;
		if (t4GlobalLogger.isLoggable(Level.FINE) == true) {
			tempinfo = new Properties();
			tempinfo.putAll(info);
			tempinfo.remove("password");
			Object p[] = T4LoggingUtilities.makeParams(null, filterOutPWD(url), tempinfo);
			t4GlobalLogger.logp(Level.FINE, "T4Driver", "getPropertyInfo", "", p);
		}
		if ( t4GlobalLogger.isLoggable(Level.FINE) && getLogWriter() != null ) {
			if(tempinfo == null){
				tempinfo = new Properties();
				tempinfo.putAll(info);
				tempinfo.remove("password");
			}
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(null, filterOutPWD(url), tempinfo);
			lr.setParameters(p);
			lr.setSourceClassName("T4Driver");
			lr.setSourceMethodName("getPropertyInfo");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			getLogWriter().println(temp);
		}
		if (acceptsURL(url)) {
			return super.getPropertyInfo(url, info);
		} else {
			return null;
		}
	}

	/**
	 * Returns whether the Type 4 driver is JDBC compliant.
	 * 
	 * @return true
	 */
	public boolean jdbcCompliant() {
		if (t4GlobalLogger.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null);
			t4GlobalLogger.logp(Level.FINE, "T4Driver", "jdbcCompliant", "", p);
		}
		try {
			if ( t4GlobalLogger.isLoggable(Level.FINE) && getLogWriter() != null ) {
				LogRecord lr = new LogRecord(Level.FINE, "");
				Object p[] = T4LoggingUtilities.makeParams(null);
				lr.setParameters(p);
				lr.setSourceClassName("T4Driver");
				lr.setSourceMethodName("jdbcCompliant");
				T4LogFormatter lf = new T4LogFormatter();
				String temp = lf.format(lr);
				getLogWriter().println(temp);
			}
		} catch (SQLException se) {
			// ignore
		}

		return true;
	};

	// Fields
	private static T4Driver singleton_;
	static Hashtable dsCache_;

	/**
	 * Instantiated by either <code>
	 * Class.forName("org.trafodion.jdbc.t4.T4Driver")</code>
	 * or by passing <code>-Djdbc.drivers=org.trafodion.jdbc.t4.T4Driver</code>
	 * property in the command line of the JDBC program.
	 */
	public T4Driver() {
		super();
		if (logger.isLoggable(Level.INFO)) {
			logger.entering("TrafT4DataSource", "getConnection");
		}
	}

	// initializer to register the Driver with the Driver manager
	static {
		// Register the Driver with the Driver Manager
		try {
			singleton_ = new T4Driver();
			DriverManager.registerDriver(singleton_);
		} catch (SQLException e) {
			singleton_ = null;
			e.printStackTrace();
		}

		dsCache_ = new Hashtable();
	}

	public Logger getParentLogger() throws SQLFeatureNotSupportedException {
		// TODO Auto-generated method stub
		return null;
	}
}
