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

public class RequestUnknown {
    private static  final Log LOG = LogFactory.getLog(RequestUnknown.class);

    private ZkClient zkc = null;
    private String parentZnode = "";

    RequestUnknown(ZkClient zkc,String parentZnode){
        this.zkc = zkc;
        this.parentZnode = parentZnode;
    }

    ClientData processRequest(ClientData clientData, Socket s) { 
        boolean cancelConnection = false;
        GetObjRefException exception = new GetObjRefException();
        ByteBuffer header = clientData.header;
        ByteBuffer body = clientData.body;
        Header hdr = clientData.hdr;
        SocketAddress clientSocketAddress = clientData.clientSocketAddress;
        
        header.flip();
        hdr.extractFromByteArray(header);
        
        header.clear();
        body.clear();
        exception.exception_nr = ListenerConstants.DcsMasterParamError_exn;
        exception.ErrorText = "Api is not implemented [" + hdr.getOperationId() + "]";
        LOG.debug(clientSocketAddress + ": " + exception.ErrorText);
        try {
            exception.insertIntoByteBuffer(body);
            body.flip();
            hdr.setTotalLength(body.limit());
            hdr.insertIntoByteBuffer(header);
            header.flip();

            clientData.header = header;
            clientData.body = body;
            clientData.hdr = hdr;
        } catch (UnsupportedEncodingException ue){
            if(LOG.isErrorEnabled())
                LOG.error("UnsupportedEncodingException in RequestUnknown: " + s.getRemoteSocketAddress() + ": " + ue.getMessage(), ue);
            cancelConnection = true;
        } catch (Exception e){
            if(LOG.isErrorEnabled())
                LOG.error("Exception in RequestUnknown: " + s.getRemoteSocketAddress() + ": " + e.getMessage(), e);
            cancelConnection = true;
        }
        header = null;
        body = null;
        hdr = null;
        clientSocketAddress = null;
        exception = null;

        if (cancelConnection == true)
            clientData.requestReply = ListenerConstants.REQUST_CLOSE;
        else
            clientData.requestReply = ListenerConstants.REQUST_WRITE_EXCEPTION;
         return clientData;
    }
}
