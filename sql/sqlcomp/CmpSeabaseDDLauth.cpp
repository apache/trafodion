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

// *****************************************************************************
// *
// * File:         CmpSeabaseDDLauth.cpp
// * Description:  Implements methods for user management
// *
// * Contains methods for classes:
// *   CmpSeabaseDDLauth
// *   CmpSeabaseDDLuser
// *
// *
// *****************************************************************************

#include "CmpSeabaseDDLauth.h"
#include "CmpSeabaseDDL.h"
#include "StmtDDLRegisterUser.h"
#include "StmtDDLAlterUser.h"
#include "ElemDDLGrantee.h"
#include "CompException.h"
#include "Context.h"
#include "dbUserAuth.h"
#include "ComUser.h"
#include "CmpDDLCatErrorCodes.h"
#include "NAStringDef.h"
#include "ExpHbaseInterface.h"

#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#ifndef   SQLPARSERGLOBALS_LEX_AND_PARSE
#define   SQLPARSERGLOBALS_LEX_AND_PARSE
#endif
#define   SQLPARSERGLOBALS_FLAGS
#define   SQLPARSERGLOBALS_NADEFAULTS_SET
#include "SqlParserGlobalsCmn.h"

#define  RESERVED_AUTH_NAME_PREFIX  "DB__"

inline static bool validateExternalUsername(
   const char *                                 externalUsername,
   DBUserAuth::AuthenticationConfiguration      configurationNumber,
   DBUserAuth::AuthenticationConfiguration &    foundConfigurationNumber);

// ****************************************************************************
// Class CmpSeabaseDDLauth methods
// ****************************************************************************

// ----------------------------------------------------------------------------
// Default constructor
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::CmpSeabaseDDLauth()
: authID_(NA_UserIdDefault),
  authType_(COM_UNKNOWN_ID_CLASS),
  authCreator_(NA_UserIdDefault),
  authCreateTime_(0),
  authRedefTime_(0),
  authValid_(true),
  authImmutable_(false)
{}

// ----------------------------------------------------------------------------
// public method:  authExists
//
// Input:
//   authName - name to look up
//   isExternal -
//       true - the auth name is the external name (auth_ext_name)
//       false - the auth name is the database name (auth_db_name)
//
// Output:
//   Returns true if authorization row exists in the metadata
//   Returns false if authorization row does not exist in the metadata or an 
//      unexpected error occurs
//
//  The diags area contains an error if any unexpected errors occurred.
//  Callers should check the diags area when false is returned
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLauth::authExists (const NAString &authName, bool isExternal)
{
  // Read the auths table to get a count of rows for the authName
  Int64 rowCount = 0;
  try
  {
    NAString whereClause ("where ");
    whereClause += (isExternal) ? "auth_ext_name = '" : "auth_db_name = '";
    whereClause += authName;
    whereClause += "'";
    rowCount = selectCount(whereClause);
    return (rowCount > 0) ? true : false;
  }

  catch (...)
  {
    // If there is no error in the diags area, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      *CmpCommon::diags() << DgSqlCode (-CAT_INTERNAL_EXCEPTION_ERROR)
                          << DgInt0(__LINE__)
                          << DgString0("CmpSeabaseDDLauth::authExists for authName");
    return false;
  }
}

