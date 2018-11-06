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
* File:         ComTdbUdr.h
* Description:  TDB class for user-defined routines
*
* Created:      2/8/2000
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COM_TDB_UDR_H
#define COM_TDB_UDR_H

#include "ComTdb.h"
#include "UdrFormalParamInfo.h"
#include "udrtabledescinfo.h"
#include "ComQueue.h"

//
// Classes defined in this file
//
class ComTdbUdr;


//
// ComTdbUdr is the TDB class for UDRs. It contains all routine metadata
// and information needed by the TCB at runtime. Column-level metadata is
// stored in an array of UdrFormalParamInfo objects. The UdrFormalParamInfo
// class is defined in another file.
//
class ComTdbUdr : public ComTdb
{
public:

  friend class ExUdrTcb;

  ComTdbUdr (
    char *sqlName,
    char *routineName,
    char *routineSignature,
    char *containerName,
    char *externalPath,
    char *librarySqlName,
    Int64 libraryRedefTime,
    char *libraryBlobHandle,
    char *librarySchName,
    Int32 libraryVersion,
    char *javaOptions,
    char *javaOptionDelimiters,

    ULng32 flags,
    ULng32 numInputValues,
    ULng32 numOutputValues,
    ULng32 numParams,
    ULng32 maxResultSets,
    ULng32 stateAreaSize,
    short udrType,
    short languageType,
    short paramStyle,
    short sqlAccessMode,
    short transactionAttrs,
    UInt16 externalSecurity,
    Int32 routineOwnerId,
    Cardinality estimatedRowCount,
    ex_cri_desc *criDescParent,
    ex_cri_desc *criDescReturned,
    ex_cri_desc *workCriDesc,
    queue_index downQueueMaxSize,
    queue_index upQueueMaxSize,

    Lng32 numOutputBuffers,
    ULng32 outputBufferSize,
    ULng32 requestBufferSize,
    ULng32 replyBufferSize,
    
    ex_expr *inputExpr,
    ex_expr *outputExpr,
    ex_expr *scanExpr,
    ex_expr *projExpr,

    unsigned short requestTuppIndex,
    unsigned short replyTuppIndex,
    ULng32 requestRowLen,
    ULng32 replyRowLen,
    ULng32 outputRowLen,

    ULng32 numChildInputs,
    ex_expr ** childInputExprs,
    ComTdb ** childTdbs,
    Queue *optionalData,

    Int32 udrSerInvocationInfoLen,
    char *udrSerInvocationInfo,
    Int32 udrSerPlanInfoLen,
    char *udrSerPlanInfo,

    Int32 javaDebugPort,
    Int32 javaDebugTimeout,

    Space *space

    );
  
  
  ~ComTdbUdr();

  // ---------------------------------------------------------------------
  // Default constructor is only called to instantiate an object used
  // for retrieval of the virtual table function pointer of the class
  // while unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ComTdbUdr()
    : ComTdb(ex_UDR, "FAKE")
  {
  }

  //----------------------------------------------------------------------
  // Redefine virtual functions required for versioning
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
  {
    return (short) sizeof(ComTdbUdr);
  }
  
  //----------------------------------------------------------------------
  // Pack/unpack
  //----------------------------------------------------------------------
  Long pack(void *);
  Lng32 unpack(void *, void *);

  //----------------------------------------------------------------------
  // Other required TDB support functions
  //----------------------------------------------------------------------
  Int32 orderedQueueProtocol() const
  {
    return -1;
  }
  void display() const
  {
    // All TDBs have an no-op display() function. Not sure why.
  }
  virtual Int32 numChildren() const
  {
    return numChildTableInputs_;
  }
  virtual const char *getNodeName() const
  {
    return "EX_UDR";
  }
  virtual const ComTdb *getChild(Int32 pos) const
  {
    if ((numChildTableInputs_ > 0) && (pos < numChildTableInputs_) && (pos >= 0))
      return childTdbs_[pos];
    else
    return NULL;
  }
  virtual Int32 numExpressions() const
  {
    return (4+numChildTableInputs_);
  }
  virtual const char *getExpressionName(Int32 pos) const
  {
    if (pos == 0)
      return "inputExpr_";
    else if (pos == 1)
      return "outputExpr_";
    else if (pos == 2)
      return "scanExpr_";
    else if (pos == 3)
      return "projExpr_";
    else if (childInputExprs_ && (pos >= 4) && (pos < (4+numChildTableInputs_))) 
      return( "childInputExpr_" );
    else
      return NULL;
  }
  virtual ex_expr *getExpressionNode(Int32 pos)
  {
    if (pos == 0)
      return inputExpr_;
    else if (pos == 1)
      return outputExpr_;
    else if (pos == 2)
      return scanExpr_;
    else if (pos == 3)
      return projExpr_;
    else if (childInputExprs_ && (pos >= 4) && (pos < (4+numChildTableInputs_))) 
      return childInputExprs_[pos-4];
    else
      return NULL;
  }
  virtual void displayContents(Space *space, ULng32 flag);

