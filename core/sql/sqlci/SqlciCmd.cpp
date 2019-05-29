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
 * File:         SqlciCmd.C
 * Description:
 *
 * Created:      4/15/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include <errno.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <wchar.h>

#include "Platform.h"
#include "NAWinNT.h"

#include "ComCextdecs.h"
#include "ComDiags.h"
#include "ComSmallDefs.h"
#include "ErrorMessage.h"
#include "GetErrorMessage.h"
#include "InputStmt.h"
#include "SqlciError.h"
#include "SqlciCmd.h"
#include "sqlcmd.h"
#include "ShellCmd.h"
#include "SqlciError.h"
#include "SqlciParser.h"
#include "str.h"
#include "charinfo.h"
#include "SqlciEnv.h"
#include "Sqlci.h"
#include "sql_id.h"
#include "ComRtUtils.h"
#include "ComUser.h"

extern ComDiagsArea sqlci_DA;

SqlciCmd::SqlciCmd(const sqlci_cmd_type cmd_type_)
                  : SqlciNode(SqlciNode::SQLCI_CMD_TYPE),
		    cmd_type(cmd_type_)
{
  arglen = 0;
  argument = 0;
}

SqlciCmd::SqlciCmd(const sqlci_cmd_type cmd_type_, char * argument_, Lng32 arglen_)
                  : SqlciNode(SqlciNode::SQLCI_CMD_TYPE),
		    cmd_type(cmd_type_)
{
  arglen = arglen_;

  if (argument_)
    {
      argument = new char[arglen_ + 1]; 
      strncpy(argument, argument_,arglen_);
      argument[arglen] = 0;
    }
  else
    {
      argument = 0;
    }
};

SqlciCmd::SqlciCmd(const sqlci_cmd_type cmd_type_, NAWchar * argument_, Lng32 arglen_)
                  : SqlciNode(SqlciNode::SQLCI_CMD_TYPE),
		    cmd_type(cmd_type_)
{
  arglen = 2*arglen_;

  if (argument_)
    {
      NAWchar* tgt = new NAWchar[arglen_ + 1]; // extra byte for NAWchar typed argument
      NAWstrncpy(tgt, argument_,arglen_);
      tgt[arglen_] = 0;
      argument = (char*) tgt;
    }
  else
    {
      argument = 0;
    }
};

SqlciCmd::SqlciCmd(const sqlci_cmd_type cmd_type_, Int32 argument_)
                  : SqlciNode(SqlciNode::SQLCI_CMD_TYPE),
		    cmd_type(cmd_type_)
{
  numeric_arg = argument_;
  argument = 0;
};

SqlciCmd::~SqlciCmd()
{
  delete [] argument;
};

FixCommand::FixCommand(char * argument_, Lng32 arglen_)
                 : SqlciCmd(SqlciCmd::FC_TYPE, argument_, arglen_)
{
  cmd = argument_;
};

FixCommand::FixCommand(Int32 argument_, short neg_num_)
                 : SqlciCmd(SqlciCmd::FC_TYPE, argument_)
{
  cmd_num = argument_;
  neg_num = neg_num_;
  cmd = 0;
};

Obey::Obey(char * argument_, Lng32 arglen_, char * section_name_)
                 : SqlciCmd(SqlciCmd::OBEY_TYPE, argument_, arglen_)
{
  if (section_name_)
    {
      section_name = new char[strlen(section_name_)+1];
      strcpy (section_name, section_name_);
    }
  else
    {
      section_name = 0;
    }
};

Log::Log(char * argument_, Lng32 arglen_, log_type type_, Int32 commands_only)
                 : SqlciCmd(SqlciCmd::LOG_TYPE, argument_, arglen_),
		   type(type_), commandsOnly_(commands_only)
{
};

Shape::Shape(NABoolean type, char * infile, char * outfile)
  : SqlciCmd(SqlciCmd::SHAPE_TYPE),
    type_(type), infile_(infile), outfile_(outfile)
{
};

Statistics::Statistics(char * argument_, Lng32 arglen_,StatsCmdType type, 
		       char * statsOptions)
  : SqlciCmd(SqlciCmd::STATISTICS_TYPE, argument_, arglen_),
    type_(type)
{
    if (statsOptions)
    {
      statsOptions_ = new char[strlen(statsOptions)+1];
      strcpy (statsOptions_, statsOptions);
    }
  else
    {
      statsOptions_ = NULL;
    }
};

Statistics::~Statistics()
{
  if (statsOptions_)
    delete [] statsOptions_;
};

QueryId::QueryId(char * argument_, Lng32 arglen_, 
                 NABoolean isSet, char * qidVal)
  : SqlciCmd(SqlciCmd::QUERYID_TYPE, argument_, arglen_),
    isSet_(isSet), qidVal_(NULL)
{
  if ((isSet_) && (qidVal))
    {
      qidVal_ = new char[strlen(qidVal) + 1];
      strcpy(qidVal_, qidVal);
    }
};

