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
//
**********************************************************************/
#ifndef _MXCI_EH_CALL_BACK_H_
#define _MXCI_EH_CALL_BACK_H_
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         MxciEHCallBack.cpp
 * Description:  Mxci Call back functions for exception handling
 *               
 *               
 * Created:      5/4/02
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "EHCallBack.h"

class MxciEHCallBack : public EHCallBack {
public:
  virtual void doFFDC();
  virtual void dumpDiags();
};

extern MxciEHCallBack GMxciEHCallBack;

#endif





