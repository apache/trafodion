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
* File:         ExplainTuple.cpp
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

#include "ExplainTupleMaster.h"
#include "ComPackDefs.h"
#include "dfs2rec.h"
#include "exp_tuple_desc.h"
#include "csconvert.h"

// Methods that implement the Explain Tree (ExplainDesc
// and ExplainTuple nodes).  An Explain Tree consists of an
// ExplainDesc root which points to a single child ExplainTuple.
// Each ExplainTuple can in turn point to upto two children
// ExplainTuple's.  

// The first version of Explain.  This is a simple attempt to
// have some handle on the version of explain used to create
// an Explain Tree that might be stored in a module.  This version
// number should be changed whenever the data structures of the 
// Explain Tree change

// #define EXPLAIN_VERSION 100

// ExplainDesc constructor:  
//  - Initializes version_ and explainTreeRoot_.
//
//  - Initializes recLength_ and numCols_ based on the input
//    explainDesc.
//
//  - constructs an array of ExplainTupleColDesc to describe the
//    layout of the explain tuple.  Uses the input explainDesc
//    to get info. (This array will be stored with the explain Tree
//    and should be used for all accesses to an explain tuple.
//
//  Called from RelRoot::codeGen()


ExplainDesc::ExplainDesc(Lng32 numCols, Lng32 recLength, Space *space)
: // version_(EXPLAIN_VERSION), // handled now by NAVersionedObject
  explainTreeRoot_(0), numCols_(numCols), recLength_(recLength),
  NAVersionedObject(-1)
{
  // Allocate space for the array of explainTupleColDesc.  This array
  // will have numCols_ entries and will be used to access the Explain Tuple.
  explainCols_ = (ExplainTupleColDesc *)
                   new(space) char[sizeof(ExplainTupleColDesc) * numCols_];

  // Method setColDescr must be called for each or the numCols_
  // allocated column descriptors to set them!!!
}

// Pack the Explain Descriptor and all its children
Long ExplainDesc::pack(void *space)
{
  // REVISIT
  // explainCols_.packArray(space,numCols_);
  explainCols_.pack(space);
  explainTreeRoot_.pack(space);
  return NAVersionedObject::pack(space);
}

