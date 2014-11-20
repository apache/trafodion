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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "seabed/fs.h"
#include "seabed/ms.h"
#include "seabed/trace.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

#define FIFO1 "/tmp/zt205fifo1"
#define FIFO2 "/tmp/zt205fifo2"


enum { MAX_ARGS = 100 };
enum { MAX_SRV  = 100 };
enum {
    T_CLOSE,
    T_FORKEXEC,
    T_FORKEXEC_SU,
    T_NEWPROC,
    T_OPEN,
    T_PROCINFO,
    T_PROCINFO_TYPE,
    T_TOTAL,
    T_MAX
};

short                     filenumr;
int                       maxsp = 10;
bool                      trace = false;

char                      fifo1[100];
char                      fifo2[100];
int                       ffds[2];
MS_Mon_Process_Info_Type  infotype[MAX_SRV];
TPT_DECL                 (phandle);
char                      procname[BUFSIZ];
char                      proc_fname[BUFSIZ];
char                      program_file[BUFSIZ];
char                      recv_buffer[BUFSIZ];
short                     sfilenum[MAX_SRV];
char                      sname[MAX_SRV][100];
int                       soid[MAX_SRV];
TPT_DECL2                (sphandle,MAX_SRV);
char                     *sargv[MAX_ARGS];
char                      sprog[BUFSIZ];
long long                 t_elapsed[T_MAX];
long long                 t_start[T_MAX];
long long                 t_stop[T_MAX];

inline void time_elapsed(int t) {
    long long elapsed = t_stop[t] - t_start[t];
    t_elapsed[t] += elapsed;
}

inline double time_sec(int t) {
    long long sec = t_elapsed[t] / 1000000;
    long long usec = t_elapsed[t] - sec * 1000000;
    double elapsed = (double) sec + ((double) usec / 1000000.0);
    return elapsed;
}

inline void time_start(int t) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    t_start[t] = tv.tv_sec * 1000000 + tv.tv_usec;
}

inline void time_stop(int t) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    t_stop[t] = tv.tv_sec * 1000000 + tv.tv_usec;
}

typedef struct process_extension         PE;
typedef struct process_extension_results PER;

int fifo_close(int fd) {
    int err;

    err = close(fd);
    assert(err == 0);
    return 0;
}

void fifo_create(const char *name1, const char *name2) {
    int  err;
    char errnobuf[100];

    err = mkfifo(name1, 0666);
    if (trace && (err == -1))
        trace_printf("cli: mkfifo(%s) err=%s\n",
                     name1,
                     strerror_r(errno, errnobuf, sizeof(errnobuf)));
    err = mkfifo(name2, 0666);
    if (trace && (err == -1))
        trace_printf("cli: mkfifo(%s) err=%s\n",
                     name2,
                     strerror_r(errno, errnobuf, sizeof(errnobuf)));
}

void fifo_destroy(const char *name1, const char *name2) {
    unlink(name1);
    unlink(name2);
}

int fifo_open(const char *name, int flags) {
    int  err;
    char errnobuf[100];

    do {
        err = ::open(name, flags);
        if (err == -1) {
            if (errno == EINTR)
                continue;
            printf("fifo-open errno=%d(%s)\n",
                   errno,
                   strerror_r(errno, errnobuf, sizeof(errnobuf)));
            assert(err != -1);
        }
    } while (err == -1);
    return err;
}

