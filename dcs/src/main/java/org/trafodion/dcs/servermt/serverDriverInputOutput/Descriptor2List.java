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
    private int length;
    private long paramLength;
    private long nullLength;
    private long noNullLength;
    private int numberParams;
    private Descriptor2[] buffer;
    private boolean oldFormat;
    private int lengthOldFormat;        //for Catalogs Api we need to convert Descriptor2 to Old Format
    
    public Descriptor2List(){
        length = 0;
        paramLength = 0;
        numberParams = 0; 
        buffer = null;
        oldFormat = false;
        lengthOldFormat = 0;
    }
    public Descriptor2List(int numberParams, boolean oldFormat){
        this.oldFormat = oldFormat;
        lengthOldFormat = ServerConstants.INT_FIELD_SIZE; //old SQLItemDescList
        this.length = 3 * ServerConstants.INT_FIELD_SIZE;
        this.paramLength = 0;
        this.nullLength = 0;        //length of null buffer. every description needs short 
        this.noNullLength = 0;        //length of nonull buffer. 
        this.numberParams = numberParams; 
        buffer = new Descriptor2[numberParams];
    }
    public Descriptor2List(Descriptor2List dl){
        oldFormat = dl.oldFormat;
        lengthOldFormat = dl.lengthOldFormat;
        length = dl.length;
        paramLength = dl.paramLength;
        nullLength = dl.nullLength;
        noNullLength = dl.noNullLength;
        numberParams = dl.numberParams;
        buffer = new Descriptor2[numberParams];
        for (int i = 0; i < numberParams; i++)
            buffer[i] = dl.buffer[i];
    }
    public void addDescriptor(int param, Descriptor2 dsc){
        if(LOG.isDebugEnabled())
            LOG.debug("addDescriptor param :" + param + " numberParams :" + numberParams);
        length += dsc.lengthOfData();
        if (dsc.getNullInfo() == 1){        //nullable
            dsc.setNullValue((int)nullLength);
            nullLength += 2;
        }
        else {
            dsc.setNullValue(-1);            //nonullable
        }
        dsc = getMemoryAllocInfo(dsc, noNullLength);
        noNullLength += dsc.getMemAlignOffset();
        noNullLength = ((noNullLength + 2 - 1) >> 1) << 1;
        noNullLength += ServerConstants.SHORT_FIELD_SIZE;
        dsc.setNoNullValue((int)noNullLength);
        buffer[param - 1] = dsc;
        
        noNullLength += dsc.getAllocSize();
        if (numberParams == param){
            Descriptor2 tmpdsc = null;
            long tmpNoNullLength = 0;
            
            nullLength = ((nullLength + 8 - 1) >> 3) << 3;
            noNullLength = ((noNullLength + 8 - 1) >> 3) << 3;
            paramLength = nullLength + noNullLength;
            if(LOG.isDebugEnabled()){
                LOG.debug("nullLength :" + nullLength);
                LOG.debug("noNullLength :" + noNullLength);
                LOG.debug("maxLen :" + paramLength);
            }
            for (int i = 0; i < numberParams; i++) {
                tmpdsc = buffer[i];
                tmpNoNullLength = tmpdsc.getNoNullValue();
                tmpdsc.setNoNullValue((int)(nullLength + tmpNoNullLength));
                buffer[i] = tmpdsc;
                if(LOG.isDebugEnabled()){
                    LOG.debug("param :" + (i+1));
                    LOG.debug("noNullValue :" + tmpdsc.getNoNullValue());
                    LOG.debug("nullValue :" + tmpdsc.getNullValue());
                    LOG.debug("maxLen :" + tmpdsc.getMaxLen());
                }
            }
        }
    }
    public void insertIntoByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException {
        if (oldFormat == false){
            if(LOG.isDebugEnabled())
                LOG.debug("length :" + length + " paramLength :" + paramLength + " numberParams :" + numberParams);
            bbBuf.putInt(length);
            if (length > 0){
                bbBuf.putInt((int)paramLength);
                bbBuf.putInt(numberParams);
                for (int i = 0; i < numberParams; i++) {
                    buffer[i].insertIntoByteBuffer(bbBuf);
                }
            }
         }
        else {
            if(LOG.isDebugEnabled())
                 LOG.debug("numberParams :" + numberParams);
            bbBuf.putInt(numberParams);
            if (numberParams > 0){
                for (int i = 0; i < numberParams; i++) {
                    buffer[i].insertIntoByteBuffer(bbBuf);
                }
            }
        }
    }
    public int lengthOfData() {
        if (oldFormat == false)
            return length;        
        else
            return lengthOldFormat;
    }
    public void setOldFormat(boolean oldFormat){
        this.oldFormat = oldFormat;
    }
    public boolean getOldFormat(){
        return oldFormat;
    }
    public void  setParamLength(long paramLength){
        this.paramLength = paramLength;
    }
    public Descriptor2[] getDescriptors2(){
        return buffer;
    }
    public int getLength(){
        return length;
    }
    public long getParamLength(){
        return paramLength;
    }
    public int getNumberParams(){
        return numberParams;
    }