QueryId::~QueryId()
{
  if (qidVal_)
    delete qidVal_;
}

History::History(char * argument_, Lng32 arglen_)
                 : SqlciCmd(SqlciCmd::HISTORY_TYPE, argument_, arglen_)
{
};

ListCount::ListCount(char * argument_, Lng32 arglen_)
                 : SqlciCmd(SqlciCmd::LISTCOUNT_TYPE, argument_, arglen_)
{
};

Mode::Mode(ModeType type_, NABoolean value)
                 : SqlciCmd(SqlciCmd::MODE_TYPE),
				   value(value)
{
	type = type_;
};


Verbose::Verbose(char * argument_, Lng32 arglen_, VerboseCmdType type)
                 : SqlciCmd(SqlciCmd::VERBOSE_TYPE, argument_, arglen_),
                   type_(type)
{
};

ParserFlags::ParserFlags(ParserFlagsOperation opType_, Int32 param_)
                 : SqlciCmd(SqlciCmd::PARSERFLAGS_TYPE, param_)
{
  opType = opType_;
  param  = param_;
};

Error::Error(char * argument_, Lng32 arglen_, error_type type_)
             : SqlciCmd(SqlciCmd::ERROR_TYPE, argument_, arglen_)
{
  type = type_;
};

FCRepeat::FCRepeat(char * argument_, Lng32 arglen_)
                 : SqlciCmd(SqlciCmd::REPEAT_TYPE, argument_, arglen_)
{
  cmd = argument_;
};

FCRepeat::FCRepeat(Int32 argument_, short neg_num_)
                 : SqlciCmd(SqlciCmd::REPEAT_TYPE, argument_)
{
  cmd_num = argument_;
  neg_num = neg_num_;
  cmd = 0;
};

Exit::Exit(char * argument_, Lng32 arglen_)
                 : SqlciCmd(SqlciCmd::EXIT_TYPE, argument_, arglen_)
{
};

Reset::Reset(reset_type type_, char * argument_, Lng32 arglen_)
: SqlciCmd(SqlciCmd::SETPARAM_TYPE, argument_, arglen_),
  type(type_)
{
};

Reset::Reset(reset_type type_)
: SqlciCmd(SqlciCmd::SETPARAM_TYPE),
  type(type_)
{
};

SetParam::SetParam(char * param_name_, Lng32 namelen_, char * argument_, Lng32 arglen_, CharInfo::CharSet x)
                 : SqlciCmd(SqlciCmd::SETPARAM_TYPE, argument_, arglen_), 
                   cs(x), inSingleByteForm_(TRUE),
                   m_convUTF16ParamStrLit(NULL),
                   isQuotedStrWithoutCharSetPrefix_(FALSE),
                   m_termCS(CharInfo::UnknownCharSet)
{
  if (param_name_)
    {
      param_name = new char[namelen_+1];
      strcpy (param_name, param_name_);
      namelen = namelen_;
    }
  else
    {
      param_name = 0;
      namelen = 0;
    }
};

SetParam::SetParam(char * param_name_, Lng32 namelen_, NAWchar * argument_, Lng32 arglen_, CharInfo::CharSet x)
                 : SqlciCmd(SqlciCmd::SETPARAM_TYPE, argument_, arglen_), 
                   cs(x), inSingleByteForm_(FALSE),
                   m_convUTF16ParamStrLit(NULL),
                   isQuotedStrWithoutCharSetPrefix_(FALSE),
                   m_termCS(CharInfo::UnknownCharSet)
{
  if (param_name_)
    {
      param_name = new char[namelen_+1];
      strcpy (param_name, param_name_);
      namelen = namelen_;
    }
  else
    {
      param_name = 0;
      namelen = 0;
    }
};

SetPattern::SetPattern(char * pattern_name_, Lng32 namelen_, char * argument_, Lng32 arglen_)
                 : SqlciCmd(SqlciCmd::SETPATTERN_TYPE, argument_, arglen_)
{
  if (pattern_name_)
    {
      pattern_name = new char[namelen_+1];
      strcpy (pattern_name, pattern_name_);
      namelen = namelen_;
    }
  else
    {
      pattern_name = 0;
      namelen = 0;
    }
};

