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
//
#include "stubtrace.h"


#ifndef MXAIX
#include <wchar.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
//#include <share.h>
#include "sql.h"
#include "sqlext.h"
#include "odbcinst.h"

#include "tracecon.h"	
#include "sqlucode.h"
//#include <tchar.h>
//#include <locale.h>
#include <sys/timeb.h>
#include <time.h>

#include "transportbase.h"	
#include "swap.h"
#include <errno.h>

#define MAX_PATH			256
#define DEBUG_BUFFER_SIZE	2048 	    /* Maximum string length */
#define DEBUG_LINE_LENGTH	90
#define DEBUG_LINE_BUFFER_SIZE	(DEBUG_LINE_LENGTH + 1)
#define BROKEN_WORD_LENGTH	45		 

#ifndef DISABLE_TRACE
extern	FILE	*fhTraceFile;
extern  BOOL	g_fNoTrace;
#endif
extern  CRITICAL_SECTION2 g_csWrite;
extern	DWORD	dwGlobalTraceVariable;


extern RETCODE SQL_API TraceOpenLogFile(LPWSTR	szFileName, LPWSTR lpwszOutputMsg,DWORD	cbOutputMsg);

void text_out(char *text, char *option)
{
#ifndef DISABLE_TRACE
	char dobuffer[DEBUG_LINE_BUFFER_SIZE];
	char *last_space;
	char *start_space;
	char *source;
	char *p;
	char szProcName[MAX_PATH];
	char buf[PID_TID_BUF_LENGTH];

	GetWindowText(NULL, szProcName, MAX_PATH );

	GetCurrentProcessAndThreadId(buf);
	sprintf(dobuffer,"\r\n%-15.15s %s\t%.50s: ", szProcName, buf, option);

	fwrite(dobuffer, strlen(dobuffer),1,fhTraceFile);
	fwrite("\r\n",2,1,fhTraceFile);
	fflush(fhTraceFile);

	last_space = source = text;

	while (*source != '\0')
	{
		strcpy(dobuffer,"\t\t");

		p = (char *) dobuffer + strlen(dobuffer);
    
		last_space = p-1;
		start_space = p-1;
    
		while ((*source != '\0') &&
			(*source != '\r') &&
			(*source != '\n') &&
			((p - (char *) dobuffer) < (sizeof dobuffer - 1)))
		{
			if (*source == ' ' || *source == ',')
				last_space = p;
			*p++ = *source++;
		}

				/* Broke at end of display line in word?    */
		if ((*source != '\0') &&
			(*source != '\r') &&
			(*source != '\n') &&
			(*source != ' ')  &&
			(*source != '\t')	)
		{
			if ((p - last_space) <= BROKEN_WORD_LENGTH && last_space != start_space)
  			{			    /* Back to start of word	*/
				source -= (p - last_space) - 1;
				p = last_space;
			}
		}

		*p = '\0';	    /* Terminate it */

		while ((*source == ' ')  ||		     
			(*source == '\r') ||
			(*source == '\n') ||
			(*source == '\t')   )
			source++;

	// output to a debug process if this is a client trace event

		fwrite(dobuffer, strlen(dobuffer),1,fhTraceFile);
		fwrite("\r\n",2,1,fhTraceFile);
		fflush(fhTraceFile);

  }	 // end of while
#endif
}

//==================================================================
char tabPrintable[] = {
//
//  00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 00
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 01
   ' ','!','"','#','$','%','&',39 ,'(',')','*','+',',','-','.','/',	// 02
   '0','1','2','3','4','5','6','7','8','9',':',';','<','=','>','?',	// 03
   '@','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',	// 04
   'P','Q','R','S','T','U','V','W','X','Y','Z','[',92 ,']','^','_',	// 05
   '`','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',	// 06
   'p','q','r','s','t','u','v','w','x','y','z','{','|','}','~','.',	// 07
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 08
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 09
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 0A
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 0B
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 0C
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 0D
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',	// 0E
   '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'	// 0F
//  00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F
};

