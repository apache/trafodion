/**
* @@@ START COPYRIGHT @@@

* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at

*   http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.

* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs.servermt;

import java.net.InetAddress;
import java.io.*;
import java.util.List;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.StringTokenizer;
import java.util.concurrent.Callable;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.CompletionService;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
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
import org.trafodion.dcs.util.RetryCounterFactory;
import org.trafodion.dcs.util.RetryCounter;
import org.trafodion.dcs.zookeeper.ZkClient;
import org.trafodion.dcs.script.ScriptManager;
import org.trafodion.dcs.script.ScriptContext;
import org.trafodion.dcs.servermt.serverHandler.*;

public final class ServerManager implements Callable {
    private static final Log LOG = LogFactory.getLog(ServerManager.class);
    private Configuration conf;
    private ZkClient zkc;
    private boolean userProgEnabled;
    private String userProgramHome;
    private String userProgCommand;
    private String hostName;
    private String masterHostName;
    private long masterStartTime;
    private int port;
    private int portRange;
    private DcsNetworkConfiguration netConf;
    private int instance;
    private int serverThreads;
    private String parentZnode;
    private int connectingTimeout;
    private int zkSessionTimeout;
    private int userProgExitAfterDisconnect;
    private int infoPort;
    private String portMap;
    private HashMap<String, Integer>  hashPortMap = new HashMap<String, Integer>();
    private int maxRestartAttempts;
    private int retryIntervalMillis;
    private RetryCounterFactory retryCounterFactory;
    
    private String CLUSTERNAME;
    private String HOME;
    private String TRAF_HOME;
    private byte[] cert;
    private String key;
    private Integer data;
    private String[] tokens;
    
    static
    {
        try {
            Class.forName(Constants.T2_DRIVER_CLASS_NAME);
        } catch (Exception e) {
            e.printStackTrace();
            if(LOG.isDebugEnabled())
                LOG.error("T2 Driver Class not found in CLASSPATH :" + e.getMessage());
            System.exit(-1);
        }
    }
    
    public ServerManager(Configuration conf,ZkClient zkc,DcsNetworkConfiguration netConf,
            String instance,int infoPort,int serverThreads) throws Exception {
        this.conf = conf;
        this.zkc = zkc;
        this.netConf = netConf;
        this.hostName = netConf.getHostName();
        this.instance = Integer.parseInt(instance);
        this.infoPort = infoPort;
        this.serverThreads = serverThreads;
        this.parentZnode = this.conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);           
        this.connectingTimeout = this.conf.getInt(Constants.DCS_SERVER_USER_PROGRAM_CONNECTING_TIMEOUT,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_CONNECTING_TIMEOUT);           
        this.zkSessionTimeout = this.conf.getInt(Constants.DCS_SERVER_USER_PROGRAM_ZOOKEEPER_SESSION_TIMEOUT,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_ZOOKEEPER_SESSION_TIMEOUT);           
        this.userProgExitAfterDisconnect = this.conf.getInt(Constants.DCS_SERVER_USER_PROGRAM_EXIT_AFTER_DISCONNECT,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_EXIT_AFTER_DISCONNECT);
        System.setProperty("hbaseclient.log4j.properties",System.getProperty("dcs.conf.dir") + "/log4j.properties");
        System.setProperty("dcs.root.logger",System.getProperty("dcs.root.logger"));
        System.setProperty("dcs.log.dir",System.getProperty("dcs.log.dir"));
        System.setProperty("dcs.log.file",System.getProperty("dcs.log.file"));
        
    }

    @Override
    public Boolean call() throws Exception {
        
        ExecutorService executorService = Executors.newFixedThreadPool(serverThreads);
        CompletionService<Integer> completionService = new ExecutorCompletionService<Integer>(executorService);
        
        try {
            featureCheck();
            getCertificate();
            getMaster();
            portMap = getPortMap();
            registerInRunning(instance);
            int assignedPort;

            maxRestartAttempts = conf.getInt(Constants.DCS_SERVER_CHECK_TCPIPPORT_ATTEMPTS,Constants.DEFAULT_DCS_SERVER_CHECK_TCPIPPORT_ATTEMPTS);
            retryIntervalMillis = conf.getInt(Constants.DCS_SERVER_CHECK_TCPIPPORT_RETRY_INTERVAL_MILLIS,Constants.DEFAULT_DCS_SERVER_CHECK_TCPIPPORT_RETRY_INTERVAL_MILLIS);
            retryCounterFactory = new RetryCounterFactory(maxRestartAttempts, retryIntervalMillis);

            tokens = portMap.split("[:]");
            if (tokens == null || tokens.length < 3){
                throw new Exception("PortMap incorrect :[" + portMap + "]");
            }
            for (int indx = 0; indx < tokens.length; indx += 3){
                if(LOG.isDebugEnabled())
                    LOG.debug("Mapping element :" + tokens[indx] + " " + tokens[indx + 1]+ " "+ tokens[indx + 2]);
                key = tokens[indx] + ":" + tokens[indx + 1];
                data = Integer.valueOf(tokens[indx + 2]);
                hashPortMap.put(key,data);
            }
            for(int serverThread = 1; serverThread <= serverThreads; serverThread++) {
                key = instance + ":" + Integer.toString(serverThread);
                if (hashPortMap.containsKey(key) == false) {
                    throw new Exception("Key [" + key + "]. Cannot be found in hashPortMap");
                }
                assignedPort = hashPortMap.get(key);
                completionService.submit(new ServerHandler(conf, zkc, netConf, instance, infoPort,masterHostName, retryCounterFactory, assignedPort, cert, serverThread ));
            }

            while(true) {
                Future<Integer> f = completionService.take();//blocks waiting for any ServerHandler to finish
                if(f != null) {
                    Integer result = f.get();
                    if(LOG.isDebugEnabled())
                        LOG.debug("Server handler Thread returned result [" + result + "]");
                    if (1 <= result && result <= serverThreads ){
                        if(LOG.isDebugEnabled())
                            LOG.debug("Restarting Server handler Thread [" + instance + ":" + result + "]");
                        key = instance + ":" + Integer.toString(result);
                        if (hashPortMap.containsKey(key) == false) {
                            throw new Exception("Key [" + key + "]. Cannot be found in hashPortMap");
                        }
                        assignedPort = hashPortMap.get(key);
                        completionService.submit(new ServerHandler(conf,zkc,netConf, instance,infoPort,masterHostName, retryCounterFactory, assignedPort, cert, result ));
                    }
                    else{
                        if(LOG.isDebugEnabled())
                            LOG.debug("Server handler Thread will not be restarted [" + instance + ":" + result + "]. Expected result should be in range [" + instance + ":1-" + serverThreads + "]");
                        break;
                    }
                }
            }
            
        } catch (Exception e) {
            e.printStackTrace();
            LOG.error(e);
        } finally {
            System.out.println("finally: Shutdown ServerHandler Pool");
            if(executorService != null) {
                executorService.shutdown(); // Disable new tasks from being submitted
                try {
                        // Wait a while for existing tasks to terminate
                        if (!executorService.awaitTermination(60, TimeUnit.SECONDS)) {
                            executorService.shutdownNow(); // Cancel currently executing tasks
                            // Wait a while for tasks to respond to being cancelled
                            if (!executorService.awaitTermination(60, TimeUnit.SECONDS))
                                System.out.println("finally: ServerHandler Pool did not terminate");
                        }
                  } catch (InterruptedException ie) {
                        // (Re-)Cancel if current thread also interrupted
                        executorService.shutdownNow();
                        // Preserve interrupt status
                        Thread.currentThread().interrupt();
                  }
            }
         }
        return false;
    }
    
    private void featureCheck() throws IOException{
        String value = System.getenv("TRAF_HOME");
        if(value == null || value.length() == 0 ) {
            LOG.error("Environment variable $TRAF_HOME is not set.");
            throw new IOException("Environment variable $TRAF_HOME is not set.");
        }
        TRAF_HOME = value;
        if(LOG.isDebugEnabled())
            LOG.info("Environment variable $TRAF_HOME is set [" + TRAF_HOME + "]");
    }
    
    private void getCertificate() throws IOException {
        String path;
        CLUSTERNAME = System.getenv("CLUSTERNAME");
        if(CLUSTERNAME == null || CLUSTERNAME.length() == 0 ) {
            path = TRAF_HOME + "/sqcert/server.crt";
        }
        else {
            HOME = System.getenv("HOME");
            if(HOME == null || HOME.length() == 0 ) {
                LOG.error("Environment variable $HOME is not set.");
                throw new IOException("Environment variable $HOME is not set.");
           }
           path = HOME + "/sqcert/server.crt";
        }
        int readLength;
        int outLength;
        
        File f = new File(path);
        if(false == f.exists()){
            LOG.error("Certificate file doesn't exist [" + path + "]");
            throw new IOException("Certificate file doesn't exist [" + path + "]");
        }
        BufferedInputStream in = new BufferedInputStream(new FileInputStream(f));
        if ((readLength = in.available()) > 0) {
            cert = new byte[readLength];
            if(readLength != (outLength = in.read(cert))){
                in.close();
                LOG.error("Expected Certificate lenght and File read length do not match [" + readLength + "/" + outLength + "]");
                throw new IOException("Expected Certificate lenght and File read length do not match [" + readLength + "/" + outLength + "]");
            }
        }
        else {
            in.close();
            LOG.error("Certificate file doesn't exist [" + path + "]");
            throw new IOException("Certificate file doesn't exist [" + path + "]");
        }
        in.close();
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
                    } catch (InterruptedException e) {    }
                }

            } catch (Exception e) {
                e.printStackTrace();
                LOG.error(e);
            }
        }
    }
    private void registerInRunning(int instance) {
        String znode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING + "/" + hostName + ":" + instance + ":" + infoPort + ":" + System.currentTimeMillis(); 
        try {
            Stat stat = zkc.exists(znode,false);
            if(stat == null) {
                zkc.create(znode,new byte[0],ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.EPHEMERAL);
                if(LOG.isDebugEnabled())
                    LOG.debug("Created znode [" + znode + "]");
            }
        } catch (KeeperException.NodeExistsException e) {
            //do nothing...leftover from previous shutdown
        } catch (Exception e) {
            e.printStackTrace();
            LOG.error(e);
        }
    }
    
    private String getPortMap() {
        byte[] data = null;

        String nodeRegisteredPath = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED;
        try {
            Stat stat = zkc.exists(nodeRegisteredPath,false);
            if(stat == null) {
                LOG.error("NodeRegistered doesn't exist [" + nodeRegisteredPath + "]");
                throw new Exception("NodeRegistered doesn't exist [" + nodeRegisteredPath + "]");
            }
            else {
                data = zkc.getData(nodeRegisteredPath, false, stat);
            }
        } catch (KeeperException.NodeExistsException e) {
            //do nothing...leftover from previous shutdown
        } catch (Exception e) {
            e.printStackTrace();
            LOG.error(e);
        }
        return new String(data);
    }
    
    public String getMasterHostName(){
        return masterHostName;
    }
    
    public String getZKParentZnode(){
        return parentZnode;
    }
    public String getUserProgramHome(){
        return userProgramHome;
    }

}
