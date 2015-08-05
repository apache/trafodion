/*! \file    stfslib.h
    \brief   STFS initial header file

    Contains the functional API for STFS.

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

#ifndef STFSLIB_H
#define STFSLIB_H
//    ----------------
//    Base includes
//    ----------------
/* Get O_NONBLOCK and other definitions */
#include <fcntl.h>       
//#include <stdio.h>        //for flags    
/* Get fd_set */
#include <sys/select.h> 
/* Get timeval */
#include <bits/time.h> 
#include <vector>

//    ----------------
//    Defines
//    ----------------
/* \brief Maximum number of file handles in an fh_set. */
#define FH_ARRAY_SIZE 32   
/* \brief Maximum file name size. */
#define STFS_NAME_MAX 255 
/* \brief NULL value for stfs_fhndl_t. */
#define STFS_NULL_FHNDL -1
/* \brief Maximum Pathname size including NULL terminator (STFS_NAME_MAX + 1). */
#define STFS_PATH_MAX 256 

/* \brief Maximum size of STFS specific suffix in the STFS file name */
#define STFS_NAME_SUFFIX_MAX          40    

/* \brief Defines for SSD and HDD options */
#define STFS_HDD 0
#define STFS_SSD 1

/* \brief 2Gb (1024*1024*1024)*2 */
//#define STFS_SSIZE_MAX 2147483648L
const unsigned long STFS_SSIZE_MAX= ((unsigned long)2 * ((unsigned long)(1024*((long)(1024*1024)))));

/* \brief Bit mask flags for the 'mask' parameter of the STFS_stat/STFS_fstat API */
#define  S_STFS_NID         01   /* \brief Node Id number */
#define  S_STFS_MODE        02   /* \brief Protection */
#define  S_STFS_OPENS      010   /* \brief Current Number of Opens */
#define  S_STFS_NFRAG      020   /* \brief Number of STFS File Fragments */
#define  S_STFS_UGID      0300   /* \brief Bit mask for owner */
#define  S_STFS_UID       0100   /* \brief User ID of owner */
#define  S_STFS_GID       0200   /* \brief Group ID of owner */
#define  S_STFS_SIZE     01000   /* \brief Total size, in bytes */
#define  S_STFS_AMCTM   070000   /* \brief Bit mask for times */
#define  S_STFS_ATIME   010000   /* \brief Time of last access */
#define  S_STFS_MTIME   020000   /* \brief Time of last modification */
#define  S_STFS_CTIME   040000   /* \brief Time of last status change */

//    ----------------
//    Type Definitions 
//    ----------------

/// \typedef    long stfs_fhndl_t
/// \brief      Type Definition for stfs_fhndl_t object
typedef long stfs_fhndl_t;
/// \typedef    long nfhs_t
/// \brief      Type Definition for count of Fhandles
typedef long nfhs_t;

typedef int stfs_nodeid_t;     

typedef unsigned int stfs_statmask_t;

typedef unsigned int stfs_mode_t;       

typedef unsigned int stfs_nopens_t;     

typedef unsigned int stfs_nfrag_t;      

#define STFS_STUB
#ifdef STFS_STUB
/*! \typedef    fd_set fh_set
    \brief      Type Definition for fh_set object
*/
/*typedef fd_set fh_set; */
#define fh_set fd_set
#else
//    ----------------
//    Structures
//    ----------------
// struct fh_set will be used when STFS_select and STFS_FH_* are fully implemented.
struct fh_set{
   std::vector<stfs_fhndl_t> FhArray;
};
#endif

struct stfs_stat {
    stfs_nodeid_t     nid;    
    stfs_mode_t       mode;  
    stfs_nopens_t     opens;
    stfs_nfrag_t      nfrag;
    uid_t             uid; 
    gid_t             gid;
    off_t             size;  
    time_t            atime;
    time_t            mtime; 
    time_t            ctime;
};


//    ----------------
//    Macros
//    ----------------
/*! \name      Set manipulation macros 
*/
//@{

/*! \brief Removes a file handle from the set 
    \param[in]    fhandle         file handle returned from STFS_open or STFS_mkstemp.
    \param[in]    set             container of fhandles with corresponding  handle 
                                  indicator. Can contain a maximum of FH_SETSIZE handles.
*/
void STFS_FH_CLR( stfs_fhndl_t  pv_fhandle         
                , fh_set       *pp_set);           

