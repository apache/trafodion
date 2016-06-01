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

#ifndef __XARMMAIN_H_
#define __XARMMAIN_H_

#include <stddef.h>

#include "dtm/tm_util.h"
#include "seabed/ms.h"

#include "dtm/xa.h"
#include "tmxatxn.h"
#include "tmxarm.h"
#include "tmxidmap.h"

// List of XARM superior TMs by rmid and XID
TM_MAP *gp_xarmRmidList = NULL;
XID_MAP *gp_xarmXIDList = NULL;

void tm_xarm_initialize();
void tm_process_msg_from_xarm(CTmTxMessage * pp_msg);

CTmXaTxn *tm_lookup_xaTxn(int pv_rmid, XID *pp_xid);
CTmXaTxn *tm_new_xaTxn(int pv_rmid, XID *pp_xid, int pv_nid, int pv_pid);

CTmXaRm *tm_lookup_xaRM(int pv_rmid);
bool add_xaRM(CTmXaRm *pp_xarm);
void delete_xaRM(CTmXaRm *pp_xarm);

void tm_process_xa_start(CTmTxMessage * pp_msg);
void tm_process_xa_end(CTmTxMessage * pp_msg);
void tm_process_xa_commit(CTmTxMessage * pp_msg);
void tm_process_xa_prepare(CTmTxMessage * pp_msg);
void tm_process_xa_rollback(CTmTxMessage * pp_msg);
void tm_process_xa_open(CTmTxMessage * pp_msg);
void tm_process_xa_close(CTmTxMessage * pp_msg);
void tm_process_xa_recover(CTmTxMessage * pp_msg);
void tm_process_xa_forget(CTmTxMessage * pp_msg);
void tm_process_xa_complete(CTmTxMessage * pp_msg);
void tm_process_ax_reg(CTmTxMessage * pp_msg);
void tm_process_ax_unreg(CTmTxMessage * pp_msg);
#endif //__XARMMAIN_H_
