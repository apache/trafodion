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
//// @@@ END COPYRIGHT @@@
//*****************************************************************************

#ifndef PRIVMGR_PRIVILEGES_H
#define PRIVMGR_PRIVILEGES_H

#include "PrivMgrDefs.h"
#include "PrivMgrMD.h"
#include "PrivMgrMDTable.h"
#include "PrivMgrDesc.h"
#include "ComSmallDefs.h"
#include "ComViewColUsage.h"
#include "CmpDDLCatErrorCodes.h"

#include <string>
#include <bitset>
#include <vector>
#include <set>

class ComSecurityKey;
class NATable;
class PrivMgrObjectInfo;

// *****************************************************************************
// *
// * File:         PrivMgrPrivileges.h
// * Description:  This file contains the class that accesses and maintains 
// *               access rights by granting, revoking, and obtaining current
//                 privileges               
// * Language:     C++
// *
// *****************************************************************************

class UIDAndPrivs
{
public:
   int64_t objectUID;
   PrivObjectBitmap privsBitmap;
};

// *****************************************************************************
// * Class:         PrivMgrPrivileges
// * Description:  This class represents the access rights for objects
// *****************************************************************************
class PrivMgrPrivileges : public PrivMgr
{
public:
   // Set default column privileges for a bitmap based on a table or view
   static void setColumnPrivs(PrivColumnBitmap &bitmap)
   {
      bitmap.reset();
      bitmap.set(SELECT_PRIV);
      bitmap.set(INSERT_PRIV);
      bitmap.set(UPDATE_PRIV);
      bitmap.set(REFERENCES_PRIV);
   }
 enum ChosenPrivs { ORIGINAL_PRIVS, CURRENT_PRIVS };

 //
 // -------------------------------------------------------------------
 // Constructors and destructor:
 // -------------------------------------------------------------------
   PrivMgrPrivileges();

   PrivMgrPrivileges(
      const int64_t objectUID,
      const std::string &objectName,
      const int32_t grantorID,
      const std::string &metadataLocation,
      ComDiagsArea * pDiags = NULL);

   PrivMgrPrivileges (
      const PrivMgrObjectInfo &objectInfo,
      const std::string &metadataLocation,
      ComDiagsArea *pDiags = NULL);

   PrivMgrPrivileges(
      const int64_t objectUID,
      const std::string &metadataLocation,
      ComDiagsArea *pDiags = NULL);

   PrivMgrPrivileges(
      const std::string &metadataLocation,
      ComDiagsArea *pDiags = NULL);

   PrivMgrPrivileges(const PrivMgrPrivileges &other);

   virtual ~PrivMgrPrivileges();

  // -------------------------------------------------------------------
  // Public functions:
  // -------------------------------------------------------------------
   PrivStatus getGrantorDetailsForObject(
      const bool isGrantedBySpecified,
      const std::string grantedByName,
      const int_32 objectOwner,
      int_32 &effectiveGrantorID,
      std::string &effectiveGrantorName);

   PrivStatus getPrivBitmaps(
      const std::string & whereClause,
      const std::string & orderByClause,
      std::vector<PrivObjectBitmap> & privBitmaps);
      
   PrivStatus getPrivsOnObject (
      const ComObjectType objectType,
      std::vector<PrivMgrDesc> & privDescs );
 
   PrivStatus getPrivsOnObjectForUser(
      const int64_t objectUID,
      ComObjectType objectType,
      const int32_t userID,
      PrivMgrDesc &privsForTheUser);

   PrivStatus getPrivRowsForObject(
      const int64_t objectUID,
      std::vector<ObjectPrivsRow> & objectPrivsRows);

   PrivStatus getPrivTextForObject(
      const PrivMgrObjectInfo &objectInfo,
      std::string &privilegeText);

   PrivStatus givePrivForObjects(
      const int32_t currentOwnerID,
      const int32_t newOwnerID,
      const std::string &newOwnerName,
      const std::vector<int64_t> &objectUIDs);
         
   PrivStatus grantColumnPriv(
      const ComObjectType objectType,
      const int32_t granteeID,
      const std::string &granteeName,
      const std::string &grantorName,
      const PrivMgrDesc &privsToGrant,
      const bool isWGOSpecified);
  
   PrivStatus grantObjectPriv(
      const ComObjectType objectType,
      const int32_t granteeID,
      const std::string &granteeName,
      const std::string &grantorName,
      const std::vector<PrivType> &privList,
      const std::vector<ColPrivSpec> & colPrivsArray,
      const bool isAllSpecified,
      const bool isWGOSpecified);
       
