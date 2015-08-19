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
* File:         ComTdbTimeout.h
* Description:  TDB for SET TIMEOUT
*
* Created:      12/27/99
* Language:     C++
*
*
****************************************************************************
*/

#ifndef COM_TDB_TIMEOUT_H
#define COM_TDB_TIMEOUT_H

#include "ComTdb.h"

///////////////////////////////////////////////////////
//   class ComTdbTimeout
///////////////////////////////////////////////////////
class ComTdbTimeout : public ComTdb
{
  friend class ExTimeoutTcb;
  friend class ExTimeoutPrivateState;

public:
  enum { STO_STREAM = 0x0001 , STO_RESET = 0x0002 };

  ComTdbTimeout():ComTdb(ComTdb::ex_SET_TIMEOUT, eye_SET_TIMEOUT) {};
    
  ComTdbTimeout( ex_expr * timeout_value_expr,
		 ex_cri_desc * work_cri_desc,
		 ex_cri_desc * given_cri_desc,
		 ex_cri_desc * returned_cri_desc,
		 queue_index down,
		 queue_index up,
		 Lng32 num_buffers,
		 ULng32 buffer_size);

  // methods to set/check the RESET / STREAM flags
  void setStream ( NABoolean isStream )
  { if ( isStream ) flags_ |= STO_STREAM; else flags_ &= ~STO_STREAM ; }

  NABoolean isStream () { return flags_ & STO_STREAM ; }

  void setReset ( NABoolean isReset )
  { if ( isReset ) flags_ |= STO_RESET; else flags_ &= ~STO_RESET ; }

  NABoolean isReset () { return flags_ & STO_RESET ; }

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

  virtual short getClassSize() { return (short)sizeof(ComTdbTimeout); } 

  Long pack (void *);
  Lng32 unpack(void *, void * reallocator);
  
  Int32 orderedQueueProtocol() const{return -1;};
  
  virtual const ComTdb *getChild(Int32 /*child*/) const { return NULL; };
  virtual Int32 numChildren() const { return 0; };
  virtual const char *getNodeName() const { return "EX_SET_TIMEOUT"; };
  virtual Int32 numExpressions() const { return 1; };
  virtual const char * getExpressionName(Int32 pos) const 
  { return pos == 0 ? "timeoutValueExpr_" : NULL ; };
  virtual ex_expr* getExpressionNode(Int32 pos) 
  { return pos == 0 ? timeoutValueExpr_ : (ExExprPtr) NULL ; };

protected:

  // expression used to compute the timeout value
  ExExprPtr timeoutValueExpr_;                               // 00-07

  // cri desc to evaluate the timeoutValueExpr_
  ExCriDescPtr workCriDesc_;                                 // 08-15

  // Keep the booleans: Is it a stream, is it a RESET
  UInt16 flags_;                                             // 16-17

  char fillersComTdbTimeout_[46];                            // 18-63

};


#endif
