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
 * Filename	: SQLMXConnectionPoolDataSource.java
 * Description :
 */

package org.apache.trafodion.jdbc.t2;

import java.io.PrintWriter;
import java.sql.*;

import javax.sql.*;
import java.util.Properties;
import javax.naming.Referenceable;
import javax.naming.NamingException;
import javax.naming.Reference;
import javax.naming.StringRefAddr;
import java.util.Locale;
import java.lang.ref.*;
import java.util.HashMap;
import java.util.logging.Logger;

/**
 * A factory for <code>PooledConnection</code> objects. An object that
 * implements this interface will typically be registered with a naming service
 * that is based on the Java<sup><font size=-2>TM</font></sup> Naming and
 * Directory Interface (JNDI).
 *
 * @version HP JDBC/MX
 */

public class SQLMXConnectionPoolDataSource extends T2Properties implements
		javax.sql.ConnectionPoolDataSource, java.io.Serializable, Referenceable

{

	/**
	 * Attempts to establish a physical database connection that can be used as
	 * a pooled connection.
	 *
	 * @return a <code>PooledConnection</code> object that is a physical
	 *         connection to the database that this
	 *         <code>SQLMXConnectionPoolDataSource</code> object represents
	 * @exception SQLException
	 *                if a database access error occurs
	 */
	public PooledConnection getPooledConnection() throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getPooledConnection]
					.methodEntry(JdbcDebug.debugLevelPooling);
		try {
			SQLMXPooledConnection connect;

//			connect = new SQLMXPooledConnection(this, getDataSourceProperties());
			connect = new SQLMXPooledConnection(this, getT2Properties());
			if (JdbcDebugCfg.traceActive)
				debug[methodId_getPooledConnection].methodReturn(connect
						.toString());
			return connect;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getPooledConnection].methodExit();
		}
	}

	/**
	 * Attempts to establish a physical database connection that can be used as
	 * a pooled connection.
	 *
	 * @param user
	 *            the database user on whose behalf the connection is being made
	 * @param password
	 *            the user's password
	 * @return a <code>PooledConnection</code> object that is a physical
	 *         connection to the database that this
	 *         <code>SQLMXConnectionPoolDataSource</code> object represents
	 * @exception SQLException
	 *                if a database access error occurs
	 */
	public PooledConnection getPooledConnection(String username, String password)
			throws SQLException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getPooledConnection_LL]
					.methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive)
			debug[methodId_getPooledConnection_LL].methodParameters("username="
					+ username + ", password=" + password);
		try {
			Properties info;
			SQLMXPooledConnection connect;

			info = getDataSourceProperties();
			info.setProperty("user", username);
			info.setProperty("password", password);
			return getPooledConnection();
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getPooledConnection_LL].methodExit();
		}
	}

		/**
	 * Returns a javax.naming.Reference to this object.
	 *
	 * @return a <code>javax.naming.Reference</code> to this object
	 * @exception javax.naming.NamingException
	 *                if the Object Factory for this data source cannot be
	 *                found.
	 */
	public Reference getReference() throws NamingException {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getReference]
					.methodEntry(JdbcDebug.debugLevelPooling);
		try {
			Reference ref;

			ref = new Reference(this.getClass().getName(),
					"org.apache.trafodion.jdbc.t2.SQLMXConnectionPoolDataSourceFactory",
					null);
			ref = super.addReferences(ref);

			return ref;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getReference].methodExit();
		}
	}


	/**
	 * The interval in seconds that the pool should wait before enforcing the
	 * current policy defined by the values of the connection pool properties
	 * for <code>maxStatements</code>, <code>initialPoolSize</code>,
	 * <code>maxPoolSize</code>, and <code>maxIdleTime</code>.
	 *
	 * @param propertyCycleInterval
	 *            the interval in seconds that the pool should wait before
	 *            enforcing the current policy defined by the values of the
	 *            connection pool properties.
	 * @see #getPropertyCycle
	 */
	public void setPropertyCycle(int propertyCycle) {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_setPropertyCycle]
					.methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive)
			debug[methodId_setPropertyCycle].methodParameters("propertyCycle="
					+ propertyCycle);
		try {
			propertyCycle_ = propertyCycle;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_setPropertyCycle].methodExit();
		}
	}

	/**
	 * Returns the interval in seconds that the pool should wait before
	 * enforcing the current policy defined by the values of the connection pool
	 * properties for <code>maxStatements</code>, <code>initialPoolSize</code>,
	 * <code>maxPoolSize</code>, and <code>maxIdleTime</code>.
	 *
	 * @return the interval in seconds that the pool should wait before
	 *         enforcing the current policy defined by the values of the
	 *         connection pool properties.
	 * @see #setPropertyCycle
	 */
	public int getPropertyCycle() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getPropertyCycle]
					.methodEntry(JdbcDebug.debugLevelPooling);
		try {
			if (JdbcDebugCfg.traceActive)
				debug[methodId_getPropertyCycle].methodReturn("propertyCycle_="
						+ propertyCycle_);
			return propertyCycle_;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getPropertyCycle].methodExit();
		}
	}

	// Local methods
	Properties getDataSourceProperties() {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getDataSourceProperties]
					.methodEntry(JdbcDebug.debugLevelPooling);
		try {
			Properties info;

			info = getProperties();

			return info;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getDataSourceProperties].methodExit();
		}
	}

	public SQLMXConnectionPoolDataSource() {
		if (JdbcDebugCfg.entryActive) {
			debug[methodId_SQLMXConnectionPoolDataSource]
					.methodEntry(JdbcDebug.debugLevelPooling);
			debug[methodId_SQLMXConnectionPoolDataSource].methodExit();
		}
	}

	public SQLMXConnectionPoolDataSource(Properties props) {
	super(props);
	}
	// fields

	//PrintWriter logWriter_;
	int propertyCycle_;
	// serialVersionUID set to resolve JDK5.0 javax -Xlint:serial definition
	// warning.
	private static final long serialVersionUID = 3L;


	private static int methodId_getPooledConnection = 0;
	private static int methodId_getPooledConnection_LL = 1;
	private static int methodId_getLogWriter = 2;
	private static int methodId_getPropertyCycle = 3;
	private static int methodId_setLogWriter = 4;
	private static int methodId_setPropertyCycle = 5;
	private static int methodId_getReference = 6;
	private static int methodId_SQLMXConnectionPoolDataSource = 7;
	private static int methodId_getDataSourceProperties = 8;
	
	private static int totalMethodIds =9;
	private static JdbcDebug[] debug;

	static {
		String className = "SQLMXConnectionPoolDataSource";
		if (JdbcDebugCfg.entryActive) {
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_getPooledConnection] = new JdbcDebug(className,
					"getPooledConnection");
			debug[methodId_getPooledConnection_LL] = new JdbcDebug(className,
					"getPooledConnection_LL");		
			debug[methodId_getLogWriter] = new JdbcDebug(className,
					"getLogWriter");
			debug[methodId_setLogWriter] = new JdbcDebug(className,
					"setLogWriter");
			debug[methodId_getReference] = new JdbcDebug(className,
					"getReference");
			debug[methodId_setPropertyCycle] = new JdbcDebug(className,
					"setPropertyCycle");
			debug[methodId_getPropertyCycle] = new JdbcDebug(className,
					"getPropertyCycle");
			debug[methodId_getDataSourceProperties] = new JdbcDebug(className,
					"getDataSourceProperties");
			debug[methodId_SQLMXConnectionPoolDataSource] = new JdbcDebug(
					className, "SQLMXConnectionPoolDataSource");
		}

	}
	public Logger getParentLogger() throws SQLFeatureNotSupportedException {
		// TODO Auto-generated method stub
		return null;
	}
}
