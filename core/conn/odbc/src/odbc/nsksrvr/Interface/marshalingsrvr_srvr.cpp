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
#include "odbc_sv.h"
#include "Transport.h"
#include "marshaling.h"
#include "Global.h"
#include "CSrvrStmt.h"
#include "srvrcommon.h"


extern void logError( short Code, short Severity, short Operation );
extern char errStrBuf1[], errStrBuf2[], errStrBuf3[], errStrBuf4[], errStrBuf5[];

//================== Marshaling ==============================

CEE_status
odbc_SQLSvc_InitializeDialogue_param_res_(
		  CInterface* pnode
		, IDL_char*& buffer
		, IDL_unsigned_long& message_length
		, /* In    */ const struct odbc_SQLSvc_InitializeDialogue_exc_ *exception_
		, /* In    */ const OUT_CONNECTION_CONTEXT_def *outContext
)
{
	SRVRTRACE_ENTER(FILE_OMR+1);

	IDL_char* curptr;
	IDL_long wlength;
	IDL_long exceptionLength = 0;
	IDL_long computerNameLength = 0;
	IDL_long catalogLength = 0;
	IDL_long schemaLength = 0;

	VERSION_def version[4];
	VERSION_def* versionPtr = &version[0];


	wlength = sizeof(HEADER);

//
// calculate length of the buffer for each parameter
//
	//
	// length of odbc_SQLSvc_InitializeDialogue_exc_ *exception_
	//
	wlength += sizeof(exception_->exception_nr);
	wlength += sizeof(exception_->exception_detail);

	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START
		case odbc_SQLSvc_InitializeDialogue_ParamError_exn_:
           wlength += sizeof(exceptionLength);
           if (exception_->u.ParamError.ParamDesc != NULL)
		   {
		      exceptionLength = strlen(exception_->u.ParamError.ParamDesc) + 1;
		      wlength += exceptionLength;
		   }
		   break;
		case odbc_SQLSvc_InitializeDialogue_InvalidConnection_exn_:
		   break;
		case odbc_SQLSvc_InitializeDialogue_SQLError_exn_:
			ERROR_DESC_LIST_length( (ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, wlength);
		   break;
		case odbc_SQLSvc_InitializeDialogue_SQLInvalidHandle_exn_:
		   break;
		case odbc_SQLSvc_InitializeDialogue_SQLNeedData_exn_:
		   break;
		case odbc_SQLSvc_InitializeDialogue_InvalidUser_exn_:
           ERROR_DESC_LIST_length( (ERROR_DESC_LIST_def *)&exception_->u.InvalidUser.errorList, wlength);
		   break;
		default:
		   break;
//LCOV_EXCL_STOP
	}


	//
	// length of OUT_CONNECTION_CONTEXT_def
	//

	// VERSION_LIST_def clientVersionList;
	// length of IDL_long versionListlength
	wlength += sizeof(outContext->versionList._length);

	// Get the versionPtr
	versionPtr = outContext->versionList._buffer;

	for (int i = 0; i < outContext->versionList._length; i++)
	{

		// length of componentId
		wlength += sizeof(versionPtr->componentId);

		// length of majorVersion
		wlength += sizeof(versionPtr->majorVersion);

		// length of minorVersion
		wlength += sizeof(versionPtr->minorVersion);

		// length of buildId
		wlength += sizeof(versionPtr->buildId);

		// Get the next versionlist values
		versionPtr++;
	}

	// get sizeof of nodeId
	wlength += sizeof(outContext->nodeId);

	// get sizeof processId
	wlength += sizeof(outContext->processId);
	//

	//  length of SQL_IDENTIFIER_DEF computerName
	if (outContext->computerName[0] !=  '\0')
	{
	    wlength += sizeof(computerNameLength);
		computerNameLength = strlen(outContext->computerName) + 1;
	    wlength += computerNameLength;
	}
  	else
	{
		wlength += sizeof(computerNameLength);
		computerNameLength = 0;
	}

	//  length of SQL_IDENTIFIER_DEF catalog
	if (outContext->catalog[0] !=  '\0')
	{
	    wlength += sizeof(catalogLength);
		catalogLength = strlen(outContext->catalog) + 1;
	    wlength += catalogLength;
	}
  	else
	{
		wlength += sizeof(catalogLength);
		catalogLength = 0;
	}

	//  length of SQL_IDENTIFIER_DEF schema
	if (outContext->schema[0] !=  '\0')
	{
	    wlength += sizeof(schemaLength);
		schemaLength = strlen(outContext->schema) + 1;
	    wlength += schemaLength;
	}
  	else
	{
		wlength += sizeof(schemaLength);
		schemaLength = 0;
	}

	wlength += sizeof(outContext->outContextOptions1);
	wlength += sizeof(outContext->outContextOptions2);

	if (outContext->outContextOptions1 & OUTCONTEXT_OPT1_ROLENAME || outContext->outContextOptions1 & OUTCONTEXT_OPT1_DOWNLOAD_CERTIFICATE)
	{
		wlength += sizeof(outContext->outContextOptionStringLen);
		wlength += outContext->outContextOptionStringLen;
	}

	// message_length
	message_length = wlength;
	if ((buffer = pnode->w_allocate(message_length)) == NULL)
	{
		return CEE_ALLOCFAIL;
	}


	curptr = buffer + sizeof(HEADER);

	//
	// copy odbc_SQLSvc_InitializeDialogue_exc_ *exception_
	//
	IDL_long_copy((IDL_long *)&exception_->exception_nr, curptr);
	IDL_long_copy((IDL_long *)&exception_->exception_detail, curptr);

	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START
		case odbc_SQLSvc_InitializeDialogue_ParamError_exn_:
		    IDL_long_copy(&exceptionLength, curptr);
	        if (exception_->u.ParamError.ParamDesc != NULL)
			   IDL_charArray_copy((const IDL_char *)exception_->u.ParamError.ParamDesc, curptr);
			break;
		case odbc_SQLSvc_InitializeDialogue_InvalidConnection_exn_:
			break;
		case odbc_SQLSvc_InitializeDialogue_SQLError_exn_:
			ERROR_DESC_LIST_copy( (ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, curptr);
			break;
		case odbc_SQLSvc_InitializeDialogue_SQLInvalidHandle_exn_:
			break;
		case odbc_SQLSvc_InitializeDialogue_SQLNeedData_exn_:
			break;
		case odbc_SQLSvc_InitializeDialogue_InvalidUser_exn_:
			ERROR_DESC_LIST_copy( (ERROR_DESC_LIST_def *)&exception_->u.InvalidUser.errorList, curptr);
			break;
		default:
			break;
//LCOV_EXCL_STOP
	}

	//
	// copy OUT_CONNECTION_CONTEXT
	//

  	// copy VERSION_LIST_def *versionList
	// Get the versionPtr
	versionPtr = outContext->versionList._buffer;

	// copy version length
	IDL_unsigned_long_copy((unsigned int *)&outContext->versionList._length, curptr);

	for (int i = 0; i < outContext->versionList._length; i++)
	{
		// copy componentId
		IDL_short_copy(&versionPtr->componentId, curptr);

		// copy majorVersion
		IDL_short_copy(&versionPtr->majorVersion, curptr);

		// copy minorVersion
		IDL_short_copy(&versionPtr->minorVersion, curptr);

		// copy buildId
		IDL_unsigned_long_copy(&versionPtr->buildId, curptr);

		// Get the next versionlist values
		versionPtr++;
	}


	// copy nodeid
	IDL_short_copy((IDL_short *) &outContext->nodeId, curptr);

	// copy processId
	IDL_unsigned_long_copy((IDL_unsigned_long *)&outContext->processId, curptr);

	//copy computerNameLength
	IDL_long_copy(&computerNameLength, curptr);
	// copy computerName
    if (outContext->computerName[0] !=  '\0')
	{
		IDL_charArray_copy(outContext->computerName, curptr);
	}

	// copy catalog length
	IDL_long_copy(&catalogLength, curptr);
	// copy catalog
    if (outContext->catalog[0] !=  '\0')
	{
		IDL_charArray_copy(outContext->catalog, curptr);
	}

	// copy schema length
	IDL_long_copy(&schemaLength, curptr);
	// copy catalog
    if (outContext->schema[0] !=  '\0')
	{
		IDL_charArray_copy(outContext->schema, curptr);
	}

	IDL_unsigned_long_copy((IDL_unsigned_long *)&outContext->outContextOptions1, curptr);
	IDL_unsigned_long_copy((IDL_unsigned_long *)&outContext->outContextOptions2, curptr);

//#ifdef _TMP_SQ_SECURITY
	if (outContext->outContextOptions1 & OUTCONTEXT_OPT1_ROLENAME || outContext->outContextOptions1 & OUTCONTEXT_OPT1_DOWNLOAD_CERTIFICATE)
	{
		IDL_unsigned_long_copy((IDL_unsigned_long *)&outContext->outContextOptionStringLen, curptr);
		if (outContext->outContextOptionStringLen > 0)
		{
			IDL_byteArray_copy((BYTE *)(outContext->outContextOptionString), outContext->outContextOptionStringLen, curptr);
		}
	}
//#else
//	if (outContext->outContextOptions1 & OUTCONTEXT_OPT1_ROLENAME)
//	{
//		IDL_unsigned_long_copy((IDL_unsigned_long *)&outContext->outContextOptionStringLen, curptr);
//		if (outContext->outContextOptionStringLen > 0)
//		{
//			IDL_charArray_copy(outContext->outContextOptionString, curptr);
//		}
//	}
//#endif

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingas_srvr.cpp");
		strcpy( errStrBuf3, "AS-odbcas_ASSvc_GetObjRefHdl_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}


	SRVRTRACE_EXIT(FILE_OMR+1);
	return CEE_SUCCESS;

} // odbc_SQLSvc_InitializeDialogue_param_res_()

CEE_status
odbc_SQLSvc_TerminateDialogue_param_res_(
		  CInterface* pnode
		, IDL_char*& buffer
		, IDL_unsigned_long& message_length
		, /* In    */ const odbc_SQLSvc_TerminateDialogue_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OMR+2);

	IDL_char* curptr;
	IDL_long wlength;
	IDL_long exceptionLength = 0;

	wlength = sizeof(HEADER);

//
// calculate length of the buffer for each parameter
//
	//
	// length of odbc_SQLSvc_TerminateDialogue_exc_ *exception_
	//
	wlength += sizeof(exception_->exception_nr);
	wlength += sizeof(exception_->exception_detail);

	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START
		case odbc_SQLSvc_TerminateDialogue_ParamError_exn_:
           wlength += sizeof(exceptionLength);
           if (exception_->u.ParamError.ParamDesc != NULL)
		   {
		      exceptionLength = strlen(exception_->u.ParamError.ParamDesc) + 1;
		      wlength += exceptionLength;
		   }
		   break;
		case odbc_SQLSvc_TerminateDialogue_InvalidConnection_exn_:
			break;
		case odbc_SQLSvc_TerminateDialogue_SQLError_exn_:
			ERROR_DESC_LIST_length( (ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, wlength);
			break;
		default:
			break;
//LCOV_EXCL_STOP
	}


	// message_length
	message_length = wlength;
	if ((buffer = pnode->w_allocate(message_length)) == NULL)
	{
		return CEE_ALLOCFAIL;
	}


	curptr = buffer + sizeof(HEADER);

	//
	// copy odbc_SQLSvc_TerminateDialogue_exc_ *exception_
	//
	IDL_long_copy((IDL_long *)&exception_->exception_nr, curptr);
	IDL_long_copy((IDL_long *)&exception_->exception_detail, curptr);

	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START
		case odbc_SQLSvc_TerminateDialogue_ParamError_exn_:
		    IDL_long_copy(&exceptionLength, curptr);
	        if (exception_->u.ParamError.ParamDesc != NULL)
			   IDL_charArray_copy((const IDL_char *)exception_->u.ParamError.ParamDesc, curptr);
			break;
		case odbc_SQLSvc_TerminateDialogue_InvalidConnection_exn_:
			break;
		case odbc_SQLSvc_TerminateDialogue_SQLError_exn_:
			ERROR_DESC_LIST_copy( (ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, curptr);
			break;
		default:
			break;
//LCOV_EXCL_STOP
	}

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingas_srvr.cpp");
		strcpy( errStrBuf3, "AS-odbcas_ASSvc_GetObjRefHdl_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

	SRVRTRACE_EXIT(FILE_OMR+2);
	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSrvr_Close_param_res_(
		  CInterface* pnode
		, IDL_char*& buffer
		, IDL_unsigned_long& message_length
		, /* In    */ IDL_long returnCode
		, /* In    */ IDL_long sqlWarningOrErrorLength
		, /* In    */ BYTE *sqlWarningOrError
		, /* In    */ IDL_long rowsAffected
)
{
	SRVRTRACE_ENTER(FILE_OMR+12);
	IDL_char *curptr;

	IDL_long wlength;

	wlength = sizeof(HEADER);

//
// calculate length of the buffer for each parameter
//
// length of IDL_long returnCode
//
	wlength += sizeof(returnCode);

//
// length of IDL_long sqlWarningOrErrorLength
// length of BYTE *sqlWarningOrError
//
	if (sqlWarningOrError != NULL)
	{
		wlength += sizeof (sqlWarningOrErrorLength);
		wlength += sqlWarningOrErrorLength;
	}

//
// length of IDL_long rowsAffected
//
	wlength += sizeof(rowsAffected);

//
// message_length = header + param + maplength + data length
//
	message_length = wlength;
	buffer = pnode->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	curptr = buffer + sizeof(HEADER);

//
// copy of IDL_long returnCode
//
	IDL_long_copy(&returnCode, curptr);

//
// copy IDL_long sqlWarningOrErrorLength
// copy BYTE *sqlWarningOrError
//
	if (sqlWarningOrError != NULL)
	{
		IDL_long_copy(&sqlWarningOrErrorLength, curptr);
		IDL_byteArray_copy(sqlWarningOrError, sqlWarningOrErrorLength, curptr);
	}

//
// copy of IDL_long rowsAffected
//
	IDL_long_copy(&rowsAffected, curptr);

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbc_SQLSrvr_Close_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

	SRVRTRACE_EXIT(FILE_OMR+12);
	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_StopServer_param_res_(
		  CInterface* pnode
		, char*& buffer
		, UInt32& message_length
		, /* In    */ const struct odbc_SQLSvc_StopServer_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OMR+16);
	long* parptr;
	long* mapptr;
	char* curptr;
	char* pbuffer;

	long wlength;
	long maplength;

	short number_of_param = StopServer_out_params;

	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);
//
// calculate length of the buffer for each parameter
//
//
// length of odbc_SQLSvc_StopServer_exc_ *exception_
//
	wlength += sizeof(odbc_SQLSvc_StopServer_exc_);
	switch(exception_->exception_nr)
	{
	case odbc_SQLSvc_StopServer_ParamError_exn_:
		STRING_length( exception_->u.ParamError.ParamDesc,  wlength, maplength);
		break;
	case odbc_SQLSvc_StopServer_ProcessStopError_exn_:
		STRING_length( exception_->u.ProcessStopError.ErrorText, wlength, maplength);
		break;
	default:
		break;
	}
//
// message_length = header + param + maplength + data length
//
	message_length = maplength + wlength;
	buffer = pnode->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	pbuffer = buffer + sizeof(HEADER);
	parptr = (long*)pbuffer;
	mapptr = parptr + number_of_param;
	curptr = (char*)parptr + maplength;
//
// copy odbc_SQLSvc_StopServer_exc_ *exception_
//
	odbc_SQLSvc_StopServer_exc_* par1ptr = (odbc_SQLSvc_StopServer_exc_ *)curptr;
	memcpy(curptr, exception_, sizeof(odbc_SQLSvc_StopServer_exc_));
	curptr += sizeof (odbc_SQLSvc_StopServer_exc_);
	switch(exception_->exception_nr)
	{
	case odbc_SQLSvc_StopServer_ParamError_exn_:
		STRING_copy( pbuffer, exception_->u.ParamError.ParamDesc, &par1ptr->u.ParamError.ParamDesc, curptr, mapptr);
		break;
	case odbc_SQLSvc_StopServer_ProcessStopError_exn_:
		STRING_copy( pbuffer, exception_->u.ProcessStopError.ErrorText, &par1ptr->u.ParamError.ParamDesc, curptr, mapptr);
		break;
	default:
		break;
	}

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbc_SQLSvc_StopServer_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_START
	}
//
// set end of map list
//
	*(mapptr) = 0;
//
// save relative positions of all parameters
//
	*(parptr++) = (long)par1ptr - (long)pbuffer;

	if (pnode->swap() == SWAP_YES)
		pnode->process_swap(pbuffer);

	SRVRTRACE_EXIT(FILE_OMR+16);
	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_EnableServerTrace_param_res_(
		  CInterface* pnode
		, char*& buffer
		, UInt32& message_length
		, /* In    */ const struct odbc_SQLSvc_EnableServerTrace_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OMR+17);
	long* parptr;
	long* mapptr;
	char* curptr;
	char* pbuffer;

	long wlength;
	long maplength;

	short number_of_param = EnableServerTrace_out_params;

	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);
