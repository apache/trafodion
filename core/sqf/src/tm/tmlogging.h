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

#ifndef __TMLOGGING_H
#define __TMLOGGING_H

#include "dtm/tm_util.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "common/evl_sqlog_eventnum.h"

#define DTM_STRING_BUF_SIZE 512
#define DTM_EVENT_BUF_SIZE 4096

int tm_init_logging();

int tm_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string);

int tm_alt_log_write(int eventType, posix_sqlog_severity_t severity, char *msg);

int tm_log_event(int event_type, 
                 posix_sqlog_severity_t severity,
                 const char *temp_string, 
                 int error_code = -1, // 1
                 int rmid = -1,       // 2
                 int dtmid = -1,      // 3
                 int seq_num = -1,    // 4
                 int msgid = -1,      // 5
                 int xa_error = -1,   // 6
                 int pool_size = -1,  // 7
                 int pool_elems = -1, // 8
                 int msg_retries = -1,// 9
                 int pool_high = -1,  // 10
                 int pool_low = -1,   // 11
                 int pool_max = -1,   // 12
                 int tx_state = -1,   // 13
                 int data = -1,       // 14
                 int data1 = -1,      // 15
                 int64 data2 = -1,    // 16
                 const char *string1 = NULL,// 17   
                 int node = -1,       // 18
                 int msgid2 = -1,     // 19
                 int offset = -1,     // 20
                 int tm_event_msg = -1, // 21
                 uint data4 = 0);          //22

int tm_log_stdout
                (int event_type, 
                 posix_sqlog_severity_t severity,
                 const char *temp_string, 
                 int error_code = -1, // 1
                 int rmid = -1,       // 2
                 int dtmid = -1,      // 3
                 int seq_num = -1,    // 4
                 int msgid = -1,      // 5
                 int xa_error = -1,   // 6
                 int pool_size = -1,  // 7
                 int pool_elems = -1, // 8
                 int msg_retries = -1,// 9
                 int pool_high = -1,  // 10
                 int pool_low = -1,   // 11
                 int pool_max = -1,   // 12
                 int tx_state = -1,   // 13
                 int data = -1,       // 14
                 int data1 = -1,      // 15
                 int64 data2 = -1,// 16
                 const char *string1 = NULL,// 17   
                 int node = -1,       // 18
                 int msgid2 = -1,     // 19
                 int offset = -1,     // 20
                 int tm_event_msg = -1, // 21
                 uint data4 = 0);          //22

#endif



