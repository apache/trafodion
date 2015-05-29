package org.trafodion.wms.client;

import java.io.*;
import java.net.*;
import java.nio.charset.Charset;
import java.util.List;
import java.util.Iterator;
import java.util.StringTokenizer;
import java.util.HashMap;
import java.util.Map;
import java.util.Date;

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

import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.Constants;
import org.trafodion.wms.zookeeper.ZkClient;
import org.trafodion.wms.thrift.generated.*;

public class WmsClient {
    private static final Log LOG = LogFactory.getLog(WmsClient.class);
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
    private Configuration config;
    private String lastError;
    private String parentZnode;
    private JVMShutdownHook jvmShutdownHook;
    private Request request = new Request(new Header(),new Data());
    private Response response = new Response(new Header(),new Data());
    private Data requestData;
    
    public WmsClient() {
        config = WmsConfiguration.create();
        parentZnode = config.get(Constants.ZOOKEEPER_ZNODE_PARENT,Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);    
        jvmShutdownHook = new JVMShutdownHook();
        Runtime.getRuntime().addShutdownHook(jvmShutdownHook);   
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
    
    public String getLastError() {
        return lastError;
    }

    void setLastError(String err) {
        lastError = err;
    }
    
    void register() throws IOException {
        LOG.debug("WmsClient.register() called.");
        
        try {
            InetAddress ip = null;
            try {
                ip = InetAddress.getLocalHost();
            } catch (UnknownHostException e) {
                LOG.error("UnknownHostException " + e.getMessage());
                throw e;
            }

            //register in zookeeper /wms/clients
            clientTimestamp = System.currentTimeMillis();
            clientIpAddress = ip.getHostAddress();
            clientZnode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_CLIENTS + "/" + clientIpAddress + ":" + clientTimestamp; 
            zkc.create(clientZnode,new byte[0],ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.EPHEMERAL);
            LOG.info("Client registered as [" + clientZnode + "]");

        } catch (InterruptedException e) {
            throw new IOException(e.getMessage());
        } catch (KeeperException e) {
            throw new IOException(e.getMessage());
        }
    }
    
    private List<String> getServerList() throws IOException {
        LOG.debug("WmsClient.getServerList() called.");
        
        List<String> servers;
        try {
            if(zkc == null)
                zkc = new ZkClient();//CTRL-C...set sessionTimeout,maxRetries,retryIntervalMillis
            zkc.connect();
            servers = zkc.getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING,zkc);
        } catch (InterruptedException e) {
            throw new IOException(e.getMessage());
        } catch (KeeperException e) {
            throw new IOException(e.getMessage());
        }
        return servers;     
    }
 
    boolean pingServer() {
        LOG.debug("WmsClient.pingServer() called. " );
        long startTs;
        long endTs;

        try {
            TFramedTransport transport;
            TProtocol protocol;
            WmsService.Client tClient;
            
            LOG.debug("Pinging Thrift server " + serverIpAddress + ":" + thriftPort);
            //transport = new TSocket(serverIpAddress, thriftPort);
            transport = new TFramedTransport(new TSocket(serverIpAddress, thriftPort));
            transport.open();
            protocol = new  TBinaryProtocol(transport);
            tClient = new WmsService.Client(protocol);
            startTs = System.currentTimeMillis();
            endTs = tClient.ping(startTs);
            LOG.debug("Thrift ping successful [" + (endTs - startTs) + " millisecond(s)]");
            transport.close();
            
        } catch (Exception e) {
            LOG.error("Exception thrown in WmsClient.pingServer(): " + e.getMessage());
            return false;
        }
        return true;
    }

    void getServer() throws Exception {
        List<String> servers = getServerList();
        serverIpAddress=null;
        serverInstance=null;
        
        boolean result=false;
        int roundRobinIndex=0;
        int roundRobinMax=(int)(Math.random() * 10.0) + 1;

        for(int j=0; j < roundRobinMax; j++)
            roundRobinIndex = (int)(Math.random() * (float)(servers.size() - 1) + 0.5);

        for(int i=0; i<servers.size(); i++, roundRobinIndex++) {
        //for(String aserver: servers) {
            roundRobinIndex = roundRobinIndex % servers.size();
            String aserver = servers.get(roundRobinIndex);

            StringTokenizer st = new StringTokenizer(aserver, ":"); 
            while(st.hasMoreTokens()) { 
                serverIpAddress=st.nextToken();
                serverInstance=st.nextToken();
                thriftPort=Integer.parseInt(st.nextToken());//Thrift port
                Integer.parseInt(st.nextToken());//skip info port
                startTime=Long.parseLong(st.nextToken());
            }
            //We have a server but let's ping it just in case
            //since there can be a short delay before ephemeral nodes
            //really disappear from /wms/servers.
            if(pingServer()) {
                LOG.debug("Server selected [" + aserver +  "]");
                result=true;
                break;
            }
            result=true;
        }

        if(result==false) 
            throw new Exception("No active Wms servers found");
    }

