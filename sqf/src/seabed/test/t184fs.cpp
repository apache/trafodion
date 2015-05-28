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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/epoll.h>

#include "seabed/fs.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"


char            host[100];
unsigned short  port;
bool            verbose = false;

int             epoll_fd      = -1;
int             pin_main      = -1;
bool            thread_io_done = false;
pthread_t       thread_io_tid = -1;

typedef struct QE {
    struct QE          *next;
    struct epoll_event  event;
} QE;

class Q {
public:
    Q() : head(NULL), tail(NULL) {}
    void  add(QE *qe);
    QE   *remove();
    QE *head;
    QE *tail;
};


void Q::add(QE *qe) {
    qe->next = NULL;
    if (head == NULL)
       head = qe;
    else
       head->next = qe;
   tail = qe;
}

QE *Q::remove() {
    QE *qe;

    qe = head;
    if (qe != NULL) {
        head = qe->next;
        if (head == NULL)
            tail = NULL;
        qe->next = NULL;
    }
    return qe;
}

Q io_q;

void epoll_add(int fd) {
    int                err;
    struct epoll_event event;

    event.events = EPOLLIN | EPOLLET;
    event.data.fd = fd;
    err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    if (verbose)
        printf("epoll_ctl(add) fd=%d, rtn=%d\n", fd, err);
    assert(err != -1);
}

