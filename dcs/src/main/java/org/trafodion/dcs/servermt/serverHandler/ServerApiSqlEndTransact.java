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

public class ServerApiSqlEndTransact {
    private static final int odbc_SQLSvc_EndTransaction_ParamError_exn_ = 1;
    private static final int odbc_SQLSvc_EndTransaction_InvalidConnection_exn_ = 2;
    private static final int odbc_SQLSvc_EndTransaction_SQLError_exn_ = 3;
    private static final int odbc_SQLSvc_EndTransaction_SQLInvalidHandle_exn_ = 4;
    private static final int odbc_SQLSvc_EndTransaction_TransactionError_exn_ = 5;

    private static  final Log LOG = LogFactory.getLog(ServerApiSqlEndTransact.class);
    private int instance;
    private int serverThread;
    private String serverWorkerName;
    private ClientData clientData;

    private int dialogueId;
    private short transactionOpt;

    private ServerException serverException;

    ServerApiSqlEndTransact(int instance, int serverThread) {  
        this.instance = instance;
        this.serverThread = serverThread;
        serverWorkerName = ServerConstants.SERVER_WORKER_NAME + "_" + instance + "_" + serverThread;
     }
    void init(){
        dialogueId = 0;
        transactionOpt = 0;
        serverException = null;
    }
    void reset(){
        dialogueId = 0;
        transactionOpt = 0;
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
            transactionOpt = bbBody.getShort();
            if(LOG.isDebugEnabled()){
                LOG.debug(serverWorkerName + ". dialogueId :" + dialogueId);
                LOG.debug(serverWorkerName + ". transactionOpt :" + transactionOpt);
            }
            if (dialogueId < 1 ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId :" + dialogueId);
            }
            if (dialogueId != clientData.getDialogueId() ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId sent by the Client [sent/expected] : [" + dialogueId + "/" + clientData.getDialogueId() + "]");
            }
//=====================Process ServerApiEndTransaction==============
//
            serverException = new ServerException();
            try {
              if (transactionOpt == 1)
                  clientData.getTrafConnection().rollback();
              else
                  clientData.getTrafConnection().commit();
            } catch (SQLException ex){
                LOG.error(serverWorkerName + ". SQLException :" + ex);
                serverException.setServerException (odbc_SQLSvc_EndTransaction_TransactionError_exn_, 0, ex);
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
            int availableBuffer = 0;
            
            dataLength = serverException.lengthOfData();
            availableBuffer = bbBody.capacity() - bbBody.position();
            
            // If there is no Exception, serverException.lengthOfData() will return 8;
            // The driver still need read a int will indicate the number of the exception,
            // which should be 0;
            // So here, we add extra 4 bytes for it.
            if (dataLength == 2 * ServerConstants.INT_FIELD_SIZE) {
                dataLength = dataLength + ServerConstants.INT_FIELD_SIZE;
            }
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". dataLength :" + dataLength + " availableBuffer :" + availableBuffer);
        
            if (dataLength > availableBuffer ) {
                bbBody = ByteBufferUtils.increaseCapacity(bbBody, dataLength > ServerConstants.BODY_SIZE ? dataLength : ServerConstants.BODY_SIZE );
                ByteBufferUtils.printBBInfo(bbBody);
                clientData.bbBuf[1] = bbBody;
            }
//===================== build output ==============================================
            serverException.insertIntoByteBuffer(bbBody);
            
            // if there is no exception, we add a extra 4 bytes which indicate the number of
            // the exception which should be 0;
            if (serverException.lengthOfData() == 2 * ServerConstants.INT_FIELD_SIZE) {
                bbBody.putInt(0);
            }
            bbBody.flip();
//=========================Update header================================ 
            hdr.setTotalLength(bbBody.limit());
            hdr.insertIntoByteBuffer(bbHeader);
            bbHeader.flip();

            clientData.setByteBufferArray(bbHeader, bbBody);
            clientData.setHdr(hdr);
            clientData.setRequest(ServerConstants.REQUST_WRITE_READ);
            
        } catch (SQLException se){
            LOG.error(serverWorkerName + ". SQLException :" + se);
            clientData.setRequestAndDisconnect();
        } catch (UnsupportedEncodingException ue){
            LOG.error(serverWorkerName + ". UnsupportedEncodingException :" + ue);
            clientData.setRequestAndDisconnect();
        } catch (Exception e){
            LOG.error(serverWorkerName + ". Exception :" + e);
            clientData.setRequestAndDisconnect();
        }
        reset();
        return clientData;
    }
}
