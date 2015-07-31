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

// ==========================================================================
// Contains non inline methods in the following classes
//   ComSecurityKey
// Contains helper function: qiCheckForInvalidKeys
// ==========================================================================

#include "ComSecurityKey.h"
#include <bitset>
#include <string>
#include <vector>
#include "exp_function.h"
#include "ComDistribution.h"

NABoolean qiCheckForInvalidObject (const Int32 numInvalidationKeys,
                                   const SQL_QIKEY* invalidationKeys,
                                   const Int64 objectUID,
                                   const ComSecurityKeySet objectKeys)
{
  NABoolean found = FALSE;
  ComQIActionType invalidationKeyType = COM_QI_INVALID_ACTIONTYPE;

  // check each invalidation key against objects in NATableDB or NARoutineDB 
  // cache
  for ( Int32 i = 0; i < numInvalidationKeys && !found; i++ )
  {
    invalidationKeyType = ComQIActionTypeLiteralToEnum( invalidationKeys[i].operation );

    // if the object redef time changed and this is the object, remove entry
    if (invalidationKeyType == COM_QI_OBJECT_REDEF &&
        invalidationKeys[i].ddlObjectUID == objectUID)
      found = TRUE;
    else
    {
      // Scan the passed-in object keys to find any that match the 
      // invalidation key 
      Int32 numObjectKeys = objectKeys.entries();
      for (Int32 j = 0; j < numObjectKeys && !found; j++ )
      {
        ComSecurityKey keyValue = objectKeys[j];
        if ( ( invalidationKeys[i].revokeKey.subject ==
                 keyValue.getSubjectHashValue() ) &&
           ( invalidationKeys[i].revokeKey.object ==
                 keyValue.getObjectHashValue() )  &&
           ( invalidationKeyType ==
                 keyValue.getSecurityKeyType() ) ) 
          found = TRUE;
      }
    }
  }
  return found;
}

// *****************************************************************************
//    ComSecurityKey methods
// *****************************************************************************
// Constructor for privilege grant on a SQL object to an authID
ComSecurityKey::ComSecurityKey(const int32_t subjectUserID, 
                               const int64_t objectUID,
                               const PrivType which, 
                               const QIType typeOfObject) 
:
  subjectHash_(0),
  objectHash_(0),
  actionType_(COM_QI_INVALID_ACTIONTYPE)
  {
    // Set the type.
    actionType_ = convertBitmapToQIActionType(which, typeOfObject);

    if (actionType_ != COM_QI_INVALID_ACTIONTYPE)
    {
      // This is a valid key.  Set the subject and object values.

      // Authorization ID of the grantee
      // Hash value of the authorization ID of the subject
      subjectHash_ = generateHash(subjectUserID);
      // Hash value of the UID of the object
      objectHash_ = generateHash(objectUID);
    }
  }

ComSecurityKey::ComSecurityKey(
  const int32_t subjectUserID, 
  const int64_t objectUserID,
  const QIType typeOfSubject) 
:
  subjectHash_(0),
  objectHash_(0),
  actionType_(COM_QI_INVALID_ACTIONTYPE)
  {
    if (typeOfSubject == SUBJECT_IS_USER)
      actionType_ = COM_QI_USER_GRANT_ROLE;  // revoke role <object> from <user subject>
    else
      actionType_ = COM_QI_ROLE_GRANT_ROLE;  

    if (actionType_ != COM_QI_INVALID_ACTIONTYPE)
    {
      // This is a valid key.  Set the subject and object values.

      // Authorization ID of the grantee
      // Hash value of the authorization ID of the subject
      subjectHash_ = generateHash(subjectUserID);

      // Hash value of the authorization ID of the object
      objectHash_ = generateHash(objectUserID);
    }
  }

// Constructor for a special role grant to an authID.
ComSecurityKey::ComSecurityKey(
  const int32_t subjectUserID, 
  const QIType typeOfObject ) 
:
  subjectHash_(0),
  objectHash_(0),
  actionType_(COM_QI_INVALID_ACTIONTYPE)
  {
    // Set the values
    if (typeOfObject == OBJECT_IS_SPECIAL_ROLE)
    {
      actionType_ = COM_QI_USER_GRANT_SPECIAL_ROLE;
      subjectHash_ = generateHash(subjectUserID);
      objectHash_ = SPECIAL_OBJECT_HASH;
    }
  }

ComSecurityKey::ComSecurityKey() :
  subjectHash_(0),
  objectHash_(0),
  actionType_(COM_QI_INVALID_ACTIONTYPE)
{};

bool ComSecurityKey::operator == (const ComSecurityKey &other) const
{
  if((subjectHash_ == other.subjectHash_)&&
     (objectHash_ == other.objectHash_) &&
     (actionType_ == other.actionType_))
    return true;

  return false;
}

