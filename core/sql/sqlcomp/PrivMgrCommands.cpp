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
//   PrivMgrObjectInfo
//   PrivMgrCommands
// ==========================================================================

#include "PrivMgrCommands.h"
#include "PrivMgrMD.h"
#include "DgBaseType.h"
#include "NATable.h"
#include "NAColumn.h"

#include "PrivMgrPrivileges.h"

#include "PrivMgrComponents.h"
#include "PrivMgrComponentOperations.h"
#include "PrivMgrComponentPrivileges.h"
#include "PrivMgrRoles.h"
#include "ComSecurityKey.h"
#include "ComUser.h"
#include <cstdio>
#include <algorithm>

// ****************************************************************************
// Class: PrivMgrObjectInfo
// ****************************************************************************
PrivMgrObjectInfo::PrivMgrObjectInfo(
  const NATable *naTable)
: objectOwner_ (naTable->getOwner()),
  schemaOwner_ (naTable->getSchemaOwner()),
  objectUID_   (naTable->objectUid().get_value()),
  objectName_  (naTable->getTableName().getQualifiedNameAsAnsiString().data()),
  objectType_  (naTable->getObjectType())
{
  const NAColumnArray &colNameArray = naTable->getNAColumnArray();
  for (size_t i = 0; i < colNameArray.entries(); i++)
  {
    const NAColumn * naCol = colNameArray.getColumn(i);
    std::string columnName(naCol->getColName().data());
    columnList_.push_back( columnName);
  }
}
 
// ****************************************************************************
// Class: PrivMgrUserPrivs
// ****************************************************************************
bool PrivMgrUserPrivs::initUserPrivs(
  const std::vector<int32_t> & roleIDs,
  const TrafDesc *priv_desc,
  const int32_t userID,
  const int64_t objectUID,
  ComSecurityKeySet & secKeySet)
{
  hasPublicPriv_ = false;

  // generate PrivMgrUserPrivs from the priv_desc structure
  TrafDesc *priv_grantees_desc = priv_desc->privDesc()->privGrantees;
  NAList<PrivMgrDesc> descList(NULL);

  // Find relevant descs for the user
  while (priv_grantees_desc)
  {
    Int32 grantee = priv_grantees_desc->privGranteeDesc()->grantee;
    bool addDesc = false;
    if (grantee == userID)
      addDesc = true;

    if (PrivMgr::isRoleID(grantee))
    {
      if ((std::find(roleIDs.begin(), roleIDs.end(), grantee)) != roleIDs.end())
        addDesc = true;
    }

    if (ComUser::isPublicUserID(grantee))
    {
      addDesc = true;
      hasPublicPriv_ = true;
    }

    // Create a list of PrivMgrDesc contain privileges for user, user's roles,
    // and public
    if (addDesc)
    {
      TrafDesc *objectPrivs = priv_grantees_desc->privGranteeDesc()->objectBitmap;

      PrivMgrCoreDesc objectDesc(objectPrivs->privBitmapDesc()->privBitmap,
                                 objectPrivs->privBitmapDesc()->privWGOBitmap);
      
      TrafDesc *priv_grantee_desc = priv_grantees_desc->privGranteeDesc();
      TrafDesc *columnPrivs = priv_grantee_desc->privGranteeDesc()->columnBitmaps;
      NAList<PrivMgrCoreDesc> columnDescs(NULL);
      while (columnPrivs)
      {
        PrivMgrCoreDesc columnDesc(columnPrivs->privBitmapDesc()->privBitmap,
                                   columnPrivs->privBitmapDesc()->privWGOBitmap,
                                   columnPrivs->privBitmapDesc()->columnOrdinal);
        columnDescs.insert(columnDesc);
        columnPrivs = columnPrivs->next;
      }

      PrivMgrDesc privs(priv_grantees_desc->privGranteeDesc()->grantee);
      privs.setTablePrivs(objectDesc);
      privs.setColumnPrivs(columnDescs);
      privs.setHasPublicPriv(hasPublicPriv_);

      descList.insert(privs);
    }
    priv_grantees_desc = priv_grantees_desc->next;
  }

  // Union privileges from all grantees together to create a single set of
  // bitmaps.  Create security invalidation keys
  for (int i = 0; i < descList.entries(); i++)
  {
    PrivMgrDesc privs = descList[i];

    // Set up object level privileges
    objectBitmap_ |= privs.getTablePrivs().getPrivBitmap();
    grantableBitmap_ |= privs.getTablePrivs().getWgoBitmap();

    // Set up column level privileges
    NAList<PrivMgrCoreDesc> columnPrivs = privs.getColumnPrivs();
    std::map<size_t,PrivColumnBitmap>::iterator it;
    for (int j = 0; j < columnPrivs.entries(); j++)
    {
      PrivMgrCoreDesc colDesc = columnPrivs[j];
      Int32 columnOrdinal = colDesc.getColumnOrdinal();
      it = colPrivsList_.find(columnOrdinal);
      if (it == colPrivsList_.end())
      {
        colPrivsList_[columnOrdinal] = colDesc.getPrivBitmap();
        colGrantableList_[columnOrdinal] = colDesc.getWgoBitmap();
      }
      else
      {
        colPrivsList_[columnOrdinal] |= colDesc.getPrivBitmap();
        colGrantableList_[columnOrdinal] |= colDesc.getWgoBitmap();
      }
    }

    // set up security invalidation keys
    if (!buildSecurityKeys(userID, privs.getGrantee(), objectUID, privs.getTablePrivs(), secKeySet))
      return false;

    for (int k = 0; k < colPrivsList_.size(); k++)
    {
      PrivMgrCoreDesc colDesc(colPrivsList_[k], colGrantableList_[k]);
      if (!buildSecurityKeys(userID, privs.getGrantee(), objectUID, colDesc, secKeySet))
        return false;
    }
  }

  // TBD - add schema privilege bitmaps
  return true;
}

