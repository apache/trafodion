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

public class ServerApiSqlClose {
    
    private static final int odbc_SQLSvc_Close_ParamError_exn_ = 1;
    private static final int odbc_SQLSvc_Close_InvalidConnection_exn_ = 2;
    private static final int odbc_SQLSvc_Close_SQLError_exn_ = 3;
    private static final int odbc_SQLSvc_Close_TransactionError_exn_ = 4;
    
    private static  final Log LOG = LogFactory.getLog(ServerApiSqlClose.class);
    private int instance;
    private int serverThread;
    private String serverWorkerName;
    private ClientData clientData;
    private TrafConnection trafConn;

    private int dialogueId;
    private String stmtLabel;
    private short freeResourceOpt;
    
    private int returnCode;
    private int rowsAffected;
    private SQLWarningOrErrorList errorList;
    
    private ServerException serverException;
    
    ServerApiSqlClose(int instance, int serverThread) {  
        this.instance = instance;
        this.serverThread = serverThread;
        serverWorkerName = ServerConstants.SERVER_WORKER_NAME + "_" + instance + "_" + serverThread;
     }
    void init(){
        reset();
        serverException = new ServerException();
    }
    void reset(){
        dialogueId = 0;
        stmtLabel = "";
        freeResourceOpt = 0;
        
        returnCode = ServerConstants.SQL_SUCCESS;
        rowsAffected = 0;
        errorList = null;
        
        trafConn = null;
        serverException = null;
    }
    ClientData processApi(ClientData clientData) {  
        this.clientData = clientData;
        init();
//        
// ==============process input ByteBuffer===========================
// 
        ByteBuffer bbHeader = clientData.bbHeader;
        ByteBuffer bbBody = clientData.bbBody;
        Header hdr = clientData.hdr;

        bbHeader.flip();
        bbBody.flip();
        
        try {

            hdr.extractFromByteArray(bbHeader);
            
            dialogueId =  bbBody.getInt();
            stmtLabel = ByteBufferUtils.extractString(bbBody);
            freeResourceOpt = bbBody.getShort();
            
            if(LOG.isDebugEnabled()){
                LOG.debug(serverWorkerName + ". dialogueId :" + dialogueId);
                LOG.debug(serverWorkerName + ". stmtLabel :" + stmtLabel);
                LOG.debug(serverWorkerName + ". freeResourceOpt :" + freeResourceOpt + " :" + ServerUtils.convertFreeResourceOptToString(freeResourceOpt));
            }
            if (dialogueId < 1 ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId :" + dialogueId);
            }
            if (dialogueId != clientData.getDialogueId() ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId sent by the Client [sent/expected] : [" + dialogueId + "/" + clientData.getDialogueId() + "]");
            }
//=====================Process ServerApiSqlClose===========================
//
          try {
              trafConn = clientData.getTrafConnection();
              if (trafConn != null){
                  switch(freeResourceOpt){
                      case ServerConstants.SQL_DROP:
                          trafConn.closeTrafStatement(stmtLabel);
                          break;
                      default:
                          if(LOG.isDebugEnabled())
                              LOG.debug(serverWorkerName + ". Unknown freeResourceOpt :[" + freeResourceOpt + "]");
                          break;
                  }
              }
           } catch (SQLException ex){
               if(LOG.isDebugEnabled())
                   LOG.debug(serverWorkerName + ". SQLException :" + ex);
               serverException.setServerException (odbc_SQLSvc_Close_SQLError_exn_, 0, ex);                
            }
//            
//===================calculate length of output ByteBuffer========================
//
            bbHeader.clear();
            bbBody.clear();
//
// check if ByteBuffer is big enough for output
//      
            int dataLength = 0;
            dataLength += ServerConstants.INT_FIELD_SIZE;             //returnCode
            if (returnCode != ServerConstants.SQL_SUCCESS && returnCode != ServerConstants.SQL_NO_DATA_FOUND){
                if (errorList != null)
                    dataLength += errorList.lengthOfData();
                else
                    dataLength += ServerConstants.INT_FIELD_SIZE;             //totalErrorLength = 0
            }
            if (returnCode == ServerConstants.SQL_SUCCESS || returnCode == ServerConstants.SQL_SUCCESS_WITH_INFO)
                dataLength += ServerConstants.INT_FIELD_SIZE;                               //rowsAffected
//
            int availableBuffer = bbBody.capacity() - bbBody.position();
            if (errorList != null)
                dataLength += errorList.lengthOfData();
            else
                dataLength += ServerConstants.INT_FIELD_SIZE;             //totalErrorLength = 0
            
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". dataLength :" + dataLength + " availableBuffer :" + availableBuffer);
        
            if (dataLength > availableBuffer ) {
                bbBody = ByteBufferUtils.increaseCapacity(bbBody, dataLength > ServerConstants.BODY_SIZE ? dataLength : ServerConstants.BODY_SIZE );
                ByteBufferUtils.printBBInfo(bbBody);
                clientData.bbBuf[1] = bbBody;
            }
//===================== build output ==============================================
            bbBody.putInt(returnCode);
            if (returnCode != ServerConstants.SQL_SUCCESS && returnCode != ServerConstants.SQL_NO_DATA_FOUND){
                if (errorList != null)
                    errorList.insertIntoByteBuffer(bbBody);
                else
                    bbBody.putInt(0);
            }
            if (returnCode == ServerConstants.SQL_SUCCESS || returnCode == ServerConstants.SQL_SUCCESS_WITH_INFO)
                bbBody.putInt(rowsAffected);
            
            bbBody.flip();
//=========================Update header================================ 
            hdr.setTotalLength(bbBody.limit());
            hdr.insertIntoByteBuffer(bbHeader);
            bbHeader.flip();

            clientData.setByteBufferArray(bbHeader, bbBody);
            clientData.setHdr(hdr);
            clientData.setRequest(ServerConstants.REQUST_WRITE_READ);
            
        } catch (SQLException se){
            LOG.error(serverWorkerName + ". Close.SQLException :" + se);
            clientData.setRequestAndDisconnect();
        } catch (UnsupportedEncodingException ue){
            LOG.error(serverWorkerName + ". Close.UnsupportedEncodingException :" + ue);
            clientData.setRequestAndDisconnect();
        } catch (Exception e){
            LOG.error(serverWorkerName + ". Close.Exception :" + e);
            clientData.setRequestAndDisconnect();
        }
        reset();
        return clientData;
    }
}