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

class CONNECTION_CONTEXT_def {
	String datasource = "";
	String catalog = "";
    private String schema = "";
	String location = "";
	String userRole = "";

	short accessMode;
	short autoCommit;
	short queryTimeoutSec;
	short idleTimeoutSec;
	short clipVarchar;
	short loginTimeoutSec;
	short txnIsolationLevel;
	short rowSetSize;

	int diagnosticFlag;
	int processId;

	String computerName = "";
	String windowText = "";

	int ctxACP;
	int ctxDataLang;
	int ctxErrorLang;
	short ctxCtrlInferNXHAR;

	short cpuToUse = -1;
	short cpuToUseEnd = -1; // for future use by DBTransporter

	String connectOptions = "";

	VERSION_LIST_def clientVersionList = new VERSION_LIST_def();

	byte[] datasourceBytes;
	byte[] catalogBytes;
	byte[] schemaBytes;
	byte[] locationBytes;
	byte[] userRoleBytes;
	byte[] computerNameBytes;
	byte[] windowTextBytes;
	byte[] connectOptionsBytes;

	// ----------------------------------------------------------
	int sizeOf(InterfaceConnection ic) throws CharacterCodingException, UnsupportedCharsetException {
		int size = 0;

		datasourceBytes = ic.encodeString(datasource, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		catalogBytes = ic.encodeString(catalog, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		schemaBytes = ic.encodeString(schema, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		locationBytes = ic.encodeString(location, 1);
		userRoleBytes = ic.encodeString(userRole, 1);
		computerNameBytes = ic.encodeString(computerName, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		windowTextBytes = ic.encodeString(windowText, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		connectOptionsBytes = ic.encodeString(connectOptions, 1);

		size = TRANSPORT.size_bytes(datasourceBytes);
		size += TRANSPORT.size_bytes(catalogBytes);
		size += TRANSPORT.size_bytes(schemaBytes);
		size += TRANSPORT.size_bytes(locationBytes);
		size += TRANSPORT.size_bytes(userRoleBytes);

		size += TRANSPORT.size_short; // accessMode
		size += TRANSPORT.size_short; // autoCommit
		size += TRANSPORT.size_int; // queryTimeoutSec
		size += TRANSPORT.size_int; // idleTimeoutSec
		size += TRANSPORT.size_int; // loginTimeoutSec
		size += TRANSPORT.size_short; // txnIsolationLevel
		size += TRANSPORT.size_short; // rowSetSize

		size += TRANSPORT.size_short; // diagnosticFlag
		size += TRANSPORT.size_int; // processId

		size += TRANSPORT.size_bytes(computerNameBytes);
		size += TRANSPORT.size_bytes(windowTextBytes);

		size += TRANSPORT.size_int; // ctxACP
		size += TRANSPORT.size_int; // ctxDataLang
		size += TRANSPORT.size_int; // ctxErrorLang
		size += TRANSPORT.size_short; // ctxCtrlInferNCHAR

		size += TRANSPORT.size_short; // cpuToUse
		size += TRANSPORT.size_short; // cpuToUseEnd
		size += TRANSPORT.size_bytes(connectOptionsBytes);

		size += clientVersionList.sizeOf();

		return size;
	}

	// ----------------------------------------------------------
	void insertIntoByteArray(LogicalByteArray buf) {
		buf.insertString(datasourceBytes);
		buf.insertString(catalogBytes);
		buf.insertString(schemaBytes);
		buf.insertString(locationBytes);
		buf.insertString(userRoleBytes);

		buf.insertShort(accessMode);
		buf.insertShort(autoCommit);
		buf.insertInt(queryTimeoutSec);
		buf.insertInt(idleTimeoutSec);
		buf.insertInt(loginTimeoutSec);
		buf.insertShort(txnIsolationLevel);
		buf.insertShort(rowSetSize);

		buf.insertInt(diagnosticFlag);
		buf.insertInt(processId);

		buf.insertString(computerNameBytes);
		buf.insertString(windowTextBytes);

		buf.insertInt(ctxACP);
		buf.insertInt(ctxDataLang);
		buf.insertInt(ctxErrorLang);
		buf.insertShort(ctxCtrlInferNXHAR);

		buf.insertShort(cpuToUse);
		buf.insertShort(cpuToUseEnd);
		buf.insertString(connectOptionsBytes);

		clientVersionList.insertIntoByteArray(buf);
	}

    public String getSchema() {
        return schema;
    }

    public void setSchema(String schema) {
        this.schema = schema;
    }
}
