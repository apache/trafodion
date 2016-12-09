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
package org.trafodion.ci.pwdencrypt;

import java.util.HashMap;
/**
 * The map of valid arguments.
 *
 */
public class ArgMap {

	/**
	 * Map of valid arguments.
	 */
	public static final HashMap<Character, ArgsDef> ARGMAP = argMapBuilder();
	
	/**
	 * Constructor.
	 * @return ArgMap
	 */
	private static HashMap<Character, ArgsDef> argMapBuilder() {
		HashMap<Character, ArgsDef> constructmap = 
			new HashMap<Character, ArgsDef>();
		constructmap.put('o', new ArgsDef(
				"option", true, 'o', 
				"either \"install\", \"add\", or \"del\""));
		constructmap.put('u', new ArgsDef(
				"username", true, 'u', 
				"username valid only for \"add\" and \"del\""));
		constructmap.put('p', new ArgsDef(
				"password", true, 'p', 
				"password valid only for \"add\""));
		constructmap.put('l', new ArgsDef(
				"logdir",  true, 'l', "Specify log file directory"));
		constructmap.put('f', new ArgsDef(
				"logfile", true, 'f', "Specify log file name"));
		constructmap.put('h', new ArgsDef(
				"help", false, 'h', "Display this message"));
		return constructmap;
	}
}
