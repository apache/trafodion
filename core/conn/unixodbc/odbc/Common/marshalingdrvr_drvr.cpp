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
//
#include <windows.h>
#include "cee.h"
#include "idltype.h"
#include "odbccommon.h"
#include "Transport.h"
#include "inoutparams.h"
#include "marshalingdrvr_drvr.h"

CEE_status 
odbcas_ASSvc_GetObjRefHdl_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ const CONNECTION_CONTEXT_def *inContext
		, /* In    */ const USER_DESC_def *userDesc
		, /* In    */ IDL_long srvrType
		, /* In    */ IDL_short retryCount
)
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = GetObjRefHdl_in_params;

// 4 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
// CONNECTION_CONTEXT_def *inContext
// 
	wlength += sizeof (CONNECTION_CONTEXT_def);
	STRING_length( inContext->windowText, wlength, maplength);
	LIST_length( (LIST_def*)&inContext->clientVersionList, sizeof(VERSION_def), wlength, maplength);
//
//  USER_DESC_def *userDesc
//
	wlength += sizeof (USER_DESC_def);
	OCTET_length( (OCTET_def*)&userDesc->userSid, wlength, maplength);
	STRING_length( userDesc->domainName, wlength, maplength);
	STRING_length( userDesc->userName, wlength, maplength);
	OCTET_length( (OCTET_def*)&userDesc->password, wlength, maplength);
//
// IDL_long srvrType
//
	wlength += sizeof(srvrType);
//
// IDL_short retryCount
//
	wlength += sizeof(retryCount);
//
// message length
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
// copy CONNECTION_CONTEXT_def *inContext
//
	CONNECTION_CONTEXT_def* par1ptr = (CONNECTION_CONTEXT_def* )curptr;
	memcpy(curptr, inContext, sizeof(CONNECTION_CONTEXT_def));
	curptr += sizeof(CONNECTION_CONTEXT_def);

	STRING_copy( buffer, inContext->windowText, (IDL_string*)&par1ptr->windowText, curptr, mapptr);
	LIST_copy( buffer, (LIST_def*)&inContext->clientVersionList, (LIST_def*)&par1ptr->clientVersionList, sizeof(VERSION_def), curptr, mapptr);
//
// copy USER_DESC_def *userDesc
//
	USER_DESC_def *par2ptr = (USER_DESC_def *)curptr;
	memcpy(curptr, userDesc, sizeof(USER_DESC_def));
	curptr += sizeof(USER_DESC_def);

	OCTET_copy( buffer, (OCTET_def*)&userDesc->userSid, (OCTET_def*)&par2ptr->userSid, curptr, mapptr);
	STRING_copy( buffer, userDesc->domainName, &par2ptr->domainName, curptr, mapptr);
	STRING_copy( buffer, userDesc->userName, &par2ptr->userName, curptr, mapptr);
	OCTET_copy( buffer, (OCTET_def*)&userDesc->password, (OCTET_def*)&par2ptr->password, curptr, mapptr);
//
// copy IDL_long srvrType
//
	IDL_long* par3ptr = (IDL_long* )curptr;
	IDL_long_copy(&srvrType, curptr);
//
// copy IDL_short retryCount
//
	IDL_short* par4ptr = (IDL_short* )curptr;
	IDL_short_copy(&retryCount, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}

// set end of map list

	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = (long)par2ptr - (long)buffer;
	*(parptr++) = (long)par3ptr - (long)buffer;
	*(parptr++) = (long)par4ptr - (long)buffer;

	pSystem->process_swap(buffer);

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
  )
{
	long* parptr;
	long* mapptr;
	char* curptr;

	long wlength;
	long maplength;

	short number_of_param = UpdateSrvrState_in_params;
//
// 3 input params
//
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

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
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

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbcas_ASSvc_StopSrvr_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ IDL_long srvrType
		, /* In    */ const IDL_char *srvrObjRef
		, /* In    */ IDL_long StopType
)
{
	long* parptr;
	long* mapptr;
	char* curptr;

	long wlength;
	long maplength;

	short number_of_param = StopSrvr_in_params;
//
// 4
//
	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);
//
// calculate length of the buffer for each parameter
//
//
//
// length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof(dialogueId);
//
// length of IDL_long srvrType
//
	wlength += sizeof(srvrType);
//
// length of IDL_char *srvrObjRef
//
	if (srvrObjRef != NULL)
		wlength += strlen(srvrObjRef) + 1;
//
// length of IDL_long StopType
//
	wlength += sizeof(StopType);
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_long srvrType
//
	IDL_long* par2ptr = (IDL_long* )curptr;
	IDL_long_copy(&srvrType, curptr);
//	
// copy IDL_char* srvrObjRef
//
	IDL_char* par3ptr = (IDL_char *)curptr;
	IDL_charArray_copy(srvrObjRef, curptr);
//
// copy IDL_long StopType
//
	IDL_long* par4ptr = (IDL_long* )curptr;
	IDL_long_copy(&StopType, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
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

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbcas_ASSvc_StatusDSAll_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
  )
{
	long* parptr;
	long* mapptr;
	char* curptr;

	long wlength;
	long maplength;

	short number_of_param = 0;

	wlength = sizeof(HEADER);

// input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

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
// set end of map list
//
	*(mapptr) = 0;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_InitializeDialogue_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ const USER_DESC_def *userDesc
		, /* In    */ const CONNECTION_CONTEXT_def *inContext
		, /* In    */ DIALOGUE_ID_def dialogueId
  )
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = InitializeDialogue_in_params;

