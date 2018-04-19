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
**********************************************************************/

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExExeUtilExplain.cpp
 * Description:  
 *               
 *               
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComCextdecs.h"
#include  "cli_stdh.h"
#include  "ex_stdh.h"
#include  "sql_id.h"
#include  "ex_transaction.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include  "ComSqlId.h"

#include  "ExExeUtil.h"
#include  "ex_exe_stmt_globals.h"
#include  "exp_expr.h"
#include  "exp_clause_derived.h"
#include  "ComRtUtils.h"
#include  "ExStats.h"

#include "SqlParserGlobalsEnum.h"

//////////////////////////////////////////////////////////
// classes defined in this file:
//
// class ExExeUtilDisplayExplain
// class ExExeUtilDisplayExplainComplex
//
//////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilDisplayExplainTdb::build(ex_globals * glob)
{
  ExExeUtilDisplayExplainTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilDisplayExplainTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilDisplayExplainTcb
///////////////////////////////////////////////////////////////
ExExeUtilDisplayExplainTcb::ExExeUtilDisplayExplainTcb(
     const ComTdbExeUtilDisplayExplain & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob),
      module_(NULL),
       stmt_(NULL),
       sql_src_(NULL),
       input_desc_(NULL),
       output_desc_(NULL),
       outputBuf_(NULL),
       explainQuery_(NULL),
       cntLines_(0),
       nextLine_(0),
       header_(0),
       lastFrag_(0)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  MLINE = exe_util_tdb.colDescSize_ / 50 + 14;
  MLEN = exe_util_tdb.outputRowSize_;
  MWIDE = MLEN - 1;

  lines_ = new(getMyHeap()) char*[MLINE];
  for (Lng32 i = 0; i < MLINE; i++)
    {
      lines_[i] = new(getMyHeap()) char[MLEN];
    }
  optFOutput = new(getMyHeap()) char[MLEN];
}

