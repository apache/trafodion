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

#ifndef _QRLOGGER_H_
#define _QRLOGGER_H_

#include "CommonLogger.h"

// qmscomon
extern const char CAT_QR_COMMON[];
extern const char CAT_QR_IPC[];
extern const char CAT_MEMORY[];
extern const char CAT_RANGE[];
extern const char CAT_QR_TRACER[];

// QMM
extern const char CAT_QMM[];
// QMP
extern const char CAT_QMP[];
// QMS
extern const char CAT_QMS_MAIN[];
extern const char CAT_QMS_INIT[];
extern const char CAT_MVMEMO_JOINGRAPH[];
extern const char CAT_MVMEMO_STATS[];
extern const char CAT_GRP_LATTCE_INDX[];
extern const char CAT_MATCHTST_MVDETAILS[];
extern const char CAT_QMS_XML[];
// MXCMP
extern const char CAT_QR_DESC_GEN[];
extern const char CAT_QR_HANDLER[];
extern const char CAT_MVCAND[];
extern const char CAT_CMP_XML[];

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
    QRL_QMS,
    QRL_QMP,
    QRL_QMM
  };

  /**
    * Initializes log4cpp by using the configuration file.
    * If the path given is relative (does not start with a
    * slash), it is appended to the $MY_SQROOT environment variable.
    * If the configuration file is not found, perform hard-coded
    * default configuration.
    * @param configFileName name of the log4cpp configuration file.
    * @return FALSE if the configuration file is not found.
    */
  virtual NABoolean initLog4cpp(const char* configFileName);

  /**
   * Is this logger being used by QMS, QMM, QMP or MXCMP?
   * Must be called before initLog4cpp().
   * Used for the default initialization in case the log4cpp configuration
   * file is not found.
   * @param module Specify the module in use.
   */
  void setModule(ExecutableModule module)
  {
    module_ = module;
  }

  /**
    * Returns a reference to the %QRLogger singelton instance in use.
    * @return Reference to the singleton instance of this class.
    */
  static QRLogger& instance();

protected:

    void initCategory(const char* cat, log4cpp::Priority::PriorityLevel defaultPriority);

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
  log4cpp::Appender *fileAppender_;

  /** Is this QMS, QMM, QMP or MXCMP? */
  ExecutableModule module_;

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
 * Exception thrown when an error in the program logic is found.
 */
// LCOV_EXCL_START :rfi
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
// LCOV_EXCL_STOP

#endif  /* _QRLOGGER_H_ */
