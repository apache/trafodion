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
// PREPROC: start of section: 
#if (defined(kphandlz_h_) || (!defined(kphandlz_h_including_section) && !defined(kphandlz_h_including_self)))
#undef kphandlz_h_
//

#include "rosetta/rosgen.h" /* rosetta utilities */


//
#endif
// PREPROC: end of section: 
//
// #pragma section PROCESSHANDLE_TEMPLATE_
//
// PREPROC: start of section: processhandle_template_
#if (defined(kphandlz_h_processhandle_template_) || (!defined(kphandlz_h_including_section) && !defined(kphandlz_h_including_self)))
#undef kphandlz_h_processhandle_template_
//

#pragma page "T9050 Guardian PROCESSHANDLE_TEMPLATE_"
#pragma fieldalign shared2 NSK_PHandle

// following definition is from Rosetta and should not been
// used for any NSK-lite API, since the phandle definition
// has been changed.

/*
class NSK_PHandle { // struct processhandle_template_
public:
  _redefarray
    (int_16,RESERVED_type,0,-1,_tobitfield(NSK_PHandle,_filler)); // Type and reserved bits
  unsigned short type:4;							// Process handle type
  unsigned short _filler:4;							// Reserved
  unsigned short _filler1:8;					  // Reserved
  char         _filler2[18];                                      // never accessed directly
}; // struct processhandle_template_
*/

///////////////////////////////////////////////////////////////////////////////////
//  
// <NOTE!!> 
//		Following is the new definition for NSK_PHandle, the size of NSK_PHandle
//		should be the same as NSK_PORT_PHANDLE defined in NSKport.h. Also the 
//		location(offset) for the fields(such as type) should be the same too.
//		otherwise the client program will fail.
//
//		as of current(5/13/97), the size of NSK_PORT_HANDLE is 20 bytes
//
///////////////////////////////////////////////////////////////////////////////////

class NSK_PHandle { // struct processhandle_template_
public:
  unsigned char type; // process handle type

  inline int_16 &RESERVED_type() { return _coerce(int_16, type); }

  char _filler[19];
};

enum {PROCESSHANDLE_UNNAMED = 1};
enum {PROCESSHANDLE_NAMED = 2};
enum {PROCESSHANDLE_OLD_UNNAMED = 3};
enum {PROCESSHANDLE_OLD_NAMED = 4};
enum {PROCESSHANDLE_OLD_LDEV = 5};
enum {PROCESSHANDLE_NULL = 15};

#endif
// PREPROC: end of section: processhandle_template_
//
//
#if (!defined(kphandlz_h_including_self))
#undef kphandlz_h_including_section
#endif
// end of file
