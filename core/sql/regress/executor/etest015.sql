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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

EXEC SQL MODULE CAT.SCH.ETEST015M NAMES ARE ISO88591;

EXEC SQL BEGIN DECLARE SECTION;
ROWSET[10] long a_arr;
ROWSET[10] long b_arr;
ROWSET[10] long a1_arr;
ROWSET[10] long b1_arr;

ROWSET[10] long i_arr;
ROWSET[10] long j_arr;
ROWSET[10] long i1_arr;
ROWSET[10] long j1_arr;
ROWSET[10] long k1_arr;

char  statementBuffer[390]; 
char  statementBuffer1[500];     
long  output_rowset_size, num_in;

char  out_desc[13], in_desc[13];
char  out_desc1[15], in_desc1[15];
char  out_desc2[15], in_desc2[15];

long SQLCODE;
EXEC SQL END DECLARE SECTION;

/*****************************************************/
void dynamic_merge()
/*****************************************************/
{                                 
  /* Initialize all variables */
  printf("DYNAMIC_MERGE:\n");  
  strcpy(in_desc,"inscols     "); 
  memset(statementBuffer, ' ', 390);
  statementBuffer[389] = '\0';
  output_rowset_size = 0;
  
  /* INSERTING 10 ROWS */
  
  strcpy(statementBuffer, "control query default odbc_process 'ON';");
  exec sql execute immediate :statementBuffer;
  printf("SQLCODE after exec immed is %d\n", SQLCODE);

  printf("prepare insert:\n");  
  strcpy(statementBuffer,
	 "MERGE INTO T015T1 ON a = ?[10] when matched then update set b = b + ?[10] when not matched then insert values (?[10], ?[10])");

  /* construct S1 from of INSERT statement */
  EXEC SQL PREPARE S1 FROM :statementBuffer; 
  printf("SQLCODE after prepare is %d\n", SQLCODE);
  
  num_in = 30;
  /* create SQLDA for INSERT columns */
  EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :in_desc with MAX :num_in;
  
  /* populate the SQLDA */
  EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc;
  
  EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = ROWSET_SIZE;
  printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);
  
  long loop ;
  for (loop=0; loop<10; loop++) 
    {
      a_arr[loop] = loop;
      b_arr[loop] = 2*loop ;
      a1_arr[loop] = loop;
      b1_arr[loop] = 2*loop ;
    }
  
  EXEC SQL EXECUTE S1 USING :a_arr, :b_arr, :a1_arr, :b1_arr;
  printf("SQLCODE after execute is %d\n", SQLCODE);

  /* 
  long rowcountstmtnum = 102;
  long rowsetrowsaffected[100];
  long retcode ;
  long size = 100;
  long actual_size = 0;

  retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(0, 
					     rowcountstmtnum, 
					     rowsetrowsaffected, 
					     0,
					     size,
					     &actual_size);

  if (retcode == 0) 
   {
     printf("Rowset Row Count Array:\n");
     for(long i=0;i<actual_size;i++)
      printf("Rows affected by row number %d = %d\n",i,rowsetrowsaffected[i]);
     printf("\n");
   }
   else
    printf("SQL_EXEC_GetDiagnosticsStmtInfo2 returned retcode = %d\n", retcode);
    */

  EXEC SQL COMMIT WORK;
  
  EXEC SQL DEALLOCATE DESCRIPTOR :in_desc ;
  EXEC SQL DEALLOCATE PREPARE S1;
  
}

