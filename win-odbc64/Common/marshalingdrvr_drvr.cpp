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
#include "sql.h"
#include "sqlext.h"
#include "DrvrSrvr.h"

CEE_status 
odbcas_ASSvc_GetObjRefHdl_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ const CONNECTION_CONTEXT_def *inContext
		, /* In    */ const USER_DESC_def *userDesc
		, /* In    */ IDL_long srvrType
		, /* In    */ IDL_short retryCount
)
{
	IDL_long wlength;
	char *curptr;

	wlength = 0;
	VERSION_def version[4];
	VERSION_def* versionPtr = &version[0];


	IDL_long		datasourceLength = 0; // includes null terminator
	IDL_long		catalogLength = 0;    // includes null terminator
	IDL_long	 	schemaLength = 0;     // includes null terminator
	IDL_long		locationLength = 0;   // includes null terminator
	IDL_long		userRoleLength = 0;   // includes null terminator
    IDL_long		computerNameLength = 0; // includes null terminator
	IDL_long		windowTextLength = 0;   // includes null terminator
	IDL_long		connectOptionsLength = 0; // includes null terminator
	IDL_long		vprocLength = 0; // includes null terminator
	IDL_long		clientUserNameLength = 0; // includes null terminator
	IDL_unsigned_long inContextOptions1 = inContext->inContextOptions1;
	
	//
	// calculate length of the buffer for each parameter
	//
	//

	//  length of IDL_long datasourceLength
	//  length of SQL_IDENTIFIER_DEF datasource
	wlength += sizeof(datasourceLength);
    if (inContext->datasource[0] !=  '\0')
	{
		datasourceLength = strlen(inContext->datasource) + 1;
        wlength += datasourceLength;
	}
	
	//  length of IDL_long catalogLength
	//  length of SQL_IDENTIFIER_DEF catalog
	wlength += sizeof(catalogLength);
    if (inContext->catalog[0] !=  '\0')
	{
		catalogLength = strlen(inContext->catalog) + 1;
	    wlength += catalogLength;
	}

	//  length of IDL_long schemaLength
	//  length of SQL_IDENTIFIER_DEF schema
	wlength += sizeof(schemaLength);
    if (inContext->schema[0] !=  '\0')
	{
		schemaLength = strlen(inContext->schema) + 1;
	    wlength += schemaLength;
	}

	//  length of IDL_long locationLength
	//  length of SQL_IDENTIFIER_DEF location
    wlength += sizeof(locationLength);
    if (inContext->location[0] !=  '\0')
	{
		locationLength = strlen(inContext->location) + 1;
	    wlength += locationLength;
	}
	
	//  length of IDL_long userRoleLength
	//  length of SQL_IDENTIFIER_DEF userRole
	wlength += sizeof(userRoleLength);
    if (inContext->userRole[0] !=  '\0')
	{
		userRoleLength = strlen(inContext->userRole) + 1;
	    wlength += userRoleLength;
	}
  
	// length of IDL_short accessMode
	wlength += sizeof(inContext->accessMode);

	// length of IDL_short autoCommit
	wlength += sizeof(inContext->autoCommit);

	// length of IDL_long queryTimeoutSec
	wlength += sizeof(inContext->queryTimeoutSec);

	// length of IDL_long idleTimeoutSec
	wlength += sizeof(inContext->idleTimeoutSec);

	// length of IDL_long loginTimeoutSec
	wlength += sizeof(inContext->loginTimeoutSec);

	// length of IDL_short txnIsolationLevel
	wlength += sizeof(inContext->txnIsolationLevel);

	// length of IDL_short rowSetSize
	wlength += sizeof(inContext->rowSetSize);

	// length of IDL_long diagnosticFlag
	wlength += sizeof(inContext->diagnosticFlag);

	// lenght of IDL_unsigned_long
	wlength += sizeof(inContext->processId);

	//  length of IDL_long computerNameLength
	//  length of SQL_IDENTIFIER_DEF computerName
    wlength += sizeof(computerNameLength);
	if (inContext->computerName[0] !=  '\0')
	{
		computerNameLength = strlen(inContext->computerName) + 1;
	    wlength += computerNameLength;
	}

	//  length of IDL_long windowTextLength
	//  length of SQL_IDENTIFIER_DEF windowText
	wlength += sizeof(windowTextLength);
    if (inContext->windowText[0] !=  '\0')
	{
		windowTextLength = strlen(inContext->windowText) + 1;
	    wlength += windowTextLength;
	}

	// length of IDL_unsigned_long ctxACP
	wlength += sizeof(inContext->ctxACP);

	// length of IDL_unsigned_long ctxDataLang
	wlength += sizeof(inContext->ctxDataLang);

	// length of IDL_unsigned_long ctxErrorLang
	wlength += sizeof(inContext->ctxErrorLang);

	// length of IDL_short ctxCtrlInferNCHAR
	wlength += sizeof(inContext->ctxCtrlInferNCHAR);

	// length of IDL_short cpuToUse
	wlength += sizeof(inContext->cpuToUse);

	// length of IDL_short cpuToUseEnd
	wlength += sizeof(inContext->cpuToUseEnd);

	// length of IDL_long connectOptionsLength
	wlength += sizeof(connectOptionsLength);
	if(inContext->connectOptions != NULL)
	{
		connectOptionsLength = strlen(inContext->connectOptions) +1;
		wlength += connectOptionsLength;
	}

	// VERSION_LIST_def clientVersionList;
	// length of IDL_long versionListlength
	wlength += sizeof(inContext->clientVersionList._length);

	// Get the versionPtr
	versionPtr = inContext->clientVersionList._buffer;
	
	for (unsigned int i = 0; i < inContext->clientVersionList._length; i++)
	{
		// length of version[0] componentId
		wlength += sizeof(versionPtr->componentId);

		// length of version[0] majorVersion
		wlength += sizeof(versionPtr->majorVersion);

		// length of version[0] minorVersion
		wlength += sizeof(versionPtr->minorVersion);

		// length of version[0] buildId
		wlength += sizeof(versionPtr->buildId);
		
		// Get the next versionlist values
 		versionPtr++;
	}


	// 2nd Parameter

	IDL_unsigned_long userSidLength = 0;
	IDL_long domainNameLength = 0;
	IDL_long userNameLength = 0;
	IDL_long passwordLength = 0;


	// length of userDesc type
	wlength += sizeof(userDesc->userDescType);

	// length of userSid
	wlength += sizeof(userSidLength);
	if (userDesc->userSid._buffer != NULL && userDesc->userSid._length > 0)
	{
	   userSidLength = strlen((char *)userDesc->userSid._buffer) + 1;
	   wlength += userSidLength;
	}
	
	// length of domainName
    wlength += sizeof(domainNameLength);
	if (userDesc->domainName != NULL && userDesc->domainName[0] != '\0')
	{
		domainNameLength = strlen(userDesc->domainName) + 1;
		wlength += domainNameLength;
	}

	// length of userName
	wlength += sizeof(userNameLength);
	if (userDesc->userName != NULL && userDesc->userName[0] != '\0')
	{
		userNameLength = strlen(userDesc->userName) + 1;
		wlength += userNameLength;
	}

	// length of password
	wlength += sizeof(passwordLength);
	if (userDesc->password._buffer != NULL)
	{
	   passwordLength = strlen((char *)userDesc->password._buffer) + 1;
	   wlength += passwordLength;
	}

	// 3rd Parameter
	// length of serverType
	wlength += sizeof(srvrType);

	// 4th Parameter
	// length of retryCount
	wlength += sizeof(retryCount);

	// 5th Param: inContext Options

	wlength += sizeof(inContext->inContextOptions1);
	wlength += sizeof(inContext->inContextOptions2);


	// 6th Parameter: Vproc String
	//
	wlength += sizeof(vprocLength);
	vprocLength = strlen(inContext->clientVproc);
	if (vprocLength > 0)
	{
	   vprocLength = vprocLength+ 1;
	   wlength += vprocLength;
	}

	// 7th Parameter: clientUserName String
	//
	if(inContextOptions1 & INCONTEXT_OPT1_CLIENT_USERNAME)
	{
		wlength += sizeof(clientUserNameLength);
		clientUserNameLength = strlen(inContext->clientUserName);
		if (clientUserNameLength > 0)
		{
		   clientUserNameLength = clientUserNameLength + 1;
		   wlength += clientUserNameLength;
		}
	}

    // Mesage Length
	message_length = wlength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}
	curptr = buffer;
	

    // Swap and then copy the values
    
    // Copy datasource length
	LONG_swap(&datasourceLength);
	IDL_long_copy(&datasourceLength, curptr);
    // copy datasource
	if (inContext->datasource[0] !=  '\0')
	{
		IDL_charArray_copy(inContext->datasource, curptr);
	}
	
	// copy catalog length
	LONG_swap(&catalogLength);
	IDL_long_copy(&catalogLength, curptr);
	// copy catalog
    if (inContext->catalog[0] !=  '\0')
	{
		IDL_charArray_copy(inContext->catalog, curptr);
	}

	// copy schema length
	LONG_swap(&schemaLength);
	IDL_long_copy(&schemaLength, curptr);
	// copy schema
    if (inContext->schema[0] !=  '\0')
	{
		IDL_charArray_copy(inContext->schema, curptr);
	}

	// copy location length
	LONG_swap(&locationLength);
	IDL_long_copy(&locationLength, curptr);
	// copy location
    if (inContext->location[0] !=  '\0')
	{
		IDL_charArray_copy(inContext->location, curptr);
	}
	
	// copy userRole length
	LONG_swap(&userRoleLength);
	IDL_long_copy(&userRoleLength, curptr);
	// copy userRole
	if (inContext->userRole[0] !=  '\0')
	{
		IDL_charArray_copy(inContext->userRole, curptr);
	}

	// copy accessMode
	SHORT_swap((IDL_short *)&inContext->accessMode);
	IDL_short_copy((IDL_short *)&inContext->accessMode, curptr);
	SHORT_swap((IDL_short *)&inContext->accessMode);  

	// copy autoCommit
	SHORT_swap((IDL_short *)&inContext->autoCommit);
	IDL_short_copy((IDL_short *)&inContext->accessMode, curptr);
	SHORT_swap((IDL_short *)&inContext->autoCommit);

	// copy queryTimeoutSec
	LONG_swap((IDL_long *)&inContext->queryTimeoutSec);
	IDL_long_copy((IDL_long *)&inContext->queryTimeoutSec, curptr);
	LONG_swap((IDL_long *)&inContext->queryTimeoutSec);

	// copy idleTimeoutSec
	LONG_swap((IDL_long *)&inContext->idleTimeoutSec);
	IDL_long_copy((IDL_long *)&inContext->idleTimeoutSec, curptr);
	LONG_swap((IDL_long *)&inContext->idleTimeoutSec);

	// copy loginTimeoutSec
	LONG_swap((IDL_long *)&inContext->loginTimeoutSec);
	IDL_long_copy((IDL_long *)&inContext->loginTimeoutSec, curptr);
	LONG_swap((IDL_long *)&inContext->loginTimeoutSec);

	// copy txnIsolationLevel
	SHORT_swap((IDL_short *)&inContext->txnIsolationLevel);
	IDL_short_copy((IDL_short *)&inContext->txnIsolationLevel, curptr);
	SHORT_swap((IDL_short *)&inContext->txnIsolationLevel);

	//copy rowSetSize
	SHORT_swap((IDL_short *)&inContext->rowSetSize);
	IDL_short_copy((IDL_short *)&inContext->rowSetSize, curptr);
	SHORT_swap((IDL_short *)&inContext->rowSetSize);

	// copy diagnosticFlag
	LONG_swap((IDL_long *)&inContext->diagnosticFlag);
	IDL_long_copy((IDL_long *)&inContext->diagnosticFlag, curptr);
	LONG_swap((IDL_long *)&inContext->diagnosticFlag);

	// copy processId
	ULONG_swap((IDL_unsigned_long *)&inContext->processId);
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->processId, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->processId);


	// copy computerNameLength
	LONG_swap(&computerNameLength);
	IDL_long_copy(&computerNameLength, curptr);
	// copy computerName
	if (inContext->computerName[0] !=  '\0')
	{
		IDL_charArray_copy(inContext->computerName, curptr);

	}

	// copy windowTextLength
	LONG_swap(&windowTextLength);
	IDL_long_copy(&windowTextLength, curptr);
	// copy windowText
	if (inContext->windowText !=  NULL)
	{
		IDL_charArray_copy(inContext->windowText, curptr);
	}
 
	// copy ctxACP
	ULONG_swap((IDL_unsigned_long *)&inContext->ctxACP);
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->ctxACP, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->ctxACP);
	
	// copy ctxDataLang
	ULONG_swap((IDL_unsigned_long *)&inContext->ctxDataLang);
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->ctxDataLang, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->ctxDataLang);

	// copy ctxErrorLang
	ULONG_swap((IDL_unsigned_long *)&inContext->ctxErrorLang);
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->ctxErrorLang, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->ctxErrorLang);

	// copy ctxCtrlInferNCHAR
	SHORT_swap((IDL_short *)&inContext->ctxCtrlInferNCHAR);
	IDL_short_copy((IDL_short *)&inContext->ctxCtrlInferNCHAR, curptr);
	SHORT_swap((IDL_short *)&inContext->ctxCtrlInferNCHAR);

	// copy cpuToUse
	SHORT_swap((IDL_short *)&inContext->cpuToUse);
	IDL_short_copy((IDL_short *)&inContext->cpuToUse, curptr);
	SHORT_swap((IDL_short *)&inContext->cpuToUse);

	// copy cpuToUseEnd
	SHORT_swap((IDL_short *)&inContext->cpuToUseEnd);
	IDL_short_copy((IDL_short *)&inContext->cpuToUseEnd, curptr);
	SHORT_swap((IDL_short *)&inContext->cpuToUseEnd);


	// copy connectOptions
	LONG_swap(&connectOptionsLength);
	IDL_long_copy(&connectOptionsLength, curptr);
	if (connectOptionsLength > 0)
	{
		IDL_charArray_copy(inContext->connectOptions, curptr);
	}


	// copy versionList Length
	ULONG_swap((IDL_unsigned_long *)&inContext->clientVersionList._length);
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->clientVersionList._length, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->clientVersionList._length);
	

	versionPtr = inContext->clientVersionList._buffer;
	
	for (unsigned int j = 0; j < inContext->clientVersionList._length; j++)
	{
		// copy componentId
		SHORT_swap(&versionPtr->componentId);
		IDL_short_copy(&versionPtr->componentId, curptr);
		SHORT_swap(&versionPtr->componentId);

		// copy majorVersion
		SHORT_swap(&versionPtr->majorVersion);
		IDL_short_copy(&versionPtr->majorVersion, curptr);
		SHORT_swap(&versionPtr->majorVersion);

		// copy minorVersion
		SHORT_swap(&versionPtr->minorVersion);
		IDL_short_copy(&versionPtr->minorVersion, curptr);
		SHORT_swap(&versionPtr->minorVersion);

		// copy buildId
		ULONG_swap(&versionPtr->buildId);
		IDL_unsigned_long_copy(&versionPtr->buildId, curptr);
		ULONG_swap(&versionPtr->buildId);
		
		// Get the next versionlist values
 		versionPtr++;
	}

	// Copy 2nd Parameter

	// copy userDesc Type

	LONG_swap((IDL_long *)&userDesc->userDescType);
	IDL_long_copy((IDL_long *)&userDesc->userDescType, curptr);
	LONG_swap((IDL_long *)&userDesc->userDescType);

	// copy userSidLength
	ULONG_swap((IDL_unsigned_long *)&userSidLength);
	IDL_unsigned_long_copy((IDL_unsigned_long *)&userSidLength, curptr);
	
	// copy userSid
	if (userSidLength > 0)
	{
		IDL_charArray_copy((char *)userDesc->userSid._buffer, curptr);
	}
	
	// copy domainNameLength
	LONG_swap(&domainNameLength);
	IDL_long_copy(&domainNameLength, curptr);
	// copy domainName
	if (userDesc->domainName != NULL && userDesc->domainName[0] != '\0')
	{
		IDL_charArray_copy(userDesc->domainName, curptr);
	}

	// copy userNameLength
	LONG_swap(&userNameLength);
	IDL_long_copy(&userNameLength, curptr);
	// copy userName
	if (userDesc->userName != NULL && userDesc->userName[0] != '\0')
	{
		IDL_charArray_copy(userDesc->userName, curptr);
	}


	// copy passwordLength
	ULONG_swap((IDL_unsigned_long *)&passwordLength);
	IDL_unsigned_long_copy((IDL_unsigned_long *)&passwordLength, curptr);

	// copy password
	if (userDesc->password._buffer != NULL)
	{
		IDL_charArray_copy((char *)userDesc->password._buffer, curptr);
	}
 
	// Copy 3rd Parameter
	// copy serverType
	LONG_swap(&srvrType);
	IDL_long_copy(&srvrType, curptr);
	LONG_swap(&srvrType);

	// Copy 4th Parameter
	// copy retryCount
	SHORT_swap(&retryCount);
	IDL_short_copy(&retryCount, curptr);
	SHORT_swap(&retryCount);

	//
	// 5th Param
	// Incontext options
	ULONG_swap((IDL_unsigned_long *)&inContext->inContextOptions1);
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->inContextOptions1, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->inContextOptions1);

	ULONG_swap((IDL_unsigned_long *)&inContext->inContextOptions2);
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->inContextOptions2, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->inContextOptions2);



	// 6th Parameter: Vproc String
	LONG_swap(&vprocLength);
	IDL_long_copy(&vprocLength, curptr);
	LONG_swap(&vprocLength);
	if(vprocLength > 0)
	{
		IDL_charArray_copy((char *)inContext->clientVproc, curptr);
	}

	// 7th Parameter: Vproc String
	if(inContextOptions1 & INCONTEXT_OPT1_CLIENT_USERNAME)
	{
		LONG_swap(&clientUserNameLength);
		IDL_long_copy(&clientUserNameLength, curptr);
		LONG_swap(&clientUserNameLength);
		if(clientUserNameLength > 0)
		{
			IDL_charArray_copy((char *)inContext->clientUserName, curptr);
		}
	}

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}

	return CEE_SUCCESS;

}


