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
/* teste251.sql
 * Mike Hanlon & Suresh Subbiah
 * 02-16-2005
 *
 * embedded C tests for Non Atomic Rowsets
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *   VSBB Non-atomic rowset insert tests
 *
 */                                                    

/*  DDL for table nt1setset and view ntv1set is
CREATE SET TABLE nt1setset (id int not null, 
eventtime timestamp, 
description varchar(12), 
primary key (id), check (id > 0)) ;   

CREATE VIEW ntv1set (v_id, v_eventtime, v_description) AS
SELECT id, eventtime, description from nt1setset
WHERE id > 100
WITH CHECK OPTION ;
*/


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#define NAR00 30022
#define SIZE 500


void display_diagnosis();
void display_diagnosis3();
Int32 test1();
Int32 test2();
Int32 test3();
Int32 test4();
Int32 test5();
Int32 test6();
Int32 test7();
Int32 test8();
Int32 test9();
Int32 test10();
Int32 test11();
Int32 test12();
Int32 test13();
Int32 test14();
Int32 test15();
Int32 test16();
Int32 test17();
Int32 test18();
Int32 test19();
Int32 test20();
Int32 test21();
Int32 test22();
Int32 test23();
Int32 test24();
Int32 test25();


EXEC SQL MODULE CAT.SCH.TESTE261M NAMES ARE ISO88591;

/* globals */


EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [SIZE] Int32 a_int;
  ROWSET [SIZE] Int32 a1_int;
  ROWSET [SIZE] Int32 a2_int;
  ROWSET [SIZE] Int32 a3_int;
  ROWSET [SIZE] VARCHAR b_char5[5];
  ROWSET [SIZE] TIMESTAMP b_time;
  ROWSET [SIZE] Int32 j;
  Int32 inp;
  Int32 numRows ;
  Int32 savesqlcode;
EXEC SQL END DECLARE SECTION;


EXEC SQL BEGIN DECLARE SECTION;  
/**** host variables for get diagnostics *****/
   NUMERIC(5) i;
   NUMERIC(5) hv_num;
   Int32 hv_sqlcode;
   Int32 hv_rowindex;
   Int32 hv_rowcount;
   char hv_msgtxt[329];
   char hv_sqlstate[6];
   char hv_tabname[129];
   char SQLSTATE[6];
   Int32 SQLCODE;
EXEC SQL END DECLARE SECTION;



Int32 main()
{
/*  EXEC SQL CONTROL QUERY DEFAULT IGNORE_DUPLICATE_KEYS 'ON' ; */
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
 test13();
 test14();
 test15();
 test16();
 test17();
 test18();
 test19();
 test20();
 test21();
 test22();  
test23();  
test24();
test25();  
  return(0);

}


