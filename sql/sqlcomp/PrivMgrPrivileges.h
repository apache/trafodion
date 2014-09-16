//*****************************************************************************
// @@@ START COPYRIGHT @@@
//
//// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
////
////  Licensed under the Apache License, Version 2.0 (the "License");
////  you may not use this file except in compliance with the License.
////  You may obtain a copy of the License at
////
////      http://www.apache.org/licenses/LICENSE-2.0
////
////  Unless required by applicable law or agreed to in writing, software
////  distributed under the License is distributed on an "AS IS" BASIS,
////  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
////  See the License for the specific language governing permissions and
////  limitations under the License.
////
//// @@@ END COPYRIGHT @@@
//*****************************************************************************

#ifndef PRIVMGR_PRIVILEGES_H
#define PRIVMGR_PRIVILEGES_H

#include <string>
#include <bitset>
#include "PrivMgrMDDefs.h"
#include "PrivMgrDefs.h"
#include "PrivMgrMD.h"
#include "PrivMgrMDTable.h"
#include "PrivMgrDesc.h"

class ComSecurityKey;

// *****************************************************************************
// *
// * File:         PrivMgrPrivileges.h
// * Description:  This file contains the class that accesses and maintains 
// *               access rights by granting, revoking, and obtaining current
//                 privileges               
// * Language:     C++
// *
// *****************************************************************************

// *****************************************************************************
// * Class:         PrivMgrPrivileges
// * Description:  This class represents the access rights for objects
// *****************************************************************************
class PrivMgrPrivileges : public PrivMgr
{
public:

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

   PrivMgrPrivileges(
      const std::string &metadataLocation,
      ComDiagsArea *pDiags = NULL);

   PrivMgrPrivileges(const PrivMgrPrivileges &other);

   virtual ~PrivMgrPrivileges();

  // -------------------------------------------------------------------
  // Public functions:
  // -------------------------------------------------------------------
   PrivStatus buildSecurityKeys(
      const int32_t granteeID, 
      const PrivMgrCoreDesc &privs,
      std::vector <ComSecurityKey *> & secKeySet);
      
   PrivStatus grantObjectPriv(
      const std::string &objectType,
      const int32_t granteeID,
      const std::string &granteeName,
      const std::string &grantorName,
      const std::vector<std::string> &privList,
      const bool isAllSpecified,
      const bool isWGOSpecified);
       
  PrivStatus grantObjectPriv(
      const std::string &objectType,
      const int32_t granteeID,
      const std::string &granteeName,
      const PrivMgrBitmap privsBitmap,
      const PrivMgrBitmap grantableBitmap);

   PrivStatus grantSelectOnAuthsToPublic(
      const std::string & objectsLocation,
      const std::string & authsLocation);
      
   PrivStatus revokeObjectPriv(
      const std::string &objectType,
      const int32_t granteeID,
      const std::vector<std::string> &privList,
      const bool isAllSpecified,
      const bool isGOFSpecified);
       
   PrivStatus revokeObjectPriv();

   PrivStatus populateObjectPriv(
      const std::string &objectsLocation,
      const std::string &authsLocation);
 
   PrivStatus getPrivTextForObject(std::string &privilegeText);

   PrivStatus getPrivsOnObjectForUser(
      const int64_t objectUID,
      const int32_t userID,
      PrivMgrBitmap &userPrivs,
      PrivMgrBitmap &grantablePrivs);

   bool isAuthIDGrantedPrivs(const int32_t authID);
   
  PrivStatus sendSecurityKeysToRMS(
     const int32_t granteeID, const PrivMgrDesc &listOfRevokedPrivileges);

  // -------------------------------------------------------------------
  // helpers
  // -------------------------------------------------------------------
  bool isSystemUser(int32_t userID) { return (userID == -2); }
  bool isPublicUser(int32_t userID) { return (userID == -1); }

protected:

   PrivStatus getUserPrivs(
     const int32_t grantee,
     PrivMgrDesc &privs,
     const bool withRoles = false
     );
     
   PrivStatus getPrivsFromAllGrantors(
     const int64_t objectUID,
     const int32_t grantee,
     PrivMgrDesc &privs,
     const bool withRoles = false
     );
          
   PrivStatus convertPrivsToDesc( 
     const std::string objectType,
     const bool isAllSpecified,
     const bool isWGOSpecified,
     const bool isGOFSpecified,
     const std::vector<std::string> privs,
     PrivMgrDesc &privsToGrant);

private: 

// -------------------------------------------------------------------
// Private functions:
// -------------------------------------------------------------------

  bool checkRevokeRestrict (
    PrivMgrMDRow &rowIn,
    std::vector<PrivMgrMDRow *> &rowList );

  PrivStatus getGrantedPrivs(
    const int32_t granteeID,
    PrivMgrMDRow &row);

  void scanObjectBranch( 
    const PrivType pType, // in
    const int32_t& grantor,              // in
    const std::vector<PrivMgrMDRow *>& rowList  );   // in

  void scanPublic( 
    const PrivType pType, // in
    const std::vector<PrivMgrMDRow *>& rowList );    // in

  PrivStatus getAffectedObjects(
    const ObjectUsage &objectUsage,
    std::vector<ObjectUsage *> &listOfAffectedObjects);

  void deleteListOfAffectedObjects(
    std::vector<ObjectUsage *> listOfAffectedObjects)
  {
    while(!listOfAffectedObjects.empty()) 
       delete listOfAffectedObjects.back(), listOfAffectedObjects.pop_back();
  }

  PrivStatus gatherViewPrivileges(
    ViewUsage &viewUsage,
    const std::vector<ObjectUsage *> listOfAffectedObjects);

  PrivStatus getRowsForGrantee(
    const int64_t objectUID,
    const int32_t granteeID,
    const bool withroles,
    std::vector<PrivMgrMDRow *> &rowList);

  void deleteRowsForGrantee(std::vector<PrivMgrMDRow *> rowList)
  {
    while(!rowList.empty())
       delete rowList.back(), rowList.pop_back();
  }

  PrivStatus summarizeCurrentAndOriginalPrivs(
    const int64_t objectUID,
    const int32_t granteeID,
    const bool withRoles,
    const std::vector<ObjectUsage *> listOfChangedPrivs,
    PrivMgrDesc &summarizedOriginalPrivs,
    PrivMgrDesc &summarizedCurrentPrivs);

// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
      
int64_t        objectUID_;
std::string    objectName_;
int32_t        grantorID_;   // is this needed as a member

std::string    fullTableName_;

};
#endif // PRIVMGR_PRIVILEGES_H









