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

import java.nio.ByteBuffer;

class SQL_DataValue_def {
    int length;
    byte[] buffer;
    ByteBuffer userBuffer;

    // ----------------------------------------------------------
    int sizeof() {
        return (buffer != null) ? TRANSPORT.size_int + buffer.length + 1 : TRANSPORT.size_int;
    }

    // ----------------------------------------------------------
    void insertIntoByteArray(LogicalByteArray buf) {
        if (buffer != null) {
            buf.insertInt(length);
            buf.insertByteArray(buffer, length);
        } else {
            buf.insertInt(0);
        }
    }

    // ----------------------------------------------------------
    void extractFromByteArray(LogicalByteArray buf) {
        length = buf.extractInt();

        if (length > 0) {
            buffer = buf.extractByteArray(length);
        }
    }
}
