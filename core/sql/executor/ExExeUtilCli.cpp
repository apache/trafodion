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
**********************************************************************/

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExExeUtilCli.cpp
 * Description:  
 *               
 *               
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

//#include "ComCextdecs.h"
#include  "cli_stdh.h"
#include  "ex_stdh.h"
#include  "sql_id.h"
#include  "ComSqlId.h"
#include  "ExExeUtilCli.h"
OutputInfo::OutputInfo(Lng32 numEntries)
  : numEntries_(numEntries)
{
  
   ex_assert( numEntries <= MAX_OUTPUT_ENTRIES, "try to fetch more than max columns allowed");

   for (Int32 i = 0; i < numEntries_; i++)
    {
      data_[i] = NULL;
      len_[i] = 0;
    }
}

void OutputInfo::insert(Lng32 index, char * data)
{
  data_[index] = data;
}

void OutputInfo::insert(Lng32 index, char * data, Lng32 len)
{
  data_[index] = data;
  len_[index] = len;
}

void OutputInfo::insert(Lng32 index, char * data, Lng32 len, Lng32 type, Lng32 *indOffset , Lng32 *varOffset )
{
  data_[index] = data;
  len_[index] = len;
  type_[index] = type;
}

char * OutputInfo::get(Lng32 index)
{
  if (index < numEntries_)
    return data_[index];
  else
    return NULL;
}

short OutputInfo::get(Lng32 index, char* &data, Lng32 &len)
{
  if (index < numEntries_)
    {
      data = data_[index];
      len = len_[index];
      return 0;
    }

  return -1;
}

short OutputInfo::get(Lng32 index, char* &data, Lng32 &len, Lng32 &type, Lng32 *indOffset , Lng32 *varOffset )
{
  if (index < numEntries_)
    {
      data = data_[index];
      len = len_[index];
      type = type_[index];
      return 0;
    }

  return -1;
}

void OutputInfo::dealloc(CollHeap * heap)
{
  for (Int32 i = 0; i < numEntries_; i++)
    {
      if(data_[i] != NULL)
        NADELETEBASIC(data_[i], heap);
    }
}

/////////////////////////////////////////////////////////////////
// class ExeCliInterface
/////////////////////////////////////////////////////////////////
ExeCliInterface::ExeCliInterface(CollHeap * heap, Int32 isoMapping,
				 ContextCli * currContext,
                                 const char *parentQid)
     : heap_(heap),
       module_(NULL),
       stmt_(NULL),
       sql_src_(NULL),
       input_desc_(NULL),
       output_desc_(NULL),
       rs_input_maxsize_desc_(NULL),
       outputBuf_(NULL),
       inputBuf_(NULL),
       currContext_(currContext),
       moduleWithCK_(NULL),
       stmtWithCK_(NULL),
       sql_src_withCK_(NULL),
       input_desc_withCK_(NULL),
       output_desc_withCK_(NULL),
       outputBuf_withCK_(NULL),
       rsInputBuffer_(NULL),
       isoMapping_((Int32)SQLCHARSETCODE_ISO88591), // ISO_MAPPING=ISO88591
       parentQid_(parentQid),
       inputAttrs_(NULL),
       outputAttrs_(NULL),
       explainData_(NULL),
       explainDataLen_(0),
       flags_(0)
{
  if (parentQid_)
  {
    Int32 len = str_len(parentQid_);
    ex_assert( len >= ComSqlId::MIN_QUERY_ID_LEN, "parentQid too short.");
    ex_assert( len <= ComSqlId::MAX_QUERY_ID_LEN, "parentQid too long.");
    ex_assert( !str_cmp(parentQid_, COM_SESSION_ID_PREFIX, 4), 
               "invalid parentQid.");
  }  
}

ExeCliInterface::~ExeCliInterface()
{
   dealloc();
}

Lng32 ExeCliInterface::deallocStuff(SQLMODULE_ID * &module,
				SQLSTMT_ID * &stmt,
				SQLDESC_ID * &sql_src,
				SQLDESC_ID * &input_desc,
				SQLDESC_ID * &output_desc)
{
  if (sql_src)
    {
      SQL_EXEC_DeallocDesc(sql_src);
      NADELETEBASIC(sql_src, heap_);
      sql_src = NULL;
    }

  if (stmt)
    {
      SQL_EXEC_DeallocStmt(stmt);
      NADELETEBASIC(stmt, heap_);
      stmt = NULL;
    }

  if (input_desc)
    {
      SQL_EXEC_DeallocDesc(input_desc);
      NADELETEBASIC(input_desc, heap_);
      input_desc = NULL;
    }

  if (output_desc)
    {
      SQL_EXEC_DeallocDesc(output_desc);
      NADELETEBASIC(output_desc, heap_);
      output_desc = NULL;
    }

  if (module)
    {
      NADELETEBASIC(module, heap_);
      module = NULL;
    }

  if (rsInputBuffer_)
    {
      NADELETEBASIC(rsInputBuffer_, heap_);
      rsInputBuffer_ = NULL;
    }

  if (outputAttrs_)
    {
      NADELETEBASIC(outputAttrs_, heap_);
      outputAttrs_ = NULL;
    }

  if (outputBuf_)
    {
      NADELETEBASIC(outputBuf_, heap_);
      outputBuf_ = NULL;
    }

  return 0;
}

Lng32 ExeCliInterface::dealloc()
{
  return deallocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_);
}

void ExeCliInterface::clearGlobalDiags()
{
  SQL_EXEC_ClearDiagnostics(NULL);
}

Lng32 ExeCliInterface::allocStuff(SQLMODULE_ID * &module,
				  SQLSTMT_ID * &stmt,
				  SQLDESC_ID * &sql_src,
				  SQLDESC_ID * &input_desc,
				  SQLDESC_ID * &output_desc,
				  const char * stmtName
  )
{
  Lng32 retcode = 0;
  deallocStuff(module, stmt, sql_src, input_desc, output_desc);

  clearGlobalDiags();

  module = new(heap_) SQLMODULE_ID;
  init_SQLMODULE_ID(module);

  // Allocate a SQL statement
  stmt = new(heap_) SQLSTMT_ID;
  if (stmtName)
    {
      init_SQLSTMT_ID(stmt, SQLCLI_CURRENT_VERSION, stmt_name, module);
      stmt->identifier = 
	new(heap_) char[strlen(stmtName) +1];
      strcpy((char*)stmt->identifier, stmtName);
      stmt->identifier_len = (Lng32)strlen(stmtName);
    }
  else
    init_SQLSTMT_ID(stmt, SQLCLI_CURRENT_VERSION, stmt_handle, module);

  retcode = SQL_EXEC_AllocStmt(stmt, 0);
  if (retcode < 0)
    return retcode;

  // Allocate a descriptor which will hold the SQL statement source
  sql_src = new(heap_) SQLSTMT_ID;
  init_SQLDESC_ID(sql_src, SQLCLI_CURRENT_VERSION, desc_handle, module);

  retcode = SQL_EXEC_AllocDesc(sql_src, 1);
  if (retcode < 0)
    return retcode;

  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_TYPE_FS,
				 REC_BYTE_V_ANSI, 0);
  if (retcode < 0)
    return retcode;

  // Allocate a descriptor which will hold the SQL statement input
  input_desc = new(heap_) SQLSTMT_ID;
  init_SQLDESC_ID(input_desc, SQLCLI_CURRENT_VERSION, desc_handle, module);

  retcode = SQL_EXEC_AllocDesc(input_desc, 1);
  if (retcode < 0)
    return retcode;

  // Allocate a descriptor which will hold the SQL statement output
  output_desc = new(heap_) SQLSTMT_ID;
  init_SQLDESC_ID(output_desc, SQLCLI_CURRENT_VERSION, desc_handle, module);

  retcode = SQL_EXEC_AllocDesc(output_desc, 1);
  if (retcode < 0)
    return retcode;

  return 0;
}

