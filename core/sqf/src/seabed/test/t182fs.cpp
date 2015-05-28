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
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "seabed/fs.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

#ifdef USE_SB_FC
enum {  MAX_INFO_LIST = 10 };
xzsys_ddl_complete_element_def add_list[MAX_INFO_LIST];
xzsys_ddl_completion_info_def  info;
xzsys_ddl_complete_element_def info_list[MAX_INFO_LIST];
#endif // USE_SB_FC

enum {  MAX_SRV = 2 };
char                           host[MAX_SRV][100];
unsigned short                 port[MAX_SRV];
bool                           verbose = false;


#ifdef USE_SB_FC
void do_set_guardian(short filenum, short set_file, int count) {
    short ferr;
    short info_count;
    short list_inx;

    add_list[0].z_fnum_fd = filenum;
    if (set_file)
        add_list[0].u_z_options.z_options.z_set_file = 1;
    else
        add_list[0].u_z_options.z_options.z_set_file = 0;
    add_list[0].u_z_options.z_options.z_filetype = 0;
    ferr = BFILE_COMPLETE_SET_((short *) add_list,
                               1,
                               &list_inx);
    TEST_CHK_FEOK(ferr);
    ferr = BFILE_COMPLETE_GETINFO_((short *) info_list,
                                   MAX_INFO_LIST,
                                   &info_count);
    TEST_CHK_FEOK(ferr);
    assert(info_count == count);
}

void do_set_linux(int fd, short set_file, int rr, int wr, int exc, int count) {
    short ferr;
    short info_count;
    short list_inx;

    add_list[0].z_fnum_fd = fd;
    if (set_file)
        add_list[0].u_z_options.z_options.z_set_file = 1;
    else
        add_list[0].u_z_options.z_options.z_set_file = 0;
    add_list[0].u_z_options.z_options.z_filetype = 1;
    if (rr)
        add_list[0].u_z_options.z_options.z_read_ready = 1;
    else
        add_list[0].u_z_options.z_options.z_read_ready = 0;
    if (wr)
        add_list[0].u_z_options.z_options.z_write_ready = 1;
    else
        add_list[0].u_z_options.z_options.z_write_ready = 0;
    if (exc)
        add_list[0].u_z_options.z_options.z_exception = 1;
    else
        add_list[0].u_z_options.z_options.z_exception = 0;
    ferr = BFILE_COMPLETE_SET_((short *) add_list,
                               1,
                               &list_inx);
    TEST_CHK_FEOK(ferr);
    ferr = BFILE_COMPLETE_GETINFO_((short *) info_list,
                                   MAX_INFO_LIST,
                                   &info_count);
    TEST_CHK_FEOK(ferr);
    assert(info_count == count);
}

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#endif
int do_accept(int sock) {
    struct sockaddr   c_addr;
    int               socka;

    socklen_t c_addr_len = sizeof(c_addr);
    if (verbose)
        printf("accept attempt to sock=%d\n", sock);
    socka = accept(sock, &c_addr, &c_addr_len);
    if (verbose) {
        if (sock == -1)
            printf("accept completed errno=%d\n", errno);
        else
            printf("accept completed sock=%d\n", socka);
    }
    if (socka == -1) {
        if (errno == EBADF) {
            // socket closed
            if (verbose)
                printf("accept completed errno=%d\n", errno);
            return 0; //
        }
        assert(socka != -1);
    }
    if (verbose)
        printf("accept completed on sock=%d, new sock=%d\n",
               sock, socka);
    return socka;
}

int do_connect(int srv) {
    struct sockaddr_in  addr;
    unsigned char      *addrp;
    int                 err;
    char                hostbuf[500];
    struct hostent      hostent;
    int                 hostenterrno;
    struct hostent     *hostentp;
    int                 err;
    int                 sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (verbose) {
        if (sock == -1)
            printf("creating socket, domain=%d, errno=%d\n", AF_INET, errno);
        else
            printf("creating socket, domain=%d, sock=%d\n", AF_INET, sock);
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port[srv]);
    err = gethostbyname_r(host[srv],
                          &hostent,
                          hostbuf,
                          sizeof(hostbuf),
                          &hostentp,
                          &hostenterrno);
    if (verbose) {
        printf("gethostbyname_r(host[%d]=%s), err=%d\n", srv, host[srv], err);
    }
    assert(err == 0);
    assert(hostentp != NULL);
    assert(*(long *) hostentp->h_addr != -1);
    addr.sin_addr.s_addr = (uint32_t) *(uint32_t *) hostentp->h_addr;
    if (verbose) {
        addrp = (unsigned char *) &addr.sin_addr.s_addr;
        printf("connect attempt to host=%s (%d.%d.%d.%d:%d) (0x%08X:%X), sock=%d\n",
               host[srv],
               addrp[0], addrp[1], addrp[2], addrp[3],
               port[srv], addr.sin_addr.s_addr,
               port[srv], sock);
    }
    err = connect(sock,
                  (struct sockaddr *) &addr,
                  sizeof(struct sockaddr));
    if (verbose) {
        if (err == -1)
            printf("connect failure, errno=%d\n", errno);
        else
            printf("connect completed\n");
    }
    assert(err != -1);
    return sock;
}

