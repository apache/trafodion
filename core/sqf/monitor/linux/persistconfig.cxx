///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett Packard Enterprise Development LP
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
// 
///////////////////////////////////////////////////////////////////////////////

using namespace std;

#include <string.h>
#include "montrace.h"
#include "monlogging.h"
#include "persistconfig.h"

const char *FormatNidString( FormatNid_t type )
{
    const char *str;
    
    switch( type )
    {
        case Nid_ALL:
            str = "Nid_ALL";
            break;
        case Nid_RELATIVE:
            str = "Nid_RELATIVE";
            break;
        default:
            str = "Nid_Undefined";
    }

    return( str );
}

const char *FormatZidString( FormatZid_t type )
{
    const char *str;
    
    switch( type )
    {
        case Zid_ALL:
            str = "Zid_ALL";
            break;
        case Zid_RELATIVE:
            str = "Zid_RELATIVE";
            break;
        default:
            str = "Zid_Undefined";
    }

    return( str );
}

///////////////////////////////////////////////////////////////////////////////
//  Persistent Process Configuration
///////////////////////////////////////////////////////////////////////////////

CPersistConfig::CPersistConfig( const char  *persistPrefix
                              , const char  *processNamePrefix
                              , const char  *processNameFormat
                              , const char  *stdoutPrefix
                              , const char  *stdoutFormat
                              , const char  *programName
                              , const char  *zidFormat
                              , PROCESSTYPE  processType
                              , bool         requiresDTM
                              , int          persistRetries
                              , int          persistWindow
                              )
              : persistPrefix_(persistPrefix)
              , processNamePrefix_(processNamePrefix)
              , processNameFormat_(processNameFormat)
              , stdoutPrefix_(stdoutPrefix)
              , stdoutFormat_(stdoutFormat)
              , programName_(programName)
              , zoneFormat_(zidFormat)
              , processType_(processType)
              , processNameNidFormat_(Nid_Undefined)
              , stdoutNidFormat_(Nid_Undefined)
              , zoneZidFormat_(Zid_Undefined)
              , requiresDTM_(requiresDTM)
              , persistRetries_(persistRetries)
              , persistWindow_(persistWindow)
              , next_(NULL)
              , prev_(NULL)
{
    const char method_name[] = "CPersistConfig::CPersistConfig";
    TRACE_ENTRY;

    if (processNameFormat_.compare(TOKEN_NID_PLUS) == 0)
    {
        processNameNidFormat_ = Nid_ALL;
    }
    else
    {
        if (processNameFormat_.compare(TOKEN_NID) == 0)
        {
            processNameNidFormat_ = Nid_RELATIVE;
        }
    }
    if (stdoutFormat_.compare(TOKEN_NID_PLUS) == 0)
    {
        stdoutNidFormat_ = Nid_ALL;
    }
    else
    {
        if (stdoutFormat_.compare(TOKEN_NID) == 0)
        {
            stdoutNidFormat_ = Nid_RELATIVE;
        }
    }
    if (zoneFormat_.compare(TOKEN_ZID_PLUS) == 0)
    {
        zoneZidFormat_ = Zid_ALL;
    }
    else
    {
        if (zoneFormat_.compare(TOKEN_ZID) == 0)
        {
            zoneZidFormat_ = Zid_RELATIVE;
        }
    }

    TRACE_EXIT;
}

CPersistConfig::~CPersistConfig( void )
{
    const char method_name[] = "CPersistConfig::~CPersistConfig";
    TRACE_ENTRY;

    TRACE_EXIT;
}

CPersistConfigContainer::CPersistConfigContainer( void )
                       : persistsCount_(0)
                       , head_(NULL)
                       , tail_(NULL)
{
    const char method_name[] = "CPersistConfigContainer::CPersistConfigContainer";
    TRACE_ENTRY;

    TRACE_EXIT;
}

