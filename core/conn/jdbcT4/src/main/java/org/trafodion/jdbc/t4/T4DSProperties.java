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
import java.util.Locale;
import java.util.Properties;
import java.util.logging.Level;

/**
 * <p>
 * JDBC Type 4 driver data source connetion properties class.
 * </p>
 * <p>
 * Description: The <code>T4DSProperties</code> class contains all the
 * properties associated with Type 4 data source connection.
 * <code>T4DSProperties</code> is inherited by the <code>TrafT4DataSource,
 * TrafT4ConnectionPooledDataSource</code>
 * classes for configuring Type 4 connection properties.
 * <p>
 * The properties passed to the Type 4 driver have this precedence order in
 * event of the values set through more than one option:
 * </p>
 * <blockquote>
 * <p>
 * 1. java.util.properties parameter in the
 * <code>DriverManager.getConnection</code> call or through
 * <code>DataSource.setXXX()</code> call.
 * </p>
 * <p>
 * 2. <code>java.util.properties</code> file properties set through
 * <code>-Dt4jdbc.properties</code> option.
 * </p>
 * <p>
 * 3. Command line properties using -D option. All the system properties passed
 * through the command-line option have to be prefixed with
 * <code>t4jdbc</code>, to distinguish JDBC Type 4 driver properties
 * from other system properties. For example: property <code>user</code> when
 * specified with -D has to be qualified as
 * <code>-Dt4jdbc.user=super.super</code>.
 * </p>
 * </blockquote>
 * <p>
 * Copyright: (C) Apache Software Foundation (ASF)
 * </p>
 * 
 */
public class T4DSProperties extends T4Properties {

	public T4DSProperties() {
		super();
	}

	public T4DSProperties(Properties props) {
		super(props);
	}

	/**
	 * Sets the description for the current Type 4 connection.
	 * 
	 * @param description
	 *            For the current Type 4 connection.
	 * @see #getDescription()
	 */
	public void setDescription(String description) {
		super.setDescription(description);
	}

	/**
	 * Returns the description associated with the current Type 4 connection.
	 * 
	 * @return The description associated with the current Type 4 connection.
	 * @see #setDescription(String)
	 */
	public String getDescription() {
		return super.getDescription();
	}

	/**
	 * Sets the data source name for the current Type 4 connection.
	 * 
	 * @param dataSourceName
	 *            For the client side <code>DataSource</code> object.
	 * @see #getDataSourceName()
	 */
	public void setDataSourceName(String dataSourceName) {
		super.setDataSourceName(dataSourceName);
	}

	/**
	 * Returns the client's data source name.
	 * 
	 * @return data source name.
	 * @see #setDataSourceName(String)
	 */
	public String getDataSourceName() {
		return super.getDataSourceName();
	}

	/**
	 * Sets the NDCS server data source name. The NDCS server data source is
	 * defined by NDCS on the server.
	 * 
	 * @param serverDataSource
	 *            the NDCS data source name to use on the NDCS server side. The
	 *            default value is an empty string.
	 * @see #getServerDataSourceName()
	 */
	public void setServerDataSource(String serverDataSource) {
		super.setServerDataSource(serverDataSource);
	}

	/**
	 * Returns the NDCS server-side data source name used for the current Type 4
	 * connection.
	 * 
	 * @return NDCS server-side data source name.
	 * @see #setServerDataSource(String)
	 */
	public String getServerDataSource() {
		return super.getServerDataSource();
	}

	/**
	 * Sets the default catalog that will be used to access SQL objects
	 * referenced in SQL statements if the SQL objects are not fully qualified.
	 * 
	 * @param catalog
	 *            Database catalog name. The default catalog name is
	 *            set by the DCS server-side data source.
	 * @see #getCatalog()
	 */
	public void setCatalog(String catalog) {
		super.setCatalog(catalog);
	}

	/**
	 * Gets the default catalog that will be used to access SQL objects
	 * referenced in SQL statements if the SQL objects are not fully qualified.
	 * 
	 * @return TrafT4 catalog name.
	 * @see #setCatalog(String)
	 */
	public String getCatalog() {
		return super.getCatalog();
	}

