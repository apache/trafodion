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
 * Filename : SQLMXPooledConnectionManager.java
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import javax.sql.*;
import java.util.Properties;
import java.util.LinkedList;
import java.io.PrintWriter;
import org.apache.trafodion.jdbc.t2.*;
import java.util.Date;

public class SQLMXPooledConnectionManager implements javax.sql.ConnectionEventListener
{

	public void connectionClosed(ConnectionEvent event)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_connectionClosed].methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive) debug[methodId_connectionClosed].methodParameters(
										  "event=" + JdbcDebug.debugObjectStr(event));
		try
		{
			if (out_ != null)
			{
				if ((traceFlag_ == T2Driver.POOLING_LVL) || 
					(traceFlag_ == T2Driver.ENTRY_LVL))
					out_.println(getTraceId() + "connectionClosed("+ event + ")");
			}
			PooledConnection pc;
			
			pc = (PooledConnection)event.getSource();
			getInUse_().remove(pc);
			SQLMXPooledConnection sqlmxpc = (SQLMXPooledConnection) pc;
			sqlmxpc.setLastUsedTime(System.currentTimeMillis());
//			if (getMinPoolSize_() > 0 && getInUse_().size() < getMaxPoolSize_())
			if (getMinPoolSize_() >= 0)
			{
				try
				{
					if((getMinPoolSize_() == 0) || (getMinPoolSize_() > 0 && ((SQLMXConnectionPoolDataSource)pds_).getMaxIdleTime() > 0))    
					{
						getFree_().add(pc);
					}
					else       //if maxIdleTime_ = 0 
					{		   
//						if(getInUse_().size() + getFree_().size() >= getMinPoolSize_())
						if(getFree_().size() >= getMinPoolSize_())
						{
							if (out_ != null)
							{
								if ((traceFlag_ == T2Driver.POOLING_LVL) || 
									(traceFlag_ == T2Driver.ENTRY_LVL))
									out_.println(getTraceId() + "Free Size reached to minPoolSize, This Connection will be Hard closed. Free Size="+getFree_().size()+" and MinPoolSize="+getMinPoolSize_());
							}
							pc.close();   
						}
						else     
						{
							getFree_().add(pc);   //ensuring minPoolSize_ no. of connections
												  //remain in the pool
						}
							
					}
				}
				catch (SQLException e)
				{
					// ignore any close error
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_connectionClosed].methodExit();
		}
	}

	public void connectionErrorOccurred(ConnectionEvent event)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_connectionErrorOccurred].methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive) debug[methodId_connectionErrorOccurred].methodParameters(
										  "event=" + JdbcDebug.debugObjectStr(event));
		try
		{
			if (out_ != null)
			{
				if ((traceFlag_ == T2Driver.POOLING_LVL) || 
					(traceFlag_ == T2Driver.ENTRY_LVL))
					out_.println(getTraceId() + "connectionErrorOccurred("+ event + ")");
			}
			
			PooledConnection pc;

			pc = (PooledConnection)event.getSource();
			try
			{
				pc.close();
			}
			catch (SQLException e)
			{
				// ignore any close error
			}
			getInUse_().remove(pc);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_connectionErrorOccurred].methodExit();
		}
	}
		
	public Connection getConnection() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getConnection].methodEntry(JdbcDebug.debugLevelPooling);
		try
		{
			if (out_ != null)
			{
				if ((traceFlag_ == T2Driver.POOLING_LVL) || 
					(traceFlag_ == T2Driver.ENTRY_LVL))
					out_.println(getTraceId() + "getConnection()");
			}
			
			PooledConnection pc;
			if (getFree_().size() == 0)
			{
				if (JdbcDebugCfg.traceActive) debug[methodId_getConnection].traceOut(
												  JdbcDebug.debugLevelPooling,
												  "Free pool empty");
				//R3.2 changes -- start
				if (out_ != null)
				{
					if ((traceFlag_ == T2Driver.POOLING_LVL)|| 
							(traceFlag_ == T2Driver.ENTRY_LVL)) 
						out_.println(getTraceId() + "getConnection()"+"Free pool empty");
				}
				//R3.2 changes -- end
			
				if (getMaxPoolSize_() == 0 || getInUse_().size() < getMaxPoolSize_()) //Create a new Connection 
				{
					if (JdbcDebugCfg.traceActive) debug[methodId_getConnection].traceOut(
													  JdbcDebug.debugLevelPooling,
													  "Adding pooled connection");
					
					//R3.2 changes -- start
					if (out_ != null)
					{
						if ((traceFlag_ == T2Driver.POOLING_LVL|| 
								(traceFlag_ == T2Driver.ENTRY_LVL))) 
							out_.println(getTraceId() + "getConnection()"+"Adding pooled connection");
					}
					//R3.2 changes -- end
					
					
					pc = pds_.getPooledConnection();
					pc.addConnectionEventListener(this);
					getInUse_().add(pc);
					// R3.2 changes -- start
					if (out_ != null) {
						if ((traceFlag_ == T2Driver.POOLING_LVL)|| 
								(traceFlag_ == T2Driver.ENTRY_LVL)) {
							if (maxPoolSize_ > 0) {
								out_.println(getTraceId() + "getConnection()"
										+ "IN USE " + getInUse_().size()
										+ " OF MAXPOOL " + getMaxPoolSize_());
							} else {
								out_.println(getTraceId() + "getConnection()"
										+ "IN USE " + getInUse_().size()
										+ " OF UNLIMITED MAXPOOL");
							}
						}
					}
					// R3.2 changes -- end
				}
				else
				{
					Object[] errMsg = new Object[1];
					errMsg[0]	    = new Integer(getMaxPoolSize_());
					throw Messages.createSQLException(null, "max_pool_size_reached", errMsg);
				}
				if(((SQLMXConnectionPoolDataSource)pds_).getInitialPoolSize() > 0 
						&& getMaxPoolSize_() >= ((SQLMXConnectionPoolDataSource)pds_).getInitialPoolSize()) {
					for(int nfor = 0; nfor < ((SQLMXConnectionPoolDataSource)pds_).getInitialPoolSize()-1; ++nfor) {
						PooledConnection pcX = pds_.getPooledConnection();
						pcX.addConnectionEventListener(this);
						getFree_().add(pcX);
					}
				}
			}
			else   //Get the connection from the pool
			{
				if (JdbcDebugCfg.traceActive) debug[methodId_getConnection].traceOut(
												  JdbcDebug.debugLevelPooling,
												  "Getting pooled connection from free list");
				//R3.2 changes -- start
				if (out_ != null)
				{
					if ((traceFlag_ == T2Driver.POOLING_LVL)|| 
							(traceFlag_ == T2Driver.ENTRY_LVL)) 
						out_.println(getTraceId() + "getConnection()"+"Getting pooled connection from free list");
				}
				//R3.2 changes -- end
				pc = (PooledConnection)getFree_().get(0);
				getFree_().remove(0);
				getInUse_().add(pc);
				// R3.2 changes -- start
				if (out_ != null) {
					if ((traceFlag_ == T2Driver.POOLING_LVL)|| 
							(traceFlag_ == T2Driver.ENTRY_LVL)) {
						if (maxPoolSize_ > 0) {
							out_.println(getTraceId() + "getConnection()"
									+ "IN USE " + getInUse_().size()
									+ " OF MAXPOOL " + getMaxPoolSize_());
						} else {
							out_.println(getTraceId() + "getConnection()"
									+ "IN USE " + getInUse_().size()
									+ " OF UNLIMITED MAXPOOL");
						}
					}
				}
				// R3.2 changes -- end
			}
			Connection conn = pc.getConnection();
			if (JdbcDebugCfg.traceActive) debug[methodId_getConnection].methodReturn(
											  "conn=" + JdbcDebug.debugObjectStr(conn));
			return conn;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getConnection].methodExit();
		}
	}

	void setLogWriter(PrintWriter out)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setLogWriter].methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive) debug[methodId_setLogWriter].methodParameters(
										  "out=" + JdbcDebug.debugObjectStr(out));
		try
		{
			out_ = out;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setLogWriter].methodExit();
		}
	}

	SQLMXPooledConnectionManager(SQLMXConnectionPoolDataSource pds,
		int traceFlag)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXPooledConnectionManager].methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive) debug[methodId_SQLMXPooledConnectionManager].methodParameters(
										  "pds=" + JdbcDebug.debugObjectStr(pds) +
										  ", traceFlag=" + traceFlag);
		try
		{
			

			pds_ = pds;
			setInUse_(new LinkedList<PooledConnection>());
			setFree_(new LinkedList<PooledConnection>());
			setMaxPoolSize_(pds.getMaxPoolSize());
			setMinPoolSize_(pds.getMinPoolSize());
//			setMaxIdleTime_(pds.getMaxIdleTime());
			traceFlag_ = traceFlag;
			SQLMXMaxIdleTimeRunnable.getSQLMXMaxIdleTimeRunnable().addPoolManager(this);
			// Build up template portion of jdbcTrace output. Pre-appended to jdbcTrace entries.
			// jdbcTrace:[XXXX]:[Thread[X,X,X]]:[XXXXXXXX]:ClassName.
			
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXPooledConnectionManager].methodExit();
		}
	}

	public void setFree_(LinkedList<PooledConnection> free_) {
		this.free_ = free_;
	}
	public LinkedList<PooledConnection> getFree_() {
		return free_;
	}
	public void setMaxPoolSize_(int maxPoolSize_) {
		this.maxPoolSize_ = maxPoolSize_;
	}
	public int getMaxPoolSize_() {
		return maxPoolSize_;
	}
	public void setMinPoolSize_(int minPoolSize_) {
		this.minPoolSize_ = minPoolSize_;
	}
	public int getMinPoolSize_() {
		return minPoolSize_;
	}
	public void setInUse_(LinkedList<PooledConnection> inUse_) {
		this.inUse_ = inUse_;
	}
	public LinkedList<PooledConnection> getInUse_() {
		return inUse_;
	}
	public void setTraceId(String traceId_) {
		this.traceId_ = traceId_;
	}

	public String getTraceId() {
		String className = getClass().getName();
		setTraceId(T2Driver.traceText + T2Driver.dateFormat.format(new Date()) 
				+ "]:[" + Thread.currentThread() + "]:[" + hashCode() +  "]:" 
				+ className.substring(T2Driver.REMOVE_PKG_NAME,className.length()) 
				+ ".");
		return traceId_;
	}
	

	ConnectionPoolDataSource		pds_;
	private LinkedList<PooledConnection>	inUse_;
	private LinkedList<PooledConnection>	free_;

	private int								maxPoolSize_;
	private int								minPoolSize_;
	int								loginTimeout_;
	int								traceFlag_;
	PrintWriter						out_;
	private String							traceId_;

	private static int methodId_connectionClosed				= 0;
	private static int methodId_connectionErrorOccurred			= 1;
	private static int methodId_getConnection					= 2;
	private static int methodId_setLogWriter					= 3;
	private static int methodId_SQLMXPooledConnectionManager	= 4;
	private static int totalMethodIds							= 5;
	private static JdbcDebug[] debug;


	static
	{
		String className = "SQLMXPooledConnectionManager";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_connectionClosed] = new JdbcDebug(className,"connectionClosed");
			debug[methodId_connectionErrorOccurred] = new JdbcDebug(className,"connectionErrorOccurred");
			debug[methodId_getConnection] = new JdbcDebug(className,"getConnection");
			debug[methodId_setLogWriter] = new JdbcDebug(className,"setLogWriter");
			debug[methodId_SQLMXPooledConnectionManager] = new JdbcDebug(className,"SQLMXPooledConnectionManager");
		}
	}
}
