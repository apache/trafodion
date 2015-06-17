// ===============================================================================================
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
// ===============================================================================================

#include "PubInterface.h"
#include <seabed/trace.h>
#include "smxoevl.h"
#include "sqevlog/evl_sqlog_writer.h"

#include "CommonLogger.h"
extern const char CAT_MXOSRVR[];

#define MAX_EVT_BUF_SIZE 3900

// trace variables
static bool	gv_trace_ems_dll = false;
static bool	gv_trace_legacy_dll = false;

static std::string MXOSRVR_COMPONENT = "MXOSRVR";

void UpdateStringText(string& textStr)
{
    int pos = 0;
    while (pos != -1)
    {
        pos = textStr.find("'", pos);
        if (pos != -1)
        {
            if(textStr.substr(pos, 2).compare("''") != 0)
                textStr.insert(pos, "'");
			pos = pos + 2;
        }
    }
}

inline long getLongEventId(int type, int component_id, int event_id, int max_len)
{
   char *buff = new char[max_len+1];

   if (buff == 0)
      return -1;

   int len = sprintf(buff, "%01d%02d%06d", type, component_id, event_id);
   len = (len > max_len) ? max_len : len;
   buff[len] = '\0';
   long long_event_id = atol(buff);

   delete []buff;

   return long_event_id;
}

inline bool findString(char *p, int *idx, string msg)
{
   *idx = msg.find(p);
   if (*idx == string::npos)
   {
      return false;
   }
   return true;
   ;
}

