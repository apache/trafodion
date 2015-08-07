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
/* teste105.sql
 * Melody Xu
 * 12-14-1998
 * embedded C tests for direct use of RowSet arrays
 *
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *      - direct use of rowset arrays in SELECT...INTO clause with string
 *        trancation
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE105M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

/* direct use of rowset array in SELECT...INTO with string transaction */
long test1()
{
   int i = 0;
   EXEC SQL BEGIN DECLARE SECTION;
     ROWSET [5] char array_name[4];
     ROWSET [5] short array_ind;
     long row_cnt;
   EXEC SQL END DECLARE SECTION;

   printf("----- Test 1 -----\n");

   /* test direct rowset array with string trancation*/
   EXEC SQL 
     select empname INTO :array_name:array_ind
     from rsstaff
     order by empnum;

   if (SQLCODE != 0) {
     printf("SELECT opertion failed. SQLCODE = %ld\n", SQLCODE);
   }

   EXEC SQL get diagnostics :row_cnt = ROW_COUNT;
   if (SQLCODE != 0) {
     printf("GET DIAGNOSTICS failed. SQLCODE = %ld\n", SQLCODE);
     return(SQLCODE);
   }

   printf("EMPNAME\tINDICATOR\n");
   printf("-------\t---------\n");

   for (i=0;i<row_cnt;i++) {
     array_name[i][3] = 0;
     printf("%s\t%d\n", array_name[i], array_ind[i]);
   }

   printf("--- %d row(s) selected.\n", row_cnt);
   printf("\nExpecting 5 rows selected.\n");
   return(SQLCODE);
}


int main()
{ 
    test1();
    return(0);
}
