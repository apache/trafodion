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
package org.trafodion.dcs.server;

import java.net.InetAddress;
import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.util.StringTokenizer;
import java.util.concurrent.Callable;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutionException;
import java.util.Date;
import java.util.Scanner;
import java.text.DateFormat;

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
import org.trafodion.dcs.util.DcsNetworkConfiguration;
import org.trafodion.dcs.zookeeper.ZkClient;
import org.trafodion.dcs.script.ScriptManager;
import org.trafodion.dcs.script.ScriptContext;

public final class ServerManager implements Callable {
	private static final Log LOG = LogFactory.getLog(ServerManager.class);
	private static Configuration conf;
	private static ZkClient zkc;
	private static boolean userProgEnabled;
	private static String userProgHome;
	private static String userProgCommand;
	private static String hostName;
	private static String masterHostName;
	private static long masterStartTime;
	private static int port;
	private static int portRange;
	private static DcsNetworkConfiguration netConf;
	private static String instance;
	private static String registeredPath;
	private static String parentZnode;
	private static int connectingTimeout;
	private static int zkSessionTimeout;
	private static int userProgExitAfterDisconnect;
	private static int infoPort;
	
	class ServerRunner implements Callable<ScriptContext> {
		ScriptContext scriptContext;
		Stat stat=null;

		public ServerRunner() {
			scriptContext = new ScriptContext();
			scriptContext.setHostName(hostName);
			scriptContext.setScriptName("sys_shell.py");
			StringBuilder progParams = new StringBuilder();
			progParams.append(" -ZkHost " + zkc.getZkQuorum());
			progParams.append(" -RZ " + hostName + ":" + instance);
			progParams.append(" -ZkPnode " + "\"" + parentZnode + "\""); 
			progParams.append(" -CNGTO " + connectingTimeout); 
			progParams.append(" -ZKSTO " + zkSessionTimeout);
			progParams.append(" -EADSCO " + userProgExitAfterDisconnect);
			progParams.append(" -TCPADD " + netConf.getExtHostAddress());
			scriptContext.setCommand(userProgCommand + progParams.toString());
		}
		@Override
		public ScriptContext call() throws Exception {
			cleanupZk();
			LOG.info("User program exec [" + scriptContext.getCommand() + "]");
			ScriptManager.getInstance().runScript(scriptContext);//This will block while user prog is running
			LOG.info("User program exit [" + scriptContext.getExitCode()+ "]");
			return scriptContext;
		}
		private void cleanupZk() {
			try {
				stat = zkc.exists(registeredPath,false);
				if(stat != null)  	 
					zkc.delete(registeredPath,-1);
			} catch (Exception e) {
				e.printStackTrace();
				LOG.debug(e);
			}
		}
	}
	
	class ServerMonitor implements Callable<Boolean> {
		ScriptContext scriptContext;
		Stat stat=null;

		public ServerMonitor() {
			scriptContext = new ScriptContext();
			scriptContext.setHostName(hostName);
			scriptContext.setScriptName("sys_shell.py");
		}
		@Override
		public Boolean call() throws Exception {
			Boolean result = Boolean.valueOf(false);
			stat = zkc.exists(registeredPath,false);
			if(stat != null) {	//User program znode found in /registered...check pid
				if(isPid())
					result=true;//User program znode found in /registered...pid found to be running
			}
			return result;
		}
		private boolean isPid() throws Exception {
			String data = Bytes.toString(zkc.getData(registeredPath, false, stat));
			Scanner scn = new Scanner(data);
			scn.useDelimiter(":");
			scn.next();//state
			scn.next();//timestamp
			scn.next();//dialogue Id 
			scn.next();//nid
			String pid = scn.next();//pid
			scn.close();
			scriptContext.setCommand("ps -p " + pid);
			ScriptManager.getInstance().runScript(scriptContext);
			return scriptContext.getExitCode() != 0 ? false: true;
		}
	}