  PrivStatus grantObjectPriv(
      const ComObjectType objectType,
      const int32_t granteeID,
      const PrivObjectBitmap privsBitmap,
      const PrivObjectBitmap grantableBitmap);

   PrivStatus grantToOwners(
      const ComObjectType objectType,
      const Int32 granteeID,
      const std::string & granteeName,
      const Int32 ownerID,
      const std::string & ownerName,
      const Int32 creatorID,
      const std::string & creatorName);
      
   PrivStatus insertPrivRowsForObject(
      const int64_t objectUID,
      const std::vector<ObjectPrivsRow> & objectPrivsRows);
   
   PrivStatus populateObjectPriv(
      const std::string &objectsLocation,
      const std::string &authsLocation);
      
   PrivStatus revokeColumnPriv(
         const ComObjectType objectType,
         const int32_t granteeID,
         const std::string & granteeName,
         const std::string & grantorName,
         const PrivMgrDesc &privsToRevoke,
         const bool isWGOSpecified);
         
   PrivStatus revokeObjectPriv(
      const ComObjectType objectType,
      const int32_t granteeID,
      const std::string & granteeName,
      const std::string & grantorName,
      const std::vector<PrivType> &privList,
      const std::vector<ColPrivSpec> & colPrivsArray,
      const bool isAllSpecified,
      const bool isGOFSpecified);
       
   PrivStatus revokeObjectPriv();

   PrivStatus sendSecurityKeysToRMS(
     const int32_t granteeID, const PrivMgrDesc &listOfRevokedPrivileges);

   void setTrafMetadataLocation (const std::string &trafMetadataLocation)
    { trafMetadataLocation_ = trafMetadataLocation; }

  // -------------------------------------------------------------------
  // helpers
  // -------------------------------------------------------------------
  bool isAuthIDGrantedPrivs(
    const int32_t authID,
    std::vector<int64_t> &objectUIDs);

protected:

   PrivStatus convertPrivsToDesc( 
     const ComObjectType objectType,
     const bool isAllSpecified,
     const bool isWGOSpecified,
     const bool isGOFSpecified,
     const std::vector<PrivType> privsList,
     const std::vector<ColPrivSpec> & colPrivsList,
     PrivMgrDesc &privsToGrant);

   PrivStatus getPrivsFromAllGrantors(
     const int64_t objectUID,
     ComObjectType objectType,
     const int32_t grantee,
     const std::vector<int32_t> & roleIDs,
     PrivMgrDesc &privs,
     bool & hasManagePrivileges
     );
          
   PrivStatus getUserPrivs(
     ComObjectType objectType,
     const int32_t grantee,
     const std::vector<int32_t> & roleIDs,
     PrivMgrDesc &privs,
     bool & hasManagePrivileges
     );
     
private: 

// -------------------------------------------------------------------
// Private functions:
// -------------------------------------------------------------------

  bool checkColumnRevokeRestrict (
    int32_t granteeID,
    const NAList<PrivMgrCoreDesc> &colPrivsToRevoke,
    std::vector <PrivMgrMDRow *> &rowList );

  bool checkRevokeRestrict (
    PrivMgrMDRow &rowIn,
    std::vector<PrivMgrMDRow *> &rowList );

  PrivStatus dealWithConstraints (
    const ObjectUsage &objectUsage,
    std::vector<ObjectUsage *> &listOfAffectedObjects);

  PrivStatus dealWithUdrs (
    const ObjectUsage &objectUsage,
    std::vector<ObjectUsage *> &listOfAffectedObjects);

  PrivStatus dealWithViews (
    const ObjectUsage &objectUsage,
    const PrivCommand command,
    const int32_t grantorID,
    std::vector<ObjectUsage *> &listOfAffectedObjects);

  void deleteListOfAffectedObjects(
    std::vector<ObjectUsage *> listOfAffectedObjects)
  {
    while(!listOfAffectedObjects.empty()) 
       delete listOfAffectedObjects.back(), listOfAffectedObjects.pop_back();
  }

  PrivStatus gatherConstraintPrivileges(
    ObjectUsage &constraintUsage,
    const std::vector<ObjectUsage *> listOfAffectedObjects);

  PrivStatus gatherViewColUsages(
    ObjectReference *objectRef,
    ViewUsage &viewUsage,
    std::vector<ComViewColUsage> &viewColUsages);

  PrivStatus gatherViewPrivileges(
    ViewUsage &viewUsage,
    const PrivCommand command,
    const int32_t grantorID,
    const std::vector<ObjectUsage *> listOfAffectedObjects);

  PrivStatus generateColumnRowList();
  PrivStatus generateObjectRowList();

