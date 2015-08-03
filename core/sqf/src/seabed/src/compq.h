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

#ifndef __SB_COMPQ_H_
#define __SB_COMPQ_H_

#include "queuemd.h"

//
// Completion Queue
//
class SB_Comp_Queue : public SB_Ts_Md_Queue {
public:
    SB_Comp_Queue(int pv_qid, const char *pp_name);
    ~SB_Comp_Queue() {}

    virtual void  add(SB_DQL_Type *pp_item);
    virtual char *printbuf(char *pp_buf);
    virtual void  printself(bool pv_traverse, bool pv_lock);
    virtual void *remove();
    virtual bool  remove_list(SB_DQL_Type *pp_item);
    virtual bool  remove_list_lock(SB_DQL_Type *pp_item, bool pv_lock);

protected:
    pid_t iv_tid;

private:
    void          remove_account(void *pp_item);
};

#ifdef USE_SB_INLINE
#include "compq.inl"
#endif

#endif // !__SB_COMPQ_H_
