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
* File:         ComTdbFirstN.h
* Description:  
*
* Created:      5/2/2003
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COM_FIRSTN_H
#define COM_FIRSTN_H

#include "ComTdb.h"

class ComTdbFirstN : public ComTdb
{
  friend class ExFirstNTcb;

public:

  ComTdbFirstN();

  ComTdbFirstN(
      ComTdb * child_tdb,
      Int64 firstNRows,
      ex_expr * firstNRowsExpr,
      ex_cri_desc *workCriDesc,
      ex_cri_desc * givenCriDesc,
      ex_cri_desc * returnedCriDesc,
      queue_index down,
      queue_index up,
      Lng32 numBuffers,
      ULng32 bufferSize);

  ~ComTdbFirstN();

  Int32 orderedQueueProtocol() const
  {
    return -1;
  };

  ComTdb * getChildTdb(){return tdbChild_;};

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

  virtual short getClassSize()
  { return (short)sizeof(ComTdbFirstN); }

  virtual Long pack (void *);
  virtual Lng32 unpack(void *, void * reallocator);

  void display() const;

  virtual const ComTdb* getChild(Int32 pos) const
  {
    if (pos == 0)
      return tdbChild_.getPointer();
    else
      return NULL;
  }

  virtual Int32 numChildren() const { return 1; }
  virtual const char *getNodeName() const { return "EX_FIRSTN"; };
  virtual Int32 numExpressions() const { return 1; };
  virtual ex_expr* getExpressionNode(Int32 pos) { return firstNRowsExpr_; };
  virtual const char * getExpressionName(Int32 pos) const { return "firstNRowsExpr"; };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  virtual void displayContents(Space *space,ULng32 flag);

  Int64 firstNRows() { return firstNRows_; }
protected:

  Int64 firstNRows_;                // 00-07
  ComTdbPtr tdbChild_;              // 08-15
  ExExprPtr firstNRowsExpr_;      // 16-23
  ExCriDescPtr workCriDesc_;       // 24-31
  char filler0ComTdbFirstN_[32];    // 32-63  unused
};

#endif