short SetTerminalCharset::process(SqlciEnv * sqlci_env)
{
  HandleCLIErrorInit();

  char* tcs = get_argument();
  Int32 tcs_len;

  if ( tcs != NULL && ((tcs_len=strlen(tcs)) <= 128) )
  {
     char tcs_uppercase[129];
     str_cpy_convert(tcs_uppercase, tcs, tcs_len, 1);
     tcs_uppercase[tcs_len] = 0;

     if ( CharInfo::isCharSetSupported(tcs_uppercase) == FALSE )
     {
      SqlciError (SQLCI_INVALID_TERMINAL_CHARSET_NAME_ERROR,
		  (ErrorParam *) 0
		 );
      return 0;
     }

     if ( CharInfo::isTerminalCharSetSupported(tcs_uppercase) == FALSE ) 
     {
      ErrorParam *ep = new ErrorParam(tcs);
      SqlciError (2038,
		  ep,
		  (ErrorParam *) 0
		 );
      delete ep;
      return 0;
     }

// The following code had been commented out but has been restored
//  for the charset project		CQD removed on 12/11/2007
/*
     char cqd_stmt[200]; // charset name can be up to 128 bytes long

     sprintf(cqd_stmt, "CONTROL QUERY DEFAULT TERMINAL_CHARSET '%s';",
                       tcs_uppercase
            );

     long retcode = SqlCmd::executeQuery(cqd_stmt, sqlci_env);

     if ( retcode == 0 )*/
       sqlci_env -> setTerminalCharset(
                     CharInfo::getCharSetEnum(tcs_uppercase)
                                      );
//     else
//       HandleCLIError(retcode, sqlci_env);


  } else 
      SqlciError (SQLCI_INVALID_TERMINAL_CHARSET_NAME_ERROR,
		  (ErrorParam *) 0
		 );

  return 0;
}

short SetIsoMapping::process(SqlciEnv * sqlci_env)
{
  HandleCLIErrorInit();


  char* omcs = get_argument();
  Int32 omcs_len;

  if ( omcs != NULL && ((omcs_len=strlen(omcs)) <= 128) )
  {
     char omcs_uppercase[129];
     str_cpy_convert(omcs_uppercase, omcs, omcs_len, 1);
     omcs_uppercase[omcs_len] = 0;

     if ( strcmp(omcs_uppercase, "ISO88591") != 0
          )
     {
       // 15001 42000 99999 BEGINNER MAJOR DBADMIN
       // A syntax error occurred at or before: $0~string0
       ErrorParam *ep = new ErrorParam(omcs);
       SqlciError (15001,
                   ep,
                   (ErrorParam *) 0
                  );
       delete ep;
       return 0;
     }


  } else {
    // 15001 42000 99999 BEGINNER MAJOR DBADMIN
    // A syntax error occurred at or before: $0~string0
    ErrorParam *ep;
    if (omcs)
      ep = new ErrorParam(omcs);
    else
      ep = new ErrorParam("ISO_MAPPING");
    SqlciError (15001,
                ep,
                (ErrorParam *) 0
                );
    delete ep;
  }

  return 0;
}

short SetDefaultCharset::process(SqlciEnv * sqlci_env)
{
  HandleCLIErrorInit();

  char* dcs = get_argument();
  Int32 dcs_len;

  if ( dcs != NULL && ((dcs_len=strlen(dcs)) <= 128) )
  {
     char dcs_uppercase[129];
     str_cpy_convert(dcs_uppercase, dcs, dcs_len, 1);
     dcs_uppercase[dcs_len] = 0;

     if ( strcmp(dcs_uppercase, "ISO88591") != 0 &&
          strcmp(dcs_uppercase, "UTF8"    ) != 0 &&
          strcmp(dcs_uppercase, "SJIS"    ) != 0
          )
     {
       // 15001 42000 99999 BEGINNER MAJOR DBADMIN
       // A syntax error occurred at or before: $0~string0
       ErrorParam *ep = new ErrorParam(dcs);
       SqlciError (15001,
                   ep,
                   (ErrorParam *) 0
                  );
       delete ep;
       return 0;
     }

     char cqd_stmt[200]; // charset name can be up to 128 bytes long

     sprintf(cqd_stmt, "CONTROL QUERY DEFAULT DEFAULT_CHARSET '%s';",
                       dcs_uppercase
            );

     Lng32 retcode = SqlCmd::executeQuery(cqd_stmt, sqlci_env);

     if ( retcode == 0 )
       sqlci_env -> setDefaultCharset(CharInfo::getCharSetEnum(dcs_uppercase));
     else
       HandleCLIError(retcode, sqlci_env);

  } else {
    // 15001 42000 99999 BEGINNER MAJOR DBADMIN
    // A syntax error occurred at or before: $0~string0
    ErrorParam *ep;
    if (dcs)
      ep = new ErrorParam(dcs);
    else
      ep = new ErrorParam("DEFAULT_CHARSET");
    SqlciError (15001,
                ep,
                (ErrorParam *) 0
                );
    delete ep;
  }

  return 0;
}

