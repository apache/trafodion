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
//
**********************************************************************/
/* -*-C++-*-
******************************************************************************
*
* File:         RuException.cpp
* Description:  Implementation of class CRUException
*
* Created:      12/29/1999
* Language:     C++
*
*
******************************************************************************
*/

#include "RuException.h"
#include "uofsIpcMessageTranslator.h"

//--------------------------------------------------------------------------//
//	CRUException::StoreData()
//--------------------------------------------------------------------------//
void CRUException::StoreData(CUOFsIpcMessageTranslator &translator)
{
	Int32 i;
	Int32 numErrors = GetNumErrors();
	translator.WriteBlock(&numErrors,sizeof(Int32));
	for (i=0;i<numErrors;i++)
	{
		Lng32 errorCode = GetErrorCode(i);
		translator.WriteBlock(&errorCode,sizeof(Lng32));
		
		// Load the resource, substitute the arguments etc.
		BuildErrorMsg(i);
		// Only now we know the exact (null-terminated) buffer length
		Int32 bufsize = GetErrorMsgLen(i);

		char *buffer = new char[bufsize];
		
		GetErrorMsg(i, buffer, bufsize);
#pragma nowarn(1506)   // warning elimination 
		Int32 strSize = strlen(buffer)+1;	// Can be smaller than bufsize
#pragma warn(1506)  // warning elimination 

		translator.WriteBlock(&strSize, sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
		translator.WriteBlock(buffer, strSize);
#pragma warn(1506)  // warning elimination 

		delete [] buffer;

		// Oblivious. The error message is already built.
		// StoreErrorParams(translator,i);
	}
}

//--------------------------------------------------------------------------//
//	CRUException::StoreErrorParams()
//--------------------------------------------------------------------------//
void CRUException::StoreErrorParams(CUOFsIpcMessageTranslator &translator,
									Int32 index)
{
	Int32 i;

	Int32 numLongParams = GetNumLongArguments(index);
	translator.WriteBlock(&numLongParams,sizeof(Int32));
	for (i=0;i<numLongParams;i++)
	{
		Lng32 errorCode = GetLongArgument(index,i);
		translator.WriteBlock(&errorCode,sizeof(Lng32));
	}

	Int32 numStrParams = GetNumStrArguments(index);
	translator.WriteBlock(&numStrParams,sizeof(Int32));
	for (i=0;i<numStrParams;i++)
	{
		const char *param = GetStrArgument(index,i);
#pragma nowarn(1506)   // warning elimination 
		Int32 strSize = strlen(param) + 1;
#pragma warn(1506)  // warning elimination 
		translator.WriteBlock(&strSize,sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
		translator.WriteBlock(param,strSize);
#pragma warn(1506)  // warning elimination 
	}
}

//--------------------------------------------------------------------------//
//	CRUException::LoadData()
//--------------------------------------------------------------------------//
void CRUException::LoadData(CUOFsIpcMessageTranslator &translator)
{
	Int32 i;
	Int32 numErrors;
	translator.ReadBlock(&numErrors,sizeof(Int32));
	for (i=0;i<numErrors;i++)
	{
		Lng32 errorCode;
		translator.ReadBlock(&errorCode,sizeof(Lng32));
		
		Int32 strSize;
		translator.ReadBlock(&strSize,sizeof(Int32));

		char *buffer = new char[strSize];
#pragma nowarn(1506)   // warning elimination 
		translator.ReadBlock(buffer,strSize);
#pragma warn(1506)  // warning elimination 
		SetError(errorCode,buffer);

		delete [] buffer;

		// Oblivious. The error message was built by StoreData().
		// LoadErrorParams(translator,i);
	}
}

//--------------------------------------------------------------------------//
//	CRUException::LoadErrorParams()
//--------------------------------------------------------------------------//
void CRUException::
LoadErrorParams(CUOFsIpcMessageTranslator &translator,
				Int32 index)
{
	Int32 i;

	Int32 numLongParams;
	translator.ReadBlock(&numLongParams,sizeof(Int32));
	for (i=0;i<numLongParams;i++)
	{
		Lng32 errorCode;
		translator.ReadBlock(&errorCode,sizeof(Lng32));
		AddArgument(errorCode);
	}

	Int32 numStrParams;
	translator.ReadBlock(&numStrParams,sizeof(Int32));
	for (i=0;i<numStrParams;i++)
	{
		Int32 strSize;
		translator.ReadBlock(&strSize,sizeof(Int32));

		char *buffer = new char[strSize];
#pragma nowarn(1506)   // warning elimination 
		translator.ReadBlock(buffer,strSize);
#pragma warn(1506)  // warning elimination 
		AddArgument(buffer);

		delete [] buffer;
	}
}
