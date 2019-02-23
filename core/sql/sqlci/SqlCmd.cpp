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
 * File:         SqlCmd.C
 * RCS:          $Id: SqlCmd.cpp,v 1.1 2007/10/09 19:40:29  Exp $
 * Description:
 *
 *
 * Created:      4/15/95
 * Modified:     $ $Date: 2007/10/09 19:40:29 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include "Platform.h"

#include <ctype.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <unistd.h>

#include "ComAnsiNamePart.h"
#include "ComGuardianFileNameParts.h"
#include "ComASSERT.h"
#include "Formatter.h"
#include "SqlciStats.h"
#include "NAString.h"
#include "ErrorMessage.h"		// NAWriteConsole
#include "IntervalType.h"		// NAType::convertTypeToText etc.
#include "SqlciError.h"
#include "SQLCLIdev.h"
#include "sqlcmd.h"
#include "sql_id.h"
#include "ComSqlId.h"
#include "ComRtUtils.h"
#define CAT_MAX_HEADING_LEN 132
#define CM_GUA_ENAME_LEN 36
#include "dfs2rec.h"
#include "ex_error.h"
#include "str.h"
#include "stringBuf.h"
#include "charinfo.h"
#include "NLSConversion.h"
#include "nawstring.h"
#include "SqlciList_templ.h"
#include "ComCextMisc.h"
#include "ComCextdecs.h"
#include "conversionHex.h"

#include "ComQueue.h"
#include "ExExeUtilCli.h"

extern SqlciEnv * global_sqlci_env; // global sqlci_env for break key handling purposes.
extern ComDiagsArea sqlci_DA;
extern NAHeap sqlci_Heap;
extern BOOL WINAPI CtrlHandler (DWORD signalType);
const Lng32 BREAK_ERROR = -999;

const Int32  MAX_MSGTEXT_LEN = 4096;
const Int32  MAX_OTHERBUF_LEN = ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN+1+1;

// These should come from a SQL Message Text (immudefs) file! ##
#define UNLOADED_MESSAGE          "--- %ld row(s) unloaded."
#define LOADED_MESSAGE          "--- %ld row(s) loaded."
#define SELECTED_MESSAGE	"--- %d row(s) selected."
#define SELECTED_BUT_MESSAGE	"--- %d row(s) selected (but none displayed)."
#define SELECTED_FIRST_ROW_DISPLAY_MESSAGE "--- %d row(s) selected (and first row displayed)."
#define INSERTED_MESSAGE	"--- %ld row(s) inserted."
#define UPDATED_MESSAGE		"--- %ld row(s) updated."
#define DELETED_MESSAGE		"--- %ld row(s) deleted."
#define OP_COMPLETE_MESSAGE	"--- SQL operation complete."
#define OP_COMPLETED_ERRORS	"--- SQL operation failed with errors."
#define OP_COMPLETED_WARNINGS	"--- SQL operation completed with warnings."
#define PREPARED_MESSAGE	"--- SQL command prepared."
#define LISTCOUNT_MESSAGE	"  LIST_COUNT was reached."
#define CATAPI			"CREATE TANDEM_CAT_REQUEST&"


// These static variables and global functions, in conjunction with their
// callers, have as one goal to nicely format blank lines for sqlci output
// (output to screen and to logfile -- which should be identical):
// We want single blank lines to enhance legibility; we never want two
// blank non-data lines in a row.

static Lng32 worstcode;
static NABoolean lastLineWasABlank;

// remember the number of rows affected by an SQL statement
// before resetting the diagnostics info
static Int64 rowsAffected;

#define Succ_or_Warn	(retcode == SQL_Success || (retcode >= 0 && retcode != SQL_Eof))
void HandleCLIErrorInit()
{ 
  ComASSERT(SQL_Error < SQL_Warning && SQL_Warning < SQL_Eof);
  worstcode = SQL_Eof;
  lastLineWasABlank = FALSE;

}

void SqlCmd::clearCLIDiagnostics()
{
  SQLSTMT_ID dummy_stmt;
  SQLMODULE_ID module;
  init_SQLMODULE_ID(&module);

  init_SQLSTMT_ID(&dummy_stmt,
                  SQLCLI_CURRENT_VERSION,
                  stmt_handle,
                  &module
		 );

  SQL_EXEC_ClearDiagnostics(&dummy_stmt);
}

volatile Int32 breakReceived = 0;

void HandleCLIError(SQLSTMT_ID *stmt, Lng32 &error, SqlciEnv *sqlci_env,
		    NABoolean displayErr, NABoolean * isEOD,
                               Int32 prepcode)
{
  Int32 diagsCondCount = 0;
  if (error == 100)
     diagsCondCount = getDiagsCondCount(stmt); 
  // Get Warnings only when there are 2 or more conditions.
  // One condition is for the error code 100 and the others are the actual warnings
  NABoolean getWarningsWithEOF = (diagsCondCount > 1); 
  HandleCLIError(error, sqlci_env, displayErr, isEOD, prepcode, getWarningsWithEOF);
}

void HandleCLIError(Lng32 &error, SqlciEnv *sqlci_env,
		    NABoolean displayErr, NABoolean * isEOD,
                               Int32 prepcode, NABoolean getWarningsWithEOF)
{
  if (error == 100) 
  {
     if (isEOD != NULL)
        *isEOD = 1;
     if (! getWarningsWithEOF) {
        SqlCmd::clearCLIDiagnostics();
        return;
     }
  }
  if (isEOD)
    *isEOD = 0;

  if (error == BREAK_ERROR)
  {
    DWORD dwd = 0;
    CtrlHandler(dwd);
  }
  else
  #define MXCI_DONOTISSUE_ERRMSGS        	-1
 
  if (error != 0)
    {

      Logfile *log = sqlci_env->get_logfile();

      if (error != SQL_Eof) 
	worstcode = error;

      SQLMODULE_ID module;
      init_SQLMODULE_ID(&module);
      SQLDESC_ID cond_desc;
      init_SQLDESC_ID(&cond_desc, SQLCLI_CURRENT_VERSION, desc_handle,
		      &module);

      // Get the total number of conditions and # of rows affected

      Int32  total_conds = 0;
      Int64 rows_affected = 0;

      SQL_EXEC_AllocDesc(&cond_desc,2);

      SQL_EXEC_SetDescItem(&cond_desc, 1, SQLDESC_TYPE_FS,
				     REC_BIN32_SIGNED, 0);
      SQL_EXEC_SetDescItem(&cond_desc, 1, SQLDESC_VAR_PTR,
				     (Long) &total_conds, 0);
      SQL_EXEC_SetDescItem(&cond_desc, 2, SQLDESC_TYPE_FS,
				     REC_BIN64_SIGNED, 0);
      SQL_EXEC_SetDescItem(&cond_desc, 2, SQLDESC_VAR_PTR,
				     (Long) &rows_affected, 0);
      Lng32 stmt_items[2];
      stmt_items[0] = SQLDIAG_NUMBER;
      stmt_items[1] = SQLDIAG_ROW_COUNT;
      SQL_EXEC_GetDiagnosticsStmtInfo(stmt_items, &cond_desc);

      // remember # of rows affected in a global variable
      rowsAffected += rows_affected;

      // Attach msgtext variable with the output descriptor

      SQLMODULE_ID module2;
      init_SQLMODULE_ID(&module2);
      SQLDESC_ID msg_desc;
      init_SQLDESC_ID(&msg_desc, SQLCLI_CURRENT_VERSION, desc_handle,
		      &module2);
      
      SQLDIAG_COND_INFO_ITEM cond_info_item[4];
      short SQLCode;
      char msgtext[MAX_MSGTEXT_LEN];
      char otherbuf[MAX_OTHERBUF_LEN];
      NABoolean getCondInfo  = log->isVerbose();
      NABoolean showSQLSTATE = !!getenv("SHOW_SQLSTATE");

      short num_output_entries = 2;
      if (showSQLSTATE) 
	num_output_entries = 3;
      // total_conds can be 1 (Executor error) or 2 (Compile error, the second
      // sqlcode being 8822 "Unable to prepare the stmt").
      Lng32 specialErr =
        (ABS(error) == ABS(sqlci_env->specialError()) && total_conds <= 2) ?
			ABS(error) : 0;

      if (specialErr)
	{
	  ComASSERT(total_conds > 0);
	  total_conds = 1;
	  getCondInfo  = TRUE;
	  showSQLSTATE = FALSE;
	  num_output_entries = 3;
	  lastLineWasABlank = TRUE;
	}
      
      SQL_EXEC_AllocDesc(&msg_desc, num_output_entries);

      // Set up to receive Message text.
      SQL_EXEC_SetDescItem(&msg_desc, 1, SQLDESC_TYPE_FS,
				     REC_BYTE_F_ASCII, 0);
      SQL_EXEC_SetDescItem(&msg_desc, 1, SQLDESC_LENGTH,
				     MAX_MSGTEXT_LEN, 0);
      SQL_EXEC_SetDescItem(&msg_desc, 1, SQLDESC_VAR_PTR,
				     (Long) &msgtext, 0);
      cond_info_item[0].item_id = SQLDIAG_MSG_TEXT;
      cond_info_item[0].cond_number_desc_entry = 1;

      // set up to receive sqlcode
      SQL_EXEC_SetDescItem(&msg_desc, 2, SQLDESC_TYPE_FS,
				     REC_BIN16_SIGNED, 0);

      SQL_EXEC_SetDescItem(&msg_desc, 2, SQLDESC_VAR_PTR,
				     (Long) &SQLCode, 0);

      cond_info_item[1].item_id = SQLDIAG_SQLCODE;
      cond_info_item[1].cond_number_desc_entry = 2;

      //
      if (showSQLSTATE || specialErr)
      {
	SQL_EXEC_SetDescItem(&msg_desc, 3, SQLDESC_TYPE_FS,
				       REC_BYTE_F_ASCII, 0);
	SQL_EXEC_SetDescItem(&msg_desc, 3, SQLDESC_LENGTH,
				       MAX_OTHERBUF_LEN, 0);
	SQL_EXEC_SetDescItem(&msg_desc, 3, SQLDESC_VAR_PTR,
				       (Long) &otherbuf, 0);
	cond_info_item[2].item_id = SQLDIAG_RET_SQLSTATE;
	cond_info_item[2].cond_number_desc_entry = 3;

	if (specialErr)
	{
	  // "msgtext" is really cat-name; "otherbuf" will contain schema-name.
	  cond_info_item[0].item_id = SQLDIAG_CATALOG_NAME;
	  cond_info_item[2].item_id = SQLDIAG_SCHEMA_NAME;
	}
      }

      Int32 curr_cond = 1;
      SQL_EXEC_SetDescEntryCount(&cond_desc, 1);
      SQL_EXEC_SetDescItem(&cond_desc, 1, SQLDESC_VAR_PTR,
				     (Long) &curr_cond, 0);

      if (!(error == SQL_Success))
	
	{
	  if (total_conds)
	    {
	      // Start with a blank line -- cf. NADumpDiags(GetErrorMessage.C)
	      if (!lastLineWasABlank)
		{
		  SQL_EXEC_GetDiagnosticsCondInfo(cond_info_item,
						  &cond_desc, &msg_desc);
		  if ((SQLCode != 100) && 
		      ((SQLCode < 0) || (SQLCode > 0 && getCondInfo )))
		    {
		      if (displayErr) // || (SQLCode > 0))
			{
			  log->WriteAll("");
			  // The error text(s) we are about to emit 
			  // each have two lines,
			  // the second of which is a blank.
			  lastLineWasABlank = TRUE;
			}
		    }
		}
	    }
	  
	// Loop through total number of conditions, and print out
	// the error message text.  This loop should emulate
	// NADumpDiags (GetErrorMessage.C) as much as possible.
	for (; curr_cond <= total_conds; curr_cond++)
	  {
	    SQL_EXEC_GetDiagnosticsCondInfo(cond_info_item,
				       &cond_desc, &msg_desc);
	    
	    // do not output no data warning 100.
            // do not output warning message if it's printed during preparation
            // on Linux.
	    if ((SQLCode == 100) || (SQLCode == prepcode))
	      {
		if ((isEOD) && (SQLCode == 100)) // 100: SQL_Eof
		  *isEOD = 1;
		continue;
	      } 

	    //if 'set warnings off' set
	    if ((SQLCode < 0) || (SQLCode > 0 && getCondInfo ))
	      {
		
		if (msgtext[0] == '\0') // an empty message
		  {
		    strcpy(msgtext, "*** ERROR[15000] Unexpected Error: Message text missing");
		  }
		CharInfo::CharSet TCS = sqlci_env->getTerminalCharset();
		CharInfo::CharSet ISOMAPCS = sqlci_env->getIsoMappingCharset();

		if(
		   TCS != CharInfo::UTF8/*msgcharset*/
		   )
		{
		  charBuf cbuf((unsigned char*)msgtext, strlen(msgtext));
		  NAWcharBuf* wcbuf = 0;
		  Int32 errorcode	= 0;
		  wcbuf = csetToUnicode(cbuf, 0, wcbuf, CharInfo::UTF8/*msgcharset*/, errorcode);
		  NAString* tempstr;
		  if (errorcode == 0){				
		    tempstr = unicodeToChar(wcbuf->data(),wcbuf->getStrLen(), TCS, NULL, TRUE);
		    TrimNAStringSpace(*tempstr, FALSE, TRUE);  // trim trailing blanks
		    strcpy(msgtext, tempstr->data());
		  }
		}
		
		if (specialErr && sqlci_env->specialHandler())
		  {
		    sqlci_env->specialHandler()(sqlci_env, error, msgtext, otherbuf);
		  }
		else
		  {
		    // ## Kludge for "error" 20109.  Internationalization problem...!
		    const char *outtext = msgtext;
		    size_t pfxl = strlen("*** ERROR[20109] ");
		    
#if defined(USE_WCHAR)
		    if (wcsncmp(msgtext, L"*** ERROR[20109] ", pfxl) == 0)
#else
		      if (strncmp(msgtext,  "*** ERROR[20109] ", pfxl) == 0)
#endif
			outtext += pfxl;
		    
#ifdef USE_WCHAR
		    if (showSQLSTATE)
		      {
			$$do something here$$
		      }
		    if (displayErr)// || (SQLCode > 0))
		      log->WriteAll((NAWchar*)outtext, NAWstrlen((NAWchar*)outtext));
		    #else			// not wide, so don't use NAWxxx
		    //
		    // Rather than returning from the first executable statements in
		    // HandleCLIError, a couple of breakReceived conditional checks are
		    // performed to suppress the outputting of error messages after a
		    // break key was encountered. This allows MXCI to have more control
		    // of issuing (future break processing enhancements) warning and
		    // error messages during break key processing.
		    if (! breakReceived)
		      {
			if (showSQLSTATE)
			  {
			    if (displayErr)// || (SQLCode > 0))
			      {
				log->WriteAllWithoutEOL("*** SQLSTATE: ");
				log->WriteAll(otherbuf);
			      }
			  }
			if (displayErr)// || (SQLCode > 0))
			  log->WriteAll(outtext);
                        
			if (sqlci_env->specialError() ==
                            MXCI_DONOTISSUE_ERRMSGS)
                        {
                          sqlci_env->resetSpecialError();
                          curr_cond = curr_cond + total_conds;
                        }
			
		      } // breakReceived
#endif
		    // write out a blank line after error message
		    if (! breakReceived)
		      {
			if (displayErr)// || (SQLCode > 0))
			  log->WriteAll("");
		      } // breakReceived
		  }
	      }
	  } // for
      }

      if (displayErr)// || (SQLCode > 0))
	SqlCmd::clearCLIDiagnostics();

      SQL_EXEC_DeallocDesc(&cond_desc);
      SQL_EXEC_DeallocDesc(&msg_desc);
      //      delete (cond_desc);
      //      delete (msg_desc);
  } // error != 0

  // Check if Ctrl+break occured... nk
  if ( sqlci_env->diagsArea().mainSQLCODE() == SQLCI_BREAK_RECEIVED)
    error = SQL_Canceled;

} // HandleCLIError


void handleLocalError(ComDiagsArea *diags, SqlciEnv *sqlci_env)
{
  Logfile *log = sqlci_env->get_logfile();

  // Here the variable worstcode has to be set to SQL_Error in 
  // case of an error and SQL_Warning in case of a warning.
  // which might have been caused due to a param processing error/warning.
  // Usually this variable gets set in the HandleCLIError() function,
  // when HandleCLIError() is called with a error after a CLI call.
  // Soln :10-021203-3433

  if (diags->getNumber(DgSqlCode::ERROR_)) {
     worstcode = SQL_Error;
  }
  else if (diags->getNumber(DgSqlCode::WARNING_)) {
    worstcode = SQL_Warning;
  }

  if (!lastLineWasABlank) log->WriteAllWithoutEOL("");
  lastLineWasABlank = TRUE;

  ostringstream errMsg;
  NADumpDiags(errMsg, diags, TRUE/*newline*/, 0, NULL, log->isVerbose(),
              sqlci_env->getTerminalCharset());

  errMsg << ends;

  log->WriteAllWithoutEOL(errMsg.str().c_str());
}

Int64 getRowsAffected(SQLSTMT_ID *stmt)
{
   Int32 rc;
   rc = SQL_EXEC_GetDiagnosticsStmtInfo2(stmt,
                           SQLDIAG_ROW_COUNT, &rowsAffected,
                           NULL, 0, NULL);
   if (rc == 0)
      return rowsAffected; 
   else
      return -1;
}

Int32 getDiagsCondCount(SQLSTMT_ID *stmt)
{
   Int32 rc;
   Int32 diagsCondCount;
   rc = SQL_EXEC_GetDiagnosticsStmtInfo2(stmt,
                           SQLDIAG_NUMBER, &diagsCondCount,
                           NULL, 0, NULL);
   if (rc >= 0)
      return diagsCondCount; 
   else
      return 0;
}

static char * upshiftStr(char * inStr, char * outStr, UInt32 len)
{
  for (UInt32 i = 0; i < len; i++)
    {
      outStr[i] = toupper(inStr[i]);
    }

  return outStr;
}