Lng32 ExeCliInterface::prepare(const char * stmtStr,
                              SQLMODULE_ID * module,
                              SQLSTMT_ID * stmt,
                              SQLDESC_ID * sql_src,
                              SQLDESC_ID * input_desc,
                              SQLDESC_ID * output_desc,
                              char ** outputBuf,
                              Queue * outputVarPtrList,
                              char ** inputBuf,
                              Queue * inputVarPtrList,
                              char *uniqueStmtId,
                              Lng32 *uniqueStmtIdLen,
                              SQL_QUERY_COST_INFO *query_cost_info, 
                              SQL_QUERY_COMPILER_STATS_INFO *comp_stats_info,
                              NABoolean monitorThis,
                              NABoolean doNotCachePlan)
{
  Lng32 retcode = 0;
  ULng32 prepFlags = 0;
  SQL_QUERY_COST_INFO local_query_cost_info;
  SQL_QUERY_COMPILER_STATS_INFO local_comp_stats_info;

  if (monitorThis)
    prepFlags |= PREPARE_MONITOR_THIS_QUERY;

  if ((currContext_) && (currContext_->getSessionDefaults()))
    {
      if(currContext_->getSessionDefaults()->callEmbeddedArkcmp())
	prepFlags |= PREPARE_USE_EMBEDDED_ARKCMP;
    }

  if (doNotCachePlan)
    {
      prepFlags |= PREPARE_NO_TEXT_CACHE;
      prepFlags |= PREPARE_DONT_CACHE;
    }

  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_LENGTH,
				 strlen(stmtStr) + 1, 0);
  if (retcode != SUCCESS)
    return retcode;
  retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_VAR_PTR,
				 (Long)stmtStr, 0);
  if (retcode != SUCCESS)
    return retcode;

    retcode = SQL_EXEC_SetDescItem(sql_src, 1, SQLDESC_CHAR_SET,  
                                   (Int32)SQLCHARSETCODE_UTF8,0);
  if (retcode != SUCCESS)
    return retcode;

  char parentQid[ComSqlId::MAX_QUERY_ID_LEN + 1];
  char *pQidPtr = NULL;
  if (parentQid_)
  {
    // The parentQid_ is a const char *, but the CLI doesn't 
    // want a const char * arg, so convert it safely here.
    memset(parentQid, 0, sizeof(parentQid));
    memcpy(parentQid,  parentQid_, ComSqlId::MAX_QUERY_ID_LEN);
    pQidPtr = parentQid;
  }
  retcode = SQL_EXEC_SetStmtAttr(stmt, SQL_ATTR_PARENT_QID, 0, pQidPtr);
  if (retcode < 0)
    return retcode;

  // set parserflags to indicate that this is an internal query from exeutil
  NABoolean flagWasSetHere = FALSE;
  if (currContext_)
    {
      if (((currContext_->getSqlParserFlags() & 0x20000) == 0) &&
	  (NOT notExeUtilInternalQuery())) // this is an exeutil internal query. This is the default.
	{
	  flagWasSetHere = TRUE;
	  currContext_->setSqlParserFlags(0x20000); // INTERNAL_QUERY_FROM EXEUTIL
	}
    }
   
  if (uniqueStmtId == NULL)
    retcode = SQL_EXEC_Prepare2(stmt, sql_src,NULL,0,NULL,
				(query_cost_info?query_cost_info:&local_query_cost_info), 
				(comp_stats_info?comp_stats_info:&local_comp_stats_info),
				NULL,0,prepFlags);
  else
  {
    prepFlags |= PREPARE_STANDALONE_QUERY;
    retcode = SQL_EXEC_Prepare2(stmt, sql_src, NULL, 0, NULL,
				(query_cost_info?query_cost_info:&local_query_cost_info), 
				(comp_stats_info?comp_stats_info:&local_comp_stats_info),
				uniqueStmtId, uniqueStmtIdLen, prepFlags);
  }
  // reset internal exeutil query parserflags if it was set in this method.
  if ((currContext_) && (flagWasSetHere))
    currContext_->resetSqlParserFlags(0x20000); // INTERNAL_QUERY_FROM EXEUTIL

  // return the error code that was returned from SQL_EXEC_Prepare
  if (retcode < 0)
    return retcode;

  // clear diagnostics to reset any warnings.
  SQL_EXEC_ClearDiagnostics(NULL);

  Lng32 num_input_entries, num_output_entries;
  retcode = SQL_EXEC_DescribeStmt(stmt, input_desc, output_desc);
  if (retcode != SUCCESS)
    return retcode;
  
  retcode = SQL_EXEC_GetDescEntryCount(input_desc, &num_input_entries);
  if (retcode != SUCCESS)
    return retcode;

  retcode = SQL_EXEC_GetDescEntryCount(output_desc, &num_output_entries);
  if (retcode != SUCCESS)
    return retcode;

  numInputEntries_ = num_input_entries;
  numOutputEntries_ = num_output_entries;

  inputAttrs_ = NULL;
  outputAttrs_ = NULL;
  if (numInputEntries_ > 0)
    inputAttrs_ = (Attrs*) (new(heap_) char[sizeof(Attrs) * numInputEntries_]);

  if (numOutputEntries_ > 0)
    outputAttrs_ = (Attrs*) (new(heap_) char[sizeof(Attrs) * numOutputEntries_]);

  short entry=1;
  Lng32 datatype = 0;
  Lng32 datacharset = 0;
  Lng32 length = 0;
  Lng32 null_flag = 0;
  Lng32 dataOffset = -1;
  Lng32 nullIndOffset = -1;

  outputDatalen_ = 0;
  inputDatalen_ = 0;
  for (; entry <= num_output_entries; entry++)
    {
      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
				     SQLDESC_TYPE_FS,
				     &datatype, 0, 0, 0, 0);
      if (retcode != SUCCESS)
	return retcode;

      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
				     SQLDESC_OCTET_LENGTH,
				     &length, 0, 0, 0, 0);
      if (retcode != SUCCESS)
	return retcode;

      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
				     SQLDESC_NULLABLE,
				     &null_flag, 0, 0, 0, 0);
      if (retcode != SUCCESS)
	return retcode;

      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
				     SQLDESC_CHAR_SET,
				     &datacharset, 0, 0, 0, 0);
      if (retcode != SUCCESS)
	return retcode;

      outputDatalen_ += length;
      if (null_flag)
	outputDatalen_ += SQL_NULL_HDR_SIZE;
      if (datatype == REC_BYTE_V_ASCII ||
	  datatype == REC_BYTE_V_ASCII_LONG ||
	  datatype == REC_BYTE_V_DOUBLE)
	outputDatalen_ += SQL_VARCHAR_HDR_SIZE;

      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
				     SQLDESC_NULL_IND_OFFSET,
				     &nullIndOffset, 0, 0, 0, 0);
      if (retcode != SUCCESS)
	return retcode;
      
      retcode = SQL_EXEC_GetDescItem(output_desc, entry,
				     SQLDESC_DATA_OFFSET,
				     &dataOffset, 0, 0, 0, 0);
      if (retcode != SUCCESS)
	return retcode;
      
      outputAttrs_[entry-1].fsDatatype_ = datatype;
      outputAttrs_[entry-1].length_ = length;
      outputAttrs_[entry-1].nullFlag_ = null_flag;
      outputAttrs_[entry-1].indOffset_ = nullIndOffset;
      outputAttrs_[entry-1].varOffset_ = dataOffset;
    }

  if (outputBuf)
    {
      if ((*outputBuf == NULL) &&
	  (outputDatalen_ > 0))
	*outputBuf = new (heap_) char[outputDatalen_];
      
      Lng32 curpos = 0;
      for (entry=1; entry <= num_output_entries; entry++)
	{
	  retcode = SQL_EXEC_GetDescItem(output_desc, entry,
					 SQLDESC_NULLABLE,
					 &null_flag, 0, 0, 0, 0);
	  if (retcode != SUCCESS)
	    return retcode;
	  
	  if (null_flag)
	    {
	      retcode = SQL_EXEC_SetDescItem(output_desc, entry,
					     SQLDESC_IND_PTR,
					     (Long)&(*outputBuf)[curpos],0);
	      if (retcode != SUCCESS)
		return retcode;

	      curpos += SQL_NULL_HDR_SIZE;
	    }

	  retcode = SQL_EXEC_SetDescItem(output_desc, entry,
					 SQLDESC_VAR_PTR,
					 (Long)&(*outputBuf)[curpos],0);
	  if (retcode != SUCCESS)
	    return retcode;
	  
	  retcode = SQL_EXEC_GetDescItem(output_desc, entry,
					 SQLDESC_OCTET_LENGTH,
					 &length, 0, 0, 0, 0);

          if (outputVarPtrList)
          {
            outputVarPtrList->insert((char*) &(*outputBuf)[curpos]);
          }

	  curpos += length;
	  
	  retcode = SQL_EXEC_GetDescItem(output_desc, entry,
					 SQLDESC_TYPE_FS,
					 &datatype, 0, 0, 0, 0);
	  if (retcode != SUCCESS)
	    return retcode;
	  
	  retcode = SQL_EXEC_GetDescItem(output_desc, entry,
					 SQLDESC_CHAR_SET,
					 &datacharset, 0, 0, 0, 0);
	  if (retcode != SUCCESS)
	    return retcode;

	  if (datatype == REC_BYTE_V_ASCII ||
	      datatype == REC_BYTE_V_ASCII_LONG ||
	      datatype == REC_BYTE_V_DOUBLE)
	    curpos += SQL_VARCHAR_HDR_SIZE;

	}
    }

  entry=1;
  inputDatalen_ = 0;
  for (; entry <= num_input_entries; entry++)
    {
      Lng32 alignedLen = 0;
      retcode = SQL_EXEC_GetDescItem(input_desc, entry,
				     SQLDESC_ALIGNED_LENGTH,
				     &alignedLen, 0, 0, 0, 0);
      if (retcode != SUCCESS)
	return retcode;

      inputDatalen_ += alignedLen;

      retcode = SQL_EXEC_GetDescItem(input_desc, entry,
				     SQLDESC_TYPE_FS,
				     &datatype, 0, 0, 0, 0);
      if (retcode != SUCCESS)
	return retcode;

      retcode = SQL_EXEC_GetDescItem(input_desc, entry,
				     SQLDESC_OCTET_LENGTH,
				     &length, 0, 0, 0, 0);
      if (retcode != SUCCESS)
	return retcode;

      retcode = SQL_EXEC_GetDescItem(input_desc, entry,
				     SQLDESC_NULLABLE,
				     &null_flag, 0, 0, 0, 0);
      if (retcode != SUCCESS)
	return retcode;

      retcode = SQL_EXEC_GetDescItem(input_desc, entry,
				     SQLDESC_NULL_IND_OFFSET,
				     &nullIndOffset, 0, 0, 0, 0);
      if (retcode != SUCCESS)
	return retcode;
      
      retcode = SQL_EXEC_GetDescItem(input_desc, entry,
				     SQLDESC_DATA_OFFSET,
				     &dataOffset, 0, 0, 0, 0);
      if (retcode != SUCCESS)
	return retcode;
      
      inputAttrs_[entry-1].fsDatatype_ = datatype;
      inputAttrs_[entry-1].length_ = length;
      inputAttrs_[entry-1].nullFlag_ = null_flag;
      inputAttrs_[entry-1].indOffset_ = nullIndOffset;
      inputAttrs_[entry-1].varOffset_ = dataOffset;
    }

  if (inputBuf)
    {
      if ((*inputBuf == NULL) &&
	  (inputDatalen_ > 0))
	*inputBuf = new (heap_) char[inputDatalen_];
      
      Lng32 curpos = 0;
      for (entry=1; entry <= num_input_entries; entry++)
	{
	  retcode = SQL_EXEC_GetDescItem(input_desc, entry,
					 SQLDESC_NULL_IND_OFFSET,
					 &nullIndOffset, 0, 0, 0, 0);
	  if (retcode != SUCCESS)
	    return retcode;

	  retcode = SQL_EXEC_GetDescItem(input_desc, entry,
					 SQLDESC_DATA_OFFSET,
					 &dataOffset, 0, 0, 0, 0);
	  if (retcode != SUCCESS)
	    return retcode;

	  if (nullIndOffset >= 0)
	    {
	      retcode = SQL_EXEC_SetDescItem(input_desc, entry,
					     SQLDESC_IND_PTR,
					     (Long)&(*inputBuf)[nullIndOffset],0);
	      if (retcode != SUCCESS)
		return retcode;
	    }
	  
	  retcode = SQL_EXEC_SetDescItem(input_desc, entry,
					 SQLDESC_VAR_PTR,
					 (Long)&(*inputBuf)[dataOffset],0);
	  if (retcode != SUCCESS)
	    return retcode;
	}
    }

  return 0;
}

