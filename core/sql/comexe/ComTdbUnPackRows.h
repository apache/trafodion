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
* File:         ComTdbUnPackRows.h
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

#ifndef ComUnPackRows_h
#define ComUnPackRows_h

// External forward declarations
//
#include "ComTdb.h"


// class ComTdbUnPackRows --------------------------------------------------
// The Task Definition Block for the UnPackRows operator.  This structure is
// produced by the generator and is passed to the executor as part of
// a TDB tree.  This structure contains all the static information 
// necessary to execute the UnPackRows operation.
// 
class ComTdbUnPackRows : public ComTdb
{
  // The Task Control Block for the UnPackRows operator.  This struture
  // contains the run-time information necessary to execute the UnPackRows
  // operator.
  //
  friend class ExUnPackRowsTcb;
  friend class ExUnPackRowwiseRowsTcb;

  // The private state for the UnPackRows operator.  This structure contains
  // the information associated with a given request of the UnPackRows TCB.
  //
  friend class ExUnPackRowsPrivateState;

    enum FlagValues 
    { 
      ROWSET_ITERATOR         = 0x0001,
      ROWWISE_ROWSET          = 0x0002
    };
  
public:

  // Default Constructor.
  // Used when unpacking the UnPackRows TDB.  Used to get a pointer
  // to the Virtual Method Table.
  //
  ComTdbUnPackRows();
  
  // Construct a copy of the given node.
  // (This constructor does not seem to be used)
  // 
  ComTdbUnPackRows(const ComTdbUnPackRows *UnPackRowsTdb);
        
  // Construct a new UnPackRows TDB.
  // This constructor is call by the generator (PhysUnPackRows::codeGen() in
  // GenRelPackedRows.cpp.) 
  //
  // Parameters
  //
  // ComTdb *childTdb
  //  IN: The child of this UnPackRows TDB.
  //
  // ex_expr *packingFactor
  //  IN: A move expression used to extract the packing factor from one of
  //      the packed columns.
  //
  // ex_expr *unPackColsExpr
  //  IN: A move expression which unpacks one value from each packed column.
  //
  // long unPackColsTupleLen
  //  IN: The length of the tuple which will hold the unpacked values.  This
  //      tuple will be allocated by the UnPackRows node.
  //
  // unsigned short unPackColsAtpIndex
  //  IN: The index of the UnPackRows tuple in the work and returned ATP.
  //
  // unsigned short indexValueAtpIndex
  //  IN: The index of the indexValue in the work ATP.
  //
  // ex_cri_desc *criDescDown
  //  IN: The Cri Descriptor given to this node by its parent.
  //
  // ex_cri_desc *criDescUp
  //  IN: The Cri Descriptor returned to the parent node.
  //
  // ex_cri_desc *workCriDesc
  //  IN: The Cri Descriptor for the work Atp.
  //
  // queue_index queueSizeDown
  //  IN: Recommended queue size for the down queue used to communicate 
  //      with the parent.
  //
  // queue_index queueSizeUp
  //  IN: Recommended queue size for the up queue used to communicate
  //      with the parent.
  //
  // Cardinality estimatedRowCount
  //  IN: compiler estimate on number of returned rows
  //
  // NABoolean rowsetIterator
  //  IN: Used to set flags_ if parent flow node will be setting RowNumber.
  //
  // NABoolean tolerateNonFatalError
  //  IN: Used to set flags_ if nonfatal errors are tolerated.
  //

  ComTdbUnPackRows(ComTdb *childTdb,
		   ex_expr *packingFactor,
		   ex_expr *unPackColsExpr,
		   Lng32 unPackColsTupleLen,
		   unsigned short unPackColsAtpIndex,
		   unsigned short indexValueAtpIndex,
		   ex_cri_desc *criDescDown,
		   ex_cri_desc *criDescUp,
		   ex_cri_desc *workCriDesc,
		   queue_index queueSizeDown,
		   queue_index queueSizeUp,
		   Cardinality estimatedRowCount,
		   NABoolean rowsetIterator,
		   NABoolean tolerateNonFatalError);
  
  ComTdbUnPackRows(ComTdb *childTdb,
		   ex_expr *inputSizeExpr,
		   ex_expr *maxInputRowlenExpr,
		   ex_expr *rwrsBufferAddrExpr,
		   unsigned short rwrsAtpIndex,
		   ex_cri_desc *criDescDown,
		   ex_cri_desc *criDescUp,
		   ex_cri_desc *workCriDesc,
		   queue_index queueSizeDown,
		   queue_index queueSizeUp,
		   Cardinality estimatedRowCount,
		   Lng32 num_buffers,
		   ULng32 buffer_size);


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
                                { return (short)sizeof(ComTdbUnPackRows); }

  // ComTdbUnPackRows::pack() ---------------------------------------------
  // Pack the UnPackRows TDB for transmission from the compiler to the
  // executor, or from an ESP to DP2, etc.  This method needs to convert
  // all pointers to offsets, so that the memory containing this TDB fragment,
  // can be relocated and 'unpacked' (a different meaning of unpack) at a
  // new base address.
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

  // ComTdbUnPackRows::unpack() ---------------------------------------------
  // Unpack (a different meaning of unpack) the UnPackRows TDB after
  // transmission from the compiler to the executor, or from an ESP to DP2,
  // etc.  This method needs to convert all offsets to pointers, so that all
  // pointers now reflected the new location of the TDB fragment.
  //
  // Parameters
  //
  // long base
  //  IN - The base address of the TDB fragment.  Pointers are calculated
  //       by adding the offset to the base address (more or less).
  //
  Lng32 unpack(void *, void * reallocator);

