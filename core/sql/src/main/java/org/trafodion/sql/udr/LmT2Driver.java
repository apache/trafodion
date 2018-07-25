/**********************************************************************
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
**********************************************************************/
/* -*-Java-*-
 ******************************************************************************
 *
 * File:         LmT2Driver.java
 * Description:  A driver class servicing the following URLs
 *		 - "jdbc:t2jdbc:"    (Trafodion Type2 driver connection)
 *		 - "jdbc:default:connection"	(Default connection)
 *
 * Created:      03/14/2014
 * Language:     Java
 *
 ******************************************************************************
 */

package org.trafodion.sql.udr;

import java.sql.*;
import java.util.*;
import java.util.logging.Logger;
import java.net.InetAddress;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.util.logging.Logger;
/**
 * Description:  A driver class servicing the following URLs
 *  - "jdbc:sqlmx:"    (JDBC/MX Type 2 connection)
 *  - "jdbc:default:connection"	(Default connection)
 *    By default, Default Connections will be mapped to Type 4 connections.
 *    It is also possible that this class can be initialized to map
 *    default connection to Type2 connection. 
 *
 *    This driver is loaded by the UDR server and it's main purpose
 *    is to intercept calls to the getConnection() method calls from
 *    a Java Stored procedure when using the URLs "jdbc:sqlmx:" or
 *    "jdbc:default:connection". This is done so that the Connection
 *    objects can be created with property values as dictated by the
 *    UDR server. 
 *
 *    The driver performs the following tasks:
 *    - First gets and stores the object reference of the currently
 *    loaded Type 2 JDBC/MX driver
 *    - De-registers the Type 2 JDBC/MX driver
 *    - Registers itself as the driver servicing the urls
 *    "jdbc:sqlmx:" and "jdbc:default:connection"
 *    - The connect() method services connection requests from user
 *    methods. In response, connect() sets connection property values
 *    as needed and passes "jdbc:sqlmx:" requests to the Type 2 JDBC/MX
 *    driver and "jdbc:default:connection" requests to the Type 4
 *    JDBC Driver.
 **/

public class LmT2Driver implements java.sql.Driver
{
  static Driver jdbcMxDrvr_ = null;
  static String type2URL_ = null;
  static String UDRcatalog_ = "";
  static String UDRschema_ = "";
  static String maxStmts_ = "";
  static boolean useSqlmxUDRCatSch_ = false;

  // By default, default conn gets mapped to Type 4 conn
  static boolean mapDefaultConnToType2_ = false;

  // The following are used only for Type 4 Connections.
  static String type4URL_ = null;
  static String dataSourceName_ = null;
  static String userName_ = null;
  static String password_ = null;
  static final int DEFAULT_MXCS_PORT_NUMBER = 18650;

  static String[] acceptedURLs = { "jdbc:t2jdbc:",
				   "jdbc:default:connection" };
  static  Method joinUDRTransMethodId_ = null;
  
  // The following static values are defined after 'enum ComRoutineTransactionAttributes'
  // in common/ComSmallDefs.h file. getTransactionAttrs() returns one of
  // these values.
  static int UNKNOWN_ROUTINE_TRANSACTION_ATTRIBUTE = 0;
  static int NO_TRANSACTION_REQUIRED = 1;
  static int TRANSACTION_REQUIRED = 2;

  // Native method that returns the Transaction Attributes of the SPJ
  native public static int getTransactionAttrs();

  // The following static values are defined after 'enum ComRoutineSQLAccess'
  // in common/ComSmallDefs.h file. getSqlAccessMode() returns one of
  // these values.
  static int UNKNOWN_ROUTINE_SQL_ACCESS = 0;
  static int NO_SQL = 1;
  static int CONTAINS_SQL = 2;
  static int READS_SQL = 3;
  static int MODIFIES_SQL = 4;

  // Native method that returns the SQL Access mode of the SPJ
  native public static int getSqlAccessMode();

  // Native method to indicate to the LM C++ layer that a 
  // default Connection has been created
  native public static void addConnection(Object conn);

  // Native method that returns the id of the transaction started by 
  // executor and passed down to Type 4 JDBC driver and MXOSRVR
  native public static long getTransId();

