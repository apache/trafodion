/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2015 Hewlett-Packard Development Company, L.P.
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
 *****************************************************************************
 *
 * File:         UdrExeIpc.cpp
 * Description:  IPC streams and message objects shared by the SQL/MX
 *               executor and the UDR server
 *
 * Created:      08/20/2000
 * Language:     C++
 *
 *****************************************************************************
 */

#include <iostream>

#include "ComTransInfo.h"
#include "IpcMessageObj.h"
#include "ComplexObject.h"
#include "CliMsgObj.h"
#include "UdrExeIpc.h"
#include "sql_buffer.h"
#include "ExCextdecs.h"
#include "ComRtUtils.h"


//----------------------------------------------------------------------
// UDR debugging code. See comments in UdrExeIpc.h.
//----------------------------------------------------------------------
#ifdef UDR_DEBUG
#include <stdarg.h>
void UdrPrintf(FILE *f, const char *formatString, ...)
{
  if (f)
  {
    va_list args;
    va_start(args, formatString);
    fprintf(f, "[UDR] ");
    vfprintf(f, formatString, args);
    fprintf(f, "\n");
    fflush(f);
  }
}
#endif

const char *GetUdrIpcTypeString(UdrIpcObjectType t)
{
  switch (t)
  {
    case UDR_MSG_LOAD:              return "UDR_MSG_LOAD";
    case UDR_MSG_LOAD_REPLY:        return "UDR_MSG_LOAD_REPLY";
    case UDR_MSG_UNLOAD:            return "UDR_MSG_UNLOAD";
    case UDR_MSG_UNLOAD_REPLY:      return "UDR_MSG_UNLOAD_REPLY";
    case UDR_MSG_DATA_HEADER:       return "UDR_MSG_DATA_HEADER";
    case UDR_MSG_DATA_REQUEST:      return "UDR_MSG_DATA_REQUEST";
    case UDR_MSG_DATA_REPLY:        return "UDR_MSG_DATA_REPLY";
    case UDR_MSG_CONTINUE_REQUEST:  return "UDR_MSG_CONTINUE_REQUEST";
    case UDR_MSG_ERROR_REPLY:       return "UDR_MSG_ERROR_REPLY";
    case UDR_MSG_SESSION:           return "UDR_MSG_SESSION";
    case UDR_MSG_SESSION_REPLY:     return "UDR_MSG_SESSION_REPLY";
    case UDR_MSG_ENTER_TX:          return "UDR_MSG_ENTER_TX";
    case UDR_MSG_ENTER_TX_REPLY:    return "UDR_MSG_ENTER_TX_REPLY";
    case UDR_MSG_EXIT_TX:           return "UDR_MSG_EXIT_TX";
    case UDR_MSG_EXIT_TX_REPLY:     return "UDR_MSG_EXIT_TX_REPLY";
    case UDR_MSG_RS_LOAD:           return "UDR_MSG_RS_LOAD";
    case UDR_MSG_RS_LOAD_REPLY:     return "UDR_MSG_RS_LOAD_REPLY";
    case UDR_MSG_RS_DATA_HEADER:    return "UDR_MSG_RS_DATA_HEADER";
    case UDR_MSG_RS_CONTINUE:       return "UDR_MSG_RS_CONTINUE";
    case UDR_MSG_RS_CLOSE:          return "UDR_MSG_RS_CLOSE";
    case UDR_MSG_RS_CLOSE_REPLY:    return "UDR_MSG_RS_CLOSE_REPLY";
    case UDR_MSG_RS_UNLOAD:         return "UDR_MSG_RS_UNLOAD";
    case UDR_MSG_RS_UNLOAD_REPLY:   return "UDR_MSG_RS_UNLOAD_REPLY";
    case UDR_MSG_RS_INFO:           return "UDR_MSG_RS_INFO";
    case UDR_MSG_SUSPEND_TX:        return "UDR_MSG_SUSPEND_TX";
    case UDR_MSG_SUSPEND_TX_REPLY:  return "UDR_MSG_SUSPEND_TX_REPLY";
    case UDR_STREAM_CLIENT_CONTROL: return "UDR_STREAM_CLIENT_CONTROL";
    case UDR_STREAM_SERVER_CONTROL: return "UDR_STREAM_SERVER_CONTROL";
    case UDR_STREAM_CLIENT_DATA:    return "UDR_STREAM_CLIENT_DATA";
    case UDR_STREAM_SERVER_DATA:    return "UDR_STREAM_SERVER_DATA";
    case UDR_STREAM_SERVER_REPLY:   return "UDR_STREAM_SERVER_REPLY";
    case UDR_IPC_INVALID:           return "UDR_IPC_INVALID";
    default:                        return ComRtGetUnknownString((Int32) t);
  }
}

//
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
  result = sizeof(Lng32);
  if (s)
  {
    result += str_len(s) + 1;
  }
  return result;
}

// -----------------------------------------------------------------------
// class UdrParameterInfo
// -----------------------------------------------------------------------
UdrParameterInfo::UdrParameterInfo(ComUInt32 position,
                                   ComUInt32 flags,
                                   ComSInt16 fsType,
                                   ComSInt16 ansiType,
                                   ComUInt16 paramNameLen,
                                   const char *paramName,
                                   ComUInt16 prec,
                                   ComUInt16 scale,
                                   ComUInt16 encodingCharSet,
                                   ComUInt16 collation,
                                   ComUInt32 dataLength,
                                   ComSInt16 nullIndicatorLength,
                                   ComSInt32 nullIndicatorOffset,
                                   ComUInt32 dataOffset,
                                   ComSInt32 vcLenIndOffset,
                                   ComSInt16 vcIndicatorLength)

 : position_(position),
   flags_(flags),
   fsType_(fsType),
   ansiType_(ansiType),
   paramNameLen_((short) paramNameLen),
   prec_(prec),
   scale_(scale),
   encodingCharSet_(encodingCharSet),
   collation_(collation),
   dataLength_(dataLength),
   nullIndicatorLength_(nullIndicatorLength),
   nullIndicatorOffset_(nullIndicatorOffset),
   dataOffset_(dataOffset),
   vcLenIndOffset_(vcLenIndOffset),
   vcIndicatorLength_(vcIndicatorLength)
{
  UdrExeAssert(paramNameLen <= 128, "Param name length cannot exceed 128");
  if (paramNameLen > 0)
    str_cpy_all(paramName_, paramName, paramNameLen);
  paramName_[paramNameLen_] = 0;
}

IpcMessageObjSize UdrParameterInfo::packedLength() const
{
  IpcMessageObjSize result = 0;
  result += sizeof(position_);
  result += sizeof(flags_);
  result += sizeof(fsType_);
  result += sizeof(ansiType_);
  result += sizeof(paramNameLen_);
  result += paramNameLen_;
  result += sizeof(prec_);
  result += sizeof(scale_);
  result += sizeof(encodingCharSet_);
  result += sizeof(collation_);
  result += sizeof(dataLength_);
  result += sizeof(nullIndicatorLength_);
  result += sizeof(nullIndicatorOffset_);
  result += sizeof(dataOffset_);
  result += sizeof(vcLenIndOffset_);
  result += sizeof(vcIndicatorLength_);
  return result;
}

IpcMessageObjSize UdrParameterInfo::pack(IpcMessageBufferPtr &buffer) const
{
  IpcMessageObjSize result = 0;
  result += packIntoBuffer(buffer, position_);
  result += packIntoBuffer(buffer, flags_);
  result += packIntoBuffer(buffer, fsType_);
  result += packIntoBuffer(buffer, ansiType_);
  result += packIntoBuffer(buffer, paramNameLen_);
  if (paramNameLen_ > 0)
    result += packStrIntoBuffer(buffer, (char *) (&paramName_[0]),
                                paramNameLen_);
  result += packIntoBuffer(buffer, prec_);
  result += packIntoBuffer(buffer, scale_);
  result += packIntoBuffer(buffer, encodingCharSet_);
  result += packIntoBuffer(buffer, collation_);
  result += packIntoBuffer(buffer, dataLength_);
  result += packIntoBuffer(buffer, nullIndicatorLength_);
  result += packIntoBuffer(buffer, nullIndicatorOffset_);
  result += packIntoBuffer(buffer, dataOffset_);
  result += packIntoBuffer(buffer, vcLenIndOffset_);
  result += packIntoBuffer(buffer, vcIndicatorLength_);
  return result;
}

void UdrParameterInfo::unpack(IpcConstMessageBufferPtr &buffer)
{
  unpackBuffer(buffer, position_);
  unpackBuffer(buffer, flags_);
  unpackBuffer(buffer, fsType_);
  unpackBuffer(buffer, ansiType_);
  unpackBuffer(buffer, paramNameLen_);
  if (paramNameLen_ > 0)
    unpackStrFromBuffer(buffer, paramName_, paramNameLen_);
  paramName_[paramNameLen_] = 0;
  unpackBuffer(buffer, prec_);
  unpackBuffer(buffer, scale_);
  unpackBuffer(buffer, encodingCharSet_);
  unpackBuffer(buffer, collation_);
  unpackBuffer(buffer, dataLength_);
  unpackBuffer(buffer, nullIndicatorLength_);
  unpackBuffer(buffer, nullIndicatorOffset_);
  unpackBuffer(buffer, dataOffset_);
  unpackBuffer(buffer, vcLenIndOffset_);
  unpackBuffer(buffer, vcIndicatorLength_);
}