// ----------------------------------------------------------------------------
// method: initUserPrivs
//
// Creates a PrivMgrUserPrivs object from a PrivMgrDesc object
// ----------------------------------------------------------------------------
void PrivMgrUserPrivs::initUserPrivs(PrivMgrDesc &privsOfTheUser)
{
  objectBitmap_ = privsOfTheUser.getTablePrivs().getPrivBitmap();
  grantableBitmap_ = privsOfTheUser.getTablePrivs().getWgoBitmap();

  for (int32_t i = 0; i < privsOfTheUser.getColumnPrivs().entries(); i++)
  {
    const int32_t columnOrdinal = privsOfTheUser.getColumnPrivs()[i].getColumnOrdinal();
    colPrivsList_[columnOrdinal] = privsOfTheUser.getColumnPrivs()[i].getPrivBitmap();
    colGrantableList_[columnOrdinal] = privsOfTheUser.getColumnPrivs()[i].getWgoBitmap();
  }
  hasPublicPriv_ = privsOfTheUser.getHasPublicPriv();
}

// ****************************************************************************
// Class: PrivMgrCommands
// ****************************************************************************

// -----------------------------------------------------------------------
// Default Constructor
// -----------------------------------------------------------------------
PrivMgrCommands::PrivMgrCommands () 
{
};

// -----------------------------------------------------------------------
// Construct a PrivMgrCommands object for a new component.
// -----------------------------------------------------------------------
PrivMgrCommands::PrivMgrCommands ( const std::string trafMetadataLocation
                                 , const std::string &metadataLocation
                                 , ComDiagsArea *pDiags
                                 , PrivMDStatus authorizationEnabled )
: PrivMgr(trafMetadataLocation,metadataLocation,pDiags,authorizationEnabled)
{
};

PrivMgrCommands::PrivMgrCommands ( const std::string &metadataLocation
                                 , ComDiagsArea *pDiags
                                 , PrivMDStatus authorizationEnabled )
: PrivMgr(metadataLocation,pDiags,authorizationEnabled)
{
};

// -----------------------------------------------------------------------
// Copy constructor
// -----------------------------------------------------------------------
PrivMgrCommands:: PrivMgrCommands ( const PrivMgrCommands &other )
: PrivMgr(other)
{
}

// -----------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------
PrivMgrCommands::~PrivMgrCommands() 
{
}

// ----------------------------------------------------------------------------
// Assignment operator
// ----------------------------------------------------------------------------
PrivMgrCommands& PrivMgrCommands::operator=(const PrivMgrCommands& other)
{
  //  Check for pathological case of X = X.
  if ( this == &other )
    return *this;

  metadataLocation_ = other.metadataLocation_;
  trafMetadataLocation_ = other.trafMetadataLocation_;
  pDiags_ = other.pDiags_;
  parserFlags_ = other.parserFlags_;
  authorizationEnabled_ = other.authorizationEnabled_;
  return *this;
}

// ----------------------------------------------------------------------------
// method: authorizationEnabled
//
// Return true if authorization has been enabled, false otherwise.
//
// ----------------------------------------------------------------------------
bool PrivMgrCommands::authorizationEnabled()
{
  if (authorizationEnabled_ == PRIV_INITIALIZED)
    return true;
  PrivMgrMDAdmin admin(getMetadataLocation(), getDiags());
  return admin.isAuthorizationEnabled();
}
  
// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrCommands::createComponentOperation                       *
// *                                                                           *
// *    Add an operation for a specified component.                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the component name.                                                 *
// *                                                                           *
// *  <operationName>                 const std::string &             In       *
// *    is the operation name to be added.                                     *
// *                                                                           *
// *  <operationCode>                 const std::string &             In       *
// *    is a 2 character code associated with the operation unique to the      *
// *    component.                                                             *
// *                                                                           *
// *  <isSystemOperation>             bool                            In       *
// *    is true if the operation is a system operation.                        *
// *                                                                           *
// *  <operationDescription>          const std::string &             In       *
// *    is a descrption of the operation.  May be empty.                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Operation added.                                             *
// *           *: Operation not added. A CLI error is put into the diags area. *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrCommands::createComponentOperation(
   const std::string & componentName,
   const std::string & operationName,
   const std::string & operationCode,
   bool isSystem,
   const std::string & operationDescription)
   
{

PrivStatus privStatus = STATUS_GOOD;

   try
   {
      PrivMgrComponentOperations componentOperations(getMetadataLocation(),
                                                     pDiags_);
      
      privStatus = componentOperations.createOperation(componentName,
                                                       operationName,
                                                       operationCode,
                                                       isSystem,
                                                       operationDescription);
   }

   catch (...)
   {
      return STATUS_ERROR;
   }
   
   return privStatus;
   
}
//************* End of PrivMgrCommands::createComponentOperation ***************

//-----------------------------------------------------------------------------
// method: describeComponents
// returns description of component in the form of REGISTER, CREATE, GRANT statements, 
// by a specified component name.
//   
// Input:
//     componentName - a unique component name
//      
// Output:
//     outlines -output string lines in array
// 
// returns true on success.
//-----------------------------------------------------------------------------
bool PrivMgrCommands::describeComponents(
   const std::string & componentName, 
   std::vector<std::string> & outlines)
   
