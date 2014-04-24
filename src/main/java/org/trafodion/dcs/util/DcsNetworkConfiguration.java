/**
 *(C) Copyright 2013 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * Copyright 2007 The Apache Software Foundation
 *
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
package org.trafodion.dcs.util;

import java.net.NetworkInterface;
import java.net.InetAddress;
import java.net.InetSocketAddress;

import java.util.Enumeration;
import java.util.Scanner;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.Bytes;
import org.trafodion.dcs.util.DcsConfiguration;
import org.trafodion.dcs.script.ScriptManager;
import org.trafodion.dcs.script.ScriptContext;
import org.trafodion.dcs.server.ServerManager;

public class DcsNetworkConfiguration {

	private static final Log LOG = LogFactory.getLog(DcsNetworkConfiguration.class);
	private static Configuration conf;
	private InetAddress ia;
	private String intHostAddress;
	private String extHostAddress;
	private String canonicalHostName;

	public DcsNetworkConfiguration(Configuration conf) throws Exception {

		this.conf = conf;
		ia = InetAddress.getLocalHost();

		String dcsDnsInterface = conf.get(Constants.DCS_DNS_INTERFACE, Constants.DEFAULT_DCS_DNS_INTERFACE);
		if(dcsDnsInterface.equalsIgnoreCase("default")) {
			intHostAddress = extHostAddress = ia.getHostAddress();
			canonicalHostName = ia.getCanonicalHostName();
			LOG.info("Using local host [" + canonicalHostName + "," + extHostAddress + "]");
		} else {			
			// For all nics get all hostnames and addresses	
			// and try to match against dcs.dns.interface property 
			Enumeration<NetworkInterface> nics = NetworkInterface.getNetworkInterfaces();
			while(nics.hasMoreElements()) {
				NetworkInterface ni = nics.nextElement();
				Enumeration<InetAddress> rawAdrs = ni.getInetAddresses();
				while(rawAdrs.hasMoreElements()) {
					InetAddress inet = rawAdrs.nextElement();
					LOG.info("Found interface [" + ni.getDisplayName() + "," + inet.getCanonicalHostName() + "," + inet.getHostAddress() + "]");
					if( dcsDnsInterface.equalsIgnoreCase(ni.getDisplayName()) && inet.getCanonicalHostName().contains(".") ) {
						intHostAddress = extHostAddress = inet.getHostAddress();
						canonicalHostName = inet.getCanonicalHostName();
						LOG.info("Using interface [" + ni.getDisplayName() + "," + canonicalHostName + "," + extHostAddress + "]");
						ia = inet;
						break;
					}
				}
			}
		}

		checkCloud();
	}

	public void checkCloud() {
		//Ideally we want to use http://jclouds.apache.org/ so we can support all cloud providers.
		//For now, use OpenStack Nova to retrieve int/ext network address map.
		LOG.info("Checking Cloud environment");
		String cloudCommand = conf.get(Constants.DCS_CLOUD_COMMAND, Constants.DEFAULT_DCS_CLOUD_COMMAND);
		ScriptContext scriptContext = new ScriptContext();
		scriptContext.setScriptName("sys_shell.py");
		scriptContext.setCommand(cloudCommand + " | grep -v '^+' | grep -w '" + canonicalHostName + "' | sed 's/.*=\\([0-9.]*\\), \\([0-9.]*\\).*$/\\1,\\2/'");
		LOG.info(scriptContext.getScriptName() + " exec [" + scriptContext.getCommand() + "]");
		ScriptManager.getInstance().runScript(scriptContext);//This will block while script is running

		StringBuilder sb = new StringBuilder();
		sb.append(scriptContext.getScriptName() + " exit code [" + scriptContext.getExitCode() + "]");
		if(! scriptContext.getStdOut().toString().isEmpty()) 
			sb.append(", stdout [" + scriptContext.getStdOut().toString() + "]");
		if(! scriptContext.getStdErr().toString().isEmpty())
			sb.append(", stderr [" + scriptContext.getStdErr().toString() + "]");
		LOG.info(sb.toString());

		if(! scriptContext.getStdOut().toString().isEmpty()){
			Scanner scn = new Scanner(scriptContext.getStdOut().toString());
			scn.useDelimiter(",");
			intHostAddress = scn.next();//internal ip
			extHostAddress = scn.next();//external ip		
			scn.close();
			LOG.info("Cloud environment found");
		} else
			LOG.info("Cloud environment not found");
	}

	public String getHostName() {
		return canonicalHostName;
	}

	public String getIntHostAddress() {
		return intHostAddress;
	}

	public String getExtHostAddress() {
		return extHostAddress;
	}
}
