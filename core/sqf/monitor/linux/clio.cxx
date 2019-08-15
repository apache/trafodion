///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <linux/unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>

#include "clio.h"
#include "localio.h"
#include "common/evl_sqlog_eventnum.h"
#include "sqevlog/evl_sqlog_writer.h"

#ifdef NDEBUG
#  ifdef USE_ASSERT_ABORT
#    define LIOTM_assert(exp) (void)((exp)||abort())
#  else
#    define LIOTM_assert(exp) ((void)0)
#  endif
#else
#  define LIOTM_assert(exp) (void)((exp)||(LIOTM_assert_fun(#exp, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#endif  // NDEBUG

#define gettid() syscall(__NR_gettid)

#define ENDL                endl; fflush(stdout)
#define SUCCESS             0
#define FAILURE            -1

bool Shutdown = false;
bool Local_IO_To_Monitor::cv_trace = false;
void (*Local_IO_To_Monitor::cp_trace_cb)(const char *, const char *, va_list) = NULL;

enum {              ALTSIG_NODES = 1000 };
typedef struct altsig_node {
    struct altsig_node *ip_next;
    int                 iv_sig;
    siginfo_t           iv_siginfo;
} Altsig_Node;
Altsig_Node         *gp_altsig_free_head  = NULL;
Altsig_Node         *gp_altsig_free_tail  = NULL;
Altsig_Node         *gp_altsig_queue_head = NULL;
Altsig_Node         *gp_altsig_queue_tail = NULL;
Altsig_Node        **gpp_altsig_nodes     = NULL;
pthread_cond_t       gv_altsig_cond       = PTHREAD_COND_INITIALIZER;
pthread_mutex_t      gv_altsig_mutex      = PTHREAD_MUTEX_INITIALIZER;
pthread_spinlock_t   gv_altsig_spinlock;

// the global local io object.  When this is set, the code in msmon.cpp
// and the shell.cxx follow the localio code path.
Local_IO_To_Monitor *gp_local_mon_io = NULL;

void LIOTM_assert_fun(const char *pp_exp,
                      const char *pp_file,
                      unsigned    pv_line,
                      const char *pp_fun) {
    char la_buf[BUFSIZ];
    char la_cmdline[BUFSIZ];

    strcpy(la_buf, "/proc/self/cmdline");
    FILE *lp_file = fopen(la_buf, "r");
    char *lp_s;
    if (lp_file != NULL) {
        lp_s = fgets(la_cmdline, sizeof(la_cmdline), lp_file);
        fclose(lp_file);
    } else
        lp_s = NULL;
    if (lp_s == NULL)
        lp_s = (char *) "<unknown>"; // cast
    sprintf(la_buf, "%s (%d-%ld): %s:%u %s: Assertion '%s' failed.\n",
            lp_s,
            getpid(), gettid(),
            pp_file, pv_line, pp_fun, pp_exp);
    fprintf(stderr, la_buf);
    fflush(stderr);
    abort();
}

//
// alt signal handler
//
// get a free node, copy siginfo, queue node, signal
//
void altsig_sig(int pv_sig, siginfo_t *pp_siginfo, void *) {
    Altsig_Node *lp_node;
    int          lv_err;
    int          lv_lerr;

    // get lock to queue
    lv_lerr = pthread_spin_lock(&gv_altsig_spinlock);
    LIOTM_assert(lv_lerr == 0);

    // get free node
    lp_node = gp_altsig_free_head;
    LIOTM_assert(lp_node != NULL);
    gp_altsig_free_head = gp_altsig_free_head->ip_next;

    // set node data 
    lp_node->ip_next = NULL; 
    lp_node->iv_sig = pv_sig;
    memcpy(&lp_node->iv_siginfo, pp_siginfo, sizeof(lp_node->iv_siginfo));

    // queue it up
    if (gp_altsig_queue_tail == NULL)
        gp_altsig_queue_head = lp_node;
    else
        gp_altsig_queue_tail->ip_next = lp_node;
    gp_altsig_queue_tail = lp_node;

    // unlock
    lv_lerr = pthread_spin_unlock(&gv_altsig_spinlock);
    LIOTM_assert(lv_lerr == 0);

    // tell reader there's a node
    lv_err = pthread_cond_signal(&gv_altsig_cond);
    LIOTM_assert(lv_err == 0);
}

//
// alt signal init
//
void altsig_init() {
    struct sigaction lv_act;
    int              lv_err;
    int              lv_inx;

    // create free list
    gpp_altsig_nodes = new Altsig_Node *[ALTSIG_NODES];
    for (lv_inx = 0; lv_inx < ALTSIG_NODES; lv_inx++)
        gpp_altsig_nodes[lv_inx] = new Altsig_Node;
    for (lv_inx = 0; lv_inx < ALTSIG_NODES - 1; lv_inx++)
        gpp_altsig_nodes[lv_inx]->ip_next = gpp_altsig_nodes[lv_inx + 1];
    gp_altsig_free_head = gpp_altsig_nodes[0];
    gp_altsig_free_tail = gpp_altsig_nodes[ALTSIG_NODES - 1];

    // init spinlock
    lv_err = pthread_spin_init(&gv_altsig_spinlock, 0);
    if (lv_err)
        abort();

    // setup signal handler
    memset(&lv_act, 0, sizeof(struct sigaction));
    lv_act.sa_sigaction = altsig_sig;
    lv_act.sa_flags = SA_SIGINFO;
    lv_err = sigaction(SQ_LIO_SIGNAL_REQUEST_REPLY, &lv_act, NULL);
    if (lv_err)
        abort();
}

//
// alt sigtimedwait - act like regular sigtimedwait, but use private queue/cv
//
int altsig_sigtimedwait(const sigset_t *, siginfo_t *pp_info, const struct timespec *pp_timeout) {
    Altsig_Node *lp_node;
    int          lv_lerr;
    int          lv_err;
    int          lv_ret;

    // get lock to check for node
    lv_lerr = pthread_spin_lock(&gv_altsig_spinlock);
    LIOTM_assert(lv_lerr == 0);

    lp_node = gp_altsig_queue_head;
    if (lp_node == NULL) {
        // no node, give up lock
        lv_lerr = pthread_spin_unlock(&gv_altsig_spinlock);
        LIOTM_assert(lv_lerr == 0);

        // wait
        lv_err = pthread_cond_timedwait(&gv_altsig_cond, &gv_altsig_mutex, pp_timeout);

        // get lock
        lv_lerr = pthread_spin_lock(&gv_altsig_spinlock);
        LIOTM_assert(lv_lerr == 0);

        // check pthread_cond_timedwait outcome
        switch (lv_err) {
        case 0:
            lp_node = gp_altsig_queue_head;
            break;
        case ETIMEDOUT:
            lp_node = gp_altsig_queue_head; // check it even if it timedout
            break;
        default:
            LIOTM_assert(lv_err == 0); // pthread_cond_timedwait returns 0/ETIMEDOUT/EINVAL/EPERM
            break;
       }
    }

    if (lp_node == NULL) {
        lv_ret = -1;
        errno = EAGAIN;
    } else {
        lv_ret = 0;
        // fix queue-head/tail
        gp_altsig_queue_head = lp_node->ip_next;
        if (gp_altsig_queue_head == NULL)
            gp_altsig_queue_tail = NULL;
        // copy info
        memcpy(pp_info, &lp_node->iv_siginfo, sizeof(lp_node->iv_siginfo));
        // put node on free list
        lp_node->ip_next = NULL;
        if (gp_altsig_free_tail == NULL)
            gp_altsig_free_head = lp_node;
        else
            gp_altsig_free_tail->ip_next = lp_node;
        gp_altsig_free_tail = lp_node;
    }

    // unlock
    lv_lerr = pthread_spin_unlock(&gv_altsig_spinlock);
    LIOTM_assert(lv_lerr == 0);

    return lv_ret;
}

