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

#include <iostream>

using namespace std;

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "monlogging.h"
#include "montrace.h"
#include "config.h"
#include "monitor.h"
#include "clusterconf.h"
#include "lock.h"
#include "lnode.h"
#include "pnode.h"
#include "device.h"
#include "replicate.h"

//
// Lunmgr mount and unmount errors
//
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

extern bool NameServerEnabled;
extern int MyPNID;
extern CMonitor *Monitor;
extern CNodeContainer *Nodes;
extern CConfigContainer *Config;
extern CNode *MyNode;
extern CReplicate Replicator;

extern char *ErrorMsg (int error_code);

CDevice::CDevice( string name )
        : Mounted (false),
          Nid (-1),
          Pid (-1),
          Next (NULL),
          Prev (NULL),
          name_ (name)
{
    const char method_name[] = "CDevice::CDevice";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "DEVC", 4);

    TRACE_EXIT;
}

CDevice::~CDevice( void )
{
    const char method_name[] = "CDevice::~CDevice";
    TRACE_ENTRY;
    if ( Mounted )
    {
        UnMount();
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "devc", 4);

    TRACE_EXIT;
}

void CDevice::DeLink( CDevice **head, CDevice **tail )
{
    const char method_name[] = "CDevice::DeLink";
    TRACE_ENTRY;
    if (*head == this)
        *head = Next;
    if (*tail == this)
        *tail = Prev;
    if (Prev)
        Prev->Next = Next;
    if (Next)
        Next->Prev = Prev;
    TRACE_EXIT;
}

CDevice *CDevice::GetNext( void )
{
    const char method_name[] = "CDevice::GetNext";
    TRACE_ENTRY;
    TRACE_EXIT;
    return Next;
}

CDevice *CDevice::Link( CDevice *entry )
{
    const char method_name[] = "CDevice::Link";
    TRACE_ENTRY;
    Next = entry;
    entry->Prev = this;
    TRACE_EXIT;
    return entry;
}

const char *LunmgrErrorString( int state)
{
    const char *str;
    
    switch( state )
    {
        case LUNMGR_STATE_UNMOUNT_ERROR:
            str = "Unmount failed";
            break;
        case LUNMGR_STATE_UNMOUNT_ERROR_DEVICE_BUSY:
            str = "Device busy";
            break;
        case LUNMGR_STATE_UNMOUNT_ERROR_DEVICE_NOT_MOUNTED:
            str = "Device not mounted";
            break;
        case LUNMGR_STATE_UNMOUNT_ERROR_DEVICE_NOT_FOUND:
            str = "Device not found";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_BAD_INVOKE:
            str = "Bad mount invocation";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_SYSERR:
            str = "System error on mount";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_INTBUG:
            str = "Internal mount bug or missing NFS support in mount";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_INTR:
            str = "User interrupt on mount";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_MTAB_ERR:
            str = "mtab error on mount";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_MOUNT_FAILED:
            str = "Mount failed";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_SOME_SUCCEEDED:
            str = "Some mount succeeded";
            break;
        default:
            str = "Unknown lunmgr error";
            break;
    }

    return( str );
}

