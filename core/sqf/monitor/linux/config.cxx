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

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <unistd.h>

using namespace std;

#include "monlogging.h"
#include "monsonar.h"
#include "montrace.h"
#include "monitor.h"
#include "clusterconf.h"
#include "lock.h"
#include "lnode.h"
#include "pnode.h"
#include "config.h"
#include "replicate.h"
#include "mlio.h"

extern int MyPNID;
extern char Node_name[MPI_MAX_PROCESSOR_NAME];
extern CNodeContainer *Nodes;
extern CConfigContainer *Config;
extern CNode *MyNode;
extern CMonStats *MonStats;
extern CMonTrace *MonTrace;
extern CMonLog *MonLog;
extern CReplicate Replicator;

extern char *ErrorMsg (int error_code);

CConfigKey::CConfigKey(CConfigGroup *group, string name, char *value)
           :
             group_ (group),
             name_ (name),
             value_ (value),
             Next (NULL),
             Prev (NULL)
{
    const char method_name[] = "CConfigKey::CConfigKey";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "CFGK", 4);

    TRACE_EXIT;
}

CConfigKey::~CConfigKey(void)
{
    const char method_name[] = "CConfigKey::~CConfigKey";
    TRACE_ENTRY;

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "cfgk", 4);

    TRACE_EXIT;
}

void CConfigKey::DeLink( CConfigKey **head, CConfigKey **tail )
{
    const char method_name[] = "CConfigKey::DeLink";
    TRACE_ENTRY;
    if (*head == this)
        *head = Next;
    if (*tail == this)
        *tail = Prev;
    if (Prev)
        Prev->Next = Next;
    if (Next)
        Next->Prev = Prev;
    TRACE_EXIT;
}

CConfigKey *CConfigKey::GetNext( void )
{
    const char method_name[] = "CConfigKey::GetNext";
    TRACE_ENTRY;
    TRACE_EXIT;
    return Next;
}

CConfigKey *CConfigKey::Link( CConfigKey *entry )
{
    const char method_name[] = "CConfigKey::Link";
    TRACE_ENTRY;
    Next = entry;
    entry->Prev = this;

    TRACE_EXIT;
    return entry;
}

CConfigGroup::CConfigGroup(ConfigType type, char *name)
            : type_ (type),
              numKeys_ (0),
              Next (NULL),
              Prev (NULL),
              Head (NULL),
              Tail (NULL),
              name_ (name)
{
    const char method_name[] = "CConfigGroup::CConfigGroup(type,name)";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "CFGG", 4);

    NormalizeName(name_);
    TRACE_EXIT;
}


CConfigGroup::~CConfigGroup(void)
{
    CConfigKey *entry;
    
    const char method_name[] = "CConfigGroup::~CConfigGroup";
    TRACE_ENTRY;
    while (Head)
    {
        entry = Head;
        entry->DeLink(&Head,&Tail);
        delete entry;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "cfgg", 4);

    TRACE_EXIT;
}

void CConfigGroup::DeleteKey( CConfigKey *key )
{
    const char method_name[] = "CConfigGroup::DeleteKey";
    TRACE_ENTRY;
    key->DeLink(&Head,&Tail);
    delete key;
    numKeys_--;
    TRACE_EXIT;
}

void CConfigGroup::DeLink( CConfigGroup **head, CConfigGroup **tail )
{
    const char method_name[] = "CConfigGroup::DeLink";
    TRACE_ENTRY;
    if (*head == this)
        *head = Next;
    if (*tail == this)
        *tail = Prev;
    if (Prev)
        Prev->Next = Next;
    if (Next)
        Next->Prev = Prev;
    TRACE_EXIT;
}

CConfigKey *CConfigGroup::GetKey(char *key)
{
    CConfigKey *entry;

    const char method_name[] = "CConfigGroup::GetKey";
    TRACE_ENTRY;
    if (*key)
    {
        entry = Head;

        while (entry)
        {
            if ( strcasecmp(entry->GetName(), key) == 0)
            {
                break;
            }
            entry = entry->GetNext();
        }
    }
    else
    {
        // Return 1st ConfigKey if key = ""
        entry = Head;
    }    
    TRACE_EXIT;

    return entry;
}

CConfigGroup *CConfigGroup::GetNext( void )
{
    const char method_name[] = "CConfigGroup::GetNext";
    TRACE_ENTRY;
    TRACE_EXIT;
    return Next;
}

CConfigGroup *CConfigGroup::Link( CConfigGroup *entry )
{
    const char method_name[] = "CConfigGroup::Link";
    TRACE_ENTRY;
    Next = entry;
    entry->Prev = this;

    TRACE_EXIT;
    return entry;
}


