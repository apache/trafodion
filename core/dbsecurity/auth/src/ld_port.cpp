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
#include <stdio.h>  
#include <string.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <vector>
#include "ld_globals.h"

// LCOV_EXCL_START
  

// ACH:    
//   This function prints to stdout when there is a problem in 
//   handling the connection to the LDAP server for user authorization
//   or to verify a user exists on the LDAP server.    This may be
//   due to network problems or a misconfiguration in the .sqldapconfig
//   file.    When using the internal development tool sqlci, this output
//   will go to the user's screen.   For customer tools (HPDM or HPDCI) 
//   stdout gets redirected to a file, so this will be saved in the
//   log file on the node.    This can be very handy for diagnosing 
//   connection & configuration problems.
//
//   A suggestion has also been made that the CLI code which ends up calling
//   the user authentication code as well as DDL code like Register User could
//   check first that the LDAP configuration needed is at least present.  This
//   would allow for better error reporting, for example, in cases where REMOTE
//   AUTHENTICATION is specified but the remote connection is not configured in
//   the .sqldapconfig file.
//   
//

std::vector<AuthEvents> authEvents;

// *****************************************************************************
// *                                                                           *
// * Function: clearAuthEvents                                                 *
// *                                                                           *
// *    Clears all entries from the authentication events vector.              *
// *                                                                           *
// *****************************************************************************
void clearAuthEvents()

{

   authEvents.clear();
   
}
//************************* End of clearAuthEvents *****************************


// *****************************************************************************
// *                                                                           *
// * Function: getAuthEventCount                                               *
// *                                                                           *
// *    Returns the number of entries in the authentication events vector.     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:  size_t                                                         *
// *                                                                           *
// *****************************************************************************
size_t getAuthEventCount()

{

   return authEvents.size();
   
}
//************************ End of getAuthEventCount ****************************





// *****************************************************************************
// *                                                                           *
// * Function: getAuthEvent                                                    *
// *                                                                           *
// *    Returns the AuthEvent entry specified by index.                        *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameter:                                                               *
// *                                                                           *
// *  <index>                   size_t                                 In      *
// *    is an index into the AuthEvents vector.  The index is not validated;   *
// *  if the index is too large an error will result.                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:  const AuthEvents &                                             *
// *                                                                           *
// *****************************************************************************
const AuthEvents & getAuthEvent(size_t index)

{

   return authEvents[index];
   
}
//*************************** End of getAuthEvent ******************************




// *****************************************************************************
// *                                                                           *
// * Function: logAuthEvent                                                    *
// *                                                                           *
// *    Logs an authentication event (mostly resource errors) to standard out  *
// *  and stores the event data in a vector for later retrieval.  (To be       *
// *  written to the instance repository in non-Live Feed cases.)              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameter:                                                               *
// *                                                                           *
// *  <eventID>                 DB_SECURITY_EVENTID                    In      *
// *    is the event ID associated with the authentication event.              *
// *                                                                           *
// *  <msg>                     const char *                           In      *
// *    is the text associated with the authentication event.                  *
// *                                                                           *
// *  <filename>                const std::string &                    In      *
// *    is the filename where the error/event was detected.                    *
// *                                                                           *
// *  <lineNumber>              int32_t                                In      *
// *    is the line number where the error/event was detected.                 *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
void logAuthEvent(
   DB_SECURITY_EVENTID eventID,
   const char *        msg,
   const std::string & filename, 
   int32_t             lineNumber) 
    
{

// Format the timestamp

char tbuff[24] = {0};
struct tm *stm;

   time_t now = time(0);
   stm = gmtime(&now);
   strftime(tbuff, sizeof(tbuff), "%Y-%m-%d %H:%M:%S %Z", stm);
   
// Format the event message, with the timestamp at the beginning.
   
char eventMessage[5100];

   sprintf(eventMessage,
           "%s (pid=%d) Error detected while establishing LDAP connection: %s\n",
          tbuff, getpid(), msg); 

// Write the event message to the stdout file.          
   printf(eventMessage);
   
// We only store the first 5000 characters of the message.  The TEXT column
// is limited to 5000 characters in the EVENT_TEXT_TABLE table.
   if (strlen(eventMessage) > 5000)
      eventMessage[5000] = 0;
   
AuthEvents authEvent;

   authEvent.eventID = eventID;
   authEvent.eventText = eventMessage;
   authEvent.filename = filename;
   authEvent.lineNumber = lineNumber;
   authEvents.push_back(authEvent);       
           
} 
//*************************** End of logAuthEvent ******************************
// LCOV_EXCL_STOP  


