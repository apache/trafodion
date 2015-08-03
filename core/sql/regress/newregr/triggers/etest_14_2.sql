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
// +++ Code modified on 2003/5/6
**********************************************************************/
/* -*SQL - C*- */



#include <string.h>
#include <stdlib.h>
#include <stdio.h>


void SQL_EXEC_SetSQLSTATE(char *SQLSTATE_param);


EXEC SQL MODULE update1 NAMES ARE ISO88591 ; 


/* special defines */

#define MP_NODE_VOL_NAME NOAHARK.$e001

void SQL_EXEC_SetSQLSTATE(char *SQLSTATE_param)
  {
  strcpy(SQLSTATE_param, "00000");
  }


char* argumentDesc[] = {"# of col. in table /30", "# of iterations", "# insert each iter.", "# select each iter."};

bool debugFlag = true;
void printDebug(char* s)
{
	if(debugFlag)
		fprintf(stderr,"%s",s);
}

/* Global host variables/definitions */
///////////////////////////////////////////
	EXEC SQL BEGIN DECLARE SECTION;
	
	int  val1;
	int  val2;
	int  val3;
	int  val4;

    char SQLSTATE[6];
	char SQLSTATE_OK[6] = "00000";
	char SQLSTATE_NODATA[6] = "02000"; 
	unsigned NUMERIC (4)	hv_num;
	char					hv_cmdfcn[128];
	unsigned NUMERIC (4)	diag;
	char					hv_sqlstate[6];
	char					hv_tabname[128];
	char					hv_colname[128];
	char					hv_cursname[128];
	long SQLCODE;

	EXEC SQL END DECLARE SECTION;
////////////////////////////////////////////

void sqlError()
{
	EXEC SQL WHENEVER SQLERROR CONTINUE;
	SQLSTATE[5] = '\0';
	printDebug("\nSQL error.");
	printf("\n ERROR. getting diagnostics...");
	EXEC SQL GET DIAGNOSTICS
		:hv_num = NUMBER,
		:hv_cmdfcn = COMMAND_FUNCTION;

	hv_cmdfcn[127]='\0';
	printf("\n statement: %s", hv_cmdfcn);
	for (diag=1; diag< hv_num; diag++) {
		EXEC SQL GET DIAGNOSTICS EXCEPTION :diag
			:hv_tabname = TABLE_NAME,
			:hv_colname = COLUMN_NAME,
			:hv_cursname = CURSOR_NAME,
			:hv_sqlstate = RETURNED_SQLSTATE;

		hv_tabname[127] = '\0';
		hv_cursname[127]= '\0';
		printf("\ncondition: %hu \nTable: %s, Column: %s, Cursor: %s",
			diag, hv_tabname, hv_colname, hv_cursname);
		printf("\nSQLSTATE: %s", hv_sqlstate);
	}

	exit (1); 
}

EXEC SQL WHENEVER SQLERROR GOTO sqlerror;
	

#define NUM_ROWS_RESULT 4

int main(int argc, char* argv[])
{

	int i=0;
	bool result = true;	// final result
	int  expectedRes[NUM_ROWS_RESULT][4] = {{1,11,2,1}, {2,12,4,3}, {3,13,6,4}, {4,14,8,7}};
	printDebug("\n\n--------------- start ------------------------");
	
        EXEC SQL CONTROL QUERY DEFAULT ISOLATION_LEVEL 'READ_COMMITTED';

        EXEC SQL CONTROL QUERY DEFAULT ISOLATION_LEVEL_FOR_UPDATES '';

	EXEC SQL BEGIN WORK;

	EXEC SQL DECLARE curs1 CURSOR FOR
		SELECT a, c FROM cat1.schm.tab1A WHERE a<4
		for update;

	val1 = val2 = val3 = val4 = -1;	// initialize host variables

	EXEC SQL OPEN curs1;
	EXEC SQL FETCH curs1
		INTO :val1, :val2;
	while(strcmp(SQLSTATE, SQLSTATE_OK)== 0 ){
		EXEC SQL UPDATE cat1.schm.tab1A
			SET  c = c+1
			WHERE CURRENT OF curs1;

		EXEC SQL FETCH curs1
			INTO :val1, :val2;
		i++;
	}

	EXEC SQL CLOSE curs1;

	EXEC SQL COMMIT WORK;

	printf("\n %d row(s) affected by the update", i);

	/*-----------------------*\
	* checking results
	\*-----------------------*/

	i = 0;

	EXEC SQL BEGIN WORK;
	EXEC SQL DECLARE curs2 CURSOR FOR
		SELECT * FROM cat1.schm.tab1A;

	EXEC SQL OPEN curs2;
	EXEC SQL FETCH curs2
		INTO :val1, :val2, :val3, :val4;
	while(strcmp(SQLSTATE, SQLSTATE_OK)== 0 ){
		if(i < NUM_ROWS_RESULT) {
			if(val1 != expectedRes[i][0] || val2 != expectedRes[i][1] ||
				val3 != expectedRes[i][2] || val4 != expectedRes[i][3]) {
				result=false;
			}
		}
		else {
			result = false;			
		}
		
		EXEC SQL FETCH curs2
			INTO :val1, :val2, :val3, :val4;
		i++;
	}

	EXEC SQL CLOSE curs2;
	EXEC SQL COMMIT WORK;

	if(i != NUM_ROWS_RESULT) result = false;
	printf("\n %d row(s) found in cat1.schm.tab1A", i);
	printf("\n RESULT: %s\n", (result ? "success" : "failure"));
	printDebug("\n\n--------------- end ------------------------");
	return 0;

notFound:
	printf("\n\n Data not found exception. exiting...");
	exit(1);

sqlerror: sqlError();
		  return 1;


}	// end of main



			
