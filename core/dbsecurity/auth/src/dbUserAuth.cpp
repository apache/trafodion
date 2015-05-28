//******************************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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
//******************************************************************************

// LCOV_EXCL_START -- lets be a little paranoid and not let any in-line funcs
//                    from the header files slip into the coverage count

#include "dbUserAuth.h"
#include "token.h"
#include "tokenkey.h"
#include "ldapconfignode.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <string>
#include <vector>
#include <exception>
#include <ctype.h>

#include <iostream>
#include <sys/types.h>
#include <linux/unistd.h> // gettid
#include "../../../sqf/inc/cextdecs/cextdecs.h"

// Used to disable and enable dumps via setrlimit in a release env.
#ifndef DBSECURITY_DEBUG
#include <sys/resource.h>
#endif

#include "ld_globals.h"
#include "ldapconfignode.h"

#include "common/evl_sqlog_eventnum.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "seabed/int/types.h"
#include "seabed/pctl.h"
#include "seabed/ms.h"
#include "seabed/fserr.h"
class ComDiagsArea;
class Queue;
#include "sqlcli.h"
#ifndef Lng32
typedef int             Lng32;
#endif
#include "ComSmallDefs.h"
#include "ExExeUtilCli.h"

// LCOV_EXCL_STOP

#define ZFIL_ERR_BADPARMVALUE 590
#define ZFIL_ERR_BOUNDSERR 22
#define ZFIL_ERR_OK 0
#define ZFIL_ERR_SECVIOL 48
#ifndef SEABASE_AUTHS
#define SEABASE_AUTHS            "AUTHS"
#endif 

using namespace std;

class UserCacheContents;
class ClientContents;

static void authenticateUser(
   DBUserAuthContents  & self,
   const char          * username,
   const char          * password,
   ClientContents      & clientInfo,
   AUTHENTICATION_INFO & authenticationInfo,
   PERFORMANCE_INFO    & performanceInfo);
   
void cacheUserInfo(
   const char    * username,
   const char    * databaseUsername,
   int32_t         userID,
   bool            isValid,
   int64_t         redefTime);   
   
static LDAuthStatus executeLDAPAuthentication(
   DBUserAuthContents &            self,
   const char *                    username,
   const char *                    password,
   LDAPConfigNode::LDAPConfigType  configType,
   bool &                          errorsLogged,
   PERFORMANCE_INFO &              performanceInfo);

inline static const UserCacheContents * fetchFromCacheByUsername(const char * username);

inline static const UserCacheContents * fetchFromCacheByUserID(long userID);

static int32_t fetchFromAUTHSTable(
   const char     * externalUsername,
   char           * databaseUsername,
   int32_t        & userID,
   bool           & isValid,
   int64_t        & redefTime);

static int32_t fetchFromAUTHSTable(
   int32_t          userID,
   char           * databaseUsername,
   char           * externalUsername,
   bool           & isValid,
   int64_t        & redefTime);
 

static void logAuthenticationErrors(int nodeID);

static void logAuthenticationOutcome(
   const string   & external_user_name,
   const string   & internal_user_name,
   const int32_t    user_id,
   ClientContents & clientInfo,
   const string   & outcome);
                                    
static void logAuthenticationRetries(
   int          nodeID,
   const char * username);
 
static void logToEventTable(
   int                    nodeID,
   DB_SECURITY_EVENTID    eventID, 
   posix_sqlog_severity_t severity, 
   const char *           msg);                                    

static void prepareUserNameForQuery(
   char       *nameForQuery,
   const char *extName);

static void stripAllTrailingBlanks(
   char   * buf,
   size_t   len);

static void timeDiff (
   struct timespec t1,
   struct timespec t2,
   struct timespec &tDiff);
   
inline static void upshiftString(
   const char * inputString,
   char       * outputString); 
   
static void writeLog(const char * text);     

class ClientContents
{
public:
   char            clientName[257];
   char            clientUserName[257];
   char            applicationName[257];
};


class BlackBox
{
public:
   USERS_INFO      usersInfo;
   int             error;   //ACH do we need this?
   UA_Status       errorDetail;
   bool            isAuthenticated;
};

class DBUserAuthContents
{
public:
   BlackBox        bb;
   int             nodeID;

   int bypassAuthentication(
      Token &               token,
      AUTHENTICATION_INFO & authenticationInfo);


   void deserialize(Token &token);
   void reset();
};

class UserCacheContents
{
public:
   char            externalUsername[257];
   char            databaseUsername[257];
   int32_t         userID;
   bool            isValid;
   int64_t         redefTime;
};

static std::vector<UserCacheContents> userCache;

#define gettid() static_cast<pid_t>(syscall(__NR_gettid))

static DBUserAuth * me = NULL;

#pragma page "DBUserAuth::CloseConnection"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::CloseConnection                                     *
// *                                                                           *
// *    Closes any open LDAP connections.                                      *
// *                                                                           *
// *****************************************************************************

void DBUserAuth::CloseConnection()

{

   LDAPConfigNode::CloseConnection();

}
//******************** End of DBUserAuth::CloseConnection **********************

#pragma page "DBUserAuth::FreeInstance"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::FreeInstance                                        *
// *                                                                           *
// *    Destroys the singleton instance of a DBUserAuth object.                *
// *                                                                           *
// *****************************************************************************

void DBUserAuth::FreeInstance()

{

   if (me != NULL)
      delete me;

}
//********************* End of DBUserAuth::FreeInstance *************************

#pragma page "DBUserAuth::GetInstance"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::GetInstance                                         *
// *    Constructs or returns the singleton instance of a DBUserAuth object.   *
// *                                                                           *
// *****************************************************************************

DBUserAuth * DBUserAuth::GetInstance()

{

   if (me != NULL)
      return me;

   me = new DBUserAuth();
   return me;

}
//********************** End of DBUserAuth::GetInstance ************************

#pragma page "DBUserAuth::GetBlackBox"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::GetBlackBox                                         *
// *                                                                           *
// *    Returns the black box data associated with this session.  If there is  *
// * no black box associated with this session, an empty string is returned.   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <blackBox>                      char *                          Out      *
// *    is the external username (not the database username).                  *
// *                                                                           *
// *  <pretendTokenIsNull>            bool                            In       *
// *    is a minivan boolean used in testing.                                  *
// *                                                                           *
// *****************************************************************************
void DBUserAuth::GetBlackBox(
   char *blackBox,
   bool  pretendTokenIsNull)

{

Token *token = Token::Obtain();

//
// The only way Token::Obtain could return a NULL is if we were unable to
// allocate memory (a couple hundred bytes), which either means there is a
// serious memory leak somewhere or the heap is corrupted.  Neither is very
// likely, but good coding practice dictates we check for NULL anyway.  So
// why would we ever want to pretend the token is NULL?  Just to take a
// different branch and execute a few lines?  Count on it.
//

   if (pretendTokenIsNull)
      token = NULL;

//
// If token is NULL, either real or pretend, return an empty black box.
//

   if (token == NULL)
   {
      blackBox[0] = 0;
      return;
   }

   token->getData(blackBox);

}
//********************** End of DBUserAuth::GetBlackBox ************************


