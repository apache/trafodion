/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/


/*
 *******************************************************************************
 *
 * File:         CmpSeabaseDDLauth.h
 * Description:  This file describes the DDL classes for Trafodion authorization
 *               
 * Contents:
 *   class CmpSeabaseDDLauth   
 *   class CmpSeabaseDDLuser
 *
 *****************************************************************************
*/
#ifndef _CMP_SEABASE_DDL_AUTH_H_
#define _CMP_SEABASE_DDL_AUTH_H_

#include "CmpDDLCatErrorCodes.h"

class StmtDDLRegisterUser;
class StmtDDLAlterUser;

// ----------------------------------------------------------------------------
// class:  CmpSeabaseDDLauth
//
// Class that manages authorization IDs
//
// Authorization IDs consist of user, roles, and groups
// ----------------------------------------------------------------------------
class CmpSeabaseDDLauth  
{
   public:

     CmpSeabaseDDLauth ();

     Int32 getAuthDetails(const char *pAuthName, bool isExternal = false);
     Int32 getAuthDetails(Int32 authID);

     // accessors
     Int32          getAuthCreator() const    { return authCreator_; }
     ComTimestamp   getAuthCreateTime() const {return authCreateTime_;}
     const NAString getAuthDbName() const     { return authDbName_; }
     const NAString getAuthExtName() const    { return authExtName_; }
     Int32          getAuthID() const         { return authID_; }
     ComTimestamp   getAuthRedefTime() const  { return authRedefTime_; }
     ComIdClass     getAuthType() const       { return authType_; }

     bool  isAuthImmutable() const { return authImmutable_; }
     bool  isAuthValid() const     { return authValid_; }
     bool  isPublic() const        { return false; }
     bool  isRole()   const        { return authType_ == COM_ROLE_CLASS; }
     bool  isUser()   const        { return authType_ == COM_USER_CLASS; }

     virtual bool describe (const NAString &authName, NAString &authText) = 0;

     // mutators
     void setAuthCreator      (const Int32 authCreator)
       {authCreator_ = authCreator;}
     void setAuthCreateTime   (const ComTimestamp authCreateTime)
       { authCreateTime_ = authCreateTime;}
     void setAuthDbName       (const NAString &authDbName)
       {authDbName_=authDbName;}
     void setAuthExtName      (const NAString &authExtName)
       {authExtName_=authExtName;}
     void setAuthID           (const Int32 authID)
       {authID_ = authID;}
     void setAuthImmutable    (bool isImmutable)
       {authImmutable_ = isImmutable;}
     void setAuthRedefTime    (const ComTimestamp authRedefTime)
       { authRedefTime_ = authRedefTime;}
     void setAuthType        (ComIdClass authType)
       {authType_ = authType;}
     void setAuthValid       (bool isValid)
       {authValid_ = isValid;}

 protected:

    bool authExists      (bool isExternal = false);
    bool isAuthNameReserved(const NAString &authName);
    void verifyAuthority (void);

    void deleteRow      (const NAString &authName);
    void insertRow      (void);
    bool selectExactRow (Int32 authID);
    bool selectExactRow (const NAString &cmd); 

 private:

    Int32                  authCreator_;
    ComTimestamp           authCreateTime_;
    NAString               authDbName_;
    NAString               authExtName_;
    Int32                  authID_;
    bool                   authImmutable_;
    ComTimestamp           authRedefTime_;
    ComIdClass             authType_;
    bool                   authValid_;

};

// ----------------------------------------------------------------------------
// class:  CmpSeabaseDDLuser
//
// Class that manages user authorization IDs
//
// Child class of CmpSeabaseDDLauth
// ----------------------------------------------------------------------------
class CmpSeabaseDDLuser : public CmpSeabaseDDLauth
{
   public:

     CmpSeabaseDDLuser ();

     // Execute level methods
     void alterUser(StmtDDLAlterUser * pNode);
     void registerUser(StmtDDLRegisterUser * pNode);
     void unregisterUser(StmtDDLRegisterUser * pNode);
     
     Int32 getUserDetails(const char *pUserName, bool isExternal = false);
     Int32 getUserDetails(Int32 userID);

     bool describe (const NAString &authName, NAString &authText);

   protected:

      Int32     getUniqueUserID (void);
};
#endif
