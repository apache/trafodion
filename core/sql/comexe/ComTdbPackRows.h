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
* File:         ComTdbPackRows.h
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

#ifndef COMTDBPACKROWS_H
#define COMTDBPACKROWS_H

// External forward declarations
//
#include "ComTdb.h"


class ComTdbPackRows: public ComTdb
{
  friend class ExPackRowsTcb;
  friend class ExPackRowsPrivateState;
  
public:

  // Default Constructor.
  ComTdbPackRows();
  
  // Copy constructor.
  ComTdbPackRows(const ComTdbPackRows* packTdb);

  // Constructor used by the PhyPack::codeGen() in GenRelMisc.cpp.
  ComTdbPackRows(ComTdb* childTdb,
            ex_expr* packExpr,
            ex_expr* predExpr,
            const unsigned short packTuppIndex,
            Lng32 packTuppLen,
            ex_cri_desc* givenCriDesc,
            ex_cri_desc* returnedCriDesc,
            queue_index fromParent,
            queue_index toParent);

 
  // Pack the pack TDB for transmission from one process to another.
  Long pack(void *);

  // Unpack the pack TDB after receiving it from another process.
  Lng32 unpack(void *, void * reallocator);
 
  // Don't know what it is for ??
  void display() const;

  // Return a pointer to the child TBD of this Pack TDB.
  inline ComTdb* getChildTdb() { return childTdb_; }

  // We are observing order queue protocol.
  Int32 orderedQueueProtocol() const ;

  // Return a pointer to the specifed (by position) child TDB.
  virtual const ComTdb* getChild(Int32 pos) const
  {
    if(pos == 0) return childTdb_;
    return NULL;
  }

  // Return the number of children for this node.
  virtual Int32 numChildren() const { return 1; }

  // Return the number of expression this node has.
  virtual Int32 numExpressions() const { return 2; }

  // Return the expression by position.
  virtual ex_expr* getExpressionNode(Int32 pos) 
  {
    switch(pos)
    {
      case 0:  return packExpr_;
      case 1:  return predExpr_;
      default: break;
    }
    return NULL;
  }

  // Return the name of an expression by position.
  virtual const char* getExpressionName(Int32 pos) const
  {
    switch(pos)
    {
      case 0:  return "packExpr";
      case 1:  return "predExpr";
      default: break;
    }
    return NULL;
  }

  virtual const char* getNodeName() const { return "EX_PACK"; }

protected:

  // The child of this Pack TDB.
  ComTdbPtr childTdb_;                                            // 00-07

  // The expression to do the real packing.
  ExExprPtr packExpr_;                                            // 08-15

  // The selection predicate.
  ExExprPtr predExpr_;                                            // 16-23

  // The Cri Descriptor given to this node by its parent.
  ExCriDescPtr givenCriDesc_;                                     // 24-31

  // The Cri Descriptor return to the parent node.
  ExCriDescPtr returnedCriDesc_;                                  // 32-39

  // The length of the packed record.
  Int32 packTuppLen_;                                             // 40-43

  // The length of the down queue used to communicate with the parent.
  UInt32 fromParent_;                                             // 44-47

  // The length of the up queue used to communicate with the parent.
  UInt32 toParent_;                                               // 48-51

  // The size of pool buffers to allocate.
  UInt32 bufferSize_;                                             // 52-55

  // The number of buffers to allocate for the pool.
  UInt16 noOfBuffers_;                                            // 56-57

  // The index of the generated tupp in the ATP.
  UInt16 packTuppIndex_;                                          // 58-59

  char fillersComTdbPackRows_[36];                                // 60-95


};

#endif

