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
#include "ComUser.h"
#include "PrivMgrDefs.h"

// ****************************************************************************
// function: qiSubjectMatchesRole
//
// This function compares the subjectKey with the list of roles the current
// user has been granted.  If it matches one of the roles, return TRUE, 
// otherwise it returns FALSE.
// ****************************************************************************
NABoolean qiSubjectMatchesRole(uint32_t subjectKey)
{
  NAList <Int32> roleIDs(NULL);
  ComUser::getCurrentUserRoles(roleIDs);
  for (int i = 0; i < roleIDs.entries(); i++)
  {
    if (subjectKey = ComSecurityKey::generateHash(roleIDs[i]))
      return TRUE;
  }
  return FALSE;
}

// ****************************************************************************
// function: qiCheckForInvalidObject
//
// This function compares the list of query invalidate keys that changed to
// the list of invalidation keys associated with the object.
//
// If the changed invalidation key matches one of the keys for the object,
// return TRUE.  TRUE indicates that the object should be reloaded to pick up
// DDL or privilege changes.
// ****************************************************************************
NABoolean qiCheckForInvalidObject (const Int32 numInvalidationKeys,
                                   const SQL_QIKEY* invalidationKeys,
                                   const Int64 objectUID,
                                   const ComSecurityKeySet & objectKeys)
{
  NABoolean found = FALSE;
  ComQIActionType invalidationKeyType = COM_QI_INVALID_ACTIONTYPE;

  // check each invalidation key against objects in NATableDB or NARoutineDB 
  // cache
  for ( Int32 i = 0; i < numInvalidationKeys && !found; i++ )
  {
    invalidationKeyType = ComQIActionTypeLiteralToEnum( invalidationKeys[i].operation );
    Int32 numObjectKeys = objectKeys.entries();

    switch (invalidationKeyType)
    {
      // Indicates that the DDL of the object has changed. 
      case COM_QI_OBJECT_REDEF:
      // Indicates that the histogram statistics of the object has changed.
      case COM_QI_STATS_UPDATED:
      {
        if (invalidationKeys[i].ddlObjectUID == objectUID)
          found = TRUE;
        break;
      }

      // Scan the passed-in object keys to find any that match the subject, 
      // object, and key type. That is, the subject has the privilege
      // (invalidation key type) on the object or a column of the object.
      case COM_QI_OBJECT_SELECT:
      case COM_QI_OBJECT_INSERT:
      case COM_QI_OBJECT_DELETE:
      case COM_QI_OBJECT_UPDATE:
      case COM_QI_OBJECT_USAGE:
      case COM_QI_OBJECT_REFERENCES: 
      case COM_QI_OBJECT_EXECUTE:
        for (Int32 j = 0; j < numObjectKeys && !found; j++ )
        {
          ComSecurityKey keyValue = objectKeys[j];
          if ( ( invalidationKeys[i].revokeKey.object ==
                 keyValue.getObjectHashValue() )  &&
               ( invalidationKeyType ==
                 keyValue.getSecurityKeyType() ) )
          {
            if ( invalidationKeys[i].revokeKey.subject ==
                   keyValue.getSubjectHashValue() ||
                 qiSubjectMatchesRole(invalidationKeys[i].revokeKey.subject) )
              found = TRUE;
          }
        }
        break;
      case COM_QI_USER_GRANT_SPECIAL_ROLE:
      case COM_QI_USER_GRANT_ROLE:
      {
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
        break;
      }

      default:
        found = TRUE;
        break;
    }
  }
  return found;
}

