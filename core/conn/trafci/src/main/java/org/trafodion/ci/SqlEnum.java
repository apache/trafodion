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
package org.trafodion.ci;


public enum SqlEnum {
	
	ROLE,
	SCHEMA,
	SERVER,
	USER,
	
	SCHEDULE,
	ERROR,
	ALLOW,
	DENY,
	AUDIT,
	ONLINEDUMP,
	
	MV,
	MVS,
	MATERIALIZED_NAME ("MATERIALIZED VIEW NAME"),
	
	INDEX,
	INDEXES,
	INDEX_NAME ("INDEX NAME"),
	
	SYNONYM,
	SYNONYMS,
	SYNONYM_NAME ("SYNONYM NAME"),
	
	TRIGGER,
	TRIGGERS,
	TRIGGER_NAME ("TRIGGER NAME"),

	TABLE,
	TABLE_NAME,
	TABLE_TYPE,
	
	VIEW,
	VIEWS,
	
	ON,
	DESCRIBE__COL
	;
	
	private String label;
	SqlEnum() {
		this.label = "";
	}
	SqlEnum(String label) {
		this.label = label;
	}
	/**
	 * @return label of the SqlEnum
	 */
	public String getLabel() {
		return this.label;
	}
	
	public static String getFullQualifiedName(
			final String cat, final String schema, final String table) {
		return cat + "." + schema + "." + table;
	}
}
