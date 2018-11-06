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
* File:         ComTdbUdr.cpp
* Description:  
*
* Created:      2/8/2000
* Language:     C++
*
*
*
****************************************************************************
*/

#include "ComTdbUdr.h"

// ---------------------------------------------------------------------
// ComTdbUdr::ComTdbUdr()
// ---------------------------------------------------------------------
ComTdbUdr::ComTdbUdr(char *sqlName,
                     char *routineName,
                     char *routineSignature,
                     char *containerName,
                     char *externalPath,
                     char *librarySqlName,
                     Int64 libraryRedefTime,
                     char *libraryBlobHandle,
                     char *librarySchName,
                     Int32 libraryVersion,
                     char *runtimeOptions,
                     char *runtimeOptionDelimiters,

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
                     )
: ComTdb(ex_UDR, eye_UDR, estimatedRowCount, criDescParent,
         criDescReturned, downQueueMaxSize, upQueueMaxSize,
         numOutputBuffers, outputBufferSize),

  sqlName_(sqlName),
  routineName_(routineName),
  routineSignature_(routineSignature),
  containerName_(containerName),
  externalPath_(externalPath),
  librarySqlName_(librarySqlName),
  libraryRedefTime_(libraryRedefTime),
  libraryBlobHandle_(libraryBlobHandle),
  librarySchName_(librarySchName),
  libraryVersion_(libraryVersion),
  runtimeOptions_(runtimeOptions),
  runtimeOptionDelimiters_(runtimeOptionDelimiters),

  flags_(flags),
  numInputValues_(numInputValues),
  numOutputValues_(numOutputValues),
  numParams_(numParams),
  maxResultSets_(maxResultSets),
  stateAreaSize_(stateAreaSize),
  udrType_(udrType),
  languageType_(languageType),
  paramStyle_(paramStyle),
  sqlAccessMode_(sqlAccessMode),
  transactionAttrs_(transactionAttrs),
  externalSecurity_(externalSecurity),
  routineOwnerId_(routineOwnerId),
  requestSqlBufferSize_(requestBufferSize),
  replySqlBufferSize_(replyBufferSize),
  
  workCriDesc_(workCriDesc),
  inputExpr_(inputExpr),
  outputExpr_(outputExpr),
  scanExpr_(scanExpr),
  projExpr_(projExpr),
  requestTuppIndex_(requestTuppIndex),
  replyTuppIndex_(replyTuppIndex),
  requestRowLen_(requestRowLen),
  replyRowLen_(replyRowLen),
  outputRowLen_(outputRowLen),
  optionalData_(optionalData),
  udrSerInvocationInfoLen_(udrSerInvocationInfoLen),
  udrSerInvocationInfo_(udrSerInvocationInfo),
  udrSerPlanInfoLen_(udrSerPlanInfoLen),
  udrSerPlanInfo_(udrSerPlanInfo),
  javaDebugPort_(javaDebugPort),
  javaDebugTimeout_(javaDebugTimeout),

  numChildTableInputs_((Int16) numChildInputs),
  childInputExprs_(space,(void **)childInputExprs,numChildInputs),
  childTdbs_(space,(void **)childTdbs,numChildInputs)
{
  //
  // Allocate an array of pointers to the param info objects
  //
  paramInfo_ = (UdrFormalParamInfoPtr *)
    (space->allocateAlignedSpace(numParams_ * sizeof(UdrFormalParamInfoPtr)));
  for (UInt32 i = 0; i < numParams_; i++)
  {
    paramInfo_[i] = (UdrFormalParamInfoPtrPtr)NULL;
  }

  //
  // Allocate an array of pointers to the Child Table info objects
  //
  udrChildTableDescInfo_ = (UdrTableDescInfoPtr *)
    (space->allocateAlignedSpace(numChildInputs * sizeof(UdrTableDescInfoPtr)));
  for (UInt32 i = 0; i < numChildInputs; i++)
  {
    udrChildTableDescInfo_[i] = (UdrFormalParamInfoPtrPtr)NULL;
  }
}

ComTdbUdr::~ComTdbUdr()
{
}

