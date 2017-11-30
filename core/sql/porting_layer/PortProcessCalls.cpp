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
 * File:         PortProcessCalls.cpp
 * Description:  Some common Process related functions that are called by the
 *               SQL engine components
 *
 *               Includes class NAProcessHandle, this is used to encapsulate
 *               process handle differences for different platforms
 *
 * Created:      08/28/08
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include <string.h>

#include "PortProcessCalls.h"
#include "seabed/fs.h" 


// ------------------------------------------------------------------------
// Default constructor
// Setup phandle data structure and elements to extract process information
// from phandle.
// ------------------------------------------------------------------------
NAProcessHandle::NAProcessHandle()
{
  SB_Phandle_Type phandle_ ; // 64 bytes phandle       // in

  cpu_ = 0;                               
  pin_ = 0;  
  nodeNumber_ = 0;                        
  memset(nodeName_, ' ', NodeNameLen);
  nodeName_[NodeNameLen] = '\0';
  nodeNameLen_ = 0;
  memset(phandleString_, ' ', PhandleStringLen);
  phandleString_[PhandleStringLen] = '\0';
  phandleStringLen_ = 0;
  seqNum_= 0L;
}

// ------------------------------------------------------------------------
// Default constructor
// Setup phandle data structure and elements to extract process information
// from phandle. This constructor is to be used when phandle is available as
// an input paramter and there is a need to instantiate and store this phandle
// in NAProcessHandle object. 
// ------------------------------------------------------------------------
NAProcessHandle::NAProcessHandle(const SB_Phandle_Type *phandle)
{
  memcpy (&this->phandle_, phandle, PhandleSize);
  cpu_ = 0;                             
  pin_ = 0;                       
  nodeNumber_ = 0;
  memset(nodeName_, ' ', NodeNameLen);
  nodeName_[NodeNameLen] = '\0';
  nodeNameLen_ = 0;
  memset(phandleString_, ' ', PhandleStringLen);
  phandleString_[PhandleStringLen] = '\0';
  phandleStringLen_ = 0;
  seqNum_= 0L; 
}



// ------------------------------------------------------------------------------
// DECOMPOSE
// ------------------------------------------------------------------------------
// After decomposing phandle, various phandle components can be extracted 
// using accessor methods provided.
//
// If an error occurs, return the appropriate file system error
// ------------------------------------------------------------------------------
short NAProcessHandle::decompose()
{
  Int32 err = 0;
  char processName[PhandleStringLen];
  processName[0] = '\0';
  short processNameLen = 0;

  err = XPROCESSHANDLE_DECOMPOSE_((SB_Phandle_Type *)&(this->phandle_)
                                  ,&this->cpu_
                                  ,&this->pin_
                                  ,(Int32 *) &this->nodeNumber_
                                  ,this->nodeName_
                                  ,NodeNameLen
                                  ,&this->nodeNameLen_
                                  ,processName
                                  ,PhandleStringLen
                                  ,&processNameLen
                                   ,(SB_Int64_Type *)&this->seqNum_
                                  );
  processName[processNameLen] = '\0';
  this->phandleStringLen_ = strlen(processName);
  strcpy(this->phandleString_, processName);
  this->phandleString_[this->phandleStringLen_] = '\0';

  return err;
}


// ---------------------------------------------------------------------------
// GETMINE
// ---------------------------------------------------------------------------
//
// Keeps a copy of phandle in the NAProcessHandle instance and copies to
// phandle input parameter to be returned to the caller.
//
// If an error occurs, return the appropriate file system error
// ---------------------------------------------------------------------------
short NAProcessHandle::getmine(SB_Phandle_Type *phandle)
{
  Int32 err=0;
  
  err = XPROCESSHANDLE_GETMINE_( (SB_Phandle_Type *) &this->phandle_);
  
  memcpy (phandle, &this->phandle_, PhandleSize); 
  
  return err;
}

short NAProcessHandle::getmine()
{
  short  err=0;
  
  err = XPROCESSHANDLE_GETMINE_( (SB_Phandle_Type *) &this->phandle_);
  return err;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
// Keeps a copy of phandle in the NAProcessHandle instance and copies to
// phandle input parameter to be returned to the caller.
//
// If an error occurs, return the appropriate file system error
// ---------------------------------------------------------------------------
short NAProcessHandle::nullit(SB_Phandle_Type *phandle)
{

  Int32 err=0;
  
  err = XPROCESSHANDLE_NULLIT_( (SB_Phandle_Type *) &this->phandle_);
  
  memcpy (phandle, &this->phandle_, 20);
  
  return err;
}

//-----------------------------------------------------
// Set phandle in NAProcessHandle to callers phandle
//-----------------------------------------------------
void NAProcessHandle::setPhandle(SB_Phandle_Type *phandle)
{
  memcpy (&this->phandle_, phandle, PhandleSize);
}

char *NAProcessHandle::getNodeName()
{
  return nodeName_;
}

short NAProcessHandle::getNodeNameLen()
{
  if (nodeNameLen_ == 0)
    getNodeName();

  return nodeNameLen_;
}

char *NAProcessHandle::getPhandleString()
{
  return phandleString_;
}

short NAProcessHandle::getPhandleStringLen()
{
  if (phandleStringLen_ == 0)
    getPhandleString();

  return phandleStringLen_;
}
