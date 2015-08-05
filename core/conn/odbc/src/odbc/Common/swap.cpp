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

#include <platform_ndcs.h>
#include <cee.h>
#include <idltype.h>
#include "DrvrSrvr.h"
#include "TransportBase.h"
#include "marshaling.h"
#include "swap.h"

#ifndef NSK_PLATFORM
#define SRVRTRACE_ENTER(name)
#define SRVRTRACE_EXIT(name)
#endif

#ifdef __OBSOLETE
void swapPointers(char* buffer, short number_of_param)
{
	SRVRTRACE_ENTER(FILE_SWAP+1);
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
	SRVRTRACE_EXIT(FILE_SWAP+1);
}
#endif /* __OBSOLETE */

void SHORT_swap(short* pshort,char swap)
{
	if(swap == SWAP_NO) return;	

	SRVRTRACE_ENTER(FILE_SWAP+2);
	if (*pshort != 0 && *pshort != -1)
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
	SRVRTRACE_EXIT(FILE_SWAP+2);
}

void USHORT_swap(unsigned short* pushort, char swap)
{
	if(swap == SWAP_NO) return;	

	SRVRTRACE_ENTER(FILE_SWAP+3);
	if (*pushort != 0 && *pushort != 0xFFFF)
	{
		union
		{
			char c[2];
			unsigned short i;
		} u;
		char hold;
		u.i = *pushort;
		hold = u.c[0];
		u.c[0] = u.c[1];
		u.c[1] = hold;
		*pushort = u.i;
	}
	SRVRTRACE_EXIT(FILE_SWAP+3);
}

