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

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.math.BigDecimal;
import java.sql.DriverPropertyInfo;
import java.sql.SQLException;
import java.util.Locale;
import java.util.Properties;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.naming.Reference;
import javax.naming.StringRefAddr;

/**
 * <p>
 * JDBC Type 4 driver connetion properties class.
 * </p>
 * <p>
 * Description: The <code>T4Properties</code> class contains all the
 * properties associated with Type 4 connection. <code>T4Properties</code> is
 * inherited directy or indirectly by the <code>T4Driver, TrafT4DataSource,
 * TrafT4ConnectionPooledDataSource</code>
 * classes for configuring Type 4 connection properties.
 * </p>
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
 *  Licensed to the Apache Software Foundation (ASF)
 * </p>
 */
public class T4Properties {
    final static int DEFAULT_NETWORK_TIMEOUT_IN_MILLIS = 1000;
    final static int DEFAULT_CONNECT_TIMEOUT_IN_SECS = 10;
	private String description_;
	private String dataSourceName_;
	private String serverDataSource_;
	private String catalog_;
	private String schema_;
	private String user_;
	private String url_;
	private String pwd_;
	private Locale locale_;
	private int maxPoolSize_;
	private int minPoolSize_;
	private int initialPoolSize_;
	private int maxStatements_;
	private int loginTimeout_;
	// private int closeConnectionTimeout_;
	private int networkTimeoutInMillis_ = DEFAULT_NETWORK_TIMEOUT_IN_MILLIS;
	private int connectionTimeout_;
	private int maxIdleTime_;
	private Level t4LogLevel;
	private String T4LogFile_;
	private Properties defaults_;
	private Properties inprops_;
	private PrintWriter logWriter_;
	// For LOB Support - SB 9/28/04
	private int roundMode_;
	private String language_;

	private short fetchBufferSize_;

	private String connectionID_;
	private String dialogueID_;
	private String serverID_;
	private short ncsMajorVersion_;
	private short ncsMinorVersion_;
	private short sqlmxMajorVersion_;
	private short sqlmxMinorVersion_;

    private int lobChunkSize_ = 10; // default 10M
    private boolean useLobHandle_ = false;

    // private short transportBufferSize_;
	private boolean useArrayBinding_;
	private boolean batchRecovery_;
	private final String propPrefix_ = "t4jdbc.";
	//clipVarchar flag
	private short clipVarchar_ = 0;
	// Default catalog 
	static final String DEFAULT_CATALOG = "TRAFODION";

	// propertiy queryTimeout_ for future use.
	private short queryTimeout_;
 	private boolean ignoreCancel_ = true;
	private int activeTimeBeforeCancel_; 
	private T4Address t4addr_;

	// Error handling while setting Type 4 properties.
	String sqlExceptionMessage_;

	// Logger for this connection.
	Logger logger;
	Logger t4Logger_;

	// Log file handler for this connection.
	FileHandler t4LogFileHandler_;
	
	//clientInfoProperties
	private Properties clientInfoProp;
	

	private boolean SPJEnv_ = false;
	private boolean keepRawFetchBuffer_ = false;
	private short cpuToUse_ = -1;
	private String sessionName;
	private String replacementString_;
	private String ISO88591_;

	private String _roleName;
	private String _applicationName;
	private boolean _sessionToken;
	private boolean _fetchAhead;
	private boolean _delayedErrorMode;
	private boolean _compression;
	private String _certificateDir;
	private String _certificateFileActive;
	private String _certificateFile;
	private boolean _keepAlive = false;
	private boolean _tokenAuth;
	private static int DEFAULT_MAX_IDLE_TIMEOUT = 0; // Max idle timeout
	// default = infinite
    
	//TCP Nagle's algorithm 
	private boolean _tcpNoDelay = true;

	// -----------------------------------------------------------
	//
	// The following static members and static block are ment to
	// establish the one and only global t4 logger for this JVM.
	//
	static Level t4GlobalLogLevel = null;
	static String t4GlobalLogFile = null;
	static Logger t4GlobalLogger = null;
	static FileHandler t4GlobalLogFileHandler = null;

	private String clientCharset;

	void initializeLogging() {
		if (t4GlobalLogger != null) {
			return;
		}

		t4GlobalLogger = Logger.getLogger("org.trafodion.jdbc.t4.logger");

		try {
			boolean createGlobalLogFile = false;
			String userSpecifiedLogFile = null;
			Level userSpecifiedLogLevel = null;
			Properties p = getPropertiesFileValues();

			t4GlobalLogger.setUseParentHandlers(false);
			//
			// See if the user specified a log file and/or log level via the
			// properties file or the system properties.
			//
			if (p != null) {
				userSpecifiedLogFile = p.getProperty("T4LogFile");
				String temp = p.getProperty("T4LogLevel");
				if (temp != null) {
					userSpecifiedLogLevel = Level.parse(temp);
				}
			}

			if (userSpecifiedLogFile == null) {
				userSpecifiedLogFile = System.getProperty("t4jdbc.T4LogFile");

			}
			if (userSpecifiedLogLevel == null) {
				String temp = System.getProperty("t4jdbc.T4LogLevel");
				if (temp != null) {
					userSpecifiedLogLevel = Level.parse(temp);
				} else {
					userSpecifiedLogLevel = null;
				}
			}

			//
			// At this point, if the user specified a log file and/or log level,
			// we have captured
			// that information.
			//

			//
			// Decide if we should create a global log file now or wait until
			// someone
			// asks for it. That is, until a connection turns on logging and
			// doesn't
			// specify its own log file.
			//
			if (userSpecifiedLogFile != null || (userSpecifiedLogLevel != null && userSpecifiedLogLevel != Level.OFF)) {
				createGlobalLogFile = true;
			}

			//
			// Set the global log file and global log level.
			//
			if (userSpecifiedLogFile == null) {
				t4GlobalLogFile = T4LoggingUtilities.getUniqueLogFileName(T4LoggingUtilities.getUniqueID());
			} else {
				t4GlobalLogFile = userSpecifiedLogFile;
			}
			if (userSpecifiedLogLevel == null) {
				t4GlobalLogLevel = Level.OFF;
			} else {
				t4GlobalLogLevel = userSpecifiedLogLevel;
			}
			t4GlobalLogger.setLevel(t4GlobalLogLevel);

			//
			// If we decided to make the log file now, make the file (i.e.
			// FileHandler) and
			// it the global logger.
			//
			if (createGlobalLogFile == true) {
				t4GlobalLogFileHandler = new FileHandler(t4GlobalLogFile);

				t4GlobalLogger.addHandler(t4GlobalLogFileHandler);

				Formatter ff1 = new T4LogFormatter();

				t4GlobalLogFileHandler.setFormatter(ff1);
			}
		} catch (Exception e) {
			// SQLException se = TrafT4Messages.createSQLException(null, null,
			// "problem_with_logging", e.getMessage());
			// sqlExceptionMessage_ = se.getMessage();

			SQLException se = TrafT4Messages.createSQLException(null, null, "problem_with_logging", e.getMessage());
			sqlExceptionMessage_ = se.getMessage();
			// RuntimeException rte = new RuntimeException(se.getMessage(), se);
			// throw rte;
			// e.printStackTrace();
			// throw TrafT4Messages.createSQLException(null, null,
			// "problem_with_logging", e.getMessage());
		}
	} // end initializeLogging

	// -----------------------------------------------------------------

	public T4Properties() {
		initializeLogging();
		initialize(null);
	}

	public T4Properties(Properties props) {
		initializeLogging();
		initialize(props);
	}

	void initialize(Properties props) {
		sqlExceptionMessage_ = null;
		inprops_ = props;
		setProperties();
		clientInfoProp = new Properties();
	}

	// ---------------------------------------------------------------
	private Object getHashTableEntry(String token) {
		Object outObj = null;

		// if the token is present in input-props get it
		if (inprops_ != null) {
			outObj = inprops_.get((Object) token);

		}
		return outObj;
	} // end getHashTableEntry

	// ---------------------------------------------------------------
	private String getProperty(String token) {
		String ret = null;

		// check input props first
		if (inprops_ != null) {
			ret = inprops_.getProperty(token);
		}
		// props file next
		if (ret == null && defaults_ != null) {
			ret = defaults_.getProperty(token);
		}
		// system properties with the t4jdbc prefix
		if (ret == null) {
			ret = System.getProperty(propPrefix_ + token);
		}

		return ret;
	}

