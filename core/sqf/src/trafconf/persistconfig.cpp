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

#include <errno.h>
#include <string.h>
#include "tclog.h"
#include "tctrace.h"
#include "persistconfig.h"

const char *PersistProcessTypeString( TcProcessType_t type )
{
    const char *str;

    switch( type )
    {
        case ProcessType_TSE:
            str = "TSE";
            break;
        case ProcessType_DTM:
            str = "DTM";
            break;
        case ProcessType_ASE:
            str = "ASE";
            break;
        case ProcessType_Generic:
            str = "GENERIC";
            break;
        case ProcessType_Watchdog:
            str = "WDG";
            break;
        case ProcessType_NameServer:
            str = "TNS";
            break;
        case ProcessType_AMP:
            str = "AMP";
            break;
        case ProcessType_Backout:
            str = "BO";
            break;
        case ProcessType_VolumeRecovery:
            str = "VR";
            break;
        case ProcessType_MXOSRVR:
            str = "MXOSRVR";
            break;
        case ProcessType_SPX:
            str = "SPX";
            break;
        case ProcessType_SSMP:
            str = "SSMP";
            break;
        case ProcessType_PSD:
            str = "PSD";
            break;
        case ProcessType_SMS:
            str = "SMS";
            break;
        case ProcessType_TMID:
            str = "TMID";
            break;
        case ProcessType_PERSIST:
            str = "PERSIST";
            break;
        default:
            str = "Undefined";
            break;
    }

    return( str );
}

const char *ProcessTypeString( TcProcessType_t type )
{
    const char *str;
    
    switch( type )
    {
        case ProcessType_Undefined:
            str = "ProcessType_Undefined";
            break;
        case ProcessType_TSE:
            str = "ProcessType_TSE";
            break;
        case ProcessType_DTM:
            str = "ProcessType_DTM";
            break;
        case ProcessType_ASE:
            str = "ProcessType_ASE";
            break;
        case ProcessType_Generic:
            str = "ProcessType_Generic";
            break;
        case ProcessType_Watchdog:
            str = "ProcessType_Watchdog";
            break;
        case ProcessType_AMP:
            str = "ProcessType_AMP";
            break;
        case ProcessType_Backout:
            str = "ProcessType_Backout";
            break;
        case ProcessType_VolumeRecovery:
            str = "ProcessType_VolumeRecovery";
            break;
        case ProcessType_MXOSRVR:
            str = "ProcessType_MXOSRVR";
            break;
        case ProcessType_SPX:
            str = "ProcessType_SPX";
            break;
        case ProcessType_SSMP:
            str = "ProcessType_SSMP";
            break;
        case ProcessType_PSD:
            str = "ProcessType_PSD";
            break;
        case ProcessType_SMS:
            str = "ProcessType_SMS";
            break;
        case ProcessType_TMID:
            str = "ProcessType_TMID";
            break;
        case ProcessType_PERSIST:
            str = "ProcessType_PERSIST";
            break;
        default:
            str = "ProcessType_Invalid";
    }

    return( str );
}

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

