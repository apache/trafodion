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

#ifndef PRIVMGR_COMPONENTS_DEFS_H
#define PRIVMGR_COMPONENTS_DEFS_H

// *****************************************************************************
// *
// * Component definition section
// *
// * Several system components are created and managed by the database.  
// * They are managed by two main structures:
// *     ComponentListStruct - the list of components
// *     ComponentOpStruct   - the list of operations for each component
// *
// * To add a new component (assume xxx is component name):
// *    Assign a UID                (in enum ComponentOp add xxx_COMPONENT_UID)
// *    Generate a component name   (add new define called xxx_NAME)
// *    Define component operations (add enum xxxOperation) 
// *    Define operation attributes (add ComponentOpStruct xxxOpStruct)
// *    Add component to list       (add component to componentList)
// *
// * To add a new operation to an existing component, see comments associated
// * with the component.
// *
// *****************************************************************************

// The ComponentOpStruct describes a component
//   operationID   - a number from xxxOperation representing the operation 
//   operationCode - unique 2 charater value that represents the operation
//   operationName - unique name for the operation
//   isRootRoleOp  - grant DB__ROOTROLE this operation
//   isAdminOp     - grant DB__ADMIN/DB__ADMINROLE this operation
//   isDMLOp       - this is a DML operation
//   isPublicOp    - grant PUBLIC this operation
//   unusedOp      - operation is not supported at this time but maybe later
struct ComponentOpStruct
{
  int32_t      operationID;
  const char * operationCode;
  const char * operationName;
  const bool   isRootRoleOp;
  const bool   isAdminOp;
  const bool   isDMLOp;
  const bool   isPublicOp;
  const bool   unusedOp;
};

// The ComponentListStruct describes the relationship between a component UID,
// its name, the number of operations for the component, and a pointer to the
// list of operations.
//   componentUID  - the UID for the component
//   componentName - the component name
//   numOps        - the number of operations in the component
//   componentOps  - pointer the ComponentOpStruct describing the operations
struct ComponentListStruct
{
   int64_t                   componentUID;
   const char              * componentName;
   int32_t                   numOps;
   const ComponentOpStruct * componentOps;
};

// UID's for system component   
// USER_COMPONENT_START_UID begins user defined components
enum ComponentOp{ INVALID_COMPONENT_UID        = 0,
                  SQL_OPERATIONS_COMPONENT_UID = 1,
                  USER_COMPONENT_START_UID     = 1000};

// List of components
#define SQL_OPERATIONS_NAME "SQL_OPERATIONS"

// Defines component operations for SQL_OPERATIONS:
//  to add a new operation, add an entry to this list (in alphebetic order)
//  and add a corresponding entry to the sqlOpList. 
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
   COMMENT,
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
   DML_DELETE,
   DML_EXECUTE,
   DML_INSERT,
   DML_REFERENCES,
   DML_SELECT,
   DML_SELECT_METADATA,
   DML_UPDATE,
   DML_USAGE,
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
   MANAGE,
   MANAGE_COMPONENTS,
   MANAGE_LIBRARY,
   MANAGE_LOAD,
   MANAGE_PRIVILEGES,
   MANAGE_ROLES,
   MANAGE_STATISTICS,
   MANAGE_USERS,
   QUERY_ACTIVATE,
   QUERY_CANCEL,
   QUERY_SUSPEND,
   REGISTER_HIVE_OBJECT,
   REMAP_USER,
   SHOW,
   UNREGISTER_HIVE_OBJECT,
   USE_ALTERNATE_SCHEMA,
   FIRST_OPERATION = ALTER,
   LAST_OPERATION = USE_ALTERNATE_SCHEMA,
   NUMBER_OF_OPERATIONS = LAST_OPERATION - FIRST_OPERATION + 1,
   UNKNOWN,
   FIRST_DML_PRIV = DML_DELETE,
   LAST_DML_PRIV = DML_USAGE
};

