-- @@@ START COPYRIGHT @@@
--
-- Licensed to the Apache Software Foundation (ASF) under one
-- or more contributor license agreements.  See the NOTICE file
-- distributed with this work for additional information
-- regarding copyright ownership.  The ASF licenses this file
-- to you under the Apache License, Version 2.0 (the
-- "License"); you may not use this file except in compliance
-- with the License.  You may obtain a copy of the License at
--
--   http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing,
-- software distributed under the License is distributed on an
-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
-- KIND, either express or implied.  See the License for the
-- specific language governing permissions and limitations
-- under the License.
--
-- @@@ END COPYRIGHT @@@
/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
//
// @@@ END COPYRIGHT @@@
// +++ Copyright added on 2003/12/3
// +++ Code modified on 2003/8/29
**********************************************************************/

/* teste235.sql
 * Mike Hanlon & Suresh Subbiah
 * 05-16-2005
 *
 * CLI tests for Non Atomic Rowsets
 *   Test Classification: Positive & Negative
 *   Test Level: Functional
 *   Test Coverage:
 *   Raise error 8101 during Non-atomic insert. Set statement attribute to ATOMIC
 *   Raise error 8101 during Non-atomic insert. Set statement attribute to UNSPECIFIED
 *   Raise error 30030. Use ATOMIC syntax in input string
 *   Raise error 8101 during Non-atomic insert. Raise warning 30032 by resetting SQL_ATTR_ROWSET_ATOMICITY
 *   Raise error 30025. Set statement attribute to ATOMIC and use non-rowset insert
 *   Raise error 30025. Set statement attribute to ATOMIC on a non-insert statement
 */                                                    

/*  DDL for table nt is
CREATE TABLE nt1 (id int not null, 
eventtime timestamp, 
description varchar(12), 
primary key (id), check (id > 0)) ;   */


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

#include "defaultSchema.h"

#define NAR00 30022
#define SIZE 50

EXEC SQL BEGIN DECLARE SECTION;  
  char buf[64];
/**** host variables for get diagnostics *****/
   NUMERIC(5) i;
   NUMERIC(5) hv_num;
   Int32 hv_sqlcode;
   Int32 hv_rowindex;
   Int32 hv_rowcount;
   char hv_msgtxt[329];
   char hv_sqlstate[6];
   char hv_tabname[129];
   char SQLSTATE[6];
   Int32 SQLCODE;
EXEC SQL END DECLARE SECTION;

char defaultSchema[MAX_DEFAULT_SCHEMA_LEN + 1];

void display_diagnosis();
void exitWhenError(Int32 error) ;
void deleteRows();
void odbcCQD();
void commit();
void test1();
void test2();
void test3();
void test4();
void test5();
void test6();

SQLSTMT_ID   *stmt;  
SQLSTMT_ID   *stmt1;
SQLMODULE_ID *module;
SQLMODULE_ID *module1;

Int32    retcode; 

#include "defaultSchema.cpp"

Int32 main ()
{ 

setDefaultSchema(defaultSchema, MAX_DEFAULT_SCHEMA_LEN, 0);
sprintf(buf, "set schema %s;", defaultSchema);
exec sql execute immediate :buf;

stmt1        = new SQLSTMT_ID;
module1      = new SQLMODULE_ID;
stmt1->name_mode           = stmt_handle;
stmt1->module              = module1;
module1->module_name = 0;
stmt1->identifier          = 0;
stmt1->handle              = 0;
retcode = SQL_EXEC_AllocStmt(stmt1, 0);
exitWhenError(retcode);

test1();
test2();
test3();
test4();
test5();
test6();
return 0;
}