CEE_status
odbcas_ASSvc_StopSrvr_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ IDL_long srvrType
		, /* In    */ const IDL_char *srvrObjRef
		, /* In    */ IDL_long StopType
)
{

	IDL_char *curptr = NULL;
	IDL_long wlength =  0;
	IDL_long srvrObjRefLen = 0;

//
// calculate length of the buffer for each parameter
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
	wlength += sizeof(srvrObjRefLen);
	if (srvrObjRef != NULL)
	{
		srvrObjRefLen = strlen(srvrObjRef) + 1;
		wlength += srvrObjRefLen;
	}

//
// length of IDL_long StopType
//
	wlength += sizeof(StopType);

//
// message length
//
	message_length = wlength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	curptr = buffer;

	if(pSystem->swap() == SWAP_YES)
	{
		LONG_swap(&dialogueId);
		LONG_swap(&srvrType);
		LONG_swap(&srvrObjRefLen);
		LONG_swap(&StopType);
	}


//
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long_copy(&dialogueId, curptr);

//
// copy IDL_long srvrType
//
	IDL_long_copy(&srvrType, curptr);

//	
// copy IDL_char* srvrObjRef
//
	IDL_long_copy(&srvrObjRefLen, curptr);
	if(srvrObjRefLen > 0)
	   IDL_charArray_copy(srvrObjRef, curptr);

//
// copy IDL_long StopType
//
	IDL_long_copy(&StopType, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}

	return CEE_SUCCESS;

} // odbcas_ASSvc_StopSrvr_param_pst_()


