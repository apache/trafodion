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
* File:         ComTdbOnlj.h
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

#ifndef COM_ONLJ_H
#define COM_ONLJ_H

// An onlj is a nested loop join that follows the ordered queue protocol
#include "ComTdb.h"


//
// Task Definition Block
//
class ComTdbOnlj : public ComTdb
{
  friend class ExOnljTcb;
  friend class ExOnljPrivateState;
  
  enum join_flags { SEMI_JOIN = 0x0001,
                    LEFT_JOIN = 0x0002,
                    ANTI_JOIN = 0x0004,
		    ROWSET_ITERATOR = 0x0008,
		    VSBB_INSERT = 0x0010,
		    INDEX_JOIN = 0x0020,
                    UNDO_JOIN = 0x0040,
                    SET_NONFATAL_ERROR = 0x0080,
                    DRIVING_MV_LOGGING = 0x0100};
                    

protected:  

  ComTdbPtr tdbLeft_;                     // 00-07
  ComTdbPtr tdbRight_;                    // 08-15
  ExCriDescPtr workCriDesc_;              // 16-23
  ExExprPtr preJoinPred_;                 // 24-31
  ExExprPtr postJoinPred_;                // 32-39
  ExExprPtr ljExpr_;                      // 40-47
  ExExprPtr niExpr_;                      // 48-55
  Int32 ljRecLen_;                        // 56-59
  UInt16 instantiatedRowAtpIndex_;        // 60-61
  UInt16 flags_;                          // 62-63
  Int32 rowsetRowCountArraySize_;         // 64-67
  char fillersComTdbOnlj_[36];            // 68-103

   
  inline Int32   isSemiJoin() const;  // True if we are doing a semi-join
  inline Int32   isAntiJoin() const;  // True if we are doing a anti-join
  inline Int32   isLeftJoin() const;  // True if we are doing a left-join
  inline Int32   isUndoJoin() const;  // True if we are using this to drive an undo
                                    //tree
  inline Int32   isSetNFErrorJoin() const;  // True if we are using this to set the NF row indexes
                                    //tree
  inline Int32   isRowsetIterator() const; // True if we are using this onlj to flow entries in a rowset
  inline Int32   isIndexJoin() const; // True if this onlj is an index join
  inline NABoolean vsbbInsertOn() const;
  inline NABoolean isDrivingMVLogging() const; //True if this onlj is used to drive mv logging
  // returns positive value only if this onlj is being used for rowset update and deletes
  // and rowset_row_count feature is enabled.
  inline Int32 getRowsetRowCountArraySize() const; 
  
 
public:
  // Constructor
  ComTdbOnlj();
  
  ComTdbOnlj(ComTdb * left_tdb,
	     ComTdb * right_tdb,
	     ex_cri_desc * given_cri_desc,
	     ex_cri_desc * returned_cri_desc,
	     queue_index down,
	     queue_index up,
	     Cardinality estimatedRowCount,
	     Lng32 num_buffers,
	     ULng32 buffer_size,
	     ex_expr    *before_pred,
	     ex_expr    *after_pred,
	     ex_expr    *lj_expr,
	     ex_expr    *ni_expr,
	     ex_cri_desc * work_cri_desc,
	     const unsigned short instantiated_row_atp_index,
	     Lng32 reclen,
	     Int32 semi_join,
	     Int32 anti_semi_join,
	     Int32 left_join,
	     Int32 undo_join,
	     Int32 setNFError,
	     Int32 rowset_iterator,
	     Int32 index_join,
	     NABoolean vsbbInsert,
	     Int32 rowsetRowCountArraySize,
	     NABoolean tolerateNonFatalError,
             NABoolean drivingMVLogging);
    

  virtual ~ComTdbOnlj();

  Int32       orderedQueueProtocol() const;

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

  virtual short getClassSize()       { return (short)sizeof(ComTdbOnlj); }

  Long      pack(void *);
  Lng32      unpack(void *, void * reallocator);

  void      display() const;
  
  inline ComTdb * getLeftTdb();
  inline ComTdb * getRightTdb();
  inline void setLeftTdb(ComTdb *);
  inline void setRightTdb(ComTdb *);

  virtual const ComTdb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const { return 2; }
  virtual const char *getNodeName() const;
  virtual Int32 numExpressions() const { return 4; }
  virtual ex_expr* getExpressionNode(Int32 pos);
  virtual const char * getExpressionName(Int32 pos) const;

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  virtual void displayContents(Space *space,ULng32 flag);
};


inline ComTdb * ComTdbOnlj::getLeftTdb() { return tdbLeft_;  };
inline ComTdb * ComTdbOnlj::getRightTdb() { return tdbRight_; };
inline void ComTdbOnlj::setLeftTdb(ComTdb *left) { tdbLeft_ = left; };
inline void ComTdbOnlj::setRightTdb(ComTdb *right) { tdbRight_ = right; };


inline Int32    ComTdbOnlj::orderedQueueProtocol() const
{
  return -1; // return true
};

// tdb inline procedures
// to determine if it is a semi_join or a left join check the flags_

inline Int32  ComTdbOnlj::isSemiJoin() const
{
  return (flags_ & SEMI_JOIN);
};

inline Int32  ComTdbOnlj::isAntiJoin() const
{
  return (flags_ & ANTI_JOIN);
};

inline Int32  ComTdbOnlj::isLeftJoin() const
{
  return (flags_ & LEFT_JOIN);
};

inline Int32  ComTdbOnlj::isUndoJoin() const
{
  return (flags_ & UNDO_JOIN);
};
inline Int32  ComTdbOnlj::isSetNFErrorJoin() const
{
  return (flags_ & SET_NONFATAL_ERROR);
};
inline Int32  ComTdbOnlj::isRowsetIterator() const
{
  return (flags_ & ROWSET_ITERATOR);
};

inline Int32  ComTdbOnlj::isIndexJoin() const
{
  return (flags_ & INDEX_JOIN);
};


inline NABoolean ComTdbOnlj::vsbbInsertOn() const
{
  return (flags_ & VSBB_INSERT);
}
inline NABoolean ComTdbOnlj::isDrivingMVLogging() const
{
  return (flags_ & DRIVING_MV_LOGGING);
}

inline Int32 ComTdbOnlj::getRowsetRowCountArraySize() const
{
  return rowsetRowCountArraySize_;
}

  
// end of inline procedures

#endif // EX_ONLJ_H
