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
* Filename		: WeakConnection.java
* Description	:
*/

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.lang.ref.*;
import java.util.HashMap;
 
class WeakConnection
{
	void removeElement(SQLMXConnection conn)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_removeElement].methodEntry();
		try
		{
			refToDialogueId_.remove(conn.pRef_);
			conn.pRef_.clear();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_removeElement].methodExit();
		}
	}
	
	void gcConnections() 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_gcConnections].methodEntry(JdbcDebug.debugLevelPooling);
		try
		{
			Reference pRef;
			long	dialogueId;
			Long dialogueIdObject;
			while ((pRef = refQ_.poll()) != null)
			{
				dialogueIdObject = (Long)refToDialogueId_.get(pRef);
				// All PreparedStatement objects are added to HashMap
				// Only Statement objects that produces ResultSet are added to HashMap
				// Hence stmtLabel could be null
				if (dialogueIdObject != null)	
				{
					dialogueId = dialogueIdObject.longValue();
					try
					{
						SQLMXConnection.close(null, dialogueId);
					}
					catch (SQLException e)
					{
					}
					finally
					{
						refToDialogueId_.remove(pRef);
					}
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_gcConnections].methodExit();
		}
	}

	WeakConnection()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_WeakConnection].methodEntry();
		try
		{
			refQ_ = new ReferenceQueue<SQLMXConnection>();
			refToDialogueId_ = new HashMap<WeakReference, Long>();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_WeakConnection].methodExit();
		}
	}
	
	static ReferenceQueue<SQLMXConnection>	refQ_;
	static HashMap<WeakReference, Long>			refToDialogueId_;

	private static int methodId_removeElement			= 0;
	private static int methodId_gcConnections			= 1;
	private static int methodId_WeakConnection		= 2;
	private static int totalMethodIds				= 3;
	private static JdbcDebug[] debug;
	
	static
	{
		String className = "WeakConnection";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_removeElement] = new JdbcDebug(className,"removeElement"); 
			debug[methodId_gcConnections] = new JdbcDebug(className,"gcConnections"); 
			debug[methodId_WeakConnection] = new JdbcDebug(className,"WeakConnection"); 
		}
	}
}
