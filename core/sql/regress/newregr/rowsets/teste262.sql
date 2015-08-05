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
/* teste262.sql
 * Sandhya Sundaresan
 * 04-16-2007
 *
 * embedded C tests for Non Atomic Rowsets with index support
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *   non-VSBB Non-Atomic Rowset insert tests
 *  (this is basically Teste245 but the table has indexes in this case)
 */                                                    

/*  DDL for table snt1 is
CREATE TABLE snt1 (id int not null, id2 int,
eventtime timestamp, 
description varchar(12), 
primary key (id), check (id > 0)) ;  

create index isnt1 on snt1 (id2);  

CREATE TABLE snt2 (id int not null, id2 int,
eventdate date,
primary key (id)) ;

create index isnt2 on snt2 (id2);

*/


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#define NAR00 30022
#define SIZE 50


void display_diagnosis();
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


EXEC SQL MODULE CAT.SCH.NARCLI NAMES ARE ISO88591;

/* globals */


EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [SIZE] Int32 a_int;
  ROWSET [SIZE] VARCHAR b_char6[6];
  ROWSET [SIZE] VARCHAR b_char3[3];
  ROWSET [SIZE] Int32 b_date;
  ROWSET [SIZE] char b_char11[11];
  ROWSET [SIZE] short a_ind ;
  ROWSET [SIZE] VARCHAR b_char5[5];
  ROWSET [SIZE] Int32 b_int;
  ROWSET [SIZE] char name_char[3];
  ROWSET [SIZE] char pattern_char[4];
  ROWSET [SIZE] VARCHAR escape_char[3];
  char n_char[3];
  char p_char[4];
  char e_char[3];
  ROWSET [SIZE] Int32 j;
  Int32 numRows ,inp;
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
   char hv_more[2];
   char SQLSTATE[6];
   Int32 SQLCODE;
EXEC SQL END DECLARE SECTION;



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
 test13();
// test14();
 test15();
return 0;
}

Int32 test1()
{

printf("\n ***TEST1 : Expecting -8102 on row 25***\n");

EXEC SQL DELETE FROM snt1 ;
Int32 i=0;
for (i=0; i<SIZE; i++)
    a_int[i] = i+1;

a_int[25] = 23 ;
 
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int,cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?') NOT ATOMIC ;
 
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
printf("\n ***TEST2 : Expecting -8402 on row 25***\n");

EXEC SQL DELETE FROM snt1 ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    strcpy(b_char6[i], "you?");
    } 

    strcpy(b_char6[25], "you??"); 
  
 
  EXEC SQL

  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are ' || :b_char6) NOT ATOMIC ;
 
  if (SQLCODE < 0) {
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
printf("\n ***TEST3 : Expecting -8404 on row 9***\n");

EXEC SQL DELETE FROM snt1 ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    strcpy(b_char3[i], "?") ;
    } 

strcpy(b_char3[9], "??") ;
 
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int,  cast( '05.01.1997 03.04.55.123456' as timestamp), 
  TRIM(:b_char3 FROM 'how are you?'))  NOT ATOMIC  ;
 
  if (SQLCODE < 0) {
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

printf("\n ***TEST4 : Expecting -8405 on row 32***\n");

EXEC SQL DELETE FROM snt1 ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    b_date[i] = 1 ;
    } 

 b_date[32] = -1 ; 
  EXEC SQL
  INSERT INTO snt1 
  VALUES (:a_int, :a_int,
  CONVERTTIMESTAMP(:b_date*211696834500000000), 
  'how are you?') NOT ATOMIC  ;
 
  if (SQLCODE < 0) {
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

printf("\n ***TEST5 : Expecting -8415 on row 7***\n");

EXEC SQL DELETE FROM snt2 ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    strcpy(b_char11[i], "2000-11-11");
    } 

    strcpy(b_char11[7], "2000-yy-zz");


 
  EXEC SQL
  INSERT INTO snt2 VALUES (:a_int, :a_int,
  cast( :b_char11 as date))  NOT ATOMIC  ;
 
  if (SQLCODE < 0) {
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
printf("\n ***TEST6 : Expecting -8421 on all rows except row 5***\n");

EXEC SQL DELETE FROM snt1 ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    a_ind[i] = -1;
    strcpy(b_char5[i], "you?");
    } 

    a_ind[5] = 0;


 
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int :a_ind, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are ' || :b_char5)  NOT ATOMIC  ;
 
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
printf("\n ***TEST7 : Expecting -8409 on row 45***\n");

