///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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
///////////////////////////////////////////////////////////////////////////////

#ifndef CLIO_H
#define CLIO_H

#include <list>
using namespace std;

#include <mpi.h>
#include <signal.h>
#include <stdarg.h>

#include "localio.h"


extern int  gv_ms_su_nid;
extern int  gv_ms_su_pid;
extern SB_Verif_Type  gv_ms_su_verif;
extern char ga_ms_su_c_port[MPI_MAX_PORT_NAME];

typedef void (*CallBack)(struct message_def *, int);

typedef enum {
    SET_CB_RET_OK = 0,
    SET_CB_RET_CB_SET,
    SET_CB_RET_MSG_SET,
    SET_CB_RET_COMM_DN,
    SET_CB_RET_MSG_UNKNOWN
} SET_CB_RET_TYPE;

typedef enum {
    CALL_SEL_RET_OK = 0,
    CALL_SEL_RET_NO_UNSOL_CB,
    CALL_SEL_RET_NO_SOL_CB,
    CALL_SEL_RET_NO_BOTH_CB,
    CALL_SEL_RET_SEL_ERROR
} CALL_SEL_RET_TYPE;

extern void block_lio_signals();

class Local_IO_To_Monitor {
friend void *local_monitor_reader(void *pp_arg);
public:
    Local_IO_To_Monitor(int pv_nid);
    ~Local_IO_To_Monitor();

    int                 acquire_msg(struct message_def **pp_msg);
    void                disableWDT()
                         { if ( iv_initted ) ((SharedMemHdr *) ip_cshm)->wdtEnabler = -17958194; }
    int                 get_notice(struct message_def **pp_msg, bool wait = false);
    inline int          get_monitor_pid(void) {return(iv_mpid); }
    bool                init_comm();
    const char *        mon_port() {return ip_mon_mpi_port.c_str();};
    int                 signal_cv( int err=0 );
    const char *        mon_port_fname() {return ip_port_fname;};
    int                 mon_port_num();
    int                 release_msg(struct message_def *pp_msg, 
                                    bool pv_lock = true);
    void                scan_liobufs ( void );
    // send a request to monitor with no expectation of a reply
    int                 send(struct message_def *pp_msg);
    // send a request to monitor with the expectation of receiving
    // an imediate reply, return -1 if msg != shm sr_msg
    int                 send_recv(struct message_def *pp_msg, bool pv_nw=false);
    int                 set_cb(CallBack pp_cb, const char *pp_type);
    int                 size_of_msg(struct message_def *pp_msg, 
                                    bool reply = false);
    void                shutdown( void );
    int                 wait_for_event(struct message_def **pp_msg);
    bool                wait_for_monitor( int &status );

    inline int          getSharedBufferCount() {return(iv_client_buffers_max); }
    int                 getCurrentBufferCount();

    inline int          getLastMonRefresh() { return ((SharedMemHdr*)ip_cshm)->lastMonRefresh; }

private:
    int                 acquire_lock( bool pv_show = true );
    int                 acquire_ev_lock( bool pv_show = true );
    bool                init_local_IO();
    int                 get_io(int sig, siginfo_t *siginfo);
    const char         *get_type_str(int pv_type);
    bool                is_monitor_up();
    void                log_error( int event, int severity, char * buf );
    int                 process_notice(struct message_def *pp_msg);
    int                 put_on_event_list(struct message_def *pp_msg, 
                                          int pv_size);
    int                 put_on_notice_list(struct message_def *pp_msg, 
                                           int pv_size);
    int                 release_lock( bool pv_show = true );
    int                 release_ev_lock( bool pv_show = true );
    int                 send_ctl_msg(MonitorCtlType pv_type, 
                                     int pv_index);
    inline void         set_worker_thread_id(pthread_t thread_id) {
        iv_worker_thread_id = thread_id;
    }
    int                 signal_event_cv();
    void                trace_print_msg(const char *pp_where, 
                                        SharedMsgDef *pp_msg);
    static void         trace_where_printf(const char *pp_where,
                                           const char *pp_format, ...)
                        __attribute__((format(printf, 2, 3)));
    int                 wait_on_event_cv();
    int                 wait_on_cv();

public:

    // set to enable tracing
    static bool                cv_trace;
    // set to tracing function
    static void              (*cp_trace_cb)(const char *pp_where, 
                                            const char *pp_format, 
                                            va_list pv_ap);
    bool                       iv_initted;
    bool                       iv_monitor_down;
    bool                       iv_shutdown;
    int                        iv_pid;
    Verifier_t                 iv_verifier;

private:
    // callback routines here
    CallBack ip_notice_cb;
    CallBack ip_event_cb;
    CallBack ip_recv_cb;
    CallBack ip_unsol_cb;

private:
    char                      *ip_cshm;
    char                      *ip_cshm_end;
    pthread_t                  iv_worker_thread_id;
    bool                       iv_ev_signaled; 
    pthread_cond_t             iv_ev_cv;
    pthread_mutex_t            iv_ev_lock;
    list<struct message_def*>  iv_event_list;
    bool                       iv_sr_signaled; 
    bool                       iv_sr_interrupted;
    pthread_cond_t             iv_sr_cv;
    pthread_mutex_t            iv_sr_lock;
    int                        iv_cmid;
    int                        iv_client_buffers_max;
    int                        iv_acquired_buffer_count;
    int                        iv_acquired_buffer_count_max;
    int                        iv_mpid;
    int                        iv_nid;
    int                        iv_nid_base;
    int                        iv_nodes;
    list<struct message_def*>  iv_notice_list;
    int                        iv_qid;
    long                       iv_port_file_tries;
    pthread_t                  iv_recv_tid;
    bool                       iv_virtual_nodes; 
    bool                       iv_unknown_process;
    bool                       iv_stats;
    char                       ip_port_fname[MAX_PROCESS_PATH];
    string                     ip_mon_mpi_port;

    static const char *        msgTypes_[];
    static const char *        reqTypes_[];
    static const char *        replyTypes_[];
};

extern Local_IO_To_Monitor *gp_local_mon_io;

#endif