/*
*	for stmtHandle 64bit
*/
void Long_swap(Long* plong, char swap)
{
        if(swap == SWAP_NO) return;

        SRVRTRACE_ENTER(FILE_SWAP+4);
        if (*plong != 0 && *plong != -1 )
        {
                union
                {
                        char c[4];
                        unsigned int l;
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
        SRVRTRACE_EXIT(FILE_SWAP+4);

}

#if defined(_WIN32) || defined(_WIN64)
void LONG_swap(long* plong, char swap)
#else
void LONG_swap(int* plong, char swap)
#endif
{
	if(swap == SWAP_NO) return;	

	SRVRTRACE_ENTER(FILE_SWAP+4);
	if (*plong != 0 && *plong != -1 )
	{
		union
		{
			char c[4];
			unsigned int l;
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
	SRVRTRACE_EXIT(FILE_SWAP+4);

}
#if defined(_WIN32) || defined(_WIN64)
void ULONG_swap(unsigned long* pulong, char swap)
#else
void ULONG_swap(unsigned int* pulong, char swap)
#endif
{
	if(swap == SWAP_NO) return;	

	SRVRTRACE_ENTER(FILE_SWAP+5);
	if (*pulong != 0 && *pulong != 0xFFFFFFFF)
	{
		union
		{
			char c[4];
			unsigned int l;
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
	SRVRTRACE_EXIT(FILE_SWAP+5);

}

void LONGLONG_swap(long long* plonglong)
{
	SRVRTRACE_ENTER(FILE_SWAP+7);
	if (*plonglong != 0)
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
	SRVRTRACE_EXIT(FILE_SWAP+7);
}

void ULONGLONG_swap(unsigned long long* plonglong)
{
	SRVRTRACE_ENTER(FILE_SWAP+7);
	if (*plonglong != 0)
	{
		union
		{
			char c[8];
			unsigned long long ll;
		} u;
		char llc[8];
		u.ll = *plonglong;
		memcpy(llc, u.c, 8);
		for(int i=0; i<8; i++)
			u.c[i] = llc[7-i];
		*plonglong = u.ll;
	}
	SRVRTRACE_EXIT(FILE_SWAP+7);
}


void VERSION_LIST_swap(char* buffer, VERSION_LIST_def* pverlist)
{
	SRVRTRACE_ENTER(FILE_SWAP+8);

	if (pverlist->_buffer != NULL && pverlist->_length != 0 )
	{
		VERSION_def *_buffer = pverlist->_buffer;
		IDL_unsigned_long _length = pverlist->_length;

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
	SRVRTRACE_EXIT(FILE_SWAP+8);
}

#if defined(_WIN32) || defined(_WIN64)
void HEADER_swap(HEADER* header)
{
	SRVRTRACE_ENTER(FILE_SWAP+9);
	SHORT_swap(&header->operation_id);
	LONG_swap((long*)&header->dialogueId);
	ULONG_swap((unsigned long*)&header->total_length);
	ULONG_swap((unsigned long*)&header->cmp_length);
	LONG_swap((long*)&header->hdr_type);
	ULONG_swap((unsigned long*)&header->signature);
	ULONG_swap((unsigned long*)&header->version);
	SHORT_swap(&header->error);
	SHORT_swap(&header->error_detail);
	SRVRTRACE_EXIT(FILE_SWAP+9);
}
#else
void HEADER_swap(HEADER* header)
{
	SRVRTRACE_ENTER(FILE_SWAP+9);
	SHORT_swap(&header->operation_id);
	LONG_swap(&header->dialogueId);
	ULONG_swap(&header->total_length);
	ULONG_swap(&header->cmp_length);
	LONG_swap((int*)&header->hdr_type);
	ULONG_swap(&header->signature);
	ULONG_swap(&header->version);
	SHORT_swap(&header->error);
	SHORT_swap(&header->error_detail);
	SRVRTRACE_EXIT(FILE_SWAP+9);
}
#endif

void SQL_WARNING_OR_ERROR_swap(BYTE *WarningOrError, IDL_long WarningOrErrorLengthCheck,char swap)
{
	IDL_long msg_total_len = 0;
	
	IDL_long numConditions = 0;
	IDL_long errorTextLen = 0;

	unsigned char *curptr;
	int i;
		
	curptr = (unsigned char *)WarningOrError;

	//swap numConditions and then pull it out, for indexing
	LONG_swap((IDL_long*)(curptr+msg_total_len),swap);
	numConditions = *(IDL_long*)(curptr+msg_total_len);
	msg_total_len +=4;

	if (numConditions > 0)
	{
		for (i = 0; i < numConditions; i++)
		{
			//rowId= *(IDL_long*)(curptr+msg_total_len);
			LONG_swap((IDL_long*)(curptr+msg_total_len),swap);
			msg_total_len +=4;

			//sqlCode= *(IDL_long*)(curptr+msg_total_len);
			LONG_swap((IDL_long*)(curptr+msg_total_len),swap);
			msg_total_len +=4;

			//swap errorTextLen then grab it
			LONG_swap((IDL_long*)(curptr+msg_total_len),swap);
			errorTextLen= *(IDL_long*)(curptr+msg_total_len);
			msg_total_len +=4;
			msg_total_len +=errorTextLen;
			
			msg_total_len += 6 * (sizeof(char));//sizeof(sqlState);
		}
	}
	//if(msg_total_len!=WarningOrErrorLengthCheck) return appropriate error;
}
void ERROR_DESC_LIST_swap(char* buffer, ERROR_DESC_LIST_def *errorList)
{
	SRVRTRACE_ENTER(FILE_SWAP+10);

	if (errorList->_buffer != NULL && errorList->_length != 0 )
	{
		ERROR_DESC_def *_buffer = errorList->_buffer;
		IDL_unsigned_long _length = errorList->_length;

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
	SRVRTRACE_EXIT(FILE_SWAP+10);
}

void OUT_CONNECTION_CONTEXT_swap(char* buffer, OUT_CONNECTION_CONTEXT_def *outContext)
{
	SRVRTRACE_ENTER(FILE_SWAP+11);
	VERSION_LIST_swap(buffer, &outContext->versionList);
	SHORT_swap(&outContext->nodeId);
	ULONG_swap(&outContext->processId);
	SRVRTRACE_EXIT(FILE_SWAP+11);
}

void SQL_VALUE_LIST_swap(char* buffer, SQLValueList_def *valueList)
{
	SRVRTRACE_ENTER(FILE_SWAP+12);

	if (valueList->_buffer != NULL && valueList->_length != 0 )
	{
		SQLValue_def *_buffer = valueList->_buffer;
		IDL_unsigned_long _length = valueList->_length;

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
	SRVRTRACE_EXIT(FILE_SWAP+12);
}

void SQL_ITEM_DESC_LIST_swap(char* buffer, SQLItemDescList_def *itemList)
{
	SRVRTRACE_ENTER(FILE_SWAP+13);

	if (itemList->_buffer != NULL && itemList->_length != 0 )
	{
		SQLItemDesc_def *_buffer = itemList->_buffer;
		IDL_unsigned_long _length = itemList->_length;

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
	SRVRTRACE_EXIT(FILE_SWAP+13);
}
