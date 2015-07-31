#ifndef SQLCISTMTS_H
#define SQLCISTMTS_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciStmts.h
 * RCS:          $Id: SqlciStmts.h,v 1.2 1997/04/23 00:31:15  Exp $
 * Description:  
 *               
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1997/04/23 00:31:15 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
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
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
// Change history:
// 
// $Log: SqlciStmts.h,v $
// Revision 1.2  1997/04/23 00:31:15
// Merge of MDAM/Costing changes into SDK thread
//
// Revision 1.1.1.1.2.1  1997/04/11 23:25:04
// Checking in partially resolved conflicts from merge with MDAM/Costing
// thread. Final fixes, if needed, will follow later.
//
// Revision 1.4.4.1  1997/04/10 18:33:27
// *** empty log message ***
//
// Revision 1.1.1.1  1997/03/28 01:39:45
// These are the source files from SourceSafe.
//
// 
// 4     1/22/97 11:04p 
// Merged UNIX and NT versions.
// 
// 2     1/13/97 1:16p 
// Revision 1.4  1996/12/21 04:15:02
// Made most methods const.
//
// Revision 1.3  1996/05/31 21:35:57
// Corrected various bugs with FC and Repeat commands.
//
// Revision 1.2  1996/04/05 20:19:10
// Included the standard banner with RCS strings.
//
// Revision 1.1  1995/07/17 22:02:49
// Initial revision
//
// 
// -----------------------------------------------------------------------


class StmtEntry;
class InputStmt;

class SqlciStmts {
  StmtEntry * First;
  StmtEntry * Last;
  Lng32 last_stmt_num;
  Lng32 max_entries;
public:
  SqlciStmts(Lng32 max_entries_);
  ~SqlciStmts();
  void add(InputStmt * input_stmt_);
  void display(Lng32 num_stmts_) const;
  InputStmt * get(Lng32 stmt_num_) const;
  InputStmt * get(char * stmt_) const;
  InputStmt * get() const; 		// gets last statement
  void remove(); 			// removes last statement
  Lng32 last_stmt() const { return last_stmt_num; } 
};

class StmtEntry {
  Lng32 stmt_num;
  InputStmt * input_stmt;
  StmtEntry * next;
  StmtEntry * prev;
public:
  StmtEntry();
  ~StmtEntry();
  void disconnect();
  void set(Lng32 stmt_num_, InputStmt * input_stmt_);
  inline Lng32 getStmtNum() const		{ return stmt_num; }
  inline InputStmt * getInputStmt() const	{ return input_stmt; }
  inline StmtEntry * getNext() const		{ return next; }
  inline StmtEntry * getPrev() const		{ return prev; }
  inline void setNext(StmtEntry * next_)	{ next = next_; }
  inline void setPrev(StmtEntry * prev_)	{ prev = prev_; }

};

#endif
