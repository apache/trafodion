package org.trafodion.wms.client;

import java.io.*;
import java.net.*;
import java.nio.charset.Charset;
import java.util.*;

import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.CreateMode;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.thrift.TException;
import org.apache.thrift.transport.TSSLTransportFactory;
import org.apache.thrift.transport.TTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSSLTransportFactory.TSSLTransportParameters;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.hadoop.conf.Configuration;

import org.trafodion.wms.MasterNotRunningException;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.Constants;
import org.trafodion.wms.zookeeper.ZkClient;
import org.trafodion.wms.thrift.generated.WmsAdminService;
import org.trafodion.wms.thrift.generated.Stream;
import org.trafodion.wms.thrift.generated.StreamResponse;
import org.trafodion.wms.thrift.generated.Rule;
import org.trafodion.wms.thrift.generated.RuleResponse;
import org.trafodion.wms.thrift.generated.WorkloadResponse;

public class WmsAdmin {
	private static final Log LOG = LogFactory.getLog(WmsAdmin.class);
	private static final Charset CHARSET = Charset.forName("UTF-8");
	private static ZkClient zkc = null;
	private boolean isOpen = false;
	private String serverIpAddress;
	private String serverInstance;
	private int thriftPort=0;
	private long startTime=0L;
	private String clientZnode;
	private String clientIpAddress;
	private long clientTimestamp=0L;
	private Configuration conf;
	private String parentZnode;
    private JVMShutdownHook jvmShutdownHook;
	private TFramedTransport transport;
	private TProtocol protocol;
	private WmsAdminService.Client tClient;
	
