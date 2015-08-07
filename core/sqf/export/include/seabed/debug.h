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
// Debug module
//
#ifndef __SB_DEBUG_H_
#define __SB_DEBUG_H_

#include "int/exp.h"

SB_Export void XDEBUG();

// backtrace callback
typedef enum {
    SB_BT_REASON_BEGIN = 1, // callback begin - str is NULL
    SB_BT_REASON_END   = 2, // callback end - str is NULL
    SB_BT_REASON_STR   = 3  // callback str - str contains string
} SB_BT_REASON;
typedef void (*SB_BT_CB)(SB_BT_REASON reason, const char *str);
// backtrace
SB_Export void SB_backtrace(SB_BT_CB callback);
SB_Export void SB_backtrace2(int   max_rec_count,
                             int   rec_size,
                             int  *rec_count,
                             char *records);

#endif // !__SB_DEBUG_H_
