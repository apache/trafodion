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

using namespace std;

#ifndef NAMESERVER_PROCESS
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#endif
#include <sys/epoll.h>
#ifndef NAMESERVER_PROCESS
#include <errno.h>
#include <stdlib.h>
#endif
#include <string.h>
#ifndef NAMESERVER_PROCESS
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#endif

#ifndef NAMESERVER_PROCESS
#include "monlogging.h"
#include "montrace.h"
#include "monitor.h"
#include "clusterconf.h"
#include "lock.h"
#include "lnode.h"
#include "pnode.h"
#include "lock.h"
#include "mlio.h"
#include "msgdef.h"
#include "redirector.h"
#include "replicate.h"
#include "monsonar.h"
#include "reqqueue.h"
#include "ptpclient.h"
#endif

#ifndef NAMESERVER_PROCESS
extern CNode *MyNode;
extern sigset_t SigSet;
extern CRedirector Redirector;
extern int MyPNID;
extern CLock MemModLock;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;
extern CMonStats *MonStats;
extern CReqQueue ReqQueue;
extern CPtpClient *PtpClient;
extern bool NameServerEnabled;
extern const char *StateString( STATE state);
#endif

const char *EpollEventString( __uint32_t events )
{
    static char str[80] = {0};
    
    strcpy( str, "( " );
    if ( events & EPOLLIN )
    {
        strncat( str, "EPOLLIN ", sizeof(str) );
    }
    if ( events & EPOLLOUT )
    {
        strncat( str, "EPOLLOUT ", sizeof(str) );
    }
    if ( events & EPOLLRDHUP )
    {
        strncat( str, "EPOLLRDHUP ", sizeof(str) );
    }
    if ( events & EPOLLPRI )
    {
        strncat( str, "EPOLLPRI ", sizeof(str) );
    }
    if ( events & EPOLLERR )
    {
        strncat( str, "EPOLLERR ", sizeof(str) );
    }
    if ( events & EPOLLHUP )
    {
        strncat( str, "EPOLLHUP ", sizeof(str) );
    }
    if ( events & EPOLLET )
    {
        strncat( str, "EPOLLET ", sizeof(str) );
    }
    if ( events & EPOLLONESHOT )
    {
        strncat( str, "EPOLLONESHOT ", sizeof(str) );
    }
    strncat( str, ")", sizeof(str) );

    return( str );
}

const char *EpollOpString( int op )
{
    static char str[15] = {0};

    switch (op)
    {
        case EPOLL_CTL_ADD:
            strcpy( str, "EPOLL_CTL_ADD" );
            break;
        case EPOLL_CTL_MOD:
            strcpy( str, "EPOLL_CTL_MOD" );
            break;
        case EPOLL_CTL_DEL:
            strcpy( str, "EPOLL_CTL_DEL" );
            break;
        default:
            strcpy( str, "Invalid OP" );
            break;
    }

    return( str );
}

#ifndef NAMESERVER_PROCESS
CRedirect::CRedirect(int nid, int pid)
                    :fd_(-1)
                    ,activity_(false)
                    ,nid_(nid)
                    ,pid_(pid)
                    ,idle_(false)
{
}

CRedirect::CRedirect(const char *nodeName, const char *processName,
                     int nid, int pid)
          :fd_(-1)
          ,activity_(false)
          ,nodeName_(nodeName)
          ,processName_(processName)
          ,nid_(nid)
          ,pid_(pid)
          ,idle_(false)
{
}

CRedirect::~CRedirect()
{
    pid_ = 0;
    fd_  = -1;
}

// Set specified flags for the given file descriptor
void CRedirect::setFdFlags(int fd, int newFlags)
{
    const char method_name[] = "CRedirect::setFdFlags";
    TRACE_ENTRY;

    int flags;
    if ((flags = fcntl(fd, F_GETFL)) == -1)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], fcntl(%d) error, %s.\n", method_name, fd,
                strerror(errno));
        mon_log_write(MON_REDIR_SETFDFLAGS_1, SQ_LOG_ERR, buf);
        flags = 0;
    }

    flags |= newFlags;
    if (fcntl(fd, F_SETFL, flags) == -1)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], fcntl(%d) error, %s.\n", method_name, fd,
                strerror(errno));
        mon_log_write(MON_REDIR_SETFDFLAGS_1, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}

void CRedirect::validateObj( void )
{
    if (strncmp((const char *) &eyecatcher_, "RED", 3) != 0 )
    {  // Not a valid object
        abort();
    }
}

CRedirectStdinTty::CRedirectStdinTty(int nid, int pid, char filename[], int pipeFd)
    : CRedirect(nid, pid), bufferDataLen_(0), pipeFd_(pipeFd)
{
    const char method_name[] = "CRedirectStdinTty::CRedirectStdinTty";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "REDA", 4);

    fd_ = open(filename,  O_RDONLY | O_NONBLOCK);
    if( fd_ == -1 )
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], open error for %s, %s.\n", method_name,
                filename, strerror(errno));
        mon_log_write(MON_REDIR_STDIN_TTY_1, SQ_LOG_ERR, buf);
    }

    else
    {
        filename_ = filename;
        if (trace_settings & TRACE_REDIRECTION)
        {
            trace_printf("%s@%d opened %s fd=%d, pid=%d, pipeFd=%d.\n",
                         method_name, __LINE__, filename, fd_, pid, pipeFd);
        }
    }

    // Allocate buffer for reading from stdin source file.
    // todo: handle memory allocation failure
    buffer_ = new char[bufferSize_];
    bufferPos_ = 0;

    // Set nonblocking mode for pipe
    setFdFlags(pipeFd, O_NONBLOCK);

    TRACE_EXIT;
}

CRedirectStdinTty::~CRedirectStdinTty()
{
    const char method_name[] = "CRedirectStdinTty::~CRedirectStdinTty";
    TRACE_ENTRY;

    if (trace_settings & TRACE_REDIRECTION)
    {
        trace_printf("%s@%d closing stdin fd=%d.\n",
                     method_name, __LINE__, fd_);
    }

    if (fd_ != -1)
    {
        Redirector.delFromMap(fd_);

        // Remove from list of monitored file descriptors
        Redirector.delFromEpollSet(fd_);

        if (close(fd_))
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], file %s close(%d) error, %s.\n", method_name,
                    filename_.c_str(), fd_, strerror(errno));
            mon_log_write(MON_REDIR_USTDIN_TTY_1, SQ_LOG_ERR, buf);
        }
    }
    delete [] buffer_;

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "reda", 4);

    TRACE_EXIT;
}


void CRedirectStdinTty::handleHangup()
{
    const char method_name[] = "CRedirectStdinTty::handleHangup";
    TRACE_ENTRY;


    TRACE_EXIT;
}

// Write saved data to pipe
int CRedirectStdinTty::handleInput()
{
    const char method_name[] = "CRedirectStdinTty::handleInput";
    TRACE_ENTRY;

    ssize_t countR;
    ssize_t countW;
    int retVal = 0;

    // Have data left over from earlier, compute amount remaining
    countR = bufferDataLen_ - bufferPos_;

    if (countR > 0)
    {
        if ((countW = write(pipeFd_, &buffer_[bufferPos_], countR)) == -1)
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], write to fd=%d, %s\n", method_name,
                    fd_, strerror(errno));
            mon_log_write(MON_REDIR_STDIN_TTY_HNDLIN_1, SQ_LOG_ERR, buf);
            retVal = -1;
        }
        else if (countW == countR)
        {
            // Since we have finished draining the saved buffer data:
            //   turn on  epoll events for stdin file.
            //   turn off epoll events for pipe.
            Redirector.delFromEpollSet(pipeFd_);
            Redirector.addToEpollSet(fd_, EPOLLIN, "stdin");
        }

        // Keep track of first unwritten byte for subsequent writes.
        bufferPos_ = bufferPos_ + countW;
    }

    TRACE_EXIT;

    return retVal;
}