#pragma page "DBUserAuth::GetBlackBoxSize"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::GetBlackBoxSize                                     *
// *    Returns the size of the black box data associated with this session.   *
// * If there is no black box associated with this session, zero is returned.  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameter:                                                               *
// *                                                                           *
// *  <pretendTokenIsNull>            bool                            In       *
// *    is a minivan boolean used in testing.                                  *
// *                                                                           *
// *****************************************************************************
size_t DBUserAuth::GetBlackBoxSize(bool  pretendTokenIsNull)

{

Token *token = Token::Obtain();

   if (pretendTokenIsNull)
      token = NULL;

   if (token == NULL)
      return 0;

   return token->getDataSize();

}
//******************** End of DBUserAuth::GetBlackBoxSize **********************

#pragma page "DBUserAuth::FormatStatusMsg"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::FormatStatusMsg                                     *
// *    Formats a status error or warning as a message.                        *
// *                                                                           *
// *                                                                           *
// * If there is no black box associated with this session, zero is returned.  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <status>                        UA_Status                       In       *
// *    is the status code to be formatted.                                    *
// *                                                                           *
// *  <statusMsg>                     char *                          Out      *
// *    returns the formatted message.                                         *
// *                                                                           *
// *****************************************************************************
void DBUserAuth::FormatStatusMsg(
   UA_Status status,
   char *    statusMsg)

{

   switch (status)
   {
      case UA_STATUS_OK:
         strcpy(statusMsg,"OK, no warnings");
         break;
      case UA_STATUS_ERR_INVALID:
         strcpy(statusMsg,"Username or password is invalid");
         break;
      case UA_STATUS_ERR_SYSTEM:
         strcpy(statusMsg,"Resource or internal error occurred");
         break;
      default:
         strcpy(statusMsg,"Resource or internal error occurred");
         break;
   }

}
//******************** End of DBUserAuth::FormatStatusMsg **********************

#pragma page "DBUserAuth::CheckExternalUsernameDefined"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::CheckExternalUsernameDefined                        *
// *                                                                           *
// *    Determines if an external username of the form expected by the         *
// *    identity during authentication is defined on the identity store.       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <externalUsername>              const char *                    In       *
// *    is the external username to be checked on the identity.                *
// *                                                                           *
// *  <configurationOrdinal>          AuthenticationConfiguration     In       *
// *    specifies which configuration to use when searching for the username.  *
// *    If Unknown, LDAPConfigNode will use the default specified in the       *
// *    config file (.traf_authentication_config).                             *
// *                                                                           *
// *  <foundConfigurationOrdinal>     AuthenticationConfiguration &   Out      *
// *    passes back the configuration where the user was found, or zero if the *
// *    user was not found.                                                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: CheckUserResult                                                  *
// *                                                                           *
// *   UserExists       -- User was found on LDAP server                       *
// *   UserDoesNotExist -- User was not found on LDAP server                   *
// *   ErrorInCheck     -- An error prevented checking                         *
// *                                                                           *
// *****************************************************************************
DBUserAuth::CheckUserResult DBUserAuth::CheckExternalUsernameDefined(
   const char *                  externalUsername,          
   AuthenticationConfiguration   configurationOrdinal,    
   AuthenticationConfiguration & foundConfigurationOrdinal)   

{
  
#ifdef __SQ_LDAP_NO_SECURITY_CHECKING
  return CheckUserResult_UserExists;
#else

char *authEnv;

   authEnv = getenv("TRAFODION_ENABLE_AUTHENTICATION");
   if (authEnv == NULL || strcmp(authEnv,"YES") != 0)
   {
      foundConfigurationOrdinal = PrimaryConfiguration;
      return UserExists;
   }

string userDN = "";       // User DN, used to bind to LDAP serer
LDAPConfigNode *searchNode;

LDAPConfigNode::LDAPConfigType configType = LDAPConfigNode::UnknownConfiguration;

   switch (configurationOrdinal)
   {
      case 1:
         configType = LDAPConfigNode::PrimaryConfiguration;
         break;
      case 2:
         configType = LDAPConfigNode::SecondaryConfiguration;
         break;
      default:
         configType = LDAPConfigNode::UnknownConfiguration;
    } 

   searchNode = LDAPConfigNode::GetLDAPConnection(configType,SearchConnection);

   if (searchNode == NULL) 
      return ErrorDuringCheck;

LDSearchStatus searchStatus = searchNode->lookupUser(externalUsername,userDN);
                                                     
   if (searchStatus == LDSearchNotFound)
      return UserDoesNotExist;
     
// didn't get "found" or "not found", so we have problems!
   if (searchStatus != LDSearchFound) 
      return ErrorDuringCheck;

// External username was found.  But where?      
   switch (searchNode->getConfigType())
   {
      case LDAPConfigNode::UnknownConfiguration:
         foundConfigurationOrdinal = DefaultConfiguration;
         break;
      case LDAPConfigNode::PrimaryConfiguration:
         foundConfigurationOrdinal = PrimaryConfiguration;
         break;
      case LDAPConfigNode::SecondaryConfiguration:
         foundConfigurationOrdinal = SecondaryConfiguration;
         break;
      default:
         foundConfigurationOrdinal = DefaultConfiguration;
   }   
      
   return UserExists;

#endif

}
//************** End of DBUserAuth::CheckExternalUsernameDefined ***************

#pragma page "void DBUserAuth::DBUserAuth"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::DBUserAuth                                          *
// *    This function constructs a DBUserAuth object and its contents.         *
// *                                                                           *
// *****************************************************************************
DBUserAuth::DBUserAuth()
: self(*new DBUserAuthContents())

{

   self.reset();

char processName[MS_MON_MAX_PROCESS_NAME+1]; // +1 for trailing null
        
// obtain our node ID from Seabed
    msg_mon_get_my_info(&self.nodeID,              // nodeID
                        NULL,                      // mon process-id
                        processName,               // mon name
                        MS_MON_MAX_PROCESS_NAME,   // mon name-len
                        NULL,                      // mon process-type
                        NULL,                      // mon zone-id
                        NULL,                      // os process-id
                        NULL);                     // os thread-id

}
//********************** End of DBUserAuth::DBUserAuth *************************


#pragma page "void DBUserAuth::~DBUserAuth"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::~DBUserAuth                                         *
// *    This function destroysa DBUserAuth object and its contents.            *
// *                                                                           *
// *****************************************************************************
DBUserAuth::~DBUserAuth()

{

   delete &self;

}
//********************** End of DBUserAuth::~DBUserAuth ************************


#pragma page "DBUserAuth::getAuthFunction"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::getAuthFunction                                     *
// *                                                                           *
// *    Returns the token verification function set by MXOSRVR.                *
// *                                                                           *
// *****************************************************************************
AuthFunction DBUserAuth::getAuthFunction()

{

   return validateTokenFunc;

}
//******************* End of DBUserAuth::getAuthFunction ***********************
#pragma page "DBUserAuth::getUserID"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::getUserID                                           *
// *                                                                           *
// *    Returns the database user ID for the user.                             *
// *                                                                           *
// *****************************************************************************
int32_t DBUserAuth::getUserID() const

