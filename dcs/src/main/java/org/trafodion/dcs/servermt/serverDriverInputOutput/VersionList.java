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

import java.util.*;
import java.nio.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverHandler.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class VersionList {
    private static  final Log LOG = LogFactory.getLog(VersionList.class);
    private Version[] list = {null,null};

    public VersionList (){
        for (int i = 0; i < 2; i++) {
            list[i] = new Version();
        }
    }
    public VersionList (VersionList vl){
        for (int i = 0; i < 2; i++) {
            list[i] = new Version(vl.list[i]);
        }
    }
    public void extractFromByteBuffer(ByteBuffer bbBuf) {
        int len = bbBuf.getInt();

        for (int i = 0; i < len; i++) {
            list[i].extractFromByteBuffer(bbBuf);
        }
    }
    public void insertIntoByteBuffer(ByteBuffer bbBuf) {
        bbBuf.putInt(list.length);

        for (int i = 0; i < list.length; i++) {
            list[i].insertIntoByteBuffer(bbBuf);
        }
    }
    public int lengthOfData() {

        int dataLength = 0;
        dataLength += ServerConstants.INT_FIELD_SIZE;
        
        for (int i = 0; i < list.length; i++) {
            dataLength += list[i].lengthOfData();
        }
        return dataLength;
    }
    public Version[] getList(){
        return list;
    }
}