void CRedirectStdinTty::handleOutput(ssize_t countR, char *buffer)
{
    const char method_name[] = "CRedirectStdinTty::handleOutput";
    TRACE_ENTRY;

    ssize_t countW;

    if ((countW = write(pipeFd_, buffer, countR)) == -1)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], write to fd=%d, %s\n", method_name,
                fd_, strerror(errno));
        mon_log_write(MON_REDIR_STDIN_TTY_HNDLOUT_1, SQ_LOG_ERR, buf);
    }
    else if (countW != countR)
    {   // Could not write all data, save data so it can be
        // written when the pipe has room for it.
        bufferPos_ = 0;
        bufferDataLen_ = countR - countW;
        if ( bufferDataLen_ > bufferSize_ )
        {   // Not expected to occur but guard against buffer overrun
            bufferDataLen_ = bufferSize_;
        }
        memcpy(buffer_, buffer, bufferDataLen_);

        // So we can drain the saved buffer data:
        //   turn off epoll events for stdin file.
        //   turn on  epoll events for pipe.
        Redirector.delFromEpollSet(fd_);
        Redirector.addToEpollSet(pipeFd_, EPOLLOUT, "stdin");
    }

    TRACE_EXIT;
}

CRedirectStdinFile::CRedirectStdinFile(int nid, int pid, char filename[], int pipeFd)
    : CRedirect(nid, pid), bufferDataLen_(0), pipeFd_(pipeFd)
{
    const char method_name[] = "CRedirectStdinFile::CRedirectStdinFile";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "REDB", 4);

    // Open the stdin source file
    fd_ = open(filename, O_RDONLY | O_NONBLOCK);
    if( fd_ == -1 )
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], open error for stdin file %s, %s.\n", method_name,
                filename, strerror(errno));
        mon_log_write(MON_REDIR_STDIN_FILE_1, SQ_LOG_ERR, buf);
    }
    else
    {
        filename_ = filename;
        if (trace_settings & TRACE_REDIRECTION)
        {
            trace_printf("%s@%d opened %s fd=%d, pid=%d, pipeFd=%d.\n",
                         method_name, __LINE__, filename, fd_, pid, pipeFd);
        }
    }

    // Set nonblocking mode for pipe
    setFdFlags(pipeFd, O_NONBLOCK);

    // Allocate buffer for reading from stdin source file.
    // todo: handle memory allocation failure
    buffer_ = new char[bufferSize_];
    bufferPos_ = 0;

    TRACE_EXIT;
}

CRedirectStdinFile::~CRedirectStdinFile()
{
    const char method_name[] = "CRedirectStdinFile::~CRedirectStdinFile";
    TRACE_ENTRY;

    if (trace_settings & TRACE_REDIRECTION)
    {
        trace_printf("%s@%d closing stdin fd=%d.\n",
                     method_name, __LINE__, fd_);
    }

    if ((fd_ != -1) && close(fd_))
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], file %s close(%d) error, %s.\n", method_name,
                filename_.c_str(), fd_, strerror(errno));
        mon_log_write(MON_REDIR_USTDIN_FILE_1, SQ_LOG_ERR, buf);
    }
    delete [] buffer_;

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "redb", 4);

    TRACE_EXIT;
}

void CRedirectStdinFile::handleHangup()
{
    const char method_name[] = "CRedirectStdinFile::handleHangup";
    TRACE_ENTRY;


    TRACE_EXIT;
}


// Got indication that pipe is ready for writing.
// Read from file, write to pipe.
// Detect end of file, other file problems.
int CRedirectStdinFile::handleInput()
{
    const char method_name[] = "CRedirectStdinFile::handleInput";
    TRACE_ENTRY;

    ssize_t countR;
    ssize_t countW;
    int retVal = 0;

    if (bufferPos_ == 0)
    {   // No saved data, read more from file
        if ((countR = read(fd_, buffer_, bufferSize_)) == -1)
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], read from file %s (fd=%d), %s\n", method_name,
                    filename_.c_str(), fd_, strerror(errno));
            mon_log_write(MON_REDIR_STDIN_HNDLIN_1, SQ_LOG_ERR, buf);
        }
        else if (countR == 0)
        {
            // No bytes read, must be end-of-file
            if (trace_settings & TRACE_REDIRECTION)
            {
                trace_printf("%s@%d at end-of-file on fd=%d\n",
                             method_name, __LINE__, fd_);
            }
            retVal = -1;
        }
        else
        {
            bufferDataLen_ = countR;
        }
    }
    else
    {   // Have data left over from earlier, compute amount remaining
        countR = bufferDataLen_ - bufferPos_;
    }

    if (countR > 0)
    {
        if ((countW = write(pipeFd_, &buffer_[bufferPos_], countR)) == -1)
        {
            if (errno == EAGAIN)
            {   // Writing would cause blocking
                // Will try again later.
                if (trace_settings & TRACE_REDIRECTION)
                {
                    trace_printf("%s@%d Got EAGAIN for fd=%d, bufferPos=%d, "
                                 "bufferDataLen=%d\n",
                                 method_name, __LINE__, fd_, bufferPos_,
                                 bufferDataLen_);
                }
            }
            else if (errno == EPIPE)
            {   // Read end of pipe has been closed
                if (trace_settings & TRACE_REDIRECTION)
                {
                    trace_printf("%s@%d Got EPIPE for fd=%d\n",
                                 method_name, __LINE__, fd_);
                }
                retVal = -1;
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                sprintf(buf, "[%s], write to fd=%d, %s\n", method_name,
                        pipeFd_, strerror(errno));
                mon_log_write(MON_REDIR_STDIN_HNDLIN_2, SQ_LOG_ERR, buf);
            }
        }
        else if (countW != countR)
        {   // Could not write all data, keep track of first unwritten
            // byte for subsequent writes.
            bufferPos_ = bufferPos_ + countW;
        }
        else
        {   // Have written all data, will need to get more next time.
            bufferPos_ = 0;
        }
    }

    TRACE_EXIT;

    return retVal;
}


void CRedirectStdinFile::handleOutput(ssize_t , char *)
{
    const char method_name[] = "CRedirectStdinFile::handleOutput";
    TRACE_ENTRY;

    // handleOutput not used for CRedirectStdinFile object

    TRACE_EXIT;
}

CRedirectAncestorStdin::CRedirectAncestorStdin(int nid, int pid, int pipeFd,
                                               int ancestorNid,
                                               int ancestorPid)
    : CRedirect(nid, pid), buffer_(NULL), bufferPos_(0), bufferDataLen_(0),
      pipeFd_(pipeFd), ancestorNid_(ancestorNid), ancestorPid_(ancestorPid)
{
    const char method_name[] = "CRedirectAncestorStdin::CRedirectAncestorStdin";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "REDC", 4);

    // Set nonblocking mode for pipe
    setFdFlags(pipeFd, O_NONBLOCK);

    idle_ = true;

    TRACE_EXIT;
}

CRedirectAncestorStdin::~CRedirectAncestorStdin()
{
    const char method_name[] = "CRedirectAncestorStdin::~CRedirectAncestorStdin";
    TRACE_ENTRY;

    // Delete pending buffer (if any)
    if (buffer_)
    {
        delete [] buffer_;
    }

    // Delete queued data (if any)
    while (!ioDataList_.empty())
    {
        // Get first data buffer from list
        buffer_ = ioDataList_.front();
        ioDataList_.pop_front();
        delete [] buffer_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "redc", 4);

    TRACE_EXIT;
}

void CRedirectAncestorStdin::handleHangup()
{
    const char method_name[] = "CRedirectAncestorStdin::handleHangup";
    TRACE_ENTRY;

    TRACE_EXIT;
}

