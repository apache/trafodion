#ifndef TEMPFILE_H
#define TEMPFILE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      8/14/96
 * Language:     C++
 *
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
 *
 *
 *****************************************************************************
 */

#include "BaseTypes.h"
#include "NABasicObject.h"
#include "ex_stdh.h"

// for now we support only UNIX and OSS

// The class FilePos is NOT derived from ExGod! It is never allocated
// with new. Instead, it is just an "overlay" on an allocated buffer.
class FilePos {
  friend class TempFile;
public:
  inline FilePos();
  inline ~FilePos() {};
  inline NABoolean valid();
private:
  unsigned short fileId_;
  Int32 offset_;
};

inline FilePos::FilePos() {
  fileId_ = 0;
  offset_ = -1;
};

inline NABoolean FilePos::valid() {
  return (offset_ != -1);
};

class TempFile : public NABasicObject {
public:
  TempFile();
  ~TempFile();
  void TFread(FilePos * position,
	      char * buffer,
	      Int32 size);
  void TFwrite(FilePos * position,
	       char * buffer,
	       Int32 size);
  void TFclose();

  NABoolean ioComplete();

  NABoolean ioSucessfull();

inline char * getName() {
  return (name_);
};
private:
  Int32 fd_;
  char  name_[L_tmpnam];
  Int32 offset_;
};

#endif












