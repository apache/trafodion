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
//******************************************************************************
//                                                                             *
//     This program is used to test authenticating with an LDAP server         *
//  configured in a Trafodion instance. It may also be used to test the        *
//  correctness of the code that communicates with the LDAP server.            *
//                                                                             *
//   Trafodion does not to be started to run this program.                     *
//                                                                             *
//******************************************************************************
#include "ldapconfignode.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <termios.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <vector>
#include "common/evl_sqlog_eventnum.h"

using namespace std;

struct AuthEvents
{
DB_SECURITY_EVENTID eventID;
std::string         eventText;
std::string         filename;
int32_t             lineNumber;
};

std::vector<AuthEvents> authEvents;

enum Operation {
   Authenticate = 2,
   Lookup = 3
};

enum  LDAPAuthResult {
    AuthResult_Successful,       // User was authenticated on LDAP server
    AuthResult_Rejected,         // User was rejected by LDAP server
    AuthResult_ResourceError};   // An error prevented authentication
    
LDAPAuthResult authenticateLDAPUser(
   const char *                   username,
   const char *                   password,
   LDAPConfigNode::LDAPConfigType configType,
   char *                         searchHostName,
   char *                         authHostName);
   
void doAuthenticate(
   const char *                   username,
   const char *                   password,
   LDAPConfigNode::LDAPConfigType configType,
   bool                           verbose);
   
void doCanaryCheck(
   const char *                   username,
   LDAPConfigNode::LDAPConfigType configType,
   bool                           verbose,
   int &                          exitCode);   
   
void logAuthEvent(
   DB_SECURITY_EVENTID eventID,
   const char *        msg,
   const std::string & filename, 
   int32_t             lineNumber);
   
LDSearchStatus lookupLDAPUser(
   const char *                    username,          
   LDAPConfigNode::LDAPConfigType  configType,        
   char *                          searchHostName);
   
void printTime();        
      
void reportAuthenticationErrors();

void reportRetries(Operation operation);
    
void setEcho(bool enable);

// *****************************************************************************
// *                                                                           *
// * Function: authenticateLDAPUser                                            *
// *                                                                           *
// *    Authenticate an LDAP user with either the primary or secondary LDAP    *
// * configuration.                                                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <username>                      const char *                    In       *
// *    is the external username to be checked on the directory server.        *
// *                                                                           *
// *  <password>                      const char *                    In       *
// *    is the password of the external user                                   *
// *                                                                           *
// *  <configType>                    LDAPConfigNode::LDAPConfigType  In       *
// *    is the type of configuration to use.                                   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: LDAPAuthResult                                                   *
// *                                                                           *
// *    AuthResult_Successful,       -- User was authenticated on LDAP server  *
// *    AuthResult_Rejected,         -- User was rejected by LDAP server       *
// *    AuthResult_ResourceError     -- An error prevented authentication      *
// *                                                                           *
// *****************************************************************************
LDAPAuthResult authenticateLDAPUser(
   const char *                   username,
   const char *                   password,
   LDAPConfigNode::LDAPConfigType configType,
   char *                         searchHostName,
   char *                         authHostName)

{

string userDN = "";       // User DN, used to bind to LDAP serer
LDAPConfigNode *searchNode;

// Zero len password is a non-auth bind in LDAP, so we treat it
// as a failed authorization.
   if (strlen(password) == 0)
      return AuthResult_Rejected;

   searchNode = LDAPConfigNode::GetLDAPConnection(configType,SearchConnection,
                                                  searchHostName);

   if (searchNode == NULL) 
      return AuthResult_ResourceError;

LDSearchStatus searchStatus = searchNode->lookupUser(username,userDN);
                                                     
   if (searchStatus == LDSearchNotFound)
      return AuthResult_Rejected;
      
   if (searchStatus != LDSearchFound)
      return AuthResult_ResourceError;

// User is defined there.  But is their password correct?

LDAPConfigNode *authNode = LDAPConfigNode::GetLDAPConnection(configType,
                                                             AuthenticationConnection,
                                                             authHostName);

   if (authNode == NULL)
      return AuthResult_ResourceError;

//
// User exists, let's validate that non-blank password!
//

LDAuthStatus authStatus = authNode->authenticateUser(userDN.c_str(),password);

   if (authStatus == LDAuthSuccessful)
      return AuthResult_Successful;
      
   if (authStatus == LDAuthRejected)
      return AuthResult_Rejected;

   return AuthResult_ResourceError;

}
//************************ End of authenticateLDAPUser *************************