{

   return self.bb.usersInfo.effectiveUserID;

}
//************************ End of DBUserAuth::getUserID ************************

#pragma page "DBUserAuth::getDBUserName"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::getDBUserName                                       *
// *                                                                           *
// *    Returns the database username for the user.                           *
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <databaseUsername>              char *                          Out      *
// *    returns the user name as a null terminated string.                     *
// *  <maxLen>                        size_t                          In       *
// *    maximum size of the null terminated string.                            *
// *                                                                           *
// *****************************************************************************
void DBUserAuth::getDBUserName(char *databaseUsername, size_t maxLen) const

{

   memset(databaseUsername, '\0', maxLen);
   strncpy(databaseUsername,self.bb.usersInfo.databaseUsername, maxLen - 1);
   databaseUsername[maxLen -1] = '\0';

}
//********************** End of DBUserAuth::getDBUserName **********************

#pragma page "DBUserAuth::getExternalUsername"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::getExternalUsername                                 *
// *                                                                           *
// *    Returns the external username for the user.                            *
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <externalUsername>              char *                          Out      *
// *    passes back the external username as a null terminated string.         *
// *                                                                           *
// *  <maxLen>                        size_t                          In       *
// *    maximum size of the null terminated string.                            *
// *                                                                           *
// *****************************************************************************
void DBUserAuth::getExternalUsername(
   char * externalUsername,
   size_t maxLen) const

{

   memset(externalUsername,'\0',maxLen);
   strncpy(externalUsername,self.bb.usersInfo.externalUsername,maxLen - 1);
   externalUsername[maxLen - 1] = '\0';

}
//****************** End of DBUserAuth::getExternalUsername ********************


#pragma page "DBUserAuth::getTokenKeyAsString"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::getTokenKeyAsString                                 *
// *    Returns the Token Key as a formatted ASCII string.                     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <tokenKeyString>                char *                          Out      *
// *    returns the Token Key as a formatted ASCII string.                     *
// *                                                                           *
// *****************************************************************************

void DBUserAuth::getTokenKeyAsString(char *tokenKeyString) const

{

Token *token = Token::Obtain();

// LCOV_EXCL_START
   if (token == NULL)
   {
      tokenKeyString[0] = 0;
      return;
   }
// LCOV_EXCL_STOP

   token->getTokenKeyAsString(tokenKeyString);

}
//***************** End of DBUserAuth::getTokenKeyAsString *********************




#pragma page "Token::getTokenKeySize"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::getTokenKeySize                                     *
// *                                                                           *
// *    Returns the token key size.  Only used in testing.                     *
// *                                                                           *
// *****************************************************************************
size_t DBUserAuth::getTokenKeySize() const

{

Token *token = Token::Obtain();

   if (token == NULL)
      return 0;
      
   return token->getTokenKeySize();

}
//******************* End of DBUserAuth:getTokenKeySize ************************


#pragma page "DBUserAuth::setAuthFunction"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::setAuthFunction                                     *
// *                                                                           *
// *    Sets the token verification function.                                  *
// *                                                                           *
// *****************************************************************************
void DBUserAuth::setAuthFunction(AuthFunction somefunc)

{

   validateTokenFunc = somefunc;

}
//******************** End of DBUserAuth::setAuthFunction **********************


#pragma page "DBUserAuth::verify"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::verify                                              *
// *                                                                           *
// *    Verifies a username and password on a database instance.               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <username>                      const char *                    In       *
// *    is the external username (not the database username).                  *
// *                                                                           *
// *  <credentials>                   char *                          In/Out   *
// *    is the password for the user or a token that represents the user       *
// *  session.  On return is a token.                                          *
// *                                                                           *
// *  <errorDetail>                   UA_Status &                     Out      *
// *    passes back which input parameter is at fault on failure.              *
// *                                                                           *
// *  <authenticationInfo             AUTHENTICATION_INFO &           Out      *
// *    passes back user and error data related to the authentication.         *
// *                                                                           *
// *  <client_info>                   const CLIENT_INFO &             In       *
// *    is the client related information needed for audit logging.            *
// *                                                                           *
// *  <performanceInfo>               PERFORMANCE_INFO &              Out      *
// *    passes back elapsed time for authentication suboperations.             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: int                                                              *
// *                                                                           *
// *   0 - Authentication completed.                                           *
// *  22 - Bounds error.  Number of param in error returned in errorDetail.    *
// * 590 - NULL param.  Number of param in error returned in errorDetail.      *
// *                                                                           *
// *****************************************************************************
int DBUserAuth::verify(
   const char           * externalUsername,        
   char                 * credentials,     
   UA_Status            & errorDetail,     
   AUTHENTICATION_INFO  & authenticationInfo,  
   const CLIENT_INFO    & client_info,     
   PERFORMANCE_INFO     & performanceInfo) 


{

char buf[500];

   memset((void *)&performanceInfo, '\0', sizeof(performanceInfo));


   if (externalUsername == NULL)
   {
      errorDetail = UA_STATUS_PARAM1;
      return ZFIL_ERR_BADPARMVALUE;
   }

   if (credentials == NULL)
   {
      errorDetail = UA_STATUS_PARAM2;
      return ZFIL_ERR_BADPARMVALUE;
   }

   if (strlen(externalUsername) > MAX_EXTUSERNAME_LEN)
   {
      errorDetail = UA_STATUS_PARAM1;
      return ZFIL_ERR_BOUNDSERR;
   }

   if (strlen(credentials) > MAX_EXTPASSWORD_LEN)
   {
      errorDetail = UA_STATUS_PARAM2;
      return ZFIL_ERR_BOUNDSERR;
   }
   
static ClientContents clientInfo;

   memset((void *)&clientInfo, '\0', sizeof(clientInfo));

   if (client_info.client_name != NULL)
      strncpy(clientInfo.clientName,
              client_info.client_name,
              sizeof(clientInfo.clientName) - 1);
   if (client_info.client_user_name != NULL)
       strncpy(clientInfo.clientUserName,
               client_info.client_user_name,
               sizeof(clientInfo.clientUserName) - 1);
   if (client_info.application_name != NULL)
       strncpy(clientInfo.applicationName,
               client_info.application_name,
               sizeof(clientInfo.applicationName) - 1);
   

   errorDetail = UA_STATUS_OK;
   self.reset();

Token *token = Token::Verify(credentials,validateTokenFunc);

// If we have a valid token, we need to deserialize the black box data
   if (token != NULL)
   {
      self.deserialize(*token);  //ACH load auth info from black box data.

      //Need to set performance info
      return ZFIL_ERR_OK;
   }

//
// Credentials are not a valid token; must be a password.  Proceed with normal
// authentication.  First, obtain a token to store authentication results.
//

   token = Token::Obtain();

   if (token == NULL)
      return ZFIL_ERR_BOUNDSERR; //ACH better error?

   token->reset();
char *authEnv;

   authEnv = getenv("TRAFODION_ENABLE_AUTHENTICATION");
   if (authEnv == NULL || strcmp(authEnv,"YES") != 0)
      return self.bypassAuthentication(*token,authenticationInfo);

//
// Ok, let's actually verify the username and password.
//

   authenticateUser(self,externalUsername,credentials,clientInfo,
                    authenticationInfo,performanceInfo);
   
// Convert password to a token key.  Note this is done for all authentications,
// both successful and unsuccessful.

   token->getTokenKey(authenticationInfo.tokenKey);
   authenticationInfo.tokenKeySize = token->getTokenKeySize();

// Save authentication results in black box within token container
   token->setData((char *)&self.bb,sizeof(self.bb));   
   
   return ZFIL_ERR_OK;

}
//************************* End of DBUserAuth::verify **************************