// 3 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  USER_DESC_def *userDesc
//
	wlength += sizeof (USER_DESC_def);
	OCTET_length( (OCTET_def*)&userDesc->userSid, wlength, maplength);
	STRING_length( userDesc->domainName, wlength, maplength);
	STRING_length( userDesc->userName, wlength, maplength);
	OCTET_length( (OCTET_def*)&userDesc->password, wlength, maplength);
//
// CONNECTION_CONTEXT_def *inContext
// 
	wlength += sizeof (CONNECTION_CONTEXT_def);
	STRING_length( inContext->windowText, wlength, maplength);
	LIST_length((LIST_def*)&inContext->clientVersionList, sizeof(VERSION_def), wlength, maplength);
//
// length DIALOGUE_ID_def dialogueId
//
	wlength += sizeof(dialogueId);
//
// message length
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
// copy USER_DESC_def *userDesc
//
	USER_DESC_def *par1ptr = (USER_DESC_def *)curptr;
	memcpy(curptr, userDesc, sizeof(USER_DESC_def));
	curptr += sizeof(USER_DESC_def);

	OCTET_copy( buffer, (OCTET_def*)&userDesc->userSid, (OCTET_def*)&par1ptr->userSid, curptr, mapptr);
	STRING_copy( buffer, userDesc->domainName, &par1ptr->domainName, curptr, mapptr);
	STRING_copy( buffer, userDesc->userName, &par1ptr->userName, curptr, mapptr);
	OCTET_copy( buffer, (OCTET_def*)&userDesc->password, (OCTET_def*)&par1ptr->password, curptr, mapptr);
//
// copy CONNECTION_CONTEXT_def *inContext
//
	CONNECTION_CONTEXT_def* par2ptr = (CONNECTION_CONTEXT_def* )curptr;
	memcpy(curptr, inContext, sizeof(CONNECTION_CONTEXT_def));
	curptr += sizeof(CONNECTION_CONTEXT_def);

	STRING_copy( buffer, inContext->windowText, (IDL_string*)&par2ptr->windowText, curptr, mapptr);
	LIST_copy(buffer, (LIST_def*)&inContext->clientVersionList, (LIST_def*)&par2ptr->clientVersionList, sizeof(VERSION_def), curptr, mapptr);
//
//
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par3ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list

	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = (long)par2ptr - (long)buffer;
	*(parptr++) = (long)par3ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_TerminateDialogue_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
  )
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = TerminateDialogue_in_params;

// 1 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
// DIALOGUE_ID_def dialogueId
//
	wlength += sizeof(dialogueId);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list

	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_SetConnectionOption_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ IDL_short connectionOption
		, /* In    */ IDL_long optionValueNum
		, /* In    */ IDL_string optionValueStr
  )
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = SetConnectionOption_in_params;

// 4 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_short connectionOption
// 
	wlength += sizeof (connectionOption);
//
// length IDL_long optionValueNum
//
	wlength += sizeof(optionValueNum);
//
// length of IDL_string optionValueStr
//
	if (optionValueStr != NULL)
		wlength += strlen(optionValueStr) + 1;
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_short connectionOption
//
	IDL_short* par2ptr = (IDL_short* )curptr;
	IDL_short_copy(&connectionOption, curptr);
//
//
// copy IDL_long optionValueNum
//
	IDL_long* par3ptr = (IDL_long* )curptr;
	IDL_long_copy(&optionValueNum, curptr);
//
//
//
// copy IDL_string optionValueStr
//
	IDL_char* par4ptr = (IDL_char* )curptr;
	if (optionValueStr != NULL)
		IDL_charArray_copy(optionValueStr, curptr);
	else
		par4ptr = NULL;

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = (long)par2ptr - (long)buffer;
	*(parptr++) = (long)par3ptr - (long)buffer;
	*(parptr++) = par4ptr != NULL ?(long)par4ptr - (long)buffer: NULL;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_Prepare_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ const IDL_char *stmtExplainLabel
		, /* In    */ IDL_short stmtType
		, /* In    */ IDL_string sqlString
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
)
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = Prepare_in_params;

// 7 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_char *stmtLabel
// 
	if (stmtLabel != NULL)
		wlength += strlen(stmtLabel)+1;
//
// length of IDL_char *stmtExplainLabel
//
	if (stmtExplainLabel != NULL)
		wlength += strlen(stmtExplainLabel) + 1;
//
// length of IDL_short stmtType
//
	wlength += sizeof (stmtType);
//
// length of IDL_string sqlString
//
	if (sqlString != NULL)
		wlength += strlen(sqlString) + 1;
//
// length of IDL_short sqlAsyncEnable
//
	wlength += sizeof (sqlAsyncEnable);
//
// length of IDL_long queryTimeout
//
	wlength += sizeof (queryTimeout);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_char *stmtLabel
//
	IDL_char* par2ptr = (IDL_char* )curptr;
	if (stmtLabel != NULL)
		IDL_charArray_copy(stmtLabel, curptr);
	else
		par2ptr = NULL;
//
// copy IDL_char *stmtExplainLabel
//
	IDL_char* par3ptr = (IDL_char* )curptr;
	if (stmtExplainLabel != NULL)
		IDL_charArray_copy(stmtExplainLabel, curptr);
	else
		par3ptr = NULL;
//
// copy IDL_short stmtType
//
	IDL_short* par4ptr = (IDL_short* )curptr;
	IDL_short_copy(&stmtType, curptr);
//
// copy IDL_string sqlString
//
	IDL_char* par5ptr = (IDL_char* )curptr;
	if (sqlString != NULL)
		IDL_charArray_copy(sqlString, curptr);
	else
		par5ptr = NULL;