// this is the real time signal processing thread for seabed and the shell
// that receives control messages from the monitor.
void *local_monitor_reader(void *pp_arg) {
    const char       *WHERE = "local_monitor_reader";
    bool              lv_altsig;
    bool              lv_not_done;
    int               lv_ret = SUCCESS;
    sigset_t          lv_sig_set;
    siginfo_t         lv_siginfo;
    int               lv_sig;
    struct timespec   lv_tp;
    pid_t             lv_mpid = (pid_t)(long)pp_arg;

    if (gp_local_mon_io->cv_trace)
        gp_local_mon_io->trace_where_printf(WHERE,
                                            "ENTER, monitor pid = %ld\n",
                                            (long) pp_arg);

    lv_altsig = gp_local_mon_io->iv_altsig;
    // Setup signal handling
    sigemptyset(&lv_sig_set);
    sigaddset(&lv_sig_set, SQ_LIO_SIGNAL_REQUEST_REPLY);
    LIOTM_assert(pthread_sigmask(SIG_BLOCK, &lv_sig_set, NULL) == 0);

    gp_local_mon_io->set_worker_thread_id(pthread_self());
    
    lv_not_done = true;
    while (lv_not_done) {
        lv_tp.tv_sec = 0;
        lv_tp.tv_nsec = SQ_LIO_SIGNAL_TIMEOUT;
        if (lv_altsig)
            lv_sig = altsig_sigtimedwait( &lv_sig_set, &lv_siginfo, &lv_tp );
        else
            lv_sig = sigtimedwait( &lv_sig_set, &lv_siginfo, &lv_tp );
        if (lv_sig == -1) {
            if (gp_local_mon_io->cv_trace && errno != EAGAIN)
                gp_local_mon_io->trace_where_printf(WHERE,
                   "monitor pid = %ld, sigtimedwait errno: %d(%s)\n", 
                   (long) pp_arg, errno, strerror(errno));
            LIOTM_assert(errno == EAGAIN || errno == EINTR);
            // check if monitor is gone
            if (kill(lv_mpid,0) == -1 && errno == ESRCH) {
                if (gp_local_mon_io->cv_trace)
                    gp_local_mon_io->trace_where_printf(WHERE,
                        "monitor pid = %ld, is gone, exiting\n", (long) pp_arg);
                gp_local_mon_io->shutdown();
                break;
            }
        }
        else
        {
            // If time to exit thread, set by Local_IO_To_Monitor destructor
            if ( Shutdown ) 
            {
                break;
            }
            else
            {
                lv_ret = gp_local_mon_io->get_io(lv_sig, &lv_siginfo);
                if (lv_ret == FAILURE ) {
                    
                    break;
                }
            }
        }
    }

    if (gp_local_mon_io && gp_local_mon_io->cv_trace)
        gp_local_mon_io->trace_where_printf(WHERE, "EXIT thread\n");

    pthread_exit((void *) errno); // cast
    return (void *) errno; // cast
}

void block_lio_signals() {

    sigset_t lv_sig_set;
    // Setup signal handling
    sigemptyset(&lv_sig_set);
    sigaddset(&lv_sig_set, SQ_LIO_SIGNAL_REQUEST_REPLY);
    LIOTM_assert(pthread_sigmask(SIG_BLOCK, &lv_sig_set, NULL) == 0);
}

// the shell and seabed localio constructor
Local_IO_To_Monitor::Local_IO_To_Monitor(int pv_pid) 
    :ip_cshm(NULL), ip_cshm_end(NULL), iv_cmid(0), iv_qid(0), iv_port_file_tries ( 30 )
{
    const char *WHERE = "Local_IO_To_Monitor::Local_IO_To_Monitor";
    char     *ptr;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");
    
    iv_worker_thread_id=0;
    iv_initted=false;
    iv_altsig=false;
    iv_pid=pv_pid;
    iv_verifier = gv_ms_su_verif;
    ip_notice_cb=NULL;
    ip_event_cb=NULL;
    ip_recv_cb=NULL;
    ip_unsol_cb=NULL;
    iv_monitor_down=true;
    iv_mpid=0;
    iv_client_buffers_max=SQ_LIO_MAX_BUFFERS;

    char la_node_name[MPI_MAX_PROCESSOR_NAME];
    char la_short_node_name[MPI_MAX_PROCESSOR_NAME];
    memset( la_short_node_name, 0, MPI_MAX_PROCESSOR_NAME );
    if (gethostname(la_node_name, MPI_MAX_PROCESSOR_NAME) == -1)
    {
        if (cv_trace)
            trace_where_printf(WHERE, "gethostname failed , errno=%d (%s)\n",
                               errno, strerror(errno));
        la_node_name[0] = '\0';
    }
    char *tmpptr = la_node_name;
    while ( *tmpptr )
    {
        *tmpptr = (char)tolower( *tmpptr);
        tmpptr++;
    }

    // Remove the domain portion of the name if any
    char str1[MPI_MAX_PROCESSOR_NAME];
    memset( str1, 0, MPI_MAX_PROCESSOR_NAME );
    strcpy (str1, la_node_name );

    char *str1_dot = strchr( (char *) str1, '.' );
    if ( str1_dot )
    {
        memcpy( la_short_node_name, str1, str1_dot - str1 );
    }
    else
    {
        strcpy (la_short_node_name, str1 );
    }

    char *lp_nodes = getenv("SQ_VIRTUAL_NODES");
    if (lp_nodes != NULL) 
    {
        iv_virtual_nodes = true;
        iv_nodes = atoi(lp_nodes);
        if (iv_nodes <= 0)
        {
            iv_nodes = 1;
        }

        int lv_MyNID;

        if (gv_ms_su_nid < 0)
        {   // Node id not set, get node number from environment variable
            char * lp_nid = getenv("SQ_LIO_VIRTUAL_NID");
            if (lp_nid != NULL)
            {
                lv_MyNID = atoi(lp_nid);
                if (cv_trace)
                    trace_where_printf(WHERE, "env var SQ_LIO_VIRTUAL_NID=%s, "
                                       "MyNID=%d\n", lp_nid, lv_MyNID);
            }
            else
            {
                if (cv_trace)
                    trace_where_printf(WHERE, "env var SQ_LIO_VIRTUAL_NID "
                                       "not found\n");
                lv_MyNID = 0;
            }
        }
        else
        {   // Use the globally set node id
            lv_MyNID = gv_ms_su_nid;
        }
        sprintf(ip_port_fname,"%.*s/monitor.port.%d.%s",
                (int)(sizeof(ip_port_fname)-(sizeof("/monitor.port..")+11
                                        +strlen(la_node_name))),
                getenv("TRAF_LOG"),lv_MyNID,la_node_name);
    } else 
    {
        // It's a real cluster
        iv_virtual_nodes = false;
        iv_nodes = 1;

        sprintf(ip_port_fname,"%.*s/monitor.port.%s",
                (int)(sizeof(ip_port_fname)-(sizeof("/monitor.port.")+11
                                        +strlen(la_short_node_name))),
                getenv("TRAF_LOG"), la_short_node_name);

    }
    // Assume nid zero if the global Seabed nid variable is not initialized
    iv_nid = gv_ms_su_nid < 0 ? 0 : gv_ms_su_nid ;
    // On a real cluster, set base the segment id to zero
    iv_nid_base = iv_virtual_nodes ? iv_nid : 0;
    
    iv_recv_tid=0;
    iv_unknown_process=false;
    iv_shutdown=false;
    ptr = getenv( "SQ_LIO_MAX_BUFFERS" );
    if (ptr) {
        int nb = atoi( ptr );
        if (nb > 0)
        {
            iv_client_buffers_max = nb;
        }
    }
    iv_acquired_buffer_count = 0;
    iv_acquired_buffer_count_max = 0;

    // Check for override of default setting of retry count used by 
    // wait_for_monitor()
    ptr = getenv( "SQ_LIO_PORT_FILE_TRIES" );
    if (ptr) {
        iv_port_file_tries = strtol ( ptr, NULL, 10 );
        if ( iv_port_file_tries < 10 )
            iv_port_file_tries = 10;
    }

    ptr = getenv( "SQ_LIO_CLIENT_STATS" );
    if (ptr && *ptr == '1') {
        iv_stats = true;
    }
    else {
        iv_stats = false;
    }
    // Setup thread control
    iv_sr_signaled = false;
    iv_sr_interrupted = false;
    LIOTM_assert(pthread_mutex_init(&iv_sr_lock, NULL) == 0);
    LIOTM_assert(pthread_cond_init(&iv_sr_cv, NULL) == 0);
    iv_ev_signaled = false;
    LIOTM_assert(pthread_mutex_init(&iv_ev_lock, NULL) == 0);
    LIOTM_assert(pthread_cond_init(&iv_ev_cv, NULL) == 0);

    // Setup signal handling
    //   The main thread in client process cannot handle
    //   LIO real time signals
    sigset_t lv_sig_set;
    sigemptyset(&lv_sig_set);
    sigaddset(&lv_sig_set, SQ_LIO_SIGNAL_REQUEST_REPLY);
    LIOTM_assert(pthread_sigmask(SIG_BLOCK, &lv_sig_set, NULL) == 0);

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT\n");
}