// ****************************************************************************
// Function that builds query invalidation keys for privileges. A separate
// invalidation key is added for each granted DML privilege. 
//
// Types of keys available for privs:
//   OBJECT_IS_SCHEMA - not supported until we support schema level privs
//   OBJECT_IS_OBJECT - supported for granting privs to user
//   OBJECT_IS_COLUMN - not supported at this time
//   OBJECT_IS_SPECIAL_ROLE - key for PUBLIC authorization ID
//   SUBJECT_IS_USER - support for granting roles to user
//   SUBJECT_IS_ROLE - not supported until we grant roles to roles
//
// returns false is unable to build keys
// ****************************************************************************
bool buildSecurityKeys( const int32_t userID,
                        const int32_t granteeID,
                        const int64_t objectUID,
                        const PrivMgrCoreDesc &privs,
                        ComSecurityKeySet &secKeySet )
{
  if (privs.isNull())
    return true;

  NABoolean doDebug = (getenv("DBUSER_DEBUG") ? TRUE : FALSE);
  std::string  msg ("Method: buildSecurityKeys: ");
  if (doDebug)
  {
    printf("[DBUSER:%d] %s\n", (int) getpid(), msg.c_str());
    fflush(stdout);
  }

  // If public is the grantee, generate special security key
  // A user cannot be revoked from public
  if (ComUser::isPublicUserID(granteeID))
  {
    ComSecurityKey key(granteeID, ComSecurityKey::OBJECT_IS_SPECIAL_ROLE);
    if (doDebug)
    {
      NAString msg (key.print(granteeID, objectUID));
      printf("[DBUSER:%d]  (public) %s\n", (int) getpid(), msg.data());
      fflush(stdout);
    }

    if (key.isValid())
      secKeySet.insert(key);
    else
      return false;
  }

  // If the grantee is a role, generate a special security key
  // If the role is revoked from the user, this key takes affect
  if (PrivMgr::isRoleID(granteeID))
  {
    ComSecurityKey key (userID, granteeID, ComSecurityKey::SUBJECT_IS_USER);
    if (doDebug)
    {
      NAString msg = key.print(userID, granteeID);
      printf("[DBUSER:%d]   (role) %s\n",
             (int) getpid(), msg.data());
      fflush(stdout);
    }

    if (key.isValid())
     secKeySet.insert(key);
    else
      return false;
  }

  // Generate object invalidation keys
  // Only need to generate keys for DML privileges
  for ( size_t i = FIRST_DML_PRIV; i <= LAST_DML_PRIV; i++ )
  {
    if ( privs.getPriv(PrivType(i)))
    {
      ComSecurityKey key (granteeID, objectUID, PrivType(i), 
                          ComSecurityKey::OBJECT_IS_OBJECT);
      if (doDebug)
      {
        NAString msg = key.print(granteeID, objectUID);
        printf("[DBUSER:%d]   (DML)%s\n",
               (int) getpid(), msg.data());
        fflush(stdout);
      }

      if (key.isValid())
       secKeySet.insert(key);
      else
        return false;
    }
  }

  return true;
}