	private void setProperties() {
		defaults_ = null;
		String propsFile = getProperty("properties");
		if (propsFile != null) {
			propsFile = propsFile.trim();
			if (propsFile.length() != 0) {
				FileInputStream fis = null;
				try {
					fis = new FileInputStream(new File(propsFile));
					defaults_ = new Properties();
					defaults_.load(fis);
				} catch (Exception ex) {
					fis = null;
					sqlExceptionMessage_ = "Error while loading " + propPrefix_ + "properties file: " + ex.getMessage();
				} finally {
					try {
						if (fis != null) {
							fis.close();
						}
					} catch (IOException ioe) {
						// ignore
					}
				}
			}
		}

		setDescription(getProperty("description"));
		setDataSourceName(getProperty("dataSourceName"));
		setServerDataSource(getProperty("serverDataSource"));
		setCatalog(getProperty("catalog"));
		setSchema(getProperty("schema"));

		// DriverManager passes property user. HPUX does not like
		// property user. System properties must be prefixed with t4jdbc.
		// For example user should be t4jdbc.user if set as system property.
		setUser(getProperty("user"));
		setPassword(getProperty("password"));

		// url is special. Set it from properties only when it is not already
		// set.
		if (url_ == null) {
			setUrl(getProperty("url"));

		}
		setMaxPoolSize(getProperty("maxPoolSize"));
		setMinPoolSize(getProperty("minPoolSize"));
		setInitialPoolSize(getProperty("initialPoolSize"));
		setMaxStatements(getProperty("maxStatements"));
		setLoginTimeout(getProperty("loginTimeout"));
		// setCloseConnectionTimeout(getProperty("closeConnectionTimeout"));
		setNetworkTimeout(getProperty("networkTimeout"));
		setT4LogLevel(getProperty("T4LogLevel"));
		setT4LogFile(getProperty("T4LogFile"));
		setLogger(getHashTableEntry("logger"));
		setT4LogFileHandler(getHashTableEntry("T4LogFileHandler"));
		setUseArrayBinding(getProperty("useArrayBinding"));
		setBatchRecovery(getProperty("batchRecovery"));
		// setTransportBufferSize(getProperty("TransportBufferSize"));
		setLanguage(getProperty("language"));
		setMaxIdleTime(getProperty("maxIdleTime"));
		setConnectionTimeout(getProperty("connectionTimeout"));
		setClipVarchar(getProperty("clipVarchar"));
		setFetchBufferSize(getProperty("fetchBufferSize")); 
		setQueryTimeout(getProperty("queryTimeout"));
		setActiveTimeBeforeCancel(getProperty("activeTimeBeforeCancelInSecs"));
		setRoundingMode(getProperty("roundingMode"));
		setSPJEnv(getProperty("SPJEnv"));
		setKeepRawFetchBuffer(getProperty("keepRawFetchBuffer"));
		setCpuToUse(getProperty("cpuToUse"));
		setSessionName(getProperty("sessionName"));
		setReplacementString(getProperty("replacementString"));
		setISO88591(getProperty("ISO88591"));

		setRoleName(getProperty("roleName"));
		setApplicationName(getProperty("applicationName"));
		setSessionToken(getProperty("sessionToken"));
		setFetchAhead(getProperty("fetchAhead"));
		setDelayedErrorMode(getProperty("delayedErrorMode"));
		setCompression(getProperty("compression"));
		setCertificateDir(getProperty("certificateDir"));
		setCertificateFileActive(getProperty("certificateFileActive"));
		setCertificateFile(getProperty("certificateFile"));
		setKeepAlive(getProperty("keepAlive"));
		setTokenAuth(getProperty("tokenAuth"));
        setTcpNoDelay(getProperty("tcpNoDelay"));

        setLobChunkSize(getProperty("lobChunkSize"));
        setUseLobHandle(getProperty("useLobHandle"));

        setClientCharset(getProperty("clientCharset"));
	}

	T4Properties getT4Properties() {
		return this;
	}

	/**
	 * Returns all the connection properties associated with the current Type 4
	 * connection object in the <code>java.util.Properties</code> object.
	 * 
	 * @return The properties associated with the current Type 4 connection.
	 */
	public Properties getProperties() {
		Properties props = new Properties();
		if (getCatalog() != null) {
			props.setProperty("catalog", catalog_);
		}
		if (getSchema() != null) {
			props.setProperty("schema", schema_);
		}
		if (url_ != null) {
			props.setProperty("url", url_);
		}

		props.setProperty("user", user_);
		props.setProperty("password", pwd_);
		props.setProperty("maxPoolSize", String.valueOf(maxPoolSize_));
		props.setProperty("minPoolSize", String.valueOf(minPoolSize_));
		props.setProperty("initialPoolSize", String.valueOf(initialPoolSize_));
		props.setProperty("maxStatements", String.valueOf(maxStatements_));
		props.setProperty("T4LogLevel", t4LogLevel.toString());
		props.setProperty("fetchBufferSize", String.valueOf(fetchBufferSize_));

		if (logger != null) {
			props.put("logger", logger);
		}
		if (t4LogFileHandler_ != null) {
			props.put("T4LogFileHandler", t4LogFileHandler_);

		}
		props.setProperty("T4LogFile", T4LogFile_);
		props.setProperty("loginTimeout", String.valueOf(loginTimeout_));
		// props.setProperty("closeConnectionTimeout",
		// String.valueOf(closeConnectionTimeout_));
		props.setProperty("networkTimeout", String.valueOf(getNetworkTimeout()));
		props.setProperty("connectionTimeout", String.valueOf(connectionTimeout_));
		props.setProperty("description", description_);
		props.setProperty("dataSourceName", dataSourceName_);
		props.setProperty("serverDataSource", serverDataSource_);
		// props.setProperty("transportBufferSize",
		// String.valueOf(transportBufferSize_));
		props.setProperty("useArrayBinding", String.valueOf(useArrayBinding_));
		props.setProperty("batchRecovery", String.valueOf(batchRecovery_));
		props.setProperty("maxIdleTime", String.valueOf(maxIdleTime_));
		props.setProperty("language", language_);

		// properties queryTimeout_ for future use.
		props.setProperty("queryTimeout", String.valueOf(queryTimeout_));
		props.setProperty("ignoreCancel", String.valueOf(ignoreCancel_));
		props.setProperty("activeTimeBeforeCancelInSecs", String.valueOf(activeTimeBeforeCancel_));
		props.setProperty("roundingMode", String.valueOf(roundMode_));
		props.setProperty("SPJEnv", String.valueOf(SPJEnv_));
		props.setProperty("keepRawFetchBuffer", String.valueOf(keepRawFetchBuffer_));
		props.setProperty("cpuToUse", String.valueOf(cpuToUse_));

        // NOTE
        // String.valueOf(null) will give a "null" string, this may raise bug.
        // So do not use String.valueOf() for a String variable
        if (sessionName != null) {
            props.setProperty("sessionName", sessionName);
        }
        if (replacementString_ != null) {
            props.setProperty("replacementString", replacementString_);
        }
        if (ISO88591_ != null) {
            props.setProperty("ISO88591", ISO88591_);
        }
		if (_roleName != null)
			props.setProperty("roleName", _roleName);
		if (_applicationName != null)
			props.setProperty("applicationName", _applicationName);

		props.setProperty("sessionToken", String.valueOf(_sessionToken));
		props.setProperty("fetchAhead", String.valueOf(_fetchAhead));
		props.setProperty("delayedErrorMode", String.valueOf(_delayedErrorMode));
		props.setProperty("compression", String.valueOf(_compression));
		if(_certificateDir != null) 	
			props.setProperty("certificateDir", _certificateDir);
		if(_certificateFileActive != null)
			props.setProperty("certFileActive", _certificateFileActive);
		if(_certificateFile != null)
			props.setProperty("certificateFile", _certificateFile);
		props.setProperty("keepAlive", String.valueOf(_keepAlive));
		props.setProperty("tokenAuth", String.valueOf(_tokenAuth));
        props.setProperty("tcpNoDelay", String.valueOf(_tcpNoDelay));

        props.setProperty("clipVarchar", String.valueOf(clipVarchar_));
        props.setProperty("lobChunkSize", String.valueOf(lobChunkSize_));
        props.setProperty("useLobHandle", String.valueOf(useLobHandle_));
        if (clientCharset != null)
            props.setProperty("clientCharset", clientCharset);
		return props;
	}

	/**
	 * Sets the description for the current Type 4 connection.
	 * 
	 * @param description
	 *            For the current Type 4 connection.
	 * @see #getDescription()
	 */
	void setDescription(String description) {
		if (description == null) {
			description_ = "JDBC T4 DataSource.";
		} else {
			description_ = description;
		}
	}

	/**
	 * Returns the description associated with the current Type 4 connection.
	 * 
	 * @return The description associated with the current Type 4 connection.
	 * @see #setDescription(String)
	 */
	String getDescription() {
		return description_;
	}

	/**
	 * Sets the data source name for the current Type 4 connection.
	 * 
	 * @param dataSourceName
	 *            For the client side <code>DataSource</code> object.
	 * @see #getDataSourceName()
	 */
	void setDataSourceName(String dataSourceName) {
		if (dataSourceName == null) {
			dataSourceName_ = "";
		} else {
			dataSourceName_ = dataSourceName;
		}
	}

	/**
	 * Return the data source name given to the client side data source.
	 * 
	 * @return data source name.
	 * @see #setDataSourceName(String)
	 */
	String getDataSourceName() {
		return dataSourceName_;
	}

	/**
	 * Sets the data source name to use on the NDCS server side.
	 * 
	 * @param serverDataSource
	 *            the data source name to use on the NDCS server side. The
	 *            default value is a blank string.
	 * @see #getDataSourceName()
	 */
	void setServerDataSource(String serverDataSource) {
		if (serverDataSource == null) {
			serverDataSource_ = "";
		} else {
			serverDataSource_ = serverDataSource;
		}
	}

