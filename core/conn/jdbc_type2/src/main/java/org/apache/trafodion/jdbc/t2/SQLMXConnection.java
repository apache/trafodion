// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

/* -*-java-*-
 * Filename	: SQLMXConnection.java
 * Description :
 */
package org.apache.trafodion.jdbc.t2;

import java.io.FileOutputStream;
import java.io.PrintWriter;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Modifier;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.sql.Array;
import java.sql.Blob;
import java.sql.CallableStatement;
import java.sql.Clob;
import java.sql.DatabaseMetaData;
import java.sql.NClob;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLClientInfoException;
import java.sql.SQLException;
import java.sql.SQLXML;
import java.sql.Savepoint;
import java.sql.Statement;
import java.sql.Struct;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Locale; // JDK 1.2
import java.util.Map;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.Executor;

import javax.sql.PooledConnection;
//import com.tandem.tmf.Current;		// Linux port - ToDo
//import com.tandem.util.FSException;	// Linux port - ToDo

public class SQLMXConnection extends PreparedStatementManager implements
java.sql.Connection {

private static final short TYPE_UNKNOWN = 0;
private static final short TYPE_SELECT = 0x0001;
private static final short TYPE_UPDATE = 0x0002;
private static final short TYPE_DELETE = 0x0004;
private static final short TYPE_INSERT = 0x0008;
private static final short TYPE_INSERT_PARAM = 0x0120; //Modified for CQDs filter from 0x0100 to 0x0120
private static final short TYPE_EXPLAIN = 0x0010;
private static final short TYPE_CREATE = 0x0020;
private static final short TYPE_GRANT = 0x0040;
private static final short TYPE_DROP = 0x0080;
private static final short TYPE_CALL = 0x0800;
    static final short TYPE_CONTROL = 0x0900;

private static CallableStatement createExternalCallableStatement(
            String className, String call) throws SQLException {
        Class classX = null;

        /*
         * Check if the Class exists in the Class path.
         */
        try {
            classX = Class.forName(className);
        } catch (ClassNotFoundException e) {
            throw new SQLException("Specified Class not found: "
                    + e.getLocalizedMessage());
        }

        /*
         * Check if the Class implements the CallableStatement interface.
         */
        boolean blnIsCallableStatement = false;

        if (classX != null) {
            Class[] interfaces = classX.getInterfaces();
            for (int nfor = 0; nfor < interfaces.length; ++nfor) {
                if (interfaces[nfor].getName().equals(
                                CallableStatement.class.getName())) {
                    blnIsCallableStatement = true;
                    break;
                }
            }
        }
        if (!blnIsCallableStatement) {
            throw new SQLException(
                    "The Specified Class does not implement java.sql.CallableStatement interface.");
        }

        Constructor[] array = null;

        if (classX != null) {
            array = classX.getDeclaredConstructors();
        }

        if (array != null) {

            boolean blnPublic = false;

            for (int nfor = 0; nfor < array.length; ++nfor) {
                if (array[nfor].getModifiers() == Modifier.PUBLIC) {
                    blnPublic = true;
                    Class[] params = array[nfor].getParameterTypes();
                    if (params != null) {
                        if (params.length == 1) {
                            if (params[0].getName().equals(
                                            String.class.getName())) {
                                Object initVar[] = new Object[1];
                                initVar[0] = call;
                                Object o = null;
                                try {
                                    o = array[nfor].newInstance(initVar);
                                } catch (IllegalArgumentException e) {
                                    throw new SQLException(e.getMessage());
                                } catch (InstantiationException e) {
                                    throw new SQLException(e.getMessage());
                                } catch (IllegalAccessException e) {
                                    throw new SQLException(e.getMessage());
                                } catch (InvocationTargetException e) {
                                    throw new SQLException(e.getMessage());
                                }
                                return (CallableStatement) o;
                            }
                        }
                    }
                }
            }
            if (!blnPublic) {
                throw new SQLException(
                        "No Public Constructors available in the Specified Class.");
            }
        } else {
            throw new SQLException(
                    "No Constructors available in the Specified Class.");
        }
        throw new SQLException(
                "No Constructor available accepting ONLY java.lang.String parameter in the Specified Class.");
    }

    // java.sql.Connection interface methods
public void close() throws SQLException {
        if (this.getTracer() != null)
        this.getTracer().println(getTraceId() + "close()");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_close_V].methodEntry(JdbcDebug.debugLevelPooling);
        try {
            // if it is a pooledConnection, don't do hardClose (Close SQL/MX
            // Resources)
            if (pc_ != null)
            close(false, true);
            else
            close(true, true);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_close_V].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization commit() is now synchronized
     */
public synchronized void commit() throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "commit()");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_commit].methodEntry();
        try {
            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            if (autoCommit_)
            throw Messages.createSQLException(locale_,
                    "invalid_commit_mode", null);
            if (beginTransFlag_) {
                /*
                 * Transaction was started using Connection.begintransaction()
                 * API, set the autoCommit_ flag to true.
                 */
                autoCommit_ = true;
                beginTransFlag_ = false;
            }
            Statement cs = null;
            try {
                // commit the Transaction
                cs = this.createStatement();
                cs.execute("commit");
            } catch (SQLException se) {
                if (se.getErrorCode() != -8605)
                throw se;
            }finally {
                setTxid_(0);
                if(cs != null ) {
                    try {cs.close();} catch(Exception ce) {}
                }
            }
        }finally {
            if (JdbcDebugCfg.traceActive)
            debug[methodId_commit].traceOut(JdbcDebug.debugLevelEntry,
                    "beginTransFlag_ = " + beginTransFlag_
                    + "; autoCommit_ = " + autoCommit_
                    + "; txid_ = " + getTxid());
            if (JdbcDebugCfg.entryActive)
            debug[methodId_commit].methodExit();
        }
    }

public Statement createStatement() throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "createStatement()");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_createStatement].methodEntry();
        try {
            SQLMXStatement stmt = null;

            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            connectInit();
            stmt = new SQLMXStatement(this);
            if (out_ != null)
            out_.println(getTraceId() + "createStatement() returns Statement ["
                    + stmt.getStmtLabel_() + "]");
            if (out_ != null) {
                return new TStatement(stmt, out_);
            }
            return stmt;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_createStatement].methodExit();
        }
    }

public Statement createStatement(int resultSetType, int resultSetConcurrency)
    throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "createStatement(" + resultSetType + ","
                + resultSetConcurrency + ")");
        if (JdbcDebugCfg.entryActive)
        debug[methodId_createStatement_II].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_createStatement_II]
        .methodParameters("resultSetType=" + resultSetType
                + ", resultSetConcurrency=" + resultSetConcurrency);
        try {
            SQLMXStatement stmt = null;

            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            connectInit();
            stmt = new SQLMXStatement(this, resultSetType, resultSetConcurrency);
            if (out_ != null)
            out_.println(getTraceId() + "createStatement(" + resultSetType + ","
                    + resultSetConcurrency + ") returns Statement ["
                    + ((SQLMXStatement) stmt).getStmtLabel_() + "]");
            if (out_ != null) {
                return new TStatement(stmt, out_);
            }
            return stmt;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_createStatement_II].methodExit();
        }
    }

public Statement createStatement(int resultSetType,
            int resultSetConcurrency, int resultSetHoldability)
    throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "createStatement(" + resultSetType + ","
                + resultSetConcurrency + "," + resultSetHoldability + ")");
        if (JdbcDebugCfg.entryActive)
        debug[methodId_createStatement_III].methodEntry();
        try {
            SQLMXStatement stmt = null;

            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            connectInit();
            stmt = new SQLMXStatement(this, resultSetType,
                    resultSetConcurrency, resultSetHoldability);
            if (out_ != null)
            out_.println(getTraceId() + "createStatement(" + resultSetType + ","
                    + resultSetConcurrency + "," + resultSetHoldability
                    + ") returns Statement ["
                    + ((SQLMXStatement) stmt).getStmtLabel_() + "]");
            if (out_ != null) {
                return new TStatement(stmt, out_);
            }
            return stmt;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_createStatement_III].methodExit();
        }
    }

public boolean getAutoCommit() throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "getAutoCommit()");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_getAutoCommit].methodEntry();
        try {
            clearWarnings();
            return autoCommit_;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getAutoCommit].methodExit();
        }
    }

public String getCatalog() throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "getCatalog()");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_getCatalog].methodEntry();
        try {
            clearWarnings();
            Statement s = null;
            ResultSet rs = null;
            String catalog = null;
            
            try {
                s = this.createStatement();
                rs = s.executeQuery("SHOWCONTROL DEFAULT CATALOG, match full, no header");
                rs.next();
                catalog = rs.getString(1);
                catalog = catalog.substring(catalog.indexOf('.') + 1);
            } catch (SQLException e) {
                return this.t2props.getCatalog();
            } finally {
                if (rs != null)
                    rs.close();
                if (s != null)
                    s.close();
            }
            return catalog;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getCatalog].methodExit();
        }
    }

public int getHoldability() throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "getCatalog()");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_getHoldability].methodEntry();
        try {
            clearWarnings();
            return holdability_;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getHoldability].methodExit();
        }
    }

public DatabaseMetaData getMetaData() throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "getMetaData()");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_getMetaData].methodEntry();
        try {
            SQLMXDatabaseMetaData metaData;

            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            metaData = new SQLMXDatabaseMetaData(this);
            if (out_ != null)
            out_.println(getTraceId() + "getMetaData() returns DatabaseMetaData ["
                    + System.identityHashCode(metaData) + "]");
            if(out_ != null) {
                return new TDatabaseMetaData(metaData, new TConnection(this, out_), out_);
            }
            return metaData;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getMetaData].methodExit();
        }
    }

public int getTransactionIsolation() throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "getTransactionIsolation()");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_getTransactionIsolation].methodEntry();
        try {
            clearWarnings();
            return transactionIsolation_;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getTransactionIsolation].methodExit();
        }
    }

    // JDK 1.2
public java.util.Map<String, Class<?>> getTypeMap() throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "getTypeMap()");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_getTypeMap].methodEntry();
        try {
            clearWarnings();
            return userMap_;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getTypeMap].methodExit();
        }
    }

public boolean isClosed() throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "isClosed()");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_isClosed].methodEntry();
        try {
            clearWarnings();
            return isClosed_;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_isClosed].methodExit();
        }
    }

public boolean isReadOnly() throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "isReadOnly()");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_isReadOnly].methodEntry();
        try {
            clearWarnings();
            return isReadOnly_;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_isReadOnly].methodExit();
        }
    }

public String nativeSQL(String sql) throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "nativeSQL(\"" + sql + "\")");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_nativeSQL].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_nativeSQL].methodParameters("sql=" + sql);
        try {
            clearWarnings();
            return sql;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_nativeSQL].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization prepareCall() is now synchronized
     */