//
// copy IDL_short sqlAsyncEnable
//
	IDL_short* par6ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlAsyncEnable, curptr);
//
// copy IDL_long queryTimeout
//
	IDL_long* par7ptr = (IDL_long* )curptr;
	IDL_long_copy(&queryTimeout, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = par2ptr != NULL ?(long)par2ptr - (long)buffer: NULL;
	*(parptr++) = par3ptr != NULL ?(long)par3ptr - (long)buffer: NULL;
	*(parptr++) = (long)par4ptr - (long)buffer;
	*(parptr++) = par5ptr != NULL ?(long)par5ptr - (long)buffer: NULL;
	*(parptr++) = (long)par6ptr - (long)buffer;
	*(parptr++) = (long)par7ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_ExecDirect_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_string cursorName
		, /* In    */ const IDL_char *stmtExplainLabel
		, /* In    */ IDL_short stmtType
		, /* In    */ IDL_short sqlStmtType
		, /* In    */ IDL_string sqlString
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
  )
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = ExecDirect_in_params;

// 9 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_char *stmtLabel
// 
	if (stmtLabel != NULL)
		wlength += strlen(stmtLabel)+1;
//
// length IDL_string cursorName
//
	if (cursorName != NULL)
		wlength += strlen(cursorName)+1;
//
// length of IDL_char *stmtExplainLabel
//
	if (stmtExplainLabel != NULL)
		wlength += strlen(stmtExplainLabel) + 1;
//
// length of IDL_short stmtType
//
	wlength += sizeof (stmtType);
//
// length of IDL_short sqlStmtType
//
	wlength += sizeof (sqlStmtType);
//
// length of IDL_string sqlString
//
	if (sqlString != NULL)
		wlength += strlen(sqlString) + 1;
//
// length of IDL_short sqlAsyncEnable
//
	wlength += sizeof (sqlAsyncEnable);
//
// length of IDL_long queryTimeout
//
	wlength += sizeof (queryTimeout);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_char *stmtLabel
//
	IDL_char* par2ptr = (IDL_char* )curptr;
	if (stmtLabel != NULL)
		IDL_charArray_copy(stmtLabel, curptr);
	else
		par2ptr = NULL;
//
// copy IDL_string cursorName
//
	IDL_char* par3ptr = (IDL_char* )curptr;
	if (cursorName != NULL)
		IDL_charArray_copy(cursorName, curptr);
	else
		par3ptr = NULL;
//
// copy IDL_char *stmtExplainLabel
//
	IDL_char* par4ptr = (IDL_char* )curptr;
	if (stmtExplainLabel != NULL)
		IDL_charArray_copy(stmtExplainLabel, curptr);
	else
		par4ptr = NULL;
//
// copy IDL_short stmtType
//
	IDL_short* par5ptr = (IDL_short* )curptr;
	IDL_short_copy(&stmtType, curptr);
//
// copy IDL_short sqlStmtType
//
	IDL_short* par6ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlStmtType, curptr);
//
// copy IDL_string sqlString
//
	IDL_char* par7ptr = (IDL_char* )curptr;
	if (sqlString != NULL)
		IDL_charArray_copy(sqlString, curptr);
	else
		par7ptr = NULL;
//
// copy IDL_short sqlAsyncEnable
//
	IDL_short* par8ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlAsyncEnable, curptr);
//
// copy IDL_long queryTimeout
//
	IDL_long* par9ptr = (IDL_long* )curptr;
	IDL_long_copy(&queryTimeout, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = par2ptr != NULL ?(long)par2ptr - (long)buffer: NULL;
	*(parptr++) = par3ptr != NULL ?(long)par3ptr - (long)buffer: NULL;;
	*(parptr++) = par4ptr != NULL ?(long)par4ptr - (long)buffer: NULL;
	*(parptr++) = (long)par5ptr - (long)buffer;
	*(parptr++) = (long)par6ptr - (long)buffer;
	*(parptr++) = par7ptr != NULL ?(long)par7ptr - (long)buffer: NULL;
	*(parptr++) = (long)par8ptr - (long)buffer;
	*(parptr++) = (long)par9ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_PrepareRowset_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ const IDL_char *stmtExplainLabel
		, /* In    */ IDL_short stmtType
		, /* In    */ IDL_short sqlStmtType
		, /* In    */ IDL_string sqlString
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
		, /* In    */ IDL_long maxRowsetSize
  )
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = PrepareRowset_in_params;

// 9 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_char *stmtLabel
// 
	if (stmtLabel != NULL)
		wlength += strlen(stmtLabel)+1;
//
// length of IDL_char *stmtExplainLabel
//
	if (stmtExplainLabel != NULL)
		wlength += strlen(stmtExplainLabel) + 1;
//
// length of IDL_short stmtType
//
	wlength += sizeof (stmtType);
//
// length of IDL_short sqlStmtType
//
	wlength += sizeof (sqlStmtType);
//
// length of IDL_string sqlString
//
	if (sqlString != NULL)
		wlength += strlen(sqlString) + 1;
//
// length of IDL_short sqlAsyncEnable
//
	wlength += sizeof (sqlAsyncEnable);
//
// length of IDL_long queryTimeout
//
	wlength += sizeof (queryTimeout);
//
// length of IDL_long maxRowsetSize
//
	wlength += sizeof (maxRowsetSize);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_char *stmtLabel
//
	IDL_char* par2ptr = (IDL_char* )curptr;
	if (stmtLabel != NULL)
		IDL_charArray_copy(stmtLabel, curptr);
	else
		par2ptr = NULL;
