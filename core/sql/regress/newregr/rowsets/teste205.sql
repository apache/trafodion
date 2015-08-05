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
// +++ Code modified on 2002/12/17
**********************************************************************/
/* This test is to verify correct functionality (as specified in Rowsets ES) of the bulk insert
and bulk select features of Rowsets, from ODBC. This test covers the following aspects
a) CLI call DescribeStmt to check if  Rowset_Size and Rowset_var_layout fields are set correctly.
b) execution time rowset size(M) is same as input array maximum size (N), i.e. M=N
c) same plan is used multiple time with various values of M and M <= N. M=1 is also checked.
d) NULL values are inserted into table only in columns which are nullable.
e) inserted values are selected from table using bulk select feature of rowsets.
 
All tests are positive .
The DDL for the table is Create table DYNAMIC1 (A int, B int, C int not null, d int, e int, f int); */



#include <stdio.h>
#include <string.h>

#include "defaultSchema.h"

#define ROWSET_SIZE 50

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

  char *insert_str    = "INSERT INTO DYNAMIC1 VALUES(cast(?p as int not null),?,?,?,?,?)";
  char *commit_str       = "COMMIT WORK" ;
  char *odbc_str   = "CONTROL QUERY DEFAULT ODBC_PROCESS 'ON'" ;
 
  Int32    retcode;  
  Int32    rowset_size = ROWSET_SIZE ;
  Int32    actual_size ;
  Int32    rowset_status[1];    // Has no functionality currently. 
                               //However it is part of RowsetSetDesc API
  Int32	  input[ROWSET_SIZE * 6];
  Int32	  ind_input[ROWSET_SIZE * 6];
  Int32 check_rowset_size = 0;
  Int32 check_layout_size = 0;
  Int32 numEntries = 0;
  Int32 i ;

  // layout of data in memeory is as follows. 
  // input[0 --> ROWSET_SIZE -1] is for column A
  // input[ROWSET_SIZE --> 2*ROWSET_SIZE -1] is for column B
  // input[2*ROWSET_SIZE --> 3*ROWSET_SIZE -1] is for column C
  // input[3*ROWSET_SIZE --> 4*ROWSET_SIZE -1] is for column D
  // input[4*ROWSET_SIZE --> 5*ROWSET_SIZE -1] is for column E
  // input[5*ROWSET_SIZE --> 6*ROWSET_SIZE -1] is for column F
  // and likewise for the indicator variable "ind".


  // Preparing data for input

  for (i = 0; i < ROWSET_SIZE ; i++) {
    input[i] = i ;
    ind_input[i] = 0;
    input[ROWSET_SIZE + i] = i;
    ind_input[ROWSET_SIZE + i] = 0;
    input[2 * ROWSET_SIZE + i] = i;
    ind_input[2 * ROWSET_SIZE + i] = 0;
    input[3 * ROWSET_SIZE + i] = i;
    ind_input[3 * ROWSET_SIZE + i] = 0;
    input[4 * ROWSET_SIZE + i] = i;
    ind_input[4 * ROWSET_SIZE + i] = 0;
    input[5 * ROWSET_SIZE + i] = i;
    ind_input[5 * ROWSET_SIZE + i] = 0;
  }



  SQLSTMT_ID   *stmt        = new SQLSTMT_ID;
  SQLMODULE_ID *module      = new SQLMODULE_ID;
  SQLDESC_ID   *arraysize_desc = new SQLDESC_ID;
  SQLDESC_ID   *sql_src     = new SQLDESC_ID;
  SQLDESC_ID   *input_desc = new SQLDESC_ID;
  SQLDESC_ID   *temp_desc = new SQLDESC_ID;
  SQLDESC_ID   *commit_desc = new SQLDESC_ID;
  SQLDESC_ID   *odbc_desc = new SQLDESC_ID;



  // Allocate a SQL statement
  stmt->name_mode           = stmt_handle;
  stmt->module              = module;
  module->module_name = 0;
  stmt->identifier          = 0;
  stmt->handle              = 0;
  retcode = SQL_EXEC_AllocStmt(stmt, 0);
  exitWhenError(retcode);

// Allocate a descriptor to set input array maximum size
  arraysize_desc->name_mode            = desc_handle;
  arraysize_desc ->module              = module;
  module->module_name = 0;
  arraysize_desc ->identifier          = 0;
  arraysize_desc ->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(arraysize_desc, 1);
  exitWhenError(retcode);

