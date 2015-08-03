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
/* teste041.sql
 * Melody Xu
 * 07-29-1998
 *
 * embedded C tests for RowSet-derived table
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *     - Test SOME, ANY in HAVING clause. 
 *     - Rowset null indicator
 *
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE041M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET[6] long array_col1;
  ROWSET[6] long array_col2;
  ROWSET[6] long array_col3;
  ROWSET[6] long array_col4;
  ROWSET[6] short array_ind;

  ROWSET[5] char staff_num[4];
  ROWSET[5] long staff_grade;

  ROWSET[12] char works_snum[4];
  ROWSET[12] char works_pnum[4];
  ROWSET[12] long works_hours;
EXEC SQL END DECLARE SECTION;


// select all the rows in RSVTABLE table into RowSet arrays
long LoadRSVTABLE()
{

  EXEC SQL
    SELECT col1,col2,col3,col4 into 
           :array_col1,:array_col2,:array_col3,:array_col4:array_ind
    FROM rsvtable;

  if (SQLCODE != 0) {
    printf("Failed to load RSVTABLE table. SQLCODE = %ld\n",SQLCODE);
  }

  /* adding 2 more element into rowset array */
  array_col1[4] = 10;
  array_col2[4] = 11;
  array_col3[4] = 12;
  array_col4[4] = 13;
  array_ind[4] = 0;

  array_col1[5] = 100;
  array_col2[5] = 111;
  array_col3[5] = 1112;
  array_col4[5] = 113;
  array_ind[5] = 0;

  return(SQLCODE);
}

//load RSSTAFF into rowset arrays
long LoadRSSTAFF() {

  EXEC SQL
    SELECT empnum,grade into :staff_num,:staff_grade
    FROM RSSTAFF;

  if (SQLCODE != 0) {
    printf("Failed to load RSSTAFF table. SQLCODE = %ld\n",SQLCODE);
  }
  return(SQLCODE);
}

//load RSWORKS table into rowset arrays
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

//test SOME, ANY in HAVING clause
long test1()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    long max_value, rsvtable_col1;
  EXEC SQL END DECLARE SECTION;
  printf("----- Test 1 -----\n");

  /* do the actual test */
  EXEC SQL DECLARE cur_d041t01 CURSOR FOR
    SELECT col1, MAX(col2)
    FROM ROWSET(:array_col1,:array_col2,:array_col3,:array_col4 INDICATOR :array_ind)
      as d041t01(col1,col2,col3,col4)
    GROUP BY col1
    HAVING max(col2) > ANY (SELECT grade
                            FROM ROWSET(:staff_num,:staff_grade)
			      as d041t02(empnum,grade)
			   )
       AND max(col2) < SOME (SELECT hours
                             FROM ROWSET(:works_snum,:works_pnum,:works_hours)
	              		   as d041t03(empnum,pnum,hours)
			     )
    ORDER BY col1;

  EXEC SQL OPEN cur_d041t01;
  if (SQLCODE != 0) {
    printf("Failed to open cursor: cur_d041t01. SQLCODE = %ld\n",SQLCODE);
    return(SQLCODE);
  }
  printf("COL1  \tMAX COL2\n");
  printf("------\t--------\n");
  while (SQLCODE == 0) {
    EXEC SQL FETCH cur_d041t01 INTO :rsvtable_col1,:max_value;
    if (SQLCODE == 0) {
     printf("%ld\t%ld\n",rsvtable_col1,max_value);
     i++;
    }
  }
  EXEC SQL CLOSE cur_d041t01;
  printf("--- %d row(s) selected\n",i);
  printf("\nExpecting COL1= 10 and MAX(COL2) = 20\n");
  return(SQLCODE);
}

int main()
{
   if ( (LoadRSSTAFF() == 0) && (LoadRSWORKS() == 0) && (LoadRSVTABLE() == 0)) {
     test1();
   }
   return 0;
}
