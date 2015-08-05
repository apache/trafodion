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
/* teste051.sql
 * Melody Xu
 * 07-29-1998
 *
 * embedded C tests for RowSet-derived table
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *     - Test AVG, MIN on joined Rowset derived table with WHERE without GROUP
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE051M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET[12] char works_snum[4];
  ROWSET[12] char works_pnum[4];
  ROWSET[12] int works_hours;

  ROWSET[5] char staff_num[4];
  ROWSET[5] char staff_name[21];
  ROWSET[5] int staff_grade;
  ROWSET[5] VARCHAR staff_city[16];
EXEC SQL END DECLARE SECTION;


//load RSWORKS table into Rowset arrays
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

//load RSSTAFF into rowset arrays
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

//test AVG, MIN on joined table with WHERE without GROUP
long test1()
{
  EXEC SQL BEGIN DECLARE SECTION;
    long min_hour;
    float avg_hour;
  EXEC SQL END DECLARE SECTION;

  printf("----- Test 1 -----\n");

  /* do the actual test */
  EXEC SQL 
    SELECT avg(hours), min(hours) INTO :avg_hour,:min_hour
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
	     as d051t02(empnum, empname, grade, city),
         ROWSET(:works_snum,:works_pnum,:works_hours) 
             as d051t01(empnum, pnum, hours)
    WHERE d051t02.empnum = 'E2' AND
          d051t02.empnum = d051t01.empnum;

  if (SQLCODE != 0) {
    printf("The SELECT opereation failed. SQLCODE = %ld\n",SQLCODE);
  }
  else {
    printf("AVG(HOURS) = %.5f\tMIN(HOURS) = %d\n",avg_hour,min_hour);
  }
  printf("\nExpecting AVG(HOURS) = 60, MIN(HOURS) = 40\n");
  return(SQLCODE);
}

int main()
{
   if ((LoadRSWORKS() == 0) && (LoadRSSTAFF() == 0)) {
     test1();
   }
   return 0;
}
