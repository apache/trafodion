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

#ifndef PERSISTCONFIG_H_
#define PERSISTCONFIG_H_

#include <string>
#include <vector>
#include "trafconf/trafconfig.h"

using namespace std;

#define TOKEN_NID       "%nid"
#define TOKEN_NID_PLUS  "%nid+"
#define TOKEN_ZID       "%zid"
#define TOKEN_ZID_PLUS  "%zid+"

typedef enum 
{
     Nid_Undefined=0
   , Nid_ALL          // %nid+
   , Nid_RELATIVE     // %nid
   //, Nid_SET        // %nid[n,...] future?
} FormatNid_t; 

typedef enum 
{
     Zid_Undefined=0
   , Zid_ALL          // %zid+
   , Zid_RELATIVE     // %zid
   //, Zid_SET        // %zid[n,...] future?
} FormatZid_t; 

typedef vector<string>  pkeysVector_t;
typedef vector<string>  stringVector_t;

typedef struct persistConfigInfo_s
{
    char            persistPrefix[TC_PERSIST_KEY_MAX];
    char            processNamePrefix[TC_PERSIST_VALUE_MAX];
    char            processNameFormat[TC_PERSIST_VALUE_MAX];
    char            stdoutPrefix[TC_PERSIST_VALUE_MAX];
    char            stdoutFormat[TC_PERSIST_VALUE_MAX];
    char            programName[TC_PERSIST_VALUE_MAX];
    char            programArgs[TC_PERSIST_VALUE_MAX];
    char            zoneFormat[TC_PERSIST_VALUE_MAX];
    TC_PROCESS_TYPE processType;
    bool            requiresDTM;
    int             persistRetries;
    int             persistWindow;
} persistConfigInfo_t;

class CPersistConfig;

class CPersistConfigContainer
{
public:
    CPersistConfigContainer( void );
    ~CPersistConfigContainer( void );

    CPersistConfig *AddPersistConfig( persistConfigInfo_t &persistConfigInfo );
    void          Clear( void );
    void          DeletePersistConfig( CPersistConfig *persistConfig );
    inline CPersistConfig *GetFirstPersistConfig( void ) { return ( head_ ); }
    CPersistConfig *GetPersistConfig( const char *persistPrefix );
    CPersistConfig *GetPersistConfig( TC_PROCESS_TYPE processType
                                    , const char *processName
                                    , int         nid );
    inline int    GetPersistConfigCount( void ) { return ( persistsCount_ ); }

protected:
    void  InitializePersistKeys( char *persistkeys 
                               , pkeysVector_t &pkeysVector );
    int   GetPersistKeysCount( void ) { return ( static_cast<int>(pkeysVector_.size()) ); }

    int             persistsCount_; // # of persistent configuration object
    pkeysVector_t   pkeysVector_;   // vector of persist keys

private:
    CPersistConfig  *head_;  // head of persist configuration linked list
    CPersistConfig  *tail_;  // tail of persist configuration linked list
};

class CPersistConfig
{
    friend CPersistConfig *CPersistConfigContainer::AddPersistConfig( persistConfigInfo_t &persistConfigInfo );
    friend void CPersistConfigContainer::DeletePersistConfig( CPersistConfig *persistConfig );
public:
    CPersistConfig( persistConfigInfo_t &persistConfigInfo );
    ~CPersistConfig( void );

    inline CPersistConfig *GetNext( void ){ return( next_); }

    inline const char   *GetPersistPrefix( void ) { return( persistPrefix_.c_str() ); }
           const char   *GetProcessName( int nid );
    inline const char   *GetProcessNamePrefix( void ) { return( processNamePrefix_.c_str() ); }
    inline const char   *GetProcessNameFormat( void ) { return( processNameFormat_.c_str() ); }
    inline FormatNid_t   GetProcessNameNidFormat( void ) { return( processNameNidFormat_ ); }
           const char   *GetStdoutFile( int nid );
    inline const char   *GetStdoutPrefix( void ) { return( stdoutPrefix_.c_str() ); }
    inline const char   *GetStdoutFormat( void ) { return( stdoutFormat_.c_str() ); }
    inline FormatNid_t   GetStdoutNidFormat( void ) { return( stdoutNidFormat_ ); }
    inline const char   *GetProgramName( void ) { return( programName_.c_str() ); }
    inline const char   *GetProgramArgs( void ) { return( programArgs_.c_str() ); }
    inline int           GetProgramArgc( void ) { return( programArgc_ ); }
    inline const char   *GetProgramArgv( void ) { return( programArgv_ ); }
    inline int           GetProgramArgvLen( void ) { return( programArgvLen_ ); }
    inline const char   *GetZoneFormat( void ) { return( zoneFormat_.c_str() ); }
    inline FormatZid_t   GetZoneZidFormat( void ) { return( zoneZidFormat_ ); }
    inline TC_PROCESS_TYPE GetProcessType( void ) { return ( processType_ ); }
    inline bool          GetRequiresDTM( void ) { return ( requiresDTM_ ); }
    inline int           GetPersistRetries( void ) { return ( persistRetries_ ); }
    inline int           GetPersistWindow( void ) { return ( persistWindow_ ); }
           bool          IsPersistConfig( const char *processName, int nid );
           bool          IsZoneMatch( int zid );

protected:
private:
    string          persistPrefix_;
    string          processName_;
    string          processNamePrefix_;
    string          processNameFormat_;
    string          stdoutFile_;
    string          stdoutPrefix_;
    string          stdoutFormat_;
    string          programName_;
    string          programArgs_;
    string          zoneFormat_;
    TC_PROCESS_TYPE processType_;
    FormatNid_t     processNameNidFormat_;
    FormatNid_t     stdoutNidFormat_;
    FormatZid_t     zoneZidFormat_;
    bool            requiresDTM_;
    int             persistRetries_;
    int             persistWindow_;

    int             programArgc_;
    char           *programArgv_;
    int             programArgvLen_;

    CPersistConfig *next_;   // next PersistConfig in CPersistConfigContainer list
    CPersistConfig *prev_;   // previous PersistConfig in CPersistConfigContainer list
};

#endif /* PERSISTCONFIG_H_ */