//    
// Compute the memory allocation requirements for the descriptor data type
//   
    Descriptor2 getMemoryAllocInfo(Descriptor2 desc, long currMemOffset) {
        int dataType = desc.getDataType();
        int dataLength = desc.getMaxLen();
        int charSet = desc.getSqlCharset();
//
        int varPad = 0;            // Bytes to pad allocation for actual data type memory requirements
        int varNulls = 0;        // Number of extra bytes that will be appended to data type (e.g. NULL for strings)
        long memAlignOffset = 0;    // Boundry offset from current memory location to set the data pointer
        int allocBoundry = 0;    // Boundry to round the size of the memory allocation to end on proper boundry
        int allocSize = 0;
        int varLayout = 0;

        switch (dataType)
        {
        case ServerConstants.SQLTYPECODE_CHAR:
        case ServerConstants.SQLTYPECODE_VARCHAR:
            if( charSet == ServerConstants.sqlCharsetCODE_ISO88591 )
                varNulls = 1;
            break;
        case ServerConstants.SQLTYPECODE_VARCHAR_WITH_LENGTH:
        case ServerConstants.SQLTYPECODE_VARCHAR_LONG:
            memAlignOffset = (((currMemOffset + 2 - 1) >> 1) << 1) - currMemOffset;
            varPad = 2;
            varNulls = 1;
            allocBoundry = 2;
            break;
        case ServerConstants.SQLTYPECODE_SMALLINT:
        case ServerConstants.SQLTYPECODE_SMALLINT_UNSIGNED:
            memAlignOffset = (((currMemOffset + 2 - 1) >> 1) << 1) - currMemOffset;
            break;
        case ServerConstants.SQLTYPECODE_INTEGER:
        case ServerConstants.SQLTYPECODE_INTEGER_UNSIGNED:
            memAlignOffset = (((currMemOffset + 4 - 1) >> 2) << 2) - currMemOffset;
            break;
        case ServerConstants.SQLTYPECODE_LARGEINT:
        case ServerConstants.SQLTYPECODE_REAL:
        case ServerConstants.SQLTYPECODE_DOUBLE:
            memAlignOffset = (((currMemOffset + 8 - 1) >> 3) << 3) - currMemOffset;
            break;
        case ServerConstants.SQLTYPECODE_DECIMAL_UNSIGNED:
        case ServerConstants.SQLTYPECODE_DECIMAL:
        case ServerConstants.SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
        case ServerConstants.SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
            break;
        case ServerConstants.SQLTYPECODE_INTERVAL:        // Treating as CHAR
        case ServerConstants.SQLTYPECODE_DATETIME:
            memAlignOffset = (((currMemOffset + 2 - 1) >> 1) << 1) - currMemOffset;
            varPad = 2;
            varNulls = 1;
            allocBoundry = 2;
            break;
        default:
            memAlignOffset = (((currMemOffset + 8 - 1) >> 3) << 3) - currMemOffset;
            break;
        }
        varLayout = dataLength + varNulls;
        allocSize = varLayout + varPad;
        if (allocBoundry != 0) allocSize += allocSize % allocBoundry;
        
        desc.setMemAlignOffset(memAlignOffset);
        desc.setAllocSize(allocSize);
        desc.setVarLayout(varLayout);

        if(LOG.isDebugEnabled()){
            LOG.debug("input currMemOffset :" + currMemOffset);
            LOG.debug("input dataType :" + SqlUtils.getSqlDataType(dataType) + " [" + dataType + "]" );
            LOG.debug("input dataLength :" + dataLength);
            LOG.debug("input SqlCharsetSTRING :" + SqlUtils.getCharsetName(charSet) + " [" + charSet + "]");
            LOG.debug("tmp varNulls :" + varNulls);
            LOG.debug("tmp varPad :" + varPad);
            LOG.debug("tmp allocBoundry :" + allocBoundry);
            LOG.debug("output memAlignOffset :" + memAlignOffset);
            LOG.debug("output allocSize :" + allocSize);
            LOG.debug("output varLayout :" + varLayout);
        }

        return desc;
    }
}
