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
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>


#define MINOF(X,Y) (X <= Y ? X : Y)
#define NAR00 30022
#define NAR01 30022
#define SIZE 1600 
#define RATE 100 

void display_diagnosis(); 
void testmulind();
void testRi();
void testmv1();
void testmv2();
void testmv3();
void testmv4();
void testmv4v();
void testmv5();



EXEC SQL MODULE CAT.SCH.TESTE266M
 NAMES ARE ISO88591;

/* globals */


EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [SIZE] Int32 a_int;
  ROWSET [SIZE] Int32 a1_int;
  ROWSET [SIZE] Int32 a2_int;
  ROWSET [SIZE] Int32 a3_int;
  ROWSET [SIZE] char b_char[97];
  ROWSET [SIZE] Int32 c1_int;
  ROWSET [SIZE] Int32 c2_int;
  ROWSET [SIZE] Int32 oint1;
  ROWSET [SIZE] Int32 oint2;
  ROWSET [SIZE] Int32 oint3;
  ROWSET [SIZE] Int32 oint4;


  
  Int32 numRows ;
  Int32 loop, inp, inp1, trynum, rate, numiter;
  Int32 savesqlcode;
EXEC SQL END DECLARE SECTION;


EXEC SQL BEGIN DECLARE SECTION;  
/**** host variables for get diagnostics *****/
   NUMERIC(5) j;
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
    testmulind();
    testRi();
    testmv1();
    testmv2();
    testmv3(); 
    testmv4();
    testmv4v();
    testmv5();



return(0);
}

/****************************************************************************************/
/*  DDL for table notatomic is

create table tabind (c1 int not null, c2 int , primary key (c1) ,foreign key (c2) references ct2 (d1) );
create unique index uitabind1  on tabind (c2);
create unique index uitabind2  on tabind (c2);
create unique index uitabind3  on tabind (c2);

insert into tabind values (400,400);
insert into tabnd values (800,800);
*/

 
 
void testmulind()
{


Int32 i=0;
for (i=0; i<SIZE; i++) {
    c1_int[i] = i+1;
    c2_int[i] = (i+1)*100;

    oint1[i] = 0;
    oint2[i] = 0;
    oint3[i] = 0;
    oint4[i] = 0;
    
    }

   inp = 20;
printf("\n *** Test multiple index - Expecting insert of all except 2 rows 3,7 to succeed ***\n");

  /* EXEC SQL CONTROL QUERY DEFAULT INSERT_VSBB 'OFF' ; */
 EXEC SQL delete from ct1;
 EXEC SQL delete from ct2;

 EXEC SQL DELETE from tabind;

 EXEC SQL insert into tabind values (400,400);
 EXEC SQL insert into tabind values (800,800);



    EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO tabind VALUES (:c1_int , :c2_int) NOT ATOMIC;

    /* if (SQLCODE != 0) {
          printf("  val = %d\n", a_int);
          return (0) ;    

         } */

    if (SQLCODE <0)
	{
       printf("SQLCODE = %d\n", SQLCODE);
		
	}
	else
	if (SQLCODE !=0)
		display_diagnosis();

	
	EXEC SQL COMMIT ;



EXEC SQL select * into :oint1, :oint2 from tabind order by c2;

printf("c1\tc2\n");
printf("------\t------\n");
for (i=0;i<20;i++)
{
   printf("%d\t",oint1[i]);
   printf("%d\t",oint2[i]);
   
   printf("\n");
}



 }
/****************************************************************************************/
/*  DDL for table notatomic is
create table ct2 ( d1 int not null, d2 int, primary key (d1));
create table ct1 (c1 int not null, c2 int , primary key (c1) ,foreign key (c2) references ct2 (d1) );
create unique index uit1  on ct1 (c2);
insert into ct2 values (400,400);
insert into ct2 values (1200,1200);
*/

 
 