void test1( )
{
  printf("\n **** test1 : Expecting -8101 ***\n");

  stmt        = new SQLSTMT_ID;
  module      = new SQLMODULE_ID;
  char insert_str[128];

  sprintf(insert_str, "INSERT INTO %s.nt1 VALUES(?, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')", defaultSchema);
 
  Int32    rowset_size = SIZE ;
  Int32    actual_size ;
  Int32    rowset_status[1];    // Has no functionality currently. 
                               //However it is part of RowsetSetDesc API
  Int32	  a_int[SIZE];
  Int32 numEntries = 0;
  Int32 i ;


  // Preparing data for input

  for (i = 0; i < SIZE ; i++) {
    a_int[i] = i+1 ;
  }

a_int[39] = -1 ;


  SQLDESC_ID   *sql_src     = new SQLDESC_ID;
  SQLDESC_ID   *input_desc = new SQLDESC_ID;

 

  // Allocate a SQL statement
  stmt->name_mode           = stmt_handle;
  stmt->module              = module;
  module->module_name = 0;
  stmt->identifier          = 0;
  stmt->handle              = 0;
  retcode = SQL_EXEC_AllocStmt(stmt, 0);
  exitWhenError(retcode);


  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_INPUT_ARRAY_MAXSIZE,
				 rowset_size,
				 NULL);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_ROWSET_ATOMICITY,
				 SQL_ATOMIC,
				 NULL);
  exitWhenError(retcode);






  // Allocate a descriptor to hold the sql statement source
  sql_src->name_mode           = desc_handle;
  sql_src->module              = module;
  module->module_name	       = 0;
  sql_src->identifier          = 0;
  sql_src->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(sql_src, 1);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_TYPE,
                                 SQLTYPECODE_VARCHAR, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
                                 (long)insert_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
                                 strlen(insert_str)+1, 0);
  exitWhenError(retcode);

  deleteRows();
  odbcCQD();


  // Prepare the statement
  retcode = SQL_EXEC_Prepare(stmt, sql_src);
  exitWhenError(retcode);

  // Allocate an input descriptor to send the values
  input_desc->name_mode           = desc_handle;
  input_desc->module              = module;
  module->module_name = 0;
  input_desc->identifier          = 0;
  input_desc->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(input_desc, 500);
  exitWhenError(retcode);

  retcode = SQL_EXEC_DescribeStmt(stmt, input_desc, 0);
  exitWhenError(retcode);

 

  struct SQLCLI_QUAD_FIELDS    quad_fields[2];
// In this batch of inserts no null values are to be inserted. 
// So we do not bind the ind_ptr and ind_layout fields. This binding is optional
// if we do not need to insert null values.

  actual_size = rowset_size  ;
  quad_fields[0].var_layout = 0;
  quad_fields[0].var_ptr = (void *)&actual_size;
  quad_fields[0].ind_layout = 0;
  quad_fields[0].ind_ptr = NULL; 

  quad_fields[1].var_layout = sizeof(a_int[0]);
  quad_fields[1].var_ptr = (void *)&a_int[0];
  quad_fields[1].ind_layout = 0;
  quad_fields[1].ind_ptr = NULL;

  

  


  retcode = SQL_EXEC_SETROWSETDESCPOINTERS(input_desc,
                                           rowset_size,
                                           rowset_status,
                                           1,
                                           2,
                                           quad_fields);
  exitWhenError(retcode);

  
  retcode = SQL_EXEC_ExecFetch(stmt, input_desc, 0);
  exitWhenError(retcode);

  if (retcode != 0 && retcode != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",retcode);
  }
  else {
      display_diagnosis();
      commit();
  }




  // Finish section
  delete input_desc;
  delete stmt;  
  delete module;
  delete sql_src;
  return ;
}

void test2( )
{

 printf("\n **** test2 : Expecting -8101 ***\n");
  stmt        = new SQLSTMT_ID;
  module      = new SQLMODULE_ID;
  char insert_str[128];
  sprintf(insert_str, "INSERT INTO %s.nt1 VALUES(?, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')", defaultSchema);
 
  Int32    rowset_size = SIZE ;
  Int32    actual_size ;
  Int32    rowset_status[1];    // Has no functionality currently. 
                               //However it is part of RowsetSetDesc API
  Int32	  a_int[SIZE];
  Int32 numEntries = 0;
  Int32 i ;


  // Preparing data for input

  for (i = 0; i < SIZE ; i++) {
    a_int[i] = i+1 ;
  }

a_int[39] = -1 ;


  SQLDESC_ID   *sql_src     = new SQLDESC_ID;
  SQLDESC_ID   *input_desc = new SQLDESC_ID;

 

  // Allocate a SQL statement
  stmt->name_mode           = stmt_handle;
  stmt->module              = module;
  module->module_name = 0;
  stmt->identifier          = 0;
  stmt->handle              = 0;
  retcode = SQL_EXEC_AllocStmt(stmt, 0);
  exitWhenError(retcode);


  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_INPUT_ARRAY_MAXSIZE,
				 rowset_size,
				 NULL);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_ROWSET_ATOMICITY,
				 SQL_NOT_SPECIFIED,
				 NULL);
  exitWhenError(retcode);






  // Allocate a descriptor to hold the sql statement source
  sql_src->name_mode           = desc_handle;
  sql_src->module              = module;
  module->module_name	       = 0;
  sql_src->identifier          = 0;
  sql_src->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(sql_src, 1);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_TYPE,
                                 SQLTYPECODE_VARCHAR, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
                                 (long)insert_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
                                 strlen(insert_str)+1, 0);
  exitWhenError(retcode);

  deleteRows();
  odbcCQD();


  // Prepare the statement
  retcode = SQL_EXEC_Prepare(stmt, sql_src);
  exitWhenError(retcode);

  // Allocate an input descriptor to send the values
  input_desc->name_mode           = desc_handle;
  input_desc->module              = module;
  module->module_name = 0;
  input_desc->identifier          = 0;
  input_desc->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(input_desc, 500);
  exitWhenError(retcode);

  retcode = SQL_EXEC_DescribeStmt(stmt, input_desc, 0);
  exitWhenError(retcode);

 

  struct SQLCLI_QUAD_FIELDS    quad_fields[2];
