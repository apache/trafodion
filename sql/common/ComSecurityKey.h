//*****************************************************************************
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
//*****************************************************************************

#ifndef COMSECURITYKEY_H
#define COMSECURITYKEY_H

#include "PrivMgrDefs.h"
#include "ComSmallDefs.h"
#include "Collections.h"

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
    OBJECT_IS_SPECIAL_ROLE
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
   const QIType typeOfSubject = SUBJECT_IS_USER);

  // Constructor for a special role grant to an authID.
  ComSecurityKey(
    const int32_t subjectUserID, 
    const QIType typeOfObject);

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
  uint32_t generateHash(int64_t hashInput) const;
  // Generate hash value based on authorization ID
  uint32_t generateHash(int32_t hashID) const;

  // For debugging purposes
  void print() const ;

private:
  uint32_t subjectHash_ ;
  uint32_t objectHash_;
  ComQIActionType actionType_;

}; // class ComSecurityKey

typedef NASet<ComSecurityKey>  ComSecurityKeySet;

#endif

