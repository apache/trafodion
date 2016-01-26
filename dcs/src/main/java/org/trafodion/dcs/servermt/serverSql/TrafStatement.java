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
import java.util.*;
import java.math.BigDecimal;
import java.util.Calendar;
import java.util.concurrent.*;
//import java.util.HashMap;
//import java.util.Map;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.serverDriverInputOutput.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class TrafStatement {
    private static  final Log LOG = LogFactory.getLog(TrafStatement.class);
    private String serverWorkerName = "";
    private String stmtLabel = "";
    private int stmtHandle = 0;
    private Object stmt = null;
//    private Statement stmt = null;
//    private PreparedStatement pstmt = null;
    private int paramCount = 0;
    private long paramLength = 0;
    private Descriptor2List paramDescList = null;
    private boolean isResultSet = false;
    private boolean isSpj = false;
    private int sqlStmtType = ServerConstants.TYPE_UNKNOWN;
// result sets
    private int resultSetCount;
    private Integer curKey;
    private Descriptor2List outDescList;
    private ConcurrentHashMap<Integer, TrafResultSet> resultSetList = new ConcurrentHashMap<Integer, TrafResultSet>();

    private Random random = new Random();

    public TrafStatement(String serverWorkerName, String stmtLabel, Connection conn, String sqlString, int sqlStmtType) throws SQLException {
        init();
        this.stmtLabel = stmtLabel;
        stmtHandle = this.hashCode();
        this.serverWorkerName = serverWorkerName;
        if(LOG.isDebugEnabled())
            LOG.debug(serverWorkerName + ". constructor TrafStatement[" + stmtLabel + "/" + stmtHandle + "]");
        setStatement(conn, sqlString, sqlStmtType);
    }
    void init(){
        reset();
    }
    void reset(){
        stmt = null;
        paramDescList = null;
        paramCount = 0;
        paramLength = 0;
        resultSetCount = 0;
        curKey = 0;
        outDescList = null;
        isResultSet = false;
        isSpj = false;
        sqlStmtType = ServerConstants.TYPE_UNKNOWN;
    }
    public void closeTStatement() {
        try {
            if (stmt != null){
                if (stmt instanceof Statement){
                    Statement st = (Statement)stmt;
                    if (st.isClosed() == false){
                        st.close();
                        if(LOG.isDebugEnabled())
                            LOG.debug(serverWorkerName + ". T2 st.close(" + stmtLabel + ")");
                    }
                }
                else if (stmt instanceof PreparedStatement){
                    PreparedStatement pst = (PreparedStatement)stmt;
                    if (pst.isClosed() == false){
                        pst.close();
                        if(LOG.isDebugEnabled())
                            LOG.debug(serverWorkerName + ". T2 pst.close(" + stmtLabel + ")");
                    }
                }
            }
        } catch (SQLException sql){}
        closeAllTResultSets();
        reset();
    }
    public void closeTResultSet(){
        if(LOG.isDebugEnabled())
            LOG.debug(serverWorkerName + ". TrafStatement closeTResultSet (" + stmtLabel + ")");
        try {
            if (curKey != 0){
                resultSetList.get(curKey).closeTResultSet();
             }
        } catch (Exception e){}
    }
    public void closeAllTResultSets() {
        if (LOG.isDebugEnabled())
            LOG.debug(serverWorkerName + ". closeAllTResultSets resultSetCount : " + resultSetCount);
        if (resultSetCount != 0){
            Integer key;
            Iterator<Integer> keySetIterator = resultSetList.keySet().iterator();
            while (keySetIterator.hasNext()) {
                key = keySetIterator.next();
                resultSetList.get(key).closeTResultSet();
            }
            resultSetList.clear();
        }
        resultSetCount = 0;
        curKey = 0;
    }
    public boolean getNextTResultSet(){
        if (LOG.isDebugEnabled())
            LOG.debug(serverWorkerName + ". getNextTResultSet key :" + (curKey + 1) );
        Integer key = curKey + 1;
        if (key <= resultSetCount){
            if (resultSetList.containsKey(key)){
                if (LOG.isDebugEnabled())
                    LOG.debug(serverWorkerName + ". getNextTResultSet returns true ");
                return true;
            }
        }
        if (LOG.isDebugEnabled())
            LOG.debug(serverWorkerName + ". getNextTResultSet returns false ");
        return false;
    }
    public void setFirstTResultSet(){
        if (LOG.isDebugEnabled())
            LOG.debug(serverWorkerName + ". setFirstTResultSet");
        curKey = 1;
    }
    public void addTResultSet(TrafResultSet trs){
        Integer maxKey = 0;
        Integer key = 0;
        Iterator<Integer> keySetIterator = resultSetList.keySet().iterator();
        while (keySetIterator.hasNext()) {
            key = keySetIterator.next();
            if (key > maxKey) maxKey = key;
        }
        key = maxKey + 1;
        resultSetList.put(key, trs);
        resultSetCount++;
        curKey = 1;
        if (LOG.isDebugEnabled())
            LOG.debug(serverWorkerName + ". addTResultSet key :" + key);
    }
