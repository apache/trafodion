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
#include <stdarg.h>
#include "CommonLogger.h"
#include <errno.h>
#include <string.h>
#include <stdexcept>
// ComDiags.h must precede logmxevent.h, or get a bunch of syntax errors on
// Linux builds.
#include "ComDiags.h"
#include "logmxevent.h"
#include "NLSConversion.h"
#include <log4cpp/Category.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/Configurator.hh>

  #define SLASH_S "/"
  #define SLASH_C '/'

// **************************************************************************
// If the logFileName is fully qualified (starts with a slash) then use its
// path.
// Otherwise, try the $MY_SQROOT environment variable.
// If its not defined either use an empty string in order to use the current 
// directory.
// **************************************************************************
void CommonLogger::initLogsFolder(const char* configFileName)
{
  if (configFileName[0] == SLASH_C)
  {
    // LCOV_EXCL_START :cnu
    // Absolute path. Use the config folder.
    logFolder_ = configFileName;
    // Remove the config file name.
    Int32 pos = logFolder_.last(SLASH_C); // Find last slash
    logFolder_.remove(pos+1); // Make it the last character.
    // LCOV_EXCL_STOP
  }
  else
  {
    // The log files are written to $MY_SQROOT/logs
    // The config file is expected to be in the same folder.
    char *installPath = getenv("MY_SQROOT");
    if (installPath)
    {
      logFolder_ = installPath;
      // Last char may or may not be a slash.
      if (logFolder_[logFolder_.length()-1] != SLASH_C)
        logFolder_ += SLASH_C;  // LCOV_EXCL_LINE :nu

      logFolder_ += "logs" SLASH_S;
    }
    else
    {
      // Environment variable $MY_SQROOT not found.
      // use the current directory.
      logFolder_ = "";
    }
  }
}

// **************************************************************************
// Reads configuration file, creates an appender, layout, and categories.
// Attaches layout to the appender and appender to categories.
// Will be called by constructor qms, qmp, qmm, and mxcmp after calling 
// setLogFileName().
// Return TRUE if all was well, or FALSE if the config file was not read
// successfully.
// **************************************************************************
NABoolean CommonLogger::initLog4cpp(const char* configFileName)
{
  NAString configPath;

  initLogsFolder(configFileName);

  if (configFileName[0] == SLASH_C)
    configPath = configFileName;
  else
  {
    configPath = logFolder_;
    configPath += configFileName;
  }

  try 
  {
    log4cpp::PropertyConfigurator::configure(configPath.data(), logFolder_.data());
    return TRUE;
  }
  catch (log4cpp::ConfigureFailure e) 
  {
    return FALSE;
  }
}

// **************************************************************************
// Is the category set to log DEBUG messages?
// This method should be used when the work of producing the logging text 
// is expensive, such as a join graph or an extensive dump.
// **************************************************************************
NABoolean CommonLogger::isCategoryInDebug(const char* cat)
{
  log4cpp::Category &myCat = log4cpp::Category::getInstance(cat);
  return (myCat.getPriority() >= log4cpp::Priority::DEBUG);
}

// **************************************************************************
// Log an exception-type error message:
// First log "Error thrown at line <line> in file <filename>:"
// Then log the actual error message, which is passed with a variable 
// number of parameters. The error message is epected to be a single line 
// of text not exceeding 2K.
// This error message can be logged as either ERROR or FATAL type.
// Both lines of text are sent to both the log file, and to the EMS/SeaLog server.
// MVQR failures that prevent only rewrite, and not execution of the original
// form of the query, cause an informational rather than an error notification
// to be entered in the system log.
// **************************************************************************
// LCOV_EXCL_START :rfi
void CommonLogger::logError(const char* file, 
                            Int32       line, 
                            const char* cat,
                            logLevel    level,
                            const char* logMsgTemplate...)
{
  #define BUFFER_SIZE 2048
  char buffer[BUFFER_SIZE]; // This method expects a single line to text.
  if (level == LL_MVQR_FAIL)
    snprintf(buffer, sizeof(buffer), "Attempted MVQR operation (rewrite of query or publication of MV) failed at line %d in file %s:", line, file);
  else
    snprintf(buffer, sizeof(buffer), "Error thrown at line %d in file %s:", line, file);

  va_list args;
  va_start(args, logMsgTemplate);

  if (level == LL_FATAL)
    log4cpp::Category::getInstance(cat).fatal(buffer);
  else
    log4cpp::Category::getInstance(cat).error(buffer);

  // first send auxiliary msg with file and line info to event server.
  if (level == LL_MVQR_FAIL)
    SQLMXLoggingArea::logMVQRInfoEvent(buffer);
  else
    SQLMXLoggingArea::logMVQRErrorEvent(buffer);

  // format actual error message
  vsnprintf(buffer, BUFFER_SIZE, logMsgTemplate, args);

  // log actual error msg to logfile.
  if (level == LL_FATAL)
    log4cpp::Category::getInstance(cat).fatal(buffer);
  else
    log4cpp::Category::getInstance(cat).error(buffer);

  // Send real error msg to event server. 
  if (level == LL_MVQR_FAIL)
    SQLMXLoggingArea::logMVQRInfoEvent(buffer);
  else
    SQLMXLoggingArea::logMVQRErrorEvent(buffer);

  va_end(args);
}
// LCOV_EXCL_STOP


