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
#include "msgdef.h"
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

class CPersistConfig;

class CPersistConfigContainer
{
public:
    CPersistConfigContainer( void );
    ~CPersistConfigContainer( void );

    CPersistConfig *AddPersistConfig( const char  *persistPrefix
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
                                    );
    void          Clear( void );
    void          DeletePersistConfig( CPersistConfig *persistConfig );
    inline CPersistConfig *GetFirstPersistConfig( void ) { return ( head_ ); }
    CPersistConfig *GetPersistConfig( const char *persistPrefix );
    CPersistConfig *GetPersistConfig( PROCESSTYPE processType );
    inline int    GetPersistConfigCount( void ) { return ( persistsCount_ ); }

protected:
    void  InitializePersistKeys( char *persistkeys );
    int   GetPersistKeysCount( void ) { return ( pkeysVector_.size() ); }

    int             persistsCount_; // # of persistent configuration object
    vector<string>  pkeysVector_;   // vector of persist keys

private:
    CPersistConfig  *head_;  // head of persist configuration linked list
    CPersistConfig  *tail_;  // tail of persist configuration linked list
};

class CPersistConfig
{
    friend CPersistConfig *CPersistConfigContainer::AddPersistConfig( const char  *persistPrefix
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
                                                                    );
    friend void CPersistConfigContainer::DeletePersistConfig( CPersistConfig *persistConfig );
public:
    CPersistConfig( const char  *persistPrefix
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
                  );
    ~CPersistConfig( void );

    inline CPersistConfig *GetNext( void ){ return( next_); }

    inline const char   *GetPersistPrefix( void ) { return( persistPrefix_.c_str() ); }
    inline const char   *GetProcessNamePrefix( void ) { return( processNamePrefix_.c_str() ); }
    inline const char   *GetProcessNameFormat( void ) { return( processNameFormat_.c_str() ); }
    inline FormatNid_t   GetProcessNameNidFormat( void ) { return( processNameNidFormat_ ); }
    inline const char   *GetStdoutPrefix( void ) { return( stdoutPrefix_.c_str() ); }
    inline const char   *GetStdoutFormat( void ) { return( stdoutFormat_.c_str() ); }
    inline FormatNid_t   GetStdoutNidFormat( void ) { return( stdoutNidFormat_ ); }
    inline const char   *GetProgramName( void ) { return( programName_.c_str() ); }
    inline const char   *GetZoneFormat( void ) { return( zoneFormat_.c_str() ); }
    inline FormatZid_t   GetZoneZidFormat( void ) { return( zoneZidFormat_ ); }
    inline PROCESSTYPE   GetProcessType( void ) { return ( processType_ ); }
    inline bool          GetRequiresDTM( void ) { return ( requiresDTM_ ); }
    inline int           GetPersistRetries( void ) { return ( persistRetries_ ); }
    inline int           GetPersistWindow( void ) { return ( persistWindow_ ); }

protected:
private:
    string          persistPrefix_;
    string          processNamePrefix_;
    string          processNameFormat_;
    string          stdoutPrefix_;
    string          stdoutFormat_;
    string          programName_;
    string          zoneFormat_;
    PROCESSTYPE     processType_;
    FormatNid_t     processNameNidFormat_;
    FormatNid_t     stdoutNidFormat_;
    FormatZid_t     zoneZidFormat_;
    bool            requiresDTM_;
    int             persistRetries_;
    int             persistWindow_;

    CPersistConfig *next_;   // next PersistConfig in CPersistConfigContainer list
    CPersistConfig *prev_;   // previous PersistConfig in CPersistConfigContainer list
};

#endif /* PERSISTCONFIG_H_ */
