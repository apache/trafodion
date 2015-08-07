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

#include "ceercv.h"

#include <platform_ndcs.h>
#include "inoutparams.h"
#include "odbcCommon.h"
#include "odbcsrvrcommon.h"
#include "odbc_sv.h"
#include "Transport.h"
#include "marshaling.h"
#include "Global.h"

extern void logError( short Code, short Severity, short Operation );
extern char errStrBuf1[], errStrBuf2[], errStrBuf3[], errStrBuf4[], errStrBuf5[];

//================== Marshaling ==============================

CEE_status
odbcas_ASSvc_RegProcess_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ const VERSION_def *intfVersion
		, /* In    */ IDL_long srvrType
		, /* In    */ const IDL_char *srvrObjRef
		, /* In    */ const PROCESS_ID_def *nskProcessInfo
)
{
	long* parptr;
	long* mapptr;
	char* curptr;

	long wlength;
	long maplength;

	short number_of_param = RegProcess_in_params;

	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);
//
// calculate length of the buffer for each parameter
//
//
// length of VERSION_def *intfVersion
//
	wlength += sizeof(VERSION_def);
//
// IDL_long srvrType
//
	wlength += sizeof(srvrType);
//
// length of IDL_char *srvrObjRef
//
	if (srvrObjRef != NULL)
		wlength += strlen(srvrObjRef) + 1;
//
// length of PROCESS_ID_def *nskProcessInfo
//
	wlength += sizeof(PROCESS_ID_def);
//
// message_length = header + param + maplength + data length
//
	message_length = wlength + maplength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}
	parptr = (long*)buffer;
	mapptr = (long*)buffer + number_of_param;
	curptr = buffer + maplength;
//
// copy VERSION_def* intfVersion
//
	VERSION_def* par1ptr = (VERSION_def *)curptr;
	memcpy(curptr, intfVersion, sizeof(VERSION_def));
	curptr += sizeof (VERSION_def);
//
// copy IDL_long srvrType
//
	IDL_long* par2ptr = (IDL_long* )curptr;
	IDL_long_copy(&srvrType, curptr);
//
// copy IDL_char *srvrObjRef
//
	IDL_char* par3ptr = (IDL_char *)curptr;
	IDL_charArray_copy(srvrObjRef, curptr);
//
// copy PROCESS_ID_def *nskProcessInfo
//
	PROCESS_ID_def* par4ptr = (PROCESS_ID_def *)curptr;
	memcpy(curptr, nskProcessInfo, sizeof(PROCESS_ID_def));
	curptr += sizeof (PROCESS_ID_def);

	if (curptr > buffer + message_length)
	{
		strcpy( errStrBuf2, "marshalingsrvr_drvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbcas_ASSvc_RegProcess_param_pst_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
	}
//
// set end of map list
//
	*(mapptr) = 0;
//
// save relative positions of all parameters
//
	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = (long)par2ptr - (long)buffer;
	*(parptr++) = (long)par3ptr - (long)buffer;
	*(parptr++) = (long)par4ptr - (long)buffer;

	return CEE_SUCCESS;
}

CEE_status
odbcas_ASSvc_UpdateSrvrState_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
	    , /* In    */ IDL_long srvrType
		, /* In    */ const IDL_char *srvrObjRef
		, /* In    */ IDL_long srvrState
		, /* In    */ IDL_long userID
		, /* In    */ const IDL_char *userSID
)
{
	long* parptr;
	long* mapptr;
	char* curptr;

	long wlength;
	long maplength;

	short number_of_param = UpdateSrvrState_in_params;

	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);
//
// calculate length of the buffer for each parameter
//
//
// IDL_long srvrType
//
	wlength += sizeof(srvrType);
//
// length of IDL_char *srvrObjRef
//
	if (srvrObjRef != NULL)
		wlength += strlen(srvrObjRef) + 1;
//
// length of IDL_long srvrState
//
	wlength += sizeof(srvrState);

//
// length of IDL_long userID
//
	wlength += sizeof(userID);
//
// length of IDL_char *userSID
//
	if (userSID != NULL)
		wlength += strlen(userSID) + 1;


//
// message_length = header + param + maplength + data length
//
	message_length = wlength + maplength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}
	parptr = (long*)buffer;
	mapptr = (long*)buffer + number_of_param;
	curptr = buffer + maplength;
//
// copy IDL_long srvrType
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&srvrType, curptr);
//
// copy IDL_char *srvrObjRef
//
	IDL_char* par2ptr = (IDL_char *)curptr;
	IDL_charArray_copy(srvrObjRef, curptr);
