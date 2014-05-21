// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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

#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdarg.h>

#include "sp_registry.h"
#include "sp_common.h"

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"

#include "SCMVersHelp.h"

int gvMyNid = -1;
int gvMyPid = -1;
char gvMyName[MS_MON_MAX_PROCESS_NAME];

DEFINE_EXTERN_COMP_DOVERS(sp_wrapper)

// ---------------------------------------------------------------------------
//
// execGeneric_script - launch a generic script, variable arguments
//
//----------------------------------------------------------------------------
void execGeneric_script (int pv_argc, char **pv_argv)
{
    char  *la_argv[SP_MAX_PROG_ARGS];  // can't allocate this whole array, exec won't work - irritating
    char   la_buf[MS_MON_MAX_PROCESS_PATH];
    char   la_script_full[MS_MON_MAX_PROCESS_PATH];
    int    lv_error = 0;
    char  *lp_final;
    char  *lp_temp = NULL;

   if (((pv_argc - SP_PROXY_ARGS) >= SP_MAX_PROG_ARGS) || 
       (pv_argc < 1))
   {
       LOG_AND_TRACE (1,("execGeneric_script (%s): too many or not enough program arguments (%d)\n", 
                       gvMyName, pv_argc));
       return;
   }

   for (int i = SP_PROXY_ARGS; i < pv_argc; i++)
   {
       // skip over the proxy args
       lp_temp = pv_argv[i];
       lp_final = new char[strlen(lp_temp) + 1];
       sprintf(lp_final, "%s", lp_temp);
       la_argv[i-SP_PROXY_ARGS] = lp_final;
   }

   // we need a null as the never last entry
   la_argv[pv_argc-SP_PROXY_ARGS] = NULL;

/*
    // for debugging
    for (int i = 0; i < pv_argc-SP_PROXY_ARGS; i++)
        LOG_AND_TRACE (2,("execGeneric_script (%s): arg[%d] : %s\n", gvMyName, i, la_argv[i]));
*/

    if (!sp_find_exec (la_argv[0], la_script_full))
       return;

    la_argv[0] = la_script_full;

    sprintf(la_buf, "%s,%d", gvMyName, gvMyNid); // name, node

    if (!lv_error)
       execv (la_argv[0], la_argv);

    // should never get here
    LOG_AND_TRACE (1,("\n execv failed for %s\n", gvMyName));
    return;

}

// ---------------------------------------------------------------------------
//
// execGeneric_exe - launch a generic executable, variable arguments
//
//----------------------------------------------------------------------------
void execGeneric_exe ( int pv_argc, char **pv_argv)
{
    char  *la_argv[SP_MAX_PROG_ARGS];
    char   la_buf[MS_MON_MAX_PROCESS_PATH];
    char   la_prog[MS_MON_MAX_PROCESS_PATH];
    int    lv_error = 0;
    char  *lp_final;
    char  *lp_temp = NULL;

   if (((pv_argc - SP_PROXY_ARGS) >= SP_MAX_PROG_ARGS) || 
       (pv_argc < 1))
   {
       LOG_AND_TRACE (1,("execGeneric_exe (%s): too many or not enough program arguments (%d)\n", gvMyName, pv_argc));
       return;
   }

   for (int i = SP_PROXY_ARGS; i < pv_argc; i++)
   {
       // skip over the proxy args
       lp_temp = pv_argv[i];
       if (( i == (pv_argc - 1)) && (i != SP_PROXY_ARGS/*exe name */))
       {
           lp_final = new char[strlen(lp_temp) + 3];
           // qpid processes will not work without these spaces - another irritation 
           // doesn't hurt normal programs
           sprintf(lp_final, " %s ", lp_temp);   
       }
       else
       {
           lp_final = new char[strlen(lp_temp) + 1];
           sprintf(lp_final, "%s", lp_temp);
       }

       la_argv[i-SP_PROXY_ARGS] = lp_final;
   }

   // last arg needs to be a null
   la_argv[pv_argc-SP_PROXY_ARGS] = NULL;

/*
    // for debugging
    for (int i = 0; i < pv_argc-SP_PROXY_ARGS; i++)
       LOG_AND_TRACE (1,("execGeneric_exe (%s): arg[%d] :_%s_\n", gvMyName, i, la_argv[i]));

*/
    if (!sp_find_exec (la_argv[0], la_prog))
       return;


    sprintf(la_buf, "%s,%d", gvMyName, gvMyNid); // name, node

    if (atoi(pv_argv[1]) == SP_BROKER)
    {
        char la_buf2[1024];
        memset(la_buf2, 0, 1024);
        sprintf(la_buf2, "%s_%s_%d", SP_PROCESS_UP, gvMyName, gvMyNid);
        lv_error = sp_reg_set(MS_Mon_ConfigType_Cluster, CLUSTER_GROUP,
                          la_buf2, la_buf);

    }
    if (!lv_error)
       execv (la_prog, la_argv);

    // should never get here
    LOG_AND_TRACE (1, ("\n execv failed for %s\n", gvMyName));
    return;
}

// -----------------------------------------------------------
//
// sp_initialize - get setup information and exec into what 
//                 exe we need to be
//
// -----------------------------------------------------------
void sp_initialize (int pv_argc, char *pa_argv[])
{
     MS_Mon_Process_Info_Type lv_info;
     int lv_type = atoi(pa_argv[1]);
     int lv_error = msg_mon_get_process_info_detail(NULL, &lv_info);

     if (lv_error)
        return;

     gvMyNid = lv_info.nid;
     gvMyPid = lv_info.pid;
     sprintf(gvMyName, "%s",lv_info.process_name);

     switch (lv_type)
     {
     case SP_BROKER:
     case SP_METRICS:
     case SP_GENERIC_EXE:
     {
         execGeneric_exe(pv_argc, pa_argv);
         break;
     }
     case SP_TPA_SCRIPTS:
     case SP_HARNESS:
     case SP_GENERIC_SCRIPT:
     {
         execGeneric_script(pv_argc, pa_argv);
         break;
     }
     default:
     {
        LOG_AND_TRACE (1,("\nWrapper : Unknown process type(%d) for %s\n", lv_type, gvMyName));
        break;
     }
     }

     LOG_AND_TRACE (1,("SP_Process::sp_initialize  (%s) EXIT (ERROR)\n", gvMyName));
    // should never get here, but aborts are bad  
     return; 
}

// ----------------------------------------------------------------------------
//
// main - simpy attach to the seaquest env and initialize
//
// ----------------------------------------------------------------------------
int main(int pv_argc, char *pa_argv[]) 
{
    CALL_COMP_DOVERS(sp_wrapper, pv_argc, pa_argv);
    int error = msg_init_attach (&pv_argc, &pa_argv, false, NULL);
    if (!error)
    {
        error = msg_mon_process_startup2(false, false); // client
        sp_initialize(pv_argc, pa_argv );
    }
    msg_mon_process_shutdown();
}