// Assign initial privileges for SQL_OPERATIONS (based on ComponentOpStruct):
//    recommend that DB__ROOTROLE granted all non DML privileges
//    recommend that DB__ADMIN and DB__ADMINROLE granted all non DML privileges
//    recommend that PUBLIC granted only a small subset of privileges
static const ComponentOpStruct sqlOpList[] =
{
 {(int32_t)SQLOperation::ALTER,               "A0","ALTER",true,true,false,false,false},
 {(int32_t)SQLOperation::ALTER_LIBRARY,       "AL","ALTER_LIBRARY",true,false,false,false,false},
 {(int32_t)SQLOperation::ALTER_ROUTINE,       "AR","ALTER_ROUTINE",true,false,false,false,false},
 {(int32_t)SQLOperation::ALTER_ROUTINE_ACTION,"AA","ALTER_ROUTINE_ACTION",true,false,false,false,true},
 {(int32_t)SQLOperation::ALTER_SCHEMA,        "AH","ALTER_SCHEMA",true,false,false,false,false},
 {(int32_t)SQLOperation::ALTER_SEQUENCE,      "AQ","ALTER_SEQUENCE",true,false,false,false,false},
 {(int32_t)SQLOperation::ALTER_SYNONYM,       "AY","ALTER_SYNONYM",true,false,false,false,true},
 {(int32_t)SQLOperation::ALTER_TABLE,         "AT","ALTER_TABLE",true,false,false,false,false},
 {(int32_t)SQLOperation::ALTER_TRIGGER,       "AG","ALTER_TRIGGER",true,false,false,false,true},
 {(int32_t)SQLOperation::ALTER_VIEW,          "AV","ALTER_VIEW",true,false,false,false,false},

 {(int32_t)SQLOperation::COMMENT,             "CO","COMMENT",true,true,false,false,false},

 {(int32_t)SQLOperation::CREATE,              "C0","CREATE",true,true,false,false,false },
 {(int32_t)SQLOperation::CREATE_CATALOG,      "CC","CREATE_CATALOG",true,false,false,false,true},
 {(int32_t)SQLOperation::CREATE_INDEX,        "CI","CREATE_INDEX",true,false,false,false,false},
 {(int32_t)SQLOperation::CREATE_LIBRARY,      "CL","CREATE_LIBRARY",true,false,false,false,false},
 {(int32_t)SQLOperation::CREATE_PROCEDURE,    "CP","CREATE_PROCEDURE",true,false,false,false,false},
 {(int32_t)SQLOperation::CREATE_ROUTINE,      "CR","CREATE_ROUTINE",true,false,false,false,false},
 {(int32_t)SQLOperation::CREATE_ROUTINE_ACTION,"CA","CREATE_ROUTINE_ACTION",true,false,false,false,true},
 {(int32_t)SQLOperation::CREATE_SCHEMA,       "CH","CREATE_SCHEMA",true,false,false,true,false},
 {(int32_t)SQLOperation::CREATE_SEQUENCE,     "CQ","CREATE_SEQUENCE",true,false,false,false,false},
 {(int32_t)SQLOperation::CREATE_SYNONYM,      "CY","CREATE_SYNONYM",true,false,false,false,true},
 {(int32_t)SQLOperation::CREATE_TABLE,        "CT","CREATE_TABLE",true,false,false,false,false},
 {(int32_t)SQLOperation::CREATE_TRIGGER,      "CG","CREATE_TRIGGER",true,false,false,false,true},
 {(int32_t)SQLOperation::CREATE_VIEW,         "CV","CREATE_VIEW",true,false,false,false,false},

 {(int32_t)SQLOperation::DML_DELETE,     "PD","DML_DELETE",false,false,true,false,true},
 {(int32_t)SQLOperation::DML_EXECUTE,    "PE","DML_EXECUTE",false,false,true,false,true},
 {(int32_t)SQLOperation::DML_INSERT,     "PI","DML_INSERT",false,false,true,false,true},
 {(int32_t)SQLOperation::DML_REFERENCES, "PR","DML_REFERENCES",false,false,true,false,true},
 {(int32_t)SQLOperation::DML_SELECT,     "PS","DML_SELECT",false,false,true,false,true},
 {(int32_t)SQLOperation::DML_SELECT_METADATA,"PM","DML_SELECT_METADATA",true,true,true,false,false},
 {(int32_t)SQLOperation::DML_UPDATE,     "PU","DML_UPDATE",false,false,true,false,true},
 {(int32_t)SQLOperation::DML_USAGE,      "PG","DML_USAGE",false,false,true,false,true},

 {(int32_t)SQLOperation::DROP,               "D0","DROP",true,true,false,false,false},
 {(int32_t)SQLOperation::DROP_CATALOG,       "DC","DROP_CATALOG",true,false,false,false,true},
 {(int32_t)SQLOperation::DROP_INDEX,         "DI","DROP_INDEX",true,false,false,false,false},
 {(int32_t)SQLOperation::DROP_LIBRARY,       "DL","DROP_LIBRARY",true,false,false,false,false},
 {(int32_t)SQLOperation::DROP_PROCEDURE,     "DP","DROP_PROCEDURE",true,false,false,false,false},
 {(int32_t)SQLOperation::DROP_ROUTINE,       "DR","DROP_ROUTINE",true,false,false,false,false},
 {(int32_t)SQLOperation::DROP_ROUTINE_ACTION,"DA","DROP_ROUTINE_ACTION",true,false,false,false,true},
 {(int32_t)SQLOperation::DROP_SCHEMA,        "DH","DROP_SCHEMA",true,false,false,false,false},
 {(int32_t)SQLOperation::DROP_SEQUENCE,      "DQ","DROP_SEQUENCE",true,false,false,false,false},
 {(int32_t)SQLOperation::DROP_SYNONYM,       "DY","DROP_SYNONYM",true,false,false,false,true},
 {(int32_t)SQLOperation::DROP_TABLE,         "DT","DROP_TABLE",true,false,false,false,false},
 {(int32_t)SQLOperation::DROP_TRIGGER,       "DG","DROP_TRIGGER",true,false,false,false,true},
 {(int32_t)SQLOperation::DROP_VIEW,          "DV","DROP_VIEW",true,false,false,false,false},

 {(int32_t)SQLOperation::MANAGE,            "M0","MANAGE",true,true,false,false,false},
 {(int32_t)SQLOperation::MANAGE_COMPONENTS, "MC","MANAGE_COMPONENTS",true,false,false,false,false},
 {(int32_t)SQLOperation::MANAGE_LIBRARY,    "ML","MANAGE_LIBRARY",true,false,false,false,false},
 {(int32_t)SQLOperation::MANAGE_LOAD,       "MT","MANAGE_LOAD",true,false,false,false,false},
 {(int32_t)SQLOperation::MANAGE_PRIVILEGES, "MP","MANAGE_PRIVILEGES",true,false,false,false,false},
 {(int32_t)SQLOperation::MANAGE_ROLES,      "MR","MANAGE_ROLES",true,false,false,false,false},
 {(int32_t)SQLOperation::MANAGE_STATISTICS, "MS","MANAGE_STATISTICS",true,false,false,false,false},
 {(int32_t)SQLOperation::MANAGE_USERS,      "MU","MANAGE_USERS",true,false,false,false,false},

 {(int32_t)SQLOperation::QUERY_ACTIVATE, "QA","QUERY_ACTIVATE",true,true,false,false,false},
 {(int32_t)SQLOperation::QUERY_CANCEL,   "QC","QUERY_CANCEL",true,true,false,false,false},
 {(int32_t)SQLOperation::QUERY_SUSPEND,  "QS","QUERY_SUSPEND",true,true,false,false,false},
 {(int32_t)SQLOperation::REGISTER_HIVE_OBJECT,  "RH","REGISTER_HIVE_OBJECT",true,true,false,false,true},

 {(int32_t)SQLOperation::REMAP_USER,           "RU","REMAP_USER",true,true,false,false,true},
 {(int32_t)SQLOperation::SHOW,                 "SW","SHOW",true,true,false,true,false},
 {(int32_t)SQLOperation::UNREGISTER_HIVE_OBJECT,  "UH","UNREGISTER_HIVE_OBJECT",true,true,false,false,true},
 {(int32_t)SQLOperation::USE_ALTERNATE_SCHEMA, "UA","USE_ALTERNATE_SCHEMA",true,true,false,false,true}
};

// List of components
static const ComponentListStruct componentList[]
{ { (int64_t)SQL_OPERATIONS_COMPONENT_UID, SQL_OPERATIONS_NAME, sizeof(sqlOpList)/sizeof(ComponentOpStruct), (ComponentOpStruct *)&sqlOpList } };

#endif
