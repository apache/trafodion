// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
//
#include <platform_ndcs.h>
#include <platform_utils.h>
#include <time.h>

#include "Transport.h"
#include "tdm_odbcSrvrMsg.h"
#include "Listener.h"
#include "Global.h"
#include "TCPIPSystemSrvr.h"
#include "FileSystemSrvr.h"
#include "swap.h"

#ifndef NSK_CFGSRVR
#define TRACE_TCPIP
#endif

#ifdef  NSK_QS
#undef TRACE_TCPIP
#endif

#ifdef  NSK_STATS
#undef TRACE_TCPIP
#endif

#ifdef  NSK_COM
#undef TRACE_TCPIP
#endif

#ifdef  NSK_RULE
#undef TRACE_TCPIP
#endif

#ifdef  NSK_OFFNDR
#undef TRACE_TCPIP
#endif
 
//===================== CTimer ============================================
CTimer::CTimer(long seconds, long microseconds, void* e_routine, CEE_tag_def u_tag, 
				CEE_handle_def *thandle, short t_tag, CTimer* cnext)
{
	SRVRTRACE_ENTER(FILE_LSN+1);
	m_seconds = seconds;
	m_microseconds = microseconds;
	m_expiration_routine = e_routine;
	m_user_tag = u_tag;
	memcpy(&m_timer_handle, thandle, sizeof(CEE_handle_def));
	m_timer_tag = t_tag;
	m_pObject = NULL;
	next = cnext;
	SRVRTRACE_EXIT(FILE_LSN+1);
}
//LCOV_EXCL_START
CTimer::CTimer(long seconds, long microseconds, void* e_routine, CEE_tag_def u_tag, 
				CEE_handle_def *thandle, short t_tag, void* pObject, CTimer* cnext)
{
	SRVRTRACE_ENTER(FILE_LSN+1);
	m_seconds = seconds;
	m_microseconds = microseconds;
	m_expiration_routine = e_routine;
	m_user_tag = u_tag;
	memcpy(&m_timer_handle, thandle, sizeof(CEE_handle_def));
	m_timer_tag = t_tag;
	m_pObject = pObject;
	next = cnext;
	SRVRTRACE_EXIT(FILE_LSN+1);
}
//LCOV_EXCL_STOP
CTimer::~CTimer()
{
}
void CTimer::timer_restart(CEE_handle_def *thandle)
{
	SRVRTRACE_ENTER(FILE_LSN+2);
	SIGNALTIMEOUT(m_seconds * 100 + m_microseconds / 100, 0, (long)thandle, &m_timer_tag);
	SRVRTRACE_EXIT(FILE_LSN+2);
}

//===================== CTimer_list ========================================

CTimer_list::CTimer_list()
{
	SRVRTRACE_ENTER(FILE_LSN+3);
	list=NULL;
	memset(&m_timer_handle,0,sizeof(CEE_handle_def));
	SRVRTRACE_EXIT(FILE_LSN+3);
}
CTimer_list::~CTimer_list()
{
	SRVRTRACE_ENTER(FILE_LSN+4);
	CTimer* cnode = list;
	CTimer* nnode;
	while( cnode != NULL )
	{
		nnode = cnode->next;
		delete cnode;
		cnode = nnode;
	}
	list=NULL;
	SRVRTRACE_EXIT(FILE_LSN+4);
}

