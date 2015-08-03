#ifndef STMTDMLSET_TRANSACTION_H
#define STMTDMLSET_TRANSACTION_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDMLSetTransaction.h
 * 
 * Description:  class for parse node representing Set Transaction statements
 *
 * Created:      07/01/96
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
 *****************************************************************************
 */

#include "ComTransInfo.h"
#include "StmtNode.h"
#include "ItemExpr.h"
#include "ItemExprList.h"


class StmtDMLSetTransaction : public StmtNode
{

public:

  StmtDMLSetTransaction(ItemExpr * transaction_mode); 
 
  virtual ~StmtDMLSetTransaction();

  TransMode::AccessMode getAccessMode() const	      { return accessMode_; }
  TransMode::IsolationLevel getIsolationLevel() const { return isolationLevel_;}
  ItemExpr * getDiagSizeNode() const		      { return diagSize_; }
  TransMode::RollbackMode getRollbackMode() const     { return rollbackMode_; }
  Lng32 getAutoAbortInterval() const		      { return autoabortInterval_ ;}
  TransMode::MultiCommit getMultiCommit() const       { return multiCommit_; }

  ULng32 getMultiCommitSize() const
  {return multiCommitSize_ ;}

  NABoolean isIsolationLevelSpec() const
  { return isolationLevel_ != TransMode::IL_NOT_SPECIFIED_; }

  NABoolean isAccessModeSpec() const
  { return accessMode_ != TransMode::AM_NOT_SPECIFIED_; }

  NABoolean isRollbackModeSpec() const
  { return rollbackMode_ != TransMode::ROLLBACK_MODE_NOT_SPECIFIED_; }

  NABoolean isAutoabortIntervalSpec() const
  { return autoabortInterval_ != -1; }

  NABoolean isMultiCommitSize() const
  { return multiCommitSize_ != 0; }

  NABoolean isMultiCommitSpec() const
  { return multiCommit_ != TransMode::MC_NOT_SPECIFIED_; }

  ExprNode * bindNode(BindWA *bindWAPtr);

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;
  void setAttribute(ItemExpr * child);

private:
  
  ItemExpr * childNode_;
  ItemExpr * diagSize_;
  TransMode::AccessMode accessMode_;
  TransMode::IsolationLevel isolationLevel_;
  TransMode::RollbackMode rollbackMode_;
  Lng32 autoabortInterval_;
  ULng32 multiCommitSize_;
  TransMode::MultiCommit multiCommit_;

}; // class StmtDMLSetTransaction


 
class TxnIsolationItem : public ItemExpr 
{
public:
  TxnIsolationItem(TransMode::IsolationLevel isolationLevel_);
  TransMode::IsolationLevel getIsolationLevel() const { return isolationLevel_;}
private:
  TransMode::IsolationLevel isolationLevel_;
};


class TxnAccessModeItem : public ItemExpr
{
public:
  TxnAccessModeItem(TransMode::AccessMode access_mode);  
  TransMode::AccessMode getAccessMode() const	{ return accessMode_; }
private:
  TransMode::AccessMode accessMode_;
};


class DiagnosticSizeItem : public ItemExpr
{
public: 
  DiagnosticSizeItem(ItemExpr * child);
  ItemExpr * getDiagSizeNode()	    { return getChild(0)->castToItemExpr(); }
private:
};

class TxnRollbackModeItem : public ItemExpr
{
public:
  TxnRollbackModeItem(TransMode::RollbackMode rollback_mode);  
  TransMode::RollbackMode getRollbackMode() const	{ return rollbackMode_; }
private:
  TransMode::RollbackMode rollbackMode_;
};

class TxnAutoabortIntervalItem : public ItemExpr
{
public:
  TxnAutoabortIntervalItem(Lng32 val);  
  Lng32 getAutoabortInterval() const	{ return autoabortInterval_; }

private:
  Lng32 autoabortInterval_;
};

class TxnMultiCommitItem : public ItemExpr
{
public:
  TxnMultiCommitItem(ULng32 val, NABoolean mc);
  TxnMultiCommitItem(NABoolean mc);
  ULng32 getMultiCommitSize() const	         { return multiCommitSize_; }
  TransMode::MultiCommit getMultiCommit() const  { return multiCommit_; }
private:
  ULng32 multiCommitSize_;
  TransMode::MultiCommit multiCommit_;
};

#endif	// STMTDMLSET_TRANSACTION_H
