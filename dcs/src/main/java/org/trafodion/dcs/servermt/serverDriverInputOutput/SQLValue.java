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

public class SQLValue {
    private static  final Log LOG = LogFactory.getLog(SQLValue.class);
    private int dataType;
    private short dataInd;
    private SQLDataValue dataValue;
    private int dataCharSet;
    
    public SQLValue(){
        dataType = 0;
        dataInd = 0;
        dataValue = null;
        dataCharSet = 0;
    }
    public SQLValue( SQLValue sv){
        dataType = sv.dataType;
        dataInd = sv.dataInd;
        dataValue = sv.dataValue;
        dataCharSet = sv.dataCharSet;
    }
// ----------------------------------------------------------
    public void insertIntoByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException{
        bbBuf.putInt(dataType);
        bbBuf.putShort(dataInd);
        dataValue.insertIntoByteBuffer(bbBuf);
        bbBuf.putInt(dataCharSet);
    }
// ----------------------------------------------------------
    public void extractFromByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException{
        dataType = bbBuf.getInt();
        dataInd = bbBuf.getShort();

        dataValue = new SQLDataValue();
        dataValue.extractFromByteBuffer(bbBuf);

        dataCharSet = bbBuf.getInt();
    }
// ----------------------------------------------------------
    public int lengthOfData() {
        return ServerConstants.INT_FIELD_SIZE * 2 + ServerConstants.SHORT_FIELD_SIZE + dataValue.lengthOfData();
    }
}