int main(int argc, char *argv[]) {
    enum                    { MAX_RETRIES = 100 };
    enum                    { SLEEP_US    = 1000 };
    bool                      attach = false;
    _xcc_status               cc;
    bool                      client = false;
    int                       count;
    int                       count_read;
    int                       count_written;
    bool                      dif = false;
    double                    dloop;
    double                    dms;
    double                    dsec;
    int                       err;
    bool                      exec = false;
    int                       ferr;
    bool                      fin = false;
    MS_Mon_Process_Info_Type  info;
    MS_Mon_Process_Info_Type *infop;
    int                       inx;
    int                       inx2;
    int                       inx3;
    short                     len;
    short                     lerr;
    short                     lerr2;
    int                       loop = 10;
    int                       max;
    int                       nid;
    pid_t                     pid;
    int                       sargc;
    ssize_t                   size;
    int                       snid;
    int                       spid;
    bool                      startup = false;
    xzsys_ddl_smsg_def       *sys_msgp = (xzsys_ddl_smsg_def *) recv_buffer;
    int                       sys_msg;
    int                       sys_msg_count;
    bool                      verbose = false;
    TAD                       zargs[] = {
      { "-attach",    TA_Bool, TA_NOMAX,    &attach    },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-dif",       TA_Bool, TA_NOMAX,    &dif       },
      { "-exec",      TA_Bool, TA_NOMAX,    &exec      },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxsp",     TA_Int,  TA_NOMAX,    &maxsp     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-startup",   TA_Bool, TA_NOMAX,    &startup   },
      { "-trace",     TA_Bool, TA_NOMAX,    &trace     },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-verbose",   TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    sprintf(fifo1, "%s-%s", FIFO1, getenv("USER"));
    sprintf(fifo2, "%s-%s", FIFO2, getenv("USER"));
    if (trace)
        msg_init_trace();
    if (exec)
        return 0;
    if (startup) {
        err = fifo_open(fifo1, O_WRONLY);
        assert(err != -1);
        ffds[1] = err;
        err = fifo_open(fifo2, O_RDONLY);
        assert(err != -1);
        ffds[0] = err;
        if (trace)
            trace_printf("cli: writing fifo\n");
        size = write(ffds[1], recv_buffer, 1);
        if (trace)
            trace_printf("cli: fifo write, size=%d\n", (int) size);
        assert(size == 1);
        if (trace)
            trace_printf("cli: fifo written\n");
        close(ffds[1]);
        return 0;
    }
    if (attach)
        ferr = file_init_attach(&argc, &argv, false, NULL);
    else
        ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    util_test_start(client);
    ferr = msg_mon_process_startup(true); // system messages
    util_check("msg_mon_process_startup", ferr);
    ferr = msg_mon_get_my_process_name(procname, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);
    ferr = msg_mon_get_process_info(procname, &nid, &pid);
    TEST_CHK_FEOK(ferr);

    if (trace)
        trace_printf("proc=%s, nid=%d, pid=%d\n", procname, nid, pid);
    dloop = (double) loop;
    for (inx = 0; inx < T_MAX; inx++)
        t_elapsed[inx] = 0.0;
    if (client) {
        printf("loop=%d, maxsp=%d\n", loop, maxsp);
        sargc = argc;
        assert(sargc < MAX_ARGS);
        for (inx2 = 0; inx2 < argc; inx2++) {
            if (strcmp(argv[inx2], "-client") == 0)
                sargv[inx2] = (char *) "-server";
            else
                sargv[inx2] = argv[inx2];
            if (strcmp(argv[inx2], "-attach") == 0)
                sargv[inx2] = (char *) "-server";
        }
        sargv[argc] = NULL;
        sprintf(sprog, "%s/%s", getenv("PWD"), argv[0]);
        time_start(T_TOTAL);
        for (inx = 0; inx < loop; inx += maxsp) {
            if (dif)
                snid = -1;
            else
                snid = nid;
            max = loop - inx;
            if (max > maxsp)
                max = maxsp;
            for (inx2 = 0; inx2 < max; inx2++)
                sname[inx2][0] = 0; // mon picks name
            if (trace)
                trace_printf("cli: newproc, inx=%d\n", inx);
            time_start(T_NEWPROC);
            for (inx2 = 0; inx2 < max; inx2++) {
                ferr = msg_mon_start_process(sprog,                  // prog
                                             sname[inx2],            // name
                                             sname[inx2],            // ret_name
                                             sargc,                  // argc
                                             sargv,                  // argv
                                             TPT_REF2(sphandle,inx2),// phandle
                                             false,                  // open
                                             &soid[inx2],            // oid
                                             MS_ProcessType_Generic, // type
                                             0,                      // priority
                                             false,                  // debug
                                             false,                  // backup
                                             &snid,                  // nid
                                             &spid,                  // pid
                                             NULL,                   // infile
                                             NULL);                  // outfile
                TEST_CHK_FEOK(ferr);
            }
            time_stop(T_NEWPROC);
            time_elapsed(T_NEWPROC);

            // wait here until processes are 'up'
            // so that open timing is correct
            inx3 = 0;
            for (inx2 = 0; inx2 < max; inx2++) {
                ferr = msg_mon_get_process_info_detail(sname[inx2], &info);
                TEST_CHK_FEOK(ferr);
                if (info.state != MS_Mon_State_Up) {
                    inx3++;
                    if (inx3 > MAX_RETRIES) {
                        printf("process %s did not enter 'UP' state\n", sname[inx2]);
                        assert(inx3 < MAX_RETRIES);
                    }
                    usleep(SLEEP_US);
                    inx2--;
                    continue;
                } else
                    inx3 = 0;
            }

            if (trace)
                trace_printf("cli: open, inx=%d\n", inx);
            time_start(T_OPEN);
            for (inx2 = 0; inx2 < max; inx2++) {
                if (trace)
                    trace_printf("cli: opening inx=%d, name=%s\n", inx, sname[inx2]);
                len = (short) strlen(sname[inx2]);
                ferr = BFILE_OPEN_(sname[inx2], len, &sfilenum[inx2],
                                   0, 0, 0,
                                   0, 0, 0, 0);
                if (trace)
                    trace_printf("cli: open, inx=%d, name=%s, ferr=%d\n",
                           inx, sname[inx2], ferr);
                TEST_CHK_FEOK(ferr);
            }
            time_stop(T_OPEN);
            time_elapsed(T_OPEN);

            if (trace)
                trace_printf("cli: procinfo, inx=%d\n", inx);
            time_start(T_PROCINFO);
            for (inx2 = 0; inx2 < max; inx2++) {
                ferr = msg_mon_get_process_info_detail(sname[inx2], &info);
                TEST_CHK_FEOK(ferr);
            }
            time_stop(T_PROCINFO);
            time_elapsed(T_PROCINFO);

            if (trace)
                trace_printf("cli: procinfo-type, inx=%d\n", inx);
            time_start(T_PROCINFO_TYPE);
            ferr = msg_mon_get_process_info_type(MS_ProcessType_Generic,
                                                 &count,
                                                 MAX_SRV,
                                                 infotype);
            TEST_CHK_FEOK(ferr);
            time_stop(T_PROCINFO_TYPE);
            time_elapsed(T_PROCINFO_TYPE);
            if (verbose) {
                for (inx2 = 0; inx2 < count; inx2++) {
                    infop = &infotype[inx2];
                    char s_em = infop->event_messages ? 'E' : '-';
                    char s_sm = infop->system_messages ? 'S' : '-';
                    char s_pr = infop->pending_replication ? 'R' : '-';
                    char s_pd = infop->pending_delete ? 'D' : '-';
                    char s_s  = infop->state == MS_Mon_State_Up ? 'A' : 'U';
                    char s_o  = infop->opened ? 'O' : '-';
                    char s_p  = infop->paired ? 'P' : infop->backup ? 'B' : '-';
                    printf("%3.3d,%8.8d %3.3d %d %c%c%c%c%c%c%c %-11s %-11s %-15s\n",
                           infop->nid,
                           infop->pid,
                           infop->priority,
                           infop->state,
                           s_em, s_sm, s_pr, s_pd, s_s, s_o, s_p,
                           infop->process_name,
                           infop->parent_name,
                           infop->program);
                }
            }

            if (trace)
                trace_printf("cli: close, inx=%d\n", inx);
            time_start(T_CLOSE);
            for (inx2 = 0; inx2 < max; inx2++) {
                ferr = BFILE_CLOSE_(sfilenum[inx2]);
                TEST_CHK_FEOK(ferr);
            }
            time_stop(T_CLOSE);
            time_elapsed(T_CLOSE);

            // re-open/close
            for (inx2 = 0; inx2 < max; inx2++) {
                if (trace)
                    trace_printf("cli: re-opening inx=%d, name=%s\n",
                           inx, sname[inx2]);
                len = (short) strlen(sname[inx2]);
                ferr = BFILE_OPEN_(sname[inx2], len, &sfilenum[inx2],
                                   0, 0, 0,
                                   0, 0, 0, 0);
                TEST_CHK_FEOK(ferr);
            }
            if (trace)
                trace_printf("cli: re-close, inx=%d\n", inx);
            for (inx2 = 0; inx2 < max; inx2++) {
                ferr = BFILE_CLOSE_(sfilenum[inx2]);
                TEST_CHK_FEOK(ferr);
            }

            if (trace)
                trace_printf("cli: newproc-forkexec, inx=%d\n", inx);
            sargc = 2;
            sargv[0] = argv[0];
            sargv[1] = (char *) "-exec";
            if (trace)
                sargv[sargc++] = (char *) "-trace";
            sargv[sargc] = NULL;
            time_start(T_FORKEXEC);
            for (inx2 = 0; inx2 < max; inx2++) {
                pid = fork();
                assert(pid >= 0);
                if (pid == 0) {
                    // child
                    err = execv(sprog, sargv);
                    assert(err == 0);
                }
            }
            time_stop(T_FORKEXEC);
            time_elapsed(T_FORKEXEC);

            if (trace)
                trace_printf("cli: newproc-forkexec-su, inx=%d\n", inx);
            sargc = 2;
            sargv[0] = argv[0];
            sargv[1] = (char *) "-startup";
            if (trace)
                sargv[sargc++] = (char *) "-trace";
            sargv[sargc] = NULL;
            time_start(T_FORKEXEC_SU);
            for (inx2 = 0; inx2 < max; inx2++) {
                fifo_create(fifo1, fifo2);
                pid = fork();
                assert(pid >= 0);
                if (pid > 0) {
                    // parent
                    err = fifo_open(fifo1, O_RDONLY);
                    assert(err != -1);
                    ffds[0] = err;
                    err = fifo_open(fifo2, O_WRONLY);
                    assert(err != -1);
                    ffds[1] = err;
                    if (trace)
                        trace_printf("cli: reading fifo, inx=%d\n", inx2);
                    size = ::read(ffds[0], recv_buffer, 1);
                    if (trace)
                        trace_printf("cli: fifo read, size=%d\n", (int) size);
                    assert(size == 1);
                    if (trace)
                        trace_printf("cli: fifo read, inx=%d\n", inx2);
                    ::read(ffds[0], recv_buffer, 1);
                    err = fifo_close(ffds[0]);
                    assert(err == 0);
                    err = fifo_close(ffds[1]);
                    assert(err == 0);
                    fifo_destroy(fifo1, fifo1);
                } else {
                    // child
                    err = execv(sprog, sargv);
                    assert(err == 0);
                }
            }
            fifo_destroy(fifo2, fifo2);
            time_stop(T_FORKEXEC_SU);
            time_elapsed(T_FORKEXEC_SU);
        }
    } else {
        sys_msg_count = 0;
        time_start(T_TOTAL);
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 0,
                           1, 0); // sys msgs
        TEST_CHK_FEOK(ferr);
        for (inx = 0; !fin; inx++) {
            if (trace)
                trace_printf("srv: readupdate, inx=%d\n", inx);
            cc = BREADUPDATEX(filenumr,
                              recv_buffer,
                              4,
                              &count_read,
                              0);
            sys_msg = _xstatus_ne(cc);
            if (trace && sys_msg)
                trace_printf("srv: rcvd sys msg=%d\n",
                             sys_msgp->u_z_msg.z_msgnumber[0]);
            if (sys_msg) {
                sys_msg_count++;
                inx--;
            }
            lerr2 = BFILE_GETINFO_(filenumr, &lerr);
            TEST_CHK_FEIGNORE(lerr2);
            if (trace)
                trace_printf("srv: reply, inx=%d\n", inx);
            cc = BREPLYX(recv_buffer,
                         (unsigned short) 0,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
            if (sys_msg_count >= 4)
                fin = true;
        }
    }
    time_stop(T_TOTAL);
    time_elapsed(T_TOTAL);

    if (client) {
        dsec = time_sec(T_TOTAL);
        dms = dsec * 1000.0;
        printf("elapsed=%f\n", dms);
        printf("open/close/newprocess/processinfo/forkexec=%d\n", loop);
        dsec = time_sec(T_OPEN);
        dms = dsec * 1000.0;
        printf("open            : total-time=%f ms, time/loop=%f ms, ops/sec=%f\n",
               dms, dms / dloop, dloop / dsec);
        dsec = time_sec(T_CLOSE);
        dms = dsec * 1000.0;
        printf("close           : total-time=%f ms, time/loop=%f ms, ops/sec=%f\n",
               dms, dms / dloop, dloop / dsec);
        dsec = time_sec(T_PROCINFO);
        dms = dsec * 1000.0;
        printf("procinfo        : total-time=%f ms, time/loop=%f ms, ops/sec=%f\n",
               dms, dms / dloop, dloop / dsec);
        dsec = time_sec(T_PROCINFO_TYPE);
        dms = dsec * 1000.0;
        printf("procinfo-type   : total-time=%f ms, time/loop=%f ms, ops/sec=%f\n",
               dms, dms / dloop, dloop / dsec);
        dsec = time_sec(T_NEWPROC);
        dms = dsec * 1000.0;
        printf("newproc         : total-time=%f ms, time/loop=%f ms, ops/sec=%f\n",
               dms, dms / dloop, dloop / dsec);
        dsec = time_sec(T_FORKEXEC);
        dms = dsec * 1000.0;
        printf("forkexec        : total-time=%f ms, time/loop=%f ms, ops/sec=%f\n",
               dms, dms / dloop, dloop / dsec);
        dsec = time_sec(T_FORKEXEC_SU);
        dms = dsec * 1000.0;
        printf("forkexec-startup: total-time=%f ms, time/loop=%f ms, ops/sec=%f\n",
               dms, dms / dloop, dloop / dsec);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
