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
import java.math.BigDecimal;
import java.math.BigInteger;
import java.text.SimpleDateFormat;
import java.text.ParseException;
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

public class ServerApiSqlExecute {
    private static final int odbc_SQLSvc_Execute_ParamError_exn_ = 1;
    private static final int odbc_SQLSvc_Execute_InvalidConnection_exn_ = 2;
    private static final int odbc_SQLSvc_Execute_SQLError_exn_ = 3;
    private static final int odbc_SQLSvc_Execute_SQLInvalidHandle_exn_ = 4;
    private static final int odbc_SQLSvc_ExecuteSQLNeedData_exn_ = 5;
    private static final int odbc_SQLSvc_Execute_SQLRetryCompile_exn_ = 6;
    private static final int odbc_SQLSvc_Execute_SQLStillExecuting_exn_ = 7;
    private static final int odbc_SQLSvc_Execute_SQLQueryCancelled_exn_ = 8;
    private static final int odbc_SQLSvc_Execute_TransactionError_exn_ = 9;

    private static  final Log LOG = LogFactory.getLog(ServerApiSqlExecute.class);
    private int instance;
    private int serverThread;
    private String serverWorkerName;
    private ClientData clientData;
//
    private int dialogueId;
    private int holdableCursor;
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
    private byte[] txId;

//=======================================================================
    private boolean isResultSet;
    private boolean isSpj;
    private Object stmt;
    private PreparedStatement pstmt;
    private CallableStatement cstmt;
    private TrafConnection trafConn;
    private TrafStatement trafStmt;
//------------------Parameters----------------------------------
    private int regParamCount;
    private int paramCount;
    private long paramLength;
    private Descriptor2List paramDescList;
    private Descriptor2 paramDesc;
    private int paramValuesLength;
//------------------output Values-------------------------------
    private int outValuesLength;
    private byte[] outValues;
//--------------------------------------------------------------
    private Descriptor2List outDescList;
    private ResultSet rs;
    private ResultSetMetaData rsMD;
//-------------------output-------------------------------------
    private int returnCode;
    private SQLWarningOrErrorList errorList;
    private long rowsAffected;
    private int sqlQueryType;
    private int estimatedCost;