// ----------------------------------------------------------------------------
// public method: getAuthDetails
//
// Populates the CmpSeabaseDDLauth class containing auth details for the
// requested authName
//
// Input:
//    authName - the database or external auth name
//    isExternal -
//       true - the auth name is the external name (auth_ext_name)
//       false - the auth name is the database name (auth_db_name)
//
// Output:
//    Returned parameter (AuthStatus):
//       STATUS_GOOD: authorization details are populated:
//       STATUS_NOTFOUND: authorization details were not found
//       STATUS_WARNING: (not 100) warning was returned, diags area populated
//       STATUS_ERROR: error was returned, diags area populated
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::AuthStatus 
CmpSeabaseDDLauth::getAuthDetails(const char *pAuthName, bool isExternal)
{
  try
  {
    NAString whereClause ("where ");
    whereClause += (isExternal) ? "auth_ext_name = '" : "auth_db_name = '";
    whereClause += pAuthName;
    whereClause += "'";
    return selectExactRow(whereClause);
  }

  catch (...)
  {
    // If there is no error in the diags area, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      *CmpCommon::diags() << DgSqlCode (-CAT_INTERNAL_EXCEPTION_ERROR)
                          << DgInt0(__LINE__)
                          << DgString0("CmpSeabaseDDLauth::getAuthDetails for authName");
    return STATUS_ERROR;
  }
}

// ----------------------------------------------------------------------------
// public method:  getAuthDetails
//
// Create the CmpSeabaseDDLauth class containing auth details for the
// request authID
//
// Input:
//    authID - the database authorization ID to search for
//
//  Output:
//    A returned parameter:
//       STATUS_GOOD: authorization details are populated:
//       STATUS_NOTFOUND: authorization details were not found
//       STATUS_WARNING: (not 100) warning was returned, diags area populated
//       STATUS_ERROR: error was returned, diags area populated
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::AuthStatus CmpSeabaseDDLauth::getAuthDetails(Int32 authID)
{
  try
  {
    NAString whereClause ("where auth_id = ");
    whereClause += authID;
    return selectExactRow(whereClause);
  }

  catch (...)
  {
    // If there is no error in the diags area, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      *CmpCommon::diags() << DgSqlCode (-CAT_INTERNAL_EXCEPTION_ERROR)
                          << DgInt0(__LINE__)
                          << DgString0("CmpSeabaseDDLauth::getAuthDetails for authID");

    return STATUS_ERROR;
  }
}

// ----------------------------------------------------------------------------
// method: isAuthNameReserved
//
// Checks to see if proposed name is reserved
//
// Input: authorization name
//
// Output:
//   true - name is reserved
//   false - name is not reserved
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLauth::isAuthNameReserved (const NAString &authName)
{
  bool result;
  result = authName.length() >= strlen(RESERVED_AUTH_NAME_PREFIX)
           &&
           authName.operator()(0,strlen(RESERVED_AUTH_NAME_PREFIX)) ==
                                                   RESERVED_AUTH_NAME_PREFIX
           ||
           authName == "_SYSTEM"
           ||
           authName == "PUBLIC"
           ||
           authName == "NONE";

  return result;
}

// ----------------------------------------------------------------------------
// method: isAuthNameValid
//
// checks to see if the name contains valid character
//
// Input:
//    NAString - authName -- name string to check
//
// Returns:  true if valid, false otherwise
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLauth::isAuthNameValid(const NAString &authName)
{
  string validChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_@./";
  string strToScan = authName.data();
  size_t found = strToScan.find_first_not_of(validChars);
  if (found == string::npos)
    return true;
  return false;
}

// ----------------------------------------------------------------------------
// method: verifyAuthority
//
// makes sure user has privilege to perform operation
//
// Input: none
//
// Output:  an exception is generated if user does not have authority
// ----------------------------------------------------------------------------
void CmpSeabaseDDLauth::verifyAuthority()
{
  // get effective user from the Context
  const char *databaseUserName = GetCliGlobals()->currContext()->getDatabaseUserName();
  NAString userNameStr = databaseUserName;
  NAString rootNameStr = ComUser::getRootUserName();
  if (rootNameStr != userNameStr)
  {
    *CmpCommon::diags() << DgSqlCode (-CAT_NOT_AUTHORIZED);
    UserException excp (NULL, 0);
    throw excp;
  }
}

// ----------------------------------------------------------------------------
// Methods that perform metadata access
//
// All methods return a UserException if an unexpected error occurs
//-----------------------------------------------------------------------------