VOID FormatHexOutput( unsigned Offset,char *InIn, char *OutOut,int Length )
{
	int i,j;
	unsigned char *In = (unsigned char *)InIn;
	unsigned char *Out = (unsigned char *)OutOut;

	static char HexChars[] = "0123456789ABCDEF";

	if(Length <= 0 ) return;

 // format offset to 4 hex digits
	*Out++ = HexChars[Offset >> 12];
	*Out++ = HexChars[(Offset & 0X0FFF) >>  8];
	*Out++ = HexChars[(Offset & 0X00FF) >>  4];
	*Out++ = HexChars[ Offset & 0X000F];
	*Out++ = ' ';
	*Out++ = ' ';

 // convert the input byte to hex
	for( i = 0, j = 0; i < Length; i++,j++ )
	{
		*Out++ = HexChars[(*In) >> 4];
		*Out++ = HexChars[(*In++) & 0X0F];
		*Out++ = ' ';
		if(j==7)
		{
			*Out++ = '-';
			*Out++ = ' ';
		}
	}

	*Out++ = ' ';

	for( i = 0; i < (16-Length); i++,j++ ) // Align the ascii to 16 data bytes
	{
		*Out++ = ' ';
		*Out++ = ' ';
		*Out++ = ' ';
		if(j==7)
		{
			*Out++ = '-';
			*Out++ = ' ';
		}
	}

 // output the original data
	In = (unsigned char *)InIn;
	for( i = 0; i < Length; i++ )
	{
		char ch = tabPrintable[*In++];
		*Out++ = ch;
	}
//	*Out++ = '\n';
	*Out = '\0';
 
	return;
 
}  // end of FormatHexOutput


void hex_out(  IDL_unsigned_long Len,char *Data, LPSTR TextString )
{
#ifndef DISABLE_TRACE
	char Temp[90];
	char buffer[110];
	int i;
	char *pData = Data;
	int NumWholeLines;
	int PartialLineLength;
	unsigned int Offset;
 
	if((int)Len <= 0) return;

	// Len = ((Len + 1 + 16)/16)*16;
	// Len = Len > 4096 ? 4096 : Len;

	Offset = 0;
 
 // Compute the number of lines and their sizes
	NumWholeLines = Len / 16;
	PartialLineLength = Len % 16;

	sprintf(Temp, "\r\n\t\t<%lX> %.70s, size:%d (0x%04X)", Data,TextString, Len, Len );

	fwrite(Temp, strlen(Temp),1,fhTraceFile);
	fwrite("\r\n",2,1,fhTraceFile);
	fflush(fhTraceFile);


// output the whole lines
	for( i = 0; i < NumWholeLines; i++ )
	{
		FormatHexOutput( Offset, pData, Temp, 16 );
		sprintf(buffer, "\t\t%s", Temp );

		fwrite(buffer, strlen(buffer),1,fhTraceFile);
		fwrite("\r\n",2,1,fhTraceFile);
		fflush(fhTraceFile);

		pData += 16;      
		Offset += 16;    
	}

 // output the partial line (if any)
	if( PartialLineLength )
	{
		FormatHexOutput( Offset, pData, Temp, PartialLineLength );
		sprintf(buffer, "\t\t%s", Temp );

		fwrite(buffer, strlen(buffer),1,fhTraceFile);
		fwrite("\r\n",2,1,fhTraceFile);
		fflush(fhTraceFile);

	}

	return;
#endif
}

void systemVersion()
{

}
void dllVersion( char* szDllName, char* szTitle)
{

}

extern VERSION_def* ASVersion=NULL;
extern VERSION_def* SrvrVersion=NULL;
extern VERSION_def* SqlVersion=NULL;

