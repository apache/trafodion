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
package org.trafodion.dcs.servermt.serverHandler;

import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.net.*;
import java.util.*;
import java.util.LinkedList;
import java.util.List;
import java.util.HashMap;

import java.net.InetAddress;
import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.util.StringTokenizer;
import java.util.concurrent.Callable;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.CompletionService;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ConcurrentHashMap;
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
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.*;

public final class ServerHandler implements Callable {

    private static final Log LOG = LogFactory.getLog(ServerHandler.class);
    private ServerSocketChannel server=null;
    private Selector selector=null;
    private Configuration conf;
    private ZkClient zkc;
    private boolean userProgEnabled;
    private String userProgHome;
    private String userProgCommand;
    private String masterHostName;
    private long masterStartTime;
    private DcsNetworkConfiguration netConf;
    private int requestTimeout;
    private int connectingTimeout;
    private int selectorTimeout;
    private int zkSessionTimeout;
    private int userProgExitAfterDisconnect;
    private int infoPort;
    private SelectionKey serverkey;
    private ServerWorker worker=null;
    private ServerUtils utils = null;
    
    public String parentZnode;
    public int fport;
    public String hostName;
    public int instance;
    public int serverThread;
    public String serverName;
    public int serverState= ServerConstants.SERVER_STATE_INIT;

    public byte[] cert;
    private RetryCounterFactory retryCounterFactory;

    private List<PendingRequest> pendingChanges = new LinkedList<PendingRequest>();    //list of PendingRequests instances
    private ConcurrentHashMap<SelectionKey, Long> timeouts = new ConcurrentHashMap<SelectionKey, Long>(); // hash map of timeouts

    public ServerHandler(Configuration conf,ZkClient zkc, DcsNetworkConfiguration netConf,
        int instance,int infoPort, String masterHostName, RetryCounterFactory retryCounterFactory, int assignedPort, byte[] cert, int serverThread) throws Exception {
        this.conf = conf;
        this.zkc = zkc;
        this.netConf = netConf;
        this.hostName = netConf.getHostName();
        this.requestTimeout = conf.getInt(Constants.DCS_SERVER_LISTENER_REQUEST_TIMEOUT,Constants.DEFAULT_SERVER_LISTENER_REQUEST_TIMEOUT);
        this.connectingTimeout = conf.getInt(Constants.DCS_SERVER_USER_PROGRAM_CONNECTING_TIMEOUT,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_CONNECTING_TIMEOUT) * 1000;
        this.selectorTimeout = conf.getInt(Constants.DCS_SERVER_LISTENER_SELECTOR_TIMEOUT,Constants.DEFAULT_SERVER_LISTENER_SELECTOR_TIMEOUT);
        this.instance = instance;
        this.infoPort = infoPort;
        this.masterHostName = masterHostName;
        this.retryCounterFactory = retryCounterFactory;
        this.fport = assignedPort;
        this.cert = cert;
        this.serverThread = serverThread;
        this.parentZnode = this.conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);           
        this.zkSessionTimeout = this.conf.getInt(Constants.DCS_SERVER_USER_PROGRAM_ZOOKEEPER_SESSION_TIMEOUT,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_ZOOKEEPER_SESSION_TIMEOUT);
        this.userProgExitAfterDisconnect = this.conf.getInt(Constants.DCS_SERVER_USER_PROGRAM_EXIT_AFTER_DISCONNECT,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_EXIT_AFTER_DISCONNECT);

