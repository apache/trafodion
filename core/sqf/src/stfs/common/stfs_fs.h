#ifndef STFS_FS_H
#define STFS_FS_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_fs.h
///  \brief   File System encapsulation abstract class
///    
///  This file contains the abstract class, STFS_fs, which 
///  will be used for file system encapsulation.
/// 
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
///////////////////////////////////////////////////////////////////////////////

// Get O_NONBLOCK and other definitions 
#include <fcntl.h>    
// Get fd_set 
#include <sys/select.h> 
// Get timeval 
#include <bits/time.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>

#include "stfs/stfslib.h"

namespace STFS {

///////////////////////////////////////////////////////////////////////////////
///
/// \class  STFS_fs
///
/// \brief  File System encapsulation class
///
/// This abstraction class will be used as a file system encapsulation and will
/// allow different file systems to be used.
///
///////////////////////////////////////////////////////////////////////////////
class STFS_fs {
public:
   STFS_fs();
   STFS_fs(char *inFilename);
   virtual ~STFS_fs();
 
   void fd_CLR(fd_set   *set);


   int  fd_ISSET(fd_set *set);


   void fd_SET(fd_set  *set);


   void fd_ZERO(fd_set   *set);

   virtual int close()=0; 

   virtual int fcntl (int cmd) = 0;    

   virtual int fcntl (int cmd,
                      long arglong) = 0; 

   virtual off_t lseek(off_t          offset
                     , int            whence) = 0; 

   virtual long mkstemp(char *ctemplate)=0; 

   virtual long open(int   oflag) = 0;
   
   virtual long open(int     oflag
                   , mode_t  mode) = 0;

   virtual int read(void   *buf
                  , size_t  count) = 0; 

   virtual int select( long    nfhs
                     , fd_set         *readfhs
                     , fd_set         *writefhs
                     , fd_set         *exceptfhs
                     , struct timeval *timeout) = 0; 

   virtual int stat (struct stat *buf) = 0; 

   virtual int unlink()=0; 

   virtual ssize_t write(const void  *buf
                       , size_t       count) = 0; 

   char * FilenameGet() const {
      return (char *)filename_;
   }
   
   long FdGet() const {
      return fd_;
   }

   bool IsOpen() const {
     return isOpen_;
   }

protected:
   long fd_;
   char *filename_;
   bool isOpen_;
};
} //end of namespace STFS
#endif // STFS_FS_H

