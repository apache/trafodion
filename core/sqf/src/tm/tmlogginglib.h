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

#include "sqevlog/evl_sqlog_writer.h"
#include "common/evl_sqlog_eventnum.h"

#define DTM_STRING_BUF_SIZE 512
#define DTM_EVENT_BUF_SIZE 4096

int tm_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string);

#endif



