/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

package org.trafodion.dcs.master;
import java.util.ArrayList;
import java.util.Hashtable;


public class QueryPlanResponse {
	public String getId() {
		return id;
	}
	public void setId(String id) {
		this.id = id;
	}
	public String getName() {
		return name;
	}
	public void setName(String name) {
		this.name = name;
	}
	public Hashtable getData() {
		return data;
	}
	public void setData(Hashtable data) {
		this.data = data;
	}
	public ArrayList<QueryPlanResponse> getChildren() {
		return children;
	}
	public void setChildren(ArrayList<QueryPlanResponse> children) {
		this.children = children;
	}
	String id;
	String name;
	Hashtable data = new Hashtable();
	ArrayList<QueryPlanResponse> children = new ArrayList<QueryPlanResponse>();
}
