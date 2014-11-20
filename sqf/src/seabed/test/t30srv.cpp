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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"


#include "t25.h"

// global variables
int       gargc;
char    **gargp;
char      gdisplay_name[MS_MON_MAX_PROCESS_NAME];
bool      gim_backup = false;
char      gname[MS_MON_MAX_PROCESS_NAME];
int       gopen_count = 0;
int       gpeer_nid = -1;
int       gpeer_oid;
TPT_DECL (gpeer_phandle);
int       gpeer_pid = -1;
char      gprog[MS_MON_MAX_PROCESS_PATH];
MS_SRE    gsre;
bool      gtakeover = false;

// forwards
static int  recv(void *buf, int *size);
static bool takeover();

#include "t25.cpp.h"

void check_mon_msg(char *mon_msg) {
    MS_Mon_Msg *msg = (MS_Mon_Msg *) mon_msg;

    switch (msg->type) {
    case MS_MsgType_Close:    // process close notification
        myprintf("Close notice received from %s.\n",
                 msg->u.close.process_name);
        if (strcmp(gname, msg->u.close.process_name) != 0) {
            gopen_count--;
            myprintf("Disconnecting from client\n");
        }
        break;
    case MS_MsgType_NodeDown: // node is down notification
    case MS_MsgType_NodeUp:   // node is up notification
        myprintf("Node Up/Down notices not currently supported.\n");
        break;
    case MS_MsgType_Open:     // process open notification
        myprintf("Open notice received from %s.\n",
                 msg->u.open.target_process_name);
        if (strcmp(gname, msg->u.open.target_process_name) == 0) {
            gpeer_nid = msg->u.open.nid;
            gpeer_pid = msg->u.open.pid;
            myprintf("peer p-id=%d/%d\n", gpeer_nid, gpeer_pid);
        }
        break;
    case MS_MsgType_ProcessDeath: // process death notification
        myprintf("ProcessDeath notice received from p-id=%d/%d, peer-p-id=%d/%d.\n",
                msg->u.death.nid, msg->u.death.pid, gpeer_nid, gpeer_pid);
        if ((gpeer_nid == msg->u.death.nid) &&
            (gpeer_pid == msg->u.death.pid))
            gtakeover = takeover();
        break;
    default:
        myprintf("Invalid notice message type received.\n");
    }
}

