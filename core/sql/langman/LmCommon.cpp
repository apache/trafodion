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
/* -*-C++-*-
******************************************************************************
*
* File:         LmCommon.cpp
* Description:  Helper functions and other code shared by all LM
*               source modules
* Created:      June 2003
* Language:     C++
*
******************************************************************************
*/

#include "Platform.h"
#include "ComSmallDefs.h"
#include "str.h"
#include "NAMemory.h"
#include "sqlcli.h"
#include "LmCommon.h"
#include "ComDefs.h"
#include "LmError.h"
#include "LmDebug.h"

#ifdef LANGMAN
#include "LmAssert.h"
#define LMCOMMON_ASSERT LM_ASSERT
#else
#define LMCOMMON_ASSERT assert
#endif

// In the NSK debug lmcomp build we cannot call dlopen
#undef LMCOMMON_CANNOT_CALL_DLOPEN

#ifndef LMCOMMON_CANNOT_CALL_DLOPEN
#include <dlfcn.h>
#endif

// copy_string(): create a copy of a string on the specified heap. A
// pointer to the copy is returned.
char *copy_string(NAMemory *heap, const char *src)
{
  return copy_chars(heap, src, (src ? strlen(src) : 0), TRUE);
}

// copy_chars(): create a copy of given chars on the specified heap.
// The C runtime heap is used if heap is NULL. Target string is null
// terminated if terminate is TRUE. A pointer to the copy is returned.
char *copy_chars(NAMemory *heap, const char *src, ComUInt32 len,
                 NABoolean terminate)
{
  if (!src)
    return NULL;

  if (terminate)
    len++;

  char *tgt;
  if (heap)
    tgt = new (heap) char [len];
  else
    tgt = (char *) malloc(len);
  
  LMCOMMON_ASSERT(tgt);
  
  if (terminate)
  {
    str_cpy_all(tgt, src, (Lng32)len-1);
    tgt[len-1] = '\0';
  }
  else
    str_cpy_all(tgt, src, (Lng32)len);
  
  return tgt;
}

// copy_and_pad(): create a copy of the given buffer on the C runtime
// heap. Optionally add zeroed-out padding at the end.
char *copy_and_pad(const char *src, ComUInt32 len, ComUInt32 pad)
{
  char *tgt = NULL;
  if (src)
  {
    // Make sure the buffer size is a multiple of 8 so that it's
    // always aligned to store binary integers and floating point
    // values if necessary
    ComUInt32 actualLen = ROUND8(len + pad);
    ComUInt32 actualPad = actualLen - len;
    tgt = (char *) malloc(actualLen);
    LMCOMMON_ASSERT(tgt);

    if (len > 0)
      memcpy(tgt, src, len);
    if (actualPad)
      memset(tgt + len, 0, actualPad);
  }
  return tgt;
}

// strip_spaces(): strips leading and trailing spaces in a string. A
// pointer to the first non-space character is returned. If there are
// trailing spaces, the leftmost one is changed to '\0'.
char *strip_spaces(char *str)
{
  // Strip leading spaces
  while (isSpace8859_1(*str))
  {
    str++;
  }

  ComUInt32 len = str_len(str);

  // Strip trailing spaces
  for (ComUInt32 i = len; i > 0; i--)
  {
    if (!isSpace8859_1(str[i - 1]))
    {
      str[i] = '\0';
      break;
    }
  }

  return str;
}

// This function maps INTERVAL FS datatypes to the corresponding
// INTERVAL codes from sqlcli.h
Lng32 getIntervalCode(short fstype)
{
  switch (fstype)
  {
    case REC_INT_YEAR:          return SQLINTCODE_YEAR;
    case REC_INT_MONTH:         return SQLINTCODE_MONTH;
    case REC_INT_YEAR_MONTH:    return SQLINTCODE_YEAR_MONTH;
    case REC_INT_DAY:           return SQLINTCODE_DAY;
    case REC_INT_HOUR:          return SQLINTCODE_HOUR;
    case REC_INT_DAY_HOUR:      return SQLINTCODE_DAY_HOUR;
    case REC_INT_MINUTE:        return SQLINTCODE_MINUTE;
    case REC_INT_HOUR_MINUTE:   return SQLINTCODE_HOUR_MINUTE;
    case REC_INT_DAY_MINUTE:    return SQLINTCODE_DAY_MINUTE;
    case REC_INT_SECOND:        return SQLINTCODE_SECOND;
    case REC_INT_MINUTE_SECOND: return SQLINTCODE_MINUTE_SECOND;
    case REC_INT_HOUR_SECOND:   return SQLINTCODE_HOUR_SECOND;
    case REC_INT_DAY_SECOND:    return SQLINTCODE_DAY_SECOND;
  }

  LMCOMMON_ASSERT(0);
  return -1;
}

// Class LmCBuffer
LmCBuffer::LmCBuffer(ComUInt32 len)
  : buf_(NULL),
    len_(len)
{
  if (len_ > 0)
    init(len_);
}

