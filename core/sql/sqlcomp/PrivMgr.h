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
// @@@ END COPYRIGHT @@@
//*****************************************************************************

#ifndef PRIVMGR_H
#define PRIVMGR_H

#include <set>
#include <string>
#include <vector>
#include "PrivMgrDefs.h"
#include "PrivMgrComponentDefs.h"
#include "ComSmallDefs.h"
#include "CmpSeabaseDDLauth.h"

// following includes needed for diags interface
class ComDiagsArea;

#ifndef Lng32
typedef int             Lng32;
#endif

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class PrivMgr;

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

    enum PrivMgrTableEnum { OBJECT_PRIVILEGES_TABLE = 30,
                            COLUMN_PRIVILEGES_TABLE = 31,
                            SCHEMA_PRIVIELGES_TABLE = 32,
                            COMPONENTS_TABLE        = 33,
                            COMPONENT_OPERATIONS_TABLE  = 34,
                            COMPONENT_PRIVILEGES_TABLE  = 35,
                            ROLE_USAGE_TABLE            = 36,
                            UNKNOWN_TABLE               = 37
                          };

    enum PrivCommand { GRANT_OBJECT           = 30,
                       GRANT_COLUMN           = 31,
                       REVOKE_OBJECT_RESTRICT = 32,
                       REVOKE_OBJECT_CASCADE  = 33,
                       REVOKE_COLUMN_RESTRICT = 34,
                       REVOKE_COLUMN_CASCADE  = 35,
                       UNKNOWN_PRIV_COMMAND   = 36
                     };

    bool isRevokeCommand (const PrivCommand command)
    {
      return (command == REVOKE_OBJECT_RESTRICT ||
              command == REVOKE_OBJECT_CASCADE ||
              command == REVOKE_COLUMN_RESTRICT ||
              command == REVOKE_COLUMN_CASCADE);
    }

    bool isGrantCommand (const PrivCommand command)
    {
      return (command == GRANT_OBJECT || command == GRANT_COLUMN);
    }

    // -------------------------------------------------------------------
    // Static functions:
    // -------------------------------------------------------------------
    
    // 4.4.6 implementation of to_string only supports double, long long int,
    // and unsigned long long int.  Update when int, etc. are supported.
    static inline std::string authIDToString(const int32_t value)  
      {return std::to_string(static_cast<long long int>(value));}
    static inline std::string UIDToString(const int64_t value)  
      {return std::to_string(static_cast<long long int>(value));}
    static bool getAuthNameFromAuthID(
      const int32_t authID,
      std::string &authName);
    
    static const char * getSQLOperationCode(SQLOperation operation);
    static const char * getSQLOperationDescription(SQLOperation operation);
    static const char * getSQLOperationName(SQLOperation operation);
    static int32_t getSQLUnusedOpsCount();
    static bool isSQLAlterOperation(SQLOperation operation);
    static bool isSQLCreateOperation(SQLOperation operation);
    static bool isSQLDropOperation(SQLOperation operation);
    static bool isSQLManageOperation(SQLOperation operation);
    static bool isSQLManageOperation(const char * operationCode);
    static const char * ObjectEnumToLit(ComObjectType objectType);
    static ComObjectType ObjectLitToEnum(const char *objectLiteral);    
    static bool isRoleID(int_32 authID){ return CmpSeabaseDDLauth::isRoleID(authID); }
    static bool isUserID(int_32 authID){ return CmpSeabaseDDLauth::isUserID(authID); }
    
    static bool isSecurableObject(const ComObjectType objectType)
    {
      return (objectType == COM_BASE_TABLE_OBJECT ||
              objectType == COM_LIBRARY_OBJECT ||
              objectType == COM_USER_DEFINED_ROUTINE_OBJECT ||
              objectType == COM_VIEW_OBJECT ||
              objectType == COM_SEQUENCE_GENERATOR_OBJECT ||
              objectType == COM_STORED_PROCEDURE_OBJECT);
    }

    // Set default privileges for a bitmap based on a table or view
    static void setTablePrivs(PrivMgrBitmap &bitmap)
    {
       bitmap.reset();
       bitmap.set(SELECT_PRIV);
       bitmap.set(DELETE_PRIV);
       bitmap.set(INSERT_PRIV);
       bitmap.set(UPDATE_PRIV);
       bitmap.set(REFERENCES_PRIV);
    }

    static void translateObjectName(
      const std::string inputName,
      std::string &outputName);

    static void log(
      const std::string filename,
      const std::string message,
      const int_32 index);

    // -------------------------------------------------------------------
    // Constructors and destructors:
    // -------------------------------------------------------------------
    PrivMgr();
    PrivMgr( 
       const std::string &metadataLocation,
       ComDiagsArea * pDiags = NULL,
       PrivMDStatus authorizationEnabled = PRIV_INITIALIZED);
    PrivMgr( 
       const std::string &trafMetadataLocation,
       const std::string &metadataLocation,
       ComDiagsArea * pDiags = NULL,
       PrivMDStatus authorizationEnabled = PRIV_INITIALIZED);
    PrivMgr(const PrivMgr &rhs);
    virtual ~PrivMgr(void);
    

    // -------------------------------------------------------------------
    // Accessors and destructors:
    // -------------------------------------------------------------------
    PrivStatus getGranteeIDsForRoleIDs(
      const std::vector<int32_t> & roleIDs,
      std::vector<int32_t> & userIDs,
      bool includeSysGrantor = true);

    inline std::string getMetadataLocation (void) {return metadataLocation_;}
    inline const std::string & getMetadataLocation (void) const {return metadataLocation_;}
    inline std::string getTrafMetadataLocation (void) {return trafMetadataLocation_;}
    inline const std::string & getTrafMetadataLocation (void) const {return trafMetadataLocation_;}
    bool isAuthorizationEnabled(void); 
    void setAuthorizationEnabled(PrivMDStatus authStatus) {authorizationEnabled_ = authStatus;}
    bool isAuthIDGrantedPrivs(
       const int32_t authID,
       std::vector<PrivClass> privClasses,
       std::vector<int64_t> &objectUIDs);
    void resetFlags();
    void setFlags();


  protected:
  // Returns status of privilege manager metadata

    PrivMDStatus authorizationEnabled(std::set<std::string> &existingObjectList);
    
    // -------------------------------------------------------------------
    // Data members:
    // -------------------------------------------------------------------
    std::string      trafMetadataLocation_;
    std::string      metadataLocation_;
    ComDiagsArea *   pDiags_;
    unsigned int     parserFlags_;
    PrivMDStatus     authorizationEnabled_;
    
}; // class PrivMgr      
  

#endif // PRIVMGR_H









