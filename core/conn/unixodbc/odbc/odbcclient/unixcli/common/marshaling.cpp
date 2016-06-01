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

#ifndef NSK_PLATFORM
#define SRVRTRACE_ENTER(name)
#define SRVRTRACE_EXIT(name)
#endif

//
//----------- input parameters ------------------------------
//

// New format
void ERROR_DESC_LIST_length( ERROR_DESC_LIST_def* pname,  IDL_long& wlength)
{
	IDL_unsigned_long i = 0;

	wlength += sizeof(pname->_length);

    if(pname->_length > 0 && pname->_buffer != NULL)
	{
		ERROR_DESC_def *ptr = pname->_buffer;

		for(i = 0; i < pname->_length; i++)
		{
			wlength += sizeof(ptr->rowId);
			wlength += sizeof(ptr->errorDiagnosticId);
			wlength += sizeof(ptr->sqlcode);
			wlength += sizeof(ptr->sqlstate);

			wlength += sizeof(IDL_long);
			wlength += strlen(ptr->errorText) + 1;

			wlength += sizeof(ptr->operationAbortId);
			wlength += sizeof(ptr->errorCodeType);

			wlength += sizeof(IDL_long);
			wlength += strlen(ptr->Param1) + 1;

			wlength += sizeof(IDL_long);
			wlength += strlen(ptr->Param2) + 1;

			wlength += sizeof(IDL_long);
			wlength += strlen(ptr->Param3) + 1;

			wlength += sizeof(IDL_long);
			wlength += strlen(ptr->Param4) + 1;

			wlength += sizeof(IDL_long);
			wlength += strlen(ptr->Param5) + 1;

			wlength += sizeof(IDL_long);
			wlength += strlen(ptr->Param6) + 1;

			wlength += sizeof(IDL_long);
			wlength += strlen(ptr->Param7) + 1;

			ptr++;

		}
        
	}

} // ERROR_DESC_LIST_length


// new format
void SQLVALUE_LIST_length(SQLValueList_def* pname,  IDL_long& wlength)
{
	IDL_unsigned_long i;

	wlength += sizeof(pname->_length);

	if(pname->_length > 0)
	{
		SQLValue_def *pSQLValue = pname->_buffer;

		for(i = 0; i < pname->_length; i++)
		{
			wlength += sizeof(pSQLValue->dataType);
			wlength += sizeof(pSQLValue->dataInd);
			wlength += sizeof(pSQLValue->dataValue._length);
			if(pSQLValue->dataValue._length > 0)
				wlength += pSQLValue->dataValue._length;
			wlength += sizeof(pSQLValue->dataCharset);
			pSQLValue++;
		}

	}

} // SQLVALUE_LIST_length()


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

void IDL_charArray_Pad_copy(const IDL_char* pname, long length, char*& curptr)
{
	if (pname != NULL)
	{
		memset(curptr,0,length);
		memcpy(curptr, pname,length);
		curptr += length;
	}
}

