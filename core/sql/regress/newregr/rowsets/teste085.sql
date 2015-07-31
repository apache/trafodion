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
/* teste085.sql
 * Melody Xu
 * 07-23-1998
 * embedded C tests for RowSet-derived table
 *
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *      - Test SELECT [first N] on rowset derived table 
 *      - Test SELECT [first N] with direct use of rowset
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

EXEC SQL MODULE CAT.SCH.TESTE085M NAMES ARE ISO88591;

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
  int a, hv_cnt, row_cnt;
  ROWSET [100] int arrayA;
  ROWSET [50] int array_idx;
  int hv_idx;
EXEC SQL END DECLARE SECTION;


void initArray()
{
  int i;
  for (i=0;i<100;i++)
    arrayA[i] = i%10;
}

/* select all the rows in RSPROJ table into RowSet arrays */
long LoadRSPROJ()
{
  EXEC SQL
    SELECT pnum,pname,ptype,city into
        :proj_num,:proj_name,:proj_type,:proj_city
    from RSPROJ;

  if (SQLCODE != 0) {
    printf("Failed to load RSPROJ table. SQLCODE = %ld\n", SQLCODE);
  }
  return(SQLCODE);
}


/* load RSSTAFF into rowset arrays */
long LoadRSSTAFF()
{

  EXEC SQL
    SELECT * into :staff_num,:staff_name,:staff_grade,:staff_city
    FROM RSSTAFF;

  if (SQLCODE != 0) {
    printf("Failed to load RSWORKS table. SQLCODE = %ld\n",SQLCODE);
  }
  return(SQLCODE);
}

long LoadDynamic3()
{
    exec sql begin work;
    exec sql delete from dynamic3;
    for (hv_cnt=1;hv_cnt<=40;hv_cnt++)
      exec sql insert into dynamic3 values (:hv_cnt);
    return(SQLCODE);
}

//test select [frist 20]
long test1()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    char empnum[4], empname[21],pname[21];
    VARCHAR s_city[16],p_city[16];
    long grade;
  EXEC SQL END DECLARE SECTION;

  printf("----- Test 1 -----\n");

  EXEC SQL DECLARE cur_d085t01 CURSOR FOR
    SELECT [first 5] empnum,empname,grade,d085t01.city,pname,d085t02.city
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
      as d085t01(empnum, empname, grade,city),
      ROWSET(:proj_num,:proj_name,:proj_type,:proj_city)
        as d085t02(pnum,pname,ptype,city)
    WHERE d085t01.city = d085t02.city
    order by empname, pname;

  EXEC SQL OPEN cur_d085t01;
  if (SQLCODE != 0) {
    printf("Failed to open cursor: cur_d085t01. SQLCODE = %ld\n",SQLCODE);
    return(SQLCODE);
  }
  printf("EMPNUM\tEMPNAME\tGRADE\tCITY1\tPNAME\tCITY2\n");
  printf("------\t-------\t-----\t-----\t-----\t-----\n");
  while (SQLCODE == 0) {
    EXEC SQL FETCH cur_d085t01 INTO :empnum,:empname,:grade,:s_city, :pname,:p_city;
    if (SQLCODE == 0) {
      empnum[3] = '\0';
      empname[20] = '\0';
      pname[20] = '\0';
      printf("%s\t%s\t%d\t%s\t%s\t%s\n",empnum,empname,grade,s_city,pname,p_city);
      i++;
    }
  }
  EXEC SQL CLOSE cur_d085t01;
  printf("--- %d row(s) selected.\n",i);
  printf("\nExpceting 5 rows are selected.\n");
  return(SQLCODE);
}

//test select [first 5]
long test2()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    char empnum[4], empname[21],pname[21];
    VARCHAR s_city[16],p_city[16];
    long grade;
  EXEC SQL END DECLARE SECTION;

  printf("\n----- Test 2 -----\n");

  EXEC SQL DECLARE cur_d085t02 CURSOR FOR
    SELECT [first 5] empnum,empname,grade,d085t03.city,pname,d085t04.city
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
      as d085t03(empnum, empname, grade,city),
      ROWSET(:proj_num,:proj_name,:proj_type,:proj_city)
        as d085t04(pnum,pname,ptype,city)
    WHERE d085t03.city = d085t04.city
    order by empname, pname;

  EXEC SQL OPEN cur_d085t02;
  if (SQLCODE != 0) {
    printf("Failed to open cursor: cur_d085t02. SQLCODE = %ld\n",SQLCODE);
    return(SQLCODE);
  }
  printf("EMPNUM\tEMPNAME\tGRADE\tCITY1\tPNAME\tCITY2\n");
  printf("------\t-------\t-----\t-----\t-----\t-----\n");
  while (SQLCODE == 0) {
    EXEC SQL FETCH cur_d085t02 INTO :empnum,:empname,:grade,:s_city, :pname,:p_city;
    if (SQLCODE == 0) {
      empnum[3] = '\0';
      empname[20] = '\0';
      pname[20] = '\0';
      printf("%s\t%s\t%d\t%s\t%s\t%s\n",empnum,empname,grade,s_city,pname,p_city);
      i++;
    }
  }
  EXEC SQL CLOSE cur_d085t02;
  printf("--- %d row(s) selected.\n",i);
  printf("\nExpceting 5 rows are selected.\n");
  return(SQLCODE);
}