// set input_array_maxsize
  retcode = SQL_EXEC_SetDescItem( arraysize_desc,
                                  1,                    // entry #1, attr value
                                  SQLDESC_TYPE,    // what to set - data type.
                                  SQLTYPECODE_INTEGER,
                                  NULL);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetDescItem( arraysize_desc,
                                  1,                    // entry #1, attr value.
                                  SQLDESC_VAR_PTR,      // what to set - host var addr
                                  (long) &rowset_size,    // Notice that output from CLI
                                  // will be read from host var
                                  // rowset_size.
                                  NULL);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_INPUT_ARRAY_MAXSIZE,
				 rowset_size,
				 NULL);
  exitWhenError(retcode);

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




  // Allocate a descriptor to hold the sql statement source
  sql_src->name_mode           = desc_handle;
  sql_src->module              = module;
  module->module_name = 0;
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

  // compile and execute CQD statement.
  retcode = SQL_EXEC_ExecDirect(stmt, odbc_desc, NULL, 0) ;
  exitWhenError(retcode); 


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

  // Allocate a temporary descriptor
  temp_desc->name_mode           = desc_handle;
  temp_desc->module              = module;
  module->module_name = 0;
  temp_desc->identifier          = 0;
  temp_desc->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(temp_desc, 500);
  exitWhenError(retcode);


  //check if the statment has been prepared correctly
  retcode = SQL_EXEC_SetDescItem(temp_desc,
                                 1,
                                 SQLDESC_TYPE, 
                                 SQLTYPECODE_INTEGER,
                                 NULL);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetDescItem(temp_desc,
                                 1, 
                                 SQLDESC_VAR_PTR, 
                                 (long)(&numEntries), 
                                 NULL);
  exitWhenError(retcode);
  
  retcode = SQL_EXEC_GetDescEntryCount(input_desc, temp_desc);
  exitWhenError(retcode);

  //check if number of columns is 7
  printf("Expected number of input columns is 7.\n Actual number of input colums = %d\n", 
          numEntries); 

  //check if maximum input array size size is part of compiled plan
  retcode = SQL_EXEC_GetDescItem(input_desc, 
                                 0, 
                                 SQLDESC_ROWSET_SIZE, 
                                 &check_rowset_size, 
                                 NULL,
                                 0,
                                 NULL,
                                 0);
  exitWhenError(retcode);
  printf("Expected maximum input array size = %d.\n Actual maximum input array size = %d\n", 
          rowset_size, check_rowset_size);
  
Int32 check_datatype = 0 ;
retcode = SQL_EXEC_GetDescItem(input_desc, 
                                 1, 
                                 SQLDESC_TYPE, 
                                 &check_datatype, 
                                 NULL,
                                 0,
                                 NULL,
                                 0);
  exitWhenError(retcode);
  printf("Expected datatype = 4.\n Actual datatype = %d\n", 
         check_datatype); 




  //check if column-wise binding is specified correctly

  for (i=2; i <= numEntries; i++) {

    retcode = SQL_EXEC_GetDescItem(input_desc, 
                                   i, 
                                   SQLDESC_ROWSET_VAR_LAYOUT_SIZE, 
                                   &check_layout_size, 
                                   NULL,
                                   0,
                                   NULL,
                                   0);
    exitWhenError(retcode);
    printf("Expected var layout size = %d.\n Actual var layout size = %d\n", 
           4, check_layout_size); 

  retcode = SQL_EXEC_GetDescItem(input_desc, 
                                 i, 
                                 SQLDESC_TYPE, 
                                 &check_datatype, 
                                 NULL,
                                 0,
                                 NULL,
                                 0);
  exitWhenError(retcode);
  printf("Expected datatype = 4.\n Actual datatype = %d\n", 
         check_datatype); 



    printf("\n\n");
  }

  struct SQLCLI_QUAD_FIELDS    quad_fields[7];