short SetInferCharset::process(SqlciEnv * sqlci_env)
{
  HandleCLIErrorInit();

  char* ics = get_argument();
  Int32 ics_len;

  if ( ics != NULL && ((ics_len=strlen(ics)) <= 128) )
  {
     char ics_uppercase[129];
     str_cpy_convert(ics_uppercase, ics, ics_len, 1);
     ics_uppercase[ics_len] = 0;

     if ( strcmp(ics_uppercase, "FALSE") != 0 &&
          strcmp(ics_uppercase, "TRUE" ) != 0 &&
          strcmp(ics_uppercase, "0"    ) != 0 &&
          strcmp(ics_uppercase, "1"    ) != 0 )
     {
       // 15001 42000 99999 BEGINNER MAJOR DBADMIN
       // A syntax error occurred at or before: $0~string0
       ErrorParam *ep = new ErrorParam(ics);
       SqlciError (15001,
                   ep,
                   (ErrorParam *) 0
                  );
       delete ep;
       return 0;
     }

     char cqd_stmt[200]; // charset name can be up to 128 bytes long

     sprintf(cqd_stmt, "CONTROL QUERY DEFAULT INFER_CHARSET '%s';",
                       ics_uppercase
            );

     Lng32 retcode = SqlCmd::executeQuery(cqd_stmt, sqlci_env);

     if ( retcode == 0 )
     {
       if ( ics_uppercase[0] == '1' || ics_uppercase[0] == 'T'/*RUE*/)
         sqlci_env -> setInferCharset(TRUE);
       else
         sqlci_env -> setInferCharset(FALSE);
     }
     else
       HandleCLIError(retcode, sqlci_env);

  } else {
    // 15001 42000 99999 BEGINNER MAJOR DBADMIN
    // A syntax error occurred at or before: $0~string0
    ErrorParam *ep;
    if (ics)
      ep = new ErrorParam(ics);
    else
      ep = new ErrorParam("INFER_CHARSET");
    SqlciError (15001,
                ep,
                (ErrorParam *) 0
                );
    delete ep;
  }

  return 0;
}



Show::Show(show_type type_, NABoolean allValues)
                 : SqlciCmd(SqlciCmd::SHOW_TYPE),
		   allValues_(allValues)
{
  type = type_;
};

Wait::Wait(char * argument_, Lng32 arglen_)
                 : SqlciCmd(SqlciCmd::WAIT_TYPE, argument_, arglen_)
{
};

//////////////////////////////////////////////////
short Exit::process(SqlciEnv * sqlci_env)
{
  // Default is to exit sqlci. In the special case of an
  // active transaction, the user may choose not to exit.
  short retval = -1;

 if (sqlci_env->statusTransaction())
    {
      // ## The following English text needs to come from the message file
      // ## so it can be translated, for I18N:

      sqlci_env->get_logfile()->WriteAll("\nThere is an active transaction.  Do you want to commit the transaction?");
      sqlci_env->get_logfile()->WriteAll("                 Y to commit transaction");
      sqlci_env->get_logfile()->WriteAll("                 N to abort transaction");
      sqlci_env->get_logfile()->WriteAllWithoutEOL("                 Any other key to resume in MXCI: ");

      char response[2];
      response[0] = 'N';			// noninteractive default
      response[1] = '\0';			// terminate string for WriteAll
      if (sqlci_env->isInteractiveSession())
        {
	  cin.clear(); clearerr(stdin);
	  cin.get(response[0]);
	  if (cin.eof())
	    {
	      // EOF encountered. Treat this as any key other than [YyNn]
	      response[0] = '\0';

	      // Output a '\n' since EOF keeps the cursor on the same line
	      cout << endl;

	      cin.clear(); clearerr(stdin);
	    }
	}

      // If not EOF and not simple ENTER/RETURN, then
      // display the character; if furthermore we're interactive, then
      // throw away the rest of the input line.
      if (response[0] == '\n')
	  sqlci_env->get_logfile()->WriteAll("", 0);	// write ONE endl, not 2
      else if (response[0])
	{
	  sqlci_env->get_logfile()->WriteAll(response, 1);
	  if (sqlci_env->isInteractiveSession())
	    {
	      InputStmt ignore(sqlci_env);
	      ignore.consumeLine();
	    }
	}

      // What if the executeQuery's below fail? should we still emit the
      // WriteAll "Transaction XXXed" message and proceed to exit?
      switch (response[0])
	{
	case 'y':
	case 'Y':
	  SqlCmd::executeQuery("COMMIT WORK;", sqlci_env);
	  sqlci_env->get_logfile()->WriteAll("Transaction committed."); //##I18N
	  retval = -1;
	  break;

	case 'n':
	case 'N':
	  SqlCmd::executeQuery("ROLLBACK WORK;", sqlci_env);
	  sqlci_env->get_logfile()->WriteAll("Transaction aborted.");  // ##I18N
	  retval = -1;
	  break;

	default:
	  sqlci_env->get_logfile()->WriteAll("Transaction state maintained.");	// ##I18N
	  retval = 0;
	  break;

	}

    } // if transaction is active

 // tell CLI that this user session is finished.
 SqlCmd::executeQuery("SET SESSION DEFAULT SQL_SESSION 'DROP';", sqlci_env);
 
  if (retval == -1)
    {
      sqlci_env->get_logfile()->WriteAll("\nEnd of MXCI Session\n"); // ##I18N
    }

  return retval;
}


