/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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
********************************************************************/

#include "WmsEvents.h"

#include "smxoevl.h"
#include <linux/unistd.h>

char process_name[MS_MON_MAX_PROCESS_NAME];
int nid, pid;
bool isWms;
bool consoleOn = false;
bool optionalTokens;
experienceTypes experienceLevel;
severityTypes severity;
targetTypes target;
bool trace_ems = false;
bool trace_legacy = false;

static void resetIsWms() {isWms = false;}
static void setOptionalTokens(bool flag) {optionalTokens = flag;}
static bool getOptionalTokens() {return optionalTokens;}
static void setExperienceLevelAdvanced() {experienceLevel = ADVANCED;}
static void setExperienceLevelBeginner() {experienceLevel = BEGINNER;}
static void setCritical() {severity = CRTCL;}
static void setMajor() {severity = MAJOR;}
static void setMinor() {severity = MINOR;}
static void setInform() {severity = INFRM;}
static void setDialout() {target = DIALOUT;}
static void setDbAdmin() {target = DBADMIN;}
static void setLogOnly() {target = LOGONLY;}

void set_is_wms() {isWms = true;}
void set_critical_dialout(){

    setOptionalTokens(true);
    setExperienceLevelAdvanced(); setCritical(); setDialout();
};
void set_trace_variables(bool trace_ems_dll, bool trace_legacy_dll ){
	trace_ems = trace_ems_dll;
	trace_legacy = trace_legacy_dll;
}