    public synchronized boolean open() {
        LOG.debug("WmsClient.open() called.");
        
        try {
            getServer();
            register();
        } catch (Exception e) {
            LOG.error("Exception thrown in WmsClient.open(): " + e.getMessage());
            setLastError(e.getMessage());
            return false;
        }

        isOpen=true;
        return true;
    }
    
    public synchronized boolean cleanup() {
        return true;
    }   

    public synchronized ClientData writeread(ClientData data) {
        LOG.debug("WmsClient.writeread request(" + data.toString() + ")");
        
        request.setData(data);
        request.getHeader().setClientIpAddress(clientIpAddress);
        request.getHeader().setClientTimestamp(clientTimestamp);//client timestamp is set at open() time
        
        try {
            if(isOpen==false)
                throw new IllegalStateException("Connection is not open");

            TFramedTransport transport;
            TProtocol protocol;
            WmsService.Client tClient;
            transport = new TFramedTransport(new TSocket(serverIpAddress, thriftPort));
            transport.open();
            protocol = new  TBinaryProtocol(transport);
            tClient = new WmsService.Client(protocol);
            response = tClient.writeread(request);
            LOG.debug("WmsClient.writeread response(" + response.toString());
            transport.close();
        } catch (Exception e) {
            e.printStackTrace();
            LOG.error("Exception thrown in WmsClient.writeread(): " + e.getMessage());
            setLastError(e.getMessage());
        }
        
        LOG.debug("WmsClient.writeread response(" + response.toString());

        request.getHeader().setServerLastUpdated(response.getHeader().getServerLastUpdated());//Last updated by WmsServer
        return new ClientData(response.getData());
//      return response.getData();
    }

    public synchronized boolean close() {
        LOG.debug("WmsClient.close() called.");

        if(isOpen == false)
            return true;

        try {
            if(zkc != null)
                zkc.close();
            isOpen = false;
            zkc = null;
            serverIpAddress=null;
            serverInstance=null;
            thriftPort=0;
            clientIpAddress=null;
            clientZnode=null;
            clientTimestamp=0L;
        } catch (Exception e) {
            LOG.error("Exception thrown in WmsClient.close(): " + e.getMessage());
            setLastError(e.getMessage());
            return false;
        }

        return true;
    }
    
    public static void main(String[] args) {
        try {
            WmsClient wmsClient = new WmsClient();
            wmsClient.open();
            
            ClientData request = new ClientData();
            ClientData response = new ClientData();
            Map<String, KeyValue> m = new HashMap<String, KeyValue>();
            request.setKeyValues(m);

            request.putKeyValue("operation",Operation.OPERATION_BEGIN);
            request.putKeyValue("state","RUNNING");
            request.putKeyValue("subState","BEGIN");
            request.putKeyValue("beginTimestamp",System.currentTimeMillis());
            request.putKeyValue("endTimestamp",System.currentTimeMillis());
            request.putKeyValue("type","trafodion");
            request.putKeyValue("queryId","MXID11000001075212235857042874154000000000106U6553500_4_SQL_DATASOURCE_Q8");
            request.putKeyValue("queryText","This is some query text");
            System.out.println("Request=" + request);
            response = wmsClient.writeread(request);
            System.out.println("Response=" + response);

            request.putKeyValue("operation",Operation.OPERATION_UPDATE);
            request.putKeyValue("state","RUNNING");
            request.putKeyValue("subState","UPDATE");
            request.putKeyValue("workloadId",response.getKeyValueAsString("workloadId"));
            request.putKeyValue("endTimestamp",System.currentTimeMillis());
            System.out.println("Request=" + request);
            response = wmsClient.writeread(request);
            System.out.println("Response=" + response);
            
            request.putKeyValue("operation",Operation.OPERATION_END);
            request.putKeyValue("state","COMPLETED");
            request.putKeyValue("subState","SUCCEEDED");
            request.putKeyValue("workloadId",response.getKeyValueAsString("workloadId"));
            request.putKeyValue("endTimestamp",System.currentTimeMillis());
            System.out.println("Request=" + request);
            request = wmsClient.writeread(request);
            System.out.println("Response=" + response);
            if(response.getKeyValues() != null){
                for (String key: response.getKeyValues().keySet()) {
                    System.out.println(key + "=" + response.getKeyValues().get(key));
                } 
            }
            
            wmsClient.close();
            
        
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