void CConfigGroup::NormalizeName (string &name)
{
    const char method_name[] = "CConfigGroup::NormalizeName";
    TRACE_ENTRY;

    for (size_t i=0; i < name.length(); i++)
    {
        name[i] = toupper(name[i]);
    }

    TRACE_EXIT;
}



void CConfigGroup::SendChangeNotification (CConfigKey *key)
{
    CProcess *process;
    struct message_def *msg;

    const char method_name[] = "CConfigGroup::SendChangeNotification";
    TRACE_ENTRY;

    if (strncasecmp(key->GetName(), "mtrc-", 5) == 0)
    {
        // This is a monitor tracing key.  Process it internally to 
        // the monitor.  No need to propagate change notification.
        MonTrace->mon_trace_change(&key->GetName()[5], key->GetValue());

        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_INIT))
            trace_printf("%s%d - Process monitor trace key=%s, value=%s\n",
                         method_name, __LINE__, key->GetName(),
                         key->GetValue());

        TRACE_EXIT;
        return;
    }

    if (strncasecmp(key->GetName(), "menv-", 5) == 0)
    {
        // This is a change in monitor's env variable.
        if (strcasecmp(&key->GetName()[5], "MON_ALTLOG") == 0)
        {
            bool newValue;

            if (strcmp(key->GetValue(), "1") == 0)
            {
                newValue = true;
            }
            else
            {
                newValue = false;
            }

            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_INIT))
                trace_printf("%s%d - Process monitor env change - altlog. value = %d\n", method_name, __LINE__, newValue);

            MonLog->setUseAltLog(newValue);

            TRACE_EXIT;
            return;
        }

        if (strcasecmp(&key->GetName()[5], "MON_ALTLOG_FILENUM") == 0)
        {
            int newValue = atoi(key->GetValue());

            if (newValue < 1 || newValue > 99)
                newValue = MonLog->getLogFileNum(); // not permitted, use the old value

            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_INIT))
                trace_printf("%s%d - Process monitor env change - altlog_filenum. value = %d\n", method_name, __LINE__, newValue);

            MonLog->setLogFileNum(newValue);

            TRACE_EXIT;
            return;
        }
    }

    if (strncasecmp(key->GetName(), "~US_", 4) == 0)
    {  // Internal monitor unique string, don't send change notification
        TRACE_EXIT;
        return;
    }
    if (strncasecmp(key->GetName(), "sonarstate-", 11) == 0) {
        gv_sonar_state = strtoul(key->GetValue(), NULL,0);
    }

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->notice_registry_change_Incr();

    // Build Notice
    msg = new struct message_def;

    msg->type = MsgType_Change;
    msg->noreply = true;
    msg->u.request.type = ReqType_Notice;
    msg->u.request.u.change.type = type_;
    STRCPY(msg->u.request.u.change.group, name_.c_str());
    STRCPY(msg->u.request.u.change.key, key->GetName());
    STRCPY(msg->u.request.u.change.value, key->GetValue());

    switch (type_)
    {
    case ConfigType_Cluster:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_INIT))
           trace_printf("%s@%d - Sending Configuration Change message to cluster\n", method_name, __LINE__);
        // Each monitor will send this notice to their nodes processes
        if ( MyNode ) MyNode->Bcast(msg);
        delete msg;
        break;

    case ConfigType_Node:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_INIT))
           trace_printf("%s@%d - Sending Configuration Change message to nodes in pnid=%d\n", method_name, __LINE__, MyPNID);
        // Only this node's processes will receive the notice    
        if ( MyNode ) MyNode->Bcast(msg);
        delete msg;
        break;
        
    case ConfigType_Process:
        // check if we need to associate a process
        Nodes->GetLNode ((char *)name_.c_str(), &process);
        if ( process )
        {
            if ( process->IsSystemMessages() && !process->IsClone() )
            {
                if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_INIT))
                    trace_printf("%s@%d - Sending Configuration Change message to %s\n", method_name, __LINE__, process->GetName());
                SQ_theLocalIOToClient->putOnNoticeQueue( process->GetPid()
                                                       , process->GetVerifier()
                                                       , msg
                                                       , NULL);
            }
            else
            {
                if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_INIT))
                    trace_printf("%s@%d - Not sending Configuration Change message to %s, system_messages=%d, isClone=%d\n", method_name, __LINE__, process->GetName(), process->IsSystemMessages(), process->IsClone());

                delete msg;
            }
        }
        else
        {   // Could not locate the process
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_INIT))
                trace_printf("%s@%d - Cannot send Configuration Change message, no process %s\n", method_name, __LINE__, name_.c_str());
            delete msg;
        }
        break;

    default:
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[CConfigGroup::SendChangeNotification],"
                 " Invalid group ConfigType for %s (%p).\n", name_.c_str(),
                 this);
        mon_log_write(MON_CONFIGGROUP_SENDCHANGENOTICE, SQ_LOG_ERR, buf);
        delete msg;
    }
    TRACE_EXIT;
}

