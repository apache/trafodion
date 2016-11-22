//******************************************************************************
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
//******************************************************************************
#ifndef INCLUDE_AUTHEVENT_H
#define INCLUDE_AUTHEVENT_H  1
#include "common/evl_sqlog_eventnum.h"
#include "CommonLogger.h"
#include <string>
#include <vector>

// From a web search, the log message max length is a bit over 8000 bytes.
// For DBSecurity don't believe any messages will be more than 1M
#define MAX_EVENT_MSG_SIZE 1024

// Different outcomes can be returned when authenticating the user. 
// AUTH_OUTCOME is a status for each of these outcomes.
// getAuthOutcome translates the status into text form.
enum AUTH_OUTCOME{
  AUTH_OK               = 0,
  AUTH_REJECTED         = 1,
  AUTH_FAILED           = 2,
  AUTH_NOT_REGISTERED   = 3,
  AUTH_MD_NOT_AVAILABLE = 4,
  AUTH_USER_INVALID     = 5,
  AUTH_TYPE_INCORRECT   = 6,
  AUTH_NO_PASSWORD      = 7,
};

std::string getAuthOutcome(AUTH_OUTCOME outcome);

// The ported code had the caller sending in the filename and line number
// for certain events.  This has not been implemented at this time.
class AuthEvent
{
  private:

  DB_SECURITY_EVENTID eventID_;
  std::string         eventText_;
  logLevel            severity_;
  std::string         filename_;
  int32_t             lineNumber_;
  std::string         callerName_;

  public:

  AuthEvent ()
  : eventID_ (DBS_GENERIC_ERROR),
    severity_ (LL_INFO),
    lineNumber_ (0),
    callerName_ ("??")
  {}

  AuthEvent (
    DB_SECURITY_EVENTID eventID,
    std::string         eventText,
    logLevel            severity)
  : eventID_ (eventID),
    eventText_ (eventText),
    severity_ (severity),
    lineNumber_(0),
    callerName_ ("??")
  {}
  
  DB_SECURITY_EVENTID getEventID () { return eventID_; }
  logLevel getSeverity() { return severity_; }
  int32_t getLineNum() { return lineNumber_; }
  std::string getEventText() { return eventText_; }
  std::string getFilename() { return filename_; }
  std::string getCallerName() { return callerName_; }

  void setCallerName (std::string callerName) { callerName_ = callerName; }
  void setLineNumber(int32_t lineNumber) { lineNumber_ = lineNumber; }
  void setFilename(std::string filename) { filename_ = filename; }

  static std::string formatEventText( const char * eventText );

  void logAuthEvent ();

};

void authInitEventLog();
void insertAuthEvent(
  std::vector<AuthEvent> & authEvents,
  DB_SECURITY_EVENTID eventID,
  const char * eventText,
  logLevel severity);

#endif 
