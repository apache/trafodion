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
#include <marshaling.h>

//
//----------- input parameters ------------------------------
//
CEE_status decodeParameters(short count, long* param[], char* buffer, long length)
{
	char* pbuffer = buffer;
	long* parptr, *mapptr;
	int i;
	long start_data = 0;
	long end_data = length;
	long offset;

	memset (param, 0, count*4);

	for (start_data=count * 4 ;start_data < length; start_data += 4)
	{
		if ((long)buffer[start_data] == 0)
		{
			start_data += 4;
			break;
		}
	}
	if (start_data >= length)
		return -1;

	for (parptr = (long*)pbuffer, i=0; i < count; i++, parptr++)
	{
		if (*parptr != NULL)
		{
			offset = *(parptr);
			if (offset < start_data || offset >= end_data)
				return i+1;
			*(parptr) += (long)pbuffer;
			param[i] = (long*)*(parptr);
		}
		else
			param[i] = NULL;
	}

	for (mapptr = (long*)buffer + count; *mapptr != 0; mapptr++, i++)
	{
		offset =(long)*mapptr;
		if (offset < start_data || offset >= end_data)
			return i+1;
		parptr = (long*)(*mapptr + pbuffer);
		offset = *(parptr);
		if (offset < start_data || offset >= end_data)
			return i+1;
		*parptr += (long)pbuffer;
	}

	return CEE_SUCCESS;
}
//
//---------------- output parameters ---------------------------
//
//---------------- calculate the length ------------------------
//
void LIST_length( LIST_def* pname, long length, long& wlength, long& maplength)
{
	if (pname->_buffer != NULL && pname->_length > 0)
	{
		wlength += ( pname->_length * length);
		maplength += sizeof(long);
	}
}

void OCTET_length( OCTET_def* pname, long& wlength, long& maplength)
{
	if (pname->_buffer != NULL && pname->_length > 0)
	{
		wlength += pname->_length;
		maplength += sizeof(long);
	}
}

void STRING_length( IDL_string pname,  long& wlength, long& maplength)
{
	if (pname != NULL)
	{
		wlength += strlen(pname) + 1;
		maplength += sizeof(long);
	}
}

void ERROR_DESC_LIST_length( ERROR_DESC_LIST_def* pname,  long& wlength, long& maplength )
{
	wlength += sizeof(ERROR_DESC_LIST_def);

	LIST_length((LIST_def*) pname, sizeof(ERROR_DESC_def), wlength, maplength);

	if (pname->_buffer != NULL && pname->_length > 0)
	{
		ERROR_DESC_def* pED;
		for (unsigned int i = 0; i < pname->_length; i++)
		{
			pED = pname->_buffer + i;

			STRING_length( pED->errorText, wlength, maplength);
			STRING_length( pED->Param1, wlength, maplength);
			STRING_length( pED->Param2, wlength, maplength);
			STRING_length( pED->Param3, wlength, maplength);
			STRING_length( pED->Param4, wlength, maplength);
			STRING_length( pED->Param5, wlength, maplength);
			STRING_length( pED->Param6, wlength, maplength);
			STRING_length( pED->Param7, wlength, maplength);
		}
	}

}

void SRVR_CONTEXT_length( const SRVR_CONTEXT_def* pname, long& wlength, long& maplength )
{
	wlength += sizeof(SRVR_CONTEXT_def);
	ENV_DESC_LIST_length( &pname->envDescList, wlength, maplength );
	RES_DESC_LIST_length( &pname->resDescList, wlength, maplength );

}

void DATASOURCE_CFG_LIST_length( const DATASOURCE_CFG_LIST_def* pname,  long& wlength, long& maplength )
{
	wlength += sizeof(DATASOURCE_CFG_LIST_def);

	LIST_length((LIST_def*)pname, sizeof(DATASOURCE_CFG_def), wlength, maplength);

	if (pname->_buffer != NULL && pname->_length > 0)
	{
		DATASOURCE_CFG_def* pDCFG;

		for (unsigned int i=0; i < pname->_length; i++)
		{
			pDCFG = pname->_buffer + i;
			ENV_DESC_LIST_length( &pDCFG->EnvDescList, wlength, maplength );
			RES_DESC_LIST_length( &pDCFG->ResDescList, wlength, maplength );
			OCTET_length( (OCTET_def*)&pDCFG->DefineDescList, wlength, maplength);
		}
	}
}

void SQLVALUE_LIST_length( const SQLValueList_def* pname,  long& wlength, long& maplength )
{
	wlength += sizeof(SQLValueList_def);

	LIST_length((LIST_def*)pname, sizeof(SQLValue_def), wlength, maplength);

	if (pname->_buffer != NULL && pname->_length > 0)
	{
		SQLValue_def* pSQLValue;

		for (unsigned int i=0; i < pname->_length; i++)
		{
			wlength += sizeof(SQLValue_def);

			pSQLValue = pname->_buffer + i;

			OCTET_length( (OCTET_def*)&pSQLValue->dataValue, wlength, maplength);

		}
	}
}

