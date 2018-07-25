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
// File:       RtsExeIpc.cpp
// Description:  IPC streams and message objects shared by the
//               executor, SSCP and SSMP
//
// Created:      04/27/2006
******************************************************************************
*/

#include "Platform.h"
#include <iostream>

#include "sqlcli.h"
#include "IpcMessageObj.h"
#include "rts_msg.h"
#include "ExCextdecs.h"
/*
// Helper function to determine number of bytes that a string
// will occupy in a packed IpcMessageObj.
//
inline static IpcMessageObjSize packedStringLength(const char *s)
{
  //
  // The string will be preceded by a 4-byte length field and will
  // include the null-terminator
  //
  IpcMessageObjSize result;
  result = sizeof(long);
  if (s)
  {
    result += str_len(s) + 1;
  }
  return result;
}
*/
// -----------------------------------------------------------------------
// class RtsMessageObj
// -----------------------------------------------------------------------
RtsMessageObj::RtsMessageObj(RtsMessageObjType objType,
                             IpcMessageObjVersion objVersion,
                             NAMemory *heap)
 : IpcMessageObj(objType, objVersion),
 heap_(heap),
 handle_(INVALID_RTS_HANDLE)
{
}

void RtsMessageObj::operator delete(void *p)
{
  if (p)
  {
    NAMemory *h = ((RtsMessageObj *) p)->getHeap();
    if (h)
    {
      h->deallocateMemory(p);
    }
    else
    {
      ::delete ((RtsMessageObj *) p);
    }
  }
}

IpcMessageObjSize RtsMessageObj::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(handle_);
  return result;
}

IpcMessageObjSize RtsMessageObj::packObjIntoMessage(IpcMessageBufferPtr buffer)
{

  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, handle_);
  return result;
}

void RtsMessageObj::unpackObj(IpcMessageObjType objType,
                              IpcMessageObjVersion objVersion,
                              NABoolean sameEndianness,
                              IpcMessageObjSize objSize,
                              IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);
  unpackBuffer(buffer, handle_);
}


IpcMessageRefCount RtsMessageObj::decrRefCount()
{
  IpcMessageRefCount result = 0;

  if (getRefCount() == 1)
  {
    //First clean up space allocated within derived objects.
    deleteMe();

    //
    // IpcMessageObj::decrRefCount() would delete the object by calling
    // global operator delete since IpcMessageObj doesn't have an
    // operator delete. However, if we code the "delete this" statement
    // in the context of this class we will pick up the correct operator
    // delete.
    //
    delete this;
    result = 0;
  }
  else
  {
    //
    // This is the normal case. The object won't be deleted
    //
    result = IpcMessageObj::decrRefCount();
  }

  return result;
}

IpcMessageObjSize RtsStatsReq::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(wmsProcess_);
  return result;
}

IpcMessageObjSize RtsStatsReq::packObjIntoMessage(IpcMessageBufferPtr buffer)
{

  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, wmsProcess_);
  return result;
}

void RtsStatsReq::unpackObj(IpcMessageObjType objType,
                              IpcMessageObjVersion objVersion,
                              NABoolean sameEndianness,
                              IpcMessageObjSize objSize,
                              IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);
  unpackBuffer(buffer, wmsProcess_);
}

IpcMessageObjSize RtsStatsReply::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(numSscpErrors_);
  result += sizeof(numSqlProcs_);
  result += sizeof(numCpus_);
  return result;

}

IpcMessageObjSize RtsStatsReply::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, numSscpErrors_);
  result += packIntoBuffer(buffer, numSqlProcs_);
  result += packIntoBuffer(buffer, numCpus_);
  return result;

}

void RtsStatsReply::unpackObj(IpcMessageObjType objType,
		 IpcMessageObjVersion objVersion,
		 NABoolean sameEndianness,
		 IpcMessageObjSize objSize,
		 IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);
  unpackBuffer(buffer, numSscpErrors_);
  unpackBuffer(buffer, numSqlProcs_);
  unpackBuffer(buffer, numCpus_);
}