// Pipe is ready to accept more data.  Write data to it if we have any.
int CRedirectAncestorStdin::handleInput()
{
    const char method_name[] = "CRedirectAncestorStdin::handleInput";
    TRACE_ENTRY;

    ssize_t countR;
    ssize_t countW;
    int retVal = 0;
    StdinReqType reqType = STDIN_FLOW_OFF;

    if (bufferPos_ == 0)
    {   // No saved data, get more from queued data list

        ioDataListLock_.lock();
        if (!ioDataList_.empty())
        {
            // Get first data buffer from list
            buffer_ = ioDataList_.front();
            ioDataList_.pop_front();

            bufferDataLen_ = buffer_->length;
            countR = bufferDataLen_;
        }
        else
        {
            // No data to write to pipe.

            retVal = -1;  // Caller will remove the pipe fd from list
                          // of monitored file descriptors.
            idle_ = true;
            countR = 0;
        }
        ioDataListLock_.unlock();
    }
    else
    {   // Have data left over from earlier, compute amount remaining
        countR = bufferDataLen_ - bufferPos_;
    }

    if (countR > 0)
    {
        if (trace_settings & TRACE_REDIRECTION)
        {
            trace_printf("%s@%d for fd=%d writing %d bytes to pipe.\n",
                         method_name, __LINE__, pipeFd_, (int)countR);
        }

        if ((countW = write(pipeFd_, &buffer_->data[bufferPos_], countR)) == -1)
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], write to fd=%d, %s\n", method_name,
                    fd_, strerror(errno));
            mon_log_write(MON_REDIR_ANCSTDIN_HNDLIN_1, SQ_LOG_ERR, buf);
            retVal = -1;

            bufferPos_ = 0;
            delete [] buffer_;
            buffer_ = NULL;

            reqType = STDIN_FLOW_ON;
        }
        else if (countW != countR)
        {   // Could not write all data, keep track of first unwritten
            // byte for subsequent writes.
            bufferPos_ = bufferPos_ + countW;
        }
        else
        {   // Have written all data, will need to get more.
            bufferPos_ = 0;
            delete [] buffer_;
            buffer_ = NULL;

            reqType = STDIN_FLOW_ON;
        }

        if (NameServerEnabled)
        {
            PtpClient->ProcessStdInReq( MyPNID
                                      , pid_
                                      , reqType
                                      , ancestorNid_
                                      , ancestorPid_  );
        }
        else
        {
            CReplStdinReq *repl = new CReplStdinReq( MyPNID
                                                   , pid_
                                                   , reqType
                                                   , ancestorNid_
                                                   , ancestorPid_ );
            Replicator.addItem(repl);
        }
    }

    TRACE_EXIT;

    return retVal;
}

// This method is invoked when stdin data is received from another
// node.   The process consuming the stdin data is on this node
// but the device or file supplying the data is on another node.
// We queue the data so it can be written to the pipe by the 
// handleInput method.
void CRedirectAncestorStdin::handleOutput(ssize_t countR, char *buffer)
{
    const char method_name[] = "CRedirectAncestorStdin::handleOutput";
    TRACE_ENTRY;

    if (trace_settings & TRACE_REDIRECTION)
    {
        trace_printf("%s@%d have incoming data for pipe fd=%d\n",
                     method_name, __LINE__, pipeFd_);
    }

    // Allocate buffer to contain stdin data arriving from another process
    ioData_t * buf = new ioData_t;
    if ( (size_t) countR > sizeof buf->data )
    {   // Not expected to occur but guard against buffer overrun
        countR = sizeof buf->data;
    }
    buf->length = countR;
    memcpy(buf->data, buffer, countR);

    // Queue data to be written to pipe
    ioDataListLock_.lock();
    ioDataList_.push_back( buf );

    if (idle_)
    {
        // Enable epoll events for the pipe.
        idle_ = false;
        Redirector.addToEpollSet(pipeFd_, EPOLLOUT, "stdin");
    }

    ioDataListLock_.unlock();

    TRACE_EXIT;
}


CRedirectStdinRemote::CRedirectStdinRemote(const char *filename,
                                           int requesterNid,
                                           int requesterPid)
    : CRedirect(requesterNid, requesterPid), requesterNid_(requesterNid), fileType_(false)
{
    const char method_name[] = "CRedirectStdinRemote::CRedirectStdinRemote";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "REDD", 4);

    fd_ = open(filename,  O_RDONLY | O_NONBLOCK);

    if( fd_ == -1 )
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], open error for %s, %s.\n", method_name,
                filename, strerror(errno));
        mon_log_write(MON_REDIR_STDINREMOTE_1, SQ_LOG_ERR, buf);
    }

    else
    { 
        filename_ = filename;
        if (trace_settings & TRACE_REDIRECTION)
        {
            trace_printf("%s@%d opened %s fd=%d, nid=%d, pid=%d.\n",
                         method_name, __LINE__, filename, fd_, requesterNid_, pid_);
        }

        struct stat statbuf;
        if (fstat(fd_, &statbuf) == -1)
        {
            printf("Error doing fstat on fd=%d, %s\n", fd_, strerror(errno));
        }
        else
        {
            if (S_ISREG(statbuf.st_mode))
            {
                fileType_ = true;
                if (trace_settings & TRACE_REDIRECTION)
                {
                    trace_printf("%s@%d %s is a regular file.\n",
                                 method_name, __LINE__, filename);
                }
            }
            // For now tty is unsupported due to problems with 
            // when multiple readers with outstanding reads.
//             else if (S_ISCHR(statbuf.st_mode))
//             {
//                 if (trace_settings & TRACE_REDIRECTION)
//                 {
//                     trace_printf("%s@%d %s is a character device.\n",
//                                  method_name, __LINE__, filename);
//                 }
//             }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                sprintf(buf, "[%s], %s is an unsupported file type.\n",
                        method_name, filename);
                mon_log_write(MON_REDIR_STDINREMOTE_2, SQ_LOG_INFO, buf);

                close(fd_);
                fd_ = -1;
            }
        }
    }

    TRACE_EXIT;
}

CRedirectStdinRemote::~CRedirectStdinRemote()
{
    const char method_name[] = "CRedirectStdinRemote::~CRedirectStdinRemote";
    TRACE_ENTRY;

    if (trace_settings & TRACE_REDIRECTION)
    {
        trace_printf("%s@%d closing stdin fd=%d.\n",
                     method_name, __LINE__, fd_);
    }

    if (fd_ != -1)
    {
        Redirector.delFromMap(fd_);

        if ( !fileType_ )
        {   // Remove from list of monitored file descriptors
            Redirector.delFromEpollSet(fd_);
        }

        if (close(fd_))
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], file %s close(%d) error, %s.\n", method_name,
                    filename_.c_str(), fd_, strerror(errno));
            mon_log_write(MON_REDIR_USTDINREMOTE_1, SQ_LOG_ERR, buf);
        }
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "redd", 4);

    TRACE_EXIT;
}

void CRedirectStdinRemote::handleHangup()
{
    const char method_name[] = "CRedirectStdinRemote::handleHangup";
    TRACE_ENTRY;

    TRACE_EXIT;
}

// Source for standard input is a file.  Read from the file, send
// data to requesting process.
int CRedirectStdinRemote::handleInput()
{
    const char method_name[] = "CRedirectStdinRemote::handleInput";
    TRACE_ENTRY;

    // todo: implementation

    TRACE_EXIT;

    return 0;
}

void CRedirectStdinRemote::handleOutput(ssize_t count, char *buffer)
{
    const char method_name[] = "CRedirectStdinRemote::handleOutput";
    TRACE_ENTRY;

    if (trace_settings & TRACE_REDIRECTION)
    {
        trace_printf("%s@%d adding buffer to list for (%d, %d), %d bytes\n",
                     method_name, __LINE__, requesterNid_, pid_,
                     (int)count);
    }

    if (NameServerEnabled)
    {
        PtpClient->ProcessStdIoData( requesterNid_
                                   , pid_
                                   , STDIN_DATA
                                   , count
                                   , buffer );
    }
    else
    {
        CReplStdioData *repl = new CReplStdioData( requesterNid_
                                                 , pid_
                                                 , STDIN_DATA
                                                 , count
                                                 , buffer );
        Replicator.addItem(repl);
    }

    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->StdinRemoteDataReplIncr();

    TRACE_EXIT;
}

