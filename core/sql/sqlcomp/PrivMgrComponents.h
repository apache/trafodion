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

#ifndef PRIVMGR_COMPONENTS_H
#define PRIVMGR_COMPONENTS_H
#include "PrivMgrMD.h"
#include "PrivMgrDefs.h"
#include <string>

class ComDiagsArea;
class PrivMgrMDTable;

// *****************************************************************************
// *
// * File:         PrivMgrComponents.h
// * Description:  This file contains classes that access and maintain the 
// *               contents of the Privilege Manager Components Table
// *               
// * Language:     C++
// *
// *****************************************************************************

// *****************************************************************************
// * Class:         PrivMgrComponents
// * Description:  This class represents the component table which contains:
// *                - fully qualified table name
// *****************************************************************************
class PrivMgrComponents : public PrivMgr
{
public:

// -------------------------------------------------------------------
// Constructors and destructors:
// -------------------------------------------------------------------
   PrivMgrComponents(
      const std::string & metadataLocation,
      ComDiagsArea * pDiags = NULL);
   PrivMgrComponents(const PrivMgrComponents &other);
   virtual ~PrivMgrComponents();

// -------------------------------------------------------------------
// Public functions:
// -------------------------------------------------------------------
   void clear();

   PrivStatus describeComponents(
        const  std::string & componentName, 
        std::string & componentUIDString, 
        int64_t & componentUID, 
        std::vector<std::string> & outlines);
   
   PrivStatus dropAll();
   
   bool exists(const std::string & componentName);
   
   PrivStatus fetchByName(
      const std::string & componentName,
      std::string & componentUIDString,
      int64_t & componentUID,
      bool & isSystem,
      std::string & componentDescription);

//TODO: Not currently implemented.
   PrivStatus fetchByUID(
      int64_t componentUID,
      std::string & componentName,
      bool & isSystem,
      std::string & componentDescription);
      
   int64_t getCount();
   
   PrivStatus registerComponent(
      const std::string &componentName,
      const bool isSystem,
      const std::string & componentDescription,
      const bool existsErrorOK = false);
       
   PrivStatus registerComponentInternal(
      const std::string & componentName,
      const int64_t componentUID,
      const bool isSystem,
      const std::string & componentDescription);
      
   PrivStatus unregisterComponent(
      const std::string &componentName,
      PrivDropBehavior dropBehavior);
       
private: 
// -------------------------------------------------------------------
// Private functions:
// -------------------------------------------------------------------
   PrivMgrComponents();
      
   int64_t getUniqueID();

// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
      
std::string        fullTableName_;
PrivMgrMDTable   & myTable_;

};
#endif // PRIVMGR_COMPONENTS_H









