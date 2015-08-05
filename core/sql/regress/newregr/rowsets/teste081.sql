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
/* teste081.sql
 * Melody Xu
 * 07-23-1998
 * embedded C tests for RowSet-derived table
 *
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *      - index indetifier in SELECT list
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE081M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [6] char proj_num[4];
  ROWSET [6] char proj_name[21];
  ROWSET [6] VARCHAR proj_type[7];
  ROWSET [6] VARCHAR proj_city[16];

  ROWSET [5] char staff_num[4];
  ROWSET [5] char staff_name[21];
  ROWSET [5] int staff_grade;
  ROWSET [5] VARCHAR staff_city[16];
EXEC SQL END DECLARE SECTION;

EXEC SQL CONTROL QUERY DEFAULT PCODE_OPT_LEVEL 'OFF' ;

/* select all the rows in RSPROJ table into RowSet arrays */
long LoadRSPROJ()
{
  int i;
  EXEC SQL
    SELECT pnum,pname,ptype,city into
           :proj_num,:proj_name,:proj_type,:proj_city
    from RSPROJ
    order by pnum;

  if (SQLCODE != 0) {
    printf("Failed to load RSPROJ table. SQLCODE = %ld\n", SQLCODE);
  }
  printf("PROJ_NUM\n");
  printf("--------\n");
  for (i=0;i<6;i++) {
    printf("%s\n",proj_num[i]);
  }
  printf("\n");
  return(SQLCODE);
}


/* load RSSTAFF into rowset arrays */
long LoadRSSTAFF()
{
  int i;
  EXEC SQL
    SELECT * into :staff_num,:staff_name,:staff_grade,:staff_city
    FROM RSSTAFF
    order by empnum;

  if (SQLCODE != 0) {
    printf("Failed to load RSWORKS table. SQLCODE = %ld\n",SQLCODE);
  }
  printf("STAFF_NUM\n");
  printf("---------\n");
  for (i=0;i<5;i++)
    printf("%s\n",staff_num[i]);
  printf("\n");
  return(SQLCODE);
}

//test index identifier used in KEY BY 
long test1()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    char empnum[4], empname[21],pname[21];
    VARCHAR s_city[16];
    int idx1,idx2;
  EXEC SQL END DECLARE SECTION;

  printf("----- Test 1 -----\n");

  EXEC SQL DECLARE cur_d081t01 CURSOR FOR
    SELECT rowid1,empnum,empname,d081t01.city,rowid2,pname
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city) KEY BY rowid1
      as d081t01(empnum, empname, grade,city, rowid1),
      ROWSET(:proj_num,:proj_name,:proj_type,:proj_city) KEY BY rowid2
        as d081t02(pnum,pname,ptype,city, rowid2)
    WHERE d081t01.city = d081t02.city
    ORDER BY rowid1, rowid2;

  EXEC SQL OPEN cur_d081t01;
  if (SQLCODE != 0) {
    printf("Failed to open cursor: cur_d081t01. SQLCODE = %ld\n",SQLCODE);
    return(SQLCODE);
  }
  printf("ROWID1\tEMPNUM\tEMPNAME\tCITY1\tROWID2\tPNAME\n");
  printf("------\t------\t-------\t-----\t------\t-----\n");
  while (SQLCODE == 0) {
    EXEC SQL FETCH cur_d081t01 
       INTO :idx1,:empnum,:empname,:s_city, :idx2,:pname;
    if (SQLCODE == 0) {
      empnum[3] = '\0';
      empname[20] = '\0';
      pname[20] = '\0';
      printf("%ld\t%s\t%s\t%s\t%ld\t%s\n",idx1,empnum,empname,s_city,idx2,pname);
      i++;
    }
  }
  EXEC SQL CLOSE cur_d081t01;
  printf("--- %d row(s) selected.\n",i);
  printf("\nExpecting 10 rows are selected.\n");
  return(SQLCODE);
}

int main()
{
  if ((LoadRSSTAFF() == 0) && (LoadRSPROJ() == 0)) {
	test1();
  }
  return 0;
}