CRedirectStdout::CRedirectStdout(int nid, int pid, const char *filename, int sourceFd)
    : CRedirect(nid, pid), sourceFd_(sourceFd), cantWrite_(false), fileTooLarge_(false)
{
    const char method_name[] = "CRedirectStdout::CRedirectStdout";
    TRACE_ENTRY;

    int rc = 0;
    int err = 0;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "REDE", 4);

    if (trace_settings & TRACE_REDIRECTION)
    {
        trace_printf( "%s@%d stdout, file=%s\n"
                    , method_name, __LINE__
                    , filename );
    }

    if (strlen(filename))
    {   // stdout file/device is on this node
        struct stat statbuf;
        memset(&statbuf, 0, sizeof(statbuf));
        rc = stat(filename, &statbuf);
        if (rc == -1)
        {
            err = errno;
        }

        if (rc == 0 || err == ENOENT)
        {
            fd_ = open(filename, O_CREAT | O_APPEND | O_WRONLY | O_NONBLOCK,
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if( fd_ == -1 )
            {
                err = errno;
                if ( err == EACCES &&
                    (S_ISCHR(statbuf.st_mode) || S_ISFIFO(statbuf.st_mode)) )
                { // Don't log error since it is a common occurrence
                    if (trace_settings & TRACE_REDIRECTION)
                    {
                        trace_printf( "%s@%d stdout is character device or "
                                      "named pipe, file=%s, errno=%d (%s)\n"
                                    , method_name, __LINE__
                                    , filename, err, strerror(err) );
                    }
                }
                else
                {
                    char buf[MON_STRING_BUF_SIZE];
                    sprintf( buf
                           , "[%s], open error for: "
                             "file=%s, errno=%d (%s)\n"
                           , method_name, filename, err, strerror(err) );
                    mon_log_write(MON_REDIR_STDOUT_1, SQ_LOG_ERR, buf);
                }
            }
            else
            {
                // Retain file name.  Might be needed in case of error on file.
                filename_ = filename;
    
                Redirector.addToMap(fd_, this);
                if (trace_settings & TRACE_REDIRECTION)
                {
                    trace_printf("%s@%d opened %s fd=%d.  Added to fdMap.\n",
                                 method_name, __LINE__, filename, fd_);
                }
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf( buf
                   , "[%s], unable to obtain file info for stdout file"
                     ", file=%s, errno=%d (%s)\n"
                   , method_name, filename, err, strerror(err) );
            mon_log_write(MON_REDIR_STDOUT_2, SQ_LOG_ERR, buf);
        }
    }

    TRACE_EXIT;
}

CRedirectStdout::~CRedirectStdout()
{
    const char method_name[] = "CRedirectStdout::~CRedirectStdout";
    TRACE_ENTRY;

    if (trace_settings & TRACE_REDIRECTION)
    {
        trace_printf("%s@%d closing stdout fd=%d.\n",
                     method_name, __LINE__, fd_);
    }

    if ((fd_ != -1) && close(fd_))
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], file %s close(%d) error, %s.\n", method_name,
                filename_.c_str(), fd_, strerror(errno));
        mon_log_write(MON_REDIR_USTDOUT_1, SQ_LOG_ERR, buf);
    }

    Redirector.delFromMap(fd_);

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rede", 4);

    TRACE_EXIT;
}

void CRedirectStdout::handleHangup()
{
    const char method_name[] = "CRedirectStdout::handleHangup";
    TRACE_ENTRY;

    TRACE_EXIT;
}

int CRedirectStdout::handleInput()
{
    const char method_name[] = "CRedirectStdout::handleInput";
    TRACE_ENTRY;

    activity_ = true;

    // The file descriptor fd_ is now available for handling additional writes

    // Write as much deferred data as possible to output file descriptor
    deferredData_t deferredData;
    while ( !deferredDataList_.empty() )
    {
        deferredData = deferredDataList_.front();

        if (write(fd_, deferredData.data, deferredData.count) == -1)
        {
            if ( errno != EAGAIN && ! (errno == EFBIG && fileTooLarge_))
            {
                char buf[MON_STRING_BUF_SIZE];
                sprintf(buf, "[%s], write to file %s (fd=%d), %s\n",
                        method_name, filename_.c_str(), fd_, strerror(errno));
                mon_log_write(MON_REDIR_STDOUT_HNDLOUT_1, SQ_LOG_ERR, buf);

                if ( errno == EFBIG )
                {  // Could not write to the file because max file
                   // size exceeded.  Remember that this happened
                   // and don't attempt to write more of these
                   // errors to the file.  Otherwise the monitor's
                   // log file might fill up with "file too large
                   // errors".
                    fileTooLarge_ = true;
                }
            }

            // Will write more data when output file is ready 
            break;
        }
        else
        {   // Successfully wrote deferred output data.
            // Delete data buffer for just-written data
            delete [] deferredData.data;
            // Discard deferred data item from list
            deferredDataList_.pop_front();
        }
    }

    if (deferredDataList_.size() < 5)
    {
        // Restore source file descriptor to epoll set to allow more
        // data to arrive.
        Redirector.addToEpollSet(sourceFd_, EPOLLIN, "stdout");
    }

    if (deferredDataList_.empty())
    {
        // Backlog of saved data is cleared out.  Can go back to normal
        // output handling.
        Redirector.delFromEpollSet(fd_);
        cantWrite_ = false;
    }

    TRACE_EXIT;

    return 0;
}

void CRedirectStdout::saveData(ssize_t count, char *buffer)
{
    const char method_name[] = "CRedirectStdout::saveData";
    TRACE_ENTRY;

    deferredData_t deferredData;
    deferredData.count = count;
    deferredData.data = new char [count];
    memcpy(deferredData.data, buffer, count);
    
    deferredDataList_.push_back( deferredData );

    if (trace_settings & TRACE_REDIRECTION)
    {
        trace_printf("%s@%d can't write to fd=%d, saving %d bytes, list "
                     "size=%d\n", method_name, __LINE__,
                     fd_, (int) count, (int)deferredDataList_.size());
    }

    if (deferredDataList_.size() > 5)
    {   // Accumulated too much data, remove source fd from epoll list
        // so we don't get any more for now.
        Redirector.delFromEpollSet(sourceFd_);
    }

    TRACE_EXIT;
}

// Write output data to redirected standard out file
void CRedirectStdout::handleOutput(ssize_t count, char *buffer)
{
    const char method_name[] = "CRedirectStdout::handleOutput";
    TRACE_ENTRY;

    activity_ = true;

    if (fd_ != -1)
    {
        if ( cantWrite_ )
        {
            // Retain data for writing later when file descriptor
            // is available.
            saveData(count, buffer);
        }
        else
        {
            if (write(fd_, buffer, count) == -1)
            {
                if ( errno == EAGAIN)
                {   // Writing would cause blocking

                    // Want event when file descriptor is ready for output
                    Redirector.addToEpollSet(fd_, EPOLLOUT, "stdout");
                    cantWrite_ = true;

                    // Retain data for writing later when file descriptor
                    // is available.
                    saveData(count, buffer);
                }
                else if ( ! (errno == EFBIG && fileTooLarge_) )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    sprintf(buf, "[%s], write to file %s (fd=%d), %s\n",
                            method_name, filename_.c_str(), fd_,
                            strerror(errno));
                    mon_log_write(MON_REDIR_STDOUT_HNDLOUT_1,
                                  SQ_LOG_ERR, buf);

                    if ( errno == EFBIG )
                    {  // Could not write to the file because max file
                       // size exceeded.  Remember that this happened
                       // and don't attempt to write more of these
                       // errors to the file.  Otherwise the monitor's
                       // log file might fill up with "file too large
                       // errors".
                       fileTooLarge_ = true;
                    }
                }
            }
        }
    }

    TRACE_EXIT;
}

CRedirectAncestorStdout::CRedirectAncestorStdout(int nid, int pid, int ancestor_nid, int ancestor_pid)
    : CRedirect(nid, pid), ancestor_nid_(ancestor_nid), ancestor_pid_(ancestor_pid)
{
    const char method_name[] = "CRedirectAncestorStdout::CRedirectAncestorStdout";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "REDF", 4);

    TRACE_EXIT;
}

