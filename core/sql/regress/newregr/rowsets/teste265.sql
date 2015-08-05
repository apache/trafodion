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
/*
 This tests NAR support with indexes and a composite primary key
 *
 *
 */

/*  --multiple indexes plus a composite PK
create table rownum265 (a int not null, b int not null, c int not null,  primary key (b, c));
create unique index rownum265idx1 on rownum265(a);
create unique index rownum265idx2 on rownum265(c);
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

#define MINOF(X,Y) (X <= Y ? X : Y)
#define SIZE 20

void display_diagnosis();
void delete_table_data_init_insert_hvs();
void init_select_hvs();
void display_table_data();
void execute_sql();
Int32 test1();
Int32 test2();
Int32 test3();
Int32 test4();
Int32 test5();
Int32 test6();

EXEC SQL MODULE CAT.SCH.TESTE265M NAMES ARE ISO88591;

/* globals */

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [SIZE] Int32 a_int;
  ROWSET [SIZE] Int32 b_int;
  ROWSET [SIZE] Int32 c_int;

  ROWSET [SIZE] Int32 a_int2;
  ROWSET [SIZE] Int32 b_int2;
  ROWSET [SIZE] Int32 c_int2;

  Int32 numRows;
  Int32 inp = 20;
  Int32 savesqlcode;
EXEC SQL END DECLARE SECTION;


EXEC SQL BEGIN DECLARE SECTION;
/**** host variables for get diagnostics *****/
   NUMERIC(5) i;
   NUMERIC(5) hv_num;
   Int32 hv_sqlcode;
   Int32 hv_rowindex;
   Int32 hv_rowcount;
   char hv_msgtxt[257];
   char hv_sqlstate[6];
   char hv_tabname[129];
   char SQLSTATE[6];
   Int32 SQLCODE;
EXEC SQL END DECLARE SECTION;

/*exec sql whenever sqlerror call display_diagnosis; */

Int32 main()
{
   /*  turn off NAR */
  /*  EXEC SQL CONTROL QUERY DEFAULT NAR_DEPOBJ_ENABLE 'OFF'; */

  test1();
  test2();
  test3();
  test4();
  test5();
  test6();

  return(0);
}

/***************************************************************************************/
void execute_sql()
{
  EXEC SQL
	  ROWSET FOR INPUT SIZE :inp
	 INSERT INTO rownum265 VALUES (:a_int, :b_int, :c_int) NOT ATOMIC;

  if (SQLCODE != 0)
      display_diagnosis();

  EXEC SQL COMMIT ;

}

/***************************************************************************************/
Int32 test1()
{
  printf("\n *** Test 1 - Expecting entire rowset insert to succeed ***\n");

  delete_table_data_init_insert_hvs();

  execute_sql();
  display_table_data();
  return(0);
}

/***************************************************************************************/
Int32 test2()
{
  printf("\n *** Test 2 - Expecting -8102 on row 10         ***\n");
  printf("              Duplicate pk on column b and c    ***\n");
  printf(" ***          Only 19 rows should be inserted   ***\n");

  delete_table_data_init_insert_hvs();

  b_int[9] = 50; /* Introduce a duplicate primary key */
  c_int[9] = 500;

  execute_sql();
  display_table_data();
  return(0);
}

/*************************************************************************************/
Int32 test3()
{
  printf("\n *** Test 3 - Expecting -8102 on row 11            ***\n");
  printf(" ***          Duplicate unique index key on col a  ***\n");
  printf(" ***          Only 19 rows should be inserted      ***\n");

  delete_table_data_init_insert_hvs();

  a_int[10] = 6; /* Introduce a duplicate unique index key value */

  execute_sql();
  display_table_data();
  return (0);
}

/*************************************************************************************/
Int32 test4()
{
  printf("\n *** Test 4 - Expecting -8102 on row 12 and row 13       ***\n");
  printf(" ***          Duplicate unique index key on cols a and c ***\n");
  printf(" ***          Only 18 rows should be inserted            ***\n");

  delete_table_data_init_insert_hvs();

  a_int[11] = 6;   /* Introduce a duplicate unique index key value */
  c_int[12] = 700; /* Introduce a duplicate unique index key value */

  execute_sql();
  display_table_data();
  return (0);
}

