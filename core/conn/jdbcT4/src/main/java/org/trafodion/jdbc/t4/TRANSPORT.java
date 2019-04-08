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

import java.math.BigDecimal;

class TRANSPORT {
	static final int size_long = 8;
	static final int size_int = 4;
	static final int size_short = 2;
	static final int sql_identifier = 129;

	static final int size_bytes(byte[] buf, boolean fixForServer) {
		return (buf != null && buf.length > 0) ? size_int + buf.length + 1 : size_int + 1;
	}

	static final int size_bytes(byte[] buf) {
		return (buf != null && buf.length > 0) ? size_int + buf.length + 1 : size_int;
	}

	static final int size_bytesWithCharset(byte[] buf) {
		return (buf != null && buf.length > 0) ? size_int + buf.length + 1 + size_int : size_int;
	}

	// password types
	static final int UNAUTHENTICATED_USER_TYPE = 2; // regular password
	static final int PASSWORD_ENCRYPTED_USER_TYPE = 3; // used for SAS

	//
	// IO_BUFFER_LENGTH is used to determin the
	// size of physical buffers as they are
	// created and used by the transport layer
	// for doing internal buffering as well
	// as I/O.
	//
	// The static value here corresponds to the
	// expected buffer size by the ODBC server.
	// It's a fixed value here, but could
	// easly be changed to a System properties
	// value, or gotten programmatically from
	// the ODBC server.
	//

	static final int IO_BUFFER_LENGTH = 4096;
	static final int IO_BUFFER_BLOCKS = 8;

	// ============== ERRORS ====================

	static final int COMM_LINK_FAIL_EXCEPTION = 98;
	static final int TIMEOUT_EXCEPTION = 99;
	static final int WRONG_SIGNATURE = 100;
	static final int MEMORY_ALLOC_FAILED = 101;
	static final int DRVR_ERR_INCORRECT_LENGTH = 102;
	static final int DRVR_ERR_ERROR_FROM_SERVER = 103;
	static final int UNKNOWN_HEADER_TYPE = 104;

	// ================ APIs ====================

	static final short UNKNOWN_API = 0;
	static final short AS_API_START = 1000;
	static final short CFG_API_START = 2000;
	static final short SRVR_API_START = 3000;
	static final short ALL_API_MXCS_END = 9999; // end of all APIs that MXCS
	// understands.

	static final short AS_API_INIT = AS_API_START;
	static final short AS_API_GETOBJREF_OLD = AS_API_INIT + 1; // OK
	// NSKDRVR/CFGDRVR
	static final short AS_API_REGPROCESS = AS_API_GETOBJREF_OLD + 1; // OK
	// NSKSRVR/CFGSRVR
	static final short AS_API_UPDATESRVRSTATE = AS_API_REGPROCESS + 1; // OK
	// NSKSRVR
	static final short AS_API_WOULDLIKETOLIVE = AS_API_UPDATESRVRSTATE + 1; // OK
	// NSKSRVR
	static final short AS_API_STARTAS = AS_API_WOULDLIKETOLIVE + 1; // OK
	// CFGDRVR
	static final short AS_API_STOPAS = AS_API_STARTAS + 1; // OK CFGDRVR
	static final short AS_API_STARTDS = AS_API_STOPAS + 1; // OK CFGDRVR
	static final short AS_API_STOPDS = AS_API_STARTDS + 1; // OK CFGDRVR
	static final short AS_API_STATUSAS = AS_API_STOPDS + 1; // OK CFGDRVR
	static final short AS_API_STATUSDS = AS_API_STATUSAS + 1; // OK CFGDRVR
	static final short AS_API_STATUSDSDETAIL = AS_API_STATUSDS + 1; // OK
	// CFGDRVR
	static final short AS_API_STATUSSRVRALL = AS_API_STATUSDSDETAIL + 1; // OK
	// CFGDRVR
	static final short AS_API_STOPSRVR = AS_API_STATUSSRVRALL + 1; // OK
	// CFGDRVR
	static final short AS_API_STATUSDSALL = AS_API_STOPSRVR + 1; // OK
	// CFGDRVR
	static final short AS_API_DATASOURCECONFIGCHANGED = AS_API_STATUSDSALL + 1; // OK
	// CFGSRVR
	static final short AS_API_ENABLETRACE = AS_API_DATASOURCECONFIGCHANGED + 1; // OK
	// CFGDRVR
	static final short AS_API_DISABLETRACE = AS_API_ENABLETRACE + 1; // OK
	// CFGDRVR
	static final short AS_API_GETVERSIONAS = AS_API_DISABLETRACE + 1; // OK
	// CFGDRVR
	static final short AS_API_GETOBJREF = AS_API_GETVERSIONAS + 1; // OK
	// NSKDRVR/CFGDRVR

