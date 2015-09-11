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
// +++ Code modified on 2003/5/6
**********************************************************************/
/* teste001.sql
 * Melody Xu
 * 07-01-1998
 * embedded C tests for RowSet-derived table
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *        Test COUNT function on rowset derived table.
 * 	    - test COUNT ALL
 * 	    - test COUNT DISTINCT
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>


EXEC SQL MODULE CAT.SCH.TESTE001M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [12] char works_snum[4];
  ROWSET [12] char works_pnum[4];
  ROWSET [12] long works_hours;
EXEC SQL END DECLARE SECTION;

/* load RSWORKS table into Rowset arrays */
long LoadRSWORKS()
{
  int i=0;

  EXEC SQL BEGIN DECLARE SECTION;
    char w_snum[4], w_pnum[4];
    long w_hours;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL DECLARE cur_rsworks CURSOR FOR
    SELECT * FROM rsworks;

  EXEC SQL OPEN cur_rsworks;
  if (SQLCODE != 0) {
    printf("Failed to open cursor: cur_rsworks. SQLCODE = %ld\n",SQLCODE);
    return(SQLCODE);
  }
  while (SQLCODE == 0) {
    EXEC SQL FETCH cur_rsworks INTO :w_snum,:w_pnum,:w_hours;
    if (SQLCODE == 0) {
      w_snum[3] = '\0';
      w_pnum[3] = '\0';
      strcpy(works_snum[i], w_snum);
      strcpy(works_pnum[i], w_pnum);
      works_hours[i] = w_hours;
      i++;
    }
  }
  EXEC SQL CLOSE cur_rsworks;
  return(SQLCODE);
}

void check_result(long expected, long actual)
{
  if (SQLCODE == 0) {
    printf("count = %ld\n", actual);
    printf("\nExpecting count = %ld\n", expected);
    if (expected == actual) {
      printf("Test case PASSED\n");
    }
    else {
      printf("Test case FAILED\n");
    }
  }
  else {
    printf("SELECT operation failed. SQLCODE = %ld\n", SQLCODE);
    printf("Test FAILED");
  }
}

//test count(*)
long test1()
{
  EXEC SQL BEGIN DECLARE SECTION;
    int count;
  EXEC SQL END DECLARE SECTION;
  printf("----- Test 1 -----\n");

  // select count(*) on char type column
  EXEC SQL 
    SELECT count(*) INTO :count 
    FROM ROWSET(:works_snum, :works_pnum) as d001t01(empnum, pnum)
    WHERE pnum = 'P2';
 
  check_result(4, count);
  printf("--------------------\n");
  //select count(*) on exec numeric data type column
  EXEC SQL
    SELECT count(*) INTO :count
    FROM ROWSET(:works_snum,:works_pnum,:works_hours)
           as d001t01(empnum,pnum,hours)
    WHERE hours=40;

  check_result(3, count);

  return(SQLCODE);
}

//test count(distinct)
long test2()
{
  EXEC SQL BEGIN DECLARE SECTION;
    int cnt;
  EXEC SQL END DECLARE SECTION;
 
  printf("\n----- Test 2 -----\n");

  //select count(distinct) on int data type column
  EXEC SQL
    SELECT COUNT(DISTINCT hours) INTO :cnt
    FROM ROWSET(:works_snum, :works_pnum, :works_hours)
    	as d001t02(empnum,pnum,hours);

  check_result(4, cnt);

  printf("--------------------\n");
  cnt = 0;
  //select count(distinct) on char data type 
  EXEC SQL
    SELECT COUNT(DISTINCT empnum) INTO :cnt
    FROM ROWSET(:works_snum,:works_pnum,:works_hours)
    	 as d001t03(empnum,pnum,hours);
 
  check_result(4, cnt);
  return(SQLCODE);
}

int main()
{
  if (LoadRSWORKS() == 0) {
	test1();
	test2();
  }
  return 0;
}
