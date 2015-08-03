/* -*-C++-*-
******************************************************************************
*
* File:         termpfile.C
* Description:  Methods temporary overflow files
*
*               
*               
* Created:      08/14/96
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
******************************************************************************
*/

// begining of regular compilation
#include "Platform.h"
#include "tempfile.h"
#include <fcntl.h>

#include <stdio.h>


TempFile::TempFile()
  : fd_(-1),
    offset_(-1) {
};

TempFile::~TempFile() {
  TFclose();
};

void TempFile::TFread(FilePos * position,
			 char * buffer,
			 Int32 size) {
  if ((position->fileId_ != fd_) ||
      (!position->valid() ||
      ((position->offset_ + size) > offset_)))
    ex_assert(0, "tried to read from non existing temporary file");
  lseek(fd_, offset_, SEEK_SET);
  read(fd_, buffer, size);
};

void TempFile::TFwrite(FilePos * position,
		       char * buffer,
		       Int32 size) {
  if (fd_ == -1) {
    fd_ = open(name_, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if (fd_ == -1)
    {
      // we couldn't create the tmpfile. Too bad!!
      printf ("temporary file name: %s\n", name_);
      ex_assert(0, "unable to open temporary file");
    };
    offset_ = 0;
  };

  lseek(fd_, offset_, SEEK_SET);
  write(fd_, buffer, size);
  // return the file position of the written buffer
  position->fileId_ = fd_;
  position->offset_ = offset_;

  // the offset has changed
  offset_ += size;
};

void TempFile::TFclose() {
  if (fd_ > -1) {
    ex_assert((close(fd_) == 0), "unable to close temporary file");
    ex_assert((unlink(name_) == 0), "unable to purge temporary file");
    fd_ = -1;
  };
  offset_ = -1;
};

NABoolean TempFile::ioComplete() {
  return FALSE; // for NSK, no overlapped io
};

NABoolean TempFile::ioSucessfull() {
  return TRUE;
};