	static final short SRVR_API_INIT = SRVR_API_START;
	static final short SRVR_API_SQLCONNECT = SRVR_API_INIT + 1; // OK NSKDRVR
	static final short SRVR_API_SQLDISCONNECT = SRVR_API_SQLCONNECT + 1; // OK
	// NSKDRVR
	static final short SRVR_API_SQLSETCONNECTATTR = SRVR_API_SQLDISCONNECT + 1; // OK
	// NSKDRVR
	static final short SRVR_API_SQLENDTRAN = SRVR_API_SQLSETCONNECTATTR + 1; // OK
	// NSKDRVR
	static final short SRVR_API_SQLPREPARE = SRVR_API_SQLENDTRAN + 1; // OK
	// NSKDRVR
	static final short SRVR_API_SQLPREPARE_ROWSET = SRVR_API_SQLPREPARE + 1; // OK
	// NSKDRVR
	static final short SRVR_API_SQLEXECUTE_ROWSET = SRVR_API_SQLPREPARE_ROWSET + 1; // OK
	// NSKDRVR
	static final short SRVR_API_SQLEXECDIRECT_ROWSET = SRVR_API_SQLEXECUTE_ROWSET + 1; // OK
	// NSKDRVR
	static final short SRVR_API_SQLFETCH = SRVR_API_SQLEXECDIRECT_ROWSET + 1;
	static final short SRVR_API_SQLFETCH_ROWSET = SRVR_API_SQLFETCH + 1; // OK
	// NSKDRVR
	static final short SRVR_API_SQLEXECUTE = SRVR_API_SQLFETCH_ROWSET + 1; // OK
	// NSKDRVR
	static final short SRVR_API_SQLEXECDIRECT = SRVR_API_SQLEXECUTE + 1; // OK
	// NSKDRVR
	static final short SRVR_API_SQLEXECUTECALL = SRVR_API_SQLEXECDIRECT + 1; // OK
	// NSKDRVR
	static final short SRVR_API_SQLFETCH_PERF = SRVR_API_SQLEXECUTECALL + 1; // OK
	// NSKDRVR
	static final short SRVR_API_SQLFREESTMT = SRVR_API_SQLFETCH_PERF + 1; // OK
	// NSKDRVR
	static final short SRVR_API_GETCATALOGS = SRVR_API_SQLFREESTMT + 1; // OK
	// NSKDRVR
	static final short SRVR_API_STOPSRVR = SRVR_API_GETCATALOGS + 1; // OK AS
	static final short SRVR_API_ENABLETRACE = SRVR_API_STOPSRVR + 1; // OK AS
	static final short SRVR_API_DISABLETRACE = SRVR_API_ENABLETRACE + 1; // OK
	// AS
	static final short SRVR_API_ENABLE_SERVER_STATISTICS = SRVR_API_DISABLETRACE + 1; // OK
	// AS
	static final short SRVR_API_DISABLE_SERVER_STATISTICS = SRVR_API_ENABLE_SERVER_STATISTICS + 1; // OK
	// AS
	static final short SRVR_API_UPDATE_SERVER_CONTEXT = SRVR_API_DISABLE_SERVER_STATISTICS + 1; // OK
	// AS
	static final short SRVR_API_MONITORCALL = SRVR_API_UPDATE_SERVER_CONTEXT + 1; // OK
	// PCDRIVER
	static final short SRVR_API_SQLPREPARE2 = SRVR_API_MONITORCALL + 1; // OK
	// PCDRIVER
	static final short SRVR_API_SQLEXECUTE2 = SRVR_API_SQLPREPARE2 + 1; // OK
	// PCDRIVER
	static final short SRVR_API_SQLFETCH2 = SRVR_API_SQLEXECUTE2 + 1; // OK
	// PCDRIVER

	// extent API used to extract lob data
	static final short SRVR_API_EXTRACTLOB = 3030;
	static final short SRVR_API_UPDATELOB = 3031;

	static final short SQL_ATTR_ROWSET_RECOVERY = 2000;
    static final short SQL_ATTR_CLIPVARCHAR = 2001;

	static final int MAX_REQUEST = 300;
	static final int MAX_BUFFER_LENGTH = 32000;
	static final int MAX_PROCESS_NAME = 50;
	static final int MAX_OBJECT_REF = 129;
	static final int SIGNATURE = 12345; // 0x3039
	static final int VERSION = 100;