int do_listen() {
    struct sockaddr_in  addr;
    long               *addrl;
    unsigned char      *addrp;
    bool                done;
    int                 err;
    struct hostent      hostent;
    int                 hostenterrno;
    struct hostent     *hostentp;
    int                 inx;
    socklen_t           len;
    int                 sock;
    int                 value;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    value = 1;
    err = setsockopt(sock,
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     (char *) &value,
                     sizeof(value));
    if (verbose)
        printf("setsockopt REUSEADDR sock=%d, err=%d\n", sock, err);
    assert(err == 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);
    err = gethostname(host[0], sizeof(host[0]));
    err = gethostbyname_r(host[0],
                          &hostent,
                          hostbuf,
                          sizeof(hostbuf),
                          &hostentp,
                          &hostenterrno);
    if (verbose) {
        printf("gethostbyname_r(host[0]=%s), err=%d\n", host[0], err);
    }
    assert(err == 0);
    assert(hostentp != NULL);
    done = false;
    for (inx = 0;; inx++) {
        addrl = (long *) hostentp->h_addr_list[inx];
        if (addrl == NULL)
            break;
        if (*addrl == 0)
            break;
        addr.sin_addr.s_addr = (int) *addrl;
        if (verbose) {
            addrp = (unsigned char *) addrl;
            printf("trying to bind to host=%s (%d.%d.%d.%d:%d) (0x%08X:%X), sock=%d\n",
                   host[0], addrp[0], addrp[1],
                   addrp[2], addrp[3], 0,
                   addr.sin_addr.s_addr, 0, sock);
        }
        err = bind(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr));
        if (err == 0) {
            done = true;
            break;
        } else {
            if (verbose) {
                printf("bind failed, err=%d, sock=%d, host=%s\n",
                       errno, sock, host[0]);
            }
        }
    }
    assert(done);
    if (verbose)
        printf("bind complete, sock=%d\n", sock);
    err = listen(sock, 10);
    assert(err != -1);
    len = sizeof(addr);
    err = getsockname(sock, (struct sockaddr *) &addr, &len);
    assert(err == 0);
    addrp = (unsigned char *) &addr.sin_addr.s_addr;
    sprintf(host[0], "%d.%d.%d.%d",
            addrp[0], addrp[1], addrp[2], addrp[3]);
    port[0] = ntohs(addr.sin_port);
    if (verbose)
        printf("listening to host=%s:%d (0x%08X:%X), sock=%d\n",
               host[0], port[0], addr.sin_addr.s_addr,
               port[0], sock);
    return sock;
}
#ifdef __GNUC__
#pragma GCC diagnostic error "-Wconversion"
#endif
#endif // USE_SB_FC