//test select [first 1]
long test3()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    char empnum[4], empname[21],pname[21];
    VARCHAR s_city[16],p_city[16];
    long grade;
  EXEC SQL END DECLARE SECTION;

  printf("\n----- Test 3 -----\n");

  EXEC SQL
    SELECT [first 1] empnum,empname,grade,d085t05.city,pname,d085t06.city
      INTO :empnum,:empname,:grade,:s_city, :pname,:p_city
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
      as d085t05(empnum, empname, grade,city),
      ROWSET(:proj_num,:proj_name,:proj_type,:proj_city)
        as d085t06(pnum,pname,ptype,city)
    WHERE d085t05.city = d085t06.city
    order by empname, pname;

  if (SQLCODE != 0) {
    printf("SELECT operation failed.SQLCODE = %ld\n",SQLCODE);
    return(SQLCODE);
  }
  i++;
  printf("EMPNUM\tEMPNAME\tGRADE\tCITY1\tPNAME\tCITY2\n");
  printf("------\t-------\t-----\t-----\t-----\t-----\n");
      empnum[3] = '\0';
      empname[20] = '\0';
      pname[20] = '\0';
      printf("%s\t%s\t%d\t%s\t%s\t%s\n",empnum,empname,grade,s_city,pname,p_city);
  printf("--- %d row(s) selected.\n",i);
  printf("\nExpceting 1 row is selected.\n");
  return(SQLCODE);
}

//test select [first 0]
long test4()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    char empnum[4], empname[21],pname[21];
    VARCHAR s_city[16],p_city[16];
    long grade = 0;
    long row_cnt = 0;
  EXEC SQL END DECLARE SECTION;

  printf("\n----- Test 4 -----\n");

  strcpy(s_city, "");
  strcpy(p_city, "");
  empnum[0] = 0;
  empname[0] = 0;
  pname[0] = 0;
  EXEC SQL
    SELECT [first 0] empnum,empname,grade,d085t07.city,pname,d085t08.city
      INTO :empnum,:empname,:grade,:s_city, :pname,:p_city
    FROM ROWSET(:staff_num,:staff_name,:staff_grade,:staff_city)
      as d085t07(empnum, empname, grade,city),
      ROWSET(:proj_num,:proj_name,:proj_type,:proj_city)
        as d085t08(pnum,pname,ptype,city)
    WHERE d085t07.city = d085t08.city
    order by empname, pname;

    printf("SQLCODE = %ld\n", SQLCODE);
    printf("\nExpecting SQLCODE=100\n");

    exec sql get diagnostics :row_cnt = ROW_COUNT;
    printf("The number of rows returned: %d\n", row_cnt);
    printf("\nExpecting row_cnt = 0\n");
    empnum[3] = 0;
    empname[20] = 0;
    pname[20] = 0;
    printf("empnum=%s\tempname=%s\tgrade=%d\ts_city=%s\tp_city=%s\n",
            empnum,empname,grade,s_city,p_city);
  return(SQLCODE);
}

//test select [first 20] on single rowset derived table
long test5()
{
  int i = 0;
  EXEC SQL BEGIN DECLARE SECTION;
    long hv1;
  EXEC SQL END DECLARE SECTION;

  printf("\n----- Test 5 -----\n");

  EXEC SQL DECLARE cur_d085t05 CURSOR FOR
    SELECT [first 20] a
    FROM ROWSET(:arrayA) as d085t09(a)
    order by a;

  EXEC SQL OPEN cur_d085t05;
  if (SQLCODE != 0) {
    printf("Failed to open cursor: cur_d085t01. SQLCODE = %ld\n",SQLCODE);
    return(SQLCODE);
  }
  printf("A\n");
  printf("----\n");
  i = 0;
  while (SQLCODE == 0) {
    EXEC SQL FETCH cur_d085t05 INTO :hv1;
    if (SQLCODE == 0) {
      printf("%d\n",hv1);
      i++;
    }
  }
  EXEC SQL CLOSE cur_d085t05;
  printf("--- %d row(s) selected.\n",i);
  printf("\nExpceting 20 rows are selected with 10 0s and 10 1s.\n");
  return(SQLCODE);
}

