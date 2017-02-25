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
        mon_log_write(MON_CONFIGGROUP_SENDCHANGENOTICE_1, SQ_LOG_ERR, buf);
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
{
    const char method_name[] = "CConfigContainer::CConfigContainer";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "CCTR", 4);

    cluster_ = AddGroup( (char *)"CLUSTER", ConfigType_Cluster );
    if ( ! getenv("SQ_VIRTUAL_NODES") )
    {
        int pnid = Nodes->GetClusterConfig()->GetPNid(Node_name);
        snprintf(localNodeName_, sizeof(localNodeName_), "NODE%d",pnid);
    }
    else
    {
        snprintf(localNodeName_, sizeof(localNodeName_), "NODE%d",MyPNID);
    }
    node_ = AddGroup( localNodeName_, ConfigType_Node );

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
    const char method_name[] = "CConfigContainer::Init";
    TRACE_ENTRY;

    int rc;
    int regClusterCount = 0;
    int regProcessCount = 0;
    int regClusterMax = 0;
    int regProcessMax = 0;
    ConfigType type;
    registry_configuration_t *regClusterConfig;
    registry_configuration_t *regProcessConfig;
    
    // Get cluster scope configuration registry entries count
    rc = tc_get_registry_cluster_set( &regClusterCount
                                    , regClusterMax
                                    , NULL );
    if ( rc )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] Node configuration access failed!\n"
                , method_name );
        mon_log_write(MON_CONFIGCONT_INIT_1, SQ_LOG_ERR, la_buf);
    }

    regClusterMax = regClusterCount;
    regClusterConfig = new registry_configuration_t[regClusterMax];
    if (regClusterConfig)
    {
        rc = tc_get_registry_cluster_set( &regClusterCount
                                        , regClusterMax
                                        , regClusterConfig );
        if ( rc )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] Cluster scope configuration registry access failed!\n"
                    , method_name );
            mon_log_write(MON_CONFIGCONT_INIT_2, SQ_LOG_CRIT, la_buf);
        }
        
        type = ConfigType_Cluster;

        // Process cluster scope configuration registry entries
        for (int i = 0; i < regClusterCount; i++ )
        {
            Set( regClusterConfig[i].scope
               , type
               , regClusterConfig[i].key
               , regClusterConfig[i].value
               , false );
        }
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] Node configuration access failed!\n"
                , method_name );
        mon_log_write(MON_CONFIGCONT_INIT_3, SQ_LOG_CRIT, la_buf);
    }

    // Get process scope configuration registry entries count
    rc = tc_get_registry_cluster_set( &regProcessCount
                                    , regProcessMax
                                    , NULL );
    if ( rc )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] Node configuration access failed!\n"
                , method_name );
        mon_log_write(MON_CONFIGCONT_INIT_4, SQ_LOG_ERR, la_buf);
    }

    regProcessMax = regProcessCount;
    regProcessConfig = new registry_configuration_t[regProcessMax];
    if (regClusterConfig)
    {
        rc = tc_get_registry_cluster_set( &regProcessCount
                                        , regProcessMax
                                        , regProcessConfig );
        if ( rc )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] Process scope configuration registry access failed!\n"
                    , method_name );
            mon_log_write(MON_CONFIGCONT_INIT_5, SQ_LOG_CRIT, la_buf);
        }
        
        type = ConfigType_Process;

        // Process cluster scope configuration registry entries
        for (int i = 0; i < regClusterCount; i++ )
        {
            Set( regProcessConfig[i].scope
               , type
               , regProcessConfig[i].key
               , regProcessConfig[i].value
               , false );
        }
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] Node configuration access failed!\n"
                , method_name );
        mon_log_write(MON_CONFIGCONT_INIT_6, SQ_LOG_CRIT, la_buf);
    }

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

    int rc;
    
    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d saving registry key=%s\n"
                    , method_name, __LINE__
                    , key );
    }

    rc = tc_put_registry_key( key );
    if ( rc )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] tc_put_registry_key() failed, error=%d (%s)\n"
                , method_name, rc, tc_errmsg( rc ) );
        mon_log_write( MON_CONFIGCONT_ADDKEYNAME_1, SQ_LOG_ERR, buf );
    }

    TRACE_EXIT;
}

// insert key into monRegProcName table
void CConfigContainer::addDbProcName ( const char * name )
{
    const char method_name[] = "CConfigContainer::addDbProcName";
    TRACE_ENTRY;

    int rc;
    
    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d saving registry process, name=%s\n"
                    , method_name, __LINE__
                    , name );
    }

    rc = tc_put_registry_process( name );
    if ( rc )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] tc_put_registry_process() failed, error=%d (%s)\n"
                , method_name, rc, tc_errmsg( rc ) );
        mon_log_write( MON_CONFIGCONT_ADDPROCNAME_1, SQ_LOG_ERR, buf );
    }

    TRACE_EXIT;
}

