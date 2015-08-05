//------------------------------------------------------------------
//
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
// Alternate logging module
//
#ifndef __SBX_LOGALT_H_
#define __SBX_LOGALT_H_

#include "sqevlog/evl_sqlog_writer.h"

//
// Basic log file format:
//   MM/DD/YYYY-HH:MM:SS.MMM.UUU: <msg_prefix> (name=<name>/pid=<pid>/tid=<tid>) (cmp=<component>/ev=<eventid>/fac=<facility>/sev=<severity>: msg
//

//
// SBX_LOG_TYPE_STDERR         - log via stderr
// SBX_LOG_TYPE_STDERR_PSTACK  - also generate a pstack via stderr
// SBX_LOG_TYPE_STDOUT         - log via stderr
// SBX_LOG_TYPE_STDOUT_PSTACK  - also generate a pstack via stdout
// SBX_LOG_TYPE_LOGFILE        - log via logfile
// SBX_LOG_TYPE_LOGFILE_PSTACK - also generate a pstack via logfile
// SBX_LOG_TYPE_SYSLOG         - log via syslog
// SBX_LOG_TYPE_SNMPTRAP       - log via snmptrap
//
enum {
    SBX_LOG_TYPE_STDERR          = 0x0001,
    SBX_LOG_TYPE_STDERR_PSTACK   = 0x0002,
    SBX_LOG_TYPE_STDOUT          = 0x0004,
    SBX_LOG_TYPE_STDOUT_PSTACK   = 0x0008,
    SBX_LOG_TYPE_LOGFILE         = 0x0010,
    SBX_LOG_TYPE_LOGFILE_PERSIST = 0x0020,
    SBX_LOG_TYPE_LOGFILE_PSTACK  = 0x0040,
    SBX_LOG_TYPE_SYSLOG          = 0x0080,
    SBX_LOG_TYPE_SNMPTRAP        = 0x0100
};

//
// environment-variables:
//   SBX_LOG_DEBUG         - if set and non-zero will produce debug output
//   SBX_LOG_SNMPTRAP_ADDR - use this snmptrap addr
//
// parameters:
//   log_type        - ORing of SBX_LOG_TYPE's above
//   log_file_dir    - IFF LOGFILE, specifies logfile-directory
//                   - default: current-working-directory
//   log_file_prefix - IFF LOGFILE, specifies logfile prefix
//                   - default: command-name
//   comp_id         - component-id
//   event_id        - event-id
//   facility        - facility
//   severity        - severity
//   name            - process name
//   msg_prefix      - message prefix
//   msg             - message
//   snmptrap_cmd    - IFF SNMPTRAP, snmptrap-cmd is used for command
//   msg_snmptrap    - IFF SNMPTRAP, msg_snmptrap is used for message
//   msg_ret         - returned log message
//   msg_ret_size    - returned log message size
//
void SBX_log_write(int                     log_type,
                   const char             *log_file_dir,
                   const char             *log_file_prefix,
                   int                     comp_id,
                   int                     event_id,
                   posix_sqlog_facility_t  facility,
                   posix_sqlog_severity_t  severity,
                   const char             *name,
                   const char             *msg_prefix,
                   const char             *msg,
                   const char             *snmptrap_cmd,
                   const char             *msg_snmptrap,
                   char                   *msg_ret,
                   size_t                  msg_ret_size);

#endif // !__SBX_LOGALT_H_