// *****************************************************************************
// *                                                                           *
// * Function: doAuthenticate                                                  *
// *                                                                           *
// *    Authenticate an LDAP user with either the primary or secondary LDAP    *
// * configuration.                                                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <username>                      const char *                    In       *
// *    is the external username to be checked on the directory server.        *
// *                                                                           *
// *  <password>                      const char *                    In       *
// *    is the password of the external user                                   *
// *                                                                           *
// *  <configType>                    LDAPConfigNode::LDAPConfigType  In       *
// *    is the type of configuration to use.                                   *
// *                                                                           *
// *  <verbose>                       bool                            In       *
// *    if true, additional output is produced.                                *
// *                                                                           *
// *****************************************************************************
void doAuthenticate(
   const char *                   username,
   const char *                   password,
   LDAPConfigNode::LDAPConfigType configType,
   bool                           verbose)

{

   LDAPConfigNode::ClearRetryCounts();
   
char searchHostName[256];
char authHostName[256];

   searchHostName[0] = authHostName[0] = 0;
 
LDAPAuthResult rc = authenticateLDAPUser(username,password,configType,
                                         searchHostName,authHostName);
                             
   if (verbose)
   {
      cout << "Search host name: " << searchHostName << endl;
      cout << "Auth host name: " << authHostName << endl;
   }
       
//
// How'd it go?  In addition to a thumbs up/down, print diagnostics if 
// requested.  This matches production behavior; we log retries if
// successful, and we log all internal errors as well as number of retries
// if authentication failed due to a resource error.  Resource errors include
// problems with the LDAP servers and parsing the LDAP connection configuration
// file (.traf_authentication_config).
//      
   if (rc == AuthResult_Rejected)
   {                                          
      cout << "Invalid username or password" << endl; 
      return;
   }
   
   if (rc == AuthResult_Successful)
      cout << "Authentication successful" << endl;
   else
      cout << "Authentication failed: resource error" << endl;

   if (verbose)
   {
      reportRetries(Authenticate);
      reportAuthenticationErrors(); 
   }

}
//*************************** End of doAuthenticate ****************************

// *****************************************************************************
// *                                                                           *
// * Function: doCanaryCheck                                                   *
// *                                                                           *
// *    Lookup a user on an LDAP server to test configuration and connection.  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <username>                      const char *                    In       *
// *    is the external username to be checked on the directory server.        *
// *                                                                           *
// *  <configType>                    LDAPConfigNode::LDAPConfigType  In       *
// *    is the type of configuration to use.                                   *
// *                                                                           *
// *  <verbose>                       bool                            In       *
// *    if true, additional output is produced.                                *
// *                                                                           *
// *****************************************************************************
void doCanaryCheck(
   const char *                   username,
   LDAPConfigNode::LDAPConfigType configType,
   bool                           verbose,
   int &                          exitCode)   
   
{

   exitCode = 0;
   
char searchHostName[256];

   searchHostName[0] = 0;
   
LDSearchStatus searchStatus = lookupLDAPUser(username,configType,searchHostName);

   if (verbose)
      cout << "Search host name: " << searchHostName << endl;
   
   if (authEvents.size() > 0)
      exitCode = 1;

   switch (searchStatus)
   {
      case LDSearchFound:
         cout << "User " << username << " found" << endl;
         break;
      case LDSearchNotFound:
         cout << "User " << username << " not found" << endl;
         break;
      case LDSearchResourceFailure:
         cout << "Unable to lookup user due to LDAP errors" << endl;
         exitCode = 2;
         break;
      default:
      {
         cout << "Internal error" << endl;
         exitCode = 2;
      }
   }
   
   if (verbose)
   {
      reportRetries(Lookup);
      reportAuthenticationErrors(); 
   }

}
//*************************** End of doCanaryCheck *****************************