//
// calculate length of the buffer for each parameter
//
//
// length of odbc_SQLSvc_EnableServerTrace_exc_ *exception_
//
	wlength += sizeof(odbc_SQLSvc_EnableServerTrace_exc_);
	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START
	case odbc_SQLSvc_EnableServerTrace_ParamError_exn_:
		STRING_length( exception_->u.ParamError.ParamDesc,  wlength, maplength);
		break;
	case odbc_SQLSvc_EnableServerTrace_TraceError_exn_:
	case odbc_SQLSvc_EnableServerTrace_TraceAlreadyEnabled_exn_:
		break;
	default:
		break;
//LCOV_EXCL_STOP
	}
//
// message_length = header + param + maplength + data length
//
	message_length = maplength + wlength;
	buffer = pnode->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	pbuffer = buffer + sizeof(HEADER);
	parptr = (long*)pbuffer;
	mapptr = parptr + number_of_param;
	curptr = (char*)parptr + maplength;
//
// copy odbc_SQLSvc_EnableServerTrace_exc_ *exception_
//
	odbc_SQLSvc_EnableServerTrace_exc_* par1ptr = (odbc_SQLSvc_EnableServerTrace_exc_ *)curptr;
	memcpy(curptr, exception_, sizeof(odbc_SQLSvc_EnableServerTrace_exc_));
	curptr += sizeof (odbc_SQLSvc_EnableServerTrace_exc_);
	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START
	case odbc_SQLSvc_EnableServerTrace_ParamError_exn_:
		STRING_copy( pbuffer, exception_->u.ParamError.ParamDesc, &par1ptr->u.ParamError.ParamDesc, curptr, mapptr);
		break;
	case odbc_SQLSvc_EnableServerTrace_TraceError_exn_:
	case odbc_SQLSvc_EnableServerTrace_TraceAlreadyEnabled_exn_:
		break;
	default:
		break;
