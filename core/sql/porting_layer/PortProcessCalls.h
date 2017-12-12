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
 * File:         PortProcessCalls.h
 * Description:  Some common Process related functions that are called by the
 *               SQL engine components
 *
 *               Includes class NAProcessHandle. This is used to encapsulate 
 *               process handle.
 *               See comments in class heading for more details.
 *
 * Created:      08/28/08
 * Language:     C++
 *
 *
 *****************************************************************************
 */
#ifndef PORTPROCESSCALLS_H
#define PORTPROCESSCALLS_H

#include "Platform.h"
#include "seabed/fs.h"

#define PhandleSize 64
#define PhandleStringLen (MS_MON_MAX_PROCESS_NAME + 1 + 20 + 1)
#define NodeNameLen 9

//
// Class NAProcessHandle:
//
// This class encapsulates the differences between platforms for the 
// Process Handle.
//
//  On LINUX, the PHANDLE is 64 bytes
//
// ------------------------------------------------------------------------
class NAProcessHandle
{
  public:

    // Constructor
    NAProcessHandle();

    NAProcessHandle(const SB_Phandle_Type * phandle);

    // Destructor
    virtual ~NAProcessHandle (void) {};

    // Guardian procedure call wrappers
    // Add new methods to support any other guardian procedure calls 
    // in this section

    // Wrapper for PROCESSHANDLE_DECOMPOSE_
    short decompose(); 

    // Wrapper for PROCESSHANDLE_GETMINE_
    short getmine( SB_Phandle_Type * phandle);

    short getmine();

    // Wrapper for PROCESSHANDLE_NULLIT_
    short nullit( SB_Phandle_Type * phandle);

  // Accessors to access various process handle components
    SB_Phandle_Type *getPhandle() { return &phandle_; }
    Int32 getCpu() {return cpu_;}
    Int32 getPin() {return pin_;}
    Int64 getSeqNum() {return seqNum_;}
    void setPhandle( SB_Phandle_Type * phandle);

    Lng32 getNodeNumber() {return nodeNumber_;}
    Lng32 getSegment() {return nodeNumber_;}
    char * getNodeName();
    short getNodeNameLen();
    char * getPhandleString();
    short getPhandleStringLen();

  private:

  SB_Phandle_Type phandle_;  // 64 bytes phandle  
  Int32         cpu_;                               
  Int32         pin_;                               
  Int64   seqNum_;  

  Lng32        nodeNumber_;                        
  char        nodeName_[NodeNameLen+1];         
  short       nodeNameLen_;                       
  char        phandleString_[PhandleStringLen+1];               
  short       phandleStringLen_;                      
};


#endif // PORTPROCESSCALLS_H
