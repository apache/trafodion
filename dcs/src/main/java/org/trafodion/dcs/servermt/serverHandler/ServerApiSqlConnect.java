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

import java.sql.*;
import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.util.Properties;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverDriverInputOutput.*;
import org.trafodion.dcs.servermt.serverSql.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooKeeper;
import org.trafodion.dcs.zookeeper.ZkClient;

public class ServerApiSqlConnect {
    static final int odbc_SQLSvc_InitializeDialogue_ParamError_exn_ = 1;
    static final int odbc_SQLSvc_InitializeDialogue_InvalidConnection_exn_ = 2;
    static final int odbc_SQLSvc_InitializeDialogue_SQLError_exn_ = 3;
    static final int odbc_SQLSvc_InitializeDialogue_SQLInvalidHandle_exn_ = 4;
    static final int odbc_SQLSvc_InitializeDialogue_SQLNeedData_exn_ = 5;
    static final int odbc_SQLSvc_InitializeDialogue_InvalidUser_exn_ = 6;

    private static  final Log LOG = LogFactory.getLog(ServerApiSqlConnect.class);
    private int instance;
    private int serverThread;
    private String serverWorkerName;
    private ClientData clientData;
//
    private String threadRegisteredPath = "";
    private String threadRegisteredData = "";
    private byte[] data = null;
//
    private ConnectionContext connectionContext;
    private UserDesc userDesc;
    private byte[] cert;
//
    private ServerException serverException;
    private OutConnectionContext outConnectionContext;
    private TrafConnection trafConnection;
    
