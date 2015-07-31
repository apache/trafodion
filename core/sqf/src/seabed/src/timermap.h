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
// Implement timer map
//

#ifndef __SB_TIMERMAP_H_
#define __SB_TIMERMAP_H_

#include "seabed/int/opts.h"

#include "seabed/otimer.h"

#include "mapcom.h"

class SB_TimerMap;
class SB_TimerMap_Enum;

//
// A user of SB_TimerMap needs to embed SB_TML_Type into an 'object'
// that is to be managed by the map.
//
// The contents of SB_TML_Type are managed by SB_TimerMap and SB_TimerMap_Enum.
//
class SB_TML_Type {
public:
    SB_TML_Type();
    // this callback is called on a timeout
    typedef void (*Timeout_Cb_Type)(long pv_user_param, void *pp_item);

private:
    friend class SB_TimerMap;
    friend class SB_TimerMap_Enum;
    SB_TML_Type          *ip_next;       // next link (higher pop-time)
    SB_TML_Type          *ip_prev;       // prev link
    void                 *ip_item;       // user's item
    int                   iv_hash;       // hash
    SB_Timer::Time_Stamp  iv_pop_time;   // pop time
    bool                  iv_running;    // running?
    int                   iv_qid;        // qid
    int                   iv_qid_last;   // last qid
    int                   iv_tics;       // user's tics
    Timeout_Cb_Type       iv_to_cb;      // user's timeout callback
    long                  iv_user_param; // user's param
};

//
// This is a timer map.
// The map is ordered by timestamp.
// Link/items are added to hash-chains.
// Add time is dependent on timeout and hash-chain length.
// Remove time is independent of hash-chain length.
//
// constructors:
//   pp_name   : name of map
//
// methods:
//   empty         : returns true if map is empty
//   get           : returns item given a id
//   init_link     : initialize SB_TML_Type
//   keys          : returns enumeration of items
//   lock          : lock map
//   printself     : prints map
//   put           : add item to map
//   remove        : remove item from map
//   removeall     : remove all items from map
//   removeall_del : remove all items from map (with delete cb)
//   size          : returns number of items in map
//   unlock        : unlock map
//   walk          : walk map (with walk cb)
//
class SB_TimerMap : public SB_Map_Comm {
public:
    // this cb is called from removeall_del(), note map is locked on cb
    typedef void (*Removeall_Del_Cb_Type)(long pv_user_param, void *pp_item);
    // this cb is called from walk(), note map is locked on cb
    typedef void (*Walk_Cb_Type)(long pv_user_param, void *pp_item);

    SB_TimerMap(int pv_interval);         // tic-count to check timers
    SB_TimerMap(const char *pp_name,      // map name
                int         pv_interval); // tic-count to check timers
    virtual ~SB_TimerMap();

    friend class SB_TimerMap_Enum;

    bool                   empty();
    virtual void          *get(SB_TML_Type *pp_link); // returns item (or NULL)
    virtual void          *get_lock(SB_TML_Type *pp_link, bool pv_lock);
    static void            init_link(SB_TML_Type *pp_link);
    virtual
    SB_TimerMap_Enum      *keys();
    virtual void           lock();
    virtual void           printself(bool pv_traverse);
    virtual void           put(SB_TML_Type                  *pp_link,
                               void                         *pp_item,
                               long                          pv_user_param,
                               SB_TML_Type::Timeout_Cb_Type  pv_to_cb,
                               int                           pv_tics);
    virtual void           put_lock(SB_TML_Type                  *pp_link,
                                    void                         *pp_item,
                                    long                          pv_user_param,
                                    SB_TML_Type::Timeout_Cb_Type  pv_to_cb,
                                    int                           pv_tics,
                                    bool                          pv_lock);
    virtual void          *remove(SB_TML_Type *pp_link); // returns item (or NULL)
    virtual void          *remove_lock(SB_TML_Type *pp_link, bool pv_lock);
    virtual void           removeall();
    virtual void           removeall_lock(bool pv_lock);
    virtual void           removeall_del(Removeall_Del_Cb_Type pv_cb);
    virtual void           removeall_del_lock(Removeall_Del_Cb_Type pv_cb,
                                              bool                  pv_lock);
    int                    size();
    virtual void           unlock();
    virtual void           walk(Walk_Cb_Type pv_cb);
    virtual void           walk_lock(Walk_Cb_Type pv_cb, bool pv_lock);

protected:
#ifdef TIMERMAP_CHECK
    void                   check_integrity();
#endif // TIMERMAP_CHECK
    int                    hash(SB_Timer::Time_Stamp &pv_ts);
    void                   init();
    void                   timer_map_cancel(SB_TML_Type *pp_link);
    void                   timer_map_check();
    static void            timer_master_cb(int   pv_tleid,
                                           int   pv_toval,
                                           short pv_parm1,
                                           long  pv_parm2);
    void                   timer_master_check();
    void                   timer_master_start();
    void                   timer_master_stop();

    enum                 { BUCKETS       = 8192 };
    enum                 { TICS_PER_SLOT =  256 };
    typedef unsigned int   Hash_Type;
    SB_Thread::ECM        *ip_mutex;
    SB_TML_Type          **ipp_HT;
    int                    iv_interval;
    Hash_Type              iv_last_hash_checked;
    SB_Timer::Time_Stamp   iv_time_last_check;
    bool                   iv_master_running;
    short                  iv_tleid;
};

class SB_TimerMap_Enum {
public:
    SB_TimerMap_Enum(SB_TimerMap *pp_map);
    virtual ~SB_TimerMap_Enum() {}

    virtual bool         more();
    virtual SB_TML_Type *next();

protected:
    SB_TimerMap_Enum() {}

private:
    SB_TML_Type *ip_item;
    SB_TimerMap *ip_map;
    int          iv_count;
    int          iv_hash;
    int          iv_inx;
    int          iv_mod;
};

#ifdef USE_SB_INLINE
#include "timermap.inl"
#endif

#endif // !__SB_TIMERMAP_H_
