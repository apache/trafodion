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
// wrapper for monitor's clio.cxx
//

#include "seabed/int/opts.h"
#include "seabed/log.h"

#include "clio.cxx"

#include "chk.h"

void Local_IO_To_Monitor::log_error(int, int pv_severity, char *pp_buf) {
    int lv_err;

    lv_err = SB_log_write_str(SQEVL_SEABED,
                              SB_EVENT_ID,
                              SQ_LOG_SEAQUEST,
                              pv_severity,
                              pp_buf);
    CHK_ERRIGNORE(lv_err);
}