//LCOV_EXCL_STOP
	}

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbc_SQLSvc_EnableServerTrace_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}
//
// set end of map list
//
	*(mapptr) = 0;
//
// save relative positions of all parameters
//
	*(parptr++) = (long)par1ptr - (long)pbuffer;

	if (pnode->swap() == SWAP_YES)
		pnode->process_swap(pbuffer);

	SRVRTRACE_EXIT(FILE_OMR+17);
	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_DisableServerTrace_param_res_(
		  CInterface* pnode
		, char*& buffer
		, UInt32& message_length
		, /* In    */ const struct odbc_SQLSvc_DisableServerTrace_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OMR+18);
	long* parptr;
	long* mapptr;
	char* curptr;
	char* pbuffer;

	long wlength;
	long maplength;

	short number_of_param = DisableServerTrace_out_params;

	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);
//
// calculate length of the buffer for each parameter
//
//
// length of odbc_SQLSvc_DisableServerTrace_exc_ *exception_
//
	wlength += sizeof(odbc_SQLSvc_DisableServerTrace_exc_);
	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START
	case odbc_SQLSvc_DisableServerTrace_ParamError_exn_:
		STRING_length( exception_->u.ParamError.ParamDesc,  wlength, maplength);
		break;
	case odbc_SQLSvc_DisableServerTrace_TraceError_exn_:
	case odbc_SQLSvc_DisableServerTrace_TraceAlreadyDisabled_exn_:
		break;
	default:
		break;
//LCOV_EXCL_STOP
	}
//
// message_length = header + param + maplength + data length
//
	message_length = maplength + wlength;
	buffer = pnode->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	pbuffer = buffer + sizeof(HEADER);
	parptr = (long*)pbuffer;
	mapptr = parptr + number_of_param;
	curptr = (char*)parptr + maplength;
//
// copy odbc_SQLSvc_DisableServerTrace_exc_ *exception_
//
	odbc_SQLSvc_DisableServerTrace_exc_* par1ptr = (odbc_SQLSvc_DisableServerTrace_exc_ *)curptr;
	memcpy(curptr, exception_, sizeof(odbc_SQLSvc_DisableServerTrace_exc_));
	curptr += sizeof (odbc_SQLSvc_DisableServerTrace_exc_);
	switch(exception_->exception_nr)
	{
	case odbc_SQLSvc_DisableServerTrace_ParamError_exn_:
		STRING_copy( pbuffer, exception_->u.ParamError.ParamDesc, &par1ptr->u.ParamError.ParamDesc, curptr, mapptr);
		break;
	case odbc_SQLSvc_DisableServerTrace_TraceError_exn_:
	case odbc_SQLSvc_DisableServerTrace_TraceAlreadyDisabled_exn_:
		break;
	default:
		break;
	}

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbc_SQLSvc_DisableServerTrace_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}
//
// set end of map list
//
	*(mapptr) = 0;
//
// save relative positions of all parameters
//
	*(parptr++) = (long)par1ptr - (long)pbuffer;

	if (pnode->swap() == SWAP_YES)
		pnode->process_swap(pbuffer);

	SRVRTRACE_EXIT(FILE_OMR+18);
	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_EnableServerStatistics_param_res_(
		  CInterface* pnode
		, char*& buffer
		, UInt32& message_length
		, /* In    */ const struct odbc_SQLSvc_EnableServerStatistics_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OMR+19);
	long* parptr;
	long* mapptr;
	char* curptr;
	char* pbuffer;

	long wlength;
	long maplength;

	short number_of_param = EnableServerStatistics_out_params;

	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);
//
// calculate length of the buffer for each parameter
//
//
// length of odbc_SQLSvc_EnableServerStatistics_exc_ *exception_
//
	wlength += sizeof(odbc_SQLSvc_EnableServerStatistics_exc_);
	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START
	case odbc_SQLSvc_EnableServerStatistics_ParamError_exn_:
		STRING_length( exception_->u.ParamError.ParamDesc,  wlength, maplength);
		break;
	case odbc_SQLSvc_EnableServerStatistics_StatisticsError_exn_:
	case odbc_SQLSvc_EnableServerStatistics_StatisticsAlreadyEnabled_exn_:
		break;
	default:
		break;
//LCOV_EXCL_STOP
	}
//
// message_length = header + param + maplength + data length
//
	message_length = maplength + wlength;
	buffer = pnode->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	pbuffer = buffer + sizeof(HEADER);
	parptr = (long*)pbuffer;
	mapptr = parptr + number_of_param;
	curptr = (char*)parptr + maplength;
//
// copy odbc_SQLSvc_EnableServerStatistics_exc_ *exception_
//
	odbc_SQLSvc_EnableServerStatistics_exc_* par1ptr = (odbc_SQLSvc_EnableServerStatistics_exc_ *)curptr;
	memcpy(curptr, exception_, sizeof(odbc_SQLSvc_EnableServerStatistics_exc_));
	curptr += sizeof (odbc_SQLSvc_EnableServerStatistics_exc_);
	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START
	case odbc_SQLSvc_EnableServerStatistics_ParamError_exn_:
		STRING_copy( pbuffer, exception_->u.ParamError.ParamDesc, &par1ptr->u.ParamError.ParamDesc, curptr, mapptr);
		break;
	case odbc_SQLSvc_EnableServerStatistics_StatisticsError_exn_:
	case odbc_SQLSvc_EnableServerStatistics_StatisticsAlreadyEnabled_exn_:
		break;
	default:
		break;
//LCOV_EXCL_STOP
	}

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbc_SQLSvc_EnableServerStatistics_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}
//
// set end of map list
//
	*(mapptr) = 0;
//
// save relative positions of all parameters
//
	*(parptr++) = (long)par1ptr - (long)pbuffer;

	if (pnode->swap() == SWAP_YES)
		pnode->process_swap(pbuffer);

	SRVRTRACE_EXIT(FILE_OMR+19);
	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_DisableServerStatistics_param_res_(
		  CInterface* pnode
		, char*& buffer
		, UInt32& message_length
		, /* In    */ const struct odbc_SQLSvc_DisableServerStatistics_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OMR+20);
	long* parptr;
	long* mapptr;
	char* curptr;
	char* pbuffer;

	long wlength;
	long maplength;

	short number_of_param = DisableServerStatistics_out_params;

	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);
//
// calculate length of the buffer for each parameter
//
//
// length of odbc_SQLSvc_DisableServerStatistics_exc_ *exception_
//
	wlength += sizeof(odbc_SQLSvc_DisableServerStatistics_exc_);
	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START
	case odbc_SQLSvc_DisableServerStatistics_ParamError_exn_:
		STRING_length( exception_->u.ParamError.ParamDesc,  wlength, maplength);
		break;
	case odbc_SQLSvc_DisableServerStatistics_StatisticsError_exn_:
	case odbc_SQLSvc_DisableServerStatistics_StatisticsAlreadyDisabled_exn_:
		break;
	default:
		break;
