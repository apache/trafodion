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

package org.trafodion.wms.server;

import java.io.IOException;
import java.io.InputStream;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.BindException;

import java.nio.charset.Charset;

import java.util.Iterator;
import java.util.List;
import java.util.ArrayList;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutionException;

import org.apache.commons.io.IOUtils;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.hadoop.conf.Configuration;

import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.KeeperException;

import org.trafodion.wms.Constants;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.util.VersionInfo;
import org.trafodion.wms.util.InfoServer;
import org.trafodion.wms.zookeeper.ZkClient;
import org.trafodion.wms.zookeeper.ZKConfig;
import org.trafodion.wms.script.ScriptManager;
;
import org.trafodion.wms.server.store.*;

public class WmsServer extends Thread {
	private static  final Log LOG = LogFactory.getLog(WmsServer.class);
	private Thread thrd;
	private ZkClient zkc=null;
    private Configuration conf;
    private InetAddress ia;
	private String serverName;
	private static String masterHostName;
    public static final String SERVER = "server";
	private InfoServer infoServer;
    private ServerManager serverManager;
	private Metrics metrics;
	private long startTime;
    private String parentZnode;
	private int infoPort;
	private String[] args;
	private String instance=null;
    private ExecutorService pool=null;
    private JVMShutdownHook jvmShutdownHook;
    
    private class JVMShutdownHook extends Thread {
    	public void run() {
    		LOG.debug("JVM shutdown hook is running");  
    		try {
    			zkc.close();
    		} catch (InterruptedException ie) {};
    	}
    }	
	
	public WmsServer(String[] args) {
		this.args = args;
	   	conf = WmsConfiguration.create();
		parentZnode = conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);	 
		jvmShutdownHook = new JVMShutdownHook();
		Runtime.getRuntime().addShutdownHook(jvmShutdownHook);	 
		thrd = new Thread(this);
		thrd.start();
	}
	
	private static int findFreePort()throws IOException {
		ServerSocket server = new ServerSocket(0);
		int port = server.getLocalPort();
		server.close();
		return port;
	}
	
	public void run () {
		VersionInfo.logVersion();
		
		Options opt = new Options();
		CommandLine cmd;
		try {
			cmd = new GnuParser().parse(opt, args);
		    //List a = cmd.getArgList();
			instance = cmd.getArgList().get(0).toString();
		} catch (NullPointerException e) {
			LOG.error("No args found: ", e);
			System.exit(1);
		} catch (ParseException e) {
			LOG.error("Could not parse: ", e);
			System.exit(1);
		}

		try {
			zkc = new ZkClient();
			zkc.connect();
			LOG.info("Connected to ZooKeeper");
		} catch (Exception e) {
			LOG.error(e);
			System.exit(1);
		}
		
		metrics = new Metrics();
		startTime = System.currentTimeMillis();
		
		try {	
			//Setup RPC services
			ia = InetAddress.getLocalHost();
			serverName = ia.getCanonicalHostName();

			//Setup script manager
			ScriptManager.getInstance(); 
			
			// Start the info server.
			String bindAddr = conf.get(Constants.WMS_SERVER_INFO_BIND_ADDRESS, Constants.DEFAULT_WMS_SERVER_INFO_BIND_ADDRESS);
			infoPort = conf.getInt(Constants.WMS_SERVER_INFO_PORT, Constants.DEFAULT_WMS_SERVER_INFO_PORT);
			infoPort += Integer.parseInt(instance);		    
			boolean auto = this.conf.getBoolean(Constants.WMS_SERVER_INFO_PORT_AUTO,false);
		    while (true) {
		    	try {
		    		if (infoPort >= 0) {
		    			infoServer = new InfoServer(SERVER, bindAddr, infoPort, false, this.conf);
		    			infoServer.addServlet("status", "/server-status", ServerStatusServlet.class);
		    			infoServer.setAttribute(SERVER, this);
		    			infoServer.start();
		    		} else {
			    		LOG.warn("Http server info port is disabled");
		    		}
		    		break;
		    	} catch (BindException e) {
		    		if (!auto) {
		    			// auto bind disabled throw BindException
		    			throw e;
		    		}
		    		// auto bind enabled, try to use another port
		    		LOG.info("Failed binding http info server to port: " + infoPort);
		    		infoPort++;
		    	}
		    }

			//Setup Hadoop Job Tracker client and Vertica Client
			//if(conf.getBoolean("hadoop.workloads.enabled",false)) {
			//	LOG.info("Hadoop workloads enabled");
				//JTClient jtc = new JTClient();
			//} else {
			//	LOG.info("Hadoop workloads disabled");
			//}
			//if(conf.getBoolean("vertica.workloads.enabled",false)) {	
			//	LOG.info("Vertica workloads enabled");
			//	VerticaClient vc = new VerticaClient();
			//} else {
			//	LOG.info("Vertica workloads disabled");
			//}
			//if(conf.getBoolean(Constants.YARN_REST_ENABLED,false)) {
			//	LOG.info("Yarn workloads enabled");
				//final ExecutorService exService = Executors.newSingleThreadExecutor();
				//final Future<String> callFuture = exService.submit(new YarnClient(new YarnClientExecute());
			//} else {
			//	LOG.info("Yarn workloads disabled");
			//}
		
			pool = Executors.newSingleThreadExecutor();
			serverManager = new ServerManager(this);
		    Future future = pool.submit(serverManager);
		    future.get();
		    
		} catch (Exception e) {
			LOG.error(e);
			e.printStackTrace();
		} finally {
			pool.shutdown();
			System.exit(0);
		}
	}
	
	public ServerManager getServerManager(){
		return serverManager;
	}
	
	public InetAddress getInetAddress(){
		return ia;
	}
	
	public String getInstance(){
		return instance;
	}
	
	public String getMetrics(){
		return metrics.toString();
	}
	
	public long getStartTime(){
		return startTime;
	}
	
	public String getServerName(){
		return serverName;
	}
	
	public String getMasterHostName() {
		return serverManager.getMasterHostName();
	}
	
	public int getInfoPort(){
		return infoPort;
	}
	
	public Configuration getConfiguration(){
		return conf;
	}
	
	public ZkClient getZkClient() {
		return zkc;
	}
	
	public String getZKQuorumServersString() {
		return ZKConfig.getZKQuorumServersString(conf);
	}
	
	public String getZKParentZnode() {
		return parentZnode;
	}
	
	public static void main(String [] args) {
		WmsServer server = new WmsServer(args);
	}
}
