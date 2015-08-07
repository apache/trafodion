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
/* Test for rowset for input size and key by rowid support in static cursor declaration*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SIZE 10

EXEC SQL MODULE CAT.SCH.TESTE221M NAMES ARE ISO88591;

/* globals */
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
  ROWSET [SIZE] char b_char[26];
  ROWSET [SIZE] long e_int;
  ROWSET [SIZE] long ei_int;
  ROWSET [SIZE] short rowid;
  long numFetched, inp_size, out_size;
EXEC SQL END DECLARE SECTION;

void initialize ( ) 
{
    int i ;

    inp_size =5;
    numFetched=0;
    out_size=7;
    for (i=0; i<SIZE; i++) {
    b_char[i][25] = '\0';
    }

    for (i=0; i<SIZE; i++) {
    ei_int[i] = 100*i;
    rowid[i] = -1;
    }
}

void delete_rows ( ) 
{
    EXEC SQL DELETE FROM dynamic2;
}

void insert_rows ( ) 
{
    EXEC SQL INSERT INTO dynamic2 VALUES (0.0,'Sunny Austin',0);
    EXEC SQL INSERT INTO dynamic2 VALUES (0.0,'Cool Cupertino',100);
    EXEC SQL INSERT INTO dynamic2 VALUES (0.0,'Icy Detroit',200);
    EXEC SQL INSERT INTO dynamic2 VALUES (0.0,'Hot Tunis',400);
    EXEC SQL INSERT INTO dynamic2 VALUES (0.0,'Cold Denmark',500);
    EXEC SQL INSERT INTO dynamic2 VALUES (0.0,'Pleasant Hawaii',100);
    EXEC SQL INSERT INTO dynamic2 VALUES (0.0,'Fast Indianapolis',200);
    EXEC SQL INSERT INTO dynamic2 VALUES (0.0,'Rainy Bangalore',200);
}


void printOutput (int withRowid ) 
{
    int i ;
    for (i=0; i<SIZE; i++) {
	b_char[i][25] = '\0';
    }

    EXEC SQL GET DIAGNOSTICS :numFetched = ROW_COUNT;
    if (withRowid) 
    {
	for (i=0; i<numFetched; i++) {
	printf("%s    %d     %d\n",b_char[i], e_int[i], rowid[i]);
	}
    }
    else 
    {
	 for (i=0; i<numFetched; i++) {
	 printf("%s    %d\n",b_char[i], e_int[i]);
	 }
    }
    printf("\n");
	    
}

int main()
{

    initialize () ;
    delete_rows();
    insert_rows();

    


/* Inputsize and Key By rowid */

   printf( "In C3 cursor : Inputsize and Key By rowid\n");


    EXEC SQL DECLARE C3 CURSOR FOR
    ROWSET FOR INPUT SIZE :inp_size, KEY BY rowid
    SELECT b,e,rowid FROM dynamic2 WHERE dynamic2.e = :ei_int 
    ORDER BY b;

    EXEC SQL OPEN C3;
    if (SQLCODE != 0) {
	printf("Failed to open cursor: C3. SQLCODE = %ld\n",SQLCODE);
	return(SQLCODE);
    }

    EXEC SQL FETCH C3 INTO :b_char, :e_int, :rowid;
    printOutput(1);
    EXEC SQL CLOSE C3;



/* Inputsize */

printf( "In C5 cursor : Inputsize \n");


    EXEC SQL DECLARE C5 CURSOR FOR
    ROWSET FOR INPUT SIZE :inp_size
    SELECT b,e FROM dynamic2 WHERE dynamic2.e = :ei_int 
    ORDER BY b;
  
    EXEC SQL OPEN C5;
    if (SQLCODE != 0) {
	printf("Failed to open cursor: C5. SQLCODE = %ld\n",SQLCODE);
	return(SQLCODE);
    }

    EXEC SQL FETCH C5 INTO :b_char, :e_int;
    printOutput(0);
    EXEC SQL CLOSE C5;



/* Key By Rowid */

printf( "In C7 cursor : Key By Rowid \n");


    EXEC SQL DECLARE C7 CURSOR FOR
    ROWSET FOR KEY BY rowid
    SELECT b,e, rowid FROM dynamic2 WHERE dynamic2.e = :ei_int 
    ORDER BY b;
  
    EXEC SQL OPEN C7;
    if (SQLCODE != 0) {
	printf("Failed to open cursor: C7. SQLCODE = %ld\n",SQLCODE);
	return(SQLCODE);
    }

    EXEC SQL FETCH C7 INTO :b_char, :e_int, :rowid;
    printOutput(1);
    EXEC SQL CLOSE C7;

 
    delete_rows();

    return(SQLCODE);

}
