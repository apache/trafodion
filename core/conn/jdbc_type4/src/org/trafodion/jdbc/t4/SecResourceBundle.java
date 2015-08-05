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

import java.util.ResourceBundle;
import java.text.MessageFormat;

public class SecResourceBundle
{

    private static ResourceBundle rb = ResourceBundle.getBundle("secClient");

    /**
     * This method is used to obtain parameterized message text
     *
     */
    static String obtainMessageText (String key, Object[] params) {
        String pattern;
        try {
                pattern = rb.getString(key);
        } catch (Exception e) {
                return key;
        }
        if(pattern == null) {
                return key;
        }
        String message;
        try {
                message = MessageFormat.format(pattern, params);
        } catch (Exception e) {
                return pattern;
        }
        return message;
    }
}