CEE_status
odbc_SQLSvc_InitializeDialogue_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ const USER_DESC_def *userDesc
		, /* In    */ const CONNECTION_CONTEXT_def *inContext
		, /* In    */ DIALOGUE_ID_def dialogueId
  )
{
	IDL_long wlength;
	char *curptr;

	wlength = 0;
	VERSION_def version[4];
	VERSION_def* versionPtr = &version[0];


	IDL_long		datasourceLength = 0; // includes null terminator
	IDL_long		catalogLength = 0;    // includes null terminator
	IDL_long	 	schemaLength = 0;     // includes null terminator
	IDL_long		locationLength = 0;   // includes null terminator
	IDL_long		userRoleLength = 0;   // includes null terminator
		
	IDL_long		computerNameLength = 0; // includes null terminator
	IDL_long		windowTextLength = 0;   // includes null terminator
	IDL_long        connectOptionsLength = 0; // includes null terminator
	IDL_long		sessionNameLength = 0; // includes null terminator
	IDL_long		clientUserNameLength = 0; // includes null terminator
	IDL_unsigned_long inContextOptions1 = inContext->inContextOptions1;
	
	//
	// calculate length of the buffer for each parameter
	//
	//

	// 1st Parameter

	IDL_unsigned_long userSidLength = 0;
	IDL_long domainNameLength = 0;
	IDL_long userNameLength = 0;
	IDL_long passwordLength = 0;


	// length of userDesc type
	wlength += sizeof(userDesc->userDescType);

	// length of userSid
	wlength += sizeof(userSidLength);
	if (userDesc->userSid._buffer != NULL && userDesc->userSid._length > 0)
	{
	   userSidLength = strlen((char *)userDesc->userSid._buffer) + 1;
	   wlength += userSidLength;
	}
	
	// length of domainName
	wlength += sizeof(domainNameLength);
	if (userDesc->domainName[0] != '\0')
	{
		domainNameLength = strlen(userDesc->domainName) + 1;
		wlength += domainNameLength;
	}

	// length of userName
	wlength += sizeof(userNameLength);
	if (userDesc->userName != 0 && userDesc->userName[0] != '\0')
	{
		userNameLength = strlen(userDesc->userName) + 1;
		wlength += userNameLength;
	}

	// length of password
    wlength += sizeof(passwordLength);
	if (userDesc->password._buffer != NULL)
	{
	   //passwordLength = strlen((char *)userDesc->password._buffer) + 1;
	   passwordLength = userDesc->password._length + 1;
	   wlength += passwordLength;
	}


	// 2nd Parameter

	//  length of IDL_long datasourceLength
	//  length of SQL_IDENTIFIER_DEF datasource
    wlength += sizeof(datasourceLength);
    if (inContext->datasource[0] !=  '\0')
	{
		datasourceLength = strlen(inContext->datasource) + 1;
	    wlength += datasourceLength;
	}
	
	//  length of IDL_long catalogLength
	//  length of SQL_IDENTIFIER_DEF catalog
    wlength += sizeof(catalogLength);
    if (inContext->catalog[0] !=  '\0')
	{
		catalogLength = strlen(inContext->catalog) + 1;
	    wlength += catalogLength;
	}

	//  length of IDL_long schemaLength
	//  length of SQL_IDENTIFIER_DEF schema
    wlength += sizeof(schemaLength);
    if (inContext->schema[0] !=  '\0')
	{
		schemaLength = strlen(inContext->schema) + 1;
	    wlength += schemaLength;
	}

	//  length of IDL_long locationLength
	//  length of SQL_IDENTIFIER_DEF location
    wlength += sizeof(locationLength);
    if (inContext->location[0] !=  '\0')
	{
		locationLength = strlen(inContext->location) + 1;
	    wlength += locationLength;
	}
	
	//  length of IDL_long userRoleLength
	//  length of SQL_IDENTIFIER_DEF userRole
    wlength += sizeof(userRoleLength);
    if (inContext->userRole[0] !=  '\0')
	{
		userRoleLength = strlen(inContext->userRole) + 1;
	    wlength += userRoleLength;
	}
  
	// length of IDL_short accessMode
	wlength += sizeof(inContext->accessMode);

	// length of IDL_short autoCommit
	wlength += sizeof(inContext->autoCommit);

	// length of IDL_long queryTimeoutSec
	wlength += sizeof(inContext->queryTimeoutSec);

	// length of IDL_long idleTimeoutSec
	wlength += sizeof(inContext->idleTimeoutSec);

	// length of IDL_long loginTimeoutSec
	wlength += sizeof(inContext->loginTimeoutSec);

	// length of IDL_short txnIsolationLevel
	wlength += sizeof(inContext->txnIsolationLevel);

	// length of IDL_short rowSetSize
	wlength += sizeof(inContext->rowSetSize);

	// length of IDL_long diagnosticFlag
	wlength += sizeof(inContext->diagnosticFlag);

	// lenght of IDL_unsigned_long
	wlength += sizeof(inContext->processId);

	//  length of IDL_long computerNameLength
	//  length of SQL_IDENTIFIER_DEF computerName
    wlength += sizeof(computerNameLength);
	if (inContext->computerName[0] !=  '\0')
	{
		computerNameLength = strlen(inContext->computerName) + 1;
	    wlength += computerNameLength;
	}

	//  length of IDL_long windowTextLength
	//  length of SQL_IDENTIFIER_DEF windowText
    wlength += sizeof(windowTextLength);
    if (inContext->windowText[0] !=  '\0')
	{
		windowTextLength = strlen(inContext->windowText) + 1;
	    wlength += windowTextLength;
	}

	// length of IDL_unsigned_long ctxACP
	wlength += sizeof(inContext->ctxACP);

	// length of IDL_unsigned_long ctxDataLang
	wlength += sizeof(inContext->ctxDataLang);

	// length of IDL_unsigned_long ctxErrorLang
	wlength += sizeof(inContext->ctxErrorLang);

	// length of IDL_short ctxCtrlInferNCHAR
	wlength += sizeof(inContext->ctxCtrlInferNCHAR);

	// length of IDL_short cpuToUse
	wlength += sizeof(inContext->cpuToUse);

	// length of IDL_short cpuToUseEnd
	wlength += sizeof(inContext->cpuToUseEnd);


	// length of IDL_long connectOptionsLength
	wlength += sizeof(connectOptionsLength);

	if(inContext->connectOptions != NULL)
	{
		connectOptionsLength = strlen(inContext->connectOptions) +1;
		wlength += connectOptionsLength;
	}


	// VERSION_LIST_def clientVersionList;
	// length of IDL_long versionListlength
	wlength += sizeof(inContext->clientVersionList._length);

	// Get the versionPtr
	versionPtr = inContext->clientVersionList._buffer;
	
	for (unsigned int i = 0; i < inContext->clientVersionList._length; i++)
	{
		// length of version[0] componentId
		wlength += sizeof(versionPtr->componentId);

		// length of version[0] majorVersion
		wlength += sizeof(versionPtr->majorVersion);

		// length of version[0] minorVersion
		wlength += sizeof(versionPtr->minorVersion);

		// length of version[0] buildId
		wlength += sizeof(versionPtr->buildId);
		
		// Get the next versionlist values
 		versionPtr++;
	}



	// 3rd Parameter
	// length of dialogueId
	wlength += sizeof(dialogueId);

	//
	// 5th Param
	// Incontext options
	wlength += sizeof(inContext->inContextOptions1);
	wlength += sizeof(inContext->inContextOptions2);

	if(inContextOptions1 & INCONTEXT_OPT1_SESSIONNAME)
	{
		wlength += sizeof(sessionNameLength);
		sessionNameLength = strlen(inContext->sessionName);
		if(sessionNameLength > 0)
		{
			sessionNameLength += 1; // For the null terminator
			wlength += sessionNameLength;
		}
	}

	if(inContextOptions1 & INCONTEXT_OPT1_CLIENT_USERNAME)
	{
		wlength += sizeof(clientUserNameLength);
		clientUserNameLength = strlen(inContext->clientUserName);
		if(clientUserNameLength > 0)
		{
			clientUserNameLength += 1; // For the null terminator
			wlength += clientUserNameLength;
		}
	}

    // Mesage Length
	message_length = wlength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}
	curptr = buffer;
	

    // Swap and then copy the values

	// Copy 1st Parameter
	// copy userDesc Type

	LONG_swap((IDL_long *)&userDesc->userDescType,pSystem->swap());
	IDL_long_copy((IDL_long *)&userDesc->userDescType, curptr);
	LONG_swap((IDL_long *)&userDesc->userDescType,pSystem->swap());

	// copy userSidLength
	ULONG_swap((IDL_unsigned_long *)&userSidLength,pSystem->swap());
	IDL_unsigned_long_copy((IDL_unsigned_long *)&userSidLength, curptr);
	
	// copy userSid
	if (userSidLength > 0)
	{
		IDL_charArray_copy((char *)userDesc->userSid._buffer, curptr);
	}
	
	// copy domainNameLength
	LONG_swap(&domainNameLength,pSystem->swap());
	IDL_long_copy(&domainNameLength, curptr);
	// copy domainName
	if (userDesc->domainName != NULL && userDesc->domainName[0] != '\0')
	{
		IDL_charArray_copy(userDesc->domainName, curptr);
	}

	// copy userNameLength
	LONG_swap(&userNameLength,pSystem->swap());
	IDL_long_copy(&userNameLength, curptr);
	// copy userName
	if (userDesc->userName != NULL && userDesc->userName[0] != '\0')
	{
		IDL_charArray_copy(userDesc->userName, curptr);
	}


	// copy passwordLength
	IDL_unsigned_long tmp_passwordLength = (IDL_unsigned_long) passwordLength;
	ULONG_swap((IDL_unsigned_long *)&passwordLength,pSystem->swap());
	IDL_unsigned_long_copy((IDL_unsigned_long *)&passwordLength, curptr);
	// copy password
	if (userDesc->password._buffer != NULL)
	{
		//Modified by Arvind to support the blank and 0 values
		IDL_byteArray_copy(userDesc->password._buffer,tmp_passwordLength, curptr);
	}

    
	// Copy 2nd Parameter
    // Copy datasource length
	LONG_swap(&datasourceLength,pSystem->swap());
	IDL_long_copy(&datasourceLength, curptr);
    // copy datasource
	if (inContext->datasource[0] !=  '\0')
	{
		IDL_charArray_copy(inContext->datasource, curptr);
	}
	
	// copy catalog length
	LONG_swap(&catalogLength,pSystem->swap());
	IDL_long_copy(&catalogLength, curptr);
	// copy catalog
    if (inContext->catalog[0] !=  '\0')
	{
		IDL_charArray_copy(inContext->catalog, curptr);
	}

	// copy schema length
	LONG_swap(&schemaLength,pSystem->swap());
	IDL_long_copy(&schemaLength, curptr);
	// copy schema
    if (inContext->schema[0] !=  '\0')
	{
		IDL_charArray_copy(inContext->schema, curptr);
	}

	// copy location length
	LONG_swap(&locationLength,pSystem->swap());
	IDL_long_copy(&locationLength, curptr);
	// copy location
    if (inContext->location[0] !=  '\0')
	{
		IDL_charArray_copy(inContext->location, curptr);
	}
	
	// copy userRole length
	LONG_swap(&userRoleLength,pSystem->swap());
	IDL_long_copy(&userRoleLength, curptr);
	// copy userRole
	if (inContext->userRole[0] !=  '\0')
	{
		IDL_charArray_copy(inContext->userRole, curptr);
	}

	// copy accessMode
	SHORT_swap((short *)&inContext->accessMode,pSystem->swap());
	IDL_short_copy((short *)&inContext->accessMode, curptr);
	SHORT_swap((short *)&inContext->accessMode,pSystem->swap());

	// copy autoCommit
	SHORT_swap((short *)&inContext->autoCommit,pSystem->swap());
	IDL_short_copy((short *)&inContext->autoCommit, curptr);
	SHORT_swap((short *)&inContext->autoCommit,pSystem->swap());

	// copy queryTimeoutSec
	LONG_swap((IDL_long *)&inContext->queryTimeoutSec,pSystem->swap());
	IDL_long_copy((IDL_long *)&inContext->queryTimeoutSec, curptr);
	LONG_swap((IDL_long *)&inContext->queryTimeoutSec,pSystem->swap());

	// copy idleTimeoutSec
	LONG_swap((IDL_long *)&inContext->idleTimeoutSec,pSystem->swap());
	IDL_long_copy((IDL_long *)&inContext->idleTimeoutSec, curptr);
	LONG_swap((IDL_long *)&inContext->idleTimeoutSec,pSystem->swap());

	// copy loginTimeoutSec
	LONG_swap((IDL_long *)&inContext->loginTimeoutSec,pSystem->swap());
	IDL_long_copy((IDL_long *)&inContext->loginTimeoutSec, curptr);
	LONG_swap((IDL_long *)&inContext->loginTimeoutSec,pSystem->swap());

	// copy txnIsolationLevel
	SHORT_swap((short *)&inContext->txnIsolationLevel,pSystem->swap());
	IDL_short_copy((short *)&inContext->txnIsolationLevel, curptr);
	SHORT_swap((short *)&inContext->txnIsolationLevel,pSystem->swap());

	//copy rowSetSize
	SHORT_swap((short *)&inContext->rowSetSize,pSystem->swap());
	IDL_short_copy((short *)&inContext->rowSetSize, curptr);
	SHORT_swap((short *)&inContext->rowSetSize,pSystem->swap());

	// copy diagnosticFlag
	LONG_swap((IDL_long *)&inContext->diagnosticFlag,pSystem->swap());
	IDL_long_copy((IDL_long *)&inContext->diagnosticFlag, curptr);
	LONG_swap((IDL_long *)&inContext->diagnosticFlag,pSystem->swap());

	// copy processId
	ULONG_swap((IDL_unsigned_long *)&inContext->processId,pSystem->swap());
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->processId, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->processId,pSystem->swap());


	// copy computerNameLength
	LONG_swap(&computerNameLength,pSystem->swap());
	IDL_long_copy(&computerNameLength, curptr);
	// copy computerName
	if (inContext->computerName[0] !=  '\0')
	{
		IDL_charArray_copy(inContext->computerName, curptr);

	}

	// copy windowTextLength
	LONG_swap(&windowTextLength,pSystem->swap());
	IDL_long_copy(&windowTextLength, curptr);
	// copy windowText
	if (inContext->windowText !=  NULL)
	{
		IDL_charArray_copy(inContext->windowText, curptr);
	}
 
	// copy ctxACP
	ULONG_swap((IDL_unsigned_long *)&inContext->ctxACP,pSystem->swap());
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->ctxACP, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->ctxACP,pSystem->swap());
	
	// copy ctxDataLang
	ULONG_swap((IDL_unsigned_long *)&inContext->ctxDataLang,pSystem->swap());
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->ctxDataLang, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->ctxDataLang,pSystem->swap());

	// copy ctxErrorLang
	ULONG_swap((IDL_unsigned_long *)&inContext->ctxErrorLang,pSystem->swap());
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->ctxErrorLang, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->ctxErrorLang,pSystem->swap());

	// copy ctxCtrlInferNCHAR
	SHORT_swap((short *)&inContext->ctxCtrlInferNCHAR,pSystem->swap());
	IDL_short_copy((short *)&inContext->ctxCtrlInferNCHAR, curptr);
	SHORT_swap((short *)&inContext->ctxCtrlInferNCHAR,pSystem->swap());

	// copy cpuToUse
	SHORT_swap((IDL_short *)&inContext->cpuToUse,pSystem->swap());
	IDL_short_copy((IDL_short *)&inContext->cpuToUse, curptr);
	SHORT_swap((IDL_short *)&inContext->cpuToUse,pSystem->swap());

	// copy cpuToUseEnd
	SHORT_swap((IDL_short *)&inContext->cpuToUseEnd,pSystem->swap());
	IDL_short_copy((IDL_short *)&inContext->cpuToUseEnd, curptr);
	SHORT_swap((IDL_short *)&inContext->cpuToUseEnd,pSystem->swap());


	// copy connectOptions
	LONG_swap(&connectOptionsLength,pSystem->swap());
	IDL_long_copy(&connectOptionsLength, curptr);
	if (connectOptionsLength > 0)
	{
		IDL_charArray_copy(inContext->connectOptions, curptr);
	}

	// copy versionList Length
	ULONG_swap((IDL_unsigned_long *)&inContext->clientVersionList._length,pSystem->swap());
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->clientVersionList._length, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->clientVersionList._length,pSystem->swap());
	

	versionPtr = inContext->clientVersionList._buffer;
	
	for (unsigned int j = 0; j < inContext->clientVersionList._length; j++)
	{
		// copy componentId
		SHORT_swap(&versionPtr->componentId,pSystem->swap());
		IDL_short_copy(&versionPtr->componentId, curptr);
		SHORT_swap(&versionPtr->componentId,pSystem->swap());

		// copy majorVersion
		SHORT_swap(&versionPtr->majorVersion,pSystem->swap());
		IDL_short_copy(&versionPtr->majorVersion, curptr);
		SHORT_swap(&versionPtr->majorVersion,pSystem->swap());

		// copy minorVersion
		SHORT_swap(&versionPtr->minorVersion,pSystem->swap());
		IDL_short_copy(&versionPtr->minorVersion, curptr);
		SHORT_swap(&versionPtr->minorVersion,pSystem->swap());

		// copy buildId
		ULONG_swap(&versionPtr->buildId,pSystem->swap());
		IDL_unsigned_long_copy(&versionPtr->buildId, curptr);
		ULONG_swap(&versionPtr->buildId,pSystem->swap());
		
		// Get the next versionlist values
 		versionPtr++;
	}


 
	// Copy 3rd Parameter
	// copy dialogueId
	LONG_swap(&dialogueId,pSystem->swap());
	IDL_long_copy(&dialogueId, curptr);
	LONG_swap(&dialogueId,pSystem->swap());

	ULONG_swap((IDL_unsigned_long *)&inContext->inContextOptions1,pSystem->swap());
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->inContextOptions1, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->inContextOptions1,pSystem->swap());

	ULONG_swap((IDL_unsigned_long *)&inContext->inContextOptions2,pSystem->swap());
	IDL_unsigned_long_copy((IDL_unsigned_long *)&inContext->inContextOptions2, curptr);
	ULONG_swap((IDL_unsigned_long *)&inContext->inContextOptions2,pSystem->swap());

	if(inContextOptions1 & INCONTEXT_OPT1_SESSIONNAME)
	{
	LONG_swap(&sessionNameLength,pSystem->swap());
	IDL_long_copy(&sessionNameLength, curptr);
	if (sessionNameLength > 0)
		IDL_charArray_copy(inContext->sessionName, curptr);
	}

	if(inContextOptions1 & INCONTEXT_OPT1_CLIENT_USERNAME)
	{
		LONG_swap(&clientUserNameLength,pSystem->swap());
		IDL_long_copy(&clientUserNameLength, curptr);
		if (clientUserNameLength > 0)
			IDL_charArray_copy(inContext->clientUserName, curptr);
	}

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}

	return CEE_SUCCESS;

} // odbc_SQLSvc_InitializeDialogue_param_pst_()

