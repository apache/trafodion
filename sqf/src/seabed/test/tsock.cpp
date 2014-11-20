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

#include "tsock.h"
bool TSockClient::trace   = false;
bool TSockListener::trace = false;
bool TSockServer::trace   = false;

TSockClient::TSockClient()
: sock(-1) {
}

TSockClient::~TSockClient() {
    if (sock >= 0)
        ::close(sock);
}

//
// without this pragma, gcc has problems with the following
//   addr.sin_port = htons((uint16_t) port);
// And gcc doesn't like this pragma inside the function
//
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#endif
void TSockClient::connect(char *host, int port) {
    const char *WHERE = "TSockClient::connect";
    char        hostbuf[500];

    int lsock = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(lsock != -1);
    int tcpopt = 1;
    int err = ::setsockopt(lsock,
                           IPPROTO_TCP,
                           TCP_NODELAY,
                           (char *) &tcpopt,
                           sizeof(tcpopt));
    if (trace)
        printf("%s: setsockopt sock=%d, err=%d\n",
               WHERE,
               lsock,
               err);
    assert(err == 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t) port);
    struct hostent hostent;
    struct hostent *hostentp;
    int hostenterrno;
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
    if (trace)
        printf("%s: connect attempt to host=%s, port=%d\n",
               WHERE, host, port);
    err = ::connect(lsock,
                    (struct sockaddr *) &addr,
                    sizeof(struct sockaddr));
    if (trace) {
        if (err)
            printf("%s: connect failure, errno=%d\n", WHERE, errno);
        else
            printf("%s: connect completed\n", WHERE);
    }
    assert(err != -1);
    sock = lsock;
}
#ifdef __GNUC__
#pragma GCC diagnostic error "-Wconversion"
#endif

void TSockClient::read(void *buf, size_t count) {
    const char *WHERE = "TSockClient::read";
    char       *bufp;
    char        errnobuf[100];
    ssize_t     rcnt;

    bufp = (char *) buf;
    do {
        rcnt = ::read(sock, bufp, count);
        ierrno = errno;
        if (trace) {
            if (rcnt == -1)
                printf("%s: read completed on sock=%d, rcnt=%d, errno=%d(%s)\n",
                       WHERE,
                       sock,
                       (int) rcnt,
                       errno,
                       strerror_r(errno, errnobuf, sizeof(errnobuf)));
            else
                printf("%s: read completed on sock=%d, rcnt=%d\n",
                       WHERE, sock, (int) rcnt);
        }
        if (rcnt == -1) {
            if (errno == EINTR)
                continue;
            assert(rcnt != -1);
        }
        bufp += rcnt;
        count -= rcnt;
    } while (count > 0);
}

void TSockClient::set_trace(bool ltrace) {
    trace = ltrace;
}

void TSockClient::write(const void *buf, size_t count) {
    const char *WHERE = "TSockClient::write";
    const char *bufp;
    char        errnobuf[100];
    ssize_t     wcnt;

    bufp = (const char *) buf;
    do {
        wcnt = ::write(sock, bufp, count);
        ierrno = errno;
        if (trace) {
            if (wcnt == -1)
                printf("%s: write completed on sock=%d, wcnt=%d, errno=%d(%s)\n",
                       WHERE,
                       sock,
                       (int) wcnt,
                       errno,
                       strerror_r(errno, errnobuf, sizeof(errnobuf)));
            else
                printf("%s: write completed on sock=%d, wcnt=%d\n",
                       WHERE, sock, (int) wcnt);
        }
        if (wcnt == -1) {
            if (errno == EINTR)
                continue;
            assert(wcnt != -1);
        }
        bufp += wcnt;
        count -= wcnt;
    } while (count > 0);
}

TSockListener::TSockListener()
: sock(-1) {
}

TSockListener::~TSockListener() {
    if (sock >= 0)
        ::close(sock);
}

