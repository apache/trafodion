//*****************************************************************************
// @@@ START COPYRIGHT @@@
//
//// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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

#ifndef PRIVMGR_MD_H
#define PRIVMGR_MD_H

#include <string>
#include <vector>
#include "PrivMgrDefs.h"
#include "PrivMgr.h"
#include "PrivMgrDesc.h"
#include "ComSmallDefs.h"
#include "CmpSeabaseDDLauth.h"

// following includes needed for cli interface
class Queue;
class ExeCliInterface;
class OutputInfo;
#ifndef Lng32
typedef int             Lng32;
#endif

// *****************************************************************************
// *
// * File:         PrivMgrMD.h
// * Description:  This file contains classes that access and maintain the 
// *               contents of the Privilege Manager metadata and that 
// *               interact with Trafodion system metadata.
// *               
// * Language:     C++
// *
// *****************************************************************************

// -----------------------------------------------------------------------
// class contents of this file
// -----------------------------------------------------------------------
class ObjectPrivsRow;
class PrivMgrMDAdmin;

// -----------------------------------------------------------------------
// Struct definitions
// -----------------------------------------------------------------------
typedef struct {
  int64_t objectUID;
  int32_t objectOwner;
  std::string objectName;
  ComObjectType objectType;
  PrivMgrDesc originalPrivs;
  PrivMgrDesc updatedPrivs;
} ObjectUsage;

typedef struct {
  int64_t viewUID;
  int32_t viewOwner;
  std::string viewName;
  bool isUpdatable;
  bool isInsertable;
  PrivMgrDesc originalPrivs;
  PrivMgrDesc updatedPrivs;
} ViewUsage;

typedef struct {
  int64_t objectUID;
  int32_t objectOwner;
  ComObjectType objectType;
  std::string objectName;
  PrivMgrDesc updatedPrivs;
} ObjectReference;


// ****************************************************************************
// class: PrivMgrMDAdmin
//
// This class initializes, drops, and upgrades metadata managed by the
// Privilege Manager
// ****************************************************************************
class PrivMgrMDAdmin : public PrivMgr
{
  public:

    // -------------------------------------------------------------------
    // Constructors and destructors:
    // -------------------------------------------------------------------
    PrivMgrMDAdmin ();
    PrivMgrMDAdmin(
       const std::string & trafMetadataLocation,
       const std::string & metadataLocation,
       ComDiagsArea * pDiags = NULL);
    PrivMgrMDAdmin(
       const std::string & metadataLocation,
       ComDiagsArea * pDiags = NULL);
    PrivMgrMDAdmin ( const PrivMgrMDAdmin &rhs );
    virtual ~PrivMgrMDAdmin ( void );

    // -------------------------------------------------------------------
    // Accessors and destructors:
    // -------------------------------------------------------------------
    inline std::string getMetadataLocation (void) {return metadataLocation_;}
    PrivStatus initializeComponentPrivileges();
    PrivStatus initializeMetadata(const std::string &objectsLocation,
                                  const std::string &authsLocation);
    PrivStatus dropMetadata(const std::vector<std::string> &objectsToDrop);
    PrivStatus upgradeMetadata();


    inline void setMetadataLocation (const std::string metadataLocation)
      {metadataLocation_ = metadataLocation;};

    bool getConstraintName(
      const int64_t referencedTableUID,
      const int64_t referencingTableUID, 
      std::string &referencingTable);

    PrivStatus getObjectsThatViewReferences (
      const ViewUsage &viewUsage,
      std::vector<ObjectReference *> &objectReference );

    PrivStatus getReferencingTablesForConstraints(
      const ObjectUsage &objectUsage,
      std::vector<ObjectReference *> &objectReferences );

    PrivStatus getUdrsThatReferenceLibrary(
      const ObjectUsage &objectUsage,
      std::vector<ObjectReference *> &objectReferences );

    PrivStatus getViewsThatReferenceObject(
      const ObjectUsage &objectUsage, 
      std::vector<ViewUsage> &viewUsages);

    bool isAuthorized (void);
    std::string deriveTableName(const char *name)
    {
      std::string derivedName (metadataLocation_);
      derivedName += ".";
      derivedName += name;
      return derivedName;
    }

  private:

    bool isRoot(std::string userName)
    { return ((userName == "DB__ROOT") ? true : false); }
    
    PrivStatus updatePrivMgrMetadata(
       const std::string &objectsLocation,
       const std::string &authsLocation,
       const bool shouldPopulateObjectPrivs,
       const bool shouldPopulateRoleGrants);

}; // class PrivMgrMDAdmin


#endif // PRIVMGR_MD_H