public synchronized CallableStatement prepareCall(String sql)
    throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "prepareCall(\"" + sql + "\")");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_prepareCall_L].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_prepareCall_L].methodParameters("sql=" + sql);
        try {

            if (!this.t2props.getExternalCallHandler().equals("NONE")) {
                String sqlX = sql;
                // Set the prefix:
                String prefix = "EXT";
                String callHandler = "NONE";

                if (!this.t2props.getExternalCallHandler().equals("NONE")) {
//					callHandler = SQLMXDataSource.externalCallHandler;
                    callHandler = this.t2props.getExternalCallHandler();
                }

                if (!this.t2props.getExternalCallPrefix().equalsIgnoreCase("EXT")) {
                    prefix = this.t2props.getExternalCallPrefix();
                }

                if (sqlX.trim().startsWith("{")) {
                    sqlX = sqlX.trim().substring(1).trim();
                    if (sqlX.toUpperCase().startsWith(prefix.toUpperCase())) {
                        CallableStatement externalCallStmt = SQLMXConnection
                        .createExternalCallableStatement(callHandler,
                                sql);
                        return externalCallStmt;
                    }
                }
            }
            SQLMXCallableStatement cstmt;

            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            connectInit();
            gcStmts();
            if (isStatementCachingEnabled()) {
                cstmt = (SQLMXCallableStatement) getPreparedStatement(this,
                        sql.trim(), ResultSet.TYPE_FORWARD_ONLY,
                        ResultSet.CONCUR_READ_ONLY, holdability_);
                if (cstmt != null) {
                    if (out_ != null) {
                        out_.println(getTraceId() + "prepareCall(\"" + sql
                                + "\") returns CallableStatement ["
                                + cstmt.getStmtLabel_() + "]");
                        if (traceFlag_ >= T2Driver.POOLING_LVL) {
                            out_.println(getTraceId() + "prepareStatement(String,resultSetType,resultSetConcurrency,resultSetHoldability) "
                                    + cstmt.stmtId_+"\"+ GOT STMT FROM CACHE ");
                        }
                    }

                    if(out_ != null) {
                        return new TCallableStatement(cstmt, out_);
                    }
                    return cstmt;
                }
            }
            cstmt = new SQLMXCallableStatement(this, sql);

            pStmtCount++;
            if (out_ != null) {
                if (traceFlag_ >= T2Driver.POOLING_LVL) {
                    out_.println(getTraceId() + "prepareCall(String) "
                            + "PREPARED STMT - " + "\""
                            + cstmt.stmtId_ + "\"");
                    out_.println(getTraceId() + "prepareCall(String) "
                            + "\""+"PREPARED STMT COUNT:"+this.pStmtCount
                            + "\"");
                }
            }

            if (this.t2props.getEnableLog().equalsIgnoreCase("ON"))
            printIdMapEntry(cstmt);
            cstmt.prepareCall(server_, getDialogueId(), getTxid(),
                    autoCommit_, transactionMode_, cstmt.getStmtLabel_(),
                    cstmt.sql_.trim(), cstmt.queryTimeout_,
                    cstmt.resultSetHoldability_, cstmt.fetchSize_);
            if (isStatementCachingEnabled()) {
                addPreparedStatement(this, cstmt.sql_.trim(), cstmt,
                        ResultSet.TYPE_FORWARD_ONLY,
                        ResultSet.CONCUR_READ_ONLY, holdability_);
            }
            addElement(cstmt);
            if (out_ != null)
            out_.println(getTraceId() + "prepareCall(\"" + sql
                    + "\") returns CallableStatement ["
                    + cstmt.getStmtLabel_() + "]");
            if (out_ != null) {
                return new TCallableStatement(cstmt, out_);
            }
            return cstmt;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_prepareCall_L].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization prepareCall() is now synchronized
     */
public synchronized CallableStatement prepareCall(String sql,
            int resultSetType, int resultSetConcurrency) throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "prepareCall(\"" + sql + "\","
                + resultSetType + "," + resultSetConcurrency + ")");
        if (JdbcDebugCfg.entryActive)
        debug[methodId_prepareCall_LII].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_prepareCall_LII].methodParameters("sql=" + sql
                + ", resultSetType=" + resultSetType
                + ", resultSetConcurrency=" + resultSetConcurrency);
        try {
            SQLMXCallableStatement cstmt;

            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            connectInit();
            gcStmts();
            if (isStatementCachingEnabled()) {
                cstmt = (SQLMXCallableStatement) getPreparedStatement(this,
                        sql.trim(), resultSetType, resultSetConcurrency, holdability_);
                if (cstmt != null) {
                    if (out_ != null) {
                        out_.println(getTraceId() + "prepareCall(\"" + sql + "\","
                                + resultSetType + "," + resultSetConcurrency
                                + ") returns CallableStatement ["
                                + cstmt.getStmtLabel_() + "]");
                        if (traceFlag_ >= T2Driver.POOLING_LVL) {
                            out_.println(getTraceId() + "prepareCall(String,resultSetType,resultSetConcurrency) "
                                    + cstmt.stmtId_+"\" GOT STMT FROM CACHE ");
                        }
                    }
                    if(out_ != null) {
                        return new TCallableStatement(cstmt, out_);
                    }
                    return cstmt;

                }
            }
            cstmt = new SQLMXCallableStatement(this, sql, resultSetType,
                    resultSetConcurrency);

            pStmtCount++;
            if (out_ != null) {
                if (traceFlag_ >= T2Driver.POOLING_LVL) {
                    out_.println(getTraceId() + "prepareCall(String,resultSetType,resultSetConcurrency) "
                            + " PREPARED STMT - " + "\""
                            + cstmt.stmtId_ + "\"");
                    out_.println(getTraceId() + "prepareCall(String,resultSetType,resultSetConcurrency) "
                            + "\""+"PREPARED STMT COUNT:"+this.pStmtCount
                            + "\"");
                }
            }
            if (this.t2props.getEnableLog().equalsIgnoreCase("ON"))
            printIdMapEntry(cstmt);
            cstmt.prepareCall(server_, getDialogueId(), getTxid(),
                    autoCommit_, transactionMode_, cstmt.getStmtLabel_(),
                    cstmt.sql_.trim(), cstmt.queryTimeout_,
                    cstmt.resultSetHoldability_, cstmt.fetchSize_);
            if (isStatementCachingEnabled()) {
                addPreparedStatement(this, cstmt.sql_.trim(), cstmt,
                        resultSetType, resultSetConcurrency, holdability_);
            }
            addElement(cstmt);
            if (out_ != null)
            out_.println(getTraceId() + "prepareCall(\"" + sql + "\","
                    + resultSetType + "," + resultSetConcurrency
                    + ") returns CallableStatement ["
                    + cstmt.getStmtLabel_() + "]");
            if (out_ != null) {
                return new TCallableStatement(cstmt, out_);
            }
            return cstmt;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_prepareCall_LII].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization prepareCall() is now synchronized
     */
public synchronized CallableStatement prepareCall(String sql,
            int resultSetType, int resultSetConcurrency,
            int resultSetHoldability) throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "prepareCall(\"" + sql + "\","
                + resultSetType + "," + resultSetConcurrency + ","
                + resultSetHoldability + ")");
        if (JdbcDebugCfg.entryActive)
        debug[methodId_prepareCall_LIII].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_prepareCall_LIII].methodParameters("sql=" + sql
                + ", resultSetType=" + resultSetType
                + ", resultSetConcurrency=" + resultSetConcurrency
                + ", resultSetHoldability=" + resultSetHoldability);
        try {
            SQLMXCallableStatement cstmt;

            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            connectInit();
            gcStmts();
            if (isStatementCachingEnabled()) {
                cstmt = (SQLMXCallableStatement) getPreparedStatement(this, sql
                        .trim(), resultSetType, resultSetConcurrency,
                        resultSetHoldability);
                if (cstmt != null) {
                    if (out_ != null) {
                        out_.println(getTraceId() + "prepareCall(\"" + sql + "\","
                                + resultSetType + "," + resultSetConcurrency + ","
                                + resultSetHoldability + ") returns CallableStatement ["
                                + cstmt.getStmtLabel_() + "]");
                        if (traceFlag_ >= T2Driver.POOLING_LVL) {
                            out_.println(getTraceId() + "repareCall(String,resultSetType,resultSetConcurrency,resultSetHoldability)"
                                    + cstmt.stmtId_+"\"+ GOT STMT FROM CACHE ");
                        }
                    }

                    if(out_ != null) {
                        return new TCallableStatement(cstmt, out_);
                    }

                    return cstmt;
                }

            }
            cstmt = new SQLMXCallableStatement(this, sql, resultSetType,
                    resultSetConcurrency, resultSetHoldability);

            pStmtCount++;
            if (out_ != null) {
                if (traceFlag_ >= T2Driver.POOLING_LVL) {
                    out_.println(getTraceId() + "prepareCall(String,resultSetType,resultSetConcurrency,resultSetHoldability) "
                            + " PREPARED STMT - " + "\""
                            + cstmt.stmtId_ + "\"");
                    out_.println(getTraceId() + "prepareCall(String,resultSetType,resultSetConcurrency,resultSetHoldability) "
                            +"\""+"PREPARED STMT COUNT:"+this.pStmtCount
                            + "\"");
                }
            }

            if (this.t2props.getEnableLog().equalsIgnoreCase("ON"))
            printIdMapEntry(cstmt);
            cstmt.prepareCall(server_, getDialogueId(), getTxid(),
                    autoCommit_, transactionMode_, cstmt.getStmtLabel_(),
                    cstmt.sql_.trim(), cstmt.queryTimeout_,
                    cstmt.resultSetHoldability_, cstmt.fetchSize_);
            if (isStatementCachingEnabled()) {
                addPreparedStatement(this, cstmt.sql_.trim(), cstmt,
                        resultSetType, resultSetConcurrency,
                        resultSetHoldability);
            }
            addElement(cstmt);
            if (out_ != null)
            out_.println(getTraceId() + "prepareCall(\"" + sql + "\","
                    + resultSetType + "," + resultSetConcurrency + ","
                    + resultSetHoldability + ") returns CallableStatement ["
                    + cstmt.getStmtLabel_() + "]");
            if (out_ != null) {
                return new TCallableStatement(cstmt, out_);
            }
            return cstmt;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_prepareCall_LIII].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization prepareStatement() is now synchronized
     */
