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
// Thread module
//
//
// Encapsulate threading primitives inside namespace SB_Thread
//   which includes:
//     Static Threading class - thread creation, scheduling, etc
//     Thread class - a thread
//     Mutex class - Mutex
//     Errorcheck_Mutex class - Mutex (error check)
//     Preemptive_Mutex class - Mutex that does nothing in non-preemptive thr
//     Scoped_Mutex class - Mutex that unlocks when it goes out of scope
//     CV (Condition Variable) class - Condition variable
//     SL (Spin Lock) class - Spin lock
//
#ifndef __SB_THREAD_H_
#define __SB_THREAD_H_

#define SB_THREAD_LINUX // allow optimization for linux
#ifdef SB_THREAD_LINUX
#include <pthread.h>
#include <semaphore.h>
#endif

#include "int/assert.h"
#include "int/diag.h"
#include "int/exp.h"
#include "int/opts.h"
#include "int/time.h"
#include "int/types.h"

//#define SB_THREAD_PRINT_PTHREAD_CALLS
//#define SB_THREAD_LOCK_STATS

#ifdef SB_THREAD_LOCK_STATS
#include "int/threadstats.h"
#endif

//
// resume suspended threads
//
SB_Export int thread_resume_suspended()
SB_DIAG_UNUSED;

//
// suspend all threads except self
//
SB_Export int thread_suspend_all()
SB_DIAG_UNUSED;

namespace SB_Thread {
    typedef int *Thr_Gen_Ptr;
    typedef struct Time_Type {
        long sec;
        int  ms;
    } Time_Type;
    typedef struct Utime_Type {
        long sec;
        int  us;
    } Utime_Type;

    //
    // Static threading class
    //
    class SB_Export Sthr {
    public:
        typedef void *(*Function)(void *);
        typedef void *Id_Ptr;
        typedef void (*Dtor_Function)(void *);

        // create a thread,
        // which executes fun with argument arg.
        static Id_Ptr         create(char     *name,
                                     Function  fun,
                                     void     *arg) SB_DIAG_UNUSED;

        static void           delete_id(Id_Ptr tid);
        static void           exit(void *value);
        static void           nsleep(SB_Int64_Type ns);
        static long           self_id() SB_DIAG_UNUSED;
        static char          *self_name() SB_DIAG_UNUSED;
        static void           sleep(int ms);
        static void           set_name(char *name);
        static SB_Int64_Type  time() SB_DIAG_UNUSED;
        static void           time(Time_Type *time);
        static SB_Int64_Type  time_us() SB_DIAG_UNUSED;
        static void           usleep(int us);
        static SB_Int64_Type  utime() SB_DIAG_UNUSED;
        static void           utime(Utime_Type *time);
        static void           yield();

        static int            cancel(Id_Ptr tid) SB_DIAG_UNUSED;
        static int            detach(Id_Ptr tid) SB_DIAG_UNUSED;
        static int            join(Id_Ptr tid, void **result) SB_DIAG_UNUSED;
        static int            kill(Id_Ptr tid, int sig) SB_DIAG_UNUSED;

        // thread-specific functions. These allow the efficient association
        // of data separately for each thread.
        static void          *specific_get(int key) SB_DIAG_UNUSED;
        static int            specific_key_create(int &new_key,
                                                  Dtor_Function) SB_DIAG_UNUSED;
        static int            specific_key_create2(Dtor_Function) SB_DIAG_UNUSED;
        static int            specific_set(int key, const void *data) SB_DIAG_UNUSED;
    };

    //
    // A thread.
    // It would be nice to have a protected Function, but I couldn't get
    // this to work.
    //
    class SB_Export Thread {
    public:
        typedef Sthr::Function Function;
        Thread(Function fun, const char *name);
        virtual ~Thread();

        void          allow_suspend();
        void          delete_exit(bool del_exit);
        void         *disp(void *arg) SB_DIAG_UNUSED;
        void          exit(void *value);
        char         *get_name() SB_DIAG_UNUSED;
        int           join(void **result) SB_DIAG_UNUSED;
        int           kill(int sig) SB_DIAG_UNUSED;
        void          resume();
        void          set_daemon(bool daemon);
        void          sleep(int ms);
        void          start();
        void          stop();
        void          suspend();
        void          yield();

        // NOT for general public use
        Thr_Gen_Ptr    ip_rs;
        Thr_Gen_Ptr    ip_tid;

    private:
        static int     cv_count;
        Sthr::Id_Ptr   ip_id;
        char          *ip_name;
        bool           iv_daemon;
        bool           iv_del_exit;
        Function       iv_fun;
    };

