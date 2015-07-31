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
// +++ Code modified on 2003/11/22
**********************************************************************/
/* teste140.sql
 * Suresh Subbiah 
 * 07-28-2003
 * embedded C tests for setting rownumber in diags area
 *
 *   Test Classification: Positive and Negative
 *   Test Level: Functional
 *   Test Coverage:
 *      - Unique constraint violation on rowset insert into table
 *      - Unique constraint violation on rowset insert into index table
 *	- Unique constraint violation on rowset insert into partitioned table
 *      - Unique constraint violation on rowset update (with index maintainence)
 *      - Unique constraint violation for rowset + CS
 *      - Divide by zero error in rowset where clause for select, delete and update
 *      - Negative tests for derived rowsets
 *   This test uses a direct CLI call to test value of rownumber in diags area as
 *   there is no support in preprocessor to get this value currently.
 */

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include "sqlcli.h"

void display_diagnosis();

EXEC SQL MODULE CAT.SCH.TESTE140M NAMES ARE ISO88591;

exec sql whenever sqlerror call display_diagnosis;

exec sql control query default upd_savepoint_on_error 'OFF';

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [10] Int32 a_int;
  ROWSET [10] Int32 b_int;
  ROWSET [10] Int32 c_int;
  ROWSET [10] Int32 d_int;
  ROWSET [1000] Int32 e_int;
  ROWSET [1000] Int32 f_int;
  ROWSET [10000] Int32 g_int;
  ROWSET [10000] Int32 h_int;

EXEC SQL END DECLARE SECTION;

EXEC SQL BEGIN DECLARE SECTION;  
/**** host variables for get diagnostics *****/
   NUMERIC(5) i;
   NUMERIC(5) hv_num;
   Int32 hv_sqlcode;
   Int32 hv_rownum;
   char hv_msgtxt[513];
   char SQLSTATE[6];
   Int32 SQLCODE;
   Int32 savesqlcode;
EXEC SQL END DECLARE SECTION;


/*****************************************************/
void display_diagnosis()
/*****************************************************/
{

  savesqlcode = SQLCODE ;
  hv_rownum = -2;
  Int32 rowcondnum = 103;
  Int32 retcode;


  exec sql get diagnostics :hv_num = NUMBER;

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[512]='\0';

   printf("Number  : %d\n", hv_num);

  for (i = 1; i <= hv_num; i++) {
      exec sql get diagnostics exception :i
          :hv_sqlcode = SQLCODE,
          :hv_msgtxt = MESSAGE_TEXT;

   printf("Sqlcode : %d\n", hv_sqlcode);
   printf("Message : %s\n", hv_msgtxt);

   retcode = SQL_EXEC_GetDiagnosticsCondInfo2(rowcondnum, i, &hv_rownum, 0,0,0);
   printf("RowNum  : %d\n", hv_rownum);

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[512]='\0';
  }

   SQLCODE = savesqlcode ;
}