    private int resultSetCount;
    private String[] stmtLabels;
    private String[] proxySyntax;
    private String singleSyntax;
//-------------------------------------------------------------
    ServerApiSqlExecute(int instance, int serverThread) {
        this.instance = instance;
        this.serverThread = serverThread;
        serverWorkerName = ServerConstants.SERVER_WORKER_NAME + "_" + instance + "_" + serverThread;
    }
    void init() {
        reset();
    }
    void reset() {
        dialogueId = 0;
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
        txId = null; // JDBC is the only one that will use this to join a transaction

        holdableCursor = 0;//default
//==================================================================
        isResultSet = false;
        isSpj = false;
        stmt = null;
        pstmt = null;
        cstmt = null;
        trafConn = null;
        trafStmt = null;
//-----------------------------params--------------------------------------
        regParamCount = 0;
        paramCount = 0;
        paramLength = 0;
        paramDescList = null;
        paramDesc = null;
        paramValuesLength = 0;
//------------------------------output-------------------------------------
        returnCode = 0;
        errorList = null;
        rowsAffected = 0;
        sqlQueryType = 0;
        estimatedCost = 0;

        outValuesLength = 0;
        outValues = null;
        resultSetCount = 0;
        outDescList = null;
        rs = null;
        rsMD = null;
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

            dialogueId = bbBody.getInt();
            holdableCursor = bbBody.getInt();
            queryTimeout = bbBody.getInt();
            inpRowCnt = bbBody.getInt();
            maxRowsetSize = bbBody.getInt();
            sqlStmtType = bbBody.getInt();
            stmtHandle = bbBody.getInt();
            stmtType = bbBody.getInt();
            sqlString = ByteBufferUtils.extractStringWithCharset(bbBody);
            cursorName = ByteBufferUtils.extractStringWithCharset(bbBody);
            stmtLabel = ByteBufferUtils.extractStringWithCharset(bbBody);
            stmtExplainLabel = ByteBufferUtils.extractString(bbBody);
//            txId = ByteBufferUtils.extractByteArray(bbBody);
            if(LOG.isDebugEnabled()) {
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
//                LOG.debug(serverWorkerName + ". txId :" + txId);
            }
            if (dialogueId < 1 ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId :" + dialogueId);
            }
            if (dialogueId != clientData.getDialogueId() ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId sent by the Client [sent/expected] : [" + dialogueId + "/" + clientData.getDialogueId() + "]");
            }
//=============================================================================
            trafConn = clientData.getTrafConnection();
            trafStmt = trafConn.getTrafStatement(stmtLabel, stmtHandle);

            isResultSet = trafStmt.getIsResultSet();
            isSpj = trafStmt.getIsSpj();
            stmt = trafStmt.getStatement();
            if (isSpj == true) cstmt = (CallableStatement)stmt; else pstmt = (PreparedStatement)stmt;
//
//=====================Process ServerApiSqlExecute===========================
//
            try {
                int addedBatches = 0;
                paramCount = trafStmt.getParamCount();
                if (paramCount > 0) {
                    long startOffset;
                    int rowCount;
                    int paramRowCount;
                    int paramMode;

                    if(LOG.isDebugEnabled())
                    ByteBufferUtils.toHexString("inValues", bbBody, 10);
                    paramDescList = trafStmt.getParamDescList();
                    paramLength = trafStmt.getParamLength();
                    paramValuesLength = bbBody.getInt();
                    if(LOG.isDebugEnabled()) {
                        LOG.debug("paramValuesLength :" + paramValuesLength);
                        LOG.debug("paramLength :" + paramLength);
                    }
                    if (paramValuesLength > 0) {
                        startOffset = bbBody.position();
                        rowCount = inpRowCnt == 0 ? 1 : inpRowCnt;
                        paramRowCount = rowCount;
                        paramMode = 0;
                        for (int row = 0; row < rowCount; row++) {
                            for (int param = 1; param <= paramCount; param++) {
                                paramDesc = paramDescList.getDescriptors2()[param - 1];
                                paramMode = paramDesc.getParamMode();
                                if ( paramMode == java.sql.ParameterMetaData.parameterModeOut) continue;
                                bbBody = setData(paramDesc, paramRowCount, param, row, bbBody, startOffset);
                            }
                            if (rowCount > 1) {
                                pstmt.addBatch(); // no SPJ
                                addedBatches++;
                            }
                            if(LOG.isDebugEnabled())
                            LOG.debug("endof row :" + row + " addedBatches :" + addedBatches);
                        }
                    }
                    if(LOG.isDebugEnabled())
                        LOG.debug("addedBatches :" + addedBatches);

                    for (int param = 1; param <= paramCount; param++) {
                        paramDesc = paramDescList.getDescriptors2()[param - 1];
                        paramMode = paramDesc.getParamMode();
                        if ( paramMode == java.sql.ParameterMetaData.parameterModeIn) continue;
                        if(LOG.isDebugEnabled())
                        LOG.debug("registerOutParameter :" + param);
                        regParamCount++;
                        cstmt.registerOutParameter(param, java.sql.Types.VARCHAR);
                    }
                }
                int txIdLen = bbBody.limit() - bbBody.position();
                if (txIdLen > 0) {
                    txId = new byte[txIdLen];
                    bbBody.get(txId);
                }
                if(LOG.isDebugEnabled())
                    LOG.debug(serverWorkerName + ". txId :" + Arrays.toString(txId));

                if (addedBatches > 1) {

                    if(LOG.isDebugEnabled())
                        LOG.debug(serverWorkerName + ". T2 Execute.executeBatch()");

                    int[] numStatus = pstmt.executeBatch();
                    rowsAffected = numStatus.length;

                    if(LOG.isDebugEnabled())
                        LOG.debug(serverWorkerName + ". T2 Execute.executeBatch rowsAffected :" + rowsAffected);
                }
                else if (isResultSet) {
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverWorkerName + ". T2 Execute.executeQuery()");
                    rs = pstmt.executeQuery();
                    if (rs != null) {
                        outDescList = trafStmt.getOutDescList();
                        trafStmt.closeAllTResultSets();
                        trafStmt.addTResultSet(new TrafResultSet(rs, 0, stmtLabel, 0, trafStmt.getOutDescList(),""));
                    }
                } else if (isSpj == true) {
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverWorkerName + ". SPJ T2 Execute.execute()");
                    boolean rsAvailable = cstmt.execute();
                    if (regParamCount > 0) { //out, inout parameters
                        rowsAffected = 1;
                        outValuesLength = (int)paramLength;
                        outValues = new byte[outValuesLength];
                        outValues = buildOutValues(paramDescList, outValues, paramCount, cstmt, bbBody.order());
                    }
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverWorkerName + ". SPJ T2 Execute.rsAvailable :" + rsAvailable);
                    if (rsAvailable == true) {
                        setMultipleResultSets(cstmt);
                    }
                } else {
                    resultSetCount = 0;
                    rowsAffected = pstmt.executeUpdate();
                    if(LOG.isDebugEnabled())
                        LOG.debug(serverWorkerName + ". T2 Execute.executeUpdate() rowsAffected :" + rowsAffected);
                }
            } catch (BatchUpdateException bex) {
                LOG.error(serverWorkerName + ". Execute.BatchUpdateException " + bex);
                errorList = new SQLWarningOrErrorList(bex, bex.getUpdateCounts());
                returnCode = errorList.getReturnCode();

            } catch (SQLException ex) {
                LOG.error(serverWorkerName + ". Execute.SQLException " + ex);
                errorList = new SQLWarningOrErrorList(ex);
                returnCode = errorList.getReturnCode();
            }
            if (sqlString.isEmpty() == false)
                sqlQueryType = SqlUtils.getSqlStmtType(sqlString);
            else
                sqlQueryType = SqlUtils.getSqlStmtType(sqlStmtType);
