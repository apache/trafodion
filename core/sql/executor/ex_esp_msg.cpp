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
 *****************************************************************************
 *
 * File:         ex_esp_msg.cpp
 * Description:  Requests and replies exchanged between master executor and
 *               ESPs and dependent objects sent in them.
 *               
 * Created:      1/5/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Platform.h"
#include "ex_stdh.h"
#include "Ex_esp_msg.h"
#include "LateBindInfo.h"
#include "timeout_data.h"  
#include "NAUserId.h"
#include "ComRtUtils.h"

const char *getESPStreamTypeString(ESPMessageTypeEnum t)
{
  switch (t)
  {
    case IPC_MSG_SQLESP_CONTROL_REQUEST: return "IPC_MSG_SQLESP_CONTROL_REQUEST";
    case IPC_MSG_SQLESP_CONTROL_REPLY: return "IPC_MSG_SQLESP_CONTROL_REPLY";
    case IPC_MSG_SQLESP_DATA_REQUEST: return "IPC_MSG_SQLESP_DATA_REQUEST";
    case IPC_MSG_SQLESP_DATA_REPLY: return "IPC_MSG_SQLESP_DATA_REPLY";
    case IPC_MSG_SQLESP_CANCEL_REQUEST: return "IPC_MSG_SQLESP_CANCEL_REQUEST";
    case IPC_MSG_SQLESP_CANCEL_REPLY: return "IPC_MSG_SQLESP_CANCEL_REPLY";
    case IPC_MSG_SQLESP_SERVER_ROUTING: return "IPC_MSG_SQLESP_SERVER_ROUTING";
    case IPC_MSG_SQLESP_SERVER_INCOMING: return "IPC_MSG_SQLESP_SERVER_INCOMING";
    default: return ComRtGetUnknownString((Int32) t);
  }
}

const char *getESPMessageObjTypeString(ESPMessageObjTypeEnum t)
{
  switch (t)
  {
    case ESP_INPUT_DATA_HDR: return "ESP_INPUT_DATA_HDR";
    case ESP_CONTINUE_HDR: return "ESP_CONTINUE_HDR";
    case ESP_RETURN_DATA_HDR: return "ESP_RETURN_DATA_HDR";
    case ESP_LOAD_FRAGMENT_HDR: return "ESP_LOAD_FRAGMENT_HDR";
    case ESP_FIXUP_FRAGMENT_HDR: return "ESP_FIXUP_FRAGMENT_HDR";
    case ESP_RELEASE_FRAGMENT_HDR: return "ESP_RELEASE_FRAGMENT_HDR";
    case ESP_OPEN_HDR: return "ESP_OPEN_HDR";
    case ESP_PARTITION_INPUT_DATA_HDR: return "ESP_PARTITION_INPUT_DATA_HDR";
    case ESP_WORK_TRANSACTION_HDR: return "ESP_WORK_TRANSACTION_HDR";
    case ESP_RELEASE_TRANSACTION_HDR: return "ESP_RELEASE_TRANSACTION_HDR";
    case ESP_CANCEL_HDR: return "ESP_CANCEL_HDR";
    case ESP_LATE_CANCEL_HDR: return "ESP_LATE_CANCEL_HDR";
    case ESP_RETURN_STATUS_HDR: return "ESP_RETURN_STATUS_HDR";
    case ESP_RETURN_CANCEL_HDR: return "ESP_RETURN_CANCEL_HDR";
    case ESP_FRAGMENT_KEY: return "ESP_FRAGMENT_KEY";
    case ESP_INPUT_SQL_BUFFER: return "ESP_INPUT_SQL_BUFFER";
    case ESP_OUTPUT_SQL_BUFFER: return "ESP_OUTPUT_SQL_BUFFER";
    case ESP_PROCESS_IDS_OF_FRAG: return "ESP_PROCESS_IDS_OF_FRAG";
    case ESP_LATE_NAME_INFO: return "ESP_LATE_NAME_INFO";
    case ESP_DIAGNOSTICS_AREA: return "ESP_DIAGNOSTICS_AREA";
    case ESP_STATISTICS: return "ESP_STATISTICS";
    case ESP_FRAGMENT: return "ESP_FRAGMENT";
    case ESP_TRANSID: return "ESP_TRANSID";
    case ESP_TIMEOUT_DATA: return "ESP_TIMEOUT_DATA";
    case ESP_SM_DOWNLOAD_INFO: return "ESP_SM_DOWNLOAD_INFO";
    case ESP_RESOURCE_INFO: return "ESP_RESOURCE_INFO";
    case ESP_SECURITY_INFO: return "ESP_SECURITY_INFO";
    case IPC_SQL_STATS_AREA: return "IPC_SQL_STATS_AREA";
    default: return ComRtGetUnknownString((Int32) t);
  }
}

// -----------------------------------------------------------------------
// Methods for class ExEspMsgObj
// -----------------------------------------------------------------------

void ExEspMsgObj::operator delete(void *p)
{
  if (p)
    {
      ((ExEspMsgObj *) p)->heap_->deallocateMemory(p);
    }
}


IpcMessageObjSize ExEspMsgObj::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  // we get here if the class that is derived from ExEspMsgObj has not
  // defined its own packObjIntoMessage method (usually the class will
  // have defined the packedLength() method)

  IpcMessageObjSize result;
  NAMemory *savedHeap = heap_;

  // zero out heap field before packing the whole object into the message
  heap_ = NULL;
  result = IpcMessageObj::packObjIntoMessage(buffer);
  heap_ = savedHeap;
  return result;
}

void ExEspMsgObj::unpackObj(IpcMessageObjType objType,
			    IpcMessageObjVersion objVersion,
			    NABoolean sameEndianness,
			    IpcMessageObjSize objSize,
			    IpcConstMessageBufferPtr buffer)
{
  // similar to packObjIntoMessage, just make sure we don't use
  // the heap pointer that comes from another process
  NAMemory *savedHeap = heap_;

  IpcMessageObj::unpackObj(objType,objVersion,sameEndianness,objSize,buffer);
  heap_ = savedHeap;
}

