// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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
#include <log4cpp/Category.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/Configurator.hh>

#define SLASH_S "/"
#define SLASH_C '/'


// **************************************************************************
// Reads configuration file, creates an appender, layout, and categories.
// Attaches layout to the appender and appender to categories.
// Will be called by constructor qms, qmp, qmm, and mxcmp after calling
// setLogFileName().
// Return TRUE if all was well, or FALSE if the config file was not read
// successfully.
// **************************************************************************
bool CommonLogger::initLog4cpp(const char* configFileName, const char* fileSuffix)
{
  std::string configPath;

  logFolder_ = "";

  // Form the config path $MY_SQROOT/conf
  char *installPath = getenv("MY_SQROOT");
  if (installPath)
  {
	configPath = installPath;
    // Last char may or may not be a slash.
    if (configPath[configPath.length()-1] != SLASH_C)
	  configPath += SLASH_C;  // LCOV_EXCL_LINE :nu

    configPath += "conf" SLASH_S;
  }
  else
  {
    // Environment variable $MY_SQROOT not found.
    // use the current directory.
    configPath = "";
  }

  if (configFileName[0] == SLASH_C)
    configPath = configFileName;
  else
    configPath += configFileName;

/*
  if (configFileName[0] == SLASH_C)
    configPath = configFileName;
  else
  {
    configPath = logFolder_;
    configPath += configFileName;
  }
*/

  try
  {
    log4cpp::PropertyConfigurator::configure(configPath.data(), logFolder_.data(), fileSuffix);
    return true;
  }
  catch (log4cpp::ConfigureFailure e)
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
bool CommonLogger::isCategoryInDebug(const char* cat)
{
  log4cpp::Category &myCat = log4cpp::Category::getInstance(cat);
  return (myCat.getPriority() >= log4cpp::Priority::DEBUG);
}

// **************************************************************************
// The generic message logging method for any message type and length.
// **************************************************************************
void CommonLogger::log1(const char* cat,
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

  switch(level)
  {
    case LL_FATAL:
      log4cpp::Category::getInstance(cat).fatal(msg);
      break;

    // LCOV_EXCL_START :cnu
    case LL_ALERT:
      log4cpp::Category::getInstance(cat).alert(msg);
      break;
    // LCOV_EXCL_STOP

    case LL_ERROR:
      log4cpp::Category::getInstance(cat).error(msg);
      break;

    case LL_MVQR_FAIL:
      log4cpp::Category::getInstance(cat).error(msg);
      break;

    case LL_WARN:
      log4cpp::Category::getInstance(cat).warn(msg);
      break;

    case LL_INFO:
      log4cpp::Category::getInstance(cat).info(msg);
      break;

    case LL_DEBUG:
      log4cpp::Category::getInstance(cat).debug(msg);
      break;
  }
}

char* CommonLogger::buildMsgBuffer(const char* cat,
                                   logLevel    level,
                                   const char* logMsgTemplate,
                                   va_list     args)
{
  // calculate needed bytes to allocate memory from system heap.
  int bufferSize = 20049;
  int msgSize = 0;
  char *buffer = new char[bufferSize];
  va_list args2;
  va_copy(args2, args);
  bool secondTry = false;

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

      log4cpp::Category::getInstance(cat).error("Cannot log full message, error %d returned from vsnprintf()", msgSize);
      log4cpp::Category::getInstance(cat).error(smallBuff);

      return buffer;
      // LCOV_EXCL_STOP
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

// **************************************************************************
// The generic message logging method for any message type and length.
// **************************************************************************
void CommonLogger::log(const char* cat,
                       logLevel    level,
                       const char* logMsgTemplate...)
{
  // Don't do anything if this message will be ignored anyway.
  log4cpp::Category &myCat = log4cpp::Category::getInstance(cat);
  if (myCat.getPriority() < level)
    return;

  va_list args ;
  va_start(args, logMsgTemplate);

  char* buffer = buildMsgBuffer(cat, level, logMsgTemplate, args);
  log1(cat, level, buffer);
  delete [] buffer;

  va_end(args);
}
