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
#ifndef STATICLOCKINGH
#define STATICLOCKINGH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WINDOWS
#include <Windows.h>
#else
#include <pthread.h>
#endif

#include "securityException.h"

/**
 * Create the global mutex handle.  This method is called by the ODBC drive only once
 * before establishing any connection
 */
#ifdef _WINDOWS
void __declspec( dllexport )SecurityInitialize(void) throw (SecurityException);
#else
void SecurityInitialize(void) throw (SecurityException);
#endif 

static void setupMutex(void);
void getMutex(void);
void releaseMutex(void);
void cleanupMutex(void);

#ifdef _WINDOWS
#define MUTEX_TYPE       HANDLE
#define MUTEX_SETUP(x)   (x) = CreateMutex(NULL, FALSE, NULL)
#define MUTEX_CLEANUP(x) CloseHandle(x)
#define MUTEX_LOCK(x)    WaitForSingleObject((x), INFINITE)
#define MUTEX_UNLOCK(x)  ReleaseMutex(x)
#define THREAD_ID        GetCurrentThreadId()

#else //Unix - POSIX_THREADS if pthreads are used 
#define MUTEX_TYPE       pthread_mutex_t 
#define MUTEX_SETUP(x)   pthread_mutex_init(&(x), NULL) 
#define MUTEX_CLEANUP(x) pthread_mutex_destroy(x)
#define MUTEX_LOCK(x)    pthread_mutex_lock(x)
#define MUTEX_UNLOCK(x)  pthread_mutex_unlock(x)
#define THREAD_ID        pthread_self()

// If posix threads are not used, need to define mutex operations
// appropriate for other supported platforms

#endif

#endif

