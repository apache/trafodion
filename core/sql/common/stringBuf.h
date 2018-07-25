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
 *****************************************************************************
 *
 * File:         stringBuf.h
 * RCS:          $Id: 
 * Description:  A simple buffer class for conversion routines
 *               
 *               
 * Created:      7/8/98
 * Modified:     $ $Date: 2006/11/01 01:38:09 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */

#ifndef _STRING_BUF_H
#define _STRING_BUF_H

#if !defined(MODULE_DEBUG) 
#include "Platform.h"
#include "BaseTypes.h"
#include "NAWinNT.h"
#endif

#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG 

template <class T> class stringBuf
{

public:

stringBuf(T* buf, Int32 len, CollHeap* heap = 0) : 
      buf_(buf), bufSize_(len), count_(len), alloc_(FALSE), heap_(heap) {};

stringBuf(T* buf, Int32 iBufSize, Int32 iStrLen, CollHeap* heap = 0) : 
      buf_(buf), bufSize_(iBufSize),
      count_(iStrLen <= iBufSize? iStrLen : iBufSize),
      alloc_(FALSE), heap_(heap) {};

 stringBuf(Int32 len, CollHeap* heap = 0) : 
      bufSize_(len),
      count_(len), // should be set to 0, but keep the existing behavior for now
      alloc_(FALSE), heap_(heap)
  {
    if ( len == 0 ) 
        buf_ = 0;
    else {
        alloc_ = TRUE;
        buf_ = (heap) ? (new (heap) T[len]) : 
                      new T[len];
    }
    if (buf_ != NULL)
    {
      if (bufSize_ > 0)
        buf_[0] = 0;
    }
  };

  ~stringBuf() 
  {
     if ( alloc_ ) {
       if ( heap_ ) 
            NADELETEBASIC(buf_, heap_);
       else
            delete [] buf_;
     }
  };

  inline Int32 getBufSize() const { return bufSize_; }

  inline Int32 getStrLen() const { return count_; } // same as length()

  inline Int32 length() const { return count_; }

  inline void setStrLen(Int32 x) { count_ = (x <= bufSize_) ? x : bufSize_; } // avoid buffer overrun

  inline void setLength(Int32 x) { count_ = x; }

  inline T* data() const { return buf_; }

  T operator() (Int32 i) const { return buf_[i]; };

  T last() const { return buf_[getStrLen()-1]; };

  void decrementAndNullTerminate() { /* if (count > 0) */ buf_[--count_]=0; }

  void zeroOutBuf(Int32 startPos = 0)
  {
    if (startPos >= 0 && buf_ && bufSize_ > 0 && bufSize_ - startPos > 0)
    {
      memset((void*)&buf_[startPos], 0, (size_t)((bufSize_-startPos)*sizeof(T)));
      count_ = startPos;
    }
  };

#ifdef _DEBUG
  ostream& print(ostream& out) { 

      Int32 i=0;
      for (; i<count_; i++ ) 
         out << Int32(buf_[i]) << " ";

      out << endl;
      for ( i=0; i<count_; i++ ) 
         out << hex << Int32(buf_[i]) << " ";

      out << endl;
      return out;
  };
#endif // _DEBUG

private:
  T* buf_;
  Int32 bufSize_;
  Int32 count_;
  NABoolean alloc_;
  CollHeap* heap_;
};

// define the type of char and wchar buffer used in these
// conversion routines.
typedef stringBuf<NAWchar> NAWcharBuf;
typedef stringBuf<unsigned char> charBuf;

//
// Check and optionally allocate space for char or NAWchar typed buffer. The 
// following comments apply to both the NAWchar and char version of 
// checkSpace().
//
//  case 1: target buffer pointer 'target' is not NULL 
//        if has enough space to hold SIZE chars, then 'target' 
//        is returned; otherwise, NULL is returned. 
//  case 2: target buffer pointer 'target' is NULL 
//        a buffer of SIZE is allocated from the heap (if the heap argument 
//        is not NULL), or from the C runtime system heap. 
//  For either case, SIZE is defined as olen + (addNullAtEnd) ? 1 : 0.
//
//  If addNullAtEnd is TRUE, a NULL will be inserted at the beginning of 'target' 
//  if it is not NULL. 
//
NAWcharBuf* checkSpace(CollHeap* heap, Int32 olen, NAWcharBuf*& target, NABoolean addNullAtEnd);

charBuf* checkSpace(CollHeap* heap, Int32 olen, charBuf*& target, NABoolean addNullAtEnd);

#endif