	/**
	 * Returns the NDCS server side data source name used for the current Type 4
	 * connection. The default server data source name is
	 * <code>TDM_Defaullt_DataSource</code>.
	 * 
	 * @return NDCS server side data source Name.
	 * @see #setServerDataSource(String)
	 */
	String getServerDataSource() {
		return serverDataSource_;
	}

	/**
	 * Sets the default catalog that will be used to access SQL objects
	 * referenced in SQL statements if the SQL objects are not fully qualified.
	 * 
	 * @param catalog
	 *            Database catalog name. The default catalog name is set by
	 *            the NDCS server side data source.
	 * @see #getCatalog()
	 */
	void setCatalog(String catalog) {
		catalog_ = catalog;
		if (catalog_ == null) {
			catalog_ = getProperty("catalog");

		}
		if (catalog_ != null) {
			if (catalog_.length() == 0) {
				catalog_ = DEFAULT_CATALOG;
			} else if (!catalog_.startsWith("\"")) {
				catalog_ = catalog_.trim().toUpperCase();
			}
		} else {
			// catalog_ = null;
			catalog_ = DEFAULT_CATALOG;
		}
	}

	/**
	 * Gets the default catalog that will be used to access SQL objects
	 * referenced in SQL statements if the SQL objects are not fully qualified.
	 * 
	 * @return T4 2.0 catalog name.
	 * @see #setCatalog(String)
	 */
	String getCatalog() {
		return catalog_;
	}

	/**
	 * Sets the default schema that will be used to access SQL objects
	 * referenced in SQL statements if the SQL objects are not fully qualified.
	 * 
	 * @param schema
	 *            Sets the database schema name. The default schema name is set
	 *            by the NDCS server side data source.
	 * @see #getSchema()
	 */
	void setSchema(String schema) {
		schema_ = schema;
		if (schema_ == null) {
			schema_ = getProperty("schema");

		}
		if (schema_ != null) {
			if (!schema_.startsWith("\"")) {
				schema_ = schema_.trim().toUpperCase();
			}
		} else {
			schema_ = null;
		}
	}

	/**
	 * Gets the default schema that will be used to access SQL objects
	 * referenced in SQL statements if the SQL objects are not fully qualified.
	 * 
	 * @return The schema associated with the current Type 4 connection.
	 * @see #setSchema(String)
	 */
	String getSchema() {
		return schema_;
	}

	/**
	 * Returns the <code>java.util.Locale</code> object associated with the
	 * current Type 4 connection.
	 * 
	 * @return <code>java.util.Locale</code> object.
	 * @see #setLanguage(String language)
	 */
	Locale getLocale() {
		return locale_;
	}

	/**
	 * The maximum number of physical connections that the pool (free and inuse
	 * pool) should contain. When the maximum number of physical connections is
	 * reached, the Type 4 driver throws an <code>SQLException
	 * </code> with the
	 * message "Maximum pool size reached". Specifying a value of 0 (zero)
	 * indicates there is no maximum size for the pool. Specifying a value of -1
	 * indicates no connection pooling is performed. The default value is -1
	 * indicating that no pooling of physical connections is done.
	 * 
	 * @param maxPoolSize
	 *            the maximum number of physical connections the pool should
	 *            contain in the pool (free and inuse).
	 * @see #setMaxPoolSize(int)
	 */
	void setMaxPoolSize(String maxPoolSize) {
		int maxPs = -1;
		if (maxPoolSize != null) {
			try {
				maxPs = Integer.parseInt(maxPoolSize);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect maxPoolSize value set: " + maxPoolSize + ". " + ex.getMessage();
				maxPs = -1;
			}
		}
		setMaxPoolSize(maxPs);
	}

	/**
	 * The maximum number of physical connections that the pool (free and inuse
	 * pool) should contain. When the maximum number of physical connections is
	 * reached, the Type 4 driver throws an <code>SQLException
	 * </code> with the
	 * message "Maximum pool size reached". Specifying a value of 0 (zero)
	 * indicates there is no maximum size for the pool. Specifying a value of -1
	 * indicates no connection pooling is performed. The default value is -1
	 * indicating that no pooling of physical connections is done.
	 * 
	 * @param maxPoolSize
	 *            the maximum number of physical connections the pool should
	 *            contain in the pool (free and inuse).
	 * @see #setMaxPoolSize(String)
	 */
	void setMaxPoolSize(int maxPoolSize) {
		if (maxPoolSize < -1) {
			// The ES says that we set maxPoolSize to -1. It doesn't say we
			// produce an error.
			// sqlExceptionMessage_ = "Incorrect maxPoolSize value set: " +
			// maxPoolSize;
			maxPoolSize_ = -1;
		} else if (maxPoolSize > 0) {
			if (minPoolSize_ != 0 && minPoolSize_ > maxPoolSize) {
				maxPoolSize_ = minPoolSize_;
			} else {
				maxPoolSize_ = maxPoolSize;
			}
		} else {
			maxPoolSize_ = maxPoolSize;
		}
	}