CRedirectAncestorStdout::~CRedirectAncestorStdout()
{
    const char method_name[] = "CRedirectAncestorStdout::~CRedirectAncestorStdout";
    TRACE_ENTRY;

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "redf", 4);

    TRACE_EXIT;
}

void CRedirectAncestorStdout::handleHangup()
{
    const char method_name[] = "CRedirectAncestorStdout::handleHangup";
    TRACE_ENTRY;

    TRACE_EXIT;
}

int CRedirectAncestorStdout::handleInput()
{
    const char method_name[] = "CRedirectAncestorStdout::handleInput";
    TRACE_ENTRY;

    activity_ = true;

    TRACE_EXIT;

    return 0;
}

void CRedirectAncestorStdout::handleOutput(ssize_t count, char *buffer)
{
    const char method_name[] = "CRedirectAncestorStdout::handleOutput";
    TRACE_ENTRY;

    activity_ = true;

    if (trace_settings & TRACE_REDIRECTION)
    {
        trace_printf("%s@%d adding buffer to list for (%d, %d), %d bytes\n",
                     method_name, __LINE__, ancestor_nid_, ancestor_pid_,
                     (int)count);
    }

    if (NameServerEnabled)
    {
        PtpClient->ProcessStdIoData( ancestor_nid_
                                   , ancestor_pid_
                                   , STDOUT_DATA
                                   , count
                                   , buffer );
    }
    else
    {
        CReplStdioData *repl = new CReplStdioData( ancestor_nid_
                                                 , ancestor_pid_
                                                 , STDOUT_DATA
                                                 , count
                                                 , buffer );
        Replicator.addItem(repl);
    }

    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->StdioDataReplIncr();


    TRACE_EXIT;
}

CRedirectStderr::CRedirectStderr(const char *nodeName, const char *processName,
                                 int nid, int pid)
                : CRedirect(nodeName, processName, nid, pid)
                , header_count_(0)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "REDG", 4);

    // Calculate STDERR header string size
    char buf[MON_STRING_BUF_SIZE];
    header_count_ = sprintf(buf, "STDERR redirected from %s.%s.%d.%d: "
                               , nodeName, processName, nid, pid);
}

CRedirectStderr::~CRedirectStderr()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "redg", 4);
}


void CRedirectStderr::handleHangup()
{
    // Stderr pipe broken, an indication that process died.
    // Act as if got a signal indicating child death (SIGCHLD).

    const char method_name[] = "CRedirectStderr::handleHangup";
    TRACE_ENTRY;

    CProcess *process = NULL;

    if ( !MyNode->IsKillingNode() )
    {
        process = MyNode->GetProcess ( pid_ );
    }

    if ( process )
    { // Save the pid/verifier to cleanup LIO buffers
        SQ_theLocalIOToClient->addToVerifierMap( process->GetPid()
                                               , process->GetVerifier() );
    }

    if ( process && process->IsAttached() )
    {
        if (trace_settings & (TRACE_PROCESS | TRACE_REDIRECTION))
            trace_printf("%s@%d Detected broken stderr pipe for attached "
                         "process, pid=%d\n", method_name, __LINE__, pid_);

        // Verify that process is gone.  If not, kill it.
        char filepath[30];
        sprintf (filepath, "/proc/%d/cmdline", pid_);
        FILE * cl = fopen(filepath, "r");
        if (cl != NULL)
        {   // Check process command line, if non-zero then process still exists
            char cmdline[50];
            cmdline[0] = '\0';
            size_t cl_len = fread(cmdline, 1, sizeof(cmdline), cl);
            fclose(cl);

            if (cl_len != 0)
            {
                    if (kill(pid_, SIGKILL))
                    {
                        char buf[MON_STRING_BUF_SIZE];
                        sprintf(buf, "[%s], Killing process, pid=%d, %s\n",
                                method_name, pid_, strerror(errno));
                        mon_log_write(MON_REDIR_KILL_ERR, SQ_LOG_ERR, buf);
                        if (trace_settings & (TRACE_PROCESS | TRACE_REDIRECTION))
                            trace_printf("%s@%d - Completed kill for pid=%d\n", method_name, __LINE__, pid_);
                    }
            }
        }

        // Child death signal is not delivered on an 'attached' process
        // since it is not created by the monitor, so queue 
        // process termination request since stderr pipe is broken,
        // and add the pid to the dead pids list which is used to process 
        // the verifier map entries.
        SQ_theLocalIOToClient->handleDeadPid(pid_);
    }
    else if ( process )
    {
        if ( process->GetState() != State_Down && !process->IsAbended() )
        {
            process->SetAbended( true );
        }
        if (trace_settings & (TRACE_PROCESS | TRACE_REDIRECTION))
            trace_printf( "%s@%d Detected broken stderr pipe for child "
                          "process %s(%d,%d:%d), state=%s, IsAbended=%s; "
                          "waiting for child death signal\n"
                        , method_name, __LINE__
                        , process->GetName()
                        , process->GetNid()
                        , process->GetPid()
                        , process->GetVerifier()
                        , StateString(process->GetState())
                        , process->IsAbended()?"TRUE":"FALSE" );
        process->SetHangupTime ();
        MyNode->PidHangupSet ( pid_ );
    }
    else
    {
        if (trace_settings & (TRACE_PROCESS | TRACE_REDIRECTION))
            trace_printf("%s@%d Detected broken stderr pipe for child "
                         "process, pid=%d; could not locate process object\n",
                         method_name, __LINE__, pid_);
    }

    TRACE_EXIT;
}

int CRedirectStderr::handleInput()
{
    const char method_name[] = "CRedirectStderr::handleInput";
    TRACE_ENTRY;

    activity_ = true;

    TRACE_EXIT;

    return 0;
}

// Write output data to redirected standard error file
void CRedirectStderr::handleOutput(ssize_t count, char *buffer)
{
    const char method_name[] = "CRedirectStderr::handleOutput";
    TRACE_ENTRY;
    
    ssize_t buf_size = header_count_+count+2;
    char *buf = new char[buf_size];
    if ( buf )
    {
        memset(buf, 0, buf_size);
        // Copy up to MON_EVENT_BUF_SIZE
        ssize_t size = snprintf( buf
                               , (buf_size<MON_EVENT_BUF_SIZE)?buf_size:MON_EVENT_BUF_SIZE
                               , "STDERR redirected from %s.%s.%d.%d: %s"
                               ,  nodeName(), processName(), nid(), pid(), buffer );
        if ( size > 0 )
        {
            if (size >= MON_EVENT_BUF_SIZE )
            { // truncated
                buf[MON_EVENT_BUF_SIZE-2] = '\n';
                buf[MON_EVENT_BUF_SIZE-1] = 0;
            }
            else if ( buf[size-1] != '\n')
            {
                buf[size-1] = '\n';
            }
        }
        mon_log_write(MON_REDIR_STDERR, SQ_LOG_DEBUG, buf);

        delete [] buf;
    }
    
    activity_ = true;
    
    TRACE_EXIT;
}

CRedirector::CRedirector(): thread_id_(0), shutdown_(false)
{
    const char method_name[] = "CRedirector::CRedirector";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RDTR", 4);

    // Create epoll file descriptor for use in redirecting i/o from
    // child proceses.
    epoll_fd_ = epoll_create(MAX_EPOLL_FDS);
    if (epoll_fd_ == -1)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], epoll_create error, %s\n",
                method_name, strerror(errno));
        mon_log_write(MON_REDIR_REDIR_1, SQ_LOG_ERR, buf);

        // Fatal error if cannot epoll_create
        shutdown_ = true;
    }
    else
    {
        // The epoll file descriptor will be closed on exec of a new process
        // (i.e. do not propagate this file descriptor to a child process.)
        if (fcntl(epoll_fd_, F_SETFD, FD_CLOEXEC))
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], fcntl error, %s\n",
                    method_name, strerror(errno));
            mon_log_write(MON_REDIR_REDIR_2, SQ_LOG_ERR, buf);
        }
    }

    TRACE_EXIT;
}