Lng32 ExeCliInterface::setupExplainData(SQLMODULE_ID * module,
                                        SQLSTMT_ID * stmt)
{
  Lng32 retcode = 0;

  if (explainData_)
    NADELETEBASIC(explainData_, getHeap());

  explainData_ = NULL;
  explainDataLen_ = 0;

  // get explain fragment.
  explainDataLen_ = 50000; // start with 50K bytes
  Int32 retExplainLen = 0;
  explainData_ = new(getHeap()) char[explainDataLen_+1];
  retcode = SQL_EXEC_GetExplainData(stmt,
                                    explainData_, explainDataLen_+1, 
                                    &retExplainLen
                                    );
  if (retcode == -CLI_GENCODE_BUFFER_TOO_SMALL)
    {
      NADELETEBASIC(explainData_, getHeap());

      explainDataLen_ = retExplainLen;
      explainData_ = new(getHeap()) char[explainDataLen_ + 1];

      retcode = SQL_EXEC_GetExplainData(stmt,
                                        explainData_, explainDataLen_+1, 
                                        &retExplainLen
                                        );
    }
  
  if (retcode != SUCCESS)
    {
      NADELETEBASIC(explainData_, getHeap());
      explainData_ = NULL;
      explainDataLen_ = 0;

      return retcode;
    }

  explainDataLen_ = retExplainLen;
  
  return retcode;
}

Lng32 ExeCliInterface::setupExplainData()
{
  return setupExplainData(module_, stmt_);
}


Lng32 ExeCliInterface::exec(char * inputBuf, Lng32 inputBufLen)
{
  Lng32 retcode = 0;

  if (! stmt_)
    {
      return -CLI_STMT_NOT_EXISTS;
    }

  // set parserflags to indicate that this is an internal query from exeutil
  NABoolean flagWasSetHere = FALSE;
  if (currContext_)
    {
      if (((currContext_->getSqlParserFlags() & 0x20000) == 0) &&
	  (NOT notExeUtilInternalQuery())) // this is an exeutil internal query. This is the default.
	{
	  flagWasSetHere = TRUE;
	  currContext_->setSqlParserFlags(0x20000); // INTERNAL_QUERY_FROM EXEUTIL
	}
    }

  if ((inputBuf) && (inputBuf_) && (inputDatalen_ > 0) && (inputBufLen > 0))
    memcpy(inputBuf_, inputBuf, MINOF(inputDatalen_, inputBufLen));

  retcode = SQL_EXEC_Exec(stmt_, input_desc_, 0);

  // reset internal exeutil query parserflags if it was set in this method.
   if ((currContext_) && (flagWasSetHere))
     currContext_->resetSqlParserFlags(0x20000); // INTERNAL_QUERY_FROM EXEUTIL

  if (retcode != SUCCESS)
    return retcode;
 
  return retcode;
}

Lng32 ExeCliInterface::fetch()
{
  Lng32 retcode = 0;

  retcode = SQL_EXEC_Fetch(stmt_, output_desc_, 0);
  if (retcode != SUCCESS)
    return retcode;
 
  return retcode;
}

//if indicator pointer is NULL, do not retrieve the indicator
//the datatype SQLTYPECODE... is added (the others did not work)
Lng32 ExeCliInterface::getPtrAndLen(short entry, char* &ptr, Lng32 &len, short**ind)
{
  Long data_addr;
  Lng32 datatype = 0;
  Lng32 retcode;
  Long ind_addr;

  retcode = SQL_EXEC_GetDescItem(output_desc_, entry,
				 SQLDESC_VAR_PTR,
				 &data_addr, 0, 0, 0, 0);
  if (retcode != SUCCESS)
    return retcode;
  
  retcode = SQL_EXEC_GetDescItem(output_desc_, entry,
		  		 SQLDESC_IND_PTR ,
			  	 &ind_addr, 0, 0, 0, 0);
  if (retcode != SUCCESS)
    return retcode;
  
  if (ind != NULL)
    *ind = (short*)ind_addr;
  
  retcode = SQL_EXEC_GetDescItem(output_desc_, entry,
				 SQLDESC_TYPE,
				 &datatype, 0, 0, 0, 0);
  if (retcode != SUCCESS)
    return retcode;
  
  retcode = SQL_EXEC_GetDescItem(output_desc_, entry,
				 SQLDESC_OCTET_LENGTH,
				 &len, 0, 0, 0, 0);
  if (retcode != SUCCESS)
    return retcode;

  short *nullIndPtr = (short *)ind_addr;
  if (nullIndPtr && *nullIndPtr < 0)  
    {
      // without this test, when "showddl component sql_operations"
      // ran some child query which returned a nullable column
      // with a SQL NULL value, junk from data_addr was interpreted
      // as a length and the caller of this method tried to allocate
      // too many bytes.
      len = 0;
    }
  else 
  if (datatype == REC_BYTE_V_ASCII ||
      datatype == REC_BYTE_V_ASCII_LONG ||
      datatype == REC_BYTE_V_DOUBLE ||
      datatype == SQLTYPECODE_VARCHAR_WITH_LENGTH ||
      datatype == SQLTYPECODE_VARCHAR_LONG ||
      datatype == SQLTYPECODE_BLOB ||
      datatype == SQLTYPECODE_CLOB
      )
    {

      // Depending on value of len, first 2 or 4 bytes of data indicate
      // the actual length -- adjust data and length
      if (len & 0xFFFF8000)
	{
	  Lng32 VClen;
	  str_cpy_all((char*)&VClen, (char*)data_addr, sizeof(Lng32));
	  data_addr += sizeof(Lng32);
	  len = VClen;
	}
      else
	{
	  short VClen;
	  str_cpy_all((char*)&VClen, (char*)data_addr, sizeof(short));
	  data_addr += sizeof(short);
	  len = VClen;
	}
    }

  ptr = (char*)data_addr;

  return 0;
}

Lng32 ExeCliInterface::getHeadingAndLen(short entry, char* heading, Lng32 &len)
{
  Lng32 retcode;

  len = 0;
  retcode = SQL_EXEC_GetDescItem(output_desc_, entry,
				 SQLDESC_HEADING,
				 0, heading,
				 ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES, &len, 0);
  if (retcode != SUCCESS)
    return retcode;
  
  if (len == 0)
    {
      retcode = SQL_EXEC_GetDescItem(output_desc_, entry,
				     SQLDESC_NAME,
				     0, heading,
				     ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES/*CAT_MAX_HEADING_LEN*/, &len, 0);
      if (retcode != SUCCESS)
	return retcode;
    }

  return 0;
}

Lng32 ExeCliInterface::getNumEntries(Lng32 &numInput, Lng32 &numOutput)
{
  Lng32 retcode = 0;

  retcode = SQL_EXEC_GetDescEntryCount(input_desc_, &numInput);
  if (retcode != SUCCESS)
    return retcode;

  retcode = SQL_EXEC_GetDescEntryCount(output_desc_, &numOutput);
  if (retcode != SUCCESS)
    return retcode;

  return retcode;
}


Lng32 ExeCliInterface::getAttributes(short entry, NABoolean forInput,
				    Lng32 &fsDatatype, Lng32 &length,
				    Lng32 *indOffset, Lng32 *varOffset)
{
  Lng32 retcode = 0;

  if (forInput)
    {
      fsDatatype = inputAttrs_[entry-1].fsDatatype_;
      length = inputAttrs_[entry-1].length_;
      if (indOffset)
	*indOffset = inputAttrs_[entry-1].indOffset_;
      if (varOffset)
	*varOffset = inputAttrs_[entry-1].varOffset_;
    }
  else
    {
      fsDatatype = outputAttrs_[entry-1].fsDatatype_;
      length = outputAttrs_[entry-1].length_;
      if (indOffset)
	*indOffset = outputAttrs_[entry-1].indOffset_;
      if (varOffset)
	*varOffset = outputAttrs_[entry-1].varOffset_;
    }

  return retcode;
}