//
// copy IDL_char *stmtExplainLabel
//
	IDL_char* par3ptr = (IDL_char* )curptr;
	if (stmtExplainLabel != NULL)
		IDL_charArray_copy(stmtExplainLabel, curptr);
	else
		par3ptr = NULL;
//
// copy IDL_short stmtType
//
	IDL_short* par4ptr = (IDL_short* )curptr;
	IDL_short_copy(&stmtType, curptr);
//
// copy IDL_short sqlStmtType
//
	IDL_short* par5ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlStmtType, curptr);
//
// copy IDL_string sqlString
//
	IDL_char* par6ptr = (IDL_char* )curptr;
	if (sqlString != NULL)
		IDL_charArray_copy(sqlString, curptr);
	else
		par6ptr = NULL;
//
// copy IDL_short sqlAsyncEnable
//
	IDL_short* par7ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlAsyncEnable, curptr);
//
// copy IDL_long queryTimeout
//
	IDL_long* par8ptr = (IDL_long* )curptr;
	IDL_long_copy(&queryTimeout, curptr);
//
// copy IDL_long maxRowsetSize
//
	IDL_long* par9ptr = (IDL_long* )curptr;
	IDL_long_copy(&maxRowsetSize, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = par2ptr != NULL ?(long)par2ptr - (long)buffer: NULL;
	*(parptr++) = par3ptr != NULL ?(long)par3ptr - (long)buffer: NULL;
	*(parptr++) = (long)par4ptr - (long)buffer;
	*(parptr++) = (long)par5ptr - (long)buffer;
	*(parptr++) = par6ptr != NULL ?(long)par6ptr - (long)buffer: NULL;
	*(parptr++) = (long)par7ptr - (long)buffer;
	*(parptr++) = (long)par8ptr - (long)buffer;
	*(parptr++) = (long)par9ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}


CEE_status
odbc_SQLSvc_ExecuteN_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_string cursorName
		, /* In    */ IDL_short sqlStmtType
		, /* In    */ IDL_long inputRowCnt
		, /* In    */ const SQLValueList_def *inputValueList
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
)
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = ExecuteN_in_params;

// 8 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_char *stmtLabel
// 
	if (stmtLabel != NULL)
		wlength += strlen(stmtLabel)+1;
//
// length IDL_string cursorName
//
	if (cursorName != NULL)
		wlength += strlen(cursorName)+1;
//
// length of IDL_short sqlStmtType
//
	wlength += sizeof (sqlStmtType);
//
// length of IDL_long inputRowCnt
//
	wlength += sizeof (inputRowCnt);
//
// length of SQLValueList_def *inputValueList
//
	wlength += sizeof(SQLValueList_def);
	SQLVALUE_LIST_length(inputValueList, wlength, maplength);
//
// length of IDL_short sqlAsyncEnable
//
	wlength += sizeof (sqlAsyncEnable);
//
// length of IDL_long queryTimeout
//
	wlength += sizeof (queryTimeout);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_char *stmtLabel
//
	IDL_char* par2ptr = (IDL_char* )curptr;
	if (stmtLabel != NULL)
		IDL_charArray_copy(stmtLabel, curptr);
	else
		par2ptr = NULL;
//
// copy IDL_string cursorName
//
	IDL_char* par3ptr = (IDL_char* )curptr;
	if (cursorName != NULL)
		IDL_charArray_copy(cursorName, curptr);
	else
		par3ptr = NULL;
//
// copy IDL_short sqlStmtType
//
	IDL_short* par4ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlStmtType, curptr);
//
// copy IDL_long inputRowCnt
//
	IDL_long* par5ptr = (IDL_long* )curptr;
	IDL_long_copy(&inputRowCnt, curptr);
//
// copy SQLValueList_def *inputValueList
//
	SQLValueList_def* par6ptr = (SQLValueList_def *)curptr;
	SQLVALUE_LIST_copy( buffer, inputValueList, par6ptr, curptr, mapptr);
//
// copy IDL_short sqlAsyncEnable
//
	IDL_short* par7ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlAsyncEnable, curptr);
//
// copy IDL_long queryTimeout
//
	IDL_long* par8ptr = (IDL_long* )curptr;
	IDL_long_copy(&queryTimeout, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = par2ptr != NULL ?(long)par2ptr - (long)buffer: NULL;
	*(parptr++) = par3ptr != NULL ?(long)par3ptr - (long)buffer: NULL;
	*(parptr++) = (long)par4ptr - (long)buffer;
	*(parptr++) = (long)par5ptr - (long)buffer;
	*(parptr++) = (long)par6ptr - (long)buffer;
	*(parptr++) = (long)par7ptr - (long)buffer;
	*(parptr++) = (long)par8ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}


CEE_status
odbc_SQLSvc_ExecuteRowset_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_string cursorName
		, /* In    */ IDL_short sqlStmtType
		, /* In    */ IDL_long inputRowCnt
		, /* In    */ const SQL_DataValue_def *inputDataValue
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
)
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = ExecuteRowset_in_params;

// 8 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_char *stmtLabel
// 
	if (stmtLabel != NULL)
		wlength += strlen(stmtLabel)+1;
//
// length IDL_string cursorName
//
	if (cursorName != NULL)
		wlength += strlen(cursorName)+1;
//
// length of IDL_short sqlStmtType
//
	wlength += sizeof (sqlStmtType);
//
// length of IDL_long inputRowCnt
//
	wlength += sizeof (inputRowCnt);
//
// length of SQL_DataValue_def *inputDataValue
//
	wlength += sizeof(SQL_DataValue_def);
	LIST_length( (LIST_def*)inputDataValue, sizeof(IDL_octet), wlength, maplength);
