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

#include <stdio.h>

#include "reqworker.h"
#include "reqqueue.h"
#include "monlogging.h"
#include "monsonar.h"
#include "montrace.h"
#include "seabed/trace.h"

#include <signal.h>
#ifndef NAMESERVER_PROCESS
extern void child_death_signal_handler2 (int signal, siginfo_t *info, void *);
#endif

extern CReqQueue ReqQueue;
extern CMonStats *MonStats;

// Request worker thread ids
pthread_t CReqWorker::workerIds[CReqWorker::NUM_WORKERS];
// Request worker objects
CReqWorker * CReqWorker::worker[CReqWorker::NUM_WORKERS];

CReqWorker::CReqWorker(): shutdown_(false)
{
}

CReqWorker::~CReqWorker()
{
}

void CReqWorker::reqWorkerThread()
{
    const char method_name[] = "CReqWorker::reqWorkerThread";
    TRACE_ENTRY;

    CRequest *request = NULL;

    if (trace_settings & TRACE_REQUEST)
        trace_printf("%s@%d request worker starting\n", method_name, __LINE__);

    // Wait for request, process request
    for (;;)
    {
        request = ReqQueue.getRequest();

        if (request->isShutdown())
        {   // Monitor is shutting down, exit thread
            break;
        }
        else
        {
            // Record statistics (sonar counters): record start of request
            if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
                MonStats->RequestServiceTimeIncr();

            request->performRequest();

            // Record statistics (sonar counters): record request finished
            if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
                MonStats->RequestServiceTimeDecr();

            ReqQueue.finishRequest(request);
        }
    }

    // Delete the shutdown request
    delete request;

    if (trace_settings & TRACE_REQUEST)
        trace_printf("%s@%d request worker thread %lx exiting\n", method_name,
                     __LINE__, pthread_self());

    pthread_exit(0);

    TRACE_EXIT;
}

// Worker thread start routine
static void *reqWorker(void *arg)
{
    const char method_name[] = "reqWorker";
    TRACE_ENTRY;

    // Parameter passed to the thread is an instance of the CReqWorker class
    CReqWorker *rwo = (CReqWorker *) arg;

#ifndef NAMESERVER_PROCESS
    // Set sigaction such that SIGCHLD signal is caught.
    struct sigaction act;
    act.sa_sigaction = child_death_signal_handler2;
    sigemptyset(&act.sa_mask);
    sigaddset (&act.sa_mask, SIGCHLD);
    act.sa_flags = SA_SIGINFO;
    sigaction (SIGCHLD, &act, NULL);
#endif

    // Mask all allowed signals except SIGCHLD
    sigset_t              mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGCHLD);
    sigdelset(&mask, SIGPROF); // allows profiling such as google profiler
    int rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
    if (rc != 0)
    {
        if (trace_settings & TRACE_REQUEST)
            trace_printf("%s@%d pthread_sigmask error=%s (%d)\n",
                         method_name, __LINE__, strerror(rc), rc);

        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], pthread_sigmask error=%s (%d)\n", method_name,
                strerror(rc), rc);
        mon_log_write(MON_REQWORKER_1, SQ_LOG_ERR, buf);
    }

    sigset_t curSigSet;
    int rv = pthread_sigmask(SIG_SETMASK, NULL, &curSigSet);
    if (rv == 0)
    {
        if (trace_settings & TRACE_REQUEST)
            trace_printf("%s@%d request worker signal mask, SIGCHILD=%d\n",
                         method_name, __LINE__,
                         sigismember(&curSigSet, SIGCHLD));
    }

    // Enter thread processing loop
    rwo->reqWorkerThread();

    TRACE_EXIT;
    return NULL;
}

void CReqWorker::startReqWorkers()
{
    const char method_name[] = "CReqWorker::startReqWorkers";
    TRACE_ENTRY;

    for (int i=0; i< NUM_WORKERS; i++)
    {
        if (trace_settings & TRACE_INIT)
            trace_printf("%s@%d creating request worker thread %d\n",
                         method_name, __LINE__, i);

        worker[i] = new CReqWorker();

        int rc = pthread_create(&workerIds[i], NULL, reqWorker, worker[i]);
        if (rc != 0)
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], thread create error=%d\n", method_name, rc);
            mon_log_write(MON_REQWORKER_2, SQ_LOG_ERR, buf);
        }
    }

    TRACE_EXIT;
}

void CReqWorker::shutdownWork()
{
    const char method_name[] = "CReqWorker::shutdownWork";
    TRACE_ENTRY;

    // Enqueue a request that will cause request worker threads to exit
    ReqQueue.enqueueReq(CExternalReq::ShutdownWork, -1, 0, -1, NULL);

    for (int i=0; i< NUM_WORKERS; i++)
    {
        if (trace_settings & TRACE_REQUEST)
            trace_printf("%s@%d waiting for request worker thread %lx\n",
                         method_name, __LINE__, workerIds[i]);

        int rc = pthread_join(workerIds[i], NULL);
        if (rc)
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], thread join error=%d\n", method_name, rc);
            mon_log_write(MON_REQWORKER_SHUTDOWN_1, SQ_LOG_ERR, buf);
        }
        delete worker[i];
    }
    if (trace_settings & TRACE_REQUEST)
        trace_printf("%s@%d all request worker threads joined\n",
                     method_name, __LINE__);

    TRACE_EXIT;
}
