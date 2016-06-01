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

//package org.apache.hadoop.hbase.rest;
package org.trafodion.wms.rest;

import java.util.*;
import java.util.Map.Entry;
import java.io.IOException;
import java.lang.InterruptedException;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.PosixParser;
import org.apache.commons.cli.ParseException;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.net.DNS;

import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.util.Strings;
import org.trafodion.wms.util.VersionInfo;

import org.mortbay.jetty.Connector;
import org.mortbay.jetty.Server;
import org.mortbay.jetty.nio.SelectChannelConnector;
import org.mortbay.jetty.servlet.Context;
import org.mortbay.jetty.servlet.ServletHolder;
import org.mortbay.thread.QueuedThreadPool;

import com.sun.jersey.api.json.JSONConfiguration;
import com.sun.jersey.spi.container.servlet.ServletContainer;

public class WmsRest implements Runnable, RestConstants {  
	private static final Log LOG = LogFactory.getLog(WmsRest.class.getName());
	private Configuration conf;
	private Thread thrd;
	private String[] args;
	private RESTServlet servlet = null;
	
	private static void printUsageAndExit(Options options, int exitCode) {
		HelpFormatter formatter = new HelpFormatter();
		formatter.printHelp("bin/wms rest start", "", options,
				"\nTo run the REST server as a daemon, execute " +
				"bin/wms-daemon.sh start|stop rest [--infoport <port>] [-p <port>] [-ro]\n", true);
		System.exit(exitCode);
	}
	
	public WmsRest(String[] args) {
		this.args = args;
		conf = WmsConfiguration.create();
		
		Options options = new Options();
		options.addOption("p", "port", true, "Port to bind to [default: 8080]");
		options.addOption("ro", "readonly", false, "Respond only to GET HTTP " +
		"method requests [default: false]");
		options.addOption(null, "infoport", true, "Port for web UI");
		
		try {
			servlet = RESTServlet.getInstance(conf);
		} catch (IOException e) {
			LOG.error("Exception " + e);
			e.printStackTrace();
		}
		
		CommandLine commandLine = null;
		try {
			commandLine = new PosixParser().parse(options, args);
		} catch (ParseException e) {
			LOG.error("Could not parse: ", e);
			printUsageAndExit(options, -1);
		}

		// check for user-defined port setting, if so override the conf
		if (commandLine != null && commandLine.hasOption("port")) {
			String val = commandLine.getOptionValue("port");
			servlet.getConfiguration().setInt("wms.rest.port", Integer.valueOf(val));
			LOG.debug("port set to " + val);
		}

		// check if server should only process GET requests, if so override the conf
		if (commandLine != null && commandLine.hasOption("readonly")) {
			servlet.getConfiguration().setBoolean("wms.rest.readonly", true);
			LOG.debug("readonly set to true");
		}

		// check for user-defined info server port setting, if so override the conf
		if (commandLine != null && commandLine.hasOption("infoport")) {
			String val = commandLine.getOptionValue("infoport");
			servlet.getConfiguration().setInt("wms.rest.info.port", Integer.valueOf(val));
			LOG.debug("Web UI port set to " + val);
		}

		@SuppressWarnings("unchecked")
		List<String> remainingArgs = commandLine != null ?	commandLine.getArgList() : new ArrayList<String>();
		if (remainingArgs.size() != 1) {
			printUsageAndExit(options, 1);
		}

		String command = remainingArgs.get(0);
		if ("start".equals(command)) {
			// continue and start container
		} else if ("stop".equals(command)) {
			System.exit(1);
		} else {
			printUsageAndExit(options, 1);
		}
		
		thrd = new Thread(this);
		thrd.start();	
	}
	
	public WmsRest(Configuration conf) {
		try {
			servlet = RESTServlet.getInstance(conf);
		} catch (IOException e) {
			LOG.error("Exception " + e);
			e.printStackTrace();
			return;
		}
		
		thrd = new Thread(this);
		thrd.start();	
	}

	public void run() {
		VersionInfo.logVersion();
		
		// set up the Jersey servlet container for Jetty
		ServletHolder sh = new ServletHolder(ServletContainer.class);
		sh.setInitParameter(
				"com.sun.jersey.config.property.resourceConfigClass",
				ResourceConfig.class.getCanonicalName());
		sh.setInitParameter("com.sun.jersey.config.property.packages","org.trafodion.wms.rest");

		// set up Jetty and run the embedded server
		Server server = new Server();

		Connector connector = new SelectChannelConnector();
		connector.setPort(servlet.getConfiguration().getInt("wms.rest.port", 8080));
		connector.setHost(servlet.getConfiguration().get("wms.rest.host", "0.0.0.0"));

		server.addConnector(connector);

		// Set the default max thread number to 100 to limit
		// the number of concurrent requests so that REST server doesn't OOM easily.
		// Jetty set the default max thread number to 250, if we don't set it.
		//
		// Our default min thread number 2 is the same as that used by Jetty.
		int maxThreads = servlet.getConfiguration().getInt("wms.rest.threads.max", 100);
		int minThreads = servlet.getConfiguration().getInt("wms.rest.threads.min", 2);
		QueuedThreadPool threadPool = new QueuedThreadPool(maxThreads);
		threadPool.setMinThreads(minThreads);
		server.setThreadPool(threadPool);
		server.setSendServerVersion(false);
		server.setSendDateHeader(false);
		server.setStopAtShutdown(true);

		Context context = new Context(server, "/", Context.SESSIONS);
		context.addServlet(sh, "/*");
		
		try {
			server.start();
			server.join();
		} catch (InterruptedException e) {
			LOG.error("InterruptedException " + e);
		} catch (Exception e) {
			LOG.error("Exception " + e);
		}
	}

	public static void main(String[] args) throws Exception {
		WmsRest server = new WmsRest(args);
	}
}