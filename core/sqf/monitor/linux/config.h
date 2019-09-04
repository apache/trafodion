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

#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>
//#include <sqlite3.h>
#include "msgdef.h"
#include "internal.h"
#include "clusterconf.h"

class CConfigGroup;

struct cluster_set
{
   ConfigType type;
   int scopeLength;
   int keyLength;
   int valueLength;
   char stringData;
};

struct unique_string_set
{
   int nid;
   int unique_id;
   int stringLength;
   char stringData;
};

class CConfigKey
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    
    CConfigKey( CConfigGroup *group, string name, char *value );
    ~CConfigKey( void );

    void DeLink( CConfigKey **head, CConfigKey **tail );
    CConfigGroup *GetGroup ( void ) { return group_; }
    CConfigKey *GetNext( void );
    CConfigKey *Link( CConfigKey *entry );
    const char * GetName  ( void ) { return name_.c_str(); }
    const char * GetValue ( void ) { return value_.c_str(); }
    void SetValue ( const char * value ) { value_ = value; }
    
protected:
private:
    CConfigGroup *group_;        // point to group owning this key
    string name_;               // key name
    string value_;              // key value
    CConfigKey *Next;           // pointer to next key in group
    CConfigKey *Prev;           // pointer to previous key in group
};

class CConfigGroup
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:

    CConfigGroup( ConfigType type,char *name ); // create predefined group
    ~CConfigGroup( void );

    void DeleteKey( CConfigKey *key );
    void DeLink( CConfigGroup **head, CConfigGroup **tail );
    CConfigKey *GetKey( char *key );
    const char * GetName  ( void ) { return name_.c_str(); }
    CConfigGroup *GetNext( void );
    int GetNumKeys ( void ) { return numKeys_; }
    ConfigType GetType ( void ) { return type_; }
    CConfigGroup *Link( CConfigGroup *entry );
    void SendChangeNotification( CConfigKey *key );
    void Set( char *key, char *value, bool replicated=false, bool addToDb=true );
    
protected:
    
private:
    void NormalizeName ( string &name );

    ConfigType  type_;          // Type of this configuration group
    int         numKeys_;       // Number of keys associated with group
    CConfigGroup *Next;         // pointer to next group in container
    CConfigGroup *Prev;         // pointer to previous group in container
    CConfigKey   *Head;         // Head of key list in group
    CConfigKey   *Tail;         // Tail of key list in group
    string       name_;         // name of group
};

class CConfigContainer
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    CConfigContainer(void);
    ~CConfigContainer(void);

    void Init(void);
    
    CConfigGroup *AddGroup( char *groupkey,ConfigType type=ConfigType_Process,
                            bool addToDb=true );
    void DeleteGroup( CConfigGroup *group );
    CConfigGroup *GetGroup( const char *groupkey );
    CConfigGroup *GetClusterGroup ( void ) { return cluster_; }
    CConfigGroup *GetLocalNodeGroup ( void ) { return node_; }
    const char *GetLocalNodeName ( void ) { return localNodeName_; }

    void Set( char *groupName, ConfigType type, char *keyName, char *value,
              bool addToDb );

    void addDbKeyName ( const char * key );
    void addDbProcName ( const char * key );
    void addDbProcData ( const char * procName, const char * key, const char * dataValue );
    void addDbClusterData ( const char * key, const char * dataValue );
    void addUniqueString(int nid, int id, const char * uniqStr );
    int  getMaxUniqueId( int nid );
    bool getUniqueString(int nid, int id, string & uniqStr );
    bool getUniqueStringId(int nid, const char * uniqStr, strId_t & id );
    void strIdToString ( strId_t stringId, string & value );

    int    PackRegistry( char *&buffer, ConfigType type );
    void   UnpackRegistry( char *&buffer, int length );    
    int    PackUniqueStrings( char *&buffer );
    void   UnpackUniqueStrings( char *&buffer, int length );
    int    getRegistrySize();
    int    getUniqueStringsSize();

protected:
private:
    CConfigGroup *cluster_;     // predefined cluster global group 
    CConfigGroup *node_;        // predefined node local group
    char          localNodeName_[MAX_KEY_NAME];
    CConfigGroup *Head;         // Head of process configuration group list
    CConfigGroup *Tail;         // Tail of process configuration group list
    //sqlite3      *db_;
};

#endif /*CONFIG_H_*/
