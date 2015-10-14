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
import java.util.concurrent.*;

import org.apache.hadoop.conf.Configuration;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.serverDriverInputOutput.*;
import org.trafodion.dcs.servermt.serverHandler.*;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;

public class TrafConnection {
    private static final Log LOG = LogFactory.getLog(TrafConnection.class);
    private String serverWorkerName = "";
    private Configuration conf = null;
    private Properties prop = null;
    private Connection conn = null;
    private boolean isClosed = true;

    private String datasource = "";
    private String catalog = "";
    private String schema = "";
    private String location = "";
    private String userRole = "";

    private short accessMode = 0;
    private short autoCommit = 0;
    private int queryTimeoutSec = 0;
    private int idleTimeoutSec = 0;
    private int loginTimeoutSec = 0;
    private short txnIsolationLevel = 0;
    private short rowSetSize = 0;

    private int diagnosticFlag = 0;
    private int processId = 0;

    private String computerName = "";
    private String windowText = "";

    private int ctxACP = 0;
    private int ctxDataLang = 0;
    private int ctxErrorLang = 0;
    private short ctxCtrlInferNXHAR = 0;

    private short cpuToUse = 0;
    private short cpuToUseEnd = 0;

    private String connectOptions = "";

    private VersionList clientVersionList = null;

    private short clientComponentId = 0;
    private short clientMajorVersion = 0;
    private short clientMinorVersion = 0;
    private int clientBuildId = 0;

    private int dialogueId = 0;
    private long contextOptions1 = 0L;
    private long contextOptions2 = 0L;

    private String sessionName = "";
    private String clientUserName = "";
    // ----------------------------------------------------------------
    private int batchBinding = 500;
    // character set information
    private int isoMapping = 15;
    private int termCharset = 15;
    private boolean enforceISO = false;
//    private Configuration conf = DcsConfiguration.create();

    //
    // --------------------------------------------------------------
    //
    private ConcurrentHashMap<String, TrafStatement> statements = new ConcurrentHashMap<String, TrafStatement>(); // keeping
                                                                                                                  // statements

    public TrafConnection() {
        init();
    }