TSockServer *TSockListener::accept() {
    const char *WHERE = "TSockListener::accept";

    struct sockaddr c_addr;
    socklen_t c_addr_len = sizeof(c_addr);
    if (trace)
        printf("%s: accept attempt to sock=%d\n", WHERE, sock);
    int lsock = ::accept(sock, &c_addr, &c_addr_len);
    assert(lsock != -1);
    int value = 1;
    int err = ::setsockopt(lsock,
                           SOL_SOCKET,
                           SO_REUSEADDR,
                           (char *) &value,
                           sizeof(value));
    if (trace)
        printf("%s: setsockopt sock=%d, err=%d\n",
               WHERE,
               lsock,
               err);
    assert(err == 0);
    int tcpopt = 1;
    err = ::setsockopt(lsock,
                       IPPROTO_TCP,
                       TCP_NODELAY,
                       (char *) &tcpopt,
                       sizeof(tcpopt));
    if (trace)
        printf("%s: setsockopt sock=%d, err=%d\n",
               WHERE,
               lsock,
               err);
    assert(err == 0);
    if (trace)
        printf("%s: accept completed on sock=%d, new sock=%d\n",
               WHERE, sock, lsock);
    TSockServer *listener = new TSockServer(lsock);
    return listener;
}

//
// without this pragma, gcc has problems with the following
//   addr.sin_port = htons((uint16_t) *port);
//   *port = ntohs(addr.sin_port);
// And gcc doesn't like this pragma inside the function
//
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#endif
void TSockListener::listen(char *host, int *port, char *bind_addr) {
    const char *WHERE = "TSockListener::listen";
    char        hostbuf[500];
    char        lhost[BUFSIZ];
    int        *addrlp;

    int lsock = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(lsock != -1);
    int value = 1;
    int err = ::setsockopt(lsock,
                           SOL_SOCKET,
                           SO_REUSEADDR,
                           (char *) &value,
                           sizeof(value));
    if (trace)
        printf("%s: setsockopt sock=%d, err=%d\n",
               WHERE,
               lsock,
               err);
    assert(err == 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t) *port);
    struct hostent hostent;
    struct hostent *hostentp;
    int hostenterrno;
    err = gethostbyname_r(host,
                          &hostent,
                          hostbuf,
                          sizeof(hostbuf),
                          &hostentp,
                          &hostenterrno);
    assert(err == 0);
    assert(hostentp != NULL);
    bool done = false;
    for (int inx = 0;; inx++) {
        addrlp = (int *) hostentp->h_addr_list[inx];
        if (addrlp == NULL)
            break;
        if (*addrlp == 0)
            break;
        addr.sin_addr.s_addr = (int) *addrlp;
        if (trace) {
            unsigned char *addrp = (unsigned char *) addrlp;
            printf("%s: trying to bind to host=%d.%d.%d.%d:%d, sock=%d\n",
                   WHERE, addrp[0], addrp[1],
                   addrp[2], addrp[3], *port,
                   lsock);
        }
        err = ::bind(lsock, (struct sockaddr *) &addr,
                     sizeof(struct sockaddr));
        if (err == 0) {
            done = true;
            break;
        } else {
            if (trace) {
                ::gethostname(lhost, sizeof(lhost));
                printf("%s: bind failed, err=%d, sock=%d, host=%s\n",
                       WHERE, errno, lsock, lhost);
            }
        }
    }
    assert(done);
    err = ::listen(lsock, 1);
    assert(err != -1);
    sock = lsock;
    socklen_t len = sizeof(addr);
    err = ::getsockname(lsock, (struct sockaddr *) &addr, &len);
    assert(err == 0);
    *port = ntohs(addr.sin_port);
    if (bind_addr != NULL) {
        unsigned char *addrp = (unsigned char *) addrlp;
        sprintf(bind_addr, "%d.%d.%d.%d:%d",
                addrp[0], addrp[1], addrp[2], addrp[3],
                *port);
    }
    if (trace)
        printf("%s: bound to host=%s, port=%d, sock=%d\n",
               WHERE, host, *port, lsock);
}
#ifdef __GNUC__
#pragma GCC diagnostic error "-Wconversion"
#endif

