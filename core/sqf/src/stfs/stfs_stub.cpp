/*! \file    stfs.cpp
    \brief   STFS API stub implementation

    Contains the stub implementation for the STFS APIs,  This is not the real 
    implementation but simply calls to the corresponding Linux function call.
    These stubs will work with a single, local underlying file.

    The functional STFS API is modeled after the POSIX API. It is POSIX-like,
    not POSIX compliant in its behavior. For detailed information please refer to
    Scratch & Temporary File System(STFS) External Specification.

*/
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

#include "stfs/stfslib.h"
#include "seabed/trace.h"
#include "seabed/ms.h"
#include <stdio.h>
#include <ctype.h>
#include <libgen.h>

#ifdef DEBUG
   #define ASSERT(a)  assert((a))
#else 
   #define ASSERT(a) 
#endif

#define STFS_SUFFIX           40    

/*********   Trace Setup - Begin *****************/

#define TRACE_PRINTF_BASIC(pv_TraceLevel)	\
  if (!STFS_stub::traceEnvChecked_)             \
    STFS_checkTrace();                          \
  if (STFS_stub::traceLevel_ >= pv_TraceLevel)  \
    trace_printf( 

#define TRACE_PRINTF1(pv_TraceLevel,a1)		\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1);	

#define TRACE_PRINTF2(pv_TraceLevel,a1,a2)	\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1,a2);	

#define TRACE_PRINTF3(pv_TraceLevel,a1,a2,a3)	\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1,a2,a3);	

#define TRACE_PRINTF4(pv_TraceLevel,a1,a2,a3,a4)	\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1,a2,a3,a4);	

#define TRACE_PRINTF5(pv_TraceLevel,a1,a2,a3,a4,a5)	\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1,a2,a3,a4,a5);	

#define TRACE_PRINTF6(pv_TraceLevel,a1,a2,a3,a4,a5,a6)	\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1,a2,a3,a4,a5,a6);	
    
#define TRACE_PRINTF7(pv_TraceLevel,a1,a2,a3,a4,a5,a6,a7)	\
  TRACE_PRINTF_BASIC(pv_TraceLevel) a1,a2,a3,a4,a5,a6,a7);	

namespace STFS_stub {
  
  short traceLevel_                = 0;
  bool  traceEnvChecked_           = false;

  short unlink_                    = 1;
  bool  unlinkEnvChecked_          = false;

  char stfsLocation_[STFS_PATH_MAX - STFS_SUFFIX + 1];
  bool locationEnvChecked_         = false;

  const short MAX_STFS_LOCATIONS   = 32;

  //Setting default STFS overflow type to HDD
  static int overflowType_       = STFS_HDD;

  typedef struct stfs_directories {
    char stfsDirs[MAX_STFS_LOCATIONS][STFS_PATH_MAX - STFS_SUFFIX+1];
    short numLocations;
    short currIndex;
    int   lastInstanceNum; 
    bool processedDirs;

    stfs_directories() : numLocations(0), currIndex(-1), lastInstanceNum(-1), processedDirs(false) {}
  } stfs_directories;

  stfs_directories stfsDirsHDD_, stfsDirsSSD_;

}

/// \brief Checks for the Trace Flag
void STFS_checkTrace() 
{

  char *lp_EnvTraceLevel = getenv("STFS_TRACE");
  short lv_EnvTraceLevel = 0;

  STFS_stub::traceEnvChecked_ = true;

  if (!lp_EnvTraceLevel) {
    return;
  }
  
  lv_EnvTraceLevel = (short) atoi(lp_EnvTraceLevel);

  if (lv_EnvTraceLevel <= 0) {
    STFS_stub::traceLevel_ = 0;
  }
  else {
    STFS_stub::traceLevel_ = lv_EnvTraceLevel;
  }

  if (! trace_get_fd()) { // check if there is already a trace file 

    char *lp_TraceFileName = getenv("STFS_TRACE_FILE");
    char lv_TraceFileName[512];

    if (lp_TraceFileName && lp_TraceFileName[0]) {
      memset(lv_TraceFileName,
	     0,
	     sizeof(lv_TraceFileName));
      strncpy(lv_TraceFileName,
	      lp_TraceFileName,
	      sizeof(lv_TraceFileName) - 1);
    }
    else {
      strcpy(lv_TraceFileName,
	     "stfs_trace");
    }

    trace_init(lv_TraceFileName,
	       true, 
	       (char *) "stfs", 
	       false);
  }
}