bool CDevice::Mount( CProcess *process, const char *device )
{
    bool rstate;
    char cmd[132];
    int  status;
    
    const char method_name[] = "CDevice::Mount";
    TRACE_ENTRY;
    if ( Mounted )
    {
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST)) 
            trace_printf("%s@%d - Device %s already has been mounted by process (%d, %d)\n", method_name, __LINE__, name_.c_str(), Nid, Pid);
        return false;
    }
    if ( !process->IsClone() )
    {
        sprintf(cmd,"lunmgr --nodoublemount --sqmount '%s'", device);

        if (trace_settings & TRACE_REQUEST)
            trace_printf("%s@%d - lungmgr cmd: %s\n", method_name, __LINE__, cmd);

        status = system(cmd);

        if ( status == -1 )
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s]: lunmgr --sqmount '%s' failed, for process "
                    "%s (%d, %d), errno=%d, %s.\n",
                    method_name, name_.c_str(), process->GetName(), process->GetNid(),
                    process->GetPid(), errno, strerror(errno));
            mon_log_write(MON_DEVICE_MOUNT_1, SQ_LOG_CRIT, buf);

            rstate = false;
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
                trace_printf("%s@%d - fork failed on device %s for process %s (%d, %d)\n", method_name, __LINE__, name_.c_str(), process->GetName(), process->GetNid(), process->GetPid());
        }    
        else
        {
            status = WEXITSTATUS(status);
            if (status)
            {
                rstate = false;

                // Write critical event to evlog
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[%s], mount failed on device %s "
                        "for process %s (%d,%d), lunmgr error=%d(%s)\n", method_name,
                        name_.c_str(), process->GetName(), process->GetNid(),
                        process->GetPid(), status, LunmgrErrorString(status));
                mon_log_write(MON_DEVICE_MOUNT_2, SQ_LOG_CRIT, la_buf);
            }
            else
            {
                rstate = true;
                Mounted = true;
                Nid = process->GetNid();
                Pid = process->GetPid();
                if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
                    trace_printf("%s@%d - device %s has been mounted, owning process %s (%d, %d)\n", method_name, __LINE__, name_.c_str(), process->GetName(), Nid, Pid);
            }
        }
    }
    else
    {
        rstate = true;
        Mounted = true;
        Nid = process->GetNid();
        Pid = process->GetPid();
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
            trace_printf("%s@%d - Device %s mount not processed for clone process %s (%d, %d)\n", method_name, __LINE__, name_.c_str(), process->GetName(), Nid, Pid);
    }

    TRACE_EXIT;
    return rstate;
}

bool CDevice::UnMount( void )
{
    bool rstate;
    char cmd[132];
    int  status;

    const char method_name[] = "CDevice::UnMount";
    TRACE_ENTRY;
    if ( !Mounted )
    {
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
            trace_printf("%s@%d - Device %s has not been mounted" "\n", method_name, __LINE__, name_.c_str());
        return true;
    }
    if (MyNode->IsMyNode(Nid))
    {
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
          trace_printf("%s@%d - Unmounting device %s, owning process (%d, %d)\n", method_name, __LINE__, name_.c_str(), Nid, Pid);

        sprintf(cmd,"lunmgr --squnmount '%s'", name_.c_str());
        status = system(cmd);
        if ( status == -1 )
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s]: lunmgr --squnmount '%s' failed, for process "
                    "(%d, %d), errno=%d, %s.\n", method_name, name_.c_str(),
                    Nid, Pid, errno, strerror(errno));
            mon_log_write(MON_DEVICE_UNMOUNT_1, SQ_LOG_CRIT, buf);

            rstate = false;
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
                trace_printf("%s@%d - fork failed on device %s for process (%d, %d)\n", method_name, __LINE__, name_.c_str(), Nid, Pid);
        }    
        else
        {
            status = WEXITSTATUS(status);
            if (status)
            {
                rstate = false;
                // Write critical event to evlog
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[%s], unmount failed on device %s "
                        "for process (%d,%d), lunmgr error=%d(%s)\n", method_name,
                        name_.c_str(), Nid, Pid, status, LunmgrErrorString(status));
                mon_log_write(MON_DEVICE_UNMOUNT_2, SQ_LOG_CRIT, la_buf);
                //if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
                //    trace_printf("%s@%d - umount failed on device %s for process (%d, %d)\n", method_name, __LINE__, name_.c_str(), Nid, Pid);
            }
            else
            {
                rstate = true;
                Mounted = false;
                if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
                    trace_printf("%s@%d - device %s has been unmounted, owning process (%d, %d)\n", method_name, __LINE__, name_.c_str(), Nid, Pid);
                Nid = -1;
                Pid = -1;
            }
        }
    }
    else
    {
        rstate = true;
        Mounted = false;
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
            trace_printf("%s@%d - Device %s unmount not processed for clone process (%d, %d)\n", method_name, __LINE__, name_.c_str(), Nid, Pid);
        Nid = -1;
        Pid = -1;
    }

    TRACE_EXIT;
    return rstate;
}

