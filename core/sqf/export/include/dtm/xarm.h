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
//  XARM Header file
//-------------------------------------------------------------------

#ifndef XARM_H_
#define XARM_H_
#include "dtm/xa.h"



// Globals

#define XARM_RM_NAME "SEAQUEST_RM_XARM"


// Default to XARM. xaTM_initialize will set to tse_switch  for TMs.
extern xa_switch_t xarm_switch;


// XARM generic interface to DTM
extern int xarm_xa_close (char *, int, int64);
extern int xarm_xa_commit (XID *, int, int64);
extern int xarm_xa_complete (int *, int *, int, int64);
extern int xarm_xa_end (XID *, int, int64);
extern int xarm_xa_forget (XID *, int, int64);
extern int xarm_xa_open (char *, int, int64);
extern int xarm_xa_prepare (XID *, int, int64);
extern int xarm_xa_recover (XID *, int64, int, int64);
extern int xarm_xa_rollback (XID *, int, int64);
extern int xarm_xa_start (XID *, int, int64);
#endif //XARM_H_
