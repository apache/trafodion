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

#ifndef __SB_SLOTMGR_H_
#define __SB_SLOTMGR_H_

#include <stdio.h> // FILE

#include "seabed/int/opts.h"

#include "seabed/thread.h"

#include "ecid.h"

class SB_Slot_Mgr {
private:
    SB_Ecid_Type iv_aecid_slot_mgr; // should be first instance

public:
    typedef void (*Cb_Type)(FILE *pp_f, char *pp_str);
    enum {
        PRINT_ALL = 1,
        PRINT_USED = 2,
        PRINT_FREE = 3
    };
    typedef enum { // allocation method
        ALLOC_SCAN = 1, // scan for first free
        ALLOC_FAST = 2, // fastest possible alloc
        ALLOC_FIFO = 3, // fifo
        ALLOC_MIN  = 4  // find minimum
    } Alloc_Type;
    SB_Slot_Mgr(const char *pp_name, Alloc_Type pv_alloc, int pv_cap);
    virtual ~SB_Slot_Mgr();
    virtual int  alloc();
    virtual int  alloc_if_cap();
    virtual void check_min();
    virtual void free_slot(int pv_slot);
    virtual int  get_cap();
    int          hi();
    bool         inuse(int pv_slot);
    virtual int  max_slot();
    virtual void print(int pv_print_type);
    virtual void print(FILE *pp_f, int pv_print_type);
    virtual void print(FILE *pp_f, Cb_Type pv_cb, int pv_print_type);
    virtual void resize(int pv_cap);
    int          size();

protected:
    enum {
        SLOT_FREE = -3,
        SLOT_USED = -2,
        SLOT_TAIL = -1
    };

    void print_slot(FILE *pp_f, Cb_Type pv_cb, int pv_slot, int *pp_slot);

    char        ia_slotmgr_name[40];
    int        *ip_slots;
    Alloc_Type  iv_alloc;
    int         iv_cap;
    bool        iv_change;
    int         iv_free;
    int         iv_head;
    int         iv_hi;
    int         iv_max;
    int         iv_size;
    int         iv_tail;
};

class SB_Ts_Slot_Mgr : public SB_Slot_Mgr {
public:
    SB_Ts_Slot_Mgr(const char *pp_name, Alloc_Type pv_alloc, int pv_cap);
    virtual ~SB_Ts_Slot_Mgr() {}
    virtual int  alloc();
    virtual int  alloc_if_cap();
    virtual void check_min();
    virtual void free_slot(int pv_slot);
    virtual void lock();
    virtual int  max_slot();
    virtual void print(int pv_print_type);
    virtual void print(FILE *pp_f, int pv_print_type);
    virtual void print(FILE *pp_f, Cb_Type pv_cb, int pv_print_type);
    virtual void resize(int pv_cap);
    virtual void unlock();

private:
    static void      dtor(void *);

    static int       cv_tls_inx;
    SB_Thread::MSL   iv_lock;         // for protection
};

#ifdef USE_SB_INLINE
#include "slotmgr.inl"
#endif

#endif // !__SB_SLOTMGR_H_
