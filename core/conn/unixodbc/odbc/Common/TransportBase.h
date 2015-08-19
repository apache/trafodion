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
#ifndef TRANSPORT_BASE_H
#define TRANSPORT_BASE_H

#include "odbccommon.h"
#include "odbcsrvrcommon.h"

typedef struct request
{
	short	request_code;
} request_def;

typedef short Phandle_def[18];

typedef struct receive_info
{
	short	ioType;
	short	maxReplyCount;
	short	messageTag;
	short	senderFileNum;
	long	syncID;
	Phandle_def	senderPhandle;
	short	openLabel;
} receive_info_def;

//=========== defines for FILE_GETINFOBYNAME_ =======

#define ODBC_DEVICE_TYPE				70
#define ODBC_DEVICE_SUBTYPE_AS			0
#define ODBC_DEVICE_SUBTYPE_CFG			1
#define ODBC_DEVICE_SUBTYPE_SRVR		2
#define ODBC_DEVICE_SUBTYPE_DS			10

#define ODBC_SSID						231
#define ODBC_ABBR_ZMXO					"ZMXO"
#define ODBC_ABBR_MXO					"MXO"
#define ODBC_PRODUCT_MODULE				"ZMXOSCF"

#define UNKNOWN_API						0
#define AS_API_START					1000
#define CFG_API_START					2000
#define SRVR_API_START					3000

//=========== as server apis ========================

enum AS_API {
	AS_API_INIT = AS_API_START, 
	AS_API_GETOBJREF,							//OK NSKDRVR/CFGDRVR
	AS_API_REGPROCESS,							//OK NSKSRVR/CFGSRVR
	AS_API_UPDATESRVRSTATE,						//OK NSKSRVR
	AS_API_WOULDLIKETOLIVE,						//OK NSKSRVR
	AS_API_STARTAS,								//OK CFGDRVR
	AS_API_STOPAS,								//OK CFGDRVR
	AS_API_STARTDS,								//OK CFGDRVR
	AS_API_STOPDS,								//OK CFGDRVR
	AS_API_STATUSAS,							//OK CFGDRVR
	AS_API_STATUSDS,							//OK CFGDRVR
	AS_API_STATUSDSDETAIL,						//OK CFGDRVR
	AS_API_STATUSSRVRALL,						//OK CFGDRVR
	AS_API_STOPSRVR,							//OK CFGDRVR							
	AS_API_STATUSDSALL,							//OK CFGDRVR
	AS_API_DATASOURCECONFIGCHANGED,				//OK CFGSRVR
	AS_API_ENABLETRACE,							//OK CFGDRVR							
	AS_API_DISABLETRACE,						//OK CFGDRVR
	AS_API_GETVERSIONAS,						//OK CFGDRVR
};

//=========== cfg server apis =======================

enum CFG_API {
	CFG_API_INIT = CFG_API_START,
	CFG_API_GETOBJECTNAMELIST,					//OK CFGDRVR
	CFG_API_GETDATASOURCE,						//OK CFGDRVR
	CFG_API_DROPDATASOURCE,						//OK CFGDRVR
	CFG_API_SETDATASOURCE,						//OK CFGDRVR
	CFG_API_ADDNEWDATASOURCE,					//OK CFGDRVR
	CFG_API_CHECKDATASOURCENAME,				//OK CFGDRVR
	CFG_API_GETDSNCONTROL,						//OK CFGDRVR
	CFG_API_SETDSNCONTROL,						//OK CFGDRVR
	CFG_API_GETRESOURCEVALUES,					//OK CFGDRVR
	CFG_API_SETRESOURCEVALUES,					//OK CFGDRVR
	CFG_API_GETENVIRONMENTVALUES,				//OK CFGDRVR
	CFG_API_SETENVIRONMENTVALUES,				//OK CFGDRVR
	CFG_API_GETSTARTUPCONFIGVALUES,				//OK AS
	CFG_API_GETDATASOURCEVALUES,				//OK AS/CFGDRVR
	CFG_API_SETDSSTATUS,						//OK AS
	CFG_API_SETASSTATUS,						//OK AS
	CFG_API_USERAUTHENTICATE,					//OK 
	CFG_API_CHANGEPASSWORD 						//OK 
};