CEE_status
odbc_SQLSvc_TerminateDialogue_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
  )
{
	IDL_long wlength;
	IDL_char *curptr;

	wlength = 0;

	//
	// calculate length of the buffer for each parameter
	//
	//

	// 1st Parameter

	// length of dialogueId
	wlength += sizeof(dialogueId);

    // Mesage Length
	message_length = wlength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}
	curptr = buffer;
	

    // Swap and then copy the values
	// Copy 1st Parameter
	// copy dialogueId
	LONG_swap(&dialogueId,pSystem->swap());
	IDL_long_copy(&dialogueId, curptr);
	LONG_swap(&dialogueId,pSystem->swap());

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}

	return CEE_SUCCESS;

} // odbc_SQLSvc_TerminateDialogue_param_pst_()


CEE_status
odbc_SQLDrvr_SetConnectionOption_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ IDL_short connectionOption
		, /* In    */ IDL_long optionValueNum
		, /* In    */ IDL_string optionValueStr
  )
{
	IDL_long wlength = 0;
	IDL_long optionValueStrLen = 0;
	IDL_char *curptr;

//
// calculate length of the buffer for each parameter
//
//

//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof(dialogueId);

//
// length of IDL_short connectionOption
// 
	wlength += sizeof (connectionOption);

//
// length IDL_long optionValueNum
//
	wlength += sizeof(optionValueNum);

//
// length of IDL_long optionValueStrLen + IDL_string optionValueStr
//
	wlength += sizeof(optionValueStrLen);

	if (optionValueStr != NULL)
		optionValueStrLen = strlen(optionValueStr) + 1;
	else
		optionValueStr = 0;

	wlength += optionValueStrLen;


//
// message length
//
	message_length = wlength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	curptr = buffer;

	if(pSystem->swap() == SWAP_YES)
	{
		LONG_swap(&dialogueId);
		SHORT_swap(&connectionOption);
		LONG_swap(&optionValueNum);
		LONG_swap(&optionValueStrLen);
	}

//
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_short connectionOption
//
	IDL_short_copy(&connectionOption, curptr);
//
//
// copy IDL_long optionValueNum
//
	IDL_long_copy(&optionValueNum, curptr);
//
//
//
// copy IDL_string optionValueStr
//
	IDL_long_copy(&optionValueStrLen, curptr);
	if (optionValueStr != NULL)
		IDL_charArray_copy(optionValueStr, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}

	return CEE_SUCCESS;

} // odbc_SQLDrvr_SetConnectionOption_param_pst_()