void testRi()
{


Int32 i=0;
for (i=0; i<SIZE; i++) {
    c1_int[i] = i+1;
    c2_int[i] = (i+1)*100;

    oint1[i] = 0;
    oint2[i] = 0;
    oint3[i] = 0;
    oint4[i] = 0;
    
    }

   inp = 20;
printf("\n *** Test RI - Expecting insert of just 2 rows 3,11 to succeed ***\n");

  /* EXEC SQL CONTROL QUERY DEFAULT INSERT_VSBB 'OFF' ; */
 
 EXEC SQL delete from ct2;
 EXEC SQL delete from ct1;
 EXEC SQL insert into ct2 values (400,400);
 EXEC SQL insert into ct2 values (1200,1200);



    EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO ct1 VALUES (:c1_int , :c2_int) NOT ATOMIC;

    /* if (SQLCODE != 0) {
          printf("  val = %d\n", a_int);
          return (0) ;    

         } */

    if (SQLCODE <0)
	{
       printf("SQLCODE = %d\n", SQLCODE);
		
	}
	else
	if (SQLCODE !=0)
		display_diagnosis();

	
	EXEC SQL COMMIT ;



EXEC SQL select * into :oint1, :oint2 from ct1 order by c1;

printf("c1\tc2\n");
printf("------\t------\n");
for (i=0;i<20;i++)
{
   printf("%d\t",oint1[i]);
   printf("%d\t",oint2[i]);
   
   printf("\n");
}



 }
/********************************************************************************************/

/* To test a table that has unique index and an on request mv
 * Sandhya
 * 
 *
 * 
 *
 */                                                    

/*  DDL for table notatomic is
>>CREATE TABLE tab1_im (a INT NOT NULL, b INT, c INT, d INT NOT NULL NOT DROPPAB
LE,
+>        PRIMARY KEY (a, d) NOT DROPPABLE ) ;

--- SQL operation complete.
>>
>>
>>
>>create mv tab1_im_MV
+>        Refresh on request
+>        initialize on create
+>        as
+>        select a,sum (b) sum_b
+>        from tab1_im
+>        group by a;

--- SQL operation complete.
>>

create index itab_im_b on tab1_im (b);

create unique index i_tab1_im_c on tab1_im (c);


  */

void testmv1()
{

/* typedef long long int Int64; */


Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    a1_int[i] = (i+1)*10;
    a2_int[i] = (i+1)*100;
    a3_int[i] = (i+1)*1000;
   
    oint1[i] = 0;
    oint2[i] = 0;
    oint3[i] = 0;
    oint4[i] = 0;
   
    }

   inp = 40;

   a2_int[19] = 1000;
   a2_int[29] = 1500;

  /* EXEC SQL CONTROL QUERY DEFAULT INSERT_VSBB 'OFF' ; */
  EXEC SQL delete from tab1_im;
  
  printf("\n *** Test Index/MV - Expecting insert of all except 2 rows 19,29 to succeed ***\n");


    EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO tab1_im VALUES (:a_int , :a1_int, :a2_int, :a3_int ) NOT ATOMIC;

    /* if (SQLCODE != 0) {
          printf("  val = %d\n", a_int);
          return (0) ;    

         } */

    if (SQLCODE <0)
	{
       printf("SQLCODE = %d\n", SQLCODE);
		
	}
	else
	if (SQLCODE !=0)
		display_diagnosis();

	
	EXEC SQL COMMIT ;


EXEC SQL select * into :oint1, :oint2, :oint3, :oint4 from tab1_im order by c;

printf("a\tb\tc\td\n");
printf("------\t------\t------\t------\n");
for (i=0;i<40;i++)
{
   printf("%d\t",oint1[i]);
   printf("%d\t",oint2[i]);
   printf("%d\t",oint3[i]);
   printf("%d\t",oint4[i]);
   printf("\n");
}

   


}




void testmv5()
{

/* typedef long long int Int64; */


Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    a1_int[i] = (i+1)*10;
    a2_int[i] = (i+1)*100;
    a3_int[i] = (i+1)*1000;
   
    oint1[i] = 0;
    oint2[i] = 0;
    oint3[i] = 0;
    oint4[i] = 0;
   
    }

   inp = 40;

   a_int[10] = 1610;
   a_int[11] = 1610;

   a_int[20] = 1620;
   a_int[21] = 1620;

  /* EXEC SQL CONTROL QUERY DEFAULT INSERT_VSBB 'OFF' ; */
  EXEC SQL delete from tab1_dupkey;
  
  printf("\n *** Test MV with duplicate keys - Expecting insert of all except 2 rows 11,21 to succeed ***\n");


    EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO tab1_dupkey VALUES (:a_int , :a1_int, :a2_int, :a3_int ) NOT ATOMIC;


    if (SQLCODE <0)
	{
       printf("SQLCODE = %d\n", SQLCODE);
		
	}
	else
	if (SQLCODE !=0)
		display_diagnosis();

	
	EXEC SQL COMMIT ;