{

    PrivStatus privStatus = STATUS_GOOD;
    try
    {   
        std::string componentUIDString;
        int64_t componentUID;
        //generate register component statement
        PrivMgrComponents components(getMetadataLocation(), 
                                     pDiags_); 

        privStatus = components.describeComponents(componentName, 
                                                   componentUIDString, 
                                                   componentUID, 
                                                   outlines);
        // If the component name was not registered
        if (privStatus == STATUS_NOTFOUND)
          return false;

        //component was registered, try to 
        //find it in component_operations, component_privileges tables,
        //and generate create & grant statements.
        PrivMgrComponentPrivileges componentPrivileges(getMetadataLocation(), 
                                                       pDiags_); 
        PrivMgrComponentOperations componentOperations(getMetadataLocation(), 
                                                       pDiags_); 
        privStatus = componentOperations.describeComponentOperations(componentUIDString, 
                                                                     componentName, 
                                                                     outlines, 
                                                                     &componentPrivileges);
    }

    catch (...)
    {
      return false;
    }
    
    return true;
    
}
//*************** End of PrivMgrCommands::describeComponents *******************


// ----------------------------------------------------------------------------
// method: describePrivileges
//
// returns GRANT statements for privileges associated with the 
// specified objectUID
//
// Parameters:
//     objectUID - a unique object identifier
//     objectName - the name of the object
//     privilegeText - the resultant grant text
//
// returns true if successful
// The Trafodion diags area contains any errors that were encountered
// ----------------------------------------------------------------------------
bool PrivMgrCommands::describePrivileges (const PrivMgrObjectInfo &objectInfo,
                                          std::string &privilegeText)
{
  PrivMgrPrivileges objectPrivs (objectInfo, metadataLocation_, pDiags_);
  PrivStatus retcode = objectPrivs.getPrivTextForObject(objectInfo,privilegeText);
  return (retcode == STATUS_GOOD) ? true : false;
}

   
// ----------------------------------------------------------------------------
// method: dropAuthorizationMetadata
//
// This method drops all the metadata used by the privilege manager
//
// Returns the status of the request
// The Trafodion diags area contains the error that was encountered
// ----------------------------------------------------------------------------
PrivStatus PrivMgrCommands::dropAuthorizationMetadata(bool doCleanup)
{
  PrivMgrMDAdmin metadata(getMetadataLocation(),getDiags());
  std::vector<string> tablesToDrop;
  size_t numTables = sizeof(privMgrTables)/sizeof(PrivMgrTableStruct);
  for (int ndx_tl = 0; ndx_tl < numTables; ndx_tl++)
  {
    const PrivMgrTableStruct &tableDefinition = privMgrTables[ndx_tl];
    tablesToDrop.push_back(tableDefinition.tableName);
  }
  return metadata.dropMetadata(tablesToDrop, doCleanup);
}



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrCommands::dropComponentOperation                         *
// *                                                                           *
// *    Removes operation for the specified component from the list of         *
// *  operations for that component.  Granted privileges are automatically     *
// *  removed if the CASCADE option is specified.                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the component name.                                                 *
// *                                                                           *
// *  <operationName>                 const std::string &             In       *
// *    is the operation name.                                                 *
// *                                                                           *
// *  <dropBehavior>                  PrivDropBehavior                In       *
// *    indicates whether restrict or cascade behavior is requested.           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Operation deleted.                                           *
// *           *: Operation not deleted. A CLI error is put into the           *
// *              diags area.                                                  *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrCommands::dropComponentOperation(
   const std::string & componentName,
   const std::string & operationName,
   PrivDropBehavior dropBehavior) 

{

PrivStatus privStatus = STATUS_GOOD;

   try
   {
      PrivMgrComponentOperations componentOperations(getMetadataLocation(),
                                                     pDiags_);
      
      privStatus = componentOperations.dropOperation(componentName,
                                                     operationName,
                                                     dropBehavior);
   }

   catch (...)
   {
      return STATUS_ERROR;
   }
   
   return privStatus;
   
}
//************** End of PrivMgrCommands::dropComponentOperation ****************

// ----------------------------------------------------------------------------
// method: getGrantorDetailsForObject
//
// Calls PrivMgrPrivileges::getGrantorDetailsForObject to return the effective 
// grantor ID and grantor name for object level grant and revoke statements.
//
// Input: see PrivMgrPrivileges.cpp for description
// Output: see PrivMgrPrivileges.cpp for description
//
// returns PrivStatus with the results of the operation.  The diags area 
// contains error details.
// ----------------------------------------------------------------------------
PrivStatus PrivMgrCommands::getGrantorDetailsForObject(
   const bool isGrantedBySpecified,
   const std::string grantedByName,
   const int_32 objectOwner,
   int_32 &effectiveGrantorID,
   std::string &effectiveGrantorName)
{
  PrivMgrPrivileges grantorDetails (metadataLocation_, pDiags_);
  grantorDetails.setTrafMetadataLocation(trafMetadataLocation_);
  return grantorDetails.getGrantorDetailsForObject
   (isGrantedBySpecified, grantedByName, objectOwner, 
    effectiveGrantorID, effectiveGrantorName);
}

