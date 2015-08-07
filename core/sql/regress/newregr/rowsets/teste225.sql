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
// +++ Code modified on 2003/8/29
**********************************************************************/
/* Test for dynamic rowsets */

/* create table dynamic5 ( a char(3), b int ) ; */
                    
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "defaultSchema.h"

void display_diagnosis();
void populateInputHostvars();
void printRowsFromDynamic5();
void displayRowsetRowCountArray();
void dynamic_cast1();
void dynamic_constant();
void dynamic_delete();
void dynamic_direct();
void dynamic_expression();
void dynamic_inputsize();
void dynamic_mix1();
void dynamic_mix2();
void dynamic_select1();
void dynamic_select2();
void dynamic_cursor1();
void dynamic_cursor2();


EXEC SQL MODULE CAT.SCH.ROWSETDYNTEST1 NAMES ARE ISO88591;

EXEC SQL BEGIN DECLARE SECTION;  
/**** host variables for get diagnostics *****/
   NUMERIC(5) i;
   NUMERIC(5) hv_num;
   Int32 hv_sqlcode;
   char hv_msgtxt[129];
   char hv_colname[129];
   char hv_tabname[129];
   char hv_cmdfcn[129];
   char hv_curname[129];
   char SQLSTATE[6];
   Int32 SQLCODE;
EXEC SQL END DECLARE SECTION;
   
EXEC SQL BEGIN DECLARE SECTION;

ROWSET[10] char a_arr[4];
ROWSET[10] Int32 b_arr;
ROWSET[10] short b_arr_ind;
Int32 b_int;
char a_char[4];
ROWSET[10] char aout_arr[4];
ROWSET[10] Int32 bout_arr;
ROWSET[10] Int32 rowid_arr;

EXEC SQL END DECLARE SECTION;

EXEC SQL BEGIN DECLARE SECTION;
	char  selectBuffer[390];
        char  out_desc[13], in_desc[13], data_name[30];
	char  statementBuffer[390];     
        Int32  data_type, col_num;  
        Int32  out_degree,num_out,num_in,j,rows_fetched;
	Int32  output_rowset_size, num, step_size, indicator_size;
	long   arr_ptr, ind_ptr;
	Int32  arr_size, ind_size, char_size, count;

EXEC SQL END DECLARE SECTION;

char defaultSchema[MAX_DEFAULT_SCHEMA_LEN + 1];

exec sql whenever sqlerror call display_diagnosis;
exec sql whenever sql_warning continue;

#include "defaultSchema.cpp"

Int32 main(Int32 argc, char ** argv)
{
    char * regrType = (argc > 0) ? argv[1] : 0 ;

    setDefaultSchema(defaultSchema, MAX_DEFAULT_SCHEMA_LEN, 0);
    sprintf(statementBuffer, "set schema %s;", defaultSchema);
    exec sql execute immediate :statementBuffer;

    /* regrType 1 => MX,  2 => MP , if no arg. then MP is assumed */
    if (regrType && (regrType[0] == '1')) {
      sprintf(statementBuffer, "declare schema '%s'", defaultSchema);
      exec sql execute immediate :statementBuffer;
    }
    dynamic_cast1();
    dynamic_constant();
    dynamic_delete();
    dynamic_direct();
    dynamic_expression();
    dynamic_inputsize();
    dynamic_mix1();
    dynamic_mix2();
    dynamic_select1();
    dynamic_select2();
    dynamic_cursor1();
    dynamic_cursor2();

    return 0;
}

/*****************************************************/
void dynamic_cast1()
/*****************************************************/
{
   /* Initialize all variables */
    printf("DYNAMIC_CAST:\n");  
    strcpy(in_desc,"inscols     "); 
    SQLSTATE[5] = '\0';
    memset(statementBuffer, ' ', 390);
    statementBuffer[389] = '\0';
    output_rowset_size = 0;

    EXEC SQL DELETE FROM DYNAMIC5;

    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('aaa',0), ('aaa',1), ('aaa',2), ('aaa', 3);
    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('aaa',4), ('aaa',5), ('aaa',6), ('aaa', 7);
    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('aaa',8), ('aaa',9), ('aaa',10), ('aaa', 10);

    sprintf(statementBuffer, "CONTROL QUERY DEFAULT ROWSET_ROW_COUNT 'ON';");
    EXEC SQL EXECUTE IMMEDIATE :statementBuffer  ;

    printf("prepare update with cast:\n");  
    /* variable to hold source from UPDATE statement */
    strcpy(statementBuffer,
    "UPDATE DYNAMIC5 SET A = 'bbb' WHERE B = CAST (?[10] AS INT) ;" );

    printf("SQLCODE after prepare is %d\n", SQLCODE);
    EXEC SQL PREPARE S1 FROM :statementBuffer; 

    num_in = 30;
    /* create SQLDA for UPDATE columns */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :in_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);

    /* populate the SQLDA */
    EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc;
    printf("SQLSTATE after describe is %s\n", SQLSTATE);


    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = ROWSET_SIZE;
    printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);

    populateInputHostvars();

    EXEC SQL EXECUTE S1 USING :b_arr ;

    displayRowsetRowCountArray();

    EXEC SQL COMMIT ;

    printRowsFromDynamic5();

    EXEC SQL DELETE FROM DYNAMIC5 ;
    EXEC SQL DEALLOCATE DESCRIPTOR :in_desc ;
    EXEC SQL DEALLOCATE PREPARE S1;



}

