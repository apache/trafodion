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
#ifndef ComCompoundStmt_h
#define ComCompoundStmt_h

/* -*-C++-*-
******************************************************************************
*
* File: 	ComTdbCompoundStmt.h
* Description:  3GL compound statement (CS) operator.
*               
* Created:      4/1/98
* Language:     C++
*
*
*
******************************************************************************
*/

#include "ComTdb.h"

//////////////////////////////////////////////////////////////////////////////
//
// CompoundStmt TDB class.
//
//////////////////////////////////////////////////////////////////////////////
class ComTdbCompoundStmt : public ComTdb
{
  friend class ExCatpoundStmtTcb;
  friend class ExCatpoundStmtPrivateState;
  
public:
  //
  // Standard TDB methods.
  //
  ComTdbCompoundStmt();
  
  ComTdbCompoundStmt(ComTdb *left,
                     ComTdb *right,
	             ex_cri_desc *given,
	             ex_cri_desc *returned,
	             queue_index down,
                     queue_index up,
	             Lng32 numBuffers,
	             ULng32 bufferSize,
                     NABoolean rowsFromLeft,
                     NABoolean rowsFromRight,
                     NABoolean AfterUpdate);


// exclude from code coverage analsysis since this method is not used
  Int32 orderedQueueProtocol() const { return -1; } 

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  void display() const {};
  
  virtual const ComTdb *getChild(Int32 pos) const;

  virtual Int32 numChildren() const { return 2; }
  virtual Int32 numExpressions() const { return 0; }

  virtual const char *getNodeName() const { return "EX_COMPOUND_STMT"; };

// exclude from code coverage analysys since there are no expressions for
// this operator
  virtual ex_expr* getExpressionNode(Int32 pos) { return NULL; } 

  virtual const char * getExpressionName(Int32 pos) const { return NULL; }   

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

  virtual short getClassSize()     { return (short)sizeof(*this); }
  
protected:
  //
  // CS TDB specific attributes.
  //

  enum {ROWS_FROM_LEFT = 0x0001,  ROWS_FROM_RIGHT = 0x0002,
        AFTER_UPDATE = 0x0004};

  ComTdbPtr tdbLeft_;     // TDB for left child.  00-07
  ComTdbPtr tdbRight_;    // TDB for right child. 08-15

  UInt16    flags_;                            // 16-17

  char fillersComTdbCompound_[30];             // 18-47
  
  inline NABoolean expectingLeftRows() const
                { return (flags_ & ROWS_FROM_LEFT) != 0; }

  inline NABoolean expectingRightRows() const
                { return (flags_ & ROWS_FROM_RIGHT) != 0; }

  inline NABoolean afterUpdate() const
                { return (flags_ & AFTER_UPDATE) != 0; }

};

#endif // ComCompoundStmt_h