/// \brief Checks for the Unlink Flag
void STFS_checkUnlink() 
{

  if (STFS_stub::unlinkEnvChecked_) {
    return;
  }

  char *lp_EnvUnlink = getenv("STFS_UNLINK");
  int  lv_EnvUnlink = 0;

  STFS_stub::unlinkEnvChecked_ = true;

  if (!lp_EnvUnlink) {
    return;
  }
  
  lv_EnvUnlink = atoi(lp_EnvUnlink);

  if (lv_EnvUnlink <= 0) {
    STFS_stub::unlink_ = 0;
  }
  else {
    STFS_stub::unlink_ = 1;
  }

  return;

}

/// \brief Removes the Leading and Trailing Spaces from argument pp_str
/// \brief If the length of the resulting string is greater than pv_lengthCheck, 
///        it leaves pp_str intact
static
void
removeLeadingTrailingSpace(char *pp_str, int pv_lengthCheck)
{
  char lv_str[STFS_PATH_MAX];
  char *lv_ptr = 0;
  memset(lv_str, 0, STFS_PATH_MAX);
  strcpy(lv_str, pp_str);
  lv_ptr = lv_str;

  // Remove leading and trailing space
  char *tmpPtr = lv_ptr;
  while (isspace(*tmpPtr)) {
    tmpPtr++;
  }
  lv_ptr = tmpPtr;
  tmpPtr = lv_ptr + strlen(lv_ptr) -1;
  while (isspace(*tmpPtr)) {
    *tmpPtr = 0;
    tmpPtr--;
  }
  
  if ((int) strlen(lv_ptr) > pv_lengthCheck) {
    return;
  }

  strcpy(pp_str, lv_ptr);
}


/// \brief Extracts ':' separated strings from pp_locations
///        and copies it to STFS_stub::stfsDirs_[]
///        It also removes the leading & trailing spaces of 
///        these strings
static 
void
extractLocations(char *pp_locations, int pv_overflowtype)
{
  for (short i = 0; i < STFS_stub::MAX_STFS_LOCATIONS; i++) {
    if(pv_overflowtype == STFS_HDD) {
      memset(STFS_stub::stfsDirsHDD_.stfsDirs[i], 0, STFS_PATH_MAX - STFS_SUFFIX + 1);
    }
    else if(pv_overflowtype == STFS_SSD) {
      memset(STFS_stub::stfsDirsSSD_.stfsDirs[i], 0, STFS_PATH_MAX - STFS_SUFFIX + 1);
    }
  }

  char *lv_ptr1 = pp_locations;
  char *lv_ptr2 = 0;
  bool lv_done = false;

  while (lv_ptr1 && *lv_ptr1 && !lv_done) {

    lv_ptr2 = strchr(lv_ptr1, ':');

    if (lv_ptr2) {
      if(pv_overflowtype == STFS_HDD) {
        strncpy(STFS_stub::stfsDirsHDD_.stfsDirs[STFS_stub::stfsDirsHDD_.numLocations], lv_ptr1, lv_ptr2 - lv_ptr1);
        lv_ptr1 = lv_ptr2 + 1;
        if (STFS_stub::stfsDirsHDD_.numLocations >= (STFS_stub::MAX_STFS_LOCATIONS - 1)) {
    	  lv_done = true;
        }
      }
      else if(pv_overflowtype == STFS_SSD) {
        strncpy(STFS_stub::stfsDirsSSD_.stfsDirs[STFS_stub::stfsDirsSSD_.numLocations], lv_ptr1, lv_ptr2 - lv_ptr1);
        lv_ptr1 = lv_ptr2 + 1;
        if (STFS_stub::stfsDirsSSD_.numLocations >= (STFS_stub::MAX_STFS_LOCATIONS - 1)) {
  	  lv_done = true;
        }
      }
    }
    else {
      //Only one entry for overflow value
      lv_done = true;
      if(pv_overflowtype == STFS_HDD) {
        strcpy(STFS_stub::stfsDirsHDD_.stfsDirs[STFS_stub::stfsDirsHDD_.numLocations], lv_ptr1);
      }
      else if(pv_overflowtype == STFS_SSD) {
        strcpy(STFS_stub::stfsDirsSSD_.stfsDirs[STFS_stub::stfsDirsSSD_.numLocations], lv_ptr1);
      }
    }


    if(pv_overflowtype == STFS_HDD) {
      removeLeadingTrailingSpace(STFS_stub::stfsDirsHDD_.stfsDirs[STFS_stub::stfsDirsHDD_.numLocations], STFS_PATH_MAX - STFS_SUFFIX);
  
      if (strlen(STFS_stub::stfsDirsHDD_.stfsDirs[STFS_stub::stfsDirsHDD_.numLocations]) > 0) {
        ++ STFS_stub::stfsDirsHDD_.numLocations;
      }
    }

    if(pv_overflowtype == STFS_SSD) {
      removeLeadingTrailingSpace(STFS_stub::stfsDirsSSD_.stfsDirs[STFS_stub::stfsDirsSSD_.numLocations], STFS_PATH_MAX - STFS_SUFFIX);
  
      if (strlen(STFS_stub::stfsDirsSSD_.stfsDirs[STFS_stub::stfsDirsSSD_.numLocations]) > 0) {
        ++ STFS_stub::stfsDirsSSD_.numLocations;
      }
    }
  }
}