// Unpack the Explain Descriptor and all its children
Lng32 ExplainDesc::unpack(void * base, void * reallocator)
{
  if(explainCols_.unpack(base)) return -1;
  if(explainTreeRoot_.unpack(base, reallocator)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}


// ExplainTuple Constructor:
//  - initializes some data members
//
//  - sets explainDesc_ to point to root of tree (from input)
//
//  - sets up eye catcher
//
//  - sets up child pointers (from inputs)
//
//  - sets up childrens' parent pointers to point to this node.
//
//  Called by RelExpr::addExplainInfo()
//
// Note that ExplainTuple::init() actually allocates the explainTuple_.
// init() could/should probably be pulled into the costructor.

ExplainTuple::ExplainTuple(ExplainTuple *leftChild,
                           ExplainTuple *rightChild,
                           ExplainDesc *explainDesc)
: state_(NO_EXPLAIN_INFO),
  explainTupleData_(),
  descCursor_(0),
  explainDesc_(explainDesc),
  parent_(0),
  explainTupleStr_(NULL),
  NAVersionedObject(-1)
{
  // Eye catcher.  Helps with debugging.
  eyeCatcher_[0] = 'E';
  eyeCatcher_[1] = 'X';
  eyeCatcher_[2] = 'T';
  eyeCatcher_[3] = 'P';

  // Initialize the child pointers.
  child(0) = leftChild;
  child(1) = rightChild;

  // Make the Explain Tree linked both ways.
  if(leftChild)
    leftChild->setParent(this);
  
  if(rightChild)
    rightChild->setParent(this);

  usedRecLength_ = getRecLength();
}

ExplainTuple::~ExplainTuple()
{
  if (explainTupleStr_)
    delete explainTupleStr_;
}

short ExplainTuple::genExplainTupleData(Space * space)
{
  if (explainTupleStr_)
    {
      // find out used record size by removing unused bytes in description field.
      Lng32 descrColSize = getColLength(EX_COL_DESCRIPT);
      Lng32 usedRecLength = getRecLength() - (descrColSize - descCursor_);
      setUsedRecLength(usedRecLength);
      explainTupleData_ = space->allocateAndCopyToAlignedSpace(
                                                               explainTupleStr_, 
                                                               usedRecLength);

      delete explainTupleStr_;
      explainTupleStr_ = NULL;
    } 
  else
    {
      setUsedRecLength(getRecLength());
    }

  return 0;
}

// Pack this node and all it children
Long ExplainTuple::pack(void *space)
{
  explainTupleData_.pack(space);

  // Since the explainDesc is at the start of the Fragment,
  // the offset will end up being zero.  During unpack, a zero
  // offset would be mistaken for a NULL pointer.  To get around
  // this, subtract one from the offset.  Must also do something
  // similar when unpacking.

  explainDesc_++;
  explainDesc_.packShallow(space);

  // Convert the parent pointer, but don't follow.
  parent_.pack(space);

  // Follow the child pointers to traverse the whole tree.
  children_[0].pack(space);
  children_[1].pack(space);
  return NAVersionedObject::pack(space);
}


// unpack this node and all it children
Lng32 ExplainTuple::unpack(void * base, void * reallocator)
{
  // Unpack the children pointers and the children nodes.
  if(children_[0].unpack(base, reallocator)) return -1;
  if(children_[1].unpack(base, reallocator)) return -1;
  if(parent_.unpack(base, reallocator)) return -1;

  // Since the explainDesc is at the start of the Fragment,
  // the offset will be zero.  During unpack, a zero
  // offset would be mistaken for a NULL pointer.  To get around
  // this, subtract one from the offset after packing.  Then to
  // convert back to a pointer, must subtract 1 after converting.

  if(explainDesc_.unpackShallow(base)) return -1;
  explainDesc_--;

  if(explainTupleData_.unpack(base)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}

// init : 
//  - Allocates explainTuple_ for this node and initializes it
//    with some defaults.
//  
//  Called by RelExpr::addExplainInfo() right after the constructor
//  for this class.  This method should probably be pulled into the
//  constructor.  It was originally done differently.
// 
//  This is a method on ExplainTupleMaster since it should only be
//  called during the generate phase (not at run time)
//
Int32
ExplainTupleMaster::init(Space *space, NABoolean doExplainSpaceOpt)
{
  descCursor_ = 0;
  
  explainTupleStr_ = NULL;
  explainTupleData_ = (NABasicPtr)NULL;

  // if space opt is to be done, start by allocating max length.
  // this will be trimmed and reset in ExplainTuple::genExplainData at end.
  if (doExplainSpaceOpt)
    explainTupleStr_ = new char[getRecLength()];
  else
    explainTupleData_ = new (space) char[getRecLength()];
  
  setModuleName("");
  setStatementName("");
  setPlanId(0);
  setSeqNum(0);
  setOperator("Unknown");
  setChildSeqNum(0,0);
  setChildSeqNum(1,0);
  setTableName("");
  setCardinality(0.0);
  setOperatorCost(0.0);
  setTotalCost(0.0);
  setDetailCost(0);
  setDescription("");
  return(0);
}

// setCol - set the specified column of the explain Tuple with the
// value pointed to by 'value'.
//
// - col        - the column number to set
// - value      - a pointer to the value 
// - dataLength - the length of the data pointed to by value
//                (only meaningful for VarChar fields)
// - cursor     - the position within the field to start placing 'value'
//                (only meaningful for Char fields and only used
//                with the 'description' field.
void
ExplainTuple::setCol(Int32 col,
		     void const *value,
		     UInt32 dataLength, // Used only by VarChar fields
		     UInt32 cursor)
{
  char * e = NULL;
  if (explainTupleStr_)
    e = explainTupleStr_;
  else if (explainTupleData_)
    e = explainTupleData_;

  if (e)
    {
      // Get the relative position of this column in the explainTuple.
      // For varchars len is the declared varchar max length
      UInt32 len = (UInt32) getColLength(col);
      UInt32 offset = (UInt32) getColOffset(col);
      Int16 dataType = (Int16) getColDataType(col);

      char *v = (char *)value;
      char *p = NULL;

      UInt32 nullOffset = 0;
      UInt32 vcOffset = 0;
      NABoolean isNullable = (getColNullFlag(col) != 0);
      UInt32 colOffset = ExpTupleDesc::sqlarkExplodedOffsets(offset, len,
                                                             dataType,
                                                             isNullable,
							     &nullOffset,
                                                             &vcOffset);
      
      // If the column can have NULL values...
      if (isNullable)
	{
          p = e + nullOffset;
	  // and if the value is non-null or something has
	  // already been placed in this field.
	  if (v || (cursor > 0))
	    {
	      // Set the NULL indicator to non NULL
	      *p++ = (char)0;
	      *p++ = (char)0;
	    }
	  else
	    {
	      // set the null indicator to NULL
	      *p++ = (char)-1;
	      *p++ = (char)-1;
	    }	    
	}
      

      // If this column is a VarChar, the actual length field must be set.
      if (DFS2REC::isAnyVarChar(dataType))
	{
	  // Desired length is minimum of:
	  //   - declared size of this column (len)
	  //   - length of value (dataLength) + offset into this field (cursor)
	  UInt32 desiredLength = ((cursor + dataLength) < len) ? (cursor + dataLength) : len;


	  // Bytes to copy is minimum of:
	  //   - remaining space (len - cursor)
	  //   - dataLength
	 
	  UInt32 bytesToCopy = 0;

	  if (desiredLength < len)
	  {	      
	      bytesToCopy = dataLength;//if all fits, copy everything
	  }
	  else 
	  {
	    if (len > cursor)
	    {
	      bytesToCopy = len - cursor;//space exists, but need to truncate
	    }
	    else
	    {
	      bytesToCopy = 0;	      //no space available
	    }
	  }

	  if (bytesToCopy)
	    {
  	      // Set the length field.
              p = e + vcOffset;
	      Int16 actualLength = (Int16) desiredLength;
	      *p++ = ((char *)&actualLength)[0];
	      *p++ = ((char *)&actualLength)[1];
	    }
	  len = bytesToCopy;  // len is now number of bytes to copy.
	}

      // Destination pointer is now indexed into field by cursor.
      p = e + colOffset + cursor;

      // If field is Fixed Char. it must be pad extended to the declared size.
      // Otherwise, just copy the bytes.
      if (dataType == REC_BYTE_F_ASCII)
	for(UInt32 i = 0; i < len; i++)
	  {
  	    if(v && *v)
	      *p++ = *v++;
	    else
	      *p++ = ' ';
	  }
      else if (v)
	for (UInt32 i = 0; i < len; i++)
	  *p++ = *v++;

      //in case of VarChar, need to check for data truncation and indicate accordingly
      ///whenever len is zero => nothing was copied => no need to check
      if (len && DFS2REC::isAnyVarChar(dataType))
	{
	  //if we tried putting in more than there is space for, we must have truncated
	  if ((cursor + dataLength) > (UInt32)getColLength(col))
	  {
	    //replace the last two characters by the end of line character and the truncation indicator
	    //NOTE: Must be careful not to damage a multi-byte UTF8 character, so we ensure
	    //      that *(p-2) [i.e. where the '*' will be put] is either an ASCII character
	    //      or it is the first byte of a multi-byte UTF8 character.
	    //
	    if ( (*(p-2)) & 0x80 ) // If non-ASCII char
	    { 
	       char *pSt = findStartOfChar( p-2, e + colOffset + 2 );
	       // Note: We don't care if pSt -> valid UTF8 or not.
	       while ( p > (pSt + 2) ) *--p = '\0'; // replace char with zeroes
	    }

	    *--p = '\0';
	    *--p = '*';
	  }
	}
	
      // Update the state of this node.
      state_ = SOME_EXPLAIN_INFO;
    }
}

// Below are specific routines to set the specific
// columns of the explain tuple.  They are very dependent
// on the structure of the tuple.  In particular, they
// assume the ordering of the columns and the type of
// each column.

// setModuleName:
// a method on ExplainTuple - is called from executor.
void
ExplainTuple::setModuleName(const char *modName)
{
  setCol(EX_COL_MODNAME, modName, 0, 0);
}

// setStatementName:
// a method on ExplainTuple - is called from executor.
void
ExplainTuple::setStatementName(const char *stmtName)
{
  setCol(EX_COL_STMTNAME, stmtName, 0, 0);
}

void
ExplainTupleMaster::setPlanId(Int64 planId)
{
  setCol(EX_COL_PLANID, &planId, 0, 0);
}

void
ExplainTuple::setSeqNum(Int32 seqNum)
{
  setCol(EX_COL_SEQNUM, &seqNum, 0, 0);
}

Int32
ExplainTuple::getSeqNum()
{
  Int32 retval = 0;
  char * e = NULL;
  if (explainTupleStr_)
    e = explainTupleStr_;
  else if (explainTupleData_)
    e = explainTupleData_;
    
  if (e)
    {
      // Get the relative position of this column in the explainTuple.
      UInt32 len = (UInt32) getColLength(EX_COL_SEQNUM);
      UInt32 offset = (UInt32) getColOffset(EX_COL_SEQNUM);
      Int16 dataType = (Int16) getColDataType(EX_COL_SEQNUM);
      NABoolean isNullable = (getColNullFlag(EX_COL_SEQNUM) != 0);

      UInt32 nullOffset = 0;
      UInt32 colOffset = ExpTupleDesc::sqlarkExplodedOffsets(offset,
                           len, dataType, isNullable,
                           &nullOffset);

      if ((isNullable) &&
          (*((Int16 *) ((char *)e + nullOffset)) != 0))
        {
          // the NULL flag is set, return a -1 in this case
          return -1;
        }

      // Get pointer to seqnum and cast it to an Int32
      Int32 *r = ((Int32 *) ((char *)e + colOffset));
      retval = *r;
    }
  return retval;
}

void
ExplainTupleMaster::setOperator(const char *op)
{
  setCol(EX_COL_OPER, op, 0, 0);
}

void
ExplainTuple::setChildSeqNum(Int32 child, Int32 seqNum)
{
  
  Int32 col = (child == 0 ? EX_COL_LEFTSEQNUM : EX_COL_RIGHTSEQNUM);

  // A value of zero for seqNum indicates a NULL value,
  // so pass a NULL pointer.
  if(seqNum == 0)
    setCol(col, 0, 0, 0);
  else
    setCol(col, &seqNum, 0, 0);
}

void
ExplainTupleMaster::setTableName(const char *tabName)
{
  setCol(EX_COL_TABLENAME, tabName, 0, 0);
}

void
ExplainTupleMaster::setCardinality(double card)
{
  
  float fcard = (float) card; 
  
  setCol(EX_COL_CARD, &fcard, 0, 0);
}


void
ExplainTupleMaster::setOperatorCost(double opCost)
{
  float fopCost = (float) opCost;
  
  setCol(EX_COL_OPCOST, &fopCost, 0, 0);
}

void
ExplainTupleMaster::setTotalCost(double totCost)
{
  float ftotCost = (float) totCost; 
  
  setCol(EX_COL_TOTCOST, &ftotCost, 0, 0);
}


void
ExplainTupleMaster::setDetailCost(char *detail)
{
  if(detail)
    setCol(EX_COL_DETCOST, detail, str_len(detail), 0);
  else
    setCol(EX_COL_DETCOST, 0, 0, 0);
}

void
ExplainTupleMaster::setDescription(const char *desc)
{
  
  setCol(EX_COL_DESCRIPT, desc, str_len(desc), descCursor_);

  // Maintain the cursor for the description field.  This is the
  // only field that uses a cursor.

  descCursor_ += str_len(desc);
}