// Delete a row from the AUTHS table based on the AUTH_ID
void CmpSeabaseDDLauth::deleteRow(const NAString &authName)
{
  NAString systemCatalog = CmpSeabaseDDL::getSystemCatalogStatic();
  char buf[1000];
  ExeCliInterface cliInterface(STMTHEAP);
  str_sprintf(buf, "delete from %s.\"%s\".%s where auth_db_name = '%s'",
              systemCatalog.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS, authName.data());
  Lng32 cliRC = cliInterface.executeImmediate(buf);
  
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    UserException excp (NULL, 0);
    throw excp;
  }

  // Existence of the row is already checked, throw an assertion the row does not exist
  // With optimistic locking, this might occur if a concurrent session is deleting the
  // same row and gets in first
  CMPASSERT (cliRC == 100);

  // Not sure if it is possible to get a warning from a delete and
  // what it means if one is returned for now, it is ignored.
}

// Insert a row into the AUTHS table
void CmpSeabaseDDLauth::insertRow()
{
  char buf[1000];
  ExeCliInterface cliInterface(STMTHEAP);

  NAString authType;
  switch (getAuthType())
  {
    case COM_ROLE_CLASS:
      authType = COM_ROLE_CLASS_LIT;
      break;
    case COM_USER_CLASS:
      authType = COM_USER_CLASS_LIT;
      break;
    default:
      *CmpCommon::diags() << DgSqlCode (-CAT_INTERNAL_EXCEPTION_ERROR)
                          << DgInt0(__LINE__)
                          << DgString0("CmpSeabaseDDLauth::deleteRow invalid authType");
      UserException excp (NULL, 0);
      throw excp;
  }

  NAString authValid = isAuthValid() ? "Y" : "N";
  NAString immutable = isAuthImmutable() ? "Y" : "N";

  NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
  str_sprintf(buf, "insert into %s.\"%s\".%s values (%d, '%s', '%s', '%s', %d, '%s', %Ld, %Ld)",
              sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS,
              getAuthID(),
              getAuthDbName().data(),
              getAuthExtName().data(),
              authType.data(),
              getAuthCreator(),
              authValid.data(),
              getAuthCreateTime(),
              getAuthRedefTime());

  Int32 cliRC = cliInterface.executeImmediate(buf);

  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    UserException excp (NULL, 0);
    throw excp;
  }

  // Not sure if it is possible to get a warning from an insert and
  // what it means if one is returned, for now it is ignored.
  
}

// update a row in AUTHS table based on the passed in setClause
void CmpSeabaseDDLauth::updateRow(const NAString &setClause)
{
  char buf[1000];
  ExeCliInterface cliInterface(STMTHEAP);

  NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
  str_sprintf(buf, "update %s.\"%s\".%s %s where auth_id = %d",
              sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS,
              setClause.data(),
              getAuthID());

  Int32 cliRC = cliInterface.executeImmediate(buf);

  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    UserException excp (NULL, 0);
    throw excp;
  }
  
  // Existence of the row is already checked, throw an assertion the row does not exist
  // With optimistic locking, this might occur if a concurrent session is deleting the
  // same row and gets in first
  CMPASSERT (cliRC == 100);

  // Not sure if it is possible to get a warning from an update and
  // what it means if one is returned, for now it is ignored.
  
}

