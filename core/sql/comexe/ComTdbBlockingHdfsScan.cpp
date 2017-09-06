// **********************************************************************
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
// **********************************************************************

#include "ComTdbBlockingHdfsScan.h"
#include "ComTdbCommon.h"

// Dummy constructor for "unpack" routines.
ComTdbBlockingHdfsScan::ComTdbBlockingHdfsScan():
 ComTdb(ComTdb::ex_BLOCK_HDFS_SCAN, eye_BLOCK_HDFS_SCAN)
{};

// Constructor

  ComTdbBlockingHdfsScan::ComTdbBlockingHdfsScan(
                 ex_expr * select_pred,
		 ex_expr * move_expr,
                 ex_expr * convert_expr,
                 char * hostName,
                 tPort port,
                 char * hdfsFileName, 
                 char recordDelimiter,
                 char columnDelimiter,
		 Int64 hdfsOffset,
		 Int64 hdfsLength,
		 Int64 hdfsSqlMaxRecLen,
                 Int64 outputRowLength,
		 Int64 asciiRowLen,
		 const unsigned short tuppIndex,
		 const unsigned short asciiTuppIndex,
		 const unsigned short workAtpIndex,
		 ex_cri_desc * work_cri_desc,
		 ex_cri_desc * given_cri_desc,
		 ex_cri_desc * returned_cri_desc,
		 queue_index down,
		 queue_index up,
		 Cardinality estimatedRowCount,
		 Int32  numBuffers,
		 UInt32  bufferSize
                 )

: ComTdb( ComTdb::ex_BLOCK_HDFS_SCAN,
            eye_BLOCK_HDFS_SCAN,
            estimatedRowCount,
            given_cri_desc,
            returned_cri_desc,
            down,
            up,
            numBuffers,        // num_buffers - we use numInnerTuples_ instead.
            bufferSize),       // buffer_size - we use numInnerTuples_ instead.
    selectPred_(select_pred),
    moveExpr_(move_expr),
    convertExpr_(convert_expr),
    hostName_(hostName),
    port_(port),
    hdfsFileName_(hdfsFileName),
    recordDelimiter_(recordDelimiter),
    columnDelimiter_(columnDelimiter),
    hdfsOffset_(hdfsOffset),
    hdfsLength_(hdfsLength),
    hdfsSqlMaxRecLen_(hdfsSqlMaxRecLen),
    outputRowLength_(outputRowLength),
    asciiRowLen_(asciiRowLen),
    tuppIndex_(tuppIndex),
    asciiTuppIndex_(asciiTuppIndex),
    workAtpIndex_(workAtpIndex),
    workCriDesc_(work_cri_desc),
    flags_(0)
{};

ComTdbBlockingHdfsScan::~ComTdbBlockingHdfsScan()
{};

void ComTdbBlockingHdfsScan::display() const 
{};

Long ComTdbBlockingHdfsScan::pack(void * space)
{
  selectPred_.pack(space);
  moveExpr_.pack(space);
  convertExpr_.pack(space);
  hostName_.pack(space);
  hdfsFileName_.pack(space);
  workCriDesc_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbBlockingHdfsScan::unpack(void * base, void * reallocator)
{
  if(selectPred_.unpack(base, reallocator)) return -1;
  if(moveExpr_.unpack(base, reallocator)) return -1;
  if(convertExpr_.unpack(base, reallocator)) return -1;
  if(hostName_.unpack(base)) return -1;
  if(hdfsFileName_.unpack(base)) return -1;
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
}

void ComTdbBlockingHdfsScan::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);

  if(flag & 0x00000008)
    {
      char buf[2048];

      str_sprintf(buf, "\nFor ComTdbBlockingHdfsScan :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "\nhostName_ = %s, port_ = %d", 
                       (char *) hostName_, port_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "\nhdfsFileName_ = %s", (char *) hdfsFileName_ );
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "\nrecordDelimiter_ = %d, columnDelimiter_ = %d", 
                         recordDelimiter_ ,  columnDelimiter_ );
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "\nhdfsOffset_ = %d, hdfsLength_ = %d",
                         hdfsOffset_ ,  hdfsLength_ );
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    }

  if(flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

Int32 ComTdbBlockingHdfsScan::orderedQueueProtocol() const
{
  return 1;
}


