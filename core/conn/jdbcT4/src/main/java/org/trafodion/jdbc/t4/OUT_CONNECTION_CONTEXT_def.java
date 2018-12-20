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

class OUT_CONNECTION_CONTEXT_def {
	static final long OUTCONTEXT_OPT1_ENFORCE_ISO88591 = 1; // (2^0)
	static final long OUTCONTEXT_OPT1_IGNORE_SQLCANCEL = 1073741824; // (2^30)
	static final long OUTCONTEXT_OPT1_EXTRA_OPTIONS = 2147483648L; // (2^31)
	static final long OUTCONTEXT_OPT1_DOWNLOAD_CERTIFICATE = 536870912; //(2^29)

	VERSION_LIST_def versionList;

	short nodeId;
	int processId;

	String computerName;
	String catalog;
    private String schema;

	int optionFlags1;
	int optionFlags2;

	String _roleName;
	boolean _enforceISO;
	boolean _ignoreCancel;

	byte [] certificate;

	void extractFromByteArray(LogicalByteArray buf, InterfaceConnection ic) throws SQLException,
			UnsupportedCharsetException, CharacterCodingException {
		versionList = new VERSION_LIST_def();
		versionList.extractFromByteArray(buf);

		nodeId = buf.extractShort();
		processId = buf.extractInt();
		computerName = ic.decodeBytes(buf.extractString(), 1);

		catalog = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);
		schema = ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8);

		optionFlags1 = buf.extractInt();
		optionFlags2 = buf.extractInt();

		this._enforceISO = (optionFlags1 & OUTCONTEXT_OPT1_ENFORCE_ISO88591) > 0;
		this._ignoreCancel = (optionFlags1 & OUTCONTEXT_OPT1_IGNORE_SQLCANCEL) > 0;
		if((optionFlags1 & OUTCONTEXT_OPT1_DOWNLOAD_CERTIFICATE) > 0) {
			certificate = buf.extractByteArray();
		}
		else if ((optionFlags1 & OUTCONTEXT_OPT1_EXTRA_OPTIONS) > 0) {
			try {
				this.decodeExtraOptions(ic.decodeBytes(buf.extractString(), InterfaceUtilities.SQLCHARSETCODE_UTF8));
			} catch (Exception e) {
				ic.t4props_.logger.warning("An error occurred parsing OutConnectionContext: " + e.getMessage());
			}
		}
	}

	public void decodeExtraOptions(String options) {
		String[] opts = options.split(";");
		String token;
		String value;
		int index;

		for (int i = 0; i < opts.length; i++) {
			index = opts[i].indexOf('=');
			token = opts[i].substring(0, index).toUpperCase();
			value = opts[i].substring(index + 1);

			if (token.equals("RN")) {
				this._roleName = value;
			}
		}
	}

    public String getSchema() {
        return schema;
    }

    public void setSchema(String schema) {
        this.schema = schema;
    }
}