IpcMessageRefCount ExEspMsgObj::decrRefCount()
{
  if (getRefCount() == 1)
    {
      // IpcMessageObj::decrRefCount() would delete the object by calling
      // global operator delete (since IpcMessageObj doesn't have an
      // operator delete). However, if we code the "delete this" statement
      // in this context we will pick up the correct operator delete.
      delete this;
      return 0;
    }
  else
    {
      // normal case, object won't be deleted
      return IpcMessageObj::decrRefCount();
    }
}

// -----------------------------------------------------------------------
// Methods for class ExEspRequestHeader
// -----------------------------------------------------------------------

ExEspRequestHeader::ExEspRequestHeader(ESPMessageObjTypeEnum objType,
				       IpcMessageObjVersion objVersion,
				       NAMemory *heap) :
     ExEspMsgObj(objType,objVersion,heap) {}

// constructor used to perform copyless receive, maps packed objects in place
ExEspRequestHeader::ExEspRequestHeader(IpcBufferedMsgStream* msgStream)
: ExEspMsgObj(msgStream)
  { }

// -----------------------------------------------------------------------
// Methods for class ExEspReplyHeader
// -----------------------------------------------------------------------

ExEspReplyHeader::ExEspReplyHeader(ESPMessageObjTypeEnum objType,
				   IpcMessageObjVersion objVersion,
				   NAMemory *heap) :
     ExEspMsgObj(objType,objVersion,heap) {}

// constructor used to perform copyless receive, maps packed objects in place
ExEspReplyHeader::ExEspReplyHeader(IpcBufferedMsgStream* msgStream)
: ExEspMsgObj(msgStream)
  { }

// -----------------------------------------------------------------------
// Methods for class ExEspInputDataReqHeader
// -----------------------------------------------------------------------

ExEspInputDataReqHeader::ExEspInputDataReqHeader(NAMemory *heap)
: ExEspRequestHeader(ESP_INPUT_DATA_HDR,CurrInputDataHdrVersion,heap),
  endianness_(IpcMyEndianness),
  spare1_(0),
  spare2_(0),
  injectErrorAtQueueFreq_(0),
  spare3_(0), spare4_(0), spare5_(0)
  {}

// constructor used to perform copyless receive, maps packed objects in place
ExEspInputDataReqHeader::ExEspInputDataReqHeader(
                                             IpcBufferedMsgStream* msgStream)
: ExEspRequestHeader(msgStream)
  {
  if (endianness_ != IpcMyEndianness)
    {
    ABORT("TO BE DONE: need to swap endianness!");
    endianness_ = IpcMyEndianness;
    }
  }

IpcMessageObjSize ExEspInputDataReqHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class ExEspContinueReqHeader
// -----------------------------------------------------------------------

ExEspContinueReqHeader::ExEspContinueReqHeader(NAMemory *heap)
: ExEspRequestHeader(ESP_CONTINUE_HDR,CurrContinueHdrVersion,heap),
  endianness_(IpcMyEndianness),
  spare1_(0),
  spare2_(0),
  spare3_(0)
  {}

// constructor used to perform copyless receive, maps packed objects in place
ExEspContinueReqHeader::ExEspContinueReqHeader(IpcBufferedMsgStream* msgStream)
: ExEspRequestHeader(msgStream)
  {
  if (endianness_ != IpcMyEndianness)
    {
    ABORT("TO BE DONE: need to swap endianness!");
    endianness_ = IpcMyEndianness;
    }
  }

IpcMessageObjSize ExEspContinueReqHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class ExEspReturnDataReplyHeader
// -----------------------------------------------------------------------

ExEspReturnDataReplyHeader::ExEspReturnDataReplyHeader(NAMemory *heap)
: ExEspReplyHeader(ESP_RETURN_DATA_HDR,CurrReturnDataHdrVersion,heap),
  stopSendingData_(FALSE),
  endianness_(IpcMyEndianness),
  spare1_(0),
  spare2_(0),
  spare3_(0),
  spare4_(0)
  {}

// constructor used to perform copyless receive, maps packed objects in place
ExEspReturnDataReplyHeader::ExEspReturnDataReplyHeader(
                                             IpcBufferedMsgStream* msgStream)
: ExEspReplyHeader(msgStream)
  {
  if (endianness_ != IpcMyEndianness)
    {
    ABORT("TO BE DONE: need to swap endianness!");
    endianness_ = IpcMyEndianness;
    }
  }

IpcMessageObjSize ExEspReturnDataReplyHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class ExEspLoadFragmentReqHeader
// -----------------------------------------------------------------------

ExEspLoadFragmentReqHeader::ExEspLoadFragmentReqHeader(NAMemory *heap) :
     ExEspRequestHeader(ESP_LOAD_FRAGMENT_HDR,
			CurrLoadFragmentHdrVersion,heap),
     spare1_(0),
     spare2_(0)
{}

IpcMessageObjSize ExEspLoadFragmentReqHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class ExEspFixupFragmentReqHeader
// -----------------------------------------------------------------------

ExEspFixupFragmentReqHeader::ExEspFixupFragmentReqHeader(NAMemory *heap) :
     ExEspRequestHeader(ESP_FIXUP_FRAGMENT_HDR,
			CurrFixupFragmentHdrVersion,heap),
     flags_(0),
     espExecutePriority_(0),
     espFixupPriority_(0),
     maxPollingInterval_(300),
     espFreeMemTimeout_(0)
{}

IpcMessageObjSize ExEspFixupFragmentReqHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class ExEspReleaseFragmentReqHeader
// -----------------------------------------------------------------------

ExEspReleaseFragmentReqHeader::ExEspReleaseFragmentReqHeader(NAMemory *heap) :
     ExEspRequestHeader(ESP_RELEASE_FRAGMENT_HDR,
			CurrReleaseFragmentHdrVersion,heap),
     idleTimeout_(0),
     flags_(0),
     detachFromMaster_(FALSE),
     spare1_(0),
     spare2_(0)
{}

