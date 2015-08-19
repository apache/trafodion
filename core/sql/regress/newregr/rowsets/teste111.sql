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
/* teste111.sql
 * Melody Xu
 * 12-14-1998
 * embedded C tests for direct use of RowSet arrays
 *
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *      - direct use of rowset arrays in WHERE clause of UPDATE statement
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE111M NAMES ARE ISO88591;

/* globals */
Int32 SQLCODE;

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
     ROWSET [5] char array_pnum[4];
     ROWSET [12] char hv_pnum[4];
     ROWSET [12] Int32 hv_hours;
     Int32 row_cnt;
   EXEC SQL END DECLARE SECTION;

   printf("----- Test 1 -----\n");

   /* load date into rowset array */
   exec sql
     select pnum INTO :array_pnum
     from rsproj
     where pnum <> 'P6'
     order by pnum;

   if (SQLCODE != 0) {
     printf("Failed to load data from RSPROJ. SQLCODE = %d\n", SQLCODE);
     return(SQLCODE);
   }

   for (i=0;i<5;i++)
     array_pnum[i][3] = 0;

   exec sql
     update rsworks
     set hours = (select max(budget)/1000 from rsproj)
     where pnum = :array_pnum;

   if (SQLCODE != 0) {
     printf("UPDATE operation failed. SQLCODE = %d\n", SQLCODE);
     return(SQLCODE);
   }

   exec sql get diagnostics :row_cnt = ROW_COUNT;

   printf("Number of rows updated: %d\n\n", row_cnt);

   printf("\nExpecting 11 rows updated\n\n");

   displayRowsetRowCountArray();

   exec sql
     select  pnum, hours INTO :hv_pnum, :hv_hours
     from rsworks
     order by pnum;

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

   return(SQLCODE);
}


Int32 main()
{ 
  test1();
  return(0);
}
