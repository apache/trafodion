/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciCSSimulator.cpp
 * Description:  Methods to implement MACL commands on the NT platform.
 *               These are stubs for the funtions available in macl/CSInterface.cpp.
 *               
 *               
 * Created:      11/10/2003
 * Language:     C++
 * Status:       
 *
 *
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
 *
 *
 *****************************************************************************
 */

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "sqlcmd.h"



class MXCSEnv
{
public:
  MXCSEnv();
  ~MXCSEnv();

};

MXCSEnv::MXCSEnv()
{
}

MXCSEnv::~MXCSEnv()
{
}

Lng32 CS_MXCI_Constructor (void* &CSEnv)
{
  MXCSEnv * csEnv = new MXCSEnv();

  CSEnv = csEnv;

  return CSSUCCESS;
}

Lng32 CS_MXCI_Destructor (void* CSEnv)
{
  delete (MXCSEnv *)CSEnv;

  return CSSUCCESS;
}

Lng32 CS_MXCI_sendQuery (void* CSEnv, char *cmd, Lng32 len)
{
  MXCSEnv * csEnv = (MXCSEnv *)CSEnv;

    return CSGET_OUTPUT_ROW;
}


Lng32 CS_MXCI_handleBreak(void *CSEnv)
{
  MXCSEnv * csEnv = (MXCSEnv *)CSEnv;

    return CSSUCCESS;
}

Lng32 CS_MXCI_getErrorInfo(void* CSEnv, CSErrorValue* &err)
{
  MXCSEnv * csEnv = (MXCSEnv *)CSEnv;

    return CSSUCCESS;
}

Lng32 CS_MXCI_getReportLine(void *CSEnv, char* &line, Lng32 &len)
{
  MXCSEnv * csEnv = (MXCSEnv *)CSEnv;

    return CSSUCCESS;
}

void CS_MXCI_StopContext(void *CSEnv)
{
  MXCSEnv * csEnv = (MXCSEnv *)CSEnv;

}
