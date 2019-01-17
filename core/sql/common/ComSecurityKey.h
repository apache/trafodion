//*****************************************************************************
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
//*****************************************************************************

#ifndef COMSECURITYKEY_H
#define COMSECURITYKEY_H

#include "PrivMgrDefs.h"
#include "ComSmallDefs.h"
#include "Collections.h"
#include "sqlcli.h"
#include "PrivMgrCommands.h"
#include "PrivMgrDesc.h"

class PrivMgrUserPrivs;

class ComSecurityKey;

typedef NASet<ComSecurityKey>  ComSecurityKeySet;

bool buildSecurityKeys( const NAList <Int32> &roleGrantees,
                        const int32_t roleID,
                        const int64_t objectUID,
                        const bool isColumn,
                        const PrivMgrCoreDesc &privs,
                        ComSecurityKeySet &secKeySet);

NABoolean qiCheckForInvalidObject (const Int32 numInvalidationKeys, 
                                   const SQL_QIKEY* invalidationKeys, 
                                   const Int64 objectUID,
                                   const ComSecurityKeySet & objectKeys);

void qiInvalidationType (const Int32 numInvalidationKeys,
                         const SQL_QIKEY* invalidationKeys,
                         const Int32 userID,
                         bool &resetRoleList,
                         bool &updateCaches);

NABoolean qiSubjectMatchesRole(uint32_t subjectKey);

// ****************************************************************************
// Class:  ComSecurityKey 
//   Represents a key describing a change that will effect query and compiler
//   cache
//
// members:
//   subject = hash value describing the effected user or role
//   object = hash value describing the effected object
//   action type = type of operation
//***************************************************************************** 


class ComSecurityKey
{
public:

  // QIType indicates the type of entity
  enum QIType {
    INVALID_TYPE = 0,
    OBJECT_IS_SCHEMA,
    OBJECT_IS_OBJECT,
    OBJECT_IS_COLUMN,
    SUBJECT_IS_USER,
    SUBJECT_IS_ROLE,
    OBJECT_IS_SPECIAL_ROLE,
    SUBJECT_IS_GRANT_ROLE
  };

  // QISpecialHashValues are used for security keys for special roles
  enum QISpecialHashValues {
    SPECIAL_SUBJECT_HASH = -1,
    SPECIAL_OBJECT_HASH = -1
  };

  // Constructor for privilege grant on a SQL object to an authID
  ComSecurityKey(
    const int32_t subjectUserID, 
    const int64_t objectUID, 
    const PrivType which, 
    const QIType typeOfObject);

  // Constructor for a role grant to an authID.  Currently only supporting grants of roles to users.
  ComSecurityKey(
   const int32_t subjectUserID, 
   const int64_t objectUserID, 
   const QIType typeOfSubject);

  // Constructor for a special role grant to an authID.
  ComSecurityKey(
    const int32_t subjectUserID, 
    const QIType typeOfObject);

  // Constructor for generating revoke role from subject
  ComSecurityKey(
    const uint32_t subjectHashValue,
    const uint32_t objectHashValue)
  : subjectHash_(subjectHashValue),
    objectHash_ (objectHashValue),
    actionType_(COM_QI_USER_GRANT_ROLE)
  {};

  ComSecurityKey();  // do not use
  bool operator == (const ComSecurityKey &other) const;
   
 // Accessors

  uint32_t getSubjectHashValue() const
  {return subjectHash_;}
  uint32_t getObjectHashValue() const
  {return objectHash_;}
  ComQIActionType getSecurityKeyType() const
  {return actionType_;}

  void getSecurityKeyTypeAsLit (std::string &actionString) const;
  bool isValid() const
 {
    if (actionType_ == COM_QI_INVALID_ACTIONTYPE)
      return false;

    return true;
  }

  ComQIActionType convertBitmapToQIActionType(const PrivType which, const QIType inputType) const;

  // Basic method to generate hash values
  static uint32_t generateHash(int64_t hashInput);
  // Generate hash value based on authorization ID
  static uint32_t generateHash(int32_t hashID);

  // For debugging purposes
  NAString print(Int32 subject, Int64 object);

private:
  uint32_t subjectHash_ ;
  uint32_t objectHash_;
  ComQIActionType actionType_;

}; // class ComSecurityKey

#endif

