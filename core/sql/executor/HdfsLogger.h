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

#ifndef _HDFSLOGGER_H_
#define _HDFSLOGGER_H_

#include <CommonLogger.h>

extern const char CAT_JNI_TOP[];
extern const char CAT_ORC_FILE_READER[];
extern const char CAT_SEQ_FILE_READER[];
extern const char CAT_SEQ_FILE_WRITER[];
extern const char CAT_HBASE[];

/**
 * A logging class for MVQR. This is a singleton class, the sole instance
 * being accessible via the #instance function. 
 **************************************************************************
 */
class HdfsLogger : public CommonLogger
{
public:

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
    * Returns a reference to the %QRLogger singelton instance in use.
    * @return Reference to the singleton instance of this class.
    */
  static HdfsLogger& instance();

protected:

    void initCategory(const char* cat, log4cxx::Priority::PriorityLevel defaultPriority);

private:
    /**
     * Creates the single instance of this class, with logging initially enabled.
     * Append mode must be used to prevent separate processes from overwriting
     * each other's logged messages.
     */
    HdfsLogger();

    virtual ~HdfsLogger() {};
    HdfsLogger(const HdfsLogger&);
    HdfsLogger& operator=(const HdfsLogger&);

private:

  /** The appender. */
  log4cxx::Appender *fileAppender_;

}; // HdfsLogger

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