void srvrVersions()
{
#ifndef DISABLE_TRACE
	char	buffer[100];
	if (ASVersion != NULL)
	{
		sprintf(buffer,"Association Server Version = %d.%d.%ld",ASVersion->majorVersion,ASVersion->minorVersion,ASVersion->buildId);
		fwrite(buffer, strlen(buffer),1,fhTraceFile);
		fwrite("\r\n",2,1,fhTraceFile);
	}
	if (SrvrVersion != NULL)
	{
		sprintf(buffer,"Server Version = %d.%d.%ld",SrvrVersion->majorVersion,SrvrVersion->minorVersion,SrvrVersion->buildId);
		fwrite(buffer, strlen(buffer),1,fhTraceFile);
		fwrite("\r\n",2,1,fhTraceFile);
	}
	if (SqlVersion != NULL)
	{
		sprintf(buffer,"SqlVersion = %d.%d.%ld (Sql %s Rowsets)",SqlVersion->majorVersion,SqlVersion->minorVersion,SqlVersion->buildId,
			SqlVersion->minorVersion!=0? "supports":"does not support");
		fwrite(buffer, strlen(buffer),1,fhTraceFile);
		fwrite("\r\n",2,1,fhTraceFile);
	}
#endif
}

void traceInfo(char* szDriverDllName)
{
#ifndef DISABLE_TRACE
	char buffer[110];
	struct _timeb CurrentTime;
	char* timeline;

   _ftime(&CurrentTime);
	timeline = ctime( & ( CurrentTime.time ) );
	*(timeline + 20 + 4 ) = 0;
	sprintf(buffer,"\r\n-------------- Info Block Entry (%.50s) ------------\r\n\r\n", szDriverDllName);
	fwrite(buffer, strlen(buffer),1,fhTraceFile);

	sprintf(buffer,"Date: %s\r\n\r\n",timeline);
	fwrite(buffer, strlen(buffer),1,fhTraceFile);

	srvrVersions();
	fwrite("\r\n",2,1,fhTraceFile);

	sprintf(buffer,"-------------- Info Block Exit ------------\r\n\r\n");
	fwrite(buffer, strlen(buffer),1,fhTraceFile);
	fflush(fhTraceFile);
#endif
}
//=================================================================================

#define FORMAT_TEXT 0
#define FORMAT_DUMP 1

extern "C" VOID SQL_API TraceDebugOut( unsigned int length, char* szBuffer, char* text, int format)
{
#ifndef DISABLE_TRACE
/*
 *  TraceReturn calls GrabErrors(). GrabErrors() goes in and gets diagnostics - now if there is
 *  any kind of debug tracing going on there TraceDebygOut gets called and 
 *  we could get into a problem with deadlocks.  To avoid that condition we will attempt to lock the mutex
 *  if the calling thread owns the lock, EDEADLK will be returned and we can proceed
 *  otherwise EBUSY is returned and we can call the normal lock and wait
 *
 */

	int status;
	int wasAlreadyLocked = false;

	status = g_csWrite.TryLock();

	if(status == EBUSY)
	   EnterCriticalSection2(&g_csWrite);
	else if(status == EDEADLK)
	  wasAlreadyLocked = true;

	if (g_fNoTrace)
	{
		if(!wasAlreadyLocked) 
		   LeaveCriticalSection2(&g_csWrite);
		return;
	}


	if (!fhTraceFile)
	{
		(void) TraceOpenLogFile((LPWSTR)szGlobalTraceFileName,NULL,0);
		if (!fhTraceFile)
		{
			if(!wasAlreadyLocked) 
		       LeaveCriticalSection2(&g_csWrite);
			return;
	}
	}

	if(FORMAT_TEXT==format)
	{
		text_out(szBuffer,text);
	}
	else if(FORMAT_DUMP==format)
	{
		hex_out(length, szBuffer, text);
	}
	fflush(fhTraceFile);

	if(!wasAlreadyLocked) 
	LeaveCriticalSection2(&g_csWrite);
#endif
}

extern "C" DWORD* SQL_API TraceProcessEntry()
{
	return &dwGlobalTraceVariable;
}

extern "C" VOID SQL_API TracePrintMarker(void)
{

}

extern "C" VOID SQL_API TraceFirstEntry( char* szDriverDLLName )
{
#ifndef DISABLE_TRACE
	EnterCriticalSection2(&g_csWrite);

	if (!fhTraceFile)
	{
		(void) TraceOpenLogFile((LPWSTR)szGlobalTraceFileName,NULL,0);
		if (!fhTraceFile)
		{
		   	LeaveCriticalSection2(&g_csWrite);
			return;
		}
	}

	traceInfo(szDriverDLLName);
	fflush(fhTraceFile);

	LeaveCriticalSection2(&g_csWrite);
#endif
}

