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

package org.trafodion.wms.rest.model;

import java.io.IOException;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

import org.apache.hadoop.conf.Configuration;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.Constants;

import javax.xml.bind.annotation.XmlElementRef;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Simple representation of a list of workloads.
 */
@XmlRootElement(name="WorkloadList")
public class WorkloadListModel {
	private static final long serialVersionUID = 1L;
	private List<WorkloadModel> workloads = new ArrayList<WorkloadModel>();
	/**
	 * Default constructor
	 */
	public WorkloadListModel() {
	}

	/**
	 * Add the workload name model to the list
	 * @param workload the workload model
	 */
	public void add(WorkloadModel workload) {
		workloads.add(workload);
	}
	
	/**
	 * @param index the index
	 * @return the workload model
	 */
	public WorkloadModel get(int index) {
		return workloads.get(index);
	}

	/**
	 * @return the workloads
	 */
	public List<WorkloadModel> getWorkloads() {
		return workloads;
	}

	/**
	 * @param workloads the workloads to set
	 */
	public void setWorkloads(List<WorkloadModel> workloads) {
		this.workloads = workloads;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append("<html>\n");
		sb.append("<head>\n");
		sb.append("<title>WMS (Workload Management Services)</title>\n");
		sb.append("</head>\n");
		sb.append("<body>\n");
		//
		sb.append("<table border=\"1\">\n");
		sb.append("<tr>\n");	
		sb.append("<th>Workload Id</th>\n");
		sb.append("<th>State</th>\n");
		sb.append("<th>SubState</th>\n");
		sb.append("<th>Workload Details</th>\n");
		sb.append("</tr>\n");
		//
		for(WorkloadModel aWorkload : workloads) {
			sb.append(aWorkload.toString());
			sb.append('\n');
		}
		//
		sb.append("</table>\n");
		sb.append("</body>\n");
		sb.append("</html>\n");
		return sb.toString();
	}
}