public synchronized PreparedStatement prepareStatement(String sql)
    throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "prepareStatement(\"" + sql + "\")");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_prepareStatement_L].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_prepareStatement_L].methodParameters("sql=" + sql);
        try {
            SQLMXPreparedStatement pstmt;

            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            connectInit();
            gcStmts();
            if (isStatementCachingEnabled()) {
                pstmt = (SQLMXPreparedStatement) getPreparedStatement(this,
                        sql.trim(), ResultSet.TYPE_FORWARD_ONLY,
                        ResultSet.CONCUR_READ_ONLY, holdability_);
                if (pstmt != null) {
                    if (out_ != null) {
                        out_.println(getTraceId() + "prepareStatement(\"" + sql
                                + "\") returns PreparedStatement ["
                                + (pstmt).getStmtLabel_() + "]");
                        if (traceFlag_ >= T2Driver.POOLING_LVL) {
                            out_.println(getTraceId() + "prepareStatement(String) "
                                    + pstmt.stmtId_+"\"+ GOT STMT FROM CACHE ");
                        }
                    }
                    if(out_ != null) {
                        return new TPreparedStatement(pstmt, out_);
                    }
                    return pstmt;
                }
            }
            pstmt = new SQLMXPreparedStatement(this, sql);

            pStmtCount++;
            if (out_ != null) {
                if (traceFlag_ >= T2Driver.POOLING_LVL) {
                    out_.println(getTraceId() + "prepareStatement(String) "
                            + " PREPARED STMT - " + "\""
                            + pstmt.stmtId_ + "\"");
                    out_.println(getTraceId() + "prepareStatement(String) "
                            +"\""+"PREPARED STMT COUNT:"+this.pStmtCount
                            + "\"");
                }
            }

            if (this.t2props.getEnableLog().equalsIgnoreCase("ON"))
            printIdMapEntry(pstmt);

            // not insert
            int batchBindingSizePrev = 0;
//			if (SQLMXConnection.getSqlStmtType(sql) != SQLMXConnection.TYPE_INSERT
//					&& SQLMXConnection.getSqlStmtType(sql) != SQLMXConnection.TYPE_INSERT_PARAM) {
//
//				batchBindingSizePrev = batchBindingSize_;
//				batchBindingSize_ = 0;
//			}
//
/*  Selva
            if ( pstmt.getSqlType() != SQLMXConnection.TYPE_INSERT
                    && pstmt.getSqlType() != SQLMXConnection.TYPE_INSERT_PARAM
                    && pstmt.getSqlType() != SQLMXConnection.TYPE_UPDATE
                    && pstmt.getSqlType() != SQLMXConnection.TYPE_DELETE
            )
            {

                batchBindingSizePrev = this.t2props.getBatchBinding();
                batchBindingSize_ = 0;
            }
*/

            pstmt.prepare(server_, getDialogueId(), getTxid(), autoCommit_,
                        pstmt.getStmtLabel_(), pstmt.sql_.trim(), pstmt.isSelect_,
                        pstmt.queryTimeout_, pstmt.resultSetHoldability_,
                        batchBindingSize_, pstmt.fetchSize_);

            // value
//			if (SQLMXConnection.getSqlStmtType(sql) != SQLMXConnection.TYPE_INSERT
//					&& SQLMXConnection.getSqlStmtType(sql) != SQLMXConnection.TYPE_INSERT_PARAM) {
//
//				batchBindingSize_ = batchBindingSizePrev;
//			}
/*
            if (pstmt.getSqlType() != SQLMXConnection.TYPE_INSERT
                    && pstmt.getSqlType() != SQLMXConnection.TYPE_INSERT_PARAM
                    && pstmt.getSqlType() != SQLMXConnection.TYPE_UPDATE
                    && pstmt.getSqlType() != SQLMXConnection.TYPE_DELETE
            )
            {
                batchBindingSize_ = batchBindingSizePrev;
            }
            // End
*/

            if (isStatementCachingEnabled()) {
                addPreparedStatement(this, pstmt.sql_.trim(), pstmt,
                        ResultSet.TYPE_FORWARD_ONLY,
                        ResultSet.CONCUR_READ_ONLY, holdability_);
            }
            addElement(pstmt);
            if (out_ != null)
            out_.println(getTraceId() + "prepareStatement(\"" + sql
                    + "\") returns PreparedStatement ["
                    + (pstmt).getStmtLabel_() + "]");
            if (out_ != null) {
                return new TPreparedStatement(pstmt, out_);
            }
            return pstmt;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_prepareStatement_L].methodExit();
        }
    }

public PreparedStatement prepareStatement(String sql, int autoGeneratedKeys)
    throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
                + autoGeneratedKeys + ")");
        if (JdbcDebugCfg.entryActive)
        debug[methodId_prepareStatement_LI].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_prepareStatement_LI].methodParameters("sql=" + sql
                + ", autoGeneratedKeys=" + autoGeneratedKeys);
        try {
            if (autoGeneratedKeys != SQLMXStatement.NO_GENERATED_KEYS)
            throw Messages.createSQLException(locale_,
                    "auto_generated_keys_not_supported", null);
            SQLMXPreparedStatement stmt = (SQLMXPreparedStatement) prepareStatement(sql);
            if (out_ != null)
            out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
                    + autoGeneratedKeys + ") returns PreparedStatement ["
                    + ((SQLMXStatement) stmt).getStmtLabel_() + "]");
            if (out_ != null) {
                return new TPreparedStatement(stmt, out_);
            }
            return stmt;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_prepareStatement_LI].methodExit();
        }
    }

public PreparedStatement prepareStatement(String sql, int[] columnIndexes)
    throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
                + columnIndexes + ")");
        if (JdbcDebugCfg.entryActive)
        debug[methodId_prepareStatement_LI_array].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_prepareStatement_LI_array]
        .methodParameters("sql=" + sql + ", int["
                + columnIndexes.length + "] columnIndexes");
        try {
            if (columnIndexes != null && columnIndexes.length > 0)
            throw Messages.createSQLException(locale_,
                    "auto_generated_keys_not_supported", null);
            SQLMXPreparedStatement stmt = (SQLMXPreparedStatement) prepareStatement(sql);
            if (out_ != null)
            out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
                    + columnIndexes + ") returns PreparedStatement ["
                    + ((SQLMXStatement) stmt).getStmtLabel_() + "]");
            if (out_ != null) {
                return new TPreparedStatement(stmt, out_);
            }
            return stmt;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_prepareStatement_LI_array].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization prepareStatement() is now synchronized
     */
public synchronized PreparedStatement prepareStatement(String sql,
            int resultSetType, int resultSetConcurrency) throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
                + resultSetType + "," + resultSetConcurrency + ")");
        if (JdbcDebugCfg.entryActive)
        debug[methodId_prepareStatement_LII].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_prepareStatement_LII].methodParameters("sql=" + sql
                + ", resultSetType=" + resultSetType
                + ", resultSetConcurrency=" + resultSetConcurrency);
        try {
            SQLMXPreparedStatement stmt;

            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            connectInit();
            gcStmts();
            if (isStatementCachingEnabled()) {
                stmt = (SQLMXPreparedStatement) getPreparedStatement(this, sql
                        .trim(), resultSetType, resultSetConcurrency,
                        holdability_);
                if (stmt != null) {
                    if (out_ != null) {
                        out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
                                + resultSetType + "," + resultSetConcurrency
                                + ") returns PreparedStatement ["
                                + ((SQLMXStatement) stmt).getStmtLabel_() + "]");
                        if (traceFlag_ >= T2Driver.POOLING_LVL) {
                            out_.println(getTraceId() + "prepareStatement(String,resultSetType,resultSetConcurrency) "
                                    + stmt.stmtId_+"\"+ GOT STMT FROM CACHE ");
                        }
                    }
                    if(out_ != null) {
                        return new TPreparedStatement(stmt, out_);
                    }
                    return stmt;
                }
            }
            stmt = new SQLMXPreparedStatement(this, sql, resultSetType,
                    resultSetConcurrency);

            pStmtCount++;
            if (out_ != null) {
                if (traceFlag_ >= T2Driver.POOLING_LVL) {
                    out_.println(getTraceId() + "prepareStatement(String,resultSetType,resultSetConcurrency) "
                            + " PREPARED STMT - " + "\""
                            + stmt.stmtId_ + "\"");
                    out_.println(getTraceId() + "prepareStatement(String,resultSetType,resultSetConcurrency) "
                            +"\""+"PREPARED STMT COUNT:"+this.pStmtCount
                            + "\"");
                }
            }

            if (this.t2props.getEnableLog().equalsIgnoreCase("ON"))
            printIdMapEntry(stmt);
/*
            // is not insert
            int batchBindingSizePrev = 0;
            if (stmt.getSqlType() != SQLMXConnection.TYPE_INSERT
                    && stmt.getSqlType() != SQLMXConnection.TYPE_INSERT_PARAM) {

                batchBindingSizePrev = this.t2props.getBatchBinding();
                batchBindingSize_ = 0;
            }
*/

                stmt.prepare(server_, getDialogueId(), getTxid(),
                        autoCommit_, stmt.getStmtLabel_(), stmt.sql_.trim(),
                        stmt.isSelect_, stmt.queryTimeout_,
                        stmt.resultSetHoldability_, batchBindingSize_,
                        stmt.fetchSize_);
/*
            if (stmt.getSqlType() != SQLMXConnection.TYPE_INSERT
                    && stmt.getSqlType() != SQLMXConnection.TYPE_INSERT_PARAM) {

                batchBindingSize_ = batchBindingSizePrev;
            }
*/
            if (isStatementCachingEnabled()) {
                addPreparedStatement(this, stmt.sql_.trim(), stmt,
                        resultSetType, resultSetConcurrency, holdability_);
            }
            addElement(stmt);
            if (out_ != null)
            out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
                    + resultSetType + "," + resultSetConcurrency
                    + ") returns PreparedStatement ["
                    + ((SQLMXStatement) stmt).getStmtLabel_() + "]");
            if (out_ != null) {
                return new TPreparedStatement(stmt, out_);
            }
            return stmt;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_prepareStatement_LII].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization prepareStatement() is now synchronized
     */
public synchronized PreparedStatement prepareStatement(String sql,
            int resultSetType, int resultSetConcurrency,
            int resultSetHoldability) throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
                + resultSetType + "," + resultSetConcurrency + ","
                + resultSetHoldability + ")");
        if (JdbcDebugCfg.entryActive)
        debug[methodId_prepareStatement_LIII].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_prepareStatement_LIII].methodParameters("sql=" + sql
                + ", resultSetType=" + resultSetType
                + ", resultSetConcurrency=" + resultSetConcurrency
                + ", resultSetHoldability=" + resultSetHoldability);
        try {
            SQLMXPreparedStatement stmt;

            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            connectInit();
            gcStmts();
            if (isStatementCachingEnabled()) {
                stmt = (SQLMXPreparedStatement) getPreparedStatement(this, sql
                        .trim(), resultSetType, resultSetConcurrency,
                        resultSetHoldability);
                if (stmt != null) {
                    if (out_ != null) {
                        out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
                                + resultSetType + "," + resultSetConcurrency + ","
                                + resultSetHoldability + ") returns PreparedStatement ["
                                + ((SQLMXStatement) stmt).getStmtLabel_() + "]");
                        if (traceFlag_ >= T2Driver.POOLING_LVL) {
                            out_.println(getTraceId() + "prepareStatement(String,resultSetType,resultSetConcurrency,resultSetHoldability) "
                                    + stmt.stmtId_+"\"+ GOT STMT FROM CACHE ");
                        }
                    }
                    if(out_ != null) {
                        return new TPreparedStatement(stmt, out_);
                    }
                    return stmt;
                }
            }
            stmt = new SQLMXPreparedStatement(this, sql, resultSetType,
                    resultSetConcurrency, resultSetHoldability);

            pStmtCount++;
            if (out_ != null) {
                if (traceFlag_ >= T2Driver.POOLING_LVL) {
                    out_.println(getTraceId() + "prepareStatement(String,resultSetType,resultSetConcurrency,resultSetHoldability) "
                            + " PREPARED STMT - " + "\""
                            + stmt.stmtId_ + "\"");
                    out_.println(getTraceId() + "prepareStatement(String,resultSetType,resultSetConcurrency,resultSetHoldability) "
                            +"\""+"PREPARED STMT COUNT:"+this.pStmtCount
                            + "\"");
                }
            }

            if (this.t2props.getEnableLog().equalsIgnoreCase("ON"))
            printIdMapEntry(stmt);