// In the first batch of inserts no null values are to be inserted. 
// So we do not bind the ind_ptr and ind_layout fields. This binding is optional
// if we do not need to insert null values.
  actual_size = rowset_size  ;
  quad_fields[0].var_layout = 0;
  quad_fields[0].var_ptr = (void *)&actual_size;
  quad_fields[0].ind_layout = 0;
  quad_fields[0].ind_ptr = NULL; 

  quad_fields[1].var_layout = sizeof(input[0]);
  quad_fields[1].var_ptr = (void *)&input[0];
  quad_fields[1].ind_layout = 0;
  quad_fields[1].ind_ptr = NULL;

  quad_fields[2].var_layout = sizeof(input[0]);
  quad_fields[2].var_ptr = (void *)&input[ROWSET_SIZE];
  quad_fields[2].ind_layout = 0;
  quad_fields[2].ind_ptr = NULL;

  quad_fields[3].var_layout = sizeof(input[0]);
  quad_fields[3].var_ptr = (void *)&input[2*ROWSET_SIZE];
  quad_fields[3].ind_layout = 0;
  quad_fields[3].ind_ptr = NULL;

  quad_fields[4].var_layout = sizeof(input[0]);
  quad_fields[4].var_ptr = (void *)&input[3*ROWSET_SIZE];
  quad_fields[4].ind_layout = 0;
  quad_fields[4].ind_ptr = NULL;

  quad_fields[5].var_layout = sizeof(input[0]);
  quad_fields[5].var_ptr = (void *)&input[4*ROWSET_SIZE];
  quad_fields[5].ind_layout = 0;
  quad_fields[5].ind_ptr = NULL;

  quad_fields[6].var_layout = sizeof(input[0]);
  quad_fields[6].var_ptr = (void *)&input[5*ROWSET_SIZE];
  quad_fields[6].ind_layout = 0;
  quad_fields[6].ind_ptr = NULL;

  


  retcode = SQL_EXEC_SETROWSETDESCPOINTERS(input_desc,
                                           rowset_size,
                                           rowset_status,
                                           1,
                                           7,
                                           quad_fields);
  exitWhenError(retcode);

  
  retcode = SQL_EXEC_ExecFetch(stmt, input_desc, 0);
  exitWhenError(retcode);

  
 

  // batch insert of some more rows into SQL table. This time with less than max.
  // number of rows

  Int32 rowset_size_2 = 25 ;  

  if (ROWSET_SIZE <= rowset_size_2 ) {
    printf("The variable rowset_size_2 must be smaller than previously defined input array maxsize\n");
    return 0;
  }

  for (i=0; i<rowset_size_2; i++) {
    input[i] = ROWSET_SIZE + 10*i;
    ind_input[i] = 0;
    input[ROWSET_SIZE + i] = ROWSET_SIZE + 11*i;
    ind_input[ROWSET_SIZE + i] = 0;
    input[2 * ROWSET_SIZE + i] = ROWSET_SIZE + 12*i;
    ind_input[2 * ROWSET_SIZE + i] = 0;
    input[3 * ROWSET_SIZE + i] = ROWSET_SIZE + 13*i;
    ind_input[3 * ROWSET_SIZE + i] = 0;
    input[4 * ROWSET_SIZE + i] = ROWSET_SIZE + 14*i;
    ind_input[4 * ROWSET_SIZE + i] = 0;
    input[5 * ROWSET_SIZE + i] = 0;
    ind_input[5 * ROWSET_SIZE + i] = -1;   // column F is all NULL in this second batch of inserts.
  }

  ind_input[ROWSET_SIZE] = -1 ;  // We also make column B of first row of the second batch of inserts to be NULL
