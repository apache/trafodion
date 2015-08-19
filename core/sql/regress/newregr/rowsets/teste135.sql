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
/* teste135.sql
 * Melody Xu
 * 04-02-1999
 * embedded C tests for direct use of RowSet arrays
 *
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *      - rowset array is used in set clause for UPDATE statement
 *	- all the rowset arrays have same size
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE135M NAMES ARE ISO88591;

/* globals */
Int32 SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [6] char array_pnum[4];
  ROWSET [6] Int32 array_budget;
EXEC SQL END DECLARE SECTION;

Int32 init_rowsets()
{
  Int32 i;
  /* initialize rowset array data */
  EXEC SQL
    select pnum, budget/1000 INTO :array_pnum, :array_budget
    FROM rsproj
    ORDER BY pnum;

  if (SQLCODE != 0) {
     printf("Failed to select data from table RSPROJ\n");
  }
  else {
    for (i=0;i<6;i++) {
      array_pnum[i][3] = 0;
    }
  }
  return(SQLCODE);
}

/*****************************************************/
void displayRowsetRowCountArray()
/*****************************************************/
{
  Int32 rowcountstmtnum = 102;
  Int32 rowsetrowsaffected[100];
  Int32 retcode ;
  Int32 size = 100;
  Int32 actual_size = 0;

  EXEC SQL CONTROL QUERY DEFAULT ROWSET_ROW_COUNT 'ON' ;

  retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(0, 
					     rowcountstmtnum, 
					     rowsetrowsaffected, 
					     0,
					     size,
					     &actual_size);

   if (retcode == 0) 
   {
     printf("Rowset Row Count Array:\n");
     for(Int32 i=0;i<actual_size;i++)
      printf("Rows affected by row number %d = %d\n",i,rowsetrowsaffected[i]);
     printf("\n");
   }
   else
    printf("SQL_EXEC_GetDiagnosticsStmtInfo2 returned retcode = %d\n", retcode);
}

/* direct use of rowset arrays in UPDATE statement with WHERE clause */
Int32 test1()
{
   Int32 i = 0;
   EXEC SQL BEGIN DECLARE SECTION;
     ROWSET [12] char hv_pnum[4];
     ROWSET [12] Int32 hv_hours;
     Int32 row_cnt;
   EXEC SQL END DECLARE SECTION;

   printf("----- Test 1 -----\n");

   exec sql
     update rsworks
     set hours = hours + :array_budget
     where pnum = :array_pnum;

   if (SQLCODE != 0) {
     printf("UPDATE operation failed. SQLCODE = %d\n", SQLCODE);
     return(SQLCODE);
   }

   exec sql get diagnostics :row_cnt = ROW_COUNT;

   printf("Number of rows updated: %d\n\n", row_cnt);

   printf("\nExpecting 12 rows updated\n\n");

   displayRowsetRowCountArray();

   /* verify the UPDATE operation result */
   exec sql
     select pnum, hours INTO :hv_pnum, :hv_hours
     from rsworks
     order by pnum, hours;

   if (SQLCODE != 0) {
     printf("SELECT operation failed on RSWORKS table. SQLCODE = %d\n", SQLCODE);
     return(SQLCODE);
   }
   printf("PNUM\tHOURS\n");
   printf("----\t-----\n");
   for (i=0;i<12;i++) {
     hv_pnum[i][3] = 0;
     printf("%s\t%d\n", hv_pnum[i], hv_hours[i]);
   }

   /* expected result */
   printf("\nExpecting %d rows with following values pairs:\n", row_cnt);
   printf("('P1',50),('P1',50),('P2',50),('P2',50),('P2',50),('P2',110),\n");
   printf("('P3',110),('P4',40),('P4',60),('P5',22),('P5',90),('P6',62)\n");
   return(SQLCODE);
}


Int32 main()
{
  if (init_rowsets() == 0) {
   test1();
  }
  return(0);
}
