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
/* 
-- Test: TEST231 (Rowsets)
-- Owner: Compiler Team
-- Functionality: Input rowsets for Insert, update, delete and select 
-- statements
-- Expected files: EXPECTED231
-- Table created: tpassdp2
-- Indexes : if1
-- Limitations:
-- Revision history:
--     (2/19/04) - Created TESTE231 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This test case tests for large input rowsets. Executor in DP2 has 
   restrictions on the data that it can hold, normally 31000. This test case 
   tests that we do not attemp to pass the complete rowset to DP2 */

EXEC SQL MODULE CAT.SCH.TESTE231M NAMES ARE ISO88591;

/* Globals */

Int32 SQLCODE;
#define SIZE 50000

EXEC SQL BEGIN DECLARE SECTION;
ROWSET [SIZE] char f1[100];
ROWSET [SIZE] char fupdate[100];
ROWSET [SIZE] float f2;
ROWSET [SIZE] float fcount;
Int32 i;
Int32 count;
EXEC SQL END DECLARE SECTION;

/* Only test_insert actually uses data, the rest of the operations like 
 * update, select and delete act on empty tables. The reason being that 
 * these operations are time consuming, our only intent is to see that we 
 * dont crash the master executor or DP2 by sending the complete rowset in one 
 * shot to DP2 */

void test_insert();
void test_update();
void test_select();
void test_delete();
void displayRowsetRowCountArray();

Int32 main() {
  
  EXEC SQL CONTROL QUERY DEFAULT ROWSET_ROW_COUNT 'ON' ;

  i = SIZE;
  
  Int32 j;
  for(j=1; j < SIZE-1; j++) {
    strcpy(f1[j],"aa	a");
    strcpy(fupdate[j],"bb	b");
    f2[j] = j;	
  }

  test_insert();
  test_update();
  test_select();
  test_delete();

}

void test_insert()
{
  EXEC SQL
    rowset for input size :i
    insert into tpassdp2 values (:f1,:f2);

  if (SQLCODE != 0) {
    printf("Fail: Insert into tpassdp2 table. SQLCODE = %d\n", SQLCODE);
    EXEC SQL ROLLBACK;
    return ;
  }
  EXEC SQL COMMIT;
  printf("Insert passed\n");
  EXEC SQL select count(*) into :count from tpassdp2;
  printf("Number of records after insert: %d\n",count);
  EXEC SQL delete from tpassdp2;
  EXEC SQL COMMIT;
}

void test_update()
{
  EXEC SQL
    rowset for input size :i
    update tpassdp2 set f1= :fupdate
    where f1 = :f1 and f2 = :f2;

  if (SQLCODE != 100) {
    printf("Fail: Update of tpassdp2 table. SQLCODE = %d\n", SQLCODE);
    EXEC SQL ROLLBACK;
    return ;
  }
  else
    displayRowsetRowCountArray();

  EXEC SQL COMMIT;
  printf("Update passed\n");
  EXEC SQL select count(*) into :count from tpassdp2;
  printf("Number of records after update: %d\n",count);
}

void test_select()
{
  EXEC SQL
    rowset for input size :i
    select count(*) into :fcount from tpassdp2
    where f1 = :f1 and f2 = :f2;

  if (SQLCODE != 0) {
    printf("Fail: Select of tpassdp2 table. SQLCODE = %d\n", SQLCODE);
    EXEC SQL ROLLBACK;
    return ;
  }
  printf("Select passed\n");
  EXEC SQL select count(*) into :count from tpassdp2;
  printf("Number of records after select: %d\n",count);
}
void test_delete()
{
  EXEC SQL
    rowset for input size :i
    delete
    from tpassdp2
    where f1 = :f1 and f2 = :f2;

  if (SQLCODE != 100) {
    printf("Fail: Delete from tpassdp2 table. SQLCODE = %d\n", SQLCODE);
    EXEC SQL ROLLBACK;
    return ;
  }
  else
    displayRowsetRowCountArray();

  EXEC SQL COMMIT ;
  printf("Delete passed\n");
  EXEC SQL select count(*) into :count from tpassdp2;
  printf("Number of records after delete: %d\n",count);
}

/*****************************************************/
void displayRowsetRowCountArray()
/*****************************************************/
{
  Int32 rowcountstmtnum = 102;
  Int32 rowsetrowsaffected[SIZE];
  Int32 retcode ;
  Int32 size = SIZE;
  Int32 actual_size = 0;

  

  retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(0, 
					     rowcountstmtnum, 
					     rowsetrowsaffected, 
					     0,
					     size,
					     &actual_size);

   if (retcode == 0) 
   {
     printf("Rowset Row Count Array (only non-zero rows are printed):\n");
     for(Int32 i=0;i<actual_size;i++) {
      if (rowsetrowsaffected[i] != 0)
	printf("Rows affected by row number %d = %d\n",i,rowsetrowsaffected[i]);
      }
     printf("\n");
   }
   else
    printf("SQL_EXEC_GetDiagnosticsStmtInfo2 returned retcode = %d\n", retcode);
}
