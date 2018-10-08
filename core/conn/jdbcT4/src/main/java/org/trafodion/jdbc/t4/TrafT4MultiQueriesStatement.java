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

package org.trafodion.jdbc.t4;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.logging.Level;

public class TrafT4MultiQueriesStatement extends TrafT4Statement {

    private String[] sqlArr = null;
    private TrafT4Statement[] stmtArr = null;
    private TrafT4Connection conn = null;
    private int resultSetType;
    private int resultSetConcurrency;
    private int resultSetHoldability;
    private String stmtLabel = null;
    private int currentSqlIndex;

    TrafT4MultiQueriesStatement(TrafT4Connection connection) throws SQLException {
        this(connection, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY,
                TrafT4ResultSet.CLOSE_CURSORS_AT_COMMIT, null);
        T4LoggingUtilities.log(connection_.props_.t4Logger_, Level.FINE, connection_.props_,
                "TrafT4MultiQueriesStatement", "<init>", "", connection);
    }

    TrafT4MultiQueriesStatement(TrafT4Connection connection, String stmtLabel) throws SQLException {
        this(connection, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY,
                TrafT4ResultSet.CLOSE_CURSORS_AT_COMMIT, stmtLabel);
        T4LoggingUtilities.log(connection_.props_.t4Logger_, Level.FINE, connection_.props_,
                "TrafT4MultiQueriesStatement", "<init>", "", connection, stmtLabel);
    }

    TrafT4MultiQueriesStatement(TrafT4Connection connection, int resultSetType, int resultSetConcurrency)
            throws SQLException {
        this(connection, resultSetType, resultSetConcurrency, TrafT4ResultSet.CLOSE_CURSORS_AT_COMMIT, null);
        T4LoggingUtilities.log(connection_.props_.t4Logger_, Level.FINE, connection_.props_,
                "TrafT4MultiQueriesStatement", "<init>", "", connection, resultSetType, resultSetConcurrency);
    }

    TrafT4MultiQueriesStatement(TrafT4Connection connection, int resultSetType, int resultSetConcurrency,
            int resultSetHoldability) throws SQLException {
        this(connection, resultSetType, resultSetConcurrency, resultSetHoldability, null);
        T4LoggingUtilities.log(connection_.props_.t4Logger_, Level.FINE, connection_.props_,
                "TrafT4MultiQueriesStatement", "<init>", "", connection, resultSetType, resultSetConcurrency,
                resultSetHoldability);
    }