//=====================================================
    public void setOutDescList(Descriptor2List outDescList){
        this.outDescList = outDescList;
    }
    public void setParamCount(int paramCount){
        this.paramCount = paramCount;
    }
    public void setParamDescList(Descriptor2List paramDescList){
        this.paramDescList = paramDescList;
    }
    public void setParamLength(long paramLength){
        this.paramLength = paramLength;
    }
    public void setIsResultSet(boolean isResultSet){
        this.isResultSet = isResultSet;
    }
    public void setIsSpj(boolean isSpj){
        this.isSpj = isSpj;
    }
    public void setStatement(Connection conn, String sqlString, int sqlStmtType) throws SQLException{
        if(LOG.isDebugEnabled())
            LOG.debug(serverWorkerName + ". TrafStatement.setStatement [" + stmtLabel + "]");
        closeTStatement();
        this.sqlStmtType = sqlStmtType;
        switch (sqlStmtType){
            case ServerConstants.TYPE_SELECT:
            case ServerConstants.TYPE_EXPLAIN:
            case ServerConstants.TYPE_CATOLOG:
                isResultSet = true;
                break;
            case ServerConstants.TYPE_CALL:
                isSpj = true;
            case ServerConstants.TYPE_UPDATE:
            case ServerConstants.TYPE_DELETE:
            case ServerConstants.TYPE_INSERT:
            case ServerConstants.TYPE_INSERT_PARAM:
            case ServerConstants.TYPE_CREATE:
            case ServerConstants.TYPE_GRANT:
            case ServerConstants.TYPE_DROP:
            case ServerConstants.TYPE_CONTROL:
                isResultSet = false;
            default:
        }
        if (sqlString != null){
            if (isSpj == true){
                stmt = conn.prepareCall(sqlString);
                if(LOG.isDebugEnabled())
                    LOG.debug(serverWorkerName + ". T2 conn.prepareCall [" + stmtLabel + "] sqlString :" + sqlString);
            }
            else {
                stmt = conn.prepareStatement(sqlString);
                if(LOG.isDebugEnabled())
                    LOG.debug(serverWorkerName + ". T2 conn.prepareStatement [" + stmtLabel + "] sqlString :" + sqlString);
            }
        }
        else {
            this.stmt = conn.createStatement();
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". T2 conn.createStatement [" + stmtLabel + "]");
        }
    }
    public void setSqlStmtType(int sqlStmtType){
        this.sqlStmtType = sqlStmtType;
    }
//================================================
    public Object getStatement(){
        return stmt;
    }
    public Descriptor2List getOutDescList(){
        return outDescList;
    }
    public Descriptor2List getParamDescList(){
        return paramDescList;
    }
    public int getParamCount(){
        return paramCount;
    }
    public long getParamLength(){
        return paramLength;
    }
    public boolean getIsResultSet(){
        return isResultSet;
    }
    public boolean getIsSpj(){
        return isSpj;
    }
    public TrafResultSet getTrafResultSet(){
        return resultSetList.get(curKey);
    }
    public int getSqlStmtType(){
        return sqlStmtType;
    }
    public Integer getStmtHandle(){
        return stmtHandle;
    }
}