void CConfigGroup::Set(char *key, char *value, bool replicated, bool addToDb)
{
    CConfigKey *entry = Head;
    bool replicateEntry = !replicated;

    const char method_name[] = "CConfigGroup::Set";
    TRACE_ENTRY;
    
    // Check if we need to update the key value    
    while (entry)
    {
        if (strcasecmp(entry->GetName(), key)==0)
        {
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_INIT))
                trace_printf("%s@%d" " - Updating group: " "%s" ", key: " "%s""\n", method_name, __LINE__, name_.c_str(), entry->GetName());
            entry->SetValue (value);
            if (addToDb)
            {
                if (type_ == ConfigType_Cluster) 
                {
                    Config->addDbClusterData( key, value );
                }
                else if (type_ == ConfigType_Process)
                {
                    Config->addDbProcData( name_.c_str(), key, value );
                }
            }

            break;
        }
        entry = entry->GetNext();
    }
        
    // Check if we need to add the key
    if (entry==NULL)
    {
        string normalizedKey(key);
        NormalizeName(normalizedKey);
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_INIT))
            trace_printf("%s@%d" " - Adding %s::%s\n", method_name, __LINE__,
                         name_.c_str(), normalizedKey.c_str());
        entry = new CConfigKey(this,normalizedKey,value);
        if (Head==NULL)
        {
            Head = Tail = entry;
        }
        else
        {
            Tail = Tail->Link(entry);
        }
        numKeys_++;

        if (addToDb && (strncasecmp(key, "mtrc-", 5) != 0))
        {
            Config->addDbKeyName( key );

            if (type_ == ConfigType_Cluster) 
            {
                Config->addDbClusterData( key, value );
            }
            else if (type_ == ConfigType_Process)
            {
                Config->addDbProcData( name_.c_str(), key, value );
            }
        }
    }

    if (type_ == ConfigType_Node) 
    {
        if ( name_.compare(Config->GetLocalNodeName()) == 0 )
        {
            // Never replicate our own node's configuration
            replicateEntry = false;
            SendChangeNotification (entry);
        }
    }
    else
    {
        SendChangeNotification (entry);
    }

    if (replicateEntry)
    {
        CReplConfigData *repl = new CReplConfigData(entry);
        Replicator.addItem(repl);
    }

    TRACE_EXIT;
}

CConfigContainer::CConfigContainer(void)
                : Head (NULL)
                , Tail (NULL)
                , db_  (NULL)
{
    const char method_name[] = "CConfigContainer::CConfigContainer";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "CCTR", 4);

    // Open the configuration database file
    char dbase[MAX_PROCESS_PATH];
    //    snprintf(dbase, sizeof(dbase), "%s/sql/scripts/sqconfig.db.%d",
    //             getenv("TRAF_HOME"),MyPNID);

    snprintf(dbase, sizeof(dbase), "%s/sql/scripts/sqconfig.db",
             getenv("TRAF_HOME"));
    int rc = sqlite3_open_v2(dbase, &db_,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX,
                             NULL);

    if ( rc )
    {
        db_ = NULL;

        // failed
        if (trace_settings & TRACE_INIT)
        {
            trace_printf("%s@%d Can't open database: %s, err=%d\n", method_name,
                         __LINE__, dbase, rc);
        }

        // See if have database in current directory
        int rc2 = sqlite3_open_v2("sqconfig.db", &db_,
                                  SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX,
                                  NULL);

        if ( rc2 )
        {
            // failed to open database
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] Can't open database %s: %d, "
                      "or %s: %d\n", method_name,  dbase, rc ,
                      "sqconfig.db", rc2);
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );

            MPI_Abort(MPI_COMM_SELF,99);
        }

        if (trace_settings & TRACE_INIT)
        {
            char buf[1000];
            getcwd(buf, sizeof(buf));

            trace_printf("%s@%d Successfully opened sqconfig.db in %s\n",
                         method_name, __LINE__, buf);
        }
    } 

    if ( db_ != NULL )
    {
        rc = sqlite3_busy_timeout(db_, 1000);

        if ( rc )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] Can't set busy timeout for "
                      "database %s: %s (%d)\n",
                      method_name,  dbase, sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }

        char *sErrMsg = NULL;
        sqlite3_exec(db_, "PRAGMA synchronous = OFF", NULL, NULL, &sErrMsg);
        if (sErrMsg != NULL)
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] Can't set PRAGMA synchronous for "
                      "database %s: %s\n",
                      method_name,  dbase, sErrMsg );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }

    }
    
    cluster_ = AddGroup((char *) "CLUSTER",ConfigType_Cluster);
    if ( ! getenv("SQ_VIRTUAL_NODES") )
    {
        int pnid = Nodes->GetClusterConfig()->GetPNid(Node_name);
        snprintf(localNodeName_, sizeof(localNodeName_), "NODE%d",pnid);
    }
    else
    {
        snprintf(localNodeName_, sizeof(localNodeName_), "NODE%d",MyPNID);
    }
    node_ = AddGroup(localNodeName_,ConfigType_Node);

    TRACE_EXIT;
}