Long ComTdbUdr::pack(void *space)
{
  sqlName_.pack(space);
  routineName_.pack(space);
  routineSignature_.pack(space);
  containerName_.pack(space);
  externalPath_.pack(space);
  librarySqlName_.pack(space);
  runtimeOptions_.pack(space);
  runtimeOptionDelimiters_.pack(space);
  workCriDesc_.pack(space);
  inputExpr_.pack(space);
  outputExpr_.pack(space);
  scanExpr_.pack(space);
  projExpr_.pack(space);

  //
  // The NAVersionedObject array templates use long values to index
  // into the array, so we cast numParams_ to long here. This is assumed
  // not to be a problem because other parts of the system do not store 
  // param count as unsigned long. E.g. the PARAMS table stores column
  // number as INT.
  //
  paramInfo_.pack(space, (Lng32) numParams_);

  optionalData_.pack(space);
  udrSerInvocationInfo_.pack(space);
  udrSerPlanInfo_.pack(space);
  libraryBlobHandle_.pack(space);
  librarySchName_.pack(space);
  udrChildTableDescInfo_.pack(space,(Lng32)numChildTableInputs_);
  childInputExprs_.pack(space,(Lng32)numChildTableInputs_);
  childTdbs_.pack(space,(Lng32)numChildTableInputs_);
  
  return ComTdb::pack(space);
}

Lng32 ComTdbUdr::unpack(void *base, void *reallocator)
{
  if (sqlName_.unpack(base)) return -1;
  if (routineName_.unpack(base)) return -1;
  if (routineSignature_.unpack(base)) return -1;
  if (containerName_.unpack(base)) return -1;
  if (externalPath_.unpack(base)) return -1;
  if (librarySqlName_.unpack(base)) return -1;
  if (runtimeOptions_.unpack(base)) return -1;
  if (runtimeOptionDelimiters_.unpack(base)) return -1;
  if (workCriDesc_.unpack(base, reallocator)) return -1;
  if (inputExpr_.unpack(base, reallocator)) return -1;
  if (outputExpr_.unpack(base, reallocator)) return -1;
  if (scanExpr_.unpack(base, reallocator)) return -1;
  if (projExpr_.unpack(base, reallocator)) return -1;
 
  //
  // The NAVersionedObject array templates use long values to index
  // into the array, so we cast numParams_ to long here. This is assumed
  // not to be a problem because other parts of the system do not store 
  // param count as unsigned long. E.g. the PARAMS table stores column
  // number as INT.
  //
  if (paramInfo_.unpack(base, (Lng32) numParams_, reallocator))
    return -1;

  if (optionalData_.unpack(base, reallocator))
    return -1;
  if (udrChildTableDescInfo_.unpack(base,(Lng32)numChildTableInputs_,reallocator))
    return -1;
  if (childInputExprs_.unpack(base,(Lng32)numChildTableInputs_,reallocator))
    return -1;
  if (childTdbs_.unpack(base,(Lng32)numChildTableInputs_,reallocator))
    return -1;
  if (udrSerInvocationInfo_.unpack(base))
    return -1;
  if (udrSerPlanInfo_.unpack(base))
    return -1;
  if (libraryBlobHandle_.unpack(base)) return -1;
  if (librarySchName_.unpack(base)) return -1;
  return ComTdb::unpack(base, reallocator);
}

