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
/* teste021.sql
 * Melody Xu
 * 07-23-1998
 * embedded C tests for RowSet-derived table
 *
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *       Test RowSet derived table in subquery with MAX in < comparison 
 *       predicate
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE021M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [5] char staff_num[4];
  ROWSET [5] char staff_name[21];
  ROWSET [5] long staff_grade;

EXEC SQL END DECLARE SECTION;


/* load RSSTAFF into rowset arrays */
long LoadRSSTAFF()
{

  EXEC SQL
    SELECT empnum,empname,grade into :staff_num,:staff_name,:staff_grade
    FROM RSSTAFF;

  if (SQLCODE != 0) {
    printf("Failed to load RSSTAFF table. SQLCODE = %ld\n",SQLCODE);
  }
  return(SQLCODE);
}

//test rowset derived table used in subquery with MAX in < comparasion predicate
long test1()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    char empnum[4];
  EXEC SQL END DECLARE SECTION;

  printf("----- Test 1 -----\n");

  EXEC SQL DECLARE cur_d021t01 CURSOR FOR
    SELECT empnum
    FROM ROWSET(:staff_num,:staff_name,:staff_grade)
      as d021t01(empnum, empname, grade)
    WHERE grade <
      (SELECT max(d021t02.grade)
        FROM ROWSET(:staff_num,:staff_name,:staff_grade)
	  as d021t02(empnum, empname,grade))
    ORDER BY empnum;


  EXEC SQL OPEN cur_d021t01;
  if (SQLCODE != 0) {
    printf("Failed to open cursor: cur_d021t01. SQLCODE = %ld\n",SQLCODE);
    return(SQLCODE);
  }
  printf("EMPNUM\n");
  printf("------\n");
  while (SQLCODE == 0) {
    EXEC SQL FETCH cur_d021t01 INTO :empnum;
    if (SQLCODE == 0) {
      empnum[3] = '\0';
      printf("%s\n",empnum);
      i++;
    }
  }
  EXEC SQL CLOSE cur_d021t01;
  printf("--- %d row(s) selected.\n",i);
  printf("\nExpecting 3 rows with EMPNUMs: 'E1','E2','E4'\n");
  return(SQLCODE);
}

int main()
{
  if (LoadRSSTAFF() == 0) {
	test1();
  }
  return 0;
}