// select exact based on the passed in whereClause
CmpSeabaseDDLauth::AuthStatus 
CmpSeabaseDDLauth::selectExactRow(const NAString & whereClause)
{
   NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
   char buf[1000];
   str_sprintf(buf, "select auth_id, auth_db_name, auth_ext_name, auth_type, auth_creator, auth_is_valid, auth_create_time, auth_redef_time from %s.\"%s\".%s %s ",
              sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS, whereClause.data());
    NAString cmd (buf);

  ExeCliInterface cliInterface(STMTHEAP);

  Int32 cliRC = cliInterface.fetchRowsPrologue(cmd.data(), true/*no exec*/);
  if (cliRC != 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      DDLException excp (cliRC, NULL, 0);
      throw excp;
    }

  cliRC = cliInterface.clearExecFetchClose(NULL, 0);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      DDLException excp (cliRC, NULL, 0);
      throw excp;
    }

  if (cliRC == 100) // did not find the row
  {
    cliInterface.clearGlobalDiags();
    return STATUS_NOTFOUND;
  }

  // Set the return status
  CmpSeabaseDDLauth::AuthStatus authStatus = 
    (cliRC == 0) ? STATUS_GOOD : STATUS_WARNING;

  // Populate the class
  char * ptr = NULL;
  Lng32 len = 0;
  char type [6];

  // value 1:  auth_id (int32)
  cliInterface.getPtrAndLen(1, ptr, len);
  setAuthID(*(Int32*)ptr);

  // value 2: auth_db_name (NAString)
  cliInterface.getPtrAndLen(2,ptr,len);
  NAString dbName(ptr,len);
  setAuthDbName(dbName);

  // value 3: auth_ext_name (NAString)
  cliInterface.getPtrAndLen(3,ptr,len);
  NAString extName(ptr,len);
  setAuthExtName(extName);

  // value 4: auth_type (char)
  // str_cpy_and_null params: *tgt, *src, len, endchar, blank, null term
  cliInterface.getPtrAndLen(4,ptr,len);
  str_cpy_and_null(type, ptr, len, '\0', ' ', true);
  if ( type[0] == 'U')
    setAuthType(COM_USER_CLASS);
  else if (type[0] == 'R')
    setAuthType(COM_USER_CLASS);
  else
    setAuthType(COM_UNKNOWN_ID_CLASS);

  // value 5: auth_creator (int32)
  cliInterface.getPtrAndLen(5,ptr,len);
  setAuthCreator(*(Int32*)ptr);

  // value 6: auth_is_valid (char)
  cliInterface.getPtrAndLen(6,ptr,len);
  str_cpy_and_null(type, ptr, len, '\0', ' ', true);
  if (type[0] == 'Y')
    setAuthValid(true);
  else
    setAuthValid(false);

  // value 7: auth_create_time (int64)
  cliInterface.getPtrAndLen(7,ptr,len);
  Int64 intValue = *(Int64*)ptr;
  setAuthCreateTime((ComTimestamp) intValue);

  // value 8: auth_redef_time (int64)
  cliInterface.getPtrAndLen(8,ptr,len);
  intValue = *(Int64*)ptr;
  setAuthRedefTime((ComTimestamp) intValue);

  cliInterface.fetchRowsEpilogue(NULL, true);
  return authStatus;
}

// selectCount - returns the number of rows based on the where clause
Int64 CmpSeabaseDDLauth::selectCount(const NAString & whereClause)
{
  NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
  char buf[1000];
  str_sprintf(buf, "select count(*) from %s.\"%s\".%s %s ",
                sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS, 
                whereClause.data());

  Lng32 len = 0;
  Int64 rowCount = 0;
  ExeCliInterface cliInterface(STMTHEAP);
  Lng32 cliRC = cliInterface.executeImmediate(buf, (char*)&rowCount, &len, NULL);

  // If unexpected error occurred, return an exception
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    DDLException excp (cliRC, NULL, 0);
    throw excp;
  }

  return rowCount;
}

// selectMaxAuthID - gets the last used auth ID
// ACH - need to improve the algorithm
Int32 CmpSeabaseDDLauth::selectMaxAuthID(const NAString &whereClause)
{
  NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
  char buf[400];
  str_sprintf(buf, "select max (auth_id) from %s.\"%s\".%s %s" , 
              sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS,
              whereClause.data());

  Lng32 len = 0;
  Int32 maxValue = 0;
  ExeCliInterface cliInterface(STMTHEAP);
  Lng32 cliRC = cliInterface.executeImmediate(buf, (char *)&maxValue, &len, true);
  if (cliRC != 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    UserException excp (NULL, 0);
    throw excp;
  }

  return maxValue;
}

