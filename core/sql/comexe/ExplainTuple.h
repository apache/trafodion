/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
****************************************************************************
*
* File:         ExplainTuple.h
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

#ifndef EXPLAINTUPLE_H
#define EXPLAINTUPLE_H


#include "Int64.h"
#include "Collections.h"
#include "NAVersionedObject.h"

const Int32 MAX_REL_ARITYS = 2;

class ExplainTuple;
class ExplainDesc;

// Structure used to hold column information for the Virtual
// Explain Table.  An array of these will be constructed by the
// ExplainDesc constructor.  This array will be pointed to by 
// the root ExplainDesc node of the explain Tree.

struct ExplainTupleColDesc
{
  // type should correspond to DataType in desc.h
  Int32 datatype;                                    // 00-03
  Int32 length;                                      // 04-07
  Int32 offset;                                      // 08-11
  Int16 nullflag;                                    // 12-13
  Int16 fillersExplainTupleColDesc;                  // 14-15
};

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ExplainTuple
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<ExplainTuple> ExplainTuplePtr;

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ExplainDesc
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<ExplainDesc> ExplainDescPtr;

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ExplainTupleColDescPtr
// ---------------------------------------------------------------------
typedef NABasicPtrTempl<ExplainTupleColDesc> ExplainTupleColDescPtr;


// ExplainDesc:  This class implements the root node of the explain
// tree.  The node contains:
//  - a pointer to the rest of the explain tree 
//    (made up of ExplainTuple nodes)
//  - a pointer to an ExplainTupleColDesc[].  This is a descriptor
//    of the explainTuple.
//
class ExplainDesc : public NAVersionedObject
{
public:

  ExplainDesc() : NAVersionedObject(-1)
  {}

  // The constructor, Allocates the ExplainTupleColDesc array.
  ExplainDesc(Lng32 numCols, Lng32 recLength, Space *space);

  // The destuctor (none currently defined.  This node should be
  // allocated using a Space object, so will be reclaimed in bulk)
  ~ExplainDesc() {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(ExplainDesc); }

  // Pack and Unpack the ExplainDesc node and its children
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // set and get a pointer to the rest of the explain tree.
  inline void setExplainTreeRoot(ExplainTuple *rootExplainTuple);
  inline ExplainTuple *getExplainTreeRoot();

  // set the column descriptions
  inline void setColDescr(Int32 col,
			  Lng32 datatype,
			  Lng32 length,
			  Lng32 offset,
			  Lng32 nullflag);

  // Get values of the various fields of the class
  inline Int32 getRecLength();
  inline Int32 getNumCols();
  inline Lng32 getColLength(Int32 col);
  inline Lng32 getColOffset(Int32 col);
  inline short getColNullFlag(Int32 col);
  inline Lng32 getColDataType(Int32 col);

private:
  // The version of the explain tree.  Might be useful to handle
  // versioning issues.
  // int version_;
  // (Handled in NAVersionedObject header now.)

  // Number of columns in the explainTuple.
  Int32 numCols_;                                       // 00-03

  // Number of bytes in the explainTuple.
  Int32 recLength_;                                     // 04-07
  
  // A pointer to an ExplainTupleColDesc array.
  ExplainTupleColDescPtr explainCols_;                  // 08-15

  // Pointer to the rest of the explain tree.
  ExplainTuplePtr explainTreeRoot_;                     // 16-23

  char fillersExplainDesc_[40];                         // 24-63
};
  

inline void
ExplainDesc::setExplainTreeRoot(ExplainTuple *rootExplainTuple)
{
  explainTreeRoot_ = rootExplainTuple;
};

inline ExplainTuple *
ExplainDesc::getExplainTreeRoot()
{
  return explainTreeRoot_;
};

inline void ExplainDesc::setColDescr(Int32 col,
				     Lng32 datatype,
				     Lng32 length,
				     Lng32 offset,
				     Lng32 nullflag)
{
  explainCols_[col].datatype = datatype;
  explainCols_[col].length   = length;
  explainCols_[col].offset   = offset;
  explainCols_[col].nullflag = (Int16) nullflag;
}

inline Int32
ExplainDesc::getRecLength()
{
  return recLength_;
};

inline Int32
ExplainDesc::getNumCols()
{
  return numCols_;
};

inline Lng32
ExplainDesc::getColLength(Int32 col)
{
  return explainCols_[col].length;
};

inline Lng32
ExplainDesc::getColOffset(Int32 col)
{
  return explainCols_[col].offset;
};

inline short
ExplainDesc::getColNullFlag(Int32 col)
{
  return explainCols_[col].nullflag;
};

inline Lng32
ExplainDesc::getColDataType(Int32 col)
{
  return explainCols_[col].datatype;
};


// ExplainTuple - A binary node of the explain tree.  Besides the root
// node (ExplainDesc), the explain tree is made up of these
// exclusively.  The explain tree is generated a generate time in a
// separate fragment by the methods addExplainInfo().  This class
// implements some methods to set certain fields of the explainTuple.
// Only the ones that can be called from within the executor are
// implemented here.  The others are implemented in the class
// ExplainTupleMaster which is derived from this class and only contains
// methods.

class ExplainTuple : public NAVersionedObject
{
public:

  // The enumeration of the columns of the Explain Tuple.
  // The order (and values of the tokens) of this enumeration
  // is important.
  enum ExplainTupleCols {
    EX_COL_SYSKEY,
    EX_COL_MODNAME,
    EX_COL_STMTNAME,
    EX_COL_PLANID,
    EX_COL_SEQNUM,
    EX_COL_OPER,
    EX_COL_LEFTSEQNUM,
    EX_COL_RIGHTSEQNUM,
    EX_COL_TABLENAME,
    EX_COL_CARD,
    EX_COL_OPCOST,
    EX_COL_TOTCOST,
    EX_COL_DETCOST,
    EX_COL_DESCRIPT
  };

