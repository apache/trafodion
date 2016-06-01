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
#ifndef __TMTSE_H_
#define __TMTSE_H_
#include <stdarg.h>
#include "dtm/tm_util.h"
#include "dtm/tmtransid.h"
#include "dtm/xa.h"
#include "seabed/ms.h"

// TMTSE_MAXREPLYDATA is the maximum size of the pp_data field
// returned by TMTSE_GETREPLY[_EXT].
// Currently this is the size of an XID.  If TMTSE_GETREPLY 
// is changed to support other reply types with data then
// this value must be adjusted by hand.
#define TMTSE_MAXREPLYDATA 140

#define TM_TT_NOFLAGS           0x00
#define TM_TT_READ_ONLY         0x01  
#define TM_TT_NO_UNDO           0x02  
#define TM_TT_NO_CAPACITY_ABORT 0x04  // unsupported
#define TM_TT_NO_AUTO_ABORT     0x08  // default for DTM
#define TM_TT_LOAD_BALANCING    0x10  // unsupported
#define TM_TT_FORCE_CONSISTENCY 0x100 // Overrides NO_UNDO!
                                      // Note the Force Consistency flag is passthrough 
                                      //  - the TM does nothing with this.


// TMTSETYPE is returned by TMTSE_GETREPLY[_EXT] to indicate the
// type of request that this reply is for.
typedef enum {
   TMTSETYPE_UNDEFINED                  = 0,
   TMTSETYPE_DOOMTRANSACTION            = 1,
   TMTSETYPE_REGISTERTRANSACTION        = 2
} TMTSETYPE;

typedef enum {
    TMTSE_OPTN_NONE   = 0,
    TMTSE_OPTN_LDONEQ = BMSG_LINK_LDONEQ
} TMTSE_OPTNS;

extern "C" short DOOMTRANSACTION (int64 pv_transid, int *pp_msgid,
                                  TMTSE_OPTNS pv_optns=TMTSE_OPTN_LDONEQ);
extern "C" short DOOMTRANSACTION_EXT (TM_Transid_Type *pp_extTransid, 
                                      int *pp_msgid,
                                      TMTSE_OPTNS pv_optns=TMTSE_OPTN_LDONEQ);

extern "C" short GETTRANSACTIONNODE (int64 pv_transid, int *pp_node);
extern "C" short GETTRANSACTIONNODE_EXT (TM_Transid_Type *pp_extTransid,
                                         int *pp_node);

extern "C" short REGISTERTRANSACTION (int64 pv_transid, int pv_rmid,
                                      int64 pv_flags,  int *pp_msgid,
                                      TMTSE_OPTNS pv_optns=TMTSE_OPTN_NONE);
extern "C" short REGISTERTRANSACTION_EXT (TM_Transid_Type *pp_extTransid, int pv_rmid,
                                          int64 pv_flags, int * pp_msgid,
                                          TMTSE_OPTNS pv_optns=TMTSE_OPTN_NONE);

extern "C" bool  TMTSE_GETTRANSINFO (XID *pp_xid, int64 pv_TTbits); 
extern "C" int64 TMTSE_GETTRANSFLAGS (XID  *pp_xid);
extern "C" short TMTSE_GETREPLY (int pv_msgid, TMTSETYPE *pp_replyType, 
                                 short *pp_replyError, int64 *pp_transid,
                                 void *pp_data);
extern "C" short TMTSE_GETREPLY_EXT (int pv_msgid, TMTSETYPE *pp_replyType, 
                                     short *pp_replyError,
                                     TM_Transid_Type *pp_extTransid, 
                                     void *pp_data);
extern "C" short TMTSE_COMPLETE (int pv_msgid);
extern "C" short GETTRANSACTIONNODE (int64 pv_transid, 
                                     int *pp_node);
extern "C" short GETTRANSACTIONNODE_EXT (TM_Transid_Type *pp_extTransid,
                                         int *pp_node);
extern "C" short GETTRANSACTIONSEQNUM (int64 pv_transid, 
                                     int *pp_seqNum);
extern "C" short GETTRANSACTIONSEQNUM_EXT (TM_Transid_Type *pp_extTransid,
                                           int *pp_seqNum);

#endif // __TMTSE_H_
