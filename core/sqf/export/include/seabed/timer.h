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
// Timer module
//
#ifndef __SB_TIMER_H_
#define __SB_TIMER_H_

#include <unistd.h> // pid_t

#include "int/diag.h"
#include "int/exp.h"

#include "cc.h"

typedef void (*Timer_Cb_Type)(int tleid, int toval, short parm1, long parm2);
SB_Export int         timer_cancel(short tag)
SB_DIAG_UNUSED;
SB_Export int         timer_register()
SB_DIAG_UNUSED;
SB_Export int         timer_start_cb(int            toval,
                                     short          parm1,
                                     long           parm2,
                                     short         *tleid,
                                     Timer_Cb_Type  callback)
SB_DIAG_UNUSED;
SB_Export _xcc_status XCANCELTIMEOUT(short  tag)
SB_DIAG_UNUSED;
SB_Export _xcc_status XSIGNALTIMEOUT(int    toval,
                                     short  parm1,
                                     long   parm2,
                                     short *tleid,
                                     pid_t  tid = 0)
SB_DIAG_UNUSED;


#endif // !__SB_TIMER_H_
