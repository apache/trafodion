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
import java.sql.SQLException;
import java.util.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverHandler.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class ErrorDescList {
    private static  final Log LOG = LogFactory.getLog(ErrorDescList.class);
    private int length;
    private ErrorDesc[] buffer;
    
    public ErrorDescList(){
        length = 0;
        buffer = null;
    }
    public ErrorDescList(int length){
        this.length = length;
        buffer = new ErrorDesc[length];
        for (int i = 0; i < length; i++)
            buffer[i] = new ErrorDesc();
    }
    public ErrorDescList(ErrorDescList edl){
        length = edl.length;
        buffer = new ErrorDesc[length];
        for (int i = 0; i < length; i++){
            buffer[i] = new ErrorDesc(edl.buffer[i]);
        }
    }
    public ErrorDescList(SQLException ex){

        List<SQLException> ax = new ArrayList<SQLException>();
        
        SQLException next;
        next = ex;
        do
        {
            ax.add(next);
        }
        while ((next = next.getNextException()) != null);
        
        length = ax.size();
        buffer = new ErrorDesc[length];
        for (int i = 0; i < length; i++){
            buffer[i] = new ErrorDesc(ax.get(i));
        }
    }
    // ----------------------------------------------------------
    public void insertIntoByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException {
        bbBuf.putInt(length);

        for (int i = 0; i < length; i++) {
            buffer[i].insertIntoByteBuffer(bbBuf);
        }
    }
    public int lengthOfData() {
        int dataLength = 0;
        
        dataLength += ServerConstants.INT_FIELD_SIZE;         //length
        for (int i = 0; i < length; i++) {
            dataLength += buffer[i].lengthOfData();       //ERROR_DESC_def          
        }
        return dataLength;        
    }
    public int getLength(){
        return length;
    }
    public ErrorDesc[] getBuffer(){
        return buffer;
    }
}