CConfigContainer::~CConfigContainer(void)
{
    CConfigGroup *entry;
    
    const char method_name[] = "CConfigContainer::~CConfigContainer";
    TRACE_ENTRY;
    
    while (Head)
    {
        entry = Head;
        entry->DeLink(&Head,&Tail);
        delete entry;
    }
    
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "cctr", 4);

    TRACE_EXIT;
}

void CConfigContainer::Init(void)
{
    int rc;

    const char method_name[] = "CConfigContainer::Init";
    TRACE_ENTRY;

    if ( db_ == NULL)
    {
        if (trace_settings & TRACE_INIT)
        {
            trace_printf("%s@%d cannot initialize registry from database "
                         "since database open failed.\n", method_name,
                         __LINE__);
        }

        TRACE_EXIT;
        return;
    }

    // Read cluster configuration registry entries and populate in-memory
    // structures.
    const char *selStmt;
    selStmt = "select k.keyName, d.dataValue "
              " from monRegKeyName k, monRegClusterData d "
              " where k.keyId = d.keyId";
    sqlite3_stmt *prepStmt;

    rc = sqlite3_prepare_v2( db_, selStmt, strlen(selStmt)+1, &prepStmt, NULL);
    if( rc!=SQLITE_OK )
    {
        if (trace_settings & TRACE_INIT)
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n", method_name,
                         __LINE__, sqlite3_errmsg(db_), rc);
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );

        TRACE_EXIT;
        return;
    }


    ConfigType type;
    const unsigned char * group;
    const unsigned char * key;
    const unsigned char * value;
    int rowNum = 1;

    while ( 1 )
    {
        rc = sqlite3_step(prepStmt);
        if ( rc == SQLITE_ROW )
        {  // Process row
            type = ConfigType_Cluster;
            group = (const unsigned char *) "CLUSTER";
            key = sqlite3_column_text(prepStmt, 0);
            value = sqlite3_column_text(prepStmt, 1);
            if (trace_settings & TRACE_INIT)
            {
                trace_printf("%s@%d row %d: type=%d, group=%s, key=%s, "
                             "value=%s\n", method_name, __LINE__, rowNum,
                             type, group, key, value);
            }
            Set( (char *) group, type, (char *) key, (char *) value, false );
            ++rowNum;
        }
        else if ( rc == SQLITE_DONE )
        {
            break;
        }
        else
        { 
            if (trace_settings & TRACE_INIT)
            {
                trace_printf("%s@%d step failed, retrieving cluster registry "
                             "data, error=%s (%d)\n",
                             method_name, __LINE__, sqlite3_errmsg(db_), rc);
            }

            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] step failed, retrieving "
                      "cluster registry data, error=%s (%d)\n",
                      method_name,  sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );

            break;
        }
    }

    // Destroy prepared statement object
    if ( prepStmt != NULL )
        sqlite3_finalize(prepStmt);

    // Read process configuration registry entries and populate in-memory
    // structures.
    selStmt = "select p.procName, k.keyName, d.dataValue"
              " from monRegProcName p, monRegKeyName k, monRegProcData d"
              " where p.procId = d.procId"
              "   and k.keyId = d.keyId";

    rc = sqlite3_prepare_v2( db_, selStmt, strlen(selStmt)+1, &prepStmt, NULL);
    if( rc!=SQLITE_OK )
    {
        if (trace_settings & TRACE_INIT)
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n", method_name,
                         __LINE__, sqlite3_errmsg(db_), rc);
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );

        TRACE_EXIT;
        return;
    }

    rowNum = 1;

    while ( 1 )
    {
        rc = sqlite3_step(prepStmt);
        if ( rc == SQLITE_ROW )
        {  // Process row
            type = ConfigType_Process;
            group = sqlite3_column_text(prepStmt, 0);
            key = sqlite3_column_text(prepStmt, 1);
            value = sqlite3_column_text(prepStmt, 2);
            if (trace_settings & TRACE_INIT)
            {
                trace_printf("%s@%d row %d: type=%d, group=%s, key=%s, "
                             "value=%s\n", method_name, __LINE__, rowNum,
                             type, group, key, value);
            }
            Set( (char *) group, type, (char *) key, (char *) value, false );
            ++rowNum;
        }
        else if ( rc == SQLITE_DONE )
        {
            break;
        }
        else
        { 
            if (trace_settings & TRACE_INIT)
            {
                trace_printf("%s@%d step failed, retrieving process registry"
                             " data, error=%s (%d)\n",
                             method_name, __LINE__, sqlite3_errmsg(db_), rc);
            }

            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] step failed, retrieving process"
                      " registry data, error=%s (%d)\n",
                      method_name,  sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );

            break;
        }
    }

    // Destroy prepared statement object
    if ( prepStmt != NULL )
        sqlite3_finalize(prepStmt);
 
    TRACE_EXIT;

}