/*
            batchBindingSizePrev = this.t2props.getBatchBinding();
            // is not insert
            int batchBindingSizePrev = 0;
            if (stmt.getSqlType() != SQLMXConnection.TYPE_INSERT
                    &&stmt.getSqlType() != SQLMXConnection.TYPE_INSERT_PARAM) {

                batchBindingSizePrev = this.t2props.getBatchBinding();
                batchBindingSize_ = 0;
            }

*/
                stmt.prepare(server_, getDialogueId(), getTxid(),
                        autoCommit_, stmt.getStmtLabel_(), stmt.sql_.trim(),
                        stmt.isSelect_, stmt.queryTimeout_,
                        stmt.resultSetHoldability_, batchBindingSize_,
                        stmt.fetchSize_);
/*

            if (stmt.getSqlType() != SQLMXConnection.TYPE_INSERT
                    && stmt.getSqlType() != SQLMXConnection.TYPE_INSERT_PARAM) {

                batchBindingSize_ = batchBindingSizePrev;
            }
*/
            if (isStatementCachingEnabled()) {
                addPreparedStatement(this, stmt.sql_.trim(), stmt,
                        resultSetType, resultSetConcurrency,
                        resultSetHoldability);
            }
            addElement(stmt);
            if (out_ != null)
            out_.println(getTraceId() + "prepareStatement(\"" + sql + "\","
                    + resultSetType + "," + resultSetConcurrency + ","
                    + resultSetHoldability
                    + ") returns PreparedStatement ["
                    + ((SQLMXStatement) stmt).getStmtLabel_() + "]");
            if (out_ != null) {
                return new TPreparedStatement(stmt, out_);
            }
            return stmt;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_prepareStatement_LIII].methodExit();
        }
    }

public PreparedStatement prepareStatement(String sql, String[] columnNames)
    throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "prepareStatement(\"" + sql + "\")");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_prepareStatement_LL_array].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_prepareStatement_LL_array].methodParameters("sql="
                + sql + ", String[" + columnNames.length + "] columnNames");
        try {
            if (columnNames != null && columnNames.length > 0)
            throw Messages.createSQLException(locale_,
                    "auto_generated_keys_not_supported", null);
            SQLMXPreparedStatement stmt = (SQLMXPreparedStatement) prepareStatement(sql);
            if (out_ != null)
            out_.println(getTraceId() + "prepareStatement(\"" + sql
                    + "\") returns PreparedStatement ["
                    + ((SQLMXStatement) stmt).getStmtLabel_() + "]");
            if (out_ != null) {
                return new TPreparedStatement(stmt, out_);
            }
            return stmt;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_prepareStatement_LL_array].methodExit();
        }
    }

    //**************************************************************************
    // * Function: releaseSavepoint(Savepoint)
    // *
    // * Description: This is a required method for the JDBC 3.0 API. This
    // method
    // * takes a Savepoint object as a parameter and removes it from
    // * current transaction.
    // *
    // * Input: Savepoint object, one that was previous saved.
    // *
    // * NOTE: This is an unsupported function, calling it will cause an
    // * unsupported exception to be thown.
    // *
    //**************************************************************************
    // *

public void releaseSavepoint(Savepoint savepoint) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_releaseSavepoint].methodEntry();
        try {
            clearWarnings();
            Messages.throwUnsupportedFeatureException(locale_,
                    "releaseSavepoint()");
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_releaseSavepoint].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization rollback() is now synchronized.
     */
public synchronized void rollback() throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "rollback()");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_rollback_V].methodEntry();
        try {
            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            if (autoCommit_)
            throw Messages.createSQLException(locale_,
                    "invalid_commit_mode", null);

            if (beginTransFlag_) {
                /*
                 * Transaction was started using Connection.begintransaction()
                 * API, set the autoCommit_ flag to true.
                 */
                autoCommit_ = true;
                beginTransFlag_ = false;
            }
            Statement cs = null;
            try {
                // commit the Transaction
                cs = this.createStatement();
                cs.execute("rollback");
            } catch (SQLException se) {
                if (se.getErrorCode() != -8609)
                throw se;
            }finally {
                if(cs != null) {
                    try {cs.close();} catch(Exception ee) {}
                }
                setTxid_(0);
            }
        }finally {
            if (JdbcDebugCfg.traceActive)
            debug[methodId_rollback_V].traceOut(JdbcDebug.debugLevelEntry,
                    "beginTransFlag_ = " + beginTransFlag_
                    + "; autoCommit_ = " + autoCommit_
                    + "; txid_ = " + getTxid());
            if (JdbcDebugCfg.entryActive)
            debug[methodId_rollback_V].methodExit();
        }
    }

    //**************************************************************************
    // * Function: rollback(SavePoint)
    // *
    // * Description: This is a required method for the JDBC 3.0 API. This
    // method
    // * takes a Savepoint object as a parameter and roll back to when
    // * that Savepoint object was taken.
    // *
    // * Input: Savepoint object, one that was previous saved.
    // *
    // * NOTE: This is an unsupported function, calling it will cause an
    // * unsupported exception to be thown.
    // *
    //**************************************************************************
    // *

public void rollback(Savepoint savepoint) throws SQLException {
        if (out_ != null) {
            out_.println(getTraceId() + "rollback (" + savepoint + ")");
        }
        if (JdbcDebugCfg.entryActive)
        debug[methodId_rollback_L].methodEntry();
        try {
            clearWarnings();
            Messages.throwUnsupportedFeatureException(locale_,
                    "rollback(Savepoint)");
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_rollback_L].methodExit();
        }
    }

public void setAutoCommit(boolean autoCommit) throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "setAutoCommit(" + autoCommit + ")");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_setAutoCommit].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_setAutoCommit].methodParameters("autoCommit="
                + autoCommit);
        try {
            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_, "invalid_connection", null);
            if(this.getAutoCommit() == autoCommit) {
                return;
            }
            //changes to comply with standards, if autocommit mode is same then NO-OP
            // Don't allow autoCommit false when internal txn mode
            if (transactionMode_ == TXN_MODE_INTERNAL)
            autoCommit_ = true;
            else
            {
                if (autoCommit_ != autoCommit)
                {
                    if (connectInitialized_)
                    setAutoCommit(server_, getDialogueId(), autoCommit);
                    autoCommit_ = autoCommit;
                }
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setAutoCommit].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization setCatalog() is now synchronized.
     */
public synchronized void setCatalog(String catalog) throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "setCatalog(\"" + catalog + "\")");
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setCatalog].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_setCatalog].methodParameters("catalog=" + catalog);
        try {
            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            if (catalog != null) {
                setCatalog(server_, getDialogueId(), catalog);
                if (!catalog.startsWith("\""))
                catalog_ = catalog.toUpperCase();
                else
                catalog_ = catalog;
                updateConnectionReusability(SQL_SET_CATALOG);
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setCatalog].methodExit();
        }
    }

public void setHoldability(int holdability) throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "setHoldability(\"" + holdability + "\")");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_setHoldability].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_setHoldability].methodParameters("holdability="
                + holdability);
        try {
            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            if (holdability != SQLMXResultSet.HOLD_CURSORS_OVER_COMMIT
                    && holdability != SQLMXResultSet.CLOSE_CURSORS_AT_COMMIT)
            throw Messages.createSQLException(locale_,
                    "invalid_holdability", null);
            holdability_ = holdability;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setHoldability].methodExit();
        }
    }

public void setReadOnly(boolean readOnly) throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "setReadOnly(" + readOnly + ")");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_setReadOnly].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_setReadOnly]
        .methodParameters("readOnly=" + readOnly);
        try {
            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            // setReadOnly(server_, dialogueId_, readOnly);
            isReadOnly_ = readOnly;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setReadOnly].methodExit();
        }
    }

    //**************************************************************************
    // * Function: setSavepoint(String)
    // *
    // * Description: This is a required method for the JDBC 3.0 API. This
    // method
    // * takes a String object as a parameter and returns a Savepoint
    // * object.
    // *
    // * Input: String Object - Optional name of the setpoint.
    // * Returns: Setpoint Object - used to roll back to this point in time.
    // *
    // * NOTE: This is an unsupported function, calling it will cause an
    // * unsupported exception to be thown.
    // *
    //**************************************************************************
    // *

public Savepoint setSavepoint(String name) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setSavepoint_L].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_setSavepoint_L].methodParameters("name=" + name);
        try {
            clearWarnings();
            Messages.throwUnsupportedFeatureException(locale_, "setSavepoint");
            return null;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setSavepoint_L].methodExit();
        }
    }

    //**************************************************************************
    // * Function: setSavepoint()
    // *
    // * Description: This is a required method for the JDBC 3.0 API. This
    // method
    // * takes no parameter and returns a Savepoint object.
    // *
    // * Input: None.
    // * Returns: Setpoint Object - used to roll back to this point in time.
    // *
    // * NOTE: This is an unsupported function, calling it will cause an
    // * unsupported exception to be thown.
    // *
    //**************************************************************************
    // *

public Savepoint setSavepoint() throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_setSavepoint_V].methodEntry();
        try {
            clearWarnings();
            Messages.throwUnsupportedFeatureException(locale_, "setSavepoint");
            return null;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setSavepoint_V].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization setTransactionIsolation() is now
     * synchronized.
     */
public synchronized void setTransactionIsolation(int level)
    throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "setTransactionIsolation(" + level + ")");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_setTransactionIsolation].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_setTransactionIsolation].methodParameters("level="
                + level);
        try {
            clearWarnings();
            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            switch (level) {
                case TRANSACTION_NONE:
                case TRANSACTION_READ_UNCOMMITTED:
                case TRANSACTION_READ_COMMITTED:
                case TRANSACTION_REPEATABLE_READ:
                case TRANSACTION_SERIALIZABLE:
                // Check if connection is open
                setTransactionIsolation(server_, getDialogueId(),
                        mapTxnIsolation(level));
                transactionIsolation_ = level;
                updateConnectionReusability(SQL_SET_TRANSACTION);
                break;
                default:
                throw Messages.createSQLException(locale_,
                        "invalid_transaction_isolation", null);
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setTransactionIsolation].methodExit();
        }
    }

    // JDK 1.2
public void setTypeMap(java.util.Map<String, Class<?>> map)
    throws SQLException {
        if (out_ != null)
        out_.println(getTraceId() + "setTypeMap(" + map + ")");

        if (out_ != null)
        out_.println(getTraceId() + "setTypeMap(" + map + ")");

        if (JdbcDebugCfg.entryActive)
        debug[methodId_setTypeMap].methodEntry();
        try {
            clearWarnings();
            userMap_ = map;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_setTypeMap].methodExit();
        }
    }

