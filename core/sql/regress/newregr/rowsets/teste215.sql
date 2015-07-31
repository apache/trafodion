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
// (C) Copyright 2002-2014 Hewlett-Packard Development Company, L.P.
//
// @@@ END COPYRIGHT @@@
// +++ Copyright added on 2003/12/3
// +++ Code modified on 2002/3/26
**********************************************************************/
/*                        DYNAMIC ROWSET SELECT TEST

In this test we execute a SELECT A FROM DYNAMIC3 statement, where DYNAMIC3 is 
a table with exactly 3 rows of data in it. The ROWSET_SIZE is set to 10 
(some number larger than 3). We use direct CLI calls to prepare and execute 
the statement so that we can use dynamic rowsets and obtain all three rows in
the output buffer with one call to SQL_EXEC_Fetch. DDL for DYNAMIC3 is
CREATE TABLE DYNAMIC3 (A INT);
 
The purpose of this test is to ensure that we return a retcode = 0 after the first call to 
SQL_EXEC_Fetch and retcode = 100 on the second call to SQL_EXEC_FETCH. This test was 
introduced after fixing the bug reported in CR#10-010608-3266.

This test is extremely sensitive to size of the up-queue from the PA node to the 
root node. While the test will pass for any queue size, it will no longer be testing
the defect in our original source code if the up-queue size different from 4. To
protect the test from dynamic queue resizing we execute a CQD statement setting
the upper limit of the up-queue (from PA node to its parent) to be 4. But the CQD
statement does not protect this test from the initial up-queue size changing from
its current value of 4. For this purpose we have introduced an assert statement
in ComTdbPartnAccess.cpp. If you are reading these comments after having botched that 
assert (i.e. you are trying to change initial up-queue size of PA node to something other 
than 4) then 
(a) please change the initail size of the up-queue to the value you desire (say X).
(b) change the assert statement to assert if the up-queue size is not X
(c) change the setup for this test as follows. The regression this test is trying
to protect us from occurs when a Q_NO_DATA is found at the top of the up-queue 
(from PA node to root node) as it enters the FOR loop (on queuesize) 
in ex_root::fetch() method, after having processed some data rows in previous passes 
through the FOR loop  on the same call to ex_root::fetch(). Say the up-queue size
is 4, then after fetching 3 data rows (with OK_MMORE entries) the up-queue is full 
and we break out of the FOR loop after processing and removing these 3 entries. If 
there are only 3 entries in the table then, when we enter the FOR loop the next time
we have a Q_NO_DATA entry at the top of the queue. This is the situation we would like
to reproduce to ensure that this test is meaningful. So if the queue size is changed to 
X and then change the file regress/rowsetsetup to insert X-1 rows into the table DYNAMIC3.

(d) change the expected file for this test by replacing all occurences of 3 with X-1. 

If for some reason this test has to be disabled then please remove the assert statement
in ComTdbPartnAccess.cpp which checks to see if the initialUpQueueSize is of a certain
fixed value. 

Mike Hanlon and Suresh Subbiah, 08/16/2001 */

   

#include <stdio.h>
#include <string.h>
#include "sqlcli.h"

#include "defaultSchema.h"

#define ROWSET_SIZE 10

char defaultSchema[MAX_DEFAULT_SCHEMA_LEN + 1];

void exitWhenError(Int32 error)
{
  if (error < 0)
    {
		printf( "Error was detected. %d\n",error);
    }
}

#include "regrInit.cpp"
#include "defaultSchema.cpp"

