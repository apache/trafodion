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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fserr.h"

#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum  { MAX_CBUF = 1024 * 32 };        // 32 KB
enum  { MAX_DBUF = 1024 * 1024 };      // 1 MB

typedef struct m {
    pthread_mutex_t pth_mutex;
} Mutex;

typedef struct cv {
    Mutex           mutex;
    pthread_cond_t  pth_cond;
    bool            flag;
} CV;

typedef struct msg {
    struct msg *next;
    char       *cbuf;
    char       *dbuf;
} Msg;

typedef struct mq {
    Msg *head;
    Msg *tail;
} MsgQueue;

bool       bidir = false;
bool       bm = false;
bool       client = false;
int        csize = 0;
CV         cv_cli;
CV         cv_srv;
int        dsize = 1024;
int        loop = 10;
Msg        msg_pool;
Mutex      mutex_cli;
Mutex      mutex_pool;
Mutex      mutex_srv;
bool       nocopy = false;
MsgQueue   queue_cli;
MsgQueue   queue_srv;
MsgQueue   queue_pool;
char       recv_buffer[MAX_DBUF];
short      recv_buffer2[MAX_CBUF/2];
char       send_buffer[MAX_DBUF];
short      send_buffer2[MAX_CBUF/2];
bool       verbose = false;


void mutex_init(Mutex *mutex) {
    int                 err;
    pthread_mutexattr_t pth_attr;

    err = pthread_mutexattr_init(&pth_attr);
    assert(err == 0);
    err = pthread_mutex_init(&mutex->pth_mutex, &pth_attr);
    assert(err == 0);

}

void mutex_lock(Mutex *mutex) {
    int err;

    err = pthread_mutex_lock(&mutex->pth_mutex);
    assert(err == 0);
}

void mutex_unlock(Mutex *mutex) {
    int err;

    err = pthread_mutex_unlock(&mutex->pth_mutex);
    assert(err == 0);
}

void cv_init(CV *cv) {
    pthread_condattr_t  pth_attr;
    int                 err;

    mutex_init(&cv->mutex);

    err = pthread_condattr_init(&pth_attr);
    assert(err == 0);
    err = pthread_cond_init(&cv->pth_cond, &pth_attr);
    assert(err == 0);

    cv->flag = false;
}

void cv_signal(CV *cv) {
    int err;

    mutex_lock(&cv->mutex);
    cv->flag = true;
    err = pthread_cond_signal(&cv->pth_cond);
    assert(err == 0);
    mutex_unlock(&cv->mutex);
}

void cv_wait(CV *cv) {
    int err;

    mutex_lock(&cv->mutex);
    if (!cv->flag) {
        err = pthread_cond_wait(&cv->pth_cond, &cv->mutex.pth_mutex);
        assert(err == 0);
    }
    cv->flag = false;
    mutex_unlock(&cv->mutex);
}

void msg_init(Msg *msg) {
    msg->next = NULL;
    msg->cbuf = NULL;
    msg->dbuf = NULL;
}

void msg_queue_add(Mutex *mutex, MsgQueue *queue, Msg *msg) {
    if (mutex != NULL)
        mutex_lock(mutex);
    msg->next = NULL;
    if (queue->head == NULL)
        queue->head = msg;
    else
        queue->tail->next = msg;
    queue->tail = msg;
    if (mutex != NULL)
        mutex_unlock(mutex);
}

void msg_queue_init(MsgQueue *queue) {
    queue->head = NULL;
    queue->tail = NULL;
}

Msg *msg_queue_remove(Mutex *mutex, MsgQueue *queue) {
    Msg *msg;

    if (mutex != NULL)
        mutex_lock(mutex);
    if (queue->head == NULL)
        msg = NULL;
    else {
        msg = queue->head;
        queue->head = msg->next;
        msg->next = NULL;
        if (queue->head == NULL)
            queue->tail = NULL;
    }
    if (mutex != NULL)
        mutex_unlock(mutex);
    return msg;
}