enum SRVR_API {
	SRVR_API_INIT = SRVR_API_START,
	SRVR_API_SQLCONNECT,						//OK NSKDRVR
	SRVR_API_SQLDISCONNECT,						//OK NSKDRVR
	SRVR_API_SQLSETCONNECTATTR,					//OK NSKDRVR
	SRVR_API_SQLENDTRAN,						//OK NSKDRVR
	SRVR_API_SQLPREPARE,						//OK NSKDRVR
	SRVR_API_SQLPREPARE_ROWSET,					//OK NSKDRVR
	SRVR_API_SQLEXECUTE_ROWSET,					//OK NSKDRVR
	SRVR_API_SQLEXECDIRECT_ROWSET,				//OK NSKDRVR
	SRVR_API_SQLFETCH_ROWSET,					//OK NSKDRVR
	SRVR_API_SQLEXECUTE,						//OK NSKDRVR
	SRVR_API_SQLEXECDIRECT,						//OK NSKDRVR
	SRVR_API_SQLEXECUTECALL,					//OK NSKDRVR
	SRVR_API_SQLFETCH_PERF,						//OK NSKDRVR
	SRVR_API_SQLFREESTMT,						//OK NSKDRVR
	SRVR_API_GETCATALOGS,						//OK NSKDRVR
	SRVR_API_STOPSRVR,							//OK AS
	SRVR_API_ENABLETRACE,						//OK AS
	SRVR_API_DISABLETRACE,						//OK AS
	SRVR_API_ENABLE_SERVER_STATISTICS,			//OK AS
	SRVR_API_DISABLE_SERVER_STATISTICS,			//OK AS
	SRVR_API_UPDATE_SERVER_CONTEXT,				//OK AS
	SRVR_API_MONITORCALL,						//OK PCDRIVER
};
//==================== nt socket errors ============

#define NT_SOCKET_ERR 10000

//==================== nsk socket errors ============

#define NSK_SOCKET_ERR 4000

//==================== server error codes ===========

#define SRVR_ERR_ 5000

enum SRVR_ERR {
	SRVR_ERR_ERR = SRVR_ERR_,
	SRVR_ERR_WRONG_MESSAGE_FORMAT,
	SRVR_ERR_NODE_WRITE_REQUEST_FIRST,
	SRVR_ERR_MEMORY_ALLOCATE,
	SRVR_ERR_NODE_WRITE_REQUEST_NEXT,
	SRVR_ERR_NODE_READ_RESPONSE_FIRST,
	SRVR_ERR_NODE_READ_RESPONSE_NEXT,
	SRVR_ERR_READ_OPERATION,
	SRVR_ERR_COMPRESS_OPERATION,
	SRVR_ERR_EXPAND_OPERATION,
	SRVR_ERR_WRITE_OPERATION,
	SRVR_ERR_UNKNOWN_REQUEST,
	SRVR_ERR_LISTENER_ERROR1,
	SRVR_ERR_LISTENER_ERROR2,
	SRVR_ERR_ZERO_MESSAGE_LENGTH
};

//================== driver error codes ==============

#define DRVR_ERR_ 6000

enum DRVR_ERR {
	DRVR_ERR_ERR = DRVR_ERR_,
	DRVR_ERR_INVALID_DLLHANDLE,
	DRVR_ERR_CANNOTLOAD_PROCADDRESS,
	DRVR_ERR_WRONGWINSOCKVERSION,
	DRVR_ERR_WRONGSIGNATURE,
	DRVR_ERR_WRONGVERSION,
	DRVR_ERR_ERROR_FROM_SERVER,
	DRVR_ERR_INCORRECT_LENGTH,
	DRVR_ERR_MEMORY_ALLOCATE,
	DRVR_ERR_WRONG_IP_ADDRESS
};

//===================================================
#define TCPI_DRVR_SEND_BUFFER		32000
#define TCPI_DRVR_RECV_BUFFER		32000
#define TCPI_SRVR_SEND_BUFFER		32000
#define TCPI_SRVR_RECV_BUFFER		32000

#ifdef COLLAPSED_CFGLIB
#define OPEN_SESSION_TIMEOUT			10	//seconds 
#define FIRST_READ_TIMEOUT			10	//seconds
#else
#define OPEN_SESSION_TIMEOUT			-1	//seconds, -1 for notimeout
#define FIRST_READ_TIMEOUT			-1	//seconds
#endif

