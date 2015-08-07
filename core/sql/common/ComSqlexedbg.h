#ifndef COMSQLEXEDBG_H
#define COMSQLEXEDBG_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComSqlcmpdbg.h
 * Description:  This file contains declarations common to arkcmp components 	
 *               and tdm_sqlcmpdbg the GUI tool used to display query 
 *               compilation.
 *               
 * Created:      06/25/97
 * Modified:     $Author:
 * Language:     C++
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

// forward reference
class ExSubtask;
class ExScheduler;
class ex_tcb;

typedef struct tagSqlexedbgExpFuncs {
  void (*fpDisplayTCBTree) (ExSubtask**, ExScheduler*);
  void (*fpSetRootTCB) (ex_tcb*);
} SqlexedbgExpFuncs;

typedef SqlexedbgExpFuncs* (*fpGetSqlexedbgExpFuncs) (void); 

#endif
	
