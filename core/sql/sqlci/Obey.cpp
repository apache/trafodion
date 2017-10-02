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
 * File:         Obey.C
 * RCS:          $Id: Obey.cpp,v 1.7.16.1 1998/03/11 22:34:07  Exp $
 * Description:
 *
 * Created:      7/10/95 * Modified:     $ $Date: 1998/03/11 22:34:07 $ (GMT)
 * Language:     C++
 *
 *
 *****************************************************************************
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "SqlciCmd.h"
#include "SqlciError.h"
#include "SqlciParser.h"
#include "InputStmt.h"
#include "str.h"
#include "ComDiags.h"

extern ComDiagsArea sqlci_DA;

short Obey::process(SqlciEnv * sqlci_env)
{
  short retcode = 0;

  enum ObeyState 
   {
     SKIP_STMT, PROCESS_STMT, DONE
   };

  if (sqlci_env->isOleServer())
  {
	SqlciError (SQLCI_CMD_NOT_SUPPORTED, (ErrorParam *) 0 );
	return 0;
  }

  char *name = get_argument();

  errno = 0;
  FILE * file_stream = fopen(name, "r");
  if (!file_stream)
    {
      #ifndef NA_CASE_INSENSITIVE_FILENAMES
	static Int32 desensitize = -1;
	if (desensitize < 0) {
	  const char *env = getenv("SQL_MXCI_CASE_INSENSITIVE_OBEY");
	  if (!env || !*env || *env == '0')
	    desensitize = 0;
	  else if (*env == '1'   || isupper(*env))
	    desensitize = 'U';
	  else if (isdigit(*env) || islower(*env))
	    desensitize = 'L';
	  else
	    desensitize = 'U';
	}
	if (desensitize)
	  {
	    NABoolean U = (desensitize == 'U');
	    for (Int32 i = 0; i < 2 && !file_stream; i++, U = !U)
	      {
		if (U)
		  {for (char *n=name; *n; n++) *n = toupper(*n);}
		else
		  {for (char *n=name; *n; n++) *n = tolower(*n);}
		file_stream = fopen(name, "r");
	      }

	    if (!file_stream)
	      {
		// We've tried the original name aBc,
		// and ABC and abc, so as a last-ditch effort we try Abc.
		{for (char *n=name; *n; n++) *n = tolower(*n);}
		*name = toupper(*name);
		file_stream = fopen(name, "r");

		// If all failed, ensure name all upper for prettier error msg
		if (!file_stream)
		  {for (char *n=name; *n; n++) *n = toupper(*n);}
	      }
	  }
	if (!file_stream)
      #endif
	  {
	    ErrorParam *p1 = new ErrorParam (errno);
	    ErrorParam *p2 = new ErrorParam (name);
	    SqlciError (SQLCI_OBEY_FOPEN_ERROR,
			p1,
			p2,
			(ErrorParam *) 0
			);
	    delete p1;
	    delete p2;	    
	    return 0;
	  }
    }

  short prevEnvObey = sqlci_env->inObeyFile();
  sqlci_env->setObey(-1);

  Int32 done = 0;
  Int32 ignore_toggle = 0;
  Int32 veryFirst = 1;
  ObeyState state;

  if (!section_name)
    state = PROCESS_STMT;
  else
    state = SKIP_STMT;

  Int32 section_was_seen = (state == PROCESS_STMT);

  InputStmt * input_stmt;
  SqlciNode * sqlci_node = 0;

  while (!done)
    {
      input_stmt = new InputStmt(sqlci_env);
      Int32 read_error = 0;
	  if(veryFirst == 1)
	  {
	    veryFirst = 0;
		input_stmt->setVeryFirstLine();
      }
      if (state != DONE)
	{
	  // If section wasn't seen yet, then suppress echoing of blank lines
	  // in the preceding sections of the obey file as we read thru it.
	  read_error = input_stmt->readStmt(file_stream, !section_was_seen);

	  if (feof(file_stream) || read_error == -99)
	  {
	    if (!section_was_seen && read_error != -99)
	      {
		// Clear the DiagnosticsArea of any preceding sections'
		// syntax errors.
		sqlci_DA.clear();
		SqlciError (SQLCI_SECTION_NOT_FOUND,
			    new ErrorParam (section_name),
			    new ErrorParam (name),
			    (ErrorParam *) 0
			   );
	      }
	    else if (!input_stmt->isEmpty() && read_error != -4)
	      {
		// Unterminated statement in obey file.
		// Make the parser emit an error message.
		input_stmt->display((UInt16)0); //64bit project: add dummy arg - prevent C++ error
		input_stmt->logStmt();
		input_stmt->syntaxErrorOnEof();
	      }
	      state = DONE;
	      
		}
		
	  // if there is an eof directly after a statement
	  // that is terminated with a semi-colon, process the
	  // statement
	  if (read_error == -4)   state = PROCESS_STMT;
	}


    switch (state)
	{
	case SKIP_STMT:
	  {
	    if (input_stmt->sectionMatches(section_name))
	      {
		input_stmt->display((UInt16)0); //64bit project: add dummy arg - prevent C++ error
		state = PROCESS_STMT;
		section_was_seen = (state == PROCESS_STMT);
		// Clear the DiagnosticsArea for the section we're to process
		// (clear it of any preceding sections' syntax errors).
		sqlci_DA.clear();
	      }
	  }
	  break;

	case PROCESS_STMT:
	  {
	    short section_match = input_stmt->sectionMatches();

	    if (!section_name && section_match)
	      {
		input_stmt->display((UInt16)0); //64bit project: add dummy arg - prevent C++ error

		if (sqlci_env->logCommands())
		  sqlci_env->get_logfile()->setNoLog(FALSE);
		input_stmt->logStmt();
		if (sqlci_env->logCommands())
		  sqlci_env->get_logfile()->setNoLog(TRUE);
		break;
	      }

	    if ((!section_name) || (!section_match))
	      {
		input_stmt->display((UInt16)0); //64bit project: add dummy arg - prevent C++ error

		if (sqlci_env->logCommands())
		  sqlci_env->get_logfile()->setNoLog(FALSE);
		input_stmt->logStmt();
		if (sqlci_env->logCommands())
		  sqlci_env->get_logfile()->setNoLog(TRUE);

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
                        retcode = sqlci_parser(input_stmt->getPackedString(),
                                               input_stmt->getPackedString(),
                                               &sqlci_node,sqlci_env);
                        if (sqlci_node)
                          {
                            retcode = sqlci_node->process(sqlci_env);
                            if (retcode == SQL_Canceled)
                              {
			      	state = DONE;
			        retcode = 0;
			      }
			    delete sqlci_node;
                          }
                        if (retcode > 0)
                          {
                            sqlci_env->setPrevErrFlushInput();
                            retcode = 0;
                          }
                      }
                  }
                
                sqlci_env->displayDiagnostics() ;

		    // Clear the DiagnosticsArea for the next command...
		    sqlci_DA.clear();

		    // if an EXIT statement was seen, then a -1 will be returned
		    // from process. We are done in that case.
		    if (retcode == -1)
		    {
		      state = DONE;
		      fclose(file_stream);
		      done = -1;

		    }
		      
	      }
	    else
	      {
		state = DONE;
	      }
	  }
	  break;

	case DONE:
	  {
	    fclose(file_stream);
	    done = -1;
  	  }
	  break;

	default:
	  {
	  }
	  break;

	} // switch on state

      // Delete the stmt if it's not one of those we saved on the history list
      if (!input_stmt->isInHistoryList())
	delete input_stmt;

      if (breakReceived)
      {
        state = DONE;
        sqlci_env->resetPrevErrFlushInput();
	retcode = 0;
      }

    } // while not done

  sqlci_env->setObey(prevEnvObey);
  return retcode;
}
