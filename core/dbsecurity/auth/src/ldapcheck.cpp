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
#include "authEvents.h"
#include "auth.h"

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


using namespace std;

enum Operation {
   Authenticate = 2,
   Lookup = 3
};

AUTH_OUTCOME authenticateLDAPUser(
   std::vector<AuthEvent> &       authEvents,
   const char *                   username,
   const char *                   password,
   LDAPConfigNode::LDAPConfigType configType,
   char *                         searchHostName,
   char *                         authHostName);
   
void doAuthenticate(
   std::vector<AuthEvent> &       authEvents,
   const char *                   username,
   const char *                   password,
   LDAPConfigNode::LDAPConfigType configType,
   bool                           verbose);
   
void doCanaryCheck(
   std::vector<AuthEvent> &       authEvents,
   const char *                   username,
   LDAPConfigNode::LDAPConfigType configType,
   bool                           verbose,
   AUTH_OUTCOME                   exitCode);   
   
LDSearchStatus lookupLDAPUser(
   std::vector<AuthEvent> &        authEvents,
   const char *                    username,          
   LDAPConfigNode::LDAPConfigType  configType,        
   char *                          searchHostName);
   
void printTime();        
      
void reportResults(
    Operation                      operation, 
    const std::vector<AuthEvent> & authEvents,
    const char                   * username,
    AUTH_OUTCOME                   outcome,
    int                            verbose);

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
// *  <authEvents>                    std::vector<AuthEvent> &        Out      *
// *    detailed event results of request                                      *
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
// * Returns: AUTH_OUTCOME                                                   *
// *                                                                           *
// *    AUTH_OK                      -- User was authenticated on LDAP server  *
// *    AUTH_REJECTED                -- User was rejected by LDAP server       *
// *    AUTH_FAILED                  -- An error prevented authentication      *
// *                                                                           *
// *****************************************************************************
AUTH_OUTCOME authenticateLDAPUser(
   std::vector<AuthEvent> &       authEvents,
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
      return AUTH_REJECTED;

   searchNode = LDAPConfigNode::GetLDAPConnection(authEvents,configType,SearchConnection,
                                                  searchHostName);

   if (searchNode == NULL) 
      return AUTH_FAILED;

   LDSearchStatus searchStatus = searchNode->lookupUser(authEvents,username,userDN);
                                                     
   if (searchStatus == LDSearchNotFound)
      return AUTH_REJECTED;
      
   if (searchStatus != LDSearchFound)
      return AUTH_FAILED;

   // User is defined there.  But is their password correct?

   LDAPConfigNode *authNode = LDAPConfigNode::GetLDAPConnection(authEvents,configType,
                                                                AuthenticationConnection,
                                                                authHostName);

   if (authNode == NULL)
      return AUTH_FAILED;

   // User exists, let's validate that non-blank password!

   LDAuthStatus authStatus = authNode->authenticateUser(authEvents,userDN.c_str(),password);

   if (authStatus == LDAuthSuccessful)
      return AUTH_OK;
      
   if (authStatus == LDAuthRejected)
      return AUTH_REJECTED;

   return AUTH_FAILED;

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
// *  <authEvents>                    std::vector<AuthEvent> &        Out      *
// *    detailed event results of request                                      *
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
   std::vector<AuthEvent> &       authEvents,
   const char *                   username,
   const char *                   password,
   LDAPConfigNode::LDAPConfigType configType,
   bool                           verbose)

{

   LDAPConfigNode::ClearRetryCounts();
   
   char searchHostName[256];
   char authHostName[256];

   searchHostName[0] = authHostName[0] = 0;
 
   AUTH_OUTCOME rc = authenticateLDAPUser(authEvents,username,password,configType,
                                          searchHostName,authHostName);
                             
   if (verbose)
   {
      cout << "Search host name: " << searchHostName << endl;
      cout << "Auth host name: " << authHostName << endl;
   }
       
   // How'd it go?  In addition to a thumbs up/down, print diagnostics if 
   // requested.  This matches production behavior; we log retries if
   // successful, and we log all internal errors as well as number of retries
   // if authentication failed due to a resource error.  Resource errors include
   // problems with the LDAP servers and parsing the LDAP connection configuration
   // file (.traf_authentication_config).
           
   reportResults(Authenticate,authEvents,username,rc,verbose);

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
// *  <authEvents>                    std::vector<AuthEvent> &        Out      *
// *    detailed event results of request                                      *
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
   std::vector<AuthEvent> &       authEvents,
   const char *                   username,
   LDAPConfigNode::LDAPConfigType configType,
   bool                           verbose,
   AUTH_OUTCOME                   exitCode)   
   
{
   exitCode = AUTH_OK;

   char searchHostName[256];

   searchHostName[0] = 0;
   
   LDSearchStatus searchStatus = lookupLDAPUser(authEvents,username,configType,searchHostName);

   if (verbose)
      cout << "Search host name: " << searchHostName << endl;
   
   switch (searchStatus)
   {
      case LDSearchFound:
         exitCode = AUTH_OK;
         break;
      case LDSearchNotFound:
         exitCode = AUTH_REJECTED;
         break;
      case LDSearchResourceFailure:
         exitCode = AUTH_FAILED;
         break;
      default:
         exitCode = AUTH_FAILED;
   }
   

   reportResults(Lookup,authEvents,username,exitCode,verbose);

}
//*************************** End of doCanaryCheck *****************************


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
// *  <authEvents>                    std::vector<AuthEvent> &        Out      *
// *    detailed event results of request                                      *
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
   std::vector<AuthEvent> &        authEvents,
   const char *                    username,          
   LDAPConfigNode::LDAPConfigType  configType,        
   char *                          searchHostName)

