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

#ifndef __TSOCK_H_
#define __TSOCK_H_

//
// Simple socket client
//
class TSockClient {
public:
    TSockClient();
    virtual ~TSockClient();

    void        connect(char *host, int port);
    void        read(void *buf, size_t count);
    static void set_trace(bool trace);
    void        write(const void *buf, size_t count);

private:
    int         ierrno;
    int         sock;
    static bool trace;
};

class TSockServer;

//
// Simple socket listener
//
class TSockListener {
public:
    TSockListener();
    virtual ~TSockListener();

    TSockServer *accept();
    void         listen(char *host, int  *port, char *bind_addr);
    static void  set_trace(bool trace);

private:
    int         sock;
    static bool trace;
};

//
// Simple socket server
//
class TSockServer {
public:
    TSockServer(int sock);
    virtual ~TSockServer();

    void        read(void *buf, size_t count);
    static void set_trace(bool trace);
    void        write(const void *buf, size_t count);

private:
    int         ierrno;
    int         sock;
    static bool trace;
};

#endif // !__TSOCK_H_