#define MAX_REQUEST					300
#define MAX_BUFFER_LENGTH			32000
#define MAX_PROCESS_NAME			50
#define MAX_OBJECT_REF				129
#define SIGNATURE					12345			// 0x3039
#define VERSION						100

#define FILE_SYSTEM					'F'
#define TCPIP						'T'
#define UNKNOWN_TRANSPORT			'N'

#define NSK							'N'
#define PC							'P'

#define SWAP_YES					'Y'
#define SWAP_NO						'N' 

#define COMP_YES					'Y'
#define COMP_NO						'N'

#define COMP_12						0x12
#define COMP_13						0x13
#define COMP_14						0x14

enum HDRTYPE {
	WRITE_REQUEST_FIRST = 1,
	WRITE_REQUEST_NEXT,
	READ_RESPONSE_FIRST,
	READ_RESPONSE_NEXT,
	CLEANUP,
	SRVR_TRANSPORT_ERROR,
	CLOSE_TCPIP_SESSION
};

typedef struct {
	short	operation_id;
	long	dialogueId;
	unsigned long total_length;
	unsigned long cmp_length;
	char compress_ind;
	char compress_type;
	HDRTYPE	hdr_type;
	unsigned long signature;
	unsigned long version;
	char	platform;
	char	transport;
	char	swap;
	short	error;
	short	error_detail;
} 
HEADER;

enum ERROR_TYPE {
	E_UNKNOWN = 0,
	E_DRIVER,
	E_SERVER,
	E_ASSERVER,
	E_CFGSERVER,
	E_TEMP_MEMORY,
	E_TIMER,
	E_LISTENER,
	E_TCPIPROCESS
};

enum OPERATION {
	O_UNDEFINED = 0,
	O_INIT_PROCESS,
	O_OPEN_SESSION,
	O_DO_WRITE_READ,
	O_DO_OPERATOR_NEW,
	O_DO_EXPAND,
	O_NEW_CONNECTION
};

enum FUNCTION {
	F_UNDEFINED = 0,
	F_AWAITIOX,
	F_CHECK_IF_ASSVC_LIVES,
	F_ENV_GET_SYSTEM_CATALOG_NAME,
	F_ENV_GET_MX_SYSTEM_CATALOG_NAME,
	F_FILE_GETINFO_,
	F_FILE_OPEN_,
	F_FILENAME_TO_PROCESSHANDLE_,
	F_HDR_TYPE,
	F_INS_NODE,
	F_INSTANTIATE_RG_OBJECT,
	F_NEW,
	F_PROCESS_GETINFO_,
	F_PROCESSHANDLE_DECOMPOSE_,
	F_PROCESSHANDLE_GETMINE_,
	F_SETMODE,
	F_FILE_COMPLETE_SET,
	F_FILE_COMPLETE,
	F_FILE_COMPLETE_GETINFO,
// tcpip
	F_LOAD_DLL,
	F_SOCKET,
	F_SOCKET_GET_LEN,
	F_CONNECT,
	F_RESOLVE_IP_ADDRESS,
	F_SETSOCOPT,
	F_WSACREATEEVENT,
	F_WSAEVENTSELECT,
	F_WSAWAITFORMULTIPLEEVENTS,
	F_CHECKCONNECTION,
	F_CHECKSOCKET,
	F_SELECT,
	F_SEND,
	F_SEND_GETOVERLAPPEDRESULTS,
	F_RECV,
	F_RECV_GETOVERLAPPEDRESULTS,
	F_LISTEN,
	F_ACCEPT,
	F_BIND,
	F_DO_IO,
	F_SRVR_TRANSPORT_ERROR
};

class CInterface
{
public:
	virtual char* w_allocate(int s) = 0;
	virtual char* r_allocate(int s) = 0;
	virtual void w_release() = 0;
	virtual void r_release() = 0;
	virtual char* w_buffer() = 0;
	virtual char* r_buffer() = 0;
	virtual long w_buffer_length() = 0;
	virtual long r_buffer_length() = 0;
	virtual void w_assign(char* buffer, long length) = 0;
	virtual void r_assign(char* buffer, long length) = 0;
	virtual void  process_swap( char* buffer ) = 0;
	virtual char  swap() = 0;
	virtual char  transport() = 0;
	virtual void  send_error(short error, short error_detail, const CEE_handle_def *call_id_) = 0;
	virtual CEE_status  send_response(char* buffer, unsigned long message_length, const CEE_handle_def *call_id_) = 0;
};
//================== CTempMemory ============================