UdrParameterInfo& UdrParameterInfo::operator=(const UdrParameterInfo &other)
{
  //--------------------------------------
  // Nothing to do when copying yourself
  //--------------------------------------
  if (this == &other)
  {
    return *this;
  }

  position_            = other.position_;
  flags_               = other.flags_;
  fsType_              = other.fsType_;
  ansiType_            = other.ansiType_;
  paramNameLen_        = other.paramNameLen_;
  str_cpy_all(paramName_, other.paramName_, paramNameLen_ + 1);
  prec_                = other.prec_;
  scale_               = other.scale_;
  encodingCharSet_     = other.encodingCharSet_;
  collation_           = other.collation_;
  dataLength_          = other.dataLength_;
  nullIndicatorLength_ = other.nullIndicatorLength_;
  nullIndicatorOffset_ = other.nullIndicatorOffset_;
  dataOffset_          = other.dataOffset_;
  vcLenIndOffset_      = other.vcLenIndOffset_;
  vcIndicatorLength_   = other.vcIndicatorLength_;

  return *this;

} //UdrParameterInfo::operator=


void UdrParameterInfo::display(FILE *f, Lng32 indent,
                               UdrParameterInfo *pi) const
{
  char ind[100];
  Lng32 indIdx = 0;
  if (indent > 99)
  {
    indent = 99;
  }
  for (indIdx = 0; indIdx < indent; indIdx++)
  {
    ind[indIdx] = ' ';
  }
  ind[indIdx] = '\0';

  fprintf(f, "\n");
  fprintf(f, "%sContents of UdrParameterInfo:\n", ind);
  fprintf(f, "%s-----------------------------\n", ind );

  fprintf(f, "%sPosition                 : %d\n", ind ,
          (Lng32) pi->getPosition());
  fprintf(f, "%sFS Data Type             : %d\n", ind ,
          (Lng32) pi->getFSType());
  fprintf(f, "%sANSI Data Type           : %d\n", ind ,
          (Lng32) pi->getAnsiType());
  fprintf(f, "%sPrecision                : %d\n", ind ,
          (Lng32) pi->getPrec());
  fprintf(f, "%sScale                    : %d\n", ind ,
          (Lng32) pi->getScale());
  fprintf(f, "%sencodingCharSet          : %d\n", ind ,
          (Lng32) pi->getEncodingCharSet());
  fprintf(f, "%scollation                : %d\n", ind ,
          (Lng32) pi->getCollation());

  if (pi->isInOut())
  {
    fprintf(f, "%sMode                     : INOUT\n", ind );
  }
  else if (pi->isIn())
  {
    fprintf(f, "%sMode                     : IN\n", ind );
  }
  else if (pi->isOut())
  {
    fprintf(f, "%sMode                     : OUT\n", ind );
  }
  else
  {
    fprintf(f, "%sMode                     : INVALID\n", ind );
  }

  if (pi->isLmObjType())
    fprintf(f, "%sObject Mapping           : TRUE\n", ind );
  else
    fprintf(f, "%sObject Mapping           : FALSE\n", ind );

  if (pi->isNullable())
    fprintf(f, "%sNull Flag                : TRUE\n", ind );
  else
    fprintf(f, "%sNull Flag                : FALSE\n", ind );

  fprintf(f, "%sNull Indicator Length    : %d\n",
          ind , (Lng32) pi->getNullIndicatorLength());

  fprintf(f, "%sNull Indicator Offset    : %d\n",
          ind , (Lng32) pi->getNullIndicatorOffset());

  fprintf(f, "%sVC Len Indicator Offset  : %d\n",
          ind , (Lng32) pi->getVCLenIndOffset());

  fprintf(f, "%sVC Indicator Length      : %d\n",
          ind , (Lng32) pi->getVCIndicatorLength());

  fprintf(f, "%sData Offset              : %d\n",
          ind , (Lng32) pi->getDataOffset());

  fprintf(f, "%sData Length              : %d\n",
          ind , (Lng32) pi->getDataLength());

  fprintf(f, "%sPosition                 : %d\n\n",
          ind , (Lng32) pi->getPosition());

  fflush(f);

} // UdrParameterInfo::display

UdrTableInputInfo::UdrTableInputInfo(
    ComUInt16 tabIndex,
    ComUInt16 tableNameLen,
    const char *tableName,
    ComUInt16 numColumns,
    ComUInt32 outputRowLen
    )
  :tabIndex_(tabIndex),
   tableNameLen_(tableNameLen),
   numColumns_(numColumns),
   outputRowLen_(outputRowLen),
   inTableColumnDescs_(NULL)
{
  if (tableNameLen >0)
    {
    str_cpy_all(tableName_,tableName,tableNameLen);
    tableName_[tableNameLen_] = 0;
    }
 
}
const UdrParameterInfo &UdrTableInputInfo::getInTableColumnDesc(ComUInt32 i) const
{
UdrExeAssert(inTableColumnDescs_ && numColumns_ > i,
    "An invalid index was passed to UdrLoadMsg::getInTableCoumndesc()");
  return inTableColumnDescs_[i];
}

void UdrTableInputInfo::setInTableColumnDesc(ComUInt32 i, const UdrParameterInfo &info, NAMemory *heap)
{
  UdrExeAssert(numColumns_ > i,
    "An invalid index was passed to UdrLoadMsg::setInTableColumnDesc()");
  
  if(inTableColumnDescs_== NULL)
  {
    // allocate memory for the column desc info for this table desc entry
    const ComUInt32 colsize= numColumns_ *sizeof(UdrParameterInfo);
    inTableColumnDescs_ = (UdrParameterInfo *)(colsize ? 
						heap->allocateMemory(colsize) : NULL);
  }
  inTableColumnDescs_[i] = info;
}

UdrTableInputInfo& UdrTableInputInfo::operator=(const UdrTableInputInfo &other)
{
  tabIndex_ = other.tabIndex_;
  tableNameLen_ = other.tableNameLen_;
  str_cpy_all(tableName_,other.tableName_,tableNameLen_+1);
 
  numColumns_ = other.numColumns_;
  outputRowLen_ = other.outputRowLen_;
  inTableColumnDescs_ = other.inTableColumnDescs_;
  return *this;
  
}
IpcMessageObjSize UdrTableInputInfo::packedLength() const
{
  IpcMessageObjSize result = 0;
  result += sizeof(tabIndex_);
  result += sizeof(tableNameLen_);
  result += tableNameLen_;
  result += sizeof(numColumns_);
  result +=sizeof(outputRowLen_);
  UInt32 i = 0;
  for (i = 0; i < numColumns_; i++)
    {
      UdrParameterInfo &cinfo = inTableColumnDescs_[i];
      result += cinfo.packedLength();
    }
  return result;
}

IpcMessageObjSize UdrTableInputInfo::pack(IpcMessageBufferPtr &buffer) const
{
  IpcMessageObjSize result = 0;
  result += packIntoBuffer(buffer,tabIndex_);
  result += packIntoBuffer(buffer,tableNameLen_);
  if (tableNameLen_ >0)
    result += packStrIntoBuffer(buffer,(char *)(&tableName_[0]), 
				     tableNameLen_);
  
  result += packIntoBuffer(buffer,numColumns_);
  result += packIntoBuffer(buffer,outputRowLen_);
  UInt32 i = 0;

  for (i=0; i < numColumns_; i++)
    {
      const UdrParameterInfo &cinfo = inTableColumnDescs_[i];
      result += cinfo.pack(buffer);
    }
  return result;
}

void UdrTableInputInfo::unpack(IpcConstMessageBufferPtr &buffer, NAMemory *heap)
{
  unpackBuffer(buffer,tabIndex_);
  unpackBuffer(buffer,tableNameLen_);
  if (tableNameLen_ >0)
    unpackStrFromBuffer(buffer,tableName_,tableNameLen_);
  tableName_[tableNameLen_] = 0;
  unpackBuffer(buffer,numColumns_);
  unpackBuffer(buffer,outputRowLen_);
  
  // allocate memory for the column desc info for this table desc entry
  const ComUInt32 colsize= numColumns_ *sizeof(UdrParameterInfo);
  inTableColumnDescs_ = (UdrParameterInfo *)(colsize ? 
			  heap->allocateMemory(colsize) : NULL);
  ComUInt32 i;
  for (i = 0; i < numColumns_; i++)
  {
    UdrParameterInfo &cinfo = inTableColumnDescs_[i];
    cinfo.unpack(buffer);
  }
}
// -----------------------------------------------------------------------
// class UdrMessageObj
// -----------------------------------------------------------------------
UdrMessageObj::UdrMessageObj(UdrIpcObjectType objType,
                             IpcMessageObjVersion objVersion,
                             NAMemory *heap)
 : IpcMessageObj(objType, objVersion),
 heap_(heap),
 handle_(INVALID_UDR_HANDLE)
{
}

void UdrMessageObj::operator delete(void *p)
{
  if (p)
  {
    NAMemory *h = ((UdrMessageObj *) p)->getHeap();
    if (h)
    {
      h->deallocateMemory(p);
    }
    else
    {
      ::delete ((UdrMessageObj *) p);
    }
  }
}


