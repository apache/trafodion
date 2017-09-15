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
 * File:         SqlciReset.C
 * RCS:          $Id: SqlciReset.cpp,v 1.2 1997/04/23 00:31:06  Exp $
 * Description:  
 *               
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1997/04/23 00:31:06 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
// Change history:
// 
// $Log: SqlciReset.cpp,v $
// Revision 1.2  1997/04/23 00:31:06
// Merge of MDAM/Costing changes into SDK thread
//
// Revision 1.1.1.1.2.1  1997/04/11 23:24:57
// Checking in partially resolved conflicts from merge with MDAM/Costing
// thread. Final fixes, if needed, will follow later.
//
// Revision 1.3.4.1  1997/04/10 18:33:19
// *** empty log message ***
//
// Revision 1.1.1.1  1997/03/28 01:39:45
// These are the source files from SourceSafe.
//
// 
// 10    1/22/97 11:04p 
// Merged UNIX and NT versions.
// 
// 8     1/14/97 4:55a 
// Merged UNIX and  NT versions.
// 
// 6     12/09/96 2:31a 
// Put the previous NT version on the top.
// Revision 1.3  1996/05/29 14:21:46
// fixed RESET DEFINE to be case insensitive
//
// Revision 1.2  1996/04/05 20:08:03
// Included the standard banner with RCS strings.
//
// Revision 1.1  1995/07/17 21:59:16
// Initial revision
//
// 
// -----------------------------------------------------------------------
#include <stdlib.h>
#include <ctype.h>

#include "sqlcmd.h"
#include "SqlciCmd.h"

Reset::~Reset()
{
}


short Reset::reset_control(SqlciEnv * sqlci_env)
{
  return 0;
}

short Reset::reset_param(SqlciEnv * sqlci_env)
{
  if (!get_argument())
    {
      /* RESET all params */
      Param * param = sqlci_env->get_paramlist()->getFirst();
      while (param)
	{
	  sqlci_env->get_paramlist()->remove(param->getName());
	  param = sqlci_env->get_paramlist()->getNext();
	}
    }
  else
    {
      Param * param = sqlci_env->get_paramlist()->get(get_argument());
      if (param)
	{
	  sqlci_env->get_paramlist()->remove(get_argument());
	}
    }
  
  return 0;
}

short Reset::reset_pattern(SqlciEnv * sqlci_env)
{
  if (!get_argument())
    {
      /* RESET all patterns */
      Param * pattern = sqlci_env->get_patternlist()->getFirst();
      while (pattern)
	{
	  sqlci_env->get_patternlist()->remove(pattern->getName());
	  pattern = sqlci_env->get_patternlist()->getNext();
	}
    }
  else
    {
      Param * pattern = sqlci_env->get_patternlist()->get(get_argument());
      if (pattern)
	{
	  sqlci_env->get_patternlist()->remove(get_argument());
	}
    }
  
  return 0;
}

short Reset::reset_prepared(SqlciEnv * sqlci_env)
{
  return 0;
}

short Reset::process(SqlciEnv * sqlci_env)
{
  short retcode = 0;
  
  switch (type)
    {
    case CONTROL_:
      retcode = reset_control(sqlci_env);
      break;

    case PARAM_:
      retcode = reset_param(sqlci_env);
      break;

    case PATTERN_:
      retcode = reset_pattern(sqlci_env);
      break;

    case PREPARED_:
      retcode = reset_prepared(sqlci_env);
      break;
      
    default:
      break;
    }
  
  return retcode;
}