    public TrafConnection(String serverWorkerName, ClientData clientData,
            ConnectionContext cc) throws SQLException, ClassNotFoundException {
        init();
        this.serverWorkerName = serverWorkerName;
        datasource = cc.getDatasource();
        catalog = cc.getCatalog();
        schema = cc.getSchema();
        location = cc.getLocation();
        userRole = cc.getUserRole();
        accessMode = cc.getAccessMode();
        autoCommit = cc.getAutoCommit();
        queryTimeoutSec = cc.getQueryTimeoutSec();
        idleTimeoutSec = cc.getIdleTimeoutSec();
        loginTimeoutSec = cc.getLoginTimeoutSec();
        txnIsolationLevel = cc.getTxnIsolationLevel();
        rowSetSize = cc.getRowSetSize();
        diagnosticFlag = cc.getDiagnosticFlag();
        processId = cc.getProcessId();
        computerName = cc.getComputerName();
        windowText = cc.getWindowText();
        ctxACP = cc.getCtxACP();
        ctxDataLang = cc.getCtxDataLang();
        ctxErrorLang = cc.getCtxErrorLang();
        ctxCtrlInferNXHAR = cc.getCtxCtrlInferNXHAR();
        cpuToUse = cc.getCpuToUse();
        cpuToUseEnd = cc.getCpuToUseEnd();
        connectOptions = cc.getConnectOptions();
        clientVersionList = cc.getClientVersionList();
        dialogueId = cc.getDialogueId();
        contextOptions1 = cc.getContextOptions1();
        contextOptions2 = cc.getContextOptions2();
        sessionName = cc.getSessionName();
        clientUserName = cc.getClientUserName();

        Version[] cvl = clientVersionList.getList();
        for (int i = 0; i < cvl.length; i ++ ){
            Version cv = cvl[i];
            if (i == 0){
                clientComponentId = cv.getComponentId();
                clientMajorVersion = cv.getMajorVersion();
                clientMinorVersion = cv.getMinorVersion();
                clientBuildId = cv.getBuildId();
            }
            if (LOG.isDebugEnabled()) {
                LOG.debug(serverWorkerName + ". version index :" + 0);
                cv.debug();
            }
        }

        /*--------------------------------------------------------------------
         T2 Driver properties
         catalog
         schema
         batchBinding
         language
         mploc
         sql_nowait
         Spjrs
         stmtatomicity
         transactionMode
         ISO88591
         contBatchOnError
         maxIdleTime
         maxPoolSize
         minPoolSize
         maxStatements
         initialPoolSize
         blobTableName
         clobTableName
         enableMFC
         compileModuleLocation
         traceFlag
         traceFile
         externalCallHandler
         externalCallPrefix
         queryExecutionTime
         T2QueryExecuteLogFile
         enableLog
         idMapFile
         */
        conf = clientData.getConf();
        prop = new Properties();
        prop.setProperty("catalog", catalog);
        prop.setProperty("schema", schema);
// Publication Properties
        int statisticsIntervalTime = conf.getInt(Constants.DCS_SERVER_USER_PROGRAM_STATISTICS_INTERVAL_TIME,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_INTERVAL_TIME);
        int statisticsLimitTime = conf.getInt(Constants.DCS_SERVER_USER_PROGRAM_STATISTICS_LIMIT_TIME,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_LIMIT_TIME);
        String statisticsType = conf.get(Constants.DCS_SERVER_USER_PROGRAM_STATISTICS_TYPE,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_TYPE);
        String statisticsEnable = conf.get(Constants.DCS_SERVER_USER_PROGRAM_STATISTICS_ENABLE,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_ENABLE);
        String sqlplanEnable = conf.get(Constants.DCS_SERVER_USER_PROGRAM_STATISTICS_SQLPLAN_ENABLE,Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_SQLPLAN_ENABLE);
        if(LOG.isDebugEnabled()){
            LOG.debug(serverWorkerName + ". Publication properties : statisticsIntervalTime :" + statisticsIntervalTime + " statisticsLimitTime :" + statisticsLimitTime + " statisticsType :" + statisticsType +
                    " statisticsEnable :" + statisticsEnable + " sqlplanEnable : " + sqlplanEnable);
        }

        prop.setProperty(Constants.PROPERTY_STATISTICS_INTERVAL_TIME, Integer.toString(statisticsIntervalTime));
        prop.setProperty(Constants.PROPERTY_STATISTICS_LIMIT_TIME, Integer.toString(statisticsLimitTime));
        prop.setProperty(Constants.PROPERTY_STATISTICS_TYPE, statisticsType);
        prop.setProperty(Constants.PROPERTY_PROGRAM_STATISTICS_ENABLE, statisticsEnable);
        prop.setProperty(Constants.PROPERTY_STATISTICS_SQLPLAN_ENABLE, sqlplanEnable);

        String traceFile = conf.get(Constants.T2_DRIVER_TRACE_FILE,
                Constants.DEFAULT_T2_DRIVER_TRACE_FILE);
        prop.setProperty("traceFile", traceFile);
        String traceFlag = conf.get(Constants.T2_DRIVER_TRACE_FLAG,
                Constants.DEFAULT_T2_DRIVER_TRACE_FLAG);
        prop.setProperty("traceFlag", traceFlag);

        // prop.put("batchBinding", batchBinding);
        if (LOG.isDebugEnabled()) {
            LOG.debug(serverWorkerName + ". catalog :" + catalog + " schema :"
                    + schema);
            String[] envs = { "LD_LIBRARY_PATH", "LD_PRELOAD" };
            for (String env : envs) {
                String value = System.getenv(env);
                if (value != null) {
                    LOG.debug(serverWorkerName + ". " + env + " = " + value);
                } else {
                    LOG.debug(serverWorkerName + ". " + env
                            + " is not assigned.");
                }
            }

            LOG.debug(serverWorkerName + ". jdbcT2 properties = " + prop.toString());
        }
        Class.forName(Constants.T2_DRIVER_CLASS_NAME);
        conn = DriverManager.getConnection(Constants.T2_DRIVER_URL, prop);
        if (conn.isClosed() == false) {
            isClosed = false;
            if (LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". T2 connection is open.");
        } else {
            isClosed = true;
            if (LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". T2 connection is close.");
        }

        // isoMapping, termCharset and enforceISO must be set by properties?
        if (isoMapping == SqlUtils.getCharsetValue("ISO8859_1")) {
            setTerminalCharset(SqlUtils.getCharsetValue("ISO8859_1"));
            this.ctxDataLang = 0;
            this.ctxErrorLang = 0;
        } else {
            setTerminalCharset(SqlUtils.getCharsetValue("UTF-8"));
        }
    }