  // The states of the explain Tuple.  If no info has been
  // placed into the tuple, it should not be processed.
  enum ExplainTupleState {
    NO_EXPLAIN_INFO,
    SOME_EXPLAIN_INFO
  };

 ExplainTuple() : NAVersionedObject(-1),
    explainTupleStr_(NULL)
  {}

  // Contructor - called by RelExpr::addExplainInfo()
  ExplainTuple(ExplainTuple *leftChild,
               ExplainTuple *rightChild,
               ExplainDesc *explainDesc);

  ~ExplainTuple();

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()     { return (short)sizeof(ExplainTuple); }

  // Pack and Unpack this node and all its children
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  short genExplainTupleData(Space * space);

  // Copy the data pointed to by value to the specified col of the
  // explain tuple.
  void setCol(Int32 col,
	      void const *value,
	      UInt32 length, 
	      UInt32 cursor);

  // Initialize specific columns of the explain Tuple.
  // These routines call setCol().
  // These are the columns that need to be set at run time.
  // The methods to set the other columns are implemented in
  // the class ExplainTupleMaster.
  void setModuleName(const char *modName);
  void setStatementName(const char *stmtName);
  void setSeqNum(Int32  seqNum);
  Int32 getSeqNum();
  void setChildSeqNum(Int32 child, Int32 seqNum);

  // The state of this explain tuple.
  inline ExplainTupleState getState() const;

  // get/set the pointer to the parent of this node.
  // The child of the ExplainDesc node will have a NULL parent
  // pointer.
  inline void setParent(ExplainTuple *parent);
  inline ExplainTuple * getParent();

  // The number of children this node can have. (always returns 2)
  inline Int32 numChildren() const;

  // A reference to a child.
  inline ExplainTuple *&child(Int32 index);

  // A pointer to the explainTuple data;
  inline char *getExplainTuple();

  // A pointer to the root node (ExplainDesc) of the explain tree.
  // This node contains the description of the explain tuple.
  inline ExplainDesc *getExplainDesc();

  // Get info about the explain tuple.  These methods access the
  // ExplainDesc node at the root of the tree.
  inline Int32 getRecLength();
  inline Int32 getNumCols();
  inline Lng32 getColLength(Int32 col);
  inline Lng32 getColOffset(Int32 col);
  inline short getColNullFlag(Int32 col);
  inline Lng32 getColDataType(Int32 col);
  
  Int32 getUsedRecLength() { return usedRecLength_; }
  void setUsedRecLength(Int32 v) { usedRecLength_ = v;}

protected:
  // Used for debugging and self checks.
  char eyeCatcher_[4];                                      // 00-03

  // State of this explain Tuple. (ExplainTupleState)
  Int16 state_;                                             // 04-05
  Int16 fillersExplainTuple1_;                              // 06-07

  // A cursor for the description field of the explain Tuple.
  // The description field is filled in in peices.  The cursor
  // is used to keep track of the current position.
  UInt32 descCursor_;                                       // 08-11
  Int32 fillersExplainTuple2_;                              // 12-15

  // A pointer to the data.
  NABasicPtr explainTupleData_;                                 // 16-23

  // A pointer back to the root of the explain tree.
  ExplainDescPtr explainDesc_;                              // 24-31

  // Pointer to the parent node.
  ExplainTuplePtr parent_;                                  // 32-39

  // Pointers to the children nodes.
  ExplainTuplePtr children_[MAX_REL_ARITYS];                // 40-55

  // this field is initially allocated from heap for max length of explain tuple.
  // At end of explain, only the used up bytes are moved to explainTupleData
  // to be stored in explain fragment.
  // This is done to reduce the amount of data in explain fragment since that
  // could be stored in trafodion repository.
  char * explainTupleStr_;

  // Number of actual bytes of data in explainTuple.
  Int32 usedRecLength_;                             

  char fillersExplainTuple_[36];                            // 56-95
};

// Inline routines for ExplainTuple.

inline ExplainTuple::ExplainTupleState
ExplainTuple::getState() const
{
  return ExplainTupleState(state_);
};

inline Int32
ExplainTuple::numChildren() const
{
  return MAX_REL_ARITYS;
};

inline ExplainTuple *& 
ExplainTuple::child(Int32 index)
{
  return children_[index].pointer();
};

inline void
ExplainTuple::setParent(ExplainTuple *parent)
{
  parent_ = parent;
};

inline ExplainTuple *
ExplainTuple::getParent()
{
  return parent_;
};

inline char *
ExplainTuple::getExplainTuple()
{
  return explainTupleData_;
};

inline ExplainDesc *
ExplainTuple::getExplainDesc()
{
  return explainDesc_;
};

inline Int32
ExplainTuple::getRecLength()
{
  return explainDesc_->getRecLength();
};

inline Int32
ExplainTuple::getNumCols()
{
  return explainDesc_->getNumCols();
};

inline Lng32
ExplainTuple::getColLength(Int32 col)
{
  return explainDesc_->getColLength(col);
};

inline Lng32
ExplainTuple::getColOffset(Int32 col)
{
  return explainDesc_->getColOffset(col);
};

inline short
ExplainTuple::getColNullFlag(Int32 col)
{
  return explainDesc_->getColNullFlag(col);
};

inline Lng32
ExplainTuple::getColDataType(Int32 col)
{
  return explainDesc_->getColDataType(col);
};

#endif



