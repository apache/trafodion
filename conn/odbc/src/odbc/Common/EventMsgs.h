/*************************************************************************
*
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
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
**************************************************************************/
//
//  EventMsgs.h - Environment object
//

#ifndef EVENTMSGS_DEFINED
#define EVENTMSGS_DEFINED

#include "odbceventmsgutil.h"
#ifdef NSK_PLATFORM
#include "zsysc.h"
#else
#include "seabed/ms.h"
#endif

enum collector_type 
{ 
	COLL_INIT = 0,
	COLL_EMS,
	COLL_TRACE,
	COLL_RESSTATS
};


#include <seabed/trace.h>
#include <seabed/ms.h>

#ifdef NSK_ODBC_SRVR
// mxosrvr related trace variables
extern const char *gp_mxosrvr_trace_filename;
extern const char *gp_mxosrvr_env_trace_enable;
extern const char *gp_mxosrvr_env_trace_ems;
extern const char *gp_mxosrvr_env_trace_legacy;
extern bool        gv_mxosrvr_trace_enable;
extern bool        gv_mxosrvr_trace_ems;
extern bool        gv_mxosrvr_trace_legacy;
extern int mxosrvr_init_seabed_trace();
#endif

#ifdef NSK_AS
// mxoas related trace variables
extern const char *gp_mxoas_trace_filename;
extern const char *gp_mxoas_env_trace_enable;
extern const char *gp_mxoas_env_trace_ems;
extern bool        gv_mxoas_trace_enable;
extern bool        gv_mxoas_trace_ems;
extern int mxoas_init_seabed_trace();
#endif

#ifdef NSK_CFGSRVR
// mxocfg related trace variables
extern const char *gp_mxocfg_trace_filename;
extern const char *gp_mxocfg_env_trace_enable;
extern const char *gp_mxocfg_env_trace_ems;
extern bool        gv_mxocfg_trace_enable;
extern bool        gv_mxocfg_trace_ems;
extern int mxocfg_init_seabed_trace();
#endif



class ODBCMXEventMsg
{

private:
	HMODULE				hModule;
   char process_name[MS_MON_MAX_PROCESS_NAME];
   int nid, pid;
	SENDMSG_FNPTR		sendEventMsgPtr;
	char ClusterName[201];
	int NodeId;

public:
	ODBCMXEventMsg();
	ODBCMXEventMsg(bool isWms);
	~ODBCMXEventMsg();	
	short SendEventMsg(DWORD EventId, short EventLogType, DWORD Pid, char *ComponentName,
			char *ObjectRef, short nToken, ...);
	int  getComponentId();
	long getLongEventId(int type, int component_id, int event_id, int max_len);

#define EXT_FILENAME_LEN ZSYS_VAL_LEN_FILENAME
    enum experienceTypes {ADVANCED, BEGINNER};
    enum severityTypes {CRTCL, MAJOR, MINOR, INFRM};
    enum targetTypes {DIALOUT, DBADMIN, LOGONLY };
private:
    bool optionalTokens;
    experienceTypes experienceLevel;
    severityTypes severity;
    targetTypes target;
    void setOptionalTokens(bool flag) {optionalTokens = flag;}
    bool getOptionalTokens() {return optionalTokens;}
    void setExperienceLevelAdvanced() {experienceLevel = ADVANCED;}
    void setExperienceLevelBeginner() {experienceLevel = BEGINNER;}
    void setCritical() {severity = CRTCL;}
    void setMajor() {severity = MAJOR;}
    void setMinor() {severity = MINOR;}
    void setInform() {severity = INFRM;}
    void setDialout() {target = DIALOUT;}
    void setDbAdmin() {target = DBADMIN;}
    void setLogOnly() {target = LOGONLY;}
	bool isWms;
public:
	char ems_name[ EXT_FILENAME_LEN ];
	short ems_fnum;
	collector_type collectorType; 
	short open_ems_name( char* collector );
    short open_ems();
	void close_ems();
	void set_ems_name( char* collector);
	char* get_ems_name( void );
    void setCriticalDialout() 
    {
        setOptionalTokens(true);
        setExperienceLevelAdvanced(); setCritical(); setDialout(); 
    };
	void setIsWms() {isWms = true;}
	void resetIsWms() {isWms = false;}
	void send_to_eventlog (short evt_num, short EventLogType, char *ComponentName
								  , char *ObjectRef, short nToken, va_list vl);


};


// some UTF8 helper routines
extern bool isUTF8(const char *str);
extern char* strcpyUTF8(char *dest, const char *src, size_t destSize, size_t copySize=0);

#endif
