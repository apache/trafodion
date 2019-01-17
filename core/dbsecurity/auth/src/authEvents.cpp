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

#include "authEvents.h"
#include <sys/time.h>
#include <str.h>
#include <unistd.h> 
#include <stdio.h>
#include <iostream>

class AuthEvent;

static std::string AUTH_COMPONENT = "DBSECURITY";


// ****************************************************************************
// function: insertAuthEvent
//
// This function inserts an AuthEvent into the current list of authEvents
//
// DB_SECUITY_EVENTID - is the event ID to insert 
//    see: $TRAF_HOME/export/include/common/evl_sqlog_eventnum.h
// eventText - text to insert
// severity - severity of the event
// ****************************************************************************
void insertAuthEvent(
  std::vector<AuthEvent> & authEvents,
  DB_SECURITY_EVENTID eventID,
  const char * eventText,
  logLevel severity)
{
  AuthEvent authEvent(eventID,
                      eventText,
                      severity);
  authEvents.push_back(authEvent);
}

// ****************************************************************************
// function authInitEventLog()
//
// This function create a new log in $TRAF_LOG directory with the 
// following name dbsecurity_<host>_<pid>.log
//
// It is called for standalone executables (e.g. ldapcheck) to log issues
// When users are authenticated during client authentication (mxosrvr), it
//   is assumed that the event log has already been initialized.
//
// ****************************************************************************
void authInitEventLog()
{
  // Log4cxx logging
  int my_nid = 1;

  // get my pid
  my_nid = getpid();
  char my_hostname[HOST_NAME_MAX+1];
  Int32  result;

  // who am I?
  if (gethostname(my_hostname,HOST_NAME_MAX) != 0)
    strcpy(my_hostname, "unknown");

  int log_name_suffix_len = strlen(my_hostname) + 32;
  char log_name_suffix[log_name_suffix_len];
  snprintf( log_name_suffix, log_name_suffix_len, "_%s_%d.log", my_hostname, my_nid );
  CommonLogger::instance().initLog4cxx("log4cxx.trafodion.auth.config",log_name_suffix);
}

// ****************************************************************************
//  function getAuthOutcome
//
//  Returns a text representation of the error from the AUTH_OUTCOME enum
//
// ****************************************************************************
std::string getAuthOutcome(AUTH_OUTCOME outcome)
{
   std::string outcomeDesc;
   switch (outcome)
   {
      case AUTH_OK:
         outcomeDesc = "Authentication successful";
         break;
      case AUTH_NOT_REGISTERED:
         outcomeDesc = "User not registered";
         break;
      case AUTH_MD_NOT_AVAILABLE:
         outcomeDesc = "Unexpected error occurred looking up user in database";
         break;
      case AUTH_USER_INVALID:
         outcomeDesc = "User is not valid";
         break;
      case AUTH_TYPE_INCORRECT:
         outcomeDesc = "Unexpected authorization type detected";
         break;
      case AUTH_NO_PASSWORD:
         outcomeDesc = "Invalid password";
         break;
      case AUTH_REJECTED:
         outcomeDesc = "Invalid username or password";
         break;
      case AUTH_FAILED:
         outcomeDesc = "Unexpected error returned from LDAP";
         break;
      default:
         outcomeDesc = "Unexpected error occurred";
    }
  return outcomeDesc;
}

// ****************************************************************************
// method: AuthEvent::formatEventText
//
// This method formats event text into a message that can be added to the 
//   cxx log.
//
//  eventText - the text to be logged
// ****************************************************************************
std::string AuthEvent::formatEventText( const char * eventText )
{
  // Format the timestamp
   char tbuff[24] = {0};
   struct tm *stm;

   time_t now = time(0);
   stm = gmtime(&now);
   strftime(tbuff, sizeof(tbuff), "%Y-%m-%d %H:%M:%S %Z", stm);

   // Format the event message, with the timestamp at the beginning.
   char eventMessage[MAX_EVENT_MSG_SIZE];
   snprintf(eventMessage, MAX_EVENT_MSG_SIZE, "%s (pid=%d) %s", tbuff, getpid(), eventText);

   std::string newMessage(eventMessage);
   return newMessage;
}

// ****************************************************************************
// method: AuthEvent::logAuthEvent
//
// This method writes the AuthEvent to the cxx log
//
// ****************************************************************************
void AuthEvent::logAuthEvent()
{
    int my_nid = 0; //gethostid()? 
    int my_pid = getpid();
    int my_cpu = 0;
  
    // Log4cxx logging
    char buf[MAX_EVENT_MSG_SIZE];
    snprintf(buf, MAX_EVENT_MSG_SIZE, "Node Number: %u, CPU: %u, PIN: %u ,,,, %s", 
            my_nid, my_cpu, my_pid, eventText_.c_str());
    
    // strip off final new line before logging
    int32_t lastPos = strlen(buf) -1;
    if (buf [lastPos] == '\n')
      buf[lastPos] = '.';
    CommonLogger::log(AUTH_COMPONENT, severity_, buf);
} 
