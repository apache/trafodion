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
import java.sql.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverHandler.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class Descriptor2 {

    private static  final Log LOG = LogFactory.getLog(Descriptor2.class);
    private boolean oldFormat = false;
//================== T2 desc fields ============================================
    private int        sqlCharset_;
    private int        odbcCharset_;
    private int        sqlDataType_;
    private int        dataType_;
    private short    sqlPrecision_;
    private short    sqlDatetimeCode_;
    private int        sqlOctetLength_;
    private int        isNullable_;
    private String    name_;
    private int        scale_;
    private int        precision_;
    private boolean    isSigned_;
    private boolean    isCurrency_;
    private boolean    isCaseSensitive_;
    private String     catalogName_;
    private String    schemaName_;
    private String    tableName_;
    private int        fsDataType_;
    private int        intLeadPrec_;
    private int        paramMode_;
    private int        paramIndex_;
    private int        paramPos_;
//====== Added zo
    private int        odbcPrecision_;
    private int        maxLen_;
    
    private int        displaySize_;
    private String    label_;
//================== T4 desc fields ============================================
    private int     noNullValue;
    private int     nullValue;
    private int     version;
    private int     dataType;
    private int     datetimeCode;
    private int     maxLen;
    private int     precision;
    private int     scale;
    private int     nullInfo;
    private int     signed;
    private int     odbcDataType;
    private int     odbcPrecision;
    private int     sqlCharset;
    private int     odbcCharset;
    private String     colHeadingNm;
    private String     tableName;
    private String     catalogName;
    private String     schemaName;
    private String     headingName;
    private int     intLeadPrec;
    private int     paramMode;
// temp values
    private long     memAlignOffset;
    private int     allocSize;
    private int     varLayout;
    
    public Descriptor2(int sqlCharset_, int odbcCharset_, int sqlDataType_, int dataType_, short sqlPrecision_, short sqlDatetimeCode_,
            int sqlOctetLength_,int isNullable_,String name_,int scale_,int precision_,boolean isSigned_,
            boolean isCurrency_,boolean isCaseSensitive_,String catalogName_,String schemaName_,String tableName_,
            int fsDataType_,int intLeadPrec_,int paramMode_,int paramIndex_,int paramPos_,int odbcPrecision_,
            int maxLen_,int displaySize_,String label_, boolean oldFormat){
        
        this.oldFormat = oldFormat;
        this.sqlCharset_ = sqlCharset_;
        this.odbcCharset_ = odbcCharset_;
        this.sqlDataType_ = sqlDataType_;
        if (dataType_ == ServerConstants.SQLTYPECODE_NUMERIC)
            this.dataType_ = ServerConstants.SQLTYPECODE_INTEGER;
        else
            this.dataType_ = dataType_;
        this.sqlPrecision_ = sqlPrecision_;
        this.sqlDatetimeCode_ = sqlDatetimeCode_;
        this.sqlOctetLength_ = sqlOctetLength_;
        this.isNullable_ = isNullable_;
        this.name_ = name_;
        this.scale_ = scale_;
        this.precision_ = precision_;
        this.isSigned_ = isSigned_;
        this.isCurrency_ = isCurrency_;
        this.isCaseSensitive_ = isCaseSensitive_;
        this.catalogName_ = catalogName_;
        this.schemaName_ = schemaName_;
        this.tableName_ = tableName_;
        this.fsDataType_ = fsDataType_;
        this.intLeadPrec_ = intLeadPrec_;
        this.paramMode_ = paramMode_;
        this.paramIndex_ = paramIndex_;
        this.paramPos_ = paramPos_;
        this.odbcPrecision_ = odbcPrecision_;
        this.maxLen_ = maxLen_;
        this.displaySize_ = displaySize_;
        this.label_ = label_;
//====================================================================
        if(LOG.isDebugEnabled()){
            LOG.debug("T2 descriptor ----------");
            LOG.debug("Old Format       :" + oldFormat);
            LOG.debug("sqlCharset_      :" + sqlCharset_);
            LOG.debug("odbcCharset_     :" + odbcCharset_);
            LOG.debug("sqlDataType_     :" + sqlDataType_ + " :" + SqlUtils.getSqlDataType(sqlDataType_));
            LOG.debug("dataType_        :" + dataType_ + " :" + SqlUtils.getDataType(dataType_));
            LOG.debug("sqlPrecision_    :" + sqlPrecision_);
            LOG.debug("sqlDatetimeCode_ :" + sqlDatetimeCode_);
            LOG.debug("sqlOctetLength_  :" + sqlOctetLength_);
            LOG.debug("isNullable_      :" + isNullable_);
            LOG.debug("name_            :" + name_);
            LOG.debug("scale_           :" + scale_);
            LOG.debug("precision_       :" + precision_);
            LOG.debug("isSigned_        :" + isSigned_);
            LOG.debug("isCurrency_      :" + isCurrency_);
            LOG.debug("isCaseSensitive_ :" + isCaseSensitive_);
            LOG.debug("catalogName_     :" + catalogName_);
            LOG.debug("schemaName_      :" + schemaName_);
            LOG.debug("tableName_       :" + tableName_);
            LOG.debug("fsDataType_      :" + fsDataType_);
            LOG.debug("intLeadPrec_     :" + intLeadPrec_);
            LOG.debug("paramMode_       :" + paramMode_);
            LOG.debug("paramIndex_      :" + paramIndex_);
            LOG.debug("paramPos_        :" + paramPos_);
            LOG.debug("odbcPrecision_   :" + odbcPrecision_);
            LOG.debug("maxLen_          :" + maxLen_);
            LOG.debug("displaySize_     :" + displaySize_);
            LOG.debug("label_           :" + label_);
            LOG.debug("T2 descriptor End ----------");
        }
//=====================================================
        noNullValue = -1;
        nullValue = -1;
        version = 0;
        dataType = sqlDataType_;
        datetimeCode = sqlDatetimeCode_;
        maxLen = maxLen_;
        precision = sqlPrecision_;
        scale = scale_;
        nullInfo = isNullable_;
        signed = isSigned_ == true ? 1 : 0;
        odbcDataType = dataType_;
        odbcPrecision = odbcPrecision_;
        sqlCharset = sqlCharset_;
        odbcCharset = odbcCharset_;
        colHeadingNm = name_;
        tableName = tableName_;
        catalogName = catalogName_;
        schemaName = schemaName_;
        headingName = name_;
        intLeadPrec = intLeadPrec_;
        paramMode = paramMode_;

        if (dataType == ServerConstants.SQLTYPECODE_NUMERIC || dataType == ServerConstants.SQLTYPECODE_NUMERIC_UNSIGNED)
        {
            switch (fsDataType_)
            {
                case 130:
                    dataType = ServerConstants.SQLTYPECODE_SMALLINT;
                    break;
                case 131:
                    dataType = ServerConstants.SQLTYPECODE_SMALLINT_UNSIGNED;
                    break;
                case 132:
                    dataType = ServerConstants.SQLTYPECODE_INTEGER;
                    break;
                case 133:
                    dataType = ServerConstants.SQLTYPECODE_INTEGER_UNSIGNED;
                    break;
                case 134:
                    dataType = ServerConstants.SQLTYPECODE_LARGEINT;
                    break;                              
                default:
                    break;
            }
        }
        switch(odbcDataType){
            case Types.DATE:
                odbcDataType = 9;
                break;
            case Types.TIME:
                odbcDataType = 10;
                precision = odbcPrecision;
                break;
            case Types.TIMESTAMP:
                odbcDataType = 11;
                precision = odbcPrecision;
                break;
        }
//================================================================        
        memAlignOffset = 0;
        allocSize = 0;
        varLayout = 0;
        
        if(LOG.isDebugEnabled()){
            LOG.debug("T4 descriptor ----------");
            LOG.debug("noNullValue      :" + noNullValue);
            LOG.debug("nullValue        :" + nullValue);
            LOG.debug("version          :" + version);
            LOG.debug("dataType         :" + dataType);
            LOG.debug("datetimeCode     :" + datetimeCode);
            LOG.debug("maxLen           :" + maxLen);
            LOG.debug("precision        :" + precision);
            LOG.debug("scale            :" + scale);
            LOG.debug("nullInfo         :" + nullInfo);
            LOG.debug("signed           :" + signed);
            LOG.debug("odbcDataType     :" + odbcDataType);
            LOG.debug("odbcPrecision    :" + odbcPrecision);
            LOG.debug("sqlCharset       :" + sqlCharset);
            LOG.debug("odbcCharset      :" + odbcCharset);
            LOG.debug("colHeadingNm     :" + colHeadingNm);
            LOG.debug("tableName        :" + tableName);
            LOG.debug("schemaName       :" + schemaName);
            LOG.debug("headingName      :" + headingName);
            LOG.debug("intLeadPrec      :" + intLeadPrec);
            LOG.debug("paramMode        :" + paramMode);
            LOG.debug("memAlignOffset   :" + memAlignOffset);
            LOG.debug("allocSize        :" + allocSize);
            LOG.debug("varLayout        :" + varLayout);
            LOG.debug("T4 descriptor End ----------");
        }
    }
//==========================================================================
    public Descriptor2(Descriptor2 dsc){
        this.oldFormat = dsc.oldFormat;
        this.sqlCharset_ = dsc.sqlCharset_;
        this.odbcCharset_ = dsc.odbcCharset_;
        this.sqlDataType_ = dsc.sqlDataType_;
        this.dataType_ = dsc.dataType_;
        this.sqlPrecision_ = dsc.sqlPrecision_;
        this.sqlDatetimeCode_ = dsc.sqlDatetimeCode_;
        this.sqlOctetLength_ = dsc.sqlOctetLength_;
        this.isNullable_ = dsc.isNullable_;
        this.name_ = dsc.name_;
        this.scale_ = dsc.scale_;
        this.precision_ = dsc.precision_;
        this.isSigned_ = dsc.isSigned_;
        this.isCurrency_ = dsc.isCurrency_;
        this.isCaseSensitive_ = dsc.isCaseSensitive_;
        this.catalogName_ = dsc.catalogName_;
        this.schemaName_ = dsc.schemaName_;
        this.tableName_ = dsc.tableName_;
        this.fsDataType_ = dsc.fsDataType_;
        this.intLeadPrec_ = dsc.intLeadPrec_;
        this.paramMode_ = dsc.paramMode_;
        this.paramIndex_ = dsc.paramIndex_;
        this.paramPos_ = dsc.paramPos_;
        this.odbcPrecision_ = dsc.odbcPrecision_;
        this.maxLen_ = dsc.maxLen_;
        this.displaySize_ = dsc.displaySize_;
        this.label_ = dsc.label_;
//====================================================================
        this.noNullValue = dsc.noNullValue;
        this.nullValue = dsc.nullValue;
        this.version = dsc.version;
        this.dataType = dsc.dataType;
        this.datetimeCode = dsc.datetimeCode;
        this.maxLen = dsc.maxLen;
        this.precision = dsc.precision;
        this.scale = dsc.scale;
        this.nullInfo = dsc.nullInfo;
        this.signed = dsc.signed;
        this.odbcDataType = dsc.odbcDataType;
        this.odbcPrecision = dsc.odbcPrecision;
        this.sqlCharset = dsc.sqlCharset;
        this.odbcCharset = dsc.odbcCharset;
        this.colHeadingNm = dsc.colHeadingNm;
        this.tableName = dsc.tableName;
        this.catalogName = dsc.catalogName;
        this.schemaName = dsc.schemaName;
        this.headingName = dsc.headingName;
        this.intLeadPrec = dsc.intLeadPrec;
        this.paramMode = dsc.paramMode;
        
        memAlignOffset = 0;
        allocSize = 0;
        varLayout = 0;
        
    }
    public void insertIntoByteBuffer(ByteBuffer bbBuf) throws UnsupportedEncodingException {
        if (oldFormat == false){
            bbBuf.putInt(noNullValue);
            bbBuf.putInt(nullValue);
            bbBuf.putInt(version);
            bbBuf.putInt(dataType);
            bbBuf.putInt(datetimeCode);
            bbBuf.putInt(maxLen);
            bbBuf.putInt(precision);
            bbBuf.putInt(scale);
            bbBuf.putInt(nullInfo);
            bbBuf.putInt(signed);
            bbBuf.putInt(odbcDataType);
            bbBuf.putInt(odbcPrecision);
            bbBuf.putInt(sqlCharset);
            bbBuf.putInt(odbcCharset);
            ByteBufferUtils.insertString(colHeadingNm,bbBuf);
            ByteBufferUtils.insertString(tableName,bbBuf);
            ByteBufferUtils.insertString(catalogName,bbBuf);
            ByteBufferUtils.insertString(schemaName,bbBuf);
            ByteBufferUtils.insertString(headingName,bbBuf);
            bbBuf.putInt(intLeadPrec);
            bbBuf.putInt(paramMode);
        }
        else {
            bbBuf.putInt(version);
            bbBuf.putInt(dataType);
            bbBuf.putInt(datetimeCode);
            bbBuf.putInt(maxLen);
            bbBuf.putShort((short)precision);
            bbBuf.putShort((short)scale);
            bbBuf.put((byte)nullInfo);
            ByteBufferUtils.insertString(colHeadingNm,bbBuf);
            bbBuf.put((byte)signed);
            bbBuf.putInt(odbcDataType);
            bbBuf.putShort((short)odbcPrecision);
            bbBuf.putInt(sqlCharset);
            bbBuf.putInt(odbcCharset);
            ByteBufferUtils.insertString(tableName,bbBuf);
            ByteBufferUtils.insertString(catalogName,bbBuf);
            ByteBufferUtils.insertString(schemaName,bbBuf);
            ByteBufferUtils.insertString(headingName,bbBuf);
            bbBuf.putInt(intLeadPrec);
            bbBuf.putInt(paramMode);
        }
    }
    public int lengthOfData() {
        int datamaxLen = 0;
        
        if (oldFormat == false){
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ByteBufferUtils.lengthOfString(colHeadingNm);
            datamaxLen += ByteBufferUtils.lengthOfString(tableName);
            datamaxLen += ByteBufferUtils.lengthOfString(catalogName);
            datamaxLen += ByteBufferUtils.lengthOfString(schemaName);
            datamaxLen += ByteBufferUtils.lengthOfString(headingName);
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
            datamaxLen += ServerConstants.INT_FIELD_SIZE;
        }
        else {
            datamaxLen += ServerConstants.INT_FIELD_SIZE;       //version
            datamaxLen += ServerConstants.INT_FIELD_SIZE;       //dataType
            datamaxLen += ServerConstants.INT_FIELD_SIZE;       //datetimeCode
            datamaxLen += ServerConstants.INT_FIELD_SIZE;       //maxLen
            datamaxLen += ServerConstants.SHORT_FIELD_SIZE;       //precision
            datamaxLen += ServerConstants.SHORT_FIELD_SIZE;       //scale
            datamaxLen += ServerConstants.BYTE_FIELD_SIZE;       //nullInfo
            datamaxLen += ByteBufferUtils.lengthOfString(colHeadingNm);
            datamaxLen += ServerConstants.BYTE_FIELD_SIZE;       //signed
            datamaxLen += ServerConstants.INT_FIELD_SIZE;       //odbcDataType
            datamaxLen += ServerConstants.SHORT_FIELD_SIZE;       //odbcPrecision
            datamaxLen += ServerConstants.INT_FIELD_SIZE;       //sqlCharset
            datamaxLen += ServerConstants.INT_FIELD_SIZE;       //odbcCharset
            datamaxLen += ByteBufferUtils.lengthOfString(tableName);
            datamaxLen += ByteBufferUtils.lengthOfString(catalogName);
            datamaxLen += ByteBufferUtils.lengthOfString(schemaName);
            datamaxLen += ByteBufferUtils.lengthOfString(headingName);
            datamaxLen += ServerConstants.INT_FIELD_SIZE;       //intLeadPrec
            datamaxLen += ServerConstants.INT_FIELD_SIZE;       //paramMode
        }
        return datamaxLen;
    }
    public void setOldFormat(boolean oldFormat){
        this.oldFormat = oldFormat;
    }
    public boolean getOldFormat(){
        return oldFormat;
    }
    public void setNoNullValue(int v){
        noNullValue = v;
    }
    public void setNullValue(int v){
        nullValue = v;        
    }
    public void setVersion(int v){
        version = v;         
    }
    public void setDataType(int v){
        dataType = v;         
    }
    public void setDatetimeCode(int v){
        datetimeCode = v;         
    }
    public void setMaxLen(int v){
        maxLen = v;         
    }
    public void setPrecision(int v){
        precision = v;         
    }
    public void setScale(int v){
        scale = v;         
    }
    public void setNullInfo(int v){
        nullInfo = v;         
    }
    public void setSigned(int v){
        signed = v;         
    }
    public void setOdbcDataType(int v){
        odbcDataType = v;        
    }
    public void setOdbcPrecision(int v){
        odbcPrecision = v;        
    }
    public void setSqlCharset(int v){
        sqlCharset = v;         
    }
    public void setOdbcCharset(int v){
        odbcCharset = v;        
    }
    public void setColHeadingNm(String v){
        colHeadingNm = v;         
    }
    public void setTableName(String v){
        tableName = v;         
    }
    public void setCatalogName(String v){
        catalogName = v;         
    }
    public void setSchemaName(String v){
        schemaName = v;         
    }
    public void setHeadingName(String v){
        headingName = v;         
    }
    public void setIntLeadPrec(int v){
        intLeadPrec = v;         
    }
    public void setParamMode(int v){
        paramMode = v;        
    }
//--------------------------------
    public void setMemAlignOffset(long memAlignOffset){
        this.memAlignOffset = memAlignOffset;
    }
    public void setAllocSize(int allocSize){
        this.allocSize = allocSize;
    }
    public void setVarLayout(int varLayout){
        this.varLayout = varLayout;
    }
//-----------------------------
    public int getNoNullValue(){
        return noNullValue;
    }
    public int getNullValue(){
        return nullValue;        
    }
    public int getVersion(){
        return version;        
    }
    public int getDataType(){
        return dataType;        
    }
    public int getDatetimeCode(){
        return datetimeCode;        
    }
    public int getMaxLen(){
        return maxLen;        
    }
    public int getPrecision(){
        return precision;        
    }
    public int getScale(){
        return scale;        
    }
    public int getNullInfo(){
        return nullInfo;        
    }
    public int getSigned(){
        return signed;        
    }
    public int getOdbcDataType(){
        return odbcDataType;        
    }
    public int getOdbcPrecision(){
        return odbcPrecision;        
    }
    public int getSqlCharset(){
        return sqlCharset;        
    }
    public int getOdbcCharset(){
        return odbcCharset;        
    }
    public String getColHeadingNm(){
        return colHeadingNm;        
    }
    public String getTableName(){
        return tableName;        
    }
    public String getCatalogName(){
        return catalogName;        
    }
    public String getSchemaName(){
        return schemaName;        
    }
    public String getHeadingName(){
        return headingName;        
    }
    public int getIntLeadPrec(){
        return intLeadPrec;        
    }
    public int getParamMode(){
        return paramMode;        
    }
//--------------------------------
    public long getMemAlignOffset(){
        return memAlignOffset;
    }
    public int getAllocSize(){
        return allocSize;
    }
    public int getVarLayout(){
        return varLayout;
    }
//-----------------------------
    public int getFsDataType(){
        return fsDataType_;
    }
//-----------------------------
    public void debugDescriptor(){
        if(LOG.isDebugEnabled()){
            LOG.debug("T4 descriptor -----------");
            LOG.debug("Old Format       :" + oldFormat);
            LOG.debug("noNullValue      :" + noNullValue);
            LOG.debug("nullValue        :" + nullValue);
            LOG.debug("version          :" + version);
            LOG.debug("dataType         :" + dataType);
            LOG.debug("datetimeCode     :" + datetimeCode);
            LOG.debug("maxLen           :" + maxLen);
            LOG.debug("precision        :" + precision);
            LOG.debug("scale            :" + scale);
            LOG.debug("nullInfo         :" + nullInfo);
            LOG.debug("signed           :" + signed);
            LOG.debug("odbcDataType     :" + odbcDataType);
            LOG.debug("odbcPrecision    :" + odbcPrecision);
            LOG.debug("sqlCharset       :" + sqlCharset);
            LOG.debug("odbcCharset      :" + odbcCharset);
            LOG.debug("colHeadingNm     :" + colHeadingNm);
            LOG.debug("tableName        :" + tableName);
            LOG.debug("catalogName      :" + catalogName);
            LOG.debug("schemaName       :" + schemaName);
            LOG.debug("headingName      :" + headingName);
            LOG.debug("intLeadPrec      :" + intLeadPrec);
            LOG.debug("paramMode        :" + paramMode);
            LOG.debug("memAlignOffset   :" + memAlignOffset);
            LOG.debug("allocSize        :" + allocSize);
            LOG.debug("varLayout        :" + varLayout);
            LOG.debug("T4 descriptor End -----------");
        }
    }
}
