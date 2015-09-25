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

import java.lang.reflect.*;
import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.sql.*;
import java.util.*;

import org.trafodion.jdbc.t2.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverDriverInputOutput.*;
import org.trafodion.dcs.servermt.serverSql.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class ServerApiSqlExecDirect {
    private static  final Log LOG = LogFactory.getLog(ServerApiSqlExecDirect.class);
    private int instance;
    private int serverThread;
    private String serverWorkerName;
    private ClientData clientData;
    //
    private int dialogueId;
    private int sqlAsyncEnable;
    private int queryTimeout;
    private int inpRowCnt;
    private int maxRowsetSize;
    private int sqlStmtType;
    private int stmtHandle;
    private int stmtType;
    private String sqlString;
    private String cursorName;
    private String stmtLabel;
    private String stmtExplainLabel;
    private SQLDataValue inpDataValue;
    private SQLValueList inpValueList;
    private byte[] txId;
    
    private int holdableCursor;
//=======================================================================    
    private Statement stmt;
    private TrafConnection trafConn;
    private TrafStatement trafStmt;
//-------------------output-------------------------------------    
    private int returnCode;
    private SQLWarningOrErrorList errorList;
    private long rowsAffected;
    private int sqlQueryType;
    private int estimatedCost;
    private byte[] outValues;

//----------- tmp for result set --------------------
    private int numResultSets;
    private int resultSetColumns;
    private Descriptor2List resultSetDescList;
    private ResultSet rs;
    private ResultSetMetaData rsmd;
    private TResultSetMetaData trsmd;
    private SQLMXResultSetMetaData strsmd;
    private String[] stmtLabels;
    private String[] proxySyntax;
    private String singleSyntax;
//-------------T2 desc fields-------------------
    private int        sqlCharset_;
    private int        odbcCharset_;
    private int        sqlDataType_;
    private int        dataType_;
    private short    sqlPrecision_;
    private short    sqlDatetimeCode_;
    private int        sqlOctetLength_;
    private int        isNullable_;
    private String    name_;
    private int        scale_;
    private int        precision_;
    private boolean    isSigned_;
    private boolean    isCurrency_;
    private boolean    isCaseSensitive_;
    private String     catalogName_;
    private String    schemaName_;
    private String    tableName_;
    private int        fsDataType_;
    private int        intLeadPrec_;
    private int        paramMode_;
    private int        paramIndex_;
    private int        paramPos_;

    private int        odbcPrecision_;
    private int        maxLen_;

    private int     displaySize_;
    private String  label_;
    
    ServerApiSqlExecDirect(int instance, int serverThread) {  
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
        inpRowCnt = 0;
        maxRowsetSize = 0;
        sqlStmtType = 0;
        stmtHandle = 0;
        stmtType = 0;
        sqlString = null;
        cursorName = null;
        stmtLabel = null;
        stmtExplainLabel = null;
        txId = null;     // JDBC is the only one that will use this to join a transaction

        holdableCursor = 0; //default
//==================================================================
        trafConn = null;
        trafStmt = null;
//------------------------------output------------------------------------
        returnCode = 0;
        errorList = null;
        rowsAffected = 0;
        sqlQueryType = 0;
        estimatedCost = 0;
        outValues = null;
        numResultSets = 0;
        resultSetColumns = 0;
        resultSetDescList = null;
        rs = null;
        rsmd = null;
        trsmd = null;
        strsmd = null;
        stmtLabels = null;
        singleSyntax = "";
        proxySyntax = null;
    }
    ClientData processApi(ClientData clientData) {  
        init();
        this.clientData = clientData;
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
            holdableCursor =  bbBody.getInt();
            queryTimeout =  bbBody.getInt();
            inpRowCnt =  bbBody.getInt();
            maxRowsetSize =  bbBody.getInt();
            sqlStmtType =  bbBody.getInt();
            stmtHandle =  bbBody.getInt();
            stmtType =  bbBody.getInt();
            sqlString = ByteBufferUtils.extractStringWithCharset(bbBody);
            cursorName = ByteBufferUtils.extractStringWithCharset(bbBody);
            stmtLabel = ByteBufferUtils.extractStringWithCharset(bbBody);
            stmtExplainLabel = ByteBufferUtils.extractString(bbBody);
//            txId = ByteBufferUtils.extractByteArray(bbBody);
            if(LOG.isDebugEnabled()){
                LOG.debug(serverWorkerName + ". dialogueId :" + dialogueId);
                LOG.debug(serverWorkerName + ". holdableCursor :" + holdableCursor);
                LOG.debug(serverWorkerName + ". queryTimeout :" + queryTimeout);
                LOG.debug(serverWorkerName + ". inpRowCnt :" + inpRowCnt);
                LOG.debug(serverWorkerName + ". maxRowsetSize :" + maxRowsetSize);
                LOG.debug(serverWorkerName + ". sqlStmtType :" + sqlStmtType);
                LOG.debug(serverWorkerName + ". stmtHandle :" + stmtHandle);
                LOG.debug(serverWorkerName + ". stmtType :" + stmtType);
                LOG.debug(serverWorkerName + ". sqlString :" + sqlString);
                LOG.debug(serverWorkerName + ". cursorName :" + cursorName);
                LOG.debug(serverWorkerName + ". stmtLabel :" + stmtLabel);
                LOG.debug(serverWorkerName + ". stmtExplainLabel :" + stmtExplainLabel);
                LOG.debug(serverWorkerName + ". txId :" + txId);
            }
            if (dialogueId < 1 ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId :" + dialogueId);
            }
            if (dialogueId != clientData.getDialogueId() ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId sent by the Client [sent/expected] : [" + dialogueId + "/" + clientData.getDialogueId() + "]");
            }
//=============================================================================
            boolean isResultSet = false;
            sqlQueryType = sqlStmtType;
            
            switch (sqlStmtType){
                case ServerConstants.TYPE_SELECT:
                case ServerConstants.TYPE_EXPLAIN:
                    isResultSet = true;
                    break;
                case ServerConstants.TYPE_UPDATE:
                case ServerConstants.TYPE_DELETE:
                case ServerConstants.TYPE_INSERT:
                case ServerConstants.TYPE_INSERT_PARAM:
                case ServerConstants.TYPE_CREATE:
                case ServerConstants.TYPE_GRANT:
                case ServerConstants.TYPE_DROP:
                case ServerConstants.TYPE_CALL:
                case ServerConstants.TYPE_CONTROL:
                default:
            }
            try {
                trafConn = clientData.getTrafConnection();
                trafStmt = trafConn.createTrafStatement(stmtLabel, isResultSet);
                trafStmt.setResultSet(null);
                stmt = (Statement)trafStmt.getStatement();
//            
//=====================Process ServerApiSqlExecute===========================
//
                boolean status = stmt.execute(sqlString);
                if(LOG.isDebugEnabled())
                     LOG.debug(serverWorkerName + ". T2 Execute.execute(sqlString) status: " + status);
                if(status){
                    rs = stmt.getResultSet();
                    rsmd = rs.getMetaData();
                    trafStmt.setResultSet(rs);
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverWorkerName + ". T2 Execute.getResultSet()");
                    resultSetColumns = rsmd.getColumnCount();
                } else {
                    rowsAffected = stmt.getUpdateCount();
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverWorkerName + ". T2 Execute.getUpdateCount() rowsAffected :" + rowsAffected);
                }
            } catch (SQLException se){
                LOG.error(serverWorkerName + ". ExecDirect.SQLException " + se);
                errorList = new SQLWarningOrErrorList(se); 
                returnCode = errorList.getReturnCode();
            } catch (Exception ex){
                LOG.error(serverWorkerName + ". ExecDirect.Exception " + ex);
                throw ex;
            }
            if (returnCode == ServerConstants.SQL_SUCCESS || returnCode == ServerConstants.SQL_SUCCESS_WITH_INFO) {
                if (resultSetColumns > 0){
                    strsmd = (SQLMXResultSetMetaData)rsmd;
                    resultSetDescList = new Descriptor2List(resultSetColumns,false);
                    
                    numResultSets = 1;
                    stmtLabels = new String[numResultSets];
                    proxySyntax = new String[numResultSets];
                    singleSyntax = "";
                    stmtLabels[0] = stmtLabel;
                    proxySyntax[0] = "";
                    
                    for (int column = 1; column <= resultSetColumns; column++){
                        sqlCharset_ = strsmd.getSqlCharset(column);
                        odbcCharset_ = strsmd.getOdbcCharset(column);
                        sqlDataType_ = strsmd.getSqlDataType(column);
                        dataType_ = strsmd.getDataType(column);
                        sqlPrecision_ = strsmd.getSqlPrecision(column);
                        sqlDatetimeCode_ = strsmd.getSqlDatetimeCode(column);
                        sqlOctetLength_ = strsmd.getSqlOctetLength(column);
                        isNullable_ = strsmd.getIsNullable(column);
                        name_ = strsmd.getName(column);
                        scale_ = strsmd.getScale(column);
                        precision_ = strsmd.getPrecision(column);
                        isSigned_ = strsmd.getIsSigned(column);
                        isCurrency_ = strsmd.getIsCurrency(column);
                        isCaseSensitive_ = strsmd.getIsCaseSensitive(column);
                        catalogName_ = strsmd.getCatalogName(column);
                        schemaName_ = strsmd.getSchemaName(column);
                        tableName_ = strsmd.getTableName(column);
                        fsDataType_ = strsmd.getFsDataType(column);
                        intLeadPrec_ = strsmd.getIntLeadPrec(column);
                        paramMode_ = strsmd.getMode(column);
                        paramIndex_ = strsmd.getIndex(column);
                        paramPos_ = strsmd.getPos(column);
                        
                        odbcPrecision_ = strsmd.getOdbcPrecision(column);
                        maxLen_ = strsmd.getMaxLen(column);
                        
                        displaySize_ = strsmd.getDisplaySize(column);
                        label_ = strsmd.getLabel(column);
    
                        Descriptor2 outDesc = new Descriptor2(sqlCharset_,odbcCharset_,sqlDataType_,dataType_,sqlPrecision_,sqlDatetimeCode_,
                            sqlOctetLength_,isNullable_,name_,scale_,precision_,isSigned_,
                            isCurrency_,isCaseSensitive_,catalogName_,schemaName_,tableName_,
                            fsDataType_,intLeadPrec_,paramMode_,paramIndex_,paramPos_,odbcPrecision_,
                            maxLen_,displaySize_,label_, false);
                        resultSetDescList.addDescriptor(column,outDesc);
                    }
                    if(LOG.isDebugEnabled()){
                        for (int column = 1; column <= resultSetColumns; column++){
                            Descriptor2 dsc = resultSetDescList.getDescriptors2()[column-1];
                            LOG.debug(serverWorkerName + ". [" + column + "] Output descriptor -------------" );
                            LOG.debug(serverWorkerName + ". oldFormat " + column + " :" + dsc.getOldFormat());
                            LOG.debug(serverWorkerName + ". noNullValue " + column + " :" + dsc.getNoNullValue());
                            LOG.debug(serverWorkerName + ". nullValue " + column + " :" + dsc.getNullValue());
                            LOG.debug(serverWorkerName + ". version " + column + " :" + dsc.getVersion());
                            LOG.debug(serverWorkerName + ". dataType " + column + " :" + SqlUtils.getDataType(dsc.getDataType()));
                            LOG.debug(serverWorkerName + ". datetimeCode " + column + " :" + dsc.getDatetimeCode());
                            LOG.debug(serverWorkerName + ". maxLen " + column + " :" + dsc.getMaxLen());
                            LOG.debug(serverWorkerName + ". precision " + column + " :" + dsc.getPrecision());
                            LOG.debug(serverWorkerName + ". scale " + column + " :" + dsc.getScale());
                            LOG.debug(serverWorkerName + ". nullInfo " + column + " :" + dsc.getNullInfo());
                            LOG.debug(serverWorkerName + ". signed " + column + " :" + dsc.getSigned());
                            LOG.debug(serverWorkerName + ". odbcDataType " + column + " :" + dsc.getOdbcDataType());
                            LOG.debug(serverWorkerName + ". odbcPrecision " + column + " :" + dsc.getOdbcPrecision());
                            LOG.debug(serverWorkerName + ". sqlCharset " + column + " :" + SqlUtils.getCharsetName(dsc.getSqlCharset()) + "[" + dsc.getSqlCharset() + "]");
                            LOG.debug(serverWorkerName + ". odbcCharset " + column + " :" + SqlUtils.getCharsetName(dsc.getOdbcCharset()) + "[" + dsc.getOdbcCharset() + "]");
                            LOG.debug(serverWorkerName + ". colHeadingNm " + column + " :" + dsc.getColHeadingNm());
                            LOG.debug(serverWorkerName + ". tableName " + column + " :" + dsc.getTableName());
                            LOG.debug(serverWorkerName + ". schemaName " + column + " :" + dsc.getSchemaName());
                            LOG.debug(serverWorkerName + ". headingName " + column + " :" + dsc.getHeadingName());
                            LOG.debug(serverWorkerName + ". intLeadPrec " + column + " :" + dsc.getParamMode());
                            LOG.debug(serverWorkerName + ". paramMode " + column + " :" + dsc.getColHeadingNm());
                            LOG.debug(serverWorkerName + ". memAlignOffset " + column + " :" + dsc.getMemAlignOffset());
                            LOG.debug(serverWorkerName + ". allocSize " + column + " :" + dsc.getAllocSize());
                            LOG.debug(serverWorkerName + ". varLayout " + column + " :" + dsc.getVarLayout());
                            LOG.debug(serverWorkerName + ". Output descriptor End-------------");
                        }
                    }
                    trafStmt.setOutNumberParams(resultSetColumns);
                    if (resultSetColumns > 0){
                        trafStmt.setOutParamLength(resultSetDescList.getParamLength());
                        trafStmt.setOutDescList(resultSetDescList);
                    }
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
            int dataLength = ServerConstants.INT_FIELD_SIZE;                 //returnCode
            if (errorList != null)
                dataLength += errorList.lengthOfData();
            else
                dataLength += ServerConstants.INT_FIELD_SIZE;             //totalErrorLength = 0

            dataLength += ServerConstants.INT_FIELD_SIZE;             //outDescLength = 0

            dataLength += ServerConstants.INT_FIELD_SIZE;                 //rowsAffected
            dataLength += ServerConstants.INT_FIELD_SIZE;                 //queryType
            dataLength += ServerConstants.INT_FIELD_SIZE;                 //estimatedCost
            dataLength += ByteBufferUtils.lengthOfByteArray(outValues); //outValues
            dataLength += ServerConstants.INT_FIELD_SIZE;                 //numResultSets
            if (numResultSets > 0) {
                for (int i = 0; i < numResultSets; i++) {
                    dataLength += ServerConstants.INT_FIELD_SIZE;         //stmt_handle
                    dataLength += ByteBufferUtils.lengthOfString(stmtLabels[i]); //stmtLabels
                    dataLength += ServerConstants.INT_FIELD_SIZE;         //stmt_label_charset
                    
                    if (resultSetDescList != null)
                        dataLength += resultSetDescList.lengthOfData();
                    else
                        dataLength += ServerConstants.INT_FIELD_SIZE;
                
                    dataLength += ByteBufferUtils.lengthOfString(proxySyntax[i]); //proxySyntax[i]
                }
            }
            dataLength += ByteBufferUtils.lengthOfString(singleSyntax);
            
            int availableBuffer = bbBody.capacity() - bbBody.position();
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". dataLength :" + dataLength + " availableBuffer :" + availableBuffer);
            if (dataLength > availableBuffer )
                bbBody = ByteBufferUtils.increaseCapacity(bbBody, dataLength > ServerConstants.BODY_SIZE ? dataLength : ServerConstants.BODY_SIZE );

//===================== build output ==============================================
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". returnCode :" + returnCode);
            bbBody.putInt(returnCode);
            if (errorList != null)
                errorList.insertIntoByteBuffer(bbBody);
            else
                bbBody.putInt(0);
            
            bbBody.putInt(0);             //outDescLength = 0
            
            ByteBufferUtils.insertUInt(rowsAffected, bbBody);
            bbBody.putInt(sqlQueryType);
            bbBody.putInt(estimatedCost);
            ByteBufferUtils.insertByteArray(outValues, bbBody);     //outValues
            bbBody.putInt(numResultSets);

            if (numResultSets > 0) {
                for (int i = 0; i < numResultSets; i++) {
                    bbBody.putInt(0); // int stmt_handle - ignored
                    ByteBufferUtils.insertString(stmtLabels[i], bbBody);
                    bbBody.putInt(0); // long stmt_label_charset - ignored
                    
                    if (resultSetDescList != null)
                         resultSetDescList.insertIntoByteBuffer(bbBody);
                    else
                        bbBody.putInt(0);
                    
                    ByteBufferUtils.insertString(proxySyntax[i], bbBody);
                }
            }
            ByteBufferUtils.insertString(singleSyntax, bbBody);

            bbBody.flip();
//=========================Update header================================ 
            hdr.setTotalLength(bbBody.limit());
            hdr.insertIntoByteBuffer(bbHeader);
            bbHeader.flip();

            clientData.setByteBufferArray(bbHeader, bbBody);
            clientData.setHdr(hdr);
            clientData.setRequest(ServerConstants.REQUST_WRITE_READ);

        } catch (UnsupportedEncodingException ue){
            LOG.error(serverWorkerName + ". ExecDirect.UnsupportedEncodingException :" + ue);
            clientData.setRequestAndDisconnect();
        } catch (Exception e){
            LOG.error(serverWorkerName + ". ExecDirect.Exception :" + e);
            clientData.setRequestAndDisconnect();
        }
        reset();
        return clientData;
    }
}