CPersistConfigContainer::~CPersistConfigContainer(void)
{
    CPersistConfig *persistConfig = head_;

    const char method_name[] = "CPersistConfigContainer::~CPersistConfigContainer";
    TRACE_ENTRY;

    while ( head_ )
    {
        DeletePersistConfig( persistConfig );
        persistConfig = head_;
    }

    pkeysVector_.clear();

    TRACE_EXIT;
}

void CPersistConfigContainer::Clear( void )
{
    CPersistConfig *persistConfig = head_;

    const char method_name[] = "CPersistConfigContainer::Clear";
    TRACE_ENTRY;

    while ( head_ )
    {
        DeletePersistConfig( persistConfig );
        persistConfig = head_;
    }

    persistsCount_ = 0;
    head_ = NULL;
    tail_ = NULL;

    TRACE_EXIT;
}

CPersistConfig *CPersistConfigContainer::AddPersistConfig( const char  *persistPrefix
                                                         , const char  *processNamePrefix
                                                         , const char  *processNameFormat
                                                         , const char  *stdoutPrefix
                                                         , const char  *stdoutFormat
                                                         , const char  *programName
                                                         , const char  *zidFormat
                                                         , PROCESSTYPE  processType
                                                         , bool         requiresDTM
                                                         , int          persistRetries
                                                         , int          persistWindow
                                                         )
{
    const char method_name[] = "CPersistConfigContainer::AddPersistConfig";
    TRACE_ENTRY;

    CPersistConfig *persistConfig = new CPersistConfig( persistPrefix
                                                      , processNamePrefix
                                                      , processNameFormat
                                                      , stdoutPrefix
                                                      , stdoutFormat
                                                      , programName
                                                      , zidFormat
                                                      , processType
                                                      , requiresDTM
                                                      , persistRetries
                                                      , persistWindow
                                                      );
    if (persistConfig)
    {
        persistsCount_++;
        // Add it to the container list
        if ( head_ == NULL )
        {
            head_ = tail_ = persistConfig;
        }
        else
        {
            tail_->next_ = persistConfig;
            persistConfig->prev_ = tail_;
            tail_ = persistConfig;
        }
    }
    else
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Can't allocate persistent configuration "
                        "object - errno=%d (%s)\n"
                      , method_name, err, strerror(errno));
        mon_log_write(MON_PERSISTCONFIG_ADDCONFIG_1, SQ_LOG_ERR, la_buf);
    }

    TRACE_EXIT;
    return persistConfig;
}

void CPersistConfigContainer::DeletePersistConfig( CPersistConfig *persistConfig )
{
    
    if ( head_ == persistConfig )
        head_ = persistConfig->next_;
    if ( tail_ == persistConfig )
        tail_ = persistConfig->prev_;
    if ( persistConfig->prev_ )
        persistConfig->prev_->next_ = persistConfig->next_;
    if ( persistConfig->next_ )
        persistConfig->next_->prev_ = persistConfig->prev_;
    delete persistConfig;
}

CPersistConfig *CPersistConfigContainer::GetPersistConfig( const char *persistPrefix )
{
    CPersistConfig *config = head_;

    const char method_name[] = "CPersistConfigContainer::GetPersistConfig";
    TRACE_ENTRY;

    while ( config )
    {
        if (strcasecmp( config->GetPersistPrefix(), persistPrefix) == 0)
        {
            break;
        }
        config = config->GetNext();
    }

    TRACE_EXIT;
    return config;
}

void CPersistConfigContainer::InitializePersistKeys( char *persistkeys )
{
    const char method_name[] = "CPersistConfigContainer::InitializePersistKeys";
    TRACE_ENTRY;

    char *token;
    static const char *delim = ", ";

    token = strtok( persistkeys, delim );
    while (token != NULL)
    {
        if ( trace_settings & TRACE_INIT )
        {
            trace_printf("%s@%d Setting pkeysVector=%s\n",
                         method_name, __LINE__, token);
        }
        pkeysVector_.push_back( token );
        token = strtok( NULL, " ," );
    }

    TRACE_EXIT;
}
