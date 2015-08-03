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
/* teste075.sql
 * Melody Xu
 * 08-05-1998
 *
 * embedded C tests for RowSet-derived table
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *     - Test 10 rowset derived tables in SELECT statement
 *
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE075M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET[12] char works_snum[4];
  ROWSET[12] char works_pnum[4];
  ROWSET[12] long works_hours;

  ROWSET[5] char staff_num[4];
  ROWSET[5] char staff_name[21];
  ROWSET[5] VARCHAR staff_city[16];
  ROWSET[5] long staff_grade;

  ROWSET[6] char proj_num[4];
  ROWSET[6] char proj_name[21];
  ROWSET[6] VARCHAR proj_type[7];
  ROWSET[6] long proj_budget;
  ROWSET[6] VARCHAR proj_city[16];

EXEC SQL END DECLARE SECTION;


/* load RSWORKS table into Rowset arrays */
long LoadRSWORKS()
{

  EXEC SQL
    SELECT * INTO :works_snum,:works_pnum,:works_hours
    FROM rsworks;

  if (SQLCODE != 0) {
    printf("Failed to load RSWORKS table. SQLCODE = %ld\n",SQLCODE);
  }
  return(SQLCODE);
}


/* load RSSTAFF into rowset arrays */
long LoadRSSTAFF()
{

  EXEC SQL 
    SELECT * 
      INTO :staff_num,:staff_name,:staff_grade,:staff_city
    FROM RSSTAFF;

  if (SQLCODE != 0) {
    printf("Failed to load RSPROJ table. SQLCODE = %ld\n",SQLCODE);
  }
  return(SQLCODE);
}


/* select all the rows in RSPROJ table into RowSet arrays */
long LoadRSPROJ()
{
  EXEC SQL
    SELECT * into :proj_num,:proj_name,:proj_type,
                  :proj_budget,:proj_city
    from RSPROJ;

  if (SQLCODE != 0) {
    printf("Failed to load RSPROJ table. SQLCODE = %ld\n", SQLCODE);
  }
  return(0);
}

/* 10 rowset derived table in SELECT statement */
long test1()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    char empnum[4],empname[21];
  EXEC SQL END DECLARE SECTION;

  printf("----- Test 1 ------\n");

  EXEC SQL CONTROL QUERY DEFAULT SEMIJOIN_TO_INNERJOIN_TRANSFORMATION 'OFF' ;


  EXEC SQL DECLARE cur_d075t01 CURSOR FOR
    SELECT empnum,empname
     FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
           as d075t01(empnum,empname, grade,city)
     WHERE empnum in
       (SELECT empnum
        FROM ROWSET(:works_snum,:works_pnum,:works_hours)
	   as d075t02(empnum,pnum,hours)
	WHERE pnum IN
	  (SELECT pnum
	   FROM ROWSET(:proj_num,:proj_name,:proj_type,:proj_budget,:proj_city)
	      as d075t03(pnum,pname,ptype,budget,city)
	   WHERE ptype IN
	    (SELECT ptype
	     FROM ROWSET(:proj_num,:proj_name,:proj_type,:proj_budget,:proj_city)
	       as d075t04(pnum,pname,ptype,budget,city)
	     WHERE pnum IN
	      (SELECT pnum
	       FROM ROWSET(:works_snum,:works_pnum,:works_hours)
	         as d075t05(empnum,pnum,hours)
	       WHERE empnum IN
	        (SELECT empnum
		 FROM ROWSET(:works_snum,:works_pnum,:works_hours)
		      as d075t06(empnum,pnum,hours)
		 WHERE pnum IN
		   (SELECT pnum
		    FROM ROWSET(:proj_num,:proj_name,:proj_type,:proj_budget,:proj_city)
		       as d075t07(pnum,pname,ptype,budget,city)
		    WHERE ptype IN
		      (SELECT ptype
		       FROM ROWSET(:proj_num,:proj_name,:proj_type,:proj_budget,:proj_city)
		         as d075t08(pnum,pname,ptype,budget,city)
		       WHERE city IN
		         (SELECT city
			  FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
			     as d075t09(empnum,empname,grade,city)
			  WHERE empnum IN
			    (SELECT empnum
			     FROM ROWSET(:works_snum,:works_pnum,:works_hours)
			        as d075t10(empnum,pnum,hours)
		             WHERE hours = 20
			     AND pnum = 'P2')))))))))
           ORDER BY empnum;
   EXEC SQL OPEN cur_d075t01;
   if (SQLCODE != 0) {
     printf("Failed to open cursor: cur_d075t01. SQLCODE = %ld\n", SQLCODE);
     return(SQLCODE);
   }
   printf("EMPNUM\tEMPNAME\n");
   printf("------\t-------\n");
   while (SQLCODE == 0) {
     EXEC SQL FETCH cur_d075t01 INTO :empnum,:empname;
     if (SQLCODE == 0) {
       empnum[3] = '\0';
       empname[20] = '\0';
       printf("%s\t%s\n", empnum,empname);
       i++;
     }
   }
   EXEC SQL CLOSE cur_d075t01;
   printf("--- %d row(s) selected.\n", i);
   printf("\nExpecting 4 rows.\n");
   return(SQLCODE);
}

int main()
{
  if ((LoadRSWORKS() == 0) && (LoadRSSTAFF() == 0) && (LoadRSPROJ() == 0)) {
    test1();
  }
  return 0;
}