CLogicalDevice::CLogicalDevice(const char *name, CDevice *primary, CDevice *mirror, int primaryZid, int backupZid)
               :Primary (primary),
                Mirror (mirror),
                Next (NULL),
                Prev (NULL),
                primaryZid_(primaryZid),
                backupZid_(backupZid)
{
    const char method_name[] = "CLogicalDevice::CLogicalDevice";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "LDEV", 4);

    name_ = name;
    TRACE_EXIT;
}

CLogicalDevice::~CLogicalDevice(void)
{
    const char method_name[] = "CLogicalDevice::~CLogicalDevice";
    TRACE_ENTRY;

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "ldev", 4);

    TRACE_EXIT;
}

void CLogicalDevice::DeLink( CLogicalDevice **head, CLogicalDevice **tail )
{
    const char method_name[] = "CLogicalDevice::DeLink";
    TRACE_ENTRY;
    if (*head == this)
        *head = Next;
    if (*tail == this)
        *tail = Prev;
    if (Prev)
        Prev->Next = Next;
    if (Next)
        Next->Prev = Prev;
    TRACE_EXIT;
}

CLogicalDevice *CLogicalDevice::GetNext( void )
{
    const char method_name[] = "CLogicalDevice::GetNext";
    TRACE_ENTRY;
    TRACE_EXIT;
    return Next;
}

CLogicalDevice *CLogicalDevice::Link( CLogicalDevice *entry )
{
    const char method_name[] = "CLogicalDevice::Link";
    TRACE_ENTRY;
    Next = entry;
    entry->Prev = this;
    TRACE_EXIT;
    return entry;
}

bool CLogicalDevice::MirrorMounted( void )
{
    bool status;
    const char method_name[] = "CLogicalDevice::MirrorMounted";
    TRACE_ENTRY;

    if (Mirror && Mirror->Mounted)
    {
        status = true;
    }
    else
    {
        status = false;
    }

    if (trace_settings & TRACE_REQUEST)
        trace_printf("%s@%d" " - Mirror Device " "%s"  " is "  "%s"  "\n", method_name, __LINE__, name_.c_str(), (status?"mounted":"not mounted"));

    TRACE_EXIT;
    return status;
}

bool CLogicalDevice::Mount( CProcess *process, bool replicate )
{
    bool rstate = false;
    bool pstate = false;
    bool mstate = false;
    const char *device, *devicem;
    int  zone;

    const char method_name[] = "CLogicalDevice::Mount";
    TRACE_ENTRY;

    zone = Nodes->GetLNode( process->GetNid() )->GetZone();
    if ( !(( primaryZid_ == zone ) || ( backupZid_  == zone )) )
    {
        if (trace_settings & TRACE_REQUEST)
            trace_printf("%s@%d - Device %s not associated with process %s (%d, %d)'s fault zones, ZONE=%d, ZONE-B=%d\n", method_name, __LINE__, name_.c_str(), process->GetName(), process->GetNid(), process->GetPid(), primaryZid_, backupZid_ );
        return false;
    }

    device = Primary ? Primary->GetName() : NULL;
    devicem = Mirror ? Mirror->GetName() : NULL;

    if (Primary)
    {
       pstate = Primary->Mount(process, device);
    }

    if (Mirror)
    {
       mstate = Mirror->Mount(process, devicem);
    }

    rstate = (pstate || mstate) ? true : false;

    if ( rstate && replicate )
    {
        if ( NameServerEnabled )
        {
            int rc = -1;

#if 0
            rc = PtpClient->ProcessDevice(this); // TODO: in future
#endif
            if (rc)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                snprintf( la_buf, sizeof(la_buf)
                        , "[%s] - Logical device mount request not supported: "
                          "device %s (%d, %d): "
                          "zone=%d, primaryZid_=%d, backupZid_=%d\n"
                        , method_name
                        , process->GetName()
                        , process->GetNid()
                        , process->GetPid()
                        , zone
                        , primaryZid_
                        , backupZid_  );
                mon_log_write(MON_LDEVICE_MOUNT_1, SQ_LOG_ERR, la_buf);
            }
        }
        else
        {
            // Replicate the mount to other nodes
            CReplDevice *repl = new CReplDevice(this);
            Replicator.addItem(repl);
        }
    }

    TRACE_EXIT;
    return rstate;
}