IpcMessageObjSize ExEspReleaseFragmentReqHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class ExEspOpenReqHeader
// -----------------------------------------------------------------------

ExEspOpenReqHeader::ExEspOpenReqHeader(NAMemory *heap)
: ExEspRequestHeader(ESP_OPEN_HDR,CurrOpenHdrVersion,heap),
  endianness_(IpcMyEndianness),
  openType_((char)NORMAL),
  spare2_(0),
  statID_(0),
  spare3_(0),
  spare4_(0)
{}

// constructor used to perform copyless receive, maps packed objects in place
ExEspOpenReqHeader::ExEspOpenReqHeader(IpcBufferedMsgStream* msgStream)
: ExEspRequestHeader(msgStream)
  {
  if (endianness_ != IpcMyEndianness)
    {
    ABORT("TO BE DONE: need to swap endianness!");
    endianness_ = IpcMyEndianness;
    }
  }

IpcMessageObjSize ExEspOpenReqHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class ExEspPartInputDataReqHeader
// -----------------------------------------------------------------------

ExEspPartInputDataReqHeader::ExEspPartInputDataReqHeader(NAMemory *heap) :
     ExEspRequestHeader(ESP_PARTITION_INPUT_DATA_HDR,
			CurrPartitionInputDataHdrVersion,heap),
  staticAssignment_(FALSE),
  askForMoreWorkWhenDone_(FALSE),
  spare1_(0),
  spare2_(0)
{}

IpcMessageObjSize ExEspPartInputDataReqHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class ExEspWorkReqHeader
// -----------------------------------------------------------------------

ExEspWorkReqHeader::ExEspWorkReqHeader(NAMemory *heap) :
     ExEspRequestHeader(ESP_WORK_TRANSACTION_HDR,
			CurrWorkTransactionHdrVersion,heap),
     spare1_(0),
     spare2_(0)
{}

IpcMessageObjSize ExEspWorkReqHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class ExEspReleaseWorkReqHeader
// -----------------------------------------------------------------------

ExEspReleaseWorkReqHeader::ExEspReleaseWorkReqHeader(NAMemory *heap) :
     ExEspRequestHeader(ESP_RELEASE_TRANSACTION_HDR,
			CurrReleaseTransactionHdrVersion,heap),
     allWorkRequests_(FALSE),
     inactiveTimeout_(0),
     spare1_(0)
{}

IpcMessageObjSize ExEspReleaseWorkReqHeader::packedLength()
{
  return sizeof(*this);
}


// -----------------------------------------------------------------------
// Methods for class ExEspCancelReqHeader
// -----------------------------------------------------------------------

ExEspCancelReqHeader::ExEspCancelReqHeader(NAMemory *heap) :
      ExEspRequestHeader(ESP_CANCEL_HDR,CurrCancelHdrVersion,heap),
      endianness_(IpcMyEndianness),
      myInstanceNum_(0),
      spare1_(0),
      spare2_(0)
      {}

// constructor used to perform copyless receive, maps packed objects in place
ExEspCancelReqHeader::ExEspCancelReqHeader(IpcBufferedMsgStream* msgStream)
: ExEspRequestHeader(msgStream)
  {
  if (endianness_ != IpcMyEndianness)
    {
    ABORT("TO BE DONE: need to swap endianness!");
    endianness_ = IpcMyEndianness;
    }
  }

IpcMessageObjSize ExEspCancelReqHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class ExEspCancelReplyHeader
// -----------------------------------------------------------------------

ExEspCancelReplyHeader::ExEspCancelReplyHeader(NAMemory *heap) :
      ExEspReplyHeader(ESP_RETURN_CANCEL_HDR,
		       CurrCancelReplyHdrVersion,heap),
      spare1_(0),
      spare2_(0)
{}

// constructor used to perform copyless receive, maps packed objects in place
ExEspCancelReplyHeader::ExEspCancelReplyHeader(IpcBufferedMsgStream* msgStream)
: ExEspReplyHeader(msgStream) {}

IpcMessageObjSize ExEspCancelReplyHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class ExEspLateCancelReqHeader
// -----------------------------------------------------------------------

ExEspLateCancelReqHeader::ExEspLateCancelReqHeader(NAMemory *heap) :
      ExEspRequestHeader(ESP_LATE_CANCEL_HDR,CurrLateCancelHdrVersion,heap),
      myInstanceNum_(-1),
      spare1_(0),
      spare2_(0),
      spare3_(0),
      spare4_(0)
{}

// constructor used to perform copyless receive, maps packed objects in place
ExEspLateCancelReqHeader::ExEspLateCancelReqHeader(
     IpcBufferedMsgStream* msgStream)
: ExEspRequestHeader(msgStream)
{
  // if (endianness_ != IpcMyEndianness)
  //   ABORT("TO BE DONE: need to swap endianness!");
}

IpcMessageObjSize ExEspLateCancelReqHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class ExEspReturnStatusReplyHeader
// -----------------------------------------------------------------------

ExEspReturnStatusReplyHeader::ExEspReturnStatusReplyHeader(NAMemory *heap)
: ExEspReplyHeader(ESP_RETURN_STATUS_HDR,CurrReturnStatusHdrVersion,heap),
  endianness_(IpcMyEndianness),
  spare1_(0),
  spare2_(0),
  spare3_(0),
  spare4_(0)
  {}

// constructor used to perform copyless receive, maps packed objects in place
ExEspReturnStatusReplyHeader::ExEspReturnStatusReplyHeader(
                                             IpcBufferedMsgStream* msgStream)
: ExEspReplyHeader(msgStream)
  {
  if (endianness_ != IpcMyEndianness)
    {
    ABORT("TO BE DONE: need to swap endianness!");
    endianness_ = IpcMyEndianness;
    }
  }

IpcMessageObjSize ExEspReturnStatusReplyHeader::packedLength()
{
  return sizeof(*this);
}