void CConfigContainer::addDbClusterData ( const char * key,
                                          const char * dataValue )
{
    const char method_name[] = "CConfigContainer::addDbClusterData";
    TRACE_ENTRY;

    int rc;
    
    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d saving registry cluster data key=%s\n"
                    , method_name, __LINE__
                    , key);
    }

    rc = tc_put_registry_cluster_data( key, dataValue );
    if ( rc )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] tc_put_registry_cluster_data() failed, error=%d (%s)\n"
                , method_name, rc, tc_errmsg( rc ) );
        mon_log_write( MON_CONFIGCONT_ADDCLUSTERDATA_1, SQ_LOG_ERR, buf );
    }

    TRACE_EXIT;
}

void CConfigContainer::addDbProcData ( const char * procName,
                                       const char * key,
                                       const char * dataValue )
{
    const char method_name[] = "CConfigContainer::addDbProcData";
    TRACE_ENTRY;

    int rc;
    
    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d saving registry process data procName=%s, "
                      "key=%s\n"
                    , method_name, __LINE__
                    , procName, key );
    }

    rc = tc_put_registry_process_data( procName, key, dataValue );
    if ( rc )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] tc_put_registry_process_data() failed, error=%d (%s)\n"
                , method_name, rc, tc_errmsg( rc ) );
        mon_log_write( MON_CONFIGCONT_ADDPROCDATA_1, SQ_LOG_ERR, buf );
    }

    TRACE_EXIT;
}

void CConfigContainer::addUniqueString(int nid, int id, const char * uniqStr )
{
    const char method_name[] = "CConfigContainer::addUniqueString";
    TRACE_ENTRY;

    int rc;
    
    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d saving unique string nid=%d id=%d\n"
                    , method_name, __LINE__
                    , nid, id );
    }

    rc = tc_put_unique_string( nid, id, uniqStr );
    if ( rc )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] tc_put_unique_string() failed, error=%d (%s)\n"
                , method_name, rc, tc_errmsg( rc ) );
        mon_log_write( MON_CONFIGCONT_ADDUNIQUESTRING_1, SQ_LOG_ERR, buf );
    }

    TRACE_EXIT;
}

bool CConfigContainer::findUniqueString( int         nid
                                       , const char *uniqStr
                                       , strId_t    &strId )
{
    const char method_name[] = "CConfigContainer::findUniqueString";
    TRACE_ENTRY;

    bool result = false;
    int rc;
    int id;

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d finding unique string nid=%d string=%s\n"
                    , method_name, __LINE__
                    , nid, uniqStr );
    }

    rc = tc_get_unique_string_id( nid, uniqStr, &id );
    if ( rc )
    {
        if ( rc != TCDBNOEXIST )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] tc_get_unique_string_id() failed, ""error=%d (%s)\n"
                    , method_name, rc, tc_errmsg( rc ) );
            mon_log_write( MON_CONFIGCONT_FINDUNIQUESTRING_1, SQ_LOG_ERR, buf );
        }
    }
    else
    {
        strId.nid = nid;
        strId.id = id;
        result = true;
    }

    TRACE_EXIT;
    return result;
}

int CConfigContainer::getMaxUniqueId( int nid )
{
    const char method_name[] = "CConfigContainer::getMaxUniqueId";
    TRACE_ENTRY;

    int id = 0;
    int rc;

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d finding max unique string id for nid=%d\n"
                    , method_name, __LINE__
                    , nid );
    }

    rc = tc_get_unique_string_id_max( nid, &id );
    if ( rc )
    {
        if ( rc != TCDBNOEXIST )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] tc_get_unique_string_id_max() failed, error=%d (%s)\n"
                    , method_name, rc, tc_errmsg( rc ) );
            mon_log_write( MON_CONFIGCONT_GETUNIQUESTRIDMAX_1, SQ_LOG_ERR, buf );
        }
    }
    else
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf( "%s@%d found max(id)=%d for nid=%d\n"
                        , method_name, __LINE__, id, nid);
        }
    }

    TRACE_EXIT;
    return id;
}

void CConfigContainer::strIdToString( strId_t stringId,  string & value )
{
    const char method_name[] = "CConfigContainer::strIdToString";
    TRACE_ENTRY;

    int rc;
    char uniqueString[TC_UNIQUE_STRING_VALUE_MAX] = { 0 };

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d Get unique string, stringId(nid=%d, id=%d)\n"
                    , method_name, __LINE__
                    , stringId.nid, stringId.id );
    }

    rc = tc_get_unique_string( stringId.nid, stringId.id, uniqueString );
    if ( rc )
    {
        value.assign( "" );
        if ( rc != TCDBNOEXIST )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] tc_get_unique_string() failed, ""error=%d (%s)\n"
                    , method_name, rc, tc_errmsg( rc ) );
            mon_log_write( MON_CONFIGCONT_STRINGIDTPSTRING_1, SQ_LOG_ERR, buf );
        }
    }
    else
    {
        value.assign( uniqueString  );
    }

    TRACE_EXIT;
}
