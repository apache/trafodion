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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "seabed/trace.h"
#include "montrace.h"
#include "monlogging.h"
#include "system.h"

#define MSGLEN  4096

// Class inplementation

CSystem::CSystem( const char *command )
        : command_( command )
        , commandOutFd_( -1 )
        , argList_( )
{
    const char method_name[] = "CSystem::CSystem";
    TRACE_ENTRY;

    sigemptyset( &oldMask_ );

    TRACE_EXIT;
}

CSystem::~CSystem( void )
{
    const char method_name[] = "CSystem::~CSystem";
    TRACE_ENTRY;

    TRACE_EXIT;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CSystem::ExecuteCommand
//
// Description: Executes the command string passed in the constructor
//
// Output: The outList string list will contain the stdout lines,
//         one line per string in the list which the caller can
//         then parse.
//              
// Return:
//        1 - successful child create, but command failed execution
//        0 - successful child create and command execution
//       -1 - failure
//
///////////////////////////////////////////////////////////////////////////////
int CSystem::ExecuteCommand( LineList_t &outList )
{
    const char method_name[] = "CSystem::ExecuteCommand";
    TRACE_ENTRY;

    int     rc;
    pid_t   pid;
    
    rc = LaunchCommand( pid );
    if ( rc == 0 )
    {
        rc = WaitCommand( outList, pid );
    }

    TRACE_EXIT;
    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CSystem::ExecuteCommand
//
// Description: Executes the command string passed in as is if
//              current command string is empty or appends it if not
//
// Output: The outList string list will contain the stdout lines,
//         one line per string in the list which the caller can
//         then parse.
//              
// Return:
//        1 - successful child create, but command failed execution
//        0 - successful child create and command execution
//       -1 - failure
//
///////////////////////////////////////////////////////////////////////////////
int CSystem::ExecuteCommand( const char *command, LineList_t &outList )
{
    const char method_name[] = "CSystem::ExecuteCommand";
    TRACE_ENTRY;

    string saveCommand;
    
    if ( command_.empty() )
    {
        if ( command )
        {
            command_ = command;
        }
        else
        {
            return( -1 );
        }
    }
    else
    {
        if ( command )
        {
            saveCommand = command_;
            command_ += " ";
            command_ += command;
        }
    }

    int rc = CSystem::ExecuteCommand( outList );
    
    if ( !saveCommand.empty() )
    {
        command_ = saveCommand;
    }

    TRACE_EXIT;
    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CSystem::LaunchCommand
//
// Description: Executes the command string passed in as is if
//              current command string is empty or appends it if not
//
// Output: The outList string list will contain the stdout lines,
//         one line per string in the list which the caller can
//         then parse.
//              
// Return:
//        1 - successful child create, but command failed execution
//        0 - successful child create and command execution
//       -1 - failure
//
///////////////////////////////////////////////////////////////////////////////
int CSystem::LaunchCommand( const char *command, int &commandPid )
{
    const char method_name[] = "CSystem::LaunchCommand";
    TRACE_ENTRY;

    string saveCommand;
    
    if ( command_.empty() )
    {
        if ( command )
        {
            command_ = command;
        }
        else
        {
            return( -1 );
        }
    }
    else
    {
        if ( command )
        {
            saveCommand = command_;
            command_ += " ";
            command_ += command;
        }
    }

    int rc = CSystem::LaunchCommand( commandPid );
    
    if ( !saveCommand.empty() )
    {
        command_ = saveCommand;
    }

    TRACE_EXIT;
    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CSystem::LaunchCommand
//
// Description: Executes the command string passed in the constructor
//
// Output: Returns command process pid
//              
// Return:
//        1 - successful child create, but command failed execution
//        0 - successful child create and command execution
//       -1 - failure
//
///////////////////////////////////////////////////////////////////////////////
int CSystem::LaunchCommand( int &commandPid )
{
    const char method_name[] = "CSystem::LaunchCommand";
    TRACE_ENTRY;

    int     rc;
    int     pFd[2] = { -1, -1 };
    pid_t   cPid;
    
    ParseCommand();

    struct stat stdoutStat;
    rc = fstat( STDOUT_FILENO, &stdoutStat );
    if (rc != 0)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Internal error fstat(), errno=%d(%s)\n", method_name, err, strerror(errno) );
        mon_log_write(MON_SYSTEM_LAUNCH_COMMAND_1, SQ_LOG_ERR, la_buf);
        return( -1 );
    }

    if ( pipe( pFd ) == -1 )
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Internal error while creating pipe, errno=%d(%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_SYSTEM_LAUNCH_COMMAND_1, SQ_LOG_ERR, la_buf);
        return( -1 );
    }

    sigset_t forkMask;
    sigemptyset(&forkMask);
    sigaddset(&forkMask, SIGCHLD);
    rc = pthread_sigmask(SIG_UNBLOCK, &forkMask, &oldMask_);
    if (rc != 0)
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Internal error pthread_sigmask(), errno=%d(%s)\n", method_name, rc, strerror(rc));
        mon_log_write(MON_SYSTEM_LAUNCH_COMMAND_2, SQ_LOG_ERR, la_buf);
        
        return( -1 );
    }

    cPid = fork( );
    if ( cPid == -1 )
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Internal error on fork(), errno=%d(%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_SYSTEM_LAUNCH_COMMAND_3, SQ_LOG_ERR, la_buf);
        
        return( -1 );
    }
    if ( cPid == 0 )
    {   // Child reads from pipe
        int     rc;
        int     outPFd = -1;
        unsigned int i;
        char   **newArgs = new  char *[argList_.size()+1];
        LineList_t::iterator    alit;
#if 0   // enable to debug child here
        sleep( 15 );
#endif

        for ( i = 0, alit = argList_.begin(); alit != argList_.end() ; i++, alit++ )
        {
            newArgs[i] = (char *)alit->c_str();
        }

        newArgs[i] = 0;

        // Unmask all allowed signals in the child
        sigset_t              mask;
        sigemptyset(&mask);
        rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
        if (rc != 0)
        {
            int err = errno;
            fprintf( stderr,"[%s] Error: Internal error pthread_sigmask(), errno=%d(%s)\n", newArgs[0], err, strerror(errno) );
            exit( EXIT_FAILURE );
        }

        close( STDOUT_FILENO );
        // Dup the write end of the pipe
        outPFd = dup( pFd[1] ); 
        if ( outPFd == -1 )
        {
            int err = errno;
            fprintf( stderr,"[%s] Error: Internal error on fork child dup(), errno=%d(%s)\n", newArgs[0], err, strerror(errno) );
            exit( EXIT_FAILURE );
        }
        close( pFd[0] );        // Close unused pipe read fd
        close( pFd[1] );        // Close unused pipe write fd
        
        //fprintf( stderr,"[Child - err]: execvp(%s), newArgs[1]=%s, newArgs[1]=%s\n", newArgs[0], newArgs[1], newArgs[2] );
        rc = execvp( newArgs[0], newArgs );
        if ( rc == -1 )
        {
            int err = errno;
            fprintf( stderr,"[%s] Error: Internal error on fork child execvp(), errno=%d(%s)\n", newArgs[0], err, strerror(errno) );
            exit( EXIT_FAILURE );
        }
        if ( outPFd != -1)
            close( outPFd );
        exit( EXIT_SUCCESS );
    }
    else
    {
        close( pFd[1] );        // Close unused write end of pipe
        commandOutFd_ = pFd[0]; // Save read end of pipe (child stdout)
        commandPid = cPid;      // Return the child pid
    }

    TRACE_EXIT;
    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CSystem::WaitCommand
//
// Description: Completes command initiated by LaunchCommand
//
// Output: The outList string list will contain the stdout lines,
//         one line per string in the list which the caller can
//         then parse.
//              
// Return:
//        >1 - successful child create, but command failed execution
//        0 - successful child create and command execution
//       -1 - failure
//       -2 - waitpid completed with unexpected state
//
///////////////////////////////////////////////////////////////////////////////
int CSystem::WaitCommand( LineList_t &outList, int commandPid )
{
    const char method_name[] = "CSystem::WaitCommand";
    TRACE_ENTRY;

    int     rc = -1;
    int     cStatus;
    pid_t   cPid = commandPid;
    pid_t   pid;
    
    if ( cPid > 0 )
    {   // Parent writes argv[1] to pipe
        char message[MSGLEN];
        int  msgLen;
        
        if ( commandOutFd_ != -1 )
        {
            memset( message, 0, sizeof(message) );
            // Read line from command stdout
            while ( (msgLen = read( commandOutFd_, message, sizeof(message) )) > 0 )
            {
                ParseMessage( outList, message, msgLen );
            }

            close( commandOutFd_ );
            commandOutFd_ = -1;
        }

        rc = pthread_sigmask(SIG_SETMASK, &oldMask_, NULL);
        if (rc != 0)
        {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Error: Internal error pthread_sigmask(), errno=%d(%s)\n", method_name, rc, strerror(rc));
            mon_log_write(MON_SYSTEM_WAIT_COMMAND_1, SQ_LOG_ERR, la_buf);
            
            return( -1 );
        }

        pid = waitpid( cPid, &cStatus, 0 );           // Wait for child
        if ( pid == cPid )
        {
            if ( WIFEXITED(cStatus) )
            {
                rc = WEXITSTATUS(cStatus);
            }
            else if ( WIFSIGNALED(cStatus) )
            {
                rc = WTERMSIG(cStatus);
            }
            else
            {
                rc = -2;
            }
        }
        else
        {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Error: Command failed, pid=%d (expected=%d),"
                    " errno=%d(%s)\n", method_name, pid, cPid,
                    errno, strerror(errno));
            mon_log_write(MON_SYSTEM_WAIT_COMMAND_2, SQ_LOG_ERR, la_buf);
        
            rc = -1;
        }
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Invalid pid=%d\n", method_name, cPid);
        mon_log_write(MON_SYSTEM_WAIT_COMMAND_3, SQ_LOG_ERR, la_buf);
        
        rc = -1;
    }

    TRACE_EXIT;
    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CSystem::ParseCommand
//
// Description: Parses command string passed in and
//              stores as a list of individual strings later used
//              as arguments in the ExecuteCommand() method.
//              
// Return:      None
//
///////////////////////////////////////////////////////////////////////////////
void  CSystem::ParseCommand( void )
{
    const char method_name[] = "CSystem::ParseCommand";
    TRACE_ENTRY;

    char *command = strdup(command_.c_str());
    char *cmdTail = command;
    char  delimiter;
    char  token[TOKEN_SIZE_MAX];
    int   tokenCnt = 0;

    argList_.clear();
    
    do
    {
        cmdTail = CToken::GetToken( cmdTail, token, &delimiter );
        if ( token[0] != '\0' )
        {
            argList_.push_back( token );
            tokenCnt++;
        }
    }
    while( token[0] != '\0' );
    free( command );

    TRACE_EXIT;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CSystem::ParseMessage
//
// Description: Parses the buffer returned by the command utility and
//              stores each line as a list of individual strings.
//              
// Return:      None
//
///////////////////////////////////////////////////////////////////////////////
void  CSystem::ParseMessage( LineList_t &outList, char *message, int len )
{
    const char method_name[] = "CSystem::ParseMessage";
    TRACE_ENTRY;

    char *buffer = (char *)malloc( len );
    char *tail = buffer;
    char  line[MSGLEN];
    int   lineCnt = 0;

    memcpy( buffer, message, len );
    
    do
    {
        tail = CToken::GetLine( tail, line, MSGLEN );
        if ( line[0] != '\0' )
        {
            outList.push_back( line );
            lineCnt++;
        }
    }
    while( line[0] != '\0' );
    free( buffer );

    TRACE_EXIT;
}

// Class inplementation

CUtility::CUtility( const char *utility )
         :CSystem( utility )
         ,outputList_( )
{
    const char method_name[] = "CUtility::CUtility";
    TRACE_ENTRY;

    TRACE_EXIT;
}

CUtility::~CUtility( void )
{
    const char method_name[] = "CUtility::~CUtility";
    TRACE_ENTRY;

    TRACE_EXIT;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CUtility::ExecuteCommand
//
// Description: Executes the command string passed in
//              
// Return:
//        0 - success
//        n - failure
//
///////////////////////////////////////////////////////////////////////////////
int CUtility::ExecuteCommand( const char *command )
{
    const char method_name[] = "CUtility::ExecuteCommand";
    TRACE_ENTRY;

    int rc;

    rc = CSystem::ExecuteCommand( command, outputList_ );
    if ( rc == -1 )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf( la_buf, "[%s] Error: While executing '%s'\n"
                       , method_name, command_.data() );
        mon_log_write(MON_UTILITY_EXECUTECOMMAND_1, SQ_LOG_ERR, la_buf);
    }

    TRACE_EXIT;
    return( rc );
}

bool CUtility::HaveOutput( void ) 
{ 
    return( !outputList_.empty() ); 
}

void CUtility::GetOutput( char *buf, int bufSize )
{
    const char method_name[] = "CUtility::GetOutput";
    TRACE_ENTRY;

    int bytes = 0, count = 0, size = 0;
    char *ptr = buf;
    string str;
    
    if ( !outputList_.empty() )
    {
        OutList_t::iterator    it;
        for ( it = outputList_.begin(); it != outputList_.end() && count < bufSize ; it++ )
        {
            str = *it;
            size =+ str.size();
            bytes = (size <= (bufSize - count)) ? size : (bufSize - count);
            count =+ bytes;
            memcpy( ptr, str.data(), bytes );
            for (int i=0; i<bytes ;ptr++ );
        }
    }

    TRACE_EXIT;
}

int CUtility::GetOutputLineCount( void )
{
    const char method_name[] = "CUtility::GetOutputLineCount";
    TRACE_ENTRY;

    int count = 0;
    string str;
    
    if ( !outputList_.empty() )
    {
        count = outputList_.size();
    }

    TRACE_EXIT;
    return( count );
}

int CUtility::GetOutputSize( void )
{
    const char method_name[] = "CUtility::GetOutputSize";
    TRACE_ENTRY;

    int size = 0;
    string str;
    
    if ( !outputList_.empty() )
    {
        OutList_t::iterator    it;
        for ( it = outputList_.begin(); it != outputList_.end() ; it++ )
        {
            str = *it;
            size =+ str.size();
        }
    }

    TRACE_EXIT;
    return( size );
}
