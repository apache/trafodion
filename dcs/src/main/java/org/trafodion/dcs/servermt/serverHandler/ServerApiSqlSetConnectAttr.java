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
import java.sql.SQLException;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverDriverInputOutput.*;
import org.trafodion.dcs.servermt.serverSql.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class ServerApiSqlSetConnectAttr {
    private static final int odbc_SQLSvc_SetConnectionOption_ParamError_exn_ = 1;
    private static final int odbc_SQLSvc_SetConnectionOption_InvalidConnection_exn_ = 2;
    private static final int odbc_SQLSvc_SetConnectionOption_SQLError_exn_ = 3;
    private static final int odbc_SQLSvc_SetConnectionOption_SQLInvalidHandle_exn_ = 4;
    
    private static  final Log LOG = LogFactory.getLog(ServerApiSqlSetConnectAttr.class);
    private int instance;
    private int serverThread;
    private String serverWorkerName;
    private ClientData clientData;
//    
    private SetConnectionOption setConnectionOption;
    private ServerException serverException;
    private ErrorDescList errorDescList;
    private int exception = 0;
    private int exception_detail = 0;
    private TrafConnection trafConnection;
    private int attr;
    private int option;
    
    ServerApiSqlSetConnectAttr(int instance, int serverThread) {  
        this.instance = instance;
        this.serverThread = serverThread;
        serverWorkerName = ServerConstants.SERVER_WORKER_NAME + "_" + instance + "_" + serverThread;
    }
    void init(){
        setConnectionOption = new SetConnectionOption();
        serverException = new ServerException();
        errorDescList = new ErrorDescList();
    }
    void reset(){
        setConnectionOption = null;
        serverException = null;
        errorDescList = null;
    }
    ClientData processApi(ClientData clientData) {  
        this.clientData = clientData;
        init();
// ==============process input ByteBuffer===========================
// hdr + setConnectionOption
//
        ByteBuffer bbHeader = clientData.bbHeader;
        ByteBuffer bbBody = clientData.bbBody;
        Header hdr = clientData.hdr;

        bbHeader.flip();
        bbBody.flip();
        
        try {
            hdr.extractFromByteArray(bbHeader);
            setConnectionOption.extractFromByteBuffer(bbBody);
    
            if (setConnectionOption.getDialogueId() < 1 ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId :" + setConnectionOption.getDialogueId());
            }
            if (setConnectionOption.getDialogueId() != clientData.getDialogueId() ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId sent by the Client [sent/expected] : [" + setConnectionOption.getDialogueId() + "/" + clientData.getDialogueId() + "]");
            }
//
//=====================Process SqlSetConnectAttr===========================
//
            exception = 0;
            exception_detail = 0;
            trafConnection = clientData.getTrafConnection();
            attr = setConnectionOption.getConnectionOption();
            option = setConnectionOption.getOptionValueNum();
            
            switch(attr){
            case ServerConstants.SQL_ATTR_ROWSET_RECOVERY:
                if(LOG.isDebugEnabled())
                    LOG.debug(serverWorkerName + ". Connection Attr: SQL_ATTR_ROWSET_RECOVERY [" + option + "]" );
                break;
            case ServerConstants.SQL_ATTR_ACCESS_MODE:
                if(LOG.isDebugEnabled())
                    LOG.debug(serverWorkerName + ". Connection Attr: SQL_ATTR_ACCESS_MODE [" + option + "]" );
               break;
            case ServerConstants.SQL_ATTR_AUTOCOMMIT:
                if(LOG.isDebugEnabled())
                    LOG.debug(serverWorkerName + ". Connection Attr: SQL_ATTR_AUTOCOMMIT [" + option + "]" );
                trafConnection.setAutoCommit(option);
                break;
            case ServerConstants.SQL_TXN_ISOLATION:
                if(LOG.isDebugEnabled())
                    LOG.debug(serverWorkerName + ". Connection Attr: SQL_TXN_ISOLATION [" + option + "]" );
                break;
            default:
                if(LOG.isDebugEnabled())
                    LOG.debug(serverWorkerName + ". Unknown Connection Attr: [" + attr + "/" + option + "]" );
            }
            errorDescList = new ErrorDescList(1);
            errorDescList.getBuffer()[0].setRowId(1);
            errorDescList.getBuffer()[0].setErrorDiagnosticId(2);
            errorDescList.getBuffer()[0].setSqlcode(3);
            errorDescList.getBuffer()[0].setSqlstate("state");
            errorDescList.getBuffer()[0].setErrorText("Text");
            errorDescList.getBuffer()[0].setOperationAbortId(4);
            errorDescList.getBuffer()[0].setErrorCodeType(5);
            errorDescList.getBuffer()[0].setParam1("A");
            errorDescList.getBuffer()[0].setParam2("B");
            errorDescList.getBuffer()[0].setParam3("C");
            errorDescList.getBuffer()[0].setParam4("D");
            errorDescList.getBuffer()[0].setParam5("E");
            errorDescList.getBuffer()[0].setParam6("F");
            errorDescList.getBuffer()[0].setParam7("G");
            
            serverException.setServerException (exception, exception_detail, errorDescList);
//
//===================calculate length of output ByteBuffer========================
//
//  hdr + serverException
//
            bbHeader.clear();
            bbBody.clear();
//
// check if ByteBuffer is big enough for serverException
//      
            int dataLength = serverException.lengthOfData();
            int availableBuffer = bbBody.capacity() - bbBody.position();
             if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". dataLength :" + dataLength + " availableBuffer :" + availableBuffer);
            if (dataLength > availableBuffer )
                bbBody = ByteBufferUtils.increaseCapacity(bbBody, dataLength > ServerConstants.BODY_SIZE ? dataLength : ServerConstants.BODY_SIZE );
//===================== build output ==============================================
            serverException.insertIntoByteBuffer(bbBody);

            bbBody.flip();
//=========================Update header================================ 
            hdr.setTotalLength(bbBody.limit());
            hdr.insertIntoByteBuffer(bbHeader);
            bbHeader.flip();

            clientData.setByteBufferArray(bbHeader, bbBody);
            clientData.setHdr(hdr);
            clientData.setRequest(ServerConstants.REQUST_WRITE_READ);
            
        } catch (SQLException se){
            LOG.error(serverWorkerName + ". SetConnectAttr.SQLException :" + se);
            clientData.setRequestAndDisconnect();
        } catch (UnsupportedEncodingException ue){
            LOG.error(serverWorkerName + ". SetConnectAttr.UnsupportedEncodingException :" + ue);
            clientData.setRequestAndDisconnect();
        } catch (Exception e){
            LOG.error(serverWorkerName + ". SetConnectAttr.Exception :" + e);
            clientData.setRequestAndDisconnect();
        }
        reset();
        return clientData;
    }
}
