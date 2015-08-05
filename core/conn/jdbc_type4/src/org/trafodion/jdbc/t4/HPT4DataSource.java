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

import java.io.PrintWriter;
import java.sql.Connection;
import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.util.Properties;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;

import javax.naming.Context;
import javax.naming.NamingException;
import javax.naming.Reference;
import javax.naming.Referenceable;

/**
 * 
 * <p>
 * JDBC Type 4 Driver <code>DataSource</code> class.
 * </p>
 * <p>
 * Description: A <code>DataSource</code> object is a factory for Connection
 * objects. An object that implements the <code>DataSource</code> interface is
 * typically registered with a JNDI service provider. A JDBC driver that is
 * accessed through the <code>DataSource</code> API does not automatically
 * register itself with the <code>DriverManager</code> object.
 * </p>
 * 
 * <p>
 * The <code>HPT4DataSource</code> class can provide connection pooling and
 * statement pooling features.
 * </p>
 * 
 * <pre>
 * &lt;b&gt;Setting properties for the HPT4DataSource in the Type 4 driver&lt;/b&gt;
 *    HPT4DataSource ds = new HPT4DataSource();
 *   ds.setUrl(&quot;jdbc:t4jdbc://&lt;NDCS host&gt;:&lt;NDCS port&gt;/:&quot;);
 *   ds.setCatalog(&quot;your catalog&quot;);
 *   ds.setSchema(&quot;your schema&quot;);
 *   ds.setUser(&quot;safeguard user name&quot;);
 *   ds.setPassword(&quot;safeguard password&quot;);
 * 
 *   // Following are optional properties
 *   ds.setConnectionTimeout(&quot;timeout in seconds&quot;);
 *   ds.setT4LogFile(&quot;your log file location&quot;);
 *   ds.setT4LogLevel(&quot;SEVERE&quot;);
 *   ds.setServerDataSource(&quot;NDCS datasource name&quot;);
 * 
 *   // Properties relevant for Type 4 connection pooling.
 *   // Set ds.setMaxPoolSize(-1) to turn OFF connection pooling
 *   ds.setMaxPoolSize(&quot;number of connections required&quot;);
 *   ds.setMinPoolSize(&quot;number of connections required&quot;);
 * 
 *   // Properties relevant for Type 4 statement pooling.
 *   // Set ds.setMaxStatement(0) to turn statement pooling OFF
 *   // Statement pooling is enabled only when connection pooling is enabled.
 *   ds.setMaxStatements(&quot;number of statements to be pooled&quot;);
 * </pre>
 * 
 * <pre>
 * &lt;b&gt;Programmatically registering HPT4DataSource with JDNI&lt;/b&gt;
 * 	java.util.Hashtable env = new java.util.Hashtable();
 *      env.put(Context.INITIAL_CONTEXT_FACTORY, &quot;Factory class name here&quot;);
 *      javax.naming.Context ctx = new javax.naming.InitialContext(env);
 *      ctx.rebind(&quot;DataSource name here&quot;, ds);
 * </pre>
 * 
 * <pre>
 * &lt;b&gt;Application making Type4 connection using the DataSource from JDNI&lt;/b&gt;
 * 	java.util.Hashtable env = new java.util.Hashtable();
 *      env.put(Context.INITIAL_CONTEXT_FACTORY, &quot;Factory class name here&quot;);
 *      javax.naming.Context ctx = new javax.naming.InitialContext(env);
 *      DataSource ds = (DataSource)ctx.lookup(&quot;DataSource name here&quot;);
 *      java.sql.Connection con = ds.getConnection();
 * </pre>
 * 
 * <p>
 * Copyright: (C) Copyright 2004-2007 Hewlett-Packard Development Company, L.P.
 * </p>
 * 
 * @see T4Properties
 */
public class HPT4DataSource extends T4DSProperties implements javax.sql.DataSource, java.io.Serializable, Referenceable

{
	/**
	 * Attempts to establish an NDCS connection.
	 * 
	 * @return a connection to the NDCS server.
	 * @throws SQLException
	 *             if a database access error or NDCS error occurs.
	 * @see #getConnection(String, String)
	 */
	synchronized public Connection getConnection() throws SQLException {
		if (logger.isLoggable(Level.FINER)) {
			logger.entering("HPT4DataSource", "getConnection");
		}

		Connection conn;
		TrafT4Connection t4Conn;
		HPT4ConnectionPoolDataSource pds;

		if (getSQLException() != null) {
			throw HPT4Messages.createSQLException(null, getLocale(), "invalid_property", getSQLException());
		}

		if (getMaxPoolSize() == -1) {
			t4Conn = new TrafT4Connection(this, getT4Properties());
		} else {
			if (poolManager != null) {
				t4Conn = (TrafT4Connection) poolManager.getConnection();
			} else {

				pds = new HPT4ConnectionPoolDataSource(getProperties());
				poolManager = new HPT4PooledConnectionManager(pds, getT4LogLevel());
				t4Conn = (TrafT4Connection) poolManager.getConnection();
			}
		}

		t4Conn.setLogInfo(getT4LogLevel(), getLogWriter());
		conn = t4Conn;

		if (logger.isLoggable(Level.FINER)) {
			logger.exiting("HPT4DataSource", "getConnection", conn);
		}

		return conn;
	}