CPersistConfig::CPersistConfig( persistConfigInfo_t &persistConfigInfo )
              : persistPrefix_(persistConfigInfo.persistPrefix)
              , processName_("")
              , processNamePrefix_(persistConfigInfo.processNamePrefix)
              , processNameFormat_(persistConfigInfo.processNameFormat)
              , stdoutFile_("")
              , stdoutPrefix_(persistConfigInfo.stdoutPrefix)
              , stdoutFormat_(persistConfigInfo.stdoutFormat)
              , programName_(persistConfigInfo.programName)
              , programArgs_(persistConfigInfo.programArgs)
              , zoneFormat_(persistConfigInfo.zoneFormat)
              , processType_(persistConfigInfo.processType)
              , processNameNidFormat_(Nid_Undefined)
              , stdoutNidFormat_(Nid_Undefined)
              , zoneZidFormat_(Zid_Undefined)
              , requiresDTM_(persistConfigInfo.requiresDTM)
              , persistRetries_(persistConfigInfo.persistRetries)
              , persistWindow_(persistConfigInfo.persistWindow)
              , programArgc_(0)
              , programArgv_(NULL)
              , programArgvLen_(0)
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

    char *token, *programArgs = NULL;
    static const char *delim = " ";
    stringVector_t argvVector;
    
    if (programArgs_.size())
    {
        programArgs = new char [programArgs_.size()+100];
        memset(programArgs, 0, programArgs_.size()+100);
        memcpy(programArgs, programArgs_.c_str(), programArgs_.size());
        
        token = strtok( programArgs, delim );
        while (token != NULL)
        {
            if ( TcTraceSettings & TC_TRACE_INIT )
            {
                trace_printf("%s@%d Setting argvVector=%s\n",
                             method_name, __LINE__, token);
            }
            argvVector.push_back( token );
            token = strtok( NULL, delim );
        }

        programArgc_ = static_cast<int>(argvVector.size());

        // Compute amount of space need to store argument strings
        stringVector_t::iterator avit;
        for (avit = argvVector.begin(); avit < argvVector.end(); avit++ )
        {
            programArgvLen_ += static_cast<int>(strlen(avit->c_str()) + 1);
        }
        
        if ( TcTraceSettings & TC_TRACE_INIT )
        {
            trace_printf( "%s@%d - Copying arguments "
                          "programArgc_=%d, programArgvLen_=%d\n"
                        , method_name, __LINE__
                        , programArgc_, programArgvLen_);
        }

        if (programArgvLen_ != 0)
        {
            programArgv_ = new char[programArgvLen_];
            if (programArgv_)
            {
                memset(programArgv_, 0, programArgvLen_);
                char *pProgramArgv = programArgv_;
                for (avit = argvVector.begin(); avit < argvVector.end(); avit++ )
                {
                    if ( TcTraceSettings & TC_TRACE_INIT )
                    {
                        trace_printf("%s@%d - prefix=%s, Copying argvVector='%s'\n"
                                    , method_name, __LINE__
                                    , persistPrefix_.c_str(), avit->c_str());
                    }
                    strcpy (pProgramArgv, avit->c_str());
                    pProgramArgv += strlen(avit->c_str()) + 1;
                }
            }
        }
    }
    
    if (programArgs) delete [] programArgs;

    TRACE_EXIT;
}

CPersistConfig::~CPersistConfig( void )
{
    const char method_name[] = "CPersistConfig::~CPersistConfig";
    TRACE_ENTRY;

    if (programArgv_) delete [] programArgv_;

    TRACE_EXIT;
}


const char *CPersistConfig::GetProcessName( int nid )
{
    const char method_name[] = "CPersistConfig::GetProcessName";
    TRACE_ENTRY;

    char nidStr[TC_PROCESSOR_NAME_MAX];

    switch (processNameNidFormat_)
    {
    case Nid_ALL:
    case Nid_RELATIVE:
        if (nid == -1)
        {
            processName_ = processNamePrefix_;
        }
        else
        {
            sprintf( nidStr, "%d", nid );
            processName_ = processNamePrefix_ + nidStr;
        }
        break;
    case Nid_Undefined:
        processName_ = processNamePrefix_;
    }

    if ( TcTraceSettings & (TC_TRACE_REQUEST | TC_TRACE_PROCESS))
    {
        trace_printf( "%s@%d Process prefix=%s, name=%s, format=%s\n"
                    , method_name, __LINE__
                    , processNamePrefix_.c_str()
                    , processName_.c_str()
                    , FormatNidString(processNameNidFormat_));
    }

    TRACE_EXIT;
    return( processName_.c_str() );
}