/*****************************************************/
void dynamic_constant()
/*****************************************************/
{                                 
    /* Initialize all variables */
    printf("DYNAMIC_CONSTANT:\n");  
    strcpy(in_desc,"inscols     "); 
    SQLSTATE[5] = '\0';
    memset(statementBuffer, ' ', 390);
    statementBuffer[389] = '\0';
    output_rowset_size = 0;

    EXEC SQL DELETE FROM DYNAMIC5;

    /* INSERTING 10 ROWS */

    printf("prepare insert with a constant:\n");  
    /* variable to hold source from INSERT statement */
    strcpy(statementBuffer,
    "INSERT INTO DYNAMIC5 VALUES ( 'tom', ?[10] );" );
    /* construct S1 from of INSERT statement */
    EXEC SQL PREPARE S0 FROM :statementBuffer; 
    printf("SQLSTATE after prepare is %s\n", SQLSTATE);
    printf("SQLCODE after prepare is %d\n", SQLCODE);
    EXEC SQL PREPARE S1 FROM :statementBuffer; 

    num_in = 30;
    /* create SQLDA for INSERT columns */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :in_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);

    /* populate the SQLDA */
    EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc;
    printf("SQLSTATE after describe is %s\n", SQLSTATE);


    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = ROWSET_SIZE;
    printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);

    populateInputHostvars();

    EXEC SQL EXECUTE S1 USING :b_arr ;

    EXEC SQL COMMIT ;

    printRowsFromDynamic5();

    EXEC SQL DELETE FROM DYNAMIC5;
    EXEC SQL DEALLOCATE DESCRIPTOR :in_desc ;
    EXEC SQL DEALLOCATE PREPARE S1;

}

/*****************************************************/
void dynamic_delete()
/*****************************************************/
{
    /* Initialize all variables */
    printf("DYNAMIC_DELETE:\n");  
    strcpy(in_desc,"inscols     "); 
    SQLSTATE[5] = '\0';
    memset(statementBuffer, ' ', 390);
    statementBuffer[389] = '\0';
    output_rowset_size = 0;

    EXEC SQL DELETE FROM DYNAMIC5;

    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('abc',0), ('abc',1), ('abc',2), ('abc', 3);
    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('abc',4), ('abc',5), ('abc',6), ('abc', 7);
    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('abc',8), ('abc',9), ('abc',10), ('abc', 10);

 
    printf("prepare delete:\n");  
    strcpy(statementBuffer,
    "DELETE FROM DYNAMIC5 WHERE B = ?[10] ;" );
    printf("SQLCODE after prepare is %d\n", SQLCODE);
    EXEC SQL PREPARE S1 FROM :statementBuffer; 

    num_in = 30;
    /* create SQLDA for DELETE columns */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :in_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);

    /* populate the SQLDA */
    EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc;
    printf("SQLSTATE after describe is %s\n", SQLSTATE);


    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = ROWSET_SIZE;
    printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);

    populateInputHostvars();

    EXEC SQL EXECUTE S1 USING :b_arr ;

    EXEC SQL COMMIT ;

    printRowsFromDynamic5();

    EXEC SQL DELETE FROM DYNAMIC5;
    EXEC SQL DEALLOCATE DESCRIPTOR :in_desc ;
    EXEC SQL DEALLOCATE PREPARE S1;

}

