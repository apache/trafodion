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
// Object-Trace module
//
#ifndef __SB_OTRACE_H_
#define __SB_OTRACE_H_

#include <stdio.h> // FILE

#include "int/diag.h"
#include "int/exp.h"

class SB_Trace {
public:
    SB_Trace();
    virtual ~SB_Trace();
    virtual void  trace_close();
    virtual void  trace_flush();
    FILE         *trace_get_fd() SB_DIAG_UNUSED;
    virtual void  trace_init(char *filename,
                             bool  unique,
                             char *prefix,
                             bool  flush); // flush ignored
    virtual void  trace_init2(char      *filename,
                              bool       unique,
                              char      *prefix,
                              bool       flush,  // flush ignored
                              long long  max_size);
    virtual void  trace_init3(char      *filename,
                              bool       unique,
                              char      *prefix,
                              bool       flush,  // flush ignored
                              long long  max_size,
                              bool       sig_hdlr);
    virtual void  trace_lock();
    virtual void  trace_nolock_printf(const char *format, ...)
                                      __attribute__((format(printf, 2, 3)));
    virtual void  trace_nolock_vprintf(void       *ra,
                                       const char *format,
                                       va_list     ap);
    virtual void  trace_nolock_where_printf(const char *where,
                                            const char *format, ...)
                                            __attribute__((format(printf, 3, 4)));
    virtual void  trace_nolock_where_vprintf(void        *ra,
                                             const char  *where,
                                             const char  *format,
                                             va_list      ap);
    virtual void  trace_print_data(void *buf,
                                   int   count,
                                   int   max_count);
    virtual void  trace_printf(const char *format, ...)
                               __attribute__((format(printf, 2, 3)));
    virtual void  trace_set_assert_no_trace(bool assert_no_trace);
    virtual void  trace_set_delta(bool delta);
    virtual void  trace_set_inmem(int size);
    virtual void  trace_set_lock(bool lock);
    virtual void  trace_set_mem(int size);
    virtual void  trace_set_pname(const char *pname);
    virtual void  trace_unlock();
    virtual void  trace_vprintf(void       *ra,
                                const char *format,
                                va_list     ap);
    virtual void  trace_where_printf(const char *where,
                                     const char *format, ...)
                                     __attribute__((format(printf, 3, 4)));
    virtual void  trace_where_vprintf(void        *ra,
                                      const char  *where,
                                      const char  *format,
                                      va_list      ap);

private:
    void              trace_clear_line(char *line);
    void              trace_print_format(const char *format, va_list ap);
    void              trace_print_fprintf(const char *format, ...)
                                          __attribute__((format(printf, 2, 3)));
    void              trace_print_ra(void *ra, const char *where);
    void              trace_print_time_id();
    void              trace_printf_time(char *pa_tl);

    char              ia_trace_pname[20];
    FILE             *ip_trace_file;
    char             *ip_trace_file_buf;
    char             *ip_trace_mem_buf;
    char             *ip_trace_prefix;
    double          (*ip_trace_ts)();
    bool              iv_trace_assert_no_trace;
    bool              iv_trace_delta;
    long long         iv_trace_max_size;
    int               iv_trace_mem_inx;
    int               iv_trace_mem_size;
    int               iv_trace_pid;
    int               iv_trace_set_mem_size;
    bool              iv_trace_sig_hdlr;
    bool              iv_trace_lock;
};

#endif // !__SB_OTRACE_H_