// ----------------------------------------------------------------------------
// method: getPrivileges
//
// returns the list of privileges and security keys for the userID given the  
// specified objectUID
//
// Input:
//     naTable - a pointer to the object
//     userID - user to gather privs
//     userPrivileges - returns available privileges
//     secKeySet - the security keys for the object/user
//
// returns true if results were found, false otherwise
// The Trafodion diags area contains any errors that were encountered
// ----------------------------------------------------------------------------
PrivStatus PrivMgrCommands::getPrivileges(
  NATable *naTable,
  const int32_t userID,
  PrivMgrUserPrivs &userPrivs,
  std::vector <ComSecurityKey *>* secKeySet)
{
  PrivMgrDesc privsOfTheUser;

  // authorization is not enabled, return bitmaps with all bits set
  // With all bits set, privilege checks will always succeed
  if (!authorizationEnabled())
  {
    privsOfTheUser.setAllTableGrantPrivileges(true);
    userPrivs.initUserPrivs(privsOfTheUser);
    return STATUS_GOOD;
  }

  // if a hive table and does not have an external table and is not
  // registered in traf metadata, assume no privs
  if ((naTable->isHiveTable()) && 
      (NOT naTable->isRegistered()) &&
      (!naTable->hasExternalTable()))
  {
    PrivMgrDesc emptyDesc;
    userPrivs.initUserPrivs(emptyDesc);
  }
  else
  {
    PrivMgrPrivileges objectPrivs (metadataLocation_, pDiags_);
    PrivStatus retcode = objectPrivs.getPrivsOnObjectForUser((int64_t)naTable->objectUid().get_value(),
                                                             naTable->getObjectType(),
                                                             userID,
                                                             privsOfTheUser,
                                                             secKeySet);
    if (retcode != STATUS_GOOD)
      return retcode;

    userPrivs.initUserPrivs(privsOfTheUser);
  }

  return STATUS_GOOD;
}


// ----------------------------------------------------------------------------
// method: getPrivileges
//
// Creates a set of priv descriptors for all user grantees on an object
// Used by Trafodion compiler to store as part of the table descriptor.
//                                                       
//  Parameters:    
//                                                                       
//  <objectUID> is the unique identifier of the object
//  <objectType> is the type of object
//  <privDescs> is the returned list of privileges the on the object
//                                                                  
// Returns: PrivStatus                                               
//                                                                  
//   STATUS_GOOD: privilege descriptors were built
//             *: unexpected error occurred, see diags.     
// ----------------------------------------------------------------------------
PrivStatus PrivMgrCommands::getPrivileges(
  const int64_t objectUID,
  ComObjectType objectType,
  std::vector <PrivMgrDesc > &privDescs)
{
  // If authorization is enabled, go get privilege bitmaps from metadata
  if (authorizationEnabled())
  {
    PrivMgrPrivileges privInfo (objectUID, metadataLocation_, pDiags_);
    return privInfo.getPrivsOnObject(objectType, privDescs);
  }
  return STATUS_GOOD;
}


// ----------------------------------------------------------------------------
// method: getPrivileges
//
// returns GRANT statements for privileges associated with the 
// specified objectUID
//
// Input:
//     objectUID - a unique object identifier
//     objectType - the type of the object
//     objectName - the name of the object
//     privilegeText - the resultant grant text
//     secKeySet - the security keys for the object/user
//
// returns true if results were found, false otherwise
// The Trafodion diags area contains any errors that were encountered
// ----------------------------------------------------------------------------
PrivStatus PrivMgrCommands::getPrivileges(
  const int64_t objectUID,
  ComObjectType objectType,
  const int32_t userID,
  PrivMgrUserPrivs &userPrivs,
  std::vector <ComSecurityKey *>* secKeySet)
{
  PrivMgrDesc privsOfTheUser;

  // If authorization is enabled, go get privilege bitmaps from metadata
  if (authorizationEnabled())
  {
    PrivMgrPrivileges objectPrivs (metadataLocation_, pDiags_);
    PrivStatus retcode = objectPrivs.getPrivsOnObjectForUser(objectUID,
                                                             objectType,
                                                             userID,
                                                             privsOfTheUser,
                                                             secKeySet);
    if (retcode != STATUS_GOOD)
      return retcode;
  }

  // authorization is not enabled, return bitmaps with all bits set
  // With all bits set, privilege checks will always succeed
  else
    privsOfTheUser.getTablePrivs().setAllPrivAndWgo(true);

  userPrivs.initUserPrivs(privsOfTheUser);

  return STATUS_GOOD;
}

// ----------------------------------------------------------------------------
// method: getRoles
//
// Returns roleIDs for the grantee.
//                                                                       
//  <granteeID> the authorization ID to obtain roles for
//  <roleIDs> is the returned list of roles
//                                                                  
// Returns: PrivStatus                                               
//                                                                  
//   STATUS_GOOD: role list was built
//             *: unexpected error occurred, see diags.     
// ----------------------------------------------------------------------------
PrivStatus PrivMgrCommands::getRoles(
  const int32_t granteeID,
  std::vector <int32_t > &roleIDs)
{
  // If authorization is not enabled, return
  if (!authorizationEnabled())
    return STATUS_GOOD;
  
  PrivMgrRoles roles(" ",metadataLocation_,pDiags_);
  std::vector<std::string> roleNames;
  std::vector<int32_t> roleDepths;

  return  roles.fetchRolesForUser(granteeID,roleNames,roleIDs,roleDepths);
}

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrCommands::getPrivRowsForObject                           *
// *                                                                           *
// *    Returns rows for privileges granted on an object.                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <objectUID>                    const int64_t                     In      *
// *    is the unique ID of the object whose grants are being returned.        *
// *                                                                           *
// *  <objectPrivsRows>              std::vector<ObjectPrivsRow> &     In      *
// *    passes back a vector of rows representing the grants for the object.   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD    : Rows were returned                                       *
// * STATUS_NOTFOUND: No rows were returned                                    *
// *               *: Unable to read privileges, see diags.                    *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrCommands::getPrivRowsForObject(
   const int64_t objectUID,
   std::vector<ObjectPrivsRow> & objectPrivsRows)
   
