// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014 Hewlett-Packard Development Company, L.P.
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

#include "ComObjectName.h"
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

    // VerifyAction lists privilege grantable operations.  Privileges can be
    // granted to authorization IDs that allow these operations to be performed.
    enum VerifyAction { ANY
                      , SELECT
                      , INSERT
                      , DELETE_PRIV
                      , UPDATE
                      , USAGE
                      , REFERENCES
                      , EXECUTE
                      , CREATE
                      , CREATE_TABLE
                      , CREATE_VIEW
                      , CREATE_TRIGGER
                      , CREATE_PROCEDURE
                      , CREATE_ROUTINE
                      , CREATE_ROUTINE_ACTION
                      , CREATE_SYNONYM
                      , CREATE_LIBRARY
                      , ALTER
                      , ALTER_TABLE
                      , ALTER_TRIGGER
                      , ALTER_VIEW
                      , ALTER_SYNONYM
                      , ALTER_ROUTINE
                      , ALTER_ROUTINE_ACTION
                      , ALTER_LIBRARY
                      , DROP
                      , DROP_TABLE
                      , DROP_VIEW
                      , DROP_TRIGGER
                      , DROP_PROCEDURE
                      , DROP_ROUTINE
                      , DROP_ROUTINE_ACTION
                      , DROP_SYNONYM
                      , DROP_LIBRARY
                      , REGISTER_USER
                      };

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
     static Int32 getSessionUser();
     static bool isRootUserID();
     static bool isRootUserID(Int32 userID);
     static bool isPublicUserID(Int32 userID);
     static bool isSystemUserID(Int32 userID);
     static Int32 getRootUserID() { return SUPER_USER; }
     static Int32 getPublicUserID() { return PUBLIC_USER; }
     static Int32 getSystemUserID() { return SYSTEM_USER; }
     static const char * getRootUserName() { return "DB__ROOT"; }
     static const char * getPublicUserName() { return "PUBLIC"; }
     static const char * getSystemUserName() { return "_SYSTEM"; }
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

     // default constructor
     ComUser ();


     // accessors
     Int32 getEffectiveUserID(const ComUser::VerifyAction act = VerifyAction::ANY);
     Int16 getEffectiveUserName (NAString &userNameStr,
                                 const ComUser::VerifyAction act = VerifyAction::ANY);

     virtual bool  userHasPriv(ComUser::VerifyAction act);
     virtual bool  isAuthorized( VerifyAction authAction,
                                 bool trustedCaller = false);

 protected:

    bool belongsToCreateGroup(ComUser::VerifyAction authAction);
    Int32     getSchemaOwner() { return 33333; } //TBD

};

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// class:  ComUserVerifyObj
//
// Class that checks privileges granted on objects for authorization IDs
//
//  TBD - move this class functionality somewhere else as it really does not
//  belong as a child of ComUser
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class ComUserVerifyObj : public ComUser
{
  public:

     enum ObjType { UNKNOWN_OBJ_TYPE,
                    SYS_OBJ_TYPE,
                    COMP_OBJ_TYPE,
                    SCH_OBJ_TYPE,
                    OBJ_OBJ_TYPE };

    ComUserVerifyObj ();
    ComUserVerifyObj (const ComObjectName &objName,
                          ComUserVerifyObj::ObjType objType);

     const ComObjectName  getObjName() const { return objName_; }
     ObjType              getObjType() const { return objType_; }

     virtual bool   userHasPriv(ComUser::VerifyAction act);
     virtual bool   isAuthorized( VerifyAction authAction,
                                  bool trustedCaller = false);

  private:

    ComObjectName      objName_;
    ObjType            objType_;

    bool userOwnsObject(ComUser::VerifyAction act, Int32 objOwnerID);
};

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// class:  ComUserVerifyAuth
//
// Class that checks privileges granted to authorization requests such as
// REGISTER USER for authorization IDs
//
//  TBD - move this class functionality somewhere else as it really does not
//  belong as a child of ComUser
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class ComUserVerifyAuth : public ComUser
{

  public:

    enum AuthType { UNKNOWN_AUTH_TYPE,
                    USER_AUTH_TYPE,
                    ROLE_AUTH_TYPE };

    ComUserVerifyAuth ();
    ComUserVerifyAuth (const NAString &authName, ComUserVerifyAuth::AuthType authType);

    const NAString getAuthName () const { return authName_; }
    AuthType       getAuthType () const { return authType_; }

   virtual bool userHasPriv(ComUser::VerifyAction act);
   virtual bool isAuthorized(VerifyAction authAction,
                             bool trustedCaller = false);

  private:

    NAString     authName_;
    AuthType     authType_;

};

#endif
