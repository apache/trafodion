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
/* teste005.sql
 * Melody Xu
 * 07-16-1998
 * embedded C tests for RowSet-derived table
 * 
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage: 
 *       Test SELECT ALL, SELECT DISTINCT
 * 
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#if !defined(__TANDEM) || (__CPLUSPLUS_VERSION >= 3)
using namespace std;
#endif
#include <string.h>


EXEC SQL MODULE CAT.SCH.TESTE005M NAMES ARE ISO88591;

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
  int i;
  EXEC SQL
    SELECT * into :works_snum,:works_pnum,:works_hours
    FROM rsworks;

  if (SQLCODE != 0) {
    printf("Failed to load RSWORKS table. SQLCODE = %ld\n",SQLCODE);
  }
  else {
    for (i=0; i<12; i++) {
      works_snum[i][3] = 0;
      works_pnum[i][3] = 0;
    }
 }
  return(SQLCODE);
}

//test SELECT ALL
long test1()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    char empnum[4];
  EXEC SQL END DECLARE SECTION;
  cout << "----- Test 1 (SELECT ALL ) -----" << endl;

  EXEC SQL DECLARE cur_d005t01 CURSOR FOR
    SELECT ALL empnum
    FROM ROWSET(:works_snum, :works_pnum, :works_hours) 
    	as d005t01(empnum, pnum, hours)
    WHERE hours = 12;
 

  EXEC SQL OPEN cur_d005t01;
  
  if (SQLCODE != 0) {
	printf("Failed to open cursor: cur_d005t01. SQLCODE = %ld\n",SQLCODE);
	return(SQLCODE);
  }

  printf("EMPNUM\n");
  printf("------\n");
  while (SQLCODE == 0) {
    EXEC SQL FETCH cur_d005t01 INTO :empnum;
    if (SQLCODE == 0) {
	empnum[3] = '\0';
        printf("%s\n", empnum);
	i++;
    }
  }
  EXEC SQL CLOSE cur_d005t01;
  printf("--- %d row(s) selected.\n", i);
  printf("\nExpecting 2 rows are selected and both EMPNUMs are 'E1'\n");
  return(0);
}

/* test SELECT DISTINCT */
long test2() {
  EXEC SQL BEGIN DECLARE SECTION;
    char empnum[4];
  EXEC SQL END DECLARE SECTION;
  printf("\n----- Test 2 (SELECT DISTINCT) -----\n");

  EXEC SQL 
    SELECT DISTINCT empnum INTO :empnum
    FROM ROWSET(:works_snum, :works_pnum, :works_hours)
    	as d005t02(empnum, pnum, hours)
    WHERE hours = 12;
 
    if (SQLCODE == 0) {
	empnum[3] = '\0';
        printf("EMPNUM = %s\n", empnum);
    }
    else {
       printf("The SELECT DISTINCT opeartion failed. SQLCODE = %ld\n", SQLCODE);
    }
  printf("\nExpecting 1 row selected and EMPNUM = 'E1'\n");
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