Lng32 ExeCliInterface::getDataOffsets(short entry, Lng32 forInput,
				    Lng32 *indOffset, Lng32 *varOffset)
{
  Lng32 temp;
  if (forInput)
    {
      if (inputAttrs_ && inputAttrs_[entry-1].varOffset_ && inputAttrs_[entry-1].indOffset_)
	{
	  *varOffset = inputAttrs_[entry-1].varOffset_;
	  *indOffset = inputAttrs_[entry-1].indOffset_;
	}
      else
	{
	  return getAttributes(entry, forInput, temp, temp,
			       indOffset, varOffset);
	}
    }
  else
    {
      return getAttributes(entry, forInput, temp, temp,
			   indOffset, varOffset);
    }
  return 0;
}

Lng32 ExeCliInterface::getStmtAttr(char * stmtName, Lng32 attrName, 
				   Lng32 * numeric_value, char * string_value)
{
  Lng32 retcode = 0;
  
  SQLMODULE_ID module;
  init_SQLMODULE_ID(&module);
  
  char stmtNameBuf[400];
  SQLSTMT_ID stmt;
  init_SQLSTMT_ID(&stmt, SQLCLI_CURRENT_VERSION, stmt_name, &module);
  stmt.identifier = stmtNameBuf;
  strcpy((char*)stmt.identifier, stmtName);
  stmt.identifier_len = (Lng32)strlen(stmtName);
  
  retcode = SQL_EXEC_GetStmtAttr(&stmt, attrName, numeric_value, string_value,
				  0, NULL);
  
  return retcode;
}

Lng32 ExeCliInterface::close()
{
  Lng32 retcode = 0;

  if (stmt_)
    {
      retcode = SQL_EXEC_CloseStmt(stmt_);
      if (retcode != SUCCESS)
	return retcode;
    }

  return retcode;
}



Lng32 ExeCliInterface::executeImmediatePrepare(const char * stmtStr,
					       char * outputBuf,
					       Lng32 * outputBufLen,
					       Int64 * rowsAffected,
					       NABoolean monitorThis,
					       char * stmtName
					      )
{
  Lng32 retcode = 0;
  if (outputBufLen)
    *outputBufLen = 0;

  retcode = allocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_,
		       stmtName);
  if (retcode != SUCCESS)
    return retcode;

  retcode = prepare(stmtStr, module_, stmt_, sql_src_, 
		    input_desc_, output_desc_, &outputBuf,NULL,&inputBuf_,
		    NULL,NULL,NULL,NULL,NULL,monitorThis);
  if (retcode < 0)
    {
      deallocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_);
      return retcode;
    }
  
  return 0;
}

Lng32 ExeCliInterface::executeImmediatePrepare2(const char * stmtStr,
					     char *uniqueStmtId,
                                             Lng32 *uniqueStmtIdLen,
                                             SQL_QUERY_COST_INFO *query_cost_info,
                                             SQL_QUERY_COMPILER_STATS_INFO *comp_stats_info,
                                             char * outputBuf,
					     Lng32 * outputBufLen,
					     Int64 * rowsAffected,
					     NABoolean monitorThis
					     )
{
  Lng32 retcode = 0;

  if (outputBufLen)
    *outputBufLen = 0;

  retcode = allocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_);
  if (retcode != SUCCESS)
    return retcode;

  retcode = prepare(stmtStr, module_, stmt_, sql_src_, 
		    input_desc_, output_desc_, &outputBuf, NULL, NULL, NULL,
                    uniqueStmtId, uniqueStmtIdLen, query_cost_info, comp_stats_info,monitorThis);
  if (retcode < 0)
    {
      deallocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_);
      return retcode;
    }
  
  return 0;
}

Lng32 ExeCliInterface::executeImmediateExec(const char * stmtStr,
					   char * outputBuf,
					   Lng32 * outputBufLen,
					   NABoolean nullTerminate,
					   Int64 * rowsAffected,
                                           ComDiagsArea **diagsArea)
{
  Lng32 retcode = 0;
  Lng32 diagsCount = 0;

  retcode = exec();
  if (retcode < 0)
    {
      deallocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_);
      goto ExecReturn;
    }

  retcode = fetch();
  if (retcode < 0)
    {
      deallocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_);
      goto ExecReturn;
    }

  if ((outputBuf) &&
      (outputBufLen))
    {
      *outputBufLen = 0;
      if (retcode != 100)
	{
	  char * ptr;
	  Lng32 len;
	  getPtrAndLen(1, ptr, len);         
          str_cpy_all(outputBuf, ptr, len);
	  
	  if (nullTerminate)
	    outputBuf[len] = 0;
	  *outputBufLen = len;
	}
    }

  Lng32 rc;
  if (retcode >= 0)
  {
    if (rowsAffected)
    {
      Int64 tmpRowsAffected = 0;
      rc = SQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_ROW_COUNT,
        &tmpRowsAffected, NULL,
        0, NULL);

      if (rc == EXE_NUMERIC_OVERFLOW) 
      {
        GetRowsAffected(rowsAffected);
      }
      else
        *rowsAffected = tmpRowsAffected;
    }
    rc = SQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_NUMBER,
                       &diagsCount, NULL, 0, NULL);
    // No need to passback the warnings of SQL_NO_DATA (100)
    if (retcode == 100)
       diagsCount--; 
  }
ExecReturn:
  if (retcode < 0 ||  diagsCount > 0) 
  {   
     if (diagsArea != NULL) 
     {
         if (*diagsArea == NULL)
            *diagsArea = ComDiagsArea::allocate(heap_); 
         rc = SQL_EXEC_MergeDiagnostics_Internal(**diagsArea);
     }
     // if diagsArea is not passed in, retain the diagnostics info
     // in ContextCli for it to be retrieved later 
     // But, deallocate the statement
     else
     {
        rc = close();
        deallocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_);
        return retcode;
     }
  }
  rc = close();
  deallocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_);
  clearGlobalDiags();
  return retcode;
}
// When the globalDiags is not null, errors and warnings are
// passed to the caller. If *globalsDiags points to NULL,
// allocate ComDiagsArea and pass back the error or warnings
// to the caller.
// 
// If *globalDiags points to ComDiagsArea already, pass back
// the diagnostics conditions back to the caller after executing
// the stmtStr along with new errors or warnings.
Lng32 ExeCliInterface::executeImmediate(const char * stmtStr,
				       char * outputBuf,
				       Lng32 * outputBufLen,
				       NABoolean nullTerminate,
				       Int64 * rowsAffected,
				       NABoolean monitorThis,
				       ComDiagsArea **globalDiags)
{
  Lng32 retcode = 0;

  ComDiagsArea * tempDiags = NULL;
  if (globalDiags != NULL && *globalDiags != NULL && (*globalDiags)->getNumber() > 0)
  {
     tempDiags = ComDiagsArea::allocate(heap_);
     tempDiags->mergeAfter(**globalDiags);
  }

  clearGlobalDiags();
  outputBuf_ = NULL;
  inputBuf_ = NULL;
  retcode = executeImmediatePrepare(stmtStr, 
				    (nullTerminate ? outputBuf_ : outputBuf),
				    outputBufLen,
				    rowsAffected,monitorThis);
  if (retcode < 0) 
     goto ExecuteImmediateReturn;
  retcode = executeImmediateExec(stmtStr, 
				 outputBuf, outputBufLen,
				 nullTerminate,
				 rowsAffected,
                                 globalDiags);
ExecuteImmediateReturn:
  if ((globalDiags))
    {
      // Allocate the diagnostics area if needed
      // and populate the diagnostics conditions
      if (*globalDiags == NULL && retcode != 0 && retcode != 100) {
         *globalDiags = ComDiagsArea::allocate(getHeap());
         SQL_EXEC_MergeDiagnostics_Internal(**globalDiags);
      }
      //  populate the diagnostics conditons passed in
      if (tempDiags)
         (*globalDiags)->mergeAfter(*tempDiags);
    }

  if (tempDiags)
      tempDiags->deAllocate();
  return retcode;
}

short ExeCliInterface::fetchRowsPrologue(const char * sqlStrBuf, 
					 NABoolean noExec,
					 NABoolean monitorThis,
					 char * stmtName)
{
  Lng32 retcode;

  clearGlobalDiags();

  retcode = allocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_,
		       stmtName);
  if (retcode < 0)
    {
      //ex_assert(0, "Error in fetchRows");
      return (short)retcode;
    }

  outputBuf_ = NULL;
  inputBuf_ = NULL;
  retcode = prepare(sqlStrBuf, module_, stmt_, sql_src_,
		    input_desc_, output_desc_, &outputBuf_, NULL,
		    &inputBuf_, NULL,NULL,NULL,NULL,NULL,monitorThis);
  if (retcode < 0)
    {
      //ex_assert(0, "Error in fetchRows");
      return (short)retcode;
    }

  if (NOT noExec)
    {
      retcode = exec();
      if (retcode < 0)
	{
	  //ex_assert(0, "Error in fetchRows");
	  return (short)retcode;
	}
    }

  return 0;
}

short ExeCliInterface::fetchRowsEpilogue(const char * sqlStrBuf,
					 NABoolean noClose)
{
  if (NOT noClose)
    close();

  deallocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_);

  if (NOT noClose)
    SQL_EXEC_ClearDiagnostics(NULL);

  return 0;
}