	/**
	 * Sets the default schema that will be used to access SQL objects
	 * referenced in SQL statements if the SQL objects are not fully qualified.
	 * 
	 * @param schema
	 *            Database schema name. The default schema name is
	 *            set by the DCS server side data source.
	 * @see #getSchema()
	 */
	public void setSchema(String schema) {
		super.setSchema(schema);
	}

	/**
	 * Gets the default schema that will be used to access SQL objects
	 * referenced in SQL statements if the SQL objects are not fully qualified.
	 * 
	 * @return The schema associated with the current Type 4 connection.
	 * @see #setSchema(String)
	 */
	public String getSchema() {
		return super.getSchema();
	}

	/**
	 * Configure the Type 4 connection to continue batch processing of next
	 * commands even after errors. Default value is true.
	 * 
	 * @param batchRecovery
	 */
	void setBatchRecovery(String batchRecovery) {
		boolean boolBatchRecovery = true;
		if (batchRecovery != null) {
			if (batchRecovery.equalsIgnoreCase("false")) {
				boolBatchRecovery = false;
			}
		}
		setBatchRecovery(boolBatchRecovery);
	}

	/**
	 * Configure the Type 4 connection to continue batch processing of next
	 * commands even after errors. Default value is true.
	 * 
	 * @param batchRecovery
	 */
	void setBatchRecovery(boolean batchRecovery) {
		super.setBatchRecovery(batchRecovery);
	}

	/**
	 * Return whether the Type 4 connection is configured to continue batch
	 * processing of next commands even after errors.
	 * 
	 * @return batchRecovery
	 */
	boolean getBatchRecovery() {
		return super.getBatchRecovery();
	}

	/**
	 * Returns the <code>java.util.Locale</code> object associated with the
	 * current Type 4 connection.
	 * 
	 * @return <code>java.util.Locale</code> object.
	 */
	public Locale getLocale() {
		return super.getLocale();
	}

	/**
	 * The maximum number of physical connections that the pool (free and
	 * in-use) should contain. When the maximum number of physical connections
	 * is reached, the Type 4 driver throws an <code>SQLException
	 * </code> with
	 * the message "Maximum pool size reached." Specifying a value of 0 (zero)
	 * indicates there is no maximum size for the pool. Specifying a value of -1
	 * indicates no connection pooling is performed. The default value is -1,
	 * indicating that no pooling of physical connections is done.
	 * 
	 * @param maxPoolSize
	 *            the maximum number of physical connections the pool should
	 *            contain (free and in use).
	 * @see #getMaxPoolSize()
	 * @see #setMaxPoolSize(int)
	 */
	public void setMaxPoolSize(String maxPoolSize) {
		super.setMaxPoolSize(maxPoolSize);
	}

	/**
	 * The maximum number of physical connections that the pool (free and
	 * in-use) should contain. When the maximum number of physical connections
	 * is reached, the Type 4 driver throws an <code>SQLException
	 * </code> with
	 * the message "Maximum pool size reached." Specifying a value of 0 (zero)
	 * indicates there is no maximum size for the pool. Specifying a value of -1
	 * indicates no connection pooling is performed. The default value is -1,
	 * indicating that no pooling of physical connections is done.
	 * 
	 * @param maxPoolSize
	 *            the maximum number of physical connections the pool should
	 *            contain (free and in use).
	 * @see #getMaxPoolSize()
	 * @see #setMaxPoolSize(String)
	 */

	public void setMaxPoolSize(int maxPoolSize) {
		super.setMaxPoolSize(maxPoolSize);
	}

	/**
	 * Returns the maximum number of physical connections that the pool (free
	 * and inuse) should contain. A value of zero (0) indicates no maximum size.
	 * A value of -1 indicates that connection pooling is not being done.
	 * 
	 * @return the maximum number of physical connections that the pool should
	 *         contain.
	 * @see #setMaxPoolSize(int)
	 */
	public int getMaxPoolSize() {
		return super.getMaxPoolSize();
	}