int checkpoint(void *buf, int *size) {
    int ferr;

    if (gim_backup) {
        myprintf("Waiting for checkpoint.\n");
        ferr = recv(buf, size);
    } else {
        int            msgid;
        MS_Result_Type results;
        myprintf("Sending checkpoint.\n");
        ferr = XMSG_LINK_(TPT_REF(gpeer_phandle),               // phandle
                          &msgid,                      // msgid
                          NULL,                        // reqctrl
                          0,                           // reqctrlsize
                          NULL,                        // replyctrl
                          0,                           // replyctrlmax
                          (char *) buf,                // reqdata
                          (ushort) *size,              // reqdatasize
                          NULL,                        // replydata
                          0,                           // replydatamax
                          0,                           // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          0);                          // linkopts
        TEST_CHK_FEOK(ferr);
        // ignore checkpoint error
        ferr = XMSG_BREAK_(msgid, (short *) &results, TPT_REF(gpeer_phandle));
        TEST_CHK_FEIGNORE(ferr);
    }
    return ferr;
}

bool is_backup() {
    MS_Mon_Process_Info_Type proc_info;

    int ferr = msg_mon_get_process_info_detail(gname, &proc_info);
    TEST_CHK_FEOK(ferr);
    bool backup = proc_info.backup ? true : false;
    return backup;
}

void mysleep(int ms) {
    struct timespec t;

    t.tv_sec = 0;
    t.tv_nsec = ms * 1000 * // us
                     1000;  // ns
    nanosleep(&t, NULL);
}

bool process_request(int *recvbuf) {
    bool        abort = false;
    const char *cmd;
    bool        done = false;
    char        sendbuf[100];
    int         sendlen;

    switch (recvbuf[2]) {
    case CMD_CONT:
        cmd = "CMD_CONT";
        break;
    case CMD_END:
        cmd = "CMD_END";
        done = true;
        break;
    case CMD_ABORT:
        cmd = "CMD_ABORT";
        abort = true;
        break;
    default:
        cmd = "UNKNOWN";
    }
    sprintf(sendbuf, "[%s] Received (%d:%d) %s", gdisplay_name,
            recvbuf[0], recvbuf[1], cmd);
    if (gim_backup) {
        myprintf("processing checkpointed data\n");
        sendlen = 0;
    } else {
        myprintf("processed client request\n");
        sendlen = (int) strlen(sendbuf) + 1; // cast
    }
    myprintf("sending reply\n");
    XMSG_REPLY_(gsre.sre_msgId,    // msgid
                NULL,              // replyctrl
                0,                 // replyctrlsize
                sendbuf,           // replydata
                (ushort) sendlen,  // replydatasize
                0,                 // errorclass
                NULL);             // newphandle

    if (abort) {
        myprintf("Exiting\n");
        msg_mon_process_shutdown_now();
        exit(0);
    }
    return done;
}

int recv(void *buf, int *size) {
    int   ferr;
    int  *lbuf;
    int   lerr;
    bool  mon_msg;
    int   nid;
    int   pid;

    do {
        do {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            lerr = XMSG_LISTEN_((short *) &gsre,   // sre
                                0,                 // listenopts
                                0);                // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        mon_msg = (gsre.sre_flags & XSRE_MON);
        if (mon_msg) {
            int msg_size = gsre.sre_reqDataSize;
            char *msg = new char[msg_size];
            ferr = XMSG_READDATA_(gsre.sre_msgId,     // msgid
                                  msg,                // reqdata
                                  (ushort) msg_size); // bytecount
            assert(ferr == XZFIL_ERR_OK);
            check_mon_msg(msg);
            XMSG_REPLY_(gsre.sre_msgId,      // msgid
                        NULL,                // replyctrl
                        0,                   // replyctrlsize
                        NULL,                // replydata
                        0,                   // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
            delete [] msg;
            if (gtakeover)
                return 1;
        } else {
            ferr = XMSG_READDATA_(gsre.sre_msgId,   // msgid
                                  (char *) buf,     // reqctrl
                                  (ushort) *size);  // bytecount
            assert(ferr == XZFIL_ERR_OK);
            *size = gsre.sre_reqDataSize;
            ferr = XMSG_GETREQINFO_(MSGINFO_NID,
                                    gsre.sre_msgId,
                                    &nid);
            assert(ferr == XZFIL_ERR_OK);
            ferr = XMSG_GETREQINFO_(MSGINFO_PID,
                                    gsre.sre_msgId,
                                    &pid);
            assert(ferr == XZFIL_ERR_OK);
            lbuf = (int *) buf;
            myprintf("received from p-id=%d/%d: %d.%d.%d\n",
                     nid, pid, lbuf[0], lbuf[1], lbuf[2]);
        }
    } while (mon_msg);
    return 0;
}

void start_backup(int nid) {
    int ferr;

    nid = nid; // no-warn
    ferr = msg_mon_get_process_info(NULL, &nid, &gpeer_pid);
    TEST_CHK_FEOK(ferr);
    gpeer_nid = 1 - nid;
    gpeer_pid = -1;
    myprintf("starting backup process with open.\n");
    sleep(2); // TODO remove when close timing problem fixed
    ferr = msg_mon_start_process(gprog,                  // prog
                                 gname,                  // name
                                 NULL,                   // ret name
                                 gargc,
                                 gargp,
                                 TPT_REF(gpeer_phandle),
                                 0,                      // open
                                 &gpeer_oid,
                                 MS_ProcessType_Generic, // type
                                 0,                      // priority
                                 0,                      // debug
                                 1,                      // backup
                                 &gpeer_nid,             // nid
                                 &gpeer_pid,             // pid
                                 NULL,                   // infile
                                 NULL);                  // outfile
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_open_process_backup(gname,
                                       TPT_REF(gpeer_phandle),
                                       &gpeer_oid);
    TEST_CHK_FEOK(ferr);
    myprintf("after start_backup - peer p-id=%d/%d\n",
             gpeer_nid, gpeer_pid);
}

bool takeover() {
    bool activate = false;

    if (gim_backup) {
        myprintf("mark primary down, takeover and start backup.\n");
        gpeer_nid = -1;
        gpeer_pid = -1;
        gim_backup = false;
        strcpy(gdisplay_name, gname);
        strcat(gdisplay_name, "-P(B)");
        activate = true;
    } else {
        myprintf("mark backup down and restart backup.\n");
        start_backup(-1);
    }

    return activate;
}

int main(int argc, char *argv[]) {
    bool done = false;
    int  ferr;
    int  recvbuf[3];

    sprintf(gprog, "%s/%s", getenv("PWD"), argv[0]);
    ferr = msfs_util_init_role(false, &argc, &argv, msg_debug_hook);
    assert(ferr == XZFIL_ERR_OK);
    ferr = msg_mon_process_startup(true); // system messages
    assert(ferr == XZFIL_ERR_OK);
    ferr = msg_mon_get_my_process_name(gname, sizeof(gname));
    assert(ferr == XZFIL_ERR_OK);
    msg_mon_enable_mon_messages(true); // get mon messages
    gargc = argc; // after msg_init - it removes args
    gargp = argv;

    if (is_backup()) {
        strcpy(gdisplay_name, gname);
        strcat(gdisplay_name, "-B");
        myprintf("We are the backup process.\n");
        gim_backup = true;
        while (!gtakeover) {
            int len = sizeof(recvbuf);
            ferr = checkpoint(recvbuf, &len);
            if (ferr == XZFIL_ERR_OK)
                process_request(recvbuf);
        }
        myprintf("The backup is now the primary process.\n");
        strcpy(gdisplay_name, gname);
        strcat(gdisplay_name, "-P");
    } else {
        strcpy(gdisplay_name, gname);
        strcat(gdisplay_name, "-P");
        myprintf("We are the primary process.\n");
    }
    start_backup(-1);

    while (!done) {
        int len = sizeof(recvbuf);
        if (!recv(recvbuf, &len)) {
            done = process_request(recvbuf);
            checkpoint(recvbuf, &len);
        }
    }

    ferr = msg_mon_process_close();
    TEST_CHK_FEOK(ferr);

    if (gpeer_nid >= 0) {
        myprintf("stopping backup.\n");
        ferr = msg_mon_stop_process((char*)"", gpeer_nid, gpeer_pid);
        TEST_CHK_FEOK(ferr);
    }
    myprintf("sending exit process message.\n");
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
}
