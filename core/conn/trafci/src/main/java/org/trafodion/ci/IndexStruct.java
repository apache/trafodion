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

public class IndexStruct {
	private String indexName;
	private String tableName;
	private String tableType;

	public IndexStruct(String indexName, String tableName, String tableType) {
		super();
		this.indexName = indexName;
		this.tableName = tableName;
		this.tableType = tableType;
	}
	public final String getTableType() {
		return tableType;
	}
	public final void setTableType(String tableType) {
		this.tableType = tableType;
	}
	public final String getIndexName() {
		return indexName;
	}

	public final void setIndexName(String indexName) {
		this.indexName = indexName;
	}

	public final String getTableName() {
		return tableName;
	}

	public final void setTableName(String tableName) {
		this.tableName = tableName;
	}
}
