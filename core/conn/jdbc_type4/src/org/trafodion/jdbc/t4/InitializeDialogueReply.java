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

package org.trafodion.jdbc.t4;

import java.nio.charset.CharacterCodingException;
import java.nio.charset.UnsupportedCharsetException;
import java.sql.SQLException;

class InitializeDialogueReply {
	static final int odbc_SQLSvc_InitializeDialogue_ParamError_exn_ = 1;
	static final int odbc_SQLSvc_InitializeDialogue_InvalidConnection_exn_ = 2;
	static final int odbc_SQLSvc_InitializeDialogue_SQLError_exn_ = 3;
	static final int odbc_SQLSvc_InitializeDialogue_SQLInvalidHandle_exn_ = 4;
	static final int odbc_SQLSvc_InitializeDialogue_SQLNeedData_exn_ = 5;
	static final int odbc_SQLSvc_InitializeDialogue_InvalidUser_exn_ = 6;

	static final int SQL_PASSWORD_EXPIRING = 8857;
	static final int SQL_PASSWORD_GRACEPERIOD = 8837;
	
	Header m_hdr;
	
	int exception_nr;
	int exception_detail;
	String ParamError;
	ERROR_DESC_LIST_def SQLError;
	ERROR_DESC_LIST_def InvalidUser;
	String clientErrorText;
	
	OUT_CONNECTION_CONTEXT_def outContext;

	// -------------------------------------------------------------
	InitializeDialogueReply(LogicalByteArray buf, String addr, InterfaceConnection ic, boolean downloadCert) throws CharacterCodingException,
			UnsupportedCharsetException, SQLException {
		buf.setLocation(Header.sizeOf());

		exception_nr = buf.extractInt();
		exception_detail = buf.extractInt();
		
		switch (exception_nr) {
		case TRANSPORT.CEE_SUCCESS:
			outContext = new OUT_CONNECTION_CONTEXT_def();
			outContext.extractFromByteArray(buf, ic);
			break;
		case odbc_SQLSvc_InitializeDialogue_SQLError_exn_:
			SQLError = new ERROR_DESC_LIST_def();
			SQLError.extractFromByteArray(buf, ic);
			
			if (exception_detail == SQL_PASSWORD_EXPIRING || exception_detail == SQL_PASSWORD_GRACEPERIOD) {
				outContext = new OUT_CONNECTION_CONTEXT_def();
				outContext.extractFromByteArray(buf, ic);
			}
			break;
		case odbc_SQLSvc_InitializeDialogue_InvalidUser_exn_:
			SQLError = new ERROR_DESC_LIST_def();
			SQLError.extractFromByteArray(buf, ic);
			
			ic.outContext = new OUT_CONNECTION_CONTEXT_def();
			ic.outContext.extractFromByteArray(buf, ic);
			break;
			//throw HPT4Messages.createSQLException(null, ic.getLocale(), "ids_28_000", null);
		case odbc_SQLSvc_InitializeDialogue_ParamError_exn_:
			ParamError = ic.decodeBytes(buf.extractString(), 1);
			throw HPT4Messages.createSQLException(null, ic.getLocale(), "ids_program_error", ParamError, addr);
		case odbc_SQLSvc_InitializeDialogue_InvalidConnection_exn_:
			throw HPT4Messages.createSQLException(null, ic.getLocale(), "ids_08_s01", null);
		default:
			clientErrorText = "unknown_initialize_dialogue_reply_error";
			break;
		}
	}
}