static char* formatAPI(int api)
{
	switch(api)
	{
//as
	case AS_API_INIT:
		return "AS_INIT";
		break;
	case AS_API_GETOBJREF:
		return "GETOBJREF";
		break;
	case AS_API_REGPROCESS:
		return "REGPROCESS";
		break;
	case AS_API_UPDATESRVRSTATE:
		return "UPDATESRVRSTATE";
		break;
	case AS_API_WOULDLIKETOLIVE:
		return "WOULDLIKETOLIVE";
		break;
	case AS_API_STARTAS:
		return "STARTAS";
		break;
	case AS_API_STOPAS:
		return "STOPAS";
		break;
	case AS_API_STARTDS:
		return "STARTDS";
		break;
	case AS_API_STOPDS:
		return "STOPDS";
		break;
	case AS_API_STATUSAS:
		return "STATUSAS";
		break;
	case AS_API_STATUSDS:
		return "STATUSDS";
		break;
	case AS_API_STATUSDSDETAIL:
		return "STATUSDSDETAIL";
		break;
	case AS_API_STATUSSRVRALL:
		return "STATUSSRVRALL";
		break;
	case AS_API_STOPSRVR:
		return "STOPSRVR";
		break;
	case AS_API_STATUSDSALL:
		return "STATUSDSALL";
		break;
	case AS_API_DATASOURCECONFIGCHANGED:
		return "DATASOURCECONFIGCHANGED";
		break;
	case AS_API_ENABLETRACE:
		return "ENABLETRACE";
		break;
	case AS_API_DISABLETRACE:
		return "DISABLETRACE";
		break;
	case AS_API_GETVERSIONAS:
		return "GETVERSIONAS";
		break;
//cfg
	case CFG_API_INIT:
		return "CFG_INIT";
		break;
	case CFG_API_GETOBJECTNAMELIST:
		return "GETOBJECTNAMELIST";
		break;
	case CFG_API_GETDATASOURCE:
		return "GETDATASOURCE";
		break;
	case CFG_API_DROPDATASOURCE:
		return "DROPDATASOURCE";
		break;
	case CFG_API_SETDATASOURCE:
		return "SETDATASOURCE";
		break;
	case CFG_API_ADDNEWDATASOURCE:
		return "ADDNEWDATASOURCE";
		break;
	case CFG_API_CHECKDATASOURCENAME:
		return "CHECKDATASOURCENAME";
		break;
	case CFG_API_GETDSNCONTROL:
		return "GETDSNCONTROL";
		break;
	case CFG_API_SETDSNCONTROL:
		return "SETDSNCONTROL";
		break;
	case CFG_API_GETRESOURCEVALUES:
		return "GETRESOURCEVALUES";
		break;
	case CFG_API_SETRESOURCEVALUES:
		return "SETRESOURCEVALUES";
		break;
	case CFG_API_GETENVIRONMENTVALUES:
		return "GETENVIRONMENTVALUES";
		break;
	case CFG_API_SETENVIRONMENTVALUES:
		return "SETENVIRONMENTVALUES";
		break;
	case CFG_API_GETSTARTUPCONFIGVALUES:
		return "GETSTARTUPCONFIGVALUES";
		break;
	case CFG_API_GETDATASOURCEVALUES:
		return "GETDATASOURCEVALUES";
		break;
	case CFG_API_SETDSSTATUS:
		return "SETDSSTATUS";
		break;
	case CFG_API_SETASSTATUS:
		return "SETASSTATUS";
		break;
//srvr	
	case SRVR_API_INIT:
		return "SRVR_INIT";
		break;
	case SRVR_API_SQLCONNECT:
		return "SQLCONNECT";
		break;
	case SRVR_API_SQLDISCONNECT:
		return "SQLDISCONNECT";
		break;
	case SRVR_API_SQLSETCONNECTATTR:
		return "SQLSETCONNECTATTR";
		break;
	case SRVR_API_SQLENDTRAN:
		return "SQLENDTRAN";
		break;
	case SRVR_API_SQLPREPARE:
		return "SQLPREPARE";
		break;
	case SRVR_API_SQLPREPARE_ROWSET:
		return "SQLPREPARE_ROWSET";
		break;
	case SRVR_API_SQLPREPARE2:
		return "SQLPREPARE2";
		break;
	case SRVR_API_SQLEXECUTE_ROWSET:
		return "SQLEXECUTE_ROWSET";
		break;
	case SRVR_API_SQLEXECDIRECT_ROWSET:
		return "SQLEXECDIRECT_ROWSET";
		break;
	case SRVR_API_SQLFETCH_ROWSET:
		return "SQLFETCH_ROWSET";
		break;
	case SRVR_API_SQLEXECDIRECT:
		return "SQLEXECDIRECT";
		break;
	case SRVR_API_SQLEXECUTE:
		return "SQLEXECUTE";
		break;
	case SRVR_API_SQLEXECUTECALL:
		return "SQLEXECUTECALL";
		break;
	case SRVR_API_SQLEXECUTE2:
		return "SQLEXECUTE2";
		break;
	case SRVR_API_SQLFETCH_PERF:
		return "SQLFETCH_PERF";
		break;
	case SRVR_API_SQLFREESTMT:
		return "SQLFREESTMT";
		break;
	case SRVR_API_GETCATALOGS:
		return "GETCATALOGS";
		break;
	case SRVR_API_STOPSRVR:
		return "STOPSRVR";
		break;
	case SRVR_API_ENABLETRACE:
		return "ENABLETRACE";
		break;
	case SRVR_API_DISABLETRACE:
		return "DISABLETRACE";
		break;
	case SRVR_API_ENABLE_SERVER_STATISTICS:
		return "ENABLE_SERVER_STATISTICS";
		break;
	case SRVR_API_DISABLE_SERVER_STATISTICS:
		return "DISABLE_SERVER_STATISTICS";
		break;
	case SRVR_API_UPDATE_SERVER_CONTEXT:
		return "UPDATE_SERVER_CONTEXT";
		break;
	default:
		return "UNKNOWN_API";
	}

	// if (api < AS_API_START )
		// return "UNKNOWN_API";
	// else if (api < CFG_API_START )
		// return formatAS_API(api);
	// else if (api < SRVR_API_START )
		// return formatCFG_API(api);
	// else
		// return formatSRVR_API(api);
}