/*****************************************************/
void merge_unique_index_violation()
/*****************************************************/
{                                 
  /* Initialize all variables */
  printf("Merge stmt with unique index:\n");  
  strcpy(in_desc1,"mergeUIV      "); 
  memset(statementBuffer1, ' ', 500);
  statementBuffer[499] = '\0';
  output_rowset_size = 0;
  
  /* INSERTING 10 ROWS */
  
  strcpy(statementBuffer1, "control query default odbc_process 'ON';");
  exec sql execute immediate :statementBuffer1;
  printf("SQLCODE after exec immed is %d\n", SQLCODE);

  printf("prepare Merge:\n");  
  strcpy(statementBuffer1,
	 "MERGE INTO T015T8 ON i = ?[10] when matched then update set j = ?[10] when not matched then insert values (?[10], ?[10], ?[10])");

  /* construct S1 from of INSERT statement */
  EXEC SQL PREPARE S1 FROM :statementBuffer1; 
  printf("SQLCODE after prepare is %d\n", SQLCODE);
  
  num_in = 50;
  /* create SQLDA for INSERT columns */
  EXEC SQL ALLOCATE DESCRIPTOR :in_desc1 with MAX :num_in;
  
  /* populate the SQLDA */
  EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc1;
  
  EXEC SQL GET DESCRIPTOR :in_desc1 :output_rowset_size = ROWSET_SIZE;
  printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);
  
  long loop ;
  for (loop=0; loop<10; loop++) 
    {
      i_arr[loop] = loop +1;
      j_arr[loop] = loop +2;
      i1_arr[loop] = loop +1;
      j1_arr[loop] = loop +2;
      k1_arr[loop] = loop +3;
    }
  j1_arr[1] = 2; /*force unique index violation */ 
  
  EXEC SQL EXECUTE S1 USING :i_arr, :j_arr, :i1_arr, :j1_arr, :k1_arr;
  printf("SQLCODE after execute is %d\n", SQLCODE);

  /* 
  long rowcountstmtnum = 102;
  long rowsetrowsaffected[100];
  long retcode ;
  long size = 100;
  long actual_size = 0;

  retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(0, 
					     rowcountstmtnum, 
					     rowsetrowsaffected, 
					     0,
					     size,
					     &actual_size);

  if (retcode == 0) 
   {
     printf("Rowset Row Count Array:\n");
     for(long i=0;i<actual_size;i++)
      printf("Rows affected by row number %d = %d\n",i,rowsetrowsaffected[i]);
     printf("\n");
   }
   else
    printf("SQL_EXEC_GetDiagnosticsStmtInfo2 returned retcode = %d\n", retcode);
    */

  EXEC SQL COMMIT WORK;
  
  EXEC SQL DEALLOCATE DESCRIPTOR :in_desc1 ;
  EXEC SQL DEALLOCATE PREPARE S1;
  
}

/*****************************************************/
void merge_unique_index()
/*****************************************************/
{                                 
  /* Initialize all variables */
  printf("Merge stmt with unique index:\n");  
  strcpy(in_desc2,"MergeUI       "); 
  memset(statementBuffer1, ' ', 500);
  statementBuffer[499] = '\0';
  output_rowset_size = 0;
  
  /* INSERTING 10 ROWS */
  
  strcpy(statementBuffer1, "control query default odbc_process 'ON';");
  exec sql execute immediate :statementBuffer1;
  printf("SQLCODE after exec immed is %d\n", SQLCODE);

  printf("prepare Merge:\n");  
  strcpy(statementBuffer1,
	 "MERGE INTO T015T10 ON i = ?[10] when matched then update set j = ?[10] when not matched then insert values (?[10], ?[10], ?[10])");

  /* construct S1 from of INSERT statement */
  EXEC SQL PREPARE S1 FROM :statementBuffer1; 
  printf("SQLCODE after prepare is %d\n", SQLCODE);
  
  num_in = 50;
  /* create SQLDA for INSERT columns */
  EXEC SQL ALLOCATE DESCRIPTOR :in_desc2 with MAX :num_in;
  
  /* populate the SQLDA */
  EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc2;
  
  EXEC SQL GET DESCRIPTOR :in_desc2 :output_rowset_size = ROWSET_SIZE;
  printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);
  
  long loop ;
  for (loop=0; loop<10; loop++) 
  {
    i_arr[loop] = loop +1;
    j_arr[loop] = loop +2;
    i1_arr[loop] = loop +1;
    j1_arr[loop] = loop +2;
    k1_arr[loop] = loop +3;
  }
  
  EXEC SQL EXECUTE S1 USING :i_arr, :j_arr, :i1_arr, :j1_arr, :k1_arr;
  printf("SQLCODE after execute is %d\n", SQLCODE);

  /* 
  long rowcountstmtnum = 102;
  long rowsetrowsaffected[100];
  long retcode ;
  long size = 100;
  long actual_size = 0;

  retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(0, 
					     rowcountstmtnum, 
					     rowsetrowsaffected, 
					     0,
					     size,
					     &actual_size);

  if (retcode == 0) 
   {
     printf("Rowset Row Count Array:\n");
     for(long i=0;i<actual_size;i++)
      printf("Rows affected by row number %d = %d\n",i,rowsetrowsaffected[i]);
     printf("\n");
   }
   else
    printf("SQL_EXEC_GetDiagnosticsStmtInfo2 returned retcode = %d\n", retcode);
    */

  EXEC SQL COMMIT WORK;
  
  EXEC SQL DEALLOCATE DESCRIPTOR :in_desc2 ;
  EXEC SQL DEALLOCATE PREPARE S1;
  
}


int main(int argc, char **argv)
{
  /*set the default schema for this program to be the environment default
  */
  char * schema = getenv("TEST_SCHEMA");
  if (!schema)
    schema = "CAT.SCH";
    
  sprintf(statementBuffer, "control query default schema \'%s\';", schema);
  exec sql execute immediate :statementBuffer;
  dynamic_merge();
  merge_unique_index_violation();
  merge_unique_index();

  return 0;
}