#pragma page "DBUserAuth::verifyChild"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuth::verifyChild                                         *
// *                                                                           *
// *    Verifies the token key sent by a child MXOSRVR and returns the         *
// * blackbox data.                                                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <credentials>                   const char *                    In       *
// *    is the token sent by the child server.                                 *
// *                                                                           *
// *  <data>                          char *                          Out      *
// *    passes back the authentication data (black box) associated with        *
// *    this token.                                                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: size_t                                                           *
// *                                                                           *
// *    The number of bytes in <data>.                                         *
// *                                                                           *
// *****************************************************************************
size_t DBUserAuth::verifyChild(
   const char  * credentials,
   char        * data)

{

// Compare the token key sent with the one stored in this server
Token *token = Token::Verify(credentials);

   if (token == NULL)
      return 0;

   token->getData(data);

   return token->getDataSize();

}
//********************* End of DBUserAuth::verifyChild **********************

// DBUserAuthContents functions

#pragma page "DBUserAuthContents::bypassAuthentication"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuthContents::bypassAuthentication                        *
// *                                                                           *
// *    Skip authentication, populate fields as if DB__ROOT logged on          *
// *  successfully.                                                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <token>                         Token &                         In       *
// *    is a reference to the token containing the data to be deserialized.    *
// *                                                                           *
// *  <authenticationInfo             AUTHENTICATION_INFO &           Out      *
// *    passes back user and error data related to the authentication.         *
// *                                                                           *
// *****************************************************************************
int DBUserAuthContents::bypassAuthentication(
   Token &               token,
   AUTHENTICATION_INFO & authenticationInfo)

{

   bb.error = ZFIL_ERR_OK;
   bb.isAuthenticated = true;
   bb.errorDetail = UA_STATUS_OK;
   bb.usersInfo.effectiveUserID = SUPER_USER;
   bb.usersInfo.sessionUserID = SUPER_USER;
   strcpy(bb.usersInfo.databaseUsername,"DB__ROOT");
   strcpy(bb.usersInfo.externalUsername,"DB__ROOT");
   token.setData((char *)&bb,sizeof(bb)); 
   
   authenticationInfo.error = bb.error;
   authenticationInfo.errorDetail = bb.errorDetail;   
   authenticationInfo.usersInfo.effectiveUserID = SUPER_USER;
   authenticationInfo.usersInfo.sessionUserID = SUPER_USER;
   strcpy(authenticationInfo.usersInfo.databaseUsername,"DB__ROOT");
   strcpy(authenticationInfo.usersInfo.externalUsername,"DB__ROOT");
   authenticationInfo.tokenKeySize = token.getTokenKeySize();
   token.getTokenKey(authenticationInfo.tokenKey);
   
   return ZFIL_ERR_OK;

}
//************* End of DBUserAuthContents::bypassAuthentication ****************



#pragma page "DBUserAuthContents::deserialize"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuthContents::deserialize                                 *
// *    Sets all member values from a serialized token blackbox.               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <token>                         Token &                         In       *
// *    is a reference to the token containing the data to be deserialized.    *
// *                                                                           *
// *****************************************************************************

void DBUserAuthContents::deserialize(Token &token)

{

size_t blackBoxSize = token.getDataSize();

char *blackBox = new char[blackBoxSize];

   token.getData(blackBox);

   memcpy((char *)(&bb),blackBox,blackBoxSize);

   delete [] blackBox;

}
//***************** End of DBUserAuthContents::deserialize *********************

#pragma page "DBUserAuthContents::reset"
// *****************************************************************************
// *                                                                           *
// * Function: DBUserAuthContents::reset                                       *
// *    Resets all the member values to their default.                         *
// *                                                                           *
// *****************************************************************************
void DBUserAuthContents::reset()

{

   bb.usersInfo.effectiveUserID = 0;//ACH can we reserve value for NO_SUCH_USER?
   bb.usersInfo.sessionUserID = 0;//ACH can we reserve value for NO_SUCH_USER?
   bb.usersInfo.externalUsername[0] = 0;
   bb.usersInfo.databaseUsername[0] = 0;
   bb.usersInfo.redefTime = 0;
   bb.isAuthenticated = false;
   bb.error = 0;
   bb.errorDetail = UA_STATUS_OK;

}
//********************* End of DBUserAuthContents::reset ***********************

// Begin private functions


#pragma page "authenticateUser"
// *****************************************************************************
// *                                                                           *
// * Function: authenticateUser                                                *
// *                                                                           *
// *    Validates user is registered on Trafodion, and the username and        *
// *  password are valid on the directory server.                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <self>                          DBUserAuthContents &            In/Out   *
// *    is a reference to a DBUserAuthContents object.  Results of the         *
// *  authentication are stored here.                                          *
// *                                                                           *
// *  <externalUsername>              const char *                    In       *
// *    is the username.  Username must be registered (in AUTHS table) and     *
// *  defined on LDAP server.                                                  *
// *                                                                           *
// *  <password>                      const char *                    In       *
// *    is the password for username.  Password is authenticated on LDAP server*
// *                                                                           *
// *  <authenticationInfo             AUTHENTICATION_INFO &           Out      *
// *    passes back user and error data related to the authentication.         *
// *                                                                           *
// *  <client_info>                   const CLIENT_INFO &             In       *
// *    is the client related information needed for audit logging.            *
// *                                                                           *
// *  <performanceInfo>               PERFORMANCE_INFO &              Out      *
// *    passes back elapsed time for authentication suboperations.             *
// *                                                                           *
// *****************************************************************************

static void authenticateUser(
   DBUserAuthContents &  self,
   const char *          externalUsername,
   const char *          password,
   ClientContents &      clientInfo,
   AUTHENTICATION_INFO & authenticationInfo,
   PERFORMANCE_INFO &    performanceInfo)