// the shell and seabed localio destructor
Local_IO_To_Monitor::~Local_IO_To_Monitor() {
    const char *WHERE = "Local_IO_To_Monitor::~Local_IO_To_Monitor";

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");

    Shutdown = iv_shutdown = true;

    if (iv_worker_thread_id != 0)
    {
        pthread_kill(iv_worker_thread_id, SQ_LIO_SIGNAL_REQUEST_REPLY);
        int rc = pthread_join(iv_worker_thread_id, NULL);
        if (rc)
        {
            if (cv_trace)
                trace_where_printf(WHERE, "EXIT, Error= Failed waiting for thread to exit! - errno = %d (%s)\n", rc, strerror(rc));
        }
    }

    if (iv_initted)
    {
        if (iv_stats)
        {
            if (cv_trace)
            {
                trace_where_printf( WHERE, "EXIT, LIO Stats: shared buffers: inuse=%d, total=%d, acquiredMax=%d\n"
                                  , iv_acquired_buffer_count
                                  , iv_client_buffers_max
                                  , iv_acquired_buffer_count_max
                                  );
            }
        }
        if ( iv_acquired_buffer_count != 0 )
        {
            char la_buf[256];
            sprintf(la_buf, "[%s], %d local io buffers were acquired but "
                    "not released.\n",
                    WHERE, iv_acquired_buffer_count);
            log_error ( MON_CLIO_LEAK_1, SQ_LOG_WARNING, la_buf );
        }

        // detach from shared memory
        shmdt(ip_cshm);
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT\n");
}

int Local_IO_To_Monitor::mon_port_num()
{
    #define BLOCK_SIZE  512
    char MonitorPort[BLOCK_SIZE];
    unsigned long int myPortNum = 0;
    int rc = 0;
    struct stat lv_stat;
    const char *WHERE = "Local_IO_To_Monitor::mon_port_num";

    if ( strlen(ga_ms_su_c_port) != 0 )
    {
        char *lp_port = strstr(ga_ms_su_c_port, "$port#");
        if (lp_port)
        {
            lp_port += 5;
        }
        else
        {
            lp_port = strchr(ga_ms_su_c_port, ':');
        }
        myPortNum = strtoul(&lp_port[1], NULL, 10);

        if (gp_local_mon_io && gp_local_mon_io->cv_trace)
            gp_local_mon_io->trace_where_printf(WHERE, "Return, monitor port=%d (%s)\n", (int)myPortNum, &lp_port[1]);

        return (int)myPortNum;
    }
    
    memset( (void *)MonitorPort, 0 , BLOCK_SIZE );
    memset( (void *)&lv_stat, 0 , sizeof(lv_stat) );
    int fd = open( ip_port_fname, O_RDONLY  );
    if( fd == -1 )
    {
        if (gp_local_mon_io && gp_local_mon_io->cv_trace)
            gp_local_mon_io->trace_where_printf(WHERE, "Cannot open %s, errno=%d, %s\n", ip_port_fname, errno, strerror(errno));
    }
    else
    {
        do
        {
            rc = stat( ip_port_fname, &lv_stat );
            if ( lv_stat.st_size || rc == -1 )
            {
                break;
            }
            // we need to wait for the monitor to write the port number
            // only in the startup case
            usleep(25000); // 25 ms
        }
        while ( lv_stat.st_size == 0 && rc == 0 );
        
        if ( rc == 0 )
        {
            rc = (int) read( fd, MonitorPort, BLOCK_SIZE);
            if ( rc == -1 )
            {
                int err = errno;
                if (gp_local_mon_io && gp_local_mon_io->cv_trace)
                    gp_local_mon_io->trace_where_printf(WHERE, "Cannot read %s, errno=%d, %s\n", ip_port_fname, err, strerror(err));
            }
            else
            {   // Ensure port string is null terminated
                MonitorPort[lv_stat.st_size - 1] = '\0';
                ip_mon_mpi_port = MonitorPort;
            }

            char *lp_port = strstr(MonitorPort, "$port#");
            if (lp_port)
            {
                lp_port += 5;
            }
            else
            {
                lp_port = strchr(MonitorPort, ':');
            }
            myPortNum = strtoul(&lp_port[1], NULL, 10);
        }
        close(fd);
    }

    if (gp_local_mon_io && gp_local_mon_io->cv_trace)
        gp_local_mon_io->trace_where_printf(WHERE, "Return, monitor port=%d\n", (int)myPortNum);

    return (int)myPortNum;
}

// acquire the lock used to synchronize the threads on send/recv/notice
int Local_IO_To_Monitor::acquire_lock( bool pv_show ) {
    const char *WHERE = "Local_IO_To_Monitor::acquire_lock";
    int         lv_ret = SUCCESS;

    if (cv_trace && pv_show)
        trace_where_printf(WHERE, "ENTER\n");

    lv_ret = pthread_mutex_lock(&iv_sr_lock);
    if ( lv_ret != 0) {
        char la_buf[256];
        sprintf(la_buf, "[%s], pthread_mutex_lock returned error=%d (%s)\n",
                WHERE, lv_ret, strerror(lv_ret));
        log_error ( MON_CLIO_ACQUIRE_LOCK_1, SQ_LOG_ERR, la_buf );

        errno = lv_ret;
        lv_ret = FAILURE;
    }
    else {
        errno = 0;
    }

    if (cv_trace && pv_show)
        trace_where_printf(WHERE, "EXIT, ret=%d\n", lv_ret);

    return(lv_ret);
}

// acquire the lock used to synchronize the threads on event
int Local_IO_To_Monitor::acquire_ev_lock( bool pv_show ) {
    const char *WHERE = "Local_IO_To_Monitor::acquire_ev_lock";
    int         lv_ret = SUCCESS;

    if (cv_trace && pv_show)
        trace_where_printf(WHERE, "ENTER\n");

    lv_ret = pthread_mutex_lock(&iv_ev_lock);
    if ( lv_ret != 0) {
        char la_buf[256];
        sprintf(la_buf, "[%s], pthread_mutex_lock returned error=%d (%s)\n",
                WHERE, lv_ret, strerror(lv_ret));
        log_error ( MON_CLIO_ACQUIRE_LOCK_2, SQ_LOG_ERR, la_buf );

        errno = lv_ret;
        lv_ret = FAILURE;
    }
    else {
        errno = 0;
    }

    if (cv_trace && pv_show)
        trace_where_printf(WHERE, "EXIT, ret=%d, errno=%d(%s)\n", lv_ret, errno, strerror(errno));

    return(lv_ret);
}

// get a client buffer from the available pool
int Local_IO_To_Monitor::acquire_msg(struct message_def **pp_msg) {
    const char         *WHERE = "Local_IO_To_Monitor::acquire_msg";
    ClientBufferInfo    lv_cbi;
    SharedMsgDef       *lp_sr_msg = NULL;
    struct message_def *lp_msg = NULL;
    struct msqid_ds     lv_mds;
    int                 lv_ret = SUCCESS;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");

    errno = 0;
    lv_ret = acquire_lock();
    if ( lv_ret == FAILURE) {
        if (cv_trace)
            trace_where_printf(WHERE, "EXIT, ret=%p\n", (void *)lp_msg);
        *pp_msg = NULL;
        return lv_ret;
    }

    if (cv_trace) {
        lv_ret = msgctl(iv_qid, IPC_STAT, &lv_mds);
        if (lv_ret != -1) {
            trace_where_printf(WHERE, 
                               "(qid=%d) shared buffers available=%d, "
                               "acquired=%d, errno=%d\n",
                               iv_qid, (int)lv_mds.msg_qnum,
                               iv_acquired_buffer_count, errno);
        }
    }
    // get a client buffer from the available pool
    lv_ret = (int) msgrcv( iv_qid, &lv_cbi, 
                           sizeof( lv_cbi.index ),
                           SQ_LIO_NORMAL_MSG, IPC_NOWAIT );
    if (lv_ret == -1) {
        lv_ret = errno;
        char la_buf[256];
        sprintf(la_buf, "[%s], msgrcv() failed getting buffer from shared pool, errno=%d (%s)\n", WHERE, errno, strerror(errno));
        log_error ( MON_CLIO_ACQUIRE_MSG_1, SQ_LOG_ERR, la_buf );

        if (cv_trace)
            trace_where_printf(WHERE, "%s", la_buf);

        *pp_msg = NULL;
        LIOTM_assert(release_lock() == SUCCESS);
        return lv_ret;
    }

    LIOTM_assert(lv_ret == sizeof( lv_cbi.index ));

    if (cv_trace)
        trace_where_printf(WHERE, "dequeued shared buffer, idx=%d\n", lv_cbi.index);

    lp_sr_msg = (SharedMsgDef *)(ip_cshm+sizeof(SharedMemHdr)
                                 +(lv_cbi.index*sizeof(SharedMsgDef)));
    memset( (void*)&lp_sr_msg->trailer, 0, sizeof(lp_sr_msg->trailer) );
    lp_sr_msg->trailer.index = lv_cbi.index;
    lp_sr_msg->trailer.OSPid = iv_pid;
    lp_sr_msg->trailer.verifier = iv_verifier;
    lp_sr_msg->trailer.bufInUse = iv_pid;
    clock_gettime(CLOCK_REALTIME, &lp_sr_msg->trailer.timestamp);

    if (cv_trace) {
        ++iv_acquired_buffer_count;
        iv_acquired_buffer_count_max = 
            iv_acquired_buffer_count > iv_acquired_buffer_count_max 
            ? iv_acquired_buffer_count : iv_acquired_buffer_count_max;

        lv_ret = msgctl(iv_qid, IPC_STAT, &lv_mds);
        if (lv_ret != -1) {
            trace_where_printf(WHERE, 
                               "(qid=%d) shared buffers available=%d, "
                               "acquired=%d, errno=%d\n",
                               iv_qid, (int)lv_mds.msg_qnum,
                               iv_acquired_buffer_count, errno);
        }
    }

    if (cv_trace)
        trace_print_msg(WHERE, lp_sr_msg);

    lv_ret = release_lock();
    LIOTM_assert(lv_ret == SUCCESS);
    
    if (iv_unknown_process)
    {
        LIOTM_assert(iv_unknown_process);
    }
    else
        lp_msg = &lp_sr_msg->msg;

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%p\n", (void *) lp_msg);

    *pp_msg = lp_msg;
    return lv_ret;
}


// called from the monitor reader thread when a rt signal is received
// from the monitor
int Local_IO_To_Monitor::get_io(int pv_sig, siginfo_t *pp_siginfo) {
    const char     *WHERE = "Local_IO_To_Monitor::get_io";
    int             lv_ret = SUCCESS;
    int             lv_idx;
    SharedMsgDef   *lv_m;

    // type is the control message from the monitor (low order byte)
    MonitorCtlType  lv_type = (MonitorCtlType) (0xff & pp_siginfo->si_int);
    // the index to client buffer is the payload shifted over 8 bits.
    lv_idx = (pp_siginfo->si_int >> 8) & 0xfffff;

    if (cv_trace) {
        const char *lp_type_str = get_type_str(lv_type);
        trace_where_printf(WHERE,
                           "ENTER, got ctl msg: type=%d(%s), val=0x%lx, idx=%d, sig=%d\n",
                           lv_type, lp_type_str, (long) pp_siginfo->si_int, lv_idx, pv_sig);
    }
    
    if (lv_idx < 0 || lv_idx > iv_client_buffers_max) {
        if (cv_trace) {
            trace_where_printf(WHERE,
                               "ENTER, shared buffer index out of range: idx=%d(0x%x), max=%d\n",
                               lv_idx, lv_idx, iv_client_buffers_max);
        }
        return lv_ret;
    }

    lv_m = (SharedMsgDef *)(ip_cshm+sizeof(SharedMemHdr)
                            +(lv_idx*sizeof(SharedMsgDef)));

    // Bug catcher: shared buffer pid and verifier must match my process
    //              unless it is a notice from monitor process
    LIOTM_assert((lv_type == MC_NoticeReady) ||
                 (iv_pid ==-1 && iv_verifier ==-1) ||
                 (iv_pid == lv_m->trailer.OSPid && iv_verifier == -1) ||
                 (iv_pid == lv_m->trailer.OSPid && 
                  iv_verifier == lv_m->trailer.verifier) ||
                 (lv_m->trailer.OSPid == BCAST_PID && 
                  lv_m->trailer.verifier == -1));
                            
    switch (lv_type) {
    case MC_ReadySend:
        if (cv_trace)
            trace_where_printf(WHERE, "Got Ready Send msg\n");
        // the monitor has told us that it no longer needs this client
        // buffer and that the monitor has no reply data, so release it 
        lv_ret = release_msg(&lv_m->msg);
        break;

    case MC_NoticeReady:
        if (cv_trace)
            trace_where_printf(WHERE, "Got notice msg\n");
        // A notice is available to the client
        lv_ret = process_notice(&lv_m->msg);
        if ( lv_ret == SUCCESS) {
            {
                // Tell the monitor we don't need the shared buffer any more
                if (cv_trace)
                    trace_where_printf(WHERE, "sending Notice Clear to Monitor, buffer #%d\n", 
                                            lv_m->trailer.index);
                if (lv_idx == lv_m->trailer.index)
                    lv_ret = send_ctl_msg( MC_NoticeClear, lv_m->trailer.index );
            }
        }
        break;

    case MC_SReady:
        if (cv_trace)
            trace_where_printf(WHERE, "Got Recv msg\n");
        // got a request or a reply from the monitor, 
        // signal IO completion to waiting thread
        lv_ret = acquire_lock();
        if ( lv_ret == SUCCESS) {
            // tell the waiting thread of the completion
            lv_m->trailer.received = true;
            lv_ret = signal_cv();
            

            if ( lv_ret == SUCCESS) {
                lv_ret = release_lock();
                LIOTM_assert(lv_ret == SUCCESS);
            }
            else {
                LIOTM_assert(release_lock() == SUCCESS);
            }
        }
        
        break;


    default:
        if (cv_trace)
            trace_where_printf(WHERE,
                               "This is a strange place to be, type: %d\n", lv_type);
        break;
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d\n", lv_ret);

    return lv_ret;
}

const char *Local_IO_To_Monitor::get_type_str(int pv_type) {
    const char *lp_type_str;
    switch (pv_type)
    {
    case MC_ReadySend:
        lp_type_str = "ReadySend";
        break;
    case MC_NoticeReady:
        lp_type_str = "NoticeReady";
        break;
    case MC_SReady:
        lp_type_str = "SReady";
        break;
    case MC_AttachStartup:
        lp_type_str = "AttachStartup";
        break;
    case MC_NoticeClear:
        lp_type_str = "NoticeClear";
        break;
    default:
        lp_type_str = "<unknown>";
        break;
    }
    return lp_type_str;
}

// this initialization routine will start the reader thread 
bool Local_IO_To_Monitor::init_comm(bool altsig) {
    const char *WHERE = "Local_IO_To_Monitor::init_comm";
    int         retries = 4;
    bool        lv_lio_init = false;
    bool        lv_ret = false;  // assume failure

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER, initted: %d, down %d\n",
                           iv_initted, iv_monitor_down);

    errno = 0;
    iv_altsig = altsig;
    if (iv_initted)
    {
        if (cv_trace)
            trace_where_printf(WHERE, "local IO already initialized\n");
        if (is_monitor_up())
        {
            if (cv_trace)
                trace_where_printf(WHERE, "monitor is up\n");
        } 
        else
        {
            if (cv_trace)
                trace_where_printf(WHERE, "monitor is down\n");
            lv_ret = true;
        }
    } 
    else
    {
        if (cv_trace)
            trace_where_printf(WHERE, "initializing local IO\n");
        do
        {
            if ( init_local_IO() )
            {
                lv_lio_init = true;
                break;
            }
            // we need to wait for the monitor to write the port number
            // only in the startup case
            --retries;
            sleep(1);
        }
        while (retries);
        if (lv_lio_init)
        {
            if (is_monitor_up())
            {
                if (cv_trace)
                    trace_where_printf(WHERE, "monitor is up\n");
                if (altsig)
                    altsig_init();
                // create client worker thread    
                int lv_rc = pthread_create(&iv_recv_tid,
                                            NULL,
                                            local_monitor_reader,
                                            (void *) (long) iv_mpid);
                if ( lv_rc != 0) {
                    errno = lv_rc;
                    lv_ret = false;
                }
                else
                    lv_ret = true; // indicate just initialized
            } 
            else
            {
                if (cv_trace)
                    trace_where_printf(WHERE, "monitor is down\n");
                lv_ret = true;
            }
        }
    }
    
    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d, errno=%d(%s)\n", lv_ret, errno, strerror(errno));

    return (lv_ret);
}

// check if the local monitor is up
bool Local_IO_To_Monitor::init_local_IO() {
    const char *WHERE = "Local_IO_To_Monitor::init_local_IO";
    int         lv_errno;
    bool        lv_ret = false; // assume failure

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");
    
    iv_initted = false;
    errno = 0;
    
    if (cv_trace && iv_virtual_nodes)
        trace_where_printf(WHERE, "virtual nodes=%d\n", iv_nodes);
        
    key_t lv_sharedSegKey = (iv_nid_base << 28)+ 0x10000
                          + (mon_port_num() & 0xFFFF);

    if (cv_trace)
        trace_where_printf(WHERE, "nodes=%d, nid=%d, gv_ms_su_nid=%d, nidBase=%d, sharedSegKey=0x%x\n"
                           , iv_nodes, iv_nid, gv_ms_su_nid, iv_nid_base,
                           lv_sharedSegKey);
        
    size_t lv_shsize = (iv_client_buffers_max * sizeof( SharedMsgDef )) + 
                        sizeof( SharedMemHdr );
   long enableHugePages;
   int lv_shmFlag = SQ_LIO_SHM_PERMISSIONS;
   char *envShmHugePages = getenv("SQ_ENABLE_HUGEPAGES");

   if (envShmHugePages != NULL)
   {
      enableHugePages = (long) atoi(envShmHugePages);
      if (enableHugePages > 0)
      {  
     // Round it 2 MB - Huge page size
     // Possible get the HugePageSize and adjust it accordingly
        lv_shsize = (lv_shsize + (2*1024*1024)) >> 22  << 22;
        if (lv_shsize == 0)
           lv_shsize = 2 *1024 *1024;
        lv_shmFlag =  lv_shmFlag | SHM_HUGETLB;
      }
    }
    iv_cmid = shmget( lv_sharedSegKey, lv_shsize, lv_shmFlag );
    if (iv_cmid == -1) {
        lv_errno = errno;
        if (cv_trace)
            trace_where_printf(WHERE, "failed shmget(0x%x,%d), errno=%d(%s)\n", lv_sharedSegKey, (int)lv_shsize, lv_errno, strerror(lv_errno));
        errno = lv_errno;
    }
    else {
        if (cv_trace)
            trace_where_printf(WHERE, "shared-memory-id=%d, size=%d, key=0x%x\n", iv_cmid, (int)lv_shsize, lv_sharedSegKey);

        ip_cshm = (char *) shmat(iv_cmid, NULL, 0); // cast
        if (ip_cshm == (void *) -1) {
            lv_errno = errno;
            perror( "failed shmat()" );
            if (cv_trace)
                trace_where_printf(WHERE, "failed shmat(%d) errno=%d(%s)\n", iv_cmid, lv_errno, strerror(lv_errno));
            errno = lv_errno;
        }
        else
        {
            iv_mpid = ((SharedMemHdr*)ip_cshm)->mPid;
            if (iv_mpid > 0)
            {
                if (cv_trace)
                    trace_where_printf(WHERE, "shared-memory=%p, monitor pid=%d, nid=%d\n"
                                           , ip_cshm, iv_mpid, iv_nid);
    
                iv_qid = msgget( lv_sharedSegKey, SQ_LIO_MSQ_PERMISSIONS );
                if (iv_qid == -1) {
                    lv_errno = errno;
                    perror( "failed msgget()" );
                    if (cv_trace)
                        trace_where_printf(WHERE, "failed msgget() errno=%d(%s)\n", lv_errno, strerror(lv_errno));
                    // detach from shared memory
                    shmdt(ip_cshm);
                    errno = lv_errno;
                    ip_cshm = NULL;
                    iv_qid = iv_mpid = 0;
                }
                else {
                    ip_cshm_end = ip_cshm + lv_shsize;
                    iv_initted = lv_ret = true;
                }
            }
            // else return false and the caller handle retries
        }
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d, errno=%d(%s)\n", lv_ret, errno, strerror(errno));

    return lv_ret;
}

// check if the local monitor is up
bool Local_IO_To_Monitor::is_monitor_up() {
    const char *WHERE = "Local_IO_To_Monitor::is_monitor_up";
    int         lv_retries = 0;
    bool        lv_ret = false;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");

    LIOTM_assert(iv_initted);
    
    if (iv_mpid)
    {
        iv_monitor_down = true;
        do
        {
            if (kill(iv_mpid,0) == -1)
            {
                if (cv_trace)
                    trace_where_printf(WHERE, "no monitor process, errno=%d(%s)\n", errno, strerror(errno));
                ++lv_retries;
                usleep( 1000 * lv_retries );
                if ( iv_mpid != ((SharedMemHdr*)ip_cshm)->mPid)
                {
                    iv_mpid = ((SharedMemHdr*)ip_cshm)->mPid;
                }
            }
            else
            {
                iv_monitor_down = false;
                lv_ret = true;
            }
        }
        while( iv_monitor_down && lv_retries < 10 );
        if ( iv_monitor_down && iv_initted )
        {
            // detach from shared memory
            shmdt(ip_cshm);
            ip_cshm = NULL;
            iv_qid = iv_mpid = 0;
            iv_initted = false;
        }
    }
    else
    {
        if (cv_trace)
            trace_where_printf(WHERE, "monitor pid=%d\n", iv_mpid);
        iv_monitor_down = true;
        lv_ret = false;
    }
    
    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d\n", lv_ret);

    return lv_ret;
}

// process a notice received from the monitor
int Local_IO_To_Monitor::process_notice(struct message_def *pp_msg) {
    const char  *WHERE = "Local_IO_To_Monitor::process_notice";
    int          lv_ret = SUCCESS;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER, msgtype: %d\n", pp_msg->type);

    switch (pp_msg->type) {
    case MsgType_Close:
    case MsgType_Open:
    case MsgType_Change:
    case MsgType_ProcessCreated:
        if (cv_trace)
            trace_where_printf(WHERE,
                               "calling recv callback %d received\n",
                                pp_msg->u.request.type );
        if (ip_recv_cb)
        {
            ip_recv_cb(pp_msg, size_of_msg(pp_msg));
        }
        else
        {
            lv_ret = put_on_notice_list( pp_msg, size_of_msg(pp_msg));
        }
        break;

    case MsgType_NodeAdded:
    case MsgType_NodeChanged:
    case MsgType_NodeDeleted:
    case MsgType_NodeDown:
    case MsgType_NodeJoining:
    case MsgType_NodeQuiesce:
    case MsgType_NodeUp:
    case MsgType_SpareUp:
    case MsgType_ProcessDeath:
    case MsgType_ReintegrationError:
    case MsgType_Shutdown:
        if (cv_trace)
            trace_where_printf(WHERE,
                              "notice %d received\n",
                               pp_msg->u.request.type);
        if (ip_notice_cb)
            ip_notice_cb(pp_msg, size_of_msg(pp_msg));
        else
            lv_ret = put_on_notice_list( pp_msg, size_of_msg(pp_msg));
        break;

    case MsgType_Event:
        if (cv_trace)
            trace_where_printf(WHERE,
                              "Event %d received\n",
                              pp_msg->u.request.u.event_notice.event_id);
        if (ip_event_cb)
        {
            ip_event_cb(pp_msg, size_of_msg(pp_msg));
        }
        else
        {
            lv_ret = put_on_event_list( pp_msg, size_of_msg(pp_msg));
        }
        break;

    default:
        if (cv_trace)
            trace_where_printf(WHERE,
                               "Invalid Notice Type(%d) received\n",
                               pp_msg->type);
        lv_ret = FAILURE;
        LIOTM_assert(0);
        break;
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d\n", lv_ret);

    return lv_ret;
}

// Put it on the event list to be removed with wait_for_event()
int Local_IO_To_Monitor::put_on_event_list( struct message_def *pp_msg,
                                              int pv_size ) {
    const char  *WHERE = "Local_IO_To_Monitor::put_on_event_list";
    int          lv_ret = SUCCESS;
    struct message_def *lv_event = new struct message_def;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER - event size %d\n", pv_size);

    memcpy( lv_event, pp_msg, pv_size );

    lv_ret = acquire_ev_lock();
    if ( lv_ret == SUCCESS) {
        iv_event_list.push_back( lv_event );
        lv_ret = signal_event_cv();
        if ( lv_ret == SUCCESS)
            lv_ret = release_ev_lock();
        else
            release_ev_lock();
    }
    else
    {
        delete lv_event;
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d, errno=%d(%s)\n", lv_ret, errno, strerror(errno));

    return(lv_ret);
}

// if a notice cannot be processed right away, put it on the notice list
// to be removed with get_notice()
int Local_IO_To_Monitor::put_on_notice_list( struct message_def *pp_msg,
                                              int pv_size ) {
    const char  *WHERE = "Local_IO_To_Monitor::put_on_notice_list";
    int          lv_ret = SUCCESS;
    struct message_def *lv_notice = new struct message_def;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER - notice size %d\n", pv_size);

    memcpy( lv_notice, pp_msg, pv_size );

    lv_ret = acquire_lock();
    if ( lv_ret == SUCCESS) {
        iv_notice_list.push_back( lv_notice );
        lv_ret = signal_cv();
        if ( lv_ret == SUCCESS)
        {
            lv_ret = release_lock();
            LIOTM_assert(lv_ret == SUCCESS);
        }
        else
        {
            LIOTM_assert(release_lock() == SUCCESS);
        }
    }
    else
    {
        delete lv_notice;
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d, errno=%d(%s)\n", lv_ret, errno, strerror(errno));

    return(lv_ret);
}

// get the first notice from the list
int Local_IO_To_Monitor::get_notice( struct message_def **pp_msg, bool wait ) {
    const char   *WHERE = "Local_IO_To_Monitor::get_notice";
    int          lv_ret = SUCCESS;
    struct message_def *lp_msg = NULL;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");

    errno = 0;
    if (!iv_notice_list.empty() || wait)
    {
        lv_ret = acquire_lock();
        if ( lv_ret == SUCCESS) {
            while (iv_notice_list.empty())
            {
                if (cv_trace)
                    trace_where_printf(WHERE, "waiting for notice\n");

                lv_ret = wait_on_cv();
                if ( lv_ret != SUCCESS) {
                    break;
                }
            }
            if ( lv_ret == SUCCESS) {
                lp_msg = iv_notice_list.front();
                iv_notice_list.pop_front();
            }
            lv_ret = release_lock();
            LIOTM_assert(lv_ret == SUCCESS);
        }
    }

    if (lv_ret == FAILURE)
        lp_msg = NULL;
        
    *pp_msg = lp_msg;

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, notice=%p, ret=%d, errno=%d(%s)\n"
                          , (void *) lp_msg, lv_ret, errno, strerror(errno));

    return lv_ret;
}


// release the lock used to synchronize the threads on send/recv/notice
int Local_IO_To_Monitor::release_lock( bool pv_show ) {
    const char *WHERE = "Local_IO_To_Monitor::release_lock";
    int         lv_ret = SUCCESS;

    if (cv_trace && pv_show)
        trace_where_printf(WHERE, "ENTER\n");

    lv_ret = pthread_mutex_unlock(&iv_sr_lock);
    if ( lv_ret != 0) {
        char la_buf[256];
        sprintf(la_buf, "[%s], pthread_mutex_unlock returned error=%d (%s)\n",
                WHERE, lv_ret, strerror(lv_ret));
        log_error ( MON_CLIO_RELEASE_LOCK_1, SQ_LOG_ERR, la_buf );

        errno = lv_ret;
        lv_ret = FAILURE;
    }
    else {
        errno = 0;
    }

    if (cv_trace && pv_show)
        trace_where_printf(WHERE, "EXIT, ret=%d\n", lv_ret);

    return lv_ret;
}

// release the lock used to synchronize the threads on event
int Local_IO_To_Monitor::release_ev_lock( bool pv_show ) {
    const char *WHERE = "Local_IO_To_Monitor::release_ev_lock";
    int         lv_ret = SUCCESS;

    if (cv_trace && pv_show)
        trace_where_printf(WHERE, "ENTER\n");

    lv_ret = pthread_mutex_unlock(&iv_ev_lock);
    if ( lv_ret != 0) {
        char la_buf[256];
        sprintf(la_buf, "[%s], pthread_mutex_unlock returned error=%d (%s)\n",
                WHERE, lv_ret, strerror(lv_ret));
        log_error ( MON_CLIO_RELEASE_LOCK_2, SQ_LOG_ERR, la_buf );

        errno = lv_ret;
        lv_ret = FAILURE;
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d, errno=%d(%s)\n", lv_ret, errno, strerror(errno));

    return lv_ret;
}

// release a client buffer back to the available pool
int Local_IO_To_Monitor::release_msg(struct message_def *pp_msg, 
                                      bool pv_lock) {
    const char      *WHERE = "Local_IO_To_Monitor::release_msg";
    int              lv_ret = SUCCESS;
    ClientBufferInfo lv_cbi;
    SharedMsgDef    *lp_sr_msg = reinterpret_cast<SharedMsgDef *>(pp_msg);
    int              lv_buffer_index = -1;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER, buff=%p\n", (void *) pp_msg);

    errno = 0;
    
    if (pv_lock)
    {
        lv_ret = acquire_lock();
        if (lv_ret == FAILURE) {
            if (cv_trace)
                trace_where_printf(WHERE, "EXIT, ret=%d\n", lv_ret);
            return(lv_ret);
        }
    }

    if ((char*)pp_msg >= ip_cshm && 
        (char*)pp_msg  < ip_cshm_end )
    {   // Buffer to be released is in local io buffer shared memory region

        lv_buffer_index = lp_sr_msg->trailer.index;

        if ( lp_sr_msg->trailer.bufInUse != -1
        &&   lp_sr_msg->trailer.bufInUse != iv_pid)
        {  // Unexpectedly, buffer is not owned by this process
            char la_buf[256];
            sprintf(la_buf, "[%s], attempt to release buffer %d but it is not"
                    " owned by this process.  Last freed by pid=%d, verifier%d\n",
                    WHERE, 
                    lv_buffer_index, 
                    lp_sr_msg->trailer.OSPid,
                    lp_sr_msg->trailer.verifier);
            log_error ( MON_CLIO_RELEASE_MSG_1, SQ_LOG_ERR, la_buf );
        }
        else if (lv_buffer_index >= 0
              && lv_buffer_index < iv_client_buffers_max)
        {  // buffer index is in valid range

            // reset trailer values
            lp_sr_msg->trailer.received = 0;
            lp_sr_msg->trailer.attaching = 0;
            lp_sr_msg->trailer.bufInUse = 0;
            lp_sr_msg->trailer.OSPid = iv_pid; // Identify releaser
            lp_sr_msg->trailer.verifier = iv_verifier;   // Identify releaser

            // put the buffer back in the shared buffer pool
            lv_cbi.mtype = SQ_LIO_NORMAL_MSG;
            lv_cbi.index = lv_buffer_index;
            lv_ret = msgsnd( iv_qid, &lv_cbi, sizeof( lv_cbi.index ), 0);
            if (lv_ret == -1) {
                lv_ret = errno;
                if (cv_trace)
                    trace_where_printf(WHERE,
                                       "EXIT, msgsnd() failed returning buffer"
                                       " to shared pool, errno=%d (%s)\n", 
                                       errno, strerror(errno));
                if (pv_lock) {
                    LIOTM_assert(release_lock() == SUCCESS);
                }
                return(lv_ret);
            }

            if (cv_trace) {
                struct msqid_ds  lv_mds;

                trace_where_printf(WHERE, "returned buffer to shared pool. "
                                   "idx=%d\n", lp_sr_msg->trailer.index);

                --iv_acquired_buffer_count;

                lv_ret = msgctl(iv_qid, IPC_STAT, &lv_mds);
                if (lv_ret != -1) {
                    trace_where_printf(WHERE, 
                                       "(qid=%d) shared buffers available=%d, "
                                       "acquired=%d, errno=%d\n",
                                       iv_qid, (int)lv_mds.msg_qnum,
                                       iv_acquired_buffer_count, errno);
                }
            }

            // todo: why is this needed?
            lv_ret = signal_cv();
        }
        else
        {  // Invalid buffer index
            char la_buf[256];
            sprintf(la_buf, "[%s], attempt to release buffer %d but index is "
                    " invalid.  Buffer address=%p\n",
                    WHERE, lv_buffer_index, (void *) lp_sr_msg);
            log_error ( MON_CLIO_RELEASE_MSG_2, SQ_LOG_ERR, la_buf );
        }
    } else {
        // not local io shared memory. must be heap memory, delete it
        if (cv_trace) {
            trace_where_printf(WHERE, 
                               "deleting buffer, buf=%p\n", (void *)pp_msg);
        }
        delete pp_msg;
    } 

    if (pv_lock) {
        lv_ret = release_lock();
        LIOTM_assert(lv_ret == SUCCESS);
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d, errno=%d(%s)\n", lv_ret, errno, strerror(errno));

    return lv_ret;
}

// send a request to the monitor that does not expect a reply
int Local_IO_To_Monitor::send(struct message_def *pp_msg) {
    const char *WHERE = "Local_IO_To_Monitor::send";
    int         lv_ret = SUCCESS;
    SharedMsgDef *lp_sr_msg = reinterpret_cast<SharedMsgDef *>(pp_msg);

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER, Sending req type %d\n",
                           pp_msg->u.request.type);

    errno = 0;
    if (pp_msg != &lp_sr_msg->msg) {
        if (cv_trace)
            trace_where_printf(WHERE,
                               "EXIT, pp_msg did not come from 'acquire_msg()'\n");
        errno = EINVAL;
        return FAILURE;
    }
    if (!pp_msg->noreply) {
        if (cv_trace)
            trace_where_printf(WHERE,
                               "EXIT, cannot use send with noreply == false\n");
        errno = EINVAL;
        return FAILURE;
    }
    lp_sr_msg->trailer.received = false;
    lv_ret = send_ctl_msg(MC_SReady, lp_sr_msg->trailer.index );

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d\n", lv_ret);

    return lv_ret;
}

// send an rt signal control message to the monitor
int Local_IO_To_Monitor::send_ctl_msg(MonitorCtlType pv_type, int pv_index) {
    const char   *WHERE = "Local_IO_To_Monitor::send_ctl_msg";
    int          lv_ret = SUCCESS;
    sigval       lv_value;
    SharedMsgDef *lp_msg = NULL;

    if (cv_trace) {
        trace_where_printf(WHERE, "ENTER\n");
    }
    
    assert( pv_index >= 0 );

    lv_value.sival_int = ((pv_index&0xfffff) << 8) | (pv_type & 0xff);
    if ( pv_type == MC_SReady || pv_type == MC_AttachStartup )
    {
        lp_msg = ((SharedMsgDef *)(ip_cshm+sizeof(SharedMemHdr)+
                                   (pv_index*sizeof(SharedMsgDef))));
        lp_msg->trailer.OSPid = iv_pid;
        lp_msg->trailer.verifier = iv_verifier;
    }

    if (cv_trace) {
        const char *lp_type_str = get_type_str(pv_type);
        trace_where_printf(WHERE,
                           "sending ctl msg: type=%d(%s), val=0x%x, idx=%d, pid=%d\n", 
                           pv_type, lp_type_str, lv_value.sival_int, pv_index, iv_pid);
    }

    int err;
    do {
        err = errno = 0;
        lv_ret = sigqueue(iv_mpid, SQ_LIO_SIGNAL_REQUEST_REPLY, lv_value);
        if (lv_ret != 0) {
            err = errno; // save initial error
            if(errno != ESRCH && errno != EAGAIN) {
                if (cv_trace)
                    trace_where_printf(WHERE, "sigqueue() failed, ret=%d, errno=%d(%s)\n"
                                      , lv_ret, err, strerror(err));
            }
        }
    } while( errno == EAGAIN );
    
    if (lv_ret == -1) {
        int rc;
        switch (pv_type) {
        case MC_SReady:
        case MC_AttachStartup:
            // Could not signal the monitor so complete the message to the client
            // Ignore any subsequent errors
            if (lp_msg != NULL)
                lp_msg->trailer.received = true;
            rc = acquire_lock();
            if (rc == FAILURE) {
                break;
            }
            signal_cv();
            LIOTM_assert(release_lock() == SUCCESS);
            break;

        default:
          break;
        }
    }

    // restore initial error
    errno = err; 
    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d, errno=%d(%s)\n"
                          , lv_ret, errno, strerror(errno));

    return(lv_ret);
}

// this routine sends a message to the monitor and waits for a reply
int Local_IO_To_Monitor::send_recv(struct message_def *pp_msg,
                                             bool          pv_nw) {
    const char   *WHERE = "Local_IO_To_Monitor::send_recv";
    int           lv_ret = SUCCESS;
    SharedMsgDef *lp_sr_msg = reinterpret_cast<SharedMsgDef *>(pp_msg);

    if (cv_trace)
        trace_where_printf(WHERE,
                           "Sending req type %d, idx=%d\n",
                           pp_msg->u.request.type, lp_sr_msg->trailer.index);

    if (lp_sr_msg->trailer.attaching)
        lv_ret = send_ctl_msg(MC_AttachStartup, lp_sr_msg->trailer.index);
    else
    {
        lp_sr_msg->trailer.received = false; // dg
        lv_ret = send_ctl_msg(MC_SReady, lp_sr_msg->trailer.index);
    }

    if ( lv_ret == SUCCESS) {
        lv_ret = acquire_lock();

        if ( lv_ret == SUCCESS) {
            while (!lp_sr_msg->trailer.received && !pv_nw && !iv_shutdown) {

                if (cv_trace)
                    trace_where_printf(WHERE, "Waiting for response\n");

                lv_ret = wait_on_cv();
                if ( lv_ret != SUCCESS) {
                    break;
                }
                else {
                    if (iv_unknown_process) {
                        if (cv_trace)
                            trace_where_printf(WHERE,
                               "got iv_unknown_process from monitor, returning NULL\n");
                        break;
                    }
                }
            }

            if ( lv_ret == FAILURE || 
                 iv_shutdown ||
                 lp_sr_msg->trailer.received ) {
                lp_sr_msg->trailer.received = false;
            }

            if ( lv_ret == SUCCESS)
            {
                lv_ret = release_lock();
                LIOTM_assert(lv_ret == SUCCESS);
            }
            else    
            {
                LIOTM_assert(release_lock() == SUCCESS);
            }
        }
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, msg=%p, ret=%d, errno=%d(%s)\n"
                          , (void *) pp_msg, lv_ret, errno, strerror(errno));
    return(lv_ret);
}

// sets the callback routines
int Local_IO_To_Monitor::set_cb(CallBack pp_cb, const char *pp_type) {
    const char      *WHERE = "Local_IO_To_Monitor::set_cb";

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");

    if (strcasecmp(pp_type, "notice") == 0)
        ip_notice_cb = pp_cb;
    else if (strcasecmp(pp_type, "event") == 0)
        ip_event_cb = pp_cb;
    else if (strcasecmp(pp_type, "recv") == 0)
        ip_recv_cb = pp_cb;
    else if (strcasecmp(pp_type, "unsol") == 0)
        ip_unsol_cb = pp_cb;
    else {
        if (cv_trace)
            trace_where_printf(WHERE, "EXIT, invalid callback\n");
        errno = EINVAL;
        return(FAILURE);
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT\n");

    return(SUCCESS);
}

// wakes up a thread waiting on the localio event cv
int Local_IO_To_Monitor::signal_event_cv() {
    const char *WHERE = "Local_IO_To_Monitor::signal_event_cv";
    int         lv_ret = SUCCESS;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");

    iv_ev_signaled = true;
    lv_ret = pthread_cond_broadcast(&iv_ev_cv);
    if ( lv_ret != 0) {
        errno = lv_ret;
        lv_ret = FAILURE;
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d, errno=%d(%s)\n", lv_ret, errno, strerror(errno));

    return(lv_ret);
}

// wakes up a thread waiting on the localio send/recv cv
int Local_IO_To_Monitor::signal_cv( int err ) {
    const char *WHERE = "Local_IO_To_Monitor::signal_cv";
    int         lv_ret = SUCCESS;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");

    if ( err == EINTR ) {
        iv_sr_interrupted = true;
    }
    else {
        iv_sr_signaled = true;
    }
    lv_ret = pthread_cond_broadcast(&iv_sr_cv);
    if ( lv_ret != 0) {
        errno = lv_ret;
        lv_ret = FAILURE;
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d, errno=%d(%s)\n", lv_ret, errno, strerror(errno));

    return(lv_ret);
}

// end all local IO processing
void Local_IO_To_Monitor::shutdown( void ) {
    const char *WHERE = "Local_IO_To_Monitor::shutdown";
    int         lv_ret = SUCCESS;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");

    // on error from called method, just return
    
    errno = 0;
    lv_ret = gp_local_mon_io->acquire_lock();
    if (lv_ret == FAILURE ) {
        if (cv_trace)
            trace_where_printf(WHERE, "EXIT\n");
        return;
    }
    gp_local_mon_io->iv_shutdown = true;
    lv_ret = gp_local_mon_io->signal_cv();
    if (lv_ret == FAILURE ) {
        if (cv_trace)
            trace_where_printf(WHERE, "EXIT\n");
        return;
    }
    lv_ret = gp_local_mon_io->release_lock();
    if (lv_ret == FAILURE ) {
        if (cv_trace)
            trace_where_printf(WHERE, "EXIT\n");
        return;
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT\n");
}

// the size of the message.  used to reduce the amount of copy
int Local_IO_To_Monitor::size_of_msg( struct message_def *pp_msg, bool reply) {
    const char      *WHERE = "Local_IO_To_Monitor::size_of_msg";
    size_t lv_len;
    // Must use the first structure in union to get correct offset
    long lv_preamble = (long)&pp_msg->u.request.u.shutdown - (long)pp_msg;

    switch (pp_msg->type) {
    case MsgType_Change:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.change);
        break;

    case MsgType_Close:
        if (reply)
            lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.close);
        else
            lv_len = lv_preamble + sizeof(pp_msg->u.request.u.close);
        break;

    case MsgType_Event:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.event_notice);
        break;

    case MsgType_NodeAdded:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.node_added);
        break;

    case MsgType_NodeChanged:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.node_changed);
        break;

    case MsgType_NodeDeleted:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.node_deleted);
        break;

    case MsgType_NodeDown:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.down);
        break;

    case MsgType_NodeJoining:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.joining);
        break;

    case MsgType_NodeQuiesce:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.quiesce);
        break;

    case MsgType_NodeUp:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.up);
        break;

    case MsgType_Open:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.open);
        break;

    case MsgType_ProcessCreated:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.process_created);
        break;

    case MsgType_ProcessDeath:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.death);
        break;

    case MsgType_ReintegrationError:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.reintegrate);
        break;

    case MsgType_Shutdown:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.shutdown);
        break;

    case MsgType_SpareUp:
        lv_len = lv_preamble + sizeof(pp_msg->u.request.u.spare_up);
        break;

    case MsgType_Service:
        if (reply) {

            switch (pp_msg->u.reply.type) {
            case ReplyType_Generic:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.generic);
                break;
            case ReplyType_Dump:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.dump);
                break;
            case ReplyType_Get:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.get);
                break;
            case ReplyType_MonStats:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.mon_info);
                break;
            case ReplyType_Mount:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.mount);
                break;
            case ReplyType_NewProcess:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.new_process);
                break;
            case ReplyType_NodeInfo:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.node_info);
                break;
            case ReplyType_NodeName:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.nodename);
                break;
            case ReplyType_Open:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.open);
                break;
            case ReplyType_OpenInfo:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.open_info);
                break;
            case ReplyType_PNodeInfo:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.pnode_info);
                break;
            case ReplyType_ProcessInfo:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.process_info);
                break;
            case ReplyType_Startup:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.startup_info);
                break;
            case ReplyType_ZoneInfo:
                lv_len = lv_preamble + sizeof(pp_msg->u.reply.u.zone_info);
                break;
            default:
                lv_len = sizeof(*pp_msg);
                break;
            }
        } else {
            switch (pp_msg->u.request.type) {
            case ReqType_Close:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.close);
                break;
            case ReqType_Dump:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.dump);
                break;
            case ReqType_Event:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.event);
                break;
            case ReqType_Exit:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.exit);
                break;
            case ReqType_Get:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.get);
                break;
            case ReqType_Kill:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.kill);
                break;
            //   ReqType_MonStats: see default: below
            case ReqType_Mount:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.mount);
                break;
            case ReqType_NewProcess:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.new_process);
                break;
            case ReqType_NodeAdd:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.node_add);
                break;
            case ReqType_NodeDelete:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.node_delete);
                break;
            case ReqType_NodeDown:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.down);
                break;
            case ReqType_NodeInfo:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.node_info);
                break;
            case ReqType_NodeName:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.nodename);
                break;
            case ReqType_NodeUp:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.up);
                break;
            case ReqType_Notice:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.event_notice);
                break;
            case ReqType_Notify:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.notify);
                break;
            case ReqType_Open:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.open);
                break;
            case ReqType_OpenInfo:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.open_info);
                break;
            //TODO:
            //case ReqType_PersistAdd:
            //    lv_len = lv_preamble + sizeof(pp_msg->u.request.u.persist);
            //    break;
            //case ReqType_PersistDelete:
            //    lv_len = lv_preamble + sizeof(pp_msg->u.request.u.persist);
            //    break;
            case ReqType_PNodeInfo:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.pnode_info);
                break;
            case ReqType_ProcessInfo:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.process_info);
                break;
            case ReqType_ProcessInfoCont:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.process_info_cont);
                break;
            case ReqType_Set:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.set);
                break;
            case ReqType_Shutdown:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.shutdown);
                break;
            case ReqType_Startup:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.startup);
                break;
            case ReqType_TmLeader:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.leader);
                break;
            case ReqType_TmReady:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.tm_ready);
                break;
            case ReqType_ZoneInfo:
                lv_len = lv_preamble + sizeof(pp_msg->u.request.u.zone_info);
                break;
            case ReqType_MonStats:
            default:
                lv_len = sizeof(*pp_msg);
                break;
            }
        }
        break;
    default:
        lv_len = sizeof(*pp_msg);
        break;
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, len=%d\n", (int) lv_len);

    return (int) lv_len;
}

