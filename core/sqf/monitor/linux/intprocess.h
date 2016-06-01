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

#ifndef INTPROCESS_H_
#define INTPROCESS_H_

#include <map>
#include <semaphore.h>

#include "lock.h"

//
// Internal-process manager
//
// singleton
//
class CIntProcess
{
private:
    typedef void (*Exit_Cb)(void *ctx, int pid, int status);
    typedef struct Ctx
    {
        CIntProcess *intprocess;
        Exit_Cb      exit_cb;
        int          exit_cb_pid;
        void        *exit_cb_ctx;
    } Ctx;

    typedef std::map<pid_t, Ctx *>   Map_Type;
    typedef Map_Type::const_iterator Map_Iter_Type;

    // map pid->ctx
    class CMap
    {
    public:
        CMap();
        virtual ~CMap();

        Map_Iter_Type end();
        void          erase(int pid);
        Map_Iter_Type find(pid_t pid);
        void          insert(int pid, Ctx *ctx);
        void          lock();
        void          print_map();
        void          unlock();

    private:
        CLock    Lock;
        Map_Type Map;
    };

public:
    CIntProcess();
    virtual ~CIntProcess();

    int          create(const char  *filename,       // filename to exec
                        char *const  argv[],         // arguments
                        Exit_Cb      exit_cb,        // callback
                        int          exit_cb_pid,    // callback pid
                        void        *exit_cb_ctx,    // callback context
                        pid_t       *pid);           // returned pid of exec'd process
    static Ctx  *get_ctx(pid_t pid);
    static bool  handle_signal(pid_t pid, int status);
    static void  print_map();
    static void  remove_ctx(pid_t pid);

private:
    static CMap   Map;
    static sem_t *Sem;
    static char   SemName[100];
};

#endif /*INTPROCESS_H_*/
