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
// sqstate ic counters
//

#ifndef __SB_MSICCTR_H_
#define __SB_MSICCTR_H_

#include "seabed/atomic.h"

#include "recvq.h"

extern SB_Recv_Queue gv_ms_lim_q;
extern SB_Recv_Queue gv_ms_recv_q;

//
// Encapsulate ms-ic counters
//
class MS_IC_Ctr {
public:
    MS_IC_Ctr();
    virtual ~MS_IC_Ctr();

    inline void ctr_bump_msgs_rcvd() {
        iv_msg_count_rcvd.add_val(1);
    }

    inline void ctr_bump_msgs_sent() {
        iv_msg_count_sent.add_val(1);
    }

    inline int ctr_get_limq_len() {
        return gv_ms_lim_q.size();
    }

    inline int ctr_get_limq_hi() {
        return gv_ms_lim_q.hi();
    }

    inline long long ctr_get_msgs_rcvd() {
        return iv_msg_count_rcvd.read_val();
    }

    inline long long ctr_get_msgs_sent() {
        return iv_msg_count_sent.read_val();
    }

    inline int ctr_get_recvq_len() {
        return gv_ms_recv_q.size();
    }

    inline int ctr_get_recvq_hi() {
        return gv_ms_recv_q.hi();
    }

private:
    SB_Atomic_Long_Long iv_msg_count_rcvd;
    SB_Atomic_Long_Long iv_msg_count_sent;

};

extern MS_IC_Ctr gv_ms_ic_ctr;

#endif // !__SB_MSICCTR_H_