public synchronized void begintransaction() throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_begintransaction].methodEntry();
        try {
            int txid;

            if (isClosed_)
            throw Messages.createSQLException(locale_,
                    "invalid_connection", null);
            if (getTxid() != 0)
            throw Messages.createSQLException(locale_,
                    "invalid_transaction_state", null);

            txid = beginTransaction(server_, getDialogueId());
            if (txid != 0) {
                setTxid_(txid);
                autoCommit_ = false;

                /*
                 * This flag was introduced to resolve the problem with BEGIN
                 * WORK COMMIT WORK and ROLLBACK WORK in SQLJ clause. The
                 * autoCommit flag set to false by SQLJ BEGIN WORK should to
                 * reset to true in SQLJ COMMIT WORK and ROLLBACK WORK.
                 */

                beginTransFlag_ = true;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_begintransaction].methodExit();
        }
    }

    // Other methods
private int mapTxnIsolation(int level) {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_mapTxnIsolation].methodEntry();
        if (JdbcDebugCfg.traceActive)
        debug[methodId_mapTxnIsolation].methodParameters("level=" + level);
        try {
            int isolationLevel;

            switch (level) {
                case TRANSACTION_NONE: // May be we default to SQL/MX default
                isolationLevel = SQL_TXN_READ_COMMITTED;
                break;
                case TRANSACTION_READ_COMMITTED:
                isolationLevel = SQL_TXN_READ_COMMITTED;
                break;
                case TRANSACTION_READ_UNCOMMITTED:
                isolationLevel = SQL_TXN_READ_UNCOMMITTED;
                break;
                case TRANSACTION_REPEATABLE_READ:
                isolationLevel = SQL_TXN_REPEATABLE_READ;
                break;
                case TRANSACTION_SERIALIZABLE:
                isolationLevel = SQL_TXN_SERIALIZABLE;
                break;
                default:
                isolationLevel = SQL_TXN_READ_COMMITTED;
                break;
            }
            return isolationLevel;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_mapTxnIsolation].methodExit();
        }
    }

    long getNextRSCount() {
        return ++rsCount_;
    }

    /*
     * RFE: Connection synchronization gcStmts()is now synchronized.
     */
    synchronized void gcStmts() {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_gcStmts].methodEntry();
        try {
            Reference pRef;
            //Venu changed stmtId from int to long for 64 bit
            long stmtId;
            Long stmtObject;

            while ((pRef = refQ_.poll()) != null) {
                Object obj = refToStmt_.get(pRef);
                if (obj != null) {
                    stmtObject = (Long)obj;
                    // All PreparedStatement objects are added to HashMap
                    // Only Statement objects that produces ResultSet are added
                    // to
                    // HashMap
                    // Hence stmtLabel could be null
                    if (stmtObject != null) {
                        stmtId = stmtObject.intValue();
                        try {
                            SQLMXStatement.close(server_, getDialogueId(),
                                    stmtId, true);
                        } catch (SQLException e) {
                        }finally {
                            refToStmt_.remove(pRef);
                        }
                    }
                }
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_gcStmts].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization removeElement()is now synchronized.
     */
    synchronized void removeElement(SQLMXStatement stmt) {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_removeElement_L].methodEntry();
        try {
            // if (stmt.pRef_.isEnqueued())
            // don't know what to do
            refToStmt_.remove(stmt.pRef_);
            stmt.pRef_.clear();
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_removeElement_L].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization addElement()is now synchronized.
     */
    synchronized void addElement(SQLMXStatement stmt) {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_addElement].methodEntry();
        try {
            //venu changed from Int to Long
            refToStmt_.put(stmt.pRef_, new Long(stmt.stmtId_));
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_addElement].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization connectInit() is now synchronized.
     */
    synchronized void connectInit() throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_connectInit].methodEntry();
        // Big Num Changes
        //Current tx = new Current();
        //int status =0;
        try {
            if (!connectInitialized_) {
                /*		status = tx.get_status();
                 if(status == com.tandem.tmf.Current.StatusNoTransaction){
                 tx.begin();
                 }*/
                try {
                    boolean blnCQD = System.getProperty(
                            "t2jdbc.cqdDoomUserTxn", "OFF").equalsIgnoreCase(
                            "ON")
                    || System.getProperty("cqdDoomUserTxn", "OFF")
                    .equalsIgnoreCase("ON");
                    connectInit(server_, getDialogueId(), catalog_, schema_,
                            isReadOnly_, autoCommit_,
                            mapTxnIsolation(transactionIsolation_),
                            loginTimeout_, queryTimeout_, blnCQD,
                            statisticsIntervalTime_, statisticsLimitTime_, statisticsType_, programStatisticsEnabled_, statisticsSqlPlanEnabled_
                    );

                    if (iso88591EncodingOverride_ != null)
                    setCharsetEncodingOverride(server_, getDialogueId(),
                            SQLMXDesc.SQLCHARSETCODE_ISO88591,
                            iso88591EncodingOverride_);
                } catch (SQLException e) {
                    /*	if(status == com.tandem.tmf.Current.StatusNoTransaction){
                     tx.rollback();
                     }*/
                    if (pc_ != null)
                    pc_.sendConnectionErrorEvent(e);
                    else
                    close(true, true);
                    throw e;
                }
                connectInitialized_ = true;
                /*	if(status == com.tandem.tmf.Current.StatusNoTransaction){
                 tx.rollback();
                 }*/
            }

        }
        /*catch (FSException e) {
         if(status == com.tandem.tmf.Current.StatusNoTransaction){
         try {
         tx.rollback();
         } catch (FSException e1) {
         }
         }
         }*/
        finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_connectInit].methodExit();

        }
    }

    void removeElement() {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_removeElement_V].methodEntry();
        try {
            if (ds_ != null) {
                ds_.weakConnection.removeElement(this);
                ds_ = null;
            } else if (pc_ != null) {
                pc_.removeElement(this);
                pc_ = null;
            } else {
                driver_.weakConnection.removeElement(this);
                driver_ = null;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_removeElement_V].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization close() is now synchronized.
     */
    synchronized void close(boolean hardClose, boolean sendEvents)
    throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_close_Z].methodEntry(JdbcDebug.debugLevelPooling);
        if (JdbcDebugCfg.traceActive)
        debug[methodId_close_Z].methodParameters("hardClose=" + hardClose
                + ", sendEvents=" + sendEvents);
        try {
            clearWarnings();

            if (!hardClose) {

                if (traceFlag_ >= T2Driver.POOLING_LVL) {
                    out_.println(getTraceId() + "close()"+" Connection logically closed");
                    out_.println(getTraceId() + "close(hardClose="+hardClose +",sendEvents="+sendEvents+")");
                }
                if (isClosed_)
                return;
                if (isStatementCachingEnabled()) {
                    Object[] csArray = null;
                    csArray = (prepStmtsInCache_.values()).toArray();
                    closePreparedStatementsAll();
                    //Venu changed the below line from int to Long for 64 bit
                    ArrayList<Long> stmtIds = new ArrayList<Long>(
                            refToStmt_.values());
                    //Venu changed the below line from Int to Long
                    ArrayList<Long> listOfStmtsClosed = new ArrayList<Long>();
                    /*
                     * List to store the Statement Id Pointer.
                     */
                    // venu changed the below line from Int to Long
                    ArrayList<Long> listOfCachedPrepStmtIds = new ArrayList<Long>();
                    for (int nfor = 0; nfor < csArray.length; ++nfor) {
                        listOfCachedPrepStmtIds
                        .add(((CachedPreparedStatement) csArray[nfor])
                                .getPstmt_().stmtId_);
                    }
                    csArray = null;
                    for (int i = 0; i < stmtIds.size(); i++) {
                        /*
                         * If the Statement is not found in
                         * CachedPreparedStatements. Then Close it.
                         */
                        if (!listOfCachedPrepStmtIds.contains(stmtIds.get(i))) {
                            try {
                                // getDialogueId()
                                SQLMXStatement.close(server_, getDialogueId(),
                                        ((Long) stmtIds.get(i)).intValue(),
                                        true);
                            } catch (SQLException se) {

                            }finally {
                                /*
                                 * Mark this Statement so that this Statement is
                                 * removed from the Statement References table.
                                 */
                                listOfStmtsClosed.add(stmtIds.get(i));
                            }
                        }
                    }
                    listOfCachedPrepStmtIds.clear();
                    /*
                     * Remove the Statement marked (already closed) from the
                     * Statement Reference table.
                     */
                    Set<WeakReference> ks = refToStmt_.keySet();
                    Iterator<WeakReference> iter = ks.iterator();
                    ArrayList<WeakReference> listOfPrefs = new ArrayList<WeakReference>();
                    int nfor = 0;
                    while (iter.hasNext()) {
                        WeakReference pRef = iter.next();
                        if (listOfStmtsClosed.contains(refToStmt_.get(pRef))) {
                            listOfPrefs.add(pRef);
                        }
                    }
                    for (nfor = 0; nfor < listOfPrefs.size(); ++nfor) {
                        refToStmt_.remove(listOfPrefs.get(nfor));
                    }
                    listOfPrefs.clear();
                    /*
                     * Clear off the Temp list.
                     */
                    listOfStmtsClosed.clear();
                } else {
                    // close all the statements
                    //venu changed the below line from Int to Long
                    ArrayList<Long> stmtIds = new ArrayList<Long>(
                            refToStmt_.values());
                    int size = stmtIds.size();
                    for (int i = 0; i < size; i++) {
                        try {
                            SQLMXStatement
                            .close(server_, getDialogueId(),
                                    ((Long) stmtIds.get(i))
                                    .longValue(), true);
                        } catch (SQLException se) {
                            // Ignore any exception and proceed to the next
                            // statement
                        }
                    }
                    refToStmt_.clear();
                }
                // Need to logically close the statement
                // Rollback the Transaction independent of autoCommit mode
                if (getTxid() != 0) {
                    try {
                        rollback(server_, getDialogueId(), getTxid());
                    } catch (SQLException se1) {
                    }
                    setTxid_(0);
                }
                pc_.logicalClose(sendEvents);
                isClosed_ = true;
            } else {

                if (traceFlag_ >= T2Driver.POOLING_LVL) {
                    out_.println(getTraceId() +"close()"+" Connection Hard closed");
                    out_.println(getTraceId() + "close(hardClose="+hardClose +",sendEvents="+sendEvents+")");
                }
                if (getDialogueId() == 0)
                return;
                // close all the statements
                Set<WeakReference> ks1 = refToStmt_.keySet();
                Iterator<WeakReference> iter1 = ks1.iterator();
                ArrayList<WeakReference> listOfStmts = new ArrayList<WeakReference> ();
                while (iter1.hasNext()) {
                    WeakReference pRef = iter1.next();
                    listOfStmts.add(pRef);
                }
                for(int nfor=0;nfor<listOfStmts.size();nfor++) {
                    SQLMXStatement st=(SQLMXStatement)(listOfStmts.get(nfor)).get();
                    if (st != null) {
                        try {
                            st.close();
                        }
                        catch(SQLException se) {
                            // Ignore any exception and proceed to the next
                            // statement
                        }
                    }
                }
                refToStmt_.clear();
                // Need to logically close the statement
                // Rollback the Transaction independent of autoCommit mode
                if (getTxid() != 0) {
                    try {
                        rollback(server_, getDialogueId(), getTxid());
                    } catch (SQLException se1) {
                    }
                    setTxid_(0);
                }
                if (isStatementCachingEnabled())
                clearPreparedStatementsAll();
                // Close the connection
                try {
                    // getDialogueId(),setDialogueI_()
                    SQLMXConnection.close(server_, getDialogueId());
                }finally {
                    isClosed_ = true;
                    keyForMap=this.incrementAndGetkeyForMapCounter();
                    mapOfClosedDialogs.put(keyForMap, getDialogueId());

                    setDialogueId(0);
                    removeElement();
                }
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_close_Z].methodExit();
        }
    }

    /*
     * RFE: Connection synchronization reuse() is now synchronized.
     */
    synchronized void reuse() throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_reuse].methodEntry(JdbcDebug.debugLevelPooling);
        try {
            if (connReuseBitMap_ != 0) {
                connectReuse(server_, getDialogueId(), connReuseBitMap_,
                        dsCatalog_, dsSchema_, 
                        TRANSACTION_READ_COMMITTED);
                // Reset all connection attribute values
                catalog_ = dsCatalog_;
                schema_ = dsSchema_;
                transactionIsolation_ = TRANSACTION_READ_COMMITTED;
                connReuseBitMap_ = 0;
            }
            batchBindingSize_ = dsBatchBindingSize_;
            autoCommit_ = true;
            isReadOnly_ = false;
            isClosed_ = false;
            setTxid_(0);
            transactionMode_ = dsTransactionMode_;
            iso88591EncodingOverride_ = dsIso88591EncodingOverride_;
            contBatchOnErrorval_ = dsContBatchOnError_; // RFE: Batch update
            // improvements
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_reuse].methodExit();
        }
    }

    void updateConnectionReusability(int stmtType) {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_updateConnectionReusability]
        .methodEntry(JdbcDebug.debugLevelPooling);
        try {
            switch (stmtType) {
                case SQL_CONTROL:
                connReuseBitMap_ |= SQL_CONTROL_FLAG;
                break;
                case SQL_SET_CATALOG:
                connReuseBitMap_ |= SQL_SET_CATALOG_FLAG;
                break;
                case SQL_SET_SCHEMA:
                connReuseBitMap_ |= SQL_SET_SCHEMA_FLAG;
                break;
                case SQL_SET_TRANSACTION:
                connReuseBitMap_ |= SQL_SET_TRANSACTION_FLAG;
                break;
            }
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_updateConnectionReusability].methodExit();
        }
    }
    //
    // Extension method for WLS, this method gives the pooledConnection object
    // associated with the given connection object.
    public PooledConnection getPooledConnection() throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getPooledConnection]
        .methodEntry(JdbcDebug.debugLevelPooling);
        try {
            if (pc_ == null)
            throw Messages.createSQLException(locale_,
                    "null_pooled_connection", null);
            if (JdbcDebugCfg.traceActive)
            debug[methodId_getPooledConnection].methodReturn("pc="
                    + JdbcDebug.debugObjectStr(pc_));
            return pc_;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getPooledConnection].methodExit();
        }
    }

    static int mapTxnMode(String txnModeStr) {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_mapTxnMode].methodEntry(JdbcDebug.debugLevelPooling);
        try {
            int txnMode;

            if (txnModeStr.equalsIgnoreCase("internal") || txnModeStr.equals("1"))
            txnMode = TXN_MODE_INTERNAL;
            else if (txnModeStr.equalsIgnoreCase("mixed") || txnModeStr.equals("2"))
            txnMode = TXN_MODE_MIXED;
            else if (txnModeStr.equalsIgnoreCase("external") || txnModeStr.equals("3"))
            txnMode = TXN_MODE_EXTERNAL;
            else
            txnMode = TXN_MODE_INVALID;
            return txnMode;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_mapTxnMode].methodExit();
        }
    }

    static String mapTxnModeToString(int txnMode) {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_mapTxnModeToString]
        .methodEntry(JdbcDebug.debugLevelPooling);
        try {
            String txnModeStr = null;

            switch (txnMode) {
                case 1:
                txnModeStr = "internal";
                break;
                case 2:
                txnModeStr = "mixed";
                break;
                case 3:
                txnModeStr = "external";
                break;
                default:
                txnModeStr = "mixed";
            }
            return txnModeStr;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_mapTxnModeToString].methodExit();
        }
    }