/*************************************************************************************/
Int32 test5()
{
  printf("\n *** Test 5 - Expecting -8102 on row 14 and 15                               ***\n");
  printf(" ***          Duplicate unique index key on col a and duplicate primary keys ***\n");
  printf(" ***          Only 18 rows should be inserted                                ***\n");

  delete_table_data_init_insert_hvs();

  a_int[13] = 3; /* Introduce a duplicate unique index key in first index (column a) */

  b_int[14] = 40;  /* Introduce a duplicate primary key                   */
  c_int[14] = 400; /* (Note: primary key composed of both column b and c) */

  execute_sql();
  display_table_data();
  return (0);
}

/*************************************************************************************/
Int32 test6()
{
  printf("\n *** Test 6 - Expecting -8102 on row 16, 17, and 18                                  ***\n");
  printf(" ***          Duplicate unique index keys on col a & c plus duplicate primary keys   ***\n");
  printf(" ***          Only 17 rows should be inserted                                        ***\n");

  delete_table_data_init_insert_hvs();

  a_int[15] = 5; /* Introduce a duplicate unique index key in first index (column a) */

  b_int[16] = 60;  /* Introduce a duplicate primary key                   */
  c_int[16] = 600; /* (Note: primary key composed of both column b and c) */

  c_int[17] = 700; /* Introduce a duplicate unique index key value in second index (column c) */

  execute_sql();
  display_table_data();
  return (0);
}

/***************************************************/
void delete_table_data_init_insert_hvs()
/***************************************************/
{
 printf("Delete table data\n");
 EXEC SQL DELETE FROM rownum265;
 EXEC SQL COMMIT;

 Int32 i=0;
 for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    b_int[i] = (i+1)*10;
    c_int[i] = (i+1)*100;
    }
}

/***************************************************/
void init_select_hvs()
/***************************************************/
{
 Int32 i=0;
 for (i=0; i<SIZE; i++) {
    a_int2[i] = 0;
    b_int2[i] = 0;
    c_int2[i] = 0;
    }
}

/***************************************************/
void display_table_data()
/***************************************************/
{
  init_select_hvs();

  printf("Verify the contents of base table\n");

  EXEC SQL select * into :a_int2, :b_int2, :c_int2 from rownum265 order by b;

  printf("a\tb\tc\n");
  printf("------\t------\t------\n");
  for (i=0;i<SIZE;i++)
  {
     printf("%d\t",a_int2[i]);
     printf("%d\t",b_int2[i]);
     printf("%d\t",c_int2[i]);
     printf("\n");
  }
}

/*****************************************************/
void display_diagnosis()
/*****************************************************/
{
  printf("Start display diags\n");

  Int32 rowcondnum = 103;
  Int32 retcode ;
  savesqlcode = SQLCODE ;
  hv_rowcount = -1 ;
  hv_rowindex = -2 ;
  exec sql get diagnostics :hv_num = NUMBER,
		:hv_rowcount = ROW_COUNT;

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[256]='\0';
   memset(hv_sqlstate,' ',sizeof(hv_sqlstate));
   hv_sqlstate[6]='\0';

   printf("Number of conditions  : %d\n", hv_num);
   printf("Number of rows inserted: %d\n", hv_rowcount);
   printf("\n");

   for (i = 1; i <= hv_num; i++) {
      exec sql get diagnostics exception :i
          :hv_tabname = TABLE_NAME,
          :hv_sqlcode = SQLCODE,
	  :hv_sqlstate = RETURNED_SQLSTATE,
/*	  :hv_rowindex = ROW_INDEX, */
          :hv_msgtxt = MESSAGE_TEXT;

   retcode = SQL_EXEC_GetDiagnosticsCondInfo2(rowcondnum, i, &hv_rowindex, 0,0,0);

   printf("Condition number : %d\n", i);
   printf("ROW INDEX : %d\n", hv_rowindex);
   printf("SQLCODE : %d\n", hv_sqlcode);
   printf("SQLSTATE  : %s\n", hv_sqlstate);
   printf("MESSAGE : %s\n", hv_msgtxt);
   printf("TABLE   : %s\n", hv_tabname);
   printf("\n");

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[256]='\0';
   memset(hv_tabname,' ',sizeof(hv_tabname));
   hv_tabname[128]='\0';
   memset(hv_sqlstate,' ',sizeof(hv_sqlstate));
   hv_sqlstate[6]='\0';
  }

   SQLCODE = savesqlcode;
   printf("End display diags; SQLCODE = %d\n", SQLCODE);
}
