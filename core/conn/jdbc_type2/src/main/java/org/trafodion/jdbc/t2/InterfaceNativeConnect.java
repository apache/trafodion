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
import java.util.Locale;
import java.util.logging.Level;

class InterfaceNativeConnect {
    protected Locale m_locale;
    protected long m_dialogueId;
    InterfaceConnection m_ic;

    InterfaceNativeConnect(InterfaceConnection ic) throws SQLException {
        if (ic == null) {
            throwInternalException();
        }
        m_ic = ic;
        m_locale = ic.getLocale();
        m_dialogueId = ic.getDialogueId();
    }

    public void finalizer() {
    }

    protected long getDialogueId() {
        return m_dialogueId;
    }

    protected Locale getLocale() {
        return m_locale;
    }

    protected void throwInternalException() throws SQLException {
        T2Properties tempP = null;

        if (m_ic != null) {
            tempP = m_ic.t2props_;
        }

        SQLException se = Messages.createSQLException(tempP, m_locale, "internal_error", null);
        SQLException se2 = Messages.createSQLException(tempP, m_locale, "contact_hp_error", null);

        se.setNextException(se2);
        throw se;
    }

    // --------------------------------------------------------------------------------
    protected LogicalByteArray getReadBuffer(short odbcAPI, LogicalByteArray wbuffer) throws SQLException {
        LogicalByteArray rbuffer = null;

        return rbuffer;
    }

}
