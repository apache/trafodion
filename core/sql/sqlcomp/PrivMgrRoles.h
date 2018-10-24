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

#ifndef PRIVMGR_ROLES_H
#define PRIVMGR_ROLES_H
#include "PrivMgrMD.h"
class PrivMgrMDTable;

#include <string>
#include <vector>

class ComDiagsArea;

// *****************************************************************************
// * Class:         PrivMgrRoles                                               *
// * Description:  This class represents roles.                                *
// *                                                                           *
// *****************************************************************************
class PrivMgrRoles : public PrivMgr
{
public:

// -------------------------------------------------------------------
// Constructors and destructors:
// -------------------------------------------------------------------
   PrivMgrRoles( 
      const std::string & trafMetadataLocation,
      const std::string & metadataLocation,
      ComDiagsArea * pDiags);
   PrivMgrRoles(const PrivMgrRoles & other);
   virtual ~PrivMgrRoles();

// -------------------------------------------------------------------
// Public functions:
// -------------------------------------------------------------------
   
   PrivStatus fetchRolesForAuth(
      const int32_t authID,
      std::vector<std::string> & roleNames,
      std::vector<int32_t> & roleIDs,
      std::vector<int32_t> & grantDepths,
      std::vector<int32_t> & grantees);

   PrivStatus fetchGranteesForRole(
      const int32_t roleID,
      std::vector<std::string> & granteeNames,
      std::vector<int32_t> & grantorIDs,
      std::vector<int32_t> & grantDepths);

   PrivStatus fetchGranteesForRoles(
      const std::vector<int32_t> & roleIDs,
      std::vector<int32_t> & granteeIDs,
      bool includeSysGrantor = true);

   PrivStatus grantRole(
      const std::vector<int32_t> & roleIDs,
      const std::vector<std::string> & roleNames,
      const std::vector<int32_t> & grantorIDs,
      const std::vector<std::string> & grantorNames,
      PrivAuthClass grantorClass,
      const std::vector<int32_t> & grantees,
      const std::vector<std::string> & granteeNames,
      const std::vector<PrivAuthClass> & granteeClasses,
      const int32_t grantDepth); 
      
   PrivStatus grantRoleToCreator(
      const int32_t roleID,
      const std::string & roleName,
      const int32_t granteeID,
      const std::string granteeName); 
     
   bool hasRole(
      int32_t authID,
      int32_t roleID);
   
   bool isGranted(
      const int32_t roleID,
      const bool shouldExcludeGrantsBySystem = false);
      
   bool isUserGrantedAnyRole(const int32_t authID);
   
   PrivStatus populateCreatorGrants(
      const std::string &authsLocation,
      const std::vector<std::string> &rolesToAdd);      
     
   PrivStatus revokeRole(
      const std::vector<int32_t> & roleIDs,
      const std::vector<int32_t> & granteeIDs,
      const std::vector<PrivAuthClass> & granteeClasses,
      const std::vector<int32_t> & grantorIDs,
      const bool isGOFSpecified,
      const int32_t newGrantDepth,
      PrivDropBehavior dropBehavior);
       
   PrivStatus revokeRoleFromCreator(
      const int32_t roleID,
      const int32_t granteeID); 
     
private: 
   PrivMgrRoles();
   
   bool areRemainingGrantedPrivsSufficient(
      const std::string whereClause,
      PrivType privType);
      
   bool dependentObjectsExist(
      const int32_t userID,
      const int32_t roleID,
      PrivDropBehavior dropBehavior);
       
   bool hasWGO(
      const int32_t authID,
      const int32_t roleID);

   bool grantExists(
      const int32_t roleID,
      const int32_t grantorID,
      const int32_t granteeID,
      int32_t & grantDepth);
       
   PrivStatus  revokeAllForGrantor(
      const int32_t grantorID,
      const int32_t roleID,
      const bool isGOFSpecified,
      const int32_t newGrantDepth,
      PrivDropBehavior dropBehavior); 

// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
      
std::string        fullTableName_;
PrivMgrMDTable & myTable_;
};
#endif // PRIVMGR_ROLES_H