IpcMessageObjSize UdrMessageObj::packedLength()
{
  return udrBaseClassPackedLength();
}

IpcMessageObjSize UdrMessageObj::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  return packUdrBaseClass(buffer);
}

void UdrMessageObj::unpackObj(IpcMessageObjType objType,
                              IpcMessageObjVersion objVersion,
                              NABoolean sameEndianness,
                              IpcMessageObjSize objSize,
                              IpcConstMessageBufferPtr buffer)
{
  unpackUdrBaseClass(buffer);
}

#ifdef UDR_DEBUG
//
// Debug-build flags to control when the UDR server will force
// corruption in certain reply messages. By forcing corruption in a
// reply we can test the IPC integrity checks and error handling in
// the executor.
//
// Note: in the future it may not be a good idea to maintain these
// flags as global variables. Right now they are globals only because
// that allows us to make fewer getenv() calls in the UDR server. The
// fields are being set by udrBaseClassPackedLength() and read by
// packUdrBaseClass(). That is OK for now because for a given LOAD or
// UNLOAD reply those two methods are always called in a predictable
// sequence by the message stream. This assumption about the sequence
// of calls may not always be valid.
//
static NABoolean corruptLoadReply = FALSE;
static NABoolean corruptUnloadReply = FALSE;
#endif

IpcMessageObjSize UdrMessageObj::udrBaseClassPackedLength()
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - udrBaseClassPackedLength()
  // - packUdrBaseClass()
  // - unpackUdrBaseClass()
  // - checkUdrBaseClass()

  IpcMessageObjSize result = 0;
  result += super::baseClassPackedLength();

#ifdef UDR_DEBUG
  // We allow the debug-build UDR server to corrupt a LOAD or UNLOAD
  // reply by not packing the UDR handle. Note that the getenv() tests
  // check for empty strings as values. These environment variables
  // ans set and unset dynamically, and apparently in OSS calling
  // putenv("X=") does not remove the environment variable, instead
  // the variable gets an empty string as its value.
  NABoolean packTheHandle = TRUE;
  IpcMessageType t = getType();
  if (t == UDR_MSG_LOAD_REPLY)
  {
    char *val = getenv("MXUDR_CORRUPT_LOAD_REPLY");
    corruptLoadReply = (val && val[0]);
    if (corruptLoadReply)
    {
      packTheHandle = FALSE;
    }
  }
  else if (t == UDR_MSG_UNLOAD_REPLY)
  {
    char *val = getenv("MXUDR_CORRUPT_UNLOAD_REPLY");
    corruptUnloadReply = (val && val[0]);
    if (corruptUnloadReply)
    {
      packTheHandle = FALSE;
    }
  }
  if (packTheHandle)
  {
    result += sizeof(handle_);
  }
#else
  result += sizeof(handle_);
#endif

  return result;
}

IpcMessageObjSize UdrMessageObj::packUdrBaseClass(IpcMessageBufferPtr &buffer)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - udrBaseClassPackedLength()
  // - packUdrBaseClass()
  // - unpackUdrBaseClass()
  // - checkUdrBaseClass()

  //
  // Note that we don't pack the heap pointer
  //
  IpcMessageObjSize result;
  result = super::packBaseClassIntoMessage(buffer);

#ifdef UDR_DEBUG
  // We allow the debug-build UDR server to corrupt a LOAD or UNLOAD
  // reply by not packing the UDR handle. See commentary above in
  // udrBaseClassPackedLength().
  NABoolean packTheHandle = TRUE;
  IpcMessageType t = getType();
  if (t == UDR_MSG_LOAD_REPLY && corruptLoadReply)
  {
    packTheHandle = FALSE;
  }
  else if (t == UDR_MSG_UNLOAD_REPLY && corruptUnloadReply)
  {
    packTheHandle = FALSE;
  }
  if (packTheHandle)
  {
    result += packIntoBuffer(buffer, handle_);
  }
#else
  result += packIntoBuffer(buffer, handle_);
#endif

  return result;
}

void UdrMessageObj::unpackUdrBaseClass(IpcConstMessageBufferPtr &buffer)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - udrBaseClassPackedLength()
  // - packUdrBaseClass()
  // - unpackUdrBaseClass()
  // - checkUdrBaseClass()

  super::unpackBaseClass(buffer);
  unpackBuffer(buffer, handle_);
}

NABoolean UdrMessageObj::checkObj(IpcMessageObjType t,
                                  IpcMessageObjVersion v,
                                  NABoolean sameEndianness,
                                  IpcMessageObjSize size,
                                  IpcConstMessageBufferPtr buffer) const
{
  return checkUdrBaseClass(t, v, sameEndianness, size, buffer);
}

NABoolean UdrMessageObj::checkUdrBaseClass(IpcMessageObjType t,
                                           IpcMessageObjVersion v,
                                           NABoolean sameEndianness,
                                           IpcMessageObjSize size,
                                           IpcConstMessageBufferPtr &buf) const
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - udrBaseClassPackedLength()
  // - packUdrBaseClass()
  // - unpackUdrBaseClass()
  // - checkUdrBaseClass()

  const IpcConstMessageBufferPtr lastByte = buf + size - 1;
  if (!super::checkBaseClass(t, v, sameEndianness, size, buf))
    return FALSE;
  if (!checkBuffer(buf, sizeof(handle_), lastByte))
    return FALSE;
  return TRUE;
}

