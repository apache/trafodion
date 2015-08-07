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
//
// MODULE: NskUtil.cpp
//
// PURPOSE: Implements the common functions used by Servers. 
//
// NOTE:	This file will compile only if NSK_PLATFORM is defined
//
//
#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "Global.h"
#include "srvrcommon.h"
#include "srvrfunctions.h"
#include "srvrkds.h"
#include "sqlinterface.h"
#include "CommonDiags.h"
#include "tdm_odbcSrvrMsg.h"

#include "NskUtil.h"

using namespace SRVR;

char systemNm[10] = {0};
char systemCatalogNm[MAX_SQL_IDENTIFIER_LEN+3] = {0};

BOOL SRVR::GetSystemNm (char *systemNm)
{
	short errorCode;
	char  sysNm[8];
	short sysNmLen;

    TPT_DECL(processHandle);

	PROCESSHANDLE_GETMINE_(TPT_REF(processHandle));
	errorCode = PROCESSHANDLE_DECOMPOSE_(TPT_REF(processHandle)
									   ,OMITREF /* cpu */
									   ,OMITREF /* pin */
									   ,OMITREF /* nodenumber */
									   ,sysNm
									   ,(short)sizeof(sysNm)
									   ,&sysNmLen);
	if (errorCode == FEOK)
	{
	  memcpy(systemNm, sysNm, sysNmLen);
	  systemNm[sysNmLen] = '\0';
	  return TRUE;
	}

	return FALSE;
}

BOOL SRVR::envGetMXSystemCatalogName (char *catalogNm)
{
	strcpy(catalogNm, "HP_SYSTEM_CATALOG");
	return TRUE;
}

BOOL SRVR::envGetMXSystemCatalogName (char *catalogNm, char *systemName)
{
	 strcpy(catalogNm, "HP_SYSTEM_CATALOG");
	return TRUE;
}

//LCOV_EXCL_START
BOOL SRVR::getTimeInterval(long long jtimestamp , int* day, short* hours, short* minutes, short* seconds)
{
	long long temp_jtimestamp =   jtimestamp - JULIANTIMESTAMP(0);
	*day = INTERPRETINTERVAL(temp_jtimestamp , hours, minutes, seconds,OMITREF,OMITREF );
	return true;
}
//LCOV_EXCL_STOP

char* SRVR::_i64toa( __int64 n, char *buff, int base )
{
   char t[100], *c=t, *f=buff;
   long d;
    int bit;

   if (base == 10) {
     if (n < 0) {
        *(f++) = '-';
        n = -n;
	 }

     while (n != 0) {
        d = n % base;
		if (d < 0) d = -d;
        *(c++) = d + '0';
        n = n / base;
	 }
   }
   
   else {
	 short bitlen = 64;

     if (base == 2) bit = 1;
     else if (base == 8) bit = 3;
     else if (base == 16) bit = 4;
	 { base = 16; bit =4;} // printf("Base value unknown!\n");

     while (bitlen != 0) {
        d = (n  & (base-1));
        *(c++) = d < 10 ? d + '0' : d - 10 + 'A';
        n =  n >> bit;
		bitlen -= bit;
	 }

   }

   c--;

   while (c >= t) *(f++) = *(c--);
     
   if (buff == f)
	   *(f++) = '0';
   *f = '\0';
   return buff;
}
