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
//
**********************************************************************/

#ifndef _WINDOWS
#include <sys/time.h>
#else
#include <windows.h>
#include "timeval.h"
#endif

#include "utils.h"

// Utility Functions

unsigned long long get_msts()
{
	unsigned long long val = 0;
	struct timeval t;
	gettimeofday(&t,NULL);
	val = (unsigned long long) t.tv_sec * 1000000 + t.tv_usec;
	return val;
}

int is_little_endian(void) {
	union {
		long l;
		unsigned char uc[sizeof(long)];
	} u;

	u.l = 1;
	return u.uc[0];
}

int swapByteOrderOfS(unsigned int us)
{
    return   ((us) << 24) | \
             (((us) << 8) & 0x00ff0000) | \
             (((us) >> 8) & 0x0000ff00) | \
             ((us) >> 24) ;
}

unsigned long long swapByteOrderOfLL(unsigned long long ull)
{
    return (ull >> 56) |
          ((ull<<40) && 0x00FF000000000000LL) |
          ((ull<<24) && 0x0000FF0000000000LL) |
          ((ull<<8) && 0x000000FF00000000LL) |
          ((ull>>8) && 0x00000000FF000000LL) |
          ((ull>>24) && 0x0000000000FF0000LL) |
          ((ull>>40) && 0x000000000000FF00LL) |
          (ull << 56);
}