static
void
copyLocationHelper(STFS_stub::stfs_directories &pv_dir, int pv_instnum)
{

    if ((pv_dir.lastInstanceNum == -1) || (pv_dir.lastInstanceNum != pv_instnum)) {
      pv_dir.lastInstanceNum = abs(pv_instnum);
      pv_dir.currIndex = pv_instnum % pv_dir.numLocations;
    }
    else {
      pv_dir.currIndex++;
    }
    if (pv_dir.currIndex >= pv_dir.numLocations) {
      pv_dir.currIndex = 0;
    }
  
    if (pv_dir.numLocations > 0) {
      strcpy(STFS_stub::stfsLocation_, pv_dir.stfsDirs[pv_dir.currIndex]);
    }

}

/// \brief Simply copies the next STFS location to STFS_stub::stfsLocation_
static
void
copySTFSLocation(int pv_instnum)
{

  //Directory locations not specified 
  if ((STFS_stub::stfsDirsHDD_.numLocations <= 0) && (STFS_stub::overflowType_ == STFS_HDD)) {
    return;
  }
  if ((STFS_stub::stfsDirsSSD_.numLocations <= 0) && (STFS_stub::overflowType_ == STFS_SSD)) {
    return;
  }


  if(STFS_stub::overflowType_ == STFS_HDD) {
    copyLocationHelper(STFS_stub::stfsDirsHDD_, pv_instnum);
  }
  else if(STFS_stub::overflowType_ == STFS_SSD) {
    copyLocationHelper(STFS_stub::stfsDirsSSD_, pv_instnum);
  }

  if (STFS_stub::stfsLocation_[strlen(STFS_stub::stfsLocation_) - 1] != '/') {
    strcat(STFS_stub::stfsLocation_, "/");
  }

}

/// \brief Checks for the Location Env Variable 
///        and processes it
void STFS_checkLocation(int pv_instnum) 
{
  if (STFS_stub::locationEnvChecked_) {
    copySTFSLocation(pv_instnum);
    return;
  }

  char *lp_HDDEnvLocation = getenv("STFS_HDD_LOCATION");
  char *lp_SSDEnvLocation = getenv("STFS_SSD_LOCATION");
  char *lp_EnvLocation    = getenv("STFS_LOCATION");

  STFS_stub::locationEnvChecked_ = true;

  memset(STFS_stub::stfsLocation_, 0, sizeof(STFS_stub::stfsLocation_));

  if (!lp_HDDEnvLocation) {
    if (!lp_EnvLocation) {
      return;
    }
    //No HDD directory specified in configuration, use old location 
    extractLocations(lp_EnvLocation, STFS_HDD);

    if(lp_SSDEnvLocation) {
      extractLocations(lp_SSDEnvLocation, STFS_SSD);
    }
  }
  else {
    if(lp_HDDEnvLocation) {
      extractLocations(lp_HDDEnvLocation, STFS_HDD);
    }

    if(lp_SSDEnvLocation) {
      extractLocations(lp_SSDEnvLocation, STFS_SSD);
    }
  }
  
  copySTFSLocation(pv_instnum);

  return;
}

/*********   Trace Setup - End *****************/

//    -------------------------
//    STFS_FH_* Set Manipulators
//    -------------------------

/// \brief Removes a file handle from the set
void STFS_FH_CLR( stfs_fhndl_t  fhandle
                , fh_set       *set)
{
   FD_CLR(fhandle, set);
}

/// \brief Tests for existence of file handle in a set
int  STFS_FH_ISSET( stfs_fhndl_t  fhandle
                  , fh_set       *set)
{
   return FD_ISSET(fhandle, set);
}

/// \brief Adds a file handle to the set 
void STFS_FH_SET( stfs_fhndl_t  fhandle
                , fh_set       *set)
{
   FD_SET(fhandle, set);
}

/// \brief Clears all file handles from the set
void STFS_FH_ZERO(fh_set *set)
{
   FD_ZERO(set);
}

