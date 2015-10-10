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

package org.trafodion.dcs.servermt.serverSql;

import java.sql.*;
import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.math.BigDecimal;
import java.util.Calendar;
import java.util.HashMap;
import java.util.Map;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.serverDriverInputOutput.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class TrafResultSet {
    private static  final Log LOG = LogFactory.getLog(TrafResultSet.class);

    /* -----------------output for resultset
        int stmt_handle                    (int)
        String stmtLabels[resultSetIndex]  (String)
        long stmt_label_charset            (int)
        ----- Descriptor2List------------------------
        outputDescLength                   (int)
        if (outputDescLength > 0){
            outputParamsLength             (int)
            outputNumberParams             (int)

            for (int j = 0; j < outputNumberParams; j++) {
                Descriptor2(buf, ic);      (Descriptor2)
            }
        }
        proxySyntax[resultSetIndex] = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
    ------------------------ end of resultset
    */
    private boolean isClosed;
    private ResultSet rs;

    private int stmtHandle;
    private String stmtLabel;
    private long stmtLabelCharset;
    private Descriptor2List columnDescList;
    private String proxySyntax;

    public TrafResultSet(ResultSet rs, int stmtHandle, String stmtLabel, long stmtLabelCharset, Descriptor2List columnDescList, String proxySyntax){
        if(LOG.isDebugEnabled())
             LOG.debug("TrafResultSet constructor");
        init();
        this.rs = rs;
        this.stmtHandle = stmtHandle;
        this.stmtLabel = stmtLabel;
        this.stmtLabelCharset = stmtLabelCharset;
        this.columnDescList = columnDescList;
        this.proxySyntax = proxySyntax;
    }
    void init(){
        reset();
    }
    void reset(){
        isClosed = false;
        rs = null;
        stmtHandle = 0;
        stmtLabel = "";
        stmtLabelCharset = 0;
        columnDescList = null;
        proxySyntax = null;
    }
    public void insertIntoByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException {
        bbBuf.putInt(stmtHandle);
        ByteBufferUtils.insertString(stmtLabel,bbBuf);
        bbBuf.putInt((int)stmtLabelCharset);
        columnDescList.insertIntoByteBuffer(bbBuf);
        ByteBufferUtils.insertString(proxySyntax,bbBuf);
    }
    public int lengthOfData() {
        int length = ServerConstants.INT_FIELD_SIZE; //stmtHandle
        length += ByteBufferUtils.lengthOfString(stmtLabel);
        length += ServerConstants.INT_FIELD_SIZE;  //stmtLabelCharset
        length += columnDescList.lengthOfData();
        length += ByteBufferUtils.lengthOfString(proxySyntax);
        return length;
    }
    public void closeTResultSet(){
        try {
           if (rs != null && isClosed == false){
               rs.close();
               isClosed = true;
               rs = null;
           }
         } catch (SQLException sql){}
    }
    public boolean isClosed(){
        return isClosed;
    }
    public ResultSet getResultSet(){
        return rs;
    }
    public String getStmtLabel(){
        return stmtLabel;
    }
    public long getStmtLabelCharset(){
        return stmtLabelCharset;
    }
    public Descriptor2List getColumnDescList(){
        return columnDescList;
    }
    public String getProxySyntax(){
        return proxySyntax;
    }
    public void setResultSet(ResultSet rs){
        this.rs = rs;
    }
    public void setStmtLabel(String stmtLabel){
        this.stmtLabel = stmtLabel;
    }
    public void setStmtLabelCharset(long stmtLabelCharset){
        this.stmtLabelCharset = stmtLabelCharset;
    }
    public void setColumnDescList(Descriptor2List columnDescList){
        this.columnDescList = columnDescList;
    }
    public void setProxySyntax(String proxySyntax){
        this.proxySyntax = proxySyntax;
    }
}
