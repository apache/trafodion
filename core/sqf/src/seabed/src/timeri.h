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

#ifndef __SB_TIMERI_H_
#define __SB_TIMERI_H_

typedef enum {
    TIMER_TLE_KIND_CB    = 1,
    TIMER_TLE_KIND_COMPQ = 2
} Timer_TLE_Kind_Type;
typedef enum {
    LIST_NONE = 0,
    LIST_TIMER,
    LIST_COMP
} Timer_List_Type;

#endif // !__SB_TIMERI_H_
