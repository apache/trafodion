/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
package org.trafodion.wms.hive;

import java.io.*;
import java.util.*;

import org.apache.hadoop.hive.ql.hooks.*;
import org.apache.hadoop.hive.conf.HiveConf;
import org.apache.hadoop.hive.ql.session.SessionState;
import org.apache.hadoop.hive.ql.session.SessionState.LogHelper;
import org.apache.hadoop.security.UserGroupInformation;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.server.WorkloadsQueue;
import org.trafodion.wms.thrift.generated.*;
import org.trafodion.wms.client.WmsClient;
//
// Implementation of a pre execute hook that simply prints out its parameters to
// standard output.
//
public class PreExecute implements ExecuteWithHookContext {

	final static long ONE_SECOND = 1000;

	private static void setQueryId(String value){
		queryId = value;
	}
	private static void setJobID(String value){
		jobID = value;
	}
	private static void setConsole(LogHelper value){
		console = value;
	}
	private static void setLastUpdateTimestamp(long value){
		lastUpdateTimestamp = value;
	}
	private static String getQueryId(){
		return queryId;
	}
	private static String getJobID(){
		return jobID;
	}
	private static LogHelper getConsole(){
		return console;
	}
	private static long getLastUpdateTimestamp(){
		return lastUpdateTimestamp;
	}

	private static HashMap< String, JobStore> workloadMap = new HashMap<String, JobStore>();
	private static WmsClient conn = null;
	//	private static WmsWorkloadFactory factory = null;
	//	private static WmsWorkload w = null;
	private static String queryId = "";
	private static String jobID = "";
	private static LogHelper console = null;
	private static long lastUpdateTimestamp = 0;

	@Override
	public void run(HookContext hookContext) throws Exception {
		SessionState ss = SessionState.get();
		Set<ReadEntity> inputs = hookContext.getInputs();
		Set<WriteEntity> outputs = hookContext.getOutputs();
		UserGroupInformation ugi = hookContext.getUgi();
		HiveConf hconf = hookContext.getConf();
		this.run(ss,inputs,outputs,ugi,hconf);
	}

	public void run(SessionState sess, Set<ReadEntity> inputs,
			Set<WriteEntity> outputs, UserGroupInformation ugi, HiveConf hconf)
	throws Exception {

		LogHelper console = SessionState.getConsole();

		if (console == null) {
			return;
		}
		setConsole(console);

		setQueryId("");
		setJobID("");

		if (sess != null) {
			console.printError("PREHOOK: query: " + sess.getCmd().trim());
			console.printError("PREHOOK: type: " + sess.getCommandType());
			console.printError("PREHOOK: queryId: " + sess.getQueryId());
			console.printError("PREHOOK: sessionId: " + sess.getSessionId());
			printWms(sess,hconf,console);
		}

		printEntities(console, inputs, "PREHOOK: Input: ");
		printEntities(console, outputs, "PREHOOK: Output: ");
	}

	static void printEntities(LogHelper console, Set<?> entities, String prefix) {
		List<String> strings = new ArrayList<String>();
		for (Object o : entities) {
			strings.add(o.toString());
		}
		Collections.sort(strings);
		for (String s : strings) {
			console.printError(prefix + s);
		}
	}
	private static class JobStore {
		private long startTime;
		private String workloadId;
		//		private ActionType action;

		JobStore() {
			this.startTime = 0;
			this.workloadId = "";
			//action = ActionType.CONTINUE;
		}
	}

	static void printWms(SessionState sess,  HiveConf hconf, LogHelper console){
		String workloadId;
		String queryId = sess.getQueryId();

		try {
			if (conn == null){
				conn = new WmsClient();
				conn.open();
			}
			//if (factory == null){
			//	factory = new WmsWorkloadFactory();
			//	w = WmsWorkloadFactory.getWorkload("org.trafodion.wms.HadoopWorkload");
			//}
		} catch (Exception e) {
			console.printError("HOOK: printWms:Exception " + e);
		} 

		try {
			long startTime;
			long endTime;
			long duration;

			if (!workloadMap.containsKey(sess.getQueryId())){
				console.printError("HOOK: printWms:BEGIN ");
				setQueryId(sess.getQueryId());
				JobStore job = new JobStore();
				if (job != null ){
					startTime = System.currentTimeMillis();
					endTime = startTime;
					duration = endTime - startTime;
					job.startTime = startTime;

					if (conn != null && w != null) {

						w.setOperation(OperationType.BEGIN);
						w.setJobType(JobType.HIVE);
						w.setJobText(sess.getCmd().trim());
						w.setUserInfo(hconf.getUser());
						w.setJobInfo(sess.getCommandType());
						w.setJobState("RUNNING");
						w.setJobSubState("BEGIN");
	//					w.setUserInfo(sess.conf.getUser());
						w.setStartTime(startTime);
						w.setEndTime(endTime);
						w.setDuration(duration);
						w.setParentId("");
						w.setParentKey("");
						console.printError("HOOK: printWms:BEGIN " + w);
						WorkloadResponse response = conn.writeread(w);
						job.workloadId = response.getWorkloadId().toString();
						job.action = response.getAction();
					}
					workloadMap.put(getQueryId(), job);
				}
			} else {
				JobStore job = workloadMap.get(sess.getQueryId());
				startTime = job.startTime;
				endTime = System.currentTimeMillis();
				duration = endTime - startTime;
				if (conn != null && w != null) {
					w.setOperation(OperationType.END);
					w.setWorkloadId(job.workloadId);
					w.setJobType(JobType.HIVE);
					w.setJobText(sess.getCmd().trim());
					w.setUserInfo(hconf.getUser());
					w.setJobInfo(sess.getCommandType());
					w.setJobState("COMPLETED");
					w.setJobSubState("SUCCEEDED");
					w.setStartTime(startTime);
					w.setEndTime(endTime);
					w.setDuration(duration);
					WorkloadResponse response = conn.writeread(w);
				}
				workloadMap.remove(sess.getQueryId());
			}
		} catch (IOException e) {
			console.printError("HOOK: printWms:IOException " + e);
		} catch (IllegalStateException e){
			//no connection open - return back
			console.printError("HOOK: printWms:IllegalStateException " + e);
		}
	}
	
