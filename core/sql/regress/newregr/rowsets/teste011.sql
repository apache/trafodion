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
/* teste011.sql
 * Melody Xu
 * 07-20-1998
 * embedded C tests for RowSet-derived table
 * 
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *       Test NOT BETWEEN predicate on RowSet derived table.
 * 
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>


EXEC SQL MODULE CAT.SCH.teste011M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [5] char staff_num[4];
  ROWSET [5] char staff_name[21];
  ROWSET [5] long staff_grade;
  ROWSET [5] VARCHAR staff_city[16];
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

//NOT BETWEEN predicate on the derived table
long test1()
{
  EXEC SQL BEGIN DECLARE SECTION;
    VARCHAR city[16];
  EXEC SQL END DECLARE SECTION;
  printf("----- Test 1 -----\n");

  EXEC SQL SELECT city INTO :city
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
           as d011t01(empnum, empname, grade, city)
    WHERE grade NOT BETWEEN 12 and 13;

  if (SQLCODE != 0) {
     printf("the SELECT operation failed. SQLCODE = %ld\n",SQLCODE);
  }
  else {
     printf("CITY = %s\n",city);
  }
  printf("\nExpecting CITY = 'Vienna'\n");
  return(SQLCODE);
}

/* test NOT BETWEEN */
long test2()
{
  EXEC SQL BEGIN DECLARE SECTION;
    VARCHAR city[16];
  EXEC SQL END DECLARE SECTION;
  printf("\n----- Test 2 -----\n");

  EXEC SQL SELECT city INTO :city
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
           as d010t02(empnum, empname, grade, city)
    WHERE NOT(grade BETWEEN 12 and 13);

  if (SQLCODE != 0) {
     printf("the SELECT operation failed. SQLCODE = %ld\n",SQLCODE);
  }
  else
    printf("CITY = %s\n", city);
  printf("\nExpecting CITY = 'Vienna'\n");
  return(SQLCODE);
}

int main()
{
  if (LoadRSSTAFF() == 0) {
	test1();
	test2();
  }
  return 0;
}
