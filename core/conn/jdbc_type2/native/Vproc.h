/**************************************************************************
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
**************************************************************************/

#ifndef VPROC_H
#define VPROC_H
#include "SCMBuildStr.h"
#include "SCMVersHelp.h"
#define JDBC_VERS_BUILD1(name,branch) #name #branch
#define JDBC_VERS_BUILD2(name,branch) name ## branch
#define JDBC_VERS1(name,branch) JDBC_VERS_BUILD1(name,branch)
#define JDBC_VERS2(name,branch) JDBC_VERS_BUILD2(name,branch)
extern const char *driverVproc;
extern "C" void JDBC_VERS2(Traf_JDBC_Type2_Build_,VERS_BR3)(void);
#endif /* VPROC_H */