CRedirector::~CRedirector()
{
    const char method_name[] = "CRedirector::~CRedirector";
    TRACE_ENTRY;

    if (close(epoll_fd_) == -1)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], error when closing epoll fd, %s\n",
                method_name, strerror(errno));
        mon_log_write(MON_REDIR_UREDIR_1, SQ_LOG_ERR, buf);
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rdtr", 4);

    TRACE_EXIT;
}





void CRedirector::disposeIoData(int fd, int count, char *buffer)
{
    const char method_name[] = "CRedirector::disposeIoData";
    TRACE_ENTRY;

    // Locate the redirect object associated with the fd
    fdToRedirect_t::iterator iter;
    CRedirect *redirect = NULL;
    fdMapLock_.lock();
    iter = fdMap_.find(fd);
    if( iter != fdMap_.end() ) 
    {
        redirect = iter->second;

        // bugcatcher, temp call
        redirect->validateObj();

        if (count != 0)
            redirect->handleOutput(count, buffer);
        else if (redirect->idle())
        {   // Supplier indicates no more data available.
            delFromEpollSet(fd);

            if (trace_settings & TRACE_REDIRECTION)
                trace_printf("%s@%d supplier indicates end-of-file for fd=%d\n",
                             method_name, __LINE__, fd);

        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], fd=%d not found in map\n", method_name, fd);
        mon_log_write(MON_REDIR_DISPOSEIODATA_1, SQ_LOG_ERR, buf);
    }
    fdMapLock_.unlock();


    TRACE_EXIT;
}

// Add the file descriptor to the set monitored by epoll.  That
// will allow epoll_wait to return an indication when data are available.
void CRedirector::addToEpollSet(int fd, int epoll_events, const char * type)
{
    const char method_name[] = "CRedirector::addToEpollSet";
    TRACE_ENTRY;

    // Add file descriptor to epoll set
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = epoll_events;
    ev.data.fd = fd;

    if ((epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1)
        && (errno != EEXIST))
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], epoll_ctl error, adding %s fd=%d, %s\n",
                method_name, type, fd, strerror(errno));
        mon_log_write(MON_REDIR_ADDTOEPOLL_1, SQ_LOG_ERR, buf);
    }

    if (trace_settings & TRACE_REDIRECTION)
        trace_printf("%s@%d added %s fd=%d to list of epoll monitored fds\n",
                     method_name, __LINE__, type, fd);

    TRACE_EXIT;
}

void CRedirector::delFromEpollSet(int fd)
{
    const char method_name[] = "CRedirector::delFromEpollSet";
    TRACE_ENTRY;

    if ((epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) == -1)
        && !(errno == EBADF || errno == ENOENT || errno == EPERM))
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], epoll_ctl delete error, fd=%d, %s\n",
                method_name, fd, strerror(errno));
        mon_log_write(MON_REDIR_DELFROMEPOLL_1, SQ_LOG_ERR, buf);
    }

    if (trace_settings & TRACE_REDIRECTION)
        trace_printf("%s@%d deleted fd=%d from list of epoll monitored fds\n",
                     method_name, __LINE__, fd);

    TRACE_EXIT;
}

void CRedirector::addToMap(int fd, CRedirect * redirect)
{
    const char method_name[] = "CRedirector::addToMap";
    TRACE_ENTRY;

    fdMapLock_.lock();
    fdMap_.insert(std::make_pair(fd, redirect));
    fdMapLock_.unlock();

    TRACE_EXIT;
}

void CRedirector::delFromMap(int fd)
{
    const char method_name[] = "CRedirector::delFromMap";
    TRACE_ENTRY;

    fdMapLock_.lock();
    fdMap_.erase(fd);
    fdMapLock_.unlock();

    TRACE_EXIT;
}

void CRedirector::stdinFd(int nid, int pid, int &pipeFd, char filename[],
                          int ancestor_nid, int ancestor_pid)
{
    const char method_name[] = "CRedirector::stdinFd";
    TRACE_ENTRY;

    CRedirect *redirect;

    if (filename[0])
    {   // stdin source file/device is on this node
        struct stat statbuf;
        if (stat(filename, &statbuf) == -1)
        {
            int err = errno;
            if (err != ENOENT)
            {
                char buf[MON_STRING_BUF_SIZE];
                sprintf(buf, "[%s], unable to obtain file info for stdin file"
                        ", file=%s, errno=%d (%s). Closing stdin pipe fd=%d\n",
                        method_name, filename, err, strerror(err), pipeFd );
                mon_log_write(MON_REDIR_STDIN_FD_1, SQ_LOG_ERR, buf);
            }
            delFromMap( pipeFd );
            close( pipeFd );
            pipeFd = -1;
        }
        else
        {
            if (S_ISCHR(statbuf.st_mode))
            {   // Character device
// For now tty is unsupported due to problems with 
// when multiple readers with outstanding reads.
#ifdef STDIN_TTY
                redirect = new CRedirectStdinTty(nid, pid, filename, pipeFd);
                int sourceFd = redirect->fd();

                if (sourceFd != -1)
                {
                    if (trace_settings & TRACE_REDIRECTION)
                        trace_printf("%s@%d adding stdin fd=%d to list of "
                                     "monitored fds.  Input source from %s\n",
                                     method_name, __LINE__, sourceFd, filename);

                    // Create a mapping between the pipe file descriptor
                    // and an object that will handle input data.
                    fdMapLock_.lock();
                    fdMap_.insert(std::make_pair(sourceFd, redirect));
                    fdMap_.insert(std::make_pair(pipeFd, redirect));
                    fdMapLock_.unlock();

                    addToEpollSet(sourceFd, EPOLLIN, "stdin");
                }
                else
                {
                    delete redirect;
                }
#else
                if (trace_settings & TRACE_REDIRECTION)
                    trace_printf("%s@%d tty stdin unsupported, file=%s. "
                                 "Closing stdin pipe fd=%d\n",
                                 method_name, __LINE__, filename, pipeFd);
                close ( pipeFd );
                pipeFd = -1;
#endif
            }
            else if (S_ISREG(statbuf.st_mode))
            {   // Regular file
                redirect = new CRedirectStdinFile(nid, pid, filename, pipeFd);
                int sourceFd = redirect->fd();

                if (sourceFd != -1)
                {
                    // Create a mapping between the pipe file descriptor
                    // and an object that will handle input data.
                    fdMapLock_.lock();
                    fdMap_.insert(std::make_pair(pipeFd, redirect));
                    fdMapLock_.unlock();

                    addToEpollSet(pipeFd, EPOLLOUT, "stdin");
                }
                else
                {
                    if (trace_settings & TRACE_REDIRECTION)
                        trace_printf("%s@%d Unable to use stdin file=%s."
                                     "  Closing stdin pipe fd=%d\n",
                                     method_name, __LINE__, filename, pipeFd);
                    close ( pipeFd );
                    pipeFd = -1;
                    delete redirect;
                }
            }
            else
            {   
                // Don't know how to handle this stdin file type
                char buf[MON_STRING_BUF_SIZE];
                sprintf(buf, "[%s], unsupported stdin file type, file=%s\n",
                        method_name, filename);
                mon_log_write(MON_REDIR_STDIN_FD_2, SQ_LOG_ERR, buf);

                if (trace_settings & TRACE_REDIRECTION)
                    trace_printf("%s@%d Unable to handle stdin file=%s."
                                 "  Closing stdin pipe fd=%d\n",
                                 method_name, __LINE__, filename, pipeFd);
                close ( pipeFd );
                pipeFd = -1;
            }
        }
    }
    else
    {  // stdin source file/device is on another node

        if (trace_settings & TRACE_REDIRECTION)
            trace_printf("%s@%d adding stdin fd=%d to list of monitored "
                         "pipes. Input source from Nid=%d Pid=%d\n",
                         method_name, __LINE__, pipeFd, ancestor_nid,
                         ancestor_pid);

        redirect = new CRedirectAncestorStdin(nid, pid, pipeFd,
                                              ancestor_nid, ancestor_pid);

        // Create a mapping between the pipe file descriptor
        // and an object that will handle input data.
        fdMapLock_.lock();
        fdMap_.insert(std::make_pair(pipeFd, redirect));
        fdMapLock_.unlock();

        if (NameServerEnabled)
        {
            PtpClient->ProcessStdInReq( nid
                                      , pid
                                      , STDIN_REQ_DATA
                                      , ancestor_nid
                                      , ancestor_pid );
        }
        else
        {
            CReplStdinReq *repl = new CReplStdinReq( nid
                                                   , pid
                                                   , STDIN_REQ_DATA
                                                   , ancestor_nid
                                                   , ancestor_pid );
            Replicator.addItem(repl);
        }
    }

    TRACE_EXIT;
}

