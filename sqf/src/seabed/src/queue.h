//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

//
// Implement queue, thread-safe queue, and signaled queue
//

#ifndef __SB_QUEUE_H_
#define __SB_QUEUE_H_

#include <stdio.h>

#include "seabed/int/opts.h"

#include "seabed/thread.h"

#include "ecid.h"
#include "qid.h"
#include "ql.h"
#include "util.h"

// comment to get non-normal output
#define PRINTSELF_NORMAL

// uncomment to allow MS_TRACE_QALLOC
#define SB_Q_ALLOW_QALLOC


extern void sb_queue_ql_init(SB_QL_Type *pp_ql);
extern void sb_queue_dql_init(SB_DQL_Type *pp_dql);

//
// Regular Queue
//
class SB_Queue {
private:
    SB_Ecid_Type iv_aecid_queue; // should be first instance

public:
    SB_Queue(const char *pp_name);
    virtual ~SB_Queue();

    virtual void  add(SB_QL_Type *pp_item);
    virtual void  add_at_front(SB_QL_Type *pp_item);
    virtual bool  empty();
    virtual int   hi();
    virtual void  printself(bool pv_traverse);
    virtual void *remove();
    virtual int   size();

protected:
    char        ia_q_name[40];
    SB_QL_Type *ip_head;
    SB_QL_Type *ip_tail;
    int         iv_count;
    int         iv_hi;
    bool        iv_multi_reader;
};

//
// Regular Queue (doubly-linked)
//
class SB_D_Queue {
private:
    SB_Ecid_Type iv_aecid_d_queue; // should be first instance

public:
    SB_D_Queue(int pv_qid, const char *pp_name);
    virtual ~SB_D_Queue();

    virtual void          add(SB_DQL_Type *pp_item);
    virtual void          add_at_front(SB_DQL_Type *pp_item);
    virtual void          add_list(SB_DQL_Type *pp_prev, SB_DQL_Type *pp_item);
    virtual bool          empty();
    virtual SB_DQL_Type  *head();
    virtual int           hi();
    virtual void          printself(bool pv_traverse);
    virtual void         *remove();
    virtual bool          remove_list(SB_DQL_Type *pp_item);
    virtual int           size();

protected:
    char         ia_d_q_name[40];
    SB_DQL_Type *ip_head;
    SB_DQL_Type *ip_tail;
    int          iv_count;
    int          iv_hi;
    int          iv_qid;
    bool         iv_multi_reader;
};

//
// Thread-safe Queue
//
class SB_Ts_Queue : public SB_Queue {
public:
    SB_Ts_Queue(const char *pp_name);
    virtual ~SB_Ts_Queue();

    virtual void          add(SB_QL_Type *pp_item);
    virtual void          add_at_front(SB_QL_Type *pp_item);
    virtual void          lock();
    virtual void          printself(bool pv_traverse);
    virtual void         *remove();
    virtual void          unlock();

protected:
    SB_Thread::CV  iv_cv;         // for protection
};

//
// Thread-safe Queue (doubly-linked)
//
class SB_Ts_D_Queue : public SB_D_Queue {
public:
    SB_Ts_D_Queue(int pv_qid, const char *pp_name);
    virtual ~SB_Ts_D_Queue();

    virtual void          add(SB_DQL_Type *pp_item);
    virtual void          add_at_front(SB_DQL_Type *pp_item);
    virtual void          add_list(SB_DQL_Type *pp_prev, SB_DQL_Type *pp_item);
    virtual void          lock();
    virtual void          printself(bool pv_traverse, bool pv_lock);
    virtual void         *remove();
    virtual bool          remove_list(SB_DQL_Type *pp_item);
    virtual void          unlock();

protected:
    SB_Thread::CV  iv_cv;         // for protection
};

//
// Signaling Queue
//
class SB_Sig_Queue : public SB_Ts_Queue {
public:
    SB_Sig_Queue(const char *pp_name, bool pv_multi_reader);
    virtual ~SB_Sig_Queue();

    virtual void  add(SB_QL_Type *pp_item);
    virtual void  add_at_front(SB_QL_Type *pp_item);
    virtual void  add_lock(SB_QL_Type *pp_item, bool pv_lock);
    virtual void  add_lock_at_front(SB_QL_Type *pp_item, bool pv_lock);
    virtual void  lock();
    virtual void  printself(bool pv_traverse);
    virtual void *remove();
    virtual void *remove_lock(bool pv_lock);
    virtual void  unlock();

private:
    int           iv_waiters; // Wait counter
};

//
// Signaling Queue (doubly-linked)
//
class SB_Sig_D_Queue : public SB_Ts_D_Queue {
public:
    SB_Sig_D_Queue(int pv_qid, const char *pp_name, bool pv_multi_reader);
    virtual ~SB_Sig_D_Queue();

    virtual void  add(SB_DQL_Type *pp_item);
    virtual void  add_at_front(SB_DQL_Type *pp_item);
    virtual void  add_list(SB_DQL_Type *pp_prev, SB_DQL_Type *pp_item);
    virtual void  add_lock(SB_DQL_Type *pp_item, bool pv_lock);
    virtual void  add_lock_at_front(SB_DQL_Type *pp_item, bool pv_lock);
    virtual void  add_lock_list(SB_DQL_Type *pp_prev,
                                SB_DQL_Type *pp_item,
                                bool         pv_lock);
    virtual void  lock();
    virtual void  printself(bool pv_traverse, bool pv_lock);
    virtual void *remove();
    virtual void *remove_lock(bool pv_lock);
    virtual void  unlock();

private:
    int           iv_waiters; // Wait counter
};

//
// Lock-free Queue
//
class SB_Lf_Queue : public SB_Queue {
public:
    SB_Lf_Queue(const char *pp_name);
    virtual ~SB_Lf_Queue();

    virtual void  add(SB_QL_Type *pp_item);
    virtual void  add_at_front(SB_QL_Type *pp_item);
    virtual bool  empty();
    virtual void  printself(bool pv_traverse);
    virtual void *remove();
    virtual int   size();

    typedef struct PT {
        struct PT     *ip_ptr;
        unsigned int   iv_count;
    } PT;

protected:
    typedef struct LFNT {
        PT          iv_next;
        SB_QL_Type *ip_data;
    } LFNT;
    PT      iv_head;
    PT      iv_tail;
};

#ifdef USE_SB_INLINE
#include "queue.inl"
#endif

#endif // !__SB_QUEUE_H_
