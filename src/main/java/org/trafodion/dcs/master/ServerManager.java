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
package org.trafodion.dcs.master;

import java.net.InetAddress;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileNotFoundException;

import java.util.Scanner;
import java.util.Collections;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Queue;
import java.util.List;
import java.util.ArrayList;
import java.util.concurrent.Callable;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutionException;
import java.util.Date;
import java.util.Comparator;

import java.text.DateFormat;

import org.apache.zookeeper.*;
import org.apache.zookeeper.data.Stat;

import org.apache.hadoop.conf.Configuration;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.trafodion.dcs.master.Server;
import org.trafodion.dcs.master.Metrics;
import org.trafodion.dcs.script.ScriptManager;
import org.trafodion.dcs.script.ScriptContext;
import org.trafodion.dcs.Constants;
import org.trafodion.dcs.zookeeper.ZkClient;
import org.trafodion.dcs.util.DcsConfiguration;
import org.trafodion.dcs.util.DcsNetworkConfiguration;
import org.trafodion.dcs.util.RetryCounter;
import org.trafodion.dcs.util.RetryCounterFactory;

public class ServerManager implements Callable {
	private static  final Log LOG = LogFactory.getLog(ServerManager.class);
    private Configuration conf;
    private DcsNetworkConfiguration netConf;
    private ZkClient zkc = null;
    private long startupTimestamp;
    private int maxRestartAttempts;
    private int retryIntervalMillis;
    private ExecutorService pool = null;
    private Metrics metrics;
    private String parentZnode;
	private RetryCounterFactory retryCounterFactory;
	private final ArrayList<String> configuredServers = new ArrayList<String>();
	private final ArrayList<String> runningServers = new ArrayList<String>();
	private final ArrayList<String> registeredServers = new ArrayList<String>();
	private final Queue<RestartHandler> restartQueue = new LinkedList<RestartHandler>();
	
