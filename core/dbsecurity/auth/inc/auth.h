#ifndef _AUTH_H
#define _AUTH_H
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

#include <stdint.h>

#define USERTOKEN_ID_1 '\3'     // User token identifier, must be a sequence
#define USERTOKEN_ID_2 '\4'     // not allowed in password

#define DEFINERTOKEN_ID_1 '\5'  // Identifies that the password is generated
#define DEFINERTOKEN_ID_2 '\6'  // by UDR and contains the original token.

#define DEFINERTOKEN_NO_OF_ITEMS 5 

// Define a function pointer to be used by child MXOSRVRs to verify
// and fetch the authentication data from the parent MXOSRVR

typedef int (*AuthFunction) (char *, int, unsigned char *, int, int *, unsigned char *);

enum UA_Status{
   UA_STATUS_OK = 0,//Returned with error 0, user logged on, no warnings
   
   UA_STATUS_ERR_INVALID = 1,//Username or password is invalid
   UA_STATUS_ERR_SYSTEM = 20,//Resource or internal error occurred
  
   UA_STATUS_PARAM1 = 1,
   UA_STATUS_PARAM2 = 2,
   UA_STATUS_PARAM3 = 3,
   UA_STATUS_PARAM4 = 4,
   UA_STATUS_PARAM5 = 5
};

// Define a struct to populate the fields needed by authentication audit

typedef struct client_info
{
   char *client_name;
   char *client_user_name;
   char *application_name;
   client_info():client_name(0),
                 client_user_name(0),
                 application_name(0)
                 {};
} CLIENT_INFO;

typedef struct performance_info
{
   int64_t sqlUserTime;
   int64_t searchConnectionTime;
   int64_t searchTime;
   int64_t authenticationConnectionTime;
   int64_t authenticationTime;
   
} PERFORMANCE_INFO;

typedef struct users_info
{
   int32_t     effectiveUserID;
   int32_t     sessionUserID;
   char        externalUsername[257];  //ACH use string?
   char        databaseUsername[257];
   int64_t     redefTime;
   users_info():effectiveUserID(0),
                 sessionUserID(0),
                 redefTime(0LL)
                 { externalUsername[0] = databaseUsername[0] = 0; };
} USERS_INFO;

typedef struct authentication_info
{
   USERS_INFO  usersInfo;
   int         error;
   UA_Status   errorDetail;
   char        tokenKey[387];
   int32_t     tokenKeySize;
} AUTHENTICATION_INFO;

#endif /* _AUTH_H */

