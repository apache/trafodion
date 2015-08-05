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
/* teste263.sql
 * Sandhya Sundaresan
 * 04-16-2007
 *
 * embedded C tests for Non Atomic Rowsets with index support
 *   Test Classification: Positive
 *   Test Level: Functional
 *   Test Coverage:
 *   VSBB Non-atomic rowset insert tests
 *   (This is basically teste251 but the table has indexes) 
 */                                                    

/*  DDL for table snt1 and view sntv1 is
CREATE TABLE snt1 (id int not null, id2 int,
eventtime timestamp, 
description varchar(12), 
primary key (id), check (id > 0)) ;   

create index isnt1 on snt1 (id2);

CREATE VIEW sntv1 (v_id, v_eventtime, v_description) AS
SELECT id, eventtime, description from snt1
WHERE id > 100
WITH CHECK OPTION ;

*/


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define NAR00 30022
#define SIZE 500


void display_diagnosis();
void display_diagnosis3();
void unexpected_error();
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


EXEC SQL MODULE CAT.SCH.TESTE251M NAMES ARE ISO88591;

/* globals */


EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [SIZE] Int32 a_int;
  ROWSET [SIZE] VARCHAR b_char5[5];
  ROWSET [SIZE] TIMESTAMP b_time;
  ROWSET [SIZE] Int32 j;
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
   
  return(0);

}


