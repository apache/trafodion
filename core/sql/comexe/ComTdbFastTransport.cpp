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
* File:         ComTdbFastTransport.cpp
* Description:  
*
* Created:      09/13/2012
* Language:     C++
*
*
*
****************************************************************************
*/

#include "ComTdbFastTransport.h"

// ---------------------------------------------------------------------
// ComTdbFastExtract::ComTdbFastExtract()
// ---------------------------------------------------------------------
ComTdbFastExtract::ComTdbFastExtract(
                     ULng32 flags,
                     Cardinality estimatedRowCount,
                     char * targetName,
                     char * hdfsHost,
                     Lng32 hdfsPort,
                     char * hiveTableName,
                     char * delimiter,
                     char * header,
                     char * nullString,
                     char * recordSeparator,
                     ex_cri_desc *criDescParent,
                     ex_cri_desc *criDescReturned,
                     ex_cri_desc *workCriDesc,
                     queue_index downQueueMaxSize,
                     queue_index upQueueMaxSize,
                     Lng32 numOutputBuffers,
                     ULng32 outputBufferSize,
                     UInt16 numIOBuffers,
                     UInt16 ioTimeout,
                     ex_expr *inputExpr,
                     ex_expr *outputExpr,
                     ULng32 requestRowLen,
                     ULng32 outputRowLen,
                     ex_expr * childDataExpr,
                     ComTdb * childTdb,
                     Space *space,
                     unsigned short childDataTuppIndex,
                     unsigned short cnvChildDataTuppIndex,
                     ULng32 childDataRowLen,
                     Int64 hdfBuffSize,
                     Int16 replication

                     )
: ComTdb(ex_FAST_EXTRACT, eye_FAST_EXTRACT, estimatedRowCount, criDescParent,
         criDescReturned, downQueueMaxSize, upQueueMaxSize,
         numOutputBuffers, outputBufferSize),
  flags_(flags),
  targetName_(targetName),
  hdfsHostName_(hdfsHost),
  hdfsPortNum_(hdfsPort),
  delimiter_(delimiter),
  header_(header),
  nullString_(nullString),
  recordSeparator_(recordSeparator),
  workCriDesc_(workCriDesc),
  inputExpr_(inputExpr),
  outputExpr_(outputExpr),
  requestRowLen_(requestRowLen),
  outputRowLen_(outputRowLen),
  childDataExpr_(childDataExpr),
  childTdb_(childTdb),
  childDataTuppIndex_(childDataTuppIndex),
  cnvChildDataTuppIndex_(cnvChildDataTuppIndex),
  numIOBuffers_(numIOBuffers),
  hiveTableName_(hiveTableName),
  hdfsIOBufferSize_(hdfBuffSize),
  hdfsReplication_(replication),
  ioTimeout_(ioTimeout),
  childDataRowLen_(childDataRowLen),
  hdfsIoByteArraySizeInKB_(0),
  modTSforDir_(-1)
{

}

ComTdbFastExtract::~ComTdbFastExtract()
{
}

Long ComTdbFastExtract::pack(void *space)
{

  targetName_.pack(space);
  delimiter_.pack(space);
  header_.pack(space);
  nullString_.pack(space);
  recordSeparator_.pack(space);
  workCriDesc_.pack(space);
  inputExpr_.pack(space);
  outputExpr_.pack(space);
  childDataExpr_.pack(space);
  childTdb_.pack(space);
  hiveTableName_.pack(space);
  hdfsHostName_.pack(space);

  return ComTdb::pack(space);
}

Lng32 ComTdbFastExtract::unpack(void *base, void *reallocator)
{

  if (targetName_.unpack(base)) return -1;
  if (delimiter_.unpack(base)) return -1;
  if (header_.unpack(base)) return -1;
  if (nullString_.unpack(base)) return -1;
  if (recordSeparator_.unpack(base)) return -1;
  if (workCriDesc_.unpack(base, reallocator)) return -1;
  if (inputExpr_.unpack(base, reallocator)) return -1;
  if (outputExpr_.unpack(base, reallocator)) return -1;
  if (childDataExpr_.unpack(base, reallocator)) return -1;
  if (childTdb_.unpack(base,reallocator))  return -1;
  if (hiveTableName_.unpack(base)) return -1;
  if (hdfsHostName_.unpack(base)) return -1;

  return ComTdb::unpack(base, reallocator);
}

void ComTdbFastExtract::displayContents(Space *space, ULng32 flag)
{
  ComTdb::displayContents(space, flag & 0xFFFFFFFE);

  if (flag & 0x00000008)
  {
    const size_t sz = sizeof(short);
    char buf[512];

    str_sprintf(buf, "\nFor ComTdbFastExtract :");
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

    Lng32 lowFlags = (Lng32) (flags_ % 65536);
    Lng32 highFlags = (Lng32) ((flags_ - lowFlags) / 65536);
    str_sprintf(buf, "flags = %x%x", highFlags, lowFlags);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

    str_sprintf(buf, "requestRowLength = %d ", requestRowLen_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

    str_sprintf(buf, "outputRowLen = %d ", outputRowLen_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

    str_sprintf(buf, "childTuppIndex = %d ", (Int32) childDataTuppIndex_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);

    if (targetName_)
   {
	 char *s = targetName_;
	 str_sprintf(buf, "targetName = %s", s);
	 space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
   }

    if (hdfsHostName_)
   {
	 char *s = hdfsHostName_;
	 str_sprintf(buf, "hdfsHostName = %s", s);
	 space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
   }
    
   str_sprintf(buf,"hdfsPortNum = %d", hdfsPortNum_);
   space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(UInt16));

   if (delimiter_)
   {
	 char *s = delimiter_;
	 str_sprintf(buf, "delimiter = %s", s);
	 space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
   }

   if (header_)
   {
	 char *s = header_;
	 str_sprintf(buf, "header = %s", s);
	 space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
   }

   if (nullString_)
   {
	 char *s = nullString_;
	 str_sprintf(buf, "nullString = %s", s);
	 space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
   }

   if (recordSeparator_)
   {
	 char *s = recordSeparator_;
	 str_sprintf(buf, "recordSeparator = %s", s);
	 space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sz);
   }

   str_sprintf(buf,"numIOBuffers = %d", numIOBuffers_);
   space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(UInt16));

   str_sprintf(buf, "modTSforDir_ = %ld", modTSforDir_);
   space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
   
  } // if (flag & 0x00000008)

   displayExpression(space,flag);
   displayChildren(space,flag);

}