{

PrivStatus privStatus = STATUS_GOOD;

   try
   {
      PrivMgrPrivileges objectPrivileges(objectUID,getMetadataLocation(),pDiags_);
      privStatus = objectPrivileges.getPrivRowsForObject(objectUID, objectPrivsRows);
   }

   catch (...)
   {
      return STATUS_ERROR;
   }
   
   return privStatus;

}
//*************** End of PrivMgrCommands::getPrivRowsForObject *****************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrCommands::givePrivForObjects                             *
// *                                                                           *
// *    Gives privileges (grants) associated with one or more objects to       *
// *  a new owner.                                                             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <currentOwnerID>                const int32_t                    In      *
// *    is the authID of the current owner of the object(s).                   *
// *                                                                           *
// *  <newOwnerID>                    const int32_t                    In      *
// *    is the the authID of the new owner of the object(s).                   *
// *                                                                           *
// *  <newOwnerName>                  const std::string &              In      *
// *    is the name of the new owner of the object(s).                         *
// *                                                                           *
// *  <objectUIDs>                    const std::vector<int64_t> &     In      *
// *    is a list of objects whose privileges are to be given to a new owner.  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Component privilege(s) were granted                          *
// *           *: One or more component privileges were not granted.           *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrCommands::givePrivForObjects(
   const int32_t currentOwnerID,
   const int32_t newOwnerID,
   const std::string &newOwnerName,
   const std::vector<int64_t> &objectUIDs)
   
{

PrivStatus privStatus = STATUS_GOOD;

   try
   {
      PrivMgrPrivileges objectPrivileges(getMetadataLocation(),pDiags_);
      
      privStatus = objectPrivileges.givePrivForObjects(currentOwnerID,
                                                       newOwnerID,
                                                       newOwnerName,
                                                       objectUIDs);
   }

   catch (...)
   {
      return STATUS_ERROR;
   }
   
   return privStatus;



}         
//**************** End of PrivMgrCommands::givePrivForObjects ******************

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrCommands::grantComponentPrivilege                        *
// *                                                                           *
// *    Grants the authority to perform one or more operations on a            *
// *  component to an authID.                                                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &              In      *
// *    is the component name.                                                 *
// *                                                                           *
// *  <operationNamesList>            const std::vector<std::string> & In      *
// *    is a list of component operations to be granted.                       *
// *                                                                           *
// *  <grantorID>                    const int32_t                     In      *
// *    is the authID granting the privilege.                                  *
// *                                                                           *
// *  <grantorName>                   const std::string &              In      *
// *    is the name of the authID granting the privilege.                      *
// *                                                                           *
// *  <granteeID>                     const int32_t                    In      *
// *    is the the authID being granted the privilege.                         *
// *                                                                           *
// *  <granteeName>                   const std::string &              In      *
// *    is the name of the authID being granted the privilege.                 *
// *                                                                           *
// *  <grantDepth>                    const int32_t                    In      *
// *    is the number of levels this privilege may be granted by the grantee.  *
// *  Initially this is either 0 or -1.                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Component privilege(s) were granted                          *
// *           *: One or more component privileges were not granted.           *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrCommands::grantComponentPrivilege(
   const std::string & componentName,
   const std::vector<std::string> & operationNamesList,
   const int32_t grantorID,
   const std::string & grantorName,
   const int32_t granteeID,
   const std::string & granteeName,
   const int32_t grantDepth)
   
{

PrivStatus privStatus = STATUS_GOOD;

   try
   {
      PrivMgrComponentPrivileges componentPrivileges(getMetadataLocation(),
                                                     pDiags_);
      
      privStatus = componentPrivileges.grantPrivilege(componentName,
                                                      operationNamesList,
                                                      grantorID,
                                                      grantorName,
                                                      granteeID,
                                                      granteeName,
                                                      grantDepth);
   }

   catch (...)
   {
      return STATUS_ERROR;
   }
   
   return privStatus;

}
//************* End of PrivMgrCommands::grantComponentPrivilege ****************


// ----------------------------------------------------------------------------
// method: grantObjectPrivilege
//
// Grants one or more privilege on an object to the grantee (user or TBD role)
//
// Input:
//    objectUID, objectName, objectType - identifies the object
//    granteeUID, granteeName - identifies the grantee
//    grantorUID, grantorName - identifies the grantor
//    privsList - a list of privileges to grant
//    colPrivsArray - an array of column privileges
//    isAllSpecified - grant all privileges for the object type
//    isWGOSpecified - indicates if WITH GRANT OPTION was specified
//
// Returns the status of the request
// The Trafodion diags area contains any errors that were encountered
// ----------------------------------------------------------------------------
PrivStatus PrivMgrCommands::grantObjectPrivilege (
   const int64_t objectUID,
   const std::string &objectName,
   const ComObjectType objectType,
   const int32_t granteeUID,
   const std::string &granteeName,
   const int32_t grantorUID,
   const std::string &grantorName,
   const std::vector<PrivType> &privsList,
   const std::vector<ColPrivSpec> & colPrivsArray,
   const bool isAllSpecified,
   const bool isWGOSpecified)
{
  if (!isSecurableObject(objectType))
  {
    *pDiags_ << DgSqlCode (-15455)
             << DgString0 ("GRANT")
             << DgString1 (objectName.c_str());
     return STATUS_ERROR;
  }

  PrivMgrPrivileges grantCmd(objectUID, objectName, grantorUID, metadataLocation_, pDiags_);
  grantCmd.setTrafMetadataLocation(trafMetadataLocation_);
  return grantCmd.grantObjectPriv
   (objectType, granteeUID, granteeName, grantorName, privsList, colPrivsArray, isAllSpecified, isWGOSpecified);
}

