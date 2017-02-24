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
import java.util.Arrays;
import java.util.Vector;

/**
 * Defines the valid options for the -o arg.
 *
 */
public class WorkTypes {

	/**
	 * Install type.
	 */
	public static final String INSTALL = "install";
	/**
	 * Add text work type.
	 */
	public static final String ADD = "add";
	/**
	 * Delete text work type.
	 */
	public static final String DEL = "del";
	/**
	 * Unknown work type.
	 */
	public static final String UNKNOWN = "unknown";
	
	/**
	 * Valid set of worktypes. 
	 */
	public static final Vector<String> WORKTYPES = new Vector<String>(
			Arrays.asList(INSTALL, ADD, DEL, UNKNOWN));
}
