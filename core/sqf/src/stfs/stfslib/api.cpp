///////////////////////////////////////////////////////////////////////////////
// 
/// \file    api.cpp
/// \brief   The top-level API entry points for STFS Lib
///   
/// This file contains the STFS API interface shell implementation.  It contains
/// the interface only; actual function is in different files.  The functional
/// STFS API is modeled after the POSIX API. It is POSIX-like, not POSIX
/// compliant in its behavior. For detailed information please refer to Scratch
/// & Temporary File System(STFS) External Specification.
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



#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <ctype.h>

#include "stfs/stfslib.h"

#include "stfs_metadata.h"
#include "stfs_defs.h"
#include "stfs_util.h"
#include "stfs_message.h"

#include "stfslibcalls.h"
#include "stfsinfocalls.h"
#include "send.h"

using namespace STFS;

//    -------------------------
//    STFS Utilities
//    -------------------------

///  For now, stubs are used here for the utilities
///  \brief STFS_openers()
int STFS_openers(stfs_nodeid_t     pv_nid,      
                 char             *pp_path,    
                 long             *pp_previndex,   
                 stfs_nodeid_t    &pv_openernid,  
                 pid_t            &pv_openerpid, 
                 char             *pp_openername)
{
  const char     *WHERE = "STFS_openers";
  STFS_ScopeTrace lv_st(WHERE);
  if((!pp_path) || (!pp_previndex) || (!pp_openername)) {
    errno = EINVAL;
    return -1;
  }

  STFS_util::Init(false);

  return STFSINFO_openers(pv_nid, 
			  pp_path, 
			  pp_previndex,
			  pv_openernid, 
			  pv_openerpid, 
			  pp_openername);
}

int STFS_fopeners(stfs_fhndl_t     pv_Fhandle,      
                  long            *pp_PrevIndex,    
                  stfs_nodeid_t   &pv_OpenerNid,  
                  pid_t           &pv_OpenerPid,  
                  char            *pp_OpenerName, 
                  char            *pp_OpenPath)
{
  const char *WHERE = "STFS_fopeners";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if((!pp_PrevIndex) || (!pp_OpenerName) || (!pp_OpenPath)) {
    errno = EINVAL;
    return -1;
  }

  STFS_util::Init(false);

  return STFSINFO_fopeners(pv_Fhandle, 
                           pp_PrevIndex, 
                           pv_OpenerNid,
                           pv_OpenerPid, 
                           pp_OpenerName, 
                           pp_OpenPath);
}

int STFS_stat(stfs_nodeid_t     pv_Nid,
              char             *pp_Path,
              long             *pp_PrevIndex,
              stfs_statmask_t   pv_Mask,
              struct stfs_stat *pp_Buf)
{
  const char *WHERE = "STFS_stat";
  STFS_ScopeTrace   lv_st(WHERE,2);
  
  if((!pp_Path) || (!pp_PrevIndex) || (!pp_Buf)) {
    errno = EINVAL;
    return -1;
  }
 
  STFS_util::Init(false);

  return STFSINFO_stat(pv_Nid, 
                       pp_Path, 
                       pp_PrevIndex, 
                       pv_Mask, 
                       pp_Buf);
}

int STFS_fstat(stfs_fhndl_t      pv_Fhandle,
               stfs_statmask_t   pv_Mask,
               struct stfs_stat *pp_Buf)
{
  const char *WHERE = "STFS_fstat";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if(!pp_Buf) {
    errno = EINVAL;
   return -1;
  }

  STFS_util::Init(false);

  return STFSINFO_fstat(pv_Fhandle, 
                        pv_Mask, 
                        pp_Buf);
}


//    -------------------------
//    STFS_FH_* Set Manipulators
//    -------------------------

/// \brief Removes a file handle from the set
void STFS_FH_CLR( stfs_fhndl_t  pv_fhandle
                , fh_set       *pp_set)
{
   const char *WHERE = "STFS_FH_CLR";
   STFS_ScopeTrace   lv_st(WHERE,2);

   try {
      std::vector<stfs_fhndl_t>::iterator it;
      for (it = pp_set->FhArray.begin();
           it != pp_set->FhArray.end();
           it++) {
        if(pv_fhandle == *it) {
          pp_set->FhArray.erase(it);
          break; 
        }
      }
   }
   catch(...) {
      TRACE_PRINTF1(1,"In the catch-all exception handler\n");
      STFS_util::SoftwareFailureHandler(WHERE);
      errno=EFAULT;
   }
}