void SRVR_STATUS_LIST_length( const SRVR_STATUS_LIST_def* pname, long& wlength, long& maplength )
{
	wlength += sizeof(SRVR_STATUS_LIST_def);

	LIST_length((LIST_def*)pname, sizeof(SRVR_STATUS_def), wlength, maplength);

	if (pname->_buffer != NULL && pname->_length > 0)
	{
		SRVR_STATUS_def* pSRVR;

		for (unsigned int i=0; i < pname->_length; i++)
		{
			wlength += sizeof(SRVR_STATUS_def);

			pSRVR = pname->_buffer + i;

			STRING_length( pSRVR->userName, wlength, maplength);
			STRING_length( pSRVR->windowText, wlength, maplength);

		}
	}
}

void ENV_DESC_LIST_length( const ENV_DESC_LIST_def* pname, long& wlength, long& maplength )
{
	wlength += sizeof(ENV_DESC_LIST_def);

	LIST_length((LIST_def*)pname, sizeof(ENV_DESC_def), wlength, maplength);
	if (pname->_buffer != NULL && pname->_length > 0)
	{
		ENV_DESC_def* pED;
		for (unsigned int i = 0; i < pname->_length; i++)
		{
			pED = pname->_buffer + i;

			STRING_length( pED->VarVal, wlength, maplength);
		}
	}
}

void RES_DESC_LIST_length( const RES_DESC_LIST_def* pname, long& wlength, long& maplength )
{
	wlength += sizeof(RES_DESC_LIST_def);

	LIST_length((LIST_def*)pname, sizeof(RES_DESC_def), wlength, maplength);
	if (pname->_buffer != NULL && pname->_length > 0)
	{
		RES_DESC_def* pRD;
		for (unsigned int i = 0; i < pname->_length; i++)
		{
			pRD = pname->_buffer + i;

			STRING_length( pRD->Action, wlength, maplength);
		}
	}
}

void DATASOURCE_CFG_length( const DATASOURCE_CFG_def* pname, long& wlength, long& maplength )
{
	wlength += sizeof(DATASOURCE_CFG_def);
	ENV_DESC_LIST_length( &pname->EnvDescList, wlength, maplength );
	RES_DESC_LIST_length( &pname->ResDescList, wlength, maplength );
	OCTET_length( (OCTET_def*)&pname->DefineDescList, wlength, maplength);
}

//
//--------------------- copy parameters ------------------------
//
void IDL_charArray_copy(const IDL_char* pname, char*& curptr)
{
	long length;

	if (pname != NULL)
	{
		length = strlen(pname) + 1;
		memcpy(curptr, pname,length);
		curptr += length;
	}
}

void IDL_long_copy(IDL_long* pname, char*& curptr)
{
	memcpy(curptr, pname, sizeof(IDL_long));
	curptr += sizeof(IDL_long);
}

void IDL_unsigned_long_copy(IDL_unsigned_long* pname, char*& curptr)
{
	memcpy(curptr, pname, sizeof(IDL_unsigned_long));
	curptr += sizeof(IDL_unsigned_long);
}

void IDL_short_copy(IDL_short* pname, char*& curptr)
{
	memcpy(curptr, pname, sizeof(IDL_short));
	curptr += sizeof(IDL_short);
}

void IDL_unsigned_short_copy(IDL_unsigned_short* pname, char*& curptr)
{
	memcpy(curptr, pname, sizeof(IDL_unsigned_short));
	curptr += sizeof(IDL_unsigned_short);
}

void LIST_copy( char* buffer, LIST_def* pname, LIST_def* parptr, long length, char*& curptr, long*& mapptr)
{
	long* tmpptr;
	if (pname->_buffer != NULL && pname->_length > 0)
	{
		tmpptr = (long*)&(parptr->_buffer);
		*(mapptr++) = (long)tmpptr - (long)buffer;
		*tmpptr = curptr - buffer;
		memcpy(curptr, pname->_buffer, pname->_length * length);
		curptr += pname->_length * length;
	}
}

void OCTET_copy( char* buffer, OCTET_def* pname, OCTET_def* parptr, char*& curptr, long*& mapptr)
{
	long* tmpptr;
	if (pname->_buffer != NULL && pname->_length > 0)
	{
		tmpptr = (long*)&(parptr->_buffer);
		*(mapptr++) = (long)tmpptr - (long)buffer;
		*tmpptr =  curptr - buffer;
		memcpy(curptr, pname->_buffer, pname->_length);
		curptr += pname->_length;
	}
}