{

   self.bb.isAuthenticated = false;

long retCode;
bool isValid;
AuthConfigType authType;
USERS_INFO usersInfo;

//
// First step; is the user registered.  If not, no sense bothering the
// LDAP server.
//
   memset(usersInfo.databaseUsername,' ',sizeof(usersInfo.databaseUsername));
   strcpy(usersInfo.externalUsername,externalUsername);
   
int64_t startTime = JULIANTIMESTAMP();

   retCode = fetchFromAUTHSTable(externalUsername,usersInfo.databaseUsername,
                                 usersInfo.sessionUserID,isValid,
                                 usersInfo.redefTime);                                   
   
   performanceInfo.sqlUserTime = JULIANTIMESTAMP() - startTime;
   if (retCode != 0)
   {
// LCOV_EXCL_START
      authenticationInfo.error = self.bb.error = ZFIL_ERR_SECVIOL;
      // Error 100 (NOT FOUND) means user isn't a registered SQ user.
      // For now this is an error, in the future there could be an
      // option to auto-register users.
      if (retCode == 100)
         self.bb.errorDetail = UA_STATUS_ERR_INVALID;
      else//ACH Should we log other SQL errors here?
         self.bb.errorDetail = UA_STATUS_ERR_SYSTEM;

      authenticationInfo.errorDetail = self.bb.errorDetail;
      
      logAuthenticationOutcome(usersInfo.externalUsername,
                               usersInfo.databaseUsername,
                               usersInfo.sessionUserID,clientInfo,"F ");
      return;
// LCOV_EXCL_STOP
   }

//
// User is registered, but is the account still valid?  Users can be
// marked offline by ALTER USER command or possibly in the future when
// we detect a registered user is no longer defined on the directory server.
//

   if (!isValid)
   {
      authenticationInfo.error = self.bb.error = ZFIL_ERR_SECVIOL;
      authenticationInfo.errorDetail = self.bb.errorDetail = UA_STATUS_ERR_INVALID;

      logAuthenticationOutcome(usersInfo.externalUsername,
                               usersInfo.databaseUsername,
                               usersInfo.sessionUserID,clientInfo,"F ");
      return;
   }
   
//
// User is registered and valid, but is the authentication type recognized?  
// If not, reject the authentication.
//
//ACH not in metadata currently.
/*
   if (authType == AuthUnknownConfiguration)
   {
      self.bb.error = ZFIL_ERR_SECVIOL;
      self.bb.errorDetail = UA_STATUS_ERR_INVALID;

      logAuthenticationOutcome(usersInfo.externalUsername,
                               usersInfo.databaseUsername,
                               usersInfo.sessionUserID,clientInfo,"F ");
      return;
   }
*/      
//
// Let's check on the credentials first.
//
   if (strlen(password) == 0)
   {
      // zero len password is a non-auth bind in LDAP, so we treat it
      // as a failed authorization
      authenticationInfo.error = self.bb.error = ZFIL_ERR_SECVIOL;
      authenticationInfo.errorDetail = self.bb.errorDetail = UA_STATUS_ERR_INVALID;
      logAuthenticationOutcome(usersInfo.externalUsername,
                               usersInfo.databaseUsername,
                               usersInfo.sessionUserID,clientInfo,"F ");
      return;
   }

LDAuthStatus authStatus = LDAuthSuccessful;
bool errorsLogged = false;
   
//
// Next step, see if the user is defined on the LDAP server.
//

//ACH For now, only support primary configuration   
   authStatus = executeLDAPAuthentication(self,usersInfo.externalUsername,
                                          password,
                                          LDAPConfigNode::PrimaryConfiguration,
                                          errorsLogged,
                                          performanceInfo);
   
// We log errors encountered in LDAP authentication to the repository.  
// We can't log within the LDAP authentication module since other clients
// don't include the necessary libraries.  executeLDAPAuthentication() may  
// have already logged the internal error(s); if not, log them now.                       
   if (!errorsLogged)
   {
      logAuthenticationErrors(self.nodeID); 
      errorsLogged = true;
   }

// We log retries regardless of whether the authentication succeeded 
// or failed.  Logging the retries for failed authentications shows
// problem was not transient.  Logging retries when the 
// authentication was successful allows operations to be aware of 
// potential problems.
   logAuthenticationRetries(self.nodeID,externalUsername);

//
// If all is well, save all the relevant user data in our container.
//
   if (authStatus == LDAuthSuccessful)
   {
      // Strings returned from SQL could have trailing blanks and nulls
      // Remove all trailing blanks so callers have an accurate length
      // for comparison.
      stripAllTrailingBlanks(usersInfo.databaseUsername,256);
      //ACH copy from BB usersInfo to authenticationInfo.usersInfo?
      strcpy(authenticationInfo.usersInfo.databaseUsername,usersInfo.databaseUsername);
      strcpy(authenticationInfo.usersInfo.externalUsername,externalUsername);
      usersInfo.effectiveUserID = usersInfo.sessionUserID;      
   // Copy USERS_INFO fields.  Class, = operator, byte copy
      authenticationInfo.usersInfo.effectiveUserID = usersInfo.effectiveUserID; 
      authenticationInfo.usersInfo.sessionUserID = usersInfo.sessionUserID; 
      authenticationInfo.usersInfo.redefTime = usersInfo.redefTime; 
      authenticationInfo.error = ZFIL_ERR_OK; 
      authenticationInfo.errorDetail = UA_STATUS_OK; 
   

       // copy user info to black box
      self.bb.error = ZFIL_ERR_OK;
      self.bb.errorDetail = UA_STATUS_OK;
      self.bb.usersInfo.effectiveUserID = usersInfo.effectiveUserID;
      self.bb.usersInfo.sessionUserID = usersInfo.sessionUserID;
      self.bb.usersInfo.redefTime = usersInfo.redefTime;
      strcpy(self.bb.usersInfo.databaseUsername,usersInfo.databaseUsername);
      strcpy(self.bb.usersInfo.externalUsername,externalUsername);
      self.bb.isAuthenticated = true;
      // Log the successful authentication to the audit log repository.
      logAuthenticationOutcome(externalUsername,usersInfo.databaseUsername,
                               usersInfo.sessionUserID,clientInfo,"S ");

      return;
   }

//
// Rejected!
//
// Either the provided password does not match the one stored on the LDAP 
// server or we had internal problems with the server.
//
// No soup for you.
//

   authenticationInfo.error = ZFIL_ERR_SECVIOL; 
   self.bb.error = ZFIL_ERR_SECVIOL;
   if (authStatus == LDAuthRejected)
      self.bb.errorDetail = UA_STATUS_ERR_INVALID;
   else 
   {
      self.bb.errorDetail = UA_STATUS_ERR_SYSTEM;
   }
   authenticationInfo.errorDetail = self.bb.errorDetail; 

// Log the failed authentication to the audit log repository.
   logAuthenticationOutcome(externalUsername,usersInfo.databaseUsername,
                            usersInfo.sessionUserID,clientInfo,"F ");

}
//************************** End of authenticateUser ***************************


// *****************************************************************************
// *                                                                           *
// * Function: cacheUserInfo                                                   *
// *                                                                           *
// *    Stores information for the user in local cache.                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <externalUsername>              const char *                    In       *
// *    is the external username.                                              *
// *                                                                           *
// *  <databaseUsername>              const char *                    In       *
// *    is the database username associated with the external username.        *
// *                                                                           *
// *  <userID>                        int32_t                         In       *
// *    passes back the numeric ID of the database user associated             *
// *  with the external username.                                              *
// *                                                                           *
// *  <isValid>                       bool                            In       *
// *    is true if the user is online, false if offline.                       *
// *                                                                           *
// *  <redefTime>                     int64_t                         In       *
// *    is the time the user record was last written.                          *
// *                                                                           * 
// *****************************************************************************
void cacheUserInfo(
   const char    * externalUsername,
   const char    * databaseUsername,
   int32_t         userID,
   bool            isValid,
   int64_t         redefTime)
   