//
//===================calculate length of output ByteBuffer========================
//
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". calculate length of output ByteBuffer");
            bbHeader.clear();
            bbBody.clear();
//
// check if ByteBuffer is big enough for output
//
            TrafResultSet trs = null;

            int dataLength = ServerConstants.INT_FIELD_SIZE;//returnCode
            if (errorList != null)
                dataLength += errorList.lengthOfData();
            else
                dataLength += ServerConstants.INT_FIELD_SIZE;//totalErrorLength = 0

            if (outDescList != null)
                dataLength += outDescList.lengthOfData();
            else
                dataLength += ServerConstants.INT_FIELD_SIZE;//outDescLength = 0

            dataLength += ServerConstants.INT_FIELD_SIZE;//rowsAffected
            dataLength += ServerConstants.INT_FIELD_SIZE;//sqlQueryType
            dataLength += ServerConstants.INT_FIELD_SIZE;//estimatedCost
            dataLength += ByteBufferUtils.lengthOfByteArray(outValues);//outValues

            dataLength += ServerConstants.INT_FIELD_SIZE;// SPJ Result Sets
            if (resultSetCount > 0) {
                boolean bNextResultSet = true;
                trafStmt.setFirstTResultSet();
                while (bNextResultSet) {
                    trs = trafStmt.getTrafResultSet();
                    dataLength += trs.lengthOfData();
                    bNextResultSet = trafStmt.getNextTResultSet();
                }
            }
            dataLength += ByteBufferUtils.lengthOfString(singleSyntax);

            int availableBuffer = bbBody.capacity() - bbBody.position();
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". dataLength :" + dataLength + " availableBuffer :" + availableBuffer);
            if (dataLength > availableBuffer )
                bbBody = ByteBufferUtils.increaseCapacity(bbBody, dataLength > ServerConstants.BODY_SIZE ? dataLength : ServerConstants.BODY_SIZE );