Int32 test1()
{
printf("\n ***TEST1 : Expecting -8101 on rows 95 & 399***\n");

EXEC SQL DELETE FROM nt1set ;


EXEC SQL COMMIT ;


Int32 i=0;
for (i=0; i<SIZE; i++)
    a_int[i] = i+1;

a_int[95] = -1 ;
a_int[399] = -1 ;
 
  EXEC SQL
  INSERT INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test2()
{
printf("\n ***TEST2 : Expecting -8101 on rows 0 & 499***\n");

EXEC SQL DELETE FROM nt1set ;


EXEC SQL COMMIT ;


Int32 i=0;
for (i=0; i<SIZE; i++)
    a_int[i] = i+1;

a_int[0] = -1 ;
a_int[499] = -1 ;
 
  EXEC SQL
  INSERT INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test3()
{
printf("\n ***TEST3 : Expecting -8101 on all rows except rows 92***\n");

EXEC SQL DELETE FROM nt1set ;

EXEC SQL COMMIT ;

Int32 i=0;
for (i=0; i<SIZE; i++)
    a_int[i] = -(i+1);

a_int[92] = 93 ;
 
  EXEC SQL
  INSERT INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test4()
{
printf("\n ***TEST4 : Expecting -8101 on rows 110 & 112, and -8102 on rows 109 & 111***\n");

EXEC SQL DELETE FROM nt1set ;

EXEC SQL COMMIT ;

Int32 i=0;
for (i=0; i<SIZE; i++)
    a_int[i] = i+1;

a_int[109] = 1;
a_int[110] = -1 ; 
a_int[111] = 1;
a_int[112] = -1;
 
  EXEC SQL
  INSERT INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test5()
{
printf("\n ***TEST5 : Expecting -8105 on row 401***\n");

EXEC SQL DELETE FROM nt1set ;

EXEC SQL COMMIT ;

Int32 i=0;
for (i=0; i<SIZE; i++)
    a_int[i] = i+101;

a_int[401] = 22 ;
 
  EXEC SQL
  INSERT INTO ntv1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?') NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test6()
{
printf("\n ***TEST6 : Expecting -8403 on row 481***\n");

EXEC SQL DELETE FROM nt1set ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    } 


   a_int[481] = -5;
 
  EXEC SQL
  INSERT INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp), 
  SUBSTRING('how are you?', 1, :a_int ))  NOT ATOMIC ;
 
  if (SQLCODE < 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test7()
{
printf("\n ***TEST7 : Expecting -8411 on row 12***\n");
/* error raised in PA node */

EXEC SQL DELETE FROM nt1set ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    } 

a_int[12] = 2000000000 ;

  EXEC SQL
  INSERT INTO nt1set 
  VALUES (:a_int + 2000000000, 
  cast ('05.01.1997 03.04.55.123456' as timestamp), 
  'how are you?')  NOT ATOMIC  ;
 
  if (SQLCODE < 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test8()
{
printf("\n ***TEST8 : Expecting -8411 on all rows except row 12***\n");
/* error raised in PA node. Uses GetCondInfo3 to get diagnostics in an array*/

EXEC SQL DELETE FROM nt1set ;

Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 2000000000 ;;
    } 

 a_int[12] = 13 ; 

  EXEC SQL
  INSERT INTO nt1set 
  VALUES (:a_int + 2000000000, 
  cast ('05.01.1997 03.04.55.123456' as timestamp), 
  'how are you?')  NOT ATOMIC  ;
 
  if (SQLCODE < 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis3();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test9()
{
printf("\n ***TEST9 : Expecting -8412 on row 255***\n");
EXEC SQL DELETE FROM nt1set ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    strcpy(b_char5[i], "you?");
    } 

    memset(b_char5[255], '?', 5);


 
  EXEC SQL
  INSERT INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are ' || :b_char5)  NOT ATOMIC  ;
 
  if (SQLCODE < 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test10()
{
printf("\n ***TEST10 : Expecting -8415 on row 489***\n");
EXEC SQL DELETE FROM nt1set ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    strcpy(b_time[i], "05.01.1997 03.04.55.123456");
    } 



strcpy(b_time[489], "aaaaaaaaaaaaaaaaa");
  EXEC SQL
  INSERT INTO nt1set 
  VALUES (:a_int, 
  CONVERTTIMESTAMP(JULIANTIMESTAMP(:b_time)), 
  'how are you?') NOT ATOMIC ;
 
  if (SQLCODE < 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test11()
{
printf("\n ***TEST11 : Expecting -8419 on rows 1,2,3,4,5,391,392,393,394, & 395. Also expecting -8102 on row 40***\n");
 EXEC SQL DELETE FROM nt1set ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    j[i] = 1;
}


a_int[40] = 1 ;
 j[1] = 0; 
j[2] = 0;
j[3] = 0;
j[4] = 0;
j[5] = 0;
j[391] = 0; 
j[392] = 0;
j[393] = 0;
j[394] = 0;
j[395] = 0; 
 
  EXEC SQL
  INSERT INTO nt1set VALUES (:a_int/:j, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test12()
{
printf("\n ***TEST12 : Expecting -8102 on all rows except row 0***\n");
 EXEC SQL DELETE FROM nt1set ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 1;
}



 
  EXEC SQL
  INSERT INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test13()
{
printf("\n ***TEST13 : Expect -8102 only for row 5 and row 10 \n");
 EXEC SQL DELETE FROM nt1set ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 1;
    strcpy(b_char5[i], "you?");
}
strcpy(b_char5[5], "hi");
strcpy(b_char5[10], "me");

  exec sql control query default QUERY_CACHE '0';
  
  /* INSERT_VSBB set to OFF is temporary until we get
     a fix from DP2 to support set feature. */
  exec sql control query default INSERT_VSBB 'OFF';
 
  EXEC SQL
  INSERT INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),:b_char5)  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test14()
{
printf("\n ***TEST14 : Expect no errors. All duplicate rows are ignored in set table.\n ");
 EXEC SQL DELETE FROM nt1set ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 1;
}
  
  exec sql control query default QUERY_CACHE '0';
  
  /* INSERT_VSBB set to OFF is temporary until we get
     a fix from DP2 to support set feature. */
  exec sql control query default INSERT_VSBB 'OFF';
 
  EXEC SQL
  INSERT INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test15()
{
printf("\n ***TEST15 : Expect -8102 only for row 5 and row 10, VSBB inserts used.\n ");
 EXEC SQL DELETE FROM nt1set ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 1;
    strcpy(b_char5[i], "you?");
}
strcpy(b_char5[5], "hi");
strcpy(b_char5[10], "me");

  exec sql control query default QUERY_CACHE '0';
  
  EXEC SQL
  INSERT USING VSBB INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),:b_char5)  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test16()
{
printf("\n ***TEST16 : Expect no errors. All duplicate rows are ignored in set table.\n");
printf("   ***VSBB Insertes used. \n");
 EXEC SQL DELETE FROM nt1set ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 1;
}
  
  exec sql control query default QUERY_CACHE '0';
  
  EXEC SQL
  INSERT USING VSBB INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

/* Repeat tests 14, 15 & 16 on SET tables wit rowsets ATOMIC. */
Int32 test17()
{
printf("\n ***TEST17 : Expect no errors. All duplicate rows are ignored in set table ");
printf("\n ***ATOMIC Rowsets , regular inserts used.\n");

 EXEC SQL DELETE FROM nt1set ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 1;
}
  
  exec sql control query default QUERY_CACHE '0';
  exec sql control query default INSERT_VSBB 'OFF';
 
  EXEC SQL
  INSERT INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?');
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test18()
{
  printf("\n ***TEST18 : Expect -8102 error");
  printf("\n ***ATOMIC rowsets & VSBB inserts \n");
  EXEC SQL DELETE FROM nt1set ;
  for (i=0; i<SIZE; i++) {
      a_int[i] = 1;
      strcpy(b_char5[i], "you?");
  }
  strcpy(b_char5[5], "hi");
  strcpy(b_char5[10], "me");
  exec sql control query default QUERY_CACHE '0';
  
  EXEC SQL
  INSERT USING VSBB INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),:b_char5);
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
  return(0);
}

Int32 test19()
{
  printf("\n ***TEST19 : Expect no errors. All duplicate rows are ignored in set table.\n");
  printf("\n*** Atomic rowsets & VSBB Insertes used.\n");
  EXEC SQL DELETE FROM nt1set ;
  for (i=0; i<SIZE; i++) {
      a_int[i] = 1;
  }
  
  exec sql control query default QUERY_CACHE '0';
  
  EXEC SQL
  INSERT USING VSBB INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?');
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
  return(0);
}


Int32 test20()
{
printf("\n ***TEST20 SET Table with ON STATEMENT MV, VSBB Inserts:");
printf("\n ***Expect no errors. All duplicate rows are ignored in SET table");
printf("\n ***and ON Statement MV.\n");
  EXEC SQL DELETE FROM settablemv;
  for (i=0; i<SIZE; i++) {
      a_int[i] = 1;
  }
  exec sql control query default QUERY_CACHE '0';
  
  EXEC SQL
  INSERT USING VSBB INTO settablemv VALUES (:a_int, :a_int + 1, :a_int+2 );
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
  return(0);
}

Int32 test21()
{
printf("\n ***TEST21 SET Table with ON STATEMENT MV, regular Inserts:");
printf("\n ***Expect no errors. All duplicate rows are ignored in SET table");
printf("\n ***and ON Statement MV.\n");
  EXEC SQL DELETE FROM settablemv ;
  for (i=0; i<SIZE; i++) {
      a_int[i] = 1;
  }
  
  exec sql control query default INSERT_VSBB 'OFF';
  exec sql control query default QUERY_CACHE '0';
  
  EXEC SQL
  INSERT INTO settablemv VALUES (:a_int, :a_int + 1, :a_int+2 );
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
  return(0);
}

Int32 test22()
{
printf("\n ***TEST22 : Expect 8102 errors. All insert-values duplicate rows are not ignored in set table, mode_special mode. VSBB Insertes used \n ");
 EXEC SQL DELETE FROM nt1set ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 1;
}
  
  exec sql control query default QUERY_CACHE '0';
  exec sql control query default comp_bool_58 'ON';
  exec sql control query default mode_special_1 'ON';
  exec sql control query default rounding_mode '2';
  exec sql control query default limit_max_numeric_precision 'ON';
  
  EXEC SQL
  INSERT USING VSBB INTO nt1set VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test23()
{
  printf("\n ***TEST23 : Expect sql code 0, followed by 20 8102 followed by sql code 0.");
  printf("\n All duplicate rows are ignored in set table. However, in mode_special one, only"); 
  printf("\n insert-select queries can expect duplicate row ignored. \n");
  exec sql control query default QUERY_CACHE '0';
  exec sql control query default comp_bool_58 'ON';
  exec sql control query default mode_special_1 'ON';
  exec sql control query default rounding_mode '2';
  exec sql control query default limit_max_numeric_precision 'ON';
  exec sql control query default INSERT_VSBB 'OFF';

  Int32 i=0;
  for (i=0; i<20; i++) {
      a_int[i] = i;
      a1_int[i] =i;
      a2_int[i] =i;
      }
     inp = 20;

  EXEC SQL DELETE from settable;

  /* Insert first 20 rows */
  EXEC SQL 
    ROWSET FOR INPUT SIZE :inp
    INSERT INTO settable VALUES (:a_int , :a1_int, :a2_int ) NOT ATOMIC;

  if (SQLCODE <=0)
    printf("SQLCODE = %d\n", SQLCODE);
  else
    if (SQLCODE !=0)
	display_diagnosis();

  SQL_EXEC_ClearDiagnostics(NULL);
  SQLCODE = 0;
  
  /* Insert next 20 duplicate rows */
  EXEC SQL 
    ROWSET FOR INPUT SIZE :inp
    INSERT INTO settable VALUES (:a_int , :a1_int, :a2_int ) NOT ATOMIC;

  if (SQLCODE <=0)
    printf("SQLCODE = %d\n", SQLCODE);
  else
    if (SQLCODE !=0)
	display_diagnosis();

  SQL_EXEC_ClearDiagnostics(NULL);
  SQLCODE = 0;

  /* Insert next 20 unique rows */
  for (i=0; i<20; i++) {
      a_int[i] = i+40;
      a1_int[i] =i+40;
      a2_int[i] = i+40 ;
      }
   
  EXEC SQL 
    ROWSET FOR INPUT SIZE :inp
    INSERT INTO settable VALUES (:a_int , :a1_int, :a2_int ) NOT ATOMIC;

  if (SQLCODE <=0)
    printf("SQLCODE = %d\n", SQLCODE);
  else
    if (SQLCODE !=0)
	display_diagnosis();
	
  EXEC SQL COMMIT ;

  return(0);

}

Int32 test24()
{
  printf("\n ***TEST24 USING VSBB inserts: Expect sql code 0, followed by 20 8102 followed by sql code 0.");
  printf("\n All duplicate rows are ignored in set table. However, in mode_special one, only"); 
  printf("\n insert-select queries can expect duplicate row ignored.\n");
  exec sql control query default QUERY_CACHE '0';
    exec sql control query default comp_bool_58 'ON';
    exec sql control query default mode_special_1 'ON';
    exec sql control query default rounding_mode '2';
    exec sql control query default limit_max_numeric_precision 'ON';

  Int32 i=0;
  for (i=0; i<20; i++) {
      a_int[i] = i;
      a1_int[i] =i;
      a2_int[i] =i;
      }
     inp = 20;

  EXEC SQL DELETE from settable;

  /* Insert first 20 rows */
  EXEC SQL 
    ROWSET FOR INPUT SIZE :inp
    INSERT USING VSBB INTO settable VALUES (:a_int , :a1_int, :a2_int ) NOT ATOMIC;

  if (SQLCODE <=0)
    printf("SQLCODE = %d\n", SQLCODE);
  else
    if (SQLCODE !=0)
	display_diagnosis();
  
  SQL_EXEC_ClearDiagnostics(NULL);
  SQLCODE = 0;

  /* Insert next 20 duplicate rows */
  EXEC SQL 
    ROWSET FOR INPUT SIZE :inp
    INSERT USING VSBB INTO settable VALUES (:a_int , :a1_int, :a2_int ) NOT ATOMIC;

  if (SQLCODE <=0)
    printf("SQLCODE = %d\n", SQLCODE);
  else
    if (SQLCODE !=0)
	display_diagnosis();

  SQL_EXEC_ClearDiagnostics(NULL);
  SQLCODE = 0;

  /* Insert next 20 unique rows */
  for (i=0; i<20; i++) {
      a_int[i] = i+40;
      a1_int[i] =i+40;
      a2_int[i] = i+40 ;
      }
   
  EXEC SQL 
    ROWSET FOR INPUT SIZE :inp
    INSERT USING VSBB INTO settable VALUES (:a_int , :a1_int, :a2_int ) NOT ATOMIC;

  if (SQLCODE <=0)
    printf("SQLCODE = %d\n", SQLCODE);
  else
    if (SQLCODE !=0)
	display_diagnosis();
	
  EXEC SQL COMMIT ;

  return(0);
}

Int32 test25()
{
  printf("\n ***TEST25 USING VSBB inserts and NOT NAR: \n");
  printf("Expect sql code 0, followed by 8102, followed by 0\n");
  exec sql control query default QUERY_CACHE '0';
  
  Int32 i=0;
  for (i=0; i<20; i++) {
      a_int[i] = i+1;
      }
     inp = 20;

  EXEC SQL DELETE from nt1set;

  /* Insert first 20 rows */
  EXEC SQL 
    ROWSET FOR INPUT SIZE :inp
    INSERT USING VSBB INTO nt1set VALUES (:a_int , cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?');

  if (SQLCODE <=0)
    printf("SQLCODE = %d\n", SQLCODE);
  else
    if (SQLCODE !=0)
	display_diagnosis();
  
  SQL_EXEC_ClearDiagnostics(NULL);
  SQLCODE = 0;  

  /* Insert next 20 duplicate rows */
  EXEC SQL 
    ROWSET FOR INPUT SIZE :inp
    INSERT USING VSBB INTO nt1set VALUES (:a_int , cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?');

  if (SQLCODE <=0)
    printf("SQLCODE = %d\n", SQLCODE);
  else
    if (SQLCODE !=0)
	display_diagnosis();

  SQL_EXEC_ClearDiagnostics(NULL);
  SQLCODE = 0;

  /* Insert next 20 unique rows */
  for (i=0; i<20; i++) {
      a_int[i] = i+41;
      }
   
  EXEC SQL 
    ROWSET FOR INPUT SIZE :inp
    INSERT USING VSBB INTO nt1set VALUES (:a_int , cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?');

  if (SQLCODE <=0)
    printf("SQLCODE = %d\n", SQLCODE);
  else
    if (SQLCODE !=0)
	display_diagnosis();
	
  EXEC SQL COMMIT ;

  return(0);
}




/*****************************************************/
void display_diagnosis()
/*****************************************************/
{
  Int32 rowcondnum = 103;
  Int32 retcode ;

  savesqlcode = SQLCODE ;
  hv_rowcount = -1 ;
  hv_rowindex = -2 ;
  exec sql get diagnostics :hv_num = NUMBER,
		:hv_rowcount = ROW_COUNT;

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[328]='\0';
   memset(hv_sqlstate,' ',sizeof(hv_sqlstate));
   hv_sqlstate[6]='\0';

   

   printf("Number of conditions  : %d\n", hv_num);
   printf("Number of rows inserted: %d\n", hv_rowcount);
   printf("\n");

   /* If we have more than 100 conditions, just print the first 10 */
   if (hv_num > 100) {
      hv_num = 10;
      printf("\n Only the first 10 conditions will be printed. \n");
    }

  for (i = 1; i <= hv_num; i++) {
      exec sql get diagnostics exception :i                
          :hv_tabname = TABLE_NAME,
          :hv_sqlcode = SQLCODE,
	  :hv_sqlstate = RETURNED_SQLSTATE,
/*	  :hv_rowindex = ROW_INDEX,  */
          :hv_msgtxt = MESSAGE_TEXT;

  retcode = SQL_EXEC_GetDiagnosticsCondInfo2(rowcondnum, i, &hv_rowindex, 0,0,0);


   printf("Condition number : %d\n", i);
   printf("ROW INDEX : %d\n", hv_rowindex);
   printf("SQLCODE : %d\n", hv_sqlcode);
   printf("SQLSTATE  : %s\n", hv_sqlstate);
   printf("TABLE   : %s\n", hv_tabname);
   printf("TEXT   : %s\n", hv_msgtxt);
   printf("\n");

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[328]='\0';
   memset(hv_tabname,' ',sizeof(hv_tabname));
   hv_tabname[128]='\0';
   memset(hv_sqlstate,' ',sizeof(hv_sqlstate));
   hv_sqlstate[6]='\0';
  }

   SQLCODE = savesqlcode ;
}

/*****************************************************/
void display_diagnosis3()
/*****************************************************/
{
#define NO_OF_COND_ITEMS 500
#define NO_OF_CONDITIONS 100
#define MAX_SQLSTATE_LEN 6
#define MAX_MSG_TEXT_LEN 350
#define MAX_ANSI_NAME 129

  SQLDIAG_COND_INFO_ITEM_VALUE	gCondInfoItems[NO_OF_COND_ITEMS];
  char		Sqlstate[NO_OF_CONDITIONS][ MAX_SQLSTATE_LEN];
  char		MessageText[NO_OF_CONDITIONS][MAX_MSG_TEXT_LEN];
  Int32		Sqlcode[NO_OF_CONDITIONS];
  Int32		RowNumber[NO_OF_CONDITIONS];
  char		TableName[NO_OF_CONDITIONS][MAX_ANSI_NAME];
  Int32          SqlstateLen[NO_OF_CONDITIONS];
  Int32          MessageTextLen[NO_OF_CONDITIONS];
  Int32          TableNameLen[NO_OF_CONDITIONS];


  Int32 max_sqlstate_len = MAX_SQLSTATE_LEN;
  Int32 max_msg_text_len = MAX_MSG_TEXT_LEN;
  Int32 max_ansi_name = MAX_ANSI_NAME;

  Int32 retcode;

   exec sql get diagnostics :hv_num = NUMBER,
		:hv_rowcount = ROW_COUNT;
	

   printf("Number of conditions  : %d\n", hv_num);
   printf("Number of rows inserted: %d\n", hv_rowcount);
   printf("\n");

  Int32 conditionsLeft = hv_num;
  Int32 NumConditionsInThisIter = NO_OF_CONDITIONS;
  for (Int32 loop = 0; loop < ((hv_num/NO_OF_CONDITIONS) +1) ; loop++) 
  {
    if (conditionsLeft < NO_OF_CONDITIONS)
      NumConditionsInThisIter = conditionsLeft;

    Int32 i,j;

    for (j=0; j<NumConditionsInThisIter; j++) 
    {
      Sqlcode[j] = 0;
      RowNumber[j] = 0;
      memset(MessageText[j],' ',MAX_MSG_TEXT_LEN);
      memset(Sqlstate[j],' ',MAX_SQLSTATE_LEN);
      memset(TableName[j],' ',MAX_ANSI_NAME);
      SqlstateLen[j] = MAX_SQLSTATE_LEN;
      MessageTextLen[j] = MAX_MSG_TEXT_LEN;
      TableNameLen[j] = MAX_ANSI_NAME;
    }

    j=0;
    for (i=0; i<NumConditionsInThisIter*5; i=i+5) 
    {
      j=j+1;
      gCondInfoItems[i+0].item_id_and_cond_number.item_id = SQLDIAG_SQLCODE;
      gCondInfoItems[i+1].item_id_and_cond_number.item_id = SQLDIAG_RET_SQLSTATE;
      gCondInfoItems[i+2].item_id_and_cond_number.item_id = SQLDIAG_ROW_NUMBER;
      gCondInfoItems[i+3].item_id_and_cond_number.item_id = SQLDIAG_TABLE_NAME;
      gCondInfoItems[i+4].item_id_and_cond_number.item_id = SQLDIAG_MSG_TEXT;
      gCondInfoItems[i+0].item_id_and_cond_number.cond_number_desc_entry = loop*NO_OF_CONDITIONS + j;
      gCondInfoItems[i+1].item_id_and_cond_number.cond_number_desc_entry = loop*NO_OF_CONDITIONS + j;
      gCondInfoItems[i+2].item_id_and_cond_number.cond_number_desc_entry = loop*NO_OF_CONDITIONS + j;
      gCondInfoItems[i+3].item_id_and_cond_number.cond_number_desc_entry = loop*NO_OF_CONDITIONS + j;
      gCondInfoItems[i+4].item_id_and_cond_number.cond_number_desc_entry = loop*NO_OF_CONDITIONS + j;
      gCondInfoItems[i+0].num_val_or_len = &(Sqlcode[j-1]);
      gCondInfoItems[i+1].num_val_or_len = &(SqlstateLen[j-1]);
      gCondInfoItems[i+2].num_val_or_len = &(RowNumber[j-1]);
      gCondInfoItems[i+3].num_val_or_len = &(TableNameLen[j-1]);
      gCondInfoItems[i+4].num_val_or_len = &(MessageTextLen[j-1]);
      gCondInfoItems[i+0].string_val = 0;
      gCondInfoItems[i+1].string_val = Sqlstate[j-1];
      gCondInfoItems[i+2].string_val = 0;
      gCondInfoItems[i+3].string_val = TableName[j-1];
      gCondInfoItems[i+4].string_val = MessageText[j-1];
    }

    retcode = SQL_EXEC_GetDiagnosticsCondInfo3( NumConditionsInThisIter*5,
			    (SQLDIAG_COND_INFO_ITEM_VALUE *)&gCondInfoItems);

    if (retcode != 0) 
    {
      printf("SQL_EXEC_GetDiagnosticsCondInfo3 returned retcode = %d\n", retcode);
      exit (0);
    }

    conditionsLeft = conditionsLeft - NumConditionsInThisIter ;

    for (j=0; j<NumConditionsInThisIter; j++) 
    {
      MessageText[j][MessageTextLen[j]] = 0;
      TableName[j][TableNameLen[j]] = 0;

       printf("Condition number : %d\n", gCondInfoItems[5*j].item_id_and_cond_number.cond_number_desc_entry);
       printf("ROW INDEX : %d\n", RowNumber[j]);
       printf("SQLCODE : %d\n", Sqlcode[j]);
       printf("SQLSTATE  : %s\n", Sqlstate[j]);
       printf("MESSAGE : %s\n", MessageText[j]);
       printf("TABLE   : %s\n", TableName[j]);
       printf("\n");

    }
  }
}