ComQIActionType ComSecurityKey::convertBitmapToQIActionType (
  const PrivType which,
  const QIType inputType) const
{
  // Convert from a PrivType value into a security key action type
  // Note that this does not cover grant or revoke role
  // Only converts values that are applicable to query invalidation.  Returns an 
  // invalid value for all other privileges.

  // Need to differentiate between schema, object, and column
  ComQIActionType result = COM_QI_INVALID_ACTIONTYPE;

  switch(which)
  {
    case SELECT_PRIV:
      if (inputType == OBJECT_IS_OBJECT)
        result = COM_QI_OBJECT_SELECT;
      //else 
      //  result = COM_QI_COLUMN_SELECT;
      break;
    case INSERT_PRIV:
      if (inputType == OBJECT_IS_OBJECT)
        result = COM_QI_OBJECT_INSERT;
      //else 
      //  result = COM_QI_COLUMN_INSERT;
      break;
    case DELETE_PRIV:
      if (inputType == OBJECT_IS_OBJECT)
        result = COM_QI_OBJECT_DELETE;
      break;
    case UPDATE_PRIV:
      if (inputType == OBJECT_IS_OBJECT)
        result = COM_QI_OBJECT_UPDATE;
      //else 
      //  result = COM_QI_COLUMN_UPDATE;
      break;
    case USAGE_PRIV:
      if (inputType == OBJECT_IS_OBJECT)
        result = COM_QI_OBJECT_USAGE;
      break;
    case REFERENCES_PRIV:
      if (inputType == OBJECT_IS_OBJECT)
        result = COM_QI_OBJECT_REFERENCES;
      break;
    case EXECUTE_PRIV:  
      if (inputType == OBJECT_IS_OBJECT)
        result = COM_QI_OBJECT_EXECUTE;
      else if (inputType == OBJECT_IS_SCHEMA)
        result = COM_QI_SCHEMA_EXECUTE;
      break;
    default:
      result = COM_QI_INVALID_ACTIONTYPE;
      break;
  };

return (result);
}

// Basic method to generate hash values
uint32_t ComSecurityKey::generateHash(int64_t hashInput) const
{
  uint32_t hashResult = ExHDPHash::hash8((char*)&hashInput, ExHDPHash::NO_FLAGS);
  return hashResult;
}

// Generate hash value based on authorization ID
uint32_t ComSecurityKey::generateHash(int32_t hashID) const
{
  int64_t hVal = (int64_t) hashID;
  uint32_t hashResult = generateHash(hVal);
  return hashResult;
}

void ComSecurityKey::getSecurityKeyTypeAsLit (std::string &actionString) const
{ 
  switch(actionType_)
  { 
    case COM_QI_USER_GRANT_ROLE:
      actionString = COM_QI_USER_GRANT_ROLE_LIT;
      break;
    case COM_QI_ROLE_GRANT_ROLE:
      actionString = COM_QI_ROLE_GRANT_ROLE_LIT;
      break;
    case COM_QI_OBJECT_SELECT:
      actionString = COM_QI_OBJECT_SELECT_LIT;
      break;
    case COM_QI_OBJECT_INSERT:
      actionString = COM_QI_OBJECT_INSERT_LIT;
      break;
    case COM_QI_OBJECT_DELETE:
      actionString = COM_QI_OBJECT_DELETE_LIT;
      break;
    case COM_QI_OBJECT_UPDATE:
      actionString = COM_QI_OBJECT_UPDATE_LIT;
      break;
    case COM_QI_OBJECT_USAGE:
      actionString = COM_QI_OBJECT_USAGE_LIT;
      break;
    case COM_QI_OBJECT_REFERENCES:
      actionString = COM_QI_OBJECT_REFERENCES_LIT;
      break;
    case COM_QI_SCHEMA_SELECT:
      actionString = COM_QI_SCHEMA_SELECT_LIT;
      break;
    case COM_QI_SCHEMA_INSERT:
      actionString = COM_QI_SCHEMA_INSERT_LIT;
      break;
    case COM_QI_SCHEMA_DELETE:
      actionString = COM_QI_SCHEMA_DELETE_LIT;
      break;
    case COM_QI_SCHEMA_UPDATE:
      actionString = COM_QI_SCHEMA_UPDATE_LIT;
      break;
    case COM_QI_OBJECT_EXECUTE:
      actionString = COM_QI_OBJECT_EXECUTE_LIT;
      break;
    case COM_QI_SCHEMA_EXECUTE:
      actionString = COM_QI_SCHEMA_EXECUTE_LIT;
      break;
    case COM_QI_USER_GRANT_SPECIAL_ROLE:
      actionString = COM_QI_USER_GRANT_SPECIAL_ROLE;
      break;
    default:
      actionString = COM_QI_INVALID_ACTIONTYPE_LIT;
    }
}

void ComSecurityKey::print() const
{
  std::string typeString;
  switch(actionType_)
  {
    case COM_QI_USER_GRANT_ROLE:
      typeString = "USER_GRANT_ROLE";
      break;
    case COM_QI_ROLE_GRANT_ROLE:
      typeString = "ROLE_GRANT_ROLE";
      break;
    case COM_QI_SCHEMA_SELECT:
      typeString = "SCHEMA_SELECT";
      break;
    case COM_QI_SCHEMA_INSERT:
      typeString = "SCHEMA_INSERT";
      break;
    case COM_QI_SCHEMA_DELETE:
      typeString = "SCHEMA_DELETE";
      break;
    case COM_QI_SCHEMA_UPDATE:
      typeString = "SCHEMA_UPDATE";
      break;
    case COM_QI_OBJECT_SELECT:
      typeString = "OBJECT_SELECT";
      break;
    case COM_QI_OBJECT_INSERT:
      typeString = "OBJECT_INSERT";
      break;
    case COM_QI_OBJECT_DELETE:
      typeString = "OBJECT_DELETE";
      break;
    case COM_QI_OBJECT_UPDATE:
      typeString = "OBJECT_UPDATE";
      break;
    case COM_QI_OBJECT_USAGE:
      typeString = "OBJECT_USAGE";
      break;
    case COM_QI_OBJECT_REFERENCES:
      typeString = "OBJECT_REFERENCES";
      break;
    default:
      typeString = "INVALID_ACTIONTYPE";
      break;
  };
  cout << subjectHash_  << " : " << objectHash_ << " : " << typeString << endl;
}


