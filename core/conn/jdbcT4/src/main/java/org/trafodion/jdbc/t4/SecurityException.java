/**********************************************************************
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
//
 **********************************************************************/
package org.trafodion.jdbc.t4;

import java.lang.String;
import java.lang.Integer;
import java.sql.SQLException;

public class SecurityException extends SQLException
{
   private static final String SQLState = "38001";

   public SecurityException(Exception cex, String key, Object[] params)
   {
	  // Get the text message from the file secClient.properties.
	  // Parse the message for the error message and error number.
      this(cex, (SecResourceBundle.obtainMessageText(key, params)).substring(6),
    		  Integer.parseInt((SecResourceBundle.obtainMessageText(key, params)).substring(0, 5)));
   }

   public SecurityException(Exception cex, String errMsg, int errNum)
   {
	   super(errMsg, SQLState, errNum, cex);
   }

    public SecurityException(String key, Object[] params) {
        this(null, key, params);
    }
}