	static final byte FILE_SYSTEM = 70; // 'F'
	static final byte TCPIP = 84; // 'T'
	static final byte UNKNOWN_TRANSPORT = 78; // 'N'

	static final byte NSK = 78; // 'N'
	static final byte PC = 80; // 'P'

	static final byte SWAP_YES = 89; // 'Y'
	static final byte SWAP_NO = 78; // 'N'

	static final int WRITE_REQUEST_FIRST = 1;
	static final int WRITE_REQUEST_NEXT = WRITE_REQUEST_FIRST + 1;
	static final int READ_RESPONSE_FIRST = WRITE_REQUEST_NEXT + 1;
	static final int READ_RESPONSE_NEXT = READ_RESPONSE_FIRST + 1;
	static final int CLEANUP = READ_RESPONSE_NEXT + 1;
	static final int SRVR_TRANSPORT_ERROR = CLEANUP + 1;
	static final int CLOSE_TCPIP_SESSION = SRVR_TRANSPORT_ERROR + 1;

	// ================ SQL Statement type ====================

	static final short TYPE_UNKNOWN = 0;

	static final short TYPE_SELECT = 0x0001;
	static final short TYPE_UPDATE = 0x0002;
	static final short TYPE_DELETE = 0x0004;
	static final short TYPE_INSERT = 0x0008;
	static final short TYPE_EXPLAIN = 0x0010;
	static final short TYPE_CREATE = 0x0020;
	static final short TYPE_GRANT = 0x0040;
	static final short TYPE_DROP = 0x0080;
	static final short TYPE_INSERT_PARAM = 0x0100;
	static final short TYPE_SELECT_CATALOG = 0x0200;
	static final short TYPE_SMD = 0x0400;
	static final short TYPE_CALL = 0x0800;
	static final short TYPE_STATS = 0x1000;
	static final short TYPE_CONFIG = 0x2000;
	// qs_interface support
	static final short TYPE_QS = 0x4000;
	static final short TYPE_QS_OPEN = 0x4001;
	static final short TYPE_QS_CLOSE = 0x4002;
	static final short TYPE_CMD = 0x03000;
	static final short TYPE_CMD_OPEN = 0x03001;
	static final short TYPE_CMD_CLOSE = 0x03002;
	static final short TYPE_BEGIN_TRANSACTION = 0x05001;
	static final short TYPE_END_TRANSACTION = 0x05002;

	// ================ SQL Query type ====================
	//
	// These values are taken from "Performace Updates External Specification,
	// Database Software"
	// document Version 0.4 Created on May 10, 2005.
	//
	static final int SQL_QUERY_TYPE_NOT_SET  = 99;
	static final int SQL_OTHER = -1;
	static final int SQL_UNKNOWN = 0;
	static final int SQL_SELECT_UNIQUE = 1;
	static final int SQL_SELECT_NON_UNIQUE = 2;
	static final int SQL_INSERT_UNIQUE = 3;
	static final int SQL_INSERT_NON_UNIQUE = 4;
	static final int SQL_UPDATE_UNIQUE = 5;
	static final int SQL_UPDATE_NON_UNIQUE = 6;
	static final int SQL_DELETE_UNIQUE = 7;
	static final int SQL_DELETE_NON_UNIQUE = 8;
	static final int SQL_CONTROL = 9;
	static final int SQL_SET_TRANSACTION = 10;
	static final int SQL_SET_CATALOG = 11;
	static final int SQL_SET_SCHEMA = 12;
	static final int SQL_CALL_NO_RESULT_SETS = 13;
  	static final int SQL_CALL_WITH_RESULT_SETS = 14;
  	static final int SQL_SP_RESULT_SET = 15;
  	static final int SQL_INSERT_RWRS = 16;


	// ================ Execute2 return values ====================

	static final int NO_DATA_FOUND = 100;

	// =========================== NCS versions ==========

	static final BigDecimal NCS_VERSION_3_3 = new BigDecimal("3.3");
	static final BigDecimal NCS_VERSION_3_4 = new BigDecimal("3.4");

	// From CEE class
	static final int CEE_SUCCESS = 0;
	// Added by SB 7/5/2005 for handling SUCCESS_WITH_INFO for Prepare2
	static final int SQL_SUCCESS = 0; // ODBC Standard
	static final int SQL_SUCCESS_WITH_INFO = 1; // ODBC Standard

	// From Global.h
	static final int ESTIMATEDCOSTRGERRWARN = 2;

} // end class TRANSPORT