// In this batch of inserts no null values are to be inserted. 
// So we do not bind the ind_ptr and ind_layout fields. This binding is optional
// if we do not need to insert null values.

  actual_size = rowset_size  ;
  quad_fields[0].var_layout = 0;
  quad_fields[0].var_ptr = (void *)&actual_size;
  quad_fields[0].ind_layout = 0;
  quad_fields[0].ind_ptr = NULL; 

  quad_fields[1].var_layout = sizeof(a_int[0]);
  quad_fields[1].var_ptr = (void *)&a_int[0];
  quad_fields[1].ind_layout = 0;
  quad_fields[1].ind_ptr = NULL;

  

  


  retcode = SQL_EXEC_SETROWSETDESCPOINTERS(input_desc,
                                           rowset_size,
                                           rowset_status,
                                           1,
                                           2,
                                           quad_fields);
  exitWhenError(retcode);

  
  retcode = SQL_EXEC_ExecFetch(stmt, input_desc, 0);
  exitWhenError(retcode);

  if (retcode != 0 && retcode != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",retcode);
  }
  else {
      display_diagnosis();
      commit();
  }




  // Finish section
  delete input_desc;
  delete stmt;  
  delete module;
  delete sql_src;
  return ;
}

void test3( )
{

   printf("\n **** test3 : Expecting -30030 ***\n");
  stmt        = new SQLSTMT_ID;
  module      = new SQLMODULE_ID;
  char insert_str[128];

  sprintf(insert_str, "INSERT INTO %s.nt1 VALUES(?, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?') ATOMIC", defaultSchema);
 
  Int32    rowset_size = SIZE ;
  Int32    actual_size ;
  Int32    rowset_status[1];    // Has no functionality currently. 
                               //However it is part of RowsetSetDesc API
  Int32	  a_int[SIZE];
  Int32 numEntries = 0;
  Int32 i ;


  // Preparing data for input

  for (i = 0; i < SIZE ; i++) {
    a_int[i] = i+1 ;
  }

a_int[39] = -1 ;


  SQLDESC_ID   *sql_src     = new SQLDESC_ID;
  SQLDESC_ID   *input_desc = new SQLDESC_ID;

 

  // Allocate a SQL statement
  stmt->name_mode           = stmt_handle;
  stmt->module              = module;
  module->module_name = 0;
  stmt->identifier          = 0;
  stmt->handle              = 0;
  retcode = SQL_EXEC_AllocStmt(stmt, 0);
  exitWhenError(retcode);


  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_INPUT_ARRAY_MAXSIZE,
				 rowset_size,
				 NULL);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_ROWSET_ATOMICITY,
				 SQL_NOT_ATOMIC,
				 NULL);
  exitWhenError(retcode);






  // Allocate a descriptor to hold the sql statement source
  sql_src->name_mode           = desc_handle;
  sql_src->module              = module;
  module->module_name	       = 0;
  sql_src->identifier          = 0;
  sql_src->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(sql_src, 1);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_TYPE,
                                 SQLTYPECODE_VARCHAR, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
                                 (long)insert_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
                                 strlen(insert_str)+1, 0);
  exitWhenError(retcode);

  deleteRows();
  odbcCQD();


  // Prepare the statement
  retcode = SQL_EXEC_Prepare(stmt, sql_src);
  exitWhenError(retcode);

  // Finish section
  delete input_desc;
  delete stmt;  
  delete module;
  delete sql_src;
  return;
}

