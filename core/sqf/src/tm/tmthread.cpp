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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/ms.h"
#include "seabed/thread.h"
#include "seabed/trace.h"
#include "tmthread.h"


//----------------------------------------------------------------------------
// CTmThread::CTmThread   
// Purpose : Constructor
//----------------------------------------------------------------------------
CTmThread::CTmThread(SB_Thread::Sthr::Function pv_fun, int64 pv_num, const char *pp_name)
   :SB_Thread::Thread(pv_fun, pp_name), iv_threadNum(pv_num)
{
   //TMTrace(2, XATM_TraceExit, ("CTmThread::CTmThread: ENTRY thread object %p(%s)\n",
   //                                  (void *) this, pp_name));
}


//----------------------------------------------------------------------------
// CTmThread::~CTmThread   
// Purpose : Destructor
//----------------------------------------------------------------------------
CTmThread::~CTmThread()
{
   //TMThreadTrace(2, XATM_TraceExit, ("CTmThread::~CTmThread : ENTRY.\n"));
}


//----------------------------------------------------------------------------
// CTmThread::lock
//----------------------------------------------------------------------------
void CTmThread::lock()
{
   //TMThreadTrace(4, XATM_TraceLock, ("CTmThread::lock(%d)\n", iv_lock_count));
   iv_mutex.lock();
}


//----------------------------------------------------------------------------
// CTmThread::unlock
//----------------------------------------------------------------------------
void CTmThread::unlock()
{
   //TMThreadTrace(4, XATM_TraceLock, ("CTmThread::unlock(%d)\n", iv_lock_count));
   iv_mutex.unlock();
}
