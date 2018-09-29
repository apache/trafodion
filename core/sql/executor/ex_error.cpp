/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_error.C
 * Description:  Executor error-handling routines
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

// -----------------------------------------------------------------------

#include  "ex_stdh.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include  "ex_expr.h"
#include  "str.h"
#include  "ex_exe_stmt_globals.h"
#include  "ex_error.h"
#include  "ex_queue.h"
#include "ComDiags.h"

ComDiagsArea *ExRaiseSqlError(CollHeap* heap, ex_queue_entry* req,
			      ExeErrorCode err, 
			      Lng32 * intParam1,
			      char * stringParam1,
			      ComCondition** newCond)
{
  ComDiagsArea* da = req->getDiagsArea();
  if (da == NULL)
    da = ComDiagsArea::allocate(heap);
  else
    da = da->copy();
  
  return ExRaiseSqlError(heap, &da, (ExeErrorCode)(-err), newCond, intParam1, NULL, NULL, stringParam1);
}

ComDiagsArea *ExRaiseSqlWarning(CollHeap* heap, ex_queue_entry* req,
				ExeErrorCode code, 
				Lng32 * intParam1,
				char * stringParam1,
				ComCondition** cond)
{
  return ExRaiseSqlError(heap, req, 
			 (ExeErrorCode)(-code),
			 intParam1,
			 stringParam1,
			 cond);
}


// This is a misnomer - the point of this routine is to merge diags, if 
// not null, into the qparent's diags, and otherwise, create a new
// condition with the 2024 error and put that in the qparent's diags.
// If diagsArea is not NULL, then its error code is used;
// otherwise, err is used to handle error.
//
// Called by ExTransTcb::handleErrors(), ExControlTcb::work(), ...
//
// ## ExDDLTcb::handleErrors() does not currently call this,
// ## though it's very similar -- perhaps its logic can be brought in?
//
void ExHandleArkcmpErrors(ex_queue_pair  &qparent,
			  ex_queue_entry *down_entry,
			  Lng32 matchNo,
			  ex_globals     *globals,
			  ComDiagsArea   *diags_in,
			  ExeErrorCode    err)
{
  ex_queue_entry *up_entry = qparent.up->getTailEntry();

  up_entry->upState.parentIndex = down_entry->downState.parentIndex;
  up_entry->upState.setMatchNo(matchNo);
  up_entry->upState.status = ex_queue::Q_SQLERROR;

  ComDiagsArea *diags_my = up_entry->getDiagsArea();

  if (!diags_my)
    diags_my = ComDiagsArea::allocate(globals->getDefaultHeap());
  else
    diags_my->incrRefCount();

  if (diags_in)
    diags_my->mergeAfter(*diags_in);
  else
    {
      ComCondition *cond = diags_my->makeNewCondition();
      cond->setSQLCODE(err);
      diags_my->acceptNewCondition();
    }
  up_entry->setDiagsAreax(diags_my);

  // insert into parent
  qparent.up->insert();
}

void ExHandleErrors(ex_queue_pair  &qparent,
		    ex_queue_entry *down_entry,
		    Lng32 matchNo,
		    ex_globals     *globals,
		    ComDiagsArea   *diags_in,
		    ExeErrorCode    err,
		    Lng32 * intParam1,
		    const char * stringParam1,
                    Lng32 * nskErr,
		    const char * stringParam2)
{
  ex_queue_entry *up_entry = qparent.up->getTailEntry();

  up_entry->upState.parentIndex = down_entry->downState.parentIndex;
  up_entry->upState.setMatchNo(matchNo);
  up_entry->upState.status = ex_queue::Q_SQLERROR;

  ComDiagsArea *diags_my = up_entry->getDiagsArea();

  if (!diags_my)
    diags_my = ComDiagsArea::allocate(globals->getDefaultHeap());
  else
    diags_my->incrRefCount();

  if (diags_in)
  {
    diags_my->mergeAfter(*diags_in);
    if (!diags_my->contains(err))
    {
      ComCondition *cond = diags_my->makeNewCondition();
      cond->setSQLCODE(err);
      if (intParam1)
	cond->setOptionalInteger(0, *intParam1);
      if (stringParam1)
	cond->setOptionalString(0, stringParam1);
      if (nskErr)
	cond->setNskCode(*nskErr);
      if (stringParam2)
	cond->setOptionalString(1, stringParam2);

      diags_my->acceptNewCondition();
    }
  }
  else
    {
      //ExAddCondition(globals->getDefaultHeap(), &diags_my, err);
      ComCondition *cond = diags_my->makeNewCondition();
      cond->setSQLCODE(err);
      if (intParam1)
	cond->setOptionalInteger(0, *intParam1);
      if (stringParam1)
	cond->setOptionalString(0, stringParam1);
      if (nskErr)
	cond->setNskCode(*nskErr);
      if (stringParam2)
	cond->setOptionalString(1, stringParam2);

      diags_my->acceptNewCondition();

    }
  up_entry->setDiagsAreax(diags_my);

  // insert into parent
  qparent.up->insert();
}
