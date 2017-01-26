/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

package org.trafodion.dtm;
import java.io.IOException;

/** Thrown when a transaction conflicts with another transaction. 
 * 
 */
public class TransactionManagerException extends IOException {

  private static final long serialVersionUID = 7062921444531109202L;
  private static short errorCode;

  /** Default Constructor */
  public TransactionManagerException() {
    super();
    errorCode = 0;
  }

  /**
   * @param arg0 message
   * @param arg1 cause
   */
  public TransactionManagerException(String arg0, Throwable arg1, short errCode) {
    super(arg0, arg1);
    errorCode = errCode;
  }

  /**
   * @param arg0 message
   */
  public TransactionManagerException(String arg0, short errCode) {
    super(arg0);
    errorCode = errCode;
  }

  /**
   * @param arg0 cause
   */
  public TransactionManagerException(Throwable arg0, short errCode) {
    super(arg0);
    errorCode = errCode;
  }
  
  public short getErrorCode(){
    return errorCode;
  }

}