CConfigGroup *CConfigContainer::AddGroup(char *groupkey, ConfigType type,
                                         bool addToDb)
{
    CConfigGroup *group;

    const char method_name[] = "CConfigContainer::AddGroup";
    TRACE_ENTRY;
    group = new CConfigGroup(type, groupkey);
    if (Head==NULL)
    {
        Head = Tail = group;
    }
    else
    {
        Tail = Tail->Link(group);
    }

    if ( addToDb && type == ConfigType_Process )
    {
        // Add to monRegProcName
        addDbProcName ( groupkey );
    }

    TRACE_EXIT;

    return group;
}

void CConfigContainer::DeleteGroup(CConfigGroup *group)
{
    const char method_name[] = "CConfigContainer::DeleteGroup";
    TRACE_ENTRY;
    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_INIT))
       trace_printf("%s@%d" " - Deleting group: " "%s" "\n", method_name,
                    __LINE__, group->GetName());
    group->DeLink(&Head,&Tail);
    delete group;
    TRACE_EXIT;
}

CConfigGroup *CConfigContainer::GetGroup(const char *groupkey)
{
    CConfigGroup *group = Head;

    const char method_name[] = "CConfigContainer::GetGroup";
    TRACE_ENTRY;
    while (group)
    {
        if (strcasecmp(group->GetName(), groupkey)==0)
        {
            break;
        }
        group = group->GetNext();
    }

    TRACE_EXIT;

    return group;
}



void CConfigContainer::Set( char *groupName, ConfigType type,
                            char *keyName, char *value, bool addToDb )
{
    CConfigGroup *group;

    // This function is only to be called when replicating keys
    // across nodes or when doing a WarmStart of the registry

    const char method_name[] = "CConfigContainer::Set";
    TRACE_ENTRY;
        
    group = GetGroup(groupName);
    if (group)
    {
        group->Set(keyName,value,true, addToDb);
    }
    else
    {
        group = AddGroup(groupName,type,addToDb);
        group->Set(keyName,value,true, addToDb);
    }
    TRACE_EXIT;
}

// insert key into monRegKeyName table
void CConfigContainer::addDbKeyName ( const char * key )
{
    const char method_name[] = "CConfigContainer::addDbKeyName";
    TRACE_ENTRY;

    if ( db_ == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d cannot use database since database open"
                         " failed.\n", method_name, __LINE__);
        }

        TRACE_EXIT;
        return;
    }

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf("%s@%d inserting key=%s into monRegKeyName\n",
                     method_name, __LINE__, key);
    }

    int rc;
    const char * sqlStmt;
    sqlStmt = "insert into monRegKeyName (keyName) values ( :key );";
    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, strlen(sqlStmt)+1, &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_text( prepStmt, 
                           sqlite3_bind_parameter_index( prepStmt, ":key" ),
                           key, -1, SQLITE_STATIC );
#ifndef SQLITE_IO_RETRY
        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW ) 
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), 
                      "[%s] inserting key=%s into "
                      "monRegKeyName failed, error=%s (%d)\n",
                      method_name, key, sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }
#else
        bool logError = true;
        int i;
        for ( i = 0; i < sqLiteIORetry_; i++ )
       {
            rc = sqlite3_step( prepStmt );
            if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
             && ( rc != SQLITE_CONSTRAINT ) )
            {
                if ( rc != SQLITE_BUSY )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf), 
                              "[%s] inserting key=%s into "
                              "monRegKeyName failed, error=%s (%d)\n",
                              method_name, key, sqlite3_errmsg(db_), rc );
                    mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
                    logError = false;
                    break;
                }
                usleep( sqLiteIODelay_ );
            }
            else
            {
                break;
            }
        }
        if ( logError && ( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] retries=%d exceeded "
                      "inserting key=%s into "
                      "monRegKeyName failed, error=%s (%d)\n",
                      method_name, i, key, sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }
