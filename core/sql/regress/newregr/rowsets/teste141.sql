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
/* teste141.sql
 * Suresh Subbiah 
 * 07-05-2005
 * embedded C tests for setting rownumber in diags area
 *
 *   Test Classification: Positive and Negative
 *   Test Level: Functional
 *   Test Coverage:
 *      - constraint violation on when before trigger fires on rowset insert (negative)
 *      - constraint violation on when before trigger fires on rowset update (positive)
 *      - constraint violation on when after trigger fires on rowset insert (positive)
 *      - constraint violation on when after trigger fires on rowset update (positive)
 *      - RI constraint violation on rowset insert (positive)
 *      - RI constraint violation on rowset update(positive)
 *      - expression error on rowset insert on a table with MV, error during base table insert(positive)
 *      - expression error on rowset delete on a table with MV, error during base table delete(positive)
 *      - Unique constraint violation on rowset insert on table with multiple indexes
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

EXEC SQL MODULE CAT.SCH.TESTE141M NAMES ARE ISO88591;

exec sql whenever sqlerror call display_diagnosis;

exec sql control query default upd_savepoint_on_error 'OFF';


EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [10] Int32 a_int;
  ROWSET [10] Int32 b_int;
EXEC SQL END DECLARE SECTION;

EXEC SQL BEGIN DECLARE SECTION;  
/**** host variables for get diagnostics *****/
   NUMERIC(5) i;
   NUMERIC(5) hv_num;
   Int32 hv_sqlcode;
   Int32 hv_rownum;
   char hv_msgtxt[129];
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
   hv_msgtxt[128]='\0';

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
   hv_msgtxt[128]='\0';
  }

   SQLCODE = savesqlcode ;
}




/* Negative test. RowNumber is not available for the flow node for before triggers defined on an insert statement */
/* 8101 when before trigger fires on an insert */
void test1() {
printf("\n\ntest1 : Expecting rownumber = -1\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+1;
}


  EXEC SQL DELETE FROM rownum5;

  a_int[5] = -1;

  EXEC SQL INSERT INTO rownum5 VALUES (:a_int, :b_int);

  if (SQLCODE != 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Insert succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* Positive test. RowNumber is available for the flow node for before update/delete triggers */
/* 8101 when before trigger fires on an update */
void test2() {
printf("\n\ntest2 : Expecting rownumber = 5\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+100;
}


  EXEC SQL DELETE FROM rownum6;

  EXEC SQL INSERT INTO rownum6 VALUES (:a_int, :b_int); 

  b_int[5] = 5;

  EXEC SQL UPDATE rownum6 SET b = :b_int WHERE a = :a_int; 

  if (SQLCODE != 0) {
    printf("Failed to update. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Update succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}


/* 8101 when after trigger fires on an insert */
void test3() {
printf("\n\ntest3 : Expecting rownumber = 7\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+1;
}


  EXEC SQL DELETE FROM rownum7;

  a_int[7] = -1;

  EXEC SQL INSERT INTO rownum7 VALUES (:a_int, :b_int);

  if (SQLCODE != 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Insert succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* Positive test. RowNumber is available for the flow node for before update/delete triggers */
/* 8101 when after trigger fires on an update */
void test4() {
printf("\n\ntest4 : Expecting rownumber = 3\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+100;
}


  EXEC SQL DELETE FROM rownum7;

  EXEC SQL INSERT INTO rownum7 VALUES (:a_int, :b_int); 

  b_int[3] = -5;

  EXEC SQL UPDATE rownum7 SET b = :b_int WHERE a = :a_int; 

  if (SQLCODE != 0) {
    printf("Failed to update. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Update succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* 8103 when RI constarint fails on an insert */
void test5() {
printf("\n\ntest5 : Expecting rownumber = 6\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+1;
}


  EXEC SQL DELETE FROM rownum11;
  EXEC SQL DELETE FROM rownum8;
  EXEC SQL INSERT INTO rownum11 VALUES (:a_int, :b_int);

  b_int[6] = -1;

  EXEC SQL INSERT INTO rownum8 VALUES (:a_int, :b_int);

  if (SQLCODE != 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Insert succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* 8103 when RI constarint fails on an update */
void test6() {
printf("\n\ntest6 : Expecting rownumber = 9\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+1;
}


  EXEC SQL DELETE FROM rownum8;
  EXEC SQL DELETE FROM rownum11;
  EXEC SQL INSERT INTO rownum11 VALUES (:a_int, :b_int);
  EXEC SQL INSERT INTO rownum8 VALUES (:a_int, :b_int);

  b_int[9] = -1;

  EXEC SQL UPDATE rownum8 SET y= :b_int where x = :a_int;

  if (SQLCODE != 0) {
    printf("Failed to update. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Update succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* 8419 when inserting into a table with an MV. The error is raised when inserting into
the base table. Could not come up with a test that would fail on the insert to the MV.
If such test exists, then it would be a negative test as rowsetIterator is not enabled
for blocked union nodes. */
void test7() {
printf("\n\ntest7 : Expecting rownumber = 8\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = 1;
}

b_int[8] = 0;

  EXEC SQL DELETE FROM rownum9;
  EXEC SQL INSERT INTO rownum9 VALUES (:a_int/:b_int, :b_int);

  if (SQLCODE != 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Insert succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* 8419 when deleting a table with an MV. The error is raised when deleting the
the base table. Could not come up with a test that would fail on the delete to the MV.
If such test exists, then it would be a positive test. */
void test8() {
printf("\n\ntest8 : Expecting rownumber = 7\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+1;
}


  EXEC SQL DELETE FROM rownum9;
  EXEC SQL INSERT INTO rownum9 VALUES (:a_int, :b_int);

  b_int[7] = 0;

  EXEC SQL DELETE FROM rownum9 WHERE a = :a_int/:b_int;

  if (SQLCODE != 0) {
    printf("Failed to delete. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Delete succeeded. SQLCODE = %d\n", SQLCODE);
    EXEC SQL COMMIT ;
  }
}

/* 8102 when inserting into a table with multiple indexes. Error from inserting into indexes */
void test9() {
printf("\n\ntest9 : Expecting rownumber = 2\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+1;
}

b_int[2] = 2;

  EXEC SQL DELETE FROM rownum10;

  EXEC SQL INSERT INTO rownum10 VALUES (:a_int , :a_int, :b_int, :a_int);


  if (SQLCODE != 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Insert succeeded. SQLCODE = %d\n", SQLCODE);
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

  return(0);
}