EXEC SQL select * into :oint1, :oint2, :oint3, :oint4 from tab1_dupkey order by c;

printf("a\tb\tc\td\n");
printf("------\t------\t------\t------\n");
for (i=0;i<40;i++)
{
   printf("%d\t",oint1[i]);
   printf("%d\t",oint2[i]);
   printf("%d\t",oint3[i]);
   printf("%d\t",oint4[i]);
   printf("\n");
}

   


}

/*******************************************************************************************/
/* To test a table that has an RI and an on request mv
 * Sandhya
 * 
 *
 * 
 *
 */                                                    

/*  DDL for table notatomic is

-- table with RI and MV
create table tab1_rm_ct2 ( d1 int not null, d2 int, primary key (d1));


CREATE TABLE tab1_rm (a INT NOT NULL, b INT, c INT, d INT NOT NULL NOT DROPPABLE
,
        PRIMARY KEY (a, d) NOT DROPPABLE, foreign key (c)
 references tab1_rm_ct2 (d1) ) ;



create mv tab1_rm_MV
        Refresh on request
        initialize on create
        as
        select a,sum (b) sum_b
        from tab1_rm
        group by a;

  */

void testmv2()
{

/* typedef long long int Int64; */


Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    a1_int[i] = (i+1)*10;
    a2_int[i] = (i+1)*100;
    a3_int[i] = (i+1)*1000;
    oint1[i] = 0;
    oint2[i] = 0;
    oint3[i] = 0;
    oint4[i] = 0;

   
    }

   inp = 20;

  printf("\n *** Test RI and MV - Expecting insert of just 3 rows 4,9,14 to succeed ***\n");


  /* EXEC SQL CONTROL QUERY DEFAULT INSERT_VSBB 'OFF' ; */
  EXEC SQL DELETE from tab1_rm;
  EXEC SQL DELETE from tab1_rm_ct2;

 


  // excpxet only 3 rows to be inserted.
  EXEC SQL insert into tab1_rm_ct2 values (500,500);
  EXEC SQL insert into tab1_rm_ct2 values (1000,1000);
  EXEC SQL insert into tab1_rm_ct2 values (1500,1500);
  

    EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO tab1_rm VALUES (:a_int , :a1_int, :a2_int, :a3_int ) NOT ATOMIC;

    /* if (SQLCODE != 0) {
          printf("  val = %d\n", a_int);
          return (0) ;    

         } */

    if (SQLCODE <0)
	{
       printf("SQLCODE = %d\n", SQLCODE);
		
	}
	else
	if (SQLCODE !=0)
		display_diagnosis();

	
	EXEC SQL COMMIT ;



EXEC SQL select * into :oint1, :oint2, :oint3, :oint4 from tab1_rm order by a;

printf("a\tb\tc\td\n");
printf("------\t------\t------\t------\n");
for (i=0;i<20;i++)
{
   printf("%d\t",oint1[i]);
   printf("%d\t",oint2[i]);
   printf("%d\t",oint3[i]);
   printf("%d\t",oint4[i]);
   printf("\n");
}

   
 

}

/***********************************************************************************************/
/* To test a table that has unique index and an RI and an on request mv
 * Sandhya
 * 
 *
 * 
 *
 */                                                    

/*  DDL 


CREATE TABLE tab1_irm (a INT NOT NULL, b INT, c INT, d INT NOT NULL NOT DROPPABLE,
        PRIMARY KEY (a, d) NOT DROPPABLE, foreign key (c)
 references ct2 (d1) ) ;




create mv tab1_irm_MV
        Refresh on request
        initialize on create
        as
        select a,sum (b) sum_b
        from tab1_irm
        group by a;

create index i_tab1_irm_b on tab1_irm (b);

create unique index i_tab1_irm_c on tab1_irma (c);


create table tab1_irm_ct2 ( d1 int not null, d2 int, primary key (d1));
  */

