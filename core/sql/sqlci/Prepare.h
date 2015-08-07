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
#ifndef PREPARE_H
#define PREPARE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Prepare.h
 * RCS:          $Id: Prepare.h,v 1.7 1998/09/07 21:49:50  Exp $
 * Description:  
 *               
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1998/09/07 21:49:50 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include "SQLCLIdev.h"
#include "SqlciDefs.h"

// The following class was named as Entry before. I renamed it to PrepEntry
// because the file btree.h defines another class named Entry and the Tandem
// linker sometimes resolves the destructor to the wrong one. -- Freda Xu
class PrepEntry {
  Lng32 datatype_;
  Lng32 length_;
  Lng32 scale_;
  Lng32 precision_;
  Lng32 nullFlag_;
  char * heading_;
  Lng32 headingLen_;
  char * outputName_;
  Lng32 outputNameLen_;
  Lng32 displayLen_;
  Lng32 displayBufLen_;
  Lng32 charsetEnum_;
  char * charset_;
  char * tableName_;
  Lng32 tableLen_;
  char * varPtr_;
  char * indPtr_;
public:
  PrepEntry(Lng32 datatype, Lng32 length, Lng32 scale, Lng32 precision,
	    Lng32 nullFlag, char * heading_, Lng32 headingLen,
	    char * outputName, Lng32 outputNameLen_, Lng32 displayLen_, 
	    Lng32 displayBufLen_, Lng32 charsetEnum_, char * tableName, 
	    Lng32 tableLen);
  ~PrepEntry();

  Lng32 datatype()   { return datatype_;}
  Lng32 length()     { return length_;}
  Lng32 scale()      { return scale_; }
  Lng32 precision()  { return precision_;}
  Lng32 nullFlag()   { return nullFlag_;}
  char * heading()  { return heading_; }
  Lng32 headingLen() { return headingLen_;}
  char * outputName()  { return outputName_; }
  Lng32 outputNameLen() { return outputNameLen_;}
  Lng32 displayLen() { return displayLen_; }
  Lng32 displayBufLen() { return displayBufLen_; }
  Lng32 charsetEnum() { return charsetEnum_; }
  char * charset()  { return charset_; }
  char * tableName() { return tableName_; }
  Lng32 tableLen() { return tableLen_; }
  char * varPtr() { return varPtr_;}
  char * indPtr() { return indPtr_;}
  void setVarPtr(char * vp) { varPtr_ = vp;}
  void setIndPtr(char * ip) { indPtr_ = ip;}
  
};

////////////////////////////////////////////////////
class PrepStmt {
  friend class PreparedStmts;

  char * stmt_name;
  Lng32 stmt_name_len;

  char * stmt_str;
  Lng32 stmt_str_len;

  SQLDESC_ID * sql_src;

  SQLSTMT_ID * stmt;
  SQLDESC_ID * input_desc;
  SQLDESC_ID * output_desc;
  dml_type type;

  Lng32 queryType_;
  Lng32 subqueryType_;

  Lng32 numInputEntries_;
  Lng32 numOutputEntries_;

  PrepEntry ** outputEntries_;
  PrepEntry ** inputEntries_;

  // space to fetch the selected row.
  Lng32 outputDatalen_;
  char * outputData_;

  // this is where the formatted output row will be created.
  Lng32 outputBuflen_;
  char * outputBuf_;

  // query id for this query
  char * uniqueQueryId_;
  Lng32 uniqueQueryIdLen_;
public:
  PrepStmt(const char * stmt_name_, dml_type type_ = DML_SELECT_TYPE);
  ~PrepStmt();
  void set(const char * str_,
	   SQLDESC_ID * sql_src_, SQLSTMT_ID * stmt_,
	   Lng32 numInputEntries, SQLDESC_ID * input_desc_, 
	   Lng32 numOutputEntries, SQLDESC_ID * output_desc_);

  void remove();

  inline char * getStmtName(){return stmt_name;};
  Lng32 getStmtNameLen(){return stmt_name_len;};

  inline SQLDESC_ID * getSqlSrc(){return sql_src;};
  inline SQLDESC_ID * getInputDesc(){return input_desc;};
  inline SQLDESC_ID * getOutputDesc(){return output_desc;};
  inline SQLSTMT_ID * getStmt(){return stmt;};

  inline char * getStr(){return stmt_str;};
  inline Lng32 getStrLen(){return stmt_str_len;};

  inline dml_type getType(){return type;};

  short contains(const char * value) const;

  Lng32 &queryType() { return queryType_; }
  void setSubqueryType(Lng32 subqueryType)
  { subqueryType_ = subqueryType; }
  Lng32 getSubqueryType() { return subqueryType_; }


  Lng32  numInputEntries()  { return numInputEntries_;}
  Lng32  numOutputEntries() { return numOutputEntries_;}

  void setOutputDesc(Lng32 n, SQLDESC_ID *desc)
  {
    numOutputEntries_ = n;
    output_desc = desc;
  }
  
  PrepEntry ** &outputEntries() { return outputEntries_;}
  PrepEntry ** &inputEntries()  { return inputEntries_;}

  Lng32  &outputDatalen() { return outputDatalen_;}
  char* &outputData() { return outputData_;}

  Lng32  &outputBuflen() { return outputBuflen_;}
  char* &outputBuf() { return outputBuf_;}

  Lng32  &uniqueQueryIdLen() { return uniqueQueryIdLen_;}
  char* &uniqueQueryId() { return uniqueQueryId_;}
};

class CursorStmt {
public:
  CursorStmt(char * cursorName, SQLSTMT_ID * cursorStmtId,
	     PrepStmt * stmt, 
	     Int16 internallyPrepared = -1);
  ~CursorStmt();

  char * cursorName() { return cursorName_;};
  SQLSTMT_ID * cursorStmtId() { return cursorStmtId_; };
  PrepStmt * prepStmt() { return stmt_; };
  short contains(const char * value) const;
  Int16 internallyPrepared() { return internallyPrepared_;};
  Lng32 getResultSetIndex() const { return resultSetIndex_; }
  void setResultSetIndex(Lng32 i) { resultSetIndex_ = i; }
private:  
  char * cursorName_;
  Lng32 cursorNameLen_;
  SQLSTMT_ID * cursorStmtId_;
  PrepStmt * stmt_;
  Int16 internallyPrepared_;
  Lng32 resultSetIndex_;
};

class PreparedStmts {
  PrepStmt * prep_stmt_hdr;
public:
  PreparedStmts();
  ~PreparedStmts();

  void addPrepStmt(PrepStmt * in_prep_stmt);
  PrepStmt * getPrepStmt(const char * stmt_name_);
  void removePrepStmt(const char * stmt_name_);

};

#endif