EXEC SQL DELETE FROM snt1 ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    b_int[i] = 1001 + i;
    name_char[i][0] = 'a';
    name_char[i][1] = 'b';
    name_char[i][2] = 'c';
    pattern_char[i][0] = 'a';
    pattern_char[i][1] = 'b';
    pattern_char[i][2] = 'c';
    pattern_char[i][3] = 0;
    escape_char[i][0] = '$';
    escape_char[i][1] = 0;
    escape_char[i][2] = 0;
    }

    escape_char[45][1] = 'a';
    

 
  EXEC SQL
  INSERT INTO snt1 VALUES (case when :name_char like :pattern_char escape :escape_char then :a_int else :b_int end, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?') NOT ATOMIC;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
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

printf("\n ***TEST8 : Expecting -8410 on row 25***\n");

EXEC SQL DELETE FROM snt1 ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    b_int[i] = 1001 + i;
    name_char[i][0] = 'a';
    name_char[i][1] = 'b';
    name_char[i][2] = 'c';
    pattern_char[i][0] = 'a';
    pattern_char[i][1] = 'b';
    pattern_char[i][2] = 'c';
    pattern_char[i][3] = 0;
    }

    pattern_char[25][2] = '$';

 
  EXEC SQL
  INSERT INTO snt1 VALUES (case when :name_char like :pattern_char escape '$' then :a_int else :b_int end, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?') NOT ATOMIC;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test9()
{
printf("\n ***TEST9 : Expecting -8419 on rows 1,2,3,4,5,41,42,43,44, & 45. Also expecting -8102 on row 40***\n");
 EXEC SQL DELETE FROM snt1 ;
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
j[41] = 0; 
j[42] = 0;
j[43] = 0;
j[44] = 0;
j[45] = 0; 
 
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int/:j, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
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
printf("\n ***TEST10 : Expecting -8102 on all rows except row 0***\n");
 EXEC SQL DELETE FROM snt1 ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 1;
}



 
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
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
printf("\n ***TEST11 : Expecting -8412 on all rows ***\n");
EXEC SQL DELETE FROM snt1 ; 
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    memset(b_char5[i], '?', 5);
    } 

   /* a_int[2] = 5; strcpy(b_char5[2], "you?");
    a_int[4] = 6; strcpy(b_char5[4] , "you?");*/
    

    
//inp = 50;
exec sql control query default upd_savepoint_on_error 'OFF';
 
  EXEC SQL
 // ROWSET FOR INPUT SIZE :inp
  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are ' || :b_char5)  NOT ATOMIC  ;
 



  if (SQLCODE < 0) {
    display_diagnosis();
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

printf("\n ***TEST12 : Expecting -8101 on all rows ***\n");

 EXEC SQL DELETE FROM snt1 ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 0;
}



 
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    display_diagnosis();
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

printf("\n ***TEST13 : Expecting -30031 after 35 8412 conditions ***\n");

EXEC SQL DELETE FROM snt1 ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    memset(b_char5[i], '?', 5);
    } 

    

  EXEC SQL CONTROL QUERY DEFAULT NOT_ATOMIC_FAILURE_LIMIT '35' ;
 
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are ' || :b_char5)  NOT ATOMIC  ;
 
  if (SQLCODE < 0) {
    display_diagnosis();
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

printf("\n ***TEST14 : Expecting -30031 after 40 -8102 conditions ***\n");

 EXEC SQL DELETE FROM snt1 ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 1;
}


