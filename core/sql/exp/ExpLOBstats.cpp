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
#include <stdio.h>
#include "ExpLOBstats.h"
#include "str.h"

/////////////////////////////////////////////////////////////////////////////
// ExLobStats
// Lob/hdfs related stats. Returned when ExLobsOper is called with Lob_Stats oper.
// Caller need to allocate and pass in the struct. ExLobsOper will populate it.
/////////////////////////////////////////////////////////////////////////////

void ExLobStats::init()
  {
    bytesToRead = 0;
    bytesRead = 0;
    bytesWritten = 0;
    CumulativeReadTime = 0;
    CumulativeWriteTime = 0;
    hdfsAccessLayerTime = 0;
    hdfsConnectionTime = 0;
    cursorElapsedTime = 0;
    AvgReadTime = 0;
    AvgWriteTime = 0;
    numBufferPrefetchFail = 0;
    avgReqQueueSize = 0;
    numReadReqs = 0;
    numHdfsReqs = 0;
    numWriteReqs = 0;
    bytesPrefetched = 0;
    buffersUsed = 0;
  };

ExLobStats& ExLobStats::operator+(const ExLobStats &other)
{
  bytesToRead  += other.bytesToRead;
  bytesRead  += other.bytesRead;
  numReadReqs += other.numReadReqs;
  CumulativeReadTime += other.CumulativeReadTime;
  cursorElapsedTime += other.cursorElapsedTime;
  bytesPrefetched += other.bytesPrefetched;
  hdfsAccessLayerTime += other.hdfsAccessLayerTime;
  hdfsConnectionTime += other.hdfsConnectionTime;
  numHdfsReqs += other.numHdfsReqs;
  buffersUsed += other.buffersUsed;

  return *this;
}

void ExLobStats::getVariableStatsInfo(char * dataBuffer,
				      char * datalen,
				      Lng32 maxLen)
{
  char * buf = dataBuffer;

  sprintf(buf, " BytesRead: %ld", bytesRead);
  buf += str_len(buf);

  sprintf(buf, " BytesToRead: %ld", bytesToRead);
  buf += str_len(buf);

  sprintf(buf, " BytesPrefetched: %ld", bytesPrefetched);
  buf += str_len(buf);

  sprintf(buf, " NumReadReqs: %ld", numReadReqs);
  buf += str_len(buf);

  sprintf(buf, " NumHdfsReqs: %ld", numHdfsReqs);
  buf += str_len(buf);

  sprintf(buf, " CumulativeReadTime: %10.6f secs", ((double)CumulativeReadTime/NUM_NSECS_IN_SEC));
  buf += str_len(buf);

  sprintf(buf, " hdfsConnectionTime: %10.6f secs", ((double)hdfsConnectionTime/NUM_NSECS_IN_SEC));
  buf += str_len(buf);

  sprintf(buf, " hdfsAccessLayerTime: %10.6f secs", ((double)hdfsAccessLayerTime/NUM_NSECS_IN_SEC));
  buf += str_len(buf);

  sprintf(buf, " cursorElapsedTime: %10.6f secs", ((double)cursorElapsedTime/NUM_NSECS_IN_SEC));
  buf += str_len(buf);

  sprintf(buf, " buffersUsed: %ld ", buffersUsed);
  buf += str_len(buf);

  *(short*)datalen = (short) (buf - dataBuffer);
}
