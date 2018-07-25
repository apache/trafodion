// **********************************************************************
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
// **********************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "CommonLogger.h"
#include <errno.h>
#include <string.h>
#include <string>
#include <stdexcept>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

#define SLASH_S "/"
#define SLASH_C '/'

using namespace log4cxx;
using namespace log4cxx::helpers;

bool gv_commonLoggerInitialized = false;


// **************************************************************************
// Reads configuration file, creates an appender, layout, and categories.
// Attaches layout to the appender and appender to categories.
// Will be called by constructor qms, qmp, qmm, and mxcmp after calling
// setLogFileName().
// Return TRUE if all was well, or FALSE if the config file was not read
// successfully.
// **************************************************************************
bool CommonLogger::initLog4cxx(const char* configFileName, const char* fileSuffix)
{
  std::string configPath;

  logFolder_ = "";

  // Form the config path $TRAF_CONF
  char *installPath = getenv("TRAF_CONF");
  if (installPath)
  {
	configPath = installPath;
    // Last char may or may not be a slash.
    if (configPath[configPath.length()-1] != SLASH_C)
	  configPath += SLASH_C;  // LCOV_EXCL_LINE :nu
  }
  else
  {
    // Environment variable $TRAF_CONF not found.
    // use the current directory.
    configPath = "";
  }

  if (configFileName[0] == SLASH_C)
    configPath = configFileName;
  else
    configPath += configFileName;


  try
  {
    if ( fileSuffix && fileSuffix[0] != '\0' )
    {
      setenv("TRAFODION_LOG_FILENAME_SUFFIX", fileSuffix, 1);
    }
    else
    {
      setenv("TRAFODION_LOG_FILENAME_SUFFIX", "", 1);
    }

    log4cxx::PropertyConfigurator::configure(configPath.data());
    gv_commonLoggerInitialized = true;
    return true;

  }
  //catch (log4cxx::ConfigureFailure e)
  catch (Exception e)
  {
    return false;
  }
}

CommonLogger& CommonLogger::instance()
{
  static CommonLogger onlyInstance_;
  return onlyInstance_;
}

// **************************************************************************
// Is the category set to log DEBUG messages?
// This method should be used when the work of producing the logging text
// is expensive, such as a join graph or an extensive dump.
// **************************************************************************
bool CommonLogger::isCategoryInDebug(const std::string &cat)
{
    bool result = false;
    log4cxx::LoggerPtr myLogger(log4cxx::Logger::getLogger(cat));
    if ( myLogger )
    {
      log4cxx::LevelPtr myLevel = myLogger->getLevel();
      if ( myLevel )
      {
          result = ( myLevel != log4cxx::Level::getOff() && myLevel->isGreaterOrEqual(log4cxx::Level::getDebug()) );
      }
    }
    
    return result;   
}

// **************************************************************************
// The generic message logging method for any message type and length.
// **************************************************************************
void CommonLogger::log1(const std::string &cat,
                        logLevel    level,
                        const char* cmsg,
                        unsigned int eventId)
{
  // Have to use a std::string for the message. If a char* is passed, the
  // overload of the various logging functions (debug(), fatal(), etc.) that
  // accepts a char* is used, and they all use the char* as a format string.
  // This causes any contained % character to be treated as the beginning of
  // a format specifier, with ugly consequences.
  std::string msg(cmsg);

  eventId = eventId; // touch

  switch(level)
  {
    case LL_FATAL:
       LOG4CXX_FATAL(log4cxx::Logger::getLogger(cat),msg);
      break;

    case LL_ERROR:
      LOG4CXX_ERROR(log4cxx::Logger::getLogger(cat),msg);
      break;

    case LL_MVQR_FAIL:
      LOG4CXX_ERROR(log4cxx::Logger::getLogger(cat),msg);
      break;

    case LL_WARN:
      LOG4CXX_WARN(log4cxx::Logger::getLogger(cat),msg);
      break;

    case LL_INFO:
      LOG4CXX_INFO(log4cxx::Logger::getLogger(cat),msg);
      break;

    case LL_DEBUG:
      LOG4CXX_DEBUG(log4cxx::Logger::getLogger(cat),msg);
      break;

    case LL_TRACE:
      LOG4CXX_TRACE(log4cxx::Logger::getLogger(cat),msg);
      break;      
  }
}

