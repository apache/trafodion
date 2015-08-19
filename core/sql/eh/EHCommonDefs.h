/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         EHCommonDefs.h
 * Description:  common definitions -- This file was derived from the file
 *               CommonDefs.h 
 *
 * Created:      6/20/95
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#ifndef EHCOMMONDEFS_H
#define EHCOMMONDEFS_H

#include "Platform.h"  // 64-BIT
#include "EHBaseTypes.h"

// -----------------------------------------------------------------------
// Declare a Boolean type
// (compatible with standard C++ boolean expressions) 
// -----------------------------------------------------------------------

#ifndef EHBOOLEAN_DEFINED
#define EHBOOLEAN_DEFINED
typedef Int32 EHBoolean;
#endif

#ifndef TRUE
#ifndef TRUE_DEFINED
#define TRUE_DEFINED
const EHBoolean		TRUE = (1 == 1);
#endif
#endif

#ifndef FALSE
#ifndef FALSE_DEFINED
#define FALSE_DEFINED
const EHBoolean		FALSE = (0 == 1);
#endif
#endif

// -----------------------------------------------------------------------
// C++ operators in a more readable form
// -----------------------------------------------------------------------

#ifndef EQU
#define EQU		==
#endif
#ifndef NEQ
#define NEQ		!=
#endif
#ifndef NOT
#define NOT		!
#endif
#ifndef AND
#define AND		&&
#endif
#ifndef OR
#define OR		||
#endif


// -----------------------------------------------------------------------
// Abnormal program termination (EHAbort defined in EHAbort.C)
// -----------------------------------------------------------------------

#ifndef EH_ABORT
#define EH_ABORT(msg)	EHAbort (__FILE__, __LINE__, (msg))
#endif
void EHAbort(const char * filename, Int32 lineno, const char * msg);

// -----------------------------------------------------------------------
// Abort program if AssertTruth condition fails
// -----------------------------------------------------------------------

#ifndef EH_ASSERT
#define EH_ASSERT(cond)	{ if (NOT(cond)) \
	EHAbort (__FILE__, __LINE__, "AssertTruth condition failed"); }
#endif

#endif // EHCOMMONDEFS_H