    //
    // A Mutex
    //
    class SB_Export Mutex {
    public:
        Mutex();
        Mutex(const char *name);
        Mutex(bool recursive);
        Mutex(bool recursive, bool errorcheck);
        virtual ~Mutex();

        virtual int  destroy() SB_DIAG_UNUSED;
        virtual int  lock() SB_DIAG_UNUSED;
        void         setname(const char *name);
        virtual int  trylock() SB_DIAG_UNUSED;
        virtual int  unlock() SB_DIAG_UNUSED;

    protected:
        void init(const char *name);

#ifdef SB_THREAD_LOCK_STATS
        void               *ia_lock_bt[3];
        int                 iv_last_locker;
        SB_Thread_Lock_Time iv_lock_start;
#endif
#ifdef SB_THREAD_LINUX
        bool                iv_destroyed;
        pthread_mutex_t     iv_mutex;
#else
        //
        // thread package specific mutex, generalized.
        //
        Thr_Gen_Ptr         ip_mutex;
#endif
        const char         *ip_mutex_name;
        bool                iv_errorcheck;
        bool                iv_recursive;
    };

    //
    // A Preemptive Mutex
    //
    class SB_Export Preemptive_Mutex : public Mutex {
    public:
        Preemptive_Mutex();
        virtual ~Preemptive_Mutex();

        virtual int lock() SB_DIAG_UNUSED;
        virtual int trylock() SB_DIAG_UNUSED;
        virtual int unlock() SB_DIAG_UNUSED;
    };

    //
    // A Scoped Mutex
    //
    class SB_Export Scoped_Mutex {
    public:
        Scoped_Mutex(Mutex &mutex);
        ~Scoped_Mutex();

        void lock();
        void unlock();

    private:
        Mutex &ir_mutex;
        bool   iv_lock;
        Scoped_Mutex();
        Scoped_Mutex(const Scoped_Mutex &);
        Scoped_Mutex &operator=(const Scoped_Mutex &);
    };

    //
    // A Errorcheck Mutex
    //
    class SB_Export Errorcheck_Mutex : public Mutex {
    public:
        Errorcheck_Mutex();
        Errorcheck_Mutex(bool destructor_unlock);
        ~Errorcheck_Mutex();

        virtual int  lock() SB_DIAG_UNUSED;
        bool         locked() SB_DIAG_UNUSED;
        virtual int  trylock() SB_DIAG_UNUSED;
        virtual int  unlock() SB_DIAG_UNUSED;

    private:
        bool iv_destructor_unlock;
        bool iv_locked;
    };
#ifdef USE_SB_ECM
    typedef Errorcheck_Mutex ECM;
#else
    typedef Mutex            ECM;
#endif

    //
    // A Condition Variable
    //
    class SB_Export CV : public ECM {
    public:
        CV();
        CV(const char *name);
        ~CV();

        virtual int  destroy() SB_DIAG_UNUSED;
        virtual int  lock() SB_DIAG_UNUSED;
        virtual int  trylock() SB_DIAG_UNUSED;
        virtual int  unlock() SB_DIAG_UNUSED;

        int  broadcast() SB_DIAG_UNUSED;                // like signal but for multiple threads
        int  broadcast(bool lock) SB_DIAG_UNUSED;       // broadcast (lock CV?)
        void reset_flag();                              // call to reset flag (used if signal b4 wait)
        int  signal() SB_DIAG_UNUSED;                   // signal
        int  signal(bool lock);                         // signal (lock CV?)
        bool signaled() SB_DIAG_UNUSED;                 // signaled?
        int  wait() SB_DIAG_UNUSED;                     // wait
        int  wait(bool lock) SB_DIAG_UNUSED;            // wait (lock CV?)
        int  wait(int sec, int usec) SB_DIAG_UNUSED;    // wait
        int  wait(bool lock,                            // wait (lock CV?)
                  int sec, int usec) SB_DIAG_UNUSED;

    protected:
        void init(const char *pp_name);

#ifdef SB_THREAD_LINUX
        pthread_cond_t iv_cv;
        bool           iv_destroyed;
#else
        //
        // thread package specific condition variable, generalized.
        //
        Thr_Gen_Ptr    ip_cv;
#endif
        bool           iv_flag;
    };

    //
    // A RW lock
    //
    class SB_Export RWL {
    public:
        RWL();
        virtual ~RWL();

        virtual int  readlock() SB_DIAG_UNUSED;
        void         setname(const char *name);
        virtual int  tryreadlock() SB_DIAG_UNUSED;
        virtual int  trywritelock() SB_DIAG_UNUSED;
        virtual int  unlock() SB_DIAG_UNUSED;
        virtual int  writelock() SB_DIAG_UNUSED;