void IDL_byteArray_copy(BYTE* pname, long length, char*& curptr)
{
	if (pname != NULL)
	{
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

void IDL_long_long_copy(IDL_long_long* pname, char*& curptr)
{
	memcpy(curptr, pname, sizeof(IDL_long_long));
	curptr += sizeof(IDL_long_long);
}

void IDL_unsigned_long_long_copy(IDL_unsigned_long_long* pname, char*& curptr)
{
	memcpy(curptr, pname, sizeof(IDL_unsigned_long_long));
	curptr += sizeof(IDL_unsigned_long_long);
}

void IDL_double_copy(double* pname, char*& curptr)
{
	memcpy(curptr, pname, sizeof(double));
	curptr += sizeof(double);
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


// marshalling - new format
void ERROR_DESC_LIST_copy(ERROR_DESC_LIST_def* pname, IDL_char*& curptr)
{

	IDL_unsigned_long i = 0;
	IDL_long tmpLength;

	IDL_unsigned_long_copy(&pname->_length, curptr);

	if(pname->_length > 0 && pname->_buffer != NULL)
	{
		ERROR_DESC_def *ptr = pname->_buffer;

		for(i = 0; i < pname->_length; i++)
		{

            IDL_long_copy(&ptr->rowId, curptr);
			IDL_long_copy(&ptr->errorDiagnosticId, curptr);
			IDL_long_copy(&ptr->sqlcode, curptr);
			
			strncpy(curptr,ptr->sqlstate,sizeof(ptr->sqlstate));
            curptr += sizeof(ptr->sqlstate);

			tmpLength = strlen(ptr->errorText);
			if(tmpLength > 0)
			{
				tmpLength++; // null terminator
				IDL_long_copy(&tmpLength,curptr);
				IDL_charArray_copy(ptr->errorText,curptr);

			}
			else
				IDL_long_copy(&tmpLength,curptr);

			IDL_long_copy(&ptr->operationAbortId,curptr);
			IDL_long_copy(&ptr->errorCodeType,curptr);

			tmpLength = strlen(ptr->Param1);
			if(tmpLength > 0)
			{
				tmpLength++; // null terminator
				IDL_long_copy(&tmpLength,curptr);
				IDL_charArray_copy(ptr->Param1,curptr);

			}
			else
				IDL_long_copy(&tmpLength,curptr);

			tmpLength = strlen(ptr->Param2);
			if(tmpLength > 0)
			{
				tmpLength++; // null terminator
				IDL_long_copy(&tmpLength,curptr);
				IDL_charArray_copy(ptr->Param2,curptr);

			}
			else
				IDL_long_copy(&tmpLength,curptr);

			tmpLength = strlen(ptr->Param3);
			if(tmpLength > 0)
			{
				tmpLength++; // null terminator
				IDL_long_copy(&tmpLength,curptr);
				IDL_charArray_copy(ptr->Param3,curptr);

			}
			else
				IDL_long_copy(&tmpLength,curptr);

			tmpLength = strlen(ptr->Param4);
			if(tmpLength > 0)
			{
				tmpLength++; // null terminator
				IDL_long_copy(&tmpLength,curptr);
				IDL_charArray_copy(ptr->Param4,curptr);

			}
			else
				IDL_long_copy(&tmpLength,curptr);

			tmpLength = strlen(ptr->Param5);
			if(tmpLength > 0)
			{
				tmpLength++; // null terminator
				IDL_long_copy(&tmpLength,curptr);
				IDL_charArray_copy(ptr->Param5,curptr);

			}
			else
				IDL_long_copy(&tmpLength,curptr);			

			tmpLength = strlen(ptr->Param6);
			if(tmpLength > 0)
			{
				tmpLength++; // null terminator
				IDL_long_copy(&tmpLength,curptr);
				IDL_charArray_copy(ptr->Param6,curptr);

			}
			else
				IDL_long_copy(&tmpLength,curptr);

			tmpLength = strlen(ptr->Param7);
			if(tmpLength > 0)
			{
				tmpLength++; // null terminator
				IDL_long_copy(&tmpLength,curptr);
				IDL_charArray_copy(ptr->Param7,curptr);

			}
			else
				IDL_long_copy(&tmpLength,curptr);
			
			ptr++;
		}
	}

} /* ERROR_DESC_LIST_copy() */



// new format
void SQLVALUE_LIST_copy(SQLValueList_def* pname, IDL_char*& curptr)
{

	IDL_unsigned_long i;

	IDL_unsigned_long_copy(&pname->_length, curptr);

    if(pname->_length > 0)
	{
		SQLValue_def *pSQLValue = pname->_buffer;

		for(i = 0; i < pname->_length; i++)
		{
			IDL_long_copy(&pSQLValue->dataType,curptr);
			IDL_short_copy(&pSQLValue->dataInd,curptr);
			IDL_unsigned_long_copy(&pSQLValue->dataValue._length, curptr);
			if(pSQLValue->dataValue._length > 0)
				IDL_byteArray_copy((BYTE*)pSQLValue->dataValue._buffer,pSQLValue->dataValue._length,curptr);
			IDL_long_copy(&pSQLValue->dataCharset,curptr);
			pSQLValue++;
		}

	}	
} // SQLVALUE_LIST_copy()

// new format
void SQLITEMDESC_LIST_copy(SQLItemDescList_def* pname, IDL_char*& curptr)
{

	IDL_unsigned_long i;

	IDL_long colHeadingNmLength = 0;
	IDL_long TableNameLength = 0;
    IDL_long CatalogNameLength = 0;
    IDL_long SchemaNameLength = 0;
    IDL_long HeadingLength = 0;


	IDL_unsigned_long_copy(&pname->_length, curptr);

    if(pname->_length > 0 && pname->_buffer != NULL)
	{
		SQLItemDesc_def *pSQLItemDesc = pname->_buffer;

		for(i = 0; i < pname->_length; i++)
		{
			IDL_long_copy(&pSQLItemDesc->version,curptr);
			IDL_long_copy(&pSQLItemDesc->dataType,curptr);
			IDL_long_copy(&pSQLItemDesc->datetimeCode,curptr);
			IDL_long_copy(&pSQLItemDesc->maxLen,curptr);
			IDL_short_copy(&pSQLItemDesc->precision,curptr);
			IDL_short_copy(&pSQLItemDesc->scale,curptr);
			IDL_byteArray_copy((BYTE*)&pSQLItemDesc->nullInfo,sizeof(pSQLItemDesc->nullInfo),curptr);

			if(pSQLItemDesc->colHeadingNm[0] != '\0')
			   colHeadingNmLength = (strlen(pSQLItemDesc->colHeadingNm) +1);
			else
			   colHeadingNmLength = 0;
			IDL_long_copy(&colHeadingNmLength,curptr);
			if(colHeadingNmLength > 0)
			   IDL_charArray_copy(pSQLItemDesc->colHeadingNm,curptr);


			IDL_byteArray_copy((BYTE*)&pSQLItemDesc->signType,sizeof(pSQLItemDesc->signType),curptr);
			IDL_long_copy(&pSQLItemDesc->ODBCDataType,curptr);
			IDL_short_copy(&pSQLItemDesc->ODBCPrecision,curptr);
			IDL_long_copy(&pSQLItemDesc->SQLCharset,curptr);
			IDL_long_copy(&pSQLItemDesc->ODBCCharset,curptr);

			if(pSQLItemDesc->TableName[0] != '\0')
			   TableNameLength = (strlen(pSQLItemDesc->TableName) +1);
			else
			   TableNameLength = 0;
			IDL_long_copy(&TableNameLength,curptr);
			if(TableNameLength > 0)
			   IDL_charArray_copy(pSQLItemDesc->TableName,curptr);
			
			if(pSQLItemDesc->CatalogName[0] != '\0')
			   CatalogNameLength = (strlen(pSQLItemDesc->CatalogName) +1);
			else
			   CatalogNameLength = 0;
			IDL_long_copy(&CatalogNameLength,curptr);
			if(CatalogNameLength > 0)
			   IDL_charArray_copy(pSQLItemDesc->CatalogName,curptr);

			if(pSQLItemDesc->SchemaName[0] != '\0')
			   SchemaNameLength = (strlen(pSQLItemDesc->SchemaName) +1);
			else
			   SchemaNameLength = 0;
			IDL_long_copy(&SchemaNameLength,curptr);
			if(SchemaNameLength > 0)
			   IDL_charArray_copy(pSQLItemDesc->SchemaName,curptr);

			if(pSQLItemDesc->Heading[0] != '\0')
			   HeadingLength = (strlen(pSQLItemDesc->Heading) +1);
			else
			   HeadingLength = 0;
			IDL_long_copy(&HeadingLength,curptr);
			if(HeadingLength > 0)
			   IDL_charArray_copy(pSQLItemDesc->Heading,curptr);

			IDL_long_copy(&pSQLItemDesc->intLeadPrec,curptr);
			IDL_long_copy(&pSQLItemDesc->paramMode,curptr);

			pSQLItemDesc++;
		}

	}	
} // SQLVALUE_LIST_copy()