//////////////////////////////////////////////
// Begin ERROR
////////////////////////////////////////////////
short Error::process(SqlciEnv * sqlci_env)
{
  NAWchar   *error_msg;
  Int32        codeE, codeW;
  char       stateE[10], stateW[10];
  NABoolean  msgNotFound;

  ostringstream omsg;

  #define GETERRORMESS(in, out, typ)	GetErrorMessage(in, (NAWchar *&)out, typ)

  codeW = ABS(atoi(get_argument()));	// >= 0, i.e. warning
  codeE = -codeW;			// <= 0, i.e. error

  // These calls must be done before any of the GETERRORMESS(),
  // as they all use (overwrite) the same static buffer in GetErrorMessage.cpp.
  ComSQLSTATE(codeE, stateE);
  ComSQLSTATE(codeW, stateW);
  msgNotFound = GETERRORMESS(codeE, error_msg, ERROR_TEXT);

  if (type == ENVCMD_)
    {
      if (msgNotFound)
        {
	  error_msg[0] = NAWchar('\n');
	  error_msg[1] = NAWchar('\0');
	}
      else
        {
	  // Extract the {braces-enclosed version string}
	  // that msgfileVrsn.ksh (called by GenErrComp.bat)
	  // has put into this ENVCMD_ message.
	  NAWchar *v = NAWstrchr(error_msg, NAWchar(']'));
	  if (v) error_msg = v;
	  v = NAWstrchr(error_msg, NAWchar('{'));
	  if (v) error_msg = v;
	  v = NAWstrchr(error_msg, NAWchar('}'));
	  if (v++ && *v == NAWchar('.')) *v = NAWchar(' ');
	}
      NAWriteConsole(error_msg, omsg, FALSE);
    }
  else if (!msgNotFound || codeE == 0)		// SQL "success", special case
    {
      omsg << "\n*** SQLSTATE (Err): " << stateE
           << " SQLSTATE (Warn): " << stateW;
      omsg << endl;
      NAWriteConsole(error_msg,omsg, TRUE);
    }
  else
    {
      // Msg not found.  
      ComDiagsArea diags;
      diags << DgSqlCode(codeW);
      NADumpDiags(omsg, &diags, TRUE/*newline*/);
    }

#if 0			// CAUSE/EFFECT/RECOVERY text not implemented yet!
  if (!msgNotFound && type == DETAIL_)
    {
      GETERRORMESS(codeE, error_msg, CAUSE_TEXT);
      NAWriteConsole(error_msg,omsg, TRUE);

      GETERRORMESS(codeE, error_msg, EFFECT_TEXT);
      NAWriteConsole(error_msg,omsg, TRUE);

      GETERRORMESS(codeE, error_msg, RECOVERY_TEXT);
      NAWriteConsole(error_msg,omsg, TRUE);

    }
#endif // 0		// CAUSE/EFFECT/RECOVERY text not implemented yet!

  omsg << ends;					// to tack on a null-terminator
  sqlci_env->get_logfile()->WriteAllWithoutEOL(omsg.str().c_str());
  return 0;
}


//////////////////////////////////////////////////
// Begin SHAPE
///////////////////////////////////////////////////
short Shape::process(SqlciEnv * sqlci_env)
{
  sqlci_env->showShape() = type_;

  if (! infile_)
    return 0;

  // open the infile to read Sql statements from
  FILE * fStream = 0;
  fStream = fopen(infile_, "r");
  if (!fStream)
    {
      ErrorParam *p1 = new ErrorParam (errno);
      ErrorParam *p2 = new ErrorParam (infile_);
      SqlciError (SQLCI_OBEY_FOPEN_ERROR,
	          p1,
		  p2,
		  (ErrorParam *) 0
		 );
      delete p1;
      delete p2;
      return 0;
    }
  
  // close and remember the current logfile
  char * logname = NULL;
  if (sqlci_env->get_logfile()->IsOpen())
    {
      logname = new char[strlen(sqlci_env->get_logfile()->Logname()) + 1];
      strcpy(logname, sqlci_env->get_logfile()->Logname());
      sqlci_env->get_logfile()->Close();
    }

  // if infile is the same as outfile, generate output into a temp
  // file(called outfile_ + __temp), and then rename it to infile. 
  // Also, rename infile to infile.bak.
  NABoolean tempFile = FALSE;
  char * tempOutfile = NULL;
  if (outfile_)
    {
      if (strcmp(infile_, outfile_) == 0)
	{
	  tempFile = TRUE;
	  tempOutfile = new char[strlen(outfile_) + 
				strlen("__temp") + 1];
	  strcpy(tempOutfile, outfile_);
	  strcat(tempOutfile, "__temp");
	}
      else
	tempOutfile = outfile_;

      sqlci_env->get_logfile()->Open(tempOutfile, Logfile::CLEAR_);
    }

  ////////////////////////////////////////////////////////////////
  // BEGIN PROCESS NEXT INPUT STMT
  ////////////////////////////////////////////////////////////////

  short retcode = processNextStmt(sqlci_env, fStream);
  if (retcode)
  {
    if(logname != NULL)
      delete [] logname;
    return retcode;
  }

  ////////////////////////////////////////////////////////////////
  // END PROCESS NEXT INPUT STMT
  ////////////////////////////////////////////////////////////////

  fclose(fStream);

  if (outfile_)
    {
      sqlci_env->get_logfile()->Close();

      char buf[200];
      if (tempFile)
	{
	  snprintf(buf, 200, "sh mv %s %s.bak", infile_, infile_);
	  ShellCmd * shCmd = new Shell(buf);
	  shCmd->process(sqlci_env);
	  delete shCmd;

	  snprintf(buf, 200, "sh mv %s %s", tempOutfile, infile_);
	  shCmd = new Shell(buf);
	  shCmd->process(sqlci_env);
	  delete shCmd;
	}
    }

  // reopen the original logfile
  if (logname)
    sqlci_env->get_logfile()->Open(logname, Logfile::APPEND_);

  delete [] logname;
  delete [] tempOutfile;
  tempOutfile = NULL;

  return 0;
}


