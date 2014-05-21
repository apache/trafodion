///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LUNMGR_H_
#define LUNMGR_H_

#include <list>
#include <string>
using namespace std;

#include "system.h"

#define LUNMGR_STATE_UNMOUNT_ERROR                    10
#define LUNMGR_STATE_UNMOUNT_ERROR_DEVICE_BUSY        11
#define LUNMGR_STATE_UNMOUNT_ERROR_DEVICE_NOT_MOUNTED 12
#define LUNMGR_STATE_UNMOUNT_ERROR_DEVICE_NOT_FOUND   13
#define LUNMGR_STATE_MOUNT_ERROR_BAD_INVOKE           20
#define LUNMGR_STATE_MOUNT_ERROR_SYSERR               21
#define LUNMGR_STATE_MOUNT_ERROR_INTBUG               22
#define LUNMGR_STATE_MOUNT_ERROR_INTR                 23
#define LUNMGR_STATE_MOUNT_ERROR_MTAB_ERR             24
#define LUNMGR_STATE_MOUNT_ERROR_MOUNT_FAILED         25
#define LUNMGR_STATE_MOUNT_ERROR_SOME_SUCCEEDED       26

typedef list<string>    OutputList_t;

class CLunMgr : public CSystem
{
public:
    CLunMgr( const char *utility );
   ~CLunMgr( void );

    // The caller should save and close stdin before calling this proc
    // and restore it when done. This is to prevent ssh from consuming
    // caller's stdin contents when executing the command. 
    int     LaunchCommand( const char *command, int &pid );
    int     WaitCommand( int pid );
    
private:
    OutputList_t    outputList_;
    
    const char *LunMgrErrorString( int status);
};

#endif /*LUNMGR_H_*/
