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
* File:         ComTdbTranspose.h
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

#ifndef COMTDBTRANSPOSE_H
#define COMTDBTRANSPOSE_H

#include "ComTdb.h"

// class ComTdbTranspose --------------------------------------------------
// The Task Definition Block for the transpose operator.  This structure is
// produced by the generator and is passed to the executor as part of
// a TDB tree.  This structure contains all the static information 
// necessary to execute the Transpose operation.
// 
class ComTdbTranspose : public ComTdb
{
  // The Task Control Block for the transpose operator.  This struture
  // contains the run-time information necessary to execute the Transpose
  // operator.
  //
  friend class ExTransposeTcb;

  // The private state for the transpose operator.  This structure contains
  // the information associated with a given request of the transpose TCB.
  //
  friend class ExTransposePrivateState;
  
public:

  // Default Constructor.
  // Used when unpacking the Transpose TDB.  Used to get a pointer
  // to the Virtual Method Table.
  //
  ComTdbTranspose();
  
  // Construct a copy of the given node.
  // (This constructor does not seem to be used)
  // 
  ComTdbTranspose(const ComTdbTranspose *transposeTdb);
	
  // Construct a new Transpose TDB.
  // This constructor is call by the generator (PhysTranspose::codeGen() in
  // GenRelMisc.cpp.) 
  //
  // Parameters
  //
  // ComTdb *childTdb
  //  IN: The child of this Transpose TDB.
  //
  // ex_expr **transColExprs
  //  IN: A vector of pointers to ex_expr.  There are 'numTransExprs'
  //      in the vector.  Each expression represents a transpose expression.
  //      Each expression is a move expression which will generate a key
  //      value, and multiple values.  One value will be from a transpose
  //      expression, the others will generate the NULL value.
  //
  // int numTransExprs
  //  IN: The number of expressions in transColsExprs.
  //
  // ex_expr *afterTransPred
  //  IN: The selection Predicate for the Transpose operator. This expression
  //      is applied after the transpose columns have been generated (so this
  //      predicate will likely involve these generated values, other wise it
  //      could have been pushed to the child of this node.
  //
  // long transRowLen
  //  IN: The length of the tuple which will hold the generated values.  This
  //      tuple will be allocated by the transpose node.
  //
  // const unsigned short transTuppIndex
  //  IN: The index of the transpose tuple in the ATP.
  //
  // ex_cri_desc *givenCriDesc
  //  IN: The Cri Descriptor given to this node by its parent.
  //
  // ex_cri_desc *returnedCriDesc
  //  IN: The Cri Descriptor return to the parent node.
  //
  // queue_index fromParent
  //  IN: Recommended queue size for the down queue used to communicate 
  //      with the parent.
  //
  // queue_index toParent
  //  IN: Recommended queue size for the up queue used to communicate
  //      with the parent.
  //
  // Cardinality estimatedRowCount
  //  IN: compiler estimate on number of returned rows
  //
  //
  // long numBuffers
  //  IN: Recommended number of buffers to allocate.
  //
  // unsigned long bufferSize
  //  IN: Recommended size for pool buffers.
  //
  ComTdbTranspose(ComTdb * childTdb,
		  ex_expr ** transColExprs,
		  Int32 numTransExprs,
		  ex_expr * afterTransPred,
		  Lng32 transRowLen,
		  const unsigned short transTuppIndex,
		  ex_cri_desc * givenCriDesc,
		  ex_cri_desc * returnedCriDesc,
		  queue_index down,
		  queue_index up,
		  Cardinality estimatedRowCount,
		  Lng32 numBuffers,
		  ULng32 bufferSize,
		  Space *space);

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
                                { return (short)sizeof(ComTdbTranspose); }


  // ComTdbTranspose::pack() ---------------------------------------------
  // Pack the transpose TDB for transmission from the compiler to the
  // executor, or from an ESP to DP2, etc.  This method needs to convert
  // all pointers to offsets, so that the memory containing this TDB fragment,
  // can be relocated and 'unpacked' at a new base address.
  //
  // Parameters
  //
  // void *space
  //  IN - The space object which was used to allocate this TDB. Used to
  //       compute offsets all pointers.  It is an error if any pointer
  //       that can be reached from this TDB points to memory outside 
  //       this space object.
  //
  Long pack(void *);

