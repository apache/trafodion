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

#ifndef PRIVMGR_DEFS_H
#define PRIVMGR_DEFS_H

#include <bitset>

// *****************************************************************************
// *
// * File:         PrivMgrDef.h
// * Description:  This file contains common definitions used by the  
// *               privilege manager component
// *
// *****************************************************************************



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

// NOTE: These values need to match the corresponding values in 
// common/ComSmallDefs.h, ComIdClass.
enum class PrivAuthClass {
   UNKNOWN = 0,
   ROLE = 1,
   USER = 2 
};                

const static int32_t FIRST_DML_PRIV = SELECT_PRIV;
const static int32_t LAST_DML_PRIV = EXECUTE_PRIV;
const static int32_t FIRST_DDL_PRIV = CREATE_PRIV;
const static int32_t LAST_DDL_PRIV = DROP_PRIV;

const static int32_t NBR_DML_PRIVS = LAST_DML_PRIV-FIRST_DML_PRIV + 1;
const static int32_t NBR_DDL_PRIVS = LAST_DDL_PRIV-FIRST_DDL_PRIV + 1;
const static int32_t NBR_OF_PRIVS = NBR_DML_PRIVS+NBR_DDL_PRIVS;

// Defines the privileges and grantable bitmaps as PrivMgrBitmap
//using PrivMgrBitmap = std::bitset<NBR_OF_PRIVS>;
#define PrivMgrBitmap std::bitset<NBR_OF_PRIVS>

#endif
