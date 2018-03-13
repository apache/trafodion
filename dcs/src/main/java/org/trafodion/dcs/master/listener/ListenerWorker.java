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
package org.trafodion.dcs.master.listener;

import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.util.LinkedList;
import java.util.List;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooKeeper;
import org.trafodion.dcs.zookeeper.ZkClient;
import org.trafodion.dcs.Constants;

public class ListenerWorker extends Thread {
    private static  final Log LOG = LogFactory.getLog(ListenerWorker.class);
    private List<DataEvent> queue = new LinkedList<DataEvent>();
    private ZkClient zkc=null;
    ConnectReply connectReplay = null;
    private String parentZnode;
    
    private RequestGetObjectRef requestGetObjectRef = null;
    private RequestCancelQuery requestCancelQuery = null;
    private RequestUnknown requestUnknown = null;
    
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
    ListenerWorker(ZkClient zkc,String parentZnode){	
        this.zkc=zkc;
        this.parentZnode=parentZnode;
        connectReplay = new ConnectReply(zkc,parentZnode);
        
        requestGetObjectRef = new RequestGetObjectRef(zkc,parentZnode);
        requestCancelQuery = new RequestCancelQuery(zkc,parentZnode);
        requestUnknown = new RequestUnknown(zkc,parentZnode);
        
        System.setProperty("hbaseclient.log4j.properties",System.getProperty("dcs.conf.dir") + "/log4j.properties");
        System.setProperty("dcs.root.logger",System.getProperty("dcs.root.logger"));
        System.setProperty("dcs.log.dir",System.getProperty("dcs.log.dir"));
        System.setProperty("dcs.log.file",System.getProperty("dcs.log.file"));
    }
    
    public void processData(ListenerService server, SelectionKey key) {
        synchronized(queue) {
            queue.add(new DataEvent(server, key));
            queue.notify();
        }
    }

    public void run() {
        DataEvent dataEvent;
    
        while(true) {
            try {
                // Wait for data to become available
                synchronized(queue) {
                    while(queue.isEmpty()) {
                        try {
                            queue.wait();
                        } catch (InterruptedException e) {
                            LOG.error("ListenerWorker be interrupted by DCS master, exit thread.", e);
                            return;
                        }
                    }
                    dataEvent = queue.remove(0);
                }
                SelectionKey key = dataEvent.key;
                SocketChannel client = (SocketChannel) key.channel();
                Socket s = client.socket();
                ClientData clientData = (ClientData) key.attachment();
                ListenerService server = dataEvent.server;
                dataEvent.key = null;
                dataEvent.server = null;

                switch (clientData.hdr.getOperationId()){
                    case ListenerConstants.DCS_MASTER_GETSRVRAVAILABLE:
                        clientData = requestGetObjectRef.processRequest(clientData, s);
                        break;
                    case ListenerConstants.DCS_MASTER_CANCELQUERY:
                        clientData = requestCancelQuery.processRequest(clientData, s);
                        break;
                    default:
                        clientData = requestUnknown.processRequest(clientData, s);
                        break;
                }
                // Return to sender
                int requestReply = clientData.requestReply;
                key.attach(clientData);
                server.send(new PendingRequest(key, requestReply));
            } catch (Exception e){
                LOG.error("Unexpected Exception", e);
                System.exit(-1);
            }
        }
    }
}

