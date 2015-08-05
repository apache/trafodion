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
/* teste025.sql
 * Melody Xu
 * 07-01-1998
 *
 * embedded C tests for RowSet-derived table
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *      Test nested NOT EXISTS with correlated subquery and DISTINCT 
 *      using RowSet derived tables
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE025M NAMES ARE ISO88591;

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

  EXEC SQL 
    SELECT * INTO :works_snum, :works_pnum, :works_hours
    FROM rsworks;

  if (SQLCODE != 0) {
    printf("SELECT operation failed.. SQLCODE = %ld\n",SQLCODE);
  }
  return(SQLCODE);
}

//test NOT EXISTS with correlated subquery
long test1()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    char empnum[4];
  EXEC SQL END DECLARE SECTION;

  printf("----- Test 1 -----\n");

  EXEC SQL DECLARE cur_d025t01 CURSOR FOR
    SELECT DISTINCT empnum
      FROM ROWSET(:works_snum,:works_pnum,:works_hours)
        as d025t01(empnum,pnum,hours)
      WHERE NOT EXISTS
        (SELECT *
	 FROM ROWSET(:works_snum,:works_pnum,:works_hours)
	   as d025t02(empnum,pnum,hours)
	 WHERE d025t02.empnum = 'E2'
	 AND NOT EXISTS
	   (SELECT *
	    FROM ROWSET(:works_snum,:works_pnum,:works_hours)
	      as d025t03(empnum,pnum,hours)
	      WHERE d025t03.empnum = d025t01.empnum
	      AND d025t03.pnum = d025t02.pnum))
      ORDER by empnum;

  EXEC SQL OPEN cur_d025t01;
  if (SQLCODE != 0) {
    printf("Failed to open cursor: cur_d025t01. SQLCODE = %ld\n",SQLCODE);
    return(SQLCODE);
  }
  printf("EMPNUM\n");
  printf("------\n");
  while (SQLCODE == 0) {
    EXEC SQL FETCH cur_d025t01 INTO :empnum;
    if (SQLCODE == 0) {
      empnum[3] = '\0';
      printf("%s\n",empnum);
      i++;
    }
  }
  EXEC SQL CLOSE cur_d025t01;
  printf("--- %d row(s) selected.\n",i);
  printf("\nExpecting 2 rows with EMPNUMs: 'E1','E2'\n");
  return(SQLCODE);
}

int main()
{
  if (LoadRSWORKS() == 0) {
	test1();
  }
  return 0;
}
