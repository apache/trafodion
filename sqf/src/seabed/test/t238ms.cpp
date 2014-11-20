//------------------------------------------------------------------
//
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "seabed/logalt.h"

#include "common/evl_sqlog_eventnum.h"

int main() {
    char        cat[BUFSIZ];
    char        host[BUFSIZ];
    char        log[BUFSIZ];
    const char *prefix;

    printf("stderr\n");
    SBX_log_write(SBX_LOG_TYPE_STDERR,         // log_type
                  NULL,                        // dir
                  "log-prefix1",               // log_file_prefix
                  SQEVL_SEABED,                // comp_id
                  SB_EVENT_ID,                 // event_id
                  SQ_LOG_SEAQUEST,             // facility
                  SQ_LOG_CRIT,                 // severity
                  "$test1",                    // name
                  "msg-prefix1",               // msg_prefix
                  "msg1",                      // msg
                  NULL,                        // snmptrap_cmd,
                  NULL,                        // msg_snmptrap
                  NULL,                        // msg_ret
                  0);                          // msg_ret_size
    printf("\n");

    printf("stderr-pstack\n");
    prefix = "log-prefix2";
    SBX_log_write(SBX_LOG_TYPE_STDERR |        // log_type
                  SBX_LOG_TYPE_STDERR_PSTACK,
                  NULL,                        // dir
                  prefix,                      // log_file_prefix
                  SQEVL_SEABED,                // comp_id
                  SB_EVENT_ID,                 // event_id
                  SQ_LOG_SEAQUEST,             // facility
                  SQ_LOG_CRIT,                 // severity
                  "$test2",                    // name
                  "msg-prefix2",               // msg_prefix
                  "msg2",                      // msg
                  NULL,                        // snmptrap_cmd,
                  NULL,                        // msg_snmptrap
                  NULL,                        // msg_ret
                  0);                          // msg_ret_size
    printf("stderr-pstack-output\n");
    printf("\n");

    printf("stdout\n");
    SBX_log_write(SBX_LOG_TYPE_STDOUT,         // log_type
                  NULL,                        // dir
                  "log-prefix3",               // log_file_prefix
                  SQEVL_SEABED,                // comp_id
                  SB_EVENT_ID,                 // event_id
                  SQ_LOG_SEAQUEST,             // facility
                  SQ_LOG_CRIT,                 // severity
                  "$test3",                    // name
                  "msg-prefix3",               // msg_prefix
                  "msg3",                      // msg
                  NULL,                        // snmptrap_cmd,
                  NULL,                        // msg_snmptrap
                  NULL,                        // msg_ret
                  0);                          // msg_ret_size
    printf("\n");

    printf("stdout-pstack\n");
    prefix = "log-prefix4";
    SBX_log_write(SBX_LOG_TYPE_STDOUT |        // log_type
                  SBX_LOG_TYPE_STDOUT_PSTACK,
                  NULL,                        // dir
                  prefix,                      // log_file_prefix
                  SQEVL_SEABED,                // comp_id
                  SB_EVENT_ID,                 // event_id
                  SQ_LOG_SEAQUEST,             // facility
                  SQ_LOG_CRIT,                 // severity
                  "$test4",                    // name
                  "msg-prefix4",               // msg_prefix
                  "msg4",                      // msg
                  NULL,                        // snmptrap_cmd,
                  NULL,                        // msg_snmptrap
                  NULL,                        // msg_ret
                  0);                          // msg_ret_size
    printf("stdout-pstack-output\n");
    printf("\n");

    printf("logfile\n");
    prefix = "log-prefix5";
    SBX_log_write(SBX_LOG_TYPE_LOGFILE,        // log_type
                  NULL,                        // dir
                  prefix,                      // log_file_prefix
                  SQEVL_SEABED,                // comp_id
                  SB_EVENT_ID,                 // event_id
                  SQ_LOG_SEAQUEST,             // facility
                  SQ_LOG_CRIT,                 // severity
                  "$test5",                    // name
                  "msg-prefix5",               // msg_prefix
                  "msg5\n",                    // msg
                  NULL,                        // snmptrap_cmd,
                  NULL,                        // msg_snmptrap
                  NULL,                        // msg_ret
                  0);                          // msg_ret_size
    printf("logfile-output-%s\n", prefix);
    gethostname(host, sizeof(host));
    sprintf(log, "%s.%s.%d.log", prefix, host, getpid());
    sprintf(cat, "cat %s", log);
    system(cat);
    printf("\n");
    unlink(log);

    printf("logfile\n");
    prefix = "log-prefix6";
    SBX_log_write(SBX_LOG_TYPE_LOGFILE |       // log_type
                  SBX_LOG_TYPE_LOGFILE_PERSIST,// log_type
                  NULL,                        // dir
                  prefix,                      // log_file_prefix
                  SQEVL_SEABED,                // comp_id
                  SB_EVENT_ID,                 // event_id
                  SQ_LOG_SEAQUEST,             // facility
                  SQ_LOG_CRIT,                 // severity
                  "$test6",                    // name
                  "msg-prefix6-1",             // msg_prefix
                  "msg6-1\n",                  // msg
                  NULL,                        // snmptrap_cmd,
                  NULL,                        // msg_snmptrap
                  NULL,                        // msg_ret
                  0);                          // msg_ret_size
    SBX_log_write(SBX_LOG_TYPE_LOGFILE |       // log_type
                  SBX_LOG_TYPE_LOGFILE_PERSIST,// log_type
                  NULL,                        // dir
                  prefix,                      // log_file_prefix
                  SQEVL_SEABED,                // comp_id
                  SB_EVENT_ID,                 // event_id
                  SQ_LOG_SEAQUEST,             // facility
                  SQ_LOG_CRIT,                 // severity
                  "$test6",                    // name
                  "msg-prefix6-2",             // msg_prefix
                  "msg6-2\n",                  // msg
                  NULL,                        // snmptrap_cmd,
                  NULL,                        // msg_snmptrap
                  NULL,                        // msg_ret
                  0);                          // msg_ret_size
    printf("logfile-output-%s\n", prefix);
    gethostname(host, sizeof(host));
    sprintf(log, "%s.%s.%d.log", prefix, host, getpid());
    sprintf(cat, "cat %s", log);
    system(cat);
    printf("\n");
    unlink(log);

    printf("logfile-pstack\n");
    prefix = "log-prefix7";
    SBX_log_write(SBX_LOG_TYPE_LOGFILE |       // log_type
                  SBX_LOG_TYPE_LOGFILE_PSTACK,
                  NULL,                        // dir
                  prefix,                      // log_file_prefix
                  SQEVL_SEABED,                // comp_id
                  SB_EVENT_ID,                 // event_id
                  SQ_LOG_SEAQUEST,             // facility
                  SQ_LOG_CRIT,                 // severity
                  "$test7",                    // name
                  "msg-prefix7",               // msg_prefix
                  "msg7",                      // msg
                  NULL,                        // snmptrap_cmd,
                  NULL,                        // msg_snmptrap
                  NULL,                        // msg_ret
                  0);                          // msg_ret_size
    printf("logfile-pstack-output\n");
    sprintf(log, "%s.%s.%d.log", prefix, host, getpid());
    sprintf(cat, "cat %s", log);
    system(cat);
    printf("\n");
    unlink(log);

    printf("logfile-syslog\n");
    prefix = "log-prefix8";
    SBX_log_write(SBX_LOG_TYPE_SYSLOG,         // log_type
                  NULL,                        // dir
                  prefix,                      // log_file_prefix
                  SQEVL_SEABED,                // comp_id
                  SB_EVENT_ID,                 // event_id
                  SQ_LOG_SEAQUEST,             // facility
                  SQ_LOG_CRIT,                 // severity
                  "$test8",                    // name
                  "msg-prefix8",               // msg_prefix
                  "msg8",                      // msg
                  NULL,                        // snmptrap_cmd,
                  NULL,                        // msg_snmptrap
                  NULL,                        // msg_ret
                  0);                          // msg_ret_size
    printf("syslog-output - check /var/log/messages\n");
    printf("\n");

#if 0 // default snmptrap-cmd prints error
    printf("logfile-snmptrap-default-cmd\n");
    prefix = "log-prefix9";
    SBX_log_write(SBX_LOG_TYPE_SNMPTRAP,       // log_type
                  NULL,                        // dir
                  prefix,                      // log_file_prefix
                  SQEVL_SEABED,                // comp_id
                  SB_EVENT_ID,                 // event_id
                  SQ_LOG_SEAQUEST,             // facility
                  SQ_LOG_CRIT,                 // severity
                  "$test9",                    // name
                  NULL,                        // msg_prefix
                  NULL,                        // msg
                  NULL,                        // snmptrap_cmd,
                  "snmptrap-msg9",             // msg_snmptrap
                  NULL,                        // msg_ret
                  0);                          // msg_ret_size
    printf("snmptrap-output\n");
    printf("\n");
#endif

    printf("logfile-snmptrap-cmd\n");
    prefix = "log-prefix10";
    SBX_log_write(SBX_LOG_TYPE_SNMPTRAP,       // log_type
                  NULL,                        // dir
                  prefix,                      // log_file_prefix
                  SQEVL_SEABED,                // comp_id
                  SB_EVENT_ID,                 // event_id
                  SQ_LOG_SEAQUEST,             // facility
                  SQ_LOG_CRIT,                 // severity
                  "$test10",                   // name
                  NULL,                        // msg_prefix
                  NULL,                        // msg
                  "t238cmd",                   // snmptrap_cmd,
                  "snmptrap-msg10",            // msg_snmptrap
                  NULL,                        // msg_ret
                  0);                          // msg_ret_size
    printf("snmptrap-output\n");
    printf("\n");

    return 0;
}