short Shape::processNextStmt(SqlciEnv * sqlci_env, FILE * fStream)
{
  short retcode = 0;

  enum ShapeState 
   {
     PROCESS_STMT, DONE
   };

  Int32 done = 0;
  Int32 ignore_toggle = 0;
  ShapeState state;
  InputStmt * input_stmt;
  SqlciNode * sqlci_node = NULL;

  state = PROCESS_STMT;

  while (!done)
    {
      input_stmt = new InputStmt(sqlci_env);
      Int32 read_error = 0;
      if (state != DONE)
	{
	  read_error = input_stmt->readStmt(fStream, TRUE);
	  
	  if (feof(fStream) || read_error == -99)
	    {
	      if (!input_stmt->isEmpty() && read_error != -4)
		{
		  // Unterminated statement in obey file.
		  // Make the parser emit an error message.
		  input_stmt->display((UInt16)0, TRUE);
		  input_stmt->logStmt(TRUE);
		  input_stmt->syntaxErrorOnEof();
		}
	      state = DONE;
	    } // feof or error (=-99)
	}
      
      // if there is an eof directly after a statement
      // that is terminated with a semi-colon, process the
      // statement
      if (read_error == -4) state = PROCESS_STMT;
      
      switch (state)
	{
	case PROCESS_STMT:
	  {
	    Int32 ignore_stmt = input_stmt->isIgnoreStmt();
	    if (ignore_stmt)
	      ignore_toggle = ~ignore_toggle;
	    
	    if (ignore_stmt || ignore_toggle || input_stmt->ignoreJustThis())
	      {
		// ignore until stmt following the untoggling ?ignore
		sqlci_DA.clear();
	      }
	    else
	      {
		if (!read_error || read_error == -4)
		  {
		    sqlci_parser(input_stmt->getPackedString(),
				 input_stmt->getPackedString(),
				 &sqlci_node,sqlci_env);
		    if ((sqlci_node) &&
			(sqlci_node->getType() == SqlciNode::SQL_CMD_TYPE))
		      {
			delete sqlci_node;

			SqlCmd sqlCmd(SqlCmd::DML_TYPE, NULL);
			
			short retcode = sqlCmd.showShape(sqlci_env, 
							 input_stmt->getPackedString());
			if (retcode)
			{
			  delete input_stmt;
			  return retcode;
			}
		      }

		    input_stmt->display((UInt16)0, TRUE);
		    input_stmt->logStmt(TRUE);
		  }
		
		sqlci_env->displayDiagnostics() ;
		
		// Clear the DiagnosticsArea for the next command...
		sqlci_DA.clear();
		    
		// if an EXIT statement was seen, then a -1 will be returned
		// from process. We are done in that case.
		if (retcode == -1)
		  state = DONE;
	      }
	  }
	break;
	
	case DONE:
	  {
	    done = -1;
  	  }
	break;
	
	default:
	  {
	  }
	break;
	
	} // switch on state
      
      delete input_stmt;
      
    } // while not done

  return 0;
}

//////////////////////////////////////////////////
short Wait::process(SqlciEnv * sqlci_env)
{
  char buf[100];

  cout << "Enter a character + RETURN to continue: ";
  cin >> buf;

  return 0;
}

