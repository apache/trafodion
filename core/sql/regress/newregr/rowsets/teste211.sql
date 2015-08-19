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
// +++ Code modified on 2003/9/12
**********************************************************************/
/* This test is to verify correct functionality (as specified in Rowsets ES) of the bulk insert
and bulk select features of dynamic Rowsets, from ODBC. This test covers the following aspects
a) Insert into table with different datatypes.
b) Arithmetic expressions in insert query
c) execution time rowset size(M) is same as input array maximum size (N), i.e. M=N
d) negative test with M > N. 
e) bulk select of of different datatypes and nulls. */


#include <stdio.h>
#include <string.h>

#include "defaultSchema.h"

#define ROWSET_SIZE 10

char defaultSchema[MAX_DEFAULT_SCHEMA_LEN + 1];

typedef struct col_b {
  char array[25] ;
} col_b;

SQLSTMT_ID   *stmt = 0;
SQLMODULE_ID *module = 0;
SQLDESC_ID   *arraysize_desc = 0;
SQLDESC_ID   *sql_src = 0;
SQLDESC_ID   *input_desc = 0;
SQLDESC_ID   *temp_desc = 0;
SQLDESC_ID   *commit_desc = 0;
SQLDESC_ID   *odbc_desc = 0;
SQLDESC_ID   *output_desc = 0;
SQLSTMT_ID   *outstmt        = 0;
SQLMODULE_ID *outmodule      = 0;

char *statement_str = 0;

void exitWhenError(Int32 error)
{
  if (error < 0)
    {
      printf( "Error was detected. %d\n",error);
    }
}

Int32 allocStmtAndDescs(char * regrType)
{
  exec sql begin declare section;
  Int32 SQLCODE;
  char cntrl[200];
  exec sql end declare section;

  Int32 retcode;

  retcode = SQL_EXEC_ClearDiagnostics(0);
  exitWhenError(retcode);

  Int32    rowset_size = ROWSET_SIZE ;

  char *odbc_str   = "CONTROL QUERY DEFAULT ODBC_PROCESS 'ON'" ;

  stmt        = new SQLSTMT_ID;
  module      = new SQLMODULE_ID;
  arraysize_desc = new SQLDESC_ID;
  sql_src     = new SQLDESC_ID;
  input_desc = new SQLDESC_ID;
  temp_desc = new SQLDESC_ID;
  commit_desc = new SQLDESC_ID;
  odbc_desc = new SQLDESC_ID;

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
  retcode = 
    SQL_EXEC_SetDescItem( arraysize_desc,
			  1,               // entry #1, attr value
			  SQLDESC_TYPE,    // what to set - data type.
			  SQLTYPECODE_INTEGER,
			  NULL);
  exitWhenError(retcode);

  retcode = 
    SQL_EXEC_SetDescItem( arraysize_desc,
			  1,  // entry #1, attr value.
			  SQLDESC_VAR_PTR, // what to set - host var addr
			  (long) &rowset_size,  // Notice that output from CLI
			                        // will be read from host var
                        			// rowset_size.
			  NULL);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_INPUT_ARRAY_MAXSIZE,
                                 rowset_size,
				 NULL);
  exitWhenError(retcode);
  
				 
  // prelims for ODBC CONTROL QUERY DEFAULT statement
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
                                 strlen(odbc_str) + 1, 0);
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

  // compile and execute CQD statement
  retcode = SQL_EXEC_ExecDirect(stmt, odbc_desc, NULL, 0) ;
  exitWhenError(retcode); 

  // Allocate an input descriptor to send the values
  input_desc->name_mode           = desc_handle;
  input_desc->module              = module;
  module->module_name = 0;
  input_desc->identifier          = 0;
  input_desc->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(input_desc, 500);
  exitWhenError(retcode);

  if (regrType && (regrType[0] == '2'))
    {
      // issue a CQD so float values could be returned in tandem format.
      strcpy(cntrl, "control query default floattype 'TANDEM';");

      exec sql execute immediate :cntrl;
    }

  return 0;
}

