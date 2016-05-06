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
import java.sql.SQLException;

class CloseReply {
    odbc_SQLSvc_Close_exc_ m_p1;
    int m_p2;
    ERROR_DESC_LIST_def m_p3;

    // -------------------------------------------------------------
    CloseReply(LogicalByteArray buf, String addr, InterfaceConnection ic) throws CharacterCodingException,
        UnsupportedCharsetException, SQLException {
            buf.setLocation(0);

            m_p1 = new odbc_SQLSvc_Close_exc_();
            m_p1.extractFromByteArray(buf, addr, ic);

            if (m_p1.exception_nr != TRANSPORT.CEE_SUCCESS) {
                m_p2 = buf.extractInt();

                m_p3 = new ERROR_DESC_LIST_def();
                m_p3.extractFromByteArray(buf, ic);
            }
    }
}