//////////////////////////////////////////////////
short ParserFlags::process(SqlciEnv * sqlci_env)
{
  Int32 retCode;

  if (!ComUser::isRootUserID())
  {    
    // Return - "not authorized" error 
    ComDiagsArea diags;
    diags << DgSqlCode(-1017);
    handleLocalError(&diags, sqlci_env);
    return -1;
  }

  if (opType == DO_SET)
  {
    if (param == 0)
    {
      // Warning 3190:
      // Please use "RESET PARSERFLAGS <value>" to reset the flags.
      ComDiagsArea diags;
      diags << DgSqlCode(3190);
      handleLocalError(&diags, sqlci_env);
    }
    retCode = SQL_EXEC_SetParserFlagsForExSqlComp_Internal2(param);
  }

  else
  {
    // It's DO_RESET
    retCode = SQL_EXEC_ResetParserFlagsForExSqlComp_Internal2(param);
  }

  if (retCode)
  {
    // This is most probably error 1017: 
    // You are not authorized to perform this operation.
    ComDiagsArea diags;
    diags << DgSqlCode(retCode);
    handleLocalError(&diags, sqlci_env);
  }
  return 0;
}


///////////////////////////////
//Process of the MODE Command//
///////////////////////////////

//   SQL is the normal mode in which MXCI executes.
short Mode::process(SqlciEnv * sqlci_env)
{
  short retcode = 1;
  switch (type)
  {
    case SQL_:
      retcode = process_sql(sqlci_env);
      break;
    default: 
      SqlciError(SQLCI_INVALID_MODE 
			,(ErrorParam *) 0 );
      break;
  }
  return retcode;
}

short Mode::process_sql(SqlciEnv * sqlci_env)
{
    if (SqlciEnv::SQL_ == sqlci_env->getMode())
    {
	SqlciError(SQLCI_RW_MODE_ALREADY_SQL 
			,(ErrorParam *) 0 );
       return 0;
    }
    
    sqlci_env->setMode(SqlciEnv::SQL_);

    return 0;
}

