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

import java.sql.*;
import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverHandler.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class ErrorDesc {
    private static  final Log LOG = LogFactory.getLog(ErrorDesc.class);
    private int rowId;
    private int errorDiagnosticId;
    private int sqlcode;
    private String sqlstate;
    private String errorText;
    private int operationAbortId;
    private int errorCodeType;
    private String Param1;
    private String Param2;
    private String Param3;
    private String Param4;
    private String Param5;
    private String Param6;
    private String Param7;
    
    public ErrorDesc(){
        reset();
    }
    public ErrorDesc(ErrorDesc ed){
        rowId = ed.rowId;
        errorDiagnosticId = ed.errorDiagnosticId;
        sqlcode = ed.sqlcode;
        sqlstate = ed.sqlstate;
        errorText = ed.errorText;
        operationAbortId = ed.operationAbortId;
        errorCodeType = ed.errorCodeType;
        Param1 = ed.Param1;
        Param2 = ed.Param2;
        Param3 = ed.Param3;
        Param4 = ed.Param4;
        Param5 = ed.Param5;
        Param6 = ed.Param6;
        Param7 = ed.Param7;
    }
    public ErrorDesc(SQLException ex){
        if(LOG.isDebugEnabled()){
            LOG.debug("Messge : " + ex.getMessage()); //String
            LOG.debug("Vendor Code : " + ex.getErrorCode()); // int
            LOG.debug("SQLState : " + ex.getSQLState()); //String
        }
        reset();
        sqlcode = ex.getErrorCode();
        sqlstate = ex.getSQLState();
        errorText = ex.getMessage();
    }
    private void reset(){
        rowId = 0;
        errorDiagnosticId = 0;
        sqlcode = 0;
        sqlstate = "";
        errorText = "";
        operationAbortId = 0;
        errorCodeType = 0;
        Param1 = "";
        Param2 = "";
        Param3 = "";
        Param4 = "";
        Param5 = "";
        Param6 = "";
        Param7 = "";
    }
    // ----------------------------------------------------------
    public void extractFromByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException{
        rowId = bbBuf.getInt();
        errorDiagnosticId = bbBuf.getInt();
        sqlcode = bbBuf.getInt();

        // Note, SQLSTATE is logically 5 bytes, but ODBC uses 6 bytes for some
        // reason.
        byte[] dst = new byte[6];
        bbBuf.get(dst,0,5);
        sqlstate = new String(dst);
        errorText = ByteBufferUtils.extractString(bbBuf);

        operationAbortId = bbBuf.getInt();
        errorCodeType = bbBuf.getInt();
        Param1 = ByteBufferUtils.extractString(bbBuf);
        Param2 = ByteBufferUtils.extractString(bbBuf);
        Param3 = ByteBufferUtils.extractString(bbBuf);
        Param4 = ByteBufferUtils.extractString(bbBuf);
        Param5 = ByteBufferUtils.extractString(bbBuf);
        Param6 = ByteBufferUtils.extractString(bbBuf);
        Param7 = ByteBufferUtils.extractString(bbBuf);
    }
    
    public void insertIntoByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException {
        bbBuf.putInt(rowId);
        bbBuf.putInt(errorDiagnosticId);
        bbBuf.putInt(sqlcode);

        sqlstate = sqlstate + " ";
        bbBuf.put(sqlstate.getBytes());
        ByteBufferUtils.insertString(errorText, bbBuf);

        bbBuf.putInt(operationAbortId);
        bbBuf.putInt(errorCodeType);
        ByteBufferUtils.insertString(Param1,bbBuf);
        ByteBufferUtils.insertString(Param2,bbBuf);
        ByteBufferUtils.insertString(Param3,bbBuf);
        ByteBufferUtils.insertString(Param4,bbBuf);
        ByteBufferUtils.insertString(Param5,bbBuf);
        ByteBufferUtils.insertString(Param6,bbBuf);
        ByteBufferUtils.insertString(Param7,bbBuf);
    }
    
    public int lengthOfData() {
        int dataLength = 0;
        
        dataLength += ServerConstants.INT_FIELD_SIZE;                     //rowId
        dataLength += ServerConstants.INT_FIELD_SIZE;                     //errorDiagnosticId
        dataLength += ServerConstants.INT_FIELD_SIZE;                     //sqlcode
        dataLength += 6;                                //sqlstate
        dataLength += ByteBufferUtils.lengthOfString(errorText);
        dataLength += ServerConstants.INT_FIELD_SIZE;                     //operationAbortId
        dataLength += ServerConstants.INT_FIELD_SIZE;                     //errorCodeType
        dataLength += ByteBufferUtils.lengthOfString(Param1);
        dataLength += ByteBufferUtils.lengthOfString(Param2);
        dataLength += ByteBufferUtils.lengthOfString(Param3);
        dataLength += ByteBufferUtils.lengthOfString(Param4);
        dataLength += ByteBufferUtils.lengthOfString(Param5);
        dataLength += ByteBufferUtils.lengthOfString(Param6);
        dataLength += ByteBufferUtils.lengthOfString(Param7);
       
        return dataLength;
    }
    public int getRowId(){
        return rowId;
    }
    public int getErrorDiagnosticId(){
        return errorDiagnosticId;
    }
    public int getSqlcode(){
        return sqlcode;
    }
    public String getSqlstate(){
        return sqlstate;
    }
    public String getErrorText(){
        return errorText;
    }
    public int getOperationAbortId(){
        return operationAbortId;
    }
    public int getErrorCodeType(){
        return errorCodeType;
    }
    public String getParam1(){
        return Param1;
    }
    public String getParam2(){
        return Param2;
    }
    public String getParam3(){
        return Param3;
    }
    public String getParam4(){
        return Param4;
    }
    public String getParam5(){
        return Param5;
    }
    public String getParam6(){
        return Param6;
    }
    public String getParam7(){
        return Param7;
    }
//================================================
    public void setRowId(int rowId){
        this.rowId = rowId ;
    }
    public void setErrorDiagnosticId(int errorDiagnosticId){
        this.errorDiagnosticId = errorDiagnosticId;
    }
    public void setSqlcode(int sqlcode){
        this.sqlcode = sqlcode;
    }
    public void setSqlstate(String sqlstate){
        this.sqlstate =  sqlstate;
    }
    public void setErrorText(String errorText){
        this.errorText = errorText;
    }
    public void setOperationAbortId(int operationAbortId){
        this.operationAbortId = operationAbortId;
    }
    public void setErrorCodeType(int errorCodeType){
        this.errorCodeType = errorCodeType;
    }
    public void setParam1(String Param1){
        this.Param1 = Param1;
    }
    public void setParam2(String Param2){
        this.Param2 = Param2;
    }
    public void setParam3(String Param3){
        this.Param3 = Param3;
    }
    public void setParam4(String Param4){
        this.Param4 = Param4;
    }
    public void setParam5(String Param5){
        this.Param5 = Param5;
    }
    public void setParam6(String Param6){
        this.Param6 = Param6;
    }
    public void setParam7(String Param7){
        this.Param7 = Param7;
    }
}
