///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfs_ext2.cpp
/// \brief   Implementation of the STFS_ext2 class methods
//                                                                                      
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

/* for close, lseek, read, unlink, write */
#include <unistd.h>
/* for mkstemp */
#include <stdlib.h>
/* for va_list and va_start */
#include <stdarg.h>
/* for assert */
#include <assert.h>
/* for strlen */
#include <string.h>
/* for errno defines */
#include <errno.h>
/* for printf */
#include <stdio.h>
#include <string.h>

#include "stfs_ext2.h"
#include "stfs_util.h"
#include "stfs_defs.h"

using namespace STFS;

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::STFS_ext2()
///
///  \brief Default constructor for STFS_fs
///
///////////////////////////////////////////////////////////////////////////////
STFS_ext2::STFS_ext2() : STFS_fs()
{
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::STFS_ext2()
///
///  \brief Constructor for STFS_fs takes in filename
///
///  Sets the filename to be used. 
///
///  \param[in]   infilename  Input filename
///
///////////////////////////////////////////////////////////////////////////////
STFS_ext2::STFS_ext2(char *infilename) : STFS_fs(infilename)
{
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::STFS_ext2()
///
///  \brief STFS_ext2() copy constructor
///
///  \param[in]  STFS_ext2 
///
///////////////////////////////////////////////////////////////////////////////
STFS_ext2::STFS_ext2(STFS_ext2 &s):STFS_fs()
{
  if(s.filename_)
  {
    delete [] filename_;
    filename_ = new char[strlen(s.filename_) + 1];
    strcpy(filename_,s.filename_);  
  } 
  
  if (fd_>0) {
    ::close(fd_);
  } 

  fd_=s.fd_;
}

STFS_ext2::~STFS_ext2()
{
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::mkstemp()
///
///  \brief Create and open a unique file name
///
///  The STFS_mkstemp function appends the string pointed to by template with 
///  a unique suffix. Creates a new empty file and opens the file for reading 
///  and writing. The file is created with mode read/write and permissions 0600. 
///
///  \param[in]   ctemplate   must not be a string constant, it should be 
///                           declared as a character array of at least  
///                           STFS_PATHMAX bytes to include the null terminator. 
///                           A null string is valid on input.
///
///  \retval     stfs_fhndl_t SUCCESS: returns an open file handle.\n\n
///                           ERROR:   -1 is returned and errno is set.
///////////////////////////////////////////////////////////////////////////////
long
STFS_ext2::mkstemp(char *pp_Ctemplate)
{
  const char       *WHERE = "STFS_ext2::mkstemp";
  STFS_ScopeTrace   lv_st(WHERE,2);


  if(pp_Ctemplate == NULL)
    {
      errno=EFAULT;
      return -1;
    }

  strcat(pp_Ctemplate, "XXXXXX");

  fd_=::mkstemp(pp_Ctemplate);
  if(fd_<0) {
    return -1;
  }

  isOpen_ = true;

  if(filename_) {
    delete [] filename_;
  }

  filename_ = new char[strlen(pp_Ctemplate) + 1];
  strcpy(filename_,pp_Ctemplate);
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::close()
///
///  \brief Close a file handle
///
///  The STFS_close function closes the given fhandle. When all open file 
///  handles have been closed, the temporary file is unlinked, i.e., deleted.
///
///  \param[in]   fhandle   file handle returned from STFS_open or STFS_mkstemp
///
///  \retval      int       Success: 0      \n\n
///                         Error:   1
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_ext2::close()
{
  const char       *WHERE = "STFS_ext2::close";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (fd_<0) {
    return -1;
  }
  int ret=::close(fd_);
  if(ret < 0) {
    return -1;
  }
  fd_=-1;
  isOpen_ = false;
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::fcntl()
///
///  \brief Manipulate a file handle
/// 
///  The STFS_fcntl function controls or returns open file attributes depending 
///  on the value of cmd.
///
///  \param[in]   cmd       values are defined in <fcntl.h> and are as follows: \n
///                            F_GETFL - get file status flags \n
///                            F_SETFL - set file status flags
///
///  \retval       int      SUCCESS: Value returned shall depend on cmd as follows:\n
///                            F_GETFL -  Value of file status flags and access
///                                       modes.  The return value is not negative
///                                       on success.\n
///                            F_SETFL -  Zero on sucess.\n\n
///                         ERROR:   -1 is returned and errno is set. 
///////////////////////////////////////////////////////////////////////////////
int
STFS_ext2::fcntl( int   cmd )
{
  const char       *WHERE = "STFS_ext2::fcntl";
  STFS_ScopeTrace   lv_st(WHERE,2);

  return ::fcntl(fd_, cmd );
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::fcntl()
///
///  \brief Manipulate a file handle
/// 
///  The STFS_fcntl function controls or returns open file attributes depending 
///  on the value of cmd.
///
///  \param[in]   cmd       values are defined in <fcntl.h> and are as follows: \n
///                            F_GETFL - get file status flags \n
///                            F_SETFL - set file status flags
///  \param[in]   (opt)arg  values are defined in <fcntl.h> and are as follows: \n
///                            O_NONBLOCK - Non-blocking mode 
///
///  \retval       int      SUCCESS: Value returned shall depend on cmd as follows:\n
///                            F_GETFL -  Value of file status flags and access
///                                       modes.  The return value is not negative
///                                       on success.\n
///                            F_SETFL -  Zero on sucess.\n\n
///                         ERROR:   -1 is returned and errno is set. 
///////////////////////////////////////////////////////////////////////////////
int
STFS_ext2::fcntl(int   cmd 
                 , long  arglong)
{
  const char       *WHERE = "STFS_ext2::fcntl";
  STFS_ScopeTrace   lv_st(WHERE,2);

  return ::fcntl(fd_, cmd, arglong);
}

///////////////////////////////////////////////////////////////////////////////
///
//      STFS_ext2::lseek()
/// \brief Reposition read/write file offset
///
/// 
///     Repositions the offset of the file handle fhandle to the argument offset 
///     according to the argument whence.
///
///  \param[in]   offset    byte offset count based on whence
///  \param[in]   whence    values are as follow: \n
///                         SEEK_SET - The offset is set to offset bytes. \n
///                         SEEK_CUR - The offset is set to its current
///                                    location plus offset bytes. \n
///                         SEEK_END - The offset is set to the size of the 
///                                    file plus offset bytes. 
///  \retval      off_t     SUCCESS: The resulting offset location from the 
///                                  beginning of the file.\n\n
///                         Error:   -1 is returned and errno is set.
///////////////////////////////////////////////////////////////////////////////
off_t
STFS_ext2::lseek(off_t   offset
                 , int     whence)
{
  const char       *WHERE = "STFS_ext2::lseek";
  STFS_ScopeTrace   lv_st(WHERE,2);

  return ::lseek(fd_, offset, whence);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::open()
///
///  \brief Open a temporary file  
///
///  The STFS_open function opens the filename specified in the base class. 
///
///  \param[in]   oflag   values are constructed by a bitwise-inclusive OR of 
///                       flags defined in <fcntl.h>. 
///
///  \retval              SUCCESS: Returns 0 \n\n
///                       ERROR:   -1 is returned and errno is set.
///
///////////////////////////////////////////////////////////////////////////////
long
STFS_ext2::open(int  oflag)
{  
  const char       *WHERE = "STFS_ext2::open";
  STFS_ScopeTrace   lv_st(WHERE,2);

  // Rev: Is it ok if there already is an fd_
  //      Could return back an existing fd_ (if not -1)
  //         & do an lseek(0)
  //      Should also check the flags 
  //      Could also return back something like E_ALREADY_OPEN
  // 
  // Maybe have an ASSERT() (in debug) and in the release path just return 0
  //
  fd_ = ::open(filename_, oflag);
  if(fd_) {
    isOpen_ = true;
    return 0;
  }
  //errno = 
  return -1;
}

///////////////////////////////////////////////////////////////////////////////
///
//         STFS_ext2::open()
///
///  \brief Open a temporary file  
/// 
///  The STFS_open function opens the filename specified in the base class 
///
///  \param[in]   oflag   values are constructed by a bitwise-inclusive OR of 
///                       flags defined in <fcntl.h>. 
///  \param[in]   mode    valid values are: 
///                       S_IRUSR   00400 user has read permission
///                       S_IWUSR   00200 user has write permission
///
///  \retval stfs_fhndl_t SUCCESS: Returns an open file handle.\n\n
///                       ERROR:   -1 is returned and errno is set.
///
///////////////////////////////////////////////////////////////////////////////
long STFS_ext2::open(int     oflag
                     , mode_t  mode)
                     
{  
  const char       *WHERE = "STFS_ext2::open";
  STFS_ScopeTrace   lv_st(WHERE,2);

  // Rev: Is it ok if there already is an fd_
  //      Could return back an existing fd_ (if not -1)
  //         & do an lseek(0)
  //      Should also check the flags 
  //      Could also return back something like E_ALREADY_OPEN
  // 
  // Maybe have an ASSERT() (in debug) and in the release path just return 0
  //
  fd_ = ::open(filename_, oflag, mode);
  if(fd_) {
    isOpen_ = true;
    return 0;
  }
  //errno = 
  return -1;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::read()
///
///  \brief Read from a file handle
///
///  STFS_read() attempts to read up to count bytes from file handle fhandle into 
///  the buffer starting at buf. 
///
///  \param[out]  buf       buffer where the read contents are returned.
///  \param[in]   count     value between zero and SSIZE_MAX.
///
///  \retval      int       SUCCESS: The number of bytes read is returned, the 
///                                  file position is advanced by this number.\n
///                                  Zero indicates end of file\n\n
///                         ERROR:   -1 is returned and errno is set. 
///////////////////////////////////////////////////////////////////////////////
int 
STFS_ext2::read(void   *buf
                , size_t   count)
{
  const char       *WHERE = "STFS_ext2::read";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if(!buf) {
    errno = EFAULT;
    return -1;
  }

  return ::read(fd_, buf, count);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::select()
///
///  \brief Synchronous I/O multiplexing
/// 
///  Allows a program to monitor multiple file handles, waiting until one or more 
///  STFS file handles has a completion to an I/O operation.
///
///  \param[in]       nfhs       specifies the number of handles to be tested in 
///                              each set.
///  \param[in,out]   readfhs    if not a null pointer, points to an object type 
///                              fh_set that on input specifies the file handles to
///                              be watched to see if a read will not block (the file
///                              handles that are ready to read), on output indicates
///                              which file handles are ready to read.
///  \param[in,out]   writefhs   if not a null pointer, points to an object type fh_set
///                              that on input specifies the file handles to be watched
///                              to see if a write will not block (the file handles that 
///                              are ready to write), on output indicates which file 
///                              handles are ready to write.
///  \param[in,out]   exceptfhs  if not a null pointer, points to an object type fh_set
///                              that on input specifies the file handles to be watched 
///                              to see if there is a pending error condition, on output 
///                              indicates which file handles have error conditions pending.
///  \param[in]       timeout    the length of time the handles will be watched. The timeout 
///                              period is given in seconds and microseconds. If the timeout 
///                              period is zero, it will return immediately if no file handles 
///                              are ready to read, write, or have pending error conditions. 
///  \retval          int        SUCCESS: The number of file handles contained in the three 
///                                       returned file handle sets. The number may be zero if 
///                                       the timeout expires and no file handles are ready to 
///                                       read, ready to write, or have a pending error
///                                       condition.\n\n 
///                              ERROR:   -1 is returned and errno is set.
///////////////////////////////////////////////////////////////////////////////
int
STFS_ext2::select( long          nfhs       
                   , fd_set         *readfhs
                   , fd_set         *writefhs
                   , fd_set         *exceptfhs
                   , struct timeval *timeout)
{
  const char       *WHERE = "STFS_ext2::select";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if(!timeout) {
    errno = EINVAL;
    return -1;
  }

  return ::select(nfhs, readfhs, writefhs, exceptfhs, timeout);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::stat()
///
///  \brief Obtain statistics for file associated with base class
/// 
///  The STFS_stat function passes a stat struct with file statistics.
///
///  \param[in]   struct stat*  holds file statistics  
///
///  \retval       int      SUCCESS: 0
///                         ERROR:   -1 is returned and errno is set. 
///////////////////////////////////////////////////////////////////////////////
int
STFS_ext2::stat( struct stat *buf)
{
  const char       *WHERE = "STFS_ext2::stat";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if(!buf) {
    errno = EFAULT;
    return -1;
  }
  
  if(fd_ < 0) {
    errno = EBADF;
    return -1;
  }

  return ::stat(filename_, buf);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::unlink()
///
///  The STFS_unlink function deletes a name from the file system. If no processes 
///  have the file open the file is deleted. Otherwise, the file remains in 
///  existence until the last file handle referring to the file is closed.
///
///  \param[in]   path   must be value of the STFS file returned from STFS_openers, 
///                      STFS_fopeners, or STFS_mkstemp. 
///
///  \retval      int    SUCCESS: 0      \n\n
///                      ERROR:   -1 is returned and errno is set
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_ext2::unlink ()
{ 
  const char       *WHERE = "STFS_ext2::unlink ";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if(fd_>0) {
    this->close();
  }
  if((!filename_) || 
     (filename_[0]==0)) {
    return -1; 
  }
  int ret=::unlink(filename_);
  if(ret < 0) {
    return -1;
  }
  delete [] filename_;
  filename_ = 0;
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ext2::write()
///
///  \brief Write to a file handle
///
///  The STFS_write function writes up to the count bytes to the file referenced
///  by fhandle from the buffer starting at buf.
///
///  \param[in]   buf       buffer with the write contents
///  \param[in]   count     value between zero and SSIZE_MAX
///
///  \retval      ssize_t   SUCCESS: Returns the number of bytes written\n
///                                  A Zero indicates nothing was written\n\n
///                         ERROR:   -1 is returned and errno is set
///////////////////////////////////////////////////////////////////////////////

// Rev: add an extra parameter (error number), so that any error detected
// could be passed back (even with a positive return value).
ssize_t
STFS_ext2::write(const void   *buf
                 , size_t        count)
{
  const char       *WHERE = "STFS_ext2::write";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int flag;
  ssize_t writeReturn=0;
  ssize_t bytesWritten=0;
  const void *lBuf = buf;
  off_t lOffset = 0;

  if(fd_ < 0) {
    errno = EBADF;
    return -1;
  }

  flag = fcntl(fd_, F_GETFL);
  if((flag&O_APPEND)==O_APPEND)
  {
    lOffset = lseek(0, SEEK_END);
  } 
  else
  {  
    lOffset = lseek(0, SEEK_CUR);
  }

  while((unsigned long) bytesWritten < count)
  {   
    writeReturn = ::write(fd_, lBuf, count - bytesWritten);

    if(writeReturn < 0) {
      if (bytesWritten <= 0) {
        return -1; 
      }
      else {
        // set the pp_ErrorNumber here
        break;
      }
    }   

    bytesWritten += writeReturn;
    lOffset += writeReturn;
    if ((unsigned long)lOffset >= STFS_SSIZE_MAX - 1) {
      break;
    }   
    lBuf = ((char *) lBuf) +  bytesWritten;
  }   
  return bytesWritten;
}

