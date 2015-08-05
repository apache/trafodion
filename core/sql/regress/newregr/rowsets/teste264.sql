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
 This test tests NAR support with indexes.
 * 
 *
 * 
 *
 */                                                    

/*  --multiple indexes
create table rownum10 (a int not null, b int not null, c int not null, d int not
 null, primary key (a));
create unique index rownum10idx1 on rownum10(b);
create unique index rownum10idx2 on rownum10(c);
create unique index rownum10idx3 on rownum10(d);
 */


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>


#define MINOF(X,Y) (X <= Y ? X : Y)
#define NAR00 30022
#define NAR01 30022
#define SIZE 100 
#define RATE 100 

void display_diagnosis(); 


EXEC SQL MODULE CAT.SCH.NARINT3
 NAMES ARE ISO88591;

/* globals */


EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [SIZE] Int32 a_int;
  ROWSET [SIZE] Int32 b_int;
  ROWSET [SIZE] Int32 c_int;
  ROWSET [SIZE] Int32 d_int;

  ROWSET [SIZE] char b_char[97];
  Int32 numRows ;
  Int32 loop, inp, inp1, trynum, rate, numiter;
  Int32 savesqlcode;
EXEC SQL END DECLARE SECTION;


EXEC SQL BEGIN DECLARE SECTION;  
/**** host variables for get diagnostics *****/
   NUMERIC(5) i;
   NUMERIC(5) hv_num;
   Int32 hv_sqlcode;
   Int32 hv_rowindex;
   Int32 hv_rowcount;
   char hv_msgtxt[256];
   char hv_sqlstate[6];
   char hv_tabname[129];
   char SQLSTATE[6];
   Int32 SQLCODE;
EXEC SQL END DECLARE SECTION;

exec sql whenever sqlerror call display_diagnosis;



Int32 main()
{


/* typedef long long int Int64; */


Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    b_int[i] = (i+1)*10;
    c_int[i] = (i+1)*100;
    d_int[i] = (i+1)*1000;

  
    }

   inp = 20;


  /* EXEC SQL CONTROL QUERY DEFAULT INSERT_VSBB 'OFF' ; */

  /*******************************************************************************************************/
 
  EXEC SQL DELETE FROM rownum10;

  // Expecting the whole rowset insert to succeed 

  EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO rownum10 VALUES (:a_int , :b_int, :c_int, :d_int) NOT ATOMIC;

    
	if (SQLCODE !=0)
          display_diagnosis();

	
  EXEC SQL COMMIT ; 

  
        


/***************************************************************************************/
 EXEC SQL DELETE FROM rownum10;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    b_int[i] = (i+1)*10;
    c_int[i] = (i+1)*100;
    d_int[i] = (i+1)*1000;

  
    }
 // Expecting error from basetable insert . Only 19 rows should be inserted 
printf("\n *** Expecting -8102 on row 10 ***\n");

  a_int[10] = 5; // Introduce a duplicate primary key 

  EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO rownum10 VALUES (:a_int , :b_int, :c_int, :d_int) NOT ATOMIC;

    
	if (SQLCODE !=0)
          display_diagnosis();

	
  EXEC SQL COMMIT ; 

  
        

 /*************************************************************************************/
  EXEC SQL DELETE FROM rownum10;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    b_int[i] = (i+1)*10;
    c_int[i] = (i+1)*100;
    d_int[i] = (i+1)*1000;

  
    }
 // Expecting error from first index  insert. Only 19 rows should be inserted 
 printf("\n *** Expecting -8102 on row 10 ***\n");


  a_int[10] = 200; // Introduce a duplicate unique index key value 
  b_int[10] = 110;
  c_int[10] = 200;
  d_int[10] = 200;

  EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO rownum10 VALUES (:a_int , :b_int, :c_int, :d_int) NOT ATOMIC;

    
	if (SQLCODE !=0)
          display_diagnosis();

	
  EXEC SQL COMMIT ; 
 
        

 /*************************************************************************************/

 EXEC SQL DELETE FROM rownum10;
 for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    b_int[i] = (i+1)*10;
    c_int[i] = (i+1)*100;
    d_int[i] = (i+1)*1000;

  
    }

 // Expecting errors from basetable and all indexes. Only 17 rows should be inserted  

  a_int[15] = 5; // Introduce a duplicate primary key 

  a_int[16] = 60; // Introduce a duplicate unique index key value in first index
  b_int[16] = 60;
  c_int[16] = 60;
  d_int[16] = 60;


  a_int[17] = 700; // Introduce a duplicate unique index key value in second index
  b_int[17] = 700;
  c_int[17] = 700;
  d_int[17] = 700;


  a_int[18] = 8000; // Introduce a duplicate unique index key value in third index
  b_int[18] = 8000;
  c_int[18] = 8000;
  d_int[18] = 8000;

