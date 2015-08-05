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

//  Class definition for XARM Subordinate RM.

#ifndef CTMXARM_H_
#define CTMXARM_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/atomic.h"

#include "dtm/tmtransid.h"
#include "tmaudit.h"
#include "tmlibmsg.h"
#include "tmrm.h"
#include "tmmap.h"
#include "tmdeque.h"
#include "tmsync.h"
#include "tmtxmsg.h"
#include "tmtxkey.h"
#include "tmtimer.h"
#include "tmpoolelement.h"
#include "tmpool.h"
#include "tmtxstats.h"
#include "tmtxthread.h"
#include "dtm/xa.h"
#include "tmtxbase.h"
#include "tmxidmap.h"

extern XID_MAP *gp_xarmXIDList;

class CTmXaRm
{
private:
   int iv_rmid;
   char iv_openInfo[MAXINFOSIZE];
   int64 iv_flags;
   TM_MAP iv_txnList;

public:
   CTmXaRm(int pv_rmid, char *pp_openInfo, int64 pv_flags)
      :iv_rmid(pv_rmid), iv_flags(pv_flags)
   {
      strncpy((char *) &iv_openInfo, pp_openInfo, sizeof(iv_openInfo));
   }
   ~CTmXaRm() {}

   int rmid() {return iv_rmid;}
   void rmid(int pv_rmid) {iv_rmid = pv_rmid;}
   CTmXaTxn *getTxn(XID *pp_xid) 
      {return (CTmXaTxn *) gp_xarmXIDList->get(pp_xid);}
};

#endif //CTMXARM_H_