void STRING_copy( char* buffer, IDL_string pname, IDL_string* parptr, char*& curptr, long*& mapptr)
{
	long length;
	long* tmpptr;

	if (pname != NULL)
	{
		tmpptr = (long*)parptr;
		*(mapptr++) = (long)tmpptr - (long)buffer;
		*tmpptr =  curptr - buffer;
		length = strlen(pname) + 1;
		memcpy(curptr,pname, length);
		curptr += length;
	}
}

void ERROR_DESC_LIST_copy( char* buffer, ERROR_DESC_LIST_def* pname, ERROR_DESC_LIST_def* parptr, char*& curptr, long*& mapptr)
{
	memcpy(curptr, pname, sizeof(ERROR_DESC_LIST_def));
	curptr += sizeof(ERROR_DESC_LIST_def);

	LIST_copy( buffer, (LIST_def*)pname, (LIST_def*) parptr, sizeof(ERROR_DESC_def), curptr, mapptr);

	if (pname->_buffer != NULL && pname->_length > 0)
	{
		ERROR_DESC_def* pED;
		ERROR_DESC_def* pEDpar;
		for (unsigned int i = 0; i < pname->_length; i++)
		{
			pED = pname->_buffer + i;
			pEDpar = (ERROR_DESC_def*)((ULONG)parptr->_buffer + (ULONG)buffer + i*sizeof(ERROR_DESC_def));

			STRING_copy( buffer, pED->errorText, &pEDpar->errorText, curptr, mapptr);
			STRING_copy( buffer, pED->Param1, &pEDpar->Param1, curptr, mapptr);
			STRING_copy( buffer, pED->Param2, &pEDpar->Param2, curptr, mapptr);
			STRING_copy( buffer, pED->Param3, &pEDpar->Param3, curptr, mapptr);
			STRING_copy( buffer, pED->Param4, &pEDpar->Param4, curptr, mapptr);
			STRING_copy( buffer, pED->Param5, &pEDpar->Param5, curptr, mapptr);
			STRING_copy( buffer, pED->Param6, &pEDpar->Param6, curptr, mapptr);
			STRING_copy( buffer, pED->Param7, &pEDpar->Param7, curptr, mapptr);
		}
	}
}

void SRVR_CONTEXT_copy( char* buffer, const SRVR_CONTEXT_def* pname, SRVR_CONTEXT_def* parptr, char*& curptr, long*& mapptr)
{
	memcpy(curptr, pname, sizeof(SRVR_CONTEXT_def));
	curptr += sizeof(SRVR_CONTEXT_def);

	ENV_DESC_LIST_copy(buffer, &pname->envDescList, &parptr->envDescList, curptr, mapptr);
	RES_DESC_LIST_copy(buffer, &pname->resDescList, &parptr->resDescList, curptr, mapptr);
}

void DATASOURCE_CFG_LIST_copy( char* buffer, const DATASOURCE_CFG_LIST_def* pname, DATASOURCE_CFG_LIST_def* parptr, char*& curptr, long*& mapptr)
{
	memcpy(curptr, pname, sizeof(DATASOURCE_CFG_LIST_def));
	curptr += sizeof(DATASOURCE_CFG_LIST_def);

	LIST_copy( buffer, (LIST_def*)pname, (LIST_def*)parptr, sizeof(DATASOURCE_CFG_def), curptr, mapptr);
	
	if (pname->_buffer != NULL && pname->_length > 0)
	{
		DATASOURCE_CFG_def* pDCFG;
		DATASOURCE_CFG_def* pDCFGpar;

		for (unsigned int i=0; i < pname->_length; i++)
		{
			pDCFG = pname->_buffer + i;
			pDCFGpar = (DATASOURCE_CFG_def*)((ULONG)parptr->_buffer + (ULONG)buffer + i*sizeof(DATASOURCE_CFG_def));
			
			ENV_DESC_LIST_copy(buffer,  &pDCFG->EnvDescList, &pDCFGpar->EnvDescList, curptr, mapptr);
			RES_DESC_LIST_copy(buffer,  &pDCFG->ResDescList, &pDCFGpar->ResDescList, curptr, mapptr);
			OCTET_copy( buffer, (OCTET_def*)&pDCFG->DefineDescList, (OCTET_def*)&pDCFGpar->DefineDescList, curptr, mapptr);
		}
	}
}