void test4( )
{

 printf("\n **** test4 : Expecting 30032 & 30022 ***\n");
  stmt        = new SQLSTMT_ID;
  module      = new SQLMODULE_ID;
  char insert_str[128];

  sprintf(insert_str, "INSERT INTO %s.nt1 VALUES(?, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')", defaultSchema);
 
  Int32    rowset_size = SIZE ;
  Int32    actual_size ;
  Int32    rowset_status[1];    // Has no functionality currently. 
                               //However it is part of RowsetSetDesc API
  Int32	  a_int[SIZE];
  Int32 numEntries = 0;
  Int32 i ;


  // Preparing data for input

  for (i = 0; i < SIZE ; i++) {
    a_int[i] = i+1 ;
  }

a_int[39] = -1 ;


  SQLDESC_ID   *sql_src     = new SQLDESC_ID;
  SQLDESC_ID   *input_desc = new SQLDESC_ID;

 

  // Allocate a SQL statement
  stmt->name_mode           = stmt_handle;
  stmt->module              = module;
  module->module_name = 0;
  stmt->identifier          = 0;
  stmt->handle              = 0;
  retcode = SQL_EXEC_AllocStmt(stmt, 0);
  exitWhenError(retcode);


  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_INPUT_ARRAY_MAXSIZE,
				 rowset_size,
				 NULL);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_ROWSET_ATOMICITY,
				 SQL_NOT_ATOMIC,
				 NULL);
  exitWhenError(retcode);






  // Allocate a descriptor to hold the sql statement source
  sql_src->name_mode           = desc_handle;
  sql_src->module              = module;
  module->module_name	       = 0;
  sql_src->identifier          = 0;
  sql_src->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(sql_src, 1);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_TYPE,
                                 SQLTYPECODE_VARCHAR, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
                                 (long)insert_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
                                 strlen(insert_str)+1, 0);
  exitWhenError(retcode);

  deleteRows();
  odbcCQD();


  // Prepare the statement
  retcode = SQL_EXEC_Prepare(stmt, sql_src);
  exitWhenError(retcode);

    retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_ROWSET_ATOMICITY,
				 SQL_ATOMIC,
				 NULL);
  exitWhenError(retcode);

   retcode = SQL_EXEC_ClearDiagnostics(stmt);
    exitWhenError(retcode);

  // Allocate an input descriptor to send the values
  input_desc->name_mode           = desc_handle;
  input_desc->module              = module;
  module->module_name = 0;
  input_desc->identifier          = 0;
  input_desc->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(input_desc, 500);
  exitWhenError(retcode);

  retcode = SQL_EXEC_DescribeStmt(stmt, input_desc, 0);
  exitWhenError(retcode);

 

  struct SQLCLI_QUAD_FIELDS    quad_fields[2];
// In this batch of inserts no null values are to be inserted. 
// So we do not bind the ind_ptr and ind_layout fields. This binding is optional
// if we do not need to insert null values.

  actual_size = rowset_size  ;
  quad_fields[0].var_layout = 0;
  quad_fields[0].var_ptr = (void *)&actual_size;
  quad_fields[0].ind_layout = 0;
  quad_fields[0].ind_ptr = NULL; 

  quad_fields[1].var_layout = sizeof(a_int[0]);
  quad_fields[1].var_ptr = (void *)&a_int[0];
  quad_fields[1].ind_layout = 0;
  quad_fields[1].ind_ptr = NULL;

  

  


  retcode = SQL_EXEC_SETROWSETDESCPOINTERS(input_desc,
                                           rowset_size,
                                           rowset_status,
                                           1,
                                           2,
                                           quad_fields);
  exitWhenError(retcode);

  
  retcode = SQL_EXEC_ExecFetch(stmt, input_desc, 0);
  exitWhenError(retcode);

  if (retcode != 0 && retcode != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",retcode);
  }
  else {
      display_diagnosis();
      commit();
  }




  // Finish section
  delete input_desc;
  delete stmt;  
  delete module;
  delete sql_src;
  return;
}

