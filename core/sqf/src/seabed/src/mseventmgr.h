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

#ifndef __SB_MSEVENTMGR_H_
#define __SB_MSEVENTMGR_H_

#include "seabed/int/opts.h"

#include "seabed/pctl.h"
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "chk.h"
#include "imap.h"
#include "lmap.h"

class SB_Ms_Tl_Event_Mgr;

//
// Access this through SB_Ms_Tl_Event_Mgr
//
class SB_Ms_Event_Mgr {
public:
    SB_Ms_Event_Mgr(long pv_id, int pv_tls_inx);
    ~SB_Ms_Event_Mgr();

    friend class SB_Ms_Tl_Event_Mgr;

    int          change_replies_done(int pv_change, int pv_mask);
    void         clear_event(int pv_mask);
    void         deregister_event(int pv_event);
    void         deregister_tls_dtor(int pv_inx);
    int          get_event(int pv_mask);
    void         get_event_str(char *pp_line, int pv_mask);
    bool         get_timedout(SB_Int64_Type *pp_curr_time);
    int          get_wait_time_rem(SB_Int64_Type pv_curr_time);
    long         get_wait_time_start(int pv_tics);
    void         print();
    void         print(FILE *pp_file);
    bool         register_event(int pv_event);
    bool         register_event_check(int pv_event);
    void         register_group_pin(int pv_group, int pv_pin);
    void         register_tls_dtor(int    pv_inx,
                                   void (*pp_dtor)(void *pp_data),
                                   void  *pp_data);
    void         remove_from_event_all();
    void         set_event(int pv_event, bool *pp_done);
    void         set_event_atomic(int pv_event, bool *pp_done);
    static void  set_event_pin(int pv_pin, int pv_event, int pv_fun);
    static void  set_trace_events(bool pv_trace_events);
    void         set_wait_start_time(int pv_tics, SB_Int64_Type *pp_curr_time);
    void         set_wait_time(int pv_tics);
    static void  shutdown();
    void         wait(long pv_us);

protected:
    void         call_tls_dtors();

private:
    typedef enum {
        EVENT_LREQ  = 0,
        EVENT_LDONE = 1
    } Event_Types;
    enum { EVENT_MAX = 2 };

    enum { TLS_DTOR_MAX = 2 };

    typedef struct Map_All_Entry_Type {
        SB_DQL_Type      iv_link;
        SB_Ms_Event_Mgr *ip_mgr;
    } Map_All_Entry_Type;
    typedef struct Map_Reg_Entry_Type {
        SB_ML_Type       iv_link;
        SB_Ms_Event_Mgr *ip_mgr;
    } Map_Reg_Entry_Type;
    typedef struct Map_Pin_Entry_Type {
        SB_ML_Type       iv_link;
        int              iv_group;
        SB_Ms_Event_Mgr *ip_mgr;
    } Map_Pin_Entry_Type;
    typedef struct Map_Group_Entry_Type {
        SB_ML_Type       iv_link;
        SB_Imap         *ip_map;
    } Map_Group_Entry_Type;
    typedef void (*Tls_Dtor_Type)(void *pp_data);
    typedef union Tls_Un_Dtor_Type {
        void            *ip_dtor;
        Tls_Dtor_Type    iv_dtor;
    } Tls_Un_Dtor_Type;
    static int              ca_hi[256];
    static SB_Lmap          ca_reg_map[EVENT_MAX];
    static SB_D_Queue       cv_all_list;
    static SB_Lmap          cv_all_map;
    static SB_Ts_Imap       cv_group_map;
    static SB_Imap          cv_pin_map;
    static SB_Thread::MSL   cv_sl_map;
    static bool             cv_trace_events;
    Map_All_Entry_Type      ia_reg_entry[EVENT_MAX];
    Tls_Un_Dtor_Type        ia_tls_dtor[TLS_DTOR_MAX];
    void                   *ia_tls_dtor_data[TLS_DTOR_MAX];
    SB_Ms_Event_Mgr        *ip_waiter_next;
    SB_Ms_Event_Mgr        *ip_waiter_prev;
    Map_All_Entry_Type      iv_all_list_entry;    // used in cv_group_map
    Map_All_Entry_Type      iv_all_map_entry;     // used in cv_group_map
    int                     iv_awake;
    int                     iv_awake_event;
    SB_Thread::CV           iv_cv;
    SB_Thread::ECM          iv_event_mutex;
    int                     iv_group;
    Map_Group_Entry_Type    iv_group_entry;       // used in cv_group_map
    Map_Pin_Entry_Type      iv_group_pin_entry;   // used in cv_group_map.iv_map
    long                    iv_id;
    bool                    iv_mutex_locked;
    int                     iv_pin;
    Map_Pin_Entry_Type      iv_pin_entry;         // used in cv_pin_map
    int                     iv_replies;
    int                     iv_tls_inx;
    SB_Int64_Type           iv_wait_start_time;
    long                    iv_wait_us;
};

class SB_Ms_Tl_Event_Mgr {
public:
    SB_Ms_Tl_Event_Mgr();
    ~SB_Ms_Tl_Event_Mgr();

    SB_Ms_Event_Mgr *get_gmgr();
    SB_Ms_Event_Mgr *get_mgr(bool *pp_first);
    SB_Ms_Event_Mgr *get_mgr_tid(int pv_tid);
    void             remove_from_event_all();
    static void      set_event_all(int pv_event);
    static void      set_event_reg(int pv_event, bool *pp_done);
    static void      set_ret_gmgr(bool pv_on);
    static void      set_trace_events(bool pv_trace_events);

private:
    static void      dtor(void *);

    static SB_Ms_Event_Mgr  *cp_gmgr;
    static bool              cv_ret_gmgr;
    static SB_Thread::MSL    cv_sl;
    static int               cv_tls_inx;
    static bool              cv_trace_events;
};

#ifdef USE_SB_INLINE
#include "mseventmgr.inl"
#endif

#endif // !__SB_MSEVENTMGR_H_