// *****************************************************************************
// *                                                                           *
// * Function: logAuthEvent                                                    *
// *                                                                           *
// *    Stores an authentication event (mostly resource errors) in a vector    *
// *  for later retrieval.                                                     *
// *                                                                           *
// *    The function signature must exactly match the same function in         *
// *  ld_port.cpp.                                                             *
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
           tbuff,getpid(),msg); 

AuthEvents authEvent;

   authEvent.eventID = eventID;
   authEvent.eventText = eventMessage;
   authEvent.filename = filename;
   authEvent.lineNumber = lineNumber;
   authEvents.push_back(authEvent);       

} 
//**************************** End of logAuthEvent *****************************




// *****************************************************************************
// *                                                                           *
// * Function: lookupLDAPUser                                                  *
// *                                                                           *
// *    Determines if the username is defined on an LDAP server in the         *
// *  specified configuration.                                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <username>                      const char *                    In       *
// *    is the external username to be checked on the directory server.        *
// *                                                                           *
// *  <configType>                    LDAPConfigType                  In       *
// *    specifies which configuration (i.e.,which set of LDAP servers) to use  *
// *  when searching for the username.                                         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: LDSearchStatus                                                   *
// *                                                                           *
// *   LDSearchFound            -- User was found on LDAP server               *
// *   LDSearchNotFound         -- User was not found on LDAP server           *
// *   LDSearchResourceFailure  -- An error prevented checking                 *
// *                                                                           *
// *****************************************************************************
LDSearchStatus lookupLDAPUser(
   const char *                    username,          
   LDAPConfigNode::LDAPConfigType  configType,        
   char *                          searchHostName)

{
  
LDAPConfigNode *searchNode;

   searchNode = LDAPConfigNode::GetLDAPConnection(configType,SearchConnection,
                                                  searchHostName);

   if (searchNode == NULL) 
      return LDSearchResourceFailure;
      
string userDN = "";

   return searchNode->lookupUser(username,userDN);

}
//************************** End of lookupLDAPUser *****************************

// *****************************************************************************
// *                                                                           *
// * Function: printTime                                                       *
// *                                                                           *
// *    Displays the current time in format YYYY-MM-DD hh:mm:ss.               *
// *                                                                           *
// *****************************************************************************
void printTime()

{

time_t t = time(NULL);
struct tm tm = *localtime(&t);

   printf("%4d-%02d-%02d %02d:%02d:%02d\n",tm.tm_year + 1900,tm.tm_mon + 1, 
          tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);

}
//***************************** End of printTime *******************************


// *****************************************************************************
// *                                                                           *
// * Function: printUsage                                                      *
// *                                                                           *
// *    Displays usage information for this program.                           *
// *                                                                           *
// *****************************************************************************
void printUsage()

{

   cout << "Usage:ldapcheck [option]..." << endl;
   cout << "option ::= --help|-h            display usage information" << endl;
   cout << "           --username=<LDAP-username>" << endl;
   cout << "           --password[=<password>]" << endl;
   cout << "           --platform           Use internal cluster configuration" << endl;
   cout << "           --primary            Use primary (ENTERPRISE|LOCAL) configuration" << endl;
   cout << "           --secondary          Use secondary (CLUSTER|REMOTE) configuration" << endl;
   cout << "           --verbose            Display non-zero retry counts and LDAP errors" << endl;

}
//***************************** End of printUsage ******************************



// *****************************************************************************
// *                                                                           *
// * Function: reportAuthenticationErrors                                      *
// *                                                                           *
// *    Displays any resource errors encountered during authentication.        *
// *                                                                           *
// *****************************************************************************
void reportAuthenticationErrors()

{

size_t errorCount = authEvents.size();

// *****************************************************************************
// *                                                                           *
// *    Walk the list of errors encountered during the attempt to authenticate *
// * the username and password, and for each error, display the event ID and   *
// * text along with the file and line number where the error was detected.    *
// *                                                                           *
// * If there is more than one error, number the errors.                       *
// *                                                                           *
// *****************************************************************************

   for (size_t index = 0; index < errorCount; index++)
   {
      if (errorCount > 1)
         cout << "Error #" << index + 1 << endl;
   
      const AuthEvents &authEvent = authEvents[index];
      
      cout << "Filename: " << authEvent.filename << 
              " Line number: " << authEvent.lineNumber << endl;  
      cout << "Event ID: " << authEvent.eventID << endl;
      cout << authEvent.eventText << endl;
   }

}
//********************** End of reportAuthenticationErrors *********************


