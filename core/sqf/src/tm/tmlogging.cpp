// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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

#include <sys/time.h>
#include "common/evl_sqlog_eventnum.h"

#include "tminfo.h"
#include "tmlogging.h"
#include "seabed/logalt.h"

#include "CommonLogger.h"

int gv_dual_logging =1; // Write to both SeaLog and stdout by default

int tm_init_logging()
{
	// Log4cpp logging
	CommonLogger::instance().initLog4cpp("log4cpp.trafodion.config");

    ms_getenv_int ("TM_DUAL_LOGGING", &gv_dual_logging);
    return gv_dual_logging; 
}

int tm_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string)
{
    int    lv_err = 0;
    return lv_err;
}

int tm_alt_log_write(int eventType, posix_sqlog_severity_t severity, char *msg) {
    static int logFileType = SBX_LOG_TYPE_LOGFILE;
    static char startTimeFmt[20] = "";

    char   logFileDir[PATH_MAX];
    char  *logFileDirPtr;
    char   logFilePrefix[25];
    char  *rootDir;

    struct timeval startTime;
    struct tm * ltime;

    if ((logFileType&SBX_LOG_TYPE_LOGFILE_PERSIST) != SBX_LOG_TYPE_LOGFILE_PERSIST)
    {
        // getting date time for log file name
        gettimeofday(&startTime, NULL);
        ltime = localtime(&startTime.tv_sec);
        sprintf(startTimeFmt, "%02d%02d%02d.%02d.%02d.%02d", ltime->tm_mon+1, ltime->tm_mday, ltime->tm_year-100, ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
    }

    // directory to write log file
    rootDir = getenv("MY_SQROOT");
    if (rootDir == NULL)
    {
        logFileDirPtr = NULL;
    }
    else
    {
        sprintf(logFileDir, "%s/logs", rootDir);
        logFileDirPtr = logFileDir;
    }

    // log file prefix will be tm.<date>.hh.mm.ss
    sprintf(logFilePrefix, "tm.%s", (char *)&startTimeFmt);

    SBX_log_write(logFileType,             // log_type
                  logFileDirPtr,           // log_file_dir
                  logFilePrefix,           // log_file_prefix
                  SQEVL_DTM,               // component id
                  eventType,               // event id
                  SQ_LOG_SEAQUEST,         // facility
                  severity,                // severity
                  "TM",                    // name
                  NULL,                    // msg_prefix
                  msg,                     // msg
                  NULL,                    // snmptrap_cmd
                  NULL,                    // msg_snmptrap
                  NULL,                    // msg_ret
                  0);                      // msg_ret size

    // write to the same file in future without opening and closing it.
    if (logFileType == SBX_LOG_TYPE_LOGFILE)
    {
        logFileType |= SBX_LOG_TYPE_LOGFILE_PERSIST;
    }
    return 0;
}

int tm_log_event(int event_id, 
                 posix_sqlog_severity_t severity, 
                 const char *temp_string,
                 int error_code,
                 int rmid,
                 int dtmid,
                 int seq_num,
                 int msgid,
                 int xa_error,
                 int pool_size,
                 int pool_elems,
                 int msg_retries,
                 int pool_high,
                 int pool_low,
                 int pool_max,
                 int tx_state,
                 int data,
                 int data1, 
                 int64 data2,
                 const char *string1,   
                 int node,
                 int msgid2,
                 int offset,
                 int tm_event_msg,
                 uint data4)

{
    int rc = 0;
    if (gv_dual_logging)
    {   
        char la_buf[DTM_STRING_BUF_SIZE];
        strncpy (la_buf, temp_string, DTM_STRING_BUF_SIZE - 1);
        tm_log_stdout(event_id, severity, la_buf, error_code, rmid, dtmid, seq_num, msgid, xa_error,
                      pool_size, pool_elems, msg_retries, pool_high, pool_low, pool_max, tx_state,
                      data, data1, data2, string1, node, msgid2, offset, tm_event_msg, data4);
    }

   return rc;
}


int tm_log_stdout(int event_id, 
                 posix_sqlog_severity_t severity, 
                 const char *temp_string,
                 int error_code,
                 int rmid,
                 int dtmid,
                 int seq_num,
                 int msgid,
                 int xa_error,
                 int pool_size,
                 int pool_elems,
                 int msg_retries,
                 int pool_high,
                 int pool_low,
                 int pool_max,
                 int tx_state,
                 int data,
                 int data1, 
                 int64 data2,
                 const char *string1,   
                 int node,
                 int msgid2,
                 int offset,
                 int tm_event_msg,
                 uint data4)

{
    time_t    current_time;
    char      timestamp[50];

    char      my_name[MS_MON_MAX_PROCESS_NAME];
    int       my_nid,my_pid;
    int       error;

	logLevel ll_severity = LL_INFO;

    current_time = time(NULL);
    ctime_r(&current_time,timestamp);
    timestamp[strlen(timestamp)-1] = '\0';

    printf("%s  ", timestamp);

    error = msg_mon_get_my_process_name( my_name, sizeof(my_name) );
    if (!error)
    {
      error = msg_mon_get_process_info( my_name, &my_nid, &my_pid );
      if (!error)
         printf("(%s,%u,%u) ",my_name,my_nid,my_pid);
      else
      {
         my_nid = -1; 
         my_pid = -1;
      }
    }
    else
      strcpy(my_name, "UNKNOWN");

    printf("Event %s(%d), Sev ", temp_string, event_id);
    switch (severity)
    {
    case SQ_LOG_EMERG: printf("EMERGENCY"); 
		ll_severity = LL_FATAL;
		break;
    case SQ_LOG_ALERT: printf("ALERT"); 
		ll_severity = LL_ALERT;
		break;
    case SQ_LOG_CRIT: printf("CRITICAL"); 
		ll_severity = LL_FATAL;
		break;
    case SQ_LOG_ERR: printf("ERROR"); 
		ll_severity = LL_ERROR;
		break;
    case SQ_LOG_WARNING: printf("WARNING"); 
		ll_severity = LL_WARN;
		break;
    case SQ_LOG_NOTICE: printf("NOTICE"); 
		ll_severity = LL_INFO;
		break;
    case SQ_LOG_INFO: printf("INFO"); 
		ll_severity = LL_INFO;
		break;
    case SQ_LOG_DEBUG: printf("DEBUG"); 
		ll_severity = LL_DEBUG;
		break;
    default: printf("%d Unknown", severity);
    }
    printf(", ");

    if (error_code != -1)
       printf(", Error=%d",error_code);
    if (rmid != -1)
       printf(", rmid=%d",rmid);
    if (dtmid != -1)
       printf(", dtmid=%d",dtmid);
    if (seq_num != -1)
       printf(", seqnum=%d",seq_num);
    if (msgid != -1)
       printf(", msgid=%d",msgid);
    if (xa_error != -1)
       printf(", XAERR=%d",xa_error);
    if (pool_size != -1)
       printf(", pool_size=%d",pool_size);
    if (pool_elems != -1)
       printf(", elements in pool=%d",pool_elems);
    if (msg_retries != -1)
       printf(", msg retries=%d",msg_retries);
    if (pool_high != -1)
       printf(", pool_high_ss=%d",pool_high);
    if (pool_low != -1)
       printf(", pool_low_ss=%d",pool_low);
    if (pool_max != -1)
       printf(", pool_max_size=%d",pool_max);
    if (tx_state != -1)
       printf(", Txn State=%d",tx_state);
    if (data != -1)
       printf(", data=%d",data);
    if (data1 != -1)
       printf(", data1=%d",data1);
    if (data2 != -1)
       printf(", data2=" PFLL,data2);
    if (string1 != NULL)
       printf(", string1=%s",string1);
    if (node != -1)
       printf(", node=%d",node);
    if (msgid2 != -1)
       printf(", msgid2=%d",msgid2);
    if (offset != -1)
       printf(", offset=%d",offset);
    if (tm_event_msg != -1)
       printf(", tm_event_msg=%d",tm_event_msg);
    if (data4 != 0)
       printf(", data4=%u",data4);
    printf("\n");

	// Log4cpp logging
	CommonLogger::log("TM", ll_severity, "Node Number: %u, CPU: %u, PIN: %u , Process Name: %s,,, Message: %s", my_nid, my_nid, my_pid, my_name, temp_string);

    return error;
} 