void TSockListener::set_trace(bool ltrace) {
    trace = ltrace;
}

TSockServer::TSockServer(int lsock) {
    sock = lsock;
}

TSockServer::~TSockServer() {
    ::close(sock);
}

void TSockServer::read(void *buf, size_t count) {
    const char *WHERE = "TSockServer::read";
    char       *bufp;
    char        errnobuf[100];
    ssize_t     rcnt;

    bufp = (char *) buf;
    do {
        rcnt = ::read(sock, bufp, count);
        ierrno = errno;
        if (trace) {
            if (rcnt == -1)
                printf("%s: read completed on sock=%d, rcnt=%d, errno=%d(%s)\n",
                       WHERE,
                       sock,
                       (int) rcnt,
                       errno,
                       strerror_r(errno, errnobuf, sizeof(errnobuf)));
            else
                printf("%s: read completed on sock=%d, rcnt=%d\n",
                       WHERE, sock, (int) rcnt);
        }
        if (rcnt == -1) {
            if (errno == EINTR)
                continue;
            assert(rcnt != -1);
        }
        bufp += rcnt;
        count -= rcnt;
    } while (count > 0);
}

void TSockServer::set_trace(bool ltrace) {
    trace = ltrace;
}

void TSockServer::write(const void *buf, size_t count) {
    const char *WHERE = "TSockServer::write";
    const char *bufp;
    char        errnobuf[100];
    ssize_t     wcnt;

    bufp = (const char *) buf;
    do {
        wcnt = ::write(sock, bufp, count);
        ierrno = errno;
        if (trace) {
            if (wcnt == -1)
                printf("%s: write completed on sock=%d, wcnt=%d, errno=%d(%s)\n",
                       WHERE,
                       sock,
                       (int) wcnt,
                       errno,
                       strerror_r(errno, errnobuf, sizeof(errnobuf)));
            else
                printf("%s: write completed on sock=%d, wcnt=%d\n",
                       WHERE, sock, (int) wcnt);
        }
        if (wcnt == -1) {
            if (errno == EINTR)
                continue;
            assert(wcnt != -1);
        }
        bufp += wcnt;
        count -= wcnt;
    } while (count > 0);
}

#ifdef MAIN
int main(int argc, char *argv[]) {
    int  arg;
    bool  client = false;
    int   err;
    FILE *f;
    char  host[BUFSIZ];
    char  info[BUFSIZ];
    int   len;
    char *p;
    int   port;

    for (arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "-client") == 0)
            client = true;
    }

    err = ::gethostname(host, sizeof(host));
    assert(!err);

    if (client) {
        TSockClient client;
        f = fopen("zx", "r");
        assert(f != NULL);
        fgets(info, sizeof(info), f);
        fclose(f);
        p = strstr(info, ":");
        assert(p != NULL);
        *p = 0;
        port = atoi(&p[1]);
        client.connect(info, port);
        char *wbuf = (char *) "hi from c";
        char rbuf[BUFSIZ];
        printf("client write: '%s'\n", wbuf);
        len = (int) strlen(wbuf) + 1;
        client.write(&len, sizeof(len));
        client.write(wbuf, len);
        client.read(&len, sizeof(len));
        client.read(rbuf, len);
        printf("client read: '%s'\n", rbuf);
    } else {
        TSockListener  listener;
        TSockServer   *server;

        listener.listen(host, &port, NULL);
        f = fopen("zx", "w");
        assert(f != NULL);
        fprintf(f, "%s:%d", host, port);
        fclose(f);
        server = listener.accept();
        char rbuf[BUFSIZ];
        server->read(&len, sizeof(len));
        server->read(rbuf, len);
        printf("server read: '%s'\n", rbuf);
        printf("server write: '%s'\n", rbuf);
        len = (int) strlen(rbuf) + 1;
        server->write(&len, sizeof(len));
        server->write(rbuf, strlen(rbuf) + 1);
    }
    return 0;
}
#endif