private void initSetDefaults() throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_initSetDefaults].methodEntry();
        try {
            clearWarnings();
            autoCommit_ = true;
            transactionIsolation_ = TRANSACTION_READ_COMMITTED;
            isReadOnly_ = false;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_initSetDefaults].methodExit();
        }
    }

    // Log the JDBC SQL statement and the STMTID to the idMapFile if the
    // enableLog_ property is set.
    private void printIdMapEntry(SQLMXStatement stmt) {
        SQLMXDataSource.prWriter_.println("["
                + T2Driver.dateFormat.format(new java.util.Date()) + "] "
                + stmt.getStmtLabel_() + " (" + stmt.sql_.trim() + ")\n");
    }

    // Constructors with access specifier as "default"
    SQLMXConnection(T2Driver driver, String url, T2Properties info)
    throws SQLException {
        super();
        if (JdbcDebugCfg.entryActive)
        debug[methodId_SQLMXConnection_LLL].methodEntry();
        /*	this.txIDPerThread = new ThreadLocal<Integer>() {
         protected synchronized Integer initialValue() {
         return new Integer(0);
         }
         };*/
        try {
            driver_ = driver;
            url_ = url;
            initSetDefaults();

            t2props = info;
            initConnectionProps(t2props);
//			catalog_ = T2Driver.catalog_;
//			schema_ = T2Driver.schema_;
//			locale_ = T2Driver.locale_;
//			batchBindingSize_ = T2Driver.batchBindingSize_;
//
//			connectionTimeout_ = 60;

            setDialogueId(connect(server_, uid_, pwd_));
            if (getDialogueId() == 0)
            return;
            isClosed_ = false;
            byteSwap_ = false;

            refQ_ = new ReferenceQueue<SQLMXStatement>();
            refToStmt_ = new HashMap<WeakReference, Long>();
            pRef_ = new WeakReference<SQLMXConnection>(this, driver_.weakConnection.refQ_);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_SQLMXConnection_LLL].methodExit();
        }
    }

    SQLMXConnection(SQLMXDataSource ds, T2Properties properties)
    throws SQLException {
        super();
        if (JdbcDebugCfg.entryActive)
        debug[methodId_SQLMXConnection_LL_ds].methodEntry();

        t2props = properties;
        if(this.t2props.getLogWriter()!=null) {
            this.setTracer(this.t2props.getLogWriter());
        }

        if (this.getTracer() != null)
        this.getTracer().println(getTraceId() + "<Init>");

        /*
         this.txIDPerThread = new ThreadLocal<Integer>() {
         protected synchronized Integer initialValue() {
         return new Integer(0);
         }
         };*/

        try {
            ds_ = ds;
            initSetDefaults();

//			dsn_ = properties.getProperty("dataSourceName");
//			dsn_ = properties.getDataSourceName();

            // Obtain all the property info
//			getProperties(properties);

            initConnectionProps(t2props);

            setDialogueId(connect(server_, uid_, pwd_));
            if (getDialogueId() == 0)
            return;
            isClosed_ = false;
            byteSwap_ = false;

            setIsSpjRSFlag(getDialogueId(), t2props.isSpjrsOn());

            refQ_ = new ReferenceQueue<SQLMXStatement>();
            refToStmt_ = new HashMap<WeakReference, Long>();
            pRef_ = new WeakReference<SQLMXConnection>(this,WeakConnection.refQ_);
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_SQLMXConnection_LL_ds].methodExit();
        }
    }

    SQLMXConnection(SQLMXPooledConnection poolConn, T2Properties info)
    throws SQLException {
        super(info);
        if (JdbcDebugCfg.entryActive)
        debug[methodId_SQLMXConnection_LL_pool]
        .methodEntry(JdbcDebug.debugLevelPooling);
        if (JdbcDebugCfg.traceActive)
        debug[methodId_getProperties].methodParameters("poolConn="
                + JdbcDebug.debugObjectStr(poolConn) + ", info="
                + JdbcDebug.debugObjectStr(info));
        t2props = info;
        if(this.t2props.getTraceFile()!=null&&SQLMXDataSource.traceWriter_!=null) {
            this.setTracer(SQLMXDataSource.traceWriter_);
        }

        if (this.getTracer() != null)
        this.getTracer().println(getTraceId() + "<Init>");

        /*	this.txIDPerThread = new ThreadLocal<Integer>() {
         protected synchronized Integer initialValue() {
         return new Integer(0);
         }
         };*/

        try {
            pc_ = poolConn;
            initSetDefaults();

//			getProperties(info);

            initConnectionProps(t2props);

            setDialogueId(connect(server_, uid_, pwd_));
            if (getDialogueId() == 0)
            return;
            isClosed_ = false;
            byteSwap_ = false;

            refQ_ = new ReferenceQueue<SQLMXStatement>();
            refToStmt_ = new HashMap<WeakReference, Long>();
            pRef_ = new WeakReference<SQLMXConnection>(this,
                    SQLMXPooledConnection.refQ_);
            holdability_ = SQLMXResultSet.CLOSE_CURSORS_AT_COMMIT;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_SQLMXConnection_LL_pool].methodExit();
        }
    }

    static String getCharsetEncodingCached(String server, long dialogueId,
            int charset, String encodingOverride) throws SQLException {
        if (JdbcDebugCfg.entryActive)
        debug[methodId_getCharsetEncodingCached]
        .methodEntry(JdbcDebug.debugLevelPooling);
        try {
            if ((server == getCharsetEncodingCached_server)
                    && (dialogueId == getCharsetEncodingCached_dialogueId)
                    && (charset == getCharsetEncodingCached_charset)
                    && (encodingOverride == getCharsetEncodingCached_encodingOverride))
            return getCharsetEncodingCached_encoding;
            getCharsetEncodingCached_encoding = getCharsetEncoding(server,
                    dialogueId, charset, encodingOverride);
            getCharsetEncodingCached_server = server;
            getCharsetEncodingCached_dialogueId = dialogueId;
            getCharsetEncodingCached_charset = charset;
            getCharsetEncodingCached_encodingOverride = encodingOverride;
            return getCharsetEncodingCached_encoding;
        }finally {
            if (JdbcDebugCfg.entryActive)
            debug[methodId_getCharsetEncodingCached].methodExit();
        }
    }

