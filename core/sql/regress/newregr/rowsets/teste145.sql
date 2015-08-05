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

EXEC SQL MODULE CAT.SCH.TEMPM NAMES ARE ISO88591;

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
   Int32 hv_rowindex;
   Int32 hv_rowcount;
   char hv_msgtxt[129];
   char hv_sqlstate[6];
   char hv_tabname[129];
   char SQLSTATE[6];
   Int32 SQLCODE;
EXEC SQL END DECLARE SECTION;

#define SIZE 20

/*****************************************************/
void displayRowsetRowCountArray()
/*****************************************************/
{
  Int32 rowcountstmtnum = 102;
  Int32 rowsetrowsaffected[100];
  Int32 retcode ;
  Int32 size = 100;
  Int32 actual_size = 0;

  
  EXEC SQL CONTROL QUERY DEFAULT ROWSET_ROW_COUNT 'ON' ;

  retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(0, 
					     rowcountstmtnum, 
					     rowsetrowsaffected, 
					     0,
					     size,
					     &actual_size);

   if (retcode == 0) 
   {
     printf("Rowset Row Count Array:\n");
     for(Int32 i=0;i<actual_size;i++)
      printf("Rows affected by row number %d = %d\n",i,rowsetrowsaffected[i]);
     printf("\n");
   }
   else
    printf("SQL_EXEC_GetDiagnosticsStmtInfo2 returned retcode = %d\n", retcode);
}

/*****************************************************/
void display_diagnosis()
/*****************************************************/
{
  Int32 rowcondnum = 103;
  Int32 rowcountstmtnum = 102;
  Int32 rowsetrowsaffected[SIZE];
  Int32 retcode ;
  Int32 size = SIZE;
  Int32 actual_size = 0;

  Int32 savesqlcode = SQLCODE ;
  hv_rowcount = -1 ;
  hv_rowindex = -2 ;
  exec sql get diagnostics :hv_num = NUMBER,
		:hv_rowcount = ROW_COUNT;

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[128]='\0';
   memset(hv_sqlstate,' ',sizeof(hv_sqlstate));
   hv_sqlstate[6]='\0';

   printf("Number of conditions  : %d\n", hv_num);
   printf("Number of rows inserted: %d\n", hv_rowcount);
   printf("\n");

   displayRowsetRowCountArray();

  for (i = 1; i <= hv_num; i++) {
      exec sql get diagnostics exception :i                
          :hv_tabname = TABLE_NAME,
          :hv_sqlcode = SQLCODE,
	  :hv_sqlstate = RETURNED_SQLSTATE,
/*	  :hv_rowindex = ROW_INDEX,  */
          :hv_msgtxt = MESSAGE_TEXT ;

  retcode = SQL_EXEC_GetDiagnosticsCondInfo2(rowcondnum, i, &hv_rowindex, 0,0,0);


   printf("Condition number : %d\n", i);
   printf("ROW INDEX : %d\n", hv_rowindex);
   printf("SQLCODE : %d\n", hv_sqlcode);
   printf("SQLSTATE  : %s\n", hv_sqlstate);
   printf("TABLE   : %s\n", hv_tabname);
   printf("\n");

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[128]='\0';
   memset(hv_tabname,' ',sizeof(hv_tabname));
   hv_tabname[128]='\0';
   memset(hv_sqlstate,' ',sizeof(hv_sqlstate));
   hv_sqlstate[6]='\0';
  }

   SQLCODE = savesqlcode ;
}






/* Positive test. RowNumber is available for the flow node for before update/delete triggers */
/* 8101 when before trigger fires on an update */
void test2() {
printf("\n\ntest2\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+100;
}


  EXEC SQL DELETE FROM rownum6;

  EXEC SQL INSERT INTO rownum6 VALUES (:a_int, :b_int); 

  EXEC SQL UPDATE rownum6 SET b = :b_int WHERE a = :a_int; 

  if (SQLCODE != 0) {
    printf("Failed to update. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Update succeeded. SQLCODE = %d\n", SQLCODE);
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
}



/* Positive test. RowNumber is available for the flow node for before update/delete triggers */
/* 8101 when after trigger fires on an update */
void test4() {
printf("\n\ntest4\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+100;
}


  EXEC SQL DELETE FROM rownum7;

  EXEC SQL INSERT INTO rownum7 VALUES (:a_int, :b_int); 


  EXEC SQL UPDATE rownum7 SET b = :b_int WHERE a = :a_int; 

  if (SQLCODE != 0) {
    printf("Failed to update. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Update succeeded. SQLCODE = %d\n", SQLCODE);
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
}



/* 8103 when RI constarint fails on an update */
void test6() {
printf("\n\ntest6\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+1;
}


  EXEC SQL DELETE FROM rownum8;
  EXEC SQL DELETE FROM rownum11;
  EXEC SQL INSERT INTO rownum11 VALUES (:a_int, :b_int);
  EXEC SQL INSERT INTO rownum8 VALUES (:a_int, :b_int);


  EXEC SQL UPDATE rownum8 SET y= :b_int where x = :a_int;

  if (SQLCODE != 0) {
    printf("Failed to update. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Update succeeded. SQLCODE = %d\n", SQLCODE);
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
}


/* 8419 when deleting a table with an MV. The error is raised when deleting the
the base table. Could not come up with a test that would fail on the delete to the MV.
If such test exists, then it would be a positive test. */
void test8() {
printf("\n\ntest8\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+1;
}


  EXEC SQL DELETE FROM rownum9;
  EXEC SQL INSERT INTO rownum9 VALUES (:a_int, :b_int);


  EXEC SQL DELETE FROM rownum9 WHERE a = :a_int/:b_int;

  if (SQLCODE != 0) {
    printf("Failed to delete. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Delete succeeded. SQLCODE = %d\n", SQLCODE);
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
} 

void test10() {
printf("\n\ntest10\n");
Int32 i=0;
for (i=0; i<10; i++) {
    a_int[i] = i+1;
    b_int[i] = i+1;
}


  EXEC SQL DELETE FROM rownum2;
  EXEC SQL INSERT INTO rownum2 VALUES (:a_int, :b_int);


  EXEC SQL UPDATE  rownum2 SET b = :b_int*100 WHERE a = :a_int;

  if (SQLCODE != 0) {
    printf("Failed to update. SQLCODE = %d\n",SQLCODE);
  }
  else {
    printf("Update succeeded. SQLCODE = %d\n", SQLCODE);
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
}


Int32 main()
{

   test2();
   test4();
   test6();
   test8();
   test10();

  return(0);
}
