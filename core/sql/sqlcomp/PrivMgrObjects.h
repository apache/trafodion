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

#ifndef PRIVMGR_OBJECTS_H
#define PRIVMGR_OBJECTS_H
#include "PrivMgrMD.h"
#include "PrivMgrDefs.h"
#include <string>
#include <vector>

class ComDiagsArea;
class PrivMgrMDTable;

// *****************************************************************************
// *
// * File:         PrivMgrObjects.h
// * Description:  This file contains classes that access the 
// *               contents of the OBJECTS table
// *               
// * Language:     C++
// *
// *****************************************************************************

typedef struct 
{
   int64_t UID;
   ComObjectType objectType;
} UIDAndType;

// *****************************************************************************
// * Class:         PrivMgrObjects
// * Description:  This class provides access to the OBJECTS table.
// *****************************************************************************
class PrivMgrObjects : public PrivMgr
{
public:

// -------------------------------------------------------------------
// Constructors and destructors:
// -------------------------------------------------------------------
   PrivMgrObjects(
      const std::string & metadataLocation,
      ComDiagsArea * pDiags = NULL);
   PrivMgrObjects(const PrivMgrObjects &other);
   virtual ~PrivMgrObjects();

// -------------------------------------------------------------------
// Public functions:
// -------------------------------------------------------------------
   PrivStatus addEntry(
      const int64_t objectUID,
      const std::string & catalogName, 
      const std::string & schemaName, 
      const std::string & objectName,
      const ComObjectType objectType, 
      const int64_t createTime,
      const int64_t redefTime,
      const bool isValid,
      const int32_t objectOwner,
      const int32_t schemaOwner);
      
   void clear();

   PrivStatus deleteEntryByName(
      const std::string & catalogName, 
      const std::string & schemaName, 
      const std::string & objectName);
      
   PrivStatus fetchQualifiedName(
      const int64_t objectUID,
      std::string & qualifiedObjectName);
       
   PrivStatus fetchQualifiedName(
      const std::string & whereClause,
      std::string & qualifiedObjectName);
       
   PrivStatus fetchUIDs(
      const std::string whereClause,
      vector<int64_t> &UIDs);
        
   PrivStatus fetchUIDandTypes(
      const std::string whereClause,
      vector<UIDAndType> &UIDandTypes);
        
   PrivStatus fetchNames(
      const std::string whereClause,
      std::vector<std::string> & catalogNames,
      std::vector<std::string> & schemaNames,
      std::vector<std::string> & objectNames);

       
private: 
// -------------------------------------------------------------------
// Private functions:
// -------------------------------------------------------------------
   PrivMgrObjects();
      
// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
      
std::string        fullTableName_;
PrivMgrMDTable   & myTable_;

};
#endif // PRIVMGR_OBJECTS_H









