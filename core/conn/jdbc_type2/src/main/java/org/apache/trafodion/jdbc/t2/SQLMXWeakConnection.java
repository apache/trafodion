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
 * Filename    : SQLMXWeakConnection.java
 * Description :
 *
 */

package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.lang.ref.*;
import java.util.HashMap;
 
public class SQLMXWeakConnection
{
	void removeElement(SQLMXConnection conn)
	{
		refToDialogueId_.remove(conn.pRef_);
		conn.pRef_.clear();
	}
	
	void gcConnections() 
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

	SQLMXWeakConnection()
	{
		refQ_ = new ReferenceQueue();
		refToDialogueId_ = new HashMap();
	}

	static ReferenceQueue	refQ_;
	static HashMap			refToDialogueId_;
} 