//LCOV_EXCL_STOP
	}
//
// message_length = header + param + maplength + data length
//
	message_length = maplength + wlength;
	buffer = pnode->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	pbuffer = buffer + sizeof(HEADER);
	parptr = (long*)pbuffer;
	mapptr = parptr + number_of_param;
	curptr = (char*)parptr + maplength;
//
// copy odbc_SQLSvc_DisableServerStatistics_exc_ *exception_
//
	odbc_SQLSvc_DisableServerStatistics_exc_* par1ptr = (odbc_SQLSvc_DisableServerStatistics_exc_ *)curptr;
	memcpy(curptr, exception_, sizeof(odbc_SQLSvc_DisableServerStatistics_exc_));
	curptr += sizeof (odbc_SQLSvc_DisableServerStatistics_exc_);
	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START
	case odbc_SQLSvc_DisableServerStatistics_ParamError_exn_:
		STRING_copy( pbuffer, exception_->u.ParamError.ParamDesc, &par1ptr->u.ParamError.ParamDesc, curptr, mapptr);
		break;
	case odbc_SQLSvc_DisableServerStatistics_StatisticsError_exn_:
	case odbc_SQLSvc_DisableServerStatistics_StatisticsAlreadyDisabled_exn_:
		break;
	default:
		break;
//LCOV_EXCL_STOP
	}

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbc_SQLSvc_DisableServerStatistics_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}
//
// set end of map list
//
	*(mapptr) = 0;
//
// save relative positions of all parameters
//
	*(parptr++) = (long)par1ptr - (long)pbuffer;

	if (pnode->swap() == SWAP_YES)
		pnode->process_swap(pbuffer);

	SRVRTRACE_EXIT(FILE_OMR+20);
	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_UpdateServerContext_param_res_(
		  CInterface* pnode
		, char*& buffer
		, UInt32& message_length
		, /* In    */ const struct odbc_SQLSvc_UpdateServerContext_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OMR+21);
	long* parptr;
	long* mapptr;
	char* curptr;
	char* pbuffer;

	long wlength;
	long maplength;

	short number_of_param = UpdateServerContext_out_params;

	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);
//
// calculate length of the buffer for each parameter
//
//
// length of odbc_SQLSvc_UpdateServerContext_exc_ *exception_
//
	wlength += sizeof(odbc_SQLSvc_UpdateServerContext_exc_);
	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START

	case odbc_SQLSvc_UpdateServerContext_ParamError_exn_:
		STRING_length( exception_->u.ParamError.ParamDesc,  wlength, maplength);
		break;
	case odbc_SQLSvc_UpdateServerContext_SQLError_exn_:
		ERROR_DESC_LIST_length( (ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, wlength, maplength);
		break;
	default:
		break;
//LCOV_EXCL_STOP
	}
//
// message_length = header + param + maplength + data length
//
	message_length = maplength + wlength;
	buffer = pnode->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	pbuffer = buffer + sizeof(HEADER);
	parptr = (long*)pbuffer;
	mapptr = parptr + number_of_param;
	curptr = (char*)parptr + maplength;
//
// copy odbc_SQLSvc_UpdateServerContext_exc_ *exception_
//
	odbc_SQLSvc_UpdateServerContext_exc_* par1ptr = (odbc_SQLSvc_UpdateServerContext_exc_ *)curptr;
	memcpy(curptr, exception_, sizeof(odbc_SQLSvc_UpdateServerContext_exc_));
	curptr += sizeof (odbc_SQLSvc_UpdateServerContext_exc_);
	switch(exception_->exception_nr)
	{
//LCOV_EXCL_START
	case odbc_SQLSvc_UpdateServerContext_ParamError_exn_:
		STRING_copy( pbuffer, exception_->u.ParamError.ParamDesc, &par1ptr->u.ParamError.ParamDesc, curptr, mapptr);
		break;
	case odbc_SQLSvc_UpdateServerContext_SQLError_exn_:
		ERROR_DESC_LIST_copy( pbuffer, (ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, &par1ptr->u.SQLError.errorList, curptr, mapptr);
		break;
	default:
		break;
//LCOV_EXCL_STOP
	}

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbc_SQLSvc_UpdateServerContext_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}
//
// set end of map list
//
	*(mapptr) = 0;
//
// save relative positions of all parameters
//
	*(parptr++) = (long)par1ptr - (long)pbuffer;

	if (pnode->swap() == SWAP_YES)
		pnode->process_swap(pbuffer);

	SRVRTRACE_EXIT(FILE_OMR+21);
	return CEE_SUCCESS;
}

CEE_status
odbc_SQLSvc_MonitorCall_param_res_(
		  CInterface* pnode
		, char*& buffer
		, UInt32& message_length
		, /* In    */ const struct odbc_SQLSvc_MonitorCall_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OMR+22);
	long* parptr;
	long* mapptr;
	char* curptr;
	char* pbuffer;

	long wlength;
	long maplength;

	short number_of_param = MonitorCall_out_params;

	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);
//
// calculate length of the buffer for each parameter
//
//
// length of odbc_SQLSvc_MonitorCall_exc_ *exception_
//
	wlength += sizeof(odbc_SQLSvc_MonitorCall_exc_);
	switch(exception_->exception_nr)
	{
	default:
		break;
	}
//
// message_length = header + param + maplength + data length
//
	message_length = maplength + wlength;
	buffer = pnode->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	pbuffer = buffer + sizeof(HEADER);
	parptr = (long*)pbuffer;
	mapptr = parptr + number_of_param;
	curptr = (char*)parptr + maplength;
//
// copy odbc_SQLSvc_MonitorCall_exc_ *exception_
//
	odbc_SQLSvc_MonitorCall_exc_* par1ptr = (odbc_SQLSvc_MonitorCall_exc_ *)curptr;
	memcpy(curptr, exception_, sizeof(odbc_SQLSvc_MonitorCall_exc_));
	curptr += sizeof (odbc_SQLSvc_MonitorCall_exc_);
	switch(exception_->exception_nr)
	{
	default:
		break;
	}

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbc_SQLSvc_MonitorCall_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}
//
// set end of map list
//
	*(mapptr) = 0;
//
// save relative positions of all parameters
//
	*(parptr++) = (long)par1ptr - (long)pbuffer;

	if (pnode->swap() == SWAP_YES)
		pnode->process_swap(pbuffer);

	SRVRTRACE_EXIT(FILE_OMR+22);
	return CEE_SUCCESS;
}


CEE_status
odbc_SQLSrvr_Prepare_param_res_(
		  CInterface* pnode
		, IDL_char*& buffer
		, IDL_unsigned_long& message_length
		, /* In   */ IDL_long returnCode
		, /* In   */ IDL_long sqlWarningOrErrorLength
		, /* In   */ BYTE *sqlWarningOrError
		, /* In   */ IDL_long sqlQueryType
		, /* In   */ IDL_long stmtHandleKey
		, /* In   */ IDL_long estimatedCost
		, /* In   */ IDL_long inputDescLength
		, /* In   */ BYTE *inputDesc
		, /* In   */ IDL_long outputDescLength
		, /* In   */ BYTE *outputDesc
)
{
	SRVRTRACE_ENTER(FILE_OMR+15);

	IDL_char* curptr;
	IDL_long wlength;

	wlength = sizeof(HEADER);

//
// calculate length of the buffer for each parameter
//
// length of IDL_long returnCode
//
	wlength += sizeof(returnCode);
//
// length of IDL_long sqlWarningOrErrorLength
// length of BYTE *sqlWarningOrError
//
	if (sqlWarningOrError != NULL)
	{
		wlength += sizeof(sqlWarningOrErrorLength);
		wlength += sqlWarningOrErrorLength;
	}
//
// length of IDL_long sqlQueryType
//
	wlength += sizeof(sqlQueryType);
//
// length of IDL_long stmtHandleKey
//
	wlength += sizeof(stmtHandleKey);
//
// length of IDL_long estimatedCost
//
	wlength += sizeof(estimatedCost);
//
// length of IDL_long inputDescLength
// length of BYTE *inputDesc
//
	if (inputDesc != NULL)
	{
		wlength += sizeof(inputDescLength);
		wlength += inputDescLength;
	}
	else
		wlength += sizeof(inputDescLength);
//
// length of IDL_long outputDescLength
// length of BYTE *outputDesc
//
	if (outputDesc != NULL)
	{
		wlength += sizeof(outputDescLength);
		wlength += outputDescLength;
	}
	else
		wlength += sizeof(outputDescLength);

//
// message_length = header + param + maplength + data length
//
	message_length = wlength;
	buffer = pnode->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	curptr = (IDL_char*)(buffer + sizeof(HEADER));
//
// copy of IDL_long returnCode
//
	IDL_long_copy(&returnCode, curptr);
//
// copy IDL_long sqlWarningOrErrorLength
// copy BYTE *sqlWarningOrError
//
	if (sqlWarningOrError != NULL)
	{
		IDL_long_copy(&sqlWarningOrErrorLength, curptr);
		IDL_byteArray_copy(sqlWarningOrError, sqlWarningOrErrorLength, curptr);
	}
//
// copy of IDL_long sqlQueryType
//
	IDL_long_copy(&sqlQueryType, curptr);
//
// copy of IDL_long stmtHandle
//
	IDL_long_copy(&stmtHandleKey, curptr);
//
// copy of IDL_long estimatedCost
//
	IDL_long_copy(&estimatedCost, curptr);
//
// copy IDL_long inputDescLength
// copy BYTE *inputDesc
//
	if (inputDesc != NULL)
	{
		IDL_long_copy(&inputDescLength, curptr);
		IDL_byteArray_copy(inputDesc, inputDescLength, curptr);
	}
	else
	{
		IDL_long_copy(&inputDescLength, curptr);
	}
//
// copy IDL_long outputDescLength
// copy BYTE *outputDesc
//
	if (outputDesc != NULL)
	{
		IDL_long_copy(&outputDescLength, curptr);
		IDL_byteArray_copy(outputDesc, outputDescLength, curptr);
	}
	else
	{
		IDL_long_copy(&outputDescLength, curptr);
	}

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbc_SQLSrvr_Prepare_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

	SRVRTRACE_EXIT(FILE_OMR+15);
	return CEE_SUCCESS;
} /* odbc_SQLSrvr_Prepare_param_res_() */