Int32 main( )
{
  exec sql begin declare section;  
  char buf[64];
  Int32 SQLCODE;
  exec sql end declare section;  

  regrInit();

  setDefaultSchema(defaultSchema, MAX_DEFAULT_SCHEMA_LEN, 0);
  sprintf(buf, "set schema %s;", defaultSchema);
  exec sql execute immediate :buf;
 
  Int32    retcode;  
  Int32    rowset_status[1];    // Has no functionality currently. 
                               //However it is part of RowsetSetDesc API
  


 char *queue_str   = "CONTROL QUERY DEFAULT GEN_PA_SIZE_UP '4'" ;



  SQLSTMT_ID   *stmt        = new SQLSTMT_ID;
  SQLMODULE_ID *module      = new SQLMODULE_ID;
  SQLDESC_ID   *sql_src     = new SQLDESC_ID;
  SQLDESC_ID   *queue_desc   = new SQLDESC_ID;


  // Allocate a SQL statement
  stmt->name_mode           = stmt_handle;
  stmt->module              = module;
  module->module_name = 0;
  stmt->identifier          = 0;
  stmt->handle              = 0;
  retcode = SQL_EXEC_AllocStmt(stmt, 0);
  exitWhenError(retcode);


  // Allocate a descriptor to hold the sql statement source
  sql_src->name_mode           = desc_handle;
  sql_src->module              = module;
  module->module_name = 0;
  sql_src->identifier          = 0;
  sql_src->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(sql_src, 1);
  exitWhenError(retcode);


// prelims of control query default statement.
  queue_desc->name_mode           = desc_handle;
  queue_desc->module              = module;
  module->module_name = 0;
  queue_desc->identifier          = 0;
  queue_desc->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(queue_desc, 1);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetDescItem(queue_desc, 1, SQLDESC_TYPE,
                                 SQLTYPECODE_VARCHAR, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(queue_desc, 1, SQLDESC_VAR_PTR,
                                 (long)queue_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(queue_desc, 1, SQLDESC_LENGTH,
                                 strlen(queue_str)+1, 0);
  exitWhenError(retcode);


  // compile and execute CQD statement.
  retcode = SQL_EXEC_ExecDirect(stmt, queue_desc, NULL, 0) ;
  exitWhenError(retcode); 


  /* START BULK SELECT SECTION
This part of the test makes direct use of the CLI to retrieve 
the results of a select query in blocks of 10 rows.  */

char *select_str    = "SELECT A FROM DYNAMIC3";
  
  Int32    rowset_nprocessed = 0;
  Int32    ind[ROWSET_SIZE];              // indicator param for nullable values
  Int32	  output[ROWSET_SIZE];

  SQLDESC_ID   *output_desc = new SQLDESC_ID;

  

  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_TYPE,
                                 SQLTYPECODE_VARCHAR, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
                                 (long)select_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
                                 strlen(select_str)+1, 0);
  exitWhenError(retcode);

  // Prepare the statement
  retcode = SQL_EXEC_Prepare(stmt, sql_src);
  exitWhenError(retcode);

  // Allocate an output descriptor to retrieve the values
  output_desc->name_mode           = desc_handle;
  output_desc->module              = module;
  module->module_name = 0;
  output_desc->identifier          = 0;
  output_desc->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(output_desc, 500);
  exitWhenError(retcode);

  // Describe the output entries into the output descriptor
  retcode = SQL_EXEC_DescribeStmt(stmt, 0, output_desc);
  exitWhenError(retcode);

 
  struct SQLCLI_QUAD_FIELDS    out_quad_fields[1];

  out_quad_fields[0].var_layout = sizeof(output[0]);
  out_quad_fields[0].var_ptr = (void *)&output[0];
  out_quad_fields[0].ind_layout = sizeof(ind[0]);
  out_quad_fields[0].ind_ptr = (void *)&ind[0];

  
  retcode = SQL_EXEC_SETROWSETDESCPOINTERS(output_desc,
                                           ROWSET_SIZE,
                                           rowset_status,
                                           1,
                                           1,
                                           out_quad_fields);
 
  exitWhenError(retcode);

  // Execute the statement
  retcode = SQL_EXEC_Exec(stmt, 0, 0, 0);
  exitWhenError(retcode);

  // Start fetching. 

  retcode = SQL_EXEC_Fetch(stmt, output_desc, 0, 0);
  exitWhenError(retcode);
  printf("retcode = %d", retcode);

  // Find out how many where fetched
  SQL_EXEC_GetDescItem(output_desc, 1,
                       SQLDESC_ROWSET_NUM_PROCESSED,
                       &rowset_nprocessed,
                       0, 0, 0, 0);

  printf("  numrows = %d\n",rowset_nprocessed);

  
  // Call Fetch again tnd check for retcode = 100 to ensure 
  // all rows have been obtained 
  retcode = SQL_EXEC_Fetch(stmt, output_desc, 0, 0);
  printf("retcode = %d", retcode);
  rowset_nprocessed = 10;  //resetting rowset_nprocessed to an arbitrary value.
  SQL_EXEC_GetDescItem(output_desc, 1,
                       SQLDESC_ROWSET_NUM_PROCESSED,
                       &rowset_nprocessed,
                       0, 0, 0, 0);

  printf("  numrows = %d\n",rowset_nprocessed);

  // Finish 
  delete stmt;  
  delete module;
  delete sql_src;
  delete output_desc;
  delete queue_desc;
  return 0;
}