IpcMessageRefCount UdrMessageObj::decrRefCount()
{
  IpcMessageRefCount result = 0;

  if (getRefCount() == 1)
  {
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

char *UdrMessageObj::allocateMemory(ComUInt32 nBytes)
{
  char *result = NULL;
  if (getHeap())
  {
    result = (char *) getHeap()->allocateMemory(nBytes);
  }
  else
  {
    result = new char[nBytes];
  }
  return result;
}

void UdrMessageObj::deallocateMemory(char *s)
{
  if (s)
  {
    if (getHeap())
    {
      getHeap()->deallocateMemory(s);
    }
    else
    {
      delete [] s;
    }
  }
}

char *UdrMessageObj::allocateString(const char *s)
{
  char *result = NULL;
  if (s)
  {
    ComUInt32 n = str_len(s) + 1;
    result = allocateMemory(n);
    str_cpy_all(result, s, (Lng32)n);
  }
  return result;
}

void UdrMessageObj::deallocateString(char *&s)
{
  deallocateMemory(s);
  s = NULL;
}

//----------------------------------------------------------------------
// UdrControlStream
//----------------------------------------------------------------------
UdrControlStream::UdrControlStream(IpcEnvironment *env,
                                   IpcMessageType msgType,
                                   IpcMessageObjVersion version)
  : IpcMessageStream(env,
                     msgType,
                     version,
                     0,      // 0 implies no fixed buffer size
                     TRUE)   // share message objects?
{
  sendCount_ = 0;
  recvCount_ = 0;
}

UdrControlStream::~UdrControlStream()
{
}

void UdrControlStream::actOnSend(IpcConnection *connection)
{
  // Base class implementation does nothing
}

void UdrControlStream::actOnReceive(IpcConnection *connection)
{
  // Base class implementation does nothing
}

//----------------------------------------------------------------------
// UdrLoadMsg
//----------------------------------------------------------------------
UdrLoadMsg::UdrLoadMsg(NAMemory *heap)
: UdrControlMsg(UDR_MSG_LOAD, UdrLoadMsgVersionNumber, heap),
  sqlName_(NULL), routineName_(NULL), routineSignature_(NULL),
  containerName_(NULL), externalPath_(NULL), librarySqlName_(NULL),transactionAttrs_(0),
  sqlAccessMode_(0), language_(0), paramStyle_(0),
  externalSecurity_(0), maxNumResultSets_(0),
  numParameters_(0), numInValues_(0), numOutValues_(0),
  inBufferSize_(0), outBufferSize_(0),
  inputRowSize_(0), outputRowSize_(0),
  udrFlags_(0), inParamInfo_(NULL), outParamInfo_(NULL), 
  routineOwnerId_(0),
  parentQid_(NULL),
  numInputTables_(0),
  inTables_(NULL),
  instanceNum_(0),
  numInstances_(0),
  numOptionalDataBufs_(0),
  optionalData_(NULL),
  optionalDataIsShared_(TRUE),
  udrSerInvocationInfoLen_(0),
  udrSerInvocationInfo_(NULL),
  udrSerPlanInfoLen_(0),
  udrSerPlanInfo_(NULL)
{
}

UdrLoadMsg::UdrLoadMsg(NAMemory *heap,
                       const char *sqlName,
                       const char *routineName,
                       const char *routineSignature,
                       const char *containerName,
                       const char *externalPath,
                       const char *librarySqlName,
                       ComRoutineTransactionAttributes transactionAttrs,
                       ComRoutineSQLAccess sqlAccessMode,
                       ComRoutineLanguage language,
                       ComRoutineParamStyle paramStyle,
                       ComRoutineExternalSecurity externalSecurity,
                       ComUInt32 maxNumResultSets,
                       ComUInt32 numParameters,
                       ComUInt32 numInValues,
                       ComUInt32 numOutValues,
                       ComUInt32 inBufferSize,
                       ComUInt32 outBufferSize,
                       ComUInt32 inputRowSize,
                       ComUInt32 outputRowSize,
                       ComUInt32 udrFlags,
                       Int32 routineOwnerId,
                       const char *parentQid,
                       ComUInt32 udrSerInvocationInfoLen,
                       const char *udrSerInvocationInfo,
                       ComUInt32 udrSerPlanInfoLen,
                       const char *udrSerPlanInfo,
		       ComUInt32 instanceNum,
		       ComUInt32 numInstances
		       )
: UdrControlMsg(UDR_MSG_LOAD, UdrLoadMsgVersionNumber, heap),
  sqlName_(NULL), routineName_(NULL), routineSignature_(NULL),
  containerName_(NULL), externalPath_(NULL), librarySqlName_(NULL),transactionAttrs_(transactionAttrs),
  sqlAccessMode_(sqlAccessMode), language_(language), paramStyle_(paramStyle),
  externalSecurity_(externalSecurity),
  maxNumResultSets_(maxNumResultSets),
  numParameters_(numParameters), numInValues_(numInValues),
  numOutValues_(numOutValues),
  inBufferSize_(inBufferSize), outBufferSize_(outBufferSize),
  inputRowSize_(inputRowSize), outputRowSize_(outputRowSize),
  udrFlags_(udrFlags), inParamInfo_(NULL), outParamInfo_(NULL),
  numInputTables_(0),
  inTables_(NULL), 
  routineOwnerId_(routineOwnerId),
  parentQid_(NULL),
  numOptionalDataBufs_(0), optionalData_(NULL), optionalDataIsShared_(TRUE),
  udrSerInvocationInfoLen_(udrSerInvocationInfoLen),
  udrSerInvocationInfo_(udrSerInvocationInfo),
  udrSerPlanInfoLen_(udrSerPlanInfoLen),
  udrSerPlanInfo_(udrSerPlanInfo),
  instanceNum_(instanceNum),
  numInstances_(numInstances)
{
  sqlName_ = allocateString(sqlName);
  routineName_ = allocateString(routineName);
  routineSignature_ = allocateString(routineSignature);
  containerName_ = allocateString(containerName);
  externalPath_ = allocateString(externalPath);
  librarySqlName_ = allocateString(librarySqlName);
  parentQid_ = allocateString(parentQid);
  allocateParamInfo();
  // allocateTableInputInfo();
}

//
// The following helper functions take care of memory management
// for parameter info arrays. They call NAMemory methods to allocate
// and deallocate memory. It is safe to call NAMemory::allocate(0),
// which returns NULL, and NAMemory::deallocate(NULL), which does
// nothing.
//
void UdrLoadMsg::allocateParamInfo()
{
  deallocateParamInfo();
  const ComUInt32 n1 = numInValues_ * sizeof(UdrParameterInfo);
  const ComUInt32 n2 = numOutValues_ * sizeof(UdrParameterInfo);
  inParamInfo_ = (UdrParameterInfo *) (n1 ? allocateMemory(n1) : NULL);
  outParamInfo_ = (UdrParameterInfo *) (n2 ? allocateMemory(n2) : NULL);
}
void UdrLoadMsg::deallocateParamInfo()
{
  deallocateMemory((char *) inParamInfo_);
  deallocateMemory((char *) outParamInfo_);
  inParamInfo_ = NULL;
  outParamInfo_ = NULL;
}

void UdrLoadMsg::allocateTableInputInfo()
{
  deallocateTableInputInfo();
  // First allocate the array of table descriptors
  const ComUInt32 t1 = numInputTables_ * sizeof(UdrTableInputInfo);
  inTables_ = (UdrTableInputInfo *) (t1 ? allocateMemory(t1) : NULL);
  

  /*
  // For each table descriptor allocate memory for the column descriptors  
  short j= 0;
  for (j = 0; j < numInputTables_; j++)
    {
      const ComUInt32 colsize=inTables_[j].getNumColumns() *sizeof(UdrParameterInfo);
      inTables_[j].inTableColumnDescs_ = (UdrParameterInfo *)(colsize ? 
      allocateMemory(colsize) : NULL); 
      
      } */

}

void UdrLoadMsg::deallocateTableInputInfo()
{
  short j = 0;
  if (inTables_)
    {
      for (j=0; j < numInputTables_;j++)
	{
	  deallocateMemory((char *)inTables_[j].inTableColumnDescs_);
	  inTables_[j].inTableColumnDescs_ = NULL;
	}
      deallocateMemory((char *)inTables_);
    
      inTables_ = NULL;
    }
}

void UdrLoadMsg::allocateInvocationInfo()
{
  if (udrSerInvocationInfoLen_)
    udrSerInvocationInfo_ = allocateMemory(udrSerInvocationInfoLen_);
  if (udrSerPlanInfoLen_)
    udrSerPlanInfo_ = allocateMemory(udrSerPlanInfoLen_);
}

void UdrLoadMsg::deallocateInvocationInfo()
{
  if (udrSerInvocationInfoLen_)
    deallocateMemory((char *) udrSerInvocationInfo_);
  if (udrSerPlanInfoLen_)
    deallocateMemory((char *) udrSerPlanInfo_);
  udrSerInvocationInfoLen_ = 0;
  udrSerInvocationInfo_ = NULL;
  udrSerPlanInfoLen_ = 0;
  udrSerPlanInfo_ = NULL;
}

UdrParameterInfo *UdrLoadMsg::setInParam(ComUInt32 i,
                                         const UdrParameterInfo &info)
{
  UdrExeAssert(inParamInfo_ && numInValues_ > i,
    "An invalid index was passed to UdrLoadMsg::setInParam()");
  UdrParameterInfo *result = NULL;
  if (inParamInfo_ && numInValues_ > i)
  {
    inParamInfo_[i] = info;
    result = &(inParamInfo_[i]);
  }
  return result;
}

UdrParameterInfo *UdrLoadMsg::setOutParam(ComUInt32 i,
                                          const UdrParameterInfo &info)
{
  UdrExeAssert(outParamInfo_ && numOutValues_ > i,
    "An invalid index was passed to UdrLoadMsg::setOutParam()");
  UdrParameterInfo *result = NULL;
  if (outParamInfo_ && numOutValues_ > i)
  {
    outParamInfo_[i] = info;
    result = &(outParamInfo_[i]);
  }
  return result;
}

const UdrParameterInfo &UdrLoadMsg::getInParam(ComUInt32 i) const
{
  UdrExeAssert(inParamInfo_ && numInValues_ > i,
    "An invalid index was passed to UdrLoadMsg::getInParam()");
  return inParamInfo_[i];
}

const UdrParameterInfo &UdrLoadMsg::getOutParam(ComUInt32 i) const
{
  UdrExeAssert(outParamInfo_ && numOutValues_ > i,
    "An invalid index was passed to UdrLoadMsg::getOutParam()");
  return outParamInfo_[i];
}
void UdrLoadMsg::setChildTableInput(ComUInt32 i,
                                         const UdrTableInputInfo &info)
{
  UdrExeAssert(inTables_ && numInputTables_ > i,
    "An invalid index was passed to UdrLoadMsg::setInParam()");
  UdrTableInputInfo *result = NULL;
  if (inTables_ && numInputTables_ > i)
  {
    inTables_[i] = info;
  }
 
}
void UdrLoadMsg::deallocateOptionalDataBufs()
{
  if (optionalData_)
  {
    if (!optionalDataIsShared_)
    {
      for (ComUInt32 i = 0; i < numOptionalDataBufs_; i++)
        deallocateMemory(optionalData_[i]);
    }
    deallocateMemory((char *) optionalData_);
  }

  numOptionalDataBufs_ = 0;
  optionalDataIsShared_ = TRUE;
  optionalData_ = NULL;
}

void UdrLoadMsg::initOptionalDataBufs(ComUInt32 numBufs, NABoolean isShared)
{
  deallocateOptionalDataBufs();
  numOptionalDataBufs_ = numBufs;
  optionalDataIsShared_ = isShared;
  optionalData_ = NULL;
  if (numBufs > 0)
    optionalData_ = (char **) allocateMemory(numBufs * sizeof(char *));
}

ComUInt32 UdrLoadMsg::getNumOptionalDataBufs()
{
  return numOptionalDataBufs_;
}

void UdrLoadMsg::setOptionalDataBuf(ComUInt32 i,
                                    const char *buf,
                                    ComUInt32 bufLen)
{
  UdrExeAssert(i < numOptionalDataBufs_, "Index out of range");
  const char *buf2 = buf;
  if (!optionalDataIsShared_)
  {
    buf2 = allocateMemory(bufLen);
    str_cpy_all((char *) buf2, buf, (Lng32) bufLen);
  }
  optionalData_[i] = (char *) buf2;
}

char *UdrLoadMsg::getOptionalDataBuf(ComUInt32 i)
{
  UdrExeAssert(i < numOptionalDataBufs_, "Index out of range");
  return optionalData_[i];
}



UdrLoadMsg::~UdrLoadMsg()
{
  deallocateString(sqlName_);
  deallocateString(routineName_);
  deallocateString(routineSignature_);
  deallocateString(containerName_);
  deallocateString(externalPath_);
  deallocateString(librarySqlName_);
  deallocateString(parentQid_);
  deallocateParamInfo();
  deallocateOptionalDataBufs();
  deallocateTableInputInfo();
}

IpcMessageObjSize UdrLoadMsg::packedLength()
{
  IpcMessageObjSize result;
  result = udrBaseClassPackedLength();
  result += packedStringLength(sqlName_);
  result += packedStringLength(routineName_);
  result += packedStringLength(routineSignature_);
  result += packedStringLength(containerName_);
  result += packedStringLength(externalPath_);
  result += packedStringLength(librarySqlName_);
  result += sizeof(transactionAttrs_);
  result += sizeof(sqlAccessMode_);
  result += sizeof(language_);
  result += sizeof(paramStyle_);
  result += sizeof(externalSecurity_);
  result += sizeof(maxNumResultSets_);
  result += sizeof(numParameters_);
  result += sizeof(numInValues_);
  result += sizeof(numOutValues_);
  result += sizeof(inBufferSize_);
  result += sizeof(outBufferSize_);
  result += sizeof(inputRowSize_);
  result += sizeof(outputRowSize_);
  result += sizeof(udrFlags_);
  result += sizeof(routineOwnerId_);
  result += packedStringLength(parentQid_);
  result += sizeof(numInputTables_);
  result += sizeof(numInstances_);
  result += sizeof(instanceNum_);

  ComUInt32 i = 0;
  for (i = 0; i < numInValues_; i++)
  {
    const UdrParameterInfo &info = inParamInfo_[i];
    const IpcMessageObjSize infoSize = info.packedLength();
    result += infoSize;
  }
  for (i = 0; i < numOutValues_; i++)
  {
    const UdrParameterInfo &info = outParamInfo_[i];
    const IpcMessageObjSize infoSize = info.packedLength();
    result += infoSize;
  }

  for (i=0; i <numInputTables_;i++)
    {
      const UdrTableInputInfo &tinfo = inTables_[i];
      const IpcMessageObjSize tinfosize = tinfo.packedLength();
      result += tinfosize;
    }

  result += sizeof(numOptionalDataBufs_);
  for (i = 0; i < numOptionalDataBufs_; i++)
  {
    const char *buf = optionalData_[i];
    ComUInt32 dataLen = 0;
    str_cpy_all((char *) &dataLen, buf, 4);
    result += (dataLen + 4);
  }

  result += sizeof(udrSerInvocationInfoLen_);
  result += sizeof(udrSerPlanInfoLen_);
  result += udrSerInvocationInfoLen_;
  result += udrSerPlanInfoLen_;

  return result;
}

IpcMessageObjSize UdrLoadMsg::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result;
  result = packUdrBaseClass(buffer);
  result += packCharStarIntoBuffer(buffer, sqlName_);
  result += packCharStarIntoBuffer(buffer, routineName_);
  result += packCharStarIntoBuffer(buffer, routineSignature_);
  result += packCharStarIntoBuffer(buffer, containerName_);
  result += packCharStarIntoBuffer(buffer, externalPath_);
  result += packCharStarIntoBuffer(buffer, librarySqlName_);
  result += packIntoBuffer(buffer, transactionAttrs_);
  result += packIntoBuffer(buffer, sqlAccessMode_);
  result += packIntoBuffer(buffer, language_);
  result += packIntoBuffer(buffer, paramStyle_);
  result += packIntoBuffer(buffer, externalSecurity_);
  result += packIntoBuffer(buffer, maxNumResultSets_);
  result += packIntoBuffer(buffer, numParameters_);
  result += packIntoBuffer(buffer, numInValues_);
  result += packIntoBuffer(buffer, numOutValues_);
  result += packIntoBuffer(buffer, inBufferSize_);
  result += packIntoBuffer(buffer, outBufferSize_);
  result += packIntoBuffer(buffer, inputRowSize_);
  result += packIntoBuffer(buffer, outputRowSize_);
  result += packIntoBuffer(buffer, udrFlags_);
  result += packIntoBuffer(buffer, routineOwnerId_);
  result += packCharStarIntoBuffer(buffer, parentQid_);
  result += packIntoBuffer(buffer, numInstances_);
  result += packIntoBuffer(buffer, instanceNum_);

  ComUInt32 i;
  for (i = 0; i < numInValues_; i++)
  {
    const UdrParameterInfo &info = inParamInfo_[i];
    result += info.pack(buffer);
  }
  for (i = 0; i < numOutValues_; i++)
  {
    const UdrParameterInfo &info = outParamInfo_[i];
    result += info.pack(buffer);
  }

  result +=packIntoBuffer(buffer,numInputTables_);
  for (i = 0; i < numInputTables_; i++)
    {
      const UdrTableInputInfo &tinfo = inTables_[i];
      result += tinfo.pack(buffer);
    }
  result += packIntoBuffer(buffer, numOptionalDataBufs_);
  for (i = 0; i < numOptionalDataBufs_; i++)
  {
    char *buf = optionalData_[i];
    ComUInt32 dataLen = 0;
    str_cpy_all((char *) &dataLen, buf, 4);
    result += packStrIntoBuffer(buffer, buf, (dataLen + 4));
  }

  result += packIntoBuffer(buffer, udrSerInvocationInfoLen_);
  result += packIntoBuffer(buffer, udrSerPlanInfoLen_);
  if (udrSerInvocationInfoLen_)
    result += packStrIntoBuffer(buffer,
                                (char *) udrSerInvocationInfo_,
                                udrSerInvocationInfoLen_); 
  if (udrSerPlanInfoLen_)
    result += packStrIntoBuffer(buffer,
                                (char *) udrSerPlanInfo_,
                                udrSerPlanInfoLen_); 

  return result;
}

void UdrLoadMsg::unpackObj(IpcMessageObjType objType,
                           IpcMessageObjVersion objVersion,
                           NABoolean sameEndianness,
                           IpcMessageObjSize objSize,
                           IpcConstMessageBufferPtr buffer)
{
  unpackUdrBaseClass(buffer);
  unpackBuffer(buffer, sqlName_, getHeap());
  unpackBuffer(buffer, routineName_, getHeap());
  unpackBuffer(buffer, routineSignature_, getHeap());
  unpackBuffer(buffer, containerName_, getHeap());
  unpackBuffer(buffer, externalPath_, getHeap());
  unpackBuffer(buffer, librarySqlName_, getHeap());
  unpackBuffer(buffer, transactionAttrs_);
  unpackBuffer(buffer, sqlAccessMode_);
  unpackBuffer(buffer, language_);
  unpackBuffer(buffer, paramStyle_);
  unpackBuffer(buffer, externalSecurity_);
  unpackBuffer(buffer, maxNumResultSets_);
  unpackBuffer(buffer, numParameters_);
  unpackBuffer(buffer, numInValues_);
  unpackBuffer(buffer, numOutValues_);
  unpackBuffer(buffer, inBufferSize_);
  unpackBuffer(buffer, outBufferSize_);
  unpackBuffer(buffer, inputRowSize_);
  unpackBuffer(buffer, outputRowSize_);
  unpackBuffer(buffer, udrFlags_);
  unpackBuffer(buffer, routineOwnerId_);
  unpackBuffer(buffer, parentQid_, getHeap());
  unpackBuffer(buffer, numInstances_);
  unpackBuffer(buffer, instanceNum_);
  allocateParamInfo();
  
  ComUInt32 i;
  for (i = 0; i < numInValues_; i++)
  {
    UdrParameterInfo &info = inParamInfo_[i];
    info.unpack(buffer);
  }
  for (i = 0; i < numOutValues_; i++)
  {
    UdrParameterInfo &info = outParamInfo_[i];
    info.unpack(buffer);
  }
  unpackBuffer(buffer, numInputTables_);
  allocateTableInputInfo();
  for (i =0; i < numInputTables_; i++)
    {
      UdrTableInputInfo &tinfo = inTables_[i];
      tinfo.unpack(buffer, getHeap());
    }
  ComUInt32 numBufs = 0;
  unpackBuffer(buffer, numBufs);
  initOptionalDataBufs(numBufs, FALSE);
  for (i = 0; i < numOptionalDataBufs_; i++)
  {
    ComUInt32 dataLen = 0;
    str_cpy_all((char *) &dataLen, buffer, 4);
    ComUInt32 totalLen = dataLen + 4;
    setOptionalDataBuf(i, buffer, totalLen);
    buffer += totalLen;
  }

  deallocateInvocationInfo();
  unpackBuffer(buffer, udrSerInvocationInfoLen_);
  unpackBuffer(buffer, udrSerPlanInfoLen_);
  allocateInvocationInfo();
  if (udrSerInvocationInfoLen_)
    unpackStrFromBuffer(buffer,
                        (char *) udrSerInvocationInfo_,
                        udrSerInvocationInfoLen_);
  if (udrSerPlanInfoLen_)
    unpackStrFromBuffer(buffer,
                        (char *) udrSerPlanInfo_,
                        udrSerPlanInfoLen_);
}

//----------------------------------------------------------------------
// UdrSessionMsg
//----------------------------------------------------------------------
UdrSessionMsg::UdrSessionMsg(NAMemory *heap)
  : UdrControlMsg(UDR_MSG_SESSION, UdrSessionMsgVersionNumber, heap),
    attrType_(UDR_SESSION_TYPE_UNKNOWN),
    flags_(0),
    strings_(heap)
{
}

UdrSessionMsg::UdrSessionMsg(UdrSessionAttrType attrType,
                             ComUInt32 flags,
                             NAMemory *heap)
  : UdrControlMsg(UDR_MSG_SESSION, UdrSessionMsgVersionNumber, heap),
    attrType_(attrType),
    flags_(flags),
    strings_(heap)
{
}

UdrSessionMsg::~UdrSessionMsg()
{
  ComUInt32 e = strings_.entries();
  for (ULng32 i = 0; i < e; i++)
  {
    NADELETEBASIC(strings_[i], getHeap());
  }
}

void UdrSessionMsg::addString(const char *option)
{
  if (option == NULL || option[0] == '\0')
  {
    return;
  }
  Int32 len = str_len(option);
  char *copy = new (getHeap()) char[len + 1];
  str_cpy_all(copy, option, len + 1);
  strings_.insert(copy);
}

IpcMessageObjSize UdrSessionMsg::packedLength()
{
  IpcMessageObjSize result;
  result = udrBaseClassPackedLength();

  result += sizeof(attrType_);
  result += sizeof(flags_);

  ComUInt32 e = strings_.entries();
  result += sizeof(e);

  for (ULng32 i = 0; i < e; i++)
  {
    result += packedStringLength(strings_[i]);
  }

  return result;
}

IpcMessageObjSize UdrSessionMsg::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result;
  result = packUdrBaseClass(buffer);
  result += packIntoBuffer(buffer, attrType_);
  result += packIntoBuffer(buffer, flags_);

  ComUInt32 e = strings_.entries();
  result += packIntoBuffer(buffer, e);

  for (ULng32 i = 0; i < e; i++)
  {
    result += packCharStarIntoBuffer(buffer, strings_[i]);
  }

  return result;
}

