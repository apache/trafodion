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
import java.sql.SQLFeatureNotSupportedException;
import java.util.Properties;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;

import javax.naming.NamingException;
import javax.naming.Reference;
import javax.naming.Referenceable;
import javax.naming.StringRefAddr;
import javax.sql.PooledConnection;

/**
 * 
 * <p>
 * JDBC Type 4 Driver <code>ConnectionPoolDataSource</code> class.
 * </p>
 * <p>
 * Description: A <code>ConnectionPoolDataSource</code> object is a factory
 * for <code>PooledConnection</code> objects. As the name indicates, this
 * object provides a <code>PooledConnection</code> for data sources to be used
 * by the application servers.
 * </p>
 * 
 * <p>
 * The <code>TrafT4ConnectionPoolDataSource</code> class should be used to
 * provide JDBC3.0 connection pooling features. The
 * <code>TrafT4ConnectionPoolDataSource</code> is used by the application
 * servers like WSAS to provide connection pooling features to the J2EE
 * applications. <code>TrafT4ConnectionPoolDataSource.getPooledConnection()</code>
 * returns the <code>javax.sql.PooledConnection object</code>.
 * </p>
 * 
 * 
 * Setting connection properties such as catalog, schema, timeouts, and so on
 * are done at the higher level objects such as DataSource or DriverManager.
 * 
 * <p>
 * Licensed to the Apache Software Foundation (ASF)
 * </p>
 * 
 * @see T4Properties
 * @see TrafT4DataSource
 */

public class TrafT4ConnectionPoolDataSource extends T4DSProperties implements javax.sql.ConnectionPoolDataSource,
		java.io.Serializable, Referenceable