printf("\n *** Expecting -8102 on row 15,16,17,18 ***\n");

  EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO rownum10 VALUES (:a_int , :b_int, :c_int, :d_int) NOT ATOMIC;

    
	if (SQLCODE !=0)
          display_diagnosis();

	
  EXEC SQL COMMIT ; 
  
        


 /*************************************************************************************/

 /* SET table tests */

 /*************************************************************************************/

 EXEC SQL DELETE FROM rownum10set;
 for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    b_int[i] = (i+1)*10;
    c_int[i] = (i+1)*100;
    d_int[i] = (i+1)*1000;

  
    }

 // Expecting no errors. Only 17 rows should be inserted  

  

  a_int[16] = 6; // Introduce a dup row at index 16
  b_int[16] = 60;
  c_int[16] = 600;
  d_int[16] = 6000;


  a_int[17] = 7; // Introduce a duplicate row at index 17
  b_int[17] = 70;
  c_int[17] = 700;
  d_int[17] = 7000;


  a_int[18] = 8; // Introduce a duplicate row at index 18
  b_int[18] = 80;
  c_int[18] = 800;
  d_int[18] = 8000;

printf("\n *** Expecting 0 errors .. 17 rows inserted ***\n");

  EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO rownum10set VALUES (:a_int , :b_int, :c_int, :d_int) NOT ATOMIC;

    
	if (SQLCODE !=0)
          display_diagnosis();

	
  EXEC SQL COMMIT ; 
  
        


 /*************************************************************************************/



 /*************************************************************************************/

 EXEC SQL DELETE FROM rownum10set;
 for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    b_int[i] = (i+1)*10;
    c_int[i] = (i+1)*100;
    d_int[i] = (i+1)*1000;

  
    }

 // Expecting no errors from basetable and some 8102 from indexes indexes. Only 17 rows should be inserted  

  a_int[15] = 5; // Introduce a dup row 
  b_int[15] = 50; 
  c_int[15] = 500;
  d_int[15] = 5000;
 

  a_int[16] = 60; // Introduce a duplicate unique index key value in first index
  b_int[16] = 60;
  c_int[16] = 60;
  d_int[16] = 60;


  a_int[17] = 700; // Introduce a duplicate unique index key value in second index
  b_int[17] = 700;
  c_int[17] = 700;
  d_int[17] = 700;


  a_int[18] = 8000; // Introduce a duplicate unique index key value in third index
  b_int[18] = 8000;
  c_int[18] = 8000;
  d_int[18] = 8000;

printf("\n *** Expecting -8102 on row 16,17,18 : 16 rows inserted ***\n");

  EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO rownum10set VALUES (:a_int , :b_int, :c_int, :d_int) NOT ATOMIC;

    
	if (SQLCODE !=0)
          display_diagnosis();

	
  EXEC SQL COMMIT ; 
  
        


 /*************************************************************************************/


 /*************************************************************************************/

 EXEC SQL DELETE FROM rownum10set;
 for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    b_int[i] = (i+1)*10;
    c_int[i] = (i+1)*100;
    d_int[i] = (i+1)*1000;

  
    }

 // Expecting one error from basetable . Only 18 rows should be inserted  

  a_int[15] = 5; // Introduce a duplicate  primary key 

  a_int[16] = 6; // Introduce a duplicate row
  b_int[16] = 60;
  c_int[16] = 600;
  d_int[16] = 6000;


  

printf("\n *** Expecting -8102 on row 15: 18 rows should be inserted ***\n");

  EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO rownum10set VALUES (:a_int , :b_int, :c_int, :d_int) NOT ATOMIC;

    
	if (SQLCODE !=0)
          display_diagnosis();

	
  EXEC SQL COMMIT ; 
  
        


 /*************************************************************************************/
   
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
   hv_msgtxt[255]='\0';
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
/*	  :hv_rowindex = ROW_INDEX,  */
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
   hv_msgtxt[255]='\0';
   memset(hv_tabname,' ',sizeof(hv_tabname));
   hv_tabname[128]='\0';
   memset(hv_sqlstate,' ',sizeof(hv_sqlstate));
   hv_sqlstate[6]='\0';
  }

   SQLCODE = savesqlcode ;
}