PrivStatus PrivMgrCommands::grantObjectPrivilege (
      const int64_t objectUID,
      const std::string &objectName,
      const ComObjectType objectType,
      const int32_t grantorUID,
      const int32_t granteeUID,
      const PrivMgrBitmap &objectPrivs,
      const PrivMgrBitmap &grantablePrivs)
{
  if (!isSecurableObject(objectType))
  {
    *pDiags_ << DgSqlCode (-15455)
             << DgString0 ("GRANT")
             << DgString1 (objectName.c_str());
     return STATUS_ERROR;
  }

  PrivMgrPrivileges grantCmd(objectUID, objectName, grantorUID, metadataLocation_, pDiags_);
  grantCmd.setTrafMetadataLocation(trafMetadataLocation_);
  return grantCmd.grantObjectPriv
   (objectType, granteeUID, objectPrivs, grantablePrivs);
}

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrCommands::grantRole                                      *
// *                                                                           *
// *    Grants a role to an authorization ID.                                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <roleIDs>                       const std::vector<int32_t> &     In      *
// *    is a vector of roleIDs.  The caller is responsible for ensuring all    *
// *  IDs are unique.                                                          *
// *                                                                           *
// *  <roleNames>                     const std::vector<std::string> & In      *
// *    is a list of role names.  The elements in <roleIDs> must correspond    *
// *  to the elements in <roleNames>, i.e., roleIDs[n] is the role ID for      *
// *  the role with name roleNames[n].                                         *
// *                                                                           *
// *  <grantorIDs>                    const std::vector<int32_t> &     In      *
// *    is a vector of authIDs granting the role.  The elements of the vector  *
// *  are for the corresponding role entry.                                    *
// *                                                                           *
// *  <grantorNames>                  const std::vector<std::string> & In      *
// *    is a list of names of the authID granting the role in the              *
// *  corresponding position in the role vectors.                              *
// *                                                                           *
// *  <grantorClass                   PrivAuthClass                    In      *
// *    is the auth class of the authID granting the role.  Currently only     *
// *  user is supported.                                                       *
// *                                                                           *
// *  <grantees>                      const std::vector<int32_t> &     In      *
// *    is a vector of authIDs being granted the role.  The caller is          *
// *  responsible for ensuring all IDs are unique.                             *
// *                                                                           *
// *  <granteeNames>                  const std::vector<std::string> & In      *
// *    is a list of grantee names.  The elements in <grantees> must correspond*
// *  to the elements in <granteeNames>, i.e., grantees[n] is the numeric      *
// *  auth ID for authorization ID with the name granteeNames[n].              *
// *                                                                           *
// *  <granteeClasses>                const std::vector<PrivAuthClass> & In    *
// *     is a list of classes for each grantee.  List must correspond to       *
// *  <grantees>.  Currently only user is supported.                           *
// *                                                                           *
// *  <grantDepth>                    const int64_t                    In      *
// *    is the number of levels this role may be granted by the grantee.       *
// *  Initially this is either 0 or -1.                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Role(s) were granted                                         *
// *           *: One or more roles were not granted.                          *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrCommands::grantRole(
   const std::vector<int32_t> & roleIDs,
   const std::vector<std::string> & roleNames,
   const std::vector<int32_t> & grantorIDs,
   const std::vector<std::string> & grantorNames,
   PrivAuthClass grantorClass,
   const std::vector<int32_t> & grantees,
   const std::vector<std::string> & granteeNames,
   const std::vector<PrivAuthClass> & granteeClasses,
   const int32_t grantDepth)
    
{

PrivStatus privStatus = STATUS_GOOD;

   try
   {
      PrivMgrRoles roles(trafMetadataLocation_,metadataLocation_,pDiags_);
      
      privStatus = roles.grantRole(roleIDs,
                                   roleNames,
                                   grantorIDs,
                                   grantorNames,
                                   grantorClass,
                                   grantees,
                                   granteeNames,                                 
                                   granteeClasses,                               
                                   grantDepth);                                   
   }

   catch (...)
   {
      return STATUS_ERROR;
   }
   
   return privStatus;

}
//******************** End of PrivMgrCommands::grantRole ***********************


// ----------------------------------------------------------------------------
// method: initializeAuthorizationMetadata
//
// This method creates all the metadata needed by the privilege manager
//
// Input: 
//     Location of the Trafodion OBJECTS table
//     Location of the Trafodion AUTHS table
//
// Returns the status of the request
// The Trafodion diags area contains any errors that were encountered
// ----------------------------------------------------------------------------
PrivStatus PrivMgrCommands::initializeAuthorizationMetadata(
    const std::string &objectsLocation,
    const std::string &authsLocation,
    const std::string &colsLocation,
    std::vector<std::string> &tablesCreated,
    std::vector<std::string> &tablesUpgraded)
{
   PrivMgrMDAdmin metadata(getMetadataLocation(),getDiags());
   return metadata.initializeMetadata(objectsLocation, authsLocation, colsLocation,
                                      tablesCreated, tablesUpgraded);
}
   

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrCommands::insertPrivRowsForObject                        *
// *                                                                           *
// *    Writes rows for privileges granted on an object.                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <objectUID>                 const int64_t                        In      *
// *    is the unique ID of the object whose grants are being written.         *
// *                                                                           *
// *  <objectPrivsRows>           const std::vector<ObjectPrivsRow> &  In      *
// *    is a vector of rows representing the grants for the object.            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD    : Rows were inserted                                       *
// *               *: Unable to insert, see diags.                             *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrCommands::insertPrivRowsForObject(
   const int64_t objectUID,
   const std::vector<ObjectPrivsRow> & objectPrivsRows)
   