	/**
	 * Returns the maximum number of physical connections that the pool (free
	 * and inuse) should contain. A value of zero (0) indicates no maximum size.
	 * A value of -1 indicates that connection pooling is not being done.
	 * 
	 * @return maxPoolSize the maximum number of physical connections that the
	 *         pool should contain.
	 * @see #setMaxPoolSize(int)
	 */
	int getMaxPoolSize() {
		return maxPoolSize_;
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
	 *            ignored. The default value is 0. For this data source it is
	 *            recommended that you use the default value.
	 * @return the number of physical connections the pool should keep available
	 *         at all times.
	 * @see #getMinPoolSize()
	 * @see #setMinPoolSize(int minPoolSize)
	 */
	void setMinPoolSize(String minPoolSize) {
		int minPs = -1;
		if (minPoolSize != null) {
			try {
				minPs = Integer.parseInt(minPoolSize);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for minPoolSize set: " + minPoolSize + ". " + ex.getMessage();
				minPs = -1;
			}
		}
		setMinPoolSize(minPs);
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
	 *            ignored. The default value is 0. For this data source it is
	 *            recommended that you use the default value.
	 * @return the number of physical connections the pool should keep available
	 *         at all times.
	 * @see #getMinPoolSize()
	 * @see #setMinPoolSize(String minPoolSize)
	 */
	void setMinPoolSize(int minPoolSize) {
		if (maxPoolSize_ == -1) {
			// minPoolSize_ = minPoolSize;
			minPoolSize_ = -1;
			return;
		}
		if (minPoolSize < -1) {
			// Swastik: Commented on 14th Feb 2005 to match behavior in
			// setMaxPoolSize(int)
			// sqlExceptionMessage_ = "Incorrect value for minPoolSize set: " +
			// minPoolSize;
			minPoolSize_ = -1;
		} else if (minPoolSize == 0) {
			minPoolSize_ = 0;
		} else {
			if (minPoolSize > 0) {
				if (maxPoolSize_ != 0 && maxPoolSize_ < minPoolSize) {
					minPoolSize_ = maxPoolSize_;
				} else {
					minPoolSize_ = minPoolSize;
				}
			} else {
				minPoolSize_ = minPoolSize;
			}
		}
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
	int getMinPoolSize() {
		return minPoolSize_;
	}

	/**
	 * The initial number of physical connections that the pool should be
	 * created with. Specifying a valueof 0 (zero) or less indicates that the
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
	void setInitialPoolSize(String initialPoolSize) {
		int initPs = -1;
		if (initialPoolSize != null) {
			try {
				initPs = Integer.parseInt(initialPoolSize);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect initialPoolSize value set: " + initialPoolSize + ". "
						+ ex.getMessage();
				initPs = -1;
			}
		}
		setInitialPoolSize(initPs);
	}

	/**
	 * The initial number of physical connections that the pool should be
	 * created with. Specifying a valueof 0 (zero) or less indicates that the
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
	void setInitialPoolSize(int initialPoolSize) {
		if (initialPoolSize <= 0) {
			initialPoolSize_ = -1;
			return;
		}
		// otherwise initialPoolSize > 0
		if (initialPoolSize > maxPoolSize_) {
			initialPoolSize_ = maxPoolSize_;
		} else {
			initialPoolSize_ = initialPoolSize;
		}
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
	int getInitialPoolSize() {
		return initialPoolSize_;
	}

	/**
	 * Total number of statements that can be pooled. A value of zero (0)
	 * indicates that caching of statements is disabled.
	 * 
	 * @param maxStatements
	 *            The number of statements that can be pooled.
	 * @see #setMaxStatements(int)
	 */
	void setMaxStatements(String maxStatements) {
		int maxstmt = 0;
		if (maxStatements != null) {
			try {
				maxstmt = Integer.parseInt(maxStatements);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for maxStatements set: " + maxStatements + ex.getMessage();
				maxstmt = 0;
			}
		}
		setMaxStatements(maxstmt);
	}

	/**
	 * Total number of statements that can be pooled. A value of zero (0)
	 * indicates that caching of statements is disabled.
	 * 
	 * @param maxStatements
	 *            The number of statements that can be pooled.
	 * @see #setMaxStatements(int)
	 */
	void setMaxStatements(int maxStatements) {
		if (maxStatements < 0) {
			sqlExceptionMessage_ = "Incorrect value for maxStatements set: " + maxStatements;
			maxStatements_ = 0;
		} else {
			maxStatements_ = maxStatements;
		}
	}

	/**
	 * Returns the total number of statements that can be pooled. A value of
	 * zero (0) indicates that pooling of statements is disabled.
	 * 
	 * @return The total number of statements that can be pooled.
	 */
	int getMaxStatements() {
		return maxStatements_;
	}

	/**
	 * Returns the URL used in the current Type 4 connection. JDBC Type 4
	 * driver URL uses the following format:-
	 * <code>jdbc:t4jdbc://host:port/:</code>
	 * 
	 * @deprecated Use <code>getUrl()</code> to obtain the URL string.
	 * @return the URL string.
	 * @see #getUrl()
	 */
	String getURL() {
		return getUrl();
	}

	/**
	 * Returns the URL used in the current Type 4 connection. JDBC Type 4
	 * driver URL uses the following format:-
	 * <code>jdbc:t4jdbc://host:port/:</code>
	 * 
	 * @return the URL string.
	 * @see #setUrl(String)
	 */
	public String getUrl() {
		return url_;
	}

	/**
	 * Sets the URL for the Type 4 connection. JDBC Type 4 driver URL uses
	 * the following format:-
	 * <code>jdbc:t4jdbc://host:port/:prop-name=value</code> This method
	 * does not validate the url value.
	 * 
	 * @param url
	 *            the URL.
	 * @see #getUrl()
	 */
	void setUrl(String url) {
		if (url != null) {
			url_ = url;
		} else {
			url_ = "";
		}
	}

	/**
	 * Validates the input url string follows the correct jdbc:t4jdbc:<host>:<port>/:<prop-name=prop-val>;<prop-name=prop-val>
	 * format.
	 * 
	 * @param url
	 *            of the Type 4 driver.
	 * @return <code>true</code> when the <url> is in the Type 4 driver
	 *         format.
	 */
	boolean acceptsUrl(String url) {
		try {
			new T4Address(this, locale_, url);
		} catch (SQLException sqlex) {
			return false;
		}
		return true;
	}

	/**
	 * Sets the URL for the Type 4 connection. JDBC Type 4 driver URL uses
	 * the following format:-
	 * <code>jdbc:t4jdbc://host:port/:prop-name=value</code>. This
	 * validates the url value and throws SQLException if the URL value is
	 * incorrect.
	 * 
	 * @param url
	 *            the URL.
	 * @see #getUrl()
	 */
	Properties setURL(String url) throws SQLException {
		String host = null;
		String port = null;
		Properties props = null;

		t4addr_ = new T4Address(this, locale_, url);
		props = t4addr_.getProps();
		initialize(props);
		setUrl(t4addr_.getUrl());
		return props;
	}

	/**
	 * Retunrs the Type 4 <code>T4Address</code> class.
	 * 
	 * @return the current <code>T4Address</code> class.
	 */
	T4Address getAddress() {
		return t4addr_;
	}

	/**
	 * Sets the Safeguard user name to be used while connecting to NDCS server
	 * for authentication.
	 * 
	 * @param user
	 *            Sets the user for the current Type 4 connection.
	 * @see #getUser()
	 */
	void setUser(String user) {
		if (user == null) {
			user_ = "";
		} else {
			user_ = user;
		}
	}

	/**
	 * Returns the Safeguard user name associated with this Type 4 connection.
	 * 
	 * @return The user name.
	 * @see #setUser(String)
	 */
	String getUser() {
		return user_;
	}

	/**
	 * Sets the Safeguard password to be used for authentication when connecting
	 * to the NDCS server.
	 * 
	 * @param pwd
	 *            The Safeguard password for the current Type 4 connection.
	 */
	void setPassword(String pwd) {
		if (pwd == null) {
			pwd_ = "";
		} else {
			pwd_ = pwd;
		}
	}

	/**
	 * @deprecated <code>getPassword()</code> would not be supported in the
	 *             future releases.
	 * @return the password associated with this Type 4 connection.
	 */
	String getPassword() {
		return pwd_;
	}

	/**
	 * Sets the login timeout in seconds for the Type 4 connection. The default
	 * login timeout value is set to 30 minutes.
	 * 
	 * @param loginTimeout
	 *            The login timeout value in seconds.
	 * @see #setLoginTimeout(int)
	 * @see #getLoginTimeout()
	 */
	void setLoginTimeout(String loginTimeout) {
		int loginTo = 1800;
		if (loginTimeout != null) {
			try {
				loginTo = Short.parseShort(loginTimeout);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for loginTimeout set: " + loginTimeout + ex.getMessage();
				loginTo = 1800;
			}
		}
		setLoginTimeout(loginTo);
	}

	/**
	 * Sets the login timeout in seconds for the Type 4 connection. The default
	 * login timeout value is set to 30 minutes.
	 * 
	 * @param loginTimeout
	 *            The login timeout value in seconds.
	 * @see #setLoginTimeout(String)
	 * @see #getLoginTimeout()
	 */
	void setLoginTimeout(int loginTimeout) {
		if (loginTimeout < 0) {
			sqlExceptionMessage_ = "Incorrect value for loginTimeout set: " + loginTimeout + ".";
			loginTimeout_ = 1800;
		} else {
			loginTimeout_ = loginTimeout;
		}
	}

	/**
	 * Returns the login timeout value set for the current Type 4 connection.
	 * 
	 * @return the login timeout value in seconds.
	 * @see #setLoginTimeout(int)
	 * @see #setLoginTimeout(String)
	 */
	int getLoginTimeout() {
		return loginTimeout_;
	}

	// -----------------------------------------------------------------

	/***************************************************************************
	 * Start comment out seciont ************* * Sets the close connection
	 * timeout in seconds for the Type 4 connection. The default close
	 * connection timeout value is set to 60 seconds.
	 * 
	 * @param closeConnectionTimeout
	 *            The close connection timeout value in seconds.
	 * @see #setCloseConnectionTimeout(int)
	 * @see #getCloseConnectionTimeout()
	 * 
	 * public void setCloseConnectionTimeout(String closeConnectionTimeout) {
	 * int closeConnectionTo = 60; if (closeConnectionTimeout != null) { try {
	 * closeConnectionTo = Integer.parseInt(closeConnectionTimeout); } catch
	 * (NumberFormatException ex) { sqlExceptionMessage_ = "Incorrect value for
	 * closeConnectionTimeout set: " + closeConnectionTimeout + ex.getMessage();
	 * closeConnectionTo = 60; } } setCloseConnectionTimeout(closeConnectionTo); } *
	 * Sets the close connection timeout in seconds for the Type 4 connection.
	 * The default close connection timeout value is set to 60 seconds.
	 * @param closeConnectionTimeout
	 *            The close connection timeout value in seconds.
	 * @see #setCloseConnectionTimeout(String)
	 * @see #getCloseConnectionTimeout()
	 * 
	 * public void setCloseConnectionTimeout(int closeConnectionTimeout) { if
	 * (closeConnectionTimeout < 0) { sqlExceptionMessage_ = "Incorrect value
	 * for closeConnectionTimeout set: " + closeConnectionTimeout + ".";
	 * closeConnectionTimeout_ = 60; } else { closeConnectionTimeout_ =
	 * closeConnectionTimeout; } } * Returns the close connection timeout value
	 * set for the current Type 4 connection.
	 * @return the close connection timeout value in seconds.
	 * @see #setCloseConnectionTimeout(int)
	 * @see #setCloseConnectionTimeout(String)
	 * 
	 * public int getCloseConnectionTimeout() { return closeConnectionTimeout_; }
	 **************************************************************************/

	// -----------------------------------------------------------------
	/**
	 * Sets the network timeout in seconds for the Type 4 connection. The
	 * default network timeout value is set to infinity seconds.
	 * 
	 * @param networkTimeout
	 *            The network timeout value in seconds.
	 * @see #setNetworkTimeout(String)
	 * @see #getNetworkTimeout()
	 */
	void setNetworkTimeout(String networkTimeout) {
		int networkTo = 0;
		if (networkTimeout != null) {
			try {
				networkTo = Integer.parseInt(networkTimeout);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for networkTimeout set: " + networkTimeout + ex.getMessage();
				networkTo = 0;
			}
		}
		setNetworkTimeout(networkTo);
	}

	/**
	 * Sets the network timeout in seconds for the Type 4 connection. The
	 * default network timeout value is set to infinity seconds.
	 * 
	 * @param networkTimeout
	 *            The network timeout value in seconds.
	 * @see #setNetworkTimeout(int)
	 * @see #setNetworkTimeout()
	 */
	void setNetworkTimeout(int networkTimeout) {
		if (networkTimeout < 0) {
			sqlExceptionMessage_ = "Incorrect value for networkTimeout set: " + networkTimeout + ".";
			networkTimeoutInMillis_ = DEFAULT_NETWORK_TIMEOUT_IN_MILLIS;
		} else {
			networkTimeoutInMillis_ = networkTimeout * 1000;
		}
	}

	void setNetworkTimeoutInMillis(int networkTimeoutInMillis) {
		networkTimeoutInMillis_ = networkTimeoutInMillis;
	}

	/**
	 * Returns the network timeout value set for the current Type 4 connection.
	 * 
	 * @return the network timeout value in seconds.
	 * @see #getNetworkTimeout(int)
	 * @see #getNetworkTimeout(String)
	 */
	int getNetworkTimeout() {
		return networkTimeoutInMillis_ / 1000;
	}

        int getNetworkTimeoutInMillis() {
            return networkTimeoutInMillis_;
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
	void setConnectionTimeout(String connectionTimeout) {
		int tmpTimeout = -1;
		if (connectionTimeout != null) {
			try {
				tmpTimeout = Integer.parseInt(connectionTimeout);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for connectionTimeout set: " + connectionTimeout + ". "
						+ ex.getMessage();
				tmpTimeout = -1;
			}
		}
		setConnectionTimeout(tmpTimeout);
	}

	void setClipVarchar(String clipVarchar) {
		short tmp = 0;
		if (clipVarchar != null) {
			try {
				tmp = Short.parseShort(clipVarchar);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for clipVarchar set: " + clipVarchar + ". "
						+ ex.getMessage();
				tmp = 0;
			}
		}
		setClipVarchar(tmp);
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
	void setConnectionTimeout(int connectionTimeout) {
        if (connectionTimeout > Short.MAX_VALUE) {
            sqlExceptionMessage_ = "Incorrect value for connectionTimeout set: [" + connectionTimeout
                    + "]. Max value is: [" + Short.MAX_VALUE + "]";
        }

		if (connectionTimeout < 0) {
			/*
			 * sqlExceptionMessage_ = "Incorrect value for connectionTimeout
			 * set: " + connectionTimeout + ". ";
			 */
			connectionTimeout_ = -1;
		} else {
			connectionTimeout_ = connectionTimeout;
		}
	}
	void setClipVarchar(short clipVarchar) {
		clipVarchar_=clipVarchar;
	}
	/**
	 * Sets the max idle time value for the Type 4 connection. The default is
	 * set to 0 (no timeout). Negative values are treated as 0.
	 * 
	 * @param maxIdleTime
	 *            The timeout value in seconds.
	 * @see #setMaxIdleTime(int)
	 */
	void setMaxIdleTime(String maxIdleTime) {
		int tmpTimeout = DEFAULT_MAX_IDLE_TIMEOUT;
		if (maxIdleTime != null) {
			try {
				tmpTimeout = Integer.parseInt(maxIdleTime);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for maxIdleTime set: " + maxIdleTime + ". " + ex.getMessage();
				tmpTimeout = DEFAULT_MAX_IDLE_TIMEOUT;
			}
		}
		setMaxIdleTime(tmpTimeout);
	}

	/**
	 * Sets the max idle time value for the Type 4 connection. The default is
	 * set to 0 (no timeout). Negative values are treated as 0.
	 * 
	 * @param maxIdleTime
	 *            The timeout value in seconds.
	 * @see #setMaxIdleTime(String)
	 */
	void setMaxIdleTime(int maxIdleTime) {
		if (maxIdleTime < 0) {
			maxIdleTime_ = DEFAULT_MAX_IDLE_TIMEOUT;
		} else {
			maxIdleTime_ = maxIdleTime;
		}
	}

	/*
	 * Returns the connection timeout value associated with this Type 4
	 * connection. @return The connection timeout value in seconds.
	 * 
	 * @see #setConnectionTimeout(int)
	 */

	int getConnectionTimeout() {
		return connectionTimeout_;
	}
	short getClipVarchar() {
		return clipVarchar_;
	}

	/**
	 * Returns the max idle time value associated with this Type 4 connection.
	 * 
	 * @return The connection timeout value in seconds.
	 * @see #setMaxIdleTime(int)
	 */
	int getMaxIdleTime() {
		return maxIdleTime_;
	}

	/**
	 * Sets the logging level for the current Type 4 connection. Default value
	 * is OFF. Other valid values are SEVERE (highest value) WARNING INFO CONFIG
	 * FINE FINER FINEST (lowest value).
	 * 
	 * @param level
	 *            logging level.
	 * @see #getT4LogLevel()
	 * @see java.util.logging.Level
	 */
	void setT4LogLevel(String level) {
		t4LogLevel = Level.parse("OFF");
		if (level != null) {
			try {
				t4LogLevel = Level.parse(level);
			} catch (Exception ex) {

				SQLException se = TrafT4Messages.createSQLException(null, null, "problem_with_logging", ex.getMessage());
				sqlExceptionMessage_ = se.getMessage();
				// throw se;
				// RuntimeException rte = new RuntimeException(se.getMessage(),
				// se);
				// throw rte;
				// sqlExceptionMessage_ = "Incorrect value for T4LogLevel set: "
				// +
				// level + ". " + ex.getMessage();
				// t4LogLevel = Level.parse("INFO");
			}
		}
	}

	/**
	 * Returns the Type 4 log level associated with the current Type 4
	 * connection. The value returned must one of the following strings. SEVERE
	 * (highest value) WARNING INFO CONFIG FINE FINER FINEST (lowest value).
	 * 
	 * @return <code>java.util.logging.Level</code> associated with the
	 *         current Type 4 connection.
	 * @see #setT4LogLevel(String)
	 * @see java.util.logging.Level
	 */
	Level getT4LogLevel() {
		return t4LogLevel;
	}

	/**
	 * Sets the location of the file to which the logging is to be done.
	 * Changing this location after making a connection has no effect; because
	 * the Type 4 reads this property before the connection is made. The default
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
	void setT4LogFile(String t4LogFile) {
		if (t4LogFile != null) {
			T4LogFile_ = t4LogFile;
		} else {
			T4LogFile_ = t4GlobalLogFile;
		}
	}

	/**
	 * Returns the Type 4 log file location associated with the current Type 4
	 * connection.
	 * 
	 * @return The Type 4 log file location.
	 * @see #setT4LogFile(String)
	 */
	String getT4LogFile() {
		return T4LogFile_;
	}

	String getT4GlobalLogFile() {
		return t4GlobalLogFile;
	}

	void setT4GlobalLogFile(String lgf) {
		t4GlobalLogFile = lgf;
	}

	// --------------------------------------------
	FileHandler getT4GlobalLogFileHandler() {
		return t4GlobalLogFileHandler;
	} // end getT4GlobalLogFileHandler

	// --------------------------------------------
	void setT4GlobalLogFileHandler(Object fh) {
		t4GlobalLogFileHandler = (FileHandler) fh;
	}

	// --------------------------------------------

	Logger getLogger() {
		return logger;
	}

	void setLogger(Object log) {
		if (log != null) {
			logger = (Logger) log;
			t4Logger_ = logger;
		} else {
			logger = t4GlobalLogger;
			t4Logger_ = logger;
		}
	}

	// --------------------------------------------
	/**
	 * This method will return the log file handlder It also has the side effect
	 * of creating a log file handler if one doesn't exist.
	 */
	FileHandler getT4LogFileHandler() {
		try {
			if (t4LogFileHandler_ == null) {
				if (T4LogFile_.equals(t4GlobalLogFile)) {
					t4LogFileHandler_ = t4GlobalLogFileHandler;
				} else {
					t4LogFileHandler_ = new FileHandler(T4LogFile_);
					t4LogFileHandler_.setFormatter(new T4LogFormatter());
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return t4LogFileHandler_;
	} // end getT4LogFileHandler

	// --------------------------------------------
	void setT4LogFileHandler(Object fh) {
		t4LogFileHandler_ = (FileHandler) fh;
	}

	// ----------------------------------------------------------

	/**
	 * Returns the <code>PrintWriter</code> object associated with the current
	 * Type 4 connection.
	 * 
	 * @return <code>java.io.PrintWriter</code> object associated with current
	 *         connection.
	 * @throws SQLException
	 *             when error occurs.
	 * @see #setLogWriter(PrintWriter)
	 * @see javax.sql.ConnectionPoolDataSource
	 */
	public PrintWriter getLogWriter() throws SQLException {
		return logWriter_;
	}

	/**
	 * Sets the <code>PrintWriter</code> object for the current Type 4
	 * connection.
	 * 
	 * @param printWriter
	 *            For the current Type 4 logging.
	 * @throws SQLException
	 *             when error occurs.
	 * @see #getLogWriter()
	 * @see javax.sql.ConnectionPoolDataSource
	 */
	public void setLogWriter(PrintWriter printWriter) throws SQLException {
		logWriter_ = printWriter;
	}

	// properties queryTimeout_ for future use.
	// setter/getter methods for queryTimeout. These methods are not public YET.
	/**
	 * @param queryTimeout
	 *            Sets the query timeout value in seconds. For future use only
	 *            this property is not supported in the current release.
	 */
	void setQueryTimeout(String queryTimeout) {
		short tmpQTimeOut = 0;
		if (queryTimeout != null) {
			try {
				tmpQTimeOut = Short.parseShort(queryTimeout);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for queryTimeout set: " + queryTimeout + ex.getMessage();
				tmpQTimeOut = 0;
			}
		}
		setQueryTimeout(tmpQTimeOut);
	}

	/**
	 * @param queryTimeout
	 *            Sets the query timeout value in seconds. For future use only
	 *            this property is not supported in the current release.
	 */
	void setQueryTimeout(short queryTimeout) {
		if ((queryTimeout *1000) > networkTimeoutInMillis_)
			queryTimeout = (short)(networkTimeoutInMillis_ % 1000);
		queryTimeout_ = queryTimeout;
	}

	/**
	 * @return queryTimeOut value in seconds. For future use only this property
	 *         is not supported in the current release.
	 */
	short getQueryTimeout() {
		return queryTimeout_;
	}

	void setIgnoreCancel(String ignoreCancel)
	{
		if (ignoreCancel != null)
			ignoreCancel_ = Boolean.parseBoolean(ignoreCancel);
		else
			ignoreCancel_ = true;
	}

        void setIgnoreCancel(boolean ignoreCancel)
	{
		ignoreCancel_ = ignoreCancel;
	}

	boolean getIgnoreCancel()
	{
		return ignoreCancel_;
	}

	void setActiveTimeBeforeCancel(String activeTimeBeforeCancel) {
		int tmpActiveTimeBeforeCancel = 0;
		if (activeTimeBeforeCancel != null) {
			try {
				tmpActiveTimeBeforeCancel = Integer.parseInt(activeTimeBeforeCancel);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for activeTimeBeforeCancel set: " + activeTimeBeforeCancel + ex.getMessage();
				tmpActiveTimeBeforeCancel = -1;
			}
		}
		setActiveTimeBeforeCancel(tmpActiveTimeBeforeCancel);
	}

	void setActiveTimeBeforeCancel(int activeTimeBeforeCancel) 
	{
		if (activeTimeBeforeCancel < networkTimeoutInMillis_)
			activeTimeBeforeCancel = networkTimeoutInMillis_;
		activeTimeBeforeCancel_ = activeTimeBeforeCancel;
	}

	int getActiveTimeBeforeCancel() 
	{
		return activeTimeBeforeCancel_;
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
		fetchBufferSize_ = 512;
		if (fetchBufferSize > 512) {
			fetchBufferSize_ = fetchBufferSize;
		}
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
		short setFetchSizeVal = 4;
		if (fetchBufferSize != null) {
			try {
				setFetchSizeVal = Short.parseShort(fetchBufferSize);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect fetchBufferSize value set: " + setFetchSizeVal + ". "
						+ ex.getMessage();
				setFetchSizeVal = 1;
			}
		}
		setFetchBufferSize(setFetchSizeVal);
	}

	/**
	 * Returns the size of the fetch buffer.
	 * 
	 * @see #setFetchBufferSize(short)
	 * @see #setFetchBufferSize(String)
	 */
	short getFetchBufferSize() {
		return fetchBufferSize_;
	}

	/**
	 * Configure to use array binding feature for the Type 4 connection. Default
	 * value is true.
	 * 
	 * @param useArrayBinding
	 */
	void setUseArrayBinding(String useArrayBinding) {
		boolean boolUseArrayBinding = false;
		if (useArrayBinding != null) {
			if (useArrayBinding.equalsIgnoreCase("true")) {
				boolUseArrayBinding = true;
			}
		}
		setUseArrayBinding(boolUseArrayBinding);
	}

	/**
	 * Configure to use array binding feature for the Type 4 connection. Default
	 * value is true.
	 * 
	 * @param useArrayBinding
	 */
	void setUseArrayBinding(boolean useArrayBinding) {
		useArrayBinding_ = useArrayBinding;
	}

	/**
	 * Return whether the connection uses Database array binding feature.
	 * 
	 * @return useArrayBinding boolean flag indicates Database RowSet (array
	 *         binding) feature is used or not.
	 */
	boolean getUseArrayBinding() {
		return useArrayBinding_;
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
		batchRecovery_ = batchRecovery;
	}

	/**
	 * Return whether the Type 4 connection is configured to continue batch
	 * processing of next commands even after errors.
	 * 
	 * @return batchRecovery
	 */
	boolean getBatchRecovery() {
		return batchRecovery_;
	}

	/**
	 * Sets the buffer size in bytes used by the transport layer between Type 4
	 * client classes and NDCS server on the NSK system.
	 * 
	 * @param transportBufferSize
	 *            set the transport buffer size in bytes for the current Type 4
	 *            connection. Default value is 32000.
	 */
	/*
	 * public void setTransportBufferSize(String transportBufferSize) { short
	 * tmpbuf = 32000; if (transportBufferSize != null) { try { tmpbuf = (new
	 * Short(transportBufferSize)).shortValue(); } catch (Exception ex) {
	 * sqlExceptionMessage_ = "Incorrect value for transportBufferSize set: " +
	 * transportBufferSize + ex.getMessage(); tmpbuf = 32000; } }
	 * setTransportBufferSize(tmpbuf); }
	 */

	/**
	 * Sets the buffer size in bytes used by the transport layer between Type 4
	 * client classes and NDCS server on the NSK system. Transport buffer
	 * minimum acceptable size is 2000 bytes. If the value set is below 2000
	 * default buffer size of 3200 is set instead.
	 * 
	 * @param transportBufferSize
	 *            set the transport buffer size in bytes for the current
	 *            connection. Default value for the driver is 32000 bytes.
	 */
	/*
	 * public void setTransportBufferSize(short transportBufferSize) { if
	 * (transportBufferSize < 2000) { transportBufferSize_ = 32000; } else {
	 * transportBufferSize_ = transportBufferSize; } }
	 */

	/**
	 * Returns the buffer size in bytes used by the transport layer between Type
	 * 4 client classes and NDCS server on the NSK system.
	 * 
	 * @return The transport buffer size used by the current Type 4 connection.
	 */
	/*
	 * public short getTransportBufferSize() { return transportBufferSize_; }
	 */

	/**
	 * @return any sql exception associated while setting the properties on this
	 *         Type 4 connection. This mthod is accessed by InterfaceConnection
	 *         to check if there is any SQL error setting the Type 4 properties.
	 */
	String getSQLException() {
		// System.out.println("sqlExceptionMessage_ = " + sqlExceptionMessage_);
		return sqlExceptionMessage_;
	}

    public int getLobChunkSize() {
        return lobChunkSize_;
    }

    public void setLobChunkSize(int lobChunkSize_) {
        this.lobChunkSize_ = lobChunkSize_;
    }

    public void setLobChunkSize(String val) {
        this.lobChunkSize_ = 10;
        if (val != null) {
            try {
                this.lobChunkSize_ = Integer.parseInt(val);
            } catch (NumberFormatException ex) {
                sqlExceptionMessage_ = "Incorrect value for setLobChunkSize set: " + val + ex.getMessage();
                this.lobChunkSize_ = 10;
            }
        }
    }

    public boolean getUseLobHandle() {
        return useLobHandle_;
    }

    public void setUseLobHandle(boolean useLobHandle) {
        this.useLobHandle_ = useLobHandle;
    }

    public void setUseLobHandle(String val) {
        if (val != null) {
            setUseLobHandle(Boolean.parseBoolean(val));
        }
        else {
            setUseLobHandle(false);
        }
    }

	/**
	 * Returns the rounding mode set for the driver as an Integer value with one
	 * of the following values. static int ROUND_CEILING Rounding mode to round
	 * towards positive infinity. static int ROUND_DOWN Rounding mode to round
	 * towards zero. static int ROUND_FLOOR Rounding mode to round towards
	 * negative infinity. static int ROUND_HALF_DOWN Rounding mode to round
	 * towards "nearest neighbor" unless both neighbors are equidistant, in
	 * which case round down. static int ROUND_HALF_EVEN Rounding mode to round
	 * towards the "nearest neighbor" unless both neighbors are equidistant, in
	 * which case, round towards the even neighbor. static int ROUND_HALF_UP
	 * Rounding mode to round towards "nearest neighbor" unless both neighbors
	 * are equidistant, in which case round up. static int ROUND_UNNECESSARY
	 * Rounding mode to assert that the requested operation has an exact result,
	 * hence no rounding is necessary. static int ROUND_UP Rounding mode to
	 * round away from zero.
	 */
	int getRoundingMode() {
		return roundMode_;
	}

	/**
	 * This method sets the round mode behaviour for the driver.
	 * 
	 * @param roundMode
	 *            Integer value with one of the following values: static int
	 *            ROUND_CEILING Rounding mode to round towards positive
	 *            infinity. static int ROUND_DOWN Rounding mode to round towards
	 *            zero. static int ROUND_FLOOR Rounding mode to round towards
	 *            negative infinity. static int ROUND_HALF_DOWN Rounding mode to
	 *            round towards "nearest neighbor" unless both neighbors are
	 *            equidistant, in which case round down. static int
	 *            ROUND_HALF_EVEN Rounding mode to round towards the "nearest
	 *            neighbor" unless both neighbors are equidistant, in which
	 *            case, round towards the even neighbor. static int
	 *            ROUND_HALF_UP Rounding mode to round towards "nearest
	 *            neighbor" unless both neighbors are equidistant, in which case
	 *            round up. static int ROUND_UNNECESSARY Rounding mode to assert
	 *            that the requested operation has an exact result, hence no
	 *            rounding is necessary. static int ROUND_UP Rounding mode to
	 *            round away from zero. The default behaviour is to do
	 *            ROUND_HALF_EVEN.
	 */
	void setRoundingMode(String roundMode) {
		roundMode_ = Utility.getRoundingMode(roundMode);
	}

	/**
	 * This method sets the round mode behaviour for the driver.
	 * 
	 * @param roundMode
	 *            Integer value with one of the following values: static int
	 *            ROUND_CEILING Rounding mode to round towards positive
	 *            infinity. static int ROUND_DOWN Rounding mode to round towards
	 *            zero. static int ROUND_FLOOR Rounding mode to round towards
	 *            negative infinity. static int ROUND_HALF_DOWN Rounding mode to
	 *            round towards "nearest neighbor" unless both neighbors are
	 *            equidistant, in which case round down. static int
	 *            ROUND_HALF_EVEN Rounding mode to round towards the "nearest
	 *            neighbor" unless both neighbors are equidistant, in which
	 *            case, round towards the even neighbor. static int
	 *            ROUND_HALF_UP Rounding mode to round towards "nearest
	 *            neighbor" unless both neighbors are equidistant, in which case
	 *            round up. static int ROUND_UNNECESSARY Rounding mode to assert
	 *            that the requested operation has an exact result, hence no
	 *            rounding is necessary. static int ROUND_UP Rounding mode to
	 *            round away from zero. The default behaviour is to do
	 *            ROUND_HALF_EVEN.
	 */
	void setRoundingMode(int roundMode) {
		roundMode_ = Utility.getRoundingMode(roundMode);
	}

	// ----------------------------------------------------------
	void setConnectionID(String connID) {
		connectionID_ = connID;
	}

	String getConnectionID() {
		return connectionID_;
	}

	// ----------------------------------------------------------
	void setDialogueID(String diaID) {
		dialogueID_ = diaID;
	}

	String getDialogueID() {
		return dialogueID_;
	}

	// ----------------------------------------------------------
	void setNcsMajorVersion(short majorVer) {
		ncsMajorVersion_ = majorVer;
	}

	void setNcsMajorVersion(String majorVer) {
		short mv = 0;

		if (majorVer != null) {
			try {
				mv = Short.parseShort(majorVer);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for NDCS major version set: " + majorVer + ex.getMessage();
				mv = 0;
			}
		}
		setNcsMajorVersion(mv);
		ncsMajorVersion_ = mv;
	}

	short getNcsMajorVersion() {
		return ncsMajorVersion_;
	}

	// ----------------------------------------------------------
	void setNcsMinorVersion(short minorVer) {
		ncsMinorVersion_ = minorVer;
	}

	void setNcsMinorVersion(String minorVer) {
		short mv = 0;

		if (minorVer != null) {
			try {
				mv = Short.parseShort(minorVer);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for NDCS minor version set: " + minorVer + ex.getMessage();
				mv = 0;
			}
		}
		setNcsMinorVersion(mv);
		ncsMinorVersion_ = mv;
	}

	short getNcsMinorVersion() {
		return ncsMinorVersion_;
	}

	void setSqlmxMajorVersion(short majorVer) {
		sqlmxMajorVersion_ = majorVer;
	}

	void setSqlmxMajorVersion(String majorVer) {
		short mv = 0;

		if (majorVer != null) {
			try {
				mv = Short.parseShort(majorVer);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for Database major version set: " + majorVer
						+ ex.getMessage();
				mv = 0;
			}
		}
		setSqlmxMajorVersion(mv);
	}

	public void setSPJEnv(String SPJEnv) {
		if (SPJEnv == null) {
			SPJEnv_ = false;
		} else {
			SPJEnv_ = (SPJEnv.equalsIgnoreCase("true"));
		}
	}

	public void setSPJEnv(boolean SPJEnv) {
		SPJEnv_ = SPJEnv;
	}

	public boolean getSPJEnv() {
		return SPJEnv_;
	}

	public void setKeepRawFetchBuffer(String keep) {
		if (keep == null) {
			keepRawFetchBuffer_ = false;
		} else {
			keepRawFetchBuffer_ = (keep.equalsIgnoreCase("true"));
		}
	}

	public void setKeepRawFetchBuffer(boolean keep) {
		keepRawFetchBuffer_ = keep;
	}

	public boolean getKeepRawFetchBuffer() {
		return keepRawFetchBuffer_;
	}

	short getSqlmxMajorVersion() {
		return sqlmxMajorVersion_;
	}

	public void setCpuToUse(String cpu) {
		if (cpu == null) {
			cpuToUse_ = -1;
		} else {
			cpuToUse_ = Short.parseShort(cpu);
		}
	}

	public void setCpuToUse(short cpu) {
		cpuToUse_ = cpu;
	}

	public short getCpuToUse() {
		return cpuToUse_;
	}

	public void setSessionName(String name) {
		if (name == null)
			sessionName = "";
		else {
			sessionName = name;
		}
	}

	public String getSessionName() {
		return sessionName;
	}

	public void setReplacementString(String str) {
		if (str == null) {
			replacementString_ = "?";
		} else {
			replacementString_ = str;
		}
	}

	public String getReplacementString() {
		return replacementString_;
	}

	void setISO88591(String lang) {
		if (lang == null) {
			ISO88591_ = InterfaceUtilities.getCharsetName(InterfaceUtilities.SQLCHARSETCODE_ISO88591);
		} else {
			ISO88591_ = lang;
		}
	}

	String getISO88591() {
		return ISO88591_;
	}

	public String getRoleName() {
		return _roleName;
	}

	public void setRoleName(String roleName) {
		if (roleName != null && roleName.length() > 0) {
			if (roleName.length() > 128)
				sqlExceptionMessage_ = "Invalid RoleName.  Max length: 128";
			}
		
		this._roleName = roleName;
	}

	public String getApplicationName() {
		return _applicationName;
	}

	public void setApplicationName(String applicationName) {
		if (applicationName == null || applicationName.length() == 0)
			this._applicationName = "FASTJDBC";
		else
			this._applicationName = applicationName;
	}
	
	public void setCertificateFileActive(String file) {
		_certificateFileActive = file;
	}
	
	public String getCertificateFileActive() {
		return _certificateFileActive;
	}
	
	public void setCertificateFile(String file) {
		_certificateFile = file;
	}
	
	public String getCertificateFile() {
		return _certificateFile;
	}
	
	public void setCertificateDir(String dir) {
		_certificateDir = dir;
	}
	
	public String getCertificateDir() {
		return _certificateDir;
	}
	
	public void setDelayedErrorMode(String mode) {
		_delayedErrorMode = Boolean.valueOf(mode).booleanValue();
	}

	public void setDelayedErrorMode(boolean mode) {
		_delayedErrorMode = mode;
	}

	public boolean getDelayedErrorMode() {
		return _delayedErrorMode;
	}
	
	public void setCompression(String compression) {
		_compression = Boolean.valueOf(compression).booleanValue();
	}

	public void setCompression(boolean compression) {
		_compression = compression;
	}

	public boolean getCompression() {
		return _compression;
	}
	
	public void setKeepAlive(String val) {
		if(val == null) 
		{
			_keepAlive = true;
		}
		else 
		{
			_keepAlive = Boolean.valueOf(val).booleanValue();
		}
	}

	public void setKeepAlive(boolean val) {
		_keepAlive = val;
	}

	public boolean getKeepAlive() {
		return _keepAlive;
	}

    public void setTcpNoDelay(String val) {
        if(val == null) {
            _tcpNoDelay = true;
        }
        else {
            _tcpNoDelay = Boolean.valueOf(val).booleanValue();
        }
    }

    public void setTcp_NoDelay(boolean val) {
        _tcpNoDelay = val;
    }

    public boolean getTcpNoDelay() {
        return _tcpNoDelay;
    }

	public void setTokenAuth(String val) {
		if(val == null) 
		{
			_tokenAuth = true;
		}
		else 
		{
			_tokenAuth = Boolean.valueOf(val).booleanValue();
		}
	}

	public void setTokenAuth(boolean val) {
		_tokenAuth = val;
	}

	public boolean getTokenAuth() {
		return _tokenAuth;
	}

	boolean getSessionToken() {
		return this._sessionToken;
	}

	void setSessionToken(boolean sessionToken) {
		this._sessionToken = sessionToken;
	}

	void setSessionToken(String sessionToken) {
		setSessionToken(Boolean.valueOf(sessionToken).booleanValue());
	}

	boolean getFetchAhead() {
		return this._fetchAhead;
	}

	void setFetchAhead(boolean fetchAhead) {
		this._fetchAhead = fetchAhead;
	}

	void setFetchAhead(String fetchAhead) {
		setFetchAhead(Boolean.valueOf(fetchAhead).booleanValue());
	}

	// ----------------------------------------------------------
	void setSqlmxMinorVersion(short minorVer) {
		sqlmxMinorVersion_ = minorVer;
	}

	void setSqlmxMinorVersion(String minorVer) {
		short mv = 0;

		if (minorVer != null) {
			try {
				mv = Short.parseShort(minorVer);
			} catch (NumberFormatException ex) {
				sqlExceptionMessage_ = "Incorrect value for Database minor version set: " + minorVer
						+ ex.getMessage();
				mv = 0;
			}
		}
		setSqlmxMinorVersion(mv);
	}

	short getSqlmxMinorVersion() {
		return sqlmxMinorVersion_;
	}

	// ----------------------------------------------------------
	BigDecimal getNcsVersion() {
		String minor = new Short(getNcsMinorVersion()).toString();
		String major = new Short(getNcsMajorVersion()).toString();
		BigDecimal bigD = new BigDecimal(major + "." + minor);
		return bigD;
	}

	// ----------------------------------------------------------
	void setServerID(String serID) {
		serverID_ = serID;
	}

	String getServerID() {
		return serverID_;
	}

	/**
	 * Sets the language to use for the error messages.
	 * 
	 * @param language
	 *            Sets the language for the current Type 4 connection. The
	 *            default language used by the Type 4 driver is American
	 *            English.
	 * @see #getLanguage()
	 */
	void setLanguage(String language) {
		if (language == null) {
			locale_ = Locale.getDefault();
		} else {
			locale_ = new Locale(language, "", "");
		}
		language_ = locale_.getLanguage();
	}

	/**
	 * Returns the language in which to the Type 4 error messages will be
	 * returned to the application.
	 * 
	 * @return The language associated with the current Type 4 connection.
	 * @see #setLanguage(String language)
	 */
	String getLanguage() {
		return language_;
	}

	// ----------------------------------------------------------

	/**
	 * @return Reference object containing all the Type 4 connection properties.
	 *         The reference object can be used to register with naming
	 *         services.
	 */
	Reference addReferences(Reference ref) {
		ref.add(new StringRefAddr("dataSourceName", getDataSourceName()));
		ref.add(new StringRefAddr("serverDataSource", getServerDataSource()));
		ref.add(new StringRefAddr("description", getDescription()));
		String val = getCatalog();
		if (val != null) {
			ref.add(new StringRefAddr("catalog", val));
		}
		val = getSchema();
		if (val != null) {
			ref.add(new StringRefAddr("schema", val));

		}
		ref.add(new StringRefAddr("language", getLanguage()));
		ref.add(new StringRefAddr("maxPoolSize", Integer.toString(getMaxPoolSize())));
		ref.add(new StringRefAddr("minPoolSize", Integer.toString(getMinPoolSize())));
		ref.add(new StringRefAddr("initialPoolSize", Integer.toString(getInitialPoolSize())));
		ref.add(new StringRefAddr("maxStatements", Integer.toString(getMaxStatements())));

		ref.add(new StringRefAddr("connectionTimeout", Integer.toString(getConnectionTimeout())));
		ref.add(new StringRefAddr("maxIdleTime", Integer.toString(getMaxIdleTime())));
		ref.add(new StringRefAddr("loginTimeout", Integer.toString(getLoginTimeout())));
		// ref.add(new StringRefAddr("closeConnectionTimeout",
		// Integer.toString(getCloseConnectionTimeout())));
		ref.add(new StringRefAddr("networkTimeout", Integer.toString(getNetworkTimeout())));
		ref.add(new StringRefAddr("T4LogLevel", getT4LogLevel().toString()));
		ref.add(new StringRefAddr("T4LogFile", getT4LogFile()));

		ref.add(new StringRefAddr("url", getUrl()));
		ref.add(new StringRefAddr("user", getUser()));
		ref.add(new StringRefAddr("password", getPassword()));
		/*
		 * ref.add(new StringRefAddr("transportBufferSize",
		 * Short.toString(getTransportBufferSize())));
		 */
		/*
		 * ref.add(new StringRefAddr("useArrayBinding",
		 * Boolean.toString(getUseArrayBinding())));
		 */

		ref.add(new StringRefAddr("roundingMode", Integer.toString(getRoundingMode())));

		// propertiy queryTimeout_ for future use.
		ref.add(new StringRefAddr("queryTimeout", Integer.toString(getQueryTimeout())));
		ref.add(new StringRefAddr("ignoreCancel", Boolean.toString(getIgnoreCancel())));
		ref.add(new StringRefAddr("activeTimeBeforeCancelInSecs", Integer.toString(getActiveTimeBeforeCancel())));
		ref.add(new StringRefAddr("fetchBufferSize", Short.toString(this.getFetchBufferSize())));
		ref.add(new StringRefAddr("batchRecovery", Boolean.toString(this.getBatchRecovery())));
		return ref;
	}

	/**
	 * Instantiated by either <code>
	 * Class.forName("org.trafodion.jdbc.t4.T4Driver")</code>
	 * or by passing <code>-Djdbc.drivers=org.trafodion.jdbc.t4.T4Driver</code>
	 * property in the command line of the JDBC program.
	 */
	DriverPropertyInfo[] getPropertyInfo(String url, Properties defaults) throws SQLException {

		String level[] = { "OFF", "SEVERE", "WARNING", "INFO", "CONFIG", "FINE", "FINER", "FINEST" };
		String roundingMode[] = { "ROUND_CEILING", "ROUND_DOWN", "ROUND_UP", "ROUND_FLOOR", "ROUND_HALF_UP",
				"ROUND_UNNECESSARY", "ROUND_HALF_EVEN", "ROUND_HALF_DOWN", "ROUND_DOWN" };

		Properties props = new Properties(defaults);
		props.setProperty("url", url);

		// catalog setting
		DriverPropertyInfo[] propertyInfo = {
				setPropertyInfo("catalog", props, true, "Database catalog name", null),
				setPropertyInfo("schema", props, true, "Database schema name", null),
				setPropertyInfo("url", props, false, "jdbc:t4jdbc://<host>:<port>/:", null),
				setPropertyInfo("user", props, true, "NSK safeguard user name", null),
				setPropertyInfo("password", props, true, "NSK safeguard user password", null),
				setPropertyInfo("maxPoolSize", props, false, "Maximum connection pool size", null),
				setPropertyInfo("minPoolSize", props, false, "Minimum connection pool size", null),
				setPropertyInfo("initialPoolSize", props, false, "Initial connection pool size", null),
				setPropertyInfo("maxStatements", props, false, "Maximum statement pool size", null),
				setPropertyInfo("T4LogLevel", props, false, "Logging Level", level),
				setPropertyInfo("T4LogFile", props, false, "Logging file location", null),
				setPropertyInfo("loginTimeout", props, false, "Login time out in secs", null),
				setPropertyInfo("networkTimeout", props, false, "Network time out in secs", null),
				setPropertyInfo("connectionTimeout", props, false, "Connection time out in secs", null),
				setPropertyInfo("maxIdleTime", props, false, "Max idle time for a free pool connection in secs", null),
				setPropertyInfo("language", props, false, "Locale language to use", null),
				setPropertyInfo("serverDataSource", props, false, "NDCS data source name", null),
				setPropertyInfo("roundingMode", props, false, "Data rounding mode", roundingMode),
				setPropertyInfo("fetchBufferSize", props, false,
						"Value (in KB) for the size of the fetch buffer to be used when rows are fetched", null),
				setPropertyInfo("batchRecovery", props, false,
						"Continue batch processing of next commands even after errors", null) };

		return propertyInfo;

	}

	private DriverPropertyInfo setPropertyInfo(String name, Properties props, boolean required, String description,
			String[] choices) {
		String value = props.getProperty(name);
		DriverPropertyInfo propertyInfo = new DriverPropertyInfo(name, value);
		propertyInfo.required = required;
		propertyInfo.description = description;
		propertyInfo.choices = choices;
		return propertyInfo;
	}

	// ---------------------------------------------------------------
	static private Properties getPropertiesFileValues() {
		Properties values = null;
		String propsFile = System.getProperty("t4jdbc.properties");

		if (propsFile != null) {
			FileInputStream fis = null;
			try {
				fis = new FileInputStream(new File(propsFile));
				values = new Properties();
				values.load(fis);
			} catch (Exception ex) {
				fis = null;
				// sqlExceptionMessage_ = "Error while loading " +
				// prefix_ + "properties file: " + ex.getMessage();
			} finally {
				if (fis != null) {
					try {
						fis.close();
					} catch (IOException ioe) {
						// ignore
					}
				}
			}
		} // end if

		return values;
	} // end getPropertiesFileValues

	// -------------------------------------------------------------------------

    void setClientInfoProperties(Properties prop) {
        this.clientInfoProp = prop;
    }

    Properties getClientInfoProperties() {
        return this.clientInfoProp;
    }

    public String getClientCharset() {
        return clientCharset;
    }

    public void setClientCharset(String clientCharset) {
        this.clientCharset = clientCharset;
    }
}