IpcMessageObjSize RtsQueryId::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();

  result += sizeof(queryIdLen_);
  result += queryIdLen_;
  result += sizeof(nodeName_);
  result += sizeof(cpu_);
  result += sizeof(pid_);
  result += sizeof(timeStamp_);
  result += sizeof(queryNumber_);
  result += sizeof(reqType_);
  result += sizeof(statsMergeType_);
  result += sizeof(activeQueryNum_);
  result += sizeof(detailLevel_);
  result += sizeof(subReqType_);
  return result;

}

IpcMessageObjSize RtsQueryId::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, queryIdLen_);
  str_cpy_all(buffer, queryId_, queryIdLen_);
  buffer += queryIdLen_;
  result += queryIdLen_;
  str_cpy_all(buffer, nodeName_, sizeof(nodeName_));
  result += sizeof(nodeName_);
  buffer += sizeof(nodeName_);
  result += packIntoBuffer(buffer, cpu_);
  result += packIntoBuffer(buffer, pid_);
  result += packIntoBuffer(buffer, timeStamp_);
  result += packIntoBuffer(buffer, queryNumber_);
  result += packIntoBuffer(buffer, reqType_);
  result += packIntoBuffer(buffer, statsMergeType_);
  result += packIntoBuffer(buffer, activeQueryNum_);
  result += packIntoBuffer(buffer, detailLevel_);
  result += packIntoBuffer(buffer, subReqType_);
  return result;
}


void RtsQueryId::unpackObj(IpcMessageObjType objType,
				   IpcMessageObjVersion objVersion,
				   NABoolean sameEndianness,
				   IpcMessageObjSize objSize,
				   IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);

  unpackBuffer(buffer, queryIdLen_);

  queryId_ = new (getHeap()) char[queryIdLen_+1];
  str_cpy_all(queryId_, buffer, queryIdLen_);
  queryId_[queryIdLen_] = '\0';
  buffer += queryIdLen_;
  str_cpy_all(nodeName_, buffer, sizeof(nodeName_));
  buffer += sizeof(nodeName_);
  unpackBuffer(buffer, cpu_);
  unpackBuffer(buffer, pid_);
  unpackBuffer(buffer, timeStamp_);
  unpackBuffer(buffer, queryNumber_);
  unpackBuffer(buffer, reqType_);
  unpackBuffer(buffer, statsMergeType_);
  unpackBuffer(buffer, activeQueryNum_);
  unpackBuffer(buffer, detailLevel_);
  unpackBuffer(buffer, subReqType_);
}

void RtsQueryId::deleteMe()
{
  if (queryId_ != NULL)
  {
    NAMemory *h = getHeap();
    if (h)
      h->deallocateMemory(queryId_);
  }
}

RtsQueryId::RtsQueryId(NAMemory *heap, char *nodeName, short cpu, UInt16 statsMergeType,
                       short activeQueryNum)
    : RtsMessageObj(RTS_QUERY_ID, currRtsQueryIdVersionNumber, heap),
    cpu_(cpu),
    timeStamp_(-1),
    queryNumber_(-1),
    statsMergeType_(statsMergeType)
{
  short len;
  if (nodeName != NULL)
    str_cpy_all(nodeName_, nodeName, str_len(nodeName)+1);
  else
  {
    len = 0;
    nodeName_[len] = '\0';
  }
  reqType_ = SQLCLI_STATS_REQ_CPU;
  pid_ = -1;
  queryId_ = NULL;
  queryIdLen_ = 0;
  activeQueryNum_ = activeQueryNum;
  detailLevel_ = 0;
  subReqType_ = -1;
}

RtsQueryId::RtsQueryId(NAMemory *heap, char *nodeName, short cpu, pid_t pid, UInt16 statsMergeType,
                       short activeQueryNum,
                       short reqType)
    : RtsMessageObj(RTS_QUERY_ID, currRtsQueryIdVersionNumber, heap),
    cpu_(cpu),
    pid_(pid),
    timeStamp_(-1),
    queryNumber_(-1),
    statsMergeType_(statsMergeType)
{
  short len;
  if (nodeName_ != NULL)
    str_cpy_all(nodeName_, nodeName, str_len(nodeName)+1);
  else
  {
    len = 0;
    nodeName_[len] = '\0';
  }
  reqType_ = reqType;
  queryId_ = NULL;
  queryIdLen_ = 0;
  activeQueryNum_ = activeQueryNum;
  detailLevel_ = 0;
  subReqType_ = -1;
}

