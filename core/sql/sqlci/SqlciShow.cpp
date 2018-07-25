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
 * File:         SqlciShow.C
 * RCS:          $Id: SqlciShow.cpp,v 1.10 1998/09/07 21:50:07  Exp $
 * Description:  
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1998/09/07 21:50:07 $ (GMT)
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
#include <limits.h>

#include <unistd.h>


#include "SqlciEnv.h"
#include "SqlciCmd.h"
#include "sqlcmd.h"
#include "SQLCLIdev.h"

Show::~Show()
{
}


short Show::show_control(SqlciEnv * sqlci_env)
{
  return 0;
}

short Show::show_cursor(SqlciEnv * sqlci_env)
{
  CursorStmt * cursor = sqlci_env->getCursorList()->getFirst();
  
  while (cursor)
    {
      sqlci_env->get_logfile()->WriteAll(cursor->cursorName());

      NAString pretty(cursor->prepStmt()->getStr());
      PrettifySqlText(pretty);
      LineBreakSqlText(pretty, FALSE, 79, 2, 2);
      pretty += '\n';
      sqlci_env->get_logfile()->WriteAll(pretty);

      cursor = sqlci_env->getCursorList()->getNext(); 
    }
  
  return 0;
}

short Show::show_param(SqlciEnv * sqlci_env)
{
  char buff[250];
  Param * param = sqlci_env->get_paramlist()->getFirst();
  
  while (param)
    {
      if (param->isNull())
	{
	  snprintf(buff, 250, "PARAM ?%s NULL", param->getName());
	  sqlci_env->get_logfile()->WriteAll(buff);
	}
      else
	{
	  snprintf(buff, 250, "PARAM ?%s %s", param->getName(), param->getDisplayValue(sqlci_env->getTerminalCharset()));
	  sqlci_env->get_logfile()->WriteAll(buff);
	}
      
      param = sqlci_env->get_paramlist()->getNext(); 
    }
  
  return 0;
}

short Show::show_pattern(SqlciEnv * sqlci_env)
{
  char buff[250];
  Param * pattern = sqlci_env->get_patternlist()->getFirst();
  
  while (pattern)
    {
      snprintf(buff, 250, "PATTERN $$%s$$ %s", pattern->getName(), pattern->getValue());
      sqlci_env->get_logfile()->WriteAll(buff);
      
      pattern = sqlci_env->get_patternlist()->getNext(); 
    }
  
  return 0;
}

short Show::show_prepared(SqlciEnv * sqlci_env)
{
  PrepStmt * prep_stmt = sqlci_env->get_prep_stmts()->getFirst();
  
  while (prep_stmt)
    {
      //sprintf(buff, "%s\n%s", prep_stmt->getStmtName(), prep_stmt->getStr());

      sqlci_env->get_logfile()->WriteAll(prep_stmt->getStmtName());

      NAString pretty(prep_stmt->getStr());
      PrettifySqlText(pretty);
      LineBreakSqlText(pretty, FALSE, 79, 2, 2);
      pretty += '\n';
      sqlci_env->get_logfile()->WriteAll(pretty);

      prep_stmt = sqlci_env->get_prep_stmts()->getNext(); 
    }
  
  return 0;
}

short Show::show_session(SqlciEnv * sqlci_env)
{
  Env env(NULL, 0);			// the default ENV command
  return env.process(sqlci_env);
}

short Show::show_version(SqlciEnv * sqlci_env)
{
  sqlci_env->welcomeMessage();

  return 0;
}

short Show::process(SqlciEnv * sqlci_env)
{
  short retcode = 1;			// assume error
  
  switch (type)
    {
    case CONTROL_:
      retcode = show_control(sqlci_env);
      break;

    case CURSOR_:
      retcode = show_cursor(sqlci_env);
      break;

    case PARAM_:
      retcode = show_param(sqlci_env);
      break;
      
    case PATTERN_:
      retcode = show_pattern(sqlci_env);
      break;
      
    case PREPARED_:
      retcode = show_prepared(sqlci_env);
      break;
  
    case SESSION_:
      retcode = show_session(sqlci_env);
      break;
    
    case VERSION_:
      retcode = show_version(sqlci_env);
      break;
    
    default:
      break;
    }
  
  return retcode;
}


