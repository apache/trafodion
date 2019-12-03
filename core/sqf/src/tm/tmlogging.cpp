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

#include <sys/time.h>
#include "common/evl_sqlog_eventnum.h"

#include "tminfo.h"
#include "tmlogging.h"
#include "seabed/logalt.h"

#include "CommonLogger.h"

int gv_dual_logging = 1; // Write to both SeaLog and stdout by default
static std::string TM_COMPONENT = "TM";
static bool gv_log4cxx_initialized = false;

int tm_init_logging()
{
    // Log4cxx logging
    CommonLogger::instance().initLog4cxx("log4cxx.trafodion.tm.config");
    gv_log4cxx_initialized = true;
    ms_getenv_int ("TM_DUAL_LOGGING", &gv_dual_logging);
    return gv_dual_logging; 
}

int tm_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *err_string, char *exception_stack, long transid)
{ 
    char      my_processName[MS_MON_MAX_PROCESS_NAME+1];
    int       my_nid,my_pid;
    logLevel ll_severity = LL_INFO;

    getTMLoggingHeaderInfo(pv_severity, ll_severity, my_processName, sizeof(my_processName), my_nid, my_pid);
    if (! gv_log4cxx_initialized) {
       tm_log_stdout(pv_event_type, pv_severity, err_string, 0, transid, 
          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, exception_stack);
       return -1;
    }
    if (exception_stack == NULL) 
       CommonLogger::log(TM_COMPONENT, ll_severity, "Node: %d Pid: %d Name: %s TransId: %Ld Event: %d Message: %s ", 
              my_nid, my_pid, my_processName, transid, pv_event_type, err_string);
    else
       CommonLogger::log(TM_COMPONENT, ll_severity, "Node: %d Pid: %d Name: %s TransId: %Ld Event: %d Message: Error at %s caused by exception %s ", 
              my_nid, my_pid, my_processName, transid, pv_event_type, err_string, exception_stack);
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
    char la_buf[DTM_STRING_BUF_SIZE];
    
    if (gv_dual_logging || (! gv_log4cxx_initialized))
    {   
        tm_log_stdout(event_id, severity, temp_string, error_code, -1, rmid, dtmid, seq_num, msgid, xa_error,
                      pool_size, pool_elems, msg_retries, pool_high, pool_low, pool_max, tx_state,
                      data, data1, data2, string1, node, msgid2, offset, tm_event_msg, data4);
        if (! gv_log4cxx_initialized)
           return -1;
    }
    char      my_processName[MS_MON_MAX_PROCESS_NAME+1];
    int       my_nid,my_pid;
    logLevel ll_severity = LL_INFO;
    getTMLoggingHeaderInfo(severity, ll_severity, my_processName, sizeof(my_processName), my_nid, my_pid);
    la_buf[0] = '\0';
    if (msgid != -1)
       sprintf(la_buf, ", msgid=%d",msgid);
    if (xa_error != -1)
       sprintf(la_buf+strlen(la_buf), ", XAERR=%d",xa_error);
    if (pool_size != -1)
       sprintf(la_buf+strlen(la_buf), ", pool_size=%d",pool_size);
    if (pool_elems != -1)
       sprintf(la_buf+strlen(la_buf), ", elements in pool=%d",pool_elems);
    if (msg_retries != -1)
       sprintf(la_buf+strlen(la_buf), ", msg retries=%d",msg_retries);
    if (pool_high != -1)
       sprintf(la_buf+strlen(la_buf), ", pool_high_ss=%d",pool_high);
    if (pool_low != -1)
       sprintf(la_buf+strlen(la_buf), ", pool_low_ss=%d",pool_low);
    if (pool_max != -1)
       sprintf(la_buf+strlen(la_buf), ", pool_max_size=%d",pool_max);
    if (tx_state != -1)
       sprintf(la_buf+strlen(la_buf), ", Txn State=%d",tx_state);
    if (data != -1)
       sprintf(la_buf+strlen(la_buf), ", data=%d",data);
    if (data1 != -1)
       sprintf(la_buf+strlen(la_buf), ", data1=%d",data1);
    if (data2 != -1)
       sprintf(la_buf+strlen(la_buf), ", data2=" PFLL,data2);
    if (node != -1)
       sprintf(la_buf+strlen(la_buf), ", node=%d",node);
    if (msgid2 != -1)
       sprintf(la_buf+strlen(la_buf), ", msgid2=%d",msgid2);
    if (offset != -1)
       sprintf(la_buf+strlen(la_buf), ", offset=%d",offset);
    if (tm_event_msg != -1)
       sprintf(la_buf+strlen(la_buf), ", tm_event_msg=%d",tm_event_msg);
    if (data4 != 0)
       sprintf(la_buf+strlen(la_buf), ", data4=%u",data4);
    
    CommonLogger::log(TM_COMPONENT, ll_severity, "Node: %d Pid: %d Name: %s TransId: %d,%d,%d Event: %d Message: %s %s %s", 
              my_nid, my_pid, my_processName, rmid, dtmid, seq_num, event_id, temp_string, (string1 == NULL ? "" : string1), la_buf);
    return rc;
}