CEE_status
odbc_SQLSrvr_Fetch_param_res_(
		  CInterface* pnode
		, IDL_char*& buffer
		, UInt32& message_length
		, /* In    */ IDL_long returnCode
		, /* In    */ IDL_long sqlWarningOrErrorLength
		, /* In    */ BYTE *sqlWarningOrError
		, /* In    */ IDL_long rowsAffected
		, /* In    */ IDL_long outValuesFormat
		, /* In    */ IDL_long outValuesLength
		, /* In    */ BYTE *outValues
)
{
  SRVRTRACE_ENTER(FILE_OMR+15);

  IDL_char  *curptr;
  IDL_long   wlength;

  wlength = sizeof(HEADER);

//
// calculate length of the buffer for each parameter
//

// length of IDL_long returnCode
//
  wlength += sizeof(returnCode);

//
// length of IDL_long sqlWarningOrErrorLength
// length of BYTE *sqlWarningOrError
//
  if (sqlWarningOrError != NULL)
  {
     wlength += sizeof (sqlWarningOrErrorLength);
     wlength += sqlWarningOrErrorLength;
  }

//
// length of IDL_long rowsAffected
//
  wlength += sizeof(outValuesFormat);

//
// length of IDL_long rowsAffected
//
  wlength += sizeof(rowsAffected);

//
// length of IDL_long outValuesLength
// length of BYTE *outValues
//
  if (outValues != NULL)
  {
     wlength += sizeof (outValuesLength);
     wlength += outValuesLength;
  }
  else
     wlength += sizeof (outValuesLength);

//
// message_length = header + param + maplength + data length
//
  message_length = wlength;

  buffer = pnode->w_allocate(message_length);
  if (buffer == NULL)
      return CEE_ALLOCFAIL;


  curptr = (IDL_char*)(buffer + sizeof(HEADER));

//
// copy of IDL_long returnCode
//
  IDL_long_copy(&returnCode, curptr);

//
// copy IDL_long sqlWarningOrErrorLength
// copy BYTE *sqlWarningOrError
//
  if (sqlWarningOrError != NULL)
  {
     IDL_long_copy(&sqlWarningOrErrorLength, curptr);
     IDL_byteArray_copy(sqlWarningOrError, sqlWarningOrErrorLength, curptr);
  }

//
// copy of IDL_long rowsAffected
//
  IDL_long_copy(&rowsAffected, curptr);

//
// copy of IDL_long outValuesFormat
//
  IDL_long_copy(&outValuesFormat, curptr);

//
// copy IDL_long outValuesLength
// copy BYTE *outValues
//
  if (outValues != NULL)
  {
    IDL_long_copy(&outValuesLength, curptr);
    if (outValues != NULL)
	{
        IDL_byteArray_copy(outValues, outValuesLength, curptr);
	}

  }
  else
  {
     IDL_long_copy(&outValuesLength, curptr);
  }

  if (curptr > buffer + message_length)
    {
//LCOV_EXCL_START
    strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
    strcpy( errStrBuf3, "SRVR-odbc_SQLSrvr_Fetch_param_res_");
    strcpy( errStrBuf4, "buffer overflow");
    sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
    logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
    exit(1000);
//LCOV_EXCL_START
    }

  SRVRTRACE_EXIT(FILE_OMR+15);
  return CEE_SUCCESS;

}  // end odbc_SQLSrvr_Fetch_param_res_()


