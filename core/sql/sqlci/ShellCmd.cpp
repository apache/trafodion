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
 * File:         ShellCmd.C
 * RCS:          $Id: ShellCmd.cpp,v 1.6 1997/06/09 16:52:35  Exp $
 * Description:
 *
 *
 * Created:      4/15/95
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *****************************************************************************
 */


#include "Platform.h"


#include <iostream>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "NAString.h"
#include "ShellCmd.h"
#include "sqlcmd.h"
#include "SqlciError.h"

ShellCmd::ShellCmd(const shell_cmd_type cmd_type_, char * argument_)
                  : SqlciNode(SqlciNode::SHELL_CMD_TYPE),
		    cmd_type(cmd_type_)
{
  argument = new char[strlen(argument_)+1];
  strcpy(argument, argument_);
}

ShellCmd::~ShellCmd()
{
  delete [] argument;
}


Chdir::Chdir(char * argument_)
                 : ShellCmd(ShellCmd::CD_TYPE, argument_)
{
}

Ls::Ls(char * argument_)
                 : ShellCmd(ShellCmd::LS_TYPE, argument_)
{
}


Shell::Shell(char * argument_)
                 : ShellCmd(ShellCmd::SHELL_TYPE, argument_)
{
}

// Trim leading blanks, and trailing blanks and semicolons.
static char * trim(char * str)
{
  while (*str && isspace((unsigned char)*str)) str++;	// advance head ptr to first token  // For VS2003
  size_t len = strlen(str);
  if (len)
    {					// trim trailing blanks and semicolons
      char * end = &str[len-1];
      while (*end && (isspace((unsigned char)*end) || *end == ';')) end--;  //For VS2003
      *++end = '\0';
    }
  return str;
}

short Chdir::process(SqlciEnv * sqlci_env)
{
  char * cmd = trim(get_argument());

  cmd += 2;				// get past length("cd")

  if (!*cmd)				// empty (i.e. "cd;" was input)
    {
      if (getenv("HOME")) cmd = getenv("HOME");
    }
  else
    cmd = trim(cmd);			// get past blanks preceding token 2


  NAString dir = LookupDefineName(cmd);

  if (chdir(dir))
    {
      //use constructor and destructor when C++ compiler bug is fixed 
      ErrorParam *ep1 = new ErrorParam(errno);
      ErrorParam *ep2 = new ErrorParam(dir);
     
      SqlciError (SQLCI_NO_DIRECTORY,
		  ep1, 
		  ep2,
		  (ErrorParam *) 0
		 );
      delete ep1;
      delete ep2;

      return 0 /*errno*/;
    }

  return 0;
}

short Ls::process(SqlciEnv * sqlci_env)
{
  char * cmd = trim(get_argument());

  if (sqlci_env->isOleServer())
  {
	SqlciError (SQLCI_CMD_NOT_SUPPORTED,
				(ErrorParam *) 0 );
	return 0;
  }

  memcpy(cmd, "ls", 2);			// Make sure that 'ls' is passed in
                                	// lower case to the shell.
  system(cmd);

  return 0;
}

short Shell::process(SqlciEnv * sqlci_env)
{
  char * cmd = trim(get_argument());

  // replace any user defined pattern
  //  cmd = SqlCmd::replacePattern(sqlci_env, cmd);

  cmd += 2;				// get past length("sh")
  cmd = trim(cmd);			// get past blanks preceding token 2

  if (sqlci_env->isOleServer())
  {
	SqlciError (SQLCI_CMD_NOT_SUPPORTED,
				(ErrorParam *) 0 );
	return 0;
  }

  // Close the log file before running the command. This is to prevent
  // any logging activities (to the sqlci log file) in the command to 
  // interfere with the data already written and flushed to the log file.
  Logfile* logfile = sqlci_env->get_logfile();
  if ( logfile )
     logfile->Close_(); 

  if (!*cmd)				// empty (i.e. "sh;" was input)
    system("sh");
  else
    system(cmd);

  // reopen the log file in append mode
  if ( logfile )
     logfile->Reopen(); 

  if (sqlci_env->isInteractiveNow())
    cout << endl;

  return 0;
}

