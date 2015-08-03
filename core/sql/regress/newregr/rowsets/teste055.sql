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
/* teste055.sql
 * Melody Xu
 * 07-30-1998
 *
 * embedded C tests for RowSet-derived table
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
       - Test outer join on Rowset derived table
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE055M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET[12] char works_snum[4];
  ROWSET[12] char works_pnum[4];
  ROWSET[12] long works_hours;

  ROWSET[6] char staff_num[4];
  ROWSET[6] char staff_name[21];
  ROWSET[6] long staff_grade;
  ROWSET[6] VARCHAR staff_city[16];
EXEC SQL END DECLARE SECTION;


//load RSWORKS table into Rowset arrays
long LoadRSWORKS()
{

  EXEC SQL
    SELECT * into :works_snum,:works_pnum,:works_hours
    FROM rsworks;

  if (SQLCODE != 0) {
    printf("Failed to load RSWORKS table. SQLCODE = \n",SQLCODE);
  }
  return(SQLCODE);
}

//load RSSTAFF into rowset arrays
long LoadRSSTAFF()
{

  EXEC SQL
    SELECT * into :staff_num,:staff_name,:staff_grade,:staff_city
    FROM RSSTAFF;

  if (SQLCODE != 0) {
    printf("Failed to load RSWORKS table. SQLCODE = \n",SQLCODE);
  }

  /* adding 1 more element into rowset arrays */
  strcpy(staff_num[5],"E6 ");
  memset(staff_name[5],' ',20);
  strncpy(staff_name[5],"Lendle", strlen("Lendle"));
  staff_name[5][20] = '\0';
  staff_grade[5] = 17;
  strcpy(staff_city[5],"Potomac");

  return(SQLCODE);
}

//test outer join
long test1()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    char empnum[4],empname[21];
    VARCHAR expr1[3];
    int expr2;
  EXEC SQL END DECLARE SECTION;

  printf("----- Test 1 -----\n");

  /* do the actual test */
    EXEC SQL DECLARE cur_d055t01 CURSOR FOR
      SELECT 'ZZ',empnum,empname,-99
      FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
	     as d055t01(empnum, empname, grade, city)
      WHERE NOT EXISTS (SELECT * 
      			FROM ROWSET(:works_snum,:works_pnum,:works_hours)
			  as d055t02(empnum,pnum,hours)
			WHERE d055t02.empnum = d055t01.empnum)
      ORDER BY empnum;

    EXEC SQL OPEN cur_d055t01;
    if (SQLCODE != 0) {
      printf("Failed to open cursor: cur_d055t01. SQLCODE = \n",SQLCODE);
      return(SQLCODE);
     }
     printf("EXPR1\tEMPNUM\tEMPNAME\tEXPR2\n");
     printf("-----\t------\t-------\t-----\n");
     while (SQLCODE == 0) {
       EXEC SQL FETCH cur_d055t01 INTO :expr1,:empnum,:empname,:expr2;
       if (SQLCODE == 0) {
          empnum[3] = '\0';
	  empname[20] = '\0';
	  printf("%s\t%s\t%s\t%d\n",expr1,empnum,empname,expr2);
          i++;
       }
     }
     EXEC SQL CLOSE cur_d055t01;
     printf("--- %d row(s) selected\n", i);
     printf("\nExpecting 2 rows with following values:\n");
     printf("(ZZ,E5,Ed,-99),(ZZ,E6,Lendle,-99)\n");
   return(0);
}

int main()
{
   if ((LoadRSWORKS() == 0) && (LoadRSSTAFF() == 0)) {
     test1();
   }
   return 0;
}