void send_to_eventlog (short evt_num, short EventLogType, char *ComponentName, char *ObjectRef, short nToken, va_list marker){

	BOOL	found = FALSE;
   short	i,j;
   int err, tokIdLen, severity, idx;
   char sqleventBuf[MAX_EVT_BUF_SIZE];
   char *TokenPtr;
   char tmpBuf[MS_MON_MAX_PROCESS_NAME + 64];
   size_t szBuf = MAX_EVT_BUF_SIZE;

   sqleventBuf[0] = '\0';
   string emsMsg;
   emsMsg.reserve(MAX_EVT_BUF_SIZE);

   if(getOptionalTokens())
   {
      printf("**************  getOptionalTokens() = TRUE *******************");
      printf("**************  contact SBOAND             *******************");
   }

   if (process_name[0] == 0)
   {
      msg_mon_get_my_process_name(process_name ,MS_MON_MAX_PROCESS_NAME );
      msg_mon_get_process_info(process_name, &nid, &pid);
   }

   for (i = 0, found = FALSE; eventDataMap[i].eventId != 0L ; i++)
   {
      if (eventDataMap[i].eventId == evt_num)
      {
         found = TRUE;
         //sprintf(tmpBuf, "%s (%d,%d): %d ",process_name, nid, pid, evt_num);
         //type = 1, component_id = 8, max_len = 9
         //sprintf(tmpBuf, "%s (%d,%d): %ld ",process_name, nid, pid, getLongEventId(1, 8, evt_num, 9));
         sprintf(tmpBuf, "%.30s (%d,%d): %ld ",process_name, nid, pid, getLongEventId(1, getComponentId(), evt_num, 9));
         emsMsg.append(tmpBuf);
         for (j=0;eventDataMap[i].msgPtr[j] !=NULL || j>6 ;j++ )
         {
            emsMsg.append(eventDataMap[i].msgPtr[j]);
         }
         break;
      }
   }
   if (!found)
   {
	  stringstream ss("ODBCMXEventMsg::send_to_eventlog: eventDataMap entry not found ");
      throw(ss.str()) ;
   }

   _itoa(EventLogType, tmpBuf, 10);

   if (findString("<1>",&idx, emsMsg))
      emsMsg.replace( emsMsg.find("<1>"), 3, tmpBuf );
   if (findString("<2>",&idx, emsMsg))
      emsMsg.replace( emsMsg.find("<2>"), 3, ComponentName );
   if (findString("<3>",&idx, emsMsg))
      emsMsg.replace( emsMsg.find("<3>"), 3, ObjectRef );

   for ( i=0,j=4; i<nToken; i++,j++ )
   {
      TokenPtr = va_arg(marker, char *);
      //errno = strstr(TokenPtr, "4126");
      tokIdLen = sprintf(tmpBuf,"<%d>",j);
      if (!findString(tmpBuf,&idx, emsMsg))
         emsMsg.append( TokenPtr );
      else
         emsMsg.replace( idx, tokIdLen, TokenPtr );
   }
   va_end(marker);

   const char *emsMsgStr = emsMsg.c_str();
   if (consoleOn)
   {
      char* type;
      if (EventLogType == EVENTLOG_ERROR_TYPE)
         type = "ERROR: " ;
      else
         type = "";

/***************************************************************************************************************************************
       time_t	LogTime;
       struct	tm	*LogTimePtr;
       struct	_timeb tstruct;
       LogTime = time(NULL);
       LogTimePtr = localtime(&LogTime);
       _ftime( &tstruct );
 	   printf("%02d:%02d:%02d.%u\t%s%s\n", LogTimePtr->tm_hour, LogTimePtr->tm_min, LogTimePtr->tm_sec,tstruct.millitm, type,emsMsgStr);
***************************************************************************************************************************************/
      printf("%s%s\n", type,emsMsgStr);
      return;
   }


/********************************************************************************
      if (err = evl_sqlog_init(sqleventBuf, szBuf))
      {
         //return err ?? fix;
         printf("ODBCMXEventMsg::send_to_eventlog: evl_sqlog_init err: %d ", err);
         return;
      }
********************************************************************************/

      //strut is defined for Monitor to pass values to common headers
      //Other seaquest modules won't use this struct

	  static sq_common_header_t ssq_header;
	  sq_common_header_t* sq_header = &ssq_header;
//    sq_header = (sq_common_header_t*)malloc(sizeof(sq_common_header_t));
      //sq_header->comp_id = SQEVL_NDCS;
      sq_header->comp_id = getComponentId();
      sq_header->process_id = getpid();
      sq_header->zone_id = 0;
      sq_header->thread_id = int((long int)syscall(__NR_gettid));

      // Instead of calling the previous function, Monitor and seabed will call the below function to initiate logging buffer
      if ( err = evl_sqlog_init_header(sqleventBuf, szBuf, sq_header) )
      {
    	 stringstream ss("ODBCMXEventMsg::send_to_eventlog: evl_sqlog_init_header err: ");
         ss << err << " ";
         throw(ss.str());
      }
   //err = evl_sqlog_add_token(sqleventBuf, TY_STRING, (void*)emsMsg.c_str() );
   err = evl_sqlog_add_token(sqleventBuf, TY_STRING, (void*)emsMsgStr );
   if (err)
   {
	  stringstream ss("ODBCMXEventMsg::send_to_eventlog: evl_sqlog_add_token err: ");
      ss << err << " ";
      throw(ss.str());
   }

   switch (EventLogType)
   {
   case EVENTLOG_ERROR_TYPE:
      severity = SQ_LOG_ERR;
      break;
   case EVENTLOG_WARNING_TYPE:
      severity = SQ_LOG_WARNING;
      break;
   case EVENTLOG_INFORMATION_TYPE:
      severity = SQ_LOG_INFO;
      break;
   default:
      severity = SQ_LOG_ERR;
   }
   if( (trace_ems == true ) || (trace_legacy && evt_num == MSG_SERVER_TRACE_INFO))
      trace_printf("%s\n",emsMsgStr);

   //err = evl_sqlog_write((posix_sqlog_facility_t)SQ_LOG_SEAQUEST, (int)evt_num, severity, sqleventBuf);
   //type = 1, component_id = 8, max_len = 9
   //err = evl_sqlog_write((posix_sqlog_facility_t)SQ_LOG_SEAQUEST, (int)getLongEventId(1, 8, evt_num, 9), severity, sqleventBuf);
   err = evl_sqlog_write((posix_sqlog_facility_t)SQ_LOG_SEAQUEST, (int)getLongEventId(1, getComponentId(), evt_num, 9), severity, sqleventBuf);
   if (err)
   {
	   stringstream ss;
	   ss << "evt_num= " <<  evt_num << " severity=" << severity << " emsMsg=" << emsMsg;
	   ss << " ODBCMXEventMsg::send_to_eventlog: evl_sqlog_write err: " << err << " ";
	   throw(ss.str());
   }
}



/*******************************************   fix make these statics
 format
 Event Number(event_type)  = %event_type:d%
 Component ID(componentID) = %componentID%
 Process ID(processID)     = %processID%
 Zone ID(zoneID)           = %zoneID%
 Thread ID(threadID)       = %threadID%
 Error Message             = %errMessage%

   int event_type = (int)evt_num;
   int componentID = SQEVL_NDCS;
   int processID = getpid();
   int zoneID = 0; //fix get zoneId
   int threadID = int((long int)syscall(__NR_gettid));
*******************************************/