CEE_status CTimer_list::timer_create( long seconds,long microseconds, void* e_routine, CEE_tag_def u_tag, 
			CEE_handle_def *thandle, int receiveThrId )
{
	SRVRTRACE_ENTER(FILE_LSN+5);
	CTimer* cnode = list;
	CTimer* pnode = list;
	CTimer* nnode;
	static short t_tag = 0;
	_cc_status cc;

	ADD_ONE_TO_HANDLE(&m_timer_handle);
	memcpy(thandle, &m_timer_handle, sizeof(CEE_handle_def));
    cc = SIGNALTIMEOUT(seconds * 100 + microseconds / 100, 0, (long)thandle, &t_tag, receiveThrId);
	if (_status_lt (cc))
	{
//SIGNALTIMEOUT cannot allocate a time-list element (TLE).
		SRVRTRACE_EXIT(FILE_LSN+5);
		SendEventMsg( MSG_SET_SRVR_CONTEXT_FAILED,
						EVENTLOG_INFORMATION_TYPE,
						GetCurrentProcessId(),
						ODBCMX_SERVICE,
						"CTimer_list::timer_create",
						1,
						"SIGNALTIMEOUT cannot allocate a time-list element (TLE)."
				);
		return CEE_OBJECTINITFAILED;
	}
	else if (_status_gt (cc))
	{
//The given timeout value is illegal.
		SRVRTRACE_EXIT(FILE_LSN+5);
		SendEventMsg( MSG_PROGRAMMING_ERROR,
						EVENTLOG_INFORMATION_TYPE,
						GetCurrentProcessId(),
						ODBCMX_SERVICE,
						"CTimer_list::timer_create",
						1,
						"The given timeout value is illegal for SIGNALTIMEOUT."
				);
		return CEE_BADTIME;
	}
//the timer has been created
	while(cnode!=NULL )
	{
		pnode=cnode;
		cnode=cnode->next;
	}
	if((nnode = new CTimer( seconds, microseconds, e_routine, u_tag, thandle, t_tag, cnode))!=NULL)
	{
		if(pnode!=NULL) 
			pnode->next = nnode;
		else
			list = nnode;
	}
	else
		CANCELTIMEOUT ( t_tag );
	SRVRTRACE_EXIT(FILE_LSN+5);
	return (nnode == NULL)?CEE_OBJECTINITFAILED:CEE_SUCCESS;
}

void CTimer_list::timer_destroy(const CEE_handle_def *thandle)
{
	SRVRTRACE_ENTER(FILE_LSN+6);
	_cc_status cc;
	CTimer* cnode = find_timerByhandle(thandle);
	if( cnode==NULL)
	{
		SRVRTRACE_EXIT(FILE_LSN+6);
		return;
	}
	cc = CANCELTIMEOUT ( cnode->m_timer_tag );
	del_timer_node(cnode);
	SRVRTRACE_EXIT(FILE_LSN+6);
}
CTimer* CTimer_list::find_timerByhandle(const CEE_handle_def* handle)
{
	SRVRTRACE_ENTER(FILE_LSN+7);
	CTimer* cnode = list;
	CTimer* pnode = list;
	while( cnode!= NULL && memcmp(&cnode->m_timer_handle, handle,sizeof(CEE_handle_def)) != 0 )
	{
		pnode = cnode;
		cnode = cnode->next;
	}
	SRVRTRACE_EXIT(FILE_LSN+7);
	return cnode;
}
void CTimer_list::del_timer_node(CTimer* timer_node)
{
	SRVRTRACE_ENTER(FILE_LSN+8);
	CTimer* cnode = list;
	CTimer* pnode = list;
	while( cnode!= NULL && cnode != timer_node )
	{
		pnode = cnode;
		cnode = cnode->next;
	}
	if( cnode==NULL)
	{
		SRVRTRACE_EXIT(FILE_LSN+8);
		return;
	}
	if (pnode == list && cnode == list)
		list = cnode->next;
	else
		pnode->next = cnode->next;
	delete cnode;
	SRVRTRACE_EXIT(FILE_LSN+8);
}
//LCOV_EXCL_START
void CTimer_list::setObject(const CEE_handle_def* handle, void* pObject)
{
	CTimer* ctimer = find_timerByhandle(handle);
	if (ctimer != NULL)
		ctimer->m_pObject = pObject;
}
void* CTimer_list::getObject(const CEE_handle_def* handle)
{
	CTimer* ctimer = find_timerByhandle(handle);
	if (ctimer != NULL)
		return ctimer->m_pObject;
	return NULL;
}
//LCOV_EXCL_STOP
CNSKListener::CNSKListener(): m_TraceCount(0), m_bIPv4 (false)
{
	SRVRTRACE_ENTER(FILE_LSN+9);
	m_bKeepRunning = true;
	m_bTCPThreadKeepRunning = true;
	memset(&m_call_id,0,sizeof(CEE_handle_def));
	SRVRTRACE_EXIT(FILE_LSN+9);
}

CNSKListener::~CNSKListener()
{
}

