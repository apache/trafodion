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
import java.sql.SQLException;
import java.util.*;
import java.lang.reflect.Array;
import java.math.*;

import org.apache.trafodion.jdbc.t2.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverDriverInputOutput.*;
import org.trafodion.dcs.servermt.serverSql.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class ServerApiSqlFetch {
    private static final int odbc_SQLSvc_Fetch_ParamError_exn_ = 1;
    private static final int odbc_SQLSvc_Fetch_InvalidConnection_exn_ = 2;
    private static final int odbc_SQLSvc_Fetch_SQLError_exn_ = 3;
    private static final int odbc_SQLSvc_Fetch_SQLInvalidHandle_exn_ = 4;
    private static final int odbc_SQLSvc_Fetch_SQLNoDataFound_exn_ = 5;
    private static final int odbc_SQLSvc_Fetch_SQLStillExecuting_exn_ = 6;
    private static final int odbc_SQLSvc_Fetch_SQLQueryCancelled_exn_ = 7;
    private static final int odbc_SQLSvc_Fetch_TransactionError_exn_ = 8;

    private static final int dateLength = 10;
    private static final int timeLength = 8;
    private static final int timestampLength = 26;

    private static  final Log LOG = LogFactory.getLog(ServerApiSqlFetch.class);
    private int instance;
    private int serverThread;
    private String serverWorkerName;
    private ClientData clientData;
//-----------------------------------------------
    private Statement stmt;
    private PreparedStatement pstmt;
    private ResultSet rs;
    private ResultSetMetaData rsMD;
    private SQLMXResultSet sqlrs;
    private SQLMXResultSetMetaData strsmd;
    private TrafConnection trafConn;
    private TrafStatement trafStmt;
    private TrafResultSet trafResultSet;
    private Descriptor2List columnDescList;
//----------------------------------------------
    private String stmtLabelBytes;
    private String cursorNameBytes;
    private String stmtOptionsBytes;

    private int dialogueId;
    private int sqlAsyncEnable;
    private int queryTimeout;
    private int stmtHandle;
    private String stmtLabel;
    private long maxRowCnt;
    private long maxRowLen;
    private String cursorName;
    private String stmtOptions;
//===========================================
    private int returnCode;
    private SQLWarningOrErrorList errorList;
    private int rowsAffected;
    private int outValuesFormat;
    private byte[] outValues;
//===========================================
    private byte[] sqlarray;
    private long totalOutLen;
    private long curOutPos;
    private long curOutNullPos;
    private long rowOffset;
    private int curRowNumber;
    private int columnCount;
    private int columnLength;

    ServerApiSqlFetch(int instance, int serverThread) {
        this.instance = instance;
        this.serverThread = serverThread;
        serverWorkerName = ServerConstants.SERVER_WORKER_NAME + "_" + instance + "_" + serverThread;
    }
    void init(){
        reset();
    }
    void reset(){
        dialogueId = 0;
        sqlAsyncEnable = 0;
        queryTimeout = 0;
        stmtHandle = 0;
        stmtLabel = "";
        maxRowCnt = 0;
        maxRowLen = 0;
        cursorName = "";
        stmtOptions = "";

        returnCode = ServerConstants.SQL_SUCCESS;
        errorList = null;
        rowsAffected = 0;
        outValuesFormat = 0;
        outValues = null;
        stmtLabelBytes = "";
        cursorNameBytes = "";
        stmtOptionsBytes = "";

        strsmd = null;
        sqlrs = null;
        sqlarray = null;
        totalOutLen = 0;
        curOutPos = 0;
        curOutNullPos = 0;
        curRowNumber = 0;
        columnCount = 0;
        columnDescList = null;
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
            sqlAsyncEnable =  bbBody.getInt();
            queryTimeout =  bbBody.getInt();
            stmtHandle =  bbBody.getInt();
            stmtLabel = ByteBufferUtils.extractStringWithCharset(bbBody);
            maxRowCnt =  bbBody.getLong();
            maxRowLen =  bbBody.getLong();
            cursorName = ByteBufferUtils.extractStringWithCharset(bbBody);
            stmtOptions = ByteBufferUtils.extractString(bbBody);

            if(LOG.isDebugEnabled()){
                LOG.debug(serverWorkerName + ". dialogueId :" + dialogueId);
                LOG.debug(serverWorkerName + ". sqlAsyncEnable :" + sqlAsyncEnable);
                LOG.debug(serverWorkerName + ". queryTimeout :" + queryTimeout);
                LOG.debug(serverWorkerName + ". stmtHandle :" + stmtHandle);
                LOG.debug(serverWorkerName + ". stmtLabel :" + stmtLabel);
                LOG.debug(serverWorkerName + ". maxRowCnt :" + maxRowCnt);
                LOG.debug(serverWorkerName + ". maxRowLen :" + maxRowLen);
                LOG.debug(serverWorkerName + ". cursorName :" + cursorName);
                LOG.debug(serverWorkerName + ". stmtOptions :" + stmtOptions);
            }
            if (dialogueId < 1 ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId :" + dialogueId);
            }
            if (dialogueId != clientData.getDialogueId() ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId sent by the Client [sent/expected] : [" + dialogueId + "/" + clientData.getDialogueId() + "]");
            }
//=========================================================================
            trafConn = clientData.getTrafConnection();
            trafStmt = trafConn.getTrafStatement(stmtLabel,stmtHandle);
            trafResultSet = trafStmt.getTrafResultSet(); // get current result set

            rs = trafResultSet.getResultSet();
            if (trafResultSet.isClosed() == true){
                if(true == trafStmt.getNextTResultSet()){
                    trafResultSet = trafStmt.getTrafResultSet();
                    rs = trafResultSet.getResultSet();
                }
            }
            if (trafResultSet.isClosed() == false){

                columnDescList = trafResultSet.getColumnDescList();
                maxRowLen = columnDescList.getVarLength();
                columnCount = columnDescList.getDescCount();

                if(LOG.isDebugEnabled()){
                    LOG.debug(serverWorkerName + ". maxRowLen :" + maxRowLen);
                    LOG.debug(serverWorkerName + ". columnCount :" + columnCount);
                }
                returnCode = ServerConstants.SQL_SUCCESS;
                boolean closeResultSet = false;

//=====================Process ServerApiSqlFetch===========================
                if (columnDescList.getOldFormat() == false){
                    try {
                        while( rs != null){
                            Descriptor2[] desc = columnDescList.getDescriptors2();
                            Descriptor2 dsc;
                            int dataType = 0;
                            int dataMaxLen = 0;
                            sqlrs = (SQLMXResultSet)rs;
                            if (false == rs.next()){
                                closeResultSet = true;
                                if(LOG.isDebugEnabled())
                                    LOG.debug(serverWorkerName + ". T2 Fetch.rs.next() false");
                                break;
                            }
                            else{
                                if(LOG.isDebugEnabled())
                                    LOG.debug(serverWorkerName + ". T2 Fetch.rs.next() true");
                            }
                            if(totalOutLen == 0){
                                totalOutLen = maxRowLen * maxRowCnt;
                                outValues = new byte[(int)totalOutLen];
                            }
                            rowOffset = maxRowLen * curRowNumber;

                            if(LOG.isDebugEnabled()){
                                LOG.debug(serverWorkerName + ". curRowNumber :" + curRowNumber + "  maxRowCnt :" + maxRowCnt);
                                LOG.debug(serverWorkerName + ". totalOutLen :" + totalOutLen);
                                LOG.debug(serverWorkerName + ". rowOffset :" + rowOffset);
                            }

                            for(int column = 1; column <= columnCount; column++){
                                dsc = desc[column - 1];
                                int noNullValueOffset = dsc.getNoNullValue();
                                int nullValueOffset = dsc.getNullValue();

                                if (nullValueOffset != -1)
                                    nullValueOffset += rowOffset;
                                if (noNullValueOffset != -1)
                                    noNullValueOffset += rowOffset;

                                byte[] sqlarray = sqlrs.getSQLBytes(column);
                                if (sqlarray == null) {
                                    short value = -1;
                                    outValues[(int)nullValueOffset] = (byte) ((value >>> 8) & 0xff);
                                    outValues[(int)(nullValueOffset + 1)] = (byte) ((value) & 0xff);
                                }
                                else {
                                    outValues = SqlUtils.formatSqlT4Output(dsc, sqlarray, noNullValueOffset, outValues, bbBody.order());
                                }
                            }
                            curRowNumber++;
                            if (curRowNumber == maxRowCnt)
                                break;
                        }
                    } catch (SQLException ex){
                        LOG.error(serverWorkerName + ". Fetch.SQLException " + ex);
                        errorList = new SQLWarningOrErrorList(ex);
                        returnCode = errorList.getReturnCode();
                    }
                    rowsAffected = curRowNumber;
                    if (rowsAffected != maxRowCnt && rowsAffected != 0){
                        int len = (int)(rowsAffected * maxRowLen);
                        if(LOG.isDebugEnabled()){
                            LOG.debug(serverWorkerName + ". len :" + len);
                        }
                        byte[] dst = new byte[len];
                        System.arraycopy(outValues, 0, dst, 0, len);
                        if(LOG.isDebugEnabled()){
                            LOG.debug(serverWorkerName + ". dst :" + Arrays.toString(dst));
                        }
                        outValues = dst;
                    }
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverWorkerName + ". closeResultSet");
                    if (closeResultSet == true)
                        trafStmt.closeTResultSet();
                }
//---------------------------------------------------------------------------------------------
                else {
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverWorkerName + ". Processing old format result set");
                    int byteIndex = 0;

                    try {
                        while( rs != null){
                            Descriptor2[] desc = columnDescList.getDescriptors2();
                            Descriptor2 dsc;
                            int dataType = 0;
                            int dataMaxLen = 0;
                            sqlrs = (SQLMXResultSet)rs;
                            if (false == rs.next()){
                                closeResultSet = true;
                                if(LOG.isDebugEnabled())
                                    LOG.debug(serverWorkerName + ". T2 Fetch.rs.next() false");
                                break;
                            }
                            else{
                                if(LOG.isDebugEnabled())
                                    LOG.debug(serverWorkerName + ". T2 Fetch.rs.next() true");
                            }
                            if(totalOutLen == 0){
                                byteIndex = 0;
                                totalOutLen = maxRowLen * maxRowCnt;
                                outValues = new byte[(int)totalOutLen];
                            }
                            byte[] sqlarray = null;
                            short SQLDataInd = 0;

                            for(int column = 1; column <= columnCount; column++){
                                dsc = desc[column - 1];
                                sqlarray = sqlrs.getSQLBytes(column);
                                if (sqlarray != null)
                                    SQLDataInd = 0;
                                else
                                    SQLDataInd = -1;

                                outValues[byteIndex++] = (byte) (SQLDataInd);
                                if (SQLDataInd == 0){
                                    ByteBuffer tb = ByteBuffer.wrap(sqlarray).order(bbBody.order());
                                    int allocLength = sqlarray.length;

                                    switch (dsc.getDataType()) {
                                    case ServerConstants.SQLTYPECODE_VARCHAR_WITH_LENGTH:
                                    case ServerConstants.SQLTYPECODE_VARCHAR_LONG:
                                    case ServerConstants.SQLTYPECODE_BITVAR:
                                    case ServerConstants.SQLTYPECODE_VARCHAR:
                                        boolean shortLength = dsc.getPrecision() < Math.pow(2, 15);
                                        allocLength = (shortLength) ? tb.getShort() + 2 : tb.getInt() + 4;
                                        break;
                                    }
                                    System.arraycopy(sqlarray, 0, outValues, (int)byteIndex, allocLength);

                                    byteIndex = byteIndex + allocLength;

                                    switch (dsc.getDataType()) {
                                    case ServerConstants.SQLTYPECODE_VARCHAR_WITH_LENGTH:
                                    case ServerConstants.SQLTYPECODE_VARCHAR_LONG:
                                    case ServerConstants.SQLTYPECODE_BITVAR:
                                    case ServerConstants.SQLTYPECODE_VARCHAR:
                                        byteIndex++;
                                        break;
                                    }
                                }
                            }
                            curRowNumber++;
                            if (curRowNumber == maxRowCnt)
                                break;
                        }
                    } catch (SQLException ex){
                        LOG.error(serverWorkerName + ". Fetch.SQLException " + ex);
                        errorList = new SQLWarningOrErrorList(ex);
                        returnCode = errorList.getReturnCode();
                    }
                    rowsAffected = curRowNumber;
                    if (rowsAffected != maxRowCnt && rowsAffected != 0){
                        int len = byteIndex;
                        if(LOG.isDebugEnabled()){
                            LOG.debug(serverWorkerName + ". len :" + len);
                        }
                        byte[] dst = new byte[len];
                        System.arraycopy(outValues, 0, dst, 0, len);
                        outValues = dst;
                    }
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverWorkerName + ". closeResultSet before");
                    if (closeResultSet == true)
                        trafResultSet.closeTResultSet();
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverWorkerName + ". closeResultSet after");
                }
            }
