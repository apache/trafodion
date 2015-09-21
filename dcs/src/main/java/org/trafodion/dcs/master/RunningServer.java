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

public class RunningServer  {
	private static  final Log LOG = LogFactory.getLog(RunningServer.class);
	private String hostname;
	private String instance;
	private int infoPort;
	private long startTime;
	private ArrayList<RegisteredServer> registeredList = new ArrayList<RegisteredServer>();

	public void setHostname(String value) {
		hostname = value;
	}
	public String getHostname() {
		return hostname;
	}
	public void setInstance(String value) {
		instance = value;
	}
	public String getInstance() {
		return instance;
	}
	public int getInstanceIntValue() {
		return new Integer(instance).intValue();
	}
	public void setInfoPort(int value) {
		infoPort = value;
	}
	public int getInfoPort() {
		return infoPort;
	}
	public void setStartTime(long value) {
		startTime = value;
	}
	public long getStartTime() {
		return startTime;
	}
	public Date getStartTimeAsDate() {
		return new Date(startTime);
	}
	public ArrayList<RegisteredServer> getRegistered() {
		return registeredList;
	}
	public RegisteredServer getItem(int index) {
		LOG.info("getItem [index:" + index + "," + registeredList.get(index) + "]");
		return registeredList.get(index);
	}
	public String getHref() {
		String href = String.format("<a href=\"http://%s:%d\">%s</a>",hostname,infoPort,hostname);
		return href;
	}
}

