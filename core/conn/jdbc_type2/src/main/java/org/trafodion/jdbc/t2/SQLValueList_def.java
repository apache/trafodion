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

import java.sql.SQLException;

class SQLValueList_def {
    SQLValue_def[] buffer;

    // ----------------------------------------------------------
    int sizeof() {
        int size = TRANSPORT.size_int;

        if (buffer != null) {
            for (int i = 0; i < buffer.length; i++) {
                size += buffer[i].sizeof();
            }
        }
        return size;
    }

    // ----------------------------------------------------------
    void insertIntoByteArray(LogicalByteArray buf) {
        if (buffer != null) {
            buf.insertInt(buffer.length);
            for (int i = 0; i < buffer.length; i++) {
                buffer[i].insertIntoByteArray(buf);
            }
        } else {
            buf.insertInt(0);
        }
    }

    // ----------------------------------------------------------
    void extractFromByteArray(LogicalByteArray buf) throws SQLException {
        int len = buf.extractInt();

        buffer = new SQLValue_def[len];

        for (int i = 0; i < buffer.length; i++) {
            buffer[i] = new SQLValue_def();
            buffer[i].extractFromByteArray(buf);
        }
    }
}
