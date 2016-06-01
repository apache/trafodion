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
#ifndef EX_TCB_PRIVATE_H
#define EX_TCB_PRIVATE_H


/* -*-C++-*-
******************************************************************************
*
* File:         ex_tcb_privte.h
* Description:  Class declaration for ex_tcb_private
*               
*               
* Created:      5/3/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "Platform.h"

// external reference
class ex_tcb;

//////////////////////////////////////////////////////////////////////////
//
// Each ex_tcb class can store in an input queue a state associated 
// with that input row.
// All such states should be a subclass of this one
//
// Allocate_new acts like a virtual constructor given another object 
// of the desired sub-class.
//
//////////////////////////////////////////////////////////////////////////

#pragma nowarn(1103)   // warning elimination 
class ex_tcb_private_state : public ExGod
#pragma warn(1103)  // warning elimination 
{
  // Error related information. For now, make it a long.
  // Later, make it the SQLDiagnosticStruct (or something similar).
  Lng32 errorCode_;

public:
NA_EIDPROC
  ex_tcb_private_state();
NA_EIDPROC
  virtual ex_tcb_private_state * allocate_new(const ex_tcb * tcb);
NA_EIDPROC
  virtual ~ex_tcb_private_state();

NA_EIDPROC
  inline Lng32 getErrorCode(){return errorCode_;}
NA_EIDPROC
  inline void setErrorCode(Lng32 error_code){errorCode_ = error_code;}

};


#endif