CEE_status
odbc_SQLDrvr_Fetch_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ IDL_unsigned_long_long MaxRowCnt
		, /* In    */ IDL_unsigned_long_long MaxRowLen
		, /* In    */ IDL_long sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
		, /* In    */ Long stmtHandle
        , /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_long stmtCharset
		, /* In    */ IDL_string cursorName
		, /* In    */ IDL_long cursorCharset
		, /* In    */ const IDL_char *setStmtOptions
)
{
	IDL_long	wlength = 0;
	IDL_char	*curptr = NULL;

	IDL_long stmtLabelLength      = 0;
	IDL_long cursorLength         = 0;
	IDL_long setStmtOptionsLength = 0;


//
// calculate length of the buffer for each parameter
//

//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof(dialogueId);

//
// length of IDL_long sqlAsyncEnable
//
	wlength += sizeof(sqlAsyncEnable);

//
// length of IDL_long queryTimeout
//
	wlength += sizeof(queryTimeout);

//
// length of IDL_long stmtHandle
// 
	wlength += sizeof(stmtHandle);

//
// length of IDL_long stmtLength
// length of IDL_char *stmtLabel
// length of IDL_long stmtCharset
// 
	wlength += sizeof(stmtLabelLength);
	if (stmtLabel != NULL)
	{
        stmtLabelLength = strlen(stmtLabel) + 1;
		wlength += stmtLabelLength;
		wlength += sizeof(stmtCharset);
	}
	else
	{
		stmtLabelLength = 0;
	}


//
// length of IDL_long MaxRowCnt
//
	wlength += sizeof(MaxRowCnt);

//
// length of IDL_long MaxRowLen
//
	wlength += sizeof(MaxRowLen);


//
// length of IDL_long cursorLength
// length IDL_string cursorName
// length of IDL_long cursorCharset
//
    wlength += sizeof(cursorLength);
	if (cursorName != NULL)
	{
		wlength += strlen(cursorName) +1;
		wlength += sizeof(cursorCharset);
	}

//
// length of IDL_long setStmtOptionsLength
// length of IDL_string setStmtOptions
// 
	wlength += sizeof(setStmtOptionsLength);
	if (setStmtOptions != NULL)
	{
		setStmtOptionsLength = strlen(setStmtOptions) + 1;
		wlength += setStmtOptionsLength;
	}


	message_length = wlength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	curptr = buffer;

	if(pSystem->swap() == SWAP_YES)
	{
		LONG_swap(&dialogueId);
		LONG_swap(&sqlAsyncEnable);
		LONG_swap(&queryTimeout);
		LONG_swap(&stmtHandle);
		ULONGLONG_swap((unsigned __int64 *)&MaxRowCnt);
		ULONGLONG_swap((unsigned __int64 *)&MaxRowLen);
		LONG_swap(&stmtLabelLength);
		LONG_swap(&stmtCharset);
		LONG_swap(&cursorLength);
		LONG_swap(&cursorCharset);
		LONG_swap(&setStmtOptionsLength);
	}


//
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long_copy(&dialogueId, curptr);
//
// copy IDL_long sqlAsyncEnable
//
	IDL_long_copy(&sqlAsyncEnable, curptr);
//
// copy IDL_long queryTimeout
//
	IDL_long_copy(&queryTimeout, curptr);

//
// copy IDL_long stmtHandle
//
	Long_copy(&stmtHandle, curptr);

//
// copy IDL_long stmtLabelLength
//
	IDL_long_copy(&stmtLabelLength, curptr);

	if (stmtLabel != NULL)
	{
//
// copy IDL_char *stmtLabel
//
		IDL_charArray_copy(stmtLabel, curptr);

//
// copy IDL_long stmtCharset
//
	
		IDL_long_copy(&stmtCharset, curptr);
	}

//
// copy IDL_long MaxRowCnt
//
	IDL_unsigned_long_long_copy(&MaxRowCnt, curptr);

//
// copy IDL_long MaxRowLen
//
	IDL_unsigned_long_long_copy(&MaxRowLen, curptr);

//
// copy IDL_long cursorLength
//
	IDL_long_copy(&cursorLength, curptr);

	if (cursorName != NULL)
	{
	//
	// copy IDL_string cursorName
	//
		IDL_charArray_copy(cursorName, curptr);
	//
	// copy IDL_long cursorCharset
	//
		IDL_long_copy(&cursorCharset, curptr);
	}

//
// copy IDL_long setStmtOptionsLength
//

	IDL_long_copy(&setStmtOptionsLength, curptr);
	if (setStmtOptions != NULL)
	{
//
// copy IDL_string setStmtOptions
//
		IDL_charArray_copy(setStmtOptions, curptr);
	}


	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}

	return CEE_SUCCESS;

} /* odbc_SQLDrvr_Fetch_param_pst_() */