void init() {
    mutex_init(&mutex_cli);
    mutex_init(&mutex_srv);
    mutex_init(&mutex_pool);
    cv_init(&cv_cli);
    cv_init(&cv_srv);

    msg_queue_init(&queue_pool);
    msg_queue_init(&queue_cli);
    msg_queue_init(&queue_srv);
    msg_init(&msg_pool);
    msg_queue_add(&mutex_pool, &queue_pool, &msg_pool);
}

void *thread_cli_fun(void *arg) {
    double          busy;
    int             inx;
    Msg            *msg;
    struct rusage   r_start;
    struct rusage   r_stop;
    struct timeval  t_elapsed_data;
    struct timeval  t_elapsed_total;
    struct timeval  t_start_data;
    struct timeval  t_start_total;
    struct timeval  t_stop;

    arg = arg; // touch
    util_time_timer_start(&t_start_total);
    util_time_timer_start(&t_start_data);
    util_cpu_timer_start(&r_start);
    for (inx = 0; inx < loop; inx++) {
        if (verbose)
            printf("count=%d\n", inx);
        msg = msg_queue_remove(&mutex_pool, &queue_pool);
        assert(msg != NULL);
        if (csize) {
            msg->cbuf = (char *) malloc(csize);
            memcpy(msg->cbuf, send_buffer2, csize);
        }
        if (dsize) {
            msg->dbuf = (char *) malloc(dsize);
            memcpy(msg->dbuf, send_buffer, dsize);
        }
        msg_queue_add(&mutex_srv, &queue_srv, msg);
        cv_signal(&cv_srv);
        cv_wait(&cv_cli);
        msg = msg_queue_remove(&mutex_cli, &queue_cli);
        assert(msg != NULL);
        msg_queue_add(&mutex_pool, &queue_pool, msg);
    }
    util_cpu_timer_stop(&r_stop);
    util_time_timer_stop(&t_stop);
    util_time_elapsed(&t_start_total, &t_stop, &t_elapsed_total);
    util_time_elapsed(&t_start_data, &t_stop, &t_elapsed_data);
    util_cpu_timer_busy(&r_start, &r_stop, &t_elapsed_data, &busy);

    if (!bm) {
        print_elapsed("", &t_elapsed_total);
        print_elapsed(" (data)", &t_elapsed_data);
    }
    print_rate(bm, "", bidir ? 2 * loop : loop, dsize, &t_elapsed_data, busy);
    return NULL;
}

void *thread_srv_fun(void *arg) {
    int  inx;
    Msg *msg;

    arg = arg; // touch

    for (inx = 0; inx < loop; inx++) {
        cv_wait(&cv_srv);
        msg = msg_queue_remove(&mutex_srv, &queue_srv);
        assert(msg != NULL);
        if (!nocopy) {
            if (csize)
                memcpy(recv_buffer2, msg->cbuf, csize);
            if (dsize)
                memcpy(recv_buffer, msg->dbuf, dsize);
        }
        if (csize)
            free(msg->cbuf);
        if (dsize)
            free(msg->dbuf);
        if (bidir) {
            if (csize)
                memcpy(send_buffer2, recv_buffer2, csize);
            if (dsize)
                memcpy(send_buffer, recv_buffer, dsize);
        }
        msg_queue_add(&mutex_cli, &queue_cli, msg);
        cv_signal(&cv_cli);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int        err;
    void      *result;
    pthread_t  thr_cli;
    pthread_t  thr_srv;
    TAD        zargs[] = {
      { "-bidir",     TA_Bool, TA_NOMAX,    &bidir     },
      { "-bm",        TA_Bool, TA_NOMAX,    &bm        },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-cluster",   TA_Ign,  TA_NOMAX,    NULL       },
      { "-csize",     TA_Int,  MAX_CBUF,    &csize     },
      { "-dsize",     TA_Int,  MAX_DBUF,    &dsize     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-nocopy",    TA_Bool, TA_NOMAX,    &nocopy    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-verbose",   TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    init();
    err = pthread_create(&thr_srv, NULL, thread_srv_fun, NULL);
    assert(err == 0);
    err = pthread_create(&thr_cli, NULL, thread_cli_fun, NULL);
    assert(err == 0);
    err = pthread_join(thr_cli, &result);
    assert(err == 0);
    err = pthread_join(thr_srv, &result);
    assert(err == 0);

    util_test_finish(client);
    return 0;
}