char * SqlCmd::replacePattern(SqlciEnv * sqlci_env, char * str)
{
  if (str == NULL)
    return NULL;

  UInt32 len = strlen(str);

  char upperStr[20];
  // if SET PATTERN or RESET PATTERN, do not replace pattern.
  UInt32 s = 0;
  if (len >= strlen("SET PATTERN"))
    {
      if (strncmp(upshiftStr(str, upperStr, 3), "SET", 3) == 0)
	s = 3;
    }
  else if (len >= strlen("RESET PATTERN"))
    {
      if (strncmp(upshiftStr(str, upperStr, 5), "RESET", 5) == 0)
	s = 5;
    }
  if (s > 0)  // SET or RESET found
    {
      // skip blanks
      while ((s <= len) &&
	     (str[s] == ' '))
	s++;

      if (s < len)
	{
	  if ((len - s) >= strlen("PATTERN"))
	    {
	      if (strncmp(upshiftStr(&str[s], upperStr, 7), "PATTERN", 7) == 0)
		return str;
	    }
	}
    }
      
  // preprocess the argument and replace the pattern: <definename >
  // (< and > are included) with the value of define/env-var "definename".
  enum State { CONSUME_CHAR, QUOTE_SEEN,
	       LPATTERN_SEEN, RPATTERN_SEEN };

  UInt32 outstr_len = 500 + len;
  char * outstr = new char[outstr_len];
  char * patternText  = new char[100];

  UInt32 i = 0;
  UInt32 j = 0;
  UInt32 k = 0;
  NABoolean skipChar = FALSE;
  NABoolean inSingleQuote = FALSE;
  State state = CONSUME_CHAR;
  NABoolean patternSeen = FALSE;
  while (i <= len)
    {
      skipChar = TRUE;
      switch (state)
	{
	case CONSUME_CHAR:
          if (str[i] == '\'')
          {
            state = QUOTE_SEEN;
            inSingleQuote=TRUE;
          }
	  if (str[i] == '"')
          {
	    state = QUOTE_SEEN;
            inSingleQuote = FALSE;
          }
	  else if ((str[i] == '$') && ((i+1) < len) && (str[i+1] == '$'))
	    {
	      i+=2;
	      state = LPATTERN_SEEN;
	      skipChar = FALSE;
	    }
	  break;

	case QUOTE_SEEN:
          if (str[i] == '\'')
          {
            // If in a single quote, make sure this quote is the true end
            if (inSingleQuote) 
            {
              // If string contains two single quotes, can skip - not the end
              if (((i+1) < len) && (str[i+1] == '\''))
              {
                outstr[k] = str[i]; // skip over quote
                i++;
                k++;
              }
              // The quote mark actually signifies the end of the quote
              else
                state = CONSUME_CHAR;
            }
            // else: Double quote found, so continue, not the end
          }

	  if (str[i] == '"')
          {
            // If in a double quote, make sure this quote is the true end
            if (!inSingleQuote)
            {
              //If string contains two double quotes, can skip - not the end
              if (((i+1) < len) && (str[i+1] == '\''))
              {
                outstr[k] = str[i]; // skip over quote
                i++;
                k++;
              }

              // The double quote mark signifies the end of the quote
              else
	        state = CONSUME_CHAR;
            }
            // else: single quote found, so continue - not the end
          }
	  break;

	case LPATTERN_SEEN:
	  patternSeen = TRUE;
	  if ((str[i] == '$') && ((i+1) < len) && (str[i+1] == '$'))
	    {
	      i+=2;
	      state = RPATTERN_SEEN;
	    }
	  else
	    {
	      patternText[j++] = str[i];
	      i++;
	    }
	  skipChar = FALSE;
	  break;

	case RPATTERN_SEEN:
	  // find value of pattern and replace
	  patternText[j] = '\0';
	  Param * pattern
	    = sqlci_env->get_patternlist()->get(patternText);

	  char * patternValue = NULL;
	  if (pattern == NULL)
	    {
	      // if an env var is present with this name, use it
	      patternValue = getenv(patternText);
	    }
	  else
	    {
	      patternValue = pattern->getValue();
	    }


	  if (patternValue)
	    {
	      strncpy((char *)&outstr[k], patternValue, strlen(patternValue));
	      k += strlen(patternValue);

              //special note: the total length of outstr should be smaller
              //than outstr_len.  if the condition below becomes false,
              //increase outstr_len at the top of the function
              ComASSERT(k <= outstr_len);
	    }

	  j = 0;
          skipChar = FALSE;
	  state = CONSUME_CHAR;
	  break;

	}
      if (skipChar)
	{
	  outstr[k] = str[i];
	  i++;
	  k++;
	}
    } // while

  outstr[k] = '\0';

  delete [] patternText;
  if (NOT patternSeen)
    {
      delete [] outstr;
      return str;
    }
  else
    return outstr;
}

SqlCmd::SqlCmd(const sql_cmd_type cmd_type_, const char * argument_)
                  : SqlciNode(SqlciNode::SQL_CMD_TYPE),
		    cmd_type(cmd_type_)
{
  if (argument_)
    {
      sql_stmt = new char[strlen(argument_)+1];
      strcpy(sql_stmt, argument_);
    }
  else
    sql_stmt = 0;
}

SqlCmd::~SqlCmd()
{
  if (sql_stmt)
    delete [] sql_stmt;
}

short SqlCmd::showShape(SqlciEnv *sqlci_env, const char *query)
{
  if ((!query) ||
      (strncmp(query, "SHOWSHAPE", 9) == 0))
    return 0;

  char * buf = new char[strlen("SHOWSHAPE ") + strlen(query) + 1];
  strcpy(buf, "SHOWSHAPE ");
  strcat(buf, query);

  DML dml(buf, DML_SHOWSHAPE_TYPE, "__SQLCI_DML_SHOWSHAPE__");

  short retcode = dml.process(sqlci_env);
  delete buf;
  if (retcode)
    return retcode;

  return 0;
}

short SqlCmd::updateRepos(SqlciEnv * sqlci_env, SQLSTMT_ID * stmt, char * queryId)
{
 Lng32 retcode = 0;

  // get explain fragment.
  Lng32 explainDataLen = 50000; // start with 50K bytes
  Int32 retExplainLen = 0;
  char * explainData = new char[explainDataLen + 1];
  retcode = SQL_EXEC_GetExplainData(stmt,
                                    explainData, explainDataLen+1, 
                                    &retExplainLen
                                    );
  if (retcode == -CLI_GENCODE_BUFFER_TOO_SMALL)
    {
      delete explainData;

      explainDataLen = retExplainLen;
      explainData = new char[explainDataLen + 1];

      SqlCmd::clearCLIDiagnostics();
      retcode = SQL_EXEC_GetExplainData(stmt,
                                        explainData, explainDataLen+1, 
                                        &retExplainLen
                                        );
    }
  
  if (retcode < 0)
    {
      delete explainData;

      HandleCLIError(retcode, sqlci_env);

      return retcode;
    }

  explainDataLen = retExplainLen;

  // update repository
  ExeCliInterface cliInterface;
  
  SQL_EXEC_SetParserFlagsForExSqlComp_Internal(0x20000);

  char * queryBuf = new char[4000];

  Int64 ts = NA_JulianTimestamp();
  str_sprintf(queryBuf, "insert into %s.\"%s\".%s (instance_id, tenant_id, host_id, exec_start_utc_ts, query_id, explain_plan) values (0,0,0, CONVERTTIMESTAMP(%ld), '%s', '' ) ",
              TRAFODION_SYSCAT_LIT, SEABASE_REPOS_SCHEMA, REPOS_METRIC_QUERY_TABLE,
              ts, queryId);

  retcode = cliInterface.executeImmediatePrepare(queryBuf);
  if (retcode < 0)
    {
      HandleCLIError(retcode, sqlci_env);

      goto label_return;
    }

  retcode = cliInterface.clearExecFetchClose(explainData, explainDataLen);
  if (retcode < 0)
    {
      HandleCLIError(retcode, sqlci_env);

      goto label_return;
    }

  retcode = SQL_EXEC_StoreExplainData(
                                      &ts, queryId,
                                      explainData, explainDataLen);
  if (retcode < 0)
    {
      HandleCLIError(retcode, sqlci_env);

      goto label_return;
    }
 
 label_return:
  SQL_EXEC_ResetParserFlagsForExSqlComp_Internal(0x20000);

  delete explainData;
  delete queryBuf;

  return retcode;
}

short SqlCmd::cleanupAfterError(Lng32 retcode,
                                SqlciEnv * sqlci_env,
                                SQLSTMT_ID * stmt,
                                SQLDESC_ID *sql_src,
                                SQLDESC_ID *output_desc,
                                SQLDESC_ID *input_desc,
                                NABoolean resetLastExecStmt)
{
  // if retcode < 0, it is an error.
  // Clean up and return.
  if (retcode < 0)
    {
      SQL_EXEC_DeallocDesc(input_desc);
      SQL_EXEC_DeallocDesc(output_desc);
      SQL_EXEC_DeallocDesc(sql_src);
      SQL_EXEC_DeallocStmt(stmt);
      if (global_sqlci_env->getDeallocateStmt())
        global_sqlci_env->resetDeallocateStmt();
      delete (SQLMODULE_ID*)output_desc->module;
      delete (SQLMODULE_ID*)input_desc->module;
      delete (SQLMODULE_ID*)sql_src->module;
      delete (SQLMODULE_ID*)stmt->module;
      delete input_desc;
      delete output_desc;
      delete sql_src;
      delete [] stmt->identifier;
      delete stmt;
      if (resetLastExecStmt)
        sqlci_env->lastExecutedStmt() = NULL;

      SqlCmd::clearCLIDiagnostics();

      return (short)SQL_Error; 
    }
  
  return 0;
}

short SqlCmd::do_prepare(SqlciEnv * sqlci_env,
			 PrepStmt * prep_stmt,
			 char * sqlStmt,
			 NABoolean resetLastExecStmt,
                         Lng32 rsIndex,
                         Int32 *prepcode,
                         Lng32 *statisticsType)
{
  if (sqlci_env->showShape())
    {
      showShape(sqlci_env, sqlStmt);
    }

  SQLSTMT_ID *stmt = new SQLSTMT_ID;
  SQLDESC_ID *sql_src = new SQLDESC_ID;
  SQLDESC_ID *output_desc = new SQLDESC_ID;
  SQLDESC_ID *input_desc = new SQLDESC_ID;

  memset (stmt, 0, sizeof(SQLSTMT_ID));
  memset (sql_src, 0, sizeof(SQLDESC_ID));
  memset (output_desc, 0, sizeof(SQLDESC_ID));
  memset (input_desc, 0, sizeof(SQLDESC_ID));

  SQLMODULE_ID * module = new SQLMODULE_ID;
  stmt->module = module;
  init_SQLMODULE_ID(module);

  module = new SQLMODULE_ID;
  sql_src->module = module;
  init_SQLMODULE_ID(module);

  module = new SQLMODULE_ID;
  input_desc->module = module;
  init_SQLMODULE_ID(module);

  module = new SQLMODULE_ID;
  output_desc->module = module;
  init_SQLMODULE_ID(module);

  SqlCmd::clearCLIDiagnostics();

  Lng32 retcode = 0;

  // replace any user defined pattern in the sql query
  //  char * str = replacePattern(sqlci_env, sqlStmt);
  char * str = sqlStmt;

  prep_stmt->set(str, NULL, stmt, 0, NULL, 0, NULL);

  char *stmtName = prep_stmt->getStmtName();

  // Bookkeeping for stored procedure result sets
  NABoolean isResultSet = (rsIndex > 0 ? TRUE : FALSE);
  NABoolean skipPrepare = isResultSet;
  if (!isResultSet)
    HandleCLIErrorInit();

  /* allocate a statement */
  stmt->version = SQLCLI_CURRENT_VERSION;
  stmt->name_mode = stmt_name;
  char * identifier = new char[strlen(stmtName) + 1];
  stmt->identifier_len = strlen(stmtName);
  str_cpy_all(identifier,stmtName, stmt->identifier_len);
  identifier[stmt->identifier_len] = 0;
  stmt->identifier = identifier;
  stmt->handle = 0;

  if (!isResultSet)
  {
    retcode = SQL_EXEC_AllocStmt(stmt, 0);
    HandleCLIError(retcode, sqlci_env);
  }
  else
  {
    SQLSTMT_ID callStmt;
    SQLMODULE_ID module;
    init_SQLMODULE_ID(&module);
    init_SQLSTMT_ID(&callStmt, SQLCLI_CURRENT_VERSION, stmt_name, &module);

    char *callStmtName = sqlci_env->lastExecutedStmt()->getStmtName();
    callStmt.identifier_len = (Lng32) strlen(callStmtName);
    callStmt.identifier = callStmtName;

    retcode = SQL_EXEC_AllocStmtForRS(&callStmt, rsIndex, stmt);
    
    // Statement allocation may have failed simply because the RS
    // index is out of range or because the parent statement is not a
    // CALL. That can be tolerated and in response we are going to
    // return early.
    if (retcode == -8909 ||  // Parent stmt is not a CALL
        retcode == -8916)    // RS index out of range
    {
      SqlCmd::clearCLIDiagnostics();
    }
    else
    {
      HandleCLIError(retcode, sqlci_env);
    }
  }

  // Statement allocation failed. If errors needed to be reported to
  // the console, that has already happened. We can cleanup and
  // return.
  if (retcode < 0)
  {
    return cleanupAfterError(retcode, sqlci_env, stmt, sql_src, 
                             output_desc, input_desc, resetLastExecStmt);
  }

  if (!isResultSet)
  {
    global_sqlci_env->setDeallocateStmt();
    global_sqlci_env->setLastAllcatedStmt(stmt);
  }

  /* allocate a descriptor which will hold the sql statement source */
  sql_src->version = SQLCLI_CURRENT_VERSION;
  sql_src->name_mode = desc_handle;
  sql_src->identifier = 0;
  sql_src->identifier_len = 0;
  sql_src->handle = 0;

  retcode = SQL_EXEC_AllocDesc(sql_src, 1);
  HandleCLIError(retcode, sqlci_env);

  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_TYPE_FS,
                                 REC_BYTE_V_ANSI, 0);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_CHAR_SET_NAM,
                                 0, (char*)CharInfo::getCharSetName(sqlci_env->getTerminalCharset()));
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR, (Long)str, 0);
  HandleCLIError(retcode, sqlci_env);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH, strlen(str) + 1, 0);
  HandleCLIError(retcode, sqlci_env);

  SQL_QUERY_COST_INFO queryCostInfo;
  SQL_QUERY_COMPILER_STATS_INFO queryCompStatsInfo;
  char uniqueQueryIdBuf[400];
  Lng32 uniqueQueryIdLenBuf = 399;
  
  char * uniqueQueryIdPtr = uniqueQueryIdBuf;
  Lng32 * uniqueQueryIdLenPtr = &uniqueQueryIdLenBuf;

  if (!skipPrepare)
  {
    /* prepare it */
    // Get cost and set unique query id as well
    strcpy(uniqueQueryIdBuf, "    ");

    ULng32 prepFlags = 0;
    if ((stmt->identifier_len > 0) &&
	(strncmp(stmt->identifier, "__SQLCI_DML_", strlen("__SQLCI_DML_")) == 0))
      {
	prepFlags |=PREPARE_STANDALONE_QUERY;

      }
    prepFlags |=PREPARE_USE_EMBEDDED_ARKCMP; // mxosrvr needs to set this too.
    retcode = SQL_EXEC_Prepare2(stmt, sql_src,
				NULL, 0, NULL,
				&queryCostInfo,
				&queryCompStatsInfo,
				uniqueQueryIdPtr,
				uniqueQueryIdLenPtr,
				prepFlags
				);
    
    // save returned query id in prep_stmt
    if (prep_stmt->uniqueQueryId())
      {
	delete prep_stmt->uniqueQueryId();
	prep_stmt->uniqueQueryIdLen() = 0;
      }
    
    if (*uniqueQueryIdLenPtr > 0)
      {
	prep_stmt->uniqueQueryIdLen() = *uniqueQueryIdLenPtr;

	prep_stmt->uniqueQueryId() = new char[prep_stmt->uniqueQueryIdLen() + 1];
	memcpy(prep_stmt->uniqueQueryId(), uniqueQueryIdPtr, 
	       prep_stmt->uniqueQueryIdLen());
	prep_stmt->uniqueQueryId()[prep_stmt->uniqueQueryIdLen()] = 0;
      }
    
     if (prepcode  != NULL)
      *prepcode = retcode;
    HandleCLIError(retcode, sqlci_env);

      // if SQL_EXEC_Prepare returned a value < 0, it is an error.
    // Clean up and return.
    if (retcode < 0)
    {
      return cleanupAfterError(retcode, sqlci_env, stmt, sql_src, 
                               output_desc, input_desc, resetLastExecStmt);
    }
  } // if (!skipPrepare)

  // Statistics Collection Type
  if (statisticsType != NULL)
    *statisticsType = queryCompStatsInfo.statsCollectionType;
  delete (SQLMODULE_ID *)input_desc->module;
  delete (SQLMODULE_ID *)output_desc->module;
  input_desc->module = output_desc->module = NULL;

  // find the statement type
  if (!skipPrepare)
  {
    retcode = SQL_EXEC_GetStmtAttr(stmt, SQL_ATTR_QUERY_TYPE, 
  				   &prep_stmt->queryType(), 
  				   NULL, 0, NULL);
    HandleCLIError(retcode, sqlci_env);
    Lng32 subqueryType;

    retcode = SQL_EXEC_GetStmtAttr(stmt, SQL_ATTR_SUBQUERY_TYPE,
      &subqueryType, NULL, 0, NULL);
    HandleCLIError(retcode, sqlci_env);
    prep_stmt->setSubqueryType(subqueryType);
   }
  else if (isResultSet)
    prep_stmt->queryType() = SQL_SP_RESULT_SET;
				 
  init_SQLDESC_ID(input_desc, SQLCLI_CURRENT_VERSION, desc_handle);
  init_SQLDESC_ID(output_desc, SQLCLI_CURRENT_VERSION, desc_handle);

  /* allocate an input descriptor to send values. */
  retcode = SQL_EXEC_AllocDesc(input_desc, 500);
  HandleCLIError(retcode, sqlci_env);

  /* allocate an output descriptor to retrieve values. */
  retcode = SQL_EXEC_AllocDesc(output_desc, 500);
  HandleCLIError(retcode, sqlci_env);

  // An undocumented environment variable can be used to select WIDE
  // descriptors. A WIDE descriptor is only meaningful for a CALL
  // statement and describes all input and output values together in a
  // single descriptor.
  if (getenv("SQLCI_DESCRIBE_WIDE"))
  {
    SQL_EXEC_SetDescItem(input_desc, 1, SQLDESC_DESCRIPTOR_TYPE,
                         DESCRIPTOR_TYPE_WIDE, 0);
    SQL_EXEC_SetDescItem(output_desc, 1, SQLDESC_DESCRIPTOR_TYPE,
                         DESCRIPTOR_TYPE_WIDE, 0);
  }

  /* describe the input/output entries into the input/output descriptor */
  Lng32 num_input_entries = 0, num_output_entries = 0;
  retcode = SQL_EXEC_DescribeStmt(stmt, input_desc, output_desc);
  if (retcode < 0)
  {
    // This is a stored procedure result set without a valid query
    // plan. It could be that we just went beyond the end of the
    // collection of results for the current CALL statement. That is
    // OK and is not an error to report to the console. It just means
    // our result set processing should end so we cleanup and
    // return. If some other error was encountered, that is unexpected
    // and we will report it to the user, then cleanup and return.

    if (isResultSet && retcode == -8915)
      SqlCmd::clearCLIDiagnostics();
    else
      HandleCLIError(retcode, sqlci_env);

    SqlCmd::clearCLIDiagnostics();

    return cleanupAfterError(retcode, sqlci_env, stmt, sql_src, 
                             output_desc, input_desc, resetLastExecStmt);
  }
  else if (retcode != 8818)
  {
    HandleCLIError(retcode, sqlci_env);
  }
  else
  {
    SqlCmd::clearCLIDiagnostics();
    
    retcode = SQL_EXEC_GetDescEntryCount(input_desc, &num_input_entries);
    HandleCLIError(retcode, sqlci_env);
    
    retcode = SQL_EXEC_GetDescEntryCount(output_desc, &num_output_entries);
    HandleCLIError(retcode, sqlci_env);
    
    if (num_input_entries > 500)
    {
      retcode = SQL_EXEC_DeallocDesc(input_desc);
      HandleCLIError(retcode, sqlci_env);
      
      retcode = SQL_EXEC_AllocDesc(input_desc, num_input_entries);
      HandleCLIError(retcode, sqlci_env);
      
      retcode = SQL_EXEC_DescribeStmt(stmt, input_desc, NULL);
      HandleCLIError(retcode, sqlci_env);
    }
    
    if (num_output_entries > 500)
    {
      retcode = SQL_EXEC_DeallocDesc(output_desc);
      HandleCLIError(retcode, sqlci_env);
      
      retcode = SQL_EXEC_AllocDesc(output_desc, num_output_entries);
      HandleCLIError(retcode, sqlci_env);
      
      retcode = SQL_EXEC_DescribeStmt(stmt, NULL, output_desc);
      HandleCLIError(retcode, sqlci_env);
    }
  }
  
  retcode = SQL_EXEC_GetDescEntryCount(input_desc, &num_input_entries);
  HandleCLIError(retcode, sqlci_env);

  retcode = SQL_EXEC_GetDescEntryCount(output_desc, &num_output_entries);
  HandleCLIError(retcode, sqlci_env);
  if (breakReceived)
  {
    delete sql_src;
    delete input_desc;
    delete output_desc;
    return SQL_Canceled;
  }
  prep_stmt->set(str, sql_src, stmt,
		 num_input_entries, input_desc,
		 num_output_entries, output_desc);

  addOutputInfoToPrepStmt(sqlci_env, prep_stmt);

  if (resetLastExecStmt)
  {
    sqlci_env->lastExecutedStmt() = NULL;
    sqlci_env->lastDmlStmtStatsType() = SQLCLIDEV_NO_STATS;
  }
  
  return 0;
}