        init();
    }
    private void init(){
        serverName = ServerConstants.SERVER_NAME + "_" + instance + "_" + serverThread;
        utils = new ServerUtils(this, zkc);
        
        try {
            worker = new ServerWorker(zkc, instance, serverThread, serverName, cert);
            worker.start();
        } catch (Exception e) {
            e.printStackTrace();
            LOG.error(e);
        }
    }
    public void send(PendingRequest preq) {
        synchronized (this.pendingChanges) {
            this.pendingChanges.add(preq);
        }
        this.selector.wakeup();
    }
    public void setConnectingTimeout(){
        timeouts.put(serverkey, System.currentTimeMillis());
        this.selector.wakeup();
    }
    public int getConnectingTimeout(){
        return connectingTimeout;
    }
    private int setSelectorTimeout(){
        if (false == timeouts.isEmpty()){
            return selectorTimeout;
        }
        else {
            return 0;
        }
    }
    private void closeClientConnection(SelectionKey key){
        try {
            ClientData clientData = (ClientData) key.attachment();
            if (clientData.getTrafConnection() != null)
                this.worker.closeTrafConnection(key);
            SocketChannel client = (SocketChannel)key.channel();
            client.close();
            clientData.bbHeader = null;
            clientData.bbBody = null;
            clientData.hdr = null;
            clientData.clientSocketAddress = null;
            key.attach(null);
            key.cancel();
        } catch (IOException e) {}
    }
    @Override
    public Integer call() throws Exception {
        Integer result = new Integer(serverThread);

        try {
            selector = SelectorProvider.provider().openSelector();
            server = ServerSocketChannel.open();
            server.configureBlocking(false);
            
            InetSocketAddress isa = new InetSocketAddress(hostName, fport);
            RetryCounter retryCounter = retryCounterFactory.create();
            
            while (true) {

                try {
                    server.socket().bind(isa);
                    LOG.info(serverName  + ". Bound to port : [" + fport + "]");
                    break;
                } catch (IOException e){
                    LOG.error(serverName + ". PORT_IN_USE: [" + fport + "] " + e);
                    utils.updateServerState(ServerConstants.SERVER_STATE_PORTINUSE);
                } catch (IllegalArgumentException e) {
                    LOG.error(serverName + ". IllegalArgumentException: " + e);
                    System.exit(1);
                } catch (SecurityException e) {    
                    LOG.error(serverName + ". SecurityException: " + e);
                    System.exit(1);
                }
                if (retryCounter.shouldRetry() == false)
                    break;
                retryCounter.sleepUntilNextRetry();
                retryCounter.useRetry();
            }
            if (server.socket().isBound() == false){
                LOG.error(serverName + ". Cannot bind to tcpip port [" + fport + "]");
                return result;
            }
//================= LISTENER ========================================================
//            
            serverkey = server.register(selector, SelectionKey.OP_ACCEPT );
            int keysAdded = 0;
            PendingRequest preq = null;
            int request = ServerConstants.REQUST_INIT;

            utils.updateServerState(ServerConstants.SERVER_STATE_INIT);

            while(true){
                synchronized (this.pendingChanges) {
                    Iterator<PendingRequest> changes = this.pendingChanges.iterator();
                    while (changes.hasNext()) {
                        preq = changes.next();
                        SelectionKey key = preq.key;
                        ClientData clientData = (ClientData) key.attachment();
                        request = clientData.getRequest();
                        if(LOG.isDebugEnabled())
                            LOG.debug(serverName + ". request :" + ServerUtils.convertRequestToString(request));
                        switch(request){
                        case ServerConstants.REQUST_WRITE_READ:
                        case ServerConstants.REQUST_WRITE_CLOSE:
                            key.interestOps(key.interestOps() | SelectionKey.OP_WRITE);
                            clientData.total_write = 0;
                            break;
                        case ServerConstants.REQUST_CLOSE:
                            closeClientConnection(key);
                            utils.updateServerState(ServerConstants.SERVER_STATE_DISCONNECTED);
                            break;
                        }
                        preq.key = null;
                    }
                    this.pendingChanges.clear();
                }
                while(true){
                    if ((keysAdded = selector.select(setSelectorTimeout())) > 0) {
                        Set<SelectionKey> keys = selector.selectedKeys();
                        Iterator<SelectionKey> i = keys.iterator();

                        while (i.hasNext()) {
                            SelectionKey key = i.next();
                            i.remove();
                            if (!key.isValid()) continue;

                            if ((key.readyOps() & SelectionKey.OP_ACCEPT) == SelectionKey.OP_ACCEPT) {
                                if(LOG.isDebugEnabled())
                                    LOG.debug(serverName + ". Ready to process ACCEPT");
                                processAccept(key);
                            } else if ((key.readyOps() & SelectionKey.OP_READ) == SelectionKey.OP_READ) {
                                if(LOG.isDebugEnabled())
                                    LOG.debug(serverName + ". Ready to process READ");
                                processRead(key);
                            } else if ((key.readyOps() & SelectionKey.OP_WRITE) == SelectionKey.OP_WRITE) {
                                if(LOG.isDebugEnabled())
                                    LOG.debug(serverName + ". Ready to process WRITE");
                                processWrite(key);
                            }
                        }
                    }
                    else
                        break;
                }
                if (false == timeouts.isEmpty()){
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverName + ". Timeouts ");
                    long currentTime = System.currentTimeMillis();
                    Iterator<SelectionKey> i = timeouts.keySet().iterator();
                    while(i.hasNext()){
                      SelectionKey key = i.next();
                      if (serverkey == key){
                          if(LOG.isDebugEnabled())
                              LOG.debug(serverName + ". Checking CONNECTING");
                          if (true != utils.checkServerState(ServerConstants.SERVER_STATE_CONNECTING)){
                              if(LOG.isDebugEnabled())
                                  LOG.debug(serverName + ". Removing CONNECTING timeout - serverState :" + utils.convertStateToString(serverState));
                              i.remove();
                          }
                          else if (currentTime - timeouts.get(key) > connectingTimeout){
                              long timeout = (currentTime - timeouts.get(key))/1000;
                              if(LOG.isDebugEnabled())
                                  LOG.debug(serverName + ". CONNECTING timeouted[" + timeout + " seconds]");
                              utils.updateServerState(ServerConstants.SERVER_STATE_CONNECTING_TIMEOUTED);
                              i.remove();
                          }
                      }
                      else if (currentTime - timeouts.get(key) > requestTimeout){
                          long timeout = (currentTime - timeouts.get(key))/1000;
                          SocketChannel client = (SocketChannel) key.channel();
                          Socket s = client.socket();
                          if ((key.interestOps() & SelectionKey.OP_READ) == SelectionKey.OP_READ){
                              if(LOG.isDebugEnabled())
                                  LOG.debug(serverName + ". Read from client timeouted[" + timeout + " seconds] from: " + s.getRemoteSocketAddress());
                              serverState = ServerConstants.SERVER_STATE_READ_TIMEOUTED;
                          }
                          else if ((key.interestOps() & SelectionKey.OP_WRITE) == SelectionKey.OP_WRITE) {
                              if(LOG.isDebugEnabled())
                                  LOG.debug(serverName + ". Write to client timeouted[" + timeout + " seconds] from: " + s.getRemoteSocketAddress());
                              serverState = ServerConstants.SERVER_STATE_WRITE_TIMEOUTED;
                          }
                          else {
                              if(LOG.isDebugEnabled())
                                  LOG.debug(serverName + ". Client timeouted[" + timeout + " seconds] from: " + s.getRemoteSocketAddress());
                              serverState = ServerConstants.SERVER_STATE_CLIENT_TIMEOUTED;
                          }
                          i.remove();
                          closeClientConnection(key);
                          utils.updateServerState(serverState);
                        }
                    }
                }
            }

        } catch (IOException e) {
            LOG.error(serverName + ". IOException :" + e);
            System.exit(1);
        } finally {
            if (server != null) {
                try {
                    server.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return result;
    }
    private void processAccept(SelectionKey key) {
        try {
            if (false == timeouts.isEmpty() && true == timeouts.containsKey(serverkey)){
                timeouts.remove(serverkey);
            }
            ServerSocketChannel server = (ServerSocketChannel)key.channel();
            SocketChannel client = server.accept();
            Socket s = client.socket();
            // Accept the request
            if(LOG.isDebugEnabled())
                LOG.debug(serverName + ". Received an incoming connection from: " + s.getRemoteSocketAddress());
            client.configureBlocking( false );
            SelectionKey clientkey = client.register( selector, SelectionKey.OP_READ );
            ClientData clientData = new ClientData(s.getRemoteSocketAddress(), utils, conf);
            utils.updateServerState(ServerConstants.SERVER_STATE_CONNECTED);
            clientData.setThreadRegisteredPath();
            clientData.setThreadRegisteredData();
            clientkey.attach(clientData);
            if(LOG.isDebugEnabled())
                LOG.debug(serverName + ". Accept processed");
        } catch (IOException ie) {
            LOG.error(serverName + ". Cannot Accept connection: " + ie.getMessage());
            utils.updateServerState(ServerConstants.SERVER_STATE_CONNECT_REJECTED);
        }
    }
    private void processRead(SelectionKey key) {

        key.interestOps( key.interestOps() ^ SelectionKey.OP_READ);

        SocketChannel client = (SocketChannel) key.channel();
        Socket s = client.socket();
        long readLength=0;

        ClientData clientData = (ClientData) key.attachment();
        if (false == timeouts.isEmpty() ){
            if (true == timeouts.containsKey(key))
                timeouts.remove(key);
            if (true == timeouts.containsKey(serverkey))
                timeouts.remove(serverkey);
        }
        try {
            while ((readLength = client.read(clientData.bbBuf)) > 0) {
                clientData.total_read += readLength;
                if(LOG.isDebugEnabled())
                    LOG.debug(serverName + ". Read readLength  " + readLength + "  total " + clientData.total_read);
            }
            if (readLength == -1 ){
                throw new IOException(serverName + ". Connection closed by peer on READ");
            }
            if (clientData.total_read < ServerConstants.HEADER_SIZE){
                key.interestOps(key.interestOps() | SelectionKey.OP_READ);
                key.attach(clientData);
                timeouts.put(key, System.currentTimeMillis());
                if(LOG.isDebugEnabled())
                    LOG.debug(serverName + ". Read length less than HEADER size. Added timeout on READ");
                return;
            }
            if (ServerConstants.BUFFER_INIT == clientData.buffer_state){
                clientData.buffer_state = ServerConstants.HEADER_PROCESSED;
                ByteBuffer bbHeader = clientData.bbBuf[0];
                bbHeader.flip();
                clientData.hdr.extractFromByteArray(bbHeader);
                
                if (clientData.hdr.getSignature() != ServerConstants.SIGNATURE)
                    throw new IOException(serverName + ". Wrong signature in read Header : " + clientData.hdr.getSignature());
            }
            if (clientData.total_read < (clientData.hdr.getTotalLength() + ServerConstants.HEADER_SIZE)){
                
                int dataLength = clientData.hdr.getTotalLength();
                int availableBuffer = clientData.bbBody.capacity() - clientData.bbBody.position();
                
                if (clientData.hdr.getTotalLength() > clientData.bbBody.capacity()){
                    clientData.bbBuf[1] = clientData.bbBody = ByteBufferUtils.increaseCapacity(clientData.bbBody, dataLength > ServerConstants.BODY_SIZE ? dataLength : ServerConstants.BODY_SIZE );
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverName + ". Total length in Header greater than buffer capacity. Increased buffer capacity [" + clientData.bbBody.capacity() + "]" );
                }
                else {
                    timeouts.put(key, System.currentTimeMillis());
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverName + ". Total read length less than in HEADER. Added timeout on READ");
                }
                key.attach(clientData);
                key.interestOps(key.interestOps() | SelectionKey.OP_READ);
                return;
            }
            if (clientData.total_read > (clientData.hdr.getTotalLength() + ServerConstants.HEADER_SIZE)){
                throw new IOException(serverName + ". Total read length greater than in HEADER. [" + clientData.total_read + "]/[" + (clientData.hdr.getTotalLength() + ServerConstants.HEADER_SIZE) + "]");
            }
             
            key.attach(clientData);
            this.worker.processData(this, key);
            
            if(LOG.isDebugEnabled())
                LOG.debug(serverName + ". Read processed");
            return;

        } catch (Exception e){
            LOG.error(serverName + ". Exception: " + e.getMessage());
            closeClientConnection(key);
            utils.updateServerState(ServerConstants.SERVER_STATE_DISCONNECTED);
        }
    }
    private void processWrite(SelectionKey key) {
        
        key.interestOps(key.interestOps() ^ SelectionKey.OP_WRITE);

        SocketChannel client = (SocketChannel) key.channel();
        Socket s = client.socket();
        long writeLength=0;
        ClientData clientData = (ClientData) key.attachment();
        
        if (false == timeouts.isEmpty() ){
            if (true == timeouts.containsKey(key))
                timeouts.remove(key);
            if (true == timeouts.containsKey(serverkey))
                timeouts.remove(serverkey);
        }
        try {
            while ((writeLength = client.write(clientData.bbBuf)) > 0) {
                clientData.total_write += writeLength;
                if(LOG.isDebugEnabled())
                    LOG.debug(serverName + ". Write writeLength  " + writeLength + "  total " + clientData.total_write);
            }
            if (writeLength == -1 ){
                throw new IOException(serverName + ". Connection closed by peer on WRITE");
            }
            if (clientData.bbBuf[0].hasRemaining() || clientData.bbBuf[1].hasRemaining()){
                key.attach(clientData);
                timeouts.put(key, System.currentTimeMillis());
                if(LOG.isDebugEnabled())
                    LOG.debug(serverName + ". Write length less than total write length. Added timeout on WRITE");
                key.interestOps(key.interestOps() | SelectionKey.OP_WRITE);
            } else {
                if(LOG.isDebugEnabled())
                    LOG.debug(serverName + ". Write processed");
                if (clientData.request == ServerConstants.REQUST_WRITE_READ){
                    key.interestOps(key.interestOps() | SelectionKey.OP_READ);
                    clientData.resetReadData();
                }
                else if (clientData.request == ServerConstants.REQUST_WRITE_CLOSE){
                    closeClientConnection(key);
                    utils.updateServerState(ServerConstants.SERVER_STATE_DISCONNECTED);
                }
            }
            return;
        } catch (IOException ie){
            LOG.error(serverName + ". IOException: " + ie.getMessage());
            closeClientConnection(key);
            utils.updateServerState(ServerConstants.SERVER_STATE_DISCONNECTED);
        }
    }
}