CEE_status
odbc_SQLDrvr_Prepare_param_pst_(
			CInterface* pSystem
			, char*& buffer
			, long& message_length
			, /* In    */ DIALOGUE_ID_def dialogueId
			, /* In    */ IDL_long sqlAsyncEnable
			, /* In    */ IDL_long queryTimeout
			, /* In    */ IDL_long sqlStmtType
			, /* In    */ const IDL_char *stmtLabel
			, /* In    */ IDL_long stmtCharset
			, /* In    */ IDL_string cursorName
			, /* In    */ IDL_long cursorCharset
			, /* In    */ const IDL_char *moduleName
			, /* In    */ IDL_long moduleCharset
			, /* In    */ IDL_long_long moduleTimestamp
			, /* In    */ IDL_string sqlString
			, /* In    */ IDL_long sqlStringCharset
			, /* In    */ IDL_string setStmtOptions
			, /* In    */ const IDL_char *stmtExplainLabel
			, /* In    */ IDL_short stmtType
            , /* In    */ IDL_long maxRowsetSize
			, /* In    */ IDL_long holdableCursor
)
{
	IDL_long wlength = 0;

	IDL_long stmtLabelLength        = 0;
   	 IDL_long cursorNameLength       = 0;
    	IDL_long moduleNameLength       = 0;
	IDL_long sqlStringLength        = 0;
	IDL_long setStmtOptionsLength   = 0;
	IDL_long stmtExplainLabelLength = 0;
	IDL_long transactionIDLength    = 0;

	IDL_char *curptr  = NULL;


//
// calculate length of the buffer for each parameter
//


//
//  length of IDL_Long dialogueId
//
	wlength += sizeof(dialogueId);
//
// length of IDL_long sqlAsyncEnable
//
// NOTE - this field is not used by the server, so we will overload the value of 
//        holdableCursor in this field for now for drvr/srvr compatibility. 
//        In the future, if we do need to use the sqlAyncEnable, we need to take care
//        of holdableCursor
//
	wlength += sizeof (sqlAsyncEnable);
//
// length of IDL_long queryTimeout
//
	wlength += sizeof (queryTimeout);
//
// length of IDL_long stmtType
//
	wlength += sizeof (stmtType);
//
// length of IDL_long sqlStmtType
//
	wlength += sizeof (sqlStmtType);


//
// length of IDL_long stmtLength
// length of IDL_char *stmtLabel
// length of IDL_long stmtCharset
// 
	wlength += sizeof(stmtLabelLength);
	if (stmtLabel != NULL)
	{
        stmtLabelLength = strlen(stmtLabel) + 1;
		wlength += stmtLabelLength;
		wlength += sizeof(stmtCharset);
	}

//
// length of IDL_long cursorLength
// length IDL_string cursorName
// length of IDL_long cursorCharset
//
	wlength += sizeof(cursorNameLength);
	if (cursorName != NULL)
	{
		cursorNameLength = strlen(cursorName) + 1;
		wlength += cursorNameLength;
		wlength += sizeof(cursorCharset);
	}

//
// length of IDL_long moduleNameLength
// length of IDL_char *moduleName
// length of IDL_long moduleCharset
// length of IDL_long_long moduleTimestamp
// 
	wlength += sizeof(moduleNameLength);
	if (moduleName != NULL)
	{
		moduleNameLength = strlen(moduleName) + 1;
		wlength += moduleNameLength;
		wlength += sizeof(moduleCharset);
		wlength += sizeof(moduleTimestamp);
	}

//
// length of IDL_long sqlStringLength
// length of IDL_string sqlString
// length of IDL_long sqlStringCharset
// 
	wlength += sizeof(sqlStringLength);
	if (sqlString != NULL)
	{
		sqlStringLength = strlen(sqlString) + 1;
		wlength += sqlStringLength;
		wlength += sizeof(sqlStringCharset);
	}


//
// length of IDL_long setStmtOptionsLength
// length of IDL_string setStmtOptions
// 
	wlength += sizeof(setStmtOptionsLength);
	if (setStmtOptions != NULL)
	{
		setStmtOptionsLength = strlen(setStmtOptions) + 1;
		wlength += setStmtOptionsLength;
	}


//
// length of IDL_long stmtExplainLabelLength
// length of IDL_string stmtExplainLabel
// 
	wlength += sizeof(stmtExplainLabelLength);
	if (stmtExplainLabel != NULL)
	{
		stmtExplainLabelLength = strlen(stmtExplainLabel) + 1;
		wlength += stmtExplainLabelLength;
	}


//
// length of IDL_long maxRowsetSize
//
	wlength += sizeof (maxRowsetSize);

//
//  length of IDL_long transactionIDLength
//
//  NOTE - currently only JDBC sets txnID, so no need to reserve space for txnID
	wlength += sizeof(transactionIDLength);

//
// message length
//
	message_length = wlength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	curptr = buffer;

//
//  Swap all input
//  

	if(pSystem->swap() == SWAP_YES)
	{
			LONG_swap(&dialogueId);
			LONG_swap(&sqlAsyncEnable);
			LONG_swap(&queryTimeout);
			SHORT_swap(&stmtType);
			LONG_swap(&sqlStmtType);
			LONG_swap(&stmtLabelLength);
			LONG_swap(&stmtCharset);
			LONG_swap(&cursorNameLength);
			LONG_swap(&cursorCharset);
			LONG_swap(&moduleNameLength);
			LONG_swap(&moduleCharset);
			LONGLONG_swap(&moduleTimestamp);
			LONG_swap(&sqlStringLength);
			LONG_swap(&sqlStringCharset);
			LONG_swap(&setStmtOptionsLength);
			LONG_swap(&stmtExplainLabelLength);
			LONG_swap(&maxRowsetSize);
			LONG_swap(&holdableCursor);
	}

//
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long_copy(&dialogueId, curptr);

//
// copy IDL_long sqlAsyncEnable
//
// NOTE - overloading the value of holdableCursor using sqlAsyncEnable field.
//
//	IDL_long_copy(&sqlAsyncEnable, curptr);
        IDL_long_copy(&holdableCursor, curptr);

//
// copy IDL_long queryTimeout
//
	IDL_long_copy(&queryTimeout, curptr);

//
// copy IDL_long stmtType
//
	IDL_short_copy(&stmtType, curptr);

//
// copy IDL_long sqlStmtType
//
	IDL_long_copy(&sqlStmtType, curptr);

//
// copy IDL_long stmtLabelLength
//
	IDL_long_copy(&stmtLabelLength, curptr);

	if (stmtLabel != NULL)
	{
//
// copy IDL_char *stmtLabel
//
		IDL_charArray_copy(stmtLabel, curptr);

//
// copy IDL_long stmtCharset
//
		IDL_long_copy(&stmtCharset, curptr);
	}

//
// copy IDL_long cursorLength
//
	IDL_long_copy(&cursorNameLength, curptr);

	if (cursorName != NULL)
	{
//
// copy IDL_string cursorName
//
		IDL_charArray_copy(cursorName, curptr);

//
// copy IDL_long cursorCharset
//
		IDL_long_copy(&cursorCharset, curptr);
	}

//
// copy IDL_long moduleNameLength
//
	IDL_long_copy(&moduleNameLength, curptr);

	if (moduleName != NULL)
	{
//
// copy IDL_char *moduleName
//
		IDL_charArray_copy(moduleName, curptr);
//
// copy IDL_long moduleCharset
//
		IDL_long_copy(&moduleCharset, curptr);
//
// copy IDL_long_long moduleTimestamp
//
		IDL_long_long_copy(&moduleTimestamp, curptr);
	}

//
// copy IDL_long sqlStringLength
//
	IDL_long_copy(&sqlStringLength, curptr);

	if (sqlString != NULL)
	{
//
// copy IDL_string sqlString
//
		IDL_charArray_copy(sqlString, curptr);
//
// copy IDL_long sqlStringCharset
//
		IDL_long_copy(&sqlStringCharset, curptr);
	}

 //
// copy IDL_long setStmtOptionsLength
//
	IDL_long_copy(&setStmtOptionsLength, curptr);

	if (setStmtOptions != NULL)
	{
//
// copy IDL_string setStmtOptions
//
		IDL_charArray_copy(setStmtOptions, curptr);
	}

//
// copy IDL_long stmtExplainLabelLength
//
	IDL_long_copy(&stmtExplainLabelLength, curptr);

	if (stmtExplainLabel != NULL)
	{
//
// copy IDL_string stmtExplainLabel
//
		IDL_charArray_copy(stmtExplainLabel, curptr);
	}


// copy IDL_long maxRowsetSize
//
	IDL_long_copy(&maxRowsetSize, curptr);

//
// copy IDL_long transactionIDLength
//
	IDL_long_copy(&transactionIDLength, curptr);

// Sanity Check
	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}

	return CEE_SUCCESS;

} // odbc_SQLDrvr_Prepare_param_pst_()


CEE_status
odbc_SQLDrvr_Close_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_unsigned_short freeResourceOpt
  )
{
	IDL_long wlength;
	IDL_char *curptr;

	wlength = 0;
	IDL_long stmtLabelLength = 0;
	//
	// calculate length of the buffer for each parameter
	//
	//

	// 1st Parameter
	// length of dialogueId
	wlength += sizeof(dialogueId);

	//
	// length of IDL_char *stmtLabel
	// 
	wlength += sizeof(stmtLabelLength);
	if (stmtLabel != NULL)
	{
		stmtLabelLength = strlen(stmtLabel)+1;
		wlength += stmtLabelLength;
	} 

	//
	// length of IDL_unsigned_short freeResourceOpt
	//
	wlength += sizeof (freeResourceOpt);

    // Mesage Length
	message_length = wlength;

	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}
	curptr = buffer;
	

    // Swap and then copy the values
	if(pSystem->swap() == SWAP_YES)
	{
		LONG_swap(&dialogueId);
		LONG_swap(&stmtLabelLength);
		USHORT_swap(&freeResourceOpt);
	}

	// Copy 1st Parameter
	// copy dialogueId
	IDL_long_copy(&dialogueId, curptr);
	
	//
	// copy IDL_char *stmtLabel
	//
	IDL_long_copy(&stmtLabelLength, curptr);
	if (stmtLabel != NULL) 
	{
		IDL_charArray_copy(stmtLabel, curptr);
	}

	//
	// copy IDL_unsigned_short freeResourceOpt
	//
	IDL_unsigned_short_copy(&freeResourceOpt, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}

	return CEE_SUCCESS;

} /* odbc_SQLDrvr_Close_param_pst_() */