// ****************************************************************************
// Function that returns the types of invalidation to perform
//   For DDL invalidation keys, always need to update caches
//   For security invalidation keys
//      update caches if key is for an object revoke from the current user or
//        current users roles
//      reset list of roles if key is a revoke role from the current user
//
// returns:
//    resetRoleList -- need to reset the list of roles for the current user
//    updateCaches -- need to update cache entries related for the keys
// ****************************************************************************
void qiInvalidationType (const Int32 numInvalidationKeys,
                         const SQL_QIKEY* invalidationKeys,
                         const Int32 userID,
                         bool &resetRoleList,
                         bool &updateCaches)
{
  NABoolean doDebug = (getenv("DBUSER_DEBUG") ? TRUE : FALSE);
  char buf[100];

  resetRoleList = false;
  updateCaches = false;
  ComQIActionType invalidationKeyType = COM_QI_INVALID_ACTIONTYPE;

  // Have the ComSecurityKey constructor compute the hash value for the the User's ID.
  // Note: The following code doesn't care about the object's hash value or the resulting 
  // ComSecurityKey's ActionType....we just need the hash value for the User's ID.
  // Perhaps a new constructor would be good (also done in RelRoot::checkPrivileges)
  uint32_t userHashValue = ComSecurityKey::generateHash(userID);

  if (doDebug)
  {
    sprintf(buf, ": num keys(%d)", numInvalidationKeys);
    printf("[DBUSER:%d] Method: qiInvalidationType%s\n",
           (int) getpid(), buf);
    fflush(stdout);
    sprintf(buf, "Not applicable");
  }

  for ( Int32 i = 0; i < numInvalidationKeys; i++ )
  {
    invalidationKeyType = ComQIActionTypeLiteralToEnum( invalidationKeys[i].operation );
    switch (invalidationKeyType)
    {
      // Object changed, need to update caches
      case COM_QI_OBJECT_REDEF:
      case COM_QI_STATS_UPDATED:
        if (doDebug)
          sprintf(buf, "object/stats, operation: %c%c, objectUID: %ld",
                  invalidationKeys[i].operation[0],
                  invalidationKeys[i].operation[1],
                  invalidationKeys[i].ddlObjectUID);

        updateCaches = true;
        break;

      // Privilege changed on an object, need to update caches if
      // any QI keys are associated with the current user
      case COM_QI_OBJECT_SELECT:
      case COM_QI_OBJECT_INSERT:
      case COM_QI_OBJECT_DELETE:
      case COM_QI_OBJECT_UPDATE:
      case COM_QI_OBJECT_USAGE:
      case COM_QI_OBJECT_REFERENCES:
      case COM_QI_OBJECT_EXECUTE:
        // If the current user matches the revoke subject, update
        if (invalidationKeys[i].revokeKey.subject == userHashValue)
        {
          if (doDebug)
            sprintf(buf, "user: %d, operation: %c%c, subject: %u, object: %u", userID,
                    invalidationKeys[i].operation[0], invalidationKeys[i].operation[1],
                    invalidationKeys[i].revokeKey.subject, invalidationKeys[i].revokeKey.object);

          updateCaches = true;
        }

        // If one of the users roles matches the revokes subject, update
        else if (qiSubjectMatchesRole(invalidationKeys[i].revokeKey.subject))
        {
          if (doDebug)
            sprintf(buf, "role: %d, operation: %c%c, subject: %u, object: %u", userID,
                    invalidationKeys[i].operation[0], invalidationKeys[i].operation[1],
                    invalidationKeys[i].revokeKey.subject, invalidationKeys[i].revokeKey.object);

          updateCaches = true;
       }
        break;

      // For public user (SPECIAL_ROLE), the subject is a special hash
      case COM_QI_USER_GRANT_SPECIAL_ROLE:
        if (invalidationKeys[i].revokeKey.subject == ComSecurityKey::SPECIAL_SUBJECT_HASH)
        {
          if (doDebug)
            sprintf(buf, "user: %d, operation: %c%c, subject: %u, object: %u", userID,
                    invalidationKeys[i].operation[0], invalidationKeys[i].operation[1],
                    invalidationKeys[i].revokeKey.subject, invalidationKeys[i].revokeKey.object);
          updateCaches = true;
        }
        break;

      // A revoke role from a user was performed.  Need to reset role list
      // if QI key associated with the current user and remove any plans
      // that include the role key
      case COM_QI_USER_GRANT_ROLE:
        if (invalidationKeys[i].revokeKey.subject == userHashValue)
        {
          if (doDebug)
            sprintf(buf, "user: %d, operation: %c%c, subject: %u, object: %u", userID,
                    invalidationKeys[i].operation[0], invalidationKeys[i].operation[1],
                    invalidationKeys[i].revokeKey.subject, invalidationKeys[i].revokeKey.object);

          resetRoleList = true;
          updateCaches = true;
        }
        break;

      // If a role was granted, refresh the active role llist
      case COM_QI_GRANT_ROLE:
       if (doDebug)
          sprintf(buf, "operation: %c%c, subject: %u, object: %u",
                  invalidationKeys[i].operation[0], invalidationKeys[i].operation[1],
                  invalidationKeys[i].revokeKey.subject, invalidationKeys[i].revokeKey.object);

        resetRoleList = true;
        break;

      // unknown key type, search and update cache (should not happen)
      default:
       if (doDebug)
          sprintf(buf, "user: %d, operation: %c%c, subject: %u, object: %u", userID,
                  invalidationKeys[i].operation[0], invalidationKeys[i].operation[1],
                  invalidationKeys[i].revokeKey.subject, invalidationKeys[i].revokeKey.object);
        resetRoleList = true;
        updateCaches = true;
        break;
    }
    if (doDebug)
    {
      printf("[DBUSER:%d]   %s\n",
             (int) getpid(), buf);
      fflush(stdout);
    }
  } 
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
    else if (typeOfSubject == SUBJECT_IS_GRANT_ROLE)
      actionType_ = COM_QI_GRANT_ROLE;
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
      subjectHash_ = SPECIAL_SUBJECT_HASH;
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
uint32_t ComSecurityKey::generateHash(int64_t hashInput)
{
  uint32_t hashResult = ExHDPHash::hash8((char*)&hashInput, ExHDPHash::NO_FLAGS);
  return hashResult;
}

// Generate hash value based on authorization ID
uint32_t ComSecurityKey::generateHash(int32_t hashID)
{
  int64_t hVal = (int64_t) hashID;
  uint32_t hashResult = generateHash(hVal);
  return hashResult;
}

void ComSecurityKey::getSecurityKeyTypeAsLit (std::string &actionString) const
{ 
  switch(actionType_)
  { 
    case COM_QI_GRANT_ROLE:
      actionString = COM_QI_GRANT_ROLE_LIT;
      break;
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
      actionString = COM_QI_USER_GRANT_SPECIAL_ROLE_LIT;
      break;
    default:
      actionString = COM_QI_INVALID_ACTIONTYPE_LIT;
    }
}

NAString ComSecurityKey::print(Int32 subjectID, Int64 objectID)
{
  std::string typeString;
  switch(actionType_)
  {
    case COM_QI_GRANT_ROLE:
      typeString = "GRANT_ROLE";
      break;
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
  char buf[200];
  sprintf (buf, " - subjectHash: %u (%d), objectHash: %u (%ld), type: %s",
           subjectHash_, subjectID,
           objectHash_, objectID,
           typeString.data());
  NAString keyDetails = buf;
  return keyDetails;
}


