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

#ifndef PRIVMGR_MD_H
#define PRIVMGR_MD_H

#include <string>
#include <vector>
#include "PrivMgrMDDefs.h"
#include "PrivMgrDefs.h"
#include "PrivMgrDesc.h"

// following includes needed for cli and diags interface
class Queue;
class ExeCliInterface;
class ComDiagsArea;
class OutputInfo;
#ifndef Lng32
typedef int             Lng32;
#endif

typedef struct {
  int64_t objectUID;
  int32_t objectOwner;
  std::string objectName;
  std::string objectType;
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
  PrivMgrDesc updatedPrivs;
} ObjectReference;

// *****************************************************************************
// *
// * File:         PrivMgrMD.h
// * Description:  This file contains classes that access and maintain the 
// *               contents of the Privilege Manager metadata
// *               
// * Language:     C++
// *
// *****************************************************************************

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class PrivMgr;
class PrivMgrMDAdmin;

enum class PrivClass {
   ALL = 2,
   OBJECT = 3,
   COMPONENT = 4,
   SCHEMA = 5 
};
   
enum class SQLOperation {
   ALTER = 2,
   ALTER_LIBRARY,
   ALTER_ROUTINE,
   ALTER_ROUTINE_ACTION,
   ALTER_SEQUENCE,
   ALTER_SYNONYM,
   ALTER_TABLE,
   ALTER_TRIGGER,
   ALTER_VIEW,
   CREATE,
   CREATE_CATALOG,
   CREATE_LIBRARY,
   CREATE_PROCEDURE,
   CREATE_ROUTINE,
   CREATE_ROUTINE_ACTION,
   CREATE_SCHEMA,
   CREATE_SEQUENCE,
   CREATE_SYNONYM,
   CREATE_TABLE,
   CREATE_TRIGGER,
   CREATE_VIEW,
   DROP,
   DROP_CATALOG,
   DROP_LIBRARY,
   DROP_PROCEDURE,
   DROP_ROUTINE,
   DROP_ROUTINE_ACTION,
   DROP_SCHEMA,
   DROP_SEQUENCE,
   DROP_SYNONYM,
   DROP_TABLE,
   DROP_TRIGGER,
   DROP_VIEW,
   MANAGE_ROLES,
   MANAGE_USERS,
   REMAP_USER,
   USE_ALTERNATE_SCHEMA,
   FIRST_OPERATION = ALTER,
   LAST_OPERATION = USE_ALTERNATE_SCHEMA,
   NUMBER_OF_OPERATIONS = LAST_OPERATION - FIRST_OPERATION + 1
};

enum {SQL_OPERATIONS_COMPONENT_UID = 1};
#define SQL_OPERATION_NAME "SQL_OPERATIONS"

// *****************************************************************************
// * Class:         PrivMgr
// * Description:  This is the base class for the Trafodion Privilege Manager.
// *                
// *****************************************************************************
class PrivMgr
{
  public:
    // -------------------------------------------------------------------
    // Static functions:
    // -------------------------------------------------------------------
    
    static const char * getSQLOperationName(SQLOperation operation);
    static const char * getSQLOperationCode(SQLOperation operation);
    static const char * getSQLOperationDescription(SQLOperation operation);
    static bool isSQLAlterOperation(SQLOperation operation);
    static bool isSQLCreateOperation(SQLOperation operation);
    static bool isSQLDropOperation(SQLOperation operation);
    
    // -------------------------------------------------------------------
    // Constructors and destructors:
    // -------------------------------------------------------------------
    PrivMgr();
    PrivMgr( 
       const std::string &metadataLocation,
       ComDiagsArea * pDiags = NULL);
    PrivMgr(const PrivMgrMDAdmin &rhs);
    virtual ~PrivMgr(void);
    

    // -------------------------------------------------------------------
    // Accessors and destructors:
    // -------------------------------------------------------------------
    inline std::string getMetadataLocation (void) {return metadataLocation_;}
    inline const std::string & getMetadataLocation (void) const {return metadataLocation_;}
    bool isAuthorizationEnabled(void); 
    bool isAuthIDGrantedPrivs(
       int32_t authID,
       std::vector<PrivClass> privClasses);

  protected:
    PrivMDStatus authorizationEnabled();
    
    // -------------------------------------------------------------------
    // Data members:
    // -------------------------------------------------------------------
    std::string  metadataLocation_;
    ComDiagsArea * pDiags_;
    
}; // class PrivMgr      
  

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

    PrivStatus getObjectsThatViewReferences (
      const ViewUsage &viewUsage,
      std::vector<ObjectReference *> &objectReference );

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









