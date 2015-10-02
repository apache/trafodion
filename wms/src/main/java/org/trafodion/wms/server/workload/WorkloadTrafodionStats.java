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

package org.trafodion.wms.server.store;

import java.util.*;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public final class WorkloadTrafodionStats extends WorkloadStats {
	private static  final Log LOG = LogFactory.getLog(WorkloadTrafodionStats.class);
//	private int maxCpuBusy=100;
//	private int maxMemUsage=50;
//	private int maxExecMinutes=5;
	
	public WorkloadTrafodionStats() {
		super();
	}
/*	
	public void setMaxCpuBusy(int value){
		maxCpuBusy = value;
	}
	public int getMaxCpuBusy(){
		return maxCpuBusy;
	}
	public void setMaxMemUsage(int value){
		this.maxMemUsage = value;
	}
	public int getMaxMemUsage(){
		return maxMemUsage;
	}
	public void setMaxExecMinutes(int value){
		this.maxExecMinutes = value;
	}
	public int getMaxExecMinutes(){
		return maxExecMinutes;
	}
	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append("MaxCpuBusy[" + getMaxCpuBusy() + "],");
		sb.append("MaxMemUsage[" + getMaxMemUsage() + "],");
		sb.append("MaxExecMinutes[" + getMaxExecMinutes() + "]");
		return sb.toString();
	}
*/
}