void UdrSessionMsg::unpackObj(IpcMessageObjType objType,
                              IpcMessageObjVersion objVersion,
                              NABoolean sameEndianness,
                              IpcMessageObjSize objSize,
                              IpcConstMessageBufferPtr buffer)
{
  unpackUdrBaseClass(buffer);

  unpackBuffer(buffer, attrType_);
  unpackBuffer(buffer, flags_);

  ComUInt32 e;
  unpackBuffer(buffer, e);

  char *copy;
  for (ULng32 i = 0; i < e; i++)
  {
    // The unpackBuffer() call will deallocate copy from the heap if
    // copy is non-NULL which is not what we want. So we make sure
    // copy is set to NULL before calling unpackBuffer().
    copy = NULL;
    unpackBuffer(buffer, copy, getHeap());
    strings_.insert(copy);
  }
}

#ifdef UDR_DEBUG
void UdrSessionMsg::display(FILE *f, const char *prefix) const
{
  fprintf(f, "%s[UdrSessionMsg] type %d, flags 0x%08x\n",
          prefix, (Lng32) attrType_, (Lng32) flags_);

  ComUInt32 e = strings_.entries();
  for (ULng32 i = 0; i < e; i++)
  {
    fprintf(f, "%s  '%s'\n", prefix, strings_[i]);
  }

  fflush(f);
}
#endif // UDR_DEBUG