{
  
LDAPConfigNode *searchNode;

   searchNode = LDAPConfigNode::GetLDAPConnection(authEvents,configType,SearchConnection,
                                                  searchHostName);

   if (searchNode == NULL) 
      return LDSearchResourceFailure;
      
string userDN = "";

   return searchNode->lookupUser(authEvents,username,userDN);

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
// * Function: reportResults                                                   *
// *                                                                           *
// * Logs and optionally displays any retries, errors, and outcome of an       *
// * authentication or lookup operation.                                       *
// *                                                                           *
// *****************************************************************************
void reportResults(
    Operation                      operation,
    const std::vector<AuthEvent> & authEvents,
    const char                   * username,
    AUTH_OUTCOME                   outcome,
    int                            verbose)

{

   std::string msg;

   // Report any retries
   size_t bindRetryCount = LDAPConfigNode::GetBindRetryCount();
   size_t searchRetryCount = LDAPConfigNode::GetSearchRetryCount();

   char operationString[20];
   
   if (operation == Authenticate)
      strcpy(operationString,"Authentication");
   else
      strcpy(operationString,"Lookup"); 
      

   // Log and optionally display event for:
   //    search (name lookup) operation that was retried
   if (searchRetryCount > 0)
   { 
      char searchRetryMessage[5100];
       
      sprintf(searchRetryMessage,"%s required %d search retries. ",
              operationString,searchRetryCount);
      msg = searchRetryMessage;
      AuthEvent authEvent (DBS_AUTH_RETRY_SEARCH, msg, LL_INFO);
      authEvent.setCallerName("ldapcheck");
      authEvent.logAuthEvent();
      if (verbose)
         cout << searchRetryMessage << endl; 
   }
   
   // Log and optionally display event for:
   //   any bind (password authentication) operation that was retried
   if (bindRetryCount > 0)
   { 
      char bindRetryMessage[5100];
       
      sprintf(bindRetryMessage,"%s required %d bind retries. ",
              operationString,bindRetryCount);
      msg = bindRetryMessage;
      AuthEvent authEvent (DBS_AUTH_RETRY_BIND, msg, LL_INFO);
      authEvent.setCallerName("ldapcheck");
      authEvent.logAuthEvent();
      if (verbose)
         cout << bindRetryMessage << endl; 
   }

  // report any errors and retries

  //  Walk the list of errors encountered during the attempt to authenticate
  //  the username, or username and password, and for each error, log the event 
  //  ID and text. If verbose is true and logLevel is above the error 
  //  level, send message to standard out.
  std::string callerName ("ldapcheck");

  for (size_t i = 0; i < authEvents.size(); i++)
  {
     AuthEvent authEvent = authEvents[i];
     authEvent.setCallerName(callerName);
     authEvent.logAuthEvent();
     if (verbose)
       cout  << "ERROR: " << authEvent.getEventText().c_str() << endl;
  }

  // Finally, report the outcome.
   std::string outcomeDesc = getAuthOutcome(outcome);
   char buf[MAX_EVENT_MSG_SIZE];
   snprintf(buf, MAX_EVENT_MSG_SIZE,
                "Authentication request: externalUser %s, "
                "result %d (%s)",
                username,
                (int)outcome, outcomeDesc.c_str());
   msg = buf;
   AuthEvent authEvent (DBS_AUTHENTICATION_ATTEMPT,msg, LL_INFO);
   authEvent.setCallerName("ldapcheck");
   authEvent.logAuthEvent();
   cout  << "INFO: " << authEvent.getEventText().c_str() << endl;

}
//*************************** End of reportResults *****************************



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
   // ldapcheck needs a username.  If not supplied, issue an error
   // and print usage information. 
   if (argc <= 1)
   {
      cout << "Username required to check LDAP" << endl;
      printUsage();
      exit(1);
   }
   
   // Help!
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

   // Walk the list of options.  Username and password are required, although
   // the password can be left blank and prompted for.
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
         break;
         configType = LDAPConfigNode::PrimaryConfiguration;
      case Platform:
         break;
         configType = LDAPConfigNode::SecondaryInternalConfiguration;
         break;
      default: //Should not happen, but just in case
         configType = LDAPConfigNode::PrimaryConfiguration;
   }    


   // allocate authEvents to store any issues
   std::vector<AuthEvent>  authEvents;
   authInitEventLog();

   // If no password is supplied, we just perform a name lookup.  This was 
   // added to provide a canary check for the LDAP server without having to 
   // supply a valid password.   
   if (!passwordSpecified)
   {
      AUTH_OUTCOME exitCode = AUTH_OK;
      while (loopCount--)
      {
         if (verbose && looping)
            printTime();
         doCanaryCheck(authEvents,username,configType,verbose,exitCode);
         if (loopCount > 0)
            sleep(delayTime);
      }

      // For the LDAP Canary check mode of ldapcheck, we return one of 
      // three exit codes (AUTH_OUTCOME)
      //
      // AUTH_OK:       LDAP configuration and server(s) good
      // AUTH_REJECTED: User was not defined in LDAP
      // AUTH_FAILED:   Could not communicate with LDAP server(s)  
      //    (check LDAP configuration or server(s))
      exit((int)exitCode);
   }
            
   // We have a username and password.  Let's authenticate!
   while (loopCount--)
   {
      if (verbose && looping)
         printTime();
      doAuthenticate(authEvents,username,password.c_str(),configType,verbose);
      if (loopCount > 0)
         sleep(delayTime);
   }
   exit(0);
   
}
//******************************** End of main *********************************