// *****************************************************************************
// *                                                                           *
// * Function: reportRetries                                                   *
// *                                                                           *
// *    Displays any retries that occurred during an authentication or         *
// * lookup operation.                                                         *
// *                                                                           *
// *****************************************************************************
void reportRetries(Operation operation)

{

size_t bindRetryCount = LDAPConfigNode::GetBindRetryCount();
size_t searchRetryCount = LDAPConfigNode::GetSearchRetryCount();

// If there were no retries, there is nothing to report.  
   if (bindRetryCount == 0 && searchRetryCount == 0)
      return;

char operationString[20];
   
   if (operation == Authenticate)
      strcpy(operationString,"Authentication");
   else
      strcpy(operationString,"Lookup"); 
      
// Log if the search (name lookup) operation had to be retried.          
   if (searchRetryCount > 0)
   { 
      char searchRetryMessage[5100];
       
      sprintf(searchRetryMessage,"%s required %d search retries. ",
              operationString,searchRetryCount);
      cout << searchRetryMessage << endl; 
   }
   
// Log if the bind (password authentication) operation had to be retried.          
   if (bindRetryCount > 0)
   { 
      char bindRetryMessage[5100];
       
      sprintf(bindRetryMessage,"%s required %d bind retries. ",
              operationString,bindRetryCount);
      cout << bindRetryMessage << endl; 
   }

}
//*************************** End of reportRetries *****************************



// *****************************************************************************
// *                                                                           *
// * Function: setEcho                                                         *
// *                                                                           *
// *    Enables or disables echoing of input characters.                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <enable>                        bool                            In       *
// *    is true if echo should be enabled, false if it should be disabled.     *
// *                                                                           *
// *****************************************************************************
void setEcho(bool enable)

{

termios tty;

   tcgetattr(STDIN_FILENO,&tty);
   
   if (enable)
      tty.c_lflag |= ECHO;
   else
      tty.c_lflag &= ~ECHO;

   tcsetattr(STDIN_FILENO,TCSANOW,&tty);

}
//****************************** End of setEcho ********************************


#pragma page "main"
// *****************************************************************************
// *                                                                           *
// * Function: main                                                            *
// *                                                                           *
// *    Parses the arguments and attempt to authentication the username and    *
// * password.                                                                 *
// *                                                                           *
// * Trafodion does not to be started to run this program.                     *
// *                                                                           *
// *****************************************************************************
int main(int argc,char *argv[])

