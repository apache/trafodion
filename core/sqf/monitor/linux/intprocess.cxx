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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mpi.h"

#include "intprocess.h"
#include "monlogging.h"
#include "montrace.h"

CIntProcess::CMap  CIntProcess::Map;
sem_t             *CIntProcess::Sem = NULL;
char               CIntProcess::SemName[100];

CIntProcess::CIntProcess()
{
}

CIntProcess::~CIntProcess()
{
    if (Sem != NULL)
    {
        sem_close(Sem);
        sem_unlink(SemName);
    }
}

// called on signal
bool CIntProcess::handle_signal(pid_t pid, int status)
{
    const char method_name[] = "CIntProcess::handle_signal";
    TRACE_ENTRY;

    status = status; // touch
    Ctx *ctx = CIntProcess::get_ctx(pid);
    if (ctx != NULL)
    {
        if (trace_settings & (TRACE_SIG_HANDLER))
            trace_nolock_printf("%s@%d - internal-process exit-callback cb=%p, cb-ctx=%p, cb-pid=%d, pid=%d.\n",
                         method_name, __LINE__, ctx->exit_cb, ctx->exit_cb_ctx, ctx->exit_cb_pid, pid);
        ctx->exit_cb(ctx->exit_cb_ctx, ctx->exit_cb_pid, status);
        CIntProcess::remove_ctx(pid);
    }

    TRACE_EXIT;
    return (ctx != NULL);
}

int CIntProcess::create(const char    *filename,
                        char *const    argv[],
                        Exit_Cb        exit_cb,
                        int            exit_cb_pid,
                        void          *exit_cb_ctx,
                        pid_t         *pid)
{
    int   err;
    pid_t lpid;
    int   rc = MPI_SUCCESS;
    int   sem_rc;

    const char method_name[] = "CIntProcess::create";
    TRACE_ENTRY;

    if (Sem == NULL)
    {
        // named semaphore is easier than using shared memory
        sprintf(SemName, "/monitor.sem3.%s", getenv("USER"));
        Sem = sem_open(SemName, O_CREAT, 0644, 0);
        if (Sem == SEM_FAILED)
        {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[CIntProcess::create], Can't open semaphore %s!\n",
                    SemName);
            mon_log_write(MON_INTPROCESS_1, SQ_LOG_ERR, la_buf);
            rc = MPI_ERR_SPAWN;
            Sem = NULL;
        }
    }

    if (rc == MPI_SUCCESS)
    {
        lpid = fork();
        if (pid != NULL)
            *pid = lpid;
        if (lpid == -1)
        {
            rc = MPI_ERR_SPAWN;
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[CIntProcess::create], Can't fork errno=%d(%s)!\n",
                    errno, strerror(errno));
            mon_log_write(MON_INTPROCESS_2, SQ_LOG_ERR, la_buf);
        }
        else if (lpid > 0)
        {
            // parent
            Ctx *ctx = new Ctx();
            ctx->intprocess = this;
            ctx->exit_cb = exit_cb;
            ctx->exit_cb_pid = exit_cb_pid;
            ctx->exit_cb_ctx = exit_cb_ctx;
            Map.lock();
            Map.insert(lpid, ctx);
            Map.unlock();
            if (trace_settings & (TRACE_PROCESS))
                trace_printf("%s@%d - Monitor created internal-process pid=%d.\n",
                             method_name, __LINE__, lpid);
            if (sem_post(Sem) == -1)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[CIntProcess::create], Parent can't post semaphore, errno=%d(%s)!\n",
                        errno, strerror(errno));
                mon_log_write(MON_INTPROCESS_3, SQ_LOG_ERR, la_buf);
            }
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - Monitor completed waiting for child to exec.\n",
                             method_name, __LINE__);
        }
        else
        {
            // child
            while (((sem_rc = sem_wait(Sem)) == -1) && (errno == EINTR))
                continue;  // Restart sem_wait if interrupted by sig handler
            if (sem_rc == -1)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[CIntProcess::create], Child can't wait semaphore, errno=%d(%s)!\n",
                        errno, strerror(errno));
                mon_log_write(MON_INTPROCESS_4, SQ_LOG_ERR, la_buf);
                abort(); // kill child - if can't get sem
            }
            err = execvp(filename, argv);
            if (err == -1)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[CIntProcess::create], Child can't exec filename=%s, errno=%d(%s)!\n",
                        filename, errno, strerror(errno));
                mon_log_write(MON_INTPROCESS_5, SQ_LOG_ERR, la_buf);
            }
            abort(); // only gets here if execvp fails
        }
    }

    TRACE_EXIT;
    return rc;
}

CIntProcess::Ctx *CIntProcess::get_ctx(pid_t pid)
{
    Map_Iter_Type iter = Map.find(pid);
    if (iter == Map.end())
        return NULL;
    else
        return iter->second;
}

void CIntProcess::print_map()
{
    Map.print_map();
}

void CIntProcess::remove_ctx(pid_t pid)
{
    Map.lock();
    Ctx *ctx = get_ctx(pid);
    if (ctx != NULL)
        delete ctx;
    Map.erase(pid);
    Map.unlock();
}

CIntProcess::CMap::CMap()
{
}

CIntProcess::CMap::~CMap()
{
}

CIntProcess::Map_Iter_Type CIntProcess::CMap::end()
{
    return Map.end();
}

void CIntProcess::CMap::erase(pid_t pid)
{
    Map.erase(pid);
}

CIntProcess::Map_Iter_Type CIntProcess::CMap::find(pid_t pid)
{
    return Map.find(pid);
}

void CIntProcess::CMap::insert(pid_t pid, Ctx *ctx)
{
    Map.insert(std::make_pair(pid, ctx));
}

void CIntProcess::CMap::lock()
{
    Lock.lock();
}

void CIntProcess::CMap::print_map()
{
    Map_Iter_Type iter;

    for (iter = Map.begin(); iter != Map.end(); iter++)
    {
        printf("iter.pid=%d, .ctx=%p\n", iter->first, iter->second);
    }
}

void CIntProcess::CMap::unlock()
{
    Lock.unlock();
}