class CTempMemory_list;

class CTempMemory {
public:
	CTempMemory( const CEE_handle_def* call_id, void* p_mem, CTempMemory* cnext);
	~CTempMemory();
public:
	void* p;
private:
	CEE_handle_def m_call_id;
	CTempMemory* next;

	friend class CTempMemory_list;
};


//================== CTempMemory_list ============================

class CTempMemory_list 
{
public:
	CTempMemory_list();
	~CTempMemory_list();
	bool add_tmp_allocate(const CEE_handle_def* call_id,void*ptr);
	void del_tmp_allocate(const CEE_handle_def* call_id);
	bool del_tmp_allocate(void* ptr);
	void del_tmp_allocate();
private:
	bool add_mem(const CEE_handle_def* call_id,void*ptr);
	void del_mem(const CEE_handle_def* call_id);
	bool del_mem(void* ptr);
	bool del_mem(CTempMemory* ptr);
	void del_mem();
private:
	CTempMemory* list;

};


class CError_list;

class CError
{
public:
	CError();
	~CError();
public:
	char			platform;
	char			transport;
	int				api;
	ERROR_TYPE		error_type;
	char			process_name[MAX_PROCESS_NAME + 1];
	OPERATION		operation;
	FUNCTION		function;
	int				error;
	int				errordetail;
	long			signature;
private:
	CError* next;

	friend class CError_list;
};

class CError_list
{
public:
	CError_list();
	~CError_list();
	void cleanup();
	CError* ins_error(long signature);
	bool del_error(long signature );
	CError* find_error(long signature);
	CError* find_last_error();
private:
	CError* list;
	CRITICAL_SECTION m_ErrorCSObject;
};

class CTransportBase
{
public:
	CTransportBase();
	~CTransportBase();
	CError_list m_error_list;
	CTempMemory_list m_TempMemory_list;
	virtual void log_error(CError* ierror);
	bool bMapErrors;
protected:
private:
};

extern CEE_status
CEE_TMP_ALLOCATE(
  /* In  */ const CEE_handle_def *call_id,
  /* In  */ long len,
  /* Out */ void **ptr
);

extern CEE_status
CEE_TMP_DEALLOCATE(
  /* In  */ void    *ptr
);

extern void 
DEALLOCATE_TEMP_MEMORY(CEE_handle_def *handle);

extern void 
DEALLOCATE_ALL_TEMP_MEMORY();

extern void 
DEALLOCATE_ALL_TEMP_MEMORY(void* p);

char* 
DecodeNSKSocketErrors(int error);
char* 
DecodeNTSocketErrors(int error);
char* 
DecodeDRVRErrors(int error);
char* 
DecodeSRVRErrors(int error);
char*
DecodeERRNOErrors(int error);

void WINAPI
SET_ERROR(long signature, char platform, char transport, int api, ERROR_TYPE error_type, char* process, OPERATION operation, FUNCTION function, int error, int errordetail);
void WINAPI
RESET_ERRORS(long signature);
int WINAPI
GET_ERROR(long signature);
int WINAPI
GET_ERROR_DETAIL(long signature);
char* 
FORMAT_ERROR(CError* ierror);
char* 
FORMAT_ERROR(long signature);
char* 
FORMAT_LAST_ERROR();
char*
ERROR_TO_TEXT(CError* ierror);
char*
ERROR_TO_TEXT(long signature);
char*
LAST_ERROR_TO_TEXT();

void
FORMAT_AS_APIS(int api, char* buffer);
void
FORMAT_CFG_APIS(int api, char* buffer);
void
FORMAT_SRVR_APIS(int api, char* buffer);

bool 
BUILD_OBJECTREF(char* ObjRef, char* BuildObjRef, char* capsula, int portNumber);

void
ADD_ONE_TO_HANDLE(CEE_handle_def *handle);
extern IDL_boolean
CEE_HANDLE_IS_NIL(const CEE_handle_def *handle);
extern void
CEE_HANDLE_SET_NIL( CEE_handle_def *handle);
extern char*
ALLOC_ERROR_BUFFER();

#endif
