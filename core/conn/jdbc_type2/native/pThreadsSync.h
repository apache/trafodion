/**************************************************************************

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

**************************************************************************/
//
// MODULE: pThreadsSync.h
//
//
#ifndef _PTHREADSYNC_DEFINED
#define _PTHREADSYNC_DEFINED

#include <thread_safe_extended.h>
//#include <spthread.h>

#include <csrvrstmt.h>


extern int registerPseudoFileIO(short fileNum);

extern int initStmtForNowait(_TSLX_cond_t *cond, _TSLX_mutex_t *mutex);

extern int WaitForCompletion(SRVR_STMT_HDL *pSrvrStmt, _TSLX_cond_t *cond, _TSLX_mutex_t *mutex);



extern int mutexCondDestroy(_TSLX_cond_t *cond, _TSLX_mutex_t *mutex);

extern short abortTransaction (void);

extern short beginTransaction (long *transTag);

extern short resumeTransaction (long transTag);

extern short endTransaction (void);

//Added for R3.0 Transaction issue sol. 10-100430-9906
extern short tmfInit(void);

#endif