// trace all message trailer info
void Local_IO_To_Monitor::trace_print_msg(const char   *pp_where,
                                          SharedMsgDef *pp_msg) {
    trace_where_printf(pp_where,
                       "shared-msg: addr=%p, msg.type=%d, rcvd=%d, att=%d, index=%d, ospid=%d, verifier=%d\n",
                       (void *) pp_msg,
                       pp_msg->msg.type,
                       pp_msg->trailer.received,
                       pp_msg->trailer.attaching,
                       pp_msg->trailer.index,
                       pp_msg->trailer.OSPid,
                       pp_msg->trailer.verifier);
}

// trace routine for localio
void Local_IO_To_Monitor::trace_where_printf(const char *pp_where,
                                             const char *pp_format, ...) {
    va_list  lv_ap;
    if (cp_trace_cb != NULL) {
        va_start(lv_ap, pp_format);
        cp_trace_cb(pp_where, pp_format, lv_ap);
        va_end(lv_ap);
    }
}

// wait for the monitor to start
bool Local_IO_To_Monitor::wait_for_monitor( int & status ) {
    const char   *WHERE = "Local_IO_To_Monitor::wait_for_monitor";

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");

    struct stat statbuf;
    bool monitorStarted = false;
    int tries = 0;
    status = 0;

    // loop, waiting for monitor port file
    while ( !monitorStarted  && tries < iv_port_file_tries)
    {
        sleep(1);
        if (stat ( ip_port_fname, &statbuf) == -1)
        {
            status = errno;
            if (errno == ENOENT)
            {   // Port file does not yet exist
                if (cv_trace)
                    trace_where_printf(WHERE, "monitor port file %s does not yet exist\n", ip_port_fname);
            }
            else
            {
                if (cv_trace)
                    trace_where_printf(WHERE, "error doing stat on monitor port file %s: %d (%s)\n", ip_port_fname, errno, strerror(errno));
            }
            ++tries;
        }
        else
        {
            monitorStarted = true;
        }
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT monitor is up=%d, tries=%d\n",
                           monitorStarted, tries);

    return monitorStarted;
}