//
//===================calculate length of output ByteBuffer========================
//
            bbHeader.clear();
            bbBody.clear();
//
// check if ByteBuffer is big enough for output
//
            if(rowsAffected == 0 && returnCode == ServerConstants.SQL_SUCCESS){
                returnCode = ServerConstants.SQL_NO_DATA_FOUND;
            }
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". returnCode :" + returnCode);

            int dataLength = ServerConstants.INT_FIELD_SIZE;                 //returnCode

            if (returnCode != ServerConstants.SQL_SUCCESS && returnCode != ServerConstants.SQL_NO_DATA_FOUND) {
                dataLength += ServerConstants.INT_FIELD_SIZE;                 //totalErrorLength
                if (errorList != null)
                    dataLength += errorList.lengthOfData();
                else
                    dataLength += ServerConstants.INT_FIELD_SIZE;         //totalErrorLength = 0
            }
            dataLength += ServerConstants.INT_FIELD_SIZE;                 //rowsAffected
            dataLength += ServerConstants.INT_FIELD_SIZE;                 //outValuesFormat
            if (returnCode == ServerConstants.SQL_SUCCESS || returnCode == ServerConstants.SQL_SUCCESS_WITH_INFO)
                dataLength += ByteBufferUtils.lengthOfByteArray(outValues);

            int availableBuffer = bbBody.capacity() - bbBody.position();
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". dataLength :" + dataLength + " availableBuffer :" + availableBuffer);
            if (dataLength > availableBuffer )
                bbBody = ByteBufferUtils.increaseCapacity(bbBody, dataLength > ServerConstants.BODY_SIZE ? dataLength : ServerConstants.BODY_SIZE );

