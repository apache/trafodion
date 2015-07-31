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
/* teste070.sql
 * Melody Xu
 * 08-12-1998
 *
 * embedded C tests for RowSet-derived table
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *     - Test relaxed union compatability rules for columns
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE070M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET[12] char works_snum[4];
  ROWSET[12] char works_pnum[4];
  ROWSET[12] long works_hours;

  ROWSET[5] char staff_num[4];
  ROWSET[5] char staff_name[21];
  ROWSET[5] long staff_grade;
  ROWSET[5] VARCHAR staff_city[16];

  ROWSET[6] char proj_num[4];
  ROWSET[6] char proj_name[21];
  ROWSET[6] VARCHAR proj_type[7];
  ROWSET[6] VARCHAR proj_city[16];
EXEC SQL END DECLARE SECTION;


/* load RSWORKS table into Rowset arrays */
long LoadRSWORKS()
{

  EXEC SQL 
    SELECT * into :works_snum,:works_pnum,:works_hours
    FROM rsworks;

  if (SQLCODE != 0) {
    printf("Failed to load RSWORKS table. SQLCODE = %ld\n",SQLCODE);
  }
  return(SQLCODE);
}


/* load RSSTAFF into rowset arrays */
long LoadRSSTAFF()
{

  EXEC SQL
    SELECT * into :staff_num,:staff_name,:staff_grade,:staff_city
    FROM RSSTAFF;

  if (SQLCODE != 0) {
    printf("Failed to load RSSTAFF table. SQLCODE = %ld\n",SQLCODE);
  }
  return(SQLCODE);
}

/* select all the rows in RSPROJ table into RowSet arrays */
long LoadRSPROJ()
{

  EXEC SQL
    SELECT pnum,pname,ptype,city into 
           :proj_num,:proj_name,:proj_type,:proj_city
    from RSPROJ;

  if (SQLCODE != 0) {
    printf("Failed to load RSPROJ table. SQLCODE = %ld\n", SQLCODE);
  }
  return(SQLCODE);
}


/* test UNION char type with varchar type */
long test1()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    VARCHAR empnum[7];
    VARCHAR city[16];
  EXEC SQL END DECLARE SECTION;

  printf("----- Test 1 ------\n");

  EXEC SQL DECLARE cur_d071t01 CURSOR FOR
    SELECT empnum,city 
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
      as d070t01(empnum,empname,grade,city)
    UNION
    SELECT ptype,city
     FROM ROWSET(:proj_num,:proj_name,:proj_type,:proj_city)
       as d071t02(pnum,pname,ptype,city)
    ORDER BY empnum,city;

  EXEC SQL OPEN cur_d071t01;
  if (SQLCODE != 0) {
    printf("Failed to open cursor: cur_d071t01. SQLCODE = %ld\n", SQLCODE);
    return(SQLCODE);
  }
  printf("EMPNUM\tCITY\n");
  printf("------\t----------\n");
  while (SQLCODE == 0) {
    EXEC SQL FETCH cur_d071t01 INTO :empnum,:city;
    if (SQLCODE == 0) {
      printf("%s\t%s\n", empnum,city);
      i++;
    }
  }
  EXEC SQL CLOSE cur_d071t01;
  printf("--- %d row(s) selected.\n", i);
  printf("\nExpecting 9 rows are selected.\n");
   return(SQLCODE);
}

/* test UNION of char column with char literal */
long test2()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    char empnum[4];
    VARCHAR city[16];
  EXEC SQL END DECLARE SECTION;

  printf("\n----- Test 2 -----\n");
  EXEC SQL DECLARE cur_d071t03 CURSOR FOR
    SELECT empnum,city
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
          as d070t03(empnum,empname,grade,city)
    UNION
     SELECT 'e1 ', city
     FROM ROWSET(:proj_num,:proj_name,:proj_city)
       as d071t04(pnum,pname,city)
    ORDER BY empnum,city;

   EXEC SQL OPEN cur_d071t03;
   if (SQLCODE != 0) {
     printf("Failed to open cursor: cur_d071t03. SQLCODE = %ld\n", SQLCODE);
     return(SQLCODE);
   }
   printf("EMPNUM\tCITY\n");
   printf("------\t---------\n");
   while (SQLCODE == 0) {
     EXEC SQL FETCH cur_d071t03 INTO :empnum,:city;
     if (SQLCODE == 0) {
       empnum[3] = '\0';
       printf("%s\t%s\n", empnum,city);
       i++;
     }
   }
   EXEC SQL CLOSE cur_d071t03;
   printf("--- %d row(s) selected.\n",i);
   printf("\nExpecting 8 rows are selected.\n");
  return(SQLCODE);
}

int main()
{
  if ((LoadRSWORKS() == 0) && (LoadRSPROJ() == 0) && (LoadRSSTAFF() == 0)) {
    test1();
    test2();
  }
  return 0;
}
