/**
 *(C) Copyright 2015 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.trafodion.dcs.servermt.serverSql;

import java.sql.*;
import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
//import java.util.*;
import java.math.BigDecimal;
import java.util.Calendar;
import java.util.HashMap;
import java.util.Map;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.serverDriverInputOutput.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class TrafStatement {
    private static  final Log LOG = LogFactory.getLog(TrafStatement.class);
    private Statement stmt = null;
    private int outNumberParams = 0;
    private long outParamLength = 0;
    private Descriptor2List outDescList = null;
    private Descriptor2List inpDescList = null;
    private int inpNumberParams = 0;
    private long inpParamLength = 0;
    private boolean isResultSet = false;

    public TrafStatement(Connection conn, String sqlString) throws SQLException {
        setStatement(conn, sqlString);
    }
    void init(){
        reset();
    }
    void reset(){
        stmt = null;
        outNumberParams = 0;
        outParamLength = 0;
        outDescList = null;
        inpDescList = null;
        inpNumberParams = 0;
        inpParamLength = 0;
        isResultSet = false;
    }
    public void closeTStatement(){
        try {
            if (stmt.isClosed() == false){
                stmt.close();
            }
        } catch (SQLException sql){}
        reset();
    }
    public void setOutNumberParams(int outNumberParams){
        this.outNumberParams = outNumberParams;
    }
    public void setOutDescList(Descriptor2List outDescList){
        this.outDescList = outDescList;
    }
    public void setOutParamLength(long outParamLength){
        this.outParamLength = outParamLength;
    }
    public void setInpNumberParams(int inpNumberParams){
        this.inpNumberParams = inpNumberParams;
    }
    public void setInpDescList(Descriptor2List inpDescList){
        this.inpDescList = inpDescList;
    }
    public void setInpParamLength(long inpParamLength){
        this.inpParamLength = inpParamLength;
    }
    public void setIsResultSet(boolean isResultSet){
        this.isResultSet = isResultSet;
    }
    public void setStatement(Connection conn, String sqlString) throws SQLException{
        if (this.stmt != null){
            if (this.stmt.isClosed() == false){
                this.stmt.close();
            }
            reset();
        }
        if (sqlString != null)
            this.stmt = conn.prepareStatement(sqlString);
        else
            this.stmt = conn.createStatement();
    }
//================================================
    public Statement getStatement(){
        return stmt;
    }
    public Descriptor2List getOutDescList(){
        return outDescList;
    }
    public int getOutNumberParams(){
        return outNumberParams;
    }
    public long getOutParamLength(){
        return outParamLength;
    }
    public Descriptor2List getInpDescList(){
        return inpDescList;
    }
    public int getInpNumberParams(){
        return inpNumberParams;
    }
    public long getInpParamLength(){
        return inpParamLength;
    }
    public boolean getIsResultSet(){
        return isResultSet;
    }
}