//----------------------------------------------------------------------
// UdrDataBuffer
//----------------------------------------------------------------------

//
// Constructor for either copyless send or allocation on an NAMemory heap.
// For copyless send the NAMemory pointer should be NULL.
//
UdrDataBuffer::UdrDataBuffer(ULng32 sqlBufferLength,
                             InOut mode, NAMemory *heap)
  : UdrMessageObj(mode == UDR_DATA_IN ?
                  UDR_MSG_DATA_REQUEST : UDR_MSG_DATA_REPLY,
                  UdrDataBufferVersionNumber,
                  heap),
    sqlBufferLength_(sqlBufferLength),
    flags_(0)
{
  if (getHeap())
  {
    //
    // Allocate SqlBuffer on a heap
    //
    theBuffer_ = (SqlBuffer *) new (getHeap()) char[sqlBufferLength];
  }
  else
  {
    //
    // The SqlBuffer starts at the next address that is aligned on an
    // 8-byte boundary
    //
    IpcMessageBufferPtr bufferStart = (IpcMessageBufferPtr) this;
    bufferStart += sizeof(*this);
    alignBufferForNextObj(bufferStart);
    theBuffer_ = (SqlBuffer *) (bufferStart);
  }

  theBuffer_->driveInit(sqlBufferLength_, FALSE, SqlBuffer::NORMAL_);
  theBuffer_->bufferInUse();
  tableIndex_ = -1;
}

//
// Constructors for copyless receive
//
UdrDataBuffer::UdrDataBuffer(IpcBufferedMsgStream *msgStream,
                             NABoolean driveUnPack)
  : UdrMessageObj(msgStream)
{
  copylessUnpack(FALSE, 0, driveUnPack);
}


UdrDataBuffer::UdrDataBuffer(/* IN  */ IpcBufferedMsgStream *msgStream,
                             /* IN  */ IpcMessageObjSize objSize,
                             /* OUT */ NABoolean &integrityCheckResult )
  : UdrMessageObj(msgStream)
{
  integrityCheckResult = copylessUnpack(TRUE, objSize);
}

NABoolean UdrDataBuffer::copylessUnpack(NABoolean doChecks,
                                        IpcMessageObjSize objSize,
                                        NABoolean driveUnPack)
{
  //
  // Notes about this method
  //
  // When doChecks is TRUE we verify that the in-place unpacking of
  // this object does not cause us to create pointers that reference
  // memory outside the bounds of this object.
  //
  // The SqlBuffer starts at the next address beyond "this" that is
  // aligned on an 8-byte boundary.
  //
  theBuffer_ = NULL;
  NABoolean result = TRUE;
  IpcConstMessageBufferPtr buffer = (IpcMessageBufferPtr) this;
  IpcConstMessageBufferPtr lastByte = (doChecks ? buffer + objSize - 1 : 0);

  IpcMessageObjSize bytesForThis = sizeof(*this);
  alignSizeForNextObj(bytesForThis);

  // The buffer must be large enough to hold an instance of this class
  if (doChecks)
  {
    if (!checkBuffer(buffer, bytesForThis, lastByte))
    {
      result = FALSE;
    }
  }
  else
  {
    buffer += bytesForThis;
  }

  if (result)
  {
    if (doChecks)
    {
      // The buffer must be large enough to hold a SqlBufferNormal instance
      IpcConstMessageBufferPtr startOfSqlBuffer = buffer;
      if (!checkBuffer(startOfSqlBuffer, sizeof(SqlBufferNormal), lastByte))
      {
        result = FALSE;
      }
      else
      {
        // Now we verify the contents of the SqlBuffer instance. We must
        // not call virtual methods on the SqlBuffer at this point. That
        // can only be done reliably after the SqlBuffer has been
        // unpacked.
        theBuffer_ = (SqlBuffer *) buffer;
        if (theBuffer_->bufType() != SqlBufferBase::NORMAL_ ||
            theBuffer_->get_buffer_size() != sqlBufferLength_ ||
            !theBuffer_->packed() ||
            !theBuffer_->driveVerify(lastByte - buffer + 1))
        {
          theBuffer_ = NULL;
          result = FALSE;
        }
      }
    } // if (doChecks)

    else
    {
      theBuffer_ = (SqlBuffer *) buffer;
    }

  } // if (result)

  if (result && driveUnPack)
  {
    theBuffer_->driveUnpack();
    
    //
    // The sql_buffer state was set to IN_USE by the sender. This
    // prevents the buffer from being deallocated during I/O. Once all
    // rows have been consumed from this sql_buffer then isFree() will
    // return TRUE and the buffer will eventually be deallocated by the
    // stream. Here we assert that the buffer is not yet free, a way of
    // showing that the sender set its state to IN_USE.
    //
    // This test was not done earlier with the other SqlBuffer tests
    // because it requires a virtual method and we have to unpack the
    // object before we can reliably invoke virtual methods.
    //
    if (doChecks)
    {
      if (theBuffer_->isFree())
      {
        theBuffer_ = NULL;
        result = FALSE;
      }
    }
    else
    {
      UdrExeAssert(!(theBuffer_->isFree()),
                   "An empty UdrDataBuffer arrived on a UDR data stream");
    }
  } // if (result)

  if (!result)
  {
    UdrExeAssert(theBuffer_ == NULL,
                 "SqlBuffer should be NULL following integrity check failure");
  }

  return result;
}

UdrDataBuffer::~UdrDataBuffer()
{
  if (getHeap())
  {
    getHeap()->deallocateMemory(theBuffer_);
  }
}

//
// Determine if UdrDataBuffer in IpcMessageBuffer is available for recycle
//
NABoolean UdrDataBuffer::msgObjIsFree()
{
  NABoolean result = (theBuffer_ && (!theBuffer_->packed()) ? theBuffer_->isFree() : TRUE);
  return result;
}

//
// Deal with transport issues, change pointers to offsets, etc.
//
void UdrDataBuffer::prepMsgObjForSend()
{
  theBuffer_->drivePack();
#ifdef UDR_DEBUG
  // We allow the debug-build UDR server to corrupt an INVOKE
  // reply. Note that the getenv() test must also check for an empty
  // string value because in OSS calling putenv("X=") apparently does
  // not remove the environment setting.
  {
    char *val = getenv("MXUDR_CORRUPT_DATA_REPLY");
    if (val && val[0])
    {
      sqlBufferLength_ -= 1;
    }
  }
#endif
}