RtsQueryId::RtsQueryId(NAMemory *heap, char *nodeName, short cpu, pid_t pid, Int64 timeStamp, Lng32 queryNumber,
                       UInt16 statsMergeType,
                       short activeQueryNum,
                       short reqType)
    : RtsMessageObj(RTS_QUERY_ID, currRtsQueryIdVersionNumber, heap),
    cpu_(cpu),
    pid_(pid),
    timeStamp_(timeStamp),
    queryNumber_(queryNumber),
    statsMergeType_(statsMergeType)
{
  short len;
  if (nodeName_ != NULL)
    str_cpy_all(nodeName_, nodeName, str_len(nodeName)+1);
  else
  {
    len = 0;
    nodeName_[len] = '\0';
  }
  reqType_ = reqType;
  queryId_ = NULL;
  queryIdLen_ = 0;
  activeQueryNum_ = activeQueryNum;
  detailLevel_ = 0;
  subReqType_ = -1;
}


RtsCpuStatsReq::RtsCpuStatsReq(const RtsHandle &h, NAMemory *heap,
                               char *nodeName, short cpu, short noOfQueries, short reqType)
    : RtsMessageObj(RTS_MSG_CPU_STATS_REQ, currRtsCpuStatsReqVersionNumber, heap)
{
  short len;

  setHandle(h);
  if (nodeName_ != NULL)
    str_cpy_all(nodeName_, nodeName, str_len(nodeName)+1);
  else
  {
    len = 0;
    nodeName_[len] = '\0';
  }
  cpu_ = cpu;
  noOfQueries_ = noOfQueries;
  reqType_ = reqType;
  subReqType_ = -1;
  filter_ = -1;
}

IpcMessageObjSize RtsCpuStatsReq::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();

  result += sizeof(nodeName_);
  result += sizeof(cpu_);
  result += sizeof(noOfQueries_);
  result += sizeof(reqType_);
  result += sizeof(subReqType_);
  result += sizeof(filter_);
  return result;

}

IpcMessageObjSize RtsCpuStatsReq::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  str_cpy_all(buffer, nodeName_, sizeof(nodeName_));
  result += sizeof(nodeName_);
  buffer += sizeof(nodeName_);
  result += packIntoBuffer(buffer, cpu_);
  result += packIntoBuffer(buffer, noOfQueries_);
  result += packIntoBuffer(buffer, reqType_);
  result += packIntoBuffer(buffer, subReqType_);
  result += packIntoBuffer(buffer, filter_);
  return result;
}


void RtsCpuStatsReq::unpackObj(IpcMessageObjType objType,
				   IpcMessageObjVersion objVersion,
				   NABoolean sameEndianness,
				   IpcMessageObjSize objSize,
				   IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);

  str_cpy_all(nodeName_, buffer, sizeof(nodeName_));
  buffer += sizeof(nodeName_);
  unpackBuffer(buffer, cpu_);
  unpackBuffer(buffer, noOfQueries_);
  unpackBuffer(buffer, reqType_);
  unpackBuffer(buffer, subReqType_);
  unpackBuffer(buffer, filter_);
}

void RtsExplainFrag::setExplainFrag(void *explainFrag, Lng32 len, Lng32 topNodeOffset)
{
  if (explainFrag_ != NULL)
  {
    NADELETEBASIC((char *)explainFrag_, getHeap());
    explainFrag_ = NULL;
    explainFragLen_ = 0;
    topNodeOffset_ = 0;
  }
  if (explainFrag != NULL)
  {
    explainFrag_ = new (getHeap()) char[len + 10]; // Why add 10
    memcpy(explainFrag_, explainFrag, len);
    explainFragLen_ = len;
    topNodeOffset_ = topNodeOffset;
  }
  else
  {
    explainFragLen_ = 0;
    topNodeOffset_ = 0;
  }
}

RtsExplainFrag::RtsExplainFrag(NAMemory *heap, RtsExplainFrag *other)
    : RtsMessageObj(RTS_EXPLAIN_FRAG, currRtsExplainFragVersionNumber, heap)
{
  explainFrag_ = NULL;
  explainFragLen_ = 0;
  topNodeOffset_ = 0;
  setExplainFrag(other->getExplainFrag(), other->getExplainFragLen(), other->getTopNodeOffset());
}

