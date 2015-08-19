//------------------------------------------------------------------
//
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

#ifndef __SB_INT_DA_H_
#define __SB_INT_DA_H_

// default arg

#define SB_DA(p,v) p = v

static short const          BOMITSHORT         = -291;      // 0xfedd
static unsigned short const BOMITUSHORT        = 0xfedd;
static int const            BOMITINT           = -19070975; // oxfedd0001
static bfat_16 const        BOMITFAT_16        = -19070975; // 0xfedd0001
#if __WORDSIZE == 64
static long const           BOMITTAG           = 0xfedd000000000001;
#else
static int const            BOMITTAG           = -19070975; // 0xfedd0001
#endif
static int const            BOMITUID           = 0;

static short const          XOMITSHORT         = -291;      // 0xfedd
static unsigned short const XOMITUSHORT        = 0xfedd;
static int const            XOMITINT           = -19070975; // oxfedd0001
static xfat_16 const        XOMITFAT_16        = -19070975; // 0xfedd0001
#if __WORDSIZE == 64
static long const           XOMITTAG           = 0xfedd000000000001;
#else
static int const            XOMITTAG           = -19070975; // 0xfedd0001
#endif
static int const            XOMITUID           = 0;

#endif // !__SB_INT_DA_H_