CEE_status
odbc_SQLSrvr_Execute_param_res_(
		  CInterface* pnode
		, IDL_char*& buffer
		, IDL_unsigned_long& message_length
		, /* In    */ IDL_long returnCode
		, /* In    */ IDL_long sqlWarningOrErrorLength
		, /* In    */ BYTE *sqlWarningOrError
		, /* In    */ IDL_long rowsAffected
        , /* In    */ IDL_long sqlQueryType     // Used by ExecDirect for unique selects
        , /* In    */ IDL_long estimatedCost
		, /* In    */ IDL_long outValuesLength
		, /* In    */ BYTE *outValues
        , /* In    */ IDL_long outputDescLength // Used to return the output descriptors for ExecDirect
        , /* In    */ BYTE *outputDesc          // Used to return the output descriptors for ExecDirect
        , /* In    */ Long stmtHandle       // Statement handle - needed to copy out SPJ result sets
        , /* In    */ IDL_long stmtHandleKey
)
{

  SRVRTRACE_ENTER(FILE_OMR+15);

  IDL_char  *curptr;
  IDL_long   wlength;

  SRVR_STMT_HDL *pSrvrStmt = (SRVR_STMT_HDL *)stmtHandle;
  SRVR_STMT_HDL *rsSrvrStmt;  // To iterate thru the result set
  Long      rsStmtHandle; // result set statement handle
  IDL_long      rsStmtLabelLength;
  IDL_char      *rsStmtName;
  IDL_long      rsOutputDescBufferLength;
  BYTE			*rsOutputDescBuffer;
  IDL_long      charSet = 1;  // KAS - SQLCHARSETCODE_ISO88591 - change this when supporting character sets
  IDL_long      numResultSets  = 0;
  IDL_long		proxySyntaxStringLen = 0;

  if(pSrvrStmt != NULL)
	  numResultSets = pSrvrStmt->numResultSets; // SPJ result sets

  wlength = sizeof(HEADER);

//
// calculate length of the buffer for each parameter
//

// length of IDL_long returnCode
//
  wlength += sizeof(returnCode);

//
// length of IDL_long sqlWarningOrErrorLength
// length of BYTE *sqlWarningOrError
//
  wlength += sizeof (sqlWarningOrErrorLength);
  if (sqlWarningOrError != NULL)
  {
     wlength += sqlWarningOrErrorLength;
  }


//
// length of IDL_long outputDescLength
// length of BYTE* outputDesc
//
  wlength += sizeof (outputDescLength);
  if(outputDescLength > 0)
	  wlength += outputDescLength;

//
// length of IDL_long rowsAffected
//
  wlength += sizeof(rowsAffected);

//
// length of IDL_long sqlQueryType
//
  wlength += sizeof(sqlQueryType);

//
// length of IDL_long estimatedCost
//
  wlength += sizeof(estimatedCost);

//
// length of IDL_long outValuesLength
// length of BYTE *outValues
//
  wlength += sizeof (outValuesLength);
  if (outValues != NULL)
  {
     wlength += outValuesLength;
  }

//
// length of SPJ numResultSets
//
  wlength += sizeof (numResultSets);

  if(numResultSets > 0)
  {
//
// length of result set information
//
		rsSrvrStmt = pSrvrStmt->nextSpjRs;

        for (int i = 0; i < numResultSets; i++)
        {
			rsStmtHandle      = (Long)rsSrvrStmt;
			rsStmtLabelLength = (IDL_long)rsSrvrStmt->stmtNameLen + 1;  // add 1 for null
			rsStmtName        = (IDL_char *)rsSrvrStmt->stmtName;
			rsOutputDescBufferLength = rsSrvrStmt->outputDescBufferLength;

			wlength += sizeof(stmtHandleKey);
			wlength += sizeof(rsStmtLabelLength);
			wlength += rsStmtLabelLength;
			wlength += sizeof(charSet);
			wlength += sizeof(rsOutputDescBufferLength);
			wlength += rsOutputDescBufferLength;
			wlength += sizeof(proxySyntaxStringLen);

			if(rsSrvrStmt->SpjProxySyntaxString != NULL)
				proxySyntaxStringLen = strlen(rsSrvrStmt->SpjProxySyntaxString);
			else
				proxySyntaxStringLen = 0;

			if(proxySyntaxStringLen > 0)
				wlength += proxySyntaxStringLen + 1; // null terminated string

			rsSrvrStmt = rsSrvrStmt->nextSpjRs;

		}  // end for

  } // if numResultSets > 0

  wlength += sizeof (proxySyntaxStringLen);

  if((pSrvrStmt != NULL) && (pSrvrStmt->SpjProxySyntaxString != NULL))
	proxySyntaxStringLen = strlen(pSrvrStmt->SpjProxySyntaxString);
  else
	proxySyntaxStringLen = 0;

  if(proxySyntaxStringLen > 0)
	wlength += proxySyntaxStringLen + 1; // null terminated string


//
// message_length = header + data length
//
  message_length = wlength;

  buffer = pnode->w_allocate(message_length);
  if (buffer == NULL)
      return CEE_ALLOCFAIL;


  curptr = (IDL_char*)(buffer + sizeof(HEADER));

//
// copy of IDL_long returnCode
//
  IDL_long_copy(&returnCode, curptr);

//
// copy IDL_long sqlWarningOrErrorLength
// copy BYTE *sqlWarningOrError
//
  IDL_long_copy(&sqlWarningOrErrorLength, curptr);
  if (sqlWarningOrError != NULL)
  {
     IDL_byteArray_copy(sqlWarningOrError, sqlWarningOrErrorLength, curptr);
  }


//
// copy of IDL_long outDescLength
// copy of BYTE* outDesc
//
  IDL_long_copy(&outputDescLength, curptr);

  if(outputDescLength > 0 && outputDesc != NULL)
	  IDL_byteArray_copy(outputDesc, outputDescLength, curptr);


//
// copy of IDL_long rowsAffected
//
  IDL_long_copy(&rowsAffected, curptr);

//
// copy of IDL_long sqlQueryType
//
  IDL_long_copy(&sqlQueryType, curptr);

//
// copy of IDL_long queryType
//
  IDL_long_copy(&estimatedCost, curptr);


//
// copy IDL_long outValuesLength
// copy BYTE *outValues
//
  IDL_long_copy(&outValuesLength, curptr);

  if (outValues != NULL)
     IDL_byteArray_copy(outValues, outValuesLength, curptr);


//
// copy of IDL_long numResultSets
//
  IDL_long_copy(&numResultSets, curptr);

  if(numResultSets > 0)
  {
//
// copy result set information
//
        rsSrvrStmt = pSrvrStmt->nextSpjRs;

        for (int i = 0; i < numResultSets; i++)
        {
			rsStmtHandle  = (Long)rsSrvrStmt;
			rsStmtLabelLength = (IDL_long)rsSrvrStmt->stmtNameLen + 1;  // add 1 for null
			rsStmtName        = (IDL_char *)rsSrvrStmt->stmtName;
            rsOutputDescBufferLength = rsSrvrStmt->outputDescBufferLength;
            rsOutputDescBuffer = rsSrvrStmt->outputDescBuffer;

			IDL_long_copy(&stmtHandleKey, curptr);
			IDL_long_copy(&rsStmtLabelLength, curptr);
			memcpy(curptr, rsStmtName, rsStmtLabelLength - 1);  // subtract 1 for the null
			curptr = curptr + (rsStmtLabelLength - 1);
			*curptr = '\0';
			curptr = curptr + 1;
			IDL_long_copy(&charSet, curptr);
			IDL_long_copy(&rsOutputDescBufferLength, curptr);
			memcpy(curptr, rsOutputDescBuffer, rsOutputDescBufferLength);
			curptr = curptr + rsOutputDescBufferLength;

			if(rsSrvrStmt->SpjProxySyntaxString != NULL)
				proxySyntaxStringLen = strlen(rsSrvrStmt->SpjProxySyntaxString);
			else
				proxySyntaxStringLen = 0;

			if(proxySyntaxStringLen > 0)
			{
				proxySyntaxStringLen = proxySyntaxStringLen + 1; // null terminated
				IDL_long_copy(&proxySyntaxStringLen, curptr);
				IDL_charArray_copy((const IDL_char *)rsSrvrStmt->SpjProxySyntaxString, curptr);
			}
			else
				IDL_long_copy(&proxySyntaxStringLen, curptr);


			rsSrvrStmt = rsSrvrStmt->nextSpjRs;
	  }  // end for

  } // if numResultSets > 0

  if((pSrvrStmt != NULL) && (pSrvrStmt->SpjProxySyntaxString != NULL))
	proxySyntaxStringLen = strlen(pSrvrStmt->SpjProxySyntaxString);
  else
	proxySyntaxStringLen = 0;

  if(proxySyntaxStringLen > 0)
  {

	proxySyntaxStringLen = proxySyntaxStringLen + 1; // null terminated
	IDL_long_copy(&proxySyntaxStringLen, curptr);
	IDL_charArray_copy((const IDL_char *)pSrvrStmt->SpjProxySyntaxString, curptr);
  }
  else
	IDL_long_copy(&proxySyntaxStringLen, curptr);

  if (curptr > buffer + message_length)
  {
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "odbc_SQLSrvr_Execute_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
  }


  SRVRTRACE_EXIT(FILE_OMR+15);
  return CEE_SUCCESS;

}  // end odbc_SQLSrvr_Execute_param_res_()