    TrafT4MultiQueriesStatement(TrafT4Connection connection, int resultSetType, int resultSetConcurrency,
            int resultSetHoldability, String stmtLabel) throws SQLException {
        super(connection, resultSetType, resultSetConcurrency, resultSetHoldability, stmtLabel);
        T4LoggingUtilities.log(connection_.props_.t4Logger_, Level.FINE, connection_.props_,
                "TrafT4MultiQueriesStatement", "<init>", "", connection, resultSetType, resultSetConcurrency,
                resultSetHoldability, stmtLabel);

        if (resultSetType != ResultSet.TYPE_FORWARD_ONLY && resultSetType != ResultSet.TYPE_SCROLL_INSENSITIVE
                && resultSetType != ResultSet.TYPE_SCROLL_SENSITIVE) {
            throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
                    "invalid_resultset_type", null);
        }
        if (resultSetConcurrency != ResultSet.CONCUR_READ_ONLY && resultSetConcurrency != ResultSet.CONCUR_UPDATABLE) {
            throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
                    "invalid_resultset_concurrency", null);
        }
        if ((resultSetHoldability != 0) && (resultSetHoldability != ResultSet.CLOSE_CURSORS_AT_COMMIT)
                && (resultSetHoldability != ResultSet.HOLD_CURSORS_OVER_COMMIT)) {
            throw TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(), "invalid_holdability",
                    null);
        }

        this.conn = connection;
        this.resultSetType = resultSetType;
        this.resultSetConcurrency = resultSetConcurrency;
        this.resultSetHoldability = resultSetHoldability;
        this.stmtLabel = stmtLabel;
    }

    public ResultSet executeQuery(String sql) throws SQLException {
        T4LoggingUtilities.log(connection_.props_.t4Logger_, Level.FINE, connection_.props_,
                "TrafT4MultiQueriesStatement", "executeQuery", "", sql);

        this.sqlArr = sql.split(";");
        this.stmtArr = new TrafT4Statement[this.sqlArr.length];
        this.currentSqlIndex = 0;

        for (int i = 0; i < this.sqlArr.length; i++) {
            this.stmtArr[i] = new TrafT4Statement(this.conn, this.resultSetType, this.resultSetConcurrency,
                    this.resultSetHoldability, this.stmtLabel);

            this.stmtArr[i].setMaxRows(super.getMaxRows());
            this.stmtArr[i].setCursorName(super.cursorName_);
            this.stmtArr[i].setFetchDirection(super.getFetchDirection());
            this.stmtArr[i].setFetchSize(super.getFetchSize());
            if (super.escapeProcess_) {
                this.stmtArr[i].setEscapeProcessing(super.escapeProcess_);
            }
            if (super.getMaxFieldSize() != 0) {
                this.stmtArr[i].setMaxFieldSize(super.getMaxFieldSize());
            }
            this.stmtArr[i].setQueryTimeout(super.getQueryTimeout());
            this.stmtArr[i].setRoundingMode(super.roundingMode_);

            this.stmtArr[i].multiQueriesStmt = this;
            this.stmtArr[i].executeQuery(this.sqlArr[i]);
        }

        return getResultSet();
    }

    public int executeUpdate(String sql) throws SQLException {
        T4LoggingUtilities.log(connection_.props_.t4Logger_, Level.FINE, connection_.props_,
                "TrafT4MultiQueriesStatement", "executeUpdate", "", sql);

        this.sqlArr = sql.split(";");
        this.stmtArr = new TrafT4Statement[this.sqlArr.length];

        int result = 0;
        for (int i = 0; i < this.sqlArr.length; i++) {
            this.stmtArr[i] = new TrafT4Statement(this.conn, this.resultSetType, this.resultSetConcurrency,
                    this.resultSetHoldability, this.stmtLabel);

            this.stmtArr[i].setMaxRows(super.getMaxRows());
            this.stmtArr[i].setCursorName(super.cursorName_);
            this.stmtArr[i].setFetchDirection(super.getFetchDirection());
            this.stmtArr[i].setFetchSize(super.getFetchSize());
            if (super.escapeProcess_) {
                this.stmtArr[i].setEscapeProcessing(super.escapeProcess_);
            }
            if (super.getMaxFieldSize() != 0) {
                this.stmtArr[i].setMaxFieldSize(super.getMaxFieldSize());
            }
            this.stmtArr[i].setQueryTimeout(super.getQueryTimeout());
            this.stmtArr[i].setRoundingMode(super.roundingMode_);

            this.stmtArr[i].multiQueriesStmt = this;
            result += this.stmtArr[i].executeUpdate(this.sqlArr[i]);
        }
        return result;
    }

    public boolean execute(String sql) throws SQLException {
        T4LoggingUtilities.log(connection_.props_.t4Logger_, Level.FINE, connection_.props_,
                "TrafT4MultiQueriesStatement", "execute", "", sql);
        this.sqlArr = sql.split(";");
        this.stmtArr = new TrafT4Statement[sqlArr.length];

        boolean result = false;
        for (int i = 0; i < this.sqlArr.length; i++) {
            this.stmtArr[i] = new TrafT4Statement(this.conn, this.resultSetType, this.resultSetConcurrency,
                    this.resultSetHoldability, this.stmtLabel);

            this.stmtArr[i].setMaxRows(super.getMaxRows());
            this.stmtArr[i].setCursorName(super.cursorName_);
            this.stmtArr[i].setFetchDirection(super.getFetchDirection());
            this.stmtArr[i].setFetchSize(super.getFetchSize());
            if (super.escapeProcess_) {
                this.stmtArr[i].setEscapeProcessing(super.escapeProcess_);
            }
            if (super.getMaxFieldSize() != 0) {
                this.stmtArr[i].setMaxFieldSize(super.getMaxFieldSize());
            }
            this.stmtArr[i].setQueryTimeout(super.getQueryTimeout());
            this.stmtArr[i].setRoundingMode(super.roundingMode_);

            this.stmtArr[i].multiQueriesStmt = this;
            result |= this.stmtArr[i].execute(this.sqlArr[i]);
        }
        return result;
    }

    public ResultSet getResultSet() throws SQLException {
        T4LoggingUtilities.log(connection_.props_.t4Logger_, Level.FINE, connection_.props_,
                "TrafT4MultiQueriesStatement", "getResultSet", "");
        return this.stmtArr[this.currentSqlIndex].getResultSet();
    }

    public boolean getMoreResults() throws SQLException {
        T4LoggingUtilities.log(connection_.props_.t4Logger_, Level.FINE, connection_.props_,
                "TrafT4MultiQueriesStatement", "getMoreResults", "");
        if (++this.currentSqlIndex >= this.sqlArr.length) {
            return false;
        }
        return true;
    }

    public void close() throws SQLException {
        T4LoggingUtilities.log(connection_.props_.t4Logger_, Level.FINE, connection_.props_,
                "TrafT4MultiQueriesStatement", "close", "");
        if (this.stmtArr == null) {
            return;
        }
        SQLException[] eArr = new SQLException[this.stmtArr.length];
        boolean hasException = false;
        int exceptionNum = 0;
        for (int i = 0; i < this.stmtArr.length; i++) {
            try {
                this.stmtArr[i].close();
            } catch (SQLException e) {
                hasException = true;
                exceptionNum++;
                eArr[i] = e;
            }
        }
        if (hasException) {
            if (exceptionNum > 1) {
                TrafT4Exception t4e = TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
                        "prepared_statement_close_failed", null);
                SQLException sqlE = t4e;
                SQLException tmpE = null;
                for (int i = 0; i < eArr.length; i++) {
                    if (eArr[i] != null) {
                        if (tmpE == null) {
                            sqlE.setNextException(eArr[i]);
                            tmpE = eArr[i];
                        } else {
                            tmpE.setNextException(eArr[i]);
                            tmpE = eArr[i];
                        }
                    }
                }
                throw sqlE;
            } else {
                for (int i = 0; i < eArr.length; i++) {
                    if (eArr[i] != null) {
                        throw eArr[i];
                    }
                }
            }
        }
    }

    public void cancel() throws SQLException {
        T4LoggingUtilities.log(connection_.props_.t4Logger_, Level.FINE, connection_.props_,
                "TrafT4MultiQueriesStatement", "cancel", "");
        if (this.stmtArr == null) {
            return;
        }
        SQLException[] eArr = new SQLException[stmtArr.length];
        boolean hasException = false;
        int exceptionNum = 0;
        for (int i = 0; i < stmtArr.length; i++) {
            try {
                this.stmtArr[i].cancel();
            } catch (SQLException e) {
                hasException = true;
                exceptionNum++;
                eArr[i] = e;
            }
        }

        if (hasException) {
            if (exceptionNum > 1) {
                TrafT4Exception t4e = TrafT4Messages.createSQLException(connection_.props_, connection_.getLocale(),
                        "prepared_statement_cancel_failed", null);
                SQLException sqlE = t4e;
                SQLException tmpE = null;
                for (int i = 0; i < eArr.length; i++) {
                    if (eArr[i] != null) {
                        if (tmpE == null) {
                            sqlE.setNextException(eArr[i]);
                            tmpE = eArr[i];
                        } else {
                            tmpE.setNextException(eArr[i]);
                            tmpE = eArr[i];
                        }
                    }
                }
                throw sqlE;
            } else {
                for (int i = 0; i < eArr.length; i++) {
                    if (eArr[i] != null) {
                        throw eArr[i];
                    }
                }
            }
        }
    }
}
