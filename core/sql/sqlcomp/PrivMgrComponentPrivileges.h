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

#ifndef PRIVMGR_COMPONENTPRIVILEGES_H
#define PRIVMGR_COMPONENTPRIVILEGES_H
#include "PrivMgrMD.h"
#include "PrivMgrDefs.h"

#include <string>
#include <vector>

class PrivMgrMDTable;
class ComDiagsArea;

// *****************************************************************************
// * Class:         PrivMgrComponentPrivileges                                 *
// * Description:  This class represents component privileges.                 *
// *                                                                           *
// *               The authority to perform component specific operations      *
// *               may be granted to authIDs (currently only users) as well    *
// *               as revoked.                                                 *
// *                                                                           *
// *****************************************************************************
class PrivMgrComponentPrivileges : public PrivMgr
{
public:

// -------------------------------------------------------------------
// Constructors and destructors:
// -------------------------------------------------------------------
   PrivMgrComponentPrivileges();

   PrivMgrComponentPrivileges( 
      const std::string & metadataLocation,
      ComDiagsArea * pDiags = NULL);
   PrivMgrComponentPrivileges(const PrivMgrComponentPrivileges & other);
   virtual ~PrivMgrComponentPrivileges();

// -------------------------------------------------------------------
// Public functions:
// -------------------------------------------------------------------
   void clear();
   
  PrivStatus describeComponentPrivileges (
      const std::string & componentUIDString,
      const std::string & componentName,
      const std::string & operationCode, 
      const std::string & operationName,
      std::vector<std::string> & outlines);
  
   PrivStatus dropAll();
   
   PrivStatus dropAllForComponent(const std::string & componentUID);
   
   PrivStatus dropAllForOperation(
      const std::string & componentUID,
      const std::string & operationCode);
  
   bool dropAllForGrantee(const int32_t granteeID);

   bool findByNames(
      const std::string & componentName,
      const std::string & operationName);
      
   int64_t getCount( const int32_t componentUID = INVALID_COMPONENT_UID );
     
   void getSQLCompPrivs(
      const int32_t                granteeID,
      const std::vector<int32_t> & roleIDs,
      PrivObjectBitmap           & DMLPrivs,
      bool                       & hasManagePrivPriv,
      bool                       & hasSelectMetadata,
      bool                       & hasAnyManagePriv);

   PrivStatus grantPrivilege(
      const std::string & componentName,
      const std::vector<std::string> & operations,
      const int32_t grantorID,
      const std::string & grantorName,
      const int32_t granteeID,
      const std::string & granteeName,
      const int32_t grantDepth);
     
   PrivStatus grantPrivilegeInternal(
      const int64_t componentUID,
      const std::vector<std::string> & operationCodes,
      const int32_t grantorIDIn,
      const std::string & grantorName,
      const int32_t granteeID,
      const std::string & granteeName,
      const int32_t grantDepth,
      const bool checkExistence);
      
   PrivStatus grantPrivilegeToCreator(
      const int64_t componentUID,
      const std::string & operationCode,
      const int32_t granteeID,
      const std::string & granteeName);
      
   bool hasPriv(
      const int32_t authID,
      const std::string & componentUIDString,
      const std::string & operationCode);
   
   bool hasSQLPriv(
      const int32_t authID,
      const SQLOperation operation,
      const bool includeRoles = true);
      
   bool isAuthIDGrantedPrivs(const int32_t authID);
   
   bool isGranted(
      const std::string & componentUID,
      const std::string & operationCode,
      const bool shouldExcludeGrantsBySystem = false);
     
   PrivStatus revokePrivilege(
      const std::string & componentName,
      const std::vector<std::string> & operations,
      const int32_t grantorID,
      const int32_t granteeID, 
      const bool isGOFSpecified,
      const int32_t newGrantDepth,
      PrivDropBehavior dropBehavior); 
     
private: 
   bool grantExists(
      const std::string componentUIDString,
      const std::string operationCode,
      const int32_t grantorID,
      const int32_t granteeID,
      int32_t & grantDepth); 

   bool hasWGO(
      int32_t authID,
      const std::string & componentUIDString,
      const std::string & operationCode);

   PrivStatus revokeAllForGrantor(
      const int32_t grantorID,
      const std::string componentName,
      const std::string componentUIDString,
      const std::string operationName,
      const std::string operationCode,
      const bool isGOFSpecified,
      const int32_t newGrantDepth,
      PrivDropBehavior dropBehavior); 
       
// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
      
std::string      fullTableName_;
PrivMgrMDTable & myTable_;
};
#endif // PRIVMGR_COMPONENTPRIVILEGES_H