ExExeUtilDisplayExplainTcb::~ExExeUtilDisplayExplainTcb()
{
  if (lines_)
    {
      for (Lng32 i = 0; i < MLINE; i++)
	{
	  NADELETEBASIC(lines_[i], getMyHeap());
	}

      NADELETEBASIC(lines_, getMyHeap());
    }

  if (optFOutput)
    NADELETEBASIC(optFOutput, getMyHeap());
  if (explainQuery_)
    {
      NADELETEBASIC(explainQuery_, getMyHeap());
      explainQuery_ = NULL;
    }
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilDisplayExplainTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilDisplayExplainPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

static const QueryString displayExplainQuery[] =
{
  {" select translate(case when module_name is null then cast(_ucs2'?' as char(60) character set ucs2 not null) "},
  {" else cast(module_name as char(60) character set ucs2 not null) end || _ucs2'  ' || "},
  {" cast(statement_name as char(60) character set ucs2 not null) || _ucs2'  ' || "},
  {" cast(plan_id as char(20) character set ucs2 ) || _ucs2'  ' || "},
  {" case when seq_num is NULL then cast(_ucs2'?' as char(11) character set ucs2) "},
  {" else cast(seq_num as char(11) character set ucs2 not null) end || _ucs2'  ' || "},
  {" cast(operator as char(30) character set ucs2 not null) || _ucs2'  ' || "},
  {" case when left_child_seq_num is NULL then cast(_ucs2'?' as char(18) character set ucs2 not null) "},
  {" else cast(left_child_seq_num as char(18) character set ucs2 not null) end || _ucs2'  ' || "},
  {" case when right_child_seq_num is NULL then cast(_ucs2'?' as char(19) character set ucs2 not null) "},
  {" else cast(right_child_seq_num as char(19) character set ucs2 not null) end || _ucs2'  ' || "},
  {" translate(tname using utf8toucs2) || _ucs2'  ' || "},
  {" cast(cardinality as char(15) character set ucs2 not null) || _ucs2'  ' || "},
  {" cast(operator_cost as char(15) character set ucs2 not null ) || _ucs2'  ' || "},
  {" translate(cast(total_cost as char(15)) using iso88591toucs2) || _ucs2'  ' || "},
  {" translate(cast(detail_cost as char(200) not null) using iso88591toucs2) || _ucs2'  ' || "},
  {" translate(description using utf8toucs2) using ucs2toutf8) "},
  {" from table(explain(%s, %s));"}
};

static const QueryString displayExplainOptionEQuery[] =
{
	{" select * from table(explain(%s, %s)) "},
	{" order by SEQ_NUM descending "},
	{" ; "}
};

short processExplainRowsWrapper(ExExeUtilDisplayExplainTcb * i)
{
  return i->processExplainRows();
}

short ExExeUtilDisplayExplainTcb::processExplainRows()
{
  return 0;
}

//////////////////////////////////////////////////////
// work() for ExExeUtilDisplayExplainTcb
//////////////////////////////////////////////////////
short ExExeUtilDisplayExplainTcb::work()
{
  Lng32 retcode;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilDisplayExplainPrivateState & pstate =
    *((ExExeUtilDisplayExplainPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  while (1) // exit via return
    {
      switch (pstate.step_)
	{
	case EMPTY_:
	  {
	    if (exeUtilTdb().getStmtName())
              {
                pstate.step_ = SETUP_EXPLAIN_;
              }
            else
              pstate.step_ = PREPARE_;
          }
          break;
          
        case PREPARE_:
	  {
	    // if generate_explain is to OFF, issue CQD to turn it on.
	    // first, hold the current default value for generate_explain.
	    if (holdAndSetCQD("generate_explain", "ON") < 0)
	      {
		pstate.step_ = HANDLE_ERROR_;
		break;
	      }

	    // tell mxcmp that this prepare is for explain.
	    retcode = 
	      cliInterface()->
	      executeImmediate("control session 'EXPLAIN' 'ON';");
	    if (retcode < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		pstate.step_ = HANDLE_ERROR_;
		break;
	      }
	    
	    cliInterface()->clearGlobalDiags();
	    retcode = 
	      cliInterface()->allocStuff(module_, stmt_, sql_src_, 
					 input_desc_, output_desc_,
					 "__EXPL_STMT_NAME__");
	    if (retcode < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		pstate.step_ = HANDLE_ERROR_;
		break;
	      }
	   
	    char * stmtStr = exeUtilTdb().getQuery();
	    retcode = cliInterface()->prepare(stmtStr, 
					      module_,
					      stmt_, sql_src_,
					      input_desc_, output_desc_,
					      NULL /* outputBuf */,
					      NULL /* outputVarPtrList */,
					      NULL /* inputBuf */,
					      NULL /* inputVarPtrList */,
					      NULL /* uniqueStmtId */,
					      NULL /* uniqueStmtIdLen */,
					      NULL /* query_cost_info */, 
					      NULL /* comp_stats_info */,
					      FALSE /* monitorThis */,
					      TRUE /* doNotCachePlan */);
	    
	    if (retcode < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		pstate.step_ = HANDLE_ERROR_;
		break;
	      }

	    pstate.step_ = SETUP_EXPLAIN_;
	  }
	break;

	case SETUP_EXPLAIN_:
	  {
	    // set sqlparserflags to disable stats for explain query
	    // DISABLE_RUNTIME_STATS 0x80000
	    masterGlob->getStatement()->getContext()->setSqlParserFlags(DISABLE_RUNTIME_STATS);
	    
	    /*set the flag here, use it later*/
	    if (exeUtilTdb().isOptionF() > 0)
	      optFlag_ = F_;
	    else if (exeUtilTdb().isOptionE() > 0)
	      optFlag_ = E_;
	    else if (exeUtilTdb().isOptionM() > 0)
	      optFlag_ = M_;
	    else
	      optFlag_ = N_;
	    
	    Int32 explain_qry_array_size = 0;
	    const QueryString * explainQuery = NULL;
	    
	    if (optFlag_ == M_)
	      {
		explain_qry_array_size = sizeof(displayExplainQuery) 
		  / sizeof(QueryString);
		
		explainQuery = displayExplainQuery;
	      }
	    else //same query used for both E_ and N_ and F_
	      {
		explain_qry_array_size = sizeof(displayExplainOptionEQuery) 
		  / sizeof(QueryString);
		
		explainQuery = displayExplainOptionEQuery;
	      }
	    
	    char * gluedQuery;
	    Lng32 gluedQuerySize;
	    glueQueryFragments(explain_qry_array_size, explainQuery,
			       gluedQuery, gluedQuerySize);
	    
	    char explArg1[200];
	    char explArg2[200];
	    if  (exeUtilTdb().getModuleName())
	      {
		strcpy(explArg1, "_iso88591'");
		strcat(explArg1, exeUtilTdb().getModuleName());
		strcat(explArg1, "'");
	      }
	    else
	      strcpy(explArg1, "NULL");
	    
	    strcpy(explArg2, "_iso88591'");
            if (exeUtilTdb().getStmtName())
	      strcat(explArg2, exeUtilTdb().getStmtName());
            else
	      strcat(explArg2, "__EXPL_STMT_NAME__");
	    strcat(explArg2, "'");
	    
	    if (explainQuery_)
              {
                // need to cleanup from previous execution
                NADELETEBASIC(explainQuery_, getMyHeap());
              }

            explainQuery_ = new (getMyHeap())
	      char[gluedQuerySize + strlen(explArg1) +
		  strlen(explArg2) + 10];
	    
	    
	    str_sprintf(explainQuery_, gluedQuery, explArg1, explArg2);

            NADELETEBASIC(gluedQuery, getMyHeap());
	    
	    pstate.step_ = FETCH_PROLOGUE_;
	  }
	break;
	
	case FETCH_PROLOGUE_:
	  {
	    retcode = cliInterface()->fetchRowsPrologue(explainQuery_);
	    if (retcode < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		pstate.step_ = HANDLE_ERROR_;
		break;
	      }

	    pstate.step_ = FETCH_FIRST_EXPLAIN_ROW_;
	    
	  }
	break;

	case FETCH_FIRST_EXPLAIN_ROW_:
	  {
	    if (optFlag_ == E_ || optFlag_ == N_)
	      {
		//for options E or N do nothing since not planning to output anything yet
		//the output will happen in DoHeader, DoOperator, DoSeparator
	      }
	    else
	      {
		// make sure there is enough space to move header
		if ((qparent_.up->getSize() - qparent_.up->getLength()) < 5)
		  return WORK_OK;	//come back later
	      }
	    
	    retcode = cliInterface()->fetch();
	    if (retcode < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		pstate.step_ = HANDLE_ERROR_;
		break;
	      }
	    
	    if (retcode == 100)
	      {
		// no rows found.
		// Either no explain information was available or statement
		// was not found.
		ExRaiseSqlError(getMyHeap(), &diagsArea_,
				(((exeUtilTdb().getModuleName()) ||
				  (exeUtilTdb().getStmtName())) ?
				 (ExeErrorCode)CLI_STMT_NOT_EXISTS :
				 (ExeErrorCode)EXE_NO_EXPLAIN_INFO));
				
		pstate.step_ = HANDLE_ERROR_;
		break;
	      }
	    
	    //the fetch was successful
	    
	    switch(optFlag_)
	      {
	      case E_:
	      case N_:
		{
		  /*for options E or N do nothing, header gets output in DoSeparator()*/
		  pstate.step_ = GET_COLUMNS_;
		}
	      break;
	      
	      case F_:
		{
		  moveRowToUpQueue(" ");
		  moveRowToUpQueue("LC   RC   OP   OPERATOR              OPT       DESCRIPTION           CARD   ");
		  moveRowToUpQueue("---- ---- ---- --------------------  --------  --------------------  ---------");
		  
		  moveRowToUpQueue(" ");
		  pstate.step_ = GET_COLUMNS_;
		}
	      break;
	      
	      case M_:
		{
		  moveRowToUpQueue(" ");
		  moveRowToUpQueue("MODULE_NAME                                                   STATEMENT_NAME                                                PLAN_ID               SEQ_NUM      OPERATOR                        LEFT_CHILD_SEQ_NUM  RIGHT_CHILD_SEQ_NUM  TNAME                                                         CARDINALITY      OPERATOR_COST    TOTAL_COST       DETAIL_COST                                                                                                                                                                                               DESCRIPTION");
		  moveRowToUpQueue("------------------------------------------------------------  ------------------------------------------------------------  --------------------  -----------  ------------------------------  ------------------  -------------------  ------------------------------------------------------------  ---------------  ---------------  ---------------  --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------  -------------------");
		  moveRowToUpQueue(" ");
		  pstate.step_ = RETURN_EXPLAIN_ROW_;
		}
	      break;
	      
	      }
	  }
	break;
	
	case FETCH_EXPLAIN_ROW_:
	  {
	    retcode = cliInterface()->fetch();
	    if (retcode < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		pstate.step_ = HANDLE_ERROR_;
		break;
	      }
	    
	    if (retcode == 100) //no more data
	      pstate.step_ = FETCH_EPILOGUE_;
	    else if (optFlag_ == E_ || optFlag_ == N_ || optFlag_ == F_)
	      pstate.step_ = GET_COLUMNS_;
	    else
	      pstate.step_ = RETURN_EXPLAIN_ROW_;
	  }
	break;
	
	case GET_COLUMNS_:
	  {
	    short rc = GetColumns();
	    if (rc != 0)	//means we got an EXE_EXPLAIN_BAD_DATA
	      //or an error from getPtrAndLen()
	      {
		ExRaiseSqlError(getMyHeap(), &diagsArea_,
				(((exeUtilTdb().getModuleName()) ||
				  (exeUtilTdb().getStmtName())) ?
				 (ExeErrorCode)CLI_STMT_NOT_EXISTS :
				 (ExeErrorCode)EXE_NO_EXPLAIN_INFO));
		pstate.step_ = HANDLE_ERROR_;
	      }
	    else
	      {
		if (optFlag_ == F_)
		  pstate.step_ = RETURN_EXPLAIN_ROW_;
		else
		  {		
		    if (header_ == 1) // got set to 1 inside GetColumns if current node is ROOT
		      pstate.step_ = DO_HEADER_;
		    else
		      pstate.step_ = DO_OPERATOR_;
		  }
	      }
	  }
	break;

	case DO_HEADER_:
	  {
	    //one blank line before the summary
	    FormatLine(NULL, NULL, 0, 0);
	    
	    DoSeparator();
	    DoHeader();
	    
	    pstate.step_ = RETURN_FMT_ROWS_;
	  }
	break;
	
	case DO_OPERATOR_:
	  {
	    DoSeparator();
	    DoOperator();
	    header_ = 0;		//reset once root operator done
	    
	    pstate.step_ = RETURN_FMT_ROWS_;
	  }
	break;
	
	case RETURN_FMT_ROWS_:
	  {
	    short rc = OutputLines();
	    
	    if (rc != 0)		//rc is the same as would be gotten from a call to moveRowToUpQueue
	      return rc;		//next state is unchanged, we need to come back here
	    
	    if (header_ == 1)	//if current node is root, just finished header
	      pstate.step_ = DO_OPERATOR_;
	    else
	      pstate.step_ = FETCH_EXPLAIN_ROW_;
	  }
	break;
	
	case RETURN_EXPLAIN_ROW_:
	  {
	    char * ptr;
	    Lng32   len;
	    short rc;
	    
	    if (optFlag_ == F_)
	    {
		FormatForF();
		retcode = moveRowToUpQueue(&optFOutput[0], -1, &rc);
	    }
	    else //must be M_
	    {
		cliInterface()->getPtrAndLen(1, ptr, len);
		retcode = moveRowToUpQueue(ptr, len, &rc);
	    }
	    if (retcode)
		return rc;
	    
	    pstate.step_ = FETCH_EXPLAIN_ROW_;
	  }
	break;
	
	case HANDLE_ERROR_:
	  {
	    pstate.step_ = FETCH_EPILOGUE_AND_RETURN_ERROR_;
	  }
	break;
	
	case FETCH_EPILOGUE_:
	case FETCH_EPILOGUE_AND_RETURN_ERROR_:
	  {
	    retcode = cliInterface()->fetchRowsEpilogue(explainQuery_);
	    if (retcode < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
	      }
	    
	    retcode = 
	      cliInterface()->deallocStuff(module_, stmt_, sql_src_, 
					   input_desc_, output_desc_);
	    if (retcode < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
	      }

	    if (exeUtilTdb().getStmtName() == NULL)
	      {
		if (restoreCQD("generate_explain") < 0)
		  {
		    pstate.step_ = DONE_;
		    break;
		  }
	      }

	    // reset CONTROL SESSION
	    retcode = 
	      cliInterface()->
	      executeImmediate("control session reset 'EXPLAIN';");
	    if (retcode < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
	      }
	    
	    if (pstate.step_ == FETCH_EPILOGUE_AND_RETURN_ERROR_)
	      pstate.step_ = RETURN_ERROR_;
	    else
	      pstate.step_ = DONE_;
	  }
	break;

	case RETURN_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;
	    
	    pstate.step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;

	    pstate.matches_ = 0;
	    pstate.step_ = EMPTY_;

            // reset sqlparserflags that disabled stats for Explain query
            // DISABLE_RUNTIME_STATS = 0x80000
            masterGlob->getStatement()->getContext()->resetSqlParserFlags(DISABLE_RUNTIME_STATS);

	    //reset variables used for options E_ and N_
	    cntLines_ = 0;
	    nextLine_ = 0;
	    header_ = 0;
	    lastFrag_ = 0;
	    
	    return WORK_OK;
	  }
	break;

	}
    }

}


//given a null-terminated string, truncates whitespace from the
//end and moves the string termination character
void ExExeUtilDisplayExplainTcb::truncate_whitespace(char * str) const
{
  Int32 total = str_len(str);
  char * end = str + total;
  --end;
  while (total > 0 && *end == ' ')  //do not go past the beginning
    {
      --end;
      --total;
    }
  ++end;
  *end = '\0';
}


/*************************************************************
Function Name: GetColumns

Argument List: None

Return Status: 
    long = 0 on success, else a return code to use in error return to caller
         = EXE_EXPLAIN_BAD_DATA returned means that a numeric field was NULL
              or truncated when expected to be valid
         = anything else, error code from getPtrAndLen, pass up and set state
              to HANDLE_ERROR_

Class Data Accessed: (optional section, omit if not needed)
    13 "local column data" variables filled with contents of this row

Description:
This function will first get the operator (column 5) and check it for "root".
If root, it will make 12 more calls to getPtrAndLen() to get each of the other
columns returned in the current row of explain information.  Otherwise it does
only 9 more calls as the other 3 are not needed after processing the root node.
It will store these fields in private variables, checking lenghts as it does.

It will use the indicator flag returned to put default values in the empty
fields and check for errors.  Note that NULL or truncated can be indicated in
some cases without causing an error return.  The idea is to not fail the display
due to one bad parameter, if we can handle it some other way.  Examples: when we
get a truncation of a string, we output "(data loss indicated)" and continue;
also, for some columns, NULL is valid and does not indicate an error.  However,
in case we have a NULL where we expected a numeric value, or if we truncate a
number, we return EXE_EXPLAIN_BAD_DATA back to caller as we can’t handle this
easily, so we pass the fail back which will abort the display.
*****/
short ExExeUtilDisplayExplainTcb::GetColumns()
{
    Lng32   len;
    Lng32   rc;
    short *ind;
    char  *ptr;
    Lng32   opDone = 0;          // =1 after doing operator, used to reorder

    for (Int32 i = 5; i < 14; ++i) // start with operator to check for root
    {
        if (opDone && (i == 5)) // if 5 already done
        {
            continue;           // don't do it again
        }
        rc = cliInterface()->getPtrAndLen(i, ptr, len, &ind);
        if (rc != 0)
            return (short)rc;	//not sure what error is, the rc will indicate the error
        if (*ind > 0) {         // if data truncation, set for all
          ptr = (char *) "(data loss indicated)";
            len = 21;
        }
        switch(i)
        {
        case 1: //module name
            if ((*ind) < 0)     // if NULL returned
            {
                str_cpy_c(moduleName_, "DYNAMICALLY COMPILED");
            }
            else                // indicator is zero, normal data
            {
                if (len > MNAME) { len = MNAME; } // watch for junk
                str_ncpy(moduleName_, ptr, len);
                moduleName_[len] = '\0';
                truncate_whitespace(moduleName_);
            }
            break;
        case 2: //statement name
            if ((*ind) < 0)     // if NULL returned
            {
                str_cpy_c(statementName_, "NOT NAMED");
            }
            else                // indicator is zero, normal data
            {
                if (len > MNAME) { len = MNAME; } // watch for junk
                str_ncpy(statementName_, ptr, len);
                statementName_[len] = '\0';
                truncate_whitespace(statementName_);
            }
            break;
        case 3: //plan id
            if ((*ind) != 0)    //NULL or truncation indicated, means bad data
                return EXE_EXPLAIN_BAD_DATA;
            else                // indicator is zero, normal data
                planId_ = *((Int64*)ptr);
            break;
        case 4: //sequence number
            if ((*ind) != 0)    //NULL or truncation indicated, means bad data
                return EXE_EXPLAIN_BAD_DATA;
            else                // indicator is zero, normal data
                seqNum_= *((Lng32*)ptr);
            break;
        case 5: //operator name is processed first, then restart at 1 or 4
            opDone = 1;         // skip next pass here
            if ((*ind) < 0)     // if NULL returned
            {
                str_cpy_c(operName_, "(not found)"); // resolve error gracefully
            }
            else                // indicator is zero, normal data
            {
                if (len > MOPER) { len = MOPER; } // watch for junk
                str_ncpy(operName_, ptr, len);
                operName_[len] = '\0';
                truncate_whitespace(operName_);
                if (str_cmp_c(operName_, "ROOT") == 0)
                {
                    header_ = 1;
                    lastFrag_ = 0;
                    i = 0;          // do all 13 fields
                }
                else
                {
                    i = 3;          // do only 1 fields
                }
            }
            break;
        case 6: //left child
            if ((*ind) < 0)     // if NULL returned
            {
                leftChild_ = 0;
            }
            else if ((*ind) > 0)    /*value was truncated*/
            {
                return EXE_EXPLAIN_BAD_DATA;//expected a NULL or a valid value
            }
            else                // indicator is zero, normal data
            {
                leftChild_ = *((Lng32*)ptr);
            }
            break;
        case 7: //right child
            if ((*ind) < 0)     // if NULL returned
            {
                rightChild_ = 0;
            }
            else if ((*ind) > 0)    /*value was truncated*/
            {
                return EXE_EXPLAIN_BAD_DATA;//expected a NULL or a valid value
            }
            else                // indicator is zero, normal data
            {
                rightChild_ = *((Lng32*)ptr);
            }
            break;
        case 8: //table name
            if ((*ind) < 0)     // if NULL returned
            {
                *tName_ = '\0'; //this oper has no table name, this is valid
            }
            else                // indicator is zero, normal data
            {
                if (len > MNAME) { len = MNAME; } // watch for junk
                str_ncpy(tName_, ptr, len);
                tName_[len] = '\0';
                truncate_whitespace(tName_);
            }
            break;
        case 9: //cardinality
            if ((*ind) != 0)    //NULL or truncation indicated, means bad data
                return EXE_EXPLAIN_BAD_DATA;
            else                // indicator is zero, normal data
            {
              if (exeUtilTdb().isOptionC())
                cardinality_ = 100;
              else
                cardinality_  = *((float*)ptr);
            }
            break;
        case 10: //operator cost
            if ((*ind) != 0)    //NULL or truncation indicated, means bad data
                return EXE_EXPLAIN_BAD_DATA;
            else                // indicator is zero, normal data
            {
                operatorCost_ = *((float*)ptr);
            }
            break;
        case 11: //total cost
            if ((*ind) != 0)    //NULL or truncation indicated, means bad data
                return EXE_EXPLAIN_BAD_DATA;
            else                // indicator is zero, normal data
            {
                totalCost_ = *((float*)ptr);
            }
            break;
        case 12: //detail cost
            if ((*ind) < 0)     // if NULL returned
            {
                str_cpy_c(detailCost_, "(not found)");// resolve this error gracefully
            }
            else                // indicator is zero, normal data
            {
                if (len > MCOST) { len = MCOST; } // watch for junk
                str_ncpy(detailCost_, ptr, len);
                detailCost_[len] = '\0';
                truncate_whitespace(detailCost_);
            }
            break;
        case 13: //description
            if ((*ind) < 0)     // if NULL returned
            {
                str_cpy_c(description_, "(not found)");//resolve this error gracefully
            }
            else                // indicator is zero, normal data
            {
                if (len > MDESC) { len = MDESC; } // watch for junk
                str_ncpy(description_, ptr, len);
                description_[len] = '\0';
                truncate_whitespace(description_);
            }
            break;
        } // end switch
    } // end for
    return 0;
}
/*************************************************************
  Function Name: FormatForF
  
  Argument List: None
  
  Return Status: None (void)
  
  Class Data Accessed:
  13 "local column data" variables with contents of this row (read only)
  char  optFOutput[MLEN]                                    (write only)
  
  Description:
 *************************************************************/
void ExExeUtilDisplayExplainTcb::FormatForF()
{
  char * current = &optFOutput[0];
  
  if (leftChild_ == 0)
    str_cpy(current, ".   ", 4);
  else if (leftChild_ > 9999)	// we had an overflow
    str_cpy(current, "****", 4);
  else
    str_sprintf(current, "%d    ", leftChild_);
  current += 4;

  str_cpy(current, " ", 1);//space between columns
  current += 1;

  if (rightChild_ == 0)
    str_cpy(current, ".   ", 4);
  else if (rightChild_ > 9999)	// we had an overflow
    str_cpy(current, "****", 4);
  else
    str_sprintf(current, "%d    ", rightChild_);
  current += 4;

  str_cpy(current, " ", 1);//space between columns
  current += 1;

  if (seqNum_ == 0)
    str_cpy(current, ".   ", 4);
  else if (seqNum_ > 9999)	// we had an overflow
    str_cpy(current, "****", 4);
  else
    str_sprintf(current, "%d    ", seqNum_);
  current += 4;
      
  str_cpy(current, " ", 1);//space between columns
  current += 1;

  //append the operator name, first 20 chars only
  bool pad_with_blanks = false;
  for (Int32 i = 0; i < 20; ++i)
  {
    if (operName_[i] == '\0')
      pad_with_blanks = true;
    
    if (pad_with_blanks)
      *current++ = ' ';
    else
      *current++ = TOLOWER(operName_[i]);
  }
  
  str_cpy(current, "  ", 2);//space between columns
  current += 2;
  
  //add information to the optimizations column
  Int32 remaining_width = 8;
  if (str_str(description_, "olt_opt_lean: used") != 0)
    {
      str_cpy(current, "ol ", 3);
      current += 3;
      remaining_width -=3;
    }
  else if (str_str(description_, "olt_optimization: used") != 0)
    {
      str_cpy(current, "o ", 2);
      current += 2;
      remaining_width -=2;
    }
  if (str_str(description_, "fast_scan: used") != 0)
    {
      str_cpy(current, "fs ", 3);
      current += 3;
      remaining_width -=3;
    }
  if (str_str(description_, "fast_replydata_move: used") != 0)
    {
      str_cpy(current, "fr ", 3);
      current += 3;
      remaining_width -=3;
    }
  if (str_str(description_, "mv_rewrite: used") != 0)
    {
      str_cpy(current, "w ", 2);
      current += 2;
      remaining_width -=2;
    }
  if (str_str(description_, "join_method: unique-hash") != 0)
    {
      str_cpy(current, "u ", 2);
      current += 2;
      remaining_width -=2;
    }
  if (str_str(description_, "seamonster_exchange: yes") != 0)
    {
      str_cpy(current, "sm", 2);
      current += 2;
      remaining_width -= 2;
    }

  if (str_cmp_c(operName_, "TRAFODION_INSERT") == 0 || 
      str_cmp_c(operName_, "TRAFODION_UPSERT") == 0 || 
      str_cmp_c(operName_, "TRAFODION_VSBB_UPSERT") == 0 || 
      str_cmp_c(operName_, "TRAFODION_LOAD") == 0 || 
      str_cmp_c(operName_, "TRAFODION_DELETE") == 0 ||
      str_cmp_c(operName_, "TRAFODION_UPDATE") == 0)
    {
      if ((str_str(description_, "region_transaction: enabled") != 0) ||
          (str_str(description_, "hbase_transaction: used") != 0))
        {
          if (str_str(description_, "region_transaction: enabled") != 0)
            str_cpy(current, "r", 1);
          else if (str_str(description_, "hbase_transaction: used") != 0)
            str_cpy(current, "h", 1);
          ++current;
          --remaining_width;
        }
    } // insert, upsert, delete

  //now adjust the pointer so that the width is not exceeded
  if (remaining_width <= 0)
    current += remaining_width;
  else
    for (; remaining_width > 0; --remaining_width)
      *current++ = ' ';
  
  str_cpy(current, "  ", 2);//space between columns
  current += 2;
  
  //add information to the description column
  remaining_width = 20; //reset for this column
  if (str_cmp_c(operName_, "ROOT") == 0)
    {
      if (str_str(description_, "upd_action_on_error: return") != 0)
	str_cpy(current, "r", 1);
      else if (str_str(description_, "upd_action_on_error: xn_rollback") != 0)
	str_cpy(current, "x", 1);
      else if (str_str(description_, "upd_action_on_error: partial_upd") != 0)
	str_cpy(current, "p", 1);
      else if (str_str(description_, "upd_action_on_error: savepoint") != 0)
	str_cpy(current, "s", 1);
      else
	str_cpy(current, " ", 1);
      ++current;
      --remaining_width;

      if (str_str(description_, "dp2_xns: enabled") != 0)
      {
	str_cpy(current, "d", 1);
	++current;
	--remaining_width;
      }
    }
  else if (str_cmp_c(operName_, "INDEX_SCAN") == 0 || str_cmp_c(operName_, "INDEX_SCAN_UNIQUE") == 0 || str_cmp_c(operName_, "TRAFODION_INDEX_SCAN") == 0)
    {
      char* index_loc = str_str(description_, "index");
      char* index_loc_end = str_str(index_loc,"(");

      if (index_loc != 0)
	{
	  char * index_name = str_str(index_loc, ".");
	  if ((index_name != 0) && (index_name < index_loc_end) ) // check if there is a . in the index name or not
	    {
	      index_name += 1;//remove the catalog
	      index_name = str_str(index_name, ".");
	      if (index_name != 0)
		index_name += 1;//remove the schema
	    }
	  else
	    {
	      //if the catalog.schema. was already out, search for a blank
	      index_name = str_str(index_loc, " ");
	      if (index_name != 0)
		++index_name;
	    }
	  //at this point should either be pointing to the index name (not cat.sch)
	  // or zero if fell through else and search failed
	  if (index_name == 0)
	    {
	      //error
	      str_cpy(current, "***", 3);
	      current += 3;
	      remaining_width -= 3;
	    }
	  else
	    while (((*index_name) != '(') && (remaining_width >1)) // ensure that the index name fits within the 20 bytes allocated for this description column
	      {
		*current = *index_name;
		++current;
		++index_name;
		--remaining_width;
	      }
	}
      else
	{
	  //error, we should never fail to find index_scan
	  str_cpy(current, "***", 3);
	  current += 3;
	  remaining_width -= 3;
	}
      
      if (str_str(description_, "mdam:") != 0)
	{
          if (remaining_width <= 4)
	    {
	      // we have to fit this in in the last 4 bytes (even if it
	      // means overwriting part of the index name)
	      // we have 1 byte reserved for atleast a " " . So we need 
	      // 3 more bytes for this info. 
	     str_cpy(current+remaining_width-3, " (m)", 4); 
             remaining_width = 0;
	    }
	  else
	    {
	      str_cpy(current, " (m)", 4);
	      current += 4;
	      remaining_width -= 4;
	    }
	}
      else
	{
	  str_cpy(current, " ", 1);
	  ++current;
	  --remaining_width;
	}
    }
  else if (str_cmp_c(operName_, "SPLIT_TOP") == 0)
    {
      //insert number of parent processes
      char * par_proc = str_str(description_, "parent_processes");
      if (par_proc == 0)
	{  //error
	  *current++ = '*';
	  --remaining_width;
	}
      else if (exeUtilTdb().isOptionC())
	{
	  *current++ = '#';
	  --remaining_width;
	}
      else
	{
	  par_proc = str_str(par_proc, " ");
	  if (par_proc != 0) //there should be a space
	    {
	      ++par_proc; 
	      while ((*par_proc) != ' ')
		{
		  *current++ = *par_proc++;
		  --remaining_width;
		}
	    }
	  else{
	    //error
	    *current++ = '*';
	    --remaining_width;
	  }
	}
      //insert parent partitioning function
      char * par_func = str_str(description_, "parent_partitioning_function");
      char * child_func = str_str(description_, "child_partitioning_function");
      
      if (par_func != 0 && child_func != 0)	//both should be present for this operator type
	{
	  par_func = str_str(par_func, " ");
	  if (par_func != 0)
	    {
	      ++par_func;
	      char* rep = str_str(par_func, "replicate no broadcast");
	      if (rep != 0 && rep < child_func)
		{
		  str_cpy(current, "(rep-n)", 7);
		  current +=7;
		  remaining_width -= 7;
		}
	      else{
		rep = str_str(par_func, "broadcast");
		if (rep != 0 && rep < child_func && str_str(par_func, "broadcast skewed") == 0)
		  {
		    str_cpy(current, "(rep-b)", 7);
		    current += 7;
		    remaining_width -= 7;
		  }
		else{
		  rep = str_str(par_func, "hash1-ud");
		  if(rep != 0 && rep < child_func)
		    {
		      str_cpy(current, "(h1-ud)", 7);
		      current += 7;
		      remaining_width -= 7;
		    }
		  else{
		    rep = str_str(par_func, "hash1-br");
		    if (rep != 0 && rep < child_func)
		      {
			str_cpy(current, "(h1-br)", 7);
			current += 7;
			remaining_width -= 7;
		      }
		    else{
		      rep = str_str(par_func, "hash2-ud");
		      if (rep != 0 && rep < child_func)
			{
			  str_cpy(current, "(h2-ud)", 7);
			  current += 7;
			  remaining_width -= 7;
			}
		      else{
			rep = str_str(par_func, "hash2-br");
			if (rep != 0 && rep < child_func)
			  {
			    str_cpy(current, "(h2-br)", 7);
			    current += 7;
			    remaining_width -=7;
			  }
			else{
			  rep = str_str(par_func, "hash-ud");
			  if (rep != 0 && rep < child_func)
			    {
			      str_cpy(current, "(h0-ud)", 7);
			      current += 7;
			      remaining_width -= 7;
			    }
			  else{
			    rep = str_str(par_func, "hash-br");
			    if (rep != 0 && rep < child_func)
			      {
				str_cpy(current, "(h0-br)", 7);
				current += 7;
				remaining_width -= 7;
			      }
			    else{
			      rep = str_str(par_func, "exactly");
			      if (rep != 0 && rep < child_func)
				{
				  //do nothing, exactly one partitioning function
				}
			      else{
				//search for the name of the partitioning function
				/*
				  the layout is the following:
				  parent_partitioning_function: grouped x to y, <optional> PAPA with z PA(s), <optional> "our partitioningfunction"
				  */
				char * comma = str_str(par_func, ",");
				if (comma == 0 || comma > child_func)
				  {
				    //no commas present, we found it, do nothing
				  }
				else{//found one comma, skip current, search for next
				  
				  char * comma2 = str_str(comma + 1, ",");
				  if (comma2 != 0 && comma2 < child_func)
				    {
				      par_func = comma2 + 2; //skip ", "
				    }
				  else//either not found or found in child
				    {
				      par_func = comma + 2; //skip ", "
				    }
				}
				//by now the par_func has been readjusted, proceed to extract
				*current++ = '(';
				--remaining_width;
				for (Int32 i = 0; i < 5; ++ i)
				  {
				    *current++ = *par_func++;
				    --remaining_width;
				  }
				*current++ = ')';
				--remaining_width;
			      }//exactly one
			    }//hash-br
			  }//hash-ud
			}//hash2-br
		      }//hash2-ud
		    }//hash1-br
		  }//hash1-ud
		}//rep-b
	      }//rep-n
	    }//end of par_func != 0 after searching for a space
	  else
	    {
	      //handle incorrect provided format only for parent
	      str_cpy(current, "(***)", 5);
	      current += 5;
	      remaining_width -= 5;
	    }	    
	}
      else //error, both parent and child partitioning functions should be present
	{
	  //handle incorrect provided format only for parent
	  str_cpy(current, "(***)", 5);
	  current += 5;
	  remaining_width -= 5;
	}
      *current++ = ':';
      --remaining_width;
      
      //insert number of child processes
      char * child_proc = str_str(description_, "child_processes");
      if (child_proc == 0)
	{//error
	  *current++ = '*';
	  --remaining_width;
	}
      else if (exeUtilTdb().isOptionC())
	{
	  *current++ = '#';
	  --remaining_width;
	}
      else{
	child_proc = str_str(child_proc, " ");
	if (child_proc != 0)
	  {
	    ++child_proc; 
	    while ((*child_proc) != ' ')
	      {
		*current++ = *child_proc++;
		--remaining_width;
	      }
	  }
	else{
	  //error
	  *current++ = '*';
	  --remaining_width;
	}
      }
      
      //insert child partitioning function, searched for it above (with parent)
      if (child_func != 0)
	{
	  child_func = str_str(child_func, " ");
	  if (child_func != 0)
	    {
	      ++child_func;
	      char* rep = str_str(child_func, "replicate no broadcast");
	      if (rep != 0)
		{
		  str_cpy(current, "(rep-n)", 7);
		  current +=7;
		  remaining_width -= 7;
		}
	      else{
		rep = str_str(child_func, "broadcast");
		if (rep != 0 && str_str(child_func, "broadcast skewed") == 0)
		  {
		    str_cpy(current, "(rep-b)", 7);
		    current += 7;
		    remaining_width -= 7;
		  }
		else{
		  rep = str_str(child_func, "hash1-ud");
		  if(rep != 0)
		    {
		      str_cpy(current, "(h1-ud)", 7);
		      current += 7;
		      remaining_width -= 7;
		    }
		  else{
		    rep = str_str(child_func, "hash1-br");
		    if (rep != 0)
		      {
			str_cpy(current, "(h1-br)", 7);
			current += 7;
			remaining_width -= 7;
		      }
		    else{
		      rep = str_str(child_func, "hash2-ud");
		      if (rep != 0)
			{
			  str_cpy(current, "(h2-ud)", 7);
			  current += 7;
			  remaining_width -= 7;
			}
		      else{
			rep = str_str(child_func, "hash2-br");
			if (rep != 0)
			  {
			    str_cpy(current, "(h2-br)", 7);
			    current += 7;
			    remaining_width -=7;
			  }
			else{
			  rep = str_str(child_func, "hash-ud");
			  if (rep != 0)
			    {
			      str_cpy(current, "(h0-ud)", 7);
			      current += 7;
			      remaining_width -= 7;
			    }
			  else{
			    rep = str_str(child_func, "hash-br");
			    if (rep != 0)
			      {
				str_cpy(current, "(h0-br)", 7);
				current += 7;
				remaining_width -= 7;
			      }
			    else{
			      rep = str_str(child_func, "exactly");
			      if (rep != 0)
				{
				  //do nothing, exactly one partitioning function
				}
			      else{
				*current++ = '(';
				--remaining_width;
				for (Int32 i = 0; i < 5; ++ i)
				  {
				    *current++ = *child_func++;
				    --remaining_width;
				  }
				*current++ = ')';
				--remaining_width;
			      }
			    }//hash-br
			  }//hash-ud
			}//hash2-br
		      }//hash2-ud
		    }//hash1-br
		  }//hash1-ud
		}//rep-b
	      }//rep-n
	    }
	  else{//no space found where there should be one
	    str_cpy(current, "(***)", 5);
	    current += 5;
	    remaining_width -= 5;
	  }
	}
      else{ //error, child partitioning functions should be present
	str_cpy(current, "(***)", 5);
	current += 5;
	remaining_width -= 5;
      }
      char * merged_order = str_str(description_, "merged_order");
      if (merged_order != 0) {
        str_cpy(current, " (m)", 4);
	current += 4;
	remaining_width -= 4;
      }
      *current++ = ' ';
      --remaining_width;
    }
  else if (str_cmp_c(operName_, "ESP_EXCHANGE") == 0 )
    {
      char * par_proc = str_str(description_, "parent_processes");
      if (par_proc == 0)
	{   //error, there should be a token present
	  *current++ = '*';
	  --remaining_width;
	}
      else if (exeUtilTdb().isOptionC())
	{
	  *current++ = '#';
	  --remaining_width;
	}
      else{
	par_proc = str_str(par_proc, " ");
	if (par_proc != 0)
	  {
	    ++par_proc; 
	    while ((*par_proc) != ' ')
	      {
		*current++ = *par_proc++;
		--remaining_width;
	      }
	  }
	else{
	  //error, there should be a space
	  *current++ = '*';
	  --remaining_width;
	}
      }
      char * par_func = str_str(description_, "parent_partitioning_function");
      char * child_func = str_str(description_, "child_partitioning_function");
      if (par_func != 0)
	{
	  char* rep = str_str(par_func, "replicate no broadcast");
	  if (rep != 0 &&(child_func==0 || rep < child_func))
	    {
	      str_cpy(current, "(rep-n)", 7);
	      current +=7;
	      remaining_width -= 7;
	    }
	  else{
	    rep = str_str(par_func, "broadcast");
	    if (rep != 0 && (child_func==0 || rep < child_func) && str_str(par_func, "broadcast skewed") == 0)
	      {
		str_cpy(current, "(rep-b)", 7);
		current += 7;
		remaining_width -= 7;
	      }
	    else{
	      rep = str_str(par_func, "hash1-ud");
	      if(rep != 0 &&(child_func==0 || rep < child_func))
		{
		  str_cpy(current, "(h1-ud)", 7);
		  current += 7;
		  remaining_width -= 7;
		}
	      else{
		rep = str_str(par_func, "hash1-br");
		if (rep != 0 &&(child_func==0 || rep < child_func))
		  {
		    str_cpy(current, "(h1-br)", 7);
		    current += 7;
		    remaining_width -= 7;
		  }
		else{
		  rep = str_str(par_func, "hash2-ud");
		  if (rep != 0 &&(child_func==0 || rep < child_func))
		    {
		      str_cpy(current, "(h2-ud)", 7);
		      current += 7;
		      remaining_width -= 7;
		    }
		  else{
		    rep = str_str(par_func, "hash2-br");
		    if (rep != 0 &&(child_func==0 || rep < child_func))
		      {
			str_cpy(current, "(h2-br)", 7);
			current += 7;
			remaining_width -=7;
		      }
		    else{
		      rep = str_str(par_func, "hash-ud");
		      if (rep != 0 &&(child_func==0 || rep < child_func))
			{
			  str_cpy(current, "(h0-ud)", 7);
			  current += 7;
			  remaining_width -= 7;
			}
		      else{
			rep = str_str(par_func, "hash-br");
			if (rep != 0 &&(child_func==0 || rep < child_func))
			  {
			    str_cpy(current, "(h0-br)", 7);
			    current += 7;
			    remaining_width -= 7;
			  }
			else{
			  par_func = str_str(par_func, " ");
			  if (par_func != 0)
			    {
			      ++par_func;
			      *current++ = '(';
			      --remaining_width;
			      for (Int32 i = 0; i < 5; ++ i)
				{
				  *current++ = *par_func++;
				  --remaining_width;
				}
			      //trim whitespace from the end
			      while (*(current-1) == ' ')
				{
				  --current;
				  ++remaining_width;
				}
			      *current++ = ')';
			      --remaining_width;
			    }
			  else
			    {	//no space found where there should be one
			      str_cpy(current, "(***)", 5);
			      current += 5;
			      remaining_width -= 5;
			    }
			}//hash-br
		      }//hash-ud
		    }//hash2-br
		  }//hash2-ud
		}//hash1-br
	      }//hash1-ud
	    }//rep-b
	  }//rep-n
	}//here it is not an error not to have parent partitioning function
      *current++ = ':';
      --remaining_width;
      char * child_proc = str_str(description_, "child_processes");
      if (child_proc == 0)
	{   //error, there should be a token present
	  *current++ = '*';
	  --remaining_width;
	}
      else if (exeUtilTdb().isOptionC())
	{
	  *current++ = '#';
	  --remaining_width;
	}
      else{
	child_proc = str_str(child_proc, " ");
	if (child_proc != 0)
	  {
	    ++child_proc; 
	    while ((*child_proc) != ' ')
	      {
		*current++ = *child_proc++;
		--remaining_width;
	      }
	  }
	else{
	  //error, there should be a space
	  *current++ = '*';
	  --remaining_width;
	}
      }
      ///now take care of child partitioning function if present
      if (child_func != 0)
	{
	  char* rep = str_str(child_func, "replicate no broadcast");
	  if (rep != 0)
	    {
	      str_cpy(current, "(rep-n)", 7);
	      current +=7;
	      remaining_width -= 7;
	    }
	  else{
	    rep = str_str(child_func, "broadcast");
	    if (rep != 0 && str_str(child_func, "broadcast skewed") == 0)
	      {
		str_cpy(current, "(rep-b)", 7);
		current += 7;
		remaining_width -= 7;
	      }
	    else{
	      rep = str_str(child_func, "hash1-ud");
	      if(rep != 0)
		{
		  str_cpy(current, "(h1-ud)", 7);
	    current += 7;
	    remaining_width -= 7;
		}
	      else{
		rep = str_str(child_func, "hash1-br");
		if (rep != 0)
		  {
		    str_cpy(current, "(h1-br)", 7);
		    current += 7;
		    remaining_width -= 7;
		  }
		else{
		  rep = str_str(child_func, "hash2-ud");
		  if (rep != 0)
		    {
		      str_cpy(current, "(h2-ud)", 7);
		      current += 7;
		      remaining_width -= 7;
		    }
		  else{
		    rep = str_str(child_func, "hash2-br");
		    if (rep != 0)
		      {
			str_cpy(current, "(h2-br)", 7);
			current += 7;
			remaining_width -=7;
		      }
		    else{
		      rep = str_str(child_func, "hash-ud");
		      if (rep != 0)
			{
			  str_cpy(current, "(h0-ud)", 7);
			  current += 7;
			  remaining_width -= 7;
			}
		      else{
			rep = str_str(child_func, "hash-br");
			if (rep != 0)
			  {
			    str_cpy(current, "(h0-br)", 7);
			    current += 7;
			    remaining_width -= 7;
			  }
			else{
			  child_func = str_str(child_func, " ");
			  if (child_func != 0)
			    {
			      ++child_func;
			      *current++ = '(';
			      --remaining_width;
			      for (Int32 i = 0; i < 5; ++ i)
				{
				  *current++ = *child_func++;
				  --remaining_width;
				}
			      //trim whitespace from the end
			      while (*(current-1) == ' ')
				{
				  --current;
				  ++remaining_width;
				}
			      *current++ = ')';
			      --remaining_width;
			    }
			  else
			    {	//no space found where there should be one
			      str_cpy(current, "(***)", 5);
			      current += 5;
			      remaining_width -= 5;
			    }
			}//hash-br
		      }//hash-ud
		    }//hash2-br
		  }//hash2-ud
		}//hash1-br
	      }//hash1-ud
	    }//rep-b
	  }//rep-n
	}//here it is not an error not to have a child partitioning function

      char * merged_order = str_str(description_, "merged_order");
      if (merged_order != 0) {
        str_cpy(current, " (m)", 4);
	current += 4;
	remaining_width -= 4;
      }
      *current++ = ' ';
      --remaining_width;
    }
  else//another kind of operator
    {
      char * tname_loc = str_str(tName_, ".");
      if (tname_loc != 0)
	{
	  ++tname_loc;//remove the catalog
	  tname_loc = str_str(tname_loc, ".");
	  if (tname_loc != 0)
	    ++tname_loc;//remove the schema
	}
      else
	tname_loc = tName_;

      // If the catalog and schema combined name lengths
      // were greater than MNAME, then NULL is returned
      // from str_str.  Only set the table name if a 
      // valid address is returned, otherwise add a
      // table name of "***"
        
      if (tname_loc == NULL)
        {
          str_cpy(current, "***", 3);
	  current += 3;
          remaining_width -= 3;
        }
      else
        {
          while (tname_loc != NULL && (*tname_loc) != '\0')
	    {
	      UInt32 UCS4value;
	      Int32 firstCharLen = LocaleCharToUCS4(tname_loc, 8, &UCS4value, cnv_UTF8);
	      if ( firstCharLen <= remaining_width ) // Copy in only *whole* characters!
	      {
	         for ( Int32 ii = 0; ii < firstCharLen; ii++ )
	         {
	            *current++ = *tname_loc++;
	            --remaining_width;
	         }
	      }
	      else break;
	    }
	}
      //trim whitespace from the end
      while (*(current-1) == ' ')
	{
	  --current;
	  ++remaining_width;
	}
      //trim close paren from the end
      while (*(current-1) == ')')
	{
	  --current;
	  ++remaining_width;
	}
      
      if (str_str(description_, "mdam:") != 0)
	{
	  str_cpy(current, " (m)", 4);
	  current += 4;
	  remaining_width -= 4;
	}
      else
	{
	  str_cpy(current, " ", 1);
	  ++current;
	  --remaining_width;
	}
    }
  //now adjust the pointer so that the width is not exceeded
  if (remaining_width <= 0)
    current += remaining_width;
  else
    for (; remaining_width > 0; --remaining_width)
      *current++ = ' ';
  
  str_cpy(current, "  ", 2);//space between columns
  current += 2;
  
  //place the cardinality 
  float absval = cardinality_;    // absolute value of input
  Lng32 valInt = 0;            // integer part of float value
  Lng32 valDec = 0;            // decimal part of float value
  Lng32 valExp = 0;            // expontial part of float value
  float nbrDec = 100.0F;      // 2 decimal digits
  //float valRnd  = 0.005F;     // value to add for rounding if we did it
  //we do no rounding since this is the way things were done in older options 'f'
  
  bool psign = true;		//assume absolute value is >= 1
  
  if (absval != 0)
    {
      if (absval <= 1 )
	{
	  while (absval < 1)
	    {
	      valExp--;
	      absval *= 10;
	      psign = false;
	    }
	}
      else
	{
	  while (absval >= 10) 
	    {                  // figure out the exponent value
	      valExp++;
	      absval /= 10;
	    }
	}
    }

  //absval += valRnd; //there shall be no rounding yet
  valInt = (Lng32)absval;                  // get the integer part
  absval -= valInt;
  valDec = (Lng32)(absval*nbrDec);         // get the decimal part as long
  
  if (psign)
    str_sprintf (current, "%d.%02dE+%03d", valInt, valDec, valExp);
  else
    str_sprintf (current, "%d.%02dE%04d", valInt, valDec, valExp);
  
  optFOutput[MLEN-2] = '\0';   
  //  optFOutput[78] = '\0';   
}   

/*************************************************************
Function Name: DoHeader

Argument List: None

Return Status: None (void)

Class Data Accessed:
    13 "local column data" variables with contents of this row (read only)
    lines_[ ] [ ]       empty on input, filled with lines to output
    cntLines_           tells how many lines are in Lines array

Description:
This function will format the lines needed for the header of the plan.  These
lines will come from data in the first (root) row of the explain plan which is
now in private variables of this object.  The rows of the header are those which
are duplicated in all other rows plus a couple of other really important ones.
The SQL statement that generated the plan will be shown here as well as the shape
statement if used.  The SQL and the shape information comes from the description
column and will be logically consumed here (not displayed again).  It will use
the GetField function to find these desired tokens in the description field and
use size to temporarily null terminate them.  It has MLINE (currently 74) lines
available to storing these formatted lines.  .  It should check for the line
MLINE +1 output and then stop.  The extra output line will cause the output to
contain a data truncation message.  If outputting more than MLINE+1 lines is
attempted the additional lines are just dropped.

In R2 it will also display any non-defaulted CQDs here if they get into the root
node.
*****/
void ExExeUtilDisplayExplainTcb::DoHeader()
{
  Lng32 fullSize = 0;
  Lng32 intSize = 0;
  Lng32 valSize = (Lng32)strlen(moduleName_);
  char outStr[MUSERSP];
  char* stmt;
  
  FormatLongLine("MODULE_NAME", moduleName_, 11, valSize);
  
  // change statement name if still default
  if (str_cmp_c("__EXPL_STMT_NAME__", statementName_) == 0)
    str_cpy_c(statementName_, "NOT NAMED");
  valSize = (Lng32)strlen(statementName_);
  FormatLongLine("STATEMENT_NAME", statementName_, 14, valSize);
  
  //convert the Int64 to string
  str_sprintf(outStr, "%ld", planId_);
  FormatLine("PLAN_ID", outStr, 7, (Lng32)str_len(outStr));
  
  // rework cardinality as rows out.  Assume probe count is always 1
  // on the root node, so use ROWS_OUT token always
  FormatFloat (outStr, intSize, valSize, cardinality_,
	       (optFlag_ == N_), (optFlag_ == E_));
  FormatLine("ROWS_OUT", outStr, 8, valSize, 0, intSize);
  
  // rework totalCost as estimated total cost
  FormatFloat (outStr, intSize, valSize, totalCost_,
	       (optFlag_ == N_), (optFlag_ == E_));
  FormatLine("EST_TOTAL_COST", outStr, 14, valSize, 0, intSize);
  
  // do the statement being displayed as a plan
  if (GetField(description_, "statement: ", stmt, valSize) != 0)
    FormatLine("STATEMENT", "(not found)", 9, 11); //error: statement should exits
  else
    FormatSQL("STATEMENT", stmt, 9, valSize);
  
  // watch for a control query shape in effect
  if (GetField(description_, "must_match: ", stmt, valSize) == 0)
    FormatLongLine("MUST_MATCH", stmt, 10, valSize);
  
  //two blank lines after the header
  FormatLine(NULL, NULL, 0, 0);
  FormatLine(NULL, NULL, 0, 0);
}


/*************************************************************
Function Name: DoOperator

Argument List: None

Return Status: None (void)

Class Data Accessed:
    13 "local column data" variables with contents of this row (may be changed)
    lines_[ ] [ ]       empty on input, filled with lines to output
    cntLines_           tells how many lines are in Lines array

Description:
This function will format all useful data from the 13 columns.  This code will
skip empty columns and most empty values.  It will skip some data when optFlag_
is N_, detailed costs for example.  It will output one keyword and value per line.
If the length of the data is over 50 characters it will call FormatLongLine,
otherwise FormatLine is called.  Some keywords will be mapped to new names.
It has MLINE (currently 74) lines available for storing these formatted lines.
It will check for the line MLINE +1 output and then stop.  The extra output line
will cause the output to contain a data truncation message.  If outputting more
than MLINE+1 lines is attempted the additional lines are just dropped.

This function might have to modify some other data in the description field when
OptFlag_ is N_, but not in R1.
*****/
void ExExeUtilDisplayExplainTcb::DoOperator()
{
  Lng32 intSize = 0;
  Lng32 valSize = 0;
  char outStr[MUSERSP];
  char* keyptr;
  char* fieldptr;
  Lng32 keySize;
  Lng32 fullSize;
  Lng32 done;                      // =1 when no data left to parse
  Lng32 firstAttr = 1;             // =1 for start of description_
  Lng32 dispAttr;                  // =1 display string attribute
  Lng32 reqIn = 1;                 // default probe value
  Lng32 i;                         // iteration variable
  Lng32 errorSeen = 0;             // no errors in detailed cost yet
  // the following 4 arrays are initialized only once per object
  const char *tokExp[] =           // expected detailed cost tokens
  {
    "CPU_TIME",
    "IO_TIME",
    "MSG_TIME",
    "IDLE_TIME"
  };
  const char *tokRep[] =           // replacement detailed cost tokens
  {
    "cpu_cost",
    "io_cost",
    "msg_cost",
    "idle_cost"
  };
  Lng32 tokRepSz[] = { 8,7,8,9 };  // replacement detailed cost tokens sizes
  
  // put up to 4 fields on the first line
  FormatFirstLine();
  
  valSize = (Lng32)str_len(tName_);
  if (valSize != 0)			// if table name not null, display it
    FormatLongLine("TABLE_NAME", tName_, 10, valSize);
  
  // rework probes as requests in
  if (GetField(detailCost_, "PROBES: ", fieldptr, fullSize) == 0)
    {
      char *tmpEnd = fieldptr+fullSize;   // note end of value
      char tmp = *(tmpEnd);               // save char at cut
      *(tmpEnd) = '\0';                   // cut to make string
      FormatNumber(outStr, intSize, valSize, fieldptr);
      FormatLine("REQUESTS_IN", outStr, 11, valSize, 0, intSize);
      *(tmpEnd) = tmp;                    // restore cut
      // if probe is not 1 (two digits or first not 1) say 2
      if ((valSize > 1) || (*outStr != '1')) { reqIn = 2;}
    }
  else    //error: probes should be locatable in detailCost_, but if not
    FormatLine("REQUESTS_IN", "(not found)", 11, 11, 0);
  
  // rework cardinality as rows out
  FormatFloat (outStr, intSize, valSize, cardinality_,
	       (optFlag_ == N_), (optFlag_ == E_));
  if (reqIn == 1)                         // if number of probes is 1
    FormatLine("ROWS_OUT", outStr, 8, valSize, 0, intSize);
  else
    FormatLine("ROWS/REQUEST", outStr, 12, valSize, 0, intSize);
  
  // rework operatorCost as estimated operator cost
  FormatFloat ((char *)outStr, intSize, valSize, operatorCost_,
	       (optFlag_ == N_), (optFlag_ == E_));
  FormatLine("EST_OPER_COST", outStr, 13, valSize, 0, intSize);
  
  // rework totalCost as estimated total cost
  FormatFloat ((char *)outStr, intSize, valSize, totalCost_,
	       (optFlag_ == N_), (optFlag_ == E_));
  FormatLine("EST_TOTAL_COST", outStr, 14, valSize, 0, intSize);
  
  // work through the 4 remaining fields in detailed cost.  Assume they
  // are in correct order, but handle other cases gracefully too
  if (optFlag_ == E_)		//only print total cost details for expert users
    {
      parsePtr_ = detailCost_;// tell ParseField where to work
      done = 0;               // forget last value
      i = 0;                  // loop counter
      while (! done)          // watch for less than 4 fields
        {
	  if (ParseField(keyptr, fieldptr, keySize, fullSize, done)) // if fail
            {
	      // report the problem depending on how many we found
	      if (i == 0)
                {
		  FormatLine("(none)", NULL, 6, 0, 2);
		  i = 4;      // skip the missing fields msg
                }
	      else { FormatLine("(invalid data)", NULL, 14, 0, 2);}
	      break;          // give up processing detailed cost field
            }
	  FormatNumber(outStr, intSize, valSize, fieldptr);
	  if ( i < 4 && str_cmp_c(keyptr,tokExp[i]) == 0) // if this is the expected token
            {
	      FormatLine(tokRep[i], outStr, tokRepSz[i], valSize, 2, intSize);
            }
	  else                // just display what we got
            {
	      FormatLine(keyptr, outStr, keySize, valSize, 2, intSize);
	      errorSeen++;    // note we saw unexpected tokens
            }
	  i++;                // do next one
	  if ((i > 3) && (! errorSeen))// if all 4 normal and done just stop
            {
	      break;
            }
	  // else continue until done flag stops it
        }
      if (i < 4) { FormatLine("(missing fields)", NULL, 16, 0, 2); }
    }
  
  // work through the description field
  FormatLine("DESCRIPTION", NULL, 11, 0);
  parsePtr_ = description_;   // tell ParseField where to work
  done = 0;                   // forget last value
  while (! done)              // while there's more to parse
    {
      if (ParseField(keyptr, fieldptr, keySize, fullSize, done) != 0)
        {
	  //can have a case of empty description_ so check for that
	  if (firstAttr)
	    FormatLine("(none)", NULL, 6, 0, 2);
	  else
	    FormatLine("(invalid data)", NULL, 14, 0, 2);
	  break;
        }
      // now have an attribute to display
      dispAttr = 1;                   // assume a string to display
      if (IsNumberFmt(fieldptr) == 1) //if this is a number
        {
	  FormatNumber((char*)outStr, intSize, valSize, fieldptr);
	  FormatLine(keyptr, outStr, keySize, valSize, 2, intSize);
	  dispAttr = 0;               // display is done
        }
      else if (header_) //this is a string, watch special on root node
	if (str_cmp_c(keyptr, "statement") == 0)
	  dispAttr = 0;           // skip display, it is in header
	else if (str_cmp_c(keyptr, "must_match") == 0)
	  dispAttr = 0;           // skip display, it is in header
      if (dispAttr)                   // if needed, format and store in lines_ array
	if (fullSize > (MWIDE - COL2)) // if longer than one line
	  FormatLongLine(keyptr, fieldptr, keySize, fullSize, 2); // multi-line
	else
	  FormatLine(keyptr, fieldptr, keySize, fullSize, 2);// one line
      firstAttr = 0;                  // first attribute done
    }  // end while for description fiels
  
  if (seqNum_ != 1)                   // if not last node
    {
      //two blank lines after each operator
      FormatLine(NULL, NULL, 0, 0);
      FormatLine(NULL, NULL, 0, 0);
    }
}


/*************************************************************
Function Name: OutputLines

Argument List: None

Return Status:
    long = 0 on success, else a return code to use in error return to caller

Class Data Accessed:
    lines_[ ] [ ]       empty on input, filled with lines to output
    cntLines_           tells how many lines are in Lines array
    nextLine_           contains index of next entry in Lines array

Description:
This function will move lines from the lines_ array to the up-queue using
moveRowToUpQueue().  It will first verify that the up-queue can accept a line.
After moving a line it will adjust the globals that define where we are in the
output of the lines_ array.  If it is told the up-queue is full it may just
return WORK_OK for all state variables are set to allow it to resume when
called back.   It knows it is done when cntLines_ < nextLine_.  After moving
all lines in the lines_ array, it will reset array pointers.
*****/
short ExExeUtilDisplayExplainTcb::OutputLines()
{
  short retcode;
  short rc;
  while (nextLine_ < cntLines_)
    {
      retcode = moveRowToUpQueue(lines_[nextLine_], -1, &rc); //need to give it rc, otherwise will always return 0
      
      if (retcode == 0)
	++nextLine_;
      else 
	return rc;
    }
  //emptying the array
  cntLines_ = 0;
  nextLine_ = 0;
  return 0; //since rc cannot be positive, need to distinguish between their returns and local returns
}


/*************************************************************
Function Name: FormatLine

Argument List:
    char *key[in]           ptr to start of key word string
    char *val[in]           ptr to start of value string
    long  keySize[in]       count of characters in key word string, 0 is ok
    long  valSize[in]       count of characters in value string, 0 is ok
    long  indent[in]        count of blank characters at start of line, default 0
    long  decLoc[in]        count of char before decimal point, 0 if not float, default 0

Return Status: None

Class Data Accessed:
    lines_[out]             new print line set
    cntLines_[in/out]       count of lines in lines_ at start

Description:
This function will take the input keyword and value string and format them
into a single 79 character line in the lines_ array.  It expects the input
value to fit but will truncate as needed.  It will follow all the formatting
rules defined in the ES.  The most important is that the string values start
in col 28, COL2 is the enum.  Float values will have the decimal in col 29
and will start as far left of that as col 25, cutting keywords to 23 char
when needed.  Dots will replace spaces between key and value if there is a
need of at least 3 with a blank on either side.

This routine expects the correct number of decimal digits to be present (2 or 4).
The complex part is the alignment of the numbers.  The line count will be
adjusted to show another line in lines_.  The output lines will look like this:
    TABLE_NAME ............... SAMS.ORDERS.ORDERS
    REQUESTS_IN .............. 1
    ROWS_OUT ............... 236
    TOTAL_COST ............... 0.0731
    TOTAL_COST_DETAILS
      CPU_TIME ............... 0.0004
    DESCRIPTION
      scan_type .............. full scan of table ORDERS but this line is
                                 folded to show the 2 char indent

This function is designed to support all needs except the first line and
the node separator.  If keySize is 0 only the val point will be used and
no dots will be inserted.  If valSize is 0 only the key will be inserted
and no dots.  So both the fold line routines can use this to insert data.
If both keySize and valSize are 0 it outputs a line with one blank on it.

This routine will check for an empty line in the lines_ array and do
nothing if there is no space.  It will set a "data loss" message in the
last line if another line is attempted.
*****/

typedef struct {
  const char * key;
  const char * value;
} FilterKeyValueStruct;
const FilterKeyValueStruct filterKeyValue[] =
  {
    {"MODULE_NAME", "###"},
    {"plan_version", "###"},
    {"statement_index", "###"},
    {"PLAN_ID", "###"},
    {"ROWS_OUT", "###"},
    {"EST_OPER_COST", "###"},
    {"EST_TOTAL_COST", "###"},
    {"REQUESTS_IN", "###"},
    {"ROWS/REQUEST", "###"},
    {"OPERATOR_COST", "###"},
    {"ROLLUP_COST", "###"},
    {"xn_autoabort_interval", "###"},
    {"max_card_est", "###"},
    {"max_max_cardinality", "###"},
    {"total_overflow_size", "###"},
    {"est_memory_per_node", "###"},
    {"est_memory_per_cpu", "###"},
    {"est_memory_per_instance", "###"},
    {"buffer_size", "###"},
    {"memory_quota", "###"},
    {"memory_quota_per_esp", "###"},
    {"memory_quota_per_instance", "###"},
    {"memory_limit_per_cpu", "###"},
    {"cache_size", "###"},
    {"probes", "###"},
    {"successful_probes", "###"},
    {"unique_probes", "###"},
    {"duplicated_succ_probes", "###"},
    {"rows_accessed", "###"},
    {"affinity_value", "###"},
    {"parent_processes", "###"},
    {"child_processes", "###"},
    {"num_cache_entries", "###"},
    {"num_inner_tuples", "###"},
    {"ObjectUIDs", "###"},
    
  };

NABoolean ExExeUtilDisplayExplainTcb::filterKey(
     const char *key, Lng32 keySize, char * value, char * retVal,
     Lng32 &decLoc)
{
  if ((! key) || (keySize == 0))
    return FALSE;

  Int32 maxSize = sizeof(filterKeyValue) / sizeof(FilterKeyValueStruct);
  
  for (Int32 i = 0; i < maxSize; i++)
    {
      if (strcmp(key, filterKeyValue[i].key) == 0)
        {
          strcpy(retVal, filterKeyValue[i].value);
          decLoc = strlen(filterKeyValue[i].value);
          return TRUE;
        }
    }

  // filter out key of pattern: esp_N_node_map
  if ((strncmp(key, "esp_", 4) == 0) &&
      (strstr(key, "_node_map")))
    {
      strcpy(retVal, "###");
      decLoc = 3;
      return TRUE;
    }
  else if ((strcmp(key, "child_partitioning_function") == 0) ||
           (strcmp(key, "parent_partitioning_function") == 0))
    {
      // value for parent func has the form similar to: broadcast N times...
      // value for child func has form similar to: hash2 partitioned N ways...
      // Replace numbers in 'value' with '#'
      Int32 i = 0;

      // if child partitioning starts with hash1 or hash2, skip that token.
      // We dont want the numbers in 'hash1'/'hash2' to be replaced.
      if ((strcmp(key, "child_partitioning_function") == 0) &&
          (strncmp(value, "hash1", 5) == 0) ||
          (strncmp(value, "hash2", 5) == 0))
        {
          memcpy(retVal, value, 5);
          i += 5;
        }

      while (i < strlen(value))
        {
          if ((value[i] >= '0') && (value[i] <= '9'))
            retVal[i] = '#';
          else
            retVal[i] = value[i];
          i++;
        }

      retVal[i] = 0;
      return TRUE;
    }

  return FALSE;
}

void ExExeUtilDisplayExplainTcb::FormatLine(const char *key, const char *inval, Lng32 keySize,
                                            Lng32 valSize, Lng32 indent, Lng32 decLoc)
{
    char *line;                 // ptr to line to insert
    char *temp;                 // working ptr
    Lng32  cnt;                  // value space available
    Lng32  dashes;               // count of dots to set
    Lng32  keycut;               // size of keyword col permitted (cut if not zero)
    Lng32  field1;               // space for col 1, keyword
    
    char valBuf[1000];
    char * val = (char*)inval;
    // See if we can do anything
    if (cntLines_ >= MLINE) {                   // if output is full
      cnt = (Lng32)str_len(lines_[cntLines_-1]);// size line
      if (cnt > 60 ) {cnt = 60; }             // decide to concat or cut
      str_cpy_c((lines_[cntLines_-1])+cnt, "***LINES DROPPED***");
      return;                                 // skip all work
    }
    line = lines_[cntLines_];                   // point to empty line, don't change
    temp = line;                                // set working pointer

    if (exeUtilTdb().isOptionC() && key)
      {
        if (filterKey(key, keySize, val, valBuf, decLoc))
          {
            val = valBuf;
            valSize = strlen(val);

            if (exeUtilTdb().isOptionP())
              return; // prune
          }
      }

    // Do the indent if needed
    if (indent > 0) {
      cnt = indent;                           // don't change input
      while (cnt--) {*temp++ = ' '; }         // add blanks
    }
    
    // Do the keyword
    if (valSize == 0) {                         // if no value, it is easy
      if (keySize == 0) {                     // if key empty too
	*temp++ = ' ';                      // force one space
	*temp = '\0';                       // empty line only
      }
      else {
	str_cpy_c(temp, key);               // take the whole thing hope it is < 79
      }
      cntLines_++;                            // bump empty line counter
      return;                                 // and we are done
    }
    else if (keySize != 0) {                    // if keyword needed
      // Do all the keyword size calculations, COL2 is start for value field usually
      field1 = COL2 - 1 - indent - 2;         // space for keyword after all else
      // total col 1 - indent - 2 blanks
      if (decLoc >= 1) {                      // if number needs to be shifted
	decLoc--;                           // count digits left of 28
      }
      field1 -= decLoc;                       // see how much space we have for key
      keycut = keySize;                       // assume it will fit
      if (keySize > field1) {                 // if no fit
	if (keySize >= 16) {                // if key really long cut it, else don't
	  if (field1 < 16) {              // min key is 16
	    keycut = 16;                // set min key
	  }
	  else {
	    keycut = field1;            // cut less than min, as needed
	  }
	}
      }
      dashes = field1 - keycut;               // see how much filler needed
      // OK now keycut is size of keyword to use, then dashes, then value string
      
      // Do the keyword if needed, use keycut to determine size
      str_ncpy(temp, key, 28);                // take a few extra char sometimes
      temp += keycut;                         // go to place for dashes
      *temp++ = ' ';                          // put one space always
      if (dashes > 0) {                       // if some filler needed
	if (dashes < 3) {     // skip dashes if none or too few
	  while (dashes--) {*temp++ = ' '; }  // use space filler
	}
	else {
	  while (dashes--) {*temp++ = '.'; }  // use dot filler
	}
      }
      *temp++ = ' ';                          // put one space always
    }
    
    // Do the value
    cnt = (MLEN-1) - (Lng32)(temp - line);             // see what we have left
    //    cnt = 79 - (Lng32)(temp - line);             // see what we have left
    if (valSize > cnt) {                        // if input too large
      str_ncpy(temp, val, cnt);               // do it the hard way
      Int32 indexOfLastByteOfUtf8Char = IndexOfLastByteOfUTF8CharAtOrBeforePos((const unsigned char*)line,
                                                                               (MLEN-2) /*strLen*/,
                                                                               (MLEN-3) /*bytePos*/);
      if (indexOfLastByteOfUtf8Char < 0 || indexOfLastByteOfUtf8Char >= 77)
      {
        *(line+(MLEN-2)) = '*';                       // show it was cut
        *(line+(MLEN-1)) = '\0';
	//        *(line+78) = '*';                       // show it was cut
        //*(line+79) = '\0';
      }
      else
      {
        *(line+indexOfLastByteOfUtf8Char+1) = '*'; // show it was cut
        *(line+indexOfLastByteOfUtf8Char+2) = '\0';
      }
    }
    else {
      str_cpy_c(temp, val);                   // do it the easy way
    }
    
    // Clean up and exit
    cntLines_++;                                // bump empty line counter
    return;                                     // return nothing
}


/*************************************************************
Function Name: FormatLongLine

Argument List:
    char *key[in]           ptr to start of key word string
    char *val[in]           ptr to start of value string, must not be a constant
    long  keySize[in]       count of characters in key word string, 0 ok but ?
    long  valSize[in]       char to use from value string, < max ok, 0 not ok
    long  indent[in]        count of blank characters at start of line, default 0

Return Status: None

Class Data Accessed:
    lines_[out]             new print line set
    cntLines_[in/out]       count of lines in lines_ at start

Description:
This function will take the input keyword and value string and format it
into one or more 79 character lines in the lines_ array.  It expects the
input key parameter to be null-terminated, but val does not need to be.

It expects the input val string not to fit on one line, but it will be OK
if it does.  It will follow all the formatting rules defined in the ES.
It will cut the value into a series of pieces, the first will be 79-COL2
(51 characters) and all the rest will be 79-COL2-2 (49). The second
and later lines always start in col COL2+2 (30) with indent=COL2+1, no
matter what the initial indent was. The line count will be adjusted to
show all the lines added to lines_.  The output lines will look like this:
  DESCRIPTION
    scan_type .............. full scan of table ORDERS but this line is
                               folded to show the 2 char indent

This function will not modify the value string permanently.  In the
following discussion 49 means 79-COL2-2 or that +2 for first line.  It will
backscan 28 characters from the last character possible in this value
field (49 char usually) for a comma, remembering the first whitespace seen.
If it finds a comma it will break (insert a null character) the line there,
if not it will break it at the first whitespace if found.  In worst case
it will just cut the line at 49 characters.  It will use FormatLine to
store the line in the lines_ array.  Then it will restore the cut character,
skip whitespace and begin the scan again until the null character is seen.
Of course it will decrement the valSize for data and whitespace processed
and stop when the last piece fits in 49 characters.

This routine will rely on FormatLine to flag data loss.

The input value string must not be in a constant because this routine cuts
it as needed (and restores the cut after).  So val = "this is a test" does
not work.
*****/
void ExExeUtilDisplayExplainTcb::FormatLongLine(const char *key, char *val, Lng32 keySize,
                                                Lng32 valSize, Lng32 indent)
{
    char *inptr = val;          // current ptr to value string remaining
    Lng32  locSize = valSize;    // size of value string remaining
    char *tmp;                  // working ptr
    Lng32  cnt;                  // value space available for this pass
    Lng32  scancnt;              // count of characters backscanned
    Lng32  cutSize;              // size of string after cut
    char  c;                    // temp for cut character
    Lng32  sp;                   // space not found on this pass if 0
    Lng32  comma;                // comma not found on this pass if 0
    Lng32  first = 1;            // first line of fold being done if 1
    
    
    // Set the limit for the value string for line 1
    if (indent < COL2) {                        // if keyword also
      cnt = MLEN - COL2;                        // COL2 is start, so use 80
      //      cnt = 80 - COL2;                        // COL2 is start, so use 80
    }
    else {                                      // assume only a value
      cnt = (MLEN-1) - indent;                      // indent is char cnt so use 79
      //      cnt = 79 - indent;                      // indent is char cnt so use 79
    }
    
    // loop until no more data, return to caller from inside loop
    while (1) {                                 // loop until it fits on one line
      // See if we can do anything
      if (cntLines_ == MLINE) {               // if output is full already
	FormatLine(key, inptr, keySize, locSize, indent); //force the "full" message
	return;                             // skip remaining work
      }
      // We can do another line at least
      sp = 0;                                 // init for another pass
      comma = 1;
      scancnt = 0;
      if (locSize <= cnt) {                   // if all fits on this line
	tmp = inptr + locSize;              // should be null, but if not
	c = *tmp;                           // cut here, in case input larger
	*tmp = '\0';
	FormatLine(key, inptr, keySize, locSize, indent); //do it
	*tmp = c;                           // restore cut char
	return;
      }
      tmp = inptr + cnt;                      // point to last cut char possible
      if (*tmp == ',') {                      // it this is a comma can not take it
	tmp--;                              // logic will cut beyond comma, too long
      }
      while ((c = *tmp--) != ',') {           // backscan input watching for stuff
	if (scancnt++ >= 28) {              // if time to quit
	  comma = 0;
	  break;
	}
	if ((c == ' ') & (! sp)) {          // if this is the first space
	  sp = scancnt;                   // remember this position, never 0
	}
      }
      // now see what we have found to work with and define cut in tmp
      if (comma) {                            // if we found a comma
	tmp += 2;                           // skip beyond it
      }
      else if (sp) {                          // else use first space
	tmp = inptr + cnt - sp + 1;         // find sp char
      }
      else {                                  // else just cut at end
	Int32 lenInBytes = cnt;
	Int32 indexOfLastByteOfUtf8Char =
	  IndexOfLastByteOfUTF8CharAtOrBeforePos ( (const unsigned char *)inptr // utf8Str
	                                         , (const Int32) lenInBytes     // utf8StrLenInBytes
	                                         , (const Int32) (lenInBytes-1) // bytePos
	                                         );
	if (indexOfLastByteOfUtf8Char >= 0)
	  lenInBytes = indexOfLastByteOfUtf8Char + 1;
	tmp = inptr + lenInBytes;
      }
      c = *tmp;                               // cut here beyond good
      *tmp = '\0';
      cutSize = (Lng32)(tmp - inptr);
      FormatLine(key, inptr, keySize, cutSize, indent); //do it finally
      *tmp = c;                               // restore cut char
      while (*tmp++ == ' ') {cutSize++; }     // drop leading spaces
      tmp--;                                  // back to first non-space
      locSize -= cutSize;                     // set remaining size
      inptr = tmp;                            // set remaining string
      if (first) {                            // if first line
	cnt -= 2;                           // field 2 smaller on second line
	keySize = 0;                        // say no more key, my copy
	indent = (MLEN-1) - cnt;                  // set the indent, my copy
	//	indent = 79 - cnt;                  // set the indent, my copy
	first = 0;                          // next is not first
      }
    } // end while forever
}


/*************************************************************
Function Name: FormatSQL

Argument List:
    char *key[in]           ptr to start of key word string
    char *val[in/out]       ptr to start of value string, it will be modified
    long  keySize[in]       count of characters in key word string, 0 ok but ?
    long  valSize[in]       char to use from value string, < max ok, 0 not ok
    long  indent[in]        count of blank characters at start of line, default 0

Return Status: None

Class Data Accessed:
    lines_[out]             new print line set
    cntLines_[in/out]       count of lines in lines_ at start

Description:
This function will fold an SQL statement at logical tokens when possible.
This is not to be a parser.  If the statement exceeds reasonable complexity
limits it will be folded using the basic FormatLongLine function.  The input
val will be null-terminated at valSize at start of processing and restored
at the single exit.  If it is already null terminated that is OK.  The key
string should be null-terminated already.

The algorithm will first decide if the statement starts with "select",
"insert into", "update", "delete from" in either upper case or lower case.
If it gets no matches it does the default folding.  It then makes the
assumption that the user has been consistent in use of case so there is
no need to convert case.  If not, you find no subsequent tokens and get the
default folding.

Once it finds a command type it looks for the legal keywords for that command
in order.  It also watches for open parenthesis to skip subquerys or watch for
data in the insert command.  It will fold before the next logical operator
and call the FormatLongLine to do the last section, no matter how long it is.
It tries to do a reasonable formatting for insert data.  It tries to get through
nested parenthesis, but gives default folding on the third nesting level.
The nesting shown below should be processed as shown.

The desired output is:
            SELECT cust_name, cust_contact, cust_address, cust_city,
              cust_state
            FROM Customers
            WHERE cust_id IN (SELECT cust_id FROM Orders WHERE order_num
              IN (SELECT order_num FROM OrderItems WHERE prod_id =
              'RGAN01')) AND cust_state = 'CA'
            ORDER BY cust_contact;
and:
            insert into Customers(cust_id, cust_name, cust_address,
              cust_city, cust_state, cust_zip, cust_country)
            values
              ('1000000006', 'Toy Land', '123 Any Street', 'New
                York', 'NY', '11111', 'USA'),
              ('1000000007', 'Toy Town for Tots', '27364 West Happy Lane',
                'Chicago', 'IL', '37363', 'USA');

There are a lot of possible inputs when you consider embedded commands and parens
in the "AND" and "OR" clauses.  So if we cannot figure out what to do, we will
just use the basic folding provided by FormatLongLine().

Because of the line cutting, only one return is permitted in this routine.  The
white space compression may pad the end with blanks and change the last char.
*****/
void ExExeUtilDisplayExplainTcb::FormatSQL(const char *key, char *val, Lng32 keySize,
                                                Lng32 valSize, Lng32 indent)
{
    // Initialize working constants once only, order important for DMLword use
    char DMLUsel[8][12] = {"SELECT", "FROM", "WHERE", "UNION",
        "GROUP BY", "HAVING", "ORDER BY", "FOR"};
    char DMLLsel[8][12] = {"select", "from", "where", "union",
        "group by", "having", "order by", "for"};
    char DMLUins[6][12] = {"INSERT INTO", "VALUES", "SELECT", "FROM",
        "WHERE", "FOR"};
    char DMLLins[6][12] = {"insert into", "values", "select", "from",
        "where", "for"};
    char DMLUupd[4][12] = {"UPDATE", "SET", "WHERE", "FOR"};
    char DMLLupd[4][12] = {"update", "set", "where", "for"};
    char DMLUdel[3][12] = {"DELETE FROM", "WHERE", "FOR"};
    char DMLLdel[3][12] = {"delete from", "where", "for"};
    Lng32 DMLword[8] = {8, 8, 6, 6, 4, 4, 3, 3}; // entries/array

    // The working variables can go on the stack
    char *inptr = val;          // current ptr to value string remaining
    Lng32  locSize = valSize;    // size of value string remaining
    char *the_end = val+valSize;// pointer to real end of string + 1
    char last_char = *the_end;  // last char in string + 1, usually a blank
    Lng32  cnt;                  // value space available for this pass
    char *DMLptr = NULL;         // DML array to be used for this command
    Lng32 DMLtype = 0;           // DML array, 0 = DMLUsel, 1 = DMLLsel, etc. order above
    Lng32 DMLpos = 0;            // DML entry being tested
    Lng32 DMLecnt;               // Entry count in chosen DML array
    Lng32 paren[2];              // next parens found, start/end position
    Lng32 toksize;               // size of next token in the array
    Lng32 cut;                   // position of cut (relative to inptr)
    Lng32 parUsed = 0;           // flag saying we skipped over parens now
    Lng32 valSeen = 0;           // flag, =1 insert being processed, =2 values now
    Lng32 good;                  // end paren for last good data looked at
    Lng32 good2;                 // end paren for last cut looked at
    char *locinptr;             // temp in ptr for use in substrings
    char *tn = NULL;            // working ptr, temp next for compression
    Lng32 moving = 0;            // =1 compression has begun
    Lng32 cnt_blank;             // count of space characters seen together
    Lng32 quoting = 0;           // =1 we are in a quoted string, no compression
    char *tmp;                  // working ptr (ptr to cut spot)
    char  c;                    // temp for cut character
    char  d = 0;                // temp for quote char

    // Set the limits for the value string, assume indent not larger than COL2
    cnt = MLEN - COL2;                            // calc the value space for line 1
    //    cnt = 80 - COL2;                            // calc the value space for line 1
    *the_end = '\0';                            // restore cut at DONE (only exit)

    // If really short do nothing
    if (valSize <= cnt) {                       // if fits on one line
      goto DONE;
    }
    
    // Determine which of the 8 DML commands we have
    while (DMLtype < 8) {                       // step through all 8 arrays
      switch (DMLtype) {                      // set a new array
      case 0: {DMLptr = DMLUsel[0]; break; }
      case 1: {DMLptr = DMLLsel[0]; break; }
      case 2: {DMLptr = DMLUins[0]; break; }
      case 3: {DMLptr = DMLLins[0]; break; }
      case 4: {DMLptr = DMLUupd[0]; break; }
      case 5: {DMLptr = DMLLupd[0]; break; }
      case 6: {DMLptr = DMLUdel[0]; break; }
      case 7: {DMLptr = DMLLdel[0]; break; }
      }
      toksize = (Lng32)str_len(DMLptr);
      if (str_ncmp(DMLptr, inptr, toksize) == 0) {// if this is it
	break;
      }
      DMLtype++;
    }
    if (DMLtype == 8) {                         // If unknown, do basic format
      goto DONE;
    }
    if ((DMLtype == 2) || (DMLtype == 3)) {     // if doing an insert
      valSeen = 1;                            // say watch for "value" clause
    }
    DMLecnt = DMLword[DMLtype];                 // set the token count
    
    // compress whitespace
    tmp = inptr;                                // first char never blank
    cnt_blank = 0;                              // no whitespace yet
    while (c = *tmp++) {                        // while not end
      if (c == '\t') {                        // if tab
	c = ' ';
	*(tmp-1) = c;                       // store a blank
      }
      if (! quoting) {                        // if not inside a quote
	if (c == ' ') {                     // if space, look closer
	  cnt_blank++;                    // count this one
	} else {
	  cnt_blank = 0;                  // reset count
	}
	if ((c == '\'') || (c == '"')) {    // if starting a quote
	  quoting = 1;                    // note that
	  d = c;                          // remember the type
	}
	if (cnt_blank > 1) {                // if compression needed
	  if (! moving) {                 // if compression not started
	    moving = 1;                 // note it must start
	    tn = tmp-1;                 // note where next non blank goes
	  }
	  locSize--;                      // adjust string length
	  continue;                       // just drop this extra space
	}
      } else {                                // watch for end of quote
	if (c == d) {quoting = 0; }         // say outside quote now
      }
      if (moving) {                           // if compression started
	*tn++ = c;                          // save this char
      }
    }
    tmp--;                                      // back to null
    if (moving) {                               // if we compressed
      *tmp = last_char;                       // restore cut
      the_end = tn;                           // save new end
      last_char = ' ';                        // force space here eventually
      while (tn != tmp) {*tn++ = ' '; }       // pad with space
      *the_end = '\0';                        // cut again
    }
    
    // Scan for first set of parens, set data in paren[]
    if (FindParens(inptr, paren)) {             // if find error (no parens is not error)
      goto DONE;
    }
    
    // loop until no more data, return to caller from inside loop
    while (1) {                                 // loop until it fits on one line
      // See if we can do anything
      if (cntLines_ == MLINE) {               // if output is full already
	goto DONE;
      }
      
      // If value clause of insert do first line special
      if (valSeen == 2) {                     // do the first line without data
	cut = 0;
	tmp = inptr;
	while (*tmp++ != '(')  {cut++; }    // scan for start of data
	tmp--;                              // point to first open
	// now tmp and cut are set to fold
	c = *tmp;                           // cut here
	*tmp = '\0';
	FormatLongLine(key, inptr, keySize, cut, indent); //do it
	*tmp = c;                           // restore cut char
	
	// Set counters for next loop
	inptr = tmp;                        // skip data done
	locSize -= cut;                     // set remainder size
	indent = COL2 + 1;                  // indent rest a little
	cnt = (MLEN-2) - COL2 - 2;                // less data now (extra 2 for , at end)
	//	cnt = 78 - COL2 - 2;                // less data now (extra 2 for , at end)
      }
      good2 = 0;                              // last good place seen to cut
      // If value clause of insert do data lines special, return from while
      while (valSeen == 2) {                  // if we are on a values string
	if (locSize <= cnt) {               // if space for the rest
	  goto DONE;
	}
	locinptr = inptr;                   // keep inptr unchanged for awhile
	good = 0;                           // nothing found yet
	while (1) {                         // while more find max line
	  // Scan for next close parens
	  while (c = *locinptr++) {       // while end not seen
	    if (c == ')') {             // if close paren
	      good = (Lng32)(locinptr - inptr);// note possible cut
	      locinptr--;             // back up in case of end
	      break;
	    }
	  }
	  // scan for next data item start's open paren, or ending semi
	  while (c = *locinptr++) {       // while end not seen
	    if (c == '(') {             // if next open paren seen
	      locinptr--;             // point to open
	      break;
	    }
	    else if (c == ';') {        // if end seen and no next
	      locinptr--;             // point to semicolon
	      break;
	    }
	  }
	  if (c == '\0') {                // if not found, syntax issue
	    goto DONE;
	  }
	  // now we have a close and next open found, decide what to do
	  if (good <= (cnt - 2)) {        // if it fits on line
	    good2 = good;               // remember it and try for more
	    good = 0;                   // reset
	    continue;                   // scan again
	  }
	  else if (! good2) {             // if first is too long for line
	    if (c == ';') {             // if it is also the last
	      goto DONE;              // just finish up
	    }
	    tmp = locinptr;             // point to next open
	    cut = (Lng32)(tmp - inptr);
	  }
	  else {                          // we know there is another and where to cut
	    cut = good2;
	    tmp += cut;                 // point to last good close
	    while (*tmp++ != '(') {cut++; } // find next open
	    tmp--;                      // back up one
	  }
	  break;                          // decided where to cut now
	}
	// now tmp and cut are set to fold
	c = *tmp;                           // cut here
	*tmp = '\0';
	FormatLongLine(key, inptr, keySize, cut, indent); //do it
	*tmp = c;                           // restore cut char
	
	// Set counters for next loop
	inptr = tmp;                        // skip data done
	locSize -= cut;                     // set remainder size
	good2 = 0;                          // start over
      } // end while for insert data processing, never fall through while
      
      // move to next token in list
      DMLpos++;                               // move to next token number
      if (DMLpos >= DMLecnt) {                // if no more to look for
	goto DONE;
      }
      DMLptr += 12;                           // skip to next expected token
      locinptr = inptr;                       // keep inptr unchanged for awhile
      // scan forward looking for this keyword in input
      while (1) {                             // max 2 tries when substrings
	tmp = str_str(locinptr, DMLptr);    // see if this is present
	if (tmp == NULL) {                  // if not found
	  break;                          // try another
	}
	cut = (Lng32)(tmp - inptr);          // see how many char we have this time
	
	// Found but make sure not in parens
	if (paren[0] > 0 ) {                // if there is a paren somewhere
	  if (paren[0] < cut) {           // if substring somewhere inside this
	    if (paren[1] < cut) {       // if substring closed inside
	      parUsed = 1;            // note we consumed the parenthesis
	    }
	    else {
	      locinptr += paren[1];   // take all of subquery or whatever
	      continue;               // try again on this one
	    }
	  }
	}
	break;
      } // end substring processing
      if (tmp == NULL) {                      // if really not found
	continue;                           // try another
      }
      
      // Now we have a logical element identified, output it
      tmp = inptr + cut;                      // point to cut char, but if not
      c = *tmp;                               // cut here
      *tmp = '\0';
      FormatLongLine(key, inptr, keySize, cut, indent); //do it
      *tmp = c;                               // restore cut char
      
      // Set counters for next loop
      inptr = tmp;                            // skip data done
      locSize -= cut;                         // set remainder size
      if (keySize != 0) {                     // if first line done, set for rest
	keySize = 0;                        // say no more key, my copy
	indent = COL2 - 1;                  // force into column 2, my copy
      }
      if (paren[0] > 0 ) {                    // if a paren
	if (parUsed) {                      // if we consumed them this time
	  // Scan for next set of parens, set data in paren[]
	  if (FindParens(inptr, paren)) { // if find error
	    goto DONE;
	  }
	}
	else {                              // else reduce location
	  paren[0] -= cut;
	  paren[1] -= cut;
	}
      }
      if (valSeen) {                          // if doing insert
	if (DMLpos == 1) {                  // if doing values of insert
	  valSeen = 2;                    // note this for next pass
	}
      }
      
    } // end while forever
    
    // Do last line and return
DONE:
    FormatLongLine(key, inptr, keySize, locSize, indent);
    *the_end = last_char;                       // restore cut with original char
    return;
}


/*************************************************************
Function Name: FindParens

Argument List:
    char *inStr[in]         ptr to start of string to scan
    long par[4][out]        ref to array to store up to two, pos open, close

Return Status: 
    long                    = 0, success, = -1, something wrong

Description:
This function will scan the input string for the next set of parenthesis.
It will accept at most 2 levels of nesting (-1 when 3rd seen) and return
their char positions from inStr.  It will return -1 if no match found.
*****/
Lng32 ExExeUtilDisplayExplainTcb::FindParens(char *inStr, Lng32 par[]) const
{
  char *tmp1 = inStr;         // working ptr
  Lng32 par2dp = 0;            // nesting found if not 0
  Lng32 cnt = 0;               // char counter
  char c;
  
  // Begin first scan for open paren
  par[0] = par[1] = 0;                        // reset all
  while (c = *tmp1++) {                       // scan to end of string
    if (c == '(') {                         // if we have an open paren
      par[0] = cnt;                       // note its position
      break;                              // and stop scan
    }
    cnt++;
  }
  if (par[0] == 0) {return (0); }             // if none, success
  cnt++;                                      // tmp1 is on next
  
  // Second scan is for close without another open
  while (c = *tmp1++) {                       // scan to end of string
    if (c == ')') {                         // if we have a close paren
      if (par2dp == 0) {                  // if no other open
	par[1] = cnt;                   // note its position
	break;                          // and stop scan
      }
      else {                              // we closed embedded open
	par2dp = 0;                     // don't care about other open now
      }
    }
    else if (c == '(') {                    // if we have another open paren
      if (par2dp != 0) {return (-1); }    // if third embedded, give up
      par2dp = cnt;                       // note its position
    }
    cnt++;
  }
  if (par[1] == 0) {return (-1); }            // if no close, syntax errro
  return (0);                                 // return success
}


/*************************************************************
Function Name: FormatFirstLine

Argument List: None

Return Status: None

Class Data Accessed:
    operName_[in]           text name of this node of the plan
    seqNum_[in]             long number of this node
    leftChild_[in]          long number of this node's left child
    rightChild_[in]         long number of this node's right child
    lines_[out]             new print line set
    cntLines_[in/out]       count of lines in lines_ at start

Description:
This function will take the specified private variables and format them
into a single 79 character line in the lines_ array.  It expects the input
values to fit.  The line count will be adjusted to show another line in
lines_.  The output line will look like this:
ROOT ======================================  SEQ_NO 7    CHILDREN 6, 18
ROOT ======================================  SEQ_NO 7    ONLY CHILD 6
FILE_SCAN =================================  SEQ_NO 7    NO CHILDREN

The formatting rules are simple.  The = finish in col 43, 44-45 are blank,
and the SEQ token starts in col 46 and the word CHILDREN in col 62.  A right
child number of 0 causes the ONLY CHILD format, and right and left of 0
cause the NO CHILDREN format.

This routine will check for an empty line in the lines_ array and do nothing
if there is no space.  That should never happen.
*****/
void ExExeUtilDisplayExplainTcb::FormatFirstLine(void)
{
  char *line;                 // ptr to line to insert
  Lng32  cnt;                  // process name size
  Lng32  dashes;               // count of equals to set
  
  
  // See if we can do anything, we require two lines left
  if (cntLines_ >= MLINE-1) {                 // if output is almost full
    FormatLine(NULL,NULL,0,0);              // force a blank line or the "full" msg
    FormatLine(NULL,NULL,0,0);              // force the "full" message
    return;                                 // skip all work
  }
  line = lines_[cntLines_];                   // point to empty line
  
  // Format the first two fields of the line
  cnt = (Lng32)str_len(operName_);             // see what we got, max is MOPER=30
  str_cpy_c(line, operName_);                 // start the line with operator name
  line += cnt;
  *line++ = ' ';                              // one space
  dashes = 43 -1 - cnt;                       // see how much filler
  while (dashes--) {*line++ = '='; }          // add equals
  str_sprintf (line, "  SEQ_NO %d            ", seqNum_); // add sequence nbr
  line += 18;                                 // point to next area
  
  // This algorithm assumes we have at least two lines in the lines_
  // array to allow overflow.  Should always be, but was checked earlier
  if (rightChild_ == 0) {                     // if right does not exist
    if (leftChild_ != 0) {                  // if it exits
      str_sprintf (line, "ONLY CHILD %d", leftChild_);
    }
    else {                                  // neither exist
      str_sprintf (line, "NO CHILDREN");
    }
  }
  else {                                      // else two node format needed
    str_sprintf (line, "CHILDREN %d, %d", leftChild_, rightChild_);
  }
  // verify numbers were not too long
  cnt = MWIDE - 61;                           // set max size in char 
  if ((Lng32)str_len(line) > cnt) {            // if it exceed 79 char
    line += cnt;                            // point to last char
    *line-- = '\0';                         // do cut
    *line = '*';                            // show cut
  }
  
  // Clean up and exit
  cntLines_++;                                // bump empty line counter
  return;                                     // return nothing
}


/*************************************************************
Function Name: DoSeparator

Argument List: None

Return Status: None

Class Data Accessed:
    header_[in]             set if doing header or root node
    lastFrag_[in/out]       previous fragment number
    lastOp_[in/out]         previous operator name
    operName_[in]           text name of this node of the plan
    description_[in]        data for node, contains frag number
    lines_[out]             new print line set
    cntLines_[in/out]       count of lines in lines_ at start

Description:
This function will output two plan separators for the first node of a plan.
The first will be for the header, the second for the root node.  They are
controlled by the settings of "header_" and "lastFrag_".  When header_ is 1 and
lastFrag_ is not -1 it does the start-of-plan separator and sets lastFrag_ to -1.
When header_ is 1 and lastFrag_ is -1 it does the start-of-nodes separator and  
sets the lastFrag_ to 0. For R1 this is all the separators it supports.  When a
spearator is set it outputs a line of dashes ending with the sparator name.

In R2 this will behave as above when header_ is 1, but in all other cases it
will find the fragment number in the description column and and output a process
separator if the current fragment is different than the one currently in
lastFrag_ (and set it to the current value).  When the current fragment and
lastFrag_ are the same it will do nothing.  It will always record current
operator name to help with decisions on next call.

This routine will check for an empty line in the lines_ array and do nothing
if there is no space.
*****/
void ExExeUtilDisplayExplainTcb::DoSeparator(void)
{
  char *line;                 // ptr to line to insert
  const char *proc_name;      // ptr to process name string, ""=none
  Lng32  cnt;                  // process name size
  Lng32  dashes;               // count of dashes to set
  
  // See if we can do anything
  if (cntLines_ >= MLINE) {                   // if output is full
    return;                                 // skip all work
  }
  line = lines_[cntLines_];                   // point to empty line
  proc_name = "";                             // set name to none
  
  // Check for start of a new plan
  if (header_) {                              // if start of a plan
    if (lastFrag_ == -1) {                  // if start of root node
      //proc_name = " MASTER PROCESS";          // name this process
      proc_name = " NODE LISTING";        // name this process
      lastFrag_ = 0;                      // set real frag nbr
    }
    else {
      proc_name = " PLAN SUMMARY";        // name this as header
      lastFrag_ = -1;                     // say header done
    }
  }
  
  // find current frag number and compare to last
  // this code will be written once we have frag in the description or decide we won't.
  /*
    if (strcmp(lastOp_, "PARTITION_ACCESS") == 0) { // if sure to be DAM
    proc_name = " DISK ACCESS";
    }
    */
  // Output line now if needed
  cnt = (Lng32)str_len(proc_name);             // get size
  if (cnt) {                                  // if something to output
    dashes = MWIDE - cnt;                   // see how many dashes needed
    while (dashes--) {*line++ = '-'; }      // set dashes
    str_cpy_c(line, proc_name);             // set name
    cntLines_++;                            // bump empty line counter
  }
  
  // Clean up and exit
  //    str_cpy_c(lastOp_, operName_);              // remember this oper
  return;                                     // return nothing
}


/*************************************************************
Function Name: ParseField

Argument List:
    char* &keyptr[out]      reference to ptr to key word, colon removed
    char* &fieldptr[out]    reference to ptr to value string
    long  &keySize[out]     count of characters in key string
    long  &fullSize[out]    count of characters in value string
    long  &done[out]        1=end of string seen, 0=not seen

Return Status: 
    long                    = 0 on success
                            = -1 no keyword found that ends in ": "

Class Data Accessed:
    parsePtr_[in/out]       points to string being processed

Description:
This function will start from the character pointed to by parsePtr_ and scan
until it has found the keyword and value pair.  It will cut each by dropping
a null character into the string and remember the start of each to return.
It will flag the last value in the input (end of string seen, even if there is
blank padding after value ends).

If it fails in any way it will return a -1 and leave the pointer set to where it began
the field scan that failed.  In this case there could be a value pointer returned
or not, but only the done paramater can be used.  There are several ways
it could fail, but they should not happen unless the input data is corrupted.
*****/
Lng32 ExExeUtilDisplayExplainTcb::ParseField (char *&keyptr, char *&fieldptr,
                                             Lng32 &keySize, Lng32 &fullSize, Lng32 &done)
{
  // Initialize working variables
  char *wrkptr;               // working ptr
  
  // Isolate the keyword
  keyptr = parsePtr_;                         // remember start of keyword
  wrkptr = str_str(keyptr, ": ");             // find end of key
  if (wrkptr == NULL) {                       // if not found
    fullSize = 0;
    done = 1;                               // say no more
    return (-1);
  }
  *wrkptr = '\0';                             // cut off colon
  keySize = (Lng32)(wrkptr - keyptr);          // get size
  wrkptr++;                                   // move to blank
  while ((*wrkptr) == ' ') {wrkptr++; }       // scan through blanks
  
  // isolate the value
  fieldptr = wrkptr;                          // must be value for keyword
  wrkptr = str_str(fieldptr, ": ");           // see if we can find next keyword
  if (wrkptr == NULL) {                       // if not found, must be last in string
    fullSize = (Lng32)str_len(fieldptr);     // just use all of it
    wrkptr = fieldptr + fullSize - 1;       // locates last char
    while (*(wrkptr--) == ' ') {            // backscan over blanks
      fullSize--;
    }
    wrkptr += 2;
    *(wrkptr) = '\0';                       // cut here
    parsePtr_ = wrkptr;                     // no good reason for doing this
    done = 1;                               // say no more
  }
  else {                                      // there is another
    // Scan back to end of value (first non-blank after a blank)
    while ((*wrkptr) != ' ') {wrkptr--;}    // back up to blank
    parsePtr_ = wrkptr + 1;                 // remember start of next keyword
    while ((*wrkptr) == ' ') {wrkptr--;}    // back over blanks
    wrkptr++;                               // point to where null should be
    *(wrkptr) = '\0';                       // cut here
    done = 0;                               // say more to come
  }
  if (fieldptr >= parsePtr_)       //an erroneous overflow into the next keyword
    fieldptr = wrkptr;            //due to no value, empty string value, or error


  fullSize = (Lng32)(wrkptr - fieldptr);       // get length of value string
  return (0);                                 // return success
}


/*************************************************************
Function Name: IsNumberFmt

Argument List:
    char *fieldptr[in]      ptr to value string to test

Return Status: 
    long                    0=char_str, 1=num_str

Description:
This function will determine if this value string needs further processing
by the FormatNumber routine to reformat it and count digits before the
decimal point.  Single digit are not flagged numeric as no special support
is needed.  When identified as numeric they must be copied to a temp buffer
becaues FormatNumber might increase the size of the string.  If the value
is known to be numeric by other info don't bother to call this function.

The algorithm is set to fail fast if FormatNumber is not needed.  The basic
approach is to fail if the current character could not be in a number.  If
it is not a number it must watch for +, -, E, and dot.  Some further
validation is provided for these cases, and weird combinations are flagged
as strings.  This is hard as the characters are not processed in a full
state machine.
*****/
Lng32 ExExeUtilDisplayExplainTcb::IsNumberFmt(char *fieldptr) const
{
  char *wrkptr = fieldptr;    // working ptr
  char c;                     // temp for current char
  Lng32 n_cnt = 0;             // Number of digits seen
  Lng32 d_cnt = 1;             // Number of '.' permitted in string
  Lng32 e_cnt = 1;             // Number of 'E' permitted in string
  
  // Scan for simple numeric of type 22, 22.22, -3.0E-003, ignore single digits
  if (*(wrkptr+1) == '\0') {return(0); }      // no need to determine if one digit
  while (c = *wrkptr++) {                     // while not end of string
    if ((c > '9') || (c < '0')) {           // if not numeric
      if ((c == '+') || (c == '-')) {     // if a sign
	if ((*wrkptr <= '9') && (*wrkptr >= '0')) {// if followed by number
	  if ((n_cnt == 0) || (*(wrkptr-2) == 'E')) {// if position OK
	    continue;
	  }
	}
      }
      else if ((c == '.') && d_cnt--) {   // if first decmal point
	if ((n_cnt > 0) && (e_cnt == 1)) { // and not start but before E
	  if (*wrkptr == '\0') { continue; } // if nothing follows
	  if ((*wrkptr <= '9') && (*wrkptr >= '0')) {// if followed by number
	    continue;
	  }
	}
      }
      else if ((c == 'E') && e_cnt--) {   // if first E
	if ((n_cnt > 0) && (*wrkptr != '\0')) { // and not start or end of string
	  continue;
	}
      }
      return (0);                         // can't be a number
    }
    else {
      n_cnt++;                            // count as digit
    }
  }
  // OK this could be a number
  if ((n_cnt > 1) && (n_cnt < 12)) {          // if a few digits, single dig ignored
    return (1);                             // say it is a number
  }
  return (0);                                 // too long to be a real number
}


/*************************************************************
Function Name: GetField

Argument List:
    char *col[in]           ptr to start of col data to scan, never changed
    char *key[in]           ptr to key word desired, end with colon and space
    char*&fieldptr[out]     reference to ptr to value string if found
    long &fullSize[out]     count of characters in value string found, else 0

Return Status: 
    long                    = 0 on success
                            = 1 if not found
                            = -1 if input key does not end in ": "

Description:
This function will scan the supplied string for the specified keyword and if found
return the pointer to the start of the value and its size and type.  The resulting
string will NOT be null terminated unless the value is the last in the input string.
It will count characters in the value as it goes.  If the key value is not found it
will return a 1, which is not considered an error.

If it fails in any way it will return a -1 and leave the pointer set to where it began
the field scan that failed.  In this case there could be a value pointer returned
or not, but none of the return parameters should be used.  There are several ways
it could fail, but they should not happen unless the input data is corrupted.
*****/
Lng32 ExExeUtilDisplayExplainTcb::GetField (char *col, const char *key, char *&fieldptr,
                                           Lng32 &fullSize) const
{
  // Initialize working variables
  char *keyptr;               // keyword start
  char *wrkptr;               // working ptr
  
  // Verify keyword is as expected, this test could be removed eventually
  keyptr = str_str(key, ": ");
  if (keyptr == NULL) {                       // if not found
    fullSize = 0;
    return (-1);
  }
  
  // Scan for the keyword, should end with ": ", or report problem
  keyptr = str_str(col, key);
  if (keyptr == NULL) {                       // if not found
    fullSize = 0;
    return (1);
  }
  // Skip over keyword and find start of value
  wrkptr = str_str(keyptr, ": ") + 1;         // get end of keyword
  while ((*wrkptr) == ' ') {wrkptr++; }       // scan through blanks
  
  // isolate the value
  fieldptr = wrkptr;                          // must be value for keyword
  wrkptr = str_str(fieldptr, ": ");           // see if we can find next keyword
  if (wrkptr == NULL) {                       // if not found, must be last in string
    fullSize = (Lng32)str_len(fieldptr);     // just use all of it
    wrkptr = fieldptr + fullSize - 1;       // locates last char
    while (*(wrkptr--) == ' ') {            // backscan over blanks
      fullSize--;
    }
    wrkptr += 2;                            // point to where we should cut
  }
  else {                                      // there is another
    // Scan back to end of value (first non-blank after a blank)
    while ((*wrkptr) != ' ') {wrkptr--;}    // back up to blank
    while ((*wrkptr) == ' ') {wrkptr--;}    // back over blanks
    wrkptr++;                               // point to where null should be
  }
  fullSize = (Lng32)(wrkptr - fieldptr);       // get length of value string
  return (0);                                 // return success
}


/*************************************************************
Function Name: FormatNumber

Argument List:
    char *outStr[out]       storage for output, must be space for MUSERSP char
    long &intSize[out]      reference to int part of output, 0=Exponential fmt
    long &fullSize[out]     ref to size of whole number field
    char *strVal[in]        number in raw ASCII format to be formatted

Return Status: None

Class Data Accessed:
    optFlag_                = E_, 4 decimal digits desired, else 2

Description:
This function will determine which input format is present, 35, 35.000, 35.889,
3.5000E+001, or 3.5889E+001 by scanning for the dot and the E.  If it is already
in floating point it will just cut the decimal digits to 2 or 4 places (with
rounding) depending on optFlag_ (or none if .0000).  If it is integer it will do
nothing but count digits and set intSize = fullSize.  Output will be in the format
given by FormatFloat.  Negative numbers work, but are not expected.

If it is in exponential format it will look at the exponent and decide if it should
be changed or not.  It will walk the string looking for the decimal point and the
full size.  In addition to cutting at 2 or 4 digits  it will round
at the cut.  The code will just move the decimal point as indicated by the exponent.

This routine must be careful to use only string functions that exist in the executor.
*****/
void ExExeUtilDisplayExplainTcb::FormatNumber (char *outStr, Lng32 &intSize,
                                              Lng32 &fullSize, char *strVal)const
{
  char c;                     // temp
  Lng32 cnt = 0;               // count of characters in input
  Lng32 neg = 0;               // input is negative if 1
  Lng32 cntzero;               // count of trailing zeros
  Lng32 maxIntDig;             // max int digits we can add commas into
  Lng32 decSize;               // final dot and digit size when a float
  Lng32 maxSize;               // place to cut long floats
  Lng32 intexp;                // exponent string converted to int
  Lng32 expdig = 0;            // loc of E if found
  Lng32 dotdig = 0;            // loc of decimal point if found
  char *ptrin1 = strVal;      // input pointer, chg only if neg
  char *ptrout = outStr;      // output pointer, chg only if neg
  char *ptrwrk = strVal;      // working variables
  char *tmpPtr;               // working variable
  Lng32 negFlg = 0;            // exponent is not negative
  
  
  //Scan the input for decimal point and E and total length.
  if (*ptrin1 == '-') {                       // if input negative
    *ptrout++ = *ptrin1++;                  // move - to output and forget
    ptrwrk = ptrin1;
    neg = 1;                                // remember negative used 1 output digit
  }
  // ignore neg numbers from now until final exit
  while ((c = *ptrwrk++) != '\0') {           // scan input watching for end
    cnt++;                                  // count this char
    switch (c) {
    case 'e':  *(ptrwrk-1) = 'E';       // upcase it and no break
    case 'E':  expdig = cnt; break;
    case '.':  dotdig = cnt; break;
    default:   break;
    }
  }
  // deal with a input size problem right now
  if (cnt > MUSERSP-1-neg) {                  // trouble in a few rare cases
    cnt = MUSERSP-1-neg;                    // cut at required output limit
    str_ncpy(ptrout, ptrin1, cnt);          // move part we can
    *(ptrout+cnt) = '\0';                   // cut it
    *(ptrout+cnt-1) = '*';                  // show we cut it
    intSize = 0;                            // say just a string
    fullSize = MUSERSP-1;
    return;                                 // forget processing this one
  }
  
  // Process input that is pure integer (common)
  if ((! dotdig) && (! expdig)) {             // if plain integer, like 35
    str_cpy_c(ptrout, ptrin1);              // move whole string
    intSize = cnt;                          // set size of int
    maxIntDig = MUSERSP - (intSize-1)/3 - neg; // calc max int digits we can expand
    if ((intSize > 3) && (intSize < maxIntDig)) {// if commas needed and there is space
      AddCommas (ptrout, intSize);        // add commas, adjust count
    }
    goto DONE;                              // this is finished
  }
  
    // Process input that is in expontial format
    // Convert exp to real if in legal range, else just return the input or limit values.
  if (expdig) {                               // convert exp to float format
    // no sscanf available in executor, so convert ascii exp to a long inline
    intexp = 0;
    tmpPtr = ptrin1+expdig;                 // point to first char of exp
    if (*tmpPtr == '-') {                   // if it is negative
      negFlg = 1;                         // remember that
      tmpPtr++;                           // move on
    }
    else if (*tmpPtr == '+') {tmpPtr++; }   // if plus, move on
    while (*tmpPtr != '\0') {               // while something, convert it
      intexp = intexp*10 + *tmpPtr++ - '0'; // assume 0 to 9 char
    }
    if (negFlg) {intexp = -intexp; }        // if negative flip it
    // decide if it is too large or small for float
    if (intexp > 9) {                       // if too large give up
      str_cpy_c(ptrout, ptrin1);          // just move it all
      intSize = 0;                        // say exp still
      fullSize = cnt + neg;               // watch for neg
      return;
    }
    else if ((optFlag_ == N_) && (intexp < -2)) {// if too small for normal mode
      str_cpy_c(ptrout, "0.01");          // set min
      intSize = 1 + neg;                  // watch for neg
      fullSize = 4 + neg;
      return;
    }
    else if ((optFlag_ == E_) && (intexp < -4)) {// if too small for expert mode
      str_cpy_c(ptrout, "0.0001");        // set min
      intSize = 1 + neg;                  // watch for neg
      fullSize = 6 + neg;
      return;
    }
    // We will convert exp input like 3.588903E+003 to float here.
    // Result must contain a decimal point for further processing.
    // Just move the decimal point around using intexp to tell where.
    // We cannot use sscanf and str_sprintf which can't take "%.4f" yet.
    //
    // first calc the locations of stuff defining the output
    Lng32 begzero = 0;
    Lng32 decloc = 1;
    Lng32 digmove = expdig - 1;              // digits we have to move
    Lng32 i = 0;                             // digits moved
    Lng32 decdig = 2;                        // decimal digits max for normal mode
    if (optFlag_ == E_) {                   // if 4 decimals needed
      decdig = 4;
    }
    if (intexp > 0) {                       // if value > 10 (exp > 0)
      decloc += intexp;
    }
    else if (intexp < 0) {                  // if value < 1 (exp < 0)
      begzero = -intexp;                  // always 1 zero
    }
    // now move digits as counters indicate
    tmpPtr = ptrin1;
    ptrwrk = ptrout;                        // point to first char for output
    while (begzero) {                       // do leading zeros if needed
      if (i == decloc) {                  // if need decimal here do it
	intSize = i;                    // remember integer size
	*ptrwrk++ = '.';
	i++;
	continue;
      }
      else if (i > decloc) {
	decdig--;
      }
      i++;
      *ptrwrk++ = '0';
      begzero--;
    }
    while ((c = *tmpPtr++) != 'E') {        // do digits
      if (c == '.') {continue; }          // just skip the old decimal
      if (i == decloc) {                  // if need decimal here do it
	intSize = i;                    // remember integer size
	*ptrwrk++ = '.';
	i++;
	decdig--;                       // will also drop a digit
      }
      else if (i > decloc) {
	if (decdig == 0) {              // if no more needed
	  break;                      // number is complete
	}
	else if (decdig == 1) {         // need to round this one
	  if ((c != '9') && (*tmpPtr != 'E')) {// if rounding possible
	    if (*tmpPtr == '.') {   // if next is dot
	      if (*(tmpPtr+1) != 'E') {
		if (*(tmpPtr+1) > '4') {c++; } // round up this digit
	      }
	    }
	    else if (*tmpPtr > '4') {c++; } // round up this digit
	  }
	}
	decdig--;
      }
      *ptrwrk++ = c;
      i++;
    }
    while (i <= decloc) {                   // if not to decimal yet
      if (i == decloc) {                  // if need decimal here do it
	intSize = i;                    // remember integer size
	*ptrwrk++ = '.';
	*ptrwrk++ = '0';
	i += 2;
      }
      else {
	*ptrwrk++ = '0';
	i++;
      }
    }
    fullSize = i;
    *ptrwrk = '\0';                         // terminate the output
  }
  else {                                      // just move float to output
    str_cpy_c(ptrout, ptrin1);              // move whole string usually
    fullSize = cnt;
    intSize = dotdig - 1;
  }
  
  // Finally process input or data that is in floating format.  It must have a
  // decimal point to be here.  Clean off trailing zeros, like 35.000 or 35.600
  ptrwrk = ptrout+fullSize-1;                 // point to last char for backscan
  cntzero = 0;                                // initialize count
    while (*ptrwrk-- == '0') {cntzero++; }      // see how many trailing zeros
    ptrwrk++;                                   // look at non-zero char
    if (*ptrwrk == '.') {                       // if dot, this is an integer
      *ptrwrk = '\0';                         // cut number here
      fullSize = intSize;
      decSize = 0 + neg;                      // no decimal digits
    }
    else {                                      // really a float, if needed "round"
      // first drop trailing zeros after the decimal
      ptrwrk++;                               // move right to the zero
      if (cntzero) {                          // if something to drop
	*ptrwrk = '\0';                     // drop them
	fullSize -= cntzero;                // set new size
      }
      if (optFlag_ == E_) {                   // if 4 decimals needed
	maxSize = 5;                        // allow decimal too
      }
      else {                                  // must be normal mode and 2
	maxSize = 3;                        // allow decimal too
      }
      decSize = maxSize + neg;                // remember this for final size calc
      maxSize += intSize;                     // set desired size
      if (fullSize > maxSize) {               // if we still need to cut more
	ptrwrk = ptrout+maxSize;            // point to cut character
	c = *ptrwrk;                        // grab it
	*ptrwrk-- = '\0';                   // cut and move back for rounding
	fullSize = maxSize;
	// this rounding algorithm is crude, but the data is too, so it is good enough
	if ((c > '4') && (*ptrwrk != '9')) {// allow only one digit rounding
	  *ptrwrk = (*ptrwrk) + 1;        // increment the last digit
	}
	// now make sure we don't have all zeros again, eg it was not 2.000006
	ptrwrk = ptrout+fullSize-1;         // point to last char for backscan
	cntzero = 0;                        // initialize count
	while (*ptrwrk-- == '0') {cntzero++; } // see how many trailing zeros
	ptrwrk++;                           // look at non-zero char
	if (*ptrwrk == '.') {               // if dot, this is an integer again
	  *(ptrwrk+cntzero) = '1';        // show there was something
	}
      }
    }
    
    // add commas to a float
    maxIntDig = MUSERSP - (intSize-1)/3 - decSize; // calc max int digits we can expand
    if ((intSize > 3) && (intSize < maxIntDig)) {// if we need commas and there is space
      AddCommas (ptrout, intSize);
    }
    
    // finish up and exit
DONE:
    intSize += neg;                             // consider minus now
    fullSize = (Lng32)str_len(outStr);
    return;                                     // return nothing
}


/*************************************************************
Function Name: FormatFloat

Argument List:
    char *outStr[out]       storage for output, must be space for 20 char
    long &intSize[out]      reference to int part of output, 0=Exponential fmt
    long &fullSize[out]     ref to size of whole number field
    float  floatVal[in]     number to be formatted

Return Status: None

Class Data Accessed:
    optFlag_                = E_, 4 decimal digits desired, else 2

Description:
This function will convert the input float (4 byte floating-point number) to an
ASCII string in the user’s output buffer.  It will walk the string looking for
the decimal point and the full size.  Numbers >= 2.0E+9 are put in exponent
notation and intSize is 0.  Numbers < 0.0001 are displayed as 0.0001 always
(or if < 0.01 as 0.01 when optFlag_ == N_).  Zero is output as 0 (not 0.0), 
with intSize=0. Other numbers are displayed as floats with 2 or 4 digits of
accuracy depending on optFlag_.  Trailing zeros are removed after the decimal
point.  Negative numbers work, but are not expected.

This routine must avoid formats %E and %.4f that do not exist in the executor.
The format %04d tells it to zero pad with exactly 4 digits.
*****/
void ExExeUtilTcb::FormatFloat (char *outStr, Lng32 &intSize,
			 Lng32 &fullSize, double floatVal,
				NABoolean normalMode, NABoolean expertMode) const
{
  Lng32 neg = 0;               // input is negative if 1
  double absval = floatVal;    // absolute value of input
  Lng32 cntzero;               // count of trailing zeros
  char *ptr;                  // working variable for neg use
  Lng32 valInt = 0;            // integer part of float value
  Lng32 valDec = 0;            // decimal part of float value
  Lng32 valExp = 0;            // expontial part of float value
  float nbrDec;               // 10*(number_of_decimal_digits)
  float valRnd;               // value to add for rounding
  const char *fmtPtr;         // points to correct format string
  
  // handle some special cases
  ptr = outStr;                               // point to output area
  intSize = 0;                                // set base value
  if (floatVal < 0) {                         // if negative
    absval = -floatVal;                     // flip it for processing
    neg = 1;                                // remember this
    *ptr++ = '-';
    intSize++;
  }
  else if (floatVal == 0) {                   // if real zero
    str_cpy_c(ptr, "0");                    // drop decimal
    fullSize = 1;
    return;
  }
  // see if this is too large for decimal format, 1,999,999,999 is OK
  if (absval >= 2.0E9F) {                     // if exponential notation required
    while (absval >= 10) {                  // figure out the exponent value
      valExp++;
      absval /= 10;
    }
    nbrDec = 1000000.0F;                    // take 6 digits
    valRnd = 0.0000005F;
    absval += valRnd;                       // first round up if needed
    valInt = (Lng32)absval;                  // get the integer part
    absval -= valInt;
    valDec = (Lng32)(absval*nbrDec);         // get the decimal part as long
    str_sprintf (ptr, "%d.%06dE+%03d", valInt, valDec, valExp);
    intSize = 0;                            // force, even if neg
    goto DONE;                              // set fullSize and return
  }

  // if too small, use bottom value
  if ((absval < 1.0E-2) && (normalMode)) {// if too small for normal mode
    str_cpy_c(ptr, "0.01");
    intSize++;
    goto DONE;                              // set fullSize and return
  }
  if ((absval < 1.0E-4) && (expertMode)) {// if too small for expert mode
    str_cpy_c(ptr, "0.0001");
    intSize++;
    goto DONE;                              // set fullSize and return
  }
  // now it must be a valid decimal number
  if (expertMode) {                       // if 4 decimals needed
    nbrDec = 10000.0F;                      // take 4 digits
    valRnd = 0.00005F;
    fmtPtr = "%d.%04d";                     // request 4 decimal digits, zero padded
    cntzero = 4;                            // set count of digits
  }
  else {                                      // must be normal mode and 2
    nbrDec = 100.0F;                        // take 2 digits
    valRnd = 0.005F;
    fmtPtr = "%d.%02d";                     // request 2 decimal digits, zero padded
    cntzero = 2;                            // set count of digits
  }
  absval += valRnd;                           // first round up if needed
  valInt = (Lng32)absval;                      // get the integer part
  absval -= valInt;
  valDec = (Lng32)(absval*nbrDec);             // get the decimal part as long
  str_sprintf (ptr, fmtPtr, valInt, valDec);
  // got the number now, see what size the parts are
  while (*ptr++ != '.') {intSize++; }         // see how long.int part is (always a dot)
  
  // clean off trailing zeros, like 35.0000 or 35.60
  ptr += cntzero-1;                           // point to last char for backscan
  cntzero = 0;                                // initialize count for real
  while (*ptr-- == '0') {cntzero++; }         // see how many trailing zeros
  ptr++;                                      // look at non-zero char
  if (cntzero) {                              // if something to drop
    if (*ptr == '.') {                      // if dot, this is an integer
      *ptr = '\0';                        // cut number here
    }
    else {                                  // just remove trailing zeros
      ptr++;                              // move right to the zero
      *ptr = '\0';                        // drop them
    }
  }
  
  // add commas if needed
  if ((intSize > 3) && (intSize < 16)) {      // if we need commas and there is space
    if (neg) {                              // if minus in front
      intSize--;                          // count only digits
      AddCommas(outStr+1, intSize);
      intSize++;
    }
    else {
      AddCommas(outStr, intSize);
    }
  }
  
  // Finish up and exit
DONE:
  fullSize = (Lng32)str_len(outStr);
  return;                                     // return nothing
}


/*************************************************************
Function Name: AddCommas

Argument List:
    char *outStr[in/out]    number string to be modified, left justified
    long &intSize[in/out]   number of integer digits in the number input, size out

Return Status: None

Description:
This function will take the input ASCII number string and add commas every 3 digits.
It expects the character array input to be large enough to hold the additional
characters.  It will increment the count of characters in the integer part of the
input number.  It uses only the input count of integer digits and validates nothing.
It processes the digits from left to right and stops after doing the last 3 integer
digits without looking at the rest of the digits.  So it does not care about any
fractional part (except that it is null terminated).  Example input is 1234567.89
and output will be 1,234,567.89.  This routine will not work properly on negative
numbers, so don't call it.
*****/
void ExExeUtilTcb::AddCommas (char *outStr, Lng32 &intSize)const
{
    Int32 loc;                // character position to cut before, 0 base
    Int32 iter;               // iterations needed
    char temp[40];          // save cut data here

    // Begin processing
    if (intSize <= 3) {return;}                 // if digit count is 3 or less, just return

    // figure out how many iterations are needed
    loc = intSize % 3;                          // modulo 3 to see about leading digits
    iter = intSize/3;                           // no rounding
    if (loc == 0) {                             // if exact multiple of 3
        loc = 3;                                // first is not 0
        iter--;                                 // one less to do
    }

    // put one comma in on each loop
    while (iter-- > 0) {
        str_cpy_c(temp, outStr+loc);            // save tail of string
        *(outStr+loc) = ',';                    // insert the comma
        loc++;                                  // bump working count
        intSize++;                              // increment real integer size
        str_cpy_c(outStr+loc, temp);            // bring rest back
        loc += 3;                               // point to next cut
    }

    return;                                     // return nothing
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilDisplayExplainPrivateState::ExExeUtilDisplayExplainPrivateState()
{
  step_ = ExExeUtilDisplayExplainTcb::EMPTY_;
  matches_ = 0;
}

ExExeUtilDisplayExplainPrivateState::~ExExeUtilDisplayExplainPrivateState()
{
};

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilDisplayExplainComplexTdb::build(ex_globals * glob)
{
  ExExeUtilTcb * exe_util_tcb;

  if (isShowddl())
    exe_util_tcb = new(glob->getSpace()) ExExeUtilDisplayExplainShowddlTcb(*this, glob);
  else
    exe_util_tcb = new(glob->getSpace()) ExExeUtilDisplayExplainComplexTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilDisplayExplainTcb
///////////////////////////////////////////////////////////////
ExExeUtilDisplayExplainComplexTcb::ExExeUtilDisplayExplainComplexTcb(
     const ComTdbExeUtilDisplayExplainComplex & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob),
       step_(EMPTY_)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);
}

ExExeUtilDisplayExplainComplexTcb::~ExExeUtilDisplayExplainComplexTcb()
{
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilDisplayExplainComplexTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilDisplayExplainComplexPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

//////////////////////////////////////////////////////
// work() for ExExeUtilDisplayExplainComplexTcb
//////////////////////////////////////////////////////
short ExExeUtilDisplayExplainComplexTcb::work()
{
  Lng32 cliRC;
  short rc;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilDisplayExplainComplexPrivateState & pstate =
    *((ExExeUtilDisplayExplainComplexPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  while (1) // exit via return
    {
      switch (step_)
	{
	case EMPTY_:
	  {
	    step_ = IN_MEMORY_CREATE_;
	  }
	break;

	case IN_MEMORY_CREATE_:
	  {
	    // issue the create table/index command 
	    masterGlob->getStatement()->getContext()->
	      setSqlParserFlags(0x20000); // INTERNAL_QUERY_FROM EXEUTIL
	    cliRC = cliInterface()->executeImmediate(exeUtilTdb().qry1_);
	    masterGlob->getStatement()->getContext()->
	      resetSqlParserFlags(0x20000); // INTERNAL_QUERY_FROM EXEUTIL
	    if (cliRC < 0)
	      {
		// table could not be created.
		// If this is an error related to inMemory creation,
		// ignore it and continue with explain.
		// Explain will return 'info not available' in its
		// description field.
		// All other errors are reported.
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		if (exeUtilTdb().loadIfExists() &&
		     getDiagsArea()->contains(-1055))
		  {
		    SQL_EXEC_ClearDiagnostics(NULL);
		  }
		else
		  {

		    // Return error.
		    // Do not do 'DROP_AND_ERROR' as we don't want to drop
		    // an existing table.
		    ExRaiseSqlError(getHeap(), &diagsArea_,
				    EXE_NO_EXPLAIN_INFO, NULL);

		    step_ = ERROR_;
		    break;
		  }
	      }

	    // explain the create
	    step_ = EXPLAIN_CREATE_;
	  }
	break;

	case EXPLAIN_CREATE_:
	  {
	    cliRC = executeQuery(NULL, NULL, 
				 exeUtilTdb().qry2_,
				 FALSE, FALSE,
				 rc,
				 NULL, NULL, FALSE);

	    if (cliRC < 0)
	      {
		step_ = DROP_AND_ERROR_;
		break;
	      }
	    
	    if (cliRC == 1)
	      return rc;

	    if (exeUtilTdb().explainType_ == 
		ComTdbExeUtilDisplayExplainComplex::CREATE_TABLE_AS)
	      {
		// explain insert...select for CTAS

		if (exeUtilTdb().qry4_) 
		  {
		    // sidetree insert. Make the table unaudited.
		    step_ = ALTER_TO_NOAUDIT_;
		  }
		else
		  {
		    step_ = EXPLAIN_INSERT_SELECT_;
		  }
		break;
	      }

	    step_ = DROP_AND_DONE_;
	  }
	break;
	  
	case ALTER_TO_NOAUDIT_:
	  {
	    cliRC = changeAuditAttribute(exeUtilTdb().getObjectName(),
					 FALSE, exeUtilTdb().isVolatile());
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = DROP_AND_ERROR_;
		break;
	      }

	    step_ = EXPLAIN_INSERT_SELECT_;
	  }
	break;

	case EXPLAIN_INSERT_SELECT_:
	  {
	    // qry4_ is sidetree insert, qry3_ is vsbb insert.
	    cliRC = executeQuery(NULL, NULL, 
				 (exeUtilTdb().qry4_ ? exeUtilTdb().qry4_
				  : exeUtilTdb().qry3_),
				 FALSE, FALSE,
				 rc,
				 NULL, NULL, FALSE);
	    if (cliRC < 0)
	      {
		step_ = DROP_AND_ERROR_;
		break;
	      }
	    
	    if (cliRC == 1)
	      return rc;

	    step_ = DROP_AND_DONE_;
	  }
	break;

	case DROP_AND_DONE_:
	case DROP_AND_ERROR_:
	  {
	    char * dtQuery = 
	      new(getMyHeap()) char[strlen("DROP TABLE ; ") + 
				   strlen(exeUtilTdb().getObjectName()) +
				   100];
	    if (exeUtilTdb().explainType_ == 
		ComTdbExeUtilDisplayExplainComplex::CREATE_INDEX_)
	      strcpy(dtQuery, "DROP INDEX ");
	    else if (exeUtilTdb().explainType_ == 
		ComTdbExeUtilDisplayExplainComplex::CREATE_MV_)
	      strcpy(dtQuery, "DROP MV ");
	    else
	      strcpy(dtQuery, "DROP TABLE ");
	    strcat(dtQuery, exeUtilTdb().getObjectName());
	    cliRC = cliInterface()->executeImmediate(dtQuery);
	    if (cliRC < 0)
	      {
		// ignore errors.
		SQL_EXEC_ClearDiagnostics(NULL);
	      }
	    
	    // Delete new'd characters
	    NADELETEBASIC(dtQuery, getHeap());

	    if (step_ == DROP_AND_ERROR_)
	      step_ = ERROR_;
	    else
	      step_ = DONE_;
	  }
	break;
	
	case ERROR_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_SQLERROR;

	    ComDiagsArea *diagsArea = up_entry->getDiagsArea();
	    
	    if (diagsArea == NULL)
	      diagsArea = 
		ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
	    
	    if (getDiagsArea())
	      diagsArea->mergeAfter(*getDiagsArea());
	    
	    up_entry->setDiagsArea (diagsArea);
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    pstate.matches_ = 0;

	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    pstate.matches_ = 0;
	    step_ = EMPTY_;
	    qparent_.down->removeHead();

	    return WORK_OK;
	  }
	break;
	
	}
    }

}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilDisplayExplainComplexPrivateState::ExExeUtilDisplayExplainComplexPrivateState()
{
  matches_ = 0;
}

ExExeUtilDisplayExplainComplexPrivateState::~ExExeUtilDisplayExplainComplexPrivateState()
{
};

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilDisplayExplainShowddlTcb
///////////////////////////////////////////////////////////////
ExExeUtilDisplayExplainShowddlTcb::ExExeUtilDisplayExplainShowddlTcb(
     const ComTdbExeUtilDisplayExplainComplex & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob),
       step_(EMPTY_), newQry_(NULL)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);
}

ExExeUtilDisplayExplainShowddlTcb::~ExExeUtilDisplayExplainShowddlTcb()
{
  if (newQry_)
    NADELETEBASIC(newQry_, getMyHeap());
  newQry_ = NULL;
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilDisplayExplainShowddlTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilDisplayExplainShowddlPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

//////////////////////////////////////////////////////
// work() for ExExeUtilDisplayExplainShowddlTcb
//////////////////////////////////////////////////////
short ExExeUtilDisplayExplainShowddlTcb::work()
{
  Lng32 cliRC;
  short rc;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilDisplayExplainShowddlPrivateState & pstate =
    *((ExExeUtilDisplayExplainShowddlPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  while (1) // exit via return
    {
      switch (step_)
	{
	case EMPTY_:
	  {
	    step_ = GET_LABEL_STATS_;
	  }
	break;

	case GET_LABEL_STATS_:
	  {
	    char countBuf[20];

	    if (exeUtilTdb().noLabelStats())
	      {
		strcpy(countBuf, "0");
	      }
	    else
	      {
		cliRC = 
		  cliInterface()->
		  executeImmediate("control session 'EXPLAIN' 'ON';");

		if (cliRC < 0)
		  {
                    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		    step_ = ERROR_;
		    break;
		  }

	 Lng32 len = 0;
		cliRC = cliInterface()->executeImmediate(exeUtilTdb().qry3_,
							 countBuf,
							 &len, TRUE);

		// reset CONTROL SESSION
		cliInterface()->
		  executeImmediate("control session reset 'EXPLAIN';");

		if (cliRC < 0)
		  {
                    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		    step_ = ERROR_;
		    break;
		  }
		countBuf[len] = 0;
	      }

	    newQry_ = new(getMyHeap()) 
	      char[strlen(exeUtilTdb().qry2_) + 100];
	    str_sprintf(newQry_, exeUtilTdb().qry2_, countBuf);

	    step_ = EXPLAIN_CREATE_;
	  }
	break;

	case EXPLAIN_CREATE_:
	  {
	    cliRC = executeQuery(NULL, NULL, 
				 newQry_, //exeUtilTdb().qry2_,
				 FALSE, FALSE,
				 rc,
				 NULL, NULL, FALSE);

	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = ERROR_;
		break;
	      }
	    
	    if (cliRC == 1)
	      return rc;

	    step_ = DONE_;
	  }
	break;
	
	case ERROR_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_SQLERROR;

	    ComDiagsArea *diagsArea = up_entry->getDiagsArea();
	    
	    if (diagsArea == NULL)
	      diagsArea = 
		ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
	    
	    if (getDiagsArea())
	      diagsArea->mergeAfter(*getDiagsArea());
	    
	    up_entry->setDiagsArea (diagsArea);
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    pstate.matches_ = 0;

	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    pstate.matches_ = 0;
	    step_ = EMPTY_;
	    qparent_.down->removeHead();

	    return WORK_OK;
	  }
	break;
	
	}
    }

}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilDisplayExplainShowddlPrivateState::ExExeUtilDisplayExplainShowddlPrivateState()
{
  matches_ = 0;
}

ExExeUtilDisplayExplainShowddlPrivateState::~ExExeUtilDisplayExplainShowddlPrivateState()
{
};