public static boolean getSqlStmtType2(String str) {
        if ( getSqlStmtType(str.toUpperCase()) == TYPE_INSERT_PARAM
                || getSqlStmtType(str.toUpperCase()) == TYPE_INSERT
                || getSqlStmtType(str.toUpperCase()) == TYPE_UPDATE
                || getSqlStmtType(str.toUpperCase()) == TYPE_DELETE
        )
        {
            return true;
        } else {
            return false;
        }
    }

    static short getSqlStmtType(String str) {
        //
        // Kludge to determin if the type of statement.
        //
        String tokens[] = str.split("[^a-zA-Z]+", 3);
        short rt1 = TYPE_UNKNOWN;
        String str3 = "";

        //
        // If there are no separators (i.e. no spaces, {, =, etc.) in front of
        // the
        // first token, then the first token is the key word we are looking for.
        // Else, the first token is an empty string (i.e. split thinks the first
        // token is the empty string followed by a separator), and the second
        // token is the key word we are looking for.
        //
        if (tokens[0].length() > 0) {
            str3 = tokens[0].toUpperCase();
        } else {
            str3 = tokens[1].toUpperCase();
        }

        if ((str3.equals("SELECT")) || (str3.equals("SHOWSHAPE"))
                || (str3.equals("INVOKE")) || (str3.equals("SHOWCONTROL"))
                || (str3.equals("SHOWPLAN"))) {
            rt1 = TYPE_SELECT;
        } else if (str3.equals("UPDATE")) {
            rt1 = TYPE_UPDATE;
        } else if (str3.equals("DELETE")) {
            rt1 = TYPE_DELETE;
        } else if (str3.equals("INSERT") || (str.equals("UPSERT"))) {
            if (str.indexOf('?') == -1) {
                rt1 = TYPE_INSERT;
            } else {
                rt1 = TYPE_INSERT_PARAM;
            }
        } else if (str3.equals("EXPLAIN")) {
            rt1 = TYPE_EXPLAIN;
        } else if (str3.equals("CREATE")) {
            rt1 = TYPE_CREATE;
        } else if (str3.equals("GRANT")) {
            rt1 = TYPE_GRANT;
        } else if (str3.equals("DROP")) {
            rt1 = TYPE_DROP;
        } else if (str3.equals("CALL")) {
            rt1 = TYPE_CALL;
        } else if (str3.equals("EXPLAIN")) {
            rt1 = TYPE_EXPLAIN;
        } else if (str3.equals("CONTROL")) {
            rt1 = TYPE_CONTROL;
        } else {
            rt1 = TYPE_UNKNOWN;

        }
        return rt1;

    }

    // Native methods
private native void setCatalog(String server, long dialogueId, String Catalog);

private native void setTransactionIsolation(String server, long dialogueId,
            int level);
private native void setAutoCommit(String server, long dialogueId, boolean autoCommit);

    // SPJRS
private native void setIsSpjRSFlag(long dialogueId, boolean isSpjrsOn);

    // private native void setReadOnly(String server, int dialogueId, boolean
    // readOnly);
private native void connectInit(String server, long dialogueId,
            String catalog, String schema, boolean isReadOnly, boolean autoCommit,
            int transactionIsolation, int loginTimeout, int queryTimeout,
            boolean blnDoomUsrTxn,
            int statisticsIntervalTime_, int statisticsLimitTime_, String statisticsType_, String programStatisticsEnabled_, String statisticsSqlPlanEnabled_) throws SQLException;

private native void connectReuse(String server, long dialogueId,
            int conResetValue, String catalog, String schema, 
            int transactionIsolation) throws SQLException;

private native long connect(String server, String uid, String pwd);

    static native void close(String server, long dialogueId) throws SQLException;

private native void commit(String server, long dialogueId, int txid)
    throws SQLException;

private native void rollback(String server, long dialogueId, int txid)
    throws SQLException;

private native int beginTransaction(String server, long dialogueId);

    static native String getCharsetEncoding(String server, long dialogueId,
            int charset, String encodingOverride) throws SQLException;

    native void setCharsetEncodingOverride(String server, long dialogueId,
            int charset, String encodingOverride) throws SQLException;

public void setTxid_(int txid_) {
        this.txid_ = txid_;
//		this.txIDPerThread.set(new Integer(txid_));
    }

public int getTxid() {
        return txid_;
//		return this.txIDPerThread.get().intValue();
    }

public void setDialogueId(long dialogueId_) throws SQLException {
        this.dialogueId_ = dialogueId_;
    }
public long getDialogueId() throws SQLException {
        if(dialogueId_ != 0) {
            if(mapOfClosedDialogs.containsKey(this.keyForMap)) {
                throw new SQLException(Long.toHexString(mapOfClosedDialogs.get(this.keyForMap)) + " Connection is already closed.");
            }
        }
        return dialogueId_;
    }

    /**
     * @param tracer the tracer to set
     */
public void setTracer(PrintWriter tracer) {
        setTraceId(org.apache.trafodion.jdbc.t2.T2Driver.traceText
                + org.apache.trafodion.jdbc.t2.T2Driver.dateFormat.format(new Date())
                + "]:["
                + Thread.currentThread()
                + "]:["
                + System.identityHashCode(this)
                + "]:"
                + this.getClass().getName().substring(
                        org.apache.trafodion.jdbc.t2.T2Driver.REMOVE_PKG_NAME, this.getClass().getName()
                        .length()) + ".");
        this.out_ = tracer;
    }

    /**
     * @return the tracer
     */
public PrintWriter getTracer() {
        return out_;
    }

public void initConnectionProps(T2Properties info) {

        dsn_ = info.getDataSourceName();
        transactionMode_ = mapTxnMode(info.getTransactionMode());
        catalog_ = info.getCatalog();
        schema_ = info.getSchema();
        inlineLobChunkSize_ = info.getInlineLobChunkSize();
        lobChunkSize_ = info.getLobChunkSize();
        String lang = info.getLanguage();
        if (lang == null)
            locale_ = Locale.getDefault();
        else {
            if (lang.equalsIgnoreCase("ja"))
            locale_ = new Locale("ja", "", "");
            else if (lang.equalsIgnoreCase("en"))
            locale_ = new Locale("en", "", "");
            else
            locale_ = Locale.getDefault();
        }
        batchBindingSize_ = info.getBatchBinding();
        connectionTimeout_ = 60;
        loginTimeout_ = info.getLoginTimeout();
        contBatchOnError_ = info.getContBatchOnError();
        iso88591EncodingOverride_ = info.getIso88591EncodingOverride();

        //Publishing
        statisticsIntervalTime_ = info.getStatisticsIntervalTime();
        statisticsLimitTime_ = info.getStatisticsLimitTime();
        statisticsType_ = info.getStatisticsType();
        programStatisticsEnabled_ = info.getProgramStatisticsEnabled();
        statisticsSqlPlanEnabled_ = info.getStatisticsSqlPlanEnabled();

        dsCatalog_ = catalog_;
        dsSchema_ = schema_;
        dsBatchBindingSize_ = batchBindingSize_;
        dsTransactionMode_ = transactionMode_;
        dsIso88591EncodingOverride_ = iso88591EncodingOverride_;
        dsContBatchOnError_ = contBatchOnErrorval_;// RFE: Batch update

        // The enableLog_ flag is used to log SQL STMTIDs and their SQL strings
        // if the
        // enableLog and idMapFile system properties are not null, and the
        // enableLog
        // system property is set to "on".
        if ((info.getEnableLog() != null) && (info.getIdMapFile() != null) && (info.getEnableLog().equalsIgnoreCase("on"))) {
            synchronized (SQLMXDataSource.class) {
                if (SQLMXDataSource.prWriter_ == null) {
                    try {
                        SQLMXDataSource.prWriter_ = new PrintWriter(new FileOutputStream(info.getIdMapFile(), true), true);
                    } catch (java.io.IOException e) {
                        SQLMXDataSource.prWriter_ = new PrintWriter(System.err, true);
                    }
                }
            }
        }

    }

    //Jaime
public synchronized long incrementAndGetkeyForMapCounter() {

        return ++keyForMapCounter;
    }

    public int getInlineLobChunkSize()
    {
        return inlineLobChunkSize_;
    }

    public int getLobChunkSize()
    {
        return lobChunkSize_;
    }

public static final int SQL_TXN_READ_UNCOMMITTED = 1;
public static final int SQL_TXN_READ_COMMITTED = 2;
public static final int SQL_TXN_REPEATABLE_READ = 4;
public static final int SQL_TXN_SERIALIZABLE = 8;

public static final int TXN_MODE_INVALID = 0;
public static final int TXN_MODE_INTERNAL = 1;
public static final int TXN_MODE_MIXED = 2;
public static final int TXN_MODE_EXTERNAL = 3;

public static final String UCS_STR = "UCS2";

    // Added the following for Connection pooling enhancement (Amena).
    // {Note: These must match what is defined in sqlcli.h}
public static final int SQL_CONTROL = 9;
public static final int SQL_SET_TRANSACTION = 10;
public static final int SQL_SET_CATALOG = 11;
public static final int SQL_SET_SCHEMA = 12;

public static final int SQL_CONTROL_FLAG = 0x0008;
public static final int SQL_SET_CATALOG_FLAG = 0x0004;
public static final int SQL_SET_SCHEMA_FLAG = 0x0002;
public static final int SQL_SET_TRANSACTION_FLAG = 0x0001;

    // Fields
    static long rsCount_;
    boolean beginTransFlag_ = false;
    String server_;
    String dsn_;
    boolean autoCommit_;
    String catalog_;
    String schema_;
    int transactionIsolation_;
    boolean isClosed_;
    boolean isReadOnly_;
    String url_;
    String uid_;
    String pwd_;
    int loginTimeout_;
    int queryTimeout_;
    int connectionTimeout_;
    int inlineLobChunkSize_;
    int lobChunkSize_;
    private long dialogueId_;
    int hClosestmtCount=0;
    int lClosestmtCount=0;
    int pStmtCount=0;

    //Publishing
    int statisticsIntervalTime_;
    int statisticsLimitTime_;
    String statisticsType_;
    String programStatisticsEnabled_;
    String statisticsSqlPlanEnabled_;

    boolean byteSwap_;
    private int txid_;
    //ThreadLocal<Integer> txIDPerThread;
    Map<String, Class<?>> userMap_;
    Locale locale_;
    ReferenceQueue<SQLMXStatement> refQ_;
    HashMap<WeakReference, Long> refToStmt_;
    boolean connectInitialized_;
    int holdability_;
    int batchBindingSize_;
    String contBatchOnError_;
    boolean contBatchOnErrorval_;
    WeakReference<SQLMXConnection> pRef_;
    SQLMXDataSource ds_;
    SQLMXPooledConnection pc_;
    T2Driver driver_;

    T2Properties t2props;

    int transactionMode_;
    String iso88591EncodingOverride_;

    String dsCatalog_;
    String dsSchema_;

    // Bit-mapped value that corresponds to what SQL connection
    // attribute statements that have been executed within the connection
    // context
    int connReuseBitMap_;

    // Cache area for character set
    static String getCharsetEncodingCached_server;
    static long getCharsetEncodingCached_dialogueId;
    static int getCharsetEncodingCached_charset;
    static String getCharsetEncodingCached_encodingOverride;
    static String getCharsetEncodingCached_encoding;