    void init() {
        reset();
    }

    void reset() {
        prop = null;
        conn = null;

        datasource = "";
        catalog = "";
        schema = "";
        location = "";
        userRole = "";
        accessMode = 0;
        autoCommit = 0;
        queryTimeoutSec = 0;
        idleTimeoutSec = 0;
        loginTimeoutSec = 0;
        txnIsolationLevel = 0;
        rowSetSize = 0;
        diagnosticFlag = 0;
        processId = 0;
        computerName = "";
        windowText = "";
        ctxACP = 0;
        ctxDataLang = 0;
        ctxErrorLang = 0;
        ctxCtrlInferNXHAR = 0;
        cpuToUse = 0;
        cpuToUseEnd = 0;
        connectOptions = "";
        VersionList clientVersionList = new VersionList();
        dialogueId = 0;
        contextOptions1 = 0L;
        contextOptions2 = 0L;
        sessionName = "";
        clientUserName = "";
    }

    public void closeTConnection() {
        TrafStatement tstmt;
        try {
            Iterator<String> keySetIterator = statements.keySet().iterator();
            if (LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". closeTConnection");
            while (keySetIterator.hasNext()) {
                String key = keySetIterator.next();
                tstmt = statements.get(key);
                tstmt.closeTStatement();
            }
            statements.clear();
                if (isClosed == false) {
                    isClosed = true;
                    conn.close();
                }
        } catch (SQLException sql) {
        }
        reset();
    }

    public TrafStatement createTrafStatement(String stmtLabel, int sqlStmtType, int stmtHandle) throws SQLException {
        TrafStatement trafStatement = null;

        if (statements.containsKey(stmtLabel) == false) {
            if (LOG.isDebugEnabled())
                LOG.debug(serverWorkerName
                        + ". createTrafStatement.containsKey [" + stmtLabel + "] is false ");
            trafStatement = new TrafStatement(serverWorkerName, stmtLabel, conn, null, sqlStmtType);
            statements.put(stmtLabel, trafStatement);
        } else {
            LOG.debug(serverWorkerName + ". createTrafStatement.containsKey [" + stmtLabel + "] is found ");
            trafStatement = getTrafStatement(stmtLabel, stmtHandle);
            trafStatement.setStatement(conn, null, sqlStmtType);
        }
        return trafStatement;
    }

    public TrafStatement prepareTrafStatement(String stmtLabel, String sqlString, int sqlStmtType) throws SQLException {
        TrafStatement trafStatement = null;

        if (statements.containsKey(stmtLabel) == false) {
            if (LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". prepareTrafStatement.containsKey [" + stmtLabel + "] is false ");
            trafStatement = new TrafStatement(serverWorkerName, stmtLabel, conn, sqlString, sqlStmtType);
            statements.put(stmtLabel, trafStatement);
        } else {
            if (LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". prepareTrafStatement.containsKey [" + stmtLabel + "] is found ");
            trafStatement = getTrafStatement(stmtLabel, 0);
            trafStatement.setStatement(conn, sqlString, sqlStmtType);
        }
        return trafStatement;
    }

