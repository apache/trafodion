/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciCSCmd.cpp
 * Description:  Methods to process commands that interact with SQL/CLI and RW.
 *               
 *               
 * Created:      11/17/2003
 * Language:     C++
 * Status:       
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

#include "Platform.h"
#include "SQLTypeDefs.h"
#include "SqlciCSCmd.h"
#include "SqlciError.h"
#include "SqlciEnv.h"
#include "CSInterface.h"


void MXCSError ( CSErrorValue *e )
{
  //Note: when C++ compiler bug is fixed, use constructor/destructor to
  //manage memory allocation/deallocation automatically

  ErrorParam *ep1 = new ErrorParam (e->charparam1_);
  ErrorParam *ep2 = new ErrorParam (e->charparam2_);
  ErrorParam *ep3 = new ErrorParam (e->charparam3_);
  ErrorParam *ep4 = new ErrorParam (e->intparam1_);
  ErrorParam *ep5 = new ErrorParam (e->intparam2_);
  ErrorParam *ep6 = new ErrorParam (e->intparam3_);

  SqlciError2 (e->errorCode_,
       ep1,
       ep2,
       ep3,
       ep4,
       ep5,
       ep6,
        (ErrorParam *) 0);

  delete ep1;
  delete ep2;
  delete ep3;
  delete ep4;
  delete ep5;
  delete ep6;
}

void SetMXCSError ( CSErrorValue *e, Lng32 errorCode)
{
    e->errorCode_ = errorCode;
    e->charparam1_= NULL;
    e->charparam2_= NULL;
    e->charparam3_= NULL;
    e->intparam1_= -1;
    e->intparam2_= -1;
    e->intparam3_= -1;
}

///////////////////////////////////////////////////////////////////////
//
// This is the base class for RW commands.
// This class is not constructed by the parser.
//
///////////////////////////////////////////////////////////////////////

SqlciCSCmd::SqlciCSCmd(const cs_cmd_type cmd_type_)
                  : SqlciNode(SqlciNode::MXCS_CMD_TYPE),
                    cmd_type(cmd_type_)
{
};

SqlciCSCmd::~SqlciCSCmd()
{
};


//////////////////////////////////////////////////////////////////////////////
//
// This class is constructed by the parser when a MACL query is seen.
// These are the queries which are to be passed to MACL without any
// interpretation by mxci.
//
//////////////////////////////////////////////////////////////////////////////
SqlciCSQueryCmd::SqlciCSQueryCmd(char * csCmd, Lng32 csCmdLen)
     : SqlciCSCmd(CS_QUERY_TYPE)
{
  csCmdLen_ = csCmdLen;
  if (csCmd)
    {
      csCmd_ = new char[strlen(csCmd)+1];
      strcpy(csCmd_, csCmd);
    }
  else
    csCmd_ = 0;
}

SqlciCSQueryCmd::~SqlciCSQueryCmd()
{
  if (csCmd_)
    delete csCmd_;
}

short SqlciCSQueryCmd::process(SqlciEnv * sqlci_env)
{
  short rc = 0;

  sqlci_env->sqlciCSEnv()->csExe()->setCSCmd(csCmd_, csCmdLen_);

  sqlci_env->sqlciCSEnv()->csExe()->setState(SqlciCSInterfaceExecutor::SEND_QUERY_TO_CS_);

  rc = sqlci_env->sqlciCSEnv()->csExe()->process(sqlci_env);

  return rc;
}



////////////////////////////////////////////////////////////////////////
//
// This class is the execution engine to process the MXCS commands.
// It contains all the methods and other information that is needed to
// interact with MACL (mxci calling MACL).
//
////////////////////////////////////////////////////////////////////////

SqlciCSInterfaceExecutor::SqlciCSInterfaceExecutor()
     : csCmd_(NULL), csCmdLen_(0),
       outputRow_(NULL), outputRowLen_(0),
       state_(INITIAL_)
{
}


void SqlciCSInterfaceExecutor::setCSCmd(char * csCmd, Lng32 csCmdLen)
{
  if (csCmd_)
    delete csCmd_;

  csCmdLen_ = csCmdLen;
  
  if (csCmd)
    {
      csCmd_ = new char[strlen(csCmd)+1];
      strcpy(csCmd_, csCmd);
    }
  else
    csCmd_ = 0;
}

SqlciCSInterfaceExecutor::~SqlciCSInterfaceExecutor()
{
}

