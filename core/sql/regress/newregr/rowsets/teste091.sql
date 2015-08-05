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
// +++ Code modified on 2003/5/6
**********************************************************************/
/* teste091.sql
 * Melody Xu
 * 12-10-1998
 * embedded C tests for direct use of RowSet arrays
 *
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *      - direct use of Rowset arrays in FETCH...INTO clause
 *      - different rowset size to test the smallest size is used.
 *	- forcing the query plan using pack/unpack node and without using
 *	- pack/unpack node
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE091M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [5] char works_snum[4];
  ROWSET [10] char works_pnum[4];
  ROWSET [12] long works_hours;

  ROWSET [2] char CQS_snum[4];
  ROWSET [2] char CQS_pnum[4];
  ROWSET [2] long CQS_hours;
  long row_cnt;
EXEC SQL END DECLARE SECTION;


long test1()
{
   int i, total=0;
   exec sql declare cur_d091t01 cursor for
     select * from rsworks
     order by empnum,pnum;

   exec sql open cur_d091t01;
   if (SQLCODE != 0) {
     printf("Failed to open cursor: cur_d091t01. SQLCODE = %ld\n", SQLCODE);
     return(SQLCODE);
   }

   printf("EMPNUM\tPNUM\tHOURS\n");
   printf("------\t----\t-----\n");

   while (SQLCODE == 0) {
     exec sql fetch cur_d091t01 INTO :works_snum,:works_pnum, :works_hours;

     if (SQLCODE == 0) {
       exec sql get diagnostics :row_cnt = ROW_COUNT;

       if (SQLCODE != 0) {
         printf("GET DIAGNOSTICS operation failed. SQLCODE = %ld\n", SQLCODE);
         return(SQLCODE);
       }

       for (i=0;i<row_cnt;i++) {
         works_snum[i][3] = 0;
         works_pnum[i][3] = 0;
         printf("%s\t%s\t%ld\n", works_snum[i], works_pnum[i], works_hours[i]);
       }
       printf("---------%d rows---------------\n",row_cnt);
       total = total + row_cnt;
     }
   }
   exec sql close cur_d091t01;
   printf("--- %d row(s) selected.\n", total);
   printf("\nExpecting 12 rows selected\n");
   return(SQLCODE);
}

/* this test forces the CQS for insert and select to use rowsets with 
 * pack/unpack node in CQS
 */
long CQSwithPack()
{
  /* prepare to insert some dummy values into works table
   * Using ZZ and 9999 will help to delete these values after
   * the insertion and so as to maintain the same number of records
   * in the table as earlier
   */
  for (int i=0; i<2; i++) /* 2 is the ROWSET SIZE */
  {
      strcpy(CQS_snum[i], "yy");
      strcpy(CQS_pnum[i], "zz");
      CQS_hours[i] = 9999;
  }

  EXEC SQL
    control query shape nested_join(unpack(tuple),partition_access(insert));

  EXEC SQL 
  	INSERT INTO rsworks values (:CQS_snum, :CQS_pnum, :CQS_hours);

  if (SQLCODE != 0) {
    printf("Failed to insert. SQLCODE = %ld\n",SQLCODE);
  }
  else {
    row_cnt=0;
    exec sql get diagnostics :row_cnt = ROW_COUNT;
    printf("Insert succeeded. Inserted Rows = %ld\n", row_cnt);
  }

  EXEC SQL
    control query shape pack(partition_access(scan(path 'rsworks', mdam off)));
  EXEC SQL SELECT empnum, pnum, hours 
  	INTO :CQS_snum, :CQS_pnum, :CQS_hours 
	FROM  rsworks 
	WHERE hours = 9999;
  if (SQLCODE != 0) {
    printf("Failed to select. SQLCODE = %ld\n",SQLCODE);
  }
  else {
    row_cnt=0;
    exec sql get diagnostics :row_cnt = ROW_COUNT;
    printf("Select succeeded. Selected Rows = %ld\n",row_cnt);
  }
  /* Delete the records inserted */

  EXEC SQL control query shape off;
  EXEC SQL
     	DELETE from rsworks where hours = 9999;
  if (SQLCODE != 0) {
    printf("Failed to delete. SQLCODE = %ld\n",SQLCODE);
  }
  else {
    row_cnt=0;
    exec sql get diagnostics :row_cnt = ROW_COUNT;
    printf("Delete succeeded. Deleted Rows = %ld\n",row_cnt);
  }
  return SQLCODE;
}

/* this test forces the CQS for insert and select to use rowsets with out
 * pack/unpack node in CQS
 */
long CQSwithoutPack()
{
  /* prepare to insert some dummy values into works table
   * Using ZZ and 9999 will help to delete these values after
   * the insertion and so as to maintain the same number of records
   * in the table as earlier
   */
  for (int i=0; i<2; i++) /* 2 is the ROWSET SIZE */
  {
      strcpy(CQS_snum[i], "yy");
      strcpy(CQS_pnum[i], "zz");
      CQS_hours[i] = 9999;
  }

  EXEC SQL
    control query shape nested_join(tuple, partition_access(insert));

  EXEC SQL 
  	INSERT INTO rsworks values (:CQS_snum, :CQS_pnum, :CQS_hours);

  if (SQLCODE != 0) {
    printf("Failed to insert. SQLCODE = %ld\n",SQLCODE);
  }
  else {
    row_cnt=0;
    exec sql get diagnostics :row_cnt = ROW_COUNT;
    printf("Insert succeeded. Inserted Rows = %ld\n", row_cnt);
  }

  EXEC SQL
    control query shape partition_access(scan(path 'rsworks', mdam off));
  EXEC SQL SELECT empnum, pnum, hours 
  	INTO :CQS_snum, :CQS_pnum, :CQS_hours 
	FROM  rsworks 
	WHERE hours = 9999;
  if (SQLCODE != 0) {
    printf("Failed to select. SQLCODE = %ld\n",SQLCODE);
  }
  else {
    row_cnt=0;
    exec sql get diagnostics :row_cnt = ROW_COUNT;
    printf("Select succeeded. Selected Rows = %ld\n",row_cnt);
  }
  /* Delete the records inserted */

  EXEC SQL control query shape off;
  EXEC SQL
     	DELETE from rsworks where hours = 9999;
  if (SQLCODE != 0) {
    printf("Failed to delete. SQLCODE = %ld\n",SQLCODE);
  }
  else {
    row_cnt=0;
    exec sql get diagnostics :row_cnt = ROW_COUNT;
    printf("Delete succeeded. Deleted Rows = %ld\n",row_cnt);
  }
  return SQLCODE;
}

int main()
{
    test1();
    CQSwithPack();
    CQSwithoutPack();
    return(0);
}
