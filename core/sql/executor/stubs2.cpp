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
#include "Platform.h"



#if (defined (NA_LINUX) && defined(__EID))
#include "ex_ex.h"
#endif // NA_LINUX

// stubbed procedures for executor in dp2 object.
#ifdef __EID 
// NT Port - vs 01/17/97

// NT Port - vs 01/17/97
#define _resident

extern "C"
{
  
#include <setjmp.h>
  void __pure_virtual_called();
  void __vec_delete();
  char    *__vec_new();
}


// #if defined __EID && defined (NA_NSK)
// extern "C" _resident void bytesPerChar__8CharInfoSFQ2_8CharInfo7CharSet()
// {
// }
// #endif




// NT Port - vs 01/17/97 



// LCOV_EXCL_START
_resident void __pure_virtual_called()
{
}

NA_EIDPROC
void NAAssert(const char * condition, const char * file_, Int32 line_)
{
#if (defined (NA_LINUX) && defined (__EID))
assert_botch_in_eid(file_, line_, condition);
#endif // NA_LINUX & __EID
}
NA_EIDPROC
void NAAssert(char * condition, char * file_, Int32 line_)
{
#if (defined (NA_LINUX) && defined (__EID))
assert_botch_in_eid(file_, line_, condition);
#endif // NA_LINUX & __EID
}

NA_EIDPROC
void GeneratorAbort(char *f, Int32 l, char * m)
{
}

NA_EIDPROC
void NAError_stub_for_breakpoints() {}
NA_EIDPROC
void NAArkcmpExceptionEpilogue() {}

// LCOV_EXCL_STOP



// LCOV_EXCL_START

#if (defined (NA_LINUX) && defined (__EID))
#endif // NA_LINUX
#endif // __EID

// LCOV_EXCL_STOP

#pragma nowarn(1103)   //warning elimination 
// This pragma hides 1103 warnings for constructor and destructor of class VerticalPartInfo; 