void  testmv3()
{

/* typedef long long int Int64; */


Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    a1_int[i] = (i+1)*10;
    a2_int[i] = (i+1)*100;
    a3_int[i] = (i+1)*1000;
    oint1[i] = 0;
    oint2[i] = 0;
    oint3[i] = 0;
    oint4[i] = 0;

  
    }

   inp = 20;
 printf("\n *** Test RI/MV/Index - Expecting insert of just 3 rows 3,11,15 to succeed ***\n");


  /* EXEC SQL CONTROL QUERY DEFAULT INSERT_VSBB 'OFF' ; */
   
  EXEC SQL DELETE FROM tab1_irm;
  EXEC SQL DELETE FROM tab1_irm_ct2;
  EXEC SQL insert into tab1_irm_ct2 values (400,400);
  EXEC SQL insert into tab1_irm_ct2 values (1200,1200);
  EXEC SQL insert into tab1_irm_ct2 values (1600,1600);

  a2_int[8] = 400; // satisfies RI constraint but raises 8102 index violation
 
    
  
    EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO tab1_irm VALUES (:a_int , :a1_int, :a2_int, :a3_int ) NOT ATOMIC;

    /* if (SQLCODE != 0) {
          printf("  val = %d\n", a_int);
          return (0) ;    

         } */

    if (SQLCODE <0)
	{
       printf("SQLCODE = %d\n", SQLCODE);
		
	}
	else
	if (SQLCODE !=0)
		display_diagnosis();

	
	EXEC SQL COMMIT ;
EXEC SQL select * into :oint1, :oint2, :oint3, :oint4 from tab1_irm order by c;

printf("a\tb\tc\td\n");
printf("------\t------\t------\t------\n");
for (i=0;i<20;i++)
{
   printf("%d\t",oint1[i]);
   printf("%d\t",oint2[i]);
   printf("%d\t",oint3[i]);
   printf("%d\t",oint4[i]);
   printf("\n");
}





   
  

}

/************************************************************************************************/
/***********************************************************************************************/
/* To test a table that has unique index and an RI an on statement MV and an on request mv
 * Sandhya
 * 
 *
 * 
 *
 */                                                    

/*  DDL 

--table with on statement mv, on request mv, RI and index
create table dt2 ( d1 int not null, d2 int, primary key (d1));

CREATE TABLE tab1_irm2 (a INT NOT NULL, b INT, c INT not NULL NOT DROPPABLE , d INT NOT NULL NOT DROPPABLE,
        PRIMARY KEY (a, d) NOT DROPPABLE, foreign key (c)
 references dt2 (d1) ) ATTRIBUTE AUTOMATIC RANGELOG;



create mv tab1_irm2_or_MV
        Refresh on request
        initialize on create
        as
        select a,sum (b) sum_b
        from tab1_irm2
        group by a;

create unique index itab_irm2_c on tab1_irm2 (c);

create mv tab1_irm2_os_MV
        refresh on statement
        initialize on create
        store by (d)
        as
        select d
        from tab1_irm2;
******************************************************************************/
void  testmv4()
{

/* typedef long long int Int64; */


Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    a1_int[i] = (i+1)*10;
    a2_int[i] = (i+1)*100;
    a3_int[i] = (i+1)*1000;
    oint1[i] = 0;
    oint2[i] = 0;
    oint3[i] = 0;
    oint4[i] = 0;

  
    }

   inp = 20;
 printf("\n *** Test RI/On request MV/On Statement MV/Index - Expecting insert\n of just 6 rows  to succeed . Expect 8103  errors on indexes 10 - 19 and \n 8102 errors on 2,4,3 and 6***\n");

   EXEC SQL control query default comp_bool_93 'off';
   EXEC SQL delete from tab1_irm2;
   EXEC SQL delete from dt2;

   EXEC SQL CONTROL QUERY DEFAULT INSERT_VSBB 'OFF' ; 
   
   EXEC SQL insert into dt2 values (100,100);
   EXEC SQL insert into dt2 values (200,200);
   EXEC SQL insert into dt2 values (300,300);
   EXEC SQL insert into dt2 values (400,400);
   EXEC SQL insert into dt2 values (500,500);
   EXEC SQL insert into dt2 values (600,600);
   EXEC SQL insert into dt2 values (700,700);
   EXEC SQL insert into dt2 values (800,800);
   EXEC SQL insert into dt2 values (900,900);
   EXEC SQL insert into dt2 values (1000,1000);

   EXEC SQL insert into tab1_irm2 values (3,30,300,3000);
   EXEC SQL insert into tab1_irm2 values (33,33,400,33);
   EXEC SQL insert into tab1_irm2 values (5,50,500,5000);
   EXEC SQL insert into tab1_irm2 values (36,36,700,36);
 
 
  
    EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO tab1_irm2 VALUES (:a_int , :a1_int, :a2_int, :a3_int ) NOT ATOMIC;

   if (SQLCODE <0)
	{
       printf("SQLCODE = %d\n", SQLCODE);
		
	}
	else
	if (SQLCODE !=0)
		display_diagnosis();

	
	EXEC SQL COMMIT ;