  // The init() method below is not part of the java.sql.Driver interface
  // and is specific to this driver's implementation.
  // This method performs the static initialization for this class and 
  // replaces the static code block of the driver. This is done to make
  // error handling easier in the native code loading this driver.
  public static void init(boolean useUDRCatSch,
		          boolean jdbcType2Enabled,
		          boolean mapDefConnToType2,
			  String hostName,
			  String userName,
			  String userPassword,
			  String dataSourceName,
		          int portNumber)
      throws SQLException, java.net.UnknownHostException, Exception
  {
    type2URL_ = "jdbc:t2jdbc:";

    if (mapDefConnToType2)
      mapDefaultConnToType2_ = true;

    if (jdbcType2Enabled)
    {
      // Get the object reference of the currently loaded JDBC/MX T2 driver
      jdbcMxDrvr_ = DriverManager.getDriver( "jdbc:t2jdbc:" );

      if( jdbcMxDrvr_ == null ) {
          String msg = "The SQL/MX language manager encountered an unexpected"
  		      + " error trying to access the JDBC/MX driver."
		      + " The DriverManager.getDriver() method returned a"
		      + " null value.";
        throw new SQLException( msg );
      }

      // De-register the JDBC/MX T2 driver if it is loaded
      DriverManager.deregisterDriver( jdbcMxDrvr_ );
    }

    // Get the system property value (if any) for maxStatements.
    // This is used only with the default connection url 
    if ( (maxStmts_ = System.getProperty( "jdbcmx.maxStatements" )) == null )
      maxStmts_ = System.getProperty( "maxStatements" );

    // Create the driver & register it with DriverManager
    LmT2Driver d = new LmT2Driver();
    useSqlmxUDRCatSch_ = useUDRCatSch;

    DriverManager.registerDriver( d );
  }

  // The methods from here on are all part of the java.sql.Driver interface

  public boolean acceptsURL( String url ) 
      throws SQLException
  {
    for( int i = 0; i < acceptedURLs.length; i++ ) {
      if( acceptedURLs[ i ].equals( url ) )
      {
        if (url.equals("jdbc:t2jdbc:"))
	{
	  // If the requested URL is Type 2, then return TRUE
	  // only if we have loaded Type2 driver.
          return (jdbcMxDrvr_ != null) ? true : false;
	}
	else
          return true;
      }
    }

    return false;
  }

  // similar to acceptsURL, but a static method that checks for substrings
  // acceptsURL is used by the DriverManager, this static method is used
  // internally by Trafodion (JDBC TMUDF)
  public static boolean checkURL( String url ) 
  {
    for (int i = 0; i < acceptedURLs.length; i++)
      if (url.indexOf(acceptedURLs[i]) >= 0)
        return true;

    return false;
  }

