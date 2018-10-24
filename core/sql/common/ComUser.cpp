/* -*-C++-*-
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

 *****************************************************************************
 *
 * File:         ComUser
 * Description:  Implements methods for user management
 *
 * Contains methods for classes:
 *   ComUser
 *   ComUserVerifyObj
 *   ComUserVerifyAuth
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS        // must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "ComUser.h"
#include "CmpSeabaseDDL.h"
#include "CmpDDLCatErrorCodes.h"

#include "sqlcli.h"
#include "SQLCLIdev.h"
#include "ExExeUtilCli.h"
#include "Context.h"

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class ComUser methods
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ----------------------------------------------------------------------------
// Constructors
// ----------------------------------------------------------------------------
ComUser::ComUser()
{ }

// ----------------------------------------------------------------------------
// method:  getCurrentUser
//
// Returns currentUser from the Cli context
// ----------------------------------------------------------------------------
Int32 ComUser::getCurrentUser(void)
{
  Int32 dbUserID = 0;
  Int32 rc = 0;

  SQL_EXEC_ClearDiagnostics(NULL);

  rc = SQL_EXEC_GetSessionAttr(SESSION_DATABASE_USER_ID,
                               &dbUserID,
                               NULL, 0, NULL);

  assert(rc >= 0);
  assert(dbUserID >= SUPER_USER);

  return dbUserID;
  
}

// ----------------------------------------------------------------------------
// method:  getCurrentUsername
//
// Returns size of username
// ----------------------------------------------------------------------------
bool ComUser::getCurrentUsername(
   char * username,
   Int32 maxUsernameLength)
   
{

   SQL_EXEC_ClearDiagnostics(NULL);

Int32 usernameLength = 0;

Int32 rc = SQL_EXEC_GetSessionAttr(SESSION_DATABASE_USER_ID,NULL,
                                   username,maxUsernameLength,&usernameLength);

   assert(rc >= 0);

   if (usernameLength > maxUsernameLength)
      return false;
     
   return true;

}

// ----------------------------------------------------------------------------
// method:  getCurrentUsername
//
// Returns a pointer to the current database username
// ----------------------------------------------------------------------------
const char * ComUser::getCurrentUsername()
   
{

   return GetCliGlobals()->currContext()->getDatabaseUserName();
   
}

// ----------------------------------------------------------------------------
// method:  getSessionUser
//
// Returns sessionUser from the Cli context
// ----------------------------------------------------------------------------
Int32 ComUser::getSessionUser(void)
{
  Int32 dbUserID = 0;
  Int32 rc = 0;

  SQL_EXEC_ClearDiagnostics(NULL);

  rc = SQL_EXEC_GetSessionAttr(SESSION_DATABASE_USER_ID,
                               &dbUserID,
                               NULL, 0, NULL);

  assert(rc >= 0);
  assert(dbUserID >= SUPER_USER);

  return dbUserID;
}

// ----------------------------------------------------------------------------
// method:  isRootUserID
//
// Returns true if current userID is the root user
// ----------------------------------------------------------------------------
bool ComUser::isRootUserID()
{
  return isRootUserID(getCurrentUser());
}

// ----------------------------------------------------------------------------
// method:  isRootUserID
//
// Returns true if passed in userID is the root user
// ----------------------------------------------------------------------------
bool ComUser::isRootUserID(Int32 userID)
{
  return (userID == SUPER_USER) ? true : false;
}

// ----------------------------------------------------------------------------
// method: getAuthType
//
// returns the type of authorization ID given the passed in authID
// ----------------------------------------------------------------------------
char ComUser::getAuthType(Int32 authID)
{

   if (authID == PUBLIC_USER)
      return AUTH_PUBLIC;

   if (authID == SYSTEM_USER)
      return AUTH_SYSTEM;

   if (authID >= MIN_USERID && authID <= MAX_USERID)
      return AUTH_USER;

   if (authID >= MAX_USERID && authID <= MAX_ROLEID)
      return AUTH_ROLE;
   return AUTH_UNKNOWN;
}

// -----------------------------------------------------------------------
// getUserNameFromUserID
//
// Reads the USERS table to get the user name associated with the passed
// in userID
//
//   <userID>                        Int32                           In
//      is the numeric ID to be mapped to a name.
//
//   <userName>                      char *                          In
//      passes back the name that the numeric ID mapped to.  If the ID does
//      not map to a name, the ASCII equivalent of the ID is passed back.
//
//   <maxLen>                        Int32                           In
//      is the size of <authName>.
//
//   <actualLen>                     Int32 &                         Out
//      is the size of the auth name in the table.  If larger than <maxLen>,
//      caller needs to supply a larger buffer.
//
// Returns:
//   FEOK          -- Found. User name written to userName. Length
//                    returned in actualLen.
//   FENOTFOUND    -- Not found
//   FEBUFTOOSMALL -- Found but output buffer is too small. Required
//                    length returned in actualLen.
//   Other         -- Unexpected error
// -----------------------------------------------------------------------
Int16 ComUser::getUserNameFromUserID(Int32 userID,
                                     char *userName,
                                     Int32 maxLen,
                                     Int32 &actualLen)
{
  Int16 result = FEOK;
  actualLen = 0;

  Int16 retcode = SQL_EXEC_GetDatabaseUserName_Internal(userID,
                                                        userName,
                                                        maxLen,
                                                        &actualLen);

  if (retcode < 0)
  {
    if (actualLen > 0)
      result = FEBUFTOOSMALL;
    else
      result = FENOTFOUND;
  }

  if (retcode != 0)
    SQL_EXEC_ClearDiagnostics(NULL);

  // On success, CLI does not return the length of the string in
  // actualLen. This function however is supposed to return a value in
  // actualLen even when the mapping is successful. The value returned
  // should not account for a null terminator.
  if (result == FEOK)
    actualLen = strlen(userName);
  return result;
}

// ----------------------------------------------------------------------------
// method: getUserIDFromUserName
//
// Reads the AUTHS table to get the userID associated with the passed
// in user name
//
//  Returns:  FEOK -- found
//            FENOTFOUND -- user not defined
//            other -- unexpected error
// ----------------------------------------------------------------------------
Int16 ComUser::getUserIDFromUserName(const char *userName, Int32 &userID)
{
  Int16 result = FEOK;

  Int16 retcode = SQL_EXEC_GetDatabaseUserID_Internal((char *) userName,
                                                      &userID);

  if (retcode < 0)
    result = FENOTFOUND;

  if (retcode != 0)
    SQL_EXEC_ClearDiagnostics(NULL);

  return result;
}


// ----------------------------------------------------------------------------
// method: getAuthIDFromAuthName
//
// Reads the AUTHS table to get the authID associated with the passed
// in authorization name
//
//  Returns:  FEOK -- found
//            FENOTFOUND -- auth name not defined
//            other -- unexpected error
// ----------------------------------------------------------------------------
Int16 ComUser::getAuthIDFromAuthName(const char *authName, Int32 &authID)
{
  Int16 result = FEOK;

  Int16 retcode = SQL_EXEC_GetAuthID(authName,authID);

  if (retcode < 0)
    result = FENOTFOUND;

  if (retcode != 0)
    SQL_EXEC_ClearDiagnostics(NULL);

  return result;
}


// ----------------------------------------------------------------------------
//  method:      getAuthNameFromAuthID
//
//  Maps an integer authentication ID to a name.  If the number cannot be
//  mapped to a name, the numeric ID is converted to a string.
//
//   <authID>                        Int32                           In
//      is the numeric ID to be mapped to a name.
//
//   <authName>                      char *                          In
//      passes back the name that the numeric ID mapped to.  If the ID does
//      not map to a name, the ASCII equivalent of the ID is passed back.
//
//   <maxLen>                        Int32                           In
//      is the size of <authName>.
//
//   <actualLen>                     Int32 &                         Out
//      is the size of the auth name in the table.  If larger than <maxLen>,
//      caller needs to supply a larger buffer.
//
//   Returns: Int16
//
//     FEOK          -- Found. User name written to userName. Length
//                      returned in actualLen.
//     FENOTFOUND    -- Not found
//     FEBUFTOOSMALL -- Found but output buffer is too small. Required
//                      length returned in actualLen.
//     Other         -- Unexpected error
// ----------------------------------------------------------------------------
Int16 ComUser::getAuthNameFromAuthID(Int32   authID,
                                     char  * authName,
                                     Int32   maxLen,
                                     Int32 & actualLen)

{
  actualLen = 0;
  int retcode = SQL_EXEC_GetAuthName_Internal(authID,authName,maxLen,actualLen);

  if (retcode < 0)  // ERROR == -1
  {
    if (actualLen > 0)
      return FEBUFTOOSMALL;

    return FENOTFOUND; //Should not happen
  }

  if (retcode != 0)
    SQL_EXEC_ClearDiagnostics(NULL);

  return FEOK;
}


// ----------------------------------------------------------------------------
// method: currentUserHasRole
//
// Searches the list of roles stored for the user to see if passed in role ID
// is found
//
//  Returns:  true - role found
//            false - role not found
// ----------------------------------------------------------------------------
bool ComUser::currentUserHasRole(Int32 roleID)
{
  Int32 numRoles = 0;
  Int32 *roleIDs = 0;
  Int32 *granteeIDs = NULL;
  if (SQL_EXEC_GetRoleList(numRoles, roleIDs, granteeIDs) < 0)
    return false;

  for (Int32 i = 0; i < numRoles; i++)
  {
    Int32 userRole = roleIDs[i];
    if (userRole == roleID)
      return true;
  }
  return false;
}

// ----------------------------------------------------------------------------
// method: getCurrentUserRoles
//
// Gets the active roles for the current user as a list of Int32's
// There should be at least one role in this list (PUBLIC)
//
// Returns:
//   -1 -- unexpected error getting roles
//    0 -- successful
// ----------------------------------------------------------------------------
Int16 ComUser::getCurrentUserRoles(NAList <Int32> &roleIDs)
{
  Int32 numRoles = 0;
  Int32 *cachedRoleIDs = NULL;
  Int32 *cachedGranteeIDs = NULL;
  RETCODE retcode =
    GetCliGlobals()->currContext()->getRoleList(numRoles, cachedRoleIDs, cachedGranteeIDs);

  if (retcode != SUCCESS)
    return -1;

  for (Int32 i = 0; i < numRoles; i++)
  {
    // in case there are duplicates
    if (!roleIDs.contains(cachedRoleIDs[i]))
      roleIDs.insert (cachedRoleIDs[i]);
  }
  return 0;
}

// ----------------------------------------------------------------------------
// method: getCurrentUserRoles
//
// Gets the active roles and grantees for the current user as a list of Int32's
// There should be at least one role in this list (PUBLIC)
//
// Returns:
//   -1 -- unexpected error getting roles
//    0 -- successful
// ----------------------------------------------------------------------------
Int16 ComUser::getCurrentUserRoles(NAList<Int32> &roleIDs, NAList<Int32> &granteeIDs)
{
  Int32 numRoles = 0;
  Int32 *cachedRoleIDs = NULL;
  Int32 *cachedGranteeIDs = NULL;
  RETCODE retcode =
    GetCliGlobals()->currContext()->getRoleList(numRoles, cachedRoleIDs, cachedGranteeIDs);

  if (retcode != SUCCESS)
    return -1;

  for (Int32 i = 0; i < numRoles; i++)
  {
    roleIDs.insert (cachedRoleIDs[i]);
    granteeIDs.insert (cachedGranteeIDs[i]);
  }

  return 0;
}


// ----------------------------------------------------------------------------
// method: getRoleList
//
// Returns the list of system roles
// Params:
//   (out) roleList - the list of roles, space is managed by the caller
//   (out) actualLen - the length of the returned role list
//   ( in) maxLen - the size of the roleList allocated by the caller
//   ( in) delimited - delimiter to use (defaults to single quote)
//   ( in) separator - specified what separator to use (defaults to comma)
//   ( in) includeSpecialAuths - includes the special auths (PUBLIC and _SYSTEM) 
//
//  Returns:  FEOK -- found
//            FEBUFTOOSMALL -- space allocated for role list is too small 
// ----------------------------------------------------------------------------
Int32 ComUser::getRoleList (char * roleList,
                            Int32 &actualLen,
                            const Int32 maxLen,
                            const char delimiter,
                            const char separator,
                            const bool includeSpecialAuths)
{
  Int32 numberRoles = sizeof(systemRoles)/sizeof(SystemAuthsStruct);
  Int32 roleListLen = (MAX_AUTHNAME_LEN*numberRoles)+(numberRoles * 4); // 4 = 2 del + 2 sep
  char generatedRoleList[roleListLen];
  char *pRoles = generatedRoleList;
  char roleName[MAX_AUTHNAME_LEN + 4];
  char currentSeparator = ' ';
  for (Int32 i = 0; i < numberRoles; i++)
  {
    const SystemAuthsStruct &roleDefinition = systemRoles[i];
    if (!includeSpecialAuths && roleDefinition.isSpecialAuth)
      continue;

    // str_sprintf does not support the %c format
    sprintf(roleName, "%c%c%s%c",
                currentSeparator, delimiter, roleDefinition.authName, delimiter);
    str_cpy_all(pRoles, roleName, sizeof(roleName)-1); // don't copy null terminator 
    currentSeparator = separator;
    pRoles = pRoles + strlen(roleName);
  }

  pRoles = '\0'; // null terminate string
  pRoles = generatedRoleList;
  actualLen = strlen(pRoles);
  if (actualLen > maxLen) 
    return FEBUFTOOSMALL;
 
  str_cpy_all(roleList, pRoles, strlen(pRoles));
  roleList[strlen(pRoles)] = 0; // null terminate string
  return FEOK;
}
