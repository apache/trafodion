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

class CloseMessage {
    // ----------------------------------------------------------
    static LogicalByteArray marshal(long dialogueId, String stmtLabel, short freeResourceOpt, InterfaceConnection ic)
        throws UnsupportedCharsetException, CharacterCodingException {
        int wlength = 0;
        LogicalByteArray buf;

        byte[] stmtLabelBytes = ic.encodeString(stmtLabel, InterfaceUtilities.SQLCHARSETCODE_UTF8);

        wlength += TRANSPORT.size_long; // dialogueId
        wlength += TRANSPORT.size_bytes(stmtLabelBytes);
        wlength += TRANSPORT.size_short; // freeResourceOpt

        buf = new LogicalByteArray(wlength, 0, true);

        buf.insertLong(dialogueId);
        buf.insertString(stmtLabelBytes);
        buf.insertShort(freeResourceOpt);

        return buf;
    }
}