bool CLogicalDevice::Mounted( void )
{
    bool status;

    const char method_name[] = "CLogicalDevice::Mounted";
    TRACE_ENTRY;
    if (Primary && Primary->Mounted)
    {
        status = true;
    }
    else if (Mirror && Mirror->Mounted)
    {
        status = true;
    }
    else
    {
        status = false;
    }
    if (trace_settings & TRACE_REQUEST)
        trace_printf("%s@%d" " - Device " "%s"  " is "  "%s"  "\n", method_name, __LINE__, name_.c_str(), (status?"mounted":"not mounted"));

    TRACE_EXIT;
    return status;
}

bool CLogicalDevice::PrimaryMounted( void )
{
    bool status;
    const char method_name[] = "CLogicalDevice::PrimaryMounted";
    TRACE_ENTRY;

    if (Primary && Primary->Mounted)
    {
        status = true;
    }
    else
    {
        status = false;
    }

    if (trace_settings & TRACE_REQUEST)
        trace_printf("%s@%d" " - Primary Device " "%s"  " is "  "%s"  "\n", method_name, __LINE__, name_.c_str(), (status?"mounted":"not mounted"));

    TRACE_EXIT;
    return status;
}

bool CLogicalDevice::UnMount( bool replicate )
{
    bool bstate = true;
    bool pstate = false;
    bool rstate;

    const char method_name[] = "CLogicalDevice::UnMount";
    TRACE_ENTRY;
    if (Primary)
    {
        pstate = Primary->UnMount ();
    }
    if (Mirror)
    {
        bstate = Mirror->UnMount();
    }
    rstate = (pstate && bstate);
    if ( replicate )
    {
        if ( NameServerEnabled )
        {
            int rc = -1;
            CProcess *process = NULL;
            Nodes->GetNode( (char *) GetName(), &process );

#if 0
            rc = PtpClient->ProcessDevice(this); // TODO: in future
#endif
            if (rc)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                snprintf( la_buf, sizeof(la_buf)
                        , "[%s] - Logical device unmount request not supported: "
                          "device %s (%d, %d), "
                          "primary=%d, mirror=%d\n"
                        , method_name
                        , process?process->GetName():""
                        , process?process->GetNid():-1
                        , process?process->GetPid():-1
                        , pstate
                        , bstate  );
                mon_log_write(MON_LDEVICE_UNMOUNT_1, SQ_LOG_ERR, la_buf);
            }
        }
        else
        {
            // Replicate the mount to other nodes
            CReplDevice *repl = new CReplDevice(this);
            Replicator.addItem(repl);
        }
    }

    TRACE_EXIT;
    return rstate;
}

CDeviceContainer::CDeviceContainer( void )
                 : Head (NULL),
                   DeviceHead (NULL),
                   DeviceTail (NULL),
                   Tail (NULL),
                   initialized_ ( true )
{

    const char method_name[] = "CDeviceContainer::CDeviceContainer";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "DCTR", 4);


    TRACE_EXIT;
}

CDeviceContainer::~CDeviceContainer( void )
{
    CDevice *device = DeviceHead;
    CLogicalDevice *ldev = Head;

    const char method_name[] = "CDeviceContainer::~CDeviceContainer";
    TRACE_ENTRY;
    while (device)
    {
        device->DeLink (&DeviceHead, &DeviceTail);
        delete device;

        device = DeviceHead;
    }
    while (ldev)
    {
        ldev->DeLink (&Head, &Tail);
        delete ldev;

        ldev = Head;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "dctr", 4);

    TRACE_EXIT;
}

CLogicalDevice *CDeviceContainer::CloneDevice( CProcess *process )
{
    CLogicalDevice *ldev;

    const char method_name[] = "CDeviceContainer::CloneDevice";
    TRACE_ENTRY;
    ldev = ConfigDevice (process);
    TRACE_EXIT;

    return ldev;
}