	public ServerManager(Configuration conf,ZkClient zkc,DcsNetworkConfiguration netConf,String instance,int infoPort) throws Exception {
		this.conf = conf;
		this.zkc = zkc;
		this.netConf = netConf;
		this.hostName = netConf.getHostName();
		this.instance = instance;
		this.infoPort = infoPort;
		this.parentZnode = this.conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);	   	
		this.registeredPath = this.parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED + "/" + hostName + ":" + instance; 
		this.connectingTimeout = this.conf.getInt(Constants.DCS_SERVER_USER_PROGRAM_CONNECTING_TIMEOUT,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_CONNECTING_TIMEOUT);	   	
		this.zkSessionTimeout = this.conf.getInt(Constants.DCS_SERVER_USER_PROGRAM_ZOOKEEPER_SESSION_TIMEOUT,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_ZOOKEEPER_SESSION_TIMEOUT);	   	
		this.userProgExitAfterDisconnect = this.conf.getInt(Constants.DCS_SERVER_USER_PROGRAM_EXIT_AFTER_DISCONNECT,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_EXIT_AFTER_DISCONNECT);

	}
	@Override
	public Boolean call() throws Exception {
	    ExecutorService pool = Executors.newSingleThreadExecutor();
	    
		try {
			getMaster();
			registerInRunning();
			featureCheck();
			
			Callable<Boolean> serverMonitor = new ServerMonitor();
			Callable<ScriptContext> serverRunner = new ServerRunner();
			
			long timeoutMillis=5000;
			
			while(true) {
				Future<Boolean> monitor = pool.submit(serverMonitor);
				if(false == monitor.get().booleanValue()) { //blocking call
					LOG.info("User program is not running");
					Future<ScriptContext> runner = pool.submit(serverRunner);
					ScriptContext scriptContext = runner.get();//blocking call
					
					StringBuilder sb = new StringBuilder();
					sb.append("exit code [" + scriptContext.getExitCode() + "]");
					if(! scriptContext.getStdOut().toString().isEmpty()) 
						sb.append(", stdout [" + scriptContext.getStdOut().toString() + "]");
					if(! scriptContext.getStdErr().toString().isEmpty())
						sb.append(", stderr [" + scriptContext.getStdErr().toString() + "]");
					LOG.info(sb.toString());
					
					switch(scriptContext.getExitCode()) {
					case 3:
						LOG.error("Trafodion is not running");
						timeoutMillis=60000; 
						break;
					case 127:
						LOG.error("Cannot find user program executable");
						timeoutMillis=60000;
						break;
					default:
						timeoutMillis=5000;
					}

				} else {
					timeoutMillis=5000;
				}

				try {
					Thread.sleep(timeoutMillis);
				} catch (InterruptedException e) {	}
			}
			
		} catch (Exception e) {
			e.printStackTrace();
			LOG.error(e);
			pool.shutdown();
			throw e;
		}
	}
	private void featureCheck() {
		final String msg1 = 
			"Property " + Constants.DCS_SERVER_USER_PROGRAM  + " is false. "
			+ "Please add to your dcs-site.xml file and set <value>false</value> to <value>true</value>.";
		final String msg2 = 
			"Environment variable $MY_SQROOT is not set.";

		boolean ready=false;
		while(! ready) {
			userProgEnabled = conf.getBoolean(Constants.DCS_SERVER_USER_PROGRAM,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM);
			userProgHome = System.getProperty("dcs.user.program.home");
			userProgCommand = conf.get(Constants.DCS_SERVER_USER_PROGRAM_COMMAND,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_COMMAND);

			if(userProgEnabled == true && userProgHome.isEmpty() == false && userProgCommand.isEmpty() == false) {
				ready=true;
				continue;
			} 
			
			if(userProgEnabled == false)
				LOG.error(msg1);
			if(userProgHome.isEmpty())
				LOG.error(msg2);
	
			try {
				Thread.sleep(60000);
			} catch (InterruptedException e) {	}
		}

		LOG.info("User program enabled");
	}
 
	private void getMaster(){
		boolean found=false;

		while(! found){
			try {
				Stat stat = zkc.exists(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER,false);
				if(stat != null) {
					List<String> nodes = zkc.getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER,null);
					if( ! nodes.isEmpty()) {
						StringTokenizer st = new StringTokenizer(nodes.get(0), ":"); 
						while(st.hasMoreTokens()) { 
							masterHostName=st.nextToken();
							port=Integer.parseInt(st.nextToken());
							portRange=Integer.parseInt(st.nextToken());
							masterStartTime=Long.parseLong(st.nextToken());
						}
						found=true;
					}
				}

				if(! found){
					try {
						Thread.sleep(5000);
					} catch (InterruptedException e) {	}
				}

			} catch (Exception e) {
				e.printStackTrace();
				LOG.error(e);
			}
		}
	}
	private void registerInRunning() {
		String znode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING + "/" + hostName + ":" + instance + ":" + infoPort + ":" + System.currentTimeMillis(); 
		try {
			Stat stat = zkc.exists(znode,false);
			if(stat == null) {
				zkc.create(znode,new byte[0],ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.EPHEMERAL);
				LOG.info("Created znode [" + znode + "]");
			}
		} catch (KeeperException.NodeExistsException e) {
			//do nothing...leftover from previous shutdown
		} catch (Exception e) {
			e.printStackTrace();
			LOG.error(e);
		}
	}
	public String getMasterHostName(){
		return masterHostName;
	}
	
	public String getZKParentZnode(){
		return parentZnode;
	}
}
