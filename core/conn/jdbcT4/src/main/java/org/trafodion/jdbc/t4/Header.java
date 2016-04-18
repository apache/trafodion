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

import java.sql.SQLException;
import java.util.Locale;

/**
 * This class corresponds to the ODBC HEADER structure as taken from
 * TransportBase.h
 * @version 1.0
 */

class Header {

	//
	// Fixed values taken from TransportBase.h
	//
	static final short WRITE_REQUEST_FIRST = 1;
	static final short WRITE_REQUEST_NEXT = (short) (WRITE_REQUEST_FIRST + 1);
	static final short READ_RESPONSE_FIRST = (short) (WRITE_REQUEST_NEXT + 1);
	static final short READ_RESPONSE_NEXT = (short) (READ_RESPONSE_FIRST + 1);
	static final short CLEANUP = (short) (READ_RESPONSE_NEXT + 1);
	static final short SRVR_TRANSPORT_ERROR = (short) (CLEANUP + 1);
	static final short CLOSE_TCPIP_SESSION = (short) (SRVR_TRANSPORT_ERROR + 1);

	static final int SIGNATURE = 12345; // 0x3039
	
	//static final int OLD_VERSION = 100; //pre 2.5 server
	static final int CLIENT_HEADER_VERSION_BE = 101;
	//static final int CLIENT_HEADER_VERSION_LE = 102 //not used in JDBC
	static final int SERVER_HEADER_VERSION_BE = 201;
	static final int SERVER_HEADER_VERSION_LE = 202;

	static final char TCPIP = 'T';

	static final char NSK = 'N';
	static final char PC = 'P';

	static final char YES = 'Y';
	static final char NO = 'N';

	static final char COMP_0 = 0x0;
	static final char COMP_12 = 0x12;
	static final char COMP_14 = 0x14;

	//
	// The Java version of the HEADER structure taken from TransportBase.h
	//
	short operation_id_;
	// + 2 filler
	int dialogueId_;
	int total_length_;
	int cmp_length_;
	char compress_ind_;
	char compress_type_;
	// + 2 filler
	int hdr_type_;
	int signature_;
	int version_;
	char platform_;
	char transport_;
	char swap_;
	// + 1 filler
	short error_;
	short error_detail_;

	Header() {
		// Do nothing constructor
	}

	Header(short operation_id, int dialogueId, int total_length, int cmp_length, char compress_ind, char compress_type,
			int hdr_type, int signature, int version, char platform, char transport, char swap) {
		operation_id_ = operation_id;
		dialogueId_ = dialogueId;
		total_length_ = total_length;
		cmp_length_ = cmp_length;
		compress_ind_ = compress_ind;
		compress_type_ = compress_type;
		hdr_type_ = hdr_type;
		signature_ = signature;
		version_ = version;
		platform_ = platform;
		transport_ = transport;
		swap_ = swap;
	}

	void reuseHeader(short operation_id, int dialogueId) {
		operation_id_ = operation_id;
		dialogueId_ = dialogueId;
	}

	static int sizeOf() {
		return 40;
	}

	void insertIntoByteArray(LogicalByteArray buffer1, Locale locale) throws SQLException {
		
		buffer1.insertShort(operation_id_);
		buffer1.insertShort((short) 0); // + 2 filler
		buffer1.insertInt(dialogueId_);
		buffer1.insertInt(total_length_);
		buffer1.insertInt(cmp_length_);
		buffer1.insertChar(compress_ind_);

		buffer1.insertChar(compress_type_);
		buffer1.insertShort((short) 0); // + 2 filler
		buffer1.insertInt(hdr_type_);
		buffer1.insertInt(signature_);
		buffer1.insertInt(version_);
		buffer1.insertChar(platform_);
		buffer1.insertChar(transport_);
		buffer1.insertChar(swap_);
		buffer1.insertByte((byte) 0); // + 2 filler
		buffer1.insertShort(error_);
		buffer1.insertShort(error_detail_);
	}

	void extractFromByteArray(LogicalByteArray buffer1) {
		operation_id_ = buffer1.extractShort();
		buffer1.extractShort(); // + 2 filler
		dialogueId_ = buffer1.extractInt();
		total_length_ = buffer1.extractInt();
		cmp_length_ = buffer1.extractInt();
		compress_ind_ = buffer1.extractChar();
		compress_type_ = buffer1.extractChar();
		buffer1.extractShort(); // + 2 filler
		hdr_type_ = buffer1.extractInt();
		signature_ = buffer1.extractInt();
		version_ = buffer1.extractInt();
		platform_ = buffer1.extractChar();
		transport_ = buffer1.extractChar();
		swap_ = buffer1.extractChar();
		buffer1.extractByte(); // + 1 filler
		error_ = buffer1.extractShort();
		error_detail_ = buffer1.extractShort();
	}
}
