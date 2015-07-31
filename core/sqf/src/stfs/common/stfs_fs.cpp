///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfs_fs.cpp
/// \brief   Implementation of the STFS_fs class methods
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

// for close, lseek, read, unlik, write 
//#include <unistd.h>
// for mkstemp
//#include <stdlib.h>
// for assert 
//#include <assert.h>
// for strlen, strcpy
//#include <string.h>
// for errno defines
//#include <errno.h>
// for strcpy 
//#include <string.h>

#include "stfs_fs.h"
#include "stfs_util.h"
#include "stfs_defs.h"

using namespace STFS;

//    -------------------------
//    Constructor/Destructor
//    -------------------------

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_fs::STFS_fs()
///
/// \brief  Default Constructor for STFS_fs
///
/// Constructor for the STFS_fs class. Sets initial values for all variables. 
///
///////////////////////////////////////////////////////////////////////////////
STFS_fs::STFS_fs():
  fd_(-1),
  filename_(0),
  isOpen_(false)
{   
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_fs::STFS_fs()
///
/// \brief  Constructor for STFS_fs.  Takes in a parameter for the Filename
///
/// Constructor for the STFS_fs class. Sets the value for the Filename and sets
/// default values for other variables.
///
/// \param  inFilename  Name of the file associated with STFS_fs
///
///////////////////////////////////////////////////////////////////////////////
STFS_fs::STFS_fs(char *inFilename) : fd_(-1),
				     isOpen_(false)
{
   //validate inFilename length not larger than STFS_FILENAME_MAX
  // Rev: Do we need to assert in a 'release' build - maybe call a SoftwareFailureHandler()/SigIll()
   ASSERT(strlen(inFilename) <= STFS_NAME_MAX);   
   filename_ = new char[strlen(inFilename) + 1];
   strcpy(filename_, inFilename);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_fs::~STFS_fs()
///
/// \brief  Destructor for STFS_fs.
///
/// Destructor for the STFS_fs class.  Closes the file and deletes filename_.
/// Sets fd_ to -1 and isOpen_ to false. 
///
///////////////////////////////////////////////////////////////////////////////
STFS_fs::~STFS_fs()
{
  if (fd_>0) {
    ::close(fd_);
  }
   fd_=-1;
   delete [] filename_; filename_ = 0;
   isOpen_ = false;
}

//    -------------------------
//    STFS_FH_* Set Manipulators
//    -------------------------

///////////////////////////////////////////////////////////////////////////////
///            
/// \brief   STFS_fs::fd_CLR() 
///
///          Removes a file handle from the set
///
/// \param[in]    set             container of fhandles with corresponding  handle 
///                               indicator. Can contain a maximum of FH_SETSIZE handles.
///
///////////////////////////////////////////////////////////////////////////////
void STFS_fs::fd_CLR(fd_set       *set)
{
   FD_CLR(fd_, set);
}

///////////////////////////////////////////////////////////////////////////////
///
//         STFS_fs::fd_ISSET()
///
/// \brief Tests for existence of file handle in a set
///
/// \param[in]    set             container of fhandles with corresponding  handle 
///                               indicator. Can contain a maximum of FH_SETSIZE handles
/// \retval       int             SUCCESS: Returns a non-zero value if the file handle
///                                        is set.\n\n
///                               Error:   0
///
///////////////////////////////////////////////////////////////////////////////
int STFS_fs::fd_ISSET(fd_set       *set)
{
   return FD_ISSET(fd_, set);
}

///////////////////////////////////////////////////////////////////////////////
///
//         STFS_fs::fd_SET()
///
/// \brief Adds a file handle to the set 
///
/// \param[in]    set             container of fhandles with corresponding  handle 
///                               indicator. Can contain a maximum of FH_SETSIZE handles
///
///////////////////////////////////////////////////////////////////////////////
void STFS_fs::fd_SET(fd_set       *set)
{
   FD_SET(fd_, set);
}

///////////////////////////////////////////////////////////////////////////////
///
//         STFS_fs::fd_ZERO()
///
/// \brief Clears all file handles from the set 
///
/// \param[in]    set             container of fhandles with corresponding  handle 
///                               indicator. Can contain a maximum of FH_SETSIZE handles
///
///////////////////////////////////////////////////////////////////////////////
void STFS_fs::fd_ZERO(fd_set *set)
{
   FD_ZERO(set);
}