CEE_status
odbc_SQLDrvr_EndTransaction_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
	    , /* In    */ DIALOGUE_ID_def dialogueId
	    , /* In    */ IDL_unsigned_short transactionOpt
)
{
    IDL_long wlength = 0;
	IDL_char *curptr;

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
	message_length = wlength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	curptr = buffer;

	if(pSystem->swap() == SWAP_YES)
	{
		LONG_swap(&dialogueId);
		USHORT_swap(&transactionOpt);
	}

//
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long_copy(&dialogueId, curptr);

//
// copy IDL_short transactionOpt
//
	IDL_unsigned_short_copy(&transactionOpt, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}

	return CEE_SUCCESS;

} // odbc_SQLDrvr_EndTransaction_param_pst_()

CEE_status
odbc_SQLDrvr_GetSQLCatalogs_param_pst_(
 		  CInterface* pSystem
		, IDL_char*& buffer
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
	IDL_long wlength = 0;
	IDL_char *curptr;

	IDL_long stmtLabelLen =0;
	IDL_long catalogNmLen =0;
	IDL_long schemaNmLen =0;
	IDL_long tableNmLen =0;
	IDL_long tableTypeListLen =0;
	IDL_long columnNmLen =0;
	IDL_long fkcatalogNmLen =0;
	IDL_long fkschemaNmLen = 0;
	IDL_long fktableNmLen = 0;

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
	wlength += sizeof (stmtLabelLen);
	if (stmtLabel != NULL)
	{
		stmtLabelLen = strlen(stmtLabel)+1;
		wlength += stmtLabelLen;
	}

//
// length of IDL_short APIType
//
	wlength += sizeof (APIType);

//
// length of IDL_char *catalogNm
// 
	wlength += sizeof(catalogNmLen);
	if (catalogNm != NULL)
	{
		catalogNmLen = strlen(catalogNm) +1;
		wlength += catalogNmLen;
	}

//
// length of IDL_char *schemaNm
// 
	wlength += sizeof(schemaNmLen);
	if (schemaNm != NULL)
	{
		schemaNmLen = strlen(schemaNm) +1;
		wlength += schemaNmLen;
	}

//
// length of IDL_char *tableNm
// 
	wlength += sizeof(tableNmLen);
	if (tableNm != NULL)
	{
		tableNmLen = strlen(tableNm)+1;
		wlength += tableNmLen;
	}

//
// length of IDL_char *tableTypeList
//
	wlength += sizeof(tableTypeListLen);
	if (tableTypeList != NULL)
	{
		tableTypeListLen = strlen(tableTypeList)+1;
		wlength += tableTypeListLen;
	}

//
// length of IDL_char *columnNm
// 
	wlength += sizeof(columnNmLen);
	if (columnNm != NULL)
	{
		columnNmLen = strlen(columnNm)+1;
		wlength += columnNmLen;
	}

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
	wlength += sizeof(fkcatalogNmLen);
	if (fkcatalogNm != NULL)
	{
		fkcatalogNmLen = strlen(fkcatalogNm)+1;
		wlength += fkcatalogNmLen;
	}

//
// length of IDL_char *fkschemaNm
//
	wlength += sizeof(fkschemaNmLen);
	if (fkschemaNm != NULL)
	{
		fkschemaNmLen = strlen(fkschemaNm)+1;
		wlength += fkschemaNmLen;
	}

//
// length of IDL_char *fktableNm
//
	wlength += sizeof(fktableNmLen);
	if (fktableNm != NULL)
	{
		fktableNmLen = strlen(fktableNm)+1;
		wlength += fktableNmLen;
	}


//
// message length
//
	message_length = wlength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	curptr = buffer;

	if(pSystem->swap() == SWAP_YES)
	{
		LONG_swap(&dialogueId);
		SHORT_swap(&APIType);
		LONG_swap(&columnType);
		LONG_swap(&rowIdScope);
		LONG_swap(&nullable);
		LONG_swap(&uniqueness);
		LONG_swap(&accuracy);
		SHORT_swap(&sqlType);
		ULONG_swap(&metadataId);
		LONG_swap(&stmtLabelLen);
		LONG_swap(&catalogNmLen);
		LONG_swap(&schemaNmLen);
		LONG_swap(&tableNmLen);
		LONG_swap(&tableTypeListLen);
		LONG_swap(&columnNmLen);
		LONG_swap(&fkcatalogNmLen);
		LONG_swap(&fkschemaNmLen);
		LONG_swap(&fktableNmLen);
	}

//
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long_copy(&dialogueId, curptr);

//
// copy IDL_char *stmtLabel
//
	IDL_long_copy(&stmtLabelLen, curptr);
	if (stmtLabel != NULL)
		IDL_charArray_copy(stmtLabel, curptr);
	
//
// copy IDL_short APIType
//
	IDL_short_copy(&APIType, curptr);

//
// copy IDL_char *catalogNm
//
	IDL_long_copy(&catalogNmLen, curptr);
	if (catalogNm != NULL)
		IDL_charArray_copy(catalogNm, curptr);

//
// copy IDL_char *schemaNm
//
	IDL_long_copy(&schemaNmLen, curptr);
	if (schemaNm != NULL)
		IDL_charArray_copy(schemaNm, curptr);

//
// copy IDL_char *tableNm
//
	IDL_long_copy(&tableNmLen, curptr);
	if (tableNm != NULL)
		IDL_charArray_copy(tableNm, curptr);

//
// copy IDL_char *tableTypeList
//
	IDL_long_copy(&tableTypeListLen, curptr);
	if (tableTypeList != NULL)
		IDL_charArray_copy(tableTypeList, curptr);

//
// copy IDL_char *columnNm
//
	IDL_long_copy(&columnNmLen, curptr);
	if (columnNm != NULL)
		IDL_charArray_copy(columnNm, curptr);

//
// copy IDL_long columnType
//
	IDL_long_copy(&columnType, curptr);

//
// copy IDL_long rowIdScope
//
	IDL_long_copy(&rowIdScope, curptr);

//
// copy IDL_long nullable
//
	IDL_long_copy(&nullable, curptr);

//
// copy IDL_long uniqueness
//
	IDL_long_copy(&uniqueness, curptr);

//
// copy IDL_long accuracy
//
	IDL_long_copy(&accuracy, curptr);

//
// copy IDL_short sqlType
//
	IDL_short_copy(&sqlType, curptr);

//
// copy IDL_unsigned_long metadataId
//
	IDL_unsigned_long_copy(&metadataId, curptr);

//
// copy IDL_char *fkcatalogNm
//
	IDL_long_copy(&fkcatalogNmLen, curptr);
	if (fkcatalogNm != NULL)
		IDL_charArray_copy(fkcatalogNm, curptr);

//
// copy IDL_char *fkschemaNm
//
	IDL_long_copy(&fkschemaNmLen, curptr);
	if (fkschemaNm != NULL)
		IDL_charArray_copy(fkschemaNm, curptr);

//
// copy IDL_char *fktableNm
//
	IDL_long_copy(&fktableNmLen, curptr);
	if (fktableNm != NULL)
		IDL_charArray_copy(fktableNm, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}

	return CEE_SUCCESS;

} // odbc_SQLDrvr_GetSQLCatalogs_param_pst_()


