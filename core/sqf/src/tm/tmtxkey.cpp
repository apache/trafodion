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

#include <assert.h>
#include "tmrecov.h"
#include "tminfo.h"
#include "tmrm.h"
#include "tmtxkey.h"

// seabed includes
#include "seabed/ms.h"
#include "seabed/trace.h"
#include "common/sq_common.h"
#include "tmlogging.h"


//----------------------------------------------------------------------------
// CTmTxKey::transid
// Construct a transid from the key and return it.
//----------------------------------------------------------------------------
TM_Txid_Internal CTmTxKey::transid()
{
    TM_Txid_Internal lv_transid;
    memset(&lv_transid, 0, sizeof(lv_transid));
    lv_transid.iv_node = node();
    lv_transid.iv_seq_num = seqnum();
    lv_transid.iv_incarnation_num = gv_tm_info.incarnation_num();
    return lv_transid;
} //transid
