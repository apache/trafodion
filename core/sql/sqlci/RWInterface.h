/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
//
**********************************************************************/
#ifndef _RWINTERFACE_H_
#define _RWINTERFACE_H_

// To use the values of SQL datatypes  _SQLDT_  defined in sqlcli.h
#include "sqlcli.h"

/* These are the standard values we use while returning status for all functions
*/

enum RetStatus { 
	ERR = -1, 
	SUCCESS, 
	DONE, 
	WARNING, 
	SEND_QUERY, 
	SEND_INPUT_ROW, 
	GET_OUTPUT_ROW,
	RESET_LIST,
	GET_ERROR_INFO};

/*These are the values sent by RW_MXCI_sendSelectStatus to indicate 
 the LIST functions status. */


enum SelectStatus {
	SELECT_STARTED,
	SELECT_ENDED,
	SELECT_ERROR,
	SELECT_CANCELLED,
	LIST_FIRST_STARTED,
	LIST_NEXT_STARTED,
	LIST_ALL_STARTED,
	LIST_ENDED };

/* These are the values used by MXCI_RW_getOutputDevice for the 
 different types of output devices. */

enum OutputDevice {
	LOG_FILE,
 	SCREEN,
	SPOOLER,
	OUT_FILE };

enum DateTimeFormat {
	DEFAULT,
	USA,
	EUROPEAN };

typedef struct {
	Int32 errorCode_; 
	char *charparam1_;  
	char *charparam2_;
	char *charparam3_;
	Lng32 intparam1_;
	Lng32 intparam2_;
	Lng32 intparam3_;
} ErrorValue;


typedef struct {
	Lng32 dataType_;		// Datatype of the column value.
        Lng32 displayLen_;       // Number of characters needed to display string representation of datatype.
	Lng32 length_;		// Length of the datatype.
	Lng32 dateTimeCode_;	// DateTimeCode.	
	Lng32 nullable_;		// Is it a nullable value or not.
	char *charSet_;		// The character set of the column.
	Lng32 precision_;	// Varies for individual datatypes.
	Lng32 leadPrecision_;    // Varies for individual datatypes.
	Lng32 scale_;		// Varies for individual datatypes.
	Lng32 collation_;	// Default collation.
	char *heading_;		// Heading name.
	Lng32 headingLen_;	// length of the heading.
	char *output_;    	// Column name for output.
	Lng32 outputLen_;   	// length of the column.
        char *tableName_;       // Table Name.
        Lng32 tableLen_;         // Table name length

} AttributeDetails;

/* function declarations implemented in Report Writer 
   and called by MXCI.
*/
Lng32 RW_MXCI_sendQuery (void* &RWEnv, void *SqlciEnv, char *query, Lng32 len);

Lng32 RW_MXCI_sendInputRow (void* &RWEnv, void* SqlciEnv, char *input_row, Lng32 len);

Lng32 RW_MXCI_getOutputRow (void* &RWEnv, void *SqlciEnv, char* &output_row, Lng32 &len);

Lng32 RW_MXCI_sendSelectStatus (void* &RWEnv, void *SqlciEnv, Lng32 num_rows, SelectStatus status);

Lng32 RW_MXCI_getErrorInfo (void* &RWEnv, void *SqlciEnv, ErrorValue *e);

Lng32 RW_MXCI_Constructor (void* &RWEnv, void *SqlciEnv) ;

Lng32 RW_MXCI_Destructor (void* &RWEnv, void *SqlciEnv) ;

Lng32 RW_MXCI_handleBreak (void* &RWEnv, void *SqlciEnv);

Lng32 RW_MXCI_sendOutputDevice (void* &RWEnv, void *SqlciEnv, OutputDevice device);


/* function declarations implemented in MXCI and called 
   by ReportWriter.
*/
Lng32 MXCI_RW_getMaxColumns (void *SqlciEnv, Lng32 &max, ErrorValue* &e);

Lng32 MXCI_RW_getColInfo (void *SqlciEnv, Lng32 column, 
                           AttributeDetails* &entry, ErrorValue* &e);

Lng32 MXCI_RW_getColAddr (void *SqlciEnv, Lng32 column, char *input_row, 
                          Lng32 len, char* &ptr, ErrorValue* &e);

Lng32 MXCI_RW_convDoIt (void *SqlciEnv, char* srcPtr, AttributeDetails* srcEntry,
char *tgtPtr, AttributeDetails* tgtEntry, short formatting, ErrorValue* &e);

Lng32 MXCI_RW_allocateHeap (void* SqlciEnv, Lng32 len, char* &ptr);

Lng32 MXCI_RW_deallocateHeap (void* SqlciEnv, Lng32 len, char* ptr);

Lng32 MXCI_RW_convertToDateFormat (void*SqlciEnv, char* srcPtr, AttributeDetails* srcEntry,
char *tgtPtr, AttributeDetails* tgtEntry, DateTimeFormat format, ErrorValue* &e);

Lng32 MXCI_RW_unicodeConvertToUpper(void *SqlciEnv, char *input_char, Lng32 num_of_input_bytes,
     char *output_char, Lng32& num_of_output_chars, ErrorValue* &e);


Lng32 MXCI_RW_unicodeConvertToLower(void *SqlciEnv, char *input_char, Lng32 num_of_input_bytes,
    char *output_char, Lng32& num_of_output_chars, ErrorValue* &e);



#endif // _RWINTERFACE_H_
