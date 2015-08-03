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

#ifndef __SB_RECVQ_H_
#define __SB_RECVQ_H_

#include "queuemd.h"

//
// Receive Queue
//
class SB_Recv_Queue : public SB_Ts_Md_Queue {
private:
    SB_Ecid_Type iv_aecid_recv_queue; // should be first instance

public:
    SB_Recv_Queue(const char *pp_name);
    ~SB_Recv_Queue() {}

    virtual void  add(SB_DQL_Type *pp_item);
    virtual void  add_at_front(SB_DQL_Type *pp_item);
    virtual void  lock();
    virtual char *printbuf(char *buf);
    virtual void  printself(bool pv_traverse, bool pv_lock);
    virtual void *remove();
    virtual bool  remove_list(SB_DQL_Type *pp_item);
    virtual void  set_priority_queue(bool pv_pri_q);
    virtual void  unlock();

protected:
    bool iv_pri_q;

private:
    void          remove_account(void *pp_item);
};

#ifdef USE_SB_INLINE
#include "recvq.inl"
#endif

#endif // !__SB_RECVQ_H_