const char *CPersistConfig::GetStdoutFile( int nid )
{
    const char method_name[] = "CPersistConfig::GetStdoutFile";
    TRACE_ENTRY;

    char nidStr[TC_PROCESSOR_NAME_MAX];

    switch (stdoutNidFormat_)
    {
    case Nid_ALL:
    case Nid_RELATIVE:
        if (nid == -1)
        {
            stdoutFile_ = stdoutPrefix_;
        }
        else
        {
            sprintf( nidStr, "%d", nid );
            stdoutFile_ = stdoutPrefix_ + nidStr;
        }
        break;
    case Nid_Undefined:
        stdoutFile_ = stdoutPrefix_;
    }

    if ( TcTraceSettings & (TC_TRACE_REQUEST | TC_TRACE_PROCESS))
    {
        trace_printf( "%s@%d stdout prefix=%s, file=%s, format=%s\n"
                    , method_name, __LINE__
                    , stdoutPrefix_.c_str()
                    , stdoutFile_.c_str()
                    , FormatNidString(stdoutNidFormat_));
    }

    TRACE_EXIT;
    return( stdoutFile_.c_str() );
}

bool CPersistConfig::IsPersistConfig( const char *processName, int nid )
{
    const char method_name[] = "CPersistConfig:IsPersistConfig";
    TRACE_ENTRY;
    
    bool match = false;

    string name = GetProcessName( nid );
    if ( name.compare( processName ) == 0 )
    {
        match = true;
    }

    TRACE_EXIT;
    return( match );
}

bool CPersistConfig::IsZoneMatch( int zid )
{
    const char method_name[] = "CPersistConfig:IsZoneMatch";
    TRACE_ENTRY;
    
    bool match = false;

    switch (stdoutNidFormat_)
    {
    case Zid_ALL:
        if (zid == -1)
        {
            match = false;
        }
        else
        {
            match = true;
        }
        break;
    case Zid_RELATIVE:
        if (zid == -1)
        {
            match = false;
        }
        else
        {
            match = true;
        }
        break;
    case Zid_Undefined:
        match = true;
        break;
    }

    TRACE_EXIT;
    return( match );
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

    pkeysVector_.clear();

    persistsCount_ = 0;
    head_ = NULL;
    tail_ = NULL;

    TRACE_EXIT;
}

CPersistConfig *CPersistConfigContainer::AddPersistConfig( persistConfigInfo_t &persistConfigInfo )
{
    const char method_name[] = "CPersistConfigContainer::AddPersistConfig";
    TRACE_ENTRY;

    CPersistConfig *persistConfig = new CPersistConfig( persistConfigInfo );
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
        char la_buf[TC_LOG_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Can't allocate persistent configuration "
                        "object - errno=%d (%s)\n"
                      , method_name, err, strerror(errno));
        TcLogWrite(MON_PERSISTCONFIG_ADDCONFIG_1, TC_LOG_ERR, la_buf);
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

CPersistConfig *CPersistConfigContainer::GetPersistConfig( TcProcessType_t processType
                                                         , const char *processName
                                                         , int         nid )
{
    CPersistConfig *config = head_;

    const char method_name[] = "CPersistConfigContainer::GetPersistConfig";
    TRACE_ENTRY;

    while ( config )
    {
        if (config->GetProcessType() == processType)
        {
            if ( TcTraceSettings & (TC_TRACE_REQUEST | TC_TRACE_PROCESS))
            {
                trace_printf( "%s@%d Process type=%s, name=%s\n"
                            , method_name, __LINE__
                            , PersistProcessTypeString(processType)
                            , processName);
            }
            if (config->IsPersistConfig( processName, nid ))
            {
                break;
            }
        }
        config = config->GetNext();
    }

    TRACE_EXIT;
    return config;
}

void CPersistConfigContainer::InitializePersistKeys( char *persistkeys
                                                   , pkeysVector_t &pkeysVector )
{
    const char method_name[] = "CPersistConfigContainer::InitializePersistKeys";
    TRACE_ENTRY;

    char *token;
    static const char *delim = ", ";

    token = strtok( persistkeys, delim );
    while (token != NULL)
    {
        if ( TcTraceSettings & TC_TRACE_INIT )
        {
            trace_printf("%s@%d Setting pkeysVector=%s\n",
                         method_name, __LINE__, token);
        }
        pkeysVector.push_back( token );
        pkeysVector_.push_back( token );
        token = strtok( NULL, " ," );
    }

    TRACE_EXIT;
}
