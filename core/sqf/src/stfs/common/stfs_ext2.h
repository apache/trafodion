#ifndef STFS_EXT2_H
#define STFS_EXT2_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_ext2.h
///  \brief   Derived class for STFS_fs
///    
///  This file contains the derived class, STFS_ext2, which 
///  will be used for file system encapsulation with use of 
///  an ext2 file system.
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

// for close, lseek, read, unlik, write 
#include <unistd.h>
// for mkstemp 
#include <stdlib.h>
// for assert 
#include <assert.h>
// for strlen,strcpy 
#include <string.h>
// for errno defines
#include <errno.h>

#include "stfs_fs.h"

namespace STFS {
///////////////////////////////////////////////////////////////////////////////
///
/// \class  STFS_ext2
///
/// \brief  Derived File System encapsulation class
///
/// This derived class will be used as a file system encapsulation and will
/// allow the ext2 file system to be used
/// \n
///
///////////////////////////////////////////////////////////////////////////////
class STFS_ext2 : public STFS_fs
{
public:
   STFS_ext2();
   STFS_ext2(char *infilename);
   STFS_ext2(STFS_ext2 &s);  
   ~STFS_ext2();

   int close();

   int fcntl (int   cmd);

   int fcntl (int   cmd
            , long  arglong);

   off_t lseek(off_t   offset
             , int     whence);

   long mkstemp(char   *ctemplate);

   long open(int   oflag);
                    
   long open(int     oflag
           , mode_t  mode);

   int read(void   *buf
          , size_t  count);

   int select( long    nfhs
             , fd_set *readfhs
             , fd_set *writefhs
             , fd_set *exceptfhs
             , struct  timeval   *timeout);

   int stat (struct stat *buf);

   int unlink();

   ssize_t write(const void    *buf
               , size_t         count);
};
}

#endif // STFS_EXT2_H 
