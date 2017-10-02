/**********************************************************************
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
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         sql_id.h
 * RCS:          $Id: 
 * Description:  The C-style ADT for manipulating CLI module/object identifiers
 *               
 *               
 * Created:      7/8/98
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#ifndef SQL_ID_H
#define SQL_ID_H

#include "Platform.h"
#include "NAWinNT.h"
#include "SQLCLIdev.h"
#include "BaseTypes.h"
#include "str.h"

#define getModNameLen( sqlmodule_id_ptr ) 				\
    (sqlmodule_id_ptr) -> module_name_len 				
   

#define getIdLen( sqlcli_obj_ptr ) 					\
   	    (sqlcli_obj_ptr) -> identifier_len 			       
   

#define getModCharSet( sqlmodule_id_ptr ) 				\
   (( (sqlmodule_id_ptr) -> version < SQLCLI_CURRENT_VERSION ) ? 	\
    SQLCHARSETSTRING_ISO88591 : (sqlmodule_id_ptr) -> charset		\
   )

#define getIdCharSet( sqlcli_obj_ptr ) 					\
   (( (sqlcli_obj_ptr) -> version < SQLCLI_CURRENT_VERSION ) ?		\
    SQLCHARSETSTRING_ISO88591 : (sqlcli_obj_ptr) -> charset		\
   )
#if 0
SQLMODULE_ID* new_SQLMODULE_ID(
	Lng32 version = SQLCLI_CURRENT_VERSION, 
	const char* module_name = 0,
	Lng32 creation_timestamp = 0,
	const char* charset = SQLCHARSETSTRING_ISO88591,
	Lng32 module_name_len = 0
	);
#endif

void init_SQLMODULE_ID(SQLMODULE_ID* m,
	Lng32 version = SQLCLI_CURRENT_VERSION, 
	const char* module_name = 0, 
	Lng32 timestamp = 0,
	const char* charset = SQLCHARSETSTRING_ISO88591,
	Lng32 name_len = 0
	);
#if 0
SQLCLI_OBJ_ID* new_SQLCLI_OBJ_ID( Lng32 version = SQLCLI_CURRENT_VERSION, 
	enum SQLOBJ_ID_NAME_MODE mode = stmt_name, 
	const SQLMODULE_ID* module = 0, 
	const char* id = 0, 
	void* handle_ = 0,
	const char* charset = SQLCHARSETSTRING_ISO88591,
        Lng32 id_len = 0, Lng32 tag = 0
	);

#define new_SQLSTMT_ID new_SQLCLI_OBJ_ID
#define new_SQLDESC_ID new_SQLCLI_OBJ_ID

#endif
void init_SQLCLI_OBJ_ID(SQLCLI_OBJ_ID* x,
	Lng32 version = SQLCLI_CURRENT_VERSION, 
	enum SQLOBJ_ID_NAME_MODE mode = stmt_name, 
	const SQLMODULE_ID* module = 0, 
	const char* id = 0, 
	void* handle_ = 0,
	const char* charset = SQLCHARSETSTRING_ISO88591,
        Lng32 id_len = 0, Lng32 tag = 0
	);

#define init_SQLSTMT_ID init_SQLCLI_OBJ_ID
#define init_SQLDESC_ID init_SQLCLI_OBJ_ID

Int32 isEqualByName(SQLCLI_OBJ_ID* x, SQLCLI_OBJ_ID* y);

Int32 isEqualByName(const SQLMODULE_ID* x, const SQLMODULE_ID* y);

void setNameForId(SQLCLI_OBJ_ID* x, const char* name, Lng32 len, 
	          const char* charset = SQLCHARSETSTRING_ISO88591);

void setNameForModule(SQLMODULE_ID* x, const char* name, Lng32 len, 
	              const char* charset = SQLCHARSETSTRING_ISO88591);
#endif