Int32 deAllocStmtAndDescs()
{
  // Finish 
  delete stmt;
  delete module;
  delete outstmt;
  delete outmodule;
  delete sql_src;
  delete output_desc;
  delete input_desc;
  delete commit_desc ;

  return 0;
}


Int32 negativeTest()
{
  // NEGATIVE TEST
  statement_str    = "INSERT INTO DYNAMIC2 VALUES(?,?,cast(? as int)+10)";
  
  Int32 retcode;

  Int32    rowset_size = ROWSET_SIZE ;
  Int32    rowset_status[1];    // Has no functionality currently. 
                               //However it is part of RowsetSetDesc API
  Int32 actual_size = 0;

  retcode = SQL_EXEC_ClearDiagnostics(0);
  exitWhenError(retcode);

  printf("\nTrying to insert more rows than INPUT_ARRAY_MAXSIZE. EXPECTING ERROR 30008 \n");
  actual_size = 2*rowset_size ;

  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_TYPE,
                                 SQLTYPECODE_VARCHAR, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
                                 (long)statement_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
                                 strlen(statement_str) + 1, 0);
  exitWhenError(retcode);

  // reset rowset array size to 0.
  retcode = 
    SQL_EXEC_SetDescItem( arraysize_desc,
			  1,  // entry #1, attr value.
			  SQLDESC_VAR_PTR, // what to set - host var addr
			  (long) &rowset_size,  // Notice that output from CLI
			                        // will be read from host var
                        			// rowset_size.
			  NULL);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_INPUT_ARRAY_MAXSIZE,
                                 rowset_size,
                                 NULL
                                );
  exitWhenError(retcode);

  // Prepare the statement
  retcode = SQL_EXEC_Prepare(stmt, sql_src);
  exitWhenError(retcode);

  retcode = SQL_EXEC_DescribeStmt(stmt, input_desc, 0);
  exitWhenError(retcode);

  struct SQLCLI_QUAD_FIELDS    quad_fields[4];

  quad_fields[0].var_layout = 0;
  quad_fields[0].var_ptr = (void *)&actual_size;
  quad_fields[0].ind_layout = 0;
  quad_fields[0].ind_ptr = NULL; 
          
  retcode = SQL_EXEC_SETROWSETDESCPOINTERS(input_desc,
                                           actual_size,
                                           rowset_status,
                                           1,
                                           4,
                                           quad_fields);   // the quad_fields do not contain 
                                           // rowset_size_2 rows of data, so this negative test is
                                           // not really testing what it says, but the error returned
                                           // should be for rowset_size_2 > rowset_size, since that 
                                           // check is done at the very beginning of execution.
  exitWhenError(retcode); 
  
  // Clear Diagnostics
  retcode = SQL_EXEC_ClearDiagnostics(stmt);
  exitWhenError(retcode);

  retcode = SQL_EXEC_ExecFetch(stmt, input_desc, 0);
  exitWhenError(retcode); 

  return 0;
}

