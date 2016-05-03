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

class GetSQLCatalogsMessage {
	static LogicalByteArray marshal(int dialogueId, String stmtLabel, short APIType, String catalogNm, String schemaNm,
			String tableNm, String tableTypeList, String columnNm, int columnType, int rowIdScope, int nullable,
			int uniqueness, int accuracy, short sqlType, int metadataId, String fkCatalogNm, String fkSchemaNm,
			String fkTableNm, InterfaceConnection ic) throws CharacterCodingException, UnsupportedCharsetException {
		int wlength = Header.sizeOf();
		LogicalByteArray buf;

		byte[] stmtLabelBytes = ic.encodeString(stmtLabel, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		byte[] catalogNmBytes = ic.encodeString(catalogNm, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		byte[] schemaNmBytes = ic.encodeString(schemaNm, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		byte[] tableNmBytes = ic.encodeString(tableNm, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		byte[] tableTypeListBytes = ic.encodeString(tableTypeList, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		byte[] columnNmBytes = ic.encodeString(columnNm, InterfaceUtilities.SQLCHARSETCODE_UTF8);

		byte[] fkCatalogNmBytes = ic.encodeString(fkCatalogNm, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		byte[] fkSchemaNmBytes = ic.encodeString(fkSchemaNm, InterfaceUtilities.SQLCHARSETCODE_UTF8);
		byte[] fkTableNmBytes = ic.encodeString(fkTableNm, InterfaceUtilities.SQLCHARSETCODE_UTF8);

		wlength += TRANSPORT.size_int; // dialogueId
		wlength += TRANSPORT.size_bytes(stmtLabelBytes);
		wlength += TRANSPORT.size_short; // APIType
		wlength += TRANSPORT.size_bytes(catalogNmBytes, true);
		wlength += TRANSPORT.size_bytes(schemaNmBytes, true);
		wlength += TRANSPORT.size_bytes(tableNmBytes, true);
		wlength += TRANSPORT.size_bytes(tableTypeListBytes, true);
		wlength += TRANSPORT.size_bytes(columnNmBytes, true);
		wlength += TRANSPORT.size_int; // columnType
		wlength += TRANSPORT.size_int; // rowIdScope
		wlength += TRANSPORT.size_int; // nullable
		wlength += TRANSPORT.size_int; // uniqueness
		wlength += TRANSPORT.size_int; // accuracy
		wlength += TRANSPORT.size_short; // sqlType
		wlength += TRANSPORT.size_int; // metadataId
		wlength += TRANSPORT.size_bytes(fkCatalogNmBytes, true);
		wlength += TRANSPORT.size_bytes(fkSchemaNmBytes, true);
		wlength += TRANSPORT.size_bytes(fkTableNmBytes, true);

		buf = new LogicalByteArray(wlength, Header.sizeOf(), ic.getByteSwap());

		buf.insertInt(dialogueId);
		buf.insertString(stmtLabelBytes);
		buf.insertShort(APIType);
		buf.insertString(catalogNmBytes, true);
		buf.insertString(schemaNmBytes, true);
		buf.insertString(tableNmBytes, true);
		buf.insertString(tableTypeListBytes, true);
		buf.insertString(columnNmBytes, true);
		buf.insertInt(columnType);
		buf.insertInt(rowIdScope);
		buf.insertInt(nullable);
		buf.insertInt(uniqueness);
		buf.insertInt(accuracy);
		buf.insertShort(sqlType);
		buf.insertInt(metadataId);
		buf.insertString(fkCatalogNmBytes, true);
		buf.insertString(fkSchemaNmBytes, true);
		buf.insertString(fkTableNmBytes, true);

		return buf;
	}
}