    ServerApiSqlConnect(int instance, int serverThread, byte[] cert) { 
        this.instance = instance;
        this.serverThread = serverThread;
        serverWorkerName = ServerConstants.SERVER_WORKER_NAME + "_" + instance + "_" + serverThread;
        this.cert = cert;
    }
    void init (){
        connectionContext = new ConnectionContext();
        userDesc = new UserDesc();
        serverException = new ServerException();
        outConnectionContext = new OutConnectionContext(cert);
        trafConnection = null;
    }
    void reset(){
        connectionContext = null;
        userDesc = null;
        serverException = null;
        outConnectionContext = null;
    }
    ClientData processApi(ClientData clientData) {  
        this.clientData = clientData;
        init();
//
// ==============process input ByteBuffer===========================
// hdr + userDesc + connectionContext
//
        ByteBuffer bbHeader = clientData.bbHeader;
        ByteBuffer bbBody = clientData.bbBody;
        Header hdr = clientData.hdr;

        bbHeader.flip();
        bbBody.flip();
        
        try {

            hdr.extractFromByteArray(bbHeader);
            userDesc.extractFromByteBuffer(bbBody);
            connectionContext.extractFromByteBuffer(bbBody);
//            
//=====================Display input data=========================================
//            
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". threadRegisteredData :" + clientData.getThreadRegisteredData());
            
            String[] st = clientData.getThreadRegisteredData().split(":");
            clientData.setDialogueId(Integer.parseInt(st[2]));
            clientData.setNodeNumber(Integer.parseInt(st[3]));
            clientData.setProcessId(Integer.parseInt(st[4]));
            clientData.setProcessName(st[5]);
            clientData.setHostName(st[6]);
            clientData.setPortNumber(Integer.parseInt(st[7]));
            clientData.setClientHostName(st[8]);
            clientData.setClientIpAddress(st[9]);
            clientData.setClientPortNumber(Integer.parseInt(st[10]));
            clientData.setClientApplication(st[11]);

            if(LOG.isDebugEnabled()){
                LOG.debug(serverWorkerName + ". dialogueId :" + clientData.getDialogueId());
                LOG.debug(serverWorkerName + ". nodeNumber :" + clientData.getNodeNumber());
                LOG.debug(serverWorkerName + ". processId :" + clientData.getProcessId());
                LOG.debug(serverWorkerName + ". processName :" + clientData.getProcessName());
                LOG.debug(serverWorkerName + ". hostName :" + clientData.getHostName());
                LOG.debug(serverWorkerName + ". portNumber :" + clientData.getPortNumber());
                LOG.debug(serverWorkerName + ". clientHostName :" + clientData.getClientHostName());
                LOG.debug(serverWorkerName + ". clientIpAddress :" + clientData.getClientIpAddress());
                LOG.debug(serverWorkerName + ". clientPortNumber :" + clientData.getClientPortNumber());
                LOG.debug(serverWorkerName + ". clientApplication :" + clientData.getClientApplication());
            }
            if (connectionContext.getDialogueId() < 1 ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId :" + connectionContext.getDialogueId());
            }
            if (connectionContext.getDialogueId() != clientData.getDialogueId() ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId sent by the Client [sent/expected] : [" + connectionContext.getDialogueId() + "/" + clientData.getDialogueId() + "]");
            }
//=====================Process SqlConnect===========================

            try {
                trafConnection = new TrafConnection(serverWorkerName, clientData, connectionContext);

                outConnectionContext.getVersionList().getList()[0].setComponentId((short)4);       //ODBC_SRVR_COMPONENT
                outConnectionContext.getVersionList().getList()[0].setMajorVersion((short)3);
                outConnectionContext.getVersionList().getList()[0].setMinorVersion((short)5);
                outConnectionContext.getVersionList().getList()[0].setBuildId(1);
                
                outConnectionContext.getVersionList().getList()[1].setComponentId((short)3);       //SQL_COMPONENT
                outConnectionContext.getVersionList().getList()[1].setMajorVersion((short)1);
                outConnectionContext.getVersionList().getList()[1].setMinorVersion((short)1);
                outConnectionContext.getVersionList().getList()[1].setBuildId(1);
                
                outConnectionContext.setNodeId((short)1);
                outConnectionContext.setProcessId(Integer.valueOf(ServerUtils.processId()));
                outConnectionContext.setComputerName(clientData.getHostName());
                outConnectionContext.setCatalog("");
                outConnectionContext.setSchema("");

                outConnectionContext.setOptionFlags1(ServerConstants.OUTCONTEXT_OPT1_ENFORCE_ISO88591 | ServerConstants.OUTCONTEXT_OPT1_DOWNLOAD_CERTIFICATE);
                outConnectionContext.setOptionFlags2(0);

                outConnectionContext.setRoleName("");
               
            } catch (SQLException ex){
                LOG.error(serverWorkerName + ". ServerApiSqlConnect.SQLException :" + ex);
                serverException.setServerException (odbc_SQLSvc_InitializeDialogue_SQLError_exn_, 0, ex);                
            }
//
//===================calculate length of output ByteBuffer========================
//
            bbHeader.clear();
            bbBody.clear();
//
// check if ByteBuffer is big enough for serverException + outConnectionContext
//        
            int dataLength = serverException.lengthOfData();
            dataLength += outConnectionContext.lengthOfData();
            int availableBuffer = bbBody.capacity() - bbBody.position();
            
            if(LOG.isDebugEnabled())
                LOG.debug("dataLength :" + dataLength + " availableBuffer :" + availableBuffer);
        
            if (dataLength > availableBuffer ) {
                bbBody = ByteBufferUtils.increaseCapacity(bbBody, dataLength > ServerConstants.BODY_SIZE ? dataLength : ServerConstants.BODY_SIZE );
                ByteBufferUtils.printBBInfo(bbBody);
                clientData.bbBuf[1] = bbBody;
            }
//===================== build output ==============================================
            serverException.insertIntoByteBuffer(bbBody);
            outConnectionContext.insertIntoByteBuffer(bbBody);
            bbBody.flip();
//=========================Update header================================
            hdr.setTotalLength(bbBody.limit());
            hdr.insertIntoByteBuffer(bbHeader);
            bbHeader.flip();

            clientData.setByteBufferArray(bbHeader, bbBody);
            clientData.setHdr(hdr);
            clientData.setRequest(ServerConstants.REQUST_WRITE_READ);
            clientData.setTrafConnection(trafConnection);
 
        } catch (SQLException se){
            LOG.error(serverWorkerName + ". Connect.SQLException :" + se);
            clientData.setRequestAndDisconnect();
        } catch (UnsupportedEncodingException ue){
            LOG.error(serverWorkerName + ". Connect.UnsupportedEncodingException :" + ue);
            clientData.setRequestAndDisconnect();
        } catch (Exception e){
            LOG.error(serverWorkerName + ". Connect.Exception :" + e);
            clientData.setRequestAndDisconnect();
        }
        reset();
        return clientData;
    }
}