/*****************************************************/
void dynamic_direct()
/*****************************************************/
{
                                    
    /* Initialize all variables */
    printf("DYNAMIC_DIRECT:\n");  
    strcpy(in_desc,"inscols     "); 
    SQLSTATE[5] = '\0';
    memset(statementBuffer, ' ', 390);
    statementBuffer[389] = '\0';
    output_rowset_size = 10;

    /* INSERTING 10 ROWS */

    EXEC SQL DELETE FROM DYNAMIC5;

    printf("prepare insert:\n");  
    strcpy(statementBuffer,
    "INSERT INTO DYNAMIC5 VALUES ( 'jim', ?[10] );" );
    /* construct S1 from of INSERT statement */
    EXEC SQL PREPARE S0 FROM :statementBuffer; 
    printf("SQLSTATE after prepare is %s\n", SQLSTATE);
    printf("SQLCODE after prepare is %d\n", SQLCODE);
    EXEC SQL PREPARE S1 FROM :statementBuffer; 

    num_in = 30;
    /* create SQLDA for INSERT columns */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :in_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);

    /* populate the SQLDA */
    EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc;
    printf("SQLSTATE after describe is %s\n", SQLSTATE);


    num = 1;

    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = ROWSET_SIZE;
    printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);

    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = COUNT;
    printf("COUNT after prepare & describe is  %d\n", output_rowset_size);

    EXEC SQL GET DESCRIPTOR :in_desc VALUE :num :output_rowset_size = ROWSET_VAR_LAYOUT_SIZE;
    printf("ROWSET_VAR_LAYOUT_SIZE after prepare & describe is  %d\n", output_rowset_size);

    EXEC SQL GET DESCRIPTOR :in_desc VALUE :num :output_rowset_size = ROWSET_IND_LAYOUT_SIZE;
    printf("ROWSET_IND_LAYOUT_SIZE after prepare & describe is  %d\n", output_rowset_size);

    EXEC SQL GET DESCRIPTOR :in_desc VALUE :num :output_rowset_size = TYPE;
    printf("TYPE after prepare & describe is  %d\n", output_rowset_size);

    EXEC SQL GET DESCRIPTOR :in_desc VALUE :num :output_rowset_size = TYPE_FS;
    printf("TYPE_FS after prepare & describe is  %d\n", output_rowset_size);
    
    populateInputHostvars();
    arr_ptr = (long) (&(b_arr[0])) ;
    ind_ptr = (long) (&(b_arr_ind[0]));
    arr_size = 4;
    ind_size = 2;

    EXEC SQL SET DESCRIPTOR :in_desc VALUE :num 
	VARIABLE_POINTER = :arr_ptr,
	INDICATOR_POINTER = :ind_ptr,
	ROWSET_VAR_LAYOUT_SIZE = :arr_size, 
	ROWSET_IND_LAYOUT_SIZE = :ind_size; 

    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = ROWSET_SIZE;
    printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);

    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = COUNT;
    printf("COUNT after prepare & describe is  %d\n", output_rowset_size);

    EXEC SQL GET DESCRIPTOR :in_desc VALUE :num :output_rowset_size = ROWSET_VAR_LAYOUT_SIZE;
    printf("ROWSET_VAR_LAYOUT_SIZE after prepare & describe is  %d\n", output_rowset_size);

    EXEC SQL GET DESCRIPTOR :in_desc VALUE :num :output_rowset_size = ROWSET_IND_LAYOUT_SIZE;
    printf("ROWSET_IND_LAYOUT_SIZE after prepare & describe is  %d\n", output_rowset_size);

    EXEC SQL GET DESCRIPTOR :in_desc VALUE :num :output_rowset_size = TYPE;
    printf("TYPE after prepare & describe is  %d\n", output_rowset_size);

    EXEC SQL GET DESCRIPTOR :in_desc VALUE :num :output_rowset_size = TYPE_FS;
    printf("TYPE_FS after prepare & describe is  %d\n", output_rowset_size);
    

    EXEC SQL EXECUTE S1 USING SQL DESCRIPTOR :in_desc;

    EXEC SQL COMMIT ;

    printRowsFromDynamic5();

    EXEC SQL DELETE FROM DYNAMIC5;
    EXEC SQL DEALLOCATE DESCRIPTOR :in_desc ;
    EXEC SQL DEALLOCATE PREPARE S1;

}

/*****************************************************/
void dynamic_expression()
/*****************************************************/
{                                 
    /* Initialize all variables */
    printf("DYNAMIC_EXPRESSION:\n");  
    strcpy(in_desc,"inscols     "); 
    SQLSTATE[5] = '\0';
    memset(statementBuffer, ' ', 390);
    statementBuffer[389] = '\0';
    output_rowset_size = 0;

    EXEC SQL DELETE FROM DYNAMIC5;

    /* INSERTING 10 ROWS */

    printf("prepare insert:\n");  
    strcpy(statementBuffer,
    "INSERT INTO DYNAMIC5 VALUES ( 'eee', CAST(?[10] AS INT)+9 );" );
    /* construct S1 from of INSERT statement */
    EXEC SQL PREPARE S0 FROM :statementBuffer; 
    printf("SQLSTATE after prepare is %s\n", SQLSTATE);
    printf("SQLCODE after prepare is %d\n", SQLCODE);
    EXEC SQL PREPARE S1 FROM :statementBuffer; 

    num_in = 30;
    /* create SQLDA for INSERT columns */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :in_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);

    /* populate the SQLDA */
    EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc;
    printf("SQLSTATE after describe is %s\n", SQLSTATE);


    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = ROWSET_SIZE;
    printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);

    populateInputHostvars();

    EXEC SQL EXECUTE S1 USING :b_arr INDICATOR :b_arr_ind;

    EXEC SQL COMMIT ;
   
    printRowsFromDynamic5();

    EXEC SQL DELETE FROM DYNAMIC5;
    EXEC SQL DEALLOCATE DESCRIPTOR :in_desc ;
    EXEC SQL DEALLOCATE PREPARE S1;

}

