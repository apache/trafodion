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
 //
 **********************************************************************/

/**
 * class SymCrypto - Stores connections and correspondence SecPwd
 *                   objects.
 */

package org.trafodion.jdbc.t4;

import java.util.HashMap;
import java.sql.Connection;

public class SymCrypto
{
	static HashMap<Connection, SymCrypto> storage = new HashMap<Connection, SymCrypto> ();

	/**
	 * Ctor -
	 * @param secpwd
	 */
	private SymCrypto(SecPwd secpwd)
	{
		m_secPwd = secpwd;
	}

	/**
	 * Returns the SymCrypto object correspondence to the connection passed in
	 * @param con
	 * @return the value to which the SymCrypto object maps the connection passed in or
	 *         null if the map contains no mapping for the connection.
	 * @throws SecurityException
	 */
	public static SymCrypto getInstance(Connection con) throws SecurityException
	{
		if (con == null)
			throw new SecurityException (SecClientMsgKeys.INPUT_PARAMETER_IS_NULL, new Object[]{"connection"});
		return storage.get(con);
	}

	/**
	 * Creates and SymCrypto object from the SecPwd object and inserts the connection
	 * and the equivalent SymCRypto object into the hash table storage.  If the table previously
	 * contained a mapping for the connection, the old value is replaced.
	 * @param con - the JDBC connection
	 * @param secpwd - the SecPwd object associated with the JDBC connection
	 */
	public static void insert(Connection con, SecPwd secpwd)
	{
		SymCrypto symcrypto = new SymCrypto(secpwd);
		storage.put(con, symcrypto);
	}

	/**
	 * Removed the mapping of this JDBC connection from the hash table if present
	 * @param con - JDBC connection whose entry is to be removed from the hash table storage
	 */
	public static void remove(Connection con)
	{
		if (con != null)
			storage.remove(con);
	}

	private SecPwd m_secPwd;

}