CEE_status
odbc_SQLSrvr_SetConnectionOption_param_res_(
		  CInterface* pnode
		, IDL_char*& buffer
		, IDL_unsigned_long& message_length
		, /* In    */ const struct odbc_SQLSvc_SetConnectionOption_exc_ *exception_
		, /* In    */ ERROR_DESC_LIST_def *sqlWarning
)
{
	SRVRTRACE_ENTER(FILE_OMR+3);

	IDL_char* curptr;
	IDL_long wlength;
	IDL_long exceptionLength;

	wlength = sizeof(HEADER);

//
// calculate length of the buffer for each parameter
//

//
// length of odbc_SQLSvc_SetConnectionOption_exc_ *exception_
//
	wlength += sizeof(exception_->exception_nr);
	wlength += sizeof(exception_->exception_detail);

	switch(exception_->exception_nr)
	{
		case odbc_SQLSvc_SetConnectionOption_ParamError_exn_:

 	       wlength += sizeof(exceptionLength);
	       if (exception_->u.ParamError.ParamDesc != NULL)
		   {
		      exceptionLength = strlen(exception_->u.ParamError.ParamDesc) + 1;
		      wlength += exceptionLength;
		   }
	       else
		   {
		      exceptionLength = 0;
		   }

		   break;

		case odbc_SQLSvc_SetConnectionOption_SQLError_exn_:
	       ERROR_DESC_LIST_length( (ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, wlength);
 		   break;

		case odbc_SQLSvc_SetConnectionOption_InvalidConnection_exn_:
		case odbc_SQLSvc_SetConnectionOption_SQLInvalidHandle_exn_:
			break;

		default:
			break;
	}

//
// length of ERROR_DESC_LIST_def *sqlWarning
//
//
	ERROR_DESC_LIST_LENGTH2(sqlWarning)

//
// message_length = header + data length
//
  message_length = wlength;

  buffer = pnode->w_allocate(message_length);
  if (buffer == NULL)
      return CEE_ALLOCFAIL;

  curptr = (IDL_char*)(buffer + sizeof(HEADER));

//
// copy odbc_SQLSvc_SetConnectionOption_exc_ *exception_
//
	IDL_long_copy((IDL_long *)&exception_->exception_nr, curptr);
	IDL_long_copy((IDL_long *)&exception_->exception_detail, curptr);

	switch(exception_->exception_nr)
	{
		case odbc_SQLSvc_SetConnectionOption_ParamError_exn_:
		   IDL_long_copy(&exceptionLength, curptr);
	        if (exception_->u.ParamError.ParamDesc != NULL)
			   IDL_charArray_copy((const IDL_char *)exception_->u.ParamError.ParamDesc, curptr);
			break;

		case odbc_SQLSvc_SetConnectionOption_SQLError_exn_:
			ERROR_DESC_LIST_copy( (ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, curptr);
			break;

		case odbc_SQLSvc_SetConnectionOption_InvalidConnection_exn_:
		case odbc_SQLSvc_SetConnectionOption_SQLInvalidHandle_exn_:
			break;

		default:
			break;
	}
//
// copy ERROR_DESC_LIST_def *sqlWarning
//
    ERROR_DESC_LIST_COPY2(sqlWarning, curptr);

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbc_SQLSrvr_SetConnectionOption_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

	SRVRTRACE_EXIT(FILE_OMR+3);
	return CEE_SUCCESS;

} // odbc_SQLSrvr_SetConnectionOption_param_res_()

CEE_status
odbc_SQLSrvr_GetSQLCatalogs_param_res_(
		  CInterface* pnode
		, IDL_char*& buffer
		, IDL_unsigned_long& message_length
		, /* In    */ const struct odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
		, /* In    */ const IDL_char *catStmtLabel
		, /* In    */ SQLItemDescList_def *outputDesc
		, /* In    */ ERROR_DESC_LIST_def *sqlWarning
		, /* In    */ SRVR_STMT_HDL *pSrvrStmt
)
{
	SRVRTRACE_ENTER(FILE_OMR+14);
	IDL_char* curptr;
	IDL_long wlength;
	IDL_long exceptionLength = 0;
	IDL_long catStmtLabelLength = 0;
	IDL_long proxySyntaxStringLen = 0;

	wlength = sizeof(HEADER);

//
// calculate length of the buffer for each parameter
//

//
// length of odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
//
	wlength += sizeof(exception_->exception_nr);
	wlength += sizeof(exception_->exception_detail);

	switch(exception_->exception_nr)
	{
		case odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_:
 	       wlength += sizeof(exceptionLength);
	       if (exception_->u.ParamError.ParamDesc != NULL)
		   {
		      exceptionLength = strlen(exception_->u.ParamError.ParamDesc) + 1;
		      wlength += exceptionLength;
		   }
		   break;

		case odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_:
			ERROR_DESC_LIST_length( (ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, wlength);
			break;

		case odbc_SQLSvc_GetSQLCatalogs_InvalidConnection_exn_:
		case odbc_SQLSvc_GetSQLCatalogs_SQLInvalidHandle_exn_:
			break;
		default:
			break;
	}

//
// length of IDL_char *catStmtLabel
//
    wlength += sizeof(catStmtLabelLength);
	if (catStmtLabel != NULL)
	{
		catStmtLabelLength = strlen(catStmtLabel)+1;
		wlength += catStmtLabelLength;
	}

//
// length of SQLItemDescList_def *outputDesc
//
	SQLITEMDESC_LIST_length( outputDesc, wlength);

//
// length of ERROR_DESC_LIST_def *sqlWarning
//
//
	ERROR_DESC_LIST_LENGTH2(sqlWarning)

//
//
//

  wlength += sizeof (proxySyntaxStringLen);

  if((pSrvrStmt != NULL) && (pSrvrStmt->SpjProxySyntaxString != NULL))
	proxySyntaxStringLen = strlen(pSrvrStmt->SpjProxySyntaxString);
  else
	proxySyntaxStringLen = 0;

  if(proxySyntaxStringLen > 0)
	wlength += proxySyntaxStringLen + 1; // null terminated string


//
// message_length = header + data length
//
    message_length = wlength;

    buffer = pnode->w_allocate(message_length);
    if (buffer == NULL)
       return CEE_ALLOCFAIL;

	curptr = (IDL_char*)(buffer + sizeof(HEADER));

//
// copy odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
//

	IDL_long_copy((IDL_long *)&exception_->exception_nr, curptr);
	IDL_long_copy((IDL_long *)&exception_->exception_detail, curptr);

	switch(exception_->exception_nr)
	{
		case odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_:
		    IDL_long_copy(&exceptionLength, curptr);
	        if (exception_->u.ParamError.ParamDesc != NULL)
			   IDL_charArray_copy((const IDL_char *)exception_->u.ParamError.ParamDesc, curptr);
			break;

		case odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_:
			ERROR_DESC_LIST_copy( (ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, curptr);
			break;

		case odbc_SQLSvc_GetSQLCatalogs_InvalidConnection_exn_:
		case odbc_SQLSvc_GetSQLCatalogs_SQLInvalidHandle_exn_:
			break;

		default:
			break;
	}

//
// copy IDL_char *catStmtLabel
//
	IDL_long_copy(&catStmtLabelLength, curptr);
	if (catStmtLabel != NULL)
		IDL_charArray_copy(catStmtLabel, curptr);

//
// copy of SQLItemDescList_def *outputDesc
//
    SQLITEMDESC_LIST_copy(outputDesc, curptr);

//
// copy ERROR_DESC_LIST_def *sqlWarning
//
    ERROR_DESC_LIST_COPY2(sqlWarning, curptr);

//
// copy the proxy Syntax
//

  if((pSrvrStmt != NULL) && (pSrvrStmt->SpjProxySyntaxString != NULL))
	proxySyntaxStringLen = strlen(pSrvrStmt->SpjProxySyntaxString);
  else
	proxySyntaxStringLen = 0;

  if(proxySyntaxStringLen > 0)
  {

	proxySyntaxStringLen = proxySyntaxStringLen + 1; // null terminated
	IDL_long_copy(&proxySyntaxStringLen, curptr);
	IDL_charArray_copy((const IDL_char *)pSrvrStmt->SpjProxySyntaxString, curptr);
  }
  else
	IDL_long_copy(&proxySyntaxStringLen, curptr);

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbc_SQLSvc_GetSQLCatalogs_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

	SRVRTRACE_EXIT(FILE_OMR+14);
	return CEE_SUCCESS;

} // odbc_SQLSrvr_GetSQLCatalogs_param_res_()

CEE_status
odbc_SQLSrvr_EndTransaction_param_res_(
		  CInterface* pnode
		, IDL_char*& buffer
		, IDL_unsigned_long& message_length
	    , /* In    */ const struct odbc_SQLSvc_EndTransaction_exc_ *exception_
	    , /* In    */ ERROR_DESC_LIST_def *sqlWarning
)
{
	SRVRTRACE_ENTER(FILE_OMR+13);

	IDL_char* curptr;
	IDL_long wlength;
	IDL_long exceptionLength = 0;

	wlength = sizeof(HEADER);

//
// calculate length of the buffer for each parameter
//

//
// length of odbc_SQLSvc_EndTransaction_exc_ *exception_
//
	wlength += sizeof(exception_->exception_nr);
	wlength += sizeof(exception_->exception_detail);

	switch(exception_->exception_nr)
	{
		case odbc_SQLSvc_EndTransaction_ParamError_exn_:
 	       wlength += sizeof(exceptionLength);
	       if (exception_->u.ParamError.ParamDesc != NULL)
		   {
		      exceptionLength = strlen(exception_->u.ParamError.ParamDesc) + 1;
		      wlength += exceptionLength;
		   }
		   break;

		case odbc_SQLSvc_EndTransaction_SQLError_exn_:
			ERROR_DESC_LIST_length( (ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, wlength);
			break;

		case odbc_SQLSvc_GetSQLCatalogs_InvalidConnection_exn_:
		case odbc_SQLSvc_GetSQLCatalogs_SQLInvalidHandle_exn_:
			break;
		default:
			break;
	}

//
// length of ERROR_DESC_LIST_def *sqlWarning
//
//
	ERROR_DESC_LIST_LENGTH2(sqlWarning)

//
// message_length = header + data length
//
    message_length = wlength;

    buffer = pnode->w_allocate(message_length);
    if (buffer == NULL)
       return CEE_ALLOCFAIL;

	curptr = (IDL_char*)(buffer + sizeof(HEADER));

//
// copy odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
//

	IDL_long_copy((IDL_long *)&exception_->exception_nr, curptr);
	IDL_long_copy((IDL_long *)&exception_->exception_detail, curptr);

	switch(exception_->exception_nr)
	{
		case odbc_SQLSvc_EndTransaction_ParamError_exn_:
		    IDL_long_copy(&exceptionLength, curptr);
	        if (exception_->u.ParamError.ParamDesc != NULL)
			   IDL_charArray_copy((const IDL_char *)exception_->u.ParamError.ParamDesc, curptr);
			break;

		case odbc_SQLSvc_EndTransaction_SQLError_exn_:
			ERROR_DESC_LIST_copy( (ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, curptr);
			break;

	    case odbc_SQLSvc_EndTransaction_InvalidConnection_exn_:
	    case odbc_SQLSvc_EndTransaction_TransactionError_exn_:
			break;

		default:
			break;
	}

//
// copy ERROR_DESC_LIST_def *sqlWarning
//
    ERROR_DESC_LIST_COPY2(sqlWarning, curptr);


	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-odbc_SQLSvc_EndTransaction_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

} // odbc_SQLSrvr_EndTransaction_param_res_()


CEE_status
MxoSrvr_ValidateToken_param_res_(
		  CInterface* pnode
		, char*& buffer
		, UInt32& message_length
		, /* In    */ int outTokenLen
		, /* In    */ unsigned char* outToken
)
{
	SRVRTRACE_ENTER(FILE_OMR+21);

	long* parptr;
	long* mapptr;
	char* curptr;
	char* pbuffer;

	long wlength;
	long maplength;

	short number_of_param = 2;

	wlength = sizeof(HEADER);

	maplength = (number_of_param + 1) * sizeof(long);

//
// calculate length of the buffer for each parameter
//

//
// length outTokenLen
//
	wlength += sizeof(outTokenLen);

//
// length of outToken
//
	if(outTokenLen > 0)
	{
//       BYTE_length(outTokenLen,outToken,wlength,maplength);
	   wlength += outTokenLen;
	}

//
// message_length = header + param + maplength + data length
//
	message_length = maplength + wlength;
	buffer = pnode->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	pbuffer = buffer + sizeof(HEADER);
	parptr = (long*)pbuffer;
	mapptr = (long*)pbuffer + number_of_param;
	curptr = (char*)pbuffer + maplength;

//
// copy outTokenLen
//
	IDL_long* par1ptr = (IDL_long* )curptr;
	IDL_long_copy(&outTokenLen, curptr);
//
// copy inToken
//
	IDL_char* par2ptr = (IDL_char *)curptr;
	IDL_byteArray_copy(outToken,outTokenLen,curptr);

	if (curptr > buffer + message_length)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "marshalingsrvr_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-MxoSrvr_ValidateToken_param_res_");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "%d > %d", curptr - buffer, message_length);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}
//
// set end of map list
//
	*(mapptr) = 0;
//
// save relative positions of all parameters
//
	*(parptr++) = (char *)par1ptr - pbuffer;
	*(parptr++) = (char *)par2ptr - pbuffer;

	SRVRTRACE_EXIT(FILE_OMR+21);
	return CEE_SUCCESS;

} // MxoSrvr_ValidateToken_param_res_()

CEE_status
odbc_SQLsrvr_ExtractLob_param_res_(
        CInterface * pnode
      , char* &buffer
      , UInt32& message_length
      , const struct odbc_SQLsrvr_ExtractLob_exc_ *exception_
      , IDL_short extractLobAPI
      , IDL_long_long lobLength
      , IDL_long_long extractLen
      , BYTE * extractData
)
{
    CEE_status sts = CEE_SUCCESS;
    IDL_long wlength = 0;
    char* curptr;

    IDL_long exceptionLength = 0;
    wlength += sizeof(HEADER);

    // calculate length of the buffer for each parameter

    // length of odbc_SQLsrvr_ExtractLob_exc_
    wlength += sizeof(exception_->exception_nr);
    wlength += sizeof(exception_->exception_detail);

    switch(exception_->exception_nr)
    {
        case odbc_SQLsrvr_ExtractLob_ParamError_exn_:
            wlength += sizeof(exceptionLength);
            if (exception_->u.ParamError.ParamDesc != NULL)
            {
                exceptionLength = strlen(exception_->u.ParamError.ParamDesc) + 1;
                wlength += exceptionLength;
            }
            break;
        case odbc_SQLSrvr_ExtractLob_SQLError_exn_:
            ERROR_DESC_LIST_length((ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, wlength);
            break;
        case odbc_SQLsrvr_ExtractLob_InvalidConnection_exn_:
        case odbc_SQLSrvr_ExtractLob_SQLInvalidhandle_exn_:
            break;
        case obdc_SQLSrvr_ExtractLob_AllocLOBDataError_exn_:
            wlength += sizeof(exceptionLength);
            if (exception_->u.ParamError.ParamDesc != NULL)
            {
                exceptionLength = strlen(exception_->u.ParamError.ParamDesc) + 1;
                wlength += exceptionLength;
            }
            break;
        default:
            break;
    }

    wlength += sizeof(IDL_short);
    switch (extractLobAPI) {
    case 0:
        wlength += sizeof(IDL_long_long);
        break;
    case 1:
        wlength += sizeof(IDL_long_long);
        wlength += extractLen;
        break;
    case 2:
        break;
    default:
        break;
    }

    // update the length of message
    message_length = wlength;

    buffer = pnode->w_allocate(message_length);
    if (buffer == NULL)
    {
        return CEE_ALLOCFAIL;
    }
    curptr = (IDL_char*)(buffer + sizeof(HEADER));

    // copy odbc_SQLsrvr_ExtractLob_exc_
    IDL_long_copy((IDL_long *)&exception_->exception_nr, curptr);
    IDL_long_copy((IDL_long *)&exception_->exception_detail, curptr);

    switch(exception_->exception_nr)
    {
        case odbc_SQLsrvr_ExtractLob_ParamError_exn_:
        case obdc_SQLSrvr_ExtractLob_AllocLOBDataError_exn_:
            IDL_long_copy(&exceptionLength, curptr);
            if (exception_->u.ParamError.ParamDesc != NULL)
                IDL_charArray_copy((const IDL_char *)exception_->u.ParamError.ParamDesc, curptr);
            break;

        case odbc_SQLSrvr_ExtractLob_SQLError_exn_:
            ERROR_DESC_LIST_copy((ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, curptr);
            break;

        case odbc_SQLsrvr_ExtractLob_InvalidConnection_exn_:
        case odbc_SQLSrvr_ExtractLob_SQLInvalidhandle_exn_:
            break;

        default:
            break;
    }

    //IDL_long_copy((IDL_long *)&extractLobAPI, curptr);
    IDL_short_copy((IDL_short *)&extractLobAPI, curptr);

    switch (extractLobAPI) {
    case 0:
        IDL_long_long_copy((IDL_long_long *)&lobLength, curptr);
        break;
    case 1:
        IDL_long_long_copy((IDL_long_long *)&extractLen, curptr);
        if (extractLen != 0)
        {
            IDL_byteArray_copy(extractData, extractLen, curptr);
        }
        break;
    case 2:
        break;
    default:
        break;
    }

    return sts;
}

CEE_status
odbc_SQLsrvr_UpdateLob_param_res_(
        CInterface * pnode
      , char* &buffer
      , UInt32& message_length
      , const struct odbc_SQLSvc_UpdateLob_exc_ *exception_
)
{
	CEE_status sts = CEE_SUCCESS;
	IDL_long wlength = 0;

	char * curptr;

	IDL_long exceptionLength = 0;

	wlength += sizeof(HEADER);

	// calculate length of the buffer for each parameter

	//length of odbc_SQLSvc_UpdateLob_exc_
	wlength += sizeof(exception_->exception_nr);
	wlength += sizeof(exception_->exception_detail);

	switch (exception_->exception_nr)
	{
		case odbc_SQLSvc_UpdateLob_ParamError_exn_:
            wlength += sizeof(exceptionLength);
            if (exception_->u.ParamError.ParamDesc != NULL)
            {
                exceptionLength = strlen(exception_->u.ParamError.ParamDesc) + 1;
                wlength += exceptionLength;
            }
            break;
		case odbc_SQLSvc_UpdateLob_InvalidConnect_exn_:
            ERROR_DESC_LIST_length((ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, wlength);
            break;
		case odbc_SQLSvc_UpdateLob_SQLError_exn_:
        case odbc_SQLSvc_UpdateLob_SQLInvalidhandle_exn_:
            break;
        default:
            break;
    }


    message_length = wlength;

    buffer = pnode->w_allocate(message_length);
    if (buffer == NULL)
    {
        return CEE_ALLOCFAIL;
    }
    curptr = (IDL_char *)(buffer + sizeof(HEADER));

    IDL_long_copy((IDL_long *)&exception_->exception_nr, curptr);
    IDL_long_copy((IDL_long *)&exception_->exception_detail, curptr);

    switch(exception_->exception_nr)
    {
        case odbc_SQLSvc_UpdateLob_ParamError_exn_:
        case odbc_SQLSvc_UpdateLob_InvalidConnect_exn_:
            IDL_long_copy(&exceptionLength, curptr);

            if (exception_->u.ParamError.ParamDesc != NULL)
                IDL_charArray_copy((const IDL_char *)exception_->u.ParamError.ParamDesc, curptr);
            break;

        case odbc_SQLSvc_UpdateLob_SQLError_exn_:
            ERROR_DESC_LIST_copy((ERROR_DESC_LIST_def *)&exception_->u.SQLError.errorList, curptr);
            break;

        case odbc_SQLSvc_UpdateLob_SQLInvalidhandle_exn_:
            break;

        default:
            break;
    }
}
