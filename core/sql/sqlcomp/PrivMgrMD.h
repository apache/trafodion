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
#include "PrivMgrDesc.h"
#include "ComSmallDefs.h"
#include "CmpSeabaseDDLauth.h"

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
   ALTER_SCHEMA,
   ALTER_SEQUENCE,
   ALTER_SYNONYM,
   ALTER_TABLE,
   ALTER_TRIGGER,
   ALTER_VIEW,
   CREATE,
   CREATE_CATALOG,
   CREATE_INDEX,
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
   DROP_INDEX,
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
   MANAGE_COMPONENTS,
   MANAGE_LIBRARY,
   MANAGE_LOAD,
   MANAGE_ROLES,
   MANAGE_STATISTICS,
   MANAGE_USERS,
   QUERY_ACTIVATE,
   QUERY_CANCEL,
   QUERY_SUSPEND,
   REMAP_USER,
   SHOW,
   USE_ALTERNATE_SCHEMA,
   FIRST_OPERATION = ALTER,
   LAST_OPERATION = USE_ALTERNATE_SCHEMA,
   NUMBER_OF_OPERATIONS = LAST_OPERATION - FIRST_OPERATION + 1,
   UNKNOWN
};

enum {SQL_OPERATIONS_COMPONENT_UID = 1};
#define SQL_OPERATION_NAME "SQL_OPERATIONS"
#define PRIVMGR_INTERNAL_ERROR(text)                                      \
   *pDiags_ << DgSqlCode(-CAT_INTERNAL_EXCEPTION_ERROR)                   \
            << DgString0(__FILE__)                                        \
            << DgInt0(__LINE__)                                           \
            << DgString1(text)                                            

// *****************************************************************************
// * Class:         PrivMgr
// * Description:  This is the base class for the Trafodion Privilege Manager.
// *                
// *****************************************************************************
class PrivMgr
{
  public:
    enum PrivMDStatus { PRIV_INITIALIZED           = 30,
                        PRIV_UNINITIALIZED         = 31,
                        PRIV_PARTIALLY_INITIALIZED = 32,
                        PRIV_INITIALIZE_UNKNOWN    = 33
                      }; 
    // -------------------------------------------------------------------
    // Static functions:
    // -------------------------------------------------------------------
    
    // 4.4.6 implementation of to_string only supports double, long long int,
    // and unsigned long long int.  Update when int, etc. are supported.
    static inline std::string authIDToString(const int32_t value)  {return std::to_string(static_cast<long long int>(value));}
    static inline std::string UIDToString(const int64_t value)  {return std::to_string(static_cast<long long int>(value));}
    
    static const char * getSQLOperationName(SQLOperation operation);
    static const char * getSQLOperationCode(SQLOperation operation);
    static const char * getSQLOperationDescription(SQLOperation operation);
    static bool isSQLAlterOperation(SQLOperation operation);
    static bool isSQLCreateOperation(SQLOperation operation);
    static bool isSQLDropOperation(SQLOperation operation);
    static const char * ObjectEnumToLit(ComObjectType objectType);
    static ComObjectType ObjectLitToEnum(const char *objectLiteral);    
    static bool isRoleID(int_32 authID){ return CmpSeabaseDDLauth::isRoleID(authID); }
    static bool isUserID(int_32 authID){ return CmpSeabaseDDLauth::isUserID(authID); }
    
    // -------------------------------------------------------------------
    // Constructors and destructors:
    // -------------------------------------------------------------------
    PrivMgr();
    PrivMgr( 
       const std::string &metadataLocation,
       ComDiagsArea * pDiags = NULL,
       PrivMDStatus authorizationEnabled = PRIV_INITIALIZE_UNKNOWN);
    PrivMgr( 
       const std::string &trafMetadataLocation,
       const std::string &metadataLocation,
       ComDiagsArea * pDiags = NULL,
       PrivMDStatus authorizationEnabled = PRIV_INITIALIZE_UNKNOWN);
    PrivMgr(const PrivMgrMDAdmin &rhs);
    virtual ~PrivMgr(void);
    

    // -------------------------------------------------------------------
    // Accessors and destructors:
    // -------------------------------------------------------------------
    inline std::string getMetadataLocation (void) {return metadataLocation_;}
    inline const std::string & getMetadataLocation (void) const {return metadataLocation_;}
    inline std::string getTrafMetadataLocation (void) {return trafMetadataLocation_;}
    inline const std::string & getTrafMetadataLocation (void) const {return trafMetadataLocation_;}
    bool isAuthorizationEnabled(void); 
    void setAuthorizationEnabled(PrivMDStatus authStatus) {authorizationEnabled_ = authStatus;}
    bool isAuthIDGrantedPrivs(
       int32_t authID,
       std::vector<PrivClass> privClasses);
    void resetFlags();
    void setFlags();

  protected:
  // Returns status of privilege manager metadata

    PrivMDStatus authorizationEnabled();
    
    // -------------------------------------------------------------------
    // Data members:
    // -------------------------------------------------------------------
    std::string  trafMetadataLocation_;
    std::string  metadataLocation_;
    ComDiagsArea * pDiags_;
    unsigned int parserFlags_;
    PrivMDStatus authorizationEnabled_;
    
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









