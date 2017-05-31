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

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <list>
#include <string>
#include <signal.h>
using namespace std;

#include "token.h"

typedef list<string>    ArgList_t;
typedef list<string>    LineList_t;

class CSystem : public CToken
{
public:
    CSystem( const char *command );
   ~CSystem( void );

    int ExecuteCommand( LineList_t &outList );
    int ExecuteCommand( const char *command, LineList_t &outList );
    int LaunchCommand( int &commandPid );
    int LaunchCommand( const char *command, int &commandPid );
    int WaitCommand( LineList_t &outList, int commandPid );

protected:
    string          command_;

private:
    int              commandOutFd_;
    sigset_t         oldMask_;
    ArgList_t        argList_;
    
    void  ParseCommand( void );
    void  ParseMessage( LineList_t &outList, char *message, int len );
};

typedef list<string>    OutList_t;

class CUtility : public CSystem
{
public:
    CUtility( const char *utility );
   ~CUtility( void );

    // The caller should save and close stdin before calling this proc
    // and restore it when done. This is to prevent ssh from consuming
    // caller's stdin contents when executing the command. 
    int     ExecuteCommand( const char *command );
    bool    HaveOutput( void );
    int     GetOutputLineCount( void );
    int     GetOutputSize( void );
    void    GetOutput( char *buf, int bufSize );
    
private:
    OutList_t    outputList_;
};

#endif /*SYSTEM_H_*/
