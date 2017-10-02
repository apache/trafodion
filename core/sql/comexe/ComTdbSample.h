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
******************************************************************************
*
* File:         $File$
* RCS:          $Id$
* Description:  
* Created:      
* Language:     C++
* Status:       $State$
*
*
*
*
******************************************************************************
*/

#ifndef ComTdbSample_h
#define ComTdbSample_h

#include "ComTdb.h"
#include "ComPackDefs.h"

// Task Definition Block
//
class ComTdbSample : public ComTdb
{
  friend class ExSampleTcb;
  friend class ExSamplePrivateState;

public:
  ComTdbSample();
  
  ComTdbSample(ex_expr *initExpr,
	       ex_expr *balanceExpr,
	       Int32 returnFactorOffset,
	       ex_expr *postPred,
	       ComTdb * child_tdb,
	       ex_cri_desc * given_cri_desc,
	       ex_cri_desc * returned_cri_desc,
	       queue_index down,
	       queue_index up);
  
  ~ComTdbSample();
  
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
  
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);
  
  void display() const;

  inline ComTdb * getChildTdb();

  Int32 orderedQueueProtocol() const;

  virtual const ComTdb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const { return 1; }
  virtual const char *getNodeName() const { return "EX_SAMPLE"; };
  virtual Int32 numExpressions() const { return 3; }
  virtual ex_expr* getExpressionNode(Int32 pos) {
    if (pos == 0)
      return initExpr_;
    else if(pos == 1)
      return balanceExpr_;
    else if(pos == 2)
      return postPred_;
    else
      return NULL;
  }
  virtual const char * getExpressionName(Int32 pos) const {
    if (pos == 0)
      return "initExpr_";
    else if (pos == 1)
      return "balanceExpr_";
    else if (pos == 2)
      return "postPred_";
    else
      return NULL;
  }   
  
protected:
  
  ExExprPtr initExpr_;           // 00-07
  ExExprPtr balanceExpr_;        // 08-15
  ExExprPtr postPred_;           // 16-23
  ComTdbPtr tdbChild_;           // 24-31
  Int32     returnFactorOffset_; // 32-35
  // ---------------------------------------------------------------------
  // Filler for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char      fillers_[36];        // 36-71
};

inline ComTdb * ComTdbSample::getChildTdb(){
  return tdbChild_;
};


/*****************************************************************************
  Description : Return ComTdb* depending on the position argument.
                  Position 0 means the left most child.
  Comments    :
  History     : Yeogirl Yun                                      8/22/95
                 Initial Revision.
*****************************************************************************/
inline const ComTdb* ComTdbSample::getChild(Int32 pos) const
{
  if (pos == 0)
    return tdbChild_;
  else
    return NULL;
}

#endif