/*! \brief Tests for existence of file handle in a set
    \param[in]    fhandle         file handle returned from STFS_open or STFS_mkstemp.
    \param[in]    set             container of fhandles with corresponding  handle 
                                  indicator. Can contain a maximum of FH_SETSIZE handles
    \retval       int             SUCCESS: Returns a non-zero value if the file handle
                                           is set.\n\n
                                  Error:   0
*/
int  STFS_FH_ISSET( stfs_fhndl_t  pv_fhandle       
                  , fh_set       *pp_set);         

/*! \brief Adds a file handle to the set 
    \param[in]    fhandle         file handle returned from STFS_open or STFS_mkstemp.
    \param[in]    set             container of fhandles with corresponding  handle 
                                  indicator. Can contain a maximum of FH_SETSIZE handles
*/
void STFS_FH_SET( stfs_fhndl_t  pv_fhandle         
                , fh_set       *pp_set);           

/*! \brief Clears all file handles from the set 
    \param[in]    set             container of fhandles with corresponding  handle 
                                  indicator. Can contain a maximum of FH_SETSIZE handles
*/
void STFS_FH_ZERO(fh_set *pp_set);                 
//@}

//    ----------------    
//    Functions 
//    ----------------

//---------------------------------------------------------------------------//
/*!  \brief Close a file handle
 
     The STFS_close function closes the given fhandle. When all open file 
     handles have been closed, the temporary file is unlinked, i.e., deleted.

     \param[in]   fhandle   file handle returned from STFS_open or STFS_mkstemp

     \retval      int       Success: 0      \n\n
                            Error:   1
*/
//--------------------------------------------------------------------------//
int STFS_close(stfs_fhndl_t fhandle);           

//--------------------------------------------------------------------------//
/*!  \brief Get error information for a file handle

     The STFS_error function extracts error information stored in an
     STFS file handle.  This information is reset at the beginning of
     each STFS API call that takes a file handle.

     \param[in]  fhandle    file handle returned from STFS_open or STFS_mkstemp
     \param[out] error      errno for the first error
     \param[out] addlError  Additional error, dependent on the
                            original error.  Value might not be in
			    error.h 
     \param[out] context    Textual context for error and addlError
     \param[in]  contextMaxLen  Max length for context buffer
     \param[out] contextLen Actual length for the context buffer

     \retval     int SUCCESS = 0
                     FAIL = =1. errno is not set in this case.

*/

int STFS_error ( stfs_fhndl_t  pv_fhandle,
		 int          *pp_error,
		 int          *pp_addlError,
		 char         *pp_context,
		 size_t        pv_contextMaxLen,
		 size_t       *pp_contextLen );

//---------------------------------------------------------------------------//
/*!  \brief Manipulate a file handle
 
     The STFS_fcntl function controls or returns open file attributes depending 
     on the value of cmd.

     \param[in]   fhandle   file handle returned from STFS_open or STFS_mkstemp
     \param[in]   cmd       values are defined in <fcntl.h> and are as follows: \n
                                 F_GETFL - get file status flags \n
                                 F_SETFL - set file status flags
     \param[in]   (opt)arg  values are defined in <fcntl.h> and are as follows: \n
                                 O_NONBLOCK - Non-blocking mode 

     \retval       int      SUCCESS: Value returned shall depend on cmd as follows:\n
                                     F_GETFL -  Value of file status flags and access
                                                modes.  The return value is not negative
                                                on success.\n
                                     F_SETFL -  Zero on sucess.\n\n
                            ERROR:   -1 is returned and errno is set. 
   
*/
//--------------------------------------------------------------------------//
int STFS_fcntl (stfs_fhndl_t fhandle, 
                int cmd, 
                ...);              

//---------------------------------------------------------------------------//
/*!  \brief Reposition read/write file offset
 
     Repositions the offset of the file handle fhandle to the argument offset 
     according to the argument whence.

     \param[in]   fhandle   file handle returned from STFS_open or STFS_mkstemp
     \param[in]   offset    byte offset count based on whence
     \param[in]   whence    values are as follow: \n
                            SEEK_SET - The offset is set to offset bytes. \n
                            SEEK_CUR - The offset is set to its current
                                       location plus offset bytes. \n
                            SEEK_END - The offset is set to the size of the 
                                       file plus offset bytes. 
     \retval      off_t     SUCCESS: The resulting offset location from the 
                                     beginning of the file.\n\n
                            Error:   -1 is returned and errno is set.
*/
//--------------------------------------------------------------------------//
off_t STFS_lseek( stfs_fhndl_t   fhandle         
                , off_t          offset         
                , int            whence);       