//    ----------------    
//    Functions 
//    ----------------

/// \brief Close a file handle
int STFS_close(stfs_fhndl_t fhandle)
{

   TRACE_PRINTF1(1, "STFS_close \n");

   return close(fhandle);
}

///  \brief Manipulate a file handle
int STFS_fcntl( stfs_fhndl_t fhandle
              , int          cmd
              , ...)
{
   TRACE_PRINTF2(1, "STFS_fcntl cmd: %d\n", cmd);

   if(cmd==F_SETFL)
   { 
      //if F_SETFL is used for second argument, 
      //   then third argument must also be set
      long arglong;
      va_list l_arg;
      va_start(l_arg, cmd);
      arglong = va_arg(l_arg, long);

      return fcntl(fhandle, cmd, arglong);
   }
   else
   {
      return fcntl(fhandle, cmd );
   }
}

/// \brief Reposition read/write file offset
off_t STFS_lseek( stfs_fhndl_t   fhandle
                , off_t          offset
                , int            whence)
{
   TRACE_PRINTF4(1, "STFS_lseek fhandle: %ld, offset: %ld, whence: %d\n", fhandle, offset, whence);

   ASSERT((unsigned long) offset <= STFS_SSIZE_MAX);

   return lseek(fhandle, offset, whence);
}

/// \brief Create and open a unique file name
stfs_fhndl_t STFS_mkstemp_instnum (char *ctemplate, int pv_instnum)
{
  short lv_retcode = 0;
  stfs_fhndl_t lv_fh = -1;

  TRACE_PRINTF2(1, "STFS_mkstemp_instnum, template: %s\n",
		ctemplate);

  STFS_checkUnlink();

  STFS_checkLocation(pv_instnum);

  errno=0;

  /* Perform some checks */
  if(ctemplate == NULL)
    {
      errno=EFAULT; 
      lv_retcode = -1; 
      goto _stfs_exit;
    }
  else if(strlen(ctemplate) > (STFS_PATH_MAX - STFS_SUFFIX))
    { 
      errno=ENAMETOOLONG;
      lv_retcode = -1; 
      goto _stfs_exit;
    }

  /* If STFS location is specified in the configuration, append the basename(ctemplate)
  ** to the STFS location. 
  */ 
  if (strlen(STFS_stub::stfsLocation_) > 0) {

    if ((strlen(ctemplate) + strlen(STFS_stub::stfsLocation_)) > (STFS_PATH_MAX - STFS_SUFFIX)) {
      errno=ENAMETOOLONG;
      lv_retcode = -1; 
      goto _stfs_exit;
    }

    char lv_temp[STFS_PATH_MAX + 1];
    memset(lv_temp, 0, sizeof(lv_temp));
    strcpy(lv_temp, ctemplate);
    char *lp_basename = basename(lv_temp);
    if (!lp_basename) {
      errno=EFAULT;
      lv_retcode = -1; 
      goto _stfs_exit;
    }

    char lv_ctemplate[STFS_PATH_MAX + 1];
    memset(lv_ctemplate, 0, sizeof(lv_ctemplate));
    sprintf(lv_ctemplate, "%s%s", STFS_stub::stfsLocation_, lp_basename);

    strcpy(ctemplate, lv_ctemplate);
  }

  /* Append XXXXXX to the template if necessary */
  if (strcmp(&ctemplate[strlen(ctemplate)-6], "XXXXXX") != 0) {
    strcat(ctemplate, "XXXXXX");
  }

  /* Create the temp file */
  lv_fh = mkstemp(ctemplate);
  if (lv_fh == -1) {
    lv_retcode = -1; 
    goto _stfs_exit;
  }

  /* Unlink the file */
  if (STFS_stub::unlink_ == 1) {
    lv_retcode = unlink(ctemplate);
    if (lv_retcode == -1) {
      lv_retcode = -1; 
      goto _stfs_exit;
    }
  }

 _stfs_exit:
  if (lv_retcode != 0) {
    lv_fh = -1;
  }

  TRACE_PRINTF6(1, "STFS_mkstemp_instnum file: %s, fh: %ld, location: %s, unlink: %d, err: %d\n",
		ctemplate,
		lv_fh,
		STFS_stub::stfsLocation_,
		STFS_stub::unlink_,
		errno);
  return lv_fh;
}

/// \brief Create and open a unique file name
stfs_fhndl_t STFS_mkstemp (char *ctemplate)
{

  TRACE_PRINTF2(1, "STFS_mkstemp, template: %s\n",
		ctemplate);
  return STFS_mkstemp_instnum(ctemplate, 0);
}