/*****************************************************/
void dynamic_inputsize()
/*****************************************************/
{                                 
    /* Initialize all variables */
    printf("DYNAMIC_INPUTSIZE:\n");  
    strcpy(in_desc,"inscols     "); 
    SQLSTATE[5] = '\0';
    memset(statementBuffer, ' ', 390);
    statementBuffer[389] = '\0';
    output_rowset_size = 0;

    EXEC SQL DELETE FROM DYNAMIC5;

    /* INSERTING 10 ROWS */

    printf("prepare insert:\n");  
    strcpy(statementBuffer,
    "ROWSET FOR INPUT SIZE ? INSERT INTO DYNAMIC5 VALUES ( ?[10], ?[10] );" );
    /* construct S1 from of INSERT statement */
    EXEC SQL PREPARE S0 FROM :statementBuffer; 
    printf("SQLSTATE after prepare is %s\n", SQLSTATE);
    printf("SQLCODE after prepare is %d\n", SQLCODE);
    EXEC SQL PREPARE S1 FROM :statementBuffer; 

    num_in = 30;
    /* create SQLDA for INSERT columns */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :in_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);

    /* populate the SQLDA */
    EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc;
    printf("SQLSTATE after describe is %s\n", SQLSTATE);


    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = ROWSET_SIZE;
    printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);

    populateInputHostvars();

    arr_size = 6;
    EXEC SQL EXECUTE S1 USING :arr_size, :a_arr, :b_arr ;

    EXEC SQL COMMIT ;
   
    printRowsFromDynamic5();

    EXEC SQL DELETE FROM DYNAMIC5;
    EXEC SQL DEALLOCATE DESCRIPTOR :in_desc ;
    EXEC SQL DEALLOCATE PREPARE S1;

}

