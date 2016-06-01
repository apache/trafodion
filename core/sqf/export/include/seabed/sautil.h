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
// standalone-utilities module
//
//

#ifndef __SB_SAUTIL_H_
#define __SB_SAUTIL_H_

#include <map>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <linux/unistd.h> // gettid

#include "int/exp.h"
#include "int/opts.h"

#define sb_sa_util_gettid() static_cast<pid_t>(syscall(__NR_gettid))

// assert
extern void SB_SA_Util_abort_fun(const char *msg,
                                 const char *file,
                                 unsigned    line,
                                 const char *fun);
extern void SB_SA_Util_assert_fun_cpeq(const char *exp,
                                       const void *lhs,
                                       const void *rhs,
                                       const char *file,
                                       unsigned    line,
                                       const char *fun);
extern void SB_SA_Util_assert_fun_cpne(const char *exp,
                                       const void *lhs,
                                       const void *rhs,
                                       const char *file,
                                       unsigned    line,
                                       const char *fun);
extern void SB_SA_Util_assert_fun_ieq(const char *exp,
                                      int         lhs,
                                      int         rhs,
                                      const char *file,
                                      unsigned    line,
                                      const char *fun);
extern void SB_SA_Util_assert_fun_if(const char *exp,
                                     int         iexp,
                                     const char *file,
                                     unsigned    line,
                                     const char *fun);
extern void SB_SA_Util_assert_fun_ige(const char *exp,
                                      int         lhs,
                                      int         rhs,
                                      const char *file,
                                      unsigned    line,
                                      const char *fun);
extern void SB_SA_Util_assert_fun_igt(const char *exp,
                                      int         lhs,
                                      int         rhs,
                                      const char *file,
                                      unsigned    line,
                                      const char *fun);
extern void SB_SA_Util_assert_fun_ile(const char *exp,
                                      int         lhs,
                                      int         rhs,
                                      const char *file,
                                      unsigned    line,
                                      const char *fun);
extern void SB_SA_Util_assert_fun_ilt(const char *exp,
                                      int         lhs,
                                      int         rhs,
                                      const char *file,
                                      unsigned    line,
                                      const char *fun);
extern void SB_SA_Util_assert_fun_ine(const char *exp,
                                      int         lhs,
                                      int         rhs,
                                      const char *file,
                                      unsigned    line,
                                      const char *fun);
extern void SB_SA_Util_assert_fun_it(const char *exp,
                                     int         iexp,
                                     const char *file,
                                     unsigned    line,
                                     const char *fun);

#define SB_SA_util_abort(msg) SB_SA_Util_abort_fun(msg, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define SB_SA_util_assert_cpeq(lhs,rhs) (void)((lhs == rhs)||(SB_SA_Util_assert_fun_cpeq(#lhs " == " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#define SB_SA_util_assert_cpne(lhs,rhs) (void)((lhs != rhs)||(SB_SA_Util_assert_fun_cpne(#lhs " != " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#define SB_SA_util_assert_ieq(lhs,rhs) (void)((lhs == rhs)||(SB_SA_Util_assert_fun_ieq(#lhs " == " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#define SB_SA_util_assert_if(iexp) (void)((!iexp)||(SB_SA_Util_assert_fun_if("!" #iexp, iexp, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#define SB_SA_util_assert_igt(lhs,rhs) (void)((lhs > rhs)||(SB_SA_Util_assert_fun_igt(#lhs " > " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#define SB_SA_util_assert_ige(lhs,rhs) (void)((lhs >= rhs)||(SB_SA_Util_assert_fun_ige(#lhs " >= " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#define SB_SA_util_assert_ilt(lhs,rhs) (void)((lhs < rhs)||(SB_SA_Util_assert_fun_ilt(#lhs " < " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#define SB_SA_util_assert_ile(lhs,rhs) (void)((lhs <= rhs)||(SB_SA_Util_assert_fun_ile(#lhs " <= " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#define SB_SA_util_assert_ine(lhs,rhs) (void)((lhs != rhs)||(SB_SA_Util_assert_fun_ine(#lhs " != " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#define SB_SA_util_assert_it(iexp) (void)((iexp)||(SB_SA_Util_assert_fun_it(#iexp, iexp, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))

//
// Util debug
//
class SB_Export SB_SA_Util_Debug {
public:
    // Call this if debug enabled to print out some debug statement
    static void debug_printf(const char *where,
                             const char *format,
                             ...) __attribute__((format(printf, 2, 3)));
    static void debug_vprintf(const char *where,
                              const char *format,
                              va_list     ap);
};

//
// Util error
//
class SB_Export SB_SA_Util_Error {
public:
    // Call this to print an eror
    static void error_printf(const char *where,
                             const char *format,
                             ...) __attribute__((format(printf, 2, 3)));
};

//
// Util file
//
class SB_Export SB_SA_Util_File {
public:
    SB_SA_Util_File(const char *file);
    ~SB_SA_Util_File();

    char *get_file_line(char *line, int line_len);

private:
    FILE *ip_file;
    int   iv_line_num;
};

