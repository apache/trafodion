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
 * File:         Define.cpp
 * Description:  Contains code for ENVVAR and DEFINE commands.
 *               
 * Created:      3/10/2000
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Define.h"
#include "GetErrorMessage.h"
#include "SqlciCmd.h"
#include "sqlcmd.h"
#include "SqlciEnv.h"
#include "str.h"
#include "SqlciError.h"

  extern char **environ;
  #define ENVIRON environ
  #define PUTENV putenv


static void invalidateEnvVarDependentCaches(const char *str)
{
  NAString env(str);
  #ifdef NA_CASE_INSENSITIVE_FILENAMES
    env.toUpper();
  #endif

  if (strncmp(env, "SQL_", 4) == 0 || strncmp(env, "SQLMX_", 6) == 0)
    {
      GetErrorMessageRebindMessageFile();
    }
}


/////////////////////////////////////////
// class Envvar
/////////////////////////////////////////
Envvar::Envvar(const char * name_, const char * value_)
{
  if (name_)
    {
      name = new char[strlen(name_) + 1];
      strcpy(name, name_);
    }
  else
    name = 0;

  if (value_)
    {
      value = new char[strlen(value_) + 1];
      strcpy(value, value_);
    }
  else
    value = 0;

  env_str = 0;
}

Envvar::~Envvar()
{
  if (name)
    delete [] name;

  if (value)
    delete [] value;

  if (env_str)
    delete [] env_str;
  
  name = 0;
  value = 0;
  env_str = 0;
}

void Envvar::setName(const char * name_)
{
  if (name)
    delete [] name;

  name = new char[strlen(name_) + 1];
  strcpy(name, name_);  
} 

void Envvar::setValue(const char * value_)
{
  if (value)
    delete [] value;

  value = new char[strlen(value_) + 1];
  strcpy(value, value_);  
} 

short Envvar::contains(const char * value) const
{
  if (strcmp(name, value) == 0)
    return -1;
  else
    return 0;
}
  
Int32 Envvar::set()
{
  // Envvars are added by creating a string "envvar_name=envvar_value"
  // and then adding it ("putting" it) to environment.

  delete [] env_str;
  
  env_str = new char[strlen(name) + 1/*for = */ + strlen(value)
		     +1/*for null*/];
  
  strcpy(env_str, name);
  strcat(env_str, "=");
  strcat(env_str, value);
  
  Int32 i = PUTENV(env_str);
  
  if (i) cerr << "*** ERROR " << i << " from putenv." << endl;
   
  invalidateEnvVarDependentCaches(name);
  return i;
}

Int32 Envvar::reset()
{
  if ((!getenv(name)) || (!env_str)) // this should be true,
    return -1;                       // otherwise its an internal error.
  
  // VO, Plan versioning support: The code below '#else' will not
  // reset the env var properly on NT, a subsequent 'sh env;' command
  // will still display it with its original value.
  char * ptr = strchr (env_str, '=');
  ptr[1] = 0;
  Int32 i = PUTENV(env_str);
  
  if (i) cerr << "*** ERROR " << i << " from putenv." << endl;

  invalidateEnvVarDependentCaches(name);
  return 0;
}


