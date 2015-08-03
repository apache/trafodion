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
/* teste015.sql
 * Melody Xu
 * 07-20-1998
 * embedded C tests for RowSet-derived table
 *
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage: 
 *      Test SOME and ANY quantifier on RowSet derived table
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE015M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  /* rowset arrays for RSSTAFF table */
  ROWSET [5] char staff_num[4];
  ROWSET [5] char staff_name[21];
  ROWSET [5] long staff_grade;
  ROWSET [5] VARCHAR staff_city[16];

  /* rowset arrays for RSPROJ table */
  ROWSET [6] char proj_num[4];
  ROWSET [6] char proj_name[21];
  ROWSET [6] VARCHAR proj_type[7];
  ROWSET [6] long proj_budget;
  ROWSET [6] VARCHAR proj_city[16];
EXEC SQL END DECLARE SECTION;


/* load RSSTAFF into rowset arrays */
long LoadRSSTAFF()
{
  int i;
  EXEC SQL
    SELECT * into :staff_num,:staff_name,:staff_grade,:staff_city
    FROM RSSTAFF;

  if (SQLCODE != 0) {
    printf("Failed to load RSSTAFF table. SQLCODE = %ld\n",SQLCODE);
  }
  else {
    for (i=0;i<5;i++) {
      staff_num[i][3] = 0;
      staff_name[i][20] = 0;
    }
  }
  return(SQLCODE);
}

/* load RSPROJ table into RowSet arrays */
long LoadRSPROJ() 
{

  int i;
  EXEC SQL
    SELECT * into
      :proj_num,:proj_name,:proj_type,:proj_budget,:proj_city
    from RSPROJ;

  if (SQLCODE != 0) {
    printf("Failed to load RSPROJ table. SQLCODE = %ld\n",SQLCODE);
  }
  else {
    for (i=0;i<6;i++) {
      proj_num[i][3] = 0;
      proj_name[i][20] = 0;
    }
  }
  return(SQLCODE);
}

//test SOME
long test1()
{
  EXEC SQL BEGIN DECLARE SECTION;
    char empname[21];
  EXEC SQL END DECLARE SECTION;
  printf("----- Test 1 -----\n");

  EXEC SQL 
    SELECT empname INTO :empname
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
      as d014t01(empnum, empname, grade, city)
    WHERE grade < SOME
      (SELECT p_budget/1000 - 39
       FROM ROWSET(:proj_num,:proj_name,:proj_type,:proj_budget,:proj_city)
        as d014t02(p_num, p_name, p_type, p_budget, p_city)
       WHERE  p_city = 'Deale');
    
  if (SQLCODE != 0) {
     printf("the SELECT operation failed. SQLCODE = %ld\n",SQLCODE);
  }
  else {
     empname[20] = '\0';
     printf("EMPNAME = %s\n",empname);
  }
  printf("\nExpecting EMPNAME = 'Betty'\n");
  return(SQLCODE);
}

// test ANY quantifier
long test2() {
  EXEC SQL BEGIN DECLARE SECTION;
    char empname[21];
  EXEC SQL END DECLARE SECTION;
  printf("\n----- Test 2 -----\n");

  EXEC SQL 
    SELECT empname INTO :empname
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
      as d014t03(empnum, empname, grade, city)
    WHERE grade < ANY
      (SELECT p_budget/1000 - 39
       FROM ROWSET(:proj_num,:proj_name,:proj_type,:proj_budget,:proj_city)
        as d014t04(p_num, p_name, p_type, p_budget, p_city)
       WHERE  p_city = 'Deale');
    
  if (SQLCODE != 0) {
     printf("the SELECT operation failed. SQLCODE = %ld\n",SQLCODE);
  }
  else {
     empname[20] = '\0';
     printf("EMPNAME = %s\n", empname);
  }
  printf("\nExpecting EMPNAME = 'Betty'\n");
  return(SQLCODE);
}

int main()
{
  if ((LoadRSSTAFF() == 0) && (LoadRSPROJ() == 0)) {
	test1();
	test2();
  }
  return 0;
}
