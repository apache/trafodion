/**********************************************************************
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
********************************************************************/
/*************************************************************************
*
**************************************************************************/
#ifndef ODBCMSG
#define ODBCMSG

#include "mxomsg.h"
#include <windows.h>

#ifdef VERSION3
	#include <string>
#else
#ifndef COLLAPSED_CFGLIB
	#include <rw/cstring.h>
#else
	#define USER_ERROR_MAX 256
#endif
#endif

// Message structure
class ODBCMXMSG_Def 
{
public:
	char	lpsSQLState[6];
#ifdef VERSION3
	std::string  lpsMsgText;
#else
#ifndef COLLAPSED_CFGLIB
	RWCString lpsMsgText;
#else
	char lpsMsgText[USER_ERROR_MAX + 1];
#endif
#endif
};


class OdbcMsg {
public:
	OdbcMsg();
	~OdbcMsg();
	int FormatODBCMessage( DWORD ErrCode, LPTSTR& lpMsgBuf, va_list* Arguments);
	BOOL GetOdbcMessage (DWORD dwLanguageId, DWORD ErrCode, ODBCMXMSG_Def *MsgStruc, ...);
	DWORD GetMsgId(DWORD ErrCode);

private:
	void FillMsgStructure (UINT ErrCode, LPSTR lpMsgBuf, ODBCMXMSG_Def *MsgStruc, int actual_len);
	void CleanUpMsgStructure(ODBCMXMSG_Def *MsgStruc);
	void ReportError(ODBCMXMSG_Def *MsgStruc, UINT MessageID, UINT error_code);
	short immu_initialize(void);
#ifndef unixcli
	short read_immu_file
	(	short            filenum,           /* IN:  Key of IMMU mesg. file       */
		char             *productnum,       /* IN:  Key of IMMU mesg. file       */
		short            err_num,           /* IN:  Key of IMMU mesg. file       */
		char             *error_text_p,     /* OUT: Pointer to the error text    */
		void             *param1,           /* IN:  Ptr to parameter 1           */
		void             *param2,           /* IN:  Ptr to parameter 2           */
		void             *param3,           /* IN:  Ptr to parameter 3           */
		void             *param4,           /* IN:  Ptr to parameter 4           */
		void             *param5);          /* IN:  Ptr to parameter 5           */
#else
	short read_immu_file(short err_num, char *error_text_p);
#endif

	void immu_close(short err_msg_fnum);
	short get_msgfile_name (void);

private:
	// global variables for the class
	BOOL		bMsgFileOpened;
	short		special_symbol;
	char		msgfile_name[EXT_FILENAME_LEN+1];
	char		product_number[PRODUCT_NO_SIZE+1];
	short		default_language;
	short		err_msg_fnum;
#ifdef unixcli
	FILE*		msgfile;
#endif	
};


#endif

