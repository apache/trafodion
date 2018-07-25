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
* File:         ComTdbTupleFlow.h
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

#ifndef COM_TUPLE_FLOW_H
#define COM_TUPLE_FLOW_H

#include "ComTdb.h"

class ComTdbTupleFlow : public ComTdb
{
  friend class ExTupleFlowTcb;
  friend class ExTupleFlowPrivateState;

  enum TFlowFlags 
  { 
    VSBB_INSERT = 0x0001,
    ROWSET_ITERATOR = 0x0002,
    USER_SIDETREE_INSERT = 0x0004,
    SEND_EOD_TO_TGT = 0x0008
  };

protected:
  
  ComTdbPtr    tdbSrc_;              // 00-07
  ComTdbPtr    tdbTgt_;              // 08-15
  
  // if present, then used to compute the 'row' that
  // has to be sent to right (target) child.
  ExExprPtr    tgtExpr_;             // 16-23
  
  ExCriDescPtr workCriDesc_;         // 24-31
  
  UInt32 flags_;                     // 32-35

  char fillersComTdbTupleFlow_[36];  // 36-71

  inline NABoolean vsbbInsertOn()
  {
    if (flags_ & VSBB_INSERT)
      return TRUE;
    else
      return FALSE;
  }

  inline NABoolean isRowsetIterator()
  {
    if (flags_ & ROWSET_ITERATOR)
      return TRUE;
    else
      return FALSE;
  }

public:

  ComTdbTupleFlow()
    : ComTdb(ComTdb::ex_TUPLE_FLOW, eye_TUPLE_FLOW)
  {}
  
  ComTdbTupleFlow(ComTdb * tdb_src,
		  ComTdb * tdb_tgt,
		  ex_cri_desc * given_cri_desc,
		  ex_cri_desc * returned_cri_desc,
		  ex_expr * tgt_expr,
		  ex_cri_desc * work_cri_desc,
		  queue_index down,
		  queue_index up,
		  Cardinality estimatedRowCount,
		  Lng32 num_buffers,
		  ULng32 buffer_size,
		  NABoolean vsbbInsert,
		  NABoolean rowsetIterator,
		  NABoolean tolerateNonFatalError);
  
  virtual ~ComTdbTupleFlow();

  NABoolean userSidetreeInsert()
  {
    return ((flags_ & USER_SIDETREE_INSERT) != 0);
  }
  void setUserSidetreeInsert(NABoolean v)
  {
    (v ? flags_ |= USER_SIDETREE_INSERT : flags_ &= ~USER_SIDETREE_INSERT);
  }

  NABoolean sendEODtoTgt()
  {
    return ((flags_ & SEND_EOD_TO_TGT) != 0);
  }
  void setSendEODtoTgt(NABoolean v)
  {
    (v ? flags_ |= SEND_EOD_TO_TGT : flags_ &= ~SEND_EOD_TO_TGT);
  }

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

  virtual short getClassSize() { return (short)sizeof(ComTdbTupleFlow); }
  Long      pack(void *);
  Lng32      unpack(void *, void * reallocator);
  
  void display() const {};
  
  Int32 orderedQueueProtocol() const
  {
    return -1; // return true
  };

  inline void setSrcTdb(ComTdb * src)  { tdbSrc_ = src; };
  inline void setTgtTdb(ComTdb * tgt)  { tdbTgt_ = tgt; };
  
  virtual const ComTdb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const { return 2; }
  virtual const char *getNodeName() const { return "EX_TUPLE_FLOW"; };
  virtual Int32 numExpressions() const { return 1; }
  virtual ex_expr* getExpressionNode(Int32 pos);
  virtual const char * getExpressionName(Int32 pos) const;

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  virtual void displayContents(Space *space,ULng32 flag);
};


#endif