/* 8102 while inserting into base table, on primary key, no indices defined */
void test1() {
printf("\n\ntest1 : Expecting rownumber = 9999\n");

Int32 i=0;
for (i=0; i<10000; i++) {
    g_int[i] = i;
    h_int[i] = i;
}

    g_int[9999] = 7;  /* causes unique constraint error */
    h_int[5] = 3;


  EXEC SQL DELETE FROM rownum1;
  EXEC SQL
  INSERT INTO rownum1 VALUES (:g_int, :h_int) ;

  if (SQLCODE != 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Insert succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* 8102 while inserting into base table, on primary key, 1 index defined. 
   Tests raising error on first (rownum=0) array entry */

void test2() {
printf("\n\ntest2 : Expecting rownumber = 0\n");

Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i;
    b_int[i] = i;
}

  EXEC SQL DELETE FROM rownum2;
  EXEC SQL INSERT INTO rownum2 VALUES (0,0);  /* causes unique constraint error */
 
  EXEC SQL
  INSERT INTO rownum2 VALUES (:a_int, :b_int) ;

  if (SQLCODE != 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Insert succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* 8102 while inserting into index table,  1 index defined due to unique constraint on b. */

void test3() {
printf("\n\ntest3 : Expecting rownumber = 5\n");

Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i;
    b_int[i] = i;
}

    b_int[5] = 3;  /* causes unique constraint error on index */


  EXEC SQL DELETE FROM rownum3;
  EXEC SQL
  INSERT INTO rownum3 VALUES (:a_int, :b_int) ;

  if (SQLCODE != 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Insert succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* 8102 while inserting into partitioned table. */
void test4() {
printf("\n\ntest4 : Expecting rownumber = 498\n");

Int32 i=0;
for (i=0; i<1000; i++) {
    e_int[i] = i;
    f_int[i] = i;
}

    e_int[498] = 7;  /* causes unique constraint error */
    f_int[502] = 501;  


  EXEC SQL DELETE FROM rownum4;
  EXEC SQL
  INSERT INTO rownum4 VALUES (:e_int, :f_int) ;

  if (SQLCODE != 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Insert succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}


/* 8102 while updating index table, 1 index defined due to unique constraint on b. */

void test5() {
printf("\n\ntest5 : Expecting rownumber = 8\n");

Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i;
    b_int[i] = i;
}

   b_int[8] = 7;  /* causes unique constraint error*/

  EXEC SQL DELETE FROM rownum3;
  EXEC SQL INSERT INTO rownum3 VALUES (7,17),(8,18);  /* causes unique constraint error */
 
  EXEC SQL
  UPDATE rownum3 SET b = :b_int WHERE a = :a_int;

  if (SQLCODE != 0) {
    printf("Failed to update. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Update succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* In CS, 8102 while updating index table, 1 index defined due to unique constraint */
void test6()
{
printf("\n\ntest6 : Expecting rownumber = 8\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i;
    b_int[i] = i;
    c_int[i] = i+10 ;
    d_int[i] = i+10;
}

  EXEC SQL DELETE FROM rownum3;
  EXEC SQL INSERT INTO rownum3 VALUES (18,17),(28,18);  /* causes unique constraint error when (18,17) updated to (18, 18)*/



EXEC SQL BEGIN

 UPDATE rownum3 SET b = :c_int WHERE a = :d_int ;
 
 INSERT INTO rownum3 VALUES (:a_int, :b_int) ;

END;


  if (SQLCODE != 0) {
    printf("Failed. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }

}

/* In CS, 8102 while inserting into base table, no indices */
void test7()
{
printf("\n\ntest7 : Expecting rownumber = 3\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i;
    b_int[i] = i;
    c_int[i] = i+10 ;
    d_int[i] = i+10;
}

  EXEC SQL DELETE FROM rownum1;
  EXEC SQL INSERT INTO rownum1 VALUES (3,3);  /* causes unique constraint error when inserting (3,3) again*/



EXEC SQL BEGIN

 UPDATE rownum1 SET b = :c_int WHERE a = :d_int ;
 
 INSERT INTO rownum1 VALUES (:a_int, :b_int) ;

END;


  if (SQLCODE != 0) {
    printf("Failed. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }

}

/* 8419 while updating base table, no indices defined */
void test8() {
printf("\n\ntest8 : Expecting rownumber = 8\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i;
}

    a_int[8] = 0;  /* causes divide by zero error */

  EXEC SQL DELETE FROM rownum1;
 
  EXEC SQL
  UPDATE rownum1 SET b = :b_int WHERE a = 100 / :a_int;

  if (SQLCODE != 0) {
    printf("Failed to update. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Update succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* 8419 while deleting base table, 1 index defined */
void test9() {
printf("\n\ntest9 : Expecting rownumber = 8\n");

Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i;
}

    a_int[8] = 0;  /* causes divide by zero error */

  EXEC SQL DELETE FROM rownum2;
 
  EXEC SQL
  DELETE FROM rownum2 WHERE a = 100 / :a_int;

  if (SQLCODE != 0) {
    printf("Failed to delete. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Delete succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* Negative test. 8419 while evaluating where clause of cursor select */
void test10()
{
printf("\n\ntest10a : Expecting rownumber = -1\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = 0; 
    b_int[i] = i;
    c_int[i] = i+1;
}
 
  EXEC SQL INSERT INTO rownum1 VALUES (1,1);

  EXEC SQL DECLARE C1 CURSOR FOR
  SELECT A FROM rownum1 WHERE B = 100 / :b_int ;

  EXEC SQL OPEN C1;

  EXEC SQL FETCH C1 INTO :a_int ;

  printf("\n\ntest10b : Expecting rownumber = -1\n");
  EXEC SQL INSERT INTO rownum1 VALUES (100,100);
  c_int[6] = 0;
  EXEC SQL SELECT * INTO :e_int, :f_int FROM rownum1 WHERE rownum1.a = 100 / :c_int; 

}

/* rownumber not output if derived rowsets are present in statement*/
void test11() {
printf("\n\ntest11 : Expecting rownumber = -1 in both cases\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i;
    b_int[i] = i;
    c_int[i] = i+1;
}

    a_int[8] = 7;  /* causes unique constraint error */
    b_int[5] = 3;


  EXEC SQL DELETE FROM rownum1;
  EXEC SQL
  INSERT INTO rownum1 SELECT a1,b1 FROM ROWSET(:a_int, :b_int) AS rs(a1,b1) ;

  if (SQLCODE != 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Insert succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }

  c_int[7] = 0;
  EXEC SQL SELECT * INTO :e_int, :f_int FROM ROWSET(:a_int, :b_int) AS rs(a,b) WHERE rs.a = 100 / :c_int; 


}

/* 8419 while updating base table, no indices defined */
void test12() {
printf("\n\ntest12 : Expecting rownumber = 8\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i;
}

    a_int[8] = 0;  /* causes divide by zero error */

  EXEC SQL DELETE FROM rownum1;

  EXEC SQL INSERT INTO rownum1 VALUES (0,0);
 
  EXEC SQL
  UPDATE rownum1 SET b = 100/:a_int WHERE :b_int = 8 AND a = 0;

  if (SQLCODE != 0) {
    printf("Failed to update. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Update succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

Int32 main()
{
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
   test12();
  return(0);
}