//
// length of IDL_short sqlAsyncEnable
//
	wlength += sizeof (sqlAsyncEnable);
//
// length of IDL_long queryTimeout
//
	wlength += sizeof (queryTimeout);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_char *stmtLabel
//
	IDL_char* par2ptr = (IDL_char* )curptr;
	if (stmtLabel != NULL)
		IDL_charArray_copy(stmtLabel, curptr);
	else
		par2ptr = NULL;
//
// copy IDL_string cursorName
//
	IDL_char* par3ptr = (IDL_char* )curptr;
	if (cursorName != NULL)
		IDL_charArray_copy(cursorName, curptr);
	else
		par3ptr = NULL;
//
// copy IDL_short sqlStmtType
//
	IDL_short* par4ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlStmtType, curptr);
//
// copy IDL_long inputRowCnt
//
	IDL_long* par5ptr = (IDL_long* )curptr;
	IDL_long_copy(&inputRowCnt, curptr);
//
// copy SQL_DataValue_def *inputDataValue
//
	SQL_DataValue_def* par6ptr = (SQL_DataValue_def *)curptr;
	memcpy(curptr, inputDataValue, sizeof(SQL_DataValue_def));
	curptr += sizeof (SQL_DataValue_def);
	LIST_copy( buffer, (LIST_def*) inputDataValue, (LIST_def*) par6ptr, sizeof(IDL_octet), curptr, mapptr);
//
// copy IDL_short sqlAsyncEnable
//
	IDL_short* par7ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlAsyncEnable, curptr);
//
// copy IDL_long queryTimeout
//
	IDL_long* par8ptr = (IDL_long* )curptr;
	IDL_long_copy(&queryTimeout, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = par2ptr != NULL ?(long)par2ptr - (long)buffer: NULL;
	*(parptr++) = par3ptr != NULL ?(long)par3ptr - (long)buffer: NULL;
	*(parptr++) = (long)par4ptr - (long)buffer;
	*(parptr++) = (long)par5ptr - (long)buffer;
	*(parptr++) = (long)par6ptr - (long)buffer;
	*(parptr++) = (long)par7ptr - (long)buffer;
	*(parptr++) = (long)par8ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_ExecDirectRowset_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_string cursorName
		, /* In    */ const IDL_char *stmtExplainLabel
		, /* In    */ IDL_short stmtType
		, /* In    */ IDL_short sqlStmtType
		, /* In    */ IDL_string sqlString
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
		, /* In    */ IDL_long maxRowsetSize
)
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = ExecDirectRowset_in_params;

// 10 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_char *stmtLabel
// 
	if (stmtLabel != NULL)
		wlength += strlen(stmtLabel)+1;
//
// length IDL_string cursorName
//
	if (cursorName != NULL)
		wlength += strlen(cursorName)+1;
//
// length of IDL_char *stmtExplainLabel
//
	if (stmtExplainLabel != NULL)
		wlength += strlen(stmtExplainLabel) + 1;
//
// length of IDL_short stmtType
//
	wlength += sizeof (stmtType);
//
// length of IDL_short sqlStmtType
//
	wlength += sizeof (sqlStmtType);
//
// length of IDL_string sqlString
//
	if (sqlString != NULL)
		wlength += strlen(sqlString) + 1;
//
// length of IDL_short sqlAsyncEnable
//
	wlength += sizeof (sqlAsyncEnable);
//
// length of IDL_long queryTimeout
//
	wlength += sizeof (queryTimeout);
//
// length of IDL_long maxRowsetSize
//
	wlength += sizeof (maxRowsetSize);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_char *stmtLabel
//
	IDL_char* par2ptr = (IDL_char* )curptr;
	if (stmtLabel != NULL)
		IDL_charArray_copy(stmtLabel, curptr);
	else
		par2ptr = NULL;
//
// copy IDL_string cursorName
//
	IDL_char* par3ptr = (IDL_char* )curptr;
	if (cursorName != NULL)
		IDL_charArray_copy(cursorName, curptr);
	else
		par3ptr = NULL;
//
// copy IDL_char *stmtExplainLabel
//
	IDL_char* par4ptr = (IDL_char* )curptr;
	if (stmtExplainLabel != NULL)
		IDL_charArray_copy(stmtExplainLabel, curptr);
	else
		par4ptr = NULL;
//
// copy IDL_short stmtType
//
	IDL_short* par5ptr = (IDL_short* )curptr;
	IDL_short_copy(&stmtType, curptr);
//
// copy IDL_short sqlStmtType
//
	IDL_short* par6ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlStmtType, curptr);
//
// copy IDL_string sqlString
//
	IDL_char* par7ptr = (IDL_char* )curptr;
	if (sqlString != NULL)
		IDL_charArray_copy(sqlString, curptr);
	else
		par7ptr = NULL;
//
// copy IDL_short sqlAsyncEnable
//
	IDL_short* par8ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlAsyncEnable, curptr);
//
// copy IDL_long queryTimeout
//
	IDL_long* par9ptr = (IDL_long* )curptr;
	IDL_long_copy(&queryTimeout, curptr);
