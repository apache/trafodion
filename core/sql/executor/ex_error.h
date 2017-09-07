#ifndef EX_ERROR_H
#define EX_ERROR_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_error.h
 * Description:  Executor error codes
 *
 *
 * Created:      2/16/96
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */


#include "ExpError.h"		// contains enum ExeErrorCode
#include "ComDiags.h"

class  ComCondition;
class  ex_globals;
class  ex_queue_entry;
struct ex_queue_pair;

// -----------------------------------------------------------------------
// Some convenient ways to raise errors
// -----------------------------------------------------------------------

// If request "req" has an attached diagnostics area, then it is copied,
// and the indicated condition is added to it.  If "req" does not have an
// attached diagnostics area, then an empty one is allocated in heap "heap",
// and the indicated condition added to it.  In either case, a pointer to
// the new condition is returned through parameter "cond".
ComDiagsArea *ExRaiseSqlError(CollHeap* heap, ex_queue_entry* req,
			      ExeErrorCode code, 
			      Lng32 * intParam1 = NULL,
			      char * stringParam1 = NULL,
			      ComCondition** cond=NULL);

ComDiagsArea *ExRaiseSqlWarning(CollHeap* heap, ex_queue_entry* req,
				ExeErrorCode code, 
				Lng32 * intParam1 = NULL,
				char * stringParam1 = NULL,
				ComCondition** cond=NULL);

void ExHandleArkcmpErrors(ex_queue_pair  &qparent,
			  ex_queue_entry *down_entry,
			  Lng32 matchNo,
			  ex_globals     *globals,
			  ComDiagsArea   *da,
			  ExeErrorCode    err = EXE_INTERNAL_ERROR);

void ExHandleErrors(ex_queue_pair  &qparent,
		    ex_queue_entry *down_entry,
		    Lng32 matchNo,
		    ex_globals     *globals,
		    ComDiagsArea   *da,
		    ExeErrorCode    err = EXE_INTERNAL_ERROR,
		    Lng32 * intParam1 = NULL,
		    const char * stringParam1 = NULL,
                    Lng32 * nskErr = NULL,
		    const char * stringParam2 = NULL);

#endif /* EX_ERROR_H */