//
// copy IDL_long srvrState
//
	IDL_long* par3ptr = (IDL_long* )curptr;
	IDL_long_copy(&srvrState, curptr);
//
// copy IDL_long userID
//
	IDL_long* par4ptr = (IDL_long* )curptr;
	IDL_long_copy(&userID, curptr);
//
// copy IDL_char *userSID
//
	IDL_char* par5ptr = (IDL_char *)curptr;
	IDL_charArray_copy(userSID, curptr);



	if (curptr > buffer + message_length)
	{
		strcpy( errStrBuf2, "marshalingsrvr_drvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbcas_ASSvc_UpdateSrvrState_param_pst_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
	}
//
// set end of map list
//
	*(mapptr) = 0;
//
// save relative positions of all parameters
//
	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = (long)par2ptr - (long)buffer;
	*(parptr++) = (long)par3ptr - (long)buffer;
	*(parptr++) = (long)par4ptr - (long)buffer;
	*(parptr++) = (long)par5ptr - (long)buffer;

	return CEE_SUCCESS;
}

CEE_status
odbcas_ASSvc_WouldLikeToLive_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
	    , /* In    */ IDL_long srvrType
	    , /* In    */ const IDL_char *srvrObjRef
)
{
	long* parptr;
	long* mapptr;
	char* curptr;

	long wlength;
	long maplength;

	short number_of_param = WouldLikeToLive_in_params;

	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);
//
// calculate length of the buffer for each parameter
//
//
// IDL_long srvrType
//
	wlength += sizeof(srvrType);
//
// length of IDL_char *srvrObjRef
//
	if (srvrObjRef != NULL)
		wlength += strlen(srvrObjRef) + 1;
//
// message_length = header + param + maplength + data length
//
	message_length = wlength + maplength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}
	parptr = (long*)buffer;
	mapptr = (long*)buffer + number_of_param;
	curptr = buffer + maplength;
//
// copy IDL_long srvrType
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&srvrType, curptr);
//
// copy IDL_char *srvrObjRef
//
	IDL_char* par2ptr = (IDL_char *)curptr;
	IDL_charArray_copy(srvrObjRef, curptr);

	if (curptr > buffer + message_length)
	{
		strcpy( errStrBuf2, "marshalingsrvr_drvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbcas_ASSvc_WouldLikeToLive_param_pst_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
	}
//
// set end of map list
//
	*(mapptr) = 0;
//
// save relative positions of all parameters
//
	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = (long)par2ptr - (long)buffer;

	return CEE_SUCCESS;
}

CEE_status
MxoSrvr_ValidateToken_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
  , /* In    */ int inTokenLen
  , /* In    */ unsigned char *inToken
  , /* In    */ int maxOutTokenLen
  )
{
	long* parptr;
	long* mapptr;
	char* curptr;

	long wlength;
	long maplength;

	short number_of_param = 3;

	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);
//
// calculate length of the buffer for each parameter
//

//
// inTokenLen
//
	wlength += sizeof(inTokenLen);
//
// length of  unsigned char inToken
//
	if (inTokenLen > 0)
		wlength += inTokenLen;

//
// maxOutTokenLen
//
	wlength += sizeof(maxOutTokenLen);

//
// message_length = header + param + maplength + data length
//
	message_length = wlength + maplength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}
	parptr = (long*)buffer;
	mapptr = (long*)buffer + number_of_param;
	curptr = buffer + maplength;
//
// copy inTokenLen
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&inTokenLen, curptr);
//
// copy inToken
//
	IDL_char* par2ptr = (IDL_char *)curptr;
	IDL_byteArray_copy(inToken,inTokenLen,curptr);

	if (curptr > buffer + message_length)
	{
		strcpy( errStrBuf2, "marshalingsrvr_drvr.cpp");
		strcpy( errStrBuf3, "SRVR-MxoSrvr_ValidateToken_param_pst_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
	}
//
// copy maxOutTokenLen
//
	IDL_long* par3ptr = (IDL_long* )curptr;
	IDL_long_copy(&maxOutTokenLen, curptr);

//
// set end of map list
//
	*(mapptr) = 0;
//
// save relative positions of all parameters
//
	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = (long)par2ptr - (long)buffer;
	*(parptr++) = (long)par3ptr - (long)buffer;

	return CEE_SUCCESS;

} /* MxoSrvr_ValidateToken_param_pst_ */
