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
#ifndef _MXTraceDef_H_
#define _MXTraceDef_H_

#ifdef NA_LINUX_DEBUG
	#include "seabed/trace.h"
	class MXTrcHelper
	{
		char sName_[200];
	public:
		MXTrcHelper(const char* name)
		{
			strcpy(sName_, name?name:"UNKNOWN");
			trace_printf("%s begin\n", sName_);
		}
		~MXTrcHelper()
		{
			trace_printf("%s end\n", sName_);
		}
	};
	#define MXTRC_FUNC(F)	\
		MXTrcHelper _mxtrc_func_(F)
	#define MXTRC(F) trace_printf(F)
	#define MXTRC_1(F,P1) trace_printf(F,P1)
	#define MXTRC_2(F,P1,P2) trace_printf(F,P1,P2)
	#define MXTRC_3(F,P1,P2,P3) trace_printf(F,P1,P2,P3)
	#define MXTRC_4(F,P1,P2,P3,P4) trace_printf(F,P1,P2,P3,P4)
	#define MXTRC_5(F,P1,P2,P3,P4,P5) trace_printf(F,P1,P2,P3,P4,P5)
	#define MXTRC_6(F,P1,P2,P3,P4,P5,P6) trace_printf(F,P1,P2,P3,P4,P5,P6)
	#define MXTRC_7(F,P1,P2,P3,P4,P5,P6,P7) trace_printf(F,P1,P2,P3,P4,P5,P6,P7)
	#define MXTRC_8(F,P1,P2,P3,P4,P5,P6,P7,P8) trace_printf(F,P1,P2,P3,P4,P5,P6,P7,P8)
#else
	#define MXTRC_FUNC(F)
	#define MXTRC(F)
	#define MXTRC_1(F,P1)
	#define MXTRC_2(F,P1,P2)
	#define MXTRC_3(F,P1,P2,P3)
	#define MXTRC_4(F,P1,P2,P3,P4)
	#define MXTRC_5(F,P1,P2,P3,P4,P5)
	#define MXTRC_6(F,P1,P2,P3,P4,P5,P6)
	#define MXTRC_7(F,P1,P2,P3,P4,P5,P6,P7)
	#define MXTRC_8(F,P1,P2,P3,P4,P5,P6,P7,P8)
#endif
#endif