short ExeCliInterface::initializeInfoList(Queue* &infoList,
					  NABoolean infoListIsOutputInfo)
{
  if (infoList)
    {
      infoList->position();
      while (NOT infoList->atEnd())
	{
	  if (infoListIsOutputInfo)
	    {
	      OutputInfo * r = (OutputInfo*)infoList->getCurr();
	      r->dealloc(getHeap());
	    }
	  else
	    {
	      char * c = (char*)infoList->getCurr();
	      NADELETEBASIC(c, getHeap());
	    }

	  infoList->advance();
	}
      NADELETE(infoList, Queue, getHeap());
    }
  
  infoList = new(getHeap()) Queue(getHeap());

  return 0;
}

short ExeCliInterface::fetchAllRows(Queue * &infoList, 
				    const char * query,
				    Lng32 inNumOutputEntries,
				    NABoolean varcharFormat,
				    NABoolean monitorThis,
				    NABoolean initInfoList)
{
  short rc = 0;

  Lng32 numOutputEntries = inNumOutputEntries;

  if (initInfoList)
    {
      rc = initializeInfoList(infoList, TRUE);
      if (rc < 0)
	{
	  return rc;
	}
    }

  rc = fetchRowsPrologue(query,FALSE,monitorThis);
  if (rc < 0)
    {
      return rc;
    }

  NABoolean rowsFound = FALSE;
  while ((rc >= 0) && 
	 (rc != 100))
    {
      rc = (short)fetch();
      if (rc < 0)
	{
	  fetchRowsEpilogue(0, TRUE);

	  return rc;
	}

      if (rc == 100)
	continue;

      rowsFound = TRUE;
      if (numOutputEntries == -1)
      {
	char * ob = new(getHeap()) char[outputDatalen()];
	str_cpy_all(ob, outputBuf(), 
		    outputDatalen());
	
	infoList->insert(ob);
      }
      else
      {
	if (numOutputEntries == 0)
	  numOutputEntries = numOutputEntries_;

	OutputInfo * oi = new(getHeap()) OutputInfo(numOutputEntries);
	
	for (Int32 j = 0; j < numOutputEntries; j++)
	{
	  char * ptr, *r;
	  Lng32   len;
          Lng32   type;
          short nulind=0;
          short *indaddr=&nulind;
          short **ind=&indaddr;
          getAttributes(j+1, FALSE, type, len, NULL, NULL);
	  getPtrAndLen(j+1, ptr, len, ind);
          NABoolean  nullableCol = TRUE, isNullVal = FALSE;
          if(*ind == NULL) // It is not a nullable value
            isNullVal = FALSE; 
          else
          {
            if( ((char*)*ind)[0] == -1 ) // NULL value
              isNullVal = TRUE;
          }
  
          if(isNullVal) 
          {
            oi->insert(j, NULL, 0, type);
          }
          else
          {
	    NABoolean nullTerminate = 
	    DFS2REC::is8bitCharacter(outputAttrs_[j].fsDatatype_);

	    r = new(getHeap()) char
	      [(varcharFormat ? SQL_VARCHAR_HDR_SIZE : 0) 
	       + len + (nullTerminate ? 1 : 0)];
	    if (varcharFormat)
	    {
	      *(short*)r = (short)len;
	      str_cpy_all(&r[SQL_VARCHAR_HDR_SIZE], ptr, len);

	      if (nullTerminate)
	        r[SQL_VARCHAR_HDR_SIZE + len] = 0;
	    }
	    else
	    {
	      str_cpy_all(r, ptr, len);
	      if (nullTerminate)
	        r[len] = 0;
	    }
	    oi->insert(j, r, len, type);
           }
	}
	
	infoList->insert(oi);
      }
    } // while

  rc = fetchRowsEpilogue(0);
  if (rc < 0)
    {
      return rc;
    }
  
  if (NOT rowsFound)
    rc = 100;

  return rc;
}

short ExeCliInterface::clearExecFetchClose(char * inputBuf,
					   Lng32 inputBufLen,

					   // ptr to buf where output values will be copied to. 
					   // Caller need to allocate this.
					   char * outputBuf, 
					   Lng32 * outputBufLen)
{
  Lng32 retcode = 0;

  SQL_EXEC_ClearDiagnostics(NULL);

  retcode = exec(inputBuf, inputBufLen);
  if (retcode < 0)
    {
      return (short)retcode;
    }

  retcode = fetch();
  if (retcode < 0)
    {
      return (short)retcode;
    }

  if ((outputBuf) &&
      (outputBufLen))
    {
      *outputBufLen = 0;
      if (retcode != 100)
	{
	  char * currPtr = outputBuf;
	  for (Int32 j = 0; j < numOutputEntries_; j++)
	    {
	      char * ptr;
	      Lng32   len;
	      getPtrAndLen(j+1, ptr, len);
	      
	      str_cpy_all(currPtr, ptr, len);
	      currPtr += len;

	      *outputBufLen += len;
	    }
	}
    }
  close(); 
  if (retcode == 100)
     fetchRowsEpilogue(NULL, TRUE); 
  return (short)retcode;
}

short ExeCliInterface::clearExecFetchCloseOpt(char * inputBuf,
					      Lng32 inputBufLen,
					      
					      // ptr to buf where output values will be copied to. 
					      // Caller need to allocate this.
					      char * outputBuf, 
					      Lng32 * outputBufLen,
                                              Int64 * rowsAffected)
{
  Lng32 retcode = 0;

  if (inputBuf)
    {
      if (inputDatalen_ < inputBufLen)
        {
          // input data will not fit in inputBuf_.
          // return error.
          return -EXE_STRING_OVERFLOW;
        }

      str_cpy_all(inputBuf_, inputBuf, inputBufLen);
    }

  if (rowsAffected)
    *rowsAffected = 0;

  retcode = SQL_EXEC_ClearExecFetchClose(
					 stmt_, 
					 (numInputEntries_ > 0 ? input_desc_ : NULL),
					 (numOutputEntries_ > 0 ? output_desc_ : NULL),
					 0, 0, 0);
  
  if ((retcode == 0) ||
      ((retcode >= 0) && (retcode != 100)))
    {
      if ((numOutputEntries_ > 0) &&
          (outputBuf) &&
          (outputBufLen))
        {
          *outputBufLen = 0;
          
          char * currPtr = outputBuf;
          for (Int32 j = 0; j < numOutputEntries_; j++)
            {
              char * ptr;
              Lng32   len;
              getPtrAndLen(j+1, ptr, len);
              
              str_cpy_all(currPtr, ptr, len);
              currPtr += len;
              
              *outputBufLen += len;
            }
          
          if (rowsAffected)
            {
              *rowsAffected = 1;
            }
        } // values being returned
      else if (rowsAffected)
        {
          Lng32 tmpRowsAffected = 0;
          retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_ROW_COUNT,
                                                     &tmpRowsAffected, NULL,
                                                     0, NULL);
          
          if(retcode == EXE_NUMERIC_OVERFLOW) // rowsAffected > LONG_MAX
            {
              GetRowsAffected(rowsAffected);
            }
          else
            *rowsAffected = (Int64)tmpRowsAffected;
        } // rowsAffected
    }

  return (short)retcode;
}

Lng32 ExeCliInterface::executeImmediateCEFC(const char * stmtStr,
                                            char * inputBuf,
                                            Lng32 inputBufLen,
                                            char * outputBuf,
                                            Lng32 * outputBufLen,
                                            Int64 * rowsAffected
                                            )
{
  Lng32 retcode = 0;


  clearGlobalDiags();

  outputBuf_ = NULL;
  inputBuf_ = NULL;
  retcode = executeImmediatePrepare(stmtStr, 
				    outputBuf,
				    outputBufLen,
				    rowsAffected);
  if (retcode < 0)
    goto ExecuteImmediateCEFCReturn;
  
  retcode = clearExecFetchCloseOpt(inputBuf, inputBufLen, NULL, NULL, rowsAffected);

ExecuteImmediateCEFCReturn:
  return retcode;
}

Lng32 ExeCliInterface::cwrsAllocStuff(SQLMODULE_ID * &module,
					SQLSTMT_ID * &stmt,
					SQLDESC_ID * &sql_src,
					SQLDESC_ID * &input_desc,
					SQLDESC_ID * &output_desc,
					SQLDESC_ID * &rs_input_maxsize_desc,
					const char * stmtName
  )
{
  Lng32 retcode = 0;

  retcode = cwrsDeallocStuff(module,
			       stmt,
			       sql_src,
			       input_desc,
			       output_desc,
			       rs_input_maxsize_desc);
  if (retcode < 0)
    {
      return (short)retcode;
    }

  retcode = allocStuff(module,
		       stmt,
		       sql_src,
		       input_desc,
		       output_desc);
  if (retcode < 0)
    {
      return (short)retcode;
    }

  // Allocate a descriptor to set input array maximum size
  rs_input_maxsize_desc = new(heap_) SQLSTMT_ID;
  init_SQLDESC_ID(rs_input_maxsize_desc, SQLCLI_CURRENT_VERSION, desc_handle, module);

  retcode = SQL_EXEC_AllocDesc(rs_input_maxsize_desc, 1);
  if (retcode < 0)
    return retcode;

  retcode = SQL_EXEC_SetDescItem(rs_input_maxsize_desc, 1, SQLDESC_TYPE,
				 SQLTYPECODE_INTEGER, NULL);
  if (retcode < 0)
    return retcode;

  retcode = SQL_EXEC_SetDescItem( rs_input_maxsize_desc,
				  1,  // entry #1, attr value.
				  SQLDESC_VAR_PTR, // what to set - host var addr
				  (long) &rsMaxsize_,  // Notice that output from CLI
				                // will be read from host var
                        			// rowset_size.
				  NULL);

  retcode = SQL_EXEC_SetStmtAttr(stmt,
                                 SQL_ATTR_INPUT_ARRAY_MAXSIZE,
                                 rsMaxsize_,
				 NULL);

  return retcode;
}
  