Int32 insertRows(char * tablename)
{
  char stmtStrBuf[300];

  strcpy(stmtStrBuf, "INSERT INTO ");
  strcat(stmtStrBuf, tablename);
  strcat(stmtStrBuf, " VALUES(?,?,cast(? as int)+10)");

  statement_str = stmtStrBuf;

  //  statement_str    = "INSERT INTO DYNAMIC2 VALUES(?,?,cast(? as int)+10)";
  /* DDL for DYNAMIC2 is create Table DYNAMIC2 ( a float not null, b char(25), e int); */

  char *commit_str       = "COMMIT WORK" ;

  Int32    retcode;  
  Int32    rowset_size = ROWSET_SIZE ;
  Int32    rowset_status[1];    // Has no functionality currently. 
                               //However it is part of RowsetSetDesc API
  double	  a_array[ROWSET_SIZE];
  col_b   b_array[ROWSET_SIZE];
  Int32 e_array[ROWSET_SIZE];
  
  Int32	  ind[ROWSET_SIZE * 3];
  Int32 actual_size = 0;
  Int32 i = 0;

  // Preparing data for input

  for (i = 0; i < ROWSET_SIZE ; i++) {
    a_array[i] = i + 1.1;
    e_array[i] = i;
    ind[i] = 0;
    ind[ROWSET_SIZE + i] = 0;
    ind[2 * ROWSET_SIZE + i] = 0;
  }
  // Trying to set first row of column A to be null. 
  // Should not succeed as column A is of type NOT NULL.
  ind[0] = -1;   
  // Trying to set second row of column B to be null. Should succeed.
  ind[ROWSET_SIZE+1] = -1;
  // Trying to set third row of column B to be null. Should succeed.
  ind[2*ROWSET_SIZE+2] = -1;
  
  strcpy(&(b_array[0].array[0]), "Tom                     " );
  strcpy(&(b_array[1].array[0]), "Awny                    " );
  strcpy(&(b_array[2].array[0]), "Suresh                  ") ;
  strcpy(&(b_array[3].array[0]), "Hema                    ");
  strcpy(&(b_array[4].array[0]), "Qifan                   ") ;
  strcpy(&(b_array[5].array[0]),"Young                   ");
  strcpy(&(b_array[6].array[0]),"Melody                  ") ;
  strcpy(&(b_array[7].array[0]),"Adrienne                ") ;
  strcpy(&(b_array[8].array[0]),"Kashif                  ") ;
  strcpy(&(b_array[9].array[0]),"John                    ") ;


  // Prepare the statement
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
                                 (long)statement_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
                                 strlen(statement_str) + 1, 0);
  exitWhenError(retcode);


  retcode = SQL_EXEC_Prepare(stmt, sql_src);
  exitWhenError(retcode);

  retcode = SQL_EXEC_DescribeStmt(stmt, input_desc, 0);
  exitWhenError(retcode);

  struct SQLCLI_QUAD_FIELDS    quad_fields[4];

  actual_size = rowset_size  ;
  quad_fields[0].var_layout = 0;
  quad_fields[0].var_ptr = (void *)&actual_size;
  quad_fields[0].ind_layout = 0;
  quad_fields[0].ind_ptr = NULL; 

  quad_fields[1].var_layout = sizeof(a_array[0]);
  quad_fields[1].var_ptr = (void *)&a_array[0];
  quad_fields[1].ind_layout = sizeof(Int32);
  quad_fields[1].ind_ptr = (void *)&ind[0];

  quad_fields[2].var_layout = sizeof(col_b);
  quad_fields[2].var_ptr = (void *)&b_array[0];
  quad_fields[2].ind_layout = sizeof(ind[0]);
  quad_fields[2].ind_ptr = (void *)&ind[ROWSET_SIZE];

  quad_fields[3].var_layout = sizeof(e_array[0]);
  quad_fields[3].var_ptr = (void *)&e_array[0];
  quad_fields[3].ind_layout = sizeof(ind[0]);
  quad_fields[3].ind_ptr = (void *)&ind[2*ROWSET_SIZE];

  retcode = SQL_EXEC_SETROWSETDESCPOINTERS(input_desc,
                                           rowset_size,
                                           rowset_status,
                                           1,
                                           4,
                                           quad_fields);
  exitWhenError(retcode);
 
  // Clear Diagnostics
  retcode = SQL_EXEC_ClearDiagnostics(stmt);
  exitWhenError(retcode);

  retcode = SQL_EXEC_ExecFetch(stmt, input_desc, 0);
  exitWhenError(retcode);

  // prelims for commit statement
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
                                 strlen(commit_str) + 1, 0);
  exitWhenError(retcode);

  retcode = SQL_EXEC_ExecDirect(stmt, commit_desc, NULL, 0) ;
  exitWhenError(retcode);

  return 0;
}