{

UserCacheContents userInfo;

   strcpy(userInfo.externalUsername,externalUsername);
   strcpy(userInfo.databaseUsername,databaseUsername);
   userInfo.userID = userID;
   userInfo.isValid = isValid;
   userInfo.redefTime = redefTime;
   
   userCache.push_back(userInfo);
   
}
//*************************** End of cacheUserInfo *****************************



#pragma page "executeLDAPAuthentication"
// *****************************************************************************
// *                                                                           *
// * Function: executeLDAPAuthentication                                       *
// *                                                                           *
// *    Sends request to LDAP server (via LDAPConfigNode class) to authenticate*
// *  the provided username and password.                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <self>                          DBUserAuthContents &            In/Out   *
// *    is a reference to a DBUserAuthContents object.  Results of the         *
// *  authentication are stored here.                                          *
// *                                                                           *
// *  <username>                      const char *                    In       *
// *    is the username.  Username must be  defined on LDAP server.            *
// *                                                                           *
// *  <password>                      const char *                    In       *
// *    is the password for username.  Password is authenticated on LDAP server*
// *                                                                           *
// *  <configType>                    LDAPConfigNode::LDAPConfigType  In       *
// *    is the LDAP configuration to use, either primary or secondary.         *
// *                                                                           *
// *  <errorsLogged>                  bool &                          In       *
// *    passes back true if an internal error was logged to the repository,    *
// *  otherwise false.                                                         *
// *                                                                           *
// *****************************************************************************

static LDAuthStatus executeLDAPAuthentication(
   DBUserAuthContents &            self,
   const char *                    username,
   const char *                    password,
   LDAPConfigNode::LDAPConfigType  configType,
   bool &                          errorsLogged,
   PERFORMANCE_INFO &              performanceInfo)

{

   LDAPConfigNode::ClearRetryCounts();
   clearAuthEvents();
   errorsLogged = false;
   
//
// First get a search connection for the specified configuration. 
// If we can't get a search connection, could be network problem or a
// bad configuration.  Either way, tough luck for the user.
//

int64_t startTime = JULIANTIMESTAMP();
LDAPConfigNode *searchNode = LDAPConfigNode::GetLDAPConnection(configType,
                                                               SearchConnection);

   performanceInfo.searchConnectionTime = JULIANTIMESTAMP() - startTime;

   if (searchNode == NULL)
   {
// LCOV_EXCL_START
      self.bb.error = ZFIL_ERR_SECVIOL;
      self.bb.errorDetail = UA_STATUS_ERR_SYSTEM;
      return LDAuthResourceFailure;
// LCOV_EXCL_STOP
   }

string userDN = "";       // User DN, used to bind to LDAP serer

   startTime = JULIANTIMESTAMP();

LDSearchStatus searchStatus = searchNode->lookupUser(username,userDN);
                                                     
   performanceInfo.searchTime = JULIANTIMESTAMP() - startTime; 
                                                       
   if (searchStatus == LDSearchNotFound)
   {
      self.bb.error = ZFIL_ERR_SECVIOL;
      self.bb.errorDetail = UA_STATUS_ERR_INVALID;

      return LDAuthRejected;
   }

   if (searchStatus != LDSearchFound)
   {
// LCOV_EXCL_START
      self.bb.error = ZFIL_ERR_SECVIOL;
      self.bb.errorDetail = UA_STATUS_ERR_SYSTEM;
      return LDAuthResourceFailure;
// LCOV_EXCL_STOP
   }

// (searchStatus == LDSearchFound)
// ACH: Should we compare UUIDOnLDAP and dsUUID and give "not registered"
//       (or some other) error if they don't match?

//
// User is defined here and there.  But is their password correct?
// Let's get an authentication connection to check on the password
//
   startTime = JULIANTIMESTAMP();

LDAPConfigNode *authNode = LDAPConfigNode::GetLDAPConnection(configType,
                                                             AuthenticationConnection);

   performanceInfo.authenticationConnectionTime = JULIANTIMESTAMP() - startTime;
   
   if (authNode == NULL)
   {
      self.bb.error = ZFIL_ERR_SECVIOL;
      self.bb.errorDetail = UA_STATUS_ERR_SYSTEM;
      return LDAuthResourceFailure;
   }

//
// Non-blank password, a user we know about, let's validate that password!
//
   startTime = JULIANTIMESTAMP();

LDAuthStatus authStatus = authNode->authenticateUser(userDN.c_str(),password);
                                     
   performanceInfo.authenticationTime = JULIANTIMESTAMP() - startTime;
                                          
   return authStatus;                                       

}
//********************** End of executeLDAPAuthentication **********************

#pragma page "fetchFromAUTHSTable"
// *****************************************************************************
// *                                                                           *
// * Function: fetchFromAUTHSTable                                             *
// *                                                                           *
// *    Fetches the requested columns from the row of the AUTHS table          *
// * containing the provided external username.                                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <externalUsername>              const char *                    In       *
// *    is the external username.                                              *
// *                                                                           *
// *  <databaseUsername>              char *                          Out      *
// *    passes back the database username associated with the external         *
// *  username.                                                                *
// *                                                                           *
// *  <userID>                        long &                          Out      *
// *    passes back the numeric ID of the database user associated             *
// *  with the external username.                                              *
// *                                                                           *
// *  <isValid>                       long *                          Out      *
// *    passes back the status (valid Y/N) of the database user associated     *
// *  with the external username.                                              *
// *                                                                           *
// *  <redefTime>                     int64_t &                       Out      *
// *                                                                           * 
// *****************************************************************************
// *                                                                           *
// * Returns: int32_t/SQL error code                                           *
// *                                                                           *
// *   0 - External username found, all return parameters set.                 *
// * 100 - External username not found.                                        *
// *   * - Query error                                                         *
// *                                                                           *
// *****************************************************************************
static int32_t fetchFromAUTHSTable(
   const char     * externalUsername,
   char           * databaseUsername,
   int32_t        & userID,
   bool           & isValid,
   int64_t        & redefTime)
   
{

const UserCacheContents *userInfo = fetchFromCacheByUsername(externalUsername);

   if (userInfo != NULL)
   {
      strcpy(databaseUsername,userInfo->databaseUsername);
      userID = userInfo->userID;
      isValid = userInfo->isValid;
      redefTime = userInfo->redefTime;
      return 0;
   }

ExeCliInterface cliInterface;
char stmt[2000]; 

   sprintf(stmt,"SELECT AUTH_TYPE,AUTH_ID,AUTH_DB_NAME,AUTH_IS_VALID,"
                "AUTH_REDEF_TIME"
//                ",AUTH_IS_IMMUTABLE"
//                ",AUTH_CONFIGURATION"
                " FROM %s.\"%s\".%s WHERE AUTH_EXT_NAME = '%s' ",
                "TRAFODION",SEABASE_MD_SCHEMA,SEABASE_AUTHS,externalUsername);

int32_t cliRC = cliInterface.fetchRowsPrologue(stmt,true/*no exec*/);
   
   if (cliRC < 0)
      return cliRC;
  
   cliRC = cliInterface.clearExecFetchClose(NULL,0);
   if (cliRC < 0)
      return cliRC;

   if (cliRC == 100) // did not find the row
     return cliRC;

char *ptr = NULL;
int32_t len = 0;
   
   cliInterface.getPtrAndLen(1,ptr,len);
   if (ptr[0] != 'U')
      return 100;
      
   cliInterface.getPtrAndLen(2,ptr,len);
   userID = *reinterpret_cast<int32_t *>(ptr);

   cliInterface.getPtrAndLen(3,ptr,len);
   strncpy(databaseUsername,ptr,len);
   databaseUsername[len] = '\0';

   cliInterface.getPtrAndLen(4,ptr,len);
    
   if (ptr[0] == 'Y')
      isValid = true;
   else
      isValid = false;
       
   cliInterface.getPtrAndLen(5,ptr,len);
   redefTime = *reinterpret_cast<int64_t *>(ptr);
      
//ACH Add support for immutable users
/*   
   cliInterface.getPtrAndLen(6,ptr,len);
    
   if (ptr[0] == 'Y')
      cacheUserInfo(externalUsername,databaseUsername,userID,isValid,redefTime);
*/  
//ACH add support for configuration number
/*
   cliInterface.getPtrAndLen(7,ptr,len);
   configurationNumber = *reinterpret_cast<int32_t *>(ptr);
*/  
   cliInterface.fetchRowsEpilogue(NULL,true);
   return 0;
  
}
//************************* End of fetchFromAUTHSTable *************************



