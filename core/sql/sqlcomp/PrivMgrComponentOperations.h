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

#ifndef PRIVMGR_COMPONENTOPERATIONS_H
#define PRIVMGR_COMPONENTOPERATIONS_H

#include <string>
#include "PrivMgrMD.h"
#include "PrivMgrDefs.h"
class ComDiagsArea;
class PrivMgrMDTable;
class PrivMgrComponentPrivileges;

// *****************************************************************************
// * Class:         PrivMgrComponentOperations
// * Description:  This class represents the component operations which contains:
// *                - metadata location of the table
// *****************************************************************************
class PrivMgrComponentOperations : public PrivMgr
{
public:

   enum OperationType { OP_TYPE_UNKNOWN,
                        OP_TYPE_SYSTEM,
                        OP_TYPE_USER,
                        OP_TYPE_UNUSED };

   // -------------------------------------------------------------------
   // Constructors and destructors:
   // -------------------------------------------------------------------
   PrivMgrComponentOperations( 
      const std::string & metadataLocation,
      ComDiagsArea * pDiags = NULL);
   PrivMgrComponentOperations(const PrivMgrComponentOperations &other);
   virtual ~PrivMgrComponentOperations();

   // -------------------------------------------------------------------
   // Public functions:
   // -------------------------------------------------------------------
    
   void clear();

   static OperationType compTypeToEnum (const char operationType )
   {
      switch (operationType)
      {
         case 'Y': return OP_TYPE_SYSTEM; 
         case 'N': return OP_TYPE_USER; 
         case 'U': return OP_TYPE_UNUSED; 
         default: return OP_TYPE_UNKNOWN; 
      }
   }
   static char compTypeToLit (OperationType type)
   {
      switch(type)
      {
         case OP_TYPE_SYSTEM: return 'Y';
         case OP_TYPE_USER: return 'N'; 
         case OP_TYPE_UNUSED: return 'U';
         default: return ' ';
      }
   }
   
   PrivStatus createOperation(
      const std::string & componentName,
      const std::string & operationName,
      const std::string & operationCode,
      bool isSystem,
      const std::string & operationDescription,
      const bool existsErrorOK = false);
    
   PrivStatus createOperationInternal(
      const int64_t componentUID,
      const std::string & operationName,
      const std::string & operationCode,
      const bool operationTypeUnused,
      const std::string & operationDescription,
      const int32_t granteeID,
      const std::string & granteeName,
      const int32_t grantDepth,
      const bool checkExistence);
      
  PrivStatus describeComponentOperations(
      const std::string & componentUIDString,
      const std::string & componentName,
      std::vector<std::string> & outlines,
      PrivMgrComponentPrivileges * componentPrivileges = NULL);

   PrivStatus dropAll();
   
   PrivStatus dropAll(const std::string & componentUID);
    
   PrivStatus dropOperation(
      const std::string & componentName,
      const std::string & operationName,
      PrivDropBehavior dropBehavior);
   
//TODO: Not currently implemented.
   PrivStatus fetchByCode(
      const std::string & componentUID,
      const std::string & operationCode,
      std::string & operationName,
      std::string & operationDescription);

   PrivStatus fetchByName(
      const std::string & componentUID,
      const std::string & operationName,
      std::string & operationCode,
      bool isSystem,
      std::string & operationDescription);

   PrivStatus getCount(
     const int64_t &componentUID,
     int32_t &numOps,
     int32_t &numUnusedOps);

   bool isComponentUsed(const std::string & componentUIDString);      
      
   PrivStatus updateOperationCodes(
      const int64_t & componentUID);

   bool nameExists(
      const int64_t componentUID,
      const std::string & operationName);
      
private: 
   PrivMgrComponentOperations();
 
   bool codeExists(
      const int64_t componentUID,
      const std::string & operationCode);
      

// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
      
std::string        fullTableName_;
PrivMgrMDTable   & myTable_;
};
#endif // PRIVMGR_COMPONENTOPERATIONS_H









