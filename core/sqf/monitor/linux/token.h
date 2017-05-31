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

#ifndef TOKEN_H_
#define TOKEN_H_

#define TOKEN_SIZE_MAX   132

class CToken
{
public:
    CToken( void );
   ~CToken( void );

    char *FindEndToken( char *cmd, int maxlen );
    char *FindEOL( char *buffer, int maxlen );
    char *FindEndOfToken( char *cmd, int maxlen );
    char *GetLine( char *buffer, char *line, int maxlen );
    char *GetToken( char *cmd, char *token, char *delimiter, int maxlen=TOKEN_SIZE_MAX );
    char *RemoveEndWhiteSpace( char *cmd );
    char *RemoveWhiteSpace( char *cmd );

private:

    char *FindDelimiter( char *cmd );
};

#endif /*TOKEN_H_*/
