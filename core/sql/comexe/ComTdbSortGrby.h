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
* File:         ComTdbSortGrby.h
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

#ifndef COM_SORT_GRBY_H
#define COM_SORT_GRBY_H

#include "ComTdb.h"

//
// Task Definition Block
//
class ComTdbSortGrby : public ComTdb
{
  friend class ex_sort_grby_tcb;
  friend class ex_sort_grby_private_state;
protected:

  ComTdbPtr tdbChild_;                    // 00-07
  ExExprPtr aggrExpr_;                    // 08-15
  ExExprPtr grbyExpr_;                    // 16-23
  ExExprPtr moveExpr_;                    // 24-31
  ExExprPtr havingExpr_;                  // 32-39
  
  // length of the aggregated row
  Int32 recLen_;                          // 40-43

  // index into atp of new sort_grby tupp
  UInt16 tuppIndex_;                      // 44-45

  UInt16 flags_;                          // 46-47

  char fillersComTdbSortGrby_[40];        // 48-87

public:
  // Constructor
NA_EIDPROC
  ComTdbSortGrby(); // dummy constructor. Used by 'unpack' routines.
  
NA_EIDPROC
  ComTdbSortGrby(ex_expr * aggr_expr,
		 ex_expr * grby_expr,
		 ex_expr * move_expr,
		 ex_expr * having_expr,
		 Lng32 reclen,
		 const unsigned short tupp_index,
		 ComTdb * child_tdb,
		 ex_cri_desc * given_cri_desc,
		 ex_cri_desc * returned_cri_desc,
		 queue_index down,
		 queue_index up,
		 Cardinality estimatedRowCount,
		 Lng32 num_buffers,
		 ULng32 buffer_size,
		 NABoolean tolerateNonFatalError);

NA_EIDPROC
  ~ComTdbSortGrby();

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
NA_EIDPROC
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

NA_EIDPROC
  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ComTdb::populateImageVersionIDArray();
  }



NA_EIDPROC
  virtual short getClassSize() { return (short)sizeof(ComTdbSortGrby); }  
  Long pack(void *);
NA_EIDPROC
  Lng32 unpack(void *, void * reallocator);
  
NA_EIDPROC
  void display() const;

NA_EIDPROC
  inline ComTdb * getChildTdb();

NA_EIDPROC
  Int32 orderedQueueProtocol() const;

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
NA_EIDPROC
  virtual void displayContents(Space *space,ULng32 flag);

NA_EIDPROC
  virtual const ComTdb* getChild(Int32 pos) const;
NA_EIDPROC
  virtual Int32 numChildren() const { return 1; }
NA_EIDPROC
  virtual const char *getNodeName() const { return "EX_SORT_GRBY"; };
NA_EIDPROC
  virtual Int32 numExpressions() const { return 4; }
NA_EIDPROC
  virtual ex_expr* getExpressionNode(Int32 pos) {
     if (pos == 0)
	return aggrExpr_;
     else if (pos == 1)
	return grbyExpr_;
     else if (pos == 2)
	return moveExpr_;
     else if (pos == 3)
	return havingExpr_;
     else
	return NULL;
  }
NA_EIDPROC
  virtual const char * getExpressionName(Int32 pos) const {
     if (pos == 0)
	return "aggrExpr_";
     else if (pos == 1)
	return "grbyExpr_";
     else if (pos == 2)
	return "moveExpr_";
     else if (pos == 3)
	return "havingExpr_";
     else
	return NULL;
  }   
   
};

NA_EIDPROC
inline ComTdb * ComTdbSortGrby::getChildTdb(){
  return tdbChild_;
};

/*****************************************************************************
  Description : Return ComTdb* depending on the position argument.
                  Position 0 means the left most child.
  Comments    :
  History     : Yeogirl Yun                                      8/22/95
                 Initial Revision.
*****************************************************************************/
inline const ComTdb* ComTdbSortGrby::getChild(Int32 pos) const
{
   if (pos == 0)
      return tdbChild_;
   else
      return NULL;
}

#endif