// wait for an event
int Local_IO_To_Monitor::wait_for_event(struct message_def **pp_msg) {
    const char   *WHERE = "Local_IO_To_Monitor::wait_for_event";
    int           lv_ret = SUCCESS;
    struct message_def *lp_event = NULL;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");

    errno = 0;
    lv_ret = acquire_ev_lock();
    if ( lv_ret == SUCCESS) {
        while (iv_event_list.empty())
        {
            if (cv_trace)
                trace_where_printf(WHERE, "waiting for event\n");

            lv_ret = wait_on_event_cv();
            if ( lv_ret != SUCCESS) {
                break;
            }
        }
        if ( lv_ret == SUCCESS) {
            lp_event = iv_event_list.front();
            iv_event_list.pop_front();
            lv_ret = release_ev_lock();
            if ( lv_ret != SUCCESS) {
                lp_event = NULL;
                lv_ret = FAILURE;
            }
        }
        else
            release_ev_lock();
    }

    *pp_msg = lp_event;
    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, event=%p, ret=%d, errno=%d(%s)\n"
                          , (void *) lp_event, lv_ret, errno, strerror(errno));
    return(lv_ret);
}

// wait on the event cv
int Local_IO_To_Monitor::wait_on_event_cv() {
    const char *WHERE = "Local_IO_To_Monitor::wait_on_event_cv";
    int         lv_ret = SUCCESS;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");

    if (!iv_ev_signaled) {
        lv_ret = pthread_cond_wait(&iv_ev_cv, &iv_ev_lock);
        if ( lv_ret != 0) {
            errno = lv_ret;
            lv_ret = FAILURE;
        }
    }
    iv_ev_signaled = false;

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d, errno=%d(%s)\n", lv_ret, errno, strerror(errno));

    return(lv_ret);
}