EXEC SQL select * into :oint1, :oint2, :oint3, :oint4 from tab1_irm2 order by c;

printf("a\tb\tc\td\n");
printf("------\t------\t------\t------\n");
for (i=0;i<20;i++)
{
   printf("%d\t",oint1[i]);
   printf("%d\t",oint2[i]);
   printf("%d\t",oint3[i]);
   printf("%d\t",oint4[i]);
   printf("\n");
}
  

}
/* The VSBB version of the same test as testmv4 */
void  testmv4v()
{

/* typedef long long int Int64; */


Int32 i=0;
for (i=0; i<SIZE; i++) {
    a_int[i] = i+1;
    a1_int[i] = (i+1)*10;
    a2_int[i] = (i+1)*100;
    a3_int[i] = (i+1)*1000;
    oint1[i] = 0;
    oint2[i] = 0;
    oint3[i] = 0;
    oint4[i] = 0;

  
    }

   inp = 20;
 printf("\n *** VSBB Test RI/On request MV/On Statement MV/Index - Expecting insert\n of just 6 rows  to succeed . Expect 8103  errors on indexes 10 - 19 and \n 8102 errors on 2,4,3 and 6***\n");
    
   EXEC SQL control query default comp_bool_93 'off';
   EXEC SQL delete from tab1_irm2;
   EXEC SQL delete from dt2;

   /*EXEC SQL CONTROL QUERY DEFAULT INSERT_VSBB 'OFF' ; */
   
   EXEC SQL insert into dt2 values (100,100);
   EXEC SQL insert into dt2 values (200,200);
   EXEC SQL insert into dt2 values (300,300);
   EXEC SQL insert into dt2 values (400,400);
   EXEC SQL insert into dt2 values (500,500);
   EXEC SQL insert into dt2 values (600,600);
   EXEC SQL insert into dt2 values (700,700);
   EXEC SQL insert into dt2 values (800,800);
   EXEC SQL insert into dt2 values (900,900);
   EXEC SQL insert into dt2 values (1000,1000);

   EXEC SQL insert into tab1_irm2 values (3,30,300,3000);
   EXEC SQL insert into tab1_irm2 values (33,33,400,33);
   EXEC SQL insert into tab1_irm2 values (5,50,500,5000);
   EXEC SQL insert into tab1_irm2 values (36,36,700,36);
 
 
  
    EXEC SQL 
	 ROWSET FOR INPUT SIZE :inp
	 INSERT INTO tab1_irm2 VALUES (:a_int , :a1_int, :a2_int, :a3_int ) NOT ATOMIC;

   if (SQLCODE <0)
	{
       printf("SQLCODE = %d\n", SQLCODE);
		
	}
	else
	if (SQLCODE !=0)
		display_diagnosis();

	
	EXEC SQL COMMIT ;
EXEC SQL select * into :oint1, :oint2, :oint3, :oint4 from tab1_irm2 order by c;

printf("a\tb\tc\td\n");
printf("------\t------\t------\t------\n");
for (i=0;i<20;i++)
{
   printf("%d\t",oint1[i]);
   printf("%d\t",oint2[i]);
   printf("%d\t",oint3[i]);
   printf("%d\t",oint4[i]);
   printf("\n");
}
  

}
/************************************************************************************************/
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

  for (j = 1; j <= hv_num; j++) {
      exec sql get diagnostics exception :j                
          :hv_tabname = TABLE_NAME,
          :hv_sqlcode = SQLCODE,
	  :hv_sqlstate = RETURNED_SQLSTATE,
/*	  :hv_rowindex = ROW_INDEX,  */
          :hv_msgtxt = MESSAGE_TEXT;

   retcode = SQL_EXEC_GetDiagnosticsCondInfo2(rowcondnum, j, &hv_rowindex, 0,0,0);

   printf("Condition number : %d\n", j);
   printf("ROW INDEX : %d\n", hv_rowindex);
   printf("SQLCODE : %d\n", hv_sqlcode);
   printf("SQLSTATE  : %s\n", hv_sqlstate);
   printf("MESSAGE : %s\n", hv_msgtxt);
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