    public TrafStatement closeTrafStatement(String stmtLabel)
            throws SQLException {
        TrafStatement trafStatement = null;

        if (statements.containsKey(stmtLabel) == false) {
            if (LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". closeTrafStatement.containsKey [" + stmtLabel + "] is false ");
        } else {
            if (LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". closeTrafStatement.containsKey [" + stmtLabel + "] is found ");
            trafStatement = getTrafStatement(stmtLabel, 0);
            trafStatement.closeTStatement();
        }
        return trafStatement;
    }

    public TrafStatement getTrafStatement(String stmtLabel, int stmtHandle) throws SQLException {
        TrafStatement trafStatement = null;
        if (stmtLabel != null && stmtLabel.isEmpty() == false){
            trafStatement = statements.get(stmtLabel);
            if (trafStatement == null)
                throw new SQLException("getTrafStatement stmtLabel : [" + stmtLabel + "] returns null");
            return trafStatement;
        } else if (stmtHandle == 0)
            throw new SQLException("getTrafStatement stmtHandle is 0 [" + stmtHandle + "]");
        else {
            TrafStatement  tstmt;
            String key;
            Iterator<String> keySetIterator = statements.keySet().iterator();
            while (keySetIterator.hasNext()) {
                key = keySetIterator.next();
                tstmt = statements.get(key);
                if (tstmt.getStmtHandle() == stmtHandle);
                    return tstmt;
            }
            throw new SQLException("getTrafStatement stmtHandle [" + stmtHandle + "] returns null");
       }
    }

    public void setDatasource(String datasource) {
        this.datasource = datasource;
    }

    public void setCatalog(String catalog) {
        this.catalog = catalog;
    }

    public void setSchema(String schema) {
        this.schema = schema;
    }

    public void setLocation(String location) {
        this.location = location;
    }

    public void setUserRole(String userRole) {
        this.userRole = userRole;
    }

    public void setAccessMode(short accessMode) {
        this.accessMode = accessMode;
    }

    public void setAutoCommit(short autoCommit) {
        this.autoCommit = autoCommit;
    }

    public void setQueryTimeoutSec(int queryTimeoutSec) {
        this.queryTimeoutSec = queryTimeoutSec;
    }

    public void setIdleTimeoutSec(int idleTimeoutSec) {
        this.idleTimeoutSec = idleTimeoutSec;
    }

    public void setLoginTimeoutSec(int loginTimeoutSec) {
        this.loginTimeoutSec = loginTimeoutSec;
    }

    public void setTxnIsolationLevel(short txnIsolationLevel) {
        this.txnIsolationLevel = txnIsolationLevel;
    }

    public void setRowSetSize(short rowSetSize) {
        this.rowSetSize = rowSetSize;
    }

    public void setDiagnosticFlag(int diagnosticFlag) {
        this.diagnosticFlag = diagnosticFlag;
    }

    public void setProcessId(int processId) {
        this.processId = processId;
    }

    public void setComputerName(String computerName) {
        this.computerName = computerName;
    }

    public void setWindowText(String windowText) {
        this.windowText = windowText;
    }

    public void setCtxACP(int ctxACP) {
        this.ctxACP = ctxACP;
    }

    public void setCtxDataLang(int ctxDataLang) {
        this.ctxDataLang = ctxDataLang;
    }

    public void setCtxErrorLang(int ctxErrorLang) {
        this.ctxErrorLang = ctxErrorLang;
    }

    public void setCtxCtrlInferNXHAR(short ctxCtrlInferNXHAR) {
        this.ctxCtrlInferNXHAR = ctxCtrlInferNXHAR;
    }

    public void setCpuToUse(short cpuToUse) {
        this.cpuToUse = cpuToUse;
    }

    public void setCpuToUseEnd(short cpuToUseEnd) {
        this.cpuToUseEnd = cpuToUseEnd;
    }

    public void setConnectOptions(String connectOptions) {
        this.connectOptions = connectOptions;
    }

    public void setClientVersionList(VersionList clientVersionList) {
        this.clientVersionList = clientVersionList;
    }

    public void setDialogueId(int dialogueId) {
        this.dialogueId = dialogueId;
    }