NABoolean UdrDataBuffer::moreRows() const
{
  NABoolean result = FALSE;
  if (theBuffer_ && !theBuffer_->atEOTD())
  {
    result = TRUE;
  }
  return result;
}

IpcMessageObjSize UdrDataBuffer::packedLength()
{
  //
  // Note: The number returned by this function is valid only for use
  // with "regular" message streams, not with buffered message streams
  // using copyless IPC
  //
  IpcMessageObjSize result = 0;
  result = udrBaseClassPackedLength();
  result += sizeof(flags_);
  result += sizeof(sqlBufferLength_);
  result +=sizeof(tableIndex_);
  result += sqlBufferLength_;
  return result;
}

IpcMessageObjSize UdrDataBuffer::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = 0;
  result += packUdrBaseClass(buffer);
  result += packIntoBuffer(buffer, flags_);
  result += packIntoBuffer(buffer, sqlBufferLength_);

  //
  // Convert pointers in the SqlBuffer to offsets and copy the SqlBuffer
  //
  theBuffer_->drivePack();
  str_cpy_all(buffer, (char *) theBuffer_, (Lng32)sqlBufferLength_);
  result += sqlBufferLength_;
  result += packIntoBuffer(buffer,tableIndex_);
  return result;
}

void UdrDataBuffer::unpackObj(IpcMessageObjType objType,
                              IpcMessageObjVersion objVersion,
                              NABoolean sameEndianness,
                              IpcMessageObjSize objSize,
                              IpcConstMessageBufferPtr buffer)
{
  UdrExeAssert((objType == UDR_MSG_DATA_REQUEST
                || objType == UDR_MSG_DATA_REPLY)
               && objVersion == UdrDataBufferVersionNumber
               && sameEndianness
               && objSize > sizeof(IpcMessageObj),
               "Invalid type or version seen by UdrDataBuffer::unpackObj()");

  unpackUdrBaseClass(buffer);
  unpackBuffer(buffer, flags_);
  unpackBuffer(buffer, sqlBufferLength_);

  //
  // Unpack the SqlBuffer
  //
  str_cpy_all((char *) theBuffer_, buffer, (Lng32)sqlBufferLength_);
  theBuffer_->driveUnpack();
  unpackBuffer(buffer,tableIndex_);

}

NABoolean UdrDataBuffer::checkObj(IpcMessageObjType t,
                                  IpcMessageObjVersion v,
                                  NABoolean sameEndianness,
                                  IpcMessageObjSize size,
                                  IpcConstMessageBufferPtr buffer) const
{
  //
  // This method is not implemented for now. All UDR data replies
  // travel on buffered streams via copyless IPC and should be
  // verified by calling the appropriate copyless receive constructor.
  //
  UdrExeAssert(FALSE, "UdrDataBuffer::checkObj() should not be called");
  return FALSE;
}

//----------------------------------------------------------------------
// class UdrRSMessageObj
//----------------------------------------------------------------------
IpcMessageObjSize UdrRSMessageObj::packedLength()
{
  return udrBaseClassPackedLength();
}

IpcMessageObjSize UdrRSMessageObj::packObjIntoMessage(IpcMessageBufferPtr buf)
{
  return packUdrBaseClass(buf);
}

void UdrRSMessageObj::unpackObj(IpcMessageObjType objType,
                                IpcMessageObjVersion objVersion,
                                NABoolean sameEndianness,
                                IpcMessageObjSize objSize,
                                IpcConstMessageBufferPtr buffer)
{
  unpackUdrBaseClass(buffer);
}

IpcMessageObjSize UdrRSMessageObj::udrBaseClassPackedLength()
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - udrBaseClassPackedLength()
  // - packUdrBaseClass()
  // - unpackUdrBaseClass()
  // - checkUdrBaseClass()

  IpcMessageObjSize result;
  result = super::udrBaseClassPackedLength();
  result += sizeof(rshandle_);
  return result;
}

IpcMessageObjSize UdrRSMessageObj::packUdrBaseClass(IpcMessageBufferPtr &buf)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - udrBaseClassPackedLength()
  // - packUdrBaseClass()
  // - unpackUdrBaseClass()
  // - checkUdrBaseClass()

  IpcMessageObjSize result;
  result = super::packUdrBaseClass(buf);
  result += packIntoBuffer(buf, rshandle_);
  return result;
}

void UdrRSMessageObj::unpackUdrBaseClass(IpcConstMessageBufferPtr &buffer)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - udrBaseClassPackedLength()
  // - packUdrBaseClass()
  // - unpackUdrBaseClass()
  // - checkUdrBaseClass()

  super::unpackUdrBaseClass(buffer);
  unpackBuffer(buffer, rshandle_);
}

NABoolean UdrRSMessageObj::checkObj(IpcMessageObjType t,
                                    IpcMessageObjVersion v,
                                    NABoolean sameEndianness,
                                    IpcMessageObjSize size,
                                    IpcConstMessageBufferPtr buffer) const
{
  return checkUdrBaseClass(t, v, sameEndianness, size, buffer);
}

NABoolean
UdrRSMessageObj::checkUdrBaseClass(IpcMessageObjType t,
                                   IpcMessageObjVersion v,
                                   NABoolean sameEndianness,
                                   IpcMessageObjSize size,
                                   IpcConstMessageBufferPtr &buf) const
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - udrBaseClassPackedLength()
  // - packUdrBaseClass()
  // - unpackUdrBaseClass()
  // - checkUdrBaseClass()

  const IpcConstMessageBufferPtr lastByte = buf + size - 1;
  if (!super::checkUdrBaseClass(t, v, sameEndianness, size, buf))
    return FALSE;
  if (!checkBuffer(buf, sizeof(rshandle_), lastByte))
    return FALSE;
  return TRUE;
}

//----------------------------------------------------------------------
// class UdrRSLoadMsg
//----------------------------------------------------------------------
UdrRSLoadMsg::UdrRSLoadMsg(ComUInt32 rsIndex,
                           ComUInt32 numRSCols,
                           ComUInt32 rowSize,
                           ComUInt32 bufferSize,
                           ComUInt32 flags,
                           NAMemory *heap)
  : UdrRSControlMsg(UDR_MSG_RS_LOAD, UdrRSLoadMsgVersionNumber, heap),
    rsIndex_(rsIndex), numRSCols_(numRSCols), rsColumnDesc_(NULL),
    outputRowSize_(rowSize), outBufferSize_(bufferSize), rsLoadFlags_(flags)
{
  allocateColumnDescs();
}

UdrRSLoadMsg::UdrRSLoadMsg(NAMemory *heap)
  : UdrRSControlMsg(UDR_MSG_RS_LOAD, UdrRSLoadMsgVersionNumber, heap),
    rsIndex_(0), numRSCols_(0), rsColumnDesc_(NULL),
    outputRowSize_(0), outBufferSize_(0), rsLoadFlags_(0)
{
}

UdrRSLoadMsg::~UdrRSLoadMsg()
{
  deallocateColumnDescs();
}

void UdrRSLoadMsg::allocateColumnDescs()
{
  deallocateColumnDescs();
  const ComUInt32 n = numRSCols_ * sizeof(UdrParameterInfo);
  rsColumnDesc_ = (UdrParameterInfo *) (n ? allocateMemory(n) : NULL);
}

void UdrRSLoadMsg::deallocateColumnDescs()
{
  deallocateMemory((char *) rsColumnDesc_);
  rsColumnDesc_ = NULL;
}

UdrParameterInfo& UdrRSLoadMsg::getColumnDesc(ComUInt32 i) const
{
  UdrExeAssert(rsColumnDesc_ && numRSCols_ > i,
	       "An invalid index was passed to UdrRSLoadMsg::getColumnDesc()");
  return rsColumnDesc_[i];
}

UdrParameterInfo * UdrRSLoadMsg::setColumnDesc(ComUInt32 i,
                                               const UdrParameterInfo &info)
{
  UdrExeAssert(rsColumnDesc_ && numRSCols_ > i,
	       "An invalid index was passed to UdrRSLoadMsg::setColumnDesc()");
  rsColumnDesc_[i] = info;
  return &rsColumnDesc_[i];
}

IpcMessageObjSize UdrRSLoadMsg::packedLength()
{
  IpcMessageObjSize result;
  result = udrBaseClassPackedLength();
  result += sizeof(rsIndex_);
  result += sizeof(numRSCols_);
  result += sizeof(outputRowSize_);
  result += sizeof(outBufferSize_);
  result += sizeof(rsLoadFlags_);

  ComUInt32 i = 0;
  for (i = 0; i < numRSCols_; i++)
  {
    result += getColumnDesc(i).packedLength();
  }

  return result;
}

IpcMessageObjSize UdrRSLoadMsg::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result;
  result = packUdrBaseClass(buffer);
  result += packIntoBuffer(buffer, rsIndex_);
  result += packIntoBuffer(buffer, numRSCols_);
  result += packIntoBuffer(buffer, outputRowSize_);
  result += packIntoBuffer(buffer, outBufferSize_);
  result += packIntoBuffer(buffer, rsLoadFlags_);

  for (ComUInt32 i=0; i<numRSCols_; i++)
  {
    result += getColumnDesc(i).pack(buffer);
  }

  return result;
}

