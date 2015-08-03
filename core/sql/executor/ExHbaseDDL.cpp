// **********************************************************************
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
// **********************************************************************
	
#include "Platform.h"

#include  "cli_stdh.h"
#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExHbaseAccess.h"
#include "ex_exe_stmt_globals.h"
#include "SQLTypeDefs.h"
#include "ExpHbaseInterface.h"
#include "ExExeUtilCli.h"
#include "ComDiags.h"
#include "ExSqlComp.h"

ExHbaseAccessDDLTcb::ExHbaseAccessDDLTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb, 
          ex_globals * glob ) :
  ExHbaseAccessTcb(hbaseAccessTdb, glob)
  , step_(NOT_STARTED)
{
}

  short ExHbaseAccessDDLTcb::createHbaseTable(HbaseStr &table,
					    const char * cf1, const char * cf2, const char * cf3)
{
  return 0;
}

short ExHbaseAccessDDLTcb::dropMDtable(const char * name)
{
  return 0;
}

ExWorkProcRetcode ExHbaseAccessDDLTcb::work()
{
  short retcode = 0;
  short rc = 0;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  while (1)
    {
      switch (step_)
	{
	case NOT_STARTED:
	  {
	    matches_ = 0;
	    if (hbaseAccessTdb().accessType_ == ComTdbHbaseAccess::CREATE_)
	      step_ = CREATE_TABLE;
	   else if (hbaseAccessTdb().accessType_ == ComTdbHbaseAccess::DROP_)
	      step_ = DROP_TABLE;
	   else
	     step_ = HANDLE_ERROR;
	  }
	  break;

	case CREATE_TABLE:
	  {
	    table_.val = hbaseAccessTdb().getTableName();
	    table_.len = strlen(hbaseAccessTdb().getTableName());

	    Queue * cfnl = hbaseAccessTdb().getColFamNameList();
	    cfnl->position();
	    HBASE_NAMELIST colFamList;
	    HbaseStr colFam;
	    while(NOT cfnl->atEnd())
	      {
		char * cfName = (char*)cfnl->getCurr();
		colFam.val = cfName;
		colFam.len = strlen(cfName);

		colFamList.insert(colFam);
		cfnl->advance();
	      }

	    step_ = DONE;
	  }
	  break;

	case DROP_TABLE:
	  {
	    step_ = DONE;
	  }
	  break;

	case HANDLE_ERROR:
	  {
	    if (handleError(rc))
	      return rc;
	    
	    step_ = DONE;
	  }
	  break;

	case DONE:
	  {
	    if (handleDone(rc))
	      return rc;

	    step_ = NOT_STARTED;

	    return WORK_OK;
	  }
	  break;

	}// switch

    } // while
}

ExHbaseAccessInitMDTcb::ExHbaseAccessInitMDTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb, 
          ex_globals * glob ) :
  ExHbaseAccessDDLTcb(hbaseAccessTdb, glob)
  , step_(NOT_STARTED)
{
}

ExWorkProcRetcode ExHbaseAccessInitMDTcb::work()
{
  short retcode = 0;
  short rc = 0;
  Lng32 cliRC = 0;

  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ComDiagsArea *da = exeGlob->getDiagsArea();
  
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
 
  while (1)
    {
      switch (step_)
	{
	case NOT_STARTED:
	  {
	    if (! masterGlob)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    matches_ = 0;
	   if (hbaseAccessTdb().accessType_ == ComTdbHbaseAccess::INIT_MD_)
	      step_ = INIT_MD;
	   else if (hbaseAccessTdb().accessType_ == ComTdbHbaseAccess::DROP_MD_)
	      step_ = DROP_MD;
	   else
	     step_ = HANDLE_ERROR;
	  }
	  break;

	case INIT_MD:
	  {
	    step_ = UPDATE_MD;
	  }
	  break;

	case UPDATE_MD:
	  {
	    step_ = DONE;
	  }
	  break;

	case DROP_MD:
	  {
	    step_ = DONE;
	  }
	  break;

	case HANDLE_ERROR:
	  {
	    if (handleError(rc))
	      return rc;
	    
	    step_ = DONE;
	  }
	  break;

	case DONE:
	  {
	    if (handleDone(rc))
	      return rc;

	    step_ = NOT_STARTED;

	    return WORK_OK;
	  }
	  break;

	}// switch

    } // while
}

ExHbaseAccessGetTablesTcb::ExHbaseAccessGetTablesTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb, 
          ex_globals * glob ) :
  ExHbaseAccessTcb(hbaseAccessTdb, glob)
  , step_(NOT_STARTED)
{
}

ExWorkProcRetcode ExHbaseAccessGetTablesTcb::work()
{
  Lng32 retcode = 0;
  short rc = 0;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  while (1)
    {
      switch (step_)
	{
	case NOT_STARTED:
	  {
	    matches_ = 0;
	    step_ = GET_TABLE;
	  }
	  break;

	case GET_TABLE:
	  {
	    retcode = ehi_->getTable(table_);
	    if (retcode == HBASE_ACCESS_EOD)
	      {
		step_ = CLOSE;
		break;
	      }

	    if (setupError(retcode, "ExpHbaseInterface::getTable"))
	      step_ = HANDLE_ERROR;
	    else
	      step_ = RETURN_ROW;
	  }
	  break;

	case RETURN_ROW:
	  {
	    char * ptr = table_.val;
	    short len = (short)table_.len;
	    if (moveRowToUpQueue(ptr, len, &rc, TRUE))
	      {
		return rc;
	      }
	    
	    step_ = GET_TABLE;
	  }
	  break;

	case CLOSE:
	  {
	    step_ = DONE;
	  }
	  break;
	
	case HANDLE_ERROR:
	  {
	    if (handleError(rc))
	      return rc;

	    step_ = DONE;
	  }
	  break;

	case DONE:
	  {
	    if (handleDone(rc))
	      return rc;

	    step_ = NOT_STARTED;

	    return WORK_OK;
	  }
	  break;

	}// switch

    } // while
}


