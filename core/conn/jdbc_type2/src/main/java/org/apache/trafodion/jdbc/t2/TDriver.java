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
 * Filename    : TDriver.java
 * Description :
 *
 * --------------------------------------------------------------------
 */
 
 package org.apache.trafodion.jdbc.t2;
 
 import java.sql.*;
 import java.io.PrintWriter;
 import java.util.Date;
import java.util.logging.Logger;

 
 public class TDriver implements java.sql.Driver
 {
 	 // java.sql.Driver interface methods
 	public boolean acceptsURL(String url) throws SQLException
 	{
		boolean retValue;
		Driver driver;

		driver = loadDriverClass(url);
		DriverManager.println(initTraceId(driver) + "acceptsURL(\"" + url + "\")");
		retValue = driver.acceptsURL(url);
		return retValue;
	}
 		
 	public java.sql.Connection connect(String url, java.util.Properties info)
    		throws SQLException 
	{
		
		Connection	connection;
		Driver driver;

		driver = loadDriverClass(url);
		DriverManager.println(initTraceId(driver) + "connect(\"" + url + "\"," + info +")");
    	connection = driver.connect(url, info);
		return new TConnection(connection, DriverManager.getLogWriter());
	}
    
	public int getMajorVersion()
	{
		int retValue;
		DriverManager.println(initTraceId(lastDriver_) + "getMajorVersion()");
		
		if (lastDriver_ != null)
			retValue = lastDriver_.getMajorVersion();
		else
			retValue = 3;
		return retValue;
	}

	public int getMinorVersion() 
	{
		int retValue;
		DriverManager.println(initTraceId(lastDriver_) + "getMinorVersion()");

		if (lastDriver_ != null)
			retValue = lastDriver_.getMinorVersion();
		else
			retValue = 0;
		return retValue;
	}
	  
	public java.sql.DriverPropertyInfo[]  getPropertyInfo(String url, java.util.Properties info) throws SQLException
    {
		Driver driver;

		driver = loadDriverClass(url);
		DriverManager.println(initTraceId(driver) + "getPropertyInfo(\"" + url + "\"," + info +")");
		return driver.getPropertyInfo(url, info);
	}
    
	public boolean jdbcCompliant()
	{
		boolean retValue;
		DriverManager.println(initTraceId(lastDriver_) + "jdbcCompliant()");

		if (lastDriver_ != null)
			retValue = lastDriver_.jdbcCompliant();
		else
			retValue = true;
		return retValue;
	};
		
	// Fields
	private static Driver	singleton_;
	private static Driver   sqlmpDriver_;
	private static Driver	sqlmxDriver_;
	private static Driver	t3Driver_;
	private static Driver	traceDriver_;
	private static Driver	lastDriver_;

	
	String initTraceId(Driver driver)
	{
	
		String	className = null;
		String	traceId;

		if (driver != null)
			className = driver.getClass().getName();

			// Build up jdbcTrace output entry	
			traceId = org.apache.trafodion.jdbc.t2.T2Driver.traceText
				+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
				+ "]:[" + Thread.currentThread() + "]:["
				+ System.identityHashCode(driver) +  "]:" 
				+ className.substring(org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME,className.length()) 
				+ ".";
		
		return traceId;
	}

	
	private Driver loadDriverClass(String url) throws SQLException
	{
		Class	driverClass;
		String	driverClassName = null;
		Driver  driver;
		
		try
		{
			if (url.startsWith("jdbc:sqlmx:"))
			{
				if (sqlmxDriver_ == null)
				{
					driverClassName = "org.apache.trafodion.jdbc.t2.T2Driver";
					driverClass = Class.forName(driverClassName);
					sqlmxDriver_ = (Driver)driverClass.newInstance();
				}
				driver = sqlmxDriver_;
				
			}
			else
			if (traceDriver_ == null)
				throw new SQLException("System property jdbc.trace.driver is not given");
			else
				driver = traceDriver_;
		}
		catch (Exception ex)
		{
			throw new SQLException(ex.getMessage());
		}
		lastDriver_ = driver;
		return driver;
	}

	TDriver()
	{
	}
		
	// initializer to register the Driver with the Driver manager
	static 
	{
		String	driverClassName;
		Class	driverClass;

		// jdbcTrace output
		DriverManager.println(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:info:Registering with the Driver Manager");

		singleton_ = new TDriver();
		// Register the Driver with the Driver Manager
		try
		{
			DriverManager.registerDriver(singleton_);
		}
		catch (SQLException e)
		{
			singleton_ = null;
			e.printStackTrace();
		}
		driverClassName = System.getProperty("jdbc.trace.driver");

		// jdbcTrace output
		DriverManager.println(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:info:Tracing JDBC Driver " + driverClassName);

		if (driverClassName != null)
		{
			try
			{
				driverClass = Class.forName(driverClassName);
				Object o = driverClass.newInstance();
				if (o instanceof Driver)
					traceDriver_ = (Driver)o;
				else
					throw new SQLException("jdbc.trace.driver is not an instance of java.sql.Driver");
			}	
			catch (Exception ex)
			{
				ex.printStackTrace();
			}
		}
		// jdbcTrace output
		DriverManager.println(org.apache.trafodion.jdbc.t2.T2Driver.traceText
			+ org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date()) 
			+ "]:info:JDBC Driver Tracing started");
	}


	public Logger getParentLogger() throws SQLFeatureNotSupportedException {
		// TODO Auto-generated method stub
		return null;
	}

}
 