// -----------------------------------------------------------------------
// Methods for class TupMsgBuffer
// -----------------------------------------------------------------------

TupMsgBuffer::TupMsgBuffer(Lng32 bufferLen,
			   InOut inOut,
			   NAMemory *heap) :
   ExEspMsgObj(
	inOut == MSG_IN ? ESP_INPUT_SQL_BUFFER : ESP_OUTPUT_SQL_BUFFER,
	inOut == MSG_IN ? CurrInputSqlBufferVersion :
	                  CurrOutputSqlBufferVersion,
	heap),
   filler64BitPtr_(0),
   endianness_(IpcMyEndianness),
   spare1_(0),
   spare2_(0),
   spare3_(0),
   spare4_(0)
{
  theBuffer_ = (SqlBuffer *) new(getHeap()) char [bufferLen];
  theBuffer_->driveInit(bufferLen, FALSE, SqlBuffer::NORMAL_);
  allocSize_ = bufferLen;
}

///////////////////////////////////////////////////////////////////////////////
// Constructor used to create a packed send message object.
TupMsgBuffer::TupMsgBuffer(Lng32 bufferLen,
                           InOut inOut,
                           IpcBufferedMsgStream* msgStream)
: ExEspMsgObj(
     inOut == MSG_IN ? ESP_INPUT_SQL_BUFFER : ESP_OUTPUT_SQL_BUFFER,
     inOut == MSG_IN ? CurrInputSqlBufferVersion : CurrOutputSqlBufferVersion,
     NULL),
   endianness_(IpcMyEndianness),
   spare1_(0),
   spare2_(0)
  {
  // IpcBufferedMsgStream parm used to differentiate from constructor
  // using heap
  ex_assert(msgStream, "Must have msgStream"); 

  // the SqlBuffer starts at the next address that is aligned at
  // an 8 byte boundary
  IpcMessageBufferPtr bufferStart = (IpcMessageBufferPtr) this;
  bufferStart += sizeof(*this);
  alignBufferForNextObj(bufferStart);

  theBuffer_ = (SqlBuffer *) (bufferStart);
  theBuffer_->driveInit(bufferLen, FALSE, SqlBuffer::NORMAL_);
  allocSize_ = bufferLen;
  }

///////////////////////////////////////////////////////////////////////////////       
// constructor used to perform copyless receive. unpacks objects in place.
TupMsgBuffer::TupMsgBuffer(IpcBufferedMsgStream* msgStream)
: ExEspMsgObj(msgStream) 
  { 
  // IpcBufferedMsgStream parm used to differentiate from default constructor
  ex_assert(msgStream, "Must have msgStream f. receive"); 

  // the SqlBuffer starts at the next address that is aligned at
  // an 8 byte boundary
  IpcMessageBufferPtr bufferStart = (IpcMessageBufferPtr) this;
  bufferStart += sizeof(*this);
  alignBufferForNextObj(bufferStart);

  theBuffer_ = (SqlBuffer *) (bufferStart);

  if (endianness_ != IpcMyEndianness)
    {
    ABORT("TO BE DONE: need to swap endianness in sql buffer!");
    endianness_ = IpcMyEndianness;
    }
  // convert offsets in buffer to pointers,(need to check if already done!)
  theBuffer_->driveUnpack();
  }

TupMsgBuffer::~TupMsgBuffer()
{
  getHeap()->deallocateMemory(theBuffer_);
  allocSize_ = 0;
}

IpcMessageObjSize TupMsgBuffer::packedLength()
{
  // NOTE: this number is valid only for use with a "regular" message
  // stream, not with a buffered message stream using copyless IPC
  return baseClassPackedLength() + theBuffer_->get_buffer_size();
}

IpcMessageObjSize TupMsgBuffer::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  // move the IpcMessageObj data members into the buffer
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);

  // convert all pointers in the buffer to offsets
  theBuffer_->drivePack();

  // now move the SqlBuffer
  str_cpy_all(buffer,(char *) theBuffer_, theBuffer_->get_buffer_size());

  result += theBuffer_->get_buffer_size();

  return result;
}

void TupMsgBuffer::unpackObj(IpcMessageObjType objType,
			     IpcMessageObjVersion objVersion,
			     NABoolean sameEndianness,
			     IpcMessageObjSize objSize,
			     IpcConstMessageBufferPtr buffer)
{
  ex_assert((objType == ESP_INPUT_SQL_BUFFER OR
	     objType == ESP_OUTPUT_SQL_BUFFER) AND
	    objVersion == 100 AND
	    sameEndianness AND
	    objSize > sizeof(IpcMessageObj),
	    "invalid type or version for TupMsgBuffer::unpackObj()");

  unpackBaseClass(buffer);

  IpcMessageObjSize sqlBufferSize = objSize - sizeof(IpcMessageObj);

  ex_assert(allocSize_ >= (Lng32) sqlBufferSize,
	    "TupMsgBuffer too small for unpack");

  str_cpy_all((char *) theBuffer_, buffer, sqlBufferSize);

  // convert offsets in buffer to pointers
  theBuffer_->driveUnpack();

  // just checking whether we really got the right size info
  ex_assert(sqlBufferSize == theBuffer_->get_buffer_size(),
	    "Buffer size mismatch");
}

// -----------------------------------------------------------------------
// Methods for class ExProcessIdsOfFrag
// -----------------------------------------------------------------------

ExProcessIdsOfFrag::ExProcessIdsOfFrag(NAMemory *heap, ExFragId fragId) :
     ExEspMsgObj(ESP_PROCESS_IDS_OF_FRAG,CurrProcessIdsOfFragVersion,heap),
     fragmentId_(fragId),
     spare_(0),
     processIds_(heap)
{
}

