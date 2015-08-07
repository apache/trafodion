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
/* teste121.sql
 * Melody Xu
 * 12-14-1998
 * embedded C tests for direct use of RowSet arrays
 *
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *      - ROWSET FOR KEY BY 
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE121M NAMES ARE ISO88591;

/* globals */
long SQLCODE;


/* test ROWSET FOR KEY BY clause */
long test1()
{
   int i = 0;
   EXEC SQL BEGIN DECLARE SECTION;
     ROWSET [40] int inarray_year;
     ROWSET [50] int outarray_idx;
     ROWSET [50] int outarray_count;
     int row_cnt;
   EXEC SQL END DECLARE SECTION;

   printf("----- Test 1 -----\n");

   /* load date into rowset array */
   exec sql
     select year(ts_0) INTO :inarray_year
     from dt
     where cnt <= 40
     order by cnt;

   if (SQLCODE != 0) {
     printf("Failed to load cloumn TS_0 from DT. SQLCODE = %ld\n", SQLCODE);
     return(SQLCODE);
   }

   /* Now do the SELECT operation */
   EXEC SQL ROWSET FOR KEY BY rowidx
     select rowidx, count(*) INTO :outarray_idx, :outarray_count
     from dt
     where year(date1) = :inarray_year AND cnt <= 50
     having count(*) > 0;

   if (SQLCODE != 0) {
     printf("SELECT rowidx failed. SQLCODE = %ld\n", SQLCODE);
     return(SQLCODE);
   }

   EXEC SQL get diagnostics :row_cnt = ROW_COUNT;

   printf("INDX\tCOUNT\n");
   printf("----\t-----\n");

   for (i=0;i<row_cnt;i++) {
     printf("%d\t%d\n", outarray_idx[i], outarray_count[i]);
   }

   printf("--- %d row(s) selected.\n", row_cnt);
   return(SQLCODE);
}


int main()
{ 
  test1();
  return(0);
}
