///////////////////////////////////////////////////////////////////////////////
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
//
///////////////////////////////////////////////////////////////////////////////

#ifndef DEVICE_H_
#define DEVICE_H_
#ifndef NAMESERVER_PROCESS

#include "msgdef.h"
#include <string>

class CProcess;

class CDevice
{
private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:

    bool      Mounted;
    int       Nid;
    int       Pid;   // Process currently owning device

    CDevice( string name );
   ~CDevice( void );

    void     DeLink( CDevice **head, CDevice **tail );
    CDevice *GetNext( void );
    CDevice *Link( CDevice *entry );   
    bool     Mount( CProcess *process, const char *device );
    bool     UnMount( void );
    const char *GetName( void ) { return name_.c_str(); }

protected:
private:
    CDevice *Next;
    CDevice *Prev;
    string  name_;
};

class CLogicalDevice
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    CDevice *Primary;
    CDevice *Mirror;

    CLogicalDevice(const char *name, CDevice *primary, CDevice *backup, int primaryZid, int backupZid);
   ~CLogicalDevice(void);

    void    DeLink( CLogicalDevice **head, CLogicalDevice **tail );
    CLogicalDevice *GetNext( void );
    int     GetBackupZone( void ) { return backupZid_; }
    const char *GetName( void ) { return name_.c_str(); }
    int     GetPrimaryZone( void ) { return primaryZid_; }
    CLogicalDevice *Link( CLogicalDevice *entry );   
    bool    MirrorMounted( void );
    bool    Mount( CProcess *process, bool replicate = true );
    bool    Mounted( void );
    bool    PrimaryMounted( void );
    bool    UnMount( bool replicate = true );
    const char *name() { return name_.c_str(); }

protected:
    CLogicalDevice *Next;
    CLogicalDevice *Prev;
private:
    string  name_;
    int     primaryZid_;
    int     backupZid_;
};

class CDeviceContainer
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    CLogicalDevice *Head;

    CDeviceContainer( void );
   ~CDeviceContainer( void );

    CLogicalDevice *CloneDevice( CProcess *process );
    CLogicalDevice *CreateDevice( CProcess *process );
    CLogicalDevice *GetLogicalDevice( const char *name );
    bool    MountVolume( const char *name, CProcess *process, CLogicalDevice **ldevice ); 
    bool    UnMountVolume( const char *name, bool isBackup ); 
    bool    IsInitialized() { return initialized_; }

protected:
private:
    CDevice         *DeviceHead;
    CDevice         *DeviceTail;
    CLogicalDevice  *Tail; 
    bool            initialized_; // true if successful unmountall

    CLogicalDevice *ConfigDevice( CProcess *process );
};

#endif
#endif /*DEVICE_H_*/
