package org.trafodion.wms.server;

import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.KeeperException;

import java.io.*;
import java.net.InetAddress;

import java.util.*;
import java.util.concurrent.Callable;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutionException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.hadoop.conf.Configuration;

import org.apache.thrift.TBase;
import org.apache.thrift.TException;
import org.apache.thrift.TDeserializer;
import org.apache.thrift.transport.TSSLTransportFactory;
import org.apache.thrift.transport.TTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSSLTransportFactory.TSSLTransportParameters;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;

import org.trafodion.wms.thrift.generated.Action;
import org.trafodion.wms.thrift.generated.Data;
import org.trafodion.wms.thrift.generated.Header;
import org.trafodion.wms.thrift.generated.IllegalArgument;
import org.trafodion.wms.thrift.generated.IOError;
import org.trafodion.wms.thrift.generated.Operation;
import org.trafodion.wms.thrift.generated.Request;
import org.trafodion.wms.thrift.generated.Response;
import org.trafodion.wms.thrift.generated.WmsService;
import org.trafodion.wms.thrift.generated.Rule;
import org.trafodion.wms.thrift.generated.Stream;

import org.trafodion.wms.Constants;
import org.trafodion.wms.util.Bytes;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.zookeeper.ZkClient;
import org.trafodion.wms.server.stats.PStats;
import org.trafodion.wms.server.rpc.thrift.ThriftRpcServer;
import org.trafodion.wms.server.workload.WorkloadStore;
import org.trafodion.wms.cep.ComplexEventProcessor;

public final class ServerManager implements Callable {
	private static  final Log LOG = LogFactory.getLog(ServerManager.class);
	private WmsServer wmsServer;
	private static Configuration conf;
	private String instance;
    private ZkClient zkc = null;
    private InetAddress ia;
    private int thriftPort;
    private int infoPort;
	private static String serverName;
	private static String masterHostName;
	private static int masterThriftPort;
	private static long masterStartTime;
	private ThriftRpcServer trpcs;
	private static String parentZnode;
    private ServerLeaderElection sle = null;
    private PStats pstats = null;
    private WorkloadStore workloadStore;
    private ComplexEventProcessor cep;
    private TDeserializer deserializer = new TDeserializer(new TBinaryProtocol.Factory());

    public ServerManager(WmsServer wmsServer) throws Exception  {
    	this.wmsServer = wmsServer;
    	this.conf = wmsServer.getConfiguration();
    	this.instance = wmsServer.getInstance();
    	this.zkc = wmsServer.getZkClient();
    	this.ia = wmsServer.getInetAddress();
    	this.thriftPort = wmsServer.getThriftPort();
		this.infoPort = wmsServer.getInfoPort();
		this.serverName = wmsServer.getServerName();
		this.parentZnode = wmsServer.getZKParentZnode();
    }
   
	@Override
	public Boolean call() throws Exception {
	    ExecutorService pool = Executors.newSingleThreadExecutor();
	    
		try {
			getMaster();
			registerInRunning();
			sle = new ServerLeaderElection(this);
 			workloadStore = new WorkloadStore(conf,zkc,parentZnode,sle);
			cep = new ComplexEventProcessor(zkc,parentZnode,conf,sle); 
 	   		pstats = new PStats(conf,instance);
			trpcs = new ThriftRpcServer(wmsServer);
			
			//Callable<Boolean> serverMonitor = new ServerMonitor();
			//Callable<ScriptContext> serverRunner = new ServerRunner();
			
			long timeoutMillis=5000;
			
			while(true) {
/*
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
*/
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
    
	private void getMaster(){
		boolean found=false;

		while(! found){
			try {
				Stat stat = zkc.exists(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER,false);
				if(stat != null) {
					List<String> nodes = zkc.getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER,null);
					StringTokenizer st = new StringTokenizer(nodes.get(0), ":"); 
					while(st.hasMoreTokens()) { 
						masterHostName=st.nextToken();
						masterThriftPort=Integer.parseInt(st.nextToken());
						masterStartTime=Long.parseLong(st.nextToken());
					}
					found=true;
				} else {
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
		String znode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING + "/" + serverName + ":" + instance + ":" + thriftPort + ":" + infoPort + ":" + System.currentTimeMillis(); 
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
	
    public String getInstance(){
    	return instance;
    }
    
    public String getHostName(){
    	return ia.getHostName();
    }
   
    public int getThriftPort(){
    	return thriftPort;
    }
    
    public ZkClient getZkClient(){
    	return zkc;
    }
    
	public String getMasterHostName(){
		return masterHostName;
	}
	
	public int getMasterThriftPort(){
		return masterThriftPort;
	}
	
	public String getZKParentZnode(){
		return parentZnode;
	}
	public WorkloadStore getWorkloadStore(){
		return workloadStore;
	}
	public ComplexEventProcessor getComplexEventProcessor(){
		return cep;
	}
}