	public ServerManager(Configuration conf, ZkClient zkc,
			DcsNetworkConfiguration netConf,long startupTimestamp, Metrics metrics) throws Exception {
		try {
			this.conf = conf;
			this.zkc = zkc;
			this.netConf = netConf;
			this.startupTimestamp = startupTimestamp;
			this.metrics = metrics;
			maxRestartAttempts = conf.getInt(Constants.DCS_MASTER_SERVER_RESTART_HANDLER_ATTEMPTS,Constants.DEFAULT_DCS_MASTER_SERVER_RESTART_HANDLER_ATTEMPTS);
			retryIntervalMillis = conf.getInt(Constants.DCS_MASTER_SERVER_RESTART_HANDLER_RETRY_INTERVAL_MILLIS,Constants.DEFAULT_DCS_MASTER_SERVER_RESTART_HANDLER_RETRY_INTERVAL_MILLIS);
			retryCounterFactory = new RetryCounterFactory(maxRestartAttempts, retryIntervalMillis);
			parentZnode = conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);
			pool = Executors.newSingleThreadExecutor();
		} catch ( Exception e ) {
			e.printStackTrace();
			LOG.error(e);
			throw e;
		}
	}
	
	class RestartHandler implements Callable<ScriptContext> {
		private ScriptContext scriptContext = new ScriptContext();
		private String znodePath;

		public RestartHandler(String znodePath) {
			this.znodePath = znodePath;
		}
		
		@Override
		public ScriptContext call() throws Exception {
			try {
				Scanner scn = new Scanner(znodePath);
				scn.useDelimiter(":");
				String hostName = scn.next();//host name
				String instance = scn.next();//instance
				int infoPort = Integer.parseInt(scn.next()); //UI port
				long serverStartTimestamp = Long.parseLong(scn.next());
				scn.close();
				
				//Get the --config property from classpath...it's always first in the classpath
				String cp = System.getProperty("java.class.path");
				scn = new Scanner(cp);
				scn.useDelimiter(":");
				String confDir = scn.next();
				scn.close();
				LOG.debug("conf dir [" + confDir + "]");

				//Get -Ddcs.home.dir
				String dcsHome = System.getProperty("dcs.home.dir");				

     			//If stop-dcs.sh is executed and DCS_MANAGES_ZK then zookeeper is stopped abruptly.
    			//Second scenario is when ZooKeeper fails for some reason regardless of whether DCS
    			//manages it. When either happens the DcsServer running znodes still exist in ZooKeeper
    			//and we see them at next startup. When they eventually timeout
    			//we get node deleted events for a server that no longer exists. So, only recognize
    			//DcsServer running znodes that have timestamps after last DcsMaster startup.
				if(serverStartTimestamp > startupTimestamp){
					scriptContext.setHostName(hostName);
					scriptContext.setScriptName("sys_shell.py");
					if(hostName.equalsIgnoreCase(netConf.getHostName())) 
						scriptContext.setCommand("bin/dcs-daemon.sh --config " + confDir + " start server " + instance);
					else
						scriptContext.setCommand("pdsh -w " + hostName + " \"cd " + dcsHome + ";bin/dcs-daemon.sh --config " + confDir + " start server " + instance + "\"");
					
					RetryCounter retryCounter = retryCounterFactory.create();
					while(true) {
						if(scriptContext.getStdOut().length() > 0)
							scriptContext.getStdOut().delete(0,scriptContext.getStdOut().length());
						if(scriptContext.getStdErr().length() > 0)
							scriptContext.getStdErr().delete(0,scriptContext.getStdErr().length());
						LOG.info("Restarting DcsServer [" + hostName + ":" + instance + "], script [ " + scriptContext.toString() + " ]");
						ScriptManager.getInstance().runScript(scriptContext);
						
						if(scriptContext.getExitCode() == 0) {
							LOG.info("DcsServer [" + hostName + ":" + instance + "] restarted");
							break;
						} else {
							StringBuilder sb = new StringBuilder();
							sb.append("exit code [" + scriptContext.getExitCode() + "]");
							if(! scriptContext.getStdOut().toString().isEmpty()) 
								sb.append(", stdout [" + scriptContext.getStdOut().toString() + "]");
							if(! scriptContext.getStdErr().toString().isEmpty())
								sb.append(", stderr [" + scriptContext.getStdErr().toString() + "]");
							LOG.error(sb.toString());
							
							if (! retryCounter.shouldRetry()) {
								LOG.error("DcsServer [" + hostName + ":" + instance + "] restart failed after "
										+ retryCounter.getMaxRetries() + " retries");
								break;
							} else {
								retryCounter.sleepUntilNextRetry();
								retryCounter.useRetry();
							}
						}
					}
				} else {
					LOG.debug("No restart for " + znodePath + "\nbecause DcsServer start time [" + DateFormat.getDateTimeInstance().format(new Date(serverStartTimestamp)) + "] was before DcsMaster start time [" + DateFormat.getDateTimeInstance().format(new Date(startupTimestamp)) + "]");
				}
			} catch (Exception e) {
				e.printStackTrace();
				LOG.error(e);
			}
			
			return scriptContext;
		}
	}
	
	class RunningWatcher implements Watcher {
		public void process(WatchedEvent event) {
			if(event.getType() == Event.EventType.NodeChildrenChanged) {
				LOG.debug("Running children changed [" + event.getPath() + "]");
				try {
					getZkRunning();
				} catch (Exception e) {
					e.printStackTrace();
					LOG.error(e);
				}
			} else if(event.getType() == Event.EventType.NodeDeleted) {
				String znodePath = event.getPath();
				LOG.debug("Running znode deleted [" + znodePath + "]");
				try {
					restartServer(znodePath);
				} catch (Exception e) {
					e.printStackTrace();
					LOG.error(e);
				}
			}
		}
	}
	
	class RegisteredWatcher implements Watcher {
		public void process(WatchedEvent event) {
			if(event.getType() == Event.EventType.NodeChildrenChanged) {
				LOG.debug("Registered children changed [" + event.getPath() + "]");
				try {
					getZkRegistered();
				} catch (Exception e) {
					e.printStackTrace();
					LOG.error(e);
				}
			}
		}
	}

	@Override
	public Boolean call() throws Exception {

		long timeoutMillis=5000;

		try {
			getServersFile();
			getZkRunning();
			getZkRegistered();

			while (true) {
				while (! restartQueue.isEmpty()) {
					LOG.debug("Restart queue size [" + restartQueue.size() + "]");
					RestartHandler handler = restartQueue.poll();
					Future<ScriptContext> runner = pool.submit(handler);
					ScriptContext scriptContext = runner.get();//blocking call
					if(scriptContext.getExitCode() != 0)  
						restartQueue.add(handler);
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
	
	private List<String> getChildren(String znode,Watcher watcher) throws Exception {
		List<String> children=null;
		children = zkc.getChildren(znode,watcher);
        if( ! children.isEmpty()) 
        	Collections.sort(children);
		return children;
	}
    
    private void getServersFile() throws Exception {
    	InputStream is = this.getClass().getResourceAsStream("/servers");
    	if(is == null)
    		throw new IOException("Cannot find servers file");

    	BufferedReader br = new BufferedReader(new InputStreamReader(is));
    	configuredServers.clear();
    	String line;
    	while((line = br.readLine()) != null) {
    		configuredServers.add(line);
     	}

    	Collections.sort(configuredServers);

    	if(configuredServers.size() < 1)
    		throw new IOException("No entries found in servers file");

    	int lnum=1;
    	for(int i=0; i < configuredServers.size(); i++) {
     		LOG.debug("servers file line " + lnum + " [" + configuredServers.get(i) + "]");
     		lnum++;
    	}
    }
	
    private synchronized void getZkRunning() throws Exception {
    	LOG.debug("Reading " + parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING);
    	List<String> children =  getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING, new RunningWatcher());

    	if( ! children.isEmpty()) {
    		for(String child : children) {
     			//If stop-dcs.sh is executed and DCS_MANAGES_ZK then zookeeper is stopped abruptly.
    			//Second scenario is when ZooKeeper fails for some reason regardless of whether DCS
    			//manages it. When either happens the DcsServer running znodes still exist in ZooKeeper
    			//and we see them at next startup. When they eventually timeout
    			//we get node deleted events for a server that no longer exists. So, only recognize
    			//DcsServer running znodes that have timestamps after last DcsMaster startup.
				Scanner scn = new Scanner(child);
				scn.useDelimiter(":");
				String hostName = scn.next(); 
				String instance = scn.next(); 
				int infoPort = Integer.parseInt(scn.next()); 
				long serverStartTimestamp = Long.parseLong(scn.next());
				scn.close();
				
    			if(serverStartTimestamp < startupTimestamp) 
    				continue;

    			if(! runningServers.contains(child)) {
    				LOG.debug("Watching running [" + child + "]");
    				zkc.exists(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING + "/" + child, new RunningWatcher());
    				runningServers.add(child);
    			}
    		}
    		metrics.setTotalRunning(runningServers.size());
    	} else {
    		metrics.setTotalRunning(0);
    	}
    }
	
	private synchronized void restartServer(String znodePath) throws Exception {
		String child = znodePath.replace(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING + "/","");
		Scanner scn = new Scanner(child);
		scn.useDelimiter(":");
		String hostName = scn.next(); 
		String instance = scn.next(); 
		int infoPort = Integer.parseInt(scn.next()); 
		long serverStartTimestamp = Long.parseLong(scn.next());
		scn.close();	
		
		LOG.error("DcsServer [" + hostName + ":" + instance + "] failed.");
		
		if(runningServers.contains(child)) {
			LOG.debug("Found [" + child + "], deleting from running servers list");
			runningServers.remove(child);
			metrics.setTotalRunning(runningServers.size());
		}

		RestartHandler handler = new RestartHandler(child);
		restartQueue.add(handler);
	}
	
	private synchronized void getZkRegistered() throws Exception {
		LOG.debug("Reading " + parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED);
        List<String> children =  getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED, new RegisteredWatcher());

        if( ! children.isEmpty()) {
            registeredServers.clear();
        	for(String child : children) {
        			LOG.debug("Registered [" + child + "]");
        			registeredServers.add(child);
        	}
           	metrics.setTotalRegistered(registeredServers.size());
        } else {
        	metrics.setTotalRegistered(0);
        }
	}

	public synchronized List<Server> getServersList() {
		ArrayList<Server> serverList = new ArrayList<Server>();
		Stat stat = null;
		byte[] data = null;
		
		int totalAvailable = 0;
		int totalConnecting = 0;
		int totalConnected = 0;
		
		LOG.debug("Begin getServersList()");
		
		if( ! runningServers.isEmpty()) {
			for(String aRunningServer : runningServers) {
				Server aServer = new Server();
				Scanner scn = new Scanner(aRunningServer);
				scn.useDelimiter(":");
				aServer.setHostname(scn.next());
				aServer.setInstance(scn.next());
				aServer.setInfoPort(Integer.parseInt(scn.next()));
				aServer.setStartTime(Long.parseLong(scn.next()));
				scn.close();
				
				if( ! registeredServers.isEmpty()) {
					for(String aRegisteredServer : registeredServers) {
						if(aRegisteredServer.equals(aServer.getHostname() + ":" + aServer.getInstance())){
							try {
								stat = zkc.exists(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED + "/" + aRegisteredServer,false);
								if(stat != null) {
									data = zkc.getData(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED + "/" + aRegisteredServer, false, stat);
									scn = new Scanner(new String(data));
									scn.useDelimiter(":");
									LOG.debug("getDataRegistered [" + new String(data) + "]");
									aServer.setState(scn.next());
									String state = aServer.getState();
									if(state.equals("AVAILABLE"))
										totalAvailable += 1;
									else if(state.equals("CONNECTING"))
										totalConnecting += 1;
									else if(state.equals("CONNECTED"))
										totalConnected += 1;
									aServer.setTimestamp(Long.parseLong(scn.next()));
									aServer.setDialogueId(scn.next());  
									aServer.setNid(scn.next());
									aServer.setPid(scn.next());
									aServer.setProcessName(scn.next());
									aServer.setIpAddress(scn.next());
									aServer.setPort(scn.next());
									aServer.setClientName(scn.next());
									aServer.setClientIpAddress(scn.next());
									aServer.setClientPort(scn.next());
									aServer.setClientAppl(scn.next());
									scn.close();
								}
							} catch (Exception e) {
								e.printStackTrace();
								LOG.error("Exception: " + e.getMessage());
							}
						
							aServer.setIsRegistered();
						}
					}
				}
				
				serverList.add(aServer);
			}
		}
		
		metrics.setTotalAvailable(totalAvailable);
		metrics.setTotalConnecting(totalConnecting);
		metrics.setTotalConnected(totalConnected);
		
		Collections.sort(serverList, new Comparator<Server>(){
		     public int compare(Server s1, Server s2){
		         if(s1.getInstanceIntValue() == s2.getInstanceIntValue())
		             return 0;
		         return s1.getInstanceIntValue() < s2.getInstanceIntValue() ? -1 : 1;
		     }
		});
		
		LOG.debug("End getServersList()");
		
		return serverList;
	}
}

