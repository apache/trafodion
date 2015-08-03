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
/* teste061.sql
 * Melody Xu
 * 07-30-1998
 *
 * embedded C tests for RowSet-derived table
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *     - Test DISTINCT with GROUP BY, HAVING
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE061M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET[6] char proj_num[4];
  ROWSET[6] char proj_name[21];
  ROWSET[6] int proj_budget;
  ROWSET[6] VARCHAR proj_type[7];
  ROWSET[6] VARCHAR proj_city[16];
EXEC SQL END DECLARE SECTION;


/* select all the rows in RSPROJ table into RowSet arrays */
long LoadRSPROJ()
{
  int i;
  EXEC SQL
    SELECT * into 
      :proj_num,:proj_name,:proj_type,:proj_budget,:proj_city
    from RSPROJ;


  if (SQLCODE != 0) {
    printf("Failed to load RSPROJ table. SQLCODE = %ld\n", SQLCODE);
  }
  else 
    for (i=0;i<6;i++) {
      proj_num[i][3]='\0';
      proj_name[i][20]='\0';
    }
  return(SQLCODE);
}

//test GROUP BY, HAVING
long test1()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    VARCHAR ptype[7], city[16];
  EXEC SQL END DECLARE SECTION;
  printf("----- Test 1 -----\n");

  EXEC SQL DECLARE cur_d061t01 CURSOR FOR
    SELECT ptype,city
    FROM ROWSET(:proj_num,:proj_name,:proj_type,:proj_budget,:proj_city)
         as d061t01(pnum,pname,ptype,budget,city)
    GROUP BY ptype,city
    HAVING avg(budget) > 21000
    ORDER BY ptype;

    EXEC SQL OPEN cur_d061t01;
    if (SQLCODE != 0) {
      printf("Failed to open cursor: cur_d061t01. SQLCODE = %ld\n",SQLCODE);
      return(SQLCODE);
     }
     printf("PTYPE  \tCITY\n");
     printf("------ \t----------\n");
     while (SQLCODE == 0) {
       EXEC SQL FETCH cur_d061t01 INTO :ptype,:city;
       if (SQLCODE == 0) {
	  printf("%s\t%s\n", ptype,city);
          i++;
       }
     }
     EXEC SQL CLOSE cur_d061t01;
     printf("--- %d row(s) selected.\n", i);
     printf("\nExpecting 3 rows:\n");
     printf("Code/Vienna, Design/Deale, Test/Tampa\n");
   return(0);
}

//test DISTINCT GROUP BY, HAVING
long test2()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    VARCHAR ptype[7], city[16];
  EXEC SQL END DECLARE SECTION;
  printf("\n----- Test 2 -----\n");

  EXEC SQL DECLARE cur_d061t02 CURSOR FOR
    SELECT DISTINCT ptype,city
    FROM ROWSET(:proj_num,:proj_name,:proj_type,:proj_budget,:proj_city)
         as d061t02(pnum,pname,ptype,budget,city)
    GROUP BY ptype,city
    HAVING avg(budget) > 21000
    ORDER BY ptype;

    EXEC SQL OPEN cur_d061t02;
    if (SQLCODE != 0) {
      printf("Failed to open cursor: cur_d061t02. SQLCODE = %ld\n",SQLCODE);
      return(SQLCODE);
     }
     printf("PTYPE  \tCITY\n");
     printf("------ \t----------\n");
     while (SQLCODE == 0) {
       EXEC SQL FETCH cur_d061t02 INTO :ptype,:city;
       if (SQLCODE == 0) {
	  printf("%s\t%s\n", ptype,city);
          i++;
       }
     }
     EXEC SQL CLOSE cur_d061t02;
     printf("--- %d row(s) selected.\n", i);
     printf("\nExpecting 3 rows:\n");
     printf("Code/Vienna, Design/Deale, Test/Tampa\n");
   return(0);
}

//test DISTINCT GROUP BY, HAVING
long test3()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    long total_budget;
  EXEC SQL END DECLARE SECTION;
  printf("\n----- Test 3 -----\n");

  EXEC SQL DECLARE cur_d061t03 CURSOR FOR
    SELECT DISTINCT sum(budget)
    FROM ROWSET(:proj_num,:proj_name,:proj_type,:proj_budget,:proj_city)
         as d061t03(pnum,pname,ptype,budget,city)
    GROUP BY ptype,city
    HAVING avg(budget) > 21000
    ORDER BY 1;

    EXEC SQL OPEN cur_d061t03;
    if (SQLCODE != 0) {
      printf("Failed to open cursor: cur_d061t03. SQLCODE = %ld\n",SQLCODE);
      return(SQLCODE);
     }
     printf("SUM(BUDGET)\n");
     printf("-----------\n");
     while (SQLCODE == 0) {
       EXEC SQL FETCH cur_d061t03 INTO :total_budget;
       if (SQLCODE == 0) {
	  printf("%d\n", total_budget);
          i++;
       }
     }
     EXEC SQL CLOSE cur_d061t03;
     printf("--- %d row(s) selected.\n", i);
     printf("\nExpecting 2 rows:\n");
     printf("SUM(BUDGET) values: 30000 and 80000\n");
   return(0);
}

int main()
{
   if (LoadRSPROJ() == 0) {
     test1();
     test2();
     test3();
   }
   return 0;
}
