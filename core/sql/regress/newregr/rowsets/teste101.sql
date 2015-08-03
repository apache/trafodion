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
/* teste101.sql
 * Melody Xu
 * 12-11-1998
 * embedded C tests for direct use of RowSet arrays
 *
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *      - test direct rowset array used in HAVING clause
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE101M NAMES ARE ISO88591;

/* globals */
long SQLCODE;


/* direct use of rowset array used in HAVING clause */
long test1()
{
   int i = 0;
   EXEC SQL BEGIN DECLARE SECTION;
     ROWSET [10] char hv_pnum[4];
     ROWSET [2] long array_budget;
     long row_cnt;
   EXEC SQL END DECLARE SECTION;

   printf("----- Test 1 -----\n");

   /* initialize rowset array array_pnum */
   array_budget[0] = 15000;
   array_budget[1] = 25000;


   /* test direct rowset array in HAVING clause */
   EXEC SQL 
     select pnum  INTO :hv_pnum
     from rsworks
     group by pnum
     having pnum IN 
       (select pnum from rsproj
        group by pnum
	having sum(budget) > :array_budget)
     order by pnum;

   if (SQLCODE != 0) {
     printf("SELECT opertion failed. SQLCODE = %ld\n", SQLCODE);
     return(SQLCODE);
   }

   EXEC SQL get diagnostics :row_cnt = ROW_COUNT;
   if (SQLCODE != 0) {
     printf("GET DIAGNOSTICS failed. SQLCODE = %ld\n", SQLCODE);
     return(SQLCODE);
   }

   printf("PNUM\n");
   printf("----\n");

   for (i=0;i<row_cnt;i++) {
     hv_pnum[i][3] = 0;
     printf("%s\n", hv_pnum[i]);
   }

   printf("--- %d row(s) selected.\n", row_cnt);
   printf("\nExpecting 7 rows selected.\n");
   return(SQLCODE);
}


int main()
{ 
    test1();
    return(0);
}