  // methods used for rowwise rowset processing
  ex_expr * rwrsInputSizeExpr()      { return packingFactor_; }
  ex_expr * rwrsMaxInputRowlenExpr() { return unPackColsExpr_;}
  ex_expr * rwrsBufferAddrExpr()     { return rwrsBufferAddrExpr_;}
  UInt16    rwrsWorkIndex()          { return unPackColsAtpIndex_;}

  // ComTdbUnPackRows::Display() -----------------------------------------
  // (Don't know why this is here.  It does not seem to be virtual and
  // on class seems to do anything for this method.)
  //
  void display() const;

  // Return a pointer to the child TBD of this UnPackRows TDB.
  //
  inline ComTdb * getChildTdb() { return childTdb_; }

  // We are observing order queue protocol. Results from
  // a request are returned in full, before any of the results
  // of the next request are returned.
  //
  // Exclude this code from coverage analysis.
  // This code could be deleted since it is the same as the base implementation.
  // LCOV_EXCL_START
  Int32 orderedQueueProtocol() const { return -1; }
  // LCOV_EXCL_STOP

  // return a pointer to the specifed (by position) child TDB.
  // UnPackRows has only one child.
  //
  virtual const ComTdb *getChild(Int32 pos) const
  {
    if(pos == 0) 
      return childTdb_;
    return NULL;
  }

  // Return the number of children for this node.
  // UnPackRows has one child.
  //
  virtual Int32 numChildren() const { return (childTdb_ ? 1 : 0); }

  // Return the number of expression this node has.
  //
  virtual Int32 numExpressions() const 
  { 
    if (rowwiseRowset()) 
      return 3; 
    else 
      return 2; 
  }

  // Return the expression by position.
  // The UnPackRows expressions come first, followed
  // by the selection pred.
  //
  virtual ex_expr * getExpressionNode(Int32 pos) 
  {
    switch(pos) {
    case 0:
      if (rowwiseRowset())
	return rwrsInputSizeExpr();
      else
	return(packingFactor_);
    case 1:
      if (rowwiseRowset())
	return rwrsMaxInputRowlenExpr();
      else
	return(unPackColsExpr_);
    case 2:
      if (rowwiseRowset())
	return (rwrsBufferAddrExpr_);
      else
	return NULL;
    default:
      return(NULL);
    }
  }

  // Return the name of an expression by position.
  // The UnPackRows expressions come first, followed
  // by the selection pred.
  //
  virtual const char * getExpressionName(Int32 pos) const
  {
    switch(pos) {
    case 0:
      if (rowwiseRowset())
	return ("inputSizeExpr");
      else
	return("packingFactor");
    case 1:
      if (rowwiseRowset())
	return("maxInputRowlenExpr");
      else
	return("unPackColsExpr");
    case 2:
      if (rowwiseRowset())
	return ("rwrsBufferAddrExpr");
      else
	return (NULL);
    default:
      return(NULL);
    }
  }

  virtual const char *getNodeName() const
  {
    return "EX_UNPACK_ROWS";
  }

  short isRowsetIterator()
  { return short(flags_ & ROWSET_ITERATOR);}
 
  NABoolean rowwiseRowset()
  { return (flags_ & ROWWISE_ROWSET) != 0;}

  NABoolean rowwiseRowset() const
  { return (flags_ & ROWWISE_ROWSET) != 0;}

  void setRowwiseRowset(NABoolean v)
  {(v ? flags_ |= ROWWISE_ROWSET : flags_ &= ~ROWWISE_ROWSET); };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

protected:

  // The child of this UnPackRows TDB.
  //
  ComTdbPtr childTdb_;                      // 00-07

  // The Cri Descriptor for the work Atp.
  //
  ExCriDescPtr workCriDesc_;                // 08-15

  // A move expression used to extract the value of the packing factor from
  // one of the packed columns.
  //
  // If rowwise rowset, this is the number of rows in the rowset.
  ExExprPtr packingFactor_;                 // 16-23

  // A move expression used to move (unpack) one packed value from all the
  // packed columns.  Which value to move is indicated by the indexValue
  // tuple of the workAtp.  This indexValue tuple references a local variable
  // for its data.
  //  
  // If rowwiwse rowset, this is the maxlen of each row in the rowset.
  ExExprPtr unPackColsExpr_;                // 24-31

  // The length of the tuple which will hold the unpacked values.  This
  // tuple will be allocated by the UnPackRows node.
  //
  Int32 unPackColsTupleLen_;                // 32-35

  // The index of the UnPackRows tupp in the ATP.
  //
  UInt16 unPackColsAtpIndex_;               // 36-37

  // The index of the indexValue tupp in the work ATP.
  //
  UInt16 indexValueAtpIndex_;               // 38-39

  // used to indicate if 
  //(a) parent flow node will setting RowNumber and unpack has to send replies suitably
  //(b) nonfatal errors are tolerated for rowset inserts
  UInt16 flags_;                            // 40-41

  char filler[6];                           // 42-47

  ExExprPtr rwrsBufferAddrExpr_;            // 48-55

  char fillersComTdbUnPackRows_[24];        // 56-79

};

#endif

