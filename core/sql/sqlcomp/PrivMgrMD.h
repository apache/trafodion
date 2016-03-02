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

#ifndef PRIVMGR_MD_H
#define PRIVMGR_MD_H

#include <string>
#include <vector>
#include "PrivMgrDefs.h"
#include "PrivMgr.h"
#include "PrivMgrDesc.h"
#include "PrivMgrMDTable.h"
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
class ObjectReference;
class ObjectUsage;

// -----------------------------------------------------------------------
// Struct definitions
// -----------------------------------------------------------------------
struct ColumnReference {
  int32_t columnOrdinal;
  PrivMgrCoreDesc originalPrivs;
  PrivMgrCoreDesc updatedPrivs;
  ColumnReference() 
  : columnOrdinal(-1),
    originalPrivs(),
    updatedPrivs(){};

  void describe (std::string &details) const
  {
    details = "column usage - column number is ";
    details += to_string((long long int) columnOrdinal);
  }

};

typedef struct {
  int64_t viewUID;
  int32_t viewOwner;
  std::string viewName;
  bool isUpdatable;
  bool isInsertable;
  PrivMgrDesc originalPrivs;
  PrivMgrDesc updatedPrivs;

  void describe (std::string &details) const
  {
    details = "view usage - type is VI";
    details += ", UID is ";
    details += to_string((long long int) viewUID);
    details += ", name is ";
    details += viewName;
    details += ", viewOwner is ";
    details += to_string((long long int) viewOwner);
    details += (isUpdatable) ? ", isUpdatable is Y " : "isUpdateable is N"; 
    details += (isInsertable) ? ", isInsertable is Y " : "isInsertable is N"; 
  }

} ViewUsage;

// -----------------------------------------------------------------------
// Class definitions
// -----------------------------------------------------------------------
class ObjectReference
{
  public:

  ObjectReference()
  : objectUID(0),
    objectOwner(NA_UserIdDefault),
    objectType(COM_UNKNOWN_OBJECT),
    columnReferences(NULL),
    updatedPrivs()
  {}
    
  virtual ~ObjectReference ( void )
  {
    if (columnReferences)
    {
      while(!columnReferences->empty())
        delete columnReferences->back(), columnReferences->pop_back();
      delete columnReferences;
    }
  }

  int64_t objectUID;
  int32_t objectOwner;
  ComObjectType objectType;
  std::string objectName;
  //TBD - make columnReferences a map instead of a vector
  std::vector<ColumnReference *> *columnReferences;
  PrivMgrDesc updatedPrivs;

  void describe (std::string &details) const
  {
    details = "object reference - type is ";
    char objectTypeLit[3] = {0};
    strncpy(objectTypeLit,PrivMgr::ObjectEnumToLit(objectType),2);
    details += objectTypeLit;
    details += ", UID is ";
    details += to_string((long long int) objectUID);
    details += ", name is ";
    details += objectName;
    details += ", owner is ";
    details += to_string((long long int) objectOwner);
  }

};

class ObjectUsage
{
  public:

  ObjectUsage()
  : objectUID (0),
    granteeID (NA_UserIdDefault),
    grantorIsSystem(false),
    objectType (COM_UNKNOWN_OBJECT),
    columnReferences(NULL),
    originalPrivs(),
    updatedPrivs()
  {}

  virtual ~ObjectUsage ( void )
  {
    if (columnReferences)
    {
      while(!columnReferences->empty())
        delete columnReferences->back(), columnReferences->pop_back();
      delete columnReferences;
    }
  }

  int64_t objectUID;
  int32_t granteeID;
  bool grantorIsSystem;
  std::string objectName;
  ComObjectType objectType;
  std::vector<ColumnReference *> *columnReferences;
  PrivMgrDesc originalPrivs;
  PrivMgrDesc updatedPrivs;

  ColumnReference * findColumn (int32_t columnOrdinal)
  {
    if (columnReferences == NULL)
      return NULL;
    for (int i = 0; i < columnReferences->size(); i++)
    {
      ColumnReference *pRef = (*columnReferences)[i];
      if (pRef->columnOrdinal == columnOrdinal)
        return pRef;
    }
    return NULL;
  }

  void describe (std::string &details) const
  {
    details = "object usage - type is ";
    char objectTypeLit[3] = {0};
    strncpy(objectTypeLit,PrivMgr::ObjectEnumToLit(objectType),2);
    details += objectTypeLit;
    details += ", UID is ";
    details += to_string((long long int) objectUID);
    details += ", name is ";
    details += objectName;
    details += ", grantee is ";
    details += to_string((long long int) granteeID);
    details += ", is owner ";
    details += (grantorIsSystem) ? "true " : "false ";
  }
};


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
    PrivStatus initializeMetadata(
      const std::string &objectsLocation,
      const std::string &authsLocation,
      const std::string &colsLocation,
      std::vector<std::string> &tablesCreated,
      std::vector<std::string> &tablesUpgraded);
    PrivStatus dropMetadata(
      const std::vector<std::string> &objectsToDrop,
      bool doCleanup);

    inline void setMetadataLocation (const std::string metadataLocation)
      {metadataLocation_ = metadataLocation;};

    bool getConstraintName(
      const int64_t referencedTableUID,
      const int64_t referencingTableUID, 
      const int32_t columnNumber,
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

    void cleanupMetadata(ExeCliInterface &cliInterface);

    bool isRoot(std::string userName)
    { return ((userName == "DB__ROOT") ? true : false); }
    
    PrivStatus updatePrivMgrMetadata(
       const std::string &objectsLocation,
       const std::string &authsLocation,
       const bool shouldPopulateObjectPrivs,
       const bool shouldPopulateRoleGrants);

    PrivStatus upgradeMetadata (
      const PrivMgrTableStruct &tableDefinition,
      ExeCliInterface &cliInterface,
      const std::string &objectsLocation,
      const std::string &colsLocation);


}; // class PrivMgrMDAdmin


#endif // PRIVMGR_MD_H