void SqlCmd::addOutputInfoToPrepStmt(SqlciEnv *sqlci_env,
                                     PrepStmt *prep_stmt)
{
  if (!prep_stmt)
    return;

  Lng32 num_output_entries = prep_stmt->numOutputEntries();
  SQLDESC_ID *output_desc = prep_stmt->getOutputDesc();

  if (num_output_entries > 0)
  {
    Lng32 retcode = 0;

    // space where actual data is returned
    char * output_data = 0;
    Lng32 output_data_len = 0;
    Lng32 output_buflen = 0;
    
    Lng32 datatype = 0;
    Lng32 length = 0;
    Lng32 precision = 0;
    Lng32 scale = 0;
    Lng32 null_flag = 0;
    Lng32 charsetEnum = 0;
    
    Lng32 curpos = 0;
    
    prep_stmt->outputEntries() = new PrepEntry * [num_output_entries];
    
    short entry=1;
    for (; entry <= num_output_entries; entry++)
    {
      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                     SQLDESC_TYPE_FS,
                                     &datatype, 0, 0, 0, 0);
      HandleCLIError(retcode, sqlci_env);
      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                     SQLDESC_OCTET_LENGTH,
                                     &length, 0, 0, 0, 0);
      HandleCLIError(retcode, sqlci_env);
      
      if (datatype >= REC_MIN_INTERVAL && datatype <= REC_MAX_INTERVAL) {
        retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                       SQLDESC_INT_LEAD_PREC,
                                       &precision, 0, 0, 0, 0);
        HandleCLIError(retcode, sqlci_env);
        retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                       SQLDESC_PRECISION,
                                       &scale, 0, 0, 0, 0);
        HandleCLIError(retcode, sqlci_env);
      } else if (datatype == REC_DATETIME) {
        retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                       SQLDESC_DATETIME_CODE,
                                       &precision, 0, 0, 0, 0);
        HandleCLIError(retcode, sqlci_env);
        retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                       SQLDESC_PRECISION,
                                       &scale, 0, 0, 0, 0);
        HandleCLIError(retcode, sqlci_env);
      }  else {
        retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                       SQLDESC_PRECISION,
                                       &precision, 0, 0, 0, 0);
        HandleCLIError(retcode, sqlci_env);
        retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                       SQLDESC_SCALE,
                                       &scale, 0, 0, 0, 0);
        HandleCLIError(retcode, sqlci_env);
        
        if (DFS2REC::isAnyCharacter(datatype)) {
          retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                         SQLDESC_CHAR_SET,
                                         &charsetEnum, 0, 0, 0, 0);
          HandleCLIError(retcode, sqlci_env);
        }
      }
      
      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                     SQLDESC_NULLABLE,
                                     &null_flag, 0, 0, 0, 0);
      HandleCLIError(retcode, sqlci_env);
      
      char heading[CAT_MAX_HEADING_LEN+1];
      Lng32 heading_len;
      char outputName[CAT_MAX_HEADING_LEN+1];
      Lng32 output_name_len;
      char tableName[CM_GUA_ENAME_LEN+1]; 
      Lng32 table_name_len;
      
      ComASSERT(ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN >=
                (Int32) CAT_MAX_HEADING_LEN);
      
      ComASSERT(ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN >=
                (Int32) CM_GUA_ENAME_LEN);
      
      /////////////////////////////////////////////////////////
      // If user specified heading is present, display it.
      // Otherwise display the name of the column or expression
      // that was used in the select list.
      /////////////////////////////////////////////////////////
      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                     SQLDESC_HEADING,
                                     0, heading,
                                     CAT_MAX_HEADING_LEN, &heading_len, 0);
      HandleCLIError(retcode, sqlci_env);
      
      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                     SQLDESC_NAME,
                                     0, outputName,
                                     CAT_MAX_HEADING_LEN, &output_name_len, 0);
      
      if (heading_len == 0) // heading not present.
      {
        strncpy(heading, outputName, output_name_len);
        heading_len = output_name_len;
        
        HandleCLIError(retcode, sqlci_env);
      }
      
      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                     SQLDESC_TABLE_NAME,
                                     0, tableName,
                                     CM_GUA_ENAME_LEN, &table_name_len, 0);
      HandleCLIError(retcode, sqlci_env);

      Lng32 alignedLen;
      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                     SQLDESC_ALIGNED_LENGTH,
                                     &alignedLen, 0, 0, 0, 0);
      HandleCLIError(retcode, sqlci_env);
      output_data_len += alignedLen;
      
      if (DFS2REC::isAnyCharacter(datatype))
	{
	  Lng32 isCaseInsensitive = 0;
	  retcode = SQL_EXEC_GetDescItem(output_desc, entry,
					 SQLDESC_CASEINSENSITIVE,
					 &isCaseInsensitive, NULL,
					 0, NULL, 0);
	  HandleCLIError(retcode, sqlci_env);
	}

      // add space to display the returned data.
      Lng32 out_buflen_this_col = 0;
      Lng32 display_len = Formatter::display_length(
                            datatype, length, precision, scale, charsetEnum,
                            heading_len, sqlci_env, &out_buflen_this_col);

      output_buflen += out_buflen_this_col +
        ((entry < num_output_entries) ? Formatter::BLANK_SEP_WIDTH : 0);
      
      prep_stmt->outputEntries()[entry-1] =
        new PrepEntry(datatype, length, scale, precision,
                      null_flag, 
                      heading, heading_len,
                      outputName, output_name_len, display_len,
                      out_buflen_this_col, charsetEnum, tableName, table_name_len);
    }

    // allocate space to hold the returned data
    output_data = new char[output_data_len];
    
    // set var_ptr and ind_ptr
    curpos = 0;
    for (entry=1; entry <= num_output_entries; entry++)
    {
      PrepEntry * outputEntry = prep_stmt->outputEntries()[entry-1];

      Lng32 dataOffset;
      Lng32 nullIndOffset;
      
      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                     SQLDESC_NULL_IND_OFFSET,
                                     &nullIndOffset, 0, 0, 0, 0);
      HandleCLIError(retcode, sqlci_env);

      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
                                     SQLDESC_DATA_OFFSET,
                                     &dataOffset, 0, 0, 0, 0);
      HandleCLIError(retcode, sqlci_env);

      if (nullIndOffset >= 0)
	{
	  retcode = SQL_EXEC_SetDescItem(output_desc, entry,
					 SQLDESC_IND_PTR,
					 (Long)&output_data[nullIndOffset],0);
	  HandleCLIError(retcode, sqlci_env);
	}

      retcode = SQL_EXEC_SetDescItem(output_desc, entry,
				     SQLDESC_VAR_PTR,
				     (Long)&output_data[dataOffset],0);
      
    }
    
    prep_stmt->outputDatalen() = output_data_len;
    prep_stmt->outputData() = output_data;
    
    // Allocate space to hold the formatted row that will be displayed.
    // The 'extraOutputBuf' multiplier is used ONLY for the
    // buffer allocation itself and NOT for the recorded length
    // (otherwise, headers get screwed up!).
    // It is calculated ONLY the first several PREPAREs in SQLCI,
    // so as to minimize its overhead --
    // a getenv of SQL_MXCI_SHOW_NONPRINTING.
    
    static Int32  prepCnt = 0;
    static Lng32 extraOutputBuf;
    
    if (prepCnt < 5) {
      prepCnt++;
      if (Formatter::getShowNonprintingReplacementChar(TRUE) ==
          Formatter::HEX_EXPANSION_ON)
        extraOutputBuf = Formatter::HEX_BUFSIZ_MULTIPLIER;
      else
        extraOutputBuf = 1;
    }
    prep_stmt->outputBuflen() = output_buflen;
    prep_stmt->outputBuf()    = new char[extraOutputBuf * output_buflen + 1];
    
  } // if (num_output_entries > 0)
  
}