IpcMessageObjSize ExProcessIdsOfFrag::packedLength()
{
  // we pack the fragment id, number of entries, and then all the process ids
  IpcMessageObjSize result = baseClassPackedLength() + 
    sizeof(spare_) + sizeof(fragmentId_) + sizeof(Int32) /* num entries */;

  CollIndex np = processIds_.entries();
  for (CollIndex i = 0; i < np; i++)
    {
      alignSizeForNextObj(result);
      result += processIds_[i].packedLength();
    }

  return result;
}

IpcMessageObjSize ExProcessIdsOfFrag::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);
  Int32 np = processIds_.entries();

  // pack fragment id and number of entries
  str_cpy_all(buffer, (const char *) &spare_, sizeof(spare_));
  result += sizeof(spare_);
  buffer += sizeof(spare_);
  str_cpy_all(buffer, (const char *) &fragmentId_, sizeof(fragmentId_));
  result += sizeof(fragmentId_);
  buffer += sizeof(fragmentId_);
  str_cpy_all(buffer, (const char *) &np, sizeof(np));
  result += sizeof(np);
  buffer += sizeof(np);

  // pack each process id, as an IpcMessageObj
  for (CollIndex i = 0; i < (CollIndex) np; i++)
    {
      alignSizeForNextObj(result);
      alignBufferForNextObj(buffer);
      IpcMessageObjSize pidsize =
	processIds_[i].packDependentObjIntoMessage(buffer);
      result += pidsize;
      buffer += pidsize;
    }

  return result;
}

void ExProcessIdsOfFrag::unpackObj(IpcMessageObjType objType,
				   IpcMessageObjVersion objVersion,
				   NABoolean sameEndianness,
				   IpcMessageObjSize objSize,
				   IpcConstMessageBufferPtr buffer)
{
  ex_assert(objType == ESP_PROCESS_IDS_OF_FRAG AND
	    objVersion == CurrProcessIdsOfFragVersion AND
	    sameEndianness AND
	    objSize >= baseClassPackedLength() +
	       sizeof(fragmentId_) + sizeof(CollIndex),
            "Received invalid ProcessIdsOfFrag object");

  unpackBaseClass(buffer);

  // unpack fragment id and number of entries
  Int32 np;
  buffer += sizeof(spare_);
  str_cpy_all((char *) &fragmentId_, buffer, sizeof(fragmentId_));
  buffer += sizeof(fragmentId_);
  str_cpy_all((char *) &np, buffer, sizeof(np));
  buffer += sizeof(np);

  // unpack the process ids
  for (CollIndex i = 0; i < (CollIndex) np; i++)
    {
      alignBufferForNextObj(buffer);
      processIds_.insert(IpcProcessId());
      processIds_[i].unpackDependentObjFromBuffer(buffer,sameEndianness);
    }
}

// -----------------------------------------------------------------------
// Methods for class ExProcessIdsOfFragList
// -----------------------------------------------------------------------

ExProcessIdsOfFrag * ExProcessIdsOfFragList::findEntry(
     ExFragId fragId) const
{
  for (CollIndex i = 0; i < entries(); i++)
    if (at(i)->getFragId() == fragId) return at(i);
  return NULL;
}

// -----------------------------------------------------------------------
// Methods for class ExMsgFragment
// -----------------------------------------------------------------------

ExMsgFragment::ExMsgFragment(NAMemory *heap) :
     ExEspMsgObj(ESP_FRAGMENT,CurrFragmentVersion,heap)
{
  fragment_           = NULL;
  f_.fragType_        = 0;
  f_.parentId_        = 0;
  f_.topNodeOffset_   = 0;
  f_.fragmentLength_  = 0;
  f_.numTemps_        = 0;
  f_.needsTransaction_= FALSE;
  f_.iOwnTheFragment_ = FALSE;
  f_.displayInGui_    = FALSE;
  f_.mxvOfOriginator_ = 0;
  f_.planVersion_     = 0;
  f_.injectErrorAtExprFreq_ = 0;
  f_.queryId_ = NULL;
  f_.queryIdLen_ = 0;
  f_.compressedLength_ = 0;
  f_.userID_ = NA_UserIdDefault;
  f_.userName_ = NULL;
  for (Int32 i = 0; i < 6; i++)
    f_.reserved[i] = 0;
}

ExMsgFragment::ExMsgFragment(
     const ExFragKey                &key,
     ExFragDir::ExFragEntryType      fragType,
     ExFragId                        parentId,
     Lng32                           topNodeOffset,
     IpcMessageObjSize               fragmentLength,
     char                           *fragment,
     Lng32                           numTemps,
     unsigned short                  mxvOfOriginator,
     unsigned short                  planVersion,
     NABoolean                       needsTransaction,
     ULng32                          injectErrorAtExprFreq,
     NAMemory                       *heap,
     NABoolean                       takeOwnership,
     NABoolean                       displayInGui,
     const char                     *queryId,
     Lng32                           queryIdLen,
     Lng32                           userID,
     const char                     *userName,
     Lng32                           userNameLen,
     IpcMessageObjSize               compressedLength) :
     ExEspMsgObj(ESP_FRAGMENT,CurrFragmentVersion,heap)
{
  key_                = key;
  fragment_           = fragment;
  f_.fragType_        = fragType;
  f_.parentId_        = parentId;
  f_.topNodeOffset_   = topNodeOffset;
  f_.fragmentLength_  = fragmentLength;
  f_.numTemps_        = numTemps;
  f_.mxvOfOriginator_ = mxvOfOriginator;
  f_.planVersion_     = planVersion;
  f_.needsTransaction_= needsTransaction;
  f_.iOwnTheFragment_ = takeOwnership;
  f_.displayInGui_    = (ExIpcMsgBoolean) displayInGui;
  f_.injectErrorAtExprFreq_ = injectErrorAtExprFreq;
#ifdef _DEBUG
  if (getenv("TEST_ERROR_AT_EXPR_NO_ESP"))
    f_.injectErrorAtExprFreq_ = 0;
#endif

  if (queryId != NULL)
  {
    f_.queryId_ = new (getHeap()) char[queryIdLen+1];
    str_cpy_all(f_.queryId_, queryId, queryIdLen);
    f_.queryId_[queryIdLen] = '\0';
    f_.queryIdLen_ = queryIdLen;
  }
  else
  {
    f_.queryId_ = NULL;
    f_.queryIdLen_ = 0;
  }

  f_.userID_ = userID;
  if (userName)
  {
    f_.userName_ = new (getHeap()) char[userNameLen];
    str_cpy_all(f_.userName_, userName, userNameLen);
    f_.userNameLen_ = userNameLen;
  }
  else
  {
    f_.userName_ = NULL;
    f_.userNameLen_ = 0;
  }

  f_.compressedLength_ = compressedLength;

  for (Int32 i = 0; i < 6; i++)
    f_.reserved[i] = 0;
}
			     
