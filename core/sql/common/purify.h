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

// -------------------------------------------------------------------------
// This .h file will define Pure Software Inc.'s purify API 
// (functions such as purify_watch)
// if you have modified the appropriate makefile (common/makedefs.$TARGTYPE)
// from:
//	CPLUS= nice CC
// to:
//	CPLUS= purify CC -DPURIFY
// -------------------------------------------------------------------------

#ifdef PURIFY
  // The path below works on SunOS Unix.
  // Probably need some #ifdef's to set this path correctly for OSS and GDN.
  #include "/usr/vendor/purify/purify.h"
#endif