short SqlCmd::doDescribeInput(SqlciEnv * sqlci_env, 
			      SQLSTMT_ID * stmt, 
			      PrepStmt * prep_stmt, 
			      Lng32 num_input_entries,
			      Int32 numUnnamedParams,
			      char ** unnamedParamArray,
			      CharInfo::CharSet* unnamedParamCharSetArray)
{
  Lng32 retcode = 0;
  Int32 num_named_params = 0;
  SqlciList<Param>    *unnamed_param_list = NULL;
  ComDiagsArea *diags = NULL;

  SQLDESC_ID * input_desc  = prep_stmt->getInputDesc();

  char * aligned_input_data = 0;
  Lng32 aligned_input_data_len = 0;
  NABoolean canAlign = TRUE;

  // if input params present, then assign their values to the dataptr
  // field of the descriptor.
  if (num_input_entries > 0)
    {
      short datatype;
      Lng32 returned_len;
      Lng32 length = 0;
      Lng32 precision;
      Lng32 scale;
      Lng32 unnamed;
      Lng32 vcIndLen = -1;
      CharInfo::CharSet charset = CharInfo::CHARSET_MIN;
      
      char param_name[ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN+1];
      
      prep_stmt->inputEntries() = new PrepEntry * [num_input_entries];
 
      for (short entry=1; entry <= num_input_entries; entry++)
	{
	  Lng32 temp;
	  retcode = SQL_EXEC_GetDescItem(input_desc, entry,
					 SQLDESC_TYPE_FS,
					 &temp, 0, 0, 0, 0);
	  datatype = (short) temp;
	  HandleCLIError(retcode, sqlci_env);
	  retcode = SQL_EXEC_GetDescItem(input_desc, entry,
					 SQLDESC_OCTET_LENGTH,
					 &length, 0, 0, 0, 0);
	  HandleCLIError(retcode, sqlci_env);

	  Lng32 alignedLen = 0;
	  retcode = SQL_EXEC_GetDescItem(input_desc, entry,
					 SQLDESC_ALIGNED_LENGTH,
					 &alignedLen, 0, 0, 0, 0);
	  HandleCLIError(retcode, sqlci_env);
	  aligned_input_data_len += alignedLen;
	  
	  // check if parameter is unnamed
	  // note: not implemented in executor yet
	  //       if parameter is unnamed, SQLDESC_UNNAMED = 1
	  retcode = SQL_EXEC_GetDescItem(input_desc, entry,
					 SQLDESC_UNNAMED,
					 &unnamed, 0, 0, 0, 0);
	  HandleCLIError(retcode, sqlci_env);
	  
	  Lng32 whatToPutIntoPrecision = SQLDESC_PRECISION;
	  Lng32 whatToPutIntoScale     = SQLDESC_SCALE;
	  if (datatype >= REC_MIN_INTERVAL && datatype <= REC_MAX_INTERVAL) {
	    whatToPutIntoPrecision = SQLDESC_INT_LEAD_PREC;
	    whatToPutIntoScale     = SQLDESC_PRECISION;
	  } else if (datatype == REC_DATETIME) {
	    whatToPutIntoPrecision = SQLDESC_DATETIME_CODE;
	    whatToPutIntoScale     = SQLDESC_PRECISION;
	  } else if ( DFS2REC::isAnyCharacter(datatype) ) {
	    
	    retcode = SQL_EXEC_GetDescItem(input_desc, entry,
					   SQLDESC_CHAR_SET,
					   &temp, 0, 0, 0, 0);
	    HandleCLIError(retcode, sqlci_env);
	    
	    charset = (CharInfo::CharSet)temp;

            if ( DFS2REC::isSQLVarChar(datatype))
              {
                retcode = SQL_EXEC_GetDescItem(input_desc, entry,
                                               SQLDESC_VC_IND_LENGTH,
                                               &vcIndLen, 0, 0, 0, 0);
                HandleCLIError(retcode, sqlci_env);
              }
          }
	  retcode = SQL_EXEC_GetDescItem(input_desc, entry,
					 whatToPutIntoPrecision,
					 &precision, 0, 0, 0, 0);
	  HandleCLIError(retcode, sqlci_env);
	  retcode = SQL_EXEC_GetDescItem(input_desc, entry,
					 whatToPutIntoScale,
					 &scale, 0, 0, 0, 0);
	  HandleCLIError(retcode, sqlci_env);
	  
	  retcode = SQL_EXEC_GetDescItem(input_desc, entry,
					 SQLDESC_NAME,
					 0, param_name,
					 ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN,
				         &returned_len, 0);
	  HandleCLIError(retcode, sqlci_env);
	  param_name[returned_len] = 0;
	  
	  // kludge:  until executor recognizes unnamed parameters,
	  // if length of parameter name is zero, parameter is unnamed
	  NABoolean isUnnamed = !returned_len;
	  
          Param *param = NULL;
	  
	  if (!isUnnamed) {
	    num_named_params++;
	    
	    // lookup this param in the sqlci param-list
	    param = sqlci_env->get_paramlist()->get(param_name);
	  }
	  else if (isUnnamed) {
	    short entryOffset = entry - num_named_params;
	    sprintf(param_name, "(UNNAMED_%hd)", entryOffset);
	    if (numUnnamedParams > 0) {
	      if (entryOffset <= numUnnamedParams) {
		
                if ( unnamedParamCharSetArray ) {
		  if ( unnamedParamCharSetArray[entryOffset-1] == CharInfo::UNICODE ) {
		    
		    Int32 len = (Int32)strlen(unnamedParamArray[entryOffset-1]);
		    NAWchar* wstrBuf = new NAWchar[len+1];
		    
		    LocaleStringToUnicode(sqlci_env->getTerminalCharset(), 
					  unnamedParamArray[entryOffset-1], len,
					  wstrBuf, len+1, TRUE);
		    
		    param = new Param(param_name, wstrBuf, CharInfo::UNICODE);
		    
		    delete [] wstrBuf;
		  } else {
		    param = new Param(param_name,
				      unnamedParamArray[entryOffset-1], 
				      unnamedParamCharSetArray[entryOffset-1]);
		  }
                } else
		  param = new Param(param_name,
				    unnamedParamArray[entryOffset-1]);
		
		// If it we do a new  Param() above that means that
		// it is an unnamed param and we have to add it to a 
		// list which will then be deallocated later.  This is  
		// for Soln 10-031203-1717. 
                if (! unnamed_param_list)
		  unnamed_param_list = new SqlciList<Param>;
        	unnamed_param_list->append(param);
                //the parameter value is NULL, so set the nullValue_ field
                //in the param
                if (!param->getValue())
                  param->makeNull();
	      }	// unnamed param found in Execute's USING list
	    }	
	  }	// isUnnamed
	  
	  if (param) 
	    {
	      if (param->isNull()) {
		retcode = SQL_EXEC_SetDescItem(input_desc, entry,
					       SQLDESC_IND_PTR,
					       (Long)(param->getNullValue()),
					       0);
	      }
	      else  {
		NABoolean error = FALSE;
		
                Lng32 inLength = length;

                Int32 previousEntry = 0;
       
                if (diags != NULL)
                    previousEntry = diags->getNumber(DgSqlCode::ERROR_);
		
		if ( DFS2REC::isAnyCharacter(datatype) ) 
		  scale = (Lng32)charset; // pass in target charset in argument 'scale'
		
		retcode = param->convertValue(sqlci_env, datatype, length,
					      precision, scale, vcIndLen, diags);
                Int32 newestEntry = 0;
                if (diags != NULL)
                    newestEntry = diags->getNumber(DgSqlCode::ERROR_);
		
		//if the convertValue gets a string overflow warning, convert
		//it to error for non characters and it remains warning for characters
		if (newestEntry > previousEntry) {
		  if (diags->getErrorEntry(newestEntry)->getSQLCODE() == EXE_STRING_OVERFLOW ){
		    if (!DFS2REC::isAnyCharacter(datatype)) {
		      diags->negateCondition(newestEntry-1);
		      error = TRUE;
		    }
		  }
		}
		
		// If convertValue did not return any errors continue on this path
		if ((retcode >= 0) && !error) {
		  if (length != inLength){
		    // the length of input is different than what cli 
		    // returned. Change the length in the descriptor.
		    retcode = SQL_EXEC_SetDescItem(input_desc, entry,
						   SQLDESC_LENGTH,
						   length,
						   0);

		    canAlign = FALSE;
		  }
		  
		  retcode = SQL_EXEC_SetDescItem(input_desc, entry,
						 SQLDESC_VAR_PTR,
						 (Long)(param->getConvertedValue()),
						 0);
		  HandleCLIError(retcode, sqlci_env);
		  retcode = SQL_EXEC_SetDescItem(input_desc, entry,
						 SQLDESC_IND_PTR,
						 0, 0);
		}
		// If convertValue failed and DID return an error the retcode 
		// will be < 0 and it will follow this code path.
		else {
		  char tgttype[100];
		  Lng32 charSet;
		  Lng32 collation;
		  
		  retcode = SQL_EXEC_GetDescItem( input_desc
						  , entry
						  , SQLDESC_CHAR_SET
						  ,&charSet
						  , 0
						  , 0
						  , 0
						  , 0
						  );
		  HandleCLIError(retcode, sqlci_env);
		  retcode = SQL_EXEC_GetDescItem( input_desc
						  , entry
						  , SQLDESC_COLLATION
						  ,&collation
						  , 0
						  , 0
						  , 0
						  , 0
						  );
                  
                  rec_datetime_field dtStartField = REC_DATE_YEAR;
                  rec_datetime_field dtEndField = REC_DATE_SECOND;
                  Lng32 intLeadPrec = SQLInterval::DEFAULT_LEADING_PRECISION;
                  if (datatype == REC_DATETIME)
                    {
                      Lng32 dtCode;
                      retcode = SQL_EXEC_GetDescItem(input_desc, entry,
                                                     SQLDESC_DATETIME_CODE,
                                                     &dtCode, 0, 0, 0, 0);
                      HandleCLIError(retcode, sqlci_env);

                      // this will get fractional precision
                      retcode = SQL_EXEC_GetDescItem(input_desc, entry,
                                                     SQLDESC_PRECISION,
                                                     &precision, 0, 0, 0, 0);
                      HandleCLIError(retcode, sqlci_env);
                      
                      if (dtCode == REC_DTCODE_DATE)
                        {
                          dtStartField = REC_DATE_YEAR;
                          dtEndField = REC_DATE_DAY;
                        }
                      else if (dtCode == REC_DTCODE_TIME)
                        {
                          dtStartField = REC_DATE_HOUR;
                          dtEndField = REC_DATE_SECOND;
                        }
                      else if (dtCode == REC_DTCODE_TIMESTAMP)
                        {
                          dtStartField = REC_DATE_YEAR;
                          dtEndField = REC_DATE_SECOND;
                        }
                     }
                  else if (DFS2REC::isInterval(datatype))
                    {
                      getIntervalFields(datatype, dtStartField, dtEndField);

                      // this will get fractional precision
                      retcode = SQL_EXEC_GetDescItem(input_desc, entry,
                                                     SQLDESC_PRECISION,
                                                     &precision, 0, 0, 0, 0);
                      HandleCLIError(retcode, sqlci_env);

                      // this will get interval leading precision
                      retcode = SQL_EXEC_GetDescItem(input_desc, entry,
                                                     SQLDESC_INT_LEAD_PREC,
                                                     &intLeadPrec, 0, 0, 0, 0);
                      HandleCLIError(retcode, sqlci_env);
                    }

		  NAType::convertTypeToText(tgttype,
					    datatype, length, precision, scale,
					    dtStartField, dtEndField,
					    (short)precision,
                                            (short)intLeadPrec,
					    FALSE/*upshift*/,
					    FALSE/*caseinsensitive*/,
					    (CharInfo::CharSet) charSet,
					    (CharInfo::Collation) collation,
                                            NULL);  // displaydatatype
		  
		  // All PARAMS are character type.
		  // #We should single-quote it and double-up any embedded quotes
		  // #Should be a global func (in NAString.cpp?) for this
		  NAString srcval(param->getDisplayValue(sqlci_env->getTerminalCharset()));
		  if (srcval.isNull()) srcval = "''";	// empty string literal
		  
                  if (diags == NULL)
                     diags = ComDiagsArea::allocate(&sqlci_Heap);
		  *diags << DgSqlCode(-SQLCI_PARAM_BAD_CONVERT)
			<< DgString0(Param::getExternalName(param_name))
			<< DgString1(srcval)
			<< DgString2(tgttype);
		}	// convertValue failed
	      } // not null param
	    }	// if param
	  else {
            if (diags == NULL)
                diags = ComDiagsArea::allocate(&sqlci_Heap);
	    *diags << DgSqlCode(-SQLCI_PARAM_NOT_FOUND)
		  << DgString0(Param::getExternalName(param_name));
	  }
	  
	  HandleCLIError(retcode, sqlci_env);
	  
	  prep_stmt->inputEntries()[entry-1] =
	    new PrepEntry(datatype, length, scale, precision,
			  (param ? param->isNull() : 0), // null_flag, 
			  NULL, 0,
			  param_name, returned_len,
			  0, 0, 0, NULL, 0);
	  if (param)
	    {
	      if (param->isNull())
		prep_stmt->inputEntries()[entry-1]->setIndPtr(
		     param->getNullValue());
	      prep_stmt->inputEntries()[entry-1]->setVarPtr(
		   param->getConvertedValue());
	    }
	  
	}	// for num_input_entries
      
    }		// if num_input_entries
  
  if (numUnnamedParams > 0 &&
      numUnnamedParams > num_input_entries - num_named_params)
    {
       if (diags == NULL)
          diags = ComDiagsArea::allocate(&sqlci_Heap);
      // Warning only, so continue processing after this!
      *diags << DgSqlCode(+SQLCI_EXTRA_PARAMS_SUPPLIED)	// + (i.e. warning)
	    << DgInt0(numUnnamedParams)
	    << DgInt1(num_input_entries - num_named_params);
    }
  
  if (diags != NULL)
    {
      handleLocalError(diags, sqlci_env);
      if (diags->getNumber(DgSqlCode::ERROR_)) {
	return SQL_Error;
      }
    }

  if ((num_input_entries > 0) && (canAlign))
    {
      Lng32 dataOffset = -1;
      Lng32 nullIndOffset = -1;

      aligned_input_data = new char[aligned_input_data_len];
      for (short entry=1; entry <= num_input_entries; entry++)
	{
	  PrepEntry * inputEntry = prep_stmt->inputEntries()[entry-1];

	  retcode = SQL_EXEC_GetDescItem(input_desc, entry,
					 SQLDESC_NULL_IND_OFFSET,
					 &nullIndOffset, 0, 0, 0, 0);
	  HandleCLIError(retcode, sqlci_env);
	  
	  retcode = SQL_EXEC_GetDescItem(input_desc, entry,
					 SQLDESC_DATA_OFFSET,
					 &dataOffset, 0, 0, 0, 0);
	  HandleCLIError(retcode, sqlci_env);
	  
	  if (nullIndOffset >= 0)
	    {
	      retcode = SQL_EXEC_SetDescItem(input_desc, entry,
					     SQLDESC_IND_PTR,
					     (Long)&aligned_input_data[nullIndOffset],0);
	      HandleCLIError(retcode, sqlci_env);

	      if (inputEntry->indPtr())
		str_cpy_all(&aligned_input_data[nullIndOffset],
			    inputEntry->indPtr(), sizeof(short));
	      else
		{
		  short noNull = 0;
		  str_cpy_all(&aligned_input_data[nullIndOffset],
			      (char*)&noNull, sizeof(short));
		}
	    }
	  
	  retcode = SQL_EXEC_SetDescItem(input_desc, entry,
					 SQLDESC_VAR_PTR,
					 (Long)&aligned_input_data[dataOffset],0);
	  HandleCLIError(retcode, sqlci_env);

	  if (inputEntry->varPtr())
	    {
	      if (DFS2REC::isAnyVarChar(inputEntry->datatype()))
		str_cpy_all(&aligned_input_data[dataOffset],
			    inputEntry->varPtr(), 
			    sizeof(short) + inputEntry->length());
	      else
		str_cpy_all(&aligned_input_data[dataOffset],
			    inputEntry->varPtr(), inputEntry->length());
	    }
	}
    }

  return 0;
}

short SqlCmd::doExec(SqlciEnv * sqlci_env,
		     SQLSTMT_ID * stmt,
		     PrepStmt * prep_stmt,
		     Int32 numUnnamedParams,
		     char ** unnamedParamArray,
		     CharInfo::CharSet* unnamedParamCharSetArray,
		     NABoolean handleError)
{
  Lng32 retcode = 0;
  rowsAffected = 0;
  SqlciList<Param>    *unnamed_param_list = NULL;

  SQLDESC_ID * input_desc  = prep_stmt->getInputDesc();
  SQLDESC_ID * output_desc = prep_stmt->getOutputDesc();
  dml_type stmt_type = prep_stmt->getType();

  Lng32 num_input_entries = prep_stmt->numInputEntries();

  retcode = doDescribeInput(sqlci_env, stmt, prep_stmt,
			    num_input_entries,
			    numUnnamedParams, unnamedParamArray,
			    unnamedParamCharSetArray);
  if (retcode == SQL_Error)
    return (short)retcode;

  /* execute the statement */
  retcode = SQL_EXEC_Exec(stmt, input_desc, 0,
			  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
  if (handleError)
    HandleCLIError(retcode, sqlci_env, TRUE);

  if (unnamed_param_list)
    delete unnamed_param_list;

  if (retcode > 0)
     getRowsAffected(stmt);
  return (short)retcode;
} // SqlCmd::doExec

short SqlCmd::doFetch(SqlciEnv * sqlci_env, SQLSTMT_ID * stmt,
		      PrepStmt * prep_stmt,
		      NABoolean firstFetch,
		      NABoolean handleError,
                                 Int32 prepcode)
{
  Lng32 retcode = 0;

  // fetch rows till EOF. Note, for statements which do not
  // return rows (like, INSERT...), an EOF will be returned
  // on the first fetch.
  retcode = SQL_EXEC_Fetch(stmt, prep_stmt->getOutputDesc(), 0,
			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);

  NABoolean isEOD = 0;
  if (handleError)
    HandleCLIError(stmt, retcode, sqlci_env, TRUE, &isEOD,prepcode);
  if (isEOD)
    retcode = SQL_Eof;


  return (short)retcode;
}

short SqlCmd::doClearExecFetchClose(SqlciEnv * sqlci_env,
				    SQLSTMT_ID * stmt,
				    PrepStmt * prep_stmt,
				    Int32 numUnnamedParams,
				    char ** unnamedParamArray,
				    CharInfo::CharSet* unnamedParamCharSetArray,
				    NABoolean handleError)
{
  Lng32 retcode = 0;
  rowsAffected = 0;

  Lng32 num_input_entries = prep_stmt->numInputEntries();
  retcode = doDescribeInput(sqlci_env, stmt, prep_stmt,
			    num_input_entries,
			    numUnnamedParams, unnamedParamArray,
			    unnamedParamCharSetArray);
  if (retcode == SQL_Error)
    return (short)retcode;
  
  retcode = SQL_EXEC_ClearExecFetchClose(
       stmt, 
       ((prep_stmt->numInputEntries() > 0)
	? prep_stmt->getInputDesc() : NULL),
       ((prep_stmt->numOutputEntries() > 0)
	? prep_stmt->getOutputDesc() : NULL),
       0, 0, 0);

  Lng32 queryType = prep_stmt->queryType();
  if (queryType != SQL_SELECT_UNIQUE)
    {
      if (retcode == SQL_Success)
	retcode = SQL_Eof;
    }
  if (handleError)
    HandleCLIError(retcode, sqlci_env, TRUE);

  if (retcode > 0)
     getRowsAffected(stmt);
  return (short)retcode;
}

short SqlCmd::getHeadingInfo(SqlciEnv * sqlci_env,
			     PrepStmt * prep_stmt,
			     char * heading_row,
			     char * underline)
{
  Lng32 retcode = 0;
  SQLDESC_ID * output_desc = prep_stmt->getOutputDesc();
  Lng32 output_buflen = prep_stmt->outputBuflen();
  char * heading = new char[ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN+1];
  Lng32 returned_len;
  Lng32 heading_len;
  Lng32 curpos = 0;

  str_pad(heading_row, output_buflen, ' ');
  str_pad(underline, output_buflen, ' ');

  Lng32 num_output_entries = prep_stmt->numOutputEntries();
  for (short entry=1; entry <= num_output_entries; entry++)
    {
      /////////////////////////////////////////////////////////
      // If user specified heading is present, display it.
      // Otherwise display the name of the column or expression
      // that was used in the select list.
      /////////////////////////////////////////////////////////
      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
				     SQLDESC_HEADING,
				     0, heading,
				     CAT_MAX_HEADING_LEN, &returned_len, 0);
      HandleCLIError(retcode, sqlci_env);

      if (returned_len == 0) // heading not present.
	{
	  retcode = SQL_EXEC_GetDescItem(output_desc, entry,
					 SQLDESC_NAME,
					 0, heading,
					 CAT_MAX_HEADING_LEN, &returned_len, 0);
	  HandleCLIError(retcode, sqlci_env);
	}

      heading[returned_len] = 0;

      heading_len = prep_stmt->outputEntries()[entry-1]->displayLen();
      str_pad(&underline[curpos], (Int32)heading_len, '-');

      str_cpy_all(&heading_row[curpos], heading, returned_len);
      curpos += heading_len +
		((entry < num_output_entries) ? Formatter::BLANK_SEP_WIDTH : 0);

    } // for
  heading_row[curpos] = 0;
  underline[curpos] = 0;

  // For long headings, trim trailing spaces
  if (curpos > 72)
    while (curpos-- && heading_row[curpos] == ' ')
      heading_row[curpos] = 0;

  delete [] heading;

  return 0;
}
short SqlCmd::displayHeading(SqlciEnv * sqlci_env,
			     PrepStmt * prep_stmt)
{
  Lng32 retcode = 0;

  Logfile *log = sqlci_env->get_logfile();
  if (!lastLineWasABlank) log->WriteAll("");

  if (prep_stmt->getType() != DML_DISPLAY_NO_HEADING_TYPE)
  {
    // Only obtain and display header row if requested
    Lng32 output_buflen = prep_stmt->outputBuflen();
    char * heading_row = new char[output_buflen + 1];
    char * underline = new char[output_buflen + 1];

    retcode = getHeadingInfo(sqlci_env, prep_stmt, heading_row, underline);

    if (sqlci_env->getTerminalCharset() != CharInfo::UTF8)
    {
      char * converted_heading_row = new char[output_buflen*4+1];
      char *pFirstUntranslatedChar = NULL;
      UInt32 outLen = 0;
      UInt32 translatedCharCount = 0;
      Int32 retCode = UTF8ToLocale ( cnv_version1
                                   , (const char*) heading_row
                                   , (const Int32) strlen(heading_row)
                                   , (const char*) converted_heading_row
                                   , (const Int32) output_buflen*4+1
                                   , (cnv_charset) convertCharsetEnum((Int32)sqlci_env->getTerminalCharset())
                                   , (char* &)     pFirstUntranslatedChar
                                   , (UInt32 *)    &outLen // unsigned int *output_data_len_p 
                                   , (const Int32) TRUE    // const int addNullAtEnd_flag
                                   , (const Int32) TRUE    // const int allow_invalids 
                                   , (UInt32 *)    &translatedCharCount // unsigned int * translated_char_cnt_p
                                   , (const char*) NULL    // const char *substitution_char_p
                                   );
      log->WriteAll(converted_heading_row);
      delete [] converted_heading_row;
    }
    else
      log->WriteAll(heading_row);

    log->WriteAll(underline);
    log->WriteAll("");

    delete [] heading_row;
    delete [] underline;
  }
  lastLineWasABlank = TRUE;

  return 0;
}

Lng32 SqlCmd::displayRow(SqlciEnv * sqlci_env,
			 PrepStmt * prep_stmt)
{
  NABoolean playItSafeUseTheOldWay = TRUE;

  Lng32 retcode = 0;
  char * buf = prep_stmt->outputBuf();
  SQLDESC_ID * output_desc = prep_stmt->getOutputDesc();

  /* print the output */
  Lng32 curpos = 0;

  Lng32 num_output_entries = prep_stmt->numOutputEntries();

  if (playItSafeUseTheOldWay)
  for (short entry=1; entry <= num_output_entries; entry++)
    {
      Long data_addr;
      Long ind_data_addr;

      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
				     SQLDESC_VAR_PTR,
				     &data_addr, 0, 0, 0, 0);
      HandleCLIError(retcode, sqlci_env);

      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
				     SQLDESC_IND_PTR,
				     &ind_data_addr, 0, 0, 0, 0);
      HandleCLIError(retcode, sqlci_env);

      PrepEntry * outputEntry = prep_stmt->outputEntries()[entry-1];
      Formatter::buffer_it(sqlci_env, (char *)data_addr,
			   outputEntry->datatype(),
			   outputEntry->length(),
			   outputEntry->precision(),
			   outputEntry->scale(),
			   (char *)ind_data_addr,
			   outputEntry->displayLen(),
			   outputEntry->displayBufLen(),
			   outputEntry->nullFlag(),
			   &buf[curpos], &curpos,
			   entry < num_output_entries,	//separatorNeeded
			   TRUE);			//checkShowNonPrinting
    }

  if (buf)
    buf[curpos] = 0;

  return curpos;
}

