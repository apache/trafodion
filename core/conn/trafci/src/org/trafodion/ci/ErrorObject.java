// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

package org.trafodion.ci;

public class ErrorObject {
    private String errorCode = null;  
    private String errorMessage = null;
    public char errorType = 'E';
    
    public ErrorObject(String errorCode, String errorMessage) {
        this.errorCode = errorCode;
        this.errorMessage = errorMessage;
        if (this.errorMessage.startsWith("*** WARNING["))
           this.errorType = 'W'; 
        else
           this.errorType = 'E'; //Default
    }
    
    public ErrorObject(String errorCode, String errorMessage, char errorType) {
        this.errorCode = errorCode;
        this.errorMessage = errorMessage;
        this.errorType = errorType;
    }
    
   // @Deprecated
    //errStr may have no error code in itself
    public ErrorObject(String errStr) {
    	Parser parser = new Parser();
    	this.errorCode = parser.getErrorCode(errStr);
        this.errorMessage = parser.getErrorMsg(errStr);
        /**
         * Bug 2549: Expected warning message was converted into error 
         * message by JDBC driver.
         * Internal Analysis: some JDBC driver msg doesn't start with 
         * *** [WARNING] will cause this error.
         * Fix Description: instead of using startsWith, use indexOf method
         */
        if (errStr.indexOf("*** WARNING[" + errorCode + "]") > -1)
           this.errorType = 'W'; 
        else
           this.errorType = 'E'; //Default
    }

    public ErrorObject(String errStr,int errorCode) {
    	Parser parser = new Parser();
		this.errorCode = parser.getErrorCode(errStr);
        this.errorMessage = parser.getErrorMsg(errStr);
		if (Parser.UNKNOWN_ERROR_CODE == this.errorCode)
			this.errorCode = String.valueOf(Math.abs(errorCode));
        /**
         * Bug 2549: Expected warning message was converted into error 
         * message by JDBC driver.
         * Internal Analysis: some JDBC driver msg doesn't start with 
         * *** [WARNING] will cause this error.
         * Fix Description: instead of using startsWith, use indexOf method
         */
        if (errStr.indexOf("*** WARNING[" + errorCode + "]") > -1)
           this.errorType = 'W'; 
        else
           this.errorType = 'E'; //Default
    }
    
    public ErrorObject(ErrorObject errObj, String errStrPrefix, String errStrSuffix) {
    	this.errorCode = errObj.errorCode;
        this.errorMessage = errStrPrefix + errObj.errorMessage + errStrSuffix;
        this.errorType = errObj.errorType;
    }
    
    public String errorCode()   { return errorCode; }
    
    public String errorMessage() { return errorMessage; }

    public String RAWOutputError() {
    	String rawOutput = (this.errorType != 'E' || this.errorCode == Parser.UNKNOWN_ERROR_CODE) ? this.errorMessage : "*** ERROR[" + this.errorCode + "] " + this.errorMessage;
      if (this.errorType == 'W') {
          rawOutput =   "*** WARNING[" + this.errorCode + "] "+ this.errorMessage;        
      }
    	return (rawOutput);
     }

    public String toString() {
         return this.RAWOutputError();
      }
}