void formatHEADER(int operation, char* reference, HEADER* header)
{
#ifndef DISABLE_TRACE
	char buffer[DEBUG_BUFFER_SIZE+1];
	char destination[100];

	if (operation == READ_RESPONSE_FIRST)
		sprintf(destination,"received from <----------- %s",reference);
	else
		sprintf(destination,"sent to -----------> %s",reference);

	sprintf( buffer,"\r\n\tBUFFER %s\r\n"  
		"\tFORMATTED HEADER:\r\n"
		"\t\toperation_id = %d\t\t%s\r\n"
		"\t\tdialogueId = %u\r\n"
		"\t\ttotal_length = %d\r\n"
		"\t\tcmp_length = %d\r\n"
		"\t\tcompress_ind = %c\r\n"
		"\t\tcompress_type = %d\r\n"
		"\t\thdr_type = %d\r\n"
		"\t\tsignature = %u\r\n"
		"\t\tveresion = %u\r\n"
		"\t\tplatform = %c\r\n"
		"\t\ttransport = %c\r\n"
		"\t\tswap = %c\r\n"
		"\t\terror = %d\r\n"
		"\t\terror_detail = %d\r\n",
		destination,
		header->operation_id,formatAPI(header->operation_id),
		header->dialogueId,
		header->total_length,
		header->cmp_length,
		header->compress_ind,
		header->compress_type,
		header->hdr_type,
		header->signature,
		header->version,
		header->platform,
		header->transport,
		header->swap,
		header->error,
		header->error_detail
	);

	fwrite(buffer, strlen(buffer),1,fhTraceFile);
	fwrite("\r\n",2,1,fhTraceFile);
#endif
}