/*****************************************************/
void dynamic_mix1()
/*****************************************************/
{                                 
    /* Initialize all variables */
    printf("DYNAMIC_MIX1:\n");  
    strcpy(in_desc,"inscols     "); 
    SQLSTATE[5] = '\0';
    memset(statementBuffer, ' ', 390);
    statementBuffer[389] = '\0';
    output_rowset_size = 0;

    EXEC SQL DELETE FROM DYNAMIC5;

    /* INSERTING 10 ROWS */

    printf("prepare insert:\n");  
    strcpy(statementBuffer,
    "INSERT INTO DYNAMIC5 VALUES ( ?[10], ? );" );
    /* construct S1 from of INSERT statement */
    EXEC SQL PREPARE S0 FROM :statementBuffer; 
    printf("SQLSTATE after prepare is %s\n", SQLSTATE);
    printf("SQLCODE after prepare is %d\n", SQLCODE);
    EXEC SQL PREPARE S1 FROM :statementBuffer; 

    num_in = 30;
    /* create SQLDA for INSERT columns */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :in_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);

    /* populate the SQLDA */
    EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc;
    printf("SQLSTATE after describe is %s\n", SQLSTATE);


    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = ROWSET_SIZE;
    printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);

    populateInputHostvars();

    b_int = 1001;
    EXEC SQL EXECUTE S1 USING :a_arr, :b_int ;

    EXEC SQL COMMIT ;
   
    printRowsFromDynamic5();

    EXEC SQL DELETE FROM DYNAMIC5;
    EXEC SQL DEALLOCATE DESCRIPTOR :in_desc ;
    EXEC SQL DEALLOCATE PREPARE S1;

}
/*****************************************************/
void dynamic_mix2()
/*****************************************************/
{                                 
    /* Initialize all variables */
    printf("DYNAMIC_MIX2:\n");  
    strcpy(in_desc,"inscols     "); 
    SQLSTATE[5] = '\0';
    memset(statementBuffer, ' ', 390);
    statementBuffer[389] = '\0';
    output_rowset_size = 0;

    EXEC SQL DELETE FROM DYNAMIC5;

    /* INSERTING 10 ROWS */

    printf("prepare insert:\n");  
    strcpy(statementBuffer,
    "INSERT INTO DYNAMIC5 VALUES ( ?,?[10] );" );
    /* construct S1 from of INSERT statement */
    EXEC SQL PREPARE S0 FROM :statementBuffer; 
    printf("SQLSTATE after prepare is %s\n", SQLSTATE);
    printf("SQLCODE after prepare is %d\n", SQLCODE);
    EXEC SQL PREPARE S1 FROM :statementBuffer; 

    num_in = 30;
    /* create SQLDA for INSERT columns */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :in_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);

    /* populate the SQLDA */
    EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc;
    printf("SQLSTATE after describe is %s\n", SQLSTATE);


    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = ROWSET_SIZE;
    printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);

    populateInputHostvars();

    strcpy((char *)&a_char[0], "lll");
    EXEC SQL EXECUTE S1 USING :a_char, :b_arr ;

    EXEC SQL COMMIT ;
   
    printRowsFromDynamic5();

    EXEC SQL DELETE FROM DYNAMIC5;
    EXEC SQL DEALLOCATE DESCRIPTOR :in_desc ;
    EXEC SQL DEALLOCATE PREPARE S1;

}
/*****************************************************/
void dynamic_select1()
/*****************************************************/
{                                 
    /* Initialize all variables */
    printf("DYNAMIC_SELECT1:\n");  
    strcpy(in_desc,"inscols     "); 
    SQLSTATE[5] = '\0';
    memset(statementBuffer, ' ', 390);
    statementBuffer[389] = '\0';
    output_rowset_size = 0;

    EXEC SQL DELETE FROM DYNAMIC5;

    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('zzz',0), ('yyy',1), ('xxx',2), ('www', 3);
    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('vvv',4), ('uuu',5), ('ttt',6), ('sss', 7);
    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('rrr',8), ('qqq',9), ('ppp',10), ('mmm', 10);


    printf("prepare select:\n");  
    strcpy(statementBuffer,
    "ROWSET FOR KEY BY rowid SELECT rowid, a, b FROM DYNAMIC5 WHERE b = ?[10] ORDER BY a;" );
    EXEC SQL PREPARE S1 FROM :statementBuffer; 
    printf("SQLSTATE after prepare is %s\n", SQLSTATE);
    printf("SQLCODE after prepare is %d\n", SQLCODE);

    num_in = 30;
    /* create SQLDA for INPUT */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :in_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);

    /* populate the SQLDA */
    EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc;
    printf("SQLSTATE after describe is %s\n", SQLSTATE);


    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = ROWSET_SIZE;
    printf("ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);

    populateInputHostvars();

    EXEC SQL EXECUTE S1 USING :b_arr INTO :rowid_arr, :aout_arr, :bout_arr;

    EXEC SQL GET DIAGNOSTICS :rows_fetched = ROW_COUNT;
    Int32 loop ;
    for (loop=0; loop<rows_fetched; loop++) {
      aout_arr[loop][3] = '\0';
      printf("%d   %s   %d\n",rowid_arr[loop], aout_arr[loop], bout_arr[loop]);
    }
   
    printf("\n***********************************************************************\n\n");

    EXEC SQL DELETE FROM DYNAMIC5;
    EXEC SQL DEALLOCATE DESCRIPTOR :in_desc ;
    EXEC SQL DEALLOCATE PREPARE S1;

}