  //----------------------------------------------------------------------
  // Methods to get/set the param info objects
  //----------------------------------------------------------------------
  inline const UdrFormalParamInfo *getFormalParamInfo(UInt32 i) const
  {
    return paramInfo_[i];
  }
  inline void setFormalParamInfo(UInt32 i, UdrFormalParamInfo *info)
  {
    paramInfo_[i] = info;
  }

  inline const char *getSqlName() const { return sqlName_; }

  inline const char *getRuntimeOptions() const { return runtimeOptions_; }
  inline const char *getRuntimeOptionDelimiters() const
  { return runtimeOptionDelimiters_; }

  Queue *getOptionalData() const { return optionalData_; }
  void setOptionalData(Queue *q) { optionalData_ = q; }
  NABoolean isTMUDF() { return flags_&UDR_TMUDF ? TRUE : FALSE ;}
  Int16 getNumChildTableInputs(){return numChildTableInputs_;};
  inline const UdrTableDescInfo *getTableDescInfo(UInt32 i) const
  {
    return udrChildTableDescInfo_[i];
  }
  inline void setTableDescInfo(UInt32 i, UdrTableDescInfo *info)
  {
    udrChildTableDescInfo_[i] = info;
  }


protected:

  NABasicPtr sqlName_;                                    // 00-07
  NABasicPtr routineName_;                                // 08-15
  NABasicPtr routineSignature_;                           // 16-23
  NABasicPtr containerName_;                              // 24-31
  NABasicPtr externalPath_;                               // 32-39
  NABasicPtr librarySqlName_;                             // 40-47

  ExExprPtr inputExpr_;                                   // 48-55
  ExExprPtr outputExpr_;                                  // 56-63
  ExCriDescPtr workCriDesc_;                              // 64-71

  UdrFormalParamInfoPtrPtr paramInfo_;                    // 72-79
  
  UInt32 flags_;                                          // 80-83
  
  UInt32 numInputValues_;                                 // 84-87
  UInt32 numOutputValues_;                                // 88-91
  UInt32 numParams_;                                      // 92-95
  UInt32 maxResultSets_;                                  // 96-99

  UInt32 requestSqlBufferSize_;                           // 100-103
  UInt32 replySqlBufferSize_;                             // 104-107

  UInt32 requestRowLen_;                                  // 108-111
  UInt32 replyRowLen_;                                    // 112-115
  UInt32 outputRowLen_;                                   // 116-119
  UInt16 requestTuppIndex_;                               // 120-121
  UInt16 replyTuppIndex_;                                 // 122-123

  UInt32 stateAreaSize_;                                  // 124-127
  Int16 udrType_;                                         // 128-129
  Int16 languageType_;                                    // 130-131
  Int16 paramStyle_;                                      // 132-133
  Int16 sqlAccessMode_;                                   // 134-135
  Int16 transactionAttrs_;                                // 136-137
  Int16   numChildTableInputs_;                           // 138-139

  // Definer Rights related 
  UInt16 externalSecurity_;                               // 140-141

  char fillerComTdbUdr1_[2];                              // 142-143

  NABasicPtr runtimeOptions_;                             // 144-151
  NABasicPtr runtimeOptionDelimiters_;                    // 152-159
  QueuePtr optionalData_;                                 // 160-167
  ExExprPtr scanExpr_;                                    // 168-175
  ExExprPtr projExpr_;                                    // 176-183
  UdrTableDescInfoPtrPtr udrChildTableDescInfo_;           //184-191
  // A vector of pointers to ex_expr.  There are 'numChildTableInputs_'
  // in the vector.  Each expression represents a move expression, to
  // move input from a particular child output into the buffer to be
  // sent to MXUDR
  ExExprPtrPtr childInputExprs_;                           // 192-199
  ComTdbPtrPtr childTdbs_;                                 // 200-207

  // Definer Rights related 
  Int32 routineOwnerId_;                                  // 208-211
  // serialized UDRInvocationInfo and UDRPlanInfo
  Int32 udrSerInvocationInfoLen_;                         // 212-215
  NABasicPtr udrSerInvocationInfo_;                       // 216-223
  Int32 udrSerPlanInfoLen_;                               // 224-227
  NABasicPtr udrSerPlanInfo_;                             // 228-235

  Int32 javaDebugPort_;                                   // 236-239
  Int32 javaDebugTimeout_;                                // 240-243
  Int64 libraryRedefTime_;                                // 244-251
  NABasicPtr libraryBlobHandle_;                          // 252-259
  NABasicPtr librarySchName_;                             // 260-267
  Int32 libraryVersion_;                                  // 268-271
  // Make sure class size is a multiple of 8
  char fillerComTdbUdr2_[24];                             // 272-295
};


#endif // COM_TDB_UDR_H

