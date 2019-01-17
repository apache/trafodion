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

/*
 *******************************************************************************
 *
 * File:         ComUser.h
 * Description:  This file describes classes associated with user management
 *
 * Contents:
 *   class ComUser
 *   class ComUserVerifyObj
 *   class ComUserVerifyAuth
 *
 *****************************************************************************
*/
#ifndef _COM_USER_H_
#define _COM_USER_H_

#include "NAUserId.h"
#include "ComSmallDefs.h"

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// class:  ComUser
//
// Class that manages authorization IDs
//
// Authorization IDs consist of users, roles, special IDs (PUBLIC and _SYSTEM),
// and groups
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class ComUser
{
   public:

    // AuthTypeChar lists the types of authorization IDs currently supported
    enum AuthTypeChar { AUTH_PUBLIC = 'P',
                        AUTH_SYSTEM = 'S',
                        AUTH_USER = 'U',
                        AUTH_ROLE = 'R',
                        AUTH_GROUP = 'G',
                        AUTH_UNKNOWN = 'Z'
                     };

     // static accessors
     static Int32 getCurrentUser();
     static bool getCurrentUsername(
                    char * username,
                    Int32 maxUsernameLength);
     static const char * getCurrentUsername();
     static Int32 getSessionUser();
     static bool isRootUserID();
     static bool isRootUserID(Int32 userID);
     static bool isPublicUserID(Int32 userID) {return (userID == PUBLIC_USER);};
     static bool isSystemUserID(Int32 userID) {return (userID == SYSTEM_USER);};
     static Int32 getRootUserID() { return SUPER_USER; }
     static Int32 getPublicUserID() { return PUBLIC_USER; }
     static Int32 getSystemUserID() { return SYSTEM_USER; }
     static const char * getRootUserName() { return DB__ROOT; }
     static const char * getPublicUserName() { return PUBLIC_AUTH_NAME; }
     static const char * getSystemUserName() { return SYSTEM_AUTH_NAME; }
     static char getAuthType(Int32 authID);
     static Int16 getUserNameFromUserID(Int32 userID,
                                        char *userName,
                                        Int32 maxLen,
                                        Int32 &actualLen);
     static Int16 getUserIDFromUserName(const char *userName,
                                        Int32 &userID);
     static Int16 getAuthNameFromAuthID (Int32   authID,
                                         char  * authName,
                                         Int32   maxLen,
                                         Int32 & actualLen);
     static Int16 getAuthIDFromAuthName (const char  * authName,
                                         Int32 & authID);

     static bool currentUserHasRole(Int32 roleID);
     static Int16 getCurrentUserRoles(NAList <Int32> &roleIDs);
     static Int16 getCurrentUserRoles(NAList <Int32> &roleIDs,
                                     NAList <Int32> &granteeIDs);


     static Int32 getRoleList (char *roleList,
                               Int32 &actualLen,
                               const Int32 maxLen,
                               const char delimiter = '\'',
                               const char separator =',',
                               const bool includeSpecialAuths = false);
private:
   // default constructor
   ComUser ();

};

#endif
