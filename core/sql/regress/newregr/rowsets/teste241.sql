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
/* teste241.sql
 * Mike Hanlon & Suresh Subbiah
 * 07-12-2005
 *
 * embedded C tests for Non Atomic Rowsets
 *   Test Classification: Positive and Negative
 *   Test Level: Functional
 *   Test Coverage:
 *   Insert on table that has an AFTER trigger defined on it
 *   Insert should succeed if trigger is disabled
 *   Insert should fail with sqlcode -30029 if trigger is enabled
 *
 */                                                    


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

#include "defaultSchema.h"

#define NAR00 30022
#define SIZE 50


void display_diagnosis();


EXEC SQL MODULE CAT.SCH.TESTE241M NAMES ARE ISO88591;

/* globals */

char defaultSchema[MAX_DEFAULT_SCHEMA_LEN + 1];

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [SIZE] Int32 a_int;
  Int32 numRows ;
  Int32 savesqlcode;
  char  statementBuffer[390];  
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

#include "defaultSchema.cpp"

Int32 main()
{

setDefaultSchema(defaultSchema, MAX_DEFAULT_SCHEMA_LEN, 0);
sprintf(statementBuffer, "set schema %s;", defaultSchema);
exec sql execute immediate :statementBuffer;

memset(statementBuffer, ' ', 390);
statementBuffer[389] = '\0';


// INDEX MAINTAINENCE

// expecting 30026
printf("expecting 30026\n");
strcpy(statementBuffer,
"insert into rownum2 values (?[10], ?[10]) NOT ATOMIC ;");
exec sql prepare s1 from :statementBuffer ;


memset(statementBuffer, ' ', 390);
statementBuffer[389] = '\0';

// BEFORE TRIGGERS

// expecting 30027
printf("expecting 30027\n");
strcpy(statementBuffer,
"insert into rownum5 values (?[10], ?[10]) NOT ATOMIC ;");
exec sql prepare s1 from :statementBuffer ;

// should prepare
memset(statementBuffer, ' ', 390);
statementBuffer[389] = '\0';
strcpy(statementBuffer,
"insert into rownum6 values (?[10], ?[10]) NOT ATOMIC ;");
exec sql prepare s1 from :statementBuffer ;


// RI

// expecting 30028
printf("expecting 30028\n");
memset(statementBuffer, ' ', 390);
statementBuffer[389] = '\0';
strcpy(statementBuffer,
"insert into rownum8 values (?[10], ?[10]) NOT ATOMIC ;");
exec sql prepare s1 from :statementBuffer ;

//should prepare
memset(statementBuffer, ' ', 390);
statementBuffer[389] = '\0';
strcpy(statementBuffer,
"insert into rownum11 values (?[10], ?[10]) NOT ATOMIC ;");
exec sql prepare s1 from :statementBuffer ;


// MV

//expecting 30033
printf("expecting 30033\n");
memset(statementBuffer, ' ', 390);
statementBuffer[389] = '\0';
strcpy(statementBuffer,
"insert into rownum9 values (?[10], ?[10]) NOT ATOMIC ;");
exec sql prepare s1 from :statementBuffer ;

//expecting 30033
printf("expecting 30033\n");
memset(statementBuffer, ' ', 390);
statementBuffer[389] = '\0';
strcpy(statementBuffer,
"insert into rownum12 values (?[10], ?[10]) NOT ATOMIC ;");
exec sql prepare s1 from :statementBuffer ;


// AFTER TRIGGERS

// should prepare
memset(statementBuffer, ' ', 390);
statementBuffer[389] = '\0';
strcpy(statementBuffer,
"insert into rownum7 values (?[10], ?[10]) NOT ATOMIC ;");
exec sql prepare s1 from :statementBuffer ;


EXEC SQL CONTROL QUERY DEFAULT RECOMPILATION_WARNINGS 'ON' ;

EXEC SQL DELETE FROM rownum7 ;
Int32 i=0;
for (i=0; i<SIZE; i++)
    a_int[i] = i+1;

  exec sql alter trigger disable itertrig3 ;
 
  EXEC SQL
  INSERT INTO rownum7 VALUES (-1*:a_int, 1)  NOT ATOMIC ;
 
  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }

  exec sql alter trigger enable itertrig3 ;


  /* should succeed as trigger will not fire for positive values of A */
  EXEC SQL
  INSERT INTO rownum7 VALUES (:a_int, 1)  NOT ATOMIC ;


  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }


  /* should raise 30029 as trigger will fire */
  EXEC SQL
  INSERT INTO rownum7 VALUES (-100*:a_int, 1)  NOT ATOMIC ;


  if (SQLCODE != 0 && SQLCODE != NAR00) {
    printf("Failed to insert. SQLCODE = %d\n",SQLCODE);
  }
  else {
    display_diagnosis();
    EXEC SQL COMMIT ;
  }
   
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