CEE_status
odbc_SQLDrvr_Execute_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ IDL_long sqlAsyncEnable
		, /* In    */ IDL_long holdableCursor		
		, /* In    */ IDL_long queryTimeout
		, /* In    */ IDL_long inputRowCnt
		, /* In    */ IDL_long maxRowsetSize
		, /* In    */ IDL_long sqlStmtType
		, /* In    */ Long stmtHandle
		, /* In    */ IDL_long stmtType
		, /* In    */ IDL_string sqlString
		, /* In    */ IDL_long sqlStringCharset
		, /* In    */ IDL_string cursorName
		, /* In    */ IDL_long cursorCharset
		, /* In    */ IDL_string stmtLabel
		, /* In    */ IDL_long stmtCharset
		, /* In    */ IDL_string stmtExplainLabel
		, /* In    */ IDL_long inValuesLength
		, /* In    */ BYTE *inValues
		, /* In    */ const SQLValueList_def *inputValueList
)
{

	IDL_char *curptr = NULL;
	IDL_long wlength =  0;
	IDL_long stmtLabelLength        = 0;
    	IDL_long cursorNameLength       = 0;
	IDL_long sqlStringLength        = 0;
	IDL_long setStmtOptionsLength   = 0;
	IDL_long stmtExplainLabelLength = 0;
	IDL_long transactionIDLength	= 0; // always since JDBC is the only one that will ever use this

	wlength = 0;
//
// calculate length of the buffer for each parameter
//

//
//  length of DIALOGUE_ID_def dialogueId
//
	wlength += sizeof(dialogueId);

//
// length of IDL_long sqlAsyncEnable
//
// NOTE - sqlAsynEnable is currently not used by the server, we will use this field to
//        overload the value of holdableCursor.
//        In the future, if this field is used, we need to take care of holdableCursor as well
	wlength += sizeof(sqlAsyncEnable);

//
// length of IDL_long queryTimeout
//
	wlength += sizeof(queryTimeout);

//
// length of IDL_long inputRowCnt
//
	wlength += sizeof(inputRowCnt);

//
// length of IDL_long maxRowsetSize
//
	wlength += sizeof(maxRowsetSize);

//
// length of IDL_long sqlStmtType
//
	wlength += sizeof(sqlStmtType);

//
// length of IDL_long stmtHandle
// 
	wlength += sizeof(stmtHandle);

//
// length of IDL_long stmtType
//
	wlength += sizeof(stmtType);

//
// length of IDL_long sqlStringLength
// length of IDL_string sqlString
// length of IDL_long sqlStringCharset
// 
	wlength += sizeof(sqlStringLength);
	if (sqlString != NULL)
	{
		sqlStringLength = strlen(sqlString) + 1;
		wlength += sqlStringLength;
		wlength += sizeof(sqlStringCharset);
	}


//
// length of IDL_long cursorLength
// length IDL_string cursorName
// length of IDL_long cursorCharset
//
	wlength += sizeof(cursorNameLength);
	if (cursorName != NULL)
	{
		cursorNameLength = strlen(cursorName)+1;
		wlength += cursorNameLength;
		wlength += sizeof(cursorCharset);
	}

//
// length of IDL_long stmtLabelLength
// length of IDL_char *stmtLabel
// length of IDL_long stmtCharset
// 
	wlength += sizeof(stmtLabelLength);
	if (stmtLabel != NULL)
	{
        stmtLabelLength = strlen(stmtLabel) + 1;
		wlength += stmtLabelLength;
		wlength += sizeof(stmtCharset);
	}

//
// length of IDL_long stmtExplainLabelLength
// length of IDL_string stmtExplainLabel
// 
	wlength += sizeof(stmtExplainLabelLength);
	if (stmtExplainLabel != NULL)
	{
		stmtExplainLabelLength = strlen(stmtExplainLabel) + 1;
		wlength += stmtExplainLabelLength;
	}


//
// length of IDL_long inValuesLength
// length of BYTE *inValues
//
	wlength += sizeof (inValuesLength);
	wlength += inValuesLength;


//
//  length of IDL_long transactionIDLength
//
	wlength += sizeof(transactionIDLength);

//
// message length
//
	message_length = wlength;
	buffer = pSystem->w_allocate(message_length);
	if (buffer == NULL)
	{
		return CEE_ALLOCFAIL;
	}

	curptr = buffer;

//  Swap

	if(pSystem->swap() == SWAP_YES)
	{
		LONG_swap(&dialogueId);
		LONG_swap(&sqlAsyncEnable);
		LONG_swap(&queryTimeout);
		LONG_swap(&inputRowCnt);
		LONG_swap(&maxRowsetSize);
		LONG_swap(&sqlStmtType);
		LONG_swap(&stmtHandle);
		LONG_swap(&stmtType);
		LONG_swap(&sqlStringLength);
		LONG_swap(&sqlStringCharset);
		LONG_swap(&cursorNameLength);
		LONG_swap(&cursorCharset);
		LONG_swap(&stmtLabelLength);
		LONG_swap(&stmtCharset);
		LONG_swap(&stmtExplainLabelLength);
		LONG_swap(&holdableCursor);
	}

//
// copy DIALOGUE_ID_def dialogueId
//
	IDL_long_copy(&dialogueId, curptr);

//
// copy IDL_long sqlAsyncEnable
//
// NOTE - overload value of holdableCuror in this field 
//
//	IDL_long_copy(&sqlAsyncEnable, curptr);
	IDL_long_copy(&holdableCursor, curptr);

//
// copy IDL_long queryTimeout
//
	IDL_long_copy(&queryTimeout, curptr);

//
// copy IDL_long inputRowCnt
//
	IDL_long_copy(&inputRowCnt, curptr);

//
// copy IDL_long maxRowsetSize
//
	IDL_long_copy(&maxRowsetSize, curptr);


//
// copy IDL_long sqlStmtType
//
	IDL_long_copy(&sqlStmtType, curptr);

//
// copy IDL_long stmtHandle
//
	Long_copy(&stmtHandle, curptr);

//
// copy IDL_long stmtType
//
	IDL_long_copy(&stmtType, curptr);

//
// copy IDL_long sqlStringLength
//
	IDL_long_copy(&sqlStringLength, curptr);

	if (sqlString != NULL)
	{
//
// copy IDL_string sqlString
//
		IDL_charArray_copy(sqlString, curptr);
//
// copy IDL_long sqlStringCharset
//
		IDL_long_copy(&sqlStringCharset, curptr);
	}


//
// copy IDL_long cursorLength
//
	IDL_long_copy(&cursorNameLength, curptr);

	if (cursorName != NULL)
	{
//
// copy IDL_string cursorName
//
		IDL_charArray_copy(cursorName, curptr);

//
// copy IDL_long cursorCharset
//
		IDL_long_copy(&cursorCharset, curptr);
	}

//
// copy IDL_long stmtLabelLength
//
	IDL_long_copy(&stmtLabelLength, curptr);

	if (stmtLabel != NULL)
	{
//
// copy IDL_char *stmtLabel
//
		IDL_charArray_copy(stmtLabel, curptr);

//
// copy IDL_long stmtCharset
//
		IDL_long_copy(&stmtCharset, curptr);
	}

//
// copy IDL_long stmtExplainLabelLength
//
	IDL_long_copy(&stmtExplainLabelLength, curptr);

	if (stmtExplainLabel != NULL)
	{
//
// copy IDL_string stmtExplainLabel
//
		IDL_charArray_copy(stmtExplainLabel, curptr);
	}

//
// copy IDL_long inValuesLength
//
	LONG_swap(&inValuesLength,pSystem->swap());
	IDL_long_copy(&inValuesLength, curptr);
	LONG_swap(&inValuesLength,pSystem->swap());

	if (inValues != NULL)
	{
//
// copy BYTE *inputValueList
//
		IDL_byteArray_copy(inValues, inValuesLength, curptr);
	}

//
// copy IDL_long transactionIDLength
//
	IDL_long_copy(&transactionIDLength, curptr);

	if (curptr > buffer + message_length)
	{
		return CEE_INTERNALFAIL;
	}

	return CEE_SUCCESS;

} /* odbc_SQLDrvr_Execute_param_pst_() */

CEE_status
odbc_SQLDrvr_ExtractLob_param_pst_(
    /* In    */ CInterface * pSystem
  , /* In    */ IDL_char * & buffer
  , /* In    */ long        & message_length
  , /* In    */ IDL_long     extractType
  , /* In    */ IDL_string   lobHandle
  , /* In    */ IDL_long     lobHandleLen
  , /* In    */ IDL_long     lobHandleCharset
  , /* In    */ IDL_long     extractlen
 )
{
    IDL_char  *curptr = NULL;
    IDL_long  wlength = 0;
    IDL_long  charSet = 0;

    //extractType
    wlength += sizeof(IDL_short);

    //lobHandleLen
    wlength += sizeof(IDL_long);
    if (lobHandle != NULL)
    {
        wlength += lobHandleLen;
        wlength += sizeof(IDL_long);
    }

    wlength += sizeof(IDL_long);

    message_length = wlength;
    buffer = pSystem->w_allocate(message_length);
    if (buffer == NULL)
    {
        return CEE_ALLOCFAIL;
    }
    curptr = buffer;

    if (pSystem->swap() == SWAP_YES)
    {
        LONG_swap(&extractType);
        LONG_swap(&lobHandleLen);
        LONG_swap(&charSet);
        LONG_swap(&extractlen);
    }

    IDL_long_copy(&extractType, curptr);

    IDL_long_copy(&lobHandleLen, curptr);
    if (lobHandle != NULL)
    {
        IDL_charArray_copy(lobHandle, curptr);
        IDL_long_copy(&charSet, curptr);
    }

    IDL_long_copy(&extractlen, curptr);

    return CEE_SUCCESS;
}

CEE_status
odbc_SQLDrvr_UpdateLob_param_pst_(
    /* In    */ CInterface * pSystem
  , /* In    */ IDL_char * & buffer
  , /* In    */ long        & message_length
  , /* In    */ IDL_long     updataType
  , /* In    */ IDL_string   lobHandle
  , /* In    */ IDL_long     lobHandleLen
  , /* In    */ IDL_long     lobHandleCharset
  , /* In    */ IDL_long_long     totalLength
  , /* In    */ IDL_long_long     offset
  , /* In    */ BYTE *        data
  , /* In    */ IDL_long_long pos
  , /* In    */ IDL_long_long length
)
{
    IDL_char  *curptr = NULL;
    IDL_long   wlength = 0;
    IDL_long   lobHandleLength = 0;

    wlength += sizeof(IDL_long);

    wlength += sizeof(lobHandleLength);
    if (lobHandle != NULL)
    {
        lobHandleLength = strlen(lobHandle) + 1;
        wlength += lobHandleLength;
        wlength += sizeof(lobHandleCharset);
    }

    wlength += sizeof(IDL_long_long) * 3;
    wlength += length;

    message_length = wlength;
    buffer = pSystem->w_allocate(message_length);
    if (buffer == NULL)
    {
        return CEE_ALLOCFAIL;
    }

    curptr = buffer;

    if (pSystem->swap() == SWAP_YES)
    {
        LONGLONG_swap(&totalLength);
        LONGLONG_swap(&offset);
        LONGLONG_swap(&length);
    }

    IDL_long_copy(&updataType, curptr);
    IDL_long_copy(&lobHandleLength, curptr);
    if (lobHandle != NULL)
    {
        IDL_charArray_copy(lobHandle, curptr);
        IDL_long_copy(&lobHandleCharset, curptr);
    }

    IDL_long_long_copy(&totalLength, curptr);
    IDL_long_long_copy(&offset, curptr);
    IDL_long_long_copy(&length, curptr);

    IDL_byteArray_copy(data, length, curptr);

    return CEE_SUCCESS;
}
/************************************************************************************************************
 *                                                                                                          *
 * Keeping these functions around for the collapsed driver - get rid of these when it is not needed anymore *
 *                                                                                                          *
 ************************************************************************************************************/