void UdrRSLoadMsg::unpackObj(IpcMessageObjType objType,
                             IpcMessageObjVersion objVersion,
                             NABoolean sameEndianness,
                             IpcMessageObjSize objSize,
                             IpcConstMessageBufferPtr buffer)
{
  unpackUdrBaseClass(buffer);
  unpackBuffer(buffer, rsIndex_);
  unpackBuffer(buffer, numRSCols_);
  unpackBuffer(buffer, outputRowSize_);
  unpackBuffer(buffer, outBufferSize_);
  unpackBuffer(buffer, rsLoadFlags_);

  allocateColumnDescs();

  for (ComUInt32 i=0; i<numRSCols_; i++)
  {
    getColumnDesc(i).unpack(buffer);
  }
}

//----------------------------------------------------------------------
// struct ResultSetInfo
//----------------------------------------------------------------------
ResultSetInfo::ResultSetInfo(const char *proxy,
                             SQLSTMT_ID *stmtID,
                             ComUInt32 cntxID,
                             NAMemory *heap)
  : stmtID_(stmtID), cntxID_(cntxID), proxySyntax_(NULL), heap_(heap)
{
  proxySyntax_ = allocateString(proxy);
}

ResultSetInfo::~ResultSetInfo()
{
  deallocateString(proxySyntax_);
}

void ResultSetInfo::deallocateString(char *&s)
{
  if (s)
  {
    if (heap_)
      heap_->deallocateMemory(s);
    else
      delete [] s;
    s = NULL;
  }
}

char *ResultSetInfo::allocateString(const char *s)
{
  char *result = NULL;
  if (s)
  {
    ComUInt32 n = str_len(s) + 1;

    if (heap_)
      result = (char *) heap_->allocateMemory(n);
    else
      result = new char[n];

    str_cpy_all(result, s, (Lng32) n);
  }

  return result;
}

IpcMessageObjSize ResultSetInfo::packedLength()
{
  IpcMessageObjSize result = 0;
  result += sizeof(stmtID_);
  result += sizeof(cntxID_);
  result += packedStringLength(proxySyntax_);

  return result;
}

IpcMessageObjSize ResultSetInfo::pack(IpcMessageBufferPtr &buffer) const
{
  IpcMessageObjSize result = 0;
  result += packIntoBuffer(buffer, stmtID_);
  result += packIntoBuffer(buffer, cntxID_);
  result += packCharStarIntoBuffer(buffer, proxySyntax_);

  return result;
}

void ResultSetInfo::unpack(IpcConstMessageBufferPtr &buffer)
{

  unpackBuffer(buffer, stmtID_);
  unpackBuffer(buffer, cntxID_);
  unpackBuffer(buffer, proxySyntax_, heap_);
}

NABoolean ResultSetInfo::checkResultSetInfoClass(IpcMessageObjType t,
                                          IpcMessageObjVersion v,
                                          NABoolean sameEndianness,
                                          IpcMessageObjSize size,
                                          IpcConstMessageBufferPtr &buf) 
{
   IpcConstMessageBufferPtr lastByte = buf + size - 1;

   // check the buffer for stmtID_
   if (!checkBuffer(buf, sizeof(void *), lastByte))
     {
       ipcIntegrityCheckEpilogue(FALSE);
       return FALSE;
     }

   // check the buffer after cntxID_
   if (!checkBuffer(buf, sizeof(ComUInt32), lastByte))
     {
        ipcIntegrityCheckEpilogue(FALSE);
        return FALSE;
     }

   // check for proxySyntax_, it is stored as (long length, *char)
   if (!(checkCharStarInBuffer(buf, sameEndianness, lastByte)))
      return FALSE;

   return TRUE;
}

ResultSetInfo& ResultSetInfo::operator=(const ResultSetInfo &src)
{
  // Nothing to do if source and target are same
  if (this == &src)
    return *this;

  deallocateString(proxySyntax_);
  proxySyntax_ = allocateString(src.proxySyntax_);

  stmtID_ = src.stmtID_;
  cntxID_ = src.cntxID_;

  return *this;
}

//----------------------------------------------------------------------
// class UdrRSInfoMsg
//----------------------------------------------------------------------
UdrRSInfoMsg::UdrRSInfoMsg(ComUInt32 numRS, NAMemory *heap)
    : UdrMessageObj(UDR_MSG_RS_INFO, UdrRSInfoMsgVersionNumber, heap),
      numRS_(numRS), resultSetInfo_(NULL)
{
  allocateResultSetInfo();
}

UdrRSInfoMsg::~UdrRSInfoMsg()
{
  deallocateResultSetInfo();
}

ResultSetInfo &UdrRSInfoMsg::getRSInfo(ComUInt32 i) const
{
  UdrExeAssert(resultSetInfo_ && numRS_ > i,
	       "An invalid index was passed to UdrRSInfoMsg::getRSInfo()");
  return resultSetInfo_[i];
}

void UdrRSInfoMsg::setRSInfo(ComUInt32 i, const ResultSetInfo &info)
{
  UdrExeAssert(resultSetInfo_ && numRS_ > i,
	       "An invalid index was passed to UdrRSInfoMsg::setRSInfo()");
  resultSetInfo_[i] = info;
}

void UdrRSInfoMsg::allocateResultSetInfo()
{
  deallocateResultSetInfo();

  // Allocate memory for resultSetInfo_ field
  ComUInt32 n = numRS_ * sizeof(ResultSetInfo);
  resultSetInfo_ = (ResultSetInfo *) (n ? allocateMemory(n) : NULL);
  memset(resultSetInfo_, 0, n);

  for (ComUInt32 i=0; i < numRS_; i++)
    resultSetInfo_[i].setHeap(getHeap());
}

void UdrRSInfoMsg::deallocateResultSetInfo()
{
  if (resultSetInfo_)
  {
     for (ComUInt32 i=0; i < numRS_; i++)
     {
        deallocateMemory(resultSetInfo_[i].proxySyntax_);
     }
  }
  deallocateMemory((char *) resultSetInfo_);
  resultSetInfo_ = NULL;
}

IpcMessageObjSize UdrRSInfoMsg::packedLength()
{
  IpcMessageObjSize result = super::packedLength();
  result += sizeof(numRS_);

  for (ComUInt32 i=0; i<numRS_; i++)
    result += getRSInfo(i).packedLength();

  return result;
}

IpcMessageObjSize UdrRSInfoMsg::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = 0;

  result += packUdrBaseClass(buffer);

  #ifdef UDR_DEBUG  
    char *val = getenv("MXUDR_CORRUPT_UDRRSINFOMSG");
    if (val && val[0])
    {
       result += packIntoBuffer(buffer, (numRS_+ 1));
    } else
       result += packIntoBuffer(buffer, numRS_);
    
  #else
    result += packIntoBuffer(buffer, numRS_);
  #endif

  for (ComUInt32 i=0; i<numRS_; i++)
  {
    result += getRSInfo(i).pack(buffer);
  }

  return result;
}

void UdrRSInfoMsg::unpackObj(IpcMessageObjType objType,
                             IpcMessageObjVersion objVersion,
                             NABoolean sameEndianness,
                             IpcMessageObjSize objSize,
                             IpcConstMessageBufferPtr buffer)
{
  unpackUdrBaseClass(buffer);

  unpackBuffer(buffer, numRS_);

  allocateResultSetInfo();

  for (ComUInt32 i=0; i<numRS_; i++)
  {
    getRSInfo(i).unpack(buffer);
  }
}

NABoolean UdrRSInfoMsg::checkObj(IpcMessageObjType t,
                                 IpcMessageObjVersion v,
                                 NABoolean sameEndianness,
                                 IpcMessageObjSize size,
                                 IpcConstMessageBufferPtr buffer) const
{
  return checkUdrBaseClass(t, v, sameEndianness, size, buffer);
}

NABoolean UdrRSInfoMsg::checkUdrBaseClass(IpcMessageObjType t,
                                          IpcMessageObjVersion v,
                                          NABoolean sameEndianness,
                                          IpcMessageObjSize size,
                                          IpcConstMessageBufferPtr &buf) const
{
   IpcConstMessageBufferPtr lastByte = buf + size - 1;
   IpcConstMessageBufferPtr oldBuf = buf;

   if (!super::checkUdrBaseClass(t, v, sameEndianness, size, buf))
      return FALSE;

   // We need to adjust size since it will be used by
   // ResultSetInfo::checkResultSetInfoClass()
   size -= (buf - oldBuf);
   oldBuf = buf;

   ComUInt32 numRS = 0;
   if (!checkAndUnpackBuffer(buf, sizeof(numRS),
                            (char *) &numRS, lastByte))
     {
       ipcIntegrityCheckEpilogue(FALSE);
       return FALSE;
     }

   for (ComUInt32 i=0; i<numRS; i++)
     {
       size -= (buf - oldBuf);
       oldBuf = buf;
       if (!(ResultSetInfo::checkResultSetInfoClass(t, v, sameEndianness, size, buf)))
          return FALSE;
     }

   return TRUE;
}
