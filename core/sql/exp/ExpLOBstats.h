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
#ifndef EXP_LOB_STATS_H
#define EXP_LOB_STATS_H

#include "Platform.h"

#define NUM_NSECS_IN_SEC (1 * 1000 * 1000 * 1000)

/////////////////////////////////////////////////////////////////////////////
// ExLobStats
// Lob/hdfs related stats. Returned when ExLobsOper is called with Lob_Stats oper.
// Caller need to allocate and pass in the struct. ExLobsOper will populate it.
/////////////////////////////////////////////////////////////////////////////
class ExLobStats
{
 public:
  void init();
  ExLobStats& operator+(const ExLobStats &other) ;
  void getVariableStatsInfo(char * dataBuffer,
			    char * datalen,
			    Lng32 maxLen);

  Int64 bytesToRead;
  Int64 bytesRead;
  Int64 bytesWritten;
  Int64 hdfsConnectionTime;
  Int64 CumulativeReadTime;
  Int64 hdfsAccessLayerTime;
  Int64 cursorElapsedTime;
  Int64 CumulativeWriteTime;
  Int64 AvgReadTime;
  Int64 AvgWriteTime;
  Int64 numBufferPrefetchFail;
  Int64 avgReqQueueSize;
  Int64 numReadReqs;
  Int64 numWriteReqs;
  Int64 numHdfsReqs;
  Int64 bytesPrefetched;
  Int64 buffersUsed;
};



#endif
