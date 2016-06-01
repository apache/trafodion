/**********************************************************************
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
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciRWCmd.cpp
 * Description:  Methods to process commands that interact with SQL/CLI and RW.
 *               
 *               
 * Created:      6/6/2003
 * Language:     C++
 * Status:       
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "SQLTypeDefs.h"
#include "SqlciRWCmd.h"
#include "SqlciError.h"
#include "sqlcmd.h"
#include "SqlciEnv.h"
#include "Formatter.h"
#include "exp_clause_derived.h"
#include "exp_datetime.h"
#include "unicode_char_set.h"
#include "RWInterface.h"

extern SqlciEnv * global_sqlci_env; // global sqlci_env for break key handling purposes.
void ReportWriterError ( ErrorValue *e )
{
#pragma nowarn(1506)  // warning elimination
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
#pragma warn(1506)  // warning elimination
}

void SetError ( ErrorValue *e, Lng32 errorCode)
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

SqlciRWCmd::SqlciRWCmd(const report_cmd_type cmd_type_)
                  : SqlciNode(SqlciNode::REPORT_CMD_TYPE),
                    cmd_type(cmd_type_)
{
};

SqlciRWCmd::~SqlciRWCmd()
{
};

///////////////////////////////////////////////////////////////////
// this class is created by sqlci parser when a LIST command
// ("LIST FIRST <N>", ...etc) is seen.
//
// The class is constructed with the list type and the number of
// rows specified in it.
//
// ListType values:
// List First:  FIRST_
// List Next:   NEXT_
// List All:    ALL_
// Cancel:      END_
//
// numRows: number of rows specified in the List command.
//          -1, if 'list all' is specified.
//
//////////////////////////////////////////////////////////////////
SqlciRWListCmd::SqlciRWListCmd(ListType type, Lng32 listCount, NABoolean listCountSet)
     : SqlciRWCmd(LIST_TYPE),
       type_(type), listCount_(listCount),
       listCountSet_(listCountSet)
{
}

SqlciRWListCmd::~SqlciRWListCmd()
{
}

short SqlciRWListCmd::process(SqlciEnv * sqlci_env)
{
  short rc = 0;

  SqlciRWInterfaceExecutor::ExecutionState state = SqlciRWInterfaceExecutor::INITIAL_;
  switch (type_)
    {
    case FIRST_:
      state = SqlciRWInterfaceExecutor::LIST_FIRST_;
      if (sqlci_env->getListCount() != SqlciEnv::MAX_LISTCOUNT
          && !listCountSet_)
#pragma nowarn(1506)   // warning elimination 
        listCount_ = sqlci_env->getListCount();
#pragma warn(1506)  // warning elimination 
      else if (listCountSet_ == FALSE)
        listCount_ = 0;
      break;

    case NEXT_:
      state = SqlciRWInterfaceExecutor::LIST_NEXT_;
      if (sqlci_env->getListCount() != SqlciEnv::MAX_LISTCOUNT
        && !listCountSet_)
#pragma nowarn(1506)   // warning elimination 
        listCount_ = sqlci_env->getListCount();
#pragma warn(1506)  // warning elimination 
      else if (listCountSet_ == FALSE)
        listCount_ = 0;

      if (sqlci_env->sqlciRWEnv()->rwExe()->resetListFlag())
	{
	  // display warning indicating that reset was done.
	  ErrorValue err;
	  err.errorCode_ = SQLCI_RW_RESET_LIST;
          err.charparam1_= NULL;
          err.charparam2_= NULL;
          err.charparam3_= NULL;
          err.intparam1_= -1;
          err.intparam2_= -1;
          err.intparam3_= -1;
	  ReportWriterError(&err);

	  // and reset the list to start at the beginning.
	  state = SqlciRWInterfaceExecutor::LIST_FIRST_;
	}
      break;

    case ALL_:
      state = SqlciRWInterfaceExecutor::LIST_ALL_;
      sqlci_env->sqlciRWEnv()->setSelectInProgress(FALSE); 
      break;

    }

  sqlci_env->sqlciRWEnv()->rwExe()->setResetListFlag(FALSE);

  sqlci_env->sqlciRWEnv()->rwExe()->setState(state);
  
  sqlci_env->sqlciRWEnv()->rwExe()->setListCount(listCount_);

  rc = sqlci_env->sqlciRWEnv()->rwExe()->process(sqlci_env);

  return rc;
}

//////////////////////////////////////////////////////////////////////////////
//
// This class is constructed by the parser when a CANCEL is seen.
// This cancels any select in progress command, then gets out of RW
// prompt and into mxci prompt.
//
//////////////////////////////////////////////////////////////////////////////
SqlciRWCancelCmd::SqlciRWCancelCmd()
     : SqlciRWCmd(CANCEL_TYPE)
{
}

SqlciRWCancelCmd::~SqlciRWCancelCmd()
{
}

short SqlciRWCancelCmd::process(SqlciEnv * sqlci_env)
{
  short rc = 0;

  sqlci_env->sqlciRWEnv()->rwExe()->setState(SqlciRWInterfaceExecutor::CANCEL_);
  
  rc = sqlci_env->sqlciRWEnv()->rwExe()->process(sqlci_env);
  sqlci_env->sqlciRWEnv()->setSelectInProgress(FALSE); 
  return rc;
}


//////////////////////////////////////////////////////////////////////////////
//
// This class is constructed by the parser when a report writer query is seen.
// These are the queries which are to be passed to RW without any
// interpretation by sqlci.
//
//////////////////////////////////////////////////////////////////////////////
SqlciRWQueryCmd::SqlciRWQueryCmd(char * rwCmd, Lng32 rwCmdLen)
     : SqlciRWCmd(RW_QUERY_TYPE)
{
  rwCmdLen_ = rwCmdLen;
  if (rwCmd)
    {
      rwCmd_ = new char[strlen(rwCmd)+1];
      strcpy(rwCmd_, rwCmd);
    }
  else
    rwCmd_ = 0;
}

SqlciRWQueryCmd::~SqlciRWQueryCmd()
{
  if (rwCmd_)
    delete rwCmd_;
}

short SqlciRWQueryCmd::process(SqlciEnv * sqlci_env)
{
  short rc = 0;

  sqlci_env->sqlciRWEnv()->rwExe()->setRWCmd(rwCmd_, rwCmdLen_);

  sqlci_env->sqlciRWEnv()->rwExe()->setState(SqlciRWInterfaceExecutor::SEND_QUERY_TO_RW_);

  rc = sqlci_env->sqlciRWEnv()->rwExe()->process(sqlci_env);

  return rc;
}

////////////////////////////////////////////////////////////////////
//
// This class is constructed by parser when a RW Select command is
// seen. This is the select query that follows a 'set list_count'
// command.
//
////////////////////////////////////////////////////////////////////
SqlciRWSelectCmd::SqlciRWSelectCmd(char * selectCmd)
     : SqlciRWCmd(SELECT_TYPE)
{
  if (selectCmd)
    {
      selectCmd_ = new char[strlen(selectCmd)+1];
      strcpy(selectCmd_, selectCmd);
    }
  else
    selectCmd_ = 0;
}

SqlciRWSelectCmd::~SqlciRWSelectCmd()
{
  if (selectCmd_)
    delete selectCmd_;
}

short SqlciRWSelectCmd::process(SqlciEnv * sqlci_env)
{
  short rc = 0;

  sqlci_env->sqlciRWEnv()->rwExe()->setSelectCmd(selectCmd_);

  sqlci_env->sqlciRWEnv()->rwExe()->setState(SqlciRWInterfaceExecutor::PREPARE_);

  sqlci_env->sqlciRWEnv()->rwExe()->setResetListFlag(FALSE);

  rc = sqlci_env->sqlciRWEnv()->rwExe()->process(sqlci_env);
  return rc;
}

////////////////////////////////////////////////////////////////////
//
// This class is constructed by parser when a RW execute command is
// seen. This is the execute of a prepared select query that follows 
// a 'set list_count' command.
//
////////////////////////////////////////////////////////////////////
SqlciRWExecuteCmd::SqlciRWExecuteCmd(char * stmtName, char * usingParamStr)
     : SqlciRWCmd(EXECUTE_TYPE)
{
  if (stmtName)
    {
      stmtName_ = new char[strlen(stmtName)+1];
      strcpy(stmtName_, stmtName);
    }
  else
    stmtName_ = 0;

  if (usingParamStr)
    {
      usingParamStr_ = new char[strlen(usingParamStr) + 1];
      strcpy(usingParamStr_, usingParamStr);
    }
  else
    {
      usingParamStr_ = NULL;
    }
}

SqlciRWExecuteCmd::~SqlciRWExecuteCmd()
{
  if (stmtName_)
    delete stmtName_;

  if (usingParamStr_)
    delete usingParamStr_;
}

short SqlciRWInterfaceExecutor::setUsingParamInfo(char * usingParamStr)
{
  short rc = 0;

  numUsingParams_ = 0;
  if (usingParamStr)
    {
      if (! usingParams_)
	usingParams_ = new char * [MAX_NUM_UNNAMED_PARAMS];

      for (Int32 i = numUsingParams_; i > 0; )
	delete usingParams_[--i];

      rc = (short)Execute::storeParams(usingParamStr, numUsingParams_,
				       usingParams_);
    }

  return rc;
}

short SqlciRWExecuteCmd::process(SqlciEnv * sqlci_env)
{
  short rc = 0;

  PrepStmt * prepStmt = sqlci_env->get_prep_stmts()->get(stmtName_);
  if (! prepStmt)
    {
      sqlci_env->diagsArea() << DgSqlCode(-SQLCI_STMT_NOT_FOUND)
			     << DgString0(stmtName_);

      return 0;
    }

  sqlci_env->sqlciRWEnv()->rwExe()->setExecutePrepStmt(prepStmt);
  sqlci_env->sqlciRWEnv()->rwExe()->setExecuteCmd(stmtName_);

  if (usingParamStr_)
    {
      rc = 
	sqlci_env->sqlciRWEnv()->rwExe()->setUsingParamInfo(usingParamStr_);
      if (rc)
	return 0;
    }

  sqlci_env->sqlciRWEnv()->rwExe()->setState(SqlciRWInterfaceExecutor::SELECT_STARTED_);

  sqlci_env->sqlciRWEnv()->rwExe()->setResetListFlag(FALSE);

  rc = sqlci_env->sqlciRWEnv()->rwExe()->process(sqlci_env);
  sqlci_env->sqlciRWEnv()->setSelectInProgress(TRUE);

  return rc;
}

////////////////////////////////////////////////////////////////////////
//
// This class is the execution engine to process the RW select command.
// It contains all the methods and other information that is needed to
// interact with RW (mxci calling RW) when a select command is processed.
//
// It is constructed during SqlciRWSelectCmd::process and is 'alive'
// as long as that 'select is in progress'. The globals RWEnv points to this
// class.
//
////////////////////////////////////////////////////////////////////////

SqlciRWInterfaceExecutor::SqlciRWInterfaceExecutor()
     : selectCmd_(NULL),
       executeStmtName_(NULL),
       usingParams_(NULL), numUsingParams_(0),
       rwCmd_(NULL), rwCmdLen_(0),
       inputRow_(NULL), inputRowLen_(0),
       outputRow_(NULL), outputRowLen_(0),
       listCount_(0), currCount_(0),
       cursorOpened_(FALSE),
       prepStmt_(NULL),
       flags_(0),
       state_(INITIAL_)
{
}

void SqlciRWInterfaceExecutor::setSelectCmd(char * selectCmd)
{
  if (selectCmd_)
    delete selectCmd_;
  selectCmd_ = NULL;

  if (executeStmtName_)
    delete executeStmtName_;
  executeStmtName_ = NULL;

  if (selectCmd)
    {
      selectCmd_ = new char[strlen(selectCmd)+1];
      strcpy(selectCmd_, selectCmd);
    }
  else
    selectCmd_ = 0;
  
  state_ = INITIAL_;
}

void SqlciRWInterfaceExecutor::setExecuteCmd(char * executeStmtName)
{
  if (selectCmd_)
    delete selectCmd_;
  selectCmd_ = NULL;

  if (executeStmtName_)
    delete executeStmtName_;

  if (executeStmtName)
    {
      executeStmtName_ = new char[strlen(executeStmtName)+1];
      strcpy(executeStmtName_, executeStmtName);
    }
  else
    executeStmtName_ = 0;
  
  state_ = INITIAL_;
}

void SqlciRWInterfaceExecutor::setRWCmd(char * rwCmd, Lng32 rwCmdLen)
{
  if (rwCmd_)
    delete rwCmd_;

  rwCmdLen_ = rwCmdLen;
  
  if (rwCmd)
    {
      rwCmd_ = new char[strlen(rwCmd)+1];
      strcpy(rwCmd_, rwCmd);
    }
  else
    rwCmd_ = 0;
}

SqlciRWInterfaceExecutor::~SqlciRWInterfaceExecutor()
{
  if (selectCmd_)
    delete selectCmd_;

  if (executeStmtName_)
    delete executeStmtName_;
}

//////////////////////////////////////////////////////////////////////////
// Returns the next state of the state machine based on the retcode
// and the nextState.
// 'retcode' is of type RetStatus in RWInterface.h.
//
// If retcode is ERROR, then next state of error is returned.
// If the nextState sent in is different than the current state, state_,
// then it overrides the retcode state and is returned.
// Otherwise, the state based on retcode is returned.
//
//////////////////////////////////////////////////////////////////////////
SqlciRWInterfaceExecutor::ExecutionState SqlciRWInterfaceExecutor::getNextState(
     Lng32 retcode,
     SqlciRWInterfaceExecutor::ExecutionState nextState)
{
  ExecutionState state;

  if (retcode == ERR)
    state = RW_ERROR_;
  else if (state_ != nextState)
    state = nextState;
  else
    {
      switch (retcode)
	{
	case SEND_QUERY:
	  state = GET_INPUT_FROM_SQLCI_;
	  break;
	  
	case SEND_INPUT_ROW:
	  state = FETCH_;
	  break;
	  
	case GET_OUTPUT_ROW:
	  state = GET_OUTPUT_ROW_FROM_RW_;
	  break;
	  
	case DONE:
	  state = CLOSE_;
	  break;

	case RESET_LIST:
	  state = RESET_LIST_;
	  break;

	default:
	  state = state_;
	  break;
	  
	}
    }

  return state;
}

Lng32 SqlciRWInterfaceExecutor::printOutputRow(SqlciEnv * sqlci_env,
					      char * outputBuf,
					      Lng32 outputBufLen)
{
  Logfile *log = sqlci_env->get_logfile();

  log->WriteAll(outputBuf, outputBufLen, 0);

  return 0;
}

static NABoolean isSqlError(Lng32 sqlRetcode)
{
  if (sqlRetcode < 0)
    return TRUE;
  else
    return FALSE;
}

short SqlciRWInterfaceExecutor::open(SqlciEnv * sqlci_env)
{
  short rc;

#ifdef _DEBUG
      if (getenv("SHOW_RW_STATE"))
	{
	  char buf[40];
	  strcpy(buf, "OPEN_");
	  cout << "State " << buf << endl;
	}
#endif

  // close cursor. TBD: add SqlCmd::close() call to do this.
  // Ignore error, stmt may already be closed.
  SQL_EXEC_CloseStmt(prepStmt_->getStmt());
  SQL_EXEC_ClearDiagnostics(prepStmt_->getStmt());
  
  // open the stmt
  rc = SqlCmd::doExec(sqlci_env,
		      prepStmt_->getStmt(),
		      prepStmt_,
		      numUsingParams_,
		      usingParams_);

  currCount_ = 0;

  cursorOpened_ = TRUE;

  return rc;
}

void SqlciRWInterfaceExecutor::Close(SqlciEnv *sqlci_env)
{
  if (prepStmt_)
    SQL_EXEC_CloseStmt(prepStmt_->getStmt());

  SQL_EXEC_ClearDiagnostics(NULL);
	
  // Deallocate, if selectCmd. This call also
  // deallocates the passed prepStmt_.
  if ((selectCmd_) &&(prepStmt_))
  {
    SqlCmd::deallocate(sqlci_env, prepStmt_);
    prepStmt_ = NULL;
    // make sure that this is reset so we dont deallocate
    // it again in interrupt handler if the break key is hit.
    global_sqlci_env->resetDeallocateStmt(); 
  }

  cursorOpened_ = FALSE;

}

short SqlciRWInterfaceExecutor::process(SqlciEnv * sqlci_env)
{
  short retcode = 0;
  Lng32 sqlRetcode = 0;
  Lng32 rwRetcode = 0;

  NABoolean done = FALSE;

// 64-bit: no more report writer
  assert(0);

  return retcode;
}

/////////////////////////////////////////////////////////////////////
// These methods are called by RW to extract information about
// an input row.
/////////////////////////////////////////////////////////////////////
Lng32 MXCI_RW_getMaxColumns (void *sqlciEnv, Lng32 &max, ErrorValue* &e)
{
  SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;
  
  PrepStmt * prepStmt = sqlci_env->sqlciRWEnv()->rwExe()->prepStmt();
  if (! prepStmt)
  {
    // populate 'e', stmt doesn't exist. TBD.
    if (e == NULL)
    {
      e = new ErrorValue;
    }
    SetError(e, -SQLCI_STMT_NOT_FOUND);
    ReportWriterError(e);
    return ERR;
  }

  max = prepStmt->numOutputEntries();

  return SUCCESS;
}

//////////////////////////////////////////////////////////////////////
Lng32 MXCI_RW_getColInfo (void *sqlciEnv, Lng32 column, 
			 AttributeDetails* &entry, ErrorValue* &e)
{
  SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;

  PrepStmt * prepStmt = sqlci_env->sqlciRWEnv()->rwExe()->prepStmt();

  if (! prepStmt)
  {
    if (e == NULL)
    {
      e = new ErrorValue;
    }
    SetError(e, -SQLCI_STMT_NOT_FOUND);
    ReportWriterError(e);
    return ERR;
  }
   

  if ((column <= 0) ||
      (column > prepStmt->numOutputEntries()))
  {
    if (e == NULL)
    {
      e = new ErrorValue;
    }
    SetError(e, -SQLCI_WRONG_COLUMN_VALUE);
    ReportWriterError(e);
    return ERR;
    
  }

  if (entry == NULL)
  {
      // allocate entry here. 
      // How will this get deallocated? TBD.
      entry = new AttributeDetails;
  }

  PrepEntry * outputEntry = prepStmt->outputEntries()[column-1];

  entry->dataType_ = outputEntry->datatype();
  entry->length_ = outputEntry->length();
  entry->precision_ = outputEntry->precision();
  entry->scale_ = outputEntry->scale();
  entry->nullable_ = outputEntry->nullFlag();
  entry->headingLen_ = outputEntry->headingLen();
  entry->heading_ = outputEntry->heading();
  entry->output_ = outputEntry->outputName();
  entry->outputLen_ = outputEntry->outputNameLen();
  entry->tableName_ = outputEntry->tableName();
  entry->tableLen_ = outputEntry->tableLen();

  
  entry->displayLen_ = Formatter::display_length(entry->dataType_,
						 entry->length_,
						 entry->precision_,
						 entry->scale_,
                                                 outputEntry->charsetEnum(),
                                                 0,
                                                 (SqlciEnv *) sqlciEnv,
                                                 NULL);

  if (entry->dataType_ == REC_DATETIME)
    {
      entry->dateTimeCode_ = outputEntry->precision();
      entry->precision_ = 0; // not used for datetime
    }
  else if (entry->dataType_ >= REC_MIN_INTERVAL && 
	   entry->dataType_ <= REC_MAX_INTERVAL) 
    {
      entry->leadPrecision_ = outputEntry->precision();
    }
  else if (DFS2REC::isAnyCharacter(entry->dataType_))
    {
      entry->charSet_ = outputEntry->charset();
	  
      entry->collation_ = 0; //TBD
    }

  return SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
#pragma nowarn(770)  // warning elimination
Lng32 MXCI_RW_getColAddr (void *sqlciEnv, Lng32 column, char *input_row, 
                          Lng32 len, char* &ptr, ErrorValue* &e)
{
  Lng32 retcode = 0;

  SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;

  PrepStmt * prepStmt = sqlci_env->sqlciRWEnv()->rwExe()->prepStmt();
  if (! prepStmt)
    // populate 'e', stmt doesn't exist. TBD.
  {
    if (e == NULL)
    {
      e = new ErrorValue;
    }
    SetError(e, -SQLCI_STMT_NOT_FOUND);
    ReportWriterError(e);
    return ERR;
    
  }

  if ((column <= 0) ||
      (column > prepStmt->numOutputEntries()))
    // populate 'e', incorrect column value. TBD.
  {
    if (e == NULL)
    {
      e = new ErrorValue;
    }
    SetError(e, -SQLCI_WRONG_COLUMN_VALUE);
    ReportWriterError(e);
    return ERR;
  }

  PrepEntry * outputEntry = prepStmt->outputEntries()[column-1];
  SQLDESC_ID * output_desc = prepStmt->getOutputDesc();

  // the address of the data row.
  // Should be the same as the 'input_row' address passed in.
  char * outputData = prepStmt->outputData();
  if (outputData != input_row)
    return ERR;

  Long data_addr = 0;
  Long ind_data_addr = 0;
  
  retcode = SQL_EXEC_GetDescItem(output_desc, column,
				 SQLDESC_VAR_PTR,
				 &data_addr, 0, 0, 0, 0);
 
  retcode = SQL_EXEC_GetDescItem(output_desc, column,
				 SQLDESC_IND_PTR,
				 &ind_data_addr, 0, 0, 0, 0);
  if (outputEntry->nullFlag())
    data_addr = ind_data_addr;

  ptr = (char*)data_addr;

  return SUCCESS;
}
#pragma warn(770)  // warning elimination

//////////////////////////////////////////////////////////////////////////
// This method formats the input datetime value to the specified datetime
// format.
// The input value is in external (string) format.
//////////////////////////////////////////////////////////////////////////
#pragma nowarn(770)  // warning elimination
Lng32 MXCI_RW_convertToDateFormat(void * sqlciEnv, 
				 char* srcPtr, AttributeDetails* srcEntry,
				 char *tgtPtr, AttributeDetails* tgtEntry, 
				 DateTimeFormat format, ErrorValue* &e)
{
  Lng32 rc;

  Int32 dtFormat;
  switch (format)
    {
    case DEFAULT:
      dtFormat = ExpDatetime::DATETIME_FORMAT_DEFAULT; break;
    case USA:
      dtFormat = ExpDatetime::DATETIME_FORMAT_USA; break;
    case EUROPEAN:
      dtFormat = ExpDatetime::DATETIME_FORMAT_EUROPEAN; break;
    default:
      assert(0);
      return ERR;
    }

  rc =  ExpDatetime::convAsciiDatetimeToASCII(srcPtr,
					      srcEntry->dateTimeCode_,
					      srcEntry->scale_,
					      srcEntry->length_,
					      tgtPtr,
					      tgtEntry->length_,
					      format,
					      NULL,
					      NULL);
  if (rc)
    {
      if (e == NULL)
	{
	  e = new ErrorValue;
	}
      SetError(e, SQLCI_CONV_RESULT_FAILED);
      ReportWriterError(e);
      return ERR;
    }
  else
    return SUCCESS;
}
#pragma warn(770)  // warning elimination

/////////////////////////////////////////////////////////////////////////////////////
Lng32 MXCI_RW_allocateHeap (void* sqlciEnv, Lng32 len, char* &ptr)
{

  SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;
  
  NAHeap *h = sqlci_env->sqlciRWEnv()->rwHeap();
  void *p = NULL;

  if (h)
    p = h->allocateMemory(len, TRUE);
 
  if (p)
  {
    ptr = (char *)p;
    return SUCCESS;
  }
  else
    return ERR;
}

///////////////////////////////////////////////////////////////////////////////////////

Lng32 MXCI_RW_deallocateHeap (void* sqlciEnv, Lng32 len, char* ptr)
{

  SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;
  
  if (len != 0)
  {
    sqlci_env->sqlciRWEnv()->rwHeap()->deallocateMemory((void*)ptr);
    return SUCCESS;
  }
  else
    return ERR;
}


////////////////////////////////////////////////////////////////////////////////////////

Lng32 MXCI_RW_unicodeConvertToUpper (void *sqlciEnv, char *input_char, Lng32 num_of_input_bytes,
     char *output_char, Lng32& num_of_output_bytes, ErrorValue* &e)
{
  //SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;
  
  Lng32 length_bytes = num_of_input_bytes;
  
  NAWchar* target = (NAWchar*)output_char;
  NAWchar* source = (NAWchar*)input_char;
  Int32 wc_len = length_bytes/2;
  Int32 actual_len = 0;
  NAWchar* tmpWCP = NULL;
  Int32 maxWideChars = num_of_output_bytes / 2;

  if ((((Long)input_char % 2) != 0) || (((Long)output_char % 2) != 0))
  {
    if (e == NULL)
    {
      e = new ErrorValue;
    }
    SetError(e, -SQLCI_RW_BUFFER_UNEVEN_BOUNDARY);
    ReportWriterError(e);
    return ERR;
  }

  // the logic in this code is from w:/exp/exp_function_upper_unicode.cpp 
  // in the eval funciton.  Whenever that code changes this has to 
  // change and vice versa.

  for (Int32 i = 0; i < wc_len ; i++)
  {
    tmpWCP = unicode_char_set::to_upper_full(source[i]);
    if (tmpWCP)
    {
      if (actual_len + ((tmpWCP[2] ==0) ? 2 : 3) > maxWideChars)
      {
        if (e == NULL)
        {
          e = new ErrorValue;
        }
        SetError(e, -SQLCI_RW_STRING_OVERFLOW);
        ReportWriterError(e);
        return ERR;
      }// end of if

      target[actual_len++] = tmpWCP[0];
      target[actual_len++] = tmpWCP[1];

      if (tmpWCP[2] != (NAWchar)0)
        target[actual_len++] = tmpWCP[2];
    }
    else
    {
      if (actual_len >= maxWideChars)
      {
        if (e == NULL)
        {
          e = new ErrorValue;
        }
        SetError(e, -SQLCI_RW_STRING_OVERFLOW);
        ReportWriterError(e);
        return ERR;
      }
      target[actual_len++] = unicode_char_set::to_upper(source[i]);
    }// end of else
  } // end of for

  num_of_output_bytes = (actual_len * 2);
  return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////

Lng32 MXCI_RW_unicodeConvertToLower(void *sqlciEnv, char *input_char, Lng32 num_of_input_bytes,
char *output_char, Lng32& num_of_output_bytes, ErrorValue* &e)
{
 // SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;

  Lng32 len1 = num_of_input_bytes;
  Lng32 buffer_size = num_of_output_bytes/2 ;
  num_of_output_bytes = len1;

  NAWchar* target = (NAWchar*)output_char;
  NAWchar* source = (NAWchar*)input_char;
  Int32 wc_len = len1/2;

  // the logic in this code is from w:/exp/exp_function_lower_unicode.cpp 
  // in the eval funciton.  Whenever that code changes this has to 
  // change and vice versa.

  for (Int32 i = 0; i < wc_len; i++)
  {
    if (i >= buffer_size)
    {
      if (e == NULL)
      {
        e = new ErrorValue;
      }
      SetError(e,-SQLCI_RW_STRING_OVERFLOW);
      ReportWriterError(e);
      return ERR;
    }
    target[i] = unicode_char_set::to_lower(source[i]);
  }
  return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Lng32 MXCI_RW_convDoIt (void *sqlciEnv, char* srcPtr, AttributeDetails* srcEntry,
  char *tgtPtr, AttributeDetails* tgtEntry, short formatting, ErrorValue* &e)
{
  Lng32 retcode = 0;
  ULng32 convFlags = 0;

  if (formatting)
    {
      // if formatting, blankpad target before doing the conversion.
      str_pad (tgtPtr,tgtEntry->displayLen_,' ');

      // the formatted target will be left blank padded (blanks to the left).
      convFlags |= CONV_LEFT_PAD;
    }
  else
    convFlags = 0;


  // SqlciEnv * sqlci_env = (SqlciEnv *)sqlciEnv;

  // Check for nullability.  The logic for this code
  // is in w:/exp/exp_conv.cpp::processNulls function

  //  Source is nullable 
  if (srcEntry->nullable_)  
    {
      //if data is null - if source data is acutally null
      if ((srcPtr[0] != '\0') || (srcPtr[1] != '\0')) 
        {
          if (formatting)
            {
	      //if Target is not nullable
	      if (!(tgtEntry->nullable_)  )
		{
		  tgtPtr[0] = '?';
		  return SUCCESS;  // Conversion has taken place.
		}
	      else 
		{
		  if (e == NULL)
		    {
		      e = new ErrorValue;
		    }
		  SetError(e, -SQLCI_RW_INVALID_FORMATTING);
		  ReportWriterError(e);
		  return ERR;
		}
            } // end of formatting
          else // if not formatting
            {
	      // source is nullable , source is null and target is nullable
	      if (tgtEntry->nullable_)
		{
		  str_pad (tgtPtr,2,'\377');
		  return SUCCESS;  // Conversion has taken place.
		}
	      else
                {
                  if (e == NULL)
		    {
		      e = new ErrorValue;
		    }
                  SetError(e, -SQLCI_CONV_NULL_TO_NOT_NULL);
                  ReportWriterError(e);
                  return ERR;
                }
            }// end of else of if not formatting
          
        }// end of source data is null
      
      // source is nullable but the source data is not null
      else 
        {
          srcPtr = (char*)srcPtr + 2; // skip the null indicator bytes
	  
        } // end of else if source is nullable but source data is not null
      
      //if source is not null and target is nullable
      // set null indicator in target if needed.
      
      if (tgtEntry->nullable_  )
	{
	  str_pad (tgtPtr,2,'\0');    // indicate that tgt is not null. Move the ptr by 2 bytes.
	  tgtPtr = (char*)tgtPtr + 2;
	}
      
    } // end of source is nullable

  char * VCLen = NULL;
  short VCLenSize = 0;
  if (DFS2REC::isAnyVarChar(tgtEntry->dataType_))
    {
      VCLen = tgtPtr;
      VCLenSize = 2;
      tgtPtr = (char*)tgtPtr + VCLenSize;
    }
  
  
  // If the datatype is of time INTERVAL or of type DATETIME, then
  // Set the datatype to _SQLDT_ASCII_F (char)
  // Set the length to the display length.
  // Set the precision to 0.
  // Set the scale to 0.
  
  Lng32 length_;
  Lng32 dataType_;
  Lng32 precision_;
  Lng32 scale_;
  
  if ( srcEntry->dataType_ == _SQLDT_DATETIME  || 
       ( srcEntry->dataType_ >= _SQLDT_INT_Y_Y && srcEntry->dataType_ <= _SQLDT_INT_D_F)
       )
    {
      length_ = srcEntry->displayLen_ ;
      dataType_ = 0;
      precision_ = 0;
      scale_ = 0;
    }
  else
    {
      length_ = srcEntry->length_;
      dataType_ = srcEntry->dataType_;
      precision_ = srcEntry->precision_;
      scale_ = srcEntry->scale_;
    }
  

#pragma warning (disable : 4244)   //warning elimination
#pragma nowarn(1506)   // warning elimination 
  retcode = convDoIt(srcPtr,
		     length_,
		     dataType_,
		     precision_,
		     scale_,
		     tgtPtr,
		     tgtEntry->length_,
		     tgtEntry->dataType_, 
		     tgtEntry->precision_,
		     tgtEntry->scale_,
		     VCLen,			//varCharLen
		     VCLenSize,		//varCharLenSize = 2
		     0,			//heap
		     0,		//diagsarea
		     CONV_UNKNOWN,
		     0,
		     convFlags);
#pragma warn(1506)  // warning elimination 
#pragma warning (default : 4244)   //warning elimination
  
  
  if (retcode)
    {
      return ERR;
    }
  else
    return SUCCESS;
  
}