void CRedirector::stdoutFd(int nid, int pid, int fd, const char *filename,
                           int ancestor_nid, int ancestor_pid)
{
    const char method_name[] = "CRedirector::stdoutFd";
    TRACE_ENTRY;

    // Create a mapping between the file descriptor and an object that
    // will handle output data.
    CRedirect *redirect;
    if (filename[0])
    {
        redirect = new CRedirectStdout(nid, pid, filename, fd);
        if (trace_settings & TRACE_REDIRECTION)
            trace_printf("%s@%d adding stdout fd=%d to list of monitored "
                         "pipes.  Redirection to %s\n",
                         method_name, __LINE__, fd, filename);
    }
    else
    {
        redirect = new CRedirectAncestorStdout(nid, pid, ancestor_nid, ancestor_pid);
        if (trace_settings & TRACE_REDIRECTION)
            trace_printf("%s@%d adding stdout fd=%d to list of monitored "
                         "pipes. Redirection to Nid=%d Pid=%d\n",
                         method_name, __LINE__, fd, ancestor_nid, ancestor_pid);
    }

    // Create a mapping between the pipe file descriptor
    // and an object that will handle input data.
    fdMapLock_.lock();
    fdMap_.insert(std::make_pair(fd, redirect));
    fdMapLock_.unlock();

    addToEpollSet(fd, EPOLLIN, "stdout");

    TRACE_EXIT;
}

void CRedirector::stderrFd(const char *nodeName, const char *processName, 
                           int nid, int pid, int fd)
{
    const char method_name[] = "CRedirector::stderrFd";
    TRACE_ENTRY;

    // Create a mapping between the file descriptor and an object that
    // will handle output data.
    CRedirectStderr *redirect;
    redirect = new CRedirectStderr(nodeName, processName, nid, pid);

    if (trace_settings & TRACE_REDIRECTION)
        trace_printf("%s@%d adding stderr fd=%d to list of monitored "
                     "pipes. Redirection to %s (%d,%d)\n",
                     method_name, __LINE__, fd, processName, nid, pid);

    // Create a mapping between the pipe file descriptor
    // and an object that will handle input data.
    fdMapLock_.lock();
    fdMap_.insert(std::make_pair(fd, redirect));
    fdMapLock_.unlock();

    addToEpollSet(fd, EPOLLIN, "stderr");

    TRACE_EXIT;
}

int CRedirector::stdinRemote(const char *filename, int requesterNid, int requesterPid)
{
    const char method_name[] = "CRedirector::stdinRemote";
    TRACE_ENTRY;

    CRedirectStdinRemote *redirect;
    redirect = new CRedirectStdinRemote(filename, requesterNid, requesterPid);
    int sourceFd = redirect->fd();

    if (sourceFd != -1)
    {
        // Create a mapping between the stdin file descriptor
        // and an object that will handle input data.
        fdMapLock_.lock();
        fdMap_.insert(std::make_pair(sourceFd, redirect));
        fdMapLock_.unlock();

        if (redirect->isFile())
        {
            // Read data from file and send to requester
            redirect->handleInput();
        }
        else
        {
            if (trace_settings & TRACE_REDIRECTION)
                trace_printf("%s@%d adding stdin fd=%d to list of "
                             "monitored fds.  Input source from %s\n",
                             method_name, __LINE__, sourceFd, filename);

            addToEpollSet(sourceFd, EPOLLIN, "stdin");
        }
    }
    else
    {
        delete redirect;
    }

    TRACE_EXIT;

    return sourceFd;
}

void CRedirector::stdinOff(int fd)
{
    const char method_name[] = "CRedirector::stdinOff";
    TRACE_ENTRY;

    delFromEpollSet(fd);

    TRACE_EXIT;
}

void CRedirector::stdinOn(int fd)
{
    const char method_name[] = "CRedirector::stdinOn";
    TRACE_ENTRY;

    // Locate the redirect object associated with the fd
    fdToRedirect_t::iterator iter;
    CRedirectStdinRemote *redirect;

    fdMapLock_.lock();
    iter = fdMap_.find(fd);
    if( iter != fdMap_.end() ) 
    {
        redirect = dynamic_cast< CRedirectStdinRemote*> (iter->second);

        if (redirect)
        {
            // bugcatcher, temp call
            redirect->validateObj();

            if (redirect->isFile())
            {
                // Regular file: read data from file and send to requester
                redirect->handleInput();
            }
            else
            {   // Character device: enable device ready events
                addToEpollSet(fd, EPOLLIN, "stdin");
            }
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], fd=%d not found in map\n", method_name, fd);
        mon_log_write(MON_REDIR_STDINON_1, SQ_LOG_ERR, buf);
    }
    fdMapLock_.unlock();


    TRACE_EXIT;
}

void CRedirector::tryShutdownPipeFd(int pid, int fd, bool pv_delete_redirect)
{
    const char method_name[] = "CRedirector::tryShutdownPipeFd";
    TRACE_ENTRY;

    if (trace_settings & TRACE_REDIRECTION)
        trace_printf("%s@%d method invoked, pid=%d, fd=%d\n",
                     method_name, __LINE__, pid, fd);

    // Locate the redirect object associated with the fd
    fdToRedirect_t::iterator iter;
    CRedirect *redirect = NULL;

    fdMapLock_.lock();
    iter = fdMap_.find(fd);
    if( iter != fdMap_.end() ) 
    {
        redirect = iter->second;

        // bugcatcher, temp call
        if (redirect->pid() != 0)
            redirect->validateObj();

        if (((pv_delete_redirect) ||
             (!redirect->active())) &&
            (pid == redirect->pid()))
        {
            if (trace_settings & TRACE_REDIRECTION)
                trace_printf("%s@%d invoking shutdownPipeFd for fd=%d\n",
                             method_name, __LINE__, fd);

            // Close down the pipe
            shutdownPipeFd(fd);

            // Delete the assocated redirect object (unless previously deleted)
            if (redirect->pid() != 0)
                delete redirect;
        }
    }
    fdMapLock_.unlock();

    TRACE_EXIT;
}

void CRedirector::shutdownPipeFd(int fd)
{
    const char method_name[] = "CRedirector::shutdownPipeFd";
    TRACE_ENTRY;

    // Remove the map entry
    if (fdMap_.erase(fd) != 0)
    {
        // Remove from list of monitored file descriptors
        delFromEpollSet(fd);

        // Close the pipe
        if (close(fd) && errno != EBADF)
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], close(%d) error, %s.\n",
                    method_name, fd, strerror(errno));
            mon_log_write(MON_REDIR_SHUTPIPE_FD_1, SQ_LOG_ERR, buf);
        }
    }

    TRACE_EXIT;
}