// **************************************************************************
// The generic message logging method for any message type and length.
// **************************************************************************
void CommonLogger::log1(const char* cat, 
                        logLevel    level, 
                        const char* cmsg,
                        ULng32      eventId)
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

      // Send to EMS log or SeaLog.
      if (eventId)
        SQLMXLoggingArea::logCommonLoggerFailureEvent(eventId, cmsg);
      else
        SQLMXLoggingArea::logMVQRFailureEvent(cmsg);
      break;

    // LCOV_EXCL_START :cnu
    case LL_ALERT:
      log4cpp::Category::getInstance(cat).alert(msg);
      break;
    // LCOV_EXCL_STOP

    case LL_ERROR:
      log4cpp::Category::getInstance(cat).error(msg);

      // Send to EMS log or SeaLog.
      if (eventId)
        SQLMXLoggingArea::logCommonLoggerErrorEvent(eventId, cmsg);
      else
        SQLMXLoggingArea::logMVQRErrorEvent(cmsg);
      break;

    case LL_MVQR_FAIL:
      log4cpp::Category::getInstance(cat).error(msg);

      // Send to EMS log or SeaLog.
      SQLMXLoggingArea::logMVQRInfoEvent(cmsg);
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
  Int32 bufferSize = 20049;
  Int32 msgSize = 0;
  char *buffer = new char[bufferSize];
  va_list args2;
  va_copy(args2, args);
  NABoolean secondTry = FALSE;

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

void CommonLogger::logDiags(ComDiagsArea* diagsArea, const char* cat)
{
  const NAWchar* diagMsg;
  NAString* diagStr;
  Int32 i;
  ComCondition* condition;
  log4cpp::Category& myCat = log4cpp::Category::getInstance(cat);
  
  if (myCat.getPriority() >= LL_ERROR)
    for (i=1; i<=diagsArea->getNumber(DgSqlCode::ERROR_); i++)
      {
        condition = diagsArea->getErrorEntry(i);
        diagMsg = condition->getMessageText();
        diagStr = unicodeToChar(diagMsg, NAWstrlen(diagMsg),
                                          CharInfo::ISO88591, NULL, TRUE);
        log(cat, LL_ERROR,
          "  condition %d: sqlcode %d, %s", i, condition->getSQLCODE(), diagStr->data());
        delete diagStr;
      }

  if (myCat.getPriority() >= LL_WARN)
    for (i=1; i<=diagsArea->getNumber(DgSqlCode::WARNING_); i++)
      {
        condition = diagsArea->getWarningEntry(i);
        diagMsg = condition->getMessageText();
        diagStr = unicodeToChar(diagMsg, NAWstrlen(diagMsg),
                                          CharInfo::ISO88591, NULL, TRUE);
        log(cat, LL_WARN,
          "  condition %d: sqlcode %d, %s", i, condition->getSQLCODE(), diagStr->data());
        delete diagStr;
      }
}

// This temporary function is used for logging QVP messages only. More extensive
// changes to the CommonLogger hierarchy will be made when the QI integration
// stream is merged into the mainline. For now, we avoid making changes that
// affect MVQR to minimize merge difficulties.
void CommonLogger::logQVP(ULng32 eventId,
                          const char* cat,
                          logLevel    level,
                          const char* logMsgTemplate...)
{
  // Don't do anything if this message will be ignored anyway.
  log4cpp::Category &myCat = log4cpp::Category::getInstance(cat);
  if (myCat.getPriority() < level)
    return;

  va_list args;
  va_start(args, logMsgTemplate);

  char* buffer = buildMsgBuffer(cat, level, logMsgTemplate, args);
  log1(cat, level, buffer, eventId);
  delete [] buffer;

  va_end(args);
}

// **************************************************************************
// **************************************************************************
CommonTracer::CommonTracer(const char*   fnName,  
                           CommonLogger& logger,
                           const char*   logCategory,
                           TraceLevel    level, 
                           const char*   file, 
                           Int32         line)
  : fnName_(fnName),
    level_(level),
    file_(file ? file : ""),
    line_(line),
    logger_(logger),
    category_(logCategory)
{
  if (level_ == TL_all)
  {
    // LCOV_EXCL_START :cnu
    if (file_.length() == 0)
      logger_.log(category_, LL_DEBUG, 
                  "Entering %s", fnName_.data()); 
    else
      logger_.log(category_, LL_DEBUG,
                  "Entering %s (file %s, line %d)",
                  fnName_.data(), file_.data(), line_); 
    // LCOV_EXCL_STOP
  }
}

// **************************************************************************
// **************************************************************************
CommonTracer::~CommonTracer()
{
  NAString logMsg;

  if (level_ >= TL_exceptionOnly && std::uncaught_exception())
    logMsg.append("Exiting %s with uncaught exception");
  else if (level_ == TL_all)
    logMsg.append("Exiting %s");  // LCOV_EXCL_LINE :cnu
  else
    return;
    
  if (file_.length() > 0)
  {
    // LCOV_EXCL_START :cnu
    logMsg.append("(file %s, line %d)");
    logger_.log(category_, LL_DEBUG,
                logMsg, fnName_.data(), file_.data(), line_);
    // LCOV_EXCL_STOP
  }
  else
  {
    logger_.log(category_, LL_DEBUG, logMsg, fnName_.data());
  }
}
