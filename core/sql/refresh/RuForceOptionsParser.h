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
#ifndef _FORCE_OPTIONS_PARSER_H_
#define _FORCE_OPTIONS_PARSER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuForceOptionsParser.h
* Description:  Definition of class CRUForceOptionsParser
*
*
* Created:      01/24/2002
* Language:     C++
*
* 
******************************************************************************
*/
// Updated 2/3/02

#include "RuForceOptions.h"
#include "dsstdiofile.h"
class CDSString;
class CRUTableForceOptions;
class CRUTableForceOptionsList;
class CRUMVForceOptions;
class CRUMVForceOptionsList;


//----------------------------------------------------------------------------
// CForceOptionsParser
// Reads an input file, parse it and create a CForceOptions object
// that continst the data from the file.
//----------------------------------------------------------------------------
// The input file format:
//
// MV [mv_name]
//
// GROUPBY = HASH / SORT 
// JOIN = NESTED / MERGE / HASH
// MDAM = ENABLE / ON / OFF
// TABLE table_name MDAM = ENABLE / ON / OFF
//
// Remarks:
// --------
// 1. table_name can be a *, which mean define one MDAM option for all the tables under the MV
// 2. the order of the 4 options commands is not restricted
//
// Rules:
// ------
//
// line --> MVExpr
//	MVExpr --> MV mv_name
// line --> RelExpr
//	RelExpr --> "GROUPBY =" GBOptions
//	RelExpr --> "JOIN =" JoinOptions
//		GBOptions --> "HASH"
//		GBOptions --> "SORT"
//		JoinOptions --> "NESTED"
//		JoinOptions --> "MERGE"
//		JoinOptions --> "HASH"
// line --> MV MDAMClause
// line --> TableExpr
//	TableExpr --> "TABLE" table_name MDAMClause
//
//  MDAMClause --> "MDAM = " MDAMOptions
//	MDAMOptions --> "ENABLE"
//	MDAMOptions --> "ON"
//	MDAMOptions --> "OFF"
// line --> ExplainOption
//		ExplainOption --> "EXPLAIN = ON"
//		ExplainOption --> "EXPLAIN = OFF"
// line --> FORCE_CQS = sql_stmt
//		sql_stmt --> (one line statement that ends with ;)


class REFRESH_LIB_CLASS CRUForceOptionsParser 
{
	//----------------------------------//
	//	Public Members
	//----------------------------------//	
public:
	
	CRUForceOptionsParser();
	virtual ~CRUForceOptionsParser();
	
public:

	//--------------------------------------------//
	// Accessors
	//--------------------------------------------//
	CRUForceOptions& GetForceOptions() { return forceOptions_; }

	//--------------------------------------------//
	// Mutators
	//--------------------------------------------//

	void SetFile(const CDSString& fileName);
	void Parse();

	//----------------------------------//
	//	Private Members
	//----------------------------------//	

private:
	enum { MAX_BUFFER_SIZE = 256 };

	enum ParsingState
	{
		STATE_INIT,
		STATE_NEXT_MV,
		STATE_NEXT_FORCE_OPT,
		STATE_GB,
		STATE_JOIN,
		STATE_MV_MDAM,
		STATE_TABLE,
		STATE_EXPLAIN,
		STATE_CQS,
		STATE_EOF
	};

	//--------------------------------------------//
	// Mutators
	//--------------------------------------------//
private:
	// The automata's states handlers functions
	void InitStateHandler();

	void NextMVStateHandler();

	void NextForceOptionStateHandler();

	void GBClauseHandler();

	void JoinClauseHandler();

	void MvMdamClauseHandler();

	void TableClauseHandler();

	void ExplainClauseHandler();

	void CQSClauseHandler();

private:
	// Utility functions
	
	const char* GetCurrentToken() const { return currentToken_; }
	
	// Read the next word and return the mdam option it represent
	CRUForceOptions::MdamOptions GetMDAMOption();
	
	void HandleStarOptions();
	void HandleTableOptions();

	// Read the next word from the buffer if the buffer is not empty else
	// fill the buffer and then read the first word in the buffer.
	void GetNextWord();
	// Fill the buffer with a new line
	void ReadNextLine();
	void UpCase(char*);
	// Find the first token and copy it to currentToken_
	BOOL GetNextToken();

	// Check if the current word is equal to the given parameter
	BOOL IsCurrentWord(const char* word);
	
	CDSString GetQualifiedName() const;

	// throws a bad format exception
	void ThrowBadFormat();

private:
	CRUForceOptions forceOptions_;
	CDSString* pFileName_;
	CDSStdioFile forceFile_;
	ParsingState state_;
	CRUMVForceOptions* pCurrentMV_;
	char  buffer_[MAX_BUFFER_SIZE];
	char  currentToken_[MAX_BUFFER_SIZE]; // holds the current parsed word
	char  *pCurrentChar_; // The next character in the buffer to parse
	BOOL bufferIsEmpty_; // Is the buffer empty?
	Int32 lineNumber_;
};


#endif