void CRedirector::shutdownWork(void)
{
    int rc;

    const char method_name[] = "CRedirector::shutdownWork";
    TRACE_ENTRY;

    // Set flag that tells the redirector thread to exit
    shutdown_ = true;   

    // Signal the redirector thread so it will wake up and exit
    if ((rc = pthread_kill(Redirector.tid(), SIGUSR1)) != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], pthread_kill error=%d\n", method_name, rc);
        mon_log_write(MON_REDIR_SHUTDOWNWORK, SQ_LOG_ERR, buf);
    }
    else
    {
        if (trace_settings & TRACE_REDIRECTION)
            trace_printf("%s@%d waiting for thread=%lx to exit.\n",
                         method_name, __LINE__, Redirector.tid());
        // Wait for redirector thread to exit
        pthread_join(Redirector.tid(), NULL);
    }

    TRACE_EXIT;
}

void sigusr1_signal_handler (int , siginfo_t *, void *)
{
    const char method_name[] = "sigusr1_signal_handler";
    TRACE_ENTRY;

    // No processing here.   Signal is used to cause the redirectThread
    // to return from epoll_wait

    TRACE_EXIT;
}

void CRedirector::redirectThread()
{
    int ready_fds;
    int fd;
    __uint32_t events;
    struct epoll_event event_list[MAX_EPOLL_EVENTS];
    char buffer[MAX_SYNC_DATA];
    ssize_t count;

    const char method_name[] = "CRedirector::redirectThread";
    TRACE_ENTRY;

    // Set sigaction such that SIGUSR1 signal is caught.  We use this
    // to detect that main thread is shutting us down.
    struct sigaction act;
    act.sa_sigaction = sigusr1_signal_handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset (&act.sa_mask);
    sigaddset (&act.sa_mask, SIGUSR1);
    sigaction (SIGUSR1, &act, NULL);

    while(true)
    {
        // Wait for activity on monitored file descriptors
        ready_fds = epoll_wait(epoll_fd_, event_list, MAX_EPOLL_EVENTS, -1);

        if (shutdown_)
        {   // We are being notified to exit.
            break;
        }

        if (ready_fds == -1)
        {
            if ( errno != EINTR )
            {
                char buf[MON_STRING_BUF_SIZE];
                sprintf(buf, "[%s], epoll_wait error, %s\n",
                        method_name, strerror(errno));
                mon_log_write(MON_REDIRECT_TH_1, SQ_LOG_ERR, buf);
            }
        }
        else if (ready_fds != 0)
        {
            if (trace_settings & TRACE_REDIRECTION)
                trace_printf("%s@%d ready_fds=%d\n",
                             method_name, __LINE__, ready_fds);

            // Take the appropriate action for each of the file
            // descriptors that is ready.
            for (int n=0; n < ready_fds; n++)
            {
                fd = event_list[n].data.fd;
                events = event_list[n].events;

                if (trace_settings & TRACE_REDIRECTION)
                    trace_printf("%s@%d for fd=%d, events=%d %s\n",
                                 method_name, __LINE__, fd, events, 
                                 EpollEventString(events));

                // Acquire lock to prevent memory modifications during
                // fork/exec (see uses of OFED_MUTEX define)
                MemModLock.lock();

                // Locate the redirect object associated with the fd.
                // Acquire lock and hold it for the duration of
                // handling the event.
                fdToRedirect_t::iterator iter;
                CRedirect *redirect = NULL;
                fdMapLock_.lock();
                iter = fdMap_.find(fd);
                if( iter != fdMap_.end() ) 
                {
                    redirect = iter->second;

                    // bugcatcher, temp call
                    redirect->validateObj();
                }
                // else fd was already deleted

                if (events & (EPOLLIN | EPOLLPRI))
                {
                    // File descriptor available for read operation
                    count = read(fd, buffer, sizeof(buffer));
                    if (count == 0)
                    {
                    }
                    else if (count == -1)
                    {
                        if (errno != EAGAIN)
                        {
                            char buf[MON_STRING_BUF_SIZE];
                            sprintf(buf, "[%s], error reading from fd=%d, "
                                    "errno=%d, %s\n", method_name, fd, 
                                    errno, strerror(errno));
                            mon_log_write(MON_REDIRECT_TH_3, SQ_LOG_ERR, buf);
                        }
                        else
                        {   // Non-blocking I/O and no data was
                            // available for reading.
                            if (trace_settings & TRACE_REDIRECTION)
                                trace_printf("%s@%d for fd=%d read returned "
                                             "no data (EAGAIN)\n",
                                             method_name, __LINE__, fd);
                            sched_yield();
                        }
                    }
                    else
                    {
                        if ((size_t) count < sizeof(buffer)) //buffer overflow
                            buffer[count] = '\0';
                        if (redirect != NULL)
                            redirect->handleOutput(count, buffer);
                    }
                }

                if (events & EPOLLOUT)
                {
                    // Take appropriate action based on redirect object type
                    if (redirect != NULL)
                    {
                        if (redirect->handleInput())
                        {
                            delFromEpollSet(fd);

                            if (trace_settings & TRACE_REDIRECTION)
                                trace_printf("%s@%d deleted fd=%d from epoll "
                                             "set\n",
                                             method_name, __LINE__, fd);

                        }
                    }
                }

                if (events & EPOLLHUP)
                {
                    // Other end of pipe closed, assume process died
                    if (trace_settings & TRACE_REDIRECTION)
                        trace_printf("%s@%d detected hang-up on pipe, fd=%d, EPOLLERR=%d\n",
                                     method_name, __LINE__, fd, (events & EPOLLERR));

                    // Close down the pipe
                    shutdownPipeFd(fd);

                    // Take appropriate action based on redirect object type
                    if (redirect != NULL && (redirect->pid() != 0))
                    {
                        // stdout can have multiple fds in epoll
                        if (!redirect->ignoreFdOnHangup(fd))
                        {
                            redirect->handleHangup();

                            // Delete the assocated redirect object
                            if (redirect->pid() != 0)
                                delete redirect;
                        }
                    }
                    // else process object was already deleted by child death processing
                }
                else if (events & EPOLLERR)
                {
                    char buf[MON_STRING_BUF_SIZE];
                    sprintf(buf, "[%s], I/O error on pipe, fd=%d.  Removing "
                            "fd from further I/O operations.\n",
                            method_name, fd);
                    mon_log_write(MON_REDIRECT_TH_5, SQ_LOG_ERR, buf);

                    // Remove from list of monitored file descriptors
                    delFromEpollSet(fd);
                }
                else if (events & ~(EPOLLIN | EPOLLPRI | EPOLLHUP | EPOLLERR | EPOLLOUT))
                {
                    char buf[MON_STRING_BUF_SIZE];
                    sprintf(buf, "[%s], unexpected condition, fd=%d, events=%d"
                            "\n", method_name, fd, events);
                    mon_log_write(MON_REDIRECT_TH_6, SQ_LOG_ERR, buf);
                }

                // Release the lock since finished handling the event.
                fdMapLock_.unlock();

                // Release lock to prevent memory modifications during
                // fork/exec (see uses of OFED_MUTEX define)
                MemModLock.unlock();
            }
        }
    }

    pthread_exit(0);
}

static void *redirect(void *arg)
{
    const char method_name[] = "redirect";
    TRACE_ENTRY;

    // Parameter passed to the thread is an instance of the CRedirector object
    CRedirector *rdo = (CRedirector *) arg;

    // Mask all allowed signals except SIGUSR1 which is used for shutdown
    sigset_t              mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);
    sigdelset(&mask, SIGPROF); // allows profiling such as google profiler
#ifdef USE_FORK_SUSPEND_RESUME
    sigdelset(&mask, SIGURG);
#endif // USE_FORK_SUSPEND_RESUME
    int rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], pthread_sigmask error=%d\n", method_name, rc);
        mon_log_write(MON_REDIR_REDIRECT_1, SQ_LOG_ERR, buf);
    }

    // Enter thread processing loop
    rdo->redirectThread();

    TRACE_EXIT;
    return NULL;
}

void CRedirector::start()
{
    const char method_name[] = "CRedirector::start";
    TRACE_ENTRY;

    int rc = pthread_create(&thread_id_, NULL, redirect, this);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], thread create error=%d\n", method_name, rc);
        mon_log_write(MON_REDIR_START_1, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}
#endif
