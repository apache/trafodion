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

import java.util.*;

import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.util.LinkedList;
import java.util.List;
import java.sql.*;
import java.lang.management.ManagementFactory;
import java.util.concurrent.CountDownLatch;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooKeeper;
import org.trafodion.dcs.zookeeper.ZkClient;

import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;

public class ServerWorker extends Thread {
    private static  final Log LOG = LogFactory.getLog(ServerWorker.class);
    private List<DataEvent> queue = new LinkedList<DataEvent>();
    private ZkClient zkc=null;
    private int instance;
    private int serverThread;
    private String serverName;
    private String serverWorkerName;
    private byte[] cert;
    private CountDownLatch latch = null;
    
    private ServerApiSqlConnect serverApiSqlConnect;
    private ServerApiSqlSetConnectAttr serverApiSqlSetConnectAttr; 
    private ServerApiSqlPrepare serverApiSqlPrepare;
    private ServerApiSqlExecute serverApiSqlExecute;
    private ServerApiSqlExecDirect serverApiSqlExecDirect;
    private ServerApiSqlFetch serverApiSqlFetch;
    private ServerApiSqlDisconnect serverApiSqlDisconnect;
    private ServerApiSqlClose serverApiSqlClose;
    private ServerApiSqlEndTransact serverApiSqlEndTransact;
    private ServerApiGetCatalogs serverApiGetCatalogs;

    ServerWorker(ZkClient zkc, int instance, int serverThread, String serverName, byte[] cert){
        this.zkc=zkc;
        this.instance = instance;
        this.serverThread = serverThread;
        this.serverName = serverName;
        serverWorkerName = ServerConstants.SERVER_WORKER_NAME + "_" + instance + "_" + serverThread;
        this.cert = cert;

        serverApiSqlConnect = new ServerApiSqlConnect(instance, serverThread, cert);
        serverApiSqlSetConnectAttr = new ServerApiSqlSetConnectAttr(instance, serverThread);
        serverApiSqlPrepare = new ServerApiSqlPrepare(instance, serverThread);
        serverApiSqlExecute = new ServerApiSqlExecute(instance, serverThread);
        serverApiSqlExecDirect = new ServerApiSqlExecDirect(instance, serverThread);
        serverApiSqlFetch = new ServerApiSqlFetch(instance, serverThread);
        serverApiSqlDisconnect = new ServerApiSqlDisconnect(instance, serverThread);
        serverApiSqlClose = new ServerApiSqlClose(instance, serverThread);
        serverApiSqlEndTransact = new ServerApiSqlEndTransact(instance, serverThread);
        serverApiGetCatalogs = new ServerApiGetCatalogs(instance, serverThread);
    }
    public void closeTrafConnection(SelectionKey key) {
        latch = new CountDownLatch(1);
        synchronized(queue) {
            queue.add(new DataEvent(null, key));
            queue.notify();
        }
        try{
            latch.await();
        }catch(InterruptedException e){
            e.printStackTrace();
        }
    }
    public void processData(ServerHandler server, SelectionKey key) {
        synchronized(queue) {
            queue.add(new DataEvent(server, key));
            queue.notify();
        }
    }
    public void run() {
        DataEvent dataEvent;

        while(true) {
            // Wait for data to become available
            synchronized(queue) {
                while(queue.isEmpty()) {
                    try {
                        queue.wait();
                    } catch (InterruptedException e) {}
                }
                dataEvent = queue.remove(0);
            }
            SelectionKey key = dataEvent.key;
            ClientData clientData = (ClientData) key.attachment();
            if (dataEvent.server == null){
                // ServerHandler sent request to close SQL Connection
                if (clientData.getTrafConnection() != null){
                    clientData.getTrafConnection().closeTConnection();
                    clientData.setTrafConnection(null);
                }
                latch.countDown();
                continue;
            }
            SocketChannel client = (SocketChannel) key.channel();
            Socket s = client.socket();
            ServerHandler server = dataEvent.server;
            dataEvent.key = null;
            dataEvent.server = null;
            clientData.setRequest(ServerConstants.REQUST_CLOSE);

            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". Api Entry: " + ServerUtils.convertOpIdToString(clientData.hdr.getOperationId()) + " :" + clientData.hdr.getOperationId() + "---------------------------");
            switch (clientData.hdr.getOperationId()){
            case ServerConstants.SRVR_API_INIT:
                break;
            case ServerConstants.SRVR_API_SQLCONNECT:
                clientData = serverApiSqlConnect.processApi(clientData );
                break;
            case ServerConstants.SRVR_API_SQLDISCONNECT:
                clientData = serverApiSqlDisconnect.processApi(clientData ); 
                break;
            case ServerConstants.SRVR_API_SQLSETCONNECTATTR:
                clientData = serverApiSqlSetConnectAttr.processApi(clientData );
                break;
            case ServerConstants.SRVR_API_SQLENDTRAN:
                clientData = serverApiSqlEndTransact.processApi(clientData ); 
                break;
            case ServerConstants.SRVR_API_SQLPREPARE:
                clientData = serverApiSqlPrepare.processApi(clientData );
                break;
            case ServerConstants.SRVR_API_SQLEXECUTE_ROWSET:
                break;
            case ServerConstants.SRVR_API_SQLEXECDIRECT_ROWSET:
                break;
            case ServerConstants.SRVR_API_SQLEXECDIRECT:
                clientData = serverApiSqlExecDirect.processApi(clientData );
                break;
            case ServerConstants.SRVR_API_SQLEXECUTE:
                break;
            case ServerConstants.SRVR_API_SQLEXECUTECALL:
                break;
            case ServerConstants.SRVR_API_SQLEXECUTE2:
                clientData = serverApiSqlExecute.processApi(clientData );
                break;
            case ServerConstants.SRVR_API_SQLFETCH:
                clientData = serverApiSqlFetch.processApi(clientData ); 
                break;
            case ServerConstants.SRVR_API_SQLFREESTMT:
                clientData = serverApiSqlClose.processApi(clientData ); 
                break;
            case ServerConstants.SRVR_API_GETCATALOGS:
                clientData = serverApiGetCatalogs.processApi(clientData ); 
                break;
            case ServerConstants.SRVR_API_STOPSRVR:
                break;
            case ServerConstants.SRVR_API_ENABLETRACE:
                break;
            case ServerConstants.SRVR_API_DISABLETRACE:
                break;
            case ServerConstants.SRVR_API_ENABLE_SERVER_STATISTICS:
                break;
            case ServerConstants.SRVR_API_DISABLE_SERVER_STATISTICS:
                break;
            case ServerConstants.SRVR_API_UPDATE_SERVER_CONTEXT:
                break;
            default:
            }
            key.attach(clientData);
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". request :" + ServerUtils.convertRequestToString(clientData.getRequest()));
            server.send(new PendingRequest(key));
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". Api Exit : " + ServerUtils.convertOpIdToString(clientData.hdr.getOperationId()) + " :" + clientData.hdr.getOperationId() + " ---------- ");
        }
    }
}