/// \brief Open a temporary file
stfs_fhndl_t STFS_open( const char *path   
                      , int         oflag
                      , ...)
{  
   TRACE_PRINTF2(1, "STFS_open: %s\n", path);

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
   return open(path, oflag);
}

/// \brief Read from a file handle
long STFS_read( stfs_fhndl_t     fhandle
             , void            *buf
             , size_t           count)
{
   TRACE_PRINTF4(1, "STFS_read fhandle: %ld, buf: %p, count: %ld\n", fhandle, buf, (long) count);

   ssize_t bytesReadCurr = 0;
   ssize_t bytesReadAll = 0;
   void    *lBuf = buf;
   short  lv_numIterations = 0;

   while((size_t) bytesReadAll < count)
   {   
     bytesReadCurr = read(fhandle, lBuf, count - bytesReadAll);

     if(bytesReadCurr < 0) {
       if (errno != EAGAIN) {
	 return -1; 
       }
       bytesReadCurr = 0;
     }   
     
     bytesReadAll += bytesReadCurr;
     lBuf = ((char *) lBuf) +  bytesReadCurr;

     if (++lv_numIterations > 5) {
       break;
     }
   }   

   TRACE_PRINTF4(1, "STFS_read fhandle: %ld, readCount: %ld, iterations: %d\n", fhandle, (long) bytesReadAll, lv_numIterations);
   return bytesReadAll;
}

/// \brief Synchronous I/O multiplexing
int STFS_select( stfs_fhndl_t    nfhs       
               , fh_set         *readfhs
               , fh_set         *writefhs
               , fh_set         *exceptfhs
               , struct timeval *timeout)
{
   TRACE_PRINTF1(1, "STFS_select\n");

   return select(nfhs, readfhs, writefhs, exceptfhs, timeout);
}

/// \brief Set the overflow disk type
int STFS_set_overflow(const int pv_overflowtype) {

   //Check that specified overflow type is valid
   if(pv_overflowtype == STFS_HDD) {       
     char *lp_HDDEnvLocation = getenv("STFS_LOCATION");
     if(lp_HDDEnvLocation) {
       STFS_stub::overflowType_ = STFS_HDD;
       return 0;
     }
     return 2;
   }
   else if (pv_overflowtype == STFS_SSD) {
     char *lp_SSDEnvLocation = getenv("STFS_SSD_LOCATION");
     if(lp_SSDEnvLocation) {
       STFS_stub::overflowType_ = STFS_SSD;
       return 0;
     }
     return 2;
   }
   else {
      return 1;
   }
      
}

/// \brief Delete a name and possibly the file that it refers to
int STFS_unlink (const char *path)
{
   TRACE_PRINTF2(1, "STFS_unlink: %s\n", path);
 
   int lv_retcode = 0;
   //Check that the pathname passed is not longer than our MAX
   ASSERT(strlen(path) <= STFS_PATH_MAX);
   
   lv_retcode = unlink(path);

   if (lv_retcode == -1) {
     if (errno == ENOENT) {
       errno = 0;
       lv_retcode = 0;
     }
   }

   return lv_retcode;
}

/// \brief Write to a file handle
ssize_t STFS_write( stfs_fhndl_t  fhandle
                  , const void   *buf
                  , size_t        count)
{
   int flag;
   ssize_t writeReturn=0;
   ssize_t bytesWritten=0;
   const void *lBuf = buf;
   off_t lOffset = 0;

   TRACE_PRINTF4(1, "STFS_write fhandle: %ld, buf: %p, count: %ld\n", fhandle, buf, (long) count);
 
   flag = fcntl(fhandle, F_GETFL);
   if((flag&O_APPEND)==O_APPEND)
   {
      lOffset = lseek(fhandle, 0, SEEK_END);
   } 
   else
   {  
      lOffset = lseek(fhandle, 0, SEEK_CUR);
   }

   if ((unsigned long) lOffset >= STFS_SSIZE_MAX - 1) {
      return 0;
   }

   while((size_t) bytesWritten < count)
   {   
      writeReturn = write(fhandle, lBuf, count - bytesWritten);

      if(writeReturn < 0) {
         if (bytesWritten <= 0) {
            return -1; 
         }
         else {
            break;
         }
      }   

      bytesWritten += writeReturn;
      lOffset += writeReturn;
      if ((unsigned long) lOffset >= STFS_SSIZE_MAX - 1) {
         break;
      }   
      lBuf = ((char *) lBuf) +  bytesWritten;
    
    }   

  return bytesWritten;
}