	/**
	 * Sets the number of physical connections the pool should keep available at
	 * all times.
	 * 
	 * @param minPoolSize
	 *            Limits the number of physical connection that can be in the
	 *            free pool. When the number of physical connections in the free
	 *            pool reaches the value of minPoolSize, subsequent connections
	 *            that are closed are physically closed and are not added to the
	 *            free pool. Specifying a value of 0 means that the value of
	 *            minPoolSize is the same as the value of maxPoolSize. If the
	 *            value of maxPoolSize is -1, the value of minPoolSize is
	 *            ignored. The default value is 0. For this data source, it is
	 *            recommended that you use the default value.
	 * @see #getMinPoolSize()
	 * @see #setMinPoolSize(int minPoolSize)
	 */
	public void setMinPoolSize(String minPoolSize) {
		super.setMinPoolSize(minPoolSize);
	}

	/**
	 * Sets the number of physical connections the pool should keep available at
	 * all times.
	 * 
	 * @param minPoolSize
	 *            Limits the number of physical connection that can be in the
	 *            free pool. When the number of physical connections in the free
	 *            pool reaches the value of minPoolSize, subsequent connections
	 *            that are closed are physically closed and are not added to the
	 *            free pool. Specifying a value of 0 means that the value of
	 *            minPoolSize is the same as the value of maxPoolSize. If the
	 *            value of maxPoolSize is -1, the value of minPoolSize is
	 *            ignored. The default value is 0. For this data source, it is
	 *            recommended that you use the default value.
	 * @see #getMinPoolSize()
	 * @see #setMinPoolSize(String minPoolSize)
	 */
	public void setMinPoolSize(int minPoolSize) {
		super.setMinPoolSize(minPoolSize);
	}

	/**
	 * Returns the number of physical connections the pool should keep in the
	 * free pool. A value of 0 (zero) indicates that minPoolSize is equal to
	 * maxPoolsize. If maxPoolsize is equal to -1, the value of minPoolSize is
	 * ignored. The default value is 0.
	 * 
	 * @return The number of physical connections the pool should maintain in
	 *         the free pool.
	 * @see #setMinPoolSize(int)
	 * @see #setMaxPoolSize(int)
	 */
	public int getMinPoolSize() {
		return super.getMinPoolSize();
	}

	/**
	 * The initial number of physical connections that the pool should be
	 * created with. Specifying a value of 0 (zero) or less indicates that the
	 * pool should not be created with any initial connections. The default
	 * value is -1 indicating that no initial pool of physical connections is
	 * created. The value can be less than minPoolSize but must be less than or
	 * equal to the value of maxPoolSize. Specifying a value greater than
	 * maxPoolSize will set the initialPoolSize to the value of maxPoolSize.
	 * 
	 * @param initialPoolSize
	 *            the initial number of physical connections the pool should be
	 *            created with.
	 * @see #setInitialPoolSize(int)
	 */
	public void setInitialPoolSize(String initialPoolSize) {
		super.setInitialPoolSize(initialPoolSize);
	}

	/**
	 * The initial number of physical connections that the pool should be
	 * created with. Specifying a value of 0 (zero) or less indicates that the
	 * pool should not be created with any initial connections. The default
	 * value is -1 indicating that no initial pool of physical connections is
	 * created. The value can be less than minPoolSize but must be less than or
	 * equal to the value of maxPoolSize. Specifying a value greater than
	 * maxPoolSize will set the initialPoolSize to the value of maxPoolSize.
	 * 
	 * @param initialPoolSize
	 *            the initial number of physical connections the pool should be
	 *            created with.
	 * @see #setInitialPoolSize(String)
	 */
	public void setInitialPoolSize(int initialPoolSize) {
		super.setInitialPoolSize(initialPoolSize);
	}

	/**
	 * Returns the number of physical connections that the pool should be
	 * created with. A value of -1 indicates that the pool is not created with
	 * any initial connections.
	 * 
	 * @return initialPoolSize the number of physical connections that the pool
	 *         should be created with.
	 * @see #setInitialPoolSize(int)
	 */
	public int getInitialPoolSize() {
		return super.getInitialPoolSize();
	}

