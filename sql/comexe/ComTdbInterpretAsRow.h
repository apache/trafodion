/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2005-2014 Hewlett-Packard Development Company, L.P.
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
* File:         ComTdbInterpretAsRow.h
* Description:  
*
* Created:      6/1/2005 
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COM_IAR_H
#define COM_IAR_H

#include "ComTdb.h"

// LCOV_EXCL_START
// Not used currently -- keep in case we ever support online populate index

class ComTdbInterpretAsRow: public ComTdb
{
  friend class ExIarTcb;
  friend class ExIarPrivateState;

  public:

  // Dummy constructor - used by unpack routines.
  ComTdbInterpretAsRow();

  // Constructor
  ComTdbInterpretAsRow (UInt32 compressionFlag,
                        UInt32 extractedRowLen,
                        UInt32 auditImageRowLen,
                        UInt32 outputRowLen,
                        Cardinality estimatedRowCount,
                        ex_cri_desc *work_cri_desc,
                        ex_cri_desc *given_cri_desc,
                        ex_cri_desc *returned_cri_desc,
                        ex_expr *extractExpr,
                        ex_expr *scanExpr,
                        ex_expr *projExpr,
                        queue_index upQueueSize,
                        queue_index downQueueSize,
                        Lng32 numBuffers,
                        ULng32 bufferSize);

  // destructor
  virtual ~ComTdbInterpretAsRow();

  Int32 orderedQueueProtocol() const { return -1; }

  // Redefine virtual functions required for versioning.
  virtual unsigned char getClassVersionID() { return 1; }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ComTdb::populateImageVersionIDArray();
  }
 
  virtual short getClassSize() { return (short)sizeof(ComTdbInterpretAsRow); }
  virtual Long pack(void *);
  virtual Lng32 unpack(void *, void *reallocator);
 
  // this is a leaf node, i.e., no children 
  virtual const ComTdb *getChild(Int32 pos) const { return NULL; }
  virtual Int32 numChildren() const { return 0; } 

  virtual const char *getNodeName() const { return "EX_INTERPRET_AS_ROW_TDB"; }
  virtual Int32 numExpressions() const { return 3; }

  virtual ex_expr *getExpressionNode(Int32 pos)
  {
     switch(pos)
     {
        case 0:
           return extractExpr_;
        case 1:
           return scanExpr_;
        case 2:
           return projExpr_;
        default:
           return NULL;
     }
  }
 
  virtual const char *getExpressionName(Int32 pos) const
  {
     switch(pos)
     {
        case 0:
           return "extractExpr_";
        case 1:
           return "scanExpr_";
        case 2:
           return "projExpr_";
        default:
           return NULL;
     }
  }

  void displayContents(Space *space, ULng32 flag);

  inline ex_cri_desc *getWorkCriDesc() const { return workCriDesc_; }
  UInt32 getAuditRowImageLen() { return auditRowImageLen_; }
  UInt32 getExtractedRowLen() { return extractedRowLen_; }
  inline ex_expr * getExtractExpr() const { return extractExpr_; }
  inline ex_expr * getScanExpr() const { return scanExpr_; }
  inline ex_expr * getProjExpr() const { return projExpr_; }
  inline UInt32 getAuditCompressionFlag() const 
                          { return flags_.auditCompressionFlag_; }

  protected:
  ExCriDescPtr workCriDesc_;                      // 00-07
  ExExprPtr scanExpr_;                            // 08-15
  ExExprPtr projExpr_;                            // 16-23
  ExExprPtr extractExpr_;                         // 24-31 
  UInt32 auditRowImageLen_;                       // 32-35 
  UInt32 extractedRowLen_;                        // 36-39
  UInt32 outputRowLen_;                           // 39-43 
  union {                                         // 44-47 
    UInt32 value_;
    struct {
       UInt32 auditCompressionFlag_:2;
       UInt32 filler1_:14;
       UInt32 filler2_:16;
    } flags_;
  };
  char fillerComTdbIAR[72];                       // 48-119 
};
// LCOV_EXCL_STOP
#endif
