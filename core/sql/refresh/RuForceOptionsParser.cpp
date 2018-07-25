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
/*
******************************************************************************
*
* File:         ForceOptionsParser.cpp
* Description:  Force options' parser
*				
* Created:      12/16/2001
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"
#include "RuException.h"
#include "dsstring.h"
#include "dsstdiofile.h"
#include "RuForceOptionsParser.h"
#include "dsAnsiSQLName.h"
#include "RuGlobals.h"
#include "RuOptions.h"


//----------------------------------------------------------------------------
//Destructor
//----------------------------------------------------------------------------

CRUForceOptionsParser::CRUForceOptionsParser() :
	pFileName_(NULL),
	state_(STATE_INIT),
	pCurrentMV_(NULL),
	pCurrentChar_(NULL),
	bufferIsEmpty_(TRUE),
	lineNumber_(0)
{}


CRUForceOptionsParser::~CRUForceOptionsParser() 
{
	if (NULL != pFileName_)
	{
		delete pFileName_;
	}
}


//----------------------------------------------------------------------------
// CRUForceOptionsParser::SetFile
//----------------------------------------------------------------------------
void CRUForceOptionsParser::SetFile(const CDSString& file) 
{
	if (pFileName_!=NULL)
	{
		delete pFileName_;
	}

	pFileName_ = new CDSString(file);
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::InitStateHandler()
//
// Open the file and check for errors
//----------------------------------------------------------------------------
void CRUForceOptionsParser::InitStateHandler()
{
	CDSStdioFile::EOpenMode openMode=CDSStdioFile::eRead;

	BOOL result = forceFile_.Open(*pFileName_,openMode);
	
	if (FALSE == result) 
	{
		CDSException e;
		e.SetError(IDS_RU_OPEN_FORCEFILE);
		e.AddArgument(*pFileName_);
		throw e;
	}

	GetNextWord(); // get the first word

	if (state_ == STATE_EOF)
	{
		return;
	}
	
	if (IsCurrentWord("MV"))
	{
		state_ = STATE_NEXT_MV;
	}
	else
	{
		ThrowBadFormat();
	}
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::ThrowBadFormat()
//
// Throw a syntax error exception
//----------------------------------------------------------------------------
void CRUForceOptionsParser::ThrowBadFormat()
{
	CDSException e;
	e.SetError(IDS_RU_BAD_FORMAT_FORCEFILE);
	e.AddArgument(*pFileName_);
	e.AddArgument(GetCurrentToken());
	e.AddArgument(lineNumber_);
	throw e;
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::Parse
//----------------------------------------------------------------------------

void CRUForceOptionsParser::Parse() 
{
	while ( STATE_EOF != state_) 
	{	
		switch(state_)
		{
		case CRUForceOptionsParser::STATE_INIT : 
		{
			InitStateHandler();
			break;
		}
		case CRUForceOptionsParser::STATE_NEXT_MV : 
		{
			NextMVStateHandler();
			break;
		}
		case CRUForceOptionsParser::STATE_NEXT_FORCE_OPT : 
		{
			NextForceOptionStateHandler();
			break;
		}
		case CRUForceOptionsParser::STATE_GB : 
		{
			GBClauseHandler();
			break;
		}
		case CRUForceOptionsParser::STATE_JOIN :
		{
			JoinClauseHandler();
			break;
		}
		case CRUForceOptionsParser::STATE_MV_MDAM :
		{
			MvMdamClauseHandler();
			break;
		}
		case CRUForceOptionsParser::STATE_TABLE :
		{
			TableClauseHandler();
			break;
		}
		case CRUForceOptionsParser::STATE_EXPLAIN :
		{
			ExplainClauseHandler();
			break;
		}
		case CRUForceOptionsParser::STATE_CQS :
		{
			CQSClauseHandler();
			break;
		}
		default :
			RUASSERT(FALSE);
		}
	}
	
	forceFile_.Close();
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::NextForceOptionStateHandler()
//
// Read next the next mv, 
//----------------------------------------------------------------------------

void CRUForceOptionsParser::NextMVStateHandler() 
{
	RUASSERT(IsCurrentWord("MV"));  // cerr<<"Syntax error: MV's name decleration expected"
	
	GetNextWord(); // fetch the MV's name
	
	CDSString mvName = GetQualifiedName();

	if 	(TRUE == forceOptions_.IsMVExist(mvName))
	{
		CDSException e;
		e.SetError(IDS_RU_FORCEFILE_DUP_MV);
		e.AddArgument(mvName);
		e.AddArgument(*pFileName_);
		throw e;
	}

	pCurrentMV_ = new CRUMVForceOptions();

	pCurrentMV_->SetMVName(mvName);

	forceOptions_.AddMV(pCurrentMV_);

	state_ = STATE_NEXT_FORCE_OPT;
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::ReadForceOption()
//
// Check which force options are used for the current mv 
//----------------------------------------------------------------------------
void CRUForceOptionsParser::NextForceOptionStateHandler()
{
	GetNextWord();

	if (state_ == STATE_EOF)
	{
		return;
	}

	if (IsCurrentWord("MV"))
	{
		state_ = STATE_NEXT_MV;
		return;
	}

	if (IsCurrentWord("GROUPBY"))
	{
		state_ = STATE_GB;
		return;
	}

	if (IsCurrentWord("JOIN"))
	{
		state_ = STATE_JOIN;
		return;
	}

	if (IsCurrentWord("MDAM"))
	{
		state_ = STATE_MV_MDAM;
		return;
	}

	if (IsCurrentWord("TABLE"))
	{
		state_ = STATE_TABLE;
		return;
	}

	if (IsCurrentWord("EXPLAIN"))
	{
		state_ = STATE_EXPLAIN;
		return;
	}

	if (IsCurrentWord("FORCE_CQS"))
	{
		state_ = STATE_CQS;
		return;
	}

	ThrowBadFormat();
}

//----------------------------------------------------------------------------
//	CRUForceOptionsParser::GBClauseHandler
//
//	RelExpr --> "GROUPBY =" GBOptions
//
//	GBOptions --> "HASH"
//	GBOptions --> "SORT"
//----------------------------------------------------------------------------

void CRUForceOptionsParser::GBClauseHandler() 
{
	RUASSERT(IsCurrentWord("GROUPBY"));
		
	CRUForceOptions::GroupByOptions result = CRUForceOptions::GB_NO_FORCE;
	
	GetNextWord(); // fetch the = operator

	if (!IsCurrentWord("="))
	{
		ThrowBadFormat();
	}

	GetNextWord(); // fetch the GB option

	if (IsCurrentWord("HASH"))
	{
		result = CRUForceOptions::GB_HASH;
	}

	if (IsCurrentWord("SORT"))
	{
		result = CRUForceOptions::GB_SORT;
	}
	
	if (CRUForceOptions::GB_NO_FORCE == result)
	{
		ThrowBadFormat();
	}

	pCurrentMV_->SetGroupBy(result);

	state_= STATE_NEXT_FORCE_OPT;
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::JoinClauseHandler
//
//	RelExpr --> "JOIN =" JoinOptions
//
//	JoinOptions --> "NESTED"
//	JoinOptions --> "MERGE"
//	JoinOptions --> "HASH"
//----------------------------------------------------------------------------

void CRUForceOptionsParser::JoinClauseHandler() 
{
	RUASSERT(IsCurrentWord("JOIN"));
		
	CRUForceOptions::JoinOptions result = CRUForceOptions::JOIN_NO_FORCE;
	
	GetNextWord(); // fetch the = operator

	if (!IsCurrentWord("="))
	{
		ThrowBadFormat();
	}

	GetNextWord(); // fetch the GB option

	if (IsCurrentWord("NESTED"))
	{
		result = CRUForceOptions::JOIN_NESTED;
	}

	if (IsCurrentWord("MERGE"))
	{
		result = CRUForceOptions::JOIN_MERGE;
	}

	if (IsCurrentWord("HASH"))
	{
		result = CRUForceOptions::JOIN_HASH;
	}
	
	if (CRUForceOptions::JOIN_NO_FORCE == result)
	{
		ThrowBadFormat();
	}

	pCurrentMV_->SetJoin(result);

	state_= STATE_NEXT_FORCE_OPT;
}

//----------------------------------------------------------------------------
//	CRUForceOptionsParser::MvMdamClauseHandler()
//
//  MDAMClause --> "MDAM = " MDAMOptions
//
//	MDAMOptions --> "ENABLE"
//	MDAMOptions --> "ON"
//	MDAMOptions --> "OFF" 
//----------------------------------------------------------------------------

void CRUForceOptionsParser::MvMdamClauseHandler() 
{
	RUASSERT(IsCurrentWord("MDAM"));

	CRUForceOptions::MdamOptions result = CRUForceOptions::MDAM_NO_FORCE;
	
	result= GetMDAMOption();

	pCurrentMV_->SetMdam(result);

	state_ = STATE_NEXT_FORCE_OPT;
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::TableStateHandler
//----------------------------------------------------------------------------

void CRUForceOptionsParser::TableClauseHandler() 
{
	RUASSERT(IsCurrentWord("TABLE"));

	GetNextWord(); // fetch the table name

	if (IsCurrentWord("*"))
	{
		HandleStarOptions();
	}
	else
	{
		HandleTableOptions();
	}

	state_ = STATE_NEXT_FORCE_OPT;
}


//----------------------------------------------------------------------------
// CRUForceOptionsParser::GetMDAMOption()
//----------------------------------------------------------------------------

CRUForceOptions::MdamOptions CRUForceOptionsParser::GetMDAMOption()
{
	RUASSERT(IsCurrentWord("MDAM"));

	CRUForceOptions::MdamOptions result = CRUForceOptions::MDAM_NO_FORCE;

	GetNextWord(); // fetch the = operator

	if (!IsCurrentWord("="))
	{
		ThrowBadFormat();
	}

	GetNextWord(); // fetch the mdam option

	if (IsCurrentWord("ON"))
	{
		result = CRUForceOptions::MDAM_ON;
	}
	
	if (IsCurrentWord("OFF"))
	{
		result = CRUForceOptions::MDAM_OFF;
	}

	if (IsCurrentWord("ENABLE"))
	{
		result = CRUForceOptions::MDAM_ENABLE;
	}

	if (CRUForceOptions::MDAM_NO_FORCE == result)
	{
		ThrowBadFormat();
	}
	
	return result;
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::HandleStarOptions()
//----------------------------------------------------------------------------

void CRUForceOptionsParser::HandleStarOptions()
{
	RUASSERT(IsCurrentWord("*") && STATE_TABLE == state_);

	if(0 != pCurrentMV_->GetNumOfTables()); 
	{
		CRUException ex;
		ex.SetError(IDS_RU_FORCE_FILE_STAR_TABLE);
		ex.AddArgument(pCurrentMV_->GetMVName());
		throw ex;
	}

	CRUForceOptions::MdamOptions result = GetMDAMOption();

	pCurrentMV_->SetUsedTableStarOption(result);
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::HandleTableOptions()
//----------------------------------------------------------------------------

void CRUForceOptionsParser::HandleTableOptions()
{
	RUASSERT(STATE_TABLE == state_);

	CDSString tableName = GetQualifiedName();

	if(CRUForceOptions::MDAM_NO_FORCE != pCurrentMV_->GetForceMdamOptionForTable(tableName))
	{
		CRUException ex;
		ex.SetError(IDS_RU_FORCE_FILE_STAR_TABLE);
		ex.AddArgument(pCurrentMV_->GetMVName());
		throw ex;
	}

	GetNextWord(); // fetch the = operator
	
	CRUForceOptions::MdamOptions result = GetMDAMOption();
	
	CRUTableForceOptions *pNewTableOpt = new CRUTableForceOptions(tableName);

	pNewTableOpt->SetMdam(result);

	pCurrentMV_->AddTable(pNewTableOpt);
}	
	
//----------------------------------------------------------------------------
//	CRUForceOptionsParser::GBClauseHandler
//
// line --> ExplainOption
//		ExplainOption --> "EXPLAIN = ON"
//		ExplainOption --> "EXPLAIN = OFF"
//----------------------------------------------------------------------------

void CRUForceOptionsParser::ExplainClauseHandler() 
{
	RUASSERT(IsCurrentWord("EXPLAIN"));
		
	CRUForceOptions::ExplainOptions result = CRUForceOptions::EXPLAIN_OFF;

	GetNextWord();

	if (!IsCurrentWord("="))
	{
		ThrowBadFormat();
	}

	GetNextWord(); // fetch the explain option

	if (IsCurrentWord("ON"))
	{
		result = CRUForceOptions::EXPLAIN_ON;
	}
	else
	{
		if (!IsCurrentWord("OFF"))
		{
			ThrowBadFormat();
		}
	}

	pCurrentMV_->SetExplainOption(result);

	state_= STATE_NEXT_FORCE_OPT;
}

//----------------------------------------------------------------------------
//	CRUForceOptionsParser::GBClauseHandler
//
// line --> ExplainOption
//		ExplainOption --> "EXPLAIN = ON"
//		ExplainOption --> "EXPLAIN = OFF"
//----------------------------------------------------------------------------

void CRUForceOptionsParser::CQSClauseHandler() 
{
	RUASSERT(IsCurrentWord("FORCE_CQS"));

	GetNextWord();

	if (!IsCurrentWord("="))
	{
		ThrowBadFormat();
	}

	CDSString sqlStmt;
	CDSString currentToken;
	do 
	{
		GetNextWord();
		currentToken = GetCurrentToken();
		sqlStmt += " " + currentToken;

	} while (FALSE == IsCurrentWord(";") && STATE_EOF != state_);

	if (STATE_EOF != state_)
	{
		pCurrentMV_->SetCQSStatment(sqlStmt);
		state_= STATE_NEXT_FORCE_OPT;
	}
	else
	{
		CDSException e;
		e.SetError(IDS_RU_FORCE_BAD_CQS);
		e.AddArgument(pCurrentMV_->GetMVName());
		throw e;
	}
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::ReadNextLine()
//
// Fill the buffer with the content of a new non empty line and normalize it
// also check for EOF
//----------------------------------------------------------------------------
void CRUForceOptionsParser::ReadNextLine()
{
	BOOL ret = forceFile_.ReadLine(buffer_,MAX_BUFFER_SIZE); //reading a line from the input file   

	if (FALSE == ret)
	{
		if (forceFile_.IsEOF())
		{
			state_ = STATE_EOF;
			return;
		}
		
		CRUException ex;
		ex.SetError(IDS_RU_CORRUPTED_FORCE_FILE);
		ex.AddArgument(*pFileName_);
		throw ex;
	}
	
	UpCase(buffer_);
	lineNumber_++;
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::GetNextToken()
//
//
//----------------------------------------------------------------------------
BOOL CRUForceOptionsParser::GetNextToken()
{
	if ('\0' == *pCurrentChar_)
		return FALSE;

	// remove leading spaces
	while(isspace((unsigned char)*pCurrentChar_))  // For VS2003
	{
		pCurrentChar_++;

		if ('\0' == *pCurrentChar_)
			return FALSE;
	} 
	
	Int32 i=0;
	do 
	{ 
		// copy characters from the buffer to the current token.
		currentToken_[i++] = *pCurrentChar_++;
		
		// Deals with non space seperator (Only '=' or ';') 
		if ('=' == (*pCurrentChar_) || 
			';' == (*pCurrentChar_) ||
			isspace((unsigned char)*pCurrentChar_) || // For VS2003
			'\0' == (*pCurrentChar_))
		{
		    break;
		}

		RUASSERT(i < MAX_BUFFER_SIZE);

	} while (1);

	currentToken_[i] = '\0';

	return TRUE;
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::GetNextWord()
//----------------------------------------------------------------------------
void CRUForceOptionsParser::GetNextWord()
{
	do 
	{
		if (TRUE == bufferIsEmpty_)
		{
			ReadNextLine();
			bufferIsEmpty_ = FALSE;
			pCurrentChar_ = buffer_;
		}

		bufferIsEmpty_ = !GetNextToken();

	} while (TRUE == bufferIsEmpty_ && STATE_EOF != state_);
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::IsCurrentWord
//----------------------------------------------------------------------------	
BOOL CRUForceOptionsParser::IsCurrentWord(const char* word)
{
	return 0 == strcmp(currentToken_,word);
}

//----------------------------------------------------------------------------
// CRUForceOptionsParser::UpCase
//----------------------------------------------------------------------------	

void CRUForceOptionsParser::UpCase(char* aLine)
{
	Int32 len = strlen(aLine);
	Int32 i;
	for (i=0; i<len; i++)
	{
		if((aLine[i]>='a') && (aLine[i]<='z'))
		{
			aLine[i] -= 32;
		}
	}
}


//----------------------------------------------------------------------------
// CRUForceOptionsParser::GetQualifiedName()
// 
// Parse the current token into an object name
//----------------------------------------------------------------------------	

CDSString CRUForceOptionsParser::GetQualifiedName() const
{
	CDsAnsiSQLName ansiName;

	ansiName.SetDefaultSchema(CRUGlobals::GetInstance()->GetOptions().GetDefaultSchema());
	ansiName.SetName(currentToken_);

	ansiName.Parse();

	return ansiName.GetCatalogName() + "." +
		   ansiName.GetSchemaName() + "." +
		   ansiName.GetObjectName();
}