	static void printUpdateWms(Map<String,Double> counterValues, String jobID){
		LogHelper console = getConsole();
		if (getJobID().length() == 0 || false == getJobID().equals(jobID)){
			setJobID(jobID);
			if (workloadMap.containsKey(getQueryId())){
				JobStore job = workloadMap.get(getQueryId());
				console.printError("PUBLISHER UPDATE_PARENT_ID: jobID:" + jobID + " queryId: " + getQueryId() );
				if (conn != null && w != null) {
					w.setOperation(OperationType.UPDATE_PARENT_ID);
					w.setParentId(job.workloadId);
					w.setParentKey(jobID);
					WorkloadResponse response = conn.writeread(w);
					if(response == null)
						console.printError("PUBLISHER: printUpdateWms:Error " + conn.getLastError());
				}
			}
		}
		if (workloadMap.containsKey(getQueryId())){
			long currentTimestamp = System.currentTimeMillis();
			if ( currentTimestamp > getLastUpdateTimestamp() + 5 * ONE_SECOND){
				setLastUpdateTimestamp(currentTimestamp);
				JobStore job = workloadMap.get(getQueryId());
				if (job.action == ActionType.CONTINUE){
					if (conn != null && w != null){
						console.printError("PUBLISHER UPDATE: jobID:" + jobID + " queryId: " + getQueryId() );
						long startTime = job.startTime;
						long endTime = getLastUpdateTimestamp();
						long duration = endTime - startTime;

						w.setOperation(OperationType.UPDATE);
						w.setWorkloadId(job.workloadId);
						w.setJobType(JobType.HIVE);
						w.setJobState("RUNNING");
						w.setJobSubState("UPDATE");
						w.setStartTime(startTime);
						w.setEndTime(endTime);
						w.setDuration(duration);

						WorkloadResponse response = conn.writeread(w);
						if(response != null) {
							job.action = response.getAction();
							workloadMap.put(getQueryId(), job);

							switch(job.action){
							case REJECT:
							case CANCEL:
								console.printError("PUBLISHER CANCEL: jobID:" + jobID + " queryId: " + getQueryId() );
								w.setOperation(OperationType.CANCEL_CHILDREN);
								w.setParentId(job.workloadId);
								w.setParentKey(jobID);
								response = conn.writeread(w);
								if(response != null) 
									console.printError("PUBLISHER: printUpdateWms:Error " + conn.getLastError());
								break;
							default:
								break;
							}

						} else {
							console.printError("PUBLISHER: printUpdateWms:Error " + conn.getLastError());
						}

					}
			} else {
				if (conn != null && w != null){
					console.printError("PUBLISHER UPDATE: jobID:" + jobID + " queryId: " + getQueryId() );
					long startTime = job.startTime;
					long endTime = getLastUpdateTimestamp();
					long duration = endTime - startTime;
						w.setOperation(OperationType.UPDATE);
						w.setWorkloadId(job.workloadId);
						w.setJobType(JobType.HIVE);
						w.setJobState("COMPLETED");
						w.setJobSubState("KILLED");
						w.setStartTime(startTime);
						w.setEndTime(endTime);
						w.setDuration(duration);

						WorkloadResponse response = conn.writeread(w);
						if(response != null) {
							job.action = response.getAction();
							workloadMap.put(getQueryId(), job);
							console.printError("PUBLISHER: printUpdateWms:Error " + conn.getLastError());
						}
				}
				workloadMap.remove(getQueryId());
			}
		}
	}

		console.printError("jobID: " + jobID + " queryId: " + queryId);
		Set<String> keys = counterValues.keySet();
		for(String key : keys){
			console.printError(key + ": " + counterValues.get(key));
		}

}
*/