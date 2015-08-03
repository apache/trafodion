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
// Implement queue, thread-safe queue, and signaled queue
//

#ifndef __SB_QUEUEMD_H_
#define __SB_QUEUEMD_H_

#include "queue.h"

#include "msi.h"  // needs queue.h

//
// Thread-safe MD-Queue (doubly-linked)
//
class SB_Ts_Md_Queue : public SB_Ts_D_Queue {
public:
    SB_Ts_Md_Queue(int pv_qid, const char *pp_name);
    virtual ~SB_Ts_Md_Queue();

    virtual void          add(SB_DQL_Type *pp_item);
    virtual void          add_at_front(SB_DQL_Type *pp_item);
    virtual void          add_list(SB_DQL_Type *pp_prev, SB_DQL_Type *pp_item);
    virtual SB_DQL_Type  *head();
    virtual void          lock();
    virtual char         *printbuf(char *pp_buf);
    virtual void          printself(bool pv_traverse, bool pv_lock);
    virtual void         *remove();
    virtual bool          remove_list(SB_DQL_Type *pp_item);
    virtual void          unlock();

protected:
    void                  remove_account(void *pp_item);
};

//
// Signaling MD-Queue (doubly-linked)
//
class SB_Sig_Md_Queue : public SB_Sig_D_Queue {
public:
    SB_Sig_Md_Queue(int          pv_qid,
                    const char  *pp_name,
                    bool         pv_multi_reader);
    virtual ~SB_Sig_Md_Queue();

    virtual void  add(SB_DQL_Type *pp_item);
    virtual void  add_at_front(SB_DQL_Type *pp_item);
    virtual void  add_list(SB_DQL_Type *pp_prev, SB_DQL_Type *pp_item);
    virtual void  lock();
    virtual void  printself(bool pv_traverse, bool pv_lock);
    virtual void *remove();
    virtual void  unlock();
};

#ifdef USE_SB_INLINE
#include "queuemd.inl"
#endif

#endif // !__SB_QUEUEMD_H_
