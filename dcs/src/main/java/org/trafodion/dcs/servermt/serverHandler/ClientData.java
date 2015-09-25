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

import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.sql.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverDriverInputOutput.*;
import org.trafodion.dcs.servermt.serverSql.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;

public class ClientData {
    private static  final Log LOG = LogFactory.getLog(ClientData.class);
    ByteBuffer bbHeader = null;
    ByteBuffer bbBody = null;
    ByteBuffer[] bbBuf = null;
    int total_read;
    int total_write;
    int buffer_state = ServerConstants.BUFFER_INIT;

    Header hdr = null;
    SocketAddress clientSocketAddress = null;
    int request = ServerConstants.REQUST_INIT;
    
    int dialogueId = 0;
    int nodeNumber = 0;
    int processId = 0;
    String processName = "";
    String hostName = "";
    int portNumber = 0;
    String clientHostName = "";
    String clientIpAddress = "";
    int clientPortNumber = 0;
    String clientApplication = "";
    
    ServerUtils utils = null;
    private String threadRegisteredPath = "";
    private String threadRegisteredData = null;
    
    Configuration conf = null;
//=========== SQL =====================================
   TrafConnection trafConnection = null;
//=====================================================    
    ClientData(SocketAddress clientSocketAddress, ServerUtils utils, Configuration conf){

    bbHeader = ByteBufferUtils.allocate(ServerConstants.HEADER_SIZE,true);
    bbBody = ByteBufferUtils.allocate(ServerConstants.BODY_SIZE,true);
    bbBuf = new ByteBuffer[]{bbHeader,bbBody};
    bbHeader.order(ByteOrder.LITTLE_ENDIAN);
    bbBody.order(ByteOrder.LITTLE_ENDIAN);
    
    total_read = 0;
    total_write = 0;
    
    hdr = new Header();
    
    this.clientSocketAddress = clientSocketAddress;
    this.utils = utils;
    this.conf = conf;
    }
    void resetReadData(){
        total_read = 0;
        total_write = 0;
        buffer_state = ServerConstants.BUFFER_INIT;
        bbHeader.clear();
        bbBody.clear();
    }
    void setThreadRegisteredPath(){
        threadRegisteredPath = utils.getThreadRegisteredPath();
    }
    void setThreadRegisteredData(){
        threadRegisteredData = utils.getThreadRegisteredData();
    }
    String getThreadRegisteredPath(){
        return threadRegisteredPath;        
    }
    String getThreadRegisteredData(){
        return threadRegisteredData;        
    }
    ByteBuffer[] getByteBufferArray(){
        return bbBuf;
    }
    int getRequest(){
        return request;
    }
    int getDialogueId(){
        return dialogueId;
    }
    int getNodeNumber(){
        return nodeNumber;
    }
    int getProcessId(){
        return processId;
    }
    String getProcessName(){
        return processName;
    }
    String getHostName(){
        return hostName;
    }
    int getPortNumber(){
        return portNumber;
    }
    String getClientHostName(){
        return clientHostName;
    }
    String getClientIpAddress(){
        return clientIpAddress;
    }
    int getClientPortNumber(){
        return clientPortNumber;
    }
    String getClientApplication(){
        return clientApplication;
    }
    TrafConnection getTrafConnection(){
        return trafConnection;
    }

//=====================================================
    
    void setByteBufferHeader(ByteBuffer bbHeader){
        this.bbHeader = bbHeader;
    }
    void setByteBufferBody(ByteBuffer bbBody){
        this.bbBody = bbBody;
    }
    void setByteBufferArray(ByteBuffer bbHeader, ByteBuffer bbBody){
        setByteBufferHeader(bbHeader);
        setByteBufferBody(bbBody);
        this.bbBuf[0] = bbHeader;
        this.bbBuf[1] = bbBody;
    }
    void setHdr(Header hdr){
        this.hdr = hdr;
    }
    void setDialogueId(int dialogueId){
        this.dialogueId = dialogueId;
    }
    void setNodeNumber(int nodeNumber){
        this.nodeNumber = nodeNumber;
    }
    void setProcessId(int processId){
        this.processId = processId;
    }
    void setProcessName(String processName){
        this.processName = processName;
    }
    void setHostName(String hostName){
        this.hostName = hostName;
    }
    void setPortNumber(int portNumber){
        this.portNumber = portNumber;
    }
    void setClientHostName(String clientHostName){
        this.clientHostName = clientHostName;
    }
    void setClientIpAddress(String clientIpAddress){
        this.clientIpAddress = clientIpAddress;
    }
    void setClientPortNumber(int clientPortNumber){
        this.clientPortNumber = clientPortNumber;
    }
    void setClientApplication(String clientApplication){
        this.clientApplication = clientApplication;
    }
    void setRequest(int request){
        this.request = request;
    }
    void setRequestAndDisconnect(){
        this.request = ServerConstants.REQUST_CLOSE;
        if (trafConnection != null)
            trafConnection.closeTConnection();
        trafConnection = null;
    }
    void setTrafConnection(TrafConnection trafConnection){
        this.trafConnection = trafConnection;
    }
    public Configuration getConf(){
        return conf;
    }
}
