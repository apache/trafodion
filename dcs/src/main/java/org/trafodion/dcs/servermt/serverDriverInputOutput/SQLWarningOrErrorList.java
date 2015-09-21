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
import java.util.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverHandler.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class SQLWarningOrErrorList {
    private static  final Log LOG = LogFactory.getLog(SQLWarningOrErrorList.class);
    private int returnCode;
    private int totalErrorLength;
    private int conditions;
    private SQLWarningOrError[] buffer;
    
    public SQLWarningOrErrorList(){
        returnCode = ServerConstants.SQL_SUCCESS;
        totalErrorLength = 0;
        conditions = 0;
        buffer = null;
    }
    public SQLWarningOrErrorList(SQLException ex){

        List<SQLException> ax = new ArrayList<SQLException>();
        
        SQLException next;
        next = ex;
        do
        {
            ax.add(next);
        }
        while ((next = next.getNextException()) != null);

        totalErrorLength = ServerConstants.INT_FIELD_SIZE;            //conditions size
        conditions = ax.size();
        buffer = new SQLWarningOrError[conditions];
        for (int i = 0; i < conditions; i++){
            buffer[i] = new SQLWarningOrError(ax.get(i),0);
            totalErrorLength += buffer[i].lengthOfData();
            if (buffer[i].getSqlCode() < 0 && returnCode >= ServerConstants.SQL_SUCCESS)
                returnCode = ServerConstants.SQL_ERROR;
            else if (buffer[i].getSqlCode() > 0 && returnCode == ServerConstants.SQL_SUCCESS)
                returnCode = ServerConstants.SQL_SUCCESS_WITH_INFO;
        }
    }
    public SQLWarningOrErrorList(SQLException ex, int[] numStatus){

        int rowId = 0;
        for (int i = 0; i < numStatus.length; i++){
          if(numStatus[i] == -3){
              rowId = i + 1;
              break;
          }
        }
        List<SQLException> ax = new ArrayList<SQLException>();
        
        SQLException next;
        next = ex;
        do {
            ax.add(next);
        }
        while ((next = next.getNextException()) != null);

        totalErrorLength = ServerConstants.INT_FIELD_SIZE;            //conditions size
        conditions = ax.size();
        buffer = new SQLWarningOrError[conditions];
        for (int i = 0; i < conditions; i++){
            SQLException bex = ax.get(i);
            if (bex.getErrorCode() != 0)
                buffer[i] = new SQLWarningOrError(bex, rowId);
            else
                buffer[i] = new SQLWarningOrError(bex, 0);
            totalErrorLength += buffer[i].lengthOfData();
            if (rowId > 0) {
                returnCode = ServerConstants.SQL_SUCCESS_WITH_INFO;
            }
            else {
                if (buffer[i].getSqlCode() < 0 && returnCode >= ServerConstants.SQL_SUCCESS)
                    returnCode = ServerConstants.SQL_ERROR;
                else if (buffer[i].getSqlCode() > 0 && returnCode == ServerConstants.SQL_SUCCESS)
                    returnCode = ServerConstants.SQL_SUCCESS_WITH_INFO;
            }
        }
    }
    public void insertIntoByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException {
        bbBuf.putInt(totalErrorLength);
        bbBuf.putInt(conditions);
    
        for (int i = 0; i < conditions; i++) {
            buffer[i].insertIntoByteBuffer(bbBuf);
        }
    }
    // ----------------------------------------------------------
    public int lengthOfData() {
        int dataLength = 0;
        
        dataLength += ServerConstants.INT_FIELD_SIZE;                 //totalErrorLength
        dataLength += ServerConstants.INT_FIELD_SIZE;             //conditions
        for (int i = 0; i < conditions; i++) {
            dataLength += buffer[i].lengthOfData();
        }
        return dataLength;
    }
    public int getReturnCode(){
        return returnCode;
    }
}