{

PrivStatus privStatus = STATUS_GOOD;

   try
   {
      PrivMgrPrivileges objectPrivileges(objectUID, getMetadataLocation(),pDiags_);
      
      privStatus = objectPrivileges.insertPrivRowsForObject(objectUID, objectPrivsRows);
   }

   catch (...)
   {
      return STATUS_ERROR;
   }
   
   return privStatus;

}
//************* End of PrivMgrCommands::insertPrivRowsForObject ****************



// ----------------------------------------------------------------------------
// method: isPrivMgrTable
//
// Input:
//   objectName - name of object to check
//
// this method returns true if the passed in object name is a privilege
// manager metadata object, false otherwise.
// ----------------------------------------------------------------------------
bool PrivMgrCommands::isPrivMgrTable(const std::string &objectName)
{
  char theQuote = '"';

  // Delimited name issue.  The passed in objectName may enclose name parts in
  // double quotes even if the name part contains only [a-z][A-Z][0-9]_
  // characters. The same is true for the stored metadataLocation_.
  // To allow equality checks to work, we strip off the double quote delimiters
  // from both names. Fortunately, the double quote character is not allowed in
  // any SQL identifier except as delimiters - so this works.
  std::string nameToCheck(objectName);
  nameToCheck.erase(std::remove(nameToCheck.begin(), nameToCheck.end(), theQuote), nameToCheck.end());

  size_t numTables = sizeof(privMgrTables)/sizeof(PrivMgrTableStruct);
  for (int ndx_tl = 0; ndx_tl < numTables; ndx_tl++)
  {
    const PrivMgrTableStruct &tableDefinition = privMgrTables[ndx_tl];

    std::string mdTable = metadataLocation_;
    mdTable.erase(std::remove(mdTable.begin(), mdTable.end(), theQuote), mdTable.end());

    mdTable += std::string(".") + tableDefinition.tableName;   
    if (mdTable == nameToCheck)
      return true;
  }
  return false;
}

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrCommands::registerComponent                              *
// *                                                                           *
// *    Registers (adds) a component to the list of components known to        *
// *  Privilege Manager.  Authority to perform operations within the           *
// *  component may be granted to one or more authorization IDs.               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the name of the component.  Name is assumed to be upper case.       *
// *                                                                           *
// *  <isSystem>                      const bool                      In       *
// *    is true if this is a system level component, false otherwise.  If a    *
// *  component is system level, code in Trafodion can assume is it always     *
// *  defined.                                                                 *
// *                                                                           *
// *  System components may only be unregistered by DB__ROOT.  Only DB__ROOT   *
// *  may register a system component.                                         *
// *                                                                           *
// *  <componentDescription>          const std::string &             In       *
// *    is a description of the component.                                     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Component name was registered.                               *
// *           *: Unable to register component name, see diags.                *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrCommands::registerComponent(
   const std::string &componentName,
   const bool isSystem,
   const std::string &componentDescription)
   
{

PrivStatus privStatus = STATUS_GOOD;

   try
   {
      PrivMgrComponents components(getMetadataLocation(),pDiags_);
      
      privStatus = components.registerComponent(componentName,
                                                isSystem,
                                                componentDescription);
   }
   catch (...)
   {
      return STATUS_ERROR;
   }
   
   return privStatus;
  
}
//**************** End of PrivMgrCommands::registerComponent *******************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrCommands::revokeComponentPrivilege                       *
// *                                                                           *
// *    Revokes the authority to perform one or more operations on a           *
// *  component from an authID.                                                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &              In      *
// *    is the component name.                                                 *
// *                                                                           *
// *  <operationNamesList>            const std::vector<std::string> & In      *
// *    is a list of component operations to be revoked.                       *
// *                                                                           *
// *  <grantorID>                     const int32_t                    In      *
// *    is the authID revoking the privilege.                                  *
// *                                                                           *
// *  <granteeID>                     const int32_t                    In      *
// *    is the authID the privilege is being revoked from.                     *
// *                                                                           *
// *  <isGOFSpecified>                const bool                       In      *
// *    is true if admin rights are being revoked.                             *
// *                                                                           *
// *  <dropBehavior>                  PrivDropBehavior                 In      *
// *    indicates whether restrict or cascade behavior is requested.           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Component privilege(s) were revoked                          *
// *           *: One or more component privileges were not revoked.           *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrCommands::revokeComponentPrivilege(
   const std::string & componentName,
   const std::vector<std::string> & operationNamesList,
   const int32_t grantorID,
   const int32_t granteeID,
   const bool isGOFSpecified,
   PrivDropBehavior dropBehavior) 
   
{

PrivStatus privStatus = STATUS_GOOD;

   try
   {
      PrivMgrComponentPrivileges componentPrivileges(getMetadataLocation(),
                                                     pDiags_);
      privStatus = componentPrivileges.revokePrivilege(componentName,
                                                       operationNamesList,
                                                       grantorID,
                                                       granteeID,
                                                       isGOFSpecified, 0,
                                                       dropBehavior);
   }

   catch (...)
   {
      return STATUS_ERROR;
   }
   
   return privStatus;

}
//************* End of PrivMgrCommands::revokeComponentPrivilege ***************

