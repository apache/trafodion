//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/ms.h"
#include "seabed/fs.h"
#include "seabed/labels.h"

#include "tmsfsutil.h"
#include "tverslib.h"


static const char *msfs_ptype_labels[] = {
    "Undefined",
    "TSE",
    "DTM",
    "ASE",
    "Generic",
    "Watchdog",
    "AMP",
    "Backout",
    "VolumeRecovery",
    "MSOSRVR",
    "SPX",
    "SSMP",
    SB_LABEL_END
};
enum { MSFS_PTYPE_MAX = MS_ProcessType_SSMP };
static SB_Label_Map msfs_ptype_map = {
    MS_ProcessType_Undefined,
    MSFS_PTYPE_MAX,
    "<unknown>",
    msfs_ptype_labels
};

static int msfs_util_init_com(bool            init,
                              int            *argc,
                              char         ***argv,
                              Util_DH_Type    dh,
                              bool            attach,
                              bool            forkexec,
                              char           *name);
static int msfs_util_init_role_com(bool           init,
                                   bool           client,
                                   int           *argc,
                                   char        ***argv,
                                   Util_DH_Type   dh,
                                   bool           attach,
                                   bool           forkexec,
                                   char          *name);


VERS_LIB(libsbztmsfsutil)

void msfs_util_event_send(int event_id, bool wait) {
    int  event_len;
    char event_data[MS_MON_MAX_SYNC_DATA];
    int  ferr;

    ferr = msg_mon_event_send(-1,                         // nid
                              -1,                         // pid
                              MS_ProcessType_Undefined,   // process-type
                              event_id,                   // event-id
                              0,                          // event-len
                              NULL);                      // event-data
    assert(ferr == XZFIL_ERR_OK);
    if (wait) {
        ferr = msg_mon_event_wait(event_id, &event_len, event_data);
        assert(ferr == XZFIL_ERR_OK);
    }
}

void msfs_util_event_wait(int event_id) {
    int  event_len;
    char event_data[MS_MON_MAX_SYNC_DATA];
    int  ferr;

    ferr = msg_mon_event_wait(event_id, &event_len, event_data);
    assert(ferr == XZFIL_ERR_OK);
}

void msfs_util_event_wait2(int *event_id) {
    int  event_len;
    char event_data[MS_MON_MAX_SYNC_DATA];
    int  ferr;

    ferr = msg_mon_event_wait2(event_id, &event_len, event_data);
    assert(ferr == XZFIL_ERR_OK);
}

const char *msfs_util_get_sysmsg_str(int sysmsg) {
    const char *str;

    switch (sysmsg) {
    case XZSYS_VAL_SMSG_CPUDOWN:
        str = "CPUDOWN";
        break;
    case XZSYS_VAL_SMSG_CPUUP:
        str = "CPUUP";
        break;
    case XZSYS_VAL_SMSG_TIMESIGNAL:
        str = "TIMESIGNAL";
        break;
    case XZSYS_VAL_SMSG_PROCDEATH:
        str = "PROCDEATH";
        break;
    case XZSYS_VAL_SMSG_OPEN:
        str = "OPEN";
        break;
    case XZSYS_VAL_SMSG_CLOSE:
        str = "CLOSE";
        break;
    case XZSYS_VAL_SMSG_UNKNOWN:
        str = "UNKNOWN";
        break;
    case XZSYS_VAL_SMSG_CHANGE:
        str = "CHANGE";
        break;
    case XZSYS_VAL_SMSG_SHUTDOWN:
        str = "SHUTDOWN";
        break;
    default:
        str = "?";
        break;
    }
    return str;
}
int msfs_util_init(int *argc, char ***argv, Util_DH_Type dh) {
    return msfs_util_init_com(1, argc, argv, dh, false, false, NULL);
}

int msfs_util_init_attach(int            *argc,
                          char         ***argv,
                          Util_DH_Type    dh,
                          bool            forkexec,
                          char           *name) {
    return msfs_util_init_com(true, argc, argv, dh, true, forkexec, name);
}

