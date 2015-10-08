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

public class Descriptor2List {
    private static  final Log LOG = LogFactory.getLog(Descriptor2List.class);
    private long varLength;
    private int descLength;
    private int descCount;
    private Descriptor2[] buffer;
    private boolean oldFormat;

    public Descriptor2List(){
        varLength = 0;
        descLength = 0;
        descCount = 0;
        buffer = null;
        oldFormat = false;
    }
    public Descriptor2List(int descCount, boolean oldFormat){
        this.varLength = 0;
        this.descLength = 0;
        this.descCount = descCount;
        this.oldFormat = oldFormat;
        buffer = new Descriptor2[descCount];
    }
    public Descriptor2List(Descriptor2List dl){
        descLength = dl.descLength;
        varLength = dl.varLength;
        descCount = dl.descCount;
        oldFormat = dl.oldFormat;
        buffer = new Descriptor2[descCount];
        for (int i = 0; i < descCount; i++)
            buffer[i] = dl.buffer[i];
    }
    public void addDescriptor(int descNumber, Descriptor2 dsc){

        if(LOG.isDebugEnabled())
            LOG.debug("addDescriptor descNumber :" + descNumber + " descCount :" + descCount);
        buffer[descNumber - 1] = dsc;

        if (descCount == descNumber){
            Descriptor2 desc = null;
            if (oldFormat == false)
                descLength = 3 * ServerConstants.INT_FIELD_SIZE;
            else
                descLength = ServerConstants.INT_FIELD_SIZE;
            varLength = 0;

            for (int i = 0; i < descCount; i++) {
                desc = buffer[i];
                descLength += desc.lengthOfData();

                if (oldFormat == false){
                    if (desc.getNullInfo() == 1){        //nullable
                        varLength = ((varLength + 2 - 1) >> 1) << 1;
                        desc.setNullValue((int)varLength);
                        varLength += 2;
                    }
                    else {
                        desc.setNullValue(-1);            //nonullable
                    }
                    desc = setVarLength(desc, varLength);
                    varLength = desc.getVarLength();
                } else {
                    desc = setVarLength(desc, varLength);
                    varLength = desc.getVarLength();
                }
                if(LOG.isDebugEnabled()){
                   LOG.debug("--------desc :" + (i+1));
                   LOG.debug("varLength :" + varLength);
                   LOG.debug("noNullValue :" + desc.getNoNullValue());
                   LOG.debug("nullValue :" + desc.getNullValue());
                   LOG.debug("maxLen :" + desc.getMaxLen());
                }
                buffer[i] = desc;
            }
        }
    }
    public void insertIntoByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException {
        if (oldFormat == false){
            bbBuf.putInt(descLength);
            if (descLength > 0){
                bbBuf.putInt((int)varLength);          // param length
                bbBuf.putInt(descCount);                // param count
                for (int i = 0; i < descCount; i++) {
                    buffer[i].insertIntoByteBuffer(bbBuf);
                }
            }
         }
        else {
            if(LOG.isDebugEnabled())
                 LOG.debug("descCount :" + descCount);
            bbBuf.putInt(descCount);
            if (descCount > 0){
                for (int i = 0; i < descCount; i++) {
                    buffer[i].insertIntoByteBuffer(bbBuf);
                }
            }
        }
    }
    public int lengthOfData() {
        return descLength;
    }
    public void setOldFormat(boolean oldFormat){
        this.oldFormat = oldFormat;
    }
    public boolean getOldFormat(){
        return oldFormat;
    }
    public void  setDescLength(int descLength){
        this.descLength = descLength;
    }
    public Descriptor2[] getDescriptors2(){
        return buffer;
    }
    public long getVarLength(){
        return varLength;
    }
    public int getDescLength(){
        return descLength;
    }
    public int getDescCount(){
        return descCount;
    }
    Descriptor2 setVarLength(Descriptor2 desc, long memOffSet) {
        int dataType = desc.getDataType();
        int dataLength = desc.getMaxLen();

        switch (dataType)
        {
        case ServerConstants.SQLTYPECODE_CHAR:
        case ServerConstants.SQLTYPECODE_VARCHAR:
            desc.setNoNullValue((int)memOffSet);
            memOffSet += dataLength;
            break;
        case ServerConstants.SQLTYPECODE_VARCHAR_WITH_LENGTH:
            if( dataLength > Short.MAX_VALUE )
            {
                if (oldFormat == false){
                    memOffSet = ((memOffSet + 4 - 1) >> 2) << 2;
                    desc.setNoNullValue((int)memOffSet);
                }
                memOffSet += dataLength + 4;
            }
            else
            {
                if (oldFormat == false){
                    memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
                    desc.setNoNullValue((int)memOffSet);
                }
                memOffSet += dataLength + 2;
            }
            break;
        case ServerConstants.SQLTYPECODE_VARCHAR_LONG:
            if (oldFormat == false){
                memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
                desc.setNoNullValue((int)memOffSet);
            }
            memOffSet += dataLength + 2;
            break;
        case ServerConstants.SQLTYPECODE_SMALLINT:
        case ServerConstants.SQLTYPECODE_SMALLINT_UNSIGNED:
            if (oldFormat == false){
                memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
                desc.setNoNullValue((int)memOffSet);
            }
            memOffSet += dataLength;
            break;
        case ServerConstants.SQLTYPECODE_INTEGER:
        case ServerConstants.SQLTYPECODE_INTEGER_UNSIGNED:
        //case SQLTYPECODE_IEEE_REAL:
            if (oldFormat == false){
                memOffSet = ((memOffSet + 4 - 1) >> 2) << 2;
                desc.setNoNullValue((int)memOffSet);
            }
            memOffSet += dataLength;
            break;
        case ServerConstants.SQLTYPECODE_LARGEINT:
        case ServerConstants.SQLTYPECODE_IEEE_REAL:
        case ServerConstants.SQLTYPECODE_IEEE_FLOAT:
        case ServerConstants.SQLTYPECODE_IEEE_DOUBLE:
            if (oldFormat == false){
                memOffSet = ((memOffSet + 8 - 1) >> 3) << 3;
                desc.setNoNullValue((int)memOffSet);
            }
            memOffSet += dataLength;
            break;
        case ServerConstants.SQLTYPECODE_DECIMAL_UNSIGNED:
        case ServerConstants.SQLTYPECODE_DECIMAL:
        case ServerConstants.SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
        case ServerConstants.SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
        case ServerConstants.SQLTYPECODE_INTERVAL:      // Treating as CHAR
        case ServerConstants.SQLTYPECODE_DATETIME:
            if (oldFormat == false){
                desc.setNoNullValue((int)memOffSet);
            }
            memOffSet += dataLength;
            break;
        default:
            if (oldFormat == false){
                memOffSet = ((memOffSet + 8 - 1) >> 3) << 3;
                desc.setNoNullValue((int)memOffSet);
            }
            memOffSet += dataLength;
            break;
        }
        desc.setVarLength(memOffSet);
        return desc;
    }

}