short SqlCmd::do_execute(SqlciEnv * sqlci_env,
			 PrepStmt * prep_stmt,
			 Int32 numUnnamedParams,
			 char ** unnamedParamArray,
                         CharInfo::CharSet* unnamedParamCharSetArray,
                         Int32 prepcode)
{
  Lng32 retcode = 0;
  //short ret;
  Logfile *log = sqlci_env->get_logfile();
  SQLSTMT_ID * stmt = prep_stmt->getStmt();
  dml_type stmt_type = prep_stmt->getType();
  ULng32 num_rows_returned = 0;
  ULng32 listcount = sqlci_env->getListCount();
  NABoolean listcountReached = ((listcount == 0) ? TRUE : FALSE);
  NABoolean noScreenOutput = (getenv("NO_SCREEN_OUTPUT") ? TRUE : FALSE);
  NABoolean displayFirstRow = (getenv("DISPLAY_FIRST_ROW") ? TRUE : FALSE);
  Int32 useCout = 0;  // Flag which tells LOG::WRITEALL to use COUT
  						  // versus COUT.WRITE.

  // Bookkeeping for stored procedure result sets
  NABoolean isResultSet = (stmt_type == DML_CALL_STMT_RS_TYPE ? TRUE : FALSE);
  char childQueryId[ComSqlId::MAX_QUERY_ID_LEN+1];
  Lng32 childQueryIdLen;
  SQL_QUERY_COST_INFO childQueryCostInfo;
  SQL_QUERY_COMPILER_STATS_INFO childQueryCompilerStatsInfo;

  Lng32 queryType = prep_stmt->queryType();
  Lng32 subqueryType = prep_stmt->getSubqueryType();

  if ((stmt_type == DML_CONTROL_TYPE) &&
      ((prep_stmt->queryType() == SQL_SELECT_NON_UNIQUE) ||
       (prep_stmt->queryType() == SQL_SELECT_UNIQUE)))
    stmt_type = DML_SELECT_TYPE;
  else if ((stmt_type == DML_DDL_TYPE) &&
	   (prep_stmt->queryType() == SQL_INSERT_NON_UNIQUE))
    stmt_type = DML_INSERT_TYPE;
  else if ((stmt_type == DML_DDL_TYPE) &&
	   ((prep_stmt->queryType() == SQL_SELECT_NON_UNIQUE) ||
            (prep_stmt->queryType() == SQL_SELECT_UNIQUE)))
    stmt_type = DML_SELECT_TYPE;
  else if ((stmt_type == DML_DDL_TYPE) &&
      (queryType == SQL_EXE_UTIL && subqueryType == SQL_STMT_CTAS))
    stmt_type = DML_INSERT_TYPE;


  NABoolean firstRowDisplayed = FALSE;

  sqlci_env->lastExecutedStmt() = prep_stmt;

  
  NABoolean doCEFC = FALSE;  // do ClearExecFetchClose
  NABoolean doCTAS = FALSE;
  NABoolean doHBL = FALSE;
  if (((queryType != SQL_OTHER) &&
       (queryType != SQL_UNKNOWN) &&
       (queryType != SQL_CAT_UTIL) &&
       (queryType != SQL_EXE_UTIL) &&
       (queryType != SQL_SELECT_NON_UNIQUE) &&
       //      (queryType != SQL_SELECT_UNIQUE) &&
       (queryType != SQL_CALL_NO_RESULT_SETS) &&
       (queryType != SQL_CALL_WITH_RESULT_SETS) &&
       (! isResultSet) &&
       //      (prep_stmt->numOutputEntries() == 0) &&
       (! getenv("NO_MXCI_CEFC"))) &&
      ((queryType == SQL_SELECT_UNIQUE) ||
       (prep_stmt->numOutputEntries() == 0)))
  {
    doCEFC = TRUE;
    retcode = 
      doClearExecFetchClose(sqlci_env,
                            prep_stmt->getStmt(),
                            prep_stmt,
                            numUnnamedParams,
                            unnamedParamArray,
                            unnamedParamCharSetArray,
			    TRUE);
  }
  if (queryType == SQL_EXE_UTIL && subqueryType == SQL_STMT_CTAS)
    doCTAS = TRUE;
  if (doCTAS)
  {
    noScreenOutput = TRUE;
    if (getenv("DISPLAY_CTAS_OUTPUT"))
      noScreenOutput = FALSE;
  }

  if (queryType == SQL_EXE_UTIL &&
      (subqueryType == SQL_STMT_HBASE_LOAD || subqueryType == SQL_STMT_HBASE_UNLOAD))
    doHBL = TRUE;

  if (NOT doCEFC)
  {
    retcode = doExec(sqlci_env,
                     prep_stmt->getStmt(),
                     prep_stmt,
                     numUnnamedParams,
                     unnamedParamArray,
                     unnamedParamCharSetArray,
		     TRUE);
    
  }

  NABoolean processedCallWithResultSets = FALSE;

  NABoolean firstFetchDone = FALSE;
  if (retcode >= 0)
  {
    // Fetch the first row
    if ((NOT doCEFC) && Succ_or_Warn)
      {
	retcode = doFetch(sqlci_env, prep_stmt->getStmt(), prep_stmt, 
			  TRUE, TRUE,prepcode);
	if (Succ_or_Warn)
	  firstFetchDone = TRUE;
      }

    // if first row and output entries, print heading.
    // don't display heading if DESCRIBE or internal SHOWSHAPE command.
    if (Succ_or_Warn && prep_stmt->numOutputEntries() > 0)
      {
	if (stmt_type != DML_DESCRIBE_TYPE && 
	    stmt_type != DML_SHOWSHAPE_TYPE &&
	    stmt_type != DML_DISPLAY_NO_ROWS_TYPE &&
            NOT doCTAS &&
            NOT doHBL)
	  displayHeading(sqlci_env, prep_stmt);
      }
    
    // We just fetched the first row. Each iteration of the following
    // WHILE loop processes one row and then fetches the next.
    while (Succ_or_Warn && !listcountReached && !processedCallWithResultSets)
      {
	num_rows_returned++;
	if (prep_stmt->numOutputEntries() > 0)
	  {
	    Lng32 curpos = displayRow(sqlci_env, prep_stmt);
	    
	    // Trim trailing spaces from SHOWDDL and INVOKE ``column'' values,
            // and GET RELATED NAMES OF .../ GET VERSION OF ...
	    // which are really specially formatted variable-length text
	    // (See CmpDescribe.C).
	    if (stmt_type == DML_DESCRIBE_TYPE ||
		stmt_type == DML_SHOWSHAPE_TYPE ||
		stmt_type == DML_DISPLAY_NO_ROWS_TYPE ||
                stmt_type == DML_DISPLAY_NO_HEADING_TYPE ||
                (doCTAS && NOT noScreenOutput) ||
                (doHBL && NOT noScreenOutput))
	      {
		useCout = -1; // use COUT for output versus using COUT.WRITE.
		
                // Move curpos backwards from the end of the string to
                // the position of the first non-space character
                while (curpos > 0 && prep_stmt->outputBuf()[curpos - 1] == ' ')
                  curpos--;

                // We want to convert a newline-only string ("\n") to
                // an empty string ("")
                if (curpos == 1 && prep_stmt->outputBuf()[0] == '\n')
                  curpos--;

                // Inject a null terminator
                prep_stmt->outputBuf()[curpos] = 0;
	      }
            
	    if (! noScreenOutput)
	      log->WriteAll(prep_stmt->outputBuf(),curpos,useCout);
	    else if ((displayFirstRow) &&
		     (NOT firstRowDisplayed))
	      {
		log->WriteAll(prep_stmt->outputBuf(),curpos,useCout);
		firstRowDisplayed = TRUE;
	      }
            
            // Child QueryId Processing
	    if (doCTAS && (NOT noScreenOutput))
            {
              if (strstr(prep_stmt->outputBuf(), "childQidBegin"))
              {
                Lng32 rc = SQL_EXEC_GetChildQueryInfo(stmt,
                    childQueryId,
                    ComSqlId::MAX_QUERY_ID_LEN,
                    &childQueryIdLen, 
                    &childQueryCostInfo,
                    &childQueryCompilerStatsInfo);
                HandleCLIError(rc, sqlci_env);
                if (rc >= 0)
                {
                  childQueryId[childQueryIdLen]='\0';
                  sprintf(prep_stmt->outputBuf(), "Child Query Id about to execute is %s", childQueryId);
                  log->WriteAll(prep_stmt->outputBuf(), prep_stmt->outputBuflen(),useCout);
                } 
              }
              if (strstr(prep_stmt->outputBuf(), "childQidEnd"))
              {
                PrepStmt *saved_prep_stmt = prep_stmt;
                sqlci_env->getStats()->displayChildQryStats(sqlci_env);
                sqlci_env->lastExecutedStmt() = saved_prep_stmt;
              }
            }
	    
	    useCout = 0;
	    lastLineWasABlank = FALSE;
	  }
	
        // If this is a CALL statement that can return result sets,
        // then the CALL has just executed and we now want to display
        // all returned result sets
        if (queryType == SQL_CALL_WITH_RESULT_SETS &&
            retcode >= 0 && !isResultSet)
	  {
	    processedCallWithResultSets = TRUE;
	    NABoolean done = FALSE;
	    Lng32 rsIndex = 1;
	    while (!done)
	      {
		// long copyOfWorstcode = worstcode;
		
		DML *rs = new DML("", DML_CALL_STMT_RS_TYPE,
				  "__SQLCI_DML_SP_RESULT_SET__");
		rs->setResultSetIndex(rsIndex);
	 Lng32 rsProcessingCode = rs->process(sqlci_env);
		
		delete rs;
		rsIndex++;
		
		// Not completely sure what to do with the worstcode
		// variable now. If errors were encountered while
		// displaying the last result set, worstcode will be
		// SQL_Error. But if we are going to ignore that error and
		// continue processing more result sets, maybe we should
		// reset worstcode to the value of copyOfWorstcode. TBD...
		//worstcode = copyOfWorstcode;
		
		// Restore last exec stmt pointer
		sqlci_env->lastExecutedStmt() = prep_stmt;
		
		// Possible return values: 0, 1, SQL_Canceled
		if (rsProcessingCode == 1 ||
		    (rsProcessingCode < 0 && rsProcessingCode != SQL_Canceled))
		  {
		    retcode = rsProcessingCode;
		    done = TRUE;
		  }
	      } // while (!done)
	    
	  } // End of CALL statement result set loop
        
	if (num_rows_returned == listcount &&
	    stmt_type != DML_DESCRIBE_TYPE &&
	    stmt_type != DML_SHOWSHAPE_TYPE)
	  listcountReached = TRUE;
	else if (!processedCallWithResultSets)
	  {
	    if (NOT doCEFC)
	      retcode = doFetch(sqlci_env, stmt, prep_stmt);	// get next row
	    else
	      retcode = SQL_Eof;
	  }
        
      } // while getting rows
    
    // close the statement, if an error was not received. The
    // statement is already closed in case of an error.
    if ((NOT doCEFC) &&
	(Succ_or_Warn || retcode == SQL_Eof || retcode == SQL_Canceled))
      {
	retcode = SQL_EXEC_CloseStmt(stmt);
	if (processedCallWithResultSets &&
	    retcode == -8811) // statement already closed
	  {
	    SqlCmd::clearCLIDiagnostics();
	    retcode = 0;
	  }
	HandleCLIError(retcode, sqlci_env);
      }
    getRowsAffected(stmt); 
    
  } // retcode >= 0
  
  char donemsg[100];
  donemsg[0] = '\0';

  switch (stmt_type)
  {
    case DML_UPDATE_TYPE:
    case DML_DELETE_TYPE:
    case DML_INSERT_TYPE:
    case DML_UNLOAD_TYPE:
    {
      if (stmt_type == DML_UPDATE_TYPE)
	sprintf(donemsg, UPDATED_MESSAGE, rowsAffected);
      else if (stmt_type == DML_DELETE_TYPE)
	sprintf(donemsg, DELETED_MESSAGE, rowsAffected);
      else if (stmt_type == DML_INSERT_TYPE)
	sprintf(donemsg, INSERTED_MESSAGE, rowsAffected);
      else if (stmt_type == DML_UNLOAD_TYPE)
        sprintf(donemsg, UNLOADED_MESSAGE, rowsAffected);

    }
    break;

    case DML_CALL_STMT_RS_TYPE:
    case DML_SELECT_TYPE:
      {
        if (! noScreenOutput)
          sprintf(donemsg, SELECTED_MESSAGE, num_rows_returned);
        else
          {
            if (displayFirstRow)
              sprintf(donemsg, SELECTED_FIRST_ROW_DISPLAY_MESSAGE, num_rows_returned);
            else
              sprintf(donemsg, SELECTED_BUT_MESSAGE, num_rows_returned);
          }
      }
    break;

    case DML_CONTROL_TYPE:
    case DML_OSIM_TYPE:
    case DML_DISPLAY_NO_HEADING_TYPE:
    case DML_DESCRIBE_TYPE:
    case DML_DDL_TYPE:
    {
	if (stmt_type == DML_DDL_TYPE && doHBL )
	  sprintf(donemsg, LOADED_MESSAGE, rowsAffected);
	else if (worstcode == SQL_Success || worstcode == SQL_Eof)
	    sprintf(donemsg, OP_COMPLETE_MESSAGE);
	  else if (worstcode < 0)
	    sprintf(donemsg, OP_COMPLETED_ERRORS);
	  else
	    sprintf(donemsg, OP_COMPLETED_WARNINGS);
    }
    break;

    case DML_DISPLAY_NO_ROWS_TYPE:
      {
	sprintf(donemsg, OP_COMPLETE_MESSAGE);
      }
    break;
    
    case DML_SHOWSHAPE_TYPE:
      {
	donemsg[0] = '\0';	// empty
      }
    break;

    default:
      {
	donemsg[0] = '\0';	// empty
      }
    break;
  }// end of switch stmt

  if (listcountReached) 
    strcat(donemsg, LISTCOUNT_MESSAGE);

  if (!lastLineWasABlank)
    {
      log->WriteAll("");
      lastLineWasABlank = TRUE;
    }
  
  if (donemsg[0] != '\0')
    {
      log->WriteAll(donemsg);
      lastLineWasABlank = FALSE;
    }
  
  if ((retcode < 0) && (retcode != SQL_Canceled))
    return SQL_Error;
  else
    return ((short)retcode);
}

short SqlCmd::executeQuery(const char *query, SqlciEnv *sqlci_env)
{
  // -----------------------------------------------------------------------
  //  Given a SQL query, this procedure invokes the appropriate
  //  CLI calls to prepare and execute the statement.
  // -----------------------------------------------------------------------
  Lng32 retcode;

  SQLMODULE_ID module;
  init_SQLMODULE_ID(&module);

  SQLSTMT_ID *stmt = new SQLSTMT_ID;
  SQLDESC_ID *sql_src = new SQLDESC_ID;
  memset (stmt, 0, sizeof(SQLSTMT_ID));
  memset (sql_src, 0, sizeof(SQLDESC_ID));

  // Allocate a SQL statement
  init_SQLSTMT_ID(stmt, SQLCLI_CURRENT_VERSION, stmt_handle, &module);

  retcode = SQL_EXEC_AllocStmt(stmt, 0);
  HandleCLIError(retcode, sqlci_env);

  // Allocate a descriptor which will hold the SQL statement source
  init_SQLDESC_ID(sql_src, SQLCLI_CURRENT_VERSION, desc_handle, &module);

  retcode = SQL_EXEC_AllocDesc(sql_src, 1);
  HandleCLIError(retcode, sqlci_env);

  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_TYPE_FS,
				 REC_BYTE_V_ANSI, 0);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
				 strlen(query) + 1, 0);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_CHAR_SET_NAM,
				 0,(char*)CharInfo::getCharSetName(sqlci_env->getTerminalCharset()));
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
				 (Long)query, 0);
  HandleCLIError(retcode, sqlci_env);

  // execute immediate this statement
  Int32 prep_flags = 0;
  retcode = SQL_EXEC_ExecDirect2(stmt, sql_src,prep_flags, 0, 0);
  HandleCLIError(retcode, sqlci_env);

  // free up resources
  retcode = SQL_EXEC_DeallocDesc(sql_src);
  HandleCLIError(retcode, sqlci_env);

  retcode = SQL_EXEC_DeallocStmt(stmt);
  HandleCLIError(retcode, sqlci_env);

  delete stmt;
  delete sql_src;

//  return ((retcode < 0) ? SQL_Error : retcode);
	// avoid compiler warning
  return (short)((retcode < 0) ? SQL_Error : retcode);

}

short SqlCmd::setEnviron(SqlciEnv *sqlci_env, Lng32 propagate)
{
  short rc = SQL_EXEC_SetEnviron_Internal(propagate);
  return rc;
}


DML::DML(const char * argument_, dml_type type_, const char * stmt_name_)
  : SqlCmd(SqlCmd::DML_TYPE, argument_), rsIndex_(0)
{
  if (stmt_name_)
    {
      this_stmt_name = new char[strlen(stmt_name_) + 1];
      strcpy(this_stmt_name, stmt_name_);
    }
  else
    this_stmt_name = NULL;

  type = type_;
}

DML::~DML()
{
}

Prepare::Prepare(char * stmt_name_, char * argument_, dml_type type_)
: SqlCmd(SqlCmd::PREPARE_TYPE, argument_)
{
  this_stmt_name = new char[strlen(stmt_name_) + 1];
  strcpy(this_stmt_name, stmt_name_);
  type = type_;
}