void test5( )
{

   printf("\n **** test5 : Expecting -30025 ***\n");
  stmt        = new SQLSTMT_ID;
  module      = new SQLMODULE_ID;
  char insert_str[128];

  sprintf(insert_str, "INSERT INTO %s.nt1 VALUES(?, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')", defaultSchema);
 
  Int32    rowset_size = SIZE ;
  Int32    actual_size ;
  Int32    rowset_status[1];    // Has no functionality currently. 
                               //However it is part of RowsetSetDesc API
  Int32	  a_int[SIZE];
  Int32 numEntries = 0;
  Int32 i ;


  // Preparing data for input

  for (i = 0; i < SIZE ; i++) {
    a_int[i] = i+1 ;
  }

a_int[39] = -1 ;


  SQLDESC_ID   *sql_src     = new SQLDESC_ID;
  SQLDESC_ID   *input_desc = new SQLDESC_ID;

 

  // Allocate a SQL statement
  stmt->name_mode           = stmt_handle;
  stmt->module              = module;
  module->module_name = 0;
  stmt->identifier          = 0;
  stmt->handle              = 0;
  retcode = SQL_EXEC_AllocStmt(stmt, 0);
  exitWhenError(retcode);


/*  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_INPUT_ARRAY_MAXSIZE,
				 rowset_size,
				 NULL);
  exitWhenError(retcode); */

  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_ROWSET_ATOMICITY,
				 SQL_NOT_ATOMIC,
				 NULL);
  exitWhenError(retcode);






  // Allocate a descriptor to hold the sql statement source
  sql_src->name_mode           = desc_handle;
  sql_src->module              = module;
  module->module_name	       = 0;
  sql_src->identifier          = 0;
  sql_src->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(sql_src, 1);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_TYPE,
                                 SQLTYPECODE_VARCHAR, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
                                 (long)insert_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
                                 strlen(insert_str)+1, 0);
  exitWhenError(retcode);

  deleteRows();
  odbcCQD();


  // Prepare the statement
  retcode = SQL_EXEC_Prepare(stmt, sql_src);
  exitWhenError(retcode);

  // Finish section
  delete input_desc;
  delete stmt;  
  delete module;
  delete sql_src;
  return;
}

void test6( )
{

    printf("\n **** test6 : Expecting -30025 ***\n");
  stmt        = new SQLSTMT_ID;
  module      = new SQLMODULE_ID;
  char insert_str[128];

  sprintf(insert_str, "DELETE FROM %s.nt1", defaultSchema);
 
  Int32    rowset_size = SIZE ;
  Int32    actual_size ;
  Int32    rowset_status[1];    // Has no functionality currently. 
                               //However it is part of RowsetSetDesc API
  Int32	  a_int[SIZE];
  Int32 numEntries = 0;
  Int32 i ;


  // Preparing data for input

  for (i = 0; i < SIZE ; i++) {
    a_int[i] = i+1 ;
  }

a_int[39] = -1 ;


  SQLDESC_ID   *sql_src     = new SQLDESC_ID;
  SQLDESC_ID   *input_desc = new SQLDESC_ID;

 

  // Allocate a SQL statement
  stmt->name_mode           = stmt_handle;
  stmt->module              = module;
  module->module_name = 0;
  stmt->identifier          = 0;
  stmt->handle              = 0;
  retcode = SQL_EXEC_AllocStmt(stmt, 0);
  exitWhenError(retcode);


  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_INPUT_ARRAY_MAXSIZE,
				 rowset_size,
				 NULL);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_ROWSET_ATOMICITY,
				 SQL_NOT_ATOMIC,
				 NULL);
  exitWhenError(retcode);






  // Allocate a descriptor to hold the sql statement source
  sql_src->name_mode           = desc_handle;
  sql_src->module              = module;
  module->module_name	       = 0;
  sql_src->identifier          = 0;
  sql_src->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(sql_src, 1);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_TYPE,
                                 SQLTYPECODE_VARCHAR, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
                                 (long)insert_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
                                 strlen(insert_str)+1, 0);
  exitWhenError(retcode);

  deleteRows();
  odbcCQD();


  // Prepare the statement
  retcode = SQL_EXEC_Prepare(stmt, sql_src);
  exitWhenError(retcode);

 
  // Finish section
  delete input_desc;
  delete stmt;  
  delete module;
  delete sql_src;
  return;
}


