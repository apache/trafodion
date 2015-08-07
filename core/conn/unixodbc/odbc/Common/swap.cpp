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

#include <windows.h>
#include <cee.h>
#include <idltype.h>
#include "TransportBase.h"
#include "marshaling.h"
#include "swap.h"

void swapPointers(char* buffer, short number_of_param)
{
	long* ptr;
	short i=0;

	for (long* mapptr = (long*)buffer; ; mapptr++, i++)
	{
		if (i>= number_of_param)
		{
			if (*mapptr == 0) break;
			ptr = (long*)(*mapptr + (long)buffer);
			POINTER_swap(ptr);
			POINTER_swap(mapptr);
		}
		else
			POINTER_swap(mapptr);
	}
}

void SHORT_swap(short* pshort)
{
	union
	{
		char c[2];
		unsigned short i;
	} u;
	char hold;
	u.i = *pshort;
	hold = u.c[0];
	u.c[0] = u.c[1];
	u.c[1] = hold;
	*pshort = u.i;
}

void USHORT_swap(unsigned short* pshort)
{
	union
	{
		char c[2];
		unsigned short i;
	} u;
	char hold;
	u.i = *pshort;
	hold = u.c[0];
	u.c[0] = u.c[1];
	u.c[1] = hold;
	*pshort = u.i;
}

void LONG_swap(long* plong)
{
	union
	{
		char c[4];
		unsigned long l;
	} u;
	char hold;
	u.l = *plong;
	hold = u.c[0];
	u.c[0] = u.c[3];
	u.c[3] = hold;
	hold = u.c[2];
	u.c[2] = u.c[1];
	u.c[1] = hold;
	*plong = u.l;
}
void ULONG_swap(unsigned long* pulong)
{
	union
	{
		char c[4];
		unsigned long l;
	} u;
	char hold;
	u.l = *pulong;
	hold = u.c[0];
	u.c[0] = u.c[3];
	u.c[3] = hold;
	hold = u.c[2];
	u.c[2] = u.c[1];
	u.c[1] = hold;
	*pulong = u.l;
}
void POINTER_swap(void* ppointer)
{
	ULONG_swap((unsigned long*)ppointer);
}

#ifndef NSK_PLATFORM
void LONGLONG_swap(__int64* plonglong)
{
	union
	{
		char c[8];
		unsigned __int64 ll;
	} u;
	char llc[8];
	u.ll = *plonglong;
	memcpy(llc, u.c, 8);
	for(int i=0; i<8; i++)
		u.c[i] = llc[7-i];
	*plonglong = u.ll;
}
#else
void LONGLONG_swap(long long* plonglong)
{
	union
	{
		char c[8];
		long long ll;
	} u;
	char llc[8];
	u.ll = *plonglong;
	memcpy(llc, u.c, 8);
	for(int i=0; i<8; i++)
		u.c[i] = llc[7-i];
	*plonglong = u.ll;
}
#endif

void VERSION_LIST_swap(char* buffer, VERSION_LIST_def* pverlist)
{
	VERSION_def *_buffer = pverlist->_buffer;
	IDL_unsigned_long _length = pverlist->_length;

	if (_buffer != NULL && _length != 0 )
	{
		VERSION_def *pver;
		ULONG_swap(&pverlist->_length);
		_buffer = (VERSION_def *)((long)_buffer + (long)buffer);

		for (unsigned int i=0; i<_length; i++)
		{
			pver = _buffer + i;
			SHORT_swap(&pver->componentId);
			SHORT_swap(&pver->majorVersion);
			SHORT_swap(&pver->minorVersion);
			ULONG_swap(&pver->buildId);		
		}
	}
}

void HEADER_swap(HEADER* header)
{
	SHORT_swap(&header->operation_id);
	LONG_swap(&header->dialogueId);
	ULONG_swap(&header->total_length);
	ULONG_swap(&header->cmp_length);
	LONG_swap((long*)&header->hdr_type);
	ULONG_swap(&header->signature);
	ULONG_swap(&header->version);
	SHORT_swap(&header->error);
	SHORT_swap(&header->error_detail);
}

void ERROR_DESC_LIST_swap(char* buffer, ERROR_DESC_LIST_def *errorList)
{
	ERROR_DESC_def *_buffer = errorList->_buffer;
	IDL_unsigned_long _length = errorList->_length;

	if (_buffer != NULL && _length != 0 )
	{
		ERROR_DESC_def *perr;
		ULONG_swap(&errorList->_length);
		_buffer = (ERROR_DESC_def *)((long)_buffer + (long)buffer);

		for (unsigned int i=0; i<_length; i++)
		{
			perr = _buffer + i;
			LONG_swap(&perr->rowId);
			LONG_swap(&perr->errorDiagnosticId);
			LONG_swap(&perr->sqlcode);
			LONG_swap(&perr->operationAbortId);
			LONG_swap(&perr->errorCodeType);
		}
	}
}

void OUT_CONNECTION_CONTEXT_swap(char* buffer, OUT_CONNECTION_CONTEXT_def *outContext)
{
	VERSION_LIST_swap(buffer, &outContext->versionList);
	SHORT_swap(&outContext->nodeId);
	ULONG_swap(&outContext->processId);
}

void SQL_VALUE_LIST_swap(char* buffer, SQLValueList_def *valueList)
{
	SQLValue_def *_buffer = valueList->_buffer;
	IDL_unsigned_long _length = valueList->_length;

	if (_buffer != NULL && _length != 0 )
	{
		SQLValue_def *pval;
		ULONG_swap(&valueList->_length);
		_buffer = (SQLValue_def *)((long)_buffer + (long)buffer);

		for (unsigned int i=0; i<_length; i++)
		{
			SQL_DataValue_def *pdataValue;
			pval = _buffer + i;

			LONG_swap(&pval->dataType);
			SHORT_swap(&pval->dataInd);
			LONG_swap(&pval->dataCharset);

			pdataValue = &pval->dataValue;
			ULONG_swap(&pdataValue->_length);
		}
	}
}

void SQL_ITEM_DESC_LIST_swap(char* buffer, SQLItemDescList_def *itemList)
{
	SQLItemDesc_def *_buffer = itemList->_buffer;
	IDL_unsigned_long _length = itemList->_length;

	if (_buffer != NULL && _length != 0 )
	{
		SQLItemDesc_def *pitem;
		ULONG_swap(&itemList->_length);
		_buffer = (SQLItemDesc_def *)((long)_buffer + (long)buffer);

		for (unsigned int i=0; i<_length; i++)
		{
			pitem = _buffer + i;

			LONG_swap(&pitem->version);
			LONG_swap(&pitem->dataType);
			LONG_swap(&pitem->datetimeCode);
			LONG_swap(&pitem->maxLen);
			SHORT_swap(&pitem->precision);
			SHORT_swap(&pitem->scale);
			LONG_swap(&pitem->ODBCDataType);
			SHORT_swap(&pitem->ODBCPrecision);
			LONG_swap(&pitem->SQLCharset);
			LONG_swap(&pitem->ODBCCharset);
			LONG_swap(&pitem->intLeadPrec);
			LONG_swap(&pitem->paramMode);
		}
	}
}