//
//===================== build output ==============================================
//
            if(LOG.isDebugEnabled()) {
                LOG.debug(serverWorkerName + ". returnCode :" + returnCode);
                LOG.debug(serverWorkerName + ". errorList :" + errorList);
                LOG.debug(serverWorkerName + ". outDescList :" + outDescList);
                LOG.debug(serverWorkerName + ". rowsAffected :" + rowsAffected);
                LOG.debug(serverWorkerName + ". sqlQueryType :" + sqlQueryType);
                LOG.debug(serverWorkerName + ". estimatedCost :" + estimatedCost);
                LOG.debug(serverWorkerName + ". outValues :" + outValues);
                LOG.debug(serverWorkerName + ". SPJ Result Sets :" + resultSetCount);
                LOG.debug(serverWorkerName + ". singleSyntax :" + singleSyntax);
            }
            bbBody.putInt(returnCode);
            if (errorList != null)
                errorList.insertIntoByteBuffer(bbBody);
            else
                bbBody.putInt(0);

            if (outDescList != null) {
                outDescList.insertIntoByteBuffer(bbBody);
            } else
                bbBody.putInt(0);

            ByteBufferUtils.insertUInt(rowsAffected, bbBody);
            bbBody.putInt(sqlQueryType);
            bbBody.putInt(estimatedCost);
            ByteBufferUtils.insertByteArray(outValues, bbBody); //outValues
            bbBody.putInt(resultSetCount);

            if (resultSetCount > 0) {
                boolean bNextResultSet = true;
                trafStmt.setFirstTResultSet();
                while (bNextResultSet) {
                    trs = trafStmt.getTrafResultSet();
                    trs.insertIntoByteBuffer(bbBody);
                    bNextResultSet = trafStmt.getNextTResultSet();
                }
                trafStmt.setFirstTResultSet();
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

        } catch (UnsupportedEncodingException ue) {
            LOG.error(serverWorkerName + ". Execute.UnsupportedEncodingException :" + ue);
            clientData.setRequestAndDisconnect();
        } catch (Exception e) {
            LOG.error(serverWorkerName + ". Execute.Exception :" + e);
            clientData.setRequestAndDisconnect();
        }
        reset();
        return clientData;
    }