// wait on the localio cv
int Local_IO_To_Monitor::wait_on_cv() {
    const char *WHERE = "Local_IO_To_Monitor::wait_on_cv";
    int         lv_ret = SUCCESS;

    if (cv_trace)
        trace_where_printf(WHERE, "ENTER\n");

    if (!iv_sr_signaled) {
        lv_ret = pthread_cond_wait(&iv_sr_cv, &iv_sr_lock);
        if ( lv_ret != 0) {
            errno = lv_ret;
            lv_ret = FAILURE;
        }
        if ( iv_sr_interrupted ) {
            errno = EINTR;
            lv_ret = FAILURE;
        }
        iv_sr_interrupted = false;
    }
    iv_sr_signaled = false;
    if (iv_shutdown && (lv_ret == SUCCESS)) {
        errno = ESRCH;
        lv_ret = FAILURE;
    }

    if (cv_trace)
        trace_where_printf(WHERE, "EXIT, ret=%d, errno=%d(%s)\n", lv_ret, errno, strerror(errno));

    return(lv_ret);
}

const char * Local_IO_To_Monitor::msgTypes_[] = {
    "invalid",
    "Change",
    "Close",
    "Event",
    "NodeAdded",
    "NodeChanged",
    "NodeDeleted",
    "NodeDown",
    "NodeJoining",
    "NodeQuiesce",
    "NodeUp",
    "Open",
    "ProcessCreated",
    "ProcessDeath",
    "ReintegrationError",
    "Service",
    "Shutdown",
    "SpareUp",
    "invalid"
};

