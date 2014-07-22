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
#ifndef __CONFIG_H
#define __CONFIG_H


#ifndef WIN32 // TRW
#include <syslog.h>
#else
#include "../wrapper_win/pthread_win.h"
#endif

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/text_format.h>

#include "wrapper/routingkey.h"

#ifdef SEAQUEST_PROCESS
#include "seabed/ms.h"
#endif

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

using std::stringstream;
using std::string;

#define MAX_RETRIES   3
#define MAX_CONFIG_RETRIES 20
#define MAX_WAIT      1000000
#define MAX_RAND      5
#define MAX_SP_BUFFER 256

class spConfig
{

   public:
    spConfig();
    ~spConfig();

   bool get_activeConfig() {return activeConfig;}
   bool get_spDisabled() {return spDisabled;}
   unsigned int get_instanceId() {return instanceId;}
   const char *get_iPAddress() {return iPAddress;}
   const char *get_mode() {return mode.data();}
   std::string get_sIpaddress();
   int get_portNumber() {return portNumber;}
   unsigned int get_host_id() {return host_id;}

   void set_portNumber(int pn) {portNumber = pn;}
   void set_spDisabled(bool sd) {spDisabled = sd;}
   void set_activeConfig(bool ac) {activeConfig = ac;}
   void set_iPAddress(const char *ip) { strcpy (iPAddress, ip);}
   void set_mode(const char *md) { mode=md;}

#ifdef SEAQUEST_PROCESS
   int get_nodeId() {return nodeId;}
   int get_pnodeId() {return pnodeId;}
   char *get_processName() {return processName;}
   void set_pnodeId(bool pnid) {pnodeId = pnid;}

    int                       getSeaquestConfig();
#endif

   private:
    pthread_mutex_t mutex;
    // configuration related
#ifdef SEAQUEST_PROCESS
  //  int                       retries;
    int                       nodeId;
    int                       pnodeId;
    char                      processName[MS_MON_MAX_PROCESS_NAME+1]; // +1 for trailing null
#endif

    unsigned int              instanceId;
    bool                      activeConfig;
    char                      iPAddress[MAX_SP_BUFFER];
    std::string               sIpaddress;
    unsigned int              host_id;
    int                       portNumber;
    bool                      spDisabled; 
    std::string               mode;

    public:
    //private helper methods
    int getConfig();

    // thread safe getter methods
    bool isActiveConfig();
};
#endif