// Column A and Column C have not ind_layout and ind_ptr fields since 
// they are of type NOT NULL.
  actual_size = rowset_size_2 ;
  quad_fields[0].var_layout = 0;
  quad_fields[0].var_ptr = (void *)&actual_size;
  quad_fields[0].ind_layout = 0;
  quad_fields[0].ind_ptr = NULL; 

  quad_fields[1].var_layout = sizeof(input[0]);
  quad_fields[1].var_ptr = (void *)&input[0];
  quad_fields[1].ind_layout = 0;
  quad_fields[1].ind_ptr = NULL;

  quad_fields[2].var_layout = sizeof(input[0]);
  quad_fields[2].var_ptr = (void *)&input[ROWSET_SIZE];
  quad_fields[2].ind_layout = sizeof(ind_input[0]);
  quad_fields[2].ind_ptr = (void *)&ind_input[ROWSET_SIZE];

  quad_fields[3].var_layout = sizeof(input[0]);
  quad_fields[3].var_ptr = (void *)&input[2*ROWSET_SIZE];
  quad_fields[3].ind_layout = 0;
  quad_fields[3].ind_ptr = NULL;

  quad_fields[4].var_layout = sizeof(input[0]);
  quad_fields[4].var_ptr = (void *)&input[3*ROWSET_SIZE];
  quad_fields[4].ind_layout = sizeof(ind_input[0]);
  quad_fields[4].ind_ptr = (void *)&ind_input[3*ROWSET_SIZE];

  quad_fields[5].var_layout = sizeof(input[0]);
  quad_fields[5].var_ptr = (void *)&input[4*ROWSET_SIZE];
  quad_fields[5].ind_layout = sizeof(ind_input[0]);
  quad_fields[5].ind_ptr = (void *)&ind_input[4*ROWSET_SIZE];

  quad_fields[6].var_layout = sizeof(input[0]);
  quad_fields[6].var_ptr = (void *)&input[5*ROWSET_SIZE];
  quad_fields[6].ind_layout = sizeof(ind_input[0]);
  quad_fields[6].ind_ptr = (void *)&ind_input[5*ROWSET_SIZE];

 


  retcode = SQL_EXEC_SETROWSETDESCPOINTERS(input_desc,
                                           rowset_size,
                                           rowset_status,
                                           1,
                                           7,
                                           quad_fields);
  exitWhenError(retcode);
 
  retcode = SQL_EXEC_ExecFetch(stmt, input_desc, 0);
  exitWhenError(retcode);


  
  // third batch insert of more rows into SQL table. This time only one row will be inserted. 

  Int32 rowset_size_3 = 1 ;  


    input[0] = 1000;
    ind_input[0] = -1;   //trying to insert null into a column which has been cast as not null
                         // should not succeed. The last row should have column A = 1000;
    input[1] = 2000;
    ind_input[1] = 0;
    input[2] = 3000;
    ind_input[2] = -1;
    input[3] = 4000;
    ind_input[3] = -1;
    input[4] = 5000;
    ind_input[4] = -1;
    input[5] = 6000;
    ind_input[5] = 0;   

// We try to insert NULL into column A but it should fail as the column has been cast to be 
// of type NOT NULL
    
  actual_size = rowset_size_3  ;
  quad_fields[0].var_layout = 0;
  quad_fields[0].var_ptr = (void *)&actual_size;
  quad_fields[0].ind_layout = 0;
  quad_fields[0].ind_ptr = NULL; 

  quad_fields[1].var_layout = sizeof(input[0]);
  quad_fields[1].var_ptr = (void *)&input[0];
  quad_fields[1].ind_layout = sizeof(ind_input[0]);
  quad_fields[1].ind_ptr = (void *)&ind_input[0];

  quad_fields[2].var_layout = sizeof(input[0]);
  quad_fields[2].var_ptr = (void *)&input[1];
  quad_fields[2].ind_layout = sizeof(ind_input[0]);
  quad_fields[2].ind_ptr = (void *)&ind_input[1];

  quad_fields[3].var_layout = sizeof(input[0]);
  quad_fields[3].var_ptr = (void *)&input[2];
  quad_fields[3].ind_layout = sizeof(ind_input[0]);
  quad_fields[3].ind_ptr = (void *)&ind_input[2];

  quad_fields[4].var_layout = sizeof(input[0]);
  quad_fields[4].var_ptr = (void *)&input[3];
  quad_fields[4].ind_layout = sizeof(ind_input[0]);
  quad_fields[4].ind_ptr = (void *)&ind_input[3];

  quad_fields[5].var_layout = sizeof(input[0]);
  quad_fields[5].var_ptr = (void *)&input[4];
  quad_fields[5].ind_layout = sizeof(ind_input[0]);
  quad_fields[5].ind_ptr = (void *)&ind_input[4];

  quad_fields[6].var_layout = sizeof(input[0]);
  quad_fields[6].var_ptr = (void *)&input[5];
  quad_fields[6].ind_layout = sizeof(ind_input[0]);
  quad_fields[6].ind_ptr = (void *)&ind_input[5];

  retcode = SQL_EXEC_SETROWSETDESCPOINTERS(input_desc,
                                           rowset_size,
                                           rowset_status,
                                           1,
                                           7,
                                           quad_fields);
  exitWhenError(retcode);
 
  retcode = SQL_EXEC_ExecFetch(stmt, input_desc, 0);
  exitWhenError(retcode);  
  

 retcode = SQL_EXEC_ExecDirect(stmt, commit_desc, NULL, 0) ;
  exitWhenError(retcode); 


  // Finish bulk insert section
  delete arraysize_desc;
  delete input_desc;
  delete temp_desc;
  delete commit_desc;



  /* START BULK SELECT SECTION
This part of the test makes direct use of the CLI to retrieve 
the results of a select query in blocks of 50 rows.  */