	/**
	 * Total number of statements that can be cached. A value of zero (0)
	 * indicates that caching of statements is disabled.
	 * 
	 * @param maxStatements
	 *            The number of statements that can be cached.
	 * @see #setMaxStatements(int)
	 */
	public void setMaxStatements(String maxStatements) {
		super.setMaxStatements(maxStatements);
	}

	/**
	 * Total number of statements that can be cached. A value of zero (0)
	 * indicates that caching of statements is disabled.
	 * 
	 * @param maxStatements
	 *            The number of statements that can be cached.
	 * @see #setMaxStatements(String)
	 */
	public void setMaxStatements(int maxStatements) {
		super.setMaxStatements(maxStatements);
	}

	/**
	 * Returns the total number of statements that can be cached. A value of
	 * zero (0) indicates that pooling of statements is disabled.
	 * 
	 * @return The total number of statements that can be cached.
	 */
	public int getMaxStatements() {
		return super.getMaxStatements();
	}

	/**
	 * Returns the URL used in the current Type 4 connection. JDBC Type 4
	 * driver URL uses the format: <code>jdbc:t4jdbc://host:port/:</code>
	 * 
	 * @deprecated Use the <code>getUrl()</code> to obtain the URL string.
	 * @return the URL string.
	 * @see #getUrl()
	 */
	public String getURL() {
		return getURL();
	}

	/**
	 * Sets the URL for the Type 4 connection. JDBC Type 4 driver URL uses
	 * the format: <code>jdbc:t4jdbc://host:port/:prop-name=value</code>.
	 * This validates the URL value and throws SQLException if the URL value is
	 * incorrect.
	 * 
	 * @param url
	 *            the URL.
	 * @see #getUrl()
	 */
	public Properties setURL(String url) throws SQLException {
		return super.setURL(url);
	}

	/**
	 * Returns the URL used in the current Type 4 connection. JDBC Type 4
	 * driver URL uses the format:
	 * <code>jdbc:t4jdbc://host:port/[:][prop-name=value][,prop-name=value]...</code>
	 * 
	 * @return the URL string.
	 * @see #setUrl(String)
	 */
	public String getUrl() {
		return super.getUrl();
	}

	/**
	 * Sets the URL for the Type 4 connection. JDBC Type 4 driver URL uses
	 * the format:
	 * <code>jdbc:t4jdbc://host:port/[:][prop-name=value][,prop-name=value]...</code>
	 * This method does not validate the URL value.
	 * 
	 * @param url
	 *            the URL.
	 * @see #getUrl()
	 */
	public void setUrl(String url) {
		super.setUrl(url);
	}

	/**
	 * Sets the Safeguard user name to be used while connecting to NDCS server
	 * for authentication.
	 * 
	 * @param user
	 *            Sets the user for the current Type 4 connection.
	 * @see #getUser()
	 */
	public void setUser(String user) {
		super.setUser(user);
	}

	/**
	 * Returns the Safeguard user name associated with this Type 4 connection.
	 * 
	 * @return The user name.
	 * @see #setUser(String)
	 */
	public String getUser() {
		return super.getUser();
	}

	/**
	 * Sets the Safeguard password to be used for authentication when connecting
	 * to the NDCS server.
	 * 
	 * @param pwd
	 *            The Safeguard password for the current Type 4 connection.
	 */
	public void setPassword(String pwd) {
		super.setPassword(pwd);
	}

	/**
	 * Sets the login timeout in seconds for the Type 4 connection. The default
	 * login timeout value is set to 60 seconds.
	 * 
	 * @param loginTimeout
	 *            The login timeout value in seconds.
	 * @see #setLoginTimeout(int)
	 * @see #getLoginTimeout()
	 */
	public void setLoginTimeout(String loginTimeout) {
		super.setLoginTimeout(loginTimeout);
	}