/*****************************************************/
void display_diagnosis()
/*****************************************************/
{
  Int32 rowcondnum = 103;
  Int32 retcode ;

  hv_rowcount = -1 ;
  hv_rowindex = -2 ;
  exec sql get diagnostics :hv_num = NUMBER,
		:hv_rowcount = ROW_COUNT;

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[328]='\0';
   memset(hv_sqlstate,' ',sizeof(hv_sqlstate));
   hv_sqlstate[6]='\0';

   printf("Number of conditions  : %d\n", hv_num);
   printf("Number of rows inserted: %d\n", hv_rowcount);
   printf("\n");


  for (i = 1; i <= hv_num; i++) {
      exec sql get diagnostics exception :i                
          :hv_tabname = TABLE_NAME,
          :hv_sqlcode = SQLCODE,
	  :hv_sqlstate = RETURNED_SQLSTATE,
          :hv_msgtxt = MESSAGE_TEXT;

  retcode = SQL_EXEC_GetDiagnosticsCondInfo2(rowcondnum, i, &hv_rowindex, 0,0,0);


   printf("Condition number : %d\n", i);
   printf("ROW INDEX : %d\n", hv_rowindex);
   printf("SQLCODE : %d\n", hv_sqlcode);
   printf("SQLSTATE  : %s\n", hv_sqlstate);
   printf("TABLE   : %s\n", hv_tabname);
   printf("MESSAGE   : %s\n", hv_msgtxt);
   printf("\n");

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[328]='\0';
   memset(hv_tabname,' ',sizeof(hv_tabname));
   hv_tabname[128]='\0';
   memset(hv_sqlstate,' ',sizeof(hv_sqlstate));
   hv_sqlstate[6]='\0';
  }
   SQL_EXEC_ClearDiagnostics(stmt);
}
/* 
{

   Int32 i;
   Int32 hv_num;
   Int32 hv_sqlcode;
   Int32 hv_rowindex;
   Int32 hv_rowcount;
   char hv_msgtxt[229];
   char hv_sqlstate[6];
   char hv_tabname[129];

  Int32 numbernum = 1;
  Int32 rowcountnum = 5;
  Int32 tabnamenum = 12;
  Int32 rowcondnum = 103;
  Int32 sqlcodenum = 204;
  Int32 sqlstatenum = 2;
  Int32 msgtxtnum = 15;

  Int32 tabname_length;
  Int32 sqlstate_length;
  Int32 msgtxt_length;

  Int32 tabname_inpsize = 128;
  Int32 sqlstate_inpsize = 6;
  Int32 msgtxt_inpsize = 228;
  Int32 retcode ;

  hv_rowcount = -1 ;
  hv_rowindex = -2 ;

  retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(0, numbernum, &hv_num, 0,0,0);
  retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(0, rowcountnum, &hv_rowcount, 0,0,0);

   

   printf("Number of conditions  : %d\n", hv_num);
   printf("Number of rows inserted: %d\n", hv_rowcount);
   printf("\n");

   for(i=1; i <= hv_num; i++) {

     memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
     hv_msgtxt[228]='\0';
     memset(hv_tabname,' ',sizeof(hv_tabname));
     hv_tabname[128]='\0';
     memset(hv_sqlstate,' ',sizeof(hv_sqlstate));
     hv_sqlstate[6]='\0';

      retcode = SQL_EXEC_GetDiagnosticsCondInfo2(tabnamenum, i, 0,(char *) &hv_tabname, tabname_inpsize, &tabname_length);
      retcode = SQL_EXEC_GetDiagnosticsCondInfo2(sqlcodenum, i, &hv_sqlcode, 0,0,0);
      retcode = SQL_EXEC_GetDiagnosticsCondInfo2(sqlstatenum, i, 0, (char *) &hv_sqlstate, sqlstate_inpsize, &sqlstate_length);
      retcode = SQL_EXEC_GetDiagnosticsCondInfo2(rowcondnum, i, &hv_rowindex, 0,0,0);
      retcode = SQL_EXEC_GetDiagnosticsCondInfo2(msgtxtnum, i, 0, (char *) &hv_msgtxt, msgtxt_inpsize, &msgtxt_length);
     
     hv_tabname[tabname_inpsize]='\0';
     hv_msgtxt[msgtxt_inpsize]='\0';
     hv_sqlstate[sqlstate_inpsize]='\0';

     printf("Condition number : %d\n", i);
     printf("ROW INDEX : %d\n", hv_rowindex);
     printf("SQLCODE : %d\n", hv_sqlcode);
     printf("SQLSTATE  : %s\n", hv_sqlstate);
     printf("TABLE   : %s\n", hv_tabname);
     printf("MESSAGE   : %s\n", hv_msgtxt);
     printf("\n");
  }

  SQL_EXEC_ClearDiagnostics(stmt);


} */

