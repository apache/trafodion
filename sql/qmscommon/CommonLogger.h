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

#ifndef _COMMONLOGGER_H_
#define _COMMONLOGGER_H_

#include <stdarg.h>
#include <stdio.h>
#include <log4cpp/Category.hh>
#include "NABoolean.h"
#include "NAString.h"

/**
 * \file 
 * Contains the CommonLogger class and some defines to facilitate logging requests.
 * This file should be included by files that use logging.
 */

class ComDiagsArea;

// Indicator for logging level.
enum logLevel {
   LL_FATAL = log4cpp::Priority::FATAL
  ,LL_ALERT = log4cpp::Priority::ALERT
  ,LL_ERROR = log4cpp::Priority::ERROR
  ,LL_WARN  = log4cpp::Priority::WARN
  ,LL_INFO  = log4cpp::Priority::INFO
  ,LL_DEBUG = log4cpp::Priority::DEBUG

  // This is the level we use for an error that prevents rewriting a query, but
  // allows the query to proceed without being rewritten. It's value places it
  // between LL_WARN and LL_ERROR in terms of priority.
  ,LL_MVQR_FAIL = 350
};

/**
 * Asserts an invariant condition, if the conditions fails,
 * log the message and throw an exception.
 *
 * @param cat The category that is logging. 
 * @param level The priority level of Message.
 * @param exception The exception class to throw.
 * @param condition The condition to check.
 * @param msg Message to write to the log in case of a failure.
 */
#define assertLogAndThrow(cat, level, condition, exception, msg) \
  { \
    if (!(condition)) \
      { \
        CommonLogger::logError(__FILE__, __LINE__, cat, level, msg); \
        throw exception(msg); \
      } \
  }

/**
 * Like assertLogAndThrow above, but supply an argument for the mesage.
 *
 * @param cat The category that is logging. 
 * @param level The priority level of Message.
 * @param exception The exception class to throw.
 * @param condition The condition to check.
 * @param msg Message to write to the log in case of a failure.
 * @param arg1 Argument to embed in the message template.
 */
#define assertLogAndThrow1(cat, level, condition, exception, msg, arg1) \
  { \
    if (!(condition)) \
      { \
        CommonLogger::logError(__FILE__, __LINE__, cat, level, msg, arg1); \
        throw exception(msg, arg1); \
      } \
  }

/**
 * Like assertLogAndThrow above, but supply two arguments for the message.
 *
 * @param cat The category that is logging. 
 * @param level The priority level of Message.
 * @param exception The exception class to throw.
 * @param condition The condition to check.
 * @param msg Message to write to the log in case of a failure.
 * @param arg1 Argument to embed in the message template.
 * @param arg2 2nd argument to embed in the message template.
 */
#define assertLogAndThrow2(cat, level, condition, exception, msg, arg1, arg2) \
  { \
    if (!(condition)) \
      { \
        CommonLogger::logError(__FILE__, __LINE__, cat, level, msg, arg1, arg2); \
        throw exception(msg, arg1, arg2); \
      } \
  }

/**
 * Three-argument version of assertLogAndThrow.
 *
 * @param cat The category that is logging. 
 * @param level The priority level of Message.
 * @param exception The exception class to throw.
 * @param condition The condition to check.
 * @param msg Message to write to the log in case of a failure.
 * @param arg1 Argument to embed in the message template.
 * @param arg2 2nd argument to embed in the message template.
 * @param arg3 3rd argument to embed in the message template.
 */
#define assertLogAndThrow3(cat, level, condition, exception, msg, arg1, arg2, arg3) \
  { \
    if (!(condition)) \
      { \
        CommonLogger::logError(__FILE__, __LINE__, cat, level, msg, arg1, arg2, arg3); \
        throw exception(msg, arg1, arg2, arg3); \
      } \
  } 