// ****************************************************************************
// Class CmpSeabaseDDLuser methods
// ****************************************************************************

// ----------------------------------------------------------------------------
// Default constructor
// ----------------------------------------------------------------------------
CmpSeabaseDDLuser::CmpSeabaseDDLuser()
: CmpSeabaseDDLauth()
{}

// ----------------------------------------------------------------------------
// public method: getUserDetails
//
// Create the CmpSeabaseDDLuser class containing user details for the
// requested userID
//
// Input:
//    userID - the database authorization ID to search for
//
//  Output:
//    Returned parameter:
//       STATUS_GOOD: authorization details are populated:
//       STATUS_NOTFOUND: authorization details were not found
//       STATUS_WARNING: (not 100) warning was returned, diags area populated
//       STATUS_ERROR: error was returned, diags area populated
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::AuthStatus CmpSeabaseDDLuser::getUserDetails(Int32 userID)
{
  CmpSeabaseDDLauth::AuthStatus retcode = getAuthDetails(userID);
  if (retcode == STATUS_GOOD && !isUser())
  {
    *CmpCommon::diags() << DgSqlCode (-CAT_IS_NOT_A_USER)
                        << DgString0(getAuthDbName().data());
     retcode =  STATUS_ERROR;
  }
  return retcode;
}

// ----------------------------------------------------------------------------
// public method: getUserDetails
//
// Create the CmpSeabaseDDLuser class containing user details for the
// requested username
//
// Input:
//    userName - the database username to retrieve details for
//    isExternal -
//       true - the username is the external name (auth_ext_name)
//       false - the username is the database name (auth_db_name)
//
//  Output:
//    Returned parameter:
//       STATUS_GOOD: authorization details are populated:
//       STATUS_NOTFOUND: authorization details were not found
//       STATUS_WARNING: (not 100) warning was returned, diags area populated
//       STATUS_ERROR: error was returned, diags area populated
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::AuthStatus 
CmpSeabaseDDLuser::getUserDetails(const char *pUserName, bool isExternal)
{
  CmpSeabaseDDLauth::AuthStatus retcode = getAuthDetails(pUserName, isExternal);
  if (retcode == STATUS_GOOD && !isUser())
  {
    *CmpCommon::diags() << DgSqlCode (-CAT_IS_NOT_A_USER)
                        << DgString0(getAuthDbName().data());
     retcode = STATUS_ERROR;
  }
  return retcode;
}

// ----------------------------------------------------------------------------
// method:  getUniqueID
//
// This method returns a unique user ID
//
// Input:  none
//
// Output:  returns a unique user ID
// ----------------------------------------------------------------------------
Int32 CmpSeabaseDDLuser::getUniqueID()
{
  Int32 newUserID = 0;
  NAString whereClause ("where auth_id >= 33333 and auth_id < 999999");
  newUserID = selectMaxAuthID(whereClause);
  newUserID++;
  return newUserID;
}  

