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

#ifndef _QRLOGGER_H_
#define _QRLOGGER_H_

#include "NABoolean.h"
#include "NAString.h"
#include "CommonLogger.h"

// -----  these categories are currently not used
// qmscomon
extern std::string CAT_SQL_COMP_QR_COMMON;
extern std::string CAT_SQL_COMP_QR_IPC;
extern std::string CAT_SQL_MEMORY;
extern std::string CAT_SQL_COMP_RANGE;
extern std::string CAT_QR_TRACER;
// QMM
extern std::string CAT_SQL_QMM;
// QMP
extern std::string CAT_SQL_QMP;
// QMS
extern std::string CAT_SQL_QMS_MAIN;
extern std::string CAT_SQL_QMS_INIT;
extern std::string CAT_SQL_QMS_MVMEMO_JOINGRAPH;
extern std::string CAT_SQL_QMS_MVMEMO_STATS;
extern std::string CAT_SQL_QMS_GRP_LATTCE_INDX;
extern std::string CAT_SQL_QMS_MATCHTST_MVDETAILS;
extern std::string CAT_SQL_QMS_XML;

// MXCMP
extern std::string CAT_SQL_COMP_QR_DESC_GEN;
extern std::string CAT_SQL_COMP_QR_HANDLER;
extern std::string CAT_SQL_COMP_MV_REFRESH;
extern std::string CAT_SQL_COMP_MVCAND;
extern std::string CAT_SQL_COMP_XML;
// ---- these categories are currently used
extern std::string CAT_SQL_EXE;
extern std::string CAT_SQL_COMP;
extern std::string CAT_SQL_ESP;
extern std::string CAT_SQL_SSCP;
extern std::string CAT_SQL_PRIVMGR;

// HDFS
extern std::string CAT_SQL_HDFS_JNI_TOP;
extern std::string CAT_SQL_HDFS_SEQ_FILE_READER;
extern std::string CAT_SQL_HDFS_SEQ_FILE_WRITER;
extern std::string CAT_SQL_HDFS_ORC_FILE_READER;
extern std::string CAT_SQL_HBASE;
extern std::string CAT_SQL_HDFS;

class ComDiagsArea;

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
        QRLogger::logError(__FILE__, __LINE__, cat, level, msg); \
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
        QRLogger::logError(__FILE__, __LINE__, cat, level, msg, arg1); \
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
        QRLogger::logError(__FILE__, __LINE__, cat, level, msg, arg1, arg2); \
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
        QRLogger::logError(__FILE__, __LINE__, cat, level, msg, arg1, arg2, arg3); \
        throw exception(msg, arg1, arg2, arg3); \
      } \
  }


/**
 * A logging class for MVQR. This is a singleton class, the sole instance
 * being accessible via the #instance function. 
 **************************************************************************
 */
class QRLogger : public CommonLogger
{
public:

  // Which of the four MVQR executable modules is running?
  enum ExecutableModule {
    QRL_NONE,
    QRL_MXCMP,
    QRL_MXEXE,
    QRL_QMS,
    QRL_QMP,
    QRL_QMM,
    QRL_ESP,
    QRL_LOB,
    QRL_SSMP,
    QRL_SSCP,
    QRL_UDR 
  };

  /**
    * Initializes log4cxx by using the configuration file.
    * If the path given is relative (does not start with a
    * slash), it is appended to the $TRAF_HOME environment variable.
    * If the configuration file is not found, perform hard-coded
    * default configuration.
    * @param configFileName name of the log4cxx configuration file.
    * @return FALSE if the configuration file is not found.
    */
  virtual NABoolean initLog4cxx(const char* configFileName);

  /**
   * Is this logger being used by QMS, QMM, QMP or MXCMP?
   * Must be called before initlog4cxx().
   * Used for the default initialization in case the log4cxx configuration
   * file is not found.
   * @param module Specify the module in use.
   */
  void setModule(ExecutableModule module)
  {
    module_ = module;
  }

  
  /**
   * Is this logger being used by EXE, ESP or MXCMP?
   * @return the executable module of this process
   */
  ExecutableModule getModule()
  {
    return module_;
  }

  std::string &getMyDefaultCat();
  const char* getMyProcessInfo();

  // overrides the method in CommonLogger so process information can be added
  static void log(std::string &cat,
                  logLevel    level,
                  const char* logMsgTemplate ...);
  static void log(std::string &cat,
                  logLevel    level,
                  int         sqlCode,
                  const char* queryId,
                  const char* logMsgTemplate ...);

  /**
    * Returns a reference to the %QRLogger singelton instance in use.
    * @return Reference to the singleton instance of this class.
    */
  static QRLogger& instance();

  static void logDiags(ComDiagsArea* diagsArea,
                       std::string &cat);

  static void logQVP(ULng32      eventId,
                     std::string &cat,
                     logLevel    level,
                     const char* logMsgTemplate ...);


  /**
    * Log an error or exception message.
    * Enters a message in the log. \c logMsgTemplate supplies a 
    * printf-style template for constructing the message text, and
    * the arguments used to fill in the placeholders in the template are
    * supplied in a variable argument list.
    * This method should be used only for exceptions, as the message size is 
    * limited to a single line of text
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
                       std::string &cat,
                       logLevel    level, 
                       const char *logMsgTemplate ...);

  void introduceSelf();
  
  static NABoolean initLog4cxx(ExecutableModule module);

protected:

   void initCategory(std::string &cat, log4cxx::LevelPtr defaultPriority);

private:
    /**
     * Creates the single instance of this class, with logging initially enabled.
     * Append mode must be used to prevent separate processes from overwriting
     * each other's logged messages.
     */
    QRLogger();

    virtual ~QRLogger() {};
    QRLogger(const QRLogger&);
    QRLogger& operator=(const QRLogger&);

private:

  /** The appender. */
  log4cxx::Appender *fileAppender_;

  /** Is this QMS, QMM, QMP or MXCMP? */
  ExecutableModule module_;

  // information about the encompassing process
  // node number, CPU, PIN, process name
  NAString processInfo_;

}; // QRLogger

/**
 * Define used to enable tracing for a given function. When in debug mode, it
 * expands to a declaration of a CommonTracer object, which will cause function
 * entry/exit messages to be posted to the log.
 *
 * @param fn Text to be incorporated into the entry/exit messages, typically
 *           the name of the function.
 * @see CommonTracer
 */
#ifdef _DEBUG
#define QRTRACER(fn) CommonTracer _tracer_(fn, QRLogger::instance(), CAT_QR_TRACER)
#else
#define QRTRACER(fn)
#endif

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
                 std::string   &logCategory,
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
    std::string &category_;
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



/**
 * Exception thrown when an error in the program logic is found.
 */
class QRLogicException : public QRException
{
  public:
    /**
     * Creates an exception with text consisting of the passed template filled in
     * with the values of the other arguments.
     *
     * @param[in] msgTemplate Template for construction of the full message;
     *                        contains printf-style placeholders for arguments,
     *                        passed as part of a variable argument list.
     * @param[in] ... Variable argument list, consisting of a value for each
     *                placeholder in the message template.
     */
    QRLogicException(const char *msgTemplate ...)
      : QRException()
    {
      qrBuildMessage(msgTemplate, msgBuffer_);
    }

    virtual ~QRLogicException()
    {}

}; //QRLogicException

#endif  /* _QRLOGGER_H_ */