void epoll_init() {
    int err;

    err = epoll_create(10);
    if (verbose)
        printf("epoll_create rtn=%d\n", err);
    assert(err != -1);
    epoll_fd = err;
}

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#endif
int do_sock_accept(int sock) {
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

int do_sock_connect() {
    struct sockaddr_in  addr;
    unsigned char      *addrp;
    int                 err;
    char                hostbuf[500];
    struct hostent      hostent;
    int                 hostenterrno;
    struct hostent     *hostentp;
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
    addr.sin_port = htons(port);
    err = gethostbyname_r(host,
                          &hostent,
                          hostbuf,
                          sizeof(hostbuf),
                          &hostentp,
                          &hostenterrno);
    assert(err == 0);
    assert(hostentp != NULL);
    assert(*(long *) hostentp->h_addr != -1);
    addr.sin_addr.s_addr = (uint32_t) *(uint32_t *) hostentp->h_addr;
    if (verbose) {
        addrp = (unsigned char *) &addr.sin_addr.s_addr;
        printf("connect attempt to host=%s (%d.%d.%d.%d:%d) (0x%08X:%X), sock=%d\n",
               host,
               addrp[0], addrp[1], addrp[2], addrp[3],
               port, addr.sin_addr.s_addr,
               port, sock);
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

int do_sock_listen() {
    struct sockaddr_in  addr;
    long               *addrl;
    unsigned char      *addrp;
    bool                done;
    int                 err;
    char                hostbuf[1000];
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
    gethostname(host, sizeof(host));
    err = gethostbyname_r(host,
                          &hostent,
                          hostbuf,
                          sizeof(hostbuf),
                          &hostentp,
                          &hostenterrno);
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
                   host, addrp[0], addrp[1],
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
                       errno, sock, host);
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
    sprintf(host, "%d.%d.%d.%d",
            addrp[0], addrp[1], addrp[2], addrp[3]);
    port = ntohs(addr.sin_port);
    if (verbose)
        printf("listening to host=%s:%d (0x%08X:%X), sock=%d\n",
               host, port, addr.sin_addr.s_addr,
               port, sock);
    return sock;
}
#ifdef __GNUC__
#pragma GCC diagnostic error "-Wconversion"
#endif

void do_io_comp() {
    QE *qe;

    for (;;) {
        qe = io_q.remove();
        if (qe == NULL)
            break;
        if (verbose)
            printf("completion fd=%d, event=0x%x\n",
                   qe->event.data.fd,
                   qe->event.events);
        delete qe;
    }
}

void *do_thread_io(void *) {
    enum {              MAX_EVENTS = 10 };
    int                 err;
    struct epoll_event *event;
    struct epoll_event  events[MAX_EVENTS];
    int                 inx;
    QE                 *qe;

    while (!thread_io_done) {
        if (verbose)
            printf("epoll_wait start\n");
        err = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        if (verbose)
            printf("epoll_wait rtn=%d\n", err);
        assert(err != -1);
        for (inx = 0; inx < err; inx++) {
            event = &events[inx];
            if (verbose)
                printf("epoll_wait fd=%d, event=0x%x\n",
                       event->data.fd,
                       event->events);
            qe = new QE();
            memcpy(&qe->event, event, sizeof(*event));
            io_q.add(qe);
            XAWAKE(pin_main, PWU);
        }
    }
    return NULL;
}

void do_thread_create() {
    int err;

    thread_io_done = false;
    err = pthread_create(&thread_io_tid, NULL, do_thread_io, NULL);
    assert(err != -1);
}

void do_thread_join() {
    int err;

    thread_io_done = true;
    err = pthread_join(thread_io_tid, NULL);
    assert(err != -1);
}

int main(int argc, char *argv[]) {
    _bcc_status     bcc;
    void           *buf;
    bool            client = false;
    int             count_read;
    int             count_written;
    int             count_xferred;
    bool            e_ldone;
    bool            e_pwu;
    int             err;
    int             ferr;
    short           filenumr;
    short           filenums;
    int             inx;
    int             loop = 10;
    char           *p;
    char            recv_buffer[BUFSIZ];
    char            send_buffer[BUFSIZ];
    int             socka;
    int             sockc;
    int             sockl;
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout = -1;
    short           wakeup;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
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
        ferr = BFILE_OPEN_((char *) "$srv", (short) 4, &filenums,
                           0, 0, 1,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);

        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "inx=%d", inx);
            bcc = BWRITEREADX(filenums,
                              send_buffer,
                              (short) (strlen(send_buffer) + 1),
                              BUFSIZ,
                              &count_read,
                              1);
            TEST_CHK_BCCEQ(bcc);
            tfilenum = -1;
            bcc = BAWAITIOX(&tfilenum,
                            &buf,
                            &count_xferred,
                            &tag,
                            timeout,
                            NULL);
            TEST_CHK_BCCEQ(bcc);
        }
        p = strchr(send_buffer, ':');
        *p = 0;
        p++;
        strcpy(host, send_buffer);
        port = (unsigned short) atoi(p);
        if (verbose)
            printf("server returned host=%s, port=%d\n",
                   host, port);

        if (verbose)
            printf("client connecting up\n");
        // connect up, and setup fds
        epoll_init();
        sockc = do_sock_connect();
        pin_main = 1;
        proc_register_group_pin(-1, pin_main);
        epoll_add(sockc);
        do_thread_create();

        bcc = BWRITEREADX(filenums,
                          NULL,
                          0,
                          BUFSIZ,
                          &count_read,
                          1);
        TEST_CHK_BCCEQ(bcc);
        usleep(10000);
        e_pwu = false;
        e_ldone = false;
        for (;;) {
            wakeup = XWAIT(LDONE | PWU, -1);
            if (wakeup & PWU) {
                e_pwu = true;
                do_io_comp();
            }
            if (wakeup & LDONE) {
                e_ldone = true;
                timeout = -1;
                tfilenum = -1;
                bcc = BAWAITIOX(&tfilenum,
                                &buf,
                                &count_xferred,
                                &tag,
                                timeout,
                                NULL);
                TEST_CHK_BCCEQ(bcc);
            }
            if (e_ldone && e_pwu)
                break;
        }

        do_thread_join();
        if (verbose)
            printf("client closing\n");
        ferr = BFILE_CLOSE_(filenums, 0);
        TEST_CHK_FEOK(ferr);
        printf("if there were no asserts, all is well\n");
    } else {
        sockl = do_sock_listen();
        assert(sockl != -1);

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
            sprintf(recv_buffer, "%s:%d\n", host, port);
            count_read = (short) (strlen(recv_buffer) + 1);
            bcc = BREPLYX(recv_buffer,
                          count_read,
                          &count_written,
                          0,
                          XZFIL_ERR_OK);
            TEST_CHK_BCCEQ(bcc);
        }

        if (verbose)
            printf("server accepting\n");
        socka = do_sock_accept(sockl);
        err = (int) write(socka, recv_buffer, 1);
        assert(err != -1);

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
