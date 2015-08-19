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

#include <stdio.h>
#include <syslog.h>
#include <unistd.h>

#include "buf.h"
#include "logsys.h"
#include "util.h"

void SB_logsys_write_str(int                     pv_comp_id,
                         int                     pv_event_id,
                         posix_sqlog_facility_t  pv_facility,
                         posix_sqlog_severity_t  pv_severity,
                         char                   *pp_str) {
    SB_Buf_Line  la_ident;
    SB_Buf_Lline lv_cmdline;
    static bool  lv_init = true;

    if (lv_init) {
        lv_init = false;
        SB_util_get_cmdline(0,
                            false, // no args
                            &lv_cmdline,
                            lv_cmdline.size());
        sprintf(la_ident, "%s[%d]", &lv_cmdline, getpid());
        openlog(la_ident, 0, 0);
    }

    // <comp-id> <event-id> <facility> str
    syslog(pv_severity, "<%d> <%d> <%d> %s\n",
           pv_comp_id,
           pv_event_id,
           pv_facility,
           pp_str);
}