EXEC SQL CONTROL QUERY DEFAULT NOT_ATOMIC_FAILURE_LIMIT '40' ;

 
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    display_diagnosis();
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return(0);

}
/******************************************************************************

  Function: test15

  Description:
  ------------
  This tests attempts to cause an assertion in executor/ex_tuple_flow 
  (Non-Atomic insert received no diags area from the CLI)
  by preparing and executing a non atomic rowset insert statement, 
  changing the table definition, and executing the same statement again.  
  
  The table definition change should remove the file open handle
  aquired when the statement is first executed.  The table definition change
  should not require a recompilation of the plan.

  - Grant privileges
  - Prepare and execute non atomic rowset insert
  - Revoke privileges
  - Re-execute non atomic rowset insert 

******************************************************************************/
Int32 
test15()
{
  printf("\n ***TEST15***\n");

  // grant priviledges 
  EXEC SQL
    GRANT ALL PRIVILEGES ON TABLE snt1 TO "sql.user1";
  
  // create new data for the insert 
  Int32 i=0;
  for (i=0; i<SIZE; i++)
  {
    a_int[i] = SIZE+i+1;
  }

  // insert, revoke privileges, insert again 
  for(i=0;i<2;i++)
  {
    EXEC SQL
      INSERT INTO snt1 VALUES (:a_int, :a_int, 
			       cast( '05.01.1997 03.04.55.123456' as timestamp),
			      'how are you?') NOT ATOMIC ;    

    // create new data to avoid duplicate insertion errors
    for (i=0; i<SIZE; i++)
    {
      a_int[i] = (SIZE*2)+i+1;
    }

    if( i == 0 )
    {
      EXEC SQL
	REVOKE ALL PRIVILEGES ON TABLE snt1 FROM "sql.user1";      
    }      

  }
  
  // check for errors
  if (SQLCODE != 0) 
  {
    display_diagnosis();
    printf( "[ERROR] SQLCODE = %d\n", SQLCODE);
  }
  else 
  {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
  return 0;
}
/*****************************************************/
void display_diagnosis()
/*****************************************************/
{

  savesqlcode = SQLCODE ;
  hv_rowcount = -1 ;
  hv_rowindex = -2 ;
  exec sql get diagnostics :hv_num = NUMBER,
		:hv_rowcount = ROW_COUNT,
		:hv_more = MORE;

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[328]='\0';
   memset(hv_sqlstate,' ',sizeof(hv_sqlstate));
   hv_sqlstate[6]='\0';
   hv_more[1] = '\0';

   printf("Number of conditions  : %d\n", hv_num);
   printf("Number of rows inserted: %d\n", hv_rowcount);
   printf("More conditions?: %s\n", hv_more);
   printf("\n");

  for (i = 1; i <= hv_num; i++) {
      exec sql get diagnostics exception :i                
          :hv_tabname = TABLE_NAME,
          :hv_sqlcode = SQLCODE,
	  :hv_sqlstate = RETURNED_SQLSTATE,
/*	  :hv_rowindex = ROW_INDEX,  */
          :hv_msgtxt = MESSAGE_TEXT;

    Int32 rowcondnum = 103;
    Int32 retcode ;
    retcode = SQL_EXEC_GetDiagnosticsCondInfo2(rowcondnum, i, &hv_rowindex, 0,0,0);

   printf("Condition number : %d\n", i);
   printf("ROW INDEX : %d\n", hv_rowindex);
   printf("SQLCODE : %d\n", hv_sqlcode);
   printf("SQLSTATE  : %s\n", hv_sqlstate);
   printf("MESSAGE : %s\n", hv_msgtxt);
   printf("TABLE   : %s\n", hv_tabname);
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
