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
#include "CmpProcess.h"
#include "ComRtUtils.h"
#include "nsk/nskport.h"
#include "seabed/ms.h"
#include "seabed/fs.h"
#include <cstdlib>

// -----------------------------------------------------------------------
// Methods for class CmpProcess
// -----------------------------------------------------------------------

/************************************************************************
constructor CmpProcess

 Get some basic information about this process and setup the class.
 The process info gathered below only needs to happen once.

************************************************************************/
CmpProcess::CmpProcess()
: nodeNum_(0), 
  pin_(0), 
  segmentNum_(0),  
  processStartTime_(0) 
{

  char programDir[100];
  short processType;
  int myCPU;
  char myNodeName[MAX_SEGMENT_NAME_LEN+1];
  Lng32  myNodeNum;
  short myNodeNameLen = MAX_SEGMENT_NAME_LEN;
  char myProcessName[PROCESSNAME_STRING_LEN];
  pid_t pid;
  if (!ComRtGetProgramInfo(programDir,100,processType,myCPU,
                          pid,myNodeNum,myNodeName,myNodeNameLen,
                          processStartTime_,myProcessName))
  {
    pin_ = pid;
    nodeNum_ = myNodeNum;
    // convert to local civil time
    processStartTime_ = CONVERTTIMESTAMP(processStartTime_,0,-1,0);
  }
}
/************************************************************************
method CmpProcess::getProcessDuration

  the number of microseconds since this process started

************************************************************************/
Int64
CmpProcess::getProcessDuration()
{
  Int64 currentTime = getCurrentTimestamp();
  return (currentTime - processStartTime_);
}
/************************************************************************
method CmpProcess::getCurrentSystemHeapSize

 Guardian procedure calls used to get the latest memory usage information
 for this process. 

************************************************************************/
Lng32
CmpProcess::getCurrentSystemHeapSize()
{
  Lng32 currentSystemHeapSize = 0;

  return currentSystemHeapSize;
}
/************************************************************************
method CmpProcess::getCompilerId

parameter:
   char *id - pass an empty buffer in and get the compiler id as output

 the compiler id is made up of:

 process start timestamp - 18 digits
 node (cpu) #            - 2 digits
 pin #                   - 4 digits
 segment #               - 3 digits


************************************************************************/
void
CmpProcess::getCompilerId(char *id)
{    
  CMPASSERT( NULL != id );

  str_sprintf
    (id, "%018ld%02d%04d%03d",
     getProcessStartTime(),        
     getNodeNum(),                      
     getPin(),                         
     getSegmentNum());                     
}
