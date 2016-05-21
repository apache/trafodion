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

#include <string.h>

#include "seabed/trace.h"
#include "montrace.h"
#include "token.h"

// Class inplementation

CToken::CToken( void )
{
    const char method_name[] = "CToken::CToken";
    TRACE_ENTRY;

    TRACE_EXIT;
}

CToken::~CToken( void )
{
    const char method_name[] = "CToken::~CToken";
    TRACE_ENTRY;

    TRACE_EXIT;
}

char *CToken::FindDelimiter( char *cmd )
{
    const char method_name[] = "CToken::FindDelimiter";
    TRACE_ENTRY;

    char *ptr = cmd;

    while (ptr && *ptr && *ptr != ' ' && *ptr != ',' && *ptr != '{' && *ptr != '}' && *ptr != ':')
    {
        if (*ptr == '=' )
        {
            break;
        }
        ptr++;
    }

    TRACE_EXIT;
    return ptr;
}

char *CToken::FindEndToken( char *cmd, int maxlen )
{
    const char method_name[] = "CToken::FindEndToken";
    TRACE_ENTRY;

    int length = maxlen;
    char *ptr = cmd;

    while (ptr && *ptr && *ptr != ' ' && *ptr != ',' && *ptr != '{' && *ptr != '}' && *ptr != ':')
    {
        length--;
        ptr--;
        if (length == 0)
        {
            break;
        }
    }

    TRACE_EXIT;
    return( ++ptr );
}

char *CToken::FindEOL( char *buffer, int maxlen )
{
    const char method_name[] = "CToken::FindEOL";
    TRACE_ENTRY;

    int length = 0;
    char *ptr = buffer;

    while ( ptr && *ptr && *ptr != '\n' )
    {
        length++;
        ptr++;
        if (length == maxlen)
        {
            break;
        }
    }

    TRACE_EXIT;
    return ptr;
}

char *CToken::FindEndOfToken( char *cmd, int maxlen )
{
    const char method_name[] = "CToken::FindEndOfToken";
    TRACE_ENTRY;

    int length = 0;
    char *ptr = cmd;

    while (ptr && *ptr && *ptr != ' ' && *ptr != ',' && *ptr != '{' && *ptr != '}' && *ptr != ':')
    {
        length++;
        ptr++;
        if (length == maxlen)
        {
            break;
        }
    }

    TRACE_EXIT;
    return ptr;
}

char *CToken::GetLine( char *buffer, char *line, int maxlen )
{
    const char method_name[] = "CToken::GetLine";
    TRACE_ENTRY;

    char *ptr = RemoveWhiteSpace( buffer );
    char *end = FindEOL( ptr, maxlen );

    if (*end)
    {
        *end = '\0';
        strcpy (line, ptr);
        end++;
        ptr = end;
    }
    else
    {
        strcpy( line, ptr);
        ptr = end;
    }

    TRACE_EXIT;
    return ptr;
}

char *CToken::GetToken( char *cmd, char *token, char *delimiter, int maxlen )
{
    const char method_name[] = "CToken::GetToken";
    TRACE_ENTRY;

    char *ptr = RemoveWhiteSpace( cmd );
    char *end = FindEndOfToken( ptr, maxlen );

    *delimiter = *end;
    if (*end)
    {
        *end = '\0';
        strcpy (token, ptr);
        *end = *delimiter;
        end = FindDelimiter( end );
        *delimiter = *end;
        ptr = RemoveWhiteSpace( end );
        if ( *ptr == '{' || *ptr == '}' || *ptr == ':' )
        {
            *delimiter = *ptr;
            ptr++;
            ptr = RemoveWhiteSpace( ptr );
        }
    }
    else
    {
        strcpy(token, ptr);
        ptr = end;
    }

    TRACE_EXIT;
    return ptr;
}

char *CToken::RemoveEndWhiteSpace( char *cmd )
{
    const char method_name[] = "CToken::RemoveEndWhiteSpace";
    TRACE_ENTRY;

    char *ptr = cmd;

    while (ptr && *ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == ',' || *ptr == ']'))
    {
        *ptr = '\0';
        ptr--;
    }

    TRACE_EXIT;
    return ptr;
}

char *CToken::RemoveWhiteSpace( char *cmd )
{
    const char method_name[] = "CToken::RemoveWhiteSpace";
    TRACE_ENTRY;

    char *ptr = cmd;

    while (ptr && *ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == ','))
    {
        ptr++;
    }

    TRACE_EXIT;
    return ptr;
}

