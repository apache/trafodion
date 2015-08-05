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
/* teste045.sql
 * Melody Xu
 * 07-29-1998
 *
 * embedded C tests for RowSet-derived table
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *     - Test computed column in ORDER BY on Rowset derived table
 * 
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE045M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET[4] long array_col1;
  ROWSET[4] long array_col2;
  ROWSET[4] long array_col3;
  ROWSET[4] long array_col4;
  ROWSET[4] short array_ind;
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
  return(SQLCODE);
}

//test computed column in ORDER BY
long test1()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    long rsvtable_c1;
    float expr;
  EXEC SQL END DECLARE SECTION;

  printf("----- Test 1 -----\n");

  /* do the actual test */
  EXEC SQL DECLARE cur_d045t01 CURSOR FOR
    SELECT col1, (col3 * col2/col1 - col2 + 10)
    FROM ROWSET(:array_col1,:array_col2,:array_col3,:array_col4 INDICATOR :array_ind)
      as d045t01(col1,col2,col3,col4)
    WHERE col1 > 0
    ORDER BY 2;

  EXEC SQL OPEN cur_d045t01;
  if (SQLCODE != 0) {
    printf("Failed to open cursor: cur_d045t01. SQLCODE = %ld\n",SQLCODE);
    return(SQLCODE);
  }
  printf("COL1  \tEXPR\n");
  printf("------\t------\n");
  while (SQLCODE == 0) {
    EXEC SQL FETCH cur_d045t01 INTO :rsvtable_c1,:expr;
    if (SQLCODE == 0) {
     printf("%ld\t%.2f\n", rsvtable_c1,expr);
     i++;
    }
  }
  EXEC SQL CLOSE cur_d045t01;
  printf("--- %d row(s) selected.\n",i);
  printf("\nExpecting 3 rows with following values:\n");
  printf("(1000,-3990),(10,50),(100,410)\n");
  return(SQLCODE);
}

int main()
{
   if (LoadRSVTABLE() == 0) {
     test1();
   }
   return 0;
}