Prepare::~Prepare()
{
  delete [] this_stmt_name;
}

DescribeStmt::DescribeStmt(char * stmtName, char * argument) : 
SqlCmd(SqlCmd::DESCRIBE_TYPE, argument){
  stmtName_ = new char[strlen(stmtName) + 1];
  strcpy(stmtName_, stmtName);
}

DescribeStmt::~DescribeStmt(){
  delete [] stmtName_;
}

Cursor::Cursor(char * cursorName, CursorOperation operation,
	       short internalPrepare, char * argument,
               NABoolean internalCursor)
  : SqlCmd(SqlCmd::CURSOR_TYPE, argument), isHoldable_(FALSE),
    resultSetIndex_(0), internalCursor_(internalCursor)
{
  cursorName_ = new char[strlen(cursorName) + 1];
  strcpy(cursorName_, cursorName);
  operation_ = operation;
  internalPrepare_ = internalPrepare;
}

Cursor::~Cursor()
{
  delete [] cursorName_;
}

Execute::Execute(char * stmt_name_, char * argument_, short flag, SqlciEnv * sqlci_env)
: SqlCmd(SqlCmd::EXECUTE_TYPE, argument_)
{
  this_stmt_name = new char[strlen(stmt_name_) + 1];
  strcpy(this_stmt_name, stmt_name_);
  num_params = 0;

  for ( Int32 i=0; i<MAX_NUM_UNNAMED_PARAMS; i++ )
    using_param_charsets[i]=CharInfo::UnknownCharSet;

  if (flag) 
    {
      Lng32 err = storeParams(argument_, num_params, using_params, using_param_charsets, sqlci_env);
      if (err)
	setErrorCode(err);
    }
}

Execute::~Execute()
{
  delete [] this_stmt_name;
  for (Int32 i = num_params; i > 0; )
    delete [] using_params[--i];
}

/*******************************************************/
/* store unnamed parameters in an array                */
/* example:  execute <stmt name> using 24, cat, 'dog'; */
/* input: 24, cat, 'dog';                              */
/* output: none                                        */
/*******************************************************/
static Lng32 errorParams(Lng32 err,
			const char *str0 = NULL, Lng32 int0 = -99)
{
  sqlci_DA << DgSqlCode(err) << DgString0(str0) << DgInt0(int0);
  return err;
}

Lng32 Execute::storeParams(char* argument_, short &num_params,
                           char * using_params[],
                           CharInfo::CharSet using_param_charsets[],
                           SqlciEnv *sqlci_env)
{
  ComASSERT(argument_);
  num_params = 0;

  // Trim trailing blanks and semicolons
  size_t arglen = strlen(argument_);
  for (; arglen; arglen--)
    if (!isspace((unsigned char)argument_[arglen-1]) && argument_[arglen-1] != ';') // For VS2003
      break;

  char sentinel = argument_[arglen];		// either ';' or '\0'
  argument_[arglen] = '\0';			// now '\0'

  // Get past leading blanks
  char *args = argument_;
  while (*args)
  {
    NABoolean literal = TRUE;    // if this arg is a literal, set it to 1
                              // otherwise, set it to 0.  It's set to 1 as default.

    NABoolean isHex = FALSE;
    while (isspace((unsigned char)*args)) args++;  // For VS2003

    if (num_params >= MAX_NUM_UNNAMED_PARAMS)
      return errorParams(-SQLCI_NUM_PARAMS_OVERFLOW,
			       NULL,
			       MAX_NUM_UNNAMED_PARAMS);

    char *errarg = args;			// points to a non-blank value
    char param[MAX_LEN_UNNAMED_PARAM+1];	// buffer for current value
    size_t i = 0;				// len(param)+1

    if ( using_param_charsets && *args == '_' ) {
       char* prefixPtr = args+1;
       while ( *prefixPtr ) {
          if ( *prefixPtr == '\'' ) {

             // check if a valid charset name is found

             // upper case the name first
             Int32 nameLen = prefixPtr-args-1;
             char* upperCaseName = new char[nameLen+1];

             Int32 j;
             for ( j =0; j<nameLen; j++) {
                upperCaseName[j] = (char)TOUPPER(args[j+1]);
             }
             upperCaseName[j] = 0;

             // name lookup
             CharInfo::CharSet cs = CharInfo::getCharSetEnum(upperCaseName);
             delete [] upperCaseName;

             if (CharInfo::isCharSetFullySupported(cs)) {
                errarg = args = prefixPtr; // advance the point to true start of the literal
             } else {
		*prefixPtr= '\0';  // terminate errarg at '\''
		return errorParams(-SQLCI_SYNTAX_ERROR, errarg);
             }

             // fill the charset. If the prefix specifies an unsupported name, 
             // the cs is CharInfo::UnknownCharSet.
             using_param_charsets[num_params] = cs;

             break;
          }
          prefixPtr++;
       }
    }

    switch (*args)
    {
      case '\'':  // Convert 'ab' to ab, 'ab''c' to ab'c
		  // because basically we want
		  //   USING 'ab'	and	USING ab
		  // to be equivalent.
		  {
		    char *qseen = errarg;	// points to beginning ' of args
		    while (*args++ &&
			   i < MAX_LEN_UNNAMED_PARAM)
		    {
		      if (*args == '\'')
			if (args[1] != '\'')
			  { qseen = args++; break; }	// ending ' of args
			else
			  param[i++] = *args++;	// yes, ++ to skip one ' of ''
		      else
			param[i++] = *args;	// no ++ (while does it)
		    }

                    if ( CharInfo::UnknownCharSet == using_param_charsets[num_params] )
                    {
                      // The character set attribute of a quoted string without a charset prefix is ...
                      if (sqlci_env != NULL)
                        using_param_charsets[num_params] = sqlci_env->getTerminalCharset();
                      else
                        using_param_charsets[num_params] = CharInfo::UnknownCharSet;
                    }
                    else {
                    // MP KANJI/KSC5601's params should be even in length
                      if ( CharInfo::is_NCHAR_MP(using_param_charsets[num_params]) &&
                           i % 2 != 0
                         ) {
		          *args = '\0';		// terminate errarg
		          return errorParams(-SQLCI_SYNTAX_ERROR, errarg);
                      }
                    }

		    if (i >= MAX_LEN_UNNAMED_PARAM)
		    {
		      *args = '\0';		// terminate errarg
		      return errorParams(-SQLCI_LEN_PARAM_OVERFLOW,
					       errarg,
					       MAX_LEN_UNNAMED_PARAM);
		    }
		    if (qseen == errarg)
		      return errorParams(-SQLCI_INPUT_MISSING_QUOTE,
					       errarg);
		    while (isspace((unsigned char)*args)) args++;    // go past trailing blanks// For VS2003
		    if (*args && *args != ',')
		    {
		      *++qseen = '\0';		// terminate errarg after end '
		      return errorParams(-SQLCI_PARAM_QUOTED_BAD_CONCAT,
					       errarg);
		    }
		  }
		  break;

      case 'x':
      case 'X':
        {
          // if this is a hex string, convert to hex and break out.
          // hex literal format:  x'hexval'
          //          Int32 arglen = strlen(args);
          //          if ((arglen > (1 + 1 + 1)) &&
          if (args[1] && args[1] == '\'')
            {
              Int32 j = 2;
              Int32 arglen = 0;
              while (args[j] && (NOT isHex) && (j < MAX_LEN_UNNAMED_PARAM))
                {
                  if (args[j] == '\'')
                    {
                      NAWString pvalue_in_wchar(CharInfo::ISO88591, 
                                                &args[2], arglen);
                      void* result = NULL;
                      enum hex_conversion_code code = 
                        verifyAndConvertHex(pvalue_in_wchar, 
                                            pvalue_in_wchar.length(), 
                                            L'\'', CharInfo::ISO88591, 
                                            &sqlci_Heap, result);
                      if (code == INVALID)
                        {
                          return errorParams(-SQLCI_SYNTAX_ERROR, errarg);
                        }
                      
                      NAString* conv_pvalue = (NAString*)result;
                      
                      str_cpy_all(param, (char*)conv_pvalue->data(), 
                                  conv_pvalue->length());
                      
                      i = conv_pvalue->length();

                      args += (1 + 1 + arglen + 1);
                      isHex = TRUE;
                    }
                  else
                    arglen++; 

                  j++;
                } // while

              if (isHex)
                break;
            }

          // fall through to default case.
        }

      default:	  // Copy as is, including embedded squotes and blanks
		  // (leading blanks were already removed above),
      		  // to param value buffer, until we hit a comma
		  // (the param value separator).
		  // Then remove trailing blanks.
		  //
		  // ## This is kinda weird, nonAnsi syntax, **AND**
		  // ## does not at all match the way sqlci_yacc rejects any of
		  // ##   SET PARAM ?p val ue string;
		  // ##   SET PARAM ?p val'ue'str''ing;
		  // ## Could definitely be fixed here for consistency --
		  // ## i.e. reject embedded punctuation, esp ' and blanks...
		  // ##
		  // ## We do of course accept
		  // ##   USING 'val ue string', 'val''ue''str''''ing';
		  // ## the same way sqlci_yacc accepts
		  // ##   SET PARAM ?p 'val ue string';
		  // ##   SET PARAM ?p 'val''ue''str''''ing';
		  // ## so that part's good.
		  {
		    for ( ;
		    	 *args && *args != ',' && i < MAX_LEN_UNNAMED_PARAM;
			 args++)
		      param[i++] = isspace((unsigned char)*args) ? ' ' : *args;  // For VS2003
		    while (isspace((unsigned char)*args)) args++;    // go past trailing blanks // For VS2003
		    if (i >= MAX_LEN_UNNAMED_PARAM)
		    {
		      if (*args && *args != ',')
		      {
			*args = '\0';			// terminate errarg
			if (&errarg[MAX_LEN_UNNAMED_PARAM+60] < args)
			  errarg[MAX_LEN_UNNAMED_PARAM+60] = '\0';
			return errorParams(-SQLCI_LEN_PARAM_OVERFLOW,
						 errarg,
						 MAX_LEN_UNNAMED_PARAM);
		      }
		    }
		    while (i && isspace((unsigned char)param[i-1]))	// Here, remove trailing// For VS2003
		      i--;				// blanks from param[].
                    literal = FALSE;

		    // ## Yuck, we also allow
		    // ##   USING,,;	equiv to  USING '','','';
		    // if (!i) return errorParams(-SQLCI_LEN_PARAM_UNDERFLOW);
		  }
    } // switch *args

    if (*args)
    {
      ComASSERT(*args == ',');				// all blanks are past;
      args++;						// go past the comma
      // Convert a final comma (with a blank value "after" it) to a blank,
      // to make a final unquoted blank value for our next (and last) while-iter
      if (!*args)
        *--args = ' ';
    }

    // dynamically get space for the param value and store it in the array
    param[i] = '\0';

    //if there's a NULL value and it was not included in quotes, the param
    //value is NULL.  Otherwise, allocate a space for the param value in
    //using_params
    if (!literal && !strcmp(param, "NULL"))
      using_params[num_params] = NULL;
    else if (isHex) {
      using_params[num_params] = new char[i+1];
      str_cpy_all(using_params[num_params], param, i);
      using_params[num_params][i] = 0;
    } else {
      using_params[num_params] = new char[i+1];
      strcpy(using_params[num_params], param);
    }
    num_params++;
  } // while *args

  argument_[arglen] = sentinel;		// reset back to ';' or '\0'

  if (!num_params)
    return errorParams(-SQLCI_NUM_PARAMS_UNDERFLOW);

  return 0;					// no error
}

QueryCacheSt::QueryCacheSt(short option)
: SqlCmd(SqlCmd::QUERYCACHE_TYPE, ""),
  option_(option)
{}

short SqlCmd::deallocate(SqlciEnv * sqlci_env, PrepStmt * prep_stmt)
{
  Lng32 retcode;

  // prepared statement exists. Deallocate it.
  retcode = SQL_EXEC_DeallocDesc(prep_stmt->getSqlSrc());
  if (prep_stmt->getSqlSrc())
  {
    if (prep_stmt->getSqlSrc()->module)
      delete (SQLMODULE_ID *)prep_stmt->getSqlSrc()->module;
    prep_stmt->getSqlSrc()->module = NULL;
  }
  HandleCLIError(retcode, sqlci_env);
  
  retcode = SQL_EXEC_DeallocDesc(prep_stmt->getInputDesc());
  HandleCLIError(retcode, sqlci_env);
  
  retcode = SQL_EXEC_DeallocDesc(prep_stmt->getOutputDesc());
  HandleCLIError(retcode, sqlci_env);
  
  retcode = SQL_EXEC_DeallocStmt(prep_stmt->getStmt());
  if (prep_stmt->getStmt())
  {
    if (prep_stmt->getStmt()->module)
      delete (SQLMODULE_ID *)prep_stmt->getStmt()->module;
    if (prep_stmt->getStmt()->identifier)
      delete [] (char *)prep_stmt->getStmt()->identifier;
    prep_stmt->getStmt()->identifier = NULL;
  }

  HandleCLIError(retcode, sqlci_env);
  
  if (prep_stmt)
    delete prep_stmt;
  prep_stmt = NULL;

  return retcode;
}



/////////////////////////////////////////////////////////////////
// ::process() methods
/////////////////////////////////////////////////////////////////

short DML::process(SqlciEnv * sqlci_env)
{
  Lng32 retcode = 0, prepcode = 0, retcodeExe = 0;

  // Bookkeeping for stored procedure result sets
  NABoolean isResultSet = (type == DML_CALL_STMT_RS_TYPE ? TRUE : FALSE);
  NABoolean skipStats = isResultSet;

  if (!isResultSet)
    HandleCLIErrorInit();

  char dml_stmt_name[50];
  // if this_stmt_name is not NULL, then this method was invoked by
  // an sqlci internal method (like stats...etc).
  if (! this_stmt_name)
    strcpy(dml_stmt_name, "__SQLCI_DML_LAST__");   // generate a dummy name
  else
    strcpy(dml_stmt_name, this_stmt_name);

  PrepStmt * prep_stmt;
  if (prep_stmt = sqlci_env->get_prep_stmts()->get(dml_stmt_name))
    {
      // this statement name should not be present in the list of prepared
      // stmts as it is an internal name. If we reach this error, it
      // means that someone has prepared a statement with the same
      // name as what we used internally. We need to fix our internal
      // name.
      sqlci_env->diagsArea() << DgSqlCode(-SQLCI_INTERNAL_ERROR);
      return 0;
    }

  prep_stmt = new PrepStmt(dml_stmt_name, type);

  if (!skipStats)
    sqlci_env->getStats()->startStats(prep_stmt);

  NABoolean resetLastExecStmt = (isResultSet ? FALSE : TRUE);
 Int32 prepareCode = 0;
 Lng32 statisticsType = SQLCLI_NO_STATS;
 prepcode = do_prepare(sqlci_env, prep_stmt, get_sql_stmt(),
			resetLastExecStmt, rsIndex_,&prepareCode, &statisticsType);
  if (statisticsType == SQLCLI_NO_STATS)
    skipStats = TRUE;
  if (!skipStats)
    sqlci_env->getStats()->startExeStats();

  if (prepcode >= 0)
  {
    NABoolean skipExec = FALSE;
      char * po = sqlci_env->getPrepareOnly();
      char * eo = sqlci_env->getExecuteOnly();
      if (!po && !eo)
	{
	  retcodeExe = do_execute(sqlci_env, prep_stmt, 0, NULL, NULL, prepareCode);
	}
      else
	{
	  // only adding prepareOnly for DML. Other cases could
	  // be added later.
	  if ((po) && (strcmp(po, "DML") == 0) &&
	      ((prep_stmt->getType() == DML_SELECT_TYPE) ||
	       (prep_stmt->getType() == DML_UPDATE_TYPE) ||
	       (prep_stmt->getType() == DML_INSERT_TYPE) ||
	       (prep_stmt->getType() == DML_DELETE_TYPE)))
	    {
	      // don't execute in this case.
	      sqlci_env->lastExecutedStmt() = NULL;

	      Logfile *log = sqlci_env->get_logfile();
	      if (!lastLineWasABlank) log->WriteAll("");
	      log->WriteAll(PREPARED_MESSAGE);
	    }
	  else
	    {
	      retcodeExe = do_execute(sqlci_env, prep_stmt);
	    }
	}
  }

  if (!skipStats)
  {
    sqlci_env->getStats()->endExeStats();
    sqlci_env->getStats()->endStats(sqlci_env);
  }

  // if stats were collected for this statement, then do not
  // deallocate it. Deallocating the stmt will destroy the stats area
  // in executor and a subsequent 'display statistics' command will
  // not be able to display stats.
  // This statement will be deallocated when the next standalone
  // statement is issued from sqlci.
  
  NABoolean deallocStmt =  ((prepcode >= 0) ? TRUE : FALSE);

  // do not display stats for internal invocations. Do it for user
  // entered standalone non-prepared queries only.
  if ((this_stmt_name == NULL) && (deallocStmt) &&
      (sqlci_env->lastExecutedStmt()) &&
      (statisticsType != SQLCLI_NO_STATS))
    {
      sqlci_env->getStats()->displayStats(sqlci_env);
    }

  if (deallocStmt)
    {
      retcode = deallocate(sqlci_env, prep_stmt);
      // if the stmt has already been deallocated then 
      // there is no need to deallocate it in the interrupt handler.
      if (!isResultSet)
      {
        if (! retcode)
          global_sqlci_env->resetDeallocateStmt() ;
        sqlci_env->lastExecutedStmt() = NULL;
      }
    }

  if ( breakReceived )
    {
      breakReceived = 0;
      
      if ((prepcode >= 0) && (retcodeExe != SQL_Canceled) && (retcodeExe != SQL_Rejected))
        {
          sqlci_env->diagsArea() << DgSqlCode(SQLCI_BREAK_RECEIVED, DgSqlCode::WARNING_);
          return SQL_Canceled;
        }
    }

  // TEMP TEMP
  // Sometimes inserted rows are not being flushed out. That causes selected
  // rows to not get returned.
  // Temporarily add a one sec delay so it can get flushed out.
  // Do this only for regressions run.
  if ((prep_stmt) && (prep_stmt->getType() == DML_INSERT_TYPE) &&
      (getenv("SQLMX_REGRESS")))
    {
      //DELAY(100);
    }
  
  if ((prepcode >= 0) && (retcodeExe == SQL_Canceled))
    {
      return (short)retcodeExe;
    }
  else
    {
      if (prepcode)
        return 1;
      else
        return 0;
    } 
} // end DML::process