//===================================================================================
//
    ByteBuffer setData(Descriptor2 paramDesc, int paramRowCount, int param, int row, ByteBuffer bbBody, long startOffset) throws SQLException, ParseException, UnsupportedEncodingException {
        int paramLength = 0;

        String tmpStr = "";
        Short tmps = 0;
        Integer tmpi = 0;
        Long tmpl = 0L;
        Float tmpf = 0f;
        Double tmpd = 0d;
        BigInteger tmpbi;
        BigDecimal tmpbd;
        boolean isSigne = true;

        Object retObj;
        byte[] tbuffer = null;
        int year, month, day, hour, minute, second;
        long nanoSeconds;
        String charSet = "";

        int precision = paramDesc.getPrecision();
        int scale = paramDesc.getScale();
        int datetimeCode = paramDesc.getDatetimeCode();
        int FSDataType = paramDesc.getFsDataType();
        int OdbcDataType = paramDesc.getOdbcDataType();
        int dataCharSet = paramDesc.getSqlCharset();
        int dataLength = paramDesc.getMaxLen();
        int dataType = paramDesc.getDataType();

        // setup the offsets
        int noNullValue = paramDesc.getNoNullValue();
        int nullValue = paramDesc.getNullValue();
        long nullOffset = 0L;
        long noNullOffset = 0L;

        if (dataType == ServerConstants.SQLTYPECODE_VARCHAR_WITH_LENGTH) {
            dataLength += 2;

            if (dataLength % 2 != 0)
                dataLength++;
        }
        if(LOG.isDebugEnabled())
            LOG.debug("nullValue :"+ nullValue);
        if (nullValue != -1) {
            nullValue = (nullValue * paramRowCount) + (row * 2);
            nullOffset = startOffset + nullValue;
            bbBody.position((int)nullOffset);
            short isNull = bbBody.getShort();
            if(LOG.isDebugEnabled())
                LOG.debug("isNull :"+ isNull);
            if ( isNull == -1) {
                if (isSpj == true) cstmt.setObject(param,null); else pstmt.setObject(param,null);
                return bbBody;
            }
        }
        noNullValue = (noNullValue * paramRowCount) + (row * dataLength);
        noNullOffset = startOffset + noNullValue;
        bbBody.position((int)noNullOffset);

        if(LOG.isDebugEnabled())
            LOG.debug("[" + param + "] noNullOffset :"+ noNullOffset + " noNullValue :" + noNullValue);

        if(dataCharSet == SqlUtils.SQLCHARSETCODE_UNICODE)
            charSet = "UTF-16LE";
        else
            charSet = SqlUtils.getCharsetName(dataCharSet);

        switch (dataType) {
            case ServerConstants.SQLTYPECODE_CHAR:
                if(LOG.isDebugEnabled())
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_CHAR");
                tbuffer = new byte[dataLength];
                bbBody.get(tbuffer, 0, dataLength);
                retObj = new String(tbuffer, charSet);
                if(LOG.isDebugEnabled())
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_CHAR length :" + dataLength + " scale :" + scale + "  retObj :" + retObj);
                break;
            case ServerConstants.SQLTYPECODE_VARCHAR:
            case ServerConstants.SQLTYPECODE_VARCHAR_LONG:
                if(LOG.isDebugEnabled())
                    ByteBufferUtils.toHexString("VARCHAR/VARCHAR_LONG", bbBody, (dataLength > 100 ? 100 : dataLength));
                tbuffer = new byte[dataLength];
                bbBody.get(tbuffer, 0, dataLength);
                retObj = new String(tbuffer, charSet);
                if(LOG.isDebugEnabled()) {
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_VARCHAR len :" + dataLength + " scale :" + scale + "  retObj :" + retObj);
                    LOG.debug(" retObj :" + Arrays.toString(((String)retObj).getBytes()));
                }
                break;
            case ServerConstants.SQLTYPECODE_VARCHAR_WITH_LENGTH:
                if(LOG.isDebugEnabled())
                    ByteBufferUtils.toHexString("VARCHAR_WITH_LENGTH", bbBody, (dataLength > 100 ? 100 : dataLength));
                boolean shortLength = precision < Math.pow(2, 15);
                int dataOffset = (shortLength) ? 2 : 4;
                dataLength = (shortLength) ? bbBody.getShort() : bbBody.getInt();
                tbuffer = new byte[dataLength];
                bbBody.get(tbuffer, 0, dataLength);
                retObj = new String(tbuffer, charSet);
                if(LOG.isDebugEnabled()) {
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_VARCHAR len :" + dataLength + " scale :" + scale + "  retObj :" + retObj);
                    LOG.debug(" retObj :" + Arrays.toString(((String)retObj).getBytes()));
                }
                break;
            case ServerConstants.SQLTYPECODE_INTERVAL:
                tbuffer = new byte[dataLength];
                bbBody.get(tbuffer);
                retObj = new String(tbuffer);
                if(LOG.isDebugEnabled())
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_INTERVAL dataLength :" + dataLength + " scale :" + scale + "  retObj :" + retObj);
                break;
            case ServerConstants.SQLTYPECODE_DATETIME:
                switch (datetimeCode) {
                    case ServerConstants.SQLDTCODE_DATE:
                        tbuffer = new byte[dataLength];
                        bbBody.get(tbuffer);
                        retObj = java.sql.Date.valueOf((new String(tbuffer)).trim());
                        if(LOG.isDebugEnabled())
                            LOG.debug("["+ param + "] dataType :SQLDTCODE_DATE dataLength :" + dataLength + " scale :" + scale + "  retObj :" + retObj);
                        break;
                    case ServerConstants.SQLDTCODE_TIMESTAMP:
                        tbuffer = new byte[dataLength];
                        bbBody.get(tbuffer);
                        retObj = Timestamp.valueOf((new String(tbuffer)).trim());
                        if(LOG.isDebugEnabled())
                            LOG.debug("["+ param + "] dataType :SQLDTCODE_TIMESTAMP dataLength :" + dataLength + " scale :" + scale + "  retObj :" + retObj);
                        break;
                    case ServerConstants.SQLDTCODE_TIME:
                        if (OdbcDataType == java.sql.Types.OTHER) // For Desc.SQLDTCODE_HOUR_TO_FRACTION
                        {
                            tbuffer = new byte[dataLength];
                            bbBody.get(tbuffer);
                            retObj = new String((new String(tbuffer)).trim());
                        } else {
                            dataLength = ServerConstants.timeLength;
                            tbuffer = new byte[dataLength];
                            bbBody.get(tbuffer);
                            retObj = Time.valueOf((new String(tbuffer)).trim());
                        }
                        if(LOG.isDebugEnabled())
                            LOG.debug("["+ param + "] dataType :SQLDTCODE_TIME dataLength :" + dataLength + " scale :" + scale + "  retObj :" + retObj);
                        break;
                    default:
                        tbuffer = new byte[dataLength];
                        bbBody.get(tbuffer);
                        retObj = new String(tbuffer);
                        if(LOG.isDebugEnabled())
                            LOG.debug("["+ param + "] dataType :default dataLength :" + dataLength + " scale :" + scale + "  retObj :" + retObj);
                        break;
                } // SQLTYPECODE_DATETIME
                break;
            case ServerConstants.SQLTYPECODE_SMALLINT:
                short sValue = bbBody.getShort();
                retObj = new Short(sValue);
                if (scale > 0) {
                    retObj = new BigDecimal(new BigInteger(retObj.toString()), scale);
                }
                if(LOG.isDebugEnabled())
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_SMALLINT dataLength :" + dataLength + " scale :" + scale + "  retObj :" + retObj);
                break;
            case ServerConstants.SQLTYPECODE_SMALLINT_UNSIGNED:
                int signedSValue = ByteBufferUtils.extractUShort(bbBody);
                if (scale > 0) {
                    tmpbd = new BigDecimal(new BigInteger(String.valueOf(signedSValue)), (int) scale);
                } else {
                    tmpbd = new BigDecimal(String.valueOf(signedSValue));
                }
                retObj = tmpbd;
                if(LOG.isDebugEnabled())
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_SMALLINT_UNSIGNED dataLength :" + dataLength + " scale :" + scale + "  retObj :" + retObj);
                break;
            case ServerConstants.SQLTYPECODE_INTEGER:
                retObj = new Integer(bbBody.getInt());
                if (scale > 0) {
                    retObj = new BigDecimal(new BigInteger(retObj.toString()), scale);
                }
                if(LOG.isDebugEnabled())
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_INTEGER dataLength :" + dataLength + " scale :" + scale + "  retObj :" + retObj);
                break;
            case ServerConstants.SQLTYPECODE_INTEGER_UNSIGNED:
                retObj = new Long(ByteBufferUtils.extractUInt(bbBody));
                if (scale > 0) {
                    retObj = new BigDecimal(new BigInteger(retObj.toString()), scale);
                }
                if(LOG.isDebugEnabled())
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_INTEGER_UNSIGNED dataLength :" + dataLength + " scale :" + scale + " retObj :" + retObj);
                break;
            case ServerConstants.SQLTYPECODE_LARGEINT:
                retObj = new Long(bbBody.getLong());
                if (scale > 0) {
                    retObj = new BigDecimal(new BigInteger(retObj.toString()), scale);
                }
                if(LOG.isDebugEnabled())
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_LARGEINT + dataLength :" + dataLength + " scale :" + scale + " retObj :" + retObj);
                break;
//--------------------------------- SQLTYPECODE_NUMERIC/SQLTYPECODE_NUMERIC_UNSIGNED converted to INTEGER using fsDataType_ (Descriptor2) ---------
            case ServerConstants.SQLTYPECODE_NUMERIC:
            case ServerConstants.SQLTYPECODE_NUMERIC_UNSIGNED:
                if(LOG.isDebugEnabled())
                    ByteBufferUtils.toHexString("SQLTYPECODE_NUMERIC", bbBody, (dataLength > 100 ? 100 : dataLength));
                throw new SQLException("restricted_data_type");
            case ServerConstants.SQLTYPECODE_DECIMAL:
            case ServerConstants.SQLTYPECODE_DECIMAL_LARGE:
            case ServerConstants.SQLTYPECODE_DECIMAL_UNSIGNED:
            case ServerConstants.SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
                String retStr;
                tbuffer = new byte[dataLength];
                bbBody.get(tbuffer);

                byte firstByte = tbuffer[0];
                byte sign = (byte) (firstByte & (byte) (0x80));
                if (sign == (byte) (0x80)) {
                    tbuffer[0] = (byte) (tbuffer[0] - (byte) (0x80));
                    retStr = "-" + new String(tbuffer);
                } else {
                    retStr = new String(tbuffer);
                }
                retObj = new BigDecimal(new BigInteger(retStr), scale);
                if(LOG.isDebugEnabled())
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_DECIMAL dataLength :" + dataLength + " scale :" + scale + " retObj :" + retObj);
                break;
            case ServerConstants.SQLTYPECODE_REAL:
                retObj = new Float(Float.intBitsToFloat(bbBody.getInt()));
                if(LOG.isDebugEnabled())
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_REAL dataLength :" + dataLength + " scale :" + scale + " retObj :" + retObj);
                break;
            case ServerConstants.SQLTYPECODE_DOUBLE:
            case ServerConstants.SQLTYPECODE_FLOAT:
                retObj = new Double(Double.longBitsToDouble(bbBody.getLong()));
                if(LOG.isDebugEnabled())
                    LOG.debug("["+ param + "] dataType :SQLTYPECODE_DOUBLE dataLength :" + dataLength + " scale :" + scale + "  retObj :" + retObj);
                break;
            case ServerConstants.SQLTYPECODE_BIT:
            case ServerConstants.SQLTYPECODE_BITVAR:
            case ServerConstants.SQLTYPECODE_BPINT_UNSIGNED:
            default:
                throw new SQLException("restricted_data_type");
        } // dataType
        if (isSpj == true) cstmt.setObject(param,retObj); else pstmt.setObject(param,retObj);
        return bbBody;
    }
    byte[] buildOutValues(Descriptor2List paramDescList, byte[] outValues, int paramCount, CallableStatement cstmt, ByteOrder o) throws SQLException, UnsupportedEncodingException {

        SQLMXCallableStatement sqlcall = (SQLMXCallableStatement)cstmt;
        int paramOffset = 0;

        if(LOG.isDebugEnabled())
            LOG.debug(" buildOutValues paramCount :"+ paramCount);

        for (int param = 1; param <= paramCount; param++) {
            Descriptor2 dsc = paramDescList.getDescriptors2()[param - 1];
            if ( dsc.getParamMode() == java.sql.ParameterMetaData.parameterModeIn) continue;

            int noNullValueOffset = dsc.getNoNullValue();
            int nullValueOffset = dsc.getNullValue();

            if (nullValueOffset != -1)
                nullValueOffset += paramOffset;
            if (noNullValueOffset != -1)
                noNullValueOffset += paramOffset;

            byte[] sqlarray = sqlcall.getSQLBytes(param);
            if (sqlarray == null) {
                short value = -1;
                outValues[(int)nullValueOffset] = (byte) ((value >>> 8) & 0xff);
                outValues[(int)(nullValueOffset + 1)] = (byte) ((value) & 0xff);
            } else {
                outValues = SqlUtils.formatSqlT4Output(dsc, sqlarray, noNullValueOffset, outValues, o);
            }
        }
        return outValues;
    }
    void setMultipleResultSets(CallableStatement cstmt) throws SQLException {
        resultSetCount = 0;
        Descriptor2List outDescList = null;
        ResultSet rs = null;
        boolean moreResults = true;
        while(moreResults) {
            rs = cstmt.getResultSet();
            addTResultSet(rs);
            resultSetCount++;
            moreResults = cstmt.getMoreResults();
        }
    }
    void addTResultSet(ResultSet rs) throws SQLException {

        ResultSetMetaData rsmd;
        TResultSetMetaData trsmd;
        SQLMXResultSetMetaData strsmd;

        //-------------T2 desc fields-------------------
        int sqlCharset_;
        int odbcCharset_;
        int sqlDataType_;
        int dataType_;
        short sqlPrecision_;
        short sqlDatetimeCode_;
        int sqlOctetLength_;
        int isNullable_;
        String name_;
        int scale_;
        int precision_;
        boolean isSigned_;
        boolean isCurrency_;
        boolean isCaseSensitive_;
        String catalogName_;
        String schemaName_;
        String tableName_;
        int fsDataType_;
        int intLeadPrec_;
        int paramMode_;
        int paramIndex_;
        int paramPos_;

        int odbcPrecision_;
        int maxLen_;

        int displaySize_;
        String label_;

        Descriptor2List descl = null;
        int columnCount;

        rsmd = rs.getMetaData();
        columnCount = rsmd.getColumnCount();

        if (columnCount > 0) {
            strsmd = (SQLMXResultSetMetaData)rsmd;
            outDescList = new Descriptor2List(columnCount,false);

            for (int column = 1; column <= columnCount; column++) {
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
                outDescList.addDescriptor(column, outDesc);
            }
            if(LOG.isDebugEnabled()) {
                for (int column = 1; column <= columnCount; column++) {
                    Descriptor2 dsc = outDescList.getDescriptors2()[column-1];
                    LOG.debug(serverWorkerName + ". [" + column + "] Column descriptor -------------" );
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
                    LOG.debug(serverWorkerName + ". varLength " + column + " :" + dsc.getVarLength());
                    LOG.debug(serverWorkerName + ". Column descriptor End-------------");
                }
            }
        }
        if (columnCount > 0)
            trafStmt.addTResultSet(new TrafResultSet(rs, 0, stmtLabel, 0, outDescList,""));
    }
}