/*****************************************************/
void dynamic_select2()
/*****************************************************/
{                                 
    /* Initialize all variables */
    printf("DYNAMIC_SELECT2:\n");  
    strcpy(in_desc,"inscols     "); 
    strcpy(out_desc,"selcols     "); 
    SQLSTATE[5] = '\0';
    memset(statementBuffer, ' ', 390);
    statementBuffer[389] = '\0';
    output_rowset_size = 0;

    EXEC SQL DELETE FROM DYNAMIC5;

    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('aaa',0), ('bbb',1), ('ccc',2), ('ddd', 3);
    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('eee',4), ('fff',5), ('ggg',6), ('hhh', 7);
    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('iii',8), ('jjj',9), ('kkk',10), ('lll', 10);


    printf("prepare select:\n");  
    strcpy(statementBuffer,
    "ROWSET FOR KEY BY rowid SELECT rowid, a,b FROM DYNAMIC5 WHERE b = ?[10] ORDER BY a;" );
    EXEC SQL PREPARE S1 FROM :statementBuffer; 
    printf("SQLSTATE after prepare is %s\n", SQLSTATE);
    printf("SQLCODE after prepare is %d\n", SQLCODE);

    num_in = 30;
    /* create SQLDA for INPUT */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :in_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);
    /* create SQLDA for OUTPUT */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :out_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);


    /* populate the SQLDA */
    EXEC SQL DESCRIBE INPUT S1 USING SQL DESCRIPTOR :in_desc;
    printf("SQLSTATE after input describe is %s\n", SQLSTATE);
    EXEC SQL DESCRIBE OUTPUT S1 USING SQL DESCRIPTOR :out_desc;
    printf("SQLSTATE after output describe is %s\n", SQLSTATE);


    output_rowset_size = 10;
    num = 1;
    arr_size = 4;
    char_size = 4;
   EXEC SQL SET DESCRIPTOR :out_desc  ROWSET_SIZE = :output_rowset_size; 

    EXEC SQL GET DESCRIPTOR :out_desc :count = COUNT;
    printf("COUNT after prepare & describe is  %d\n", count);

    EXEC SQL SET DESCRIPTOR :out_desc VALUE :num ROWSET_VAR_LAYOUT_SIZE = :arr_size;
    arr_ptr = (long) &rowid_arr[0];
    EXEC SQL SET DESCRIPTOR :out_desc VALUE :num VARIABLE_POINTER = :arr_ptr;

     num++;
    EXEC SQL SET DESCRIPTOR :out_desc VALUE :num ROWSET_VAR_LAYOUT_SIZE = :char_size; 
    arr_ptr = (long) (&(aout_arr[0]));
    EXEC SQL SET DESCRIPTOR :out_desc VALUE :num VARIABLE_POINTER = :arr_ptr;

    num++;
    EXEC SQL SET DESCRIPTOR :out_desc VALUE :num ROWSET_VAR_LAYOUT_SIZE = :arr_size; 
    arr_ptr = (long) (&(bout_arr[0]));
    EXEC SQL SET DESCRIPTOR :out_desc VALUE :num VARIABLE_POINTER = :arr_ptr; 


    populateInputHostvars();

    EXEC SQL EXECUTE S1 USING :b_arr INTO SQL DESCRIPTOR :out_desc ;

    EXEC SQL GET DIAGNOSTICS :rows_fetched = ROW_COUNT;
    Int32 loop ;
    for (loop=0; loop<rows_fetched; loop++) {
      aout_arr[loop][3] = '\0';
      printf("%d   %s   %d\n",rowid_arr[loop], aout_arr[loop], bout_arr[loop]);
    }
   
    printf("\n***********************************************************************\n\n");

    
    EXEC SQL DELETE FROM DYNAMIC5;
    EXEC SQL DEALLOCATE DESCRIPTOR :in_desc ;
    EXEC SQL DEALLOCATE DESCRIPTOR :out_desc ;
    EXEC SQL DEALLOCATE PREPARE S1;

}
/*****************************************************/
void dynamic_cursor1()
/*****************************************************/
{                                 
    /* Initialize all variables */
    printf("DYNAMIC_CURSOR1:\n");  
    SQLSTATE[5] = '\0';
    memset(statementBuffer, ' ', 390);
    statementBuffer[389] = '\0';

    EXEC SQL DELETE FROM DYNAMIC5;

    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('abc',0), ('def',0), ('ghi',2), ('jkl', 2);
    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('mno',4), ('pqr',4), ('stu',6), ('vwx', 6);
    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('yza',8), ('cab',8), ('fed',10), ('ihg', 10);


    printf("prepare select:\n");  
    strcpy(statementBuffer,
    "SELECT a, b FROM DYNAMIC5 WHERE b = ?[10] ORDER BY a;" );
    EXEC SQL PREPARE cursor_spec FROM :statementBuffer; 
    printf("SQLSTATE after prepare is %s\n", SQLSTATE);
    printf("SQLCODE after prepare is %d\n", SQLCODE);

    EXEC SQL DECLARE C2 CURSOR FOR cursor_spec ;  
    populateInputHostvars();
    EXEC SQL OPEN C2 USING :b_arr;

    EXEC SQL FETCH C2 INTO :aout_arr, :bout_arr;

    EXEC SQL GET DIAGNOSTICS :rows_fetched = ROW_COUNT;
    Int32 loop ;
    for (loop=0; loop<rows_fetched; loop++) {
      aout_arr[loop][3] = '\0';
      printf("%s   %d\n", aout_arr[loop], bout_arr[loop]);
    }
   
    printf("\nsecond fetch \n")  ;

    EXEC SQL FETCH C2 INTO :aout_arr, :bout_arr;

    EXEC SQL GET DIAGNOSTICS :rows_fetched = ROW_COUNT;
    for (loop=0; loop<rows_fetched; loop++) {
      aout_arr[loop][3] = '\0';
      printf("%s   %d\n", aout_arr[loop], bout_arr[loop]);
    }

     printf("\n***********************************************************************\n\n");
   
    EXEC SQL CLOSE C2;

    EXEC SQL DELETE FROM DYNAMIC5;
    EXEC SQL DEALLOCATE PREPARE cursor_spec;

}
/*****************************************************/
void dynamic_cursor2()
/*****************************************************/
{                                 
    /* Initialize all variables */
    printf("DYNAMIC_CURSOR2:\n");  
    strcpy(in_desc,"inscols     "); 
    strcpy(out_desc,"selcols     "); 
    SQLSTATE[5] = '\0';
    memset(statementBuffer, ' ', 390);
    statementBuffer[389] = '\0';
    output_rowset_size = 0;

    EXEC SQL DELETE FROM DYNAMIC5;

    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('AAA',0), ('BBB',0), ('CCC',2), ('DDD', 2);
    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('EEE',4), ('FFF',4), ('GGG',6), ('HHH', 6);
    EXEC SQL INSERT INTO DYNAMIC5 VALUES ('III',8), ('JJJ',10), ('KKK',12), ('LLL', 12);


    printf("prepare select:\n");  
    strcpy(statementBuffer,
    "ROWSET FOR KEY BY rowid SELECT rowid, a,b FROM DYNAMIC5 WHERE b = CAST(?[10] AS INT) ORDER BY a;" );
    EXEC SQL PREPARE cursor_spec FROM :statementBuffer; 
    printf("SQLSTATE after prepare is %s\n", SQLSTATE);
    printf("SQLCODE after prepare is %d\n", SQLCODE);

    EXEC SQL DECLARE C3 CURSOR FOR cursor_spec;

    num_in = 30;
    /* create SQLDA for INPUT */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :in_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);
    /* create SQLDA for OUTPUT */
    EXEC SQL ALLOCATE DESCRIPTOR GLOBAL :out_desc with MAX :num_in;
    printf("SQLSTATE after allocate is %s\n", SQLSTATE);


    /* populate the SQLDA */
    EXEC SQL DESCRIBE INPUT cursor_spec USING SQL DESCRIPTOR :in_desc;
    printf("SQLSTATE after input describe is %s\n", SQLSTATE);
    EXEC SQL DESCRIBE OUTPUT cursor_spec USING SQL DESCRIPTOR :out_desc;
    printf("SQLSTATE after output describe is %s\n", SQLSTATE);


    num = 1;
    arr_size = 4;
    char_size = 4;

    populateInputHostvars();

    EXEC SQL GET DESCRIPTOR :in_desc :count = COUNT;
    printf("input COUNT after prepare & describe is  %d\n", count);

    EXEC SQL GET DESCRIPTOR :in_desc :output_rowset_size = ROWSET_SIZE;
    printf("input ROWSET_SIZE after prepare & describe is  %d\n", output_rowset_size);


    EXEC SQL GET DESCRIPTOR :in_desc VALUE :num :output_rowset_size = ROWSET_VAR_LAYOUT_SIZE;
    printf("input ROWSET_VAR_LAYOUT_SIZE after prepare & describe is  %d\n", output_rowset_size);

    EXEC SQL GET DESCRIPTOR :in_desc VALUE :num :output_rowset_size = ROWSET_IND_LAYOUT_SIZE;
    printf("input ROWSET_IND_LAYOUT_SIZE after prepare & describe is  %d\n", output_rowset_size);

   
    output_rowset_size = 10;
   EXEC SQL SET DESCRIPTOR :out_desc  ROWSET_SIZE = :output_rowset_size; 

    EXEC SQL GET DESCRIPTOR :out_desc :count = COUNT;
    printf("output COUNT after prepare & describe is  %d\n", count);



    EXEC SQL SET DESCRIPTOR :out_desc VALUE :num ROWSET_VAR_LAYOUT_SIZE = :arr_size;
    arr_ptr = (long) (&(rowid_arr[0]));
    EXEC SQL SET DESCRIPTOR :out_desc VALUE :num VARIABLE_POINTER = :arr_ptr;
    arr_ptr = (long) (&(b_arr[0]));
    EXEC SQL SET DESCRIPTOR :in_desc VALUE :num VARIABLE_POINTER = :arr_ptr;
    arr_ptr = (long) (&(b_arr_ind[0]));
    EXEC SQL SET DESCRIPTOR :in_desc VALUE :num INDICATOR_POINTER = :arr_ptr;

    num++;
    EXEC SQL SET DESCRIPTOR :out_desc VALUE :num ROWSET_VAR_LAYOUT_SIZE = :char_size; 
    arr_ptr = (long) (&(aout_arr[0]));
    EXEC SQL SET DESCRIPTOR :out_desc VALUE :num VARIABLE_POINTER = :arr_ptr;

    num++;
    EXEC SQL SET DESCRIPTOR :out_desc VALUE :num ROWSET_VAR_LAYOUT_SIZE = :arr_size; 
    arr_ptr = (long) (&(bout_arr[0]));
    EXEC SQL SET DESCRIPTOR :out_desc VALUE :num VARIABLE_POINTER = :arr_ptr; 


    EXEC SQL OPEN C3 USING SQL DESCRIPTOR :in_desc;

    EXEC SQL FETCH C3 INTO SQL DESCRIPTOR :out_desc ;

    EXEC SQL GET DIAGNOSTICS :rows_fetched = ROW_COUNT;
    Int32 loop ;
    for (loop=0; loop<rows_fetched; loop++) {
      aout_arr[loop][3] = '\0';
      printf("%d   %s   %d\n", rowid_arr[loop], aout_arr[loop], bout_arr[loop]);
    }

    printf("\nsecond fetch \n")  ;

    EXEC SQL FETCH C3 INTO SQL DESCRIPTOR :out_desc ;

    EXEC SQL GET DIAGNOSTICS :rows_fetched = ROW_COUNT;
    for (loop=0; loop<rows_fetched; loop++) {
      aout_arr[loop][3] = '\0';
      printf("%d   %s   %d\n", rowid_arr[loop], aout_arr[loop], bout_arr[loop]);
    }

     printf("\n***********************************************************************\n\n");
    EXEC SQL CLOSE C3;
   

    EXEC SQL DELETE FROM DYNAMIC5;
    EXEC SQL DEALLOCATE DESCRIPTOR :in_desc ;
    EXEC SQL DEALLOCATE DESCRIPTOR :out_desc ;
    EXEC SQL DEALLOCATE PREPARE cursor_spec;

}