#endif
    }
        
    if ( prepStmt != NULL )
        sqlite3_finalize( prepStmt );

    TRACE_EXIT;
}


// insert key into monRegProcName table
void CConfigContainer::addDbProcName ( const char * name )
{
    const char method_name[] = "CConfigContainer::addDbProcName";
    TRACE_ENTRY;

    if ( db_ == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d cannot use database since database open"
                         " failed.\n", method_name, __LINE__);
        }

        TRACE_EXIT;
        return;
    }

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf("%s@%d inserting name=%s into monRegProcName\n",
                     method_name, __LINE__, name);
    }

    int rc;
    const char * sqlStmt;
    sqlStmt = "insert into monRegProcName (procName) values ( :name );";
    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, strlen(sqlStmt)+1, &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_text( prepStmt,
                           sqlite3_bind_parameter_index( prepStmt, ":name" ),
                           name, -1, SQLITE_STATIC );

#ifndef SQLITE_IO_RETRY
        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW ) 
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), 
                      "[%s] inserting name=%s into "
                      "monRegProcName failed, error=%s (%d)\n",
                      method_name,  name, sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }
#else
        bool logError = true;
        int i;
        for ( i = 0; i < sqLiteIORetry_; i++ )
        {
            rc = sqlite3_step( prepStmt );
            if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
             && ( rc != SQLITE_CONSTRAINT ) )
            {
                if ( rc != SQLITE_BUSY )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf), 
                              "[%s] inserting name=%s into "
                              "monRegProcName failed, error=%s (%d)\n",
                              method_name,  name, sqlite3_errmsg(db_), rc );
                    mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
                    logError = false;
                    break;
                }
                usleep( sqLiteIODelay_ );
            }
            else
            {
                break;
            }
        }
        if ( logError && ( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] retries=%d exceeded "
                      "inserting name=%s into "
                      "monRegProcName failed, error=%s (%d)\n",
                      method_name, i, name, sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }
#endif
    }
        
    if ( prepStmt != NULL )
        sqlite3_finalize( prepStmt );

    TRACE_EXIT;
}

void CConfigContainer::addDbClusterData ( const char * key,
                                          const char * dataValue )
{
    const char method_name[] = "CConfigContainer::addDbClusterData";
    TRACE_ENTRY;

    if ( db_ == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d cannot use database since database open"
                         " failed.\n", method_name, __LINE__);
        }

        TRACE_EXIT;
        return;
    }

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf("%s@%d inserting key=%s into monRegClusterData\n",
                     method_name, __LINE__, key);
    }

    int rc;
    const char * sqlStmt;
    sqlStmt = "insert or replace into monRegClusterData (dataValue, keyId)"
              " select :dataValue,"
              "         k.keyId FROM monRegKeyName k"
              " where k.keyName = :key";
    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, strlen(sqlStmt)+1, &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_text( prepStmt,
                           sqlite3_bind_parameter_index( prepStmt,
                                                         ":dataValue" ),
                           dataValue, -1, SQLITE_STATIC );
        sqlite3_bind_text( prepStmt,
                           sqlite3_bind_parameter_index( prepStmt, ":key" ),
                           key, -1, SQLITE_STATIC );
#ifndef SQLITE_IO_RETRY
        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW ) 
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] inserting key=%s into "
                      "monRegClusterData failed, error=%s (%d)\n",
                      method_name, key, sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }
#else
        bool logError = true;
        int i;
        for ( i = 0; i < sqLiteIORetry_; i++ )
        {
            rc = sqlite3_step( prepStmt );
            if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
             && ( rc != SQLITE_CONSTRAINT ) )
            {
                if ( rc != SQLITE_BUSY )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf), "[%s] inserting key=%s into "
                              "monRegClusterData failed, error=%s (%d) (i=%d)\n",
                              method_name, key, sqlite3_errmsg(db_), rc, i );
                    mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
                    logError = false;
                    break;
                }
                usleep( sqLiteIODelay_ );
            }
            else
            {
                break;
            }
        }
        if ( logError && ( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] retries=%d exceeded "
                      "inserting key=%s into "
                      "monRegClusterData failed, error=%s (%d)\n",
                      method_name, i, key, sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }
#endif
    }
        
    if ( prepStmt != NULL )
        sqlite3_finalize( prepStmt );

    TRACE_EXIT;
}

