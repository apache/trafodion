// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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
#ifndef WIN32 // TRW
#include <netdb.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#endif

#ifndef WIN32
#include "common/common.info_header.pb.h"
#else
#include "win/common.info_header.pb.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "wrapper/amqpwrapper.h"
#include "common/evl_sqlog_eventnum.h"

bool sp_addrs(char *pa_buf)
{
    long               *lp_addr;
    unsigned char      *lp_addrp;
    struct hostent     *lp_hostent = NULL;
    struct sockaddr_in  lv_addr;
    int                 lv_inx;
    int                 lv_retries = 0;
    char                la_HostName[MAX_SP_BUFFER];
    int                 lv_error;

    lv_error = gethostname(la_HostName, sizeof(la_HostName));
    if (lv_error)
       return false;

    while ((lp_hostent == NULL) && (lv_retries < 3))
    {
        lp_hostent = gethostbyname(la_HostName);
        lv_retries++;
    }

    if (lp_hostent == NULL)
       return false;

    for (lv_inx = 0;; lv_inx++)
    {
        lp_addr = reinterpret_cast<long *>(lp_hostent->h_addr_list[lv_inx]);
        if (lp_addr == NULL)
           return false;
        if (*lp_addr == 0)
           return false;

        lv_addr.sin_addr.s_addr = static_cast<int>(*lp_addr);
        lp_addrp = reinterpret_cast<unsigned char *>(lp_addr);
        sprintf(pa_buf, "%d.%d.%d.%d",lp_addrp[0], lp_addrp[1], 
                                      lp_addrp[2],lp_addrp[3]);
        return true;
    }
}

// -------------------------------------------------------------------------------
//
// isActiveConfig
// Purpose : A thread safe method to get the value of the activeConfig member
//           variable.
//
// -------------------------------------------------------------------------------
bool spConfig::isActiveConfig()
{
    bool isActiveConfig;

    pthread_mutex_lock( &mutex );
    isActiveConfig = activeConfig;
    pthread_mutex_unlock( &mutex );

    return isActiveConfig;
}
// ------------------------------------------------------------------------
// getConfig
// Purpose : read the configuration from the env.  Will soon change...
//
// ------------------------------------------------------------------------
#ifdef SEAQUEST_PROCESS
int  spConfig::getSeaquestConfig()
{

    int lv_error = msg_mon_get_my_info3(&nodeId, //nid
                                NULL,        // mon process-id
                                processName,       // mon name
                                MS_MON_MAX_PROCESS_NAME,   // mon name-len
                                NULL,      // mon process-type
                                NULL,        // mon zone-id
                                NULL,         // os process-id
                                NULL,          // os thread-id
                                NULL,         // component-id
                                &pnodeId);    // physical node


     if (lv_error)
          return SP_CONFIG_NOT_AVAILABLE;


     // We've observed occasional garbage characters in processName; the
     // code below sanitizes processName until we can find the root cause
     // and fix that. The code below replaces any non-ASCII character with
     // a '?' (we'll assume process names contain only ASCII). This code
     // was put in for Bugzilla case 1476.

     for (size_t i = 0; (i < MS_MON_MAX_PROCESS_NAME) && (processName[i] != '\0'); i++)
     {
         if (processName[i] < 0)  // char is signed on Linux; test if x80 bit is on
         {
             processName[i] = '?';
         }
     }

     return SP_SUCCESS;
}
#endif



// ------------------------------------------------------------------------
// getConfig
// Purpose : read the configuration from the env.  Will soon change...
//
// ------------------------------------------------------------------------
int spConfig::getConfig()
{
     // will be changing depending on config model
     char * spDisabledEnv = getenv("SQ_SEAPILOT_SUSPENDED");
     char   spIpBuf[MAX_SP_BUFFER];
 //    uid_t  spUserId;

     if (spDisabledEnv)
     {
        spDisabled = atoi(spDisabledEnv);
        if (spDisabled)
        {
           // to avoid coming in here every time
           activeConfig = true;
           return SP_SUCCESS;
        }
     }

     if (iPAddress[0] == 0)
     {
         char * iPAddressEnv = getenv("QPID_IP_ADDRESS");
         if(iPAddressEnv)
             strcpy (iPAddress,iPAddressEnv);
         else
             strcpy (iPAddress,"127.0.0.1");
     }

     if (portNumber == -1)
     {
         char * portNumberEnv = getenv("QPID_NODE_PORT");
         if(portNumberEnv)
            portNumber = atoi(portNumberEnv);
         else
            return SP_CONFIG_NOT_AVAILABLE;
     }


    // get ip address
    if (sp_addrs (spIpBuf))
       sIpaddress = spIpBuf;
    else
       sIpaddress = "0.0.0.0"; //return SP_CONFIG_NOT_AVAILABLE;
#ifndef WIN32
    host_id = (unsigned int) gethostid(); // Get unique identifier for the host
#else
	host_id = 0;
#endif

    // apparently this function is always successful....
  //  spUserId = getuid();
  //  instanceId = spUserId;
   
    //Compatibility joins in Repos ineffective if instanceId=userid; setting to 0 until this gets resolved
     instanceId = 0;

    activeConfig = true;
    return SP_SUCCESS;
}

std::string spConfig::get_sIpaddress()
{
    char   spIpBuf[MAX_SP_BUFFER];
    if (sIpaddress == "0.0.0.0")
    {
        if (sp_addrs (spIpBuf))
            sIpaddress = spIpBuf;
    }
    
    return sIpaddress;
}

spConfig::spConfig()
{
    pthread_mutex_init(&mutex, NULL);
    // configuration related
#ifdef SEAQUEST_PROCESS
    nodeId = pnodeId = -1;
    processName[0] = '\0';
#endif

    activeConfig =  false;
    spDisabled = false;
    portNumber = -1;
    iPAddress[0] = 0;
    host_id = 0;
}

spConfig::~spConfig()
{
    pthread_mutex_destroy(&mutex);
}

