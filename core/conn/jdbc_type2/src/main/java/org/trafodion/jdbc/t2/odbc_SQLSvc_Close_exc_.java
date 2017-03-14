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

class odbc_SQLSvc_Close_exc_ {
    int exception_nr;
    int exception_detail;
    String ParamError;
    ERROR_DESC_LIST_def SQLError;

    String clientErrorText = null;

    static final int odbc_SQLSvc_Close_ParamError_exn_ = 1;
    static final int odbc_SQLSvc_Close_InvalidConnection_exn_ = 2;
    static final int odbc_SQLSvc_Close_SQLError_exn_ = 3;
    static final int odbc_SQLSvc_Close_TransactionError_exn_ = 4;

    // -------------------------------------------------------------------
    void extractFromByteArray(LogicalByteArray buf, String addr, InterfaceConnection ic)
        throws CharacterCodingException, UnsupportedCharsetException, SQLException {
        exception_nr = buf.extractInt();
        exception_detail = buf.extractInt();

        String temp0 = Integer.toString(exception_nr);
        String temp1 = Integer.toString(exception_detail);

        switch (exception_nr) {
            case TRANSPORT.CEE_SUCCESS:
                break;
            case odbc_SQLSvc_Close_SQLError_exn_:
                SQLError = new ERROR_DESC_LIST_def();
                SQLError.extractFromByteArray(buf, ic);
                break;
            case odbc_SQLSvc_Close_ParamError_exn_:
                ParamError = ic.decodeBytes(buf.extractString(), 1);
                ;
                throw Messages.createSQLException(null, ic.getLocale(), "ids_program_error", ParamError, addr);
            case odbc_SQLSvc_Close_InvalidConnection_exn_:
                throw Messages.createSQLException(null, ic.getLocale(), "ids_08_s01", null);
            case odbc_SQLSvc_Close_TransactionError_exn_:
                throw Messages.createSQLException(null, ic.getLocale(), "ids_transaction_error", temp1, addr);
            default:
                throw Messages.createSQLException(null, ic.getLocale(), "ids_unknown_reply_error", temp0, temp1);
        }
    }
}