void CConfigContainer::addDbProcData ( const char * procName,
                                       const char * key,
                                       const char * dataValue )
{
    const char method_name[] = "CConfigContainer::addDbProcData";
    TRACE_ENTRY;

    if ( db_ == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d cannot use database since database open"
                         " failed.\n", method_name, __LINE__);
        }

        TRACE_EXIT;
        return;
    }

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf("%s@%d inserting key=%s into monRegProcData for "
                     "proc=%s\n", method_name, __LINE__, key, procName);
    }

    int rc;
    const char * sqlStmt;
    sqlStmt = "insert or replace into monRegProcData (dataValue, procId, keyId )"
              "   select :dataValue,"
              "      p.procId,"
              "       (SELECT k.keyId "
              "          FROM monRegKeyName k"
              "         WHERE k.keyName = :key)"
              "   FROM monRegProcName p"
              "   WHERE UPPER(p.procName) = UPPER(:procName)";

    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, strlen(sqlStmt)+1, &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_text( prepStmt,
                           sqlite3_bind_parameter_index( prepStmt,
                                                         ":procName" ),
                           procName, -1, SQLITE_STATIC );
        sqlite3_bind_text( prepStmt,
                           sqlite3_bind_parameter_index( prepStmt,
                                                         ":dataValue" ),
                           dataValue, -1, SQLITE_STATIC );
        sqlite3_bind_text( prepStmt,
                           sqlite3_bind_parameter_index( prepStmt, ":key" ),
                           key, -1, SQLITE_STATIC );
#ifndef SQLITE_IO_RETRY
        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW ) 
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] inserting key=%s into "
                      "monRegProcData for proc=%s failed, error=%s (%d)\n",
                      method_name,  key, procName, sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }
#else
        bool logError = true;
        int i;
        for ( i = 0; i < sqLiteIORetry_; i++ )
        {
            rc = sqlite3_step( prepStmt );
            if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
             && ( rc != SQLITE_CONSTRAINT ) )
            {
                if ( rc != SQLITE_BUSY )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf), "[%s] inserting key=%s into "
                              "monRegProcData for proc=%s failed, error=%s (%d)\n",
                              method_name,  key, procName, sqlite3_errmsg(db_), rc );
                    mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
                    logError = false;
                    break;
                }
                usleep( sqLiteIODelay_ );
            }
            else
            {
                break;
            }
        }
        if ( logError && ( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] retries=%d exceeded "
                      "inserting key=%s into "
                      "monRegProcData for proc=%s failed, error=%s (%d)\n",
                      method_name, i, key, procName, sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }
#endif
    }

    if ( prepStmt != NULL )
        sqlite3_finalize( prepStmt );

    TRACE_EXIT;
}

void CConfigContainer::addUniqueString(int nid, int id, const char * uniqStr )
{
    const char method_name[] = "CConfigContainer::addUniqueString";
    TRACE_ENTRY;

    if ( db_ == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d cannot use database since database open"
                         " failed.\n", method_name, __LINE__);
        }

        TRACE_EXIT;
        return;
    }


    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf("%s@%d inserting unique string nid=%d id=%d into "
                     "monRegUniqueStrings\n", method_name, __LINE__,
                     nid, id);
    }

    int rc;
    const char * sqlStmt;
    sqlStmt = "insert or replace into monRegUniqueStrings values (?, ?, ?)";

    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, strlen(sqlStmt)+1, &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_int( prepStmt, 1, nid );
        sqlite3_bind_int( prepStmt, 2, id );
        sqlite3_bind_text( prepStmt, 3, uniqStr, -1, SQLITE_STATIC );
#ifndef SQLITE_IO_RETRY
        rc = sqlite3_step( prepStmt );
        if ( rc != SQLITE_DONE )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), 
                      "[%s] inserting into monRegUniqueStrings "
                      "for nid=%d id=%d failed, error=%s (%d)\n",
                      method_name, nid, id, sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }
#else
        bool logError = true;
        int i;
        for ( i = 0; i < sqLiteIORetry_; i++ )
        {
            rc = sqlite3_step( prepStmt );
            if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
             && ( rc != SQLITE_CONSTRAINT ) )
            {
                if ( rc != SQLITE_BUSY )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf), 
                              "[%s] inserting into monRegUniqueStrings "
                              "for nid=%d id=%d failed, error=%s (%d)\n",
                              method_name, nid, id, sqlite3_errmsg(db_), rc );
                    mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
                    logError = false;
                    break;
                }
                usleep( sqLiteIODelay_ );
            }
            else
            {
                break;
            }
        }
        if ( logError && ( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] retries=%d exceeded "
                      "inserting into monRegUniqueStrings "
                      "for nid=%d id=%d failed, error=%s (%d)\n",
                      method_name, i, nid, id, sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }
