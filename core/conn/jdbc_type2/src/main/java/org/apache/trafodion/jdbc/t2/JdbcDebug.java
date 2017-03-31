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
/*
 * Filename    : JdbcDebug.java
 */

package org.apache.trafodion.jdbc.t2;

class JdbcDebug
{
	static final int debugFlagThreads		= 0x80000000;
	static final int debugFlagDirtyMem		= 0x40000000;
	static final int debugFlagTimer			= 0x20000000;
	static final int debugFlagNoTime		= 0x10000000;
	static final int debugFlagTestware		= 0x08000000;
	static final int debugLevelEntry		= 0x00000001;
	static final int debugLevelJava			= 0x00000002;
	static final int debugLevelMem			= 0x00000004;
	static final int debugLevelMemLeak		= 0x00000008;
	static final int debugLevelCLI			= 0x00000010;
	static final int debugLevelData			= 0x00000020;
	static final int debugLevelTxn			= 0x00000040;
	static final int debugLevelRowset		= 0x00000080;
	static final int debugLevelError		= 0x00000100;
	static final int debugLevelMetadata		= 0x00000200;
	static final int debugLevelUnicode		= 0x00000400;
	static final int debugLevelPooling		= 0x00000800;
	static final int debugLevelStmt			= 0x00001000;
	static final int debugLevelAll			= 0x0000ffff;

	//Venu changed debugHandle and methodNameHandle from int to long for 64 bit
	long debugHandle;
	long methodNameHandle;
    
	JdbcDebug(String class_name, String method_name)
	{
		methodNameHandle = getMethodNameHandle("JAVA_COM_HP_SQLMX_" + class_name + "_" + method_name);
		debugHandle = getDebugHandle(methodNameHandle);
	}


	void traceOut(int debug_level, String message)
	{
		if (!JdbcDebugCfg.traceActive)
		{
			System.out.println("traceOut() called during non-tracing processing");
			System.exit(1);
		}
		traceOut(debugHandle,debug_level,message);
	}

	void methodEntry()
	{
		if (!JdbcDebugCfg.entryActive)
		{
			System.out.println("methodEntry() called during non-entry processing");
			System.exit(1);
		}
		methodEntry(debugHandle,debugLevelJava,methodNameHandle);
	}

	void methodEntry(int debug_level)
	{
		if (!JdbcDebugCfg.entryActive)
		{
			System.out.println("methodEntry(I) called during non-entry processing");
			System.exit(1);
		}
		methodEntry(debugHandle,debug_level|debugLevelJava,methodNameHandle);
	}

	void methodParameters(String parameters)
	{
		if (!JdbcDebugCfg.traceActive)
		{
			System.out.println("methodParameters() called during non-traceing processing");
			System.exit(1);
		}
		traceOut(debugHandle,debugLevelEntry|debugLevelJava,"  (" + parameters + ")");
	}

	void methodExit()
	{
		if (!JdbcDebugCfg.entryActive)
		{
			System.out.println("methodExit() called during non-entry processing");
			System.exit(1);
		}
		methodExit(debugHandle);
	}

	void methodReturn(String message)
	{
		if (!JdbcDebugCfg.traceActive)
		{
			System.out.println("methodReturn() called during non-traceing processing");
			System.exit(1);
		}
		methodReturn(debugHandle,message);
	}

	static String debugObjectStr(Object obj)
	{
		if (obj==null) return("null");
		return(obj.toString());
	}

	static String debugTypeStr(int dataType)
	{
		if (JdbcDebugCfg.traceActive)
		{
			switch (dataType)
			{
				case java.sql.Types.ARRAY:
					return("ARRAY");
				case java.sql.Types.BIGINT:
					return("BIGINT");
				case java.sql.Types.BINARY:
					return("BINARY");
				case java.sql.Types.BIT:
					return("BIT");
				case java.sql.Types.BLOB:
					return("BLOB");
				case java.sql.Types.BOOLEAN:
					return("BOOLEAN");
				case java.sql.Types.CHAR:
					return("CHAR");
				case java.sql.Types.CLOB:
					return("CLOB");
				case java.sql.Types.DATALINK:
					return("DATALINK");
				case java.sql.Types.DATE:
					return("DATE");
				case java.sql.Types.DECIMAL:
					return("DECIMAL");
				case java.sql.Types.DISTINCT:
					return("DISTINCT");
				case java.sql.Types.DOUBLE:
					return("DOUBLE");
				case java.sql.Types.FLOAT:
					return("FLOAT");
				case java.sql.Types.INTEGER:
					return("INTEGER");
				case java.sql.Types.JAVA_OBJECT:
					return("JAVA_OBJECT");
				case java.sql.Types.LONGVARBINARY:
					return("LONGVARBINARY");
				case java.sql.Types.LONGVARCHAR:
					return("LONGVARCHAR");
				case java.sql.Types.NULL:
					return("NULL");
				case java.sql.Types.NUMERIC:
					return("NUMERIC");
				case java.sql.Types.OTHER:
					return("OTHER");
				case java.sql.Types.REAL:
					return("REAL");
				case java.sql.Types.REF:
					return("REF");
				case java.sql.Types.SMALLINT:
					return("SMALLINT");
				case java.sql.Types.STRUCT:
					return("STRUCT");
				case java.sql.Types.TIME:
					return("TIME");
				case java.sql.Types.TIMESTAMP:
					return("TIMESTAMP");
				case java.sql.Types.TINYINT:
					return("TINYINT");
				case java.sql.Types.VARBINARY:
					return("VARBINARY");
				case java.sql.Types.VARCHAR:
					return("VARCHAR");
			}
			return("Unknown (" + Integer.toString(dataType) + ")");
		}
		return null;
	}

	native static private long getDebugHandle(long method_name_handle);
	native static private long getMethodNameHandle(String method_name);
	native static private void methodEntry(long handle, int debug_level, long method_name_handle);
	native static private void methodReturn(long handle, String message);
	native static private void methodExit(long handle);
	native static private void traceOut(long handle, int debug_level, String message);

	static
	{
		System.loadLibrary("jdbcT2");
	}
}
