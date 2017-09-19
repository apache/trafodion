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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Prepare.C
 * RCS:          $Id: Prepare.cpp,v 1.5 1998/09/07 21:49:48  Exp $
 * Description:  
 *
 * Created:      4/15/95
 * Modified:     $ $Date: 1998/09/07 21:49:48 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include "Prepare.h"
#include "str.h"
#include "BaseTypes.h"
#include "dfs2rec.h"
  
//////////////////////////////////////////////////////////////////////
PrepEntry::PrepEntry(Lng32 datatype, Lng32 length, Lng32 scale, Lng32 precision,
		     Lng32 nullFlag, char * heading, Lng32 headingLen,
		     char * outputName, Lng32 outputNameLen, Lng32 displayLen,
                     Lng32 displayBufLen, Lng32 charsetEnum , char * tableName, Lng32 tableLen)
: datatype_(datatype), length_(length), scale_(scale),
  precision_(precision), nullFlag_(nullFlag), 
  headingLen_(headingLen), outputNameLen_(outputNameLen),
  displayLen_(displayLen), displayBufLen_(displayBufLen),
  heading_(NULL), outputName_(NULL), charsetEnum_(charsetEnum), 
  tableName_(NULL), tableLen_(tableLen),
  varPtr_(NULL), indPtr_(NULL)
{
  if (headingLen > 0)
    {
      heading_ = new char[headingLen_ + 1];
      strncpy(heading_, heading, headingLen);
      heading_[headingLen] = 0;
    }

  if (outputNameLen > 0)
    {
      outputName_ = new char[outputNameLen_ + 1];
      strncpy(outputName_, outputName, outputNameLen);
      outputName_[outputNameLen] = 0;
    }

  if (tableLen > 0)
  {
    tableName_= new char[tableLen_ + 1];
    strncpy(tableName_, tableName, tableLen);
    tableName_[tableLen] = 0;
  }

  if (DFS2REC::isAnyCharacter(datatype))
    {
      charset_ = new char[200]; // 200 is just a number.

      switch (charsetEnum)
	{
	case SQLCHARSETCODE_UNKNOWN:
	  delete [] charset_;
	  charset_ = 0;
	  break;
	  
	case SQLCHARSETCODE_ISO88591:
	  strcpy(charset_, SQLCHARSETSTRING_ISO88591);
	  break;
	  
	case SQLCHARSETCODE_KANJI:
	  strcpy(charset_, SQLCHARSETSTRING_KANJI);
	  break;
	  
	case SQLCHARSETCODE_KSC5601:
	  strcpy(charset_, SQLCHARSETSTRING_KSC5601);
	  break;
	  
	case SQLCHARSETCODE_UCS2:
	  strcpy(charset_, SQLCHARSETSTRING_UNICODE);
	  break;
	  
	default:
	  break;
	}
    }
  else
    charset_ = 0;
}

PrepEntry::~PrepEntry()
{
  if (heading_)
    delete [] heading_;

  if (outputName_)
    delete [] outputName_;

  if (tableName_)
    delete [] tableName_;

  if (charset_)
    delete [] charset_;
}


//////////////////////////////////////////////////////////////////////
PrepStmt::PrepStmt(const char * stmt_name_, dml_type type_)

{
  stmt_name = new char[strlen(stmt_name_) + 1];
  strcpy(stmt_name, stmt_name_);

  stmt_name_len = strlen(stmt_name);
 
  stmt_str = 0;
  sql_src = 0;
  stmt = 0;
  input_desc = 0;
  output_desc = 0;
  
  type = type_;

  queryType_ = SQL_UNKNOWN;
  subqueryType_ = SQL_STMT_NA;

  numInputEntries_ = 0;
  numOutputEntries_ = 0;

  outputEntries_ = NULL;
  inputEntries_  = NULL;

  outputDatalen_ = 0;
  outputBuflen_  = 0;

  outputData_ = NULL;
  outputBuf_  = NULL;

  uniqueQueryId_ = NULL;
  uniqueQueryIdLen_ = 0;
}

PrepStmt::~PrepStmt()
{
  remove();
}

void PrepStmt::remove()
{
  delete [] stmt_name;
  stmt_name = 0;
  delete [] stmt_str;
  stmt_str = 0;
  delete sql_src;
  sql_src = 0;
  delete stmt;
  stmt = 0;
  delete input_desc;
  input_desc = 0;
  delete output_desc;
  output_desc = 0;
  delete [] outputData_;
  outputData_ = 0;
  delete [] outputBuf_;
  outputBuf_ = 0;

  // delete input and output entries. TBD.
  if (inputEntries_)
    {
      for (Int32 i = 0; i < numInputEntries_; i++)
	{
	  delete inputEntries_[i];
	}

      delete [] inputEntries_;
      inputEntries_ = 0;
    }

  if (outputEntries_)
    {
      for (Int32 i = 0; i < numOutputEntries_; i++)
	{
	  delete outputEntries_[i];
	}

      delete [] outputEntries_;
      outputEntries_ = 0;
    }
}

void PrepStmt::set(const char * str, 
		   SQLDESC_ID * sql_src_, SQLSTMT_ID * stmt_,
		   Lng32 numInputEntries, SQLDESC_ID * input_desc_, 
		   Lng32 numOutputEntries, SQLDESC_ID * output_desc_)
{
  if ((! stmt_str) || (stmt_str != str))
    {
      delete [] stmt_str;
      stmt_str = new char[strlen(str) + 1];
      strcpy(stmt_str, str);
    }

  stmt_str_len = strlen(str);
  
  sql_src = sql_src_;
  stmt = stmt_;

  numInputEntries_  = numInputEntries;
  numOutputEntries_ = numOutputEntries;

  input_desc = input_desc_;
  output_desc = output_desc_;
}

short PrepStmt::contains(const char * value) const
{
  if ( stmt_name_len != strlen(value) )
    return 0;

  if (str_cmp(stmt_name, value, stmt_name_len) == 0)
    return -1;
  else
    return 0;
}

CursorStmt::CursorStmt(char * cursorName, 
		       SQLSTMT_ID * cursorStmtId,
		       PrepStmt * stmt,
		       Int16 internallyPrepared)
     : cursorStmtId_(cursorStmtId),
       stmt_(stmt),
       internallyPrepared_(internallyPrepared),
       resultSetIndex_(0)
{
  cursorName_ = new char[strlen(cursorName) + 1];
  strcpy(cursorName_, cursorName);
  cursorNameLen_ = strlen(cursorName_);
}

CursorStmt::~CursorStmt()
{
  delete [] cursorName_;
  delete cursorStmtId_;
  if (internallyPrepared_)
    delete stmt_;
}

short CursorStmt::contains(const char * value) const
{
  if ( cursorNameLen_ != strlen(value) )
    return 0;

  if (str_cmp(cursorName_, value, cursorNameLen_) == 0)
    return -1;
  else
    return 0;
}