{

	/**
	 * Attempts to establish a physical database connection that can be used as
	 * a pooled connection.
	 * 
	 * @return A <code>PooledConnection</code> object that is a physical
	 *         connection to the NDCS server that this
	 *         <code>TrafT4ConnectionPoolDataSource</code> object represents.
	 * @throws SQLException
	 *             If any NDCS error occurs.
	 */
	public PooledConnection getPooledConnection() throws SQLException {
		if (t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null);
			t4Logger_.logp(Level.FINE, "TrafT4ConnectionPoolDataSource", "getPooledConnection", "", p);
		}
		if ( t4Logger_.isLoggable(Level.FINE) && getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(null);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ConnectionPoolDataSource");
			lr.setSourceMethodName("getPooledConnection");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			getLogWriter().println(temp);
		}
		TrafT4PooledConnection connect;

		Properties l_props = super.getProperties();
		T4Properties l_t4props = new T4Properties(l_props);
		connect = new TrafT4PooledConnection(this, l_t4props);

		return connect;
	}

	/**
	 * Attempts to establish a physical database connection that can be used as
	 * a pooled connection.
	 * 
	 * @param username
	 *            Safeguard user name.
	 * @param password
	 *            Safeguard user password.
	 * @return A <code>PooledConnection</code> object that is a physical
	 *         connection to the NDCS server that this
	 *         <code>TrafT4ConnectionPoolDataSource</code> object represents.
	 * @throws SQLException
	 *             If any NDCS error occurs.
	 */
	public PooledConnection getPooledConnection(String username, String password) throws SQLException {
		if (t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null, username);
			t4Logger_.logp(Level.FINE, "TrafT4ConnectionPoolDataSource", "getPooledConnection", "", p);
		}
		if ( t4Logger_.isLoggable(Level.FINE) && getLogWriter() != null ) {
			LogRecord lr = new LogRecord(Level.FINE, "");
			Object p[] = T4LoggingUtilities.makeParams(null, username);
			lr.setParameters(p);
			lr.setSourceClassName("TrafT4ConnectionPoolDataSource");
			lr.setSourceMethodName("getPooledConnection");
			T4LogFormatter lf = new T4LogFormatter();
			String temp = lf.format(lr);
			getLogWriter().println(temp);
		}
		TrafT4PooledConnection connect;

		setUser(username);
		setPassword(password);
		return getPooledConnection();

	}

	/**
	 * Returns all the properties associated with this
	 * <code>ConnectionPoolDataSource</code>.
	 * 
	 * @return Reference Object containing all the Type 4 property references.
	 * @throws NamingException
	 */
	public Reference getReference() throws NamingException {
		if (t4Logger_ != null && t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null);
			t4Logger_.logp(Level.FINE, "TrafT4ConnectionPoolDataSource", "getReference", "", p);
		}
		try {
			if ( t4Logger_.isLoggable(Level.FINE) && getLogWriter() != null ) {
				LogRecord lr = new LogRecord(Level.FINE, "");
				Object p[] = T4LoggingUtilities.makeParams(null);
				lr.setParameters(p);
				lr.setSourceClassName("TrafT4ConnectionPoolDataSource");
				lr.setSourceMethodName("getReference");
				T4LogFormatter lf = new T4LogFormatter();
				String temp = lf.format(lr);
				getLogWriter().println(temp);
			}
		} catch (SQLException se) {
			// ignore
		}

		Reference ref = new Reference(this.getClass().getName(), "org.trafodion.jdbc.t4.TrafT4ConnectionPoolDataSourceFactory",
				null);
		ref = addReferences(ref);
		ref.add(new StringRefAddr("propertyCycle", Integer.toString(propertyCycle_)));
		return ref;

	}

	/**
	 * Sets the Property cycle property. This property is not supprted by the
	 * Type 4 driver. This property is ignored by the Type 4 driver.
	 * 
	 * @param propertyCycle
	 */
	public void setPropertyCycle(int propertyCycle) {
		if (t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null);
			t4Logger_.logp(Level.FINE, "TrafT4ConnectionPoolDataSource", "setPropertyCycle", "", p);
		}
		try {
			if ( t4Logger_.isLoggable(Level.FINE) && getLogWriter() != null ) {
				LogRecord lr = new LogRecord(Level.FINE, "");
				Object p[] = T4LoggingUtilities.makeParams(null, propertyCycle);
				lr.setParameters(p);
				lr.setSourceClassName("TrafT4ConnectionPoolDataSource");
				lr.setSourceMethodName("setPropertyCycle");
				T4LogFormatter lf = new T4LogFormatter();
				String temp = lf.format(lr);
				getLogWriter().println(temp);
			}
		} catch (SQLException se) {
			// ignore
		}
		propertyCycle_ = propertyCycle;
	}

	/**
	 * Returns the Property cycle property. This property is not supprted by the
	 * Type 4 driver. This property is ignored by the Type 4 driver.
	 * 
	 * @return propertyCycle
	 */
	public int getPropertyCycle() {
		if (t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null);
			t4Logger_.logp(Level.FINE, "TrafT4ConnectionPoolDataSource", "getPropertyCycle", "", p);
		}
		try {
			if ( t4Logger_.isLoggable(Level.FINE) && getLogWriter() != null ) {
				LogRecord lr = new LogRecord(Level.FINE, "");
				Object p[] = T4LoggingUtilities.makeParams(null);
				lr.setParameters(p);
				lr.setSourceClassName("TrafT4ConnectionPoolDataSource");
				lr.setSourceMethodName("getPropertyCycle");
				T4LogFormatter lf = new T4LogFormatter();
				String temp = lf.format(lr);
				getLogWriter().println(temp);
			}
		} catch (SQLException se) {
			// ignore
		}
		return propertyCycle_;
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
	 * Creates a pooled connection object.
	 * 
	 * @see #TrafT4ConnectionPoolDataSource(Properties)
	 * @see T4Properties
	 */
	public TrafT4ConnectionPoolDataSource() {
		super();
		if (getT4LogLevel() != Level.OFF)
			setupLogFileHandler();
		if (t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null);
			t4Logger_.logp(Level.FINE, "TrafT4ConnectionPoolDataSource", "TrafT4ConnectionPoolDataSource",
					"Note, super called before this.", p);
		}
		try {
			if ( t4Logger_.isLoggable(Level.FINE) && getLogWriter() != null ) {
				LogRecord lr = new LogRecord(Level.FINE, "");
				Object p[] = T4LoggingUtilities.makeParams(null);
				lr.setParameters(p);
				lr.setSourceClassName("TrafT4ConnectionPoolDataSource");
				lr.setSourceMethodName("");
				T4LogFormatter lf = new T4LogFormatter();
				String temp = lf.format(lr);
				getLogWriter().println(temp);
			}
		} catch (SQLException se) {
			// ignore
		}
	}

	/**
	 * Creates a pooled connection object with the properties specified.
	 * 
	 * @param props
	 *            properties for the Type 4 connection
	 * @see #TrafT4ConnectionPoolDataSource()
	 * @link T4Properties
	 */
	public TrafT4ConnectionPoolDataSource(Properties props) {
		super(props);
		if (getT4LogLevel() != Level.OFF)
			setupLogFileHandler();
		if (t4Logger_.isLoggable(Level.FINE) == true) {
			Object p[] = T4LoggingUtilities.makeParams(null, props);
			t4Logger_.logp(Level.FINE, "TrafT4ConnectionPoolDataSource", "TrafT4ConnectionPoolDataSource",
					"Note, super called before this.", p);
		}
		try {
			if ( t4Logger_.isLoggable(Level.FINE) && getLogWriter() != null ) {
				LogRecord lr = new LogRecord(Level.FINE, "");
				Object p[] = T4LoggingUtilities.makeParams(null, props);
				lr.setParameters(p);
				lr.setSourceClassName("TrafT4ConnectionPoolDataSource");
				lr.setSourceMethodName("");
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

	// Standard ConnectionPoolDataSource Properties
	int propertyCycle_;

	public Logger getParentLogger() throws SQLFeatureNotSupportedException {
		// TODO Auto-generated method stub
		return null;
	}
}
