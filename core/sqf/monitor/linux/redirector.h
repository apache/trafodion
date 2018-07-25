///////////////////////////////////////////////////////////////////////////////
//
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
//
///////////////////////////////////////////////////////////////////////////////

#ifndef REDIRECTOR_H_
#define REDIRECTOR_H_
#ifndef NAMESERVER_PROCESS

#include <map>
#include <list>
using namespace std;
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <mpi.h>

#include "msgdef.h"
#include "internal.h"
#include "lock.h"
#include "monlogging.h"

class CRedirect
{
 protected:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    CRedirect(int nid, int pid);
    CRedirect(const char *nodeName, const char *processName, int nid, int pid);
    virtual ~CRedirect();

    virtual void handleHangup() = 0;
    virtual int  handleInput() = 0;
    virtual void handleOutput(ssize_t count, char *buffer) = 0;

    virtual bool ignoreFdOnHangup(int) { return false; }; // false by default

    bool active() { return activity_; };
    const char *nodeName() { return(nodeName_.c_str()); };
    const char *processName() { return(processName_.c_str()); };
    int nid() { return nid_; };
    int pid() { return pid_; };
    int fd()  { return fd_; };
    bool idle() { return idle_; };
    void validateObj( void );

protected:
    void setFdFlags(int fd, int newFlags);

    static const int bufferSize_ = 4096;
    int  fd_;
    bool activity_;
    string nodeName_;     // hostname of associated process
    string processName_;  // name of associated process
    string filename_;     // name of file that was opened
    int nid_;             // node id of associated process
    int pid_;             // process id of associated process
    bool idle_;           // If true, previously wrote all available
                          // data to the pipe and removed pipe fd from
                          // list of file descriptors monitored by epoll.
};

// Redirect object for stdin from tty on local node,
// destination is pipe on this node.
class CRedirectStdinTty: public CRedirect
{
public:
    CRedirectStdinTty(int nid, int pid, char filename[], int pipeFd);
    virtual ~CRedirectStdinTty();

    void handleHangup();
    int handleInput();
    void handleOutput(ssize_t count, char *buffer);

private:
    char * buffer_;       // buffer for data saved due to pipe full
    int bufferPos_;       // buffer position of first unwritten byte
    int bufferDataLen_;   // number of bytes obtained in last "read"
    int pipeFd_;
};

// Redirect object for stdin from file on local node,
// destination is pipe on this node.
class CRedirectStdinFile: public CRedirect
{
public:
    CRedirectStdinFile(int nid, int pid, char filename[], int pipeFd);
    virtual ~CRedirectStdinFile();

    void handleHangup();
    int handleInput();
    void handleOutput(ssize_t count, char *buffer);

private:
    char * buffer_;       // buffer for data read from stdin file
    int bufferPos_;       // buffer position of first unwritten byte
    int bufferDataLen_;   // number of bytes obtained in last "read"
    int pipeFd_;
};

// Redirect object for stdin input coming from tty or file on another node,
// destination is pipe on this node.
class CRedirectAncestorStdin: public CRedirect
{
public:
    CRedirectAncestorStdin(int nid, int pid, int pipeFd, int ancestor_nid,
                           int ancestor_pid);
    virtual ~CRedirectAncestorStdin();

    void handleHangup();
    int handleInput();
    void handleOutput(ssize_t count, char *buffer);

private:
    ioData_t * buffer_;
    int bufferPos_;       // buffer position of first unwritten byte
    int bufferDataLen_;   // number of bytes obtained in last buffer
    int pipeFd_;

    typedef list<ioData_t*> ioDataList_t;
    ioDataList_t ioDataList_;
    CLock ioDataListLock_;

    int ancestorNid_;
    int ancestorPid_;
};

// Redirect object for stdin on local node,
// destination is pipe on remote node.
class CRedirectStdinRemote: public CRedirect
{
public:
    CRedirectStdinRemote(const char *filename, int requesterNid, int requesterPid);
    virtual ~CRedirectStdinRemote();

