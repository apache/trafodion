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

public class OutConnectionContext {
    private static  final Log LOG = LogFactory.getLog(OutConnectionContext.class);
    
    private VersionList versionList;
    private short nodeId = 0;
    private int processId = 0;
    private String computerName = "";
    private String catalog = "";
    private String schema = "";
    private long optionFlags1 = 0;
    private long optionFlags2 = 0;
    private String _roleName = "";
    private boolean _enforceISO = false;
    private boolean _ignoreCancel = false;
    private byte [] cert = null;
    
    public OutConnectionContext(byte[] cert){
        optionFlags1 = optionFlags1 | ServerConstants.OUTCONTEXT_OPT1_DOWNLOAD_CERTIFICATE;
        this.cert = cert;
        versionList = new VersionList();
    }
    public void extractFromByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException {
        versionList.extractFromByteBuffer(bbBuf);

        nodeId = bbBuf.getShort();
        processId = bbBuf.getInt();
        computerName = ByteBufferUtils.extractString(bbBuf);

        catalog = ByteBufferUtils.extractString(bbBuf);
        schema = ByteBufferUtils.extractString(bbBuf);

        optionFlags1 = bbBuf.getInt();
        optionFlags2 = bbBuf.getInt();

        this._enforceISO = (optionFlags1 & ServerConstants.OUTCONTEXT_OPT1_ENFORCE_ISO88591) > 0;
        this._ignoreCancel = (optionFlags1 & ServerConstants.OUTCONTEXT_OPT1_IGNORE_SQLCANCEL) > 0;
        if((optionFlags1 & ServerConstants.OUTCONTEXT_OPT1_DOWNLOAD_CERTIFICATE) > 0) {
            cert = ByteBufferUtils.extractByteArray(bbBuf);
        }
        else if ((optionFlags1 & ServerConstants.OUTCONTEXT_OPT1_EXTRA_OPTIONS) > 0) {
            try {
                this.decodeExtraOptions(ByteBufferUtils.extractString(bbBuf));
            } catch (UnsupportedEncodingException ue) {
                LOG.error("An error occured parsing OutConnectionContext: " + ue.getMessage());
                throw ue;
            }
        }
    }
    public void decodeExtraOptions(String options) {
        String[] opts = options.split(";");
        String token;
        String value;
        int index;

        for (int i = 0; i < opts.length; i++) {
            index = opts[i].indexOf('=');
            token = opts[i].substring(0, index).toUpperCase();
            value = opts[i].substring(index + 1);

            if (token.equals("RN")) {
                this._roleName = value;
            }
        }
    }
    public void insertIntoByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException {
        
        versionList.insertIntoByteBuffer(bbBuf);
        bbBuf.putShort(nodeId);
        bbBuf.putInt(processId);
        ByteBufferUtils.insertString(computerName, bbBuf);

        ByteBufferUtils.insertString(catalog, bbBuf);
        ByteBufferUtils.insertString(schema, bbBuf);

        ByteBufferUtils.insertUInt(optionFlags1,bbBuf);
        ByteBufferUtils.insertUInt(optionFlags2,bbBuf);
        
        ByteBufferUtils.insertByteArray(cert, bbBuf);
    }
    public int lengthOfData() {

        int dataLength = 0;
        dataLength += versionList.lengthOfData();                                       //versionList

        dataLength += ServerConstants.SHORT_FIELD_SIZE;                                       //nodeId
        dataLength += ServerConstants.INT_FIELD_SIZE;                                         //processId
        dataLength += ByteBufferUtils.lengthOfString(computerName);                        //computerName
        dataLength += ByteBufferUtils.lengthOfString(catalog);                             //catalog
        dataLength += ByteBufferUtils.lengthOfString(schema);                              //schema
        dataLength += ServerConstants.INT_FIELD_SIZE;                                         //optionFlags1
        dataLength += ServerConstants.INT_FIELD_SIZE;                                         //optionFlags2
        dataLength += ByteBufferUtils.lengthOfByteArray(cert);                              //certificate
        return dataLength;
    }
    public VersionList getVersionList(){
        return versionList;
    }
    public void setNodeId(short nodeId){
        this.nodeId = nodeId;
    }
    public void setProcessId(int processId){
        this.processId = processId;
    }
    public void setComputerName(String computerName){
        this.computerName = computerName;        
    }
    public void setCatalog(String catalog){
        this.catalog = catalog;        
    }
    public void setSchema(String schema){
        this.schema = schema;        
    }
    public void setOptionFlags1(long optionFlags1){
        this.optionFlags1 = optionFlags1;        
    }
    public void setOptionFlags2(long optionFlags2){
        this.optionFlags2 = optionFlags2;        
    }
    public void setRoleName(String _roleName){
        this._roleName = _roleName;        
    }
}