ExMsgFragment::~ExMsgFragment()
{
  if (f_.queryId_ != NULL)
    getHeap()->deallocateMemory(f_.queryId_);
  if (f_.userName_ != NULL)
    getHeap()->deallocateMemory(f_.userName_);
  if (f_.iOwnTheFragment_)
    getHeap()->deallocateMemory(fragment_);
}

IpcMessageObjSize ExMsgFragment::packedLength()
{
  if (f_.compressedLength_ == 0)
    f_.compressedLength_ = str_compress_size(fragment_, f_.fragmentLength_);

  // Packed format:
  // 
  //   base class
  //   f_
  //   query ID
  //   user name
  //   fragment
  //   key
  //
  // Each component is aligned on an 8-byte boundary 

  IpcMessageObjSize result = baseClassPackedLength();
  alignSizeForNextObj(result);
 
  result += sizeof(f_);
  alignSizeForNextObj(result);

  if (f_.queryId_ != NULL)
    result += f_.queryIdLen_;
  alignSizeForNextObj(result);

  if (f_.userName_ != NULL)
    result += f_.userNameLen_;
  alignSizeForNextObj(result);

  result += MINOF(f_.compressedLength_,f_.fragmentLength_);
  alignSizeForNextObj(result);

  result += key_.packedLength();

  return result;
}

IpcMessageObjSize ExMsgFragment::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  // See description of packed format in packedLength() method

  IpcMessageBufferPtr start = buffer;

  // Pack the base class
  packBaseClassIntoMessage(buffer);
  alignBufferForNextObj(buffer);

  // Pack the fixed length fields
  str_cpy_all(buffer, (const char *) &f_, sizeof(f_));
  buffer += sizeof(f_);
  alignBufferForNextObj(buffer);

  // Pack the query ID
  if (f_.queryId_ != NULL)
  {
    str_cpy_all(buffer, f_.queryId_, f_.queryIdLen_);
    buffer += f_.queryIdLen_;
    alignBufferForNextObj(buffer);
  }

  // Pack the user name
  if (f_.userName_ != NULL)
  {
    str_cpy_all(buffer, f_.userName_, f_.userNameLen_);
    buffer += f_.userNameLen_;
    alignBufferForNextObj(buffer);
  }

  // Pack the fragment itself
  if (f_.compressedLength_ > 0 && f_.compressedLength_ < f_.fragmentLength_)
  {
    size_t cmprSize = str_compress(buffer,fragment_,f_.fragmentLength_);
    ex_assert(cmprSize == f_.compressedLength_,
              "Error during compress a fragment for download");
    buffer += f_.compressedLength_;

  }
  else
  {
    str_cpy_all(buffer,fragment_,f_.fragmentLength_);
    buffer += f_.fragmentLength_;
  }

  alignBufferForNextObj(buffer);

  // Pack the fragment key (is an IpcMessageObj itself)
  buffer += key_.packDependentObjIntoMessage(buffer);

  return (IpcMessageObjSize) (buffer - start);
}

void ExMsgFragment::unpackObj(IpcMessageObjType objType,
			      IpcMessageObjVersion objVersion,
			      NABoolean sameEndianness,
			      IpcMessageObjSize objSize,
			      IpcConstMessageBufferPtr buffer)
{
  // See description of packed format in packedLength() method

  // get rid of existing data members
  if (fragment_ AND f_.iOwnTheFragment_)
    getHeap()->deallocateMemory(fragment_);

  ex_assert(objType == ESP_FRAGMENT AND
	    objVersion == CurrFragmentVersion AND
	    sameEndianness,
	    "Invalid object presented to ExMsgFragment::unpackObj()");

  // Unpack the base class
  unpackBaseClass(buffer);
  alignBufferForNextObj(buffer);

// Before copying from buffer into f_, first release any 
// memory pointed to by f_. Currently the only memory 
// pointed to is f_.userName_.
  if (f_.userName_ != NULL)
  {
    getHeap()->deallocateMemory(f_.userName_);
    f_.userName_ = NULL;
  }

  // Unpack the fixed length field fields
  str_cpy_all((char *) &f_,buffer,sizeof(f_));
  buffer += sizeof(f_);
  alignBufferForNextObj(buffer);

  // Unpack the query ID. If a query ID is present in this request,
  // f_.queryId_ will contain an address from the sending process.
  if (f_.queryId_ != NULL)
  {
    f_.queryId_ = new (getHeap()) char[f_.queryIdLen_ + 1];
    str_cpy_all(f_.queryId_, buffer, f_.queryIdLen_);
    f_.queryId_[f_.queryIdLen_] = '\0';
    buffer += f_.queryIdLen_;
    alignBufferForNextObj(buffer);
  }

  // Unpack the user name. If a user name is present in this request,
  // f_.userName_ will contain an address from the sending process.
  if (f_.userName_ != NULL)
  {
    f_.userName_ = new (getHeap()) char[f_.userNameLen_];
    str_cpy_all(f_.userName_, buffer, f_.userNameLen_);
    buffer += f_.userNameLen_;
    alignBufferForNextObj(buffer);
  }

  // Unpack the fragment itself
  fragment_ = new(getHeap()) char[f_.fragmentLength_];
  f_.iOwnTheFragment_ = TRUE;
  if (f_.compressedLength_ > 0 && f_.compressedLength_ < f_.fragmentLength_)
  {
    size_t decpSize = str_decompress(fragment_,buffer,f_.compressedLength_);
    ex_assert(decpSize == f_.fragmentLength_,
              "Error during uncompress a downloaded fragment");
    buffer += f_.compressedLength_;
  }
  else
  {
    str_cpy_all(fragment_, buffer,f_.fragmentLength_);
    buffer += f_.fragmentLength_;
  }

  alignBufferForNextObj(buffer);

  // Unpack the fragment key
  key_.unpackDependentObjFromBuffer(buffer,sameEndianness);

  ex_assert(objSize == packedLength(),
           "Error during unpacking a downloaded fragment");
}