	/**
	 * Sets the login timeout in seconds for the Type 4 connection. The default
	 * login timeout value is set to 60 seconds.
	 * 
	 * @param loginTimeout
	 *            The login timeout value in seconds.
	 * @see #setLoginTimeout(String)
	 * @see #getLoginTimeout()
	 */
	public void setLoginTimeout(int loginTimeout) {
		super.setLoginTimeout(loginTimeout);
	}

	/**
	 * Returns the login timeout value set for the current Type 4 connection.
	 * 
	 * @return the login timeout value in seconds.
	 * @see #setLoginTimeout(int)
	 * @see #setLoginTimeout(String)
	 */
	public int getLoginTimeout() {
		return super.getLoginTimeout();
	}

	/**
	 * Sets the network timeout in seconds for the Type 4 connection. The
	 * default network timeout value is set to infinity.
	 * 
	 * @param networkTimeout
	 *            The network timeout value in seconds.
	 * @see #setNetworkTimeout(int)
	 * @see #getNetworkTimeout()
	 */
	public void setNetworkTimeout(String networkTimeout) {
		super.setNetworkTimeout(networkTimeout);
	}

	/**
	 * Sets the network timeout in seconds for the Type 4 connection. The
	 * default network timeout value is set to infinity.
	 * 
	 * @param networkTimeout
	 *            The network timeout value in seconds.
	 * @see #setNetworkTimeout(String)
	 * @see #getNetworkTimeout()
	 */
	public void setNetworkTimeout(int networkTimeout) {
		super.setNetworkTimeout(networkTimeout);
	}

	/**
	 * Returns the network timeout value set for the current Type 4 connection.
	 * 
	 * @return The network timeout value in seconds.
	 * @see #setNetworkTimeout(int)
	 * @see #setNetworkTimeout(String)
	 */
	public int getNetworkTimeout() {
		return super.getNetworkTimeout();
	}

	// -----------------------------------------------------------------

	/*
	 * Sets the connection timeout value for the Type 4 connection. Set this
	 * value to 0 for infinite timeout. The default is set to -1. A negative
	 * value indicates the NDCS server to use the connection timeout value set
	 * by the administrator on the NDCS data source. @param connectionTimeout
	 * The connection timeout value in seconds.
	 * 
	 * @see #setConnectionTimeout(int)
	 * @see #setServerDataSource(String)
	 */
	public void setConnectionTimeout(String connectionTimeout) {
		super.setConnectionTimeout(connectionTimeout);
	}

	/*
	 * Sets the connection timeout value for the Type 4 connection. Set this
	 * value to 0 for infinite timeout. The default is set to -1. A negative
	 * value indicates the NDCS server to use the connection timeout value set
	 * by the administrator on the NDCS data source. @param connectionTimeout
	 * The connection timeout value in seconds.
	 * 
	 * @see #setConnectionTimeout(String)
	 * @see #setServerDataSource(String)
	 */
	public void setConnectionTimeout(int connectionTimeout) {
		super.setConnectionTimeout(connectionTimeout);
	}
	public void setClipVarchar(short clipVarchar) {
		super.setClipVarchar(clipVarchar);
	}

	/**
	 * Sets the max idle time value for the Type 4 connection. The default is
	 * set to 0 (no timeout). Negative values are treated as 0.
	 * 
	 * @param maxIdleTime
	 *            The timeout value in seconds.
	 * @see #setMaxIdleTime(int)
	 */
	public void setMaxIdleTime(String maxIdleTime) {
		super.setMaxIdleTime(maxIdleTime);
	}

	/**
	 * Sets the max idle time value for the Type 4 connection.The default is set
	 * to 0 (no timeout). Negative values are treated as 0.
	 * 
	 * @param maxIdleTime
	 *            The timeout value in seconds.
	 * @see #setMaxIdleTime(String)
	 */
	public void setMaxIdleTime(int maxIdleTime) {
		super.setMaxIdleTime(maxIdleTime);
	}

	/*
	 * Returns the connection timeout value associated with this Type 4
	 * connection. @return The connection timeout value in seconds.
	 * 
	 * @see #setConnectionTimeout(int)
	 */