IpcMessageObjSize RtsExplainFrag::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();

  result += sizeof(explainFragLen_);
  result += explainFragLen_;
  result += sizeof(topNodeOffset_);
  return result;

}

IpcMessageObjSize RtsExplainFrag::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, explainFragLen_);
  result += packStrIntoBuffer(buffer, (char *)explainFrag_, explainFragLen_);
  result += packIntoBuffer(buffer, topNodeOffset_);
  return result;
}


void RtsExplainFrag::unpackObj(IpcMessageObjType objType,
				   IpcMessageObjVersion objVersion,
				   NABoolean sameEndianness,
				   IpcMessageObjSize objSize,
				   IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);

  unpackBuffer(buffer, explainFragLen_);
  explainFrag_ = new (getHeap()) char[explainFragLen_+10];
  unpackStrFromBuffer(buffer, (char *)explainFrag_, explainFragLen_);
  unpackBuffer(buffer, topNodeOffset_);
}

IpcMessageObjSize QueryStarted::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(startTime_);
  result += sizeof(master_);
  result += sizeof(executionCount_);
  result += sizeof(qsFlags_);
  return result;
}

RtsExplainReq::RtsExplainReq(const RtsHandle &h, NAMemory *heap, char *qid, Lng32 qidLen)
: RtsMessageObj(RTS_MSG_EXPLAIN_REQ, currRtsExplainReqVersionNumber, heap)
{
  setHandle(h);
  qid_ = new (heap) char[qidLen+1];
  str_cpy_all(qid_, qid, qidLen);
  qid_[qidLen] = '\0';
  qidLen_ = qidLen;
}

RtsExplainReq::~RtsExplainReq()
{
  if (qid_ != NULL)
    NADELETEBASIC(qid_, getHeap());
  qid_ = NULL;
}

IpcMessageObjSize RtsExplainReq::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();

  result += sizeof(qidLen_);
  result += qidLen_;
  return result;
}

IpcMessageObjSize RtsExplainReq::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, qidLen_);
  result += packStrIntoBuffer(buffer, qid_, qidLen_);
  return result;
}

void RtsExplainReq::unpackObj(IpcMessageObjType objType,
                                   IpcMessageObjVersion objVersion,
                                   NABoolean sameEndianness,
                                   IpcMessageObjSize objSize,
                                   IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);

  unpackBuffer(buffer, qidLen_);
  qid_ = new (getHeap()) char[qidLen_+1];
  unpackStrFromBuffer(buffer, qid_, qidLen_);
  qid_[qidLen_] = '\0';
}


IpcMessageObjSize QueryStarted::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, startTime_);
  result += packIntoBuffer(buffer, master_);
  result += packIntoBuffer(buffer, executionCount_);
  result += packIntoBuffer(buffer, qsFlags_);
  return result;
}

void QueryStarted::unpackObj(IpcMessageObjType objType,
				   IpcMessageObjVersion objVersion,
				   NABoolean sameEndianness,
				   IpcMessageObjSize objSize,
				   IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);

  unpackBuffer(buffer, startTime_);
  unpackBuffer(buffer, master_);
  unpackBuffer(buffer, executionCount_);
  unpackBuffer(buffer, qsFlags_);
}

IpcMessageObjSize QueryStartedReply::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(nextAction_);
  result += sizeof(cancelLogging_);
  return result;
}

IpcMessageObjSize QueryStartedReply::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, nextAction_);
  result += packIntoBuffer(buffer, cancelLogging_);
  return result;
}

void QueryStartedReply::unpackObj(IpcMessageObjType objType,
				   IpcMessageObjVersion objVersion,
				   NABoolean sameEndianness,
				   IpcMessageObjSize objSize,
				   IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);

  unpackBuffer(buffer, nextAction_);
  unpackBuffer(buffer,cancelLogging_);
}

IpcMessageObjSize SuspendQueryRequest::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(forced_);
  result += sizeof(suspendLogging_);
  return result;
}

IpcMessageObjSize SuspendQueryRequest::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, forced_);
  result += packIntoBuffer(buffer, suspendLogging_);
  return result;
}

