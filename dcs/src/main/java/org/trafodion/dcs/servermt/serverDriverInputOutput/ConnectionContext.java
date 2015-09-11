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
package org.trafodion.dcs.servermt.serverDriverInputOutput; 

import java.util.*;
import java.nio.*;
import java.io.*;
import java.math.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
    
public class ConnectionContext {

    private static  final Log LOG = LogFactory.getLog(ConnectionContext.class);

    private String datasource = "";
    private String catalog = "";
    private String schema = "";
    private String location = "";
    private String userRole = "";

    private short accessMode = 0;
    private short autoCommit = 0;
    private int queryTimeoutSec = 0;
    private int idleTimeoutSec = 0;
    private int loginTimeoutSec = 0;
    private short txnIsolationLevel = 0;
    private short rowSetSize = 0;

    private int diagnosticFlag = 0;
    private int processId = 0;

    private String computerName = "";
    private String windowText = "";

    private int ctxACP = 0;
    private int ctxDataLang = 0;
    private int ctxErrorLang = 0;
    private short ctxCtrlInferNXHAR = 0;
    
    private short cpuToUse;
    private short cpuToUseEnd;

    private String connectOptions = "";
    
    private VersionList clientVersionList = null;
    
    private int dialogueId = 0;
    private long contextOptions1 = 0L;
    private long contextOptions2 = 0L;

    private String sessionName = "";
    private String clientUserName = "";

    public ConnectionContext(){
        clientVersionList = new VersionList();
    }

    public void extractFromByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException {
        try {
            datasource = ByteBufferUtils.extractString(bbBuf);
            catalog= ByteBufferUtils.extractString(bbBuf);
            schema= ByteBufferUtils.extractString(bbBuf);
            location= ByteBufferUtils.extractString(bbBuf);
            userRole= ByteBufferUtils.extractString(bbBuf);
    
            accessMode=bbBuf.getShort();
            autoCommit=bbBuf.getShort();
            queryTimeoutSec=bbBuf.getInt();
            idleTimeoutSec=bbBuf.getInt();
            loginTimeoutSec=bbBuf.getInt();
            txnIsolationLevel=bbBuf.getShort();
            rowSetSize=bbBuf.getShort();
    
            diagnosticFlag=bbBuf.getInt();
            processId=bbBuf.getInt();
    
            computerName=ByteBufferUtils.extractString(bbBuf);
            windowText=ByteBufferUtils.extractString(bbBuf);
    
            ctxACP=bbBuf.getInt();
            ctxDataLang=bbBuf.getInt();
            ctxErrorLang=bbBuf.getInt();
            ctxCtrlInferNXHAR=bbBuf.getShort();

            cpuToUse=bbBuf.getShort();
            cpuToUseEnd=bbBuf.getShort();
            connectOptions=ByteBufferUtils.extractString(bbBuf);

            clientVersionList.extractFromByteBuffer(bbBuf);
            dialogueId = bbBuf.getInt();
            contextOptions1 = ByteBufferUtils.extractUInt(bbBuf);
            contextOptions2 = ByteBufferUtils.extractUInt(bbBuf);
            
            if(ServerConstants.INCONTEXT_OPT1_SESSIONNAME == (contextOptions1 & ServerConstants.INCONTEXT_OPT1_SESSIONNAME))
                sessionName=ByteBufferUtils.extractString(bbBuf);
            
            if(ServerConstants.INCONTEXT_OPT1_CLIENT_USERNAME == (contextOptions1 & ServerConstants.INCONTEXT_OPT1_CLIENT_USERNAME))
                clientUserName=ByteBufferUtils.extractString(bbBuf);

            debugConnectionContext("extract");
        } catch(UnsupportedEncodingException ue){
            LOG.error("ConnectionContext :UnsupportedEncodingException :" + ue);
            throw new UnsupportedEncodingException(ue.getMessage());
        }
    }

    public void debugConnectionContext(String function){
        if(LOG.isDebugEnabled()){
            LOG.debug("Function :" + function);
            LOG.debug("datasource :"+datasource);
            LOG.debug("catalog :"+catalog);
            LOG.debug("schema :"+schema);
            LOG.debug("location :"+location);
            LOG.debug("userRole :"+userRole);
            LOG.debug("accessMode :"+accessMode);
            LOG.debug("autoCommit :"+autoCommit);
            LOG.debug("queryTimeoutSec :"+queryTimeoutSec);
            LOG.debug("idleTimeoutSec :"+idleTimeoutSec);
            LOG.debug("loginTimeoutSec :"+loginTimeoutSec);
            LOG.debug("txnIsolationLevel :"+txnIsolationLevel);
            LOG.debug("rowSetSize :"+rowSetSize);
            LOG.debug("diagnosticFlag :"+diagnosticFlag);
            LOG.debug("processId :"+processId);
            LOG.debug("computerName :"+computerName);
            LOG.debug("windowText :"+windowText);
            LOG.debug("ctxACP :"+ctxACP);
            LOG.debug("ctxDataLang :"+ctxDataLang);
            LOG.debug("ctxErrorLang :"+ctxErrorLang);
            LOG.debug("ctxCtrlInferNXHAR :"+ctxCtrlInferNXHAR);
            LOG.debug("cpuToUse :"+cpuToUse);
            LOG.debug("cpuToUseEnd :"+cpuToUseEnd);
            LOG.debug("connectOptions :"+connectOptions);
            LOG.debug("dialogueId :" + dialogueId);
            LOG.debug("contextOptions1 :" + contextOptions1);
            LOG.debug("contextOptions2 :" + contextOptions2);
            LOG.debug("sessionName :" + sessionName);
            LOG.debug("clientUserName :" + clientUserName);
        }
    }
    public String getDatasource() {
        return datasource;
    }
    public String getCatalog() {
        return catalog;
    }
    public String getSchema() {
        return schema;
    }
    public String getLocation() {
        return location;
    }
    public String getUserRole() {
        return userRole;
    }
    public short getAccessMode() {
        return accessMode;
    }
    public short getAutoCommit() {
        return autoCommit;
    }
    public int getQueryTimeoutSec() {
        return queryTimeoutSec;
    }
    public int getIdleTimeoutSec() {
        return idleTimeoutSec;
    }
    public int getLoginTimeoutSec() {
        return loginTimeoutSec;
    }
    public short getTxnIsolationLevel() {
        return txnIsolationLevel;
    }
    public short getRowSetSize() {
        return rowSetSize;
    }
    public int getDiagnosticFlag() {
        return diagnosticFlag;
    }
    public int getProcessId() {
        return processId;
    }
    public String getComputerName() {
        return computerName;
    }
    public String getWindowText() {
        return windowText;
    }
    public int getCtxACP() {
        return ctxACP;
    }
    public int getCtxDataLang() {
        return ctxDataLang;
    }
    public int getCtxErrorLang() {
        return ctxErrorLang;
    }
    public short getCtxCtrlInferNXHAR() {
        return ctxCtrlInferNXHAR;
    }
    public short getCpuToUse() {
        return cpuToUse;
    }
    public short getCpuToUseEnd() {
        return cpuToUseEnd;
    }
    public String getConnectOptions() {
        return connectOptions;
    }
    public VersionList getClientVersionList() {
        return clientVersionList;
    }
    public int getDialogueId() {
        return dialogueId;
    }
    public long getContextOptions1() {
        return contextOptions1;
    }
    public long getContextOptions2() {
        return contextOptions2;
    }
    public String getSessionName() {
        return sessionName;
    }
    public String getClientUserName() {
        return clientUserName;
    }
}