void traceBUFFER(int operation, HEADER* header,char* buffer, long count)
{
	HEADER hdr;
	memcpy(&hdr, header, sizeof(HEADER));

	if (operation == WRITE_REQUEST_FIRST)
	{
		if (header->swap == 'Y')
			HEADER_swap(&hdr);
		hex_out(   sizeof(HEADER), (char*)&hdr, "HEADER" );
	}
	else if (operation == READ_RESPONSE_FIRST)
	{
		hex_out(  sizeof(HEADER), (char*)&hdr, "HEADER" );
	}
	hex_out(  count, buffer, "DATA" );
	
}

void traceTransportIn (int operation, char* reference, void* prheader, char* buffer, long tcount, long timeout)
{
#ifndef DISABLE_TRACE
	struct _timeb StartTime;
	char *timeline;
	char	szProcName[MAX_PATH];
	char	textTimeLine[50];

	char line[100+1];
	HEADER* header = (HEADER*)prheader;

	GetWindowText(NULL, szProcName, MAX_PATH );
	_ftime(&StartTime);
	timeline = ctime( & ( StartTime.time ) );
	*(timeline + 20 + 4 ) = 0;
	*(timeline + 19 ) = 0;
	sprintf(textTimeLine," %.19s.%hu ",&timeline[11], StartTime.millitm );

	sprintf(line,
		"\r\n%-15.15s %x:%x\tTCP/IP INPUT BUFFER <length %ld> %s",
		szProcName,
		GetCurrentProcessId(),
		GetCurrentThreadId(),
		tcount,
		textTimeLine);
	fwrite(line, strlen(line),1,fhTraceFile);

	if (operation == READ_RESPONSE_FIRST)
		formatHEADER(operation, reference, header);
	traceBUFFER(operation, header, buffer, tcount);
#endif
}

void traceTransportOut (int operation, char* reference, void* pwheader, char* buffer, long tcount, long timeout)
{
#ifndef DISABLE_TRACE
	struct _timeb StartTime;
	char *timeline;
	char	szProcName[MAX_PATH];
	char	textTimeLine[50];

	char line[100+1];
	HEADER* header = (HEADER*)pwheader;

	GetWindowText(NULL, szProcName, MAX_PATH );
	_ftime(&StartTime);
	timeline = ctime( & ( StartTime.time ) );
	*(timeline + 20 + 4 ) = 0;
	*(timeline + 19 ) = 0;
	sprintf(textTimeLine," %.19s.%hu ",&timeline[11], StartTime.millitm );

	sprintf(line,
		"\r\n%-15.15s %x:%x\tTCP/IP OUTPUT BUFFER <length %ld> %s",
		szProcName,
		GetCurrentProcessId(),
		GetCurrentThreadId(),
		tcount,
		textTimeLine);
	fwrite(line, strlen(line),1,fhTraceFile);

	if (operation == WRITE_REQUEST_FIRST)
		formatHEADER(operation, reference, header);
	traceBUFFER(operation, header, buffer, tcount);
#endif
}

extern "C" VOID SQL_API TraceTransportOut(int operation, char* reference, void* pwheader, char* buffer, long tcount, long timeout)
{
#ifndef DISABLE_TRACE
	EnterCriticalSection2(&g_csWrite);
	if (!fhTraceFile)
	{
		(void) TraceOpenLogFile((LPWSTR)szGlobalTraceFileName,NULL,0);
		if (!fhTraceFile)
		{
			LeaveCriticalSection2(&g_csWrite);
			return;
		}
	}
	traceTransportOut(operation, reference, pwheader, buffer, tcount, timeout);
	fflush(fhTraceFile);
	LeaveCriticalSection2(&g_csWrite);
#endif
}

extern "C" VOID SQL_API TraceTransportIn(int operation, char* reference, void* prheader, char* buffer, long tcount, long timeout)
{
#ifndef DISABLE_TRACE
	EnterCriticalSection2(&g_csWrite);
	if (!fhTraceFile)
	{
		(void) TraceOpenLogFile((LPWSTR)szGlobalTraceFileName,NULL,0);
		if (!fhTraceFile)
		{
			LeaveCriticalSection2(&g_csWrite);
			return;
		}
	}
	traceTransportIn(operation, reference, prheader, buffer, tcount, timeout);
	fflush(fhTraceFile);
	LeaveCriticalSection2(&g_csWrite);
#endif
}


