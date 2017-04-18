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
 * Filename    : ResultSetInfo.java
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.Connection;

public class ResultSetInfo
{
	public static long getVersion()
	{
		return CLASS_VERSION;
	}

	public boolean		LOBDataDetected;
	public Connection	connection;
	public int			ctxHandle;
	public int			stmtID;
	public int			firstBufferedRow;
	public int			lastBufferedRow;
	public int			currentRowPosition;
	public int			cursorType;
	public long         RSCounter;
	public boolean		RSClosed;
	public boolean		stmtClosed; //To maintain CLI Statement Closure Status.

	private static final long CLASS_VERSION = 1;

}