//===================== build output ==============================================
            bbBody.putInt(returnCode);

            if (returnCode != ServerConstants.SQL_SUCCESS && returnCode != ServerConstants.SQL_NO_DATA_FOUND) {
                if (errorList != null)
                    errorList.insertIntoByteBuffer(bbBody);
                else
                    bbBody.putInt(0);
            }
            bbBody.putInt(rowsAffected);
            bbBody.putInt(ServerConstants.ROWWISE_ROWSETS);
            if (returnCode == ServerConstants.SQL_SUCCESS || returnCode == ServerConstants.SQL_SUCCESS_WITH_INFO)
                ByteBufferUtils.insertByteArray(outValues, bbBody);

            bbBody.flip();
//=========================Update header================================
            hdr.setTotalLength(bbBody.limit());
            hdr.insertIntoByteBuffer(bbHeader);
            bbHeader.flip();

            clientData.setByteBufferArray(bbHeader, bbBody);
            clientData.setHdr(hdr);
            clientData.setRequest(ServerConstants.REQUST_WRITE_READ);
        } catch (UnsupportedEncodingException ue){
            LOG.error(serverWorkerName + ". Fetch.UnsupportedEncodingException :" + ue);
            clientData.setRequestAndDisconnect();
        } catch (Exception e){
            LOG.error(serverWorkerName + ". Fetch.Exception :" + e);
            clientData.setRequestAndDisconnect();
        }
        if(LOG.isDebugEnabled())
            LOG.debug(serverWorkerName + ". Fetch exit: returnCode :" + returnCode + " rowsAffected :" + rowsAffected );
        reset();

        return clientData;
    }
}
