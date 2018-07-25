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

#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../inc/fs/feerrors.h"
#include "dtm/tm.h"
#include "tmlibmsg.h"
#include "seabed/ms.h"

// General Seaquest includes
#include "SCMVersHelp.h"

// Version
DEFINE_EXTERN_COMP_DOVERS(tmshutdown)

#define MAX_ARGLEN 32
using namespace std;


int main(int argc, char *argv[]) 
{
    short lv_returnCode = FEOK;
    char  la_inputval[MAX_ARGLEN];
    memset(la_inputval, 0, MAX_ARGLEN);

    CALL_COMP_DOVERS(tmshutdown, argc, argv);  

    if(argc == 1) {
        //need to specify input, print a help
        printf("Input must be specified, valid options are:\n");
        printf("    normal\n");
        printf("    immediate\n");
        return 1;
    }
    else if(argc > 2) {
        printf("ERROR: Expecting 1 command line argument, 2 or more supplied\n");
        printf("       exiting..\n");
        return 1;
    }
    strncpy(la_inputval, argv[1], MAX_ARGLEN - 1);
    for(int i = 0; la_inputval[i] != '\0'; i++) {
        la_inputval[i] = tolower(la_inputval[i]);
    }
    if((strcmp(la_inputval, "normal") != 0) && (strcmp(la_inputval, "immediate") != 0)) {
        printf("Invalid input, valid options are:\n");
        printf("    normal\n");
        printf("    immediate\n");
        return 1;
    }
    
    int returnval = 0;
    try {
        returnval = msg_init_attach(&argc, &argv, 0, (char *)"");
    }
    catch (SB_Fatal_Excep &) {
        printf("Error on tmshutdown\n");
        return 1;
    }
   
    if (returnval != 0){
        printf("Error attaching to monitor\n");
        return 1;
    }
    try {
        msg_mon_process_startup(false);
    }
    catch (SB_Fatal_Excep &) {
        printf("Error starting processs in monitor\n");
        return 1;
    }
        
    if(strcmp(la_inputval,"normal") == 0) {
        lv_returnCode = DTM_DISABLETRANSACTIONS(TM_DISABLE_SHUTDOWN_NORMAL);
        if(lv_returnCode != FEOK) {
        printf("** DTM_DISABLETRANSACTIONS returned error %d.\n", lv_returnCode);
        }
    }
    if(strcmp(la_inputval,"immediate") == 0) {
        lv_returnCode = DTM_DISABLETRANSACTIONS(TM_DISABLE_SHUTDOWN_IMMEDIATE);
        if(lv_returnCode != FEOK) {
        printf("** DTM_DISABLETRANSACTIONS returned error %d.\n", lv_returnCode);
        }
    }

    msg_mon_process_shutdown();

    if(lv_returnCode == FEOK) {
        return 0;
    }
    else {
        return lv_returnCode;
    }

}