template <class T>
class SB_SA_Util_List {
public:
    SB_SA_Util_List() {
        init();
    }

    void add(T *item) {
        ipp_list[iv_count] = item;
        iv_count++;
    }

    void cap_inc(int inc) {
        T   **lpp_list;
        int   lv_cap;
        int   lv_inx;

        if ((iv_count + 1) > iv_cap) {
            lv_cap = iv_cap;
            iv_cap += inc;
            lpp_list = new T *[iv_cap];
            for (lv_inx = 0; lv_inx < lv_cap; lv_inx++)
                lpp_list[lv_inx] = ipp_list[lv_inx];
            delete [] ipp_list;
            ipp_list = lpp_list;
        }
    }

    void init() {
        ipp_list = NULL;
        iv_cap = 0;
        iv_count = 0;
    }

    T  **ipp_list;
    int  iv_cap;
    int  iv_count;

};

template<class K, class T, class C>
class SB_SA_Util_Map {
public:
    SB_SA_Util_Map() {
    }

    ~SB_SA_Util_Map() {
    }

    T *get(K key) {
        T    *lp_value;
        Iter  lv_iter;

        lv_iter = iv_map.find(key);
        if (lv_iter == iv_map.end())
            return NULL;
        lp_value = lv_iter->second;
        return lp_value;
    }

    void put(K key, T *value) {
        iv_map.insert(std::make_pair(key, value));
    }

    void remove(K key) {
        iv_map.erase(key);
    }

private:
    typedef typename
      std::map<const char *, T *, C>::const_iterator Iter;
    typedef typename
      std::map<const char *, T *, C>                 Map;
    Map iv_map;
};

class SB_SA_Util_Map_Int_Cmp {
public:
    bool operator()(int i1, int i2) {
        bool lv_ret = i1 < i2;
        return lv_ret;
    }
};

class SB_SA_Util_Map_Str_Cmp {
public:
    bool operator()(const char *str1, const char *str2) {
        bool lv_ret = strcmp(str1, str2) < 0;
        return lv_ret;
    }
};

template<class T>
class SB_SA_Util_Map_Str {
public:
    SB_SA_Util_Map_Str() {
    }

    ~SB_SA_Util_Map_Str() {
    }

    T *get(const char *key) {
        T    *lp_value;
        Iter  lv_iter;

        lv_iter = iv_map.find(key);
        if (lv_iter == iv_map.end())
            return NULL;
        lp_value = lv_iter->second;
        return lp_value;
    }

    void put(const char *key, T *value) {
        iv_map.insert(std::make_pair(key, value));
    }

    void remove(const char *key) {
        iv_map.erase(key);
    }

private:
    typedef typename
      std::map<const char *, T *, SB_SA_Util_Map_Str_Cmp>::const_iterator Iter;
    typedef typename
      std::map<const char *, T *, SB_SA_Util_Map_Str_Cmp>                 Map;
    Map iv_map;
};


//
// Util op-line
//
class SB_Export SB_SA_Util_Op_Line {
public:
    SB_SA_Util_Op_Line(SB_SA_Util_File *file);
    ~SB_SA_Util_Op_Line();

    static char *deblank(int skip, char *line);
    int          get_op_line(char *arg[], int max);

private:
    SB_SA_Util_File *ip_file;
};

//
// Util mutex
//
class SB_Export SB_SA_Util_Mutex {
public:
    SB_SA_Util_Mutex();
    ~SB_SA_Util_Mutex();

    void lock();
    void unlock();

protected:
    pthread_mutex_t iv_mutex;
};

class SB_Export SB_SA_Util_Slot_Mgr {
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
    SB_SA_Util_Slot_Mgr(const char *pp_name, Alloc_Type pv_alloc, int pv_cap);
    virtual ~SB_SA_Util_Slot_Mgr();
    virtual int  alloc();
    virtual int  alloc_if_cap();
    virtual void check_min();
    virtual void free_slot(int pv_slot);
    virtual int  get_cap();
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

    char        ia_slotmgr_name[100];
    int        *ip_slots;
    Alloc_Type  iv_alloc;
    int         iv_cap;
    bool        iv_change;
    int         iv_free;
    int         iv_head;
    int         iv_max;
    int         iv_size;
    int         iv_tail;
};

//
// Util time
//
class SB_Export SB_SA_Util_Time {
public:
    static char *format_time(char *tl);
};

//
// Util CV
//
class SB_Export SB_SA_Util_CV : public SB_SA_Util_Mutex {
public:
    SB_SA_Util_CV();
    ~SB_SA_Util_CV();

    void broadcast(bool lock);
    void reset_flag();
    void signal(bool lock);
    void wait(bool lock);

private:
    pthread_cond_t iv_cv;
    bool           iv_flag;
};

//
// Util sem
//
class SB_Export SB_SA_Util_Sem {
public:
    SB_SA_Util_Sem();
    ~SB_SA_Util_Sem();

    void init(bool pshared, unsigned int value);
    void post();
    void wait();

private:
   bool  iv_inited;
   sem_t iv_sem;
};

#endif //!__SB_SAUTIL_H_