	public int getConnectionTimeout() {
		return super.getConnectionTimeout();
	}

	/**
	 * Returns the max idle time value associated with this Type 4 connection.
	 * 
	 * @return The max idle timeout value in seconds.
	 * @see #setMaxIdleTime(int)
	 */
	public int getMaxIdleTime() {
		return super.getMaxIdleTime();
	}

	/**
	 * Sets the logging level for the current Type 4 connection. The default
	 * value is OFF. Valid values are:
	 * 
	 * <PRE>
	 *   OFF    (no logging)
	 *   SEVERE (highest value)
	 *   WARNING
	 *   INFO
	 *   CONFIG
	 *   FINE
	 *   FINER
	 *   FINEST (lowest value).
	 *   ALL    (log all messages)
	 * </PRE>
	 * 
	 * @param level
	 *            logging level.
	 * @see #getT4LogLevel()
	 * @see java.util.logging.Level
	 */
	public void setT4LogLevel(String level) {
		super.setT4LogLevel(level);
	}

	/**
	 * Returns the Type 4 log level associated with the current Type 4
	 * connection. Possible log levels are described in java.util.logging.Level
	 * 
	 * @return <code>java.util.logging.Level</code> associated with the
	 *         current Type 4 connection.
	 * @see #setT4LogLevel(String)
	 * @see java.util.logging.Level
	 */
	public Level getT4LogLevel() {
		return super.getT4LogLevel();
	}

	/**
	 * Sets the location of the file to which the logging is to be done.
	 * Changing this location after making a connection has no effect; because
	 * the driver reads this property before the connection is made. The default
	 * name is a generated file name defined by the following pattern:
	 * %h/t4jdbc%u.log where: "/" represents the local pathname separator "%h"
	 * represents the value of the "user.home" system property. If %h is not
	 * defined, then the behavior is undefined "%u" represents a unique number
	 * to resolve conflicts
	 * 
	 * @param t4LogFile
	 *            The Type 4 log file location. If the parameter is null, then
	 *            the T4LogFile is set to the global log file.
	 * @see #getT4LogFile()
	 * @see java.util.logging.Logger
	 */
	public void setT4LogFile(String t4LogFile) {
		super.setT4LogFile(t4LogFile);
	}

	/**
	 * Returns the Type 4 log file location associated with the current Type 4
	 * connection.
	 * 
	 * @return The Type 4 log file location.
	 * @see #setT4LogFile(String)
	 */
	public String getT4LogFile() {
		return super.getT4LogFile();
	}

	/**
	 * @return Returns the rounding mode set for the driver as an Integer value
	 *         with one of the values:
	 * 
	 * <PRE>
	 * static int ROUND_CEILING
	 *       Rounding mode to round toward positive infinity.
	 * static int ROUND_DOWN
	 *       Rounding mode to round toward zero.
	 * static int ROUND_FLOOR
	 *      Rounding mode to round toward negative infinity.
	 * static int ROUND_HALF_DOWN
	 *      Rounding mode to round toward &quot;nearest neighbor&quot; unless both
	 *      neighbors are equidistant, in which case round down.
	 * static int ROUND_HALF_EVEN
	 *      Rounding mode to round toward the &quot;nearest neighbor&quot; unless
	 *      both neighbors are equidistant, in which case, round towards the even neighbor.
	 * static int ROUND_HALF_UP
	 *      Rounding mode to round toward &quot;nearest neighbor&quot; unless both
	 *      neighbors are equidistant, in which case round up.
	 * static int ROUND_UNNECESSARY
	 *      Rounding mode to assert that the requested operation has an exact
	 *      result; hence no rounding is necessary.
	 * static int ROUND_UP
	 *      Rounding mode to round away from zero.
	 * </PRE>
	 */
	public int getRoundingMode() {
		return super.getRoundingMode();
	}