int tm_log_stdout(int event_id, 
                 posix_sqlog_severity_t severity, 
                 const char *temp_string,
                 int error_code,
                 int64 transid,
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
    int       error = 0;

    logLevel ll_severity = LL_INFO;

    current_time = time(NULL);
    ctime_r(&current_time,timestamp);
    timestamp[strlen(timestamp)-1] = '\0';

    printf("%s  ", timestamp);

    getTMLoggingHeaderInfo(severity, ll_severity, my_name, sizeof(my_name), my_nid, my_pid);
    printf("(%s,%u,%u) ",my_name,my_nid,my_pid);
    printf("Event %s(%d), Sev ", temp_string, event_id);
    switch (severity)
    {
    case SQ_LOG_EMERG: printf("EMERGENCY"); 
               break;
    case SQ_LOG_ALERT: printf("ALERT"); 
               break;
    case SQ_LOG_CRIT: printf("CRITICAL"); 
               break;
    case SQ_LOG_ERR: printf("ERROR"); 
               break;
    case SQ_LOG_WARNING: printf("WARNING"); 
               break;
    case SQ_LOG_NOTICE: printf("NOTICE"); 
               break;
    case SQ_LOG_INFO: printf("INFO"); 
               break;
    case SQ_LOG_DEBUG: printf("DEBUG"); 
               break;
    default: printf("%d Unknown", severity);
    }

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

    return error;
} 



void getTMLoggingHeaderInfo(posix_sqlog_severity_t severity, logLevel &ll_severity, char *processName, int processNameLen, int &my_nid, int &my_pid)
{

    char      my_name[MS_MON_MAX_PROCESS_NAME+1];
    int       error;

    error = msg_mon_get_my_process_name( my_name, sizeof(my_name) );
    if (error == 0) {
      error = msg_mon_get_process_info( my_name, &my_nid, &my_pid );
      if (error != 0) {
         my_nid = -1;
         my_pid = -1;
      }
    }
    else
      strcpy(my_name, "UNKNOWN");
    int len = strlen(my_name);
    if (len < processNameLen)
        len = processNameLen-1;
    strncpy(processName, my_name, len);
    processName[len] = '\0';
    switch (severity) {
    case SQ_LOG_EMERG: 
       ll_severity = LL_FATAL;
       break;
    case SQ_LOG_ALERT:
       ll_severity = LL_WARN;
       break;
    case SQ_LOG_CRIT: 
       ll_severity = LL_FATAL;
       break;
    case SQ_LOG_ERR: 
       ll_severity = LL_ERROR;
       break;
    case SQ_LOG_WARNING: 
       ll_severity = LL_WARN;
       break;
    case SQ_LOG_NOTICE: 
       ll_severity = LL_INFO;
       break;
    case SQ_LOG_INFO: 
       ll_severity = LL_INFO;
       break;
    case SQ_LOG_DEBUG: 
       ll_severity = LL_DEBUG;
       break;
    default:
       ll_severity = LL_INFO;
       break;
   } 
   return;
}
