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
 * File:         Log.C
 * Description:  
 *
 * Created:      1995
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include <ctype.h>
#include <iostream>
#include <errno.h>  
#include "Platform.h"
#include "RWInterface.h"

#include "SqlciCmd.h"
#include "SqlciError.h"
#include "str.h"

Logfile::Logfile()
{
  name = 0;
  logfile_stream = 0;
  flags_ = 0;
  setVerbose(TRUE);
}

Logfile::~Logfile()               
{
  if (IsOpen())
    Close();
  
  logfile_stream = 0;

  if (name)
    delete [] name;
}

void Logfile::Open(char * name_, open_mode mode)
{
  Close();

  name = new char[strlen(name_)+1];
  strcpy(name, name_);

  #ifndef NA_CASE_INSENSITIVE_FILENAMES
    static Int32 desensitize = -1;
    if (desensitize < 0) {
      const char *env = getenv("SQL_MXCI_CASE_INSENSITIVE_LOG");
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
      if (desensitize == 'U')
#pragma nowarn(1506)   // warning elimination 
	{for (char *n=name; *n; n++) *n = toupper(*n);}
#pragma warn(1506)  // warning elimination 
      else
#pragma nowarn(1506)   // warning elimination 
	{for (char *n=name; *n; n++) *n = tolower(*n);}
#pragma warn(1506)  // warning elimination 
  #endif
  
  if (mode == CLEAR_)
    logfile_stream = fopen(name, "w");
  else
  if (mode == APPEND_)
    logfile_stream = fopen(name, "a");
  if(!logfile_stream)
  {
	delete [] name;
	name = 0;
  }
}

void Logfile::Reopen()
{
  if (name)
    logfile_stream = fopen(name, "a");
}

// The Close_() function closes the file, but doesn't delete "name" so
// that Reopen() can be called again.  This provides a mechanism where
// the log file can be closed during system() calls and reopened after
// the system() call returns.  This prevents problems where the commands
// in the system() call may append files to the same log file.
void Logfile::Close_()
{
  if (logfile_stream)
  {
    fclose(logfile_stream);
    logfile_stream = 0;
  }
}

void Logfile::Close()
{
  Close_();

  if (name)
  {
    delete [] name;
    name = 0;
  }
}

short Logfile::Write(const char * buffer, Lng32 buflen)
{
  Int32 retcode;
  
  if (noLog())
    return 0;

  if (buflen > 0)
    {
	// Strings such as error msgs come here as "ERROR[nn] Msg text.\r\n".
	// The WINNT fwrite takes that final \n and replaces it with \r\n,
	// so we end up with "ERROR[nn] Msg text.\r\r\n"
	// which appears in the log as "ERROR[nn] Msg text.^M"
	// and screws up MKS diff utility (on regressions e.g.).
	//
	// And yes, it is safe to cast away const-ness to do this;
	// we are only overwriting *existing* chars in the buffer
	// *and* if this if-test is true the buffer contents must be
	// coming from a run-time rather than a static/pure/read-only/const data
	//
	if (buflen > 1 && buffer[buflen-1] == '\n' && buffer[buflen-2] == '\r')
	  {
	    char *bufxxx = (char *)buffer;
	    bufxxx[buflen-2] = '\n';
	    bufxxx[buflen-1] = '\0';
	    buflen--;
	  }

#pragma nowarn(1506)   // warning elimination 
      retcode = fwrite(buffer, buflen, 1, logfile_stream);
#pragma warn(1506)  // warning elimination 
      fflush (logfile_stream);
      if (!retcode)
	return -1;
    }
  
  retcode = fprintf(logfile_stream, "\n");
  fflush (logfile_stream);
  if (!retcode)
    return -1;

  return 0;
}

// WriteAll: WriteAll will write the buffer to the standard out.
//           If the log file is opened, WriteAll will then write
//           the buffer to the log file.

short Logfile::WriteAll(const char * buffer, Lng32 buflen)
{
  if (NOT noDisplay())
    {
      if (buflen > 0)
	cout << buffer;
      cout << endl;
    }

  if (IsOpen())
    return Write (buffer, buflen);
  return 0;
}

short Logfile::WriteAll(const char * buffer, Lng32 buflen, Int32 useCout)
{
  if (NOT noDisplay())
    {
      if (useCout)
	cout << buffer;
      else
	cout.write (buffer, buflen);
      cout << endl;
    }

  if (IsOpen())
  {
    if (useCout)
#pragma nowarn(1506)   // warning elimination 
      return Write (buffer, strlen(buffer));
#pragma warn(1506)  // warning elimination 
    else
      return Write (buffer, buflen);
  }
  return 0;
}

short Logfile::WriteAll(const WCHAR * mbBuf, Lng32 buflen)
{
  return 0;
}

short Logfile::WriteAll(const char * buffer)
{
#pragma nowarn(1506)   // warning elimination 
   return WriteAll (buffer, strlen(buffer));
#pragma warn(1506)  // warning elimination 
}

short Logfile::WriteAllWithoutEOL(const char * buffer)
{
#pragma nowarn(1506)   // warning elimination 
   Int32 buflen = strlen(buffer);
#pragma warn(1506)  // warning elimination 
   Int32 retcode ;
   
   if (NOT noDisplay())
     {
       if (buflen > 0)
	 cout << buffer;
     }

   if (IsOpen())
      {
	if (noLog())
	  return 0;

	 if (buflen > 0)
	 {
#pragma nowarn(1506)   // warning elimination 
	    retcode = fwrite(buffer, buflen, 1, logfile_stream);
#pragma warn(1506)  // warning elimination 
            fflush(logfile_stream);
#pragma nowarn(1506)   // warning elimination 
	    return retcode;
#pragma warn(1506)  // warning elimination 
	 }
	 else
	    return -1;
      }
   else return -1;
}

short Logfile::IsOpen()
{
  if (logfile_stream)
    return -1;
  else
    return 0;
}

void Logfile::setNoDisplay(NABoolean v)
{ (v ? flags_ |= NO_DISPLAY : flags_ &= ~NO_DISPLAY);};

short Log::process(SqlciEnv * sqlci_env)
{
  
  if (sqlci_env->isOleServer())
  {
    SqlciError (SQLCI_CMD_NOT_SUPPORTED,
		(ErrorParam *) 0 );
    return 0;
  }

  if (commandsOnly_)
    {
    sqlci_env->logCommands() = TRUE;
    }
  else
    {
    sqlci_env->logCommands() = FALSE;
    }
  switch (type)
  {
      case CLEAR_:
      {
	if (sqlci_env->get_logfile()->IsOpen())
	    sqlci_env->get_logfile()->Close();
	
	sqlci_env->get_logfile()->Open(get_argument(), Logfile::CLEAR_);
        // Check to see if report mode is on and if it is then call the report
        // writer method to let them know that we have started a LOG command.
        if (sqlci_env->isReportWriterMode())
        {
// 64-bit: no more report writer, return -1
//          long retcode = RW_MXCI_sendOutputDevice (sqlci_env->sqlciRWEnv()->rwEnv(), sqlci_env, LOG_FILE);
          Lng32 retcode = -1;
          if (retcode) // retcode == -1
          {
            SqlciError (SQLCI_RW_INVALID_OUTPUT_DEVICE,(ErrorParam *) 0 );
          }
        }

        if (!sqlci_env->get_logfile()->IsOpen())
	{
	    // This code was added to capture the errno generated when an invalid
	    // operation is performed up on a file.  The header file errno.h is included
	    // to get the errno.  errno is not a local variable. It is a variable found in
	    // errno.h  If somebody wants to add more errno they can do in this case statement.

	    SqlciError (SQLCI_INVALID_LOG_FILE_NAME,
		(ErrorParam *) 0 );
	}
      }
      break;

      case APPEND_:
      {
	if (sqlci_env->get_logfile()->IsOpen())
	    sqlci_env->get_logfile()->Close();
	
	sqlci_env->get_logfile()->Open(get_argument(), Logfile::APPEND_);
        if (sqlci_env->isReportWriterMode())
        {
// 64-bit: no more report writer, return -1
//          long retcode = RW_MXCI_sendOutputDevice (sqlci_env->sqlciRWEnv()->rwEnv(), sqlci_env, LOG_FILE);
          Lng32 retcode = -1;
          if (retcode) // retcode == -1
          {
            SqlciError (SQLCI_RW_INVALID_OUTPUT_DEVICE,(ErrorParam *) 0 );
          }
        }

        if (!sqlci_env->get_logfile()->IsOpen())
	{
	    // This code was added to capture the errno generated when an invalid
	    // operation is performed up on a file.  The header file errno.h is included
	    // to get the errno.  errno is not a local variable. It is a variable found in
	    // errno.h  If somebody wants to add more errno they can do in this case statement.

	    SqlciError (SQLCI_INVALID_LOG_FILE_NAME,
		(ErrorParam *) 0 );
	}
      }
      break;
       
      case STOP_:
      {
	if (sqlci_env->get_logfile()->IsOpen())
	  sqlci_env->get_logfile()->Close();

        if (sqlci_env->isReportWriterMode())
        {
// 64-bit: no more report writer, return -1
//          long retcode = RW_MXCI_sendOutputDevice (sqlci_env->sqlciRWEnv()->rwEnv(), sqlci_env, SCREEN);
          Lng32 retcode = -1;
          if (retcode) // retcode == -1
          {
            SqlciError (SQLCI_RW_INVALID_OUTPUT_DEVICE,(ErrorParam *) 0 );
          }
        }

      }
      break;
      
    default:
      break;
      
    }
      
  return 0;
}