Int32 insertSingleRow(char * tablename)
{
  char stmtStrBuf[300];

  Int32    rowset_size = 0;

  strcpy(stmtStrBuf, "INSERT INTO ");
  strcat(stmtStrBuf, tablename);
  strcat(stmtStrBuf, " VALUES(?,?,cast(? as int)+10)");

  statement_str = stmtStrBuf;

  char *commit_str       = "COMMIT WORK" ;

  Int32    retcode;  

  // Prepare the statement
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
                                 (long)statement_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
                                 strlen(statement_str) + 1, 0);
  exitWhenError(retcode);

  // reset rowset array size to 0.
  retcode = 
    SQL_EXEC_SetDescItem( arraysize_desc,
			  1,  // entry #1, attr value.
			  SQLDESC_VAR_PTR, // what to set - host var addr
			  (long) &rowset_size,  // Notice that output from CLI
			                        // will be read from host var
                        			// rowset_size.
			  NULL);
  exitWhenError(retcode);

  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_INPUT_ARRAY_MAXSIZE,
                                 rowset_size,
                                 NULL
                                );
  exitWhenError(retcode);

  retcode = SQL_EXEC_Prepare(stmt, sql_src);
  exitWhenError(retcode);

  retcode = SQL_EXEC_DescribeStmt(stmt, input_desc, 0);
  exitWhenError(retcode);

  double v1 = 10.23;
  short v1Ind = 0;
  Int32 v2 = 40;
  short v2Ind = 0;
  char name[25];
  strcpy(name, "Anoop                   ");
  short nameInd = 0;
  retcode = SQL_EXEC_SetDescPointers(input_desc,
				     1, 3,
				     (char*)&v1, (char*)&v1Ind,
				     name, (char*)&nameInd,
				     (char*)&v2, (char*)&v2Ind);
				     
  // Clear Diagnostics
  retcode = SQL_EXEC_ClearDiagnostics(stmt);
  exitWhenError(retcode);

  retcode = SQL_EXEC_ExecFetch(stmt, input_desc, 0);
  exitWhenError(retcode);

  // prelims for commit statement
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
                                 strlen(commit_str) + 1, 0);
  exitWhenError(retcode);

  retcode = SQL_EXEC_ExecDirect(stmt, commit_desc, NULL, 0) ;
  exitWhenError(retcode);

  return 0;
}