/*****************************************************/
void display_diagnosis()
/*****************************************************/
{
  exec sql get diagnostics :hv_num = NUMBER,
                :hv_cmdfcn = COMMAND_FUNCTION;

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[128]='\0';
   hv_cmdfcn[128]='\0';

   printf("Number  : %d\n", hv_num);
   printf("Cmd fcn : %s\n", hv_cmdfcn);

  for (i = 1; i <= hv_num; i++) {
      exec sql get diagnostics exception :i
          :hv_colname = COLUMN_NAME,                   
          :hv_tabname = TABLE_NAME,
          :hv_curname = CURSOR_NAME,
          :hv_sqlcode = SQLCODE,
          :hv_msgtxt = MESSAGE_TEXT;

   printf("Column  : %s\n", hv_colname);
   printf("Table   : %s\n", hv_tabname);
   printf("Cursor  : %s\n", hv_curname);
   printf("Sqlcode : %d\n", hv_sqlcode);
   printf("Message : %s\n", hv_msgtxt);

   memset(hv_msgtxt,' ',sizeof(hv_msgtxt));
   hv_msgtxt[128]='\0';
   memset(hv_tabname,' ',sizeof(hv_tabname));
   hv_tabname[128]='\0';
   memset(hv_colname,' ',sizeof(hv_colname));
   hv_colname[128]='\0';
   memset(hv_curname,' ',sizeof(hv_curname));
   hv_curname[128]='\0';
  }
}
/*****************************************************/
void populateInputHostvars()
/*****************************************************/
{
Int32 loop ;

for (loop=0; loop<10; loop++) 
    {
	strcpy((char *)&a_arr[loop][0], "sam");
	b_arr[loop] = 2*loop ;
	b_arr_ind[loop] = 0;
    }
    b_arr_ind[4] = -1;
}
/*****************************************************/
void printRowsFromDynamic5()
/*****************************************************/
{
#define SIZE 25

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [SIZE] char a_out[4];
  ROWSET [SIZE] Int32 b_out;
  ROWSET [SIZE] short b_out_ind;
  Int32 numFetched;
EXEC SQL END DECLARE SECTION;


EXEC SQL DECLARE C1 CURSOR FOR
SELECT a,b FROM DYNAMIC5 ORDER BY a,b ;

EXEC SQL OPEN C1;
if (SQLCODE != 0) {
	printf("Failed to open cursor: C1. SQLCODE = %d\n",SQLCODE);
    }

EXEC SQL FETCH C1 INTO :a_out, :b_out :b_out_ind ;
Int32 loop ;
    for (loop=0; loop<SIZE; loop++) {
	a_out[loop][3] = '\0';
    }

    EXEC SQL GET DIAGNOSTICS :numFetched = ROW_COUNT;
    for (loop=0; loop<numFetched; loop++) {
	    if (b_out_ind[loop] == 0)
	      printf("%s     %d\n",a_out[loop], b_out[loop]);
	    else
	      printf("%s     ?\n",a_out[loop], b_out[loop]);
	 }
printf("\n***********************************************************************\n\n");
EXEC SQL CLOSE C1;
	    
}
/*****************************************************/
void displayRowsetRowCountArray()
/*****************************************************/
{
  Int32 rowcountstmtnum = 102;
  Int32 rowsetrowsaffected[100];
  Int32 retcode ;
  Int32 size = 100;
  Int32 actual_size = 0;

  

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