void ComTdbUdr::displayContents(Space *space, ULng32 flag)
{
  ComTdb::displayContents(space, flag & 0xFFFFFFFE);

  if (flag & 0x00000008)
  {
    const size_t sz = sizeof(short);
    char buf[512];

    str_sprintf(buf, "\nFor ComTdbUdr :");
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

    Lng32 lowFlags = (Lng32) (flags_ % 65536);
    Lng32 highFlags = (Lng32) ((flags_ - lowFlags) / 65536);
    str_sprintf(buf, "flags = %x%x", highFlags, lowFlags);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

    if (sqlName_)
    {
      char *s = sqlName_;
      str_sprintf(buf, "routineName = %s", s);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    }

    if (routineName_)
    {
      char *s = routineName_;
      str_sprintf(buf, "externalName = %s", s);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    }

    if (containerName_)
    {
      char *s = containerName_;
      str_sprintf(buf, "externalFile = %s", s);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    }

    if (externalPath_)
    {
      char *s = externalPath_;
      str_sprintf(buf, "externalPath = %s", s);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    }

    if (librarySqlName_)
    {
      char *s = librarySqlName_;
      str_sprintf(buf, "librarySqlName = %s", s);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    }
    if (libraryBlobHandle_)
      {
        char *s = libraryBlobHandle_;
        str_sprintf(buf, "libraryBlobHandle = %s", s);
        space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
      }
     if (librarySchName_)
      {
        char *s = librarySchName_;
        str_sprintf(buf, "librarySchName = %s", s);
        space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
      }
    str_sprintf(buf, "\nlibrayRedefTimestamp = %ld",
                libraryRedefTime_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    str_sprintf(buf, "\nlibrayVersion = %d",
                libraryVersion_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    // Some strings come from the user and there is no limit on the
    // maximum length. For these strings we will print two lines, the
    // first a header line and the second the actual string. For
    // example, the Java signature will look like this in the SHOWPLAN
    // output:
    //
    // signature =
    // (Ljava/lang/Float;[Ljava/lang/Float;)V
    //
    const char *s1;
    const char *s2;

    if (routineSignature_)
    {
      s1 = "\nsignature = ";
      s2 = routineSignature_;
      space->allocateAndCopyToAlignedSpace(s1, str_len(s1), sz);
      space->allocateAndCopyToAlignedSpace(s2, str_len(s2), sz);
    }

    if (runtimeOptions_)
    {
      s1 = "\nruntimeOptions = ";
      s2 = runtimeOptions_;
      space->allocateAndCopyToAlignedSpace(s1, str_len(s1), sz);
      space->allocateAndCopyToAlignedSpace(s2, str_len(s2), sz);
    }

    if (runtimeOptionDelimiters_)
    {
      s1 = "\noptionDelimiters = ";
      s2 = runtimeOptionDelimiters_;
      space->allocateAndCopyToAlignedSpace(s1, str_len(s1), sz);
      space->allocateAndCopyToAlignedSpace(s2, str_len(s2), sz);
    }

    str_sprintf(buf, "\nnumParameters = %d, maxResultSets = %d",
                numParams_, maxResultSets_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    
    str_sprintf(buf, "numInputValues = %d, numOutputValues = %d",
                numInputValues_, numOutputValues_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    
    str_sprintf(buf, "requestBufferSize = %d, replyBufferSize = %d",
                requestSqlBufferSize_, replySqlBufferSize_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    
    str_sprintf(buf, "requestRowLength = %d, replyRowLength = %d",
                requestRowLen_, replyRowLen_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    
    str_sprintf(buf, "outputRowLen = %d, stateAreaSize = %d",
                outputRowLen_, stateAreaSize_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    
    str_sprintf(buf, "requestTuppIndex = %d, replyTuppIndex = %d",
                (Int32) requestTuppIndex_, (Int32) replyTuppIndex_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

    str_sprintf(buf, "udrType = %d, languageType = %d",
                (Int32) udrType_, (Int32) languageType_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

    str_sprintf(buf, "parameterStyle = %d, sqlAccessMode = %d",
                (Int32) paramStyle_, (Int32) sqlAccessMode_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

    str_sprintf(buf, "transactionAttributes = %d", (Int32) transactionAttrs_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

    str_sprintf(buf, "externalSecurity = %d, routineOwnerId = %d",
                (Int32) externalSecurity_, (Int32) routineOwnerId_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

    UInt32 i;
    for (i = 0; i < numParams_; i++)
    {
      const UdrFormalParamInfo *p = paramInfo_[i];

      str_sprintf(buf, "\nParameter %d", (Int32) i);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

      str_sprintf(buf, "  name [%s]", p->getParamName());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

      str_sprintf(buf, "  flags %x, type %d", p->getFlags(),
                  (Int32) p->getType());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

      str_sprintf(buf, "  precision %d, scale %d, charset %d, collation %d",
                  (Int32) p->getPrecision(), (Int32) p->getScale(),
                  (Int32) p->getEncodingCharSet(), (Int32) p->getCollation());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    }

    Queue *optData = getOptionalData();
    if (optData)
    {
      UInt32 dataElems = optData->numEntries();
      str_sprintf(buf, "\nNumber of optional data elements: %d",
                  (Int32) dataElems);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

      const char *s = NULL;
      i = 0;

      optData->position();
      while ((s = (const char *) optData->getNext()) != NULL)
      {
        // Each data element is prefixed by a 4-byte length field
        UInt32 len = 0;
        str_cpy_all((char *)&len, s, 4);

        str_sprintf(buf, "\nOptional data %d (length %d):",
                    (Int32) i++, (Int32) len);
        space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

        if (len > 0)
        {
          // Create a buffer containing at most 200 bytes of data
          if (len > 200)
            len = 200;
          char truncatedBuf[201];
          str_cpy_all(truncatedBuf, s + 4, len);
          truncatedBuf[len] = 0;

          // Change NULL bytes and non-ASCII characters to '.' for
          // display purposes
          for (UInt32 j = 0; j < len; j++)
          {
            if (truncatedBuf[j] == 0 || !isascii(truncatedBuf[j]))
              truncatedBuf[j] = '.';
          }
          
          space->allocateAndCopyToAlignedSpace(truncatedBuf, len, sz);
        }
      }
    }
    if (javaDebugPort_ > 0)
    {
      str_sprintf(buf, "\njavaDebugPort = %d, javaDebugTimeout = %d",
                  javaDebugPort_, javaDebugTimeout_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
    }

  } // if (flag & 0x00000008)

  if (flag & 0x00000001)
  {
    displayExpression(space,flag);
    displayChildren(space,flag);
  }
}



