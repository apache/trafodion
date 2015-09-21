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
// +++ Copyright added on 2003/12/3
// +++ Code modified on 2002/12/17
**********************************************************************/
/* This tests checks if the statement attribute INPUT_ARRAY_MAXSIZE can be set to
various positive values. It also tests negatively to ensure that attempting
to set this attribute to zero or negative values will raise the appropriate errors. */

#include <stdio.h>
#include <string.h>

void exitWhenError(Int32 error)
{
  if (error < 0)
    {
		printf( "Error was detected. %d\n",error);
    }
}


Int32 main( )
{
  Int32    retcode;
  Int32 rowset_size = 10 ;
  Int32 check_size = 0;
  
 
  SQLSTMT_ID   *stmt        = new SQLSTMT_ID;
  SQLMODULE_ID *module      = new SQLMODULE_ID;
  SQLDESC_ID   *arraysize_desc = new SQLDESC_ID;
  SQLDESC_ID   *checksize_desc = new SQLDESC_ID;
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


// Allocate a descriptor to set input array maximum size
  arraysize_desc->name_mode            = desc_handle;
  arraysize_desc ->module              = module;
  module->module_name = 0;
  arraysize_desc ->identifier          = 0;
  arraysize_desc ->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(arraysize_desc, 10);
  exitWhenError(retcode);

// Allocate a descriptor to check the set value of array maximum size
  checksize_desc->name_mode            = desc_handle;
  checksize_desc ->module              = module;
  module->module_name = 0;
  checksize_desc ->identifier          = 0;
  checksize_desc ->handle              = 0;
  retcode = SQL_EXEC_AllocDesc(checksize_desc, 10);
  exitWhenError(retcode);


// set input_array_maxsize #1
retcode = SQL_EXEC_SetDescItem(
          arraysize_desc,
          1,                    // entry #1, attr value
          SQLDESC_TYPE,    // what to set - data type.
          SQLTYPECODE_INTEGER,
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetDescItem(
          arraysize_desc,
          1,                    // entry #1, attr value.
          SQLDESC_VAR_PTR,      // what to set - host var addr
          (long) &rowset_size,    // Notice that output to CLI
                                 // will be read from host var
                                // rowset_size.
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetStmtAttr(stmt,
                               SQL_ATTR_INPUT_ARRAY_MAXSIZE,
			       rowset_size,
			       NULL);
//                               arraysize_desc);
exitWhenError(retcode);


// check input array maximum size  #1
retcode = SQL_EXEC_SetDescItem(
          checksize_desc,
          1,                    // entry #1, attr value
          SQLDESC_TYPE,    // what to set - data type.
          SQLTYPECODE_INTEGER,
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetDescItem(
          checksize_desc,
          1,                    // entry #1, attr value.
          SQLDESC_VAR_PTR,      // what to get - host var addr
          (long)&check_size,    // Notice that input from CLI
                                 // will be read into host var
                                // check_size.
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_GetStmtAttr(stmt,
                               SQL_ATTR_INPUT_ARRAY_MAXSIZE,
			       &check_size,
			       NULL, 0, NULL);
//                               checksize_desc);
exitWhenError(retcode);


printf("POSITIVE TESTS\n");
//print comparision #1
printf("Input array maximum size is %d. Expected value %d\n", check_size, rowset_size);

// set input_array_maxsize #2
rowset_size = 1;

retcode = SQL_EXEC_SetDescItem(
          arraysize_desc,
          1,                    // entry #1, attr value
          SQLDESC_TYPE,    // what to set - data type.
          SQLTYPECODE_INTEGER,
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetDescItem(
          arraysize_desc,
          1,                    // entry #1, attr value.
          SQLDESC_VAR_PTR,      // what to set - host var addr
          (long) &rowset_size,    // Notice that output to CLI
                                 // will be read from host var
                                // rowset_size.
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetStmtAttr(stmt,
                               SQL_ATTR_INPUT_ARRAY_MAXSIZE,
			       rowset_size,
			       NULL);
exitWhenError(retcode);


// check input array maximum size #2
retcode = SQL_EXEC_SetDescItem(
          checksize_desc,
          1,                    // entry #1, attr value
          SQLDESC_TYPE,    // what to set - data type.
          SQLTYPECODE_INTEGER,
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetDescItem(
          checksize_desc,
          1,                    // entry #1, attr value.
          SQLDESC_VAR_PTR,      // what to get - host var addr
          (long)&check_size,    // Notice that input from CLI
                                 // will be read into host var
                                // check_size.
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_GetStmtAttr(stmt,
                               SQL_ATTR_INPUT_ARRAY_MAXSIZE,
			       &check_size,
			       NULL, 0, NULL);
exitWhenError(retcode);


//print comparision #2
printf("Input array maximum size is %d. Expected value %d\n", check_size, rowset_size);

// set input_array_maxsize #3
rowset_size = 1000000;

retcode = SQL_EXEC_SetDescItem(
          arraysize_desc,
          1,                    // entry #1, attr value
          SQLDESC_TYPE,    // what to set - data type.
          SQLTYPECODE_INTEGER,
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetDescItem(
          arraysize_desc,
          1,                    // entry #1, attr value.
          SQLDESC_VAR_PTR,      // what to set - host var addr
          (long) &rowset_size,    // Notice that output to CLI
                                 // will be read from host var
                                // rowset_size.
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetStmtAttr(stmt,
                               SQL_ATTR_INPUT_ARRAY_MAXSIZE,
			       rowset_size,
			       NULL);
exitWhenError(retcode);


// check input array maximum size #3
retcode = SQL_EXEC_SetDescItem(
          checksize_desc,
          1,                    // entry #1, attr value
          SQLDESC_TYPE,    // what to set - data type.
          SQLTYPECODE_INTEGER,
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetDescItem(
          checksize_desc,
          1,                    // entry #1, attr value.
          SQLDESC_VAR_PTR,      // what to get - host var addr
          (long)&check_size,    // Notice that input from CLI
                                 // will be read into host var
                                // check_size.
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_GetStmtAttr(stmt,
                               SQL_ATTR_INPUT_ARRAY_MAXSIZE,
			       &check_size,
			       NULL, 0, NULL);
exitWhenError(retcode);


//print comparision #3
printf("Input array maximum size is %d. Expected value %d\n", check_size, rowset_size);

//NEGATIVE TESTS
printf("\nNEGATIVE TESTS\n");
// set input_array_maxsize #-1
printf("Attemting to set INPUT_ARRAY_MAXSIZE = -1, error 8856 expected\n");
rowset_size = -1;

retcode = SQL_EXEC_SetDescItem(
          arraysize_desc,
          1,                    // entry #1, attr value
          SQLDESC_TYPE,    // what to set - data type.
          SQLTYPECODE_INTEGER,
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetDescItem(
          arraysize_desc,
          1,                    // entry #1, attr value.
          SQLDESC_VAR_PTR,      // what to set - host var addr
          (long) &rowset_size,    // Notice that output to CLI
                                 // will be read from host var
                                // rowset_size.
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetStmtAttr(stmt,
                               SQL_ATTR_INPUT_ARRAY_MAXSIZE,
			       rowset_size,
			       NULL);
exitWhenError(retcode); 


// set input_array_maxsize #-2
// This is no longer a negative test, setting rowset size to 0 is allowed.
printf("Attemting to set INPUT_ARRAY_MAXSIZE = 0, No error expected\n");
rowset_size = 0;

retcode = SQL_EXEC_SetDescItem(
          arraysize_desc,
          1,                    // entry #1, attr value
          SQLDESC_TYPE,    // what to set - data type.
          SQLTYPECODE_INTEGER,
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetDescItem(
          arraysize_desc,
          1,                    // entry #1, attr value.
          SQLDESC_VAR_PTR,      // what to set - host var addr
          (long) &rowset_size,    // Notice that output to CLI
                                 // will be read from host var
                                // rowset_size.
          NULL);
exitWhenError(retcode);

retcode = SQL_EXEC_SetStmtAttr(stmt,
                               SQL_ATTR_INPUT_ARRAY_MAXSIZE,
			       rowset_size,
			       NULL);
exitWhenError(retcode);

delete stmt ;
delete module;
delete arraysize_desc;
delete checksize_desc;
delete sql_src;
delete input_desc;

return 0 ;
}

 

 