void SuspendQueryRequest::unpackObj(IpcMessageObjType objType,
				   IpcMessageObjVersion objVersion,
				   NABoolean sameEndianness,
				   IpcMessageObjSize objSize,
				   IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);

  unpackBuffer(buffer, forced_);
  unpackBuffer(buffer,suspendLogging_);
}

IpcMessageObjSize ActivateQueryRequest::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(suspendLogging_);
  return result;
}

IpcMessageObjSize ActivateQueryRequest::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, suspendLogging_);
  return result;
}

void ActivateQueryRequest::unpackObj(IpcMessageObjType objType,
                                   IpcMessageObjVersion objVersion,
                                   NABoolean sameEndianness,
                                   IpcMessageObjSize objSize,
                                   IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);
  unpackBuffer(buffer,suspendLogging_);
}

IpcMessageObjSize CancelQueryRequest::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(cancelStartTime_);
  result += sizeof(firstEscalationInterval_);
  result += sizeof(secondEscalationInterval_);
  result += sizeof(cancelEscalationSaveabend_);
  result += sizeof(commentLen_);
  result += commentLen_;
  result += sizeof(cancelLogging_);
  result += sizeof(cancelByPid_);
  result += sizeof(cancelPid_);
  result += sizeof(minimumAge_);
  return result;
}

IpcMessageObjSize CancelQueryRequest::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, cancelStartTime_);
  result += packIntoBuffer(buffer, firstEscalationInterval_);
  result += packIntoBuffer(buffer, secondEscalationInterval_);
  result += packIntoBuffer(buffer, cancelEscalationSaveabend_);
  result += packIntoBuffer(buffer, commentLen_);
  str_cpy_all(buffer, comment_, commentLen_);
  buffer += commentLen_;
  result += commentLen_;
  result += packIntoBuffer(buffer, cancelLogging_);
  result += packIntoBuffer(buffer, cancelByPid_);
  result += packIntoBuffer(buffer, cancelPid_);
  result += packIntoBuffer(buffer, minimumAge_);

  return result;
}

void CancelQueryRequest::unpackObj(IpcMessageObjType objType,
				   IpcMessageObjVersion objVersion,
				   NABoolean sameEndianness,
				   IpcMessageObjSize objSize,
				   IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);

  unpackBuffer(buffer, cancelStartTime_);
  unpackBuffer(buffer, firstEscalationInterval_);
  unpackBuffer(buffer, secondEscalationInterval_);
  unpackBuffer(buffer, cancelEscalationSaveabend_);
  unpackBuffer(buffer, commentLen_);
  comment_ = new (getHeap()) char[commentLen_+1];
  str_cpy_all(comment_, buffer, commentLen_);
  comment_[commentLen_] = '\0';
  buffer += commentLen_;
  unpackBuffer(buffer, cancelLogging_);
  unpackBuffer(buffer, cancelByPid_);
  unpackBuffer(buffer, cancelPid_);
  unpackBuffer(buffer, minimumAge_);
}

IpcMessageObjSize ControlQueryReply::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(didAttemptControl_);
  return result;
}

IpcMessageObjSize ControlQueryReply::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, didAttemptControl_);
  return result;
}

void ControlQueryReply::unpackObj( IpcMessageObjType objType,
                                  IpcMessageObjVersion objVersion,
                                  NABoolean sameEndianness,
                                  IpcMessageObjSize objSize,
                                  IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);

  unpackBuffer(buffer, didAttemptControl_);
}

IpcMessageObjSize CancelQueryKillServersRequest::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(executionCount_);
  result += sizeof(master_);
  result += sizeof(cancelLogging_);
  result += sizeof(makeSaveabend_);
  return result;
}

IpcMessageObjSize CancelQueryKillServersRequest::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, executionCount_);
  result += packIntoBuffer(buffer, master_);
  result += packIntoBuffer(buffer, cancelLogging_);
  result += packIntoBuffer(buffer, makeSaveabend_);
  return result;
}

