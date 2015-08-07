#ifndef _DBUSERAUTH_H
#define _DBUSERAUTH_H
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
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include "auth.h"

enum AuthConfigType
{
   AuthUnknownConfiguration    = 9,
   AuthPrimaryConfiguration    = 10,
   AuthSecondaryConfiguration  = 11
};



enum  LDAPAuthResultEnum {
    AuthResult_Successful,       // User was authenticated on LDAP server
    AuthResult_Rejected,         // User was rejected by LDAP server
    AuthResult_ResourceError};   // An error prevented authentication

enum  LDAPCheckUserResultEnum {
    CheckUserResult_UserExists,       // User was found on LDAP server
    CheckUserResult_UserDoesNotExist, // User was not found on LDAP server
    CheckUserResult_ErrorInCheck};     // An error prevented checking, see error detail for reason


enum {
   MAX_EXTPASSWORD_LEN = 128,
   MAX_EXTUSERNAME_LEN = 128 
};

class DBUserAuthContents;


void cacheUserInfo(
   const char    * username,
   const char    * databaseUsername,
   const char    * logonRoleName,
   int32_t         userID,
   bool            isValid,
   AuthConfigType  authType,
   int64_t         redefTime);

class DBUserAuth 
{
public:

enum AuthenticationConfiguration
{
   DefaultConfiguration    = -2,
   PrimaryConfiguration    = 1,
   SecondaryConfiguration  = 2
};

enum  CheckUserResult {
   UserExists = 2,        // User was found on identity store
   UserDoesNotExist,      // User was not found on identity store
   ErrorDuringCheck};     // An error internal error occurred during checking

   static void CloseConnection();
   
   static LDAPAuthResultEnum AuthenticateExternalUser(
      const char * externalUsername,       
      const char * password);
      
   static CheckUserResult CheckExternalUsernameDefined(
      const char *                  externalUsername,          
      AuthenticationConfiguration   configurationOrdinal,    
      AuthenticationConfiguration & foundConfigurationOrdinal);   
      
   static void GetBlackBox(
      char *blackBox,
      bool  pretendTokenIsNull = false);
   
   static size_t GetBlackBoxSize(bool pretendTokenIsNull = false);
   static void FormatStatusMsg(
      UA_Status  status,
      char *     statusMsg);

   static void FreeInstance();
   
   static DBUserAuth * GetInstance();

   AuthFunction getAuthFunction();
   
   int32_t getUserID() const;
   
   void getExternalUsername(
      char * externalUsername,
      size_t maxLen) const;
      
   void getDBUserName(
      char    * databaseUsername,
      size_t    maxLen) const;

   void getTokenKeyAsString(char *tokenKeyString) const;
   
   size_t getTokenKeySize() const;  
   
   void setAuthFunction(AuthFunction somefunc);

   int verify(
      const char        * username,             
      char              * credentials,          
      UA_Status         & errorDetail, 
      AUTHENTICATION_INFO &authenticationInfo,  
      const CLIENT_INFO & client_info,
      PERFORMANCE_INFO  & performanceInfo);
      
   size_t verifyChild(
      const char * credentials,
      char       * blackbox);
                   
private:

   DBUserAuthContents &self;

   DBUserAuth();
   ~DBUserAuth();
   DBUserAuth(DBUserAuth &);

   AuthFunction validateTokenFunc;
};
#endif /* _DBUSERAUTH_H */