	/**
	 * Sets the round mode behaviour for the driver.
	 * 
	 * @param roundMode
	 *            String value with one of the values:
	 * 
	 * <PRE>
	 * static int ROUND_CEILING
	 *       Rounding mode to round toward positive infinity.
	 * static int ROUND_DOWN
	 *       Rounding mode to round toward zero.
	 * static int ROUND_FLOOR
	 *      Rounding mode to round toward negative infinity.
	 * static int ROUND_HALF_DOWN
	 *      Rounding mode to round toward &quot;nearest neighbor&quot; unless both
	 *      neighbors are equidistant, in which case round down.
	 * static int ROUND_HALF_EVEN
	 *      Rounding mode to round toward the &quot;nearest neighbor&quot; unless
	 *      both neighbors are equidistant, in which case, round towards the even neighbor.
	 * static int ROUND_HALF_UP
	 *      Rounding mode to round toward &quot;nearest neighbor&quot; unless both
	 *      neighbors are equidistant, in which case round up.
	 * static int ROUND_UNNECESSARY
	 *      Rounding mode to assert that the requested operation has an exact
	 *      result; hence no rounding is necessary.
	 * static int ROUND_UP
	 *      Rounding mode to round away from zero.
	 * </PRE>
	 * 
	 * The default value is ROUND_HALF_EVEN.
	 */
	public void setRoundingMode(String roundMode) {
		super.setRoundingMode(roundMode);
	}

	/**
	 * Sets the round mode behaviour for the driver.
	 * 
	 * @param roundMode
	 *            Integer value with one of the values:
	 * 
	 * <PRE>
	 * static int ROUND_CEILING
	 *       Rounding mode to round toward positive infinity.
	 * static int ROUND_DOWN
	 *       Rounding mode to round toward zero.
	 * static int ROUND_FLOOR
	 *      Rounding mode to round toward negative infinity.
	 * static int ROUND_HALF_DOWN
	 *      Rounding mode to round toward &quot;nearest neighbor&quot; unless both
	 *      neighbors are equidistant, in which case round down.
	 * static int ROUND_HALF_EVEN
	 *      Rounding mode to round toward the &quot;nearest neighbor&quot; unless
	 *      both neighbors are equidistant, in which case, round towards the even neighbor.
	 * static int ROUND_HALF_UP
	 *      Rounding mode to round toward &quot;nearest neighbor&quot; unless both
	 *      neighbors are equidistant, in which case round up.
	 * static int ROUND_UNNECESSARY
	 *      Rounding mode to assert that the requested operation has an exact
	 *      result; hence no rounding is necessary.
	 * static int ROUND_UP
	 *      Rounding mode to round away from zero.
	 * </PRE>
	 * 
	 * The default value is ROUND_HALF_EVEN.
	 */
	public void setRoundingMode(int roundMode) {
		super.setRoundingMode(roundMode);
	}

	/**
	 * Sets the value (in KB) for the size of the fetch buffer. This is used
	 * when rows are fetched are performed from a ResultSet object after a
	 * successful executeQuery() operation on a statement. The default size is
	 * 4. Zero and negative values are treated as default values.
	 * 
	 * @param fetchBufferSize
	 * @see #getFetchBufferSize()
	 * @see #setFetchBufferSize(String)
	 */
	void setFetchBufferSize(short fetchBufferSize) {
		super.setFetchBufferSize(fetchBufferSize);
	}

	/**
	 * Sets the value (in KB) for the size of the fetch buffer. This is used
	 * when rows are fetched are performed from a ResultSet object after a
	 * successful executeQuery() operation on a statement. The default size is
	 * 4. Zero and negative values are treated as default values.
	 * 
	 * @param fetchBufferSize
	 * @see #getFetchBufferSize()
	 * @see #setFetchBufferSize(short)
	 */
	void setFetchBufferSize(String fetchBufferSize) {
		super.setFetchBufferSize(fetchBufferSize);
	}

	/**
	 * Returns the size of the fetch buffer.
	 * 
	 * @see #setFetchBufferSize(short)
	 * @see #setFetchBufferSize(String)
	 */
	short getFetchBufferSize() {
		return super.getFetchBufferSize();
	}

} // end class T4DSProperties