void CancelQueryKillServersRequest::unpackObj(IpcMessageObjType objType,
				   IpcMessageObjVersion objVersion,
				   NABoolean sameEndianness,
				   IpcMessageObjSize objSize,
				   IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);

  unpackBuffer(buffer, executionCount_);
  unpackBuffer(buffer, master_);
  unpackBuffer(buffer, cancelLogging_);
  unpackBuffer(buffer, makeSaveabend_);
}

IpcMessageObjSize SuspendActivateServersRequest::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(isRequestToSuspend_);
  result += sizeof(suspendLogging_);
  return result;
}

IpcMessageObjSize SuspendActivateServersRequest::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  result += packIntoBuffer(buffer, isRequestToSuspend_);
  result += packIntoBuffer(buffer, suspendLogging_);
  return result;
}

void SuspendActivateServersRequest::unpackObj(IpcMessageObjType objType,
				   IpcMessageObjVersion objVersion,
				   NABoolean sameEndianness,
				   IpcMessageObjSize objSize,
				   IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);

  unpackBuffer(buffer, isRequestToSuspend_);
  unpackBuffer(buffer, suspendLogging_);
}

SecInvalidKeyRequest::SecInvalidKeyRequest(NAMemory *heap,
                                           Int32 numSiks,
                                           SQL_QIKEY *sikPtr) :
    RtsMessageObj(SECURITY_INVALID_KEY_REQ,
                    CurrSecurityInvalidKeyVersionNumber, heap)
    , numSiks_(numSiks)
    , sikPtr_(NULL)
{
  if (numSiks_ > 0)
  {
    sikPtr_ = new(heap) SQL_QIKEY[numSiks_];
    memcpy((void *)sikPtr_, sikPtr, numSiks_*sizeof(SQL_QIKEY));
  }
}

SecInvalidKeyRequest::~SecInvalidKeyRequest()
{
  if (numSiks_ > 0)
  {
    NADELETEBASIC(sikPtr_, getHeap());
  }
  numSiks_ = 0;
  sikPtr_ = NULL;
}

IpcMessageObjSize SecInvalidKeyRequest::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(numSiks_);
  result += numSiks_ * sizeof(SQL_QIKEY);
  return result;
}

IpcMessageObjSize SecInvalidKeyRequest::packObjIntoMessage(
                           IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);
  result += packIntoBuffer(buffer, numSiks_);
  result += packStrIntoBuffer(buffer, (char *) sikPtr_,
                              numSiks_ * sizeof(SQL_QIKEY));
  return result;
}

void SecInvalidKeyRequest::unpackObj(IpcMessageObjType objType,
	       IpcMessageObjVersion objVersion,
	       NABoolean sameEndianness,
	       IpcMessageObjSize objSize,
	       IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);

  unpackBuffer(buffer, numSiks_);
  if (numSiks_ > 0)
  {
    sikPtr_ = new (getHeap()) SQL_QIKEY[numSiks_];
    unpackStrFromBuffer(buffer, (char *) sikPtr_, sizeof(SQL_QIKEY)*numSiks_);
  }
  else
    sikPtr_ = NULL;
}

LobLockRequest::LobLockRequest(NAMemory *heap,
                               char *lobLockId
                               ) :
    RtsMessageObj(LOB_LOCK_REQ,
                    CurrLobLockVersionNumber, heap)
{
  memcpy(lobLockId_,lobLockId,LOB_LOCK_ID_SIZE+1);
}

LobLockRequest::~LobLockRequest()
{
  memset(lobLockId_,0,LOB_LOCK_ID_SIZE+1); 
}

IpcMessageObjSize LobLockRequest::packedLength()
{
  IpcMessageObjSize result = baseClassPackedLength();
  result += sizeof(lobLockId_);
  return result;
}

IpcMessageObjSize LobLockRequest::packObjIntoMessage(
                           IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);
  result += packStrIntoBuffer(buffer, lobLockId_,LOB_LOCK_ID_SIZE+1);
  return result;
}

void LobLockRequest::unpackObj(IpcMessageObjType objType,
	       IpcMessageObjVersion objVersion,
	       NABoolean sameEndianness,
	       IpcMessageObjSize objSize,
	       IpcConstMessageBufferPtr buffer)
{
  unpackBaseClass(buffer);
  unpackStrFromBuffer(buffer,lobLockId_,LOB_LOCK_ID_SIZE+1);
}

