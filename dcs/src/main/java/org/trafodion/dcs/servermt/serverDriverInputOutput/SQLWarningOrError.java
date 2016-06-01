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
import org.trafodion.dcs.servermt.serverHandler.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class SQLWarningOrError {
    private static  final Log LOG = LogFactory.getLog(SQLWarningOrError.class);
    private int rowId;
    private int sqlCode;
    private String text;
    private String sqlState;

    public SQLWarningOrError() {
        rowId = 0;
        sqlCode = 0;
        text = "";
        sqlState = "";
    }
    public SQLWarningOrError(SQLException ex, int rowId) {

        this.rowId = rowId;
        sqlCode = ex.getErrorCode();
        text = ex.getMessage();
        sqlState = ex.getSQLState();
        if(LOG.isDebugEnabled()){
            LOG.debug("SQLWarningOrError() rowId: " + rowId + ", Messge: " + sqlCode + ",Vendor Code: " + text + ",SQLState: " + sqlState);
        }
    }
    public void extractFromByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException{
        rowId = bbBuf.getInt();
        sqlCode = bbBuf.getInt();
        text = ByteBufferUtils.extractString(bbBuf);
        byte[] dst = new byte[5];
        bbBuf.get(dst,0,5);                        // is it 5 or 6??????
        sqlState = new String(dst);
        bbBuf.get(); //null terminator
    }
    public void insertIntoByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException {
        bbBuf.putInt(rowId);
        bbBuf.putInt(sqlCode);
        ByteBufferUtils.insertString(text,bbBuf);
        sqlState = (sqlState == null)? "HY024" : sqlState;
        bbBuf.put(sqlState.getBytes(),0,5);
        bbBuf.put((byte)0); //null terminator
    }
    public int lengthOfData() {
        int dataLength = 0;

        dataLength += ServerConstants.INT_FIELD_SIZE;         //rowId
        dataLength += ServerConstants.INT_FIELD_SIZE;         //sqlcode
        dataLength += ByteBufferUtils.lengthOfString(text);
        dataLength += 6;                                //sqlstate + null terminator
        return dataLength;
    }
    public int getRowId(){
        return rowId;
    }
    public int getSqlCode(){
        return sqlCode;
    }
    public String getText(){
        return text;
    }
    public String getSqlState(){
        return sqlState;
    }

    public void setRowId(int rowId){
        this.rowId = rowId;
    }
    public void setSqlCode(int sqlCode){
        this.sqlCode = sqlCode;
    }
    public void setText(String text){
        this.text = text;
    }
    public void setSqlState(String sqlState){
        this.sqlState = sqlState;
    }
}