	public WmsAdmin(Configuration conf) {
		this.conf = conf;
		parentZnode = conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);	
		jvmShutdownHook = new JVMShutdownHook();
		Runtime.getRuntime().addShutdownHook(jvmShutdownHook);	 
	}
	
	public WmsAdmin() {
		Configuration conf = new WmsConfiguration().create();
		WmsAdmin wmsClient = new WmsAdmin(conf);
	}
	
    private class JVMShutdownHook extends Thread {
    	public void run() {
    		LOG.debug("JVM shutdown hook is running");  
    		try {
    			if(zkc != null)
    				zkc.close();
    		} catch (InterruptedException ie) {};
    	}
    }	
	
	void getServer() throws IOException, MasterNotRunningException {
		serverIpAddress = null;
		List<String> servers = new ArrayList<String>();
		
		try {
			if(zkc == null)
				zkc = new ZkClient();
			zkc.connect();

			Stat stat = zkc.exists(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER,false);
			if(stat != null) {
				servers = zkc.getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER,null);
			}
		} catch (Exception e) {
			throw new IOException(e.getMessage());
		}	
		
		if( ! servers.isEmpty()) {
			StringTokenizer st = new StringTokenizer(servers.get(0), ":"); 
			while(st.hasMoreTokens()) { 
				serverIpAddress=st.nextToken();
				thriftPort=Integer.parseInt(st.nextToken());
				startTime=Long.parseLong(st.nextToken());
			}
		} else {
			throw new MasterNotRunningException("WmsMaster " + parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER +  " not found");
		}
	}

	public synchronized void open()	throws IOException, MasterNotRunningException {
        LOG.debug("open()");
        
 		getServer();
 		
 		try {
			transport = new TFramedTransport(new TSocket(serverIpAddress, thriftPort));
			transport.open();
 			tClient = new WmsAdminService.Client(new TBinaryProtocol(transport));
 		} catch (Exception e) {
 			LOG.error("Exception thrown in open(), " + e.getMessage());
 			throw new MasterNotRunningException("Exception thrown in pingServer(): " + e.getMessage());
 		}
 		
		pingServer();
		
		isOpen = true;
	}
	
	void pingServer() throws MasterNotRunningException {
		LOG.debug("pingServer()" );

		try {
			LOG.debug("Pinging Thrift server " + serverIpAddress + ":" + thriftPort);
			long startTs = System.currentTimeMillis();
			long endTs = tClient.ping(startTs);
			LOG.debug("Thrift ping successful [" + (endTs - startTs) + " millisecond(s)]");
		} catch (Exception e) {
			LOG.error("Exception thrown in pingServer(): " + e.getMessage());
			throw new MasterNotRunningException("Exception thrown in pingServer(): " + e.getMessage());
		}
	}
	
	public synchronized void addStream(Stream stream) throws IOException {
		if(isOpen == false)
			throw new IllegalStateException("Connection is not open");		

		try {
			tClient.addStream(stream);
		} catch (Exception e) {
			throw new IOException("WmsAdmin addStream error: " + e.getMessage());
		}
	}
	public synchronized void alterStream(Stream stream) throws IOException {
		if(isOpen == false)
			throw new IllegalStateException("Connection is not open");		

		try {
			tClient.alterStream(stream);
		} catch (Exception e) {
			throw new IOException("WmsAdmin alterStream error: " + e.getMessage());
		}
	}
	public synchronized void deleteStream(Stream stream) throws IOException {
		if(isOpen == false)
			throw new IllegalStateException("Connection is not open");		

		try {
			tClient.deleteStream(stream);
		} catch (Exception e) {
			throw new IOException("WmsAdmin deleteStream error: " + e.getMessage());
		}	
	}
	public synchronized StreamResponse stream() throws IOException {
		if(isOpen == false)
			throw new IllegalStateException("Connection is not open");

		StreamResponse response = new StreamResponse();
		
		try {
			response = tClient.stream();
		} catch (Exception e) {
			throw new IOException("Stream error: " + e.getMessage());
		}
		
		LOG.debug("stream response(" + response.toString());

		return response;
	}

	public synchronized void addRule(Rule rule) throws IOException {
		if(isOpen == false)
			throw new IllegalStateException("Connection is not open");		

		try {
			tClient.addRule(rule);
		} catch (Exception e) {
			throw new IOException("WmsAdmin addRule error: " + e.getMessage());
		}
	}
	public synchronized void alterRule(Rule rule) throws IOException {
		if(isOpen == false)
			throw new IllegalStateException("Connection is not open");		

		try {
			tClient.alterRule(rule);
		} catch (Exception e) {
			throw new IOException("WmsAdmin alterRule error: " + e.getMessage());
		}
	}
	public synchronized void deleteRule(Rule rule) throws IOException {
		if(isOpen == false)
			throw new IllegalStateException("Connection is not open");		

		try {
			tClient.deleteRule(rule);
		} catch (Exception e) {
			throw new IOException("WmsAdmin deleteRule error: " + e.getMessage());
		}	
	}
	public synchronized RuleResponse rule() throws IOException {
		if(isOpen == false)
			throw new IllegalStateException("Connection is not open");

		RuleResponse response = new RuleResponse();
		
		try {
			response = tClient.rule();
		} catch (Exception e) {
			throw new IOException("Rule error: " + e.getMessage());
		}
		
		LOG.debug("rule response(" + response.toString());

		return response;
	}
	
	public synchronized WorkloadResponse workload() throws IOException {
		if(isOpen == false)
				throw new IllegalStateException("Connection is not open");
		
		WorkloadResponse response = new WorkloadResponse();

		try {
			response = tClient.workload();
		} catch (Exception e) {
			throw new IOException("Workload error: " + e.getMessage());
		}
		
		LOG.debug("workload response(" + response.toString());

		return response;
	}

	public synchronized boolean close()	throws MasterNotRunningException, IOException {
		LOG.debug("close()");

		if(isOpen == false)
			return true;
		
		transport.close();
		transport = null;
		protocol = null;
		tClient = null;

		try {
		if(zkc != null)
			zkc.close();
		} catch (Exception e) {
			throw new IOException(e.getMessage());
		}
		
		isOpen = false;
		zkc = null;
		serverIpAddress = null;
		serverInstance = null;
		thriftPort=0;
		clientIpAddress = null;
		clientZnode = null;
		clientTimestamp=0L;

		return true;
	}
	
	public List<String> getWorkloads() {
		List<String> workloads = new ArrayList<String>();
		
		return workloads;
	}
	
	public void addUpdateRule() {

	}
	
	public void deleteRule() {

	}
	
	public static void main(String[] args) {
		try {
			Configuration conf = new WmsConfiguration().create();
			WmsAdmin wmsClient = new WmsAdmin(conf);
		
			wmsClient.open();
/*			
			Data request = new Data();
			request.setOperation(Operation.OPERATION_BEGIN);
			request.setState("BEGIN");
			request.setSubState("RUNNING");
			request.setBeginTimestamp(System.currentTimeMillis());
			request.setEndTimestamp(System.currentTimeMillis());
			Map<String, String> m = new HashMap<String, String>();
			request.setKeyValues(m);
			m.put("type","TRAFODION");
			m.put("query_id","MXID11000001075212235857042874154000000000106U6553500_4_SQL_DATASOURCE_Q8");
			m.put("query_text","This is some query text");
			System.out.println("Request=" + request);
			Data response = wmsClient.writeread(request);
			System.out.println("Response=" + response);

			request.setOperation(Operation.OPERATION_UPDATE);
			request.setState("EXECUTING");
			request.setSubState("UPDATE");
			request.setWorkloadId(response.getWorkloadId());
			request.setEndTimestamp(System.currentTimeMillis());
			System.out.println("Request=" + request);
			response = wmsClient.writeread(request);
			System.out.println("Response=" + response);
			
			request.setOperation(Operation.OPERATION_END);
			request.setState("COMPLETED");
			request.setSubState("SUCCEEDED");
			request.setEndTimestamp(System.currentTimeMillis());
			System.out.println("Request=" + request);
			response = wmsClient.writeread(request);
			System.out.println("Response=" + response);
			if(response.getKeyValues() != null){
				for (String key: response.getKeyValues().keySet()) {
					System.out.println(key + "=" + response.getKeyValues().get(key));
				} 
			}
*/			
			wmsClient.close();
        
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