  // ComTdbTranspose::unpack() ---------------------------------------------
  // Unpack the transpose TDB after transmission from the compiler to the
  // executor, or from an ESP to DP2, etc.  This method needs to convert
  // all offsets to pointers, so that all pointers now reflected the new
  // location of the TDB fragment.
  //
  // Parameters
  //
  // void *base
  //  IN - The base address of the TDB fragment.  Pointers are calculated
  //       by adding the offset to the base address (more or less).
  //
  Lng32 unpack(void *, void * reallocator);
  
  // ComTdbTranspose::Display() -----------------------------------------
  // (Don't know why this is here.  It does not seem to be virtual and
  // on class seems to do anything for this method.)
  //
  void display() const;

  // Return a pointer to the child TBD of this transpose TDB.
  //
  inline ComTdb * getChildTdb() { return childTdb_; }

  // We are observing order queue protocol. Results from
  // a request are returned in full, before any of the results
  // of the next request are returned.
  //
  // Exclude this code from coverage analysis.
  // This code could be deleted since it is the same as the base implementation.
  Int32 orderedQueueProtocol() const { return -1; }

  // return a pointer to the specifed (by position) child TDB.
  // Transpose has only one child.
  //
  virtual const ComTdb *getChild(Int32 pos) const
  {
    if(pos == 0) 
      return childTdb_;
    return NULL;
  }

  // Return the number of children for this node.
  // Transpose has one child.
  //
  virtual Int32 numChildren() const { return 1; }

  // Return the number of expression this node has.
  // Transpose has a variable number of expressions.
  // There are numTransExprs_ transpose expressions, plus
  // the afterTransPred_ selection predicate.
  //
  virtual Int32 numExpressions() const
  { 
    return numTransExprs_ + 1;
  }

  // Return the expression by position.
  // The transpose expressions come first, followed
  // by the selection pred.
  //
  virtual ex_expr * getExpressionNode(Int32 pos) 
  {
    if(transColExprs_ && (pos >= 0) && (pos < numTransExprs_)) 
      return( transColExprs_[pos] );
    else if(pos == numTransExprs_)
      return( afterTransPred_ );
    return( NULL );
  }

  // Return the name of an expression by position.
  // The transpose expressions come first, followed
  // by the selection pred.
  //
  virtual const char * getExpressionName(Int32 pos) const
  {
    if(transColExprs_ && (pos >= 0) && (pos < numTransExprs_)) 
      return( "transColExpr" );
    else if(pos == numTransExprs_)
      return( "afterTransPred" );
    return( NULL );
  }

  virtual const char *getNodeName() const
  {
    return "EX_TRANSPOSE";
  }

  // Return the number of transpose expressions.  Used by
  // the Transpose TDB and TCB methods.
  //
  Int32 numTransExprs() { return numTransExprs_; };
protected:

  // The child of this Transpose TDB.
  //
  ComTdbPtr childTdb_;                                  // 00-07

  // A vector of pointers to ex_expr.  There are 'numTransExprs'
  // in the vector.  Each expression represents a transpose expression.
  // Each expression is a move expression which will generate a key
  // value, and multiple values.  One value will be from a transpose
  // expression, the others will generate the NULL value.
  //  
  ExExprPtrPtr transColExprs_;                          // 08-15

  // The number of expressions in transColsExprs.
  //
  Int32 numTransExprs_;                                 // 16-19

  // The length of the tuple which will hold the generated values.  This
  // tuple will be allocated by the transpose node.
  //
  Int32 transRowLen_;                                   // 20-23

  // The selection Predicate for the Transpose operator. This expression
  // is applied after the transpose columns have been generated (so this
  // predicate will likely involve these generated values, other wise it
  // could have been pushed to the child of this node.
  //
  ExExprPtr afterTransPred_;                            // 24-31

  // The index of the transpose tupp in the ATP.
  //
  UInt16 transTuppIndex_;                               // 32-33

  char fillersComTdbTranspose_[46];                     // 34-79

};

#endif