// -----------------------------------------------------------------------
// Methods for class ExMsgTransId
// -----------------------------------------------------------------------

ExMsgTransId::ExMsgTransId(NAMemory *heap,Int64 tid, short *phandle) :
     ExEspMsgObj(ESP_TRANSID,CurrTransidVersion,heap),
     localTransId_(tid),
     originatingTransId_(tid),
     spare2_(0)
{
}

ExMsgTransId::~ExMsgTransId() {}

IpcMessageObjSize ExMsgTransId::packedLength()
{
  return sizeof(*this);
}



// -----------------------------------------------------------------------
// Methods for class ExMsgTimeoutData
// -----------------------------------------------------------------------

ExMsgTimeoutData::ExMsgTimeoutData(TimeoutData *toData,
				   NAMemory *heap) :
  ExEspMsgObj(ESP_TIMEOUT_DATA,CurrTimeoutDataVersion, heap),
  timeoutData_(toData)
{
  iOwnTD_ = ( toData == NULL ) ; // NULL means: I'll allocate it later
  ex_assert( heap , "Bad heap given to CTOR of ExMsgTimeoutData" );
}

ExMsgTimeoutData::~ExMsgTimeoutData() 
{
  if ( iOwnTD_ && timeoutData_ ) getHeap()->deallocateMemory(timeoutData_);
}

IpcMessageObjSize ExMsgTimeoutData::packedLength()
{
  return baseClassPackedLength() +  timeoutData_->packedLength() ;
}

IpcMessageObjSize 
ExMsgTimeoutData::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  IpcMessageBufferPtr start = buffer;

  packBaseClassIntoMessage(buffer);

  timeoutData_->packIntoBuffer(buffer);  // increment the pointer "buffer"

  return (IpcMessageObjSize) (buffer - start);
}

void ExMsgTimeoutData::unpackObj(IpcMessageObjType objType,
				 IpcMessageObjVersion objVersion,
				 NABoolean sameEndianness,
				 IpcMessageObjSize objSize,
				 IpcConstMessageBufferPtr buffer)
{
  IpcConstMessageBufferPtr start = buffer;

  ex_assert(objType == ESP_TIMEOUT_DATA AND
	    objVersion == 100 AND
	    sameEndianness AND
	    objSize > sizeof(IpcMessageObj),
	    "invalid type or version for ExMsgTimeoutData::unpackObj()");

  unpackBaseClass(buffer);
  
  timeoutData_ = new ( getHeap() ) TimeoutData( getHeap() ) ;
  
  timeoutData_->unpackObj(buffer);

  iOwnTD_ = TRUE;  // on the ESP side; I own the newly allocated timeoutData_
}




// -----------------------------------------------------------------------
// Methods for class ExSMDownloadInfo
// -----------------------------------------------------------------------

// Constructor to send a message
ExSMDownloadInfo::ExSMDownloadInfo(NAMemory *heap,
                                   Int64 smQueryID,
                                   Int32 smTraceLevel,
                                   const char *smTraceFilePrefix,
                                   Int32 flags)
  : ExEspMsgObj(ESP_SM_DOWNLOAD_INFO, CurrSMDownloadInfoVersion, heap),
    smQueryID_(smQueryID),
    smTraceLevel_(smTraceLevel),
    smTraceFilePrefix_(NULL),
    flags_(flags)
{
  ex_assert(heap, "Bad heap given to CTOR of ExSMDownloadInfo");

  if (smTraceFilePrefix && smTraceFilePrefix != 0)
  {
    UInt32 len = str_len(smTraceFilePrefix);
    smTraceFilePrefix_ = (char *) getHeap()->allocateMemory(len + 1);
    str_cpy_all(smTraceFilePrefix_, smTraceFilePrefix, len + 1);
  }
}

// Constructor to receive a message
ExSMDownloadInfo::ExSMDownloadInfo(NAMemory *heap)
  : ExEspMsgObj(ESP_SM_DOWNLOAD_INFO, CurrSMDownloadInfoVersion, heap),
    smQueryID_(0),
    smTraceLevel_(0),
    smTraceFilePrefix_(NULL),
    flags_(0)
{
  ex_assert(heap, "Bad heap given to CTOR of ExSMDownloadInfo");
}

ExSMDownloadInfo::~ExSMDownloadInfo()
{
  if (smTraceFilePrefix_)
    getHeap()->deallocateMemory(smTraceFilePrefix_);
}

IpcMessageObjSize ExSMDownloadInfo::packedLength()
{
  UInt32 len = (smTraceFilePrefix_ ? (str_len(smTraceFilePrefix_) + 1) : 0);

  return baseClassPackedLength()
    + sizeof(Int64)   // smQueryID_
    + sizeof(Int32)   // smTraceLevel_
    + sizeof(Int32)   // flags_
    + sizeof(UInt32)  // length of smTraceFilePrefix_
    + len;            // smTraceFilePrefix_
}

IpcMessageObjSize 
ExSMDownloadInfo::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize size = packBaseClassIntoMessage(buffer);
  size += packIntoBuffer(buffer, smQueryID_);
  size += packIntoBuffer(buffer, smTraceLevel_);
  size += packIntoBuffer(buffer, flags_);
  size += packCharStarIntoBuffer(buffer, smTraceFilePrefix_);
  return size;
}

