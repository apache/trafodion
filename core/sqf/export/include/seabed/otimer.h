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
// Object-Timer module
//
#ifndef __SB_OTIMER_H_
#define __SB_OTIMER_H_

#include "int/diag.h"
#include "int/exp.h"

#include "thread.h"

namespace SB_Timer {
    typedef SB_Int64_Type Tics;

    class Timer;

    //
    // time stamp (encapsulation)
    //
    class SB_Export Time_Stamp {
    public:
        Time_Stamp();                              // const
        Time_Stamp(Time_Stamp &ts);                // copy const
        ~Time_Stamp();                             // dest

        const char *format_ts(char *buf);          // format ts
        Tics        tic_add(Tics tics)             // add tics, return tics
        SB_DIAG_UNUSED;
        Tics        tic_get()                      // get tics
        SB_DIAG_UNUSED;
        void        tic_set(Tics tics);            // set tics
        void        tic_set_now_add(Tics tics);    // add tics to now
        bool        ts_eq(Time_Stamp &ts)          // is ts == ?
        SB_DIAG_UNUSED;
        bool        ts_ge(Time_Stamp &ts)          // is ts >= ?
        SB_DIAG_UNUSED;
        bool        ts_gt(Time_Stamp &ts)          // is ts > ?
        SB_DIAG_UNUSED;
        bool        ts_le(Time_Stamp &ts)          // is ts <= ?
        SB_DIAG_UNUSED;
        bool        ts_lt(Time_Stamp &ts)          // is ts < ?
        SB_DIAG_UNUSED;
        bool        ts_ne(Time_Stamp &ts)          // is ts != ?
        SB_DIAG_UNUSED;
        void        ts_set(Time_Stamp &ts);        // set ts
        Tics        ts_sub(Time_Stamp &ts)         // sub tics, return tics
        SB_DIAG_UNUSED;

        enum { TICS_PER_SEC = 100   }; // 10 ms granularity
        enum { US_PER_TIC   = 10000 };

    private:
        // Timer can access directly
        friend class Timer;
        SB_Uint64_Type iv_tics;
    };

    class TH;

    //
    // timer
    //
    class SB_Export Timer {
    public:
        enum { DEFAULT_WAIT_TICS = (2 * Time_Stamp::TICS_PER_SEC) };

        static void check_timers();  // check timers (any timeouts?)
        static Tics get_wait_time()  // wait time for 1st timer
        SB_DIAG_UNUSED;
        static void init();          // init

        Timer(TH   *th,              // timer handler
              long  user_param,      // param (anything caller wants)
              Tics  interval,        // interval (in tics)
              bool  start);          // start?
        virtual ~Timer();            // dest

        void        cancel();                        // cancel timer
        const char *format_pop_time(char *buf);      // format pop time
        const char *format_timer(char *buf);         // format pop time
        inline Tics get_interval() SB_DIAG_UNUSED {  // get interval
            return iv_interval;
        }
        inline long get_param() SB_DIAG_UNUSED {     // get user param
            return iv_user_param;
        }
        typedef void (*Print_Timer_Cb)(Timer &timer);
        static void print_timers(Print_Timer_Cb cb); // print timers
        void        set_interval(Tics interval,      // set interval
                                 bool start);        // start?
        void        set_param(long user_param);      // set user param
        void        start();                         // start timer

        static bool cv_trace_enabled;

    private:
        Timer();                    // const
        void cancel_int(bool lock); // cancel timer

        Timer     *ip_next;         // next timer
        TH        *ip_th;           // handler
        Tics       iv_interval;     // timer interval
        Time_Stamp iv_pop_time;     // timer pop time
        bool       iv_running;      // timer running?
        long       iv_user_param;   // user control

        typedef unsigned int Slot_Type;
        enum { MAX_SLOTS     = 8192 };
        enum { TICS_PER_SLOT =  256 };
        enum { DEFAULT_SLOT_COUNT =
                 ((2 * DEFAULT_WAIT_TICS) - 1) / TICS_PER_SLOT };

        static Slot_Type hash(Time_Stamp &slot_time);    // hash time-stamp

        static Timer            *ca_slots[MAX_SLOTS];    // timer slots
        static Slot_Type         cv_last_slot_checked;   // last slot checked
        static SB_Thread::Mutex  cv_mutex;               // mutex
    };

    //
    // abstract timer handler
    //
    class SB_Export TH {
    public:
        virtual void handle_timeout(Timer *timer) = 0;
    };
}

#endif // !__SB_OTIMER_H_
