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

#include "tminfo.h"
#include "tmlogging.h"



int tm_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string)
{
    size_t lv_buf_size = DTM_EVENT_BUF_SIZE;
    int    lv_err;
    char   lp_event_buf[DTM_EVENT_BUF_SIZE];
    char  *lp_pbuf = lp_event_buf;
  
    // init log buffer
    lv_err = evl_sqlog_init(lp_pbuf, lv_buf_size);
    if (lv_err)
        return lv_err;      

    // add our string
    lv_err = evl_sqlog_add_token(lp_pbuf, TY_STRING, pp_string);

    if (!lv_err)
    { 
        // ok to log buffer.
        // we need to translate category to sql_evl severity
        // facility is common for sql.

        lv_err = evl_sqlog_write((posix_sqlog_facility_t)SQ_LOG_SEAQUEST, pv_event_type, 
                                  pv_severity, lp_event_buf);
    }

    return lv_err;
}