  PrivStatus getAffectedObjects(
    const ObjectUsage &objectUsage,
    const PrivCommand command,
    std::vector<ObjectUsage *> &listOfAffectedObjects);

  void getColRowsForGranteeOrdinal(
    const int32_t granteeID,
    const int32_t columnOrdinal,
    const std::vector<PrivMgrMDRow *> &columnRows,
    const std::vector<int32_t> &roleIDs,
    std::vector<PrivMgrMDRow *> &rowList);

  PrivStatus getColumnRowList(
    const int64_t objectUID,
    std::vector<PrivMgrMDRow *> &columnRows);

  PrivStatus getDistinctIDs(
    const std::vector <PrivMgrMDRow *> &objectRowList,
    const std::vector <PrivMgrMDRow *> &columnRowList,
    std::vector<int32_t> &userIDs,
    std::vector<int32_t> &roleIDs);

  PrivStatus getGrantedPrivs(
    const int32_t granteeID,
    PrivMgrMDRow &row);

  PrivStatus getGranteesForViewUsage (
    const ViewUsage &viewUsage,
    std::set<int32_t> &granteeList);

  PrivStatus getObjectRowList(
    const int64_t objectUID, 
    std::vector<PrivMgrMDRow *> &objectRows);

  PrivStatus getRoleIDsForUserID(
     int32_t userID,
     std::vector<int32_t> & roleIDs);
     
  PrivStatus getRolesToCheck(
    const int32_t grantorID,
    const std::vector<int32_t> & roleIDs,
    const ComObjectType schemaType,
    std::string &rolesToCheck);

  PrivStatus getRowsForGrantee(
    const int64_t objectUID,
    const int32_t granteeID,
    const bool isObjectTable,
    const std::vector<int32_t> & roleIDs,
    std::vector<PrivMgrMDRow *> &rowList);
    
  void getTreeOfGrantors(
    const int32_t granteeID,
    std::set<int32_t> &listOfGrantors);

  PrivStatus givePriv(
     const int32_t currentOwnerID,
     const int32_t newOwnerID,
     const std::string &newOwnerName,
     const int64_t objectUID);    

  PrivStatus initGrantRevoke(
    const ComObjectType objectType,
    const int32_t granteeID,
    const std::string &grantorName,
    const std::vector<PrivType> &privList,
    const std::vector<ColPrivSpec> & colPrivsArray,
    const bool isAllSpecified,
    const bool isGOSpecified,
    const bool isGrant,
    PrivMgrDesc &privsToApply,
    PrivMgrDesc &privsOfTheGrantor,
    std::vector<int32_t> & roleIDs);

  void reportPrivWarnings(
    const PrivMgrDesc &origPrivs,
    const PrivMgrDesc &actualPrivs,
    const CatErrorCode warningCode);

  void scanColumnBranch( const PrivType pType,
    const int32_t& grantor,
    const std::set<int32_t> &listOfColumnOrdinals,
    const std::vector<PrivMgrMDRow *> & rowList  );

  void scanColumnPublic(
    const PrivType pType,
    const std::set<int32_t> &listOfColumnOrdinals,
    const std::vector<PrivMgrMDRow *>& rowList );

  void scanObjectBranch( 
    const PrivType pType,
    const int32_t& grantor,
    const std::vector<PrivMgrMDRow *>& rowList  );

  void scanPublic( 
    const PrivType pType, // in
    const std::vector<PrivMgrMDRow *>& rowList );    // in

  void summarizeColPrivs(
    const ObjectReference &objRef,
    const int32_t granteeID,
    const int32_t grantorID,
    const std::vector<int32_t> &roleIDs,
    const std::vector<ObjectUsage *> &listOfAffectedObjects,
    std::vector<ColumnReference *> &columnReferences);

  PrivStatus summarizeCurrentAndOriginalPrivs(
    const int64_t objectUID,
    const int32_t granteeID,
    const int32_t grantorID,
    const std::vector<int32_t> & roleIDs,
    const std::vector<ObjectUsage *> listOfChangedPrivs,
    PrivMgrDesc &summarizedOriginalPrivs,
    PrivMgrDesc &summarizedCurrentPrivs);

  PrivStatus updateDependentObjects(
    const ObjectUsage &objectUsage,
    const PrivCommand command);

// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
      
int64_t        objectUID_;
std::string    objectName_;
int32_t        grantorID_;   // is this needed as a member

std::string    objectTableName_;
std::string    columnTableName_;

std::vector<PrivMgrMDRow *> objectRowList_;
std::vector<PrivMgrMDRow *> columnRowList_;

};
#endif // PRIVMGR_PRIVILEGES_H