void CNSKListener::CheckReceiveMessage(_cc_status &cc, int countRead, CEE_handle_def* call_id)
{
	SRVRTRACE_ENTER(FILE_LSN+10);
	request_def *request = (request_def*)m_RequestBuf;
	FS_Receiveinfo_Type receive_info;
	FILE_GETRECEIVEINFO_ ((TPT_RECEIVE)&receive_info );
	if (_status_gt(cc))
	{
// it is a system message
		if (request->request_code == ZSYS_VAL_SMSG_TIMESIGNAL)
		{
// signaltimeout message (-22)
			BUILD_TIMER_MSG_CALL((const CEE_handle_def *)&m_call_id, request, countRead, &receive_info);
		}
		else
		{
			BUILD_SYSTEM_MSG_CALL((const CEE_handle_def *)&m_call_id, request, countRead, &receive_info);
		}
	}
	else
	{
// it is a user message
		BUILD_USER_MSG_CALL((const CEE_handle_def *)&m_call_id, request, countRead, &receive_info);
	}
	SRVRTRACE_EXIT(FILE_LSN+10);
}

int 
CNSKListener::runProgram(char* TcpProcessName, long port, int TransportTrace)
{
	SRVRTRACE_ENTER(FILE_LSN+11);
	short fnum,error;
	_cc_status cc;

	if ((error = FILE_OPEN_("$RECEIVE",8,&m_ReceiveFnum, 0, 0, 1, MAX_REQUEST,0)) != 0)
	{
// (Could not open $RECEIVE: error %d", error)
		SET_ERROR((long)0, NSK, UNKNOWN_TRANSPORT, UNKNOWN_API, E_LISTENER, "runProgram", O_INIT_PROCESS, F_FILE_OPEN_, error, 0);
		return error;
	}
	
	INITIALIZE_TRACE(TransportTrace);

	unsigned short countRead;
	SB_Tag_Type tag;

	READUPDATEX(m_ReceiveFnum, m_RequestBuf, MAX_BUFFER_LENGTH );

	while(m_bKeepRunning)
	{
		RESET_ERRORS((long)0);

		ADD_ONE_TO_HANDLE(&m_call_id);

		fnum = -1;
		cc = AWAITIOX(&fnum,OMITREF, &countRead, &tag, -1);

		TRACE_INPUT(fnum,countRead,tag,cc);

		if (fnum == m_ReceiveFnum)
		{
			CheckReceiveMessage(cc, countRead, &m_call_id);
			FS_TRACE_OUTPUT(cc);
		}

		if (m_bKeepRunning)
			cc = READUPDATEX(m_ReceiveFnum, m_RequestBuf, MAX_BUFFER_LENGTH );
		else
		{
			FILE_CLOSE_(m_ReceiveFnum);
			return 1;
		}
	}
	SRVRTRACE_EXIT(FILE_LSN+11);
	return 0;
}	

extern CEE_status
CEE_TIMER_CREATE(
  /* In  */ long                         seconds,
  /* In  */ long                         microseconds,
  /* In  */ CEE_timer_expiration_ptr     expiration_routine,
  /* In  */ CEE_tag_def                  timer_tag,
  /* Out */ CEE_handle_def              *timer_handle
  )
{
	return GTransport.m_Timer_list->timer_create(seconds, microseconds, (void*)expiration_routine, timer_tag, timer_handle);
}

extern CEE_status
CEE_TIMER_CREATE2(
  /* In  */ long                         seconds,
  /* In  */ long                         microseconds,
  /* In  */ CEE_timer_expiration_ptr     expiration_routine,
  /* In  */ CEE_tag_def                  timer_tag,
  /* Out */ CEE_handle_def              *timer_handle,
  /* In  */ int                         receiveThrId
)
{
	return GTransport.m_Timer_list->timer_create(seconds, microseconds, (void*)expiration_routine, timer_tag, timer_handle, receiveThrId);
}

extern CEE_status
CEE_TIMER_DESTROY(
  /* In  */ const CEE_handle_def     *timer_handle
  )
{
	GTransport.m_Timer_list->timer_destroy(timer_handle);
	return CEE_SUCCESS;
}
//LCOV_EXCL_START
//==========================================================================
//
// Listener trace
//
//==========================================================================
static char* frmt_elapsed_time(struct timeb StartTime)
{
	static char frmt_buffer[50];
	struct timeb CurrentTime;
	long ElapsedSeconds;
	short ElapsedMilliSec;
	char *timeline;

   ftime(&CurrentTime);

	ElapsedSeconds=(long)((long)CurrentTime.time-(long)StartTime.time);
	ElapsedMilliSec=(signed short)(CurrentTime.millitm-StartTime.millitm);
	if(ElapsedMilliSec<0){
		ElapsedSeconds--;
		ElapsedMilliSec+=1000;
		}

	timeline = ctime( & ( CurrentTime.time ) );
	*(timeline + 20 + 4 ) = 0;
	*(timeline + 19 ) = 0;
	sprintf(frmt_buffer,"%.19s.%hu (%ld.%03d sec.)", &timeline[11], CurrentTime.millitm, ElapsedSeconds, ElapsedMilliSec );

	return frmt_buffer;
}