LmCBuffer::~LmCBuffer()
{
  release();
}

char *LmCBuffer::init(ComUInt32 len)
{
  release();

  // Make sure len_ is a multiple of 8 and then add 8 bytes for safety
  len_ = ROUND8(len);
  len_ += 8;

  // malloc returns a buffer "suitably aligned for storage of any
  // type" according to the man page. We can assume the buffer will be
  // aligned on an 8-byte boundary.
  buf_ = (char *) malloc(len_);
  LMCOMMON_ASSERT(buf_);

  set(0);
  return buf_;
}

void LmCBuffer::set(Int32 c)
{
  if (buf_)
    memset(buf_, c, len_);
}

void LmCBuffer::release()
{
  if (buf_)
    free(buf_);
  buf_ = NULL;
  len_ = 0;
}

void addDllErrors(ComDiagsArea &diags,
                  const char *operation,
                  NABoolean isWarningOnly)
{
  Int32 errorCode = 0;
  Int32 errorDetail = 0;
  char *errorString = (char *)"";


#ifndef LMCOMMON_CANNOT_CALL_DLOPEN
  // dlresultcode() is not applicable to Linux
  errorString = dlerror();
#endif


  // Remove trailing period and linefeed characters from the message
  // string
  ComUInt32 msglen = 0;
  while (errorString && (msglen = strlen(errorString)) > 0)
  {
    ComUInt32 idx = msglen - 1;
    if (errorString[idx] == '\n' || errorString[idx] == '\r' ||
        errorString[idx] == '.')
      errorString[idx] = 0;
    else
      break;
  }

  diags << DgSqlCode((isWarningOnly ? LME_DLFCN_ERROR : -LME_DLFCN_ERROR))
        << DgString0(operation)
        << DgInt0(errorCode)
        << DgInt1(errorDetail)
        << DgString1(errorString);

}

LmHandle loadDll(
  const char   *containerName,
  const char   *externalPath,
  LmHandle     extLoader,
  ComUInt32    *containerSize,
  ComDiagsArea *da,
  NAMemory *heap)
{
#ifdef LMCOMMON_CANNOT_CALL_DLOPEN
  *da << DgSqlCode(-LME_INTERNAL_ERROR)
      << DgString0(": dlopen() is not supported");
  *da << DgSqlCode(-LME_DLL_CONT_NOT_FOUND)
      << DgString0(containerName)
      << DgString1(externalPath);
  return NULL;
#else
  char *libraryName = NULL;
  if (str_len(externalPath) == 0)
    externalPath = ".";
  libraryName = new (heap)
	  char[str_len(externalPath) + str_len(containerName) + 2];
  sprintf(libraryName, "%s/%s", externalPath, containerName);

  // TBD: For now, set container size to 0. Need to see how to get
  // the actual size
  if (containerSize)
    *containerSize = 0;

  // extLoader is an object of LmCLoader class. It's not used to
  // load the library. We can simply load the DLL.
  LmHandle container = NULL;
  const char *operation = "dlopen";
  short trycount = 3;
  while (trycount >0)
    {
      container = (LmHandle) dlopen(libraryName, RTLD_NOW | RTLD_GLOBAL);
      if (container == NULL)
        {
          sleep(30);
          trycount--;
        }
      else
        trycount = 0;
    }
  LM_DEBUG3("%s(%s) returned 0x%08x\n", operation, libraryName, container);
  
  if (container == NULL)
  {
    *da << DgSqlCode(-LME_DLL_CONT_NOT_FOUND)
        << DgString0(containerName)
        << DgString1(externalPath);
    addDllErrors(*da, operation, FALSE);
  }

  NADELETEBASIC(libraryName, heap);
  return container;
#endif // LMCOMMON_CANNOT_CALL_DLOPEN
}

void unloadDll(LmHandle containerHandle, ComDiagsArea *da)
{
  if (containerHandle == NULL)
    return;
  
#ifndef LMCOMMON_CANNOT_CALL_DLOPEN
  Int32 retcode = 0;
  const char *operation = "dlclose";
  retcode = dlclose(containerHandle);
  
  LM_DEBUG3("%s(0x%08x) returned 0x%08x\n",
            operation, containerHandle, retcode);
  
  // Return a warning condition
  if (retcode != 0 && da)
    addDllErrors(*da, operation, TRUE);
#endif // LMCOMMON_CANNOT_CALL_DLOPEN
}

LmHandle getRoutinePtr(
  LmHandle     container,
  const char   *routineName)
{  
#ifdef LMCOMMON_CANNOT_CALL_DLOPEN
  return NULL;
#else
  // Get a handle to the requested routine
  LmHandle routinePtr = NULL;
  LMCOMMON_ASSERT(container);
  const char *operation = "dlsym";
  routinePtr = (LmHandle) dlsym(container, routineName);

  LM_DEBUG4("%s(0x%08x,%s) returned 0x%08x\n",
            operation, container, routineName, routinePtr);
  
  return routinePtr;
#endif // LMCOMMON_CANNOT_CALL_DLOPEN
}