#pragma page "fetchFromAUTHSTable"
// *****************************************************************************
// *                                                                           *
// * Function: fetchFromAUTHSTable                                             *
// *                                                                           *
// *    Fetches the requested columns from the row of the AUTHS table          *
// * containing the provided database user ID.                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <userID>                        int32_t                         In       *
// *    is the nummeric ID of the database user.                               *
// *                                                                           *
// *  <databaseUsername>              char *                          Out      *
// *    passes back the database username associated with the numeric user ID. *
// *                                                                           *
// *  <externalUsername>              char *                          Out      *
// *    passes back the external username associated with the numeric user ID. *
// *                                                                           *
// *  <isValid>                       bool &                          Out      *
// *    passes back the status of the database user associated with the        *
// *    numeric user ID.                                                       *
// *                                                                           *
// *  <redefTime>                     int64_t &                       Out      *
// *                                                                           * 
// *****************************************************************************
// *                                                                           *
// * Returns: int32_t/SQL error code                                           *
// *                                                                           *
// *   0 - External username found, all return parameters set.                 *
// * 100 - External username not found.                                        *
// *   * - Query error                                                         *
// *                                                                           *
// *****************************************************************************
static int32_t fetchFromAUTHSTable(
   int32_t          userID,
   char           * databaseUsername,
   char           * externalUsername,
   bool           & isValid,
   int64_t        & redefTime)
   
{

const UserCacheContents *userInfo = fetchFromCacheByUserID(userID);

   if (userInfo != NULL)
   {
      strcpy(databaseUsername,userInfo->databaseUsername);
      strcpy(externalUsername,userInfo->externalUsername);
      isValid = userInfo->isValid;
      redefTime = userInfo->redefTime;
      return 0;
   }

ExeCliInterface cliInterface;
char stmt[2000]; 

   sprintf(stmt,"SELECT AUTH_TYPE,AUTH_EXT_NAME,AUTH_DB_NAME,AUTH_IS_VALID,"
                "AUTH_REDEF_TIME"
//                ",AUTH_IS_IMMUTABLE"
                " FROM %s.\"%s\".%s WHERE AUTH_ID = %ld ",
                "TRAFODION",SEABASE_MD_SCHEMA,SEABASE_AUTHS,userID);

int32_t cliRC = cliInterface.fetchRowsPrologue(stmt,true/*no exec*/);

   if (cliRC < 0)
      return cliRC;

   cliRC = cliInterface.clearExecFetchClose(NULL,0);
   if (cliRC < 0)
      return cliRC;

   if (cliRC == 100) // did not find the row
     return cliRC;

char *ptr = NULL;
int32_t len = 0;
   
   cliInterface.getPtrAndLen(1,ptr,len);
   if (ptr[0] != 'U')
      return 100;
      
   cliInterface.getPtrAndLen(2,ptr,len);
   strncpy(externalUsername,ptr,len);
   databaseUsername[len] = '\0';

   cliInterface.getPtrAndLen(3,ptr,len);
   strncpy(databaseUsername,ptr,len);
   databaseUsername[len] = '\0';

   cliInterface.getPtrAndLen(4,ptr,len);
    
   if (ptr[0] == 'Y')
      isValid = true;
   else
      isValid = false;
       
   cliInterface.getPtrAndLen(5,ptr,len);
   redefTime = *reinterpret_cast<int64_t *>(ptr);
//ACH Add support for immutable users
/*   
   cliInterface.getPtrAndLen(6,ptr,len);
    
   if (ptr[0] == 'Y')
      cacheUserInfo(externalUsername,databaseUsername,userID,isValid,redefTime);
*/    
   cliInterface.fetchRowsEpilogue(NULL,true);
   return 0;
  
}
//************************* End of fetchFromAUTHSTable *************************



// *****************************************************************************
// *                                                                           *
// * Function: fetchFromCacheByUsername                                        *
// *                                                                           *
// *    Searches local cache for user information.  If found, a record with    *
// * the data is returned, otherwise NULL is returned.                         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <externalUsername>              const char *                    In       *
// *    is the external username to search for in cache.                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: const UserCacheContents *                                        *
// *                                                                           *
// *     NULL - External username not found in cache.                          *
// * Not NULL - External username was found, cached record returned.           *
// *                                                                           *
// *****************************************************************************
inline static const UserCacheContents * fetchFromCacheByUsername(const char * externalUsername)

{

size_t cacheCount = userCache.size();
char upperUsername[129] = {0};

   upshiftString(externalUsername,upperUsername);

   for (size_t index = 0; index < cacheCount; index++)
      if (strcmp(userCache[index].externalUsername,upperUsername) == 0)
         return &userCache[index]; 
   
   return NULL;

}
//********************** End of fetchFromCacheByUsername ***********************




// *****************************************************************************
// *                                                                           *
// * Function: fetchFromCacheByUserID                                          *
// *                                                                           *
// *    Searches local cache for user information.  If found, a record with    *
// * the data is returned, otherwise NULL is returned.                         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <userID>                        long                            In       *
// *    is the database userID to search for in cache.                         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: const UserCacheContents *                                        *
// *                                                                           *
// *     NULL - Database userID not found in cache.                            *
// * Not NULL - Database userID was found, cached record returned.             *
// *                                                                           *
// *****************************************************************************
inline static const UserCacheContents * fetchFromCacheByUserID(long userID)

{

size_t cacheCount = userCache.size();

   for (size_t index = 0; index < cacheCount; index++)
      if (userCache[index].userID == userID)
         return &userCache[index]; 
   
   return NULL;

}
//*********************** End of fetchFromCacheByUserID ************************




#pragma page "logAuthenticationErrors"
// *****************************************************************************
// *                                                                           *
// * Function: logAuthenticationErrors                                         *
// *                                                                           *
// *    Logs resource failure errors encountered during authentication.        *
// * Resource failure errors include problems with the                         *
// * configuration in .traf_authrntication_config, network issues,             *
// * LDAP server issues, or coding blunders.                                   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <nodeID>                        int                             In       *
// *    is the ID of the node we are running on.                               *
// *                                                                           *
// *****************************************************************************
static void logAuthenticationErrors(int nodeID)