Lng32 ExeCliInterface::cwrsDeallocStuff(
  SQLMODULE_ID * &module,
  SQLSTMT_ID * &stmt,
  SQLDESC_ID * &sql_src,
  SQLDESC_ID * &input_desc,
  SQLDESC_ID * &output_desc,
  SQLDESC_ID * &rs_input_maxsize_desc)
{
  deallocStuff(module, stmt, sql_src, input_desc, output_desc);

  if (rs_input_maxsize_desc)
    {
      SQL_EXEC_DeallocDesc(rs_input_maxsize_desc);
      NADELETEBASIC(rs_input_maxsize_desc, heap_);
      rs_input_maxsize_desc = NULL;
    }

  return 0;
}
  
Lng32 ExeCliInterface::cwrsPrepare(const char * stmtStr,
				   Lng32 rs_maxsize, NABoolean monitorThis)
{
  Lng32 retcode = 0;

  rsMaxsize_ = rs_maxsize;
  retcode = cwrsAllocStuff(module_, stmt_, sql_src_, 
			     input_desc_, output_desc_,
                             rs_input_maxsize_desc_);
  if (retcode != SUCCESS)
    return retcode;

  retcode = prepare(stmtStr, module_, stmt_, sql_src_, 
		    input_desc_, output_desc_, NULL,NULL,NULL,NULL,
		    NULL,NULL,NULL,NULL,monitorThis);
  if (retcode < 0)
    {
      cwrsDeallocStuff(module_, stmt_, sql_src_, 
			 input_desc_, output_desc_,
			 rs_input_maxsize_desc_);
      return retcode;
    }
  
  numQuadFields_ = numInputEntries_;// + 1;
  quadFields_ = (SQLCLI_QUAD_FIELDS*)
    new (heap_) char[sizeof(SQLCLI_QUAD_FIELDS) * numQuadFields_];
  
  rsInputBuffer_ = new(heap_) char[rsMaxsize_ * inputDatalen_];

  Lng32 fsDatatype;
  Lng32 length;
  Lng32 indOffset = 0;
  Lng32 varOffset = 0;

  quadFields_[0].var_layout = 0;
  quadFields_[0].var_ptr = (void *)&currRSrow_;
  quadFields_[0].ind_layout = 0;
  quadFields_[0].ind_ptr = NULL; 

  Lng32 currOffset = 0;
  for (Lng32 entry = 2; entry <= numInputEntries_; entry++)
    {
      getAttributes(entry, TRUE,
		    fsDatatype, length,
		    &indOffset, &varOffset);
 
      quadFields_[entry-1].var_layout = length;
      quadFields_[entry-1].var_ptr = (void*)&rsInputBuffer_[currOffset];
      quadFields_[entry-1].ind_layout = 0;
      quadFields_[entry-1].ind_ptr = NULL;
      
      currOffset += length * rsMaxsize_;
    }

  currRSrow_ = 0;

  return 0;
}

Lng32 ExeCliInterface::cwrsExec(
  char * inputRow,
  Int32 inputRowLen,
  Int64 * rowsAffected)
{
  Lng32 retcode = 0;

  Int32    rowset_status[1];    // Has no functionality currently. 
                                //However it is part of RowsetSetDesc API

  if ((inputRow) && (currRSrow_ < rsMaxsize_))
    {
      Lng32 fsDatatype;
      Lng32 length;
      Lng32 indOffset = 0;
      Lng32 varOffset = 0;
      
      for (Lng32 entry = 2; entry <= numInputEntries_; entry++)
        {
	  getAttributes(entry, TRUE,
			fsDatatype, length,
			&indOffset, &varOffset);
	  
	  str_cpy_all((char*)((long)(quadFields_[entry-1].var_ptr) + 
			      currRSrow_*length),
		      &inputRow[varOffset], length);
	}

      currRSrow_++;
      return 0;
    }

  if ((inputRow == NULL) && (currRSrow_ == 0))
    return 0;

  retcode = SQL_EXEC_SETROWSETDESCPOINTERS(input_desc_,
                                           currRSrow_,
                                           rowset_status,
                                           1,
                                           numQuadFields_,
                                           quadFields_);
  if (retcode < 0)
    return retcode;

  retcode = exec();
  if (retcode < 0)
    return retcode;

  retcode = fetch();
  if (retcode < 0)
    return retcode;

  if (retcode >= 0)
  {
    if (rowsAffected)
    {
      Lng32 tmpRowsAffected = 0;
      retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_ROW_COUNT,
        &tmpRowsAffected, NULL,
        0, NULL);

      if(retcode == EXE_NUMERIC_OVERFLOW) // rowsAffected > LONG_MAX
      {
        GetRowsAffected(rowsAffected);
      }
      else
        *rowsAffected = (Int64)tmpRowsAffected;

      retcode = 0;
    }
  }

  retcode = close();

  currRSrow_ = 0;

  // current input row has not been moved to the rowset buffer.
  // move the input row to the rowset buffer
  return cwrsExec(inputRow, inputRowLen, rowsAffected);
}

Lng32 ExeCliInterface::cwrsClose(Int64 * rowsAffected)
{
  Lng32 retcode = 0;

  if (rowsAffected)
    *rowsAffected = 0;

  retcode = cwrsExec(NULL, 0, rowsAffected);

  Lng32 retcode2 = cwrsDeallocStuff(module_, stmt_, sql_src_, 
                                    input_desc_, output_desc_,
                                    rs_input_maxsize_desc_);
  return retcode;
}

Lng32 ExeCliInterface::rwrsPrepare(const char * stmtStr, Lng32 rs_maxsize,NABoolean monitorThis)
{
  Lng32 retcode = 0;

  rsMaxsize_ = rs_maxsize;
  retcode = allocStuff(module_, stmt_, sql_src_, 
		       input_desc_, output_desc_);
  if (retcode != SUCCESS)
    return retcode;

  /*
  retcode = SQL_EXEC_SetStmtAttr(stmt_,
                                 SQL_ATTR_INPUT_ARRAY_MAXSIZE,
                                 rsMaxsize_,
				 NULL);
  if (retcode != SUCCESS)
    return retcode;
  */

  retcode = prepare(stmtStr, module_, stmt_, sql_src_, 
		    input_desc_, output_desc_, NULL,NULL,NULL,NULL,NULL,
		    NULL,NULL,NULL,monitorThis);
  if (retcode < 0)
    {
      deallocStuff(module_, stmt_, sql_src_, 
		   input_desc_, output_desc_);
      return retcode;
    }
  
  rsInputBuffer_ = new(heap_) char[rsMaxsize_ * inputDatalen_];

  retcode = SQL_EXEC_SetDescItem(input_desc_,0,SQLDESC_ROWSET_TYPE, 3, 0);
  
  retcode = SQL_EXEC_SetDescItem(input_desc_,0,SQLDESC_ROWWISE_ROWSET_PTR, 
				 (long)rsInputBuffer_, 0);

  retcode = SQL_EXEC_SetDescItem(input_desc_,0,SQLDESC_ROWWISE_ROWSET_ROW_LEN, 
				 inputDatalen_, 0);

  currRSrow_ = 0;

  return 0;
}

Lng32 ExeCliInterface::rwrsExec(
  char * inputRow,
  Int32 inputRowLen,
  Int64 * rowsAffected)
{
  Lng32 retcode = 0;

  clearGlobalDiags();

  if (rowsAffected)
    *rowsAffected = 0;

  if ((inputRow) && (currRSrow_ < rsMaxsize_))
    {
      str_cpy_all(&rsInputBuffer_[currRSrow_ * inputDatalen_], 
		  inputRow, inputRowLen);
      currRSrow_++;
      return 0;
    }

  if ((inputRow == NULL) && (currRSrow_ == 0))
    return 0;

  retcode = SQL_EXEC_SetDescItem(input_desc_, 0, SQLDESC_ROWWISE_ROWSET_SIZE, 
				 currRSrow_, 0);

  retcode = exec();
  if (retcode < 0)
    return retcode;

  retcode = fetch();
  if (retcode < 0)
    return retcode;

  Lng32 cliRetcode = -1;
  if (retcode >= 0)
  {
    cliRetcode = retcode;
    if (rowsAffected)
    {
      Lng32 tmpRowsAffected = 0;
      retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_ROW_COUNT,
        &tmpRowsAffected, NULL,
        0, NULL);

      if(retcode == EXE_NUMERIC_OVERFLOW) // rowsAffected > LONG_MAX
      {
        GetRowsAffected(rowsAffected);
      }
      else
        *rowsAffected = (Int64)tmpRowsAffected;

      retcode = 0;
    }
  }

  currRSrow_ = 0;

  return retcode;
}

Lng32 ExeCliInterface::rwrsClose()
{
  Lng32 retcode = 0;

  retcode = rwrsExec(NULL, 0, NULL);

  close();

  deallocStuff(module_, stmt_, sql_src_, 
	       input_desc_, output_desc_);

  return retcode;
}