// ----------------------------------------------------------------------------
// Public method: registerUser
//
// registers a user in the Trafodion metadata
//
// Input:  parse tree containing a definition of the user
// Output: the global diags area is set up with the result
// ----------------------------------------------------------------------------
void CmpSeabaseDDLuser::registerUser(StmtDDLRegisterUser * pNode)
{
  // Set up a global try/catch loop to catch unexpected errors
  try
  {
    // Verify user is authorized to perform REGISTER USER requests
    verifyAuthority();

    // Verify that the specified user name is not reserved
    setAuthDbName(pNode->getDbUserName());
    if (isAuthNameReserved(pNode->getDbUserName()))
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_AUTH_NAME_RESERVED)
                          << DgString0(pNode->getDbUserName().data());
      return;
    }


    // Verify that the name does not include unsupported special characters
    if (!isAuthNameValid(getAuthDbName()))
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_INVALID_CHARS_IN_AUTH_NAME)
                          << DgString0(pNode->getDbUserName().data());
      return;
    }

    setAuthExtName(pNode->getExternalUserName());
    if (!isAuthNameValid(getAuthExtName()))
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_INVALID_CHARS_IN_AUTH_NAME)
                          << DgString0(pNode->getExternalUserName().data());
      return;
    }

    // set up class members from parse node
    setAuthImmutable(pNode->isImmutable());
    setAuthType(COM_USER_CLASS);  // we are a user
    setAuthValid(true); // assume a valid user

    Int64 createTime = NA_JulianTimestamp();
    setAuthCreateTime(createTime);
    setAuthRedefTime(createTime);  // make redef time the same as create time

    // Make sure db user has not already been registered
    if (authExists(getAuthDbName(), false))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_ALREADY_EXISTS)
                          << DgString0(getAuthDbName().data());
      return;
    }
    // unexpected error occurred - ?
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0)
      return;

    // Make sure external user has not already been registered
    if (authExists(getAuthExtName(), true))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_LDAP_USER_ALREADY_EXISTS)
                          << DgString0(getAuthExtName().data());
       return;
    }
    // unexpected error occurred - ?
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0)
      return;

DBUserAuth::AuthenticationConfiguration configurationNumber = DBUserAuth::DefaultConfiguration;
DBUserAuth::AuthenticationConfiguration foundConfigurationNumber = DBUserAuth::DefaultConfiguration;

    // Verify that the external user exists in configured identity store
    if (!validateExternalUsername(pNode->getExternalUserName().data(),
                                  configurationNumber,
                                  foundConfigurationNumber))
       return;

    // Get a unique auth ID number
    Int32 userID = getUniqueID();
    setAuthID (userID);

    // If the BY clause was specified, then register the user on behalf of the
    // authorization ID specified in this clause.
    // Need to translate the creator name to its authID
    if (pNode->getOwner() == NULL)
    {
      // get effective user from the Context
      Int32 *pUserID = GetCliGlobals()->currContext()->getDatabaseUserID();
      setAuthCreator(*pUserID);
    }
    else
    {
      const NAString creatorName =
        pNode->getOwner()->getAuthorizationIdentifier();
      // TODO: get the authID for the creatorName
      // TODO: verify creator can register users
      setAuthCreator(NA_UserIdDefault);
    }

    // Add the user to AUTHS table
    insertRow();
  }
  catch (...)
  {
    // At this time, an error should be in the diags area.
    // If there is no error, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      *CmpCommon::diags() << DgSqlCode (-CAT_INTERNAL_EXCEPTION_ERROR)
                          << DgInt0(__LINE__)
                          << DgString0("CmpSeabaseDDLuser::register user");
  }
}

// ----------------------------------------------------------------------------
// public method:  unregisterUser
//
// This method removes a user from the database
//
// Input:  parse tree containing a definition of the user
// Output: the global diags area is set up with the result
// ----------------------------------------------------------------------------
void CmpSeabaseDDLuser::unregisterUser (StmtDDLRegisterUser * pNode)
{
  try
  {
    verifyAuthority();

    // CASCADE option not yet supported
    if (pNode->getDropBehavior() == COM_CASCADE_DROP_BEHAVIOR)
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_ONLY_SUPPORTING_RESTRICT_DROP_BEHAVIOR);
      return;
    }

    // Verify that the specified user name is not reserved
    if (isAuthNameReserved(pNode->getDbUserName()))
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_AUTH_NAME_RESERVED)
                          << DgString0(pNode->getDbUserName().data());
       return;
    }

    // read user details from the AUTHS table
    const NAString dbUserName(pNode->getDbUserName());
    CmpSeabaseDDLauth::AuthStatus retcode =  getUserDetails(dbUserName);
    if (retcode == STATUS_ERROR)
      return;
    if (retcode == STATUS_NOTFOUND)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_NOT_EXIST)
                          << DgString0(dbUserName.data());
      return;
    }


    // TODO, check to see if the user owns anything before removing

    // delete the row
    deleteRow(getAuthDbName());
  }
  catch (...)
  {
    // At this time, an error should be in the diags area.
    // If there is no error, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      *CmpCommon::diags() << DgSqlCode (-CAT_INTERNAL_EXCEPTION_ERROR)
                          << DgInt0(__LINE__)
                          << DgString0("CmpSeabaseDDLuser::unregister user");
  }
}

