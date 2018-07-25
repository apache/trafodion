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
****************************************************************************
*
* File:         ComTdbExplain.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COMTDBEXPLAIN_H
#define COMTDBEXPLAIN_H

#include "Platform.h"
#include "ComSizeDefs.h"
#include "ComTdb.h"
#include "ExpCriDesc.h"
#include "exp_attrs.h"

class ExplainDesc;
class ExplainTuple;

// Column info for the EXPLAIN__ virtual table
// The EXPLAIN__ table uses SQLARK_EXPLODED_FORMAT tuple format.
// The column offset information comments can be useful for debugging.
//static const ComTdbVirtTableColumnInfo explainVirtTableColumnInfo[] =
static const ComTdbVirtTableColumnInfo explainVirtTableColumnInfo[] =
{                                                                                     // offset
  { "SYSKEY",              0,  COM_USER_COLUMN, REC_BIN32_UNSIGNED,   4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  //    0
  { "MODULE_NAME",   1,  COM_USER_COLUMN, REC_BYTE_F_ASCII,    60, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  //    4
  { "STATEMENT_NAME", 2,     COM_USER_COLUMN, REC_BYTE_F_ASCII,    60, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  //   66
  { "PLAN_ID",          3,   COM_USER_COLUMN, REC_BIN64_SIGNED,     8, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  //  128
  { "SEQ_NUM",        4,     COM_USER_COLUMN, REC_BIN32_SIGNED,     4, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  //  144
  { "OPERATOR",      5,      COM_USER_COLUMN, REC_BYTE_F_ASCII,    30, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  //  152
  { "LEFT_CHILD_SEQ_NUM",  6, COM_USER_COLUMN, REC_BIN32_SIGNED,     4, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  //  184
  { "RIGHT_CHILD_SEQ_NUM", 7, COM_USER_COLUMN, REC_BIN32_SIGNED,     4, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  //  192
  { "TNAME",            8,   COM_USER_COLUMN, REC_BYTE_F_ASCII,    60, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  //  200
  { "CARDINALITY",   9,     COM_USER_COLUMN, REC_FLOAT32,          4, TRUE , SQLCHARSETCODE_UNKNOWN , 22, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  // NV:262 SQ:442
  { "OPERATOR_COST", 10,      COM_USER_COLUMN, REC_FLOAT32,          4, TRUE , SQLCHARSETCODE_UNKNOWN , 22, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  // NV:268 SQ:448
  { "TOTAL_COST",     11, COM_USER_COLUMN, REC_FLOAT32,          4, TRUE , SQLCHARSETCODE_UNKNOWN , 22, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  // NV:276 SQ:456
  { "DETAIL_COST",    12, COM_USER_COLUMN, REC_BYTE_V_ASCII,   200, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0},  // NV:284 SQ:464
  { "DESCRIPTION",    13, COM_USER_COLUMN, REC_BYTE_V_ASCII,  10000, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0}   // NV:488 SQ:664
};

#define EXPLAIN_DESCRIPTION_INDEX 13

//
// Task Definition Block for Explain Function:
//
// Notable contents:
// 
// -  scanPred_ a scan predicate to be applied to each tuple in the
//    explain tree.
//
// -  paramsExpr - a contiguous move expression to be applied to the input
//    which will populate a tuple with the parameters.

class ComTdbExplain : public ComTdb
{
  friend class ExExplainTcb;

public:

  // Constructors

  // Default constructor (used in ComTdb::fixupVTblPtr() to extract
  // the virtual table after unpacking.

  ComTdbExplain();

  // Constructor used by the generator.
  ComTdbExplain(ex_cri_desc *criDescParentDown,
	       ex_cri_desc *criDescParentUp,
	       queue_index queueSizeDown,
	       queue_index queueSizeUp,
	       const unsigned short tuppIndex,
	       ex_expr *scanPred,
	       ex_cri_desc *criDescParams,
	       Lng32 tupleLength,
	       ex_expr *paramsExpr,
	       Lng32 numBuffers,
	       ULng32 bufferSize);
  
  // This always returns TRUE from now
  Int32 orderedQueueProtocol() const;

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ComTdb::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(ComTdbExplain); }

  // Pack and Unpack routines
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // For the GUI, Does nothing right now
  void display() const;

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

  // The index of the ATP being returned to the parent, that has the
  // new row (explain Info)
  inline unsigned short getTuppIndex() const;

  // A predicate to be applied to each tuple contained in the
  // statement being explained.
  inline ex_expr *getScanPred() const;

  // A contiguous move expression which when evaluated will place values
  // for the parameters (Module Name and Statement Pattern) in the paramsTuple_
  // of the TCB for this node.
  inline ex_expr *getParamsExpr() const;

  // A cri descriptor for the paramsAtp_ (in TCB).  Used when evaluating
  // the params expression.  The paramsTuple_ is always referenced through
  // the last entry in the paramsAtp_.
  inline ex_cri_desc *criDescParams() const;

  // Descriptions of the parameters in the paramsTuple_
  // Information is retrieved from yje criDescParams_
  inline Attributes *getAttrModName();
  inline Attributes *getAttrStmtPattern();
  inline Lng32 getOffsetModName();
  inline Lng32 getLengthModName();
  inline Lng32 getOffsetStmtPattern();
  inline Lng32 getVCIndOffsetStmtPattern();
  inline Lng32 getVCIndicatorLength();
  inline Lng32 getLengthStmtPattern();
  inline Lng32 getTupleLength() const;

  // Virtual routines to provide a consistent interface to TDB's

  // 
  virtual const ComTdb *getChild(Int32 /*child*/) const { return NULL; };

  // numChildren always returns 0 for ComTdbExplain
  virtual Int32 numChildren() const;

  virtual const char *getNodeName() const { return "EX_EXPLAIN"; };

  // numExpressions always returns 2 for ComTdbExplain
  virtual Int32 numExpressions() const;
  
  // The names of the expressions
  virtual const char * getExpressionName(Int32) const;

  // The expressions thenselves
  virtual ex_expr* getExpressionNode(Int32);

  static Int32 getVirtTableNumCols()
  {
    return sizeof(explainVirtTableColumnInfo)/sizeof(ComTdbVirtTableColumnInfo);
  }

  static ComTdbVirtTableColumnInfo * getVirtTableColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)explainVirtTableColumnInfo;
  }

protected:

  // A predicate to be applied to each tuple contained in the
  // statement being explained.
  ExExprPtr scanPred_;                                           // 00-07

  // A contiguous move expression which when evaluated will place values
  // for the parameters (Module Name and Statement Pattern) in the paramsTuple_
  // of the TCB for this node.
  ExExprPtr paramsExpr_;                                         // 08-15

  // A cri descriptor for the paramsAtp_ (in TCB).  Used when evaluating
  // the params expression.  The paramsTuple_ is always referenced through
  // the last entry in the paramsAtp_.
  ExCriDescPtr criDescParams_;                                   // 16-23

  // Length of the paramsTuple_ to be allocated
  Int32 tupleLength_;                                            // 24-27

  // The index of the ATP being returned to the parent, that has the
  // new row (explain Info)
  UInt16 tuppIndex_;                                             // 28-29

  char fillersComTdbExplain_[34];                                // 30-63

};


// Inline Routines for ComTdbExplain:

inline Int32
ComTdbExplain::orderedQueueProtocol() const
{
  return -1; // returns true
};

inline unsigned short
ComTdbExplain::getTuppIndex() const
{
  return tuppIndex_;
};

inline ex_expr *
ComTdbExplain::getScanPred() const
{
  return scanPred_;
};

inline ex_cri_desc *
ComTdbExplain::criDescParams() const
{
  return criDescParams_;
};

inline Lng32
ComTdbExplain::getOffsetModName()
{
  // The moduleName is the first attribute in the tuple.
  return 
    criDescParams_->
      getTupleDescriptor(criDescParams_->noTuples() - 1)->
	getAttr(0)->
	  getOffset();
};

inline Lng32
ComTdbExplain::getLengthModName()
{
  // The moduleName is the first attribute in the tuple.
  return
    criDescParams_->
      getTupleDescriptor(criDescParams_->noTuples() - 1)->
	getAttr(0)->
	  getLength();
};

inline Lng32
ComTdbExplain::getOffsetStmtPattern()
{
  // The statement Pattern is the second attribute in the tuple.
  return
    criDescParams_->
      getTupleDescriptor(criDescParams_->noTuples() - 1)->
	getAttr(1)->
	  getOffset();
};

inline Lng32
ComTdbExplain::getVCIndOffsetStmtPattern()
{
  // The statement Pattern is the second attribute in the tuple.
  return
    criDescParams_->
      getTupleDescriptor(criDescParams_->noTuples() - 1)->
	getAttr(1)->
	  getVCLenIndOffset();
};

inline Lng32
ComTdbExplain::getVCIndicatorLength()
{
  // The statement Pattern is the second attribute in the tuple.
  return
    criDescParams_->
      getTupleDescriptor(criDescParams_->noTuples() - 1)->
	getAttr(1)->
          getVCIndicatorLength();
};

inline Lng32
ComTdbExplain::getLengthStmtPattern()
{
  // The statement Pattern is the second attribute in the tuple.
  return
    criDescParams_->
      getTupleDescriptor(criDescParams_->noTuples() - 1)->
	getAttr(1)->
	  getLength();
};

inline Attributes *
ComTdbExplain::getAttrModName()
{
  // The moduleName is the first attribute in the tuple.
  return
    criDescParams_->
      getTupleDescriptor(criDescParams_->noTuples() - 1)->
	getAttr(0);
};

inline Attributes *
ComTdbExplain::getAttrStmtPattern()
{
  // The statement Pattern is the second attribute in the tuple.
  return
    criDescParams_->
      getTupleDescriptor(criDescParams_->noTuples() - 1)->
	getAttr(1);
};

inline ex_expr *
ComTdbExplain::getParamsExpr() const
{
  return paramsExpr_;
};

inline Lng32
ComTdbExplain::getTupleLength() const
{
  return tupleLength_;
};

#endif