#endif
    }

    if ( prepStmt != NULL )
        sqlite3_finalize( prepStmt );

    TRACE_EXIT;
}

bool CConfigContainer::findUniqueString(int nid, const char * uniqStr,
                                        strId_t & id )
{
    const char method_name[] = "CConfigContainer::findUniqueString";
    TRACE_ENTRY;

    if ( db_ == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d cannot use database since database open"
                         " failed.\n", method_name, __LINE__);
        }

        TRACE_EXIT;
        return false;
    }


    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf("%s@%d finding unique string nid=%d string=%s in "
                     "monRegUniqueStrings\n", method_name, __LINE__,
                     nid, uniqStr);
    }


    bool result = false;
    int rc;
    const char * sqlStmt;
    sqlStmt = "select id from monRegUniqueStrings where nid = ? and dataValue = ?";

    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, strlen(sqlStmt)+1, &prepStmt,
                             NULL);

    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_int( prepStmt, 1, nid );
        sqlite3_bind_text( prepStmt, 2, uniqStr, -1, SQLITE_STATIC );

        rc = sqlite3_step( prepStmt );

        if ( rc == SQLITE_ROW )
        {   // Found string in database, return id
            id.nid = nid;
            id.id = sqlite3_column_int (prepStmt, 0);
            result = true;

            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf("%s@%d found unique string nid=%d, id=%d\n",
                             method_name, __LINE__, nid, id.id);
            }
        }
    }

    if ( prepStmt != NULL )
        sqlite3_finalize( prepStmt );

    TRACE_EXIT;

    return result;
}


int CConfigContainer::getMaxUniqueId( int nid )
{
    const char method_name[] = "CConfigContainer::getMaxUniqueId";
    TRACE_ENTRY;


    if ( db_ == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d cannot use database since database open"
                         " failed.\n", method_name, __LINE__);
        }

        TRACE_EXIT;
        return false;
    }


    int result = 0;
    int rc;
    const char * sqlStmt;
    sqlStmt = "select max(id) from monRegUniqueStrings where nid=?";

    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, strlen(sqlStmt)+1, &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_int( prepStmt, 1, nid );

        rc = sqlite3_step( prepStmt );

        if ( rc == SQLITE_ROW )
        {
            result = sqlite3_column_int(prepStmt, 0);

            if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
            {
                trace_printf("%s@%d found max(id)=%d for nid=%d in "
                             "monRegUniqueStrings\n", method_name, __LINE__,
                             result, nid);
            }
        }
        else
        {
            if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
            {
                trace_printf("%s@%d finding max(id) in monRegUniqueStrings "
                             "for nid=%d, error=%s (%d)\n", method_name, __LINE__,
                             nid, sqlite3_errmsg(db_), rc);
            }

            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] finding max(id) in monRegUnique"
                      "Strings for nid=%d, error=%s (%d)\n",
                      method_name, nid, sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }
    }
        
    if ( prepStmt != NULL )
        sqlite3_finalize( prepStmt );

    TRACE_EXIT;

    return result;
}

void CConfigContainer::strIdToString( strId_t stringId,  string & value )
{
    const char method_name[] = "CConfigContainer::getUniqueString";
    TRACE_ENTRY;

    int rc;
    const char * selStmt;
    sqlite3_stmt * prepStmt;

    // Read process configuration registry entries and populate in-memory
    // structures.
    selStmt = "select dataValue from monRegUniqueStrings where nid = ? and id = ?";
    rc = sqlite3_prepare_v2( db_, selStmt, strlen(selStmt)+1, &prepStmt, NULL);

    if ( rc != SQLITE_OK )
    {
        if (trace_settings & TRACE_REQUEST)
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_int ( prepStmt, 1, stringId.nid );
        sqlite3_bind_int ( prepStmt, 2, stringId.id );

        rc = sqlite3_step( prepStmt );

        if ( rc == SQLITE_ROW )
        {
            value.assign( (const char *) sqlite3_column_text(prepStmt, 0) );

            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf("%s@%d retrieved unique string (%d, %d), "
                             "value=%s\n", method_name, __LINE__,
                             stringId.nid, stringId.id, value.c_str());
            }
        }
        else
        {
            value.assign( "" );

            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] unable to retrieve unique string"
                      " (%d, %d), error=%s (%d)\n",
                      method_name,  stringId.nid, stringId.id,
                      sqlite3_errmsg(db_), rc );
            mon_log_write( MON_DATABASE_ERROR, SQ_LOG_ERR, buf );
        }
        
    }
    
    if ( prepStmt != NULL )
        sqlite3_finalize( prepStmt );

    TRACE_EXIT;
}
