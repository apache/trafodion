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

public class SQLValueList {
    private static  final Log LOG = LogFactory.getLog(SQLValueList.class);
    private SQLValue[] buffer = null;

    public SQLValueList(SQLValueList svl){
        buffer = new SQLValue[svl.buffer.length];
        for (int i = 0; i < svl.buffer.length; i++)
            buffer[i] = svl.buffer[i];
    }
// ----------------------------------------------------------
    public void insertIntoByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException{
        if (buffer != null) {
            bbBuf.putInt(buffer.length);
            for (int i = 0; i < buffer.length; i++) {
                buffer[i].insertIntoByteBuffer(bbBuf);
            }
        } else {
            bbBuf.putInt(0);
        }
    }
// ----------------------------------------------------------
    public void extractFromByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException {
        int len = bbBuf.getInt();

        if (len > 0){
            buffer = new SQLValue[len];
    
            for (int i = 0; i < buffer.length; i++) {
                buffer[i] = new SQLValue();
                buffer[i].extractFromByteBuffer(bbBuf);
            }
        }
    }
// ----------------------------------------------------------
    public int lengthOfData() {
        int dataLength = ServerConstants.INT_FIELD_SIZE;

        if (buffer != null) {
            for (int i = 0; i < buffer.length; i++) {
                dataLength += buffer[i].lengthOfData();
            }
        }
        return dataLength;
    }
}