{

//
// ldapcheck needs a username.  If not supplied, issue an error
// and print usage information. 
//

   if (argc <= 1)
   {
      cout << "Username required to check LDAP" << endl;
      printUsage();
      exit(1);
   }
   
//
// Help!
//
   
   if (strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0)
   {
      printUsage();
      exit(0);
   } 
   
enum Options {
   Primary = 1,
   Secondary = 0,
   Platform = 2};
   
int c;
int optionFlag = Primary;
int verbose = 0;
string password;
char username[129];
bool usernameSpecified = false;
bool passwordSpecified = false;
int loopCount = 1;
int delayTime = 0;
bool looping = false;

//
// Walk the list of options.  Username and password are required, although
// the password can be left blank and prompted for.
//
     
   while (true)
   {
      static struct option long_options[] =
      {
         {"one",       no_argument,&optionFlag,Primary},
         {"two",       no_argument,&optionFlag,Secondary},
         {"primary",   no_argument,&optionFlag,Primary},
         {"secondary", no_argument,&optionFlag,Secondary},
         {"enterprise",no_argument,&optionFlag,Primary},
         {"cluster",   no_argument,&optionFlag,Secondary},
         {"local",     no_argument,&optionFlag,Primary},
         {"remote",    no_argument,&optionFlag,Secondary},
         {"platform",  no_argument,&optionFlag,Platform},
         {"verbose",   no_argument,&verbose,1},
         {"loop",      optional_argument,0,'l'},
         {"delay",     optional_argument,0,'d'},
         {"password",  optional_argument,0,'p'},
         {"username",  required_argument,0,'u'},
         {0, 0, 0, 0}
      };
      int option_index = 0;
 
      c = getopt_long(argc,argv,"u:p::",long_options,&option_index);
 
      // No more options, break out of loop
      if (c == -1)
         break;
                 
      switch (c)
      {
         case 0:
            break;
 
         case 'u':
            if (usernameSpecified)
            {
               cout << "Username already specified" << endl;
               exit(1);
            }
            if (strlen(optarg) >= sizeof(username))
            {
               cout << "Username limited to 128 characters" << endl;
               exit(1);
            }
            strcpy(username,optarg);
            usernameSpecified = true;
            break;
 
         case 'p':
            if (passwordSpecified)
            {
               cout << "Password already specified" << endl;
               exit(1);
            }
            if (optarg)
            {
               if (strlen(optarg) > 64)
               {
                  cout << "Password limited to 64 characters" << endl;
                  exit(1);
               }
               password = optarg;
            }
            else
               do
               {
                  cout << "Password: ";
                  setEcho(false);
                  getline(cin,password);
                  setEcho(true);
                  cout << endl;
                  if (password.size() > 64)
                     cout << "Password limited to 64 characters" << endl;
                }
                while (password.size() > 64);
            passwordSpecified = true;
            break;
         case 'l':
            loopCount = atoi(optarg);
            if (loopCount <= 0)
            {
               cout << "Loop count must be at least 1" << endl;
               exit(1);
            }
            looping = true;
         case 'd':
            delayTime = atoi(optarg);
            if (delayTime <= 0)
            {
               cout << "Delay time must be at least 1" << endl;
               exit(1);
            }
         case '?':
            // getopt_long already printed an error message.
            break;
 
         default:
            cout << "Internal error,option " << c << endl;
            exit(1);
      }
   }
 
// If there are any remaining command line arguments, report an error
   if (optind < argc)
   {
      cout << "Unrecognized text" << endl; 
      while (optind < argc)
          cout << argv[optind++] << " ";
      cout << endl;
      printUsage();
      exit(1);
   }
   
// Verify a username was supplied, or we have nothing to do
   if (!usernameSpecified)
   {
      cout << "Username required" << endl;
      printUsage();
      exit(1);   
   }
   
LDAPConfigNode::LDAPConfigType configType = LDAPConfigNode::PrimaryConfiguration;

   switch (optionFlag)
   {
      case Secondary:
         configType = LDAPConfigNode::SecondaryConfiguration;
         break;
      case Primary:
         configType = LDAPConfigNode::PrimaryConfiguration;
         break;
      case Platform:
         configType = LDAPConfigNode::SecondaryInternalConfiguration;
         break;
      default: //Should not happen, but just in case
         configType = LDAPConfigNode::PrimaryConfiguration;
   }    

// If no password is supplied, we just perform a name lookup.  This was 
// added to provide a canary check for the LDAP server without having to 
// supply a valid password.   
   if (!passwordSpecified)
   {
      int exitCode = 0;
      while (loopCount--)
      {
         if (verbose && looping)
            printTime();
         doCanaryCheck(username,configType,verbose,exitCode);
         if (loopCount > 0)
            sleep(delayTime);
      }
      //
      // For the LDAP Canary check mode of ldapcheck, we return one of 
      // three exit codes to be used by a health check:
      //
      // 0) LDAP configuration and server(s) good, no retries 
      // 1) LDAP configuration and server(s) good, retries occurred 
      // 2) Could not communicate with LDAP server(s).  Check LDAP configuration or server(s).
      //
      exit(exitCode);
   }
            
//
// We have a username and password.  Let's authenticate!
//
   while (loopCount--)
   {
      if (verbose && looping)
         printTime();
      doAuthenticate(username,password.c_str(),configType,verbose);
      if (loopCount > 0)
         sleep(delayTime);
   }
   exit(0);
   
}
//******************************** End of main *********************************