const char * Local_IO_To_Monitor::reqTypes_[] = {
    "invalid",
    "Close",
    "DelProcessNs",
    "Dump",
    "Event",
    "Exit",
    "Get",
    "InstanceId",
    "Kill",
    "MonStats",
    "Mount",
    "NameServerAdd",
    "NameServerDelete",
    "NameServerStart",
    "NameServerStop",
    "NewProcess",
    "NewProcessNs",
    "NodeAdd",
    "NodeDelete",
    "NodeDown",
    "NodeInfo",
    "NodeName",
    "NodeUp",
    "Notice",
    "Notify",
    "Open",
    "OpenInfo",
    "PersistAdd",
    "PersistDelete",
    "PNodeInfo",
    "ProcessInfo",
    "ProcessInfoCont",
    "ProcessInfoNs",
    "Set",
    "Shutdown",
    "ShutdownNs",
    "Startup",
    "TmLeader",
    "TmReady",
    "ZoneInfo",
    "invalid"
};

const char * Local_IO_To_Monitor::replyTypes_[] = {
    "Generic",
    "DelProcessNs",
    "Dump",
    "Get",
    "MonStats",
    "Mount",
    "NewProcess",
    "NewProcessNs",
    "NodeInfo",
    "NodeName",
    "Open",
    "OpenInfo",
    "PNodeInfo",
    "ProcessInfo",
    "ProcessInfoNs",
    "Startup",
    "ZoneInfo",
    "invalid"
};

