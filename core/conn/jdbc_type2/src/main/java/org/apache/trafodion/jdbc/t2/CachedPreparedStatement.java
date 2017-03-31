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
 * Filename    : CachedPreparedStatement.java
 * Description :
 */

/* 
 * Methods Added: setPstmt_(SQLMXPreparedStatement ), getPstmt_()
 * Methods Changed: getPreparedStatement(), close(boolean ),
 *                  CachedPreparedStatement(PreparedStatement ,String )
 */ 
package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import javax.sql.*;
import java.util.HashMap;
import java.util.Properties;


class CachedPreparedStatement implements Comparable<CachedPreparedStatement>
{
		
	PreparedStatement getPreparedStatement()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getPreparedStatement].methodEntry();
		try
		{
			return getPstmt_();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getPreparedStatement].methodExit();
		}
	}

	void setLastUsedInfo()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setLastUsedInfo].methodEntry();
		try
		{
			lastUsedTime_ = System.currentTimeMillis();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setLastUsedInfo].methodExit();
		}
	}

	long getLastUsedTime()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getLastUsedTime].methodEntry();
		try
		{
			return lastUsedTime_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getLastUsedTime].methodExit();
		}
	}

	String getLookUpKey()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getLookUpKey].methodEntry();
		try
		{
			return key_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getLookUpKey].methodExit();
		}
	}

	void close(boolean hardClose) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_close].methodEntry();
		try
		{
			inUse_ = false;
			getPstmt_().close(hardClose);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_close].methodExit();
		}
	}

		
	CachedPreparedStatement(PreparedStatement pstmt, String key)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_CachedPreparedStatement].methodEntry();
		try
		{
			setPstmt_((SQLMXPreparedStatement)pstmt);
			key_ = key;
			creationTime_ = System.currentTimeMillis();
			lastUsedTime_ = creationTime_;
			inUse_ = true;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_CachedPreparedStatement].methodExit();
		}
	}
	/**
	 * @param pstmt_ the pstmt_ to set
	 */
	public void setPstmt_(SQLMXPreparedStatement pstmt_) {
		this.pstmt_ = pstmt_;
	}

	/**
	 * @return the pstmt_
	 */
	public SQLMXPreparedStatement getPstmt_() {
		return pstmt_;
	}

	private SQLMXPreparedStatement	pstmt_;
	private	String					key_;
	private	long					lastUsedTime_;
	private	long					creationTime_;
	boolean							inUse_;

	private static int methodId_getPreparedStatement	= 0;
	private static int methodId_setLastUsedInfo			= 1;
	private static int methodId_getLastUsedTime			= 2;
	private static int methodId_getLookUpKey			= 3;
	private static int methodId_close					= 4;
	private static int methodId_CachedPreparedStatement	= 5;
	private static int totalMethodIds					= 6;
	private static JdbcDebug[] debug;

	static
	{
		String className = "CachedPreparedStatement";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_getPreparedStatement] = new JdbcDebug(className,"getPreparedStatement");
			debug[methodId_setLastUsedInfo] = new JdbcDebug(className,"setLastUsedInfo");
			debug[methodId_getLastUsedTime] = new JdbcDebug(className,"getLastUsedTime");
			debug[methodId_getLookUpKey] = new JdbcDebug(className,"getLookUpKey");
			debug[methodId_close] = new JdbcDebug(className,"close");
			debug[methodId_CachedPreparedStatement] = new JdbcDebug(className,"CachedPreparedStatement");
		}

	}

	public int compareTo(CachedPreparedStatement arg0) {
		
		if(this.getLastUsedTime() == arg0.getLastUsedTime()) {
			return 0;
		}
		if(this.getLastUsedTime() < arg0.getLastUsedTime()) {
			return -1;
		}
		if(this.getLastUsedTime() > arg0.getLastUsedTime()) {
			return 1;
		}
		return 0;
	}
} 