short QueryId::process(SqlciEnv * sqlci_env)
{
  Lng32 retcode = 0;

  HandleCLIErrorInit();
  
  char * stmtName = get_argument();

  PrepStmt * prep_stmt = NULL;
  if ((stmtName) &&
      (! (prep_stmt = sqlci_env->get_prep_stmts()->get(stmtName))))
    {
       sqlci_env->diagsArea() << DgSqlCode(-SQLCI_STMT_NOT_FOUND)
				 << DgString0(stmtName);
       return 0;
    }

  Logfile *log = sqlci_env->get_logfile();
  char sprintfBuf[225];    

  if (!stmtName)
  {
    //try to find the last executed statement if present
    if (sqlci_env->lastExecutedStmt() && sqlci_env->lastExecutedStmt()->getStmtNameLen() > 0)
      stmtName = sqlci_env->lastExecutedStmt()->getStmtName();
    //if not, try to find the last prepared statement if present
    else if (sqlci_env->getLastAllocatedStmt() && sqlci_env->getLastAllocatedStmt()->identifier_len > 0)
      stmtName = (char *) sqlci_env->getLastAllocatedStmt()->identifier;
  }

  if (!stmtName) 
  // no statement name found Display error.
  {
    sprintf(sprintfBuf, "No statement found. Enter command with valid statement name.");
    log->WriteAll(sprintfBuf);
    return 0;
  }

  SQLSTMT_ID stmt;
  SQLMODULE_ID module;
  init_SQLMODULE_ID(&module);
  init_SQLSTMT_ID(&stmt, SQLCLI_CURRENT_VERSION, 
		  stmt_name, &module);

  char * id = new char[strlen(stmtName) + 1];
  stmt.identifier_len = strlen(stmtName);
  str_cpy_all(id,stmtName, 
	 stmt.identifier_len);
  id[stmt.identifier_len] = 0;
  stmt.identifier = id;

  char queryId[200];
  Lng32 queryIdLen;
  
  if (isSet_)
    {
      // change query id in prep_stmt
      if (prep_stmt->uniqueQueryId())
        {
          delete prep_stmt->uniqueQueryId();
          prep_stmt->uniqueQueryIdLen() = 0;
        }
      
      prep_stmt->uniqueQueryIdLen() = strlen(qidVal_);
      prep_stmt->uniqueQueryId() = new char[prep_stmt->uniqueQueryIdLen() + 1];
      strcpy(prep_stmt->uniqueQueryId(), qidVal_);

      retcode = SQL_EXEC_SetStmtAttr(&stmt, SQL_ATTR_UNIQUE_STMT_ID,
                                     0, qidVal_);
      delete [] id;

      log->WriteAll("");

      return 0;
    }

  retcode = SQL_EXEC_GetStmtAttr(&stmt, SQL_ATTR_UNIQUE_STMT_ID,
                      NULL, queryId, 200, &queryIdLen); 
  delete [] id;

  if (queryIdLen < 200)
    queryId[queryIdLen] = 0;
  else
    queryId[199] = 0;

  HandleCLIError(retcode, sqlci_env);

  if (retcode == 0)
  {
    snprintf(sprintfBuf, 225, "QID is %s",queryId);
    log->WriteAll(sprintfBuf);
    log->WriteAll("");
  }  
    
  // display details of this query
  // for string attributes, use {type,maxLength,char[maxLength+1]}
  UNIQUEQUERYID_ATTR queryIdAttrs[11] =
  {
    {UNIQUEQUERYID_SEGMENTNUM, 0, 0},
    {UNIQUEQUERYID_SEGMENTNAME, 10, new char[11]},
    {UNIQUEQUERYID_CPU, 0, 0},
    {UNIQUEQUERYID_PIN, 0, 0},
    {UNIQUEQUERYID_EXESTARTTIME, 0, 0},
    {UNIQUEQUERYID_SESSIONNUM, 0, 0},
    {UNIQUEQUERYID_USERNAME, 24, new char[25]},
    {UNIQUEQUERYID_SESSIONNAME, 32, new char[33]},
    {UNIQUEQUERYID_QUERYNUM, 0, 0},
    {UNIQUEQUERYID_STMTNAME, 110, new char[111]},
    {UNIQUEQUERYID_SESSIONID, 104, new char[105]}
  };

  retcode = SQL_EXEC_GetUniqueQueryIdAttrs(queryId, queryIdLen,
                                           11, queryIdAttrs);
  HandleCLIError(retcode, sqlci_env);
  
  if (retcode == 0)
    {
      snprintf(sprintfBuf, 225, "QID details: ");
      log->WriteAll(sprintfBuf);
      log->WriteAll("============");

      // display UNIQUEQUERYID_SEGMENTNUM
      snprintf(sprintfBuf, 225, "  Segment Num:  " PFLL,queryIdAttrs[0].num_val_or_len);
      log->WriteAll(sprintfBuf);

      // display UNIQUEQUERYID_SEGMENTNAME
      snprintf(sprintfBuf, 225, "  Segment Name: %s",queryIdAttrs[1].string_val);
      log->WriteAll(sprintfBuf);

      // display UNIQUEQUERYID_CPU
      snprintf(sprintfBuf, 225, "  Cpu:          " PFLL,queryIdAttrs[2].num_val_or_len);
      log->WriteAll(sprintfBuf);

      // display UNIQUEQUERYID_PIN
      snprintf(sprintfBuf, 225, "  Pin:          " PFLL,queryIdAttrs[3].num_val_or_len);
      log->WriteAll(sprintfBuf);
 
      // UNIQUEQUERYID_EXESTARTTIME
      short startTimeArray[8];
      _int64 startTime = queryIdAttrs[4].num_val_or_len;
      short error;

      INTERPRETTIMESTAMP( CONVERTTIMESTAMP(startTime, 0, // GMT to LCT
                                                    -1, // use current node
                                                  &error), startTimeArray);
      sprintf(sprintfBuf, "  ExeStartTime: " PF64 "= %02d/%02d/%02d %02d:%02d:%02d.%03d%03d LCT",
                    startTime, startTimeArray[0],startTimeArray[1],startTimeArray[2],
                    startTimeArray[3], startTimeArray[4], startTimeArray[5],
                    startTimeArray[6], startTimeArray[7]);
      log->WriteAll(sprintfBuf);

      // display UNIQUEQUERYID_SESSIONNUM
      sprintf(sprintfBuf,"  SessionNum:   " PFLL,queryIdAttrs[5].num_val_or_len);
      log->WriteAll(sprintfBuf);

      // display UNIQUEQUERYID_USERNAME
      sprintf(sprintfBuf,"  UserName:     %s",queryIdAttrs[6].string_val);
      log->WriteAll(sprintfBuf);

      // display UNIQUEQUERYID_SESSIONNAME
      if (strlen(queryIdAttrs[7].string_val) > 0)
	sprintf(sprintfBuf,"  SessionName:  %s",queryIdAttrs[7].string_val);
      else
        sprintf(sprintfBuf,"  SessionName:  NULL");

      log->WriteAll(sprintfBuf);

      // display UNIQUEQUERYID_QUERYNUM
      sprintf(sprintfBuf,"  QueryNum:     " PFLL,queryIdAttrs[8].num_val_or_len);
      log->WriteAll(sprintfBuf);

      // display UNIQUEQUERYID_STMTNAME:
      sprintf(sprintfBuf,"  StmtName:     %s",queryIdAttrs[9].string_val);
      log->WriteAll(sprintfBuf);

      // display UNIQUEQUERYID_SESSIONID:
      sprintf(sprintfBuf,"  SessionId:    %s",queryIdAttrs[10].string_val);
      log->WriteAll(sprintfBuf);

      log->WriteAll("");
    }  
  
  return 0;
}