// ----------------------------------------------------------------------------
// public method:  alterUser
//
// This method changes a user definition
//
// Input:  parse tree containing a definition of the user change
// Output: the global diags area is set up with the result
// ----------------------------------------------------------------------------
void CmpSeabaseDDLuser::alterUser (StmtDDLAlterUser * pNode)
{
  try
  {
    verifyAuthority();

    // Verify that that user name being altered is not a reserved name
    if (isAuthNameReserved(pNode->getDatabaseUsername()))
     {
       *CmpCommon::diags() << DgSqlCode (-CAT_AUTH_NAME_RESERVED)
                           << DgString0(pNode->getDatabaseUsername().data());
       return;
     }

    // read user details from the AUTHS table
    const NAString dbUserName(pNode->getDatabaseUsername());
    CmpSeabaseDDLauth::AuthStatus retcode = getUserDetails(dbUserName);
    if (retcode == STATUS_ERROR)
      return;
    if (retcode == STATUS_NOTFOUND)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_NOT_EXIST)
                          << DgString0(dbUserName.data());
      return;
    }

    // Process the requested operation
    StmtDDLAlterUser::AlterUserCmdSubType cmdSubType = 
      pNode->getAlterUserCmdSubType();

    NAString setClause("set ");
    switch (cmdSubType)
    {
       case StmtDDLAlterUser::SET_EXTERNAL_NAME:
       {
          // Make sure external user has not already been registered
          if (authExists(pNode->getExternalUsername(), true))
          {
            *CmpCommon::diags() << DgSqlCode(-CAT_LDAP_USER_ALREADY_EXISTS)
                                << DgString0(pNode->getExternalUsername().data());
             return;
          }
          // unexpected error occurred - ?
          if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0)
            return;

          DBUserAuth::AuthenticationConfiguration 
            configurationNumber = DBUserAuth::DefaultConfiguration;
          DBUserAuth::AuthenticationConfiguration 
            foundConfigurationNumber = DBUserAuth::DefaultConfiguration;

          // Verify that the external user exists in configured identity store
          if (!validateExternalUsername(pNode->getExternalUsername().data(),
                                        configurationNumber,
                                        foundConfigurationNumber))
             return;

          setAuthExtName(pNode->getExternalUsername());
          setClause += "auth_ext_name = '";
          setClause += getAuthExtName();
          setClause += "'";
          break;
       }

       case StmtDDLAlterUser::SET_IS_VALID_USER:
       {
          setAuthValid(pNode->isValidUser());
          setClause += (isAuthValid()) ? "auth_is_valid = 'Y'" : "auth_is_valid = 'N'";
          break;
       }

       default:
       {
         *CmpCommon::diags() << DgSqlCode (-CAT_UNSUPPORTED_COMMAND_ERROR );
         return;
       }
    }
    updateRow(setClause);
  }
  catch (...)
  {
    // At this time, an error should be in the diags area.
    // If there is no error, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      *CmpCommon::diags() << DgSqlCode (-CAT_INTERNAL_EXCEPTION_ERROR)
                          << DgInt0(__LINE__)
                          << DgString0("CmpSeabaseDDLuser::alterUser");
  }
}