short ExeCliInterface::prepareAndExecRowsPrologue(const char * sqlInitialStrBuf,
                                                  char * sqlSecondaryStrBuf,
                                                  Queue* initialOutputVarPtrList,
                                                  Queue* secondaryOutputVarPtrList,
                                                  Int64   &rowsAffected,
						  NABoolean monitorThis)
{
  Lng32 retcode = 0;
  
  SQL_EXEC_ClearDiagnostics(NULL);
  retcode = allocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_);
  if (retcode < 0)
    return (short)retcode;

  // Prepare
  outputBuf_ = NULL;
  inputBuf_ = NULL;
  retcode = prepare(sqlInitialStrBuf, module_, stmt_, sql_src_,
		    input_desc_, output_desc_, &outputBuf_,
                    initialOutputVarPtrList, NULL,NULL,NULL,NULL,
		    NULL,NULL,monitorThis);
  if (retcode < 0)
    return (short)retcode;
  
  retcode = allocStuff(moduleWithCK_, stmtWithCK_, sql_src_withCK_,
                      input_desc_withCK_, output_desc_withCK_);
  if (retcode < 0)
    return (short)retcode;
  
  // Prepare
  outputBuf_withCK_ = NULL;
  retcode = prepare(sqlSecondaryStrBuf, moduleWithCK_, stmtWithCK_, sql_src_withCK_,
		    input_desc_withCK_, output_desc_withCK_, &outputBuf_withCK_,
		    secondaryOutputVarPtrList,NULL,NULL,NULL,NULL,
		    NULL,NULL,monitorThis);
  if (retcode < 0)
    return (short)retcode;
  
  // Ensure the input entries have the correct character set type
  retcode = setCharsetTypes();
  if (retcode < 0)
    return (short)retcode;
   
  retcode = SQL_EXEC_Exec(stmt_, input_desc_, 0);
  if (retcode < 0)
    return (short)retcode;
  
  Lng32 cliRetcode = -1;
  retcode = SQL_EXEC_Fetch(stmt_, output_desc_, 0);
  if (retcode < 0)
    return (short)retcode;
  
  if (retcode >=  0) // we continue for warnings (retcode > 0)
                     // and NO errors or warnings (retcode == 0)
  {
    cliRetcode = retcode; // save the retcode from cli fetch.
    setOutputPtrsAsInputPtrs(initialOutputVarPtrList, 0);

    Int64 tmpRowsAffected = 0;
    retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_ROW_COUNT,
      &tmpRowsAffected, NULL,
      0, NULL);

    if(retcode == EXE_NUMERIC_OVERFLOW) // rowsAffected > LONG_MAX
    {
      GetRowsAffected(&rowsAffected);
    }
    else
      rowsAffected = tmpRowsAffected;
  }
  
  SQL_EXEC_ClearDiagnostics(NULL);
  
  retcode = SQL_EXEC_CloseStmt(stmt_);
  if (retcode < 0)
    return (short)retcode;
  
  // return the return code from the cli fetch if it succeded.
  if (cliRetcode != -1) 
    return (short) cliRetcode;
  else
    return (short) retcode;
}

short ExeCliInterface::execContinuingRows(Queue * continuingOutputVarPtrList,
                                          Int64   &rowsAffected)
{
  Lng32 retcode = 0;

  SQL_EXEC_ClearDiagnostics(NULL);
 
  retcode = SQL_EXEC_Exec(stmtWithCK_, input_desc_withCK_, 0);
  if (retcode < 0)
      return (short)retcode;

  Lng32 cliRetcode = -1;
  retcode = SQL_EXEC_Fetch(stmtWithCK_, output_desc_withCK_, 0);
  if (retcode < 0)
      return (short)retcode;

  if (retcode >= 0)
  {
    cliRetcode = retcode; // save the retcode from cli fetch.
    setOutputPtrsAsInputPtrs(continuingOutputVarPtrList, 0);

    Int64 tmpRowsAffected = 0;
    retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(NULL, SQLDIAG_ROW_COUNT,
      &tmpRowsAffected, NULL,
      0, NULL);

    if(retcode == EXE_NUMERIC_OVERFLOW) // rowsAffected > LONG_MAX
    {
      GetRowsAffected(&rowsAffected);
    }
    else
      rowsAffected = tmpRowsAffected;
  }

  retcode = SQL_EXEC_CloseStmt(stmtWithCK_);
  if (retcode < 0)
      return (short)retcode;

  // return the return code from the cli fetch if it succeded.
  if (cliRetcode != -1) 
    return (short) cliRetcode;
  else
    return (short) retcode;
}

Lng32 ExeCliInterface::setCharsetTypes()
{
  Lng32 retcode = 0;
  Lng32 num_input_entries = 0;

  // Obtain the number of entries
  retcode = SQL_EXEC_GetDescEntryCount(input_desc_withCK_, &num_input_entries);

  if (retcode < 0)
    return (short)retcode;

  Lng32 data_charset = 0;

  for (Lng32 i = 1; i <= num_input_entries; i++)
  {
  
    retcode = SQL_EXEC_GetDescItem(output_desc_withCK_,
                                   i,
     			           SQLDESC_CHAR_SET,
				   &data_charset, 0, 0, 0, 0);
    if (retcode < 0)
      return (short)retcode;

    if (data_charset != SQLCHARSETCODE_UNKNOWN)  
      {
         retcode = SQL_EXEC_SetDescItem(input_desc_withCK_,
                                        i,
     			                SQLDESC_CHAR_SET,
				        data_charset, 0);
         if (retcode < 0)
	   return (short)retcode;
       }
   }

   return (short)retcode;
}

void ExeCliInterface::setOutputPtrsAsInputPtrs(Queue* outputVarPtrList,
                                                SQLDESC_ID * target_inputDesc)
{
    // Process the queue
    Int32 j = 1;
    outputVarPtrList->position();

    while (NOT outputVarPtrList->atEnd())
      {
        char * entry = (char*)outputVarPtrList->getCurr();
        SQL_EXEC_SetDescItem(input_desc_withCK_, j++,
                             SQLDESC_VAR_PTR, 
                             (Long)entry, 0);

        outputVarPtrList->advance();
      }

    return;
}

Lng32 ExeCliInterface::prepareAndExecRowsEpilogue()
{
  Lng32 retcode = 0;

  retcode = close();

  deallocStuff(module_, stmt_, sql_src_, input_desc_, output_desc_);

  if (stmtWithCK_)
    {
      retcode = SQL_EXEC_CloseStmt(stmtWithCK_);
    }

  deallocStuff(moduleWithCK_, stmtWithCK_,
               sql_src_withCK_, input_desc_withCK_, output_desc_withCK_);

  SQL_EXEC_ClearDiagnostics(NULL);

  return 0;
}

Lng32 ExeCliInterface::beginWork()
{
  return executeImmediate("begin work;");
}

Lng32 ExeCliInterface::commitWork()
{
  return executeImmediate("commit work;");
}

Lng32 ExeCliInterface::rollbackWork()
{
  return executeImmediate("rollback work;");
}

Lng32 ExeCliInterface::autoCommit(NABoolean v)
{
  if (v)
    return executeImmediate("set transaction autocommit ON;");
  else
    return executeImmediate("set transaction autocommit OFF;");
}

Lng32 ExeCliInterface::beginXn()
{
  return SQL_EXEC_Xact(SQLTRANS_BEGIN, 0);
}

Lng32 ExeCliInterface::commitXn()
{
  return SQL_EXEC_Xact(SQLTRANS_COMMIT, 0);
}

Lng32 ExeCliInterface::rollbackXn()
{
  return SQL_EXEC_Xact(SQLTRANS_ROLLBACK, 0);
}

Lng32 ExeCliInterface::statusXn()
{
  return SQL_EXEC_Xact(SQLTRANS_STATUS, NULL);
}

Lng32 ExeCliInterface::suspendXn()
{
  return SQL_EXEC_Xact(SQLTRANS_SUSPEND, 0);
}

Lng32 ExeCliInterface::resumeXn()
{
  return SQL_EXEC_Xact(SQLTRANS_RESUME, 0);
}

Lng32 ExeCliInterface::createContext(char * contextHandle)
{
  Lng32 rc = 0;
  rc = SQL_EXEC_CreateContext((SQLCTX_HANDLE*)contextHandle, NULL, FALSE);
  return rc;
}

Lng32 ExeCliInterface::switchContext(char * contextHandle)
{
  Lng32 rc = 0;
  rc = SQL_EXEC_SwitchContext(*(SQLCTX_HANDLE*)contextHandle, NULL);
  return rc;
}

Lng32 ExeCliInterface::currentContext(char * contextHandle)
{
  Lng32 rc = 0;
  rc = SQL_EXEC_CurrentContext((SQLCTX_HANDLE*)contextHandle);
  return rc;
}

Lng32 ExeCliInterface::deleteContext(char* contextHandle) // in buf contains context handle
{
  return SQL_EXEC_DeleteContext(*(SQLCTX_HANDLE*)contextHandle);
}

Lng32 ExeCliInterface::retrieveSQLDiagnostics(ComDiagsArea *toDiags)
{
  Lng32 retcode;
  ex_assert(toDiags != NULL, "ComDiagsArea is null");
  retcode = SQL_EXEC_MergeDiagnostics_Internal(*toDiags);
  SQL_EXEC_ClearDiagnostics(NULL);
  return retcode;
}


ComDiagsArea *ExeCliInterface::allocAndRetrieveSQLDiagnostics(ComDiagsArea *&toDiags)
{
  Lng32 retcode;
  NABoolean daAllocated = FALSE; 
  if (toDiags == NULL) { 
      toDiags = ComDiagsArea::allocate(heap_);
      daAllocated = TRUE;
  }

  retcode = SQL_EXEC_MergeDiagnostics_Internal(*toDiags);
  SQL_EXEC_ClearDiagnostics(NULL);
 
  if (retcode == 0)
     return toDiags;
  else {
     if (daAllocated)
        toDiags->decrRefCount();
     return NULL;
  }
}

