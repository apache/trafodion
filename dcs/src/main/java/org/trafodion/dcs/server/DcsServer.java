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
package org.trafodion.dcs.server;

import java.io.IOException;
import java.io.InputStream;
import java.net.NetworkInterface;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.BindException;
import java.nio.charset.Charset;
import java.util.Enumeration;
import java.util.StringTokenizer;
import java.util.Collections;
import java.util.List;
import java.util.Iterator;
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
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.KeeperException;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.Bytes;
import org.trafodion.dcs.util.DcsConfiguration;
import org.trafodion.dcs.util.VersionInfo;
import org.trafodion.dcs.util.InfoServer;
import org.trafodion.dcs.util.DcsNetworkConfiguration;
import org.trafodion.dcs.zookeeper.ZkClient;
import org.trafodion.dcs.zookeeper.ZKConfig;

public final class DcsServer implements Runnable {
	private static final Log LOG = LogFactory.getLog(DcsServer.class);
	private Thread thrd;
	private ZkClient zkc = null;
	private int zkSessionTimeout;
    private Configuration conf;
    private DcsNetworkConfiguration netConf;
	private String[] args;
	private String instance = null;
	private int childServers;
	private long startTime;
	private String serverName;
	private InfoServer infoServer;
	private int infoPort;
    public static final String SERVER = "server";
    private Metrics metrics;
    private ServerManager serverManager;
    private ExecutorService pool=null;
    private JVMShutdownHook jvmShutdownHook;
	private static String trafodionLog;
    
    private class JVMShutdownHook extends Thread {
    	public void run() {
    		LOG.debug("JVM shutdown hook is running");  
    		try {
    			zkc.close();
    		} catch (InterruptedException ie) {};
    	}
    }
    
	public DcsServer(String[] args) {
		this.args = args;
	   	conf = DcsConfiguration.create();
		jvmShutdownHook = new JVMShutdownHook();
		Runtime.getRuntime().addShutdownHook(jvmShutdownHook);
		thrd = new Thread(this);
		thrd.start();
	}
	
	public void run () {
	
		VersionInfo.logVersion();
		
	   	Options opt = new Options();
		CommandLine cmd;
		try {
			cmd = new GnuParser().parse(opt, args);
			LOG.debug("args [" + cmd.getArgs().length + "]");
			instance = cmd.getArgList().get(0).toString();
			if(cmd.getArgs().length > 2)
				childServers = Integer.parseInt(cmd.getArgList().get(1).toString());
			else
				childServers = 1;
		} catch (NullPointerException e) {
			LOG.error("No args found: ", e);
			System.exit(1);
		} catch (ParseException e) {
			LOG.error("Could not parse: ", e);
			System.exit(1);
		}
		
		trafodionLog = System.getProperty(Constants.DCS_TRAFODION_LOG);

		try {
			zkc = new ZkClient();	   
			zkc.connect();
			LOG.info("Connected to ZooKeeper");
		} catch (Exception e) {
			LOG.error(e);
			System.exit(1);
		}
		
		Runtime.getRuntime().addShutdownHook(new Thread() {
			public void run() {
				System.out.println("Shutdown Hook is running");
				try {
					zkc.close();
				} catch (InterruptedException ie) {};
			}
		});
		
		metrics = new Metrics();
		startTime=System.currentTimeMillis();

		try {
			
		   	netConf = new DcsNetworkConfiguration(conf);
			serverName = netConf.getHostName();
			
			// Start the info server.
			String bindAddr = conf.get(Constants.DCS_SERVER_INFO_BIND_ADDRESS, Constants.DEFAULT_DCS_SERVER_INFO_BIND_ADDRESS);
			infoPort = conf.getInt(Constants.DCS_SERVER_INFO_PORT, Constants.DEFAULT_DCS_SERVER_INFO_PORT);
			infoPort += Integer.parseInt(instance);		    
			boolean auto = this.conf.getBoolean(Constants.DCS_SERVER_INFO_PORT_AUTO,false);
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

			pool = Executors.newSingleThreadExecutor();
			serverManager = new ServerManager(conf,zkc,netConf,instance,infoPort,childServers);
		    Future future = pool.submit(serverManager);
		    future.get();
		} catch (Exception e) {
			LOG.error(e);
			e.printStackTrace();
		} finally {
			if(pool != null)
				pool.shutdown();
			System.exit(0);
		}
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
	
	public InfoServer getInfoServer(){
		return infoServer;
	}
	
	public int getInfoPort(){
		return infoPort;
	}
	
	public Configuration getConfiguration(){
		return conf;
	}
	
	public String getZKQuorumServersString() {
		return ZKConfig.getZKQuorumServersString(conf);
	}
	
	public String getZKParentZnode() {
		return serverManager.getZKParentZnode();
	}
	
	public String getUserProgramHome() {
		return serverManager.getUserProgramHome();
	}
	
	public String getTrafodionLog() {
		return trafodionLog;
	}
	
	public static void main(String [] args) {
		DcsServer server = new DcsServer(args);
	}
}