// -----------------------------------------------------------------------------
// *                                                                           *
// * Function: validateExternalUsername                                        *
// *                                                                           *
// *    Determines if an external username is valid.                           *
// *                                                                           *
// -----------------------------------------------------------------------------
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <externalUsername>          const char *                             In  *
// *    is the username to be validated.                                       *
// *                                                                           *
// *  <configurationNumber>      DBUserAuth::AuthenticationConfiguration   In  *
// *    specifies which configuration to use to validate the username; a       *
// *  configuration designates one or more identity stores and their           *
// *  parameters.  A value of zero indicates the "default" configuration       *
// *  should be used.                                                          *
// *                                                                           *
// *  <foundConfigurationNumber> DBUserAuth::AuthenticationConfiguration & In  *
// *    passes back the configuration used to validate the username.           *
// *                                                                           *
// -----------------------------------------------------------------------------
inline static bool validateExternalUsername(
   const char *                                 externalUsername,
   DBUserAuth::AuthenticationConfiguration      configurationNumber,
   DBUserAuth::AuthenticationConfiguration &    foundConfigurationNumber)

{

// During initialization external checking needs to be disabled to setup
// standard database users.
   if (Get_SqlParser_Flags(DISABLE_EXTERNAL_USERNAME_CHECK))
      return true;

// Verify that the external username is defined in the identity store.
DBUserAuth::CheckUserResult chkUserRslt = DBUserAuth::UserDoesNotExist;

   chkUserRslt = DBUserAuth::CheckExternalUsernameDefined(externalUsername,
                                                          configurationNumber,
                                                          foundConfigurationNumber);

// Username was found!
   if (chkUserRslt == DBUserAuth::UserExists)
      return true;

// Who?
   if (chkUserRslt == DBUserAuth::UserDoesNotExist)
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_LDAP_USER_NOT_FOUND)
                          << DgString0(externalUsername);
      return false;
   }

// Problem looking up the username.  Could be a bad configuration, a
// problem at the identity store, or communicating with the identity store.
   if (chkUserRslt == DBUserAuth::ErrorDuringCheck)
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_LDAP_COMM_ERROR);
      return false;
   }

   return false;

}
//---------------------- End of validateExternalUsername -----------------------

// -----------------------------------------------------------------------------
// public method:  describe
//
// This method returns the showddl text for the requested user in string format
//
// Input:
//   authName - name of user to describe
//
// Input/Output:  
//   authText - the REGISTER USER text
//
// returns result:
//    true -  successful
//    false - failed (ComDiags area will be set up with appropriate error)
//-----------------------------------------------------------------------------
bool CmpSeabaseDDLuser::describe (const NAString &authName, NAString &authText)
{
  try
  {
    CmpSeabaseDDLauth::AuthStatus retcode = getUserDetails(authName.data());
    
    // If the user was not found, set up an error
    if (retcode == STATUS_NOTFOUND)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_NOT_EXIST)
                          << DgString0(authName.data());
      return false;
    }

    // If an error was detected, throw an exception so the catch handler will 
    // put a value in ComDiags area in case no message exists
    if (retcode == STATUS_ERROR)
    {
      UserException excp (NULL, 0);
      throw excp;
    }
  
    // Generate output text
    authText = "REGISTER USER \"";
    authText += getAuthExtName();
    if (getAuthExtName() != getAuthDbName())
    {
      authText += "\" AS \"";
      authText += getAuthDbName();
    }
    authText += "\";\n";

    if (!isAuthValid())
    {
      authText += "ALTER USER \"";
      authText += getAuthDbName();
      authText += "\" SET OFFLINE;\n";
    }
  }

  catch (...)
  {
   // At this time, an error should be in the diags area.
   // If there is no error, set up an internal error
   if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
    *CmpCommon::diags() << DgSqlCode(-CAT_INTERNAL_EXCEPTION_ERROR)
                        << DgInt0(__LINE__)
                        << DgString0("CmpSeabaseDDLuser::describe");
    return false;
  }

  return true;
}
//------------------------------ End of describe -------------------------------