void send_to_eventlog (short evt_num, short EventLogType, char *ComponentName, char *ObjectRef, short nToken, va_list marker)
{
	BOOL	found = FALSE;
   short	i,j;
   int err, tokIdLen, idx;
   char sqleventBuf[MAX_EVT_BUF_SIZE];
   char *TokenPtr;
   char tmpBuf[MS_MON_MAX_PROCESS_NAME + 64];
   size_t szBuf = MAX_EVT_BUF_SIZE;
   logLevel severity;

   sqleventBuf[0] = '\0';
   string logMsg;
   logMsg.reserve(MAX_EVT_BUF_SIZE);

   static char process_name[MS_MON_MAX_PROCESS_NAME];
   static int nid, pid;

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
         //sprintf(tmpBuf, "%.30s (%d,%d): %ld ",process_name, nid, pid, getLongEventId(1, SQEVL_NDCS, evt_num, 9));
         sprintf(tmpBuf, "Node Number: %d, CPU: %d, PIN:%ld, Process Name:%.30s , , ,", nid, nid, pid, process_name);
         logMsg.append(tmpBuf);
         for (j=0;eventDataMap[i].msgPtr[j] !=NULL || j>6 ;j++ )
         {
            logMsg.append(eventDataMap[i].msgPtr[j]);
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

//   if (findString("<1>",&idx, logMsg))
//      logMsg.replace( logMsg.find("<1>"), 3, tmpBuf );
//   if (findString("<2>",&idx, logMsg))
//      logMsg.replace( logMsg.find("<2>"), 3, ComponentName );
//   if (findString("<3>",&idx, logMsg))
//      logMsg.replace( logMsg.find("<3>"), 3, ObjectRef );

   for ( i=0,j=4; i<nToken; i++,j++ )
   {
      TokenPtr = va_arg(marker, char *);
      //errno = strstr(TokenPtr, "4126");
      tokIdLen = sprintf(tmpBuf,"<%d>",j);
      if (!findString(tmpBuf,&idx, logMsg))
         logMsg.append( TokenPtr );
      else
         logMsg.replace( idx, tokIdLen, TokenPtr );
   }
   va_end(marker);

   const char *logMsgStr = logMsg.c_str();

   switch (EventLogType)
   {
   case EVENTLOG_ERROR_TYPE:
      severity = LL_ERROR;
      break;
   case EVENTLOG_WARNING_TYPE:
      severity = LL_WARN;
      break;
   case EVENTLOG_INFORMATION_TYPE:
      severity = LL_INFO;
      break;
   default:
      severity = LL_ERROR;
   }

   CommonLogger::log(MXOSRVR_COMPONENT, severity, logMsgStr);
}

void SendEventMsg(DWORD EventId, short EventLogType, DWORD Pid, char *ComponentName,
			char *ObjectRef, short nToken, ...){

	va_list marker;
	va_start( marker, nToken);

	send_to_eventlog(EventId & 0x0FFFF, EventLogType, ComponentName, ObjectRef, nToken, marker);

	va_end(marker);

	return;
}

extern void setCriticalDialout() {

	//remove when logging is figured out
	return;
}

extern void setIsWms() {

	//remove when logging is figured out
	return;
}

#ifdef NSK_ODBC_SRVR
int mxosrvr_init_seabed_trace_dll()
{
	bool bret = true;

	const char *gp_mxosrvr_trace_filename         = "mxosrvr_trace_";
	const char *gp_mxosrvr_env_trace_enable       = "MXOSRVR_TRACE_ENABLE";	 // enable trace
	const char *gp_mxosrvr_env_trace_ems          = "MXOSRVR_TRACE_EMS";     // trace what used to be sent to the legacy ems collector
	const char *gp_mxosrvr_env_trace_legacy       = "MXOSRVR_TRACE_LEGACY";
	bool        gv_mxosrvr_trace_enable = false;

	msg_getenv_bool(gp_mxosrvr_env_trace_enable, &gv_mxosrvr_trace_enable);

	if(gv_mxosrvr_trace_enable)
	{
		msg_getenv_bool(gp_mxosrvr_env_trace_ems, &gv_trace_ems_dll);
		msg_getenv_bool(gp_mxosrvr_env_trace_legacy, &gv_trace_legacy_dll);

		trace_init((char*)gp_mxosrvr_trace_filename,
                  true,
                  NULL,
                  false);

	}
	return 0;
}
int mxosrvr_seabed_trace_close_dll()
{
   trace_close();
}
#endif

#ifdef NSK_AS
int mxoas_init_seabed_trace_dll()
{
	bool bret = true;

	const char *gp_mxoas_trace_filename         = "mxoas_trace_";
	const char *gp_mxoas_env_trace_enable       = "MXOAS_TRACE_ENABLE";	 // enable trace
	const char *gp_mxoas_env_trace_ems          = "MXOAS_TRACE_EMS";     // trace what used to be sent to the legacy ems collector
	bool        gv_mxoas_trace_enable = false;

	msg_getenv_bool(gp_mxoas_env_trace_enable, &gv_mxoas_trace_enable);

	if(gv_mxoas_trace_enable)
	{
      msg_getenv_bool(gp_mxoas_env_trace_ems, &gv_trace_ems_dll);

      trace_init((char*)gp_mxoas_trace_filename,
                  true,
                  NULL,
                  false);

	}
	return 0;
}
int mxoas_seabed_trace_close_dll()
{
   trace_close();
}
#endif

#ifdef NSK_CFGSRVR
int mxocfg_init_seabed_trace_dll()
{
	bool bret = true;

	const char *gp_mxocfg_trace_filename         = "mxocfg_trace_";
	const char *gp_mxocfg_env_trace_enable       = "MXOCFG_TRACE_ENABLE";	 // enable trace
	const char *gp_mxocfg_env_trace_ems          = "MXOCFG_TRACE_EMS";       // trace what used to be sent to the legacy ems collector
	bool        gv_mxocfg_trace_enable = false;

	msg_getenv_bool(gp_mxocfg_env_trace_enable, &gv_mxocfg_trace_enable);

	if(gv_mxocfg_trace_enable)
	{
		msg_getenv_bool(gp_mxocfg_env_trace_ems, &gv_trace_ems_dll);

		trace_init((char*)gp_mxocfg_trace_filename,
                  true,
                  NULL,
                  false);

   }
   return 0;
}
int mxocfg_seabed_trace_close_dll()
{
   trace_close();
}
#endif