//
// copy IDL_long maxRowsetSize
//
	IDL_long* par10ptr = (IDL_long* )curptr;
	IDL_long_copy(&maxRowsetSize, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = par2ptr != NULL ?(long)par2ptr - (long)buffer: NULL;
	*(parptr++) = par3ptr != NULL ?(long)par3ptr - (long)buffer: NULL;
	*(parptr++) = par4ptr != NULL ?(long)par4ptr - (long)buffer: NULL;
	*(parptr++) = (long)par5ptr - (long)buffer;
	*(parptr++) = (long)par6ptr - (long)buffer;
	*(parptr++) = par7ptr != NULL ?(long)par7ptr - (long)buffer: NULL;
	*(parptr++) = (long)par8ptr - (long)buffer;
	*(parptr++) = (long)par9ptr - (long)buffer;
	*(parptr++) = (long)par10ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_FetchRowset_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_long maxRowCnt
		, /* In    */ IDL_long maxRowLen
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
)
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = FetchRowset_in_params;

// 6 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_char *stmtLabel
// 
	if (stmtLabel != NULL)
		wlength += strlen(stmtLabel)+1;
//
// length of IDL_long maxRowCnt
//
	wlength += sizeof (maxRowCnt);
//
// length of IDL_long maxRowLen
//
	wlength += sizeof (maxRowLen);
//
// length of IDL_short sqlAsyncEnable
//
	wlength += sizeof (sqlAsyncEnable);
//
// length of IDL_long queryTimeout
//
	wlength += sizeof (queryTimeout);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_char *stmtLabel
//
	IDL_char* par2ptr = (IDL_char* )curptr;
	if (stmtLabel != NULL)
		IDL_charArray_copy(stmtLabel, curptr);
	else
		par2ptr = NULL;
//
// copy IDL_long maxRowCnt
//
	IDL_long* par3ptr = (IDL_long* )curptr;
	IDL_long_copy(&maxRowCnt, curptr);
//
// copy IDL_long maxRowLen
//
	IDL_long* par4ptr = (IDL_long* )curptr;
	IDL_long_copy(&maxRowLen, curptr);
//
// copy IDL_short sqlAsyncEnable
//
	IDL_short* par5ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlAsyncEnable, curptr);
//
// copy IDL_long queryTimeout
//
	IDL_long* par6ptr = (IDL_long* )curptr;
	IDL_long_copy(&queryTimeout, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = par2ptr != NULL ?(long)par2ptr - (long)buffer: NULL;
	*(parptr++) = (long)par3ptr - (long)buffer;
	*(parptr++) = (long)par4ptr - (long)buffer;
	*(parptr++) = (long)par5ptr - (long)buffer;
	*(parptr++) = (long)par6ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_FetchPerf_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_long maxRowCnt
		, /* In    */ IDL_long maxRowLen
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
)
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = FetchPerf_in_params;

// 6 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_char *stmtLabel
// 
	if (stmtLabel != NULL)
		wlength += strlen(stmtLabel)+1;
//
// length of IDL_long maxRowCnt
//
	wlength += sizeof (maxRowCnt);
//
// length of IDL_long maxRowLen
//
	wlength += sizeof (maxRowLen);
//
// length of IDL_short sqlAsyncEnable
//
	wlength += sizeof (sqlAsyncEnable);
//
// length of IDL_long queryTimeout
//
	wlength += sizeof (queryTimeout);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_char *stmtLabel
//
	IDL_char* par2ptr = (IDL_char* )curptr;
	if (stmtLabel != NULL)
		IDL_charArray_copy(stmtLabel, curptr);
	else
		par2ptr = NULL;
//
// copy IDL_long maxRowCnt
//
	IDL_long* par3ptr = (IDL_long* )curptr;
	IDL_long_copy(&maxRowCnt, curptr);
//
// copy IDL_long maxRowLen
//
	IDL_long* par4ptr = (IDL_long* )curptr;
	IDL_long_copy(&maxRowLen, curptr);
//
// copy IDL_short sqlAsyncEnable
//
	IDL_short* par5ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlAsyncEnable, curptr);
//
// copy IDL_long queryTimeout
//
	IDL_long* par6ptr = (IDL_long* )curptr;
	IDL_long_copy(&queryTimeout, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = par2ptr != NULL ?(long)par2ptr - (long)buffer: NULL;
	*(parptr++) = (long)par3ptr - (long)buffer;
	*(parptr++) = (long)par4ptr - (long)buffer;
	*(parptr++) = (long)par5ptr - (long)buffer;
	*(parptr++) = (long)par6ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}


CEE_status
odbc_SQLSvc_Close_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_unsigned_short freeResourceOpt
  )
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = Close_in_params;

// 3 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_char *stmtLabel
// 
	if (stmtLabel != NULL)
		wlength += strlen(stmtLabel)+1;
//
// length of IDL_unsigned_short freeResourceOpt
//
	wlength += sizeof (freeResourceOpt);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_char *stmtLabel
//
	IDL_char* par2ptr = (IDL_char* )curptr;
	if (stmtLabel != NULL)
		IDL_charArray_copy(stmtLabel, curptr);
	else
		par2ptr = NULL;
//
// copy IDL_unsigned_short freeResourceOpt
//
	IDL_unsigned_short* par3ptr = (IDL_unsigned_short* )curptr;
	IDL_unsigned_short_copy(&freeResourceOpt, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = par2ptr != NULL ?(long)par2ptr - (long)buffer: NULL;
	*(parptr++) = (long)par3ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_EndTransaction_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
	    , /* In    */ DIALOGUE_ID_def dialogueId
	    , /* In    */ IDL_unsigned_short transactionOpt
)
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = EndTransaction_in_params;

// 2 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_unsigned_short transactionOpt
//
	wlength += sizeof (transactionOpt);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_unsigned_short transactionOpt