	/**
	 * Attempts to establish an NDCS connection.
	 * 
	 * @return a connection to the NDCS server.
	 * @param username
	 *            Safeguard user name
	 * @param password
	 *            Safeguard user password
	 * @throws SQLException
	 *             if a database access error or NDCS error occurs.
	 * @see #getConnection()
	 */
	synchronized public Connection getConnection(String username, String password) throws SQLException {
		if (logger.isLoggable(Level.FINER)) {
			logger.entering("HPT4DataSource", "getConnection", new Object[] { this, username });
		}

		Connection conn;

		setUser(username);
		setPassword(password);

		conn = getConnection();

		if (logger.isLoggable(Level.FINER)) {
			logger.exiting("HPT4DataSource", "getConnection", conn);
		}

		return conn;
	}

	/**
	 * @return Reference Object containing all the Type 4 property references.
	 * @throws NamingException
	 */
	public Reference getReference() throws NamingException {

		Reference ref = new Reference(this.getClass().getName(), "org.trafodion.jdbc.t4.HPT4DataSourceFactory", null);
		return addReferences(ref);
	}

	/**
	 * Sets the print writer for the current Type 4 data source.
	 * 
	 * @param out
	 *            java.io.PrintWriter for the current T4 connection.
	 * @throws SQLException
	 *             when error occurs.
	 * @see #getLogWriter()
	 * @see javax.sql.ConnectionPoolDataSource
	 */
	public void setLogWriter(PrintWriter out) throws SQLException {
		super.setLogWriter(out);
		if (t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null, out);
			t4Logger_.logp(Level.FINE, "HPT4DataSource", "setLogWriter",
					"Note, this constructor was called before the previous constructor", p);
		}
		if (getLogWriter() != null) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(null, out);
			lr.setParameters(p);
			lr.setSourceClassName("HPT4DataSource");
			lr.setSourceMethodName("setLogWriter");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			getLogWriter().println(temp);
		}
		if (poolManager != null) {
			poolManager.setLogWriter(getLogWriter());
		}
	}

	// Local methods
	void setPoolManager(Context nameCtx, String dataSourceName) throws Exception {
		if (t4Logger_.isLoggable(Level.FINER) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null, nameCtx, dataSourceName);
			t4Logger_.logp(Level.FINER, "HPT4DataSource", "setPoolManager", "", p);
		}
		Object pds;

		try {
			pds = nameCtx.lookup(dataSourceName);
			if (pds instanceof HPT4ConnectionPoolDataSource) {
				poolManager = new HPT4PooledConnectionManager((HPT4ConnectionPoolDataSource) pds, getT4LogLevel());
			}
		} catch (javax.naming.NameNotFoundException nnfe) {
		}
	}

	// --------------------------------------------------------
	void setupLogFileHandler() {
		try {
			if (getT4LogFile() == null) {
				setT4LogFile(getT4GlobalLogFile());
				setT4LogFileHandler(getT4GlobalLogFileHandler());
			} else {
				if (getT4LogFileHandler() == null) {
					String temp = getT4LogFile();
					FileHandler fh1 = new FileHandler(temp);

					Formatter ff1 = new T4LogFormatter();

					fh1.setFormatter(ff1);
					setT4LogFileHandler(fh1);
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	} // end setupLogFileHandler

	// --------------------------------------------------------

	/**
	 * Contructor for the <code>HPT4DataSource</code> object.
	 * 
	 * @see #HPT4DataSource(java.util.Properties)
	 */
	public HPT4DataSource() {
		super();
		if (getT4LogLevel() != Level.OFF) {
			setupLogFileHandler();
		}
		if (t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null);
			t4Logger_.logp(Level.FINE, "HPT4DataSource", "<init>",
					"Note, this constructor was called before the previous constructor", p);
		}
		try {
			if (getLogWriter() != null) {
				LogRecord lr = new LogRecord(Level.FINE, "");
				Object p[] = T4LoggingUtilities.makeParams(null);
				lr.setParameters(p);
				lr.setSourceClassName("HPT4DataSource");
				lr.setSourceMethodName("<init>");
				T4LogFormatter lf = new T4LogFormatter();
				String temp = lf.format(lr);
				getLogWriter().println(temp);
			}
		} catch (SQLException se) {
			// ignore
		}

	}

	/**
	 * Contructor for the <code>HPT4DataSource</code> object.
	 * 
	 * @param info
	 *            Contains all the Type 4 properties in a <code>name,
	 * value</code>
	 *            pair.
	 * @see #HPT4DataSource()
	 * @see java.util.Properties
	 */
	public HPT4DataSource(Properties info) {
		super(info);
		if (getT4LogLevel() != Level.OFF) {
			setupLogFileHandler();
		}
		if (t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null);
			t4Logger_.logp(Level.FINE, "HPT4DataSource", "<init>",
					"Note, this constructor was called before the previous constructor", p);
		}
		try {
			if (getLogWriter() != null) {
				LogRecord lr = new LogRecord(Level.FINE, "");
				Object p[] = T4LoggingUtilities.makeParams(null);
				lr.setParameters(p);
				lr.setSourceClassName("HPT4DataSource");
				lr.setSourceMethodName("<init>");
				T4LogFormatter lf = new T4LogFormatter();
				String temp = lf.format(lr);
				getLogWriter().println(temp);
			}
		} catch (SQLException se) {
			// ignore
		}
	}

	/**
	 * @deprecated
	 */
	public void setNameType(String nameType) {
	}

	/**
	 * @deprecated
	 */
	public String getNameType() {
		return null;
	}

	// fields
	HPT4PooledConnectionManager poolManager;

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