{
}
//************************ End of logAuthenticationErrors **********************

#pragma page "logAuthenticationOutcome"
// *****************************************************************************
// *                                                                           *
// * Function: logAuthenticationOutcome                                        *
// *                                                                           *
// *    Logs the outcome.                                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <external_user_name>            string &                        In       *
// *    is the external username.                                              *
// *                                                                           *
// *  <internal_user_name>            string &                        In       *
// *    internal user name                                                     *
// *                                                                           *
// *  <userID>                        long                            In       *
// *    is the numeric ID of the database user associated                      *
// *  with the external username.                                              *
// *                                                                           *
// *  <outcome>                       string &                        In       *
// *    the outcome of the authentication                                      *
// *                                                                           *
// *****************************************************************************
static void logAuthenticationOutcome(
   const string   & external_user_name,
   const string   & internal_user_name,
   const int32_t    user_id,
   ClientContents & clientInfo,
   const string   & outcome)
   
{
}
//*********************** End of logAuthenticationOutcome **********************



#pragma page "logAuthenticationRetries"
// *****************************************************************************
// *                                                                           *
// * Function: logAuthenticationRetries                                        *
// *                                                                           *
// *    Logs retries encountered during authentication.                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <nodeID>                        int                             In       *
// *    is the ID of the node this process is executing on.  The node ID is    *
// *  needed to log to the EVENT_TEXT_TABLE table.                             *
// *                                                                           *
// *  <username>                      char *                          In       *
// *    is the name of the user that we attempted to authenticate.             *
// *                                                                           *
// *****************************************************************************
static void logAuthenticationRetries(
   int          nodeID,
   const char * username)

{
}
//*********************** End of logAuthenticationRetries **********************



#pragma page "logToEventTable"
// *****************************************************************************
// *                                                                           *
// * Function: logToEventTable                                                 *
// *                                                                           *
// *    Logs to the EVENT_TEXT_TABLE table in the INSTANCE_REPOSITORY schema.  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <nodeID>                        int                             In       *
// *    is the ID of the node this process is executing on.                    *
// *                                                                           *
// *  <eventID>                       DB_SECURITY_EVENTID             In       *
// *    is the ID of the event to be logged.                                   *
// *                                                                           *
// *  <severity>                      posix_sqlog_severity_t          In       *
// *    is the severity of the event.                                          *
// *                                                                           *
// *  <msg>                           char *                          In       *
// *    is the message text to be logged.                                      *
// *                                                                           *
// *****************************************************************************
static void logToEventTable(
   int                    nodeID,
   DB_SECURITY_EVENTID    eventID, 
   posix_sqlog_severity_t severity, 
   const char *           msg)

{
}
//**************************** End of logToEventTable **************************




#pragma page "prepareUserNameForQuery"
// *****************************************************************************
// *                                                                           *
// * Function: prepareUserNameForQuery                                         *
// *                                                                           *
// *    Converts a username into uppercase and escapes any single quote        *
// * chars to match SQL string literal rules (by replacing each single         *
// *  quote char with a pair of single quote chars).                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <nameForQuery>                  char *                          Out      *
// *    is the converted username.                                             *
// *                                                                           *
// *  <extName>                       const char *                    In       *
// *    is the external username to be converted.                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: int                                                              *
// *                                                                           *
// *  0  - Validation successful.                                              *
// * 22  - Bounds error.  Number of param in error returned in status.         *
// * 590 - NULL param.  Number of param in error returned in status.           *
// *                                                                           *
// *****************************************************************************
static void prepareUserNameForQuery(
   char       * nameForQuery,
   const char * extName)

{

   while (*extName != '\0')
   {
      if (*extName == '\'')
      {
         // replace original single quote with a pair of single quotes
         *(nameForQuery++) = '\'';
         *(nameForQuery++) = '\'';
      }
      else
         *(nameForQuery++) = toupper(*extName);

      extName++;
   }
   *nameForQuery = '\0';

}
//*********************** End of prepareUserNameForQuery ***********************

#pragma page "stripAllTrailingBlanks"
// *****************************************************************************
// *                                                                           *
// * Function: stripAllTrailingBlanks                                          *
// *                                                                           *
// *  Removes trailing blanks (replaces with null). String may have embedded   *
// *  NULLs; trailing blanks are replaced with NULL until first non-blank,     *
// *  non-null character.                                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <buf>                          char *                           In/Out   *
// *    is a character buffer whose trailing blanks are to be trimmed.         *
// *                                                                           *
// *  <len>                          size_t                           In       *
// *    is the size of <buf>.                                                  *
// *                                                                           *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
static void stripAllTrailingBlanks(
   char   * buf,
   size_t   len)

{

char *ptr = &buf[len];

    while (*ptr == ' ' || *ptr == 0)
    {
       *ptr = 0;
       ptr--;
    }

}
//****************** End of trimLeadingandTrailingBlanks ***********************

#pragma page "timeDiff"
// *****************************************************************************
// *                                                                           *
// * Function: timeDiff                                                        *
// *                                                                           *
// *  Find the difference between two timestamps                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <t1>                          timespec                          In       *
// *    First timestamp                                                        *
// *                                                                           *
// *  <t2>                          timespec                          In       *
// *    Second timestamp                                                       *
// *                                                                           *
// *  <tDiff>                       timespec                          Out      *
// *    Difference between t1 and t2                                           *
// *****************************************************************************

static void timeDiff (
   struct timespec t1,
   struct timespec t2,
   struct timespec &tDiff)

{
   if ( (t2.tv_nsec - t1.tv_nsec )  < 0 )
   {
      tDiff.tv_sec = t2.tv_sec - t1.tv_sec - 1;
      tDiff.tv_nsec = 1000000000 + t2.tv_nsec - t1.tv_nsec;
   }
   else
   {
      tDiff.tv_sec = t2.tv_sec - t1.tv_sec;
        tDiff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
   }
}
//*************************** End of timeDiff **********************************

// *****************************************************************************
// *                                                                           *
// * Function: upshiftString                                                   *
// *                                                                           *
// *  Convert all characters of a string to upper case.                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <inputString>                 const char *                      In       *
// *    is the string to be converted to upper case.                           *
// *                                                                           *
// *  <outputString>                char *                            Out      *
// *    passes back the converted string.                                      *
// *                                                                           *
// *****************************************************************************
inline static void upshiftString(
   const char * inputString,
   char       * outputString)
   
{

   while (*inputString != '\0')
   {
      *(outputString++) = toupper(*inputString);

      inputString++;
   }
   *outputString = '\0';

}
//************************* End of upshiftString *******************************

// *****************************************************************************
// *                                                                           *
// * Function: writeLog                                                        *
// *                                                                           *
// *  Logging for testing ODBC authentications.                                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <text>                        const char *                      In       *
// *    is the string to be written to the log file.                           *
// *                                                                           *
// *****************************************************************************
static void writeLog(const char * text)

{
}
//*************************** End of writeLog **********************************