  public Connection connect( String url, java.util.Properties props )
      throws SQLException
  {
    if( !acceptsURL( url ) )
      return null;

    boolean defaultConn = url.equals("jdbc:default:connection");

    // Set the catalog & schema values into the Properties parameter 
    // passed to the JDBC driver's connect() method
    // The following prioritization is followed:
    // 1. If the values are already set as part of the Properties parameter to 
    //	  this method, then use that
    // 2. Else, if the useSqlmxUDRCatSch_ flag is true then use the 
    //    "sqlmx.udr.catalog" and "sqlmx.udr.schema" property values.

    // The 'sqlmx.udr.catalog' & 'sqlmx.udr.schema' property values represent
    // the UDR catalog/schema values and are set in the LM layer just before 
    // a UDR is invoked.
    // It is assumed that the property values will always contain valid 
    // (non-null & non-empty) values otherwise an SQLException is thrown.

    UDRcatalog_ = "";
    UDRschema_ = "";
    String msg = "";

    if( props != null ) {
      UDRcatalog_ = props.getProperty( "catalog" );
      UDRschema_ = props.getProperty( "schema" );

      // For default connections get the maxStatement property value, if set.
      // If there is no maxStatements property value set then a default value 
      // will be picked.
      String propVal = "";
      if( defaultConn ) {
        if( (propVal = props.getProperty( "maxStatements" )) == null ) {
          if( maxStmts_ == null )
            maxStmts_ = "256";
        }
        else
          maxStmts_ = propVal;
      }

    }
    else {
      props = new Properties();
    }

    if( useSqlmxUDRCatSch_ ) {

      if( UDRcatalog_ == null ) {
        UDRcatalog_ = System.getProperty( "sqlmx.udr.catalog" );

        if( UDRcatalog_ == null || UDRcatalog_.length() == 0 ) {
          msg = "The SQL/MX language manager encountered an unexpected"
              + " error trying to access the UDR catalog value."
              + " The System.getProperty() method returned a"
              + " null or empty value.";

          throw new SQLException( msg );
        }
      }

      if( UDRschema_ == null ) {
        UDRschema_ = System.getProperty( "sqlmx.udr.schema" );

        if( UDRschema_ == null  || UDRschema_.length() == 0 ) {
          msg = "The SQL/MX language manager encountered an unexpected"
              + " error trying to access the UDR schema value."
              + " The System.getProperty() method returned a"
              + " null or empty value.";
          throw new SQLException( msg );
        }
      }
    }

    if( UDRcatalog_ != null )
      props.setProperty( "catalog", UDRcatalog_ );

    if( UDRschema_ != null )
      props.setProperty( "schema", UDRschema_ );

    Connection conn = null;
    if (defaultConn)
    {
      if (mapDefaultConnToType2_)
      {
        String maxPoolSize = "0";
        String minPoolSize = "1";
        String initialPoolSize = "1";
        props.setProperty("maxPoolSize", maxPoolSize);
        props.setProperty("minPoolSize", minPoolSize);
        props.setProperty("initialPoolSize", initialPoolSize);

        conn = jdbcMxDrvr_.connect(type2URL_, props);
      }
      else
      {
        msg = "The Trafodion language manager encountered an unexpected"
  		      + " error: Type 4 JDBC Connection is not supported"; 
        throw new SQLException( msg );
      }
      {
        // Let's check if SPJ is registered to do any SQL. We throw
	// SQLException with code 8882 if the SPJ is registerd with "NO SQL".
	if (getSqlAccessMode() < CONTAINS_SQL)
	{
          msg = "Containing SQL is not permitted.";
          throw new SQLException(msg, "38001", 8882);
	}
/*
        // For Type 4 driver, if props has serverDataSource, username, password
        // properties, then they are used, else dataSourceName_, userName_,
        // password_ will be used.
        if (props.getProperty("serverDataSource") == null)
        {
          if (System.getProperty("hpt4jdbc.serverDataSource") != null)
            props.setProperty("serverDataSource",
                            System.getProperty("hpt4jdbc.serverDataSource"));
          else if (dataSourceName_ != null)
            props.setProperty("serverDataSource", dataSourceName_);
        }

        if (props.getProperty("user") == null && userName_ != null)
          props.setProperty("user", userName_);

	// Set password property
        String defAuthToken = System.getProperty("sqlmx.udr.defAuthToken");
        System.getProperties().remove("sqlmx.udr.defAuthToken");
        if (defAuthToken != null)
        {
	   // We are in a Definer Rights SPJ.
	   // Use the newly generated Definer Rights token as the password
	   // instead of the (invoker) token propagated from executor.
           props.setProperty("password", defAuthToken);

	   // Force encryption by setting tokenAuth property to false
           props.setProperty("tokenAuth", "false");
        }
        else
          if (props.getProperty("password") == null && password_ != null)
            props.setProperty("password", password_);
	// Set SPJEnv property
        props.setProperty("SPJEnv", "true");

	// Set applicationName property to indicate that the connection
	// is made by UDR server
        if (props.getProperty("applicationName") == null)
        {
          if (System.getProperty("hpt4jdbc.applicationName") != null)
            props.setProperty("applicationName",
                              System.getProperty("hpt4jdbc.applicationName"));
          else			      
            props.setProperty("applicationName", "UDRSERV");
        }

	// Enable connection pooling.
	// This is done in JDBC T4 with setting the maxPoolSize connection
	// property to a value other than the JDBC 4 default value of -1.
	// If it is not specified by the user thru properties parameter or thru
	// hpt4jdbc.maxPoolSize system property, we set the maxPoolSize to 0
	// which means unlimited connections. This becomes the new default 
	// maxPoolSize for all SPJ methods.
	// Also by setting the minPoolSize property to 1, we make sure that 
	// there is always at least one free connection available in  the pool.
	String maxPoolSize = props.getProperty("maxPoolSize");
        if (maxPoolSize == null)
          maxPoolSize = System.getProperty("hpt4jdbc.maxPoolSize");
        if (maxPoolSize == null)
          maxPoolSize = "0"; // Default value for max pool size
	props.setProperty("maxPoolSize", maxPoolSize);

	String minPoolSize = props.getProperty("minPoolSize");
        if (minPoolSize == null)
          minPoolSize = System.getProperty("hpt4jdbc.minPoolSize");
        if (minPoolSize == null)
          minPoolSize = "1"; // Default value for min pool size
	props.setProperty("minPoolSize", minPoolSize);

	String initialPoolSize = props.getProperty("initialPoolSize");
        if (initialPoolSize == null)
          initialPoolSize = System.getProperty("hpt4jdbc.initialPoolSize");
        if (initialPoolSize == null)
          initialPoolSize = "1"; // Default value for initial pool size
	props.setProperty("initialPoolSize", initialPoolSize);

	// statement caching property
	String maxStatements = props.getProperty("maxStatements");
        if (maxStatements == null)
          maxStatements = System.getProperty("hpt4jdbc.maxStatements");

	// The following lines are commented to workaround a problem
	// with JDBC conn pool and stmt cache. When both are enabled,
	// the statements are not reused and cleaned up properly. This
	// eventually caused a memory run down in mxosrvr and mxosrvr
	// crash. This is noted in SOLN# 10-090311-9936
	//
	// Note that we are only disabling stmt caching by default. If
	// stmt caching is set within SPJ method or globally, we honor
	// that.
	//
        // if (maxStatements == null)
        //   maxStatements = "256"; // Default value for max statements

        if (maxStatements != null)
	  props.setProperty("maxStatements", maxStatements);

        // Enable this code to print the connection attributes
        if (false)
        {
          System.out.println("[T4] url " + type4URL_);
          java.util.Enumeration e = props.propertyNames();
          while (e.hasMoreElements())
          {
            String s = e.nextElement().toString();
            System.out.println("[T4] " + s + "=" + props.getProperty(s));
          }
        }

        conn = DriverManager.getConnection(type4URL_, props);
*/
      }

      // For default connections we call the 'addConnection()' 
      // native method in LM C++ layer to indicate that a default 
      // connection has been created.
      addConnection( conn );
    }
    else
      conn = jdbcMxDrvr_.connect(type2URL_, props);
    if (conn != null)
    {
      String parentQid = System.getProperty("sqlmx.udr.parentQid");
      if (parentQid != null)
      {
        Statement stmt = conn.createStatement();
        stmt.executeUpdate("SET SESSION DEFAULT PARENT_QID '" + parentQid + "'");
	stmt.executeUpdate("CONTROL QUERY DEFAULT WMS_CHILD_QUERY_MONITORING 'OFF'");
        stmt.close();
      }
/*
      // Send a request to MXOSRVR (via JDBC T4 driver) to join the current UDR transaction 
      Object argList[] = new Object[1];
      long transId = getTransId();
      if ((transId == -1) && (getTransactionAttrs() != NO_TRANSACTION_REQUIRED)) {
          msg = "The SQL/MX language manager encountered an unexpected" 
    		      + " error trying to get transId.";
          throw new SQLException( msg );    	  
      }
      else
        argList[0] = new Long(transId);
      try
      {
      	if (getTransactionAttrs() != NO_TRANSACTION_REQUIRED)
        	joinUDRTransMethodId_.invoke(conn, argList);
      } 
      catch (InvocationTargetException ite)
      {
        msg = "The SQL/MX language manager encountered an unexpected"
  		      + " error trying to invoke joinUDRTransaction method" 
		      + " in the JDBC/MX driver TrafT4Connection class: " + ite.toString()
	              + " :" + ite.getCause();	      
        throw new SQLException( msg );
      } 
      catch (Exception e)
      {
        msg = "The SQL/MX language manager encountered an unexpected"
  		      + " error trying to invoke joinUDRTransaction method" 
		      + " in the JDBC/MX driver TrafT4Connection class: " + e.toString();
        throw new SQLException( msg );
      }
*/
    }
    return conn;
  }

  // All the methods from here on just call the corresponding 
  // methods on the JDBC/MX driver object

  public DriverPropertyInfo[] getPropertyInfo( String url,
					      java.util.Properties info )
      throws SQLException
  {
    return jdbcMxDrvr_.getPropertyInfo( url, info );
  }

  public boolean jdbcCompliant()
  { 
    return jdbcMxDrvr_.jdbcCompliant();
  }

  public int getMajorVersion()
  { 
    return jdbcMxDrvr_.getMajorVersion();
  }

  public int getMinorVersion()
  { 
    return jdbcMxDrvr_.getMinorVersion();
  }
  
  public Logger getParentLogger() throws SQLFeatureNotSupportedException {
		// TODO Auto-generated method stub
		return null;
	}
}