Int32 selectRows(char * tablename)
{
  printf( "select rows from %s \n", tablename);

  Int32 retcode;

  Int32    rowset_size = ROWSET_SIZE ;
  Int32    rowset_status[1];    // Has no functionality currently. 
                               //However it is part of RowsetSetDesc API

  char stmtStrBuf[300];

  strcpy(stmtStrBuf, "SELECT A, B, E FROM ");
  strcat(stmtStrBuf, tablename);
  strcat(stmtStrBuf, " ORDER BY A");

  char * select_str = stmtStrBuf;

  //  char *select_str    = "SELECT A, B, E FROM DYNAMIC2 ORDER BY A";
  
  Int32  	  rowset_nprocessed = 0;
  Int32    output_ind[ROWSET_SIZE * 3];    // indicator param for nullable values
  double  output_a_array[ROWSET_SIZE];
  col_b   output_b_array[ROWSET_SIZE];
  Int32 output_e_array[ROWSET_SIZE];
  
  output_desc = new SQLDESC_ID;
  outstmt        = new SQLSTMT_ID;
  outmodule      = new SQLMODULE_ID;
  
  retcode = SQL_EXEC_ClearDiagnostics(NULL);

  // Allocate a SQL statement
  outstmt->name_mode           = stmt_handle;
  outstmt->module              = outmodule;
  outmodule->module_name = 0;
  outstmt->identifier          = 0;
  outstmt->handle              = 0;
  retcode = SQL_EXEC_AllocStmt(outstmt, 0);
  exitWhenError(retcode);
  
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_TYPE,
                                 SQLTYPECODE_VARCHAR, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
                                 (long)select_str, 0);
  exitWhenError(retcode);
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
                                 strlen(select_str) + 1, 0);
  exitWhenError(retcode);
  // Prepare the statement
  retcode = SQL_EXEC_Prepare(outstmt, sql_src);
  exitWhenError(retcode);
  
  // Allocate an output descriptor to retrieve the values
  output_desc->name_mode           = desc_handle;
  output_desc->module              = outmodule;
  outmodule->module_name = 0;
  output_desc->identifier          = 0;
  output_desc->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(output_desc, 500);
  exitWhenError(retcode);
  
  // Describe the output entries into the output descriptor
  retcode = SQL_EXEC_DescribeStmt(outstmt, 0, output_desc);
  exitWhenError(retcode);
  
  
  struct SQLCLI_QUAD_FIELDS    output_quad_fields[3];
  
  output_quad_fields[0].var_layout = sizeof(output_a_array[0]);
  output_quad_fields[0].var_ptr = (void *)&output_a_array[0];
  output_quad_fields[0].ind_layout = sizeof(output_ind[0]);
  output_quad_fields[0].ind_ptr = (void *)&output_ind[0];

  output_quad_fields[1].var_layout = sizeof(col_b);
  output_quad_fields[1].var_ptr = (void *)&output_b_array[0];
  output_quad_fields[1].ind_layout = sizeof(output_ind[0]);
  output_quad_fields[1].ind_ptr = (void *)&output_ind[ROWSET_SIZE];

  output_quad_fields[2].var_layout = sizeof(output_e_array[0]);
  output_quad_fields[2].var_ptr = (void *)&output_e_array[0];
  output_quad_fields[2].ind_layout = sizeof(output_ind[0]);
  output_quad_fields[2].ind_ptr = (void *)&output_ind[2*ROWSET_SIZE];


  retcode = SQL_EXEC_SETROWSETDESCPOINTERS(output_desc,
                                           ROWSET_SIZE,
                                           rowset_status,
                                           1,
                                           3,
                                           output_quad_fields);
 
  exitWhenError(retcode);

  // Clear Diagnostics
  retcode = SQL_EXEC_ClearDiagnostics(outstmt);
  exitWhenError(retcode);

  // Execute the statement
  retcode = SQL_EXEC_Exec(outstmt, 0, 0, 0);
  exitWhenError(retcode);

  // Start fetching. 

  retcode = SQL_EXEC_Fetch(outstmt, output_desc, 0, 0);
  exitWhenError(retcode);

  while (retcode == 0 || retcode == 1) // success or warning
    {
      // Find out how many where fetched
      retcode = SQL_EXEC_GetDescItem(output_desc, 1,
                                     SQLDESC_ROWSET_NUM_PROCESSED,
                                     &rowset_nprocessed,
                                     0, 0, 0, 0);
      exitWhenError(retcode);
      
      // Print their values.
      for (Int32 i = 0; i < rowset_nprocessed; i++) 
	{
          if (output_ind[i] == -1)
            printf("NULL  ");
          else
            printf( " %f  " , output_a_array[i]);
	  
          if (output_ind[ROWSET_SIZE + i] == -1)
            printf( " NULL                    ");
          else
            printf( " %s" , output_b_array[i].array);
	  
          if (output_ind[(2*ROWSET_SIZE)+i] == -1)
            printf( " NULL\n");
          else
            printf( " %d\n" , output_e_array[i]);
        }
      // Get the next block of rows
      retcode = SQL_EXEC_Fetch(outstmt, output_desc, 0, 0);
    }
  return 0;
}

Int32 cleanupTables()
{
  exec sql begin declare section;
  Int32 SQLCODE;
  exec sql end declare section;

  exec sql delete from dynamic2;
  exec sql delete from dynamic4;

  return 0;
}

#include "regrInit.cpp"
#include "defaultSchema.cpp"

Int32 main(Int32 argc, char ** argv)
{
  exec sql begin declare section;  
  char buf[64];
  Int32 SQLCODE;
  exec sql end declare section;  

  regrInit();

  setDefaultSchema(defaultSchema, MAX_DEFAULT_SCHEMA_LEN, 0);
  sprintf(buf, "set schema %s;", defaultSchema);
  exec sql execute immediate :buf;

  cleanupTables();

  allocStmtAndDescs((argc > 0) ? argv[1] : 0);

  insertRows("DYNAMIC2");
  selectRows("DYNAMIC2");

  insertRows("DYNAMIC4");
  selectRows("DYNAMIC4");

  cleanupTables();
  insertSingleRow("DYNAMIC2");
  selectRows("DYNAMIC2");

  negativeTest();

  deAllocStmtAndDescs();

  return 0;
}
