/**
* @@@ START COPYRIGHT @@@

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.

* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs.master.listener;


public class ListenerMetrics {
	private long timestamp=0L; //start Listener timestamp
	private int requests=0; //total number of requests
	private int completedRequests=0; //total number of requests completed successfully
	private int readTimeouts=0; //total number of read timeouts
	private int writeTimeouts=0; //total number of write timeouts
	private int rejects=0; //total number of rejected requests (no AVAILABLE MXOSRVRs)
	private long totalRequestTime=0L; //total time in microseconds successfully completed requests since init timestamp
	private long avgRequests=0; //average number successfully completed requests per second
	private long avgRequestTime=0; //average request time in microsec successfully completed requests since init timestamp
	private long minRequestTime=0; //min request time in microsec successfully completed requests since init timestamp
	private long maxRequestTime=0; //max request time in microsec successfully completed requests since init timestamp

	private long curRequestTimestamp=0L;
	private long curRequestTime=0;
	private long lastRequestTime=0;

	private int noAvailableServers=0;

	public ListenerMetrics(long nanoTimestamp){
		this.timestamp = nanoTimestamp/1000;
	}
	public void listenerStartRequest(long nanoTimestamp){
		curRequestTimestamp = nanoTimestamp/1000;
		requests++;
	}
	public void listenerEndRequest(long nanoTimestamp){
		long timestamp = nanoTimestamp/1000;
		long microsec = 1000000;
		completedRequests++;
		if (curRequestTimestamp != 0){
			lastRequestTime = curRequestTime;
			curRequestTime = timestamp - curRequestTimestamp;
			if (lastRequestTime == 0)
				lastRequestTime = curRequestTime;
			totalRequestTime += curRequestTime;
			avgRequestTime = totalRequestTime / completedRequests;
			if (avgRequestTime == 0)
				avgRequests = 0;
			else
				avgRequests = microsec / avgRequestTime;
			if (minRequestTime == 0 || minRequestTime  > curRequestTime)
				minRequestTime = curRequestTime;
			if (maxRequestTime == 0 || maxRequestTime < curRequestTime)
				maxRequestTime = curRequestTime;
		}
	}
	public void listenerRequestRejected(){
		rejects++;
		curRequestTimestamp = 0; //do not include it into time statistics
	}
	public void listenerWriteTimeout(){
		writeTimeouts++;
		curRequestTimestamp = 0; //do not include it into time statistics
	}
	public void listenerReadTimeout(){
		readTimeouts++;
		curRequestTimestamp = 0; //do not include it into time statistics
	}
	public void listenerNoAvailableServers(){
		noAvailableServers++;
		curRequestTimestamp = 0; //do not include it into time statistics
	}
	public String toString() {
		String report = "listenerRequests=" + requests + ", listenerCompletedRequests=" + completedRequests + ", listenerReadTimeouts=" + readTimeouts + 
				", listenerWriteTimeouts=" + writeTimeouts + ", listenerRejects=" + rejects + ", listenerAvgRequestsSecond=" + avgRequests + 
				", listenerAvgRequestTimeMicrosec=" + avgRequestTime + ", listenerLastRequestTimeMicrosec=" + lastRequestTime + 
				", listenerMinRequestTimeMicrosec=" + minRequestTime + ", listenerMaxRequestTimeMicrosec=" + maxRequestTime + 
				", listenerNoAvailableServers=" + noAvailableServers;
		return report;
	}
};
