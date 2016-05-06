/*******************************************************************************
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
 *******************************************************************************/

package org.trafodion.jdbc.t2;

import java.nio.charset.CharacterCodingException;
import java.nio.charset.UnsupportedCharsetException;

class ERROR_DESC_LIST_def {
    int length;
    ERROR_DESC_def[] buffer;

    // ----------------------------------------------------------
    void extractFromByteArray(LogicalByteArray buf, InterfaceConnection ic) throws CharacterCodingException,
         UnsupportedCharsetException {
             length = buf.extractInt();
             buffer = new ERROR_DESC_def[length];

             for (int i = 0; i < length; i++) {
                 buffer[i] = new ERROR_DESC_def();
                 buffer[i].extractFromByteArray(buf, ic);
             }
    }
}
