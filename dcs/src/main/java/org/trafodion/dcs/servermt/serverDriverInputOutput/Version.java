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

import org.trafodion.dcs.Constants;

import java.util.*;
import java.nio.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverHandler.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class Version {
    private static  final Log LOG = LogFactory.getLog(Version.class);
    private short componentId;
    private short majorVersion;
    private short minorVersion;
    private int buildId;

    public Version(){
        componentId = 0;
        majorVersion = 0;
        minorVersion = 0;
        buildId = 0;
    }
    public Version(Version v){
        componentId = v.componentId;
        majorVersion = v.majorVersion;
        minorVersion = v.minorVersion;
        buildId = v.buildId;
    }
    public void extractFromByteBuffer(ByteBuffer bbBuf) {
        componentId = bbBuf.getShort();
        majorVersion = bbBuf.getShort();
        minorVersion = bbBuf.getShort();
        buildId = bbBuf.getInt();
    }
    public void insertIntoByteBuffer(ByteBuffer bbBuf) {
        bbBuf.putShort(componentId);
        bbBuf.putShort(majorVersion);
        bbBuf.putShort(minorVersion);
        bbBuf.putInt(buildId);
    }
    public int lengthOfData(){
        int dataLength = 0;
        dataLength += ServerConstants.SHORT_FIELD_SIZE;
        dataLength += ServerConstants.SHORT_FIELD_SIZE;
        dataLength += ServerConstants.SHORT_FIELD_SIZE;
        dataLength += ServerConstants.INT_FIELD_SIZE;
        return dataLength;
    }
    public void setComponentId( short componentId){
        this.componentId = componentId;
    }
    public void setMajorVersion( short majorVersion){
        this.majorVersion = majorVersion;
    }
    public void setMinorVersion( short minorVersion){
        this.minorVersion = minorVersion;
    }
    public void setBuildId( int buildId){
        this.buildId = buildId;
    }
}
