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
 * File:         SqlciParser.C
 * Description:  
 *
 * Created:      4/15/95
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"


#include <stdlib.h>
#include <string.h>
#include "SqlciCmd.h"
#include "SqlciParser.h"
#include "ComDiags.h"
#include "str.h"
#include "SqlciParseGlobals.h"
#include "SqlciError.h"
#include "sqlcmd.h"

// Globals from SqlciEnv.C
extern ComDiagsArea sqlci_DA;

extern int yydebug;

static Int32 sqlci_parser_subproc(char *instr, char *origstr, SqlciNode ** node, SqlciEnv *sqlci_env)
{
  Int32 retval = 0;
  *node = NULL;
  
  
  // Set (reset) globals
  SqlciParse_InputStr = origstr;
  SqlciParse_OriginalStr = origstr;
  SqlciParse_InputPos = 0;
  SqlciEnvGlobal = sqlci_env; // setting the global SqlciEnv to the local sqlci_env

  
  if (origstr)
    {
      // remove trailing blanks
      Lng32 j = strlen(origstr) - 1;
      while ((j >= 0) && (origstr[j] == ' '))
	j--;
      origstr[j+1] = 0;
      
        if (j >= 0)
	{
	  SqlciLexReinit();
	  
	  retval = sqlciparse();
	  
	  // success in parsing 
	  if (!retval)				// success so far in parsing
	    {
	      assert(SqlciParseTree->isSqlciNode());
	      
              if (retval = SqlciParseTree->errorCode())	  // oops, an error
                {
                  delete SqlciParseTree;
                  SqlciParseTree = NULL;
                  retval = -ABS(retval);	// error, caller won't retry
                }
	    }
	  else
	    retval = +ABS(retval);	// error, caller will retry
	  
	  
	  if (!retval)
	    {
	      *node = SqlciParseTree;		// successful return
	    }
	} // if (j >=0)
    } // if (origstr)
  
  return retval;
}

Int32 sqlci_parser(char *instr, char *origstr, SqlciNode ** node, SqlciEnv *sqlci_env)
{
  Lng32 prevDiags = sqlci_DA.getNumber();	// capture this before parsing

  // replace any user defined pattern in the query
  char * newstr = origstr;
  newstr = SqlCmd::replacePattern(sqlci_env, origstr);  

  Int32 retval = sqlci_parser_subproc(instr, newstr, node, sqlci_env);
  
  // There's still some weird error in the Sqlci Lexer
  // ("OBEY F(S);FC 1;" and "OBEY F;!O;" fail -- bug in the <FNAME> state?).
  // Here we *KLUDGE* around the problem, by retrying the parse once only.
  
  if (retval)
    {
      sqlci_parser_syntax_error_cleanup(NULL, sqlci_env);
      if (!prevDiags && retval > 0)		// NOT the -99 from above!
	{
	  sqlci_DA.clear();
	  retval = sqlci_parser_subproc(instr, newstr, node, sqlci_env);
	  if (retval)
	    sqlci_parser_syntax_error_cleanup(NULL, sqlci_env);
	}
    }
  
  if (retval > 0)
    {
      SqlciParse_OriginalStr = origstr;
    }

  if (newstr != origstr)
    delete [] newstr;

  return retval;
  
}

Int32 sqlci_parser_syntax_error_cleanup(char *instr, SqlciEnv *sqlci_env)
{
  SqlciNode * sqlci_node;
  
  // Parser will emit syntax error message
  // (if Parser already has emitted same, then pass this in as NULL)
  if (instr)
    sqlci_parser_subproc(instr, instr, &sqlci_node, sqlci_env);

  // This *KLUDGE* will reset parser/lexer so that the next user-input stmt
  // does not automatically get tagged as a syntax error.
  SqlciParse_HelpCmd = -1;
  char junk[4];
  strcpy(junk, ".;");
  sqlci_parser_subproc(junk, junk, &sqlci_node, sqlci_env);
  SqlciParse_HelpCmd = 0;

  return 0;
}

		
