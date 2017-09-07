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
#ifndef LMCOMMON_H
#define LMCOMMON_H
/* -*-C++-*-
******************************************************************************
*
* File:         LmCommon.h
* Description:  Helper functions and other code shared by all LM
*               source modules
* Created:      June 2003
* Language:     C++
*
******************************************************************************
*/
#include "ComSmallDefs.h"
#include "ComDiags.h"

#ifdef LMCOMP_BUILD
  // This is for the lmcomp library which gets statically linked
  // into tdm_arkcmp
  #define SQLLM_LIB_FUNC
#else
  #ifdef LM_DLL
    // This is for building the LM DLL
    #define SQLLM_LIB_FUNC __declspec( dllexport )
  #else
    // This is for LM DLL callers
    #define SQLLM_LIB_FUNC __declspec( dllimport )
  #endif
#endif

//////////////////////////////////////////////////////////////////////
//
// Primitives used by LM files
//
//////////////////////////////////////////////////////////////////////
typedef void* LmHandle;

enum {
  LMJ_ERR_SIZE_256     = 256,
  LMJ_ERR_SIZE_512     = 512,
  LMJ_DEF_OPT_SIZE     = 32,
  LMJ_NT_FILE_PATH_LEN = 256,
};

enum LmResultSetMode {
  RS_NONE,
  RS_SET
};

enum LmResultSetType {
  RS_TYPE_FORWARD_ONLY,
  RS_TYPE_SCROLL_INSENSITIVE,
  RS_TYPE_SCROLL_SENSITIVE,
  RS_TYPE_UNKNOWN = -1
};

// Forward declarations
class NAMemory;
class LmCBuffer;

// copy_string(): create a copy of a string on the specified heap. A
// pointer to the copy is returned.
char *copy_string(NAMemory *heap, const char *src);

// copy_chars(): create a copy of given chars on the specified heap.
// Use the C runtime heap if heap is NULL. Target string is null
// terminated if terminate flag is TRUE. A pointer to the copy is
// returned.
char *copy_chars(NAMemory *heap,
                 const char *src,
		 ComUInt32 len,
		 NABoolean terminate = FALSE);

// copy_and_pad(): create a copy of the given buffer on the C runtime
// heap. Optionally add zeroed-out padding at the end.
char *copy_and_pad(const char *src, ComUInt32 len, ComUInt32 pad = 0);

// strip_spaces(): strips leading and trailing spaces in a string. A
// pointer to the first non-space character is returned. If there are
// trailing spaces, the leftmost one is changed to '\0'.
char *strip_spaces(char *str);

// This function maps INTERVAL FS datatypes to the corresponding
// INTERVAL codes from sqlcli.h
Lng32 getIntervalCode(short fstype);

// A helper class to manage buffers on the C runtime heap. These
// buffers are the ones we expose to C routines. Every buffer is
// aligned on an 8-byte boundary. Every buffer contains extra bytes at
// the end to protect against the routine body from overwriting the
// buffer and corrupting some other buffer.
class LmCBuffer
{
public:
  LmCBuffer(ComUInt32 len = 0);
  virtual ~LmCBuffer();

  // Release the existing buffer and allocate a new one
  char *init(ComUInt32 len);

  // Return a pointer to the buffer
  char *getBuffer() const { return buf_; }

  // Initialize all bytes of the buffer, similar to memset()
  void set(Int32 c);

  // Release the buffer
  void release();

protected:
  char *buf_;
  ComUInt32 len_;

};

void addDllErrors(ComDiagsArea &diags,
                  const char *operation,
                  NABoolean isWarningOnly);

LmHandle loadDll(
  const char   *containerName,
  const char   *externalPath,
  LmHandle     extLoader,
  ComUInt32    *containerSize,
  ComDiagsArea *da,
  NAMemory *heap);

void unloadDll(LmHandle containerHandle, ComDiagsArea *da);

LmHandle getRoutinePtr(
  LmHandle     container,
  const char   *routineName) ;

#endif // LMCOMMON_H