int msfs_util_init_com(bool            init,
                       int            *argc,
                       char         ***argv,
                       Util_DH_Type    dh,
                       bool            attach,
                       bool            forkexec,
                       char           *name) {
    int   arg;
    char *argp;
    bool  client = false;

    for (arg = 1; arg < *argc; arg++) {
        argp = (*argv)[arg];
        if (strcmp(argp, "-client") == 0)
            client = 1;
    }
    return msfs_util_init_role_com(init,
                                   client,
                                   argc,
                                   argv,
                                   dh,
                                   attach,
                                   forkexec,
                                   name);
}

int msfs_util_init_fs(int *argc, char ***argv, Util_DH_Type dh) {
    return msfs_util_init_com(false, argc, argv, dh, false, false, NULL);
}

int msfs_util_init_role(bool            client,
                        int            *argc,
                        char         ***argv,
                        Util_DH_Type    dh) {
    return msfs_util_init_role_com(true,
                                   client,
                                   argc,
                                   argv,
                                   dh,
                                   false,
                                   false,
                                   NULL);
}

int msfs_util_init_role_com(bool            init,
                            bool            client,
                            int            *argc,
                            char         ***argv,
                            Util_DH_Type    dh,
                            bool            attach,
                            bool            forkexec,
                            char           *name) {
    int ret;

    if (init) {
        if (attach)
            ret = msg_init_attach(argc, argv, forkexec, name);
        else
            ret = msg_init(argc, argv);
    } else
        ret = 0;

    if (client) {
        if (getenv("DEBUGC") != NULL)
            dh("c", "c");
    } else {
        if (getenv("DEBUGS") != NULL)
            dh("s", "s");
    }

    return ret;
}

int msfs_util_init_role_fs(bool            client,
                           int            *argc,
                           char         ***argv,
                           Util_DH_Type    dh) {
    return msfs_util_init_role_com(false,
                                   client,
                                   argc,
                                   argv,
                                   dh,
                                   false,
                                   false,
                                   NULL);
}

//
// wait until at least 'count' processes of 'type' exist
// if retcount is not NULL, returned process count returned
// if verbose then print extra process-wait information
//
int msfs_util_wait_process_count(int  type,
                                 int  count,
                                 int *retcount,
                                 bool verbose) {
    enum       { RETRIES = 10 };
    enum       { SLEEP   = 500000 }; // 500 ms
    int          ferr;
    char        *hook;
    bool         hook_enable;
    int          inx;
    int          lretcount;
    int          pid;
    int          ret;
    int          retries;
    const char  *strtype;
    int          tmpcount;
    int          upcount;

    retries = 0;
    pid = getpid();
    hook = getenv("HOOK_ENABLE");
    if ((hook != NULL) && atoi(hook))
        hook_enable = true;
    else
        hook_enable = false;
    for (;;) {
        ret = msg_mon_get_process_info_type(type,
                                            &lretcount,
                                            0, // max
                                            NULL); // info
        if (verbose) {
            printf("pid=%d: type=%d, count=%d, retcount=%d, ret=%d\n",
                   pid, type, count, lretcount, ret);
        }
        MS_Mon_Process_Info_Type *pia =
          new MS_Mon_Process_Info_Type[lretcount];
        ferr = msg_mon_get_process_info_type(type,
                                             &tmpcount,
                                             lretcount, // max
                                             pia); // info
        if (ferr != XZFIL_ERR_OK)
            tmpcount = 0;
        upcount = 0;
        for (inx = 0; inx < tmpcount; inx++) {
            if (pia[inx].state == MS_Mon_State_Up)
                upcount++;
            if (verbose) {
                printf("pid=%d: p-id=%d/%d, type=%d, name=%s\n",
                       pid,
                       pia[inx].nid, pia[inx].pid,
                       pia[inx].type, pia[inx].process_name);
            }
        }
        delete [] pia;
        if (ret != XZFIL_ERR_OK)
            break;
        if (upcount >= count)
            break;
        if (!hook_enable)
            retries++;
        if (retries >= RETRIES) {
            strtype = SB_get_label(&msfs_ptype_map, type);
            printf("pid=%d: expecting %d processes of type %s (%d), but only %d processes are up\n",
                   pid, count, strtype, type, lretcount);
            ret = XZFIL_ERR_BADCOUNT;
            break;
        }
        usleep(SLEEP);
    }
    if (retcount != NULL)
        *retcount = lretcount;
    return ret;
}
