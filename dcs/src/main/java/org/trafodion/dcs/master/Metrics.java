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
package org.trafodion.dcs.master;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileNotFoundException;

import java.util.Scanner;
import java.util.Collections;
import java.util.List;
import java.util.ArrayList;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutionException;
import java.util.Date;

import java.text.DateFormat;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.trafodion.dcs.script.ScriptManager;
import org.trafodion.dcs.script.ScriptContext;
import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.DcsConfiguration;

import org.trafodion.dcs.master.listener.ListenerMetrics;

public class Metrics  {
	private static  final Log LOG = LogFactory.getLog(Metrics.class);

	private ListenerMetrics listenerMetrics;
	private int totalRunning;
	private int totalRegistered;
	private int totalAvailable;
	private int totalConnecting;
	private int totalConnected;

	public String getLoad(){               
		int mb = 1024*1024;  
		long total;
		long free;
		long max;
		long used;
    
		Runtime runtime = Runtime.getRuntime();                   
		used = (runtime.totalMemory() - runtime.freeMemory()) / mb;           
		free = runtime.freeMemory() / mb;                   
		total = runtime.totalMemory() / mb;           
		max = runtime.maxMemory() / mb;
		String report = "totalHeap=" + total + ", usedHeap=" + used + ", freeHeap=" + free + ", maxHeap=" + max;
		return report;
	}
	
	public void initListenerMetrics(long timestamp){
		listenerMetrics = new ListenerMetrics(timestamp);
	}

	public void listenerStartRequest(long timestamp){
		if (listenerMetrics != null)
			listenerMetrics.listenerStartRequest(timestamp);
	}
	public void listenerEndRequest(long timestamp){
		if (listenerMetrics != null)
			listenerMetrics.listenerEndRequest(timestamp);
	}
	public void listenerRequestRejected(){
		if (listenerMetrics != null)
			listenerMetrics.listenerRequestRejected();
	}
	public void listenerWriteTimeout(){
		if (listenerMetrics != null)
			listenerMetrics.listenerWriteTimeout();
	}
	public void listenerReadTimeout(){
		if (listenerMetrics != null)
			listenerMetrics.listenerReadTimeout();
	}
	public void listenerNoAvailableServers(){
		if (listenerMetrics != null)
			listenerMetrics.listenerNoAvailableServers();
	}
	private String getListenerMatrics(){
		String report = "";
		if (null != listenerMetrics){
			report = listenerMetrics.toString();
		}
		return report;
	}
	public void setTotalRunning(int value){
		totalRunning=value;
	}
	public void setTotalRegistered(int value){
		totalRegistered=value;
	}
	public void setTotalAvailable(int value){
		totalAvailable=value;
	}
	public void setTotalConnecting(int value){
		totalConnecting=value;
	}
	public void setTotalConnected(int value){
		totalConnected=value;
	}
	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append(getLoad());
		sb.append(", ");
		sb.append(getListenerMatrics());
		sb.append(", ");
		sb.append("totalRunning=" + totalRunning);
		sb.append(", ");
		sb.append("totalRegistered=" + totalRegistered);
		sb.append(", ");
		sb.append("totalAvailable=" + totalAvailable);
		sb.append(", ");
		sb.append("totalConnecting=" + totalConnecting);
		sb.append(", ");
		sb.append("totalConnected=" + totalConnected);
		return sb.toString();
	}
}

