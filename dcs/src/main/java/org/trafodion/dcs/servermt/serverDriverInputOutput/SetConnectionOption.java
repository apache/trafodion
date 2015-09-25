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
import java.io.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.serverHandler.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class SetConnectionOption  {
    private static  final Log LOG = LogFactory.getLog(SetConnectionOption.class);
    
    private int dialogueId = 0;
    private short connectionOption = 0;
    private int optionValueNum = 0;
    private String optionValueBytes = "";
    
    public void extractFromByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException{
        try {
            dialogueId = bbBuf.getInt();
            connectionOption = bbBuf.getShort();
            optionValueNum = bbBuf.getInt();
            optionValueBytes = ByteBufferUtils.extractString(bbBuf);
            debugUserDesc("extract");
        } catch(UnsupportedEncodingException ue){
            LOG.error("SetConnectionOption : UnsupportedEncodingException :" + ue);
            throw ue;
        }
    }
    public void debugUserDesc(String function){
        if(LOG.isDebugEnabled()){
            LOG.debug("Function :" + function);
            LOG.debug("dialogueId :"+dialogueId);
            LOG.debug("connectionOption :"+connectionOption);
            LOG.debug("optionValueNum :"+optionValueNum);
            LOG.debug("optionValueBytes :"+optionValueBytes);
        }
    }
    public int getDialogueId(){
        return dialogueId;
    }
    public short getConnectionOption(){
        return connectionOption;
    }
    public int getOptionValueNum(){
        return optionValueNum;
    }
    public String getOptionValueBytes(){
        return optionValueBytes;
    }
}