char *select_str    = "SELECT A, B, C, D, E, F FROM DYNAMIC1 ORDER BY A";
  
  Int32    rowset_nprocessed = 0;
  Int32    ind[ROWSET_SIZE * 6];              // indicator param for nullable values
  Int32	  output[ROWSET_SIZE * 6];

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

 
  struct SQLCLI_QUAD_FIELDS    out_quad_fields[6];

  out_quad_fields[0].var_layout = sizeof(output[0]);
  out_quad_fields[0].var_ptr = (void *)&output[0];
  out_quad_fields[0].ind_layout = sizeof(ind[0]);
  out_quad_fields[0].ind_ptr = (void *)&ind[0];

  out_quad_fields[1].var_layout = sizeof(output[0]);
  out_quad_fields[1].var_ptr = (void *)&output[ROWSET_SIZE];
  out_quad_fields[1].ind_layout = sizeof(ind[0]);
  out_quad_fields[1].ind_ptr = (void *)&ind[ROWSET_SIZE];

  out_quad_fields[2].var_layout = sizeof(output[0]);
  out_quad_fields[2].var_ptr = (void *)&output[2*ROWSET_SIZE];
  out_quad_fields[2].ind_layout = sizeof(ind[0]);
  out_quad_fields[2].ind_ptr = (void *)&ind[2*ROWSET_SIZE];

  out_quad_fields[3].var_layout = sizeof(output[0]);
  out_quad_fields[3].var_ptr = (void *)&output[3*ROWSET_SIZE];
  out_quad_fields[3].ind_layout = sizeof(ind[0]);
  out_quad_fields[3].ind_ptr = (void *)&ind[3*ROWSET_SIZE];

  out_quad_fields[4].var_layout = sizeof(output[0]);
  out_quad_fields[4].var_ptr = (void *)&output[4*ROWSET_SIZE];
  out_quad_fields[4].ind_layout = sizeof(ind[0]);
  out_quad_fields[4].ind_ptr = (void *)&ind[4*ROWSET_SIZE];

  out_quad_fields[5].var_layout = sizeof(output[0]);
  out_quad_fields[5].var_ptr = (void *)&output[5*ROWSET_SIZE];
  out_quad_fields[5].ind_layout = sizeof(ind[0]);
  out_quad_fields[5].ind_ptr = (void *)&ind[5*ROWSET_SIZE];

  retcode = SQL_EXEC_SETROWSETDESCPOINTERS(output_desc,
                                           ROWSET_SIZE,
                                           rowset_status,
                                           1,
                                           6,
                                           out_quad_fields);
 
  exitWhenError(retcode);

  // Execute the statement
  retcode = SQL_EXEC_Exec(stmt, 0, 0, 0);
  exitWhenError(retcode);

  // Start fetching. 

  retcode = SQL_EXEC_Fetch(stmt, output_desc, 0, 0);
  exitWhenError(retcode);

  while (retcode == 0 || retcode == 1) // success or warning
    {
      // Find out how many where fetched
      retcode = SQL_EXEC_GetDescItem(output_desc, 1,
                                     SQLDESC_ROWSET_NUM_PROCESSED,
                                     &rowset_nprocessed,
                                     0, 0, 0, 0);
      exitWhenError(retcode);// Print their values.
      for (Int32 i = 0; i < rowset_nprocessed; i++) 
      {
          if (ind[i] == -1)
            printf("NULL  ");
          else
            printf( " %d  " , output[i]);

          if (ind[ROWSET_SIZE + i] == -1)
            printf( "NULL  ");
          else
            printf( " %d  " , output[ROWSET_SIZE + i]);

          if (ind[(2*ROWSET_SIZE) + i] == -1)
            printf( "NULL  ");
          else
            printf( " %d  " , output[(2*ROWSET_SIZE) + i]);

          if (ind[(3*ROWSET_SIZE) + i] == -1)
            printf( "NULL  ");
          else
            printf( " %d  " , output[(3*ROWSET_SIZE)+i]);

          if (ind[(4*ROWSET_SIZE)+i] == -1)
            printf( "NULL  ");
          else
            printf( " %d  " , output[(4*ROWSET_SIZE)+i]);

          if (ind[(5*ROWSET_SIZE)+i] == -1)
            printf( "NULL\n");
          else
            printf( " %d\n" , output[(5*ROWSET_SIZE)+i]);
        }
      // Get the next block of rows
	  retcode = SQL_EXEC_Fetch(stmt, output_desc, 0, 0);
    }

  // Finish 
  delete stmt;  
  delete module;
  delete sql_src;
  delete output_desc;
  return 0;
}
