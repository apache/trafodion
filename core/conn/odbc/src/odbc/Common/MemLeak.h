/*************************************************************************
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
**************************************************************************/

#ifndef MEMLEAK
#define MEMLEAK
//LCOV_EXCL_START

//define TRACE_MEMORY_LEAK_QS
//define TRACE_MEMORY_LEAK_STATS
//define TRACE_MEMORY_LEAK_RULE
//define TRACE_MEMORY_LEAK_COM
//define TRACE_MEMORY_LEAK_SYNC
//define TRACE_MEMORY_LEAK_OFFNDR

#ifndef _DEBUG
#undef TRACE_MEMORY_LEAK_QS
#undef TRACE_MEMORY_LEAK_STATS
#undef TRACE_MEMORY_LEAK_RULE
#undef TRACE_MEMORY_LEAK_COM
#undef TRACE_MEMORY_LEAK_SYNC
#undef TRACE_MEMORY_LEAK_OFFNDR
#endif


//====================== Memory Leak Trace =============================
void ListAll(char* description, bool bskip = false);
void ListLastAdded(char* decription, bool bskip = false);
void markNewLine(char* file,char* function, long line);

#define MARK_NEW_LINE\
	markNewLine(__FILE__,__FUNCTION__, __LINE__)

//LCOV_EXCL_STOP
#endif