//////////////////////////////////////////////////////////////////////////
// Returns the next state of the state machine based on the retcode
// and the nextState.
// 'retcode' is of type RetStatus in CSInterface.h.
//
// If retcode is ERROR, then next state of error is returned.
// If the nextState sent in is different than the current state, state_,
// then it overrides the retcode state and is returned.
// Otherwise, the state based on retcode is returned.
//
//////////////////////////////////////////////////////////////////////////
SqlciCSInterfaceExecutor::ExecutionState SqlciCSInterfaceExecutor::getNextState(
     Lng32 retcode,
     SqlciCSInterfaceExecutor::ExecutionState nextState)
{
  ExecutionState state;

  if (retcode == CSGET_ERROR_INFO)
    state = CS_ERROR_;
  else if (state_ != nextState)
    state = nextState;
  else
    {
      switch (retcode)
	{
	  
	case CSGET_OUTPUT_ROW:
	  state = GET_OUTPUT_ROW_FROM_CS_;
	  break;
	  
	case CSDONE:
	  state = GET_INPUT_FROM_MXCI_;
	  break;

	default:
	  state = state_;
	  break;
	  
	}
    }

  return state;
}

Lng32 SqlciCSInterfaceExecutor::printOutputRow(SqlciEnv * sqlci_env,
					      char * outputBuf,
					      Lng32 outputBufLen)
{
  Logfile *log = sqlci_env->get_logfile();

  log->WriteAll(outputBuf, outputBufLen, 0);

  return 0;
}




short SqlciCSInterfaceExecutor::process(SqlciEnv * sqlci_env)
{
  short retcode = 0;
  Lng32 csRetcode = 0;

  ExecutionState nextState;

  NABoolean done = FALSE;

  while (NOT done)
  {
#ifdef _DEBUG
      if (getenv("SHOW_CS_STATE"))
	{
	  char buf[40];

	  switch (state_)
	    {
	    case INITIAL_: strcpy(buf, "INITIAL_"); break;

	    case GET_INPUT_FROM_MXCI_: strcpy(buf, "GET_INPUT_FROM_MXCI_");break;
	    case SEND_QUERY_TO_CS_: strcpy(buf, "SEND_QUERY_TO_CS_"); break;
	    case GET_OUTPUT_ROW_FROM_CS_: strcpy(buf, "GET_OUTPUT_ROW_FROM_CS_"); break;
	    case PRINT_OUTPUT_: strcpy(buf, "PRINT_OUTPUT_"); break;
	    case CS_ERROR_: strcpy(buf, "CS_ERROR_"); break;
	    case EXIT_: strcpy(buf, "EXIT_"); break;
	    default: strcpy(buf, "Unknown state!"); break;
	    }
	  cout << "State " << buf << endl;
	}
#endif


      switch (state_)
      {
	case INITIAL_:
	  {
	  }
	break;

	case GET_INPUT_FROM_MXCI_:
	  {
	    state_ = getNextState(0, EXIT_);
	  }
	break;

	case SEND_QUERY_TO_CS_:
	  {
	    csRetcode = CS_MXCI_sendQuery(sqlci_env->sqlciCSEnv()->csEnv(),
					  csCmd_, csCmdLen_);
	    state_ = getNextState(csRetcode, state_);
	  }
	break;

	case GET_OUTPUT_ROW_FROM_CS_:
	  {
	    csRetcode = CS_MXCI_getReportLine(sqlci_env->sqlciCSEnv()->csEnv(),
					      outputRow_, outputRowLen_);

	    if (csRetcode != CSERROR && csRetcode != CSDONE && csRetcode != CSGET_ERROR_INFO)
	      {
		printOutputRow(sqlci_env, outputRow_, outputRowLen_);
	      }

	    state_ = getNextState(csRetcode, state_);
	  }
	break;

	case CS_ERROR_:
	  {
	    CSErrorValue *err;

	    csRetcode = CS_MXCI_getErrorInfo(sqlci_env->sqlciCSEnv()->csEnv(),
					    err);
            if (csRetcode != CSDONE)
	      MXCSError(err);

	    if (csRetcode != CSGET_ERROR_INFO)
	      nextState = GET_INPUT_FROM_MXCI_;
	    else
	      nextState = state_;
	    state_ = getNextState(csRetcode, nextState);
	  }
	break;

	case EXIT_:
	  {
	    done = TRUE;
	  }
	break;

	default:
	  {
	    assert(0);
	  }
	break;

	} // switch
 } // while

  return retcode;
}