/*****************************************************/
void exitWhenError(Int32 error)
/*****************************************************/
{
  if (error < 0)
    {
      printf( "Error was detected. %d\n",error);
      if ((error == -8101) || (error == -30030) || (error == -30025))
        SQL_EXEC_ClearDiagnostics(stmt);

    }
  else if (error > 100)
  {
    printf("Warning was detected. %d\n",error);
  }

}

/*****************************************************/
void deleteRows() 
/*****************************************************/
{
    char delete_str[128];
    sprintf(delete_str, "DELETE FROM %s.nt1", defaultSchema);
    SQLDESC_ID   *delete_desc = new SQLDESC_ID;
      // prelims of commit statement
    delete_desc->name_mode           = desc_handle;
    delete_desc->module              = module;
    module->module_name = 0;
    delete_desc->identifier          = 0;
    delete_desc->handle              = 0;
    retcode = SQL_EXEC_AllocDesc(delete_desc, 1);
    exitWhenError(retcode);

    retcode = SQL_EXEC_SetDescItem(delete_desc, 1, SQLDESC_TYPE,
				   SQLTYPECODE_VARCHAR, 0);
    exitWhenError(retcode);
    retcode = SQL_EXEC_SetDescItem(delete_desc, 1, SQLDESC_VAR_PTR,
				   (long)delete_str, 0);
    exitWhenError(retcode);
    retcode = SQL_EXEC_SetDescItem(delete_desc, 1, SQLDESC_LENGTH,
				   strlen(delete_str)+1, 0);
    exitWhenError(retcode);

    retcode = SQL_EXEC_ExecDirect(stmt1, delete_desc, NULL, 0) ;
    exitWhenError(retcode); 
}

/*****************************************************/
void odbcCQD() 
/*****************************************************/
{

    char *odbc_str   = "CONTROL QUERY DEFAULT ODBC_PROCESS 'ON'" ;
    SQLDESC_ID   *odbc_desc = new SQLDESC_ID;
    // prelims of control query default statement.
  odbc_desc->name_mode           = desc_handle;
  odbc_desc->module              = module;
  module->module_name = 0;
  odbc_desc->identifier          = 0;
  odbc_desc->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(odbc_desc, 1);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetDescItem(odbc_desc, 1, SQLDESC_TYPE,
                                 SQLTYPECODE_VARCHAR, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(odbc_desc, 1, SQLDESC_VAR_PTR,
                                 (long)odbc_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(odbc_desc, 1, SQLDESC_LENGTH,
                                 strlen(odbc_str)+1, 0);
  exitWhenError(retcode);


    retcode = SQL_EXEC_ExecDirect(stmt1, odbc_desc, NULL, 0) ;
    exitWhenError(retcode); 
}

/*****************************************************/
void commit() 
/*****************************************************/
{
    char *commit_str       = "COMMIT WORK" ;
    SQLDESC_ID   *commit_desc = new SQLDESC_ID;
      // prelims of commit statement
    commit_desc->name_mode           = desc_handle;
    commit_desc->module              = module;
    module->module_name = 0;
    commit_desc->identifier          = 0;
    commit_desc->handle              = 0;
    retcode = SQL_EXEC_AllocDesc(commit_desc, 1);
    exitWhenError(retcode);

    retcode = SQL_EXEC_SetDescItem(commit_desc, 1, SQLDESC_TYPE,
				   SQLTYPECODE_VARCHAR, 0);
    exitWhenError(retcode);
    retcode = SQL_EXEC_SetDescItem(commit_desc, 1, SQLDESC_VAR_PTR,
				   (long)commit_str, 0);
    exitWhenError(retcode);
    retcode = SQL_EXEC_SetDescItem(commit_desc, 1, SQLDESC_LENGTH,
				   strlen(commit_str)+1, 0);
    exitWhenError(retcode);

    retcode = SQL_EXEC_ExecDirect(stmt1, commit_desc, NULL, 0) ;
    exitWhenError(retcode); 
}