void ExSMDownloadInfo::unpackObj(IpcMessageObjType objType,
                                   IpcMessageObjVersion objVersion,
                                   NABoolean sameEndianness,
                                   IpcMessageObjSize objSize,
                                   IpcConstMessageBufferPtr buffer)
{
  ex_assert(objType == ESP_SM_DOWNLOAD_INFO &&
	    objVersion == 100 &&
	    sameEndianness &&
	    objSize > sizeof(IpcMessageObj),
	    "Invalid type or version for ExSMDownloadInfo::unpackObj()");

  unpackBaseClass(buffer);
  unpackBuffer(buffer, smQueryID_);
  unpackBuffer(buffer, smTraceLevel_);
  unpackBuffer(buffer, flags_);
  unpackBuffer(buffer, smTraceFilePrefix_, getHeap());
}

// -----------------------------------------------------------------------
// Methods for class ExMsgResourceInfo
// -----------------------------------------------------------------------

ExMsgResourceInfo::ExMsgResourceInfo(
     const ExScratchFileOptions *sfo,
     NAMemory *heap) :
     ExEspMsgObj(ESP_RESOURCE_INFO,CurrResourceInfoVersion,heap),
     sfo_(sfo),
     spare_(0)
{
  if (sfo_)
    totalNameLength_ = sfo_->ipcGetTotalNameLength();
  else
    totalNameLength_ = 0;
  bufferForDependentObjects_ = NULL;
}

ExMsgResourceInfo::~ExMsgResourceInfo() {}

IpcMessageObjSize ExMsgResourceInfo::packedLength()
{
  return baseClassPackedLength() +
    sfo_->ipcPackedLength() + sizeof(totalNameLength_) + sizeof(spare_);
}

IpcMessageObjSize ExMsgResourceInfo::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageBufferPtr start = buffer;

  packBaseClassIntoMessage(buffer);

  str_cpy_all(buffer,(char *) &totalNameLength_, sizeof(totalNameLength_));
  buffer += sizeof(totalNameLength_);
  str_cpy_all(buffer,(char *) &spare_, sizeof(spare_));
  buffer += sizeof(spare_);
  buffer += sfo_->ipcPackObjIntoMessage(buffer);

  return (IpcMessageObjSize) (buffer - start);
}

void ExMsgResourceInfo::unpackObj(IpcMessageObjType objType,
				  IpcMessageObjVersion objVersion,
				  NABoolean sameEndianness,
				  IpcMessageObjSize objSize,
				  IpcConstMessageBufferPtr buffer)
{
  ExScratchFileOptions *sfo;
  IpcConstMessageBufferPtr start = buffer;

  ex_assert(objType == ESP_RESOURCE_INFO AND
	    objVersion == 100 AND
	    sameEndianness AND
	    objSize > sizeof(IpcMessageObj),
	    "invalid type or version for ExMsgResourceInfo::unpackObj()");
  unpackBaseClass(buffer);
  str_cpy_all((char *) &totalNameLength_,buffer,sizeof(totalNameLength_));
  buffer += sizeof(totalNameLength_);
  buffer += sizeof(spare_);
  if (bufferForDependentObjects_)
    {
      getHeap()->deallocateMemory(bufferForDependentObjects_);
      bufferForDependentObjects_ = NULL;
    }
  sfo = new(getHeap()) ExScratchFileOptions;

  // sfo_ is const
  sfo_ = sfo;

  sfo->ipcUnpackObj(objSize - (buffer-start),
		    buffer,
		    getHeap(),
		    totalNameLength_,
		    bufferForDependentObjects_);
}

// Methods for ExMsgSecurityInfo

// constructor
ExMsgSecurityInfo::ExMsgSecurityInfo(NAMemory *heap) :
  ExEspMsgObj(ESP_SECURITY_INFO, CurrTimeoutDataVersion, heap)
{
  securityKey_ = NULL;
  authid_ = NULL;
}

// destructor. 
ExMsgSecurityInfo::~ExMsgSecurityInfo() 
{
  if (getHeap()) // members were allocated on this heap
  {
    getHeap()->deallocateMemory((void *)securityKey_);
    getHeap()->deallocateMemory(authid_);
    securityKey_ = NULL;
    authid_ = NULL;
  }
}

// The sender of this object (sendTop node in consumer) uses
// buffered stream, whose packing function will pack the base class 
// (IpcMessageObj - 32 bytes). So, do not pack the base class here. 
// However, since the recipient unpacks it in the unbuffered stream,
// it also unpacks the base class. Therefore, add the base class size
// to the total pack size. 
IpcMessageObjSize ExMsgSecurityInfo::packObjIntoMessage(char* buffer) 
{
  IpcMessageObjSize size = baseClassPackedLength();
  size += packCharStarIntoBuffer(buffer,securityKey_);
  size += packCharStarIntoBuffer(buffer,authid_);
  return size;
}

// The recipient of this object (connectionStream's actOnReceive) 
// will unpack this object in unbuffered stream, whose unpacking
// function would have already unpacked the base class before getting
// here. So, do not unpack the base class here. 
void ExMsgSecurityInfo::unpackObj(IpcMessageObjType objType,
                                  IpcMessageObjVersion objVersion,
                                  NABoolean sameEndianness,
		                  IpcMessageObjSize objSize,
				  IpcConstMessageBufferPtr buffer)
{
  buffer += sizeof(IpcMessageObj);
  unpackBuffer(buffer,securityKey_, getHeap());
  unpackBuffer(buffer,authid_, getHeap());
}

// Since packed object includes sizes of each string as prefix to the string itself,
// add 4 bytes for each char string packed. 
IpcMessageObjSize ExMsgSecurityInfo::packedLength()
{
  return baseClassPackedLength() + strlen(securityKey_) + 1 + sizeof(Lng32) +
                                   strlen(authid_) + 1 + sizeof(Lng32);
}

