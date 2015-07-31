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
#ifndef __TM_UTIL_H
#define __TM_UTIL_H

#undef int16
#define int16 short
#undef uint16
#define uint16 unsigned short
#undef int32
#define int32 int
#undef uint32
#define uint32 unsigned int

#undef int64
#if __WORDSIZE == 64
  #define int64 long
  #define uint64 unsigned long
  #ifndef PFLLX
    #define PFLLX "%lx"
  #endif
  #ifndef PFLLU
    #define PFLLU "%lu"
  #endif
#else
  #define int64 long long
  #define uint64 unsigned long long
  #ifndef PFLLX
    #define PFLLX "" PFLLX ""
  #endif
  #ifndef PFLLU
    #define PFLLU "%llu"
  #endif
#endif

#endif