int main(int argc, char *argv[]) {
    _bcc_status      bcc;
    void            *buf;
    bool             client = false;
    int              count_read;
    int              count_written;
    int              count_xferred;
#ifdef USE_SB_FC
    int              err;
#endif // USE_SB_FC
    int              ferr;
    short            filenumr;
    short            filenums[MAX_SRV];
#ifdef USE_SB_FC
    short            info_count;
#endif // USE_SB_FC
    int              inx;
    int              len;
#ifdef USE_SB_FC
    short            list_inx;
#endif // USE_SB_FC
    int              loop = 10;
    char            *p;
    char             recv_buffer[BUFSIZ];
    char             send_buffer[MAX_SRV][BUFSIZ];
#ifdef USE_SB_FC
    int              socka1;
    int              socka2;
    int              sockc1[MAX_SRV];
    int              sockc2[MAX_SRV];
    int              sockl;
#endif // USE_SB_FC
    int              srv;
    SB_Tag_Type      tag;
    short            tfilenum;
    int              timeout = -1;
    TAD              zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxcp",     TA_Next, TA_NOMAX,    NULL       },
      { "-maxsp",     TA_Next, TA_NOMAX,    NULL       },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);

    if (client) {
#ifdef USE_SB_FC
        // check empty list
        ferr = BFILE_COMPLETE_GETINFO_((short *) info_list,
                                       MAX_INFO_LIST,
                                       &info_count);
        TEST_CHK_FEOK(ferr);
        assert(info_count == 0);
        // add bogus
        add_list[0].z_fnum_fd = 99;
        add_list[0].u_z_options.z_options.z_set_file = 0; // set
        add_list[0].u_z_options.z_options.z_filetype = 0; // guardian
        ferr = BFILE_COMPLETE_SET_((short *) add_list,
                                   1,
                                   &list_inx);
        assert(ferr == XZFIL_ERR_NOTFOUND);
        assert(list_inx == 0);

#endif // USE_SB_FC
        for (srv = 0; srv < MAX_SRV; srv++) {
            sprintf(send_buffer[srv], "$srv%d", srv);
            len = (int) strlen(send_buffer[srv]);
            ferr = BFILE_OPEN_((char *) send_buffer[srv], (short) len, &filenums[srv],
                               0, 0, 1,
                               0, 0, 0, 0, NULL);
            TEST_CHK_FEOK(ferr);
        }

#ifdef USE_SB_FC
        do_set_guardian(filenums[0], 0, 1); // add
        do_set_guardian(filenums[0], 1, 0); // remove (specific)
        do_set_guardian(filenums[0], 0, 1); // add
        do_set_guardian(-1, 1, 0);          // remove (all)
        do_set_guardian(-1, 0, 1);          // add (all)
#endif // USE_SB_FC

        for (srv = 0; srv < MAX_SRV; srv++) {
            for (inx = 0; inx < loop; inx++) {
                sprintf(send_buffer[srv], "inx=%d", inx);
                bcc = BWRITEREADX(filenums[srv],
                                  send_buffer[srv],
                                  (short) (strlen(send_buffer[srv]) + 1),
                                  BUFSIZ,
                                  &count_read,
                                  1);
                TEST_CHK_BCCEQ(bcc);
#ifdef USE_SB_FC
                ferr = BFILE_COMPLETE_((short *) &info,
                                        timeout,
                                        NULL,
                                        0,
                                        NULL);
                TEST_CHK_FEOK(ferr);
                assert(info.z_tag == 1);
#else
                tfilenum = -1;
                bcc = BAWAITIOX(&tfilenum,
                                &buf,
                                &count_xferred,
                                &tag,
                                timeout,
                                NULL);
                TEST_CHK_BCCEQ(bcc);
#endif
            }
        }
        for (srv = 0; srv < MAX_SRV; srv++) {
            p = strchr(send_buffer[srv], ':');
            *p = 0;
            p++;
            strcpy(host[srv], send_buffer[srv]);
            port[srv] = (unsigned short) atoi(p);
            if (verbose)
                printf("server[%d] returned host=%s, port=%d\n",
                       srv, host[srv], port[srv]);
        }

#ifdef USE_SB_FC
        if (verbose)
            printf("client connecting up\n");
        // connect up, and setup fds
        for (srv = 0; srv < MAX_SRV; srv++) {
            sockc1[srv] = do_connect(srv);
            sockc2[srv] = do_connect(srv);
        }
        do_set_guardian(-1, 1, 0);                 // remove (all)
        do_set_linux(sockc1[0], 0, 0, 0, 0, 1);    // add-no rr/wr/exc

        // nothing should be ready
        timeout = 0;
        ferr = BFILE_COMPLETE_((short *) &info,
                               timeout,
                               NULL,
                               0,
                               NULL);
        assert(ferr == XZFIL_ERR_TIMEDOUT);

        // server has sent something, so rr should be on
        sleep(1);
        do_set_linux(sockc1[0], 0, 1, 0, 0, 1); // rr
        timeout = -1;
        ferr = BFILE_COMPLETE_((short *) &info,
                               timeout,
                               NULL,
                               0,
                               NULL);
        TEST_CHK_FEOK(ferr);
        assert(info.z_filetype); // linux
        assert(info.z_error == XZFIL_ERR_OK);
        assert(info.z_fnum_fd == sockc1[0]);
        assert(info.u_z_return_value.z_return_value.z_read_ready);
        assert(!info.u_z_return_value.z_return_value.z_write_ready);
        assert(!info.u_z_return_value.z_return_value.z_exception);

        // wr should be on
        do_set_linux(sockc1[0], 0, 0, 1, 0, 1); // wr
        timeout = -1;
        ferr = BFILE_COMPLETE_((short *) &info,
                               timeout,
                               NULL,
                               0,
                               NULL);
        TEST_CHK_FEOK(ferr);
        assert(info.z_filetype); // linux
        assert(info.z_error == XZFIL_ERR_OK);
        assert(info.z_fnum_fd == sockc1[0]);
        assert(!info.u_z_return_value.z_return_value.z_read_ready);
        assert(info.u_z_return_value.z_return_value.z_write_ready);
        assert(!info.u_z_return_value.z_return_value.z_exception);

        do_set_linux(sockc1[0], 0, 0, 0, 0, 1);    // clear ready
        do_set_linux(sockc2[0], 0, 0, 0, 0, 2);    // add-no rr/wr/exc

        // nothing should be ready
        timeout = 0;
        ferr = BFILE_COMPLETE_((short *) &info,
                               timeout,
                               NULL,
                               0,
                               NULL);
        assert(ferr == XZFIL_ERR_TIMEDOUT);

        // rr should NOT be ready
        do_set_linux(sockc2[0], 0, 1, 0, 0, 2); // rr
        timeout = 100;
        ferr = BFILE_COMPLETE_((short *) &info,
                               timeout,
                               NULL,
                               0,
                               NULL);
        assert(ferr == XZFIL_ERR_TIMEDOUT);

        // test fairness (check trace)
        do_set_linux(-1, 1, 0, 0, 0, 0);  // clear linux
        for (srv = 0; srv < MAX_SRV; srv++)
            do_set_guardian(filenums[srv], 0, srv + 1); // add
        for (srv = 0; srv < MAX_SRV; srv++) {
            bcc = BWRITEREADX(filenums[srv],
                              NULL,
                              0,
                              BUFSIZ,
                              &count_read,
                              1);
            TEST_CHK_BCCEQ(bcc);
        }
        usleep(10000);
        for (srv = 0; srv < MAX_SRV; srv++) {
            timeout = -1;
            ferr = BFILE_COMPLETE_((short *) &info,
                                   timeout,
                                   NULL,
                                   0,
                                   NULL);
            TEST_CHK_FEOK(ferr);
            assert(!info.z_filetype); // guardian
            assert(info.z_error == XZFIL_ERR_OK);
            assert(info.z_fnum_fd == filenums[srv]);
        }
#endif // USE_SB_FC

        if (verbose)
            printf("client closing\n");
        for (srv = 0; srv < MAX_SRV; srv++) {
            ferr = BFILE_CLOSE_(filenums[srv], 0);
            TEST_CHK_FEOK(ferr);
        }
        printf("if there were no asserts, all is well\n");
    } else {
#ifdef USE_SB_FC
        sockl = do_listen();
        assert(sockl != -1);
#endif // USE_SB_FC

        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 1,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            bcc = BREADUPDATEX(filenumr,
                               recv_buffer,
                               BUFSIZ,
                               &count_read,
                               1);
            TEST_CHK_BCCEQ(bcc);
            tfilenum = filenumr;
            bcc = BAWAITIOX(&tfilenum,
                            &buf,
                            &count_xferred,
                            &tag,
                            timeout,
                            NULL);
            TEST_CHK_BCCEQ(bcc);
            assert(tag == 1);
            sprintf(recv_buffer, "%s:%d\n", host[0], port[0]);
            count_read = (short) (strlen(recv_buffer) + 1);
            bcc = BREPLYX(recv_buffer,
                          count_read,
                          &count_written,
                          0,
                          XZFIL_ERR_OK);
            TEST_CHK_BCCEQ(bcc);
        }

#ifdef USE_SB_FC
        if (verbose)
            printf("server accepting\n");
        socka1 = do_accept(sockl);
        socka2 = do_accept(sockl);
        err = write(socka1, recv_buffer, 1);
        assert(err == 1);

        for (inx = 0; inx < 1; inx++) {
            bcc = BREADUPDATEX(filenumr,
                               recv_buffer,
                               BUFSIZ,
                               &count_read,
                               1);
            TEST_CHK_BCCEQ(bcc);
            tfilenum = filenumr;
            bcc = BAWAITIOX(&tfilenum,
                            &buf,
                            &count_xferred,
                            &tag,
                            timeout,
                            NULL);
            TEST_CHK_BCCEQ(bcc);
            bcc = BREPLYX(recv_buffer,
                          0,
                          &count_written,
                          0,
                          XZFIL_ERR_OK);
            TEST_CHK_BCCEQ(bcc);
        }
#endif // USE_SB_FC

        if (verbose)
            printf("server closing\n");
        ferr = BFILE_CLOSE_(filenumr, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