//
	IDL_unsigned_short* par2ptr = (IDL_unsigned_short* )curptr;
	IDL_unsigned_short_copy(&transactionOpt, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = (long)par2ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_GetSQLCatalogs_param_pst_(
 		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_short APIType
		, /* In    */ const IDL_char *catalogNm
		, /* In    */ const IDL_char *schemaNm
		, /* In    */ const IDL_char *tableNm
		, /* In    */ const IDL_char *tableTypeList
		, /* In    */ const IDL_char *columnNm
		, /* In    */ IDL_long columnType
		, /* In    */ IDL_long rowIdScope
		, /* In    */ IDL_long nullable
		, /* In    */ IDL_long uniqueness
		, /* In    */ IDL_long accuracy
		, /* In    */ IDL_short sqlType
		, /* In    */ IDL_unsigned_long metadataId
	    , /* In    */ const IDL_char *fkcatalogNm
	    , /* In    */ const IDL_char *fkschemaNm
	    , /* In    */ const IDL_char *fktableNm
)
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = GetSQLCatalogs_in_params;

// 18 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_char *stmtLabel
// 
	if (stmtLabel != NULL)
		wlength += strlen(stmtLabel)+1;
//
// length of IDL_short APIType
//
	wlength += sizeof (APIType);
//
// length of IDL_char *catalogNm
// 
	if (catalogNm != NULL)
		wlength += strlen(catalogNm)+1;
//
// length of IDL_char *schemaNm
// 
	if (schemaNm != NULL)
		wlength += strlen(schemaNm)+1;
//
// length of IDL_char *tableNm
// 
	if (tableNm != NULL)
		wlength += strlen(tableNm)+1;
//
// length of IDL_char *tableTypeList
// 
	if (tableTypeList != NULL)
		wlength += strlen(tableTypeList)+1;
//
// length of IDL_char *columnNm
// 
	if (columnNm != NULL)
		wlength += strlen(columnNm)+1;
//
// length of IDL_long columnType
//
	wlength += sizeof (columnType);
//
// length of IDL_long rowIdScope
//
	wlength += sizeof (rowIdScope);
//
// length of IDL_long nullable
//
	wlength += sizeof (nullable);
//
// length of IDL_long uniqueness
//
	wlength += sizeof (uniqueness);
//
// length of IDL_long accuracy
//
	wlength += sizeof (accuracy);
//
// length of IDL_short sqlType
//
	wlength += sizeof (sqlType);
//
// length of IDL_unsigned_long metadataId
//
	wlength += sizeof (metadataId);
//
// length of IDL_char *fkcatalogNm
//
	if (fkcatalogNm != NULL)
		wlength += strlen(fkcatalogNm)+1;
//
// length of IDL_char *fkschemaNm
//
	if (fkschemaNm != NULL)
		wlength += strlen(fkschemaNm)+1;
//
// length of IDL_char *fktableNm
//
	if (fktableNm != NULL)
		wlength += strlen(fktableNm)+1;
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_char *stmtLabel
//
	IDL_char* par2ptr = (IDL_char* )curptr;
	if (stmtLabel != NULL)
		IDL_charArray_copy(stmtLabel, curptr);
	else
		par2ptr = NULL;
//
// copy IDL_short APIType
//
	IDL_short* par3ptr = (IDL_short* )curptr;
	IDL_short_copy(&APIType, curptr);
//
// copy IDL_char *catalogNm
//
	IDL_char* par4ptr = (IDL_char* )curptr;
	if (catalogNm != NULL)
		IDL_charArray_copy(catalogNm, curptr);
	else
		par4ptr = NULL;
//
// copy IDL_char *schemaNm
//
	IDL_char* par5ptr = (IDL_char* )curptr;
	if (schemaNm != NULL)
		IDL_charArray_copy(schemaNm, curptr);
	else
		par5ptr = NULL;
//
// copy IDL_char *tableNm
//
	IDL_char* par6ptr = (IDL_char* )curptr;
	if (tableNm != NULL)
		IDL_charArray_copy(tableNm, curptr);
	else
		par6ptr = NULL;
//
// copy IDL_char *tableTypeList
//
	IDL_char* par7ptr = (IDL_char* )curptr;
	if (tableTypeList != NULL)
		IDL_charArray_copy(tableTypeList, curptr);
	else
		par7ptr = NULL;
//
// copy IDL_char *columnNm
//
	IDL_char* par8ptr = (IDL_char* )curptr;
	if (columnNm != NULL)
		IDL_charArray_copy(columnNm, curptr);
	else
		par8ptr = NULL;
//
// copy IDL_long columnType
//
	IDL_long* par9ptr = (IDL_long* )curptr;
	IDL_long_copy(&columnType, curptr);
//
// copy IDL_long rowIdScope
//
	IDL_long* par10ptr = (IDL_long* )curptr;
	IDL_long_copy(&rowIdScope, curptr);
//
// copy IDL_long nullable
//
	IDL_long* par11ptr = (IDL_long* )curptr;
	IDL_long_copy(&nullable, curptr);
//
// copy IDL_long uniqueness
//
	IDL_long* par12ptr = (IDL_long* )curptr;
	IDL_long_copy(&uniqueness, curptr);
//
// copy IDL_long accuracy
//
	IDL_long* par13ptr = (IDL_long* )curptr;
	IDL_long_copy(&accuracy, curptr);
//
// copy IDL_short sqlType
//
	IDL_short* par14ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlType, curptr);
//
// copy IDL_unsigned_long metadataId
//
	IDL_unsigned_long* par15ptr = (IDL_unsigned_long* )curptr;
	IDL_unsigned_long_copy(&metadataId, curptr);
//
// copy IDL_char *fkcatalogNm
//
	IDL_char* par16ptr = (IDL_char* )curptr;
	if (fkcatalogNm != NULL)
		IDL_charArray_copy(fkcatalogNm, curptr);
	else
		par16ptr = NULL;