void SQLVALUE_LIST_copy( char* buffer, const SQLValueList_def* pname, SQLValueList_def* parptr, char*& curptr, long*& mapptr)
{
	memcpy(curptr, pname, sizeof(SQLValueList_def));
	curptr += sizeof(SQLValueList_def);

	LIST_copy( buffer, (LIST_def*)pname, (LIST_def*)parptr, sizeof(SQLValue_def), curptr, mapptr);
	
	if (pname->_buffer != NULL && pname->_length > 0)
	{
		SQLValue_def* pSQLValue;
		SQLValue_def* pSQLValuepar;

		for (unsigned int i=0; i < pname->_length; i++)
		{
			pSQLValue = pname->_buffer + i;
			pSQLValuepar = (SQLValue_def*)((ULONG)parptr->_buffer + (ULONG)buffer + i*sizeof(SQLValue_def));
			
			OCTET_copy( buffer, (OCTET_def*)&pSQLValue->dataValue, (OCTET_def*)&pSQLValuepar->dataValue, curptr, mapptr);
		}
	}
}

void SRVR_STATUS_LIST_copy(char* buffer, const SRVR_STATUS_LIST_def* pname, SRVR_STATUS_LIST_def* parptr, char*& curptr, long*& mapptr)
{
	memcpy(curptr, pname, sizeof(SRVR_STATUS_LIST_def));
	curptr += sizeof(SRVR_STATUS_LIST_def);

	LIST_copy( buffer, (LIST_def*)pname, (LIST_def*)parptr, sizeof(SRVR_STATUS_def), curptr, mapptr);
	
	if (pname->_buffer != NULL && pname->_length > 0)
	{
		SRVR_STATUS_def* pSRVR;
		SRVR_STATUS_def* pSRVRpar;

		for (unsigned int i=0; i < pname->_length; i++)
		{
			pSRVR = pname->_buffer + i;
			pSRVRpar = (SRVR_STATUS_def*)((ULONG)parptr->_buffer + (ULONG)buffer + i*sizeof(SRVR_STATUS_def));
			
			STRING_copy( buffer, pSRVR->userName, &pSRVRpar->userName, curptr, mapptr);
			STRING_copy( buffer, pSRVR->windowText, &pSRVRpar->windowText, curptr, mapptr);
		}
	}
}

void ENV_DESC_LIST_copy(char* buffer,  const ENV_DESC_LIST_def* pname, ENV_DESC_LIST_def* parptr, char*& curptr, long*& mapptr)
{
	memcpy(curptr, pname, sizeof(ENV_DESC_LIST_def));
	curptr += sizeof(ENV_DESC_LIST_def);

	LIST_copy( buffer, (LIST_def*)pname, (LIST_def*)parptr, sizeof(ENV_DESC_def), curptr, mapptr);

	if (pname->_buffer != NULL && pname->_length > 0)
	{
		ENV_DESC_def* pED;
		ENV_DESC_def* pEDpar;
		for (unsigned int i = 0; i < pname->_length; i++)
		{
			pED = pname->_buffer + i;
			pEDpar = (ENV_DESC_def*)((ULONG)parptr->_buffer + (ULONG)buffer + i*sizeof(ENV_DESC_def));

			STRING_copy( buffer, pED->VarVal, &pEDpar->VarVal, curptr, mapptr);
		}
	}
}

void RES_DESC_LIST_copy(char* buffer,  const RES_DESC_LIST_def* pname, RES_DESC_LIST_def* parptr, char*& curptr, long*& mapptr)
{
	memcpy(curptr, pname, sizeof(RES_DESC_LIST_def));
	curptr += sizeof(RES_DESC_LIST_def);

	LIST_copy( buffer, (LIST_def*)pname, (LIST_def*)parptr, sizeof(RES_DESC_def), curptr, mapptr);

	if (pname->_buffer != NULL && pname->_length > 0)
	{
		RES_DESC_def* pRD;
		RES_DESC_def* pRDpar;
		for (unsigned int i = 0; i < pname->_length; i++)
		{
			pRD = pname->_buffer + i;
			pRDpar = (RES_DESC_def*)((ULONG)parptr->_buffer + (ULONG)buffer + i*sizeof(RES_DESC_def));

			STRING_copy( buffer, pRD->Action, &pRDpar->Action, curptr, mapptr);
		}
	}
}

void DATASOURCE_CFG_copy( char* buffer, const DATASOURCE_CFG_def* pname, DATASOURCE_CFG_def* parptr, char*& curptr, long*& mapptr)
{
	memcpy(curptr, pname, sizeof(DATASOURCE_CFG_def));
	curptr += sizeof(DATASOURCE_CFG_def);
	
	ENV_DESC_LIST_copy(buffer,  &pname->EnvDescList, &parptr->EnvDescList, curptr, mapptr);
	RES_DESC_LIST_copy(buffer,  &pname->ResDescList, &parptr->ResDescList, curptr, mapptr);
	OCTET_copy( buffer, (OCTET_def*)&pname->DefineDescList, (OCTET_def*)&parptr->DefineDescList, curptr, mapptr);
}