//---------------------------------------------------------------------------//
/*!  \brief Create and open a unique file name
 
     The STFS_mkstemp function appends the string pointed to by template with 
     a unique suffix. Creates a new empty file and opens the file for reading 
     and writing. The file is created with mode read/write and permissions 0600. 

     \param[in]   ctemplate   must not be a string constant, it should be 
                              declared as a character array of at least  
                              STFS_PATHMAX bytes to include the null terminator. 
                              Pathnames with subdirectories are not valid since 
                              there is no ability to create directories in STFS. 
                              A null string is valid on input.

     \retval     stfs_fhndl_t SUCCESS: returns an open file handle.\n\n
                              ERROR:   -1 is returned and errno is set.
*/
//--------------------------------------------------------------------------//
stfs_fhndl_t STFS_mkstemp(char *ctemplate);     

//---------------------------------------------------------------------------//
/*!  \brief Create and open a unique file name. Try to use the instance 
            number parameter as a hint.
 
     The STFS_mkstemp_instnum function appends the string pointed to by 
     template with a unique suffix. Creates a new empty file and opens the 
     file for reading and writing. 
     The file is created with mode read/write and permissions 0600. 

     \param[in]   ctemplate   must not be a string constant, it should be 
                              declared as a character array of at least  
                              STFS_PATHMAX bytes to include the null terminator. 
                              Pathnames with subdirectories are not valid since 
                              there is no ability to create directories in STFS. 
                              A null string is valid on input.

     \param[in]  instanceNum  A hint to the routine to help with picking an
                              appropriate STFS directory.

     \retval     stfs_fhndl_t SUCCESS: returns an open file handle.\n\n
                              ERROR:   -1 is returned and errno is set.
*/
//--------------------------------------------------------------------------//
stfs_fhndl_t STFS_mkstemp_instnum(char *ctemplate, int instanceNum);     

//---------------------------------------------------------------------------//
/*!  \brief Open a temporary file  
 
     The STFS_open function opens the file pointed to by the path argument. 
     The value of the path argument must be the value returned in previous call 
     to STFS_mkstemp(). 

     \param[in]   path    Must the size specified in the template argument in 
                          STFS_mkstemp()below or it must be the value of the STFS 
                          file returned from STFS_openers, STFS_fopeners, or 
                          STFS_mkstemp. 
     \param[in]   oflag   values are constructed by a bitwise-inclusive OR of 
                          flags from the following list, defined in <fcntl.h>. 
                          One of the first three values (file access modes) below 
                          must be specified in the value of oflag: \n
                          O_RDONLY - Open for reading only. \n
                          O_WRONLY - Open for writing only. \n
                          O_RDWR - Open for reading and writing. \n
                          Any combination of the following may be used: \n
                          O_APPEND - The file is opened in append mode. Before each 
                          write(), the file offset is positioned at the end of the 
                          file, as if with lseek(). \n
                          O_NONBLOCK - Non-blocking mode. \n
                          O_TRUNC - If the file already exists and the open mode 
                          allows writing (i.e., is O_RDWR or O_WRONLY) it will be 
                          truncated to length 0.


     \retval stfs_fhndl_t SUCCESS: Returns an open file handle.\n\n
                          ERROR:   -1 is returned and errno is set.
*/
//--------------------------------------------------------------------------//
stfs_fhndl_t STFS_open( const char *path        
                      , int         oflag
                      , ...);

//---------------------------------------------------------------------------//
/*!  \brief Read from a file handle
 
     STFS_read() attempts to read up to count bytes from file handle fhandle into 
     the buffer starting at buf. 

     \param[in]   fhandle   file handle returned from STFS_open or STFS_mkstemp
     \param[out]  buf       buffer where the read contents are returned.
     \param[in]   count     value between zero and SSIZE_MAX.

     \retval      int       SUCCESS: The number of bytes read is returned, the 
                                     file position is advanced by this number.\n
                                     Zero indicates end of file\n\n
                            ERROR:   -1 is returned and errno is set. 
*/
//--------------------------------------------------------------------------//
long STFS_read( stfs_fhndl_t     fhandle         
             , void            *buf             
             , size_t           count);         