/// \brief Tests for existence of file handle in a set
int  STFS_FH_ISSET( stfs_fhndl_t  pv_fhandle
                  , fh_set       *pp_set)
{
   const char *WHERE = "STFS_FH_ISSET";
   STFS_ScopeTrace   lv_st(WHERE,2);

   try {
      int lv_RetVal = 0;
      std::vector<stfs_fhndl_t>::iterator it;

      for (it = pp_set->FhArray.begin();
      it != pp_set->FhArray.end();
      it++) {
        if(pv_fhandle == *it) {
          lv_RetVal = 1;
          break; 
        }
      }
      return lv_RetVal;
   } 
   catch (...) {
      TRACE_PRINTF1(1,"In the catch-all exception handler\n");
      STFS_util::SoftwareFailureHandler(WHERE);
      errno=EFAULT;
      return -1;
   }
}

/// \brief Adds a file handle to the set 
void STFS_FH_SET( stfs_fhndl_t  pv_fhandle
                , fh_set       *pp_set)
{
   const char *WHERE = "STFS_FH_SET";
   STFS_ScopeTrace   lv_st(WHERE,2);

   try {
      pp_set->FhArray.push_back(pv_fhandle);
   }
   catch (...) {
      TRACE_PRINTF1(1,"In the catch-all exception handler\n");
      STFS_util::SoftwareFailureHandler(WHERE);
      errno=EFAULT;
   }
}

/// \brief Clears all file handles from the set
void STFS_FH_ZERO(fh_set *pp_set)
{
   const char *WHERE = "STFS_FH_ZERO";
   
   try {
      STFS_ScopeTrace   lv_st(WHERE,2);
      pp_set->FhArray.clear();
   }
   catch (...) {
      TRACE_PRINTF1(1,"In the catch-all exception handler\n");
      STFS_util::SoftwareFailureHandler(WHERE);
      errno=EFAULT;
   }
}

//    ----------------    
//    Functions 
//    ----------------

/// \brief Close a file handle
int STFS_close(stfs_fhndl_t fhandle)
{
   const char *WHERE = "STFS_close";
   STFS_ScopeTrace   lv_st(WHERE,2);
   
   try {
      return STFSLIB_close(fhandle);
   }
   catch(...) {
     TRACE_PRINTF1(1,"In the catch-all exception handler\n");
     STFS_util::SoftwareFailureHandler(WHERE);
     errno=EFAULT;
     return -1;
   }
}

/// \brief Get error information from a file handle
int STFS_error ( stfs_fhndl_t  pv_fhandle,
		 int          *pp_error,
		 int          *pp_addlError,
		 char         *pp_context,
		 size_t        pv_contextMaxLen,
		 size_t       *pp_contextLen ) {

  const char *WHERE = "STFS_error";
  STFS_ScopeTrace   lv_st (WHERE,2);

  try {
    return STFSLIB_error (pv_fhandle, pp_error, pp_addlError, 
			  pp_context, pv_contextMaxLen, pp_contextLen );
  }
  catch (...) {
    TRACE_PRINTF1 (1, "In the catch-all exception handler \n");
    STFS_util::SoftwareFailureHandler (WHERE);
    errno=EFAULT;
    return -1;
  }
}

///  \brief Manipulate a file handle
int STFS_fcntl( stfs_fhndl_t fhandle
              , int          cmd
              , ...)
{
   const char *WHERE = "STFS_fcntl";
   STFS_ScopeTrace   lv_st(WHERE,2);
   
   if(cmd==F_SETFL)
   { 
      //if F_SETFL is used for second argument, 
      //   then third argument must also be set
      long arglong, 
           bitmask;
      va_list l_arg;
      va_start(l_arg, cmd);
      arglong = va_arg(l_arg, long);

      //Checking to ensure only allowed bits are set
      bitmask=(O_RDONLY|O_WRONLY|O_RDWR|O_APPEND|O_EXCL|O_NONBLOCK|O_TRUNC);
      ASSERT((arglong&(~bitmask))==0);
      arglong &= bitmask;       

      try {
         return STFSLIB_fcntl(fhandle, cmd, arglong);
      }
      catch(...) {
         TRACE_PRINTF1(1,"In the catch-all exception handler\n");
         STFS_util::SoftwareFailureHandler(WHERE);
	 errno=EFAULT;
         return -1;
      }
   }
   else
   {
      try {
         return STFSLIB_fcntl(fhandle, cmd);
      }
      catch(...) {
         TRACE_PRINTF1(1,"In the catch-all exception handler\n");
         STFS_util::SoftwareFailureHandler(WHERE);
	 errno=EFAULT;
         return -1;
      }
   }
}

/// \brief Reposition read/write file offset
off_t STFS_lseek( stfs_fhndl_t   fhandle
                , off_t          offset
                , int            whence)
{
   const char *WHERE = "STFS_lseek";
   STFS_ScopeTrace   lv_st(WHERE,2);

   try {
      return STFSLIB_lseek(fhandle, offset, whence);
   }
   catch(...) {
      TRACE_PRINTF1(1,"In the catch-all exception handler\n");
      STFS_util::SoftwareFailureHandler(WHERE);
      errno=EFAULT;
      return -1;
   }
}

