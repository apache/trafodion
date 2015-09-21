/**
* @@@ START COPYRIGHT @@@

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.

* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs.master.listener;

public final class ListenerConstants {
    // read buffer stat
    static final int BUFFER_INIT = 0;
    static final int HEADER_PROCESSED = 1;
    static final int BODY_PROCESSED = 2;
    // errors returned by DcsMaster
    static final int DcsMasterParamError_exn = 1;
    static final int DcsMasterTimeout_exn = 2;
    static final int DcsMasterNoSrvrHdl_exn = 3;
    static final int DcsMasterTryAgain_exn = 4;
    static final int DcsMasterASNotAvailable_exn = 5;
    static final int DcsMasterDSNotAvailable_exn = 6;
    static final int DcsMasterPortNotAvailable_exn = 7;
    static final int DcsMasterInvalidUser_exn = 8;
    static final int DcsMasterLogonUserFailure_exn = 9;
	//
	static final int REQUST_CLOSE = 1;
	static final int REQUST_WRITE = 2;
	static final int REQUST_WRITE_EXCEPTION = 3;
	//
	// Fixed values taken from TransportBase.h
	//
	static final int CLIENT_HEADER_VERSION_BE = 101;
	static final int CLIENT_HEADER_VERSION_LE = 102;
	static final int SERVER_HEADER_VERSION_BE = 201;
	static final int SERVER_HEADER_VERSION_LE = 202;

	static final char YES = 'Y';
	static final char NO = 'N';

	// header size
	static final int HEADER_SIZE = 40;
	// max body size
	static final int BODY_SIZE = 1024;
	//
	static final int LITTLE_ENDIAN_SIGNATURE = 959447040;   // 0x39300000
	static final int BIG_ENDIAN_SIGNATURE = 12345;          // 0x00003039
	
	static final int DCS_MASTER_COMPONENT = 2;
	static final int ODBC_SRVR_COMPONENT = 4;
	
	static final int DCS_MASTER_VERSION_MAJOR_1 = 3;
	static final int DCS_MASTER_VERSION_MINOR_0 = 0;
	static final int DCS_MASTER_BUILD_1 = 1;
	
	static final int CHARSET = 268435456; //(2^28) For charset changes compatibility
	static final int PASSWORD_SECURITY = 67108864; //(2^26)
	
	//=================MXOSRVR versions ===================
	static final int MXOSRVR_ENDIAN = 256;
	static final int MXOSRVR_VERSION_MAJOR = 3;
	static final int MXOSRVR_VERSION_MINOR = 5;
	static final int MXOSRVR_VERSION_BUILD = 1;

	static final short DCS_MASTER_GETSRVRAVAILABLE = 1000 + 19;
    static final short DCS_MASTER_CANCELQUERY = 1000 + 13;

}
