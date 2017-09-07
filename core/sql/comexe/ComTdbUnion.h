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
* File:         ComTdbUnion.h
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

#ifndef COM_UNION_H
#define COM_UNION_H

#include "ComTdb.h"

// -----------------------------------------------------------------------
// Task Definition Block
// -----------------------------------------------------------------------
class ComTdbUnion : public ComTdb
{
  friend class ex_union_tcb;
  friend class ex_union_private_state;
  
public:

  // Constructor
  ComTdbUnion();
  
  ComTdbUnion(const ComTdbUnion *union_tdb);
	       
  ComTdbUnion(ComTdb * left_tdb,
	      ComTdb * right_tdb,
	      ex_expr * left_expr,
	      ex_expr * right_expr,
	      ex_expr * merge_expr,
	      ex_expr * cond_expr,
              ex_expr * trig_expr,
              Lng32 union_reclen,
	      const unsigned short tupp_index,
	      ex_cri_desc * given_cri_desc,
	      ex_cri_desc * returned_cri_desc,
	      queue_index down,
	      queue_index up,
	      Cardinality estimatedRowCount,
	      Lng32 num_buffers,
	      ULng32 buffer_size,
	      NABoolean ordered_union,
              Int32 blocked_union, // ++ Triggers -, add blocked_union
              Int32 hasNoOutputs,  // ++ Triggers -, add noOutputs
              NABoolean rowsFromLeft,
              NABoolean rowsFromRight,
              NABoolean AfterUpdate,
	      NABoolean inNotAtomicStmt);

  virtual ~ComTdbUnion();

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

  virtual short getClassSize()      { return (short)sizeof(ComTdbUnion); }
  
  Long      pack(void *);
  Lng32      unpack(void *, void * reallocator);
  
  void      display() const;
  
  inline ComTdb * getLeftTdb()                        { return tdbLeft_; }
  inline ComTdb * getRightTdb()                      { return tdbRight_; }
  inline void setLeftTdb(ComTdb *left)                { tdbLeft_ = left; }
  inline void setRightTdb(ComTdb *right)            { tdbRight_ = right; }

// LCOV_EXCL_START
// exclude from code coverage since this code is obsolete
  Int32 orderedQueueProtocol() const  { return -1; }
// LCOV_EXCL_STOP

  virtual const ComTdb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const;
  virtual const char *getNodeName() const { return "EX_UNION"; };
  virtual Int32 numExpressions() const                         { return 5; }
  virtual ex_expr* getExpressionNode(Int32 pos);
  virtual const char * getExpressionName(Int32 pos) const;

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  virtual void displayContents(Space *space,ULng32 flag);

  inline NABoolean expectingLeftRows() const
                { return (csErrFlags_ & ROWS_FROM_LEFT) != 0; }

  inline NABoolean expectingRightRows() const
                { return (csErrFlags_ & ROWS_FROM_RIGHT) != 0; }

  inline NABoolean afterUpdate() const
                { return (csErrFlags_ & AFTER_UPDATE) != 0; }
  inline NABoolean inNotAtomicStmt() const       { return (flags_ & IN_NOT_ATOMIC_STMT) != 0; }


protected:

  // ++ Triggers -, add BLOCKED_UNION and NO_OUTPUTS
  enum union_flags { UNION_ALL = 0x0001, 
					 ORDERED_UNION = 0x0002, 
                     MERGE_UNION = 0x0004, 
					 CONDITIONAL_UNION = 0x0008 , 
					 BLOCKED_UNION = 0x0010,
					// NO_OUTPUTS can be applied to ORDER_UNION or BLOCKED_UNION 
					// to mark that it does not have outputs,
					// it is used only to control action sequence
					 NO_OUTPUTS = 0x0100,
					 // IN_NOT_ATOMIC_STMT can be applied to unary unions
					 // to mark that this union is oerating within a not atomic 
					 // statement to control enable/disable of an after trigger.
					 IN_NOT_ATOMIC_STMT = 0x0200
					};

  enum {NOT_CONDITIONAL_UNION = 0x0000, ROWS_FROM_LEFT = 0x0001,  ROWS_FROM_RIGHT = 0x0002,
        AFTER_UPDATE = 0x0004 };

  ComTdbPtr tdbLeft_;                 // 00-07
  ComTdbPtr tdbRight_;                // 08-15

  ExExprPtr leftExpr_;                // 16-23
  ExExprPtr rightExpr_;               // 24-31
  ExExprPtr mergeExpr_;               // 32-39
  ExExprPtr condExpr_;                // 40-47

  Int32 unionReclen_;                 //  48-51 

  Int16 flags_;                       // 52-53 

  // index into atp of new union tupp
  UInt16 tuppIndex_;                  // 54-55 

  UInt16 csErrFlags_;                  // 56-57 

  char fillers2ComTdbUnion_[6];       // 58-63 -- added since next attr has
                                      // to start on an 8 byte boundary
                                      // should be reused when 2 or 4 bytes
                                      // class attr is added.

  ExExprPtr trigExceptExpr_;          // 64-71

  char fillersComTdbUnion_[24];       // 72-95

  inline Int32 isOrderedUnion() const   { return (flags_ & ORDERED_UNION); }
  inline Int32 isBlockedUnion() const   { return (flags_ & BLOCKED_UNION); } // ++ Triggers -
  inline Int32 isMergeUnion() const       { return (flags_ & MERGE_UNION); }
  inline Int32 isConditionalUnion() const
                                  { return (flags_ & CONDITIONAL_UNION); }
  inline Int32 hasNoOutputs() const
                                  { return (flags_ & NO_OUTPUTS); }

};

// LCOV_EXCL_START
// exclude from code coverage analysis since this is used only by GUI
inline const ComTdb* ComTdbUnion::getChild(Int32 pos) const
{
   if (pos == 0)
      return tdbLeft_;
   else if (pos == 1)
      return tdbRight_;
   else
      return NULL;
}
// LCOV_EXCL_STOP


#endif