/////////////////////////////////////////////
// Begin PREPARE
//////////////////////////////////////////////
short Prepare::process(SqlciEnv * sqlci_env)
{
  Lng32 retcode;
  HandleCLIErrorInit();
  PrepStmt * prep_stmt;

  if (prep_stmt = sqlci_env->get_prep_stmts()->get(this_stmt_name))
    {
      // prepared statement exists. Deallocate it.
      retcode = SQL_EXEC_DeallocDesc(prep_stmt->getSqlSrc());
      HandleCLIError(retcode, sqlci_env);

      retcode = SQL_EXEC_DeallocDesc(prep_stmt->getInputDesc());
      HandleCLIError(retcode, sqlci_env);

      retcode = SQL_EXEC_DeallocDesc(prep_stmt->getOutputDesc());
      HandleCLIError(retcode, sqlci_env);

      retcode = SQL_EXEC_DeallocStmt(prep_stmt->getStmt());
      HandleCLIError(retcode, sqlci_env);

      sqlci_env->get_prep_stmts()->remove(this_stmt_name);
    }

  prep_stmt = new PrepStmt(this_stmt_name, type);

  sqlci_env->getStats()->startStats(prep_stmt);
  retcode = do_prepare(sqlci_env, prep_stmt, get_sql_stmt());
  sqlci_env->getStats()->endStats(sqlci_env);
  
  // if do_prepare was successfull then reset 
  // the deallocate stmt in global_sqlci_env
  // in case of a break.
  if (retcode >= 0)
  {
    global_sqlci_env->resetDeallocateStmt() ;
  }


  if (retcode >= 0)
    {
      sqlci_env->get_prep_stmts()->append(prep_stmt);

      Logfile *log = sqlci_env->get_logfile();
      if (!lastLineWasABlank) log->WriteAll("");
      log->WriteAll(PREPARED_MESSAGE);
    }

  // This is Prepare; no Exe time at all
  sqlci_env->getStats()->startExeStats();
  sqlci_env->getStats()->endExeStats();

  sqlci_env->getStats()->displayStats(sqlci_env);

  return 0;
}

// Prepare::process has already set up the PrepStmt by calling
// DescribeStmt CLI call and allocate PrepStmt. Some descriptor items
// has already been extracted from the output descriptor, such as
// type, length, precision, scale.
///////////////////////////////////////////////////////
// Begin DESCRIBESTMT
//////////////////////////////////////////////////////
short DescribeStmt::displayEntries(SqlciEnv *sqlci_env,
                                   SQLDESC_ID *desc, Lng32 numEntries,
                                   Logfile *log)
{
  char scratchBuffer[512];
  char charVal[ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN+1];
  Lng32 intVal;
  Lng32 retcode;

  // For each entry produce the following output
  //
  // [i] NAME "column-name", HEADING "column-heading"
  //     TYPE n, FS n, LEN n, OCTLEN n, CHARSET n
  //     PREC n, LPREC n, SCALE n, DTCODE n, NULL n, MODE n, IDX n, ORDPOS n
  //     CAT "catalog-name", SCH "schema-name", TABLE "table-name"
  
  for (Lng32 entry = 1; entry <= numEntries; entry++)
  {
    sprintf(scratchBuffer, "[%d] ", entry);
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_NAME,
                                   0, charVal,
                                   ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN,
                                   &intVal, 0);
    HandleCLIError(retcode, sqlci_env);
    charVal[intVal] = 0;
    sprintf(scratchBuffer, "NAME \"%s\", ", charVal); 
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_HEADING,
                                   0, charVal,
                                   ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN,
                                   &intVal, 0);
    HandleCLIError(retcode, sqlci_env);
    charVal[intVal] = 0;
    sprintf(scratchBuffer, "HEADING \"%s\"", charVal); 
    log->WriteAll(scratchBuffer);
    
    // ------------------------------------------------
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_TYPE,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "    TYPE %d, ", intVal);
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_TYPE_FS,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "FS %d, ", intVal);
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_LENGTH,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "LEN %d, ", intVal);
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_OCTET_LENGTH,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "OCTLEN %d, ", intVal);
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_CHAR_SET,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "CHARSET %d", intVal);
    log->WriteAll(scratchBuffer);
    
    // ------------------------------------------------
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_PRECISION,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "    PREC %d, ", intVal);
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_INT_LEAD_PREC,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "LPREC %d, ", intVal);
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_SCALE,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "SCALE %d, ", intVal);
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_DATETIME_CODE,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "DTCODE %d, ", intVal);
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_NULLABLE,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "NULL %d, ", intVal);
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_PARAMETER_MODE,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "MODE %d, ", intVal);
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_PARAMETER_INDEX,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "IDX %d, ", intVal);
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_ORDINAL_POSITION,
                                   &intVal, 0, 0, 0, 0);
    HandleCLIError(retcode, sqlci_env);
    sprintf(scratchBuffer, "ORDPOS %d", intVal);
    log->WriteAll(scratchBuffer);
    
    // ------------------------------------------------
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_CATALOG_NAME,
                                   0, charVal,
                                   ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN,
                                   &intVal, 0);
    HandleCLIError(retcode, sqlci_env);
    charVal[intVal] = 0;
    sprintf(scratchBuffer, "    CAT \"%s\", ", charVal); 
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_SCHEMA_NAME,
                                   0, charVal,
                                   ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN,
                                   &intVal, 0);
    HandleCLIError(retcode, sqlci_env);
    charVal[intVal] = 0;
    sprintf(scratchBuffer, "SCH \"%s\", ", charVal); 
    log->WriteAllWithoutEOL(scratchBuffer);
    
    retcode = SQL_EXEC_GetDescItem(desc, entry,
                                   SQLDESC_TABLE_NAME,
                                   0, charVal,
                                   ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN,
                                   &intVal, 0);
    HandleCLIError(retcode, sqlci_env);
    charVal[intVal] = 0;
    sprintf(scratchBuffer, "TABLE \"%s\"", charVal); 
    log->WriteAll(scratchBuffer);
    
  }
  
  return 0;
}

short DescribeStmt::process(SqlciEnv * sqlci_env)
{
  HandleCLIErrorInit();
  PrepStmt *prep_stmt;

  if (!(prep_stmt = sqlci_env->get_prep_stmts()->get(stmtName_)))
  {
    sqlci_env->diagsArea() << DgSqlCode(-SQLCI_STMT_NOT_FOUND)
                           << DgString0(stmtName_);
    return 0;
  }

  sqlci_env->getStats()->startStats(prep_stmt);
  sqlci_env->getStats()->startExeStats();

  // meat of the body
  Logfile *log = sqlci_env->get_logfile();
  SQLDESC_ID * input_desc  = prep_stmt->getInputDesc();
  SQLDESC_ID * output_desc = prep_stmt->getOutputDesc();
  Lng32 num_input_entries = prep_stmt->numInputEntries();
  Lng32 num_output_entries = prep_stmt->numOutputEntries();

  if (num_input_entries > 0)
  {
    log->WriteAll("---Describing the INPUT entries---");
    displayEntries(sqlci_env, input_desc, num_input_entries, log);
  }

  if (num_output_entries > 0)
  {
    log->WriteAll("---Describing the OUTPUT entries---");
    displayEntries(sqlci_env, output_desc, num_output_entries, log);
  }

  // eye marker
  sqlci_env->getStats()->endExeStats();
  sqlci_env->getStats()->endStats(sqlci_env);

  sqlci_env->getStats()->displayStats(sqlci_env);

  return 0;
}

short StoreExplain::process(SqlciEnv * sqlci_env)
{
  Lng32 retcode;
  HandleCLIErrorInit();
  PrepStmt * prep_stmt;

  if (!(prep_stmt = sqlci_env->get_prep_stmts()->get(get_sql_stmt())))
    {
      sqlci_env->diagsArea() << DgSqlCode(-SQLCI_STMT_NOT_FOUND)
			     << DgString0(get_sql_stmt());
      return 0;
    }

  retcode = updateRepos(sqlci_env, prep_stmt->getStmt(), prep_stmt->uniqueQueryId());

  char donemsg[100];
  donemsg[0] = '\0';

  if (retcode == 0)
    sprintf(donemsg, OP_COMPLETE_MESSAGE);
  else if (retcode > 0)
    sprintf(donemsg, OP_COMPLETED_WARNINGS);
  else
    sprintf(donemsg, OP_COMPLETED_ERRORS);
    
  Logfile *log = sqlci_env->get_logfile();
  if (!lastLineWasABlank)
    {
      log->WriteAll("");
      lastLineWasABlank = TRUE;
    }

  if (donemsg[0] != '\0')
    {
      log->WriteAll(donemsg);
      lastLineWasABlank = FALSE;
    }

  return 0;
}

///////////////////////////////////////////
/// Begin Execute
///////////////////////////////////////////

short Execute::process(SqlciEnv * sqlci_env)
{
  Lng32 retcode;
  HandleCLIErrorInit();
  PrepStmt * prep_stmt;

  if (!(prep_stmt = sqlci_env->get_prep_stmts()->get(this_stmt_name)))
    {
      sqlci_env->diagsArea() << DgSqlCode(-SQLCI_STMT_NOT_FOUND)
			     << DgString0(this_stmt_name);
      return 0;
    }

  sqlci_env->getStats()->startStats(prep_stmt);
  sqlci_env->getStats()->startExeStats();

  retcode = do_execute(sqlci_env, prep_stmt, 
		       getNumParams(),
		       getUnnamedParamArray(),
		       getUnnamedParamCharSetArray());

  sqlci_env->getStats()->endExeStats();
  sqlci_env->getStats()->endStats(sqlci_env);

  sqlci_env->getStats()->displayStats(sqlci_env);

  return 0;
}

//////////////////////////////////////////
// Begin Cursor
//////////////////////////////////////////

// return of -1 is error, cursor operation was not done.
// return of 0 is returned with some cursor/cli operation done.
//           retcode contains indication if it succeeded(=0) or failed(!= 0)
short Cursor::declareC(SqlciEnv * sqlci_env, char * donemsg, 
		       Lng32 &retcode)
{
  short result = 0;

  HandleCLIErrorInit();

  if (resultSetIndex_ > 0)
    result = declareCursorStmtForRS(sqlci_env, retcode);
  else
    result = declareCursorStmt(sqlci_env, retcode);

  if (donemsg)
  {
    if (retcode >= 0)
      sprintf(donemsg, OP_COMPLETE_MESSAGE);
    else
      sprintf(donemsg, OP_COMPLETED_ERRORS);
  }

  // This is Declare; no Exe time at all
  sqlci_env->getStats()->startExeStats();
  sqlci_env->getStats()->endExeStats();
  sqlci_env->getStats()->displayStats(sqlci_env);

  return result;
}

// return of -1 is error, cursor operation was not done.
// return of 0 is returned with some cursor/cli operation done.
//           retcode contains indication if it succeeded(=0) or failed(!= 0)
short Cursor::declareCursorStmt(SqlciEnv *sqlci_env, Lng32 &retcode)
{
  short result = 0;
  CursorStmt * cursor;
  PrepStmt * prepStmt = NULL;

  retcode = SQL_Success;
  if (! internalPrepare_)
  {
    // make sure that the prepared stmt exists.
    if (! (prepStmt = sqlci_env->get_prep_stmts()->get(get_sql_stmt())))
    {
      sqlci_env->diagsArea() << DgSqlCode(-SQLCI_STMT_NOT_FOUND)
                             << DgString0(get_sql_stmt());
      return -1;
    }
  }
  
  if (cursor = sqlci_env->getCursorList()->get(cursorName_))
  {
    cleanupCursorStmt(sqlci_env, cursor);
    cursor = NULL;
  }
  
  if (internalPrepare_)
  {
    NAString nameForPrepStmt("__SQLCI_PREPSTMT_FOR_CURSOR_");
    nameForPrepStmt += cursorName_;
    nameForPrepStmt += "__";
    prepStmt = new PrepStmt(nameForPrepStmt.data());
    
    sqlci_env->getStats()->startStats(prepStmt);
    
    // sqlci makes all SELECT's to be readonly by issuing a
    // control command at startup time. Since this is an explicit
    // DECLARE cursor statement which could later be updated/deleted
    // using an "upd/del...where current of" statement, temporarily
    // turn the readonly_cursor off. Turn it back on after this
    // query has been prepared.
    // If this is not an  internal prepare, then users will have to
    // specify the FOR UPDATE OF clause when they prepare the SELECT.
    // Otherwise they will not be able to do a cursor update/delete.
    retcode = 
      executeQuery("CONTROL QUERY DEFAULT READONLY_CURSOR 'FALSE';",
                   sqlci_env);
    
    if (retcode >= 0)		// QSTUFF
    {
      Lng32 prepcode = do_prepare(sqlci_env, prepStmt, get_sql_stmt());
      
      retcode = 
        executeQuery("CONTROL QUERY DEFAULT READONLY_CURSOR 'TRUE';",
                     sqlci_env);
      
      if (retcode >= 0 && prepcode)
        retcode = prepcode;
    }
    
    sqlci_env->getStats()->endStats(sqlci_env);
  }
  else
    retcode = 0;

  ComASSERT(prepStmt);
  
  if (retcode >= 0)
  {
    // Make a copy of the cursor name
    UInt32 nameLen = strlen(cursorName_);
    char *identifier = new char[nameLen + 1];
    strcpy(identifier, cursorName_);

    // Make a statement ID for the new cursor
    SQLSTMT_ID *cursorStmtId = new SQLSTMT_ID;
    memset(cursorStmtId, 0, sizeof(SQLSTMT_ID));
    cursorStmtId->version = SQLCLI_CURRENT_VERSION;
    cursorStmtId->name_mode = cursor_name;
    cursorStmtId->identifier_len = (Lng32) nameLen;
    cursorStmtId->identifier = identifier;
    cursorStmtId->handle = 0;
    cursorStmtId->module = prepStmt->getStmt()->module;
    
    retcode = SQL_EXEC_AllocStmt(cursorStmtId, prepStmt->getStmt());
    HandleCLIError(retcode, sqlci_env);
    
    // QSTUFF:  set the holdable cursor flag if necessary
    if (isHoldable())
    {
      Lng32 attrValue = SQL_HOLDABLE;
      
      retcode = SQL_EXEC_SetStmtAttr(cursorStmtId,
                                     SQL_ATTR_CURSOR_HOLDABLE,
                                     attrValue, NULL);
      HandleCLIError(retcode, sqlci_env);
      if (retcode < 0)
      {
        Lng32 cleanupRetcode;
        cleanupRetcode = SQL_EXEC_DeallocDesc(prepStmt->getSqlSrc());
        HandleCLIError(cleanupRetcode, sqlci_env);
        
        cleanupRetcode = SQL_EXEC_DeallocDesc(prepStmt->getInputDesc());
        HandleCLIError(cleanupRetcode, sqlci_env);
        
        cleanupRetcode = SQL_EXEC_DeallocDesc(prepStmt->getOutputDesc());
        HandleCLIError(cleanupRetcode, sqlci_env);
        
        cleanupRetcode = SQL_EXEC_DeallocStmt(prepStmt->getStmt());
        HandleCLIError(cleanupRetcode, sqlci_env);
      }
    } // if (isHoldable())
    
    if (retcode >= 0)
    {
      cursor = new CursorStmt(cursorName_, cursorStmtId,
                              prepStmt, internalPrepare_);
      
      sqlci_env->getCursorList()->append(cursor);
    }
    else 
    {
      delete [] identifier;
      delete cursorStmtId;
    }
  }

  return result;
} // Cursor::declareCursorStmt

// Create a CursorStmt object for a stored procedure result set
// return of -1 is error, cursor operation was not done.
// return of 0 is returned with some cursor/cli operation done.
//           retcode contains indication if it succeeded(=0) or failed(!= 0)
short Cursor::declareCursorStmtForRS(SqlciEnv * sqlci_env, Lng32 &retcode)
{
  short result = 0; // the return value of this function
  PrepStmt *prepStmt = NULL;
  CursorStmt *cursor = NULL;
  CursorStmt *parentCursor = NULL;
  char *nameOfParent = get_sql_stmt();

  retcode = SQL_Success;

  // This is a stored procedure result set cursor. The
  // parent must be another cursor, not a prepared
  // statement.
  parentCursor = sqlci_env->getCursorList()->get(nameOfParent);
  if (!parentCursor)
  {
    sqlci_env->diagsArea() << DgSqlCode(-SQLCI_STMT_NOT_FOUND)
                           << DgString0(nameOfParent);
    return -1;
  }

  if (cursor = sqlci_env->getCursorList()->get(cursorName_))
  {
    cleanupCursorStmt(sqlci_env, cursor);
    cursor = NULL;
  }
  
  // Create a PrepStmt for this result set. It will be used later to
  // store the column descriptions. It doesn't get used for anything
  // else.
  NAString nameForPrepStmt("__SQLCI_PREPSTMT_FOR_CURSOR_");
  nameForPrepStmt += cursorName_;
  nameForPrepStmt += "__";
  prepStmt = new PrepStmt(nameForPrepStmt.data());
    
  // Give this PrepStmt instance a dummy name, so that SHOW CURSOR
  // has something to display
  char buf[100];
  sprintf(buf, "Result set %d, child of ", (Lng32) resultSetIndex_);
  NAString dummyName(buf);
  dummyName += nameOfParent;

  // Push values into the PrepStmt instance. Information about the
  // output columns will be filled in later. It becomes available only
  // after the parent CALL statement executes.
  prepStmt->set((char *) dummyName.data(),
                NULL,   // SQLDESC_ID *sql_src_
                NULL,   // SQLSTMT_ID *stmt_
                0,      // Lng32 numInputEntries
                NULL,   // SQLDESC_ID *input_desc_
                0,      // Lng32 numOutputEntries
                NULL);  // SQLDESC_ID *output_desc_
  
  // Make a statement ID for the new result set statement
  SQLSTMT_ID *childStmtId = new SQLSTMT_ID;
  SQLSTMT_ID *parentStmtId = parentCursor->cursorStmtId();
  *childStmtId = *parentStmtId;
  childStmtId->name_mode = stmt_handle;
  childStmtId->identifier = NULL;
  childStmtId->identifier_len = 0;

  retcode = SQL_EXEC_AllocStmtForRS(parentStmtId,
                                    resultSetIndex_,
                                    childStmtId);
  HandleCLIError(retcode, sqlci_env);
  
  if (retcode >= 0)
  {
    cursor = new CursorStmt(cursorName_, childStmtId,
                            prepStmt, FALSE);
    cursor->setResultSetIndex(resultSetIndex_);
    sqlci_env->getCursorList()->append(cursor);
  }
  else
    delete prepStmt;

  return result;
} // Cursor::decalreCursorStmtForRS