//---------------------------------------------------------------------------//
/*!  \brief Synchronous I/O multiplexing
 
     Allows a program to monitor multiple file handles, waiting until one or more 
     STFS file handles has a completion to an I/O operation.

     \param[in]       nfhs       specifies the number of handles to be tested in 
                                 each set.
     \param[in,out]   readfhs    if not a null pointer, points to an object type 
                                 fh_set that on input specifies the file handles to
                                 be watched to see if a read will not block (the file
                                 handles that are ready to read), on output indicates
                                 which file handles are ready to read.
     \param[in,out]   writefhs   if not a null pointer, points to an object type fh_set
                                 that on input specifies the file handles to be watched
                                 to see if a write will not block (the file handles that 
                                 are ready to write), on output indicates which file 
                                 handles are ready to write.
     \param[in,out]   exceptfhs  if not a null pointer, points to an object type fh_set
                                 that on input specifies the file handles to be watched 
                                 to see if there is a pending error condition, on output 
                                 indicates which file handles have error conditions pending.
     \param[in]       timeout    the length of time the handles will be watched. The timeout 
                                 period is given in seconds and microseconds. If the timeout 
                                 period is zero, it will return immediately if no file handles 
                                 are ready to read, write, or have pending error conditions. 
     \retval          int        SUCCESS: The number of file handles contained in the three 
                                          returned file handle sets. The number may be zero if 
                                          the timeout expires and no file handles are ready to 
                                          read, ready to write, or have a pending error
                                          condition.\n\n 
                                 ERROR:   -1 is returned and errno is set.
*/
//--------------------------------------------------------------------------//
int STFS_select( stfs_fhndl_t    nfhs           
               , fh_set         *readfhs        
               , fh_set         *writefhs      
               , fh_set         *exceptfhs      
               , struct timeval *timeout);      


//---------------------------------------------------------------------------//
/*!  \brief Set the overflow disk type
      
     The STFS_set_overflow function allows the user to set the type of disk
     is being used for overflow.  Current possible disk types are HDD or SSD.

     \param[in]   overflowtype  values are defined as:   STFS_HDD
                                                         STFS_SSD
     \retval      int   SUCCESS: 0 \n\n
                         ERROR:   non-zero value
                                         1   invalid overflow type
                                         2   selected overflow type has not been configured
*/
//---------------------------------------------------------------------------//
int STFS_set_overflow(const int pv_overflowtype);

//---------------------------------------------------------------------------//
/*!  \brief Delete a name and possibly the file that it refers to
 
     The STFS_unlink function deletes a name from the file system. If no processes 
     have the file open the file is deleted. Otherwise, the file remains in 
     existence until the last file handle referring to the file is closed.

     \param[in]   path   must be value of the STFS file returned from STFS_openers, 
                         STFS_fopeners, or STFS_mkstemp. 

     \retval      int    SUCCESS: 0      \n\n
                         ERROR:   -1 is returned and errno is set
*/
//--------------------------------------------------------------------------//
int STFS_unlink(const char* path);              

//---------------------------------------------------------------------------//
/*!  \brief Write to a file handle
 
     The STFS_write function writes up to the count bytes to the file referenced
     by fhandle from the buffer starting at buf.

     \param[in]   fhandle   file handle returned from STFS_open or STFS_mkstemp
     \param[in]   buf       buffer with the write contents
     \param[in]   count     value between zero and SSIZE_MAX

     \retval      ssize_t   SUCCESS: Returns the number of bytes written\n
                                     A Zero indicates nothing was written\n\n
                            ERROR:   -1 is returned and errno is set
*/
//--------------------------------------------------------------------------//
ssize_t STFS_write( stfs_fhndl_t  fhandle       
                  , const void   *buf           
                  , size_t        count);       



// Rev: Add doxygen comments
#ifndef STFS_STUB
//  STFS utilities...


int STFS_openers(stfs_nodeid_t     pv_nid,      
                 char             *pp_path,    
                 long             *pp_previndex,   
                 stfs_nodeid_t    &pv_openernid,  
                 pid_t            &pv_openerpid, 
                 char             *pp_openername);

int STFS_fopeners(stfs_fhndl_t     pv_Fhandle,      
                  long            *pp_PrevIndex,    
                  stfs_nodeid_t   &pv_OpenerNid,  
                  pid_t           &pv_OpenerPid,  
                  char            *pp_OpenerName, 
                  char            *pp_OpenPath);

int STFS_stat(stfs_nodeid_t     pv_Nid,
              char             *pp_Path,
              long             *pp_PrevIndex,
              stfs_statmask_t   pv_Mask,
              struct stfs_stat *pp_Buf); 

int STFS_fstat(stfs_fhndl_t      pv_Fhandle,
               stfs_statmask_t   pv_Mask,
               struct stfs_stat *pp_Buf); 
#endif

#endif /* STFSLIB_H */