private static int methodId_close_V = 0;
private static int methodId_close_Z = 1;
private static int methodId_commit = 2;
private static int methodId_createStatement = 3;
private static int methodId_createStatement_II = 4;
private static int methodId_createStatement_III = 5;
private static int methodId_getAutoCommit = 6;
private static int methodId_getCatalog = 7;
private static int methodId_getHoldability = 8;
private static int methodId_getMetaData = 9;
private static int methodId_getTransactionIsolation = 10;
private static int methodId_getTypeMap = 11;
private static int methodId_isClosed = 12;
private static int methodId_isReadOnly = 13;
private static int methodId_nativeSQL = 14;
private static int methodId_prepareCall_L = 15;
private static int methodId_prepareCall_LII = 16;
private static int methodId_prepareCall_LIII = 17;
private static int methodId_prepareStatement_L = 18;
private static int methodId_prepareStatement_LI = 19;
private static int methodId_prepareStatement_LI_array = 20;
private static int methodId_prepareStatement_LL_array = 21;
private static int methodId_prepareStatement_LII = 22;
private static int methodId_prepareStatement_LIII = 23;
private static int methodId_releaseSavepoint = 24;
private static int methodId_rollback_V = 25;
private static int methodId_rollback_L = 26;
private static int methodId_setAutoCommit = 27;
private static int methodId_setCatalog = 28;
private static int methodId_setHoldability = 29;
private static int methodId_setReadOnly = 30;
private static int methodId_setSavepoint_L = 31;
private static int methodId_setSavepoint_V = 32;
private static int methodId_setTransactionIsolation = 33;
private static int methodId_setTypeMap = 34;
private static int methodId_begintransaction = 35;
private static int methodId_mapTxnIsolation = 36;
private static int methodId_gcStmts = 37;
private static int methodId_removeElement_L = 38;
private static int methodId_removeElement_V = 39;
private static int methodId_addElement = 40;
private static int methodId_connectInit = 41;
private static int methodId_reuse = 42;
private static int methodId_updateConnectionReusability = 43;
private static int methodId_getDataLocator = 44;
private static int methodId_getPooledConnection = 45;
private static int methodId_getProperties = 46;
private static int methodId_SQLMXConnection_LLL = 47;
private static int methodId_SQLMXConnection_LL_ds = 48;
private static int methodId_SQLMXConnection_LL_pool = 49;
private static int methodId_mapTxnMode = 50;
private static int methodId_mapTxnModeToString = 51;
private static int methodId_initSetDefaults = 52;
private static int methodId_getCharsetEncodingCached = 53;
private static int methodId_getSchema = 54;
private static int totalMethodIds = 55;
private static JdbcDebug[] debug;

    static {
        String className = "SQLMXConnection";
        if (JdbcDebugCfg.entryActive) {
            debug = new JdbcDebug[totalMethodIds];
            debug[methodId_close_V] = new JdbcDebug(className, "close_V");
            debug[methodId_close_Z] = new JdbcDebug(className, "close_Z");
            debug[methodId_commit] = new JdbcDebug(className, "commit");
            debug[methodId_createStatement] = new JdbcDebug(className,
                    "createStatement");
            debug[methodId_createStatement_II] = new JdbcDebug(className,
                    "createStatement");
            debug[methodId_createStatement_III] = new JdbcDebug(className,
                    "createStatement");
            debug[methodId_getAutoCommit] = new JdbcDebug(className,
                    "getAutoCommit");
            debug[methodId_getCatalog] = new JdbcDebug(className, "getCatalog");
            debug[methodId_getHoldability] = new JdbcDebug(className,
                    "getHoldability");
            debug[methodId_getMetaData] = new JdbcDebug(className,
                    "getMetaData");
            debug[methodId_getTransactionIsolation] = new JdbcDebug(className,
                    "getTransactionIsolation");
            debug[methodId_getTypeMap] = new JdbcDebug(className, "getTypeMap");
            debug[methodId_isClosed] = new JdbcDebug(className, "isClosed");
            debug[methodId_isReadOnly] = new JdbcDebug(className, "isReadOnly");
            debug[methodId_nativeSQL] = new JdbcDebug(className, "nativeSQL");
            debug[methodId_prepareCall_L] = new JdbcDebug(className,
                    "prepareCall[L]");
            debug[methodId_prepareCall_LII] = new JdbcDebug(className,
                    "prepareCall[LII]");
            debug[methodId_prepareCall_LIII] = new JdbcDebug(className,
                    "prepareCall[LIII]");
            debug[methodId_prepareStatement_L] = new JdbcDebug(className,
                    "prepareStatement[L]");
            debug[methodId_prepareStatement_LI] = new JdbcDebug(className,
                    "prepareStatement[LI]");
            debug[methodId_prepareStatement_LI_array] = new JdbcDebug(
                    className, "prepareStatement[LI_array]");
            debug[methodId_prepareStatement_LL_array] = new JdbcDebug(
                    className, "prepareStatement[LL_array]");
            debug[methodId_prepareStatement_LII] = new JdbcDebug(className,
                    "prepareStatement[LII]");
            debug[methodId_prepareStatement_LIII] = new JdbcDebug(className,
                    "prepareStatement[LIII]");
            debug[methodId_releaseSavepoint] = new JdbcDebug(className,
                    "releaseSavepoint");
            debug[methodId_rollback_V] = new JdbcDebug(className, "rollback_V");
            debug[methodId_rollback_L] = new JdbcDebug(className, "rollback_L");
            debug[methodId_setAutoCommit] = new JdbcDebug(className,
                    "setAutoCommit");
            debug[methodId_setCatalog] = new JdbcDebug(className, "setCatalog");
            debug[methodId_setHoldability] = new JdbcDebug(className,
                    "setHoldability");
            debug[methodId_setReadOnly] = new JdbcDebug(className,
                    "setReadOnly");
            debug[methodId_setSavepoint_L] = new JdbcDebug(className,
                    "setSavepoint[L]");
            debug[methodId_setSavepoint_V] = new JdbcDebug(className,
                    "setSavepoint[V]");
            debug[methodId_setTransactionIsolation] = new JdbcDebug(className,
                    "setTransactionIsolation");
            debug[methodId_setTypeMap] = new JdbcDebug(className, "setTypeMap");
            debug[methodId_begintransaction] = new JdbcDebug(className,
                    "begintransaction");
            debug[methodId_mapTxnIsolation] = new JdbcDebug(className,
                    "mapTxnIsolation");
            debug[methodId_gcStmts] = new JdbcDebug(className, "gcStmts");
            debug[methodId_removeElement_L] = new JdbcDebug(className,
                    "removeElement[L]");
            debug[methodId_removeElement_V] = new JdbcDebug(className,
                    "removeElement[V]");
            debug[methodId_addElement] = new JdbcDebug(className, "addElement");
            debug[methodId_connectInit] = new JdbcDebug(className,
                    "connectInit");
            debug[methodId_reuse] = new JdbcDebug(className, "reuse");
            debug[methodId_updateConnectionReusability] = new JdbcDebug(
                    className, "updateConnectionReusability");
            debug[methodId_getDataLocator] = new JdbcDebug(className,
                    "getDataLocator");
            debug[methodId_getPooledConnection] = new JdbcDebug(className,
                    "getPooledConnection");
            debug[methodId_getProperties] = new JdbcDebug(className,
                    "getProperties");
            debug[methodId_SQLMXConnection_LLL] = new JdbcDebug(className,
                    "SQLMXConnection[LLL]");
            debug[methodId_SQLMXConnection_LL_ds] = new JdbcDebug(className,
                    "SQLMXConnection[LL_ds]");
            debug[methodId_SQLMXConnection_LL_pool] = new JdbcDebug(className,
                    "SQLMXConnection[LL_pool]");
            debug[methodId_mapTxnMode] = new JdbcDebug(className, "mapTxnMode");
            debug[methodId_mapTxnModeToString] = new JdbcDebug(className,
                    "mapTxnModeToString");
            debug[methodId_initSetDefaults] = new JdbcDebug(className,
                    "initSetDefaults");
            debug[methodId_getCharsetEncodingCached] = new JdbcDebug(className,
                    "getCharsetEncodingCached");
        }

    }
    int dsBatchBindingSize_;
    int dsTransactionMode_;
    String dsIso88591EncodingOverride_;
    boolean dsContBatchOnError_; // RFE: Batch update improvements
    static final HashMap<Long, Long> mapOfClosedDialogs = new HashMap<Long, Long>();
    static private long keyForMapCounter = 0;
private long keyForMap;

private PrintWriter tracer;
    PreparedStatement clearCQD1;
    PreparedStatement clearCQD2;
    PreparedStatement clearCQD3;

public Object unwrap(Class iface) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public boolean isWrapperFor(Class iface) throws SQLException {
        // TODO Auto-generated method stub
        return false;
    }

public Clob createClob() throws SQLException {
	return new SQLMXClob(this, null);
    }

public Blob createBlob() throws SQLException {
        return new SQLMXBlob(this, null);
    }

public NClob createNClob() throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public SQLXML createSQLXML() throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public boolean isValid(int timeout) throws SQLException {
        // TODO Auto-generated method stub
        return false;
    }

public void setClientInfo(String name, String value)
    throws SQLClientInfoException {
        // TODO Auto-generated method stub

    }

public void setClientInfo(Properties properties)
    throws SQLClientInfoException {
        // TODO Auto-generated method stub

    }

public String getClientInfo(String name) throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public Properties getClientInfo() throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public Array createArrayOf(String typeName, Object[] elements)
    throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public Struct createStruct(String typeName, Object[] attributes)
    throws SQLException {
        // TODO Auto-generated method stub
        return null;
    }

public void setSchema(String schema) throws SQLException {
        // TODO Auto-generated method stub

    }

public void abort(Executor executor) throws SQLException {
        // TODO Auto-generated method stub

    }

public void setNetworkTimeout(Executor executor, int milliseconds)
    throws SQLException {
        // TODO Auto-generated method stub

    }

public int getNetworkTimeout() throws SQLException {
        // TODO Auto-generated method stub
        return 0;
    }

public String getSchema() throws SQLException {
    if (out_ != null)
        out_.println(getTraceId() + "getSchema()");
    if (JdbcDebugCfg.entryActive)
        debug[methodId_getSchema].methodEntry();
    try {
        clearWarnings();
        Statement s = null;
        ResultSet rs = null;
        String sch = null;

        try {
            s = this.createStatement();
            rs = s.executeQuery("SHOWCONTROL DEFAULT SCHEMA, match full, no header");
            rs.next();
            sch = rs.getString(1);
            if (sch.charAt(0) != '\"' && sch.indexOf('.') != -1) {
                sch = sch.substring(sch.indexOf('.') + 1);
            }
        } catch (SQLException e) {
            return schema_;
        } finally {
            if (rs != null)
                rs.close();
            if (s != null)
                s.close();
        }
        return sch;
    } finally {
        if (JdbcDebugCfg.entryActive)
            debug[methodId_getSchema].methodExit();
    }
    }
}