/// \brief Create and open a unique file name
stfs_fhndl_t STFS_mkstemp (char *ctemplate)
{
   const char *WHERE = "STFS_mkstemp";
   STFS_ScopeTrace   lv_st(WHERE,2);

   if(ctemplate == NULL)
   {
      errno=EFAULT;
      return -1;
   }

   for (unsigned int i = 0; i < strlen(ctemplate); i++)
   {
     if(ctemplate[i] == '/') 
     {
        errno = EINVAL;
        return -1;
     }
   }

   if(strlen(ctemplate) > (STFS_PATH_MAX-STFS_NAME_SUFFIX_MAX))
   { 
      errno=ENAMETOOLONG;
      return -1;
   }

   STFS_util::Init(false);

   try {
     // Rev: Check the STFS_Session to set the errno if the return value (!= 0)
      return STFSLIB_mkstemp(ctemplate);
   }
   catch(...) {
      TRACE_PRINTF1(1,"In the catch-all exception handler\n");
      STFS_util::SoftwareFailureHandler(WHERE);
      // Rev: Check for any standard errno for an unhandled exception
      errno=EFAULT;
      return -1;
   }
}

/// \brief Open a temporary file
stfs_fhndl_t STFS_open( const char *path   
                      , int         oflag
                      , ...)
{  
   const char *WHERE = "STFS_open";
   STFS_ScopeTrace   lv_st(WHERE,2);

   //Check that the pathname passed is not longer than our MAX
   ASSERT(strlen(path) <= STFS_PATH_MAX);

   //Checking to ensure only allowed bits are set
   int bitmask;
   bitmask=(O_RDONLY|O_WRONLY|O_RDWR|O_APPEND|O_EXCL|O_NONBLOCK|O_TRUNC);
   ASSERT((oflag&(~bitmask))==0);
   
   //O_CREAT is not supported in initial version of STFS
   if((oflag&O_CREAT)==O_CREAT)
   {
      errno=EINVAL;
      return -1;
   }
   oflag &= bitmask;
   
   STFS_util::Init(false);
   try {
      return STFSLIB_open(path, oflag);
   }
   catch(...) {
      TRACE_PRINTF1(1,"In the catch-all exception handler\n");
      STFS_util::SoftwareFailureHandler(WHERE);
      errno=EFAULT;
      return -1;
   }
}

/// \brief Read from a file handle
long STFS_read( stfs_fhndl_t     fhandle
             , void            *buf
             , size_t           count)
{
   const char *WHERE = "STFS_read";
   STFS_ScopeTrace   lv_st(WHERE,2);
   
   try {
      return STFSLIB_read(fhandle, buf, count);
   }
   catch(...) {
      TRACE_PRINTF1(1,"In the catch-all exception handler\n");
      STFS_util::SoftwareFailureHandler(WHERE);
      errno=EFAULT;
      return -1;
   }
}

/// \brief Synchronous I/O multiplexing
int STFS_select( stfs_fhndl_t    nfhs       
               , fh_set         *readfhs
               , fh_set         *writefhs
               , fh_set         *exceptfhs
               , struct timeval *timeout)
{
   const char *WHERE = "STFS_select";
   STFS_ScopeTrace   lv_st(WHERE,2);

   try {
      return STFSLIB_select(nfhs, readfhs, writefhs, exceptfhs, timeout);
   }
   catch(...) {
      TRACE_PRINTF1(1,"In the catch-all exception handler\n");
      STFS_util::SoftwareFailureHandler(WHERE);
      errno=EFAULT;
      return -1;
   }
}

/// \brief Delete a name and possibly the file that it refers to
int STFS_unlink (const char *path)
{
   const char *WHERE = "STFS_unlink";
   STFS_ScopeTrace   lv_st(WHERE,2);

   //Check that the pathname passed is not longer than our MAX
   ASSERT(strlen(path) <= STFS_PATH_MAX);

   STFS_util::Init(false);
   try {
      return STFSLIB_unlink(path);
   } 
   catch(...) {
      TRACE_PRINTF1(1,"In the catch-all exception handler\n");
      STFS_util::SoftwareFailureHandler(WHERE);
      errno=EFAULT;
      return -1;
   }
}

/// \brief Write to a file handle
ssize_t STFS_write( stfs_fhndl_t  fhandle
                  , const void   *buf
                  , size_t        count)
{
   const char *WHERE = "STFS_write";
   STFS_ScopeTrace   lv_st(WHERE,2);
   
   try {
      return STFSLIB_write(fhandle, buf, count);
   }
   catch(...) {
      TRACE_PRINTF1(1,"In the catch-all exception handler\n");
      STFS_util::SoftwareFailureHandler(WHERE);
      errno=EFAULT;
      return -1;
   }
}
