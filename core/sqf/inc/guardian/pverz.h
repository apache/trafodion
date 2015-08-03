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
// PREPROC: start of file: file guard
#if (!defined(pverz_h_included_already) || defined(pverz_h_including_section) || defined(pverz_h_including_self))
//
//
#if (!defined(pverz_h_including_self) && !defined(pverz_h_including_section))
#define pverz_h_included_already
#endif
// 
//
// PREPROC: start of section: 
#if (defined(pverz_h_) || (!defined(pverz_h_including_section) && !defined(pverz_h_including_self)))
#undef pverz_h_
//

#include "rosetta/rosgen.h" /* rosetta utilities */
//
#endif
// PREPROC: end of section: 
//
// #pragma section TOSVERSION
//
// PREPROC: start of section: tosversion
#if (defined(pverz_h_tosversion) || (!defined(pverz_h_including_section) && !defined(pverz_h_including_self)))
#undef pverz_h_tosversion
//
#ifndef ALREADY_SOURCE_IN_TOSVERSION_
#define ALREADY_SOURCE_IN_TOSVERSION_
#ifdef __cplusplus
extern "C"
#endif
DllImport
int_16 TOSVERSION();
#endif // ALREADY_SOURCE_IN_TOSVERSION_
//
#endif
// PREPROC: end of section: tosversion
//
// #pragma section NSK_MINOR_VERSION_
//
// PREPROC: start of section: nsk_minor_version_
#if (defined(pverz_h_nsk_minor_version_) || (!defined(pverz_h_including_section) && !defined(pverz_h_including_self)))
#undef pverz_h_nsk_minor_version_
//
#ifndef ALREADY_SOURCE_IN_NSK_MINOR_VERSION_
#define ALREADY_SOURCE_IN_NSK_MINOR_VERSION_
DllImport
int_16 NSK_MINOR_VERSION_();
#endif  // ALREADY_SOURCE_IN_NSK_MINOR_VERSION_
//
#endif
// PREPROC: end of section: nsk_minor_version_
//
// #pragma section VERSIONERROR
//
// PREPROC: start of section: versionerror
#if (defined(pverz_h_versionerror) || (!defined(pverz_h_including_section) && !defined(pverz_h_including_self)))
#undef pverz_h_versionerror
//
#ifndef ALREADY_SOURCE_IN_VERSIONERROR_
#define ALREADY_SOURCE_IN_VERSIONERROR_
DllImport
int_16 VERSIONERROR
  (int_16  prodvers,
   int_16  guardvers); //~ source file above = $QUINCE.GRZDV.SVER
#endif // ALREADY_SOURCE_IN_VERSIONERROR_

#endif
// PREPROC: end of section: versionerror
//
//
#if (!defined(pverz_h_including_self))
#undef pverz_h_including_section
#endif

#endif // file guard
// end of file
