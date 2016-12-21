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

public class ArgsDef {


	private String name;
	private boolean argrequired;
	private char ident;
	private String error;
	
	/**
	 * Constructor.
	 * @param name name of option rule
	 * @param argrequired is option required
	 * @param ident char that coralates command to arg
	 */
	public ArgsDef(String name, boolean argrequired, char ident, String error) {
		this.name = name;
		this.argrequired = argrequired;
		this.ident = ident;
		this.error = error;
	}

	/**
	 * Returns ident char.
	 * @return the ident
	 */
	public final char getIdent() {
		return ident;
	}

	/**
	 * @return the name
	 */
	public final String getName() {
		return name;
	}

	/**
	 * @return the required
	 */
	public final boolean isArgRequired() {
		return argrequired;
	}

	/**
	 * @return the error
	 */
	public final String getError() {
		return error;
	}
}
