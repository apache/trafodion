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
/* teste065.sql
 * Melody Xu
 * 08-04-1998
 *
 * embedded C tests for RowSet-derived table
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *     - Test outer reference directly contained in HAVING clause
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE065M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET[6] char proj_num[4];
  ROWSET[6] char proj_name[21];
  ROWSET[6] VARCHAR proj_type[7];
  ROWSET[6] long proj_budget;
  ROWSET[6] VARCHAR proj_city[16];

  ROWSET[5] char staff_num[4];
  ROWSET[5] char staff_name[21];
  ROWSET[5] long staff_grade;
  ROWSET[5] VARCHAR staff_city[16];
EXEC SQL END DECLARE SECTION;


/* select all the rows in RSPROJ table into RowSet arrays */
long LoadRSPROJ()
{

  EXEC SQL
    SELECT * into 
             :proj_num,:proj_name,:proj_type,:proj_budget,:proj_city
    from RSPROJ;

  if (SQLCODE != 0) {
    printf("Failed to load RSPROJ table. SQLCODE = %ld\n", SQLCODE);
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

/* test outer reference directly contained in HAVING clause */
long test1()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    char emp_num[4];
    long expr_grade;
  EXEC SQL END DECLARE SECTION;
  printf("----- Test 1 -----\n");

  EXEC SQL
    SELECT empnum, grade*1000 INTO :emp_num, :expr_grade
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
         as d065t01(empnum,empname,grade,city)
    WHERE grade * 1000 > ANY
      (SELECT sum(budget)
       FROM ROWSET(:proj_num,:proj_name,:proj_type,:proj_budget,:proj_city)
         as d065t02(pnum,pname,ptype,budget,city)
       GROUP BY city,ptype
       HAVING d065t02.city = d065t01.city);

    if (SQLCODE != 0) {
      printf("The SELECT operation failed. SQLCODE = %ld\n", SQLCODE);
     }
    else {
     emp_num[3] = '\0';
     printf("EMPNUM = %s\tEXPR = %ld\n", emp_num,expr_grade);
    }
   printf("\nExpecting EMPNUM = 'E3' and GRADE * 1000 = 13000\n");
   return(SQLCODE);
}


int main()
{
   if ((LoadRSPROJ() == 0) && (LoadRSSTAFF() == 0)) {
     test1();
   }
   return 0;
}