    protected:
#ifdef SB_THREAD_LINUX
        pthread_rwlock_t    iv_rwl;
#else
        //
        // thread package specific rwl, generalized.
        //
        Thr_Gen_Ptr         ip_rwl;
#endif
#ifdef SB_THREAD_LOCK_STATS
        int                 iv_last_read_locker;
        int                 iv_last_write_locker;
        SB_Thread_Lock_Time iv_lock_read_start;
        SB_Thread_Lock_Time iv_lock_write_start;
#endif
        const char         *ip_rwl_name;
    };

    //
    // A Spin lock
    //
    class SB_Export SL {
    public:
        SL(bool pshared = false);
        SL(const char *name, bool pshared = false);
        virtual ~SL();

        virtual int  lock() SB_DIAG_UNUSED;
        void         setname(const char *name);
        virtual int  trylock() SB_DIAG_UNUSED;
        virtual int  unlock() SB_DIAG_UNUSED;

    protected:
        void init(const char *name, bool pshared);

#ifdef SB_THREAD_LINUX
        pthread_spinlock_t iv_sl;
#else
        //
        // thread package specific spin-lock, generalized.
        //
        Thr_Gen_Ptr        ip_sl;
#endif
        const char        *ip_sl_name;
#ifdef SB_THREAD_LOCK_STATS
        int                 iv_last_locker;
        SB_Thread_Lock_Time iv_lock_start;
#endif
    };

    //
    // Mutex/SL
    //
#ifdef USE_SB_NO_SL
    typedef Mutex MSL;
#else
    typedef SL    MSL;
#endif

    //
    // Semaphore prototype
    //
    class SB_Export Sem {
    public:
        Sem();
        virtual ~Sem();

        virtual int post() = 0;                        // post
        virtual int trywait() = 0;                     // trywait
        virtual int value(int *val) = 0;               // value
        virtual int wait() = 0;                        // wait
        virtual int wait_timed(int sec, int usec) = 0; // wait-timed
    };

    //
    // Unnamed semaphore
    //
    class SB_Export Usem : public Sem {
    public:
        Usem();
        virtual ~Usem();

        virtual int init(bool pshared, unsigned int value) SB_DIAG_UNUSED;
        virtual int post() SB_DIAG_UNUSED;             // post
        virtual int trywait() SB_DIAG_UNUSED;          // trywait
        virtual int value(int *val) SB_DIAG_UNUSED;    // value
        virtual int wait() SB_DIAG_UNUSED;             // wait
        virtual int wait_timed(int sec, int usec)      // wait-timed
        SB_DIAG_UNUSED;

    protected:
#ifdef SB_THREAD_LINUX
        bool        iv_inited;
        sem_t       iv_sem;
#else
        //
        // thread package specific sem
        //
        Thr_Gen_Ptr ip_sem;
#endif
    };

    //
    // Named semaphore
    //
    class SB_Export Nsem : public Sem {
    public:
        Nsem();
        virtual ~Nsem();

        virtual int init(const char   *name,
                         int           oflag,
                         unsigned int  mode,
                         unsigned int  value) SB_DIAG_UNUSED;
        virtual int post() SB_DIAG_UNUSED;                      // post
        virtual int remove(const char *name) SB_DIAG_UNUSED;    // remove
        virtual int trywait() SB_DIAG_UNUSED;                   // trywait
        virtual int value(int *val) SB_DIAG_UNUSED;             // value
        virtual int wait() SB_DIAG_UNUSED;                      // wait
        virtual int wait_timed(int sec, int usec)               // wait-timed
        SB_DIAG_UNUSED;

    protected:
#ifdef SB_THREAD_LINUX
        sem_t       *ip_sem;
#else
        //
        // thread package specific sem
        //
        Thr_Gen_Ptr  ip_sem;
#endif
    };

    //
    // Robust unnamed semaphore
    //
    class SB_Export RUsem : public Sem {
    public:
        RUsem();
        virtual ~RUsem();

        virtual int destroy();
        virtual int init(unsigned int appid, unsigned int value) SB_DIAG_UNUSED;
        virtual int post() SB_DIAG_UNUSED;             // post
        virtual int trywait() SB_DIAG_UNUSED;          // trywait
        virtual int value(int *val) SB_DIAG_UNUSED;    // value
        virtual int wait() SB_DIAG_UNUSED;             // wait
        virtual int wait_timed(int sec, int usec)      // wait-timed
        SB_DIAG_UNUSED;

    private:
        int check();
        void *ip_sem;     // back-end semaphore
    };

}

#ifdef USE_SB_INLINE
#include "int/thread.inl"
#endif

#endif //!__SB_THREAD_H_