// ----------------------------------------------------------------------------
// method: revokeObjectPrivilege
//
// Revokes one or more privilege on an object to the grantee (user or TBD role)
//
// Input:
//    objectUID, objectName, objectType - identifies the object
//    granteeUID - identifies the grantee
//    grantorUID - identifies the grantor
//    privsList - a list of privileges to revoke
//    isAllSpecified - grant all privileges for the object type
//    isGOFSpecified - indicates if GRANT OPTION FOR  was specified
//
// Returns the status of the request
// The Trafodion diags area contains any errors that were encountered
// ----------------------------------------------------------------------------
PrivStatus PrivMgrCommands::revokeObjectPrivilege(
    const int64_t objectUID,
    const std::string &objectName,
    const ComObjectType objectType,
    const int32_t granteeUID,
    const std::string & granteeName,
    const int32_t grantorUID,
    const std::string & grantorName,
    const std::vector<PrivType> &privList,
    const std::vector<ColPrivSpec> & colPrivsArray,
    const bool isAllSpecified,
    const bool isGOFSpecified)
{
  if (!isSecurableObject(objectType))
  {
    *pDiags_ << DgSqlCode (-15455)
             << DgString0 ("REVOKE")
             << DgString1 (objectName.c_str());
     return STATUS_ERROR;
  }

  // set up privileges class
  PrivMgrPrivileges revokeCmd(objectUID, objectName, grantorUID, metadataLocation_, pDiags_);
  revokeCmd.setTrafMetadataLocation(trafMetadataLocation_);
  return revokeCmd.revokeObjectPriv(objectType,
                                    granteeUID,
                                    granteeName,
                                    grantorName, 
                                    privList,
                                    colPrivsArray,
                                    isAllSpecified, 
                                    isGOFSpecified); 
}

// ----------------------------------------------------------------------------
// method: revokeObjectPrivilege
//
// Revokes all privileges for an object - called as part of dropping the object
//
// Input:
//    objectUID, objectName - identifies the object
//    grantorUID - identifies the grantor
//
// Returns the status of the request
// The Trafodion diags area contains any errors that were encountered
// ----------------------------------------------------------------------------
PrivStatus PrivMgrCommands::revokeObjectPrivilege(
    const int64_t objectUID,
    const std::string &objectName,
    const int32_t grantorUID)
{
  // If we are revoking privileges on the authorization tables,
  // just return STATUS_GOOD.  Object_privileges will go away.
  if (isPrivMgrTable(objectName))
    return STATUS_GOOD;

  // set up privileges class
  PrivMgrPrivileges revokeCmd(objectUID, objectName, grantorUID, metadataLocation_, pDiags_);
  revokeCmd.setTrafMetadataLocation(trafMetadataLocation_);
  return revokeCmd.revokeObjectPriv();
}

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrCommands::revokeRole                                     *
// *                                                                           *
// *    Revokes one or more roles from one or more authorization IDs.          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <roleIDs>                    const std::vector<int32_t> &        In      *
// *    is a vector of roleIDs.  The caller is responsible for ensuring all    *
// *  IDs are unique.                                                          *
// *                                                                           *
// *  <granteeIDs>                 const std::vector<int32_t> &        In      *
// *    is a vector of granteeIDs.  The caller is responsible for ensuring all *
// *  IDs are unique.                                                          *
// *                                                                           *
// *  <granteeClasses>             const std::vector<PrivAuthClass> &  In      *
// *    is a vector of PrivAuthClass corresponding the granteeID in the same   *
// *  position in the granteeIDs vector.                                       *
// *                                                                           *
// *  <grantorIDs>                    const std::vector<int32_t> &     In      *
// *    is a vector of authIDs that granted the role.  The elements of the     *
// *  vector are for the corresponding role entry.                             *
// *                                                                           *
// *  <isGOFSpecified>             const bool                          In      *
// *    is true if admin rights are being revoked.                             *
// *                                                                           *
// *  <newGrantDepth>              const int32_t                       In      *
// *    is the number of levels this privilege may be granted by the grantee.  *
// *  Initially this is always 0 when revoking.                                *
// *                                                                           *
// *  <dropBehavior>               PrivDropBehavior                    In      *
// *    indicates whether restrict or cascade behavior is requested.           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Role(s) were revoked                                         *
// *           *: One or more roles were not revoked.                          *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrCommands::revokeRole(
   const std::vector<int32_t> & roleIDs,
   const std::vector<int32_t> & granteeIDs,
   const std::vector<PrivAuthClass> & granteeClasses,
   const std::vector<int32_t> & grantorIDs,
   const bool isGOFSpecified,
   const int32_t newGrantDepth,
   PrivDropBehavior dropBehavior) 
    
{

PrivStatus privStatus = STATUS_GOOD;

   try
   {
      PrivMgrRoles roles(trafMetadataLocation_,metadataLocation_,pDiags_);
      
      privStatus = roles.revokeRole(roleIDs,
                                    granteeIDs,
                                    granteeClasses,
                                    grantorIDs,
                                    isGOFSpecified,
                                    newGrantDepth,
                                    dropBehavior);
   }

   catch (...)
   {
      return STATUS_ERROR;
   }
   
   return privStatus;

}
//******************** End of PrivMgrCommands::revokeRole **********************

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponents::unregisterComponent                          *
// *                                                                           *
// *    Removes the component from the list of components known to the         *
// *  Privilege Manager.  Operations of the component and privileges           *
// *  granted for those operations are automatically removed if the            *
// *  CASCADE option is specified.                                             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the name of the component.  Name is assumed to be upper case.       *
// *                                                                           *
// *  <dropBehavior>                  PrivDropBehavior                In       *
// *    indicates whether restrict or cascade behavior is requested.           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Component was unregistered.                                  *
// *           *: Unable to unregister component, see diags.                   *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrCommands::unregisterComponent(
   const std::string & componentName,
   PrivDropBehavior dropBehavior)

{

PrivStatus privStatus = STATUS_GOOD;

   try
   {
      PrivMgrComponents components(getMetadataLocation(),pDiags_);

      privStatus = components.unregisterComponent(componentName,dropBehavior);
   }

   catch (...)
   {
      return STATUS_ERROR;
   }

   return privStatus;

}
//*************** End of PrivMgrCommands::unregisterComponent ******************