void Local_IO_To_Monitor::scan_liobufs ( void ) {

    SharedMsgDef *shm;
    int msgType;
    int msgReqReplyType;

    printf ( "Scanning %d buffers on node %d\n", iv_client_buffers_max,
             iv_nid );
    for (int index=0; index < iv_client_buffers_max; ++index)
    {
        shm = (SharedMsgDef *)(ip_cshm+sizeof(SharedMemHdr)
                               +(index*sizeof(SharedMsgDef)));
        if ( shm->trailer.bufInUse != 0 )
        {   // Buffer is in use
            char timestring[50];
            STRCPY ( timestring, ctime ( &shm->trailer.timestamp.tv_sec ));
            timestring[strlen(timestring)-1] = '\0';

            msgType = shm->msg.type;

            if ( msgType == MsgType_Service )
            { // Service request or reply
                msgReqReplyType = shm->msg.u.request.type;

                if ( msgReqReplyType < ReplyType_Generic )
                { // A request
                    if ( msgReqReplyType >= ReqType_Invalid)
                        msgReqReplyType = ReqType_Invalid;

                    printf( "buffer #%d, request=%s, owner=%d, acquired %s\n",
                            index, reqTypes_[msgReqReplyType],
                            shm->trailer.bufInUse, timestring);
                }
                else
                { // A reply
                    if ( msgReqReplyType >= ReplyType_Invalid)
                        msgReqReplyType = ReplyType_Invalid;
                    msgReqReplyType -= ReplyType_Generic;

                    printf( "buffer #%d, reply=%s, owner=%d, acquired %s\n",
                            index, replyTypes_[msgReqReplyType],
                            shm->trailer.bufInUse, timestring);
                }
            }
            else
            {
                if ( msgType >= MsgType_Invalid ) msgType = MsgType_Invalid; 

                printf( "buffer #%d, message=%s, owner=%d, acquired %s\n",
                        index, msgTypes_[msgType],
                        shm->trailer.bufInUse, timestring);
            }
        }
    }
}

int Local_IO_To_Monitor::getCurrentBufferCount()
{
    struct msqid_ds buf;

    if (msgctl(iv_qid, IPC_STAT, &buf) == -1)
    {
        return -1;
    }
    else
    {
        return (int)buf.msg_qnum;
    }
}