CLogicalDevice *CDeviceContainer::ConfigDevice( CProcess *process )
{
    char keyname[MAX_KEY_NAME];
    int primaryZid;
    int backupZid;
    CConfigGroup *group;
    CConfigKey *key;
    CDevice *primary = NULL;
    CDevice *mirror = NULL;
    CLogicalDevice *ldev = NULL;

    const char method_name[] = "CDeviceContainer::ConfigDevice";
    TRACE_ENTRY;
 
    group = Config->GetGroup(process->GetName());
    if (group)
    {
        // get primary device configuration
        strcpy(keyname,"ZONE");
        key = group->GetKey(keyname);
        if (key)
        {
            primaryZid = atoi(key->GetValue());
            int process_zone_id = Nodes->GetLNode( process->GetNid() )->GetZone();
            if ( primaryZid != Nodes->GetLNode( process->GetNid() )->GetZone() )
            {
                char buf[MON_STRING_BUF_SIZE];
                sprintf(buf, "[%s] Process ZONE=%d doesn't agree with configured ZONE=%d %s\n",
                        method_name, primaryZid, process_zone_id, process->GetName());
                mon_log_write(MON_DEVICE_CONFIG_1, SQ_LOG_WARNING, buf);
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s] Can't find ZONE registry key for device %s\n",
                    method_name, process->GetName());
            mon_log_write(MON_DEVICE_CONFIG_2, SQ_LOG_WARNING, buf);

            primaryZid = 0;
        }
        strcpy(keyname,"DEVICE");
        key = group->GetKey(keyname);
        if (key)
        { 
            primary = new CDevice(key->GetValue());
            if (DeviceHead == NULL)
            {
                DeviceHead = DeviceTail = primary;
            }
            else
            {
                DeviceTail = DeviceTail->Link (primary);
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s] Can't find DEVICE registry key for device %s\n",
                    method_name, process->GetName());
            mon_log_write(MON_DEVICE_CONFIG_3, SQ_LOG_WARNING, buf);
        }
        
        // get backup logical device configuration
        strcpy(keyname,"ZONE-B");
        key = group->GetKey(keyname);
        if (key)
        {
            backupZid = atoi(key->GetValue());
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s] Can't find ZONE-B registry key for device %s\n",
                    method_name, process->GetName());
            mon_log_write(MON_DEVICE_CONFIG_4, SQ_LOG_WARNING, buf);

            backupZid = -1;
        }
        strcpy(keyname,"DEVICE-M");
        key = group->GetKey(keyname);
        if (key)
        { 
            mirror = new CDevice(key->GetValue());
            if (DeviceHead == NULL)
            {
                DeviceHead = DeviceTail = mirror;
            }
            else
            {
                DeviceTail = DeviceTail->Link (mirror);
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s] Can't find DEVICE-M registry key for device %s\n",
                    method_name, process->GetName());
            mon_log_write(MON_DEVICE_CONFIG_5, SQ_LOG_WARNING, buf);
        }

        if ( primary )
        {
            ldev = new CLogicalDevice (process->GetName(), primary, mirror, primaryZid, backupZid);
            if (Head == NULL)
            {
                Head = Tail = ldev;
            }
            else
            {
                Tail = Tail->Link (ldev);
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s] Can't create LocalDevice %s\n",
                    method_name, process->GetName());
            mon_log_write(MON_DEVICE_CONFIG_6, SQ_LOG_WARNING, buf);
        }
    }

    TRACE_EXIT;
    return ldev;
}

