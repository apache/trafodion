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
****************************************************************************
*
* File:         ExportDll.cpp
* RCS:          $Id: ExportDll.cpp,v 1.1 1998/06/29 03:50:39  Exp $
* Description:  "main()" for tdm_sqlexport.dll
*
* Created:      5/6/98
* Modified:     $ $Date: 1998/06/29 03:50:39 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
*
****************************************************************************
*/

// Buf is defined here and exported. Its value is initialized by
// the topmost executable linking this DLL. (e.g. arkcmp.cpp/sqlci.cpp...)
//

#include "Platform.h"
#include <setjmp.h>

THREAD_P jmp_buf ExportJmpBuf;
