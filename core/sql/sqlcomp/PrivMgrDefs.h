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

#ifndef PRIVMGR_DEFS_H
#define PRIVMGR_DEFS_H

#include <bitset>
#include "NAUserId.h"
#include "ComSmallDefs.h"
// *****************************************************************************
// *
// * File:         PrivMgrDef.h
// * Description:  This file contains common definitions used by the  
// *               privilege manager component
// *
// *****************************************************************************
#define MAX_PRIV_OBJECT_NAME_LEN 600

class ObjectPrivsRow
{
public:
   char objectName[MAX_PRIV_OBJECT_NAME_LEN + 1];
   ComObjectType objectType;
   int32_t granteeID;
   char granteeName[MAX_USERNAME_LEN * 2 + 1];
   ComGranteeType granteeType;
   int32_t grantorID;
   char grantorName[MAX_USERNAME_LEN * 2 + 1];
   ComGrantorType grantorType;
   int64_t privilegesBitmap;
   int64_t grantableBitmap;   
};      
      
// Returns the result of the operation 
enum PrivStatus { STATUS_UNKNOWN   = 20,
                  STATUS_GOOD      = 21,
                  STATUS_WARNING   = 22,
                  STATUS_NOTFOUND  = 23,
                  STATUS_ERROR     = 24
                };

// Defines the list of supported privileges
// and their order in the privilege and grantable bitmaps
// stored in the OBJECT_PRIVILEGES table
enum PrivType { SELECT_PRIV = 0, //DML PRIVS START HERE 
                INSERT_PRIV,
                DELETE_PRIV,
                UPDATE_PRIV,
                USAGE_PRIV,
                REFERENCES_PRIV,
                EXECUTE_PRIV,
                CREATE_PRIV,     //DDL PRIVS START HERE
                ALTER_PRIV,
                DROP_PRIV,
                ALL_DML,
                ALL_DDL,
                ALL_PRIVS };
                
enum class PrivDropBehavior {
   CASCADE = 2,
   RESTRICT = 3
};                

enum class PrivCommand {
   GRANT_OBJECT = 2,
   REVOKE_OBJECT_RESTRICT = 3,
   REVOKE_OBJECT_CASCADE = 4
};
  
// NOTE: These values need to match the corresponding values in 
// common/ComSmallDefs.h, ComIdClass.
enum class PrivAuthClass {
   UNKNOWN = 0,
   ROLE = 1,
   USER = 2 
};                

const static int32_t FIRST_DML_PRIV = SELECT_PRIV;
const static int32_t LAST_PRIMARY_DML_PRIV = UPDATE_PRIV;
const static int32_t LAST_DML_PRIV = EXECUTE_PRIV;
const static int32_t FIRST_DDL_PRIV = CREATE_PRIV;
const static int32_t LAST_DDL_PRIV = DROP_PRIV;

const static int32_t NBR_DML_PRIVS = LAST_DML_PRIV-FIRST_DML_PRIV + 1;
const static int32_t NBR_DDL_PRIVS = LAST_DDL_PRIV-FIRST_DDL_PRIV + 1;
const static int32_t NBR_OF_PRIVS = NBR_DML_PRIVS+NBR_DDL_PRIVS;

// Defines the privileges and grantable bitmaps as PrivMgrBitmap
//using PrivMgrBitmap = std::bitset<NBR_OF_PRIVS>;
#define PrivMgrBitmap std::bitset<NBR_OF_PRIVS>

// object types for grantable objects
#define BASE_TABLE_OBJECT_LIT               "BT"
#define LIBRARY_OBJECT_LIT                  "LB"
#define VIEW_OBJECT_LIT                     "VI"
#define USER_DEFINED_ROUTINE_OBJECT_LIT     "UR"
#define SEQUENCE_GENERATOR_OBJECT_LIT       "SG"

#define UNKNOWN_GRANTOR_TYPE_LIT               "  "
#define SYSTEM_GRANTOR_LIT                     "S "
#define USER_GRANTOR_LIT                       "U "

#define UNKNOWN_GRANTEE_TYPE_LIT               "  "
#define PUBLIC_GRANTEE_LIT                     "P "
#define USER_GRANTEE_LIT                       "U "

#define SYSTEM_AUTH_ID          -2
#define PUBLIC_AUTH_ID          -1

#define PUBLIC_AUTH_NAME "PUBLIC"
#define SYSTEM_AUTH_NAME "_SYSTEM"

#define DB_ROOTROLE_NAME "DB__ROOTROLE"
#define DB_ROOTROLE_ID 1000000

#define MAX_SQL_IDENTIFIER_NAME_LEN 256


#endif
