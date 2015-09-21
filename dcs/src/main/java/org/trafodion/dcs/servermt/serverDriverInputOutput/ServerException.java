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
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.net.*;
import java.util.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverHandler.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class ServerException {
    private static  final Log LOG = LogFactory.getLog(ServerException.class);

    private int exception_nr=0;
    private int exception_detail=0;
    private ErrorDescList errorDescList = null;
    private String errorText="";
    
    public ServerException (){
        this.errorText = "";
        this.exception_nr = 0;
        this.exception_detail = 0;
        this.errorDescList = null;
    }
//==================================================================================
    public void setServerException (int exception_nr, int exception_detail, String errorText ){
        this.exception_nr = exception_nr;
        this.exception_detail = exception_detail;
        this.errorText = errorText;
    }
    public void setServerException (int exception_nr, int exception_detail, ErrorDescList errorDescList){
         this.exception_nr = exception_nr;
        this.exception_detail = exception_detail;
        this.errorDescList = new ErrorDescList(errorDescList);
    }
    public void setServerException (int exception_nr, int exception_detail, SQLException ex){
        this.exception_nr = exception_nr;
        this.exception_detail = exception_detail;
        this.errorDescList = new ErrorDescList(ex);
    }
//=====================================================================================
    public void insertIntoByteBuffer(ByteBuffer buf) throws UnsupportedEncodingException{
        buf.putInt(exception_nr);
        buf.putInt(exception_detail);
        if (errorDescList != null)
            errorDescList.insertIntoByteBuffer(buf);
        else if (errorText.length() > 0)
            ByteBufferUtils.insertString(errorText,buf);
    }
    public int lengthOfData() {
        int dataLength = 0;
        dataLength += ServerConstants.INT_FIELD_SIZE;                                         //exception_nr
        dataLength += ServerConstants.INT_FIELD_SIZE;                                         //exception_detail
        if (errorDescList != null)
            dataLength += errorDescList.lengthOfData();
        else if (errorText.length() > 0)
            dataLength += ByteBufferUtils.lengthOfString(errorText);                //errorText
         return dataLength;
    }
    public void setErrorText(String errorText){
        this.errorText = errorText;
    }
    public void setException_nr(int exception_nr){
        this.exception_nr = exception_nr;
    }
    public void setException_detail(int exception_detail){
        this.exception_detail = exception_detail;
    }
    public String getErrorText(){
        return errorText;
    }
    public int getException_nr(){
        return exception_nr;
    }
    public int getException_detail(){
        return exception_detail;
    }
}