// return of -1 is error, cursor operation was not done.
// return of 0 is returned with some cursor/cli operation done.
//           retcode contains indication if it succeeded(=0) or failed(!= 0)
short Cursor::open(SqlciEnv * sqlci_env, char * donemsg,
		   Lng32 &retcode)
{
  CursorStmt * cursor;

  HandleCLIErrorInit();

  retcode = SQL_Success;
  if (! (cursor = sqlci_env->getCursorList()->get(cursorName_)))
    {
      sqlci_env->diagsArea() << DgSqlCode(-SQLCI_STMT_NOT_FOUND)
			     << DgString0(cursorName_);
      return -1;
    }
  
  retcode = doExec(sqlci_env, 
		   cursor->cursorStmtId(),
		   cursor->prepStmt());

  if (retcode >= 0)
  {
    // If this is a stored procedure result set cursor then we now
    // need to DESCRIBE the statement. The DESCRIBE can only be done
    // reliably after the CALL statement executes, not necessarily at
    // the time the cursor is DECLAREd.
    if (cursor->getResultSetIndex() > 0)
    {
      PrepStmt *p = cursor->prepStmt();
      Lng32 num_output_entries = 0;
      SQLDESC_ID *output_desc = p->getOutputDesc();
      
      if (!output_desc)
      {
        output_desc = new SQLDESC_ID;
        memset(output_desc, 0, sizeof(SQLDESC_ID));
        init_SQLDESC_ID(output_desc,
                        SQLCLI_CURRENT_VERSION,
                        desc_handle,
                        cursor->cursorStmtId()->module);
        
        retcode = SQL_EXEC_AllocDesc(output_desc, 500);
        HandleCLIError(retcode, sqlci_env);
      }
      
      if (retcode >= 0)
      {
        retcode = SQL_EXEC_DescribeStmt(cursor->cursorStmtId(),
                                        NULL,
                                        output_desc);
        HandleCLIError(retcode, sqlci_env);
      }
      
      if (retcode >= 0)
      {
	 retcode = SQL_EXEC_GetDescEntryCount(output_desc,
                                             &num_output_entries);
	 HandleCLIError(retcode, sqlci_env);
      }
      
      if (retcode >= 0)
      {
	  p->setOutputDesc(num_output_entries, output_desc);
	  addOutputInfoToPrepStmt(sqlci_env, p);
      }
      else 
	delete output_desc;

    } // if (cursor->getResultSetIndex() > 0)
  } // if (retcode >= 0)
  
  if (retcode >= 0)
  {
    if (donemsg)
      sprintf(donemsg, OP_COMPLETE_MESSAGE);
  }
  else
  {
    if (donemsg)
      sprintf(donemsg, OP_COMPLETED_ERRORS);
  }
  
  return 0;
}

// return of -1 is error, cursor operation was not done.
// return of 0 is returned with some cursor/cli operation done.
//           retcode contains indication if it succeeded(=0) or failed(!= 0)
short Cursor::fetch(SqlciEnv * sqlci_env,
		    NABoolean doDisplayRow,
		    char * donemsg,
		    Lng32 &retcode)
{
  CursorStmt * cursor;

  HandleCLIErrorInit();

  retcode = SQL_Success;
  if (! (cursor = sqlci_env->getCursorList()->get(cursorName_)))
    {
      sqlci_env->diagsArea() << DgSqlCode(-SQLCI_STMT_NOT_FOUND)
			     << DgString0(cursorName_);
      return -1;
    }

  PrepStmt *p = cursor->prepStmt(); 
  if (p && p->numOutputEntries() < 1)
    doDisplayRow = FALSE;

  retcode = doFetch(sqlci_env, 
		    cursor->cursorStmtId(),
		    p);
  if (Succ_or_Warn)
  {
    if (doDisplayRow)
    {
      displayHeading(sqlci_env, p);
      
      displayRow(sqlci_env, p);
      
      Logfile *log = sqlci_env->get_logfile();
      log->WriteAll(p->outputBuf());
      lastLineWasABlank = FALSE;
    }
    
    if (donemsg)
      sprintf(donemsg, SELECTED_MESSAGE, 1);
  }
  else
  {
    if (donemsg)
      sprintf(donemsg, SELECTED_MESSAGE, 0);
  }
  
  return 0;
}

// return of -1 is error, cursor operation was not done.
// return of 0 is returned with some cursor/cli operation done.
//           retcode contains indication if it succeeded(=0) or failed(!= 0)
short Cursor::close(SqlciEnv * sqlci_env, char * donemsg,
		    Lng32 &retcode)
{
  CursorStmt * cursor;

  HandleCLIErrorInit();

  retcode = SQL_Success;
  if (! (cursor = sqlci_env->getCursorList()->get(cursorName_)))
    {
      sqlci_env->diagsArea() << DgSqlCode(-SQLCI_STMT_NOT_FOUND)
			     << DgString0(cursorName_);
      return -1;
    }
  
  retcode = SQL_EXEC_CloseStmt(cursor->cursorStmtId());
  HandleCLIError(retcode, sqlci_env);
  if (retcode > 0)
     getRowsAffected(cursor->cursorStmtId());  
  if (donemsg)
    sprintf(donemsg, OP_COMPLETE_MESSAGE);
  
  return 0;
}

// return of -1 is error, cursor operation was not done.
// return of 0 is returned with some cursor/cli operation done.
//           retcode contains indication if it succeeded(=0) or failed(!= 0)
short Cursor::dealloc(SqlciEnv * sqlci_env, char * donemsg,
		      Lng32 &retcode)
{
  CursorStmt * cursor;

  HandleCLIErrorInit();

  retcode = SQL_Success;
  if (! (cursor = sqlci_env->getCursorList()->get(cursorName_)))
  {
    sqlci_env->diagsArea() << DgSqlCode(-SQLCI_STMT_NOT_FOUND)
                           << DgString0(cursorName_);
    return -1;
  }

  cleanupCursorStmt(sqlci_env, cursor);

  if (donemsg)
    sprintf(donemsg, OP_COMPLETE_MESSAGE);
  
  return 0;
}
  
// Helper function to do all cleanup when a CursorStmt is no longer
// needed.
void Cursor::cleanupCursorStmt(SqlciEnv *sqlci_env, CursorStmt *c)
{
  if (!c)
    return;

  Lng32 retcode = 0;
  SQLSTMT_ID *cursorStmtId = c->cursorStmtId();

  // Deallocate the underlying SQL statement if this is a stored
  // procedure result set
  NABoolean isResultSet = (c->getResultSetIndex() > 0 ? TRUE : FALSE);
  if (isResultSet && cursorStmtId)
  {
    retcode = SQL_EXEC_DeallocStmt(cursorStmtId);
    HandleCLIError(retcode, sqlci_env);
  }
  
  // The PrepStmt for this cursor should be deallocated if either of
  // the following are true
  // - it was internally prepared
  //      (e.g. "declare cursor C for select * from t")
  // - this is a stored procedure result set

  PrepStmt *prepStmt = c->prepStmt();
  
  if (prepStmt && (c->internallyPrepared() || isResultSet))
  {
    SQLDESC_ID *sqlSrcDesc = prepStmt->getSqlSrc();
    SQLDESC_ID *inDesc = prepStmt->getInputDesc();
    SQLDESC_ID *outDesc = prepStmt->getOutputDesc();
    SQLSTMT_ID *stmtId = prepStmt->getStmt();
    
    if (sqlSrcDesc)
    {
      retcode = SQL_EXEC_DeallocDesc(sqlSrcDesc);
      HandleCLIError(retcode, sqlci_env);
    }
    
    if (inDesc)
    {
      retcode = SQL_EXEC_DeallocDesc(inDesc);
      HandleCLIError(retcode, sqlci_env);
    }
    
    if (outDesc)
    {
      retcode = SQL_EXEC_DeallocDesc(outDesc);
      HandleCLIError(retcode, sqlci_env);
    }
    
    if (stmtId)
    {
      retcode = SQL_EXEC_DeallocStmt(stmtId);
      HandleCLIError(retcode, sqlci_env);
    }
  }
  
  // Remove c from the cursor list. This will also call the destructor
  // for c.
  sqlci_env->getCursorList()->remove(c->cursorName());
}

short Cursor::process(SqlciEnv * sqlci_env)
{
  short rc = 0;

  if ((NOT internalCursor_) &&
      (! getenv("SQLCI_CURSOR")))	//## rename this to SQL_MXCI_CURSOR ...
    {
      sqlci_env->diagsArea() << DgSqlCode(-SQLCI_CURSOR_NOT_SUPPORTED);
      return 0;
    }

  Lng32 retcode = 0;
  char donemsg[100];
  donemsg[0] = '\0';

  switch (operation_)
    {
    case DECLARE:
      rc = declareC(sqlci_env, donemsg, retcode);
      break;

    case OPEN:
      rc = open(sqlci_env, donemsg, retcode);
      break;
    
    case FETCH:
      rc = fetch(sqlci_env, TRUE, donemsg, retcode);
      break;
    
    case CLOSE:
      rc = close(sqlci_env, donemsg, retcode);
      break;

    case DEALLOC:
      rc = dealloc(sqlci_env, donemsg, retcode);
      break;
    }

  // All the CURSOR methods return 0 or -1. Write the completion
  // message when (rc == 0). In case of rs==-1, the diags is filled with
  // error conditions.
  if (rc == 0)
  {
    Logfile *log = sqlci_env->get_logfile();
    if (!lastLineWasABlank) log->WriteAll("");
    if (donemsg[0] != '\0') log->WriteAll(donemsg);
  }

  return 0;
}
// unified display_qc to display data as follows:
// CURSIZE    MAXSIZE    ENTRIES     PLANS       LOOKUPS     HITS        HITRATE
// ---------  ---------  ----------  ----------  ----------  ----------  -------
// 429.49MB   29.49GB    858992      858992      429.496E+7  420.000E+7  97.78%
// hit rate = hits / lookups 
static const char * query_cache_query = 
" SELECT " 
" (CASE WHEN CURRENT_SIZE > 1048575 THEN "
"  CONCAT(RTRIM(CAST(CAST(CURRENT_SIZE/1048576 AS NUMERIC(7,2)) AS CHAR(7) CHARACTER SET ISO88591)), _ISO88591'GB') ELSE "
"  (CASE WHEN CURRENT_SIZE > 1023 THEN "  
"   CONCAT(RTRIM(CAST(CAST(CURRENT_SIZE/1024 AS NUMERIC(7,2)) AS CHAR(7) CHARACTER SET ISO88591)), _ISO88591'MB') ELSE "
"    CONCAT(RTRIM(CAST(CURRENT_SIZE AS CHAR(7) CHARACTER SET ISO88591)), _ISO88591'KB') "
"  END) "
" END) AS CURSIZE, "
" (CASE WHEN MAX_CACHE_SIZE > 1048575 THEN " 
"  CONCAT(RTRIM(CAST(CAST(MAX_CACHE_SIZE/1048576 AS NUMERIC(7,2)) AS CHAR(7) CHARACTER SET ISO88591)), _ISO88591'GB') ELSE "
"  (CASE WHEN MAX_CACHE_SIZE > 1023 THEN " 
"   CONCAT(RTRIM(CAST(CAST(MAX_CACHE_SIZE/1024 AS NUMERIC(7,2)) AS CHAR(7) CHARACTER SET ISO88591)), _ISO88591'MB') ELSE "
"    CONCAT(RTRIM(CAST(MAX_CACHE_SIZE AS CHAR(7) CHARACTER SET ISO88591)), _ISO88591'KB') "
"  END) "
" END) AS MAXSIZE, "
" (CASE WHEN NUM_ENTRIES + TEXT_ENTRIES > 9999999 THEN "
"  CONCAT(RTRIM(CAST(CAST((NUM_ENTRIES + TEXT_ENTRIES)/10000000 AS NUMERIC(7,3)) "
"   AS CHAR(7) CHARACTER SET ISO88591)), _ISO88591'E+7') ELSE " 
"  CAST(NUM_ENTRIES + TEXT_ENTRIES AS CHAR(7) CHARACTER SET ISO88591) "
" END) AS ENTRIES, "
" (CASE WHEN NUM_PLANS > 9999999 THEN "
"  CONCAT(RTRIM(CAST(CAST((NUM_PLANS)/10000000 AS NUMERIC(7,3)) "
"   AS CHAR(7) CHARACTER SET ISO88591)), _ISO88591'E+7') ELSE "
"  CAST(NUM_PLANS AS CHAR(7) CHARACTER SET ISO88591) " 
" END) AS PLANS, "
" (CASE WHEN NUM_LOOKUPS > 9999999 THEN "
"  CONCAT(RTRIM(CAST(CAST(NUM_LOOKUPS/10000000 AS NUMERIC(7,3)) AS CHAR(7) CHARACTER SET ISO88591)), _ISO88591'E+7') ELSE " 
"  CAST(NUM_LOOKUPS AS CHAR(7) CHARACTER SET ISO88591) "
" END) AS LOOKUPS, "
" (CASE WHEN NUM_CACHE_HITS_PARSING + NUM_CACHE_HITS_BINDING + TEXT_CACHE_HITS> 9999999 THEN "
"  CONCAT(RTRIM(CAST(CAST((NUM_CACHE_HITS_PARSING + NUM_CACHE_HITS_BINDING + "
"   TEXT_CACHE_HITS)/10000000 AS NUMERIC(7,3)) AS CHAR(7) CHARACTER SET ISO88591)), _ISO88591'E+7') ELSE "  
"  CAST(NUM_CACHE_HITS_PARSING + NUM_CACHE_HITS_BINDING + TEXT_CACHE_HITS AS CHAR(7) CHARACTER SET ISO88591) "
" END) AS HITS, "
" (CASE WHEN NUM_LOOKUPS <=0 THEN _ISO88591'NA' ELSE "  
"  CONCAT(RTRIM(CAST(CAST(100*(NUM_CACHE_HITS_PARSING + NUM_CACHE_HITS_BINDING + " 
"   TEXT_CACHE_HITS)/NUM_LOOKUPS AS NUMERIC(6,2)) AS CHAR(6) CHARACTER SET ISO88591)), _ISO88591'%') " 
" END) AS HITRATE "
" FROM TABLE(QUERYCACHE()) ";

#define SELECT_LIST1 \
" select  " \
" (CASE WHEN ROW_ID > 99999999 THEN _ISO88591'OVERFLOW' ELSE " \
"   CAST(ROW_ID AS CHAR(8) CHARACTER SET ISO88591) END) AS ROWID, " \
" SUBSTRING(TEXT, 0, 36) AS TEXT, " \
" (CASE WHEN NUM_HITS > 99999999 THEN _ISO88591'OVERFLOW' ELSE " \
"   CAST(NUM_HITS AS CHAR(8) CHARACTER SET ISO88591) END) AS NUMHITS, " \
" (CASE PHASE WHEN _ISO88591'PARSING' THEN _ISO88591'P' WHEN _ISO88591'BINDING' THEN _ISO88591'B' ELSE _ISO88591'T' END) AS PH "

#define FROMQUERYCACHEENTRIES " from table(querycacheentries()) "

static const char * query_cache_ent_query = SELECT_LIST1
",(CASE WHEN COMPILATION_TIME > 99999999 THEN _ISO88591'OVERFLOW' ELSE "
"   CAST(COMPILATION_TIME AS CHAR(8) CHARACTER SET ISO88591) END) AS COMPTIME, "

" (CASE WHEN AVERAGE_HIT_TIME > 99999999 THEN _ISO88591'OVERFLOW' ELSE "
"   CAST(AVERAGE_HIT_TIME AS CHAR(8) CHARACTER SET ISO88591) END) AS AVGHTIME "
FROMQUERYCACHEENTRIES;

static const char * query_cache_ent_query_notime = SELECT_LIST1 
FROMQUERYCACHEENTRIES;

short QueryCacheSt::process(SqlciEnv * sqlci_env)
{
 Lng32 retcode;
 DML *query;

 switch (option_) {
   case 0:
   default:  
   {
     query = new DML(query_cache_query, DML_CONTROL_TYPE);
   }
   break;

   case 1:
   {
     query = new DML(query_cache_ent_query, DML_CONTROL_TYPE);
   }
   break; 

   case 2:
   {
     query = new DML(query_cache_ent_query_notime, DML_CONTROL_TYPE);
   }
   break; 
 }
  
 retcode =  query->process(sqlci_env);

 if (query) 
   delete query;

 return (short) retcode;
}

//////////////////////////////////////////
// begin QUIESCE
//////////////////////////////////////////

// Methods for class Quiesce
Quiesce::Quiesce()
  : SqlCmd(SqlCmd::QUIESCE_TYPE, NULL)
{
}

Quiesce::~Quiesce()
{
}

short Quiesce::process(SqlciEnv *sqlci_env)
{
  if (!getenv("SQLCI_CURSOR"))
  {
    sqlci_env->diagsArea() << DgSqlCode(-SQLCI_CURSOR_NOT_SUPPORTED);
    return 0;
  }

  Lng32 retcode = SQL_EXEC_Xact(SQLTRANS_QUIESCE, NULL);
  cout << endl
       << " SQL_EXEC_Xact(SQLTRANS_QUIESCE, NULL) returned " << retcode
       << endl << endl;
  return 0;
}