//test select [first 1] on single rowset derived table
long test6()
{
  EXEC SQL BEGIN DECLARE SECTION;
    long hv1;
  EXEC SQL END DECLARE SECTION;

  printf("\n----- Test 6 -----\n");

  EXEC SQL
    SELECT [first 1] a INTO :hv1
    FROM ROWSET(:arrayA) as d085t10(a)
    order by a;

  if (SQLCODE != 0) {
    printf("SELECT operation failed. SQLCODE = %ld\n",SQLCODE);
    return(SQLCODE);
  }
  printf("A\n");
  printf("----\n");
  printf("%d\n",hv1);
  printf("\nExpceting 1 rows is selected with value 0.\n");
  return(SQLCODE);
}

//test select [first 0] on single rowset derived table
long test7()
{
  EXEC SQL BEGIN DECLARE SECTION;
    long hv1 = -1;
    long row_cnt;
  EXEC SQL END DECLARE SECTION;

  printf("\n----- Test 7 -----\n");

  EXEC SQL
    SELECT [first 0] a INTO :hv1
    FROM ROWSET(:arrayA) as d085t10(a)
    order by a;

  printf("SQLCODE = %ld\n",SQLCODE);
  printf("\nExpceting SQLCODE=100\n");

  exec sql get diagnostics :row_cnt = ROW_COUNT;
  printf("Number of rows selected: %d\n", row_cnt);
  printf("\nExpecting 0 row selected.\n");
  printf("HV1=%d\n", hv1);
  return(SQLCODE);
}

/* test select first N into rowset array */
long test8()
{
   int i = 0;

   printf("\n----- Test 8 -----\n");

   EXEC SQL
     select [first 10] a INTO :array_idx
     from dynamic3
     where a <= 30
     order by a;

   if (SQLCODE != 0) {
     printf("SELECT opeartion failed. SQLCODE = %ld\n", SQLCODE);
     return(SQLCODE);
   }

   EXEC SQL get diagnostics :row_cnt = ROW_COUNT;

   printf("Number of rows selected: %ld\n\n", row_cnt);

   printf("COUNT\n");
   printf("-----\n");

   /* print result */
   for (i=0;i< row_cnt;i++) {
     printf("%ld\n", array_idx[i]);
   }
   return(SQLCODE);
}

long test9()
{
   int i = 0;

   printf("\n----- Test 9 -----\n");

   EXEC SQL
     select [first 10] a INTO :array_idx
     from dynamic3
     where a = 1;

   if (SQLCODE != 0) {
     printf("SELECT opeartion failed. SQLCODE = %ld\n", SQLCODE);
     return(SQLCODE);
   }

   EXEC SQL get diagnostics :row_cnt = ROW_COUNT;

   printf("Number of rows selected: %ld\n\n", row_cnt);


   /* print result */
   for (i=0;i< row_cnt;i++) {
     printf("a = %ld\n", array_idx[i]);
   }
   return(SQLCODE);
}

long test10()
{
   int i = 0;

   printf("\n----- Test 10 -----\n");

   EXEC SQL BEGIN

     delete from dynamic3 where 1 = 0;

     select [first 10] a INTO :array_idx
     from dynamic3
     where a = 2;

   END;

   if (SQLCODE != 0) {
     printf("SELECT opeartion failed. SQLCODE = %ld\n", SQLCODE);
     return(SQLCODE);
   }

   EXEC SQL get diagnostics :row_cnt = ROW_COUNT;

   printf("Number of rows selected: %ld\n\n", row_cnt);

   /* print result */
   for (i=0;i< row_cnt;i++) {
     printf("a = %ld\n", array_idx[i]);
   }

   return(SQLCODE);
}

long test11()
{
   int i = 0;

   printf("\n----- Test 11 -----\n");

   EXEC SQL BEGIN

     delete from dynamic3 where 1 = 0;

     select [first 1] a INTO :hv_idx
     from dynamic3
     where a = 3;

   END;

   if (SQLCODE != 0) {
     printf("SELECT opeartion failed. SQLCODE = %ld\n", SQLCODE);
     return(SQLCODE);
   }

   

   /* print result */
   printf("a = %ld\n", hv_idx);

   return(SQLCODE);
}

int main()
{
  if ((LoadRSSTAFF() == 0) && (LoadRSPROJ() == 0) && (LoadDynamic3() == 0)) {
        initArray();
	test1();
	test2();
	test3();
	test4();
	test5();
	test6();
	test7();
	test8();
	test9();
	test10();
	test11();
  }
  return 0;
}