    public void setContextOptions1(long contextOptions1) {
        this.contextOptions1 = contextOptions1;
    }

    public void setContextOptions2(long contextOptions2) {
        this.contextOptions2 = contextOptions2;
    }

    public void setSessionName(String sessionName) {
        this.sessionName = sessionName;
    }

    public void setClientUserName(String clientUserName) {
        this.clientUserName = clientUserName;
    }

    public void setConnection(Connection conn) {
        this.conn = conn;
    }

    public void setISOMapping(int isoMapping) {
        this.isoMapping = isoMapping;
    }

    public void setTerminalCharset(int termCharset) {
        this.termCharset = termCharset;
    }

    public void setEnforceISO(boolean enforceISO) {
        this.enforceISO = enforceISO;
    }

    // =============================================================================
    public String getDatasource() {
        return datasource;
    }

    public String getCatalog() {
        return catalog;
    }

    public String getSchema() {
        return schema;
    }

    public String getLocation() {
        return location;
    }

    public String getUserRole() {
        return userRole;
    }

    public short getAccessMode() {
        return accessMode;
    }

    public short getAutoCommit() {
        return autoCommit;
    }

    public int getQueryTimeoutSec() {
        return queryTimeoutSec;
    }

    public int getIdleTimeoutSec() {
        return idleTimeoutSec;
    }

    public int getLoginTimeoutSec() {
        return loginTimeoutSec;
    }

    public short getTxnIsolationLevel() {
        return txnIsolationLevel;
    }

    public short getRowSetSize() {
        return rowSetSize;
    }

    public int getDiagnosticFlag() {
        return diagnosticFlag;
    }

    public int getProcessId() {
        return processId;
    }

    public String getComputerName() {
        return computerName;
    }

    public String getWindowText() {
        return windowText;
    }

    public int getCtxACP() {
        return ctxACP;
    }

    public int getCtxDataLang() {
        return ctxDataLang;
    }

    public int getCtxErrorLang() {
        return ctxErrorLang;
    }

    public short getCtxCtrlInferNXHAR() {
        return ctxCtrlInferNXHAR;
    }

    public short getCpuToUse() {
        return cpuToUse;
    }

    public short getCpuToUseEnd() {
        return cpuToUseEnd;
    }

    public String getConnectOptions() {
        return connectOptions;
    }

    public VersionList getClientVersionList() {
        return clientVersionList;
    }

    public int getDialogueId() {
        return dialogueId;
    }

    public long getContextOptions1() {
        return contextOptions1;
    }

    public long getContextOptions2() {
        return contextOptions2;
    }

    public String getSessionName() {
        return sessionName;
    }

    public String getClientUserName() {
        return clientUserName;
    }

    public short getClientComponentId(){
        return clientComponentId;
    }

    public short getClientMajorVersion(){
        return clientMajorVersion;
    }

    public short getClientMinorVersion(){
        return clientMinorVersion;
    }

    public int getClientBuildId(){
        return clientBuildId;
    }

    public Connection getConnection() {
        return conn;
    }

    public int getISOMapping(int isoMapping) {
        return isoMapping;
    }

    public int getTerminalCharset() {
        return termCharset;
    }

    public boolean setEnforceISO() {
        return enforceISO;
    }

    public void commit() throws SQLException {
        if (conn != null && conn.isClosed() == false) {
            if (LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". commit.");
            conn.commit();
        }
    }

    public void rollback() throws SQLException {
        if (conn != null && conn.isClosed() == false) {
            if (LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". rollback.");
            conn.rollback();
        }
    }

    public void setAutoCommit(int option) throws SQLException {
        boolean autoCommit = true;
        if (conn != null && conn.isClosed() == false) {
            if (option == 0)
                autoCommit = false;
            if (LOG.isDebugEnabled()) {
                if (autoCommit == false)
                    LOG.debug(serverWorkerName + ". setAutoCommit off.");
                else
                    LOG.debug(serverWorkerName + ". setAutoCommit on.");
            }
            conn.setAutoCommit(autoCommit);
        }
    }
}