    void handleHangup();
    int handleInput();
    void handleOutput(ssize_t count, char *buffer);

    bool isFile() { return fileType_; }

private:
    int requesterNid_;
    bool fileType_;
};

class CRedirectStdout: public CRedirect
{
public:
    CRedirectStdout(int nid, int pid, const char *filename, int sourceFd);
    virtual ~CRedirectStdout();

    void handleHangup();
    int handleInput();
    void handleOutput(ssize_t count, char *buffer);

private:
    int sourceFd_;
    bool cantWrite_;
    bool fileTooLarge_;

    void saveData(ssize_t count, char *buffer);

    // the real fd_ of the stdout file can be in epoll list 
    // if waiting for it to complete the writes. If hangup occurs
    // during this interval, the hangup will get processed on the 
    // pipedfd (sourceFd_) 
    bool ignoreFdOnHangup(int fd) { return (fd == fd_); }

    typedef struct deferredData_s
    {
        int  count;
        char *data;
    } deferredData_t;

    typedef list<deferredData_t> deferredDataList_t;
    deferredDataList_t deferredDataList_;
};

class CRedirectAncestorStdout: public CRedirect
{
public:
    CRedirectAncestorStdout(int nid, int pid, int ancestor_nid, int ancestor_pid);
    virtual ~CRedirectAncestorStdout();

    void handleHangup();
    int handleInput();
    void handleOutput(ssize_t count, char *buffer);

private:
    int ancestor_nid_;
    int ancestor_pid_;
};

class CRedirectStderr: public CRedirect
{
public:
    CRedirectStderr(const char *nodeName, const char *processName,
                    int nid, int pid);
    virtual ~CRedirectStderr();

    void handleHangup();
    int handleInput();
    void handleOutput(ssize_t count, char *buffer);

private:
    int     header_count_;
};

class CRedirector
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    CRedirector();
    virtual ~CRedirector();

    void start();

    void diagnoseHangup(CRedirect *redirect, int fd);
    void redirectThread();

    void stdinFd(int nid, int pid, int &pipeFd, char filename[],
                 int ancestor_nid, int ancestor_pid);

    void stdoutFd(int nid, int pid, int pipeFd, const char *filename,
                  int ancestor_nid, int ancestor_pid);

    void stderrFd(const char *nodeName, const char *processName,
                  int nid, int pid, int fd);

    int stdinRemote(const char *filename, int requesterNid, int requesterPid);
    void stdinOff(int fd);
    void stdinOn(int fd);

    void tryShutdownPipeFd(int pid, int fd, bool pv_delete_redirect);

    void disposeIoData(int fd, int count, char *buffer);

    void addToEpollSet(int fd, int epoll_events, const char * type);
    void delFromEpollSet(int fd);
    void addToMap(int fd, CRedirect * redirect);
    void delFromMap(int fd);

    void shutdownWork(void);

    pthread_t tid() { return thread_id_; }

    bool haveSyncData();

    void monLogWrite(int event_type, posix_sqlog_severity_t severity, char *evl_buf);
    void handleMonLogWrites();

    typedef struct {
        int event_type;
        posix_sqlog_severity_t severity;
        char msg[MON_STRING_BUF_SIZE];
    } monLogData_t; 

private:
    void shutdownPipeFd(int fd);


    static const int MAX_EPOLL_EVENTS = 7;
    // Expected number of file descriptors to manage
    static const int MAX_EPOLL_FDS = 5;

    // epoll file descriptor
    int epoll_fd_;

    // Redirector thread's id
    pthread_t                      thread_id_;

    // Mapping between file descriptors and redirect objects
    typedef map<int,CRedirect *> fdToRedirect_t;
    fdToRedirect_t fdMap_;
    CLock fdMapLock_;

    typedef list<monLogData_t*> monLogDataList_t;
    monLogDataList_t monLogDataList_;
    CLock monLogListLock_;


    bool shutdown_;
};


#endif
#endif // REDIRECTOR_H_