//
// copy IDL_char *fkschemaNm
//
	IDL_char* par17ptr = (IDL_char* )curptr;
	if (fkschemaNm != NULL)
		IDL_charArray_copy(fkschemaNm, curptr);
	else
		par17ptr = NULL;
//
// copy IDL_char *fktableNm
//
	IDL_char* par18ptr = (IDL_char* )curptr;
	if (fktableNm != NULL)
		IDL_charArray_copy(fktableNm, curptr);
	else
		par18ptr = NULL;

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = par2ptr != NULL ?(long)par2ptr - (long)buffer: NULL;
	*(parptr++) = (long)par3ptr - (long)buffer;
	*(parptr++) = par4ptr != NULL ?(long)par4ptr - (long)buffer: NULL;
	*(parptr++) = par5ptr != NULL ?(long)par5ptr - (long)buffer: NULL;
	*(parptr++) = par6ptr != NULL ?(long)par6ptr - (long)buffer: NULL;
	*(parptr++) = par7ptr != NULL ?(long)par7ptr - (long)buffer: NULL;
	*(parptr++) = par8ptr != NULL ?(long)par8ptr - (long)buffer: NULL;
	*(parptr++) = (long)par9ptr - (long)buffer;
	*(parptr++) = (long)par10ptr - (long)buffer;
	*(parptr++) = (long)par11ptr - (long)buffer;
	*(parptr++) = (long)par12ptr - (long)buffer;
	*(parptr++) = (long)par13ptr - (long)buffer;
	*(parptr++) = (long)par14ptr - (long)buffer;
	*(parptr++) = (long)par15ptr - (long)buffer;
	*(parptr++) = par16ptr != NULL ?(long)par16ptr - (long)buffer: NULL;
	*(parptr++) = par17ptr != NULL ?(long)par17ptr - (long)buffer: NULL;
	*(parptr++) = par18ptr != NULL ?(long)par18ptr - (long)buffer: NULL;
	
	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_ExecuteCall_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_string cursorName
		, /* In    */ IDL_short sqlStmtType
		, /* In    */ IDL_long inputRowCnt
		, /* In    */ const SQLValueList_def *inputValueList
		, /* In    */ IDL_short sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
)
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = ExecuteCall_in_params;

// 8 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// length of IDL_char *stmtLabel
// 
	if (stmtLabel != NULL)
		wlength += strlen(stmtLabel)+1;
//
// length IDL_string cursorName
//
	if (cursorName != NULL)
		wlength += strlen(cursorName)+1;
//
// length of IDL_short sqlStmtType
//
	wlength += sizeof (sqlStmtType);
//
// length of IDL_long inputRowCnt
//
	wlength += sizeof (inputRowCnt);
//
// length of SQLValueList_def *inputValueList
//
	SQLVALUE_LIST_length( inputValueList, wlength, maplength);
//
// length of IDL_short sqlAsyncEnable
//
	wlength += sizeof (sqlAsyncEnable);
//
// length of IDL_long queryTimeout
//
	wlength += sizeof (queryTimeout);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_char *stmtLabel
//
	IDL_char* par2ptr = (IDL_char* )curptr;
	if (stmtLabel != NULL)
		IDL_charArray_copy(stmtLabel, curptr);
	else
		par2ptr = NULL;
//
// copy IDL_string cursorName
//
	IDL_char* par3ptr = (IDL_char* )curptr;
	if (cursorName != NULL)
		IDL_charArray_copy(cursorName, curptr);
	else
		par3ptr = NULL;
//
// copy IDL_short sqlStmtType
//
	IDL_short* par4ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlStmtType, curptr);
//
// copy IDL_long inputRowCnt
//
	IDL_long* par5ptr = (IDL_long* )curptr;
	IDL_long_copy(&inputRowCnt, curptr);
//
// copy SQLValueList_def *inputValueList
//
	SQLValueList_def* par6ptr = (SQLValueList_def *)curptr;
	SQLVALUE_LIST_copy( buffer, inputValueList, par6ptr, curptr, mapptr);
//
// copy IDL_short sqlAsyncEnable
//
	IDL_short* par7ptr = (IDL_short* )curptr;
	IDL_short_copy(&sqlAsyncEnable, curptr);
//
// copy IDL_long queryTimeout
//
	IDL_long* par8ptr = (IDL_long* )curptr;
	IDL_long_copy(&queryTimeout, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;
	*(parptr++) = par2ptr != NULL ?(long)par2ptr - (long)buffer: NULL;
	*(parptr++) = par3ptr != NULL ?(long)par3ptr - (long)buffer: NULL;
	*(parptr++) = (long)par4ptr - (long)buffer;
	*(parptr++) = (long)par5ptr - (long)buffer;
	*(parptr++) = (long)par6ptr - (long)buffer;
	*(parptr++) = (long)par7ptr - (long)buffer;
	*(parptr++) = (long)par8ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_MonitorCall_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
)
{
	long maplength,wlength;
	char *curptr;
	long* parptr, *mapptr;

	short number_of_param = MonitorCall_in_params;

// 1 input parameter pointers plus 'end' indicator

	maplength = (number_of_param + 1) * sizeof(long);

	wlength = 0;
//
// calculate length of the buffer for each parameter
//
//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof (dialogueId);
//
// message length
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
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&dialogueId, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}
//
// set end of map list
//
	*(mapptr) = 0;

// save relative positions of all parameters

	*(parptr++) = (long)par1ptr - (long)buffer;

	pSystem->process_swap(buffer);

	return CEE_SUCCESS;
}