CLogicalDevice *CDeviceContainer::CreateDevice( CProcess *process )
{
    CLogicalDevice *ldev;

    const char method_name[] = "CDeviceContainer::CreateDevice";
    TRACE_ENTRY;
    ldev = GetLogicalDevice (process->GetName());
    if (!ldev)
    {
        ldev = ConfigDevice (process);
        if (ldev)
        {
            if ( NameServerEnabled )
            {
                int rc = -1;
                CProcess *process = NULL;
                Nodes->GetNode( (char *) ldev->GetName(), &process );
    
#if 0
                rc = PtpClient->ProcessDevice(this); // TODO: in future
#endif
                if (rc)
                {
                    char la_buf[MON_STRING_BUF_SIZE];
                    snprintf( la_buf, sizeof(la_buf)
                            , "[%s] - Create logical device request not supported: "
                              "device %s (%d, %d)\n"
                            , method_name
                            , process?process->GetName():""
                            , process?process->GetNid():-1
                            , process?process->GetPid():-1 );
                    mon_log_write(MON_LDEVICE_CREATEDEVICE_1, SQ_LOG_ERR, la_buf);
                }
            }
            else
            {
                // Replicate the device to other nodes
                CReplDevice *repl = new CReplDevice(ldev);
                Replicator.addItem(repl);
            }
        }
        else
        {
            if (trace_settings & (TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL))
               trace_printf("%s@%d" " - No configuration for device " "%s" "\n", method_name, __LINE__, process->GetName());
        }
    }
    else
    {
        if (trace_settings & (TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL))
            trace_printf("%s@%d" " - Logical Device " "%s" " has already been created" "\n", method_name, __LINE__, ldev->name());
    }
    TRACE_EXIT;

    return ldev;
}

bool CDeviceContainer::MountVolume( const char *name, CProcess *process, CLogicalDevice **ldevice )
{
    bool rstate = false;
    CLogicalDevice *ldev;

    const char method_name[] = "CDeviceContainer::MountVolume";
    TRACE_ENTRY;
    ldev = GetLogicalDevice(name);
    if ( ldev )
    {
        if ( ldev->Mounted() )
        {
            process->MakePrimary();
            rstate = ldev->UnMount();
            if (!rstate)
            {
                if (trace_settings & TRACE_REQUEST)
                   trace_printf("%s@%d - Can't unmount device %s for process %s (%d, %d)\n", method_name, __LINE__, ldev->name(), process->GetName(), process->GetNid(), process->GetPid());
                return rstate;
            }
        }
        Monitor->CompleteSyncCycle();
        rstate = ldev->Mount(process);
        if (!rstate)
        {
            if (trace_settings & TRACE_REQUEST)
               trace_printf("%s@%d - Can't mount device %s for process %s (%d, %d)\n", method_name, __LINE__, ldev->name(), process->GetName(), process->GetNid(), process->GetPid());
        }
        else
        {
            if (trace_settings & TRACE_REQUEST)
               trace_printf("%s@%d - Mounted device %s for process %s (%d, %d)\n", method_name, __LINE__, ldev->name(), process->GetName(), process->GetNid(), process->GetPid());
        }
    }
    *ldevice = ldev;

    TRACE_EXIT;
    return rstate;
}

bool CDeviceContainer::UnMountVolume( const char *name, bool isBackup )
{
    bool rstate = false;
    CLogicalDevice *ldev;

    const char method_name[] = "CDeviceContainer::UnMountVolume";
    TRACE_ENTRY;
    ldev = GetLogicalDevice( name );
    if ( ldev )
    {
        if ( ldev->Mounted() )
        {
            if ( !isBackup )
            {
                rstate = ldev->UnMount();
                if (!rstate)
                {
                    if (trace_settings & TRACE_REQUEST)
                       trace_printf("%s@%d - Can't unmount device %s for process %s\n", method_name, __LINE__, ldev->name(), name);
                }
                else
                {
                    if (trace_settings & TRACE_REQUEST)
                       trace_printf("%s@%d - UnMounted device %s for process %s\n", method_name, __LINE__, ldev->name(), name);
                }
            }
        }
    }

    TRACE_EXIT;
    return rstate;
}

CLogicalDevice *CDeviceContainer::GetLogicalDevice( const char *name )
{
    CLogicalDevice *ldev = Head;

    const char method_name[] = "CDeviceContainer::GetLogicalDevice";
    TRACE_ENTRY;
    while (ldev)
    {
        if ( strcmp( ldev->name(), name ) == 0 )
        {
            break;
        }
        ldev = ldev->GetNext();
    }

    TRACE_EXIT;
    return ldev;
}