Int32 test1()
{
printf("\n ***TEST1 : Expecting -8101 on rows 95 & 399***\n");

exec sql whenever sqlerror call unexpected_error ;

EXEC SQL DELETE FROM snt1 ;


EXEC SQL COMMIT ;


Int32 i=0;
for (i=0; i<SIZE; i++)
    a_int[i] = i+1;

a_int[95] = -1 ;
a_int[399] = -1 ;
 
  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test2()
{
printf("\n ***TEST2 : Expecting -8101 on rows 0 & 499***\n");

exec sql whenever sqlerror call unexpected_error ;
EXEC SQL DELETE FROM snt1 ;


EXEC SQL COMMIT ;


Int32 i=0;
for (i=0; i<SIZE; i++)
    a_int[i] = i+1;

a_int[0] = -1 ;
a_int[499] = -1 ;
 
  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test3()
{
printf("\n ***TEST3 : Expecting -8101 on all rows except rows 92***\n");

exec sql whenever sqlerror call unexpected_error ;
EXEC SQL DELETE FROM snt1 ;

EXEC SQL COMMIT ;

Int32 i=0;
for (i=0; i<SIZE; i++)
    a_int[i] = -(i+1);

a_int[92] = 93 ;
 
  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test4()
{
printf("\n ***TEST4 : Expecting -8101 on rows 110 & 112, and -8102 on rows 109 & 111***\n");

exec sql whenever sqlerror call unexpected_error ;
EXEC SQL DELETE FROM nt1 ;

EXEC SQL COMMIT ;

Int32 i=0;
for (i=0; i<SIZE; i++)
    a_int[i] = i+1;

a_int[109] = 1;
a_int[110] = -1 ; 
a_int[111] = 1;
a_int[112] = -1;
 
  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test5()
{
printf("\n ***TEST5 : Expecting -8105 on row 401***\n");

exec sql whenever sqlerror call unexpected_error ;
EXEC SQL DELETE FROM snt1 ;

EXEC SQL COMMIT ;

Int32 i=0;
for (i=0; i<SIZE; i++)
    a_int[i] = i+101;

a_int[401] = 22 ;
 
  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO sntv1 VALUES (:a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?') NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test6()
{
printf("\n ***TEST6 : Expecting -8403 on row 481***\n");

exec sql whenever sqlerror call unexpected_error ;
EXEC SQL DELETE FROM snt1 ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    } 


   a_int[481] = -5;
 
  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp), 
  SUBSTRING('how are you?', 1, :a_int ))  NOT ATOMIC ;
 
  if (SQLCODE < 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test7()
{
printf("\n ***TEST7 : Expecting -8411 on row 12***\n");
/* error raised in PA node */

exec sql whenever sqlerror call unexpected_error ;
EXEC SQL DELETE FROM snt1 ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    } 

a_int[12] = 2000000000 ;

  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO snt1 
  VALUES (:a_int + 2000000000, :a_int, 
  cast ('05.01.1997 03.04.55.123456' as timestamp), 
  'how are you?')  NOT ATOMIC  ;
 
  if (SQLCODE < 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test8()
{
printf("\n ***TEST8 : Expecting -8411 on all rows except row 12***\n");
/* error raised in PA node. Uses GetCondInfo3 to get diagnostics in an array*/

exec sql whenever sqlerror call unexpected_error ;
EXEC SQL DELETE FROM snt1 ;

Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 2000000000 ;;
    } 

 a_int[12] = 13 ; 

  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO snt1 
  VALUES (:a_int + 2000000000, :a_int, 
  cast ('05.01.1997 03.04.55.123456' as timestamp), 
  'how are you?')  NOT ATOMIC  ;
 
  if (SQLCODE < 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis3();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test9()
{
printf("\n ***TEST9 : Expecting -8412 on row 255***\n");
exec sql whenever sqlerror call unexpected_error ;
EXEC SQL DELETE FROM snt1 ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    strcpy(b_char5[i], "you?");
    } 

    memset(b_char5[255], '?', 5);


 
  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are ' || :b_char5)  NOT ATOMIC  ;
 
  if (SQLCODE < 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test10()
{
printf("\n ***TEST10 : Expecting -8415 on row 489***\n");
exec sql whenever sqlerror call unexpected_error ;
EXEC SQL DELETE FROM snt1 ;
Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    strcpy(b_time[i], "05.01.1997 03.04.55.123456");
    } 



strcpy(b_time[489], "aaaaaaaaaaaaaaaaa");
  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO snt1 
  VALUES (:a_int, :a_int, 
  CONVERTTIMESTAMP(JULIANTIMESTAMP(:b_time)), 
  'how are you?') NOT ATOMIC ;
 
  if (SQLCODE < 0) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test11()
{
printf("\n ***TEST11 : Expecting -8419 on rows 1,2,3,4,5,391,392,393,394, & 395***\n");
 exec sql whenever sqlerror call unexpected_error ;
 EXEC SQL DELETE FROM snt1 ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    j[i] = 1;
}

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
 
  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int/:j, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test12()
{
printf("\n ***TEST12 : Expecting -8102 on all rows except row 0***\n");
 exec sql whenever sqlerror call unexpected_error ;
 EXEC SQL DELETE FROM snt1 ;
i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = 1;
}



 
  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO snt1 VALUES (:a_int, :a_int, cast( '05.01.1997 03.04.55.123456' as timestamp),'how are you?')  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

Int32 test13()
{
/*
This testcase forces a lock timeout on the access to a 
referenced primary key in an RI constraint.  The error
should be fatal.
*/

printf("\n ***TEST13 : Expecting -8551 fatal error. ***\n");

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [SIZE] Int32 b_int;
EXEC SQL END DECLARE SECTION;


exec sql whenever sqlerror call unexpected_error ;

EXEC SQL DELETE FROM ri_fatal_error_dependent ;

EXEC SQL DELETE FROM ri_fatal_error_referenced ;

EXEC SQL INSERT INTO ri_fatal_error_referenced values( 10, 10 );

EXEC SQL INSERT INTO ri_fatal_error_referenced values( 410, 410 );

EXEC SQL COMMIT ;

  EXEC SQL BEGIN DECLARE SECTION;
    TIMESTAMP(6) hv_schedColumn;
  EXEC SQL END DECLARE SECTION;

  exec sql select current_timestamp into :hv_schedColumn from (values(0)) as t1;

printf("The time is filtered FUNKY_OPT_UNIQUE %s\n", hv_schedColumn);

system( "echo "
        "log background263.log clear\\; "
        "begin work\\; "
        "update ri_fatal_error_referenced SET B = 410 WHERE A = 410\\; "
        "select \\'Filtered time FUNKY_OPT_UNIQUE\\' , "
        "current_timestamp from \\(values\\(0\\)\\) as t1\\; "
#ifdef _WIN32
        "sh Sleep 90000\\; "
#else
        "sh sleep 90\\; "
#endif
        "commit\\; "
        "log\\; "
        "exit\\; "
        " > lock_referenced");

system("sqlci -i lock_referenced > /dev/null &");

printf("waiting 30 seconds for a background sqlci to start and\n");
printf("lock a row in ri_fatal_error_referenced table. Hope that\n");
printf("30 seconds is long enough.\n");
#ifdef _WIN32
  Sleep (30000);
#else
  sleep (30);
#endif


Int32 i=0;
for (i=0; i<SIZE; i++)
{
    a_int[i] = i+1;
    b_int[i] = 10;
}

b_int[12] = 0 ;
b_int[410] = 410 ;
b_int[412] = 0 ;
 
  exec sql whenever sqlerror continue;
  EXEC SQL
  INSERT INTO ri_fatal_error_dependent VALUES (:a_int, :b_int)  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    exec sql whenever sqlerror call unexpected_error ;
    EXEC SQL COMMIT ;
  }
   
  return(0);

}

/*****************************************************/
void unexpected_error()
/*****************************************************/
{
  printf("Unexpected error.\n");
  display_diagnosis();
  exit(1);
}

/*****************************************************/
void unexpected_error_in_error_handler()
/*****************************************************/
{
  printf("Unexpected error in error handler.\n");
  printf("SQLCODE = %d.\n", SQLCODE);
  exit(1);
}

EXEC SQL WHENEVER SQLERROR CALL unexpected_error_in_error_handler;
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
