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

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooKeeper;
import org.trafodion.dcs.zookeeper.ZkClient;

public class RequestGetObjectRef {
    private static  final Log LOG = LogFactory.getLog(RequestGetObjectRef.class);

    private ZkClient zkc = null;
    private String parentZnode = "";
    private ConnectReply connectReplay = null;

    RequestGetObjectRef(ZkClient zkc,String parentZnode){
        this.zkc = zkc;
        this.parentZnode = parentZnode;
        connectReplay = new ConnectReply(zkc,parentZnode);
    }

    ClientData processRequest(ClientData clientData, Socket s) { 
        boolean cancelConnection = false;
        boolean replyException = false;
        try {
            replyException = buildConnectReply(clientData);
        } catch (UnsupportedEncodingException ue){
            if(LOG.isErrorEnabled())
                LOG.error("UnsupportedEncodingException in RequestGetObjectRef: " + s.getRemoteSocketAddress() + ": " + ue.getMessage(), ue);
            cancelConnection = true;
        } catch (IOException io){
            if(LOG.isErrorEnabled())
                LOG.error("IOException in RequestGetObjectRef: " + s.getRemoteSocketAddress() + ": " + io.getMessage(), io);
            cancelConnection = true;
        } catch (BufferUnderflowException e){
            if(LOG.isErrorEnabled())
                LOG.error("BufferUnderflowException in RequestGetObjectRef: " + s.getRemoteSocketAddress() + ": " + e.getMessage(), e);
            cancelConnection = true;
        }
        // Return to sender
        if (cancelConnection == true)
            clientData.requestReply = ListenerConstants.REQUST_CLOSE;
        else if (replyException == true)
            clientData.requestReply = ListenerConstants.REQUST_WRITE_EXCEPTION;
        else
            clientData.requestReply = ListenerConstants.REQUST_WRITE;
        return clientData;
    }
    
    boolean buildConnectReply(ClientData clientData) throws UnsupportedEncodingException, IOException {

        boolean replyException = false;

        ByteBuffer header = clientData.header;
        ByteBuffer body = clientData.body;
        Header hdr = clientData.hdr;
        ConnectionContext conectContex = clientData.conectContex;
        SocketAddress clientSocketAddress = null;
        clientSocketAddress = clientData.clientSocketAddress;
    
        header.flip();
        hdr.extractFromByteArray(header);
        
        body.flip();
        conectContex.extractFromByteBuffer(body);
        
        replyException = connectReplay.buildConnectReply(hdr, conectContex, clientSocketAddress);
        
        header.clear();
        body.clear();
        
        switch(hdr.getVersion()){
        
            case ListenerConstants.CLIENT_HEADER_VERSION_BE: //from jdbc
                hdr.setSwap(ListenerConstants.YES);
                header.order(ByteOrder.BIG_ENDIAN);
                body.order(ByteOrder.LITTLE_ENDIAN);
                hdr.setVersion(ListenerConstants.SERVER_HEADER_VERSION_LE);
                break;
            case ListenerConstants.CLIENT_HEADER_VERSION_LE: //from odbc
                hdr.setSwap(ListenerConstants.NO);
                header.order(ByteOrder.LITTLE_ENDIAN);
                body.order(ByteOrder.LITTLE_ENDIAN);
                hdr.setVersion(ListenerConstants.SERVER_HEADER_VERSION_LE);
                break;
            default:
                throw new IOException(clientSocketAddress + ": " + "Wrong Header Version");
        }
        connectReplay.insertIntoByteBuffer(body);
        body.flip();
        hdr.setTotalLength(body.limit());
        hdr.insertIntoByteBuffer(header);
        header.flip();
    
        clientData.header = header;
        clientData.body = body;
        clientData.hdr = hdr;
        clientData.conectContex = conectContex;
    
        header = null;
        body = null;
        hdr = null;
        conectContex = null;
        clientSocketAddress = null;
    
        return replyException;
    }
}