static char* frmt_current_time(struct timeb CurrentTime)
{
	static char frmt_buffer[50];
	char *timeline;

	timeline = ctime(&(CurrentTime.time));
	*(timeline + 20 + 4 ) = 0;
	*(timeline + 19 ) = 0;
	sprintf( frmt_buffer, "%.19s.%hu",&timeline[11],CurrentTime.millitm);
	return frmt_buffer;
}

static char* frmt_current_date(struct timeb CurrentTime)
{
	static char frmt_buffer[50];
	char *timeline;

	timeline = ctime(&(CurrentTime.time));
	sprintf( frmt_buffer, "%s",timeline);
	frmt_buffer[strlen(frmt_buffer) -1 ] = 0;
	return frmt_buffer;
}

//==================================================================
static char tabPrintable[] = {
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

static void FormatHexOutput( unsigned Offset,char *InIn, char *OutOut,int Length )
{
	int i,j;
	unsigned char *In = (unsigned char*) InIn;
	unsigned char *Out = (unsigned char*) OutOut;

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
	In = (unsigned char*)InIn;
	for( i = 0; i < Length; i++ )
	{
		char ch = tabPrintable[*In++];
		*Out++ = ch;
	}
//	*Out++ = '\n';
	*Out = '\0';
 
	return;
 
}  // end of FormatHexOutput


static void hex_out(  unsigned len,char *Data, char* TextString, FILE* fp )
{
	char Temp[90];
	char buffer[110];
	int i;
	char *pData = Data;
	int NumWholeLines;
	int PartialLineLength;
	unsigned int Offset;
	unsigned Len = len;

 
	if((int)Len <= 0) return;

	Len = ((Len + 1 + 16)/16)*16;
	Len = Len > 4096 ? 4096 : Len;

	Offset = 0;
 
 // Compute the number of lines and their sizes
	NumWholeLines = Len / 16;
	PartialLineLength = Len % 16;

	sprintf(Temp, "\r\n<%lX> %.70s, size:%d (0x%04X)", Data,TextString, len, len );

	fwrite(Temp, strlen(Temp),1,fp);
	fwrite("\r\n",2,1,fp);
	fflush(fp);


// output the whole lines
	for( i = 0; i < NumWholeLines; i++ )
	{
		FormatHexOutput( Offset, pData, Temp, 16 );
		sprintf(buffer, "%s", Temp );

		fwrite(buffer, strlen(buffer),1,fp);
		fwrite("\r\n",2,1,fp);
		fflush(fp);

		pData += 16;      
		Offset += 16;    
	}

 // output the partial line (if any)
	if( PartialLineLength )
	{
		FormatHexOutput( Offset, pData, Temp, PartialLineLength );
		sprintf(buffer, "%s", Temp );

		fwrite(buffer, strlen(buffer),1,fp);
		fwrite("\r\n",2,1,fp);
		fflush(fp);

	}

	return;
}

static void frmt_header(short maxlen, char* ibuffer, FILE* fp)
{
	int ip;
	char operation[40];
	char obuffer[1000];
	char* pbuffer = obuffer;
	HEADER hdr;
	memcpy(&hdr, ibuffer, sizeof(HEADER));
	if (hdr.signature != SIGNATURE)
		hex_out(  sizeof(HEADER), ibuffer, "UNKNOWN HEADER", fp );
	else
	{
		ip=sprintf(pbuffer,"\t<HEADER FORMAT>\n");
		operation[0] = 0;
		if (hdr.operation_id < AS_API_START )
			strcpy(operation,"UNKNOWN_API");
		else if (hdr.operation_id < CFG_API_START )
			FORMAT_AS_APIS(hdr.operation_id, operation);
		else if (hdr.operation_id < SRVR_API_START )
			FORMAT_CFG_APIS(hdr.operation_id, operation);
		else
			FORMAT_SRVR_APIS(hdr.operation_id, operation);
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%s(%d)\n","operation_id",operation,hdr.operation_id);
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%ld\n","dialogueId",hdr.dialogueId);
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%u\n","total_length",hdr.total_length);
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%u\n","cmp_length",hdr.cmp_length);
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t'%c'\n","compress_ind",hdr.compress_ind);
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t0x%02x\n","compress_type",hdr.compress_type);
		switch(hdr.hdr_type)
		{
		case WRITE_REQUEST_FIRST:
			pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\tWRITE_REQUEST_FIRST(%d)\n","hdr_type",hdr.hdr_type);
			break;
		case WRITE_REQUEST_NEXT:
			pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\tWRITE_REQUEST_NEXT(%d)\n","hdr_type",hdr.hdr_type);
			break;
		case READ_RESPONSE_FIRST:
			pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\tREAD_RESPONSE_FIRST(%d)\n","hdr_type",hdr.hdr_type);
			break;
		case READ_RESPONSE_NEXT:
			pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\tREAD_RESPONSE_NEXT(%d)\n","hdr_type",hdr.hdr_type);
			break;
		case CLEANUP:
			pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\tCLEANUP(%d)\n","hdr_type",hdr.hdr_type);
			break;
		case SRVR_TRANSPORT_ERROR:
			pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\tSRVR_TRANSPORT_ERROR(%d)\n","hdr_type",hdr.hdr_type);
			break;
		case CLOSE_TCPIP_SESSION:
			pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\tCLOSE_TCPIP_SESSION(%d)\n","hdr_type",hdr.hdr_type);
			break;
		default:
			pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\tUNKNOWN(%d)\n","hdr_type",hdr.hdr_type);
			break;
		}
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%u\n","signature",hdr.signature);
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%u\n","version",hdr.version);
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t'%c'\n","platform",hdr.platform);
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t'%c'\n","transport",hdr.transport);
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t'%c'\n","swap",hdr.swap);
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%d\n","error",hdr.error);
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%d\n","error_detail",hdr.error_detail);
		
		fwrite(obuffer, strlen(obuffer),1,fp);
		fwrite("\r\n",2,1,fp);
		fflush(fp);
	}

}

static void frmt_SMSG(FS_Receiveinfo_Type *receiveInfo, FILE* fp)
{
	char obuffer[1000];
	char* pbuffer = obuffer;
	int	ip;

	short filenum;
	TCPU_DECL(cpu); //short cpu;
	TPIN_DECL(pin);//short pin;
	
	int  node;
	short tag;
	short procname_len = 0,nodename_len = 0;
	char  procname[81];
	char  nodename[50];
	char  tbuffer[128];
	short maxlen = sizeof(procname)-1;
//	NSK_PROCESS_HANDLE pHandle;
	TPT_DECL(pHandle);

	filenum = receiveInfo->file_number;
	memcpy(TPT_REF(pHandle),&receiveInfo->sender,sizeof(pHandle));
	tag = receiveInfo->message_tag;

	short error = PROCESSHANDLE_DECOMPOSE_ (
					TPT_REF(pHandle)
					,&cpu				/*cpu*/
					,&pin			    /*pin*/
					,&node				/*nodenumber*/
					,(char*)&nodename   /*nodename:nmax*/
					,sizeof(nodename)
					,&nodename_len	    /*nlen*/
					,procname
					,maxlen
					,&procname_len);
	procname[procname_len] = 0;
	nodename[nodename_len] = 0;
	sprintf(tbuffer,"%s.%s",nodename,procname);
	strcpy(procname,tbuffer);

	ip=sprintf(pbuffer,"\t<MSG FORMAT>\n");
	pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%d\n","filenum",filenum);
	pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%d\n","cpu",cpu);
	pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%d\n","pin",pin);
	pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%d\n","node",node);
	pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%d\n","tag",tag);
	pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%s\n","process",procname);

	fwrite(obuffer, strlen(obuffer),1,fp);
	fwrite("\r\n",2,1,fp);
	fflush(fp);
}

char* frmt_serverstate(long serverstate)
{
	switch(serverstate)
	{
	case SRVR_UNINITIALIZED:
		return "SRVR_UNINITIALIZED";
		break;
	case SRVR_STARTING:
		return "SRVR_STARTING";
		break;
	case SRVR_STARTED:
		return "SRVR_STARTED";
		break;
	case SRVR_AVAILABLE:
		return "SRVR_AVAILABLE";
		break;
	case SRVR_CONNECTING:
		return "SRVR_CONNECTING";
		break;
	case SRVR_CONNECTED:
		return "SRVR_CONNECTED";
		break;
	case SRVR_DISCONNECTING:
		return "SRVR_DISCONNECTING";
		break;
	case SRVR_DISCONNECTED:
		return "SRVR_DISCONNECTED";
		break;
	case SRVR_STOPPING:
		return "SRVR_STOPPING";
		break;
	case SRVR_STOPPED:
		return "SRVR_STOPPED";
		break;
	case SRVR_ABENDED:
		return "SRVR_ABENDED";
		break;
	case SRVR_CONNECT_REJECTED:
		return "SRVR_CONNECT_REJECTED";
		break;
	case SRVR_CONNECT_FAILED:
		return "SRVR_CONNECT_FAILED";
		break;
	case SRVR_CLIENT_DISAPPEARED:
		return "SRVR_CLIENT_DISAPPEARED";
		break;
	case SRVR_ABENDED_WHEN_CONNECTED:
		return "SRVR_ABENDED_WHEN_CONNECTED";
		break;
	case SRVR_STATE_FAULT:
		return "SRVR_STATE_FAULT";
		break;
	case SRVR_STOP_WHEN_DISCONNECTED:
		return "SRVR_STOP_WHEN_DISCONNECTED";
		break;
	default:
		return "UNKNOWN";
	}
}

void CNSKListener::OpenTraceFile()
{
	struct timeb CurrentTime;

	short error;
//	short processId;
//	short cpuNumber;
	short errorDetail;
	short procname_len = 0;
	char  procname[81];
	short maxlen = sizeof(procname)-1;
	//NSK_PROCESS_HANDLE pHandle;
	TPT_DECL(pHandle);
	TCPU_DECL(cpuNumber);
	TCPU_DECL(processId); //processId

	if ((error = PROCESSHANDLE_NULLIT_ (TPT_REF(pHandle))) != 0)
	{
		m_TransportTrace = 0;
		return;
	}

	if ((error = PROCESSHANDLE_GETMINE_(TPT_REF(pHandle))) != 0)
	{
		m_TransportTrace = 0;
		return;
	}

	procname[0]=0;
	if ((error = PROCESSHANDLE_DECOMPOSE_ (
					TPT_REF(pHandle)
					,&cpuNumber			//[ short *cpu ]
					,&processId			//[ short *pin ]
					,OMITREF			//[ long *nodenumber ]
					,OMITREF			//[ char *nodename ]
					,OMITSHORT			//[ short maxlen ]
					,OMITREF			//[ short *nodename-length ]
					,procname			//[ char *procname ]
					,maxlen				//[ short maxlen ]
					,&procname_len		//[ short *procname-length ]
					,OMITREF			//[ long long *sequence-number ] 
					)) != 0)
	{
		m_TransportTrace = 0;
		return;
	}
	procname[procname_len] = 0;

	ftime(&CurrentTime);

	sprintf( m_TraceFileName, "TT%s",procname+1);
//	sprintf( m_TraceFileName, "TT%02d%03d",cpuNumber, processId);
	FILE* fp = fopen_guardian( m_TraceFileName,"w");
	if (fp != NULL)
	{
		fprintf(fp,"<==========SERVER TRACE (%s)==========>\n",frmt_current_date(CurrentTime));
		TCP_PROCESSNAME_PORT(fp);
		fprintf(fp,"<==========PROCESS %s (%02d,%03d) ==========>\n\n",procname, cpuNumber, processId);
		fflush(fp);
		fclose(fp);
	}
	else
		m_TransportTrace = 0;

}

void CNSKListener::TCP_PROCESSNAME_PORT(FILE* fp)
{
}

void CNSKListener::SYSTEM_SNAMP(FILE* fp)
{
	char obuffer[1000];
	char* pbuffer = obuffer;
	int	ip;

	ip=sprintf(pbuffer,"\t<----SYSTEM SNAP---->\n");

	if (GTransport.m_FSystemSrvr_list != NULL)
		pbuffer = GTransport.m_FSystemSrvr_list->enum_nodes(pbuffer,fp);
#if defined(TRACE_TCPIP) 
	if (GTransport.m_TCPIPSystemSrvr_list != NULL)
		pbuffer = GTransport.m_TCPIPSystemSrvr_list->enum_nodes(pbuffer,fp);
#endif
	fwrite(obuffer, strlen(obuffer),1,fp);
	fwrite("\r\n",2,1,fp);
	fflush(fp);
}

void CNSKListener::TraceInput(short fnum,int countRead,long tag,_cc_status cc)
{
	struct timeb CurrentTime;
	ftime(&CurrentTime);
	CTCPIPSystemSrvr* pnode;
	FILE* fp = NULL;

	if (fnum == m_ReceiveFnum) {
		request_def *request = (request_def*)m_RequestBuf;

		if (_status_gt(cc))
		{
			if (request->request_code == ZSYS_VAL_SMSG_TIMESIGNAL)
			{
// signaltimeout message (-22)
			}
			else
			{
// it is a system message
				if( request->request_code == ZSYS_VAL_SMSG_OPEN ) 
				{
					receive_info_def receive_info;
					FILE_GETRECEIVEINFO_ ((TPT_RECEIVE)&receive_info );	  

					if ((fp = OnCountReOpenFile()) != NULL) {
						fprintf(fp,"<------FS INPUT OPEN MESSAGE------>\n%s fnum=%d, countRead=%d, tag=%ld, cc=%d\n",frmt_current_time(CurrentTime),fnum, countRead, tag, cc);
						frmt_SMSG(&receive_info,fp);
					}
				}
				else if( request->request_code == ZSYS_VAL_SMSG_CLOSE ) 
				{
					receive_info_def receive_info;
					FILE_GETRECEIVEINFO_ ((TPT_RECEIVE)&receive_info );
					if ((fp = OnCountReOpenFile()) != NULL) {
						fprintf(fp,"<------FS INPUT CLOSE MESSAGE------>\n%s fnum=%d, countRead=%d, tag=%ld, cc=%d\n",frmt_current_time(CurrentTime),fnum, countRead, tag, cc);
						frmt_SMSG(&receive_info,fp);
					}
				}
			}
		}
		else
		{
// it is a user message
			if ((fp = OnCountReOpenFile()) != NULL) {
				fprintf(fp,"<------FS INPUT USER MESSAGE------>\n%s fnum=%d, countRead=%d, tag=%ld, cc=%d\n",frmt_current_time(CurrentTime),fnum, countRead, tag, cc);
				if (countRead >= sizeof(HEADER))
					frmt_header((short)countRead, m_RequestBuf, fp);
			}
		}
	}
#ifdef TRACE_TCPIP
	else if (GTransport.m_TCPIPSystemSrvr_list != NULL)
	{

		if (fnum == m_nListenSocketFnum) {
			if ((fp = OnCountReOpenFile()) != NULL)
				fprintf(fp,"<------TCPIP NEW SESSION------>\n%s fnum=%d, countRead=%d, tag=%ld, cc=%d\n",frmt_current_time(CurrentTime),fnum, countRead, tag, cc);
		}
		else if ((pnode=GTransport.m_TCPIPSystemSrvr_list->find_node(fnum)) != NULL)
		{
			if ((fp = OnCountReOpenFile()) != NULL) {
				fprintf(fp,"<------TCPIP INPUT------>\n%s fnum=%d, countRead=%d, tag=%ld, cc=%d\n",frmt_current_time(CurrentTime),fnum, countRead, tag, cc);
				if (countRead >= sizeof(HEADER))
					frmt_header(countRead, pnode->m_IObuffer, fp);
			}
		}
		else {
			if ((fp = OnCountReOpenFile()) != NULL)
				fprintf(fp,"<------TCPIP UNKNOWN INPUT------>\n%s fnum=%d, countRead=%d, tag=%ld, cc=%d\n",frmt_current_time(CurrentTime),fnum, countRead, tag, cc);
		}
	}
	else {
		if ((fp = OnCountReOpenFile()) != NULL)
			fprintf(fp,"<------TCPIP UNKNOWN INPUT------>\n%s fnum=%d, countRead=%d, tag=%ld, cc=%d\n",frmt_current_time(CurrentTime),fnum, countRead, tag, cc);
	}
#endif
	finish_trace(fp);
}

void CNSKListener::TraceUnknownInput()
{
	struct timeb CurrentTime;
	ftime(&CurrentTime);

	FILE* fp = NULL;

	if ((fp = OpenTraceFileA()) != NULL)
		fprintf(fp,"<------TRACE UNKNOWN INPUT------>\n%s\n",frmt_current_time(CurrentTime));

	finish_trace(fp);
}

void CNSKListener::TCPIPTraceOutput(HEADER* phdr)
{
	struct timeb CurrentTime;
	ftime(&CurrentTime);

	FILE* fp = NULL;

	if ((fp = OpenTraceFileA()) != NULL) {
		fprintf(fp,"<------TCPIP OUTPUT------>\n%s\n",frmt_current_time(CurrentTime));
		frmt_header(sizeof(HEADER), (char*)phdr, fp);
	}

	finish_trace(fp);
}

void CNSKListener::TCPIPTraceOutputR0()
{
	struct timeb CurrentTime;
	ftime(&CurrentTime);

	FILE* fp = NULL;

	if ((fp = OpenTraceFileA()) != NULL)
		fprintf(fp,"<------TCPIP OUTPUT R0------>\n%s\n",frmt_current_time(CurrentTime));

	finish_trace(fp);
}

void CNSKListener::TCPIPTraceOutputCC(_cc_status cc)
{
	struct timeb CurrentTime;
	ftime(&CurrentTime);

	FILE* fp = NULL;

	if ((fp = OpenTraceFileA()) != NULL)
		fprintf(fp,"<------TCPIP OUTPUT CC------>\n%s cc=%s(%d)\n",frmt_current_time(CurrentTime),DecodeNSKSocketErrors(cc),cc);

	finish_trace(fp);
}

void CNSKListener::FSTraceOutput(_cc_status cc)
{
	request_def *request = (request_def*)m_RequestBuf;
	CFSystemSrvr* node;

	struct timeb CurrentTime;
	ftime(&CurrentTime);

	FILE* fp = NULL;

	if (_status_gt(cc))
	{
		if (request->request_code == ZSYS_VAL_SMSG_TIMESIGNAL)
		{
// signaltimeout message (-22)
		}
		else
		{
// it is a system message
			if( request->request_code == ZSYS_VAL_SMSG_OPEN ) 
			{
				if ((fp = OpenTraceFileA()) != NULL)
					fprintf(fp,"<------FS OUTPUT OPEN MESSAGE------>\n%s cc=%d\n",frmt_current_time(CurrentTime),cc);
			}
			else if( request->request_code == ZSYS_VAL_SMSG_CLOSE ) 
			{
				if ((fp = OpenTraceFileA()) != NULL)
					fprintf(fp,"<------FS OUTPUT CLOSE MESSAGE------>\n%s cc=%d\n",frmt_current_time(CurrentTime),cc);
			}
		}
	}
	else
	{
		receive_info_def receive_info;
		FILE_GETRECEIVEINFO_ ((TPT_RECEIVE)&receive_info );

		if ((fp = OpenTraceFileA()) != NULL) {
			if (GTransport.m_FSystemSrvr_list != NULL)
			{
				node = GTransport.m_FSystemSrvr_list->find_node(&receive_info);
				if (node != NULL)
				{
					if (node->m_whdr.operation_id == 0)
						fprintf(fp,"<------FS OUTPUT USER MESSAGE (REPLYX only)------>\n%s\n",frmt_current_time(CurrentTime));
					else
					{
						fprintf(fp,"<------FS OUTPUT USER MESSAGE------>\n%s\n",frmt_current_time(CurrentTime));
						frmt_header(sizeof(HEADER), (char*)&node->m_whdr, fp);
					}
				}
			}

		}
	}
	finish_trace(fp);
}

void CNSKListener::ListenOnSocket(short socket)
{
	struct timeb CurrentTime;
	ftime(&CurrentTime);

	FILE* fp;

	if ((fp=OpenTraceFileA()) != NULL)
		fprintf(fp,"<------LISTEN ON SOCKET------>\n%s socket=%d\n",frmt_current_time(CurrentTime),socket);

	finish_trace(fp);
}

FILE* CNSKListener::OnCountReOpenFile()
{
	if (++m_TraceCount > 50)
	{
		m_TraceCount = 0;
		OpenTraceFile();
	}
	return OpenTraceFileA();
}

FILE* CNSKListener::OpenTraceFileA()
{
	FILE* fp = fopen_guardian( m_TraceFileName,"a");
	if (fp == NULL)
		m_TransportTrace = 0;
	return fp;
}

void CNSKListener::finish_trace(FILE* fp)
{
	if (m_TransportTrace && fp)
	{
		SYSTEM_SNAMP(fp);
		fflush(fp);
		fclose(fp);
	}
}
//LCOV_EXCL_STOP