char* CommonLogger::buildMsgBuffer(const std::string &cat,
                                   logLevel    level,
                                   const char* logMsgTemplate,
                                   va_list     args)
{
  // calculate needed bytes to allocate memory from system heap.
  int bufferSize = 20049;
  int msgSize = 0;
  static __thread char *buffer = NULL;
  va_list args2;
  va_copy(args2, args);
  bool secondTry = false;
  level = level; // touch
  if (buffer == NULL)
      buffer = new char[bufferSize];

  // For messages shorter than the initial 20K limit, a single pass through
  // the loop will suffice.
  // For longer messages, 2 passes will do it. The 2nd pass uses a buffer of the
  // size returned by the call to vsnprintf. If a second pass is necessary, we
  // pass the copy of the va_list we made above, because the first call changes
  // the state of the va_list, and can cause a crash if reused.
  while (1)
  {
     msgSize = vsnprintf(buffer, bufferSize, logMsgTemplate, secondTry ? args2 : args);

    // if the initial size of 20K works, then break out from while loop.
    if ( msgSize < 0 )
    {
      // LCOV_EXCL_START :rfi
      // Some error happened.
      // Report the first 40 chars.
      #define SHORTSIZE 40
      char smallBuff[SHORTSIZE+5];
      memset(smallBuff, 0, SHORTSIZE+5);
      if (strlen(logMsgTemplate) <= SHORTSIZE)
      	strcpy(smallBuff, logMsgTemplate);
      else
      {
      	memcpy(smallBuff, logMsgTemplate, SHORTSIZE);
      	strcpy(smallBuff+SHORTSIZE, "...");
      }

      char errorMsg[100];
      sprintf(errorMsg, "Cannot log full message, error %d returned from vsnprintf()", msgSize);
      LOG4CXX_ERROR(log4cxx::Logger::getLogger(cat),errorMsg);
      LOG4CXX_ERROR(log4cxx::Logger::getLogger(cat),smallBuff);

      return buffer;
      
    }
    else if (msgSize < bufferSize)
    {
      // Buffer is large enough - we're good.
     break;
    }
    else
    {
      // Buffer is too small, reallocate it and go through the loop again.
      bufferSize = msgSize + 1; // Use correct size now.
      delete [] buffer;
      buffer = new char[bufferSize];
      secondTry = true;
    }
  }

  va_end(args2);
  return buffer;
}
//
// **************************************************************************
// The generic message logging method for any message type and length.
// **************************************************************************
void CommonLogger::log(const std::string &cat,
                       logLevel    level,
                       const char* logMsgTemplate...)
{
  // Don't do anything if this message will be ignored anyway.
  log4cxx::LoggerPtr myLogger(log4cxx::Logger::getLogger(cat));
  if ( myLogger )
  {
    log4cxx::LevelPtr myLevel = myLogger->getLevel();
    log4cxx::LevelPtr paramLevel = log4cxx::Level::toLevel(level);
    if ( myLevel && paramLevel )
    {
        // if no logging - return; if configured logging is more restrictive
        //  (greater than) the requested level - return
        if ( myLevel == log4cxx::Level::getOff() || myLevel->toInt() > paramLevel->toInt() )
        {
          return;
        }
    }
  }
  
  va_list args ;
  va_start(args, logMsgTemplate);

  char* buffer = buildMsgBuffer(cat, level, logMsgTemplate, args);
  log1(cat, level, buffer);

  va_end(args);

}