/**
 * A generic logger class encapsulating the log4cpp library.
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
    * Initializes log4cpp by using the configuration file.
    * If the path given is relative (does not start with a
    * slash), it is appended to the $MY_SQROOT environment variable.
    * @param configFileName name of the log4cpp configuration file.
    * @return FALSE if the configuration file is not found.
    */
  virtual NABoolean initLog4cpp(const char* configFileName);

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
  static void log1(const char* cat, 
                   logLevel    level, 
                   const char* cmsg,
                   ULng32      eventId = 0);

  static void log(const char* cat, 
                  logLevel    level, 
                  const char* logMsgTemplate ...);

  static void logDiags(ComDiagsArea* diagsArea,
                       const char*   cat);

  static void logQVP(ULng32      eventId,
                     const char* cat, 
                     logLevel    level, 
                     const char* logMsgTemplate ...);

  /**
    * Log an error or exception message.
    * Enters a message in the log. \c logMsgTemplate supplies a 
    * printf-style template for constructing the message text, and
    * the arguments used to fill in the placeholders in the template are
    * supplied in a variable argument list.
    * This method should be used only for exceptions, as the message size is 
    * limited to a single line of text, and the message is also sent to the 
    * EMS log on NSK, and to SeaLog on Seaquest.
    * 
    * @param[in] file The source file name.
    * @param[in] line The line number where the error occured.
    * @param[in] cat The logging category to use.
    * @param[in] level The logging level.
    * @param[in] logMsgTemplate The message template.
    * @param[in] ... Variable argument list supplying values to insert in the
    *                message template.
    */
  static void logError(const char* file, 
                       Int32       line, 
                       const char* cat, 
                       logLevel    level, 
                       const char *logMsgTemplate ...);

  /**
    * Is the category set to log DEBUG messages?
    * This method should be used when the work of producing the logging text 
    * is expensive, such as a join graph or an extensive dump.
    * @param cat The name of the category to check.
    * @return TRUE if DEBUG messages are logged.
    */
  static NABoolean isCategoryInDebug(const char* cat);

protected:
  static char* buildMsgBuffer(const char* cat, 
                              logLevel    level, 
                              const char* logMsgTemplate,
                              va_list     args);

  virtual void initLogsFolder(const char* configFileName);

  NAString logFolder_;

private:
  // Copy constructor and assignment operator are not defined.
  CommonLogger(const CommonLogger&);
  CommonLogger& operator=(const CommonLogger&);
};

/**
 * Class used for tracing function entry/exit, or to provide a stack trace when
 * an exception is thrown. This class is designed to be used in conjunction
 * with a macro such as QRTRACER (see in qmscommon/QRLogger.h). 
 * An instance of the object should be declared at the
 * beginning of a function or block to be traced. The ctor logs the entry message,
 * and the dtor will automatically log the exit message on function exit, even
 * when an exception is thrown.
 * \n\n
 * The default mode of operation is to only trace function exits when the stack
 * is being unwound in response to an exception being thrown. In this case, the
 * exit message notes that an exception is being handled.
 *
 * @see QRTRACER
 ***************************************************************************
 */
class CommonTracer
{
  public:
    /**
     * Enumeration representing the different possible trace levels.
     */
    enum TraceLevel
      {
        // The order of these enum values is significant.
        TL_none, TL_exceptionOnly, TL_all
      };

    /**
     * Instantiates the tracing object and (depending on the trace level),
     * writes the function entry message to the log.
     *
     * @param fnName Text to be used in the entry exit messages.
     * @param logger The logger instance to log to.
     * @param logCategory The logger category to use.
     * @param level The trace level.
     * @param file Name of the file the function being traced is in.
     * @param line Line number at which this object was declared.
     */
    CommonTracer(const char*   fnName, 
                 CommonLogger& logger, 
                 const char*   logCategory,
                 TraceLevel    level = TL_exceptionOnly,
                 const char*   file = "", 
                 Int32         line = 0);

    /**
     * Writes the exit message to the log.
     */
    virtual ~CommonTracer();

  private:
    //* Name to be used in the entry/exit messages. */
    NAString fnName_;

    //* Tracing level in effect. */
    TraceLevel level_;

    //* Name of the file the traced function is in. */
    NAString file_;

    //* Line at which the trace object was declared. */
    Int32 line_;

    // The logger to use.
    CommonLogger& logger_;

    // The logger category
    const char* category_;
};  // CommonTracer

/**
 * Builds a char buffer from a template and variable set of arguments. This
 * define should be invoked from within a function that takes a variable
 * argument list. \c messageTemplate must be the function parameter that
 * precedes the ... in the function's argument list. \c messageBuffer is the
 * char buffer the message is built in.
 * \n\n
 * This is used in the constructors for the various classes of the QRException
 * hierarchy. The variable argument list can't be passed without creating a
 * va_list, so each must build the exception message itself. They all use a
 * member variable of QRException as the buffer to store the composed message in.
 *
 * @param messageTemplate 
 * @param messageBuffer 
 */
#define qrBuildMessage(messageTemplate, messageBuffer) \
  { \
    va_list args; \
    va_start(args, messageTemplate); \
    vsprintf(messageBuffer, messageTemplate, args); \
    va_end(args); \
  }

/**
 * A generic exception class. This class owns the message buffer, but it is
 * set by its subclass' constructors, because they all take variable
 * argument lists.
 */
class QRException
{
  public:
    QRException()
      {}

    virtual ~QRException()
      {}

    /**
     * Returns the message constructed when the exception was instantiated.
     * @return The full exception message.
     */
    char* getMessage()
      {
        return msgBuffer_;
      }

  protected:
    /** Buffer used to construct the message in. */
    char msgBuffer_[200];
}; //QRException

#endif  /* _COMMONLOGGER_H_ */
