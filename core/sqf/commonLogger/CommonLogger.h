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

#ifndef _COMMONLOGGER_H_
#define _COMMONLOGGER_H_

#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <log4cxx/logger.h>
#include <log4cxx/level.h>
#include <log4cxx/appender.h>
#include <log4cxx/helpers/exception.h>

extern bool gv_commonLoggerInitialized;

using namespace log4cxx;
using namespace log4cxx::helpers;

/**
 * \file
 * Contains the CommonLogger class and some defines to facilitate logging requests.
 * This file should be included by files that use logging.
 */

// Indicator for logging level.
  enum logLevel {
   LL_FATAL = log4cxx::Level::FATAL_INT
  ,LL_ERROR = log4cxx::Level::ERROR_INT
  ,LL_WARN  = log4cxx::Level::WARN_INT
  ,LL_INFO  = log4cxx::Level::INFO_INT
  ,LL_DEBUG = log4cxx::Level::DEBUG_INT
  ,LL_TRACE = log4cxx::Level::TRACE_INT

  // This is the level we use for an error that prevents rewriting a query, but
  // allows the query to proceed without being rewritten. It's value places it
  // between LL_WARN and LL_ERROR in terms of priority.
  ,LL_MVQR_FAIL = 35000
};

/**
 * A generic logger class encapsulating the log4cxx library.
 */
class CommonLogger
{
public:
  /**
    * Creates the single instance of this class, with logging initially enabled.
    * Append mode must be used to prevent separate processes from overwriting
    * each other's logged messages.
    */
  CommonLogger()
  {}

  virtual ~CommonLogger()
  {}

  /**
    * Returns a reference to the %QRLogger singelton instance in use.
    * @return Reference to the singleton instance of this class.
    */
  static CommonLogger& instance();

  /**
    * Initializes log4cxx by using the configuration file.
    * If the path given is relative (does not start with a
    * slash), it is appended to the $TRAF_HOME environment variable.
    * @param configFileName name of the log4cxx configuration file.
    * @return FALSE if the configuration file is not found.
    */
  virtual bool initLog4cxx(const char* configFileName, const char* fileSuffix=NULL);

  /**
    * Enters a message in the log. \c logMsgTemplate supplies a
    * printf-style template for constructing the message text, and
    * the arguments used to fill in the placeholders in the template are
    * supplied in a variable argument list.
    * This method can handle messages of arbitrary length.
    *
    * @param[in] cat The logging category to use.
    * @param[in] level The logging priority to use.
    * @param[in] logMsgTemplate The message template.
    * @param[in] ... Variable argument list supplying values to insert in the
    *                message template.
    */

  static void log1(std::string &cat,
                   logLevel    level,
                   const char* cmsg,
                   unsigned int eventId = 0);

  static void log(std::string &cat,
                  logLevel    level,
                  const char* logMsgTemplate ...);

  /**
    * Is the category set to log DEBUG messages?
    * This method should be used when the work of producing the logging text
    * is expensive, such as a join graph or an extensive dump.
    * @param cat The name of the category to check.
    * @return TRUE if DEBUG messages are logged.
    */
  static bool isCategoryInDebug(std::string &cat);


protected:
  static char* buildMsgBuffer(std::string &cat,
                              logLevel    level,
                              const char* logMsgTemplate,
                              va_list     args);

  std::string logFolder_;

private:
  // Copy constructor and assignment operator are not defined.
  CommonLogger(const CommonLogger&);
  CommonLogger& operator=(const CommonLogger&);
};

#endif  /* _COMMONLOGGER_H_ */
