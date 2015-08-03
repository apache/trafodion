/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDMLSetTransaction.C
 * 
 * Description:  class for parse node representing Set Transaction statements
 *
 *
 * Created:      07/01/96
 * Language:     C++
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


#include "ComDiags.h"
#include "ComSmallDefs.h"
#include "StmtDDLNode.h"
#include "StmtDMLSetTransaction.h"
#include "SQLCLIdev.h"
#include "ItemOther.h"
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#include "SqlParserGlobals.h"

//
// class StmtDMLSetTransaction
//

StmtDMLSetTransaction::StmtDMLSetTransaction(ItemExpr * child)
  : childNode_(child),
    diagSize_(NULL),
    isolationLevel_(TransMode::IL_NOT_SPECIFIED_),
    accessMode_(TransMode::AM_NOT_SPECIFIED_),
    rollbackMode_(TransMode::ROLLBACK_MODE_NOT_SPECIFIED_),
    autoabortInterval_(-1),
    multiCommit_(TransMode::MC_NOT_SPECIFIED_),
    multiCommitSize_(0),
    StmtNode(STM_SET_TRANS)
{
  // traverse the child subtree and collect informations and store it in the root node for easy access.

  if (childNode_->getOperatorType() == ITM_ITEM_LIST)
    {
      ItemExprList  list ((ItemList *) childNode_,0);
      for (CollIndex i = 0; i < list.entries(); i++)
	setAttribute((list)[i]);
    }
  else
    setAttribute(childNode_);
}
  
void StmtDMLSetTransaction::setAttribute(ItemExpr * child)
{
  switch(child->getOperatorType())
    {
    case ITM_SET_TRANS_ISOLATION_LEVEL : 
      { 
	if (isIsolationLevelSpec())
	  {
	    // Duplicate Isolation Level Phrase Specified.
	    *SqlParser_Diags << DgSqlCode(-3115);
	  }
	TxnIsolationItem *itemIso = (TxnIsolationItem *) child;
	isolationLevel_ = itemIso->getIsolationLevel();
	break;
      }

    case ITM_SET_TRANS_ACCESS_MODE : 
      {
	if (isAccessModeSpec())
	  {
	    // Duplicate Transaction Mode Phrase Specified.
            *SqlParser_Diags << DgSqlCode(-3116);
	  }
	TxnAccessModeItem * itemTran = (TxnAccessModeItem *) child;
	accessMode_ = itemTran->getAccessMode();
	break;
      }

    case ITM_SET_TRANS_DIAGS : 
      {
	if (diagSize_)
	  {
	    // Duplicate Diagnostic size Phrase Specified.
            *SqlParser_Diags << DgSqlCode(-3117);
	  }
	DiagnosticSizeItem * itemDiags = (DiagnosticSizeItem* ) child;  
        diagSize_ = itemDiags->getDiagSizeNode();
        break ; 
      }
      case ITM_SET_TRANS_ROLLBACK_MODE : 
      {
	if (isRollbackModeSpec())
	  {
	    // Duplicate Transaction Mode Phrase Specified.
            *SqlParser_Diags << DgSqlCode(-3237);
	  }
	TxnRollbackModeItem * itemRoll = (TxnRollbackModeItem *) child;
	rollbackMode_ = itemRoll->getRollbackMode();
	break;
      }
      case ITM_SET_TRANS_AUTOABORT_INTERVAL : 
      {
	if (isAutoabortIntervalSpec())
	  {
	    // Duplicate Transaction Mode Phrase Specified.
            *SqlParser_Diags << DgSqlCode(-3238);
	  }
	TxnAutoabortIntervalItem * itemAuto = (TxnAutoabortIntervalItem *) child;
	autoabortInterval_ = itemAuto->getAutoabortInterval();
	break;
      }

      case ITM_SET_TRANS_MULTI_COMMIT : 
      {
	if (isMultiCommitSize() || isMultiCommitSpec())
	  {
	    // Duplicate Transaction Mode Phrase Specified.
            *SqlParser_Diags << DgSqlCode(-3239);
	  }
	TxnMultiCommitItem * itemMultiCommit = (TxnMultiCommitItem *) child;
	multiCommitSize_ = itemMultiCommit->getMultiCommitSize();
        multiCommit_ = itemMultiCommit->getMultiCommit();
	break;
      }
		    
    default : ABORT("Internal Logic Error");
    }
}

StmtDMLSetTransaction::~StmtDMLSetTransaction()
{}

const NAString 
StmtDMLSetTransaction::displayLabel1() const
{
  return NAString("Transaction Statement");
}

const NAString 
StmtDMLSetTransaction::getText() const 
{
  return NAString("Set Transaction Statement");
}


TxnIsolationItem::TxnIsolationItem(TransMode::IsolationLevel il)
  : isolationLevel_(il),
    ItemExpr(ITM_SET_TRANS_ISOLATION_LEVEL)
{}


TxnAccessModeItem::TxnAccessModeItem(TransMode::AccessMode am)
  : accessMode_(am),
    ItemExpr(ITM_SET_TRANS_ACCESS_MODE)
{}


DiagnosticSizeItem::DiagnosticSizeItem(ItemExpr * child)
  : ItemExpr(ITM_SET_TRANS_DIAGS, child)
{}

TxnRollbackModeItem::TxnRollbackModeItem(TransMode::RollbackMode rb)
  : rollbackMode_(rb),
    ItemExpr(ITM_SET_TRANS_ROLLBACK_MODE)
{}

TxnAutoabortIntervalItem::TxnAutoabortIntervalItem(Lng32 val)
  : autoabortInterval_(val),
    ItemExpr(ITM_SET_TRANS_AUTOABORT_INTERVAL)
{}

TxnMultiCommitItem::TxnMultiCommitItem(ULng32 val, NABoolean mc)
  : multiCommitSize_(val),
    ItemExpr(ITM_SET_TRANS_MULTI_COMMIT)
{
  if (mc == TRUE)
    multiCommit_ = TransMode::MC_ON_;
  else
    multiCommit_ = TransMode::MC_OFF_;
}

TxnMultiCommitItem::TxnMultiCommitItem(NABoolean mc)
  : multiCommitSize_(0),
    ItemExpr(ITM_SET_TRANS_MULTI_COMMIT)
{
  if (mc == TRUE)
    multiCommit_ = TransMode::MC_ON_;
  else
    multiCommit_ = TransMode::MC_OFF_;
}
