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



//////////////////////////////////////////////////////////////////
// class SetDefine
//////////////////////////////////////////////////////////////////
SetDefine::SetDefine(char * define_name_, Lng32 namelen_, 
		     char * argument_, Lng32 arglen_,
		     NABoolean alter)
                 : SqlciCmd(SqlciCmd::SETDEFINE_TYPE, argument_, arglen_),
		   alter_(alter)
{
  if (define_name_)
    {
      define_name = new char[namelen_+1];

      for (size_t k=0; k < strlen(define_name_); k++)
	{
#pragma nowarn(1506)   // warning elimination 
	  define_name[k] = toupper(define_name_[k]);
#pragma warn(1506)  // warning elimination 
	}
      
      define_name[strlen(define_name_)] = 0;
      namelen = namelen_;
    }
  else
    {
      define_name = 0;
      namelen = 0;
    }
}

SetDefine::~SetDefine()
{
  if (define_name)
    delete [] define_name;
}


short SetDefine::process(SqlciEnv * sqlci_env)
{
  Define * define_ = sqlci_env->get_definelist()->get(define_name);

  if (alter_)
    {
      if (NOT define_)
	{
	  SqlciError(SQLCI_DEFINE_DOESNT_EXIST, (ErrorParam*)NULL); 
	  return 0;
	}

      // reset this define
#pragma nowarn(1506)   // warning elimination 
      Reset r(Reset::DEFINE_, define_name, strlen(define_name));
#pragma warn(1506)  // warning elimination 
      r.process(sqlci_env);
    }
  else
    {
      // add define
      if (define_)
	{
	  SqlciError(SQLCI_DEFINE_EXISTS, (ErrorParam*)NULL);
	  //		     (new ErrorParam(define_name)));
	  return 0;
	}
    }
      
  define_ = new Define(define_name, get_argument());
  if (define_->set(sqlci_env))
    {
      // error case
      SqlciError (SQLCI_DEFINE_ERROR, (ErrorParam*)NULL);
    }

  sqlci_env->get_definelist()->append(define_);

  return 0;
}


/////////////////////////////////////////
// class Define
/////////////////////////////////////////
Define::Define(const char * name_, const char * value_)
{
  if (name_)
    {
      name = new char[strlen(name_) + 1];
      strcpy(name, name_);

      defineNameInternal = new char[24];
      str_pad(defineNameInternal, 24, ' ');
#pragma nowarn(1506)   // warning elimination 
      short len = MINOF(strlen(name), 24);
#pragma warn(1506)  // warning elimination 
      str_cpy_all(defineNameInternal, name, len);
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
}

Define::~Define()
{
  if (name)
    delete [] name;

  if (value)
    delete [] value;

  name = 0;
  value = 0;
}

void Define::setName(const char * name_)
{
  if (name)
    delete [] name;

  name = new char[strlen(name_) + 1];
  strcpy(name, name_);  
} 

void Define::setValue(const char * value_)
{
  if (value)
    delete [] value;

  value = new char[strlen(value_) + 1];
  strcpy(value, value_);  
} 

short Define::contains(const char * value) const
{
  if (strcmp(name, value) == 0)
    return -1;
  else
    return 0; 
}
  
Int32 Define::set(SqlciEnv * sqlci_env)
{
  
  return 0;
}

Int32 Define::reset(SqlciEnv * sqlci_env)
{

  return 0;
}

void Define::stripTrailingBlanks(unsigned char * value, short valueLen)
{
  Int32 i = valueLen - 1;
  while ((i > 0) && (value[i] == ' '))
    i--;
  
  value[i+1] = 0;

}

///////////////////////////////////////////////////////////////
// If getNext is -1, returns values for the next define
// following the one in defineName.
// Otherwise returns values for defineName.
//
// returns: 0, if define found. 1, if no more define (EOD).
//          -1, on error.
///////////////////////////////////////////////////////////////
short Define::getDefineInfo(unsigned char * defineName,
			    unsigned char * className,
			    unsigned char * attrName,
			    unsigned char * attrValue,
			    short getNext)
{
  return -1;

}


short Define::getDefaults(unsigned char * defaultSubvol)
{
  unsigned char defineName[24+1];
  unsigned char className[16+1];
  unsigned char attrName[16+1];
  
  str_pad((char *)defineName, 24, ' ');
  str_cpy_all((char *)defineName, "=_DEFAULTS", str_len("=_DEFAULTS"));

  short rc = getDefineInfo(defineName, className,
			   attrName, defaultSubvol,
			   FALSE /*get this define*/);
  
  
  return 0;
}