Lng32 ExeCliInterface::GetRowsAffected(Int64 *rowsAffected)
{
	SQLDESC_ID *rowCountDesc = NULL;
	SQLMODULE_ID * module = NULL;

	Lng32 retcode = SUCCESS;
	NABoolean sqlWarning = FALSE;

	SQLDIAG_STMT_INFO_ITEM_ID sqlItem = SQLDIAG_ROW_COUNT;
	rowCountDesc = new(heap_) SQLDESC_ID;
	rowCountDesc->version = SQLCLI_CURRENT_VERSION;
	rowCountDesc->name_mode = desc_handle;
	module = new(heap_) SQLMODULE_ID;
	rowCountDesc->module = module;
	module->module_name = 0;
	module->charset = "ISO88591";
	module->module_name_len = 0;
	module->version = SQLCLI_CURRENT_VERSION;
	rowCountDesc->identifier = 0;
	rowCountDesc->handle = 0;
  
	// get number of rows affected
  
	retcode = SQL_EXEC_AllocDesc (rowCountDesc, 1);
 	retcode = SQL_EXEC_SetDescItem(rowCountDesc, 1,
					 SQLDESC_TYPE_FS, REC_BIN64_SIGNED, 0);
	retcode = SQL_EXEC_SetDescItem(rowCountDesc, 1,
					 SQLDESC_VAR_PTR, (Long)rowsAffected, 0);

	Lng32 *temp;
	temp = (Lng32 *) &sqlItem;
  
	retcode = SQL_EXEC_GetDiagnosticsStmtInfo(temp, rowCountDesc);

  // free up resources
	SQL_EXEC_DeallocDesc(rowCountDesc);

	return SUCCESS;

} // GetRowsAffected()

Lng32 ExeCliInterface::holdAndSetCQD(const char * defaultName, const char * defaultValue, ComDiagsArea *globalDiags)
{
  Lng32 cliRC;
  
  char buf[400];
  
  strcpy(buf, "control query default ");
  strcat(buf, defaultName);
  strcat(buf, " hold;");

  // hold the current value for defaultName
  cliRC = executeImmediate(buf, NULL, NULL, TRUE, NULL, 0,&globalDiags);
  if (cliRC < 0)
    {
      return cliRC;
    }

  // now set the new value for defaultName
  strcpy(buf, "control query default ");
  strcat(buf, defaultName);
  strcat(buf, " '");
  strcat(buf, defaultValue);
  strcat(buf, "';");
  cliRC = executeImmediate(buf, NULL, NULL, TRUE, NULL, 0,&globalDiags);
  if (cliRC < 0)
    {
      return cliRC;
    }

  return 0;
}


Lng32 ExeCliInterface::restoreCQD(const char * defaultName, ComDiagsArea *globalDiags)
{
  Lng32 cliRC;

  char buf[400];
  
  // remove this cqd from the cqd array
  strcpy(buf, "control query default ");
  strcat(buf, defaultName);
  strcat(buf, " reset;");
  cliRC = executeImmediate(buf, NULL, NULL, TRUE, NULL, 0,&globalDiags);
  if (cliRC < 0)
    {
      return cliRC;
    }

  // restore the saved cqd default value
  strcpy(buf, "control query default ");
  strcat(buf, defaultName);
  strcat(buf, " restore;");
  cliRC = executeImmediate(buf, NULL, NULL, TRUE, NULL, 0,&globalDiags);
  if (cliRC < 0)
    {
      return cliRC;
    }

  return 0;
}

Lng32 ExeCliInterface::getCQDval(const char * defaultName, 
				 char * val,
				 ComDiagsArea *globalDiags)

{
  Lng32 cliRC;

  char buf[400];
  
  strcpy(buf, "cqd showcontrol_show_all 'ON';");
  cliRC = executeImmediate(buf);
  if (cliRC < 0)
    {
      return cliRC;
    }

  char valBuf[4000];
  Lng32 valBufLen = 0;
  strcpy(buf, "showcontrol default ");
  strcat(buf, defaultName);
  strcat(buf, ", match full, no header;");

  cliRC = executeImmediate(buf, valBuf, &valBufLen);
  if (cliRC < 0)
    {
      strcpy(buf, "cqd showcontrol_show_all 'OFF';");
      executeImmediate(buf);

      return cliRC;
    }

  strcpy(val, valBuf);

  strcpy(buf, "cqd showcontrol_show_all 'OFF';");
  cliRC = executeImmediate(buf);

  return 0;
}

Lng32 ExeCliInterface::setCQS(const char * shape, ComDiagsArea *globalDiags)
{
  Lng32 cliRC;

  cliRC = executeImmediate(shape, NULL, NULL, TRUE, NULL, 0,&globalDiags);
  if (cliRC < 0)
    {
      return cliRC;
    }

  return 0;
}
  

Lng32 ExeCliInterface::resetCQS(ComDiagsArea * globalDiags)
{
  Lng32 cliRC;

  char buf[400];
  
  strcpy(buf, "control query shape cut ");
  cliRC = executeImmediate(buf, NULL, NULL, TRUE, NULL, 0,&globalDiags);
  if (cliRC < 0)
    {
      return cliRC;
    }

  return 0;
}
  
  
// methods for routine invocation

Lng32 ExeCliInterface::getRoutine(
     /* IN */     const char   *serializedInvocationInfo,
     /* IN */     Int32         invocationInfoLen,
     /* IN */     const char   *serializedPlanInfo,
     /* IN */     Int32         planInfoLen,
     /* IN */     Int32         language,
     /* IN */     Int32         paramStyle,
     /* IN */     const char   *externalName,
     /* IN */     const char   *containerName,
     /* IN */     const char   *externalPath,
     /* IN */     const char   *librarySqlName,
     /* OUT */    Int32        *handle,
     /* IN/OUT */ ComDiagsArea *diags)
{
  Lng32 retcode = 0;

  SQL_EXEC_ClearDiagnostics(NULL);

  retcode = SQL_EXEC_GetRoutine(
       serializedInvocationInfo,
       invocationInfoLen,
       serializedPlanInfo,
       planInfoLen,
       language,
       paramStyle,
       externalName,
       containerName,
       externalPath,
       librarySqlName,
       handle);

  if (retcode != 0 && diags)
    {
      SQL_EXEC_MergeDiagnostics_Internal(*diags);
      SQL_EXEC_ClearDiagnostics(NULL);
    }

  return retcode;
}

Lng32 ExeCliInterface::invokeRoutine(
     /* IN */     Int32         handle,
     /* IN */     Int32         phaseEnumAsInt,
     /* IN */     const char   *serializedInvocationInfo,
     /* IN */     Int32         invocationInfoLen,
     /* OUT */    Int32        *invocationInfoLenOut,
     /* IN */     const char   *serializedPlanInfo,
     /* IN */     Int32         planInfoLen,
     /* IN */     Int32         planNum,
     /* OUT */    Int32        *planInfoLenOut,
     /* IN */     char         *inputRow,
     /* IN */     Int32         inputRowLen,
     /* OUT */    char         *outputRow,
     /* IN */     Int32         outputRowLen,
     /* IN/OUT */ ComDiagsArea *diags)
{
  Lng32 retcode = 0;

  SQL_EXEC_ClearDiagnostics(NULL);

  retcode = SQL_EXEC_InvokeRoutine(
       handle,
       phaseEnumAsInt,
       serializedInvocationInfo,
       invocationInfoLen,
       invocationInfoLenOut,
       serializedPlanInfo,
       planInfoLen,
       planNum,
       planInfoLenOut,
       inputRow,
       inputRowLen,
       outputRow,
       outputRowLen);

  if (retcode != 0 && diags)
    {
      SQL_EXEC_MergeDiagnostics_Internal(*diags);
      SQL_EXEC_ClearDiagnostics(NULL);
    }

  return retcode;
}

Lng32 ExeCliInterface::getRoutineInvocationInfo(
     /* IN */     Int32         handle,
     /* IN/OUT */ char         *serializedInvocationInfo,
     /* IN */     Int32         invocationInfoMaxLen,
     /* OUT */    Int32        *invocationInfoLenOut,
     /* IN/OUT */ char         *serializedPlanInfo,
     /* IN */     Int32         planInfoMaxLen,
     /* IN */     Int32         planNum,
     /* OUT */    Int32        *planInfoLenOut,
     /* IN/OUT */ ComDiagsArea *diags)
{
  Lng32 retcode = 0;

  SQL_EXEC_ClearDiagnostics(NULL);

  retcode = SQL_EXEC_GetRoutineInvocationInfo(
       handle,
       serializedInvocationInfo,
       invocationInfoMaxLen,
       invocationInfoLenOut,
       serializedPlanInfo,
       planInfoMaxLen,
       planNum,
       planInfoLenOut);

  if (retcode != 0 && diags)
    {
      SQL_EXEC_MergeDiagnostics_Internal(*diags);
      SQL_EXEC_ClearDiagnostics(NULL);
    }

  return retcode;
}

Lng32 ExeCliInterface::putRoutine(
     /* IN */     Int32         handle,
     /* IN/OUT */ ComDiagsArea *diags)
{
  Lng32 retcode = 0;

  SQL_EXEC_ClearDiagnostics(NULL);

  retcode = SQL_EXEC_PutRoutine(
       handle);

  if (retcode != 0 && diags)
    {
      SQL_EXEC_MergeDiagnostics_Internal(*diags);
      SQL_EXEC_ClearDiagnostics(NULL);
    }

  return retcode;
}
